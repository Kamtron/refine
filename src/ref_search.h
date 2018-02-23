

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

#ifndef REF_SEARCH_H
#define REF_SEARCH_H

#include "ref_defs.h"

BEGIN_C_DECLORATION
typedef struct REF_SEARCH_STRUCT REF_SEARCH_STRUCT;
typedef REF_SEARCH_STRUCT *REF_SEARCH;
END_C_DECLORATION

#include "ref_list.h"

BEGIN_C_DECLORATION
struct REF_SEARCH_STRUCT {
  REF_INT d, n;
  REF_INT empty;
  REF_INT *item;
  REF_INT *left, *right;
  REF_DBL *pos;
  REF_DBL *radius;
  REF_DBL *children_ball;
};

REF_STATUS ref_search_create(REF_SEARCH *ref_search, REF_INT n);

REF_STATUS ref_search_free(REF_SEARCH ref_search);

REF_STATUS ref_search_insert(REF_SEARCH ref_search, REF_INT item,
                             REF_DBL *position, REF_DBL radius);

REF_STATUS ref_search_touching(REF_SEARCH ref_search, REF_LIST ref_list,
                               REF_DBL *position, REF_DBL radius);

END_C_DECLORATION

#endif /* REF_SEARCH_H */
