
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

#include "ref_cell.h"
#include "ref_edge.h"
#include "ref_grid.h"
#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_matrix.h"
#include "ref_node.h"

#define REF_RECON_MAX_DEGREE (1000)

/* Alauzet and A. Loseille doi:10.1016/j.jcp.2009.09.020
 * section 2.2.4.1. A double L2-projection */
REF_STATUS ref_recon_l2_projection_grad(REF_GRID ref_grid, REF_DBL *scalar,
                                        REF_DBL *grad) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT i, node, cell, group, cell_node;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER], tet_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT pri_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT qua_nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL div_by_zero;
  REF_DBL cell_vol, cell_grad[3];
  REF_DBL *vol;
  REF_STATUS vol_status, grad_status;

  ref_malloc_init(vol, ref_node_max(ref_node), REF_DBL, 0.0);

  each_ref_node_valid_node(ref_node, node) for (i = 0; i < 3; i++)
      grad[i + 3 * node] = 0.0;

  if (ref_grid_twod(ref_grid)) {
    each_ref_cell_valid_cell_with_nodes(ref_grid_tri(ref_grid), cell, nodes) {
      vol_status = ref_node_tri_area(ref_node, nodes, &cell_vol);
      grad_status = ref_node_tri_grad_nodes(ref_node, nodes, scalar, cell_grad);
      if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
        for (cell_node = 0; cell_node < 3; cell_node++)
          for (i = 0; i < 3; i++)
            grad[i + 3 * nodes[cell_node]] += cell_vol * cell_grad[i];
        for (cell_node = 0; cell_node < 3; cell_node++)
          vol[nodes[cell_node]] += cell_vol;
      } else {
        printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__, __LINE__,
               __func__, vol_status, grad_status);
      }
    }
    each_ref_cell_valid_cell_with_nodes(ref_grid_qua(ref_grid), cell,
                                        qua_nodes) {
      nodes[0] = qua_nodes[0];
      nodes[1] = qua_nodes[1];
      nodes[2] = qua_nodes[2];
      nodes[3] = qua_nodes[4];
      vol_status = ref_node_tri_area(ref_node, nodes, &cell_vol);
      grad_status = ref_node_tri_grad_nodes(ref_node, nodes, scalar, cell_grad);
      if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
        for (cell_node = 0; cell_node < 3; cell_node++)
          for (i = 0; i < 3; i++)
            grad[i + 3 * nodes[cell_node]] += cell_vol * cell_grad[i];
        for (cell_node = 0; cell_node < 3; cell_node++)
          vol[nodes[cell_node]] += cell_vol;
      } else {
        printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__, __LINE__,
               __func__, vol_status, grad_status);
      }
      nodes[0] = qua_nodes[0];
      nodes[1] = qua_nodes[2];
      nodes[2] = qua_nodes[3];
      nodes[3] = qua_nodes[4];
      vol_status = ref_node_tri_area(ref_node, nodes, &cell_vol);
      grad_status = ref_node_tri_grad_nodes(ref_node, nodes, scalar, cell_grad);
      if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
        for (cell_node = 0; cell_node < 3; cell_node++)
          for (i = 0; i < 3; i++)
            grad[i + 3 * nodes[cell_node]] += cell_vol * cell_grad[i];
        for (cell_node = 0; cell_node < 3; cell_node++)
          vol[nodes[cell_node]] += cell_vol;
      } else {
        printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__, __LINE__,
               __func__, vol_status, grad_status);
      }
    }
  } else {
    each_ref_grid_3d_ref_cell(ref_grid, group, ref_cell) {
      each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
        switch (ref_cell_type(ref_cell)) {
          case REF_CELL_TET:
            vol_status = ref_node_tet_vol(ref_node, nodes, &cell_vol);
            grad_status =
                ref_node_tet_grad_nodes(ref_node, nodes, scalar, cell_grad);
            if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
              for (cell_node = 0; cell_node < 4; cell_node++)
                for (i = 0; i < 3; i++)
                  grad[i + 3 * nodes[cell_node]] += cell_vol * cell_grad[i];
              for (cell_node = 0; cell_node < 4; cell_node++)
                vol[nodes[cell_node]] += cell_vol;
            } else {
              printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__,
                     __LINE__, __func__, vol_status, grad_status);
            }
            break;
          case REF_CELL_PRI:
            tet_nodes[0] = nodes[0];
            tet_nodes[1] = nodes[4];
            tet_nodes[2] = nodes[5];
            tet_nodes[3] = nodes[3];
            vol_status = ref_node_tet_vol(ref_node, tet_nodes, &cell_vol);
            grad_status =
                ref_node_tet_grad_nodes(ref_node, tet_nodes, scalar, cell_grad);
            if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
              for (cell_node = 0; cell_node < 4; cell_node++)
                for (i = 0; i < 3; i++)
                  grad[i + 3 * tet_nodes[cell_node]] += cell_vol * cell_grad[i];
              for (cell_node = 0; cell_node < 4; cell_node++)
                vol[tet_nodes[cell_node]] += cell_vol;
            } else {
              printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__,
                     __LINE__, __func__, vol_status, grad_status);
            }
            tet_nodes[0] = nodes[0];
            tet_nodes[1] = nodes[1];
            tet_nodes[2] = nodes[5];
            tet_nodes[3] = nodes[4];
            vol_status = ref_node_tet_vol(ref_node, tet_nodes, &cell_vol);
            grad_status =
                ref_node_tet_grad_nodes(ref_node, tet_nodes, scalar, cell_grad);
            if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
              for (cell_node = 0; cell_node < 4; cell_node++)
                for (i = 0; i < 3; i++)
                  grad[i + 3 * tet_nodes[cell_node]] += cell_vol * cell_grad[i];
              for (cell_node = 0; cell_node < 4; cell_node++)
                vol[tet_nodes[cell_node]] += cell_vol;
            } else {
              printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__,
                     __LINE__, __func__, vol_status, grad_status);
            }
            tet_nodes[0] = nodes[0];
            tet_nodes[1] = nodes[1];
            tet_nodes[2] = nodes[2];
            tet_nodes[3] = nodes[5];
            vol_status = ref_node_tet_vol(ref_node, tet_nodes, &cell_vol);
            grad_status =
                ref_node_tet_grad_nodes(ref_node, tet_nodes, scalar, cell_grad);
            if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
              for (cell_node = 0; cell_node < 4; cell_node++)
                for (i = 0; i < 3; i++)
                  grad[i + 3 * tet_nodes[cell_node]] += cell_vol * cell_grad[i];
              for (cell_node = 0; cell_node < 4; cell_node++)
                vol[tet_nodes[cell_node]] += cell_vol;
            } else {
              printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__,
                     __LINE__, __func__, vol_status, grad_status);
            }
            break;
          case REF_CELL_PYR:
            tet_nodes[0] = nodes[0];
            tet_nodes[1] = nodes[4];
            tet_nodes[2] = nodes[1];
            tet_nodes[3] = nodes[2];
            vol_status = ref_node_tet_vol(ref_node, tet_nodes, &cell_vol);
            grad_status =
                ref_node_tet_grad_nodes(ref_node, tet_nodes, scalar, cell_grad);
            if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
              for (cell_node = 0; cell_node < 4; cell_node++)
                for (i = 0; i < 3; i++)
                  grad[i + 3 * tet_nodes[cell_node]] += cell_vol * cell_grad[i];
              for (cell_node = 0; cell_node < 4; cell_node++)
                vol[tet_nodes[cell_node]] += cell_vol;
            } else {
              printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__,
                     __LINE__, __func__, vol_status, grad_status);
            }
            tet_nodes[0] = nodes[0];
            tet_nodes[1] = nodes[3];
            tet_nodes[2] = nodes[4];
            tet_nodes[3] = nodes[2];
            vol_status = ref_node_tet_vol(ref_node, tet_nodes, &cell_vol);
            grad_status =
                ref_node_tet_grad_nodes(ref_node, tet_nodes, scalar, cell_grad);
            if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
              for (cell_node = 0; cell_node < 4; cell_node++)
                for (i = 0; i < 3; i++)
                  grad[i + 3 * tet_nodes[cell_node]] += cell_vol * cell_grad[i];
              for (cell_node = 0; cell_node < 4; cell_node++)
                vol[tet_nodes[cell_node]] += cell_vol;
            } else {
              printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__,
                     __LINE__, __func__, vol_status, grad_status);
            }
            break;
          case REF_CELL_HEX:

            pri_nodes[0] = nodes[1];
            pri_nodes[1] = nodes[0];
            pri_nodes[2] = nodes[4];
            pri_nodes[3] = nodes[2];
            pri_nodes[4] = nodes[3];
            pri_nodes[5] = nodes[7];

            tet_nodes[0] = pri_nodes[0];
            tet_nodes[1] = pri_nodes[4];
            tet_nodes[2] = pri_nodes[5];
            tet_nodes[3] = pri_nodes[3];
            vol_status = ref_node_tet_vol(ref_node, tet_nodes, &cell_vol);
            grad_status =
                ref_node_tet_grad_nodes(ref_node, tet_nodes, scalar, cell_grad);
            if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
              for (cell_node = 0; cell_node < 4; cell_node++)
                for (i = 0; i < 3; i++)
                  grad[i + 3 * tet_nodes[cell_node]] += cell_vol * cell_grad[i];
              for (cell_node = 0; cell_node < 4; cell_node++)
                vol[tet_nodes[cell_node]] += cell_vol;
            } else {
              printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__,
                     __LINE__, __func__, vol_status, grad_status);
            }
            tet_nodes[0] = pri_nodes[0];
            tet_nodes[1] = pri_nodes[1];
            tet_nodes[2] = pri_nodes[5];
            tet_nodes[3] = pri_nodes[4];
            vol_status = ref_node_tet_vol(ref_node, tet_nodes, &cell_vol);
            grad_status =
                ref_node_tet_grad_nodes(ref_node, tet_nodes, scalar, cell_grad);
            if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
              for (cell_node = 0; cell_node < 4; cell_node++)
                for (i = 0; i < 3; i++)
                  grad[i + 3 * tet_nodes[cell_node]] += cell_vol * cell_grad[i];
              for (cell_node = 0; cell_node < 4; cell_node++)
                vol[tet_nodes[cell_node]] += cell_vol;
            } else {
              printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__,
                     __LINE__, __func__, vol_status, grad_status);
            }
            tet_nodes[0] = pri_nodes[0];
            tet_nodes[1] = pri_nodes[1];
            tet_nodes[2] = pri_nodes[2];
            tet_nodes[3] = pri_nodes[5];
            vol_status = ref_node_tet_vol(ref_node, tet_nodes, &cell_vol);
            grad_status =
                ref_node_tet_grad_nodes(ref_node, tet_nodes, scalar, cell_grad);
            if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
              for (cell_node = 0; cell_node < 4; cell_node++)
                for (i = 0; i < 3; i++)
                  grad[i + 3 * tet_nodes[cell_node]] += cell_vol * cell_grad[i];
              for (cell_node = 0; cell_node < 4; cell_node++)
                vol[tet_nodes[cell_node]] += cell_vol;
            } else {
              printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__,
                     __LINE__, __func__, vol_status, grad_status);
            }

            pri_nodes[0] = nodes[1];
            pri_nodes[1] = nodes[4];
            pri_nodes[2] = nodes[5];
            pri_nodes[3] = nodes[2];
            pri_nodes[4] = nodes[7];
            pri_nodes[5] = nodes[6];

            tet_nodes[1] = pri_nodes[4];
            tet_nodes[2] = pri_nodes[5];
            tet_nodes[3] = pri_nodes[3];
            vol_status = ref_node_tet_vol(ref_node, tet_nodes, &cell_vol);
            grad_status =
                ref_node_tet_grad_nodes(ref_node, tet_nodes, scalar, cell_grad);
            if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
              for (cell_node = 0; cell_node < 4; cell_node++)
                for (i = 0; i < 3; i++)
                  grad[i + 3 * tet_nodes[cell_node]] += cell_vol * cell_grad[i];
              for (cell_node = 0; cell_node < 4; cell_node++)
                vol[tet_nodes[cell_node]] += cell_vol;
            } else {
              printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__,
                     __LINE__, __func__, vol_status, grad_status);
            }
            tet_nodes[0] = pri_nodes[0];
            tet_nodes[1] = pri_nodes[1];
            tet_nodes[2] = pri_nodes[5];
            tet_nodes[3] = pri_nodes[4];
            vol_status = ref_node_tet_vol(ref_node, tet_nodes, &cell_vol);
            grad_status =
                ref_node_tet_grad_nodes(ref_node, tet_nodes, scalar, cell_grad);
            if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
              for (cell_node = 0; cell_node < 4; cell_node++)
                for (i = 0; i < 3; i++)
                  grad[i + 3 * tet_nodes[cell_node]] += cell_vol * cell_grad[i];
              for (cell_node = 0; cell_node < 4; cell_node++)
                vol[tet_nodes[cell_node]] += cell_vol;
            } else {
              printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__,
                     __LINE__, __func__, vol_status, grad_status);
            }
            tet_nodes[0] = pri_nodes[0];
            tet_nodes[1] = pri_nodes[1];
            tet_nodes[2] = pri_nodes[2];
            tet_nodes[3] = pri_nodes[5];
            vol_status = ref_node_tet_vol(ref_node, tet_nodes, &cell_vol);
            grad_status =
                ref_node_tet_grad_nodes(ref_node, tet_nodes, scalar, cell_grad);
            if (REF_SUCCESS == vol_status && REF_SUCCESS == grad_status) {
              for (cell_node = 0; cell_node < 4; cell_node++)
                for (i = 0; i < 3; i++)
                  grad[i + 3 * tet_nodes[cell_node]] += cell_vol * cell_grad[i];
              for (cell_node = 0; cell_node < 4; cell_node++)
                vol[tet_nodes[cell_node]] += cell_vol;
            } else {
              printf("%s: %d: %s: vol status %d grad status %d\n", __FILE__,
                     __LINE__, __func__, vol_status, grad_status);
            }

            break;
          case REF_CELL_EDG:
          case REF_CELL_ED2:
          case REF_CELL_ED3:
          case REF_CELL_TRI:
          case REF_CELL_TR2:
          case REF_CELL_TR3:
          case REF_CELL_QUA:
          case REF_CELL_TE2:
            RSB(REF_IMPLEMENT, "implement cell type",
                { printf("unknown type %d\n", (int)ref_cell_type(ref_cell)); });
        }
      }
    }
  }

  div_by_zero = REF_FALSE;
  each_ref_node_valid_node(ref_node, node) {
    if (ref_math_divisible(grad[0 + 3 * node], vol[node]) &&
        ref_math_divisible(grad[1 + 3 * node], vol[node]) &&
        ref_math_divisible(grad[2 + 3 * node], vol[node])) {
      for (i = 0; i < 3; i++) grad[i + 3 * node] /= vol[node];
    } else {
      div_by_zero = REF_TRUE;
      for (i = 0; i < 3; i++) grad[i + 3 * node] = 0.0;
      printf("%s: %d: %s: total vol %e, ignored\n", __FILE__, __LINE__,
             __func__, vol[node]);
    }
  }
  RSS(ref_mpi_all_or(ref_grid_mpi(ref_grid), &div_by_zero), "mpi all or");
  RSS(ref_node_ghost_dbl(ref_node, grad, 3), "update ghosts");

  ref_free(vol);

  return (div_by_zero ? REF_DIV_ZERO : REF_SUCCESS);
}

