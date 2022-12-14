
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

#include "ref_iso.h"

#include <stdio.h>
#include <stdlib.h>

#include "ref_edge.h"
#include "ref_face.h"
#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_node.h"
#include "ref_sort.h"

REF_FCN static REF_STATUS ref_iso_ghost(REF_GRID iso_grid, REF_EDGE ref_edge,
                                        REF_INT *new_node) {
  REF_MPI ref_mpi = ref_grid_mpi(iso_grid);
  REF_GLOB *edge_global;
  REF_INT *edge_part;
  REF_DBL *edge_real;
  REF_INT edge, node, part, i;
  REF_GLOB global;

  ref_malloc_init(edge_global, ref_edge_n(ref_edge), REF_GLOB, REF_EMPTY);
  ref_malloc_init(edge_part, ref_edge_n(ref_edge), REF_INT, REF_EMPTY);
  ref_malloc_init(edge_real, REF_NODE_REAL_PER * ref_edge_n(ref_edge), REF_DBL,
                  -999.0);

  for (edge = 0; edge < ref_edge_n(ref_edge); edge++) {
    node = new_node[edge];
    if (REF_EMPTY != node) {
      RSS(ref_edge_part(ref_edge, edge, &part), "edge part");
      if (ref_mpi_rank(ref_mpi) == part) {
        edge_global[edge] = ref_node_global(ref_grid_node(iso_grid), node);
        edge_part[edge] = ref_node_part(ref_grid_node(iso_grid), node);
        for (i = 0; i < REF_NODE_REAL_PER; i++)
          edge_real[i + REF_NODE_REAL_PER * edge] =
              ref_node_real(ref_grid_node(iso_grid), i, node);
      }
    }
  }

  RSS(ref_edge_ghost_glob(ref_edge, ref_mpi, edge_global), "global ghost");
  RSS(ref_edge_ghost_int(ref_edge, ref_mpi, edge_part), "part ghost");
  RSS(ref_edge_ghost_dbl(ref_edge, ref_mpi, edge_real, REF_NODE_REAL_PER),
      "xyz ghost");

  for (edge = 0; edge < ref_edge_n(ref_edge); edge++) {
    global = edge_global[edge];
    if (REF_EMPTY != global) {
      /* finds local if already inserted */
      RSS(ref_node_add(ref_grid_node(iso_grid), global, &node), "add node");
      new_node[edge] = node;
      ref_node_part(ref_grid_node(iso_grid), node) = edge_part[edge];
      for (i = 0; i < REF_NODE_REAL_PER; i++)
        ref_node_real(ref_grid_node(iso_grid), i, node) =
            edge_real[i + REF_NODE_REAL_PER * edge];
    }
  }

  ref_free(edge_real);
  ref_free(edge_part);
  ref_free(edge_global);

  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_iso_interp(REF_GRID iso_grid, REF_EDGE ref_edge,
                                         REF_INT *new_node, REF_DBL *field,
                                         REF_INT ldim, REF_DBL *in,
                                         REF_DBL **out) {
  REF_MPI ref_mpi = ref_grid_mpi(iso_grid);
  REF_INT i, edge, part;
  REF_INT node0, node1;
  REF_DBL t1, t0, d;
  REF_DBL *edge_real;

  if (ldim <= 0) return REF_SUCCESS;

  ref_malloc(*out, ldim * ref_node_max(ref_grid_node(iso_grid)), REF_DBL);

  ref_malloc_init(edge_real, ldim * ref_edge_n(ref_edge), REF_DBL, -999.0);

  each_ref_edge(ref_edge, edge) {
    if (REF_EMPTY != new_node[edge]) {
      node0 = ref_edge_e2n(ref_edge, 0, edge);
      node1 = ref_edge_e2n(ref_edge, 1, edge);
      RSS(ref_edge_part(ref_edge, edge, &part), "edge part");
      if (ref_mpi_rank(ref_mpi) == part) {
        d = field[node1] - field[node0];
        t1 = 0.5;
        if (ref_math_divisible(field[node0], d)) {
          t1 = -field[node0] / d;
        }
        t0 = 1.0 - t1;
        for (i = 0; i < ldim; i++) {
          (*out)[i + ldim * new_node[edge]] =
              t1 * in[i + ldim * node1] + t0 * in[i + ldim * node0];
          edge_real[i + ldim * edge] = (*out)[i + ldim * new_node[edge]];
        }
      }
    }
  }

  RSS(ref_edge_ghost_dbl(ref_edge, ref_mpi, edge_real, ldim), "edge ghost");

  for (edge = 0; edge < ref_edge_n(ref_edge); edge++) {
    if (REF_EMPTY != new_node[edge]) {
      node0 = ref_edge_e2n(ref_edge, 0, edge);
      node1 = ref_edge_e2n(ref_edge, 1, edge);
      RSS(ref_edge_part(ref_edge, edge, &part), "edge part");
      if (ref_mpi_rank(ref_mpi) != part) {
        for (i = 0; i < ldim; i++) {
          (*out)[i + ldim * new_node[edge]] = edge_real[i + ldim * edge];
        }
      }
    }
  }

  ref_free(edge_real);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_iso_insert(REF_GRID *iso_grid_ptr, REF_GRID ref_grid,
                                  REF_DBL *field, REF_INT ldim, REF_DBL *in,
                                  REF_DBL **out) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_GRID iso_grid;
  REF_EDGE ref_edge;
  REF_INT *new_node;
  REF_INT edge, part, new_cell;
  REF_INT node0, node1, node2;
  REF_INT edge0, edge1, edge2;
  REF_GLOB global;
  REF_DBL t1, t0, d;
  REF_CELL ref_cell;
  REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT new_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT id = 1;

  RSS(ref_node_synchronize_globals(ref_grid_node(ref_grid)), "sync glob");

  RSS(ref_grid_create(iso_grid_ptr, ref_mpi), "create");
  iso_grid = *iso_grid_ptr;
  RSS(ref_node_initialize_n_global(ref_grid_node(iso_grid), 0), "zero glob");

  RSS(ref_edge_create(&ref_edge, ref_grid), "create edge");
  ref_malloc_init(new_node, ref_edge_n(ref_edge), REF_INT, REF_EMPTY);

  each_ref_edge(ref_edge, edge) {
    node0 = ref_edge_e2n(ref_edge, 0, edge);
    node1 = ref_edge_e2n(ref_edge, 1, edge);
    RSS(ref_edge_part(ref_edge, edge, &part), "edge part");
    if (ref_mpi_rank(ref_mpi) == part) {
      if (MAX(field[node0], field[node1]) > 0.0 &&
          MIN(field[node0], field[node1]) < 0.0) {
        RSS(ref_node_next_global(ref_grid_node(iso_grid), &global),
            "next global");
        RSS(ref_node_add(ref_grid_node(iso_grid), global, &(new_node[edge])),
            "add node");
        d = field[node1] - field[node0];
        t1 = 0.5;
        if (ref_math_divisible(field[node0], d)) {
          t1 = -field[node0] / d;
        }
        t0 = 1.0 - t1;
        /*printf("%f %f d %f t %f\n",field[node0],field[node1],d,t);*/
        ref_node_xyz(ref_grid_node(iso_grid), 0, new_node[edge]) =
            t1 * ref_node_xyz(ref_grid_node(ref_grid), 0, node1) +
            t0 * ref_node_xyz(ref_grid_node(ref_grid), 0, node0);
        ref_node_xyz(ref_grid_node(iso_grid), 1, new_node[edge]) =
            t1 * ref_node_xyz(ref_grid_node(ref_grid), 1, node1) +
            t0 * ref_node_xyz(ref_grid_node(ref_grid), 1, node0);
        ref_node_xyz(ref_grid_node(iso_grid), 2, new_node[edge]) =
            t1 * ref_node_xyz(ref_grid_node(ref_grid), 2, node1) +
            t0 * ref_node_xyz(ref_grid_node(ref_grid), 2, node0);
      }
    }
  }
  RSS(ref_node_shift_new_globals(ref_grid_node(iso_grid)), "shift iso glob");

  RSS(ref_iso_ghost(iso_grid, ref_edge, new_node), "new ghost");

  RSS(ref_iso_interp(iso_grid, ref_edge, new_node, field, ldim, in, out),
      "interp in to out");

  if (ref_grid_twod(ref_grid)) {
    ref_cell = ref_grid_tri(ref_grid);
    each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
      RSS(ref_edge_with(ref_edge, nodes[1], nodes[2], &edge0), "e0");
      RSS(ref_edge_with(ref_edge, nodes[2], nodes[0], &edge1), "e1");
      RSS(ref_edge_with(ref_edge, nodes[0], nodes[1], &edge2), "e2");
      node0 = new_node[edge0];
      node1 = new_node[edge1];
      node2 = new_node[edge2];
      if (REF_EMPTY == node0 && REF_EMPTY != node1 && REF_EMPTY != node2) {
        new_nodes[0] = node1;
        new_nodes[1] = node2;
        new_nodes[2] = id;
        RSS(ref_cell_add(ref_grid_edg(iso_grid), new_nodes, &new_cell), "add");
      }
      if (REF_EMPTY != node0 && REF_EMPTY == node1 && REF_EMPTY != node2) {
        new_nodes[0] = node2;
        new_nodes[1] = node0;
        new_nodes[2] = id;
        RSS(ref_cell_add(ref_grid_edg(iso_grid), new_nodes, &new_cell), "add");
      }
      if (REF_EMPTY != node0 && REF_EMPTY != node1 && REF_EMPTY == node2) {
        new_nodes[0] = node0;
        new_nodes[1] = node1;
        new_nodes[2] = id;
        RSS(ref_cell_add(ref_grid_edg(iso_grid), new_nodes, &new_cell), "add");
      }
    }
  } else {
    REF_INT cell_edge;
    REF_INT edge_nodes[6];
    REF_INT nedge;
    REF_INT tri_nodes[REF_CELL_MAX_SIZE_PER];
    REF_BOOL found;
    ref_cell = ref_grid_tet(ref_grid);
    each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
      nedge = 0;
      each_ref_cell_cell_edge(ref_cell, cell_edge) {
        RSS(ref_edge_with(ref_edge, ref_cell_e2n(ref_cell, 0, cell_edge, cell),
                          ref_cell_e2n(ref_cell, 1, cell_edge, cell), &edge),
            "cell edge");
        edge_nodes[cell_edge] = new_node[edge];
        if (REF_EMPTY != edge_nodes[cell_edge]) {
          new_nodes[nedge] = edge_nodes[cell_edge];
          nedge++;
        }
      }
      switch (nedge) {
        case 0:
          /* cell is not intersected by iso surface */
          break;
        case 3:
          new_nodes[3] = id;
          RSS(ref_cell_add(ref_grid_tri(iso_grid), new_nodes, &new_cell),
              "add");
          break;
        case 4:
          found = REF_FALSE;
          if (REF_EMPTY == edge_nodes[0] && REF_EMPTY == edge_nodes[5]) {
            RAS(!found, "found twice");
            found = REF_TRUE;
            tri_nodes[0] = edge_nodes[1];
            tri_nodes[1] = edge_nodes[2];
            tri_nodes[2] = edge_nodes[4];
            tri_nodes[3] = id;
            RSS(ref_cell_add(ref_grid_tri(iso_grid), tri_nodes, &new_cell),
                "add");
            tri_nodes[0] = edge_nodes[1];
            tri_nodes[1] = edge_nodes[4];
            tri_nodes[2] = edge_nodes[3];
            tri_nodes[3] = id;
            RSS(ref_cell_add(ref_grid_tri(iso_grid), tri_nodes, &new_cell),
                "add");
          }
          if (REF_EMPTY == edge_nodes[1] && REF_EMPTY == edge_nodes[4]) {
            RAS(!found, "found twice");
            found = REF_TRUE;
            tri_nodes[0] = edge_nodes[0];
            tri_nodes[1] = edge_nodes[3];
            tri_nodes[2] = edge_nodes[2];
            tri_nodes[3] = id;
            RSS(ref_cell_add(ref_grid_tri(iso_grid), tri_nodes, &new_cell),
                "add");
            tri_nodes[0] = edge_nodes[2];
            tri_nodes[1] = edge_nodes[3];
            tri_nodes[2] = edge_nodes[5];
            tri_nodes[3] = id;
            RSS(ref_cell_add(ref_grid_tri(iso_grid), tri_nodes, &new_cell),
                "add");
          }
          if (REF_EMPTY == edge_nodes[2] && REF_EMPTY == edge_nodes[3]) {
            RAS(!found, "found twice");
            found = REF_TRUE;
            tri_nodes[0] = edge_nodes[0];
            tri_nodes[1] = edge_nodes[4];
            tri_nodes[2] = edge_nodes[1];
            tri_nodes[3] = id;
            RSS(ref_cell_add(ref_grid_tri(iso_grid), tri_nodes, &new_cell),
                "add");
            tri_nodes[0] = edge_nodes[1];
            tri_nodes[1] = edge_nodes[4];
            tri_nodes[2] = edge_nodes[5];
            tri_nodes[3] = id;
            RSS(ref_cell_add(ref_grid_tri(iso_grid), tri_nodes, &new_cell),
                "add");
          }
          RAS(found, "not found");
          break;

        default:
          printf("nedge %d\n", nedge);
          RSS(REF_IMPLEMENT, "implement iso surface tet intersection");
          break;
      }
    }
  }

  ref_free(new_node);
  ref_edge_free(ref_edge);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_iso_signed_distance(REF_GRID ref_grid, REF_DBL *field,
                                           REF_DBL *distance) {
  REF_GRID iso_grid;
  REF_SEARCH ref_search = NULL;
  REF_CELL ref_cell;
  REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER], cand[REF_CELL_MAX_SIZE_PER];
  REF_DBL center[3], radius, dist;
  REF_DBL scale = 1.0 + 1.0e-8;
  REF_LIST ref_list;
  REF_INT node, item, candidate;

  RSS(ref_iso_insert(&iso_grid, ref_grid, field, 0, NULL, NULL), "iso");

  if (ref_grid_twod(ref_grid)) {
    ref_cell = ref_grid_edg(iso_grid);
  } else {
    ref_cell = ref_grid_tri(iso_grid);
  }
  RSS(ref_search_create(&ref_search, ref_cell_n(ref_cell)), "create search");
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (ref_grid_twod(ref_grid)) {
      RSS(ref_node_bounding_sphere(ref_grid_node(iso_grid), nodes, 2, center,
                                   &radius),
          "b");
    } else {
      RSS(ref_node_bounding_sphere(ref_grid_node(iso_grid), nodes, 3, center,
                                   &radius),
          "b");
    }
    RSS(ref_search_insert(ref_search, cell, center, scale * radius), "ins");
  }
  RSS(ref_list_create(&ref_list), "create list");
  each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
    distance[node] = REF_DBL_MAX;
    RSS(ref_search_nearest_candidates(
            ref_search, ref_list,
            ref_node_xyz_ptr(ref_grid_node(ref_grid), node)),
        "candidates");
    each_ref_list_item(ref_list, item) {
      candidate = ref_list_value(ref_list, item);
      RSS(ref_cell_nodes(ref_cell, candidate, cand), "cell");
      if (ref_grid_twod(ref_grid)) {
        RSS(ref_search_distance2(
                ref_node_xyz_ptr(ref_grid_node(iso_grid), cand[0]),
                ref_node_xyz_ptr(ref_grid_node(iso_grid), cand[1]),
                ref_node_xyz_ptr(ref_grid_node(ref_grid), node), &dist),
            "dist2");
      } else {
        RSS(ref_search_distance3(
                ref_node_xyz_ptr(ref_grid_node(iso_grid), cand[0]),
                ref_node_xyz_ptr(ref_grid_node(iso_grid), cand[1]),
                ref_node_xyz_ptr(ref_grid_node(iso_grid), cand[2]),
                ref_node_xyz_ptr(ref_grid_node(ref_grid), node), &dist),
            "dist3");
      }
      distance[node] = MIN(distance[node], dist);
    }

    /* sign distanace */
    if (0.0 > field[node]) distance[node] = -distance[node];

    RSS(ref_list_erase(ref_list), "reset list");
  }
  RSS(ref_list_free(ref_list), "free list");
  RSS(ref_search_free(ref_search), "free search");

  RSS(ref_grid_free(iso_grid), "iso free");

  return REF_SUCCESS;
}

