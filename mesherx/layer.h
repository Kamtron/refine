
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

#ifndef LAYER_H
#define LAYER_H

#include "refine_defs.h"
#include "grid.h"
#include "near.h"

BEGIN_C_DECLORATION

#define MAXNORMALDEG 100

typedef struct Normal Normal;
struct Normal {
  int constrained;
  int root, tip;
  double direction[3];
  double height;
  double initialheight;
  double length;
  double maxlength;
  double rate;
  GridBool terminated;
  GridBool directionFrozen;
};

typedef struct Triangle Triangle;
struct Triangle {
  int normal[3];
  int constrainedSide[3];
  int parentGeomEdge[3];
};

#define MAXSUBNORMAL (20)
typedef struct Blend Blend;
struct Blend {
  int nodes[2];
  int normal[4];
  int edgeId[2];
  int oldnormal[4];

  int nSubNormal0, nSubNormal1;
  int subNormal0[MAXSUBNORMAL];
  int subNormal1[MAXSUBNORMAL];
};

typedef struct Layer Layer;
struct Layer {
  Grid *grid;
  void *gridRubyVALUEusedForGC;
  int maxtriangle, ntriangle;
  Triangle *triangle;
  Adj *triangleAdj;
  int maxParentGeomFace, nParentGeomFace, *ParentGeomFace;
  int maxblend, nblend;
  Blend *blend;
  Adj *blendAdj;
  int maxnormal, nnormal, originalnormal;
  Normal *normal;
  int *globalNode2Normal;
  int *vertexNormal;
  int nConstrainingGeometry, *constrainingGeometry;
  Near *nearTree;
  GridBool mixedElementMode;
  GridBool cad_available;

  int normalTriangleHub;
  GridBool normalTriangleExclusive;
  double normalTriangleDirection[3*MAXNORMALDEG];
  GridBool normalTriangleExclude[MAXNORMALDEG];

  GridBool *cellInLayer;
  GridBool *faceInLayer;
  GridBool *edgeInLayer;

  FILE *tecplotFile;
};

Layer *layerCreate(Grid *);
Grid *layerGrid(Layer *);
void layerFree(Layer *);
Layer *formAdvancingFront( Grid *grid, char *project );
void layerPack(void *voidLayer, 
	       int nnode, int maxnode, int *nodeo2n,
	       int ncell, int maxcell, int *cello2n,
	       int nface, int maxface, int *faceo2n,
	       int nedge, int maxedge, int *edgeo2n);
void layerSortNodes(void *voidLayer, int maxnode, int *o2n);
void layerReallocator(void *voidLayer, int reallocType, 
		      int lastSize, int newSize);
void layerGridHasBeenFreed(void *voidLayer );
int layerMaxTriangle(Layer *);
int layerNTriangle(Layer *);
int layerNBlend(Layer *);
int layerMaxNormal(Layer *);
int layerNNormal(Layer *);
int layerMaxNode(Layer *);
Layer *layerPopulateAdvancingFront(Layer *, int nbc, int *bc);
Layer *layerBuildNormalTriangleAdjacency(Layer *);
Layer *layerInitializeTriangleNormalDirection(Layer *);
Layer *layerAddParentGeomFace(Layer *, int faceId);
GridBool layerParentGeomFace(Layer *, int faceId);
Layer *layerAddTriangle(Layer *, int n0, int n1, int n2);
int layerForceTriangle(Layer *, int normal0, int normal1, int nnormal2);
Layer *layerTriangle(Layer *, int triangle, int *nodes);
Layer *layerTriangleDirection(Layer *, int triangle, double *direction);
Layer *layerTriangleArea(Layer *, int triangle, double *area);
Layer *layerTriangleCenter(Layer *, int triangle, double *center);
Layer *layerTriangleFourthNode(Layer *, int triangle, double *xyz);
Layer *layerTriangleInviscidTet(Layer *, int triangle, double scale, 
				double *node0, double *node1,
				double *node2, double *node3);
Layer *layerTriangleMaxEdgeLength(Layer *, int triangle, double *length);
Layer *layerAdvancedTriangleMaxEdgeLength(Layer *, 
					  int triangle, double *length);
