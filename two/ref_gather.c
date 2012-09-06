
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ref_gather.h"

#include "ref_endian.h"
#include "ref_malloc.h"
#include "ref_mpi.h"

REF_STATUS ref_gather_tec_part( REF_GRID ref_grid, char *filename  )
{
  FILE *file;
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT nnode,ntri;

  RSS( ref_node_synchronize_globals( ref_node ), "sync" );

  nnode = ref_node_n_global(ref_node);

  RSS( ref_gather_ncell( ref_node, ref_grid_tri(ref_grid), &ntri ), "ntri");

  file = NULL;
  if ( ref_mpi_master )
    {
      file = fopen(filename,"w");
      if (NULL == (void *)file) printf("unable to open %s\n",filename);
      RNS(file, "unable to open file" );

      fprintf(file, "title=\"tecplot refine partion file\"\n");
      fprintf(file, "variables = \"x\" \"y\" \"z\" \"p\"\n");
      fprintf(file,
	"zone t=part, nodes=%d, elements=%d, datapacking=%s, zonetype=%s\n",
	      nnode, ntri, "point", "fetriangle" );
    }

  RSS( ref_gather_node_tec_part( ref_node, file ), "nodes");
  RSS( ref_gather_cell_tec( ref_node, ref_grid_tri(ref_grid), file ), "nodes");

  if ( ref_mpi_master ) fclose(file);

  return REF_SUCCESS;
}

REF_STATUS ref_gather_b8_ugrid( REF_GRID ref_grid, char *filename  )
{
  FILE *file;
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT nnode,ntri,nqua,ntet,npyr,npri,nhex;
  REF_CELL ref_cell;
  REF_INT group;

  RSS( ref_node_synchronize_globals( ref_node ), "sync" );

  nnode = ref_node_n_global(ref_node);

  RSS( ref_gather_ncell( ref_node, ref_grid_tri(ref_grid), &ntri ), "ntri");
  RSS( ref_gather_ncell( ref_node, ref_grid_qua(ref_grid), &nqua ), "nqua");

  RSS( ref_gather_ncell( ref_node, ref_grid_tet(ref_grid), &ntet ), "ntet");
  RSS( ref_gather_ncell( ref_node, ref_grid_pyr(ref_grid), &npyr ), "npyr");
  RSS( ref_gather_ncell( ref_node, ref_grid_pri(ref_grid), &npri ), "npri");
  RSS( ref_gather_ncell( ref_node, ref_grid_hex(ref_grid), &nhex ), "nhex");
  
  file = NULL;
  if ( ref_mpi_master )
    {
      file = fopen(filename,"w");
      if (NULL == (void *)file) printf("unable to open %s\n",filename);
      RNS(file, "unable to open file" );

      SWAP_INT(nnode);
      SWAP_INT(ntri);
      SWAP_INT(nqua);
      SWAP_INT(ntet);
      SWAP_INT(npyr);
      SWAP_INT(npri);
      SWAP_INT(nhex);

      REIS(1, fwrite(&nnode,sizeof(REF_INT),1,file),"nnode");

      REIS(1, fwrite(&ntri,sizeof(REF_INT),1,file),"ntri");
      REIS(1, fwrite(&nqua,sizeof(REF_INT),1,file),"nqua");

      REIS(1, fwrite(&ntet,sizeof(REF_INT),1,file),"ntet");
      REIS(1, fwrite(&npyr,sizeof(REF_INT),1,file),"npyr");
      REIS(1, fwrite(&npri,sizeof(REF_INT),1,file),"npri");
      REIS(1, fwrite(&nhex,sizeof(REF_INT),1,file),"nhex");
    }

  RSS( ref_gather_node( ref_node, file ), "nodes");

  RSS( ref_gather_cell( ref_node,ref_grid_tri(ref_grid), 
			REF_FALSE, file ), "tri c2n");
  RSS( ref_gather_cell( ref_node,ref_grid_qua(ref_grid), 
			REF_FALSE, file ), "qua c2n");

  RSS( ref_gather_cell( ref_node,ref_grid_tri(ref_grid), 
			REF_TRUE, file ), "tri faceid");
  RSS( ref_gather_cell( ref_node,ref_grid_qua(ref_grid), 
			REF_TRUE, file ), "qua faceid");

  each_ref_grid_ref_cell( ref_grid, group, ref_cell )
    RSS( ref_gather_cell( ref_node, ref_cell, 
			  REF_FALSE, file ), "cell c2n");

  if ( ref_mpi_master ) fclose(file);

  return REF_SUCCESS;
}

