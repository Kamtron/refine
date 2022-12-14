
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
#include <limits.h>

#ifdef __APPLE__       /* Not needed on Mac OS X */
#include <float.h>
#else
#include <malloc.h>
#include <values.h>
#endif

#include "near.h"

Near* nearCreate( int index, double x, double y, double z, double radius )
{
  Near *near;

  near = malloc( sizeof(Near) );

  return nearInit( near, index, x, y, z, radius );
}

Near* nearInit( Near *near, 
		int index, double x, double y, double z, double radius )
{
  near->index = index;
  near->x = x;
  near->y = y;
  near->z = z;
  near->radius = radius;

  near->leftChild = NULL;
  near->rightChild = NULL;

  near->leftRadius = 0;
  near->rightRadius = 0;

  return near;
}

void nearFree( Near *near )
{
  free( near );
}

int nearIndex( Near *near )
{
  return (NULL==near?EMPTY:near->index);
}

int nearLeftIndex( Near *near )
{
  return nearIndex(near->leftChild);
}

int nearRightIndex( Near *near )
{
  return nearIndex(near->rightChild);
}

Near *nearInsert( Near *near, Near *child )
{
  double childRadius;
  double leftDistance, rightDistance;

  childRadius = nearDistance(near,child) + child->radius;

  if (NULL==near->leftChild){ 
    near->leftChild = child;
    near->leftRadius = childRadius;
    return near;
  }
  if (NULL==near->rightChild){ 
    near->rightChild = child;
    near->rightRadius = childRadius;
    return near;
  }

  leftDistance  = nearDistance(near->leftChild,child);
  rightDistance = nearDistance(near->rightChild,child);

  if ( leftDistance < rightDistance) {
    if ( near->leftChild == nearInsert(near->leftChild,child) ) {
      near->leftRadius = MAX(childRadius,near->leftRadius);
      return near;
    }
  }else{
    if ( near->rightChild == nearInsert(near->rightChild,child) ) {
      near->rightRadius = MAX(childRadius,near->rightRadius);
      return near;
    }
  }
  return NULL;
}

double nearClearance( Near *near, Near *other)
{
  double clearance;

  clearance = nearDistance(near,other) - near->radius - other->radius;

  return clearance;
}

Near *nearVisualize( Near *near )
{
  if (NULL == near) return NULL;

  printf("index %d (%f,%f,%f) radius %f\n",
	 nearIndex(near), near->x, near->y, near->z, near->radius);
  printf("  left index %d (%f) right index %d (%f)\n",
	 nearLeftIndex(near),nearLeftRadius(near),
	 nearRightIndex(near),nearRightRadius(near));
  nearVisualize(near->leftChild);
  nearVisualize(near->rightChild);

  return near;
}

int nearCollisions(Near *near, Near *target)
{
  int collisions = 0;

  if (NULL==near || NULL == target) return 0;

  collisions += nearCollisions(near->rightChild,target);
  collisions += nearCollisions(near->leftChild,target);

  if (nearClearance(near,target) <= 0) collisions++;

  return collisions;
}

Near *nearTouched(Near *near, Near *target, int *found, int maxfound, int *list)
{
  double distance, safeZone;

  if (NULL==near || NULL==target) return NULL;

  distance = nearDistance( near, target);
  safeZone = distance - target->radius;

  if (safeZone <= nearLeftRadius(near) ) 
    nearTouched(near->leftChild, target, found, maxfound, list);
  if (safeZone <= nearRightRadius(near) ) 
    nearTouched(near->rightChild, target, found, maxfound, list);

  if ( near->radius >= safeZone ) {
    if (*found >= maxfound) return NULL;
    list[*found] = near->index;
    (*found)++;
  }

  return near;
}


int nearNearestIndex(Near *root, Near *key)
{
  int nearestIndex;
  double smallestDistance;

  if (NULL == nearNearestIndexAndDistance(root, key,
					  &nearestIndex, &smallestDistance))
    return EMPTY;

  return nearestIndex;
}

Near *private_NearestIndexAndDistance(Near *root, Near *key,
				      int *nearestIndex, 
				      double *smallestDistance)
{
  double myDistance;

  if (NULL==root || NULL==key) return NULL;

  myDistance = nearDistance(root, key);

  if (myDistance < *smallestDistance) {
    *smallestDistance = myDistance;
    *nearestIndex = nearIndex(root);
  }

  if (root->leftChild == NULL) return root;

  if (myDistance-nearLeftRadius(root) <= *smallestDistance )
    private_NearestIndexAndDistance(root->leftChild, key,
				nearestIndex, smallestDistance);

  if (root->rightChild == NULL) return root;

  if (myDistance-nearRightRadius(root) <= *smallestDistance )
    private_NearestIndexAndDistance(root->rightChild, key,
				nearestIndex, smallestDistance);

  return root;
}

Near *nearNearestIndexAndDistance(Near *root, Near *key,
				  int *index, double *distance)
{
  *index = EMPTY;
  *distance = DBL_MAX;
  return private_NearestIndexAndDistance(root, key, index, distance);
}

