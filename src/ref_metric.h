
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

#ifndef REF_METRIC_H
#define REF_METRIC_H

#include "ref_defs.h"

BEGIN_C_DECLORATION
END_C_DECLORATION

#include "ref_grid.h"
#include "ref_node.h"
#include "ref_recon.h"

BEGIN_C_DECLORATION

REF_FCN REF_STATUS ref_metric_show(REF_DBL *metric);
REF_FCN REF_STATUS ref_metric_inspect(REF_NODE ref_node);

REF_FCN REF_STATUS ref_metric_from_node(REF_DBL *metric, REF_NODE ref_node);
REF_FCN REF_STATUS ref_metric_to_node(REF_DBL *metric, REF_NODE ref_node);

REF_FCN REF_STATUS ref_metric_olympic_node(REF_NODE ref_node, REF_DBL h);
REF_FCN REF_STATUS ref_metric_side_node(REF_NODE ref_node);
REF_FCN REF_STATUS ref_metric_ring_node(REF_NODE ref_node);
REF_FCN REF_STATUS ref_metric_twod_analytic_node(REF_NODE ref_node,
                                                 const char *version);
REF_FCN REF_STATUS ref_metric_ugawg_node(REF_NODE ref_node, REF_INT version);
REF_FCN REF_STATUS ref_metric_masabl_node(REF_NODE ref_node);
REF_FCN REF_STATUS ref_metric_circle_node(REF_NODE ref_node);
REF_FCN REF_STATUS ref_metric_twod_node(REF_NODE ref_node);
REF_FCN REF_STATUS ref_metric_delta_box_node(REF_GRID ref_grid);

REF_FCN REF_STATUS ref_metric_synchronize(REF_GRID ref_grid);
REF_FCN REF_STATUS ref_metric_interpolate(REF_INTERP ref_interp);
REF_FCN REF_STATUS ref_metric_interpolate_node(REF_GRID ref_grid, REF_INT node);
REF_FCN REF_STATUS ref_metric_interpolate_between(REF_GRID ref_grid,
                                                  REF_INT node0, REF_INT node1,
                                                  REF_INT new_node);

REF_FCN REF_STATUS ref_metric_metric_space_gradation(REF_DBL *metric,
                                                     REF_GRID ref_grid,
                                                     REF_DBL beta);
REF_FCN REF_STATUS ref_metric_mixed_space_gradation(REF_DBL *metric,
                                                    REF_GRID ref_grid,
                                                    REF_DBL beta, REF_DBL t);
REF_FCN REF_STATUS ref_metric_hessian_filter(REF_DBL *metric,
                                             REF_GRID ref_grid);
REF_FCN REF_STATUS ref_metric_gradation_at_complexity(REF_DBL *metric,
                                                      REF_GRID ref_grid,
                                                      REF_DBL gradation,
                                                      REF_DBL complexity);
REF_FCN REF_STATUS ref_metric_gradation_at_complexity_mixed(REF_DBL *metric,
                                                            REF_GRID ref_grid,
                                                            REF_DBL gradation,
                                                            REF_DBL complexity);

REF_FCN REF_STATUS ref_metric_sanitize(REF_GRID ref_grid);
REF_FCN REF_STATUS ref_metric_sanitize_threed(REF_GRID ref_grid);
REF_FCN REF_STATUS ref_metric_sanitize_twod(REF_GRID ref_grid);

REF_FCN REF_STATUS ref_metric_interpolated_curvature(REF_GRID ref_grid);
REF_FCN REF_STATUS ref_metric_constrain_curvature(REF_GRID ref_grid);
REF_FCN REF_STATUS ref_metric_from_curvature(REF_DBL *metric,
                                             REF_GRID ref_grid);

REF_FCN REF_STATUS ref_metric_imply_from(REF_DBL *metric, REF_GRID ref_grid);
REF_FCN REF_STATUS ref_metric_imply_non_tet(REF_DBL *metric, REF_GRID ref_grid);

