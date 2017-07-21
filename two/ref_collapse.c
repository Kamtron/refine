
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "ref_collapse.h"
#include "ref_cell.h"
#include "ref_edge.h"
#include "ref_mpi.h"
#include "ref_sort.h"
#include "ref_malloc.h"
#include "ref_math.h"

#include "ref_adapt.h"

#include "ref_gather.h"
#include "ref_twod.h"

#define MAX_CELL_COLLAPSE (100)
#define MAX_NODE_LIST (1000)

REF_STATUS ref_collapse_pass( REF_GRID ref_grid )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_EDGE ref_edge;
  REF_DBL *ratio;
  REF_INT *order;
  REF_INT ntarget, *target, *node2target;
  REF_INT node, node0, node1;
  REF_INT i, edge;
  REF_INT item, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL edge_ratio;

  RSS( ref_edge_create( &ref_edge, ref_grid ), "orig edges" );

  ref_malloc_init( ratio, ref_node_max(ref_node), 
		   REF_DBL, 2.0*ref_adapt_collapse_ratio );

  for(edge=0;edge<ref_edge_n(ref_edge);edge++)
    {
      node0 = ref_edge_e2n( ref_edge, 0, edge );
      node1 = ref_edge_e2n( ref_edge, 1, edge );
      RSS( ref_node_ratio( ref_node, node0, node1, &edge_ratio ), "ratio");
      ratio[node0] = MIN( ratio[node0], edge_ratio );
      ratio[node1] = MIN( ratio[node1], edge_ratio );
    }

  ref_malloc( target, ref_node_n(ref_node), REF_INT );
  ref_malloc_init( node2target, ref_node_max(ref_node), REF_INT, REF_EMPTY );

  ntarget=0;
  for ( node=0 ; node < ref_node_max(ref_node) ; node++ )
    if ( ratio[node] < ref_adapt_collapse_ratio )
      {
	node2target[node] = ntarget;
	target[ntarget] = node;
	ratio[ntarget] = ratio[node];
	ntarget++;
      }

  ref_malloc( order, ntarget, REF_INT );

  RSS( ref_sort_heap_dbl( ntarget, ratio, order), "sort lengths" );

  for ( i = 0; i < ntarget; i++ )
    {
      if ( ratio[order[i]] > ref_adapt_collapse_ratio ) continue; 
      node1 = target[order[i]];
      RSS( ref_collapse_to_remove_node1( ref_grid, &node0, node1 ), 
	   "collapse rm" );
      if ( !ref_node_valid(ref_node,node1) )
	{
	  ref_node_age(ref_node,node0) = 0;
	  each_ref_cell_having_node( ref_cell, node0, item, cell )
	    {
	      RSS( ref_cell_nodes( ref_cell, cell, nodes), "cell nodes");
	      for (node=0;node<ref_cell_node_per(ref_cell);node++)
		if ( REF_EMPTY != node2target[nodes[node]] )
		  ratio[node2target[nodes[node]]] = 
		    2.0*ref_adapt_collapse_ratio;
	    }
	}
    }
  
  ref_free( order );
  ref_free( node2target );
  ref_free( target );
  ref_free( ratio );

  ref_edge_free( ref_edge );

  return REF_SUCCESS;
}

