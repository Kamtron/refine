
/* Michael A. Park
 * Computational Modeling & Simulation Branch
 * NASA Langley Research Center
 * Phone:(757)864-6604
 * Email:m.a.park@larc.nasa.gov 
 */
  
/* $Id$ */

#include <stdlib.h>
#include <stdio.h>
#include "gridfortran.h"
#include "plan.h"
#include "grid.h"
#include "gridmove.h"
#include "gridmetric.h"
#include "gridswap.h"
#include "gridcad.h"
#include "gridinsert.h"
#include "queue.h"
#include "gridmpi.h"
#include "gridgeom.h"

/* #define TRAPFPE 1 */
#ifdef TRAPFPE
#define _GNU_SOURCE 1
#include <fenv.h>
#endif

static Grid *grid = NULL;
static GridMove *gm = NULL;
static Queue *queue = NULL;
static Plan *plan = NULL;

void gridcreate_( int *partId, int *nnode, double *x, double *y, double *z ,
		  int *ncell, int *maxcell, int *c2n )
{
  int node, cell;

#ifdef TRAPFPE
  /* Enable some exceptions.  At startup all exceptions are masked.  */
  fprintf(stderr,"\ngridcreate_: Previous Exceptions %x\n",fetestexcept(FE_ALL_EXCEPT));
  feenableexcept (FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);
  feclearexcept(FE_ALL_EXCEPT);
  fprintf(stderr,"gridcreate_: Floating Point Exception Handling On!\n");
  fprintf(stderr,"gridcreate_: Present  Exceptions %x\n\n",fetestexcept(FE_ALL_EXCEPT));
#endif

  grid = gridCreate( *nnode, *ncell, 5000, 0);
  gridSetPartId(grid, *partId );
  gridSetCostConstraint(grid, gridCOST_CNST_VOLUME|gridCOST_CNST_AREAUV);
  queue = queueCreate( 9 ); /* 3:xyz + 6:m */
  for ( node=0; node<*nnode; node++) gridAddNode(grid,x[node],y[node],z[node]);
  for ( cell=0; cell<*ncell; cell++) gridAddCell( grid,
						  c2n[cell+0*(*maxcell)] - 1,
						  c2n[cell+1*(*maxcell)] - 1,
						  c2n[cell+2*(*maxcell)] - 1,
						  c2n[cell+3*(*maxcell)] - 1 );
#ifdef PARALLEL_VERBOSE 
  printf(" %6d populated                nnode%9d ncell%9d AR%14.10f\n",
	 gridPartId(grid),gridNNode(grid),gridNCell(grid),gridMinAR(grid));
  fflush(stdout);
#endif
}

void gridfree_( void )
{
  queueFree(queue); queue = NULL;
  gridFree(grid); grid = NULL;
}

void gridinsertboundary_( int *faceId, int *nnode, int *nodedim, int *inode, 
			  int *nface, int *dim1, int *dim2, int *f2n )
{
  int face;
  int node0, node1, node2;
  for(face=0;face<*nface;face++){
    node0 = f2n[face+0*(*dim1)] - 1;
    node1 = f2n[face+1*(*dim1)] - 1;
    node2 = f2n[face+2*(*dim1)] - 1;
    node0 = inode[node0] - 1;
    node1 = inode[node1] - 1;
    node2 = inode[node2] - 1;
    if ( gridNodeGhost(grid,node0) && 
	 gridNodeGhost(grid,node1) && 
	 gridNodeGhost(grid,node2) ) {
    }else{
      gridAddFace(grid, node0, node1, node2, *faceId);
    }
  }
}

void gridsetmap_( int *nnode, double* map )
{
  int node;
  for ( node=0; node<*nnode; node++) 
    gridSetMap( grid, node,
		map[0+6*node], map[1+6*node], map[2+6*node],
		map[3+6*node], map[4+6*node], map[5+6*node] );
#ifdef PARALLEL_VERBOSE 
  printf(" %6d applied metric                                         AR%14.10f\n",
	 gridPartId(grid),gridMinAR(grid));
#endif
}

void gridsetnodepart_( int *nnode, int *part )
{
  int node;
  for ( node=0; node<*nnode; node++) gridSetNodePart(grid, node, part[node]-1);
}

void gridsetnodelocal2global_( int *partId, int *nnodeg, 
			       int *nnode, int *nnode0, int *local2global )
{
  int node;
  gridSetPartId(grid, *partId );
  gridSetGlobalNNode(grid, *nnodeg );
  for ( node=0; node<*nnode; node++){ 
    gridSetNodeGlobal(grid, node, local2global[node]-1);
    if ( node < *nnode0 ) {
      gridSetNodePart(grid, node, *partId );
    }else{
      gridSetNodePart(grid, node, EMPTY );
    }
  }
}

