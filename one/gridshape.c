
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gridmath.h"
#include "gridcad.h"
#include "gridshape.h"

Grid *gridCurvedEdgeMidpoint(Grid *grid,int node0, int node1, double *e)
{
  double n0[3], n1[3];
  double t0, t1, t;
  double uv0[2], uv1[2], uv[2];
  int parent;

  e[0]=e[1]=e[2]=DBL_MAX;

  if (grid != gridNodeXYZ(grid,node0,n0)) return NULL;
  if (grid != gridNodeXYZ(grid,node1,n1)) return NULL;
  gridAverageVector(n0,n1,e);

  parent = gridParentGeometry(grid,node0,node1);
  if (0==parent) return grid;

  if (parent<0) {
    gridNodeT(grid,node0,-parent, &t0);
    gridNodeT(grid,node1,-parent, &t1);
    t = 0.5 * ( t0 + t1 );
    gridEvaluateOnEdge(grid, -parent, t, e);
  } else {
    gridNodeUV(grid,node0,parent, uv0);
    gridNodeUV(grid,node1,parent, uv1);
    uv[0] = 0.5 * ( uv0[0] + uv1[0] );
    uv[1] = 0.5 * ( uv0[1] + uv1[1] );
    gridEvaluateOnFace(grid, parent, uv, e);    
  }

  return grid;
}

double gridMinCellJacDet2(Grid *grid, int *nodes)
{
  double n0[3], n1[3], n2[3], n3[3];
  double e01[3], e02[3], e03[3];
  double e12[3], e13[3], e23[3];
  double where[3];
  double det;

  if ( !gridValidNode(grid, nodes[0]) || 
       !gridValidNode(grid, nodes[1]) ||
       !gridValidNode(grid, nodes[2]) ||
       !gridValidNode(grid, nodes[3]) ) return -999.0;

  gridNodeXYZ(grid,nodes[0],n0);
  gridNodeXYZ(grid,nodes[1],n1);
  gridNodeXYZ(grid,nodes[2],n2);
  gridNodeXYZ(grid,nodes[3],n3);

  gridCurvedEdgeMidpoint(grid,nodes[0],nodes[1],e01);
  gridCurvedEdgeMidpoint(grid,nodes[0],nodes[2],e02);
  gridCurvedEdgeMidpoint(grid,nodes[0],nodes[3],e03);

  gridCurvedEdgeMidpoint(grid,nodes[1],nodes[2],e12);
  gridCurvedEdgeMidpoint(grid,nodes[1],nodes[3],e13);
  gridCurvedEdgeMidpoint(grid,nodes[2],nodes[3],e23);

  where[0]=0.0; where[1]=0.0; where[2]=0.0; 
  det = gridShapeJacobianDet2(grid,n0,n1,n2,n3, 
			      e01, e02, e03, 
			      e12, e13, e23, 
			      where);

  where[0]=1.0; where[1]=0.0; where[2]=0.0; 
  det = MIN( det, gridShapeJacobianDet2(grid,n0,n1,n2,n3, 
					e01, e02, e03, 
					e12, e13, e23, 
					where));

  where[0]=0.0; where[1]=1.0; where[2]=0.0; 
  det = MIN( det, gridShapeJacobianDet2(grid,n0,n1,n2,n3, 
					e01, e02, e03, 
					e12, e13, e23, 
					where));
     
  where[0]=0.0; where[1]=0.0; where[2]=1.0; 
  det = MIN( det, gridShapeJacobianDet2(grid,n0,n1,n2,n3, 
					e01, e02, e03, 
					e12, e13, e23, 
					where));

  return det;
}

