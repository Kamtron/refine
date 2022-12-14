
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

#ifndef REF_RECON_H
#define REF_RECON_H

#include "ref_defs.h"

BEGIN_C_DECLORATION
typedef enum REF_RECON_RECONSTRUCTIONS { /* 0 */ REF_RECON_L2PROJECTION,
                                         /* 1 */ REF_RECON_KEXACT,
                                         /* 2 */ REF_RECON_LAST
} REF_RECON_RECONSTRUCTION;
END_C_DECLORATION

#include "ref_cloud.h"
#include "ref_grid.h"
#include "ref_node.h"

BEGIN_C_DECLORATION

/* public for one-ring/plugin-refine */
REF_FCN REF_STATUS ref_recon_l2_projection_grad(REF_GRID ref_grid,
                                                REF_DBL *scalar, REF_DBL *grad);

REF_FCN REF_STATUS ref_recon_gradient(REF_GRID ref_grid, REF_DBL *scalar,
                                      REF_DBL *grad,
                                      REF_RECON_RECONSTRUCTION recon);
REF_FCN REF_STATUS ref_recon_signed_hessian(REF_GRID ref_grid, REF_DBL *scalar,
                                            REF_DBL *hessian,
                                            REF_RECON_RECONSTRUCTION recon);
REF_FCN REF_STATUS ref_recon_hessian(REF_GRID ref_grid, REF_DBL *scalar,
                                     REF_DBL *hessian,
                                     REF_RECON_RECONSTRUCTION recon);

REF_FCN REF_STATUS ref_recon_extrapolate_zeroth(REF_GRID ref_grid,
                                                REF_DBL *recon,
                                                REF_BOOL *replace,
                                                REF_INT ldim);
REF_FCN REF_STATUS ref_recon_extrapolate_kexact(REF_GRID ref_grid,
                                                REF_DBL *recon,
                                                REF_BOOL *replace,
                                                REF_INT ldim);

/* for testing */
REF_FCN REF_STATUS ref_recon_abs_value_hessian(REF_GRID ref_grid,
                                               REF_DBL *hessian);
REF_FCN REF_STATUS ref_recon_mask_tri(REF_GRID ref_grid, REF_BOOL *replace,
                                      REF_INT ldim);
REF_FCN REF_STATUS ref_recon_ghost_cloud(REF_CLOUD *one_layer,
                                         REF_NODE ref_node);

/* eliminate */
REF_FCN REF_STATUS ref_recon_roundoff_limit(REF_DBL *recon, REF_GRID ref_grid);
REF_FCN REF_STATUS ref_recon_max_jump_limit(REF_DBL *recon, REF_GRID ref_grid,
                                            REF_DBL max_jump);

REF_FCN REF_STATUS ref_recon_normal(REF_GRID ref_grid, REF_INT node,
                                    REF_DBL *normal);
REF_FCN REF_STATUS ref_recon_rsn(REF_GRID ref_grid, REF_INT node, REF_DBL *r,
                                 REF_DBL *s, REF_DBL *n);
REF_FCN REF_STATUS ref_recon_kexact_rs(REF_GLOB center_global,
                                       REF_CLOUD ref_cloud, REF_DBL *rn,
                                       REF_DBL *sn, REF_DBL *hessian);
REF_FCN REF_STATUS ref_recon_rsn_hess(REF_GRID ref_grid, REF_DBL *scalar,
                                      REF_DBL *hessian);
REF_FCN REF_STATUS ref_recon_rsn_hess_face(REF_GRID ref_grid, REF_DBL *hessian);

END_C_DECLORATION

#endif /* REF_RECON_H */
