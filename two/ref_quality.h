
#ifndef REF_QUALITY_H
#define REF_QUALITY_H

#include "ref_defs.h"

#include "ref_grid.h"

BEGIN_C_DECLORATION

REF_STATUS ref_quality_hex( REF_GRID ref_grid );

REF_STATUS ref_quality_report_multiple_face_cell( REF_GRID ref_grid, 
						  REF_INT *count,
						  char *export_to );

REF_STATUS ref_quality_swap_multiple_face_cell( REF_GRID ref_grid );
REF_STATUS ref_quality_split_multiple_face_cell( REF_GRID ref_grid );

END_C_DECLORATION

#endif /* REF_QUALITY_H */
