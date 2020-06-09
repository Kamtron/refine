

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_EGADS
#include "egads.h"
#endif

#include "ref_cell.h"
#include "ref_dict.h"
#include "ref_egads.h"
#include "ref_export.h"
#include "ref_gather.h"
#include "ref_geom.h"
#include "ref_grid.h"
#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_matrix.h"
#include "ref_meshlink.h"
#include "ref_mpi.h"
#include "ref_node.h"
#include "ref_sort.h"

REF_STATUS ref_geom_initialize(REF_GEOM ref_geom) {
  REF_INT geom;
  ref_geom_n(ref_geom) = 0;
  for (geom = 0; geom < ref_geom_max(ref_geom); geom++) {
    ref_geom_type(ref_geom, geom) = REF_EMPTY;
    ref_geom_id(ref_geom, geom) = geom + 1;
  }
  ref_geom_id(ref_geom, ref_geom_max(ref_geom) - 1) = REF_EMPTY;
  ref_geom_blank(ref_geom) = 0;
  if (NULL != (void *)(ref_geom->ref_adj))
    RSS(ref_adj_free(ref_geom->ref_adj), "free to prevent leak");
  RSS(ref_adj_create(&(ref_geom->ref_adj)), "create ref_adj for ref_geom");

  return REF_SUCCESS;
}

REF_STATUS ref_geom_create(REF_GEOM *ref_geom_ptr) {
  REF_GEOM ref_geom;

  (*ref_geom_ptr) = NULL;

  ref_malloc(*ref_geom_ptr, 1, REF_GEOM_STRUCT);

  ref_geom = (*ref_geom_ptr);

  ref_geom_max(ref_geom) = 10;

  ref_malloc(ref_geom->descr, REF_GEOM_DESCR_SIZE * ref_geom_max(ref_geom),
             REF_INT);
  ref_malloc(ref_geom->param, 2 * ref_geom_max(ref_geom), REF_DBL);
  ref_geom->ref_adj = (REF_ADJ)NULL;
  RSS(ref_geom_initialize(ref_geom), "init geom list");

  ref_geom->uv_area_sign = NULL;
  ref_geom->initial_cell_height = NULL;
  ref_geom->face_min_length = NULL;
  ref_geom->face_seg_per_rad = NULL;
  ref_geom->segments_per_radian_of_curvature = 2.0;
  ref_geom->tolerance_protection = 100.0;
  ref_geom->gap_protection = 10.0;

  ref_geom->nnode = REF_EMPTY;
  ref_geom->nedge = REF_EMPTY;
  ref_geom->nface = REF_EMPTY;
  ref_geom->manifold = REF_TRUE;
  ref_geom->context = NULL;
  RSS(ref_egads_open(ref_geom), "open egads");
  ref_geom->solid = NULL;
  ref_geom->faces = NULL;
  ref_geom->edges = NULL;
  ref_geom->nodes = NULL;

  ref_geom->cad_data_size = 0;
  ref_geom->cad_data = (REF_BYTE *)NULL;

  ref_geom->meshlink = NULL;
  ref_geom->meshlink_projection = NULL;

  return REF_SUCCESS;
}

REF_STATUS ref_geom_free(REF_GEOM ref_geom) {
  if (NULL == (void *)ref_geom) return REF_NULL;
  ref_free(ref_geom->cad_data);
  RSS(ref_egads_close(ref_geom), "open egads");
  RSS(ref_adj_free(ref_geom->ref_adj), "adj free");
  ref_free(ref_geom->face_seg_per_rad);
  ref_free(ref_geom->face_min_length);
  ref_free(ref_geom->initial_cell_height);
  ref_free(ref_geom->uv_area_sign);
  ref_free(ref_geom->param);
  ref_free(ref_geom->descr);
  ref_free(ref_geom);
  return REF_SUCCESS;
}

REF_STATUS ref_geom_deep_copy(REF_GEOM *ref_geom_ptr, REF_GEOM original) {
  REF_GEOM ref_geom;
  REF_INT geom, i;
  (*ref_geom_ptr) = NULL;

  ref_malloc(*ref_geom_ptr, 1, REF_GEOM_STRUCT);

  ref_geom = (*ref_geom_ptr);

  ref_geom_n(ref_geom) = ref_geom_n(original);
  ref_geom_max(ref_geom) = ref_geom_max(original);

  ref_malloc(ref_geom->descr, REF_GEOM_DESCR_SIZE * ref_geom_max(ref_geom),
             REF_INT);
  ref_malloc(ref_geom->param, 2 * ref_geom_max(ref_geom), REF_DBL);
  ref_geom->uv_area_sign = NULL;
  ref_geom->initial_cell_height = NULL;
  ref_geom->face_min_length = NULL;
  ref_geom->face_seg_per_rad = NULL;
  ref_geom->segments_per_radian_of_curvature =
      original->segments_per_radian_of_curvature;
  ref_geom->tolerance_protection = original->tolerance_protection;
  ref_geom->gap_protection = original->gap_protection;

  for (geom = 0; geom < ref_geom_max(ref_geom); geom++)
    for (i = 0; i < REF_GEOM_DESCR_SIZE; i++)
      ref_geom_descr(ref_geom, i, geom) = ref_geom_descr(original, i, geom);
  ref_geom_blank(ref_geom) = ref_geom_blank(original);
  for (geom = 0; geom < ref_geom_max(ref_geom); geom++)
    for (i = 0; i < 2; i++)
      ref_geom_param(ref_geom, i, geom) = ref_geom_param(original, i, geom);

  RSS(ref_adj_deep_copy(&(ref_geom->ref_adj), original->ref_adj),
      "deep copy ref_adj for ref_geom");

  ref_geom->nnode = REF_EMPTY;
  ref_geom->nedge = REF_EMPTY;
  ref_geom->nface = REF_EMPTY;
  ref_geom->manifold = original->manifold;
  ref_geom->context = NULL;
  ref_geom->solid = NULL;
  ref_geom->faces = NULL;
  ref_geom->edges = NULL;
  ref_geom->nodes = NULL;

  ref_geom->cad_data_size = 0;
  ref_geom->cad_data = (REF_BYTE *)NULL;

  ref_geom->meshlink = NULL;
  ref_geom->meshlink_projection = NULL;

  return REF_SUCCESS;
}

REF_STATUS ref_geom_pack(REF_GEOM ref_geom, REF_INT *o2n) {
  REF_INT geom, compact, i;
  compact = 0;
  each_ref_geom(ref_geom, geom) {
    for (i = 0; i < REF_GEOM_DESCR_SIZE; i++)
      ref_geom_descr(ref_geom, i, compact) = ref_geom_descr(ref_geom, i, geom);
    ref_geom_node(ref_geom, compact) = o2n[ref_geom_node(ref_geom, geom)];
    for (i = 0; i < 2; i++)
      ref_geom_param(ref_geom, i, compact) = ref_geom_param(ref_geom, i, geom);
    compact++;
  }
  REIS(compact, ref_geom_n(ref_geom), "count mismatch");
  if (ref_geom_n(ref_geom) < ref_geom_max(ref_geom)) {
    for (geom = ref_geom_n(ref_geom); geom < ref_geom_max(ref_geom); geom++) {
      ref_geom_type(ref_geom, geom) = REF_EMPTY;
      ref_geom_id(ref_geom, geom) = geom + 1;
    }
    ref_geom_id(ref_geom, ref_geom_max(ref_geom) - 1) = REF_EMPTY;
    ref_geom_blank(ref_geom) = ref_geom_n(ref_geom);
  } else {
    ref_geom_blank(ref_geom) = REF_EMPTY;
  }
  RSS(ref_adj_free(ref_geom->ref_adj), "free to prevent leak");
  RSS(ref_adj_create(&(ref_geom->ref_adj)), "create ref_adj for ref_geom");

  each_ref_geom(ref_geom, geom) {
    RSS(ref_adj_add(ref_geom->ref_adj, ref_geom_node(ref_geom, geom), geom),
        "register geom");
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_uv_area(REF_GEOM ref_geom, REF_INT *nodes,
                            REF_DBL *uv_area) {
  REF_DBL uv0[2], uv1[2], uv2[2];
  REF_INT sens;
  RSS(ref_geom_cell_tuv(ref_geom, nodes[0], nodes, REF_GEOM_FACE, uv0, &sens),
      "uv0");
  RSS(ref_geom_cell_tuv(ref_geom, nodes[1], nodes, REF_GEOM_FACE, uv1, &sens),
      "uv1");
  RSS(ref_geom_cell_tuv(ref_geom, nodes[2], nodes, REF_GEOM_FACE, uv2, &sens),
      "uv2");
  *uv_area = 0.5 * (-uv1[0] * uv0[1] + uv2[0] * uv0[1] + uv0[0] * uv1[1] -
                    uv2[0] * uv1[1] - uv0[0] * uv2[1] + uv1[0] * uv2[1]);
  return REF_SUCCESS;
}

REF_STATUS ref_geom_uv_area_sign(REF_GRID ref_grid, REF_INT id, REF_DBL *sign) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  if (NULL == ((ref_geom)->uv_area_sign)) {
    REF_CELL ref_cell = ref_grid_tri(ref_grid);
    REF_INT face;
    REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];
    REF_DBL uv_area;
    if (REF_EMPTY == ref_geom->nface)
      RSS(ref_geom_infer_nedge_nface(ref_grid), "infer counts");
    ref_malloc_init(ref_geom->uv_area_sign, ref_geom->nface, REF_DBL, 0.0);
    each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
      face = nodes[3];
      if (face < 1 || ref_geom->nface < face) continue;
      RSS(ref_geom_uv_area(ref_geom, nodes, &uv_area), "uv area");
      if (uv_area < 0.0) {
        ((ref_geom)->uv_area_sign)[face - 1] -= 1.0;
      } else {
        ((ref_geom)->uv_area_sign)[face - 1] += 1.0;
      }
    }
    for (face = 0; face < ref_geom->nface; face++) {
      if (((ref_geom)->uv_area_sign)[face] < 0.0) {
        ((ref_geom)->uv_area_sign)[face] = -1.0;
      } else {
        ((ref_geom)->uv_area_sign)[face] = 1.0;
      }
    }
  }

  if (id < 1 || id > ref_geom->nface) return REF_INVALID;
  *sign = ((ref_geom)->uv_area_sign)[id - 1];

  return REF_SUCCESS;
}

