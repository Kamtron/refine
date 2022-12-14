
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
#include <math.h>
#include <float.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tableau.h"

Tableau* tableauCreate( int constraints, int dimension )
{
  int i, length;
  Tableau *tableau;

  tableau = (Tableau *)malloc( sizeof(Tableau) );

  tableau->constraints = constraints;
  tableau->dimension = dimension;

  tableau->constraint_matrix = (double *)malloc( tableauConstraints(tableau) *
						 tableauDimension( tableau ) * 
						 sizeof(double) );
  tableau->constraint        = (double *)malloc( tableauConstraints(tableau) *
						 sizeof(double) );
  tableau->cost              = (double *)malloc( tableauDimension(tableau) *
						 sizeof(double) );

  length = (1+tableauConstraints(tableau)) * 
           (1+tableauDimension( tableau )+tableauConstraints(tableau));
  tableau->t = (double *)malloc(  length * sizeof(double) );
  for (i=0;i<length;i++) tableau->t[i] = 0.0 ;

  tableau->basis = (int *)malloc( tableauConstraints(tableau) * sizeof(int) );
  for (i=0;i<tableauConstraints(tableau);i++) 
    tableau->basis[i] = tableauDimension( tableau ) + i ;
  length = 1+tableauDimension( tableau )+tableauConstraints(tableau);
  tableau->in_basis = (int *)malloc( length * sizeof(int) );
  for (i=0;i<length;i++)  tableau->in_basis[i] = EMPTY;
  for (i=0;i<tableauConstraints(tableau);i++) 
    tableau->in_basis[tableau->basis[i]+1] = i;
  

  return tableau;
}

void tableauFree( Tableau *tableau )
{
  if (NULL != tableau->constraint_matrix) free(tableau->constraint_matrix);
  if (NULL != tableau->constraint)        free(tableau->constraint);
  if (NULL != tableau->cost)              free(tableau->cost);
  if (NULL != tableau->basis)             free(tableau->basis);
  if (NULL != tableau->in_basis)          free(tableau->in_basis);
  if (NULL != tableau->t)                 free(tableau->t);
  free( tableau );
}

Tableau *tableauConstraintMatrix( Tableau *tableau, double *constraint_matrix )
{
  int i, length;
  length = tableauConstraints( tableau ) * tableauDimension( tableau );
  for (i=0;i<length;i++) tableau->constraint_matrix[i] = constraint_matrix[i];
  return tableau;
}

Tableau *tableauConstraint( Tableau *tableau, double *constraint )
{
  int i, length;
  length = tableauConstraints( tableau );
  for (i=0;i<length;i++) tableau->constraint[i] = constraint[i];
  return tableau;
}

Tableau *tableauCost( Tableau *tableau, double *cost )
{
  int i, length;
  length = tableauDimension( tableau );
  for (i=0;i<length;i++) tableau->cost[i] = cost[i];
  return tableau;
}


int tableauConstraints( Tableau *tableau )
{
  return tableau->constraints;
}

int tableauDimension( Tableau *tableau )
{
  return tableau->dimension;
}

Tableau *tableauBasis( Tableau *tableau, int *basis )
{
  int i;
  for (i=0;i<tableauConstraints(tableau);i++) basis[i] = tableau->basis[i];
  return tableau;
}

