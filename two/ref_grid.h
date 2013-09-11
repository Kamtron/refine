
#ifndef REF_GRID_H
#define REF_GRID_H

#include "ref_defs.h"

BEGIN_C_DECLORATION
typedef struct REF_GRID_STRUCT REF_GRID_STRUCT;
typedef REF_GRID_STRUCT * REF_GRID;
END_C_DECLORATION

#include "ref_node.h"
#include "ref_cell.h"

BEGIN_C_DECLORATION

struct REF_GRID_STRUCT {
  REF_NODE node;

  REF_CELL cell[5];

  REF_CELL tri;
  REF_CELL qua;

  REF_BOOL twod;
};

REF_STATUS ref_grid_create( REF_GRID *ref_grid );
REF_STATUS ref_grid_free( REF_GRID ref_grid );

REF_STATUS ref_grid_empty_cell_clone( REF_GRID *ref_grid, REF_GRID parent );
REF_STATUS ref_grid_free_cell_clone( REF_GRID ref_grid );

#define ref_grid_node(ref_grid) ((ref_grid)->node)
#define ref_grid_cell(ref_grid,group) ((ref_grid)->cell[(group)])

#define ref_grid_tet(ref_grid) ref_grid_cell(ref_grid,0)
#define ref_grid_pyr(ref_grid) ref_grid_cell(ref_grid,1)
#define ref_grid_pri(ref_grid) ref_grid_cell(ref_grid,2)
#define ref_grid_hex(ref_grid) ref_grid_cell(ref_grid,3)

#define ref_grid_tri(ref_grid) ((ref_grid)->tri)
#define ref_grid_qua(ref_grid) ((ref_grid)->qua)

#define ref_grid_twod(ref_grid) ((ref_grid)->twod)

#define each_ref_grid_ref_cell( ref_grid, group, ref_cell )		\
  for ( (group) = 0, (ref_cell) = ref_grid_cell(ref_grid,group) ;	\
	(group) < 4;							\
	(group)++  , (ref_cell) = ref_grid_cell(ref_grid,group) )

REF_STATUS ref_grid_inspect( REF_GRID ref_grid );

REF_STATUS ref_grid_cell_with( REF_GRID ref_grid, REF_INT node_per,
			       REF_CELL *ref_cell );
REF_STATUS ref_grid_face_with( REF_GRID ref_grid, REF_INT node_per,
			       REF_CELL *ref_cell );

REF_STATUS ref_grid_cell_has_face( REF_GRID ref_grid, 
				   REF_INT *face_nodes,
				   REF_BOOL *has_face );

REF_STATUS ref_grid_boundary_nodes( REF_GRID ref_grid, 
				    REF_INT boundary_tag, 
				    REF_INT *nnode, REF_INT *nface, 
				    REF_INT **g2l, REF_INT **l2g );

REF_STATUS ref_grid_replace_node( REF_GRID ref_grid, 
				  REF_INT old_node, REF_INT new_node );

END_C_DECLORATION

#endif /* REF_GRID_H */
