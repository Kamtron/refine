
#include <stdlib.h>
#include <stdio.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ref_migrate.h"

#include "ref_mpi.h"
#include "ref_malloc.h"

#include "ref_sort.h"
#include "ref_node.h"

#include "ref_export.h"

REF_STATUS ref_migrate_create( REF_MIGRATE *ref_migrate_ptr, REF_GRID ref_grid )
{
  REF_MIGRATE ref_migrate;
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT node;

  ref_malloc( *ref_migrate_ptr, 1, REF_MIGRATE_STRUCT );

  ref_migrate = *ref_migrate_ptr;

  ref_migrate_grid(ref_migrate) = ref_grid;

  RSS( ref_adj_create( &(ref_migrate_parent_global(ref_migrate)) ), "make adj");
  RSS( ref_adj_create( &(ref_migrate_parent_part(ref_migrate)) ), "make adj");

  ref_migrate_max(ref_migrate) = ref_node_max(ref_node);

  ref_malloc_init( ref_migrate->global, ref_migrate_max(ref_migrate),
		   REF_INT, REF_EMPTY);
  ref_malloc( ref_migrate->xyz, 3*ref_migrate_max(ref_migrate), REF_DBL);
  ref_malloc( ref_migrate->weight, ref_migrate_max(ref_migrate), REF_DBL);

  each_ref_node_valid_node( ref_node, node )
    if ( ref_mpi_id == ref_node_part(ref_node,node) )
      {
	ref_migrate_global(ref_migrate,node) = ref_node_global(ref_node,node);
	RSS( ref_adj_add( ref_migrate_parent_global(ref_migrate),
			  node, ref_node_global(ref_node, node) ),"add");
	RSS( ref_adj_add( ref_migrate_parent_part(ref_migrate),
			  node, ref_node_part(ref_node, node) ),"add");
	ref_migrate_xyz( ref_migrate, 0, node ) = 
	  ref_node_xyz(ref_node,0,node);
	ref_migrate_xyz( ref_migrate, 1, node ) = 
	  ref_node_xyz(ref_node,1,node);
	ref_migrate_xyz( ref_migrate, 2, node ) = 
	  ref_node_xyz(ref_node,2,node);
	ref_migrate_weight(ref_migrate,node) = 1.0;
      }

  return REF_SUCCESS;
}

REF_STATUS ref_migrate_free( REF_MIGRATE ref_migrate )
{
  if ( NULL == (void *)ref_migrate ) return REF_NULL;

  RSS( ref_adj_free( ref_migrate_parent_part(ref_migrate) ), "free adj");
  RSS( ref_adj_free( ref_migrate_parent_global(ref_migrate) ), "free adj");

  ref_free( ref_migrate->global );
  ref_free( ref_migrate->xyz );
  ref_free( ref_migrate->weight );
  ref_free( ref_migrate );

  return REF_SUCCESS;
}

REF_STATUS ref_migrate_inspect( REF_MIGRATE ref_migrate )
{
  REF_NODE ref_node = ref_grid_node( ref_migrate_grid(ref_migrate) );
  REF_INT node,item,global,part;

  each_ref_migrate_node( ref_migrate, node )
    {
      printf(" %2d : %3d :",
	     ref_mpi_id,
	     ref_node_global(ref_node,node));
      each_ref_adj_node_item_with_ref(ref_migrate_parent_global( ref_migrate ), 
				      node, item, global)
	{
	  part = ref_adj_item_ref( ref_migrate_parent_part(ref_migrate),item);
	  printf(" %3d+%d",global,part);
	}
      printf("\n");
    }
  return REF_SUCCESS;
}

