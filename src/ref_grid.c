
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

#include "ref_grid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ref_edge.h"
#include "ref_malloc.h"

REF_FCN REF_STATUS ref_grid_create(REF_GRID *ref_grid_ptr, REF_MPI ref_mpi) {
  REF_INT group;
  REF_GRID ref_grid;

  ref_malloc(*ref_grid_ptr, 1, REF_GRID_STRUCT);

  ref_grid = *ref_grid_ptr;

  RSS(ref_mpi_deep_copy(&ref_grid_mpi(ref_grid), ref_mpi), "deep copy ref_mpi");

  RSS(ref_node_create(&ref_grid_node(ref_grid), ref_grid_mpi(ref_grid)),
      "node create");

  for (group = 0; group < REF_CELL_N_TYPE; group++) {
    RSS(ref_cell_create(&ref_grid_cell(ref_grid, group), (REF_CELL_TYPE)group),
        "cell create");
  }

  ref_grid_cell(ref_grid, REF_CELL_N_TYPE) = NULL;

  RSS(ref_geom_create(&ref_grid_geom(ref_grid)), "geom create");
  RSS(ref_gather_create(&ref_grid_gather(ref_grid)), "gather create");
  RSS(ref_adapt_create(&(ref_grid->adapt)), "adapt create");
  ref_grid_interp(ref_grid) = NULL;

  ref_grid_partitioner(ref_grid) = REF_MIGRATE_RECOMMENDED;
  ref_grid_partitioner_seed(ref_grid) = 0;
  ref_grid_partitioner_full(ref_grid) = REF_FALSE;

  ref_grid_meshb_version(ref_grid) = 0;
  ref_grid_coordinate_system(ref_grid) = REF_GRID_XBYRZU;
  ref_grid_unit(ref_grid) = REF_GRID_M;
  ref_grid_reference(ref_grid, 0) = 1.0;
  ref_grid_reference(ref_grid, 1) = 1.0;
  ref_grid_reference(ref_grid, 2) = 1.0;
  ref_grid_reference(ref_grid, 3) = 1.0;
  ref_grid_reference(ref_grid, 4) = 0.0;
  ref_grid_reference(ref_grid, 5) = 0.0;
  ref_grid_reference(ref_grid, 6) = 0.0;

  ref_grid_twod(ref_grid) = REF_FALSE;
  ref_grid_surf(ref_grid) = REF_FALSE;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_deep_copy(REF_GRID *ref_grid_ptr,
                                      REF_GRID original) {
  REF_INT group;
  REF_GRID ref_grid;

  ref_malloc(*ref_grid_ptr, 1, REF_GRID_STRUCT);

  ref_grid = *ref_grid_ptr;

  RSS(ref_mpi_deep_copy(&ref_grid_mpi(ref_grid), ref_grid_mpi(original)),
      "deep copy ref_mpi");

  RSS(ref_node_deep_copy(&ref_grid_node(ref_grid), ref_grid_mpi(ref_grid),
                         ref_grid_node(original)),
      "node deep copy");

  for (group = 0; group < REF_CELL_N_TYPE; group++) {
    RSS(ref_cell_deep_copy(&ref_grid_cell(ref_grid, group),
                           ref_grid_cell(original, group)),
        "cell deep copy");
  }

  ref_grid_cell(ref_grid, REF_CELL_N_TYPE) = NULL;

  RSS(ref_geom_deep_copy(&ref_grid_geom(ref_grid), ref_grid_geom(original)),
      "geom deep copy");
  RSS(ref_gather_create(&ref_grid_gather(ref_grid)), "gather create");
  RSS(ref_adapt_deep_copy(&(ref_grid->adapt), original->adapt),
      "adapt deep copy");

  ref_grid_interp(ref_grid) = NULL;

  ref_grid_partitioner(ref_grid) = ref_grid_partitioner(original);
  ref_grid_partitioner_seed(ref_grid) = 0;
  ref_grid_partitioner_full(ref_grid) = ref_grid_partitioner_full(original);

  ref_grid_meshb_version(ref_grid) = 0;
  ref_grid_coordinate_system(ref_grid) = ref_grid_coordinate_system(original);
  ref_grid_unit(ref_grid) = ref_grid_unit(original);
  ref_grid_reference(ref_grid, 0) = ref_grid_reference(original, 0);
  ref_grid_reference(ref_grid, 1) = ref_grid_reference(original, 1);
  ref_grid_reference(ref_grid, 2) = ref_grid_reference(original, 2);
  ref_grid_reference(ref_grid, 3) = ref_grid_reference(original, 3);
  ref_grid_reference(ref_grid, 4) = ref_grid_reference(original, 4);
  ref_grid_reference(ref_grid, 5) = ref_grid_reference(original, 5);
  ref_grid_reference(ref_grid, 6) = ref_grid_reference(original, 6);

  ref_grid_twod(ref_grid) = ref_grid_twod(original);
  ref_grid_surf(ref_grid) = ref_grid_surf(original);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_cache_background(REF_GRID ref_grid) {
  RAS(NULL == ref_grid_interp(ref_grid), "expected NULL interp, called twice?");
  RSS(ref_interp_create_identity(&ref_grid_interp(ref_grid), ref_grid),
      "make interp");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_stable_pack(REF_GRID ref_grid) {
  REF_INT group;
  REF_INT *o2n, *n2o;
  REF_CELL ref_cell;

  RSS(ref_node_synchronize_globals(ref_grid_node(ref_grid)), "sync globals");

  RSS(ref_node_stable_compact(ref_grid_node(ref_grid), &o2n, &n2o),
      "pack node");
  RSS(ref_node_pack(ref_grid_node(ref_grid), o2n, n2o), "pack node");
  each_ref_grid_all_ref_cell(ref_grid, group, ref_cell) {
    RSS(ref_cell_pack(ref_cell, o2n), "pack cell");
  }

  RSS(ref_geom_pack(ref_grid_geom(ref_grid), o2n), "pack geom");
  RSS(ref_interp_pack(ref_grid_interp(ref_grid), n2o), "pack interp");

  ref_free(n2o);
  ref_free(o2n);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_pack(REF_GRID ref_grid) {
  REF_INT group;
  REF_INT *o2n, *n2o;
  REF_CELL ref_cell;
  REF_EDGE ref_edge;
  REF_BOOL timing = REF_FALSE;

  RSS(ref_node_synchronize_globals(ref_grid_node(ref_grid)), "sync globals");

  RSS(ref_edge_create(&ref_edge, ref_grid), "create edge");
  RSS(ref_edge_rcm(ref_edge, &o2n, &n2o), "compact");
  RSS(ref_edge_free(ref_edge), "free edge");
  if (timing) ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), " pack rcm");

  RSS(ref_node_pack(ref_grid_node(ref_grid), o2n, n2o), "pack node");
  if (timing) ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), " pack node");
  each_ref_grid_all_ref_cell(ref_grid, group, ref_cell) {
    RSS(ref_cell_pack(ref_cell, o2n), "pack cell");
  }
  if (timing) ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), " pack cell");

  RSS(ref_geom_pack(ref_grid_geom(ref_grid), o2n), "pack geom");
  if (timing) ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), " pack geom");
  RSS(ref_interp_pack(ref_grid_interp(ref_grid), n2o), "pack interp");
  if (timing) ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), " pack interp");

  ref_free(n2o);
  ref_free(o2n);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_free(REF_GRID ref_grid) {
  REF_INT group;
  REF_CELL ref_cell;

  if (NULL == (void *)ref_grid) return REF_NULL;

  if (NULL != (void *)ref_grid_interp(ref_grid)) {
    RSS(ref_grid_free(ref_interp_from_grid(ref_grid_interp(ref_grid))),
        "free cached background grid");
    RSS(ref_interp_free(ref_grid->interp), "interp free");
  }

  RSS(ref_adapt_free(ref_grid->adapt), "adapt free");
  RSS(ref_gather_free(ref_grid_gather(ref_grid)), "gather free");
  RSS(ref_geom_free(ref_grid_geom(ref_grid)), "geom free");

  each_ref_grid_all_ref_cell(ref_grid, group, ref_cell) {
    RSS(ref_cell_free(ref_cell), "cell free");
  }

  RSS(ref_node_free(ref_grid_node(ref_grid)), "node free");
  RSS(ref_mpi_free(ref_grid_mpi(ref_grid)), "mpi free");

  ref_free(ref_grid);
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_parse_coordinate_system(
    REF_GRID ref_grid, const char *coordinate_system) {
  ref_grid_coordinate_system(ref_grid) = REF_GRID_COORDSYS_LAST;
  if (0 == strcmp("xByUzL", coordinate_system))
    ref_grid_coordinate_system(ref_grid) = REF_GRID_XBYUZL;
  if (0 == strcmp("xByRzU", coordinate_system))
    ref_grid_coordinate_system(ref_grid) = REF_GRID_XBYRZU;
  if (0 == strcmp("xFyRzD", coordinate_system))
    ref_grid_coordinate_system(ref_grid) = REF_GRID_XFYRZD;
  RUS(REF_GRID_COORDSYS_LAST, ref_grid_coordinate_system(ref_grid),
      "coordinate_system not recognized");
  return REF_SUCCESS;
}
REF_FCN REF_STATUS ref_grid_parse_unit(REF_GRID ref_grid, const char *unit) {
  ref_grid_unit(ref_grid) = REF_GRID_UNIT_LAST;
  if (0 == strcmp("in", unit)) ref_grid_unit(ref_grid) = REF_GRID_IN;
  if (0 == strcmp("ft", unit)) ref_grid_unit(ref_grid) = REF_GRID_FT;
  if (0 == strcmp("m", unit)) ref_grid_unit(ref_grid) = REF_GRID_M;
  if (0 == strcmp("cm", unit)) ref_grid_unit(ref_grid) = REF_GRID_CM;
  RUS(REF_GRID_UNIT_LAST, ref_grid_unit(ref_grid), "mesh_units not recognized");
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_inspect(REF_GRID ref_grid) {
  printf("ref_grid = %p\n", (void *)ref_grid);
  printf(" %d node\n", ref_node_n(ref_grid_node(ref_grid)));
  printf(" %d tet\n", ref_cell_n(ref_grid_tet(ref_grid)));
  printf(" %d pyr\n", ref_cell_n(ref_grid_pyr(ref_grid)));
  printf(" %d pri\n", ref_cell_n(ref_grid_pri(ref_grid)));
  printf(" %d hex\n", ref_cell_n(ref_grid_hex(ref_grid)));
  printf(" %d edg\n", ref_cell_n(ref_grid_edg(ref_grid)));
  printf(" %d ed2\n", ref_cell_n(ref_grid_ed2(ref_grid)));
  printf(" %d ed3\n", ref_cell_n(ref_grid_ed3(ref_grid)));
  printf(" %d tri\n", ref_cell_n(ref_grid_tri(ref_grid)));
  printf(" %d qua\n", ref_cell_n(ref_grid_qua(ref_grid)));
  printf(" %d geom\n", ref_geom_n(ref_grid_geom(ref_grid)));
  printf(" %p gather\n", (void *)(ref_grid_gather(ref_grid)->grid_file));
  printf(" %p adapt\n", (void *)(ref_grid->adapt));
  printf(" %p interp\n", (void *)(ref_grid->interp));
  printf(" %d partitioner\n", (int)(ref_grid->partitioner));
  printf(" %d partitioner seed\n", (int)(ref_grid->partitioner_seed));
  printf(" %d partitioner full\n", (int)(ref_grid->partitioner_full));
  printf(" %d mesb_version\n", (ref_grid->meshb_version));
  printf(" %d twod\n", (ref_grid->twod));
  printf(" %d surf\n", (ref_grid->surf));

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_tattle(REF_GRID ref_grid, REF_INT node) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_CELL ref_cell;
  REF_INT group;
  RSS(ref_node_location(ref_node, node), "loc");
  RSS(ref_geom_tattle(ref_geom, node), "geom tatt");
  each_ref_grid_all_ref_cell(ref_grid, group, ref_cell) {
    RSS(ref_cell_tattle_about(ref_cell, node), "cell tatt");
  }
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_cell_with(REF_GRID ref_grid, REF_INT node_per,
                                      REF_CELL *ref_cell) {
  *ref_cell = NULL;

  switch (node_per) {
    case 4:
      *ref_cell = ref_grid_tet(ref_grid);
      break;
    case 5:
      *ref_cell = ref_grid_pyr(ref_grid);
      break;
    case 6:
      *ref_cell = ref_grid_pri(ref_grid);
      break;
    case 8:
      *ref_cell = ref_grid_hex(ref_grid);
      break;
    default:
      printf("node_per %d\n", node_per);
      RSS(REF_FAILURE, "unexpected node_per");
      break;
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_face_with(REF_GRID ref_grid, REF_INT node_per,
                                      REF_CELL *ref_cell) {
  *ref_cell = NULL;

  switch (node_per) {
    case 3:
      *ref_cell = ref_grid_tri(ref_grid);
      break;
    case 4:
      *ref_cell = ref_grid_qua(ref_grid);
      break;
    default:
      printf("node_per %d\n", node_per);
      RSS(REF_FAILURE, "unexpected node_per");
      break;
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_cell_has_face(REF_GRID ref_grid,
                                          REF_INT *face_nodes,
                                          REF_BOOL *has_face) {
  REF_CELL ref_cell;
  REF_INT group;
  REF_INT cell0, cell1;

  *has_face = REF_FALSE;

  each_ref_grid_3d_ref_cell(ref_grid, group, ref_cell) {
    RSS(ref_cell_with_face(ref_cell, face_nodes, &cell0, &cell1),
        "face search failed");
    *has_face = (REF_EMPTY != cell0);
    if (*has_face) return REF_SUCCESS;
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_faceid_range(REF_GRID ref_grid, REF_INT *min_faceid,
                                         REF_INT *max_faceid) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_CELL ref_cell;
  REF_INT cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];

  *min_faceid = REF_INT_MAX;
  *max_faceid = REF_INT_MIN;

  ref_cell = ref_grid_tri(ref_grid);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    *min_faceid = MIN(*min_faceid, nodes[ref_cell_node_per(ref_cell)]);
    *max_faceid = MAX(*max_faceid, nodes[ref_cell_node_per(ref_cell)]);
  }

  ref_cell = ref_grid_qua(ref_grid);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    *min_faceid = MIN(*min_faceid, nodes[ref_cell_node_per(ref_cell)]);
    *max_faceid = MAX(*max_faceid, nodes[ref_cell_node_per(ref_cell)]);
  }

  if (ref_mpi_para(ref_mpi)) {
    REF_INT global;

    RSS(ref_mpi_min(ref_mpi, min_faceid, &global, REF_INT_TYPE),
        "mpi min face");
    RSS(ref_mpi_bcast(ref_mpi, &global, 1, REF_INT_TYPE), "mpi min face");
    *min_faceid = global;

    RSS(ref_mpi_max(ref_mpi, max_faceid, &global, REF_INT_TYPE),
        "mpi max face");
    RSS(ref_mpi_bcast(ref_mpi, &global, 1, REF_INT_TYPE), "mpi max face");
    *max_faceid = global;
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_tri_qua_id_nodes(REF_GRID ref_grid, REF_INT cell_id,
                                             REF_INT *nnode, REF_INT *ncell,
                                             REF_INT **g2l, REF_INT **l2g) {
  REF_NODE ref_node;
  REF_CELL ref_cell;
  REF_INT cell, node, cell_node;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];

  ref_node = ref_grid_node(ref_grid);

  ref_malloc_init(*g2l, ref_node_max(ref_node), REF_INT, REF_EMPTY);

  (*nnode) = 0;
  (*ncell) = 0;

  ref_cell = ref_grid_tri(ref_grid);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (cell_id == nodes[ref_cell_id_index(ref_cell)]) {
      (*ncell)++;
      each_ref_cell_cell_node(ref_cell, cell_node) {
        if (REF_EMPTY == (*g2l)[nodes[cell_node]]) {
          (*g2l)[nodes[cell_node]] = (*nnode);
          (*nnode)++;
        }
      }
    }
  }

  ref_cell = ref_grid_qua(ref_grid);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (cell_id == nodes[ref_cell_id_index(ref_cell)]) {
      (*ncell)++;
      each_ref_cell_cell_node(ref_cell, cell_node) {
        if (REF_EMPTY == (*g2l)[nodes[cell_node]]) {
          (*g2l)[nodes[cell_node]] = (*nnode);
          (*nnode)++;
        }
      }
    }
  }

  ref_malloc(*l2g, *nnode, REF_INT);

  (*nnode) = 0;
  for (node = 0; node < ref_node_max(ref_node); node++) {
    if (REF_EMPTY != (*g2l)[node]) {
      (*g2l)[node] = (*nnode);
      (*l2g)[(*nnode)] = node;
      (*nnode)++;
    }
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_cell_id_nodes(REF_GRID ref_grid, REF_CELL ref_cell,
                                          REF_INT cell_id, REF_INT *nnode,
                                          REF_INT *ncell, REF_INT **g2l,
                                          REF_INT **l2g) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT cell, node, cell_node;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];

  RAS(ref_cell_last_node_is_an_id(ref_cell), "ref_cell with id expected");

  ref_malloc_init(*g2l, ref_node_max(ref_node), REF_INT, REF_EMPTY);

  (*nnode) = 0;
  (*ncell) = 0;

  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (cell_id == nodes[ref_cell_id_index(ref_cell)]) {
      (*ncell)++;
      each_ref_cell_cell_node(ref_cell, cell_node) {
        if (REF_EMPTY == (*g2l)[nodes[cell_node]]) {
          (*g2l)[nodes[cell_node]] = (*nnode);
          (*nnode)++;
        }
      }
    }
  }

  ref_malloc(*l2g, *nnode, REF_INT);

  (*nnode) = 0;
  for (node = 0; node < ref_node_max(ref_node); node++) {
    if (REF_EMPTY != (*g2l)[node]) {
      (*g2l)[node] = (*nnode);
      (*l2g)[(*nnode)] = node;
      (*nnode)++;
    }
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_compact_cell_nodes(REF_GRID ref_grid,
                                               REF_CELL ref_cell,
                                               REF_GLOB *nnode_global,
                                               REF_LONG *ncell_global,
                                               REF_GLOB **l2c) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_INT cell, node, cell_node, part;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT nnode, ncell;
  REF_INT proc, *counts;
  REF_GLOB offset;

  ref_malloc_init(*l2c, ref_node_max(ref_node), REF_GLOB, REF_EMPTY);

  (*nnode_global) = 0;
  (*ncell_global) = 0;
  nnode = 0;
  ncell = 0;

  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    RSS(ref_cell_part(ref_cell, ref_node, cell, &part), "part");
    if (ref_mpi_rank(ref_mpi) == part) {
      ncell++;
    }
    each_ref_cell_cell_node(ref_cell, cell_node) {
      if (ref_node_owned(ref_node, nodes[cell_node]) &&
          (REF_EMPTY == (*l2c)[nodes[cell_node]])) {
        (*l2c)[nodes[cell_node]] = nnode;
        nnode++;
      }
    }
  }
  (*ncell_global) = ncell;
  RSS(ref_mpi_allsum(ref_mpi, ncell_global, 1, REF_LONG_TYPE), "allsum");

  ref_malloc(counts, ref_mpi_n(ref_mpi), REF_INT);
  RSS(ref_mpi_allgather(ref_mpi, &nnode, counts, REF_INT_TYPE), "gather size");
  offset = 0;
  for (proc = 0; proc < ref_mpi_rank(ref_mpi); proc++) {
    offset += counts[proc];
  }
  each_ref_mpi_part(ref_mpi, proc) { (*nnode_global) += counts[proc]; }
  ref_free(counts);

  for (node = 0; node < ref_node_max(ref_node); node++) {
    if (REF_EMPTY != (*l2c)[node]) {
      (*l2c)[node] += offset;
    }
  }

  RSS(ref_node_ghost_glob(ref_node, (*l2c), 1), "xfer");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_compact_cell_id_nodes(
    REF_GRID ref_grid, REF_CELL ref_cell, REF_INT cell_id,
    REF_GLOB *nnode_global, REF_LONG *ncell_global, REF_GLOB **l2c) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_INT cell, node, cell_node, part;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT nnode, ncell;
  REF_INT proc, *counts;
  REF_INT offset;

  ref_malloc_init(*l2c, ref_node_max(ref_node), REF_GLOB, REF_EMPTY);

  (*nnode_global) = 0;
  (*ncell_global) = 0;
  nnode = 0;
  ncell = 0;

  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    if (cell_id == nodes[ref_cell_id_index(ref_cell)]) {
      RSS(ref_cell_part(ref_cell, ref_node, cell, &part), "part");
      if (ref_mpi_rank(ref_mpi) == part) {
        ncell++;
      }
      each_ref_cell_cell_node(ref_cell, cell_node) {
        if (ref_node_owned(ref_node, nodes[cell_node]) &&
            (REF_EMPTY == (*l2c)[nodes[cell_node]])) {
          (*l2c)[nodes[cell_node]] = nnode;
          nnode++;
        }
      }
    }
  }

  (*ncell_global) = ncell;
  RSS(ref_mpi_allsum(ref_mpi, ncell_global, 1, REF_LONG_TYPE), "allsum");

  ref_malloc(counts, ref_mpi_n(ref_mpi), REF_INT);
  RSS(ref_mpi_allgather(ref_mpi, &nnode, counts, REF_INT_TYPE), "gather size");
  offset = 0;
  for (proc = 0; proc < ref_mpi_rank(ref_mpi); proc++) {
    offset += counts[proc];
  }
  each_ref_mpi_part(ref_mpi, proc) { (*nnode_global) += counts[proc]; }
  ref_free(counts);

  for (node = 0; node < ref_node_max(ref_node); node++) {
    if (REF_EMPTY != (*l2c)[node]) {
      (*l2c)[node] += offset;
    }
  }

  RSS(ref_node_ghost_glob(ref_node, (*l2c), 1), "xfer");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_compact_surf_id_nodes(REF_GRID ref_grid,
                                                  REF_INT cell_id,
                                                  REF_GLOB *nnode_global,
                                                  REF_LONG *ncell_global,
                                                  REF_GLOB **l2c) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_INT cell, node, cell_node, part;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT nnode, ncell;
  REF_INT proc, *counts;
  REF_INT offset, group;
  REF_CELL ref_cell;

  ref_malloc_init(*l2c, ref_node_max(ref_node), REF_GLOB, REF_EMPTY);

  (*nnode_global) = 0;
  (*ncell_global) = 0;
  nnode = 0;
  ncell = 0;

  each_ref_grid_2d_ref_cell(ref_grid, group,
                            ref_cell){each_ref_cell_valid_cell_with_nodes(
      ref_cell, cell, nodes){if (cell_id == nodes[ref_cell_id_index(ref_cell)]){
      RSS(ref_cell_part(ref_cell, ref_node, cell, &part), "part");
  if (ref_mpi_rank(ref_mpi) == part) {
    ncell++;
  }
  each_ref_cell_cell_node(ref_cell, cell_node) {
    if (ref_node_owned(ref_node, nodes[cell_node]) &&
        (REF_EMPTY == (*l2c)[nodes[cell_node]])) {
      (*l2c)[nodes[cell_node]] = nnode;
      nnode++;
    }
  }
}
}
}

(*ncell_global) = ncell;
RSS(ref_mpi_allsum(ref_mpi, ncell_global, 1, REF_LONG_TYPE), "allsum");

ref_malloc(counts, ref_mpi_n(ref_mpi), REF_INT);
RSS(ref_mpi_allgather(ref_mpi, &nnode, counts, REF_INT_TYPE), "gather size");
offset = 0;
for (proc = 0; proc < ref_mpi_rank(ref_mpi); proc++) {
  offset += counts[proc];
}
each_ref_mpi_part(ref_mpi, proc) { (*nnode_global) += counts[proc]; }
ref_free(counts);

for (node = 0; node < ref_node_max(ref_node); node++) {
  if (REF_EMPTY != (*l2c)[node]) {
    (*l2c)[node] += offset;
  }
}

RSS(ref_node_ghost_glob(ref_node, (*l2c), 1), "xfer");

return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_inward_boundary_orientation(REF_GRID ref_grid) {
  REF_CELL tri = ref_grid_tri(ref_grid);
  REF_CELL qua = ref_grid_qua(ref_grid);
  REF_CELL ref_cell;
  REF_INT cell, cell0, cell1;
  REF_INT node;
  REF_INT face_nodes[4];
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL flip;
  REF_INT face, group;

  each_ref_cell_valid_cell_with_nodes(tri, cell, nodes) {
    for (node = 0; node < 3; node++) face_nodes[node] = nodes[node];
    face_nodes[3] = face_nodes[0];
    flip = REF_FALSE;
    each_ref_grid_3d_ref_cell(ref_grid, group, ref_cell) {
      RSS(ref_cell_with_face(ref_cell, face_nodes, &cell0, &cell1),
          "with face");
      if (REF_EMPTY == cell0) continue; /* this cell does not have the face */
      if (REF_EMPTY != cell1) {
        printf("group %d cell0 %d cell1 %d\n", group, cell0, cell1);
        for (node = 0; node < ref_cell_node_per(ref_cell); node++) {
          printf("cell0 node %d xyz %f %f %f\n", node,
                 ref_node_xyz(ref_grid_node(ref_grid), 0,
                              ref_cell_c2n(ref_cell, node, cell0)),
                 ref_node_xyz(ref_grid_node(ref_grid), 1,
                              ref_cell_c2n(ref_cell, node, cell0)),
                 ref_node_xyz(ref_grid_node(ref_grid), 2,
                              ref_cell_c2n(ref_cell, node, cell0)));
        }
        for (node = 0; node < ref_cell_node_per(ref_cell); node++) {
          printf("cell1 node %d xyz %f %f %f\n", node,
                 ref_node_xyz(ref_grid_node(ref_grid), 0,
                              ref_cell_c2n(ref_cell, node, cell1)),
                 ref_node_xyz(ref_grid_node(ref_grid), 1,
                              ref_cell_c2n(ref_cell, node, cell1)),
                 ref_node_xyz(ref_grid_node(ref_grid), 2,
                              ref_cell_c2n(ref_cell, node, cell1)));
        }
        for (node = 0; node < 3; node++) {
          printf("face node %d xyz %f %f %f\n", node,
                 ref_node_xyz(ref_grid_node(ref_grid), 0, face_nodes[node]),
                 ref_node_xyz(ref_grid_node(ref_grid), 1, face_nodes[node]),
                 ref_node_xyz(ref_grid_node(ref_grid), 2, face_nodes[node]));
        }
        /* RSS( ref_export_by_extension( ref_grid,
           "bc-flip-debug.tec" ), "ex tec"); */
        REIS(REF_EMPTY, cell1, "boundary triangle with two cells");
      }
      each_ref_cell_cell_face(ref_cell, face) {
        if (ref_cell_f2n(ref_cell, 0, face, cell0) !=
            ref_cell_f2n(ref_cell, 3, face, cell0))
          continue; /* skip quad faces */
        if ((nodes[0] == ref_cell_f2n(ref_cell, 1, face, cell0) &&
             nodes[1] == ref_cell_f2n(ref_cell, 0, face, cell0) &&
             nodes[2] == ref_cell_f2n(ref_cell, 2, face, cell0)) ||
            (nodes[1] == ref_cell_f2n(ref_cell, 1, face, cell0) &&
             nodes[2] == ref_cell_f2n(ref_cell, 0, face, cell0) &&
             nodes[0] == ref_cell_f2n(ref_cell, 2, face, cell0)) ||
            (nodes[2] == ref_cell_f2n(ref_cell, 1, face, cell0) &&
             nodes[0] == ref_cell_f2n(ref_cell, 0, face, cell0) &&
             nodes[1] == ref_cell_f2n(ref_cell, 2, face, cell0))) {
          RAS(!flip, "error, flip set twice");
          flip = REF_TRUE;
        }
      }
    }
    if (flip) {
      nodes[0] = face_nodes[2];
      nodes[1] = face_nodes[1];
      nodes[2] = face_nodes[0];
      RSS(ref_cell_replace_whole(tri, cell, nodes), "replace with flip");
    }
  }

  each_ref_cell_valid_cell_with_nodes(qua, cell, nodes) {
    for (node = 0; node < 4; node++) face_nodes[node] = nodes[node];
    flip = REF_FALSE;
    each_ref_grid_3d_ref_cell(ref_grid, group, ref_cell) {
      RSS(ref_cell_with_face(ref_cell, face_nodes, &cell0, &cell1), "wf");
      if (REF_EMPTY == cell0) continue; /* this cell does not have the face */
      REIS(REF_EMPTY, cell1, "boundary quad with two cells");
      each_ref_cell_cell_face(ref_cell, face) {
        if (ref_cell_f2n(ref_cell, 0, face, cell0) ==
            ref_cell_f2n(ref_cell, 3, face, cell0))
          continue; /* skip tri faces */
        if ((nodes[0] == ref_cell_f2n(ref_cell, 3, face, cell0) &&
             nodes[1] == ref_cell_f2n(ref_cell, 2, face, cell0) &&
             nodes[2] == ref_cell_f2n(ref_cell, 1, face, cell0) &&
             nodes[3] == ref_cell_f2n(ref_cell, 0, face, cell0)) ||
            (nodes[0] == ref_cell_f2n(ref_cell, 2, face, cell0) &&
             nodes[1] == ref_cell_f2n(ref_cell, 1, face, cell0) &&
             nodes[2] == ref_cell_f2n(ref_cell, 0, face, cell0) &&
             nodes[3] == ref_cell_f2n(ref_cell, 3, face, cell0)) ||
            (nodes[0] == ref_cell_f2n(ref_cell, 1, face, cell0) &&
             nodes[1] == ref_cell_f2n(ref_cell, 0, face, cell0) &&
             nodes[2] == ref_cell_f2n(ref_cell, 3, face, cell0) &&
             nodes[3] == ref_cell_f2n(ref_cell, 2, face, cell0)) ||
            (nodes[0] == ref_cell_f2n(ref_cell, 0, face, cell0) &&
             nodes[1] == ref_cell_f2n(ref_cell, 3, face, cell0) &&
             nodes[2] == ref_cell_f2n(ref_cell, 2, face, cell0) &&
             nodes[3] == ref_cell_f2n(ref_cell, 1, face, cell0))) {
          RAS(!flip, "error, flip set twice");
          flip = REF_TRUE;
        }
      }
    }
    if (flip) {
      nodes[0] = face_nodes[3];
      nodes[1] = face_nodes[2];
      nodes[2] = face_nodes[1];
      nodes[3] = face_nodes[0];
      RSS(ref_cell_replace_whole(qua, cell, nodes), "replace with flip");
    }
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_orient_twod(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT cell;
  REF_INT cell_node;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL valid;
  ref_cell = ref_grid_tri(ref_grid);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    RSS(ref_node_tri_twod_orientation(ref_node, nodes, &valid), "valid");
    if (!valid) {
      each_ref_cell_cell_node(ref_cell, cell_node) {
        ref_cell_c2n(ref_cell, cell_node, cell) =
            nodes[ref_cell_node_per(ref_cell) - 1 - cell_node];
      }
    }
  }
  ref_cell = ref_grid_qua(ref_grid);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    /* uses tri formed from first three nodes of quad for orientation */
    RSS(ref_node_tri_twod_orientation(ref_node, nodes, &valid), "valid");
    if (!valid) {
      each_ref_cell_cell_node(ref_cell, cell_node) {
        ref_cell_c2n(ref_cell, cell_node, cell) =
            nodes[ref_cell_node_per(ref_cell) - 1 - cell_node];
      }
    }
  }
  /* orient edge after tri and quad sorted to use them as reference */
  ref_cell = ref_grid_edg(ref_grid);
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    RSS(ref_grid_orient_edg(ref_grid, nodes), "orient_edge");
    each_ref_cell_cell_node(ref_cell, cell_node) {
      ref_cell_c2n(ref_cell, cell_node, cell) = nodes[cell_node];
    }
  }
  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_update_tet_guess(REF_CELL ref_cell, REF_INT node0,
                                               REF_INT node1, REF_INT node2,
                                               REF_INT *guess) {
  REF_INT face_nodes[4], cell0, cell1;

  face_nodes[0] = node0;
  face_nodes[1] = node1;
  face_nodes[2] = node2;
  face_nodes[3] = node0;

  RSS(ref_cell_with_face(ref_cell, face_nodes, &cell0, &cell1), "next");
  if (REF_EMPTY == cell0) THROW("bary update missing first");
  if (REF_EMPTY == cell1) { /* hit boundary */
    *guess = REF_EMPTY;
    return REF_SUCCESS;
  }

  if (*guess == cell0) {
    *guess = cell1;
    return REF_SUCCESS;
  }
  if (*guess == cell1) {
    *guess = cell0;
    return REF_SUCCESS;
  }

  return REF_NOT_FOUND;
}

REF_FCN REF_STATUS ref_grid_node_list_around(REF_GRID ref_grid, REF_INT node,
                                             REF_INT max_node, REF_INT *nnode,
                                             REF_INT *node_list) {
  REF_INT cell, item, cell_node, haves, group;
  REF_CELL ref_cell;
  REF_BOOL already_have_it;

  *nnode = 0;
  each_ref_grid_2d_3d_ref_cell(ref_grid, group, ref_cell) {
    each_ref_cell_having_node(ref_cell, node, item, cell) {
      for (cell_node = 0; cell_node < ref_cell_node_per(ref_cell);
           cell_node++) {
        if (node == ref_cell_c2n(ref_cell, cell_node, cell)) continue;
        already_have_it = REF_FALSE;
        for (haves = 0; haves < *nnode; haves++)
          if (node_list[haves] == ref_cell_c2n(ref_cell, cell_node, cell)) {
            already_have_it = REF_TRUE;
            break;
          }
        if (!already_have_it) {
          if (*nnode >= max_node) {
            RSS(REF_INCREASE_LIMIT, "max_node too small");
          }
          node_list[*nnode] = ref_cell_c2n(ref_cell, cell_node, cell);
          (*nnode)++;
        }
      }
    }
  }

  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_grid_exhaustive_enclosing_tet(REF_GRID ref_grid,
                                                            REF_DBL *xyz,
                                                            REF_INT *tet,
                                                            REF_DBL *bary) {
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT guess, best_guess;
  REF_DBL best_bary, min_bary;

  best_guess = REF_EMPTY;
  best_bary = -999.0;
  each_ref_cell_valid_cell(ref_cell, guess) {
    RSS(ref_cell_nodes(ref_cell, guess, nodes), "cell");
    RXS(ref_node_bary4(ref_node, nodes, xyz, bary), REF_DIV_ZERO, "bary");
    min_bary = MIN(MIN(bary[0], bary[1]), MIN(bary[2], bary[3]));
    if (REF_EMPTY == best_guess || min_bary > best_bary) {
      best_guess = guess;
      best_bary = min_bary;
    }
  }

  RUS(REF_EMPTY, best_guess, "failed to find cell");

  *tet = best_guess;
  RSS(ref_cell_nodes(ref_cell, best_guess, nodes), "cell");
  RSS(ref_node_bary4(ref_node, nodes, xyz, bary), "bary");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_enclosing_tet(REF_GRID ref_grid, REF_DBL *xyz,
                                          REF_INT *tet, REF_DBL *bary) {
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT guess;
  REF_DBL inside;
  REF_INT step, limit;

  guess = *tet;
  if (!ref_cell_valid(ref_cell, guess)) {
    each_ref_cell_valid_cell(ref_cell, guess) { break; }
  }

  limit = 1000;      /* was 10e6^(1/3), required 108 for twod testcase  */
  inside = -1.0e-12; /* initial inside tolerance  */

  for (step = 0; step < limit; step++) {
    /* exhaustive search if cell is invalid */
    if (!ref_cell_valid(ref_cell, guess)) {
      RSS(ref_grid_exhaustive_enclosing_tet(ref_grid, xyz, tet, bary),
          "enclose");
      return REF_SUCCESS;
    }

    RSS(ref_cell_nodes(ref_cell, guess, nodes), "cell");
    RXS(ref_node_bary4(ref_node, nodes, xyz, bary), REF_DIV_ZERO, "bary");

    if (step > 990) {
      printf("step %d, tet %d, bary %e %e %e %e inside %e\n", step, guess,
             bary[0], bary[1], bary[2], bary[3], inside);
    }

    if (bary[0] >= inside && bary[1] >= inside && bary[2] >= inside &&
        bary[3] >= inside) {
      *tet = guess;
      return REF_SUCCESS;
    }

    /* less than */
    if (bary[0] < bary[1] && bary[0] < bary[2] && bary[0] < bary[3]) {
      RSS(ref_update_tet_guess(ref_cell, nodes[1], nodes[2], nodes[3], &guess),
          "update 1 2 3");
      continue;
    }

    if (bary[1] < bary[0] && bary[1] < bary[3] && bary[1] < bary[2]) {
      RSS(ref_update_tet_guess(ref_cell, nodes[0], nodes[3], nodes[2], &guess),
          "update 0 3 2");
      continue;
    }

    if (bary[2] < bary[0] && bary[2] < bary[1] && bary[2] < bary[3]) {
      RSS(ref_update_tet_guess(ref_cell, nodes[0], nodes[1], nodes[3], &guess),
          "update 0 1 3");
      continue;
    }

    if (bary[3] < bary[0] && bary[3] < bary[2] && bary[3] < bary[1]) {
      RSS(ref_update_tet_guess(ref_cell, nodes[0], nodes[2], nodes[1], &guess),
          "update 0 2 1");
      continue;
    }

    /* less than or equal */
    if (bary[0] <= bary[1] && bary[0] <= bary[2] && bary[0] <= bary[3]) {
      RSS(ref_update_tet_guess(ref_cell, nodes[1], nodes[2], nodes[3], &guess),
          "update 1 2 3");
      continue;
    }

    if (bary[1] <= bary[0] && bary[1] <= bary[3] && bary[1] <= bary[2]) {
      RSS(ref_update_tet_guess(ref_cell, nodes[0], nodes[3], nodes[2], &guess),
          "update 0 3 2");
      continue;
    }

    if (bary[2] <= bary[0] && bary[2] <= bary[1] && bary[2] <= bary[3]) {
      RSS(ref_update_tet_guess(ref_cell, nodes[0], nodes[1], nodes[3], &guess),
          "update 0 1 3");
      continue;
    }

    if (bary[3] <= bary[0] && bary[3] <= bary[2] && bary[3] <= bary[1]) {
      RSS(ref_update_tet_guess(ref_cell, nodes[0], nodes[2], nodes[1], &guess),
          "update 0 2 1");
      continue;
    }

    THROW("unable to find the next step");
  }

  /* exhaustive search when walk runs out of iterations */
  RSS(ref_grid_exhaustive_enclosing_tet(ref_grid, xyz, tet, bary), "enclose");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_extrude_twod(REF_GRID *extruded_grid,
                                         REF_GRID twod_grid, REF_INT n_planes) {
  REF_GRID ref_grid;
  REF_NODE ref_node;
  REF_NODE twod_node = ref_grid_node(twod_grid);
  REF_INT plane, node, new_node;
  REF_INT offset, offset0, offset1;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER], new_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT cell, new_cell;
  REF_INT max_faceid, faceid1, faceid2;
  REF_GLOB global_offset;
  REF_DBL y;
  *extruded_grid = NULL;
  RAS(ref_grid_twod(twod_grid), "require twod grid input");
  RSS(ref_grid_create(extruded_grid, ref_grid_mpi(twod_grid)), "create grid");
  ref_grid = *extruded_grid;
  ref_node = ref_grid_node(ref_grid);

  global_offset = 0;
  for (plane = 0; plane < n_planes; plane++) {
    y = (REF_DBL)plane / (REF_DBL)(n_planes - 1);
    each_ref_node_valid_node(twod_node, node) {
      RSS(ref_node_add(ref_node,
                       ref_node_global(twod_node, node) + global_offset,
                       &new_node),
          "add node in plane");
      ref_node_part(ref_node, new_node) = ref_node_part(twod_node, node);
      ref_node_xyz(ref_node, 0, new_node) = ref_node_xyz(twod_node, 0, node);
      ref_node_xyz(ref_node, 1, new_node) = y;
      ref_node_xyz(ref_node, 2, new_node) = ref_node_xyz(twod_node, 1, node);
    }
    global_offset += ref_node_n_global(twod_node);
  }
  RSS(ref_node_initialize_n_global(ref_node, global_offset), "init glob");

  each_ref_cell_valid_cell_with_nodes(ref_grid_edg(twod_grid), cell, nodes) {
    REF_INT node0, node1;
    /* use triangle to orient edge correctly */
    RSS(ref_grid_orient_edg(twod_grid, nodes), "orient_edge");
    node0 = nodes[0];
    node1 = nodes[1];
    for (plane = 0; plane < n_planes - 1; plane++) {
      offset0 = (0 + plane) * ref_node_n(twod_node);
      offset1 = (1 + plane) * ref_node_n(twod_node);
      new_nodes[0] = node1 + offset0;
      new_nodes[1] = node0 + offset0;
      new_nodes[2] = node0 + offset1;
      new_nodes[3] = node1 + offset1;
      new_nodes[4] = nodes[2];
      RSS(ref_cell_add(ref_grid_qua(ref_grid), new_nodes, &new_cell),
          "boundary");
    }
  }

  /* find two unused faceids for the symmetry planes */
  max_faceid = REF_INT_MIN;
  each_ref_cell_valid_cell_with_nodes(ref_grid_edg(twod_grid), cell, nodes) {
    max_faceid = MAX(max_faceid, nodes[2]);
  }
  faceid1 = max_faceid;
  RSS(ref_mpi_max(ref_grid_mpi(twod_grid), &faceid1, &max_faceid, REF_INT_TYPE),
      "max faceid");
  RSS(ref_mpi_bcast(ref_grid_mpi(twod_grid), &max_faceid, 1, REF_INT_TYPE),
      "share max faceid");
  faceid1 = max_faceid + 1;
  faceid2 = max_faceid + 2;

  offset = (n_planes - 1) * ref_node_n(twod_node);
  each_ref_cell_valid_cell_with_nodes(ref_grid_tri(twod_grid), cell, nodes) {
    new_nodes[0] = nodes[0];
    new_nodes[1] = nodes[1];
    new_nodes[2] = nodes[2];
    new_nodes[3] = faceid1;
    RSS(ref_cell_add(ref_grid_tri(ref_grid), new_nodes, &new_cell), "boundary");
    new_nodes[0] = nodes[0] + offset;
    new_nodes[1] = nodes[2] + offset;
    new_nodes[2] = nodes[1] + offset;
    new_nodes[3] = faceid2;
    RSS(ref_cell_add(ref_grid_tri(ref_grid), new_nodes, &new_cell), "boundary");
    for (plane = 0; plane < n_planes - 1; plane++) {
      offset0 = (0 + plane) * ref_node_n(twod_node);
      offset1 = (1 + plane) * ref_node_n(twod_node);
      new_nodes[0] = nodes[0] + offset0;
      new_nodes[1] = nodes[1] + offset0;
      new_nodes[2] = nodes[2] + offset0;
      new_nodes[3] = nodes[0] + offset1;
      new_nodes[4] = nodes[1] + offset1;
      new_nodes[5] = nodes[2] + offset1;
      RSS(ref_cell_add(ref_grid_pri(ref_grid), new_nodes, &new_cell),
          "interior");
    }
  }

  offset = (n_planes - 1) * ref_node_n(twod_node);
  each_ref_cell_valid_cell_with_nodes(ref_grid_qua(twod_grid), cell, nodes) {
    new_nodes[0] = nodes[3];
    new_nodes[1] = nodes[2];
    new_nodes[2] = nodes[1];
    new_nodes[3] = nodes[0];
    new_nodes[4] = faceid1;
    RSS(ref_cell_add(ref_grid_qua(ref_grid), new_nodes, &new_cell), "boundary");
    new_nodes[0] = nodes[0] + offset;
    new_nodes[1] = nodes[1] + offset;
    new_nodes[2] = nodes[2] + offset;
    new_nodes[3] = nodes[3] + offset;
    new_nodes[4] = faceid2;
    RSS(ref_cell_add(ref_grid_qua(ref_grid), new_nodes, &new_cell), "boundary");
    for (plane = 0; plane < n_planes - 1; plane++) {
      offset0 = (0 + plane) * ref_node_n(twod_node);
      offset1 = (1 + plane) * ref_node_n(twod_node);
      new_nodes[0] = nodes[0] + offset0;
      new_nodes[1] = nodes[1] + offset0;
      new_nodes[2] = nodes[2] + offset0;
      new_nodes[3] = nodes[3] + offset0;
      new_nodes[4] = nodes[0] + offset1;
      new_nodes[5] = nodes[1] + offset1;
      new_nodes[6] = nodes[2] + offset1;
      new_nodes[7] = nodes[3] + offset1;
      RSS(ref_cell_add(ref_grid_hex(ref_grid), new_nodes, &new_cell),
          "boundary");
    }
  }

  return REF_SUCCESS;
}

/* use triangle or quad to orient edge correctly */
REF_FCN REF_STATUS ref_grid_orient_edg(REF_GRID ref_grid, REF_INT *nodes) {
  REF_INT ntri, edg_tri, nqua, edg_qua;
  REF_INT tri_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT qua_nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT node0, node1;
  RSS(ref_cell_list_with2(ref_grid_tri(ref_grid), nodes[0], nodes[1], 1, &ntri,
                          &edg_tri),
      "tri with edge side");
  RSS(ref_cell_list_with2(ref_grid_qua(ref_grid), nodes[0], nodes[1], 1, &nqua,
                          &edg_qua),
      "qua with edge side");
  REIS(1, ntri + nqua, "expect one tri/qua for an edge");
  REIS(0, MIN(ntri, nqua), "expect zero min tri/qua for an edge");
  node0 = REF_EMPTY;
  node1 = REF_EMPTY;
  if (1 == ntri) {
    RSS(ref_cell_nodes(ref_grid_tri(ref_grid), edg_tri, tri_nodes),
        "tri nodes");
    if ((MIN(nodes[0], nodes[1]) == MIN(tri_nodes[0], tri_nodes[1])) &&
        (MAX(nodes[0], nodes[1]) == MAX(tri_nodes[0], tri_nodes[1]))) {
      node0 = tri_nodes[0];
      node1 = tri_nodes[1];
    }
    if ((MIN(nodes[0], nodes[1]) == MIN(tri_nodes[1], tri_nodes[2])) &&
        (MAX(nodes[0], nodes[1]) == MAX(tri_nodes[1], tri_nodes[2]))) {
      node0 = tri_nodes[1];
      node1 = tri_nodes[2];
    }
    if ((MIN(nodes[0], nodes[1]) == MIN(tri_nodes[2], tri_nodes[0])) &&
        (MAX(nodes[0], nodes[1]) == MAX(tri_nodes[2], tri_nodes[0]))) {
      node0 = tri_nodes[2];
      node1 = tri_nodes[0];
    }
  }
  if (1 == nqua) {
    RSS(ref_cell_nodes(ref_grid_qua(ref_grid), edg_qua, qua_nodes),
        "qua nodes");
    if ((MIN(nodes[0], nodes[1]) == MIN(qua_nodes[0], qua_nodes[1])) &&
        (MAX(nodes[0], nodes[1]) == MAX(qua_nodes[0], qua_nodes[1]))) {
      node0 = qua_nodes[0];
      node1 = qua_nodes[1];
    }
    if ((MIN(nodes[0], nodes[1]) == MIN(qua_nodes[1], qua_nodes[2])) &&
        (MAX(nodes[0], nodes[1]) == MAX(qua_nodes[1], qua_nodes[2]))) {
      node0 = qua_nodes[1];
      node1 = qua_nodes[2];
    }
    if ((MIN(nodes[0], nodes[1]) == MIN(qua_nodes[2], qua_nodes[3])) &&
        (MAX(nodes[0], nodes[1]) == MAX(qua_nodes[2], qua_nodes[3]))) {
      node0 = qua_nodes[2];
      node1 = qua_nodes[3];
    }
    if ((MIN(nodes[0], nodes[1]) == MIN(qua_nodes[3], qua_nodes[0])) &&
        (MAX(nodes[0], nodes[1]) == MAX(qua_nodes[3], qua_nodes[0]))) {
      node0 = qua_nodes[3];
      node1 = qua_nodes[0];
    }
  }
  RUB(REF_EMPTY, node0, "node0 not found", {
    printf("nodes %d %d %d ntri %d nqua %d node0 %d node1 %d\n", nodes[0],
           nodes[1], nodes[2], ntri, nqua, node0, node1);
    if (nqua > 0)
      printf("qua %d %d %d %d %d\n", qua_nodes[0], qua_nodes[1], qua_nodes[2],
             qua_nodes[3], qua_nodes[4]);
  });
  RUS(REF_EMPTY, node1, "node1 not found");
  /* same direction as triangle or quad side */
  nodes[0] = node0;
  nodes[1] = node1;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_drop_volume(REF_GRID ref_grid) {
  REF_MPI ref_mpi = ref_grid_mpi(ref_grid);
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node;
  RSS(ref_cell_free(ref_grid_hex(ref_grid)), "hex free");
  RSS(ref_cell_free(ref_grid_pri(ref_grid)), "pri free");
  RSS(ref_cell_free(ref_grid_pyr(ref_grid)), "pyr free");
  RSS(ref_cell_free(ref_grid_tet(ref_grid)), "tet free");
  RSS(ref_cell_create(&ref_grid_tet(ref_grid), REF_CELL_TET), "tet create");
  RSS(ref_cell_create(&ref_grid_pyr(ref_grid), REF_CELL_PYR), "pyr create");
  RSS(ref_cell_create(&ref_grid_pri(ref_grid), REF_CELL_PRI), "pri create");
  RSS(ref_cell_create(&ref_grid_hex(ref_grid), REF_CELL_HEX), "hex create");
  ref_mpi_stopwatch_stop(ref_mpi, "dump vol cells");
  each_ref_node_valid_node(ref_node, node) {
    if (ref_cell_node_empty(ref_grid_qua(ref_grid), node) &&
        ref_cell_node_empty(ref_grid_tri(ref_grid), node) &&
        ref_cell_node_empty(ref_grid_edg(ref_grid), node)) {
      RSS(ref_node_remove_invalidates_sorted(ref_node, node), "rm node");
    }
  }
  ref_mpi_stopwatch_stop(ref_mpi, "del nodes");
  RSS(ref_node_rebuild_sorted_global(ref_node), "rebuild");
  ref_mpi_stopwatch_stop(ref_mpi, "rebuild nodes");
  RSS(ref_node_synchronize_globals(ref_node), "sync, lazy delete globals");
  ref_mpi_stopwatch_stop(ref_mpi, "sync nodes");
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_ncell(REF_GRID ref_grid, REF_INT *ncell) {
  REF_INT group;
  REF_CELL ref_cell;
  *ncell = 0;
  each_ref_grid_all_ref_cell(ref_grid, group, ref_cell) {
    (*ncell) += ref_cell_n(ref_cell);
  }
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_contiguous_group_cell(REF_GRID ref_grid,
                                                  REF_INT contiguous_cell,
                                                  REF_INT *cell_group,
                                                  REF_INT *cell) {
  REF_CELL ref_cell;
  REF_INT group;
  if (contiguous_cell < 0) {
    *cell_group = REF_EMPTY;
    *cell = REF_EMPTY;
    return REF_NOT_FOUND;
  }
  *cell = contiguous_cell;
  *cell_group = REF_EMPTY;
  each_ref_grid_all_ref_cell(ref_grid, group, ref_cell) {
    if (0 < ref_cell_n(ref_cell) && 0 <= *cell &&
        *cell < ref_cell_n(ref_cell)) {
      *cell_group = group;
      break;
    }
    (*cell) -= ref_cell_n(ref_cell);
  }
  if (REF_EMPTY == *cell_group) {
    *cell = REF_EMPTY;
    return REF_NOT_FOUND;
  }
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_grid_contiguous_cell_global(REF_GRID ref_grid,
                                                   REF_LONG *global) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT group, ncell, cell;
  REF_CELL ref_cell;
  REF_LONG *cell_global, global_ncell, offset;
  ncell = 0;
  offset = 0;
  each_ref_grid_all_ref_cell(ref_grid, group, ref_cell) {
    RSS(ref_cell_global(ref_cell, ref_node, &cell_global), "cell global");
    each_ref_cell_valid_cell(ref_cell, cell) {
      if (cell >= ref_cell_n(ref_cell)) {
        ref_free(cell_global);
        RSS(REF_FAILURE, "pack grid before calling");
      }
      global[ncell] = cell_global[cell] + offset;
      ncell++;
    }
    ref_free(cell_global);
    RSS(ref_cell_ncell(ref_cell, ref_node, &global_ncell), "global ncell");
    offset += global_ncell;
  }
  return REF_SUCCESS;
}