static REF_DBL ref_iso_volume(double *a, double *b, double *c, double *d) {
  double m11, m12, m13;
  double det;

  m11 = (a[0] - d[0]) *
        ((b[1] - d[1]) * (c[2] - d[2]) - (c[1] - d[1]) * (b[2] - d[2]));
  m12 = (a[1] - d[1]) *
        ((b[0] - d[0]) * (c[2] - d[2]) - (c[0] - d[0]) * (b[2] - d[2]));
  m13 = (a[2] - d[2]) *
        ((b[0] - d[0]) * (c[1] - d[1]) - (c[0] - d[0]) * (b[1] - d[1]));
  det = (m11 - m12 + m13);

  return (-det);
}

REF_FCN REF_STATUS ref_iso_triangle_segment(REF_DBL *triangle0,
                                            REF_DBL *triangle1,
                                            REF_DBL *triangle2,
                                            REF_DBL *segment0,
                                            REF_DBL *segment1, REF_DBL *tuvw) {
  double top_volume, bot_volume;
  double side0_volume, side1_volume, side2_volume;
  double total_volume;

  tuvw[0] = 0.0;
  tuvw[1] = 0.0;
  tuvw[2] = 0.0;
  tuvw[3] = 0.0;

  /* is segment in triangle plane? */
  top_volume = ref_iso_volume(triangle0, triangle1, triangle2, segment0);
  bot_volume = ref_iso_volume(triangle0, triangle1, triangle2, segment1);

  /* does segment pass through triangle? */
  side2_volume = ref_iso_volume(triangle0, triangle1, segment0, segment1);
  side0_volume = ref_iso_volume(triangle1, triangle2, segment0, segment1);
  side1_volume = ref_iso_volume(triangle2, triangle0, segment0, segment1);

  total_volume = top_volume - bot_volume;
  if (ref_math_divisible(top_volume, total_volume)) {
    tuvw[0] = top_volume / total_volume;
  } else {
    return REF_DIV_ZERO;
  }

  total_volume = side0_volume + side1_volume + side2_volume;
  if (ref_math_divisible(side0_volume, total_volume) &&
      ref_math_divisible(side1_volume, total_volume) &&
      ref_math_divisible(side2_volume, total_volume)) {
    tuvw[1] = side0_volume / total_volume;
    tuvw[2] = side1_volume / total_volume;
    tuvw[3] = side2_volume / total_volume;
  } else {
    return REF_DIV_ZERO;
  }

  return REF_SUCCESS;
}