void gridsetcelllocal2global_( int *ncellg, int *ncell, int *local2global )
{
  int cell;
  gridSetGlobalNCell(grid, *ncellg);
  for ( cell=0; cell<*ncell; cell++){ 
    gridSetCellGlobal(grid, cell, local2global[cell]-1);
  }
}

void gridfreezenode_( int *nodeFortran )
{
  int nodeC;
  nodeC = (*nodeFortran)-1;
  gridFreezeNode( grid, nodeC );
}

void gridparallelloadcapri_( char *modeler, char *capriProject, int *status )
{
  if( grid != gridParallelGeomLoad( grid, modeler, capriProject ) ) {
    printf( "ERROR: %s: %d: failed to load part %s, partition %d, Modeler %s\n",
	    __FILE__,__LINE__,capriProject,gridPartId(grid),modeler );
    *status = 0;
  } else {
    *status = 1;
  }
}

void gridparallelsavecapri_( char *capriProject )
{
  gridParallelGeomSave( grid, capriProject );
}

void gridprojectallfaces_( void )
{
  int face, node, nodes[3], faceId;
  double ar0, ar1;

  ar0 = gridMinAR(grid);
  for(face=0;face<gridMaxFace(grid);face++) {
    if (grid == gridFace(grid,face,nodes,&faceId) ) {
      for(node=0;node<3;node++) {
	if ( !gridNodeFrozen(grid,nodes[node]) ) {
	  if (grid != gridProjectNodeToFace(grid, nodes[node], faceId ) )
	    printf( "ERROR: %s: %d: project failed on part %d.\n",
		    __FILE__,__LINE__,gridPartId(grid) );

	}
      }
    }
  }
  ar1 = gridMinAR(grid);

#ifdef PARALLEL_VERBOSE 
  printf( " %6d project faces           initial AR%14.10f final AR%14.10f\n",
	  gridPartId(grid),ar0,ar1 );
  fflush(stdout);
#endif
}


void gridtestcadparameters_( void )
{
  int global, local; 
  int nodes[3], edge, edgeId, face, faceId;
  double oldXYZ[3], oldT, oldUV[2];
  double newXYZ[3], newT, newUV[2];
  double dXYZ[3], dist, dT, dUV;
  double xyzTol, uvTol, tTol;
  AdjIterator it;
  GridBool reportMismatch;

  reportMismatch = FALSE;
  xyzTol = 1e-7;
  uvTol  = 1e-4;
  tTol   = 1e-4;

  for(global=0;global<gridGlobalNNode(grid);global++) {
    local = gridGlobal2Local(grid,global);
    if (EMPTY!=global) {
      for ( it = adjFirst(gridFaceAdj(grid),local); 
	    adjValid(it); 
	    it = adjNext(it) ){
	face = adjItem(it);
	gridFace(grid, face, nodes, &faceId);
	gridNodeXYZ(grid,local,oldXYZ);
	gridNodeUV(grid,local,faceId,oldUV);
	gridProjectNodeToFace(grid, local, faceId );
	gridNodeXYZ(grid,local,newXYZ);
	gridNodeUV(grid,local,faceId,newUV);
	gridSubtractVector(newXYZ,oldXYZ,dXYZ);
	dist = gridVectorLength(dXYZ);
	dUV = sqrt( (newUV[0]-oldUV[0])*(newUV[0]-oldUV[0]) +
		    (newUV[1]-oldUV[1])*(newUV[1]-oldUV[1]) );
	if (reportMismatch && (dist>xyzTol ||
            (oldUV[0] != DBL_MAX && dUV > uvTol))) {
	  printf("%03d global %d local %d face %d dXYZ %e dUV %e\n",
		 gridPartId(grid), global, local, faceId,
		 dist, dUV);
	  printf("    %g %g %g [%g %g %g]\n",newXYZ[0],newXYZ[1],newXYZ[2],
                 oldXYZ[0],oldXYZ[1],oldXYZ[2]);
	  printf("    %g %g [%g %g]\n",newUV[0],newUV[1],oldUV[0],oldUV[1]);
        }
      }
      for ( it = adjFirst(gridEdgeAdj(grid),local); 
	    adjValid(it); 
	    it = adjNext(it) ){
	edge = adjItem(it);
	gridEdge(grid, edge, nodes, &edgeId);
	gridNodeXYZ(grid,local,oldXYZ);
	gridNodeT(grid,local,edgeId,&oldT);
	gridProjectNodeToEdge(grid, local, edgeId );
	gridNodeXYZ(grid,local,newXYZ);
	gridNodeT(grid,local,edgeId,&newT);
	gridSubtractVector(newXYZ,oldXYZ,dXYZ);
	dist = gridVectorLength(dXYZ);
	dT = ABS(newT-oldT);
	if (reportMismatch && (dist>xyzTol || dT > tTol))
	  printf("%03d global %d local %d edge %d dXYZ %e dT %e\n",
		 gridPartId(grid), global, local, edgeId,
		 dist, dT);
      }
    }
  }

}