Grid *gridMinCellJacDetDeriv2(Grid *grid, int *nodes,
			      double *determinate, double *dDetdx)
{
  double n0[3], n1[3], n2[3], n3[3];
  double e01[3], e02[3], e03[3];
  double e12[3], e13[3], e23[3];
  double where[3];
  double det, ddet[3];
  
  if ( !gridValidNode(grid, nodes[0]) || 
       !gridValidNode(grid, nodes[1]) ||
       !gridValidNode(grid, nodes[2]) ||
       !gridValidNode(grid, nodes[3]) ) return NULL;

  gridNodeXYZ(grid,nodes[0],n0);
  gridNodeXYZ(grid,nodes[1],n1);
  gridNodeXYZ(grid,nodes[2],n2);
  gridNodeXYZ(grid,nodes[3],n3);

  gridCurvedEdgeMidpoint(grid,nodes[0],nodes[1],e01);
  gridCurvedEdgeMidpoint(grid,nodes[0],nodes[2],e02);
  gridCurvedEdgeMidpoint(grid,nodes[0],nodes[3],e03);

  gridCurvedEdgeMidpoint(grid,nodes[1],nodes[2],e12);
  gridCurvedEdgeMidpoint(grid,nodes[1],nodes[3],e13);
  gridCurvedEdgeMidpoint(grid,nodes[2],nodes[3],e23);

  where[0]=0.0; where[1]=0.0; where[2]=0.0; 
  gridShapeJacobianDetDeriv2(grid,n0,n1,n2,n3, 
			     e01, e02, e03, 
			     e12, e13, e23, 
			     where, determinate, dDetdx);

  where[0]=1.0; where[1]=0.0; where[2]=0.0; 
  gridShapeJacobianDetDeriv2(grid,n0,n1,n2,n3, 
			     e01, e02, e03, 
			     e12, e13, e23, 
			     where, &det, ddet);
  if (det<(*determinate)) {
    *determinate = det;
    dDetdx[0] = ddet[0];
    dDetdx[1] = ddet[1];
    dDetdx[2] = ddet[2];
  }

  where[0]=0.0; where[1]=1.0; where[2]=0.0; 
  gridShapeJacobianDetDeriv2(grid,n0,n1,n2,n3, 
			     e01, e02, e03, 
			     e12, e13, e23, 
			     where, &det, ddet);
  if (det<(*determinate)) {
    *determinate = det;
    dDetdx[0] = ddet[0];
    dDetdx[1] = ddet[1];
    dDetdx[2] = ddet[2];
  }
     
  where[0]=0.0; where[1]=0.0; where[2]=1.0; 
  gridShapeJacobianDetDeriv2(grid,n0,n1,n2,n3, 
			     e01, e02, e03, 
			     e12, e13, e23, 
			     where, &det, ddet);
  if (det<(*determinate)) {
    *determinate = det;
    dDetdx[0] = ddet[0];
    dDetdx[1] = ddet[1];
    dDetdx[2] = ddet[2];
  }

  return grid;
}

Grid *gridNodeMinCellJacDet2(Grid *grid, int node, double *determinate )
{
  AdjIterator it;
  int cell, nodes[4];
  double local_determinate;

  *determinate = DBL_MAX;

  for ( it = adjFirst(gridCellAdj(grid),node); adjValid(it); it = adjNext(it) ){
    cell = adjItem(it);
    gridCell( grid, cell, nodes);
    local_determinate = gridMinCellJacDet2(grid, nodes);
    if ( local_determinate < *determinate ) *determinate = local_determinate;
  }

  return grid;
}

