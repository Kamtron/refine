#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ref_grid.h"
#include "ref_cell.h"
#include "ref_node.h"
#include "ref_list.h"
#include "ref_adj.h"
#include "ref_matrix.h"

#include "ref_sort.h"

#include "ref_shard.h"
#include "ref_fixture.h"
#include "ref_quality.h"
#include "ref_subdiv.h"
#include "ref_swap.h"
#include "ref_math.h"
#include "ref_export.h"
#include "ref_edge.h"
#include "ref_dict.h"
#include "ref_part.h"
#include  "ref_migrate.h"
#include  "ref_twod.h"
#include "ref_gather.h"
#include "ref_mpi.h"

#include "ref_import.h"

static REF_STATUS set_up_hex_for_shard( REF_SHARD *ref_shard_ptr )
{
  REF_GRID ref_grid;

  RSS( ref_fixture_hex_grid( &ref_grid ), "fixure hex" );

  RSS(ref_shard_create(ref_shard_ptr,ref_grid),"create");

  return REF_SUCCESS;
}

static REF_STATUS tear_down( REF_SHARD ref_shard )
{
  REF_GRID ref_grid;

  ref_grid = ref_shard_grid(ref_shard);

  RSS(ref_shard_free(ref_shard),"free");

  RSS( ref_grid_free(ref_grid),"free" );

  return REF_SUCCESS;
}

