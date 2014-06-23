#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ref_metric.h"

#include "ref_grid.h"
#include "ref_node.h"
#include "ref_cell.h"
#include "ref_list.h"
#include "ref_adj.h"
#include "ref_sort.h"
#include "ref_matrix.h"

#include "ref_fixture.h"
#include "ref_malloc.h"

#include "ref_mpi.h"

#include "ref_part.h"
#include "ref_import.h"
#include "ref_export.h"
#include "ref_migrate.h"
#include "ref_dict.h"

#include "ref_edge.h"
#include "ref_adapt.h"

#include "ref_histogram.h"
#include "ref_gather.h"
#include "ref_collapse.h"
#include "ref_split.h"
#include "ref_math.h"

int main(  int argc, char *argv[] )
{

  if ( argc > 2 )
    {
      REF_GRID ref_grid;

      RSS( ref_mpi_start( argc, argv ), "start" );

      RSS( ref_import_by_extension( &ref_grid, argv[1] ), "examine header" );

      RSS( ref_part_metric( ref_grid_node(ref_grid), argv[2] ), "get metric");

      RSS( ref_histogram_quality( ref_grid ), "qual");
      RSS( ref_histogram_ratio( ref_grid ), "rat");

      RSS( ref_export_tec_metric_ellipse( ref_grid, "ref_metric_test" ), "al");
      RSS( ref_export_tec_surf( ref_grid, "ref_metric_test_surf.tec" ), 
	   "surf");

      RSS( ref_metric_gradation( ref_grid ), "grad");
      RSS( ref_export_tec_metric_ellipse( ref_grid, "ref_metric_test_smooth" ), "al");

      RSS( ref_grid_free( ref_grid ), "free");

      RSS( ref_mpi_stop( ), "stop" );
    }

  {  /* imply metric right tet */
    REF_DBL tol = -1.0;
    REF_GRID ref_grid;
    REF_DBL *metric;
    REF_INT node;

    RSS( ref_fixture_tet_grid( &ref_grid ), "tet" );

    ref_malloc( metric, 6*ref_node_max(ref_grid_node(ref_grid)), REF_DBL );

    RSS( ref_metric_imply_from( metric, ref_grid ), "imply" );

    each_ref_node_valid_node( ref_grid_node(ref_grid), node )
      {
	RWDS( 1.0, metric[0+6*node], tol, "m[0]");
	RWDS( 0.5, metric[1+6*node], tol, "m[1]");
	RWDS( 0.5, metric[2+6*node], tol, "m[2]");
	RWDS( 1.0, metric[3+6*node], tol, "m[3]");
	RWDS( 0.5, metric[4+6*node], tol, "m[4]");
	RWDS( 1.0, metric[5+6*node], tol, "m[5]");
      }

    ref_free( metric );

    RSS(ref_grid_free(ref_grid),"free"); 
  }

  {  /* imply metric right prism */
    REF_DBL tol = 0.00001;
    REF_GRID ref_grid;
    REF_DBL *metric;
    REF_INT node;

    RSS( ref_fixture_pri_grid( &ref_grid ), "tet" );

    ref_malloc( metric, 6*ref_node_max(ref_grid_node(ref_grid)), REF_DBL );

    RSS( ref_metric_imply_from( metric, ref_grid ), "imply" );

    each_ref_node_valid_node( ref_grid_node(ref_grid), node )
      {
	RWDS( 1.259596e+00, metric[0+6*node], tol, "m[0]");
	RWDS(-6.394720e-01, metric[1+6*node], tol, "m[1]");
	RWDS( 6.394720e-01, metric[2+6*node], tol, "m[2]");
	RWDS( 9.546890e-01, metric[3+6*node], tol, "m[3]");
	RWDS(-3.247285e-01, metric[4+6*node], tol, "m[4]");
	RWDS( 9.546890e-01, metric[5+6*node], tol, "m[5]");
      }

    ref_free( metric );

    RSS(ref_grid_free(ref_grid),"free"); 
  }

  {  /* imply metric non tet prism */
    REF_DBL tol = 0.00001;
    REF_GRID ref_grid;
    REF_DBL *metric;
    REF_INT node;

    RSS( ref_fixture_pri_grid( &ref_grid ), "tet" );

    ref_malloc( metric, 6*ref_node_max(ref_grid_node(ref_grid)), REF_DBL );

    RSS( ref_metric_imply_non_tet( metric, ref_grid ), "imply" );

    each_ref_node_valid_node( ref_grid_node(ref_grid), node )
      {
	RWDS( 1.259596e+00, metric[0+6*node], tol, "m[0]");
	RWDS(-6.394720e-01, metric[1+6*node], tol, "m[1]");
	RWDS( 6.394720e-01, metric[2+6*node], tol, "m[2]");
	RWDS( 9.546890e-01, metric[3+6*node], tol, "m[3]");
	RWDS(-3.247285e-01, metric[4+6*node], tol, "m[4]");
	RWDS( 9.546890e-01, metric[5+6*node], tol, "m[5]");
      }

    ref_free( metric );

    RSS(ref_grid_free(ref_grid),"free"); 
  }

  {  /* imply metric tet prism */
    REF_DBL tol = 0.00001;
    REF_GRID ref_grid;
    REF_DBL *metric;
    REF_INT node;

    RSS( ref_fixture_pri_tet_cap_grid( &ref_grid ), "tet" );

    ref_malloc( metric, 6*ref_node_max(ref_grid_node(ref_grid)), REF_DBL );

    RSS( ref_metric_imply_from( metric, ref_grid ), "imply" );

    node = 6;
    RWDS( 1.00, metric[0+6*node], tol, "m[0]");
    RWDS( 0.50, metric[1+6*node], tol, "m[1]");
    RWDS( 0.05, metric[2+6*node], tol, "m[2]");
    RWDS( 1.00, metric[3+6*node], tol, "m[3]");
    RWDS( 0.05, metric[4+6*node], tol, "m[4]");
    RWDS( 0.67, metric[5+6*node], tol, "m[5]");

    RSS( ref_metric_imply_non_tet( metric, ref_grid ), "imply" );

    node = 6;
    RWDS( 1.00, metric[0+6*node], tol, "m[0]");
    RWDS( 0.50, metric[1+6*node], tol, "m[1]");
    RWDS( 0.05, metric[2+6*node], tol, "m[2]");
    RWDS( 1.00, metric[3+6*node], tol, "m[3]");
    RWDS( 0.05, metric[4+6*node], tol, "m[4]");
    RWDS( 0.67, metric[5+6*node], tol, "m[5]");

    ref_free( metric );

    RSS(ref_grid_free(ref_grid),"free"); 
  }

  {  /* imply metric pyr */
    REF_DBL tol = 0.00001;
    REF_GRID ref_grid;
    REF_DBL *metric;
    REF_INT node;

    RSS( ref_fixture_pyr_grid( &ref_grid ), "tet" );

    ref_malloc( metric, 6*ref_node_max(ref_grid_node(ref_grid)), REF_DBL );

    RSS( ref_metric_imply_from( metric, ref_grid ), "imply" );

    node = 2;
    RWDS( 9.690913e-01, metric[0+6*node], tol, "m[0]");
    RWDS( 2.525674e-01, metric[1+6*node], tol, "m[1]");
    RWDS(-4.689688e-01, metric[2+6*node], tol, "m[2]");
    RWDS( 9.503095e-01, metric[3+6*node], tol, "m[3]");
    RWDS( 2.525674e-01, metric[4+6*node], tol, "m[4]");
    RWDS( 9.690913e-01, metric[5+6*node], tol, "m[5]");

    RSS( ref_metric_imply_non_tet( metric, ref_grid ), "imply" );

    node = 2;
    RWDS( 9.690913e-01, metric[0+6*node], tol, "m[0]");
    RWDS( 2.525674e-01, metric[1+6*node], tol, "m[1]");
    RWDS(-4.689688e-01, metric[2+6*node], tol, "m[2]");
    RWDS( 9.503095e-01, metric[3+6*node], tol, "m[3]");
    RWDS( 2.525674e-01, metric[4+6*node], tol, "m[4]");
    RWDS( 9.690913e-01, metric[5+6*node], tol, "m[5]");

    ref_free( metric );

    RSS(ref_grid_free(ref_grid),"free"); 
  }

  {  /* imply metric hex */
    REF_DBL tol = 0.00001;
    REF_GRID ref_grid;
    REF_DBL *metric;
    REF_INT node;

    RSS( ref_fixture_hex_grid( &ref_grid ), "tet" );

    ref_malloc( metric, 6*ref_node_max(ref_grid_node(ref_grid)), REF_DBL );

    RSS( ref_metric_imply_from( metric, ref_grid ), "imply" );

    node = 0;
    RWDS( 7.854306e-01, metric[0+6*node], tol, "m[0]");
    RWDS( 1.331808e-01, metric[1+6*node], tol, "m[1]");
    RWDS( 1.331808e-01, metric[2+6*node], tol, "m[2]");
    RWDS( 9.960985e-01, metric[3+6*node], tol, "m[3]");
    RWDS(-5.352162e-01, metric[4+6*node], tol, "m[4]");
    RWDS( 9.960985e-01, metric[5+6*node], tol, "m[5]");

    RSS( ref_metric_imply_non_tet( metric, ref_grid ), "imply" );

    node = 0;
    RWDS( 7.854306e-01, metric[0+6*node], tol, "m[0]");
    RWDS( 1.331808e-01, metric[1+6*node], tol, "m[1]");
    RWDS( 1.331808e-01, metric[2+6*node], tol, "m[2]");
    RWDS( 9.960985e-01, metric[3+6*node], tol, "m[3]");
    RWDS(-5.352162e-01, metric[4+6*node], tol, "m[4]");
    RWDS( 9.960985e-01, metric[5+6*node], tol, "m[5]");

    ref_free( metric );

    RSS(ref_grid_free(ref_grid),"free"); 
  }

  { /* smr octave */
    REF_DBL tol = -1.0;
    REF_GRID ref_grid;
    REF_DBL *metric_file;
    REF_DBL *metric_imply;
    REF_DBL *metric;
    REF_INT node;

  /*
clear all
format long
a1 = [ 1 0 0 ;
       0 1 0 ;
       0 0 1 ]
a2 = [ 30 -25   -2.5;
      -25  25   1;
      -2.5   1  1.5];
a3 = inv(a1)*a2
[vector3, value3] = eig(a3)

val1 = vector3'*a1*vector3
val2 = vector3'*a2*vector3
for ii=1:3
 h1(ii,ii) = sqrt(1/val1(ii,ii));
 h2(ii,ii) = sqrt(1/val2(ii,ii));
 h(ii,ii) = max(0.25*h1(ii,ii),min(4.0*h1(ii,ii),h2(ii,ii)));
 val(ii,ii)=1.0 / (h(ii,ii)* h(ii,ii));
end
h1
h2
h
val
vector = inv(vector3)

smr = vector'*val*vector
[sv,se]= eig(smr)
se.^-0.5
  */

    RSS( ref_grid_create( &ref_grid ), "create grid" );
    RSS( ref_node_add( ref_grid_node(ref_grid), 0, &node ), "add" );

    ref_malloc( metric_file, 
		6*ref_node_max(ref_grid_node(ref_grid)), REF_DBL );
    ref_malloc( metric_imply, 
		6*ref_node_max(ref_grid_node(ref_grid)), REF_DBL );
    ref_malloc( metric, 
		6*ref_node_max(ref_grid_node(ref_grid)), REF_DBL );

    metric_imply[0+6*node] = 1.0;
    metric_imply[1+6*node] = 0.0;
    metric_imply[2+6*node] = 0.0;
    metric_imply[3+6*node] = 1.0;
    metric_imply[4+6*node] = 0.0;
    metric_imply[5+6*node] = 1.0;

    metric_file[0+6*node] =  30.0;
    metric_file[1+6*node] = -25.0;
    metric_file[2+6*node] =  -2.5;
    metric_file[3+6*node] =  25.0;
    metric_file[4+6*node] =   1.0;
    metric_file[5+6*node] =   1.5;

    RSS( ref_metric_smr( metric_imply, metric_file, metric, ref_grid ), "smr" );

    RWDS(  9.812655244359012, metric[0+6*node], tol, "m[0]");    
    RWDS( -6.764301991389758, metric[1+6*node], tol, "m[0]");    
    RWDS( -1.159409438169853, metric[2+6*node], tol, "m[0]");    
    RWDS(  8.527269886828027, metric[3+6*node], tol, "m[0]");    
    RWDS( -0.210986632201670, metric[4+6*node], tol, "m[0]");    
    RWDS(  1.410974767795262, metric[5+6*node], tol, "m[0]");    

    ref_free( metric );
    ref_free( metric_imply );
    ref_free( metric_file );
    RSS( ref_grid_free( ref_grid ), "free");

  }

  return 0;
}
