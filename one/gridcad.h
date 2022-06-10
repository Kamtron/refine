
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

#ifndef GRIDCAD_H
#define GRIDCAD_H

#include "refine_defs.h"
#include "grid.h"

BEGIN_C_DECLORATION

GridBool nearestOnEdge(int vol, int edge, double *xyz, double *t,
                       double *xyznew);
GridBool nearestOnFace(int vol, int face, double *xyz, double *uv,
                       double *xyznew);

Grid *gridForceNodeToEdge(Grid *g, int node, int edgeId );
Grid *gridForceNodeToFace(Grid *g, int node, int faceId );

Grid *gridProjectNodeToEdge(Grid *g, int node, int edgeId );
Grid *gridProjectNodeToFace(Grid *g, int node, int faceId );

Grid *gridEvaluateEdgeAtT(Grid *g, int node, double t );
Grid *gridEvaluateFaceAtUV(Grid *g, int node, double *uv );

Grid *gridUpdateParameters(Grid *g, int node );
Grid *gridUpdateFaceParameters(Grid *g, int node );

Grid *gridProjectToEdge(Grid *g, int edgeId, 
			double *xyz, double *t, double *newxyz );
Grid *gridProjectToFace(Grid *g, int faceId, 
			double *xyz, double *uv, double *newxyz );
Grid *gridEvaluateOnEdge(Grid *g, int edgeId, double t, double *xyz );
Grid *gridEvaluateOnFace(Grid *g, int faceId, double *uv, double *xyz );
Grid *gridResolveOnFace(Grid *grid, int faceId,
			double *uv, double *original_xyz, double *resolved_xyz);

Grid *gridFaceNormalAtUV(Grid *g, int faceId, 
			 double *uv, double *xyz, double *normal );
Grid *gridFaceNormalAtXYZ(Grid *g, int faceId, double *xyz, double *normal );

Grid *gridProjectNode(Grid *g, int node );

Grid *gridNodeProjectionDisplacement(Grid *g, int node, double *displacement );

Grid *gridRobustProject(Grid *g);

Grid *gridWholesaleEvaluation(Grid *g);

/* the {optimzation,laplacian}Limits will be set to a default if < 0.0 */ 
Grid *gridSmooth(Grid *g, double optimizationLimit, double laplacianLimit );
Grid *gridSmoothVolume(Grid *g );
Grid *gridSmoothNode(Grid *g, int node, GridBool smoothOnSurface );

Grid *gridLineSearchT(Grid *g, int node, double optimized_cost_limit );

Grid *gridLinearProgramUV(Grid *g, int node, GridBool *callAgain );

Grid *gridSmartLaplacian(Grid *g, int node );

Grid *gridStoreVolumeCostDerivatives(Grid *g, int node );
Grid *gridStoreFaceCostParameterDerivatives(Grid *g, int node );
Grid *gridRestrictStoredCostToUV(Grid *g, int node );

Grid *gridLinearProgramXYZ(Grid *g, int node, GridBool *callAgain );

Grid *gridRelaxNegativeCells(Grid *g, GridBool dumpTecplot );
Grid *gridSmoothVolumeNearNode(Grid *grid, int node, 
			       GridBool smoothOnSurface );

Grid *gridSmoothNodeFaceAreaUV(Grid *g, int node );
Grid *gridSmoothNodeFaceAreaUVSimplex( Grid *g, int node );

Grid *gridUntangleAreaUV(Grid *g, int node, int recursive_depth, 
			 GridBool allow_movement_near_ghost_nodes );
Grid *gridUntangleVolume(Grid *g, int node, int recursive_depth, 
			 GridBool allow_movement_near_ghost_nodes  );

END_C_DECLORATION

#endif /* GRIDCAD_H */
