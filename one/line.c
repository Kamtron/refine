
/* Copyright 2014 United States Government as represented by the
 * Administrator of the National Aeronautics and Space
 * Administration. No copyright is claimed in the United States under
 * Title 17, U.S. Code.  All Other Rights Reserved.
 *
 * The refine platform is licensed under the Apache License, Version
 * 2.0 (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include "line.h"

#define LINESTRIDE (15);

Line* lineCreate( void )
{
  Line *line;

  line = malloc( sizeof(Line) );

  lineInit(line);

  return line;
}

Line *lineInit( Line *line )
{
  line->length = 0;
  line->maxlength = 0;
  line->nodes = NULL;

  return line;
}

void lineFree( Line *line )
{
  if ( NULL != line->nodes ) free( line->nodes );
  free( line );
}

int lineLength( Line *line )
{
  return (NULL==line?EMPTY:line->length);
}

Line *lineAddNode( Line *line, int node )
{
  if (line->length>0 && line->nodes[line->length-1] == node) return NULL;

  if (line->length >= line->maxlength) {
    line->maxlength += LINESTRIDE;
    if (NULL == line->nodes) {
      line->nodes = malloc(line->maxlength * sizeof(int));
    }else{
      line->nodes = realloc(line->nodes,line->maxlength * sizeof(int));
    }
  } 

  line->nodes[line->length] = node;
  line->length++;

  return line;
}

int lineNode( Line *line, int index )
{
  if (NULL==line) return EMPTY;
  if (index < 0 || index >= line->length ) return EMPTY;
  return line->nodes[index];
}

#define LINESSTRIDE (5000);

Lines* linesCreate( void )
{
  Lines *lines;

  lines = malloc( sizeof(Lines) );

  lines->number = 0;
  lines->max = 0;
  lines->line = NULL;

  return lines;
}

void linesFree( Lines *lines )
{
  int i;
  if ( NULL != lines->line) {
    for (i=0;i<lines->max;i++) 
      if ( NULL != lines->line[i].nodes ) free( lines->line[i].nodes );
    free( lines->line ); 
  }
  free( lines );
}

int linesNumber( Lines *lines )
{
  return (NULL==lines?EMPTY:lines->number);
}

Lines *linesAddNode( Lines *lines, int line, int node )
{
  int i, max;
  Line *linePtr;

  if (line < 0) return NULL;

  if (line >= lines->max) {
    max = lines->max;
    lines->max = line+LINESSTRIDE;
    if (NULL == lines->line) {
      lines->line = malloc(lines->max * sizeof(Line));
    }else{
      lines->line = realloc(lines->line,lines->max * sizeof(Line));
    }
    for(i=max;i<lines->max;i++) lineInit(&lines->line[i]);
  } 

  lines->number = MAX(lines->number,line+1);

  linePtr = &lines->line[line];
  return (linePtr==lineAddNode(linePtr,node)?lines:NULL);
}

int linesNode( Lines *lines, int line, int index )
{
  if (line < 0 || line >= linesNumber(lines) ) return EMPTY;
  return lineNode(&lines->line[line],index);
}

Lines *linesRenumber( Lines *lines, int *o2n )
{
  int line, node;

  if ( NULL==lines || NULL==lines->line ) return NULL;

  for (line=0;line<lines->number;line++)
    for (node=0;node<lines->line[line].length;node++)
      lines->line[line].nodes[node] = o2n[lines->line[line].nodes[node]];

  return lines;
}

Lines *linesSave( Lines *lines, char *filename )
{
  FILE *file;
  int line, node;
  if ( NULL==lines ) return NULL;

  file = fopen(filename,"w");
  fprintf(file,"%d\n",linesNumber(lines));
  for (line=0;line<lines->number;line++){
    fprintf(file,"%d\n",lines->line[line].length);
    for (node=0;node<lines->line[line].length;node++)
      fprintf(file,"%d\n",lines->line[line].nodes[node]+1);
  }
  fclose(file);
  return lines;
}

Lines *linesLoad( Lines *lines, char *filename )
{
  FILE *file;
  int line, length, node, index;
  if ( NULL==lines ) return NULL;

  if ( NULL != lines->line) {
    for (line=0;line<lines->max;line++) 
      if ( NULL != lines->line[line].nodes ) free( lines->line[line].nodes );
    free( lines->line ); 
  }

  file = fopen(filename,"r");

  if ( 1 != fscanf(file,"%d\n",&lines->number)) return NULL;
  lines->max = lines->number;
  lines->line = malloc(lines->max * sizeof(Line));
  for (line=0;line<lines->number;line++){
    lineInit(&lines->line[line]);
    if ( 1 != fscanf(file,"%d\n",&length)) return NULL;
    for (node=0;node<length;node++) {
      if ( 1 != fscanf(file,"%d\n",&index)) return NULL;
      linesAddNode(lines,line,index-1);
    }
  }
  fclose(file);
  return lines;
}

