
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

#include "ref_adj.h"
#include "ref_cell.h"
#include "ref_grid.h"
#include "ref_list.h"
#include "ref_matrix.h"
#include "ref_node.h"

#include "ref_sort.h"

#include "ref_migrate.h"

#include "ref_dict.h"
#include "ref_export.h"
#include "ref_fixture.h"

#include "ref_mpi.h"
#include "ref_part.h"

#include "ref_edge.h"
#include "ref_gather.h"
#include "ref_import.h"
#include "ref_malloc.h"
#include "ref_twod.h"

int main(int argc, char *argv[]) {
  REF_MPI ref_mpi;
  RSS(ref_mpi_start(argc, argv), "start");
  RSS(ref_mpi_create(&ref_mpi), "make mpi");

  if (2 == argc) {
    REF_GRID import_grid;

    ref_mpi_stopwatch_start(ref_mpi);
    RSS(ref_part_by_extension(&import_grid, ref_mpi, argv[1]), "import");
    ref_mpi_stopwatch_stop(ref_grid_mpi(import_grid), "read");
    RSS(ref_migrate_to_balance(import_grid), "balance");
    ref_mpi_stopwatch_stop(ref_grid_mpi(import_grid), "balance");

    ref_mpi_stopwatch_start(ref_grid_mpi(import_grid));
    RSS(ref_gather_by_extension(import_grid, "ref_gather_test.meshb"),
        "gather");
    ref_mpi_stopwatch_stop(ref_grid_mpi(import_grid), "meshb");

    ref_mpi_stopwatch_start(ref_grid_mpi(import_grid));
    RSS(ref_gather_by_extension(import_grid, "ref_gather_test.b8.ugrid"),
        "gather");
    ref_mpi_stopwatch_stop(ref_grid_mpi(import_grid), "b8.ugrid");

    RSS(ref_grid_free(import_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "mpi free");
    RSS(ref_mpi_stop(), "stop");

    return 0;
  }

  if (4 == argc) {
    REF_GRID import_grid;
    REF_INT ldim;
    REF_DBL *field;

    ref_mpi_stopwatch_start(ref_mpi);
    RSS(ref_part_by_extension(&import_grid, ref_mpi, argv[1]), "import");
    ref_mpi_stopwatch_stop(ref_grid_mpi(import_grid), "read grid");
    RSS(ref_part_scalar(ref_grid_node(import_grid), &ldim, &field, argv[2]),
        "field");
    ref_mpi_stopwatch_stop(ref_grid_mpi(import_grid), "read field");
    RSS(ref_gather_scalar_tec(import_grid, ldim, field, NULL, argv[3]),
        "field");
    ref_mpi_stopwatch_stop(ref_grid_mpi(import_grid), "write tec");

    RSS(ref_grid_free(import_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "mpi free");
    RSS(ref_mpi_stop(), "stop");

    return 0;
  }

  { /* gather prism lb8 */
    REF_GRID ref_grid;

    RSS(ref_fixture_pri_stack_grid(&ref_grid, ref_mpi), "set up tet");

    RSS(ref_gather_by_extension(ref_grid, "ref_gather_test.lb8.ugrid"),
        "gather");

    RSS(ref_grid_free(ref_grid), "free");
    if (ref_mpi_once(ref_mpi))
      REIS(0, remove("ref_gather_test.lb8.ugrid"), "test clean up");
  }

  { /* gather prism b8 */
    REF_GRID ref_grid;

    RSS(ref_fixture_pri_stack_grid(&ref_grid, ref_mpi), "set up tet");

    RSS(ref_gather_by_extension(ref_grid, "ref_gather_test.b8.ugrid"),
        "gather");

    RSS(ref_grid_free(ref_grid), "free");
    if (ref_mpi_once(ref_mpi))
      REIS(0, remove("ref_gather_test.b8.ugrid"), "test clean up");
  }

  { /* export import .meshb tet with cad_model */
    REF_GRID export_grid, import_grid;
    REF_GEOM ref_geom;
    char file[] = "ref_gather_test.meshb";

    RSS(ref_fixture_tet_grid(&export_grid, ref_mpi), "set up tet");
    ref_geom = ref_grid_geom(export_grid);
    ref_geom_cad_data_size(ref_geom) = 3;
    ref_malloc(ref_geom_cad_data(ref_geom), ref_geom_cad_data_size(ref_geom),
               REF_BYTE);
    ref_geom_cad_data(ref_geom)[0] = 5;
    ref_geom_cad_data(ref_geom)[1] = 4;
    ref_geom_cad_data(ref_geom)[2] = 3;
    RSS(ref_gather_by_extension(export_grid, file), "gather");
    RSS(ref_grid_free(export_grid), "free");

    if (ref_mpi_once(ref_mpi)) {
      RSS(ref_import_by_extension(&import_grid, ref_mpi, file), "import");
      ref_geom = ref_grid_geom(import_grid);
      REIS(3, ref_geom_cad_data_size(ref_geom), "cad size");
      REIS(5, ref_geom_cad_data(ref_geom)[0], "cad[0]");
      REIS(4, ref_geom_cad_data(ref_geom)[1], "cad[1]");
      REIS(3, ref_geom_cad_data(ref_geom)[2], "cad[2]");
      RSS(ref_grid_free(import_grid), "free");
      REIS(0, remove(file), "test clean up");
    }
  }

  RSS(ref_mpi_free(ref_mpi), "mpi free");
  RSS(ref_mpi_stop(), "stop");

  return 0;
}
