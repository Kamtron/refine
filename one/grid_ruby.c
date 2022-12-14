
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

#include <math.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ruby.h"
#include "grid.h"

#define GET_GRID_FROM_SELF Grid *grid; Data_Get_Struct( self, Grid, grid );

static void grid_free( void *grid )
{
  gridFree( grid );
}

VALUE grid_init( VALUE self ) // not needed but for example
{
  return self;
}

VALUE grid_new( VALUE class, VALUE nnode, VALUE ncell, VALUE nface, VALUE nedge)
{
  Grid *grid;
  VALUE obj;
  grid = gridCreate( NUM2INT(nnode), NUM2INT(ncell), NUM2INT(nface), NUM2INT(nedge) );
  if ( NULL == grid ) return Qnil;
  obj = Data_Wrap_Struct( class, 0, grid_free, grid );
  return obj;
}

VALUE grid_from_mesh3d( VALUE class, VALUE rb_filename)
{
  Grid *grid;
  VALUE obj;
  grid = gridImportMesh3D( RSTRING_PTR(rb_filename) );
  if ( NULL == grid ) return Qnil;
  obj = Data_Wrap_Struct( class, 0, grid_free, grid );
  return obj;
}

VALUE grid_from_NGP( VALUE class, VALUE rb_filename)
{
  Grid *grid;
  VALUE obj;
  grid = gridImportNGP( RSTRING_PTR(rb_filename) );
  if ( NULL == grid ) return Qnil;
  obj = Data_Wrap_Struct( class, 0, grid_free, grid );
  return obj;
}

VALUE grid_from_aflr3( VALUE class, VALUE rb_filename)
{
  Grid *grid;
  VALUE obj;
  grid = gridImportAFLR3( RSTRING_PTR(rb_filename) );
  if ( NULL == grid ) return Qnil;
  obj = Data_Wrap_Struct( class, 0, grid_free, grid );
  return obj;
}

VALUE grid_from_FAST( VALUE class, VALUE rb_filename)
{
  Grid *grid;
  VALUE obj;
  grid = gridImportFAST( RSTRING_PTR(rb_filename) );
  if ( NULL == grid ) return Qnil;
  obj = Data_Wrap_Struct( class, 0, grid_free, grid );
  return obj;
}

VALUE grid_from_front( VALUE class, VALUE rb_filename)
{
  Grid *grid;
  VALUE obj;
  grid = gridImportFront( RSTRING_PTR(rb_filename) );
  if ( NULL == grid ) return Qnil;
  obj = Data_Wrap_Struct( class, 0, grid_free, grid );
  return obj;
}

VALUE grid_pack( VALUE self )
{
  GET_GRID_FROM_SELF;
  return (gridPack(grid)==grid?self:Qnil);
}

VALUE grid_sortNodeGridEx( VALUE self )
{
  GET_GRID_FROM_SELF;
  return (gridSortNodeGridEx(grid)==grid?self:Qnil);
}

VALUE grid_writeTecplotSurfaceGeom( VALUE self, VALUE rb_filename )
{
  GET_GRID_FROM_SELF;
  return (gridWriteTecplotSurfaceGeom(grid,RSTRING_PTR(rb_filename))==grid?self:Qnil);
}

VALUE grid_writeTecplotVolumeGeom( VALUE self, VALUE rb_filename )
{
  GET_GRID_FROM_SELF;
  return (gridWriteTecplotVolumeGeom(grid,RSTRING_PTR(rb_filename))==grid?self:Qnil);
}

VALUE grid_writeTecplotGeomFaceXYZ( VALUE self, VALUE faceid )
{
  GET_GRID_FROM_SELF;
  return (gridWriteTecplotGeomFaceXYZ(grid,NULL,
				      NUM2INT(faceid))==grid?self:Qnil);
}

VALUE grid_writeTecplotEquator( VALUE self, VALUE node0, VALUE node1 )
{
  GET_GRID_FROM_SELF;
  return (gridWriteTecplotEquator( grid,
				   NUM2INT(node0),
				   NUM2INT(node1),NULL)==grid?self:Qnil);
}

VALUE grid_writeTecplotSurfaceScalar( VALUE self, VALUE rb_scalar )
{
  int length, i;
  double *scalar;
  VALUE value;
  GET_GRID_FROM_SELF;
  length = RARRAY_LEN(rb_scalar);
  scalar = (double *)malloc(length*sizeof(double));
  for(i=0;i<length;i++) scalar[i] = NUM2DBL( rb_ary_entry( rb_scalar, i) );
  value = (grid==gridWriteTecplotSurfaceScalar(grid,NULL,scalar)?self:Qnil);
  free(scalar);
  return value;
}

VALUE grid_writeVTK( VALUE self, VALUE rb_filename )
{
  GET_GRID_FROM_SELF;
  return (gridWriteVTK(grid,RSTRING_PTR(rb_filename))==grid?self:Qnil);
}

VALUE grid_exportNGP( VALUE self, VALUE rb_filename)
{
  GET_GRID_FROM_SELF;
  return (gridExportNGP(grid, RSTRING_PTR(rb_filename) )==grid?self:Qnil);
}

VALUE grid_exportFAST( VALUE self, VALUE rb_filename)
{
  GET_GRID_FROM_SELF;
  return (gridExportFAST(grid, RSTRING_PTR(rb_filename))==grid?self:Qnil);
}

VALUE grid_exportFASTSurface( VALUE self, VALUE rb_filename)
{
  GET_GRID_FROM_SELF;
  return (gridExportFASTSurface(grid,RSTRING_PTR(rb_filename))==grid?self:Qnil);
}

VALUE grid_exportGRI( VALUE self, VALUE rb_filename )
{
  GET_GRID_FROM_SELF;
  return (gridExportGRI(grid, RSTRING_PTR(rb_filename))==grid?self:Qnil);
}

VALUE grid_exportOFF( VALUE self, VALUE rb_filename )
{
  GET_GRID_FROM_SELF;
  return (gridExportOFF(grid, RSTRING_PTR(rb_filename))==grid?self:Qnil);
}

VALUE grid_exportAFLR3( VALUE self, VALUE rb_filename )
{
  GET_GRID_FROM_SELF;
  return (gridExportAFLR3(grid, RSTRING_PTR(rb_filename))==grid?self:Qnil);
}

VALUE grid_maxnode( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridMaxNode(grid) );
}

VALUE grid_nnode( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNNode(grid) );
}

VALUE grid_maxcell( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridMaxCell(grid) );
}

VALUE grid_ncell( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNCell(grid) );
}

VALUE grid_maxface( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridMaxFace(grid) );
}

VALUE grid_nface( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNFace(grid) );
}

VALUE grid_maxedge( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridMaxEdge(grid) );
}

VALUE grid_nedge( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNEdge(grid) );
}

VALUE grid_nprism( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNPrism(grid) );
}

VALUE grid_npyramid( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNPyramid(grid) );
}

VALUE grid_nquad( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNQuad(grid) );
}

VALUE grid_naux( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNAux(grid) );
}

VALUE grid_setNAux( VALUE self, VALUE naux )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridSetNAux(grid,NUM2INT(naux))?self:Qnil );
}

VALUE grid_aux( VALUE self, VALUE node, VALUE aux )
{
  GET_GRID_FROM_SELF;
  return rb_float_new( gridAux(grid,NUM2INT(node),NUM2INT(aux)) );
}

