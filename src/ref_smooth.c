
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

#include "ref_adapt.h"
#include "ref_cell.h"
#include "ref_clump.h"
#include "ref_geom.h"
#include "ref_math.h"
#include "ref_matrix.h"
#include "ref_mpi.h"
#include "ref_smooth.h"
#include "ref_twod.h"

#include "ref_malloc.h"

REF_STATUS ref_smooth_tri_steepest_descent(REF_GRID ref_grid, REF_INT node) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL f, d[3];
  REF_DBL dcost, dcost_dl, dl;
  REF_BOOL verbose = REF_FALSE;

  each_ref_cell_having_node(ref_cell, node, item, cell) {
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "nodes");
    RSS(ref_node_tri_dquality_dnode0(ref_node, nodes, &f, d), "qual deriv");
    if (verbose)
      printf("cost %10.8f : %12.8f %12.8f %12.8f\n", f, d[0], d[1], d[2]);
  }

  dcost = 1.0 - f;
  dcost_dl = sqrt(d[0] * d[0] + d[1] * d[1] * d[2] * d[2]);
  dl = dcost / dcost_dl;

  ref_node_xyz(ref_node, 0, node) += dl * d[0];
  ref_node_xyz(ref_node, 1, node) += dl * d[1];
  ref_node_xyz(ref_node, 2, node) += dl * d[2];

  each_ref_cell_having_node(ref_cell, node, item, cell) {
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "nodes");
    RSS(ref_node_tri_dquality_dnode0(ref_node, nodes, &f, d), "qual deriv");
  }

  if (verbose)
    printf("rate %12.8f dcost %12.8f dl %12.8f\n", (1.0 - f) / dcost, dcost,
           dl);

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tri_quality_around(REF_GRID ref_grid, REF_INT node,
                                         REF_DBL *min_quality) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL none_found = REF_TRUE;
  REF_DBL quality;

  *min_quality = 1.0;
  each_ref_cell_having_node(ref_cell, node, item, cell) {
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "nodes");
    none_found = REF_FALSE;
    RSS(ref_node_tri_quality(ref_node, nodes, &quality), "qual");
    *min_quality = MIN(*min_quality, quality);
  }

  if (none_found) {
    *min_quality = -2.0;
    THROW("no triagle found, can not compute quality");
  }

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tri_uv_area_around(REF_GRID ref_grid, REF_INT node,
                                         REF_DBL *min_uv_area) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT id, item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL none_found = REF_TRUE;
  REF_DBL sign_uv_area, uv_area;

  *min_uv_area = -2.0;

  RSS(ref_geom_unique_id(ref_geom, node, REF_GEOM_FACE, &id), "id");
  RSS(ref_geom_uv_area_sign(ref_grid, id, &sign_uv_area), "sign");

  each_ref_cell_having_node(ref_cell, node, item, cell) {
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "nodes");
    RSS(ref_geom_uv_area(ref_geom, nodes, &uv_area), "uv area");
    uv_area *= sign_uv_area;
    if (none_found) {
      *min_uv_area = uv_area;
      none_found = REF_FALSE;
    } else {
      *min_uv_area = MIN(*min_uv_area, uv_area);
    }
  }

  if (none_found) THROW("no triagle found, can not compute min uv area");

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_outward_norm(REF_GRID ref_grid, REF_INT node,
                                   REF_BOOL *allowed) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT item, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL normal[3];

  *allowed = REF_FALSE;

  ref_cell = ref_grid_tri(ref_grid);
  each_ref_cell_having_node(ref_cell, node, item, cell) {
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "nodes");

    RSS(ref_node_tri_normal(ref_node, nodes, normal), "norm");

    if ((ref_node_xyz(ref_node, 1, nodes[0]) > 0.5 && normal[1] >= 0.0) ||
        (ref_node_xyz(ref_node, 1, nodes[0]) < 0.5 && normal[1] <= 0.0))
      return REF_SUCCESS;
  }

  *allowed = REF_TRUE;

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tri_ideal(REF_GRID ref_grid, REF_INT node, REF_INT tri,
                                REF_DBL *ideal_location) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT n0, n1;
  REF_INT ixyz;
  REF_DBL dn[3];
  REF_DBL dt[3];
  REF_DBL tangent_length, projection, scale, length_in_metric;

  RSS(ref_cell_nodes(ref_grid_tri(ref_grid), tri, nodes), "get tri");
  n0 = REF_EMPTY;
  n1 = REF_EMPTY;
  if (node == nodes[0]) {
    n0 = nodes[1];
    n1 = nodes[2];
  }
  if (node == nodes[1]) {
    n0 = nodes[2];
    n1 = nodes[0];
  }
  if (node == nodes[2]) {
    n0 = nodes[0];
    n1 = nodes[1];
  }
  if (n0 == REF_EMPTY || n1 == REF_EMPTY) THROW("empty triangle side");

  for (ixyz = 0; ixyz < 3; ixyz++)
    ideal_location[ixyz] = 0.5 * (ref_node_xyz(ref_node, ixyz, n0) +
                                  ref_node_xyz(ref_node, ixyz, n1));
  for (ixyz = 0; ixyz < 3; ixyz++)
    dn[ixyz] = ref_node_xyz(ref_node, ixyz, node) - ideal_location[ixyz];
  for (ixyz = 0; ixyz < 3; ixyz++)
    dt[ixyz] =
        ref_node_xyz(ref_node, ixyz, n1) - ref_node_xyz(ref_node, ixyz, n0);

  tangent_length = ref_math_dot(dt, dt);
  projection = ref_math_dot(dn, dt);

  if (ref_math_divisible(projection, tangent_length)) {
    for (ixyz = 0; ixyz < 3; ixyz++)
      dn[ixyz] -= (projection / tangent_length) * dt[ixyz];
  } else {
    printf("projection = %e tangent_length = %e\n", projection, tangent_length);
    return REF_DIV_ZERO;
  }

  RSS(ref_math_normalize(dn), "normalize direction");
  /* would an averaged metric be more appropriate? */
  length_in_metric =
      ref_matrix_sqrt_vt_m_v(ref_node_metric_ptr(ref_node, node), dn);

  scale = 0.5 * sqrt(3.0); /* altitude of equilateral triangle */
  if (ref_math_divisible(scale, length_in_metric)) {
    scale = scale / length_in_metric;
  } else {
    printf(" length_in_metric = %e, not invertable\n", length_in_metric);
    return REF_DIV_ZERO;
  }

  for (ixyz = 0; ixyz < 3; ixyz++) ideal_location[ixyz] += scale * dn[ixyz];

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tri_quality(REF_GRID ref_grid, REF_INT node, REF_INT id,
                                  REF_INT *nodes, REF_DBL *uv, REF_DBL *dq_duv,
                                  REF_DBL step, REF_DBL *qnew) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_DBL uvnew[2];
  uvnew[0] = uv[0] + step * dq_duv[0];
  uvnew[1] = uv[1] + step * dq_duv[1];

  RSS(ref_geom_add(ref_geom, node, REF_GEOM_FACE, id, uvnew), "set uv");
  RSS(ref_geom_constrain(ref_grid, node), "constrain");
  RSS(ref_node_tri_quality(ref_node, nodes, qnew), "qual");

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tri_ideal_uv(REF_GRID ref_grid, REF_INT node, REF_INT tri,
                                   REF_DBL *ideal_uv) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT n0, n1;
  REF_INT id, geom;
  REF_DBL uv_orig[2];
  REF_DBL uv[2];
  REF_DBL q0, q;
  REF_DBL xyz[3], dxyz_duv[15], dq_dxyz[3], dq_duv[2], dq_duv0[2], dq_duv1[2];
  REF_DBL slope, beta, num, denom;
  REF_DBL step1, step2, step3, q1, q2, q3;
  REF_INT tries, search;
  REF_BOOL verbose = REF_FALSE;

  RSS(ref_cell_nodes(ref_grid_tri(ref_grid), tri, nodes), "get tri");
  n0 = REF_EMPTY;
  n1 = REF_EMPTY;
  if (node == nodes[0]) {
    n0 = nodes[1];
    n1 = nodes[2];
  }
  if (node == nodes[1]) {
    n0 = nodes[2];
    n1 = nodes[0];
  }
  if (node == nodes[2]) {
    n0 = nodes[0];
    n1 = nodes[1];
  }
  if (n0 == REF_EMPTY || n1 == REF_EMPTY) {
    THROW("empty triangle side");
  }
  nodes[0] = node;
  nodes[1] = n0;
  nodes[2] = n1;

  RSS(ref_geom_unique_id(ref_geom, node, REF_GEOM_FACE, &id), "id");
  RSS(ref_geom_tuv(ref_geom, node, REF_GEOM_FACE, id, uv_orig), "uv");
  RSS(ref_geom_find(ref_geom, node, REF_GEOM_FACE, id, &geom), "geom");

  RSS(ref_node_tri_quality(ref_node, nodes, &q0), "qual");

  uv[0] = uv_orig[0];
  uv[1] = uv_orig[1];
  dq_duv0[0] = 0; /* uninit warning */
  dq_duv0[1] = 0;
  q = q0;
  for (tries = 0; tries < 30 && q < 0.99; tries++) {
    RSS(ref_geom_add(ref_geom, node, REF_GEOM_FACE, id, uv), "set uv");
    RSS(ref_geom_constrain(ref_grid, node), "constrain");
    RSS(ref_node_tri_dquality_dnode0(ref_node, nodes, &q, dq_dxyz), "qual");
    RSS(ref_geom_eval(ref_geom, geom, xyz, dxyz_duv), "eval face");
    dq_duv1[0] = dq_dxyz[0] * dxyz_duv[0] + dq_dxyz[1] * dxyz_duv[1] +
                 dq_dxyz[2] * dxyz_duv[2];
    dq_duv1[1] = dq_dxyz[0] * dxyz_duv[3] + dq_dxyz[1] * dxyz_duv[4] +
                 dq_dxyz[2] * dxyz_duv[5];

    if (0 == tries) {
      beta = 0;
      dq_duv[0] = dq_duv1[0];
      dq_duv[1] = dq_duv1[1];
    } else {
      /* fletcher-reeves */
      num = dq_duv1[0] * dq_duv1[0] + dq_duv1[1] * dq_duv1[1];
      denom = dq_duv0[0] * dq_duv0[0] + dq_duv0[1] * dq_duv0[1];
      /* polak-ribiere */
      num = dq_duv1[0] * (dq_duv1[0] - dq_duv0[0]) +
            dq_duv1[1] * (dq_duv1[1] - dq_duv0[1]);
      denom = dq_duv0[0] * dq_duv0[0] + dq_duv0[1] * dq_duv0[1];
      beta = 0;
      if (ref_math_divisible(num, denom)) beta = num / denom;
      beta = MAX(0.0, beta);
      dq_duv[0] = dq_duv1[0] + beta * dq_duv[0];
      dq_duv[1] = dq_duv1[1] + beta * dq_duv[1];
    }
    dq_duv0[0] = dq_duv1[0];
    dq_duv0[1] = dq_duv1[1];

    slope = sqrt(dq_duv[0] * dq_duv[0] + dq_duv[1] * dq_duv[1]);
    step3 = (1.0 - q) / slope;
    step1 = 0;
    step2 = 0.5 * (step1 + step3);
    RSS(ref_smooth_tri_quality(ref_grid, node, id, nodes, uv, dq_duv, step1,
                               &q1),
        "set uv for q1");
    RSS(ref_smooth_tri_quality(ref_grid, node, id, nodes, uv, dq_duv, step2,
                               &q2),
        "set uv for q2");
    RSS(ref_smooth_tri_quality(ref_grid, node, id, nodes, uv, dq_duv, step3,
                               &q3),
        "set uv for q3");
    for (search = 0; search < 15; search++) {
      if (q1 > q3) {
        step3 = step2;
        q3 = q2;
      } else {
        step1 = step2;
        q1 = q2;
      }
      step2 = 0.5 * (step1 + step3);
      RSS(ref_smooth_tri_quality(ref_grid, node, id, nodes, uv, dq_duv, step2,
                                 &q2),
          "set uv for q2");
    }
    RSS(ref_geom_tuv(ref_geom, node, REF_GEOM_FACE, id, uv), "uv");

    if (verbose && tries > 25) {
      printf(" slow conv %2d    q %f dq_duv1 %f %f\n", tries, q2, dq_duv1[0],
             dq_duv1[1]);
      printf("              step %f dq_duv  %f %f beta %f\n", step2, dq_duv[0],
             dq_duv[1], beta);
    }
  }

  if (verbose && q < 0.99) {
    printf(" bad ideal q %f dq_duv %f %f\n", q, dq_duv[0], dq_duv[1]);
  }

  RSS(ref_geom_add(ref_geom, node, REF_GEOM_FACE, id, uv_orig), "set uv");
  RSS(ref_geom_constrain(ref_grid, node), "constrain");

  ideal_uv[0] = uv[0];
  ideal_uv[1] = uv[1];

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tri_weighted_ideal(REF_GRID ref_grid, REF_INT node,
                                         REF_DBL *ideal_location) {
  REF_INT item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT ixyz;
  REF_DBL tri_ideal[3];
  REF_DBL quality, weight, normalization;

  normalization = 0.0;
  for (ixyz = 0; ixyz < 3; ixyz++) ideal_location[ixyz] = 0.0;

  each_ref_cell_having_node(ref_grid_tri(ref_grid), node, item, cell) {
    RSS(ref_smooth_tri_ideal(ref_grid, node, cell, tri_ideal), "tri ideal");
    RSS(ref_cell_nodes(ref_grid_tri(ref_grid), cell, nodes), "nodes");
    RSS(ref_node_tri_quality(ref_grid_node(ref_grid), nodes, &quality),
        "tri qual");
    quality = MAX(quality, ref_grid_adapt(ref_grid, smooth_min_quality));
    weight = 1.0 / quality;
    normalization += weight;
    for (ixyz = 0; ixyz < 3; ixyz++)
      ideal_location[ixyz] += weight * tri_ideal[ixyz];
  }

  if (ref_math_divisible(1.0, normalization)) {
    for (ixyz = 0; ixyz < 3; ixyz++)
      ideal_location[ixyz] = (1.0 / normalization) * ideal_location[ixyz];
  } else {
    printf("normalization = %e at %e %e %e\n", normalization,
           ref_node_xyz(ref_grid_node(ref_grid), 0, node),
           ref_node_xyz(ref_grid_node(ref_grid), 1, node),
           ref_node_xyz(ref_grid_node(ref_grid), 2, node));
    return REF_DIV_ZERO;
  }

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tri_weighted_ideal_uv(REF_GRID ref_grid, REF_INT node,
                                            REF_DBL *ideal_uv) {
  REF_INT item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT iuv;
  REF_DBL tri_uv[2];
  REF_DBL quality, weight, normalization;

  normalization = 0.0;
  for (iuv = 0; iuv < 2; iuv++) ideal_uv[iuv] = 0.0;

  each_ref_cell_having_node(ref_grid_tri(ref_grid), node, item, cell) {
    RSS(ref_smooth_tri_ideal_uv(ref_grid, node, cell, tri_uv), "tri ideal");
    RSS(ref_cell_nodes(ref_grid_tri(ref_grid), cell, nodes), "nodes");
    RSS(ref_node_tri_quality(ref_grid_node(ref_grid), nodes, &quality),
        "tri qual");
    quality = MAX(quality, ref_grid_adapt(ref_grid, smooth_min_quality));
    weight = 1.0 / quality;
    normalization += weight;
    for (iuv = 0; iuv < 2; iuv++) ideal_uv[iuv] += weight * tri_uv[iuv];
  }

  if (ref_math_divisible(1.0, normalization)) {
    for (iuv = 0; iuv < 2; iuv++)
      ideal_uv[iuv] = (1.0 / normalization) * ideal_uv[iuv];
  } else {
    printf("normalization = %e\n", normalization);
    return REF_DIV_ZERO;
  }

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_twod_tri_improve(REF_GRID ref_grid, REF_INT node) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT tries;
  REF_DBL ideal[3], original[3];
  REF_DBL backoff, quality0, quality;
  REF_INT ixyz, opposite;
  REF_BOOL allowed;

  /* can't handle boundaries yet */
  if (!ref_cell_node_empty(ref_grid_qua(ref_grid), node)) return REF_SUCCESS;

  for (ixyz = 0; ixyz < 3; ixyz++)
    original[ixyz] = ref_node_xyz(ref_node, ixyz, node);

  RSS(ref_smooth_tri_weighted_ideal(ref_grid, node, ideal), "ideal");

  RSS(ref_smooth_tri_quality_around(ref_grid, node, &quality0), "q");

  backoff = 1.0;
  for (tries = 0; tries < 8; tries++) {
    for (ixyz = 0; ixyz < 3; ixyz++)
      ref_node_xyz(ref_node, ixyz, node) =
          backoff * ideal[ixyz] + (1.0 - backoff) * original[ixyz];
    RSS(ref_smooth_outward_norm(ref_grid, node, &allowed), "normals");
    if (allowed) {
      RSS(ref_smooth_tri_quality_around(ref_grid, node, &quality), "q");
      if (quality > quality0) {
        /* update opposite side: X and Z only */
        RSS(ref_twod_opposite_node(ref_grid_pri(ref_grid), node, &opposite),
            "opp");
        ref_node_xyz(ref_node, 0, opposite) = ref_node_xyz(ref_node, 0, node);
        ref_node_xyz(ref_node, 2, opposite) = ref_node_xyz(ref_node, 2, node);
        return REF_SUCCESS;
      }
    }
    backoff *= 0.5;
  }

  for (ixyz = 0; ixyz < 3; ixyz++)
    ref_node_xyz(ref_node, ixyz, node) = original[ixyz];

  return REF_SUCCESS;
}

static REF_STATUS ref_smooth_node_same_normal(REF_GRID ref_grid, REF_INT node,
                                              REF_BOOL *allowed) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL normal[3], first_normal[3];
  REF_DBL dot;
  REF_STATUS status;
  REF_BOOL first_tri;

  *allowed = REF_TRUE;

  first_tri = REF_TRUE;
  each_ref_cell_having_node(ref_cell, node, item, cell) {
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "nodes");
    RSS(ref_node_tri_normal(ref_node, nodes, normal), "orig normal");
    if (first_tri) {
      first_tri = REF_FALSE;
      first_normal[0] = normal[0];
      first_normal[1] = normal[1];
      first_normal[2] = normal[2];
      RSS(ref_math_normalize(first_normal), "original triangle has zero area");
    }
    status = ref_math_normalize(normal);
    if (REF_DIV_ZERO == status) { /* new triangle face has zero area */
      *allowed = REF_FALSE;
      return REF_SUCCESS;
    }
    RSS(status, "new normal length");
    dot = ref_math_dot(first_normal, normal);
    /* acos(1.0-1.0e-8) ~ 0.0001 radian, 0.01 deg */
    if (dot < (1.0 - 1.0e-8)) {
      *allowed = REF_FALSE;
      return REF_SUCCESS;
    }
  }

  return REF_SUCCESS;
}

