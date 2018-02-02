
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
#include <math.h>

#include "ref_validation.h"

#include "ref_face.h"
#include "ref_export.h"
#include "ref_mpi.h"

REF_STATUS ref_validation_all( REF_GRID ref_grid )
{
  RSS( ref_validation_cell_face( ref_grid ), "cell face");
  RSS( ref_validation_cell_node( ref_grid ), "cell node valid");
  RSS( ref_validation_unused_node( ref_grid ), "unused node");

  return REF_SUCCESS;
}

REF_STATUS ref_validation_unused_node( REF_GRID ref_grid )
{
  REF_INT node;
  REF_BOOL problem;
  REF_ADJ ref_adj;
  REF_CELL ref_cell;
  REF_INT group, cell, nodes[REF_CELL_MAX_SIZE_PER];

  problem = REF_FALSE;
  each_ref_node_valid_node( ref_grid_node(ref_grid), node )
    {
      if ( ref_adj_empty( ref_cell_adj(ref_grid_tet(ref_grid)), node) &&
	   ref_adj_empty( ref_cell_adj(ref_grid_pyr(ref_grid)), node) &&
	   ref_adj_empty( ref_cell_adj(ref_grid_pri(ref_grid)), node) &&
	   ref_adj_empty( ref_cell_adj(ref_grid_hex(ref_grid)), node) )
	{
	  problem = REF_TRUE;
	  printf(" unused node %d: %e %e %e\n",
		 node,
		 ref_node_xyz(ref_grid_node(ref_grid),0,node),
		 ref_node_xyz(ref_grid_node(ref_grid),1,node),
		 ref_node_xyz(ref_grid_node(ref_grid),2,node));
	}
    }

  ref_adj_create( &ref_adj );

  each_ref_grid_ref_cell( ref_grid, group, ref_cell )
    {
      each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes )
	{
	  for ( node = 0; node < ref_cell_node_per(ref_cell); node++ )
	    RSS( ref_adj_add( ref_adj, nodes[node], group+4*cell ), "add");
	}
    }

  each_ref_node_valid_node( ref_grid_node(ref_grid), node )
    {
      if ( ref_adj_empty( ref_adj, node ) )
	{
	  problem = REF_TRUE;
	  printf(" unused node %d\n",node);	  
	}
    }

  ref_adj_free(ref_adj);

  return (problem?REF_FAILURE:REF_SUCCESS);
}

REF_STATUS ref_validation_boundary_face( REF_GRID ref_grid )
{
  REF_CELL ref_cell;
  REF_BOOL has_face;
  REF_INT cell;
  REF_INT node;
  REF_INT nodes[4];
  REF_BOOL problem;

  problem = REF_FALSE;
 
  ref_cell = ref_grid_tri( ref_grid );
  each_ref_cell_valid_cell( ref_cell, cell )
    {
      for(node=0;node<3;node++)
	nodes[node]=ref_cell_c2n(ref_cell,node,cell);
      nodes[3]=nodes[0];
      RSS( ref_grid_cell_has_face( ref_grid, nodes, &has_face ), "has_face");
      if ( !has_face ) 
	{
	  problem = REF_TRUE;
	  printf("triangle %d nodes %d %d %d global %d %d %d\n",
		 cell,
		 nodes[0], nodes[1], nodes[2],
		 ref_node_global(ref_grid_node(ref_grid),nodes[0]),
		 ref_node_global(ref_grid_node(ref_grid),nodes[1]),
		 ref_node_global(ref_grid_node(ref_grid),nodes[2]) );
	  RSS( ref_node_location( ref_grid_node(ref_grid), nodes[0] ), "n0");
	  RSS( ref_node_location( ref_grid_node(ref_grid), nodes[1] ), "n1");
	  RSS( ref_node_location( ref_grid_node(ref_grid), nodes[2] ), "n2");
	}
    }
 
  ref_cell = ref_grid_qua( ref_grid );
  each_ref_cell_valid_cell( ref_cell, cell )
    {
      for(node=0;node<4;node++)
	nodes[node]=ref_cell_c2n(ref_cell,node,cell);
      RSS( ref_grid_cell_has_face( ref_grid, nodes, &has_face ), "has_face");
      if ( !has_face ) problem = REF_TRUE;
    }
 
  return (problem?REF_FAILURE:REF_SUCCESS);
}