REF_STATUS ref_collapse_to_remove_node1( REF_GRID ref_grid, 
					 REF_INT *actual_node0, REF_INT node1 )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tet(ref_grid);
  REF_INT nnode, node;
  REF_INT node_to_collapse[MAX_NODE_LIST];
  REF_INT order[MAX_NODE_LIST];
  REF_DBL ratio_to_collapse[MAX_NODE_LIST];
  REF_INT node0;
  REF_BOOL allowed, have_geometry_support;

  *actual_node0 = REF_EMPTY;

  RSS( ref_cell_node_list_around( ref_cell, node1, MAX_NODE_LIST,
				  &nnode, node_to_collapse ), "da hood");
  for ( node=0 ; node < nnode ; node++ )
    RSS( ref_node_ratio( ref_node, node_to_collapse[node], node1, 
			 &(ratio_to_collapse[node]) ), "ratio");

  RSS( ref_sort_heap_dbl( nnode, ratio_to_collapse, order), "sort lengths" );

  for ( node=0 ; node < nnode ; node++ )
    {
      node0 = node_to_collapse[order[node]];
  
      RSS(ref_collapse_edge_mixed(ref_grid,node0,node1,&allowed),"col mixed");
      if ( !allowed ) continue;

      RSS(ref_collapse_edge_geometry(ref_grid,node0,node1,&allowed),"col geom");
      if ( !allowed ) continue;

      RSS(ref_geom_supported(ref_grid_geom(ref_grid),node0,
			     &have_geometry_support),"geom");
      if ( have_geometry_support )
	{
	  RSS(ref_collapse_edge_cad_constrained(ref_grid,node0,node1,&allowed),
	      "cad constrained");
	  if ( !allowed ) continue;
	}
      else
	{
	  RSS(ref_collapse_edge_same_normal(ref_grid,node0,node1,&allowed),
	      "normal deviation");
	  if ( !allowed ) continue;
	}

      RSS(ref_collapse_edge_quality(ref_grid,node0,node1,&allowed),"qual");
      if ( !allowed ) continue;

      RSS(ref_collapse_edge_local_tets(ref_grid,node0,node1,&allowed),"colloc");
      if ( !allowed ) 
	{
	  ref_node_age(ref_node,node0)++;
	  ref_node_age(ref_node,node1)++;
	  continue;
	}

      *actual_node0 = node0;
      RSS( ref_collapse_edge( ref_grid, node0, node1 ), "col!");

      break;

    }

  return REF_SUCCESS;
}


REF_STATUS ref_collapse_edge( REF_GRID ref_grid, 
			      REF_INT node0, REF_INT node1 )
/*                               keep node0,  remove node1 */
{
  REF_CELL ref_cell;
  REF_INT cell;
  REF_INT ncell, cell_in_list;
  REF_INT cell_to_collapse[MAX_CELL_COLLAPSE];

  ref_cell = ref_grid_tet(ref_grid);
  RSS( ref_cell_list_with2(ref_cell,node0,node1,
			   MAX_CELL_COLLAPSE, &ncell, cell_to_collapse ),"lst");

  for ( cell_in_list = 0; cell_in_list < ncell ; cell_in_list++ )
    {
      cell = cell_to_collapse[cell_in_list];
      RSS( ref_cell_remove( ref_cell, cell ), "remove" );
    }
  RSS( ref_cell_replace_node( ref_cell, node1, node0 ), "replace node" );

  ref_cell = ref_grid_tri(ref_grid);
  RSS( ref_cell_list_with2(ref_cell,node0,node1,
			   MAX_CELL_COLLAPSE, &ncell, cell_to_collapse ),"lst");

  for ( cell_in_list = 0; cell_in_list < ncell ; cell_in_list++ )
    {
      cell = cell_to_collapse[cell_in_list];
      RSS( ref_cell_remove( ref_cell, cell ), "remove" );
    }
  RSS( ref_cell_replace_node( ref_cell, node1, node0 ), "replace node" );

  ref_cell = ref_grid_edg(ref_grid);
  RSS( ref_cell_list_with2(ref_cell,node0,node1,
			   MAX_CELL_COLLAPSE, &ncell, cell_to_collapse ),"lst");

  for ( cell_in_list = 0; cell_in_list < ncell ; cell_in_list++ )
    {
      cell = cell_to_collapse[cell_in_list];
      RSS( ref_cell_remove( ref_cell, cell ), "remove" );
    }
  RSS( ref_cell_replace_node( ref_cell, node1, node0 ), "replace node" );

  RSS( ref_node_remove(ref_grid_node(ref_grid),node1), "rm");
  RSS( ref_geom_remove_all(ref_grid_geom(ref_grid),node1), "rm");
  
  return REF_SUCCESS;
}

