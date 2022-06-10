
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ref_adapt.h"
#include "ref_adj.h"
#include "ref_args.h"
#include "ref_cell.h"
#include "ref_collapse.h"
#include "ref_dict.h"
#include "ref_edge.h"
#include "ref_export.h"
#include "ref_fixture.h"
#include "ref_gather.h"
#include "ref_grid.h"
#include "ref_import.h"
#include "ref_list.h"
#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_matrix.h"
#include "ref_metric.h"
#include "ref_mpi.h"
#include "ref_node.h"
#include "ref_part.h"
#include "ref_phys.h"
#include "ref_smooth.h"
#include "ref_sort.h"
#include "ref_split.h"
#include "ref_swap.h"

int main(int argc, char *argv[]) {
  REF_INT pos;
  REF_MPI ref_mpi;
  RSS(ref_mpi_start(argc, argv), "start");
  RSS(ref_mpi_create(&ref_mpi), "make mpi");

  { /* slice tri */
    REF_GRID ref_grid, iso_grid;
    REF_NODE ref_node;
    REF_DBL *field;
    REF_INT node;
    REF_DBL offset = 0.2;

    RSS(ref_fixture_tri_grid(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc(field, ref_node_max(ref_node), REF_DBL);
    each_ref_node_valid_node(ref_node, node) {
      field[node] = ref_node_xyz(ref_node, 0, node) - offset;
    }
    RSS(ref_iso_insert(&iso_grid, ref_grid, field, 0, NULL, NULL), "iso");
    if (!ref_mpi_para(ref_mpi)) {
      REIS(2, ref_node_n(ref_grid_node(iso_grid)), "two nodes");
      REIS(0, ref_cell_n(ref_grid_tri(iso_grid)), "no tri");
      REIS(1, ref_cell_n(ref_grid_edg(iso_grid)), "one edg");
    }
    ref_grid_free(iso_grid);
    ref_free(field);
    ref_grid_free(ref_grid);
  }

  { /* slice tet, node 1 */
    REF_GRID ref_grid, iso_grid;
    REF_NODE ref_node;
    REF_DBL *field;
    REF_INT node;
    REF_DBL offset = 0.2;

    RSS(ref_fixture_tet_grid(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc(field, ref_node_max(ref_node), REF_DBL);
    each_ref_node_valid_node(ref_node, node) {
      field[node] = ref_node_xyz(ref_node, 0, node) - offset;
    }
    RSS(ref_iso_insert(&iso_grid, ref_grid, field, 0, NULL, NULL), "iso");
    if (!ref_mpi_para(ref_mpi)) {
      REIS(3, ref_node_n(ref_grid_node(iso_grid)), "three nodes");
      REIS(1, ref_cell_n(ref_grid_tri(iso_grid)), "one tri");
      REIS(0, ref_cell_n(ref_grid_edg(iso_grid)), "no edg");
    }
    ref_grid_free(iso_grid);
    ref_free(field);
    ref_grid_free(ref_grid);
  }

  { /* slice tet, node 0 1 */
    REF_GRID ref_grid, iso_grid;
    REF_NODE ref_node;
    REF_DBL *field;

    RSS(ref_fixture_tet_grid(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc(field, ref_node_max(ref_node), REF_DBL);
    field[0] = 1;
    field[1] = 1;
    field[2] = -1;
    field[3] = -1;
    RSS(ref_iso_insert(&iso_grid, ref_grid, field, 0, NULL, NULL), "iso");
    if (!ref_mpi_para(ref_mpi)) {
      REIS(4, ref_node_n(ref_grid_node(iso_grid)), "three nodes");
      REIS(2, ref_cell_n(ref_grid_tri(iso_grid)), "one tri");
      REIS(0, ref_cell_n(ref_grid_edg(iso_grid)), "no edg");
    }
    ref_grid_free(iso_grid);
    ref_free(field);
    ref_grid_free(ref_grid);
  }

  { /* slice tet, node 0 2 */
    REF_GRID ref_grid, iso_grid;
    REF_NODE ref_node;
    REF_DBL *field;

    RSS(ref_fixture_tet_grid(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc(field, ref_node_max(ref_node), REF_DBL);
    field[0] = 1;
    field[1] = -1;
    field[2] = 1;
    field[3] = -1;
    RSS(ref_iso_insert(&iso_grid, ref_grid, field, 0, NULL, NULL), "iso");
    if (!ref_mpi_para(ref_mpi)) {
      REIS(4, ref_node_n(ref_grid_node(iso_grid)), "three nodes");
      REIS(2, ref_cell_n(ref_grid_tri(iso_grid)), "one tri");
      REIS(0, ref_cell_n(ref_grid_edg(iso_grid)), "no edg");
    }
    ref_grid_free(iso_grid);
    ref_free(field);
    ref_grid_free(ref_grid);
  }

  { /* slice tet, node 0 3 */
    REF_GRID ref_grid, iso_grid;
    REF_NODE ref_node;
    REF_DBL *field;
    REF_DBL *out;
    REF_INT node;

    RSS(ref_fixture_tet_grid(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc(field, ref_node_max(ref_node), REF_DBL);
    field[0] = 1;
    field[1] = -1;
    field[2] = -1;
    field[3] = 1;
    RSS(ref_iso_insert(&iso_grid, ref_grid, field, 1, field, &out), "iso");
    if (!ref_mpi_para(ref_mpi)) {
      REIS(4, ref_node_n(ref_grid_node(iso_grid)), "three nodes");
      REIS(2, ref_cell_n(ref_grid_tri(iso_grid)), "one tri");
      REIS(0, ref_cell_n(ref_grid_edg(iso_grid)), "no edg");
    }
    each_ref_node_valid_node(ref_grid_node(iso_grid), node) {
      RWDS(0.0, out[node], -1, "interp");
    }
    ref_free(out);
    ref_grid_free(iso_grid);
    ref_free(field);
    ref_grid_free(ref_grid);
  }

  if (!ref_mpi_para(ref_mpi)) { /* distance tri */
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_DBL *field, *distance;
    REF_INT node;
    REF_DBL offset = 0.5;

    RSS(ref_fixture_twod_brick_grid(&ref_grid, ref_mpi, 4), "tri");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc(field, ref_node_max(ref_node), REF_DBL);
    ref_malloc(distance, ref_node_max(ref_node), REF_DBL);
    each_ref_node_valid_node(ref_node, node) {
      field[node] = 2.0 * (ref_node_xyz(ref_node, 0, node) - offset);
    }
    RSS(ref_iso_signed_distance(ref_grid, field, distance), "iso dist");
    each_ref_node_valid_node(ref_node, node) {
      RWDS(0.5 * field[node], distance[node], -1, "dist");
    }

    ref_free(distance);
    ref_free(field);
    ref_grid_free(ref_grid);
  }

  if (!ref_mpi_para(ref_mpi)) { /* distance tri */
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_DBL *field, *distance;
    REF_INT node;
    REF_DBL offset = 0.5;

    RSS(ref_fixture_tet_brick_grid(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc(field, ref_node_max(ref_node), REF_DBL);
    ref_malloc(distance, ref_node_max(ref_node), REF_DBL);
    each_ref_node_valid_node(ref_node, node) {
      field[node] = 2.0 * (ref_node_xyz(ref_node, 0, node) - offset);
    }
    RSS(ref_iso_signed_distance(ref_grid, field, distance), "iso dist");
    each_ref_node_valid_node(ref_node, node) {
      RWDS(0.5 * field[node], distance[node], -1, "dist");
    }

    ref_free(distance);
    ref_free(field);
    ref_grid_free(ref_grid);
  }

  { /* seg-tri, corner */
    REF_DBL triangle0[3], triangle1[3], triangle2[3];
    REF_DBL segment0[3], segment1[3];
    REF_DBL tuvw[4];

    triangle0[0] = 0.0;
    triangle0[1] = 0.0;
    triangle0[2] = 0.0;

    triangle1[0] = 1.0;
    triangle1[1] = 0.0;
    triangle1[2] = 0.0;

    triangle2[0] = 0.0;
    triangle2[1] = 1.0;
    triangle2[2] = 0.0;

    segment0[0] = 0.0;
    segment0[1] = 0.0;
    segment0[2] = 0.0;

    segment1[0] = 0.0;
    segment1[1] = 0.0;
    segment1[2] = 1.0;

    RSS(ref_iso_triangle_segment(triangle0, triangle1, triangle2, segment0,
                                 segment1, tuvw),
        "tri-seg");
    RWDS(0.0, tuvw[0], -1.0, "t");
    RWDS(1.0, tuvw[1], -1.0, "u");
    RWDS(0.0, tuvw[2], -1.0, "v");
    RWDS(0.0, tuvw[3], -1.0, "w");
  }

  { /* seg-tri, parallel */
    REF_DBL triangle0[3], triangle1[3], triangle2[3];
    REF_DBL segment0[3], segment1[3];
    REF_DBL tuvw[4];

    triangle0[0] = 0.0;
    triangle0[1] = 0.0;
    triangle0[2] = 0.0;

    triangle1[0] = 1.0;
    triangle1[1] = 0.0;
    triangle1[2] = 0.0;

    triangle2[0] = 0.0;
    triangle2[1] = 1.0;
    triangle2[2] = 0.0;

    segment0[0] = 0.0;
    segment0[1] = 0.0;
    segment0[2] = 0.0;

    segment1[0] = 1.0;
    segment1[1] = 0.0;
    segment1[2] = 0.0;

    REIS(REF_DIV_ZERO,
         ref_iso_triangle_segment(triangle0, triangle1, triangle2, segment0,
                                  segment1, tuvw),
         "tri-seg");
  }

  { /* seg-tri, collapsed seg */
    REF_DBL triangle0[3], triangle1[3], triangle2[3];
    REF_DBL segment0[3], segment1[3];
    REF_DBL tuvw[4];

    triangle0[0] = 0.0;
    triangle0[1] = 0.0;
    triangle0[2] = 0.0;

    triangle1[0] = 1.0;
    triangle1[1] = 0.0;
    triangle1[2] = 0.0;

    triangle2[0] = 0.0;
    triangle2[1] = 1.0;
    triangle2[2] = 0.0;

    segment0[0] = 0.0;
    segment0[1] = 0.0;
    segment0[2] = 0.0;

    segment1[0] = 0.0;
    segment1[1] = 0.0;
    segment1[2] = 0.0;

    REIS(REF_DIV_ZERO,
         ref_iso_triangle_segment(triangle0, triangle1, triangle2, segment0,
                                  segment1, tuvw),
         "tri-seg");
  }

  { /* seg-tri, collapsed tri */
    REF_DBL triangle0[3], triangle1[3], triangle2[3];
    REF_DBL segment0[3], segment1[3];
    REF_DBL tuvw[4];

    triangle0[0] = 0.0;
    triangle0[1] = 0.0;
    triangle0[2] = 0.0;

    triangle1[0] = 1.0;
    triangle1[1] = 0.0;
    triangle1[2] = 0.0;

    triangle2[0] = 0.0;
    triangle2[1] = 0.0;
    triangle2[2] = 0.0;

    segment0[0] = 0.0;
    segment0[1] = 0.0;
    segment0[2] = 0.0;

    segment1[0] = 0.0;
    segment1[1] = 0.0;
    segment1[2] = 1.0;

    REIS(REF_DIV_ZERO,
         ref_iso_triangle_segment(triangle0, triangle1, triangle2, segment0,
                                  segment1, tuvw),
         "tri-seg");
  }

  { /* seg-seg, cross */
    REF_DBL candidate0[3], candidate1[3];
    REF_DBL segment0[3], segment1[3];
    REF_DBL tt[2];

    candidate0[0] = 0.0;
    candidate0[1] = 0.0;
    candidate0[2] = 0.0;

    candidate1[0] = 1.0;
    candidate1[1] = 0.0;
    candidate1[2] = 0.0;

    segment0[0] = 0.0;
    segment0[1] = -1.0;
    segment0[2] = 0.0;

    segment1[0] = 0.0;
    segment1[1] = 1.0;
    segment1[2] = 0.0;

    REIS(
        REF_SUCCESS,
        ref_iso_segment_segment(candidate0, candidate1, segment0, segment1, tt),
        "seg-seg");
    RWDS(0.5, tt[0], -1.0, "segment");
    RWDS(0.0, tt[1], -1.0, "candidate");
  }

  { /* cast tet */
    REF_GRID ref_grid, iso_grid;
    REF_NODE ref_node;
    REF_INT ldim = 1;
    REF_DBL *field, *iso_field;
    REF_INT node;
    REF_DBL segment0[3], segment1[3];

    RSS(ref_fixture_tet_grid(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc(field, ldim * ref_node_max(ref_node), REF_DBL);
    each_ref_node_valid_node(ref_node, node) {
      field[node] = ref_node_xyz(ref_node, 0, node);
    }
    segment0[0] = -1.0;
    segment0[1] = 0.2;
    segment0[2] = 0.2;
    segment1[0] = 2.0;
    segment1[1] = 0.2;
    segment1[2] = 0.2;

    RSS(ref_iso_cast(&iso_grid, &iso_field, ref_grid, field, ldim, segment0,
                     segment1),
        "cast");
    if (!ref_mpi_para(ref_mpi)) {
      REIS(2, ref_node_n(ref_grid_node(iso_grid)), "two nodes");
      REIS(0, ref_cell_n(ref_grid_tri(iso_grid)), "zero tri");
      REIS(1, ref_cell_n(ref_grid_edg(iso_grid)), "one edg");
    }
    ref_free(iso_field);
    ref_grid_free(iso_grid);
    ref_free(field);
    ref_grid_free(ref_grid);
  }

  { /* cast tet brick */
    REF_GRID ref_grid, iso_grid;
    REF_NODE ref_node;
    REF_INT ldim = 1;
    REF_DBL *field, *iso_field;
    REF_INT node;
    REF_DBL segment0[3], segment1[3];
    REF_BOOL debug = REF_FALSE;

    RSS(ref_fixture_tet_brick_grid(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc(field, ldim * ref_node_max(ref_node), REF_DBL);
    each_ref_node_valid_node(ref_node, node) {
      field[node] = ref_node_xyz(ref_node, 0, node);
    }
    segment0[0] = -1.0;
    segment0[1] = 0.2;
    segment0[2] = 0.2;
    segment1[0] = 2.0;
    segment1[1] = 0.2;
    segment1[2] = 0.2;

    RSS(ref_iso_cast(&iso_grid, &iso_field, ref_grid, field, ldim, segment0,
                     segment1),
        "cast");
    if (!ref_mpi_para(ref_mpi)) {
      REIS(10, ref_node_n(ref_grid_node(iso_grid)), "ten nodes");
      REIS(0, ref_cell_n(ref_grid_tri(iso_grid)), "zero tri");
      REIS(9, ref_cell_n(ref_grid_edg(iso_grid)), "one edg");
    }
    if (debug) {
      ref_gather_scalar_by_extension(iso_grid, ldim, iso_field, NULL,
                                     "iso-edge.tec");
      ref_gather_scalar_by_extension(ref_grid, ldim, field, NULL,
                                     "iso-orig.tec");
    }
    ref_free(iso_field);
    ref_grid_free(iso_grid);
    ref_free(field);
    ref_grid_free(ref_grid);
  }

  { /* cast tri */
    REF_GRID ref_grid, iso_grid;
    REF_NODE ref_node;
    REF_INT ldim = 1;
    REF_DBL *field, *iso_field;
    REF_INT node;
    REF_DBL segment0[3], segment1[3];

    RSS(ref_fixture_tri_grid(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc(field, ldim * ref_node_max(ref_node), REF_DBL);
    each_ref_node_valid_node(ref_node, node) {
      field[node] = ref_node_xyz(ref_node, 0, node);
    }
    segment0[0] = -1.0;
    segment0[1] = 0.2;
    segment0[2] = 0.0;
    segment1[0] = 2.0;
    segment1[1] = 0.2;
    segment1[2] = 0.0;

    RSS(ref_iso_cast(&iso_grid, &iso_field, ref_grid, field, ldim, segment0,
                     segment1),
        "cast");
    if (!ref_mpi_para(ref_mpi)) {
      REIS(REF_TRUE, ref_grid_twod(iso_grid), "twod");
      REIS(2, ref_node_n(ref_grid_node(iso_grid)), "two nodes");
      REIS(0, ref_cell_n(ref_grid_tri(iso_grid)), "zero tri");
      REIS(1, ref_cell_n(ref_grid_edg(iso_grid)), "one edg");
    }
    ref_free(iso_field);
    ref_grid_free(iso_grid);
    ref_free(field);
    ref_grid_free(ref_grid);
  }

  { /* boom tet brick */
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_INT ldim = 1;
    REF_DBL *field;
    REF_INT node;
    REF_BOOL debug = REF_FALSE;
    FILE *file;
    char filename[] = "ref_iso_boom.tec";
    REF_DBL center[3] = {0.5, 0.5, 0.5};
    REF_DBL aoa, phi, h;

    RSS(ref_fixture_tet_brick_grid(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc(field, ldim * ref_node_max(ref_node), REF_DBL);
    each_ref_node_valid_node(ref_node, node) {
      field[node] = ref_node_xyz(ref_node, 0, node);
    }

    file = NULL;
    if (ref_mpi_once(ref_mpi)) {
      RSS(ref_iso_boom_header(&file, ldim, NULL, filename), "header");
    }
    aoa = 10.0;
    phi = 0.0;
    h = 0.1;
    RSS(ref_iso_boom_zone(file, ref_grid, field, ldim, center, aoa, phi, h),
        " zone 00");
    aoa = 5.0;
    phi = 20.0;
    h = 0.1;
    RSS(ref_iso_boom_zone(file, ref_grid, field, ldim, center, aoa, phi, h),
        " zone 20");
    aoa = 0.0;
    phi = 40.0;
    h = 0.1;
    RSS(ref_iso_boom_zone(file, ref_grid, field, ldim, center, aoa, phi, h),
        " zone 40");

    ref_free(field);
    ref_grid_free(ref_grid);
    if (ref_mpi_once(ref_mpi)) {
      fclose(file);
      if (!debug) REIS(0, remove(filename), "test clean up");
    }
  }

  { /* segment, case values */
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_DBL center[3] = {0, 0, 1.58087742328643799};
    REF_DBL aoa = 2.3;
    REF_DBL phi = 0.0;
    REF_DBL h = 31.8;
    REF_DBL segment0[3], segment1[3];
    REF_INT node;
    REF_GLOB global;

    RSS(ref_grid_create(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    global = 0;
    RSS(ref_node_add(ref_node, global, &node), "first add");
    ref_node_xyz(ref_node, 0, node) = 0.0;
    ref_node_xyz(ref_node, 1, node) = 0.0;
    ref_node_xyz(ref_node, 2, node) = 0.0;
    global = 1;
    RSS(ref_node_add(ref_node, global, &node), "second add");
    ref_node_xyz(ref_node, 0, node) = 1.0;
    ref_node_xyz(ref_node, 1, node) = 0.0;
    ref_node_xyz(ref_node, 2, node) = 0.0;

    RSS(ref_iso_segment(ref_grid, center, aoa, phi, h, segment0, segment1),
        "seg");

    ref_grid_free(ref_grid);
  }

  { /* segment, phi 0 */
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_DBL center[3] = {0, 0, 0};
    REF_DBL aoa = 0;
    REF_DBL phi = 0.0;
    REF_DBL h = 15;
    REF_DBL segment0[3], segment1[3];
    REF_INT node;
    REF_GLOB global;

    RSS(ref_grid_create(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    global = 0;
    RSS(ref_node_add(ref_node, global, &node), "first add");
    ref_node_xyz(ref_node, 0, node) = 0.0;
    ref_node_xyz(ref_node, 1, node) = 0.0;
    ref_node_xyz(ref_node, 2, node) = 0.0;
    global = 1;
    RSS(ref_node_add(ref_node, global, &node), "second add");
    ref_node_xyz(ref_node, 0, node) = 1.0;
    ref_node_xyz(ref_node, 1, node) = 0.0;
    ref_node_xyz(ref_node, 2, node) = 0.0;

    RSS(ref_iso_segment(ref_grid, center, aoa, phi, h, segment0, segment1),
        "seg");

    RWDS(0.0, segment0[1], -1.0, "y");
    RWDS(-15.0, segment0[2], -1.0, "z");
    RWDS(0.0, segment1[1], -1.0, "y");
    RWDS(-15.0, segment1[2], -1.0, "z");

    ref_grid_free(ref_grid);
  }

  { /* segment, phi -30 */
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_DBL center[3] = {0, 0, 0};
    REF_DBL aoa = 0;
    REF_DBL phi = -30.0;
    REF_DBL h = 20;
    REF_DBL segment0[3], segment1[3];
    REF_INT node;
    REF_GLOB global;

    RSS(ref_grid_create(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    global = 0;
    RSS(ref_node_add(ref_node, global, &node), "first add");
    ref_node_xyz(ref_node, 0, node) = 0.0;
    ref_node_xyz(ref_node, 1, node) = 0.0;
    ref_node_xyz(ref_node, 2, node) = 0.0;
    global = 1;
    RSS(ref_node_add(ref_node, global, &node), "second add");
    ref_node_xyz(ref_node, 0, node) = 1.0;
    ref_node_xyz(ref_node, 1, node) = 0.0;
    ref_node_xyz(ref_node, 2, node) = 0.0;

    RSS(ref_iso_segment(ref_grid, center, aoa, phi, h, segment0, segment1),
        "seg");

    {
      REF_DBL z;
      z = 10.0 * sqrt(3.0);
      RWDS(-10, segment0[1], -1.0, "y");
      RWDS(-z, segment0[2], -1.0, "z");
      RWDS(-10, segment1[1], -1.0, "y");
      RWDS(-z, segment1[2], -1.0, "z");
    }

    ref_grid_free(ref_grid);
  }

  { /* segment, phi 45 */
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_DBL center[3] = {0, 0, 0};
    REF_DBL aoa = 0;
    REF_DBL phi = 45.0;
    REF_DBL h = 20;
    REF_DBL segment0[3], segment1[3];
    REF_INT node;
    REF_GLOB global;

    RSS(ref_grid_create(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    global = 0;
    RSS(ref_node_add(ref_node, global, &node), "first add");
    ref_node_xyz(ref_node, 0, node) = 0.0;
    ref_node_xyz(ref_node, 1, node) = 0.0;
    ref_node_xyz(ref_node, 2, node) = 0.0;
    global = 1;
    RSS(ref_node_add(ref_node, global, &node), "second add");
    ref_node_xyz(ref_node, 0, node) = 1.0;
    ref_node_xyz(ref_node, 1, node) = 0.0;
    ref_node_xyz(ref_node, 2, node) = 0.0;

    RSS(ref_iso_segment(ref_grid, center, aoa, phi, h, segment0, segment1),
        "seg");

    {
      REF_DBL r;
      r = h / sqrt(2.0);
      RWDS(r, segment0[1], -1.0, "y");
      RWDS(-r, segment0[2], -1.0, "z");
      RWDS(r, segment1[1], -1.0, "y");
      RWDS(-r, segment1[2], -1.0, "z");
    }

    ref_grid_free(ref_grid);
  }

  { /* segment, aoa 45 */
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_DBL center[3] = {0, 0, 0};
    REF_DBL aoa = 45;
    REF_DBL phi = 0.0;
    REF_DBL h = 0;
    REF_DBL segment0[3], segment1[3];
    REF_INT node;
    REF_GLOB global;

    RSS(ref_grid_create(&ref_grid, ref_mpi), "tri");
    ref_node = ref_grid_node(ref_grid);
    global = 0;
    RSS(ref_node_add(ref_node, global, &node), "first add");
    ref_node_xyz(ref_node, 0, node) = -1.0;
    ref_node_xyz(ref_node, 1, node) = 0.0;
    ref_node_xyz(ref_node, 2, node) = 0.0;
    global = 1;
    RSS(ref_node_add(ref_node, global, &node), "second add");
    ref_node_xyz(ref_node, 0, node) = 1.0;
    ref_node_xyz(ref_node, 1, node) = 0.0;
    ref_node_xyz(ref_node, 2, node) = 0.0;

    RSS(ref_iso_segment(ref_grid, center, aoa, phi, h, segment0, segment1),
        "seg");

    {
      RWDS(0, segment0[1], -1.0, "y");
      RWDS(segment0[0], segment0[2], -1.0, "z");
      RWDS(0, segment1[1], -1.0, "y");
      RWDS(segment1[0], segment1[2], -1.0, "z");
    }

    ref_grid_free(ref_grid);
  }

  RXS(ref_args_find(argc, argv, "--hair", &pos), REF_NOT_FOUND, "arg search");
  if (REF_EMPTY != pos) {
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_DICT ref_dict_bcs;
    REF_DBL *distance, *uplus;
    REF_INT node;
    REF_DBL spalding_yplus = 0.01;
    REF_DBL yplus_of_one;
    REF_BOOL verbose = REF_FALSE;
    REF_DBL *metric;
    if (argc > 2) {
      printf("import %s\n", argv[2]);
      RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[2]), "import");
      ref_mpi_stopwatch_stop(ref_mpi, "import");
    } else {
      RSS(ref_fixture_twod_brick_grid(&ref_grid, ref_mpi, 11), "set up tri");
      ref_mpi_stopwatch_stop(ref_mpi, "brick");
    }
    ref_node = ref_grid_node(ref_grid);
    RSS(ref_dict_create(&ref_dict_bcs), "make dict");
    { /* solid-wall floor */
      REF_INT id = 1;
      REF_INT type = 4000;
      RSS(ref_dict_store(ref_dict_bcs, id, type), "store");
    }
    ref_malloc(distance, ref_node_max(ref_node), REF_DBL);
    ref_malloc(uplus, ref_node_max(ref_node), REF_DBL);
    RSS(ref_phys_wall_distance(ref_grid, ref_dict_bcs, distance), "wall dist");
    ref_mpi_stopwatch_stop(ref_mpi, "wall distance");
    each_ref_node_valid_node(ref_node, node) {
      REF_DBL yplus;
      RAB(ref_math_divisible(distance[node], spalding_yplus),
          "\nare viscous boundarys set with --viscous-tags or --fun3d-mapbc?"
          "\nwall distance not divisible by y+=1",
          {
            printf("distance %e yplus=1 %e\n", distance[node], spalding_yplus);
          });
      yplus_of_one =
          spalding_yplus * sqrt(1.0 + ref_node_xyz(ref_node, 0, node));
      yplus = distance[node] / yplus_of_one;
      RSS(ref_phys_spalding_uplus(yplus, &(uplus[node])), "uplus");
    }
    RSS(ref_gather_scalar_by_extension(ref_grid, 1, uplus, NULL,
                                       "ref_iso_test_uplus.plt"),
        "dump uplus");
    ref_malloc(metric, 6 * ref_node_max(ref_node), REF_DBL);
    {
      REF_RECON_RECONSTRUCTION reconstruction = REF_RECON_L2PROJECTION;
      REF_INT p = 2;
      REF_DBL gradation = -1.0;
      REF_DBL complexity = 1000;
      RSS(ref_metric_lp_mixed(metric, ref_grid, uplus, reconstruction, p,
                              gradation, complexity),
          "lp norm");
    }
    {
      REF_CELL ref_cell = ref_grid_edg(ref_grid);
      REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];
      REF_INT bc;
      REF_DBL *implied_metric;
      ref_malloc(implied_metric, 6 * ref_node_max(ref_grid_node(ref_grid)),
                 REF_DBL);
      RSS(ref_metric_imply_from(implied_metric, ref_grid), "imply");
      ref_mpi_stopwatch_stop(ref_mpi, "imply metric");
      each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
        bc = REF_EMPTY;
        RXS(ref_dict_value(ref_dict_bcs, nodes[ref_cell_id_index(ref_cell)],
                           &bc),
            REF_NOT_FOUND, "bc");
        if (ref_phys_wall_distance_bc(bc)) {
          REF_DBL normal[3];
          REF_DBL ratio, h;
          REF_DBL step = -5.0;
          REF_DBL segment0[3], segment1[3];
          REF_GRID iso_grid;
          REF_NODE iso_node;
          REF_INT ldim = 1;
          REF_DBL *iso_uplus, *iso_yplus;
          REF_DBL yplus_norm;
          RSS(ref_node_seg_normal(ref_node, nodes, normal), "seg normal");
          /* average node imply_metric/ */
          ratio =
              ref_matrix_sqrt_vt_m_v(&(implied_metric[6 * nodes[0]]), normal);
          RAS(ref_math_divisible(1.0, ratio), "invert ratio");
          h = 1.0 / ratio;
          segment0[0] = 0.5 * (ref_node_xyz(ref_node, 0, nodes[0]) +
                               ref_node_xyz(ref_node, 0, nodes[1]));
          segment0[1] = 0.5 * (ref_node_xyz(ref_node, 1, nodes[0]) +
                               ref_node_xyz(ref_node, 1, nodes[1]));
          segment0[2] = 0.5 * (ref_node_xyz(ref_node, 2, nodes[0]) +
                               ref_node_xyz(ref_node, 2, nodes[1]));
          segment1[0] = segment0[0] + step * h * normal[0];
          segment1[1] = segment0[1] + step * h * normal[1];
          segment1[2] = segment0[2] + step * h * normal[2];
          if (verbose) {
            printf("seg0 %f %f %f\n", segment0[0], segment0[1], segment0[2]);
            printf("seg1 %f %f %f\n", segment1[0], segment1[1], segment1[2]);
          }
          RSS(ref_iso_cast(&iso_grid, &iso_uplus, ref_grid, uplus, ldim,
                           segment0, segment1),
              "cast");
          iso_node = ref_grid_node(iso_grid);
          ref_malloc(iso_yplus, ref_node_max(iso_node), REF_DBL);
          yplus_norm = 0.0;
          each_ref_node_valid_node(iso_node, node) {
            REF_DBL dist;
            RSS(ref_phys_spalding_yplus(iso_uplus[node], &(iso_yplus[node])),
                "yplus");
            dist = sqrt(pow(ref_node_xyz(iso_node, 0, node) - segment0[0], 2) +
                        pow(ref_node_xyz(iso_node, 1, node) - segment0[1], 2) +
                        pow(ref_node_xyz(iso_node, 2, node) - segment0[2], 2));
            if (ref_math_divisible(dist, iso_yplus[node]))
              yplus_norm = dist / iso_yplus[node];
            ratio = 0;
            if (ref_math_divisible(iso_yplus[node],
                                   ref_node_xyz(iso_node, 1, node))) {
              yplus_of_one =
                  spalding_yplus * sqrt(1.0 + ref_node_xyz(ref_node, 0, node));
              ratio = yplus_of_one * iso_yplus[node] /
                      ref_node_xyz(iso_node, 1, node);
            }
            if (verbose)
              printf("ratio %f yplus %f x %f y %f\n", ratio, iso_yplus[node],
                     ref_node_xyz(iso_node, 0, node),
                     ref_node_xyz(iso_node, 1, node));
          }
          ref_free(iso_yplus);
          ref_free(iso_uplus);
          RSS(ref_grid_free(iso_grid), "free grid");
          if (yplus_norm > 1e-100) {
            REF_DBL yplus_target = 10.0;
            REF_DBL diagonal_system[12];
            REF_DBL hn, ht;
            REF_INT i;
            hn = yplus_target * yplus_norm;
            for (i = 0; i < 12; i++) diagonal_system[i] = 0;
            ref_matrix_show_diag_sys(diagonal_system);
            for (i = 0; i < 3; i++)
              ref_matrix_vec(diagonal_system, i, 0) = normal[i];
            ref_matrix_eig(diagonal_system, 0) = 1.0 / hn / hn;
            ref_matrix_show_diag_sys(diagonal_system);
            for (i = 0; i < 3; i++)
              ref_matrix_vec(diagonal_system, i, 1) =
                  ref_node_xyz(ref_node, i, nodes[1]) -
                  ref_node_xyz(ref_node, i, nodes[0]);
            ht = sqrt(ref_math_dot(ref_matrix_vec_ptr(diagonal_system, 1),
                                   ref_matrix_vec_ptr(diagonal_system, 1)));
            RSS(ref_math_normalize(ref_matrix_vec_ptr(diagonal_system, 1)),
                "norm surf segment");
            ref_matrix_eig(diagonal_system, 1) = 1.0 / ht / ht;
            ref_matrix_show_diag_sys(diagonal_system);
            ref_matrix_vec(diagonal_system, 0, 2) = 0.0;
            ref_matrix_vec(diagonal_system, 1, 2) = 0.0;
            ref_matrix_vec(diagonal_system, 2, 2) = 1.0;
            ref_matrix_eig(diagonal_system, 2) = 1.0;
            ref_matrix_show_diag_sys(diagonal_system);

            RSS(ref_matrix_form_m(diagonal_system, &(metric[6 * nodes[0]])),
                "form 0");
            RSS(ref_matrix_form_m(diagonal_system, &(metric[6 * nodes[1]])),
                "form 1");
          }
        }
      }
      ref_free(implied_metric);
    }
    RSS(ref_metric_to_node(metric, ref_node), "metric to node");
    RSS(ref_gather_metric(ref_grid, "ref_iso_test_metric.solb"),
        "gather metric");
    RSS(ref_gather_by_extension(ref_grid, "ref_iso_test.meshb"),
        "gather meshb");

    ref_free(metric);

    ref_free(uplus);
    ref_free(distance);
    ref_dict_free(ref_dict_bcs);
    ref_grid_free(ref_grid);
  }

  RSS(ref_mpi_free(ref_mpi), "mpi free");
  RSS(ref_mpi_stop(), "stop");
  return 0;
}