void gridminar_( double *aspectratio )
{
  *aspectratio = gridMinAR( grid );
#ifdef PARALLEL_VERBOSE 
  printf( " %6d Minimum Aspect Ratio %g\n", gridPartId(grid), *aspectratio );
  fflush(stdout);
#endif
}

void gridwritetecplotsurfacezone_( void )
{
  char filename[256], comment[256];
  int cell, nodes[4];

  sprintf(filename, "grid%03d.t", gridPartId(grid)+1 );
  gridWriteTecplotSurfaceGeom(grid,filename);

  for (cell=0;cell<gridMaxCell(grid);cell++)
    if (grid==gridCell(grid, cell, nodes)) {
      if ( -0.5 > gridAR(grid,nodes) ) {
	sprintf(comment,
		"cell cost of %f detected in gridwritetecplotsurfacezone_",
		gridAR(grid,nodes));
	gridWriteTecplotComment(grid, comment);
	sprintf(comment, "proc%4d cell l%12d g%12d",
		gridPartId(grid),cell,gridCellGlobal(grid,cell));
	gridWriteTecplotComment(grid, comment);
	sprintf(comment, "proc%4d cell l%12d local  nodes%10d%10d%10d%10d",
	       gridPartId(grid),cell,
	       nodes[0],nodes[1],nodes[2],nodes[3]);
	gridWriteTecplotComment(grid, comment);
	sprintf(comment, "proc%4d cell l%12d global nodes%10d%10d%10d%10d",
	       gridPartId(grid),cell,
	       gridNodeGlobal(grid,nodes[0]),gridNodeGlobal(grid,nodes[1]),
	       gridNodeGlobal(grid,nodes[2]),gridNodeGlobal(grid,nodes[3]));
	gridWriteTecplotComment(grid, comment);
	sprintf(comment, "proc%4d cell l%12d global parts%10d%10d%10d%10d",
	       gridPartId(grid),cell,
	       gridNodePart(grid,nodes[0]),gridNodePart(grid,nodes[1]),
	       gridNodePart(grid,nodes[2]),gridNodePart(grid,nodes[3]));
	gridWriteTecplotComment(grid, comment);
	gridWriteTecplotCellGeom(grid,nodes,filename);
      }
    }

#ifdef PARALLEL_VERBOSE 
  printf( " %6d tecplot dump with negative cells                         \n",
	  gridPartId(grid) );
  fflush(stdout);
#endif
}

void gridexportfast_( void )
{
  char filename[256];
  sprintf(filename, "grid%03d.fgrid", gridPartId(grid)+1 );
  gridExportFAST(grid,filename);
}

