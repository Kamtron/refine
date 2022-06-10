
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
#include <string.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "CADGeom/CADGeom.h"

#define SubtractVector(v1,v2,result) \
(result)[0] = (v1)[0] - (v2)[0]; \
(result)[1] = (v1)[1] - (v2)[1]; \
(result)[2] = (v1)[2] - (v2)[2];

#ifdef PROE_MAIN
int GridEx_Main( int argc, char *argv[] )
#else
int main( int argc, char *argv[] )
#endif
{
  int i;
  char project[256];
  UGridPtr ugrid;  
  int nGeomNode, nGeomEdge, nGeomFace, nGeomGroups;
  UGPatchPtr  localPatch, globalPatch;
  Iterator patchIterator;
  CADCurvePtr edge;
  int inode, iedge, nedgenode;
  double trange[2];
  int edgeEndPoint[2];

  int patchDimensions[3];
  int face, localNode, globalNode;
  double pxyz[3], gxyz[3], dxyz[3];
  double oldUV[2], newUV[2], xyznew[3];
  char modeler[81];

  int *l2g, volumeEdgeNode, patchEdgeNode;

  int model;
  int vol;
  vol=1;

  i = 1;
  while( i < argc ) {
    if( strcmp(argv[i],"-felisa") == 0 ) {
      i++; sprintf( project, "%s", argv[i] );
      printf("-felisa argument %d: %s\n",i, project);
      sprintf( modeler, "FELISA" );
    } else if( strcmp(argv[i],"-open") == 0 ) {
      i++; sprintf( project, "%s", argv[i] );
      printf("-open argument %d: %s\n",i, project);
      sprintf( modeler, "OpenCASCADE" );
    } else if( strcmp(argv[i],"-proe") == 0 ) {
      i++; sprintf( project, "%s", argv[i] );
      printf("-proe argument %d: %s\n",i, project);
      sprintf( modeler, "Pro/ENGINEER" );
    } else if( strcmp(argv[i],"-parasolid") == 0 ) {
      i++; sprintf( project, "%s", argv[i] );
      printf("-parasolid argument %d: %s\n",i, project);
      sprintf( modeler, "Parasolid" );
    } else if( strcmp(argv[i],"-catia") == 0 ) {
      i++; sprintf( project, "%s", argv[i] );
      printf("-catia argument %d: %s\n",i, project);
      sprintf( modeler, "CatiaV5" );
    } else if( strcmp(argv[i],"-ug") == 0 ) {
      i++; sprintf( project, "%s", argv[i] );
      printf("-ug argument %d: %s\n",i, project);
      sprintf( modeler, "UniGraphics" );
    } else if( strcmp(argv[i],"-sw") == 0 ) {
      i++; sprintf( project, "%s", argv[i] );
      printf("-sw argument %d: %s\n",i, project);
      sprintf( modeler, "SolidWorks" );
    } else {
      fprintf(stderr,"Argument \"%s %s\" Ignored\n",argv[i],argv[i+1]);
      i++;
    }
    i++;
  }
  
  printf("calling CADGeom_Start ... \n");
  if ( ! CADGeom_Start( ) ){
    printf("ERROR: CADGeom_Start broke.\n%s\n",ErrMgr_GetErrStr());
    return 1;
  }  

  printf("calling CADGeom_Load for project <%s> ... \n",project);
  if ( ! CADGeom_LoadModel( NULL, modeler, project, &model ) ){
    printf("ERROR: CADGeom_LoadModel broke.\n%s\n",ErrMgr_GetErrStr());
    return 1;
  }

  if (NULL == (ugrid = CADGeom_VolumeGrid(vol)) ) {
    printf("ERROR: Can not find grid in restart. \n%s\n",ErrMgr_GetErrStr());
    return 1;
  }

  if( !CADGeom_GetVolume(vol,&nGeomNode,&nGeomEdge,&nGeomFace,&nGeomGroups) ) {
    printf("ERROR: CADGeom_GetVolume. \n%s\n",ErrMgr_GetErrStr());
  }

  inode = nGeomNode;
  for( iedge=1; iedge<=nGeomEdge; iedge++ ) {
    printf("edge %3d\n",iedge);
    if( (edge=CADGeom_EdgeGrid(vol,iedge)) == NULL ) 
      printf("ERROR: CADGeom_EdgeGrid(%d).\n%s\n",iedge,ErrMgr_GetErrStr());
 
    nedgenode = CADCURVE_NUMPTS(edge);

    CADGeom_GetEdge( vol, iedge, trange, edgeEndPoint );

    edgeEndPoint[0]--; /* convert from fortran to c numbers */
    edgeEndPoint[1]--;

    printf("e%3d l%6d%10.5f%10.5f%10.5f g%6d%10.5f%10.5f%10.5f\n",iedge,
	   0, 
	   CADCurve_Coord(edge,0,0),
	   CADCurve_Coord(edge,0,1),
	   CADCurve_Coord(edge,0,2),
	   edgeEndPoint[0],
	   UGrid_PtValue(ugrid,edgeEndPoint[0],0),
	   UGrid_PtValue(ugrid,edgeEndPoint[0],1),
	   UGrid_PtValue(ugrid,edgeEndPoint[0],2) );
    for( i=1 ; i < (nedgenode-1) ; i++ ) { // skip end segments  
    printf("e%3d l%6d%10.5f%10.5f%10.5f g%6d%10.5f%10.5f%10.5f\n",iedge,
	   i,
	   CADCurve_Coord(edge,i,0),
	   CADCurve_Coord(edge,i,1),
	   CADCurve_Coord(edge,i,2),
	   inode,
	   UGrid_PtValue(ugrid,inode,0),
	   UGrid_PtValue(ugrid,inode,1),
	   UGrid_PtValue(ugrid,inode,2) );
      inode++;
    }
    printf("e%3d l%6d%10.5f%10.5f%10.5f g%6d%10.5f%10.5f%10.5f\n",iedge,
	   nedgenode-1,
	   CADCurve_Coord(edge,nedgenode-1,0),
	   CADCurve_Coord(edge,nedgenode-1,1),
	   CADCurve_Coord(edge,nedgenode-1,2),
	   edgeEndPoint[1],
	   UGrid_PtValue(ugrid,edgeEndPoint[1],0),
	   UGrid_PtValue(ugrid,edgeEndPoint[1],1),
	   UGrid_PtValue(ugrid,edgeEndPoint[1],2) );
  }
  
  globalPatch = DList_SetIteratorToHead(UGrid_PatchList(ugrid),&patchIterator);

  for( face=1; face<=nGeomFace; face++ ) {
    printf("face %3d UGPatch_GlobalIndex\n",face);
    localPatch = CADGeom_FaceGrid(vol,face);
    UGPatch_GetDims(localPatch,patchDimensions);
    for( localNode=0; localNode<patchDimensions[0]; localNode++ ) {
      globalNode = UGPatch_GlobalIndex(globalPatch,localNode);
      gxyz[0] = UGrid_PtValue(ugrid,globalNode,0);
      gxyz[1] = UGrid_PtValue(ugrid,globalNode,1);
      gxyz[2] = UGrid_PtValue(ugrid,globalNode,2);
      pxyz[0] = UGPatch_PtValue(localPatch,localNode,0);
      pxyz[1] = UGPatch_PtValue(localPatch,localNode,1);
      pxyz[2] = UGPatch_PtValue(localPatch,localNode,2);
      SubtractVector( pxyz, gxyz, dxyz);
      printf("f%3d p%6d%10.5f%10.5f%10.5f g%6d%10.5f%10.5f%10.5f\n",face, 
	     localNode, pxyz[0], pxyz[1], pxyz[2], 
	     globalNode, gxyz[0], gxyz[1], gxyz[2]);
    }

    globalPatch = DList_GetNextItem(&patchIterator);
  }

  if (!CADTopo_VolEdgePts( vol, &volumeEdgeNode )){
    printf("%s: %d: CADTopo_VolEdgePts failied.\n",__FILE__, __LINE__);
    return 1;
  }

  inode = volumeEdgeNode;
  
  for( face=1; face<=nGeomFace; face++ ) {
    printf("face %3d CADTopo_VolFacePts\n",face);
    localPatch = CADGeom_FaceGrid(vol,face);
    UGPatch_GetDims(localPatch,patchDimensions);
    l2g = malloc(patchDimensions[0]*sizeof(int));
    if (!CADTopo_VolFacePts(vol, face, l2g, &patchEdgeNode)) {
      printf("%s: %d: CADTopo_VolFacePts failied.\n",__FILE__, __LINE__);
      return 1;
    }
    for( localNode=patchEdgeNode;localNode<patchDimensions[0]; localNode++ ) {
      l2g[localNode] = inode; inode++;
    }
    for( localNode=0; localNode<patchDimensions[0]; localNode++ ) {
      globalNode = l2g[localNode];
      gxyz[0] = UGrid_PtValue(ugrid,globalNode,0);
      gxyz[1] = UGrid_PtValue(ugrid,globalNode,1);
      gxyz[2] = UGrid_PtValue(ugrid,globalNode,2);
      pxyz[0] = UGPatch_PtValue(localPatch,localNode,0);
      pxyz[1] = UGPatch_PtValue(localPatch,localNode,1);
      pxyz[2] = UGPatch_PtValue(localPatch,localNode,2);
      SubtractVector( pxyz, gxyz, dxyz);
      printf("f%3d p%6d%10.5f%10.5f%10.5f g%6d%10.5f%10.5f%10.5f\n",face, 
	     localNode, pxyz[0], pxyz[1], pxyz[2], 
	     globalNode, gxyz[0], gxyz[1], gxyz[2]);
      oldUV[0] = newUV[0] = UGPatch_Parameter(localPatch,localNode,0);
      oldUV[1] = newUV[1] = UGPatch_Parameter(localPatch,localNode,1);
      if (!CADGeom_NearestOnFace( vol, face, pxyz, newUV, xyznew) ) {
	printf("%s: %d: CADGeom_NearestOnFace failed.",__FILE__,__LINE__);
	return 1;  
      }
      printf("proj u%10.5f%10.5f v%10.5f%10.5f j%10.5f%10.5f%10.5f\n",
	     oldUV[0], newUV[0], oldUV[1], newUV[1],
	     xyznew[0], xyznew[1], xyznew[2]);
    }
   
    free(l2g);
  }

}
