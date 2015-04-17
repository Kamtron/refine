
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "ref_histogram.h"
#include "ref_malloc.h"
#include "ref_edge.h"
#include "ref_mpi.h"

#include "ref_adapt.h"

REF_STATUS ref_histogram_create( REF_HISTOGRAM *ref_histogram_ptr )
{
  REF_HISTOGRAM ref_histogram;

  ref_malloc( *ref_histogram_ptr, 1, REF_HISTOGRAM_STRUCT );
  ref_histogram = (*ref_histogram_ptr);

  ref_histogram_nbin(ref_histogram) = 21;

  ref_malloc_init( ref_histogram->bins, 
		   ref_histogram_nbin(ref_histogram), REF_INT, 0 );

  ref_histogram_max(ref_histogram) = -1.0e20;
  ref_histogram_min(ref_histogram) =  1.0e20;
  ref_histogram_log_total(ref_histogram) = 0.0;
  ref_histogram_log_mean(ref_histogram)  = 0.0;

  return REF_SUCCESS;
}

REF_STATUS ref_histogram_free( REF_HISTOGRAM ref_histogram )
{
  if ( NULL == (void *)ref_histogram ) return REF_NULL;
  ref_free( ref_histogram->bins );
  ref_free( ref_histogram );
  return REF_SUCCESS;
}

REF_STATUS ref_histogram_add( REF_HISTOGRAM ref_histogram, REF_DBL observation )
{
  REF_INT i;

  if ( observation <= 0.0 ) return REF_INVALID;

  ref_histogram_max(ref_histogram) = 
    MAX(ref_histogram_max(ref_histogram),observation);
  ref_histogram_min(ref_histogram) = 
    MIN(ref_histogram_min(ref_histogram),observation);
  ref_histogram_log_total(ref_histogram) += log2(observation);

  i = ref_histogram_to_bin(observation);
  i = MIN(i,ref_histogram_nbin(ref_histogram)-1);
  i = MAX(i,0);

  ref_histogram_bin( ref_histogram, i )++;

  /*
    printf("%f:%f:%f\n",
    ref_histogram_to_obs(i),
    observation,
    ref_histogram_to_obs(i-1));
  */

  return REF_SUCCESS;
}

REF_STATUS ref_histogram_gather( REF_HISTOGRAM ref_histogram )
{
  REF_INT *bins;
  REF_DBL min, max, log_total;
  REF_INT i, observations;

  ref_malloc( bins, ref_histogram_nbin(ref_histogram), REF_INT );

  RSS( ref_mpi_sum( ref_histogram->bins, bins, 
		    ref_histogram_nbin(ref_histogram), REF_INT_TYPE ), "sum" );
  RSS( ref_mpi_max( &ref_histogram_max( ref_histogram ), &max, 
		    REF_DBL_TYPE ), "max" );
  RSS( ref_mpi_min( &ref_histogram_min( ref_histogram ), &min, 
		    REF_DBL_TYPE ), "min" );
  RSS( ref_mpi_sum( &ref_histogram_log_total( ref_histogram ), &log_total, 
		    1, REF_DBL_TYPE ), "log_total" );

  if ( ref_mpi_master )
    {
      ref_histogram_max( ref_histogram ) = max;
      ref_histogram_min( ref_histogram ) = min;
      ref_histogram_log_total( ref_histogram ) = log_total;
      for ( i=0;i<ref_histogram_nbin(ref_histogram);i++ )
	ref_histogram->bins[i] = bins[i];
      observations = 0;
      for ( i=0;i<ref_histogram_nbin(ref_histogram);i++ )
	observations += ref_histogram->bins[i];
      ref_histogram_log_mean( ref_histogram ) = 0.0;
      if ( observations > 0 ) 
	ref_histogram_log_mean( ref_histogram ) = 
	  log_total / (REF_DBL)observations;
    }
  else
    {
      ref_histogram_max( ref_histogram ) = -1.0e20;
      ref_histogram_min( ref_histogram ) =  1.0e20;
      ref_histogram_log_total( ref_histogram ) = 0.0;
      for ( i=0;i<ref_histogram_nbin(ref_histogram);i++ )
	ref_histogram->bins[i] = 0;
      ref_histogram_log_mean( ref_histogram ) = 0.0;
    }

  RSS( ref_mpi_bcast( &ref_histogram_log_mean( ref_histogram ), 
		      1, REF_DBL_TYPE ), "log_mean" );

  ref_free( bins );

  return REF_SUCCESS;
}

