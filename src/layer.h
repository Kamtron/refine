
/* Michael A. Park
 * Computational Modeling & Simulation Branch
 * NASA Langley Research Center
 * Phone:(757)864-6604
 * Email:m.a.park@larc.nasa.gov 
 */
  
/* $Id$ */

#ifndef LAYER_H
#define LAYER_H

#include "master_header.h"
#include "grid.h"

BEGIN_C_DECLORATION

typedef struct Layer Layer;

Layer *layerCreate(Grid *);
void layerFree(Layer *);
int layerNFront(Layer *);
int layerNNormal(Layer *);
int layerMaxNode(Layer *);
Layer *layerMakeFront(Layer *, int nbc, int *bc);
Layer *layerFront(Layer *, int front, int *nodes);
Layer *layerFrontDirection(Layer *, int front, double *direction);
Layer *layerMakeNormal(Layer *);
Layer *layerFrontNormals(Layer *, int front, int *normals);
int layerNormalRoot(Layer *, int normal );
int layerNormalDeg(Layer *, int normal );
Layer *layerNormalFronts(Layer *, int normal, int maxfront, int *fronts);
Layer *layerNormalDirection(Layer *, int normal, double *direction);
Layer *layerConstrainNormal(Layer *, int bc );
int layerConstrained(Layer *, int normal );

END_C_DECLORATION

#endif /* LAYER_H */