Grid *gridPlotMinDeterminateAtSurface(Grid *grid)
{
  int node, cell, nodes[4];
  double n0[3], n1[3], n2[3], n3[3];
  double e01[3], e02[3], e03[3];
  double e12[3], e13[3], e23[3];
  double where[3];
  double jacobian[9];
  double *scalar;
  if (grid !=gridSortNodeGridEx(grid)) return NULL;

  scalar = (double *)malloc(gridNNode(grid)*sizeof(double));
  for (node=0;node<gridNNode(grid);node++) {
    scalar[node] = DBL_MAX;
  }

  for (cell=0;cell<gridMaxNode(grid);cell++){
    if (grid==gridCell(grid,cell,nodes)){
      gridNodeXYZ(grid,nodes[0],n0);
      gridNodeXYZ(grid,nodes[1],n1);
      gridNodeXYZ(grid,nodes[2],n2);
      gridNodeXYZ(grid,nodes[3],n3);

      gridCurvedEdgeMidpoint(grid,nodes[0],nodes[1],e01);
      gridCurvedEdgeMidpoint(grid,nodes[0],nodes[2],e02);
      gridCurvedEdgeMidpoint(grid,nodes[0],nodes[3],e03);

      gridCurvedEdgeMidpoint(grid,nodes[1],nodes[2],e12);
      gridCurvedEdgeMidpoint(grid,nodes[1],nodes[3],e13);
      gridCurvedEdgeMidpoint(grid,nodes[2],nodes[3],e23);

      /* 1-st order test gridShapeJacobian1(grid,n0,n1,n2,n3,where,jacobian);*/

      where[0]=0.0; where[1]=0.0; where[2]=0.0; 
      gridShapeJacobian2(grid,n0,n1,n2,n3, 
			 e01, e02, e03, 
			 e12, e13, e23, 
			 where,jacobian);
      scalar[nodes[0]]=MIN(scalar[nodes[0]],gridMatrixDeterminate(jacobian));

    
      where[0]=1.0; where[1]=0.0; where[2]=0.0; 
      gridShapeJacobian2(grid,n0,n1,n2,n3, 
			 e01, e02, e03, 
			 e12, e13, e23, 
			 where,jacobian);
      scalar[nodes[1]]=MIN(scalar[nodes[1]],gridMatrixDeterminate(jacobian));

      where[0]=0.0; where[1]=1.0; where[2]=0.0; 
      gridShapeJacobian2(grid,n0,n1,n2,n3, 
			 e01, e02, e03, 
			 e12, e13, e23, 
			 where,jacobian);
      scalar[nodes[2]]=MIN(scalar[nodes[2]],gridMatrixDeterminate(jacobian));
     
      where[0]=0.0; where[1]=0.0; where[2]=1.0; 
      gridShapeJacobian2(grid,n0,n1,n2,n3, 
			 e01, e02, e03, 
			 e12, e13, e23, 
			 where,jacobian);
      scalar[nodes[3]]=MIN(scalar[nodes[3]],gridMatrixDeterminate(jacobian));
     

    }
  }

  gridWriteTecplotSurfaceScalar(grid,"gridMinJac.t",scalar);
  free(scalar);
  return grid;
}

static int side2node0[] = {1, 0, 0};
static int side2node1[] = {2, 2, 1};

static void gridInsertSideIntoFace2Edge(Grid *grid, int *f2n6, int side,
					int node0, int node1)
{
  int face, nodes[3], faceId;
  int iside;
  int local0, local1;
  AdjIterator it;

  for ( it = adjFirst(gridFaceAdj(grid),node0); 
	adjValid(it); 
	it = adjNext(it) ) {
    face = adjItem(it);
    gridFace( grid, face, nodes, &faceId );
    for(iside=0;iside<3;iside++) {
      local0 = nodes[side2node0[iside]];
      local1 = nodes[side2node1[iside]];
      if ( MIN(local0, local1) == MIN(node0,node1) &&
	   MAX(local0, local1) == MAX(node0,node1) ) {
	f2n6[3+iside+6*face] = side;
      } 
    }
  }
}

/* call gridSortNodeGridEx before calling */
static Grid *gridComputeMidNodeIndex(Grid *grid, int *f2n6,
				     int *norignode, int *nmidnode)
{
  int i, face, nodes[3], faceId;
  int nface, nfacenode;
  int side, nside;

  for (face=0;face<gridNFace(grid);face++) {
    for (i=0;i<6;i++) f2n6[i+6*face] = EMPTY;
  }

  nface=0;
  nfacenode=0;
  for( face=0 ; face<gridMaxFace(grid) ; face++ ) {
    if ( grid == gridFace(grid, face, nodes, &faceId) ) {
      for (i=0;i<3;i++) {
	f2n6[i+6*nface] = nodes[i];
	nfacenode = MAX(nfacenode, nodes[i]);
      }
      nface++;
    }
  }
  nfacenode++;

  nside = nfacenode;
  for (face=0;face<nface;face++) {
    for (side=0;side<3;side++) {
      if ( EMPTY == f2n6[3+side+6*face] ) {
	gridInsertSideIntoFace2Edge(grid, f2n6, nside,
				    f2n6[side2node0[side]+6*face], 
				    f2n6[side2node1[side]+6*face]);
	nside++;
      }
    }
  }

  *norignode = nfacenode;
  *nmidnode = nside;

  return grid;
}