REF_STATUS ref_geom_uv_area_report(REF_GRID ref_grid) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT geom, id, min_id, max_id;
  REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL no_cell;
  REF_DBL uv_area, total_uv_area, min_uv_area, max_uv_area, sign_uv_area;
  REF_INT n_neg, n_pos;

  min_id = REF_INT_MAX;
  max_id = REF_INT_MIN;
  each_ref_geom_face(ref_geom, geom) {
    min_id = MIN(min_id, ref_geom_id(ref_geom, geom));
    max_id = MAX(max_id, ref_geom_id(ref_geom, geom));
  }

  for (id = min_id; id <= max_id; id++) {
    no_cell = REF_TRUE;
    total_uv_area = 0.0;
    min_uv_area = 0.0;
    max_uv_area = 0.0;
    n_neg = 0;
    n_pos = 0;
    each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
      if (id == nodes[3]) {
        RSS(ref_geom_uv_area(ref_geom, nodes, &uv_area), "uv area");
        total_uv_area += uv_area;
        if (no_cell) {
          min_uv_area = uv_area;
          max_uv_area = uv_area;
          no_cell = REF_FALSE;
        } else {
          min_uv_area = MIN(min_uv_area, uv_area);
          max_uv_area = MAX(max_uv_area, uv_area);
        }
        if (uv_area < 0.0) {
          n_neg++;
        } else {
          n_pos++;
        }
      }
    }
    if (!no_cell) {
      RSS(ref_geom_uv_area_sign(ref_grid, id, &sign_uv_area), "sign");
      printf("face%5d: %4.1f %9.2e total (%10.3e,%10.3e) %d + %d -\n", id,
             sign_uv_area, total_uv_area, min_uv_area, max_uv_area, n_pos,
             n_neg);
    }
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_inspect(REF_GEOM ref_geom) {
  REF_INT geom;
  printf("ref_geom = %p\n", (void *)ref_geom);
  printf(" n = %d, max = %d\n", ref_geom_n(ref_geom), ref_geom_max(ref_geom));
  for (geom = 0; geom < ref_geom_max(ref_geom); geom++) {
    switch (ref_geom_type(ref_geom, geom)) {
      case REF_GEOM_NODE:
        printf("%d node: %d id, %d jump, %d degen, %d global\n", geom,
               ref_geom_id(ref_geom, geom), ref_geom_jump(ref_geom, geom),
               ref_geom_degen(ref_geom, geom), ref_geom_node(ref_geom, geom));
        break;
      case REF_GEOM_EDGE:
        printf("%d edge: %d id, %d jump, %d degen, %d global, t=%e\n", geom,
               ref_geom_id(ref_geom, geom), ref_geom_jump(ref_geom, geom),
               ref_geom_node(ref_geom, geom), ref_geom_degen(ref_geom, geom),
               ref_geom_param(ref_geom, 0, geom));
        break;
      case REF_GEOM_FACE:
        printf("%d face: %d id, %d jump, %d degen, %d global, uv= %e %e\n",
               geom, ref_geom_id(ref_geom, geom), ref_geom_jump(ref_geom, geom),
               ref_geom_node(ref_geom, geom), ref_geom_degen(ref_geom, geom),
               ref_geom_param(ref_geom, 0, geom),
               ref_geom_param(ref_geom, 1, geom));
        break;
    }
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_tattle(REF_GEOM ref_geom, REF_INT node) {
  REF_INT item, geom;

  printf(" tattle on node = %d\n", node);
  each_ref_adj_node_item_with_ref(ref_geom_adj(ref_geom), node, item, geom) {
    switch (ref_geom_type(ref_geom, geom)) {
      case REF_GEOM_NODE:
        printf("%d node: %d id, %d jump, %d degen, %d global\n", geom,
               ref_geom_id(ref_geom, geom), ref_geom_jump(ref_geom, geom),
               ref_geom_degen(ref_geom, geom), ref_geom_node(ref_geom, geom));
        break;
      case REF_GEOM_EDGE:
        printf("%d edge: %d id, %d jump, %d degen, %d global, t=%e\n", geom,
               ref_geom_id(ref_geom, geom), ref_geom_jump(ref_geom, geom),
               ref_geom_degen(ref_geom, geom), ref_geom_node(ref_geom, geom),
               ref_geom_param(ref_geom, 0, geom));
        break;
      case REF_GEOM_FACE:
        printf("%d face: %d id, %d jump, %d degen, %d global, uv= %e %e\n",
               geom, ref_geom_id(ref_geom, geom), ref_geom_jump(ref_geom, geom),
               ref_geom_degen(ref_geom, geom), ref_geom_node(ref_geom, geom),
               ref_geom_param(ref_geom, 0, geom),
               ref_geom_param(ref_geom, 1, geom));
        break;
    }
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_supported(REF_GEOM ref_geom, REF_INT node,
                              REF_BOOL *has_support) {
  *has_support = !ref_adj_empty(ref_geom_adj(ref_geom), node);
  return REF_SUCCESS;
}

REF_STATUS ref_geom_tri_supported(REF_GEOM ref_geom, REF_INT *nodes,
                                  REF_BOOL *has_support) {
  REF_INT node, id, geom;
  REF_STATUS status;
  *has_support = REF_FALSE;

  node = nodes[0];
  id = nodes[3];
  status = ref_geom_find(ref_geom, node, REF_GEOM_FACE, id, &geom);
  if (REF_NOT_FOUND == status) { /* no geom support */
    *has_support = REF_FALSE;
    return REF_SUCCESS;
  }
  RSS(status, "error testing geom support");
  *has_support = REF_TRUE;
  return REF_SUCCESS;
}

static REF_STATUS ref_geom_grow(REF_GEOM ref_geom) {
  REF_INT geom;
  REF_INT orig, chunk;
  REF_INT max_limit = REF_INT_MAX / 3;

  if (REF_EMPTY != ref_geom_blank(ref_geom)) {
    return REF_SUCCESS;
  }

  RAS(ref_geom_max(ref_geom) != max_limit,
      "the number of geoms is too large for integers, cannot grow");
  orig = ref_geom_max(ref_geom);
  /* geometric growth for efficiency */
  chunk = MAX(1000, (REF_INT)(1.5 * (REF_DBL)orig));

  /* try to keep under 32-bit limit */
  RAS(max_limit - orig > 0, "chunk limit at max");
  chunk = MIN(chunk, max_limit - orig);

  ref_geom_max(ref_geom) = orig + chunk;

  ref_realloc(ref_geom->descr, REF_GEOM_DESCR_SIZE * ref_geom_max(ref_geom),
              REF_INT);
  ref_realloc(ref_geom->param, 2 * ref_geom_max(ref_geom), REF_DBL);

  for (geom = orig; geom < ref_geom_max(ref_geom); geom++) {
    ref_geom_type(ref_geom, geom) = REF_EMPTY;
    ref_geom_id(ref_geom, geom) = geom + 1;
  }
  ref_geom_id(ref_geom, ref_geom_max(ref_geom) - 1) = REF_EMPTY;
  ref_geom_blank(ref_geom) = orig;

  return REF_SUCCESS;
}

REF_STATUS ref_geom_add_with_descr(REF_GEOM ref_geom, REF_INT *descr,
                                   REF_DBL *param) {
  REF_INT type, id, gref, jump, degen, node, geom;
  type = descr[REF_GEOM_DESCR_TYPE];
  id = descr[REF_GEOM_DESCR_ID];
  gref = descr[REF_GEOM_DESCR_GREF];
  jump = descr[REF_GEOM_DESCR_JUMP];
  degen = descr[REF_GEOM_DESCR_DEGEN];
  node = descr[REF_GEOM_DESCR_NODE];
  RSS(ref_geom_add(ref_geom, node, type, id, param), "geom add");
  RSS(ref_geom_find(ref_geom, node, type, id, &geom), "geom find");
  ref_geom_gref(ref_geom, geom) = gref;
  ref_geom_degen(ref_geom, geom) = degen;
  ref_geom_jump(ref_geom, geom) = jump;
  return REF_SUCCESS;
}
REF_STATUS ref_geom_add(REF_GEOM ref_geom, REF_INT node, REF_INT type,
                        REF_INT id, REF_DBL *param) {
  REF_INT geom;
  REF_STATUS status;

  if (type < 0 || 2 < type) return REF_INVALID;

  status = ref_geom_find(ref_geom, node, type, id, &geom);
  RXS(status, REF_NOT_FOUND, "find failed");

  if (REF_SUCCESS == status) {
    if (type > 0) ref_geom_param(ref_geom, 0, geom) = param[0];
    if (type > 1) ref_geom_param(ref_geom, 1, geom) = param[1];
    return REF_SUCCESS;
  }

  if (REF_EMPTY == ref_geom_blank(ref_geom)) {
    RSS(ref_geom_grow(ref_geom), "grow add");
  }

  geom = ref_geom_blank(ref_geom);
  ref_geom_blank(ref_geom) = ref_geom_id(ref_geom, geom);

  ref_geom_type(ref_geom, geom) = type;
  ref_geom_id(ref_geom, geom) = id;
  ref_geom_gref(ref_geom, geom) = id; /* assume same until set */
  ref_geom_jump(ref_geom, geom) = 0;
  ref_geom_degen(ref_geom, geom) = 0;
  ref_geom_node(ref_geom, geom) = node;

  ref_geom_param(ref_geom, 0, geom) = 0.0;
  ref_geom_param(ref_geom, 1, geom) = 0.0;
  if (type > 0) ref_geom_param(ref_geom, 0, geom) = param[0];
  if (type > 1) ref_geom_param(ref_geom, 1, geom) = param[1];

  RSB(ref_adj_add(ref_geom->ref_adj, node, geom), "register geom", {
    printf("register node %d geom %d type %d id %d\n",
           ref_geom_node(ref_geom, geom), geom, ref_geom_type(ref_geom, geom),
           ref_geom_id(ref_geom, geom));
  });

  ref_geom_n(ref_geom)++;

  return REF_SUCCESS;
}

REF_STATUS ref_geom_remove_all(REF_GEOM ref_geom, REF_INT node) {
  REF_ADJ ref_adj = ref_geom_adj(ref_geom);
  REF_INT item, geom;

  item = ref_adj_first(ref_adj, node);
  while (ref_adj_valid(item)) {
    geom = ref_adj_item_ref(ref_adj, item);
    RSS(ref_adj_remove(ref_adj, node, geom), "unregister geom");

    ref_geom_type(ref_geom, geom) = REF_EMPTY;
    ref_geom_id(ref_geom, geom) = ref_geom_blank(ref_geom);
    ref_geom_blank(ref_geom) = geom;
    ref_geom_n(ref_geom)--;

    item = ref_adj_first(ref_adj, node);
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_is_a(REF_GEOM ref_geom, REF_INT node, REF_INT type,
                         REF_BOOL *it_is) {
  REF_INT item, geom;
  *it_is = REF_FALSE;
  each_ref_adj_node_item_with_ref(ref_geom_adj(ref_geom), node, item, geom) {
    if (type == ref_geom_type(ref_geom, geom)) {
      *it_is = REF_TRUE;
      return REF_SUCCESS;
    }
  }
  return REF_SUCCESS;
}

REF_STATUS ref_geom_unique_id(REF_GEOM ref_geom, REF_INT node, REF_INT type,
                              REF_INT *id) {
  REF_INT item, geom;
  REF_BOOL found_one;
  found_one = REF_FALSE;
  each_ref_adj_node_item_with_ref(ref_geom_adj(ref_geom), node, item, geom) {
    if (type == ref_geom_type(ref_geom, geom)) {
      if (found_one) return REF_INVALID; /* second one makes invalid */
      found_one = REF_TRUE;
      *id = ref_geom_id(ref_geom, geom);
    }
  }
  if (found_one) return REF_SUCCESS;
  return REF_NOT_FOUND;
}

REF_STATUS ref_geom_find(REF_GEOM ref_geom, REF_INT node, REF_INT type,
                         REF_INT id, REF_INT *found) {
  REF_INT item, geom;
  *found = REF_EMPTY;
  each_ref_adj_node_item_with_ref(ref_geom_adj(ref_geom), node, item, geom) {
    if (type == ref_geom_type(ref_geom, geom) &&
        id == ref_geom_id(ref_geom, geom)) {
      *found = geom;
      return REF_SUCCESS;
    }
  }
  return REF_NOT_FOUND;
}

REF_STATUS ref_geom_tuv(REF_GEOM ref_geom, REF_INT node, REF_INT type,
                        REF_INT id, REF_DBL *param) {
  REF_INT geom;

  RSS(ref_geom_find(ref_geom, node, type, id, &geom), "not found");

  REIS(0, ref_geom_jump(ref_geom, geom), "use ref_geom_cell_tuv for jumps");
  REIS(0, ref_geom_degen(ref_geom, geom), "use ref_geom_cell_tuv for degen");

  if (type > 0) param[0] = ref_geom_param(ref_geom, 0, geom);
  if (type > 1) param[1] = ref_geom_param(ref_geom, 1, geom);

  return REF_SUCCESS;
}

REF_STATUS ref_geom_cell_tuv_supported(REF_GEOM ref_geom, REF_INT *nodes,
                                       REF_INT type, REF_BOOL *supported) {
  REF_INT node_per;
  REF_INT id, geom0, geom1, geom2;
  REF_BOOL tri_supported;

  *supported = REF_TRUE;

  RAS(1 <= type && type <= 2, "type not allowed");
  node_per = type + 1;
  id = nodes[node_per];

  /* protects unsupported meshlink tri */
  if (REF_GEOM_FACE == type) {
    RSS(ref_geom_tri_supported(ref_geom, nodes, &tri_supported), "tri support");
    if (!tri_supported) { /* no geom support */
      *supported = 1.0;
      return REF_SUCCESS;
    }
  }

  switch (type) {
    case REF_GEOM_EDGE:
      RSS(ref_geom_find(ref_geom, nodes[0], type, id, &geom0), "not found");
      RSS(ref_geom_find(ref_geom, nodes[1], type, id, &geom1), "not found");

      if ((0 != ref_geom_jump(ref_geom, geom0) ||
           0 != ref_geom_degen(ref_geom, geom0)) &&
          (0 != ref_geom_jump(ref_geom, geom1) ||
           0 != ref_geom_degen(ref_geom, geom1))) {
        *supported = REF_FALSE;
      }
      break;
    case REF_GEOM_FACE:
      RSS(ref_geom_find(ref_geom, nodes[0], type, id, &geom0), "not found");
      RSS(ref_geom_find(ref_geom, nodes[1], type, id, &geom1), "not found");
      RSS(ref_geom_find(ref_geom, nodes[2], type, id, &geom2), "not found");
      if ((0 != ref_geom_jump(ref_geom, geom0) ||
           0 != ref_geom_degen(ref_geom, geom0)) &&
          (0 != ref_geom_jump(ref_geom, geom1) ||
           0 != ref_geom_degen(ref_geom, geom1)) &&
          (0 != ref_geom_jump(ref_geom, geom2) ||
           0 != ref_geom_degen(ref_geom, geom2))) {
        *supported = REF_FALSE;
      }
      break;
    default:
      RSS(REF_IMPLEMENT, "can't to geom type yet");
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_cell_tuv(REF_GEOM ref_geom, REF_INT node, REF_INT *nodes,
                             REF_INT type, REF_DBL *param, REF_INT *sens) {
#ifdef HAVE_EGADS
  REF_INT node_per;
  REF_INT id, edgeid, geom, from, from_geom;
  REF_INT node_index, cell_node;
  ego face_ego, edge_ego;
  double trange[2], uv[2], uv0[2], uv1[2], uvtmin[2], uvtmax[2];
  int periodic;
  REF_DBL from_param[2], t;
  REF_DBL dist0, dist1;
  REF_INT hits;

  RAS(1 <= type && type <= 2, "type not allowed");
  node_per = type + 1;
  id = nodes[node_per];
  node_index = REF_EMPTY;
  for (cell_node = 0; cell_node < node_per; cell_node++) {
    if (node == nodes[cell_node]) {
      REIS(REF_EMPTY, node_index, "node found twice in nodes");
      node_index = cell_node;
    }
  }
  RAS(REF_EMPTY != node_index, "node not found in nodes");

  RSB(ref_geom_find(ref_geom, node, type, id, &geom), "not found", {
    printf(" %d type %d id\n", type, id);
    ref_geom_tattle(ref_geom, node);
  });

  if (0 == ref_geom_jump(ref_geom, geom) &&
      0 == ref_geom_degen(ref_geom, geom)) {
    if (type > 0) param[0] = ref_geom_param(ref_geom, 0, geom);
    if (type > 1) param[1] = ref_geom_param(ref_geom, 1, geom);
    *sens = 0;
    return REF_SUCCESS;
  }

  switch (type) {
    case REF_GEOM_EDGE:
      edge_ego = ((ego *)(ref_geom->edges))[id - 1];
      REIS(EGADS_SUCCESS, EG_getRange(edge_ego, trange, &periodic),
           "edge range");
      from = nodes[1 - node_index];
      RSS(ref_geom_tuv(ref_geom, from, type, id, from_param), "from tuv");
      dist0 = from_param[0] - trange[0];
      dist1 = trange[1] - from_param[0];
      if (dist0 < 0.0 || dist1 < 0.0) {
        printf(" from t = %e %e %e, dist = %e %e\n", trange[0], from_param[0],
               trange[1], dist0, dist1);
        THROW("from node not in trange");
      }
      if (dist0 < dist1) {
        *sens = 1;
        param[0] = trange[0];
      } else {
        *sens = -1;
        param[0] = trange[1];
      }
      break;
    case REF_GEOM_FACE:
      if (0 == ref_geom_degen(ref_geom, geom)) {
        from = REF_EMPTY;
        for (cell_node = 0; cell_node < node_per; cell_node++) {
          RSS(ref_geom_find(ref_geom, nodes[cell_node], type, id, &from_geom),
              "not found");
          if (node_index != cell_node &&
              0 == ref_geom_jump(ref_geom, from_geom) &&
              0 == ref_geom_degen(ref_geom, from_geom)) {
            from = nodes[cell_node];
          }
        }
        RAB(REF_EMPTY != from, "can't find from tuv in tri cell", {
          ref_geom_tattle(ref_geom, nodes[0]);
          ref_geom_tattle(ref_geom, nodes[1]);
          ref_geom_tattle(ref_geom, nodes[2]);
          printf("faceid %d node %d node_index %d\n", id, node, node_index);
        });
        edgeid = ref_geom_jump(ref_geom, geom);
        RSS(ref_geom_tuv(ref_geom, from, REF_GEOM_FACE, id, uv), "from uv");
        RSS(ref_geom_tuv(ref_geom, node, REF_GEOM_EDGE, edgeid, &t), "edge t0");
        face_ego = ((ego *)(ref_geom->faces))[id - 1];
        edge_ego = ((ego *)(ref_geom->edges))[edgeid - 1];
        REIS(EGADS_SUCCESS, EG_getEdgeUV(face_ego, edge_ego, 1, t, uv0),
             "eval edge face uv sens = 1");
        REIS(EGADS_SUCCESS, EG_getEdgeUV(face_ego, edge_ego, -1, t, uv1),
             "eval edge face uv sens = -1");
        dist0 = sqrt(pow(uv0[0] - uv[0], 2) + pow(uv0[1] - uv[1], 2));
        dist1 = sqrt(pow(uv1[0] - uv[0], 2) + pow(uv1[1] - uv[1], 2));
        if (dist0 < dist1) {
          *sens = 1;
          param[0] = uv0[0];
          param[1] = uv0[1];
        } else {
          *sens = -1;
          param[0] = uv1[0];
          param[1] = uv1[1];
        }
      } else {
        uv0[0] = 0.0;
        uv0[1] = 0.0;
        hits = 0;
        for (cell_node = 0; cell_node < node_per; cell_node++) {
          RSS(ref_geom_find(ref_geom, nodes[cell_node], type, id, &from_geom),
              "not found");
          if (0 == ref_geom_jump(ref_geom, from_geom) &&
              0 == ref_geom_degen(ref_geom, from_geom)) {
            RSS(ref_geom_tuv(ref_geom, nodes[cell_node], REF_GEOM_FACE, id, uv),
                "from uv");
            uv0[0] += uv[0];
            uv0[1] += uv[1];
            hits++;
          }
        }
        RAS(0 < hits, "no seed uv found for DEGEN");
        uv0[0] /= (REF_DBL)hits;
        uv0[1] /= (REF_DBL)hits;

        *sens = 0;
        edgeid = ABS(ref_geom_degen(ref_geom, geom));
        edge_ego = ((ego *)(ref_geom->edges))[edgeid - 1];
        face_ego = ((ego *)(ref_geom->faces))[ref_geom_id(ref_geom, geom) - 1];
        /* returns -2 EGADS_NULLOBJ for EGADSlite of hemisphere
           REIB(EGADS_SUCCESS, EG_getRange(edge_ego, trange, &periodic),
           "edge trange", {
           printf("for edge %d (%p) face %d\n", edgeid, (void *)edge_ego,
           ref_geom_id(ref_geom, geom));
           });
        */
        /* use EG_getTopology as an alternate to EG_getRange */
        {
          ego ref, *pchldrn;
          int oclass, mtype, nchild, *psens;
          REIS(EGADS_SUCCESS,
               EG_getTopology(edge_ego, &ref, &oclass, &mtype, trange, &nchild,
                              &pchldrn, &psens),
               "topo");
        }
        REIB(EGADS_SUCCESS,
             EG_getEdgeUV(face_ego, edge_ego, *sens, trange[0], uvtmin),
             "edge uv tmin", {
               printf("for edge %d (%p) face %d (%p)\n", edgeid,
                      (void *)edge_ego, ref_geom_id(ref_geom, geom),
                      (void *)face_ego);
             });
        REIB(EGADS_SUCCESS,
             EG_getEdgeUV(face_ego, edge_ego, *sens, trange[1], uvtmax),
             "edge uv tmax", {
               printf("for edge %d face %d\n", edgeid,
                      ref_geom_id(ref_geom, geom));
             });

        /* edgeid sign convention defined in ref_geom_mark_jump_degen */
        if (0 < ref_geom_degen(ref_geom, geom)) {
          param[0] = ref_geom_param(ref_geom, 0, geom);
          param[1] = uv0[1];
          param[1] = MAX(param[1], MIN(uvtmin[1], uvtmax[1]));
          param[1] = MIN(param[1], MAX(uvtmin[1], uvtmax[1]));
        } else {
          param[0] = uv0[0];
          param[0] = MAX(param[0], MIN(uvtmin[0], uvtmax[0]));
          param[0] = MIN(param[0], MAX(uvtmin[0], uvtmax[0]));
          param[1] = ref_geom_param(ref_geom, 1, geom);
        }
      }
      break;
    default:
      RSS(REF_IMPLEMENT, "can't to geom type yet");
  }

#else
  *sens = 0;
  RSS(ref_geom_tuv(ref_geom, node, type, nodes[type + 1], param), "tuv");
#endif
  return REF_SUCCESS;
}

static REF_STATUS ref_geom_eval_edge_face_uv(REF_GRID ref_grid,
                                             REF_INT edge_geom) {
#ifdef HAVE_EGADS
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_ADJ ref_adj = ref_geom_adj(ref_geom);
  REF_INT node, cell_item, geom_item, cell, face_geom;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  double t;
  double uv[2], edgeuv[2], invuv[2], edgedist, invdist, edgexyz[18], invxyz[19];
  int sense;
  ego *edges, *faces;
  ego edge, face;
  REF_INT faceid;
  REF_BOOL have_jump;

  REF_BOOL verbose = REF_FALSE;

  if (edge_geom < 0 || ref_geom_max(ref_geom) <= edge_geom) return REF_INVALID;
  if (REF_GEOM_EDGE != ref_geom_type(ref_geom, edge_geom)) return REF_INVALID;

  t = ref_geom_param(ref_geom, 0, edge_geom);
  node = ref_geom_node(ref_geom, edge_geom);

  have_jump = REF_FALSE;
  each_ref_adj_node_item_with_ref(ref_adj, node, geom_item, face_geom) {
    if (REF_GEOM_FACE == ref_geom_type(ref_geom, face_geom)) {
      have_jump = have_jump || (0 != ref_geom_jump(ref_geom, face_geom));
    }
  }

  if (have_jump) {
    /* uv update at jump not needed, should always depend on cell_c2n */
    /* keeping for consistancy with non-jump */
    each_ref_cell_having_node(ref_cell, node, cell_item, cell) {
      RSS(ref_cell_nodes(ref_cell, cell, nodes), "cell nodes");
      faceid = nodes[3];
      RSS(ref_geom_cell_tuv(ref_geom, node, nodes, REF_GEOM_FACE, uv, &sense),
          "cell uv");
      if (1 == sense) { /* sense to use is arbitrary */
        each_ref_adj_node_item_with_ref(ref_adj, node, geom_item, face_geom) {
          if (REF_GEOM_FACE == ref_geom_type(ref_geom, face_geom) &&
              faceid == ref_geom_id(ref_geom, face_geom)) {
            ref_geom_param(ref_geom, 0, face_geom) = uv[0];
            ref_geom_param(ref_geom, 1, face_geom) = uv[1];
          }
        }
      }
    }
  } else {
    edges = (ego *)(ref_geom->edges);
    edge = edges[ref_geom_id(ref_geom, edge_geom) - 1];
    faces = (ego *)(ref_geom->faces);
    each_ref_adj_node_item_with_ref(ref_adj, node, geom_item, face_geom) {
      if (REF_GEOM_FACE == ref_geom_type(ref_geom, face_geom)) {
        faceid = ref_geom_id(ref_geom, face_geom);
        face = faces[faceid - 1];
        sense = 0;
        REIB(EGADS_SUCCESS, EG_getEdgeUV(face, edge, sense, t, edgeuv),
             "edge uv", {
               printf("edge %d face %d\n", ref_geom_id(ref_geom, edge_geom),
                      faceid);
               ref_geom_tattle(ref_geom, node);
             });
        if (REF_TRUE) { /* use edgeuv */
          ref_geom_param(ref_geom, 0, face_geom) = edgeuv[0];
          ref_geom_param(ref_geom, 1, face_geom) = edgeuv[1];
        } else { /* check if inverse eval on face is closer */
          invuv[0] = edgeuv[0];
          invuv[1] = edgeuv[1];
          RSS(ref_geom_inverse_eval(ref_geom, REF_GEOM_FACE, faceid,
                                    ref_node_xyz_ptr(ref_node, node), invuv),
              "inv wrapper");
          REIS(EGADS_SUCCESS, EG_evaluate(face, edgeuv, edgexyz), "EG eval");
          REIS(EGADS_SUCCESS, EG_evaluate(face, invuv, invxyz), "EG eval");
          edgedist = sqrt(pow(edgexyz[0] - ref_node_xyz(ref_node, 0, node), 2) +
                          pow(edgexyz[1] - ref_node_xyz(ref_node, 1, node), 2) +
                          pow(edgexyz[2] - ref_node_xyz(ref_node, 2, node), 2));
          invdist = sqrt(pow(invxyz[0] - ref_node_xyz(ref_node, 0, node), 2) +
                         pow(invxyz[1] - ref_node_xyz(ref_node, 1, node), 2) +
                         pow(invxyz[2] - ref_node_xyz(ref_node, 2, node), 2));
          if (edgedist <= invdist) {
            ref_geom_param(ref_geom, 0, face_geom) = edgeuv[0];
            ref_geom_param(ref_geom, 1, face_geom) = edgeuv[1];
          } else {
            if (verbose)
              printf("face eval %e closer than edgeUV %e\n", invdist, edgedist);
            ref_geom_param(ref_geom, 0, face_geom) = invuv[0];
            ref_geom_param(ref_geom, 1, face_geom) = invuv[1];
          }
        }
      }
    }
  }

  return REF_SUCCESS;
#else
  SUPRESS_UNUSED_COMPILER_WARNING(ref_grid);
  SUPRESS_UNUSED_COMPILER_WARNING(edge_geom);
  return REF_IMPLEMENT;
#endif
}

REF_STATUS ref_geom_xyz_between(REF_GRID ref_grid, REF_INT node0, REF_INT node1,
                                REF_DBL *xyz) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT type, id;
  REF_DBL param[2], param0[2], param1[2];
  REF_DBL uv_min[2], uv_max[2];
  REF_INT i;
  REF_BOOL support0, support1;
  REF_INT sense, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_STATUS status;
  REF_INT ncell, cells[2];

  for (i = 0; i < 3; i++)
    xyz[i] = 0.5 * (ref_node_xyz(ref_node, i, node0) +
                    ref_node_xyz(ref_node, i, node1));

  RSS(ref_geom_supported(ref_geom, node0, &support0), "node0 supported");
  RSS(ref_geom_supported(ref_geom, node1, &support1), "node1 supported");
  if (!support0 || !support1) {
    return REF_SUCCESS;
  }

  /* evaluate edge geom on edge cell if present */
  nodes[0] = node0;
  nodes[1] = node1;
  ref_cell = ref_grid_edg(ref_grid);
  status = ref_cell_with(ref_cell, nodes, &cell);
  if (REF_NOT_FOUND != status) {
    RSS(status, "search for edg");
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "get id");
    id = nodes[ref_cell_node_per(ref_cell)];
    type = REF_GEOM_EDGE;
    RSS(ref_geom_cell_tuv(ref_geom, node0, nodes, type, param0, &sense),
        "cell uv");
    RSS(ref_geom_cell_tuv(ref_geom, node1, nodes, type, param1, &sense),
        "cell uv");
    param[0] = 0.5 * (param0[0] + param1[0]);
    if (ref_geom_model_loaded(ref_geom)) {
      RSB(ref_geom_inverse_eval(ref_geom, type, id, xyz, param),
          "inv eval edge", ref_geom_tec(ref_grid, "ref_geom_split_edge.tec"));
      /* enforce bounding box and use midpoint as full-back */
      if (param[0] < MIN(param0[0], param1[0]) ||
          MAX(param0[0], param1[0]) < param[0]) {
        param[0] = 0.5 * (param0[0] + param1[0]);
      }
      /* constrain xyz to geom, inverse_eval does not set */
      RSS(ref_geom_eval_at(ref_geom, type, id, param, xyz, NULL), "eval at");
    }
    return REF_SUCCESS;
  }

  /* insert face between */
  ref_cell = ref_grid_tri(ref_grid);
  RSS(ref_cell_list_with2(ref_cell, node0, node1, 2, &ncell, cells), "list");
  if (0 == ncell) { /* volume edge */
    return REF_SUCCESS;
  }
  cell = cells[0]; /* may only have one in parallel */
  RSS(ref_cell_nodes(ref_cell, cell, nodes), "get id");
  id = nodes[ref_cell_node_per(ref_cell)];
  type = REF_GEOM_FACE;
  RSS(ref_geom_cell_tuv(ref_geom, node0, nodes, type, param0, &sense),
      "cell uv");
  RSS(ref_geom_cell_tuv(ref_geom, node1, nodes, type, param1, &sense),
      "cell uv");
  param[0] = 0.5 * (param0[0] + param1[0]);
  param[1] = 0.5 * (param0[1] + param1[1]);
  if (ref_geom_model_loaded(ref_geom)) {
    RSB(ref_geom_inverse_eval(ref_geom, type, id, xyz, param), "inv eval face",
        ref_geom_tec(ref_grid, "ref_geom_xyz_between_face.tec"));
    if (2 == ncell) { /* revisit in para */
      /* enforce bounding box of node0 and try midpoint */
      RSS(ref_geom_tri_uv_bounding_box2(ref_grid, node0, node1, uv_min, uv_max),
          "bb");
      if (param[0] < uv_min[0] || uv_max[0] < param[0] ||
          param[1] < uv_min[1] || uv_max[1] < param[1]) {
        param[0] = 0.5 * (param0[0] + param1[0]);
        param[1] = 0.5 * (param0[1] + param1[1]);
      }
    }
    /* constrain xyz to geom, inverse_eval does not set */
    RSS(ref_geom_eval_at(ref_geom, type, id, param, xyz, NULL), "eval at");
    return REF_SUCCESS;
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_add_between(REF_GRID ref_grid, REF_INT node0, REF_INT node1,
                                REF_DBL node1_weight, REF_INT new_node) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT type, id;
  REF_DBL param[2], param0[2], param1[2];
  REF_DBL uv_min[2], uv_max[2];
  REF_BOOL has_edge_support, supported;
  REF_INT edge_geom;
  REF_INT sense, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_STATUS status;
  REF_INT i, ncell, cells[2];
  REF_INT face_geom, geom0, geom1;
  REF_BOOL support0, support1;
  REF_DBL node0_weight = 1.0 - node1_weight;

  RSS(ref_geom_supported(ref_geom, node0, &support0), "node0 supported");
  RSS(ref_geom_supported(ref_geom, node1, &support1), "node1 supported");
  if (!support0 || !support1) {
    return REF_SUCCESS;
  }

  /* insert edge geom on edge cell if present */
  nodes[0] = node0;
  nodes[1] = node1;
  ref_cell = ref_grid_edg(ref_grid);
  status = ref_cell_with(ref_cell, nodes, &cell);
  if (REF_NOT_FOUND == status) {
    has_edge_support = REF_FALSE;
    edge_geom = REF_EMPTY;
  } else {
    RSS(status, "search for edg");
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "get id");
    id = nodes[ref_cell_node_per(ref_cell)];
    type = REF_GEOM_EDGE;
    RSS(ref_geom_cell_tuv(ref_geom, node0, nodes, type, param0, &sense),
        "cell uv");
    RSS(ref_geom_cell_tuv(ref_geom, node1, nodes, type, param1, &sense),
        "cell uv");
    param[0] = node0_weight * param0[0] + node1_weight * param1[0];
    if (ref_geom_model_loaded(ref_geom))
      RSB(ref_geom_inverse_eval(ref_geom, type, id,
                                ref_node_xyz_ptr(ref_node, new_node), param),
          "inv eval edge", ref_geom_tec(ref_grid, "ref_geom_split_edge.tec"));
    /* enforce bounding box and use midpoint as full-back */
    if (param[0] < MIN(param0[0], param1[0]) ||
        MAX(param0[0], param1[0]) < param[0])
      param[0] = node0_weight * param0[0] + node1_weight * param1[0];
    if (ref_geom_model_loaded(ref_geom)) { /* check weight and distance ratio */
      REF_DBL xyz[3], dx0[3], dx1[3], d0, d1, total, actual_weight;
      REF_DBL mid_t, mid_weight;
      REF_INT ii;
      RSS(ref_geom_eval_at(ref_geom, REF_GEOM_EDGE, id, param, xyz, NULL),
          "eval");
      for (ii = 0; ii < 3; ii++)
        dx0[ii] = ref_node_xyz(ref_node, ii, node0) - xyz[ii];
      for (ii = 0; ii < 3; ii++)
        dx1[ii] = ref_node_xyz(ref_node, ii, node1) - xyz[ii];
      d0 = sqrt(ref_math_dot(dx0, dx0));
      d1 = sqrt(ref_math_dot(dx1, dx1));
      total = d0 + d1;
      actual_weight = -1.0;
      if (ref_math_divisible(d0, total)) {
        actual_weight = d0 / total;
      }
      mid_t = node0_weight * param0[0] + node1_weight * param1[0];
      RSS(ref_geom_eval_at(ref_geom, REF_GEOM_EDGE, id, &mid_t, xyz, NULL),
          "eval");
      for (ii = 0; ii < 3; ii++)
        dx0[ii] = ref_node_xyz(ref_node, ii, node0) - xyz[ii];
      for (ii = 0; ii < 3; ii++)
        dx1[ii] = ref_node_xyz(ref_node, ii, node1) - xyz[ii];
      d0 = sqrt(ref_math_dot(dx0, dx0));
      d1 = sqrt(ref_math_dot(dx1, dx1));
      total = d0 + d1;
      mid_weight = -1.0;
      if (ref_math_divisible(d0, total)) {
        mid_weight = d0 / total;
      }
      if (ABS(mid_weight - node1_weight) < ABS(actual_weight - node1_weight)) {
        /* printf("request %f actual %f mid %f\n",
                  node1_weight,actual_weight,mid_weight); */
        param[0] = mid_t;
      }
    }
    if (param[0] < MIN(param0[0], param1[0]) ||
        MAX(param0[0], param1[0]) < param[0])
      param[0] = node0_weight * param0[0] + node1_weight * param1[0];
    RSS(ref_geom_add(ref_geom, new_node, type, id, param), "new geom");
    has_edge_support = REF_TRUE;
    RSS(ref_geom_find(ref_geom, new_node, type, id, &edge_geom),
        "find the new edge for later face uv evaluation");
  }

  /* insert face between */
  ref_cell = ref_grid_tri(ref_grid);
  RSS(ref_cell_list_with2(ref_cell, node0, node1, 2, &ncell, cells), "list");
  if (0 == ncell) { /* volume edge */
    return REF_SUCCESS;
  }
  if (ref_geom_manifold(ref_geom)) {
    REIB(2, ncell, "expected two tri for between", {
      ref_geom_tattle(ref_geom, node0);
      ref_geom_tattle(ref_geom, node1);
      ref_node_location(ref_node, node0);
      ref_node_location(ref_node, node1);
    });
  }
  for (i = 0; i < ncell; i++) {
    cell = cells[i];
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "get id");

    RSS(ref_geom_tri_supported(ref_geom, nodes, &supported), "tri support");
    if (!supported) continue; /* no geom support, skip */

    id = nodes[ref_cell_node_per(ref_cell)];
    type = REF_GEOM_FACE;
    RSS(ref_geom_cell_tuv(ref_geom, node0, nodes, type, param0, &sense),
        "cell uv");
    RSS(ref_geom_cell_tuv(ref_geom, node1, nodes, type, param1, &sense),
        "cell uv");
    param[0] = node0_weight * param0[0] + node1_weight * param1[0];
    param[1] = node0_weight * param0[1] + node1_weight * param1[1];
    if (ref_geom_model_loaded(ref_geom) && !has_edge_support) {
      RSB(ref_geom_inverse_eval(ref_geom, type, id,
                                ref_node_xyz_ptr(ref_node, new_node), param),
          "inv eval face", ref_geom_tec(ref_grid, "ref_geom_split_face.tec"));
      /* enforce bounding box of node0 and try midpoint */
      RSS(ref_geom_tri_uv_bounding_box2(ref_grid, node0, node1, uv_min, uv_max),
          "bb");
      if (param[0] < uv_min[0] || uv_max[0] < param[0] ||
          param[1] < uv_min[1] || uv_max[1] < param[1]) {
        param[0] = node0_weight * param0[0] + node1_weight * param1[0];
        param[1] = node0_weight * param0[1] + node1_weight * param1[1];
      }
    }

    RSS(ref_geom_add(ref_geom, new_node, type, id, param), "new geom");
    RSS(ref_geom_find(ref_geom, new_node, type, id, &face_geom),
        "new face geom");

    RSS(ref_geom_find(ref_geom, node0, type, id, &geom0), "face geom");
    RSS(ref_geom_find(ref_geom, node1, type, id, &geom1), "face geom");
    if (0 != ref_geom_jump(ref_geom, geom0) &&
        0 != ref_geom_jump(ref_geom, geom1) &&
        ref_geom_jump(ref_geom, geom0) == ref_geom_jump(ref_geom, geom1)) {
      ref_geom_jump(ref_geom, face_geom) = ref_geom_jump(ref_geom, geom0);
    }

#ifdef HAVE_EGADS
    /* if there is an edge between, set the face uv based on edge t */
    if (ref_geom_model_loaded(ref_geom) && has_edge_support) {
      ego *edges, *faces;
      ego edge, face;
      REF_INT faceid, edgeid;
      REF_DBL t;
      edgeid = ref_geom_id(ref_geom, edge_geom);
      edges = (ego *)(ref_geom->edges);
      edge = edges[edgeid - 1];
      faces = (ego *)(ref_geom->faces);
      faceid = ref_geom_id(ref_geom, face_geom);
      face = faces[faceid - 1];
      t = ref_geom_param(ref_geom, 0, edge_geom);
      sense = 0;
      if (0 != ref_geom_jump(ref_geom, face_geom)) sense = 1;
      REIB(EGADS_SUCCESS, EG_getEdgeUV(face, edge, sense, t, param), "edge uv",
           {
             printf("edge %d face %d sense %d\n", edgeid, faceid, sense);
             ref_geom_tattle(ref_geom, ref_geom_node(ref_geom, edge_geom));
           });
      ref_geom_param(ref_geom, 0, face_geom) = param[0];
      ref_geom_param(ref_geom, 1, face_geom) = param[1];
    }
#endif
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_support_between(REF_GRID ref_grid, REF_INT node0,
                                    REF_INT node1, REF_BOOL *needs_support) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_INT item0, item1;
  REF_INT geom0, geom1;
  REF_INT type, id;
  REF_BOOL has_id;

  *needs_support = REF_FALSE;
  /* assume face check is sufficient */
  each_ref_adj_node_item_with_ref(ref_geom_adj(ref_geom), node0, item0, geom0)
      each_ref_adj_node_item_with_ref(ref_geom_adj(ref_geom), node1, item1,
                                      geom1) {
    type = REF_GEOM_FACE;
    if (ref_geom_type(ref_geom, geom0) == type &&
        ref_geom_type(ref_geom, geom1) == type &&
        ref_geom_id(ref_geom, geom0) == ref_geom_id(ref_geom, geom1)) {
      id = ref_geom_id(ref_geom, geom0);
      RSS(ref_cell_side_has_id(ref_grid_tri(ref_grid), node0, node1, id,
                               &has_id),
          "has edge id");
      if (has_id) {
        *needs_support = REF_TRUE;
        return REF_SUCCESS;
      }
    }
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_tri_uv_bounding_box(REF_GRID ref_grid, REF_INT node,
                                        REF_DBL *uv_min, REF_DBL *uv_max) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT item, cell, cell_node, id, iuv;
  REF_DBL uv[2];
  REF_INT sense;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];

  /* get face id and initialize min and max */
  RSS(ref_geom_unique_id(ref_geom, node, REF_GEOM_FACE, &id), "id");
  RSS(ref_geom_tuv(ref_geom, node, REF_GEOM_FACE, id, uv_min), "uv_min");
  RSS(ref_geom_tuv(ref_geom, node, REF_GEOM_FACE, id, uv_max), "uv_max");

  each_ref_cell_having_node(ref_cell, node, item, cell) {
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "cell nodes");
    each_ref_cell_cell_node(ref_cell, cell_node) {
      RSS(ref_geom_cell_tuv(ref_geom, nodes[cell_node], nodes, REF_GEOM_FACE,
                            uv, &sense),
          "cell uv");
      for (iuv = 0; iuv < 2; iuv++) uv_min[iuv] = MIN(uv_min[iuv], uv[iuv]);
      for (iuv = 0; iuv < 2; iuv++) uv_max[iuv] = MAX(uv_max[iuv], uv[iuv]);
    }
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_tri_uv_bounding_box2(REF_GRID ref_grid, REF_INT node0,
                                         REF_INT node1, REF_DBL *uv_min,
                                         REF_DBL *uv_max) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT cell, cell_node, iuv;
  REF_DBL uv[2];
  REF_INT i, ncell, cells[2];
  REF_INT sense;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];

  RSS(ref_cell_list_with2(ref_cell, node0, node1, 2, &ncell, cells), "list");
  REIS(2, ncell, "expected two tri for box2 nodes");

  for (iuv = 0; iuv < 2; iuv++) uv_min[iuv] = REF_DBL_MAX;
  for (iuv = 0; iuv < 2; iuv++) uv_max[iuv] = REF_DBL_MIN;
  for (i = 0; i < ncell; i++) {
    cell = cells[i];
    RSS(ref_cell_nodes(ref_cell, cell, nodes), "cell nodes");
    each_ref_cell_cell_node(ref_cell, cell_node) {
      RSS(ref_geom_cell_tuv(ref_geom, nodes[cell_node], nodes, REF_GEOM_FACE,
                            uv, &sense),
          "cell uv");
      for (iuv = 0; iuv < 2; iuv++) uv_min[iuv] = MIN(uv_min[iuv], uv[iuv]);
      for (iuv = 0; iuv < 2; iuv++) uv_max[iuv] = MAX(uv_max[iuv], uv[iuv]);
    }
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_constrain_all(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node;
  each_ref_node_valid_node(ref_node, node) {
    RSS(ref_geom_constrain(ref_grid, node), "constrain node");
  }
  return REF_SUCCESS;
}

REF_STATUS ref_geom_constrain(REF_GRID ref_grid, REF_INT node) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_ADJ ref_adj = ref_geom_adj(ref_geom);
  REF_INT item, geom;
  REF_BOOL have_geom_node;
  REF_BOOL have_geom_edge;
  REF_BOOL have_geom_face;
  REF_INT node_geom;
  REF_INT edge_geom;
  REF_INT face_geom;
  REF_DBL xyz[3];

  /* no geom, do nothing */
  if (ref_adj_empty(ref_adj, node)) return REF_SUCCESS;

  if (ref_geom_meshlinked(ref_geom)) {
    RSS(ref_meshlink_constrain(ref_grid, node), "meshlink");
    return REF_SUCCESS;
  }

  have_geom_node = REF_FALSE;
  each_ref_adj_node_item_with_ref(ref_adj, node, item, geom) {
    if (REF_GEOM_NODE == ref_geom_type(ref_geom, geom)) {
      have_geom_node = REF_TRUE;
      node_geom = geom;
      break;
    }
  }

  if (have_geom_node) { /* update T of edges? update UV of (degen) faces? */
    RSS(ref_geom_eval(ref_geom, node_geom, xyz, NULL), "eval edge");
    node = ref_geom_node(ref_geom, node_geom);
    ref_node_xyz(ref_node, 0, node) = xyz[0];
    ref_node_xyz(ref_node, 1, node) = xyz[1];
    ref_node_xyz(ref_node, 2, node) = xyz[2];
    return REF_SUCCESS;
  }

  have_geom_edge = REF_FALSE;
  edge_geom = REF_EMPTY;
  each_ref_adj_node_item_with_ref(ref_adj, node, item, geom) {
    if (REF_GEOM_EDGE == ref_geom_type(ref_geom, geom)) {
      have_geom_edge = REF_TRUE;
      edge_geom = geom;
      break;
    }
  }

  /* edge geom, evaluate edge and update face uv */
  if (have_geom_edge) {
    RSS(ref_geom_eval(ref_geom, edge_geom, xyz, NULL), "eval edge");
    node = ref_geom_node(ref_geom, edge_geom);
    ref_node_xyz(ref_node, 0, node) = xyz[0];
    ref_node_xyz(ref_node, 1, node) = xyz[1];
    ref_node_xyz(ref_node, 2, node) = xyz[2];
    RSS(ref_geom_eval_edge_face_uv(ref_grid, edge_geom), "resol edge uv");
    return REF_SUCCESS;
  }

  /* look for face geom */
  have_geom_face = REF_FALSE;
  face_geom = REF_EMPTY;
  each_ref_adj_node_item_with_ref(ref_adj, node, item, geom) {
    if (REF_GEOM_FACE == ref_geom_type(ref_geom, geom)) {
      have_geom_face = REF_TRUE;
      face_geom = geom;
      break;
    }
  }

  /* face geom, evaluate on face uv */
  if (have_geom_face) {
    RSS(ref_geom_eval(ref_geom, face_geom, xyz, NULL), "eval face");
    node = ref_geom_node(ref_geom, face_geom);
    ref_node_xyz(ref_node, 0, node) = xyz[0];
    ref_node_xyz(ref_node, 1, node) = xyz[1];
    ref_node_xyz(ref_node, 2, node) = xyz[2];
    return REF_SUCCESS;
  }

  return REF_SUCCESS;
}
/*
  [x_t,y_t,z_t] edge [6]
  [x_tt,y_tt,z_tt]
  [x_u,y_u,z_u] [x_v,y_v,z_v] face [15]
  [x_uu,y_uu,z_uu] [x_uv,y_uv,z_uv] [x_vv,y_vv,z_vv]
*/
REF_STATUS ref_geom_eval(REF_GEOM ref_geom, REF_INT geom, REF_DBL *xyz,
                         REF_DBL *dxyz_dtuv) {
  REF_INT type, id, i;
  REF_DBL params[2];
  if (geom < 0 || ref_geom_max(ref_geom) <= geom) return REF_INVALID;
  params[0] = 0.0;
  params[1] = 0.0;
  type = ref_geom_type(ref_geom, geom);
  id = ref_geom_id(ref_geom, geom);

  for (i = 0; i < type; i++) {
    params[i] = ref_geom_param(ref_geom, i, geom);
  }
  RSS(ref_geom_eval_at(ref_geom, type, id, params, xyz, dxyz_dtuv), "eval at");
  return REF_SUCCESS;
}

REF_STATUS ref_geom_eval_at(REF_GEOM ref_geom, REF_INT type, REF_INT id,
                            REF_DBL *params, REF_DBL *xyz, REF_DBL *dxyz_dtuv) {
#ifdef HAVE_EGADS
  double eval[18];
  REF_INT i;
  ego *nodes, *edges, *faces;
  ego object;
  int status;

  object = (ego)NULL;
  switch (type) {
    case (REF_GEOM_NODE):
      RNS(ref_geom->nodes, "nodes not loaded");
      if (id < 1 || id > ref_geom->nnode) return REF_INVALID;
      nodes = (ego *)(ref_geom->nodes);
      object = nodes[id - 1];
      {
        ego ref, *pchldrn;
        int oclass, mtype, nchild, *psens;
        REIS(EGADS_SUCCESS,
             EG_getTopology(object, &ref, &oclass, &mtype, xyz, &nchild,
                            &pchldrn, &psens),
             "EG topo node");
      }
      return REF_SUCCESS;
      break;
    case (REF_GEOM_EDGE):
      RNS(ref_geom->edges, "edges not loaded");
      if (id < 1 || id > ref_geom->nedge) return REF_INVALID;
      edges = (ego *)(ref_geom->edges);
      object = edges[id - 1];
      break;
    case (REF_GEOM_FACE):
      RNS(ref_geom->faces, "faces not loaded");
      if (id < 1 || id > ref_geom->nface) return REF_INVALID;
      faces = (ego *)(ref_geom->faces);
      object = faces[id - 1];
      break;
    default:
      RSS(REF_IMPLEMENT, "unknown geom");
  }

  status = EG_evaluate(object, params, eval);
  if (EGADS_SUCCESS != status) {
    ego ref, *pchldrn;
    int oclass, mtype, nchild, *psens;
    double trange[2];
    printf("type %d id %d\n", type, id);
    if (type > 0) printf("param[0] = %f\n", params[0]);
    if (type > 1) printf("param[1] = %f\n", params[1]);
    REIS(EGADS_SUCCESS,
         EG_getTopology(object, &ref, &oclass, &mtype, trange, &nchild,
                        &pchldrn, &psens),
         "EG topo node");
    printf("trange %f %f\n", trange[0], trange[1]);
    REIS(EGADS_SUCCESS, status, "eval");
  }
  xyz[0] = eval[0];
  xyz[1] = eval[1];
  xyz[2] = eval[2];
  if (NULL != dxyz_dtuv) {
    for (i = 0; i < 6; i++) dxyz_dtuv[i] = eval[3 + i];
    if (REF_GEOM_FACE == type)
      for (i = 0; i < 9; i++) dxyz_dtuv[6 + i] = eval[9 + i];
  }
  return REF_SUCCESS;
#else
  REF_INT i;
  printf("evaluating to (0,0,0), No EGADS linked for %s\n", __func__);
  printf("type %d id %d\n", type, id);
  SUPRESS_UNUSED_COMPILER_WARNING(ref_geom);
  SUPRESS_UNUSED_COMPILER_WARNING(params);
  xyz[0] = 0.0;
  xyz[1] = 0.0;
  xyz[2] = 0.0;
  if (NULL != dxyz_dtuv) {
    for (i = 0; i < 6; i++) {
      dxyz_dtuv[i] = 0.0;
    }
    if (REF_GEOM_FACE == type) {
      for (i = 0; i < 9; i++) dxyz_dtuv[6 + i] = 0.0;
    }
  }
  return REF_IMPLEMENT;
#endif
}

REF_STATUS ref_geom_inverse_eval(REF_GEOM ref_geom, REF_INT type, REF_INT id,
                                 REF_DBL *xyz, REF_DBL *param) {
#ifdef HAVE_EGADS
  ego object = (ego)NULL;
  int i, guess_status, noguess_status;
  REF_DBL guess_param[2], noguess_param[2];
  REF_DBL guess_closest[3], noguess_closest[3];
  REF_DBL guess_dist, noguess_dist;

  ego ref, *pchldrn;
  int oclass, mtype, nchild, *psens;
  double range[4];

  REF_BOOL guess_in_range, noguess_in_range;

  REF_BOOL verbose = REF_FALSE;

  switch (type) {
    case (REF_GEOM_NODE):
      printf("GEOM_NODE ref_geom_inverse_eval not defined\n");
      return REF_IMPLEMENT;
      break;
    case (REF_GEOM_EDGE):
      RNS(ref_geom->edges, "edges not loaded");
      if (id < 1 || id > ref_geom->nedge) return REF_INVALID;
      object = ((ego *)(ref_geom->edges))[id - 1];
      break;
    case (REF_GEOM_FACE):
      RNS(ref_geom->faces, "faces not loaded");
      if (id < 1 || id > ref_geom->nface) return REF_INVALID;
      object = ((ego *)(ref_geom->faces))[id - 1];
      break;
    default:
      RSS(REF_IMPLEMENT, "unknown geom");
  }

  REIS(EGADS_SUCCESS,
       EG_getTopology(object, &ref, &oclass, &mtype, range, &nchild, &pchldrn,
                      &psens),
       "EG topo node");

  for (i = 0; i < type; i++) guess_param[i] = param[i];
  for (i = 0; i < type; i++) noguess_param[i] = param[i];
  guess_status = EG_invEvaluateGuess(object, xyz, guess_param, guess_closest);
  noguess_status = EG_invEvaluate(object, xyz, noguess_param, noguess_closest);
  if (verbose) {
    printf("guess %d noguess %d type %d id %d xyz %f %f %f\n", guess_status,
           noguess_status, type, id, xyz[0], xyz[1], xyz[2]);
    for (i = 0; i < type; i++) {
      printf("%d: start %f guess %f noguess %f\n", i, param[i], guess_param[i],
             noguess_param[i]);
    }
  }
  if (EGADS_SUCCESS != guess_status && EGADS_SUCCESS != noguess_status)
    return REF_FAILURE;

  if (EGADS_SUCCESS == guess_status && EGADS_SUCCESS != noguess_status) {
    for (i = 0; i < type; i++) param[i] = guess_param[i];
    return REF_SUCCESS;
  }

  if (EGADS_SUCCESS != guess_status && EGADS_SUCCESS == noguess_status) {
    for (i = 0; i < type; i++) param[i] = noguess_param[i];
    return REF_SUCCESS;
  }

  guess_dist = sqrt(pow(guess_closest[0] - xyz[0], 2) +
                    pow(guess_closest[1] - xyz[1], 2) +
                    pow(guess_closest[2] - xyz[2], 2));
  noguess_dist = sqrt(pow(noguess_closest[0] - xyz[0], 2) +
                      pow(noguess_closest[1] - xyz[1], 2) +
                      pow(noguess_closest[2] - xyz[2], 2));

  guess_in_range = REF_TRUE;
  for (i = 0; i < type; i++)
    if (guess_param[i] < range[0 + 2 * i] || range[1 + 2 * i] < guess_param[i])
      guess_in_range = REF_FALSE;

  noguess_in_range = REF_TRUE;
  for (i = 0; i < type; i++)
    if (noguess_param[i] < range[0 + 2 * i] ||
        range[1 + 2 * i] < noguess_param[i])
      noguess_in_range = REF_FALSE;

  if (verbose) {
    printf("guess %e noguess %e\n", guess_dist, noguess_dist);
    printf("guess %d noguess %d in range\n", guess_in_range, noguess_in_range);
  }

  if (guess_in_range && !noguess_in_range) {
    for (i = 0; i < type; i++) param[i] = guess_param[i];
    return REF_SUCCESS;
  }

  if (!guess_in_range && noguess_in_range) {
    for (i = 0; i < type; i++) param[i] = noguess_param[i];
    return REF_SUCCESS;
  }

  if (guess_dist < noguess_dist) {
    for (i = 0; i < type; i++) param[i] = guess_param[i];
  } else {
    for (i = 0; i < type; i++) param[i] = noguess_param[i];
  }
  return REF_SUCCESS;

#else
  printf("no-op, No EGADS linked for %s type %d id %d x %f p %f n %d\n",
         __func__, type, id, xyz[0], param[0], ref_geom_n(ref_geom));
  return REF_IMPLEMENT;
#endif
}

REF_STATUS ref_geom_radian_request(REF_GEOM ref_geom, REF_INT geom,
                                   REF_DBL *delta_radian) {
  REF_INT node, item, face_geom, face;
  REF_DBL segments, face_segments;
  REF_DBL face_set = -998.0;
  REF_DBL face_active = 0.01;
  REF_BOOL turn_off, use_face;
  use_face = REF_FALSE;
  turn_off = REF_FALSE;

  segments = 0.0;

  if (NULL != (ref_geom)->face_seg_per_rad) {
    node = ref_geom_node(ref_geom, geom);
    each_ref_geom_having_node(ref_geom, node, item, face_geom) {
      if (REF_GEOM_FACE == ref_geom_type(ref_geom, face_geom)) {
        face = ref_geom_id(ref_geom, face_geom) - 1;
        face_segments = ref_geom->face_seg_per_rad[face];
        if (face_segments < face_set) continue;
        if (face_segments > face_active) {
          segments = MAX(segments, face_segments);
          use_face = REF_TRUE;
        } else {
          turn_off = REF_TRUE;
          break;
        }
      }
    }
  }

  if (turn_off) {
    *delta_radian = 100.0;
    return REF_SUCCESS;
  }

  if (!use_face) {
    segments = ref_geom_segments_per_radian_of_curvature(ref_geom);
  }

  if (segments > 0.1 && ref_math_divisible(1.0, segments)) {
    *delta_radian = 1.0 / segments;
  } else {
    *delta_radian = 100.0;
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_edge_curvature(REF_GEOM ref_geom, REF_INT geom, REF_DBL *k,
                                   REF_DBL *normal) {
#ifdef HAVE_EGADS
  double curvature[4];
  ego *edges;
  ego object;
  int edgeid;
  double t;
  REIS(REF_GEOM_EDGE, ref_geom_type(ref_geom, geom), "expected edge geom");
  RNS(ref_geom->edges, "edges not loaded");
  edgeid = ref_geom_id(ref_geom, geom);
  edges = (ego *)(ref_geom->edges);
  object = edges[edgeid - 1];
  RNS(object, "EGADS object is NULL. Has the geometry been loaded?");

  t = ref_geom_param(ref_geom, 0, geom);

  REIS(EGADS_SUCCESS, EG_curvature(object, &t, curvature), "curve");
  *k = curvature[0];
  normal[0] = curvature[1];
  normal[1] = curvature[2];
  normal[2] = curvature[3];
  return REF_SUCCESS;
#else
  printf("curvature 0: No EGADS linked for %s\n", __func__);
  SUPRESS_UNUSED_COMPILER_WARNING(ref_geom);
  SUPRESS_UNUSED_COMPILER_WARNING(geom);
  *k = 0.0;
  normal[0] = 1.0;
  normal[1] = 0.0;
  normal[2] = 0.0;
  return REF_IMPLEMENT;
#endif
}

REF_STATUS ref_geom_face_curvature(REF_GEOM ref_geom, REF_INT geom, REF_DBL *kr,
                                   REF_DBL *r, REF_DBL *ks, REF_DBL *s) {
#ifdef HAVE_EGADS
  double curvature[8];
  ego *faces;
  ego object;
  int egads_status;
  int faceid;
  double uv[2];
  REIS(REF_GEOM_FACE, ref_geom_type(ref_geom, geom), "expected face geom");
  RNS(ref_geom->faces, "faces not loaded");
  faceid = ref_geom_id(ref_geom, geom);
  faces = (ego *)(ref_geom->faces);
  object = faces[faceid - 1];
  RNS(object, "EGADS object is NULL. Has the geometry been loaded?");

  uv[0] = ref_geom_param(ref_geom, 0, geom);
  uv[1] = ref_geom_param(ref_geom, 1, geom);
  egads_status = EG_curvature(object, uv, curvature);
  if (0 != ref_geom_degen(ref_geom, geom) || EGADS_DEGEN == egads_status) {
    REF_DBL xyz[3], dxyz_duv[15], du, dv;
    ego ref, *pchldrn;
    int oclass, mtype, nchild, *psens;
    double uv_range[4];
    double params[2];
    REF_DBL shift = 1.0e-2;
    params[0] = uv[0];
    params[1] = uv[1];
    RSS(ref_geom_eval_at(ref_geom, REF_GEOM_FACE, faceid, uv, xyz, dxyz_duv),
        "eval at");
    du = sqrt(ref_math_dot(&(dxyz_duv[0]), &(dxyz_duv[0])));
    dv = sqrt(ref_math_dot(&(dxyz_duv[3]), &(dxyz_duv[3])));
    REIS(EGADS_SUCCESS,
         EG_getTopology(object, &ref, &oclass, &mtype, uv_range, &nchild,
                        &pchldrn, &psens),
         "EG topo face");
    if (du > dv) {
      params[0] =
          (1.0 - shift) * params[0] + shift * 0.5 * (uv_range[0] + uv_range[1]);
    } else {
      params[1] =
          (1.0 - shift) * params[1] + shift * 0.5 * (uv_range[2] + uv_range[3]);
    }
    egads_status = EG_curvature(object, params, curvature);
  }
  REIS(EGADS_SUCCESS, egads_status, "curve");
  *kr = curvature[0];
  r[0] = curvature[1];
  r[1] = curvature[2];
  r[2] = curvature[3];
  *ks = curvature[4];
  s[0] = curvature[5];
  s[1] = curvature[6];
  s[2] = curvature[7];
  return REF_SUCCESS;
#else
  printf("curvature 0, 0: No EGADS linked for %s\n", __func__);
  SUPRESS_UNUSED_COMPILER_WARNING(ref_geom);
  SUPRESS_UNUSED_COMPILER_WARNING(geom);
  *kr = 0.0;
  r[0] = 1.0;
  r[1] = 0.0;
  r[2] = 0.0;
  *ks = 0.0;
  s[0] = 0.0;
  s[1] = 1.0;
  s[2] = 0.0;
  return REF_IMPLEMENT;
#endif
}

REF_STATUS ref_geom_uv_rsn(REF_DBL *uv, REF_DBL *r, REF_DBL *s, REF_DBL *n,
                           REF_DBL *drsduv) {
  REF_INT i;
  REF_DBL dot;
  REF_DBL len;

  for (i = 0; i < 3; i++) r[i] = uv[i];
  drsduv[0] = 1.0;
  drsduv[1] = 0.0;
  for (i = 0; i < 3; i++) s[i] = uv[i + 3];
  drsduv[2] = 0.0;
  drsduv[3] = 1.0;
  len = sqrt(ref_math_dot(r, r));
  drsduv[0] /= len;
  drsduv[1] /= len;
  RSS(ref_math_normalize(r), "norm r (u)");
  len = sqrt(ref_math_dot(s, s));
  drsduv[2] /= len;
  drsduv[3] /= len;
  RSS(ref_math_normalize(s), "norm s (v)");

  dot = ref_math_dot(r, s);
  for (i = 0; i < 3; i++) s[i] -= dot * r[i];
  drsduv[2] -= dot * drsduv[0];
  drsduv[3] -= dot * drsduv[1];

  len = sqrt(ref_math_dot(s, s));
  drsduv[2] /= len;
  drsduv[3] /= len;
  RSS(ref_math_normalize(s), "norm s (v)");

  ref_math_cross_product(r, s, n);

  return REF_SUCCESS;
}

REF_STATUS ref_geom_face_rsn(REF_GEOM ref_geom, REF_INT faceid, REF_DBL *uv,
                             REF_DBL *r, REF_DBL *s, REF_DBL *n) {
  REF_DBL xyz[3];
  REF_DBL dxyz_dtuv[15];
  REF_DBL drsduv[4];
  RSS(ref_geom_eval_at(ref_geom, REF_GEOM_FACE, faceid, uv, xyz, dxyz_dtuv),
      "eval");
  RSS(ref_geom_uv_rsn(dxyz_dtuv, r, s, n, drsduv), "deriv to rsn");
  return REF_SUCCESS;
}

REF_STATUS ref_geom_tri_centroid(REF_GRID ref_grid, REF_INT *nodes,
                                 REF_DBL *uv) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT cell_node;
  REF_DBL node_uv[2];
  REF_INT sens;
  uv[0] = 0.0;
  uv[1] = 0.0;
  each_ref_cell_cell_node(ref_cell, cell_node) {
    RSB(ref_geom_cell_tuv(ref_geom, nodes[cell_node], nodes, REF_GEOM_FACE,
                          node_uv, &sens),
        "cell node uv",
        { ref_geom_tec(ref_grid, "ref_geom_tri_centroid_error.tec"); });
    uv[0] += (1.0 / 3.0) * node_uv[0];
    uv[1] += (1.0 / 3.0) * node_uv[1];
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_tri_norm_deviation(REF_GRID ref_grid, REF_INT *nodes,
                                       REF_DBL *dot_product) {
  REF_DBL uv[2];
  REF_DBL tri_normal[3];
  REF_DBL r[3], s[3], n[3], area_sign;
  REF_INT id;
  REF_STATUS status;
  *dot_product = -2.0;

  if (ref_geom_meshlinked(ref_grid_geom(ref_grid))) {
    RSS(ref_meshlink_tri_norm_deviation(ref_grid, nodes, dot_product),
        "meshlink");
    return REF_SUCCESS;
  }

  id = nodes[ref_cell_node_per(ref_grid_tri(ref_grid))];
  RSS(ref_node_tri_normal(ref_grid_node(ref_grid), nodes, tri_normal),
      "tri normal");
  /* collapse attempts could create zero area, reject the step with -2.0 */
  status = ref_math_normalize(tri_normal);
  if (REF_DIV_ZERO == status) return REF_SUCCESS;
  RSS(status, "normalize");

  RSS(ref_geom_tri_centroid(ref_grid, nodes, uv), "tri cent");
  RSS(ref_geom_face_rsn(ref_grid_geom(ref_grid), id, uv, r, s, n), "rsn");
  RSS(ref_geom_uv_area_sign(ref_grid, id, &area_sign), "a sign");

  *dot_product = area_sign * ref_math_dot(n, tri_normal);

  return REF_SUCCESS;
}

REF_STATUS ref_geom_crease(REF_GRID ref_grid, REF_INT node, REF_DBL *dot_prod) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT item0, item1, cell0, cell1, id;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL uv[2];
  REF_DBL r[3], s[3];
  REF_DBL n0[3], area_sign0;
  REF_DBL n1[3], area_sign1;

  *dot_prod = 1.0;

  if (!ref_node_valid(ref_node, node)) {
    return REF_SUCCESS;
  }

  each_ref_cell_having_node(ref_cell, node, item0, cell0) {
    RSS(ref_cell_nodes(ref_grid_tri(ref_grid), cell0, nodes),
        "tri list for edge");
    RSS(ref_geom_tri_centroid(ref_grid, nodes, uv), "tri cent");
    id = nodes[ref_cell_node_per(ref_cell)];
    RSS(ref_geom_face_rsn(ref_grid_geom(ref_grid), id, uv, r, s, n0), "rsn");
    RSS(ref_geom_uv_area_sign(ref_grid, id, &area_sign0), "a sign");
    each_ref_cell_having_node(ref_cell, node, item1, cell1) {
      RSS(ref_cell_nodes(ref_grid_tri(ref_grid), cell1, nodes),
          "tri list for edge");
      RSS(ref_geom_tri_centroid(ref_grid, nodes, uv), "tri cent");
      id = nodes[ref_cell_node_per(ref_cell)];
      RSS(ref_geom_face_rsn(ref_grid_geom(ref_grid), id, uv, r, s, n1), "rsn");
      RSS(ref_geom_uv_area_sign(ref_grid, id, &area_sign1), "a sign");

      *dot_prod =
          MIN(*dot_prod, area_sign0 * area_sign1 * ref_math_dot(n0, n1));
    }
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_verify_param(REF_GRID ref_grid) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_INT geom;
  REF_INT node;
  REF_DBL xyz[3];
  REF_DBL dist, max, max_node, max_edge, global_max;
  REF_BOOL node_constraint, edge_constraint;

  if (!ref_geom_model_loaded(ref_geom)) return REF_SUCCESS;

  max = 0.0;
  each_ref_geom_node(ref_geom, geom) {
    node = ref_geom_node(ref_geom, geom);
    if (ref_mpi_rank(ref_mpi) != ref_node_part(ref_node, node)) continue;
    RSS(ref_geom_eval(ref_geom, geom, xyz, NULL), "eval xyz");
    dist = sqrt(pow(xyz[0] - ref_node_xyz(ref_node, 0, node), 2) +
                pow(xyz[1] - ref_node_xyz(ref_node, 1, node), 2) +
                pow(xyz[2] - ref_node_xyz(ref_node, 2, node), 2));
    max = MAX(max, dist);
  }
  RSS(ref_mpi_max(ref_mpi, &max, &global_max, REF_DBL_TYPE), "mpi max node");
  max = global_max;
  if (ref_grid_once(ref_grid)) printf("CAD topo node max eval dist %e\n", max);

  max = 0.0;
  max_node = 0.0;
  each_ref_geom_edge(ref_geom, geom) {
    node = ref_geom_node(ref_geom, geom);
    if (ref_mpi_rank(ref_mpi) != ref_node_part(ref_node, node)) continue;
    RSS(ref_geom_eval(ref_geom, geom, xyz, NULL), "eval xyz");
    dist = sqrt(pow(xyz[0] - ref_node_xyz(ref_node, 0, node), 2) +
                pow(xyz[1] - ref_node_xyz(ref_node, 1, node), 2) +
                pow(xyz[2] - ref_node_xyz(ref_node, 2, node), 2));
    RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_NODE, &node_constraint), "n");
    if (node_constraint) {
      max_node = MAX(max_node, dist);
    } else {
      max = MAX(max, dist);
    }
  }
  RSS(ref_mpi_max(ref_mpi, &max, &global_max, REF_DBL_TYPE), "mpi max edge");
  max = global_max;
  if (ref_grid_once(ref_grid)) printf("CAD topo edge max eval dist %e\n", max);
  RSS(ref_mpi_max(ref_mpi, &max_node, &global_max, REF_DBL_TYPE),
      "mpi max edge");
  max_node = global_max;
  if (ref_grid_once(ref_grid)) printf("CAD topo edge node tol %e\n", max_node);

  max = 0.0;
  max_node = 0.0;
  max_edge = 0.0;
  each_ref_geom_face(ref_geom, geom) {
    node = ref_geom_node(ref_geom, geom);
    if (ref_mpi_rank(ref_mpi) != ref_node_part(ref_node, node)) continue;
    RSS(ref_geom_eval(ref_geom, geom, xyz, NULL), "eval xyz");
    dist = sqrt(pow(xyz[0] - ref_node_xyz(ref_node, 0, node), 2) +
                pow(xyz[1] - ref_node_xyz(ref_node, 1, node), 2) +
                pow(xyz[2] - ref_node_xyz(ref_node, 2, node), 2));
    RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_NODE, &node_constraint), "n");
    RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_EDGE, &edge_constraint), "n");
    if (node_constraint) {
      max_node = MAX(max_node, dist);
    } else {
      if (edge_constraint) {
        max_edge = MAX(max_edge, dist);
      } else {
        max = MAX(max, dist);
      }
    }
  }
  RSS(ref_mpi_max(ref_mpi, &max, &global_max, REF_DBL_TYPE), "mpi max face");
  max = global_max;
  if (ref_grid_once(ref_grid)) printf("CAD topo face max eval dist %e\n", max);
  RSS(ref_mpi_max(ref_mpi, &max_edge, &global_max, REF_DBL_TYPE),
      "mpi max edge");
  max_edge = global_max;
  if (ref_grid_once(ref_grid)) printf("CAD topo face edge tol %e\n", max_edge);
  RSS(ref_mpi_max(ref_mpi, &max_node, &global_max, REF_DBL_TYPE),
      "mpi max edge");
  max_node = global_max;
  if (ref_grid_once(ref_grid)) printf("CAD topo face node tol %e\n", max_node);

  return REF_SUCCESS;
}

REF_STATUS ref_geom_verify_topo(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_CELL ref_cell;
  REF_INT node;
  REF_INT item, geom;
  REF_BOOL geom_node, geom_edge, geom_face;
  REF_BOOL no_face, no_edge;
  REF_BOOL found_one;
  REF_BOOL found_too_many;
  REF_INT cell, ncell, cell_list[2];

  for (node = 0; node < ref_node_max(ref_node); node++) {
    if (ref_node_valid(ref_node, node)) {
      RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_NODE, &geom_node), "node");
      RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_EDGE, &geom_edge), "edge");
      RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_FACE, &geom_face), "face");
      no_face = ref_cell_node_empty(ref_grid_tri(ref_grid), node) &&
                ref_cell_node_empty(ref_grid_qua(ref_grid), node);
      no_edge = ref_cell_node_empty(ref_grid_edg(ref_grid), node);
      if (geom_node) {
        if (no_edge && ref_node_owned(ref_node, node)) {
          THROW("geom node missing edge");
        }
        if (no_face && ref_node_owned(ref_node, node)) {
          THROW("geom node missing tri or qua");
        }
      }
      if (geom_edge) {
        if (no_edge && ref_node_owned(ref_node, node)) {
          RSS(ref_node_location(ref_node, node), "loc");
          RSS(ref_geom_tattle(ref_geom, node), "tatt");
          ref_cell = ref_grid_edg(ref_grid);
          each_ref_cell_having_node(ref_cell, node, item, cell) {
            printf("edge %d %d %d\n", ref_cell_c2n(ref_cell, 0, cell),
                   ref_cell_c2n(ref_cell, 1, cell),
                   ref_cell_c2n(ref_cell, 2, cell));
          }
          RSS(ref_geom_tec_para_shard(ref_grid, "ref_geom_topo_error"),
              "geom tec");
          THROW("geom edge missing edge");
        }
        if (no_face && ref_node_owned(ref_node, node)) {
          RSS(ref_node_location(ref_node, node), "loc");
          RSS(ref_geom_tattle(ref_geom, node), "tatt");
          RSS(ref_geom_tec_para_shard(ref_grid, "ref_geom_topo_error"),
              "geom tec");
          THROW("geom edge missing tri or qua");
        }
      }
      if (geom_face) {
        if (no_face && ref_node_owned(ref_node, node)) {
          printf("no face for geom\n");
          RSS(ref_node_location(ref_node, node), "loc");
          RSS(ref_geom_tattle(ref_geom, node), "tatt");
          RSS(ref_geom_tec_para_shard(ref_grid, "ref_geom_topo_error"),
              "geom tec");
          THROW("geom face missing tri or qua");
        }
      }
      if (!no_edge) {
        if (!geom_edge) {
          printf("no geom for edge\n");
          RSS(ref_node_location(ref_node, node), "loc");
          RSS(ref_geom_tattle(ref_geom, node), "tatt");
          RSS(ref_geom_tec_para_shard(ref_grid, "ref_geom_topo_error"),
              "geom tec");
          THROW("geom edge missing for edg");
        }
      }
      if (!no_face) {
        if (!geom_face && !ref_geom_meshlinked(ref_geom)) {
          printf("no geom for face\n");
          RSS(ref_node_location(ref_node, node), "loc");
          RSS(ref_geom_tattle(ref_geom, node), "tatt");
          RSS(ref_geom_tec_para_shard(ref_grid, "ref_geom_topo_error"),
              "geom tec");
          THROW("geom face missing tri or qua");
        }
      }
      if (geom_edge && !geom_node) {
        found_one = REF_FALSE;
        found_too_many = REF_FALSE;
        each_ref_geom_having_node(ref_geom, node, item, geom) {
          if (REF_GEOM_EDGE == ref_geom_type(ref_geom, geom)) {
            if (found_one) found_too_many = REF_TRUE;
            found_one = REF_TRUE;
          }
        }
        if (!found_one || found_too_many) {
          if (!found_one) printf("none found\n");
          if (found_too_many) printf("found too many\n");
          RSS(ref_node_location(ref_node, node), "loc");
          RSS(ref_geom_tattle(ref_geom, node), "tatt");
          RSS(ref_geom_tec_para_shard(ref_grid, "ref_geom_topo_error"),
              "geom tec");
          THROW("multiple geom edge away from geom node");
        }
      }
      if (geom_face && !geom_edge) {
        found_one = REF_FALSE;
        found_too_many = REF_FALSE;
        each_ref_adj_node_item_with_ref(ref_geom_adj(ref_geom), node, item,
                                        geom) {
          if (REF_GEOM_FACE == ref_geom_type(ref_geom, geom)) {
            if (found_one) found_too_many = REF_TRUE;
            found_one = REF_TRUE;
          }
        }
        if (!found_one || found_too_many) {
          if (!found_one) printf("none found\n");
          if (found_too_many) printf("found too many\n");
          RSS(ref_node_location(ref_node, node), "loc");
          RSS(ref_geom_tattle(ref_geom, node), "tatt");
          RSS(ref_geom_tec_para_shard(ref_grid, "ref_geom_topo_error"),
              "geom tec");
          THROW("multiple geom face away from geom edge");
        }
      }
    } else {
      if (!ref_adj_empty(ref_geom_adj(ref_geom), node))
        THROW("invalid node has geom");
    }
  }

  ref_cell = ref_grid_edg(ref_grid);
  each_ref_cell_valid_cell(ref_cell, cell) {
    RSS(ref_cell_list_with2(ref_cell, ref_cell_c2n(ref_cell, 0, cell),
                            ref_cell_c2n(ref_cell, 1, cell), 2, &ncell,
                            cell_list),
        "edge list for edge");
    if (2 == ncell) {
      printf("error: two edg found with same nodes\n");
      printf("edg %d n %d %d id %d\n", cell_list[0],
             ref_cell_c2n(ref_cell, 0, cell_list[0]),
             ref_cell_c2n(ref_cell, 1, cell_list[0]),
             ref_cell_c2n(ref_cell, 2, cell_list[0]));
      printf("edg %d n %d %d id %d\n", cell_list[1],
             ref_cell_c2n(ref_cell, 0, cell_list[1]),
             ref_cell_c2n(ref_cell, 1, cell_list[1]),
             ref_cell_c2n(ref_cell, 2, cell_list[1]));
      RSS(ref_node_location(ref_node, ref_cell_c2n(ref_cell, 0, cell)), "loc");
      RSS(ref_node_location(ref_node, ref_cell_c2n(ref_cell, 1, cell)), "loc");
      RSS(ref_geom_tattle(ref_geom, ref_cell_c2n(ref_cell, 0, cell)), "tatt");
      RSS(ref_geom_tattle(ref_geom, ref_cell_c2n(ref_cell, 1, cell)), "tatt");
      RSS(ref_geom_tec_para_shard(ref_grid, "ref_geom_topo_error"), "geom tec");
    }
    REIS(1, ncell, "expect only one edge cell for two nodes");
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_tetgen_volume(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  const char *stdout_name = "ref_geom_test_tetgen_stdout.txt";
  const char *poly_name = "ref_geom_test_tetgen.poly";
  const char *node_name = "ref_geom_test_tetgen.1.node";
  const char *ele_name = "ref_geom_test_tetgen.1.ele";
  const char *face_name = "ref_geom_test_tetgen.1.face";
  const char *edge_name = "ref_geom_test_tetgen.1.edge";
  char command[1024];
  FILE *file;
  REF_INT nnode, ndim, attr, mark;
  REF_GLOB global;
  REF_INT ntet, node_per;
  REF_INT node, nnode_surface, item, new_node;
  REF_DBL xyz[3], dist;
  REF_INT cell, new_cell, nodes[REF_CELL_MAX_SIZE_PER];
  int system_status;

  printf("%d surface nodes %d triangles\n", ref_node_n(ref_node),
         ref_cell_n(ref_grid_tri(ref_grid)));

  RSS(ref_export_by_extension(ref_grid, poly_name), "poly");

  sprintf(command, "tetgen -pMYq2.0/10O7/7zV %s < /dev/null > %s", poly_name,
          stdout_name);
  printf("%s\n", command);
  fflush(stdout);
  system_status = system(command);
  REIB(0, system_status, "tetgen failed", {
    printf("tec360 ref_geom_test_tetgen_geom.tec\n");
    ref_geom_tec(ref_grid, "ref_geom_test_tetgen_geom.tec");
    printf("tec360 ref_geom_test_tetgen_surf.tec\n");
    ref_export_tec_surf(ref_grid, "ref_geom_test_tetgen_surf.tec");
  });

  file = fopen(node_name, "r");
  if (NULL == (void *)file) printf("unable to open %s\n", node_name);
  RNS(file, "unable to open file");

  REIS(1, fscanf(file, "%d", &nnode), "node header nnode");
  REIS(1, fscanf(file, "%d", &ndim), "node header ndim");
  REIS(3, ndim, "not 3D");
  REIS(1, fscanf(file, "%d", &attr), "node header attr");
  REIS(0, attr, "nodes have attribute 3D");
  REIS(1, fscanf(file, "%d", &mark), "node header mark");
  REIS(0, mark, "nodes have mark");

  /* verify surface nodes */
  nnode_surface = ref_node_n(ref_node);
  for (node = 0; node < nnode_surface; node++) {
    REIS(1, fscanf(file, "%d", &item), "node item");
    RES(node, item, "node index");
    RES(1, fscanf(file, "%lf", &(xyz[0])), "x");
    RES(1, fscanf(file, "%lf", &(xyz[1])), "y");
    RES(1, fscanf(file, "%lf", &(xyz[2])), "z");
    dist = sqrt((xyz[0] - ref_node_xyz(ref_node, 0, node)) *
                    (xyz[0] - ref_node_xyz(ref_node, 0, node)) +
                (xyz[1] - ref_node_xyz(ref_node, 1, node)) *
                    (xyz[1] - ref_node_xyz(ref_node, 1, node)) +
                (xyz[2] - ref_node_xyz(ref_node, 2, node)) *
                    (xyz[2] - ref_node_xyz(ref_node, 2, node)));
    if (dist > 1.0e-12) {
      printf("node %d off by %e\n", node, dist);
      THROW("tetgen moved node");
    }
  }

  /* interior nodes */
  for (node = nnode_surface; node < nnode; node++) {
    REIS(1, fscanf(file, "%d", &item), "node item");
    REIS(node, item, "file node index");
    RSS(ref_node_next_global(ref_node, &global), "next global");
    REIS(node, global, "global node index");
    RSS(ref_node_add(ref_node, global, &new_node), "new_node");
    RES(node, new_node, "node index");
    RES(1, fscanf(file, "%lf", &(xyz[0])), "x");
    RES(1, fscanf(file, "%lf", &(xyz[1])), "y");
    RES(1, fscanf(file, "%lf", &(xyz[2])), "z");
    ref_node_xyz(ref_node, 0, new_node) = xyz[0];
    ref_node_xyz(ref_node, 1, new_node) = xyz[1];
    ref_node_xyz(ref_node, 2, new_node) = xyz[2];
  }

  fclose(file);

  /* check faces when paranoid, but tetgen -z should not mess with them */

  file = fopen(ele_name, "r");
  if (NULL == (void *)file) printf("unable to open %s\n", ele_name);
  RNS(file, "unable to open file");

  REIS(1, fscanf(file, "%d", &ntet), "ele header ntet");
  REIS(1, fscanf(file, "%d", &node_per), "ele header node_per");
  REIS(4, node_per, "expected tets");
  REIS(1, fscanf(file, "%d", &mark), "ele header mark");
  REIS(0, mark, "ele have mark");

  ref_cell = ref_grid_tet(ref_grid);
  for (cell = 0; cell < ntet; cell++) {
    REIS(1, fscanf(file, "%d", &item), "tet item");
    RES(cell, item, "node index");
    for (node = 0; node < 4; node++)
      RES(1, fscanf(file, "%d", &(nodes[node])), "tet");
    RSS(ref_cell_add(ref_cell, nodes, &new_cell), "new tet");
    RES(cell, new_cell, "tet index");
  }

  fclose(file);

  ref_grid_surf(ref_grid) = REF_FALSE;

  REIS(0, remove(edge_name), "rm .edge tetgen output file");
  REIS(0, remove(face_name), "rm .face tetgen output file");
  REIS(0, remove(node_name), "rm .node tetgen output file");
  REIS(0, remove(ele_name), "rm .ele tetgen output file");
  REIS(0, remove(poly_name), "rm .poly tetgen input file");
  REIS(0, remove(stdout_name), "rm stdout tetgen output file");

  return REF_SUCCESS;
}

static REF_STATUS ref_import_ugrid_tets(REF_GRID ref_grid,
                                        const char *filename) {
  REF_CELL ref_cell;
  REF_NODE ref_node = ref_grid_node(ref_grid);
  FILE *file;
  REF_INT nnode, ntri, nqua, ntet, npyr, npri, nhex;
  REF_DBL xyz[3];
  REF_INT new_node, orig_nnode, node, tri;
  REF_GLOB global;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT qua;
  REF_INT face_id;
  REF_INT cell, new_cell;

  file = fopen(filename, "r");
  if (NULL == (void *)file) printf("unable to open %s\n", filename);
  RNS(file, "unable to open file");

  RES(1, fscanf(file, "%d", &nnode), "nnode");
  RES(1, fscanf(file, "%d", &ntri), "ntri");
  RES(1, fscanf(file, "%d", &nqua), "nqua");
  RES(1, fscanf(file, "%d", &ntet), "ntet");
  RES(1, fscanf(file, "%d", &npyr), "npyr");
  RES(1, fscanf(file, "%d", &npri), "npri");
  RES(1, fscanf(file, "%d", &nhex), "nhex");

  orig_nnode = ref_node_n(ref_node);

  for (node = 0; node < nnode; node++) {
    REIS(1, fscanf(file, "%lf", &(xyz[0])), "x");
    REIS(1, fscanf(file, "%lf", &(xyz[1])), "y");
    REIS(1, fscanf(file, "%lf", &(xyz[2])), "z");
    if (node >= orig_nnode) {
      RSS(ref_node_next_global(ref_node, &global), "next global");
      REIS(node, global, "global node index");
      RSS(ref_node_add(ref_node, global, &new_node), "new_node");
      REIS(node, new_node, "node index");
      ref_node_xyz(ref_node, 0, new_node) = xyz[0];
      ref_node_xyz(ref_node, 1, new_node) = xyz[1];
      ref_node_xyz(ref_node, 2, new_node) = xyz[2];
    }
  }

  for (tri = 0; tri < ntri; tri++) {
    for (node = 0; node < 3; node++)
      RES(1, fscanf(file, "%d", &(nodes[node])), "tri");
  }
  for (qua = 0; qua < nqua; qua++) {
    for (node = 0; node < 4; node++)
      RES(1, fscanf(file, "%d", &(nodes[node])), "qua");
  }

  for (tri = 0; tri < ntri; tri++) {
    RES(1, fscanf(file, "%d", &face_id), "tri id");
  }

  for (qua = 0; qua < nqua; qua++) {
    RES(1, fscanf(file, "%d", &face_id), "qua id");
  }

  ref_cell = ref_grid_tet(ref_grid);
  for (cell = 0; cell < ntet; cell++) {
    for (node = 0; node < 4; node++)
      RES(1, fscanf(file, "%d", &(nodes[node])), "tet");
    nodes[0]--;
    nodes[1]--;
    nodes[2]--;
    nodes[3]--;
    RSS(ref_cell_add(ref_cell, nodes, &new_cell), "new tet");
    RES(cell, new_cell, "tet index");
  }

  fclose(file);

  return REF_SUCCESS;
}

REF_STATUS ref_geom_aflr_volume(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  const char *surface_ugrid_name = "ref_geom_test_surface.lb8.ugrid";
  const char *volume_ugrid_name = "ref_geom_test_volume.ugrid";
  char command[1024];
  int system_status;

  printf("%d surface nodes %d triangles\n", ref_node_n(ref_node),
         ref_cell_n(ref_grid_tri(ref_grid)));

  printf("tec360 ref_geom_test_aflr_geom.tec\n");
  RSS(ref_geom_tec(ref_grid, "ref_geom_test_aflr_geom.tec"), "dbg geom");
  printf("tec360 ref_geom_test_aflr_surf.tec\n");
  RSS(ref_export_tec_surf(ref_grid, "ref_geom_test_aflr_surf.tec"), "dbg surf");
  RSS(ref_export_by_extension(ref_grid, surface_ugrid_name), "ugrid");
  sprintf(command,
          "aflr3 -igrid %s -ogrid %s -mrecrbf=0 -angqbf=179.9 -angqbfmin=0.1 "
          "< /dev/null > %s.out",
          surface_ugrid_name, volume_ugrid_name, volume_ugrid_name);
  printf("%s\n", command);
  fflush(stdout);
  system_status = system(command);
  REIS(0, system_status, "aflr failed");

  RSS(ref_import_ugrid_tets(ref_grid, volume_ugrid_name), "tets only");

  ref_grid_surf(ref_grid) = REF_FALSE;

  return REF_SUCCESS;
}

REF_STATUS ref_geom_infer_nedge_nface(REF_GRID ref_grid) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_INT min_id, max_id;
  RSS(ref_cell_id_range(ref_grid_tri(ref_grid), ref_grid_mpi(ref_grid), &min_id,
                        &max_id),
      "face range");
  REIS(1, min_id, "first face id not 1");
  ref_geom->nface = max_id;
  RSS(ref_cell_id_range(ref_grid_edg(ref_grid), ref_grid_mpi(ref_grid), &min_id,
                        &max_id),
      "edge range");
  REIS(1, min_id, "first edge id not 1");
  ref_geom->nedge = max_id;
  return REF_SUCCESS;
}