int main( int argc, char *argv[] )
{
  REF_INT nhex;

  RSS( ref_mpi_start( argc, argv ), "start" );

  if (3 == argc) 
    {
      REF_GRID ref_grid;
      char file[] = "ref_shard_test.b8.ugrid";
      ref_mpi_stopwatch_start();
      if ( ref_mpi_n > 1 ){
	RSS( ref_part_b8_ugrid( &ref_grid, argv[1] ), "import" );
      }else{
	RSS( ref_import_by_extension( &ref_grid, argv[1] ), "import" );
      }
      ref_mpi_stopwatch_stop("import");
      RSS( ref_gather_ncell( ref_grid_node(ref_grid), ref_grid_hex(ref_grid), 
			     &nhex ), "nhex");
      ref_mpi_stopwatch_stop("nhex");
      if ( 0 < nhex )
	{
	  REF_SHARD ref_shard;
	  REF_CELL ref_cell;
	  REF_NODE ref_node = ref_grid_node(ref_grid);
	  REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];
	  REF_INT faceid;
	  REF_INT open_node0, open_node1;
	  REF_INT face_marks, hex_marks;

	  if ( ref_mpi_n > 1 ){
	    RSS( ref_mpi_stop( ), "stop" );
	    THROW("hex shard not parallel");
	  }

	  RSS(ref_shard_create(&ref_shard,ref_grid),"create");
	  ref_cell = ref_grid_qua(ref_grid);
	  each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes)
	    {
	      faceid = nodes[4];
	      if ( atoi(argv[2]) == faceid )
		{
		  RSS( ref_face_open_node( &ref_node_xyz(ref_node,0,nodes[0]),
					   &ref_node_xyz(ref_node,0,nodes[1]),
					   &ref_node_xyz(ref_node,0,nodes[2]),
					   &ref_node_xyz(ref_node,0,nodes[3]),
					   &open_node0 ), "find open" );
		  open_node1 = open_node0+2;
		  if ( open_node1 >= 4 ) open_node1 -= 4;
		  RSS( ref_shard_mark_to_split( ref_shard, 
						nodes[open_node0], 
						nodes[open_node1]), 
		       "mark");
		}
	    }
	  RSS( ref_shard_mark_n( ref_shard, 
				 &face_marks, &hex_marks ), "count marks");
	  printf("marked faces %d hexes %d\n",face_marks,hex_marks);
	  RSS( ref_shard_mark_relax( ref_shard ), "relax" );
	  RSS( ref_shard_mark_n( ref_shard, 
				 &face_marks, &hex_marks ), "count marks");
	  printf("relaxed faces %d hexes %d\n",face_marks,hex_marks);
	  RSS( ref_shard_split( ref_shard ), "split hex to prism" );
	  RSS(ref_shard_free(ref_shard),"free");
	}
      else
	{
	  RSS( ref_shard_prism_into_tet( ref_grid, atoi(argv[2]), REF_EMPTY ), 
	       "shrd");
	  ref_mpi_stopwatch_stop("shard");
	}
      if ( ref_mpi_n > 1 ){
	RSS( ref_gather_b8_ugrid( ref_grid, file ),"export" );
      }else{
	RSS( ref_export_by_extension( ref_grid, file ),"export" );
      }
      ref_mpi_stopwatch_stop("export");
      RSS( ref_grid_free(ref_grid),"free");
      RSS( ref_mpi_stop( ), "stop" );
      return 0;
    }

  { /* mark */
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard),"set up");

    REIS(0,ref_shard_mark(ref_shard,1),"init mark");
    REIS(0,ref_shard_mark(ref_shard,3),"init mark");

    RSS(ref_shard_mark_to_split(ref_shard,1,6),"mark face for 1-6");

    REIS(2,ref_shard_mark(ref_shard,1),"split");
    REIS(0,ref_shard_mark(ref_shard,3),"modified");

    RSS( tear_down( ref_shard ), "tear down");
  }

  { /* find mark */
    REF_SHARD ref_shard;
    REF_BOOL marked;

    RSS(set_up_hex_for_shard(&ref_shard),"set up");

    RSS(ref_shard_mark_to_split(ref_shard,1,6),"mark face for 1-6");

    RSS(ref_shard_marked(ref_shard,2,5,&marked),"is edge marked?");
    RAS(!marked,"pair 2-5 not marked");

    RSS(ref_shard_marked(ref_shard,1,6,&marked),"is edge marked?");
    RAS( marked,"pair 1-6 marked");

    RSS(ref_shard_marked(ref_shard,6,1,&marked),"is edge marked?");
    RAS( marked,"pair 6-1 marked");

    RSS( tear_down( ref_shard ), "tear down");
  }

  { /* mark and relax 1-6 */
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard),"set up");

    RSS(ref_shard_mark_to_split(ref_shard,1,6),"mark face for 1-6");

    RES(0,ref_shard_mark(ref_shard,3),"no yet");
    RSS(ref_shard_mark_relax(ref_shard),"relax");
    RES(2,ref_shard_mark(ref_shard,3),"yet");

    RSS( tear_down( ref_shard ), "tear down");
  }

  { /* mark and relax 0-7 */
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard),"set up");

    RSS(ref_shard_mark_to_split(ref_shard,0,7),"mark face for 1-6");

    RES(0,ref_shard_mark(ref_shard,1),"no yet");
    RSS(ref_shard_mark_relax(ref_shard),"relax");
    RES(2,ref_shard_mark(ref_shard,1),"yet");

    RSS( tear_down( ref_shard ), "tear down");
  }

  { /* relax nothing*/
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard),"set up");

    RSS(ref_shard_mark_relax(ref_shard),"relax");

    RES(0,ref_shard_mark(ref_shard,0),"marked");
    RES(0,ref_shard_mark(ref_shard,1),"marked");
    RES(0,ref_shard_mark(ref_shard,2),"marked");
    RES(0,ref_shard_mark(ref_shard,3),"marked");
    RES(0,ref_shard_mark(ref_shard,4),"marked");
    RES(0,ref_shard_mark(ref_shard,5),"marked");

    RSS( tear_down( ref_shard ), "tear down");
  }

  { /* mark cell edge 0 */
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard),"set up");

    RSS(ref_shard_mark_cell_edge_split(ref_shard,0,0),"mark edge 0");

    RES(2,ref_shard_mark(ref_shard,1),"face 1");
    RES(2,ref_shard_mark(ref_shard,3),"face 3");

    RSS( tear_down( ref_shard ), "tear down");
  }

  { /* mark cell edge 11 */
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard),"set up");

    RSS(ref_shard_mark_cell_edge_split(ref_shard,0,11),"mark edge 11");

    RES(2,ref_shard_mark(ref_shard,1),"face 1");
    RES(2,ref_shard_mark(ref_shard,3),"face 3");

    RSS( tear_down( ref_shard ), "tear down");
  }

  { /* mark cell edge 5 */
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard),"set up");

    RSS(ref_shard_mark_cell_edge_split(ref_shard,0,5),"mark edge 5");

    RES(3,ref_shard_mark(ref_shard,1),"face 1");
    RES(3,ref_shard_mark(ref_shard,3),"face 3");

    RSS( tear_down( ref_shard ), "tear down");
  }

  { /* mark cell edge 8 */
    REF_SHARD ref_shard;
    RSS(set_up_hex_for_shard(&ref_shard),"set up");

    RSS(ref_shard_mark_cell_edge_split(ref_shard,0,8),"mark edge 8");

    RES(3,ref_shard_mark(ref_shard,1),"face 1");
    RES(3,ref_shard_mark(ref_shard,3),"face 3");

    RSS( tear_down( ref_shard ), "tear down");
  }

  { /* split on edge 0 */
    REF_SHARD ref_shard;
    REF_GRID ref_grid;
    RSS(set_up_hex_for_shard(&ref_shard),"set up");
    ref_grid = ref_shard_grid(ref_shard);

    RSS(ref_shard_mark_cell_edge_split(ref_shard,0,0),"mark edge 0");

    RSS(ref_shard_mark_relax(ref_shard),"relax");
    RSS(ref_shard_split(ref_shard),"split");

    REIS(0, ref_cell_n(ref_grid_hex(ref_grid)),"remove hex");
    REIS(2, ref_cell_n(ref_grid_pri(ref_grid)),"add two pri");

    REIS(4, ref_cell_n(ref_grid_tri(ref_grid)),"add four tri");
    REIS(4, ref_cell_n(ref_grid_qua(ref_grid)),"keep four qua");

    RSS( tear_down( ref_shard ), "tear down");
  }

  { /* split on edge 8 */
    REF_SHARD ref_shard;
    REF_GRID ref_grid;
    RSS(set_up_hex_for_shard(&ref_shard),"set up");
    ref_grid = ref_shard_grid(ref_shard);

    RSS(ref_shard_mark_cell_edge_split(ref_shard,0,8),"mark edge 0");

    RSS(ref_shard_mark_relax(ref_shard),"relax");
    RSS(ref_shard_split(ref_shard),"split");

    REIS(0, ref_cell_n(ref_grid_hex(ref_grid)),"remove hex");
    REIS(2, ref_cell_n(ref_grid_pri(ref_grid)),"add two pri");

    REIS(4, ref_cell_n(ref_grid_tri(ref_grid)),"add two pri");
    REIS(4, ref_cell_n(ref_grid_qua(ref_grid)),"keep one qua");

    RSS( tear_down( ref_shard ), "tear down");
  }

  { /* shard prism */

    REF_GRID ref_grid;

    RSS(ref_fixture_pri_grid(&ref_grid),"set up");
    RSS(ref_cell_remove(ref_grid_tri(ref_grid),0),"one tri");

    RSS(ref_shard_prism_into_tet(ref_grid,0,0),"shard prism");

    REIS(0, ref_cell_n(ref_grid_pri(ref_grid)),"no more pri");
    REIS(3, ref_cell_n(ref_grid_tet(ref_grid)),"into 3 tets");

    REIS(0, ref_cell_n(ref_grid_qua(ref_grid)),"no more qua");
    REIS(3, ref_cell_n(ref_grid_tri(ref_grid)),"into 9 tri");

    RSS( ref_grid_free(ref_grid),"free" );
  }

  { /* shard prism keeping one layer */

    REF_GRID ref_grid;

    RSS(ref_fixture_pri_grid(&ref_grid),"set up");
    RSS(ref_cell_remove(ref_grid_tri(ref_grid),0),"one tri");

    RSS(ref_shard_prism_into_tet(ref_grid,1,101),"shard prism");

    REIS(1, ref_cell_n(ref_grid_pri(ref_grid)),"no more pri");
    REIS(0, ref_cell_n(ref_grid_tet(ref_grid)),"into 3 tets");

    REIS(1, ref_cell_n(ref_grid_qua(ref_grid)),"no more qua");
    REIS(1, ref_cell_n(ref_grid_tri(ref_grid)),"into 9 tri");

    RSS( ref_grid_free(ref_grid),"free" );
  }

  { /* shard half stack */

    REF_GRID ref_grid;
    REF_INT cell, nodes[] = {0,1,2,15};
    RSS(ref_fixture_pri_stack_grid(&ref_grid),"set up");
    RSS(ref_cell_add(ref_grid_tri(ref_grid),nodes,&cell),"add one tri");

    RSS(ref_shard_prism_into_tet(ref_grid,2,15),"shard prism");

    REIS(2, ref_cell_n(ref_grid_pri(ref_grid)),"no more pri");
    REIS(3, ref_cell_n(ref_grid_tet(ref_grid)),"into 3 tets");

    REIS(2, ref_cell_n(ref_grid_qua(ref_grid)),"no more qua");
    REIS(3, ref_cell_n(ref_grid_tri(ref_grid)),"into 9 tri");

    RSS( ref_grid_free(ref_grid),"free" );
  }

  { /* shard pyr */

    REF_GRID ref_grid;

    RSS(ref_fixture_pyr_grid(&ref_grid),"set up");

    RSS(ref_shard_prism_into_tet(ref_grid,0,0),"shard prism");

    REIS(0, ref_cell_n(ref_grid_pyr(ref_grid)),"no more pri");
    REIS(2, ref_cell_n(ref_grid_tet(ref_grid)),"into 3 tets");

    REIS(0, ref_cell_n(ref_grid_qua(ref_grid)),"no more qua");
    REIS(6, ref_cell_n(ref_grid_tri(ref_grid)),"into 9 tri");

    RSS( ref_grid_free(ref_grid),"free" );
  }

  RSS( ref_mpi_stop( ), "stop" );

  return 0;
}