REF_STATUS ref_migrate_2d_agglomeration_keep( REF_MIGRATE ref_migrate,
					      REF_INT keep, REF_INT lose)
{
  REF_NODE ref_node = ref_grid_node( ref_migrate_grid(ref_migrate) );
  REF_INT item, global;

  /* not working for general agglomeration, ghost lose? */

  RAS( ref_node_valid( ref_node, keep ), "keep node invalid" );
  RAS( ref_node_valid( ref_node, lose ), "lose node invalid" );

  ref_migrate_global(ref_migrate,lose) = REF_EMPTY;

  /* skip if the lose node has been agglomerated */
  each_ref_adj_node_item_with_ref( ref_migrate_parent_global( ref_migrate ), 
				   keep, item, global)
    if ( global == ref_node_global(ref_node,lose) ) return REF_SUCCESS;

  /* skip if keep node is off-proc or already agglomerated */
  if ( ! ref_migrate_valid( ref_migrate, keep ) ) return REF_SUCCESS;

  ref_migrate_xyz( ref_migrate, 1, keep ) = 0.5;
  ref_migrate_weight(ref_migrate,keep) = 2.0;
  RSS( ref_adj_add( ref_migrate_parent_global(ref_migrate),
		    keep, ref_node_global(ref_node, lose) ),"add");
  RSS( ref_adj_add( ref_migrate_parent_part(ref_migrate),
		    keep, ref_node_part(ref_node, lose) ),"add");

 
  return REF_SUCCESS;
}

#ifdef HAVE_ZOLTAN
#include "zoltan.h"

static int ref_migrate_local_n( void *void_ref_migrate, int *ierr )
{
  REF_MIGRATE ref_migrate = ((REF_MIGRATE)void_ref_migrate);
  int node, n;

  *ierr = 0;

  n = 0;
  each_ref_migrate_node( ref_migrate, node )
    n++;

  return n;
}

static void ref_migrate_local_ids( void *void_ref_migrate, 
				   int global_dim, int local_dim,
				   ZOLTAN_ID_PTR global, ZOLTAN_ID_PTR local,
				   int wgt_dim, float *obj_wgts, int *ierr )
{
  REF_MIGRATE ref_migrate = ((REF_MIGRATE)void_ref_migrate);
  REF_INT node, n;

  if ( 1 != global_dim || 1 != local_dim || 1 !=  wgt_dim )
    {
      printf("%s: %d: %s: %s\n",__FILE__,__LINE__,__func__,"bad sizes");
      *ierr = ZOLTAN_FATAL;
      return;
    }

  *ierr = 0;

  n = 0;
  each_ref_migrate_node( ref_migrate, node )
    {
      local[n] = node;
      global[n] = ref_migrate_global(ref_migrate,node);
      obj_wgts[n] = (float)ref_migrate_weight(ref_migrate,node);
      n++;
    }
}

static int ref_migrate_geom_dimensionality( void *void_ref_migrate, 
					    int *ierr )
{
  SUPRESS_UNUSED_COMPILER_WARNING(void_ref_migrate);
  *ierr = 0;
  return 3;
}

static void ref_migrate_geom( void *void_ref_migrate, 
			      int global_dim, int local_dim, int nnode,
			      ZOLTAN_ID_PTR global, ZOLTAN_ID_PTR local,
			      int xyz_dim, double *xyz, int *ierr )
{
  REF_MIGRATE ref_migrate = ((REF_MIGRATE)void_ref_migrate);
  REF_INT node;

  SUPRESS_UNUSED_COMPILER_WARNING(global);
  *ierr = 0;

  if ( 1 != global_dim || 1 != local_dim || 3 != xyz_dim  )
    {
      printf("%s: %d: %s: %s\n",__FILE__,__LINE__,__func__,"bad sizes");
      *ierr = ZOLTAN_FATAL;
      return;
    }

  for ( node=0 ;node < nnode ; node++ )
    {
      if ( !ref_migrate_valid( ref_migrate, local[node] ) )
	{
	  printf("%s: %d: %s: %d %d invalid\n",
		 __FILE__,__LINE__,__func__,node,(REF_INT)local[node]);
	  *ierr = ZOLTAN_FATAL;
	  return;
	}
      xyz[0+3*node] = ref_migrate_xyz(ref_migrate,0,local[node]);
      xyz[1+3*node] = ref_migrate_xyz(ref_migrate,1,local[node]);
      xyz[2+3*node] = ref_migrate_xyz(ref_migrate,2,local[node]);
    }

}