REF_STATUS ref_geom_egads_diagonal(REF_GEOM ref_geom, REF_DBL *diag) {
#ifdef HAVE_EGADS
  ego solid;
  double box[6];
  solid = (ego)(ref_geom->solid);

  RNS(solid, "EGADS solid object is NULL. Has the geometry been loaded?");

  REIS(EGADS_SUCCESS, EG_getBoundingBox(solid, box), "EG bounding box");
  *diag = sqrt((box[0] - box[3]) * (box[0] - box[3]) +
               (box[1] - box[4]) * (box[1] - box[4]) +
               (box[2] - box[5]) * (box[2] - box[5]));

#else
  printf("returning 1.0 from %s, No EGADS\n", __func__);
  *diag = 1.0;
  SUPRESS_UNUSED_COMPILER_WARNING(ref_geom);
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_geom_diagonal(REF_GEOM ref_geom, REF_INT geom, REF_DBL *diag) {
#ifdef HAVE_EGADS
  ego object;
  double box[6];
  switch (ref_geom_type(ref_geom, geom)) {
    case REF_GEOM_EDGE:
      object = ((ego *)(ref_geom->edges))[ref_geom_id(ref_geom, geom) - 1];
      break;
    case REF_GEOM_FACE:
      object = ((ego *)(ref_geom->faces))[ref_geom_id(ref_geom, geom) - 1];
      break;
    default:
      *diag = 0.0; /* for node */
      return REF_SUCCESS;
  }

  REIS(EGADS_SUCCESS, EG_getBoundingBox(object, box), "EG bounding box");
  *diag = sqrt((box[0] - box[3]) * (box[0] - box[3]) +
               (box[1] - box[4]) * (box[1] - box[4]) +
               (box[2] - box[5]) * (box[2] - box[5]));

#else
  printf("returning 1.0 from %s, No EGADS\n", __func__);
  *diag = 1.0;
  SUPRESS_UNUSED_COMPILER_WARNING(ref_geom);
  SUPRESS_UNUSED_COMPILER_WARNING(geom);
#endif

  return REF_SUCCESS;
}

REF_STATUS ref_geom_feature_size(REF_GRID ref_grid, REF_INT node, REF_DBL *h0,
                                 REF_DBL *dir0, REF_DBL *h1, REF_DBL *dir1,
                                 REF_DBL *h2, REF_DBL *dir2) {
#ifdef HAVE_EGADS
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_INT edge_item, face_item, edge_geom, face_geom, edgeid, faceid, iloop;
  ego ref, *cadnodes, *edges, *loops;
  int oclass, mtype, ncadnode, nedge, nloop, *senses;
  double data[18];
  double trange[2];
  REF_DBL xyz[3];
  REF_DBL dxyz_dt[6];
  REF_DBL xyz1[3];
  REF_INT ineligible_cad_node0, ineligible_cad_node1;
  REF_INT cad_node0, cad_node1;
  REF_DBL param[2];
  REF_INT other_edgeid, iedge;
  REF_DBL len, diagonal;
  REF_DBL tangent[3], dx[3], dot, orth[3];
  REF_DBL gap;
  int status;

  /* initialize isotropic with bounding box */
  RSS(ref_geom_egads_diagonal(ref_geom, &diagonal), "bbox diag init");
  *h0 = diagonal;
  dir0[0] = 1.0;
  dir0[1] = 0.0;
  dir0[2] = 0.0;
  *h1 = diagonal;
  dir1[0] = 0.0;
  dir1[1] = 1.0;
  dir1[2] = 0.0;
  *h2 = diagonal;
  dir2[0] = 0.0;
  dir2[1] = 0.0;
  dir2[2] = 1.0;

  each_ref_geom_having_node(ref_geom, node, edge_item, edge_geom) {
    if (REF_GEOM_EDGE == ref_geom_type(ref_geom, edge_geom)) {
      edgeid = ref_geom_id(ref_geom, edge_geom);
      REIS(EGADS_SUCCESS,
           EG_getTopology(((ego *)(ref_geom->edges))[edgeid - 1], &ref, &oclass,
                          &mtype, trange, &ncadnode, &cadnodes, &senses),
           "EG topo edge");
      RAS(mtype != DEGENERATE, "edge interior DEGENERATE");
      RSS(ref_geom_eval(ref_geom, edge_geom, xyz, dxyz_dt), "eval edge");
      tangent[0] = dxyz_dt[0];
      tangent[1] = dxyz_dt[1];
      tangent[2] = dxyz_dt[2];
      if (REF_SUCCESS != ref_math_normalize(tangent)) {
        tangent[0] = 0.0;
        tangent[1] = 0.0;
        tangent[2] = 0.0;
      }
      RAS(0 < ncadnode && ncadnode < 3, "edge children");
      ineligible_cad_node0 = EG_indexBodyTopo(ref_geom->solid, cadnodes[0]);
      if (2 == ncadnode) {
        ineligible_cad_node1 = EG_indexBodyTopo(ref_geom->solid, cadnodes[1]);
      } else {
        ineligible_cad_node1 = ineligible_cad_node0; /* ONENODE edge */
      }
      each_ref_geom_having_node(ref_geom, node, face_item, face_geom) {
        if (REF_GEOM_FACE == ref_geom_type(ref_geom, face_geom)) {
          faceid = ref_geom_id(ref_geom, face_geom);
          REIS(EGADS_SUCCESS,
               EG_getTopology(((ego *)(ref_geom->faces))[faceid - 1], &ref,
                              &oclass, &mtype, data, &nloop, &loops, &senses),
               "topo");
          for (iloop = 0; iloop < nloop; iloop++) {
            /* loop through all Edges associated with this Loop */
            REIS(EGADS_SUCCESS,
                 EG_getTopology(loops[iloop], &ref, &oclass, &mtype, data,
                                &nedge, &edges, &senses),
                 "topo");
            for (iedge = 0; iedge < nedge; iedge++) {
              other_edgeid =
                  EG_indexBodyTopo((ego)(ref_geom->solid), edges[iedge]);
              /* qualified? does not share geom nodes */
              REIS(EGADS_SUCCESS,
                   EG_getTopology(((ego *)(ref_geom->edges))[other_edgeid - 1],
                                  &ref, &oclass, &mtype, trange, &ncadnode,
                                  &cadnodes, &senses),
                   "EG topo node");
              if (mtype == DEGENERATE) continue; /* skip DEGENERATE */
              RAS(0 < ncadnode && ncadnode < 3, "edge children");
              cad_node0 = EG_indexBodyTopo(ref_geom->solid, cadnodes[0]);
              if (2 == ncadnode) {
                cad_node1 = EG_indexBodyTopo(ref_geom->solid, cadnodes[1]);
              } else {
                cad_node1 = cad_node0; /* ONENODE edge */
              }
              if (cad_node0 == ineligible_cad_node0 ||
                  cad_node0 == ineligible_cad_node1 ||
                  cad_node1 == ineligible_cad_node0 ||
                  cad_node1 == ineligible_cad_node1)
                continue;
              /* inverse projection */
              status =
                  EG_invEvaluate(((ego *)(ref_geom->edges))[other_edgeid - 1],
                                 xyz, param, xyz1);
              REIS(EGADS_SUCCESS, status, "EG_invEvaluate opp edge");
              dx[0] = xyz1[0] - xyz[0];
              dx[1] = xyz1[1] - xyz[1];
              dx[2] = xyz1[2] - xyz[2];
              len = sqrt(dx[0] * dx[0] + dx[1] * dx[1] + dx[2] * dx[2]);
              RSS(ref_geom_gap(ref_geom, node, &gap), "edge gap");
              len = MAX(len, gap);
              RSS(ref_math_normalize(dx), "direction across face");
              if (len < *h0) {
                dot = ref_math_dot(tangent, dx);
                orth[0] = tangent[0] - dot * dx[0];
                orth[1] = tangent[1] - dot * dx[1];
                orth[2] = tangent[2] - dot * dx[2];
                if (REF_SUCCESS == ref_math_normalize(orth)) {
                  *h0 = len;
                  dir0[0] = dx[0];
                  dir0[1] = dx[1];
                  dir0[2] = dx[2];
                  RSS(ref_geom_diagonal(ref_geom, edge_geom, h1), "local diag");
                  dir1[0] = orth[0];
                  dir1[1] = orth[1];
                  dir1[2] = orth[2];
                  *h2 = diagonal;
                  ref_math_cross_product(dir0, dir1, dir2);
                } else {
                  *h0 = len;
                  dir0[0] = dx[0];
                  dir0[1] = dx[1];
                  dir0[2] = dx[2];
                  *h1 = diagonal;
                  *h2 = diagonal;
                  RSS(ref_math_orthonormal_system(dir0, dir1, dir2),
                      "arbitrary orthonormal dir1 dir2");
                }
              }
            }
          }
        }
      }
    }
  }
#else
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  RSS(ref_geom_egads_diagonal(ref_geom, h0), "bbox diag init");
  dir0[0] = 1.0;
  dir0[1] = 0.0;
  dir0[2] = 0.0;
  *h1 = *h0;
  *h2 = *h0;
  RSS(ref_math_orthonormal_system(dir0, dir1, dir2),
      "arbitrary orthonormal dir1 dir2");
  SUPRESS_UNUSED_COMPILER_WARNING(node);
#endif
  return REF_SUCCESS;
}

REF_STATUS ref_geom_tolerance(REF_GEOM ref_geom, REF_INT type, REF_INT id,
                              REF_DBL *tolerance) {
#ifdef HAVE_EGADS
  ego object, *objects;
  double tol;

  object = (ego)NULL;
  switch (type) {
    case REF_GEOM_NODE:
      if (id < 1 || id > ref_geom->nnode) return REF_INVALID;
      objects = (ego *)(ref_geom->nodes);
      object = objects[id - 1];
      break;
    case REF_GEOM_EDGE:
      if (id < 1 || id > ref_geom->nedge) return REF_INVALID;
      objects = (ego *)(ref_geom->edges);
      object = objects[id - 1];
      break;
    case REF_GEOM_FACE:
      if (id < 1 || id > ref_geom->nface) return REF_INVALID;
      objects = (ego *)(ref_geom->faces);
      object = objects[id - 1];
      break;
    case REF_GEOM_SOLID:
      object = (ego)(ref_geom->solid);
      break;
    default:
      printf("ref_geom type %d unknown\n", type);
      RSS(REF_IMPLEMENT, "unknown surface type");
  }

  REIS(EGADS_SUCCESS, EG_getTolerance(object, &tol), "EG tolerance");
  *tolerance = tol;

#else
  SUPRESS_UNUSED_COMPILER_WARNING(ref_geom);
  SUPRESS_UNUSED_COMPILER_WARNING(type);
  SUPRESS_UNUSED_COMPILER_WARNING(id);
  *tolerance = -1.0;
#endif
  return REF_SUCCESS;
}

REF_STATUS ref_geom_gap(REF_GEOM ref_geom, REF_INT node, REF_DBL *gap) {
  REF_INT item, geom, type;
  REF_DBL dist, face_xyz[3], gap_xyz[3];
  REF_BOOL has_node, has_edge;
  *gap = 0.0;

  RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_NODE, &has_node), "n");
  RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_EDGE, &has_edge), "n");
  if (!has_edge) return REF_SUCCESS;

  if (has_node) {
    type = REF_GEOM_NODE;
  } else {
    type = REF_GEOM_FACE;
  }
  each_ref_geom_having_node(ref_geom, node, item, geom) {
    if (type == ref_geom_type(ref_geom, geom)) {
      RSS(ref_geom_eval(ref_geom, geom, gap_xyz, NULL), "eval");
    }
  }

  each_ref_geom_having_node(ref_geom, node, item, geom) {
    if (REF_GEOM_FACE == ref_geom_type(ref_geom, geom)) {
      RSS(ref_geom_eval(ref_geom, geom, face_xyz, NULL), "eval");
      dist = sqrt(pow(face_xyz[0] - gap_xyz[0], 2) +
                  pow(face_xyz[1] - gap_xyz[1], 2) +
                  pow(face_xyz[2] - gap_xyz[2], 2));
      (*gap) = MAX((*gap), dist);
    }
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_reliability(REF_GEOM ref_geom, REF_INT geom,
                                REF_DBL *slop) {
  REF_DBL tol, gap;
  *slop = 0.0;
  RSS(ref_geom_tolerance(ref_geom, ref_geom_type(ref_geom, geom),
                         ref_geom_id(ref_geom, geom), &tol),
      "tol");
  *slop = MAX(*slop, ref_geom_tolerance_protection(ref_geom) * tol);
  /*
    each_ref_geom_having_node(ref_geom, node, item, geom) {
      RSS(ref_geom_tolerance(ref_geom, ref_geom_type(ref_geom, geom),
                             ref_geom_id(ref_geom, geom), &tol),
          "tol");
      *slop = MAX(*slop, ref_geom_tolerance_protection(ref_geom) * tol);
    }
  */
  RSS(ref_geom_gap(ref_geom, ref_geom_node(ref_geom, geom), &gap), "gap");
  *slop = MAX(*slop, ref_geom_gap_protection(ref_geom) * gap);
  return REF_SUCCESS;
}

static REF_STATUS ref_geom_node_min_angle(REF_GRID ref_grid, REF_INT node,
                                          REF_DBL *angle) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_edg(ref_grid);
  REF_INT item1, cell1, node1;
  REF_INT item2, cell2, node2;
  REF_INT i;
  REF_DBL dot, dx1[3], dx2[3];
  *angle = 180.0;
  each_ref_cell_having_node(ref_cell, node, item1, cell1) {
    each_ref_cell_having_node(ref_cell, node, item2, cell2) {
      if (cell1 == cell2) continue; /* skip same edge */
      node1 = ref_cell_c2n(ref_cell, 0, cell1) +
              ref_cell_c2n(ref_cell, 1, cell1) - node;
      node2 = ref_cell_c2n(ref_cell, 0, cell2) +
              ref_cell_c2n(ref_cell, 1, cell2) - node;
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

static REF_STATUS ref_geom_node_short_edge(REF_GRID ref_grid, REF_INT node,
                                           REF_DBL *short_edge,
                                           REF_DBL *short_diag,
                                           REF_INT *short_id) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_INT item, geom;
  REF_DBL diag, min_diag, max_diag;
  *short_edge = 1.0;
  *short_diag = REF_DBL_MAX;
  *short_id = REF_EMPTY;
  min_diag = REF_DBL_MAX;
  max_diag = REF_DBL_MIN;
  each_ref_geom_having_node(ref_geom, node, item, geom) {
    if (REF_GEOM_EDGE != ref_geom_type(ref_geom, geom)) continue;
    RSS(ref_geom_diagonal(ref_geom, geom, &diag), "edge diag");
    if (diag < min_diag) {
      min_diag = diag;
      *short_diag = diag;
      *short_id = ref_geom_id(ref_geom, geom);
    }
    max_diag = MAX(diag, max_diag);
  }
  if (ref_math_divisible(min_diag, max_diag)) {
    *short_edge = min_diag / max_diag;
  }
  return REF_SUCCESS;
}

static REF_STATUS ref_geom_face_curve_tol(REF_GRID ref_grid, REF_INT faceid,
                                          REF_DBL *curve) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);

  REF_INT edge_geom, node;
  REF_INT item, face_geom;
  REF_DBL hmax, delta_radian, rlimit;
  REF_DBL kr, r[3], ks, s[3];
  REF_DBL hr, hs, slop;
  REF_DBL curvature_ratio = 1.0 / 20.0;

  *curve = 2.0;

  RSS(ref_geom_egads_diagonal(ref_geom, &hmax), "bbox diag");

  each_ref_geom_face(ref_geom, face_geom) {
    if (faceid != ref_geom_id(ref_geom, face_geom)) continue;
    node = ref_geom_node(ref_geom, face_geom);
    each_ref_geom_having_node(ref_geom, node, item, edge_geom) {
      if (REF_GEOM_EDGE != ref_geom_type(ref_geom, edge_geom)) continue;
      RSS(ref_geom_radian_request(ref_geom, edge_geom, &delta_radian), "drad");
      rlimit = hmax / delta_radian; /* h = r*drad, r = h/drad */
      RSS(ref_geom_face_curvature(ref_geom, face_geom, &kr, r, &ks, s),
          "curve");
      /* ignore sign, k is 1 / radius */
      kr = ABS(kr);
      ks = ABS(ks);
      /* limit the aspect ratio of the metric by reducing the largest radius */
      kr = MAX(kr, curvature_ratio * ks);
      ks = MAX(ks, curvature_ratio * kr);
      hr = hmax;
      if (1.0 / rlimit < kr) hr = delta_radian / kr;
      hs = hmax;
      if (1.0 / rlimit < ks) hs = delta_radian / ks;

      RSS(ref_geom_reliability(ref_geom, face_geom, &slop), "edge tol");
      if (hr < slop) {
        *curve = MIN(*curve, hr / slop);
      }
      if (hs < slop) {
        *curve = MIN(*curve, hs / slop);
      }
    }
  }
  return REF_SUCCESS;
}

