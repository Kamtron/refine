
#include <stdlib.h>
#include <stdio.h>

#include "ref_grid.h"

REF_STATUS ref_grid_create( REF_GRID *ref_grid_ptr )
{
  REF_GRID ref_grid;
  (*ref_grid_ptr) = NULL;
  (*ref_grid_ptr) = (REF_GRID)malloc( sizeof(REF_GRID_STRUCT) );
  RNS(*ref_grid_ptr,"malloc ref_grid NULL");

  ref_grid = *ref_grid_ptr;

  RSS( ref_node_create( &ref_grid_node(ref_grid) ), "node create" );
  RSS( ref_metric_create( &ref_grid_metric(ref_grid) ), "metric create" );

  RSS( ref_cell_create( &ref_grid_tet(ref_grid), 4, REF_FALSE ), "tet create" );
  RSS( ref_cell_create( &ref_grid_pyr(ref_grid), 5, REF_FALSE ), "pyr create" );
  RSS( ref_cell_create( &ref_grid_pri(ref_grid), 6, REF_FALSE ), "pri create" );
  RSS( ref_cell_create( &ref_grid_hex(ref_grid), 8, REF_FALSE ), "hex create" );

  ref_grid->cell[4] = NULL;

  RSS( ref_cell_create( &ref_grid_tri(ref_grid), 4, REF_TRUE ), "tri create" );
  RSS( ref_cell_create( &ref_grid_qua(ref_grid), 5, REF_TRUE ), "qua create" );

  return REF_SUCCESS;
}

REF_STATUS ref_grid_free( REF_GRID ref_grid )
{
  if ( NULL == (void *)ref_grid ) return REF_NULL;

  RSS( ref_node_free( ref_grid_node(ref_grid) ), "node free");
  RSS( ref_metric_free( ref_grid_metric(ref_grid) ), "metric free");

  RSS( ref_cell_free( ref_grid_tet(ref_grid) ), "tet free");
  RSS( ref_cell_free( ref_grid_pyr(ref_grid) ), "pyr free");
  RSS( ref_cell_free( ref_grid_pri(ref_grid) ), "pri free");
  RSS( ref_cell_free( ref_grid_hex(ref_grid) ), "hex free");

  RSS( ref_cell_free( ref_grid_tri(ref_grid) ), "tri free");
  RSS( ref_cell_free( ref_grid_qua(ref_grid) ), "qua free");

  ref_cond_free( ref_grid );
  return REF_SUCCESS;
}

REF_STATUS ref_grid_inspect( REF_GRID ref_grid )
{
  printf("ref_grid = %p\n",(void *)ref_grid);
  printf(" tet = %p\n",(void *)ref_grid_tet(ref_grid));
  printf(" pyr = %p\n",(void *)ref_grid_pyr(ref_grid));
  printf(" pri = %p\n",(void *)ref_grid_pri(ref_grid));
  printf(" hex = %p\n",(void *)ref_grid_hex(ref_grid));

  return REF_SUCCESS;
}
