
/* Computes metrics from faces and tets 
 *
 * Michael A. Park
 * Computational Modeling & Simulation Branch
 * NASA Langley Research Center
 * Phone:(757)864-6604
 * Email:m.a.park@larc.nasa.gov 
 */
  
/* $Id$ */

#ifndef GRIDMPI_H

#include "master_header.h"
#include "grid.h"

BEGIN_C_DECLORATION

Grid *gridIdentityGlobal(Grid *g );
Grid *gridSetAllLocal(Grid *g );
Grid *gridSetGhost(Grid *g, int node );

END_C_DECLORATION

#endif /* GRIDMPI_H */
