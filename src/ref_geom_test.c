
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

#include "ref_geom.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ref_adj.h"
#include "ref_args.h"
#include "ref_cell.h"
#include "ref_dict.h"
#include "ref_edge.h"
#include "ref_egads.h"
#include "ref_export.h"
#include "ref_fixture.h"
#include "ref_grid.h"
#include "ref_histogram.h"
#include "ref_import.h"
#include "ref_list.h"
#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_matrix.h"
#include "ref_metric.h"
#include "ref_mpi.h"
#include "ref_node.h"
#include "ref_part.h"
#include "ref_sort.h"
#include "ref_validation.h"

int main(int argc, char *argv[]) {
  REF_MPI ref_mpi;
  REF_INT pos = REF_EMPTY;

  RSS(ref_mpi_start(argc, argv), "start");
  RSS(ref_mpi_create(&ref_mpi), "create");

  RXS(ref_args_find(argc, argv, "--lite", &pos), REF_NOT_FOUND, "arg search");
  if (pos != REF_EMPTY) {
    REF_GRID ref_grid;
    REIS(3, argc, "required args: --lite grid.meshb");
    REIS(1, pos, "required args: --lite grid.meshb");
    printf("import grid %s\n", argv[2]);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[2]), "argv import");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "grid import");
    RAS(0 < ref_geom_cad_data_size(ref_grid_geom(ref_grid)),
        "no meshb egadslite");
    printf("load egadslite\n");
    RSS(ref_egads_load(ref_grid_geom(ref_grid), NULL), "load egads");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "geom load");
    printf("write tec %s\n", "ref_geom_viz.tec");
    RSS(ref_geom_tec(ref_grid, "ref_geom_viz.tec"), "geom tec");
    RSS(ref_geom_curve_tec(ref_grid, "ref_geom_curve_viz.tec"), "crv tec");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "geom tec");
    RSS(ref_grid_free(ref_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  RXS(ref_args_find(argc, argv, "--enrich2", &pos), REF_NOT_FOUND,
      "arg search");
  if (pos != REF_EMPTY) {
    REF_GRID ref_grid;
    RAS(argc >= 3, "required args: --enrich2 grid.ext geom.egads");
    REIS(1, pos, "required args: --enrich2 grid.ext geom.egads");
    printf("import grid %s\n", argv[2]);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[2]), "argv import");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "grid import");
    if (argc > 3) {
      printf("load geom %s\n", argv[3]);
      RSS(ref_egads_load(ref_grid_geom(ref_grid), argv[3]), "ld egads");
      ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "geom load");
      RSS(ref_egads_mark_jump_degen(ref_grid), "T and UV jumps; UV degen");
      ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "mark jump degen");
    }
    printf("enrich2\n");
    RSS(ref_geom_enrich2(ref_grid), "enrich2");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "enrich2");
    printf("write tec %s\n", "ref_geom_enrich2.tec");
    RSS(ref_export_by_extension(ref_grid, "ref_geom_enrich2.meshb"), "tec");
    RSS(ref_export_by_extension(ref_grid, "ref_geom_enrich2.tec"), "tec");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "export");
    if (argc > 3) {
      REF_GRID freeable_ref_grid;
      REF_FACELIFT ref_facelift;
      printf("original\n");
      RSS(ref_geom_verify_param(ref_grid), "geom param");
      RSS(ref_grid_deep_copy(&freeable_ref_grid, ref_grid), "deep copy");
      RSS(ref_facelift_create(&ref_facelift, freeable_ref_grid, REF_TRUE),
          "create");
      ref_geom_facelift(ref_grid_geom(ref_grid)) = ref_facelift;
      RSS(ref_geom_constrain_all(ref_grid), "constrain");
      printf("watertight\n");
      RSS(ref_geom_verify_param(ref_grid), "geom param");
    }
    RSS(ref_grid_free(ref_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  RXS(ref_args_find(argc, argv, "--enrich3", &pos), REF_NOT_FOUND,
      "arg search");
  if (pos != REF_EMPTY) {
    REF_GRID ref_grid;
    REIS(4, argc, "required args: --enrich3 grid.ext geom.egads");
    REIS(1, pos, "required args: --enrich3 grid.ext geom.egads");
    printf("import grid %s\n", argv[2]);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[2]), "argv import");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "grid import");
    printf("load geom %s\n", argv[3]);
    RSS(ref_egads_load(ref_grid_geom(ref_grid), argv[3]), "ld egads");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "geom load");
    RSS(ref_egads_mark_jump_degen(ref_grid), "T and UV jumps; UV degen");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "mark jump degen");
    printf("enrich3\n");
    RSS(ref_geom_enrich3(ref_grid), "enrich3");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "enrich3");
    printf("write tec %s\n", "ref_geom_enrich3.tec");
    RSS(ref_export_by_extension(ref_grid, "ref_geom_enrich3.tec"), "tec");
    RSS(ref_export_by_extension(ref_grid, "ref_geom_enrich3.meshb"), "tec");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "export");
    {
      REF_GRID freeable_ref_grid;
      REF_FACELIFT ref_facelift;
      printf("original\n");
      RSS(ref_geom_verify_param(ref_grid), "geom param");
      RSS(ref_grid_deep_copy(&freeable_ref_grid, ref_grid), "deep copy");
      RSS(ref_facelift_create(&ref_facelift, freeable_ref_grid, REF_TRUE),
          "create");
      ref_geom_facelift(ref_grid_geom(ref_grid)) = ref_facelift;
      RSS(ref_geom_constrain_all(ref_grid), "constrain");
      printf("watertight\n");
      RSS(ref_geom_verify_param(ref_grid), "geom param");
    }
    RSS(ref_grid_free(ref_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  RXS(ref_args_find(argc, argv, "--viz", &pos), REF_NOT_FOUND, "arg search");
  if (pos != REF_EMPTY) {
    REF_GRID ref_grid;
    REIS(4, argc, "required args: --viz grid.ext geom.egads");
    REIS(1, pos, "required args: --viz grid.ext geom.egads");
    printf("import grid %s\n", argv[2]);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[2]), "argv import");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "grid import");
    printf("load geom %s\n", argv[3]);
    RSS(ref_egads_load(ref_grid_geom(ref_grid), argv[3]), "ld egads");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "geom load");
    printf("write tec %s\n", "ref_geom_viz.tec");
    RSS(ref_geom_tec(ref_grid, "ref_geom_viz.tec"), "geom tec");
    RSS(ref_geom_curve_tec(ref_grid, "ref_geom_curve_viz.tec"), "crv tec");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "geom tec");
    RSS(ref_grid_free(ref_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  RXS(ref_args_find(argc, argv, "--face-norm", &pos), REF_NOT_FOUND,
      "arg search");
  if (pos != REF_EMPTY) {
    REF_GRID ref_grid;
    REF_INT faceid;
    REIS(5, argc, "required args: --face-norm grid.ext geom.egads faceid");
    REIS(1, pos, "required args: --face_norm grid.ext geom.egads faceid");
    printf("import grid %s\n", argv[2]);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[2]), "argv import");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "grid import");
    printf("load geom %s\n", argv[3]);
    RSS(ref_egads_load(ref_grid_geom(ref_grid), argv[3]), "ld egads");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "geom load");
    faceid = atoi(argv[4]);
    printf("faceid %d\n", faceid);
    {
      REF_GEOM ref_geom = ref_grid_geom(ref_grid);
      REF_INT geom;
      REF_DBL r[3], s[3], n[3], area_sign;
      each_ref_geom_face(ref_geom, geom) {
        if (ref_geom_id(ref_geom, geom) == faceid) {
          RAISE(ref_geom_face_rsn(
              ref_geom, faceid, &(ref_geom_param(ref_geom, 0, geom)), r, s, n));
          RSS(ref_geom_uv_area_sign(ref_grid, faceid, &area_sign), "a sign");
          n[0] *= area_sign;
          n[1] *= area_sign;
          n[2] *= area_sign;
          printf("%8.5f %8.5f %8.5f\n", n[0], n[1], n[2]);
        }
      }
    }
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "done.");
    RSS(ref_grid_free(ref_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  RXS(ref_args_find(argc, argv, "--projection", &pos), REF_NOT_FOUND,
      "arg search");
  if (pos != REF_EMPTY) {
    REF_GRID ref_grid;
    REF_GEOM ref_geom;
    REF_INT i, j, item, id;
    REF_DBL xyz[3], ws, fs, uv[2];
    FILE *file = NULL;
    REF_BOOL verbose = REF_FALSE;
    REF_LIST ref_list;
    REF_INT nface, faceid, best_face;
    REF_DBL dist, best_dist, best_xyz[3], face_xyz[3];

    REIS(4, argc,
         "required args: --projection project.egads wing-station.input");
    REIS(1, pos,
         "required args: --projection project.egads wing-station.input");

    RSS(ref_grid_create(&ref_grid, ref_mpi), "empty grid");
    ref_geom = ref_grid_geom(ref_grid);
    printf("load geom %s\n", argv[2]);
    RSS(ref_egads_load(ref_geom, argv[2]), "ld egads");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "grid load");
    file = fopen(argv[3], "r");
    if (NULL == (void *)file) printf("unable to open %s\n", argv[3]);
    RNS(file, "unable to open file");
    RSS(ref_list_create(&ref_list), "make list");
    REIS(1, fscanf(file, "%d", &nface), "read nface");
    for (i = 0; i < nface; i++) {
      REIS(1, fscanf(file, "%d", &faceid), "read faceid");
      RSS(ref_list_push(ref_list, faceid), "push id");
    }
    for (i = 0; i < 100; i++) {
      REIS(2, fscanf(file, "%lf %lf", &ws, &fs), "read xy");
      if (verbose) printf("fs %f ws %f\n", fs, ws);
      xyz[2] = 120;
      for (j = 0; j < 10; j++) {
        xyz[0] = fs;
        if (ws > 0) {
          xyz[1] = 65.0 + cos(ref_math_in_radians(6.0)) * (ws - 65.0);
        } else {
          xyz[1] = -65.0 + cos(ref_math_in_radians(6.0)) * (ws + 65.0);
        }
        if (verbose) printf("seed %f %f %f\n", xyz[0], xyz[1], xyz[2]);

        best_dist = REF_DBL_MAX;
        best_face = REF_EMPTY;
        best_xyz[0] = 0;
        best_xyz[1] = 0;
        best_xyz[2] = 0;
        each_ref_list_item(ref_list, item) {
          id = ref_list_value(ref_list, item);
          uv[0] = 0.5;
          uv[1] = 0.5;
          face_xyz[0] = xyz[0];
          face_xyz[1] = xyz[1];
          face_xyz[2] = xyz[2];
          RSS(ref_egads_inverse_eval(ref_geom, REF_GEOM_FACE, id, face_xyz, uv),
              "inv");
          RSS(ref_egads_eval_at(ref_geom, REF_GEOM_FACE, id, uv, face_xyz,
                                NULL),
              "eval at");
          dist =
              sqrt(pow(face_xyz[0] - xyz[0], 2) + pow(face_xyz[1] - xyz[1], 2));
          if (dist < best_dist) {
            best_dist = dist;
            best_face = id;
            best_xyz[0] = face_xyz[0];
            best_xyz[1] = face_xyz[1];
            best_xyz[2] = face_xyz[2];
          }
        }
        RUS(REF_EMPTY, best_face, "best face not found");
        xyz[0] = best_xyz[0];
        xyz[1] = best_xyz[1];
        xyz[2] = best_xyz[2];
        if (verbose) {
          printf("%d xyz %f %f %f uv %f %f eval\n", j, xyz[0], xyz[1], xyz[2],
                 uv[0], uv[1]);
          if (ws > 0) {
            printf("diff %f %f\n", xyz[0] - fs,
                   65.0 + cos(ref_math_in_radians(6.0)) * (ws - 65.0) - xyz[1]);
          } else {
            printf(
                "diff %f %f\n", xyz[0] - fs,
                -65.0 + cos(ref_math_in_radians(6.0)) * (ws + 65.0) - xyz[1]);
          }
        }
      }
      printf("%f %f %f\n", xyz[0], xyz[1], xyz[2]);
    }
    fclose(file);
    RSS(ref_list_free(ref_list), "free");
    RSS(ref_grid_free(ref_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  RXS(ref_args_find(argc, argv, "--triage", &pos), REF_NOT_FOUND, "arg search");
  if (pos != REF_EMPTY) {
    REF_GRID ref_grid;
    if (4 > argc) {
      printf("required args: --triage grid.ext geom.egads");
      RSS(ref_mpi_free(ref_mpi), "free");
      RSS(ref_mpi_stop(), "stop");
      return REF_FAILURE;
    }
    REIS(1, pos, "required args: --triage grid.ext geom.egads");
    printf("grid source %s\n", argv[2]);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[2]), "argv import");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "grid load");
    RSS(ref_egads_load(ref_grid_geom(ref_grid), argv[3]), "ld egads");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "geom load");
    RSS(ref_metric_interpolated_curvature(ref_grid), "interp curve");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "curvature");
    RSS(ref_geom_verify_param(ref_grid), "verify param");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "verify param");
    {
      REF_GEOM ref_geom = ref_grid_geom(ref_grid);
      REF_DBL tol;
      REF_INT id = REF_EMPTY;
      REF_INT min_id, max_id;
      REF_DBL min_tol, max_tol;
      RSS(ref_egads_tolerance(ref_geom, REF_GEOM_BODY, id, &tol), "solid tol");
      printf("%e body tolerance\n", tol);

      min_id = REF_EMPTY;
      max_id = REF_EMPTY;
      min_tol = -1.0;
      max_tol = -1.0;
      each_ref_geom_node_id(ref_geom, id) {
        RSS(ref_egads_tolerance(ref_geom, REF_GEOM_NODE, id, &tol), "node tol");
        if (REF_EMPTY == min_id || tol < min_tol) {
          min_id = id;
          min_tol = tol;
        }
        if (REF_EMPTY == max_id || tol > max_tol) {
          max_id = id;
          max_tol = tol;
        }
      }
      printf("%d node id %e tolerance\n%d node id %e tolerance\n", min_id,
             min_tol, max_id, max_tol);

      min_id = REF_EMPTY;
      max_id = REF_EMPTY;
      min_tol = -1.0;
      max_tol = -1.0;
      each_ref_geom_edge_id(ref_geom, id) {
        RSS(ref_egads_tolerance(ref_geom, REF_GEOM_EDGE, id, &tol), "edge tol");
        if (REF_EMPTY == min_id || tol < min_tol) {
          min_id = id;
          min_tol = tol;
        }
        if (REF_EMPTY == max_id || tol > max_tol) {
          max_id = id;
          max_tol = tol;
        }
      }
      printf("%d edge id %e tolerance\n%d edge id %e tolerance\n", min_id,
             min_tol, max_id, max_tol);

      min_id = REF_EMPTY;
      max_id = REF_EMPTY;
      min_tol = -1.0;
      max_tol = -1.0;
      each_ref_geom_face_id(ref_geom, id) {
        RSS(ref_egads_tolerance(ref_geom, REF_GEOM_FACE, id, &tol), "face tol");
        if (REF_EMPTY == min_id || tol < min_tol) {
          min_id = id;
          min_tol = tol;
        }
        if (REF_EMPTY == max_id || tol > max_tol) {
          max_id = id;
          max_tol = tol;
        }
      }
      printf("%d face id %e tolerance\n%d face id %e tolerance\n", min_id,
             min_tol, max_id, max_tol);
    }
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "report tol");

    {
      REF_GEOM ref_geom = ref_grid_geom(ref_grid);
      REF_NODE ref_node = ref_grid_node(ref_grid);
      REF_INT geom, id, type, node;
      REF_DBL xyz[3];
      REF_DBL hr, r[3], hs, s[3], hn, n[3];
      REF_DBL tol;
      each_ref_geom(ref_geom, geom) {
        node = ref_geom_node(ref_geom, geom);
        type = ref_geom_type(ref_geom, geom);
        id = ref_geom_id(ref_geom, geom);
        xyz[0] = ref_node_xyz(ref_node, 0, node);
        xyz[1] = ref_node_xyz(ref_node, 1, node);
        xyz[2] = ref_node_xyz(ref_node, 2, node);
        RSS(ref_egads_feature_size(ref_grid, node, &hr, r, &hs, s, &hn, n),
            "get feature size");
        RSS(ref_egads_tolerance(ref_geom, type, id, &tol), "face tol");
        if (hr < tol) {
          printf("type %d id %d node %d xyz %f %f %f h %.3e tol %.3e\n", type,
                 id, node, xyz[0], xyz[1], xyz[2], hr, tol);
        }
      }
    }
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "feature size");

    {
      REF_GEOM ref_geom = ref_grid_geom(ref_grid);
      REF_NODE ref_node = ref_grid_node(ref_grid);
      REF_INT geom, id, type, node;
      REF_DBL xyz[3];
      REF_DBL kr, r[3];
      REF_DBL tol;
      REF_DBL drad, hmax, rlimit, hr;

      drad = 1.0 / ref_geom_segments_per_radian_of_curvature(ref_geom);
      RSS(ref_egads_diagonal(ref_geom, REF_EMPTY, &hmax), "bbox diag");
      hmax *= 0.1;          /* normal spacing and max tangential spacing */
      rlimit = hmax / drad; /* h = r*drad, r = h/drad */

      each_ref_geom_edge(ref_geom, geom) {
        node = ref_geom_node(ref_geom, geom);
        type = ref_geom_type(ref_geom, geom);
        id = ref_geom_id(ref_geom, geom);
        xyz[0] = ref_node_xyz(ref_node, 0, node);
        xyz[1] = ref_node_xyz(ref_node, 1, node);
        xyz[2] = ref_node_xyz(ref_node, 2, node);
        RSS(ref_egads_edge_curvature(ref_geom, geom, &kr, r), "curve");
        kr = ABS(kr);
        hr = hmax;
        if (1.0 / rlimit < kr) hr = drad / kr;

        RSS(ref_egads_tolerance(ref_geom, type, id, &tol), "edge tol");
        if (hr < ref_geom_tolerance_protection(ref_geom) * tol) {
          printf("id %d node %d xyz %f %f %f edge hr %.3e tol %.3e\n", id, node,
                 xyz[0], xyz[1], xyz[2], hr, tol);
        }
      }
    }
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "edge curvature");

    {
      REF_GEOM ref_geom = ref_grid_geom(ref_grid);
      REF_NODE ref_node = ref_grid_node(ref_grid);
      REF_INT geom, id, type, node;
      REF_DBL xyz[3];
      REF_DBL kr, r[3], ks, s[3];
      REF_DBL tol;
      REF_DBL drad, hmax, rlimit, hr, hs;

      drad = 1.0 / ref_geom_segments_per_radian_of_curvature(ref_geom);
      RSS(ref_egads_diagonal(ref_geom, REF_EMPTY, &hmax), "bbox diag");
      hmax *= 0.1;          /* normal spacing and max tangential spacing */
      rlimit = hmax / drad; /* h = r*drad, r = h/drad */

      each_ref_geom_face(ref_geom, geom) {
        node = ref_geom_node(ref_geom, geom);
        type = ref_geom_type(ref_geom, geom);
        id = ref_geom_id(ref_geom, geom);
        xyz[0] = ref_node_xyz(ref_node, 0, node);
        xyz[1] = ref_node_xyz(ref_node, 1, node);
        xyz[2] = ref_node_xyz(ref_node, 2, node);
        RSS(ref_egads_face_curvature(ref_geom, geom, &kr, r, &ks, s), "curve");
        kr = ABS(kr);
        hr = hmax;
        if (1.0 / rlimit < kr) hr = drad / kr;
        ks = ABS(ks);
        hs = hmax;
        if (1.0 / rlimit < ks) hs = drad / ks;

        RSS(ref_egads_tolerance(ref_geom, type, id, &tol), "face tol");
        if (hr < ref_geom_tolerance_protection(ref_geom) * tol ||
            hs < ref_geom_tolerance_protection(ref_geom) * tol) {
          printf("id %d node %d xyz %f %f %f hr %.3e hs %.3e tol %.3e\n", id,
                 node, xyz[0], xyz[1], xyz[2], hr, hs, tol);
        }
      }
    }
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "face curvature");

    RSS(ref_grid_free(ref_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "free");
    return 0;
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
    RSS(ref_geom_remove_all(ref_geom, node), "ok with nothing there");
    REIS(2, ref_geom_n(ref_geom), "items");

    node = 2;
    RSS(ref_geom_remove_all(ref_geom, node), "remove all at node");
    REIS(0, ref_geom_n(ref_geom), "items");
    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* add and remove geom */
    REF_GEOM ref_geom;
    REF_INT node, type, id, geom;
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

    RSS(ref_geom_find(ref_geom, node, type, id, &geom), "not found");
    RSS(ref_geom_remove(ref_geom, geom), "remove");
    REIS(1, ref_geom_n(ref_geom), "items");

    REIS(REF_INVALID, ref_geom_remove(ref_geom, -1), "items");
    REIS(REF_INVALID, ref_geom_remove(ref_geom, ref_geom_max(ref_geom)),
         "items");
    REIS(REF_INVALID, ref_geom_remove(ref_geom, ref_geom_max(ref_geom) - 1),
         "items");

    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* add and remove geom without element */
    REF_GRID ref_grid;
    REF_GEOM ref_geom;
    REF_INT node, type, id, geom;
    REF_DBL params[2];
    REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];

    RSS(ref_grid_create(&ref_grid, ref_mpi), "create");
    ref_geom = ref_grid_geom(ref_grid);

    node = 2;
    type = REF_GEOM_FACE;
    id = 5;
    params[0] = 11.0;
    params[1] = 21.0;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "add face");
    nodes[0] = node;
    nodes[1] = node + 1;
    nodes[2] = node + 2;
    nodes[3] = id;
    RSS(ref_cell_add(ref_grid_tri(ref_grid), nodes, &cell), "add tri");

    node = 10;
    type = REF_GEOM_EDGE;
    id = 2;
    params[0] = 5.0;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "add edge");

    REIS(2, ref_geom_n(ref_geom), "items");
    RSS(ref_geom_remove_without_cell(ref_grid, node), "remove");
    REIS(1, ref_geom_n(ref_geom), "items");

    REIS(REF_NOT_FOUND, ref_geom_find(ref_geom, node, type, id, &geom),
         "not found");

    RSS(ref_grid_free(ref_grid), "free");
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

    RSS(ref_geom_add_between(ref_grid, node0, node1, 0.5, new_node), "between");
    id = 5;
    REIS(REF_NOT_FOUND, ref_geom_find(ref_geom, new_node, type, id, &geom),
         "what edge");

    nodes[0] = node0;
    nodes[1] = node1;
    nodes[2] = id;
    RSS(ref_cell_add(ref_grid_edg(ref_grid), nodes, &cell), "add edge");
    RSS(ref_geom_add_between(ref_grid, node0, node1, 0.5, new_node), "between");
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

    RSS(ref_geom_add_between(ref_grid, node0, node1, 0.5, new_node), "between");
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
    REIS(REF_INVALID, ref_egads_eval(ref_geom, geom, xyz, NULL),
         "invalid geom");
    geom = 1 + ref_geom_max(ref_geom);
    REIS(REF_INVALID, ref_egads_eval(ref_geom, geom, xyz, NULL),
         "invalid geom");

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

  { /* uv tri bary and bounding sphere */
    REF_GEOM ref_geom;
    REF_INT node;
    REF_INT type, id;
    REF_DBL params[2];
    REF_INT nodes[4];
    REF_DBL uv[2];
    REF_DBL bary[3];
    REF_DBL center[2], radius;

    RSS(ref_geom_create(&ref_geom), "create");

    type = REF_GEOM_FACE;
    id = 5;
    node = 0;
    params[0] = 0.0;
    params[1] = 0.0;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "node face");
    node = 1;
    params[0] = 1.0;
    params[1] = 0.0;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "node face");
    node = 2;
    params[0] = 1.0;
    params[1] = 10.0;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "node face");

    nodes[0] = 0;
    nodes[1] = 1;
    nodes[2] = 2;
    nodes[3] = id;

    RSS(ref_geom_tri_uv_bounding_sphere3(ref_geom, nodes, center, &radius),
        "bary");
    RWDS(2.0 / 3.0, center[0], -1.0, "c0");
    RWDS(10.0 / 3.0, center[1], -1.0, "c1");
    RWDS(sqrt(pow(1 - 2.0 / 3.0, 2) + pow(10 - 10.0 / 3.0, 2)), radius, -1.0,
         "r");

    uv[0] = 1.0;
    uv[1] = 5.0;
    RSS(ref_geom_bary3(ref_geom, nodes, uv, bary), "bary");
    RWDS(0.0, bary[0], -1.0, "b0");
    RWDS(0.5, bary[1], -1.0, "b1");
    RWDS(0.5, bary[2], -1.0, "b2");

    uv[0] = 0.5;
    uv[1] = 5.0;
    RSS(ref_geom_bary3(ref_geom, nodes, uv, bary), "bary");
    RWDS(0.5, bary[0], -1.0, "b0");
    RWDS(0.0, bary[1], -1.0, "b1");
    RWDS(0.5, bary[2], -1.0, "b2");

    uv[0] = 0.5;
    uv[1] = 0.0;
    RSS(ref_geom_bary3(ref_geom, nodes, uv, bary), "bary");
    RWDS(0.5, bary[0], -1.0, "b0");
    RWDS(0.5, bary[1], -1.0, "b1");
    RWDS(0.0, bary[2], -1.0, "b2");

    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* t edg bary and bounding sphere */
    REF_GEOM ref_geom;
    REF_INT node;
    REF_INT type, id;
    REF_DBL params;
    REF_INT nodes[3];
    REF_DBL t;
    REF_DBL bary[2];
    REF_DBL center, radius;

    RSS(ref_geom_create(&ref_geom), "create");

    type = REF_GEOM_EDGE;
    id = 15;
    node = 0;
    params = 0.0;
    RSS(ref_geom_add(ref_geom, node, type, id, &params), "node face");
    node = 1;
    params = 1.0;
    RSS(ref_geom_add(ref_geom, node, type, id, &params), "node face");

    nodes[0] = 0;
    nodes[1] = 1;
    nodes[2] = id;

    RSS(ref_geom_edg_t_bounding_sphere2(ref_geom, nodes, &center, &radius),
        "bary");
    RWDS(0.5, center, -1.0, "c0");
    RWDS(0.5, radius, -1.0, "r");

    t = 1.0;
    RSS(ref_geom_bary2(ref_geom, nodes, t, bary), "bary");
    RWDS(0.0, bary[0], -1.0, "b0");
    RWDS(1.0, bary[1], -1.0, "b1");

    t = 0.0;
    RSS(ref_geom_bary2(ref_geom, nodes, t, bary), "bary");
    RWDS(1.0, bary[0], -1.0, "b0");
    RWDS(0.0, bary[1], -1.0, "b1");

    t = 0.4;
    RSS(ref_geom_bary2(ref_geom, nodes, t, bary), "bary");
    RWDS(0.6, bary[0], -1.0, "b0");
    RWDS(0.4, bary[1], -1.0, "b1");

    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* uv area sliver */
    REF_GEOM ref_geom;
    REF_INT node, type, id, nodes[4];
    REF_DBL params[2];
    REF_DBL uv_area;
    RSS(ref_geom_create(&ref_geom), "create");
    type = REF_GEOM_FACE;
    id = 5;
    nodes[3] = id;

    node = 0;
    nodes[0] = node;
    params[0] = 1.000018525523032849e+03;
    params[1] = 9.999975080140594628e+02;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "add face");

    node = 1;
    nodes[1] = node;
    params[0] = 1.000018524308897440e+03;
    params[1] = 9.999975166749544542e+02;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "add face");

    node = 2;
    nodes[2] = node;
    params[0] = 1.000018536245624318e+03;
    params[1] = 9.999975323547345170e+02;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "add face");

    RSS(ref_geom_uv_area(ref_geom, nodes, &uv_area), "uv area");
    /* good to 2e-18 Apple clang 11*/
    RWDS(-6.121006e-11, uv_area, 1e-16, "area");

    RSS(ref_geom_free(ref_geom), "free");
  }

  { /* uv area right */
    REF_GEOM ref_geom;
    REF_INT node, type, id, nodes[4];
    REF_DBL params[2];
    REF_DBL uv_area;
    RSS(ref_geom_create(&ref_geom), "create");
    type = REF_GEOM_FACE;
    id = 5;
    nodes[3] = id;

    node = 0;
    nodes[0] = node;
    params[0] = 0.0;
    params[1] = 0.0;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "add face");

    node = 1;
    nodes[1] = node;
    params[0] = 1.0;
    params[1] = 0.0;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "add face");

    node = 2;
    nodes[2] = node;
    params[0] = 0.0;
    params[1] = 1.0;
    RSS(ref_geom_add(ref_geom, node, type, id, params), "add face");

    RSS(ref_geom_uv_area(ref_geom, nodes, &uv_area), "uv area");
    RWDS(0.5, uv_area, -1.0, "area");

    RSS(ref_geom_free(ref_geom), "free");
  }

  if (1 == ref_mpi_n(ref_mpi)) {
    REF_GRID ref_grid;
    RSS(ref_fixture_tet_grid(&ref_grid, ref_mpi), "tet fix");
    RSS(ref_geom_enrich2(ref_grid), "enrich2");
    REIS(1, ref_cell_n(ref_grid_ed2(ref_grid)), "ed2");
    REIS(1, ref_cell_n(ref_grid_tr2(ref_grid)), "tr2");
    REIS(1, ref_cell_n(ref_grid_te2(ref_grid)), "te2");
    ref_grid_free(ref_grid);
  }

  {
    REF_INT degree = 1;
    REF_INT n_control_points = 2;
    REF_DBL knots[] = {0, 0, 1, 1};
    REF_DBL t;
    REF_INT span;
    REIS(4, ref_geom_bspline_nknot(degree, n_control_points), "knots");
    t = -1;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(1, span, "wrong span");
    t = 0;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(1, span, "wrong span");
    t = 0.5;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(1, span, "wrong span");
    t = 1.0;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(1, span, "wrong span");
    t = 2.0;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(1, span, "wrong span");
  }

  {
    REF_INT degree = 2;
    REF_INT n_control_points = 3;
    REF_DBL knots[] = {0, 0, 0, 1, 1, 1};
    REF_DBL t;
    REF_INT span;
    REIS(6, ref_geom_bspline_nknot(degree, n_control_points), "knots")
    t = -1;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(2, span, "wrong span");
    t = 0;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(2, span, "wrong span");
    t = 0.5;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(2, span, "wrong span");
    t = 1.0;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(2, span, "wrong span");
    t = 2.0;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(2, span, "wrong span");
  }

  {
    REF_INT degree = 3;
    REF_INT n_control_points = 4;
    REF_DBL knots[] = {0, 0, 0, 0, 1, 1, 1, 1};
    REF_DBL t;
    REF_INT span;
    REIS(8, ref_geom_bspline_nknot(degree, n_control_points), "knots");
    t = -1;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(3, span, "wrong span");
    t = 0;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(3, span, "wrong span");
    t = 0.5;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(3, span, "wrong span");
    t = 1.0;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(3, span, "wrong span");
    t = 2.0;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(3, span, "wrong span");
  }

  {
    REF_INT degree = 2;
    REF_INT n_control_points = 8;
    REF_DBL knots[] = {0, 0, 0, 1, 2, 3, 4, 4, 5, 5, 5};
    REF_DBL t;
    REF_INT span;
    REIS(11, ref_geom_bspline_nknot(degree, n_control_points), "knots");
    t = -1;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(2, span, "wrong span");
    t = 0;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(2, span, "wrong span");
    t = 0.5;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(2, span, "wrong span");
    t = 1.0;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(3, span, "wrong span");
    t = 2.5;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(4, span, "wrong span");
    t = 4.0;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(7, span, "wrong span");
    t = 5;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(7, span, "wrong span");
    t = 6;
    RSS(ref_geom_bspline_span_index(degree, n_control_points, knots, t, &span),
        "index");
    REIS(7, span, "wrong span");
  }

  {
    REF_INT degree = 1;
    REF_DBL knots[] = {0, 0, 1, 1};
    REF_DBL t;
    REF_INT span;
    REF_DBL N[2];
    REF_DBL tol = -1.0;
    t = 0.5;
    span = 1;
    RSS(ref_geom_bspline_basis(degree, knots, t, span, N), "basis");
    RWDS(1.0 / 2.0, N[0], tol, "N0,1");
    RWDS(1.0 / 2.0, N[1], tol, "N1,1");
  }

  {
    REF_INT degree = 2;
    REF_DBL knots[] = {0, 0, 0, 1, 2, 3, 4, 4, 5, 5, 5};
    REF_DBL t;
    REF_INT span;
    REF_DBL N[3];
    REF_DBL tol = -1.0;
    t = 2.5;
    span = 4;
    RSS(ref_geom_bspline_basis(degree, knots, t, span, N), "basis");
    RWDS(1.0 / 8.0, N[0], tol, "N2,2");
    RWDS(6.0 / 8.0, N[1], tol, "N3,2");
    RWDS(1.0 / 8.0, N[2], tol, "N4,2");
  }

  {
    REF_INT degree = 1;
    REF_INT n_control_points = 2;
    REF_DBL knots[] = {0, 0, 1, 1};
    REF_DBL N[2];
    REF_DBL t;
    REF_DBL tol = -1.0;
    REIS(4, ref_geom_bspline_nknot(degree, n_control_points), "knots");
    t = 0.0;
    RSS(ref_geom_bspline_row(degree, n_control_points, knots, t, N), "row");
    RWDS(1.0, N[0], tol, "N0,1");
    RWDS(0.0, N[1], tol, "N1,1");
    t = 1.0 / 3.0;
    RSS(ref_geom_bspline_row(degree, n_control_points, knots, t, N), "row");
    RWDS(2.0 / 3.0, N[0], tol, "N0,1");
    RWDS(1.0 / 3.0, N[1], tol, "N1,1");
    t = 1.0;
    RSS(ref_geom_bspline_row(degree, n_control_points, knots, t, N), "row");
    RWDS(0.0, N[0], tol, "N0,1");
    RWDS(1.0, N[1], tol, "N1,1");
  }

  {
    REF_INT degree = 1;
    REF_INT n_control_points = 3;
    REF_DBL knots[] = {0, 0, 1, 2, 2};
    REF_DBL N[3];
    REF_DBL t;
    REF_DBL tol = -1.0;
    REIS(5, ref_geom_bspline_nknot(degree, n_control_points), "knots");
    t = 0.0;
    RSS(ref_geom_bspline_row(degree, n_control_points, knots, t, N), "row");
    RWDS(1.0, N[0], tol, "N0,1");
    RWDS(0.0, N[1], tol, "N1,1");
    RWDS(0.0, N[2], tol, "N2,1");
    t = 1.0;
    RSS(ref_geom_bspline_row(degree, n_control_points, knots, t, N), "row");
    RWDS(0.0, N[0], tol, "N0,1");
    RWDS(1.0, N[1], tol, "N1,1");
    RWDS(0.0, N[2], tol, "N2,1");
    t = 2.0;
    RSS(ref_geom_bspline_row(degree, n_control_points, knots, t, N), "row");
    RWDS(0.0, N[0], tol, "N0,1");
    RWDS(0.0, N[1], tol, "N1,1");
    RWDS(1.0, N[2], tol, "N2,1");
  }

  {
    REF_INT degree = 2;
    REF_INT n_control_points = 8;
    REF_DBL knots[] = {0, 0, 0, 1, 2, 3, 4, 4, 5, 5, 5};
    REF_DBL N[8];
    REF_DBL t;
    REF_DBL tol = -1.0;
    REIS(11, ref_geom_bspline_nknot(degree, n_control_points), "knots");
    t = 0.0;
    RSS(ref_geom_bspline_row(degree, n_control_points, knots, t, N), "eval");
    RWDS(1.0, N[0], tol, "N0,2");
    RWDS(0.0, N[1], tol, "N1,2");
    RWDS(0.0, N[2], tol, "N2,2");
    RWDS(0.0, N[3], tol, "N3,2");
    RWDS(0.0, N[4], tol, "N4,2");
    RWDS(0.0, N[5], tol, "N5,2");
    RWDS(0.0, N[6], tol, "N6,2");
    RWDS(0.0, N[7], tol, "N7,2");
    t = 4.0;
    RSS(ref_geom_bspline_row(degree, n_control_points, knots, t, N), "eval");
    RWDS(0.0, N[0], tol, "N0,2");
    RWDS(0.0, N[1], tol, "N1,2");
    RWDS(0.0, N[2], tol, "N2,2");
    RWDS(0.0, N[3], tol, "N3,2");
    RWDS(0.0, N[4], tol, "N4,2");
    RWDS(1.0, N[5], tol, "N5,2");
    RWDS(0.0, N[6], tol, "N6,2");
    RWDS(0.0, N[7], tol, "N7,2");
  }

  {
    REF_INT degree = 1;
    REF_INT n_control_points = 2;
    REF_DBL knots[] = {0, 0, 1, 1};
    REF_DBL control_points[] = {4, 8};
    REF_DBL t;
    REF_DBL val;
    REF_DBL tol = -1.0;
    REIS(4, ref_geom_bspline_nknot(degree, n_control_points), "knots");
    t = 0.0;
    RSS(ref_geom_bspline_eval(degree, n_control_points, knots, t,
                              control_points, &val),
        "eval");
    RWDS(4.0, val, tol, "f(0.0)");
    t = 0.25;
    RSS(ref_geom_bspline_eval(degree, n_control_points, knots, t,
                              control_points, &val),
        "eval");
    RWDS(5.0, val, tol, "f(0.25)");
    t = 0.5;
    RSS(ref_geom_bspline_eval(degree, n_control_points, knots, t,
                              control_points, &val),
        "eval");
    RWDS(6.0, val, tol, "f(0.5)");
    t = 0.75;
    RSS(ref_geom_bspline_eval(degree, n_control_points, knots, t,
                              control_points, &val),
        "eval");
    RWDS(7.0, val, tol, "f(0.75)");
    t = 1.0;
    RSS(ref_geom_bspline_eval(degree, n_control_points, knots, t,
                              control_points, &val),
        "eval");
    RWDS(8.0, val, tol, "f(1.0)");
  }

  {
    REF_INT degree = 2;
    REF_INT n_control_points = 8;
    REF_DBL knots[] = {0, 0, 0, 1, 2, 3, 4, 4, 5, 5, 5};
    REF_DBL control_points[] = {1, 1, 1, 1, 1, 1, 1, 1};
    REF_DBL t;
    REF_DBL val;
    REF_DBL tol = -1.0;
    REIS(11, ref_geom_bspline_nknot(degree, n_control_points), "knots");
    t = 0.0;
    RSS(ref_geom_bspline_eval(degree, n_control_points, knots, t,
                              control_points, &val),
        "eval");
    RWDS(1.0, val, tol, "unity");
    t = 2.5;
    RSS(ref_geom_bspline_eval(degree, n_control_points, knots, t,
                              control_points, &val),
        "eval");
    RWDS(1.0, val, tol, "unity");
    t = 3.8;
    RSS(ref_geom_bspline_eval(degree, n_control_points, knots, t,
                              control_points, &val),
        "eval");
    RWDS(1.0, val, tol, "unity");
  }

  RXS(ref_args_find(argc, argv, "--basis", &pos), REF_NOT_FOUND, "arg search");
  /* piegl-tiller nurbs book pg 54 Figure 2.5 */
  if (pos != REF_EMPTY) {
    REF_INT degree = 1;
    REF_INT n_control_points = 7;
    REF_DBL knots[] = {0, 0, 1, 2, 3, 4, 4, 5, 5};
    REIS(9, ref_geom_bspline_nknot(degree, n_control_points), "knots");
    RSS(ref_geom_bspline_row_tec(degree, n_control_points, knots,
                                 "ref_geom_test_basis_repeat_17.tec"),
        "tec basis");
  }
  /* piegl-tiller nurbs book pg 55 Figure 2.6 */
  if (pos != REF_EMPTY) {
    REF_INT degree = 2;
    REF_INT n_control_points = 8;
    REF_DBL knots[] = {0, 0, 0, 1, 2, 3, 4, 4, 5, 5, 5};
    REIS(11, ref_geom_bspline_nknot(degree, n_control_points), "knots");
    RSS(ref_geom_bspline_row_tec(degree, n_control_points, knots,
                                 "ref_geom_test_basis_repeat_28.tec"),
        "tec basis");
  }
  /* piegl-tiller nurbs book pg 63 Figure 2.9(a)*/
  if (pos != REF_EMPTY) {
    REF_INT degree = 3;
    REF_INT n_control_points = 7;
    REF_DBL knots[] = {0, 0, 0, 0, 2, 4, 6, 8, 8, 8, 8};
    REIS(11, ref_geom_bspline_nknot(degree, n_control_points), "knots");
    RSS(ref_geom_bspline_row_tec(degree, n_control_points, knots,
                                 "ref_geom_test_basis_uniform_37.tec"),
        "tec basis");
  }
  /* piegl-tiller nurbs book pg 67 Figure 2.12*/
  if (pos != REF_EMPTY) {
    REF_INT degree = 3;
    REF_INT n_control_points = 7;
    REF_DBL knots[] = {0, 0, 0, 0, 1, 5, 6, 8, 8, 8, 8};
    REIS(11, ref_geom_bspline_nknot(degree, n_control_points), "knots");
    RSS(ref_geom_bspline_row_tec(degree, n_control_points, knots,
                                 "ref_geom_test_basis_nonuniform_37.tec"),
        "tec basis");
  }

  {
    REF_INT degree = 3;
    REF_DBL t[] = {0, 5.0 / 17.0, 9 / 17.0, 14.0 / 17.0, 1.0};
    REF_DBL uv[] = {0, 0, 3, 4, -1, 4, -4, 0, -4, -3};
    REF_INT n_control_points = 5;
    REF_INT nknot;
    REF_DBL *bundle;
    REF_DBL tol = -1.0;
    nknot = ref_geom_bspline_nknot(degree, n_control_points);
    ref_malloc(bundle, nknot + 2 * n_control_points, REF_DBL);
    RSS(ref_geom_bspline_fit(degree, n_control_points, t, uv, bundle), "fit");
    RWDS(0.0, bundle[0], tol, "start");
    RWDS(0.0, bundle[3], tol, "start");
    RWDS(28.0 / 51.0, bundle[4], tol, "mid");
    RWDS(1.0, bundle[5], tol, "end");
    RWDS(1.0, bundle[8], tol, "end");
    if (pos != REF_EMPTY) {
      RSS(ref_geom_bspline_row_tec(degree, n_control_points, bundle,
                                   "ref_geom_test_fit_basis.tec"),
          "tec basis");
      RSS(ref_geom_bspline_bundle_tec(degree, n_control_points, bundle,
                                      "ref_geom_test_fit_bundle.tec"),
          "tec basis");
    }
    ref_free(bundle);
  }

  RSS(ref_mpi_free(ref_mpi), "free");
  RSS(ref_mpi_stop(), "stop");
  return 0;
}
