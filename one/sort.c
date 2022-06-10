
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

#include <stdlib.h>
#include <stdio.h>
#include "sort.h"

void sortHeap( int length, int *arrayInput, int *sortedIndex  )
{
  int i, l, ir, indxt, q;
  int n, j;

  for(i=0;i<length;i++) sortedIndex[i] = i;

  if (length < 2) return;

  n = length;
  l=(n >> 1)+1; 
  ir=n-1;
  for (;;) {
    if (l > 1) {
      l--;
      indxt=sortedIndex[l-1]; 
      q=arrayInput[indxt];
    } else {
      indxt=sortedIndex[ir];  
      q=arrayInput[indxt];
      sortedIndex[ir]=sortedIndex[0];
      if (--ir == 0) {
	sortedIndex[0]=indxt;
	break; 
      } 
    }
    i=l-1;
    j=l+i;

    while (j <= ir) { 
      if ( j < ir ) {
	if (arrayInput[sortedIndex[j]] < arrayInput[sortedIndex[j+1]]) j++; 
      }
      if (q < arrayInput[sortedIndex[j]]) { 
	sortedIndex[i]=sortedIndex[j];
	i=j; 

	j++;
	j <<= 1; 
	j--;

      } else break; 
    }
    sortedIndex[i]=indxt;
  } 
}

void sortDoubleHeap( int length, double *arrayInput, int *sortedIndex  )
{
  int i, l, ir, indxt;
  double q;
  int n, j;

  for(i=0;i<length;i++) sortedIndex[i] = i;

  if (length < 2) return;

  n = length;
  l=(n >> 1)+1; 
  ir=n-1;
  for (;;) {
    if (l > 1) {
      l--;
      indxt=sortedIndex[l-1]; 
      q=arrayInput[indxt];
    } else {
      indxt=sortedIndex[ir];  
      q=arrayInput[indxt];
      sortedIndex[ir]=sortedIndex[0];
      if (--ir == 0) {
	sortedIndex[0]=indxt;
	break; 
      } 
    }
    i=l-1;
    j=l+i;

    while (j <= ir) { 
      if ( j < ir ) {
	if (arrayInput[sortedIndex[j]] < arrayInput[sortedIndex[j+1]]) j++; 
      }
      if (q < arrayInput[sortedIndex[j]]) { 
	sortedIndex[i]=sortedIndex[j];
	i=j; 

	j++;
	j <<= 1; 
	j--;

      } else break; 
    }
    sortedIndex[i]=indxt;
  } 
}

int sortSearch( int length, int *sortednodes, int targetnode  )
{
  int lower, upper, mid;

  if (length<1) return EMPTY;

  if ( targetnode < sortednodes[0] || targetnode > sortednodes[length-1] ) 
    return EMPTY;

  lower = 0;
  upper = length-1;
  mid = length >> 1;

  if (targetnode==sortednodes[lower]) return lower;
  if (targetnode==sortednodes[upper]) return upper;

  while ( (lower < mid) && (mid < upper) ) {
    if ( targetnode >= sortednodes[mid] ) {
      if ( targetnode == sortednodes[mid] ) return mid;
      lower = mid;
    } else {
      upper = mid;
    }
    mid = (lower+upper) >> 1;
  }

  return EMPTY;
}