static REF_STATUS ref_migrate_2d_agglomeration( REF_MIGRATE ref_migrate )
{
  REF_GRID ref_grid = ref_migrate_grid( ref_migrate );
  REF_NODE ref_node = ref_grid_node( ref_migrate_grid(ref_migrate) );
  REF_INT cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT keep, lose;

  each_ref_cell_valid_cell_with_nodes( ref_grid_pri(ref_grid), cell, nodes)
    {
      if ( ref_node_global(ref_node,nodes[0]) <
	   ref_node_global(ref_node,nodes[3]) )
	{
	  keep = nodes[0]; lose = nodes[3];
	}
      else
	{
	  keep = nodes[3]; lose = nodes[0];
	}
      RSS( ref_migrate_2d_agglomeration_keep( ref_migrate, keep, lose), "0-3");
	
      if ( ref_node_global(ref_node,nodes[1]) <
	   ref_node_global(ref_node,nodes[4]) )
	{
	  keep = nodes[1]; lose = nodes[4];
	}
      else
	{
	  keep = nodes[4]; lose = nodes[1];
	}
      RSS( ref_migrate_2d_agglomeration_keep( ref_migrate, keep, lose), "1-4");
	
      if ( ref_node_global(ref_node,nodes[2]) <
	   ref_node_global(ref_node,nodes[5]) )
	{
	  keep = nodes[2]; lose = nodes[5];
	}
      else
	{
	  keep = nodes[5]; lose = nodes[2];
	}
      RSS( ref_migrate_2d_agglomeration_keep( ref_migrate, keep, lose), "2-5");
      
    }

  return REF_SUCCESS;
}

#endif

REF_STATUS ref_migrate_to_balance( REF_GRID ref_grid )
{

  RSS( ref_migrate_new_part( ref_grid ), "new part" );
  RSS( ref_migrate_shufflin( ref_grid ), "shufflin" );

  return REF_SUCCESS;
}