VALUE grid_setAux( VALUE self, VALUE node, VALUE aux, VALUE value )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridSetAux(grid,NUM2INT(node),NUM2INT(aux),NUM2DBL(value))?self:Qnil );
}

VALUE grid_interpolateAux2( VALUE self, 
			    VALUE node0, VALUE node1, VALUE ratio, 
			    VALUE target )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridInterpolateAux2(grid,
				       NUM2INT(node0),
				       NUM2INT(node1),
				       NUM2DBL(ratio),
				       NUM2INT(target) )?self:Qnil );
}

VALUE grid_partId( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridPartId(grid) );
}

VALUE grid_setPartId( VALUE self, VALUE partId )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridSetPartId(grid,NUM2INT(partId))?self:Qnil );
}

VALUE grid_globalnnode( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridGlobalNNode(grid) );
}

VALUE grid_setGlobalNNode( VALUE self, VALUE nglobal )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridSetGlobalNNode(grid,NUM2INT(nglobal))?self:Qnil );
}

VALUE grid_nUnusedNodeGlobal( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNUnusedNodeGlobal(grid) );
}

VALUE grid_getUnusedNodeGlobal( VALUE self )
{
  int i, n, *unused;
  VALUE rb_unused;
  GET_GRID_FROM_SELF;
  n = gridNUnusedNodeGlobal(grid);
  unused = malloc( n * sizeof(int) );
  if ( grid != gridGetUnusedNodeGlobal(grid,unused) ) {
    free(unused);
    return Qnil;
  } else {
    rb_unused = rb_ary_new2(n);
    for (i=0;i<n;i++) rb_ary_store( rb_unused, i, INT2NUM(unused[i]) );
    return rb_unused;
  }
}

VALUE grid_joinUnusedNodeGlobal( VALUE self, VALUE global )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridJoinUnusedNodeGlobal(grid,NUM2INT(global))?self:Qnil );
}

VALUE grid_eliminateUnusedNodeGlobal( VALUE self )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridEliminateUnusedNodeGlobal(grid)?self:Qnil );
}

VALUE grid_addCell( VALUE self, VALUE n0, VALUE n1, VALUE n2, VALUE n3 )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridAddCell( grid, 
			       NUM2INT(n0),  NUM2INT(n1),  
			       NUM2INT(n2),  NUM2INT(n3) ) );
}

VALUE grid_removeCell( VALUE self, VALUE cellId )
{
  GET_GRID_FROM_SELF;
  return (grid==gridRemoveCell( grid, NUM2INT(cellId) )?self:Qnil);
}

VALUE grid_reconnectAllCell( VALUE self, VALUE oldNode, VALUE newNode )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridReconnectAllCell( grid, 
					 NUM2INT(oldNode), 
					 NUM2INT(newNode) )?self:Qnil);
}

VALUE grid_cell( VALUE self, VALUE cellId )
{
  int i, nodes[4];
  VALUE rb_nodes;
  GET_GRID_FROM_SELF;
  if ( NULL == gridCell( grid, NUM2INT(cellId), nodes ) ) return Qnil;
  rb_nodes = rb_ary_new2(4);
  for ( i=0 ; i < 4 ; i++ ) rb_ary_store( rb_nodes, i, INT2NUM(nodes[i]) );
  return rb_nodes;
}

VALUE grid_cellDegree( VALUE self, VALUE nodeId )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridCellDegree(grid, NUM2INT(nodeId) ) );
}

VALUE grid_cellEdge( VALUE self, VALUE node0, VALUE node1 )
{
  GET_GRID_FROM_SELF;
  return (gridCellEdge(grid, NUM2INT(node0), NUM2INT(node1) )?Qtrue:Qfalse);
}

VALUE grid_cellFace( VALUE self, VALUE n0, VALUE n1, VALUE n2)
{
  GET_GRID_FROM_SELF;
  return (gridCellFace(grid, 
		       NUM2INT(n0), NUM2INT(n1), NUM2INT(n2) )?Qtrue:Qfalse);
}
VALUE grid_findCellWithFace( VALUE self, VALUE face )
{
  int returnedCell;
  GET_GRID_FROM_SELF;
  returnedCell = gridFindCellWithFace(grid, NUM2INT(face) );
  if (returnedCell == EMPTY) return Qnil;
  return INT2NUM( returnedCell );
}

VALUE grid_findOtherCellWith3Nodes( VALUE self, VALUE node0, VALUE node1, VALUE node2, VALUE currentCell )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridFindOtherCellWith3Nodes(grid, NUM2INT(node0), NUM2INT(node1), NUM2INT(node2), NUM2INT(currentCell) ) );
}

VALUE grid_findCell( VALUE self, VALUE rb_nodes )
{
  int nodes[4];
  GET_GRID_FROM_SELF;
  nodes[0] = NUM2INT( rb_ary_entry( rb_nodes, 0) );
  nodes[1] = NUM2INT( rb_ary_entry( rb_nodes, 1) );
  nodes[2] = NUM2INT( rb_ary_entry( rb_nodes, 2) );
  nodes[3] = NUM2INT( rb_ary_entry( rb_nodes, 3) );
  return INT2NUM( gridFindCell( grid, nodes ) );
}

VALUE grid_findEnclosingCell( VALUE self, VALUE rb_guess, VALUE rb_target )
{
  int i, guess, enclosing_cell;
  double target[3], bary[4];
  VALUE rb_find;
  GET_GRID_FROM_SELF;
  guess = NUM2INT(rb_guess); 
  for (i=0;i<3;i++) target[i] = NUM2DBL(rb_ary_entry(rb_target, i));
  enclosing_cell = gridFindEnclosingCell( grid, guess, target, bary );
  if ( EMPTY == enclosing_cell ) {
    return Qnil;
  }else{
    rb_find = rb_ary_new2(5);
    rb_ary_store( rb_find, 0, INT2NUM(enclosing_cell) );
    for(i=0;i<4;i++) rb_ary_store( rb_find, i+1, rb_float_new(bary[i]) );
    return rb_find;
  }
}

VALUE grid_nconn( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNConn(grid) );
}

VALUE grid_cell2Conn( VALUE self, VALUE cell, VALUE index )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridCell2Conn( grid, NUM2INT(cell), NUM2INT(index) ) );
}

VALUE grid_conn2Node( VALUE self, VALUE conn )
{
  int nodes[2];
  VALUE rb_nodes;
  GET_GRID_FROM_SELF;
  if (grid != gridConn2Node(grid,NUM2INT(conn),nodes) ) return Qnil;
  rb_nodes = rb_ary_new2(2);
  rb_ary_store( rb_nodes, 0, INT2NUM(nodes[0]) );
  rb_ary_store( rb_nodes, 1, INT2NUM(nodes[1]) );
  return rb_nodes;
}

VALUE grid_findConn( VALUE self, VALUE node0, VALUE node1 )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridFindConn( grid, NUM2INT(node0), NUM2INT(node1) ) );
}

VALUE grid_createConn( VALUE self )
{
  GET_GRID_FROM_SELF;
  return (gridCreateConn(grid )==NULL?Qnil:self);
}  

VALUE grid_eraseConn( VALUE self )
{
  GET_GRID_FROM_SELF;
  return (gridEraseConn(grid )==NULL?Qnil:self);
}  

