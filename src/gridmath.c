/* Michael A. Park
 * Computational Modeling & Simulation Branch
 * NASA Langley Research Center
 * Phone:(757)864-6604
 * Email:m.a.park@larc.nasa.gov 
 */
  
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "gridmath.h"

double gridVectorLength(double *v)
{
  return sqrt( gridDotProduct(v, v) );
}

void gridVectorNormalize(double *norm)
{
  double length;
  length = sqrt(gridDotProduct(norm,norm));
  if (length > 0 ) {
    norm[0] /= length;
    norm[1] /= length;
    norm[2] /= length;
  }
}

void gridVectorOrthogonalize(double *norm, double *axle)
{
  double dot;
  dot = sqrt(gridDotProduct(norm,axle));
  if (ABS(dot) < 1e-15) return;
  
  norm[0] -= dot*axle[0];
  norm[1] -= dot*axle[1];
  norm[2] -= dot*axle[2];
  gridVectorNormalize(norm);
}

void gridRotateDirection(double *v0, double *v1, 
			 double *axle, double rotation, double *result)
{
  double n0[3], n1[3], n2[3];
  double alpha, a0, a1;
  double dot0, dot1, skew0, skew1, skew;
  double adjecent;

  gridVectorCopy(n0,v0);   gridVectorNormalize(n0);
  gridVectorCopy(n1,v1);   gridVectorNormalize(n1);
  gridVectorCopy(n2,axle); gridVectorNormalize(n2);

  dot0 = gridDotProduct(n0, n2); skew0 = asin(dot0);
  dot1 = gridDotProduct(n1, n2); skew1 = asin(dot1);

  n0[0] -= dot0*n2[0]; n0[1] -= dot0*n2[1]; n0[2] -= dot0*n2[2];
  n1[0] -= dot0*n2[0]; n1[1] -= dot0*n2[1]; n1[2] -= dot0*n2[2];
  gridVectorNormalize(n0); gridVectorNormalize(n1);

  alpha = acos(gridDotProduct(n0, n1));
  alpha = alpha * rotation;

  a0 = cos(alpha);
  a1 = sin(alpha);

  gridCrossProduct(n2,n0,n1); gridVectorNormalize(n1);

  result[0] = a0*n0[0] + a1*n1[0];
  result[1] = a0*n0[1] + a1*n1[1];
  result[2] = a0*n0[2] + a1*n1[2];

  gridVectorNormalize(result);

  skew = (1.0-rotation) * skew0 + rotation * skew1;


  adjecent = tan(skew);

  result[0] += adjecent * n2[0];
  result[1] += adjecent * n2[1];
  result[2] += adjecent * n2[2];

  gridVectorNormalize(result);

  if (FALSE) {
    printf("\nv0 %f %f %f v1 %f %f %f",v0[0],v0[1],v0[2],v1[0],v1[1],v1[2]);
    printf("\nskew %f %f %f %f",skew0, skew1, rotation, skew);
    printf("\nresult %f %f %f",result[0],result[1],result[2]);
  }
}

void gridTriDiag3x3(double *m, double *d, double *e, 
		    double *q0, double *q1, double *q2)
{
  double l,u,v,s;
  d[0] = m[0];
  e[2] = 0.0;
  q0[0] = 1.0;
  q0[1] = 0.0;
  q0[2] = 0.0;
  q1[0] = 0.0;
  q2[0] = 0.0;
  
  if ( ABS(m[2]) > 1.0e-12 ) {
    l = sqrt( m[1]*m[1] + m[2]*m[2] );
    u = m[1] / l;
    v = m[2] / l;
    s = 2.0 * u * m[4] + v * ( m[5] - m[3] );
    d[1] = m[3] + v*s;
    d[2] = m[5] - v*s;
    e[0] = l;
    e[1] = m[4] - u*s;
    q1[1] = u;
    q2[1] = v;
    q1[2] = v;
    q2[2] = -u;
  } else {
    d[1] = m[3];
    d[2] = m[5];
    e[0] = m[1];
    e[1] = m[4];
    q1[1] = 1.0;
    q2[1] = 0.0;
    q1[2] = 0.0;
    q2[2] = 1.0;
  }
}