static REF_STATUS ref_recon_l2_projection_hessian(REF_GRID ref_grid,
                                                  REF_DBL *scalar,
                                                  REF_DBL *hessian) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT i, node;
  REF_DBL *grad, *dsdx, *gradx, *grady, *gradz;

  ref_malloc_init(grad, 3 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(dsdx, ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(gradx, 3 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(grady, 3 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(gradz, 3 * ref_node_max(ref_node), REF_DBL, 0.0);

  RSS(ref_recon_l2_projection_grad(ref_grid, scalar, grad), "l2 grad");

  i = 0;
  each_ref_node_valid_node(ref_node, node) dsdx[node] = grad[i + 3 * node];
  RSS(ref_recon_l2_projection_grad(ref_grid, dsdx, gradx), "gradx");

  i = 1;
  each_ref_node_valid_node(ref_node, node) dsdx[node] = grad[i + 3 * node];
  RSS(ref_recon_l2_projection_grad(ref_grid, dsdx, grady), "grady");

  i = 2;
  each_ref_node_valid_node(ref_node, node) dsdx[node] = grad[i + 3 * node];
  RSS(ref_recon_l2_projection_grad(ref_grid, dsdx, gradz), "gradz");

  /* average off-diagonals */
  each_ref_node_valid_node(ref_node, node) {
    hessian[0 + 6 * node] = gradx[0 + 3 * node];
    hessian[1 + 6 * node] = 0.5 * (gradx[1 + 3 * node] + grady[0 + 3 * node]);
    hessian[2 + 6 * node] = 0.5 * (gradx[2 + 3 * node] + gradz[0 + 3 * node]);
    hessian[3 + 6 * node] = grady[1 + 3 * node];
    hessian[4 + 6 * node] = 0.5 * (grady[2 + 3 * node] + gradz[1 + 3 * node]);
    hessian[5 + 6 * node] = gradz[2 + 3 * node];
  }

  ref_free(gradz);
  ref_free(grady);
  ref_free(gradx);
  ref_free(dsdx);
  ref_free(grad);

  return REF_SUCCESS;
}

static REF_STATUS ref_recon_kexact_with_aux(REF_GLOB center_global,
                                            REF_CLOUD ref_cloud, REF_BOOL twod,
                                            REF_DBL *gradient,
                                            REF_DBL *hessian) {
  REF_DBL geom[9], ab[90];
  REF_DBL dx, dy, dz, dq;
  REF_DBL *a, *q, *r;
  REF_INT m, n;
  REF_GLOB cloud_global;
  REF_INT item, im, i, j;
  REF_DBL xyzs[4];
  REF_BOOL verbose = REF_FALSE;

  for (im = 0; im < 6; im++) {
    hessian[im] = 0.0;
  }
  for (im = 0; im < 3; im++) {
    gradient[im] = 0.0;
  }

  RSS(ref_cloud_item(ref_cloud, center_global, &item), "missing center");
  each_ref_cloud_aux(ref_cloud, i) {
    xyzs[i] = ref_cloud_aux(ref_cloud, i, item);
  }
  /* solve A with QR factorization size m x n */
  m = ref_cloud_n(ref_cloud) - 1; /* skip self */
  if (twod) m += 4;               /* add z node node */
  n = 9;
  if (verbose)
    printf("m %d at %f %f %f %f\n", m, xyzs[0], xyzs[1], xyzs[2], xyzs[3]);
  if (m < n) {           /* underdetermined, will end badly */
    return REF_DIV_ZERO; /* signal cloud growth required */
  }
  ref_malloc(a, m * n, REF_DBL);
  ref_malloc(q, m * n, REF_DBL);
  ref_malloc(r, n * n, REF_DBL);
  i = 0;
  if (twod) {
    dx = 0;
    dy = 0;
    dz = 1;
    geom[0] = 0.5 * dx * dx;
    geom[1] = dx * dy;
    geom[2] = dx * dz;
    geom[3] = 0.5 * dy * dy;
    geom[4] = dy * dz;
    geom[5] = 0.5 * dz * dz;
    geom[6] = dx;
    geom[7] = dy;
    geom[8] = dz;
    for (j = 0; j < n; j++) {
      a[i + m * j] = geom[j];
      if (verbose) printf(" %12.4e", geom[j]);
    }
    if (verbose) printf(" %f %f %f %d\n", dx, dy, dz, i);
    i++;
    dx = 0;
    dy = 0;
    dz = 2;
    geom[0] = 0.5 * dx * dx;
    geom[1] = dx * dy;
    geom[2] = dx * dz;
    geom[3] = 0.5 * dy * dy;
    geom[4] = dy * dz;
    geom[5] = 0.5 * dz * dz;
    geom[6] = dx;
    geom[7] = dy;
    geom[8] = dz;
    for (j = 0; j < n; j++) {
      a[i + m * j] = geom[j];
      if (verbose) printf(" %12.4e", geom[j]);
    }
    if (verbose) printf(" %f %f %f %d\n", dx, dy, dz, i);
    i++;
    dx = 1;
    dy = 0;
    dz = 1;
    geom[0] = 0.5 * dx * dx;
    geom[1] = dx * dy;
    geom[2] = dx * dz;
    geom[3] = 0.5 * dy * dy;
    geom[4] = dy * dz;
    geom[5] = 0.5 * dz * dz;
    geom[6] = dx;
    geom[7] = dy;
    geom[8] = dz;
    for (j = 0; j < n; j++) {
      a[i + m * j] = geom[j];
      if (verbose) printf(" %12.4e", geom[j]);
    }
    if (verbose) printf(" %f %f %f %d\n", dx, dy, dz, i);
    i++;
    dx = 0;
    dy = 1;
    dz = 1;
    geom[0] = 0.5 * dx * dx;
    geom[1] = dx * dy;
    geom[2] = dx * dz;
    geom[3] = 0.5 * dy * dy;
    geom[4] = dy * dz;
    geom[5] = 0.5 * dz * dz;
    geom[6] = dx;
    geom[7] = dy;
    geom[8] = dz;
    for (j = 0; j < n; j++) {
      a[i + m * j] = geom[j];
      if (verbose) printf(" %12.4e", geom[j]);
    }
    if (verbose) printf(" %f %f %f %d\n", dx, dy, dz, i);
    i++;
  }
  each_ref_cloud_global(ref_cloud, item, cloud_global) {
    if (center_global == cloud_global) continue; /* skip self */
    dx = ref_cloud_aux(ref_cloud, 0, item) - xyzs[0];
    dy = ref_cloud_aux(ref_cloud, 1, item) - xyzs[1];
    dz = ref_cloud_aux(ref_cloud, 2, item) - xyzs[2];
    geom[0] = 0.5 * dx * dx;
    geom[1] = dx * dy;
    geom[2] = dx * dz;
    geom[3] = 0.5 * dy * dy;
    geom[4] = dy * dz;
    geom[5] = 0.5 * dz * dz;
    geom[6] = dx;
    geom[7] = dy;
    geom[8] = dz;
    for (j = 0; j < n; j++) {
      a[i + m * j] = geom[j];
      if (verbose) printf(" %12.4e", geom[j]);
    }
    if (verbose) printf(" %f %f %f %d\n", dx, dy, dz, i);
    i++;
  }
  REIS(m, i, "A row miscount");
  RSS(ref_matrix_qr(m, n, a, q, r), "kexact lsq hess qr");
  if (verbose) RSS(ref_matrix_show_aqr(m, n, a, q, r), "show qr");
  for (i = 0; i < 90; i++) ab[i] = 0.0;
  for (i = 0; i < 9; i++) {
    for (j = 0; j < 9; j++) {
      ab[i + 9 * j] += r[i + 9 * j];
    }
  }
  i = 0;
  if (twod) {
    dq = 0;
    for (j = 0; j < 9; j++) {
      ab[j + 9 * 9] += q[i + m * j] * dq;
    }
    i++;
    dq = 0;
    for (j = 0; j < 9; j++) {
      ab[j + 9 * 9] += q[i + m * j] * dq;
    }
    i++;
    dq = 0;
    for (j = 0; j < 9; j++) {
      ab[j + 9 * 9] += q[i + m * j] * dq;
    }
    i++;
    dq = 0;
    for (j = 0; j < 9; j++) {
      ab[j + 9 * 9] += q[i + m * j] * dq;
    }
    i++;
  }
  each_ref_cloud_global(ref_cloud, item, cloud_global) {
    if (center_global == cloud_global) continue; /* skip self */
    dq = ref_cloud_aux(ref_cloud, 3, item) - xyzs[3];
    for (j = 0; j < 9; j++) {
      ab[j + 9 * 9] += q[i + m * j] * dq;
    }
    i++;
  }
  REIS(m, i, "b row miscount");
  if (verbose) RSS(ref_matrix_show_ab(9, 10, ab), "show");
  RAISE(ref_matrix_solve_ab(9, 10, ab));
  if (verbose) RSS(ref_matrix_show_ab(9, 10, ab), "show");
  j = 9;
  for (im = 0; im < 6; im++) {
    hessian[im] = ab[im + 9 * j];
  }
  for (im = 0; im < 3; im++) {
    gradient[im] = ab[im + 6 + 9 * j];
  }
  ref_free(r);
  ref_free(q);
  ref_free(a);

  return REF_SUCCESS;
}

static REF_STATUS ref_recon_kexact_center(REF_DBL *xyz, REF_CLOUD ref_cloud,
                                          REF_DBL *center) {
  REF_DBL geom[10], ab[110];
  REF_DBL dx, dy, dz;
  REF_DBL *a, *q, *r;
  REF_INT m, n;
  REF_INT item, i, j;
  REF_BOOL verbose = REF_FALSE;
  REF_STATUS ref_status;

  /* solve A with QR factorization size m x n */
  m = ref_cloud_n(ref_cloud);
  n = 10;
  if (verbose) printf("m %d at %f %f %f\n", m, xyz[0], xyz[1], xyz[2]);
  if (m < n) {           /* underdetermined, will end badly */
    return REF_DIV_ZERO; /* signal cloud growth required */
  }
  ref_malloc(a, m * n, REF_DBL);
  ref_malloc(q, m * n, REF_DBL);
  ref_malloc(r, n * n, REF_DBL);
  i = 0;
  each_ref_cloud_item(ref_cloud, item) {
    dx = ref_cloud_aux(ref_cloud, 0, item) - xyz[0];
    dy = ref_cloud_aux(ref_cloud, 1, item) - xyz[1];
    dz = ref_cloud_aux(ref_cloud, 2, item) - xyz[2];
    geom[0] = 0.5 * dx * dx;
    geom[1] = dx * dy;
    geom[2] = dx * dz;
    geom[3] = 0.5 * dy * dy;
    geom[4] = dy * dz;
    geom[5] = 0.5 * dz * dz;
    geom[6] = dx;
    geom[7] = dy;
    geom[8] = dz;
    geom[9] = 1.0;
    for (j = 0; j < n; j++) {
      a[i + m * j] = geom[j];
      if (verbose) printf(" %12.4e", geom[j]);
    }
    if (verbose) printf(" %f %f %f %d\n", dx, dy, dz, i);
    i++;
  }
  REIB(m, i, "A row miscount", {
    ref_free(r);
    ref_free(q);
    ref_free(a);
  });
  RSB(ref_matrix_qr(m, n, a, q, r), "kexact lsq hess qr", {
    ref_free(r);
    ref_free(q);
    ref_free(a);
  });
  if (verbose)
    RSB(ref_matrix_show_aqr(m, n, a, q, r), "show qr", {
      ref_free(r);
      ref_free(q);
      ref_free(a);
    });
  for (i = 0; i < n * (n + 1); i++) ab[i] = 0.0;
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      ab[i + n * j] += r[i + n * j];
    }
  }
  i = 0;
  each_ref_cloud_item(ref_cloud, item) {
    for (j = 0; j < n; j++) {
      ab[j + n * n] += q[i + m * j] * ref_cloud_aux(ref_cloud, 3, item);
    }
    i++;
  }
  REIB(m, i, "b row miscount", {
    ref_free(r);
    ref_free(q);
    ref_free(a);
  });
  if (verbose)
    RSB(ref_matrix_show_ab(n, n + 1, ab), "show", {
      ref_free(r);
      ref_free(q);
      ref_free(a);
    });
  ref_status = ref_matrix_solve_ab(n, n + 1, ab);
  if (REF_SUCCESS != ref_status) {
    ref_free(r);
    ref_free(q);
    ref_free(a);
    return ref_status;
  }
  if (verbose)
    RSB(ref_matrix_show_ab(n, n + 1, ab), "show", {
      ref_free(r);
      ref_free(q);
      ref_free(a);
    });
  j = n;
  i = n - 1;
  *center = ab[i + n * j];

  ref_free(r);
  ref_free(q);
  ref_free(a);

  return REF_SUCCESS;
}

static REF_STATUS ref_recon_local_immediate_cloud(REF_CLOUD *one_layer,
                                                  REF_NODE ref_node,
                                                  REF_CELL ref_cell,
                                                  REF_DBL *scalar) {
  REF_INT node, item, cell, cell_node, target;
  REF_GLOB global;
  REF_DBL xyzs[4];
  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      each_ref_cell_having_node(ref_cell, node, item, cell) {
        each_ref_cell_cell_node(ref_cell, cell_node) {
          target = ref_cell_c2n(ref_cell, cell_node, cell);
          global = ref_node_global(ref_node, target);
          xyzs[0] = ref_node_xyz(ref_node, 0, target);
          xyzs[1] = ref_node_xyz(ref_node, 1, target);
          xyzs[2] = ref_node_xyz(ref_node, 2, target);
          xyzs[3] = scalar[target];
          RSS(ref_cloud_store(one_layer[node], global, xyzs),
              "store could stencil");
        }
      }
    }
  }
  return REF_SUCCESS;
}