REF_STATUS ref_histogram_print( REF_HISTOGRAM ref_histogram, 
				char *description )
{
  REF_INT i, sum;
  REF_DBL log_mean;

  sum = 0;
  for (i=0;i<ref_histogram_nbin(ref_histogram);i++)
    sum += ref_histogram_bin( ref_histogram, i );

  printf("%7.3f min %s\n", ref_histogram_min( ref_histogram ), description );

  for (i=0;i<ref_histogram_nbin(ref_histogram);i++)
    if ( ref_histogram_to_obs(i+1) > ref_histogram_min( ref_histogram ) &&
	 ref_histogram_to_obs(i-1) < ref_histogram_max( ref_histogram ) )
      {
	if ( ( ref_histogram_to_obs(i)   > ref_adapt_split_ratio ||
	       ref_histogram_to_obs(i-1) < ref_adapt_collapse_ratio ) &&
	     ref_histogram_bin( ref_histogram, i ) > 0 )
	  {
	    printf("%7.3f:%10d *\n", 
		   ref_histogram_to_obs(i),
		   ref_histogram_bin( ref_histogram, i ));
	  }
	else
	  {
	    printf("%7.3f:%10d\n", 
		   ref_histogram_to_obs(i),
		   ref_histogram_bin( ref_histogram, i ));
	  }
      }

  printf("%7.3f:%10d max\n", ref_histogram_max( ref_histogram ), sum);
  log_mean = ref_histogram_log_total( ref_histogram ) / (REF_DBL)sum;
  printf("%18.10f mean\n", pow(2.0,log_mean));

  return REF_SUCCESS;
}

REF_STATUS ref_histogram_export( REF_HISTOGRAM ref_histogram, 
				 char *description )
{
  REF_INT i;
  FILE *f;
  char filename[1024];

  sprintf(filename,"ref_histogram_%s.gnuplot",description);
  f = fopen(filename,"w");
  if (NULL == (void *)f) printf("unable to open %s\n",filename);
  RNS(f, "unable to open file" );

  fprintf(f,"set terminal postscript eps enhanced color\n");
  sprintf(filename,"ref_histogram_%s.eps",description);
  fprintf(f,"set output '%s'\n",filename);

  fprintf(f,"set style data histogram\n");
  fprintf(f,"set style histogram cluster gap 1\n");
  fprintf(f,"set style fill solid border -1\n");
  fprintf(f,"set boxwidth 0.9\n");
  fprintf(f,"set xtic rotate by -45 scale 0\n");
  fprintf(f,"plot '-' using 2:xticlabels(1) title '%s'\n",description);

  for (i=0;i<ref_histogram_nbin(ref_histogram)-1;i++)
    {
      fprintf(f,"%.3f-%.3f %d\n", 
	      ref_histogram_to_obs(i),
	      ref_histogram_to_obs(i+1),
	      ref_histogram_bin( ref_histogram, i ));
    }

  fclose(f);

  return REF_SUCCESS;
}

REF_STATUS ref_histogram_ratio( REF_GRID ref_grid )
{
  REF_HISTOGRAM ref_histogram;
  REF_EDGE ref_edge;
  REF_INT edge, part;
  REF_DBL ratio;
  REF_BOOL active;

  RSS( ref_histogram_create(&ref_histogram),"create");
  RSS( ref_edge_create( &ref_edge, ref_grid ), "make edges" );

  for (edge=0;edge< ref_edge_n(ref_edge);edge++)
    {
      RSS( ref_edge_part( ref_edge, edge, &part ), "edge part");
      RSS( ref_node_edge_twod( ref_grid_node(ref_grid), 
			       ref_edge_e2n(ref_edge, 0, edge),
			       ref_edge_e2n(ref_edge, 1, edge), 
			       &active ), "twod edge");
      active = ( active || !ref_grid_twod(ref_grid) );
      if ( part == ref_mpi_id && active )
	{
	  RSS( ref_node_ratio( ref_grid_node(ref_grid), 
			       ref_edge_e2n(ref_edge, 0, edge),
			       ref_edge_e2n(ref_edge, 1, edge), 
			       &ratio ), "rat");
	  RSS( ref_histogram_add( ref_histogram, ratio ), "add");
	}
    }

  RSS( ref_histogram_gather( ref_histogram ), "gather");
  if ( ref_mpi_master ) RSS( ref_histogram_print( ref_histogram,
						  "edge ratio"), "print");
  if ( 1 == ref_mpi_n ) RSS( ref_histogram_export( ref_histogram,
						   "edge-ratio"), "print");

  RSS( ref_edge_free(ref_edge), "free edge" );
  RSS( ref_histogram_free(ref_histogram), "free gram" );
  return REF_SUCCESS;
}