void gridparallelswap_( int *processor, double *ARlimit )
{
  GridBool swap_the_safe_old_way;
#ifdef PARALLEL_VERBOSE 
  printf(" %6d swap  processor %2d      initial AR%14.10f",
	 gridPartId(grid),*processor,gridMinAR(grid));
#endif
  swap_the_safe_old_way = TRUE;
  if (swap_the_safe_old_way) {
    if (*processor == -1) {
      gridParallelSwap(grid,NULL,*ARlimit);
    } else {
      gridParallelSwap(grid,queue,*ARlimit);
    } 
  }else{
    if (*processor == -1) {
      int plan_size_guess, plan_chunk_size;
      int cell, nodes[4];
      double ar;
      gridParallelSwap(grid,NULL,*ARlimit);
      plan_size_guess = gridNCell(grid)/10;
      plan_chunk_size = 5000;
      plan = planCreate(plan_size_guess, plan_chunk_size);
      for (cell=0;cell<gridMaxCell(grid);cell++){
	if (grid == gridCell( grid, cell, nodes) ) {
	  if ( gridCellHasGhostNode(grid,nodes)  ||
	       gridNodeNearGhost(grid, nodes[0]) ||
	       gridNodeNearGhost(grid, nodes[1]) ||
	       gridNodeNearGhost(grid, nodes[2]) ||
	       gridNodeNearGhost(grid, nodes[3]) ) {
	    ar = gridAR(grid, nodes);
	    if ( ar < *ARlimit ) {
	      planAddItemWithPriority(plan,cell,ar);
	    }
	  }
	} 
      }
      planDeriveRankingsFromPriorities(plan);
    } else {
      int ranking;
      int cell, nodes[4];
      for (ranking = 0 ; ranking < planSize(plan) ; ranking++) {
	cell = planItemWithThisRanking(plan, ranking);
	if ( grid==gridCell( grid, cell, nodes) ) {
	  if ( grid == gridRemoveTwoFaceCell(grid, queue, cell) ) continue;
	  if ( grid == gridParallelEdgeSwap(grid, queue, nodes[0], nodes[1] ) )
	    continue;
	  if ( grid == gridParallelEdgeSwap(grid, queue, nodes[0], nodes[2] ) )
	    continue;
	  if ( grid == gridParallelEdgeSwap(grid, queue, nodes[0], nodes[3] ) )
	    continue;
	  if ( grid == gridParallelEdgeSwap(grid, queue, nodes[1], nodes[2] ) )
	    continue;
	  if ( grid == gridParallelEdgeSwap(grid, queue, nodes[1], nodes[3] ) )
	    continue;
	  if ( grid == gridParallelEdgeSwap(grid, queue, nodes[2], nodes[3] ) )
	    continue;
	}
      }
      planFree(plan); plan = NULL;
    } 
  }
}

void gridparallelsmooth_( int *processor,
			  double *optimizationLimit, double *laplacianLimit,
                          int *geometryAllowed )
{
  GridBool localOnly, smoothOnSurface;
  localOnly = (-1 == (*processor));
  smoothOnSurface = (0 != (*geometryAllowed));
  gridParallelSmooth(grid, localOnly, *optimizationLimit, *laplacianLimit,
                     smoothOnSurface);
#ifdef PARALLEL_VERBOSE 
  if (localOnly) {
    printf( " %6d smooth volume and face interior  %s    AR%14.10f\n",
	    gridPartId(grid),"local only        ",gridMinAR(grid) );
  } else {
    printf( " %6d smooth volume and face interior  %s    AR%14.10f\n",
	    gridPartId(grid),"near ghost only   ",gridMinAR(grid) );
  }
  fflush(stdout);
#endif
}

void gridparallelrelaxneg_( int *processor, int *geometryAllowed )
{
  GridBool localOnly, smoothOnSurface;
  localOnly = (-1 == (*processor));
  smoothOnSurface = (0 != (*geometryAllowed));
  gridParallelRelaxNegativeCells(grid, localOnly, smoothOnSurface);
#ifdef PARALLEL_VERBOSE 
  if (localOnly) {
    printf( " %6d relaxN volume and face interior  %s    AR%14.10f\n",
	    gridPartId(grid),"local only        ",gridMinVolume(grid) );
  } else {
    printf( " %6d relaxN volume and face interior  %s    AR%14.10f\n",
	    gridPartId(grid),"near ghost only   ",gridMinVolume(grid) );
  }
  fflush(stdout);
#endif
}

void gridparallelrelaxsurf_( int *processor )
{
  GridBool localOnly;
  localOnly = (-1 == (*processor));
  gridParallelRelaxNegativeFaceAreaUV(grid, localOnly);
}

void gridparalleladapt_( int *processor, 
			 double *minLength, double *maxLength )
{
#ifdef PARALLEL_VERBOSE 
  printf(" %6d adapt processor %2d ",gridPartId(grid),*processor);
#endif
  if (*processor == -1) {
    gridParallelAdapt(grid,NULL,*minLength, *maxLength);
  } else {
    gridParallelAdapt(grid,queue,*minLength, *maxLength);
  } 
}

void queuedumpsize_( int *nInt, int *nDouble )
{
  queueDumpSize(queue, nInt, nDouble);
}

void queuedump_( int *nInt, int *nDouble, int *ints, double *doubles )
{
  queueDump(queue, ints, doubles);
}

void gridapplyqueue_( int *nInt, int *nDouble, int *ints, double *doubles )
{
  queueLoad(queue, ints, doubles);
  gridApplyQueue(grid,queue);
  queueReset(queue);
}

void gridsize_( int *nnodeg, int *ncellg )
{
  *nnodeg = gridGlobalNNode(grid);
  *ncellg = gridGlobalNCell(grid);
}