static REF_STATUS ref_recon_local_immediate_cloud_geom(REF_CLOUD *one_layer,
                                                       REF_GRID ref_grid,
                                                       REF_INT id) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_FACELIFT ref_facelift = ref_geom_facelift(ref_geom);
  REF_INT node, item, cell, cell_node, target, geom;
  REF_GLOB global;
  REF_DBL xyzs[4];
  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      each_ref_cell_having_node(ref_cell, node, item, cell) {
        if (id == ref_cell_c2n(ref_cell, ref_cell_id_index(ref_cell), cell)) {
          each_ref_cell_cell_node(ref_cell, cell_node) {
            target = ref_cell_c2n(ref_cell, cell_node, cell);
            global = ref_node_global(ref_node, target);
            xyzs[0] = ref_node_xyz(ref_node, 0, target);
            xyzs[1] = ref_node_xyz(ref_node, 1, target);
            xyzs[2] = ref_node_xyz(ref_node, 2, target);
            RSS(ref_geom_find(ref_geom, target, REF_GEOM_FACE, id, &geom),
                "find geom");
            xyzs[3] = ref_facelift_distance(ref_facelift, geom);
            RSS(ref_cloud_store(one_layer[node], global, xyzs),
                "store could stencil");
          }
        }
      }
    }
  }
  return REF_SUCCESS;
}

