
#include <stdlib.h>
#include <stdio.h>

#include "ref_fortran.h"

#include "ref_grid.h"
#include "ref_subdiv.h"
#include "ref_export.h"
#include "ref_mpi.h"
#include "ref_malloc.h"

#include "ref_migrate.h"
#include "ref_metric.h"
#include "ref_adapt.h"
#include "ref_validation.h"

#include "ref_gather.h"
#include "ref_histogram.h"

static REF_GRID ref_grid = NULL;

REF_BOOL ref_fortran_allow_screen_output = REF_TRUE;

REF_STATUS FC_FUNC_(ref_fortran_init,REF_FORTRAN_INIT)
     (REF_INT *nnodes, REF_INT *nnodesg,
      REF_INT *l2g, REF_INT *part, REF_INT *partition, 
      REF_DBL *x, REF_DBL *y, REF_DBL *z )
{
  REF_NODE ref_node;
  REF_INT node, pos;
  RSS( ref_grid_create( &ref_grid ), "create grid" );
  ref_node = ref_grid_node(ref_grid);

  RSS( ref_mpi_initialize(),"init ref mpi");
  ref_mpi_stopwatch_start(  );

  REIS( *partition, ref_mpi_id, "processor ids do not match" );

  RSS( ref_node_initialize_n_global( ref_node, *nnodesg), "init nnodesg");

  for (node=0;node<(*nnodes);node++)
    {
      RSS( ref_node_add( ref_node, l2g[node]-1, &pos ), 
	   "add node");
      ref_node_xyz(ref_node,0,pos) = x[node];
      ref_node_xyz(ref_node,1,pos) = y[node];
      ref_node_xyz(ref_node,2,pos) = z[node];
      ref_node_part(ref_node,pos) = part[node]-1;
    }


  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_import_cell,REF_FORTRAN_IMPORT_CELL)
     (REF_INT *node_per_cell, REF_INT *ncell, REF_INT *c2n)
{
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_CELL ref_cell;
  REF_INT cell, node, new_cell;

  RSS( ref_grid_cell_with(ref_grid,*node_per_cell,&ref_cell),"get cell");

  for ( cell = 0 ; cell < (*ncell) ; cell++ ) 
    {
      for ( node = 0 ; node < (*node_per_cell) ; node++ )
	nodes[node] = c2n[node+(*node_per_cell)*cell] - 1;
      RSS( ref_cell_add( ref_cell, nodes, &new_cell ), "add cell");
    }

  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_import_face,REF_FORTRAN_IMPORT_FACE)
     (REF_INT *face_index, REF_INT *node_per_face, REF_INT *nface, 
      REF_INT *f2n )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT *nodes;
  REF_CELL ref_cell;
  REF_INT face, node, new_face;
  REF_BOOL has_a_local_node;

  RSS( ref_grid_face_with(ref_grid,*node_per_face,&ref_cell),"get face");

  nodes = (REF_INT *)malloc( ((*node_per_face)+1) * sizeof(REF_INT));
  RNS( nodes,"malloc nodes NULL");
  for ( face = 0 ; face < (*nface) ; face++ ) 
    {
      has_a_local_node = REF_FALSE;
      for ( node = 0 ; node < (*node_per_face) ; node++ )
	{
	  nodes[node] = f2n[node+(*node_per_face)*face] - 1;
	  has_a_local_node = has_a_local_node ||
	    (ref_mpi_id == ref_node_part(ref_node,nodes[node]));
	}
      nodes[(*node_per_face)] = (*face_index);
      if ( has_a_local_node ) 
	RSS( ref_cell_add( ref_cell, nodes, &new_face ), "add face");
    }
  free(nodes);
  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_viz,REF_FORTRAN_VIZ)( void )
{
  char filename[1024];
  sprintf(filename,"ref_viz%04d.vtk",ref_mpi_id);
  RSS( ref_export_vtk( ref_grid, filename ), "export vtk");
  sprintf(filename,"ref_viz%04d.tec",ref_mpi_id);
  RSS( ref_export_tec( ref_grid, filename ), "export tec");
  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_adapt,REF_FORTRAN_ADAPT)( void )
{
  REF_INT passes, i;
  REF_INT ntet, npri;
  REF_BOOL single_partition_adaptation = REF_TRUE;

  RSS( ref_gather_ncell( ref_grid_node(ref_grid), 
			 ref_grid_tet(ref_grid), &ntet ), "ntet");
  RSS( ref_gather_ncell( ref_grid_node(ref_grid), 
			 ref_grid_pri(ref_grid), &npri ), "npri");

  if ( ref_mpi_master )
    {
      ref_grid_twod(ref_grid) = ( 0 == ntet && 0 != npri );
      if ( ref_grid_twod(ref_grid) ) printf ("assuming twod mode\n");
    }
  RSS( ref_mpi_bcast( &ref_grid_twod(ref_grid), 1, REF_INT_TYPE ), "bcast" );

  if ( REF_FALSE ) /* Pointwise midplane location for Troy Lake */
    {
      ref_node_twod_mid_plane(ref_grid_node(ref_grid))=-1;
      if ( ref_mpi_master ) 
	printf ("twod midplane %f\n",
		ref_node_twod_mid_plane(ref_grid_node(ref_grid)));
    }

  if ( REF_FALSE ) 
    {
      RSS( ref_metric_sanitize(ref_grid),"sant");
      RSS( ref_node_ghost_real( ref_grid_node(ref_grid) ), "ghost real");
      ref_mpi_stopwatch_stop("metric sant");
    }

  if ( REF_FALSE )
    {
      REF_INT gradation;
      for ( gradation =0 ; gradation<10 ; gradation++ )
	{
	  if ( ref_mpi_master )
	    printf("gradation %d\n",gradation);
	  RSS( ref_metric_gradation( ref_grid ), "grad");
	  RSS( ref_node_ghost_real( ref_grid_node(ref_grid) ), "ghost real");
	}
      ref_mpi_stopwatch_stop("metric gradation");
    }

  RSS( ref_gather_tec_movie_record_button( REF_FALSE ), "rec" );

  RSS(ref_validation_cell_volume(ref_grid),"vol");
  RSS( ref_histogram_ratio( ref_grid ), "gram");

  passes = 20;
  for (i = 0; i<passes ; i++ )
    {
      RSS( ref_adapt_pass( ref_grid ), "pass");
      ref_mpi_stopwatch_stop("pass");
      RSS(ref_validation_cell_volume(ref_grid),"vol");
      RSS( ref_histogram_ratio( ref_grid ), "gram");
      if ( single_partition_adaptation )
	{
	  if ( ref_mpi_master ) printf("use single parition\n");
	  RSS(ref_migrate_to_single_image(ref_grid),"balance");
	}
      else
	{
	  RSS(ref_migrate_to_balance(ref_grid),"balance");
	}
      ref_mpi_stopwatch_stop("balance");
    }

  return REF_SUCCESS;
}