REF_STATUS ref_gather_ncell( REF_NODE ref_node, REF_CELL ref_cell, 
			     REF_INT *ncell )
{
  REF_INT cell, nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT ncell_local;

  ncell_local = 0;
  each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes)
    if ( ref_mpi_id == ref_node_part(ref_node,nodes[0]) )
      ncell_local++;

  RSS( ref_mpi_sum( &ncell_local, ncell, 1, REF_INT_TYPE ), "sum");

  return REF_SUCCESS;
}

REF_STATUS ref_gather_node( REF_NODE ref_node, FILE *file )
{
  REF_INT chunk;
  REF_DBL *local_xyzm, *xyzm;
  REF_DBL swapped_dbl;
  REF_INT nnode_written, first, n, i;
  REF_INT global, local;
  REF_STATUS status;

  chunk = ref_node_n_global(ref_node)/ref_mpi_n + 1;

  ref_malloc( local_xyzm, 4*chunk, REF_DBL );
  ref_malloc( xyzm, 4*chunk, REF_DBL );

  nnode_written = 0;
  while ( nnode_written < ref_node_n_global(ref_node) )
    {

      first = nnode_written;
      n = MIN( chunk, ref_node_n_global(ref_node)-nnode_written );

      nnode_written += n;

      for (i=0;i<4*chunk;i++)
	local_xyzm[i] = 0.0;

      for (i=0;i<n;i++)
	{
	  global = first + i;
	  status = ref_node_local( ref_node, global, &local );
	  RXS( status, REF_NOT_FOUND, "node local failed" );
	  if ( REF_SUCCESS == status &&
	       ref_mpi_id == ref_node_part(ref_node,local) )
	    {
	      local_xyzm[0+4*i] = ref_node_xyz(ref_node,0,local);
	      local_xyzm[1+4*i] = ref_node_xyz(ref_node,1,local);
	      local_xyzm[2+4*i] = ref_node_xyz(ref_node,2,local);
	      local_xyzm[3+4*i] = 1.0;
	    }
	  else
	    {
	      local_xyzm[0+4*i] = 0.0;
	      local_xyzm[1+4*i] = 0.0;
	      local_xyzm[2+4*i] = 0.0;
	      local_xyzm[3+4*i] = 0.0;
	    }
	}

      RSS( ref_mpi_sum( local_xyzm, xyzm, 4*n, REF_DBL_TYPE ), "sum" );
      
      if ( ref_mpi_master )
	for ( i=0; i<n; i++ )
	  {
	    if ( ABS( xyzm[3+4*i] - 1.0 ) > 0.1 )
	      {
		printf("error gather node %d %f\n",first+i, xyzm[3+4*i]);
	      }
	    swapped_dbl = xyzm[0+4*i]; SWAP_DBL(swapped_dbl);
	    REIS(1, fwrite(&swapped_dbl,sizeof(REF_DBL),1,file),"x");
	    swapped_dbl = xyzm[1+4*i]; SWAP_DBL(swapped_dbl);
	    REIS(1, fwrite(&swapped_dbl,sizeof(REF_DBL),1,file),"y");
	    swapped_dbl = xyzm[2+4*i]; SWAP_DBL(swapped_dbl);
	    REIS(1, fwrite(&swapped_dbl,sizeof(REF_DBL),1,file),"z");
	  }
    }

  ref_free( xyzm );
  ref_free( local_xyzm );

  return REF_SUCCESS;
}
REF_STATUS ref_gather_node_tec_part( REF_NODE ref_node, FILE *file )
{
  REF_INT chunk;
  REF_DBL *local_xyzm, *xyzm;
  REF_INT nnode_written, first, n, i;
  REF_INT global, local;
  REF_STATUS status;

  chunk = ref_node_n_global(ref_node)/ref_mpi_n + 1;

  ref_malloc( local_xyzm, 5*chunk, REF_DBL );
  ref_malloc( xyzm, 5*chunk, REF_DBL );

  nnode_written = 0;
  while ( nnode_written < ref_node_n_global(ref_node) )
    {

      first = nnode_written;
      n = MIN( chunk, ref_node_n_global(ref_node)-nnode_written );

      nnode_written += n;

      for (i=0;i<5*chunk;i++)
	local_xyzm[i] = 0.0;

      for (i=0;i<n;i++)
	{
	  global = first + i;
	  status = ref_node_local( ref_node, global, &local );
	  RXS( status, REF_NOT_FOUND, "node local failed" );
	  if ( REF_SUCCESS == status &&
	       ref_mpi_id == ref_node_part(ref_node,local) )
	    {
	      local_xyzm[0+5*i] = ref_node_xyz(ref_node,0,local);
	      local_xyzm[1+5*i] = ref_node_xyz(ref_node,1,local);
	      local_xyzm[2+5*i] = ref_node_xyz(ref_node,2,local);
	      local_xyzm[3+5*i] = (REF_DBL)ref_node_part(ref_node,local);
	      local_xyzm[4+5*i] = 1.0;
	    }
	  else
	    {
	      local_xyzm[0+5*i] = 0.0;
	      local_xyzm[1+5*i] = 0.0;
	      local_xyzm[2+5*i] = 0.0;
	      local_xyzm[3+5*i] = 0.0;
	      local_xyzm[4+5*i] = 0.0;
	    }
	}

      RSS( ref_mpi_sum( local_xyzm, xyzm, 5*n, REF_DBL_TYPE ), "sum" );

      if ( ref_mpi_master )
	for ( i=0; i<n; i++ )
	  {
	    if ( ABS( xyzm[4+5*i] - 1.0 ) > 0.1 )
	      {
		printf("error gather node %d %f\n",first+i, xyzm[4+5*i]);
	      }
	    fprintf(file,"%.15e %.15e %.15e %.0f\n",
		    xyzm[0+5*i], xyzm[1+5*i], xyzm[2+5*i], xyzm[3+5*i] );
	  }
    }

  ref_free( xyzm );
  ref_free( local_xyzm );

  return REF_SUCCESS;
}