REF_STATUS ref_recon_ghost_cloud(REF_CLOUD *one_layer, REF_NODE ref_node) {
  REF_MPI ref_mpi = ref_node_mpi(ref_node);
  REF_CLOUD ref_cloud;
  REF_INT *a_nnode, *b_nnode;
  REF_INT a_nnode_total, b_nnode_total;
  REF_GLOB *a_global, *b_global;
  REF_INT *a_part, *b_part;
  REF_INT *a_ncloud, *b_ncloud;
  REF_INT a_ncloud_total, b_ncloud_total;
  REF_GLOB *a_keyval, *b_keyval;
  REF_DBL *a_aux, *b_aux;
  REF_INT part, node, degree;
  REF_INT *a_next, *b_next;
  REF_INT local, cloud, key_index, aux_index;
  REF_GLOB global;
  REF_INT nkeyval = 2, naux = 4;

  if (!ref_mpi_para(ref_mpi)) return REF_SUCCESS;

  ref_malloc_init(a_next, ref_mpi_n(ref_mpi), REF_INT, 0);
  ref_malloc_init(b_next, ref_mpi_n(ref_mpi), REF_INT, 0);
  ref_malloc_init(a_nnode, ref_mpi_n(ref_mpi), REF_INT, 0);
  ref_malloc_init(b_nnode, ref_mpi_n(ref_mpi), REF_INT, 0);
  ref_malloc_init(a_ncloud, ref_mpi_n(ref_mpi), REF_INT, 0);
  ref_malloc_init(b_ncloud, ref_mpi_n(ref_mpi), REF_INT, 0);

  /* ghost nodes need to fill */
  each_ref_node_valid_node(ref_node, node) {
    if (ref_mpi_rank(ref_mpi) != ref_node_part(ref_node, node)) {
      a_nnode[ref_node_part(ref_node, node)]++;
    }
  }

  RSS(ref_mpi_alltoall(ref_mpi, a_nnode, b_nnode, REF_INT_TYPE),
      "alltoall nnodes");

  a_nnode_total = 0;
  each_ref_mpi_part(ref_mpi, part) a_nnode_total += a_nnode[part];
  ref_malloc(a_global, a_nnode_total, REF_GLOB);
  ref_malloc(a_part, a_nnode_total, REF_INT);

  b_nnode_total = 0;
  each_ref_mpi_part(ref_mpi, part) b_nnode_total += b_nnode[part];
  ref_malloc(b_global, b_nnode_total, REF_GLOB);
  ref_malloc(b_part, b_nnode_total, REF_INT);

  a_next[0] = 0;
  each_ref_mpi_worker(ref_mpi, part) {
    a_next[part] = a_next[part - 1] + a_nnode[part - 1];
  }

  each_ref_node_valid_node(ref_node, node) {
    if (ref_mpi_rank(ref_mpi) != ref_node_part(ref_node, node)) {
      part = ref_node_part(ref_node, node);
      a_global[a_next[part]] = ref_node_global(ref_node, node);
      a_part[a_next[part]] = ref_mpi_rank(ref_mpi);
      a_next[ref_node_part(ref_node, node)]++;
    }
  }

  RSS(ref_mpi_alltoallv(ref_mpi, a_global, a_nnode, b_global, b_nnode, 1,
                        REF_GLOB_TYPE),
      "alltoallv global");
  RSS(ref_mpi_alltoallv(ref_mpi, a_part, a_nnode, b_part, b_nnode, 1,
                        REF_INT_TYPE),
      "alltoallv part");

  /* degree of these node cloud to send */
  for (node = 0; node < b_nnode_total; node++) {
    RSS(ref_node_local(ref_node, b_global[node], &local), "g2l");
    part = b_part[node];
    degree = ref_cloud_n(one_layer[local]);
    b_ncloud[part] += degree;
  }

  RSS(ref_mpi_alltoall(ref_mpi, b_ncloud, a_ncloud, REF_INT_TYPE),
      "alltoall nclouds");

  a_ncloud_total = 0;
  each_ref_mpi_part(ref_mpi, part) a_ncloud_total += a_ncloud[part];
  ref_malloc(a_keyval, nkeyval * a_ncloud_total, REF_GLOB);
  ref_malloc(a_aux, naux * a_ncloud_total, REF_DBL);

  b_ncloud_total = 0;
  each_ref_mpi_part(ref_mpi, part) b_ncloud_total += b_ncloud[part];
  ref_malloc(b_keyval, nkeyval * b_ncloud_total, REF_GLOB);
  ref_malloc(b_aux, naux * b_ncloud_total, REF_DBL);

  b_next[0] = 0;
  each_ref_mpi_worker(ref_mpi, part) {
    b_next[part] = b_next[part - 1] + b_ncloud[part - 1];
  }

  for (node = 0; node < b_nnode_total; node++) {
    RSS(ref_node_local(ref_node, b_global[node], &local), "g2l");
    part = b_part[node];
    ref_cloud = one_layer[local];
    each_ref_cloud_item(ref_cloud, key_index) {
      b_keyval[0 + nkeyval * b_next[part]] = b_global[node];
      b_keyval[1 + nkeyval * b_next[part]] =
          ref_cloud_global(ref_cloud, key_index);
      for (aux_index = 0; aux_index < naux; aux_index++) {
        b_aux[aux_index + naux * b_next[part]] =
            ref_cloud_aux(ref_cloud, aux_index, key_index);
      }
      b_next[part]++;
    }
  }

  RSS(ref_mpi_alltoallv(ref_mpi, b_keyval, b_ncloud, a_keyval, a_ncloud,
                        nkeyval, REF_GLOB_TYPE),
      "alltoallv keyval");
  RSS(ref_mpi_alltoallv(ref_mpi, b_aux, b_ncloud, a_aux, a_ncloud, naux,
                        REF_DBL_TYPE),
      "alltoallv aux");

  for (cloud = 0; cloud < a_ncloud_total; cloud++) {
    global = a_keyval[0 + nkeyval * cloud];
    RSS(ref_node_local(ref_node, global, &local), "g2l");
    ref_cloud = one_layer[local];
    RSS(ref_cloud_store(ref_cloud, a_keyval[1 + nkeyval * cloud],
                        &(a_aux[naux * cloud])),
        "add ghost");
  }

  free(b_aux);
  free(b_keyval);
  free(a_aux);
  free(a_keyval);
  free(b_part);
  free(b_global);
  free(a_part);
  free(a_global);
  free(b_ncloud);
  free(a_ncloud);
  free(b_nnode);
  free(a_nnode);
  free(b_next);
  free(a_next);

  return REF_SUCCESS;
}