#define layerTriangleAdj(layer) (layer->triangleAdj)
Layer *layerNormalMaxEdgeLength(Layer *, int normal, double *length);
int layerAddNormal(Layer *, int globalNodeId );
int layerUniqueNormalId(Layer *, int globalNodeId );
int layerDuplicateNormal(Layer *, int normal );
Layer *layerInitializeNormal(Layer *, int normal );
Layer *layerTriangleNormals(Layer *, int triangle, int *normals);
int layerNormalRoot(Layer *, int normal );
int layerNormalTip(Layer *, int normal );
int layerNormalDeg(Layer *, int normal );
double layerNormalAngle(Layer *, int normal0, int normal1);
Layer *layerNormalTriangles(Layer *, int normal, int maxtriangle, int *triangles);
int layerPreviousTriangle(Layer *, int normal, int triangle );
int layerNextTriangle(Layer *, int normal, int triangle );
Layer *layerNormalMinDot(Layer *, int normal, double *mindot, double *mindir );
Layer *layerStoreNormalTriangleDirections(Layer *, int normal);
Layer *layerNormalTriangleDirection(Layer *, 
				    int normal, int index, double *direction);
Layer *layerCommonEdge(Layer *, int triangle0, int triangle1, int *nodes);
double layerEdgeAngle(Layer *, int triangle0, int triangle1 );
Layer *layerNormalDirection(Layer *, int normal, double *direction);
GridBool layerNormalDirectionFrozen(Layer *, int normal);
Layer *layerNormalDirectionFreeze(Layer *, int normal);
Layer *layerAssignPolynomialNormalHeight(Layer *, double constant, double slope, 
                                         double exponent, double *origin,
					 double *direction);
Layer *layerSetHeightOfAllNormals(Layer *, double height);
Layer *layerLaminarInitialHeight(Layer *, double Re, double xStart );
Layer *layerLaminarInitialHeightNegZ(Layer *);
Layer *layerSetNormalHeightOfFace(Layer *, int faceId, double height);
Layer *layerSetNormalHeight(Layer *, int normal, double height);
Layer *layerGetNormalHeight(Layer *, int normal, double *height);
Layer *layerScaleNormalHeight(Layer *, double scale);
Layer *layerScaleNormalHeightWithPolynomial(Layer *, 
					    double constant, double slope,
				            double exponent, double scale,
					    double *origin, double *direction);
Layer *layerSetNormalMaxLength(Layer *, int normal, double maxLength);
double layerNormalMaxLength(Layer *, int normal);
Layer *layerSetPolynomialMaxLength(Layer *, double constant, double slope, 
				            double exponent, double *origin,
				            double *direction);
Layer *layerSetNormalInitialHeight(Layer *layer, int normal, 
				   double initialHeight);
Layer *layerSaveInitialNormalHeight(Layer *);
double layerNormalInitialHeight(Layer *, int normal);
Layer *layerSetNormalRate(Layer *, int normal, double rate);
double layerNormalRate(Layer *, int normal);
Layer *layerSetAllNormalRate(Layer *, double rate);
Layer *layerSetNormalHeightWithRate(Layer *);
Layer *layerSetNormalHeightWithMaxRate(Layer *, double maxRate);
Layer *layerSetNormalHeightForLayerNumber(Layer *, int n, double rate);

Layer *layerFeasibleNormals(Layer *, double dotLimit, double relaxation );
Layer *layerVisibleNormals(Layer *, double dotLimit, double radianLimit );
Layer *layerSmoothNormalDirection(Layer *, double relax);
Layer *layerSmoothInteriorNormalDirection(Layer *,
                                          double relax, int iterations,
                                          double visibility);
Layer *layerProjectNormalToConstraints(Layer *, int normal);
Layer *layerProjectNormalsToConstraints(Layer *);

Layer *layerAdjustNormalHeightToSmoothFront(Layer *, double maxHeight);

Layer *layerConstrainNormal(Layer *, int edgeface );
GridBool layerConstrainingGeometry(Layer *, int edgeface );
int layerConstrained(Layer *, int normal );
Layer *layerConstrainTriangleSide(Layer *, int normal0, int normal1, int bc );
int layerConstrainedSide(Layer *, int triangle, int side );
int layerNConstrainedSides(Layer *, int faceId );
Layer *layerFindParentGeomEdges(Layer *);
Layer *layerSetParentGeomEdge(Layer *, int normal0, int normal1, int edgeId );
int layerParentGeomEdge(Layer *, int triangle, int side );
int layerNParentGeomEdgeSegments(Layer *, int edgeId );