REF_STATUS ref_validation_cell_face( REF_GRID ref_grid )
{
  REF_FACE ref_face;
  REF_CELL ref_cell;
  REF_INT *hits;
  REF_INT face;
  REF_INT group, cell, cell_face;
  REF_INT node;
  REF_INT nodes[4];
  REF_BOOL problem;
  REF_STATUS code;

  problem = REF_FALSE;

  RSS( ref_face_create( &ref_face, ref_grid ), "face");

  hits = (REF_INT *)malloc( ref_face_n(ref_face) * sizeof(REF_INT) );
  RNS(hits,"malloc hits NULL");

  for ( face=0; face< ref_face_n(ref_face) ; face++ )
    hits[face]=0;

  each_ref_grid_ref_cell( ref_grid, group, ref_cell )
    each_ref_cell_valid_cell( ref_cell, cell )
      each_ref_cell_cell_face( ref_cell, cell_face )
        {
	  for(node=0;node<4;node++)
	    nodes[node]=ref_cell_f2n(ref_cell,node,cell_face,cell);
	  RSS( ref_face_with( ref_face, nodes, &face ), "find cell face");
	  hits[face]++;
	}
 
  ref_cell = ref_grid_tri( ref_grid );
  each_ref_cell_valid_cell( ref_cell, cell )
    {
      for(node=0;node<3;node++)
	nodes[node]=ref_cell_c2n(ref_cell,node,cell);
      nodes[3]=nodes[0];
      code = ref_face_with( ref_face, nodes, &face );
      if ( REF_SUCCESS != code)
	{
	  ref_node_location( ref_grid_node(ref_grid), nodes[0] );
	  ref_node_location( ref_grid_node(ref_grid), nodes[1] );
	  ref_node_location( ref_grid_node(ref_grid), nodes[2] );
	  ref_node_location( ref_grid_node(ref_grid), nodes[3] );
	}
      RSS( code, "find tri");
      hits[face]++;
    }
 
  ref_cell = ref_grid_qua( ref_grid );
  each_ref_cell_valid_cell( ref_cell, cell )
    {
      for(node=0;node<4;node++)
	nodes[node]=ref_cell_c2n(ref_cell,node,cell);
      RSS( ref_face_with( ref_face, nodes, &face ), "find qua");
      hits[face]++;
    }
 
  for ( face=0; face< ref_face_n(ref_face) ; face++ )
    if ( 2 != hits[face] )
      {
	problem = REF_TRUE;	
	printf(" hits %d\n",hits[face]);	  
	for(node=0;node<3;node++)
	  nodes[node]=ref_face_f2n(ref_face,node,face);
	printf("face %d nodes %d %d %d global %d %d %d\n",
	       face,
	       nodes[0], nodes[1], nodes[2],
	       ref_node_global(ref_grid_node(ref_grid),nodes[0]),
	       ref_node_global(ref_grid_node(ref_grid),nodes[1]),
	       ref_node_global(ref_grid_node(ref_grid),nodes[2]) );
	RSS( ref_node_location( ref_grid_node(ref_grid), nodes[0] ), "n0");
	RSS( ref_node_location( ref_grid_node(ref_grid), nodes[1] ), "n1");
	RSS( ref_node_location( ref_grid_node(ref_grid), nodes[2] ), "n2");
      }

  free(hits);

  RSS( ref_face_free( ref_face ), "face free");

  return (problem?REF_FAILURE:REF_SUCCESS);
}

REF_STATUS ref_validation_cell_node( REF_GRID ref_grid )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT group;
  REF_INT cell, node, nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL has_local;

  each_ref_grid_ref_cell( ref_grid, group, ref_cell )
    each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes )
      {
	has_local = REF_FALSE;
	for ( node=0; node<ref_cell_node_per(ref_cell); node++ )
	  {
	    if ( ! ref_node_valid(ref_grid_node(ref_grid),nodes[node]))
	      {
		RSS( REF_FAILURE, "cell with invalid node" );
	      }
	    has_local = has_local || 
	      ( ref_mpi_rank(ref_grid_mpi(ref_grid)) ==
		ref_node_part(ref_node,nodes[node]) );
	  }
	if ( !has_local )
	  {
	    RSS( REF_FAILURE, "cell with all ghost nodes" );
	  }
      }

  ref_cell = ref_grid_edg(ref_grid);
    each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes )
      {
	has_local = REF_FALSE;
	for ( node=0; node<ref_cell_node_per(ref_cell); node++ )
	  {
	    if ( ! ref_node_valid(ref_grid_node(ref_grid),nodes[node]))
	      {
		RSS( REF_FAILURE, "cell with invalid node" );
	      }
	    has_local = has_local || 
	      ( ref_mpi_rank(ref_grid_mpi(ref_grid)) ==
		ref_node_part(ref_node,nodes[node]) );
	  }
	if ( !has_local )
	  {
	    RSS( REF_FAILURE, "cell with all ghost nodes" );
	  }
      }

  ref_cell = ref_grid_tri(ref_grid);
    each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes )
      {
	has_local = REF_FALSE;
	for ( node=0; node<ref_cell_node_per(ref_cell); node++ )
	  {
	    if ( ! ref_node_valid(ref_grid_node(ref_grid),nodes[node]))
	      {
		RSS( REF_FAILURE, "cell with invalid node" );
	      }
	    has_local = has_local || 
	      ( ref_mpi_rank(ref_grid_mpi(ref_grid)) ==
		ref_node_part(ref_node,nodes[node]) );
	  }
	if ( !has_local )
	  {
	    RSS( REF_FAILURE, "cell with all ghost nodes" );
	  }
      }

  ref_cell = ref_grid_qua(ref_grid);
    each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes )
      {
	has_local = REF_FALSE;
	for ( node=0; node<ref_cell_node_per(ref_cell); node++ )
	  {
	    if ( ! ref_node_valid(ref_grid_node(ref_grid),nodes[node]))
	      {
		RSS( REF_FAILURE, "cell with invalid node" );
	      }
	    has_local = has_local || 
	      ( ref_mpi_rank(ref_grid_mpi(ref_grid)) ==
		ref_node_part(ref_node,nodes[node]) );
	  }
	if ( !has_local )
	  {
	    RSS( REF_FAILURE, "cell with all ghost nodes" );
	  }
      }

  return REF_SUCCESS;
}

REF_STATUS ref_validation_cell_volume( REF_GRID ref_grid )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_DBL volume;
  REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];

  ref_cell = ref_grid_tet(ref_grid);
  if (ref_grid_twod(ref_grid) ) ref_cell = ref_grid_tri(ref_grid);

  each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes )
    {
      if ( ref_grid_twod(ref_grid) )
	{
	  RSS( ref_node_tri_area( ref_node, nodes, &volume ), "area" );
	}
      else
	{
	  RSS( ref_node_tet_vol( ref_node, nodes, &volume ), "vol" );
	}
      RAS ( volume>0.0, "negative volume tet");
    }

  return REF_SUCCESS;
}
