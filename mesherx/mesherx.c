
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>         /* Needed in some systems for DBL_MAX definition */
#include <float.h>
#include "mesherx.h"
#include "mesherxInit.h"
#include "mesherxRebuild.h"
#include "grid.h"
#include "gridfiller.h"
#include "gridmetric.h"
#include "gridswap.h"

int MesherX_DiscretizeVolume( int maxNodes, double scale, char *project,
			      GridBool mixedElement,
			      GridBool blendElement,
			      GridBool qualityImprovement,
			      GridBool copyGridY,
			      GridBool bil )
{
  char outputProject[256];
  char linesProject[256];
  int vol=1;
  Grid *grid;
  Layer *layer;
  int i;
  double h;
  double rate;
  int nLayer;
  int face;
  double gapHeight;
  double origin[3] = {0.0, 0.0, 0.0};
  double direction[3] = {0, 1, 0};

  layer = mesherxInit(vol, maxNodes);
  if (NULL == layer) return 0; 
  grid = layerGrid(layer);

  /* case dependant */

  nLayer = (int)(60.0/scale);
  rate = exp(scale*log(1.20));

  printf("rate is set to %10.5f for %d layers\n",rate,nLayer);

  if (mixedElement) layerToggleMixedElementMode(layer);

  origin[0] = -10000;
  origin[1] = 0.0;
  origin[2] = 0.0;
  direction[0] = 1.0;
  direction[1] = 0.0;
  direction[2] = 0.0;
  layerSetPolynomialMaxHeight(layer, 0.5, 0.0, 1.0, 
			      origin, direction );
  layerAssignPolynomialNormalHeight(layer, 1.0e-4, 0.0, 1.0,
                                    origin, direction );
  layerScaleNormalHeight(layer,scale);
  layerSaveInitialNormalHeight(layer);

  if (blendElement){
    printf("inserting blends...\n");
    layerBlend(layer, 250.0 );
    printf("prevent blend normal collision...TURNED OFF\n");
    //layerPreventBlendNormalDirectionFromPointingAtNeighbors(layer,0.8);
    printf("inserting sub blends...\n");
    layerSubBlend(layer, 30.0 );
  }

  layerWriteTecplotFrontGeometry(layer);

  layerComputeNormalRateWithBGSpacing(layer,1.0);

  layerSmoothInteriorNormalDirection(layer,-1.0,-1,-1.0);

  i=0;
  while (i<nLayer & layerAnyActiveNormals(layer)){
    i++;

    if (i>(int)(10.0/scale)) rate += exp(scale*log(0.01));
    if (rate>exp(scale*log(1.3))) rate=exp(scale*log(1.3));
    if (i>(int)(25.0/scale)) rate += exp(scale*log(0.01));
    if (rate>exp(scale*log(1.4))) rate=exp(scale*log(1.4));
    if (i>1) layerScaleNormalHeight(layer,rate);
    //layerSetNormalHeightWithMaxRate(layer,rate);
    //layerSetNormalHeightForLayerNumber(layer,i-1,rate);
    //layerSmoothLayerWithHeight(layer);

    layerTerminateNormalWithLength(layer,1.0);
    layerTerminateNormalWithBGSpacing(layer, 0.8, 1.6);

    if (i>10) layerTerminateCollidingTriangles(layer,1.1);

    printf("advance layer %d\n",i);
    layerTerminateFutureNegativeCellNormals(layer);
    if (layer != layerAdvance(layer, FALSE)) {
      printf("Error, layer advancement failed.\n");
      layerWriteTecplotFrontGeometry(layer);
      return 0;
    }
    if (i/5*5==i) layerWriteTecplotFrontGeometry(layer);
  }
  layerWriteTecplotFrontGeometry(layer);
/* case dep */

  if ( layer != layerRebuildInterior(layer,vol) ) return 0;

  if ( qualityImprovement ){
    printf(" -- QUALITY IMPROVEMENT\n");
    layerThaw(layer);
    printf("minimum Thawed Aspect Ratio %8.6f Mean Ratio %8.6f Volume %10.6e\n", gridMinThawedAR(grid),gridMinThawedFaceMR(grid), gridMinVolume(grid));
    for (i=0;i<3;i++){
      printf("edge swapping grid...\n");gridSwap(grid, -1.0);
      printf("minimum Thawed Aspect Ratio %8.6f Mean Ratio %8.6f Volume %10.6e\n", gridMinThawedAR(grid),gridMinThawedFaceMR(grid), gridMinVolume(grid));
    }
  }

  printf("total grid size: %d nodes %d faces %d cells.\n",
	 gridNNode(grid),gridNFace(grid),gridNCell(grid));

  printf(" -- DUMP PART\n");

  if ( copyGridY ) {
    printf("copy grid about y=0.\n");
    gridCopyAboutY0(grid,EMPTY,EMPTY);
    printf("total grid size: %d nodes %d faces %d cells.\n",
	   gridNNode(grid),gridNFace(grid),gridNCell(grid));
  }

  if ( NULL != project ) {

    if (mixedElement) {
      sprintf(outputProject,"%s_mx.ugrid",project);
      printf("writing output AFL3R file %s\n",outputProject);
      gridExportAFLR3( grid, outputProject  );
    }else{
      sprintf(outputProject,"%s_mx.fgrid",project);
      printf("writing output FAST file %s\n",outputProject);
      gridExportFAST( grid, outputProject  );
    }
  }

  if (!bil && !copyGridY ) {
    if ( project == NULL ) {
      gridSavePart( grid, NULL );
    }else{
      sprintf(outputProject,"%s_mx",project);
      printf("writing output GridEx/CADGeom/CAPRI project %s\n",outputProject);
      gridSavePart( grid, outputProject );
    }
  }else{
    printf("skip save grid.\n");
  }

  if ( project != NULL ) {
    sprintf(linesProject,"%s_mx.lines",project);
    printf("saving lines restart %s\n",linesProject);
    linesSave(gridLines(grid),linesProject);
  }

  printf("MesherX Done.\n");
  return 1;
}