static REF_STATUS ref_recon_grow_cloud_one_layer(REF_CLOUD ref_cloud,
                                                 REF_CLOUD *one_layer,
                                                 REF_NODE ref_node) {
  REF_CLOUD copy;
  REF_STATUS ref_status;
  REF_INT pivot_index, local_pivot;
  REF_INT add_index, i;
  REF_GLOB global, global_pivot;
  REF_DBL xyzs[4];
  RSS(ref_cloud_deep_copy(&copy, ref_cloud), "copy");
  each_ref_cloud_global(copy, pivot_index, global_pivot) {
    ref_status = ref_node_local(ref_node, global_pivot, &local_pivot);
    if (REF_NOT_FOUND == ref_status) {
      printf("global %ld not found\n", global_pivot);
      continue;
    } else {
      RSS(ref_status, "local search");
    }
    each_ref_cloud_global(one_layer[local_pivot], add_index, global) {
      each_ref_cloud_aux(one_layer[local_pivot], i) {
        xyzs[i] = ref_cloud_aux(one_layer[local_pivot], i, add_index);
      }
      RSS(ref_cloud_store(ref_cloud, global, xyzs), "store stencil increase");
    }
  }
  ref_cloud_free(copy);

  return REF_SUCCESS;
}

static REF_STATUS ref_recon_kexact_gradient_hessian(REF_GRID ref_grid,
                                                    REF_DBL *scalar,
                                                    REF_DBL *gradient,
                                                    REF_DBL *hessian) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_INT node, im;
  REF_CLOUD ref_cloud;
  REF_DBL node_gradient[3], node_hessian[6];
  REF_STATUS status;
  REF_CLOUD *one_layer;
  REF_INT layer;

  if (ref_grid_twod(ref_grid)) ref_cell = ref_grid_tri(ref_grid);

  ref_malloc_init(one_layer, ref_node_max(ref_node), REF_CLOUD, NULL);
  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_cloud_create(&(one_layer[node]), 4), "cloud storage");
  }

  RSS(ref_recon_local_immediate_cloud(one_layer, ref_node, ref_cell, scalar),
      "fill immediate cloud");
  RSS(ref_recon_ghost_cloud(one_layer, ref_node), "fill ghosts");

  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      /* use ref_cloud to get a unique list of halo(2) nodes */
      RSS(ref_cloud_deep_copy(&ref_cloud, one_layer[node]), "create ref_cloud");
      status = REF_INVALID;
      for (layer = 2; status != REF_SUCCESS && layer <= 8; layer++) {
        RSS(ref_recon_grow_cloud_one_layer(ref_cloud, one_layer, ref_node),
            "grow");
        status = ref_recon_kexact_with_aux(ref_node_global(ref_node, node),
                                           ref_cloud, ref_grid_twod(ref_grid),
                                           node_gradient, node_hessian);
        if (REF_NOT_FOUND == status) {
          ref_node_location(ref_node, node);
          printf(
              " caught %s, for %d layers to kexact cloud; "
              "zero gradient and hessian\n",
              "REF_NOT_FOUND", layer);
          status = REF_SUCCESS;
          break;
        }
        if (REF_DIV_ZERO == status && layer > 4) {
          ref_node_location(ref_node, node);
          printf(" caught %s, for %d layers to kexact cloud; retry\n",
                 "REF_DIV_ZERO", layer);
        }
        if (REF_ILL_CONDITIONED == status && layer > 4) {
          ref_node_location(ref_node, node);
          printf(" caught %s, for %d layers to kexact cloud; retry\n",
                 "REF_ILL_CONDITIONED", layer);
        }
      }
      if (NULL != gradient) {
        if (ref_grid_twod(ref_grid)) {
          node_gradient[2] = 0.0;
        }
        for (im = 0; im < 3; im++) {
          gradient[im + 3 * node] = node_gradient[im];
        }
      }
      if (NULL != hessian) {
        if (ref_grid_twod(ref_grid)) {
          node_hessian[2] = 0.0;
          node_hessian[4] = 0.0;
          node_hessian[5] = 0.0;
        }
        for (im = 0; im < 6; im++) {
          hessian[im + 6 * node] = node_hessian[im];
        }
      }
      RSS(ref_cloud_free(ref_cloud), "free ref_cloud");
    }
  }

  each_ref_node_valid_node(ref_node, node) {
    ref_cloud_free(one_layer[node]); /* no-op for null */
  }
  ref_free(one_layer);

  if (NULL != gradient) {
    RSS(ref_node_ghost_dbl(ref_node, gradient, 3), "update ghosts");
  }

  if (NULL != hessian) {
    RSS(ref_node_ghost_dbl(ref_node, hessian, 6), "update ghosts");
  }

  return REF_SUCCESS;
}

REF_STATUS ref_recon_mask_tri(REF_GRID ref_grid, REF_BOOL *replace,
                              REF_INT ldim) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT i, node;

  each_ref_node_valid_node(ref_node, node) {
    for (i = 0; i < ldim; i++) {
      replace[i + ldim * node] = !ref_cell_node_empty(ref_cell, node);
    }
  }
  return REF_SUCCESS;
}

static REF_STATUS ref_recon_mask_edg(REF_GRID ref_grid, REF_BOOL *replace,
                                     REF_INT ldim) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_edg(ref_grid);
  REF_INT i, node;

  each_ref_node_valid_node(ref_node, node) {
    for (i = 0; i < ldim; i++) {
      replace[i + ldim * node] = !ref_cell_node_empty(ref_cell, node);
    }
  }
  return REF_SUCCESS;
}

/* replace with volume weighing of donor dual volume */
REF_STATUS ref_recon_extrapolate_zeroth(REF_GRID ref_grid, REF_DBL *recon,
                                        REF_BOOL *replace, REF_INT ldim) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node, item, edge;
  REF_EDGE ref_edge;
  REF_INT i, neighbor, nint;
  REF_INT pass, remain;

  RSS(ref_edge_create(&ref_edge, ref_grid), "edges");

  RSS(ref_node_ghost_int(ref_node, replace, ldim), "update ghosts");
  RSS(ref_node_ghost_dbl(ref_node, recon, ldim), "update ghosts");

  remain = 1;
  for (pass = 0; pass < 10; pass++) {
    each_ref_node_valid_node(ref_node, node) {
      if (ref_node_owned(ref_node, node)) {
        for (i = 0; i < ldim; i++) {
          if (replace[i + ldim * node]) {
            nint = 0;
            each_edge_having_node(ref_edge, node, item, edge) {
              neighbor = ref_edge_e2n(ref_edge, 0, edge) +
                         ref_edge_e2n(ref_edge, 1, edge) - node;
              if (!replace[i + ldim * neighbor]) nint++;
            }
            if (0 < nint) {
              recon[i + ldim * node] = 0.0;
              each_edge_having_node(ref_edge, node, item, edge) {
                neighbor = ref_edge_e2n(ref_edge, 0, edge) +
                           ref_edge_e2n(ref_edge, 1, edge) - node;
                if (!replace[i + ldim * neighbor]) {
                  recon[i + ldim * node] += recon[i + ldim * neighbor];
                }
              }
              /* use Euclidean average, these are derivatives */
              recon[i + ldim * node] /= (REF_DBL)nint;
              replace[i + ldim * node] = REF_FALSE;
            }
          }
        }
      }
    }

    RSS(ref_node_ghost_int(ref_node, replace, ldim), "update ghosts");
    RSS(ref_node_ghost_dbl(ref_node, recon, ldim), "update ghosts");
    remain = 0;
    each_ref_node_valid_node(ref_node, node) {
      if (ref_node_owned(ref_node, node)) {
        for (i = 0; i < ldim; i++) {
          if (replace[i + ldim * node]) remain++;
        }
      }
    }
    RSS(ref_mpi_allsum(ref_grid_mpi(ref_grid), &remain, 1, REF_INT_TYPE),
        "sum updates");

    if (0 == remain) break;
  }

  ref_edge_free(ref_edge);

  if (0 < remain && ref_mpi_once(ref_grid_mpi(ref_grid))) {
    printf(" %d remain\n", remain);
    REF_WHERE("untouched boundary nodes remain");
  }

  return REF_SUCCESS;
}

