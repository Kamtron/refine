
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

#include "ref_inflate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ref_cell.h"
#include "ref_export.h"
#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_sort.h"

REF_FCN REF_STATUS ref_inflate_pri_min_dot(REF_NODE ref_node, REF_INT *nodes,
                                           REF_DBL *min_dot) {
  REF_INT tri_nodes[3];
  REF_DBL top_normal[3];
  REF_DBL bot_normal[3];
  REF_DBL edge[3];
  REF_INT node, i;

  tri_nodes[0] = nodes[0];
  tri_nodes[1] = nodes[1];
  tri_nodes[2] = nodes[2];
  RSS(ref_node_tri_normal(ref_node, tri_nodes, bot_normal), "bot");
  RSS(ref_math_normalize(bot_normal), "norm bot");

  tri_nodes[0] = nodes[3];
  tri_nodes[1] = nodes[4];
  tri_nodes[2] = nodes[5];
  RSS(ref_node_tri_normal(ref_node, tri_nodes, top_normal), "top");
  RSB(ref_math_normalize(top_normal), "norm top", {
    printf("%d %d %d %f %f %f\n", tri_nodes[0], tri_nodes[1], tri_nodes[2],
           top_normal[0], top_normal[1], top_normal[2]);
    printf("%f %f %f\n", ref_node_xyz(ref_node, 0, tri_nodes[0]),
           ref_node_xyz(ref_node, 1, tri_nodes[0]),
           ref_node_xyz(ref_node, 2, tri_nodes[0]));
    printf("%f %f %f\n", ref_node_xyz(ref_node, 0, tri_nodes[1]),
           ref_node_xyz(ref_node, 1, tri_nodes[1]),
           ref_node_xyz(ref_node, 2, tri_nodes[1]));
    printf("%f %f %f\n", ref_node_xyz(ref_node, 0, tri_nodes[2]),
           ref_node_xyz(ref_node, 1, tri_nodes[2]),
           ref_node_xyz(ref_node, 2, tri_nodes[2]));
  });

  *min_dot = 1.0;
  for (node = 0; node < 3; node++) {
    for (i = 0; i < 3; i++) {
      edge[i] = ref_node_xyz(ref_node, i, nodes[node + 3]) -
                ref_node_xyz(ref_node, i, nodes[node]);
    }
    RSS(ref_math_normalize(edge), "norm edge0");
    *min_dot = MIN(*min_dot, ref_math_dot(edge, bot_normal));
    *min_dot = MIN(*min_dot, ref_math_dot(edge, top_normal));
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_inflate_face(REF_GRID ref_grid, REF_DICT faceids,
                                    REF_DBL *origin, REF_DBL thickness,
                                    REF_DBL xshift) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL tri = ref_grid_tri(ref_grid);
  REF_CELL qua = ref_grid_qua(ref_grid);
  REF_CELL pri = ref_grid_pri(ref_grid);
  REF_INT cell, tri_side, node0, node1;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT new_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT ntri, tris[2], nquad, quads[2];
  REF_INT tri_node;
  REF_INT *o2n, o2n_max;
  REF_GLOB *o2g;
  REF_GLOB global;
  REF_INT new_node, node;
  REF_INT new_cell;
  REF_DBL min_dot;

  REF_DBL normal[3], dot;
  REF_INT ref_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT item, ref;

  REF_DBL *ymin, *ymax, dy, y0, y1, temp;
  REF_DBL *orient;
  REF_DBL *tmin, *tmax;
  REF_INT *imin, *imax;
  REF_DBL *face_normal;
  REF_INT i;
  REF_DBL theta;

  REF_BOOL problem_detected = REF_FALSE;

  REF_BOOL debug = REF_FALSE;
  REF_BOOL verbose = REF_FALSE;

  ref_malloc_init(face_normal, 3 * ref_dict_n(faceids), REF_DBL, -1.0);

  /* determine each faceids normal, only needed if my part has a tri */

  ref_malloc_init(ymin, ref_dict_n(faceids), REF_DBL, REF_DBL_MAX);
  ref_malloc_init(ymax, ref_dict_n(faceids), REF_DBL, -REF_DBL_MAX);
  ref_malloc_init(orient, ref_dict_n(faceids), REF_DBL, 1);

  y0 = REF_DBL_MAX;
  y1 = -REF_DBL_MAX;
  each_ref_cell_valid_cell_with_nodes(tri, cell, nodes) {
    if (ref_dict_has_key(faceids, nodes[3])) {
      for (tri_node = 0; tri_node < 3; tri_node++) {
        RSS(ref_dict_location(faceids, nodes[3], &i), "key loc");
        node = nodes[tri_node];
        dy = ref_node_xyz(ref_node, 1, node) - origin[1];
        if (ymin[i] > dy) {
          ymin[i] = dy;
        }
        if (ymax[i] < dy) {
          ymax[i] = dy;
        }
        if (y0 > dy) {
          y0 = dy;
        }
        if (y1 < dy) {
          y1 = dy;
        }
      }
    }
  }
  temp = y0;
  RSS(ref_mpi_min(ref_mpi, &temp, &y0, REF_DBL_TYPE), "max thresh");
  RSS(ref_mpi_bcast(ref_mpi, &y0, 1, REF_DBL_TYPE), "bcast");
  temp = y1;
  RSS(ref_mpi_max(ref_mpi, &temp, &y1, REF_DBL_TYPE), "max thresh");
  RSS(ref_mpi_bcast(ref_mpi, &y1, 1, REF_DBL_TYPE), "bcast");

  if (verbose) printf("y %f %f\n", y0, y1);
  each_ref_dict_key_index(faceids, i) {
    temp = ymin[i];
    RSS(ref_mpi_min(ref_mpi, &temp, &(ymin[i]), REF_DBL_TYPE), "max thresh");
    RSS(ref_mpi_bcast(ref_mpi, &(ymin[i]), 1, REF_DBL_TYPE), "bcast");
    temp = ymax[i];
    RSS(ref_mpi_max(ref_mpi, &temp, &(ymax[i]), REF_DBL_TYPE), "max thresh");
    RSS(ref_mpi_bcast(ref_mpi, &(ymax[i]), 1, REF_DBL_TYPE), "bcast");
    if (y1 - ymax[i] > ymin[i] - y0) {
      orient[i] = -1.0;
    } else {
      orient[i] = 1.0;
    }
    if (verbose) printf("%d z %f %f o %f\n", i, ymin[i], ymax[i], orient[i]);
  }

  ref_malloc_init(tmin, ref_dict_n(faceids), REF_DBL, 4.0 * ref_math_pi);
  ref_malloc_init(tmax, ref_dict_n(faceids), REF_DBL, -4.0 * ref_math_pi);
  ref_malloc_init(imin, ref_dict_n(faceids), REF_INT, REF_EMPTY);
  ref_malloc_init(imax, ref_dict_n(faceids), REF_INT, REF_EMPTY);

  each_ref_cell_valid_cell_with_nodes(tri, cell, nodes) {
    if (ref_dict_has_key(faceids, nodes[3]))
      for (tri_node = 0; tri_node < 3; tri_node++) {
        RSS(ref_dict_location(faceids, nodes[3], &i), "key loc");
        node = nodes[tri_node];
        theta =
            atan2((ref_node_xyz(ref_node, 2, node) - origin[2]),
                  orient[i] * (ref_node_xyz(ref_node, 1, node) - origin[1]));

        if (tmin[i] > theta) {
          tmin[i] = theta;
          imin[i] = node;
        }
        if (tmax[i] < theta) {
          tmax[i] = theta;
          imax[i] = node;
        }
      }
  }

  if (debug) ref_dict_inspect(faceids);

  each_ref_dict_key_index(faceids, i) {
    if (!ref_mpi_para(ref_mpi)) {
      RUB(REF_EMPTY, imin[i], "imin",
          { printf("empty %d: %d \n", i, ref_dict_key(faceids, i)); });
      RUB(REF_EMPTY, imax[i], "imax",
          { printf("empty %d: %d \n", i, ref_dict_key(faceids, i)); });
    }
    if (REF_EMPTY != imin[i] && REF_EMPTY != imax[i]) {
      face_normal[0 + 3 * i] = 0.0;
      face_normal[1 + 3 * i] = (ref_node_xyz(ref_node, 2, imax[i]) -
                                ref_node_xyz(ref_node, 2, imin[i]));
      face_normal[2 + 3 * i] = -(ref_node_xyz(ref_node, 1, imax[i]) -
                                 ref_node_xyz(ref_node, 1, imin[i]));
      face_normal[1 + 3 * i] *= -orient[i];
      face_normal[2 + 3 * i] *= -orient[i];
      if (debug)
        printf("n=(%f,%f,%f)\n", face_normal[0 + 3 * i], face_normal[1 + 3 * i],
               face_normal[2 + 3 * i]);
      RSS(ref_math_normalize(&(face_normal[3 * i])), "make face norm");
      if (verbose && ref_mpi_once(ref_mpi))
        printf(
            "f=%5d n=(%7.4f,%7.4f,%7.4f) t=(%7.4f,%7.4f) angle %7.4f"
            " or %4.1f\n",
            ref_dict_key(faceids, i), face_normal[0 + 3 * i],
            face_normal[1 + 3 * i], face_normal[2 + 3 * i], tmin[i], tmax[i],
            ABS(tmin[i] - tmax[i]), orient[i]);
    }
  }

  ref_free(imax);
  ref_free(imin);
  ref_free(tmax);
  ref_free(tmin);

  ref_free(orient);

  ref_free(ymax);
  ref_free(ymin);

  o2n_max = ref_node_max(ref_node);
  ref_malloc_init(o2n, ref_node_max(ref_node), REF_INT, REF_EMPTY);

  /* build list of node globals */
  each_ref_cell_valid_cell_with_nodes(tri, cell, nodes) {
    if (ref_dict_has_key(faceids, nodes[3])) {
      for (tri_node = 0; tri_node < 3; tri_node++) {
        node = nodes[tri_node];
        if (ref_node_owned(ref_node, node) && REF_EMPTY == o2n[node]) {
          RSS(ref_node_next_global(ref_node, &global), "global");
          RSS(ref_node_add(ref_node, global, &new_node), "add node");
          /* redundant */
          ref_node_part(ref_node, new_node) = ref_mpi_rank(ref_mpi);
          o2n[node] = new_node;
        }
      }
    }
  }

  /* sync globals */
  RSS(ref_node_shift_new_globals(ref_node), "shift glob");

  ref_malloc_init(o2g, ref_node_max(ref_node), REF_GLOB, REF_EMPTY);

  /* fill ghost node globals */
  for (node = 0; node < o2n_max; node++) {
    if (REF_EMPTY != o2n[node]) {
      o2g[node] = ref_node_global(ref_node, o2n[node]);
    }
  }
  RSS(ref_node_ghost_glob(ref_node, o2g, 1), "update ghosts");

  ref_free(o2n);
  o2n_max = ref_node_max(ref_node);
  ref_malloc_init(o2n, ref_node_max(ref_node), REF_INT, REF_EMPTY);

  for (node = 0; node < o2n_max; node++) {
    if (REF_EMPTY != o2g[node]) {
      /* returns node if already added */
      RSS(ref_node_add(ref_node, o2g[node], &new_node), "add node");
      ref_node_part(ref_node, new_node) = ref_node_part(ref_node, node);
      o2n[node] = new_node;
    }
  }

  /* create offsets */
  for (node = 0; node < o2n_max; node++) {
    if (ref_node_valid(ref_node, node)) {
      if (ref_node_owned(ref_node, node) && REF_EMPTY != o2n[node]) {
        new_node = o2n[node];
        normal[0] = 0.0;
        normal[1] = ref_node_xyz(ref_node, 1, node) - origin[1];
        normal[2] = ref_node_xyz(ref_node, 2, node) - origin[2];
        RSS(ref_math_normalize(normal), "make norm");
        each_ref_cell_having_node(tri, node, item, ref) {
          RSS(ref_cell_nodes(tri, ref, ref_nodes), "cell");
          if (!ref_dict_has_key(faceids, ref_nodes[3])) continue;
          RSS(ref_dict_location(faceids, ref_nodes[3], &i), "key loc");
          dot = -ref_math_dot(normal, &(face_normal[3 * i]));
          RAS(face_normal[0 + 3 * i] > -0.1, "uninitialized face_normal");
          if (dot < 0.70 || dot > 1.01) {
            printf("out-of-range dot %.15f at %f %f %f\n", dot,
                   ref_node_xyz(ref_node, 0, node),
                   ref_node_xyz(ref_node, 1, node),
                   ref_node_xyz(ref_node, 2, node));
            problem_detected = REF_TRUE;
          }
          RAB(ref_math_divisible(normal[1], dot), "normal[1] /= dot", {
            printf("dot %e\n", dot);
            printf(" xyz %f %f %f\n", ref_node_xyz(ref_node, 0, node),
                   ref_node_xyz(ref_node, 1, node),
                   ref_node_xyz(ref_node, 2, node));
            printf(" norm %f %f %f\n", normal[0], normal[1], normal[2]);
            printf("%d face norm %f %f %f\n", i, face_normal[0 + 3 * i],
                   face_normal[1 + 3 * i], face_normal[2 + 3 * i]);
          });
          RAB(ref_math_divisible(normal[2], dot), "normal[2] /= dot", {
            printf("dot %e\n", dot);
            printf(" xyz %f %f %f\n", ref_node_xyz(ref_node, 0, node),
                   ref_node_xyz(ref_node, 1, node),
                   ref_node_xyz(ref_node, 2, node));
            printf(" norm %f %f %f\n", normal[0], normal[1], normal[2]);
            printf("%d face norm %f %f %f\n", i, face_normal[0 + 3 * i],
                   face_normal[1 + 3 * i], face_normal[2 + 3 * i]);
          });
          normal[1] /= dot;
          normal[2] /= dot;
        }
        ref_node_xyz(ref_node, 0, new_node) =
            xshift + ref_node_xyz(ref_node, 0, node);
        ref_node_xyz(ref_node, 1, new_node) =
            thickness * normal[1] + ref_node_xyz(ref_node, 1, node);
        ref_node_xyz(ref_node, 2, new_node) =
            thickness * normal[2] + ref_node_xyz(ref_node, 2, node);
      }
    }
  }

  if (problem_detected) printf("ERROR: new node normal\n");

  RSS(ref_node_ghost_real(ref_node), "set new ghost node xyz");

  ref_free(face_normal);

  each_ref_cell_valid_cell_with_nodes(tri, cell, nodes) {
    if (ref_dict_has_key(faceids, nodes[3])) {
      for (tri_side = 0; tri_side < 3; tri_side++) {
        node0 = ref_cell_e2n(tri, 0, tri_side, cell);
        node1 = ref_cell_e2n(tri, 1, tri_side, cell);
        if (ref_node_owned(ref_node, node0) ||
            ref_node_owned(ref_node, node1)) {
          RSS(ref_cell_list_with2(tri, node0, node1, 2, &ntri, tris),
              "bad tri count");
          if (1 == ntri) {
            RSS(ref_cell_list_with2(qua, node0, node1, 2, &nquad, quads),
                "bad quad count");
            REIS(1, nquad, "tri without quad");
            new_nodes[4] = ref_cell_c2n(qua, 4, quads[0]);
            new_nodes[0] = node0;
            new_nodes[1] = node1;
            new_nodes[2] = o2n[node1];
            new_nodes[3] = o2n[node0];
            RSS(ref_cell_add(qua, new_nodes, &new_cell), "qua tri1");
            continue;
          }
          if (ref_dict_has_key(faceids, ref_cell_c2n(tri, 3, tris[0])) &&
              !ref_dict_has_key(faceids, ref_cell_c2n(tri, 3, tris[1]))) {
            new_nodes[4] = ref_cell_c2n(tri, 3, tris[1]);
            new_nodes[0] = node0;
            new_nodes[1] = node1;
            new_nodes[2] = o2n[node1];
            new_nodes[3] = o2n[node0];
            RSS(ref_cell_add(qua, new_nodes, &new_cell), "qua tri1");
            continue;
          }
          if (!ref_dict_has_key(faceids, ref_cell_c2n(tri, 3, tris[0])) &&
              ref_dict_has_key(faceids, ref_cell_c2n(tri, 3, tris[1]))) {
            new_nodes[4] = ref_cell_c2n(tri, 3, tris[0]);
            new_nodes[0] = node0;
            new_nodes[1] = node1;
            new_nodes[2] = o2n[node1];
            new_nodes[3] = o2n[node0];
            RSS(ref_cell_add(qua, new_nodes, &new_cell), "qua tri1");
            continue;
          }
        }
      }
    }
  }

  each_ref_cell_valid_cell_with_nodes(tri, cell, nodes) {
    if (ref_dict_has_key(faceids, nodes[3])) {
      new_nodes[0] = nodes[0];
      new_nodes[1] = nodes[2];
      new_nodes[2] = nodes[1];
      new_nodes[3] = o2n[nodes[0]];
      new_nodes[4] = o2n[nodes[2]];
      new_nodes[5] = o2n[nodes[1]];

      RSS(ref_inflate_pri_min_dot(ref_node, new_nodes, &min_dot), "md");
      if (min_dot <= 0.0) {
        /* printf("min_dot %f\n",min_dot); */
        problem_detected = REF_TRUE;
      }

      RSS(ref_cell_add(pri, new_nodes, &new_cell), "pri");
    }
  }

  each_ref_cell_valid_cell_with_nodes(tri, cell, nodes) {
    if (ref_dict_has_key(faceids, nodes[3])) {
      nodes[0] = o2n[nodes[0]];
      nodes[1] = o2n[nodes[1]];
      nodes[2] = o2n[nodes[2]];
      RSS(ref_cell_replace_whole(tri, cell, nodes), "repl");
    }
  }

  ref_free(o2g);
  ref_free(o2n);

  ref_gather_blocking_frame(ref_grid, "layer");

  if (problem_detected) {
    printf("ERROR: inflated grid invalid, writing ref_inflate_problem.tec\n");
    RSS(ref_export_tec_surf(ref_grid, "ref_inflate_problem.tec"), "tec");
    THROW("problem detected, examine ref_inflate_problem.tec");
  }

  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_inflate_sort_rail(REF_INT n, REF_DBL *x,
                                                REF_DBL *yz) {
  REF_INT node;
  REF_INT *order;
  REF_DBL *tmp;
  ref_malloc(order, n, REF_INT);
  ref_malloc(tmp, n, REF_DBL);
  RSS(ref_sort_heap_dbl(n, x, order), "heap");
  for (node = 0; node < n; node++) {
    tmp[order[node]] = x[node];
  }
  for (node = 0; node < n; node++) {
    x[node] = tmp[node];
  }
  for (node = 0; node < n; node++) {
    tmp[order[node]] = yz[0 + 2 * node];
  }
  for (node = 0; node < n; node++) {
    yz[0 + 2 * node] = tmp[node];
  }
  for (node = 0; node < n; node++) {
    tmp[order[node]] = yz[1 + 2 * node];
  }
  for (node = 0; node < n; node++) {
    yz[1 + 2 * node] = tmp[node];
  }

  ref_free(tmp);
  ref_free(order);
  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_inflate_compact_rail(REF_INT *n, REF_DBL *x,
                                                   REF_DBL *yz) {
  REF_INT orig, compact, max;
  REF_DBL tol = 1e-8;
  max = *n;
  compact = 0;
  for (orig = 1; orig < max; orig++) {
    if (ABS(x[orig] - x[compact] > tol)) {
      compact++;
    }
    if (orig != compact) {
      x[compact] = x[orig];
      yz[0 + 2 * compact] = yz[0 + 2 * orig];
      yz[1 + 2 * compact] = yz[1 + 2 * orig];
    }
  }
  *n = compact;
  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_inflate_interpolate_rail(REF_INT n, REF_DBL *x,
                                                       REF_DBL *yz,
                                                       REF_DBL xold,
                                                       REF_DBL *newyz) {
  REF_INT i0, i1;
  REF_DBL t0, t1;
  if (xold < x[0] || x[n - 1] < xold)
    printf("x range %f %f %f\n", x[0], xold, x[n - 1]);
  RSS(ref_sort_search_dbl(n, x, xold, &i0), "rail i0");
  i1 = i0 + 1;
  t1 = 0.0;
  if (ref_math_divisible((xold - x[i0]), (x[i1] - x[i0]))) {
    t1 = (xold - x[i0]) / (x[i1] - x[i0]);
  }
  t1 = MIN(MAX(0.0, t1), 1.0);
  t0 = 1.0 - t1;
  newyz[0] = t0 * yz[0 + 2 * i0] + t1 * yz[0 + 2 * i1];
  newyz[1] = t0 * yz[1 + 2 * i0] + t1 * yz[1 + 2 * i1];

  /*
    printf("%f %f %f\n", xold - (t0 * x[i0] + t1 * x[i1]), xold,
           (t0 * x[i0] + t1 * x[i1]));
  */

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_inflate_radially(REF_GRID ref_grid, REF_DICT faceids,
                                        REF_DBL *origin, REF_DBL thickness,
                                        REF_DBL mach_angle_rad,
                                        REF_DBL alpha_rad, REF_BOOL on_rails) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL tri = ref_grid_tri(ref_grid);
  REF_CELL qua = ref_grid_qua(ref_grid);
  REF_CELL pri = ref_grid_pri(ref_grid);
  REF_INT cell, tri_side, node0, node1, node, o2n_max;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT new_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT ntri, tris[2], nquad, quads[2];
  REF_INT tri_node;
  REF_INT *o2n;
  REF_GLOB *o2g;
  REF_GLOB global;
  REF_INT new_node;
  REF_INT new_cell;
  REF_DBL min_dot;
  REF_DBL phi_rad, alpha_weighting, xshift;

  REF_DBL normal[3];

  REF_BOOL problem_detected = REF_FALSE;

  REF_INT rail_max = 10000;
  REF_INT *rail_n = NULL;
  REF_INT *rail_n0 = NULL;
  REF_INT *rail_n1 = NULL;
  REF_DBL **rail_xyz = NULL;
  REF_DBL *rail_orient = NULL;
  REF_DBL *rail_phi0 = NULL;
  REF_DBL **rail_x0 = NULL;
  REF_DBL **rail_yz0 = NULL;
  REF_DBL *rail_phi1 = NULL;
  REF_DBL **rail_x1 = NULL;
  REF_DBL **rail_yz1 = NULL;

  if (on_rails) {
    REF_INT i, n, *source;
    REF_DBL *concatenated;
    ref_malloc_init(rail_phi0, ref_dict_n(faceids), REF_DBL, -REF_DBL_MAX);
    ref_malloc_init(rail_phi1, ref_dict_n(faceids), REF_DBL, REF_DBL_MAX);
    ref_malloc_init(rail_orient, ref_dict_n(faceids), REF_DBL, 1);
    ref_malloc_init(rail_n, ref_dict_n(faceids), REF_INT, 0);
    ref_malloc_init(rail_n0, ref_dict_n(faceids), REF_INT, 0);
    ref_malloc_init(rail_n1, ref_dict_n(faceids), REF_INT, 0);
    ref_malloc_init(rail_xyz, ref_dict_n(faceids), REF_DBL *, NULL);
    ref_malloc_init(rail_x0, ref_dict_n(faceids), REF_DBL *, NULL);
    ref_malloc_init(rail_yz0, ref_dict_n(faceids), REF_DBL *, NULL);
    ref_malloc_init(rail_x1, ref_dict_n(faceids), REF_DBL *, NULL);
    ref_malloc_init(rail_yz1, ref_dict_n(faceids), REF_DBL *, NULL);
    each_ref_dict_key_index(faceids, i) {
      ref_malloc(rail_xyz[i], rail_max, REF_DBL);
    }
    each_ref_cell_valid_cell_with_nodes(tri, cell, nodes) {
      if (ref_dict_has_key(faceids, nodes[3])) {
        REF_INT dict_index;
        RSS(ref_dict_location(faceids, nodes[3], &dict_index), "loc");
        for (tri_node = 0; tri_node < 3; tri_node++) {
          REF_INT max_id = 4, n_id = 0, ids[4];
          node = nodes[tri_node];
          if (ref_node_owned(ref_node, node)) {
            RSS(ref_cell_id_list_around(tri, node, max_id, &n_id, ids), "ids");
            if (n_id > 1 || !ref_cell_node_empty(qua, node)) {
              if (rail_n[dict_index] >= rail_max) THROW("out of rail_max");

              normal[0] = 0.0;
              normal[1] = ref_node_xyz(ref_node, 1, node) - origin[1];
              normal[2] = ref_node_xyz(ref_node, 2, node) - origin[2];
              RSS(ref_math_normalize(normal), "make norm");
              phi_rad = atan2(normal[1], normal[2]);
              alpha_weighting = cos(phi_rad);
              xshift =
                  thickness / tan(mach_angle_rad + alpha_weighting * alpha_rad);
              rail_xyz[dict_index][0 + 3 * rail_n[dict_index]] =
                  xshift + ref_node_xyz(ref_node, 0, node);
              rail_xyz[dict_index][1 + 3 * rail_n[dict_index]] =
                  thickness * normal[1] + ref_node_xyz(ref_node, 1, node);
              rail_xyz[dict_index][2 + 3 * rail_n[dict_index]] =
                  thickness * normal[2] + ref_node_xyz(ref_node, 2, node);
              rail_n[dict_index]++;
            }
          }
        }
      }
    }

    each_ref_dict_key_index(faceids, i) {
      REF_DBL phi, phi0, phi1;

      RSS(ref_mpi_allconcat(ref_mpi, 3, rail_n[i], rail_xyz[i], &n, &source,
                            (void **)&concatenated, REF_DBL_TYPE),
          "concat");
      ref_free(rail_xyz[i]);
      rail_n[i] = n;
      rail_xyz[i] = concatenated;
      phi0 = REF_DBL_MAX;
      phi1 = -REF_DBL_MAX;
      for (node = 0; node < rail_n[i]; node++) {
        phi = atan2(rail_xyz[i][2 + 3 * node] - origin[2],
                    rail_orient[i] * rail_xyz[i][1 + 3 * node] - origin[1]);
        phi0 = MIN(phi0, phi);
        phi1 = MAX(phi1, phi);
      }
      rail_phi0[i] = phi0;
      rail_phi1[i] = phi1;

      ref_malloc(rail_x0[i], rail_n[i], REF_DBL);
      ref_malloc(rail_x1[i], rail_n[i], REF_DBL);
      ref_malloc(rail_yz0[i], 2 * rail_n[i], REF_DBL);
      ref_malloc(rail_yz1[i], 2 * rail_n[i], REF_DBL);
      for (node = 0; node < rail_n[i]; node++) {
        phi = atan2(rail_xyz[i][2 + 3 * node] - origin[2],
                    rail_orient[i] * rail_xyz[i][1 + 3 * node] - origin[1]);
        if (ABS(phi - rail_phi0[i]) < ABS(phi - rail_phi1[i])) {
          rail_x0[i][rail_n0[i]] = rail_xyz[i][0 + 3 * node];
          rail_yz0[i][0 + 2 * rail_n0[i]] = rail_xyz[i][1 + 3 * node];
          rail_yz0[i][1 + 2 * rail_n0[i]] = rail_xyz[i][2 + 3 * node];
          rail_n0[i]++;
        } else {
          rail_x1[i][rail_n1[i]] = rail_xyz[i][0 + 3 * node];
          rail_yz1[i][0 + 2 * rail_n1[i]] = rail_xyz[i][1 + 3 * node];
          rail_yz1[i][1 + 2 * rail_n1[i]] = rail_xyz[i][2 + 3 * node];
          rail_n1[i]++;
        }
      }
      REIS(rail_n[i], rail_n0[i] + rail_n1[i], "conservation of rail");

      RSS(ref_inflate_sort_rail(rail_n0[i], rail_x0[i], rail_yz0[i]),
          "sort rail 0");
      RSS(ref_inflate_sort_rail(rail_n1[i], rail_x1[i], rail_yz1[i]),
          "sort rail 1");

      RSS(ref_inflate_compact_rail(&(rail_n0[i]), rail_x0[i], rail_yz0[i]),
          "compact rail 0");
      RSS(ref_inflate_compact_rail(&(rail_n1[i]), rail_x1[i], rail_yz1[i]),
          "compact rail 1");

      if (ref_mpi_once(ref_mpi)) {
        printf("id %4d orient %5.2f has %6d phi %5.2f %5.2f of %6d %6d\n",
               ref_dict_key(faceids, i), rail_orient[i], rail_n[i],
               rail_phi0[i], rail_phi1[i], rail_n0[i], rail_n1[i]);
      }
    }
  }

  o2n_max = ref_node_max(ref_node);
  ref_malloc_init(o2n, ref_node_max(ref_node), REF_INT, REF_EMPTY);

  /* build list of node globals */
  each_ref_cell_valid_cell_with_nodes(tri, cell, nodes) {
    if (ref_dict_has_key(faceids, nodes[3])) {
      for (tri_node = 0; tri_node < 3; tri_node++) {
        node = nodes[tri_node];
        if (ref_node_owned(ref_node, node) && REF_EMPTY == o2n[node]) {
          RSS(ref_node_next_global(ref_node, &global), "global");
          RSS(ref_node_add(ref_node, global, &new_node), "add node");
          /* redundant */
          ref_node_part(ref_node, new_node) = ref_mpi_rank(ref_mpi);
          o2n[node] = new_node;
        }
      }
    }
  }

  /* sync globals */
  RSS(ref_node_shift_new_globals(ref_node), "shift glob");

  ref_malloc_init(o2g, ref_node_max(ref_node), REF_GLOB, REF_EMPTY);

  /* fill ghost node globals */
  for (node = 0; node < o2n_max; node++) {
    if (REF_EMPTY != o2n[node]) {
      o2g[node] = ref_node_global(ref_node, o2n[node]);
    }
  }
  RSS(ref_node_ghost_glob(ref_node, o2g, 1), "update ghosts");

  ref_free(o2n);
  o2n_max = ref_node_max(ref_node);
  ref_malloc_init(o2n, ref_node_max(ref_node), REF_INT, REF_EMPTY);

  for (node = 0; node < o2n_max; node++) {
    if (REF_EMPTY != o2g[node]) {
      /* returns node if already added */
      RSS(ref_node_add(ref_node, o2g[node], &new_node), "add node");
      ref_node_part(ref_node, new_node) = ref_node_part(ref_node, node);
      o2n[node] = new_node;
    }
  }

  /* create offsets */
  for (node = 0; node < o2n_max; node++) {
    if (ref_node_valid(ref_node, node)) {
      if (ref_node_owned(ref_node, node) && REF_EMPTY != o2n[node]) {
        new_node = o2n[node];

        normal[0] = 0.0;
        normal[1] = ref_node_xyz(ref_node, 1, node) - origin[1];
        normal[2] = ref_node_xyz(ref_node, 2, node) - origin[2];
        RSS(ref_math_normalize(normal), "make norm");
        phi_rad = atan2(normal[1], normal[2]);
        alpha_weighting = cos(phi_rad);
        xshift = thickness / tan(mach_angle_rad + alpha_weighting * alpha_rad);
        ref_node_xyz(ref_node, 0, new_node) =
            xshift + ref_node_xyz(ref_node, 0, node);
        ref_node_xyz(ref_node, 1, new_node) =
            thickness * normal[1] + ref_node_xyz(ref_node, 1, node);
        ref_node_xyz(ref_node, 2, new_node) =
            thickness * normal[2] + ref_node_xyz(ref_node, 2, node);
        if (on_rails) {
          REF_INT max_id = 4, n_id = 0, ids[4];
          RSS(ref_cell_id_list_around(tri, node, max_id, &n_id, ids), "ids");
          if (1 == n_id && ref_cell_node_empty(qua, node)) {
            REF_INT ind;
            REF_DBL yz0[2], yz1[2];
            REF_DBL t0, t1, phi;
            RSS(ref_dict_location(faceids, ids[0], &ind), "faceid loc");
            RSS(ref_inflate_interpolate_rail(
                    rail_n0[ind], rail_x0[ind], rail_yz0[ind],
                    ref_node_xyz(ref_node, 0, new_node), yz0),
                "interp 0");
            RSS(ref_inflate_interpolate_rail(
                    rail_n1[ind], rail_x1[ind], rail_yz1[ind],
                    ref_node_xyz(ref_node, 0, new_node), yz1),
                "interp 1");
            phi = atan2(ref_node_xyz(ref_node, 2, new_node) - origin[2],
                        rail_orient[ind] * ref_node_xyz(ref_node, 1, new_node) -
                            origin[1]);
            t1 = 0.0;
            if (ref_math_divisible((phi - rail_phi0[ind]),
                                   (rail_phi1[ind] - rail_phi0[ind]))) {
              t1 = (phi - rail_phi0[ind]) / (rail_phi1[ind] - rail_phi0[ind]);
            }
            t1 = MIN(MAX(0.0, t1), 1.0);
            t0 = 1.0 - t1;
            /*
            printf("ind %d %d phi %f between %f %f weight %f %f \n",
                   ref_dict_key(faceids, ind), ids[0], phi, rail_phi0[ind],
                   rail_phi1[ind], t0, t1);
            */
            /*
            printf(" old %f %f", ref_node_xyz(ref_node, 1, new_node),
                   ref_node_xyz(ref_node, 2, new_node));
            printf(" rail 0 %f %f", yz0[0], yz0[1]);
            printf(" rail 1 %f %f", yz1[0], yz1[1]);
            printf("\n");
            */
            ref_node_xyz(ref_node, 1, new_node) = t0 * yz0[0] + t1 * yz1[0];
            ref_node_xyz(ref_node, 2, new_node) = t0 * yz0[1] + t1 * yz1[1];
          }
        }
      }
    }
  }

  RSS(ref_node_ghost_real(ref_node), "set new ghost node xyz");

  each_ref_cell_valid_cell_with_nodes(tri, cell, nodes) {
    if (ref_dict_has_key(faceids, nodes[3])) {
      for (tri_side = 0; tri_side < 3; tri_side++) {
        node0 = ref_cell_e2n(tri, 0, tri_side, cell);
        node1 = ref_cell_e2n(tri, 1, tri_side, cell);
        if (ref_node_owned(ref_node, node0) ||
            ref_node_owned(ref_node, node1)) {
          RSS(ref_cell_list_with2(tri, node0, node1, 2, &ntri, tris),
              "bad tri count");
          if (1 == ntri) {
            RSS(ref_cell_list_with2(qua, node0, node1, 2, &nquad, quads),
                "bad quad count");
            if (1 != nquad) THROW("tri without quad");
            new_nodes[4] = ref_cell_c2n(qua, 4, quads[0]);
            new_nodes[0] = node0;
            new_nodes[1] = node1;
            new_nodes[2] = o2n[node1];
            new_nodes[3] = o2n[node0];
            RSS(ref_cell_add(qua, new_nodes, &new_cell), "qua tri1");
            continue;
          }
          if (ref_dict_has_key(faceids, ref_cell_c2n(tri, 3, tris[0])) &&
              !ref_dict_has_key(faceids, ref_cell_c2n(tri, 3, tris[1]))) {
            new_nodes[4] = ref_cell_c2n(tri, 3, tris[1]);
            new_nodes[0] = node0;
            new_nodes[1] = node1;
            new_nodes[2] = o2n[node1];
            new_nodes[3] = o2n[node0];
            RSS(ref_cell_add(qua, new_nodes, &new_cell), "qua tri1");
            continue;
          }
          if (!ref_dict_has_key(faceids, ref_cell_c2n(tri, 3, tris[0])) &&
              ref_dict_has_key(faceids, ref_cell_c2n(tri, 3, tris[1]))) {
            new_nodes[4] = ref_cell_c2n(tri, 3, tris[0]);
            new_nodes[0] = node0;
            new_nodes[1] = node1;
            new_nodes[2] = o2n[node1];
            new_nodes[3] = o2n[node0];
            RSS(ref_cell_add(qua, new_nodes, &new_cell), "qua tri1");
            continue;
          }
        }
      }
    }
  }

  each_ref_cell_valid_cell_with_nodes(tri, cell, nodes) {
    if (ref_dict_has_key(faceids, nodes[3])) {
      new_nodes[0] = nodes[0];
      new_nodes[1] = nodes[2];
      new_nodes[2] = nodes[1];
      new_nodes[3] = o2n[nodes[0]];
      new_nodes[4] = o2n[nodes[2]];
      new_nodes[5] = o2n[nodes[1]];

      RSS(ref_inflate_pri_min_dot(ref_node, new_nodes, &min_dot), "md");
      if (min_dot <= 0.0) {
        printf("min_dot %f\n", min_dot);
        problem_detected = REF_TRUE;
      }

      RSS(ref_cell_add(pri, new_nodes, &new_cell), "pri");
    }
  }

  each_ref_cell_valid_cell_with_nodes(tri, cell, nodes) {
    if (ref_dict_has_key(faceids, nodes[3])) {
      nodes[0] = o2n[nodes[0]];
      nodes[1] = o2n[nodes[1]];
      nodes[2] = o2n[nodes[2]];
      RSS(ref_cell_replace_whole(tri, cell, nodes), "repl");
    }
  }

  ref_free(o2g);
  ref_free(o2n);

  ref_gather_blocking_frame(ref_grid, "layer");

  if (on_rails) {
    REF_INT i;
    each_ref_dict_key_index(faceids, i) { ref_free(rail_yz1[i]); }
    each_ref_dict_key_index(faceids, i) { ref_free(rail_x1[i]); }
    each_ref_dict_key_index(faceids, i) { ref_free(rail_yz0[i]); }
    each_ref_dict_key_index(faceids, i) { ref_free(rail_x0[i]); }
    each_ref_dict_key_index(faceids, i) { ref_free(rail_xyz[i]); }
    ref_free(rail_yz1);
    ref_free(rail_x1);
    ref_free(rail_yz0);
    ref_free(rail_x0);
    ref_free(rail_xyz);
    ref_free(rail_n1);
    ref_free(rail_n0);
    ref_free(rail_n);
    ref_free(rail_orient);
    ref_free(rail_phi1);
    ref_free(rail_phi0);
  }

  if (problem_detected) {
    printf("ERROR: inflated grid invalid, writing ref_inflate_problem.tec\n");
    RSS(ref_export_tec_surf(ref_grid, "ref_inflate_problem.tec"), "tec");
    THROW("problem detected, examine ref_inflate_problem.tec");
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_inflate_rate(REF_INT nlayers, REF_DBL first_thickness,
                                    REF_DBL total_thickness, REF_DBL *rate) {
  REF_DBL r, H, err, dHdr;
  REF_BOOL keep_going;
  REF_INT iters;
  r = 1.1;

  iters = 0;
  keep_going = REF_TRUE;
  while (keep_going) {
    iters++;
    if (iters > 100) THROW("iteration count exceeded");

    RSS(ref_inflate_total_thickness(nlayers, first_thickness, r, &H), "total");
    err = H - total_thickness;

    RSS(ref_inflate_dthickness(nlayers, first_thickness, r, &dHdr), "total");

    /* printf(" r %e H %e err %e dHdr %e\n",r,H,err,dHdr); */

    r = r - err / dHdr;

    keep_going = (ABS(err / total_thickness) > 1.0e-12);
  }

  *rate = r;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_inflate_total_thickness(REF_INT nlayers,
                                               REF_DBL first_thickness,
                                               REF_DBL rate,
                                               REF_DBL *total_thickness) {
  *total_thickness =
      first_thickness * (1.0 - pow(rate, nlayers)) / (1.0 - rate);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_inflate_dthickness(REF_INT nlayers,
                                          REF_DBL first_thickness, REF_DBL rate,
                                          REF_DBL *dHdr) {
  REF_DBL dnum, dden;
  dnum = -((REF_DBL)nlayers) * pow(rate, nlayers - 1);
  dden = -1.0;

  *dHdr = first_thickness *
          (dnum * (1.0 - rate) - (1.0 - pow(rate, nlayers)) * dden) /
          (1.0 - rate) / (1.0 - rate);
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_inflate_origin(REF_GRID ref_grid, REF_DICT faceids,
                                      REF_DBL *origin) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL tri = ref_grid_tri(ref_grid);
  REF_INT cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL z0, z1, z;
  REF_INT node;
  REF_BOOL first_time;

  first_time = REF_TRUE;
  z0 = 0;
  z1 = 0;
  each_ref_cell_valid_cell_with_nodes(
      tri, cell, nodes) if (ref_dict_has_key(faceids, nodes[3])) {
    for (node = 0; node < 3; node++) {
      if (!first_time) {
        z0 = MIN(z0, ref_node_xyz(ref_node, 2, nodes[node]));
        z1 = MAX(z1, ref_node_xyz(ref_node, 2, nodes[node]));
      } else {
        z0 = ref_node_xyz(ref_node, 2, nodes[node]);
        z1 = ref_node_xyz(ref_node, 2, nodes[node]);
        first_time = REF_FALSE;
      }
    }
  }

  z = z0;
  RSS(ref_mpi_min(ref_mpi, &z, &z0, REF_DBL_TYPE), "min");
  RSS(ref_mpi_bcast(ref_mpi, &z0, 1, REF_DBL_TYPE), "bmin");
  z = z1;
  RSS(ref_mpi_max(ref_mpi, &z, &z1, REF_DBL_TYPE), "max");
  RSS(ref_mpi_bcast(ref_mpi, &z1, 1, REF_DBL_TYPE), "max");

  origin[0] = 0;
  origin[1] = 0;
  origin[2] = 0.5 * (z0 + z1);

  if (ref_mpi_once(ref_mpi))
    printf("the z range is %f %f and origin %f\n", z0, z1, origin[2]);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_inflate_read_usm3d_mapbc(REF_DICT faceids,
                                                const char *mapbc_file_name,
                                                const char *family_name,
                                                REF_INT boundary_condition) {
  FILE *file;
  char line[1024];
  char family[1024];
  REF_INT faceid, bc, bc_family, i0, i1;
  size_t len;

  len = strlen(family_name);

  file = fopen(mapbc_file_name, "r");
  if (NULL == (void *)file) printf("unable to open %s\n", mapbc_file_name);
  RNS(file, "unable to open file");

  while (fgets(line, sizeof(line), file) != NULL) {
    if (6 == sscanf(line, "%d %d %d %d %d %s", &faceid, &bc, &bc_family, &i0,
                    &i1, family)) {
      if (0 == strncmp(family_name, family, len) && bc == boundary_condition) {
        RSS(ref_dict_store(faceids, faceid, REF_EMPTY), "store");
      }
    }
  }

  fclose(file);
  return REF_SUCCESS;
}
