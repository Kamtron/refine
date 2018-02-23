
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ref_math.h"

REF_STATUS ref_math_normalize(REF_DBL *normal) {
  REF_DBL length;

  length = sqrt(ref_math_dot(normal, normal));

  if (!ref_math_divisible(normal[0], length) ||
      !ref_math_divisible(normal[1], length) ||
      !ref_math_divisible(normal[2], length))
    return REF_DIV_ZERO;

  normal[0] /= length;
  normal[1] /= length;
  normal[2] /= length;

  length = ref_math_dot(normal, normal);
  RAS((ABS(length - 1.0) < 1.0e-13), "vector length not unity");

  return REF_SUCCESS;
}