static REF_STATUS ref_smooth_no_geom_tri_improve(REF_GRID ref_grid,
                                                 REF_INT node) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT tries;
  REF_DBL ideal[3], original[3];
  REF_DBL backoff, tri_quality0, tri_quality, tet_quality;
  REF_INT ixyz;
  REF_INT n_ids, ids[2];
  REF_BOOL allowed, geom_face;

  /* can't handle mixed elements */
  if (!ref_cell_node_empty(ref_grid_qua(ref_grid), node)) return REF_SUCCESS;

  /* don't move edge nodes */
  RXS(ref_cell_id_list_around(ref_grid_tri(ref_grid), node, 2, &n_ids, ids),
      REF_INCREASE_LIMIT, "count faceids");
  if (n_ids > 1) return REF_SUCCESS;

  /* not for nodes with geom support */
  RSS(ref_geom_is_a(ref_grid_geom(ref_grid), node, REF_GEOM_FACE, &geom_face),
      "face check");
  if (geom_face) return REF_SUCCESS;

  RSS(ref_smooth_node_same_normal(ref_grid, node, &allowed), "normal dev");
  if (!allowed) return REF_SUCCESS;

  for (ixyz = 0; ixyz < 3; ixyz++)
    original[ixyz] = ref_node_xyz(ref_node, ixyz, node);

  RSS(ref_smooth_tri_weighted_ideal(ref_grid, node, ideal), "ideal");

  RSS(ref_smooth_tri_quality_around(ref_grid, node, &tri_quality0), "q");

  backoff = 1.0;
  for (tries = 0; tries < 8; tries++) {
    for (ixyz = 0; ixyz < 3; ixyz++)
      ref_node_xyz(ref_node, ixyz, node) =
          backoff * ideal[ixyz] + (1.0 - backoff) * original[ixyz];
    RSS(ref_smooth_tet_quality_around(ref_grid, node, &tet_quality), "q");
    if (tet_quality > ref_grid_adapt(ref_grid, smooth_min_quality)) {
      RSS(ref_smooth_tri_quality_around(ref_grid, node, &tri_quality), "q");
      if (tri_quality > tri_quality0) {
        return REF_SUCCESS;
      }
    }
    backoff *= 0.5;
  }

  for (ixyz = 0; ixyz < 3; ixyz++)
    ref_node_xyz(ref_node, ixyz, node) = original[ixyz];

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_local_pris_about(REF_GRID ref_grid, REF_INT about_node,
                                       REF_BOOL *allowed) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT item, cell, node;

  *allowed = REF_FALSE;

  ref_cell = ref_grid_pri(ref_grid);

  each_ref_cell_having_node(
      ref_cell, about_node, item,
      cell) for (node = 0; node < ref_cell_node_per(ref_cell);
                 node++) if (ref_mpi_rank(ref_grid_mpi(ref_grid)) !=
                             ref_node_part(
                                 ref_node,
                                 ref_cell_c2n(ref_cell, node,
                                              cell))) return REF_SUCCESS;

  *allowed = REF_TRUE;

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_twod_pass(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node;
  REF_BOOL allowed;

  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_node_node_twod(ref_node, node, &allowed), "twod");
    if (!allowed) continue;

    /* can't handle boundaries yet */
    allowed = ref_cell_node_empty(ref_grid_qua(ref_grid), node);
    if (!allowed) continue;

    RSS(ref_smooth_local_pris_about(ref_grid, node, &allowed), "para");
    if (!allowed) {
      ref_node_age(ref_node, node)++;
      continue;
    }

    ref_node_age(ref_node, node) = 0;
    RSS(ref_smooth_twod_tri_improve(ref_grid, node), "improve");
  }

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tet_quality_around(REF_GRID ref_grid, REF_INT node,
                                         REF_DBL *min_quality) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_INT item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL none_found = REF_TRUE;
  REF_DBL quality;

  *min_quality = 1.0;
  each_ref_cell_having_node(ref_cell, node, item, cell) {
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "nodes");
    none_found = REF_FALSE;
    RSS(ref_node_tet_quality(ref_node, nodes, &quality), "qual");
    *min_quality = MIN(*min_quality, quality);
  }

  if (none_found) {
    *min_quality = -2.0;
    return REF_NOT_FOUND;
  }

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tet_ideal(REF_GRID ref_grid, REF_INT node, REF_INT tet,
                                REF_DBL *ideal_location) {
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT tri_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT tet_node, tri_node;
  REF_INT ixyz;
  REF_DBL dn[3];
  REF_DBL scale, length_in_metric;

  RSS(ref_cell_nodes(ref_cell, tet, nodes), "get tri");
  tri_nodes[0] = REF_EMPTY;
  tri_nodes[1] = REF_EMPTY;
  tri_nodes[2] = REF_EMPTY;
  for (tet_node = 0; tet_node < 4; tet_node++)
    if (node == nodes[tet_node]) {
      for (tri_node = 0; tri_node < 3; tri_node++)
        tri_nodes[tri_node] =
            nodes[ref_cell_f2n_gen(ref_cell, tri_node, tet_node)];
    }
  if (tri_nodes[0] == REF_EMPTY || tri_nodes[1] == REF_EMPTY ||
      tri_nodes[2] == REF_EMPTY)
    THROW("empty tetrahedra face");

  for (ixyz = 0; ixyz < 3; ixyz++)
    ideal_location[ixyz] = (ref_node_xyz(ref_node, ixyz, tri_nodes[0]) +
                            ref_node_xyz(ref_node, ixyz, tri_nodes[1]) +
                            ref_node_xyz(ref_node, ixyz, tri_nodes[2])) /
                           3.0;

  RSS(ref_node_tri_normal(ref_node, tri_nodes, dn), "tri normal");

  RSS(ref_math_normalize(dn), "normalize direction");
  /* would an averaged metric be more appropriate? */
  length_in_metric =
      ref_matrix_sqrt_vt_m_v(ref_node_metric_ptr(ref_node, node), dn);

  scale = sqrt(6.0) / 3.0; /* altitude of regular tetrahedra */
  if (ref_math_divisible(scale, length_in_metric)) {
    scale = scale / length_in_metric;
  } else {
    printf(" length_in_metric = %e, not invertable\n", length_in_metric);
    return REF_DIV_ZERO;
  }

  for (ixyz = 0; ixyz < 3; ixyz++) ideal_location[ixyz] += scale * dn[ixyz];

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tet_weighted_ideal(REF_GRID ref_grid, REF_INT node,
                                         REF_DBL *ideal_location) {
  REF_INT item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT ixyz;
  REF_DBL tet_ideal[3];
  REF_DBL quality, weight, normalization;

  normalization = 0.0;
  for (ixyz = 0; ixyz < 3; ixyz++) ideal_location[ixyz] = 0.0;

  each_ref_cell_having_node(ref_grid_tet(ref_grid), node, item, cell) {
    RSS(ref_smooth_tet_ideal(ref_grid, node, cell, tet_ideal), "tet ideal");
    RSS(ref_cell_nodes(ref_grid_tet(ref_grid), cell, nodes), "nodes");
    RSS(ref_node_tet_quality(ref_grid_node(ref_grid), nodes, &quality),
        "tet qual");
    quality = MAX(quality, ref_grid_adapt(ref_grid, smooth_min_quality));
    weight = 1.0 / quality;
    normalization += weight;
    for (ixyz = 0; ixyz < 3; ixyz++)
      ideal_location[ixyz] += weight * tet_ideal[ixyz];
  }

  if (ref_math_divisible(1.0, normalization)) {
    for (ixyz = 0; ixyz < 3; ixyz++)
      ideal_location[ixyz] = (1.0 / normalization) * ideal_location[ixyz];
  } else {
    printf("normalization = %e\n", normalization);
    return REF_DIV_ZERO;
  }

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tet_improve(REF_GRID ref_grid, REF_INT node) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT tries;
  REF_DBL ideal[3], original[3];
  REF_DBL backoff, quality0, quality;
  REF_INT ixyz;

  /* can't handle boundaries yet */
  if (!ref_cell_node_empty(ref_grid_tri(ref_grid), node) ||
      !ref_cell_node_empty(ref_grid_qua(ref_grid), node))
    return REF_SUCCESS;

  for (ixyz = 0; ixyz < 3; ixyz++)
    original[ixyz] = ref_node_xyz(ref_node, ixyz, node);

  RSS(ref_smooth_tet_weighted_ideal(ref_grid, node, ideal), "ideal");

  RSS(ref_smooth_tet_quality_around(ref_grid, node, &quality0), "q");

  backoff = 1.0;
  for (tries = 0; tries < 8; tries++) {
    for (ixyz = 0; ixyz < 3; ixyz++)
      ref_node_xyz(ref_node, ixyz, node) =
          backoff * ideal[ixyz] + (1.0 - backoff) * original[ixyz];
    RSS(ref_smooth_tet_quality_around(ref_grid, node, &quality), "q");
    if (quality > quality0) {
      return REF_SUCCESS;
    }
    backoff *= 0.5;
  }

  for (ixyz = 0; ixyz < 3; ixyz++)
    ref_node_xyz(ref_node, ixyz, node) = original[ixyz];

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_local_tet_about(REF_GRID ref_grid, REF_INT about_node,
                                      REF_BOOL *allowed) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT item, cell, node;

  *allowed = REF_FALSE;

  ref_cell = ref_grid_tet(ref_grid);

  each_ref_cell_having_node(
      ref_cell, about_node, item,
      cell) for (node = 0; node < ref_cell_node_per(ref_cell);
                 node++) if (ref_mpi_rank(ref_grid_mpi(ref_grid)) !=
                             ref_node_part(
                                 ref_node,
                                 ref_cell_c2n(ref_cell, node,
                                              cell))) return REF_SUCCESS;

  *allowed = REF_TRUE;

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_geom_edge(REF_GRID ref_grid, REF_INT node) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_CELL edg = ref_grid_edg(ref_grid);
  REF_BOOL geom_node, geom_edge;
  REF_INT id;
  REF_INT nodes[2], nnode;
  REF_DBL t_orig, t0, t1;
  REF_DBL r0, r1;
  REF_DBL q_orig;
  REF_DBL s_orig, rsum;

  REF_DBL t, st, sr, q, backoff, t_target;
  REF_INT tries;
  REF_BOOL verbose = REF_FALSE;
  REF_INT edge_nodes[REF_CELL_MAX_SIZE_PER], cell, sense;

  RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_NODE, &geom_node), "node check");
  RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_EDGE, &geom_edge), "edge check");
  RAS(!geom_node, "geom node not allowed");
  RAS(geom_edge, "geom edge required");

  RSS(ref_geom_unique_id(ref_geom, node, REF_GEOM_EDGE, &id), "get id");
  RSS(ref_cell_node_list_around(edg, node, 2, &nnode, nodes), "edge neighbors");
  REIS(2, nnode, "expected two nodes");

  RSS(ref_node_ratio(ref_node, nodes[0], node, &r0), "get r0");
  RSS(ref_node_ratio(ref_node, nodes[1], node, &r1), "get r1");

  rsum = r1 + r0;
  if (ref_math_divisible(r0, rsum)) {
    s_orig = r0 / rsum;
    /* one percent imblance is good enough */
    if (ABS(s_orig - 0.5) < 0.01) return REF_SUCCESS;
  } else {
    printf("div zero %e r0 %e r1\n", r1, r0);
    return REF_DIV_ZERO;
  }

  edge_nodes[0] = nodes[0];
  edge_nodes[1] = node;
  RSS(ref_cell_with(edg, edge_nodes, &cell), "find nodes[0] edg cell");
  RSS(ref_geom_cell_tuv(ref_grid, nodes[0], cell, REF_GEOM_EDGE, &t0, &sense),
      "get t0");
  RSS(ref_geom_cell_tuv(ref_grid, node, cell, REF_GEOM_EDGE, &t_orig,
                        &sense), "get t_orig");
  edge_nodes[0] = nodes[1];
  edge_nodes[1] = node;
  RSS(ref_cell_with(edg, edge_nodes, &cell), "find nodes[0] edg cell");
  RSS(ref_geom_cell_tuv(ref_grid, nodes[1], cell, REF_GEOM_EDGE, &t1, &sense),
      "get t1");

  RSS(ref_smooth_tet_quality_around(ref_grid, node, &q_orig), "q_orig");

  if (verbose)
    printf("edge %d t %f %f %f r %f %f q %f\n", id, t0, t_orig, t1, r0, r1,
           q_orig);

  sr = r0 / (r1 + r0);
  st = (t_orig - t0) / (t1 - t0);
  st = st + (0.5 - sr);
  t_target = st * t1 + (1.0 - st) * t0;

  if (verbose)
    printf("t_target %f sr %f st %f %f \n", t_target, sr,
           (t_orig - t0) / (t1 - t0), st);

  backoff = 1.0;
  for (tries = 0; tries < 8; tries++) {
    t = backoff * t_target + (1.0 - backoff) * t_orig;

    RSS(ref_geom_add(ref_geom, node, REF_GEOM_EDGE, id, &t), "set t");
    RSS(ref_geom_constrain(ref_grid, node), "constrain");
    RSS(ref_node_ratio(ref_node, nodes[0], node, &r0), "get r0");
    RSS(ref_node_ratio(ref_node, nodes[1], node, &r1), "get r1");
    RSS(ref_smooth_tet_quality_around(ref_grid, node, &q), "q");

    if (verbose) printf("t %f r %f %f q %f \n", t, r0, r1, q);
    if (q > ref_grid_adapt(ref_grid, smooth_min_quality)) {
      return REF_SUCCESS;
    }
    backoff *= 0.5;
  }

  RSS(ref_geom_add(ref_geom, node, REF_GEOM_EDGE, id, &t_orig), "set t");
  RSS(ref_geom_constrain(ref_grid, node), "constrain");
  RSS(ref_smooth_tet_quality_around(ref_grid, node, &q), "q");

  if (verbose) printf("undo q %f\n", q);

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_geom_face(REF_GRID ref_grid, REF_INT node) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_BOOL geom_node, geom_edge, geom_face, no_quads;
  REF_INT id;
  REF_DBL uv_orig[2], uv_ideal[2];
  REF_DBL qtet_orig, qtri_orig;
  REF_DBL qtri, qtet, min_uv_area;
  REF_DBL backoff, uv[2];
  REF_INT tries, iuv;
  REF_DBL uv_min[2], uv_max[2];
  REF_BOOL verbose = REF_FALSE;
  RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_NODE, &geom_node), "node check");
  RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_EDGE, &geom_edge), "edge check");
  RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_FACE, &geom_face), "face check");
  no_quads = ref_cell_node_empty(ref_grid_qua(ref_grid), node);
  RAS(!geom_node, "geom node not allowed");
  RAS(!geom_edge, "geom edge not allowed");
  RAS(geom_face, "geom face required");
  RAS(no_quads, "quads not allowed");

  RSB(ref_geom_unique_id(ref_geom, node, REF_GEOM_FACE, &id), "get-id",
      ref_clump_around(ref_grid, node, "get-id.tec"));
  RSS(ref_geom_tuv(ref_geom, node, REF_GEOM_FACE, id, uv_orig), "get uv_orig");
  RSS(ref_smooth_tet_quality_around(ref_grid, node, &qtet_orig), "q tet");
  RSS(ref_smooth_tri_quality_around(ref_grid, node, &qtri_orig), "q tri");

  if (verbose)
    printf("uv %f %f tri %f tet %f\n", uv_orig[0], uv_orig[1], qtri_orig,
           qtet_orig);

  RSS(ref_smooth_tri_weighted_ideal_uv(ref_grid, node, uv_ideal), "ideal");

  RSS(ref_geom_tri_uv_bounding_box(ref_grid, node, uv_min, uv_max), "bb");

  backoff = 1.0;
  for (tries = 0; tries < 8; tries++) {
    for (iuv = 0; iuv < 2; iuv++)
      uv[iuv] = backoff * uv_ideal[iuv] + (1.0 - backoff) * uv_orig[iuv];

    RSS(ref_geom_add(ref_geom, node, REF_GEOM_FACE, id, uv), "set uv");
    RSS(ref_geom_constrain(ref_grid, node), "constrain");
    RSS(ref_smooth_tet_quality_around(ref_grid, node, &qtet), "q tet");
    RSS(ref_smooth_tri_quality_around(ref_grid, node, &qtri), "q tri");
    RSS(ref_smooth_tri_uv_area_around(ref_grid, node, &min_uv_area), "a");
    if (qtri >= qtri_orig &&
        qtet > ref_grid_adapt(ref_grid, smooth_min_quality) &&
        min_uv_area > 1.0e-12 && uv_min[0] < uv[0] && uv[0] < uv_max[0] &&
        uv_min[1] < uv[1] && uv[1] < uv_max[1]) {
      if (verbose) printf("better qtri %f qtet %f\n", qtri, qtet);
      return REF_SUCCESS;
    }
    backoff *= 0.5;
  }

  RSS(ref_geom_add(ref_geom, node, REF_GEOM_FACE, id, uv_orig), "set t");
  RSS(ref_geom_constrain(ref_grid, node), "constrain");

  if (verbose)
    printf("undo qtri %f qtet %f was %f %f\n", qtri, qtet, qtri_orig,
           qtet_orig);

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_threed_pass(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_INT geom, node;
  REF_BOOL allowed, geom_node, geom_edge, interior;

  /* smooth edges first if we have geom */
  each_ref_geom_edge(ref_geom, geom) {
    node = ref_geom_node(ref_geom, geom);
    /* don't move geom nodes */
    RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_NODE, &geom_node), "node check");
    if (geom_node) continue;
    /* next to ghost node, can't move */
    RSS(ref_smooth_local_tet_about(ref_grid, node, &allowed), "para");
    if (!allowed) {
      ref_node_age(ref_node, node)++;
      continue;
    }
    RSS(ref_smooth_geom_edge(ref_grid, node), "ideal node for edge");
    ref_node_age(ref_node, node) = 0;
  }

  if (ref_grid_adapt(ref_grid, instrument))
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "mov edge");

  /* smooth faces if we have geom skip edges*/
  each_ref_geom_face(ref_geom, geom) {
    node = ref_geom_node(ref_geom, geom);
    /* don't move geom nodes */
    RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_EDGE, &geom_edge), "node check");
    if (geom_edge) continue;
    /* next to ghost node, can't move */
    RSS(ref_smooth_local_tet_about(ref_grid, node, &allowed), "para");
    if (!allowed) {
      ref_node_age(ref_node, node)++;
      continue;
    }
    RSS(ref_smooth_geom_face(ref_grid, node), "ideal node for face");
    ref_node_age(ref_node, node) = 0;
  }

  if (ref_grid_adapt(ref_grid, instrument))
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "mov face");

  /* smooth faces without geom*/
  each_ref_node_valid_node(
      ref_node, node) if (!ref_cell_node_empty(ref_grid_tri(ref_grid), node)) {
    RSS(ref_smooth_local_tet_about(ref_grid, node, &allowed), "para");
    if (!allowed) {
      ref_node_age(ref_node, node)++;
      continue;
    }
    RSS(ref_smooth_no_geom_tri_improve(ref_grid, node), "no geom smooth");
  }

  /* smooth interior */
  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_smooth_local_tet_about(ref_grid, node, &allowed), "para");
    if (!allowed) {
      ref_node_age(ref_node, node)++;
      continue;
    }

    interior = ref_cell_node_empty(ref_grid_tri(ref_grid), node) &&
               ref_cell_node_empty(ref_grid_qua(ref_grid), node);
    if (interior) {
      RSS(ref_smooth_tet_improve(ref_grid, node), "ideal tet node");
      ref_node_age(ref_node, node) = 0;
    }
  }

  if (ref_grid_adapt(ref_grid, instrument))
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "mov int");

  /* smooth bad ones */
  {
    REF_DBL quality, min_quality = 0.10;
    REF_INT cell, cell_node, nodes[REF_CELL_MAX_SIZE_PER];
    each_ref_cell_valid_cell_with_nodes(ref_grid_tet(ref_grid), cell, nodes) {
      RSS(ref_node_tet_quality(ref_node, nodes, &quality), "qual");
      if (quality < min_quality) {
        each_ref_cell_cell_node(ref_grid_tet(ref_grid), cell_node) {
          node = nodes[cell_node];
          RSS(ref_smooth_local_tet_about(ref_grid, node, &allowed), "para");
          if (!allowed) {
            ref_node_age(ref_node, node)++;
            continue;
          }

          interior = ref_cell_node_empty(ref_grid_tri(ref_grid), node) &&
                     ref_cell_node_empty(ref_grid_qua(ref_grid), node);
          if (interior) {
            RSS(ref_smooth_tet_improve(ref_grid, node), "ideal");
            ref_node_age(ref_node, node) = 0;
          }
        }
      }
    }
  }
  return REF_SUCCESS;
}