Tableau *tableauInit( Tableau *tableau )
{

  int i, j;
  int m, n;
  int t_index, A_index;

  m = 1 + tableauConstraints( tableau );
  n = 1 + tableauDimension( tableau ) + tableauConstraints( tableau );

  /* zero out current entries */
  for (i=0;i<m*n;i++) tableau->t[i] = 0.0 ;

  /* copy the A constraint matrix into tableu */
  for (i=0;i<tableauConstraints( tableau );i++) {
    for (j=0;j<tableauDimension( tableau );j++) {
      t_index = (1+i)+(1+j)*m;
      A_index = i+j*tableauConstraints( tableau );
      tableau->t[t_index] = tableau->constraint_matrix[A_index];
    }
  }

  /* add identity slack variables */
  for (i=0;i<tableauConstraints( tableau );i++) {
    j = i+tableauDimension( tableau );
    t_index = (1+i)+(1+j)*m;
    tableau->t[t_index] = 1.0;
  }

  /* initial bfs */
  for (i=0;i<tableauConstraints(tableau);i++) 
    tableau->basis[i] = tableauDimension( tableau ) + i ;
  for (j=0;j<n;j++) tableau->in_basis[j] = EMPTY;
  for (i=0;i<tableauConstraints(tableau);i++) 
    tableau->in_basis[tableau->basis[i]+1] = i;

  for (i=0;i<tableauConstraints( tableau );i++) {
    t_index = (1+i);
    tableau->t[t_index] = tableau->constraint[i];
  }
  
  /* intial cost */ 
  tableau->t[0] = 0.0;

  /* Reduced costs */ 
  for (j=0;j<tableauDimension( tableau );j++) {
    t_index = m*(1+j);
    tableau->t[t_index] = tableau->cost[j];
  }

  return tableau;
}

Tableau *tableauAuxillaryPivot( Tableau *tableau, int *pivot_row, int *pivot_col )
{
  double zero;
  int m, column, t_index;
  int row;
  double x, best_divisor, pivot;
  int best_column;
 
  *pivot_row = EMPTY;
  *pivot_col = EMPTY;

  zero = 1.0e-14;

  m = 1 + tableauConstraints( tableau );

  for ( row = 0 ; row < tableauConstraints( tableau ) ; row++ ) {
    /* test if it is an auxillery basis */
    if ( tableau->basis[row] >= tableauDimension( tableau ) ) {
      x = tableau->t[1+row];
      best_column = EMPTY;
      best_divisor = 0.0;
      for ( column = 0 ; column < tableauDimension( tableau ) ; column++ ) {
	t_index = (1+row)+(1+column)*m;
	pivot = tableau->t[t_index];
	/* accept negative pivots if x is zero
	   because we will stay feasable */
	if ( x < zero ) pivot = ABS(pivot);
	if ( pivot > best_divisor ) {
	  best_divisor = pivot;
	  best_column = column;
	}
      }
      if ( EMPTY != best_column ){
	*pivot_row = row+1;
	*pivot_col = best_column+1;
	return tableau;
      }
    }
  }

  /* if there are no auxillery basis 
     or none to remove and stay feasable */
  return NULL;
}

Tableau *tableauLargestPivot( Tableau *tableau, int *pivot_row, int *pivot_col )
{
  int i,j;
  int m;

  double zero;

  double divisor, best_divisor;
  double reduced_cost;
  
  int best_row;
  double feasable_step_length, this_step_length;
  double pivot, x;

  *pivot_row = EMPTY;
  *pivot_col = EMPTY;

  zero = 1.0e-14;
  
  m = 1 + tableauConstraints( tableau );
  
  divisor = 0.0; /* to suppress uninitialized compiler warning */
  best_divisor = 0.0;

  /* test all basis (except slack), first col is solution */
  for (j=1;j<1 + tableauDimension( tableau );j++) { 
    if (EMPTY == tableau->in_basis[j]) { /* skip active basis */
      reduced_cost = tableau->t[m*j];
      if ( 0 > reduced_cost ) {  /* try a negative reduced cost */
	best_row = EMPTY;
	feasable_step_length = DBL_MAX;
	for (i=1;i<m;i++) {
	  pivot = tableau->t[i+m*j];
	  x = tableau->t[i];
	  if ( pivot > zero ) {
	    this_step_length =  x / pivot;
	    if ( this_step_length < feasable_step_length ) {
	      best_row = i;
	      feasable_step_length = this_step_length;
	      divisor = pivot;
	    }
	  }
	} /* end loop over rows */
	/* if this column has the best pivot so far, keep it */
	if ( EMPTY != best_row && ABS(divisor) > ABS(best_divisor) ) {
	  *pivot_row = best_row;
	  *pivot_col = j;
	  best_divisor = divisor;
	}
      }
    }
  }/* end loop over columns */
  if ( EMPTY == (*pivot_row) ) return NULL;
  return tableau;
}

