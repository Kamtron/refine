
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

#ifndef REF_PHYS_H
#define REF_PHYS_H

#include <stdio.h>

#include "ref_defs.h"
#include "ref_dict.h"
#include "ref_geom.h"
#include "ref_grid.h"
#include "ref_node.h"

BEGIN_C_DECLORATION

REF_FCN REF_STATUS ref_phys_flip_twod_yz(REF_NODE ref_node, REF_INT ldim,
                                         REF_DBL *field);

REF_FCN REF_STATUS ref_phys_make_primitive(REF_DBL *conserved,
                                           REF_DBL *primitive);
REF_FCN REF_STATUS ref_phys_make_conserved(REF_DBL *primitive,
                                           REF_DBL *conserved);
REF_FCN REF_STATUS ref_phys_entropy_adjoint(REF_DBL *primitive, REF_DBL *dual);
REF_FCN REF_STATUS ref_phys_entropy_flux(REF_DBL *primitive, REF_DBL *flux);

REF_FCN REF_STATUS ref_phys_euler(REF_DBL *state, REF_DBL *direction,
                                  REF_DBL *flux);
REF_FCN REF_STATUS ref_phys_euler_jac(REF_DBL *state, REF_DBL *direction,
                                      REF_DBL *dflux_dcons);
REF_STATUS viscosity_law(REF_DBL nondim_temperature,
                         REF_DBL reference_temperature_k, REF_DBL *mu);
REF_FCN REF_STATUS ref_phys_viscous(REF_DBL *state, REF_DBL *grad, REF_DBL turb,
                                    REF_DBL mach, REF_DBL re,
                                    REF_DBL reference_temp, REF_DBL *dir,
                                    REF_DBL *flux);
REF_FCN REF_STATUS ref_phys_mut_sa(REF_DBL turb, REF_DBL rho, REF_DBL nu,
                                   REF_DBL *mut_sa);
REF_FCN REF_STATUS ref_phys_convdiff(REF_DBL *state, REF_DBL *grad,
                                     REF_DBL diffusivity, REF_DBL *dir,
                                     REF_DBL *flux);

REF_FCN REF_STATUS ref_phys_euler_dual_flux(REF_GRID ref_grid, REF_INT ldim,
                                            REF_DBL *primitive_dual,
                                            REF_DBL *dual_flux);
REF_FCN REF_STATUS ref_phys_mask_strong_bcs(REF_GRID ref_grid,
                                            REF_DICT ref_dict,
                                            REF_BOOL *replace, REF_INT ldim);

REF_FCN REF_STATUS ref_phys_read_mapbc(REF_DICT ref_dict,
                                       const char *mapbc_filename);
REF_FCN REF_STATUS ref_phys_read_mapbc_token(REF_DICT ref_dict,
                                             const char *mapbc_filename,
                                             const char *token);
REF_FCN REF_STATUS ref_phys_parse_tags(REF_DICT ref_dict, const char *tags);
REF_FCN REF_STATUS ref_phys_av_tag_attributes(REF_DICT ref_dict,
                                              REF_GEOM ref_geom);

REF_FCN REF_STATUS ref_phys_cc_fv_res(REF_GRID ref_grid, REF_INT nequ,
                                      REF_DBL *flux, REF_DBL *res);
REF_FCN REF_STATUS ref_phys_cc_fv_embed(REF_GRID ref_grid, REF_INT nequ,
                                        REF_DBL *flux, REF_DBL *res);

REF_FCN REF_STATUS ref_phys_spalding_yplus(REF_DBL uplus, REF_DBL *yplus);
REF_FCN REF_STATUS ref_phys_spalding_dyplus_duplus(REF_DBL uplus,
                                                   REF_DBL *dyplus_duplus);
REF_FCN REF_STATUS ref_phys_spalding_uplus(REF_DBL yplus, REF_DBL *uplus);

REF_FCN REF_STATUS ref_phys_yplus_dist(REF_DBL mach, REF_DBL re,
                                       REF_DBL reference_t_k, REF_DBL rho,
                                       REF_DBL t, REF_DBL y, REF_DBL u,
                                       REF_DBL *yplus_dist);
REF_FCN REF_STATUS ref_phys_u_tau(REF_DBL y, REF_DBL u, REF_DBL nu_mach_re,
                                  REF_DBL *u_tau);

REF_FCN REF_STATUS ref_phys_yplus_lengthscale(REF_GRID ref_grid, REF_DBL mach,
                                              REF_DBL re, REF_DBL reference_t_k,
                                              REF_INT ldim, REF_DBL *field,
                                              REF_DBL *lengthscale);
REF_FCN REF_STATUS ref_phys_normal_spacing(REF_GRID ref_grid,
                                           REF_DBL *normalspacing);
REF_FCN REF_STATUS ref_phys_yplus_metric(REF_GRID ref_grid, REF_DBL *metric,
                                         REF_DBL mach, REF_DBL re,
                                         REF_DBL temperature, REF_DBL target,
                                         REF_INT ldim, REF_DBL *ref_field,
                                         REF_DICT ref_dict_bcs);
REF_FCN REF_STATUS ref_phys_strong_sensor_bc(REF_GRID ref_grid, REF_DBL *scalar,
                                             REF_DBL strong_value,
                                             REF_DICT ref_dict_bcs);
REF_FCN REF_STATUS ref_phys_minspac(REF_DBL reynolds_number, REF_DBL *yplus1);

REF_FCN REF_STATUS ref_phys_sa_surrogate(REF_DBL wall_distance,
                                         REF_DBL *nu_tilde);

REF_FCN REF_STATUS ref_phys_signed_distance(REF_GRID ref_grid, REF_DBL *field,
                                            REF_DBL *distance);

REF_FCN REF_BOOL ref_phys_wall_distance_bc(REF_INT bc);
REF_FCN REF_STATUS ref_phys_wall_distance_static(REF_GRID ref_grid,
                                                 REF_DICT ref_dict,
                                                 REF_DBL *distance);
REF_FCN REF_STATUS ref_phys_wall_distance(REF_GRID ref_grid, REF_DICT ref_dict,
                                          REF_DBL *distance);

REF_FCN REF_STATUS ref_phys_ddes_blend(REF_DBL mach, REF_DBL reynolds_number,
                                       REF_DBL sqrt_vel_grad_dot_grad,
                                       REF_DBL distance, REF_DBL nu,
                                       REF_DBL *fd);

END_C_DECLORATION

#endif /* REF_PHYS_H */