static REF_DBL ref_iso_area(double *xyz0, double *xyz1, double *xyz2) {
  double a, b, c, d;
  double area;

  a = xyz0[0] - xyz2[0];
  b = xyz0[1] - xyz2[1];
  c = xyz1[0] - xyz2[0];
  d = xyz1[1] - xyz2[1];
  area = 0.5 * (a * d - b * c);

  return area;
}

REF_FCN REF_STATUS ref_iso_segment_segment(REF_DBL *candidate0,
                                           REF_DBL *candidate1,
                                           REF_DBL *segment0, REF_DBL *segment1,
                                           REF_DBL *tt) {
  double cand0_area, cand1_area;
  double seg0_area, seg1_area;
  double total_area;

  tt[0] = 0.0;
  tt[1] = 0.0;

  /*         c1 tt[1]
   *        /
   *  s0---+----s1 tt[0]
   *      /
   *    c0
   */

  cand0_area = ref_iso_area(candidate0, segment1, segment0);
  cand1_area = ref_iso_area(candidate1, segment0, segment1);
  seg0_area = ref_iso_area(segment0, candidate1, candidate0);
  seg1_area = ref_iso_area(segment1, candidate0, candidate1);

  total_area = seg0_area + seg1_area;
  if (ref_math_divisible(seg0_area, total_area)) {
    tt[0] = seg0_area / total_area;
  } else {
    return REF_DIV_ZERO;
  }

  total_area = cand0_area + cand1_area;
  if (ref_math_divisible(cand0_area, total_area)) {
    tt[1] = cand0_area / total_area;
  } else {
    return REF_DIV_ZERO;
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_iso_cast(REF_GRID *iso_grid_ptr, REF_DBL **iso_field_ptr,
                                REF_GRID ref_grid, REF_DBL *field, REF_INT ldim,
                                REF_DBL *segment0, REF_DBL *segment1) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = NULL;
  REF_GRID iso_grid;
  REF_EDGE ref_edge = NULL;
  REF_FACE ref_face = NULL;
  REF_INT edge, face, i, j, part, cell;
  REF_GLOB global;
  REF_INT *new_node = NULL;
  REF_DBL candidate0[3], candidate1[3];
  REF_DBL triangle0[3], triangle1[3], triangle2[3];
  REF_DBL tt[2], ss[2], tuvw[4];
  REF_DBL inside = 0.0;
  REF_DBL t0, t1;
  REF_INT id = 1;
  REF_DBL *iso_field;

  RSS(ref_node_synchronize_globals(ref_node), "sync glob");

  RSS(ref_grid_create(iso_grid_ptr, ref_mpi), "create");
  iso_grid = *iso_grid_ptr;
  ref_grid_twod(iso_grid) = ref_grid_twod(ref_grid);
  RSS(ref_node_initialize_n_global(ref_grid_node(iso_grid), 0), "zero glob");

  if (ref_grid_twod(ref_grid)) {
    RSS(ref_edge_create(&ref_edge, ref_grid), "create edge");
    ref_malloc_init(new_node, ref_edge_n(ref_edge), REF_INT, REF_EMPTY);
    each_ref_edge(ref_edge, edge) {
      for (i = 0; i < 3; i++) {
        candidate0[i] =
            ref_node_xyz(ref_node, i, ref_edge_e2n(ref_edge, 0, edge));
        candidate1[i] =
            ref_node_xyz(ref_node, i, ref_edge_e2n(ref_edge, 1, edge));
      }
      if (REF_SUCCESS != ref_iso_segment_segment(candidate0, candidate1,
                                                 segment0, segment1, tt))
        continue;
      ss[0] = 1.0 - tt[0];
      ss[1] = 1.0 - tt[1];
      if (MIN(tt[0], ss[0]) < inside) continue;
      if (MIN(tt[1], ss[1]) < inside) continue;

      RSS(ref_edge_part(ref_edge, edge, &part), "edge part");
      if (ref_mpi_rank(ref_mpi) == part) {
        RSS(ref_node_next_global(ref_grid_node(iso_grid), &global),
            "next global");
        RSS(ref_node_add(ref_grid_node(iso_grid), global, &(new_node[edge])),
            "add node");
        for (i = 0; i < 3; i++) {
          ref_node_xyz(ref_grid_node(iso_grid), i, new_node[edge]) =
              ss[1] * candidate1[i] + tt[1] * candidate0[i];
        }
      }
    }
  } else {
    RSS(ref_face_create(&ref_face, ref_grid), "create face");
    ref_malloc_init(new_node, ref_face_n(ref_face), REF_INT, REF_EMPTY);

    each_ref_face(ref_face, face) {
      for (i = 0; i < 3; i++) {
        triangle0[i] =
            ref_node_xyz(ref_node, i, ref_face_f2n(ref_face, 0, face));
        triangle1[i] =
            ref_node_xyz(ref_node, i, ref_face_f2n(ref_face, 1, face));
        triangle2[i] =
            ref_node_xyz(ref_node, i, ref_face_f2n(ref_face, 2, face));
      }
      if (REF_SUCCESS != ref_iso_triangle_segment(triangle0, triangle1,
                                                  triangle2, segment0, segment1,
                                                  tuvw))
        continue;
      if (MIN(MIN(tuvw[1], tuvw[2]), tuvw[3]) < inside) continue;
      /* exclude intersections outside of segment */
      if (MIN(tuvw[0], 1.0 - tuvw[0]) < inside) continue;

      RSS(ref_face_part(ref_face, ref_node, face, &part), "face part");
      if (ref_mpi_rank(ref_mpi) == part) {
        RSS(ref_node_next_global(ref_grid_node(iso_grid), &global),
            "next global");
        RSS(ref_node_add(ref_grid_node(iso_grid), global, &(new_node[face])),
            "add node");
        t1 = tuvw[0];
        t0 = 1.0 - t1;
        for (i = 0; i < 3; i++) {
          ref_node_xyz(ref_grid_node(iso_grid), i, new_node[face]) =
              t1 * segment1[i] + t0 * segment0[i];
        }
      }
    }
  }
  if (NULL == field) {
    *iso_field_ptr = NULL;
  } else {
    REF_INT node;
    ref_malloc(*iso_field_ptr, ldim * ref_node_max(ref_grid_node(iso_grid)),
               REF_DBL);
    iso_field = *iso_field_ptr;
    if (ref_grid_twod(ref_grid)) {
      each_ref_edge(ref_edge, edge) {
        node = new_node[edge];
        if (REF_EMPTY != node) {
          for (i = 0; i < 3; i++) {
            candidate0[i] =
                ref_node_xyz(ref_node, i, ref_edge_e2n(ref_edge, 0, edge));
            candidate1[i] =
                ref_node_xyz(ref_node, i, ref_edge_e2n(ref_edge, 1, edge));
          }
          RSS(ref_iso_segment_segment(candidate0, candidate1, segment0,
                                      segment1, tt),
              "seg-seg");
          ss[0] = 1.0 - tt[0];
          ss[1] = 1.0 - tt[1];
          for (j = 0; j < ldim; j++) {
            iso_field[j + ldim * node] =
                ss[1] * field[j + ldim * ref_edge_e2n(ref_edge, 0, edge)] +
                tt[1] * field[j + ldim * ref_edge_e2n(ref_edge, 1, edge)];
          }
        }
      }
    } else {
      each_ref_face(ref_face, face) {
        node = new_node[face];
        if (REF_EMPTY != node) {
          for (i = 0; i < 3; i++) {
            triangle0[i] =
                ref_node_xyz(ref_node, i, ref_face_f2n(ref_face, 0, face));
            triangle1[i] =
                ref_node_xyz(ref_node, i, ref_face_f2n(ref_face, 1, face));
            triangle2[i] =
                ref_node_xyz(ref_node, i, ref_face_f2n(ref_face, 2, face));
          }
          RSS(ref_iso_triangle_segment(triangle0, triangle1, triangle2,
                                       segment0, segment1, tuvw),
              "tuvw");
          for (j = 0; j < ldim; j++) {
            iso_field[j + ldim * node] = 0.0;
            for (i = 0; i < 3; i++) {
              iso_field[j + ldim * node] +=
                  tuvw[i + 1] *
                  field[j + ldim * ref_face_f2n(ref_face, i, face)];
            }
          }
        }
      }
    }
  }

  if (ref_grid_twod(ref_grid)) {
    ref_cell = ref_grid_tri(ref_grid);
    each_ref_cell_valid_cell(ref_cell, cell) {
      REF_INT cell_edge, node, edge_nodes[2];
      REF_INT nedge, edges[3];
      REF_INT new_nodes[REF_CELL_MAX_SIZE_PER], new_cell;
      nedge = 0;
      each_ref_cell_cell_edge(ref_cell, cell_edge) {
        for (node = 0; node < 2; node++) {
          edge_nodes[node] = ref_cell_e2n(ref_cell, node, cell_edge, cell);
        }
        RSS(ref_edge_with(ref_edge, edge_nodes[0], edge_nodes[1], &edge),
            "get edge");
        if (REF_EMPTY != new_node[edge]) {
          edges[nedge] = edge;
          nedge++;
        }
      }
      if (2 == nedge) {
        new_nodes[0] = new_node[edges[0]];
        new_nodes[1] = new_node[edges[1]];
        new_nodes[2] = id;
        RSS(ref_cell_add(ref_grid_edg(iso_grid), new_nodes, &new_cell), "add");
      }
    }
  } else {
    ref_cell = ref_grid_tet(ref_grid);
    each_ref_cell_valid_cell(ref_cell, cell) {
      REF_INT cell_face, node, face_nodes[4];
      REF_INT nface, faces[4];
      REF_INT new_nodes[REF_CELL_MAX_SIZE_PER], new_cell;
      nface = 0;
      each_ref_cell_cell_face(ref_cell, cell_face) {
        for (node = 0; node < 4; node++) {
          face_nodes[node] = ref_cell_f2n(ref_cell, node, cell_face, cell);
        }
        RSS(ref_face_with(ref_face, face_nodes, &face), "get face");
        if (REF_EMPTY != new_node[face]) {
          faces[nface] = face;
          nface++;
        }
      }
      if (2 == nface) {
        new_nodes[0] = new_node[faces[0]];
        new_nodes[1] = new_node[faces[1]];
        new_nodes[2] = id;
        RSS(ref_cell_add(ref_grid_edg(iso_grid), new_nodes, &new_cell), "add");
      }
    }
  }

  if (NULL != ref_face) {
    RSS(ref_face_free(ref_face), "free");
  }
  if (NULL != ref_edge) {
    RSS(ref_edge_free(ref_edge), "free");
  }
  ref_free(new_node);
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_iso_segment(REF_GRID ref_grid, REF_DBL *center,
                                   REF_DBL aoa, REF_DBL phi, REF_DBL h,
                                   REF_DBL *segment0, REF_DBL *segment1) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_DBL x, x0, x1, dx;
  REF_DBL new_p[3];
  REF_INT node;
  REF_BOOL first;
  x0 = REF_DBL_MAX;
  x1 = REF_DBL_MIN;
  first = REF_TRUE;
  each_ref_node_valid_node(ref_node, node) {
    if (first) {
      first = REF_FALSE;
      x0 = ref_node_xyz(ref_node, 0, node);
      x1 = ref_node_xyz(ref_node, 0, node);
    } else {
      x0 = MIN(x0, ref_node_xyz(ref_node, 0, node));
      x1 = MAX(x1, ref_node_xyz(ref_node, 0, node));
    }
  }
  RSS(ref_mpi_min(ref_mpi, &x0, &x, REF_DBL_TYPE), "mpi min");
  RSS(ref_mpi_bcast(ref_mpi, &x, 1, REF_DBL_TYPE), "bcast min");
  x0 = x;
  RSS(ref_mpi_max(ref_mpi, &x1, &x, REF_DBL_TYPE), "mpi max");
  RSS(ref_mpi_bcast(ref_mpi, &x, 1, REF_DBL_TYPE), "bcast max");
  x1 = x;

  dx = x1 - x0;
  x0 -= 0.1 * dx;
  x1 += 0.1 * dx;

  segment0[0] = x0;
  segment0[1] = center[1] + h * sin(ref_math_in_radians(phi));
  segment0[2] = center[2] - h * cos(ref_math_in_radians(phi));

  segment1[0] = x1;
  segment1[1] = center[1] + h * sin(ref_math_in_radians(phi));
  segment1[2] = center[2] - h * cos(ref_math_in_radians(phi));

  segment0[0] -= center[0];
  segment0[1] -= center[1];
  segment0[2] -= center[2];
  new_p[0] = segment0[0] * cos(ref_math_in_radians(-aoa)) +
             segment0[2] * sin(ref_math_in_radians(-aoa));
  new_p[1] = segment0[1];
  new_p[2] = -segment0[0] * sin(ref_math_in_radians(-aoa)) +
             segment0[2] * cos(ref_math_in_radians(-aoa));
  segment0[0] = center[0] + new_p[0];
  segment0[1] = center[1] + new_p[1];
  segment0[2] = center[2] + new_p[2];

  segment1[0] -= center[0];
  segment1[1] -= center[1];
  segment1[2] -= center[2];
  new_p[0] = segment1[0] * cos(ref_math_in_radians(-aoa)) +
             segment1[2] * sin(ref_math_in_radians(-aoa));
  new_p[1] = segment1[1];
  new_p[2] = -segment1[0] * sin(ref_math_in_radians(-aoa)) +
             segment1[2] * cos(ref_math_in_radians(-aoa));
  segment1[0] = center[0] + new_p[0];
  segment1[1] = center[1] + new_p[1];
  segment1[2] = center[2] + new_p[2];

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_iso_boom_header(FILE **file_ptr, REF_INT ldim,
                                       const char **scalar_names,
                                       const char *filename) {
  REF_INT i;
  *file_ptr = fopen(filename, "w");
  if (NULL == ((void *)*file_ptr)) printf("unable to open %s\n", filename);
  RNS(*file_ptr, "unable to open file");
  fprintf(*file_ptr, "title=\"tecplot refine gather\"\n");
  fprintf(*file_ptr, "variables = \"x\" \"y\" \"z\"");
  if (NULL != scalar_names) {
    for (i = 0; i < ldim; i++) fprintf(*file_ptr, " \"%s\"", scalar_names[i]);
  } else {
    for (i = 0; i < ldim; i++) fprintf(*file_ptr, " \"V%d\"", i + 1);
  }
  fprintf(*file_ptr, "\n");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_iso_boom_zone(FILE *file, REF_GRID ref_grid,
                                     REF_DBL *field, REF_INT ldim,
                                     REF_DBL *center, REF_DBL aoa, REF_DBL phi,
                                     REF_DBL h) {
  REF_DBL segment0[3], segment1[3], ds[3], dt[3];
  REF_GRID ray_grid;
  REF_DBL *ray_field;
  REF_DBL *local_xyzf, *xyzf, *t;
  REF_NODE ref_node;
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_INT node, local_n, n, *source, i, *order;

  RSS(ref_iso_segment(ref_grid, center, aoa, phi, h, segment0, segment1),
      "seg");
  for (i = 0; i < 3; i++) ds[i] = segment1[i] - segment0[i];
  RSS(ref_math_normalize(ds), "segment unit vector");

  RSS(ref_iso_cast(&ray_grid, &ray_field, ref_grid, field, ldim, segment0,
                   segment1),
      "cast");
  ref_node = ref_grid_node(ray_grid);
  local_n = 0;
  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      local_n++;
    }
  }
  ref_malloc(local_xyzf, local_n * (3 + ldim), REF_DBL);
  local_n = 0;
  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      for (i = 0; i < 3; i++)
        local_xyzf[i + (3 + ldim) * local_n] = ref_node_xyz(ref_node, i, node);
      for (i = 0; i < ldim; i++)
        local_xyzf[3 + i + (3 + ldim) * local_n] = ray_field[i + node * ldim];
      local_n++;
    }
  }
  ref_free(ray_field);
  ref_grid_free(ray_grid);

  RSS(ref_mpi_allconcat(ref_mpi, 3 + ldim, local_n, local_xyzf, &n, &source,
                        (void **)(&xyzf), REF_DBL_TYPE),
      "concat");
  ref_free(local_xyzf);
  if (ref_mpi_once(ref_mpi)) {
    ref_free(source);
    ref_malloc(t, n, REF_DBL);

    for (node = 0; node < n; node++) {
      for (i = 0; i < 3; i++) dt[i] = xyzf[i + (3 + ldim) * node] - segment0[i];
      t[node] = ref_math_dot(dt, ds);
    }
    ref_malloc(order, n, REF_INT);
    RSS(ref_sort_heap_dbl(n, t, order), "sort t");
    fprintf(file, " zone t=\"PHI=%.1f,R=%.1f\"\n", phi, h);
    for (node = 0; node < n; node++) {
      for (i = 0; i < 3 + ldim; i++) {
        fprintf(file, " %.15e", xyzf[i + (3 + ldim) * order[node]]);
      }
      fprintf(file, "\n");
    }
    ref_free(order);
    ref_free(t);
    ref_free(xyzf);
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_iso_boomray(const char *filename, REF_GRID ref_grid,
                                   REF_DBL *field, REF_INT ldim,
                                   const char **scalar_names, REF_DBL *segment0,
                                   REF_DBL *segment1) {
  FILE *file;
  REF_DBL ds[3], dt[3];
  REF_GRID ray_grid;
  REF_DBL *ray_field;
  REF_DBL *local_xyzf, *xyzf, *t;
  REF_NODE ref_node;
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_INT node, local_n, n, *source, i, *order;
  file = fopen(filename, "w");
  if (NULL == file) printf("unable to open %s\n", filename);
  RNS(file, "unable to open file");
  fprintf(file, "title=\"tecplot refine gather\"\n");
  fprintf(file, "variables = \"x\" \"y\" \"z\"");
  if (NULL != scalar_names) {
    for (i = 0; i < ldim; i++) fprintf(file, " \"%s\"", scalar_names[i]);
  } else {
    for (i = 0; i < ldim; i++) fprintf(file, " \"V%d\"", i + 1);
  }
  fprintf(file, "\n");
  for (i = 0; i < 3; i++) ds[i] = segment1[i] - segment0[i];
  RSS(ref_math_normalize(ds), "segment unit vector");

  RSS(ref_iso_cast(&ray_grid, &ray_field, ref_grid, field, ldim, segment0,
                   segment1),
      "cast");
  ref_node = ref_grid_node(ray_grid);
  local_n = 0;
  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      local_n++;
    }
  }
  ref_malloc(local_xyzf, local_n * (3 + ldim), REF_DBL);
  local_n = 0;
  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      for (i = 0; i < 3; i++)
        local_xyzf[i + (3 + ldim) * local_n] = ref_node_xyz(ref_node, i, node);
      for (i = 0; i < ldim; i++)
        local_xyzf[3 + i + (3 + ldim) * local_n] = ray_field[i + node * ldim];
      local_n++;
    }
  }
  ref_free(ray_field);
  ref_grid_free(ray_grid);

  RSS(ref_mpi_allconcat(ref_mpi, 3 + ldim, local_n, local_xyzf, &n, &source,
                        (void **)(&xyzf), REF_DBL_TYPE),
      "concat");
  ref_free(local_xyzf);
  if (ref_mpi_once(ref_mpi)) {
    ref_free(source);
    ref_malloc(t, n, REF_DBL);

    for (node = 0; node < n; node++) {
      for (i = 0; i < 3; i++) dt[i] = xyzf[i + (3 + ldim) * node] - segment0[i];
      t[node] = ref_math_dot(dt, ds);
    }
    ref_malloc(order, n, REF_INT);
    RSS(ref_sort_heap_dbl(n, t, order), "sort t");
    fprintf(file, " zone t=\"((%.2f,%.2f,%.2f),(%.2f,%.2f,%.2f))\"\n",
            segment0[0], segment0[1], segment0[2], segment1[0], segment1[1],
            segment1[2]);
    for (node = 0; node < n; node++) {
      for (i = 0; i < 3 + ldim; i++) {
        fprintf(file, " %.15e", xyzf[i + (3 + ldim) * order[node]]);
      }
      fprintf(file, "\n");
    }
    ref_free(order);
    ref_free(t);
    ref_free(xyzf);
  }
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_iso_slice(REF_GRID *iso_grid, REF_GRID ref_grid,
                                 REF_DBL *normal, REF_DBL offset, REF_INT ldim,
                                 REF_DBL *in, REF_DBL **out) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_DBL *field;
  REF_INT node;

  ref_malloc(field, ref_node_max(ref_node), REF_DBL);
  each_ref_node_valid_node(ref_node, node) {
    field[node] = normal[0] * ref_node_xyz(ref_node, 0, node) +
                  normal[1] * ref_node_xyz(ref_node, 1, node) +
                  normal[2] * ref_node_xyz(ref_node, 2, node) - offset;
  }
  RSS(ref_iso_insert(iso_grid, ref_grid, field, ldim, in, out), "iso");
  ref_free(field);

  return REF_SUCCESS;
}
