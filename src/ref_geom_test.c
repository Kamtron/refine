
/* Copyright 2014 United States Government as represented by the
 * Administrator of the National Aeronautics and Space
 * Administration. No copyright is claimed in the United States under
 * Title 17, U.S. Code.  All Other Rights Reserved.
 *
 * The refine platform is licensed under the Apache License, Version
 * 2.0 (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ref_geom.h"

#include "ref_adj.h"
#include "ref_cell.h"
#include "ref_dict.h"
#include "ref_edge.h"
#include "ref_export.h"
#include "ref_fixture.h"
#include "ref_grid.h"
#include "ref_import.h"
#include "ref_list.h"
#include "ref_matrix.h"
#include "ref_metric.h"
#include "ref_mpi.h"
#include "ref_node.h"
#include "ref_sort.h"
#include "ref_validation.h"

#include "ref_histogram.h"

#include "ref_args.h"
#include "ref_math.h"

static REF_STATUS ref_geom_tri_status(REF_GRID ref_grid) {
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER], id;
  REF_DBL min_normdev, min_area, max_area, min_uv_area, max_uv_area;
  REF_DBL normdev, area, uv_area, area_sign;

  min_normdev = 2.0;
  min_area = 1.0e100;
  max_area = -1.0e100;
  min_uv_area = 1.0e100;
  max_uv_area = -1.0e100;
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    RSS(ref_geom_tri_norm_deviation(ref_grid, nodes, &normdev), "norm dev");
    min_normdev = MIN(min_normdev, normdev);
    RSS(ref_node_tri_area(ref_grid_node(ref_grid), nodes, &area), "vol");
    min_area = MIN(min_area, area);
    max_area = MAX(max_area, area);
    id = nodes[ref_cell_node_per(ref_cell)];
    RSS(ref_geom_uv_area_sign(ref_grid, id, &area_sign), "a sign");
    RSS(ref_geom_uv_area(ref_grid_geom(ref_grid), nodes, &uv_area), "uv area");
    uv_area *= area_sign;
    min_uv_area = MIN(min_uv_area, uv_area);
    max_uv_area = MAX(max_uv_area, uv_area);
  }
  printf("normdev %f area %.5e  %.5e uv area  %.5e  %.5e\n", min_normdev,
         min_area, max_area, min_uv_area, max_uv_area);

  return REF_SUCCESS;
}

static REF_STATUS ref_geom_tet_status(REF_GRID ref_grid) {
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL min_volume, max_volume;
  REF_DBL volume;

  min_volume = 1.0e100;
  max_volume = -1.0e100;
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    RSS(ref_node_tet_vol(ref_grid_node(ref_grid), nodes, &volume), "vol");
    min_volume = MIN(min_volume, volume);
    max_volume = MAX(max_volume, volume);
  }
  printf("volume %.5e %.5e\n", min_volume, max_volume);

  return REF_SUCCESS;
}

static REF_STATUS ref_geom_surf_adapt(REF_GRID ref_grid) {
  REF_BOOL all_done;
  int passes = 15, pass;

  RSS(ref_metric_interpolated_curvature(ref_grid), "interp curve");

  ref_grid_surf(ref_grid) = REF_TRUE;
  ref_geom_segments_per_radian_of_curvature(ref_grid_geom(ref_grid)) = 2.0;
  ref_grid_adapt(ref_grid, watch_param) = REF_TRUE;
  ref_grid_adapt(ref_grid, instrument) = REF_TRUE; /* timing datails */
  ref_grid_adapt(ref_grid, collapse_per_pass) = 5; /* timing datails */
  RSS(ref_gather_tec_movie_record_button(ref_grid_gather(ref_grid), REF_TRUE),
      "show time");

  for (pass = 0; pass < passes; pass++) {
    if (ref_grid_once(ref_grid))
      printf("\n pass %d of %d with %d ranks\n", pass + 1, passes,
             ref_mpi_n(ref_grid_mpi(ref_grid)));
    RSS(ref_adapt_parameter(ref_grid, &all_done), "param");
    if (all_done) break;
    RSS(ref_adapt_pass(ref_grid), "pass");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "pass");
    RSS(ref_metric_interpolated_curvature(ref_grid), "interp curve");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "curvature");
    RSS(ref_validation_cell_volume(ref_grid), "vol");
    RSS(ref_histogram_quality(ref_grid), "gram");
    RSS(ref_histogram_ratio(ref_grid), "gram");
    RSS(ref_grid_pack(ref_grid), "pack");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "pack");
  }

  return REF_SUCCESS;
}

