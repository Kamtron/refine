
#include "ruby.h"
#include "gridmetric.h"

#define GET_GRID_FROM_SELF Grid *grid; Data_Get_Struct( self, Grid, grid );

VALUE grid_volume( VALUE self, VALUE rb_nodes )
{
  int i, nodes[4];
  GET_GRID_FROM_SELF;
  for ( i=0 ; i<4 ; i++ ) nodes[i] = NUM2INT(rb_ary_entry(rb_nodes,i));
  return rb_float_new( gridVolume( grid, nodes ) );
}

VALUE grid_ar( VALUE self, VALUE rb_nodes )
{
  int i, nodes[4];
  GET_GRID_FROM_SELF;
  for ( i=0 ; i<4 ; i++ ) nodes[i] = NUM2INT(rb_ary_entry(rb_nodes,i));
  return rb_float_new( gridAR( grid, nodes ) );
}

VALUE grid_arDerivative( VALUE self, VALUE node )
{
  VALUE rb_ar;
  double ar, dARdx[3];
  Grid *returnedGrid;
  GET_GRID_FROM_SELF;
  returnedGrid = gridARDervative( grid, NUM2INT(node), &ar, dARdx );
  if ( returnedGrid == grid ){
    rb_ar = rb_ary_new2(4);
    rb_ary_store( rb_ar, 0, rb_float_new(ar) );
    rb_ary_store( rb_ar, 1, rb_float_new(dARdx[0]) );
    rb_ary_store( rb_ar, 2, rb_float_new(dARdx[1]) );
    rb_ary_store( rb_ar, 3, rb_float_new(dARdx[2]) );
  }else{
    rb_ar = Qnil;
  }
  return rb_ar;
}

VALUE grid_minVolume( VALUE self )
{
  GET_GRID_FROM_SELF;
  return rb_float_new( gridMinVolume( grid ) );
}

VALUE grid_minAR( VALUE self )
{
  GET_GRID_FROM_SELF;
  return rb_float_new( gridMinAR( grid ) );
}


VALUE grid_rightHandedFace( VALUE self, VALUE face )
{
  GET_GRID_FROM_SELF;
  return (gridRightHandedFace(grid, NUM2INT(face))?Qtrue:Qfalse);
}

VALUE grid_rightHandedBoundary( VALUE self )
{
  GET_GRID_FROM_SELF;
  return (gridRightHandedBoundary(grid)?Qtrue:Qfalse);
}

VALUE cGridMetric;

void Init_GridMetric() 
{
  cGridMetric = rb_define_module( "GridMetric" );
  rb_define_method( cGridMetric, "volume", grid_volume, 1 );
  rb_define_method( cGridMetric, "ar", grid_ar, 1 );
  rb_define_method( cGridMetric, "arDerivative", grid_arDerivative, 1 );
  rb_define_method( cGridMetric, "minVolume", grid_minVolume, 0 );
  rb_define_method( cGridMetric, "minAR", grid_minAR, 0 );

  rb_define_method( cGridMetric, "rightHandedFace", grid_rightHandedFace, 1 );
  rb_define_method( cGridMetric, "rightHandedBoundary", grid_rightHandedBoundary, 0);
}