REF_STATUS ref_migrate_new_part( REF_GRID ref_grid )
{

#ifdef HAVE_ZOLTAN
  {
    REF_NODE ref_node = ref_grid_node( ref_grid );
    REF_MIGRATE ref_migrate;
    int partitions_have_changed;
    int global_id_dimension, local_id_dimension;

    int import_n;
    ZOLTAN_ID_PTR import_global, import_local;
    int *import_proc, *import_part;

    int export_n;
    ZOLTAN_ID_PTR export_global, export_local;
    int *export_proc, *export_part;

    float ver;

    REF_INT node, item, local, global, part;

    REF_INT *migrate_part;
    REF_INT *node_part;
 
    REF_INT *a_next;
    REF_INT *a_parts, *b_parts;
    REF_INT *a_size, *b_size;
    REF_INT a_total, b_total;

    struct Zoltan_Struct *zz;

    RSS( ref_node_synchronize_globals( ref_node ), "sync global nodes");

    if ( 1 == ref_mpi_n ) return REF_SUCCESS;

    RSS( ref_migrate_create( &ref_migrate, ref_grid ), "create migrate");

    if ( ref_grid_twod(ref_grid) )
      {
	RSS( ref_migrate_2d_agglomeration( ref_migrate ), "2d agglom" );
      }

    REIS( ZOLTAN_OK, 
	  Zoltan_Initialize(ref_mpi_argc, ref_mpi_argv, &ver), 
	  "Zoltan is angry");
    zz = Zoltan_Create(MPI_COMM_WORLD);

    /* General parameters */

    Zoltan_Set_Param(zz, "DEBUG_LEVEL", "0");
    Zoltan_Set_Param(zz, "RETURN_LISTS", "PARTS");
    Zoltan_Set_Param(zz, "LB_APPROACH", "PARTITION");
    Zoltan_Set_Param(zz, "LB_METHOD", "RCB");

    Zoltan_Set_Param(zz, "OBJ_WEIGHT_DIM", "1");

    Zoltan_Set_Num_Obj_Fn(zz, ref_migrate_local_n, 
			  (void *)ref_migrate);
    Zoltan_Set_Obj_List_Fn(zz, ref_migrate_local_ids, 
			   (void *)ref_migrate);
    Zoltan_Set_Num_Geom_Fn(zz, ref_migrate_geom_dimensionality, 
			   (void *)ref_migrate);
    Zoltan_Set_Geom_Multi_Fn(zz, ref_migrate_geom, 
			     (void *)ref_migrate);

    REIS( ZOLTAN_OK, 
	  Zoltan_LB_Partition(zz,
			      &partitions_have_changed,
			      &global_id_dimension,
			      &local_id_dimension,
			      &import_n,
			      &import_global,
			      &import_local,
			      &import_proc,
			      &import_part,
			      &export_n,
			      &export_global,
			      &export_local,
			      &export_proc,
			      &export_part),
	  "Zoltan is angry");

    ref_malloc_init( migrate_part, ref_migrate_max(ref_node), 
		     REF_INT, REF_EMPTY );
    ref_malloc_init( node_part, ref_node_max(ref_node), REF_INT, REF_EMPTY );

    for(node=0; node<export_n; node++)
      migrate_part[export_local[node]] = export_part[node];

    ref_malloc_init( a_size, ref_mpi_n, REF_INT, 0 );
    ref_malloc_init( b_size, ref_mpi_n, REF_INT, 0 );

    each_ref_migrate_node( ref_migrate, node )
      each_ref_adj_node_item_with_ref(ref_migrate_parent_global(ref_migrate),
				      node, item, global )
        {
	  part = ref_adj_item_ref( ref_migrate_parent_part(ref_migrate),item);
	  if ( ref_mpi_id != part )
	    {
	      a_size[part]++;
	    }
	  else
	    {
	      RSS( ref_node_local( ref_node, global, &local ), "g2l" );
	      node_part[local] = migrate_part[node];
	    }
	}
    
    RSS( ref_mpi_alltoall( a_size, b_size, REF_INT_TYPE ), "alltoall sizes");

    a_total = 0;
    for ( part = 0; part<ref_mpi_n ; part++ )
      a_total += a_size[part];
    ref_malloc( a_parts, 2*a_total, REF_INT );

    b_total = 0;
    for ( part = 0; part<ref_mpi_n ; part++ )
      b_total += b_size[part];
    ref_malloc( b_parts, 2*b_total, REF_INT );

    ref_malloc( a_next, ref_mpi_n, REF_INT );
    a_next[0] = 0;
    for ( part = 1; part<ref_mpi_n ; part++ )
      a_next[part] = a_next[part-1]+a_size[part-1];

    each_ref_migrate_node( ref_migrate, node )
      each_ref_adj_node_item_with_ref(ref_migrate_parent_global(ref_migrate),
				      node, item, global )
        {
	  part = ref_adj_item_ref( ref_migrate_parent_part(ref_migrate),item);
	  if ( ref_mpi_id != part )
	    {
	      a_parts[0+2*a_next[part]] = global;
	      a_parts[1+2*a_next[part]] = migrate_part[node];
	      a_next[part]++;
	    }
	}

    RSS( ref_mpi_alltoallv( a_parts, a_size, b_parts, b_size, 
			    2, REF_INT_TYPE ), 
	 "alltoallv parts");

    for ( node = 0; node < b_total ; node++ )
      {
	global = b_parts[0+2*node];
	part = b_parts[1+2*node];
	RSS( ref_node_local( ref_node, global, &local ), "g2l" );
	node_part[local] = part;
      }

    free(a_next);
    free(b_parts);
    free(a_parts);
    free(b_size);
    free(a_size);

    RSS( ref_node_ghost_int( ref_node, node_part ), "ghost part");

    for(node=0; node<ref_node_max(ref_node); node++)
      ref_node_part(ref_node, node) = node_part[node];

    ref_free( node_part );
    ref_free( migrate_part );

    REIS( ZOLTAN_OK,
	  Zoltan_LB_Free_Part(&import_local, &import_global,
			      &import_proc, &import_proc ),
	  "Zoltan is angry");

    REIS( ZOLTAN_OK,
	  Zoltan_LB_Free_Part(&export_local, &export_global,
			      &export_proc, &export_proc ),
	  "Zoltan is angry");

    Zoltan_Destroy( &zz );

    RSS( ref_migrate_free( ref_migrate ), "free migrate");

  }
#else
  SUPRESS_UNUSED_COMPILER_WARNING(ref_grid);
#endif

  return REF_SUCCESS;
}