Layer *layerComputeNormalRateWithBGSpacing(Layer *layer, double finalRatio)
{
  int normal, root;
  double xyz[3], length, normalDirection[3];
  double initialDelta, finalDelta, rate;
  double spacing[3], direction[9];

  for(normal=0;normal<layerNNormal(layer);normal++){
    root = layerNormalRoot(layer, normal );
    gridNodeXYZ(layerGrid(layer),root,xyz);
    layerNormalDirection(layer,normal,normalDirection);
    length = layerNormalMaxLength(layer,normal);
    xyz[0] += (length * normalDirection[0]);
    xyz[1] += (length * normalDirection[1]);
    xyz[2] += (length * normalDirection[2]);
    UG_GetSpacing(&(xyz[0]),&(xyz[1]),&(xyz[2]),spacing,direction);
    finalDelta = spacing[0]*finalRatio;
    initialDelta = layerNormalInitialHeight(layer,normal);
    rate = (finalDelta - initialDelta) / length;
    gridNodeXYZ(layerGrid(layer),root,xyz);
    layerSetNormalRate(layer, normal, rate);
  }
  return layer;
}

Layer *layerComputeInitialCellHeightWithBGSpacing(Layer *layer, double finalRatio)
{
  int normal, root;
  double xyz[3], length, normalDirection[3];
  double initialDelta, finalDelta, rate;
  double spacing[3], direction[9];

  for(normal=0;normal<layerNNormal(layer);normal++){
    root = layerNormalRoot(layer, normal );
    gridNodeXYZ(layerGrid(layer),root,xyz);
    layerNormalDirection(layer,normal,normalDirection);
    length = layerNormalMaxLength(layer,normal);
    xyz[0] += (length * normalDirection[0]);
    xyz[1] += (length * normalDirection[1]);
    xyz[2] += (length * normalDirection[2]);
    UG_GetSpacing(&(xyz[0]),&(xyz[1]),&(xyz[2]),spacing,direction);
    finalDelta = spacing[0]*finalRatio;
    rate = layerNormalRate(layer,normal);
    initialDelta = finalDelta / pow(rate,length*10);
    printf("init %15.10f %15.10f %15.10f %15.10f\n",rate, initialDelta,finalDelta,length);
    layerSetNormalInitialHeight(layer, normal, initialDelta);
  }
  return layer;
}

