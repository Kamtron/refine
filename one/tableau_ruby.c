
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

#include "ruby.h"
#include "tableau.h"

#define GET_TABLEAU_FROM_SELF Tableau *tableau; Data_Get_Struct( self, Tableau, tableau );

static void tableau_free( void *tableau )
{
  tableauFree( tableau );
}

VALUE tableau_new( VALUE class, VALUE rb_constraints, VALUE rb_dimension )
{
  Tableau *tableau;
  VALUE obj;
  tableau = tableauCreate( NUM2INT(rb_constraints), NUM2INT(rb_dimension) );
  obj = Data_Wrap_Struct( class, 0, tableau_free, tableau );
  return obj;
}

VALUE tableau_constraintMatrix( VALUE self, VALUE rb_data )
{
  int i, ndata;
  double *data;
  VALUE rb_result;
  GET_TABLEAU_FROM_SELF;
  ndata = RARRAY_LEN(rb_data);
  data = (double *)malloc(ndata*sizeof(double));
  for ( i=0 ; i < ndata ; i++ ) data[i] = NUM2DBL( rb_ary_entry( rb_data, i) );
  rb_result = (tableau == tableauConstraintMatrix(tableau, data)?self:Qnil);
  free(data);
  return rb_result;
}

VALUE tableau_cost( VALUE self, VALUE rb_data )
{
  int i, ndata;
  double *data;
  VALUE rb_result;
  GET_TABLEAU_FROM_SELF;
  ndata = RARRAY_LEN(rb_data);
  data = (double *)malloc(ndata*sizeof(double));
  for ( i=0 ; i < ndata ; i++ ) data[i] = NUM2DBL( rb_ary_entry( rb_data, i) );
  rb_result = (tableau == tableauCost(tableau, data)?self:Qnil);
  free(data);
  return rb_result;
}

VALUE tableau_constraint( VALUE self, VALUE rb_data )
{
  int i, ndata;
  double *data;
  VALUE rb_result;
  GET_TABLEAU_FROM_SELF;
  ndata = RARRAY_LEN(rb_data);
  data = (double *)malloc(ndata*sizeof(double));
  for ( i=0 ; i < ndata ; i++ ) data[i] = NUM2DBL( rb_ary_entry( rb_data, i) );
  rb_result = (tableau == tableauConstraint(tableau, data)?self:Qnil);
  free(data);
  return rb_result;
}

VALUE tableau_constraints( VALUE self )
{
  GET_TABLEAU_FROM_SELF;
  return INT2NUM( tableauConstraints(tableau) );
}

VALUE tableau_dimension( VALUE self )
{
  GET_TABLEAU_FROM_SELF;
  return INT2NUM( tableauDimension(tableau) );
}

VALUE tableau_basis( VALUE self )
{
  VALUE rb_basis;
  int i, nbasis;
  int *basis;
  GET_TABLEAU_FROM_SELF;
  nbasis = tableauConstraints(tableau);
  basis = (int *)malloc(nbasis*sizeof(int));
  if (tableau != tableauBasis(tableau, basis) ) {
    free(basis);
    return Qnil;
  }
  rb_basis = rb_ary_new2(nbasis);
  for ( i=0 ; i < nbasis ; i++ ) 
    rb_ary_store( rb_basis, i, INT2NUM(basis[i]) );
  free(basis);
  return rb_basis;
}

VALUE tableau_init( VALUE self )
{
  GET_TABLEAU_FROM_SELF;
  return ( tableau == tableauInit(tableau) ? self : Qnil );
}

VALUE tableau_largestPivot( VALUE self )
{
  VALUE rb_pivot;
  int pivot_row, pivot_col;
  GET_TABLEAU_FROM_SELF;
  if (tableau != tableauLargestPivot(tableau, &pivot_row, &pivot_col) )
    return Qnil;
  rb_pivot = rb_ary_new2(2);
  rb_ary_store( rb_pivot, 0, INT2NUM(pivot_row) );
  rb_ary_store( rb_pivot, 1, INT2NUM(pivot_col) );
  return rb_pivot;
}

VALUE tableau_tableau( VALUE self )
{
  int m, n;
  int i, j;
  int length;
  double *tab;
  VALUE rb_tab;
  GET_TABLEAU_FROM_SELF;
  
  m = 1 + tableauConstraints( tableau );
  n = 1 + tableauDimension( tableau ) + tableauConstraints( tableau );

  length = m*n;

  tab = (double *)malloc(length*sizeof(double));
  if (tableau != tableauTableau(tableau,tab) ) {
    rb_tab = Qnil;
  } else {
    rb_tab = rb_ary_new2(n);
    for (j=0;j<n;j++) {
      rb_ary_store( rb_tab, j, rb_ary_new2(m) );
      for (i=0;i<m;i++) {
	rb_ary_store( rb_ary_entry( rb_tab, j), i, rb_float_new(tab[i+m*j]) );
      }
    }
  }
  free(tab);
  return rb_tab;
}

VALUE tableau_solve( VALUE self )
{
  GET_TABLEAU_FROM_SELF;
  return ( tableau == tableauSolve(tableau) ? self : Qnil );
}

VALUE tableau_show( VALUE self )
{
  GET_TABLEAU_FROM_SELF;
  return ( tableau == tableauShow(tableau) ? self : Qnil );
}

VALUE cTableau;

void Init_Tableau() 
{
  cTableau = rb_define_class( "Tableau", rb_cObject );
  rb_define_singleton_method( cTableau, "new", tableau_new, 2 );

  rb_define_method( cTableau, "constraintMatrix", tableau_constraintMatrix, 1);
  rb_define_method( cTableau, "constraint", tableau_constraint, 1);
  rb_define_method( cTableau, "cost", tableau_cost, 1);

  rb_define_method( cTableau, "constraints", tableau_constraints, 0 );
  rb_define_method( cTableau, "dimension", tableau_dimension, 0 );

  rb_define_method( cTableau, "basis", tableau_basis, 0 );

  rb_define_method( cTableau, "init", tableau_init, 0 );
  rb_define_method( cTableau, "largestPivot", tableau_largestPivot, 0 );
  rb_define_method( cTableau, "tableau", tableau_tableau, 0 );

  rb_define_method( cTableau, "solve", tableau_solve, 0 );
  rb_define_method( cTableau, "show", tableau_show, 0 );
}
