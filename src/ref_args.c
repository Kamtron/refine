
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ref_args.h"

REF_STATUS ref_args_inspect( REF_INT n, char **args )
{
  REF_INT i;

  for (i = 0; i<n; i++)
    printf("%d : '%s'\n",i,args[i]);

  return REF_SUCCESS;
}

REF_STATUS ref_args_find( REF_INT n, char **args,
                          const char *target, REF_INT *pos )
{
  REF_INT i;

  *pos = REF_EMPTY;

  for (i = 0; i<n; i++)
    {
      if ( 0 == strcmp( target, args[i] ) )
        {
          *pos = i;
          return REF_SUCCESS;
        }
    }

  return REF_NOT_FOUND;
}