Layer *layerCreateWakeWithBGSpacing(Layer *layer, 
				    double *origin, double *direction, 
				    double length )
{
  int i;
  double xyz[3];
  double spacing[3], map[9];
  double extrude[3], extrusion;

  i=0;
  extrusion = 0.0;
  xyz[0] = origin[0];  xyz[1] = origin[1];  xyz[2] = origin[2];
  while (extrusion < length){
    i++;
    UG_GetSpacing(&(xyz[0]),&(xyz[1]),&(xyz[2]),spacing,map);
    printf("wake%5d length%8.3f spacing%10.5f xyz%8.3f%8.3f%8.3f\n",
	   i,extrusion,spacing[0],xyz[0],xyz[1],xyz[2]);
    extrude[0] = direction[0]*spacing[0];
    extrude[1] = direction[1]*spacing[0];
    extrude[2] = direction[2]*spacing[0];
    layerExtrudeBlend(layer,extrude[0],extrude[1],extrude[2]); 
    xyz[0] += extrude[0];  xyz[1] += extrude[1];  xyz[2] += extrude[2];
    extrusion += spacing[0];
  }

}

int layerTerminateNormalWithBGSpacing(Layer *layer, 
				      double normalRatio, double edgeRatio)
{
  int normal, root;
  double xyz[3];
  double spacing[3];
  double direction[9];
  double height;
  int triangle, normals[3];
  double edgeLength, center[3];
  int totalterm, hterm, eterm;

  if (layerNNormal(layer) == 0 ) return EMPTY;

  hterm = 0;
  eterm = 0;

  for (normal=0;normal<layerNNormal(layer);normal++){
    layerGetNormalHeight(layer,normal,&height);

    root = layerNormalRoot(layer, normal );
    gridNodeXYZ(layerGrid(layer),root,xyz);
    UG_GetSpacing(&(xyz[0]),&(xyz[1]),&(xyz[2]),spacing,direction);

    if (height > normalRatio*spacing[0]) {     /* Assume Isotropic for now */
      if( !layerNormalTerminated(layer, normal ) ) hterm++;
      layerTerminateNormal(layer, normal);
    }
  }

  for (triangle=0;triangle<layerNTriangle(layer);triangle++){
    layerAdvancedTriangleMaxEdgeLength(layer,triangle,&edgeLength );
    layerTriangleCenter(layer,triangle,center);

    UG_GetSpacing(&(center[0]),&(center[1]),&(center[2]),
		       spacing,direction);

    if ( edgeLength > edgeRatio*spacing[0]) { /* Assume Isotropic for now */
      layerTriangleNormals(layer, triangle, normals);
      if( !layerNormalTerminated(layer, normals[0] ) ) eterm++;
      if( !layerNormalTerminated(layer, normals[1] ) ) eterm++;
      if( !layerNormalTerminated(layer, normals[2] ) ) eterm++;
      layerTerminateNormal(layer, normals[0]);
      layerTerminateNormal(layer, normals[1]);
      layerTerminateNormal(layer, normals[2]);
    }
  }

  totalterm = layerNNormal(layer)-layerNActiveNormal(layer);
  printf("%d and %d terminated by BGS h and edge test; ",
	 hterm, eterm, totalterm,layerNNormal(layer) );
  return totalterm;
}