REF_STATUS ref_collapse_edge_geometry( REF_GRID ref_grid, 
				       REF_INT node0, REF_INT node1,
				       REF_BOOL *allowed )
{
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT item, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT deg, degree1;
  REF_INT id, ids1[3]; 
  REF_BOOL already_have_it;

  REF_INT ncell;
  REF_INT cell_to_collapse[MAX_CELL_COLLAPSE];
  REF_INT id0, id1;

  REF_BOOL geom_node1;

  *allowed = REF_FALSE;

  /* ids1 is a list of degree1 face ids for node1 */
  degree1 = 0;
  each_ref_cell_having_node( ref_cell, node1, item, cell )
    {
      RSS( ref_cell_nodes( ref_cell, cell, nodes ), "nodes" );
      id = nodes[3];
      already_have_it = REF_FALSE;
      for (deg=0;deg<degree1;deg++)
	if ( id == ids1[deg] ) already_have_it = REF_TRUE;
      if ( !already_have_it )
	{
	  ids1[degree1] = id;
	  degree1++;
	  if ( 3 == degree1 ) break;
	}
    }

  RSS( ref_geom_is_a( ref_grid_geom(ref_grid),
		      node1, REF_GEOM_NODE, &geom_node1), "node check");
  if ( geom_node1 )
    {
      *allowed = REF_FALSE;
      return REF_SUCCESS;
    }

  switch ( degree1 )
    {
    case 3: /* geometry node never allowed to move */
      *allowed = REF_FALSE;
      break;
    case 2: /* geometery edge allowed if collapse is on edge */
      RSS( ref_cell_list_with2(ref_cell,node0,node1,
			       MAX_CELL_COLLAPSE, &ncell, 
			       cell_to_collapse ),"list");
      if ( 2 != ncell ) {
	*allowed = REF_FALSE;
	break;
      }
      RSS( ref_cell_nodes( ref_cell, cell_to_collapse[0], nodes ), "nodes" );
      id0 = nodes[3];
      RSS( ref_cell_nodes( ref_cell, cell_to_collapse[1], nodes ), "nodes" );
      id1 = nodes[3];
      if ( ( id0 == ids1[0] && id1 == ids1[1] ) ||
	   ( id1 == ids1[0] && id0 == ids1[1] ) ) *allowed = REF_TRUE;
      break;
    case 1: /* geometry face allowed if on that face */
      RSS( ref_cell_has_side( ref_cell, node0, node1, allowed ),
	   "allowed if a side of a triangle" );
      break;
    case 0: /* volume node always allowed */
      *allowed = REF_TRUE;
      break;
    }

  return REF_SUCCESS;
}

REF_STATUS ref_collapse_edge_same_normal( REF_GRID ref_grid, 
					  REF_INT node0, REF_INT node1,
					  REF_BOOL *allowed )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT item, cell, node;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL n0[3], n1[3];
  REF_DBL dot;
  REF_STATUS status;

  *allowed = REF_TRUE;

  each_ref_cell_having_node( ref_cell, node1, item, cell )
    {
      /* a triangle with node0 and node1 will be removed */
      if ( node0 == ref_cell_c2n(ref_cell,0,cell) ||
	   node0 == ref_cell_c2n(ref_cell,1,cell) ||
	   node0 == ref_cell_c2n(ref_cell,2,cell) ) continue;
      RSS( ref_cell_nodes( ref_cell, cell, nodes ), "nodes" );
      RSS( ref_node_tri_normal( ref_node, nodes, n0 ), "orig normal" );
      RSS( ref_math_normalize( n0 ), "original triangle has zero area" );
      for ( node = 0; node < ref_cell_node_per(ref_cell) ; node++ )
	if ( node1 == nodes[node] ) nodes[node] = node0;
      RSS( ref_node_tri_normal( ref_node, nodes, n1 ), "new normal" );
      status = ref_math_normalize( n1 );
      if ( REF_DIV_ZERO == status )
	{ /* new triangle face has zero area */
	  *allowed = REF_FALSE;
	  return REF_SUCCESS;	  
	}
      RSS( status, "new normal length" )
      dot = ref_math_dot( n0, n1 );
      if ( dot < (1.0-1.0e-8) ) /* acos(1.0-1.0e-8) ~ 0.0001 radian, 0.01 deg */
	{
	  *allowed = REF_FALSE;
	  return REF_SUCCESS;	  
	}
    }
       
  return REF_SUCCESS;
}

REF_STATUS ref_collapse_edge_mixed( REF_GRID ref_grid, 
				    REF_INT node0, REF_INT node1,
				    REF_BOOL *allowed )
{
  SUPRESS_UNUSED_COMPILER_WARNING(node0);

  *allowed = ( ref_adj_empty( ref_cell_adj(ref_grid_pyr(ref_grid)),
			      node1 ) &&
	       ref_adj_empty( ref_cell_adj(ref_grid_pri(ref_grid)),
			      node1 ) &&
	       ref_adj_empty( ref_cell_adj(ref_grid_hex(ref_grid)),
			      node1 ) );

  return REF_SUCCESS;
}