static REF_STATUS ref_recon_local_immediate_replace_cloud(
    REF_CLOUD *one_layer, REF_NODE ref_node, REF_CELL ref_cell, REF_DBL *recon,
    REF_BOOL *replace, REF_INT ldim, REF_INT i) {
  REF_INT node, item, cell, cell_node, target;
  REF_GLOB global;
  REF_DBL xyzs[4];
  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      each_ref_cell_having_node(ref_cell, node, item, cell) {
        each_ref_cell_cell_node(ref_cell, cell_node) {
          target = ref_cell_c2n(ref_cell, cell_node, cell);
          if (!replace[i + ldim * target]) {
            global = ref_node_global(ref_node, target);
            xyzs[0] = ref_node_xyz(ref_node, 0, target);
            xyzs[1] = ref_node_xyz(ref_node, 1, target);
            xyzs[2] = ref_node_xyz(ref_node, 2, target);
            xyzs[3] = recon[i + ldim * target];
            RSS(ref_cloud_store(one_layer[node], global, xyzs),
                "store cloud stencil");
          }
        }
      }
    }
  }
  return REF_SUCCESS;
}
REF_STATUS ref_recon_extrapolate_kexact(REF_GRID ref_grid, REF_DBL *recon,
                                        REF_BOOL *replace, REF_INT ldim) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_INT i, node;
  REF_CLOUD *one_layer, ref_cloud;
  REF_DBL center;
  REF_STATUS status;
  REF_INT layer;
  if (ref_grid_twod(ref_grid)) ref_cell = ref_grid_tri(ref_grid);

  RSS(ref_node_ghost_int(ref_node, replace, ldim), "update ghosts");
  RSS(ref_node_ghost_dbl(ref_node, recon, ldim), "update ghosts");

  for (i = 0; i < ldim; i++) {
    ref_malloc_init(one_layer, ref_node_max(ref_node), REF_CLOUD, NULL);
    each_ref_node_valid_node(ref_node, node) {
      RSS(ref_cloud_create(&(one_layer[node]), 4), "cloud storage");
    }
    RSS(ref_recon_local_immediate_replace_cloud(one_layer, ref_node, ref_cell,
                                                recon, replace, ldim, i),
        "fill");
    RSS(ref_recon_ghost_cloud(one_layer, ref_node), "fill ghosts");
    each_ref_node_valid_node(ref_node, node) {
      if (ref_node_owned(ref_node, node) && replace[i + ldim * node]) {
        RSS(ref_cloud_deep_copy(&ref_cloud, one_layer[node]),
            "create ref_cloud");
        status = REF_INVALID;
        for (layer = 2; status != REF_SUCCESS && layer <= 8; layer++) {
          status = ref_recon_kexact_center(ref_node_xyz_ptr(ref_node, node),
                                           ref_cloud, &center);
          if (REF_SUCCESS == status) {
            recon[i + ldim * node] = center;
            replace[i + ldim * node] = REF_FALSE;
          } else {
            RSS(ref_recon_grow_cloud_one_layer(ref_cloud, one_layer, ref_node),
                "grow");
          }
        }
        RSS(ref_cloud_free(ref_cloud), "free ref_cloud");
      }
    }

    each_ref_node_valid_node(ref_node, node) {
      ref_cloud_free(one_layer[node]); /* no-op for null */
    }
    ref_free(one_layer);
  }

  RSS(ref_node_ghost_int(ref_node, replace, ldim), "update ghosts");
  RSS(ref_node_ghost_dbl(ref_node, recon, ldim), "update ghosts");

  RSS(ref_recon_extrapolate_zeroth(ref_grid, recon, replace, ldim), "zero");

  return REF_SUCCESS;
}

REF_STATUS ref_recon_roundoff_limit(REF_DBL *recon, REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node, node0, node1, edge;
  REF_DBL *radius, dist;
  REF_DBL round_off_jitter = 1.0e-12;
  REF_DBL eig_floor;
  REF_DBL diag_system[12];
  REF_EDGE ref_edge;

  ref_malloc_init(radius, ref_node_max(ref_node), REF_DBL, -1.0);
  RSS(ref_edge_create(&ref_edge, ref_grid), "edges");
  for (edge = 0; edge < ref_edge_n(ref_edge); edge++) {
    node0 = ref_edge_e2n(ref_edge, 0, edge);
    node1 = ref_edge_e2n(ref_edge, 1, edge);
    dist = sqrt(
        pow(ref_node_xyz(ref_node, 0, node1) - ref_node_xyz(ref_node, 0, node0),
            2) +
        pow(ref_node_xyz(ref_node, 1, node1) - ref_node_xyz(ref_node, 1, node0),
            2) +
        pow(ref_node_xyz(ref_node, 2, node1) - ref_node_xyz(ref_node, 2, node0),
            2));
    if (radius[node0] < 0.0) radius[node0] = dist;
    radius[node0] = MIN(radius[node0], dist);
    if (radius[node1] < 0.0) radius[node1] = dist;
    radius[node1] = MIN(radius[node1], dist);
  }
  ref_edge_free(ref_edge);

  each_ref_node_valid_node(ref_node, node) {
    /* 2nd order central finite difference */
    RAS(ref_math_divisible((4 * round_off_jitter),
                           (radius[node] * radius[node])),
        "element with zero edge length or missing element");
    eig_floor = (4 * round_off_jitter) / (radius[node] * radius[node]);

    RSS(ref_matrix_diag_m(&(recon[6 * node]), diag_system), "eigen decomp");
    ref_matrix_eig(diag_system, 0) =
        MAX(ref_matrix_eig(diag_system, 0), eig_floor);
    ref_matrix_eig(diag_system, 1) =
        MAX(ref_matrix_eig(diag_system, 1), eig_floor);
    ref_matrix_eig(diag_system, 2) =
        MAX(ref_matrix_eig(diag_system, 2), eig_floor);
    RSS(ref_matrix_form_m(diag_system, &(recon[6 * node])), "re-form hess");
  }
  ref_free(radius);

  RSS(ref_node_ghost_dbl(ref_node, recon, 6), "update ghosts");

  return REF_SUCCESS;
}

REF_STATUS ref_recon_max_jump_limit(REF_DBL *recon, REF_GRID ref_grid,
                                    REF_DBL max_jump) {
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT i, node;
  REF_DBL radius, dist;
  REF_DBL eig_ceiling;
  REF_INT nnode, node_list[REF_RECON_MAX_DEGREE],
      max_node = REF_RECON_MAX_DEGREE;
  REF_DBL diag_system[12];

  if (ref_grid_twod(ref_grid)) ref_cell = ref_grid_tri(ref_grid);

  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_cell_node_list_around(ref_cell, node, max_node, &nnode, node_list),
        "first halo of nodes");
    radius = 0.0;
    for (i = 0; i < nnode; i++) {
      dist = sqrt(pow(ref_node_xyz(ref_node, 0, node_list[i]) -
                          ref_node_xyz(ref_node, 0, node),
                      2) +
                  pow(ref_node_xyz(ref_node, 1, node_list[i]) -
                          ref_node_xyz(ref_node, 1, node),
                      2) +
                  pow(ref_node_xyz(ref_node, 2, node_list[i]) -
                          ref_node_xyz(ref_node, 2, node),
                      2));
      if (i == 0) radius = dist;
      radius = MIN(radius, dist);
    }
    /* 2nd order central finite difference */
    if (ref_math_divisible(4 * max_jump, radius * radius)) {
      eig_ceiling = (4 * max_jump) / (radius * radius);

      RSS(ref_matrix_diag_m(&(recon[6 * node]), diag_system), "eigen decomp");
      ref_matrix_eig(diag_system, 0) =
          MIN(ref_matrix_eig(diag_system, 0), eig_ceiling);
      ref_matrix_eig(diag_system, 1) =
          MIN(ref_matrix_eig(diag_system, 1), eig_ceiling);
      ref_matrix_eig(diag_system, 2) =
          MIN(ref_matrix_eig(diag_system, 2), eig_ceiling);
      RSS(ref_matrix_form_m(diag_system, &(recon[6 * node])), "re-form hess");
    }
  }

  RSS(ref_node_ghost_dbl(ref_node, recon, 6), "update ghosts");

  return REF_SUCCESS;
}

