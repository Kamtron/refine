
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

#include "ref_shard.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ref_adj.h"
#include "ref_cell.h"
#include "ref_dict.h"
#include "ref_edge.h"
#include "ref_export.h"
#include "ref_fixture.h"
#include "ref_gather.h"
#include "ref_geom.h"
#include "ref_grid.h"
#include "ref_import.h"
#include "ref_list.h"
#include "ref_math.h"
#include "ref_matrix.h"
#include "ref_migrate.h"
#include "ref_mpi.h"
#include "ref_node.h"
#include "ref_part.h"
#include "ref_sort.h"
#include "ref_subdiv.h"
#include "ref_swap.h"
#include "ref_validation.h"

static REF_STATUS set_up_hex_for_shard(REF_SHARD *ref_shard_ptr,
                                       REF_MPI ref_mpi) {
  REF_GRID ref_grid;

  RSS(ref_fixture_hex_grid(&ref_grid, ref_mpi), "fixture hex");

  RSS(ref_shard_create(ref_shard_ptr, ref_grid), "create");

  return REF_SUCCESS;
}

static REF_STATUS tear_down(REF_SHARD ref_shard) {
  REF_GRID ref_grid;

  ref_grid = ref_shard_grid(ref_shard);

  RSS(ref_shard_free(ref_shard), "free");

  RSS(ref_grid_free(ref_grid), "free");

  return REF_SUCCESS;
}