REF_STATUS ref_collapse_edge_local_tets( REF_GRID ref_grid, 
					 REF_INT node0, REF_INT node1,
					 REF_BOOL *allowed )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT item, cell, node;

  *allowed =  REF_FALSE;

  ref_cell = ref_grid_tet(ref_grid);

  each_ref_cell_having_node( ref_cell, node1, item, cell )
    for ( node = 0 ; node < ref_cell_node_per(ref_cell); node++ )
      if ( ref_mpi_id != ref_node_part(ref_node,
				       ref_cell_c2n(ref_cell,node,cell)) )
	return REF_SUCCESS;

  /* may be able to relax node0 local if geom constraint is o.k. */
  each_ref_cell_having_node( ref_cell, node0, item, cell )
    for ( node = 0 ; node < ref_cell_node_per(ref_cell); node++ )
      if ( ref_mpi_id != ref_node_part(ref_node,
				       ref_cell_c2n(ref_cell,node,cell)) )
	return REF_SUCCESS;



  *allowed =  REF_TRUE;

  return REF_SUCCESS;
}

/* triangle can not be fully constrained by CAD edges after a collapse */
REF_STATUS ref_collapse_edge_cad_constrained( REF_GRID ref_grid, 
					      REF_INT node0, REF_INT node1,
					      REF_BOOL *allowed )
{
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_INT item, cell, node, nodes[REF_CELL_MAX_SIZE_PER];
  REF_BOOL will_be_collapsed;
  REF_BOOL edge0, edge1, edge2;
  
  *allowed = REF_TRUE;
  
  each_ref_cell_having_node( ref_cell, node1, item, cell )
    {
      RSS( ref_cell_nodes( ref_cell, cell, nodes ), "nodes" );

      will_be_collapsed = REF_FALSE;
      for ( node = 0; node < ref_cell_node_per(ref_cell) ; node++ )
	if ( node0 == nodes[node] ) will_be_collapsed = REF_TRUE;
      if ( will_be_collapsed ) continue;

      for ( node = 0; node < ref_cell_node_per(ref_cell) ; node++ )
	if ( node1 == nodes[node] ) nodes[node] = node0;
      RSS( ref_geom_is_a(ref_geom, nodes[0], REF_GEOM_EDGE, &edge0 ), "e0");
      RSS( ref_geom_is_a(ref_geom, nodes[1], REF_GEOM_EDGE, &edge1 ), "e1");
      RSS( ref_geom_is_a(ref_geom, nodes[2], REF_GEOM_EDGE, &edge2 ), "e2");
      if ( edge0 && edge1 && edge2 )
	{
	  *allowed = REF_FALSE;
	  break;
	}
    }

 return REF_SUCCESS;
}

REF_STATUS ref_collapse_edge_quality( REF_GRID ref_grid, 
				      REF_INT node0, REF_INT node1,
				      REF_BOOL *allowed )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT item, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT node;
  REF_DBL quality;
  REF_BOOL will_be_collapsed;
  REF_DBL edge_ratio;

  *allowed = REF_FALSE;

  ref_cell = ref_grid_tet(ref_grid);
  each_ref_cell_having_node( ref_cell, node1, item, cell )
    {
      RSS( ref_cell_nodes( ref_cell, cell, nodes ), "nodes" );

      will_be_collapsed = REF_FALSE;
      for ( node = 0; node < ref_cell_node_per(ref_cell) ; node++ )
	if ( node0 == nodes[node] ) will_be_collapsed = REF_TRUE;
      if ( will_be_collapsed ) continue;

      for ( node = 0; node < ref_cell_node_per(ref_cell) ; node++ )
	if ( node1 != nodes[node] )
	  {
	    RSS( ref_node_ratio( ref_node, node0, nodes[node], 
				 &edge_ratio ), "ratio");
	    if ( edge_ratio > ref_adapt_collapse_ratio_limit )
	      return REF_SUCCESS;
	  }

      for ( node = 0; node < ref_cell_node_per(ref_cell) ; node++ )
	if ( node1 == nodes[node] ) nodes[node] = node0;
      RSS( ref_node_tet_quality( ref_node,nodes,&quality ), "qual");
      if ( quality < ref_adapt_collapse_quality_absolute ) return REF_SUCCESS;
    }

  /* FIXME check tris too */

  *allowed = REF_TRUE;

  return REF_SUCCESS;
}