REF_STATUS ref_gather_cell( REF_NODE ref_node, REF_CELL ref_cell, 
			    REF_BOOL faceid_insted_of_c2n, FILE *file )
{
  REF_INT cell, node;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT node_per = ref_cell_node_per(ref_cell);
  REF_INT size_per = ref_cell_size_per(ref_cell);
  REF_INT ncell;
  REF_INT *c2n;
  REF_INT proc;

  if ( ref_mpi_master )
    {
      each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes)
	if ( ref_mpi_id == ref_node_part(ref_node,nodes[0]) )
	  {
	    if ( faceid_insted_of_c2n )
	      {
		for ( node = node_per; node < size_per; node++ )
                 {
                   SWAP_INT(nodes[node]);
                   REIS(1, fwrite(&(nodes[node]),sizeof(REF_INT),1,file),
                        "cel node");
                 }
	    }
	    else
	      {
		for ( node = 0; node < node_per; node++ )
		  {
		    nodes[node] = ref_node_global(ref_node,nodes[node]);
		    nodes[node]++;
		    SWAP_INT(nodes[node]);
		    REIS(1, fwrite(&(nodes[node]),sizeof(REF_INT),1,file),
			 "cel node");
		  }
	      }
	  }
    }

  if ( ref_mpi_master )
    {
      for (proc=1;proc<ref_mpi_n;proc++)
	{
	  RSS( ref_mpi_recv( &ncell, 1, REF_INT_TYPE, proc ), "recv ncell");
	  ref_malloc(c2n, ncell*size_per, REF_INT);
	  RSS( ref_mpi_recv( c2n, ncell*size_per, 
			     REF_INT_TYPE, proc ), "recv c2n");
	  for ( cell = 0; cell < ncell; cell++ )
	    if ( faceid_insted_of_c2n )
	      {
		for ( node = node_per; node < size_per; node++ )
		  {
		    SWAP_INT(c2n[node+size_per*cell]);
		    REIS(1, fwrite(&(c2n[node+size_per*cell]),
				   sizeof(REF_INT),1,file),"cell");
		  }
	      }
	    else
	      {
		for ( node = 0; node < node_per; node++ )
		  {
		    c2n[node+size_per*cell]++;
		    SWAP_INT(c2n[node+size_per*cell]);
		    REIS(1, fwrite(&(c2n[node+size_per*cell]),
				   sizeof(REF_INT),1,file),"cell");
		  }
	      }
	  ref_free(c2n);
	}
    }
  else
    {
      ncell = 0;
      each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes)
	if ( ref_mpi_id == ref_node_part(ref_node,nodes[0]) )
	  ncell++;
      RSS( ref_mpi_send( &ncell, 1, REF_INT_TYPE, 0 ), "send ncell");
      ref_malloc(c2n, ncell*size_per, REF_INT);
      ncell = 0;
      each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes)
	if ( ref_mpi_id == ref_node_part(ref_node,nodes[0]) )
	  {
	    for ( node = 0; node < node_per; node++ )
	      c2n[node+size_per*ncell] = ref_node_global(ref_node,nodes[node]);
	    for ( node = node_per; node < size_per; node++ )
	      c2n[node+size_per*ncell] = nodes[node];
	    ncell++;
	  }
      RSS( ref_mpi_send( c2n, ncell*size_per, 
			 REF_INT_TYPE, 0 ), "send c2n");

      ref_free(c2n);
    }

  return REF_SUCCESS;
}