void gridglobalshift_( int *oldnnodeg, int *newnnodeg, int *nodeoffset,
		       int *oldncellg, int *newncellg, int *celloffset )
{
  gridGlobalShiftNode( grid, *oldnnodeg, *newnnodeg, *nodeoffset);
  gridGlobalShiftCell( grid, *oldncellg, *newncellg, *celloffset);
}

void gridrenumberglobalnodes_( int *nnode, int *new2old )
{
  int i;
  for (i=0;i<*nnode;i++) new2old[i]--;
  gridRenumberGlobalNodes( grid, *nnode, new2old );
  for (i=0;i<*nnode;i++) new2old[i]++;
}

void gridnunusednodeglobal_( int *nunused )
{
  *nunused = gridNUnusedNodeGlobal( grid );
}

void gridgetunusednodeglobal_( int *nunused, int *unused )
{
  gridGetUnusedNodeGlobal( grid, unused );
}

void gridjoinunusednodeglobal_( int *nunused, int *unused )
{
  int i;
  for (i=0;i<(*nunused);i++) gridJoinUnusedNodeGlobal( grid, unused[i] );
}

void grideliminateunusednodeglobal_(  )
{
  gridEliminateUnusedNodeGlobal( grid );
}

void gridnunusedcellglobal_( int *nunused )
{
  *nunused = gridNUnusedCellGlobal( grid );
}

void gridgetunusedcellglobal_( int *nunused, int *unused )
{
  gridGetUnusedCellGlobal( grid, unused );
}

void gridjoinunusedcellglobal_( int *nunused, int *unused )
{
  int i;
  for (i=0;i<(*nunused);i++) gridJoinUnusedCellGlobal( grid, unused[i] );
}

void grideliminateunusedcellglobal_(  )
{
  gridEliminateUnusedCellGlobal( grid );
}

void gridsortfun3d_( int *nnodes0, int *nnodes01, int *nnodesg, 
		    int *ncell, int *ncellg )
{
  gridSortNodeFUN3D( grid, nnodes0 );
  *nnodes01 = gridNNode(grid);
  *nnodesg = gridGlobalNNode(grid);
  *ncell = gridNCell(grid);
  *ncellg = gridGlobalNCell(grid);
}

void gridgetnodes_( int *nnode, int *l2g, double *x, double *y, double *z)
{
  int node;
  double xyz[3];
  for (node=0;node<gridNNode(grid);node++) {
    l2g[node] = gridNodeGlobal(grid,node)+1;
    gridNodeXYZ(grid,node,xyz);
    x[node] = xyz[0];
    y[node] = xyz[1];
    z[node] = xyz[2];
  }
}

void gridgetcell_( int *cell, int *nodes, int *global )
{
  gridCell(grid,(*cell)-1,nodes);
  nodes[0]++;
  nodes[1]++;
  nodes[2]++;
  nodes[3]++;
  *global = gridCellGlobal(grid,(*cell)-1)+1;
}

void gridgetbcsize_( int *ibound, int *nface )
{
  int face, nodes[3], id;
  
  *nface = 0;
  for (face=0;face<gridMaxFace(grid);face++) {
    if ( grid == gridFace(grid,face,nodes,&id) ) {
      if ( *ibound == id ) (*nface)++;
    }
  }
}

void gridgetbc_( int *ibound, int *nface, int *ndim, int *f2n )
{
  int face, n, nodes[3], id;
  
  n = 0;
  for (face=0;face<gridMaxFace(grid);face++) {
    if ( grid == gridFace(grid,face,nodes,&id) ) {
      if ( *ibound == id ) {
	f2n[n+(*nface)*0] = nodes[0]+1;
	f2n[n+(*nface)*1] = nodes[1]+1;
	f2n[n+(*nface)*2] = nodes[2]+1;
	n++;
      }
    }
  }
}

void gridsetnaux_( int *naux )
{
  gridSetNAux(grid, *naux);
  if (NULL != queue) queueFree( queue );
  queue = queueCreate( 9 + gridNAux(grid) ); /* 3:xyz + 6:m + naux */
}

void gridsetauxvector_( int *nnode, int *offset, double *x )
{
  int node;
  for (node=0;node<(*nnode);node++) {
    gridSetAux(grid,node,(*offset),x[node]);
  }
}

void gridsetauxmatrix_( int *ndim, int *nnode, int *offset, double *x )
{
  int node, dim;
  for (node=0;node<(*nnode);node++) {
    for (dim=0;dim<(*ndim);dim++){
      gridSetAux(grid,node,(*offset)+dim,x[dim+(*ndim)*node]);
    }
  }
}