static REF_STATUS ref_migrate_shufflin_node( REF_NODE ref_node )
{
  REF_INT *a_size, *b_size;
  REF_INT a_total, b_total;
  REF_INT *a_global, *b_global;
  REF_INT part, node;
  REF_INT *a_next;
  REF_DBL *a_real, *b_real;
  REF_DBL *a_aux, *b_aux;
  REF_INT local;
  REF_INT i;

  ref_malloc_init( a_size, ref_mpi_n, REF_INT, 0 );
  ref_malloc_init( b_size, ref_mpi_n, REF_INT, 0 );

  each_ref_node_valid_node( ref_node, node )
    if ( ref_mpi_id != ref_node_part(ref_node,node) )
      {
	if( ref_node_part(ref_node,node) < 0 ||
	    ref_node_part(ref_node,node) >= ref_mpi_n )
	  {
	    printf("id %d node %d global %d part %d",
		   ref_mpi_id, node, 
		   ref_node_global(ref_node,node), 
		   ref_node_part(ref_node,node));
	    THROW( "part out of range" );
	  }
	a_size[ref_node_part(ref_node,node)]++;
      }

  RSS( ref_mpi_alltoall( a_size, b_size, REF_INT_TYPE ), "alltoall sizes");

  a_total = 0;
  for ( part = 0; part<ref_mpi_n ; part++ )
    a_total += a_size[part];
  ref_malloc( a_global, a_total, REF_INT );
  ref_malloc( a_real, REF_NODE_REAL_PER*a_total, REF_DBL );
  a_aux = NULL;
  if ( ref_node_naux(ref_node) > 0 )
    ref_malloc( a_aux, ref_node_naux(ref_node)*a_total, REF_DBL );

  b_total = 0;
  for ( part = 0; part<ref_mpi_n ; part++ )
    b_total += b_size[part];
  ref_malloc( b_global, b_total, REF_INT );
  ref_malloc( b_real, REF_NODE_REAL_PER*b_total, REF_DBL );
  b_aux = NULL;
  if ( ref_node_naux(ref_node) > 0 )
    ref_malloc( b_aux, ref_node_naux(ref_node)*b_total, REF_DBL );

  ref_malloc( a_next, ref_mpi_n, REF_INT );
  a_next[0] = 0;
  for ( part = 1; part<ref_mpi_n ; part++ )
    a_next[part] = a_next[part-1]+a_size[part-1];

  each_ref_node_valid_node( ref_node, node )
    if ( ref_mpi_id != ref_node_part(ref_node,node) )
      {
	part = ref_node_part(ref_node,node);
	a_global[a_next[part]] = ref_node_global(ref_node,node);
	for ( i=0; i < REF_NODE_REAL_PER ; i++ )
	  a_real[i+REF_NODE_REAL_PER*a_next[part]] = 
	    ref_node_real(ref_node,i,node);
	for ( i=0; i < ref_node_naux(ref_node) ; i++ )
	  a_aux[i+ref_node_naux(ref_node)*a_next[part]] = 
	    ref_node_aux(ref_node,i,node);
	a_next[part]++;
      }

  RSS( ref_mpi_alltoallv( a_global, a_size, b_global, b_size, 
			  1, REF_INT_TYPE ), 
       "alltoallv global");

  RSS( ref_mpi_alltoallv( a_real, a_size, b_real, b_size, 
			  REF_NODE_REAL_PER, REF_DBL_TYPE ), 
       "alltoallv global");

  if ( ref_node_naux(ref_node) > 0 )
    RSS( ref_mpi_alltoallv( a_aux, a_size, b_aux, b_size, 
			    ref_node_naux(ref_node), REF_DBL_TYPE ), 
	 "alltoallv global");

  RSS( ref_node_add_many( ref_node, b_total, b_global ), "add many" );

  for ( node=0; node < b_total; node++ )
    {
      RSS( ref_node_local( ref_node, b_global[node], &local ), "local" );
      for ( i=0; i < REF_NODE_REAL_PER ; i++ )
	ref_node_real(ref_node,i,local) = b_real[i+REF_NODE_REAL_PER*node];
      for ( i=0; i < ref_node_naux(ref_node) ; i++ )
	ref_node_aux(ref_node,i,local) = b_aux[i+ref_node_naux(ref_node)*node];
      ref_node_part(ref_node,local) = ref_mpi_id;
    }

  ref_free(a_next);
  ref_free(b_aux);
  ref_free(b_real);
  ref_free(b_global);
  ref_free(a_aux);
  ref_free(a_real);
  ref_free(a_global);
  ref_free(b_size);
  ref_free(a_size);

  return REF_SUCCESS;
}