REF_STATUS ref_smooth_threed_post_split(REF_GRID ref_grid, REF_INT node) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_BOOL allowed, interior;

  RSS(ref_smooth_local_tet_about(ref_grid, node, &allowed), "para");
  if (!allowed) {
    ref_node_age(ref_node, node)++;
    return REF_SUCCESS;
  }

  interior = ref_cell_node_empty(ref_grid_tri(ref_grid), node) &&
             ref_cell_node_empty(ref_grid_qua(ref_grid), node);
  if (interior) {
    RSS(ref_smooth_tet_improve(ref_grid, node), "ideal tet node");
    ref_node_age(ref_node, node) = 0;
  }

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_tet_report_quality_around(REF_GRID ref_grid,
                                                REF_INT node) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_INT item, cell, i;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL quality;

  i = 0;
  each_ref_cell_having_node(ref_cell, node, item, cell) {
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "nodes");
    RSS(ref_node_tet_quality(ref_node, nodes, &quality), "qual");
    printf(" %d:%5.3f", i, quality);
    i++;
  }
  printf("\n");

  return REF_SUCCESS;
}

REF_STATUS ref_smooth_nso_step(REF_GRID ref_grid, REF_INT node,
                               REF_BOOL *complete) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_INT item, cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL quality, d_quality[3];
  REF_INT *cells, *active;
  REF_DBL *quals, *grads;
  REF_INT worst, degree;
  REF_DBL min_qual;
  REF_INT i, j, ixyz;
  REF_DBL dir[3];
  REF_DBL m1, m0, alpha, min_alpha;
  REF_INT mate, reductions, max_reductions;
  REF_DBL requirement;
  REF_DBL xyz[3];
  REF_DBL active_tol = 1.0e-12;
  REF_INT nactive;
  REF_DBL last_alpha, last_qual;
  REF_BOOL verbose = REF_FALSE;

  *complete = REF_FALSE;

  xyz[0] = ref_node_xyz(ref_node, 0, node);
  xyz[1] = ref_node_xyz(ref_node, 1, node);
  xyz[2] = ref_node_xyz(ref_node, 2, node);

  RSS(ref_adj_degree(ref_cell_adj(ref_cell), node, &degree), "deg");

  ref_malloc(cells, degree, REF_INT);
  ref_malloc(active, degree, REF_INT);
  ref_malloc(quals, degree, REF_DBL);
  ref_malloc(grads, 3 * degree, REF_DBL);

  min_qual = 1.0;
  worst = REF_EMPTY;
  degree = 0;
  each_ref_cell_having_node(ref_cell, node, item, cell) {
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "nodes");
    RSS(ref_cell_orient_node0(4, node, nodes), "orient");
    RSS(ref_node_tet_dquality_dnode0(ref_node, nodes, &quality, d_quality),
        "qual");
    if (quality < min_qual || REF_EMPTY == worst) {
      worst = degree;
      min_qual = quality;
    }
    cells[degree] = cell;
    quals[degree] = quality;
    grads[0 + 3 * degree] = d_quality[0];
    grads[1 + 3 * degree] = d_quality[1];
    grads[2 + 3 * degree] = d_quality[2];
    degree++;
  }
  active[0] = worst;
  nactive = 1;
  for (i = 0; i < degree; i++) {
    if (i == worst) continue;
    if ((quals[i] - quals[active[0]]) < active_tol) {
      active[nactive] = i;
      nactive++;
    }
  }
  if (verbose)
    for (i = 0; i < nactive; i++)
      printf("%2d: %10.5f %10.5f %10.5f\n", active[i], grads[0 + 3 * active[i]],
             grads[1 + 3 * active[i]], grads[2 + 3 * active[i]]);

  if (4 <= nactive) {
    *complete = REF_TRUE;
    goto success_clean_and_return;
  }

  if (1 == nactive) {
    dir[0] = grads[0 + 3 * worst];
    dir[1] = grads[1 + 3 * worst];
    dir[2] = grads[2 + 3 * worst];
  } else { /* Charalambous and Conn DOI:10.1137/0715011 equ (3.2)  */
    REF_INT k;
    REF_DBL N[16];
    REF_DBL NNt[16];
    REF_DBL invNNt[16];
    REF_DBL NtinvNNt[16];
    REF_DBL NtinvNNtN[16];
    REF_DBL P[16];

    for (i = 0; i < nactive; i++) {
      N[i + nactive * 0] = 1.0;
      for (ixyz = 0; ixyz < 3; ixyz++)
        N[i + nactive * (1 + ixyz)] = -grads[ixyz + 3 * active[i]];
    }
    /* P = I - Nt [N Nt]^-1 N */
    for (i = 0; i < nactive; i++)
      for (j = 0; j < nactive; j++) NNt[i + j * nactive] = 0.0;
    for (i = 0; i < nactive; i++)
      for (j = 0; j < nactive; j++)
        for (k = 0; k < 4; k++)
          NNt[i + j * nactive] += N[i + nactive * k] * N[j + nactive * k];
    RSS(ref_matrix_inv_gen(nactive, NNt, invNNt), "inv");

    for (i = 0; i < 4; i++)
      for (j = 0; j < nactive; j++) NtinvNNt[i + 4 * j] = 0.0;

    for (i = 0; i < 4; i++)
      for (j = 0; j < nactive; j++)
        for (k = 0; k < nactive; k++)
          NtinvNNt[i + 4 * j] += N[k + i * nactive] * invNNt[k + j * nactive];

    for (i = 0; i < 4; i++)
      for (j = 0; j < 4; j++) NtinvNNtN[i + 4 * j] = 0.0;

    for (i = 0; i < 4; i++)
      for (j = 0; j < 4; j++)
        for (k = 0; k < nactive; k++)
          NtinvNNtN[i + 4 * j] += NtinvNNt[i + k * 4] * N[k + j * nactive];

    for (i = 0; i < 4; i++)
      for (j = 0; j < 4; j++) P[i + j * 4] = -NtinvNNtN[i + j * 4];
    for (i = 0; i < 4; i++) P[i + i * 4] += 1.0;

    for (ixyz = 0; ixyz < 3; ixyz++) dir[ixyz] = P[1 + ixyz];
  }

  RSS(ref_math_normalize(dir), "norm");

  m0 = ref_math_dot(dir, &(grads[3 * worst]));
  if (verbose)
    for (i = 0; i < nactive; i++)
      printf("slope %e\n", ref_math_dot(dir, &(grads[3 * active[i]])));
  if (0.0 >= m0) {
    printf("%s: %d: %s: m0 not positive %e", __FILE__, __LINE__, __func__, m0);
    *complete = REF_TRUE;
    goto success_clean_and_return;
  }
  mate = REF_EMPTY;
  min_alpha = 1.0e10;
  for (i = 0; i < degree; i++) {
    REF_BOOL skip;
    skip = REF_FALSE;
    for (j = 0; j < nactive; j++)
      if (i == active[j]) skip = REF_TRUE;
    if (skip) continue;
    m1 = ref_math_dot(dir, &(grads[3 * i]));
    /*
      cost = quals[i]+alpha*m1;
      cost = quals[worst]+alpha*m0;
      quals[i]+alpha*m1 = quals[worst]+alpha*m0;
    */
    if (!ref_math_divisible((quals[worst] - quals[i]), (m1 - m0))) continue;
    alpha = (quals[worst] - quals[i]) / (m1 - m0);
    if ((alpha > 0.0 && alpha < min_alpha)) {
      min_alpha = alpha;
      mate = i;
    }
  }
  if (REF_EMPTY == mate) {
    if (verbose) {
      for (i = 0; i < nactive; i++)
        printf("active slope %f %d\n",
               ref_math_dot(dir, &(grads[3 * active[i]])), i);
      for (i = 0; i < degree; i++)
        printf("all %f slope %f %d %e\n", quals[i],
               ref_math_dot(dir, &(grads[3 * i])), i,
               (1.0 - quals[i]) / ref_math_dot(dir, &(grads[3 * i])));
    }
    min_alpha = (1.0 - quals[0]) / ref_math_dot(dir, &(grads[3 * 0]));
    for (i = 1; i < degree; i++)
      min_alpha =
          MIN(min_alpha, (1.0 - quals[i]) / ref_math_dot(dir, &(grads[3 * i])));
  }

  alpha = min_alpha;
  last_alpha = 0.0;
  last_qual = 0.0;
  max_reductions = 8;
  for (reductions = 0; reductions < max_reductions; reductions++) {
    ref_node_xyz(ref_node, 0, node) = xyz[0] + alpha * dir[0];
    ref_node_xyz(ref_node, 1, node) = xyz[1] + alpha * dir[1];
    ref_node_xyz(ref_node, 2, node) = xyz[2] + alpha * dir[2];

    RSS(ref_smooth_tet_quality_around(ref_grid, node, &quality), "rep");
    requirement = 0.9 * alpha * m0 + quals[worst];
    if (verbose)
      printf(" %d alpha %e min %f required %f actual %f\n", nactive, alpha,
             min_qual, requirement, quality);
    if (reductions > 0 && quality < last_qual && quality > min_qual) {
      if (verbose)
        printf("use last alpha %e min %f last_qual %f actual %f\n", last_alpha,
               min_qual, last_qual, quality);
      alpha = last_alpha;
      quality = last_qual;
      ref_node_xyz(ref_node, 0, node) = xyz[0] + alpha * dir[0];
      ref_node_xyz(ref_node, 1, node) = xyz[1] + alpha * dir[1];
      ref_node_xyz(ref_node, 2, node) = xyz[2] + alpha * dir[2];
      break;
    }
    if (quality > requirement || alpha < 1.0e-12) break;
    last_alpha = alpha;
    last_qual = quality;
    alpha *= 0.5;
  }

  if (max_reductions <=
      reductions) { /* used all the reductions, step is small, marginal gains
                       remain */
    ref_node_xyz(ref_node, 0, node) = xyz[0];
    ref_node_xyz(ref_node, 1, node) = xyz[1];
    ref_node_xyz(ref_node, 2, node) = xyz[2];
    *complete = REF_TRUE;
  }

  if (3 == nactive &&
      (quality - min_qual) < 1.0e-5) { /* very small step toward forth active,
                                          marginal gains remain */
    *complete = REF_TRUE;
  }

success_clean_and_return:

  ref_free(grads);
  ref_free(quals);
  ref_free(active);
  ref_free(cells);
  return REF_SUCCESS;
}

REF_STATUS ref_smooth_nso(REF_GRID ref_grid, REF_INT node) {
  REF_BOOL allowed, interior;
  REF_BOOL complete = REF_FALSE;
  REF_INT step;

  RSS(ref_smooth_local_tet_about(ref_grid, node, &allowed), "para");
  if (!allowed) return REF_SUCCESS;

  interior = ref_cell_node_empty(ref_grid_tri(ref_grid), node) &&
             ref_cell_node_empty(ref_grid_qua(ref_grid), node);
  if (!interior) return REF_SUCCESS;

  for (step = 0; step < 100; step++) {
    RSS(ref_smooth_nso_step(ref_grid, node, &complete), "step");
    if (complete) break;
  }

  return REF_SUCCESS;
}
