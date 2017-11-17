
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ref_import.h"
#include "ref_export.h"

#include "ref_fixture.h"

#include "ref_adj.h"
#include "ref_sort.h"
#include "ref_dict.h"
#include "ref_matrix.h"

#include "ref_cell.h"
#include "ref_node.h"
#include "ref_list.h"
#include "ref_geom.h"
#include "ref_math.h"

#include "ref_mpi.h"
#include "ref_edge.h"
#include "ref_malloc.h"

int main( int argc, char *argv[] )
{
  REF_MPI ref_mpi;
 
  if (2 == argc) 
    {
      RSS( ref_import_examine_header( argv[1] ), "examine header" );
      return 0;
    }

  RSS( ref_mpi_create( &ref_mpi ), "create" );

  { /* export import twod .msh brick */
    REF_GRID export_grid, import_grid;
    char file[] = "ref_import_test.msh";
    RSS(ref_fixture_twod_brick_grid( &export_grid, ref_mpi ), "set up tet" );
    RSS(ref_export_twod_msh( export_grid, file ), "export" );
    RSS(ref_import_by_extension( &import_grid, ref_mpi, file ), "import" );
    REIS( ref_node_n(ref_grid_node(export_grid)),
	  ref_node_n(ref_grid_node(import_grid)), "node count" );
    REIS( ref_cell_n(ref_grid_qua(export_grid)),
	  ref_cell_n(ref_grid_qua(import_grid)), "qua count" );
    REIS( ref_cell_n(ref_grid_tri(export_grid)),
	  ref_cell_n(ref_grid_tri(import_grid)), "tri count" );
    RSS(ref_grid_free(import_grid),"free");
    RSS(ref_grid_free(export_grid),"free");
    REIS(0, remove( file ), "test clean up");
  }

  { /* export import twod .meshb brick */
    REF_GRID export_grid, import_grid;
    char file[] = "ref_import_test.2d.meshb";
    RSS(ref_fixture_twod_brick_grid( &export_grid, ref_mpi ), "set up tet" );
    RSS(ref_export_twod_meshb( export_grid, file ), "export" );
    RSS(ref_import_by_extension( &import_grid, ref_mpi, file ), "import" );
    REIS( ref_node_n(ref_grid_node(export_grid)),
	  ref_node_n(ref_grid_node(import_grid)), "node count" );
    REIS( ref_cell_n(ref_grid_qua(export_grid)),
	  ref_cell_n(ref_grid_qua(import_grid)), "qua count" );
    REIS( ref_cell_n(ref_grid_tri(export_grid)),
	  ref_cell_n(ref_grid_tri(import_grid)), "tri count" );
    RSS(ref_grid_free(import_grid),"free");
    RSS(ref_grid_free(export_grid),"free");
    REIS(0, remove( file ), "test clean up");
  }

  { /* export import .meshb tet brick */
    REF_GRID export_grid, import_grid;
    char file[] = "ref_import_test.meshb";
    RSS(ref_fixture_tet_brick_grid( &export_grid, ref_mpi ), "set up tet" );
    RSS(ref_export_by_extension( export_grid, file ), "export" );
    RSS(ref_import_by_extension( &import_grid, ref_mpi, file ), "import" );
    REIS( ref_node_n(ref_grid_node(export_grid)),
	  ref_node_n(ref_grid_node(import_grid)), "node count" );
    REIS( ref_cell_n(ref_grid_qua(export_grid)),
	  ref_cell_n(ref_grid_qua(import_grid)), "qua count" );
    REIS( ref_cell_n(ref_grid_tri(export_grid)),
	  ref_cell_n(ref_grid_tri(import_grid)), "tri count" );
    REIS( ref_cell_n(ref_grid_tet(export_grid)),
	  ref_cell_n(ref_grid_tet(import_grid)), "tet count" );
    RSS(ref_grid_free(import_grid),"free");
    RSS(ref_grid_free(export_grid),"free");
    REIS(0, remove( file ), "test clean up");
  }

  { /* export import .meshb tet brick with cad_model */
    REF_GRID export_grid, import_grid;
    REF_GEOM ref_geom;
    char file[] = "ref_import_test.meshb";
    RSS(ref_fixture_tet_brick_grid( &export_grid, ref_mpi ), "set up tet" );
    ref_geom = ref_grid_geom(export_grid);
    ref_geom_cad_data_size(ref_geom) = 5;
    ref_malloc(ref_geom_cad_data(ref_geom), ref_geom_cad_data_size(ref_geom),
	       REF_BYTE );
    ref_geom_cad_data(ref_geom)[0] = 5;
    ref_geom_cad_data(ref_geom)[1] = 4;
    ref_geom_cad_data(ref_geom)[2] = 3;
    ref_geom_cad_data(ref_geom)[3] = 2;
    ref_geom_cad_data(ref_geom)[4] = 1;
    RSS(ref_export_by_extension( export_grid, file ), "export" );
    RSS(ref_import_by_extension( &import_grid, ref_mpi, file ), "import" );
    ref_geom = ref_grid_geom(import_grid);
    REIS( 5, ref_geom_cad_data_size(ref_geom), "cad size" );
    REIS( 5, ref_geom_cad_data(ref_geom)[0], "cad[0]" );
    REIS( 4, ref_geom_cad_data(ref_geom)[1], "cad[1]" );
    REIS( 3, ref_geom_cad_data(ref_geom)[2], "cad[2]" );
    REIS( 2, ref_geom_cad_data(ref_geom)[3], "cad[3]" );
    REIS( 1, ref_geom_cad_data(ref_geom)[4], "cad[4]" );
    RSS(ref_grid_free(import_grid),"free");
    RSS(ref_grid_free(export_grid),"free");
    REIS(0, remove( file ), "test clean up");
  }

  { /* export import .meshb tet brick with geom */
    REF_GRID export_grid, import_grid;
    REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];
    REF_GEOM ref_geom;
    REF_INT type, id, node;
    REF_DBL param[2];
    char file[] = "ref_import_test.meshb";
    RSS(ref_fixture_tet_brick_grid( &export_grid, ref_mpi ), "set up tet" );
    ref_geom = ref_grid_geom(export_grid);
    nodes[0] = 0; nodes[1] = 1; nodes[2] = 15;
    RSS(ref_cell_add(ref_grid_edg(export_grid), nodes, &cell ), "add edge" );
    type = REF_GEOM_NODE;
    id = 1; node = 0;
    RSS( ref_geom_add(ref_geom,node,type,id,param), "add geom node" );
    id = 2; node = 1;
    RSS( ref_geom_add(ref_geom,node,type,id,param), "add geom node" );
    type = REF_GEOM_EDGE;
    id = 15; node = 0; param[0] = 10.0;
    RSS( ref_geom_add(ref_geom,node,type,id,param), "add geom edge" );
    id = 15; node = 1; param[0] = 20.0;
    RSS( ref_geom_add(ref_geom,node,type,id,param), "add geom edge" );
    type = REF_GEOM_FACE;
    id = 3; node = 0; param[0] = 10.0; param[1] = 20.0;
    RSS( ref_geom_add(ref_geom,node,type,id,param), "add geom face" );
    type = REF_GEOM_FACE;
    id = 3; node = 1; param[0] = 11.0; param[1] = 20.0;
    RSS( ref_geom_add(ref_geom,node,type,id,param), "add geom face" );
    type = REF_GEOM_FACE;
    id = 3; node = 2; param[0] = 10.5; param[1] = 21.0;
    RSS( ref_geom_add(ref_geom,node,type,id,param), "add geom face" );
    
    RSS(ref_export_by_extension( export_grid, file ), "export" );
    RSS(ref_import_by_extension( &import_grid, ref_mpi, file ), "import" );

    REIS( ref_node_n(ref_grid_node(export_grid)),
	  ref_node_n(ref_grid_node(import_grid)), "node count" );
    REIS( ref_cell_n(ref_grid_edg(export_grid)),
	  ref_cell_n(ref_grid_edg(import_grid)), "edg count" );
    REIS( ref_cell_n(ref_grid_qua(export_grid)),
	  ref_cell_n(ref_grid_qua(import_grid)), "qua count" );
    REIS( ref_cell_n(ref_grid_tri(export_grid)),
	  ref_cell_n(ref_grid_tri(import_grid)), "tri count" );
    REIS( ref_cell_n(ref_grid_tet(export_grid)),
	  ref_cell_n(ref_grid_tet(import_grid)), "tet count" );

    REIS( ref_geom_n(ref_grid_geom(export_grid)),
	  ref_geom_n(ref_grid_geom(import_grid)), "tet count" );

    RSS(ref_grid_free(import_grid),"free");
    RSS(ref_grid_free(export_grid),"free");
    REIS(0, remove( file ), "test clean up");
  }

  { /* export import .fgrid tet */
    REF_GRID export_grid, import_grid;
    char file[] = "ref_import_test.fgrid";
    RSS(ref_fixture_tet_grid( &export_grid, ref_mpi ), "set up tet" );
    RSS(ref_export_by_extension( export_grid, file ), "export" );
    RSS(ref_import_by_extension( &import_grid, ref_mpi, file ), "import" );
    REIS( ref_node_n(ref_grid_node(export_grid)),
	  ref_node_n(ref_grid_node(import_grid)), "node count" );
    RSS(ref_grid_free(import_grid),"free");
    RSS(ref_grid_free(export_grid),"free");
    REIS(0, remove( file ), "test clean up");
  }

  { /* export import .ugrid tet */
    REF_GRID export_grid, import_grid;
    char file[] = "ref_import_test.ugrid";
    RSS(ref_fixture_tet_grid( &export_grid, ref_mpi ), "set up tet" );
    RSS(ref_export_by_extension( export_grid, file ), "export" );
    RSS(ref_import_by_extension( &import_grid, ref_mpi, file ), "import" );
    REIS( ref_node_n(ref_grid_node(export_grid)),
	  ref_node_n(ref_grid_node(import_grid)), "node count" );
    RSS(ref_grid_free(import_grid),"free");
    RSS(ref_grid_free(export_grid),"free");
    REIS(0, remove( file ), "test clean up");
  }

  { /* export import .lb8.ugrid tet */
    REF_GRID export_grid, import_grid;
    char file[] = "ref_import_test.lb8.ugrid";
    RSS(ref_fixture_tet_grid( &export_grid, ref_mpi ), "set up tet" );
    RSS(ref_export_by_extension( export_grid, file ), "export" );
    RSS(ref_import_by_extension( &import_grid, ref_mpi, file ), "import" );
    REIS( ref_node_n(ref_grid_node(export_grid)),
	  ref_node_n(ref_grid_node(import_grid)), "node count" );
    REIS( ref_cell_n(ref_grid_tri(export_grid)),
	  ref_cell_n(ref_grid_tri(import_grid)), "tri count" );
    REIS( ref_cell_n(ref_grid_qua(export_grid)),
	  ref_cell_n(ref_grid_qua(import_grid)), "qua count" );
    REIS( ref_cell_n(ref_grid_tet(export_grid)),
	  ref_cell_n(ref_grid_tet(import_grid)), "tet count" );
    RWDS( ref_node_xyz( ref_grid_node(export_grid),0,1),
	  ref_node_xyz( ref_grid_node(import_grid),0,1), 1e-15, "x 1" );
    REIS( ref_cell_c2n(ref_grid_tet(export_grid),0,0),
	  ref_cell_c2n(ref_grid_tet(import_grid),0,0), "tet node0" );
    REIS( ref_cell_c2n(ref_grid_tet(export_grid),1,0),
	  ref_cell_c2n(ref_grid_tet(import_grid),1,0), "tet node 1" );
    RSS(ref_grid_free(import_grid),"free");
    RSS(ref_grid_free(export_grid),"free");
    REIS(0, remove( file ), "test clean up");
  }

  { /* export import .b8.ugrid tet */
    REF_GRID export_grid, import_grid;
    char file[] = "ref_import_test.b8.ugrid";
    RSS(ref_fixture_tet_grid( &export_grid, ref_mpi ), "set up tet" );
    RSS(ref_export_by_extension( export_grid, file ), "export" );
    RSS(ref_import_by_extension( &import_grid, ref_mpi, file ), "import" );
    REIS( ref_node_n(ref_grid_node(export_grid)),
	  ref_node_n(ref_grid_node(import_grid)), "node count" );
    REIS( ref_cell_n(ref_grid_tri(export_grid)),
	  ref_cell_n(ref_grid_tri(import_grid)), "tri count" );
    REIS( ref_cell_n(ref_grid_qua(export_grid)),
	  ref_cell_n(ref_grid_qua(import_grid)), "qua count" );
    REIS( ref_cell_n(ref_grid_tet(export_grid)),
	  ref_cell_n(ref_grid_tet(import_grid)), "tet count" );
    RWDS( ref_node_xyz( ref_grid_node(export_grid),0,1),
	  ref_node_xyz( ref_grid_node(import_grid),0,1), 1e-15, "x 1" );
    REIS( ref_cell_c2n(ref_grid_tet(export_grid),0,0),
	  ref_cell_c2n(ref_grid_tet(import_grid),0,0), "tet node0" );
    REIS( ref_cell_c2n(ref_grid_tet(export_grid),1,0),
	  ref_cell_c2n(ref_grid_tet(import_grid),1,0), "tet node 1" );
    RSS(ref_grid_free(import_grid),"free");
    RSS(ref_grid_free(export_grid),"free");
    REIS(0, remove( file ), "test clean up");
  }

  RSS( ref_mpi_free( ref_mpi ), "free" );
  return 0;
}
