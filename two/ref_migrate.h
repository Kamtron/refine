
#ifndef REF_MIGRATE_H
#define REF_MIGRATE_H

#include "ref_defs.h"

BEGIN_C_DECLORATION
typedef struct REF_MIGRATE_STRUCT REF_MIGRATE_STRUCT;
typedef REF_MIGRATE_STRUCT * REF_MIGRATE;
END_C_DECLORATION

#include "ref_adj.h"
#include "ref_grid.h"

BEGIN_C_DECLORATION

struct REF_MIGRATE_STRUCT {
  REF_GRID grid;
  REF_ADJ parent_global;
  REF_ADJ parent_part;
  REF_ADJ conn;
  REF_INT max;
  REF_INT *global;
  REF_DBL *xyz;
  REF_DBL *weight;
};

extern REF_INT ref_migrate_method;
#define REF_MIGRATE_GRAPH (0)
#define REF_MIGRATE_RCB   (1)

#define ref_migrate_grid( ref_migrate ) ((ref_migrate)->grid)
#define ref_migrate_parent_global( ref_migrate ) ((ref_migrate)->parent_global)
#define ref_migrate_parent_part( ref_migrate ) ((ref_migrate)->parent_part)
#define ref_migrate_conn( ref_migrate ) ((ref_migrate)->conn)

#define ref_migrate_max( ref_migrate ) ((ref_migrate)->max)

#define ref_migrate_global( ref_migrate, node ) ((ref_migrate)->global[(node)])
#define ref_migrate_valid( ref_migrate, node )			\
  (REF_EMPTY != ref_migrate_global( ref_migrate, node ))

#define ref_migrate_xyz( ref_migrate, ixyz, node ) \
  ((ref_migrate)->xyz[(ixyz)+3*(node)])
#define ref_migrate_weight( ref_migrate, node ) ((ref_migrate)->weight[(node)])

#define each_ref_migrate_node( ref_migrate, node )			\
  for ( (node) = 0 ;							\
	(node) < ref_migrate_max(ref_migrate);				\
	(node)++ )							\
    if ( ref_migrate_valid( ref_migrate, node ) )

REF_STATUS ref_migrate_create( REF_MIGRATE *ref_migrate, REF_GRID ref_grid );
REF_STATUS ref_migrate_free( REF_MIGRATE ref_migrate );

REF_STATUS ref_migrate_inspect( REF_MIGRATE ref_migrate );

REF_STATUS ref_migrate_2d_agglomeration_keep( REF_MIGRATE ref_migrate,
					      REF_INT keep, REF_INT lose);

REF_STATUS ref_migrate_to_balance( REF_GRID ref_grid );

REF_STATUS ref_migrate_new_part( REF_GRID ref_grid );

REF_STATUS ref_migrate_shufflin( REF_GRID ref_grid );
REF_STATUS ref_migrate_shufflin_cell( REF_NODE ref_node, 
				      REF_CELL ref_cell );

END_C_DECLORATION

#endif /* REF_MIGRATE_H */