REF_STATUS ref_collapse_face( REF_GRID ref_grid,
			      REF_INT keep0, REF_INT remove0,
			      REF_INT keep1, REF_INT remove1)
{
  REF_CELL ref_cell;
  REF_INT cell;
  REF_INT ncell, cell_in_list;
  REF_INT cell_to_collapse[MAX_CELL_COLLAPSE];

  ref_cell = ref_grid_pri(ref_grid);
  RSS( ref_cell_list_with2(ref_cell,keep0,remove0,
			   MAX_CELL_COLLAPSE, &ncell, cell_to_collapse ),"lst");

  for ( cell_in_list = 0; cell_in_list < ncell ; cell_in_list++ )
    {
      cell = cell_to_collapse[cell_in_list];
      RSS( ref_cell_remove( ref_cell, cell ), "remove" );
    }
  RSS( ref_cell_replace_node( ref_cell, remove0, keep0 ), "replace node0" );
  RSS( ref_cell_replace_node( ref_cell, remove1, keep1 ), "replace node1" );

  ref_cell = ref_grid_qua(ref_grid);
  RSS( ref_cell_list_with2(ref_cell,keep0,remove0,
			   MAX_CELL_COLLAPSE, &ncell, cell_to_collapse ),"lst");

  for ( cell_in_list = 0; cell_in_list < ncell ; cell_in_list++ )
    {
      cell = cell_to_collapse[cell_in_list];
      RSS( ref_cell_remove( ref_cell, cell ), "remove" );
    }
  RSS( ref_cell_replace_node( ref_cell, remove0, keep0 ), "replace node0" );
  RSS( ref_cell_replace_node( ref_cell, remove1, keep1 ), "replace node1" );

  ref_cell = ref_grid_tri(ref_grid);

  RSS( ref_cell_list_with2(ref_cell,keep0,remove0,
			   MAX_CELL_COLLAPSE, &ncell, cell_to_collapse ),"lst");
  for ( cell_in_list = 0; cell_in_list < ncell ; cell_in_list++ )
    {
      cell = cell_to_collapse[cell_in_list];
      RSS( ref_cell_remove( ref_cell, cell ), "remove" );
    }
  RSS( ref_cell_replace_node( ref_cell, remove0, keep0 ), "replace node" );

  RSS( ref_cell_list_with2(ref_cell,keep1,remove1,
			   MAX_CELL_COLLAPSE, &ncell, cell_to_collapse ),"lst");
  for ( cell_in_list = 0; cell_in_list < ncell ; cell_in_list++ )
    {
      cell = cell_to_collapse[cell_in_list];
      RSS( ref_cell_remove( ref_cell, cell ), "remove" );
    }
  RSS( ref_cell_replace_node( ref_cell, remove1, keep1 ), "replace node" );

  RSS( ref_node_remove(ref_grid_node(ref_grid),remove0), "rm");
  RSS( ref_node_remove(ref_grid_node(ref_grid),remove1), "rm");
  RSS( ref_geom_remove_all(ref_grid_geom(ref_grid),remove0), "rm");
  RSS( ref_geom_remove_all(ref_grid_geom(ref_grid),remove1), "rm");

  return REF_SUCCESS;
}

REF_STATUS ref_collapse_face_local_pris( REF_GRID ref_grid, 
					 REF_INT keep, REF_INT remove,
					 REF_BOOL *allowed )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT item, cell, node;

  *allowed =  REF_FALSE;

  ref_cell = ref_grid_pri(ref_grid);

  each_ref_cell_having_node( ref_cell, remove, item, cell )
    for ( node = 0 ; node < ref_cell_node_per(ref_cell); node++ )
      if ( ref_mpi_id != ref_node_part(ref_node,
				       ref_cell_c2n(ref_cell,node,cell)) )
	return REF_SUCCESS;

  /* may be able to relax keep local if geom constraint is o.k. */
  each_ref_cell_having_node( ref_cell, keep, item, cell )
    for ( node = 0 ; node < ref_cell_node_per(ref_cell); node++ )
      if ( ref_mpi_id != ref_node_part(ref_node,
				       ref_cell_c2n(ref_cell,node,cell)) )
	return REF_SUCCESS;



  *allowed =  REF_TRUE;

  return REF_SUCCESS;
}