void gridsetauxmatrix3_( int *ndim, int *nnode, int *offset, double *x )
{
  int node, dim;
  for (node=0;node<(*nnode);node++) {
    for (dim=0;dim<(*ndim);dim++){
      gridSetAux(grid,node,(*offset)+dim,x[dim+(*ndim)*node]);
    }
  }
}

void gridgetauxvector_( int *nnode, int *offset, double *x )
{
  int node;
  for (node=0;node<(*nnode);node++) {
    x[node] = gridAux(grid,node,(*offset));
  }
}

void gridgetauxmatrix_( int *ndim, int *nnode, int *offset, double *x )
{
  int node, dim;
  for (node=0;node<(*nnode);node++) {
    for (dim=0;dim<(*ndim);dim++){
      x[dim+(*ndim)*node] = gridAux(grid,node,(*offset)+dim);
    }
  }
}

void gridgetauxmatrix3_( int *ndim, int *nnode, int *offset, double *x )
{
  int node, dim;
  for (node=0;node<(*nnode);node++) {
    for (dim=0;dim<(*ndim);dim++){
      x[dim+(*ndim)*node] = gridAux(grid,node,(*offset)+dim);
    }
  }
}

void gridghostcount_( int *nproc, int *count )
{
  int node, part, faces, edges;
  for(node=0;node<(*nproc);node++) count[node] = 0;
  for(node=0;node<gridMaxNode(grid);node++) {
    if (gridNodeGhost(grid,node)) { 
      part = gridNodePart(grid,node);
      if (part < 0 || part >= (*nproc)) 
	printf("%s: %d: gridNodePart error, %d part, %d npart\n",
	       __FILE__, __LINE__, part, (*nproc));
      count[part]++;
      faces = gridNodeFaceIdDegree(grid,node);
      if (faces>0) count[part] += (faces+1);
      edges = gridNodeEdgeIdDegree(grid,node);
      if (edges>0) count[part] += (edges+1);
      if (faces==0 && edges>0) 
	printf("%s: %d: gridghostcount error, %d faces, %d edges\n",
	       __FILE__, __LINE__, faces, edges);
    }
  }
}

void gridloadghostnodes_( int *nproc, int *clientindex,
			  int *clientsize, int *localnode, int *globalnode )
{
  int node, part;
  int *count;
  int face, faceids, faceid[MAXFACEIDDEG];
  int edge, edgeids, edgeid[MAXEDGEIDDEG];

  count = malloc( (*nproc) * sizeof(int) );

  for(node=0;node<(*nproc);node++) count[node] = 0;
  for(node=0;node<gridMaxNode(grid);node++) {
    if (gridNodeGhost(grid,node)) {
      part = gridNodePart(grid,node);
      localnode[ count[part]+clientindex[part]-1] = node+1;
      globalnode[count[part]+clientindex[part]-1] = gridNodeGlobal(grid,node)+1;
      count[part]++;
      gridNodeFaceId(grid, node, MAXFACEIDDEG, &faceids, faceid );
      if (faceids>0) {
	localnode[ count[part]+clientindex[part]-1] = -faceids;
	globalnode[count[part]+clientindex[part]-1] = -faceids;
	count[part]++;
	for (face=0;face<faceids;face++) {
	  localnode[ count[part]+clientindex[part]-1] = -faceid[face];
	  globalnode[count[part]+clientindex[part]-1] = -faceid[face];
	  count[part]++;
	}
      }
      gridNodeEdgeId(grid, node, MAXEDGEIDDEG, &edgeids, edgeid );
      if (edgeids>0) {
	localnode[ count[part]+clientindex[part]-1] = -edgeids;
	globalnode[count[part]+clientindex[part]-1] = -edgeids;
	count[part]++;
	for (edge=0;edge<edgeids;edge++) {
	  localnode[ count[part]+clientindex[part]-1] = -edgeid[edge];
	  globalnode[count[part]+clientindex[part]-1] = -edgeid[edge];
	  count[part]++;
	}
      }
    }
  }
  free(count);
}

void gridloadlocalnodes_( int *nnode, int *global, int *local )
{
  int node, globalnode, localnode;
  int face, ids, faceId;

  localnode=0;
  node=0;
  while (node<(*nnode)) {
    if (global[node] > 0) {
      globalnode = global[node]-1;
      localnode = gridGlobal2Local(grid, globalnode);
      if (!gridValidNode(grid,localnode)) 
	printf("%d: ERROR: %s: %d: invalid node local %d global %d.\n",
	       gridPartId(grid),__FILE__, __LINE__, localnode, globalnode);
      local[node] = 1+localnode;
      node++;
    } else {
      local[node] = global[node];
      node++;
    } 
  }
}

