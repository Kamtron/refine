
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

#include "ref_oct.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ref_args.h"
#include "ref_cell.h"
#include "ref_export.h"
#include "ref_grid.h"
#include "ref_import.h"
#include "ref_matrix.h"
#include "ref_mpi.h"
#include "ref_node.h"
#include "ref_part.h"

int main(int argc, char *argv[]) {
  REF_INT pos;
  REF_MPI ref_mpi;
  RSS(ref_mpi_start(argc, argv), "start");
  RSS(ref_mpi_create(&ref_mpi), "make mpi");

  RXS(ref_args_find(argc, argv, "--tec", &pos), REF_NOT_FOUND, "arg search");
  if (pos != REF_EMPTY && pos + 1 < argc) {
    REF_OCT ref_oct;
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_split(ref_oct, 0), "split root");
    RSS(ref_oct_split(ref_oct, 1), "split first child");
    RSS(ref_oct_split(ref_oct, 9), "split second gen");
    RSS(ref_oct_tec(ref_oct, argv[pos + 1]), "tec");
    RSS(ref_oct_free(ref_oct), "free oct");
    RSS(ref_mpi_free(ref_mpi), "mpi free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  RXS(ref_args_find(argc, argv, "--box", &pos), REF_NOT_FOUND, "arg search");
  if (pos != REF_EMPTY && pos + 2 < argc) {
    REF_GRID ref_grid;
    REF_OCT ref_oct;
    REF_DBL h;
    h = atof(argv[pos + 1]);
    RSS(ref_grid_create(&ref_grid, ref_mpi), "make grid");
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_split_touching(ref_oct, ref_oct->bbox, h), "split");
    RSS(ref_oct_export(ref_oct, ref_grid), "export");
    RSS(ref_export_by_extension(ref_grid, argv[pos + 2]), "export");
    RSS(ref_oct_free(ref_oct), "free oct");
    RSS(ref_grid_free(ref_grid), "free grid");
    RSS(ref_mpi_free(ref_mpi), "mpi free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  RXS(ref_args_find(argc, argv, "--wmles", &pos), REF_NOT_FOUND, "arg search");
  if (pos != REF_EMPTY && pos + 2 < argc) {
    REF_GRID ref_grid;
    REF_OCT ref_oct;
    REF_DBL h;
    REF_DBL bbox[] = {0.0, 1.0, 0.0, 1.0, 0, 0.03};
    REF_INT nleaf;
    h = atof(argv[pos + 1]);
    RSS(ref_grid_create(&ref_grid, ref_mpi), "make grid");
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_split_touching(ref_oct, bbox, h), "split");
    RSS(ref_oct_nleaf(ref_oct, &nleaf), "count leaves");
    printf("raw %d vox\n", nleaf);
    RSS(ref_oct_gradation(ref_oct), "grad");
    RSS(ref_oct_nleaf(ref_oct, &nleaf), "count leaves");
    printf("grad %d vox\n", nleaf);
    RSS(ref_oct_export(ref_oct, ref_grid), "export");
    RSS(ref_export_by_extension(ref_grid, argv[pos + 2]), "export");
    RSS(ref_oct_free(ref_oct), "free oct");
    RSS(ref_grid_free(ref_grid), "free grid");
    RSS(ref_mpi_free(ref_mpi), "mpi free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  RXS(ref_args_find(argc, argv, "--point", &pos), REF_NOT_FOUND, "arg search");
  if (pos != REF_EMPTY && pos + 1 < argc) {
    REF_OCT ref_oct;
    REF_DBL xyz[3], h;
    RSS(ref_oct_create(&ref_oct), "make oct");
    xyz[0] = 0.3;
    xyz[1] = 0.4;
    xyz[2] = 0.49;
    h = 0.01;
    RSS(ref_oct_split_at(ref_oct, xyz, h), "split xyz h");
    RSS(ref_oct_tec(ref_oct, argv[pos + 1]), "tec");
    RSS(ref_oct_free(ref_oct), "search oct");
    RSS(ref_mpi_free(ref_mpi), "mpi free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  RXS(ref_args_find(argc, argv, "--surf", &pos), REF_NOT_FOUND, "arg search");
  if (pos != REF_EMPTY && pos + 2 < argc) {
    REF_OCT ref_oct;
    char orig[1024];
    REF_INT nleaf;

    RSS(ref_oct_create(&ref_oct), "make oct");
    {
      REF_GRID ref_grid;
      REF_CELL ref_cell;
      REF_NODE ref_node;
      REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];

      RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[pos + 1]), "import");
      ref_node = ref_grid_node(ref_grid);
      ref_cell = ref_grid_tri(ref_grid);
      each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
        REF_DBL xyz[3], h, area;
        REF_INT i, j;
        RSS(ref_node_tri_area(ref_node, nodes, &area), "tri area");
        h = sqrt(area);
        for (j = 0; j < 3; j++) {
          xyz[j] = 0;
          for (i = 0; i < 3; i++) {
            xyz[j] += ref_node_xyz(ref_node, j, nodes[i]) / 3.0;
          }
        }
        RSS(ref_oct_split_at(ref_oct, xyz, h), "split xyz h");
      }
      RSS(ref_grid_free(ref_grid), "free grid");
    }
    snprintf(orig, 1024, "%s-raw.tec", argv[pos + 2]);
    RSS(ref_oct_nleaf(ref_oct, &nleaf), "count leaves");
    printf("writing %d vox to %s from %s\n", nleaf, orig, argv[pos + 1]);
    RSS(ref_oct_tec(ref_oct, orig), "tec");
    RSS(ref_oct_gradation(ref_oct), "grad");
    RSS(ref_oct_nleaf(ref_oct, &nleaf), "count leaves");
    printf("writing %d vox to %s from %s\n", nleaf, argv[pos + 2],
           argv[pos + 1]);
    RSS(ref_oct_tec(ref_oct, argv[pos + 2]), "tec");
    RSS(ref_oct_free(ref_oct), "search oct");
    RSS(ref_mpi_free(ref_mpi), "mpi free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  RXS(ref_args_find(argc, argv, "--adapt", &pos), REF_NOT_FOUND, "arg search");
  if (pos != REF_EMPTY && pos + 3 < argc) {
    REF_OCT ref_oct;
    REF_INT nleaf;
    REF_BOOL debug = REF_FALSE;

    RSS(ref_oct_create(&ref_oct), "make oct");
    {
      REF_GRID ref_grid;
      REF_NODE ref_node;
      REF_INT node;

      RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[pos + 1]), "import");
      ref_node = ref_grid_node(ref_grid);
      RSS(ref_part_metric(ref_grid_node(ref_grid), argv[pos + 2]),
          "unable to load parent metric in position pos + 2");
      each_ref_node_valid_node(ref_node, node) {
        REF_DBL m[6], d[12], h;
        RSS(ref_node_metric_get(ref_node, node, m), "get");
        RSS(ref_matrix_diag_m(m, d), "decomp");
        if (ref_grid_twod(ref_grid)) {
          RSS(ref_matrix_descending_eig_twod(d), "2D ascend");
        } else {
          RSS(ref_matrix_descending_eig(d), "3D ascend");
        }
        h = 1.0 / sqrt(ref_matrix_eig(d, 0));
        RSS(ref_oct_split_at(ref_oct, ref_node_xyz_ptr(ref_node, node), h),
            "split xyz h");
      }
      RSS(ref_grid_free(ref_grid), "free grid");
    }

    RSS(ref_oct_nleaf(ref_oct, &nleaf), "count leaves");
    if (debug) {
      char tec[1024];
      snprintf(tec, 1024, "%s-raw.tec", argv[pos + 3]);
      printf("writing %d vox to %s from %s\n", nleaf, tec, argv[pos + 3]);
      RSS(ref_oct_tec(ref_oct, tec), "tec");
    } else {
      printf("raw vox %d\n", nleaf);
    }
    RSS(ref_oct_gradation(ref_oct), "grad");
    RSS(ref_oct_nleaf(ref_oct, &nleaf), "count leaves");
    if (debug) {
      char tec[1024];
      snprintf(tec, 1024, "%s-grad.tec", argv[pos + 3]);
      printf("writing %d vox to %s from %s\n", nleaf, tec, argv[pos + 3]);
      RSS(ref_oct_tec(ref_oct, tec), "tec");
    }
    printf("writing %d vox to %s\n", nleaf, argv[pos + 3]);
    {
      REF_GRID ref_grid;
      RSS(ref_grid_create(&ref_grid, ref_mpi), "make grid");
      RSS(ref_oct_export(ref_oct, ref_grid), "export");
      RSS(ref_export_by_extension(ref_grid, argv[pos + 3]), "export");
      RSS(ref_grid_free(ref_grid), "free grid");
    }
    RSS(ref_oct_free(ref_oct), "search oct");
    RSS(ref_mpi_free(ref_mpi), "mpi free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  { /* create */
    REF_OCT ref_oct;
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_free(ref_oct), "free oct");
  }

  { /* child's bbox */
    REF_DBL parent_bbox[6], child_bbox[6];
    REF_INT child_index = 0;
    REF_DBL tol = -1.0;
    parent_bbox[0] = 0.0;
    parent_bbox[1] = 1.0;
    parent_bbox[2] = 0.0;
    parent_bbox[3] = 1.0;
    parent_bbox[4] = 0.0;
    parent_bbox[5] = 1.0;
    RSS(ref_oct_child_bbox(parent_bbox, child_index, child_bbox), "bbox");
    RWDS(0.0, child_bbox[0], tol, "not zero");
  }

  { /* bbox diag */
    REF_DBL bbox[6];
    REF_DBL diag, tol = -1.0;
    bbox[0] = 0.0;
    bbox[1] = 0.5;
    bbox[2] = 0.5;
    bbox[3] = 1.0;
    bbox[4] = 0.2;
    bbox[5] = 0.7;
    RSS(ref_oct_bbox_diag(bbox, &diag), "bbox diag");
    RWDS(sqrt(0.75), diag, tol, "wrong size");
  }

  { /* split root */
    REF_OCT ref_oct;
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_split(ref_oct, 0), "split oct");
    RSS(ref_oct_free(ref_oct), "free oct");
  }

  { /* one refinment, matching bbox */
    REF_OCT ref_oct;
    REF_DBL bbox[6], h;
    bbox[0] = 0.0;
    bbox[1] = 1.0;
    bbox[2] = 0.0;
    bbox[3] = 1.0;
    bbox[4] = 0.0;
    bbox[5] = 1.0;
    h = 0.9 * sqrt(3.0);
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_split_touching(ref_oct, bbox, h), "split main");
    REIS(9, ref_oct_n(ref_oct), "expect one refinement, match")
    RSS(ref_oct_free(ref_oct), "free oct");
  }

  { /* two refinment, matching corner bbox */
    REF_OCT ref_oct;
    REF_DBL bbox[6], h;
    bbox[0] = 0.0;
    bbox[1] = 0.2;
    bbox[2] = 0.0;
    bbox[3] = 0.2;
    bbox[4] = 0.0;
    bbox[5] = 0.2;
    h = 0.45 * sqrt(3.0);
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_split_touching(ref_oct, bbox, h), "split main");
    REIS(17, ref_oct_n(ref_oct), "expect one refinement, match")
    RSS(ref_oct_free(ref_oct), "free oct");
  }

  { /* gradation of spot refinement */
    REF_OCT ref_oct;
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_split(ref_oct, 0), "split root");
    RSS(ref_oct_split(ref_oct, 1), "split first child");
    RSS(ref_oct_split(ref_oct, 14), "split second gen");
    RSS(ref_oct_gradation(ref_oct), "gradation");
    RSS(ref_oct_free(ref_oct), "free oct");
  }

  { /* root unique nodes */
    REF_NODE ref_node;
    REF_OCT ref_oct;
    RSS(ref_node_create(&ref_node, ref_mpi), "make node");
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_unique_nodes(ref_oct, ref_node), "make nodes");
    REIS(8, ref_oct_nnode(ref_oct), "expects 8 node hex");
    REIS(8, ref_node_n(ref_node), "ref_node n");
    REIS(8, ref_node_n_global(ref_node), "ref_node global n");
    RSS(ref_oct_free(ref_oct), "free oct");
    RSS(ref_node_free(ref_node), "free node");
  }

  { /* one split unique nodes */
    REF_NODE ref_node;
    REF_OCT ref_oct;
    RSS(ref_node_create(&ref_node, ref_mpi), "make node");
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_split(ref_oct, 0), "split root");
    RSS(ref_oct_unique_nodes(ref_oct, ref_node), "make nodes");
    REIS(27, ref_oct_nnode(ref_oct), "expects 8 node hex");
    REIS(27, ref_node_n(ref_node), "ref_node n");
    REIS(27, ref_node_n_global(ref_node), "ref_node global n");
    RSS(ref_oct_free(ref_oct), "free oct");
    RSS(ref_node_free(ref_node), "free node");
  }

  { /* root export ref_grid */
    REF_GRID ref_grid;
    REF_OCT ref_oct;
    RSS(ref_grid_create(&ref_grid, ref_mpi), "make grid");
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_export(ref_oct, ref_grid), "export");
    REIS(1, ref_cell_n(ref_grid_hex(ref_grid)), "hex");
    REIS(6, ref_cell_n(ref_grid_qua(ref_grid)), "qua");
    RSS(ref_oct_free(ref_oct), "free oct");
    RSS(ref_grid_free(ref_grid), "free grid");
  }

  { /* one export ref_grid */
    REF_GRID ref_grid;
    REF_OCT ref_oct;
    RSS(ref_grid_create(&ref_grid, ref_mpi), "make grid");
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_split(ref_oct, 0), "split root");
    RSS(ref_oct_export(ref_oct, ref_grid), "export");
    REIS(8, ref_cell_n(ref_grid_hex(ref_grid)), "hex");
    REIS(24, ref_cell_n(ref_grid_qua(ref_grid)), "qua");
    RSS(ref_oct_free(ref_oct), "free oct");
    RSS(ref_grid_free(ref_grid), "free grid");
  }

  { /* spot export ref_grid */
    REF_GRID ref_grid;
    REF_OCT ref_oct;
    RSS(ref_grid_create(&ref_grid, ref_mpi), "make grid");
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_split(ref_oct, 0), "split root");
    RSS(ref_oct_split(ref_oct, 1), "split first");
    RSS(ref_oct_export(ref_oct, ref_grid), "export");
    RSS(ref_oct_free(ref_oct), "free oct");
    RSS(ref_grid_free(ref_grid), "free grid");
  }

  { /* contains root */
    REF_OCT ref_oct;
    REF_DBL xyz[] = {0.1, 0.1, 0.1};
    REF_DBL bbox[6];
    REF_INT node;
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_contains(ref_oct, xyz, &node, bbox), "contains oct");
    REIS(0, node, "expects root");
    xyz[0] = 100.0;
    RSS(ref_oct_contains(ref_oct, xyz, &node, bbox), "contains oct");
    REIS(REF_EMPTY, node, "expects empty outside root");
    RSS(ref_oct_free(ref_oct), "free oct");
  }

  { /* contains child */
    REF_OCT ref_oct;
    REF_DBL xyz[] = {0.1, 0.1, 0.1};
    REF_DBL bbox[6];
    REF_INT node;
    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_split(ref_oct, 0), "split oct");
    RSS(ref_oct_contains(ref_oct, xyz, &node, bbox), "contains oct");
    REIS(1, node, "expects first child");
    xyz[0] = 0.9;
    xyz[1] = 0.1;
    xyz[2] = 0.1;
    RSS(ref_oct_contains(ref_oct, xyz, &node, bbox), "contains oct");
    REIS(2, node, "expects last child");
    xyz[0] = 0.1;
    xyz[1] = 0.9;
    xyz[2] = 0.1;
    RSS(ref_oct_contains(ref_oct, xyz, &node, bbox), "contains oct");
    REIS(4, node, "expects last child");
    xyz[0] = 0.9;
    xyz[1] = 0.9;
    xyz[2] = 0.9;
    RSS(ref_oct_contains(ref_oct, xyz, &node, bbox), "contains oct");
    REIS(7, node, "expects last child");
    RSS(ref_oct_free(ref_oct), "free oct");
  }

  {
    REF_OCT ref_oct;
    REF_DBL xyz[] = {0.01, 0.01, 0.01};
    REF_DBL bbox[6];
    REF_INT node;

    RSS(ref_oct_create(&ref_oct), "make oct");
    RSS(ref_oct_split(ref_oct, 0), "split root");
    RSS(ref_oct_split(ref_oct, 1), "split first child");
    RSS(ref_oct_split(ref_oct, 9), "split second gen");
    RSS(ref_oct_contains(ref_oct, xyz, &node, bbox), "contains oct");
    REIS(17, node, "expects third level");
    RSS(ref_oct_free(ref_oct), "free oct");
  }

  {
    REF_DBL bbox0[6], bbox1[6];
    REF_BOOL overlap;
    bbox0[0] = 0.0;
    bbox0[1] = 1.0;
    bbox0[2] = 0.0;
    bbox0[3] = 1.0;
    bbox0[4] = 0.0;
    bbox0[5] = 1.0;
    bbox1[0] = 0.1;
    bbox1[1] = 0.2;
    bbox1[2] = 0.1;
    bbox1[3] = 0.2;
    bbox1[4] = 0.1;
    bbox1[5] = 0.2;
    RSS(ref_oct_bbox_overlap(bbox0, bbox1, &overlap), "overlap");
    REIS(REF_TRUE, overlap, "expect overlap");
    bbox1[0] = 1.1;
    bbox1[1] = 1.2;
    bbox1[2] = 0.1;
    bbox1[3] = 0.2;
    bbox1[4] = 0.1;
    bbox1[5] = 0.2;
    RSS(ref_oct_bbox_overlap(bbox0, bbox1, &overlap), "overlap");
    REIS(REF_FALSE, overlap, "did expect overlap");
  }

  {
    REF_DBL bbox0[6], bbox1[6];
    REF_DBL factor;
    REF_DBL tol = -1.0;
    bbox0[0] = 0.0;
    bbox0[1] = 1.0;
    bbox0[2] = 0.0;
    bbox0[3] = 2.0;
    bbox0[4] = 0.0;
    bbox0[5] = 4.0;
    factor = 1.1;
    RSS(ref_oct_bbox_scale(bbox0, factor, bbox1), "scale");
    RWDS(-0.05, bbox1[0], tol, "xmin");
    RWDS(1.05, bbox1[1], tol, "xmax");
    RWDS(-0.1, bbox1[2], tol, "ymin");
    RWDS(2.1, bbox1[3], tol, "ymax");
    RWDS(-0.2, bbox1[4], tol, "zmin");
    RWDS(4.2, bbox1[5], tol, "zmax");
  }

  RSS(ref_mpi_free(ref_mpi), "mpi free");
  RSS(ref_mpi_stop(), "stop");
  return 0;
}