REF_STATUS ref_histogram_quality( REF_GRID ref_grid )
{
  REF_HISTOGRAM ref_histogram;
  REF_CELL ref_cell;
  REF_INT cell;
  REF_INT nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL quality;

  RSS( ref_histogram_create(&ref_histogram),"create");

  if ( ref_grid_twod(ref_grid) )
    {
      ref_cell = ref_grid_tri(ref_grid);
    }
  else
    {
      ref_cell = ref_grid_tet(ref_grid);
    }
  each_ref_cell_valid_cell_with_nodes( ref_cell, cell, nodes)
    {
      if ( ref_node_part(ref_grid_node(ref_grid),nodes[0]) == ref_mpi_id )
	{
	  if ( ref_grid_twod(ref_grid) )
	    {
	      RSS( ref_node_tri_quality( ref_grid_node(ref_grid),
					 nodes,&quality ), "qual");
	    }
	  else
	    {
	      RSS( ref_node_tet_quality( ref_grid_node(ref_grid),
					 nodes,&quality ), "qual");
	    }
	  RSS( ref_histogram_add( ref_histogram, quality ), "add");
	}
    }

  RSS( ref_histogram_gather( ref_histogram ), "gather");
  if ( ref_mpi_master ) RSS( ref_histogram_print( ref_histogram,
						  "quality"), "print");

  RSS( ref_histogram_free(ref_histogram), "free gram" );
  return REF_SUCCESS;
}

REF_STATUS ref_histogram_tec_ratio( REF_GRID ref_grid )
{
  REF_EDGE ref_edge;
  REF_INT edge, part;
  REF_DBL ratio;
  REF_BOOL active;
  REF_INT n, node;
  FILE *file;
  char filename[1024];

  RSS( ref_edge_create( &ref_edge, ref_grid ), "make edges" );

  n = 0;
  for (edge=0;edge< ref_edge_n(ref_edge);edge++)
    {
      RSS( ref_edge_part( ref_edge, edge, &part ), "edge part");
      RSS( ref_node_edge_twod( ref_grid_node(ref_grid), 
			       ref_edge_e2n(ref_edge, 0, edge),
			       ref_edge_e2n(ref_edge, 1, edge), 
			       &active ), "twod edge");
      active = ( active || !ref_grid_twod(ref_grid) );
      if ( part == ref_mpi_id && active )
	{
	  RSS( ref_node_ratio( ref_grid_node(ref_grid), 
			       ref_edge_e2n(ref_edge, 0, edge),
			       ref_edge_e2n(ref_edge, 1, edge), 
			       &ratio ), "rat");
	  if ( ratio > ref_adapt_split_ratio ) n++;
	}
    }

  if ( 1 == ref_mpi_n )
    {
      sprintf(filename,"ref_histogram_edge.tec");
    }
  else
    {
      sprintf(filename,"ref_histogram_%d_edge.tec",ref_mpi_id);
    }
  file = fopen(filename,"w");
  if (NULL == (void *)file) printf("unable to open %s\n",filename);
  RNS(file, "unable to open file" );

  fprintf(file, "title=\"tecplot refine long edges\"\n");
  fprintf(file, "variables = \"x\" \"y\" \"z\" \"r\"\n");

  fprintf(file,
	  "zone t=scalar, nodes=%d, elements=%d, datapacking=%s, zonetype=%s\n",
	  2*n, n, "point", "felineseg" );

  for (edge=0;edge< ref_edge_n(ref_edge);edge++)
    {
      RSS( ref_edge_part( ref_edge, edge, &part ), "edge part");
      RSS( ref_node_edge_twod( ref_grid_node(ref_grid), 
			       ref_edge_e2n(ref_edge, 0, edge),
			       ref_edge_e2n(ref_edge, 1, edge), 
			       &active ), "twod edge");
      active = ( active || !ref_grid_twod(ref_grid) );
      if ( part == ref_mpi_id && active )
	{
	  RSS( ref_node_ratio( ref_grid_node(ref_grid), 
			       ref_edge_e2n(ref_edge, 0, edge),
			       ref_edge_e2n(ref_edge, 1, edge), 
			       &ratio ), "rat");
	  if ( ratio > ref_adapt_split_ratio ) 
	    {
	      node = ref_edge_e2n(ref_edge, 0, edge);
	      fprintf(file, " %.16e %.16e %.16e %.16e\n", 
		      ref_node_xyz(ref_grid_node(ref_grid),0,node),
		      ref_node_xyz(ref_grid_node(ref_grid),1,node),
		      ref_node_xyz(ref_grid_node(ref_grid),2,node),
		      ratio ) ;
	      node = ref_edge_e2n(ref_edge, 1, edge);
	      fprintf(file, " %.16e %.16e %.16e %.16e\n", 
		      ref_node_xyz(ref_grid_node(ref_grid),0,node),
		      ref_node_xyz(ref_grid_node(ref_grid),1,node),
		      ref_node_xyz(ref_grid_node(ref_grid),2,node),
		      ratio ) ;
	    }
	}
    }

  for ( edge = 0; edge < n; edge++ )
    fprintf(file," %d %d\n",1+2*edge,2+2*edge);

  fclose(file);
  RSS( ref_edge_free(ref_edge), "free edge" );

  return REF_SUCCESS;
}