void gridloadglobalnodedata_( int *ndim, int *nnode, int *nodes, double *data )
{
  int node, localnode;
  int face, faceids, faceId;
  int edge, edgeids, edgeId;
  double t, uv[2], xyz[3];

  localnode=0;
  node=0;
  while (node<(*nnode)) {
    if (nodes[node] > 0) {
      localnode = gridGlobal2Local(grid, nodes[node]-1);
      if (grid != gridNodeXYZ(grid,localnode,xyz)) 
	printf("%d: ERROR: %s: %d: get xyz from invalid node local %d global %d.\n",
	       gridPartId(grid),__FILE__, __LINE__, localnode, nodes[node]-1);
      data[0+(*ndim)*node] = xyz[0];
      data[1+(*ndim)*node] = xyz[1];
      data[2+(*ndim)*node] = xyz[2];
      node++;
    } else {
      faceids = -nodes[node];
      node++;
      for(face=0;face<faceids;face++) {
	faceId = -nodes[node];
	gridNodeUV(grid,localnode,faceId,uv);
	data[0+(*ndim)*node] = uv[0];
	data[1+(*ndim)*node] = uv[1];
	node++;
      }
      if (node<(*nnode) && nodes[node] < 0) {
	edgeids = -nodes[node];
	node++;
	for(edge=0;edge<edgeids;edge++) {
	  edgeId = -nodes[node];
	  gridNodeT(grid,localnode,edgeId,&t);
	  data[0+(*ndim)*node] = t;
	  node++;
	}
      }
    } 
  }
}

void gridsetlocalnodedata_( int *ndim, int *nnode, int *nodes, double *data )
{
  int node, localnode;
  int face, faceids, faceId;
  int edge, edgeids, edgeId;

  localnode=0;
  node=0;
  while (node<(*nnode)) {
    if (nodes[node] > 0) {
      localnode = nodes[node]-1;
      if (grid != gridSetNodeXYZ(grid,localnode,&data[(*ndim)*node]) )
	printf("ERROR: %s: %d: set invalid node %d .\n",
	       __FILE__, __LINE__, nodes[node]-1);
      node++;
    } else {
      faceids = -nodes[node];
      node++;
      for(face=0;face<faceids;face++) {
	faceId = -nodes[node];
	gridSetNodeUV(grid,localnode,faceId,
		      data[0+(*ndim)*node],data[1+(*ndim)*node]);
	node++;
      }
      if (node<(*nnode) && nodes[node] < 0) {
	edgeids = -nodes[node];
	node++;
	for(edge=0;edge<edgeids;edge++) {
	  edgeId = nodes[node];
	  gridSetNodeT(grid,localnode,edgeId,data[0+(*ndim)*node]);
	  node++;
	}
      }
    } 
  }

#ifdef PARALLEL_VERBOSE 
  printf( " %6d update xfer                      %s    AR%14.10f\n",
	  gridPartId(grid),"                  ",gridMinAR(grid) );
  fflush(stdout);
#endif
}
void gridcopyabouty0_( int *symmetryFaceId, int *mirrorAux )
{
  gridCopyAboutY0(grid, *symmetryFaceId, *mirrorAux-1 );
}

void gridmovesetprojectiondisp_( void )
{
  gm = gridmoveCreate( grid );
  gridmoveProjectionDisplacements( gm );
}

void gridmoverelaxstartup_( int *relaxationScheme )
{
  gridmoveRelaxationStartUp(gm, *relaxationScheme);
}

void gridmoverelaxstartstep_( double *position)
{
  gridmoveRelaxationStartStep( gm, *position );
}

void gridmoverelaxsubiter_( double *residual)
{
  gridmoveRelaxationSubIteration( gm, residual );
}

void gridmoverelaxshutdown_( void )
{
  gridmoveRelaxationShutDown(gm);
}

void gridmoveapplydisplacements_( void )
{
  gridmoveApplyDisplacements(gm);
}

void gridmovedataleadingdim_( int *ndim )
{
  gridmoveDataLeadingDimension( gm, ndim );
}

void gridmoveinitializempitest_( void )
{
  gridmoveInitializeMPITest(gm);
}

void gridmovecompletempitest_( void )
{
  gridmoveCompleteMPITest(gm);
}

void gridmoveloadlocalnodedata_( int *ndim, int *nnode, 
				 int *nodes, double *data )
{
  gridmoveLoadFortranNodeData( gm, *nnode, nodes, data);
}

void gridmovesetlocalnodedata_( int *ndim, int *nnode, 
				 int *nodes, double *data )
{
  gridmoveSetFortranNodeData( gm, *nnode, nodes, data);
}