REF_STATUS ref_geom_feedback(REF_GRID ref_grid) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT geom, node, edgeid, faceid;
  REF_DBL angle_tol = 10.0;
  REF_DBL angle;
  REF_DBL short_edge_tol = 1e-3;
  REF_DBL short_edge, diag;
  REF_DBL curve;
  each_ref_geom_node(ref_geom, geom) {
    node = ref_geom_node(ref_geom, geom);
    RSS(ref_geom_node_min_angle(ref_grid, node, &angle), "node angle");
    if (angle <= angle_tol) {
      printf("%f %f %f # sliver deg=%f node %d\n",
             ref_node_xyz(ref_node, 0, node), ref_node_xyz(ref_node, 1, node),
             ref_node_xyz(ref_node, 2, node), angle,
             ref_geom_id(ref_geom, geom));
    }
  }
  if (ref_geom_model_loaded(ref_geom)) {
    each_ref_geom_node(ref_geom, geom) {
      node = ref_geom_node(ref_geom, geom);
      RSS(ref_geom_node_short_edge(ref_grid, node, &short_edge, &diag, &edgeid),
          "short edge");
      if (short_edge <= short_edge_tol) {
        printf("%f %f %f # short edge a=%e r=%e id %d\n",
               ref_node_xyz(ref_node, 0, node), ref_node_xyz(ref_node, 1, node),
               ref_node_xyz(ref_node, 2, node), diag, short_edge, edgeid);
      }
    }
  }
  if (ref_geom_model_loaded(ref_geom)) {
    for (faceid = 1; faceid <= ref_geom->nedge; faceid++) {
      RSS(ref_geom_face_curve_tol(ref_grid, faceid, &curve), "curved face");
      if (curve < 1.0) {
#ifdef HAVE_EGADS
        ego object = ((ego *)(ref_geom->faces))[faceid - 1];
        double box[6];
        REIS(EGADS_SUCCESS, EG_getBoundingBox(object, box), "EG bounding box");
        printf("%f %f %f # box corner with curve/tol %e\n", box[0], box[1],
               box[2], curve);
        printf("%f %f %f # box corner for face id %d\n", box[3], box[4], box[5],
               faceid);
#else
        printf("# face id %d curve/tol %e\n", faceid, curve);
#endif
      }
    }
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_has_jump(REF_GEOM ref_geom, REF_INT node,
                             REF_BOOL *has_jump) {
  REF_INT item, geom;
  *has_jump = REF_FALSE;
  each_ref_geom_having_node(ref_geom, node, item, geom) {
    if (0 != ref_geom_jump(ref_geom, geom)) {
      *has_jump = REF_TRUE;
      return REF_SUCCESS;
    }
  }

  return REF_SUCCESS;
}

REF_STATUS ref_geom_edge_tec_zone(REF_GRID ref_grid, REF_INT id, FILE *file) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_edg(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_DICT ref_dict;
  REF_INT geom, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT item, local, node;
  REF_INT nnode, nedg, sens;
  REF_INT jump_geom = REF_EMPTY;
  REF_DBL *t, tvalue;
  REF_DBL radius, normal[3], xyz[3];

  RSS(ref_dict_create(&ref_dict), "create dict");

  each_ref_geom_edge(ref_geom, geom) {
    if (id == ref_geom_id(ref_geom, geom)) {
      RSS(ref_dict_store(ref_dict, ref_geom_node(ref_geom, geom), geom),
          "mark nodes");
      if (0 != ref_geom_jump(ref_geom, geom)) {
        REIS(REF_EMPTY, jump_geom, "should be only one jump per edge");
        jump_geom = geom;
      }
    }
  }
  nnode = ref_dict_n(ref_dict);
  if (REF_EMPTY != jump_geom) nnode++;

  nedg = 0;
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (id == nodes[2]) {
      nedg++;
    }
  }

  /* skip degenerate */
  if (0 == nnode || 0 == nedg) {
    RSS(ref_dict_free(ref_dict), "free dict");
    return REF_SUCCESS;
  }

  fprintf(
      file,
      "zone t=\"edge%d\", nodes=%d, elements=%d, datapacking=%s, zonetype=%s\n",
      id, nnode, nedg, "point", "felineseg");

  ref_malloc(t, nnode, REF_DBL);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (id == nodes[2]) {
      RSB(ref_dict_location(ref_dict, nodes[0], &local), "localize", {
        printf("edg %d %d id %d no edge geom\n", nodes[0], nodes[1], nodes[2]);
        RSS(ref_node_location(ref_node, nodes[0]), "loc");
        RSS(ref_geom_tattle(ref_geom, nodes[0]), "tatt");
      });
      RSS(ref_geom_cell_tuv(ref_geom, nodes[0], nodes, REF_GEOM_EDGE, &tvalue,
                            &sens),
          "from");
      if (-1 == sens) local = nnode - 1;
      t[local] = tvalue;
      RSS(ref_dict_location(ref_dict, nodes[1], &local), "localize");
      RSS(ref_geom_cell_tuv(ref_geom, nodes[1], nodes, REF_GEOM_EDGE, &tvalue,
                            &sens),
          "from");
      if (-1 == sens) local = nnode - 1;
      t[local] = tvalue;
    }
  }

  each_ref_dict_key_value(ref_dict, item, node, geom) {
    radius = 0;
    xyz[0] = ref_node_xyz(ref_node, 0, node);
    xyz[1] = ref_node_xyz(ref_node, 1, node);
    xyz[2] = ref_node_xyz(ref_node, 2, node);
    if (ref_geom_model_loaded(ref_geom)) {
      RSS(ref_geom_edge_curvature(ref_geom, geom, &radius, normal), "curve");
      radius = ABS(radius);
      RSS(ref_geom_eval_at(ref_geom, REF_GEOM_EDGE, id, &(t[item]), xyz, NULL),
          "eval at");
    }
    fprintf(file, " %.16e %.16e %.16e %.16e %.16e %.16e %.16e\n", xyz[0],
            xyz[1], xyz[2], t[item], 0.0, radius, 0.0);
  }
  if (REF_EMPTY != jump_geom) {
    node = ref_geom_node(ref_geom, jump_geom);
    radius = 0;
    xyz[0] = ref_node_xyz(ref_node, 0, node);
    xyz[1] = ref_node_xyz(ref_node, 1, node);
    xyz[2] = ref_node_xyz(ref_node, 2, node);
    if (ref_geom_model_loaded(ref_geom)) {
      RSS(ref_geom_edge_curvature(ref_geom, jump_geom, &radius, normal),
          "curve");
      radius = ABS(radius);
      RSS(ref_geom_eval_at(ref_geom, REF_GEOM_EDGE, id, &(t[nnode - 1]), xyz,
                           NULL),
          "eval at");
    }
    node = ref_geom_node(ref_geom, jump_geom);
    fprintf(file, " %.16e %.16e %.16e %.16e %.16e %.16e %.16e\n", xyz[0],
            xyz[1], xyz[2], t[nnode - 1], 0.0, radius, 0.0);
  }
  ref_free(t);

  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (id == nodes[2]) {
      RSS(ref_dict_location(ref_dict, nodes[0], &local), "localize");
      RSS(ref_geom_cell_tuv(ref_geom, nodes[0], nodes, REF_GEOM_EDGE, &tvalue,
                            &sens),
          "from");
      if (-1 == sens) local = nnode - 1;
      fprintf(file, " %d", local + 1);
      RSS(ref_dict_location(ref_dict, nodes[1], &local), "localize");
      RSS(ref_geom_cell_tuv(ref_geom, nodes[1], nodes, REF_GEOM_EDGE, &tvalue,
                            &sens),
          "from");
      if (-1 == sens) local = nnode - 1;
      fprintf(file, " %d", local + 1);
      fprintf(file, "\n");
    }
  }

  RSS(ref_dict_free(ref_dict), "free dict");

  return REF_SUCCESS;
}

