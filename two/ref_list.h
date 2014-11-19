
#ifndef REF_LIST_H
#define REF_LIST_H

#include "ref_defs.h"

BEGIN_C_DECLORATION
typedef struct REF_LIST_STRUCT REF_LIST_STRUCT;
typedef REF_LIST_STRUCT * REF_LIST;
END_C_DECLORATION

BEGIN_C_DECLORATION

struct REF_LIST_STRUCT {
  REF_INT n, max;
  REF_INT *value;
};

REF_STATUS ref_list_create( REF_LIST *ref_list );
REF_STATUS ref_list_free( REF_LIST ref_list );
REF_STATUS ref_list_inspect( REF_LIST ref_list );

#define ref_list_n( ref_list ) ((ref_list)->n)
#define ref_list_max( ref_list ) ((ref_list)->max)
#define ref_list_value( ref_list, i ) ((ref_list)->value[(i)])

REF_STATUS ref_list_add( REF_LIST ref_list, REF_INT value );
REF_STATUS ref_list_pop( REF_LIST ref_list, REF_INT *value );

REF_STATUS ref_list_shift( REF_LIST ref_list, 
			   REF_INT equal_and_above, REF_INT offset );

REF_STATUS ref_list_sort( REF_LIST ref_list );

REF_STATUS ref_list_erase( REF_LIST ref_list );

REF_STATUS ref_list_allgather( REF_LIST ref_list );

REF_STATUS ref_list_contains( REF_LIST ref_list, REF_INT item, 
			      REF_BOOL *contains );

END_C_DECLORATION

#endif /* REF_LIST_H */