Grid *gridWriteTecplotCurvedGeom(Grid *grid, char *filename )
{
  int node;
  int nfacenode, nnode, nface;
  int face, side;
  int *f2n6;
  double *xyz;
  int *f2n;
  Grid *status;

  if ( grid !=  gridSortNodeGridEx(grid) ) {
    printf("gridWriteTecplotCurvedGeom: gridSortNodeGridEx failed.\n");
    return NULL;
  }

  f2n6 = (int *)malloc(sizeof(int)*6*gridNFace(grid));

  gridComputeMidNodeIndex(grid, f2n6, &nfacenode, &nnode);

  xyz = (double *)malloc(sizeof(double)*3*nnode);

  for(node=0;node<nfacenode;node++) gridNodeXYZ(grid,node,&(xyz[3*node]));

  for(face=0;face<gridNFace(grid);face++) {
    for (side=0;side<3;side++) {
      gridCurvedEdgeMidpoint(grid,
			     f2n6[side2node0[side]+6*face], 
			     f2n6[side2node1[side]+6*face],
			     &(xyz[3*f2n6[3+side+6*face]]) );
    }
  }

  nface = 4*gridNFace(grid);
  f2n = (int *)malloc(sizeof(int)*3*nface);

  for(face=0;face<gridNFace(grid);face++) {
    f2n[0+3*(0+4*face)] = f2n6[0+6*face];
    f2n[1+3*(0+4*face)] = f2n6[5+6*face];
    f2n[2+3*(0+4*face)] = f2n6[4+6*face];

    f2n[0+3*(1+4*face)] = f2n6[5+6*face];
    f2n[1+3*(1+4*face)] = f2n6[1+6*face];
    f2n[2+3*(1+4*face)] = f2n6[3+6*face];

    f2n[0+3*(2+4*face)] = f2n6[5+6*face];
    f2n[1+3*(2+4*face)] = f2n6[3+6*face];
    f2n[2+3*(2+4*face)] = f2n6[4+6*face];

    f2n[0+3*(3+4*face)] = f2n6[4+6*face];
    f2n[1+3*(3+4*face)] = f2n6[3+6*face];
    f2n[2+3*(3+4*face)] = f2n6[2+6*face];
  }

  status = gridWriteTecplotTriangleZone(grid, filename, "curved",
					nnode, xyz,
					nface, f2n);
  free(f2n6);
  free(xyz);
  free(f2n);

  return status;
}

Grid *gridWriteTecplotCellJacDet(Grid *grid, int cell, char *filename )
{
  int nodes[4];
  double n0[3], n1[3], n2[3], n3[3];
  double e01[3], e02[3], e03[3];
  double e12[3], e13[3], e23[3];
  double where[3];
  double jacobian[9];

  double dets[4];

  if (grid!=gridCell(grid,cell,nodes)) return NULL;
  gridNodeXYZ(grid,nodes[0],n0);
  gridNodeXYZ(grid,nodes[1],n1);
  gridNodeXYZ(grid,nodes[2],n2);
  gridNodeXYZ(grid,nodes[3],n3);

  gridCurvedEdgeMidpoint(grid,nodes[0],nodes[1],e01);
  gridCurvedEdgeMidpoint(grid,nodes[0],nodes[2],e02);
  gridCurvedEdgeMidpoint(grid,nodes[0],nodes[3],e03);

  gridCurvedEdgeMidpoint(grid,nodes[1],nodes[2],e12);
  gridCurvedEdgeMidpoint(grid,nodes[1],nodes[3],e13);
  gridCurvedEdgeMidpoint(grid,nodes[2],nodes[3],e23);
  

  where[0]=0.0; where[1]=0.0; where[2]=0.0; 
  gridShapeJacobian2(grid,n0,n1,n2,n3, 
		     e01, e02, e03, 
		     e12, e13, e23, 
		     where,jacobian);
  dets[0] = gridMatrixDeterminate(jacobian);
    
  where[0]=1.0; where[1]=0.0; where[2]=0.0; 
  gridShapeJacobian2(grid,n0,n1,n2,n3, 
		     e01, e02, e03, 
		     e12, e13, e23, 
		     where,jacobian);
  dets[1] = gridMatrixDeterminate(jacobian);

  where[0]=0.0; where[1]=1.0; where[2]=0.0; 
  gridShapeJacobian2(grid,n0,n1,n2,n3, 
		     e01, e02, e03, 
		     e12, e13, e23, 
		     where,jacobian);
  dets[2] = gridMatrixDeterminate(jacobian);

  where[0]=0.0; where[1]=0.0; where[2]=1.0; 
  gridShapeJacobian2(grid,n0,n1,n2,n3, 
		     e01, e02, e03, 
		     e12, e13, e23, 
		     where,jacobian);
  dets[3] = gridMatrixDeterminate(jacobian);

  return gridWriteTecplotCellGeom(grid, nodes, dets, filename );
}