REF_STATUS ref_geom_face_tec_zone(REF_GRID ref_grid, REF_INT id, FILE *file) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_DICT ref_dict, ref_dict_jump, ref_dict_degen;
  REF_INT geom, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT item2, item, local, node;
  REF_INT nnode, nnode_sens0, nnode_degen, ntri;
  REF_INT sens;
  REF_DBL *uv, param[2];
  REF_DBL kr, r[3], ks, s[3], xyz[3], kmin, kmax;

  RSS(ref_dict_create(&ref_dict), "create dict");
  RSS(ref_dict_create(&ref_dict_jump), "create dict");
  RSS(ref_dict_create(&ref_dict_degen), "create dict");

  each_ref_geom_face(ref_geom, geom) {
    node = ref_geom_node(ref_geom, geom);
    if (id == ref_geom_id(ref_geom, geom)) {
      if (0 == ref_geom_degen(ref_geom, geom)) {
        RSS(ref_dict_store(ref_dict, node, geom), "mark nodes");
        if (0 != ref_geom_jump(ref_geom, geom)) {
          RSS(ref_dict_store(ref_dict_jump, node, geom), "mark jump");
        }
      } else {
        each_ref_cell_having_node(ref_cell, node, item, cell) {
          RSS(ref_cell_nodes(ref_cell, cell, nodes), "nodes");
          if (id == nodes[3]) {
            RSS(ref_dict_store(ref_dict_degen, cell, node), "mark degen");
          }
        }
      }
    }
  }

  nnode_sens0 = ref_dict_n(ref_dict);
  nnode_degen = ref_dict_n(ref_dict) + ref_dict_n(ref_dict_jump);
  nnode = ref_dict_n(ref_dict) + ref_dict_n(ref_dict_jump) +
          ref_dict_n(ref_dict_degen);

  ntri = 0;
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (id == nodes[3]) {
      ntri++;
    }
  }

  /* skip degenerate */
  if (0 == nnode || 0 == ntri) {
    RSS(ref_dict_free(ref_dict_degen), "free degen");
    RSS(ref_dict_free(ref_dict_jump), "free jump");
    RSS(ref_dict_free(ref_dict), "free dict");
    return REF_SUCCESS;
  }

  fprintf(
      file,
      "zone t=\"face%d\", nodes=%d, elements=%d, datapacking=%s, zonetype=%s\n",
      id, nnode, ntri, "point", "fetriangle");

  ref_malloc_init(uv, 2 * nnode, REF_DBL, -1.0);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (id == nodes[3]) {
      each_ref_cell_cell_node(ref_cell, node) {
        RSS(ref_geom_find(ref_geom, nodes[node], REF_GEOM_FACE, id, &geom),
            "find");
        RSS(ref_geom_cell_tuv(ref_geom, nodes[node], nodes, REF_GEOM_FACE,
                              param, &sens),
            "cell tuv");
        if (0 == ref_geom_degen(ref_geom, geom)) {
          if (0 == sens || 1 == sens) {
            RSS(ref_dict_location(ref_dict, nodes[node], &local), "localize");
          } else {
            RSS(ref_dict_location(ref_dict_jump, nodes[node], &local),
                "localize");
            local += nnode_sens0;
          }
        } else {
          RSS(ref_dict_location(ref_dict_degen, cell, &local), "localize");
          local += nnode_degen;
        }
        uv[0 + 2 * local] = param[0];
        uv[1 + 2 * local] = param[1];
      }
    }
  }

  each_ref_dict_key_value(ref_dict, item, node, geom) {
    kr = 0;
    ks = 0;
    xyz[0] = ref_node_xyz(ref_node, 0, node);
    xyz[1] = ref_node_xyz(ref_node, 1, node);
    xyz[2] = ref_node_xyz(ref_node, 2, node);
    if (ref_geom_model_loaded(ref_geom)) {
      RSS(ref_geom_face_curvature(ref_geom, geom, &kr, r, &ks, s), "curve");
      RSS(ref_geom_eval_at(ref_geom, REF_GEOM_FACE, id, &(uv[2 * item]), xyz,
                           NULL),
          "eval at");
    }
    if (ref_geom_meshlinked(ref_geom)) {
      RSS(ref_meshlink_face_curvature(ref_grid, geom, &kr, r, &ks, s), "curve");
    }
    kmax = MAX(ABS(kr), ABS(ks));
    kmin = MIN(ABS(kr), ABS(ks));
    fprintf(file, " %.16e %.16e %.16e %.16e %.16e %.16e %.16e\n", xyz[0],
            xyz[1], xyz[2], uv[0 + 2 * item], uv[1 + 2 * item], kmax, kmin);
  }
  each_ref_dict_key_value(ref_dict_jump, item, node, geom) {
    kr = 0;
    ks = 0;
    xyz[0] = ref_node_xyz(ref_node, 0, node);
    xyz[1] = ref_node_xyz(ref_node, 1, node);
    xyz[2] = ref_node_xyz(ref_node, 2, node);
    if (ref_geom_model_loaded(ref_geom)) {
      RSS(ref_geom_face_curvature(ref_geom, geom, &kr, r, &ks, s), "curve");
      RSS(ref_geom_eval_at(ref_geom, REF_GEOM_FACE, id,
                           &(uv[2 * (nnode_sens0 + item)]), xyz, NULL),
          "eval at");
    }
    kmax = MAX(ABS(kr), ABS(ks));
    kmin = MAX(ABS(kr), ABS(ks));
    fprintf(file, " %.16e %.16e %.16e %.16e %.16e %.16e %.16e\n", xyz[0],
            xyz[1], xyz[2], uv[0 + 2 * (nnode_sens0 + item)],
            uv[1 + 2 * (nnode_sens0 + item)], kmax, kmin);
  }
  each_ref_dict_key_value(ref_dict_degen, item, cell, node) {
    kr = 0;
    ks = 0;
    xyz[0] = ref_node_xyz(ref_node, 0, node);
    xyz[1] = ref_node_xyz(ref_node, 1, node);
    xyz[2] = ref_node_xyz(ref_node, 2, node);
    if (ref_geom_model_loaded(ref_geom)) {
      each_ref_geom_having_node(ref_geom, node, item2, geom) {
        if (ref_geom_type(ref_geom, geom) == REF_GEOM_FACE &&
            ref_geom_id(ref_geom, geom) == id) {
          RSS(ref_geom_face_curvature(ref_geom, geom, &kr, r, &ks, s), "curve");
        }
      }
      RSS(ref_geom_eval_at(ref_geom, REF_GEOM_FACE, id,
                           &(uv[2 * (nnode_degen + item)]), xyz, NULL),
          "eval at");
    }
    kmax = MAX(ABS(kr), ABS(ks));
    kmin = MAX(ABS(kr), ABS(ks));
    fprintf(file, " %.16e %.16e %.16e %.16e %.16e %.16e %.16e\n", xyz[0],
            xyz[1], xyz[2], uv[0 + 2 * (nnode_degen + item)],
            uv[1 + 2 * (nnode_degen + item)], kmax, kmin);
  }
  ref_free(uv);

  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (id == nodes[3]) {
      each_ref_cell_cell_node(ref_cell, node) {
        RSS(ref_geom_find(ref_geom, nodes[node], REF_GEOM_FACE, id, &geom),
            "find");
        RSS(ref_geom_cell_tuv(ref_geom, nodes[node], nodes, REF_GEOM_FACE,
                              param, &sens),
            "cell tuv");
        if (0 == ref_geom_degen(ref_geom, geom)) {
          if (0 == sens || 1 == sens) {
            RSS(ref_dict_location(ref_dict, nodes[node], &local), "localize");
          } else {
            RSS(ref_dict_location(ref_dict_jump, nodes[node], &local),
                "localize");
            local += nnode_sens0;
          }
        } else {
          RSS(ref_dict_location(ref_dict_degen, cell, &local), "localize");
          local += nnode_degen;
        }
        fprintf(file, " %d", local + 1);
      }
      fprintf(file, "\n");
    }
  }

  RSS(ref_dict_free(ref_dict_degen), "free degen");
  RSS(ref_dict_free(ref_dict_jump), "free jump");
  RSS(ref_dict_free(ref_dict), "free dict");

  return REF_SUCCESS;
}