REF_STATUS ref_recon_gradient(REF_GRID ref_grid, REF_DBL *scalar, REF_DBL *grad,
                              REF_RECON_RECONSTRUCTION recon) {
  switch (recon) {
    case REF_RECON_L2PROJECTION:
      RSS(ref_recon_l2_projection_grad(ref_grid, scalar, grad), "l2");
      break;
    case REF_RECON_KEXACT:
      RSS(ref_recon_kexact_gradient_hessian(ref_grid, scalar, grad, NULL),
          "k-exact");
      break;
    case REF_RECON_LAST:
    default:
      THROW("reconstruction not available");
  }

  return REF_SUCCESS;
}

REF_STATUS ref_recon_abs_value_hessian(REF_GRID ref_grid, REF_DBL *hessian) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_DBL diag_system[12];
  REF_INT node;

  /* positive eigenvalues to make positive definite */
  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      RSS(ref_matrix_diag_m(&(hessian[6 * node]), diag_system), "decomp");
      ref_matrix_eig(diag_system, 0) = ABS(ref_matrix_eig(diag_system, 0));
      ref_matrix_eig(diag_system, 1) = ABS(ref_matrix_eig(diag_system, 1));
      ref_matrix_eig(diag_system, 2) = ABS(ref_matrix_eig(diag_system, 2));
      RSS(ref_matrix_form_m(diag_system, &(hessian[6 * node])), "re-form");
    }
  }

  RSS(ref_node_ghost_dbl(ref_node, hessian, 6), "update ghosts");

  return REF_SUCCESS;
}

static REF_STATUS ref_recon_skip_orphan_replace(REF_GRID ref_grid,
                                                REF_BOOL *replace,
                                                REF_INT ldim) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_EDGE ref_edge;
  REF_INT *contributions;
  REF_INT i, edge, node0, node1, node;
  ref_malloc_init(contributions, 6 * ref_node_max(ref_node), REF_INT, 0);
  RSS(ref_edge_create(&ref_edge, ref_grid), "edges");
  for (edge = 0; edge < ref_edge_n(ref_edge); edge++) {
    node0 = ref_edge_e2n(ref_edge, 0, edge);
    node1 = ref_edge_e2n(ref_edge, 1, edge);
    for (i = 0; i < ldim; i++) {
      if (replace[i + ldim * node0]) {
        if (!replace[i + ldim * node1]) contributions[i + ldim * node0]++;
      }
      if (replace[i + ldim * node1]) {
        if (!replace[i + ldim * node0]) contributions[i + ldim * node1]++;
      }
    }
  }
  RSS(ref_edge_free(ref_edge), "free edge");
  each_ref_node_valid_node(ref_node, node) {
    for (i = 0; i < ldim; i++) {
      if (0 == contributions[i + ldim * node])
        replace[i + ldim * node] = REF_FALSE;
    }
  }
  ref_free(contributions);
  return REF_SUCCESS;
}

REF_STATUS ref_recon_signed_hessian(REF_GRID ref_grid, REF_DBL *scalar,
                                    REF_DBL *hessian,
                                    REF_RECON_RECONSTRUCTION recon) {
  REF_BOOL *replace;

  switch (recon) {
    case REF_RECON_L2PROJECTION:
      RSS(ref_recon_l2_projection_hessian(ref_grid, scalar, hessian), "l2");
      ref_malloc(replace, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_BOOL);
      if (ref_grid_twod(ref_grid)) {
        RSS(ref_recon_mask_edg(ref_grid, replace, 6), "mask edg");
      } else {
        RSS(ref_recon_mask_tri(ref_grid, replace, 6), "mask tri");
      }
      RSS(ref_recon_skip_orphan_replace(ref_grid, replace, 6), "skip orphans");
      RSS(ref_recon_extrapolate_zeroth(ref_grid, hessian, replace, 6),
          "bound extrap");
      ref_free(replace);
      break;
    case REF_RECON_KEXACT:
      RSS(ref_recon_kexact_gradient_hessian(ref_grid, scalar, NULL, hessian),
          "k-exact");
      break;
    case REF_RECON_LAST:
    default:
      THROW("reconstruction not available");
  }

  return REF_SUCCESS;
}

REF_STATUS ref_recon_hessian(REF_GRID ref_grid, REF_DBL *scalar,
                             REF_DBL *hessian, REF_RECON_RECONSTRUCTION recon) {
  RSS(ref_recon_signed_hessian(ref_grid, scalar, hessian, recon), "abs hess");
  RSS(ref_recon_abs_value_hessian(ref_grid, hessian), "abs hess");
  return REF_SUCCESS;
}

REF_STATUS ref_recon_normal(REF_GRID ref_grid, REF_INT node, REF_DBL *normal) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT item, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL total, area, tri_norm[3];

  normal[0] = 0.0;
  normal[1] = 0.0;
  normal[2] = 0.0;
  total = 0.0;
  each_ref_cell_having_node(ref_cell, node, item, cell) {
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "cell nodes");
    RSS(ref_node_tri_normal(ref_node, nodes, tri_norm), "cell nodes");
    RSS(ref_node_tri_area(ref_node, nodes, &area), "area");
    normal[0] += area * tri_norm[0];
    normal[1] += area * tri_norm[1];
    normal[2] += area * tri_norm[2];
    total += area;
  }
  if (ref_math_divisible(normal[0], total) &&
      ref_math_divisible(normal[1], total) &&
      ref_math_divisible(normal[2], total)) {
    normal[0] /= total;
    normal[1] /= total;
    normal[2] /= total;
    RSS(ref_math_normalize(normal), "norm");
  } else {
    normal[0] = 0.0;
    normal[1] = 0.0;
    normal[2] = 0.0;
    return REF_DIV_ZERO;
  }
  return REF_SUCCESS;
}

REF_STATUS ref_recon_rsn(REF_GRID ref_grid, REF_INT node, REF_DBL *r,
                         REF_DBL *s, REF_DBL *n) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT item, cell, nodes[REF_CELL_MAX_SIZE_PER], cell_node;
  REF_DBL small_dot, dot;
  REF_DBL dr[3];

  r[0] = 0.0;
  r[1] = 0.0;
  r[2] = 0.0;

  small_dot = 2.0;
  RSS(ref_recon_normal(ref_grid, node, n), "norm");
  each_ref_cell_having_node(ref_cell, node, item, cell) {
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "cell nodes");
    each_ref_cell_cell_node(ref_cell, cell_node) {
      if (node != nodes[cell_node]) {
        dr[0] = ref_node_xyz(ref_node, 0, nodes[cell_node]) -
                ref_node_xyz(ref_node, 0, node);
        dr[1] = ref_node_xyz(ref_node, 1, nodes[cell_node]) -
                ref_node_xyz(ref_node, 1, node);
        dr[2] = ref_node_xyz(ref_node, 2, nodes[cell_node]) -
                ref_node_xyz(ref_node, 2, node);
        RSS(ref_math_normalize(dr), "norm dr");
        dot = ABS(ref_math_dot(dr, n));
        if (dot < small_dot) {
          small_dot = dot;
          r[0] = dr[0];
          r[1] = dr[1];
          r[2] = dr[2];
        }
      }
    }
  }

  dot = ref_math_dot(r, n);
  r[0] -= dot * n[0];
  r[1] -= dot * n[1];
  r[2] -= dot * n[2];
  RSS(ref_math_normalize(r), "norm r");

  ref_math_cross_product(n, r, s);

  return REF_SUCCESS;
}

