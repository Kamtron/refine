
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

#include "ref_mpi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ref_malloc.h"

int main(int argc, char *argv[]) {
  REF_MPI ref_mpi;

  RSS(ref_mpi_start(argc, argv), "start");
  RSS(ref_mpi_create(&ref_mpi), "make mpi");

  if (ref_mpi_para(ref_mpi) && ref_mpi_once(ref_mpi))
    printf("%s number of processors %d max tag %d\n", argv[0],
           ref_mpi_n(ref_mpi), ref_mpi_max_tag(ref_mpi));

  if (!ref_mpi_para(ref_mpi)) { /* no mpi or mpi with one proc */
    REIS(1, ref_mpi_n(ref_mpi), "n");
    REIS(0, ref_mpi_rank(ref_mpi), "rank");
    RAS(ref_mpi_once(ref_mpi), "rank zero");
  }

  /* type */
  {
    REF_SIZE size;
    REF_TYPE type;
    size = sizeof(REF_INT);
    RSS(ref_mpi_int_size_type(size, &type), "int");
    REIS(REF_INT_TYPE, type, "expected int");
    size = sizeof(REF_LONG);
    RSS(ref_mpi_int_size_type(size, &type), "long");
    REIS(REF_LONG_TYPE, type, "expected long");
  }

  /* bcast */
  {
    REF_INT bc;
    bc = REF_EMPTY;
    if (ref_mpi_once(ref_mpi)) bc = 5;
    RSS(ref_mpi_bcast(ref_mpi, &bc, 1, REF_INT_TYPE), "bcast");
    REIS(5, bc, "bc wrong");
  }

  { /* deep copy bcast */
    REF_MPI deep_copy;
    RSS(ref_mpi_deep_copy(&deep_copy, ref_mpi), "deep copy");
    {
      REF_INT bc;
      bc = REF_EMPTY;
      if (ref_mpi_once(deep_copy)) bc = 5;
      RSS(ref_mpi_bcast(deep_copy, &bc, 1, REF_INT_TYPE), "bcast");
      REIS(5, bc, "bc wrong");
    }
    ref_mpi_stopwatch_stop(deep_copy, "deep copy bcast");
    RSS(ref_mpi_free(deep_copy), "mpi free");
  }

  /* scatter send/recv */
  {
    REF_INT part, scatter = REF_EMPTY;
    REF_INT n;
    n = 1;
    if (ref_mpi_once(ref_mpi)) {
      each_ref_mpi_worker(ref_mpi, part) {
        scatter = part;
        RSS(ref_mpi_scatter_send(ref_mpi, &scatter, n, REF_INT_TYPE, part),
            "scatter send");
      }
    } else {
      RSS(ref_mpi_scatter_recv(ref_mpi, &scatter, n, REF_INT_TYPE),
          "scatter recv");
      REIS(ref_mpi_rank(ref_mpi), scatter, "scatter match rank");
    }
  }

  /* gather send/recv */
  {
    REF_INT part, gather = REF_EMPTY;
    REF_INT n;
    n = 1;
    if (ref_mpi_once(ref_mpi)) {
      each_ref_mpi_worker(ref_mpi, part) {
        RSS(ref_mpi_gather_recv(ref_mpi, &gather, n, REF_INT_TYPE, part),
            "gather send");
        REIS(part, gather, "gather match part");
      }
    } else {
      gather = ref_mpi_rank(ref_mpi);
      RSS(ref_mpi_gather_send(ref_mpi, &gather, n, REF_INT_TYPE),
          "gather send");
    }
  }

  /* alltoall */
  {
    REF_INT part;
    REF_INT *a_size, *b_size;

    ref_malloc_init(a_size, ref_mpi_n(ref_mpi), REF_INT, REF_EMPTY);
    ref_malloc_init(b_size, ref_mpi_n(ref_mpi), REF_INT, REF_EMPTY);

    each_ref_mpi_part(ref_mpi, part) a_size[part] = part;

    RSS(ref_mpi_alltoall(ref_mpi, a_size, b_size, REF_INT_TYPE),
        "alltoall sizes");

    each_ref_mpi_part(ref_mpi, part) REIS(part, a_size[part], "a_size changed");
    each_ref_mpi_part(ref_mpi, part)
        REIS(ref_mpi_rank(ref_mpi), b_size[part], "b_size wrong");

    ref_free(b_size);
    ref_free(a_size);

    ref_mpi_stopwatch_stop(ref_mpi, "alltoall");
  }

  /* alltoallv native */
  if (ref_mpi_para(ref_mpi)) {
    REF_INT part;
    REF_INT magic = 42;
    REF_INT n = 17;
    REF_INT *a, *b;
    REF_INT *a_size, *b_size;
    REF_INT i;

    ref_malloc_init(a_size, ref_mpi_n(ref_mpi), REF_INT, n);
    ref_malloc_init(b_size, ref_mpi_n(ref_mpi), REF_INT, n);
    ref_malloc_init(a, n * ref_mpi_n(ref_mpi), REF_INT, magic);
    ref_malloc_init(b, n * ref_mpi_n(ref_mpi), REF_INT, REF_EMPTY);

    RSS(ref_mpi_alltoallv_native(ref_mpi, a, a_size, b, b_size, 1,
                                 REF_INT_TYPE),
        "alltoall sizes");

    each_ref_mpi_part(ref_mpi, part) {
      REIS(n, a_size[part], "a_size changed");
    }
    each_ref_mpi_part(ref_mpi, part) {
      REIS(n, b_size[part], "b_size changed");
    }

    for (i = 0; i < n * ref_mpi_n(ref_mpi); i++) {
      REIS(magic, a[part], "b wrong");
      REIS(magic, b[part], "b wrong");
    }

    ref_free(b);
    ref_free(a);
    ref_free(b_size);
    ref_free(a_size);

    ref_mpi_stopwatch_stop(ref_mpi, "alltoallv native");
  }

  /* alltoallv */
  if (ref_mpi_para(ref_mpi)) {
    REF_INT part;
    REF_INT magic = 42;
    REF_INT n = 17;
    REF_INT *a, *b;
    REF_INT *a_size, *b_size;
    REF_INT i;

    ref_malloc_init(a_size, ref_mpi_n(ref_mpi), REF_INT, n);
    ref_malloc_init(b_size, ref_mpi_n(ref_mpi), REF_INT, n);
    ref_malloc_init(a, n * ref_mpi_n(ref_mpi), REF_INT, magic);
    ref_malloc_init(b, n * ref_mpi_n(ref_mpi), REF_INT, REF_EMPTY);

    RSS(ref_mpi_alltoallv(ref_mpi, a, a_size, b, b_size, 1, REF_INT_TYPE),
        "alltoall sizes");

    each_ref_mpi_part(ref_mpi, part) {
      REIS(n, a_size[part], "a_size changed");
    }
    each_ref_mpi_part(ref_mpi, part) {
      REIS(n, b_size[part], "b_size changed");
    }

    for (i = 0; i < n * ref_mpi_n(ref_mpi); i++) {
      REIS(magic, a[part], "b wrong");
      REIS(magic, b[part], "b wrong");
    }

    ref_free(b);
    ref_free(a);
    ref_free(b_size);
    ref_free(a_size);

    ref_mpi_stopwatch_stop(ref_mpi, "alltoallv");
  }

  /* allconcat */
  {
    REF_INT ldim = 2;
    REF_INT total, part;
    REF_INT *separate, *together, *source;

    ref_malloc(separate, ldim * 2, REF_INT);

    separate[0 + 2 * 0] = ref_mpi_rank(ref_mpi);
    separate[1 + 2 * 0] = ref_mpi_rank(ref_mpi);
    separate[0 + 2 * 1] = REF_EMPTY;
    separate[1 + 2 * 1] = REF_EMPTY;

    RSS(ref_mpi_allconcat(ref_mpi, ldim, 2, (void *)separate, &total, &source,
                          (void **)&together, REF_INT_TYPE),
        "allconcat");

    REIS(2 * ref_mpi_n(ref_mpi), total, "expected size");
    each_ref_mpi_part(ref_mpi, part) {
      REIS(part, source[0 + 2 * part], "const");
      REIS(part, source[1 + 2 * part], "const");

      REIS(part, together[0 + ldim * 2 * part], "const");
      REIS(part, together[1 + ldim * 2 * part], "const");
      REIS(REF_EMPTY, together[2 + ldim * 2 * part], "const");
      REIS(REF_EMPTY, together[3 + ldim * 2 * part], "const");
    }

    ref_free(together);
    ref_free(source);
    ref_free(separate);

    ref_mpi_stopwatch_stop(ref_mpi, "allconcat");
  }

  /* allminwho, one per rank */
  {
    REF_INT part;
    REF_DBL *val;
    REF_INT *who;

    ref_malloc(val, ref_mpi_n(ref_mpi), REF_DBL);
    ref_malloc(who, ref_mpi_n(ref_mpi), REF_INT);

    each_ref_mpi_part(ref_mpi, part) val[part] = 1.0;
    val[ref_mpi_rank(ref_mpi)] = 0.0;

    RSS(ref_mpi_allminwho(ref_mpi, val, who, ref_mpi_n(ref_mpi)), "minloc");

    each_ref_mpi_part(ref_mpi, part) {
      REIS(part, who[part], "who");
      RWDS(0.0, val[part], -1.0, "min");
    }

    ref_free(who);
    ref_free(val);
  }

  /* allminwho, 1403 */
  {
    REF_INT i, n = 1403;
    REF_DBL *val;
    REF_INT *who;

    ref_malloc(val, n, REF_DBL);
    ref_malloc(who, n, REF_INT);

    for (i = 0; i < n; i++) val[i] = (REF_DBL)ref_mpi_rank(ref_mpi);

    RSS(ref_mpi_allminwho(ref_mpi, val, who, n), "minloc");

    for (i = 0; i < n; i++) {
      REIS(0, who[i], "who");
      RWDS(0.0, val[i], -1.0, "min");
    }

    ref_free(who);
    ref_free(val);

    ref_mpi_stopwatch_stop(ref_mpi, "allwho");
  }

  /* blindsend int with 0,0 */
  {
    REF_INT ldim = 2;
    REF_INT l, i, part, size, n;
    REF_INT *send;
    REF_INT *proc;
    REF_INT *recv;

    size = 0;
    each_ref_mpi_part(ref_mpi, part) size += MIN(part, 3);

    ref_malloc(proc, size, REF_INT);
    ref_malloc(send, ldim * size, REF_INT);

    size = 0;
    each_ref_mpi_part(ref_mpi, part) for (i = 0; i < MIN(part, 3); i++) {
      proc[size] = part;
      for (l = 0; l < ldim; l++) {
        send[l + ldim * size] = ref_mpi_rank(ref_mpi);
      }
      size++;
    }

    RSS(ref_mpi_blindsend(ref_mpi, proc, (void *)send, ldim, size,
                          (void **)&recv, &n, REF_INT_TYPE),
        "blindsend");

    size = 0;
    each_ref_mpi_part(
        ref_mpi, part) for (i = 0; i < MIN(ref_mpi_rank(ref_mpi), 3); i++) {
      for (l = 0; l < ldim; l++)
        REIS(part, recv[l + ldim * size], "recv mismatch");
      size++;
    }
    REIS(size, n, "size mismatch");

    ref_free(recv);
    ref_free(send);
    ref_free(proc);
  }

  /* blindsend dbl with 0,1 */
  {
    REF_INT ldim = 1; /* must be 1 for this test */
    REF_INT i, part, size, n;
    REF_DBL *send;
    REF_INT *proc;
    REF_DBL *recv;

    size = 0;
    each_ref_mpi_part(ref_mpi, part) size += MAX(1, MIN(part, 3));

    ref_malloc(proc, size, REF_INT);
    ref_malloc(send, size, REF_DBL);

    size = 0;
    each_ref_mpi_part(ref_mpi, part) for (i = 0; i < MAX(1, MIN(part, 3));
                                          i++) {
      proc[size] = part;
      send[size] = (REF_DBL)ref_mpi_rank(ref_mpi);
      size++;
    }

    RSS(ref_mpi_blindsend(ref_mpi, proc, (void *)send, ldim, size,
                          (void **)&recv, &n, REF_DBL_TYPE),
        "blindsend");

    size = 0;
    each_ref_mpi_part(
        ref_mpi, part) for (i = 0; i < MAX(1, MIN(ref_mpi_rank(ref_mpi), 3));
                            i++) {
      RWDS((REF_DBL)part, recv[size], -1, "recv mismatch");
      size++;
    }
    REIS(size, n, "size mismatch");

    ref_free(recv);
    ref_free(send);
    ref_free(proc);

    ref_mpi_stopwatch_stop(ref_mpi, "blind send");
  }

  /* split */
  {
    REF_MPI new_mpi;
    RSS(ref_mpi_half_comm(ref_mpi, &new_mpi), "split");
    if (ref_mpi_para(new_mpi) && ref_mpi_once(new_mpi))
      printf("split number of processors %d lead by world rank %d\n",
             ref_mpi_n(new_mpi), ref_mpi_rank(ref_mpi));
    RSS(ref_mpi_join_comm(new_mpi), "join");
    RSS(ref_mpi_free(new_mpi), "new free");
  }

  /* balance, all even */
  {
    REF_DBL *items = NULL;
    REF_INT nitem = 0;
    REF_DBL *balanced = NULL;
    REF_INT nbalanced = 0;
    REF_INT total = 100;
    REF_INT ldim = 1;
    REF_INT first_rank, last_rank;
    if (0 == ref_mpi_rank(ref_mpi)) {
      nitem = total;
      ref_malloc_init(items, nitem, REF_DBL, 0);
    }
    first_rank = 0;
    last_rank = ref_mpi_n(ref_mpi) - 1;
    RSS(ref_mpi_balance(ref_mpi, ldim, nitem, (void *)items, first_rank,
                        last_rank, &nbalanced, (void **)(&balanced),
                        REF_INT_TYPE),
        "bal");
    RAS(total / ref_mpi_n(ref_mpi) <= nbalanced &&
            nbalanced <= 1 + total / ref_mpi_n(ref_mpi),
        "not bal");
    ref_free(balanced);
    ref_free(items);
  }

  /* balance, all prime */
  {
    REF_DBL *items = NULL;
    REF_INT nitem = 0;
    REF_DBL *balanced = NULL;
    REF_INT nbalanced = 0;
    REF_INT total = 163;
    REF_INT ldim = 1;
    REF_INT first_rank, last_rank;
    if (0 == ref_mpi_rank(ref_mpi)) {
      nitem = total;
      ref_malloc_init(items, nitem, REF_DBL, 0);
    }
    first_rank = 0;
    last_rank = ref_mpi_n(ref_mpi) - 1;
    RSS(ref_mpi_balance(ref_mpi, ldim, nitem, (void *)items, first_rank,
                        last_rank, &nbalanced, (void **)(&balanced),
                        REF_INT_TYPE),
        "bal");
    RAS(total / ref_mpi_n(ref_mpi) <= nbalanced &&
            nbalanced <= 1 + total / ref_mpi_n(ref_mpi),
        "not bal");
    ref_free(balanced);
    ref_free(items);
  }

  /* balance, first half, even */
  {
    REF_DBL *items = NULL;
    REF_INT nitem = 0;
    REF_DBL *balanced = NULL;
    REF_INT nbalanced = 0;
    REF_INT total = 100;
    REF_INT ldim = 1;
    REF_INT active;
    REF_INT first_rank, last_rank;
    if (0 == ref_mpi_rank(ref_mpi)) {
      nitem = total;
      ref_malloc_init(items, nitem, REF_DBL, 0);
    }
    first_rank = 0;
    last_rank = MAX(ref_mpi_n(ref_mpi) / 2 - 1, 0);
    active = last_rank - first_rank + 1;
    RSS(ref_mpi_balance(ref_mpi, ldim, nitem, (void *)items, first_rank,
                        last_rank, &nbalanced, (void **)&balanced,
                        REF_INT_TYPE),
        "bal");
    if (first_rank <= ref_mpi_rank(ref_mpi) &&
        ref_mpi_rank(ref_mpi) <= last_rank) {
      RAS(total / active <= nbalanced && nbalanced <= 1 + total / active,
          "not bal");
    } else {
      REIS(0, nbalanced, "not zero");
    }
    ref_free(balanced);
    ref_free(items);
  }

  /* balance, first half, prime */
  {
    REF_DBL *items = NULL;
    REF_INT nitem = 0;
    REF_DBL *balanced = NULL;
    REF_INT nbalanced = 0;
    REF_INT total = 191;
    REF_INT ldim = 1;
    REF_INT active;
    REF_INT first_rank, last_rank;
    if (0 == ref_mpi_rank(ref_mpi)) {
      nitem = total;
      ref_malloc_init(items, nitem, REF_DBL, 0);
    }
    first_rank = 0;
    last_rank = MAX(ref_mpi_n(ref_mpi) / 2 - 1, 0);
    active = last_rank - first_rank + 1;
    RSS(ref_mpi_balance(ref_mpi, ldim, nitem, (void *)items, first_rank,
                        last_rank, &nbalanced, (void **)(&balanced),
                        REF_INT_TYPE),
        "bal");
    if (first_rank <= ref_mpi_rank(ref_mpi) &&
        ref_mpi_rank(ref_mpi) <= last_rank) {
      RAS(total / active <= nbalanced && nbalanced <= 1 + total / active,
          "not bal");
    } else {
      REIS(0, nbalanced, "not zero");
    }
    ref_free(balanced);
    ref_free(items);
  }

  /* balance, last half, even */
  {
    REF_DBL *items = NULL;
    REF_INT nitem = 0;
    REF_DBL *balanced = NULL;
    REF_INT nbalanced = 0;
    REF_INT total = 100;
    REF_INT ldim = 1;
    REF_INT active;
    REF_INT first_rank, last_rank;
    if (0 == ref_mpi_rank(ref_mpi)) {
      nitem = total;
      ref_malloc_init(items, nitem, REF_DBL, 0);
    }
    first_rank = ref_mpi_n(ref_mpi) / 2;
    last_rank = ref_mpi_n(ref_mpi) - 1;
    active = last_rank - first_rank + 1;
    RSS(ref_mpi_balance(ref_mpi, ldim, nitem, (void *)items, first_rank,
                        last_rank, &nbalanced, (void **)(&balanced),
                        REF_INT_TYPE),
        "bal");
    if (first_rank <= ref_mpi_rank(ref_mpi) &&
        ref_mpi_rank(ref_mpi) <= last_rank) {
      RAS(total / active <= nbalanced && nbalanced <= 1 + total / active,
          "not bal");
    } else {
      REIS(0, nbalanced, "not zero");
    }
    ref_free(balanced);
    ref_free(items);
  }

  /* balance, first half, prime */
  {
    REF_DBL *items = NULL;
    REF_INT nitem = 0;
    REF_DBL *balanced = NULL;
    REF_INT nbalanced = 0;
    REF_INT total = 191;
    REF_INT ldim = 1;
    REF_INT active;
    REF_INT first_rank, last_rank;
    if (0 == ref_mpi_rank(ref_mpi)) {
      nitem = total;
      ref_malloc_init(items, nitem, REF_DBL, 0);
    }
    first_rank = ref_mpi_n(ref_mpi) / 2;
    last_rank = ref_mpi_n(ref_mpi) - 1;
    active = last_rank - first_rank + 1;
    RSS(ref_mpi_balance(ref_mpi, ldim, nitem, (void *)items, first_rank,
                        last_rank, &nbalanced, (void **)(&balanced),
                        REF_INT_TYPE),
        "bal");
    if (first_rank <= ref_mpi_rank(ref_mpi) &&
        ref_mpi_rank(ref_mpi) <= last_rank) {
      RAS(total / active <= nbalanced && nbalanced <= 1 + total / active,
          "not bal");
    } else {
      REIS(0, nbalanced, "not zero");
    }
    ref_free(balanced);
    ref_free(items);
  }

  /* balance, first half, dist */
  {
    REF_DBL *items = NULL;
    REF_INT nitem = 0;
    REF_DBL *balanced = NULL;
    REF_INT total, nbalanced = 0;
    REF_INT ldim = 1;
    REF_INT active;
    REF_INT first_rank, last_rank;
    nitem = 10;
    ref_malloc_init(items, nitem, REF_DBL, 0);
    total = nitem * ref_mpi_n(ref_mpi);

    first_rank = ref_mpi_n(ref_mpi) / 2;
    last_rank = ref_mpi_n(ref_mpi) - 1;
    active = last_rank - first_rank + 1;
    RSS(ref_mpi_balance(ref_mpi, ldim, nitem, (void *)items, first_rank,
                        last_rank, &nbalanced, (void **)(&balanced),
                        REF_INT_TYPE),
        "bal");
    if (first_rank <= ref_mpi_rank(ref_mpi) &&
        ref_mpi_rank(ref_mpi) <= last_rank) {
      RAS(total / active <= nbalanced && nbalanced <= 1 + total / active,
          "not bal");
    } else {
      REIS(0, nbalanced, "not zero");
    }
    ref_free(balanced);
    ref_free(items);

    ref_mpi_stopwatch_stop(ref_mpi, "balance");
  }

  { /* deep reduce chunk */
    REF_MPI deep_copy;
    RSS(ref_mpi_deep_copy(&deep_copy, ref_mpi), "deep copy");
    {
      REF_INT chunk_bytes = 64;
      REF_INT expected_chunk = 1024;
      ref_mpi_reduce_byte_limit(deep_copy) = chunk_bytes * expected_chunk;
      REIS(expected_chunk, ref_mpi_reduce_chunk_limit(deep_copy, chunk_bytes),
           "chunk");
    }
    RSS(ref_mpi_free(deep_copy), "mpi free");
  }

  { /* deep reduce chunk empty unilimited */
    REF_MPI deep_copy;
    RSS(ref_mpi_deep_copy(&deep_copy, ref_mpi), "deep copy");
    {
      REF_INT chunk_bytes = 64;
      REF_INT expected_chunk = REF_INT_MAX;
      ref_mpi_reduce_byte_limit(deep_copy) = REF_EMPTY;
      REIS(expected_chunk, ref_mpi_reduce_chunk_limit(deep_copy, chunk_bytes),
           "chunk");
    }
    RSS(ref_mpi_free(deep_copy), "mpi free");
  }

  RSS(ref_mpi_free(ref_mpi), "mpi free");
  RSS(ref_mpi_stop(), "stop");

  return 0;
}