VALUE grid_addFace( VALUE self, VALUE n0, VALUE n1, VALUE n2, VALUE faceId )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridAddFace(grid, NUM2INT(n0), NUM2INT(n1), NUM2INT(n2), 
			      NUM2INT(faceId)) );
}

VALUE grid_addFaceUV( VALUE self, 
		      VALUE n0, VALUE u0, VALUE v0,
		      VALUE n1, VALUE u1, VALUE v1,
		      VALUE n2, VALUE u2, VALUE v2, VALUE faceId )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridAddFaceUV(grid, 
				NUM2INT(n0), NUM2DBL(u0), NUM2DBL(v0),
				NUM2INT(n1), NUM2DBL(u1), NUM2DBL(v1), 
				NUM2INT(n2), NUM2DBL(u2), NUM2DBL(v2), 
				NUM2INT(faceId)) );
}

VALUE grid_removeFace( VALUE self, VALUE face )
{
  GET_GRID_FROM_SELF;
  return (gridRemoveFace(grid, NUM2INT(face) )==NULL?Qnil:self);
}

VALUE grid_findFace( VALUE self, VALUE n0, VALUE n1, VALUE n2 )
{
  int returnedFace;
  GET_GRID_FROM_SELF;
  returnedFace = gridFindFace(grid, NUM2INT(n0), NUM2INT(n1), NUM2INT(n2) );
  if (returnedFace == EMPTY) return Qnil;
  return INT2NUM( returnedFace );
}

VALUE grid_faceId( VALUE self, VALUE n0, VALUE n1, VALUE n2 )
{
  int returnedFace;
  GET_GRID_FROM_SELF;
  returnedFace = gridFaceId( grid, NUM2INT(n0), NUM2INT(n1), NUM2INT(n2) );
  if (returnedFace == EMPTY) return Qnil;
  return INT2NUM( returnedFace );
}

VALUE grid_findFaceWithNodesUnless( VALUE self, VALUE n0, VALUE n1, 
				    VALUE unless_face )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridFindFaceWithNodesUnless(grid, NUM2INT(n0), NUM2INT(n1), 
					      NUM2INT(unless_face) ) );
}

VALUE grid_reconnectAllFace( VALUE self, VALUE oldNode, VALUE newNode )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridReconnectAllFace( grid, 
					 NUM2INT(oldNode), 
					 NUM2INT(newNode) )?self:Qnil);
}

VALUE grid_reconnectionOfAllFacesOK( VALUE self, VALUE oldNode, VALUE newNode )
{
  GET_GRID_FROM_SELF;
  return ( gridReconnectionOfAllFacesOK( grid, 
					 NUM2INT(oldNode), 
					 NUM2INT(newNode) )?Qtrue:Qfalse);
}

VALUE grid_face( VALUE self, VALUE face )
{
  int id, nodes[3];
  VALUE rb_face;
  GET_GRID_FROM_SELF;
  if (grid != gridFace(grid, NUM2INT(face), nodes, &id )) return Qnil;
  rb_face = rb_ary_new2(4);
  rb_ary_store( rb_face, 0, INT2NUM(nodes[0]) );
  rb_ary_store( rb_face, 1, INT2NUM(nodes[1]) );
  rb_ary_store( rb_face, 2, INT2NUM(nodes[2]) );
  rb_ary_store( rb_face, 3, INT2NUM(id) );
  return rb_face;
}

VALUE grid_deleteThawedFaces( VALUE self, VALUE faceId )
{
  GET_GRID_FROM_SELF;
  return (gridDeleteThawedFaces(grid, NUM2INT(faceId) )==NULL?Qnil:self);
}

VALUE grid_nThawedFaces( VALUE self, VALUE faceId )
{
  GET_GRID_FROM_SELF;
  return INT2NUM(gridNThawedFaces(grid, NUM2INT(faceId) ));
}

VALUE grid_nodeUV( VALUE self, VALUE node, VALUE faceId )
{
  VALUE rb_uv;
  double uv[2];
  Grid *returnedGrid;
  GET_GRID_FROM_SELF;
  returnedGrid = 
    gridNodeUV( grid, NUM2INT(node), NUM2INT(faceId), uv );
  if ( returnedGrid == grid ){
    rb_uv = rb_ary_new2(2);
    rb_ary_store( rb_uv, 0, rb_float_new(uv[0]) );
    rb_ary_store( rb_uv, 1, rb_float_new(uv[1]) );
  }else{
    rb_uv = Qnil;
  }
  return rb_uv;
}

VALUE grid_setNodeUV( VALUE self, VALUE node, VALUE faceId, VALUE u, VALUE v )
{
  GET_GRID_FROM_SELF;
  return (gridSetNodeUV( grid, NUM2INT(node), NUM2INT(faceId), 
			 NUM2DBL(u), NUM2DBL(v) )==grid?self:Qnil);
}

VALUE grid_nodeFaceIdDegree( VALUE self, VALUE node )
{
  GET_GRID_FROM_SELF;
  return INT2NUM(gridNodeFaceIdDegree( grid, NUM2INT(node) ));
}

VALUE grid_nodeFaceId( VALUE self, VALUE node )
{
  VALUE rb_ids;
  int i, ids, id[MAXFACEIDDEG];
  GET_GRID_FROM_SELF;
  if (grid == gridNodeFaceId(grid, NUM2INT(node), MAXFACEIDDEG, &ids, id )) {
    rb_ids = rb_ary_new2(ids);
    for(i=0;i<ids;i++) rb_ary_store( rb_ids, i, INT2NUM(id[i]) );
    return rb_ids;
  } else {
    return Qnil;
  }
}

VALUE grid_trianglesOnFaceId( VALUE self, VALUE id )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridTrianglesOnFaceId( grid, NUM2INT(id) ) );
}

VALUE grid_nodeT( VALUE self, VALUE node, VALUE edgeId )
{
  double t;
  GET_GRID_FROM_SELF;
  if ( grid == gridNodeT( grid, NUM2INT(node), NUM2INT(edgeId), &t ))
    return rb_float_new(t);
  return Qnil;
}

VALUE grid_setNodeT( VALUE self, VALUE node, VALUE edgeId, VALUE t )
{
  GET_GRID_FROM_SELF;
  return (gridSetNodeT( grid, NUM2INT(node), NUM2INT(edgeId), 
			NUM2DBL(t) )==grid?self:Qnil);
}

VALUE grid_nodeEdgeIdDegree( VALUE self, VALUE node )
{
  GET_GRID_FROM_SELF;
  return INT2NUM(gridNodeEdgeIdDegree( grid, NUM2INT(node) ));
}

VALUE grid_nodeEdgeId( VALUE self, VALUE node )
{
  VALUE rb_ids;
  int i, ids, id[MAXEDGEIDDEG];
  GET_GRID_FROM_SELF;
  if (grid == gridNodeEdgeId(grid, NUM2INT(node), MAXEDGEIDDEG, &ids, id )) {
    rb_ids = rb_ary_new2(ids);
    for(i=0;i<ids;i++) rb_ary_store( rb_ids, i, INT2NUM(id[i]) );
    return rb_ids;
  } else {
    return Qnil;
  }
}

VALUE grid_addEdge( VALUE self, VALUE n0, VALUE n1, 
		    VALUE edgeId, VALUE t0, VALUE t1 )
{
  GET_GRID_FROM_SELF;
  return INT2NUM(gridAddEdge(grid, NUM2INT(n0), NUM2INT(n1), 
			     NUM2INT(edgeId), NUM2DBL(t0), NUM2DBL(t1) ));
}