REF_STATUS ref_migrate_shufflin_cell( REF_NODE ref_node, 
				      REF_CELL ref_cell )
{
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT all_parts[REF_CELL_MAX_SIZE_PER];
  REF_INT nunique;
  REF_INT unique_parts[REF_CELL_MAX_SIZE_PER];
  REF_INT *a_size, *b_size;
  REF_INT a_total, b_total;
  REF_INT part, node, cell, i;
  REF_INT *a_next;
  REF_INT *a_c2n, *b_c2n;
  REF_INT *a_parts, *b_parts;
  REF_BOOL need_to_keep;

  if ( 1 == ref_mpi_n ) return REF_SUCCESS;

  ref_malloc_init( a_size, ref_mpi_n, REF_INT, 0 );
  ref_malloc_init( b_size, ref_mpi_n, REF_INT, 0 );

  each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes)
    {
      for ( node=0; node < ref_cell_node_per(ref_cell); node++ )
	all_parts[node] = ref_node_part(ref_node,nodes[node]);
      RSS( ref_sort_unique_int( ref_cell_node_per(ref_cell), all_parts,
				&nunique, unique_parts ), "unique");
      for ( node=0; node < nunique; node++ )
	{
	  part = unique_parts[node];
	  if ( ref_mpi_id != part ) a_size[part]++;
	}
    }

  RSS( ref_mpi_alltoall( a_size, b_size, REF_INT_TYPE ), "alltoall sizes");

  a_total = 0;
  for ( part = 0; part<ref_mpi_n ; part++ )
    a_total += a_size[part];
  ref_malloc( a_c2n,   ref_cell_size_per(ref_cell)*a_total, REF_INT );
  ref_malloc( a_parts, ref_cell_size_per(ref_cell)*a_total, REF_INT );

  b_total = 0;
  for ( part = 0; part<ref_mpi_n ; part++ )
    b_total += b_size[part];
  ref_malloc( b_c2n,   ref_cell_size_per(ref_cell)*b_total, REF_INT );
  ref_malloc( b_parts, ref_cell_size_per(ref_cell)*b_total, REF_INT );

  ref_malloc( a_next, ref_mpi_n, REF_INT );
  a_next[0] = 0;
  for ( part = 1; part<ref_mpi_n ; part++ )
    a_next[part] = a_next[part-1]+a_size[part-1];

  each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes)
    {
      for ( node=0; node < ref_cell_node_per(ref_cell); node++ )
	all_parts[node] = ref_node_part(ref_node,nodes[node]);
      RSS( ref_sort_unique_int( ref_cell_node_per(ref_cell), all_parts,
				&nunique, unique_parts ), "unique");
      for ( node=0; node < nunique; node++ )
	{
	  part = unique_parts[node];
	  if ( ref_mpi_id != part ) 
	    {
	      for (i=0;i<ref_cell_node_per(ref_cell);i++)
		{
		  a_c2n[i+ref_cell_size_per(ref_cell)*a_next[part]] = 
		    ref_node_global(ref_node,nodes[i]);
		  a_parts[i+ref_cell_size_per(ref_cell)*a_next[part]] =
		    ref_node_part(ref_node,nodes[i]);
		}
	      if ( ref_cell_last_node_is_an_id(ref_cell) )
		a_c2n[ref_cell_node_per(ref_cell) + 
		      ref_cell_size_per(ref_cell)*a_next[part]] =
		  nodes[ref_cell_node_per(ref_cell)];
	      a_next[part]++;
	    }
	}
    }

  RSS( ref_mpi_alltoallv( a_c2n, a_size, b_c2n, b_size, 
			  ref_cell_size_per(ref_cell), REF_INT_TYPE ), 
       "alltoallv c2n");
  RSS( ref_mpi_alltoallv( a_parts, a_size, b_parts, b_size, 
			  ref_cell_size_per(ref_cell), REF_INT_TYPE ), 
       "alltoallv parts");

  RSS( ref_cell_add_many_global( ref_cell, ref_node,
				 b_total, 
				 b_c2n, b_parts, ref_mpi_id ), "many glob");

  free(a_next);
  free(b_parts);
  free(b_c2n);
  free(a_parts);
  free(a_c2n);
  free(b_size);
  free(a_size);

  each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes)
    {
      need_to_keep = REF_FALSE;
      for ( node=0; node < ref_cell_node_per(ref_cell); node++ )
	{
	  need_to_keep = ( need_to_keep || 
			   ref_mpi_id == ref_node_part(ref_node,nodes[node]) );
	}
      if ( ! need_to_keep )
	RSS( ref_cell_remove( ref_cell, cell), "remove" );
    }

  return REF_SUCCESS;
}