int main(int argc, char *argv[]) {
  REF_LONG nhex;
  REF_MPI ref_mpi;
  RSS(ref_mpi_start(argc, argv), "start");
  RSS(ref_mpi_create(&ref_mpi), "make mpi");

  if (1 < argc) {
    REF_GRID ref_grid;
    char file[] = "ref_shard_test.b8.ugrid";
    if (ref_mpi_para(ref_mpi)) {
      RSS(ref_part_by_extension(&ref_grid, ref_mpi, argv[1]), "import");
    } else {
      RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[1]), "import");
    }
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "import");
    RSS(ref_cell_ncell(ref_grid_hex(ref_grid), ref_grid_node(ref_grid), &nhex),
        "nhex");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "nhex");
    if (0 < nhex && 2 < argc) {
      REF_SHARD ref_shard;
      REF_CELL ref_cell;
      REF_NODE ref_node = ref_grid_node(ref_grid);
      REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];
      REF_INT faceid;
      REF_INT open_node0, open_node1;
      REF_INT face_marks, hex_marks;
      REF_DBL dot;

      if (ref_mpi_para(ref_grid_mpi(ref_grid))) {
        RSS(ref_mpi_free(ref_mpi), "mpi free");
        RSS(ref_mpi_stop(), "stop");
        THROW("hex shard not parallel");
      }

      RSS(ref_shard_create(&ref_shard, ref_grid), "create");
      ref_cell = ref_grid_qua(ref_grid);
      each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
        faceid = nodes[4];
        if (atoi(argv[2]) == faceid ||
            (-1 == atoi(argv[2]) && (1 == faceid || 2 == faceid))) {
          RSS(ref_face_open_node(&ref_node_xyz(ref_node, 0, nodes[0]),
                                 &ref_node_xyz(ref_node, 0, nodes[1]),
                                 &ref_node_xyz(ref_node, 0, nodes[2]),
                                 &ref_node_xyz(ref_node, 0, nodes[3]),
                                 &open_node0, &dot),
              "find open");
          if ((-1 != atoi(argv[2])) || (dot > 0.99)) {
            printf("dot %f\n", dot);
            open_node1 = open_node0 + 2;
            if (open_node1 >= 4) open_node1 -= 4;
            RSS(ref_shard_mark_to_split(ref_shard, nodes[open_node0],
                                        nodes[open_node1]),
                "mark");
          }
        }
      }
      RSS(ref_shard_mark_n(ref_shard, &face_marks, &hex_marks), "count marks");
      printf("marked faces %d hexes %d\n", face_marks, hex_marks);
      RSS(ref_shard_mark_relax(ref_shard), "relax");
      RSS(ref_shard_mark_n(ref_shard, &face_marks, &hex_marks), "count marks");
      printf("relaxed faces %d hexes %d\n", face_marks, hex_marks);
      RSS(ref_shard_split(ref_shard), "split hex to prism");
      RSS(ref_shard_free(ref_shard), "free");
      ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "split hex");
      RSS(ref_gather_scalar_surf_tec(ref_grid, 0, NULL, NULL,
                                     "ref_shard_test_surf.tec"),
          "gather surf tec");
      ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "surf");
    } else {
      REF_INT nlayer = 0;
      REF_INT pos;
      REF_DICT of_faceid;
      if (argc > 2) nlayer = atoi(argv[2]);
      if (ref_mpi_once(ref_mpi)) printf("keeping %d layers of prism\n", nlayer);
      RSS(ref_dict_create(&of_faceid), "create dict");

      for (pos = 3; pos < argc; pos++) {
        REF_INT faceid;
        faceid = atoi(argv[pos]);
        RSS(ref_dict_store(of_faceid, faceid, REF_EMPTY), "store");
        if (ref_mpi_once(ref_mpi)) printf("adjecent to faceid %d\n", faceid);
      }
      RSS(ref_shard_prism_into_tet(ref_grid, nlayer, of_faceid), "shard prism");
      RSS(ref_dict_free(of_faceid), "free dict");
      ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "shard pri");
    }
    if (ref_mpi_para(ref_grid_mpi(ref_grid))) {
      RSS(ref_gather_by_extension(ref_grid, file), "export");
    } else {
      RSS(ref_export_by_extension(ref_grid, file), "export");
    }
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "export");
    RSS(ref_grid_free(ref_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "mpi free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  { /* mark */
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard, ref_mpi), "set up");

    REIS(0, ref_shard_mark(ref_shard, 1), "init mark");
    REIS(0, ref_shard_mark(ref_shard, 3), "init mark");

    RSS(ref_shard_mark_to_split(ref_shard, 1, 6), "mark face for 1-6");

    REIS(2, ref_shard_mark(ref_shard, 1), "split");
    REIS(0, ref_shard_mark(ref_shard, 3), "modified");

    RSS(tear_down(ref_shard), "tear down");
  }

  { /* find mark */
    REF_SHARD ref_shard;
    REF_BOOL marked;

    RSS(set_up_hex_for_shard(&ref_shard, ref_mpi), "set up");

    RSS(ref_shard_mark_to_split(ref_shard, 1, 6), "mark face for 1-6");

    RSS(ref_shard_marked(ref_shard, 2, 5, &marked), "is edge marked?");
    RAS(!marked, "pair 2-5 not marked");

    RSS(ref_shard_marked(ref_shard, 1, 6, &marked), "is edge marked?");
    RAS(marked, "pair 1-6 marked");

    RSS(ref_shard_marked(ref_shard, 6, 1, &marked), "is edge marked?");
    RAS(marked, "pair 6-1 marked");

    RSS(tear_down(ref_shard), "tear down");
  }

  { /* mark and relax 1-6 */
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard, ref_mpi), "set up");

    RSS(ref_shard_mark_to_split(ref_shard, 1, 6), "mark face for 1-6");

    REIS(0, ref_shard_mark(ref_shard, 0), "no yet");
    RSS(ref_shard_mark_relax(ref_shard), "relax");
    REIS(2, ref_shard_mark(ref_shard, 0), "yet");

    RSS(tear_down(ref_shard), "tear down");
  }

  { /* mark and relax 0-7 */
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard, ref_mpi), "set up");

    RSS(ref_shard_mark_to_split(ref_shard, 0, 7), "mark face for 1-6");

    RES(0, ref_shard_mark(ref_shard, 1), "no yet");
    RSS(ref_shard_mark_relax(ref_shard), "relax");
    RES(2, ref_shard_mark(ref_shard, 1), "yet");

    RSS(tear_down(ref_shard), "tear down");
  }

  { /* relax nothing*/
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard, ref_mpi), "set up");

    RSS(ref_shard_mark_relax(ref_shard), "relax");

    RES(0, ref_shard_mark(ref_shard, 0), "marked");
    RES(0, ref_shard_mark(ref_shard, 1), "marked");
    RES(0, ref_shard_mark(ref_shard, 2), "marked");
    RES(0, ref_shard_mark(ref_shard, 3), "marked");
    RES(0, ref_shard_mark(ref_shard, 4), "marked");
    RES(0, ref_shard_mark(ref_shard, 5), "marked");

    RSS(tear_down(ref_shard), "tear down");
  }

  { /* mark cell edge 0 */
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard, ref_mpi), "set up");

    RSS(ref_shard_mark_cell_edge_split(ref_shard, 0, 0), "mark edge 0");

    REIS(2, ref_shard_mark(ref_shard, 0), "face 0");
    REIS(2, ref_shard_mark(ref_shard, 1), "face 1");

    RSS(tear_down(ref_shard), "tear down");
  }

  { /* mark cell edge 11 */
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard, ref_mpi), "set up");

    RSS(ref_shard_mark_cell_edge_split(ref_shard, 0, 11), "mark edge 11");

    REIS(2, ref_shard_mark(ref_shard, 0), "face 0");
    REIS(2, ref_shard_mark(ref_shard, 1), "face 1");

    RSS(tear_down(ref_shard), "tear down");
  }

  { /* mark cell edge 5 */
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard, ref_mpi), "set up");

    RSS(ref_shard_mark_cell_edge_split(ref_shard, 0, 5), "mark edge 5");

    REIS(3, ref_shard_mark(ref_shard, 0), "face 0");
    REIS(3, ref_shard_mark(ref_shard, 1), "face 1");

    RSS(tear_down(ref_shard), "tear down");
  }

  { /* mark cell edge 8 */
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard, ref_mpi), "set up");

    RSS(ref_shard_mark_cell_edge_split(ref_shard, 0, 8), "mark edge 8");

    REIS(3, ref_shard_mark(ref_shard, 0), "face 0");
    REIS(3, ref_shard_mark(ref_shard, 1), "face 1");

    RSS(tear_down(ref_shard), "tear down");
  }

  { /* split on edge 0 */
    REF_SHARD ref_shard;
    REF_GRID ref_grid;
    RSS(set_up_hex_for_shard(&ref_shard, ref_mpi), "set up");
    ref_grid = ref_shard_grid(ref_shard);

    RSS(ref_shard_mark_cell_edge_split(ref_shard, 0, 0), "mark edge 0");

    RSS(ref_shard_mark_relax(ref_shard), "relax");
    RSS(ref_shard_split(ref_shard), "split");

    REIS(0, ref_cell_n(ref_grid_hex(ref_grid)), "remove hex");
    REIS(2, ref_cell_n(ref_grid_pri(ref_grid)), "add two pri");

    REIS(4, ref_cell_n(ref_grid_tri(ref_grid)), "add four tri");
    REIS(4, ref_cell_n(ref_grid_qua(ref_grid)), "keep four qua");

    RSS(tear_down(ref_shard), "tear down");
  }

  { /* split on edge 8 */
    REF_SHARD ref_shard;
    REF_GRID ref_grid;
    RSS(set_up_hex_for_shard(&ref_shard, ref_mpi), "set up");
    ref_grid = ref_shard_grid(ref_shard);

    RSS(ref_shard_mark_cell_edge_split(ref_shard, 0, 8), "mark edge 0");

    RSS(ref_shard_mark_relax(ref_shard), "relax");
    RSS(ref_shard_split(ref_shard), "split");

    REIS(0, ref_cell_n(ref_grid_hex(ref_grid)), "remove hex");
    REIS(2, ref_cell_n(ref_grid_pri(ref_grid)), "add two pri");

    REIS(4, ref_cell_n(ref_grid_tri(ref_grid)), "add two pri");
    REIS(4, ref_cell_n(ref_grid_qua(ref_grid)), "keep one qua");

    RSS(tear_down(ref_shard), "tear down");
  }

  if (!ref_mpi_para(ref_mpi)) { /* shard prism */

    REF_GRID ref_grid;

    RSS(ref_fixture_pri_grid(&ref_grid, ref_mpi), "set up");

    RSS(ref_shard_prism_into_tet(ref_grid, 0, NULL), "shard prism");

    REIS(0, ref_cell_n(ref_grid_pri(ref_grid)), "no more pri");
    REIS(3, ref_cell_n(ref_grid_tet(ref_grid)), "into 3 tets");

    RSS(ref_grid_free(ref_grid), "free");
  }

  if (!ref_mpi_para(ref_mpi)) { /* shard half stack */

    REF_GRID ref_grid;
    REF_INT cell, nodes[] = {0, 1, 2, 15};
    REF_DICT of_faceid;
    RSS(ref_fixture_pri_stack_grid(&ref_grid, ref_mpi), "set up");
    RSS(ref_cell_add(ref_grid_tri(ref_grid), nodes, &cell), "add one tri");

    RSS(ref_dict_create(&of_faceid), "create dict");
    RSS(ref_dict_store(of_faceid, 15, REF_EMPTY), "store");
    RSS(ref_shard_prism_into_tet(ref_grid, 2, of_faceid), "shard prism");
    RSS(ref_dict_free(of_faceid), "free dict");

    REIS(2, ref_cell_n(ref_grid_pri(ref_grid)), "no more pri");
    REIS(3, ref_cell_n(ref_grid_tet(ref_grid)), "into 3 tets");

    REIS(2, ref_cell_n(ref_grid_qua(ref_grid)), "no more qua");
    REIS(3, ref_cell_n(ref_grid_tri(ref_grid)), "into 9 tri");

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* shard pyr */

    REF_GRID ref_grid;

    RSS(ref_fixture_pyr_grid(&ref_grid, ref_mpi), "set up");

    RSS(ref_shard_prism_into_tet(ref_grid, 0, NULL), "shard prism");

    REIS(0, ref_cell_n(ref_grid_pyr(ref_grid)), "no more pri");
    REIS(2, ref_cell_n(ref_grid_tet(ref_grid)), "into 3 tets");

    REIS(0, ref_cell_n(ref_grid_qua(ref_grid)), "no more qua");
    REIS(6, ref_cell_n(ref_grid_tri(ref_grid)), "into 9 tri");

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* shard tri to tri */

    REF_GRID ref_grid;
    REF_CELL ref_cell;

    RSS(ref_fixture_tri_grid(&ref_grid, ref_mpi), "set up");

    RSS(ref_shard_extract_tri(ref_grid, &ref_cell), "shard to tri");

    REIS(ref_cell_n(ref_grid_tri(ref_grid)), ref_cell_n(ref_cell), "same ntri");

    RSS(ref_cell_free(ref_cell), "free");
    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* shard tri+qua to tri */

    REF_GRID ref_grid;
    REF_CELL ref_cell;

    RSS(ref_fixture_tri_qua_grid(&ref_grid, ref_mpi), "set up");

    RSS(ref_shard_extract_tri(ref_grid, &ref_cell), "shard to tri");

    if (!ref_mpi_para(ref_grid_mpi(ref_grid))) {
      REIS(ref_cell_n(ref_grid_tri(ref_grid)) +
               2 * ref_cell_n(ref_grid_qua(ref_grid)),
           ref_cell_n(ref_cell), "same ntri");
    }

    RSS(ref_cell_free(ref_cell), "free");
    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* shard tet to tet */

    REF_GRID ref_grid;
    REF_CELL ref_cell;

    RSS(ref_fixture_tet_grid(&ref_grid, ref_mpi), "set up");

    RSS(ref_shard_extract_tet(ref_grid, &ref_cell), "shard to tet");

    REIS(ref_cell_n(ref_grid_tet(ref_grid)), ref_cell_n(ref_cell), "same ntet");

    RSS(ref_cell_free(ref_cell), "free");
    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* shard pyr to tet */

    REF_GRID ref_grid;
    REF_CELL ref_cell;

    RSS(ref_fixture_pyr_grid(&ref_grid, ref_mpi), "set up");

    RSS(ref_shard_extract_tet(ref_grid, &ref_cell), "shard to tet");

    if (!ref_mpi_para(ref_grid_mpi(ref_grid))) {
      REIS(2 * ref_cell_n(ref_grid_pyr(ref_grid)), ref_cell_n(ref_cell),
           "same ntet");
    }

    RSS(ref_cell_free(ref_cell), "free");
    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* shard pri to tet */

    REF_GRID ref_grid;
    REF_CELL ref_cell;

    RSS(ref_fixture_pri_grid(&ref_grid, ref_mpi), "set up");

    RSS(ref_shard_extract_tet(ref_grid, &ref_cell), "shard to tet");

    if (!ref_mpi_para(ref_grid_mpi(ref_grid))) {
      REIS(3 * ref_cell_n(ref_grid_pri(ref_grid)), ref_cell_n(ref_cell),
           "same ntet");
    }

    RSS(ref_cell_free(ref_cell), "free");
    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* shard hex to tet */

    REF_GRID ref_grid;
    REF_CELL ref_cell;

    RSS(ref_fixture_hex_grid(&ref_grid, ref_mpi), "set up");

    RSS(ref_shard_extract_tet(ref_grid, &ref_cell), "shard to tet");

    if (!ref_mpi_para(ref_grid_mpi(ref_grid))) {
      REIS(6 * ref_cell_n(ref_grid_hex(ref_grid)), ref_cell_n(ref_cell),
           "same ntet");
    }

    RSS(ref_cell_free(ref_cell), "free");
    RSS(ref_grid_free(ref_grid), "free");
  }

  if (!ref_mpi_para(ref_mpi)) { /* shard hex brick */

    REF_GRID ref_grid;
    REF_GRID shard_grid;

    RSS(ref_fixture_hex_brick_grid(&ref_grid, ref_mpi), "fixture hex");

    RSS(ref_grid_deep_copy(&shard_grid, ref_grid), "deep copy");
    RSS(ref_cell_free(ref_grid_tet(shard_grid)), "free tet");
    RSS(ref_shard_extract_tet(ref_grid, &ref_grid_tet(shard_grid)),
        "shard to tet");
    RSS(ref_cell_free(ref_grid_tri(shard_grid)), "free tet");
    RSS(ref_shard_extract_tri(ref_grid, &ref_grid_tri(shard_grid)),
        "shard to tri");
    RSS(ref_cell_free(ref_grid_qua(shard_grid)), "free qua");
    RSS(ref_cell_create(&ref_grid_qua(shard_grid), REF_CELL_QUA), "qua create");
    RSS(ref_cell_free(ref_grid_hex(shard_grid)), "free hex");
    RSS(ref_cell_create(&ref_grid_hex(shard_grid), REF_CELL_HEX), "hex create");

    RSB(ref_validation_boundary_all(shard_grid), "valid",
        { ref_export_by_extension(shard_grid, "ref_shard_test_hex.tec"); });
    RSS(ref_validation_all(shard_grid), "valid");

    RSS(ref_grid_free(shard_grid), "free");
    RSS(ref_grid_free(ref_grid), "free");
  }

  if (!ref_mpi_para(ref_mpi)) { /* shard hex brick inplace */

    REF_GRID ref_grid;

    RSS(ref_fixture_hex_brick_grid(&ref_grid, ref_mpi), "fixture hex");

    RSS(ref_shard_in_place(ref_grid), "shard to simplex");

    REIS(0, ref_cell_n(ref_grid_qua(ref_grid)), "no more qua");
    REIS(0, ref_cell_n(ref_grid_pyr(ref_grid)), "no more pyr");
    REIS(0, ref_cell_n(ref_grid_pri(ref_grid)), "no more pri");
    REIS(0, ref_cell_n(ref_grid_hex(ref_grid)), "no more hex");

    RSB(ref_validation_simplex_node(ref_grid), "valid",
        { ref_export_by_extension(ref_grid, "ref_shard_test_hex.tec"); });

    RSS(ref_grid_free(ref_grid), "free");
  }

  RSS(ref_mpi_free(ref_mpi), "mpi free");
  RSS(ref_mpi_stop(), "stop");

  return 0;
}
