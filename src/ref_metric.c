
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

#include "ref_metric.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ref_cell.h"
#include "ref_dict.h"
#include "ref_edge.h"
#include "ref_egads.h"
#include "ref_grid.h"
#include "ref_interp.h"
#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_matrix.h"
#include "ref_meshlink.h"
#include "ref_node.h"
#include "ref_phys.h"
#include "ref_sort.h"

#define REF_METRIC_MAX_DEGREE (1000)

REF_FCN REF_STATUS ref_metric_show(REF_DBL *m) {
  printf(" %18.10e %18.10e %18.10e\n", m[0], m[1], m[2]);
  printf(" %18.10e %18.10e %18.10e\n", m[1], m[3], m[4]);
  printf(" %18.10e %18.10e %18.10e\n", m[2], m[4], m[5]);
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_inspect(REF_NODE ref_node) {
  REF_INT node;
  REF_DBL m[6];
  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_node_metric_get(ref_node, node, m), "get");
    RSS(ref_metric_show(m), "show it");
  }
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_from_node(REF_DBL *metric, REF_NODE ref_node) {
  REF_INT node;

  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_node_metric_get(ref_node, node, &(metric[6 * node])), "get");
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_to_node(REF_DBL *metric, REF_NODE ref_node) {
  REF_INT node;

  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_node_metric_set(ref_node, node, &(metric[6 * node])), "set");
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_olympic_node(REF_NODE ref_node, REF_DBL h) {
  REF_INT node;
  REF_DBL hh;

  each_ref_node_valid_node(ref_node, node) {
    hh = h + (0.1 - h) * ABS(ref_node_xyz(ref_node, 2, node) - 0.5) / 0.5;
    RSS(ref_node_metric_form(ref_node, node, 1.0 / (0.1 * 0.1), 0, 0,
                             1.0 / (0.1 * 0.1), 0, 1.0 / (hh * hh)),
        "set node met");
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_side_node(REF_NODE ref_node) {
  REF_INT node;
  REF_DBL h0 = 0.1;
  REF_DBL h = 0.01;
  REF_DBL hh;

  each_ref_node_valid_node(ref_node, node) {
    hh = h + (h0 - h) * ABS(ref_node_xyz(ref_node, 2, node));
    RSS(ref_node_metric_form(ref_node, node, 1.0 / (0.1 * 0.1), 0, 0,
                             1.0 / (0.1 * 0.1), 0, 1.0 / (hh * hh)),
        "set node met");
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_ring_node(REF_NODE ref_node) {
  REF_INT node;
  REF_DBL hh;
  REF_DBL h = 0.01;
  REF_DBL x;
  each_ref_node_valid_node(ref_node, node) {
    x = ref_node_xyz(ref_node, 0, node);
    hh = h + (0.1 - h) * MIN(2 * ABS(x - 1.0), 1);
    RSS(ref_node_metric_form(ref_node, node, 1.0 / (hh * hh), 0, 0,
                             1.0 / (0.1 * 0.1), 0, 1.0 / (0.1 * 0.1)),
        "set node met");
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_twod_analytic_node(REF_NODE ref_node,
                                                 const char *version) {
  REF_INT node;
  REF_DBL x = 0, y = 0, r, t;
  REF_DBL h_z = 1, h_t = 1, h_r = 1, h0, h, hh, hy, hx, c, k1, d0;
  REF_DBL d[12], m[6];
  REF_BOOL metric_recognized = REF_FALSE;

  each_ref_node_valid_node(ref_node, node) {
    if (strcmp(version, "iso01") == 0) {
      metric_recognized = REF_TRUE;
      h = 0.1;
      RSS(ref_node_metric_form(ref_node, node, 1.0 / (h * h), 0, 0,
                               1.0 / (h * h), 0, 1.0),
          "set node met");
      continue;
    }
    if (strcmp(version, "isorad") == 0) {
      metric_recognized = REF_TRUE;
      r = sqrt(pow(ref_node_xyz(ref_node, 0, node), 2) +
               pow(ref_node_xyz(ref_node, 1, node), 2) +
               pow(ref_node_xyz(ref_node, 2, node), 2));
      h = 0.1 + 0.1 * r;
      RSS(ref_node_metric_form(ref_node, node, 1.0 / (h * h), 0, 0,
                               1.0 / (h * h), 0, 1.0),
          "set node met");
      continue;
    }
    if (strcmp(version, "masabl-1") == 0) {
      metric_recognized = REF_TRUE;
      hx = 0.01 +
           0.2 * cos(ref_math_pi * (ref_node_xyz(ref_node, 0, node) - 0.5));
      c = 0.001;
      k1 = 6.0;
      hy = c * exp(k1 * ref_node_xyz(ref_node, 1, node));
      RSS(ref_node_metric_form(ref_node, node, 1.0 / (hx * hx), 0, 0,
                               1.0 / (hy * hy), 0, 1.0),
          "set node met");
      continue;
    }
    if (strcmp(version, "side") == 0) {
      metric_recognized = REF_TRUE;
      h0 = 0.1;
      h = 0.01;
      hh = h + (h0 - h) * ref_node_xyz(ref_node, 1, node);
      RSS(ref_node_metric_form(ref_node, node, 1.0 / (0.1 * 0.1), 0, 0,
                               1.0 / (hh * hh), 0, 1.0),
          "set node met");
      continue;
    }
    if (strcmp(version, "linear-0001") == 0) {
      metric_recognized = REF_TRUE;
      h0 = 0.1;
      h = 0.0001;
      hh = h + (0.1 - h) * ABS(ref_node_xyz(ref_node, 1, node) - 0.5) / 0.5;
      RSS(ref_node_metric_form(ref_node, node, 1.0 / (0.1 * 0.1), 0, 0,
                               1.0 / (hh * hh), 0, 1.0),
          "set node met");
      continue;
    }
    if (strcmp(version, "linear-01") == 0) {
      metric_recognized = REF_TRUE;
      h0 = 0.1;
      h = 0.01;
      hh = h + (0.1 - h) * ABS(ref_node_xyz(ref_node, 1, node) - 0.5) / 0.5;
      RSS(ref_node_metric_form(ref_node, node, 1.0 / (0.1 * 0.1), 0, 0,
                               1.0 / (hh * hh), 0, 1.0),
          "set node met");
      continue;
    }
    if (strcmp(version, "polar-2") == 0) {
      metric_recognized = REF_TRUE;
      x = ref_node_xyz(ref_node, 0, node);
      y = ref_node_xyz(ref_node, 1, node);
      r = sqrt(x * x + y * y);
      h_z = 1.0;
      h_t = 0.1;
      h0 = 0.001;
      h_r = h0 + 2 * (0.1 - h0) * ABS(r - 0.5);
      d0 = MIN(10.0 * ABS(r - 0.5), 1.0);
      h_t = 0.1 * d0 + 0.025 * (1.0 - d0);
    }
    if (strcmp(version, "radial-1") == 0) {
      metric_recognized = REF_TRUE;
      x = ref_node_xyz(ref_node, 0, node) + 0.5;
      y = ref_node_xyz(ref_node, 1, node) - 0.5;
      t = atan2(y, x);
      h_z = 1.0;
      h_t = 0.1;
      h_r = 0.01;
    }
    if (strcmp(version, "circle-1") == 0) {
      metric_recognized = REF_TRUE;
      x = ref_node_xyz(ref_node, 0, node) + 0.5;
      y = ref_node_xyz(ref_node, 1, node) - 0.5;
      r = sqrt(x * x + y * y);
      t = atan2(y, x);
      h_z = 1.0;
      h_r = 0.0005 + 1.5 * ABS(1.0 - r);
      h_t = 0.1 * r + 1.5 * ABS(1.0 - r);
    }
    t = atan2(y, x);
    ref_matrix_eig(d, 0) = 1.0 / (h_r * h_r);
    ref_matrix_eig(d, 1) = 1.0 / (h_t * h_t);
    ref_matrix_eig(d, 2) = 1.0 / (h_z * h_z);
    ref_matrix_vec(d, 0, 0) = cos(t);
    ref_matrix_vec(d, 1, 0) = sin(t);
    ref_matrix_vec(d, 2, 0) = 0.0;
    ref_matrix_vec(d, 0, 1) = -sin(t);
    ref_matrix_vec(d, 1, 1) = cos(t);
    ref_matrix_vec(d, 2, 1) = 0.0;
    ref_matrix_vec(d, 0, 2) = 0.0;
    ref_matrix_vec(d, 1, 2) = 0.0;
    ref_matrix_vec(d, 2, 2) = 1.0;
    ref_matrix_form_m(d, m);
    RSS(ref_node_metric_set(ref_node, node, m), "set node met");
  }

  RAS(metric_recognized, "metric unknown");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_ugawg_node(REF_NODE ref_node, REF_INT version) {
  REF_INT node;
  REF_DBL x, y, r, t;
  REF_DBL h_z, h_t, h_r, h0, d0;
  REF_DBL d[12], m[6];

  if (1 == version || 2 == version) {
    each_ref_node_valid_node(ref_node, node) {
      x = ref_node_xyz(ref_node, 0, node);
      y = ref_node_xyz(ref_node, 1, node);
      r = sqrt(x * x + y * y);
      t = atan2(y, x);
      h_z = 0.1;
      h_t = 0.1;
      h0 = 0.001;
      h_r = h0 + 2 * (0.1 - h0) * ABS(r - 0.5);
      if (2 == version) {
        d0 = MIN(10.0 * ABS(r - 0.5), 1.0);
        h_t = 0.1 * d0 + 0.025 * (1.0 - d0);
      }
      ref_matrix_eig(d, 0) = 1.0 / (h_r * h_r);
      ref_matrix_eig(d, 1) = 1.0 / (h_t * h_t);
      ref_matrix_eig(d, 2) = 1.0 / (h_z * h_z);
      ref_matrix_vec(d, 0, 0) = cos(t);
      ref_matrix_vec(d, 1, 0) = sin(t);
      ref_matrix_vec(d, 2, 0) = 0.0;
      ref_matrix_vec(d, 0, 1) = -sin(t);
      ref_matrix_vec(d, 1, 1) = cos(t);
      ref_matrix_vec(d, 2, 1) = 0.0;
      ref_matrix_vec(d, 0, 2) = 0.0;
      ref_matrix_vec(d, 1, 2) = 0.0;
      ref_matrix_vec(d, 2, 2) = 1.0;
      ref_matrix_form_m(d, m);
      RSS(ref_node_metric_set(ref_node, node, m), "set node met");
    }

    return REF_SUCCESS;
  }

  return REF_IMPLEMENT;
}

REF_FCN REF_STATUS ref_metric_masabl_node(REF_NODE ref_node) {
  REF_INT node;
  REF_DBL hx, hz, c, k1;

  each_ref_node_valid_node(ref_node, node) {
    hx =
        0.01 + 0.2 * cos(ref_math_pi * (ref_node_xyz(ref_node, 0, node) - 0.5));
    c = 0.001;
    k1 = 6.0;
    hz = c * exp(k1 * ref_node_xyz(ref_node, 2, node));
    RSS(ref_node_metric_form(ref_node, node, 1.0 / (hx * hx), 0, 0,
                             1.0 / (0.1 * 0.1), 0, 1.0 / (hz * hz)),
        "set node met");
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_circle_node(REF_NODE ref_node) {
  REF_INT node;
  REF_DBL x, z, r, t;
  REF_DBL hy, h1, h2;
  REF_DBL d[12], m[6];

  each_ref_node_valid_node(ref_node, node) {
    x = ref_node_xyz(ref_node, 0, node);
    z = ref_node_xyz(ref_node, 2, node);
    r = sqrt(x * x + z * z);
    t = atan2(z, x);
    hy = 1.0;
    h1 = 0.0005 + 1.5 * ABS(1.0 - r);
    h2 = 0.1 * r + 1.5 * ABS(1.0 - r);
    ref_matrix_eig(d, 0) = 1.0 / (h1 * h1);
    ref_matrix_eig(d, 1) = 1.0 / (h2 * h2);
    ref_matrix_eig(d, 2) = 1.0 / (hy * hy);
    ref_matrix_vec(d, 0, 0) = cos(t);
    ref_matrix_vec(d, 1, 0) = 0.0;
    ref_matrix_vec(d, 2, 0) = sin(t);
    ref_matrix_vec(d, 0, 1) = -sin(t);
    ref_matrix_vec(d, 1, 1) = 0.0;
    ref_matrix_vec(d, 2, 1) = cos(t);
    ref_matrix_vec(d, 0, 2) = 0.0;
    ref_matrix_vec(d, 1, 2) = 1.0;
    ref_matrix_vec(d, 2, 2) = 0.0;
    ref_matrix_form_m(d, m);
    RSS(ref_node_metric_set(ref_node, node, m), "set node met");
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_twod_node(REF_NODE ref_node) {
  REF_INT node;
  REF_DBL m[6];

  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_node_metric_get(ref_node, node, m), "get");
    m[2] = 0.0;
    m[4] = 0.0;
    m[5] = 1.0;
    RSS(ref_node_metric_set(ref_node, node, m), "set");
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_delta_box_node(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node;
  REF_DBL m[6], m_int[6], m_target[6];
  REF_DBL radius, h;
  REF_DBL factor = 1.0;
  REF_DBL n[3], dot;

  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_node_metric_get(ref_node, node, m), "get");
    n[0] = 0.90631;
    n[1] = 0.42262;
    n[2] = 0.0;
    dot = ref_math_dot(n, ref_node_xyz_ptr(ref_node, node));
    radius = sqrt(pow(ref_node_xyz(ref_node, 0, node) - dot * n[0], 2) +
                  pow(ref_node_xyz(ref_node, 1, node) - dot * n[1], 2) +
                  pow(ref_node_xyz(ref_node, 2, node) - dot * n[2], 2));
    h = MIN(0.0003 + 0.002 * radius / (0.15 * 0.42262), 0.007);
    h *= factor;
    m_target[0] = 1.0 / (h * h);
    m_target[1] = 0;
    m_target[2] = 0;
    m_target[3] = 1.0 / (h * h);
    m_target[4] = 0;
    m_target[5] = 1.0 / (h * h);
    if (ref_node_xyz(ref_node, 0, node) > 1.1) {
      REF_DBL ht, s, hx;
      ht = 0.007;
      s = (ref_node_xyz(ref_node, 0, node) - 1.1) / 3.9;
      s = MIN(1.0, MAX(0.0, s));
      hx = ht * (1.0 - s) + 10 * ht * s;
      hx *= factor;
      ht *= factor;
      m_target[0] = 1.0 / (hx * hx);
      m_target[1] = 0;
      m_target[2] = 0;
      m_target[3] = 1.0 / (ht * ht);
      m_target[4] = 0;
      m_target[5] = 1.0 / (ht * ht);
    }
    if (0.1 < ref_node_xyz(ref_node, 2, node) ||
        -0.1 > ref_node_xyz(ref_node, 2, node) ||
        1.0 < ref_node_xyz(ref_node, 1, node) ||
        -0.1 > ref_node_xyz(ref_node, 0, node)) {
      h = 0.1 + ABS(ref_node_xyz(ref_node, 2, node)) / 5.0 +
          ABS(ref_node_xyz(ref_node, 1, node)) / 5.0 +
          ABS(ref_node_xyz(ref_node, 0, node)) / 5.0;
      h *= factor;
      m_target[0] = 1.0 / (h * h);
      m_target[1] = 0;
      m_target[2] = 0;
      m_target[3] = 1.0 / (h * h);
      m_target[4] = 0;
      m_target[5] = 1.0 / (h * h);
    }
    RSS(ref_matrix_intersect(m_target, m, m_int), "intersect");
    RSS(ref_node_metric_set(ref_node, node, m_int), "set node met");
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_interpolate_node(REF_GRID ref_grid,
                                               REF_INT node) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_INTERP ref_interp;
  REF_GRID from_grid;
  REF_NODE from_node;
  REF_CELL from_cell;
  REF_DBL log_parent_m[4][6], log_m[6], bary[4];
  REF_INT ibary, im;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];

  if (NULL == ref_grid_interp(ref_grid)) {
    return REF_SUCCESS;
  }

  ref_interp = ref_grid_interp(ref_grid);
  from_grid = ref_interp_from_grid(ref_interp);
  from_node = ref_grid_node(from_grid);
  if (ref_grid_twod(from_grid)) {
    from_cell = ref_interp_from_tri(ref_interp);
  } else {
    from_cell = ref_interp_from_tet(ref_interp);
  }

  if (!ref_interp_continuously(ref_interp)) {
    ref_interp_cell(ref_interp, node) = REF_EMPTY; /* mark moved */
    return REF_SUCCESS;
  }

  RAISE(ref_interp_locate_node(ref_interp, node));

  /* location unsuccessful */
  if (REF_EMPTY == ref_interp_cell(ref_interp, node) ||
      ref_mpi_rank(ref_mpi) != ref_interp->part[node])
    return REF_SUCCESS;
  RSS(ref_cell_nodes(from_cell, ref_interp_cell(ref_interp, node), nodes),
      "node needs to be localized");
  RSS(ref_node_clip_bary4(&ref_interp_bary(ref_interp, 0, node), bary), "clip");
  for (ibary = 0; ibary < ref_cell_node_per(from_cell); ibary++)
    RSS(ref_node_metric_get_log(from_node, nodes[ibary], log_parent_m[ibary]),
        "log(parentM)");
  for (im = 0; im < 6; im++) log_m[im] = 0.0;
  for (im = 0; im < 6; im++) {
    for (ibary = 0; ibary < ref_cell_node_per(from_cell); ibary++) {
      log_m[im] += bary[ibary] * log_parent_m[ibary][im];
    }
  }
  RSS(ref_node_metric_set_log(ref_grid_node(ref_grid), node, log_m),
      "set interp log met");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_interpolate_between(REF_GRID ref_grid,
                                                  REF_INT node0, REF_INT node1,
                                                  REF_INT new_node) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_INTERP ref_interp;
  REF_GRID from_grid;
  REF_NODE from_node;
  REF_CELL from_cell;
  REF_NODE to_node;
  REF_DBL log_parent_m[4][6], log_m[6], bary[4];
  REF_INT ibary, im;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];

  /* skip null interp */
  if (NULL == ref_grid_interp(ref_grid)) return REF_SUCCESS;
  ref_interp = ref_grid_interp(ref_grid);
  from_grid = ref_interp_from_grid(ref_interp);
  from_node = ref_grid_node(from_grid);
  from_node = ref_grid_node(from_grid);
  if (ref_grid_twod(from_grid)) {
    from_cell = ref_interp_from_tri(ref_interp);
  } else {
    from_cell = ref_interp_from_tet(ref_interp);
  }
  to_node = ref_grid_node(ref_interp_to_grid(ref_interp));

  if (new_node >= ref_interp_max(ref_interp)) {
    RSS(ref_interp_resize(ref_interp, ref_node_max(to_node)), "resize");
  }

  if (!ref_interp_continuously(ref_interp)) {
    ref_interp->cell[new_node] = REF_EMPTY; /* initialize new_node locate */
    return REF_SUCCESS;
  }

  RAISE(ref_interp_locate_between(ref_interp, node0, node1, new_node));

  /* location unsuccessful or off-part don't interpolate */
  if (REF_EMPTY == ref_interp_cell(ref_interp, new_node) ||
      ref_mpi_rank(ref_mpi) != ref_interp_part(ref_interp, new_node))
    return REF_SUCCESS;

  RSS(ref_cell_nodes(from_cell, ref_interp_cell(ref_interp, new_node), nodes),
      "new_node needs to be localized");
  RSS(ref_node_clip_bary4(&ref_interp_bary(ref_interp, 0, new_node), bary),
      "clip");

  for (ibary = 0; ibary < ref_cell_node_per(from_cell); ibary++)
    RSS(ref_node_metric_get_log(from_node, nodes[ibary], log_parent_m[ibary]),
        "log(parentM)");
  for (im = 0; im < 6; im++) log_m[im] = 0.0;
  for (im = 0; im < 6; im++) {
    for (ibary = 0; ibary < ref_cell_node_per(from_cell); ibary++) {
      log_m[im] += bary[ibary] * log_parent_m[ibary][im];
    }
  }
  RSS(ref_node_metric_set_log(ref_grid_node(ref_grid), new_node, log_m),
      "set interp log met");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_interpolate(REF_INTERP ref_interp) {
  REF_GRID to_grid = ref_interp_to_grid(ref_interp);
  REF_GRID from_grid = ref_interp_from_grid(ref_interp);
  REF_NODE to_node = ref_grid_node(to_grid);
  REF_NODE from_node = ref_grid_node(from_grid);
  REF_CELL from_cell;
  REF_MPI ref_mpi = ref_interp_mpi(ref_interp);
  REF_INT node, ibary, im;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL log_parent_m[4][6];
  REF_INT receptor, n_recept, donation, n_donor;
  REF_DBL *recept_log_m, *donor_log_m, *recept_bary, *donor_bary;
  REF_INT *donor_node, *donor_ret, *donor_cell;
  REF_INT *recept_proc, *recept_ret, *recept_node, *recept_cell;

  if (ref_grid_twod(from_grid)) {
    from_cell = ref_interp_from_tri(ref_interp);
  } else {
    from_cell = ref_interp_from_tet(ref_interp);
  }

  n_recept = 0;
  each_ref_node_valid_node(to_node, node) {
    if (ref_node_owned(to_node, node)) {
      n_recept++;
    }
  }

  ref_malloc(recept_bary, 4 * n_recept, REF_DBL);
  ref_malloc(recept_cell, n_recept, REF_INT);
  ref_malloc(recept_node, n_recept, REF_INT);
  ref_malloc(recept_ret, n_recept, REF_INT);
  ref_malloc(recept_proc, n_recept, REF_INT);

  n_recept = 0;
  each_ref_node_valid_node(to_node, node) {
    if (ref_node_owned(to_node, node)) {
      RUS(REF_EMPTY, ref_interp->cell[node], "node needs to be localized");
      RSS(ref_node_clip_bary4(&(ref_interp->bary[4 * node]),
                              &(recept_bary[4 * n_recept])),
          "clip");
      recept_proc[n_recept] = ref_interp->part[node];
      recept_cell[n_recept] = ref_interp->cell[node];
      recept_node[n_recept] = node;
      recept_ret[n_recept] = ref_mpi_rank(ref_mpi);
      n_recept++;
    }
  }

  RSS(ref_mpi_blindsend(ref_mpi, recept_proc, (void *)recept_cell, 1, n_recept,
                        (void **)(&donor_cell), &n_donor, REF_INT_TYPE),
      "blind send cell");
  RSS(ref_mpi_blindsend(ref_mpi, recept_proc, (void *)recept_ret, 1, n_recept,
                        (void **)(&donor_ret), &n_donor, REF_INT_TYPE),
      "blind send ret");
  RSS(ref_mpi_blindsend(ref_mpi, recept_proc, (void *)recept_node, 1, n_recept,
                        (void **)(&donor_node), &n_donor, REF_INT_TYPE),
      "blind send node");
  RSS(ref_mpi_blindsend(ref_mpi, recept_proc, (void *)recept_bary, 4, n_recept,
                        (void **)(&donor_bary), &n_donor, REF_DBL_TYPE),
      "blind send bary");

  ref_free(recept_proc);
  ref_free(recept_ret);
  ref_free(recept_node);
  ref_free(recept_cell);
  ref_free(recept_bary);

  ref_malloc(donor_log_m, 6 * n_donor, REF_DBL);

  for (donation = 0; donation < n_donor; donation++) {
    RSS(ref_cell_nodes(from_cell, donor_cell[donation], nodes),
        "node needs to be localized");
    for (ibary = 0; ibary < 4; ibary++)
      RSS(ref_node_metric_get_log(from_node, nodes[ibary], log_parent_m[ibary]),
          "log(parentM)");
    for (im = 0; im < 6; im++) {
      donor_log_m[im + 6 * donation] = 0.0;
      for (ibary = 0; ibary < 4; ibary++) {
        donor_log_m[im + 6 * donation] +=
            donor_bary[ibary + 4 * donation] * log_parent_m[ibary][im];
      }
    }
  }
  ref_free(donor_cell);
  ref_free(donor_bary);

  RSS(ref_mpi_blindsend(ref_mpi, donor_ret, (void *)donor_log_m, 6, n_donor,
                        (void **)(&recept_log_m), &n_recept, REF_DBL_TYPE),
      "blind send bary");
  RSS(ref_mpi_blindsend(ref_mpi, donor_ret, (void *)donor_node, 1, n_donor,
                        (void **)(&recept_node), &n_recept, REF_INT_TYPE),
      "blind send node");
  ref_free(donor_log_m);
  ref_free(donor_node);
  ref_free(donor_ret);

  for (receptor = 0; receptor < n_recept; receptor++) {
    node = recept_node[receptor];
    RSS(ref_node_metric_set_log(to_node, node, &(recept_log_m[6 * receptor])),
        "set received log met");
  }

  ref_free(recept_node);
  ref_free(recept_log_m);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_synchronize(REF_GRID to_grid) {
  REF_INTERP ref_interp = ref_grid_interp(to_grid);
  REF_MPI ref_mpi;
  REF_INT node;
  REF_DBL max_error, tol = 1.0e-8;
  REF_BOOL report_interp_error = REF_FALSE;

  if (NULL == ref_interp) return REF_SUCCESS;

  ref_mpi = ref_interp_mpi(ref_interp);

  if (ref_mpi_para(ref_mpi)) {
    /* parallel can miss on partition boundaries, refresh interp */
    RSS(ref_interp_locate_warm(ref_interp), "map from existing");
    RSS(ref_metric_interpolate(ref_interp), "interp");
  } else {
    /* sequential should always localized unless mixed, assert */
    each_ref_node_valid_node(ref_grid_node(to_grid), node) {
      if (!ref_cell_node_empty(ref_grid_tri(to_grid), node) &&
          !ref_cell_node_empty(ref_grid_tet(to_grid), node)) {
        RUS(REF_EMPTY, ref_interp_cell(ref_interp, node), "should be located");
      }
    }
  }

  if (report_interp_error) {
    RSS(ref_interp_max_error(ref_interp, &max_error), "err");
    if (max_error > tol && ref_mpi_once(ref_mpi)) {
      printf("warning: %e max_error greater than %e tol\n", max_error, tol);
    }
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_metric_space_gradation(REF_DBL *metric,
                                                     REF_GRID ref_grid,
                                                     REF_DBL r) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_EDGE ref_edge;
  REF_DBL *metric_orig;
  REF_DBL ratio, enlarge, log_r;
  REF_DBL direction[3];
  REF_DBL limit_metric[6], limited[6];
  REF_INT node, i;
  REF_INT edge, node0, node1;

  log_r = log(r);

  RSS(ref_edge_create(&ref_edge, ref_grid), "orig edges");

  ref_malloc(metric_orig, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);

  each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
    for (i = 0; i < 6; i++) metric_orig[i + 6 * node] = metric[i + 6 * node];
  }

  /* F. Alauzet doi:10.1016/j.finel.2009.06.028 equation (9) */

  each_ref_edge(ref_edge, edge) {
    node0 = ref_edge_e2n(ref_edge, 0, edge);
    node1 = ref_edge_e2n(ref_edge, 1, edge);
    direction[0] =
        (ref_node_xyz(ref_node, 0, node1) - ref_node_xyz(ref_node, 0, node0));
    direction[1] =
        (ref_node_xyz(ref_node, 1, node1) - ref_node_xyz(ref_node, 1, node0));
    direction[2] =
        (ref_node_xyz(ref_node, 2, node1) - ref_node_xyz(ref_node, 2, node0));

    ratio = ref_matrix_sqrt_vt_m_v(&(metric_orig[6 * node1]), direction);
    enlarge = pow(1.0 + ratio * log_r, -2.0);
    for (i = 0; i < 6; i++)
      limit_metric[i] = metric_orig[i + 6 * node1] * enlarge;
    if (REF_SUCCESS != ref_matrix_intersect(&(metric_orig[6 * node0]),
                                            limit_metric, limited)) {
      REF_WHERE("limit m0 with enlarged m1");
      ref_node_location(ref_node, node0);
      printf("ratio %24.15e enlarge %24.15e \n", ratio, enlarge);
      printf("RECOVER ref_metric_metric_space_gradation\n");
      continue;
    }
    if (REF_SUCCESS != ref_matrix_intersect(&(metric[6 * node0]), limited,
                                            &(metric[6 * node0]))) {
      REF_WHERE("update m0");
      ref_node_location(ref_node, node0);
      printf("ratio %24.15e enlarge %24.15e \n", ratio, enlarge);
      printf("RECOVER ref_metric_metric_space_gradation\n");
      continue;
    }
    ratio = ref_matrix_sqrt_vt_m_v(&(metric_orig[6 * node0]), direction);
    enlarge = pow(1.0 + ratio * log_r, -2.0);
    for (i = 0; i < 6; i++)
      limit_metric[i] = metric_orig[i + 6 * node0] * enlarge;
    if (REF_SUCCESS != ref_matrix_intersect(&(metric_orig[6 * node1]),
                                            limit_metric, limited)) {
      REF_WHERE("limit m1 with enlarged m0");
      ref_node_location(ref_node, node1);
      printf("ratio %24.15e enlarge %24.15e \n", ratio, enlarge);
      printf("RECOVER ref_metric_metric_space_gradation\n");
      continue;
    }
    if (REF_SUCCESS != ref_matrix_intersect(&(metric[6 * node1]), limited,
                                            &(metric[6 * node1]))) {
      REF_WHERE("update m1");
      ref_node_location(ref_node, node1);
      printf("ratio %24.15e enlarge %24.15e \n", ratio, enlarge);
      printf("RECOVER ref_metric_metric_space_gradation\n");
      continue;
    }
  }

  ref_free(metric_orig);

  ref_edge_free(ref_edge);

  RSS(ref_node_ghost_dbl(ref_grid_node(ref_grid), metric, 6), "update ghosts");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_mixed_space_gradation(REF_DBL *metric,
                                                    REF_GRID ref_grid,
                                                    REF_DBL r, REF_DBL t) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_EDGE ref_edge;
  REF_DBL *metric_orig;
  REF_DBL dist, ratio, enlarge, log_r;
  REF_DBL direction[3];
  REF_DBL limit_metric[6], limited[6];
  REF_INT node, i;
  REF_INT edge, node0, node1;
  REF_DBL diag_system[12];
  REF_DBL metric_space, phys_space;

  if (r < 1.0) r = 1.5;
  if (t < 0.0 || 1.0 > t) t = 1.0 / 8.0;

  log_r = log(r);

  RSS(ref_edge_create(&ref_edge, ref_grid), "orig edges");

  ref_malloc(metric_orig, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);

  each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
    for (i = 0; i < 6; i++) metric_orig[i + 6 * node] = metric[i + 6 * node];
  }

  /* F. Alauzet doi:10.1016/j.finel.2009.06.028
   * 6.2.1. Mixed-space homogeneous gradation */

  each_ref_edge(ref_edge, edge) {
    node0 = ref_edge_e2n(ref_edge, 0, edge);
    node1 = ref_edge_e2n(ref_edge, 1, edge);
    direction[0] =
        (ref_node_xyz(ref_node, 0, node1) - ref_node_xyz(ref_node, 0, node0));
    direction[1] =
        (ref_node_xyz(ref_node, 1, node1) - ref_node_xyz(ref_node, 1, node0));
    direction[2] =
        (ref_node_xyz(ref_node, 2, node1) - ref_node_xyz(ref_node, 2, node0));
    dist = sqrt(ref_math_dot(direction, direction));

    ratio = ref_matrix_sqrt_vt_m_v(&(metric_orig[6 * node1]), direction);

    RSB(ref_matrix_diag_m(&(metric_orig[6 * node1]), diag_system), "decomp",
        { ref_metric_show(&(metric_orig[6 * node1])); });
    for (i = 0; i < 3; i++) {
      metric_space = 1.0 + log_r * ratio;
      phys_space = 1.0 + sqrt(ref_matrix_eig(diag_system, i)) * dist * log_r;
      enlarge = pow(pow(phys_space, t) * pow(metric_space, 1.0 - t), -2.0);
      ref_matrix_eig(diag_system, i) *= enlarge;
    }
    RSS(ref_matrix_form_m(diag_system, limit_metric), "reform limit");

    if (REF_SUCCESS == ref_matrix_intersect(&(metric_orig[6 * node0]),
                                            limit_metric, limited)) {
      RSS(ref_matrix_intersect(&(metric[6 * node0]), limited,
                               &(metric[6 * node0])),
          "update m0");
    } else {
      ref_node_location(ref_node, node0);
      printf("dist %24.15e ratio %24.15e\n", dist, ratio);
      printf("RECOVER ref_metric_mixed_space_gradation\n");
    }

    ratio = ref_matrix_sqrt_vt_m_v(&(metric_orig[6 * node0]), direction);

    RSB(ref_matrix_diag_m(&(metric_orig[6 * node0]), diag_system), "decomp",
        { ref_metric_show(&(metric_orig[6 * node0])); });
    for (i = 0; i < 3; i++) {
      metric_space = 1.0 + log_r * ratio;
      phys_space = 1.0 + sqrt(ref_matrix_eig(diag_system, i)) * dist * log_r;
      enlarge = pow(pow(phys_space, t) * pow(metric_space, 1.0 - t), -2.0);
      ref_matrix_eig(diag_system, i) *= enlarge;
    }
    RSS(ref_matrix_form_m(diag_system, limit_metric), "reform limit");

    if (REF_SUCCESS == ref_matrix_intersect(&(metric_orig[6 * node1]),
                                            limit_metric, limited)) {
      RSS(ref_matrix_intersect(&(metric[6 * node1]), limited,
                               &(metric[6 * node1])),
          "update m1");
    } else {
      ref_node_location(ref_node, node1);
      printf("dist %24.15e ratio %24.15e\n", dist, ratio);
      printf("RECOVER ref_metric_mixed_space_gradation\n");
    }
  }

  ref_free(metric_orig);

  ref_edge_free(ref_edge);

  RSS(ref_node_ghost_dbl(ref_grid_node(ref_grid), metric, 6), "update ghosts");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_hessian_filter(REF_DBL *metric,
                                             REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_DBL *min_h, *hess_min_h, *threshold;
  REF_DBL temp, max_threshold, max_valid, eig;
  REF_INT node;
  ref_malloc(min_h, ref_node_max(ref_node), REF_DBL);
  ref_malloc(hess_min_h, 6 * ref_node_max(ref_node), REF_DBL);
  ref_malloc(threshold, ref_node_max(ref_node), REF_DBL);

  each_ref_node_valid_node(ref_node, node) {
    REF_DBL diag[12];
    RSS(ref_matrix_diag_m(&(metric[6 * node]), diag), "decomp");
    if (ref_grid_twod(ref_grid)) {
      RSS(ref_matrix_descending_eig_twod(diag), "2D ascend");
    } else {
      RSS(ref_matrix_descending_eig(diag), "3D ascend");
    }
    min_h[node] = sqrt(MAX(0.0, ref_matrix_eig(diag, 0)));
    if (ref_math_divisible(1.0, min_h[node])) {
      min_h[node] = 1.0 / min_h[node];
    } else {
      min_h[node] = REF_DBL_MAX;
    }
  }

  RSS(ref_recon_hessian(ref_grid, min_h, hess_min_h, REF_RECON_L2PROJECTION),
      "hess");

  max_threshold = 0.0;
  each_ref_node_valid_node(ref_node, node) {
    REF_DBL diag[12];
    RSS(ref_matrix_diag_m(&(hess_min_h[6 * node]), diag), "decomp");
    if (ref_grid_twod(ref_grid)) {
      RSS(ref_matrix_descending_eig_twod(diag), "2D ascend");
    } else {
      RSS(ref_matrix_descending_eig(diag), "3D ascend");
    }
    threshold[node] = log10(ABS(ref_matrix_eig(diag, 0)));
    max_threshold = MAX(max_threshold, threshold[node]);
  }
  temp = max_threshold;
  RSS(ref_mpi_max(ref_mpi, &temp, &max_threshold, REF_DBL_TYPE), "max");

  /* find the largest valid eigenvalue that is not in a discontinuity */
  max_threshold = 0.5 * max_threshold;
  max_valid = 0.0;
  each_ref_node_valid_node(ref_node, node) {
    if (threshold[node] < max_threshold) {
      REF_DBL h2 = min_h[node] * min_h[node];
      if (ref_math_divisible(1.0, h2)) {
        eig = 1.0 / h2;
        max_valid = MAX(max_valid, eig);
      }
    }
  }
  temp = max_valid;
  RSS(ref_mpi_max(ref_mpi, &temp, &max_valid, REF_DBL_TYPE), "max");

  /* limit hessian eigenvalues everwhere based on what is considered valid
   * outside a discontinuity */
  each_ref_node_valid_node(ref_node, node) {
    REF_DBL diag[12];
    RSS(ref_matrix_diag_m(&(metric[6 * node]), diag), "decomp");
    if (ref_grid_twod(ref_grid)) {
      RSS(ref_matrix_descending_eig_twod(diag), "2D ascend");
    } else {
      RSS(ref_matrix_descending_eig(diag), "3D ascend");
    }
    ref_matrix_eig(diag, 0) = MIN(ref_matrix_eig(diag, 0), max_valid);
    ref_matrix_eig(diag, 1) = MIN(ref_matrix_eig(diag, 1), max_valid);
    if (!ref_grid_twod(ref_grid))
      ref_matrix_eig(diag, 2) = MIN(ref_matrix_eig(diag, 2), max_valid);
    RSS(ref_matrix_form_m(diag, &(metric[6 * node])), "reform");
  }
  ref_free(threshold);
  ref_free(hess_min_h);
  ref_free(min_h);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_interpolation_error(
    REF_DBL *metric, REF_DBL *hess, REF_GRID ref_grid,
    REF_DBL *interpolation_error) {
  /* Corollary 3.4 CONTINUOUS MESH FRAMEWORK PART I DOI:10.1137/090754078 */
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node;
  REF_DBL error[6], m1half[6], m1neghalf[6], det, sqrt_det;
  REF_DBL constant = 1.0 / 10.0;
  if (ref_grid_twod(ref_grid)) {
    constant = 1.0 / 8.0;
  }
  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      RSS(ref_matrix_sqrt_m(&(metric[6 * node]), m1half, m1neghalf), "m^-1/2");
      RSS(ref_matrix_mult_m0m1m0(m1neghalf, &(hess[6 * node]), error),
          "error=m1half*hess*m1half");
      RSS(ref_matrix_det_m(&(metric[6 * node]), &det), "det");
      if (ref_grid_twod(ref_grid)) {
        interpolation_error[node] = constant * (error[0] + error[3]);
      } else {
        interpolation_error[node] = constant * (error[0] + error[3] + error[5]);
      }
      if (det >= 0.0) {
        sqrt_det = sqrt(det);
        if (ref_math_divisible(interpolation_error[node], sqrt_det)) {
          interpolation_error[node] /= sqrt_det;
        } else {
          interpolation_error[node] = 0.0;
        }
      } else {
        interpolation_error[node] = 0.0;
      }
    }
  }
  RSS(ref_node_ghost_dbl(ref_node, interpolation_error, 1), "update ghosts");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_integrate_error(REF_GRID ref_grid,
                                              REF_DBL *interpolation_error,
                                              REF_DBL *total_error) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT cell_node, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL volume;
  REF_BOOL have_tet;
  REF_LONG ntet;
  RSS(ref_cell_ncell(ref_grid_tet(ref_grid), ref_node, &ntet), "count");
  have_tet = (0 < ntet);
  if (have_tet) {
    ref_cell = ref_grid_tet(ref_grid);
  } else {
    ref_cell = ref_grid_tri(ref_grid);
  }
  *total_error = 0.0;
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (have_tet) {
      RSS(ref_node_tet_vol(ref_node, nodes, &volume), "vol");
    } else {
      RSS(ref_node_tri_area(ref_node, nodes, &volume), "area");
    }
    for (cell_node = 0; cell_node < ref_cell_node_per(ref_cell); cell_node++) {
      if (ref_node_owned(ref_node, nodes[cell_node])) {
        (*total_error) += interpolation_error[nodes[cell_node]] * volume /
                          ((REF_DBL)ref_cell_node_per(ref_cell));
      }
    }
  }
  RSS(ref_mpi_allsum(ref_grid_mpi(ref_grid), total_error, 1, REF_DBL_TYPE),
      "dbl sum");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_gradation_at_complexity(REF_DBL *metric,
                                                      REF_GRID ref_grid,
                                                      REF_DBL gradation,
                                                      REF_DBL complexity) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT relaxations;
  REF_DBL current_complexity;
  REF_DBL complexity_scale;
  REF_INT node, i;

  complexity_scale = 2.0 / 3.0;
  if (ref_grid_twod(ref_grid)) {
    complexity_scale = 1.0;
  }

  for (relaxations = 0; relaxations < 20; relaxations++) {
    RSS(ref_metric_complexity(metric, ref_grid, &current_complexity), "cmp");
    if (!ref_math_divisible(complexity, current_complexity)) {
      if (ref_mpi_once(ref_grid_mpi(ref_grid)))
        printf("div zero target %.18e current %.18e\n", complexity,
               current_complexity);
      return REF_DIV_ZERO;
    }
    each_ref_node_valid_node(ref_node, node) {
      for (i = 0; i < 6; i++) {
        metric[i + 6 * node] *=
            pow(complexity / current_complexity, complexity_scale);
      }
      if (ref_grid_twod(ref_grid)) {
        metric[2 + 6 * node] = 0.0;
        metric[4 + 6 * node] = 0.0;
        metric[5 + 6 * node] = 1.0;
      }
    }
    if (gradation < 1.0) {
      RSS(ref_metric_mixed_space_gradation(metric, ref_grid, -1.0, -1.0),
          "gradation");
    } else {
      RSS(ref_metric_metric_space_gradation(metric, ref_grid, gradation),
          "gradation");
    }
    if (ref_grid_twod(ref_grid)) {
      each_ref_node_valid_node(ref_node, node) {
        metric[2 + 6 * node] = 0.0;
        metric[4 + 6 * node] = 0.0;
        metric[5 + 6 * node] = 1.0;
      }
    }
  }
  RSS(ref_metric_complexity(metric, ref_grid, &current_complexity), "cmp");
  if (!ref_math_divisible(complexity, current_complexity)) {
    return REF_DIV_ZERO;
  }
  each_ref_node_valid_node(ref_node, node) {
    for (i = 0; i < 6; i++) {
      metric[i + 6 * node] *=
          pow(complexity / current_complexity, complexity_scale);
    }
    if (ref_grid_twod(ref_grid)) {
      metric[2 + 6 * node] = 0.0;
      metric[4 + 6 * node] = 0.0;
      metric[5 + 6 * node] = 1.0;
    }
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_gradation_at_complexity_mixed(
    REF_DBL *metric, REF_GRID ref_grid, REF_DBL gradation, REF_DBL complexity) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT relaxations;
  REF_DBL current_complexity;
  REF_DBL complexity_scale;
  REF_INT node, i;
  REF_DBL *metric_imply;

  ref_malloc(metric_imply, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);

  complexity_scale = 2.0 / 3.0;
  if (ref_grid_twod(ref_grid)) {
    complexity_scale = 1.0;
  }

  for (relaxations = 0; relaxations < 20; relaxations++) {
    RSS(ref_metric_complexity(metric, ref_grid, &current_complexity), "cmp");
    if (!ref_math_divisible(complexity, current_complexity)) {
      return REF_DIV_ZERO;
    }
    each_ref_node_valid_node(ref_node, node) {
      for (i = 0; i < 6; i++) {
        metric[i + 6 * node] *=
            pow(complexity / current_complexity, complexity_scale);
      }
      if (ref_grid_twod(ref_grid)) {
        metric[2 + 6 * node] = 0.0;
        metric[4 + 6 * node] = 0.0;
        metric[5 + 6 * node] = 1.0;
      }
    }
    RSS(ref_metric_imply_non_tet(metric, ref_grid), "imply non tet");
    if (gradation < 1.0) {
      RSS(ref_metric_mixed_space_gradation(metric, ref_grid, -1.0, -1.0),
          "gradation");
    } else {
      RSS(ref_metric_metric_space_gradation(metric, ref_grid, gradation),
          "gradation");
    }
    if (ref_grid_twod(ref_grid)) {
      each_ref_node_valid_node(ref_node, node) {
        metric[2 + 6 * node] = 0.0;
        metric[4 + 6 * node] = 0.0;
        metric[5 + 6 * node] = 1.0;
      }
    }
  }
  RSS(ref_metric_complexity(metric, ref_grid, &current_complexity), "cmp");
  if (!ref_math_divisible(complexity, current_complexity)) {
    return REF_DIV_ZERO;
  }
  each_ref_node_valid_node(ref_node, node) {
    for (i = 0; i < 6; i++) {
      metric[i + 6 * node] *=
          pow(complexity / current_complexity, complexity_scale);
    }
    if (ref_grid_twod(ref_grid)) {
      metric[2 + 6 * node] = 0.0;
      metric[4 + 6 * node] = 0.0;
      metric[5 + 6 * node] = 1.0;
    }
  }

  ref_free(metric_imply) return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_sanitize(REF_GRID ref_grid) {
  if (ref_grid_twod(ref_grid)) {
    RSS(ref_metric_sanitize_twod(ref_grid), "threed");
  } else {
    RSS(ref_metric_sanitize_threed(ref_grid), "threed");
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_sanitize_threed(REF_GRID ref_grid) {
  REF_DBL *metric_orig;
  REF_DBL *metric_imply;
  REF_DBL *metric;

  ref_malloc(metric_orig, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
  ref_malloc(metric_imply, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
  ref_malloc(metric, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);

  RSS(ref_metric_from_node(metric_orig, ref_grid_node(ref_grid)), "from");

  RSS(ref_metric_imply_from(metric_imply, ref_grid), "imply");

  RSS(ref_metric_smr(metric_imply, metric_orig, metric, ref_grid), "smr");

  RSS(ref_metric_imply_non_tet(metric, ref_grid), "imply non tet");

  RSS(ref_metric_to_node(metric, ref_grid_node(ref_grid)), "to");

  ref_free(metric);
  ref_free(metric_imply);
  ref_free(metric_orig);

  return REF_SUCCESS;
}
REF_FCN REF_STATUS ref_metric_sanitize_twod(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_DBL *metric_orig;
  REF_DBL *metric_imply;
  REF_DBL *metric;
  REF_INT node;

  ref_malloc(metric_orig, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
  ref_malloc(metric_imply, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
  ref_malloc(metric, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);

  RSS(ref_metric_from_node(metric_orig, ref_grid_node(ref_grid)), "from");
  for (node = 0; node < ref_node_max(ref_node); node++) {
    metric_orig[2 + 6 * node] = 0.0;
    metric_orig[4 + 6 * node] = 0.0;
    metric_orig[5 + 6 * node] = 1.0;
  }

  RSS(ref_metric_imply_from(metric_imply, ref_grid), "imply");
  for (node = 0; node < ref_node_max(ref_node); node++) {
    metric_imply[1 + 6 * node] = 0.0;
    metric_imply[3 + 6 * node] = 1.0;
    metric_imply[4 + 6 * node] = 0.0;
  }

  RSS(ref_metric_smr(metric_imply, metric_orig, metric, ref_grid), "smr");

  RSS(ref_metric_to_node(metric, ref_grid_node(ref_grid)), "to");

  ref_free(metric);
  ref_free(metric_imply);
  ref_free(metric_orig);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_interpolated_curvature(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_DBL *metric;
  REF_INT gradation;

  ref_malloc(metric, 6 * ref_node_max(ref_node), REF_DBL);
  RSS(ref_metric_from_curvature(metric, ref_grid), "curve");
  for (gradation = 0; gradation < 20; gradation++) {
    RSS(ref_metric_mixed_space_gradation(metric, ref_grid, -1.0, -1.0), "grad");
  }
  RSS(ref_metric_to_node(metric, ref_node), "to node");
  ref_free(metric);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_constrain_curvature(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_DBL *curvature_metric;
  REF_DBL m[6], m_constrained[6];
  REF_INT node, gradation;

  ref_malloc(curvature_metric, 6 * ref_node_max(ref_node), REF_DBL);
  RSS(ref_metric_from_curvature(curvature_metric, ref_grid), "curve");
  for (gradation = 0; gradation < 20; gradation++) {
    RSS(ref_metric_mixed_space_gradation(curvature_metric, ref_grid, -1.0,
                                         -1.0),
        "grad");
  }

  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_node_metric_get(ref_node, node, m), "get");
    RSS(ref_matrix_intersect(&(curvature_metric[6 * node]), m, m_constrained),
        "intersect");
    if (ref_grid_twod(ref_grid))
      RSS(ref_matrix_twod_m(m_constrained), "enforce twod");
    RSS(ref_node_metric_set(ref_node, node, m_constrained), "set node met");
  }

  ref_free(curvature_metric);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_from_curvature(REF_DBL *metric,
                                             REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_INT geom, node, face;
  REF_DBL kr, r[3], ks, s[3], n[3];
  REF_DBL diagonal_system[12];
  REF_DBL previous_metric[6], curvature_metric[6];
  REF_INT i;
  REF_DBL delta_radian; /* 1/segments per radian */
  REF_DBL hmax = REF_DBL_MAX;
  REF_DBL rlimit;
  REF_DBL hr, hs, hn, slop;
  REF_DBL aspect_ratio, curvature_ratio, norm_ratio;
  REF_DBL crease_dot_prod, ramp, scale;

  if (ref_geom_model_loaded(ref_geom)) {
    RSS(ref_egads_diagonal(ref_geom, REF_EMPTY, &hmax), "egads bbox diag");
  } else if (ref_geom_meshlinked(ref_geom)) {
    RSS(ref_node_bounding_box_diagonal(ref_node, &hmax), "bbox diag");
  } else {
    printf("\nNo geometry model, did you forget to load it?\n\n");
    RSS(REF_IMPLEMENT, "...or implement non-CAD curvature estimate");
  }
  /* normal spacing and max tangential spacing */
  hmax /= MAX(1.0, ref_geom_segments_per_bounding_box_diagonal(ref_geom));

  /* limit aspect ratio via curvature */
  aspect_ratio = 20.0;
  curvature_ratio = 1.0 / aspect_ratio;

  /* limit normal direction to a factor of surface spacing */
  norm_ratio = 2.0;

  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      if (ref_geom_model_loaded(ref_geom)) {
        RSS(ref_egads_feature_size(ref_grid, node, &hr, r, &hs, s, &hn, n),
            "feature size");
        hs = MIN(hs, hr * aspect_ratio);
        hn = MIN(hn, norm_ratio * hr);
        hn = MIN(hn, norm_ratio * hs);
        for (i = 0; i < 3; i++) ref_matrix_vec(diagonal_system, i, 0) = r[i];
        ref_matrix_eig(diagonal_system, 0) = 1.0 / hr / hr;
        for (i = 0; i < 3; i++) ref_matrix_vec(diagonal_system, i, 1) = s[i];
        ref_matrix_eig(diagonal_system, 1) = 1.0 / hs / hs;
        for (i = 0; i < 3; i++) ref_matrix_vec(diagonal_system, i, 2) = n[i];
        ref_matrix_eig(diagonal_system, 2) = 1.0 / hn / hn;
        RSS(ref_matrix_form_m(diagonal_system, &(metric[6 * node])), "form m");
      } else {
        metric[0 + 6 * node] = 1.0 / hmax / hmax;
        metric[1 + 6 * node] = 0.0;
        metric[2 + 6 * node] = 0.0;
        metric[3 + 6 * node] = 1.0 / hmax / hmax;
        metric[4 + 6 * node] = 0.0;
        metric[5 + 6 * node] = 1.0 / hmax / hmax;
      }
    }
  }

  each_ref_geom_face(ref_geom, geom) {
    node = ref_geom_node(ref_geom, geom);
    if (ref_node_owned(ref_node, node)) {
      face = ref_geom_id(ref_geom, geom) - 1;
      RSS(ref_geom_radian_request(ref_geom, geom, &delta_radian), "drad");
      rlimit = hmax / delta_radian; /* h = r*drad, r = h/drad */
      if (ref_geom_model_loaded(ref_geom)) {
        RXS(ref_egads_face_curvature(ref_geom, geom, &kr, r, &ks, s),
            REF_FAILURE, "curve");
      } else if (ref_geom_meshlinked(ref_geom)) {
        RSS(ref_meshlink_face_curvature(ref_grid, geom, &kr, r, &ks, s),
            "curve");
      } else {
        continue;
      }
      /* ignore sign, curvature is 1 / radius */
      kr = ABS(kr);
      ks = ABS(ks);
      /* limit the aspect ratio of the metric by reducing the largest radius */
      kr = MAX(kr, curvature_ratio * ks);
      ks = MAX(ks, curvature_ratio * kr);
      hr = hmax;
      if (1.0 / rlimit < kr) hr = delta_radian / kr;
      hs = hmax;
      if (1.0 / rlimit < ks) hs = delta_radian / ks;

      if (ref_geom_model_loaded(ref_geom)) {
        REF_BOOL usable;
        RSS(ref_geom_usable(ref_geom, geom, &usable), "useable curvature");
        if (!usable) continue;
        RSS(ref_geom_reliability(ref_geom, geom, &slop), "edge tol");
      } else if (ref_geom_meshlinked(ref_geom)) {
        RSS(ref_meshlink_gap(ref_grid, node, &slop), "edge tol");
        slop *= ref_geom_gap_protection(ref_geom);
      } else {
        slop = 1.0e-5 * hmax;
      }
      if (hr < slop || hs < slop) continue;
      if (0.0 < ref_geom_face_min_length(ref_geom, face)) {
        if (hr < ref_geom_face_min_length(ref_geom, face) ||
            hs < ref_geom_face_min_length(ref_geom, face))
          continue;
      }

      hn = hmax;
      if (0.0 < ref_geom_face_initial_cell_height(ref_geom, face))
        hn = ref_geom_face_initial_cell_height(ref_geom, face);
      hn = MIN(hn, norm_ratio * hr);
      hn = MIN(hn, norm_ratio * hs);

      /* cross the tangent vectors to get the (inward or outward) normal */
      ref_math_cross_product(r, s, n);
      for (i = 0; i < 3; i++) ref_matrix_vec(diagonal_system, i, 0) = r[i];
      ref_matrix_eig(diagonal_system, 0) = 1.0 / hr / hr;
      for (i = 0; i < 3; i++) ref_matrix_vec(diagonal_system, i, 1) = s[i];
      ref_matrix_eig(diagonal_system, 1) = 1.0 / hs / hs;
      for (i = 0; i < 3; i++) ref_matrix_vec(diagonal_system, i, 2) = n[i];
      ref_matrix_eig(diagonal_system, 2) = 1.0 / hn / hn;
      /* form and intersect with previous */
      RSS(ref_matrix_form_m(diagonal_system, curvature_metric), "reform m");
      for (i = 0; i < 6; i++) previous_metric[i] = metric[i + 6 * node];
      RSS(ref_matrix_intersect(previous_metric, curvature_metric,
                               &(metric[6 * node])),
          "intersect to update metric");
    }
  }

  each_ref_geom_edge(ref_geom, geom) {
    node = ref_geom_node(ref_geom, geom);
    if (ref_node_owned(ref_node, node)) {
      RSS(ref_geom_radian_request(ref_geom, geom, &delta_radian), "drad");
      rlimit = hmax / delta_radian; /* h = r*drad, r = h/drad */
      if (ref_geom_model_loaded(ref_geom)) {
        RSS(ref_egads_edge_curvature(ref_geom, geom, &kr, r), "curve");
      } else if (ref_geom_meshlinked(ref_geom)) {
        RSS(ref_meshlink_edge_curvature(ref_grid, geom, &kr, r), "curve");
      } else {
        continue;
      }
      /* ignore sign, curvature is 1 / radius */
      kr = ABS(kr);
      hr = hmax;
      if (1.0 / rlimit < kr) hr = delta_radian / kr;

      if (ref_geom_model_loaded(ref_geom)) {
        RSS(ref_geom_reliability(ref_geom, geom, &slop), "edge tol");
      } else if (ref_geom_meshlinked(ref_geom)) {
        RSS(ref_meshlink_gap(ref_grid, node, &slop), "edge tol");
        slop *= ref_geom_gap_protection(ref_geom);
      } else {
        slop = 1.0e-5 * hmax;
      }
      if (hr < slop) continue;

      if (ref_geom_model_loaded(ref_geom)) {
        RSS(ref_geom_crease(ref_grid, node, &crease_dot_prod), "crease");
        if (crease_dot_prod < -0.8) {
          ramp = (-crease_dot_prod - 0.8) / 0.2;
          scale = 0.25 * ramp + 1.0 * (1.0 - ramp);
          hr *= scale;
        }
      }

      ref_matrix_vec(diagonal_system, 0, 0) = 1.0;
      ref_matrix_vec(diagonal_system, 1, 0) = 0.0;
      ref_matrix_vec(diagonal_system, 2, 0) = 0.0;
      ref_matrix_eig(diagonal_system, 0) = 1.0 / hr / hr;

      ref_matrix_vec(diagonal_system, 0, 1) = 0.0;
      ref_matrix_vec(diagonal_system, 1, 1) = 1.0;
      ref_matrix_vec(diagonal_system, 2, 1) = 0.0;
      ref_matrix_eig(diagonal_system, 1) = 1.0 / hr / hr;

      ref_matrix_vec(diagonal_system, 0, 2) = 0.0;
      ref_matrix_vec(diagonal_system, 1, 2) = 0.0;
      ref_matrix_vec(diagonal_system, 2, 2) = 1.0;
      ref_matrix_eig(diagonal_system, 2) = 1.0 / hr / hr;

      /* form and intersect with previous */
      RSS(ref_matrix_form_m(diagonal_system, curvature_metric), "reform m");
      for (i = 0; i < 6; i++) previous_metric[i] = metric[i + 6 * node];
      RSS(ref_matrix_intersect(previous_metric, curvature_metric,
                               &(metric[6 * node])),
          "intersect to update metric");
    }
  }

  if (ref_grid_twod(ref_grid)) {
    each_ref_node_valid_node(ref_node, node) {
      if (ref_node_owned(ref_node, node)) {
        RSS(ref_matrix_twod_m(&(metric[6 * node])), "enforce twod");
      }
    }
  }

  RSS(ref_node_ghost_dbl(ref_node, metric, 6), "update ghosts");

  return REF_SUCCESS;
}

static REF_STATUS add_sub_tet(REF_INT n0, REF_INT n1, REF_INT n2, REF_INT n3,
                              REF_INT *nodes, REF_DBL *metric,
                              REF_DBL *total_node_volume, REF_NODE ref_node,
                              REF_CELL ref_cell) {
  REF_INT tet_nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL m[6], log_m[6];
  REF_DBL tet_volume;
  REF_INT node, im;
  REF_STATUS status;
  tet_nodes[0] = nodes[(n0)];
  tet_nodes[1] = nodes[(n1)];
  tet_nodes[2] = nodes[(n2)];
  tet_nodes[3] = nodes[(n3)];
  RSS(ref_node_tet_vol(ref_node, tet_nodes, &tet_volume), "vol");
  status = ref_matrix_imply_m(m, ref_node_xyz_ptr(ref_node, tet_nodes[0]),
                              ref_node_xyz_ptr(ref_node, tet_nodes[1]),
                              ref_node_xyz_ptr(ref_node, tet_nodes[2]),
                              ref_node_xyz_ptr(ref_node, tet_nodes[3]));
  if (tet_volume > 0.0 && REF_SUCCESS == status) {
    RSS(ref_matrix_log_m(m, log_m), "log");
    for (node = 0; node < ref_cell_node_per(ref_cell); node++) {
      total_node_volume[nodes[node]] += tet_volume;
      for (im = 0; im < 6; im++)
        metric[im + 6 * nodes[node]] += tet_volume * log_m[im];
    }
  } else {
    /* silently skip singular contribution */
  }

  return REF_SUCCESS;
}

static REF_STATUS add_sub_tri(REF_INT n0, REF_INT n1, REF_INT n2,
                              REF_INT *nodes, REF_DBL *metric,
                              REF_DBL *total_node_volume, REF_NODE ref_node,
                              REF_CELL ref_cell) {
  REF_INT tri_nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL m[6], log_m[6];
  REF_DBL tri_area;
  REF_INT node, im;
  REF_STATUS status;
  tri_nodes[0] = nodes[(n0)];
  tri_nodes[1] = nodes[(n1)];
  tri_nodes[2] = nodes[(n2)];
  status = ref_matrix_imply_m3(m, ref_node_xyz_ptr(ref_node, tri_nodes[0]),
                               ref_node_xyz_ptr(ref_node, tri_nodes[1]),
                               ref_node_xyz_ptr(ref_node, tri_nodes[2]));
  if (REF_SUCCESS == status) {
    RSS(ref_matrix_log_m(m, log_m), "log");
    RSS(ref_node_tri_area(ref_node, tri_nodes, &tri_area), "area");
    for (node = 0; node < ref_cell_node_per(ref_cell); node++) {
      total_node_volume[nodes[node]] += tri_area;
      for (im = 0; im < 6; im++)
        metric[im + 6 * nodes[node]] += tri_area * log_m[im];
    }
  } else {
    /* silently skip singular contribution */
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_imply_from(REF_DBL *metric, REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_DBL m[6], log_m[6];
  REF_DBL *total_node_volume;
  REF_INT node, im;
  REF_INT cell;
  REF_CELL ref_cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];

  ref_malloc_init(total_node_volume, ref_node_max(ref_node), REF_DBL, 0.0);

  for (node = 0; node < ref_node_max(ref_node); node++)
    for (im = 0; im < 6; im++) metric[im + 6 * node] = 0.0;

  if (ref_grid_twod(ref_grid)) {
    ref_cell = ref_grid_tri(ref_grid);
    each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
      RSS(add_sub_tri(0, 1, 2, nodes, metric, total_node_volume, ref_node,
                      ref_cell),
          "tet sub tet");
    }
  }

  ref_cell = ref_grid_tet(ref_grid);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    RSS(add_sub_tet(0, 1, 2, 3, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "tet sub tet");
  }

  ref_cell = ref_grid_pri(ref_grid);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    RSS(add_sub_tet(0, 4, 5, 3, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "pri sub tet");
    RSS(add_sub_tet(0, 1, 5, 4, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "pri sub tet");
    RSS(add_sub_tet(0, 1, 2, 5, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "pri sub tet");
  }

  ref_cell = ref_grid_pyr(ref_grid);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    RSS(add_sub_tet(0, 4, 1, 2, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "pyr sub tet");
    RSS(add_sub_tet(0, 3, 4, 2, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "pyr sub tet");
  }

  ref_cell = ref_grid_hex(ref_grid);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    RSS(add_sub_tet(0, 5, 7, 4, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "hex sub tet");
    RSS(add_sub_tet(0, 1, 7, 5, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "hex sub tet");
    RSS(add_sub_tet(1, 6, 7, 5, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "hex sub tet");

    RSS(add_sub_tet(0, 7, 2, 3, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "hex sub tet");
    RSS(add_sub_tet(0, 7, 1, 2, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "hex sub tet");
    RSS(add_sub_tet(1, 7, 6, 2, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "hex sub tet");
  }

  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      RAS(0.0 < total_node_volume[node], "zero metric contributions");
      for (im = 0; im < 6; im++) {
        if (!ref_math_divisible(metric[im + 6 * node], total_node_volume[node]))
          RSS(REF_DIV_ZERO, "zero volume");
        log_m[im] = metric[im + 6 * node] / total_node_volume[node];
      }
      RSS(ref_matrix_exp_m(log_m, m), "exp");
      for (im = 0; im < 6; im++) metric[im + 6 * node] = m[im];
      total_node_volume[node] = 0.0;
    }
  }

  ref_free(total_node_volume);

  RSS(ref_node_ghost_dbl(ref_node, metric, 6), "update ghosts");

  return REF_SUCCESS;
}
REF_FCN REF_STATUS ref_metric_imply_non_tet(REF_DBL *metric,
                                            REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_DBL *total_node_volume;
  REF_DBL m[6], log_m[6];
  REF_INT node, im;
  REF_INT cell;
  REF_CELL ref_cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL *backup;

  ref_malloc(backup, 6 * ref_node_max(ref_node), REF_DBL);
  each_ref_node_valid_node(ref_node, node) {
    for (im = 0; im < 6; im++) backup[im + 6 * node] = metric[im + 6 * node];
  }

  ref_malloc_init(total_node_volume, ref_node_max(ref_node), REF_DBL, 0.0);

  each_ref_node_valid_node(ref_node, node) {
    if (ref_adj_valid(
            ref_adj_first(ref_cell_adj(ref_grid_pyr(ref_grid)), node)) ||
        ref_adj_valid(
            ref_adj_first(ref_cell_adj(ref_grid_pri(ref_grid)), node)) ||
        ref_adj_valid(
            ref_adj_first(ref_cell_adj(ref_grid_hex(ref_grid)), node))) {
      for (im = 0; im < 6; im++) {
        metric[im + 6 * node] = 0.0;
      }
    }
  }

  ref_cell = ref_grid_pri(ref_grid);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    RSS(add_sub_tet(0, 4, 5, 3, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "pri sub tet");
    RSS(add_sub_tet(0, 1, 5, 4, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "pri sub tet");
    RSS(add_sub_tet(0, 1, 2, 5, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "pri sub tet");
  }

  ref_cell = ref_grid_pyr(ref_grid);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    RSS(add_sub_tet(0, 4, 1, 2, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "pyr sub tet");
    RSS(add_sub_tet(0, 3, 4, 2, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "pyr sub tet");
  }

  /*
VI1 VI6 VI8 VI5  VI1 VI2 VI8 VI6  VI2 VI7 VI8 VI6
VI1 VI8 VI3 VI4  VI1 VI8 VI2 VI3  VI2 VI8 VI7 VI3
  */

  ref_cell = ref_grid_hex(ref_grid);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    RSS(add_sub_tet(0, 5, 7, 4, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "hex sub tet");
    RSS(add_sub_tet(0, 1, 7, 5, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "hex sub tet");
    RSS(add_sub_tet(1, 6, 7, 5, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "hex sub tet");

    RSS(add_sub_tet(0, 7, 2, 3, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "hex sub tet");
    RSS(add_sub_tet(0, 7, 1, 2, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "hex sub tet");
    RSS(add_sub_tet(1, 7, 6, 2, nodes, metric, total_node_volume, ref_node,
                    ref_cell),
        "hex sub tet");
  }

  each_ref_node_valid_node(ref_node, node) {
    if (ref_adj_valid(
            ref_adj_first(ref_cell_adj(ref_grid_pyr(ref_grid)), node)) ||
        ref_adj_valid(
            ref_adj_first(ref_cell_adj(ref_grid_pri(ref_grid)), node)) ||
        ref_adj_valid(
            ref_adj_first(ref_cell_adj(ref_grid_hex(ref_grid)), node))) {
      if (ref_node_owned(ref_node, node)) {
        if (0.0 < total_node_volume[node]) {
          for (im = 0; im < 6; im++) {
            if (!ref_math_divisible(metric[im + 6 * node],
                                    total_node_volume[node]))
              RSS(REF_DIV_ZERO, "zero volume");
            log_m[im] = metric[im + 6 * node] / total_node_volume[node];
          }
          RSS(ref_matrix_exp_m(log_m, m), "exp");
          for (im = 0; im < 6; im++) metric[im + 6 * node] = m[im];
          total_node_volume[node] = 0.0;
          for (im = 0; im < 6; im++)
            RAS(isfinite(metric[im + 6 * node]), "infer not finite");
        } else {
          for (im = 0; im < 6; im++)
            metric[im + 6 * node] = backup[im + 6 * node];
          for (im = 0; im < 6; im++)
            RAS(isfinite(metric[im + 6 * node]), "backup not finite");
        }
      }
    } else {
      for (im = 0; im < 6; im++)
        RAS(isfinite(metric[im + 6 * node]), "orig,tet not finite");
    }
  }

  ref_free(total_node_volume);
  ref_free(backup);

  RSS(ref_node_ghost_dbl(ref_node, metric, 6), "update ghosts");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_smr(REF_DBL *metric0, REF_DBL *metric1,
                                  REF_DBL *metric, REF_GRID ref_grid) {
  REF_INT node;
  REF_DBL metric_inv[6];
  REF_DBL inv_m1_m2[9];
  REF_DBL n_values[3], n_vectors[9], inv_n_vectors[9];
  REF_DBL diagonal_system[12];
  REF_DBL h0, h1, h, hmax, hmin, h2;
  REF_DBL eig;
  REF_INT i;

  each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
    RSS(ref_matrix_inv_m(&(metric0[6 * node]), metric_inv), "inv");
    RSS(ref_matrix_mult_m(metric_inv, &(metric1[6 * node]), inv_m1_m2), "mult");
    RSS(ref_matrix_diag_gen(3, inv_m1_m2, n_values, n_vectors), "gen eig");
    for (i = 0; i < 3; i++) {
      h0 = ref_matrix_sqrt_vt_m_v(&(metric0[6 * node]), &(n_vectors[i * 3]));
      if (!ref_math_divisible(1.0, h0)) RSS(REF_DIV_ZERO, "inf h0");
      h0 = 1.0 / h0;
      h1 = ref_matrix_sqrt_vt_m_v(&(metric1[6 * node]), &(n_vectors[i * 3]));
      if (!ref_math_divisible(1.0, h1)) RSS(REF_DIV_ZERO, "inf h1");
      h1 = 1.0 / h1;
      hmax = 4.00 * h0;
      hmin = 0.25 * h0;
      h = MIN(hmax, MAX(hmin, h1));
      h2 = h * h;
      if (!ref_math_divisible(1.0, h2)) RSS(REF_DIV_ZERO, "zero h^2");
      eig = 1.0 / h2;
      ref_matrix_eig(diagonal_system, i) = eig;
    }
    if (REF_SUCCESS != ref_matrix_inv_gen(3, n_vectors, inv_n_vectors)) {
      printf(" unable to invert eigenvectors:\n");
      printf(" %f %f %f\n", n_vectors[0], n_vectors[1], n_vectors[2]);
      printf(" %f %f %f\n", n_vectors[3], n_vectors[4], n_vectors[5]);
      printf(" %f %f %f\n", n_vectors[6], n_vectors[7], n_vectors[8]);
      RSS(REF_FAILURE, "gen eig");
    }
    RSS(ref_matrix_transpose_gen(3, inv_n_vectors, &(diagonal_system[3])),
        "gen eig");
    RSS(ref_matrix_form_m(diagonal_system, &(metric[6 * node])), "reform m");
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_complexity(REF_DBL *metric, REF_GRID ref_grid,
                                         REF_DBL *complexity) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT cell_node, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL volume, det;
  REF_BOOL have_tet;
  REF_LONG ntet;
  RSS(ref_cell_ncell(ref_grid_tet(ref_grid), ref_node, &ntet), "count");
  have_tet = (0 < ntet);
  if (have_tet) {
    ref_cell = ref_grid_tet(ref_grid);
  } else {
    ref_cell = ref_grid_tri(ref_grid);
  }
  *complexity = 0.0;
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (have_tet) {
      RSS(ref_node_tet_vol(ref_node, nodes, &volume), "vol");
    } else {
      RSS(ref_node_tri_area(ref_node, nodes, &volume), "area");
    }
    for (cell_node = 0; cell_node < ref_cell_node_per(ref_cell); cell_node++) {
      if (ref_node_owned(ref_node, nodes[cell_node])) {
        RSS(ref_matrix_det_m(&(metric[6 * nodes[cell_node]]), &det), "det");
        if (det > 0.0) {
          (*complexity) +=
              sqrt(det) * volume / ((REF_DBL)ref_cell_node_per(ref_cell));
        }
      }
    }
  }
  RSS(ref_mpi_allsum(ref_grid_mpi(ref_grid), complexity, 1, REF_DBL_TYPE),
      "dbl sum");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_set_complexity(REF_DBL *metric, REF_GRID ref_grid,
                                             REF_DBL target_complexity) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT i, node;
  REF_DBL current_complexity;
  REF_DBL complexity_scale;

  complexity_scale = 2.0 / 3.0;
  if (ref_grid_twod(ref_grid)) {
    complexity_scale = 1.0;
  }

  RSS(ref_metric_complexity(metric, ref_grid, &current_complexity), "cmp");
  if (!ref_math_divisible(target_complexity, current_complexity)) {
    return REF_DIV_ZERO;
  }
  each_ref_node_valid_node(ref_node, node) {
    for (i = 0; i < 6; i++) {
      metric[i + 6 * node] *=
          pow(target_complexity / current_complexity, complexity_scale);
    }
    if (ref_grid_twod(ref_grid)) {
      metric[2 + 6 * node] = 0.0;
      metric[4 + 6 * node] = 0.0;
      metric[5 + 6 * node] = 1.0;
    }
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_limit_aspect_ratio(REF_DBL *metric,
                                                 REF_GRID ref_grid,
                                                 REF_DBL aspect_ratio) {
  REF_DBL diag_system[12];
  REF_DBL max_eig, limit_eig;
  REF_INT node;
  if (ref_grid_twod(ref_grid)) {
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RSS(ref_matrix_diag_m(&(metric[6 * node]), diag_system), "eigen decomp");
      RSS(ref_matrix_descending_eig_twod(diag_system), "2D eig sort");
      max_eig = ref_matrix_eig(diag_system, 0);
      max_eig = MAX(ref_matrix_eig(diag_system, 1), max_eig);
      RAS(ref_math_divisible(max_eig, (aspect_ratio * aspect_ratio)),
          "AR div zero");
      limit_eig = max_eig / (aspect_ratio * aspect_ratio);
      ref_matrix_eig(diag_system, 0) =
          MAX(ref_matrix_eig(diag_system, 0), limit_eig);
      ref_matrix_eig(diag_system, 1) =
          MAX(ref_matrix_eig(diag_system, 1), limit_eig);
      RSS(ref_matrix_form_m(diag_system, &(metric[6 * node])), "reform m");
      RSS(ref_matrix_twod_m(&(metric[6 * node])), "enforce 2D m for roundoff");
    }
  } else {
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      RSS(ref_matrix_diag_m(&(metric[6 * node]), diag_system), "eigen decomp");
      max_eig = ref_matrix_eig(diag_system, 0);
      max_eig = MAX(ref_matrix_eig(diag_system, 1), max_eig);
      max_eig = MAX(ref_matrix_eig(diag_system, 2), max_eig);
      RAS(ref_math_divisible(max_eig, (aspect_ratio * aspect_ratio)),
          "AR div zero");
      limit_eig = max_eig / (aspect_ratio * aspect_ratio);
      ref_matrix_eig(diag_system, 0) =
          MAX(ref_matrix_eig(diag_system, 0), limit_eig);
      ref_matrix_eig(diag_system, 1) =
          MAX(ref_matrix_eig(diag_system, 1), limit_eig);
      ref_matrix_eig(diag_system, 2) =
          MAX(ref_matrix_eig(diag_system, 2), limit_eig);
      RSS(ref_matrix_form_m(diag_system, &(metric[6 * node])), "reform m");
    }
  }
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_limit_aspect_ratio_field(
    REF_DBL *metric, REF_GRID ref_grid, REF_DBL *aspect_ratio_field) {
  REF_DBL diag_system[12];
  REF_DBL max_eig, limit_eig;
  REF_INT node;
  REF_DBL aspect_ratio;
  if (ref_grid_twod(ref_grid)) {
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      aspect_ratio = aspect_ratio_field[node];
      RSS(ref_matrix_diag_m(&(metric[6 * node]), diag_system), "eigen decomp");
      RSS(ref_matrix_descending_eig_twod(diag_system), "2D eig sort");
      max_eig = ref_matrix_eig(diag_system, 0);
      max_eig = MAX(ref_matrix_eig(diag_system, 1), max_eig);
      if (ref_math_divisible(max_eig, (aspect_ratio * aspect_ratio))) {
        limit_eig = max_eig / (aspect_ratio * aspect_ratio);
        ref_matrix_eig(diag_system, 0) =
            MAX(ref_matrix_eig(diag_system, 0), limit_eig);
        ref_matrix_eig(diag_system, 1) =
            MAX(ref_matrix_eig(diag_system, 1), limit_eig);
        RSS(ref_matrix_form_m(diag_system, &(metric[6 * node])), "reform m");
        RSS(ref_matrix_twod_m(&(metric[6 * node])),
            "enforce 2D m for roundoff");
      }
    }
  } else {
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      aspect_ratio = aspect_ratio_field[node];
      RSS(ref_matrix_diag_m(&(metric[6 * node]), diag_system), "eigen decomp");
      max_eig = ref_matrix_eig(diag_system, 0);
      max_eig = MAX(ref_matrix_eig(diag_system, 1), max_eig);
      max_eig = MAX(ref_matrix_eig(diag_system, 2), max_eig);
      if (ref_math_divisible(max_eig, (aspect_ratio * aspect_ratio))) {
        limit_eig = max_eig / (aspect_ratio * aspect_ratio);
        ref_matrix_eig(diag_system, 0) =
            MAX(ref_matrix_eig(diag_system, 0), limit_eig);
        ref_matrix_eig(diag_system, 1) =
            MAX(ref_matrix_eig(diag_system, 1), limit_eig);
        ref_matrix_eig(diag_system, 2) =
            MAX(ref_matrix_eig(diag_system, 2), limit_eig);
        RSS(ref_matrix_form_m(diag_system, &(metric[6 * node])), "reform m");
      }
    }
  }
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_limit_h(REF_DBL *metric, REF_GRID ref_grid,
                                      REF_DBL hmin, REF_DBL hmax) {
  REF_DBL diag_system[12];
  REF_DBL eig;
  REF_INT node;
  each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
    RSS(ref_matrix_diag_m(&(metric[6 * node]), diag_system), "eigen decomp");
    if (hmin > 0.0) {
      eig = 1.0 / (hmin * hmin);
      ref_matrix_eig(diag_system, 0) = MIN(ref_matrix_eig(diag_system, 0), eig);
      ref_matrix_eig(diag_system, 1) = MIN(ref_matrix_eig(diag_system, 1), eig);
      ref_matrix_eig(diag_system, 2) = MIN(ref_matrix_eig(diag_system, 2), eig);
    }
    if (hmax > 0.0) {
      eig = 1.0 / (hmax * hmax);
      ref_matrix_eig(diag_system, 0) = MAX(ref_matrix_eig(diag_system, 0), eig);
      ref_matrix_eig(diag_system, 1) = MAX(ref_matrix_eig(diag_system, 1), eig);
      ref_matrix_eig(diag_system, 2) = MAX(ref_matrix_eig(diag_system, 2), eig);
    }
    RSS(ref_matrix_form_m(diag_system, &(metric[6 * node])), "reform m");
  }
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_limit_h_at_complexity(REF_DBL *metric,
                                                    REF_GRID ref_grid,
                                                    REF_DBL hmin, REF_DBL hmax,
                                                    REF_DBL target_complexity) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT i, node, relaxations;
  REF_DBL current_complexity;

  /* global scaling and h limits */
  for (relaxations = 0; relaxations < 10; relaxations++) {
    RSS(ref_metric_complexity(metric, ref_grid, &current_complexity), "cmp");
    if (!ref_math_divisible(target_complexity, current_complexity)) {
      return REF_DIV_ZERO;
    }
    each_ref_node_valid_node(ref_node, node) for (i = 0; i < 6; i++) {
      metric[i + 6 * node] *=
          pow(target_complexity / current_complexity, 2.0 / 3.0);
    }
    RSS(ref_metric_limit_h(metric, ref_grid, hmin, hmax), "limit h");
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_buffer(REF_DBL *metric, REF_GRID ref_grid) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_DBL diag_system[12];
  REF_DBL eig;
  REF_INT node;
  REF_DBL r, rmax, x, xmax, exponent, hmin, s, t, smin, smax, emin, emax;

  xmax = -1.0e-100;
  rmax = 0.0;
  each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
    r = sqrt(ref_node_xyz(ref_node, 0, node) * ref_node_xyz(ref_node, 0, node) +
             ref_node_xyz(ref_node, 1, node) * ref_node_xyz(ref_node, 1, node) +
             ref_node_xyz(ref_node, 2, node) * ref_node_xyz(ref_node, 2, node));
    rmax = MAX(rmax, r);
    xmax = MAX(xmax, ref_node_xyz(ref_node, 0, node));
  }
  r = rmax;
  RSS(ref_mpi_max(ref_mpi, &r, &rmax, REF_DBL_TYPE), "mpi max");
  RSS(ref_mpi_bcast(ref_mpi, &rmax, 1, REF_DBL_TYPE), "bcast");
  x = xmax;
  RSS(ref_mpi_max(ref_mpi, &x, &xmax, REF_DBL_TYPE), "mpi max");
  RSS(ref_mpi_bcast(ref_mpi, &xmax, 1, REF_DBL_TYPE), "bcast");

  each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
    RSS(ref_matrix_diag_m(&(metric[6 * node]), diag_system), "eigen decomp");

    r = sqrt(ref_node_xyz(ref_node, 0, node) * ref_node_xyz(ref_node, 0, node) +
             ref_node_xyz(ref_node, 1, node) * ref_node_xyz(ref_node, 1, node) +
             ref_node_xyz(ref_node, 2, node) * ref_node_xyz(ref_node, 2, node));

    smin = 0.5;
    smax = 0.9;
    emin = -4.0;
    emax = -1.0;

    s = MIN(1.0, r / xmax);

    t = MIN(s / smin, 1.0);
    exponent = -15.0 * (1.0 - t) + emin * t;

    if (smin < s && s < smax) {
      t = (s - smin) / (smax - smin);
      exponent = (emin) * (1.0 - t) + (emax) * (t);
    }

    if (smax <= s) {
      exponent = emax;
    }

    hmin = rmax * pow(10.0, exponent);

    if (ref_math_divisible(1.0, hmin * hmin)) {
      eig = 1.0 / (hmin * hmin);
      ref_matrix_eig(diag_system, 0) = MIN(ref_matrix_eig(diag_system, 0), eig);
      ref_matrix_eig(diag_system, 1) = MIN(ref_matrix_eig(diag_system, 1), eig);
      ref_matrix_eig(diag_system, 2) = MIN(ref_matrix_eig(diag_system, 2), eig);
    }

    RSS(ref_matrix_form_m(diag_system, &(metric[6 * node])), "reform m");
  }
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_buffer_at_complexity(REF_DBL *metric,
                                                   REF_GRID ref_grid,
                                                   REF_DBL target_complexity) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT i, node, relaxations;
  REF_DBL current_complexity;

  /* global scaling and buffer */
  for (relaxations = 0; relaxations < 10; relaxations++) {
    RSS(ref_metric_buffer(metric, ref_grid), "buffer");
    RSS(ref_metric_complexity(metric, ref_grid, &current_complexity), "cmp");
    if (!ref_math_divisible(target_complexity, current_complexity)) {
      return REF_DIV_ZERO;
    }
    each_ref_node_valid_node(ref_node, node) for (i = 0; i < 6; i++) {
      metric[i + 6 * node] *=
          pow(target_complexity / current_complexity, 2.0 / 3.0);
    }
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_lp(REF_DBL *metric, REF_GRID ref_grid,
                                 REF_DBL *scalar, REF_DBL *weight,
                                 REF_RECON_RECONSTRUCTION reconstruction,
                                 REF_INT p_norm, REF_DBL gradation,
                                 REF_DBL target_complexity) {
  RSS(ref_recon_hessian(ref_grid, scalar, metric, reconstruction), "recon");
  RSS(ref_recon_roundoff_limit(metric, ref_grid),
      "floor metric eigenvalues based on grid size and solution jitter");
  RSS(ref_metric_local_scale(metric, weight, ref_grid, p_norm),
      "local scale lp norm");
  RSS(ref_metric_gradation_at_complexity(metric, ref_grid, gradation,
                                         target_complexity),
      "gradation at complexity");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_lp_mixed(REF_DBL *metric, REF_GRID ref_grid,
                                       REF_DBL *scalar,
                                       REF_RECON_RECONSTRUCTION reconstruction,
                                       REF_INT p_norm, REF_DBL gradation,
                                       REF_DBL target_complexity) {
  RSS(ref_recon_hessian(ref_grid, scalar, metric, reconstruction), "recon");
  RSS(ref_recon_roundoff_limit(metric, ref_grid),
      "floor metric eigenvalues based on grid size and solution jitter");
  RSS(ref_metric_local_scale(metric, NULL, ref_grid, p_norm),
      "local scale lp norm");
  RSS(ref_metric_gradation_at_complexity_mixed(metric, ref_grid, gradation,
                                               target_complexity),
      "gradation at complexity");

  return REF_SUCCESS;
}

static REF_FCN REF_STATUS ref_metric_closest_d(REF_DBL *normal, REF_DBL *m,
                                               REF_DBL *d_out) {
  REF_INT i, first_i, second_i;
  REF_DBL dot, largest_dot, largest_eig;
  REF_DBL d_in[12];
  REF_DBL ratio;
  RSS(ref_matrix_diag_m(m, d_in), "decomp");
  /* metric eigenvector direction closest to normal */
  largest_dot = -2.0;
  first_i = REF_EMPTY;
  for (i = 0; i < 3; i++) {
    dot = ABS(ref_math_dot(normal, ref_matrix_vec_ptr(d_in, i)));
    if (dot > largest_dot) {
      largest_dot = dot;
      first_i = i;
    }
  }
  /* smallest spacing / largest eigenval direction remaining */
  RUS(REF_EMPTY, first_i, "first_i not set");
  second_i = REF_EMPTY;
  largest_eig = -1.0;
  for (i = 0; i < 3; i++) {
    if (i == first_i) continue;
    if (REF_EMPTY == second_i || ref_matrix_eig(d_in, i) > largest_eig) {
      second_i = i;
      largest_eig = ref_matrix_eig(d_in, i);
    }
  }
  RUS(REF_EMPTY, second_i, "second_i not set");
  RUS(second_i, first_i, "first_i same as second_i");
  /* first eigenvector in normal direction */
  for (i = 0; i < 3; i++) {
    ref_matrix_vec(d_out, i, 0) = normal[i];
  }
  RSS(ref_math_normalize(ref_matrix_vec_ptr(d_out, 0)), "first");
  /* second eigenvector, orthonormal to first eigenvector */
  for (i = 0; i < 3; i++) {
    ref_matrix_vec(d_out, i, 1) = ref_matrix_vec(d_in, i, second_i);
  }
  dot =
      ref_math_dot(ref_matrix_vec_ptr(d_out, 0), ref_matrix_vec_ptr(d_out, 1));
  for (i = 0; i < 3; i++) {
    ref_matrix_vec(d_out, i, 1) -= dot * ref_matrix_vec(d_in, i, 0);
  }
  RSS(ref_math_normalize(ref_matrix_vec_ptr(d_out, 1)), "second");
  ref_math_cross_product(ref_matrix_vec_ptr(d_out, 0),
                         ref_matrix_vec_ptr(d_out, 1),
                         ref_matrix_vec_ptr(d_out, 2));
  RSS(ref_math_normalize(ref_matrix_vec_ptr(d_out, 2)), "third");

  ratio = ref_matrix_sqrt_vt_m_v(m, ref_matrix_vec_ptr(d_out, 0));
  ref_matrix_eig(d_out, 0) = ratio * ratio;

  ratio = ref_matrix_sqrt_vt_m_v(m, ref_matrix_vec_ptr(d_out, 1));
  ref_matrix_eig(d_out, 1) = ratio * ratio;

  ratio = ref_matrix_sqrt_vt_m_v(m, ref_matrix_vec_ptr(d_out, 2));
  ref_matrix_eig(d_out, 2) = ratio * ratio;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_faceid_spacing(REF_DBL *metric, REF_GRID ref_grid,
                                             REF_INT faceid,
                                             REF_DBL set_normal_spacing,
                                             REF_DBL ceil_normal_spacing,
                                             REF_DBL tangential_aspect_ratio) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL *new_log_metric;
  REF_INT *hits;
  REF_INT node;
  REF_INT i;
  ref_malloc_init(hits, ref_node_max(ref_node), REF_INT, 0);
  ref_malloc_init(new_log_metric, 6 * ref_node_max(ref_node), REF_DBL, 0.0);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (faceid == nodes[ref_cell_id_index(ref_cell)]) {
      REF_DBL normal[3];
      REF_INT cell_node;
      RSS(ref_node_tri_normal(ref_node, nodes, normal), "normal area");
      RSS(ref_math_normalize(normal), "normalize");
      each_ref_cell_cell_node(ref_cell, cell_node) {
        REF_DBL d[12], m[6], logm[6];
        REF_DBL h;
        node = nodes[cell_node];
        RSS(ref_metric_closest_d(normal, &(metric[6 * node]), d), "closest");
        if (set_normal_spacing > 0.0) {
          h = set_normal_spacing;
          RAS(ref_math_divisible(1.0, h * h), "eig 0");
          ref_matrix_eig(d, 0) = 1.0 / (h * h);
        }
        if (ceil_normal_spacing > 0.0) {
          RAS(ref_math_divisible(1.0, sqrt(ref_matrix_eig(d, 0))), "eig 0");
          h = 1.0 / sqrt(ref_matrix_eig(d, 0));
          h = MIN(h, ceil_normal_spacing);
          RAS(ref_math_divisible(1.0, h * h), "eig 0");
          ref_matrix_eig(d, 0) = 1.0 / (h * h);
        }
        if (tangential_aspect_ratio > 1.0) {
          ref_matrix_eig(d, 2) =
              MAX(ref_matrix_eig(d, 2), ref_matrix_eig(d, 1) *
                                            tangential_aspect_ratio *
                                            tangential_aspect_ratio);
        }
        RSS(ref_matrix_form_m(d, m), "form");
        RSS(ref_matrix_log_m(m, logm), "form");
        for (i = 0; i < 6; i++) {
          new_log_metric[i + 6 * node] += logm[i];
        }
        hits[node] += 1;
      }
    }
  }

  RSS(ref_node_ghost_dbl(ref_node, new_log_metric, 6), "ghost metric");
  RSS(ref_node_ghost_int(ref_node, hits, 1), "ghost hits");

  each_ref_node_valid_node(ref_node, node) {
    if (hits[node] > 0) {
      for (i = 0; i < 6; i++) {
        new_log_metric[i + 6 * node] /= (REF_DBL)hits[node];
      }
      hits[node] = -1;
      RSS(ref_matrix_exp_m(&(new_log_metric[6 * node]), &(metric[6 * node])),
          "form");
    }
  }

  {
    REF_EDGE ref_edge;
    REF_INT edge, node0, node1;
    RSS(ref_edge_create(&ref_edge, ref_grid), "orig edges");
    for (edge = 0; edge < ref_edge_n(ref_edge); edge++) {
      node0 = ref_edge_e2n(ref_edge, 0, edge);
      node1 = ref_edge_e2n(ref_edge, 1, edge);
      if (0 <= hits[node0] && -1 == hits[node1]) {
        for (i = 0; i < 6; i++) {
          new_log_metric[i + 6 * node0] += new_log_metric[i + 6 * node1];
        }
        hits[node0] += 1;
      }
      if (0 <= hits[node1] && -1 == hits[node0]) {
        for (i = 0; i < 6; i++) {
          new_log_metric[i + 6 * node1] += new_log_metric[i + 6 * node0];
        }
        hits[node1] += 1;
      }
    }
    ref_edge_free(ref_edge);
  }

  RSS(ref_node_ghost_dbl(ref_node, new_log_metric, 6), "ghost metric");
  RSS(ref_node_ghost_int(ref_node, hits, 1), "ghost hits");

  each_ref_node_valid_node(ref_node, node) {
    if (hits[node] > 0) {
      for (i = 0; i < 6; i++) {
        new_log_metric[i + 6 * node] /= (REF_DBL)hits[node];
      }
      hits[node] = -2;
      RSS(ref_matrix_exp_m(&(new_log_metric[6 * node]), &(metric[6 * node])),
          "form");
    }
  }

  RSS(ref_node_ghost_dbl(ref_node, metric, 6), "ghost metric");

  ref_free(new_log_metric);
  ref_free(hits);
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_multigrad(REF_DBL *metric, REF_GRID ref_grid,
                                        REF_DBL *grad, REF_INT p_norm,
                                        REF_DBL gradation,
                                        REF_DBL target_complexity) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT i, node;
  REF_DBL *dsdx, *gradx, *grady, *gradz;
  ref_malloc_init(dsdx, ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(gradx, 3 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(grady, 3 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(gradz, 3 * ref_node_max(ref_node), REF_DBL, 0.0);
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
    metric[0 + 6 * node] = gradx[0 + 3 * node];
    metric[1 + 6 * node] = 0.5 * (gradx[1 + 3 * node] + grady[0 + 3 * node]);
    metric[2 + 6 * node] = 0.5 * (gradx[2 + 3 * node] + gradz[0 + 3 * node]);
    metric[3 + 6 * node] = grady[1 + 3 * node];
    metric[4 + 6 * node] = 0.5 * (grady[2 + 3 * node] + gradz[1 + 3 * node]);
    metric[5 + 6 * node] = gradz[2 + 3 * node];
  }

  ref_free(gradz);
  ref_free(grady);
  ref_free(gradx);
  ref_free(dsdx);

  RSS(ref_recon_abs_value_hessian(ref_grid, metric), "abs");

  RSS(ref_recon_roundoff_limit(metric, ref_grid),
      "floor metric eigenvalues based on grid size and solution jitter");
  RSS(ref_metric_local_scale(metric, NULL, ref_grid, p_norm),
      "local scale lp norm");
  RSS(ref_metric_gradation_at_complexity(metric, ref_grid, gradation,
                                         target_complexity),
      "gradation at complexity");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_moving_multiscale(
    REF_DBL *metric, REF_GRID ref_grid, REF_DBL *displaced, REF_DBL *scalar,
    REF_RECON_RECONSTRUCTION reconstruction, REF_INT p_norm, REF_DBL gradation,
    REF_DBL complexity) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_DBL *jac, *x, *grad, *hess, *xyz, det;
  REF_INT i, j, node;
  ref_malloc(hess, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
  ref_malloc(jac, 9 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
  ref_malloc(x, ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
  ref_malloc(grad, 3 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);

  for (j = 0; j < 3; j++) {
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      x[node] = displaced[j + 3 * node];
    }
    RSS(ref_recon_gradient(ref_grid, x, grad, reconstruction), "recon x");
    if (ref_grid_twod(ref_grid)) {
      each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
        grad[2 + 3 * node] = 1.0;
      }
    }
    each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
      for (i = 0; i < 3; i++) {
        jac[i + 3 * j + 9 * node] = grad[i + 3 * node];
      }
    }
  }
  ref_free(grad);
  ref_free(x);

  ref_malloc(xyz, 3 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
  each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
    for (i = 0; i < 3; i++) {
      xyz[i + 3 * node] = ref_node_xyz(ref_node, i, node);
      ref_node_xyz(ref_node, i, node) = displaced[i + 3 * node];
    }
  }
  RSS(ref_recon_hessian(ref_grid, scalar, hess, reconstruction), "recon");
  each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
    for (i = 0; i < 3; i++) {
      ref_node_xyz(ref_node, i, node) = xyz[i + 3 * node];
    }
  }
  ref_free(xyz);

  RSS(ref_recon_roundoff_limit(hess, ref_grid),
      "floor metric eigenvalues based on grid size and solution jitter");
  each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
    RSS(ref_matrix_jac_m_jact(&(jac[9 * node]), &(hess[6 * node]),
                              &(metric[6 * node])),
        "J M J^t");

    RSS(ref_matrix_det_gen(3, &(jac[9 * node]), &det), "gen det");
    for (i = 0; i < 6; i++) {
      metric[i + 6 * node] *= pow(ABS(det), 1.0 / (REF_DBL)p_norm);
    }
  }
  ref_free(jac);
  ref_free(hess);

  RSS(ref_metric_local_scale(metric, NULL, ref_grid, p_norm),
      "local scale lp norm");
  RSS(ref_metric_gradation_at_complexity(metric, ref_grid, gradation,
                                         complexity),
      "gradation at complexity");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_eig_bal(REF_DBL *metric, REF_GRID ref_grid,
                                      REF_DBL *scalar,
                                      REF_RECON_RECONSTRUCTION reconstruction,
                                      REF_INT p_norm, REF_DBL gradation,
                                      REF_DBL target_complexity) {
  RSS(ref_recon_hessian(ref_grid, scalar, metric, reconstruction), "recon");
  RSS(ref_recon_roundoff_limit(metric, ref_grid),
      "floor metric eigenvalues based on grid size and solution jitter");
  RSS(ref_metric_histogram(metric, ref_grid, "hess.tec"), "histogram");
  RSS(ref_metric_local_scale(metric, NULL, ref_grid, p_norm),
      "local scale lp norm");
  RSS(ref_metric_gradation_at_complexity(metric, ref_grid, gradation,
                                         target_complexity),
      "gradation at complexity");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_local_scale(REF_DBL *metric, REF_DBL *weight,
                                          REF_GRID ref_grid, REF_INT p_norm) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT i, node;
  REF_INT dimension;
  REF_DBL det, exponent;

  dimension = 3;
  if (ref_grid_twod(ref_grid)) {
    dimension = 2;
  }

  if (ref_grid_twod(ref_grid)) {
    each_ref_node_valid_node(ref_node, node) {
      metric[2 + 6 * node] = 0.0;
      metric[4 + 6 * node] = 0.0;
      metric[5 + 6 * node] = 1.0;
    }
  }

  /* local scaling */
  exponent = -1.0 / ((REF_DBL)(2 * p_norm + dimension));
  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_matrix_det_m(&(metric[6 * node]), &det), "det_m local hess scale");
    if (det > 0.0) {
      for (i = 0; i < 6; i++) metric[i + 6 * node] *= pow(det, exponent);
    }
  }

  if (ref_grid_twod(ref_grid)) {
    each_ref_node_valid_node(ref_node, node) {
      metric[2 + 6 * node] = 0.0;
      metric[4 + 6 * node] = 0.0;
      metric[5 + 6 * node] = 1.0;
    }
  }

  /* weight in now length scale, convert to eigenvalue */
  if (NULL != weight) {
    each_ref_node_valid_node(ref_node, node) {
      if (weight[node] > 0.0) {
        for (i = 0; i < 6; i++)
          metric[i + 6 * node] /= (weight[node] * weight[node]);
      }
    }
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_opt_goal(REF_DBL *metric, REF_GRID ref_grid,
                                       REF_INT nequations, REF_DBL *solution,
                                       REF_RECON_RECONSTRUCTION reconstruction,
                                       REF_INT p_norm, REF_DBL gradation,
                                       REF_DBL target_complexity) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT i, node;
  REF_INT ldim;
  REF_INT var, dir;

  ldim = 4 * nequations;

  each_ref_node_valid_node(ref_node, node) {
    for (i = 0; i < 6; i++) metric[i + 6 * node] = 0.0;
  }

  for (var = 0; var < nequations; var++) {
    REF_DBL *lam, *grad_lam, *flux, *hess_flux;
    ref_malloc_init(lam, ref_node_max(ref_node), REF_DBL, 0.0);
    ref_malloc_init(grad_lam, 3 * ref_node_max(ref_node), REF_DBL, 0.0);
    ref_malloc_init(flux, ref_node_max(ref_node), REF_DBL, 0.0);
    ref_malloc_init(hess_flux, 6 * ref_node_max(ref_node), REF_DBL, 0.0);
    each_ref_node_valid_node(ref_node, node) {
      lam[node] = solution[var + ldim * node];
    }
    RSS(ref_recon_gradient(ref_grid, lam, grad_lam, reconstruction),
        "grad_lam");

    for (dir = 0; dir < 3; dir++) {
      each_ref_node_valid_node(ref_node, node) {
        flux[node] = solution[var + nequations * (1 + dir) + ldim * node];
      }
      RSS(ref_recon_hessian(ref_grid, flux, hess_flux, reconstruction), "hess");
      each_ref_node_valid_node(ref_node, node) {
        for (i = 0; i < 6; i++)
          metric[i + 6 * node] +=
              ABS(grad_lam[dir + 3 * node]) * hess_flux[i + 6 * node];
      }
    }
    ref_free(hess_flux);
    ref_free(flux);
    ref_free(grad_lam);
    ref_free(lam);
  }
  if (ref_grid_twod(ref_grid)) {
    each_ref_node_valid_node(ref_node, node) {
      metric[2 + 6 * node] = 0.0;
      metric[4 + 6 * node] = 0.0;
      metric[5 + 6 * node] = 1.0;
    }
  }

  RSS(ref_recon_roundoff_limit(metric, ref_grid),
      "floor metric eigenvalues based on grid size and solution jitter");

  RSS(ref_metric_local_scale(metric, NULL, ref_grid, p_norm),
      "local scale lp norm");

  RSS(ref_metric_gradation_at_complexity(metric, ref_grid, gradation,
                                         target_complexity),
      "gradation at complexity");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_belme_gfe(
    REF_DBL *metric, REF_GRID ref_grid, REF_INT ldim, REF_DBL *prim_dual,
    REF_RECON_RECONSTRUCTION reconstruction) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT var, dir, node, i;
  REF_INT nequ;
  REF_DBL state[5], node_flux[5], direction[3];
  REF_DBL *lam, *grad_lam, *flux, *hess_flux;
  ref_malloc_init(lam, ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(grad_lam, 3 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(flux, ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(hess_flux, 6 * ref_node_max(ref_node), REF_DBL, 0.0);

  nequ = ldim / 2;

  for (var = 0; var < 5; var++) {
    each_ref_node_valid_node(ref_node, node) {
      lam[node] = prim_dual[var + 1 * nequ + ldim * node];
    }
    RSS(ref_recon_gradient(ref_grid, lam, grad_lam, reconstruction),
        "grad_lam");
    for (dir = 0; dir < 3; dir++) {
      each_ref_node_valid_node(ref_node, node) {
        direction[0] = 0.0;
        direction[1] = 0.0;
        direction[2] = 0.0;
        direction[dir] = 1.0;
        for (i = 0; i < 5; i++) {
          state[i] = prim_dual[i + 0 * nequ + ldim * node];
        }
        RSS(ref_phys_euler(state, direction, node_flux), "euler");
        flux[node] = node_flux[var];
      }
      RSS(ref_recon_hessian(ref_grid, flux, hess_flux, reconstruction), "hess");
      each_ref_node_valid_node(ref_node, node) {
        for (i = 0; i < 6; i++) {
          metric[i + 6 * node] +=
              ABS(grad_lam[dir + 3 * node]) * hess_flux[i + 6 * node];
        }
      }
    }
  }

  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_matrix_healthy_m(&(metric[6 * node])), "euler-opt-goal");
  }

  ref_free(hess_flux);
  ref_free(flux);
  ref_free(grad_lam);
  ref_free(lam);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_belme_gu(
    REF_DBL *metric, REF_GRID ref_grid, REF_INT ldim, REF_DBL *prim_dual,
    REF_DBL mach, REF_DBL re, REF_DBL reference_temp,
    REF_RECON_RECONSTRUCTION reconstruction) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT var, node, i, dir;
  REF_INT nequ;
  REF_DBL *lam, *hess_lam, *grad_lam, *sr_lam, *u, *hess_u, *grad_u;
  REF_DBL *omega;
  REF_DBL u1, u2, u3;
  REF_DBL w1, w2, w3;
  REF_DBL diag_system[12];
  REF_DBL weight;
  REF_DBL gamma = 1.4;
  REF_DBL t, mu;
  REF_DBL pr = 0.72;
  REF_DBL turbulent_pr = 0.90;
  REF_DBL thermal_conductivity;
  REF_DBL rho, turb, mu_t;

  nequ = ldim / 2;

  ref_malloc_init(lam, ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(hess_lam, 6 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(grad_lam, 3 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(sr_lam, 5 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(u, ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(hess_u, 6 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(grad_u, 3 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(omega, 9 * ref_node_max(ref_node), REF_DBL, 0.0);

  for (var = 0; var < 5; var++) {
    each_ref_node_valid_node(ref_node, node) {
      lam[node] = prim_dual[var + 1 * nequ + ldim * node];
    }
    RSS(ref_recon_hessian(ref_grid, lam, hess_lam, reconstruction), "hess_lam");
    each_ref_node_valid_node(ref_node, node) {
      RSS(ref_matrix_diag_m(&(hess_lam[6 * node]), diag_system), "decomp");
      sr_lam[var + 5 * node] = MAX(MAX(ABS(ref_matrix_eig(diag_system, 0)),
                                       ABS(ref_matrix_eig(diag_system, 1))),
                                   ABS(ref_matrix_eig(diag_system, 2)));
    }
  } /* SR MAX (eig, min(1e-30*max(det^-1/3)) for all lambda of var */

  var = 4;
  each_ref_node_valid_node(ref_node, node) {
    lam[node] = prim_dual[var + 1 * nequ + ldim * node];
  }
  RSS(ref_recon_gradient(ref_grid, lam, grad_lam, reconstruction), "grad_u");

  for (dir = 0; dir < 3; dir++) {
    var = 1 + dir;
    each_ref_node_valid_node(ref_node, node) {
      u[node] = prim_dual[var + 0 * nequ + ldim * node];
    }
    RSS(ref_recon_gradient(ref_grid, u, grad_u, reconstruction), "grad_u");
    each_ref_node_valid_node(ref_node, node) {
      ref_math_cross_product(&(grad_u[3 * node]), &(grad_lam[3 * node]),
                             &(omega[3 * dir + 9 * node]));
    }
  }

  var = 1;
  w1 = 20.0;
  w2 = 2.0;
  w3 = 2.0;
  each_ref_node_valid_node(ref_node, node) {
    u[node] = prim_dual[var + 0 * nequ + ldim * node];
  }
  RSS(ref_recon_hessian(ref_grid, u, hess_u, reconstruction), "hess_u");
  each_ref_node_valid_node(ref_node, node) {
    u1 = ABS(prim_dual[1 + ldim * node]);
    u2 = ABS(prim_dual[2 + ldim * node]);
    u3 = ABS(prim_dual[3 + ldim * node]);
    weight = 0.0;
    weight += w1 * sr_lam[1 + 5 * node];
    weight += w2 * sr_lam[2 + 5 * node];
    weight += w3 * sr_lam[3 + 5 * node];
    weight += (w1 * u1 + w2 * u2 + w3 * u3) * sr_lam[4 + 5 * node];
    weight += (5.0 / 3.0) *
              ABS(omega[1 + 2 * 3 + 9 * node] - omega[2 + 1 * 3 + 9 * node]);
    t = gamma * prim_dual[4 + ldim * node] / prim_dual[0 + ldim * node];
    RSS(viscosity_law(t, reference_temp, &mu), "sutherlands");
    if (6 == nequ) {
      rho = prim_dual[0 + ldim * node];
      turb = prim_dual[5 + ldim * node];
      RSS(ref_phys_mut_sa(turb, rho, mu / rho, &mu_t), "eddy viscosity");
      mu += mu_t;
    }
    weight *= mach / re * mu;
    RAS(weight >= 0.0, "negative weight u1");
    for (i = 0; i < 6; i++) {
      metric[i + 6 * node] += weight * hess_u[i + 6 * node];
    }
    RSS(ref_matrix_healthy_m(&(metric[6 * node])), "u1");
  }

  var = 2;
  w1 = 2.0;
  w2 = 20.0;
  w3 = 2.0;
  each_ref_node_valid_node(ref_node, node) {
    u[node] = prim_dual[var + ldim * node];
  }
  RSS(ref_recon_hessian(ref_grid, u, hess_u, reconstruction), "hess_u");
  each_ref_node_valid_node(ref_node, node) {
    u1 = ABS(prim_dual[1 + ldim * node]);
    u2 = ABS(prim_dual[2 + ldim * node]);
    u3 = ABS(prim_dual[3 + ldim * node]);
    weight = 0.0;
    weight += w1 * sr_lam[1 + 5 * node];
    weight += w2 * sr_lam[2 + 5 * node];
    weight += w3 * sr_lam[3 + 5 * node];
    weight += (w1 * u1 + w2 * u2 + w3 * u3) * sr_lam[4 + 5 * node];
    weight += (5.0 / 3.0) *
              ABS(omega[2 + 0 * 3 + 9 * node] - omega[0 + 2 * 3 + 9 * node]);
    t = gamma * prim_dual[4 + ldim * node] / prim_dual[0 + ldim * node];
    RSS(viscosity_law(t, reference_temp, &mu), "sutherlands");
    if (6 == nequ) {
      rho = prim_dual[0 + ldim * node];
      turb = prim_dual[5 + ldim * node];
      RSS(ref_phys_mut_sa(turb, rho, mu / rho, &mu_t), "eddy viscosity");
      mu += mu_t;
    }
    weight *= mach / re * mu;
    RAS(weight >= 0.0, "negative weight u2");
    for (i = 0; i < 6; i++) {
      metric[i + 6 * node] += weight * hess_u[i + 6 * node];
    }
    RSS(ref_matrix_healthy_m(&(metric[6 * node])), "u2");
  }

  var = 3;
  w1 = 2.0;
  w2 = 2.0;
  w3 = 20.0;
  each_ref_node_valid_node(ref_node, node) {
    u[node] = prim_dual[var + ldim * node];
  }
  RSS(ref_recon_hessian(ref_grid, u, hess_u, reconstruction), "hess_u");
  each_ref_node_valid_node(ref_node, node) {
    u1 = ABS(prim_dual[1 + ldim * node]);
    u2 = ABS(prim_dual[2 + ldim * node]);
    u3 = ABS(prim_dual[3 + ldim * node]);
    weight = 0.0;
    weight += w1 * sr_lam[1 + 5 * node];
    weight += w2 * sr_lam[2 + 5 * node];
    weight += w3 * sr_lam[3 + 5 * node];
    weight += (w1 * u1 + w2 * u2 + w3 * u3) * sr_lam[4 + 5 * node];
    weight += (5.0 / 3.0) *
              ABS(omega[0 + 1 * 3 + 9 * node] - omega[1 + 0 * 3 + 9 * node]);
    t = gamma * prim_dual[4 + ldim * node] / prim_dual[0 + ldim * node];
    RSS(viscosity_law(t, reference_temp, &mu), "sutherlands");
    if (6 == nequ) {
      rho = prim_dual[0 + ldim * node];
      turb = prim_dual[5 + ldim * node];
      RSS(ref_phys_mut_sa(turb, rho, mu / rho, &mu_t), "eddy viscosity");
      mu += mu_t;
    }
    weight *= mach / re * mu;
    RAS(weight >= 0.0, "negative weight u2");
    for (i = 0; i < 6; i++) {
      metric[i + 6 * node] += weight * hess_u[i + 6 * node];
    }
    RSS(ref_matrix_healthy_m(&(metric[6 * node])), "u3");
  }

  each_ref_node_valid_node(ref_node, node) {
    t = gamma * prim_dual[4 + ldim * node] / prim_dual[0 + ldim * node];
    u[node] = t;
  }
  RSS(ref_recon_hessian(ref_grid, u, hess_u, reconstruction), "hess_u");
  each_ref_node_valid_node(ref_node, node) {
    t = gamma * prim_dual[4 + ldim * node] / prim_dual[0 + ldim * node];
    RSS(viscosity_law(t, reference_temp, &mu), "sutherlands");
    thermal_conductivity = mu / (pr * (gamma - 1.0));
    if (6 == nequ) {
      rho = prim_dual[0 + ldim * node];
      turb = prim_dual[5 + ldim * node];
      RSS(ref_phys_mut_sa(turb, rho, mu / rho, &mu_t), "eddy viscosity");
      thermal_conductivity =
          (mu / (pr * (gamma - 1.0)) + mu_t / (turbulent_pr * (gamma - 1.0)));
      mu += mu_t;
    }
    for (i = 0; i < 6; i++) {
      metric[i + 6 * node] += 18.0 * mach / re * thermal_conductivity *
                              sr_lam[4 + 5 * node] * hess_u[i + 6 * node];
    }
  }

  ref_free(omega);
  ref_free(grad_u);
  ref_free(hess_u);
  ref_free(u);
  ref_free(sr_lam);
  ref_free(grad_lam);
  ref_free(hess_lam);
  ref_free(lam);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_cons_euler_g(
    REF_DBL *g, REF_GRID ref_grid, REF_INT ldim, REF_DBL *prim_dual,
    REF_RECON_RECONSTRUCTION reconstruction) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT var, dir, node, i;
  REF_INT nequ;
  REF_DBL state[5], dflux_dcons[25], direction[3];
  REF_DBL *lam, *grad_lam;
  REF_BOOL debug_export = REF_FALSE;

  nequ = ldim / 2;

  ref_malloc_init(lam, ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(grad_lam, 3 * ref_node_max(ref_node), REF_DBL, 0.0);

  for (var = 0; var < 5; var++) {
    each_ref_node_valid_node(ref_node, node) {
      lam[node] = prim_dual[var + 1 * nequ + ldim * node];
    }
    RSS(ref_recon_gradient(ref_grid, lam, grad_lam, reconstruction),
        "grad_lam");
    for (dir = 0; dir < 3; dir++) {
      each_ref_node_valid_node(ref_node, node) {
        direction[0] = 0.0;
        direction[1] = 0.0;
        direction[2] = 0.0;
        direction[dir] = 1.0;
        for (i = 0; i < 5; i++) {
          state[i] = prim_dual[i + 0 * nequ + ldim * node];
        }
        RSS(ref_phys_euler_jac(state, direction, dflux_dcons), "euler");
        for (i = 0; i < 5; i++) {
          g[i + 5 * node] +=
              dflux_dcons[var + i * 5] * grad_lam[dir + 3 * node];
        }
      }
    }

    if (debug_export) {
      char filename[20];
      sprintf(filename, "gradlam%d.tec", var);
      ref_gather_scalar_by_extension(ref_grid, 3, grad_lam, NULL, filename);
    }
  }
  ref_free(grad_lam);
  ref_free(lam);

  if (debug_export) {
    RSS(ref_gather_scalar_by_extension(ref_grid, 5, g, NULL, "g.tec"),
        "dump g");
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_cons_viscous_g(
    REF_DBL *g, REF_GRID ref_grid, REF_INT ldim, REF_DBL *prim_dual,
    REF_DBL mach, REF_DBL re, REF_DBL reference_temp,
    REF_RECON_RECONSTRUCTION reconstruction) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT var, node;
  REF_INT nequ;
  REF_DBL *lam, *rhou1star, *rhou2star, *rhou3star, *rhoestar;
  REF_DBL gamma = 1.4;
  REF_DBL t, mu, u1, u2, u3, q2, e;
  REF_DBL pr = 0.72;
  REF_DBL turbulent_pr = 0.90;
  REF_DBL thermal_conductivity;
  REF_DBL rho, turb, mu_t;
  REF_DBL frhou1, frhou2, frhou3, frhoe;
  REF_INT xx = 0, xy = 1, xz = 2, yy = 3, yz = 4, zz = 5;

  nequ = ldim / 2;

  ref_malloc_init(rhou1star, 6 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(rhou2star, 6 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(rhou3star, 6 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(rhoestar, 6 * ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(lam, ref_node_max(ref_node), REF_DBL, 0.0);

  var = 1;
  each_ref_node_valid_node(ref_node, node) {
    lam[node] = prim_dual[var + 1 * nequ + ldim * node];
  }
  RSS(ref_recon_signed_hessian(ref_grid, lam, rhou1star, reconstruction), "h1");
  var = 2;
  each_ref_node_valid_node(ref_node, node) {
    lam[node] = prim_dual[var + 1 * nequ + ldim * node];
  }
  RSS(ref_recon_signed_hessian(ref_grid, lam, rhou2star, reconstruction), "h2");
  var = 3;
  each_ref_node_valid_node(ref_node, node) {
    lam[node] = prim_dual[var + 1 * nequ + ldim * node];
  }
  RSS(ref_recon_signed_hessian(ref_grid, lam, rhou3star, reconstruction), "h3");
  var = 4;
  each_ref_node_valid_node(ref_node, node) {
    lam[node] = prim_dual[var + 1 * nequ + ldim * node];
  }
  RSS(ref_recon_signed_hessian(ref_grid, lam, rhoestar, reconstruction), "h4");
  ref_free(lam);

  each_ref_node_valid_node(ref_node, node) {
    rho = prim_dual[0 + ldim * node];
    u1 = prim_dual[1 + ldim * node];
    u2 = prim_dual[2 + ldim * node];
    u3 = prim_dual[3 + ldim * node];
    q2 = u1 * u1 + u2 * u2 + u3 * u3;
    e = prim_dual[4 + ldim * node] / (gamma - 1.0) + 0.5 * rho * q2;
    t = gamma * prim_dual[4 + ldim * node] / prim_dual[0 + ldim * node];
    RSS(viscosity_law(t, reference_temp, &mu), "sutherlands");
    thermal_conductivity = mu / (pr * (gamma - 1.0));
    if (6 == nequ) {
      turb = prim_dual[5 + ldim * node];
      RSS(ref_phys_mut_sa(turb, rho, mu / rho, &mu_t), "eddy viscosity");
      thermal_conductivity =
          (mu / (pr * (gamma - 1.0)) + mu_t / (turbulent_pr * (gamma - 1.0)));
      mu += mu_t;
    }
    mu *= mach / re;
    thermal_conductivity *= mach / re;

    frhou1 = 4.0 * rhou1star[xx + 6 * node] + 3.0 * rhou1star[yy + 6 * node] +
             3.0 * rhou1star[zz + 6 * node] + rhou2star[xy + 6 * node] +
             rhou3star[xz + 6 * node];
    frhou1 += 4.0 * u1 * rhoestar[xx + 6 * node] +
              3.0 * u1 * rhoestar[yy + 6 * node] +
              3.0 * u1 * rhoestar[zz + 6 * node] +
              u2 * rhoestar[xy + 6 * node] + u3 * rhoestar[xz + 6 * node];
    frhou1 *= (1.0 / 3.0) * mu / rho;

    frhou2 = rhou1star[xy + 6 * node] + 3.0 * rhou2star[xx + 6 * node] +
             4.0 * rhou2star[yy + 6 * node] + 3.0 * rhou2star[zz + 6 * node] +
             rhou3star[yz + 6 * node];
    frhou2 += u1 * rhoestar[xy + 6 * node] +
              3.0 * u2 * rhoestar[xx + 6 * node] +
              4.0 * u2 * rhoestar[yy + 6 * node] +
              3.0 * u2 * rhoestar[zz + 6 * node] + u3 * rhoestar[yz + 6 * node];
    frhou2 *= (1.0 / 3.0) * mu / rho;

    frhou3 = rhou1star[xz + 6 * node] + rhou2star[yz + 6 * node] +
             3.0 * rhou3star[xx + 6 * node] + 3.0 * rhou3star[yy + 6 * node] +
             4.0 * rhou3star[zz + 6 * node];
    frhou3 += u1 * rhoestar[xz + 6 * node] + u2 * rhoestar[yz + 6 * node] +
              3.0 * u3 * rhoestar[xx + 6 * node] +
              3.0 * u3 * rhoestar[yy + 6 * node] +
              4.0 * u3 * rhoestar[zz + 6 * node];
    frhou3 *= (1.0 / 3.0) * mu / rho;

    frhoe = rhoestar[xx + 6 * node] + rhoestar[yy + 6 * node] +
            rhoestar[zz + 6 * node];
    frhoe *= thermal_conductivity / rho;
    /* fun3d e has density in it */
    g[0 + 5 * node] +=
        -u1 * frhou1 - u2 * frhou2 - u3 * frhou3 + (q2 - e / rho) * frhoe;
    g[1 + 5 * node] += frhou1 - u1 * frhoe;
    g[2 + 5 * node] += frhou2 - u2 * frhoe;
    g[3 + 5 * node] += frhou3 - u3 * frhoe;
    g[4 + 5 * node] += frhoe;
  }

  ref_free(rhoestar);
  ref_free(rhou3star);
  ref_free(rhou2star);
  ref_free(rhou1star);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_cons_assembly(
    REF_DBL *metric, REF_DBL *g, REF_GRID ref_grid, REF_INT ldim,
    REF_DBL *prim_dual, REF_RECON_RECONSTRUCTION reconstruction) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT var, node, i;
  REF_DBL state[5], conserved[5];
  REF_DBL *cons, *hess_cons;

  ref_malloc_init(cons, ref_node_max(ref_node), REF_DBL, 0.0);
  ref_malloc_init(hess_cons, 6 * ref_node_max(ref_node), REF_DBL, 0.0);

  for (var = 0; var < 5; var++) {
    each_ref_node_valid_node(ref_node, node) {
      for (i = 0; i < 5; i++) {
        state[i] = prim_dual[i + ldim * node];
      }
      RSS(ref_phys_make_conserved(state, conserved), "prim2cons");
      cons[node] = conserved[var];
    }
    RSS(ref_recon_hessian(ref_grid, cons, hess_cons, reconstruction), "hess");
    each_ref_node_valid_node(ref_node, node) {
      for (i = 0; i < 6; i++) {
        metric[i + 6 * node] +=
            ABS(g[var + 5 * node]) * hess_cons[i + 6 * node];
        RAS(isfinite(ABS(g[var + 5 * node])), "g not finite");
        RAS(isfinite(hess_cons[i + 6 * node]), "hess not finite");
        RAS(isfinite(metric[i + 6 * node]), "metric not finite");
      }
    }
  }

  ref_free(hess_cons);
  ref_free(cons);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_histogram(REF_DBL *metric, REF_GRID ref_grid,
                                        const char *filename) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_DBL *eig, diag_system[12];
  REF_INT i, node, n, *sorted_index;
  FILE *file;

  ref_malloc_init(eig, ref_node_n(ref_node), REF_DBL, 0.0);
  ref_malloc_init(sorted_index, ref_node_n(ref_node), REF_INT, REF_EMPTY);
  n = 0;
  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_matrix_diag_m(&(metric[6 * node]), diag_system), "decomp");
    eig[n] = MAX(MAX(ABS(ref_matrix_eig(diag_system, 0)),
                     ABS(ref_matrix_eig(diag_system, 1))),
                 ABS(ref_matrix_eig(diag_system, 2)));
    n++;
  }
  REIS(ref_node_n(ref_node), n, "node count");
  RSS(ref_sort_heap_dbl(n, eig, sorted_index), "sort");

  file = fopen(filename, "w");
  if (NULL == (void *)file) printf("unable to open %s\n", filename);
  RNS(file, "unable to open file");

  fprintf(file, "title=\"tecplot ordered max eig\"\n");
  fprintf(file, "variables = \"i\" \"e\"\n");
  fprintf(file, "zone t=\"eig\"\n");
  for (i = 0; i < n; i++) {
    fprintf(file, "%d %e\n", i, eig[sorted_index[i]]);
  }
  fclose(file);
  ref_free(sorted_index);
  ref_free(eig);
  return REF_SUCCESS;
}

/*
 *  h2          ----
 *  h1     /----
 *  h0 ---/
 *     0  s1       s2
 */

REF_FCN REF_STATUS ref_metric_step_exp(REF_DBL s, REF_DBL *h, REF_DBL h0,
                                       REF_DBL h1, REF_DBL h2, REF_DBL s1,
                                       REF_DBL s2, REF_DBL width) {
  REF_DBL blend, x, e;
  blend = 0.5 * (1.0 + tanh((s - s1) / width));
  x = (s - s1) / (s2 - s1);
  e = h1 + (h2 - h1) * (exp(x) - 1.0) / (exp(1.0) - 1.0);
  *h = (1.0 - blend) * h0 + (blend)*e;
  /* printf("s %f blend %f x %f e %f h %f\n",s,blend,x,e,*h); */
  return REF_SUCCESS;
}

/*
@article{barbier-galin-fast-dist-cyl-cone-swept-sphere,
author = {Aurelien Barbier and Eric Galin},
title = {Fast Distance Computation Between a Point and
         Cylinders, Cones, Line-Swept Spheres and Cone-Spheres},
journal = {Journal of Graphics Tools},
volume = 9,
number = 2,
pages = {11--19},
year  = 2004,
publisher = {Taylor & Francis},
doi = {10.1080/10867651.2004.10504892}
} % https://liris.cnrs.fr/Documents/Liris-1297.pdf
*/
static void ref_metric_tattle_truncated_cone_dist(REF_DBL *cone_geom,
                                                  REF_DBL *p) {
  printf("p  %.18e %.18e %.18e\n", p[0], p[1], p[2]);
  printf("x1 %.18e %.18e %.18e\n", cone_geom[0], cone_geom[1], cone_geom[2]);
  printf("x2 %.18e %.18e %.18e\n", cone_geom[3], cone_geom[4], cone_geom[5]);
  printf("r1 %.18e r2 %.18e\n", cone_geom[6], cone_geom[7]);
}

REF_FCN REF_STATUS ref_metric_truncated_cone_dist(REF_DBL *cone_geom,
                                                  REF_DBL *p, REF_DBL *dist) {
  REF_INT d;
  REF_DBL a[3], b[3], ba[3], u[3], pa[3], l, x, y, y2, n, ra, rb, delta, s;
  REF_DBL xprime, yprime;
  REF_BOOL verbose = REF_FALSE;
  if (verbose) printf("\np %f %f %f\n", p[0], p[1], p[2]);
  if (ABS(cone_geom[6]) >= ABS(cone_geom[7])) {
    ra = ABS(cone_geom[6]);
    rb = ABS(cone_geom[7]);
    for (d = 0; d < 3; d++) {
      a[d] = cone_geom[d];
      b[d] = cone_geom[d + 3];
    }
    if (verbose) printf("forward ra %f rb %f\n", ra, rb);
  } else {
    ra = ABS(cone_geom[7]);
    rb = ABS(cone_geom[6]);
    for (d = 0; d < 3; d++) {
      a[d] = cone_geom[d + 3];
      b[d] = cone_geom[d];
    }
    if (verbose) printf("reverse ra %f rb %f\n", ra, rb);
  }
  if (verbose) printf("a %f %f %f\n", a[0], a[1], a[2]);
  if (verbose) printf("b %f %f %f\n", b[0], b[1], b[2]);
  for (d = 0; d < 3; d++) {
    ba[d] = b[d] - a[d];
    u[d] = ba[d];
    pa[d] = p[d] - a[d]; /* direction flip, error in paper? */
  }
  l = sqrt(ref_math_dot(ba, ba));
  if (!ref_math_divisible(u[0], l) || !ref_math_divisible(u[1], l) ||
      !ref_math_divisible(u[2], l)) { /* assume sphere */
    REF_DBL r;
    r = sqrt(pa[0] * pa[0] + pa[1] * pa[1] + pa[2] * pa[2]);
    *dist = MAX(0, r - MAX(ra, rb));
    return REF_SUCCESS;
  }
  RSB(ref_math_normalize(u), "axis length zero",
      { ref_metric_tattle_truncated_cone_dist(cone_geom, p); });
  x = ref_math_dot(pa, u); /* sign flip, error in paper? */
  n = sqrt(ref_math_dot(pa, pa));
  y2 = n * n - x * x;
  if (verbose) printf("n2 %f x2 %f y2 %f\n", n * n, x * x, y2);
  if (y2 <= 0) {
    y = 0;
  } else {
    y = sqrt(y2);
  }
  if (verbose) printf("x %f y %f l %f\n", x, y, l);
  if (x < 0) {
    if (y2 < ra * ra) {
      *dist = -x;
      return REF_SUCCESS;
    } else {
      *dist = sqrt((y - ra) * (y - ra) + x * x);
      return REF_SUCCESS;
    }
  }
  if (y2 < rb * rb) {
    if (x > l) {
      *dist = x - l;
      return REF_SUCCESS;
    } else {
      *dist = 0;
      return REF_SUCCESS;
    }
  }
  delta = ra - rb;
  s = sqrt(l * l + delta * delta);
  RAB(ref_math_divisible(delta, s) && ref_math_divisible(l, s),
      "div zero forming i and j",
      { ref_metric_tattle_truncated_cone_dist(cone_geom, p); });
  if (verbose) printf("l/s %f delta/s %f\n", l / s, delta / s);
  xprime = x * (l / s) - (y - ra) * (delta / s);
  yprime = x * (delta / s) + (y - ra) * (l / s);
  if (verbose) printf("xprime %f yprime %f\n", xprime, yprime);
  if (xprime <= 0) {
    *dist = sqrt((y - ra) * (y - ra) + x * x);
    return REF_SUCCESS;
  }
  if (xprime >= s) {
    *dist = sqrt(yprime * yprime + (xprime - s) * (xprime - s));
    return REF_SUCCESS;
  }
  *dist = MAX(0, yprime);
  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_metric_cart_box_dist(REF_DBL *box_geom,
                                                   REF_DBL *p, REF_DBL *dist) {
  REF_INT i;
  /* distance to box, zero inside box */
  *dist = 0;
  for (i = 0; i < 3; i++) {
    if (p[i] < box_geom[i]) (*dist) += pow(p[i] - box_geom[i], 2);
    if (p[i] > box_geom[i + 3]) (*dist) += pow(p[i] - box_geom[i + 3], 2);
  }
  (*dist) = sqrt(*dist);
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_parse(REF_DBL *metric, REF_GRID ref_grid,
                                    int narg, char *args[]) {
  REF_INT i, node, pos;
  REF_DBL diag_system[12];
  REF_DBL h0, decay_distance;
  REF_DBL geom[8];
  REF_DBL r, h;
  REF_BOOL ceil;

  pos = 0;
  while (pos < narg) {
    if (strncmp(args[pos], "--uniform", 9) != 0) {
      pos++;
    } else {
      pos++;
      if (pos < narg && strncmp(args[pos], "box", 3) == 0) {
        pos++;
        RAS(pos + 8 < narg,
            "not enough arguments for\n"
            "  --uniform box {ceil,floor} h0 decay_distance xmin ymin zmin "
            "xmax ymax zmax");
        RAS(strncmp(args[pos], "ceil", 4) == 0 ||
                strncmp(args[pos], "floor", 5) == 0,
            "ceil or floor is missing\n"
            "  --uniform box {ceil,floor} h0 decay_distance xmin ymin zmin "
            "xmax ymax zmax");
        ceil = (strncmp(args[pos], "ceil", 4) == 0);
        pos++;
        h0 = atof(args[pos]);
        pos++;
        decay_distance = atof(args[pos]);
        pos++;
        h0 = ABS(h0); /* metric must be semi-positive definite */
        for (i = 0; i < 6; i++) {
          geom[i] = atof(args[pos]);
          pos++;
        }

        if (ceil) {
          if (ref_mpi_once(ref_grid_mpi(ref_grid)))
            printf("--uniform box ceil h0=%f decay=%f [%f %f %f %f %f %f]\n",
                   h0, decay_distance, geom[0], geom[1], geom[2], geom[3],
                   geom[4], geom[5]);
        } else {
          if (ref_mpi_once(ref_grid_mpi(ref_grid)))
            printf("--uniform box floor h0=%f decay=%f [%f %f %f %f %f %f]\n",
                   h0, decay_distance, geom[0], geom[1], geom[2], geom[3],
                   geom[4], geom[5]);
        }
        each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
          RSS(ref_metric_cart_box_dist(
                  geom, ref_node_xyz_ptr(ref_grid_node(ref_grid), node), &r),
              "box dist");
          h = h0;
          if (ref_math_divisible(-r, decay_distance)) {
            h = h0 * pow(2, -r / decay_distance);
          }
          RSS(ref_matrix_diag_m(&(metric[6 * node]), diag_system), "decomp");
          if (ceil) {
            ref_matrix_eig(diag_system, 0) =
                MAX(1.0 / (h * h), ref_matrix_eig(diag_system, 0));
            ref_matrix_eig(diag_system, 1) =
                MAX(1.0 / (h * h), ref_matrix_eig(diag_system, 1));
            ref_matrix_eig(diag_system, 2) =
                MAX(1.0 / (h * h), ref_matrix_eig(diag_system, 2));
          } else {
            ref_matrix_eig(diag_system, 0) =
                MIN(1.0 / (h * h), ref_matrix_eig(diag_system, 0));
            ref_matrix_eig(diag_system, 1) =
                MIN(1.0 / (h * h), ref_matrix_eig(diag_system, 1));
            ref_matrix_eig(diag_system, 2) =
                MIN(1.0 / (h * h), ref_matrix_eig(diag_system, 2));
          }
          RSS(ref_matrix_form_m(diag_system, &(metric[6 * node])), "reform");
          if (ref_grid_twod(ref_grid)) {
            metric[2 + 6 * node] = 0.0;
            metric[4 + 6 * node] = 0.0;
            metric[5 + 6 * node] = 1.0;
          }
        }
        continue;
      }
      if (pos < narg && strncmp(args[pos], "cyl", 3) == 0) {
        pos++;
        RAS(pos + 10 < narg,
            "not enough arguments for\n"
            "  --uniform cyl {ceil,floor} h0 decay_distance x1 y1 z1 "
            "x2 y2 z2 r1 r2");
        RAS(strncmp(args[pos], "ceil", 4) == 0 ||
                strncmp(args[pos], "floor", 5) == 0,
            "ceil or floor is missing\n"
            "  --uniform cyl {ceil,floor} h0 decay_distance x1 y1 z1 "
            "x2 y2 z2 r1 r2");
        ceil = (strncmp(args[pos], "ceil", 4) == 0);
        pos++;
        h0 = atof(args[pos]);
        pos++;
        decay_distance = atof(args[pos]);
        pos++;
        h0 = ABS(h0); /* metric must be semi-positive definite */
        for (i = 0; i < 8; i++) {
          geom[i] = atof(args[pos]);
          pos++;
        }
        each_ref_node_valid_node(ref_grid_node(ref_grid), node) {
          RSS(ref_metric_truncated_cone_dist(
                  geom, ref_node_xyz_ptr(ref_grid_node(ref_grid), node), &r),
              "trunc cone dist");
          h = h0;
          if (ref_math_divisible(-r, decay_distance)) {
            h = h0 * pow(2, -r / decay_distance);
          }
          RSS(ref_matrix_diag_m(&(metric[6 * node]), diag_system), "decomp");
          if (ceil) {
            ref_matrix_eig(diag_system, 0) =
                MAX(1.0 / (h * h), ref_matrix_eig(diag_system, 0));
            ref_matrix_eig(diag_system, 1) =
                MAX(1.0 / (h * h), ref_matrix_eig(diag_system, 1));
            ref_matrix_eig(diag_system, 2) =
                MAX(1.0 / (h * h), ref_matrix_eig(diag_system, 2));
          } else {
            ref_matrix_eig(diag_system, 0) =
                MIN(1.0 / (h * h), ref_matrix_eig(diag_system, 0));
            ref_matrix_eig(diag_system, 1) =
                MIN(1.0 / (h * h), ref_matrix_eig(diag_system, 1));
            ref_matrix_eig(diag_system, 2) =
                MIN(1.0 / (h * h), ref_matrix_eig(diag_system, 2));
          }
          RSS(ref_matrix_form_m(diag_system, &(metric[6 * node])), "reform");
          if (ref_grid_twod(ref_grid)) {
            metric[2 + 6 * node] = 0.0;
            metric[4 + 6 * node] = 0.0;
            metric[5 + 6 * node] = 1.0;
          }
        }
        continue;
      }
    }
  }
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_isotropic(REF_DBL *metric, REF_GRID ref_grid,
                                        REF_DBL *hh) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node;
  REF_DBL *metric_imply;
  ref_malloc(metric_imply, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
  RSS(ref_metric_imply_from(metric_imply, ref_grid), "imply");

  each_ref_node_valid_node(ref_node, node) {
    REF_DBL e, d_imply[12], hmin, hmax, d_met[12], h;
    REF_DBL ar;

    RSS(ref_matrix_diag_m(&(metric_imply[6 * node]), d_imply), "eigen decomp");
    e = MAX(MAX(ref_matrix_eig(d_imply, 0), ref_matrix_eig(d_imply, 1)),
            ref_matrix_eig(d_imply, 1));
    RAS(e >= 0, "neg eig");
    RAS(ref_math_divisible(1.0, sqrt(e)), "div zero");
    hmin = 1.0 / sqrt(e);

    e = MIN(MIN(ref_matrix_eig(d_imply, 0), ref_matrix_eig(d_imply, 1)),
            ref_matrix_eig(d_imply, 1));
    RAS(e >= 0, "neg eig");
    RAS(ref_math_divisible(1.0, sqrt(e)), "div zero");
    hmax = 1.0 / sqrt(e);

    RAS(ref_math_divisible(hmin, hmax), "div zero");
    ar = hmin / hmax;

    RSS(ref_matrix_diag_m(&(metric[6 * node]), d_met), "eigen decomp");
    RSS(ref_matrix_descending_eig(d_met), "really descend?");
    if (ar < 0.5) {
      e = ref_matrix_eig(d_met, 1);
    } else {
      e = ref_matrix_eig(d_met, 0);
    }
    RAS(e >= 0, "neg eig");
    RAS(ref_math_divisible(1.0, sqrt(e)), "div zero");
    h = 1.0 / sqrt(e);

    hh[0 + 2 * node] = h;
    hh[1 + 2 * node] = 0.5; /* CADENCE/Pointwise typical decay rate */
  }

  RSS(ref_node_ghost_dbl(ref_node, hh, 2), "update ghosts");

  ref_free(metric_imply);

  return REF_SUCCESS;
}

static REF_INT nq = 20;
static REF_DBL tq[] = {
    -0.9931285991850949247861224, -0.9639719272779137912676661,
    -0.9122344282513259058677524, -0.8391169718222188233945291,
    -0.7463319064601507926143051, -0.6360536807265150254528367,
    -0.5108670019508270980043641, -0.3737060887154195606725482,
    -0.2277858511416450780804962, -0.0765265211334973337546404,
    0.0765265211334973337546404,  0.2277858511416450780804962,
    0.3737060887154195606725482,  0.5108670019508270980043641,
    0.6360536807265150254528367,  0.7463319064601507926143051,
    0.8391169718222188233945291,  0.9122344282513259058677524,
    0.9639719272779137912676661,  0.9931285991850949247861224};
static REF_DBL wq[] = {
    0.0176140071391521183118620, 0.0406014298003869413310400,
    0.0626720483341090635695065, 0.0832767415767047487247581,
    0.1019301198172404350367501, 0.1181945319615184173123774,
    0.1316886384491766268984945, 0.1420961093183820513292983,
    0.1491729864726037467878287, 0.152753387130725850698084,
    0.1527533871307258506980843, 0.1491729864726037467878287,
    0.1420961093183820513292983, 0.1316886384491766268984945,
    0.1181945319615184173123774, 0.1019301198172404350367501,
    0.0832767415767047487247581, 0.0626720483341090635695065,
    0.0406014298003869413310400, 0.0176140071391521183118620};
/*
static REF_INT nq = 10;
static REF_DBL tq[] = {-0.9739065285171717200779640,
                       -0.8650633666889845107320967,
                       -0.6794095682990244062343274,
                       -0.4333953941292471907992659,
                       -0.1488743389816312108848260,
                       0.1488743389816312108848260,
                       0.4333953941292471907992659,
                       0.6794095682990244062343274,
                       0.8650633666889845107320967,
                       0.9739065285171717200779640};
static REF_DBL wq[] = {0.0666713443086881375935688,
                       0.1494513491505805931457763,
                       0.2190863625159820439955349,
                       0.2692667193099963550912269,
                       0.2955242247147528701738930,
                       0.2955242247147528701738930,
                       0.2692667193099963550912269,
                       0.2190863625159820439955349,
                       0.1494513491505805931457763,
                       0.0666713443086881375935688};
*/
/*
static REF_INT nq = 5;
static REF_DBL tq[] = {-9.061798459386640e-01, -5.384693101056830e-01, 0.0,
                       5.384693101056830e-01, 9.061798459386640e-01};
static REF_DBL wq[] = {2.369268850561891e-01, 4.786286704993665e-01,
                       5.688888888888889e-01, 4.786286704993665e-01,
                       2.369268850561891e-01};
*/
/*
static REF_DBL tq[] = {-(1.0 / 3.0) * sqrt(5.0 + 2.0 * sqrt(10.0 / 7.0)),
                       -(1.0 / 3.0) * sqrt(5.0 - 2.0 * sqrt(10.0 / 7.0)), 0.0,
                       (1.0 / 3.0) * sqrt(5.0 - 2.0 * sqrt(10.0 / 7.0)),
                       (1.0 / 3.0) * sqrt(5.0 + 2.0 * sqrt(10.0 / 7.0))};
static REF_DBL wq[] = {(322.0 - 13.0 * sqrt(70)) / 900.0,
                       (322.0 + 13.0 * sqrt(70)) / 900.0, 128.0 / 225.0,
                       (322.0 + 13.0 * sqrt(70)) / 900.0,
                       (322.0 - 13.0 * sqrt(70)) / 900.0};
*/

REF_FCN REF_STATUS ref_metric_integrate(ref_metric_integrand integrand,
                                        void *state, REF_DBL *integral) {
  REF_INT i;
  REF_DBL t;
  REF_DBL value;
  *integral = 0.0;
  for (i = 0; i < nq; i++) {
    t = 0.5 * tq[i] + 0.5;
    RSS(integrand(state, t, &value), "eval");
    *integral += 0.5 * wq[i] * value;
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_integrand_err2(void *void_m_diag_sys_hess,
                                             REF_DBL theta_over_2pi,
                                             REF_DBL *radial_error) {
  REF_DBL *m_diag_sys_hess = (REF_DBL *)void_m_diag_sys_hess;
  REF_DBL theta = 2.0 * ref_math_pi * theta_over_2pi;
  REF_DBL xx, yy;
  REF_DBL h0, h1;
  REF_DBL xyz[3];
  REF_DBL *hess;
  REF_DBL err, r, derr_dr2;
  xx = cos(theta);
  yy = sin(theta);
  h0 = 1.0 / sqrt(m_diag_sys_hess[0]);
  h1 = 1.0 / sqrt(m_diag_sys_hess[1]);
  xyz[0] = h0 * m_diag_sys_hess[3] * xx + h1 * m_diag_sys_hess[6] * yy;
  xyz[1] = h0 * m_diag_sys_hess[4] * xx + h1 * m_diag_sys_hess[7] * yy;
  xyz[2] = 0.0;
  hess = &(m_diag_sys_hess[12]);
  err = ref_matrix_vt_m_v(hess, xyz);
  r = sqrt(ref_math_dot(xyz, xyz));
  /* assume error = derr_dr2 * r * r */
  derr_dr2 = err / r / r;
  /* integrate 2 * pi * r * error or 2 * pi * r^4/4 * derr_dr2 */
  *radial_error = 2 * ref_math_pi * derr_dr2 * pow(r, 4) / 4.0;
  /* printf("theta %f r %f derr_dr2 %f\n",theta,r,derr_dr2); */
  return REF_SUCCESS;
}

/*
static REF_INT nq2 = 7;
static REF_DBL baryq2[7][3] = {
    {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0},
    {0.797426985353087, 0.101286507323456, 0.101286507323456},
    {0.101286507323456, 0.797426985353087, 0.101286507323456},
    {0.101286507323456, 0.101286507323456, 0.797426985353087},
    {0.470142064105115, 0.470142064105115, 0.059715871789770},
    {0.470142064105115, 0.059715871789770, 0.470142064105115},
    {0.059715871789770, 0.470142064105115, 0.470142064105115}};
static REF_DBL weightq2[7] = {
    0.225000000000000, 0.125939180544827, 0.125939180544827, 0.125939180544827,
    0.132394152785506, 0.132394152785506, 0.132394152785506};
*/
static REF_INT nq2 = 25;
static REF_DBL baryq2[25][3] = {
    {0.3333333333333333, 0.3333333333333333, 0.3333333333333334},
    {0.4272731788467755, 0.1454536423064490, 0.4272731788467755},
    {0.1454536423064490, 0.4272731788467755, 0.4272731788467755},
    {0.4272731788467755, 0.4272731788467755, 0.1454536423064490},
    {0.1830992224486750, 0.6338015551026499, 0.1830992224486750},
    {0.6338015551026499, 0.1830992224486750, 0.1830992224486751},
    {0.1830992224486750, 0.1830992224486750, 0.6338015551026499},
    {0.4904340197011306, 0.0191319605977388, 0.4904340197011307},
    {0.0191319605977388, 0.4904340197011306, 0.4904340197011306},
    {0.4904340197011306, 0.4904340197011306, 0.0191319605977389},
    {0.0125724455515805, 0.9748551088968389, 0.0125724455515805},
    {0.9748551088968389, 0.0125724455515805, 0.0125724455515806},
    {0.0125724455515805, 0.0125724455515805, 0.9748551088968389},
    {0.3080460016852477, 0.0376853303946862, 0.6542686679200660},
    {0.0376853303946862, 0.3080460016852477, 0.6542686679200660},
    {0.6542686679200661, 0.0376853303946862, 0.3080460016852477},
    {0.0376853303946862, 0.6542686679200661, 0.3080460016852477},
    {0.6542686679200661, 0.3080460016852477, 0.0376853303946862},
    {0.3080460016852477, 0.6542686679200661, 0.0376853303946861},
    {0.0333718337393048, 0.8438235891921360, 0.1228045770685593},
    {0.8438235891921360, 0.0333718337393048, 0.1228045770685593},
    {0.1228045770685593, 0.8438235891921360, 0.0333718337393047},
    {0.8438235891921360, 0.1228045770685593, 0.0333718337393048},
    {0.1228045770685593, 0.0333718337393048, 0.8438235891921360},
    {0.0333718337393048, 0.1228045770685593, 0.8438235891921360}};
static REF_DBL weightq2[25] = {
    0.0809374287976229, 0.0772985880029631, 0.0772985880029631,
    0.0772985880029631, 0.0784576386123717, 0.0784576386123717,
    0.0784576386123717, 0.0174691679959295, 0.0174691679959295,
    0.0174691679959295, 0.0042923741848328, 0.0042923741848328,
    0.0042923741848328, 0.0374688582104676, 0.0374688582104676,
    0.0374688582104676, 0.0374688582104676, 0.0374688582104676,
    0.0374688582104676, 0.0269493525918800, 0.0269493525918800,
    0.0269493525918800, 0.0269493525918800, 0.0269493525918800,
    0.0269493525918800};

REF_FCN REF_STATUS ref_metric_integrate2(ref_metric_integrand2 integrand,
                                         void *state, REF_DBL *integral) {
  REF_INT i;
  REF_DBL value;
  *integral = 0.0;
  for (i = 0; i < nq2; i++) {
    RSS(integrand(state, baryq2[i], &value), "eval");
    *integral += weightq2[i] * value;
  }

  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_metric_integrand_quad_err2(void *void_node_area,
                                                         REF_DBL *bary,
                                                         REF_DBL *error) {
  REF_DBL *node_area = (REF_DBL *)void_node_area;
  REF_DBL shape[REF_CELL_MAX_SIZE_PER];
  REF_INT i;
  REF_DBL quadratic, linear;
  REF_INT p = 1;

  RSS(ref_cell_shape(REF_CELL_TR3, bary, shape), "shape");

  quadratic = 0.0;
  for (i = 0; i < 6; i++) {
    quadratic += shape[i] * node_area[i];
  }
  linear = 0.0;
  for (i = 0; i < 3; i++) {
    linear += bary[i] * node_area[i];
  }
  *error = pow(ABS(quadratic - linear), p) * node_area[6];
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_metric_interpolation_error2(REF_GRID ref_grid,
                                                   REF_DBL *scalar) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tr2(ref_grid);
  REF_INT cell, cell_node;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL node_area[7];
  REF_DBL integral;
  REF_DBL error;
  REF_DBL *dist;

  ref_malloc_init(dist, ref_node_max(ref_node), REF_DBL, 0.0);
  error = 0.0;
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    each_ref_cell_cell_node(ref_cell, cell_node) {
      node_area[cell_node] = scalar[nodes[cell_node]];
    }
    RSS(ref_node_tri_area(ref_node, nodes, &(node_area[6])), "area");
    RSS(ref_metric_integrate2(ref_metric_integrand_quad_err2, node_area,
                              &integral),
        "intg");
    error += integral;
    {
      REF_CELL tri_cell = ref_grid_tri(ref_grid);
      REF_INT tri_nodes[REF_CELL_MAX_SIZE_PER], new_cell;
      tri_nodes[0] = nodes[0];
      tri_nodes[1] = nodes[1];
      tri_nodes[2] = nodes[2];
      tri_nodes[3] = nodes[6];
      RSS(ref_cell_add(tri_cell, tri_nodes, &new_cell), "add tri");
      dist[tri_nodes[0]] += integral / 3.0;
      dist[tri_nodes[1]] += integral / 3.0;
      dist[tri_nodes[2]] += integral / 3.0;
    }
  }
  printf("interpolation error %e\n", error);
  RSS(ref_gather_scalar_by_extension(ref_grid, 1, dist, NULL,
                                     "ref_metric_test_dist.plt"),
      "dump");

  return REF_SUCCESS;
}