GridBool gridEigTriDiag3x3(double *d, double *e,
		       double *q0, double *q1, double *q2)
{
  int ierr;
  double f,tst1,tst2;
  int i,j,l,m;
  int l1, l2;
  double h, g, p, r, dl1;
  double c, c2, c3, el1, s;
  int mml, ii, k;
  double s2;

  c3 = 0; s2 = 0; /* quiet -Wall used without set compiler warning */

  ierr = 0;

  f = 0.0;
  tst1 = 0.0;
  e[2] = 0.0;

  for( l = 0; l < 3; l++){ /* row_loop  */
    j = 0;
    h = ABS(d[l]) + ABS(e[l]);
    if (tst1 < h) tst1 = h;
    /* look for small sub-diagonal element */
    for( m = l; m<3;m++) { /*test_for_zero_e */
      tst2 = tst1 + ABS(e[m]);
      if (ABS(tst2 - tst1) < 1.0e-14 ) break;
      /* e[2] is always zero, so there is no exit through the bottom of loop*/
    }
    if (m != l) { /* l_not_equal_m */
      do {
	j = j + 1;
	/* set error -- no convergence to an eigenvalue after 30 iterations */
	if (j > 30 ) {
	  ierr = l;
	  printf( "%s: %d: EigTriDiag3x3: ierr = %d\n",__FILE__,__LINE__,ierr);
	  return FALSE;
        }
	/* form shift */
	l1 = l + 1;
	l2 = l1 + 1;
	g = d[l];
	p = (d[l1] - g) / (2.0 * e[l]);
	r = sqrt(p*p+1.0);
       	d[l] = e[l] / (p + gridSign(r,p));
	d[l1] = e[l] * (p + gridSign(r,p));
	dl1 = d[l1];
	h = g - d[l];
	if (l2<=2) {
	  for (i = l2; i<3; i++) {
	    d[i] = d[i] - h;
	  }
	}
	f = f + h;
	/* ql transformation */
	p = d[m];
	c = 1.0;
	c2 = c;
	el1 = e[l1];
	s = 0.0;
	mml = m - l;
	for (ii = 0;ii< mml; ii++ ) {
	  c3 = c2;
	  c2 = c;
	  s2 = s;
	  i = m - ii - 1;
	  g = c * e[i];
	  h = c * p;
	  r = sqrt(p*p+e[i]*e[i]);
	  e[i+1] = s * r;
	  s = e[i] / r;
	  c = p / r;
	  p = c * d[i] - s * g;
	  d[i+1] = h + s * (c * g + s * d[i]);
	  /* form vector */
	  for (k = 0;k< 3;k++){
	    if (i==0) {
	      h = q1[k];
	      q1[k] = s * q0[k] + c * h;
	      q0[k] = c * q0[k] - s * h;
	    } else{
	      h = q2[k];
	      q2[k] = s * q1[k] + c * h;
	      q1[k] = c * q1[k] - s * h;
	    }
	  }
	}
	p = -s * s2 * c3 * el1 * e[l] / dl1;
	e[l] = s * p;
	d[l] = c * p;
	tst2 = tst1 + ABS(e[l]);
	if (ABS(tst2 - tst1) < 1.0e-14) break;
      } while (TRUE); /* iterate */
    } /* l_not_equal_m */
    d[l] = d[l] + f;
  } /* row_loop */

  gridEigSort3x3(d,q0,q1,q2);

 return TRUE;
}
void gridEigSort3x3( double *eigenValues, double *v0, double *v1, double *v2 )
{
  double t, vt[3];

  if ( eigenValues[1] > eigenValues[0] ) {
    t = eigenValues[0];
    gridVectorCopy(vt,v0);
    eigenValues[0] = eigenValues[1];
    gridVectorCopy(v0,v1);
    eigenValues[1] = t;
    gridVectorMirror(v1,vt);
  }

  if ( eigenValues[2] > eigenValues[0] ) {
    t = eigenValues[0];
    gridVectorCopy(vt,v0);
    eigenValues[0] = eigenValues[2];
    gridVectorCopy(v0,v2);
    eigenValues[2] = t;
    gridVectorMirror(v2,vt);
  }

  if ( eigenValues[2] > eigenValues[1] ) {
    t = eigenValues[1];
    gridVectorCopy(vt,v1);
    eigenValues[1] = eigenValues[2];
    gridVectorCopy(v1,v2);
    eigenValues[2] = t;
    gridVectorMirror(v2,vt);
  }

}

void gridEigOrtho3x3( double *v0, double *v1, double *v2 )
{
  double v0dotv1;

  gridVectorNormalize(v0);

  v0dotv1 =  gridDotProduct(v0,v1);
  v1[0] -= v0[0]*v0dotv1;
  v1[1] -= v0[1]*v0dotv1;
  v1[2] -= v0[2]*v0dotv1;

  gridVectorNormalize(v1);

  gridCrossProduct(v0,v1,v2);

  gridVectorNormalize(v2);

}