REF_STATUS ref_geom_norm_tec_zone(REF_GRID ref_grid, REF_INT id, FILE *file) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_DICT ref_dict;
  REF_INT geom, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT item, local, node;
  REF_INT nnode, ntri;
  REF_DBL r[3], s[3], n[3], uv[2];
  REF_DBL area_sign;

  RSS(ref_dict_create(&ref_dict), "create dict");

  each_ref_geom_face(ref_geom, geom) {
    if (id == ref_geom_id(ref_geom, geom)) {
      RSS(ref_dict_store(ref_dict, ref_geom_node(ref_geom, geom), geom),
          "mark nodes");
    }
  }
  nnode = ref_dict_n(ref_dict);

  ntri = 0;
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (id == nodes[3]) {
      ntri++;
    }
  }

  /* skip degenerate */
  if (0 == nnode || 0 == ntri) {
    RSS(ref_dict_free(ref_dict), "free dict");
    return REF_SUCCESS;
  }

  fprintf(
      file,
      "zone t=\"norm%d\", nodes=%d, elements=%d, datapacking=%s, zonetype=%s\n",
      id, nnode, ntri, "point", "fetriangle");

  each_ref_dict_key_value(ref_dict, item, node, geom) {
    RSS(ref_geom_find(ref_geom, node, REF_GEOM_FACE, id, &geom), "not found");
    uv[0] = ref_geom_param(ref_geom, 0, geom);
    uv[1] = ref_geom_param(ref_geom, 1, geom);
    RSS(ref_geom_face_rsn(ref_geom, id, uv, r, s, n), "rsn");
    RSS(ref_geom_uv_area_sign(ref_grid, id, &area_sign), "a sign");
    fprintf(file, " %.16e %.16e %.16e %.16e %.16e %.16e\n",
            ref_node_xyz(ref_node, 0, node), ref_node_xyz(ref_node, 1, node),
            ref_node_xyz(ref_node, 2, node), -area_sign * n[0],
            -area_sign * n[1], -area_sign * n[2]);
  }

  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (id == nodes[3]) {
      for (node = 0; node < 3; node++) {
        RSS(ref_dict_location(ref_dict, nodes[node], &local), "localize");
        fprintf(file, " %d", local + 1);
      }
      fprintf(file, "\n");
    }
  }

  RSS(ref_dict_free(ref_dict), "free dict");

  return REF_SUCCESS;
}