REF_STATUS ref_collapse_face_quality( REF_GRID ref_grid, 
				      REF_INT keep, REF_INT remove,
				      REF_BOOL *allowed )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT item, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT node;
  REF_DBL quality;
  REF_BOOL will_be_collapsed;
  REF_DBL edge_ratio;

  *allowed = REF_FALSE;

  ref_cell = ref_grid_tri(ref_grid);
  each_ref_cell_having_node( ref_cell, remove, item, cell )
    {
      RSS( ref_cell_nodes( ref_cell, cell, nodes ), "nodes" );

      will_be_collapsed = REF_FALSE;
      for ( node = 0; node < ref_cell_node_per(ref_cell) ; node++ )
	if ( keep == nodes[node] ) will_be_collapsed = REF_TRUE;
      if ( will_be_collapsed ) continue;

      for ( node = 0; node < ref_cell_node_per(ref_cell) ; node++ )
	if ( remove != nodes[node] )
	  {
	    RSS( ref_node_ratio( ref_node, keep, nodes[node], 
				 &edge_ratio ), "ratio");
	    if ( edge_ratio > ref_adapt_collapse_ratio_limit )
	      return REF_SUCCESS;
	  }

      for ( node = 0; node < ref_cell_node_per(ref_cell) ; node++ )
	if ( remove == nodes[node] ) nodes[node] = keep;
      RSS( ref_node_tri_quality( ref_node,nodes,&quality ), "qual");
      if ( quality < ref_adapt_collapse_quality_absolute ) return REF_SUCCESS;
    }

  /* FIXME check quads too */

  *allowed = REF_TRUE;

  return REF_SUCCESS;
}

REF_STATUS ref_collapse_face_outward_norm( REF_GRID ref_grid, 
					   REF_INT keep, REF_INT remove,
					   REF_BOOL *allowed )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT item, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT node;
  REF_DBL normal[3];
  REF_BOOL will_be_collapsed;

  *allowed = REF_FALSE;

  ref_cell = ref_grid_tri(ref_grid);
  each_ref_cell_having_node( ref_cell, remove, item, cell )
    {
      RSS( ref_cell_nodes( ref_cell, cell, nodes ), "nodes" );

      will_be_collapsed = REF_FALSE;
      for ( node = 0; node < ref_cell_node_per(ref_cell) ; node++ )
	if ( keep == nodes[node] ) will_be_collapsed = REF_TRUE;
      if ( will_be_collapsed ) continue;

      for ( node = 0; node < ref_cell_node_per(ref_cell) ; node++ )
	if ( remove == nodes[node] ) nodes[node] = keep;

      RSS( ref_node_tri_normal( ref_node,nodes,normal ), "norm");

      if ( ( ref_node_xyz(ref_node,1,nodes[0]) > 0.5 &&
	     normal[1] >= 0.0 ) ||
	   ( ref_node_xyz(ref_node,1,nodes[0]) < 0.5 &&
	     normal[1] <= 0.0 ) ) return REF_SUCCESS;
    }

  *allowed = REF_TRUE;

  return REF_SUCCESS;
}

REF_STATUS ref_collapse_face_geometry( REF_GRID ref_grid, 
				       REF_INT keep, REF_INT remove,
				       REF_BOOL *allowed )
{
  REF_CELL ref_cell = ref_grid_qua(ref_grid);
  REF_INT item, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT deg, degree1;
  REF_INT id, ids1[2]; 
  REF_BOOL already_have_it;

  degree1 = 0;
  each_ref_cell_having_node( ref_cell, remove, item, cell )
    {
      RSS( ref_cell_nodes( ref_cell, cell, nodes ), "nodes" );
      id = nodes[ref_cell_node_per(ref_cell)];
      already_have_it = REF_FALSE;
      for (deg=0;deg<degree1;deg++)
	if ( id == ids1[deg] ) already_have_it = REF_TRUE;
      if ( !already_have_it )
	{
	  ids1[degree1] = id;
	  degree1++;
	  if ( 2 == degree1 ) break;
	}
    }

  *allowed = REF_FALSE;

  switch ( degree1 )
    {
    case 2: /* geometry node never allowed to move */
      *allowed = REF_FALSE;
      break;
    case 1: /* geometry face allowed if on that face */
      RSS( ref_cell_has_side( ref_cell, keep, remove, allowed ),
	   "allowed if a side of a quad" );
      break;
    case 0: /* volume node always allowed */
      *allowed = REF_TRUE;
      break;
    }

  return REF_SUCCESS;
}