VALUE grid_removeEdge( VALUE self, VALUE edge )
{
  GET_GRID_FROM_SELF;
  return (gridRemoveEdge(grid, NUM2INT(edge) )==NULL?Qnil:self);
}

VALUE grid_findEdge( VALUE self, VALUE n0, VALUE n1 )
{
  int returnedEdge;
  GET_GRID_FROM_SELF;
  returnedEdge = gridFindEdge(grid, NUM2INT(n0), NUM2INT(n1) );
  if (returnedEdge == EMPTY) return Qnil;
  return INT2NUM( returnedEdge );
}

VALUE grid_edgeId( VALUE self, VALUE n0, VALUE n1 )
{
  int returnedEdge;
  GET_GRID_FROM_SELF;
  returnedEdge = gridEdgeId( grid, NUM2INT(n0), NUM2INT(n1) );
  if (returnedEdge == EMPTY) return Qnil;
  return INT2NUM( returnedEdge );
}

VALUE grid_edge( VALUE self, VALUE edge )
{
  int id, nodes[2];
  VALUE rb_edge;
  GET_GRID_FROM_SELF;
  if (grid != gridEdge(grid, NUM2INT(edge), nodes, &id )) return Qnil;
  rb_edge = rb_ary_new2(3);
  rb_ary_store( rb_edge, 0, INT2NUM(nodes[0]) );
  rb_ary_store( rb_edge, 1, INT2NUM(nodes[1]) );
  rb_ary_store( rb_edge, 2, INT2NUM(id) );
  return rb_edge;
}

VALUE grid_deleteThawedEdgeSegments( VALUE self, VALUE edgeId )
{
  GET_GRID_FROM_SELF;
  return (gridDeleteThawedEdgeSegments(grid, NUM2INT(edgeId) )==NULL?Qnil:self);
}

VALUE grid_nThawedEdgeSegments( VALUE self, VALUE edgeId )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNThawedEdgeSegments(grid, NUM2INT(edgeId) ) );
}

VALUE grid_geomCurveSize( VALUE self, VALUE edgeId, VALUE startNode )
{
  GET_GRID_FROM_SELF;
  return 
    INT2NUM( gridGeomCurveSize( grid, NUM2INT(edgeId), NUM2INT(startNode) ) );
}

VALUE grid_geomCurve( VALUE self, VALUE edgeId, VALUE startNode )
{
  int ncurvenode, i, *curve;
  VALUE rb_curve;
  Grid *returnedGrid;
  GET_GRID_FROM_SELF;
  ncurvenode = gridGeomCurveSize( grid, NUM2INT(edgeId), NUM2INT(startNode) );
  if (ncurvenode < 2) return Qnil;
  curve = malloc(ncurvenode*sizeof(int));
  returnedGrid = 
    gridGeomCurve( grid, NUM2INT(edgeId), NUM2INT(startNode), curve );
  if ( returnedGrid == grid ){
    rb_curve = rb_ary_new2(ncurvenode);
    for ( i=0 ; i < ncurvenode ; i++ ) 
      rb_ary_store( rb_curve, i, INT2NUM(curve[i]) );
  }else{
    rb_curve = Qnil;
  }
  free(curve);
  return rb_curve;
}

VALUE grid_geomCurveT( VALUE self, VALUE edgeId, VALUE startNode )
{
  int ncurvenode, i;
  double *curve;
  VALUE rb_curve;
  Grid *returnedGrid;
  GET_GRID_FROM_SELF;
  ncurvenode = gridGeomCurveSize( grid, NUM2INT(edgeId), NUM2INT(startNode) );
  if (ncurvenode < 2) return Qnil;
  curve = malloc(ncurvenode*sizeof(double));
  returnedGrid = 
    gridGeomCurveT( grid, NUM2INT(edgeId), NUM2INT(startNode), curve );
  if ( returnedGrid == grid ){
    rb_curve = rb_ary_new2(ncurvenode);
    for ( i=0 ; i < ncurvenode ; i++ ) 
      rb_ary_store( rb_curve, i, rb_float_new(curve[i]) );
  }else{
    rb_curve = Qnil;
  }
  free(curve);
  return rb_curve;
}

VALUE grid_nfrozen( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNFrozen( grid ) );
}

VALUE grid_nodeFrozen( VALUE self, VALUE node )
{
  GET_GRID_FROM_SELF;
  return ( gridNodeFrozen( grid, NUM2INT(node) ) ? Qtrue : Qfalse );
}

VALUE grid_freezeNode( VALUE self, VALUE node )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridFreezeNode( grid, NUM2INT(node) ) ? self : Qnil );
}

VALUE grid_thawNode( VALUE self, VALUE node )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridThawNode( grid, NUM2INT(node) ) ? self : Qnil );
}

VALUE grid_freezeAll( VALUE self )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridFreezeAll( grid ) ? self : Qnil );
}

VALUE grid_thawAll( VALUE self )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridThawAll( grid ) ? self : Qnil );
}

VALUE grid_gem( VALUE self, VALUE n0, VALUE n1 )
{
  VALUE rb_gem;
  int i, ngem;
  GET_GRID_FROM_SELF;
  gridMakeGem( grid, NUM2INT(n0), NUM2INT(n1) );
  ngem = gridNGem(grid);
  rb_gem = rb_ary_new2(ngem);
  for ( i=0 ; i < ngem ; i++ ) 
    rb_ary_store( rb_gem, i, INT2NUM(gridGem(grid,i)) );
  return rb_gem;
}

VALUE grid_gemIsAllLocal( VALUE self )
{
  GET_GRID_FROM_SELF;
  return( gridGemIsAllLocal( grid )?Qtrue:Qfalse);
}

VALUE grid_equator( VALUE self, VALUE n0, VALUE n1 )
{
  int nequ, i;
  VALUE rb_equ;
  GET_GRID_FROM_SELF;
  if ( NULL == gridEquator( grid, NUM2INT(n0), NUM2INT(n1) ) ) return Qnil;
  nequ = gridNEqu(grid);
  if (nequ>0) {
    rb_equ = rb_ary_new2(nequ+1);
    for ( i=0 ; i < nequ ; i++ ) 
      rb_ary_store( rb_equ, i, INT2NUM( gridEqu(grid,i) ) );
    rb_ary_store( rb_equ, nequ, INT2NUM( gridEqu(grid,nequ) ) );
  }else{
    rb_equ = rb_ary_new2(0);
  }
  return rb_equ;
}

VALUE grid_nequ( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM(gridNEqu( grid ));
}

VALUE grid_faceOppositeCellNode( VALUE self, VALUE rb_nodes, VALUE node )
{
  VALUE rb_face;
  int i, nodes[4], face[3];
  GET_GRID_FROM_SELF;

  for (i=0;i<4;i++) nodes[i] =  NUM2DBL(rb_ary_entry(rb_nodes,i));

  if ( NULL == gridFaceOppositeCellNode( grid, nodes, NUM2INT(node),
					face ) ) return Qnil;

  rb_face = rb_ary_new2(3);
  for (i=0;i<3;i++) rb_ary_store( rb_face, i, INT2NUM(face[i]) );
  return rb_face;
}

