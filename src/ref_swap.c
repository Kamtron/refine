
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

#include "ref_swap.h"

#include <stdio.h>
#include <stdlib.h>

#include "ref_edge.h"
#include "ref_export.h"
#include "ref_math.h"

/* parallel requirement, all local */
REF_FCN REF_STATUS ref_swap_remove_two_face_cell(REF_GRID ref_grid,
                                                 REF_INT cell) {
  REF_CELL ref_cell;
  REF_INT cell_face;
  REF_INT node;
  REF_INT cell_nodes[4];
  REF_INT face_nodes[4];
  REF_INT found;
  REF_INT face0, face1;
  REF_INT cell_face0, cell_face1;
  REF_INT faceid0, faceid1;
  REF_INT temp, face;

  ref_cell = ref_grid_tet(ref_grid);

  face0 = REF_EMPTY;
  face1 = REF_EMPTY;
  faceid0 = REF_EMPTY;
  faceid1 = REF_EMPTY;
  cell_face0 = REF_EMPTY;
  cell_face1 = REF_EMPTY;
  for (cell_face = 0; cell_face < 4; cell_face++) {
    for (node = 0; node < 4; node++)
      cell_nodes[node] = ref_cell_f2n(ref_cell, node, cell_face, cell);
    if (REF_SUCCESS ==
        ref_cell_with(ref_grid_tri(ref_grid), cell_nodes, &found)) {
      RSS(ref_cell_nodes(ref_grid_tri(ref_grid), found, face_nodes), "tri");
      if (REF_EMPTY == face0) {
        face0 = found;
        faceid0 = face_nodes[3];
        cell_face0 = cell_face;
      } else {
        if (REF_EMPTY != face1) {
          RSS(REF_INVALID, "three or more faces detected");
        }
        face1 = found;
        faceid1 = face_nodes[3];
        cell_face1 = cell_face;
      }
    }
  }

  if (REF_EMPTY == face0) return REF_INVALID;
  if (REF_EMPTY == face1) return REF_INVALID;
  if (faceid0 != faceid1) return REF_INVALID;

  RSS(ref_cell_remove(ref_grid_tri(ref_grid), face0), "remove tri0");
  RSS(ref_cell_remove(ref_grid_tri(ref_grid), face1), "remove tri1");

  for (cell_face = 0; cell_face < 4; cell_face++) {
    for (node = 0; node < 4; node++)
      cell_nodes[node] = ref_cell_f2n(ref_cell, node, cell_face, cell);
    if (cell_face != cell_face0 && cell_face != cell_face1) {
      cell_nodes[3] = faceid0;
      temp = cell_nodes[0];
      cell_nodes[0] = cell_nodes[1];
      cell_nodes[1] = temp;
      RSS(ref_cell_add(ref_grid_tri(ref_grid), cell_nodes, &face), "add tri");
    }
  }

  RSS(ref_cell_remove(ref_cell, cell), "remove tet");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_swap_remove_three_face_cell(REF_GRID ref_grid,
                                                   REF_INT cell) {
  REF_CELL ref_cell;
  REF_INT cell_face;
  REF_INT node;
  REF_INT cell_nodes[4];
  REF_INT cell_face_nodes[4];
  REF_INT face_nodes[4];
  REF_INT found;
  REF_INT face0, face1, face2;
  REF_INT cell_face0, cell_face1, cell_face2;
  REF_INT faceid0, faceid1, faceid2;
  REF_INT temp, face;
  REF_INT remove_this_node;

  ref_cell = ref_grid_tet(ref_grid);

  face0 = REF_EMPTY;
  face1 = REF_EMPTY;
  face2 = REF_EMPTY;
  faceid0 = REF_EMPTY;
  faceid1 = REF_EMPTY;
  faceid2 = REF_EMPTY;
  cell_face0 = REF_EMPTY;
  cell_face1 = REF_EMPTY;
  cell_face2 = REF_EMPTY;
  for (cell_face = 0; cell_face < 4; cell_face++) {
    for (node = 0; node < 4; node++)
      cell_face_nodes[node] = ref_cell_f2n(ref_cell, node, cell_face, cell);
    if (REF_SUCCESS ==
        ref_cell_with(ref_grid_tri(ref_grid), cell_face_nodes, &found)) {
      RSS(ref_cell_nodes(ref_grid_tri(ref_grid), found, face_nodes), "tri");
      if (REF_EMPTY == face0) {
        face0 = found;
        faceid0 = face_nodes[3];
        cell_face0 = cell_face;
      } else if (REF_EMPTY == face1) {
        face1 = found;
        faceid1 = face_nodes[3];
        cell_face1 = cell_face;
      } else {
        if (REF_EMPTY != face2) {
          RSS(REF_INVALID, "four faces detected");
        }
        face2 = found;
        faceid2 = face_nodes[3];
        cell_face2 = cell_face;
      }
    }
  }

  if (REF_EMPTY == face0) return REF_INVALID;
  if (REF_EMPTY == face1) return REF_INVALID;
  if (REF_EMPTY == face2) return REF_INVALID;
  if (faceid0 != faceid1 || faceid0 != faceid2) return REF_INVALID;

  RSS(ref_cell_remove(ref_grid_tri(ref_grid), face0), "remove tri0");
  RSS(ref_cell_remove(ref_grid_tri(ref_grid), face1), "remove tri1");
  RSS(ref_cell_remove(ref_grid_tri(ref_grid), face2), "remove tri1");

  cell_face = 0 + 1 + 2 + 3 - cell_face0 - cell_face1 - cell_face2;

  for (node = 0; node < 4; node++)
    face_nodes[node] = ref_cell_f2n(ref_cell, node, cell_face, cell);

  face_nodes[3] = faceid0;
  temp = face_nodes[0];
  face_nodes[0] = face_nodes[1];
  face_nodes[1] = temp;
  RSS(ref_cell_add(ref_grid_tri(ref_grid), face_nodes, &face), "add tri");

  RSS(ref_cell_nodes(ref_cell, cell, cell_nodes), "tet");

  remove_this_node = cell_nodes[0] + cell_nodes[1] + cell_nodes[2] +
                     cell_nodes[3] - face_nodes[0] - face_nodes[1] -
                     face_nodes[2];

  RSS(ref_cell_remove(ref_cell, cell), "remove tet");

  RSS(ref_node_remove(ref_grid_node(ref_grid), remove_this_node),
      "remove node");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_swap_pass(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL tri = ref_grid_tri(ref_grid);
  REF_CELL tet = ref_grid_tet(ref_grid);
  REF_INT tri_index, tri_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT i, face_nodes[4];
  REF_INT tet0, tet1;
  REF_INT faceid0, faceid1;
  REF_BOOL more_than_two;
  REF_INT cell_face, node, cell_nodes[4];
  REF_INT found, found_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT rank;

  each_ref_cell_valid_cell_with_nodes(tri, tri_index, tri_nodes) {
    for (i = 0; i < 3; i++) face_nodes[i] = tri_nodes[i];
    face_nodes[3] = face_nodes[0];
    RSS(ref_cell_with_face(tet, face_nodes, &tet0, &tet1),
        "unable to find tets with face");
    if (REF_EMPTY == tet0) THROW("boundary tet missing");
    if (REF_EMPTY != tet1) THROW("boundary tri has two tets, not manifold");

    /* must be local to swap */
    rank = ref_mpi_rank(ref_grid_mpi(ref_grid));
    if (rank != ref_node_part(ref_node, ref_cell_c2n(tet, 0, tet0)) ||
        rank != ref_node_part(ref_node, ref_cell_c2n(tet, 1, tet0)) ||
        rank != ref_node_part(ref_node, ref_cell_c2n(tet, 2, tet0)) ||
        rank != ref_node_part(ref_node, ref_cell_c2n(tet, 3, tet0)))
      continue;

    faceid0 = REF_EMPTY;
    faceid1 = REF_EMPTY;
    more_than_two = REF_FALSE;
    for (cell_face = 0; cell_face < 4; cell_face++) {
      for (node = 0; node < 4; node++)
        cell_nodes[node] = ref_cell_f2n(tet, node, cell_face, tet0);
      if (REF_SUCCESS == ref_cell_with(tri, cell_nodes, &found)) {
        RSS(ref_cell_nodes(tri, found, found_nodes), "tri");
        if (REF_EMPTY == faceid0) {
          faceid0 = found_nodes[3];
        } else {
          if (REF_EMPTY != faceid1) {
            more_than_two = REF_TRUE;
          }
          faceid1 = found_nodes[3];
        }
      }
    }
    if (REF_EMPTY == faceid0 || REF_EMPTY == faceid1 || faceid0 != faceid1 ||
        more_than_two)
      continue;
    RSS(ref_swap_remove_two_face_cell(ref_grid, tet0), "remove it");
  }
  return REF_SUCCESS;
}

/*
 *      n2
 *    /   \
 * n0 ----- n1
 *    \   /
 *      n3
 */

REF_FCN REF_STATUS ref_swap_node23(REF_GRID ref_grid, REF_INT node0,
                                   REF_INT node1, REF_INT *node2,
                                   REF_INT *node3) {
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT ncell, cell_to_swap[2];
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  *node2 = REF_EMPTY;
  *node3 = REF_EMPTY;
  RSS(ref_cell_list_with2(ref_cell, node0, node1, 2, &ncell, cell_to_swap),
      "more then two");
  REIS(2, ncell, "there should be two triangles for manifold");

  RSS(ref_cell_nodes(ref_cell, cell_to_swap[0], nodes), "nodes tri0");
  if (node0 == nodes[0] && node1 == nodes[1]) *node2 = nodes[2];
  if (node0 == nodes[1] && node1 == nodes[2]) *node2 = nodes[0];
  if (node0 == nodes[2] && node1 == nodes[0]) *node2 = nodes[1];
  if (node1 == nodes[0] && node0 == nodes[1]) *node3 = nodes[2];
  if (node1 == nodes[1] && node0 == nodes[2]) *node3 = nodes[0];
  if (node1 == nodes[2] && node0 == nodes[0]) *node3 = nodes[1];
  RSS(ref_cell_nodes(ref_cell, cell_to_swap[1], nodes), "nodes tri0");
  if (node0 == nodes[0] && node1 == nodes[1]) *node2 = nodes[2];
  if (node0 == nodes[1] && node1 == nodes[2]) *node2 = nodes[0];
  if (node0 == nodes[2] && node1 == nodes[0]) *node2 = nodes[1];
  if (node1 == nodes[0] && node0 == nodes[1]) *node3 = nodes[2];
  if (node1 == nodes[1] && node0 == nodes[2]) *node3 = nodes[0];
  if (node1 == nodes[2] && node0 == nodes[0]) *node3 = nodes[1];

  RUB(REF_EMPTY, *node2, "node2 not found", {
    REF_DBL normal[3];
    REF_NODE ref_node = ref_grid_node(ref_grid);
    ref_node_location(ref_grid_node(ref_grid), node0);
    ref_node_location(ref_grid_node(ref_grid), node1);
    printf("node2 %d node3 %d\n", *node2, *node3);
    ref_cell_nodes(ref_cell, cell_to_swap[0], nodes);
    ref_node_tri_normal(ref_node, nodes, normal);
    printf("cell %d node %d %d %d %d %f\n", cell_to_swap[0], nodes[0], nodes[1],
           nodes[2], nodes[3], normal[1]);
    ref_node_location(ref_grid_node(ref_grid), nodes[0]);
    ref_node_location(ref_grid_node(ref_grid), nodes[1]);
    ref_node_location(ref_grid_node(ref_grid), nodes[2]);
    ref_cell_nodes(ref_cell, cell_to_swap[1], nodes);
    ref_node_tri_normal(ref_node, nodes, normal);
    printf("cell %d node %d %d %d %d %f\n", cell_to_swap[1], nodes[0], nodes[1],
           nodes[2], nodes[3], normal[1]);
    ref_node_location(ref_grid_node(ref_grid), nodes[0]);
    ref_node_location(ref_grid_node(ref_grid), nodes[1]);
    ref_node_location(ref_grid_node(ref_grid), nodes[2]);
    ref_export_by_extension(ref_grid, "ref_swap_node23.tec");
  });
  RUB(REF_EMPTY, *node3, "node3 not found", {
    ref_node_location(ref_grid_node(ref_grid), node0);
    ref_node_location(ref_grid_node(ref_grid), node1);
    printf("node2 %d node3 %d\n", *node2, *node3);
    ref_cell_nodes(ref_cell, cell_to_swap[0], nodes);
    printf("cell %d node %d %d %d %d\n", cell_to_swap[0], nodes[0], nodes[1],
           nodes[2], nodes[3]);
    ref_node_location(ref_grid_node(ref_grid), nodes[0]);
    ref_node_location(ref_grid_node(ref_grid), nodes[1]);
    ref_node_location(ref_grid_node(ref_grid), nodes[2]);
    ref_cell_nodes(ref_cell, cell_to_swap[1], nodes);
    printf("cell %d node %d %d %d %d\n", cell_to_swap[1], nodes[0], nodes[1],
           nodes[2], nodes[3]);
    ref_node_location(ref_grid_node(ref_grid), nodes[0]);
    ref_node_location(ref_grid_node(ref_grid), nodes[1]);
    ref_node_location(ref_grid_node(ref_grid), nodes[2]);
    ref_export_by_extension(ref_grid, "ref_swap_node23.tec");
  });

  return REF_SUCCESS;
}

static void ref_swap_tattle_nodes(REF_CELL ref_cell, REF_INT node0,
                                  REF_INT node1) {
  REF_INT item, cell_node, cell, cell_node2;
  each_ref_cell_having_node2(ref_cell, node0, node1, item, cell_node, cell) {
    printf("cell %d cell id %d\n", cell,
           ref_cell_c2n(ref_cell, ref_cell_id_index(ref_cell), cell));
    each_ref_cell_cell_node(ref_cell, cell_node2) {
      printf(" %d", ref_cell_c2n(ref_cell, cell_node2, cell));
    }
    printf(" %d\n", ref_cell_c2n(ref_cell, ref_cell_id_index(ref_cell), cell));
  }
}

REF_FCN REF_STATUS ref_swap_same_faceid(REF_GRID ref_grid, REF_INT node0,
                                        REF_INT node1, REF_BOOL *allowed) {
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT ncell;
  REF_INT cell_to_swap[2];
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT id0, id1;
  REF_BOOL has_edg, has_tri, has_qua;

  *allowed = REF_FALSE;

  RSS(ref_cell_has_side(ref_grid_edg(ref_grid), node0, node1, &has_edg),
      "edg side");
  RSS(ref_cell_has_side(ref_grid_tri(ref_grid), node0, node1, &has_tri),
      "tri side");
  RSS(ref_cell_has_side(ref_grid_qua(ref_grid), node0, node1, &has_qua),
      "qua side");
  if (has_edg || (has_tri && has_qua)) {
    *allowed = REF_FALSE;
    return REF_SUCCESS;
  }
  RSB(ref_cell_list_with2(ref_cell, node0, node1, 2, &ncell, cell_to_swap),
      "more then two", {
        ref_node_location(ref_grid_node(ref_grid), node0);
        ref_node_location(ref_grid_node(ref_grid), node1);
        ref_swap_tattle_nodes(ref_cell, node0, node1);
        ref_export_by_extension(ref_grid, "ref_swap_same_faceid.tec");
      });

  if (0 == ncell) { /* away from boundary */
    *allowed = REF_TRUE;
    return REF_SUCCESS;
  }
  REIB(2, ncell, "there should be zero or two triangles for manifold", {
    ref_node_location(ref_grid_node(ref_grid), node0);
    ref_node_location(ref_grid_node(ref_grid), node1);
    ref_export_by_extension(ref_grid, "ref_swap_same_faceid.tec");
  });

  RSS(ref_cell_nodes(ref_cell, cell_to_swap[0], nodes), "nodes tri0");
  id0 = nodes[ref_cell_node_per(ref_cell)];
  RSS(ref_cell_nodes(ref_cell, cell_to_swap[1], nodes), "nodes tri1");
  id1 = nodes[ref_cell_node_per(ref_cell)];

  if (id0 == id1) {
    *allowed = REF_TRUE;
    return REF_SUCCESS;
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_swap_manifold(REF_GRID ref_grid, REF_INT node0,
                                     REF_INT node1, REF_BOOL *allowed) {
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT ncell;
  REF_INT cell_to_swap[2];
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT node2, node3;
  REF_INT new_cell;
  REF_BOOL has_side;

  *allowed = REF_FALSE;

  RSS(ref_swap_node23(ref_grid, node0, node1, &node2, &node3), "other nodes");
  RSS(ref_cell_list_with2(ref_cell, node0, node1, 2, &ncell, cell_to_swap),
      "more then two");
  REIS(2, ncell, "there should be two triangles for manifold");
  RSS(ref_cell_nodes(ref_cell, cell_to_swap[0], nodes), "nodes tri0");

  nodes[0] = node0;
  nodes[1] = node3;
  nodes[2] = node2;
  RXS(ref_cell_with(ref_cell, nodes, &new_cell), REF_NOT_FOUND,
      "with node0 failed");
  if (REF_EMPTY != new_cell) {
    *allowed = REF_FALSE;
    return REF_SUCCESS;
  }

  nodes[0] = node1;
  nodes[1] = node2;
  nodes[2] = node3;
  RXS(ref_cell_with(ref_cell, nodes, &new_cell), REF_NOT_FOUND,
      "with node1 failed");
  if (REF_EMPTY != new_cell) {
    *allowed = REF_FALSE;
    return REF_SUCCESS;
  }

  RSS(ref_cell_has_side(ref_cell, node2, node3, &has_side), "find edge swap");
  if (has_side) { /* would create an edge that already exists */
    *allowed = REF_FALSE;
    return REF_SUCCESS;
  }

  *allowed = REF_TRUE;

  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_swap_outward_norm(REF_GRID ref_grid,
                                                REF_INT node0, REF_INT node1,
                                                REF_BOOL *allowed) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT node2, node3;
  REF_BOOL valid;

  *allowed = REF_FALSE;

  RSS(ref_swap_node23(ref_grid, node0, node1, &node2, &node3), "other nodes");

  nodes[0] = node0;
  nodes[1] = node3;
  nodes[2] = node2;
  RSS(ref_node_tri_twod_orientation(ref_node, nodes, &valid), "valid");
  if (!valid) return REF_SUCCESS;

  nodes[0] = node1;
  nodes[1] = node2;
  nodes[2] = node3;
  RSS(ref_node_tri_twod_orientation(ref_node, nodes, &valid), "valid");
  if (!valid) return REF_SUCCESS;

  *allowed = REF_TRUE;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_swap_geom_topo(REF_GRID ref_grid, REF_INT node0,
                                      REF_INT node1, REF_BOOL *allowed) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_BOOL node0_has_jump, node1_has_jump;

  *allowed = REF_FALSE;

  RSS(ref_geom_has_jump(ref_geom, node0, &node0_has_jump), "n0 jump");
  RSS(ref_geom_has_jump(ref_geom, node1, &node1_has_jump), "n1 jump");

  *allowed = !node0_has_jump && !node1_has_jump;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_swap_local_cell(REF_GRID ref_grid, REF_INT node0,
                                       REF_INT node1, REF_BOOL *allowed) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT item, cell, search_node, test_node;

  *allowed = REF_FALSE;

  ref_cell = ref_grid_tri(ref_grid);
  each_ref_cell_having_node(ref_cell, node0, item, cell) {
    for (search_node = 0; search_node < ref_cell_node_per(ref_cell);
         search_node++) {
      if (node1 == ref_cell_c2n(ref_cell, search_node, cell)) {
        for (test_node = 0; test_node < ref_cell_node_per(ref_cell);
             test_node++) {
          if (!ref_node_owned(ref_node,
                              ref_cell_c2n(ref_cell, test_node, cell))) {
            return REF_SUCCESS;
          }
        }
      }
    }
  }

  *allowed = REF_TRUE;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_swap_conforming(REF_GRID ref_grid, REF_INT node0,
                                       REF_INT node1, REF_BOOL *allowed) {
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_INT ncell, cell_to_swap[2];
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT node2, node3;
  REF_BOOL cell0_support, cell1_support;
  REF_DBL normdev0, normdev1, normdev2, normdev3;
  REF_DBL sign_uv_area, uv_area2, uv_area3, area2, area3;
  REF_BOOL normdev_allowed, uv_area_allowed;
  REF_BOOL supported;
  REF_DBL normal0[3], normal1[3], normal2[3], normal3[3], dot;

  *allowed = REF_FALSE;

  RSS(ref_swap_node23(ref_grid, node0, node1, &node2, &node3), "other nodes");

  RSS(ref_cell_list_with2(ref_cell, node0, node1, 2, &ncell, cell_to_swap),
      "more then two");
  REIS(2, ncell, "there should be two triangles for manifold");
  RSS(ref_cell_nodes(ref_cell, cell_to_swap[0], nodes), "nodes");
  RSS(ref_geom_tri_supported(ref_geom, nodes, &cell0_support), "tri support");
  RSS(ref_cell_nodes(ref_cell, cell_to_swap[1], nodes), "nodes");
  RSS(ref_geom_tri_supported(ref_geom, nodes, &cell1_support), "tri support");

  if (!cell0_support || !cell1_support) {
    RSS(ref_cell_nodes(ref_cell, cell_to_swap[0], nodes), "nodes");
    RSS(ref_node_tri_normal(ref_node, nodes, normal0), "tri 0 normal");
    RSS(ref_math_normalize(normal0), "triangle 0 has zero area");
    RSS(ref_cell_nodes(ref_cell, cell_to_swap[1], nodes), "nodes");
    RSS(ref_node_tri_normal(ref_node, nodes, normal1), "tri 1 normal");
    RSS(ref_math_normalize(normal1), "triangle 1 has zero area");
    dot = ref_math_dot(normal0, normal1);
    if (dot < ref_node_same_normal_tol(ref_node)) {
      *allowed = REF_FALSE;
      return REF_SUCCESS;
    }
    nodes[0] = node0;
    nodes[1] = node3;
    nodes[2] = node2;
    RSS(ref_node_tri_normal(ref_node, nodes, normal2), "tri 2 normal");
    nodes[0] = node1;
    nodes[1] = node2;
    nodes[2] = node3;
    RSS(ref_node_tri_normal(ref_node, nodes, normal3), "tri 3 normal");
    dot = MIN(ref_math_dot(normal2, normal2), ref_math_dot(normal3, normal3));
    if (!ref_math_divisible(1.0, dot)) {
      *allowed = REF_FALSE;
      return REF_SUCCESS;
    }
    RSS(ref_math_normalize(normal2), "triangle 2 has zero area");
    RSS(ref_math_normalize(normal3), "triangle 3 has zero area");
    dot = ref_math_dot(normal2, normal3);
    if (dot < ref_node_same_normal_tol(ref_node)) {
      *allowed = REF_FALSE;
      return REF_SUCCESS;
    }
    *allowed = REF_TRUE;
    return REF_SUCCESS;
  }

  RSS(ref_cell_nodes(ref_cell, cell_to_swap[0], nodes), "nodes tri0");
  RSS(ref_geom_tri_norm_deviation(ref_grid, nodes, &normdev0), "nd0");
  RSS(ref_cell_nodes(ref_cell, cell_to_swap[1], nodes), "nodes tri1");
  RSS(ref_geom_tri_norm_deviation(ref_grid, nodes, &normdev1), "nd1");
  nodes[0] = node0;
  nodes[1] = node3;
  nodes[2] = node2;
  RSS(ref_geom_cell_tuv_supported(ref_geom, nodes, REF_GEOM_FACE, &supported),
      "allow a swap to");
  if (!supported) return REF_SUCCESS;
  if (REF_SUCCESS != ref_geom_tri_norm_deviation(ref_grid, nodes, &normdev2))
    return REF_SUCCESS;
  RSS(ref_geom_uv_area(ref_geom, nodes, &uv_area2), "uv area");
  RSS(ref_node_tri_area(ref_node, nodes, &area2), "area2");
  nodes[0] = node1;
  nodes[1] = node2;
  nodes[2] = node3;
  RSS(ref_geom_cell_tuv_supported(ref_geom, nodes, REF_GEOM_FACE, &supported),
      "allow a swap to");
  if (!supported) return REF_SUCCESS;
  if (REF_SUCCESS != ref_geom_tri_norm_deviation(ref_grid, nodes, &normdev3))
    return REF_SUCCESS;
  RSS(ref_geom_uv_area(ref_geom, nodes, &uv_area3), "uv area");
  RSS(ref_node_tri_area(ref_node, nodes, &area3), "area2");

  normdev_allowed =
      ((MIN(normdev2, normdev3) > MIN(normdev0, normdev1)) ||
       ((MIN(normdev2, normdev3) > 0.9 * MIN(normdev0, normdev1)) &&
        (normdev2 > ref_grid_adapt(ref_grid, post_min_normdev) &&
         normdev3 > ref_grid_adapt(ref_grid, post_min_normdev))));

  normdev_allowed = normdev_allowed &&
                    (area2 > ref_node_min_volume(ref_node)) &&
                    (area3 > ref_node_min_volume(ref_node));

  /* skip uv checks for meshlink */
  if (ref_geom_meshlinked(ref_geom)) {
    *allowed = normdev_allowed;
    return REF_SUCCESS;
  }

  RSS(ref_geom_uv_area_sign(ref_grid, nodes[ref_cell_node_per(ref_cell)],
                            &sign_uv_area),
      "uv area sign");

  uv_area_allowed = (sign_uv_area * uv_area2 > ref_node_min_uv_area(ref_node) &&
                     sign_uv_area * uv_area3 > ref_node_min_uv_area(ref_node));

  if (normdev_allowed && uv_area_allowed) {
    *allowed = REF_TRUE;
    return REF_SUCCESS;
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_swap_ratio(REF_GRID ref_grid, REF_INT node0,
                                  REF_INT node1, REF_BOOL *allowed) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node2, node3;
  REF_DBL ratio;

  *allowed = REF_FALSE;

  RSS(ref_swap_node23(ref_grid, node0, node1, &node2, &node3), "other nodes");
  RSS(ref_node_ratio(ref_node, node2, node3, &ratio), "ratio");

  if (ref_grid_adapt(ref_grid, post_min_ratio) < ratio &&
      ratio < ref_grid_adapt(ref_grid, post_max_ratio)) {
    *allowed = REF_TRUE;
    return REF_SUCCESS;
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_swap_quality(REF_GRID ref_grid, REF_INT node0,
                                    REF_INT node1, REF_BOOL *allowed) {
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT ncell, cell_to_swap[2];
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT node2, node3;
  REF_DBL quality0, quality1, quality2, quality3;

  *allowed = REF_FALSE;

  RSS(ref_swap_node23(ref_grid, node0, node1, &node2, &node3), "other nodes");

  RSS(ref_cell_list_with2(ref_cell, node0, node1, 2, &ncell, cell_to_swap),
      "more then two");
  REIS(2, ncell, "there should be two triangles for manifold");

  RSS(ref_cell_nodes(ref_cell, cell_to_swap[0], nodes), "nodes tri0");
  RSS(ref_node_tri_quality(ref_node, nodes, &quality0), "qual");
  RSS(ref_cell_nodes(ref_cell, cell_to_swap[1], nodes), "nodes tri1");
  RSS(ref_node_tri_quality(ref_node, nodes, &quality1), "qual");
  nodes[0] = node0;
  nodes[1] = node3;
  nodes[2] = node2;
  RSS(ref_node_tri_quality(ref_node, nodes, &quality2), "qual");
  nodes[0] = node1;
  nodes[1] = node2;
  nodes[2] = node3;
  RSS(ref_node_tri_quality(ref_node, nodes, &quality3), "qual");

  if (MIN(quality2, quality3) < MIN(quality0, quality1)) {
    *allowed = REF_FALSE;
    return REF_SUCCESS;
  }

  *allowed = REF_TRUE;
  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_swap_tri_edge(REF_GRID ref_grid, REF_INT node0,
                                            REF_INT node1) {
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT ncell, cell_to_swap[2];
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT node2, node3;
  REF_INT new_cell;

  RSS(ref_swap_node23(ref_grid, node0, node1, &node2, &node3), "other nodes");

  RSS(ref_cell_list_with2(ref_cell, node0, node1, 2, &ncell, cell_to_swap),
      "more then two");
  REIS(2, ncell, "there should be two triangles for manifold");
  RSS(ref_cell_nodes(ref_cell, cell_to_swap[0], nodes), "nodes tri0");
  RSS(ref_cell_remove(ref_cell, cell_to_swap[0]), "remove");
  RSS(ref_cell_remove(ref_cell, cell_to_swap[1]), "remove");

  nodes[0] = node0;
  nodes[1] = node3;
  nodes[2] = node2;
  RSS(ref_cell_add(ref_cell, nodes, &new_cell), "add node0 version");
  nodes[0] = node1;
  nodes[1] = node2;
  nodes[2] = node3;
  RSS(ref_cell_add(ref_cell, nodes, &new_cell), "add node0 version");

  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_swap_edge_mixed(REF_GRID ref_grid, REF_INT node0,
                                              REF_INT node1,
                                              REF_BOOL *allowed) {
  REF_BOOL qua_side, pri_side, pyr_side, hex_side;

  RSS(ref_cell_has_side(ref_grid_qua(ref_grid), node0, node1, &qua_side),
      "qua");
  RSS(ref_cell_has_side(ref_grid_pri(ref_grid), node0, node1, &pri_side),
      "pri");
  RSS(ref_cell_has_side(ref_grid_pyr(ref_grid), node0, node1, &pyr_side),
      "pyr");
  RSS(ref_cell_has_side(ref_grid_hex(ref_grid), node0, node1, &hex_side),
      "hex");

  *allowed = (!qua_side && !pri_side && !pyr_side && !hex_side);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_swap_tri_pass(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_EDGE ref_edge;
  REF_INT edge, node0, node1;
  REF_BOOL allowed, has_edg, has_tri;

  RAS(ref_grid_surf(ref_grid) || ref_grid_twod(ref_grid), "only twod/surf");

  RSS(ref_edge_create(&ref_edge, ref_grid), "orig edges");
  for (edge = 0; edge < ref_edge_n(ref_edge); edge++) {
    node0 = ref_edge_e2n(ref_edge, 0, edge);
    node1 = ref_edge_e2n(ref_edge, 1, edge);

    RSS(ref_cell_has_side(ref_grid_edg(ref_grid), node0, node1, &has_edg),
        "has edg");
    if (has_edg) continue;

    RSS(ref_cell_has_side(ref_grid_tri(ref_grid), node0, node1, &has_tri),
        "still triangle side");
    if (!has_tri) continue;

    /* skip if neither node is owned */
    if (!ref_node_owned(ref_node, node0) && !ref_node_owned(ref_node, node1))
      continue;

    RSS(ref_swap_edge_mixed(ref_grid, node0, node1, &allowed), "faceid");
    if (!allowed) continue;
    RSS(ref_swap_same_faceid(ref_grid, node0, node1, &allowed), "faceid");
    if (!allowed) continue;
    RSS(ref_swap_manifold(ref_grid, node0, node1, &allowed), "manifold");
    if (!allowed) continue;
    RSS(ref_swap_geom_topo(ref_grid, node0, node1, &allowed), "topo");
    if (!allowed) continue;
    RSS(ref_swap_quality(ref_grid, node0, node1, &allowed), "qual");
    if (!allowed) continue;
    RSS(ref_swap_ratio(ref_grid, node0, node1, &allowed), "ratio");
    if (!allowed) continue;
    if (ref_grid_surf(ref_grid)) {
      RSS(ref_swap_conforming(ref_grid, node0, node1, &allowed), "normdev");
      if (!allowed) continue;
    } else {
      RSS(ref_swap_outward_norm(ref_grid, node0, node1, &allowed), "area");
      if (!allowed) continue;
    }

    RSS(ref_swap_local_cell(ref_grid, node0, node1, &allowed), "local");
    if (!allowed) {
      ref_node_age(ref_node, node0)++;
      ref_node_age(ref_node, node1)++;
      continue;
    }

    RSS(ref_swap_tri_edge(ref_grid, node0, node1), "swap");
  }

  ref_edge_free(ref_edge);

  return REF_SUCCESS;
}