REF_STATUS ref_collapse_face_same_tangent( REF_GRID ref_grid, 
					   REF_INT keep, REF_INT remove,
					   REF_BOOL *allowed )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_qua(ref_grid);
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT item, cell;
  REF_INT other, node, ixyz;
  REF_DBL tangent0[3], tangent1[3];
  REF_DBL dot;
  REF_STATUS status;

  *allowed = REF_TRUE;

  each_ref_cell_having_node( ref_cell, remove, item, cell )
    {
      /* a quad with keep and remove will be removed */
      if ( keep == ref_cell_c2n(ref_cell,0,cell) ||
	   keep == ref_cell_c2n(ref_cell,1,cell) ||
	   keep == ref_cell_c2n(ref_cell,2,cell) ||
	   keep == ref_cell_c2n(ref_cell,3,cell) ) continue;
      RSS( ref_cell_nodes( ref_cell, cell, nodes ), "nodes" );
      other = REF_EMPTY;
      for ( node=0; node<ref_cell_node_per(ref_cell) ; node++ )
	if (nodes[node] != remove && 
	    0.1 > ABS ( ref_node_xyz(ref_node,1,nodes[node] ) -
			ref_node_xyz(ref_node,1,remove) ) )
	  other = nodes[node];
      RAS( REF_EMPTY != other, "other not found" );
      for (ixyz=0;ixyz<3;ixyz++)
	{
	  tangent0[ixyz] = ref_node_xyz(ref_node,ixyz,remove) -
                           ref_node_xyz(ref_node,ixyz,other );
	  tangent1[ixyz] = ref_node_xyz(ref_node,ixyz,keep  ) -
                           ref_node_xyz(ref_node,ixyz,other );
	}
      RSS( ref_math_normalize( tangent0 ), "zero length orig quad" );
      status = ref_math_normalize( tangent1 );
      if ( REF_DIV_ZERO == status )
	{ /* new quad face has zero area */
	  *allowed = REF_FALSE;
	  return REF_SUCCESS;	  
	}
      dot = ref_math_dot( tangent0, tangent1 );
      if ( dot < (1.0-1.0e-8) ) /* acos(1.0-1.0e-8) ~ 0.0001 radian, 0.01 deg */
	{
	  *allowed = REF_FALSE;
	  return REF_SUCCESS;	  
	}
    }

  return REF_SUCCESS;
}

