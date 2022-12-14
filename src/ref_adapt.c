
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

#include "ref_adapt.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ref_cavity.h"
#include "ref_collapse.h"
#include "ref_edge.h"
#include "ref_gather.h"
#include "ref_histogram.h"
#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_matrix.h"
#include "ref_metric.h"
#include "ref_mpi.h"
#include "ref_node.h"
#include "ref_smooth.h"
#include "ref_split.h"
#include "ref_swap.h"
#include "ref_validation.h"

REF_FCN REF_STATUS ref_adapt_create(REF_ADAPT *ref_adapt_ptr) {
  REF_ADAPT ref_adapt;

  ref_malloc(*ref_adapt_ptr, 1, REF_ADAPT_STRUCT);

  ref_adapt = *ref_adapt_ptr;

  ref_adapt->split_ratio_growth = REF_FALSE;
  ref_adapt->split_ratio = sqrt(2.0);
  ref_adapt->split_quality_absolute = 1.0e-3;
  ref_adapt->split_quality_relative = 0.1;

  ref_adapt->collapse_ratio = 1.0 / sqrt(2.0);
  ref_adapt->collapse_quality_absolute = 1.0e-3;

  ref_adapt->smooth_min_quality = 1.0e-3;
  ref_adapt->smooth_pliant_alpha = 0.2;

  ref_adapt->swap_max_degree = 10000;
  ref_adapt->swap_min_quality = 0.5;

  ref_adapt->post_min_normdev = 0.0;
  ref_adapt->post_min_ratio = 1.0e-3;
  ref_adapt->post_max_ratio = 3.0;

  ref_adapt->last_min_ratio = 0.5e-3;
  ref_adapt->last_max_ratio = 6.0;

  ref_adapt->unlock_tet = REF_FALSE;

  ref_adapt->timing_level = 0;
  ref_adapt->watch_param = REF_FALSE;
  ref_adapt->watch_topo = REF_FALSE;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_adapt_deep_copy(REF_ADAPT *ref_adapt_ptr,
                                       REF_ADAPT original) {
  REF_ADAPT ref_adapt;

  ref_malloc(*ref_adapt_ptr, 1, REF_ADAPT_STRUCT);

  ref_adapt = *ref_adapt_ptr;

  ref_adapt->split_ratio_growth = original->split_ratio_growth;
  ref_adapt->split_ratio = original->split_ratio;
  ref_adapt->split_quality_absolute = original->split_quality_absolute;
  ref_adapt->split_quality_relative = original->split_quality_relative;

  ref_adapt->collapse_ratio = original->collapse_ratio;
  ref_adapt->collapse_quality_absolute = original->collapse_quality_absolute;

  ref_adapt->smooth_min_quality = original->smooth_min_quality;
  ref_adapt->smooth_pliant_alpha = original->smooth_pliant_alpha;

  ref_adapt->swap_max_degree = original->swap_max_degree;
  ref_adapt->swap_min_quality = original->swap_min_quality;

  ref_adapt->post_min_normdev = original->post_min_normdev;
  ref_adapt->post_min_ratio = original->post_min_ratio;
  ref_adapt->post_max_ratio = original->post_max_ratio;

  ref_adapt->last_min_ratio = original->last_min_ratio;
  ref_adapt->last_max_ratio = original->last_max_ratio;

  ref_adapt->unlock_tet = original->unlock_tet;

  ref_adapt->timing_level = original->timing_level;
  ref_adapt->watch_param = original->watch_param;
  ref_adapt->watch_topo = original->watch_topo;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_adapt_free(REF_ADAPT ref_adapt) {
  ref_free(ref_adapt);

  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_adapt_parameter(REF_GRID ref_grid,
                                              REF_BOOL *all_done) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_ADAPT ref_adapt = ref_grid->adapt;
  REF_CELL ref_cell;
  REF_INT cell;
  REF_LONG ncell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL det, max_det, complexity, min_metric_vol;
  REF_DBL quality, min_quality;
  REF_DBL normdev, min_normdev;
  REF_DBL volume, min_volume, max_volume;
  REF_DBL target_quality, target_normdev;
  REF_INT cell_node;
  REF_INT node, nnode;
  REF_DBL nodes_per_complexity;
  REF_INT degree, max_degree;
  REF_DBL ratio, min_ratio, max_ratio;
  REF_INT edge, part;
  REF_INT age, max_age;
  REF_EDGE ref_edge;
  REF_DBL m[6];
  REF_INT int_mixed, local_mixed;
  REF_BOOL mixed;

  int_mixed = 0;
  if (ref_cell_n(ref_grid_qua(ref_grid)) > 0 ||
      ref_cell_n(ref_grid_pyr(ref_grid)) > 0 ||
      ref_cell_n(ref_grid_pri(ref_grid)) > 0 ||
      ref_cell_n(ref_grid_hex(ref_grid)) > 0)
    int_mixed = 1;
  local_mixed = int_mixed;
  RSS(ref_mpi_max(ref_mpi, &local_mixed, &int_mixed, REF_INT_TYPE), "mpi max");
  mixed = (int_mixed > 0);

  if (ref_grid_twod(ref_grid) || ref_grid_surf(ref_grid)) {
    ref_cell = ref_grid_tri(ref_grid);
  } else {
    ref_cell = ref_grid_tet(ref_grid);
  }

  min_quality = 1.0;
  min_volume = REF_DBL_MAX;
  max_volume = REF_DBL_MIN;
  max_det = -1.0;
  complexity = 0.0;
  ncell = 0;
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (ref_grid_twod(ref_grid) || ref_grid_surf(ref_grid)) {
      RSS(ref_node_tri_quality(ref_grid_node(ref_grid), nodes, &quality),
          "qual");
      RSS(ref_node_tri_area(ref_grid_node(ref_grid), nodes, &volume), "vol");
    } else {
      RSS(ref_node_tet_quality(ref_grid_node(ref_grid), nodes, &quality),
          "qual");
      RSS(ref_node_tet_vol(ref_grid_node(ref_grid), nodes, &volume), "vol");
    }
    min_quality = MIN(min_quality, quality);
    min_volume = MIN(min_volume, volume);
    max_volume = MAX(max_volume, volume);

    for (cell_node = 0; cell_node < ref_cell_node_per(ref_cell); cell_node++) {
      if (ref_node_owned(ref_node, nodes[cell_node])) {
        RSS(ref_node_metric_get(ref_node, nodes[cell_node], m), "get");
        RSS(ref_matrix_det_m(m, &det), "det");
        max_det = MAX(max_det, det);
        if (ref_grid_surf(ref_grid)) {
          REF_DBL area, normal[3], normal_projection;
          RSS(ref_node_tri_area(ref_grid_node(ref_grid), nodes, &area), "vol");
          RSS(ref_node_tri_normal(ref_grid_node(ref_grid), nodes, normal),
              "norm");

          if (REF_SUCCESS == ref_math_normalize(normal)) {
            normal_projection = ref_matrix_vt_m_v(m, normal);
            if (ref_math_divisible(det, normal_projection)) {
              if (det / normal_projection > 0.0) {
                complexity += sqrt(det / normal_projection) * area /
                              ((REF_DBL)ref_cell_node_per(ref_cell));
              }
            }
          }
        } else {
          if (det > 0.0) {
            complexity +=
                sqrt(det) * volume / ((REF_DBL)ref_cell_node_per(ref_cell));
          }
        }
      }
    }
    RSS(ref_cell_part(ref_cell, ref_node, cell, &part), "owner");
    if (part == ref_mpi_rank(ref_mpi)) ncell++;
  }
  quality = min_quality;
  RSS(ref_mpi_min(ref_mpi, &quality, &min_quality, REF_DBL_TYPE), "mpi min");
  RSS(ref_mpi_bcast(ref_mpi, &quality, 1, REF_DBL_TYPE), "mbast");
  volume = min_volume;
  RSS(ref_mpi_min(ref_mpi, &volume, &min_volume, REF_DBL_TYPE), "mpi min");
  RSS(ref_mpi_bcast(ref_mpi, &min_volume, 1, REF_DBL_TYPE), "bcast");
  volume = max_volume;
  RSS(ref_mpi_max(ref_mpi, &volume, &max_volume, REF_DBL_TYPE), "mpi max");
  RSS(ref_mpi_bcast(ref_mpi, &max_volume, 1, REF_DBL_TYPE), "bcast");
  det = max_det;
  RSS(ref_mpi_max(ref_mpi, &det, &max_det, REF_DBL_TYPE), "mpi max");
  RSS(ref_mpi_bcast(ref_mpi, &max_det, 1, REF_DBL_TYPE), "bcast");
  RAS(ref_math_divisible(1.0, sqrt(max_det)), "can not invert sqrt(max_det)");
  min_metric_vol = 1.0 / sqrt(max_det);

  RSS(ref_mpi_allsum(ref_mpi, &complexity, 1, REF_DBL_TYPE), "dbl sum");
  RSS(ref_mpi_allsum(ref_mpi, &ncell, 1, REF_LONG_TYPE), "cell int sum");

  nnode = 0;
  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      nnode++;
    }
  }
  RSS(ref_mpi_allsum(ref_mpi, &nnode, 1, REF_INT_TYPE), "int sum");

  nodes_per_complexity = (REF_DBL)nnode / complexity;

  max_degree = 0;
  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_adj_degree(ref_cell_adj(ref_cell), node, &degree), "cell degree");
    max_degree = MAX(max_degree, degree);
  }
  degree = max_degree;
  RSS(ref_mpi_max(ref_mpi, &degree, &max_degree, REF_INT_TYPE), "mpi max");
  RSS(ref_mpi_bcast(ref_mpi, &max_degree, 1, REF_INT_TYPE), "min");

  max_age = 0;
  each_ref_node_valid_node(ref_node, node) {
    max_age = MAX(max_age, ref_node_age(ref_node, node));
  }
  age = max_age;
  RSS(ref_mpi_max(ref_mpi, &age, &max_age, REF_INT_TYPE), "mpi max");
  RSS(ref_mpi_bcast(ref_mpi, &max_age, 1, REF_INT_TYPE), "min");

  min_normdev = 2.0;
  if (ref_geom_model_loaded(ref_grid_geom(ref_grid)) ||
      ref_geom_meshlinked(ref_grid_geom(ref_grid))) {
    ref_cell = ref_grid_tri(ref_grid);
    each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
      RSS(ref_geom_tri_norm_deviation(ref_grid, nodes, &normdev), "norm dev");
      min_normdev = MIN(min_normdev, normdev);
    }
  }
  normdev = min_normdev;
  RSS(ref_mpi_min(ref_mpi, &normdev, &min_normdev, REF_DBL_TYPE), "mpi max");
  RSS(ref_mpi_bcast(ref_mpi, &min_normdev, 1, REF_DBL_TYPE), "min");

  min_ratio = REF_DBL_MAX;
  max_ratio = REF_DBL_MIN;
  RSS(ref_edge_create(&ref_edge, ref_grid), "make edges");
  for (edge = 0; edge < ref_edge_n(ref_edge); edge++) {
    RSS(ref_edge_part(ref_edge, edge, &part), "edge part");
    if (part == ref_mpi_rank(ref_grid_mpi(ref_grid))) {
      RSS(ref_node_ratio(ref_grid_node(ref_grid),
                         ref_edge_e2n(ref_edge, 0, edge),
                         ref_edge_e2n(ref_edge, 1, edge), &ratio),
          "rat");
      min_ratio = MIN(min_ratio, ratio);
      max_ratio = MAX(max_ratio, ratio);
    }
  }
  RSS(ref_edge_free(ref_edge), "free edge");
  ratio = min_ratio;
  RSS(ref_mpi_min(ref_mpi, &ratio, &min_ratio, REF_DBL_TYPE), "mpi min");
  RSS(ref_mpi_bcast(ref_mpi, &min_ratio, 1, REF_DBL_TYPE), "min");
  ratio = max_ratio;
  RSS(ref_mpi_max(ref_mpi, &ratio, &max_ratio, REF_DBL_TYPE), "mpi max");
  RSS(ref_mpi_bcast(ref_mpi, &max_ratio, 1, REF_DBL_TYPE), "max");

  target_normdev = MAX(MIN(0.1, min_normdev), 1.0e-3);
  ref_adapt->post_min_normdev = target_normdev;

  target_quality = MAX(MIN(0.1, min_quality), 1.0e-3);
  ref_adapt->collapse_quality_absolute = target_quality;
  ref_adapt->smooth_min_quality = target_quality;

  ref_node->min_volume = MIN(1.0e-15, 0.01 * min_metric_vol);

  /* allow edge growth when interpolating metric continuously */
  ref_adapt->split_ratio_growth = REF_FALSE;
  if (NULL != ref_grid_interp(ref_grid)) {
    ref_adapt->split_ratio_growth =
        ref_interp_continuously(ref_grid_interp(ref_grid));
  }

  /* bound ratio to current range */
  ref_adapt->post_min_ratio = MIN(min_ratio, ref_adapt->collapse_ratio);
  ref_adapt->post_max_ratio = MAX(max_ratio, ref_adapt->split_ratio);

  if (ref_adapt->post_max_ratio > 4.0 && ref_adapt->post_min_ratio > 0.4) {
    ref_adapt->post_min_ratio =
        (4.0 / ref_adapt->post_max_ratio) * ref_adapt->post_min_ratio;
  }

  /* when close to convergence, prevent high lower limit from stopping split */
  if (ref_adapt->post_max_ratio < 3.5 && ref_adapt->post_max_ratio > 1.6 &&
      ref_adapt->post_min_ratio > 0.4) {
    ref_adapt->post_min_ratio =
        (1.4 / ref_adapt->post_max_ratio) * ref_adapt->post_min_ratio;
  }

  ref_adapt->split_ratio = sqrt(2.0);
  /* prevent overshoot of number of nodes, unless unreliable due to mixed */
  if (!mixed && nodes_per_complexity > 3.0)
    ref_adapt->split_ratio = 0.5 * (sqrt(2.0) + max_ratio);

  if (ABS(ref_adapt->last_min_ratio - ref_adapt->post_min_ratio) <
          1e-2 * ref_adapt->post_min_ratio &&
      ABS(ref_adapt->last_max_ratio - ref_adapt->post_max_ratio) <
          1e-2 * ref_adapt->post_max_ratio &&
      (max_age < 50 ||
       (ref_adapt->post_min_ratio > 0.1 && ref_adapt->post_max_ratio < 3.0)) &&
      1.5 > ref_adapt->split_ratio) {
    *all_done = REF_TRUE;
    if (ref_grid_once(ref_grid)) {
      printf("termination recommended\n");
    }
  } else {
    *all_done = REF_FALSE;
  }
  RSS(ref_mpi_bcast(ref_mpi, all_done, 1, REF_INT_TYPE), "done");

  ref_adapt->last_min_ratio = ref_adapt->post_min_ratio;
  ref_adapt->last_max_ratio = ref_adapt->post_max_ratio;

  if (ref_grid_once(ref_grid)) {
    printf("limit quality %6.4f normdev %6.4f ratio %6.4f %6.2f split %6.2f\n",
           target_quality, ref_adapt->post_min_normdev,
           ref_adapt->post_min_ratio, ref_adapt->post_max_ratio,
           ref_adapt->split_ratio);
    printf("max degree %d max age %d normdev %7.4f\n", max_degree, max_age,
           min_normdev);
    printf("nnode %10d ncell %12ld complexity %12.1f ratio %5.2f\n", nnode,
           ncell, complexity, nodes_per_complexity);
    printf("volume range %e %e metric %e floor %e\n", max_volume, min_volume,
           min_metric_vol, ref_node->min_volume);
  }

  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_adapt_tattle(REF_GRID ref_grid,
                                           const char *mode) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL quality, min_quality;
  REF_DBL normdev, min_normdev;
  REF_INT node, nnode;
  REF_DBL ratio, min_ratio, max_ratio, delta;
  REF_INT edge, part;
  REF_EDGE ref_edge;
  char is_ok = ' ';
  char not_ok = '*';
  char quality_met, short_met, long_met, normdev_met;

  if (ref_grid_twod(ref_grid) || ref_grid_surf(ref_grid)) {
    ref_cell = ref_grid_tri(ref_grid);
  } else {
    ref_cell = ref_grid_tet(ref_grid);
  }

  min_quality = 1.0;
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (ref_grid_twod(ref_grid)) {
      RSS(ref_node_tri_quality(ref_grid_node(ref_grid), nodes, &quality),
          "qual");
    } else if (ref_grid_surf(ref_grid)) {
      RSS(ref_node_tri_quality(ref_grid_node(ref_grid), nodes, &quality),
          "qual");
    } else {
      RSS(ref_node_tet_quality(ref_grid_node(ref_grid), nodes, &quality),
          "qual");
    }
    min_quality = MIN(min_quality, quality);
  }
  quality = min_quality;
  RSS(ref_mpi_min(ref_mpi, &quality, &min_quality, REF_DBL_TYPE), "min");
  RSS(ref_mpi_bcast(ref_mpi, &quality, 1, REF_DBL_TYPE), "min");

  nnode = 0;
  each_ref_node_valid_node(ref_node, node) {
    if (ref_node_owned(ref_node, node)) {
      nnode++;
    }
  }
  RSS(ref_mpi_allsum(ref_mpi, &nnode, 1, REF_INT_TYPE), "int sum");

  min_normdev = 2.0;
  if (ref_geom_model_loaded(ref_grid_geom(ref_grid)) ||
      ref_geom_meshlinked(ref_grid_geom(ref_grid))) {
    ref_cell = ref_grid_tri(ref_grid);
    each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
      RSS(ref_geom_tri_norm_deviation(ref_grid, nodes, &normdev), "norm dev");
      min_normdev = MIN(min_normdev, normdev);
    }
  }
  normdev = min_normdev;
  RSS(ref_mpi_min(ref_mpi, &normdev, &min_normdev, REF_DBL_TYPE), "mpi max");
  RSS(ref_mpi_bcast(ref_mpi, &min_normdev, 1, REF_DBL_TYPE), "min");

  min_ratio = REF_DBL_MAX;
  max_ratio = REF_DBL_MIN;
  RSS(ref_edge_create(&ref_edge, ref_grid), "make edges");
  for (edge = 0; edge < ref_edge_n(ref_edge); edge++) {
    RSS(ref_edge_part(ref_edge, edge, &part), "edge part");
    if (part == ref_mpi_rank(ref_grid_mpi(ref_grid))) {
      RSS(ref_node_ratio(ref_grid_node(ref_grid),
                         ref_edge_e2n(ref_edge, 0, edge),
                         ref_edge_e2n(ref_edge, 1, edge), &ratio),
          "rat");
      min_ratio = MIN(min_ratio, ratio);
      max_ratio = MAX(max_ratio, ratio);
    }
  }
  RSS(ref_edge_free(ref_edge), "free edge");
  ratio = min_ratio;
  RSS(ref_mpi_min(ref_mpi, &ratio, &min_ratio, REF_DBL_TYPE), "mpi min");
  RSS(ref_mpi_bcast(ref_mpi, &min_ratio, 1, REF_DBL_TYPE), "min");
  ratio = max_ratio;
  RSS(ref_mpi_max(ref_mpi, &ratio, &max_ratio, REF_DBL_TYPE), "mpi max");
  RSS(ref_mpi_bcast(ref_mpi, &max_ratio, 1, REF_DBL_TYPE), "max");

  RSS(ref_mpi_stopwatch_delta(ref_mpi, &delta), "time delta");

  if (ref_grid_once(ref_grid)) {
    quality_met = is_ok;
    short_met = is_ok;
    long_met = is_ok;
    normdev_met = is_ok;
    if (min_quality < ref_grid_adapt(ref_grid, smooth_min_quality))
      quality_met = not_ok;
    if (min_ratio < ref_grid_adapt(ref_grid, post_min_ratio))
      short_met = not_ok;
    if (max_ratio > ref_grid_adapt(ref_grid, post_max_ratio)) long_met = not_ok;
    if (min_normdev < ref_grid_adapt(ref_grid, post_min_normdev))
      normdev_met = not_ok;

    printf(
        "mr %c %6.4f ratio %c %6.4f %6.2f %c norm %6.4f %c %4.1f sec "
        "%5s n %d\n",
        quality_met, min_quality, short_met, min_ratio, max_ratio, long_met,
        min_normdev, normdev_met, delta, mode, nnode);
  }

  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_adapt_min_sliver_angle(REF_GRID ref_grid,
                                                     REF_INT *nodes,
                                                     REF_DBL *angle) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_BOOL geom_node, has_side;
  REF_INT i, cell_node, node, node1, node2;

  REF_DBL dx1[3], dx2[3], dot;

  for (cell_node = 0; cell_node < 3; cell_node++) {
    node = nodes[cell_node];
    RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_NODE, &geom_node),
        "geom node check");
    if (geom_node) {
      node1 = cell_node + 1;
      node2 = cell_node + 2;
      if (node1 > 2) node1 -= 3;
      if (node2 > 2) node2 -= 3;
      node1 = nodes[node1];
      node2 = nodes[node2];
      RSS(ref_cell_has_side(ref_grid_edg(ref_grid), node, node1, &has_side),
          "has edge node-node1");
      if (!has_side) continue;
      RSS(ref_cell_has_side(ref_grid_edg(ref_grid), node, node2, &has_side),
          "has edge node-node2");
      if (!has_side) continue;
      for (i = 0; i < 3; i++) {
        dx1[i] =
            ref_node_xyz(ref_node, i, node1) - ref_node_xyz(ref_node, i, node);
        dx2[i] =
            ref_node_xyz(ref_node, i, node2) - ref_node_xyz(ref_node, i, node);
      }
      RSS(ref_math_normalize(dx1), "dx1");
      RSS(ref_math_normalize(dx2), "dx2");
      dot = ref_math_dot(dx1, dx2);
      *angle = MIN(*angle, ref_math_in_degrees(acos(dot)));
    }
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_adapt_tattle_faces(REF_GRID ref_grid) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_CELL ref_cell;
  REF_INT cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL quality, min_quality;
  REF_DBL normdev, min_normdev;
  REF_DBL ratio, min_ratio, max_ratio;
  REF_DBL angle, min_angle;
  REF_INT edge, n0, n1;
  REF_INT id, min_id, max_id;

  ref_cell = ref_grid_tri(ref_grid);
  RSS(ref_cell_id_range(ref_cell, ref_mpi, &min_id, &max_id), "id range");
  for (id = min_id; id <= max_id; id++) {
    min_quality = 1.0;
    each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
      if (id != nodes[ref_cell_id_index(ref_cell)]) continue;
      RSS(ref_node_tri_quality(ref_grid_node(ref_grid), nodes, &quality),
          "qual");
      min_quality = MIN(min_quality, quality);
    }
    quality = min_quality;
    RSS(ref_mpi_min(ref_mpi, &quality, &min_quality, REF_DBL_TYPE), "min");
    RSS(ref_mpi_bcast(ref_mpi, &quality, 1, REF_DBL_TYPE), "min");

    min_normdev = 2.0;
    min_angle = 90.0;
    if (ref_geom_model_loaded(ref_grid_geom(ref_grid)) ||
        ref_geom_meshlinked(ref_grid_geom(ref_grid))) {
      ref_cell = ref_grid_tri(ref_grid);
      each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
        if (id != nodes[ref_cell_id_index(ref_cell)]) continue;
        RSS(ref_geom_tri_norm_deviation(ref_grid, nodes, &normdev), "norm dev");
        min_normdev = MIN(min_normdev, normdev);
        RSS(ref_adapt_min_sliver_angle(ref_grid, nodes, &min_angle),
            "sliver angle");
      }
    }
    normdev = min_normdev;
    RSS(ref_mpi_min(ref_mpi, &normdev, &min_normdev, REF_DBL_TYPE), "mpi max");
    RSS(ref_mpi_bcast(ref_mpi, &min_normdev, 1, REF_DBL_TYPE), "min");
    angle = min_angle;
    RSS(ref_mpi_min(ref_mpi, &angle, &min_angle, REF_DBL_TYPE), "mpi max");
    RSS(ref_mpi_bcast(ref_mpi, &min_angle, 1, REF_DBL_TYPE), "min");

    min_ratio = REF_DBL_MAX;
    max_ratio = REF_DBL_MIN;
    each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
      if (id != nodes[ref_cell_id_index(ref_cell)]) continue;
      for (edge = 0; edge < 3; edge++) {
        n0 = edge;
        n1 = edge + 1;
        if (n1 > 2) n1 -= 3;
        RSS(ref_node_ratio(ref_grid_node(ref_grid), nodes[n0], nodes[n1],
                           &ratio),
            "rat");
        min_ratio = MIN(min_ratio, ratio);
        max_ratio = MAX(max_ratio, ratio);
      }
      min_quality = MIN(min_quality, quality);
    }
    ratio = min_ratio;
    RSS(ref_mpi_min(ref_mpi, &ratio, &min_ratio, REF_DBL_TYPE), "mpi min");
    RSS(ref_mpi_bcast(ref_mpi, &min_ratio, 1, REF_DBL_TYPE), "min");
    ratio = max_ratio;
    RSS(ref_mpi_max(ref_mpi, &ratio, &max_ratio, REF_DBL_TYPE), "mpi max");
    RSS(ref_mpi_bcast(ref_mpi, &max_ratio, 1, REF_DBL_TYPE), "max");

    if (ref_mpi_once(ref_mpi)) {
      if (min_quality < 0.1 || min_ratio < 0.1 || max_ratio > 4.0 ||
          min_normdev < 0.5) {
        printf(
            "face%6d quality %6.4f ratio %6.4f %6.2f normdev %6.4f"
            " topo angle %5.2f\n",
            id, min_quality, min_ratio, max_ratio, min_normdev, angle);
      }
    }
  }

  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_adapt_swap(REF_GRID ref_grid) {
  REF_INT pass;
  RSS(ref_cavity_pass(ref_grid), "cavity pass");
  if (ref_grid_surf(ref_grid) || ref_grid_twod(ref_grid)) {
    for (pass = 0; pass < 3; pass++) {
      RSS(ref_swap_tri_pass(ref_grid), "swap pass");
    }
  }
  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_adapt_topo(REF_GRID ref_grid) {
  RSS(ref_validation_cell_face(ref_grid), "cell face");
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_adapt_pass(REF_GRID ref_grid, REF_BOOL *all_done) {
  REF_INT ngeom;
  REF_BOOL all_done0, all_done1;
  REF_INT i, swap_smooth_passes = 1;

  RSS(ref_adapt_parameter(ref_grid, &all_done0), "param");

  RSS(ref_gather_ngeom(ref_grid_node(ref_grid), ref_grid_geom(ref_grid),
                       REF_GEOM_FACE, &ngeom),
      "count ngeom");
  if (ngeom > 0) RSS(ref_geom_verify_topo(ref_grid), "adapt preflight check");

  ref_gather_blocking_frame(ref_grid, "threed pass");
  if (ref_grid_adapt(ref_grid, timing_level) > 1)
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt start");
  if (ref_grid_adapt(ref_grid, watch_param))
    RSS(ref_adapt_tattle(ref_grid, "start"), "tattle");
  if (ref_grid_adapt(ref_grid, watch_topo))
    RSS(ref_adapt_topo(ref_grid), "topo");

  RSS(ref_adapt_swap(ref_grid), "swap pass");
  ref_gather_blocking_frame(ref_grid, "swap");
  if (ref_grid_adapt(ref_grid, timing_level) > 1)
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt swap");
  if (ref_grid_adapt(ref_grid, watch_param))
    RSS(ref_adapt_tattle(ref_grid, "swap"), "tattle");
  if (ref_grid_adapt(ref_grid, watch_topo))
    RSS(ref_adapt_topo(ref_grid), "topo");

  RSS(ref_collapse_pass(ref_grid), "col pass");
  ref_gather_blocking_frame(ref_grid, "collapse");
  if (ref_grid_adapt(ref_grid, timing_level) > 1)
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt col");
  if (ref_grid_adapt(ref_grid, watch_param))
    RSS(ref_adapt_tattle(ref_grid, "col"), "tattle");
  if (ref_grid_adapt(ref_grid, watch_topo))
    RSS(ref_adapt_topo(ref_grid), "topo");

  RSS(ref_adapt_swap(ref_grid), "swap pass");
  ref_gather_blocking_frame(ref_grid, "swap");
  if (ref_grid_adapt(ref_grid, timing_level) > 1)
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt swap");
  if (ref_grid_adapt(ref_grid, watch_param))
    RSS(ref_adapt_tattle(ref_grid, "swap"), "tattle");
  if (ref_grid_adapt(ref_grid, watch_topo))
    RSS(ref_adapt_topo(ref_grid), "topo");

  ref_grid_adapt(ref_grid, post_max_ratio) = sqrt(2.0);

  RSS(ref_collapse_pass(ref_grid), "col pass");
  ref_gather_blocking_frame(ref_grid, "collapse");
  if (ref_grid_adapt(ref_grid, timing_level) > 1)
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt col");
  if (ref_grid_adapt(ref_grid, watch_param))
    RSS(ref_adapt_tattle(ref_grid, "col"), "tattle");
  if (ref_grid_adapt(ref_grid, watch_topo))
    RSS(ref_adapt_topo(ref_grid), "topo");

  ref_grid_adapt(ref_grid, post_max_ratio) =
      ref_grid_adapt(ref_grid, last_max_ratio);

  for (i = 0; i < swap_smooth_passes; i++) {
    RSS(ref_adapt_swap(ref_grid), "swap pass");
    ref_gather_blocking_frame(ref_grid, "swap");
    if (ref_grid_adapt(ref_grid, timing_level) > 1)
      ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt swap");
    if (ref_grid_adapt(ref_grid, watch_param))
      RSS(ref_adapt_tattle(ref_grid, "swap"), "tattle");
    if (ref_grid_adapt(ref_grid, watch_topo))
      RSS(ref_adapt_topo(ref_grid), "topo");

    RSS(ref_smooth_pass(ref_grid), "smooth pass");
    ref_gather_blocking_frame(ref_grid, "smooth");
    if (ref_grid_adapt(ref_grid, timing_level > 1))
      ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt move");
    if (ref_grid_adapt(ref_grid, watch_param))
      RSS(ref_adapt_tattle(ref_grid, "move"), "tattle");
    if (ref_grid_adapt(ref_grid, watch_topo))
      RSS(ref_adapt_topo(ref_grid), "topo");
  }

  RSS(ref_adapt_swap(ref_grid), "swap pass");
  ref_gather_blocking_frame(ref_grid, "swap");
  if (ref_grid_adapt(ref_grid, timing_level) > 1)
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt swap");
  if (ref_grid_adapt(ref_grid, watch_param))
    RSS(ref_adapt_tattle(ref_grid, "swap"), "tattle");
  if (ref_grid_adapt(ref_grid, watch_topo))
    RSS(ref_adapt_topo(ref_grid), "topo");

  RSS(ref_adapt_parameter(ref_grid, &all_done1), "param");

  RSS(ref_split_pass(ref_grid), "split surfpass");
  ref_gather_blocking_frame(ref_grid, "split");
  if (ref_grid_adapt(ref_grid, timing_level) > 1)
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt spl");
  if (ref_grid_adapt(ref_grid, watch_param))
    RSS(ref_adapt_tattle(ref_grid, "split"), "tattle");
  if (ref_grid_adapt(ref_grid, watch_topo))
    RSS(ref_adapt_topo(ref_grid), "topo");

  RSS(ref_adapt_swap(ref_grid), "swap pass");
  ref_gather_blocking_frame(ref_grid, "swap");
  if (ref_grid_adapt(ref_grid, timing_level) > 1)
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt swap");
  if (ref_grid_adapt(ref_grid, watch_param))
    RSS(ref_adapt_tattle(ref_grid, "swap"), "tattle");
  if (ref_grid_adapt(ref_grid, watch_topo))
    RSS(ref_adapt_topo(ref_grid), "topo");

  for (i = 0; i < swap_smooth_passes; i++) {
    RSS(ref_smooth_pass(ref_grid), "smooth pass");
    ref_gather_blocking_frame(ref_grid, "smooth");
    if (ref_grid_adapt(ref_grid, timing_level) > 1)
      ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt move");
    if (ref_grid_adapt(ref_grid, watch_param))
      RSS(ref_adapt_tattle(ref_grid, "move"), "tattle");
    if (ref_grid_adapt(ref_grid, watch_topo))
      RSS(ref_adapt_topo(ref_grid), "topo");

    RSS(ref_adapt_swap(ref_grid), "swap pass");
    ref_gather_blocking_frame(ref_grid, "swap");
    if (ref_grid_adapt(ref_grid, timing_level) > 1)
      ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt swap");
    if (ref_grid_adapt(ref_grid, watch_param))
      RSS(ref_adapt_tattle(ref_grid, "swap"), "tattle");
    if (ref_grid_adapt(ref_grid, watch_topo))
      RSS(ref_adapt_topo(ref_grid), "topo");
  }

  ref_grid_adapt(ref_grid, post_max_ratio) = sqrt(2.0);

  RSS(ref_collapse_pass(ref_grid), "col pass");
  ref_gather_blocking_frame(ref_grid, "collapse");
  if (ref_grid_adapt(ref_grid, timing_level) > 1)
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt col");
  if (ref_grid_adapt(ref_grid, watch_param))
    RSS(ref_adapt_tattle(ref_grid, "col"), "tattle");
  if (ref_grid_adapt(ref_grid, watch_topo))
    RSS(ref_adapt_topo(ref_grid), "topo");

  ref_grid_adapt(ref_grid, post_max_ratio) =
      ref_grid_adapt(ref_grid, last_max_ratio);

  RSS(ref_adapt_swap(ref_grid), "swap pass");
  ref_gather_blocking_frame(ref_grid, "swap");
  if (ref_grid_adapt(ref_grid, timing_level) > 1)
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt swap");
  if (ref_grid_adapt(ref_grid, watch_param))
    RSS(ref_adapt_tattle(ref_grid, "swap"), "tattle");
  if (ref_grid_adapt(ref_grid, watch_topo))
    RSS(ref_adapt_topo(ref_grid), "topo");

  for (i = 0; i < swap_smooth_passes; i++) {
    RSS(ref_smooth_pass(ref_grid), "smooth pass");
    ref_gather_blocking_frame(ref_grid, "smooth");
    if (ref_grid_adapt(ref_grid, timing_level) > 1)
      ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt move");
    if (ref_grid_adapt(ref_grid, watch_param))
      RSS(ref_adapt_tattle(ref_grid, "move"), "tattle");
    if (ref_grid_adapt(ref_grid, watch_topo))
      RSS(ref_adapt_topo(ref_grid), "topo");

    RSS(ref_adapt_swap(ref_grid), "swap pass");
    ref_gather_blocking_frame(ref_grid, "swap");
    if (ref_grid_adapt(ref_grid, timing_level) > 1)
      ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt swap");
    if (ref_grid_adapt(ref_grid, watch_param))
      RSS(ref_adapt_tattle(ref_grid, "swap"), "tattle");
    if (ref_grid_adapt(ref_grid, watch_topo))
      RSS(ref_adapt_topo(ref_grid), "topo");
  }

  if (ref_grid_adapt(ref_grid, unlock_tet)) {
    RSS(ref_split_edge_geometry(ref_grid), "split edge geom");
    if (ref_grid_adapt(ref_grid, timing_level) > 1)
      ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "adapt unlock");
  }

  RSS(ref_adapt_tattle(ref_grid, "adapt"), "tattle");

  if (ngeom > 0)
    RSS(ref_geom_verify_topo(ref_grid), "geom topo postflight check");

  *all_done = (all_done0 && all_done1);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_adapt_surf_to_geom(REF_GRID ref_grid, REF_INT passes) {
  REF_BOOL all_done = REF_FALSE;
  REF_INT pass;

  RSS(ref_migrate_to_balance(ref_grid), "migrate to single part");
  RSS(ref_metric_interpolated_curvature(ref_grid), "interp curve");
  ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "curvature");
  RSS(ref_adapt_tattle_faces(ref_grid), "tattle");
  ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "tattle faces");

  for (pass = 0; !all_done && pass < passes; pass++) {
    if (ref_grid_once(ref_grid))
      printf("\n pass %d of %d with %d ranks\n", pass + 1, passes,
             ref_mpi_n(ref_grid_mpi(ref_grid)));
    RSS(ref_adapt_pass(ref_grid, &all_done), "pass");

    RSS(ref_migrate_to_balance(ref_grid), "migrate to single part");
    RSS(ref_grid_pack(ref_grid), "pack");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "pack");

    RSS(ref_metric_interpolated_curvature(ref_grid), "interp curve");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "curvature");

    RSS(ref_adapt_tattle_faces(ref_grid), "tattle");
    ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "tattle faces");
  }

  return REF_SUCCESS;
}