int main(int argc, char *argv[]) {
  REF_MPI ref_mpi;
  REF_INT recon_pos = REF_EMPTY;
  REF_INT viz_pos = REF_EMPTY;
  REF_INT tess_pos = REF_EMPTY;
  REF_INT tetgen_pos = REF_EMPTY;
  REF_INT face_pos = REF_EMPTY;
  REF_INT surf_pos = REF_EMPTY;

  RSS(ref_mpi_create(&ref_mpi), "create");

  RXS(ref_args_find(argc, argv, "--recon", &recon_pos), REF_NOT_FOUND,
      "arg search");
  RXS(ref_args_find(argc, argv, "--viz", &viz_pos), REF_NOT_FOUND,
      "arg search");
  RXS(ref_args_find(argc, argv, "--tess", &tess_pos), REF_NOT_FOUND,
      "arg search");
  RXS(ref_args_find(argc, argv, "--tetgen", &tetgen_pos), REF_NOT_FOUND,
      "arg search");
  RXS(ref_args_find(argc, argv, "--face", &face_pos), REF_NOT_FOUND,
      "arg search");
  RXS(ref_args_find(argc, argv, "--surf", &surf_pos), REF_NOT_FOUND,
      "arg search");

  if (face_pos != REF_EMPTY) {
    REF_GRID ref_grid;
    REIS(4, argc, "required args: --face grid.ext geom.egads");
    REIS(1, face_pos, "required args: --face grid.ext geom.egads");
    printf("match face id geometry bounding boxes\n");
    printf("grid source %s\n", argv[2]);
    printf("geometry source %s\n", argv[3]);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[2]), "argv import");
    RSS(ref_geom_egads_load(ref_grid_geom(ref_grid), argv[3]), "ld egads");
    RSS(ref_geom_face_match(ref_grid), "geom recon");
    RSS(ref_export_by_extension(ref_grid, "ref_geom_face.meshb"), "export");
    RSS(ref_grid_free(ref_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "free");
    return 0;
  }

  if (viz_pos != REF_EMPTY) {
    REF_GRID ref_grid;
    REIS(3, argc, "required args: --viz grid.ext");
    REIS(1, viz_pos, "required args: --viz grid.ext");
    printf("grid source %s\n", argv[2]);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[2]), "argv import");
    RSS(ref_geom_tec(ref_grid, "ref_geom_viz.tec"), "geom export");
    RSS(ref_grid_free(ref_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "free");
    return 0;
  }

  if (recon_pos != REF_EMPTY) {
    REF_GRID ref_grid;
    REF_INT node;
    REIS(4, argc, "required args: --recon grid.ext geom.egads");
    REIS(1, recon_pos, "required args: --recon grid.ext geom.egads");
    printf("reconstruct geometry association\n");
    printf("grid source %s\n", argv[2]);
    printf("geometry source %s\n", argv[3]);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[2]), "argv import");
    RSS(ref_geom_egads_load(ref_grid_geom(ref_grid), argv[3]), "ld egads");
    RSS(ref_geom_recon(ref_grid), "geom recon");
    printf("verify topo and params\n");
    RSS(ref_geom_verify_topo(ref_grid), "geom topo conflict");
    RSS(ref_geom_verify_param(ref_grid), "test constrained params");
    RSS(ref_metric_unit_node(ref_grid_node(ref_grid)), "unit metric");
    printf("validate\n");
    RSS(ref_validation_all(ref_grid), "validate");
    printf("constrain\n");
    RSS(ref_export_tec_surf(ref_grid, "ref_geom_orig.tec"), "tec");
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RSS(ref_geom_constrain(ref_grid, node), "original params");
    }
    RSS(ref_geom_tec(ref_grid, "ref_geom_recon.tec"), "geom export");
    printf("validate\n");
    RSS(ref_validation_all(ref_grid), "validate");
    RSS(ref_export_by_extension(ref_grid, "ref_geom_recon.meshb"), "export");
    RSS(ref_grid_free(ref_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "free");
    return 0;
  }

  if (tess_pos != REF_EMPTY || tetgen_pos != REF_EMPTY) { /* egads to grid */
    REF_GRID ref_grid;
    REF_INT node;
    REF_DBL params[3];
    REF_BOOL aflr_over_tetgen = REF_TRUE;

    if (tess_pos != REF_EMPTY) {
      REIS(1, tess_pos,
           "required args: --tess input.egads output.meshb edge chord angle");
      if (7 > argc) {
        printf(
            "required args: --tess input.egads output.meshb edge chord angle");
        return REF_FAILURE;
      }
      aflr_over_tetgen = REF_TRUE;
    }

    if (tetgen_pos != REF_EMPTY) {
      REIS(1, tetgen_pos,
           "required args: --tetgen input.egads output.meshb edge chord angle");
      if (7 > argc) {
        printf(
            "required args: --tetgen input.egads output.meshb edge chord "
            "angle");
        return REF_FAILURE;
      }
      aflr_over_tetgen = REF_FALSE;
    }

    params[0] = atof(argv[4]);
    params[1] = atof(argv[5]);
    params[2] = atof(argv[6]);

    RSS(ref_grid_create(&ref_grid, ref_mpi), "create");

    RSS(ref_geom_egads_load(ref_grid_geom(ref_grid), argv[2]), "ld egads");
    RSS(ref_geom_egads_tess(ref_grid, params), "tess egads");
    RSS(ref_export_by_extension(ref_grid, "ref_geom_test_tess.meshb"),
        "meshb export");
    RSS(ref_geom_tec(ref_grid, "ref_geom_test_tess.tec"), "geom export");
    RSS(ref_geom_tec(ref_grid, "ref_geom_test_tess.tec"), "geom export");

    RSS(ref_geom_tri_status(ref_grid), "tri status");
    printf("verify topo\n");
    RSS(ref_geom_verify_topo(ref_grid), "original params");
    printf("verify param\n");
    RSS(ref_geom_verify_param(ref_grid), "original params");
    printf("constrain\n");
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RSS(ref_geom_constrain(ref_grid, node), "original params");
    }
    printf("verify param\n");
    RSS(ref_geom_verify_param(ref_grid), "original params");

    if (REF_EMPTY != surf_pos) {
      RSS(ref_geom_surf_adapt(ref_grid), "adapt surface before volume");
      RSS(ref_geom_tri_status(ref_grid), "tri status");
      printf("verify topo\n");
      RSS(ref_geom_verify_topo(ref_grid), "original params");
      printf("verify param\n");
    }

    printf("generate volume\n");
    if (aflr_over_tetgen) {
      RSS(ref_geom_aflr_volume(ref_grid), "surface to volume ");
    } else {
      RSS(ref_geom_tetgen_volume(ref_grid), "tetgen surface to volume ");
    }
    RSS(ref_grid_inspect(ref_grid), "report size");

    RSS(ref_export_by_extension(ref_grid, argv[3]), "argv export");
    RSS(ref_geom_tec(ref_grid, "ref_geom_test_vol_geom.tec"), "geom export");

    RSS(ref_geom_tet_status(ref_grid), "original validation");
    printf("validate\n");
    RSS(ref_validation_all(ref_grid), "original validation");
    printf("constrain\n");
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RSS(ref_geom_constrain(ref_grid, node), "original params");
    }
    printf("verify topo\n");
    RSS(ref_geom_verify_topo(ref_grid), "original params");
    printf("verify param\n");
    RSS(ref_geom_verify_param(ref_grid), "constrained params");
    printf("validate\n");
    RSS(ref_validation_all(ref_grid), "constrained validation");
    RSS(ref_grid_free(ref_grid), "free");
  }

  REIS(REF_NULL, ref_geom_free(NULL), "dont free NULL");

  { /* create and destroy */
    REF_GEOM ref_geom;
    RSS(ref_geom_create(&ref_geom), "create");
    REIS(0, ref_geom_n(ref_geom), "init no nodes");
    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* deep copy empty */
    REF_GEOM ref_geom;
    REF_GEOM original;

    RSS(ref_geom_create(&original), "create");
    RSS(ref_geom_deep_copy(&ref_geom, original), "deep copy");
    REIS(ref_geom_n(original), ref_geom_n(ref_geom), "items");
    REIS(ref_geom_max(original), ref_geom_max(ref_geom), "items");
    RSS(ref_geom_free(original), "cleanup");
    RSS(ref_geom_free(ref_geom), "cleanup");
  }

  { /* add geom node */
    REF_GEOM ref_geom;
    REF_INT node, type, id;
    REF_DBL *params;
    RSS(ref_geom_create(&ref_geom), "create");
    node = 2;
    type = REF_GEOM_NODE;
    id = 5;
    params = NULL;
    REIS(0, ref_geom_add(ref_geom, node, type, id, params), "add node");
    REIS(1, ref_geom_n(ref_geom), "items");
    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* add geom edge */
    REF_GEOM ref_geom;
    REF_INT node, type, id;
    REF_DBL params[1];
    RSS(ref_geom_create(&ref_geom), "create");
    node = 2;
    type = REF_GEOM_EDGE;
    id = 5;
    params[0] = 11.0;
    REIS(0, ref_geom_add(ref_geom, node, type, id, params), "add edge");
    REIS(1, ref_geom_n(ref_geom), "items");
    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* add geom face */
    REF_GEOM ref_geom;
    REF_INT node, type, id;
    REF_DBL params[2];
    RSS(ref_geom_create(&ref_geom), "create");
    node = 2;
    type = REF_GEOM_FACE;
    id = 5;
    params[0] = 11.0;
    params[1] = 21.0;
    REIS(0, ref_geom_add(ref_geom, node, type, id, params), "add face");
    REIS(1, ref_geom_n(ref_geom), "items");
    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* add updates face uv*/
    REF_GEOM ref_geom;
    REF_INT node, type, id;
    REF_DBL params[2], uv[2];
    REF_DBL tol = 1.0e-14;
    RSS(ref_geom_create(&ref_geom), "create");
    node = 2;
    type = REF_GEOM_FACE;
    id = 5;
    params[0] = 11.0;
    params[1] = 21.0;
    REIS(0, ref_geom_add(ref_geom, node, type, id, params), "add face");
    REIS(0, ref_geom_tuv(ref_geom, node, type, id, uv), "face uv");
    RWDS(params[0], uv[0], tol, "u");
    RWDS(params[1], uv[1], tol, "v");
    params[0] = 12.0;
    params[1] = 22.0;
    REIS(0, ref_geom_add(ref_geom, node, type, id, params), "add face");
    REIS(0, ref_geom_tuv(ref_geom, node, type, id, uv), "face uv");
    RWDS(params[0], uv[0], tol, "u");
    RWDS(params[1], uv[1], tol, "v");
    REIS(1, ref_geom_n(ref_geom), "items");
    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* add and remove all */
    REF_GEOM ref_geom;
    REF_INT node, type, id;
    REF_DBL params[2];
    RSS(ref_geom_create(&ref_geom), "create");
    node = 2;
    type = REF_GEOM_FACE;
    id = 5;
    params[0] = 11.0;
    params[1] = 21.0;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "add face");
    node = 2;
    type = REF_GEOM_EDGE;
    id = 2;
    params[0] = 5.0;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "add edge");
    REIS(2, ref_geom_n(ref_geom), "items");
    node = 4;
    type = REF_GEOM_EDGE;
    id = 2;
    params[0] = 5.0;
    RSS(ref_geom_remove_all(ref_geom, node), "ok with nothing there");
    REIS(2, ref_geom_n(ref_geom), "items");
    node = 2;
    RSS(ref_geom_remove_all(ref_geom, node), "remove all at node");
    REIS(0, ref_geom_n(ref_geom), "items");
    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* reuse without reallocation */
    REF_GEOM ref_geom;
    REF_INT node, type, id;
    REF_INT geom, max;
    REF_DBL params[2];
    RSS(ref_geom_create(&ref_geom), "create");
    max = ref_geom_max(ref_geom);

    for (geom = 0; geom < max + 10; geom++) {
      node = 10000 + geom;
      type = REF_GEOM_FACE;
      id = 5;
      params[0] = 1.0;
      params[1] = 2.0;
      REIS(0, ref_geom_add(ref_geom, node, type, id, params), "add face");
      REIS(0, ref_geom_remove_all(ref_geom, node), "rm all");
    }
    REIS(max, ref_geom_max(ref_geom), "items");
    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* force realloc twice */
    REF_GEOM ref_geom;
    REF_INT max, i;
    REF_INT node, type, id;
    REF_DBL params[2];

    RSS(ref_geom_create(&ref_geom), "create");

    max = ref_geom_max(ref_geom);
    for (i = 0; i < max + 1; i++) {
      node = i;
      type = REF_GEOM_FACE;
      id = 5;
      params[0] = 1.0;
      params[1] = 2.0;
      REIS(0, ref_geom_add(ref_geom, node, type, id, params), "add face");
    }
    RAS(ref_geom_max(ref_geom) > max, "realloc max");

    max = ref_geom_max(ref_geom);
    for (i = ref_geom_n(ref_geom); i < max + 1; i++) {
      node = i;
      type = REF_GEOM_FACE;
      id = 5;
      params[0] = 1.0;
      params[1] = 2.0;
      REIS(0, ref_geom_add(ref_geom, node, type, id, params), "add face");
    }
    RAS(ref_geom_max(ref_geom) > max, "realloc max");

    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* add between common edge */
    REF_GRID ref_grid;
    REF_GEOM ref_geom;
    REF_INT node0, node1, new_node;
    REF_INT type, id, geom;
    REF_DBL params[2];
    REF_DBL tol = 1.0e-12;
    REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];
    RSS(ref_grid_create(&ref_grid, ref_mpi), "create");
    ref_geom = ref_grid_geom(ref_grid);

    node0 = 0;
    node1 = 1;
    new_node = 10;

    nodes[0] = node0;
    nodes[1] = node1;
    nodes[2] = 3;
    nodes[3] = 20;
    RSS(ref_cell_add(ref_grid_tri(ref_grid), nodes, &cell), "add tri");
    nodes[0] = node1;
    nodes[1] = node0;
    nodes[2] = 4;
    nodes[3] = 20;
    RSS(ref_cell_add(ref_grid_tri(ref_grid), nodes, &cell), "add tri");
    type = REF_GEOM_FACE;
    id = 20;
    params[0] = 100.0;
    params[1] = 200.0;
    RSS(ref_geom_add(ref_geom, node0, type, id, params), "node0 edge");
    type = REF_GEOM_FACE;
    id = 20;
    params[0] = 101.0;
    params[1] = 201.0;
    RSS(ref_geom_add(ref_geom, node1, type, id, params), "node1 edge");

    type = REF_GEOM_EDGE;
    id = 5;
    params[0] = 11.0;
    RSS(ref_geom_add(ref_geom, node0, type, id, params), "node0 edge");
    type = REF_GEOM_EDGE;
    id = 5;
    params[0] = 13.0;
    RSS(ref_geom_add(ref_geom, node1, type, id, params), "node1 edge");
    params[0] = 0.0;

    RSS(ref_geom_add_between(ref_grid, node0, node1, new_node), "between");
    id = 5;
    REIS(REF_NOT_FOUND, ref_geom_find(ref_geom, new_node, type, id, &geom),
         "what edge");

    nodes[0] = node0;
    nodes[1] = node1;
    nodes[2] = id;
    RSS(ref_cell_add(ref_grid_edg(ref_grid), nodes, &cell), "add edge");
    RSS(ref_geom_add_between(ref_grid, node0, node1, new_node), "between");
    RSS(ref_geom_tuv(ref_geom, new_node, type, id, params), "node1 edge");
    RWDS(12.0, params[0], tol, "v");

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* add between skip different edge */
    REF_GRID ref_grid;
    REF_GEOM ref_geom;
    REF_INT node0, node1, new_node;
    REF_INT type, id, geom;
    REF_DBL params[2];
    REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];

    RSS(ref_grid_create(&ref_grid, ref_mpi), "create");
    ref_geom = ref_grid_geom(ref_grid);

    node0 = 0;
    node1 = 1;
    new_node = 10;

    nodes[0] = node0;
    nodes[1] = node1;
    nodes[2] = 3;
    nodes[3] = 20;
    RSS(ref_cell_add(ref_grid_tri(ref_grid), nodes, &cell), "add tri");
    nodes[0] = node1;
    nodes[1] = node0;
    nodes[2] = 4;
    nodes[3] = 20;
    RSS(ref_cell_add(ref_grid_tri(ref_grid), nodes, &cell), "add tri");
    type = REF_GEOM_FACE;
    id = 20;
    params[0] = 100.0;
    params[1] = 200.0;
    RSS(ref_geom_add(ref_geom, node0, type, id, params), "node0 edge");
    type = REF_GEOM_FACE;
    id = 20;
    params[0] = 101.0;
    params[1] = 201.0;
    RSS(ref_geom_add(ref_geom, node1, type, id, params), "node1 edge");

    type = REF_GEOM_EDGE;
    id = 5;
    params[0] = 11.0;
    RSS(ref_geom_add(ref_geom, node0, type, id, params), "node0 edge");
    type = REF_GEOM_EDGE;
    id = 7;
    params[0] = 13.0;
    RSS(ref_geom_add(ref_geom, node1, type, id, params), "node1 edge");
    params[0] = 0.0;

    RSS(ref_geom_add_between(ref_grid, node0, node1, new_node), "between");
    id = 7;
    REIS(REF_NOT_FOUND, ref_geom_find(ref_geom, new_node, type, id, &geom),
         "what edge");
    id = 5;
    REIS(REF_NOT_FOUND, ref_geom_find(ref_geom, new_node, type, id, &geom),
         "what edge");

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* eval out of range */
    REF_GEOM ref_geom;
    REF_INT geom;
    REF_DBL xyz[3];

    RSS(ref_geom_create(&ref_geom), "create");

    geom = -1;
    REIS(REF_INVALID, ref_geom_eval(ref_geom, geom, xyz, NULL), "invalid geom");
    geom = 1 + ref_geom_max(ref_geom);
    REIS(REF_INVALID, ref_geom_eval(ref_geom, geom, xyz, NULL), "invalid geom");

    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* constrain without geom */
    REF_GRID ref_grid;
    REF_INT node;

    RSS(ref_grid_create(&ref_grid, ref_mpi), "create");

    node = 4;
    RSS(ref_geom_constrain(ref_grid, node), "no geom");

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* has_support */
    REF_GEOM ref_geom;
    REF_INT node, type, id;
    REF_DBL *params;
    REF_BOOL has_support;

    RSS(ref_geom_create(&ref_geom), "create");

    node = 2;
    type = REF_GEOM_NODE;
    id = 5;
    params = NULL;

    RSS(ref_geom_supported(ref_geom, node, &has_support), "empty");
    RAS(REF_FALSE == has_support, "phantom support");

    REIS(0, ref_geom_add(ref_geom, node, type, id, params), "add node");

    RSS(ref_geom_supported(ref_geom, node, &has_support), "empty");
    RAS(REF_TRUE == has_support, "missing support");

    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* is a */
    REF_GEOM ref_geom;
    REF_INT node, type, id;
    REF_DBL *params;
    REF_BOOL it_is;

    RSS(ref_geom_create(&ref_geom), "create");

    node = 2;
    type = REF_GEOM_NODE;
    id = 5;
    params = NULL;

    RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_NODE, &it_is), "empty");
    RAS(REF_FALSE == it_is, "expected nothing");

    REIS(0, ref_geom_add(ref_geom, node, type, id, params), "add node");

    RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_NODE, &it_is), "has node");
    RAS(REF_TRUE == it_is, "expected node");
    RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_FACE, &it_is), "empty face");
    RAS(REF_FALSE == it_is, "expected no face");

    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* determine unique id */
    REF_GEOM ref_geom;
    REF_INT node;
    REF_INT type, id;
    REF_DBL params[2];

    RSS(ref_geom_create(&ref_geom), "create");

    type = REF_GEOM_EDGE;
    node = 0;

    REIS(REF_NOT_FOUND, ref_geom_unique_id(ref_geom, node, type, &id),
         "found nothing");

    id = 5;
    params[0] = 15.0;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "node edge 5");

    id = REF_EMPTY;
    RSS(ref_geom_unique_id(ref_geom, node, type, &id), "found it");
    REIS(5, id, "expected 5");

    id = 7;
    params[0] = 17.0;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "node edge 7");
    REIS(REF_INVALID, ref_geom_unique_id(ref_geom, node, type, &id),
         "two found");

    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* system transform */
    REF_DBL tol = 1.0e-12;
    REF_DBL duv[6];
    REF_DBL r[3], s[3], n[3], drsduv[4];
    duv[0] = 2.0;
    duv[1] = 0.0;
    duv[2] = 0.0;
    duv[3] = 3.0;
    duv[4] = 4.0;
    duv[5] = 0.0;

    RSS(ref_geom_uv_rsn(duv, r, s, n, drsduv), "make orthog");
    RWDS(1.0, r[0], tol, "r[0]");
    RWDS(0.0, r[1], tol, "r[1]");
    RWDS(0.0, r[2], tol, "r[2]");
    RWDS(0.0, s[0], tol, "s[0]");
    RWDS(1.0, s[1], tol, "s[1]");
    RWDS(0.0, s[2], tol, "s[2]");
    RWDS(0.0, n[0], tol, "n[0]");
    RWDS(0.0, n[1], tol, "n[1]");
    RWDS(1.0, n[2], tol, "n[2]");

    RWDS(0.5, drsduv[0], tol, "drdu");
    RWDS(0.0, drsduv[1], tol, "drdv");
    RWDS(-0.375, drsduv[2], tol, "dsdu");
    RWDS(0.25, drsduv[3], tol, "dsdv");
  }

  { /* egads lite data is zero size and null */
    REF_GEOM ref_geom;
    RSS(ref_geom_create(&ref_geom), "create");
    REIS(0, ref_geom_cad_data_size(ref_geom), "zero length");
    RAS(NULL == (void *)ref_geom_cad_data(ref_geom), "null init");
    RSS(ref_geom_free(ref_geom), "free");
  }

  RSS(ref_mpi_free(ref_mpi), "free");
  return 0;
}