REF_STATUS FC_FUNC_(ref_fortran_import_metric,REF_FORTRAN_IMPORT_METRIC)
     (REF_INT *nnodes, REF_DBL *m )
{
  REF_INT node, i;
  REF_NODE ref_node = ref_grid_node(ref_grid);

  for (node = 0; node < (*nnodes); node++)
    for (i = 0; i < 6 ; i++)
      ref_node_metric(ref_node,i,node) = m[i+6*node];

  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_import_ratio,REF_FORTRAN_IMPORT_RATIO)
     (REF_INT *nnodes, REF_DBL *ratio )
{
  REF_SUBDIV ref_subdiv;

  REIS( *nnodes, ref_node_n( ref_grid_node(ref_grid) ), "nnode mismatch" );

  if ( ref_fortran_allow_screen_output )
    RSS(ref_validation_cell_volume(ref_grid),"vol");

  RSS(ref_subdiv_create(&ref_subdiv,ref_grid),"create");
  RSS(ref_subdiv_mark_prism_by_ratio(ref_subdiv, ratio),"mark ratio");
  RSS(ref_subdiv_split(ref_subdiv),"split");
  RSS(ref_subdiv_free(ref_subdiv),"free");

  if ( ref_fortran_allow_screen_output )
    RSS(ref_validation_cell_volume(ref_grid),"vol");

  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_size_node,REF_FORTRAN_SIZE_NODE)
     (REF_INT *nnodes0, REF_INT *nnodes, REF_INT *nnodesg)
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node;

  RSS( ref_node_synchronize_globals( ref_node ), "sync glob" );

  *nnodes = ref_node_n(ref_node);
  *nnodesg = ref_node_n_global(ref_node);

  *nnodes0 = 0;
  
  each_ref_node_valid_node(ref_node,node)
    if ( ref_mpi_id == ref_node_part(ref_node,node) ) (*nnodes0)++;

  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_node,REF_FORTRAN_NODE)
     ( REF_INT *nnodes,
       REF_INT *l2g,
       REF_DBL *x, REF_DBL *y, REF_DBL *z )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT *o2n, *n2o, node;

  RSS(ref_node_compact(ref_node,&o2n,&n2o),"compact");

  for ( node = 0; node< ref_node_n( ref_node ); node++ )
    {
      l2g[node] = ref_node_global(ref_node,n2o[node]) + 1;
      x[node] = ref_node_xyz(ref_node,0,n2o[node]);
      y[node] = ref_node_xyz(ref_node,1,n2o[node]);
      z[node] = ref_node_xyz(ref_node,2,n2o[node]);
    }

  ref_free(n2o);
  ref_free(o2n);

  REIS( *nnodes, ref_node_n( ref_node ), "nnode mismatch" );

  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_size_cell,REF_FORTRAN_SIZE_CELL)
     (REF_INT *node_per_cell, REF_INT *ncell)
{
  REF_CELL ref_cell;

  RSS( ref_grid_cell_with( ref_grid, *node_per_cell, &ref_cell ), "get cell");
  *ncell = ref_cell_n(ref_cell);

  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_cell,REF_FORTRAN_CELL)
     ( REF_INT *node_per_cell, REF_INT *ncell, 
       REF_INT *c2n )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT cell, i, node;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT *o2n, *n2o;

  RSS(ref_node_compact(ref_node,&o2n,&n2o),"compact");

  RSS( ref_grid_cell_with( ref_grid, *node_per_cell, &ref_cell ), "get cell");

  i = 0;
  each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes )
    {
      for (node=0;node<ref_cell_node_per(ref_cell);node++)
	c2n[node+(*node_per_cell)*i] = o2n[nodes[node]]+1;
      i++;
    }

  ref_free(n2o);
  ref_free(o2n);

  REIS( *ncell, i, "ncell mismatch" );

  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_size_face,REF_FORTRAN_SIZE_FACE)
     ( REF_INT *ibound, REF_INT *node_per_face, REF_INT *nface )
{
  REF_CELL ref_cell;
  REF_INT cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];

  RSS( ref_grid_face_with( ref_grid, *node_per_face, &ref_cell ), "get face");
  *nface = 0;
  each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes )
    if ( *ibound == nodes[*node_per_face] )
      (*nface)++;

  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_face,REF_FORTRAN_FACE)
     ( REF_INT *ibound, REF_INT *node_per_face, REF_INT *nface, 
       REF_INT *f2n )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_CELL ref_cell;
  REF_INT cell, i, node;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT *o2n, *n2o;

  RSS(ref_node_compact(ref_node,&o2n,&n2o),"compact");

  RSS( ref_grid_face_with( ref_grid, *node_per_face, &ref_cell ), "get face");

  i = 0;
  each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes )
    if ( *ibound == nodes[ref_cell_node_per(ref_cell)] )
    {
      for (node=0;node<ref_cell_node_per(ref_cell);node++)
	f2n[node+(*node_per_face)*i] = o2n[nodes[node]]+1;
      i++;
    }

  ref_free(n2o);
  ref_free(o2n);

  REIS( *nface, i, "nface mismatch" );

  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_naux,REF_FORTRAN_NAUX)
     ( REF_INT *naux )
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  ref_node_naux(ref_node) = *naux;
  RSS( ref_node_resize_aux( ref_node ), "size aux" );
  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_import_aux,REF_FORTRAN_IMPORT_AUX)
     ( REF_INT *ldim, REF_INT *nnodes, REF_INT *offset, REF_DBL *aux)
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node, i;
  for (node = 0; node < (*nnodes); node++)
    for (i = 0; i < (*ldim) ; i++)
      ref_node_aux(ref_node,i+(*offset),node) = aux[i+(*ldim)*node];
  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_aux,REF_FORTRAN_AUX)
     ( REF_INT *ldim, REF_INT *nnodes, REF_INT *offset, REF_DBL *aux)
{
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node, i;
  REF_INT *o2n, *n2o;

  SUPRESS_UNUSED_COMPILER_WARNING(nnodes);

  RSS(ref_node_compact(ref_node,&o2n,&n2o),"compact");

  for (node = 0; node < ref_node_n(ref_node); node++)
    for (i = 0; i < (*ldim) ; i++)
      aux[i+(*ldim)*node] = ref_node_aux(ref_node,i+(*offset),n2o[node]);

  ref_free(n2o);
  ref_free(o2n);

  REIS( *nnodes, ref_node_n(ref_node), "nnode mismatch" );

  return REF_SUCCESS;
}

REF_STATUS FC_FUNC_(ref_fortran_free,REF_FORTRAN_FREE)( void )
{
  RSS( ref_grid_free( ref_grid ), "free grid");
  ref_grid = NULL;
  return REF_SUCCESS;
}