Grid* gridShapeJacobian1(Grid *grid, 
			 double *n0, double *n1,double *n2, double *n3, 
			 double *j )
{
  double gphi[3];
  j[0] = 0.0;
  j[1] = 0.0;
  j[2] = 0.0;
  j[3] = 0.0;
  j[4] = 0.0;
  j[5] = 0.0;
  j[6] = 0.0;
  j[7] = 0.0;
  j[8] = 0.0;

  gphi[0] = -1.0;
  gphi[1] = -1.0;
  gphi[2] = -1.0;
  j[0] += n0[0] * gphi[0];
  j[1] += n0[0] * gphi[1];
  j[2] += n0[0] * gphi[2];
  j[3] += n0[1] * gphi[0];
  j[4] += n0[1] * gphi[1];
  j[5] += n0[1] * gphi[2];
  j[6] += n0[2] * gphi[0];
  j[7] += n0[2] * gphi[1];
  j[8] += n0[2] * gphi[2];

  gphi[0] = 1.0;
  gphi[1] = 0.0;
  gphi[2] = 0.0;
  j[0] += n1[0] * gphi[0];
  j[1] += n1[0] * gphi[1];
  j[2] += n1[0] * gphi[2];
  j[3] += n1[1] * gphi[0];
  j[4] += n1[1] * gphi[1];
  j[5] += n1[1] * gphi[2];
  j[6] += n1[2] * gphi[0];
  j[7] += n1[2] * gphi[1];
  j[8] += n1[2] * gphi[2];

  gphi[0] = 0.0;
  gphi[1] = 1.0;
  gphi[2] = 0.0;
  j[0] += n2[0] * gphi[0];
  j[1] += n2[0] * gphi[1];
  j[2] += n2[0] * gphi[2];
  j[3] += n2[1] * gphi[0];
  j[4] += n2[1] * gphi[1];
  j[5] += n2[1] * gphi[2];
  j[6] += n2[2] * gphi[0];
  j[7] += n2[2] * gphi[1];
  j[8] += n2[2] * gphi[2];

  gphi[0] = 0.0;
  gphi[1] = 0.0;
  gphi[2] = 1.0;
  j[0] += n3[0] * gphi[0];
  j[1] += n3[0] * gphi[1];
  j[2] += n3[0] * gphi[2];
  j[3] += n3[1] * gphi[0];
  j[4] += n3[1] * gphi[1];
  j[5] += n3[1] * gphi[2];
  j[6] += n3[2] * gphi[0];
  j[7] += n3[2] * gphi[1];
  j[8] += n3[2] * gphi[2];

  return grid;
}

