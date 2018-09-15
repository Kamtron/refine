
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

#ifndef REF_DICT_H
#define REF_DICT_H

#include "ref_defs.h"

BEGIN_C_DECLORATION
typedef struct REF_DICT_STRUCT REF_DICT_STRUCT;
typedef REF_DICT_STRUCT *REF_DICT;
END_C_DECLORATION

BEGIN_C_DECLORATION

struct REF_DICT_STRUCT {
  REF_INT n, max, naux;
  REF_INT *key;
  REF_INT *value;
  REF_DBL *aux;
};

REF_STATUS ref_dict_create(REF_DICT *ref_dict);
REF_STATUS ref_dict_includes_aux_value(REF_DICT ref_dict, REF_INT naux);
REF_STATUS ref_dict_free(REF_DICT ref_dict);

REF_STATUS ref_dict_deep_copy(REF_DICT *ref_dict, REF_DICT original);

#define ref_dict_n(ref_dict) ((ref_dict)->n)
#define ref_dict_max(ref_dict) ((ref_dict)->max)
#define ref_dict_naux(ref_dict) ((ref_dict)->naux)
#define ref_dict_key(ref_dict, key_index) ((ref_dict)->key[(key_index)])
#define ref_dict_keyvalue(ref_dict, key_index) ((ref_dict)->value[(key_index)])
#define ref_dict_keyvalueaux(ref_dict, aux_index, key_index) \
  ((ref_dict)->aux[(aux_index) + ref_dict_naux(ref_dict) * (key_index)])

#define each_ref_dict_key(ref_dict, key_index, dict_key)                \
  for ((key_index) = 0, (dict_key) = ref_dict_key(ref_dict, key_index); \
       (key_index) < ref_dict_n(ref_dict);                              \
       (key_index)++, (dict_key) = ref_dict_key(ref_dict, key_index))

#define each_ref_dict_key_value(ref_dict, key_index, dict_key, dict_value) \
  for ((key_index) = 0, (dict_key) = ref_dict_key(ref_dict, key_index),    \
      (dict_value) = ref_dict_keyvalue(ref_dict, key_index);               \
       (key_index) < ref_dict_n(ref_dict);                                 \
       (key_index)++, (dict_key) = ref_dict_key(ref_dict, key_index),      \
      (dict_value) = ref_dict_keyvalue(ref_dict, key_index))

#define each_ref_dict_key_index(ref_dict, key_index) \
  for ((key_index) = 0; (key_index) < ref_dict_n(ref_dict); (key_index)++)

#define each_ref_dict_aux_index(ref_dict, aux_index) \
  for ((aux_index) = 0; (aux_index) < ref_dict_naux(ref_dict); (aux_index)++)

REF_STATUS ref_dict_store(REF_DICT ref_dict, REF_INT key, REF_INT value);
REF_STATUS ref_dict_location(REF_DICT ref_dict, REF_INT key, REF_INT *location);
REF_STATUS ref_dict_remove(REF_DICT ref_dict, REF_INT key);
REF_STATUS ref_dict_value(REF_DICT ref_dict, REF_INT key, REF_INT *value);

REF_BOOL ref_dict_has_key(REF_DICT ref_dict, REF_INT key);
REF_BOOL ref_dict_has_value(REF_DICT ref_dict, REF_INT value);

REF_STATUS ref_dict_inspect(REF_DICT ref_dict);
END_C_DECLORATION

#endif /* REF_DICT_H */
