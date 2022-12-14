
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

#include "ref_face.h"

#include <stdio.h>
#include <stdlib.h>

#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_sort.h"

REF_FCN REF_STATUS ref_face_create(REF_FACE *ref_face_ptr, REF_GRID ref_grid) {
  REF_FACE ref_face;
  REF_INT group, cell, cell_face;
  REF_INT node;
  REF_INT nodes[4];
  REF_CELL ref_cell;

  ref_malloc(*ref_face_ptr, 1, REF_FACE_STRUCT);

  ref_face = *ref_face_ptr;

  ref_face_n(ref_face) = 0;
  ref_face_max(ref_face) = 10;

  ref_malloc(ref_face->f2n, 4 * ref_face_max(ref_face), REF_INT);

  RSS(ref_adj_create(&(ref_face_adj(ref_face))), "create adj");

  if (ref_grid_twod(ref_grid)) {
    each_ref_grid_2d_ref_cell(ref_grid, group, ref_cell) {
      each_ref_cell_valid_cell(ref_cell, cell) {
        each_ref_cell_cell_face(ref_cell, cell_face) {
          for (node = 0; node < 4; node++) {
            nodes[node] = ref_cell_f2n(ref_cell, node, cell_face, cell);
          }
          RSS(ref_face_add_uniquely(ref_face, nodes), "add face");
        }
      }
    }
  } else {
    each_ref_grid_3d_ref_cell(ref_grid, group, ref_cell) {
      each_ref_cell_valid_cell(ref_cell, cell) {
        each_ref_cell_cell_face(ref_cell, cell_face) {
          for (node = 0; node < 4; node++) {
            nodes[node] = ref_cell_f2n(ref_cell, node, cell_face, cell);
          }
          RSS(ref_face_add_uniquely(ref_face, nodes), "add face");
        }
      }
    }
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_face_free(REF_FACE ref_face) {
  if (NULL == (void *)ref_face) return REF_NULL;

  RSS(ref_adj_free(ref_face_adj(ref_face)), "free adj");
  ref_free(ref_face->f2n);

  ref_free(ref_face);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_face_inspect(REF_FACE ref_face) {
  REF_INT face, node, orig[4], sort[4];

  printf("ref_face = %p\n", (void *)ref_face);
  printf(" n = %d\n", ref_face_n(ref_face));
  printf(" max = %d\n", ref_face_max(ref_face));
  for (face = 0; face < ref_face_n(ref_face); face++)
    printf("face %4d : %4d %4d %4d %4d\n", face,
           ref_face_f2n(ref_face, 0, face), ref_face_f2n(ref_face, 1, face),
           ref_face_f2n(ref_face, 2, face), ref_face_f2n(ref_face, 3, face));
  for (face = 0; face < ref_face_n(ref_face); face++) {
    for (node = 0; node < 4; node++)
      orig[node] = ref_face_f2n(ref_face, node, face);
    RSS(ref_sort_insertion_int(4, orig, sort), "sort");
    printf("sort %4d : %4d %4d %4d %4d\n", face, sort[0], sort[1], sort[2],
           sort[3]);
  }
  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_face_make_canonical(REF_INT *original,
                                                  REF_INT *canonical) {
  RSS(ref_sort_insertion_int(4, original, canonical), "sort");

  if (canonical[0] == canonical[1]) {
    canonical[0] = REF_EMPTY;
    return REF_SUCCESS;
  }

  if (canonical[1] == canonical[2]) {
    canonical[1] = canonical[0];
    canonical[0] = REF_EMPTY;
    return REF_SUCCESS;
  }

  if (canonical[2] == canonical[3]) {
    canonical[2] = canonical[1];
    canonical[1] = canonical[0];
    canonical[0] = REF_EMPTY;
    return REF_SUCCESS;
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_face_with(REF_FACE ref_face, REF_INT *nodes,
                                 REF_INT *face) {
  REF_INT item, ref, node;
  REF_INT target[4], candidate[4], orig[4];

  (*face) = REF_EMPTY;

  RSS(ref_face_make_canonical(nodes, target), "canonical");

  each_ref_adj_node_item_with_ref(ref_face_adj(ref_face), nodes[0], item, ref) {
    for (node = 0; node < 4; node++)
      orig[node] = ref_face_f2n(ref_face, node, ref);
    RSS(ref_face_make_canonical(orig, candidate), "canonical");
    if (target[0] == candidate[0] && target[1] == candidate[1] &&
        target[2] == candidate[2] && target[3] == candidate[3]) {
      (*face) = ref;
      return REF_SUCCESS;
    }
  }

  return REF_NOT_FOUND;
}

REF_FCN REF_STATUS ref_face_spanning(REF_FACE ref_face, REF_INT node0,
                                     REF_INT node1, REF_INT *face) {
  REF_INT item, ref;

  (*face) = REF_EMPTY;

  each_ref_adj_node_item_with_ref(ref_face_adj(ref_face), node0, item, ref) {
    if ((node0 == ref_face_f2n(ref_face, 0, ref) ||
         node0 == ref_face_f2n(ref_face, 1, ref) ||
         node0 == ref_face_f2n(ref_face, 2, ref) ||
         node0 == ref_face_f2n(ref_face, 3, ref)) &&
        (node1 == ref_face_f2n(ref_face, 0, ref) ||
         node1 == ref_face_f2n(ref_face, 1, ref) ||
         node1 == ref_face_f2n(ref_face, 2, ref) ||
         node1 == ref_face_f2n(ref_face, 3, ref))) {
      (*face) = ref;
      return REF_SUCCESS;
    }
  }

  return REF_NOT_FOUND;
}

REF_FCN REF_STATUS ref_face_add_uniquely(REF_FACE ref_face, REF_INT *nodes) {
  REF_INT face, node;
  REF_STATUS status;

  status = ref_face_with(ref_face, nodes, &face);
  if (REF_SUCCESS == status) return REF_SUCCESS;
  if (REF_NOT_FOUND != status) RSS(status, "looking for face");

  if (ref_face_n(ref_face) >= ref_face_max(ref_face)) {
    REF_INT chunk;
    chunk = MAX(5000, (REF_INT)(1.5 * (REF_DBL)ref_face_max(ref_face)));
    ref_face_max(ref_face) += chunk;
    ref_realloc(ref_face->f2n, 4 * ref_face_max(ref_face), REF_INT);
  }

  face = ref_face_n(ref_face);
  ref_face_n(ref_face)++;

  for (node = 0; node < 4; node++)
    ref_face_f2n(ref_face, node, face) = nodes[node];

  for (node = 0; node < 3; node++)
    RSS(ref_adj_add(ref_face_adj(ref_face), ref_face_f2n(ref_face, node, face),
                    face),
        "reg face node");

  if (ref_face_f2n(ref_face, 0, face) != ref_face_f2n(ref_face, 3, face))
    RSS(ref_adj_add(ref_face_adj(ref_face), ref_face_f2n(ref_face, 3, face),
                    face),
        "reg face node");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_face_normal(REF_DBL *xyz0, REF_DBL *xyz1, REF_DBL *xyz2,
                                   REF_DBL *xyz3, REF_DBL *normal) {
  REF_DBL edge1[3], edge2[3];

  normal[0] = 0.0;
  normal[1] = 0.0;
  normal[2] = 0.0;

  edge1[0] = xyz2[0] - xyz1[0];
  edge1[1] = xyz2[1] - xyz1[1];
  edge1[2] = xyz2[2] - xyz1[2];

  edge2[0] = xyz0[0] - xyz1[0];
  edge2[1] = xyz0[1] - xyz1[1];
  edge2[2] = xyz0[2] - xyz1[2];

  normal[0] += 0.5 * (edge1[1] * edge2[2] - edge1[2] * edge2[1]);
  normal[1] += 0.5 * (edge1[2] * edge2[0] - edge1[0] * edge2[2]);
  normal[2] += 0.5 * (edge1[0] * edge2[1] - edge1[1] * edge2[0]);

  edge1[0] = xyz0[0] - xyz3[0];
  edge1[1] = xyz0[1] - xyz3[1];
  edge1[2] = xyz0[2] - xyz3[2];

  edge2[0] = xyz2[0] - xyz3[0];
  edge2[1] = xyz2[1] - xyz3[1];
  edge2[2] = xyz2[2] - xyz3[2];

  normal[0] += 0.5 * (edge1[1] * edge2[2] - edge1[2] * edge2[1]);
  normal[1] += 0.5 * (edge1[2] * edge2[0] - edge1[0] * edge2[2]);
  normal[2] += 0.5 * (edge1[0] * edge2[1] - edge1[1] * edge2[0]);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_face_open_node(REF_DBL *xyz0, REF_DBL *xyz1,
                                      REF_DBL *xyz2, REF_DBL *xyz3,
                                      REF_INT *open_node, REF_DBL *dot) {
  REF_DBL edge1[3], edge2[3];
  REF_DBL open_dot;

  edge1[0] = xyz0[0] - xyz3[0];
  edge1[1] = xyz0[1] - xyz3[1];
  edge1[2] = xyz0[2] - xyz3[2];

  edge2[0] = xyz1[0] - xyz0[0];
  edge2[1] = xyz1[1] - xyz0[1];
  edge2[2] = xyz1[2] - xyz0[2];

  RSS(ref_math_normalize(edge1), "e1");
  RSS(ref_math_normalize(edge2), "e2");
  open_dot = ref_math_dot(edge2, edge1);
  *open_node = 0;
  *dot = open_dot;

  edge1[0] = xyz1[0] - xyz0[0];
  edge1[1] = xyz1[1] - xyz0[1];
  edge1[2] = xyz1[2] - xyz0[2];

  edge2[0] = xyz2[0] - xyz1[0];
  edge2[1] = xyz2[1] - xyz1[1];
  edge2[2] = xyz2[2] - xyz1[2];

  RSS(ref_math_normalize(edge1), "e1");
  RSS(ref_math_normalize(edge2), "e2");
  if (ref_math_dot(edge2, edge1) > open_dot) {
    open_dot = ref_math_dot(edge2, edge1);
    *open_node = 1;
    *dot = open_dot;
  }

  edge1[0] = xyz2[0] - xyz1[0];
  edge1[1] = xyz2[1] - xyz1[1];
  edge1[2] = xyz2[2] - xyz1[2];

  edge2[0] = xyz3[0] - xyz2[0];
  edge2[1] = xyz3[1] - xyz2[1];
  edge2[2] = xyz3[2] - xyz2[2];

  RSS(ref_math_normalize(edge1), "e1");
  RSS(ref_math_normalize(edge2), "e2");
  if (ref_math_dot(edge2, edge1) > open_dot) {
    open_dot = ref_math_dot(edge2, edge1);
    *open_node = 2;
    *dot = open_dot;
  }

  edge1[0] = xyz3[0] - xyz2[0];
  edge1[1] = xyz3[1] - xyz2[1];
  edge1[2] = xyz3[2] - xyz2[2];

  edge2[0] = xyz0[0] - xyz3[0];
  edge2[1] = xyz0[1] - xyz3[1];
  edge2[2] = xyz0[2] - xyz3[2];

  RSS(ref_math_normalize(edge1), "e1");
  RSS(ref_math_normalize(edge2), "e2");
  if (ref_math_dot(edge2, edge1) > open_dot) {
    open_dot = ref_math_dot(edge2, edge1);
    *open_node = 3;
    *dot = open_dot;
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_face_part(REF_FACE ref_face, REF_NODE ref_node,
                                 REF_INT face, REF_INT *part) {
  REF_GLOB global, smallest_global;
  REF_INT node, smallest_global_node;

  /* set first node as smallest */
  node = 0;
  global = ref_node_global(ref_node, ref_face_f2n(ref_face, node, face));
  smallest_global = global;
  smallest_global_node = node;
  /* search other nodes for smaller global */
  for (node = 1; node < 4; node++) {
    global = ref_node_global(ref_node, ref_face_f2n(ref_face, node, face));
    if (global < smallest_global) {
      smallest_global = global;
      smallest_global_node = node;
    }
  }

  *part = ref_node_part(ref_node,
                        ref_face_f2n(ref_face, smallest_global_node, face));

  return REF_SUCCESS;
}
