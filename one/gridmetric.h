
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

#ifndef GRIDMETRIC_H
#define GRIDMETRIC_H

#include "refine_defs.h"
#include "grid.h"
#include "gridmath.h"

BEGIN_C_DECLORATION

Grid *gridWriteTecplotInvalid(Grid *g, char *filename );

Grid *gridSetMapMatrixToAverageOfNodes2(Grid *g, int avgNode,
					int n0, int n1 );
Grid *gridSetMapMatrixToAverageOfNodes3(Grid *g, int avgNode,
					int n0, int n1, int n2 );
Grid *gridSetMapMatrixToAverageOfNodes4(Grid *g, int avgNode,
					int n0, int n1, int n2, int n3 );
void gridMapXYZWithJ( double *j, double *x, double *y, double *z );

double gridEdgeLength(Grid *g, int n0, int n1 );
Grid *gridEdgeRatioTolerence(Grid *g, double longest, double shortest,
			     int *active_edges, int *out_of_tolerence_edges );
Grid *gridEdgeRatioRange(Grid *g, double *longest, double *shortest );
double gridEdgeRatio(Grid *g, int n0, int n1 );
double gridAverageEdgeLength(Grid *g, int node );
Grid *gridLargestRatioEdge(Grid *g, int node, int *edgeNode, double *ratio );
Grid *gridSmallestRatioEdge(Grid *g, int node, int *edgeNode, double *ratio );

double gridSpacing(Grid *g, int node );
Grid *gridResetSpacing(Grid *g );
Grid *gridScaleSpacing(Grid *g, int node, double scale );

/* rename to gridCopyMap */
Grid *gridCopySpacing(Grid *g, int originalNode, int newNode);

Grid *gridConvertMetricToJacobian(Grid *g, double *m, double *j);

double gridVolume(Grid *g, int *nodes );

double gridCostValid(Grid *g, int *nodes );
Grid *gridNodeCostValid(Grid *grid, int node, double *valid );

double gridAR(Grid *g, int *nodes );
double gridCellMetricConformity( double *xyz0, double *xyz1, 
				 double *xyz2, double *xyz3,
				 double *requested_metric );
Grid *gridCellMetricConformityFD( Grid *grid,
				  double *xyz0, double *xyz1, 
				  double *xyz2, double *xyz3,
				  double *requested_metric,
				  double *cost, double *dCostdx );
double gridCellAspectRatio( double *n0, double *n1, double *n2, double *n3 );
Grid *gridNodeValid(Grid *g, int node, double *valid );
Grid *gridNodeAR(Grid *g, int node, double *ar );
Grid *gridNodeVolume(Grid *g, int node, double *volume );
Grid *gridGemAR(Grid *g, double *ar);
Grid *gridCellARDerivative(Grid *g, int *nodes, double *ar, double *dARdx );

void gridCellAspectRatioDerivative( double *xyz1, double *xyz2, 
				    double *xyz3, double *xyz4,
				    double *ar, double *dARdx);

Grid *gridNodeARDerivative(Grid *g, int node, double *ar, double *dARdx );
double gridMinVolume(Grid *g);
Grid *gridMinVolumeAndCount(Grid *g, double *min_volume, int *count);
GridBool gridNegCellAroundNode(Grid *g, int node );
GridBool gridNegCellAroundNodeExceptGem(Grid *g, int node );
double gridMinARAroundNodeExceptGem(Grid *g, int node );
double gridMinARAroundNodeExceptGemRecon(Grid *g, int node, int becomes );
double gridMinAR(Grid *g);
double gridMinThawedAR(Grid *g);

GridBool gridRightHandedFace(Grid *g, int face );
GridBool gridRightHandedBoundary(Grid *g );
Grid *gridFlipLeftHandedFaces(Grid *g );
GridBool gridRightHandedBoundaryUV(Grid *g );

double gridFaceArea(Grid *g, int n0, int n1, int n2);
double gridFaceAreaUV(Grid *g, int face);
double gridFaceAreaUVDirect(double *uv0,  double *uv1,  double *uv2, 
			    int faceId);
Grid *gridMinFaceAreaUV(Grid *g, int node, double *min_area);
double gridMinCellFaceAreaUV(Grid *g, int *nodes );
double gridMinGridFaceAreaUV(Grid *g);

double gridFaceAR(Grid *g, int n0, int n1, int n2);
double gridFaceMR(Grid *g, int n0, int n1, int n2);
double gridMinFaceMR(Grid *g);
double gridMinThawedFaceMR(Grid *g);
Grid *gridFaceMRDerivative(Grid *g, int* nodes, double *mr, double *dMRdx );

void FaceMRDerivative(double x1, double y1, double z1,
		      double x2, double y2, double z2,
		      double x3, double y3, double z3,
		      double *mr, double *dMRdx  );
Grid *gridNodeFaceMR(Grid *g, int node, double *mr );
Grid *gridNodeFaceMRDerivative(Grid *g, int node, double *mr, double *dMRdx );

double gridCellMeanRatio( double *n0, double *n1, double *n2, double *n3 );

void gridCellMeanRatioDerivative( double *xyz0, double *xyz1, 
				  double *xyz2, double *xyz3,
				  double *mr, double *dMRdx);

Grid *gridCollapseCost(Grid *g, int node0, int node1, double *currentCost, 
		       double *node0Cost, double *node1Cost);

Grid *gridSplitCost(Grid *g, int node0, int node1, 
		    double *currentCost, double *splitCost );

Grid *gridSpacingFromTecplot(Grid *g, char *filename );

END_C_DECLORATION

#endif /* GRIDMETRIC_H */
