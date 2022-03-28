
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

#include "ref_recon.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ref_adapt.h"
#include "ref_adj.h"
#include "ref_args.h"
#include "ref_cell.h"
#include "ref_cloud.h"
#include "ref_clump.h"
#include "ref_collapse.h"
#include "ref_dict.h"
#include "ref_edge.h"
#include "ref_export.h"
#include "ref_face.h"
#include "ref_fixture.h"
#include "ref_gather.h"
#include "ref_grid.h"
#include "ref_histogram.h"
#include "ref_import.h"
#include "ref_list.h"
#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_matrix.h"
#include "ref_migrate.h"
#include "ref_mpi.h"
#include "ref_node.h"
#include "ref_part.h"
#include "ref_smooth.h"
#include "ref_sort.h"
#include "ref_split.h"
#include "ref_validation.h"

int main(int argc, char *argv[]) {
  REF_INT pos;
  REF_MPI ref_mpi;
  RSS(ref_mpi_start(argc, argv), "start");
  RSS(ref_mpi_create(&ref_mpi), "create");

  /* Influence of the Numerical Scheme in Predictions of Vortex Interaction
   *   about a Generic Missile Airframe DOI:10.2514/6.2022-1178 */
  RXS(ref_args_find(argc, argv, "--limiter-effect", &pos), REF_NOT_FOUND,
      "arg search");
  if (pos != REF_EMPTY) {
    REF_GRID ref_grid = NULL;
    REF_NODE ref_node = NULL;
    REF_DBL *field, *p, *grad, *limeff;
    REF_INT node, ldim, edge;
    REF_INT ip = 0, ivol = 1, iphi = 2;
    REF_EDGE ref_edge;
    REF_RECON_RECONSTRUCTION reconstruction = REF_RECON_KEXACT;
    REIS(4, argc, "required args: --limiter-effect grid.ext [p,vol,phi].solb");
    REIS(1, pos, "required args: --limiter-effect grid.ext [p,vol,phi].solb");
    if (ref_mpi_once(ref_mpi)) printf("import grid %s\n", argv[2]);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, argv[2]), "argv import");
    if (ref_mpi_once(ref_mpi)) printf("extract ref_node\n");
    ref_node = ref_grid_node(ref_grid);
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "grid import");
    if (ref_mpi_once(ref_mpi)) printf("reading field %s\n", argv[2]);
    RSS(ref_part_scalar(ref_grid, &ldim, &field, argv[3]),
        "unable to load field in position 3");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "field import");
    REIS(3, ldim, "expected [p,vol,phi]");

    ref_malloc_init(limeff, ref_node_max(ref_grid_node(ref_grid)), REF_DBL,
                    0.0);
    ref_malloc(p, ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    ref_malloc(grad, 3 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      p[node] = field[ip + ldim * node];
    }
    RSS(ref_recon_gradient(ref_grid, p, grad, reconstruction), "grad");
    ref_mpi_stopwatch_stop(ref_mpi, "kexact grad");
    RSS(ref_edge_create(&ref_edge, ref_grid), "create");
    ref_mpi_stopwatch_stop(ref_mpi, "create edges");
    each_ref_edge(ref_edge, edge) {
      REF_DBL qunlim, qlim;
      REF_DBL r[3];
      REF_INT n0, n1;
      n0 = ref_edge_e2n(ref_edge, 0, edge);
      n1 = ref_edge_e2n(ref_edge, 1, edge);
      /* n0 */
      r[0] = ref_node_xyz(ref_node, 0, n1) - ref_node_xyz(ref_node, 0, n0);
      r[1] = ref_node_xyz(ref_node, 1, n1) - ref_node_xyz(ref_node, 1, n0);
      r[2] = ref_node_xyz(ref_node, 2, n1) - ref_node_xyz(ref_node, 2, n0);
      qunlim = p[n0] + ref_math_dot(&(grad[3 * n0]), r);
      qlim = p[n0] + field[iphi + ldim * n0] * ref_math_dot(&(grad[3 * n0]), r);
      limeff[n0] = MAX(limeff[n0], ABS(qunlim - qlim));
      /* n1 */
      r[0] = ref_node_xyz(ref_node, 0, n0) - ref_node_xyz(ref_node, 0, n1);
      r[1] = ref_node_xyz(ref_node, 1, n0) - ref_node_xyz(ref_node, 1, n1);
      r[2] = ref_node_xyz(ref_node, 2, n0) - ref_node_xyz(ref_node, 2, n1);
      qunlim = p[n1] + ref_math_dot(&(grad[3 * n1]), r);
      qlim = p[n1] + field[iphi + ldim * n1] * ref_math_dot(&(grad[3 * n1]), r);
      limeff[n1] = MAX(limeff[n1], ABS(qunlim - qlim));
    }
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      REF_DBL h;
      h = pow(field[ivol + ldim * node], 1.0 / 3.0);
      if (ref_math_divisible(limeff[node], h)) {
        limeff[node] /= h;
      } else {
        printf("DIV zero %e / %e set to zero\n", limeff[node], h);
        limeff[node] = 0.0;
      }
    }
    ref_mpi_stopwatch_stop(ref_mpi, "limiter effect");
    RSS(ref_gather_scalar_by_extension(ref_grid, 1, limeff, NULL,
                                       "ref_recon_limiter_effect.plt"),
        "export lim eff plt");
    ref_mpi_stopwatch_stop(ref_mpi, "ref_recon_limiter_effect.plt");
    RSS(ref_gather_scalar_by_extension(ref_grid, 1, limeff, NULL,
                                       "ref_recon_limiter_effect.solb"),
        "export lim eff solb");
    ref_mpi_stopwatch_stop(ref_mpi, "ref_recon_limiter_effect.solb");
    RSS(ref_edge_free(ref_edge), "free");
    ref_free(grad);
    ref_free(p);
    ref_free(limeff);
    RSS(ref_grid_free(ref_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }
  if (argc == 3) {
    REF_GRID ref_grid;
    REF_DBL *function, *derivatives, *scalar, *grad;
    REF_INT ldim, node, i, dir;
    REF_RECON_RECONSTRUCTION reconstruction = REF_RECON_L2PROJECTION;
    if (ref_mpi_once(ref_mpi)) printf("reading grid %s\n", argv[1]);
    RSS(ref_part_by_extension(&ref_grid, ref_mpi, argv[1]),
        "unable to load grid in position 1");

    if (ref_mpi_once(ref_mpi)) printf("reading function %s\n", argv[2]);
    RSS(ref_part_scalar(ref_grid, &ldim, &function, argv[2]),
        "unable to load function in position 2");
    ref_malloc(derivatives, 3 * ldim * ref_node_max(ref_grid_node(ref_grid)),
               REF_DBL);
    ref_malloc(scalar, ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    ref_malloc(grad, 3 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);

    if (ref_mpi_once(ref_mpi)) printf("reconstruct\n");
    for (i = 0; i < ldim; i++) {
      each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
        scalar[node] = function[i + ldim * node];
      }
      RSS(ref_recon_gradient(ref_grid, scalar, grad, reconstruction), "grad");
      each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
        for (dir = 0; dir < 3; dir++) {
          derivatives[dir + 3 * i + 3 * ldim * node] = grad[dir + 3 * node];
        }
      }
    }
    if (ref_mpi_once(ref_mpi)) printf("gather %s\n", "ref_recon_deriv.plt");
    RSS(ref_gather_scalar_by_extension(ref_grid, 3 * ldim, derivatives, NULL,
                                       "ref_recon_deriv.plt"),
        "export derivatives");
    if (ref_mpi_once(ref_mpi)) printf("gather %s\n", "ref_recon_deriv.solb");
    RSS(ref_gather_scalar_by_extension(ref_grid, 3 * ldim, derivatives, NULL,
                                       "ref_recon_deriv.solb"),
        "export derivatives");
    ref_free(function);
    ref_free(derivatives);
    ref_free(scalar);
    ref_free(grad);
    RSS(ref_grid_free(ref_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  { /* l2-projection grad tet */
    REF_DBL tol = -1.0;
    REF_GRID ref_grid;
    REF_DBL *scalar, *grad;
    REF_INT node;

    RSS(ref_fixture_tet_grid(&ref_grid, ref_mpi), "tet");

    ref_malloc(scalar, ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    ref_malloc(grad, 3 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);

    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      scalar[node] = 1.3 * ref_node_xyz(ref_grid_node(ref_grid), 0, node) +
                     3.5 * ref_node_xyz(ref_grid_node(ref_grid), 1, node) +
                     7.2 * ref_node_xyz(ref_grid_node(ref_grid), 2, node) +
                     15.0;
    }

    RSS(ref_recon_gradient(ref_grid, scalar, grad, REF_RECON_L2PROJECTION),
        "l2 grad");

    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RWDS(1.3, grad[0 + 3 * node], tol, "gradx");
      RWDS(3.5, grad[1 + 3 * node], tol, "grady");
      RWDS(7.2, grad[2 + 3 * node], tol, "gradz");
    }

    ref_free(grad);
    ref_free(scalar);

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* l2-projection grad tri */
    REF_DBL tol = -1.0;
    REF_GRID ref_grid;
    REF_DBL *scalar, *grad;
    REF_INT node;

    RSS(ref_fixture_tri_grid(&ref_grid, ref_mpi), "tet");

    ref_malloc(scalar, ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    ref_malloc(grad, 3 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);

    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      scalar[node] = 1.3 * ref_node_xyz(ref_grid_node(ref_grid), 0, node) +
                     3.5 * ref_node_xyz(ref_grid_node(ref_grid), 1, node) +
                     15.0;
    }

    RSS(ref_recon_gradient(ref_grid, scalar, grad, REF_RECON_L2PROJECTION),
        "l2 grad");

    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RWDS(1.3, grad[0 + 3 * node], tol, "gradx");
      RWDS(3.5, grad[1 + 3 * node], tol, "grady");
      RWDS(0.0, grad[2 + 3 * node], tol, "gradz");
    }

    ref_free(grad);
    ref_free(scalar);

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* l2-projection grad tri twod brick */
    REF_DBL tol = -1.0;
    REF_GRID ref_grid;
    REF_DBL *scalar, *grad;
    REF_INT node;

    RSS(ref_fixture_twod_brick_grid(&ref_grid, ref_mpi, 4), "brick");

    ref_malloc(scalar, ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    ref_malloc(grad, 3 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);

    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      scalar[node] = 1.3 * ref_node_xyz(ref_grid_node(ref_grid), 0, node) +
                     3.5 * ref_node_xyz(ref_grid_node(ref_grid), 1, node) +
                     15.0;
    }

    RSS(ref_recon_gradient(ref_grid, scalar, grad, REF_RECON_L2PROJECTION),
        "l2 grad");

    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RWDS(1.3, grad[0 + 3 * node], tol, "gradx");
      RWDS(3.5, grad[1 + 3 * node], tol, "grady");
      RWDS(0.0, grad[2 + 3 * node], tol, "gradz");
    }

    ref_free(grad);
    ref_free(scalar);

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* l2-projection hessian zero, constant gradient tet brick */
    REF_DBL tol = -1.0;
    REF_GRID ref_grid;
    REF_DBL *scalar, *hessian;
    REF_INT node;

    RSS(ref_fixture_tet_brick_grid(&ref_grid, ref_mpi), "brick");

    ref_malloc(scalar, ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    ref_malloc(hessian, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);

    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      scalar[node] = 1.3 * ref_node_xyz(ref_grid_node(ref_grid), 0, node) +
                     3.5 * ref_node_xyz(ref_grid_node(ref_grid), 1, node) +
                     7.2 * ref_node_xyz(ref_grid_node(ref_grid), 2, node) +
                     15.0;
    }

    RSS(ref_recon_hessian(ref_grid, scalar, hessian, REF_RECON_L2PROJECTION),
        "l2 hess");

    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RWDS(0.0, hessian[0 + 6 * node], tol, "m11");
      RWDS(0.0, hessian[1 + 6 * node], tol, "m12");
      RWDS(0.0, hessian[2 + 6 * node], tol, "m13");
      RWDS(0.0, hessian[3 + 6 * node], tol, "m22");
      RWDS(0.0, hessian[4 + 6 * node], tol, "m23");
      RWDS(0.0, hessian[5 + 6 * node], tol, "m33");
    }

    ref_free(hessian);
    ref_free(scalar);

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* l2-projection hessian zero, constant gradient twod tri brick */
    REF_DBL tol = -1.0;
    REF_GRID ref_grid;
    REF_DBL *scalar, *hessian;
    REF_INT node;

    RSS(ref_fixture_twod_brick_grid(&ref_grid, ref_mpi, 4), "brick");

    ref_malloc(scalar, ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    ref_malloc(hessian, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);

    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      scalar[node] = 1.3 * ref_node_xyz(ref_grid_node(ref_grid), 0, node) +
                     3.5 * ref_node_xyz(ref_grid_node(ref_grid), 1, node) +
                     15.0;
    }

    RSS(ref_recon_hessian(ref_grid, scalar, hessian, REF_RECON_L2PROJECTION),
        "l2 hess");

    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RWDS(0.0, hessian[0 + 6 * node], tol, "m11");
      RWDS(0.0, hessian[1 + 6 * node], tol, "m12");
      RWDS(0.0, hessian[2 + 6 * node], tol, "m13");
      RWDS(0.0, hessian[3 + 6 * node], tol, "m22");
      RWDS(0.0, hessian[4 + 6 * node], tol, "m23");
      RWDS(0.0, hessian[5 + 6 * node], tol, "m33");
    }

    ref_free(hessian);
    ref_free(scalar);

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* absolute value */
    REF_DBL tol = -1.0;
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_INT global, node;
    REF_DBL *hessian;

    RSS(ref_grid_create(&ref_grid, ref_mpi), "create");
    ref_node = ref_grid_node(ref_grid);
    global = 0;
    RSS(ref_node_add(ref_node, global, &node), "add");
    ref_malloc(hessian, 6 * ref_node_max(ref_node), REF_DBL);
    hessian[0 + 6 * node] = 1.0;
    hessian[1 + 6 * node] = 0.0;
    hessian[2 + 6 * node] = 0.0;
    hessian[3 + 6 * node] = 2.0;
    hessian[4 + 6 * node] = 0.0;
    hessian[5 + 6 * node] = -3.0;

    RSS(ref_recon_abs_value_hessian(ref_grid, hessian), "abs hess");

    RWDS(1.0, hessian[0 + 6 * node], tol, "m11");
    RWDS(0.0, hessian[1 + 6 * node], tol, "m12");
    RWDS(0.0, hessian[2 + 6 * node], tol, "m13");
    RWDS(2.0, hessian[3 + 6 * node], tol, "m22");
    RWDS(0.0, hessian[4 + 6 * node], tol, "m23");
    RWDS(3.0, hessian[5 + 6 * node], tol, "m33");

    ref_free(hessian);
    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* zeroth order extrapolate boundary averaging constant recon */
    REF_DBL tol = -1.0;
    REF_GRID ref_grid;
    REF_CELL ref_cell;
    REF_DBL *recon;
    REF_BOOL *replace;
    REF_INT node;
    REF_INT i, cell, cell_node, nodes[REF_CELL_MAX_SIZE_PER];

    RSS(ref_fixture_tet_brick_grid(&ref_grid, ref_mpi), "brick");

    ref_malloc(recon, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    ref_malloc(replace, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_INT);

    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      recon[0 + 6 * node] = 100.0;
      recon[1 + 6 * node] = 7.0;
      recon[2 + 6 * node] = 22.0;
      recon[3 + 6 * node] = 200.0;
      recon[4 + 6 * node] = 15.0;
      recon[5 + 6 * node] = 300.0;
    }
    ref_cell = ref_grid_tri(ref_grid);
    each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
      each_ref_cell_cell_node(ref_cell, cell_node) {
        for (i = 0; i < 6; i++) recon[i + 6 * nodes[cell_node]] = 0.0;
      }
    }
    RSS(ref_recon_mask_tri(ref_grid, replace, 6), "mask");
    RSS(ref_recon_extrapolate_zeroth(ref_grid, recon, replace, 6),
        "bound extrap");

    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RWDS(100.0, recon[0 + 6 * node], tol, "m11");
      RWDS(7.0, recon[1 + 6 * node], tol, "m12");
      RWDS(22.0, recon[2 + 6 * node], tol, "m13");
      RWDS(200.0, recon[3 + 6 * node], tol, "m22");
      RWDS(15.0, recon[4 + 6 * node], tol, "m23");
      RWDS(300.0, recon[5 + 6 * node], tol, "m33");
    }

    ref_free(replace);
    ref_free(recon);

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* kexact extrapolate boundary averaging constant recon */
    REF_DBL tol = -1.0;
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_DBL *recon;
    REF_BOOL *replace;
    REF_INT node;
    REF_INT i;

    RSS(ref_fixture_tet_brick_grid(&ref_grid, ref_mpi), "brick");
    ref_node = ref_grid_node(ref_grid);

    ref_malloc(recon, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    ref_malloc(replace, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_INT);

    each_ref_node_valid_node(ref_node, node) {
      REF_DBL x = ref_node_xyz(ref_node, 0, node);
      REF_DBL y = ref_node_xyz(ref_node, 1, node);
      REF_DBL z = ref_node_xyz(ref_node, 2, node);
      recon[0 + 6 * node] = 100.0 + x * x;
      recon[1 + 6 * node] = 7.0 + x * y;
      recon[2 + 6 * node] = 22.0 + x * z;
      recon[3 + 6 * node] = 200.0 + y;
      recon[4 + 6 * node] = 15.0 + y * z;
      recon[5 + 6 * node] = 300.0 + z;
      for (i = 0; i < 6; i++) replace[i + 6 * node] = REF_FALSE;
    }
    each_ref_node_valid_node(ref_node, node) {
      if (ref_node_xyz(ref_node, 0, node) < 0.01) {
        for (i = 0; i < 6; i++) {
          recon[i + 6 * node] = 0.0;
          replace[i + 6 * node] = REF_TRUE;
        }
      }
    }

    RSS(ref_recon_extrapolate_kexact(ref_grid, recon, replace, 6),
        "bound extrap");

    each_ref_node_valid_node(ref_node, node) {
      REF_DBL x = ref_node_xyz(ref_node, 0, node);
      REF_DBL y = ref_node_xyz(ref_node, 1, node);
      REF_DBL z = ref_node_xyz(ref_node, 2, node);
      RWDS(100.0 + x * x, recon[0 + 6 * node], tol, "m11");
      RWDS(7.0 + x * y, recon[1 + 6 * node], tol, "m12");
      RWDS(22.0 + x * z, recon[2 + 6 * node], tol, "m13");
      RWDS(200.0 + y, recon[3 + 6 * node], tol, "m22");
      RWDS(15.0 + y * z, recon[4 + 6 * node], tol, "m23");
      RWDS(300.0 + z, recon[5 + 6 * node], tol, "m33");
    }

    ref_free(replace);
    ref_free(recon);

    RSS(ref_grid_free(ref_grid), "free");
  }

  if (!ref_mpi_para(ref_mpi)) { /* seq k-exact gradient for small variation */
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_INT node;
    REF_DBL *scalar, *gradient;
    REF_DBL tol = -1.0;

    RSS(ref_fixture_tet_brick_grid(&ref_grid, ref_mpi), "brick");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc(scalar, ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    ref_malloc(gradient, 3 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      REF_DBL x = ref_node_xyz(ref_node, 0, node);
      REF_DBL y = ref_node_xyz(ref_node, 1, node);
      REF_DBL z = ref_node_xyz(ref_node, 2, node);
      scalar[node] = 0.5 + 0.01 * x + 0.02 * y + 0.06 * z;
    }
    RSS(ref_recon_gradient(ref_grid, scalar, gradient, REF_RECON_KEXACT),
        "k-exact hess");
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RWDS(0.01, gradient[0 + 3 * node], tol, "g[0]");
      RWDS(0.02, gradient[1 + 3 * node], tol, "g[1]");
      RWDS(0.06, gradient[2 + 3 * node], tol, "g[2]");
    }

    ref_free(gradient);
    ref_free(scalar);

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* para file k-exact gradient for small variation */
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_INT node;
    REF_DBL *scalar, *gradient;
    REF_DBL tol = -1.0;
    char file[] = "ref_recon_test.meshb";

    if (ref_mpi_once(ref_mpi)) {
      RSS(ref_fixture_tet_brick_grid(&ref_grid, ref_mpi), "brick");
      RSS(ref_export_by_extension(ref_grid, file), "export");
      RSS(ref_grid_free(ref_grid), "free");
    }
    RSS(ref_part_by_extension(&ref_grid, ref_mpi, file), "import");
    if (ref_mpi_once(ref_mpi)) REIS(0, remove(file), "test clean up");

    ref_node = ref_grid_node(ref_grid);
    ref_malloc(scalar, ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    ref_malloc(gradient, 3 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      REF_DBL x = ref_node_xyz(ref_node, 0, node);
      REF_DBL y = ref_node_xyz(ref_node, 1, node);
      REF_DBL z = ref_node_xyz(ref_node, 2, node);
      scalar[node] = 0.5 + 0.01 * x + 0.02 * y + 0.06 * z;
    }
    RSS(ref_recon_gradient(ref_grid, scalar, gradient, REF_RECON_KEXACT),
        "k-exact hess");
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RWDS(0.01, gradient[0 + 3 * node], tol, "g[0]");
      RWDS(0.02, gradient[1 + 3 * node], tol, "g[1]");
      RWDS(0.06, gradient[2 + 3 * node], tol, "g[2]");
    }

    ref_free(gradient);
    ref_free(scalar);

    RSS(ref_grid_free(ref_grid), "free");
  }

  if (!ref_mpi_para(ref_mpi)) { /* seq k-exact hessian for small variation */
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_INT node;
    REF_DBL *scalar, *hessian;
    REF_DBL tol = -1.0;

    RSS(ref_fixture_tet_brick_grid(&ref_grid, ref_mpi), "brick");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc(scalar, ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    ref_malloc(hessian, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      REF_DBL x = ref_node_xyz(ref_node, 0, node);
      REF_DBL y = ref_node_xyz(ref_node, 1, node);
      REF_DBL z = ref_node_xyz(ref_node, 2, node);
      scalar[node] = 0.5 + 0.01 * (0.5 * x * x) + 0.02 * x * y +
                     0.04 * (0.5 * y * y) + 0.06 * (0.5 * z * z);
    }
    RSS(ref_recon_hessian(ref_grid, scalar, hessian, REF_RECON_KEXACT),
        "k-exact hess");
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RWDS(0.01, hessian[0 + 6 * node], tol, "m[0]");
      RWDS(0.02, hessian[1 + 6 * node], tol, "m[1]");
      RWDS(0.00, hessian[2 + 6 * node], tol, "m[2]");
      RWDS(0.04, hessian[3 + 6 * node], tol, "m[3]");
      RWDS(0.00, hessian[4 + 6 * node], tol, "m[4]");
      RWDS(0.06, hessian[5 + 6 * node], tol, "m[5]");
    }

    ref_free(hessian);
    ref_free(scalar);

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* para file k-exact hessian for small variation */
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_INT node;
    REF_DBL *scalar, *hessian;
    REF_DBL tol = -1.0;
    char file[] = "ref_recon_test.meshb";

    if (ref_mpi_once(ref_mpi)) {
      RSS(ref_fixture_tet_brick_grid(&ref_grid, ref_mpi), "brick");
      RSS(ref_export_by_extension(ref_grid, file), "export");
      RSS(ref_grid_free(ref_grid), "free");
    }
    RSS(ref_part_by_extension(&ref_grid, ref_mpi, file), "import");
    if (ref_mpi_once(ref_mpi)) REIS(0, remove(file), "test clean up");

    ref_node = ref_grid_node(ref_grid);
    ref_malloc(scalar, ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    ref_malloc(hessian, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      REF_DBL x = ref_node_xyz(ref_node, 0, node);
      REF_DBL y = ref_node_xyz(ref_node, 1, node);
      REF_DBL z = ref_node_xyz(ref_node, 2, node);
      scalar[node] = 0.5 + 0.01 * (0.5 * x * x) + 0.02 * x * y +
                     0.04 * (0.5 * y * y) + 0.06 * (0.5 * z * z);
    }
    RSS(ref_recon_hessian(ref_grid, scalar, hessian, REF_RECON_KEXACT),
        "k-exact hess");
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RWDS(0.01, hessian[0 + 6 * node], tol, "m[0]");
      RWDS(0.02, hessian[1 + 6 * node], tol, "m[1]");
      RWDS(0.00, hessian[2 + 6 * node], tol, "m[2]");
      RWDS(0.04, hessian[3 + 6 * node], tol, "m[3]");
      RWDS(0.00, hessian[4 + 6 * node], tol, "m[4]");
      RWDS(0.06, hessian[5 + 6 * node], tol, "m[5]");
    }

    ref_free(hessian);
    ref_free(scalar);

    RSS(ref_grid_free(ref_grid), "free");
  }

  if (!ref_mpi_para(ref_mpi)) { /* k-exact 2D */
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_INT node;
    REF_DBL *scalar, *hessian;
    REF_DBL tol = -1.0;

    RSS(ref_fixture_twod_brick_grid(&ref_grid, ref_mpi, 4), "brick");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc(scalar, ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    ref_malloc(hessian, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      REF_DBL x = ref_node_xyz(ref_node, 0, node);
      REF_DBL y = ref_node_xyz(ref_node, 1, node);
      scalar[node] =
          0.5 + 0.01 * (0.5 * x * x) + 0.02 * x * y + 0.06 * (0.5 * y * y);
    }
    RSS(ref_recon_hessian(ref_grid, scalar, hessian, REF_RECON_KEXACT),
        "k-exact hess");
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RWDS(0.01, hessian[0 + 6 * node], tol, "m[0] xx");
      RWDS(0.02, hessian[1 + 6 * node], tol, "m[1] xy");
      RWDS(0.00, hessian[2 + 6 * node], tol, "m[2] xz");
      RWDS(0.06, hessian[3 + 6 * node], tol, "m[3] yy");
      RWDS(0.00, hessian[4 + 6 * node], tol, "m[4] yz");
      RWDS(0.00, hessian[5 + 6 * node], tol, "m[5] zz");
    }

    ref_free(hessian);
    ref_free(scalar);

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* imply recon right tet */
    REF_DBL tol = 1.0e-12;
    REF_GRID ref_grid;
    REF_DBL *hess;
    REF_INT node;

    RSS(ref_fixture_tet_grid(&ref_grid, ref_mpi), "tet");

    ref_malloc_init(hess, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL,
                    0.0);

    RSS(ref_recon_roundoff_limit(hess, ref_grid), "imply");

    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RAS(tol < hess[0 + 6 * node], "m[0]");
      RAS(tol < hess[3 + 6 * node], "m[3]");
      RAS(tol < hess[5 + 6 * node], "m[5]");
    }

    ref_free(hess);

    RSS(ref_grid_free(ref_grid), "free");
  }

  { /* ghost cloud one_layer cloud */
    REF_NODE ref_node;
    REF_INT local, ghost;
    REF_GLOB global;
    REF_CLOUD one_layer[] = {NULL, NULL};
    REF_DBL xyzs[4];

    RSS(ref_node_create(&ref_node, ref_mpi), "create");

    /* set up 0-local and 1-ghost with global=part */
    global = ref_mpi_rank(ref_mpi);
    RSS(ref_node_add(ref_node, global, &local), "add");
    ref_node_part(ref_node, local) = (REF_INT)global;

    if (ref_mpi_para(ref_mpi)) {
      global = ref_mpi_rank(ref_mpi) + 1;
      if (global >= ref_mpi_n(ref_mpi)) global = 0;
      RSS(ref_node_add(ref_node, global, &ghost), "add");
      ref_node_part(ref_node, ghost) = (REF_INT)global;
    }

    /* set up local cloud with both nodes if para */
    each_ref_node_valid_node(ref_node, local) {
      RSS(ref_cloud_create(&(one_layer[local]), 4), "cloud storage");
    }
    local = 0;
    xyzs[0] = 10.0 * ref_node_part(ref_node, local);
    xyzs[1] = 20.0 * ref_node_part(ref_node, local);
    xyzs[2] = 30.0 * ref_node_part(ref_node, local);
    xyzs[3] = 40.0 * ref_node_part(ref_node, local);
    global = ref_node_global(ref_node, local);
    RSS(ref_cloud_store(one_layer[0], global, xyzs), "store cloud stencil");
    local = 1;
    if (ref_node_valid(ref_node, local)) {
      xyzs[0] = 10.0 * ref_node_part(ref_node, local);
      xyzs[1] = 20.0 * ref_node_part(ref_node, local);
      xyzs[2] = 30.0 * ref_node_part(ref_node, local);
      xyzs[3] = 40.0 * ref_node_part(ref_node, local);
      global = ref_node_global(ref_node, local);
      RSS(ref_cloud_store(one_layer[0], global, xyzs), "store cloud stencil");
    }
    RSS(ref_recon_ghost_cloud(one_layer, ref_node), "update ghosts");

    if (ref_mpi_para(ref_mpi)) {
      REIS(2, ref_cloud_n(one_layer[0]), "local");
      global = ref_mpi_rank(ref_mpi);
      RAS(ref_cloud_has_global(one_layer[0], global), "local");
      global = ref_mpi_rank(ref_mpi) + 1;
      if (global >= ref_mpi_n(ref_mpi)) global = 0;
      RAS(ref_cloud_has_global(one_layer[0], global), "local");

      REIS(2, ref_cloud_n(one_layer[1]), "ghost");
      global = ref_mpi_rank(ref_mpi) + 1;
      if (global >= ref_mpi_n(ref_mpi)) global -= ref_mpi_n(ref_mpi);
      RAS(ref_cloud_has_global(one_layer[1], global), "local");
      global = ref_mpi_rank(ref_mpi) + 2;
      if (global >= ref_mpi_n(ref_mpi)) global -= ref_mpi_n(ref_mpi);
      RAS(ref_cloud_has_global(one_layer[1], global), "local");
    } else {
      REIS(1, ref_cloud_n(one_layer[0]), "no one");
    }

    each_ref_node_valid_node(ref_node, local) {
      ref_cloud_free(one_layer[local]); /* no-op for null */
    }
    RSS(ref_node_free(ref_node), "free");
  }

  if (!ref_mpi_para(ref_mpi)) {
    REF_DBL tol = -1.0;
    REF_GRID ref_grid;
    REF_INT node;
    REF_DBL normal[3];
    RSS(ref_fixture_tri2_grid(&ref_grid, ref_mpi), "fixture");
    node = 3; /* one tri */
    RSS(ref_recon_normal(ref_grid, node, normal), "norm");
    RWDS(0.0, normal[0], tol, "nx");
    RWDS(0.0, normal[1], tol, "ny");
    RWDS(-1.0, normal[2], tol, "nz");
    node = 0; /* two tri */
    RSS(ref_recon_normal(ref_grid, node, normal), "norm");
    RWDS(0.0, normal[0], tol, "nx");
    RWDS(0.0, normal[1], tol, "ny");
    RWDS(-1.0, normal[2], tol, "nz");
    ref_grid_free(ref_grid);
  }

  if (!ref_mpi_para(ref_mpi)) {
    REF_DBL tol = -1.0;
    REF_GRID ref_grid;
    REF_INT node;
    REF_DBL r[3], s[3], n[3];
    RSS(ref_fixture_tri2_grid(&ref_grid, ref_mpi), "fixture");
    node = 3; /* one tri */
    RSS(ref_recon_rsn(ref_grid, node, r, s, n), "rsn");
    RWDS(1.0, r[0], tol, "rx");
    RWDS(0.0, r[1], tol, "ry");
    RWDS(0.0, r[2], tol, "rz");
    RWDS(0.0, s[0], tol, "sx");
    RWDS(-1.0, s[1], tol, "sy");
    RWDS(0.0, s[2], tol, "sz");
    RWDS(0.0, n[0], tol, "nx");
    RWDS(0.0, n[1], tol, "ny");
    RWDS(-1.0, n[2], tol, "nz");
    ref_grid_free(ref_grid);
  }

  if (!ref_mpi_para(ref_mpi)) {
    REF_DBL tol = -1.0;
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_INT node, center;
    REF_DBL distance[4];
    REF_DBL hess[3 * 4];
    REF_DBL r, s;
    REF_DBL rn[3], sn[3], nn[3];
    REF_DBL dxyz[3];
    center = 0; /* two tri */
    RSS(ref_fixture_tri2_grid(&ref_grid, ref_mpi), "fixture");
    ref_node = ref_grid_node(ref_grid);
    ref_node_xyz(ref_node, 1, 3) = -1.0;

    RSS(ref_recon_rsn(ref_grid, center, rn, sn, nn), "rsn");

    each_ref_node_valid_node(ref_node, node) {
      dxyz[0] =
          ref_node_xyz(ref_node, 0, node) - ref_node_xyz(ref_node, 0, center);
      dxyz[1] =
          ref_node_xyz(ref_node, 1, node) - ref_node_xyz(ref_node, 1, center);
      dxyz[2] =
          ref_node_xyz(ref_node, 2, node) - ref_node_xyz(ref_node, 2, center);
      r = ref_math_dot(rn, dxyz);
      s = ref_math_dot(sn, dxyz);
      distance[node] = 2.0 * (0.5 * r * r) - 3.0 * (0.5 * s * s);
    }

    RSS(ref_recon_rsn_hess(ref_grid, distance, hess), "rsn");

    RWDS(2.0, hess[0 + 3 * center], tol, "dr2");
    RWDS(0.0, hess[1 + 3 * center], tol, "drds");
    RWDS(3.0, hess[2 + 3 * center], tol, "ds2");

    ref_grid_free(ref_grid);
  }

  if (!ref_mpi_para(ref_mpi)) {
    REF_DBL tol = -1.0;
    REF_GRID ref_grid;
    REF_NODE ref_node;
    REF_INT node, center;
    REF_DBL *distance;
    REF_DBL *hess;
    REF_DBL r, s;
    REF_DBL rn[3], sn[3], nn[3];
    REF_DBL dxyz[3];
    center = 6; /* two tri */
    RSS(ref_fixture_twod_brick_grid(&ref_grid, ref_mpi, 4), "fixture");
    ref_node = ref_grid_node(ref_grid);
    ref_malloc_init(hess, 3 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL,
                    0.0);
    ref_malloc_init(distance, ref_node_max(ref_grid_node(ref_grid)), REF_DBL,
                    0.0);

    RSS(ref_recon_rsn(ref_grid, center, rn, sn, nn), "rsn");

    each_ref_node_valid_node(ref_node, node) {
      dxyz[0] =
          ref_node_xyz(ref_node, 0, node) - ref_node_xyz(ref_node, 0, center);
      dxyz[1] =
          ref_node_xyz(ref_node, 1, node) - ref_node_xyz(ref_node, 1, center);
      dxyz[2] =
          ref_node_xyz(ref_node, 2, node) - ref_node_xyz(ref_node, 2, center);
      r = ref_math_dot(rn, dxyz);
      s = ref_math_dot(sn, dxyz);
      distance[node] = 5.0 * (0.5 * r * r) - 7.0 * (0.5 * s * s);
    }

    RSS(ref_recon_rsn_hess(ref_grid, distance, hess), "rsn");

    RWDS(5.0, hess[0 + 3 * center], tol, "dr2");
    RWDS(0.0, hess[1 + 3 * center], tol, "drds");
    RWDS(7.0, hess[2 + 3 * center], tol, "ds2");

    ref_free(distance);
    ref_free(hess);
    ref_grid_free(ref_grid);
  }

  RSS(ref_mpi_free(ref_mpi), "free");
  RSS(ref_mpi_stop(), "stop");
  return 0;
}
