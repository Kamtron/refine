
#ifndef REF_METRIC_H
#define REF_METRIC_H

#include "ref_defs.h"

#include "ref_grid.h"
#include "ref_node.h"

BEGIN_C_DECLORATION

REF_STATUS ref_metric_show( REF_DBL *metric );

REF_STATUS ref_metric_from_node( REF_DBL *metric, REF_NODE ref_node );
REF_STATUS ref_metric_to_node( REF_DBL *metric, REF_NODE ref_node );

REF_STATUS ref_metric_unit_node( REF_NODE ref_node );
REF_STATUS ref_metric_olympic_node( REF_NODE ref_node, REF_DBL h );

REF_STATUS ref_metric_sanitize( REF_GRID ref_grid );
REF_STATUS ref_metric_sanitize_threed( REF_GRID ref_grid );

REF_STATUS ref_metric_imply_from( REF_DBL *metric, REF_GRID ref_grid );

REF_STATUS ref_metric_imply_non_tet( REF_DBL *metric, REF_GRID ref_grid );

REF_STATUS ref_metric_smr( REF_DBL *metric0, REF_DBL *metric1, REF_DBL *metric, 
			   REF_GRID ref_grid );

END_C_DECLORATION

#endif /* REF_METRIC_H */