VALUE grid_orient( VALUE self, VALUE c0, VALUE c1 , VALUE c2, VALUE c3,
		   VALUE n0, VALUE n1 )
{
  VALUE rb_result;
  int i, cell[4], result[4];
  GET_GRID_FROM_SELF;

  cell[0] = NUM2INT(c0);
  cell[1] = NUM2INT(c1);
  cell[2] = NUM2INT(c2);
  cell[3] = NUM2INT(c3);

  result[0] = NUM2INT(n0);
  result[1] = NUM2INT(n1);

  if ( NULL == gridOrient( grid, cell, result ) ) return Qnil;

  rb_result = rb_ary_new2(4);
  for ( i=0 ; i<4 ; i++ ) rb_ary_store( rb_result, i, INT2NUM(result[i]) );
  return rb_result;
}

VALUE grid_addNode( VALUE self, VALUE x, VALUE y, VALUE z )
{
  int returnedNode;
  GET_GRID_FROM_SELF;
  returnedNode = gridAddNode( grid, NUM2DBL(x), NUM2DBL(y), NUM2DBL(z) );
  if (returnedNode == EMPTY) return Qnil;
  return INT2NUM( returnedNode );
}

VALUE grid_addNodeWithGlobal( VALUE self, VALUE x, VALUE y, VALUE z, 
			      VALUE global )
{
  int returnedNode;
  GET_GRID_FROM_SELF;
  returnedNode = gridAddNodeWithGlobal( grid, 
					NUM2DBL(x), NUM2DBL(y), NUM2DBL(z),
					NUM2INT(global) );
  if (returnedNode == EMPTY) return Qnil;
  return INT2NUM( returnedNode );
}

VALUE grid_removeNode( VALUE self, VALUE node )
{
  GET_GRID_FROM_SELF;
  return (gridRemoveNode( grid, NUM2INT(node) )==grid?self:Qnil);
}

VALUE grid_validNode( VALUE self, VALUE node )
{
  GET_GRID_FROM_SELF;
  return ( gridValidNode( grid, NUM2INT( node ) ) ? Qtrue : Qfalse );
}

VALUE grid_translate( VALUE self, VALUE dx, VALUE dy, VALUE dz )
{
  GET_GRID_FROM_SELF;
  return (gridTranslate( grid, NUM2DBL(dx), 
			 NUM2DBL(dy), NUM2DBL(dz) )==grid?self:Qnil);
}


VALUE grid_nodeXYZ( VALUE self, VALUE node )
{
  VALUE rb_xyz;
  double xyz[3];
  Grid *returnedGrid;
  GET_GRID_FROM_SELF;
  returnedGrid = 
    gridNodeXYZ( grid, NUM2INT(node), xyz );
  if ( returnedGrid == grid ){
    rb_xyz = rb_ary_new2(3);
    rb_ary_store( rb_xyz, 0, rb_float_new(xyz[0]) );
    rb_ary_store( rb_xyz, 1, rb_float_new(xyz[1]) );
    rb_ary_store( rb_xyz, 2, rb_float_new(xyz[2]) );
  }else{
    rb_xyz = Qnil;
  }
  return rb_xyz;
}

VALUE grid_setNodeXYZ( VALUE self, VALUE node, VALUE rb_xyz )
{
  double xyz[3];
  GET_GRID_FROM_SELF;
  xyz[0] = NUM2DBL( rb_ary_entry( rb_xyz, 0) );
  xyz[1] = NUM2DBL( rb_ary_entry( rb_xyz, 1) );
  xyz[2] = NUM2DBL( rb_ary_entry( rb_xyz, 2) );
  return( grid == gridSetNodeXYZ( grid, NUM2INT(node), xyz )?self:Qnil);
}

VALUE grid_nodeGlobal( VALUE self, VALUE node )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNodeGlobal( grid, NUM2INT(node) ) );
}

VALUE grid_setNodeGlobal( VALUE self, VALUE node, VALUE global )
{
  GET_GRID_FROM_SELF;
  return( grid == gridSetNodeGlobal( grid, NUM2INT(node), NUM2INT(global) )?self:Qnil);
}

VALUE grid_global2local( VALUE self, VALUE global )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridGlobal2Local( grid, NUM2INT(global) ) );
}

VALUE grid_renumberGlobalNodes( VALUE self, VALUE rb_n2o )
{
  int i, nnode, *n2o;
  VALUE value;
  GET_GRID_FROM_SELF;
  nnode = RARRAY_LEN(rb_n2o);
  n2o = malloc(nnode*sizeof(int));
  for(i=0;i<nnode;i++) n2o[i] = NUM2INT( rb_ary_entry(rb_n2o,i) );
  value = (grid==gridRenumberGlobalNodes(grid,nnode,n2o)?self:Qnil);
  free(n2o);
  return value;
}

VALUE grid_nodePart( VALUE self, VALUE node )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNodePart( grid, NUM2INT(node) ) );
}

VALUE grid_setNodePart( VALUE self, VALUE node, VALUE part )
{
  GET_GRID_FROM_SELF;
  return( grid == gridSetNodePart( grid, NUM2INT(node), NUM2INT(part) )?self:Qnil);
}

VALUE grid_nodeLocal( VALUE self, VALUE node )
{
  GET_GRID_FROM_SELF;
  return ( gridNodeLocal( grid, NUM2INT(node) )?Qtrue:Qfalse );
}

VALUE grid_nodeGhost( VALUE self, VALUE node )
{
  GET_GRID_FROM_SELF;
  return ( gridNodeGhost( grid, NUM2INT(node) )?Qtrue:Qfalse );
}

VALUE grid_nGeomNode( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNGeomNode( grid ) );
}

VALUE grid_setNGeomNode( VALUE self, VALUE nGeomNode )
{
  GET_GRID_FROM_SELF;
  gridSetNGeomNode( grid, NUM2INT( nGeomNode ) );
  return self;
}

VALUE grid_nGeomEdge( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNGeomEdge( grid ) );
}

VALUE grid_setNGeomEdge( VALUE self, VALUE nGeomEdge )
{
  GET_GRID_FROM_SELF;
  gridSetNGeomEdge( grid, NUM2INT( nGeomEdge ) );
  return self;
}

VALUE grid_nGeomFace( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridNGeomFace( grid ) );
}

VALUE grid_setNGeomFace( VALUE self, VALUE nGeomFace )
{
  GET_GRID_FROM_SELF;
  gridSetNGeomFace( grid, NUM2INT( nGeomFace ) );
  return self;
}

VALUE grid_addGeomEdge( VALUE self, VALUE edge, VALUE n0, VALUE n1 )
{
  GET_GRID_FROM_SELF;
  return (grid==gridAddGeomEdge( grid, NUM2INT(edge),NUM2INT(n0),NUM2INT(n1) )?self:Qnil);
}

VALUE grid_geomEdgeStart( VALUE self, VALUE edge )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridGeomEdgeStart( grid, NUM2INT(edge) ) );
}

VALUE grid_geomEdgeEnd( VALUE self, VALUE edge )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridGeomEdgeEnd( grid, NUM2INT(edge) ) );
}

VALUE grid_geomEdgeSize( VALUE self, VALUE edge )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridGeomEdgeSize( grid, NUM2INT(edge) ) );
}