Grid* gridShapeJacobian2(Grid *grid, 
			 double *n0, double *n1, double *n2, double *n3,
			 double *e01, double *e02, double *e03,
			 double *e12, double *e13, double *e23,
			 double *w, double *j )
{
  double x, y, z;
  double gphi[3];
  j[0] = 0.0;
  j[1] = 0.0;
  j[2] = 0.0;
  j[3] = 0.0;
  j[4] = 0.0;
  j[5] = 0.0;
  j[6] = 0.0;
  j[7] = 0.0;
  j[8] = 0.0;

  x = w[0];
  y = w[1];
  z = w[2];

  gphi[0] = -3.0+4.0*x+4.0*y+4.0*z;
  gphi[1] = -3.0+4.0*x+4.0*y+4.0*z;
  gphi[2] = -3.0+4.0*x+4.0*y+4.0*z;
  j[0] += n0[0] * gphi[0];
  j[1] += n0[0] * gphi[1];
  j[2] += n0[0] * gphi[2];
  j[3] += n0[1] * gphi[0];
  j[4] += n0[1] * gphi[1];
  j[5] += n0[1] * gphi[2];
  j[6] += n0[2] * gphi[0];
  j[7] += n0[2] * gphi[1];
  j[8] += n0[2] * gphi[2];

  gphi[0] = -1.0+4.0*x;
  gphi[1] = 0.0;
  gphi[2] = 0.0;
  j[0] += n1[0] * gphi[0];
  j[1] += n1[0] * gphi[1];
  j[2] += n1[0] * gphi[2];
  j[3] += n1[1] * gphi[0];
  j[4] += n1[1] * gphi[1];
  j[5] += n1[1] * gphi[2];
  j[6] += n1[2] * gphi[0];
  j[7] += n1[2] * gphi[1];
  j[8] += n1[2] * gphi[2];

  gphi[0] = 0.0;
  gphi[1] = -1.0+4.0*y;
  gphi[2] = 0.0;
  j[0] += n2[0] * gphi[0];
  j[1] += n2[0] * gphi[1];
  j[2] += n2[0] * gphi[2];
  j[3] += n2[1] * gphi[0];
  j[4] += n2[1] * gphi[1];
  j[5] += n2[1] * gphi[2];
  j[6] += n2[2] * gphi[0];
  j[7] += n2[2] * gphi[1];
  j[8] += n2[2] * gphi[2];

  gphi[0] = 0.0;
  gphi[1] = 0.0;
  gphi[2] = -1.0+4.0*z;
  j[0] += n3[0] * gphi[0];
  j[1] += n3[0] * gphi[1];
  j[2] += n3[0] * gphi[2];
  j[3] += n3[1] * gphi[0];
  j[4] += n3[1] * gphi[1];
  j[5] += n3[1] * gphi[2];
  j[6] += n3[2] * gphi[0];
  j[7] += n3[2] * gphi[1];
  j[8] += n3[2] * gphi[2];

  gphi[0] = 4.0-8.0*x-4.0*y-4.0*z;
  gphi[1] = -4.0*x;
  gphi[2] = -4.0*x;
  j[0] += e01[0] * gphi[0];
  j[1] += e01[0] * gphi[1];
  j[2] += e01[0] * gphi[2];
  j[3] += e01[1] * gphi[0];
  j[4] += e01[1] * gphi[1];
  j[5] += e01[1] * gphi[2];
  j[6] += e01[2] * gphi[0];
  j[7] += e01[2] * gphi[1];
  j[8] += e01[2] * gphi[2];

  gphi[0] = -4.0*y;
  gphi[1] = 4.0-4.0*x-8.0*y-4.0*z;
  gphi[2] = -4.0*y;
  j[0] += e02[0] * gphi[0];
  j[1] += e02[0] * gphi[1];
  j[2] += e02[0] * gphi[2];
  j[3] += e02[1] * gphi[0];
  j[4] += e02[1] * gphi[1];
  j[5] += e02[1] * gphi[2];
  j[6] += e02[2] * gphi[0];
  j[7] += e02[2] * gphi[1];
  j[8] += e02[2] * gphi[2];

  gphi[0] = -4.0*z;
  gphi[1] = -4.0*z;
  gphi[2] = 4.0-4.0*x-4.0*y-8.0*z;
  j[0] += e03[0] * gphi[0];
  j[1] += e03[0] * gphi[1];
  j[2] += e03[0] * gphi[2];
  j[3] += e03[1] * gphi[0];
  j[4] += e03[1] * gphi[1];
  j[5] += e03[1] * gphi[2];
  j[6] += e03[2] * gphi[0];
  j[7] += e03[2] * gphi[1];
  j[8] += e03[2] * gphi[2];

  gphi[0] = 4.0*y;
  gphi[1] = 4.0*x;
  gphi[2] = 0.0;
  j[0] += e12[0] * gphi[0];
  j[1] += e12[0] * gphi[1];
  j[2] += e12[0] * gphi[2];
  j[3] += e12[1] * gphi[0];
  j[4] += e12[1] * gphi[1];
  j[5] += e12[1] * gphi[2];
  j[6] += e12[2] * gphi[0];
  j[7] += e12[2] * gphi[1];
  j[8] += e12[2] * gphi[2];

  gphi[0] = 4.0*z;
  gphi[1] = 0.0;
  gphi[2] = 4.0*x;
  j[0] += e13[0] * gphi[0];
  j[1] += e13[0] * gphi[1];
  j[2] += e13[0] * gphi[2];
  j[3] += e13[1] * gphi[0];
  j[4] += e13[1] * gphi[1];
  j[5] += e13[1] * gphi[2];
  j[6] += e13[2] * gphi[0];
  j[7] += e13[2] * gphi[1];
  j[8] += e13[2] * gphi[2];

  gphi[0] = 0.0;
  gphi[1] = 4.0*z;
  gphi[2] = 4.0*y;
  j[0] += e23[0] * gphi[0];
  j[1] += e23[0] * gphi[1];
  j[2] += e23[0] * gphi[2];
  j[3] += e23[1] * gphi[0];
  j[4] += e23[1] * gphi[1];
  j[5] += e23[1] * gphi[2];
  j[6] += e23[2] * gphi[0];
  j[7] += e23[2] * gphi[1];
  j[8] += e23[2] * gphi[2];

  return grid;
}