static REF_STATUS ref_geom_curve_tec_zone(REF_GRID ref_grid, REF_INT id,
                                          FILE *file) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_DICT ref_dict;
  REF_INT geom, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT item, local, node;
  REF_INT nnode, ntri;
  REF_DBL kr, r[3], ks, s[3];
  kr = 0;
  r[0] = 0;
  r[1] = 0;
  r[2] = 0;
  ks = 0;
  s[0] = 0;
  s[1] = 0;
  s[2] = 0;

  RSS(ref_dict_create(&ref_dict), "create dict");

  each_ref_geom_face(ref_geom, geom) {
    if (id == ref_geom_id(ref_geom, geom)) {
      RSS(ref_dict_store(ref_dict, ref_geom_node(ref_geom, geom), geom),
          "mark nodes");
    }
  }
  nnode = ref_dict_n(ref_dict);

  ntri = 0;
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (id == nodes[3]) {
      ntri++;
    }
  }

  /* skip degenerate */
  if (0 == nnode || 0 == ntri) {
    RSS(ref_dict_free(ref_dict), "free dict");
    return REF_SUCCESS;
  }

  fprintf(file,
          "zone t=\"curve%d\", nodes=%d, elements=%d, datapacking=%s, "
          "zonetype=%s\n",
          id, nnode, ntri, "point", "fetriangle");

  each_ref_dict_key_value(ref_dict, item, node, geom) {
    if (ref_geom_model_loaded(ref_geom)) {
      RSS(ref_geom_face_curvature(ref_geom, geom, &kr, r, &ks, s), "curve");
    }
    if (ref_geom_meshlinked(ref_geom)) {
      RSS(ref_meshlink_face_curvature(ref_grid, geom, &kr, r, &ks, s), "curve");
    }
    fprintf(file,
            " %.16e %.16e %.16e %.16e %.16e %.16e %.16e "
            "%.16e %.16e %.16e %.16e\n",
            ref_node_xyz(ref_node, 0, node), ref_node_xyz(ref_node, 1, node),
            ref_node_xyz(ref_node, 2, node), kr, r[0], r[1], r[2], ks, s[0],
            s[1], s[2]);
  }

  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (id == nodes[3]) {
      for (node = 0; node < 3; node++) {
        RSS(ref_dict_location(ref_dict, nodes[node], &local), "localize");
        fprintf(file, " %d", local + 1);
      }
      fprintf(file, "\n");
    }
  }

  RSS(ref_dict_free(ref_dict), "free dict");

  return REF_SUCCESS;
}