VALUE grid_geomEdge( VALUE self, VALUE edge )
{
  int ncurvenode, i, *curve;
  VALUE rb_curve;
  Grid *rGrid;
  GET_GRID_FROM_SELF;
  ncurvenode = gridGeomEdgeSize( grid, NUM2INT(edge) );
  if (ncurvenode < 2) return Qnil;
  curve = malloc(ncurvenode*sizeof(int));
  rGrid = gridGeomEdge( grid, NUM2INT(edge), curve );
  if ( rGrid == grid ){
    rb_curve = rb_ary_new2(ncurvenode);
    for ( i=0 ; i < ncurvenode ; i++ ) 
      rb_ary_store( rb_curve, i, INT2NUM(curve[i]) );
  }else{
    rb_curve = Qnil;
  }
  free(curve);
  return rb_curve;
}

VALUE grid_frozenEdgeEndPoint( VALUE self, VALUE edge, VALUE startNode )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridFrozenEdgeEndPoint( grid, 
					  NUM2INT(edge), 
					  NUM2INT(startNode) ) );
}

VALUE grid_geometryNode( VALUE self, VALUE node )
{
  GET_GRID_FROM_SELF;
  return ( gridGeometryNode( grid, NUM2INT( node ) ) ? Qtrue : Qfalse );
}

VALUE grid_geometryEdge( VALUE self, VALUE node )
{
  GET_GRID_FROM_SELF;
  return ( gridGeometryEdge( grid, NUM2INT( node ) ) ? Qtrue : Qfalse );
}

VALUE grid_geometryFace( VALUE self, VALUE node )
{
  GET_GRID_FROM_SELF;
  return ( gridGeometryFace( grid, NUM2INT( node ) ) ? Qtrue : Qfalse );
}

VALUE grid_parentGeometry( VALUE self, VALUE node0, VALUE node1 )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridParentGeometry( grid, NUM2INT(node0), NUM2INT(node1) ) );
}


VALUE grid_addPrism( VALUE self, VALUE n0, VALUE n1, VALUE n2, VALUE n3,
		      VALUE n4, VALUE n5 )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridAddPrism(grid,NUM2INT(n0),NUM2INT(n1),NUM2INT(n2),
				  NUM2INT(n3),NUM2INT(n4),NUM2INT(n5) )?self:Qnil );
}

VALUE grid_prism( VALUE self, VALUE prismIndex )
{
  int i;
  int nodes[6];
  VALUE rb_prism;
  GET_GRID_FROM_SELF;
  if ( grid != gridPrism(grid,NUM2INT(prismIndex),nodes) ) return Qnil;
  rb_prism = rb_ary_new2(6);
  for (i=0;i<6;i++){
    rb_ary_store( rb_prism, i, INT2NUM(nodes[i]) );
  }
  return rb_prism;
}

VALUE grid_addPyramid( VALUE self, VALUE n0, VALUE n1, VALUE n2, VALUE n3,
		      VALUE n4 )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridAddPyramid(grid,NUM2INT(n0),NUM2INT(n1),NUM2INT(n2),
				  NUM2INT(n3),NUM2INT(n4))?self:Qnil );
}

VALUE grid_pyramid( VALUE self, VALUE pyramidIndex )
{
  int i;
  int nodes[5];
  VALUE rb_pyramid;
  GET_GRID_FROM_SELF;
  if ( grid != gridPyramid(grid,NUM2INT(pyramidIndex),nodes) ) return Qnil;
  rb_pyramid = rb_ary_new2(5);
  for (i=0;i<5;i++){
    rb_ary_store( rb_pyramid, i, INT2NUM(nodes[i]) );
  }
  return rb_pyramid;
}

VALUE grid_addQuad( VALUE self, VALUE n0, VALUE n1, VALUE n2, VALUE n3, 
		    VALUE faceId )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridAddQuad(grid,NUM2INT(n0),NUM2INT(n1),NUM2INT(n2),
				  NUM2INT(n3), NUM2INT(faceId) )?self:Qnil );
}

VALUE grid_quad( VALUE self, VALUE quadIndex )
{
  int i;
  int nodes[4], faceId;
  VALUE rb_quad;
  GET_GRID_FROM_SELF;
  if ( grid != gridQuad(grid,NUM2INT(quadIndex),nodes,&faceId) ) return Qnil;
  rb_quad = rb_ary_new2(5);
  for (i=0;i<4;i++){
    rb_ary_store( rb_quad, i, INT2NUM(nodes[i]) );
  }
  rb_ary_store( rb_quad, 4, INT2NUM(faceId) );
  return rb_quad;
}

VALUE grid_setMap( VALUE self, VALUE node, 
		   VALUE m11, VALUE m12, VALUE m13,
		              VALUE m22, VALUE m23,
		                         VALUE m33)
{
  GET_GRID_FROM_SELF;
  return
    (gridSetMap(grid, NUM2INT(node), 
		NUM2DBL(m11), NUM2DBL(m12), NUM2DBL(m13), 
		              NUM2DBL(m22), NUM2DBL(m23), 
		                            NUM2DBL(m33) )==grid?self:Qnil);
}

VALUE grid_map( VALUE self, VALUE node )
{
  VALUE rb_map;
  int i;
  double map[6];
  GET_GRID_FROM_SELF;
  if ( grid != gridMap( grid, NUM2INT(node), map ) ) return Qnil;
  rb_map = rb_ary_new2(6);
  for(i=0;i<6;i++) rb_ary_store( rb_map, i, rb_float_new(map[i]) );
  return rb_map;
}

VALUE grid_interpolateMap2( VALUE self, 
			    VALUE node0, VALUE node1, VALUE ratio, 
			    VALUE target )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridInterpolateMap2(grid,
				       NUM2INT(node0),
				       NUM2INT(node1),
				       NUM2DBL(ratio),
				       NUM2INT(target) )?self:Qnil );
}

VALUE grid_costFunction( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridCostFunction(grid) );
}

VALUE grid_setCostFunction( VALUE self, VALUE costFunction )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridSetCostFunction(grid,NUM2INT(costFunction))?self:Qnil );
}

VALUE grid_costConstraint( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridCostConstraint(grid) );
}

VALUE grid_setCostConstraint( VALUE self, VALUE costConstraint )
{
  GET_GRID_FROM_SELF;
  return ( grid == gridSetCostConstraint(grid,NUM2INT(costConstraint))?self:Qnil );
}

VALUE grid_storedCostDegree( VALUE self )
{
  GET_GRID_FROM_SELF;
  return INT2NUM( gridStoredCostDegree(grid) );
}

VALUE grid_storedCost( VALUE self, VALUE node )
{
  double cost;
  GET_GRID_FROM_SELF;
  cost = gridStoredCost(grid, NUM2INT(node));
  if ( cost == DBL_MAX ) return Qnil;
  return rb_float_new(cost);
}

VALUE grid_storedCostDerivative( VALUE self, VALUE node )
{
  VALUE rb_deriv;
  double deriv[3];
  GET_GRID_FROM_SELF;
  if (grid != gridStoredCostDerivative(grid, NUM2INT(node), deriv)) return Qnil;
  rb_deriv = rb_ary_new2(3);
  rb_ary_store( rb_deriv, 0, rb_float_new(deriv[0]) );
  rb_ary_store( rb_deriv, 1, rb_float_new(deriv[1]) );
  rb_ary_store( rb_deriv, 2, rb_float_new(deriv[2]) );
  return rb_deriv;
}