REF_STATUS ref_collapse_twod_pass( REF_GRID ref_grid )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_EDGE ref_edge;
  REF_DBL *ratio;
  REF_INT *order;
  REF_INT ntarget, *target, *node2target;
  REF_INT node, node0, node1;
  REF_INT i, edge;
  REF_INT item, cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL edge_ratio;
  REF_BOOL active;

  RSS( ref_edge_create( &ref_edge, ref_grid ), "orig edges" );

  ref_malloc_init( ratio, ref_node_max(ref_node), 
		   REF_DBL, 2.0*ref_adapt_collapse_ratio );

  for(edge=0;edge<ref_edge_n(ref_edge);edge++)
    {
      node0 = ref_edge_e2n( ref_edge, 0, edge );
      node1 = ref_edge_e2n( ref_edge, 1, edge );
      RSS( ref_node_edge_twod( ref_node, node0, node1, 
			       &active ), "act" );
      if ( !active ) continue;

      RSS( ref_node_ratio( ref_node, node0, node1,
			   &edge_ratio ), "ratio");
      ratio[node0] = MIN( ratio[node0], edge_ratio );
      ratio[node1] = MIN( ratio[node1], edge_ratio );
    }

  ref_malloc( target, ref_node_n(ref_node), REF_INT );
  ref_malloc_init( node2target, ref_node_max(ref_node), REF_INT, REF_EMPTY );

  ntarget=0;
  for ( node=0 ; node < ref_node_max(ref_node) ; node++ )
    if ( ratio[node] < ref_adapt_collapse_ratio )
      {
	node2target[node] = ntarget;
	target[ntarget] = node;
	ratio[ntarget] = ratio[node];
	ntarget++;
      }

  ref_malloc( order, ntarget, REF_INT );

  RSS( ref_sort_heap_dbl( ntarget, ratio, order), "sort lengths" );

  for ( i = 0; i < ntarget; i++ )
    {
      if ( ratio[order[i]] > ref_adapt_collapse_ratio ) continue; 
      node1 = target[order[i]];
      RSS( ref_collapse_face_remove_node1( ref_grid, &node0, node1 ), 
	   "collapse rm" );
      if ( !ref_node_valid(ref_node,node1) )
	{
	  ref_node_age(ref_node,node0) = 0;
	  each_ref_cell_having_node( ref_cell, node0, item, cell )
	    {
	      RSS( ref_cell_nodes( ref_cell, cell, nodes), "cell nodes");
	      for (node=0;node<ref_cell_node_per(ref_cell);node++)
		if ( REF_EMPTY != node2target[nodes[node]] )
		  ratio[node2target[nodes[node]]] = 
		    2.0*ref_adapt_collapse_ratio;
	    }
	}
    }
  
  ref_free( order );
  ref_free( node2target );
  ref_free( target );
  ref_free( ratio );

  ref_edge_free( ref_edge );

  return REF_SUCCESS;
}

REF_STATUS ref_collapse_face_remove_node1( REF_GRID ref_grid, 
					   REF_INT *actual_node0, 
					   REF_INT node1 )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT nnode, node;
  REF_INT node_to_collapse[MAX_NODE_LIST];
  REF_INT order[MAX_NODE_LIST];
  REF_DBL ratio_to_collapse[MAX_NODE_LIST];
  REF_INT node0, node2, node3;
  REF_BOOL allowed;
  REF_BOOL verbose = REF_FALSE;

  *actual_node0 = REF_EMPTY;

  RSS( ref_cell_node_list_around( ref_cell, node1, MAX_NODE_LIST,
				  &nnode, node_to_collapse ), "da hood");
  for ( node=0 ; node < nnode ; node++ )
    RSS( ref_node_ratio( ref_node, node_to_collapse[node], node1, 
			 &(ratio_to_collapse[node]) ), "ratio");

  RSS( ref_sort_heap_dbl( nnode, ratio_to_collapse, order), "sort lengths" );

  for ( node=0 ; node < nnode ; node++ )
    {
      node0 = node_to_collapse[order[node]];
  
      RSS(ref_collapse_face_geometry(ref_grid,node0,node1,&allowed),"col geom");
      if ( !allowed && verbose ) printf("%d geom\n",node);
      if ( !allowed ) continue;

      RSS(ref_collapse_face_same_tangent(ref_grid,node0,node1,&allowed),"tan");
      if ( !allowed && verbose ) printf("%d tang\n",node);
      if ( !allowed ) continue;

      RSS(ref_collapse_face_outward_norm(ref_grid,node0,node1,&allowed),"norm");
      if ( !allowed && verbose ) printf("%d outw\n",node);
      if ( !allowed ) continue;

      RSS(ref_collapse_face_quality(ref_grid,node0,node1,&allowed),"qual");
      if ( !allowed && verbose ) printf("%d qual\n",node);
      if ( !allowed ) continue;

      RSS(ref_collapse_face_local_pris(ref_grid,node0,node1,&allowed),"colloc");
      if ( !allowed && verbose ) printf("%d loca\n",node);
      if ( !allowed ) 
	{
	  ref_node_age(ref_node,node0)++;
	  ref_node_age(ref_node,node1)++;
	  continue;
	}

      if (verbose) printf("%d split!\n",node);

      *actual_node0 = node0;
      RSS(ref_twod_opposite_edge(ref_grid_pri(ref_grid),
				 node0,node1,&node2,&node3),"opp");

      RSS( ref_collapse_face( ref_grid, node0, node1, node2, node3 ), "col!");

      break;

    }

  return REF_SUCCESS;
}

