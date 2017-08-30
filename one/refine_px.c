
/* Michael A. Park
 * Computational Modeling & Simulation Branch
 * NASA Langley Research Center
 * Phone:(757)864-6604
 * Email:m.a.park@larc.nasa.gov
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "grid.h"
#include "gridmetric.h"
#include "gridswap.h"
#include "gridinsert.h"
#include "gridcad.h"

#define PRINT_STATUS {double l0,l1;gridEdgeRatioRange(grid,&l0,&l1);printf("Len %12.5e %12.5e AR %8.6f MR %8.6f Vol %10.6e\n", l0,l1, gridMinThawedAR(grid),gridMinThawedFaceMR(grid), gridMinVolume(grid)); fflush(stdout);}

#define STATUS { \
  PRINT_STATUS; \
}

static void usage( char *executable );
static void usage( char *executable )
{
  printf("Usage: %s -g input.gri -m projectx.metric -o output.gri\n",
	 executable );
  printf(" -g input.{gri|fgrid}\n");
  printf(" -m input.metric\n");
  printf(" -o output.{gri|fgrid}\n");
}

#ifdef PROE_MAIN
int GridEx_Main( int argc, char *argv[] )
#else
int main( int argc, char *argv[] )
#endif
{
  Grid *grid;
  int end_of_string;
  char file_input[256] = "";
  char metric_input[256] = "";
  char file_output[256] = "";

  int i;

  int active_edges, out_of_tolerence_edges;

  printf("refine %s\n",VERSION);

  i = 1;
  while( i < argc ) {
    if( strcmp(argv[i],"-g") == 0 ) {
      i++; sprintf( file_input, "%s", argv[i] );
      printf("-g argument %d: %s\n",i, file_input);
    } else if( strcmp(argv[i],"-m") == 0 ) {
      i++; sprintf( metric_input, "%s", argv[i] );
      printf("-m argument %d: %s\n",i, metric_input);
    } else if( strcmp(argv[i],"-o") == 0 ) {
      i++; sprintf( file_output, "%s", argv[i] );
      printf("-o argument %d: %s\n",i, file_output);
    } else if( strcmp(argv[i],"-h") == 0 ) {
      usage( argv[0] );
      return(0);
    } else {
      fprintf(stderr,"Argument \"%s %s\" Ignored\n",argv[i],argv[i+1]);
      i++;
    }
    i++;
  }
  
  if ( (strcmp(file_input,"")==0) ||
       (strcmp(file_output,"")==0) ) {
    printf("no input or output file names specified.\n");
    usage( argv[0] );
    printf("Done.\n");  
    return 1;
  }

  end_of_string = strlen(file_input);
  grid = NULL;
  if( strcmp(&file_input[end_of_string-4],".gri") == 0 ) {
    printf("gri input file %s\n", file_input);
    grid = gridImportGRI( file_input );
  } else if( strcmp(&file_input[end_of_string-6],".fgrid") == 0 ) {
    printf("fast input file %s\n", file_input);
    grid = gridImportFAST( file_input );
  } else {
    printf("input file name extension unknown %s\n", file_input);
  }

  if ( NULL == grid ) 
    {
      printf("read of %s failed. stop\n",file_input);
      return(1);
    }

  printf("grid size: %d nodes %d faces %d cells.\n",
	 gridNNode(grid),gridNFace(grid),gridNCell(grid));

  gridRobustProject(grid);

  gridSetCostConstraint(grid,
			gridCOST_CNST_VOLUME );

  STATUS;

  if ( !(strcmp(metric_input,"")==0) )
    {
      printf("reading metric file %s\n",metric_input);
      gridImportAdapt(grid,metric_input);
      STATUS;
    } else {
      printf("Spacing reset.\n");
      gridResetSpacing(grid);
    }

  STATUS;

  gridWriteTecplotSurfaceGeom(grid,NULL);
  {
    int iteration;
    int iterations = 10;
    double ratioSplit, ratioCollapse;

    ratioCollapse = 0.3;
    ratioSplit    = 1.8;      

    STATUS;

    printf("edge swapping grid...\n");gridSwap(grid,0.9);
    STATUS;

    gridAdapt(grid, ratioCollapse, ratioSplit);
    STATUS;
    
    for ( iteration=0; (iteration<iterations) ; iteration++){
      
      for (i=0;i<1;i++){
	printf("edge swapping grid...\n");gridSwap(grid,0.9);
	STATUS;
	printf("node smoothin grid...\n");gridSmooth(grid,0.9,0.5);
	STATUS;
      }

      gridEdgeRatioTolerence(grid, ratioSplit, ratioCollapse,
			     &active_edges, &out_of_tolerence_edges );

      printf("edges %d of %d (%6.2f%%) out of tol\n",
	     out_of_tolerence_edges, active_edges, 
	     100.0*(double)out_of_tolerence_edges/(double)active_edges);

      if ( 0.001 > (double)out_of_tolerence_edges/(double)active_edges)
	break;

      gridAdapt(grid, ratioCollapse, ratioSplit);
      STATUS;
    
      gridWriteTecplotSurfaceGeom(grid,NULL);
    }
  

  }

  end_of_string = strlen(file_output);
  if( strcmp(&file_output[end_of_string-4],".gri") == 0 ) {
    printf("gri output file %s\n", file_output);
    gridExportGRI( grid, file_output );
  } else if( strcmp(&file_output[end_of_string-6],".fgrid") == 0 ) {
    printf("fast output file %s\n", file_output);
    gridExportFAST( grid, file_output );
  } else {
    printf("output file name extension unknown %s\n", file_output );
  }

  printf("Done.\n");
  
  return 0;
}

