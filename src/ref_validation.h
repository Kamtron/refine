
/* Copyright 2014 United States Government as represented by the
 * Administrator of the National Aeronautics and Space
 * Administration. No copyright is claimed in the United States under
 * Title 17, U.S. Code.  All Other Rights Reserved.
 *
 * The refine platform is licensed under the Apache License, Version
 * 2.0 (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef REF_VALIDATION_H
#define REF_VALIDATION_H

#include "ref_defs.h"

#include "ref_grid.h"

BEGIN_C_DECLORATION

REF_STATUS ref_validation_all(REF_GRID ref_grid);
REF_STATUS ref_validation_unused_node(REF_GRID ref_grid);
REF_STATUS ref_validation_boundary_face(REF_GRID ref_grid);
REF_STATUS ref_validation_cell_face(REF_GRID ref_grid);
REF_STATUS ref_validation_cell_node(REF_GRID ref_grid);
REF_STATUS ref_validation_cell_volume(REF_GRID ref_grid);

END_C_DECLORATION

#endif /* REF_VALIDATION_H */