Tableau *tableauSolve( Tableau *tableau )
{
  int row, column;
  int iteration, max_iterations;
  int i;

  if ( tableau != tableauInit( tableau ) ) {
    printf( "%s: %d: %s: tableauInit NULL\n",
	    __FILE__, __LINE__, "tableauSolve");
    return NULL;
  }

  while ( tableau == tableauAuxillaryPivot( tableau, &row, &column ) ) {
    tableauPivotAbout(tableau, row, column);
  }

  max_iterations = tableauDimension( tableau )*tableauDimension( tableau );
  iteration = 0;
  while ( tableau == tableauLargestPivot( tableau, &row, &column ) ) {
    iteration++;
    if ( iteration > max_iterations ){
      printf( "%s: %d: %s: max iterations %d exceeded\n",
	      __FILE__, __LINE__, "tableauSolve", max_iterations );
      return NULL;
    }
    tableauPivotAbout(tableau, row, column);
  }

  for (i=0;i<tableauConstraints(tableau);i++) {
    if ( tableau->basis[i] > tableauDimension(tableau)){
      printf( "%s: %d: %s: original basis[%d] %d has not been eliminated\n",
	      __FILE__, __LINE__, "tableauSolve", i, tableau->basis[i]);
      return NULL;
    }
  }

  return tableau;
}

Tableau *tableauPivotAbout( Tableau *tableau, int row, int column )
{
  int m, n;
  int i, j;
  double pivot;
  double factor;

  m = 1 + tableauConstraints( tableau );
  n = 1 + tableauDimension( tableau ) + tableauConstraints( tableau );

  if ( row < 1 || row >= m ||  column < 1 || column >= n ) {
    printf( "%s: %d: %s: requested row %d or column %d is outside %d m %d n\n",
	    __FILE__, __LINE__, "tableauPivotAbout",row,column,m,n);
    return NULL;
  }

  if (EMPTY != tableau->in_basis[column]) {
    printf( "%s: %d: %s: requested column %d is already active\n",
	    __FILE__, __LINE__, "tableauPivotAbout",column);
    return NULL;
  }

  tableau->in_basis[tableau->basis[row-1]+1] = EMPTY;
  tableau->basis[row-1] = column-1;
  tableau->in_basis[tableau->basis[row-1]+1] = row-1;

  /* normalize row */
  pivot = tableau->t[row+m*column];
  for (j=0;j<n;j++) {
    tableau->t[row+m*j] /= pivot;
  }

  for (i=0;i<m;i++) {
    if (i!=row) {
      factor = tableau->t[i+m*column];
      for (j=0;j<n;j++) {
	tableau->t[i+m*j] -= tableau->t[row+m*j]*factor;
      }
    }
  }

  return tableau;
}

Tableau *tableauTableau( Tableau *tableau, double *tab )
{
  int i;
  int m, n;
  int length;

  m = 1 + tableauConstraints( tableau );
  n = 1 + tableauDimension( tableau ) + tableauConstraints( tableau );
  length = m*n;

  for (i=0;i<length;i++) {
    tab[i] = tableau->t[i];
  }
  return tableau;
}

double tableauBound( Tableau *tableau )
{
  return -tableau->t[0];
}

Tableau *tableauShow( Tableau *tableau )
{
  int i, j;
  int m, n;

  m = 1 + tableauConstraints( tableau );
  n = 1 + tableauDimension( tableau ) + tableauConstraints( tableau );

  printf("\n");
  for (i=0;i<m;i++) {
    for (j=0;j<n;j++) {
      printf(" %10.6f",tableau->t[i+j*m]);
    }
    printf("\n");
  }
  return tableau;
}

Tableau *tableauShowTransposed( Tableau *tableau )
{
  int i, j;
  int m, n;

  m = 1 + tableauConstraints( tableau );
  n = 1 + tableauDimension( tableau ) + tableauConstraints( tableau );

  printf("\n");
  for (j=0;j<n;j++) {
    printf(" col%4d",j);
    for (i=0;i<m;i++) {
      printf(" %12.4e",tableau->t[i+j*m]);
    }
    printf("\n");
  }
  return tableau;
}