VALUE grid_storeCost( VALUE self, VALUE rb_cost, VALUE rb_deriv )
{
  double cost, deriv[3];
  GET_GRID_FROM_SELF;
  cost = NUM2DBL( rb_cost );
  deriv[0] = NUM2DBL( rb_ary_entry( rb_deriv, 0) );
  deriv[1] = NUM2DBL( rb_ary_entry( rb_deriv, 1) );
  deriv[2] = NUM2DBL( rb_ary_entry( rb_deriv, 2) );
  return( grid == gridStoreCost( grid, cost, deriv )?self:Qnil);
}

VALUE grid_clearStoredCost( VALUE self )
{
  GET_GRID_FROM_SELF;
  return( grid == gridClearStoredCost( grid )?self:Qnil);
}

VALUE grid_copyAboutY0( VALUE self, VALUE rb_faceId, VALUE rb_aux )
{
  GET_GRID_FROM_SELF;
  return( grid == gridCopyAboutY0( grid, NUM2INT(rb_faceId), NUM2INT(rb_aux) )?self:Qnil);
}


VALUE cGrid;

void Init_Grid() 
{
  cGrid = rb_define_class( "Grid", rb_cObject );
  rb_define_singleton_method( cGrid, "new", grid_new, 4 );
  rb_define_singleton_method( cGrid, "from_aflr3", grid_from_aflr3, 1 );
  rb_define_singleton_method( cGrid, "from_FAST", grid_from_FAST, 1 );
  rb_define_singleton_method( cGrid, "from_mesh3d", grid_from_mesh3d, 1 );
  rb_define_singleton_method( cGrid, "from_NGP", grid_from_NGP, 1 );
  rb_define_singleton_method( cGrid, "from_front", grid_from_front, 1 );
  rb_define_method( cGrid, "initialize", grid_init, 0 );
  rb_define_method( cGrid, "pack", grid_pack, 0 );
  rb_define_method( cGrid, "sortNodeGridEx", grid_sortNodeGridEx, 0 );
  rb_define_method( cGrid, "writeTecplotSurfaceGeom", grid_writeTecplotSurfaceGeom, 1 );
  rb_define_method( cGrid, "writeTecplotVolumeGeom", grid_writeTecplotVolumeGeom, 1 );
  rb_define_method( cGrid, "writeTecplotGeomFaceXYZ", grid_writeTecplotGeomFaceXYZ, 1 );
  rb_define_method( cGrid, "writeTecplotEquator", grid_writeTecplotEquator, 2 );
  rb_define_method( cGrid, "writeTecplotSurfaceScalar", grid_writeTecplotSurfaceScalar, 1 );
  rb_define_method( cGrid, "writeVTK", grid_writeVTK, 1 );
  rb_define_method( cGrid, "exportNGP", grid_exportNGP, 1 );
  rb_define_method( cGrid, "exportFAST", grid_exportFAST, 1 );
  rb_define_method( cGrid, "exportFASTSurface", grid_exportFASTSurface, 1 );
  rb_define_method( cGrid, "exportGRI", grid_exportGRI, 1 );
  rb_define_method( cGrid, "exportOFF", grid_exportOFF, 1 );
  rb_define_method( cGrid, "exportAFLR3", grid_exportAFLR3, 1 );
  rb_define_method( cGrid, "maxnode", grid_maxnode, 0 );
  rb_define_method( cGrid, "nnode", grid_nnode, 0 );
  rb_define_method( cGrid, "maxcell", grid_maxcell, 0 );
  rb_define_method( cGrid, "ncell", grid_ncell, 0 );
  rb_define_method( cGrid, "maxface", grid_maxface, 0 );
  rb_define_method( cGrid, "nface", grid_nface, 0 );
  rb_define_method( cGrid, "maxedge", grid_maxedge, 0 );
  rb_define_method( cGrid, "nedge", grid_nedge, 0 );
  rb_define_method( cGrid, "nprism", grid_nprism, 0 );
  rb_define_method( cGrid, "npyramid", grid_npyramid, 0 );
  rb_define_method( cGrid, "nquad", grid_nquad, 0 );

  rb_define_method( cGrid, "naux", grid_naux, 0 );
  rb_define_method( cGrid, "setNAux", grid_setNAux, 1 );
  rb_define_method( cGrid, "aux", grid_aux, 2 );
  rb_define_method( cGrid, "setAux", grid_setAux, 3 );
  rb_define_method( cGrid, "interpolateAux2", grid_interpolateAux2, 4 );

  rb_define_method( cGrid, "partId", grid_partId, 0 );
  rb_define_method( cGrid, "setPartId", grid_setPartId, 1 );
  rb_define_method( cGrid, "globalnnode", grid_globalnnode, 0 );
  rb_define_method( cGrid, "setGlobalNNode", grid_setGlobalNNode, 1 );

  rb_define_method( cGrid, "nUnusedNodeGlobal", grid_nUnusedNodeGlobal, 0 );
  rb_define_method( cGrid, "getUnusedNodeGlobal", grid_getUnusedNodeGlobal, 0 );
  rb_define_method( cGrid, "joinUnusedNodeGlobal", grid_joinUnusedNodeGlobal, 1 );
  rb_define_method( cGrid, "eliminateUnusedNodeGlobal", grid_eliminateUnusedNodeGlobal, 0 );

  rb_define_method( cGrid, "addCell", grid_addCell, 4 );
  rb_define_method( cGrid, "removeCell", grid_removeCell, 1 );
  rb_define_method( cGrid, "reconnectAllCell", grid_reconnectAllCell, 2 );
  rb_define_method( cGrid, "cell", grid_cell, 1 );
  rb_define_method( cGrid, "cellDegree", grid_cellDegree, 1 );
  rb_define_method( cGrid, "cellEdge", grid_cellEdge, 2 );
  rb_define_method( cGrid, "cellFace", grid_cellFace, 3 );
  rb_define_method( cGrid, "findOtherCellWith3Nodes", 
		    grid_findOtherCellWith3Nodes, 4 );
  rb_define_method( cGrid, "findCellWithFace", grid_findCellWithFace, 1 );
  rb_define_method( cGrid, "findCell", grid_findCell, 1 );
  rb_define_method( cGrid, "findEnclosingCell", 
		    grid_findEnclosingCell, 2 );

  rb_define_method( cGrid, "nconn", grid_nconn, 0 );
  rb_define_method( cGrid, "cell2Conn", grid_cell2Conn, 2 );
  rb_define_method( cGrid, "conn2Node", grid_conn2Node, 1 );
  rb_define_method( cGrid, "findConn", grid_findConn, 2 );
  rb_define_method( cGrid, "createConn", grid_createConn, 0 );
  rb_define_method( cGrid, "eraseConn", grid_eraseConn, 0 );

  rb_define_method( cGrid, "addFace", grid_addFace, 4 );
  rb_define_method( cGrid, "addFaceUV", grid_addFaceUV, 10 );
  rb_define_method( cGrid, "removeFace", grid_removeFace, 1 );
  rb_define_method( cGrid, "findFace", grid_findFace, 3 );
  rb_define_method( cGrid, "faceId", grid_faceId, 3 );
  rb_define_method( cGrid, "findFaceWithNodesUnless", 
		       grid_findFaceWithNodesUnless, 3 );
  rb_define_method( cGrid, "reconnectAllFace", grid_reconnectAllFace, 2 );
  rb_define_method( cGrid, "reconnectionOfAllFacesOK", 
		    grid_reconnectionOfAllFacesOK, 2 );
  rb_define_method( cGrid, "face", grid_face, 1 );
  rb_define_method( cGrid, "deleteThawedFaces", grid_deleteThawedFaces, 1 );
  rb_define_method( cGrid, "nThawedFaces", grid_nThawedFaces, 1 );

  rb_define_method( cGrid, "nodeUV", grid_nodeUV, 2 );
  rb_define_method( cGrid, "setNodeUV", grid_setNodeUV, 4 );
  rb_define_method( cGrid, "nodeFaceIdDegree", grid_nodeFaceIdDegree, 1 );
  rb_define_method( cGrid, "nodeFaceId", grid_nodeFaceId, 1 );
  rb_define_method( cGrid, "trianglesOnFaceId", grid_trianglesOnFaceId, 1 );

  rb_define_method( cGrid, "nodeT", grid_nodeT, 2 );
  rb_define_method( cGrid, "setNodeT", grid_setNodeT, 3 );
  rb_define_method( cGrid, "nodeEdgeIdDegree", grid_nodeEdgeIdDegree, 1 );
  rb_define_method( cGrid, "nodeEdgeId", grid_nodeEdgeId, 1 );

  rb_define_method( cGrid, "addEdge", grid_addEdge, 5 );
  rb_define_method( cGrid, "removeEdge", grid_removeEdge, 1 );
  rb_define_method( cGrid, "findEdge", grid_findEdge, 2 );
  rb_define_method( cGrid, "edgeId", grid_edgeId, 2 );
  rb_define_method( cGrid, "edge", grid_edge, 1 );
  rb_define_method( cGrid, "deleteThawedEdgeSegments", 
		    grid_deleteThawedEdgeSegments, 1 );
  rb_define_method( cGrid, "nThawedEdgeSegments", grid_nThawedEdgeSegments, 1 );
  rb_define_method( cGrid, "geomCurveSize", grid_geomCurveSize, 2 );
  rb_define_method( cGrid, "geomCurve", grid_geomCurve, 2 );
  rb_define_method( cGrid, "geomCurveT", grid_geomCurveT, 2 );

  rb_define_method( cGrid, "nfrozen", grid_nfrozen, 0 );
  rb_define_method( cGrid, "nodeFrozen", grid_nodeFrozen, 1 );
  rb_define_method( cGrid, "freezeNode", grid_freezeNode, 1 );
  rb_define_method( cGrid, "thawNode", grid_thawNode, 1 );
  rb_define_method( cGrid, "freezeAll", grid_freezeAll, 0 );
  rb_define_method( cGrid, "thawAll", grid_thawAll, 0 );

  rb_define_method( cGrid, "gem", grid_gem, 2 );
  rb_define_method( cGrid, "gemIsAllLocal", grid_gemIsAllLocal, 0 );
  rb_define_method( cGrid, "equator", grid_equator, 2 );
  rb_define_method( cGrid, "nequ", grid_nequ, 0 );

  rb_define_method( cGrid, "faceOppositeCellNode",
		    grid_faceOppositeCellNode, 2 );
  rb_define_method( cGrid, "orient", grid_orient, 6 );

  rb_define_method( cGrid, "addNode", grid_addNode, 3 );
  rb_define_method( cGrid, "addNodeWithGlobal", grid_addNodeWithGlobal, 4 );
  rb_define_method( cGrid, "removeNode", grid_removeNode, 1 );
  rb_define_method( cGrid, "validNode", grid_validNode, 1 );
  rb_define_method( cGrid, "translate", grid_translate, 3 );
  rb_define_method( cGrid, "nodeXYZ", grid_nodeXYZ, 1 );
  rb_define_method( cGrid, "setNodeXYZ", grid_setNodeXYZ, 2 );

  rb_define_method( cGrid, "nodeGlobal", grid_nodeGlobal, 1 );
  rb_define_method( cGrid, "global2local", grid_global2local, 1 );
  rb_define_method( cGrid, "setNodeGlobal", grid_setNodeGlobal, 2 );
  rb_define_method( cGrid, "renumberGlobalNodes", grid_renumberGlobalNodes, 1 );
  rb_define_method( cGrid, "nodePart", grid_nodePart, 1 );
  rb_define_method( cGrid, "setNodePart", grid_setNodePart, 2 );
  rb_define_method( cGrid, "nodeLocal", grid_nodeLocal, 1 );
  rb_define_method( cGrid, "nodeGhost", grid_nodeGhost, 1 );

  rb_define_method( cGrid, "nGeomNode", grid_nGeomNode, 0 );
  rb_define_method( cGrid, "setNGeomNode", grid_setNGeomNode, 1 );
  rb_define_method( cGrid, "nGeomEdge", grid_nGeomEdge, 0 );
  rb_define_method( cGrid, "setNGeomEdge", grid_setNGeomEdge, 1 );
  rb_define_method( cGrid, "nGeomFace", grid_nGeomFace, 0 );
  rb_define_method( cGrid, "setNGeomFace", grid_setNGeomFace, 1 );

  rb_define_method( cGrid, "addGeomEdge", grid_addGeomEdge, 3 );
  rb_define_method( cGrid, "geomEdgeStart", grid_geomEdgeStart, 1 );
  rb_define_method( cGrid, "geomEdgeEnd", grid_geomEdgeEnd, 1 );
  rb_define_method( cGrid, "geomEdgeSize", grid_geomEdgeSize, 1 );
  rb_define_method( cGrid, "geomEdge", grid_geomEdge, 1 );
  rb_define_method( cGrid, "frozenEdgeEndPoint", grid_frozenEdgeEndPoint, 2 );

  rb_define_method( cGrid, "geometryNode", grid_geometryNode, 1 );
  rb_define_method( cGrid, "geometryEdge", grid_geometryEdge, 1 );
  rb_define_method( cGrid, "geometryFace", grid_geometryFace, 1 );
  rb_define_method( cGrid, "parentGeometry", grid_parentGeometry, 2 );

  rb_define_method( cGrid, "addPrism", grid_addPrism, 6 );
  rb_define_method( cGrid, "prism", grid_prism, 1 );

  rb_define_method( cGrid, "addPyramid", grid_addPyramid, 5 );
  rb_define_method( cGrid, "pyramid", grid_pyramid, 1 );

  rb_define_method( cGrid, "addQuad", grid_addQuad, 5 );
  rb_define_method( cGrid, "quad", grid_quad, 1 );

  rb_define_method( cGrid, "setMap", grid_setMap, 7 );
  rb_define_method( cGrid, "map", grid_map, 1 );
  rb_define_method( cGrid, "interpolateMap2", grid_interpolateMap2, 4 );

  rb_define_method( cGrid, "costFunction", grid_costFunction, 0 );
  rb_define_method( cGrid, "setCostFunction", grid_setCostFunction, 1 );
  rb_define_method( cGrid, "costConstraint", grid_costConstraint, 0 );
  rb_define_method( cGrid, "setCostConstraint", grid_setCostConstraint, 1 );

  rb_define_method( cGrid, "storedCostDegree", grid_storedCostDegree, 0 );
  rb_define_method( cGrid, "storedCost", grid_storedCost, 1 );
  rb_define_method( cGrid, "storedCostDerivative", grid_storedCostDerivative, 1 );
  rb_define_method( cGrid, "storeCost", grid_storeCost, 2 );
  rb_define_method( cGrid, "clearStoredCost", grid_clearStoredCost, 0 );
  rb_define_method( cGrid, "copyAboutY0", grid_copyAboutY0, 2 );

}