REF_FCN REF_STATUS ref_metric_smr(REF_DBL *metric0, REF_DBL *metric1,
                                  REF_DBL *metric, REF_GRID ref_grid);

REF_FCN REF_STATUS ref_metric_interpolation_error(REF_DBL *metric,
                                                  REF_DBL *hess,
                                                  REF_GRID ref_grid,
                                                  REF_DBL *interpolation_error);
REF_FCN REF_STATUS ref_metric_integrate_error(REF_GRID ref_grid,
                                              REF_DBL *interpolation_error,
                                              REF_DBL *total_error);

REF_FCN REF_STATUS ref_metric_complexity(REF_DBL *metric, REF_GRID ref_grid,
                                         REF_DBL *complexity);
REF_FCN REF_STATUS ref_metric_set_complexity(REF_DBL *metric, REF_GRID ref_grid,
                                             REF_DBL target_complexity);
REF_FCN REF_STATUS ref_metric_limit_aspect_ratio(REF_DBL *metric,
                                                 REF_GRID ref_grid,
                                                 REF_DBL aspect_ratio);
REF_FCN REF_STATUS ref_metric_limit_aspect_ratio_field(
    REF_DBL *metric, REF_GRID ref_grid, REF_DBL *aspect_ratio_field);
REF_FCN REF_STATUS ref_metric_limit_h(REF_DBL *metric, REF_GRID ref_grid,
                                      REF_DBL hmin, REF_DBL hmax);
REF_FCN REF_STATUS ref_metric_limit_h_at_complexity(REF_DBL *metric,
                                                    REF_GRID ref_grid,
                                                    REF_DBL hmin, REF_DBL hmax,
                                                    REF_DBL complexity);
REF_FCN REF_STATUS ref_metric_buffer(REF_DBL *metric, REF_GRID ref_grid);
REF_FCN REF_STATUS ref_metric_buffer_at_complexity(REF_DBL *metric,
                                                   REF_GRID ref_grid,
                                                   REF_DBL complexity);

REF_FCN REF_STATUS ref_metric_faceid_spacing(REF_DBL *metric, REF_GRID ref_grid,
                                             REF_INT faceid,
                                             REF_DBL set_normal_spacing,
                                             REF_DBL ceil_normal_spacing,
                                             REF_DBL tangential_aspect_ratio);

REF_FCN REF_STATUS ref_metric_multigrad(REF_DBL *metric, REF_GRID ref_grid,
                                        REF_DBL *grad, REF_INT p_norm,
                                        REF_DBL gradation, REF_DBL complexity);
REF_FCN REF_STATUS ref_metric_lp(REF_DBL *metric, REF_GRID ref_grid,
                                 REF_DBL *scalar,
                                 REF_RECON_RECONSTRUCTION reconstruction,
                                 REF_INT p_norm, REF_DBL gradation,
                                 REF_DBL aspect_ratio, REF_DBL complexity);
REF_FCN REF_STATUS ref_metric_lp_mixed(REF_DBL *metric, REF_GRID ref_grid,
                                       REF_DBL *scalar,
                                       REF_RECON_RECONSTRUCTION reconstruction,
                                       REF_INT p_norm, REF_DBL gradation,
                                       REF_DBL complexity);
REF_FCN REF_STATUS ref_metric_moving_multiscale(
    REF_DBL *metric, REF_GRID ref_grid, REF_DBL *displaced, REF_DBL *scalar,
    REF_RECON_RECONSTRUCTION reconstruction, REF_INT p_norm, REF_DBL gradation,
    REF_DBL complexity);
REF_FCN REF_STATUS ref_metric_eig_bal(REF_DBL *metric, REF_GRID ref_grid,
                                      REF_DBL *scalar,
                                      REF_RECON_RECONSTRUCTION reconstruction,
                                      REF_INT p_norm, REF_DBL gradation,
                                      REF_DBL complexity);
