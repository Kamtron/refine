
/* Copyright 2006, 2014, 2021 United States Government as represented
 * by the Administrator of the National Aeronautics and Space
 * Administration. No copyright is claimed in the United States under
 * Title 17, U.S. Code.  All Other Rights Reserved.
 *
 * The refine version 3 unstructured grid adaptation platform is
 * licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "ref_layer.h"

#include <stdio.h>
#include <stdlib.h>

#include "ref_cavity.h"
#include "ref_cloud.h"
#include "ref_edge.h"
#include "ref_egads.h"
#include "ref_export.h"
#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_matrix.h"
#include "ref_metric.h"
#include "ref_mpi.h"
#include "ref_phys.h"
#include "ref_sort.h"
#include "ref_split.h"
#include "ref_validation.h"

REF_FCN REF_STATUS ref_layer_create(REF_LAYER *ref_layer_ptr, REF_MPI ref_mpi) {
  REF_LAYER ref_layer;

  ref_malloc(*ref_layer_ptr, 1, REF_LAYER_STRUCT);

  ref_layer = *ref_layer_ptr;

  RSS(ref_list_create(&(ref_layer_list(ref_layer))), "create list");
  RSS(ref_grid_create(&(ref_layer_grid(ref_layer)), ref_mpi), "create grid");

  ref_layer->nnode_per_layer = REF_EMPTY;
  ref_layer->verbose = REF_FALSE;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_layer_free(REF_LAYER ref_layer) {
  if (NULL == (void *)ref_layer) return REF_NULL;

  ref_grid_free(ref_layer_grid(ref_layer));
  ref_list_free(ref_layer_list(ref_layer));
  ref_free(ref_layer);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_layer_attach(REF_LAYER ref_layer, REF_GRID ref_grid,
                                    REF_INT faceid) {
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];

  /* copy nodes into local copy that provides compact index */
  each_ref_cell_valid_cell_with_nodes(
      ref_cell, cell, nodes) if (faceid == nodes[ref_cell_node_per(ref_cell)])
      RSS(ref_list_push(ref_layer_list(ref_layer), cell), "parent");

  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_layer_normal(REF_LAYER ref_layer,
                                           REF_GRID ref_grid, REF_INT node,
                                           REF_DBL *norm) {
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT i, item, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL contains;
  REF_DBL angle, total, triangle_norm[3];

  total = 0.0;
  norm[0] = 0.0;
  norm[1] = 0.0;
  norm[2] = 0.0;

  each_ref_cell_having_node(ref_cell, node, item, cell) {
    RSS(ref_list_contains(ref_layer_list(ref_layer), cell, &contains),
        "in layer");
    if (!contains) continue;
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "tri nodes");
    RSS(ref_node_tri_node_angle(ref_grid_node(ref_grid), nodes, node, &angle),
        "angle");
    RSS(ref_node_tri_normal(ref_grid_node(ref_grid), nodes, triangle_norm),
        "norm");
    RSS(ref_math_normalize(triangle_norm), "normalize tri norm");
    total += angle;
    for (i = 0; i < 3; i++) norm[i] += angle * triangle_norm[i];
  }

  if (!ref_math_divisible(norm[0], total) ||
      !ref_math_divisible(norm[1], total) ||
      !ref_math_divisible(norm[2], total))
    return REF_DIV_ZERO;

  for (i = 0; i < 3; i++) norm[i] /= total;

  RSS(ref_math_normalize(norm), "normalize average norm");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_layer_puff(REF_LAYER ref_layer, REF_GRID ref_grid) {
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_NODE layer_node = ref_grid_node(ref_layer_grid(ref_layer));
  REF_CELL layer_prism = ref_grid_pri(ref_layer_grid(ref_layer));
  REF_CELL layer_edge = ref_grid_edg(ref_layer_grid(ref_layer));
  REF_INT item, cell, cell_node, cell_edge, nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT prism[REF_CELL_MAX_SIZE_PER];
  REF_INT new_cell;
  REF_INT node, local, i, nnode_per_layer;
  REF_GLOB global;
  REF_DBL norm[3];

  /* first layer of nodes */
  each_ref_list_item(ref_layer_list(ref_layer), item) {
    cell = ref_list_value(ref_layer_list(ref_layer), item);
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "nodes");
    each_ref_cell_cell_node(ref_cell, cell_node) {
      RSS(ref_node_add(layer_node, nodes[cell_node], &node), "add");
      for (i = 0; i < 3; i++)
        ref_node_xyz(layer_node, i, node) =
            ref_node_xyz(ref_grid_node(ref_grid), i, nodes[cell_node]);
    }
  }
  nnode_per_layer = ref_node_n(layer_node);
  ref_layer->nnode_per_layer = nnode_per_layer;

  /* second layer of nodes */
  for (local = 0; local < nnode_per_layer; local++) {
    global = (REF_GLOB)local + ref_node_n_global(ref_grid_node(ref_grid));
    RSS(ref_node_add(layer_node, global, &node), "add");
    RSS(ref_layer_normal(ref_layer, ref_grid,
                         (REF_INT)ref_node_global(layer_node, local), norm),
        "normal");
    for (i = 0; i < 3; i++)
      ref_node_xyz(layer_node, i, node) =
          0.1 * norm[i] + ref_node_xyz(layer_node, i, local);
  }

  /* layer of prisms */
  each_ref_list_item(ref_layer_list(ref_layer), item) {
    cell = ref_list_value(ref_layer_list(ref_layer), item);
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "nodes");
    each_ref_cell_cell_node(ref_cell, cell_node) {
      RSS(ref_node_local(layer_node, nodes[cell_node], &local), "local");
      prism[cell_node] = local;
      prism[3 + cell_node] = local + nnode_per_layer;
    }
    RSS(ref_cell_add(layer_prism, prism, &new_cell), "add");
  }

  /* constrain faces */
  each_ref_list_item(ref_layer_list(ref_layer), item) {
    cell = ref_list_value(ref_layer_list(ref_layer), item);
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "nodes");
    each_ref_cell_cell_edge(ref_cell, cell_edge) {
      REF_INT node0;
      REF_INT node1;
      REF_INT ncell, cell_list[2];
      REF_BOOL contains0, contains1;
      REF_INT edge_nodes[REF_CELL_MAX_SIZE_PER];
      node0 = nodes[ref_cell_e2n_gen(ref_cell, 0, cell_edge)];
      node1 = nodes[ref_cell_e2n_gen(ref_cell, 1, cell_edge)];
      RSS(ref_cell_list_with2(ref_cell, node0, node1, 2, &ncell, cell_list),
          "find with 2");
      REIS(2, ncell, "expected two tri for tri side");
      RSS(ref_list_contains(ref_layer_list(ref_layer), cell_list[0],
                            &contains0),
          "0 in layer");
      RSS(ref_list_contains(ref_layer_list(ref_layer), cell_list[1],
                            &contains1),
          "1 in layer");
      if (contains0 && contains1) continue; /* tri side interior to layer */
      if (!contains0 && !contains1) THROW("tri side is not in layer");
      RSS(ref_node_local(layer_node, node0, &local), "local");
      edge_nodes[0] = local + nnode_per_layer;
      RSS(ref_node_local(layer_node, node1, &local), "local");
      edge_nodes[1] = local + nnode_per_layer;
      if (contains0) {
        REIS(cell, cell_list[0], "cell should be in layer");
        edge_nodes[2] = ref_cell_c2n(ref_cell, 3, cell_list[1]);
      }
      if (contains1) {
        REIS(cell, cell_list[1], "cell should be in layer");
        edge_nodes[2] = ref_cell_c2n(ref_cell, 3, cell_list[0]);
      }
      RSS(ref_cell_add(layer_edge, edge_nodes, &new_cell), "add");
    }
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_layer_insert(REF_LAYER ref_layer, REF_GRID ref_grid) {
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_NODE layer_node = ref_grid_node(ref_layer_grid(ref_layer));
  REF_INT nnode_per_layer, node, local;
  REF_GLOB global;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER], node0, node1, node2, node3;
  REF_INT tet, i, new_node;
  REF_DBL bary[4];
  REF_INT zeros;
  REF_DBL zero_tol = 1.0e-10;
  REF_BOOL has_support;

  nnode_per_layer = ref_layer->nnode_per_layer;

  for (node = 0; node < nnode_per_layer; node++) {
    global = ref_node_global(layer_node, node); /* base */
    tet = ref_adj_first(ref_cell_adj(ref_cell), global);
    local = node + nnode_per_layer; /* target */
    RSS(ref_grid_enclosing_tet(ref_grid, ref_node_xyz_ptr(layer_node, local),
                               &tet, bary),
        "enclosing tet");
    RSS(ref_cell_nodes(ref_cell, tet, nodes), "nodes");
    zeros = 0;
    for (i = 0; i < 4; i++)
      if (ABS(bary[i]) < zero_tol) zeros++;
    new_node = REF_EMPTY;
    switch (zeros) {
      case 2: /* split an edge */
        node0 = REF_EMPTY;
        node1 = REF_EMPTY;
        for (i = 0; i < 4; i++)
          if (ABS(bary[i]) >= zero_tol) {
            if (node0 == REF_EMPTY) {
              node0 = i;
              continue;
            }
            if (node1 == REF_EMPTY) {
              node1 = i;
              continue;
            }
            THROW("more non-zeros than nodes");
          }
        if (node0 == REF_EMPTY) THROW("non-zero node0 missing");
        if (node1 == REF_EMPTY) THROW("non-zero node1 missing");
        node0 = nodes[node0];
        node1 = nodes[node1];
        RSS(ref_geom_supported(ref_grid_geom(ref_grid), node1, &has_support),
            "got geom?");
        if (has_support) RSS(REF_IMPLEMENT, "add geometry handling");
        RSS(ref_geom_supported(ref_grid_geom(ref_grid), node0, &has_support),
            "got geom?");
        if (has_support) RSS(REF_IMPLEMENT, "add geometry handling");
        RSS(ref_node_next_global(ref_node, &global), "next global");
        RSS(ref_node_add(ref_node, global, &new_node), "new node");
        RSS(ref_node_interpolate_edge(ref_node, node0, node1, 0.5, new_node),
            "interp new node");
        RSS(ref_geom_add_between(ref_grid, node0, node1, 0.5, new_node),
            "geom new node");
        RSS(ref_geom_constrain(ref_grid, new_node), "geom constraint");
        RSS(ref_split_edge(ref_grid, node0, node1, new_node), "split");
        for (i = 0; i < 3; i++)
          ref_node_xyz(ref_node, i, new_node) =
              ref_node_xyz(layer_node, i, local);
        break;
      case 1: /* split a triangular face*/
        node0 = REF_EMPTY;
        for (i = 0; i < 4; i++)
          if (ABS(bary[i]) < zero_tol) {
            if (node0 == REF_EMPTY) {
              node0 = i;
              continue;
            }
            THROW("more zeros than nodes");
          }
        if (node0 == REF_EMPTY) THROW("non-zero node0 missing");
        node1 = ref_cell_f2n_gen(ref_cell, 0, node0);
        node2 = ref_cell_f2n_gen(ref_cell, 1, node0);
        node3 = ref_cell_f2n_gen(ref_cell, 2, node0);
        node1 = nodes[node1];
        node2 = nodes[node2];
        node3 = nodes[node3];
        RSS(ref_geom_supported(ref_grid_geom(ref_grid), node1, &has_support),
            "got geom?");
        if (has_support) RSS(REF_IMPLEMENT, "add geometry handling");
        RSS(ref_geom_supported(ref_grid_geom(ref_grid), node2, &has_support),
            "got geom?");
        if (has_support) RSS(REF_IMPLEMENT, "add geometry handling");
        RSS(ref_geom_supported(ref_grid_geom(ref_grid), node3, &has_support),
            "got geom?");
        if (has_support) RSS(REF_IMPLEMENT, "add geometry handling");
        RSS(ref_node_next_global(ref_node, &global), "next global");
        RSS(ref_node_add(ref_node, global, &new_node), "new node");
        RSS(ref_node_interpolate_face(ref_node, node1, node2, node3, new_node),
            "interp new node");
        for (i = 0; i < 3; i++)
          ref_node_xyz(ref_node, i, new_node) =
              ref_node_xyz(layer_node, i, local);
        RSS(ref_split_face(ref_grid, node1, node2, node3, new_node), "fsplit");
        if (ref_layer->verbose)
          printf("split zeros %d bary %f %f %f %f\n", zeros, bary[0], bary[1],
                 bary[2], bary[3]);
        break;
      default:
        if (ref_layer->verbose)
          printf("implement zeros %d bary %f %f %f %f\n", zeros, bary[0],
                 bary[1], bary[2], bary[3]);
        RSS(REF_IMPLEMENT, "missing a general case");
        break;
    }
    RAS(REF_EMPTY != new_node, "new_node not set");
    layer_node->global[local] = new_node;

    if (ref_layer->verbose) RSS(ref_validation_cell_volume(ref_grid), "vol");
  }

  RSS(ref_node_rebuild_sorted_global(layer_node), "rebuild");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_layer_recon(REF_LAYER ref_layer, REF_GRID ref_grid) {
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_NODE layer_node = ref_grid_node(ref_layer_grid(ref_layer));
  REF_CELL layer_edge = ref_grid_edg(ref_layer_grid(ref_layer));
  REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL has_side;
  REF_INT node0, node1;

  each_ref_cell_valid_cell_with_nodes(layer_edge, cell, nodes) {
    node0 = (REF_INT)ref_node_global(layer_node, nodes[0]);
    node1 = (REF_INT)ref_node_global(layer_node, nodes[1]);
    RSS(ref_cell_has_side(ref_cell, node0, node1, &has_side), "side?");
    if (has_side) {
      if (ref_layer->verbose) printf("got one\n");
    } else {
      if (ref_layer->verbose) printf("need one\n");
    }
  }

  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_layer_quad_right_triangles(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL tri = ref_grid_tri(ref_grid);
  REF_EDGE ref_edge;
  REF_DBL *dots;
  REF_INT edge, *order, o;
  RSS(ref_edge_create(&ref_edge, ref_grid), "orig edges");
  ref_malloc_init(dots, ref_edge_n(ref_edge), REF_DBL, 2.0);
  ref_malloc(order, ref_edge_n(ref_edge), REF_INT);
  for (edge = 0; edge < ref_edge_n(ref_edge); edge++) {
    REF_INT n0, n1;
    REF_INT ntri, tri_list[2];
    n0 = ref_edge_e2n(ref_edge, 0, edge);
    n1 = ref_edge_e2n(ref_edge, 1, edge);

    RSS(ref_cell_list_with2(tri, n0, n1, 2, &ntri, tri_list), "tri with2");
    if (2 == ntri) {
      REF_INT t, cell_node, n2, n3, i;
      REF_DBL e0[3], e1[3];
      t = tri_list[0];
      n2 = REF_EMPTY;
      each_ref_cell_cell_node(tri, cell_node) {
        if (ref_cell_c2n(tri, cell_node, t) != n0 &&
            ref_cell_c2n(tri, cell_node, t) != n1) {
          n2 = ref_cell_c2n(tri, cell_node, t);
        }
      }
      RAS(REF_EMPTY != n2, "n2 not found");
      t = tri_list[1];
      n3 = REF_EMPTY;
      each_ref_cell_cell_node(tri, cell_node) {
        if (ref_cell_c2n(tri, cell_node, t) != n0 &&
            ref_cell_c2n(tri, cell_node, t) != n1) {
          n3 = ref_cell_c2n(tri, cell_node, t);
        }
      }
      RAS(REF_EMPTY != n3, "n3 not found");
      /* n2 corner */
      for (i = 0; i < 3; i++)
        e0[i] = ref_node_xyz(ref_node, i, n0) - ref_node_xyz(ref_node, i, n2);
      for (i = 0; i < 3; i++)
        e1[i] = ref_node_xyz(ref_node, i, n1) - ref_node_xyz(ref_node, i, n2);
      RSS(ref_math_normalize(e0), "norm e0");
      RSS(ref_math_normalize(e1), "norm e1");
      dots[edge] = ABS(ref_math_dot(e0, e1));
      /* n3 corner */
      for (i = 0; i < 3; i++)
        e0[i] = ref_node_xyz(ref_node, i, n0) - ref_node_xyz(ref_node, i, n3);
      for (i = 0; i < 3; i++)
        e1[i] = ref_node_xyz(ref_node, i, n1) - ref_node_xyz(ref_node, i, n3);
      RSS(ref_math_normalize(e0), "norm e0");
      RSS(ref_math_normalize(e1), "norm e1");
      dots[edge] = MAX(ABS(ref_math_dot(e0, e1)), dots[edge]);
      /* n0 corner */
      for (i = 0; i < 3; i++)
        e0[i] = ref_node_xyz(ref_node, i, n2) - ref_node_xyz(ref_node, i, n0);
      for (i = 0; i < 3; i++)
        e1[i] = ref_node_xyz(ref_node, i, n3) - ref_node_xyz(ref_node, i, n0);
      RSS(ref_math_normalize(e0), "norm e0");
      RSS(ref_math_normalize(e1), "norm e1");
      dots[edge] = MAX(ABS(ref_math_dot(e0, e1)), dots[edge]);
      /* n1 corner */
      for (i = 0; i < 3; i++)
        e0[i] = ref_node_xyz(ref_node, i, n2) - ref_node_xyz(ref_node, i, n1);
      for (i = 0; i < 3; i++)
        e1[i] = ref_node_xyz(ref_node, i, n3) - ref_node_xyz(ref_node, i, n1);
      RSS(ref_math_normalize(e0), "norm e0");
      RSS(ref_math_normalize(e1), "norm e1");
      dots[edge] = MAX(ABS(ref_math_dot(e0, e1)), dots[edge]);
    }
  }
  RSS(ref_sort_heap_dbl(ref_edge_n(ref_edge), dots, order), "sort dots");

  for (o = 0; o < ref_edge_n(ref_edge); o++) {
    REF_INT n0, n1;
    REF_INT ntri, tri_list[2];
    edge = order[o];
    if (dots[edge] < 0.1736) { /* sin(10 degrees) */
      n0 = ref_edge_e2n(ref_edge, 0, edge);
      n1 = ref_edge_e2n(ref_edge, 1, edge);

      RSS(ref_cell_list_with2(tri, n0, n1, 2, &ntri, tri_list), "tri with2");
      if (2 == ntri) {
        REF_INT t, cell_node, n2, n3, tn, new_cell;
        REF_INT nodes[REF_CELL_MAX_NODE_PER];
        t = tri_list[0];
        n2 = REF_EMPTY;
        tn = REF_EMPTY;
        each_ref_cell_cell_node(tri, cell_node) {
          if (ref_cell_c2n(tri, cell_node, t) != n0 &&
              ref_cell_c2n(tri, cell_node, t) != n1) {
            n2 = ref_cell_c2n(tri, cell_node, t);
            tn = cell_node;
          }
        }
        RAS(REF_EMPTY != n2, "n2 not found");
        RAS(REF_EMPTY != tn, "tn not found")
        t = tri_list[1];
        n3 = REF_EMPTY;
        each_ref_cell_cell_node(tri, cell_node) {
          if (ref_cell_c2n(tri, cell_node, t) != n0 &&
              ref_cell_c2n(tri, cell_node, t) != n1) {
            n3 = ref_cell_c2n(tri, cell_node, t);
          }
        }
        RAS(REF_EMPTY != n3, "n3 not found");
        tn--;
        if (tn < 0) tn += 3;
        t = tri_list[0];
        nodes[0] = ref_cell_c2n(tri, tn, t);
        tn++;
        if (tn > 2) tn -= 3;
        nodes[1] = ref_cell_c2n(tri, tn, t);
        tn++;
        if (tn > 2) tn -= 3;
        nodes[2] = ref_cell_c2n(tri, tn, t);
        nodes[3] = n3;
        nodes[4] = ref_cell_c2n(tri, 3, t);
        RSS(ref_cell_add(ref_grid_qua(ref_grid), nodes, &new_cell), "add");
        RSS(ref_cell_remove(tri, tri_list[0]), "remove tri 0");
        RSS(ref_cell_remove(tri, tri_list[1]), "remove tri 1");
      }
    }
  }
  ref_free(order);
  ref_free(dots);
  RSS(ref_edge_free(ref_edge), "free edge");
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_layer_interior_seg_normal(REF_GRID ref_grid,
                                                 REF_INT cell,
                                                 REF_DBL *normal) {
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_CELL edg = ref_grid_edg(ref_grid);
  REF_CELL tri = ref_grid_tri(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT ntri, tri_list[2], other, t;
  REF_DBL tri_side[3];
  REF_DBL dot;
  RSS(ref_cell_nodes(edg, cell, nodes), "cell");
  RSS(ref_node_seg_normal(ref_node, nodes, normal), "normal");
  RSS(ref_math_normalize(normal), "norm");
  RSS(ref_cell_list_with2(tri, nodes[0], nodes[1], 2, &ntri, tri_list),
      "tri with2");
  REIS(1, ntri, "expected one tri at bounary");
  t = tri_list[0];
  other = ref_cell_c2n(tri, 0, t) + ref_cell_c2n(tri, 1, t) +
          ref_cell_c2n(tri, 2, t) - nodes[0] - nodes[1];
  tri_side[0] =
      ref_node_xyz(ref_node, 0, other) - ref_node_xyz(ref_node, 0, nodes[0]);
  tri_side[1] =
      ref_node_xyz(ref_node, 1, other) - ref_node_xyz(ref_node, 1, nodes[0]);
  tri_side[2] =
      ref_node_xyz(ref_node, 2, other) - ref_node_xyz(ref_node, 2, nodes[0]);
  RSS(ref_math_normalize(tri_side), "norm");
  dot = ref_math_dot(normal, tri_side);
  if (dot < 0) {
    normal[0] = -normal[0];
    normal[1] = -normal[1];
    normal[2] = -normal[2];
  }
  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_layer_twod_normal(REF_GRID ref_grid, REF_INT node,
                                                REF_DBL *normal) {
  REF_CELL ref_cell = ref_grid_edg(ref_grid);
  REF_DBL seg_normal[3];
  REF_INT item, cell;
  normal[0] = 0.0;
  normal[1] = 0.0;
  normal[2] = 0.0;
  each_ref_cell_having_node(ref_cell, node, item, cell) {
    RSS(ref_layer_interior_seg_normal(ref_grid, cell, seg_normal), "normal");
    normal[0] += seg_normal[0];
    normal[1] += seg_normal[1];
    normal[2] += seg_normal[2];
  }
  RSS(ref_math_normalize(normal), "norm");
  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_layer_align_first_layer(REF_GRID ref_grid,
                                                      REF_CLOUD ref_cloud,
                                                      REF_LIST ref_list) {
  REF_CELL ref_cell = ref_grid_edg(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_INT node;

  each_ref_node_valid_node(ref_node, node) {
    if (!ref_cell_node_empty(ref_cell, node)) {
      REF_DBL normal[3];

      REF_DBL d[12], m[6];
      REF_DBL dot;
      RSS(ref_layer_twod_normal(ref_grid, node, normal), "twod normal");
      RSS(ref_node_metric_get(ref_node, node, m), "get");
      RSS(ref_matrix_diag_m(m, d), "eigen decomp");
      RSS(ref_matrix_descending_eig_twod(d), "2D eig sort");
      dot = ref_math_dot(normal, &(d[3]));
      if (ABS(dot) > 0.9848) { /* cos(10 degrees) */
        REF_DBL h, xyz[3], dist;
        REF_INT closest_node;
        REF_INT new_node;
        REF_GLOB global;
        REF_INT type, id;
        REF_DBL uv[2];
        REF_CAVITY ref_cavity;
        h = 1.0 / sqrt(d[0]);
        xyz[0] = ref_node_xyz(ref_node, 0, node) + h * normal[0];
        xyz[1] = ref_node_xyz(ref_node, 1, node) + h * normal[1];
        xyz[2] = ref_node_xyz(ref_node, 2, node) + h * normal[2];
        RSS(ref_node_nearest_xyz(ref_node, xyz, &closest_node, &dist), "close");
        RSS(ref_node_next_global(ref_node, &global), "global");
        RSS(ref_cloud_push(ref_cloud, global, normal), "store cloud");
        RSS(ref_list_push(ref_list, node), "store list");
        RSS(ref_node_add(ref_node, global, &new_node), "add");
        ref_node_xyz(ref_node, 0, new_node) = xyz[0];
        ref_node_xyz(ref_node, 1, new_node) = xyz[1];
        ref_node_xyz(ref_node, 2, new_node) = xyz[2];
        type = REF_GEOM_FACE;
        RSS(ref_geom_unique_id(ref_geom, node, type, &id), "unique face id");
        RSS(ref_geom_tuv(ref_geom, node, type, id, uv), "uv");
        RSS(ref_egads_inverse_eval(ref_geom, type, id, xyz, uv), "inverse uv");
        RSS(ref_geom_add(ref_geom, new_node, type, id, uv), "new geom");
        RSS(ref_metric_interpolate_between(ref_grid, node, REF_EMPTY, new_node),
            "metric interp");
        RSS(ref_cavity_create(&ref_cavity), "cav create");
        RSS(ref_cavity_form_insert(ref_cavity, ref_grid, new_node, node,
                                   REF_EMPTY, REF_EMPTY),
            "ball");
        RSB(ref_cavity_enlarge_conforming(ref_cavity), "enlarge", {
          ref_cavity_tec(ref_cavity, "cav-fail.tec");
          ref_export_by_extension(ref_grid, "mesh-fail.tec");
        });
        RSB(ref_cavity_replace(ref_cavity), "cav replace", {
          ref_cavity_tec(ref_cavity, "ref_layer_align_quad_cavity.tec");
          ref_export_by_extension(ref_grid, "ref_layer_align_quad_mesh.tec");
          printf("norm %f %f %f dir %f %f %f dot %f\n", normal[0], normal[1],
                 normal[2], d[3], d[4], d[5], ref_math_dot(normal, &(d[3])));
          printf("new %f %f %f\n", xyz[0], xyz[1], xyz[2]);
        });
        RSS(ref_cavity_free(ref_cavity), "cav free");
      }
    }
  }
  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_layer_align_quad_advance(REF_GRID ref_grid,
                                                       REF_CLOUD last,
                                                       REF_LIST last_list,
                                                       REF_CLOUD next,
                                                       REF_LIST next_list) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_INT node, item;
  REF_GLOB global;

  each_ref_cloud_global(last, item, global) {
    REF_DBL normal[3];

    REF_DBL d[12], m[6];
    REF_DBL dot;
    REF_INT aux_index;
    RSS(ref_node_local(ref_node, global, &node), "local");
    each_ref_cloud_aux(last, aux_index) {
      normal[aux_index] = ref_cloud_aux(last, aux_index, item);
    }
    RSS(ref_node_metric_get(ref_node, node, m), "get");
    RSS(ref_matrix_diag_m(m, d), "eigen decomp");
    RSS(ref_matrix_descending_eig_twod(d), "2D eig sort");
    dot = ref_math_dot(normal, &(d[3]));
    if (ABS(dot) > 0.9848) { /* cos(10 degrees) */
      REF_DBL h, xyz[3], dist;
      REF_INT closest_node;
      REF_INT new_node;
      REF_INT type, id;
      REF_DBL uv[2];
      REF_CAVITY ref_cavity;
      h = 1.0 / sqrt(d[0]);
      xyz[0] = ref_node_xyz(ref_node, 0, node) + h * normal[0];
      xyz[1] = ref_node_xyz(ref_node, 1, node) + h * normal[1];
      xyz[2] = ref_node_xyz(ref_node, 2, node) + h * normal[2];
      RSS(ref_node_nearest_xyz(ref_node, xyz, &closest_node, &dist), "close");
      RSS(ref_node_next_global(ref_node, &global), "global");
      RSS(ref_cloud_store(next, global, normal), "store cloud");
      RSS(ref_list_push(next_list, node), "store list");
      RSS(ref_node_add(ref_node, global, &new_node), "add");
      ref_node_xyz(ref_node, 0, new_node) = xyz[0];
      ref_node_xyz(ref_node, 1, new_node) = xyz[1];
      ref_node_xyz(ref_node, 2, new_node) = xyz[2];
      type = REF_GEOM_FACE;
      RSS(ref_geom_unique_id(ref_geom, node, type, &id), "unique face id");
      RSS(ref_geom_tuv(ref_geom, node, type, id, uv), "uv");
      RSS(ref_egads_inverse_eval(ref_geom, type, id, xyz, uv), "inverse uv");
      RSS(ref_geom_add(ref_geom, new_node, type, id, uv), "new geom");
      RSS(ref_metric_interpolate_between(ref_grid, node, REF_EMPTY, new_node),
          "metric interp");
      RSS(ref_cavity_create(&ref_cavity), "cav create");
      RSS(ref_cavity_form_insert(ref_cavity, ref_grid, new_node, node,
                                 ref_list_value(last_list, item), REF_EMPTY),
          "ball");
      RSB(ref_cavity_enlarge_conforming(ref_cavity), "enlarge", {
        ref_cavity_tec(ref_cavity, "cav-fail.tec");
        ref_export_by_extension(ref_grid, "mesh-fail.tec");
      });
      if (REF_CAVITY_VISIBLE == ref_cavity_state(ref_cavity)) {
        RSB(ref_cavity_replace(ref_cavity), "cav replace", {
          ref_cavity_tec(ref_cavity, "ref_layer_align_quad_cavity.tec");
          ref_export_by_extension(ref_grid, "ref_layer_align_quad_mesh.tec");
          printf("norm %f %f %f dir %f %f %f dot %f\n", normal[0], normal[1],
                 normal[2], d[3], d[4], d[5], ref_math_dot(normal, &(d[3])));
          printf("new %f %f %f\n", xyz[0], xyz[1], xyz[2]);
        });
      } else {
        RSS(ref_node_remove(ref_node, new_node), "rm");
        RSS(ref_geom_remove_all(ref_geom, new_node), "rm");
        ref_cloud_n(next)--;
        ref_list_n(next_list)--;
      }
      RSS(ref_cavity_free(ref_cavity), "cav free");
    }
  }

  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_layer_align_quad_seq(REF_GRID ref_grid) {
  REF_INT layers = 2;
  REF_CLOUD previous_cloud, next_cloud;
  REF_LIST previous_list, next_list;
  RSS(ref_cloud_create(&previous_cloud, 3), "previous cloud");
  RSS(ref_list_create(&previous_list), "previous list");

  RSS(ref_layer_align_first_layer(ref_grid, previous_cloud, previous_list),
      "first layer");
  if (layers > 1) {
    RSS(ref_layer_quad_right_triangles(ref_grid), "tri2qaud");
    RSS(ref_cloud_create(&next_cloud, 3), "next cloud");
    RSS(ref_list_create(&next_list), "next list");
    RSS(ref_layer_align_quad_advance(ref_grid, previous_cloud, previous_list,
                                     next_cloud, next_list),
        "first layer");
    RSS(ref_cloud_free(previous_cloud), "free previous cloud");
    RSS(ref_list_free(previous_list), "free previous list");
    previous_cloud = next_cloud;
    previous_list = next_list;
    next_cloud = NULL;
    next_list = NULL;
  }

  RSS(ref_cloud_free(previous_cloud), "free next cloud");

  RSS(ref_layer_quad_right_triangles(ref_grid), "tri2qaud");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_layer_align_quad(REF_GRID ref_grid) {
  REF_MIGRATE_PARTIONER previous;
  previous = ref_grid_partitioner(ref_grid);
  ref_grid_partitioner(ref_grid) = REF_MIGRATE_SINGLE;
  RSS(ref_migrate_to_balance(ref_grid), "migrate to single part");
  RSS(ref_layer_align_quad_seq(ref_grid), "quad");
  ref_grid_partitioner(ref_grid) = previous;
  RSS(ref_migrate_to_balance(ref_grid), "migrate to single part");
  return REF_SUCCESS;
}

static REF_FCN REF_STATUS ref_layer_tet_prism(REF_INT *pri_nodes,
                                              REF_INT *tet_nodes,
                                              REF_BOOL *contains) {
  REF_INT hits, tet_node, pri_node;
  *contains = REF_FALSE;
  hits = 0;
  for (pri_node = 0; pri_node < 6; pri_node++) {
    for (tet_node = 0; tet_node < 4; tet_node++) {
      if (pri_nodes[pri_node] == tet_nodes[tet_node]) {
        hits++;
      }
    }
  }
  RAB(hits <= 4, "repeated tet nodes in prism", { printf("hits %d\n", hits); });
  if (4 == hits) *contains = REF_TRUE;
  return REF_SUCCESS;
}

static REF_FCN REF_STATUS ref_layer_tet_to_pyr(REF_GRID ref_grid, REF_INT pri,
                                               REF_INT tet0, REF_INT tet1) {
  REF_INT pri_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT tet0_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT tet1_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT pyr_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT cell_face, i, hits, quad_node, quad_face, new_pyr;

  RSS(ref_cell_nodes(ref_grid_pri(ref_grid), pri, pri_nodes), "pri nodes");
  RSS(ref_cell_nodes(ref_grid_tet(ref_grid), tet0, tet0_nodes), "tet0 nodes");
  RSS(ref_cell_nodes(ref_grid_tet(ref_grid), tet1, tet1_nodes), "tet1 nodes");

  quad_face = REF_EMPTY;
  each_ref_cell_cell_face(ref_grid_pri(ref_grid), cell_face) {
    if (ref_cell_f2n_gen(ref_grid_pri(ref_grid), 0, cell_face) !=
        ref_cell_f2n_gen(ref_grid_pri(ref_grid), 3, cell_face)) {
      hits = 0;
      for (i = 0; i < 4; i++) {
        quad_node =
            pri_nodes[ref_cell_f2n_gen(ref_grid_pri(ref_grid), i, cell_face)];
        if (quad_node == tet0_nodes[0] || quad_node == tet0_nodes[1] ||
            quad_node == tet0_nodes[2] || quad_node == tet0_nodes[3] ||
            quad_node == tet1_nodes[0] || quad_node == tet1_nodes[1] ||
            quad_node == tet1_nodes[2] || quad_node == tet1_nodes[3])
          hits++;
      }
      RAB(hits == 2 || hits == 4, "hits not 2 or 4",
          { printf("hits %d\n", hits); });
      if (4 == hits) {
        REIS(REF_EMPTY, quad_face, "two quad faces");
        quad_face = cell_face;
      }
    }
  }
  RUS(REF_EMPTY, quad_face, "quad face not set");
  /* aflr winding, quad points into prism */
  pyr_nodes[0] =
      pri_nodes[ref_cell_f2n_gen(ref_grid_pri(ref_grid), 0, quad_face)];
  pyr_nodes[1] =
      pri_nodes[ref_cell_f2n_gen(ref_grid_pri(ref_grid), 1, quad_face)];
  pyr_nodes[4] =
      pri_nodes[ref_cell_f2n_gen(ref_grid_pri(ref_grid), 2, quad_face)];
  pyr_nodes[3] =
      pri_nodes[ref_cell_f2n_gen(ref_grid_pri(ref_grid), 3, quad_face)];
  pyr_nodes[2] = REF_EMPTY;
  for (i = 0; i < 4; i++) {
    if (pyr_nodes[0] != tet0_nodes[i] && pyr_nodes[1] != tet0_nodes[i] &&
        pyr_nodes[4] != tet0_nodes[i] && pyr_nodes[3] != tet0_nodes[i]) {
      RAS(pyr_nodes[2] == REF_EMPTY || pyr_nodes[2] == tet0_nodes[i],
          "multiple off nodes");
      pyr_nodes[2] = tet0_nodes[i];
    }
    if (pyr_nodes[0] != tet1_nodes[i] && pyr_nodes[1] != tet1_nodes[i] &&
        pyr_nodes[4] != tet1_nodes[i] && pyr_nodes[3] != tet1_nodes[i]) {
      RAS(pyr_nodes[2] == REF_EMPTY || pyr_nodes[2] == tet1_nodes[i],
          "multiple off nodes");
      pyr_nodes[2] = tet1_nodes[i];
    }
  }
  RUS(REF_EMPTY, pyr_nodes[2], "pyramid peak not set");

  RSS(ref_cell_remove(ref_grid_tet(ref_grid), tet0), "rm tet0");
  RSS(ref_cell_remove(ref_grid_tet(ref_grid), tet1), "rm tet1");

  RSS(ref_cell_add(ref_grid_pyr(ref_grid), pyr_nodes, &new_pyr), "add pyr");

  return REF_SUCCESS;
}

static REF_FCN REF_STATUS ref_layer_seed_tet(REF_GRID ref_grid, REF_INT node0,
                                             REF_INT node1, REF_INT *tet) {
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT item, cell;
  REF_INT best_cell;
  REF_DBL best_vol;
  REF_INT cell_face, nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL vol, subtetvol;
  *tet = REF_EMPTY;
  best_cell = REF_EMPTY;
  best_vol = -REF_DBL_MAX;
  each_ref_cell_having_node(ref_cell, node0, item, cell) {
    each_ref_cell_cell_face(ref_cell, cell_face) {
      if (node0 == ref_cell_f2n(ref_cell, 0, cell_face, cell) ||
          node0 == ref_cell_f2n(ref_cell, 1, cell_face, cell) ||
          node0 == ref_cell_f2n(ref_cell, 2, cell_face, cell))
        continue;
      vol = REF_DBL_MAX;
      nodes[0] = node0;
      nodes[1] = node1;
      nodes[2] = ref_cell_f2n(ref_cell, 1, cell_face, cell);
      nodes[3] = ref_cell_f2n(ref_cell, 0, cell_face, cell);
      RSS(ref_node_tet_vol(ref_node, nodes, &subtetvol), "vol");
      vol = MIN(vol, subtetvol);
      nodes[0] = node0;
      nodes[1] = node1;
      nodes[2] = ref_cell_f2n(ref_cell, 2, cell_face, cell);
      nodes[3] = ref_cell_f2n(ref_cell, 1, cell_face, cell);
      RSS(ref_node_tet_vol(ref_node, nodes, &subtetvol), "vol");
      vol = MIN(vol, subtetvol);
      nodes[0] = node0;
      nodes[1] = node1;
      nodes[2] = ref_cell_f2n(ref_cell, 0, cell_face, cell);
      nodes[3] = ref_cell_f2n(ref_cell, 2, cell_face, cell);
      RSS(ref_node_tet_vol(ref_node, nodes, &subtetvol), "vol");
      vol = MIN(vol, subtetvol);
      if (vol > best_vol) {
        best_vol = vol;
        best_cell = cell;
      }
    }
  }
  RUS(REF_EMPTY, best_cell, "best tet empty");
  RAS(best_vol > 0, "best vol not positive");
  *tet = best_cell;
  return REF_SUCCESS;
}

static REF_FCN REF_STATUS ref_layer_swap(REF_GRID ref_grid, REF_INT node0,
                                         REF_INT node1, REF_INT site,
                                         REF_BOOL *complete) {
  REF_CAVITY ref_cavity;
  *complete = REF_FALSE;
  RSS(ref_cavity_create(&ref_cavity), "create");
  RSS(ref_cavity_form_edge_swap(ref_cavity, ref_grid, node0, node1, site),
      "form");
  RSS(ref_cavity_enlarge_combined(ref_cavity), "enlarge");
  if (REF_CAVITY_VISIBLE == ref_cavity_state(ref_cavity)) {
    RSS(ref_cavity_replace(ref_cavity), "replace");
    *complete = REF_TRUE;
  }
  RSS(ref_cavity_free(ref_cavity), "free");
  return REF_SUCCESS;
}

static REF_FCN REF_STATUS ref_layer_recover_face(REF_GRID ref_grid,
                                                 REF_INT *face_nodes) {
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT face_node0, face_node1, item, cell_node, cell, cell_edge;
  REF_INT node0, node1;
  REF_DBL t, uvw[3];
  REF_BOOL complete;
  face_node0 = face_nodes[0];
  face_node1 = face_nodes[1];
  each_ref_cell_having_node2(ref_cell, face_node0, face_node1, item, cell_node,
                             cell) {
    each_ref_cell_cell_edge(ref_cell, cell_edge) {
      node0 = ref_cell_e2n(ref_cell, 0, cell_edge, cell);
      node1 = ref_cell_e2n(ref_cell, 1, cell_edge, cell);
      if (node0 == face_node0 || node1 == face_node0 || node0 == face_node1 ||
          node1 == face_node1)
        continue;
      RSS(ref_node_tri_seg_intersection(ref_node, node0, node1, face_nodes, &t,
                                        uvw),
          "int");
      if (t > 0.0 && t < 1.0 && uvw[0] > 0.0 && uvw[1] > 0.0 && uvw[2] > 0.0) {
        printf("t %f u %f v %f w %f \n", t, uvw[0], uvw[1], uvw[2]);
        RSS(ref_layer_swap(ref_grid, node0, node1, face_nodes[0], &complete),
            "n0");
        if (complete) break;
        RSS(ref_layer_swap(ref_grid, node0, node1, face_nodes[1], &complete),
            "n1");
        if (complete) break;
        RSS(ref_layer_swap(ref_grid, node0, node1, face_nodes[2], &complete),
            "n2");
        if (complete) break;
        printf("nothing \n");
      }
    }
  }
  face_node0 = face_nodes[1];
  face_node1 = face_nodes[2];
  each_ref_cell_having_node2(ref_cell, face_node0, face_node1, item, cell_node,
                             cell) {
    each_ref_cell_cell_edge(ref_cell, cell_edge) {
      node0 = ref_cell_e2n(ref_cell, 0, cell_edge, cell);
      node1 = ref_cell_e2n(ref_cell, 1, cell_edge, cell);
      if (node0 == face_node0 || node1 == face_node0 || node0 == face_node1 ||
          node1 == face_node1)
        continue;
      RSS(ref_node_tri_seg_intersection(ref_node, node0, node1, face_nodes, &t,
                                        uvw),
          "int");
      if (t > 0.0 && t < 1.0 && uvw[0] > 0.0 && uvw[1] > 0.0 && uvw[2] > 0.0) {
        printf("t %f u %f v %f w %f \n", t, uvw[0], uvw[1], uvw[2]);
        RSS(ref_layer_swap(ref_grid, node0, node1, face_nodes[0], &complete),
            "n0");
        if (complete) break;
        RSS(ref_layer_swap(ref_grid, node0, node1, face_nodes[1], &complete),
            "n1");
        if (complete) break;
        RSS(ref_layer_swap(ref_grid, node0, node1, face_nodes[2], &complete),
            "n2");
        if (complete) break;
        printf("nothing \n");
      }
    }
  }
  face_node0 = face_nodes[2];
  face_node1 = face_nodes[0];
  each_ref_cell_having_node2(ref_cell, face_node0, face_node1, item, cell_node,
                             cell) {
    each_ref_cell_cell_edge(ref_cell, cell_edge) {
      node0 = ref_cell_e2n(ref_cell, 0, cell_edge, cell);
      node1 = ref_cell_e2n(ref_cell, 1, cell_edge, cell);
      if (node0 == face_node0 || node1 == face_node0 || node0 == face_node1 ||
          node1 == face_node1)
        continue;
      RSS(ref_node_tri_seg_intersection(ref_node, node0, node1, face_nodes, &t,
                                        uvw),
          "int");
      if (t > 0.0 && t < 1.0 && uvw[0] > 0.0 && uvw[1] > 0.0 && uvw[2] > 0.0) {
        printf("t %f u %f v %f w %f \n", t, uvw[0], uvw[1], uvw[2]);
        RSS(ref_layer_swap(ref_grid, node0, node1, face_nodes[0], &complete),
            "n0");
        if (complete) break;
        RSS(ref_layer_swap(ref_grid, node0, node1, face_nodes[1], &complete),
            "n1");
        if (complete) break;
        RSS(ref_layer_swap(ref_grid, node0, node1, face_nodes[2], &complete),
            "n2");
        if (complete) break;
        printf("nothing \n");
      }
    }
  }
  return REF_SUCCESS;
}

static REF_FCN REF_STATUS ref_layer_remove_sliver(REF_GRID ref_grid,
                                                  REF_INT *quad) {
  REF_BOOL complete;
  RSS(ref_layer_swap(ref_grid, quad[0], quad[2], quad[1], &complete), "quad1");
  if (complete) return REF_SUCCESS;
  RSS(ref_layer_swap(ref_grid, quad[0], quad[2], quad[3], &complete), "quad1");
  if (complete) return REF_SUCCESS;
  RSS(ref_layer_swap(ref_grid, quad[1], quad[3], quad[0], &complete), "quad0");
  if (complete) return REF_SUCCESS;
  RSS(ref_layer_swap(ref_grid, quad[1], quad[3], quad[2], &complete), "quad3");
  if (complete) return REF_SUCCESS;
  return REF_SUCCESS;
}

static REF_FCN REF_STATUS ref_layer_prism_insert_hair(REF_GRID ref_grid,
                                                      REF_DICT ref_dict_bcs,
                                                      REF_BOOL *active,
                                                      REF_INT *off_node) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);

  REF_INT node;
  REF_INT item, cell;
  REF_CAVITY ref_cavity;
  REF_DBL m[6], ratio, h;
  REF_GLOB global;
  REF_INT new_node;
  REF_INT bc;
  REF_BOOL constrained;
  REF_DBL tri_normal[3], normal[3];
  REF_INT faceid, constraining_faceid;
  REF_BOOL prevent_constrain = REF_TRUE;

  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node) && active[node]) {
      normal[0] = 0.0;
      normal[1] = 0.0;
      normal[2] = 0.0;
      constrained = REF_FALSE;
      constraining_faceid = REF_EMPTY;
      each_ref_cell_having_node(ref_cell, node, item, cell) {
        faceid = ref_cell_c2n(ref_cell, ref_cell_id_index(ref_cell), cell);
        RXS(ref_dict_value(ref_dict_bcs, faceid, &bc), REF_NOT_FOUND, "bc");
        if (ref_phys_wall_distance_bc(bc)) {
          RSS(ref_node_tri_normal(ref_node, &(ref_cell_c2n(ref_cell, 0, cell)),
                                  tri_normal),
              "tri norm");
          normal[0] += tri_normal[0];
          normal[1] += tri_normal[1];
          normal[2] += tri_normal[2];
        } else {
          constrained = REF_TRUE;
          RAS(constraining_faceid == REF_EMPTY || constraining_faceid == faceid,
              "multiple face constraints");
          constraining_faceid = faceid;
        }
      }
      RSS(ref_math_normalize(normal), "norm");
      RSS(ref_node_metric_get(ref_node, node, m), "get");
      ratio = ref_matrix_vt_m_v(m, normal);
      RAS(ratio > 0.0, "ratio not positive");
      ratio = sqrt(ratio);
      RAS(ref_math_divisible(1.0, ratio), "1/ratio is inf 1/0");
      h = 1.0 / ratio;
      RSS(ref_node_next_global(ref_node, &global), "next global");
      RSS(ref_node_add(ref_node, global, &new_node), "add");
      off_node[node] = new_node;
      ref_node_xyz(ref_node, 0, new_node) =
          ref_node_xyz(ref_node, 0, node) + h * normal[0];
      ref_node_xyz(ref_node, 1, new_node) =
          ref_node_xyz(ref_node, 1, node) + h * normal[1];
      ref_node_xyz(ref_node, 2, new_node) =
          ref_node_xyz(ref_node, 2, node) + h * normal[2];
      if (constrained) {
        REF_GEOM ref_geom = ref_grid_geom(ref_grid);
        REF_DBL param[2];
        RSS(ref_geom_tuv(ref_geom, node, REF_GEOM_FACE, constraining_faceid,
                         param),
            "base param");
        RSS(ref_egads_inverse_eval(ref_geom, REF_GEOM_FACE, constraining_faceid,
                                   ref_node_xyz_ptr(ref_node, new_node), param),
            "inv eval");
        RSS(ref_egads_eval_at(ref_geom, REF_GEOM_FACE, constraining_faceid,
                              param, ref_node_xyz_ptr(ref_node, new_node),
                              NULL),
            "inv eval");
        RSS(ref_geom_add(ref_geom, new_node, REF_GEOM_FACE, constraining_faceid,
                         param),
            "add param");
        if (prevent_constrain) {
          off_node[node] = REF_EMPTY;
          RSS(ref_geom_remove_all(ref_geom, new_node), "rm geom");
          RSS(ref_node_remove(ref_node, new_node), "rm node");
          continue;
        } else {
          RSS(ref_cavity_create(&ref_cavity), "cav create");
          if (ref_cavity_debug(ref_cavity)) printf("cavity debug start\n");
          RSS(ref_cavity_form_insert2(ref_cavity, ref_grid, new_node, node,
                                      REF_EMPTY, constraining_faceid),
              "ball");
          if (REF_CAVITY_UNKNOWN != ref_cavity_state(ref_cavity)) {
            printf(" form state %d error\n", (int)ref_cavity_state(ref_cavity));
            ref_cavity_tec(ref_cavity, "cav-form.tec");
          }
          RSB(ref_cavity_enlarge_combined(ref_cavity), "enlarge", {
            ref_cavity_tec(ref_cavity, "cav-fail.tec");
            ref_export_by_extension(ref_grid, "mesh-fail.tec");
          });
          if (REF_CAVITY_VISIBLE != ref_cavity_state(ref_cavity)) {
            RSB(ref_cavity_form_insert2_unconstrain(ref_cavity,
                                                    constraining_faceid),
                "unconst", {
                  printf(" unconstrain state %d\n",
                         (int)ref_cavity_state(ref_cavity));
                  ref_cavity_tec(ref_cavity, "cav-unconst.tec");
                });
            RSB(ref_cavity_enlarge_combined(ref_cavity), "enlarge", {
              ref_cavity_tec(ref_cavity, "cav-fail.tec");
              ref_export_by_extension(ref_grid, "mesh-fail.tec");
            });
          }
          if (node == REF_EMPTY && /* turns off continue */
              REF_CAVITY_VISIBLE != ref_cavity_state(ref_cavity)) {
            RSS(ref_cavity_free(ref_cavity), "cav free");
            off_node[node] = REF_EMPTY;
            RSS(ref_geom_remove_all(ref_geom, new_node), "rm geom");
            RSS(ref_node_remove(ref_node, new_node), "rm node");
            continue;
          }
          RSB(ref_cavity_replace(ref_cavity), "cav replace", {
            ref_export_by_extension(ref_grid, "ref_layer_prism_replace.tec");
            ref_cavity_tec(ref_cavity, "ref_layer_prism_cavity.tec");
            ref_export_by_extension(ref_grid, "ref_layer_prism_mesh.tec");
          });
          if (ref_cavity_debug(ref_cavity)) printf("replace complete\n");
          RSS(ref_cavity_free(ref_cavity), "cav free");
        }
      } else {
        RSS(ref_cavity_create(&ref_cavity), "cav create");
        RSS(ref_cavity_form_insert_tet(ref_cavity, ref_grid, new_node, node,
                                       REF_EMPTY),
            "ball");
        RSB(ref_cavity_enlarge_combined(ref_cavity), "enlarge", {
          ref_cavity_tec(ref_cavity, "cav-fail.tec");
          ref_export_by_extension(ref_grid, "mesh-fail.tec");
        });
        RSB(ref_cavity_replace(ref_cavity), "cav replace", {
          ref_cavity_tec(ref_cavity, "ref_layer_prism_cavity.tec");
          ref_export_by_extension(ref_grid, "ref_layer_prism_mesh.tec");
        });
        RSS(ref_cavity_free(ref_cavity), "cav free");
      }
    }
  }
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_layer_align_prism(REF_GRID ref_grid,
                                         REF_DICT ref_dict_bcs) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT item, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL *active;
  REF_INT *off_node;

  RSS(ref_node_synchronize_globals(ref_node), "sync glob");

  RSS(ref_export_by_extension(ref_grid, "ref_layer_prism_before.tec"),
      "dump surf");

  ref_malloc_init(active, ref_node_max(ref_node), REF_BOOL, REF_FALSE);
  ref_malloc_init(off_node, ref_node_max(ref_node), REF_BOOL, REF_EMPTY);
  /* mark active nodes */
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    REF_INT bc = REF_EMPTY;
    RXS(ref_dict_value(ref_dict_bcs, nodes[ref_cell_id_index(ref_cell)], &bc),
        REF_NOT_FOUND, "bc");
    if (ref_phys_wall_distance_bc(bc)) {
      REF_INT cell_node;
      each_ref_cell_cell_node(ref_cell, cell_node) {
        active[nodes[cell_node]] = REF_TRUE;
      }
    }
  }

  RSS(ref_layer_prism_insert_hair(ref_grid, ref_dict_bcs, active, off_node),
      "insert hair");

  /* recover tet slides of prism tops, extend for boundary */
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (REF_EMPTY != off_node[nodes[0]] && REF_EMPTY != off_node[nodes[1]] &&
        REF_EMPTY != off_node[nodes[2]]) {
      REF_INT cell_edge;
      each_ref_cell_cell_edge(ref_cell, cell_edge) {
        REF_INT node0 = off_node[ref_cell_e2n(ref_cell, 0, cell_edge, cell)];
        REF_INT node1 = off_node[ref_cell_e2n(ref_cell, 1, cell_edge, cell)];
        REF_BOOL has_side;
        RSS(ref_cell_has_side(ref_grid_tet(ref_grid), node0, node1, &has_side),
            "find tet side");
        if (!has_side) {
          REF_CAVITY ref_cavity;
          REF_INT tet;
          RSS(ref_layer_seed_tet(ref_grid, node0, node1, &tet), "seed");
          RSS(ref_cavity_create(&ref_cavity), "cav create");
          RSS(ref_cavity_form_empty(ref_cavity, ref_grid, node1), "empty");
          RSS(ref_cavity_add_tet(ref_cavity, tet), "add tet");
          REIS(REF_CAVITY_UNKNOWN, ref_cavity_state(ref_cavity),
               "add tet not unknown");
          RSB(ref_cavity_enlarge_visible(ref_cavity), "enlarge", {
            ref_cavity_tec(ref_cavity, "cav-fail.tec");
            ref_export_by_extension(ref_grid, "mesh-fail.tec");
          });
          REIB(REF_CAVITY_VISIBLE, ref_cavity_state(ref_cavity),
               "prism top not visible", {
                 ref_cavity_tec(ref_cavity, "ref_layer_prism_cavity.tec");
                 ref_export_by_extension(ref_grid, "ref_layer_prism_mesh.tec");
               });
          RSB(ref_cavity_replace(ref_cavity), "cav replace", {
            ref_cavity_tec(ref_cavity, "ref_layer_prism_cavity.tec");
            ref_export_by_extension(ref_grid, "ref_layer_prism_mesh.tec");
          });
          RSS(ref_cavity_free(ref_cavity), "cav free");
        }
      }
    }
  }
  /* recover prism sides */
  {
    REF_INT cell_edge;
    REF_INT quad[4], face_nodes[4], tet0, tet1;
    REF_BOOL has_side;
    each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
      if (REF_EMPTY != off_node[nodes[0]] && REF_EMPTY != off_node[nodes[1]] &&
          REF_EMPTY != off_node[nodes[2]]) {
        each_ref_cell_cell_edge(ref_cell, cell_edge) {
          quad[0] = REF_EMPTY;
          quad[1] = REF_EMPTY;
          quad[2] = REF_EMPTY;
          quad[3] = REF_EMPTY;
          RSS(ref_cell_has_side(
                  ref_grid_tet(ref_grid),
                  ref_cell_e2n(ref_cell, 0, cell_edge, cell),
                  off_node[ref_cell_e2n(ref_cell, 1, cell_edge, cell)],
                  &has_side),
              "diag");
          if (has_side) {
            quad[0] = ref_cell_e2n(ref_cell, 0, cell_edge, cell);
            quad[1] = ref_cell_e2n(ref_cell, 1, cell_edge, cell);
            quad[2] = off_node[ref_cell_e2n(ref_cell, 1, cell_edge, cell)];
            quad[3] = off_node[ref_cell_e2n(ref_cell, 0, cell_edge, cell)];
          }
          RSS(ref_cell_has_side(
                  ref_grid_tet(ref_grid),
                  ref_cell_e2n(ref_cell, 1, cell_edge, cell),
                  off_node[ref_cell_e2n(ref_cell, 0, cell_edge, cell)],
                  &has_side),
              "diag");
          if (has_side) {
            quad[0] = ref_cell_e2n(ref_cell, 1, cell_edge, cell);
            quad[1] = ref_cell_e2n(ref_cell, 0, cell_edge, cell);
            quad[2] = off_node[ref_cell_e2n(ref_cell, 0, cell_edge, cell)];
            quad[3] = off_node[ref_cell_e2n(ref_cell, 1, cell_edge, cell)];
          }
          RUS(REF_EMPTY, quad[0], "diag not found");
          face_nodes[0] = quad[0];
          face_nodes[1] = quad[1];
          face_nodes[2] = quad[2];
          face_nodes[3] = face_nodes[0];
          RSS(ref_cell_with_face(ref_grid_tet(ref_grid), face_nodes, &tet0,
                                 &tet1),
              "tets");
          if (tet0 == REF_EMPTY && tet1 == REF_EMPTY) {
            RSS(ref_layer_recover_face(ref_grid, face_nodes), "recover upper");
            printf("lower tets %d %d\n", tet0, tet1);
          }
          face_nodes[0] = quad[0];
          face_nodes[1] = quad[2];
          face_nodes[2] = quad[3];
          face_nodes[3] = face_nodes[0];
          RSS(ref_cell_with_face(ref_grid_tet(ref_grid), face_nodes, &tet0,
                                 &tet1),
              "tets");
          if (tet0 == REF_EMPTY && tet1 == REF_EMPTY) {
            RSS(ref_layer_recover_face(ref_grid, face_nodes), "recover upper");
            printf("upper tets %d %d\n", tet0, tet1);
          }
        }
      }
    }
  }
  /* remove quad slivers */
  {
    REF_INT cell_edge;
    REF_INT quad[4];
    REF_BOOL has_diag02, has_diag13;
    each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
      if (REF_EMPTY != off_node[nodes[0]] && REF_EMPTY != off_node[nodes[1]] &&
          REF_EMPTY != off_node[nodes[2]]) {
        each_ref_cell_cell_edge(ref_cell, cell_edge) {
          quad[0] = ref_cell_e2n(ref_cell, 0, cell_edge, cell);
          quad[1] = ref_cell_e2n(ref_cell, 1, cell_edge, cell);
          quad[2] = off_node[ref_cell_e2n(ref_cell, 1, cell_edge, cell)];
          quad[3] = off_node[ref_cell_e2n(ref_cell, 0, cell_edge, cell)];
          RSS(ref_cell_has_side(ref_grid_tet(ref_grid), quad[0], quad[2],
                                &has_diag02),
              "diag02");
          RSS(ref_cell_has_side(ref_grid_tet(ref_grid), quad[1], quad[3],
                                &has_diag13),
              "diag13");
          if (has_diag02 && has_diag13) {
            printf("sliver\n");
            RSS(ref_layer_remove_sliver(ref_grid, quad), "remove sliver");
          }
        }
      }
    }
  }
  /* replace tets with prism */
  {
    REF_ADJ tri_tet;
    REF_INT deg;
    RSS(ref_adj_create(&tri_tet), "tet list");
    each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
      if (REF_EMPTY != off_node[nodes[0]] && REF_EMPTY != off_node[nodes[1]] &&
          REF_EMPTY != off_node[nodes[2]]) {
        REF_CELL ref_tet = ref_grid_tet(ref_grid);
        REF_INT cell_node, tet, tet_nodes[REF_CELL_MAX_SIZE_PER],
            pri_nodes[REF_CELL_MAX_SIZE_PER];
        REF_BOOL contains;
        pri_nodes[0] = nodes[0];
        pri_nodes[1] = nodes[1];
        pri_nodes[2] = nodes[2];
        pri_nodes[3] = off_node[nodes[0]];
        pri_nodes[4] = off_node[nodes[1]];
        pri_nodes[5] = off_node[nodes[2]];
        each_ref_cell_having_node2(ref_tet, nodes[0], off_node[nodes[0]], item,
                                   cell_node, tet) {
          RSS(ref_cell_nodes(ref_tet, tet, tet_nodes), "tet");
          RSS(ref_layer_tet_prism(pri_nodes, tet_nodes, &contains), "contains");
          if (contains) {
            RSS(ref_adj_add_uniquely(tri_tet, cell, tet), "add");
          }
        }
        each_ref_cell_having_node2(ref_tet, nodes[1], off_node[nodes[1]], item,
                                   cell_node, tet) {
          RSS(ref_cell_nodes(ref_tet, tet, tet_nodes), "tet");
          RSS(ref_layer_tet_prism(pri_nodes, tet_nodes, &contains), "contains");
          if (contains) {
            RSS(ref_adj_add_uniquely(tri_tet, cell, tet), "add");
          }
        }
        each_ref_cell_having_node2(ref_tet, nodes[2], off_node[nodes[2]], item,
                                   cell_node, tet) {
          RSS(ref_cell_nodes(ref_tet, tet, tet_nodes), "tet");
          RSS(ref_layer_tet_prism(pri_nodes, tet_nodes, &contains), "contains");
          if (contains) {
            RSS(ref_adj_add_uniquely(tri_tet, cell, tet), "add");
          }
        }

        RSS(ref_adj_degree(tri_tet, cell, &deg), "deg");
        if (3 != deg) {
          REF_CAVITY ref_cavity;
          char filename[1024];
          RSS(ref_cavity_create(&ref_cavity), "cav create");
          RSS(ref_cavity_form_empty(ref_cavity, ref_grid, REF_EMPTY), "empty");
          each_ref_adj_node_item_with_ref(tri_tet, cell, item, tet) {
            RSS(ref_cavity_add_tet(ref_cavity, tet), "add tet");
          }
          snprintf(filename, 1024, "prism-%d-cav.tec", cell);
          printf("prism tets %d %s\n", deg, filename);
          RSS(ref_cavity_tec(ref_cavity, filename), "cav tec");
          RSS(ref_cavity_free(ref_cavity), "cav free");
        }
        if (3 == deg) {
          REF_INT new_pri;
          each_ref_adj_node_item_with_ref(tri_tet, cell, item, tet) {
            RSS(ref_cell_remove(ref_grid_tet(ref_grid), tet), "rm tet");
          }
          RSS(ref_cell_add(ref_grid_pri(ref_grid), pri_nodes, &new_pri),
              "add pri");
        }
      }
    }
    RSS(ref_adj_free(tri_tet), "free");
  }
  /* replace two tets with pyramid */
  each_ref_cell_valid_cell_with_nodes(ref_grid_pri(ref_grid), cell, nodes) {
    REF_CAVITY ref_cavity;
    char filename[1024];
    REF_INT cell_face, tet, cell_node;
    each_ref_cell_cell_face(ref_grid_pri(ref_grid), cell_face) {
      RSS(ref_cavity_create(&ref_cavity), "cav create");
      RSS(ref_cavity_form_empty(ref_cavity, ref_grid, REF_EMPTY), "empty");
      if (ref_cell_f2n(ref_grid_pri(ref_grid), 0, cell_face, cell) !=
          ref_cell_f2n(ref_grid_pri(ref_grid), 3, cell_face, cell)) {
        each_ref_cell_having_node2(
            ref_grid_tet(ref_grid),
            ref_cell_f2n(ref_grid_pri(ref_grid), 0, cell_face, cell),
            ref_cell_f2n(ref_grid_pri(ref_grid), 2, cell_face, cell), item,
            cell_node, tet) {
          RSS(ref_cavity_add_tet(ref_cavity, tet), "add tet");
        }
        each_ref_cell_having_node2(
            ref_grid_tet(ref_grid),
            ref_cell_f2n(ref_grid_pri(ref_grid), 1, cell_face, cell),
            ref_cell_f2n(ref_grid_pri(ref_grid), 3, cell_face, cell), item,
            cell_node, tet) {
          RSS(ref_cavity_add_tet(ref_cavity, tet), "add tet");
        }
      }
      if (2 != ref_list_n(ref_cavity_tet_list(ref_cavity)) &&
          0 != ref_list_n(ref_cavity_tet_list(ref_cavity))) {
        snprintf(filename, 1024, "glue-%d-%d-cav.tec", cell, cell_face);
        printf("pyramid tets missing %d %s\n",
               ref_list_n(ref_cavity_tet_list(ref_cavity)), filename);
        if (0 < ref_cavity_nface(ref_cavity))
          RSS(ref_cavity_tec(ref_cavity, filename), "cav tec");
      }
      if (2 == ref_list_n(ref_cavity_tet_list(ref_cavity))) {
        /* replace two tets with pyramid */
        RSS(ref_layer_tet_to_pyr(
                ref_grid, cell,
                ref_list_value(ref_cavity_tet_list(ref_cavity), 0),
                ref_list_value(ref_cavity_tet_list(ref_cavity), 1)),
            "tet2pyr");
      }
      RSS(ref_cavity_free(ref_cavity), "cav free");
    }
  }

  RSS(ref_export_by_extension(ref_grid, "ref_layer_prism_after.tec"),
      "dump surf after");
  ref_free(off_node);
  ref_free(active);

  return REF_SUCCESS;
}