void gridmovefree_( void )
{
  gridmoveFree( gm ); gm = NULL;
}

void gridgeomsize_( int *nGeomNode, int *nGeomEdge, int *nGeomFace )
{
  *nGeomNode = gridNGeomNode(grid);
  *nGeomEdge = gridNGeomEdge(grid);
  *nGeomFace = gridNGeomFace(grid);
}

void gridlocalboundnode_( int *nBoundNode )
{
  int node, nnode;
  nnode = 0;
  for(node=0;node<gridMaxNode(grid);node++) 
    if ( gridGeometryFace(grid,node) && gridNodeLocal(grid,node) ) nnode++;

  *nBoundNode = nnode;
}

void gridgeomedgeendpoints_( int *edgeId, int *endPoints )
{
  endPoints[0] = 1 + gridGeomEdgeStart(grid,*edgeId);
  endPoints[1] = 1 + gridGeomEdgeEnd(grid,*edgeId);
}

void gridmaxedge_( int *maxedge )
{
  *maxedge = gridMaxEdge( grid );
}

void gridedge_( int *edge, int *edgeId,
		int *globalnodes, int *nodeparts,
		double *t, double *xyz)
{
  int localnodes[2];
  if (grid==gridEdge(grid, (*edge)-1, localnodes, edgeId)) {
    globalnodes[0] = 1 + gridNodeGlobal(grid,localnodes[0]);
    globalnodes[1] = 1 + gridNodeGlobal(grid,localnodes[1]);
    nodeparts[0] = gridNodePart(grid,localnodes[0]);
    nodeparts[1] = gridNodePart(grid,localnodes[1]);
    gridNodeT(grid,*edgeId,localnodes[0],&t[0]);
    gridNodeT(grid,*edgeId,localnodes[1],&t[1]);
    gridNodeXYZ(grid,localnodes[0],&xyz[0*3]);
    gridNodeXYZ(grid,localnodes[1],&xyz[1*3]);
  }else{
    *edgeId = EMPTY;
  }
}

void gridupdateedgegrid_(int *edgeId, int *nCurveNode, double *xyz, double *t)
{
  gridUpdateEdgeGrid( grid, *edgeId, *nCurveNode, xyz, t);
}

void gridmaxface_( int *maxface )
{
  *maxface = gridMaxFace( grid );
}

void gridface_( int *face, int *faceId,
		int *globalnodes, int *nodeparts,
		double *uv, double *xyz)
{
  int localnodes[3];
  if (grid==gridFace(grid, (*face)-1, localnodes, faceId)) {
    globalnodes[0] = 1 + gridNodeGlobal(grid,localnodes[0]);
    globalnodes[1] = 1 + gridNodeGlobal(grid,localnodes[1]);
    globalnodes[2] = 1 + gridNodeGlobal(grid,localnodes[2]);
    nodeparts[0] = gridNodePart(grid,localnodes[0]);
    nodeparts[1] = gridNodePart(grid,localnodes[1]);
    nodeparts[2] = gridNodePart(grid,localnodes[2]);
    gridNodeUV(grid,localnodes[0],*faceId,&uv[0*2]);
    gridNodeUV(grid,localnodes[1],*faceId,&uv[1*2]);
    gridNodeUV(grid,localnodes[2],*faceId,&uv[2*2]);
    gridNodeXYZ(grid,localnodes[0],&xyz[0*3]);
    gridNodeXYZ(grid,localnodes[1],&xyz[1*3]);
    gridNodeXYZ(grid,localnodes[2],&xyz[2*3]);
  }else{
    *faceId = EMPTY;
  }
}

void gridfaceedgecount_( int *faceId, int *faceEdgeCount )
{
  *faceEdgeCount = gridFaceEdgeCount( grid, *faceId );
}

void gridfaceedgel2g_( int *faceId, int *faceEdgeCount, int *local2global )
{
  int i;
  gridFaceEdgeLocal2Global( grid, *faceId, *faceEdgeCount, local2global );
  for (i=0;i<*faceEdgeCount;i++) local2global[i]++;
}

void gridupdategeometryface_( int *faceId, int *nnode, double *xyz, double *uv,
			      int *nface, int *f2n )
{
  int i;
  for( i=0 ; i<3*(*nface) ; i++) f2n[i]--;
  gridUpdateGeometryFace( grid, *faceId, *nnode, xyz, uv,
			  *nface, f2n );
  for( i=0 ; i<3*(*nface) ; i++) f2n[i]++; 
}

void gridcreateshellfromfaces_( void )
{
  gridCreateShellFromFaces( grid );
}