REF_FCN REF_STATUS ref_metric_local_scale(REF_DBL *metric, REF_GRID ref_grid,
                                          REF_INT p_norm);
REF_FCN REF_STATUS ref_metric_opt_goal(REF_DBL *metric, REF_GRID ref_grid,
                                       REF_INT nequations, REF_DBL *solution,
                                       REF_RECON_RECONSTRUCTION reconstruction,
                                       REF_INT p_norm, REF_DBL gradation,
                                       REF_DBL complexity);

REF_FCN REF_STATUS ref_metric_belme_gfe(
    REF_DBL *metric, REF_GRID ref_grid, REF_INT ldim, REF_DBL *prim_dual,
    REF_RECON_RECONSTRUCTION reconstruction);
REF_FCN REF_STATUS ref_metric_belme_gu(REF_DBL *metric, REF_GRID ref_grid,
                                       REF_INT ldim, REF_DBL *prim_dual,
                                       REF_DBL mach, REF_DBL re,
                                       REF_DBL reference_temp,
                                       REF_RECON_RECONSTRUCTION reconstruction);

REF_FCN REF_STATUS ref_metric_cons_euler_g(
    REF_DBL *g, REF_GRID ref_grid, REF_INT ldim, REF_DBL *prim_dual,
    REF_RECON_RECONSTRUCTION reconstruction);
REF_FCN REF_STATUS ref_metric_cons_viscous_g(
    REF_DBL *g, REF_GRID ref_grid, REF_INT ldim, REF_DBL *prim_dual,
    REF_DBL mach, REF_DBL re, REF_DBL reference_temp,
    REF_RECON_RECONSTRUCTION reconstruction);

REF_FCN REF_STATUS ref_metric_cons_assembly(
    REF_DBL *metric, REF_DBL *g, REF_GRID ref_grid, REF_INT ldim,
    REF_DBL *prim_dual, REF_RECON_RECONSTRUCTION reconstruction);

REF_FCN REF_STATUS ref_metric_histogram(REF_DBL *metric, REF_GRID ref_grid,
                                        const char *filename);

REF_FCN REF_STATUS ref_metric_step_exp(REF_DBL s, REF_DBL *h, REF_DBL h0,
                                       REF_DBL h1, REF_DBL h2, REF_DBL s1,
                                       REF_DBL s2, REF_DBL width);

REF_FCN REF_STATUS ref_metric_parse(REF_DBL *metric, REF_GRID ref_grid,
                                    int narg, char *args[]);
REF_FCN REF_STATUS ref_metric_parse_to_node(REF_GRID ref_grid, int narg,
                                            char *args[]);
REF_FCN REF_STATUS ref_metric_truncated_cone_dist(REF_DBL *cone_geom,
                                                  REF_DBL *xyz, REF_DBL *dist);

REF_FCN REF_STATUS ref_metric_isotropic(REF_DBL *metric, REF_GRID ref_grid,
                                        REF_DBL *hh);

typedef REF_STATUS (*ref_metric_integrand)(void *state, REF_DBL t,
                                           REF_DBL *value);
REF_FCN REF_STATUS ref_metric_integrand_err2(void *m_diag_sys_hess,
                                             REF_DBL theta_over_2pi,
                                             REF_DBL *radial_error);
REF_FCN REF_STATUS ref_metric_integrate(ref_metric_integrand integrand,
                                        void *state, REF_DBL *integral);

typedef REF_STATUS (*ref_metric_integrand2)(void *state, REF_DBL *bary,
                                            REF_DBL *value);
REF_FCN REF_STATUS ref_metric_integrate2(ref_metric_integrand2 integrand,
                                         void *state, REF_DBL *integral);

REF_FCN REF_STATUS ref_metric_interpolation_error2(REF_GRID ref_grid,
                                                   REF_DBL *scalar);

END_C_DECLORATION

#endif /* REF_METRIC_H */