REF_STATUS ref_migrate_shufflin( REF_GRID ref_grid )
{
  REF_NODE ref_node = ref_grid_node( ref_grid );
  REF_CELL ref_cell;
  REF_INT group, node;
  REF_BOOL need_to_keep;

  if ( 1 == ref_mpi_n ) return REF_SUCCESS;

  RSS( ref_node_synchronize_globals( ref_node ), "sync global nodes");

  RSS( ref_migrate_shufflin_node( ref_node ), "send out nodes" );

  each_ref_grid_ref_cell( ref_grid, group, ref_cell )
    {
      RSS( ref_migrate_shufflin_cell( ref_node, ref_cell ), "cell" );
    }

  RSS( ref_migrate_shufflin_cell( ref_node, ref_grid_tri(ref_grid) ), "tri");
  RSS( ref_migrate_shufflin_cell( ref_node, ref_grid_qua(ref_grid) ), "qua");

  each_ref_node_valid_node( ref_node, node )
    if ( ref_mpi_id != ref_node_part(ref_node,node) )
      {
	need_to_keep = REF_FALSE;
	each_ref_grid_ref_cell( ref_grid, group, ref_cell )
	  need_to_keep = ( need_to_keep ||
			   !ref_adj_empty( ref_cell_adj(ref_cell), node ) );
	if ( !need_to_keep )
	  RSS( ref_node_remove_without_global( ref_node, node), "remove" );
      }
  RSS( ref_node_rebuild_sorted_global( ref_node ), "rebuild" );

  RSS( ref_node_ghost_real( ref_node ), "ghost real");

  return REF_SUCCESS;
}