REF_STATUS ref_recon_kexact_rs(REF_GLOB center_global, REF_CLOUD ref_cloud,
                               REF_DBL *rn, REF_DBL *sn, REF_DBL *hessian) {
  REF_DBL geom[3], ab[12];
  REF_DBL dxyz[3], dq, dr, ds;
  REF_DBL *a, *q, *r;
  REF_INT m, n;
  REF_GLOB cloud_global;
  REF_INT item, im, i, j;
  REF_DBL xyzs[4];
  REF_BOOL verbose = REF_FALSE;

  RSS(ref_cloud_item(ref_cloud, center_global, &item), "missing center");
  each_ref_cloud_aux(ref_cloud, i) {
    xyzs[i] = ref_cloud_aux(ref_cloud, i, item);
  }
  /* solve A with QR factorization size m x n */
  m = ref_cloud_n(ref_cloud) - 1; /* skip self */
  n = 3;
  if (verbose)
    printf("m %d at %f %f %f %f\n", m, xyzs[0], xyzs[1], xyzs[2], xyzs[3]);
  if (verbose)
    printf("r %f %f %f\ns %f %f %f\n", rn[0], rn[1], rn[2], sn[0], sn[1],
           sn[2]);
  if (m < n) {           /* underdetermined, will end badly */
    return REF_DIV_ZERO; /* signal cloud growth required */
  }
  ref_malloc(a, m * n, REF_DBL);
  ref_malloc(q, m * n, REF_DBL);
  ref_malloc(r, n * n, REF_DBL);
  i = 0;
  each_ref_cloud_global(ref_cloud, item, cloud_global) {
    if (center_global == cloud_global) continue; /* skip self */
    dxyz[0] = ref_cloud_aux(ref_cloud, 0, item) - xyzs[0];
    dxyz[1] = ref_cloud_aux(ref_cloud, 1, item) - xyzs[1];
    dxyz[2] = ref_cloud_aux(ref_cloud, 2, item) - xyzs[2];
    dr = ref_math_dot(dxyz, rn);
    ds = ref_math_dot(dxyz, sn);
    geom[0] = 0.5 * dr * dr;
    geom[1] = dr * ds;
    geom[2] = 0.5 * ds * ds;
    for (j = 0; j < n; j++) {
      a[i + m * j] = geom[j];
      if (verbose) printf(" %12.4e", geom[j]);
    }
    if (verbose) printf(" %f %f %d\n", dr, ds, i);
    i++;
  }
  REIS(m, i, "A row miscount");
  RSS(ref_matrix_qr(m, n, a, q, r), "kexact lsq hess qr");
  if (verbose) RSS(ref_matrix_show_aqr(m, n, a, q, r), "show qr");
  for (i = 0; i < (n + 1) * n; i++) ab[i] = 0.0;
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      ab[i + n * j] += r[i + n * j];
    }
  }
  i = 0;
  each_ref_cloud_global(ref_cloud, item, cloud_global) {
    if (center_global == cloud_global) continue; /* skip self */
    dq = ref_cloud_aux(ref_cloud, 3, item) - xyzs[3];
    for (j = 0; j < n; j++) {
      ab[j + n * n] += q[i + m * j] * dq;
    }
    i++;
  }
  REIS(m, i, "b row miscount");
  if (verbose) RSS(ref_matrix_show_ab(n, n + 1, ab), "show");
  RAISE(ref_matrix_solve_ab(n, n + 1, ab));
  if (verbose) RSS(ref_matrix_show_ab(n, n + 1, ab), "show");
  j = n;
  for (im = 0; im < 3; im++) {
    hessian[im] = ab[im + n * j];
  }
  ref_free(r);
  ref_free(q);
  ref_free(a);

  return REF_SUCCESS;
}

static REF_STATUS ref_recon_abs_value_hessian2(REF_GRID ref_grid,
                                               REF_DBL *hessian) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_DBL diag_system[6];
  REF_INT node;

  /* positive eigenvalues to make positive definite */
  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      RSS(ref_matrix_diag_m2(&(hessian[3 * node]), diag_system), "decomp");
      ref_matrix_eig2(diag_system, 0) = ABS(ref_matrix_eig2(diag_system, 0));
      ref_matrix_eig2(diag_system, 1) = ABS(ref_matrix_eig2(diag_system, 1));
      RSS(ref_matrix_form_m2(diag_system, &(hessian[3 * node])), "re-form");
    }
  }

  RSS(ref_node_ghost_dbl(ref_node, hessian, 3), "update ghosts");

  return REF_SUCCESS;
}

REF_STATUS ref_recon_rsn_hess(REF_GRID ref_grid, REF_DBL *scalar,
                              REF_DBL *hessian) {
  REF_CLOUD *one_layer;
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT node, im;
  REF_CLOUD ref_cloud;
  REF_DBL r[3], s[3], n[3];
  REF_DBL node_hessian[3];
  REF_STATUS status;
  REF_INT layer;

  ref_malloc_init(one_layer, ref_node_max(ref_node), REF_CLOUD, NULL);
  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_cloud_create(&(one_layer[node]), 4), "cloud storage");
  }
  RSS(ref_recon_local_immediate_cloud(one_layer, ref_node, ref_cell, scalar),
      "fill immediate cloud");
  RSS(ref_recon_ghost_cloud(one_layer, ref_node), "fill ghosts");

  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      RSS(ref_recon_rsn(ref_grid, node, r, s, n), "rsn");
      /* use ref_cloud to get a unique list of halo(2) nodes */
      RSS(ref_cloud_deep_copy(&ref_cloud, one_layer[node]), "create ref_cloud");
      status = REF_INVALID;
      for (layer = 2; status != REF_SUCCESS && layer <= 8; layer++) {
        RSS(ref_recon_grow_cloud_one_layer(ref_cloud, one_layer, ref_node),
            "grow");

        status = ref_recon_kexact_rs(ref_node_global(ref_node, node), ref_cloud,
                                     r, s, node_hessian);
        if (REF_DIV_ZERO == status && layer > 4) {
          ref_node_location(ref_node, node);
          printf(" caught %s, for %d layers to kexact cloud; retry\n",
                 "REF_DIV_ZERO", layer);
        }
        if (REF_ILL_CONDITIONED == status && layer > 4) {
          ref_node_location(ref_node, node);
          printf(" caught %s, for %d layers to kexact cloud; retry\n",
                 "REF_ILL_CONDITIONED", layer);
        }
      }
      RSB(status, "kexact qr node", { ref_node_location(ref_node, node); });
      for (im = 0; im < 3; im++) {
        hessian[im + 3 * node] = node_hessian[im];
      }
      RSS(ref_cloud_free(ref_cloud), "free ref_cloud");
    }
  }

  each_ref_node_valid_node(ref_node, node) {
    ref_cloud_free(one_layer[node]); /* no-op for null */
  }
  ref_free(one_layer);

  RSS(ref_node_ghost_dbl(ref_node, hessian, 3), "update ghosts");

  RSS(ref_recon_abs_value_hessian2(ref_grid, hessian), "abs(H)");

  return REF_SUCCESS;
}

REF_STATUS ref_recon_rsn_hess_face(REF_GRID ref_grid, REF_DBL *hessian) {
  REF_CLOUD *one_layer;
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT node, im, min_id, max_id, id;
  REF_CLOUD ref_cloud;
  REF_DBL r[3], s[3], n[3];
  REF_DBL node_hessian[3];
  REF_STATUS status;
  REF_INT layer;
  REF_BOOL has_support;

  each_ref_node_valid_node(ref_node, node) {
    hessian[0 + 3 * node] = 0.0;
    hessian[1 + 3 * node] = 0.0;
    hessian[2 + 3 * node] = 0.0;
  }

  ref_malloc_init(one_layer, ref_node_max(ref_node), REF_CLOUD, NULL);
  RSS(ref_cell_id_range(ref_cell, ref_grid_mpi(ref_grid), &min_id, &max_id),
      "face id range");

  for (id = min_id; id <= max_id; id++) {
    each_ref_node_valid_node(ref_node, node) {
      RSS(ref_cloud_create(&(one_layer[node]), 4), "cloud storage");
    }
    RSS(ref_recon_local_immediate_cloud_geom(one_layer, ref_grid, id),
        "fill immediate cloud");
    RSS(ref_recon_ghost_cloud(one_layer, ref_node), "fill ghosts");

    each_ref_node_valid_node(ref_node, node) {
      RSS(ref_geom_id_supported(ref_grid_geom(ref_grid), node, REF_GEOM_FACE,
                                id, &has_support),
          "geom support");
      if (has_support && ref_node_owned(ref_node, node)) {
        RSS(ref_recon_rsn(ref_grid, node, r, s, n), "rsn");
        /* use ref_cloud to get a unique list of halo(2) nodes */
        RSS(ref_cloud_deep_copy(&ref_cloud, one_layer[node]),
            "create ref_cloud");
        status = REF_INVALID;
        for (layer = 2; status != REF_SUCCESS && layer <= 8; layer++) {
          RSS(ref_recon_grow_cloud_one_layer(ref_cloud, one_layer, ref_node),
              "grow");

          status = ref_recon_kexact_rs(ref_node_global(ref_node, node),
                                       ref_cloud, r, s, node_hessian);
          if (REF_DIV_ZERO == status && layer > 4) {
            ref_node_location(ref_node, node);
            printf(" caught %s, for %d layers to kexact cloud; retry\n",
                   "REF_DIV_ZERO", layer);
          }
          if (REF_ILL_CONDITIONED == status && layer > 4) {
            ref_node_location(ref_node, node);
            printf(" caught %s, for %d layers to kexact cloud; retry\n",
                   "REF_ILL_CONDITIONED", layer);
          }
        }
        if (REF_SUCCESS == status) {
          if (ref_math_dot(&(hessian[3 * node]), &(hessian[3 * node])) <
              ref_math_dot(node_hessian, node_hessian)) {
            for (im = 0; im < 3; im++) {
              hessian[im + 3 * node] = node_hessian[im];
            }
          }
        } else {
          printf("skip contribution to face %d at ", id);
          RSS(ref_node_location(ref_node, node), "loc");
        }

        RSS(ref_cloud_free(ref_cloud), "free ref_cloud");
      }
    }

    each_ref_node_valid_node(ref_node, node) {
      ref_cloud_free(one_layer[node]); /* no-op for null */
    }
  }
  ref_free(one_layer);

  RSS(ref_node_ghost_dbl(ref_node, hessian, 3), "update ghosts");

  RSS(ref_recon_abs_value_hessian2(ref_grid, hessian), "abs(H)");

  return REF_SUCCESS;
}