REF_STATUS ref_gather_cell_tec( REF_NODE ref_node, REF_CELL ref_cell, 
				FILE *file )
{
  REF_INT cell, node;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_INT node_per = ref_cell_node_per(ref_cell);
  REF_INT size_per = ref_cell_size_per(ref_cell);
  REF_INT ncell;
  REF_INT *c2n;
  REF_INT proc;

  if ( ref_mpi_master )
    {
      each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes)
	if ( ref_mpi_id == ref_node_part(ref_node,nodes[0]) )
	  {
	    for ( node = 0; node < node_per; node++ )
	      {
		nodes[node] = ref_node_global(ref_node,nodes[node]);
		nodes[node]++;
		fprintf(file," %d",nodes[node]);
	      }
	    fprintf(file,"\n");
	  }
    }

  if ( ref_mpi_master )
    {
      for (proc=1;proc<ref_mpi_n;proc++)
	{
	  RSS( ref_mpi_recv( &ncell, 1, REF_INT_TYPE, proc ), "recv ncell");
	  ref_malloc(c2n, ncell*size_per, REF_INT);
	  RSS( ref_mpi_recv( c2n, ncell*size_per, 
			     REF_INT_TYPE, proc ), "recv c2n");
	  for ( cell = 0; cell < ncell; cell++ )
	    {
	      for ( node = 0; node < node_per; node++ )
		{
		  c2n[node+size_per*cell]++;
		  fprintf(file," %d",c2n[node+size_per*cell]);
		}
	      fprintf(file,"\n");
	    }
	  ref_free(c2n);
	}
    }
  else
    {
      ncell = 0;
      each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes)
	if ( ref_mpi_id == ref_node_part(ref_node,nodes[0]) )
	  ncell++;
      RSS( ref_mpi_send( &ncell, 1, REF_INT_TYPE, 0 ), "send ncell");
      ref_malloc(c2n, ncell*size_per, REF_INT);
      ncell = 0;
      each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes)
	if ( ref_mpi_id == ref_node_part(ref_node,nodes[0]) )
	  {
	    for ( node = 0; node < node_per; node++ )
	      c2n[node+size_per*ncell] = ref_node_global(ref_node,nodes[node]);
	    for ( node = node_per; node < size_per; node++ )
	      c2n[node+size_per*ncell] = nodes[node];
	    ncell++;
	  }
      RSS( ref_mpi_send( c2n, ncell*size_per, 
			 REF_INT_TYPE, 0 ), "send c2n");

      ref_free(c2n);
    }

  return REF_SUCCESS;
}
