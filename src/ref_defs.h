
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

#ifndef REF_DEFS_H
#define REF_DEFS_H

#ifdef __cplusplus
#define BEGIN_C_DECLORATION extern "C" {
#define END_C_DECLORATION }
#else
#define BEGIN_C_DECLORATION
#define END_C_DECLORATION
#endif

#include <limits.h>

BEGIN_C_DECLORATION

typedef int REF_BOOL;
#define REF_TRUE (1)
#define REF_FALSE (0)

#define REF_EMPTY (-1)

#if !defined(ABS)
#define ABS(a) ((a) > 0 ? (a) : -(a))
#endif

#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#if !defined(MAX)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef int REF_INT;
#define REF_INT_MAX (INT_MAX)
#define REF_INT_MIN (INT_MIN)

typedef double REF_DBL;

typedef char REF_BYTE;

typedef int REF_STATUS;

#define REF_SUCCESS (0)
#define REF_FAILURE (1)
#define REF_NULL (2)
#define REF_INVALID (3)
#define REF_DIV_ZERO (4)
#define REF_NOT_FOUND (5)
#define REF_IMPLEMENT (6)
#define REF_INCREASE_LIMIT (7)

#define RSS(fcn, msg)                                             \
  {                                                               \
    REF_STATUS ref_private_macro_code;                            \
    ref_private_macro_code = (fcn);                               \
    if (REF_SUCCESS != ref_private_macro_code) {                  \
      printf("%s: %d: %s: %d %s\n", __FILE__, __LINE__, __func__, \
             ref_private_macro_code, (msg));                      \
      return ref_private_macro_code;                              \
    }                                                             \
  }

#define RSE(fcn, msg)                                             \
  {                                                               \
    REF_STATUS ref_private_macro_code;                            \
    ref_private_macro_code = (fcn);                               \
    if (REF_SUCCESS != ref_private_macro_code) {                  \
      printf("%s: %d: %s: %d %s\n", __FILE__, __LINE__, __func__, \
             ref_private_macro_code, (msg));                      \
      return REF_EMPTY;                                           \
    }                                                             \
  }

#define RSB(fcn, msg, block)                                      \
  {                                                               \
    REF_STATUS ref_private_macro_code;                            \
    ref_private_macro_code = (fcn);                               \
    if (REF_SUCCESS != ref_private_macro_code) {                  \
      printf("%s: %d: %s: %d %s\n", __FILE__, __LINE__, __func__, \
             ref_private_macro_code, (msg));                      \
      block;                                                      \
      return ref_private_macro_code;                              \
    }                                                             \
  }

#define RXS(fcn, allowed_exception, msg)                          \
  {                                                               \
    REF_STATUS ref_private_macro_code;                            \
    ref_private_macro_code = (fcn);                               \
    if (!(REF_SUCCESS == ref_private_macro_code ||                \
          (allowed_exception) == ref_private_macro_code)) {       \
      printf("%s: %d: %s: %d %s\n", __FILE__, __LINE__, __func__, \
             ref_private_macro_code, (msg));                      \
      return ref_private_macro_code;                              \
    }                                                             \
  }

#define REF_WHERE(msg) \
  printf("%s: %d: %s: %s\n", __FILE__, __LINE__, __func__, (msg));

#define RNS(ptr, msg)            \
  {                              \
    if (NULL == (void *)(ptr)) { \
      REF_WHERE(msg);            \
      return REF_NULL;           \
    }                            \
  }

#define RNB(ptr, msg, block)     \
  {                              \
    if (NULL == (void *)(ptr)) { \
      REF_WHERE(msg);            \
      block;                     \
      return REF_NULL;           \
    }                            \
  }

#define REIB(a, b, msg, block)                                               \
  {                                                                          \
    REF_INT ref_private_status_ai, ref_private_status_bi;                    \
    ref_private_status_ai = (a);                                             \
    ref_private_status_bi = (b);                                             \
    if (ref_private_status_ai != ref_private_status_bi) {                    \
      printf("%s: %d: %s: %s\nexpected %d was %d\n", __FILE__, __LINE__,     \
             __func__, (msg), ref_private_status_ai, ref_private_status_bi); \
      block;                                                                 \
      return REF_FAILURE;                                                    \
    }                                                                        \
  }

#define REIS(a, b, msg)                                                      \
  {                                                                          \
    REF_INT ref_private_status_ai, ref_private_status_bi;                    \
    ref_private_status_ai = (a);                                             \
    ref_private_status_bi = (b);                                             \
    if (ref_private_status_ai != ref_private_status_bi) {                    \
      printf("%s: %d: %s: %s\nexpected %d was %d\n", __FILE__, __LINE__,     \
             __func__, (msg), ref_private_status_ai, ref_private_status_bi); \
      return REF_FAILURE;                                                    \
    }                                                                        \
  }

#define RWDS(a, b, tol, msg)                                                  \
  {                                                                           \
    REF_DBL ref_private_status_ad, ref_private_status_bd;                     \
    REF_DBL ref_private_status_del, ref_private_status_allowed;               \
    ref_private_status_ad = (a);                                              \
    ref_private_status_bd = (b);                                              \
    ref_private_status_del =                                                  \
        ABS(ref_private_status_ad - ref_private_status_bd);                   \
    ref_private_status_allowed = (tol);                                       \
    if (tol < 0.0)                                                            \
      ref_private_status_allowed =                                            \
          MAX(1.0E-12, 1.0E-12 * ABS(ref_private_status_ad));                 \
    if (ref_private_status_del > ref_private_status_allowed) {                \
      printf("%s: %d: %s: %s\nexpected %.15e, was %.15e, %e outside of %e\n", \
             __FILE__, __LINE__, __func__, (msg), ref_private_status_ad,      \
             ref_private_status_bd, ref_private_status_del,                   \
             ref_private_status_allowed);                                     \
      return REF_FAILURE;                                                     \
    }                                                                         \
  }

#define RES(a, b, msg)    \
  {                       \
    if ((a) != (b)) {     \
      REF_WHERE(msg);     \
      return REF_FAILURE; \
    }                     \
  }

#define RUS(a, b, msg)    \
  {                       \
    if ((a) == (b)) {     \
      REF_WHERE(msg);     \
      return REF_FAILURE; \
    }                     \
  }

#define RAS(a, msg)       \
  {                       \
    if (!(a)) {           \
      REF_WHERE(msg);     \
      return REF_FAILURE; \
    }                     \
  }

#define RAB(a, msg, block) \
  {                        \
    if (!(a)) {            \
      REF_WHERE(msg);      \
      block;               \
      return REF_FAILURE;  \
    }                      \
  }

#define RAE(a, msg)     \
  {                     \
    if (!(a)) {         \
      REF_WHERE(msg);   \
      return REF_EMPTY; \
    }                   \
  }

#define THROW(msg)      \
  {                     \
    REF_WHERE(msg);     \
    return REF_FAILURE; \
  }

#define RAISE(fcn)                               \
  {                                              \
    REF_STATUS ref_private_macro_code;           \
    ref_private_macro_code = (fcn);              \
    if (REF_SUCCESS != ref_private_macro_code) { \
      return ref_private_macro_code;             \
    }                                            \
  }

#define SUPRESS_UNUSED_COMPILER_WARNING(ptr) \
  if (NULL == (void *)(&(ptr) + 1)) printf("unused macro failed\n");

#define SKIP_BLOCK(why)                                        \
  printf(" *** %s *** at %s:%d\n", (why), __FILE__, __LINE__); \
  if (REF_FALSE)

END_C_DECLORATION

#endif /* REF_DEFS_H */