Layer *layerTerminateNormal(Layer *, int normal );
GridBool layerNormalTerminated(Layer *, int normal );
Layer *layerTerminateFaceNormals(Layer *, int faceId );
Layer *layerTerminateTriangleNormals(Layer *, int triangle );
int layerNActiveNormal(Layer *);
GridBool layerAnyActiveNormals(Layer *);

GridBool layerCellInLayer(Layer *, int cell);
GridBool layerFaceInLayer(Layer *, int face);
GridBool layerEdgeInLayer(Layer *, int edge);
int layerNEdgeInLayer(Layer *, int edgeId);
int layerEdgeEndPoint(Layer *, int edgeId, int startNode);

Layer *layerReconnectCellUnlessInLayer(Layer *, int oldNode, int newNode );
Layer *layerReconnectEdgeUnlessInLayer(Layer *, int edgeId, 
				       int oldNode, int newNode );
Layer *layerReconnectFaceUnlessInLayer(Layer *, int faceId, 
				       int oldNode, int newNode );

Layer *layerAdvance(Layer *, GridBool reconnect );
Layer *layerAdvanceConstantHeight(Layer *, double height );
Layer *layerWiggle(Layer *, double height );

Layer *layerTerminateNormalWithSpacing(Layer *, double spacing);
Layer *layerTerminateNormalWithX(Layer *, int direction, double x);
int layerTerminateNormalWithLength(Layer *, double ratio);

Layer *layerThaw(Layer*);

GridBool layerCADAvailable(Layer *);
Layer *layerMakeCADAvailable(Layer *);

GridBool layerTetrahedraOnly(Layer *);
Layer *layerToggleMixedElementMode(Layer *);

Adj *layerBuildNormalBlendAdjacency(Layer *layer);
int layerFirstTriangleAfterGap(Layer *, int normal );
int layerNRequiredBlends(Layer *, int normal, double angleLimit );
Layer *layerBlend(Layer *, double angleLimit );
int layerAddBlend(Layer *, int normal0, int normal1, int otherNode );
Layer *layerBlendNormals(Layer *, int blend, int *normals );
Layer *layerSubBlendNormals(Layer *, int blend, int subBlend, int *normals );
Layer *layerBlendAxle(Layer *, int blend, double *axle);
Layer *layerNormalBlendAxle(Layer *, int normal, double *axle);
#define layerBlendAdj(layer) (layer->blendAdj)
int layerBlendDegree(Layer *, int normal);
int layerSubNormalDegree(Layer *, int normal);

Layer *layerSubBlend(Layer *, double maxNormalAngle);
int layerNSubBlend(Layer *, int blend );

Layer *layerBlendNormalDirectionFreeze(Layer *);

Layer *layerPreventBlendNormalDirectionFromPointingAtNeighbors(Layer *, double dot);

Layer *layerExtrudeBlend(Layer *, double dx, double dy, double dz );

Layer *layerOrderedVertexBlends(Layer *, int normal, 
				 int *nVertexBlends, int *vertexBlends );
Layer *layerOrderedVertexNormals(Layer *, int normal, 
				 int *vertexNormals );

Layer *layerPopulateNormalNearTree(Layer *);
Layer *layerPopulateTriangleNearTree(Layer *, double scale);
Layer *layerTerminateCollidingNormals(Layer *);
Layer *layerTerminateCollidingTriangles(Layer *, double scale);

Layer *layerTerminateFutureNegativeCellNormals(Layer *);

Layer *layerSmoothLayerWithHeight(Layer *);

Layer *layerOffsetTriangleMR(Layer *, int triangle, int normal, double offset,
			     double *MR, double *dMRdX );

Layer *layerOptimizeNormalDirection(Layer *, double offsetRatio );

Layer *layerWriteTecplotFrontGeometry(Layer *);
Layer *layerWriteTecplotFrontWithData(Layer *, int);

END_C_DECLORATION

#endif /* LAYER_H */