Grid *gridShapeJacobianDetDeriv2(Grid *grid,
				 double *n0, double *n1, double *n2, double *n3,
				 double *e01, double *e02, double *e03,
				 double *e12, double *e13, double *e23,
				 double *where,
				 double *determinate, double *dDetdx)
{
  double x, y, z;
  double gphi[3];
  double j[9];

  dDetdx[0] = dDetdx[1] = dDetdx[2] = 0.0;

  gridShapeJacobian2(grid, 
		     n0, n1, n2, n3,
		     e01, e02, e03,
		     e12, e13, e23,
		     where, j );

  *determinate = 
    j[0]*j[4]*j[8] +
    j[1]*j[5]*j[6] + 
    j[2]*j[3]*j[7] - 
    j[0]*j[5]*j[7] - 
    j[1]*j[3]*j[8] - 
    j[2]*j[4]*j[6]; 

  x = where[0];
  y = where[1];
  z = where[2];

  gphi[0] = -3.0+4.0*x+4.0*y+4.0*z;
  gphi[1] = -3.0+4.0*x+4.0*y+4.0*z;
  gphi[2] = -3.0+4.0*x+4.0*y+4.0*z;

  dDetdx[0] += 
    gphi[0]*j[4]*j[8] +
    gphi[1]*j[5]*j[6] + 
    gphi[2]*j[3]*j[7] - 
    gphi[0]*j[5]*j[7] - 
    gphi[1]*j[3]*j[8] - 
    gphi[2]*j[4]*j[6]; 

  dDetdx[1] += 
    j[0]*gphi[1]*j[8] +
    j[1]*gphi[2]*j[6] + 
    j[2]*gphi[0]*j[7] - 
    j[0]*gphi[2]*j[7] - 
    j[1]*gphi[0]*j[8] - 
    j[2]*gphi[1]*j[6];

  dDetdx[2] += 
    j[0]*j[4]*gphi[2] +
    j[1]*j[5]*gphi[0] + 
    j[2]*j[3]*gphi[1] - 
    j[0]*j[5]*gphi[1] - 
    j[1]*j[3]*gphi[2] - 
    j[2]*j[4]*gphi[0]; 

  return grid;
}

double gridShapeJacobianDet2(Grid *grid,
			     double *n0, double *n1, double *n2, double *n3,
			     double *e01, double *e02, double *e03,
			     double *e12, double *e13, double *e23,
			     double *where)
{
  double jacobian[9];
  if (grid !=gridShapeJacobian2(grid,n0,n1,n2,n3, 
				e01, e02, e03, 
				e12, e13, e23, 
				where,jacobian) ) return -999.0;
  return gridMatrixDeterminate(jacobian);
}

