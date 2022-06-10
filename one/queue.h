
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

#ifndef QUEUE_H
#define QUEUE_H

#include "refine_defs.h"
#include <stdio.h>

BEGIN_C_DECLORATION

typedef struct Queue Queue;

struct Queue {
  int nodeSize;
  int transactions;
  int maxTransactions;

  int *removedCells;
  int maxRemovedCells;
  int nRemovedCells;
  int *removedCellNodes;
  int *addedCells;
  int maxAddedCells;
  int nAddedCells;
  int *addedCellNodes;
  double *addedCellXYZs;

  int *removedFaces;
  int maxRemovedFaces;
  int nRemovedFaces;
  int *removedFaceNodes;
  int *addedFaces;
  int maxAddedFaces;
  int nAddedFaces;
  int *addedFaceNodes;
  double *addedFaceUVs;

  int *removedEdges;
  int maxRemovedEdges;
  int nRemovedEdges;
  int *removedEdgeNodes;
  int *addedEdges;
  int maxAddedEdges;
  int nAddedEdges;
  int *addedEdgeNodes;
  double *addedEdgeTs;
};

Queue *queueCreate( int nodeSize );
void queueFree( Queue * );
Queue *queueReset( Queue * );
int queueNodeSize( Queue * );
int queueTransactions( Queue * );
Queue *queueNewTransaction( Queue * );
Queue *queueResetCurrentTransaction( Queue * );

Queue *queueRemoveCell( Queue *, int *nodes, int *nodeParts );
int queueRemovedCells( Queue *, int transaction );
Queue *queueRemovedCellNodes( Queue *, int index, int *nodes );
Queue *queueRemovedCellNodeParts( Queue *, int index, int *nodeParts );
Queue *queueAddCell( Queue *, int *nodes, int *nodeParts,
		     double *xyzs );
int queueAddedCells( Queue *, int transaction );
Queue *queueAddedCellNodes( Queue *, int index, int *nodes );
Queue *queueAddedCellNodeParts( Queue *, int index, int *nodeParts );
Queue *queueAddedCellXYZs( Queue *, int index, double *xyzs );
int queueTotalRemovedCells( Queue * );

Queue *queueRemoveFace( Queue *, int *nodes, int *nodeParts );
int queueRemovedFaces( Queue *, int transaction );
Queue *queueRemovedFaceNodes( Queue *, int index, int *nodes );
Queue *queueRemovedFaceNodeParts( Queue *, int index, int *nodeParts );
Queue *queueAddFace( Queue *, int *nodes, int faceId, int *nodesParts, 
		     double *uvs );
Queue *queueAddFaceScalar( Queue *, 
			   int n0, int p0, double u0, double v0,
			   int n1, int p1, double u1, double v1,
			   int n2, int p2, double u2, double v2, int faceId);
int queueAddedFaces( Queue *, int transaction );
Queue *queueAddedFaceNodes( Queue *, int index, int *nodes );
Queue *queueAddedFaceId( Queue *, int index, int *faceId );
Queue *queueAddedFaceNodeParts( Queue *, int index, int *nodes );
Queue *queueAddedFaceUVs( Queue *, int index, double *uvs );

Queue *queueRemoveEdge( Queue *, int *nodes, int *nodeParts );
int queueRemovedEdges( Queue *, int transaction );
Queue *queueRemovedEdgeNodes( Queue *, int index, int *nodes );
Queue *queueRemovedEdgeNodeParts( Queue *, int index, int *nodeParts );
Queue *queueAddEdge( Queue *, int *nodes, int edgeId, int *nodesParts, 
		     double *ts );
int queueAddedEdges( Queue *, int transaction );
Queue *queueAddedEdgeNodes( Queue *, int index, int *nodes );
Queue *queueAddedEdgeId( Queue *, int index, int *edgeId );
Queue *queueAddedEdgeNodeParts( Queue *, int index, int *nodes );
Queue *queueAddedEdgeTs( Queue *, int index, double *ts );

Queue *queueDumpSize( Queue *, int *nInt, int *nDouble );
Queue *queueDump( Queue *, int *ints, double *doubles );
Queue *queueLoad( Queue *, int *ints, double *doubles );

Queue *queueGlobalShiftNode( Queue *, 
			     int old_nnode_global,
			     int node_offset );

Queue *queueContents(Queue *queue, FILE *file);



END_C_DECLORATION

#endif /* QUEUE_H */