REF_STATUS ref_geom_tec(REF_GRID ref_grid, const char *filename) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  FILE *file;
  REF_INT geom, id, min_id, max_id;

  file = fopen(filename, "w");
  if (NULL == (void *)file) printf("unable to open %s\n", filename);
  RNS(file, "unable to open file");

  fprintf(file, "title=\"refine cad coupling in tecplot format\"\n");
  fprintf(file, "variables = \"x\" \"y\" \"z\" \"p0\" \"p1\" \"k0\" \"k1\"\n");

  min_id = REF_INT_MAX;
  max_id = REF_INT_MIN;
  each_ref_geom_edge(ref_geom, geom) {
    min_id = MIN(min_id, ref_geom_id(ref_geom, geom));
    max_id = MAX(max_id, ref_geom_id(ref_geom, geom));
  }

  for (id = min_id; id <= max_id; id++)
    RSS(ref_geom_edge_tec_zone(ref_grid, id, file), "tec edge");

  min_id = REF_INT_MAX;
  max_id = REF_INT_MIN;
  each_ref_geom_face(ref_geom, geom) {
    min_id = MIN(min_id, ref_geom_id(ref_geom, geom));
    max_id = MAX(max_id, ref_geom_id(ref_geom, geom));
  }

  for (id = min_id; id <= max_id; id++)
    RSS(ref_geom_face_tec_zone(ref_grid, id, file), "tec face");

  fclose(file);
  return REF_SUCCESS;
}

REF_STATUS ref_geom_curve_tec(REF_GRID ref_grid, const char *filename) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  FILE *file;
  REF_INT geom, id, min_id, max_id;

  file = fopen(filename, "w");
  if (NULL == (void *)file) printf("unable to open %s\n", filename);
  RNS(file, "unable to open file");

  fprintf(file, "title=\"refine cad coupling in tecplot format\"\n");
  fprintf(file,
          "variables = \"x\" \"y\" \"z\" \"kr\" \"rx\" \"ry\" \"rz\" "
          "\"ks\" \"sx\" \"sy\" \"sz\"\n");

  min_id = REF_INT_MAX;
  max_id = REF_INT_MIN;
  each_ref_geom_face(ref_geom, geom) {
    min_id = MIN(min_id, ref_geom_id(ref_geom, geom));
    max_id = MAX(max_id, ref_geom_id(ref_geom, geom));
  }

  for (id = min_id; id <= max_id; id++)
    RSS(ref_geom_curve_tec_zone(ref_grid, id, file), "tec face");

  fclose(file);
  return REF_SUCCESS;
}

REF_STATUS ref_geom_tec_para_shard(REF_GRID ref_grid,
                                   const char *root_filename) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  char filename[1024];
  if (ref_mpi_para(ref_mpi)) {
    sprintf(filename, "%s_%04d_%04d.tec", root_filename, ref_mpi_n(ref_mpi),
            ref_mpi_rank(ref_mpi));
  } else {
    sprintf(filename, "%s.tec", root_filename);
  }
  RSS(ref_geom_tec(ref_grid, filename), "tec");
  return REF_SUCCESS;
}

REF_STATUS ref_geom_ghost(REF_GEOM ref_geom, REF_NODE ref_node) {
  REF_MPI ref_mpi = ref_node_mpi(ref_node);
  REF_INT *a_nnode, *b_nnode;
  REF_INT a_nnode_total, b_nnode_total;
  REF_GLOB *a_global, *b_global;
  REF_INT *a_part, *b_part;
  REF_INT *a_ngeom, *b_ngeom;
  REF_INT a_ngeom_total, b_ngeom_total;
  REF_GLOB *a_descr, *b_descr;
  REF_GLOB global;
  REF_DBL *a_param, *b_param;
  REF_INT part, node, degree;
  REF_INT *a_next, *b_next;
  REF_INT local, item, geom, i;
  REF_INT descr[REF_GEOM_DESCR_SIZE];

  if (!ref_mpi_para(ref_mpi)) return REF_SUCCESS;

  ref_malloc_init(a_next, ref_mpi_n(ref_mpi), REF_INT, 0);
  ref_malloc_init(b_next, ref_mpi_n(ref_mpi), REF_INT, 0);
  ref_malloc_init(a_nnode, ref_mpi_n(ref_mpi), REF_INT, 0);
  ref_malloc_init(b_nnode, ref_mpi_n(ref_mpi), REF_INT, 0);
  ref_malloc_init(a_ngeom, ref_mpi_n(ref_mpi), REF_INT, 0);
  ref_malloc_init(b_ngeom, ref_mpi_n(ref_mpi), REF_INT, 0);

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
      "alltoallv global");

  for (node = 0; node < b_nnode_total; node++) {
    RSS(ref_node_local(ref_node, b_global[node], &local), "g2l");
    part = b_part[node];
    RSS(ref_adj_degree(ref_geom_adj(ref_geom), local, &degree), "deg");
    /* printf("%d: node %d global %d local %d part %d degree %d\n",
       ref_mpi_rank(ref_mpi), node,b_global[node], local, part, degree); */
    b_ngeom[part] += degree;
  }

  RSS(ref_mpi_alltoall(ref_mpi, b_ngeom, a_ngeom, REF_INT_TYPE),
      "alltoall ngeoms");

  a_ngeom_total = 0;
  each_ref_mpi_part(ref_mpi, part) a_ngeom_total += a_ngeom[part];
  ref_malloc(a_descr, REF_GEOM_DESCR_SIZE * a_ngeom_total, REF_GLOB);
  ref_malloc(a_param, 2 * a_ngeom_total, REF_DBL);

  b_ngeom_total = 0;
  each_ref_mpi_part(ref_mpi, part) b_ngeom_total += b_ngeom[part];
  ref_malloc(b_descr, REF_GEOM_DESCR_SIZE * b_ngeom_total, REF_GLOB);
  ref_malloc(b_param, 2 * b_ngeom_total, REF_DBL);

  b_next[0] = 0;
  each_ref_mpi_worker(ref_mpi, part) {
    b_next[part] = b_next[part - 1] + b_ngeom[part - 1];
  }

  for (node = 0; node < b_nnode_total; node++) {
    RSS(ref_node_local(ref_node, b_global[node], &local), "g2l");
    part = b_part[node];
    each_ref_geom_having_node(ref_geom, local, item, geom) {
      each_ref_descr(ref_geom, i) {
        b_descr[i + REF_GEOM_DESCR_SIZE * b_next[part]] =
            (REF_GLOB)ref_geom_descr(ref_geom, i, geom);
      }
      b_descr[REF_GEOM_DESCR_NODE + REF_GEOM_DESCR_SIZE * b_next[part]] =
          ref_node_global(ref_node, ref_geom_node(ref_geom, geom));
      b_param[0 + 2 * b_next[part]] = ref_geom_param(ref_geom, 0, geom);
      b_param[1 + 2 * b_next[part]] = ref_geom_param(ref_geom, 1, geom);
      b_next[part]++;
    }
  }

  RSS(ref_mpi_alltoallv(ref_mpi, b_descr, b_ngeom, a_descr, a_ngeom,
                        REF_GEOM_DESCR_SIZE, REF_GLOB_TYPE),
      "alltoallv descr");
  RSS(ref_mpi_alltoallv(ref_mpi, b_param, b_ngeom, a_param, a_ngeom, 2,
                        REF_DBL_TYPE),
      "alltoallv param");

  for (geom = 0; geom < a_ngeom_total; geom++) {
    each_ref_descr(ref_geom, i) {
      descr[i] = (REF_INT)a_descr[i + REF_GEOM_DESCR_SIZE * geom];
    }
    global = a_descr[REF_GEOM_DESCR_NODE + REF_GEOM_DESCR_SIZE * geom];
    RSS(ref_node_local(ref_node, global, &local), "g2l");
    descr[REF_GEOM_DESCR_NODE] = local;
    RSS(ref_geom_add_with_descr(ref_geom, descr, &(a_param[2 * geom])),
        "add ghost");
  }

  free(b_param);
  free(b_descr);
  free(a_param);
  free(a_descr);
  free(b_part);
  free(b_global);
  free(a_part);
  free(a_global);
  free(b_ngeom);
  free(a_ngeom);
  free(b_nnode);
  free(a_nnode);
  free(b_next);
  free(a_next);

  return REF_SUCCESS;
}

REF_STATUS ref_geom_face_match(REF_GRID ref_grid) {
#if defined(HAVE_EGADS) && !defined(HAVE_EGADS_LITE)
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  double *cad_box;
  double *face_box;
  double *cad_cga;
  double *face_cga;
  double massprop[14];
  REF_DBL area, centroid[3];
  ego face_ego;
  REF_INT face, faceid, min_faceid, max_faceid, nfaceid;
  REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT i, j;
  REF_DBL *face_norm;
  REF_DBL norm;
  REF_INT *candidates;
  REF_DICT ref_dict;
  REF_INT old_faceid, new_faceid;

  ref_malloc(cad_box, 6 * ref_geom->nface, double);
  ref_malloc(cad_cga, 4 * ref_geom->nface, double);
  for (face = 0; face < (ref_geom->nface); face++) {
    face_ego = ((ego *)(ref_geom->faces))[face];
    REIS(EGADS_SUCCESS, EG_getBoundingBox(face_ego, &(cad_box[6 * face])),
         "EG bounding box");
    REIS(EGADS_SUCCESS, EG_getMassProperties(face_ego, massprop),
         "EG mass properties");
    cad_cga[0 + 4 * face] = massprop[2];
    cad_cga[1 + 4 * face] = massprop[3];
    cad_cga[2 + 4 * face] = massprop[4];
    cad_cga[3 + 4 * face] = massprop[1];
  }

  RSS(ref_cell_id_range(ref_grid_tri(ref_grid), ref_grid_mpi(ref_grid),
                        &min_faceid, &max_faceid),
      "id range");
  nfaceid = max_faceid - min_faceid + 1;
  ref_malloc(face_box, 6 * nfaceid, double);
  ref_malloc(face_cga, 4 * nfaceid, double);
  for (face = 0; face < nfaceid; face++) {
    faceid = face + min_faceid;
    for (j = 0; j < 3; j++) {
      face_box[j + 0 * 3 + 6 * face] = REF_DBL_MAX;
      face_box[j + 1 * 3 + 6 * face] = REF_DBL_MIN;
    }
    for (j = 0; j < 4; j++) {
      face_cga[j + 4 * face] = 0.0;
    }
    each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
      if (faceid == nodes[ref_cell_node_per(ref_cell)]) {
        for (i = 0; i < ref_cell_node_per(ref_cell); i++) {
          for (j = 0; j < 3; j++) {
            face_box[j + 0 * 3 + 6 * face] =
                MIN(face_box[j + 0 * 3 + 6 * face],
                    ref_node_xyz(ref_node, j, nodes[i]));
            face_box[j + 1 * 3 + 6 * face] =
                MAX(face_box[j + 1 * 3 + 6 * face],
                    ref_node_xyz(ref_node, j, nodes[i]));
          }
        }
        RSS(ref_node_tri_area(ref_node, nodes, &area), "area");
        RSS(ref_node_tri_centroid(ref_node, nodes, centroid), "cent");
        for (j = 0; j < 3; j++) {
          face_cga[j + 4 * face] += centroid[j] * area;
        }
        face_cga[3 + 4 * face] += area;
      }
    }
    for (j = 0; j < 3; j++) {
      face_cga[j + 4 * face] /= face_cga[3 + 4 * face];
    }
  }

  for (face = 0; face < (ref_geom->nface); face++) {
    printf("%4d cmin %10.6f %10.6f %10.6f\n", face + 1, cad_box[0 + 6 * face],
           cad_box[1 + 6 * face], cad_box[2 + 6 * face]);
    printf("%4d cmax %10.6f %10.6f %10.6f\n", face + 1, cad_box[3 + 6 * face],
           cad_box[4 + 6 * face], cad_box[5 + 6 * face]);
    printf("%4d cmpp %10.6f %10.6f %10.6f %10.6f\n", face + 1,
           cad_cga[0 + 4 * face], cad_cga[1 + 4 * face], cad_cga[2 + 4 * face],
           cad_cga[3 + 4 * face]);
  }

  for (face = 0; face < nfaceid; face++) {
    faceid = face + min_faceid;
    printf("%4d fmin %10.6f %10.6f %10.6f\n", faceid, face_box[0 + 6 * face],
           face_box[1 + 6 * face], face_box[2 + 6 * face]);
    printf("%4d fmax %10.6f %10.6f %10.6f\n", faceid, face_box[3 + 6 * face],
           face_box[4 + 6 * face], face_box[5 + 6 * face]);
    printf("%4d fmpp %10.6f %10.6f %10.6f %10.6f\n", face + 1,
           face_cga[0 + 4 * face], face_cga[1 + 4 * face],
           face_cga[2 + 4 * face], face_cga[3 + 4 * face]);
  }

  ref_malloc(face_norm, nfaceid, REF_DBL);
  ref_malloc(candidates, nfaceid, REF_INT);
  RSS(ref_dict_create(&ref_dict), "create dict");
  for (i = 0; i < (ref_geom->nface); i++) {
    /* bbox */
    for (face = 0; face < nfaceid; face++) {
      norm = 0.0;
      for (j = 0; j < 6; j++) {
        norm += pow(face_box[j + 6 * face] - cad_box[j + 6 * i], 2);
      }
      norm = sqrt(norm);
      face_norm[face] = norm;
    }
    RSS(ref_sort_heap_dbl(nfaceid, face_norm, candidates), "sort");
    printf("%4d bbox %4.2f", i + 1,
           face_norm[candidates[0]] / face_norm[candidates[1]]);
    if (0.5 < face_norm[candidates[0]] / face_norm[candidates[1]]) {
      printf(" *");
    } else {
      if (candidates[0] + min_faceid == i + 1) {
        printf(" m");
      } else {
        printf("  ");
      }
    }
    for (j = 0; j < 3; j++) {
      printf(" %4d %10.3e", min_faceid + candidates[j],
             face_norm[candidates[j]]);
    }
    printf("\n");
    /* centroid */
    for (face = 0; face < nfaceid; face++) {
      norm = 0.0;
      for (j = 0; j < 3; j++) {
        norm += pow(face_cga[j + 4 * face] - cad_cga[j + 4 * i], 2);
      }
      norm = sqrt(norm);
      face_norm[face] = norm;
    }
    RSS(ref_sort_heap_dbl(nfaceid, face_norm, candidates), "sort");
    printf("%4d cent %4.2f", i + 1,
           face_norm[candidates[0]] / face_norm[candidates[1]]);
    if (0.5 < face_norm[candidates[0]] / face_norm[candidates[1]]) {
      printf(" *");
    } else {
      if (candidates[0] + min_faceid == i + 1) {
        printf(" m");
      } else {
        printf("  ");
      }
    }
    for (j = 0; j < 3; j++) {
      printf(" %4d %10.3e", min_faceid + candidates[j],
             face_norm[candidates[j]]);
    }
    printf("\n");

    /* last best canidate */
    RSS(ref_dict_store(ref_dict, candidates[0] + min_faceid, i + 1),
        "best guess at new number");
  }

  RSS(ref_dict_inspect(ref_dict), "list map");
  printf("replacing %d faces\n", ref_dict_n(ref_dict));
  each_ref_cell_valid_cell(ref_cell, cell) {
    old_faceid = ref_cell_c2n(ref_cell, ref_cell_node_per(ref_cell), cell);
    RSS(ref_dict_value(ref_dict, old_faceid, &new_faceid),
        "map old to new faceid");
    ref_cell_c2n(ref_cell, ref_cell_node_per(ref_cell), cell) = new_faceid;
  }

  ref_dict_free(ref_dict);
  ref_free(candidates);
  ref_free(face_norm);
  ref_free(face_cga);
  ref_free(face_box);
  ref_free(cad_cga);
  ref_free(cad_box);

#else
  printf("unable to %s, Need full EGADS linked.\n", __func__);
  SUPRESS_UNUSED_COMPILER_WARNING(ref_grid);
#endif
  return REF_SUCCESS;
}

REF_STATUS ref_geom_report_tri_area_normdev(REF_GRID ref_grid) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER], id;
  REF_DBL min_normdev, min_area, max_area, min_uv_area, max_uv_area;
  REF_DBL normdev, area, uv_area, area_sign;
  REF_DBL local;

  min_normdev = 2.0;
  min_area = REF_DBL_MAX;
  max_area = REF_DBL_MIN;
  min_uv_area = REF_DBL_MAX;
  max_uv_area = REF_DBL_MIN;
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
  local = min_normdev;
  RSS(ref_mpi_min(ref_mpi, &local, &min_normdev, REF_DBL_TYPE), "mpi min");
  local = min_area;
  RSS(ref_mpi_min(ref_mpi, &local, &min_area, REF_DBL_TYPE), "mpi min");
  local = min_uv_area;
  RSS(ref_mpi_min(ref_mpi, &local, &min_uv_area, REF_DBL_TYPE), "mpi min");
  local = max_area;
  RSS(ref_mpi_max(ref_mpi, &local, &max_area, REF_DBL_TYPE), "mpi max");
  local = max_uv_area;
  RSS(ref_mpi_max(ref_mpi, &local, &max_uv_area, REF_DBL_TYPE), "mpi max");

  if (ref_mpi_once(ref_mpi)) {
    printf("normdev %f area %.5e  %.5e uv area  %.5e  %.5e\n", min_normdev,
           min_area, max_area, min_uv_area, max_uv_area);
  }

  return REF_SUCCESS;
}
