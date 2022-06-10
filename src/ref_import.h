
/* Copyright 2006, 2014, 2021 United States Government as represented
 * by the Administrator of the National Aeronautics and Space
 * Administration. No copyright is claimed in the United States under
 * Title 17, U.S. Code.  All Other Rights Reserved.
 *
 * The refine version 3 unstructured grid adaptation platform is
 * licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef REF_IMPORT_H
#define REF_IMPORT_H

#include <stdio.h>

#include "ref_dict.h"
#include "ref_grid.h"

BEGIN_C_DECLORATION

REF_FCN REF_STATUS ref_import_by_extension(REF_GRID *ref_grid, REF_MPI ref_mpi,
                                           const char *filename);

#define REF_IMPORT_MESHB_LAST_KEYWORD (156) /* 203-47 */
REF_FCN REF_STATUS ref_import_meshb_header(const char *filename,
                                           REF_INT *version,
                                           REF_FILEPOS *key_pos);
REF_FCN REF_STATUS ref_import_meshb_jump(FILE *file, REF_INT version,
                                         REF_FILEPOS *key_pos, REF_INT keyword,
                                         REF_BOOL *available,
                                         REF_FILEPOS *next_position);

REF_FCN REF_STATUS ref_import_examine_header(const char *filename);

END_C_DECLORATION

#endif /* REF_IMPORT_H */
