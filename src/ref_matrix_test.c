
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
#include <string.h>
#include <math.h>

#include "ref_matrix.h"

int main( void )
{

  { /* sqrt ( v^t M v ) */
    /* 
       m = [ 1.0 1.3 0.4 ; 1.3 1.8 0.6 ; 0.4 0.6 0.5 ] 
       v = [1.4;1.5;1.6]
       ratio = sqrt(v'*m*v)
    */ 
    REF_DBL m[6]={ 1.0, 1.3, 0.4, 
                        1.8, 0.6,
                             0.5};
    REF_DBL v[3]={ 1.4, 1.5, 1.6 };
    REF_DBL ratio;
    
    ratio = ref_matrix_sqrt_vt_m_v( m, v );
    RWDS( 4.1740, ratio, 1.0e-4, "ratio");
  }

  { /* macro matches deriv f */
    REF_DBL m[6]={ 1.0, 1.3, 0.4, 
                        1.8, 0.6,
                             0.5};
    REF_DBL v[3]={ 1.4, 1.5, 1.6 };
    REF_DBL f, d[3];
    RSS( ref_matrix_sqrt_vt_m_v_deriv( m, v, &f, d ), "deriv");
    RWDS( ref_matrix_sqrt_vt_m_v( m, v ), f, -1, "ratio");

  }

  { /* finite diff deriv */
    REF_DBL m[6]={ 1.0, 1.3, 0.4, 
                        1.8, 0.6,
                             0.5};
    REF_DBL v[3]={ 1.4, 1.5, 1.6 };

    REF_DBL f, d[3];
    REF_DBL fd[3], x0, step = 1.0e-7, tol = 2.0e-7;
    REF_INT dir;

    RSS( ref_matrix_sqrt_vt_m_v_deriv( m, v, &f, d ), "deriv")
    for ( dir=0;dir<3;dir++) 
      {
	x0 = v[dir];
	v[dir] = x0+step;
	RSS( ref_matrix_sqrt_vt_m_v_deriv( m, v, &(fd[dir]), d ), "fd")
	fd[dir] = (fd[dir]-f)/step;
	v[dir] = x0;
      }
    RSS( ref_matrix_sqrt_vt_m_v_deriv( m, v, &f, d ), "deriv")
    RWDS( fd[0], d[0], tol, "d[0] expected" );
    RWDS( fd[1], d[1], tol, "d[1] expected" );
    RWDS( fd[2], d[2], tol, "d[2] expected" );
  }
  
  { /* v^t M v */
    /* 
       m = [ 1.0 1.3 0.4 ; 1.3 1.8 0.6 ; 0.4 0.6 0.5 ] 
       v = [1.4;1.5;1.6]
       ratio = v'*m*v
    */ 
    REF_DBL m[6]={ 1.0, 1.3, 0.4, 
                        1.8, 0.6,
                             0.5};
    REF_DBL v[3]={ 1.4, 1.5, 1.6 };
    REF_DBL ratio;
    
    ratio = ref_matrix_vt_m_v( m, v );
    RWDS( 17.422, ratio, 1.0e-4, "ratio");
  }

  { /* macro matches deriv f */
    REF_DBL m[6]={ 1.0, 1.3, 0.4, 
                        1.8, 0.6,
                             0.5};
    REF_DBL v[3]={ 1.4, 1.5, 1.6 };
    REF_DBL f, d[3];
    RSS( ref_matrix_vt_m_v_deriv( m, v, &f, d ), "deriv");
    RWDS( ref_matrix_vt_m_v( m, v ), f, -1, "ratio");
  }

  { /* finite diff deriv */
    REF_DBL m[6]={ 1.0, 1.3, 0.4, 
                        1.8, 0.6,
                             0.5};
    REF_DBL v[3]={ 1.4, 1.5, 1.6 };

    REF_DBL f, d[3];
    REF_DBL fd[3], x0, step = 1.0e-7, tol = 2.0e-7;
    REF_INT dir;

    RSS( ref_matrix_vt_m_v_deriv( m, v, &f, d ), "deriv")
    for ( dir=0;dir<3;dir++) 
      {
	x0 = v[dir];
	v[dir] = x0+step;
	RSS( ref_matrix_vt_m_v_deriv( m, v, &(fd[dir]), d ), "fd")
	fd[dir] = (fd[dir]-f)/step;
	v[dir] = x0;
      }
    RSS( ref_matrix_vt_m_v_deriv( m, v, &f, d ), "deriv")
    RWDS( fd[0], d[0], tol, "d[0] expected" );
    RWDS( fd[1], d[1], tol, "d[1] expected" );
    RWDS( fd[2], d[2], tol, "d[2] expected" );
  }

  { /* diag decom, already diag and decending */
    REF_DBL m[6]={ 3.0, 0.0, 0.0, 
                        2.0, 0.0,
                             1.0};
    REF_DBL d[12];

    RSS( ref_matrix_diag_m( m, d ), "diag");

    RWDS(3.0,d[0],-1,"eig 0");
    RWDS(2.0,d[1],-1,"eig 1");
    RWDS(1.0,d[2],-1,"eig 2");

    RWDS(1.0,d[3],-1,"x0");
    RWDS(0.0,d[4],-1,"y0");
    RWDS(0.0,d[5],-1,"z0");

    RWDS(0.0,d[6],-1,"x1");
    RWDS(1.0,d[7],-1,"y1");
    RWDS(0.0,d[8],-1,"z1");

    RWDS(0.0,d[9],-1,"x2");
    RWDS(0.0,d[10],-1,"y2");
    RWDS(1.0,d[11],-1,"z2");
  }

  { /* diag decom, already diag and decending */
    REF_DBL m[6]={ 3.0, 0.0, 0.0, 
                        2.0, 0.0,
                             1.0};
    REF_DBL d[12];

    RSS( ref_matrix_diag_m( m, d ), "diag");

    RWDS(3.0,d[0],-1,"eig 0");
    RWDS(2.0,d[1],-1,"eig 1");
    RWDS(1.0,d[2],-1,"eig 2");

    RWDS(1.0,d[3],-1,"x0");
    RWDS(0.0,d[4],-1,"y0");
    RWDS(0.0,d[5],-1,"z0");

    RWDS(0.0,d[6],-1,"x1");
    RWDS(1.0,d[7],-1,"y1");
    RWDS(0.0,d[8],-1,"z1");

    RWDS(0.0,d[9],-1,"x2");
    RWDS(0.0,d[10],-1,"y2");
    RWDS(1.0,d[11],-1,"z2");
  }

  { /* diag decom, already diag and repeated */
    REF_DBL m[6]={ 10.0,  0.0,  0.0, 
                         10.0,  0.0,
                               10.0};
    REF_DBL d[12];

    RSS( ref_matrix_diag_m( m, d ), "diag");

    RWDS(10.0,d[0],-1,"eig 0");
    RWDS(10.0,d[1],-1,"eig 1");
    RWDS(10.0,d[2],-1,"eig 2");

    RWDS(1.0,d[3],-1,"x0");
    RWDS(0.0,d[4],-1,"y0");
    RWDS(0.0,d[5],-1,"z0");

    RWDS(0.0,d[6],-1,"x1");
    RWDS(1.0,d[7],-1,"y1");
    RWDS(0.0,d[8],-1,"z1");

    RWDS(0.0,d[9],-1,"x2");
    RWDS(0.0,d[10],-1,"y2");
    RWDS(1.0,d[11],-1,"z2");
  }

  { /* diag decom, already tri diag */
    REF_DBL m[6]={ 13.0, -4.0,  0.0, 
                          7.0,  0.0,
                                1.0};
    REF_DBL d[12];

    RSS( ref_matrix_diag_m( m, d ), "diag");

    RWDS(15.0,d[0],-1,"eig 0");
    RWDS( 5.0,d[1],-1,"eig 1");
    RWDS( 1.0,d[2],-1,"eig 2");

  }

  { /* diag decom, already tri diag */
    REF_DBL m[6]={ 13.0,  0.0, -4.0, 
                          4.0,  0.0,
                                7.0};
    REF_DBL d[12];

    RSS( ref_matrix_diag_m( m, d ), "diag");

    RWDS(15.0,d[0],-1,"eig 0");
    RWDS( 5.0,d[1],-1,"eig 1");
    RWDS( 4.0,d[2],-1,"eig 2");
  }

  { /* diag decom, already diad need sort */
    REF_DBL m[6]={ 1.0,     0.0,     0.0, 
                         1000.0,     0.0,
                               1000000.0};
    REF_DBL d[12];

    RSS( ref_matrix_diag_m( m, d ), "diag");

    RSS( ref_matrix_ascending_eig( d ), "ascend");

    RWDS(1000000.0,d[0],-1,"eig 0");
    RWDS(1000.0,d[1],-1,"eig 1");
    RWDS(1.0,d[2],-1,"eig 2");

    RWDS(0.0,d[3],-1,"x0");
    RWDS(0.0,d[4],-1,"y0");
    RWDS(1.0,d[5],-1,"z0");

    RWDS(0.0,d[6],-1,"x1");
    RWDS(1.0,d[7],-1,"y1");
    RWDS(0.0,d[8],-1,"z1");

    RWDS(1.0,d[9],-1,"x2");
    RWDS(0.0,d[10],-1,"y2");
    RWDS(0.0,d[11],-1,"z2");
  }

  { /* diag decom, exercise self check */
    REF_DBL m[6]={ 1.0,     2.0,     3.0, 
                            4.0,     5.0,
                                     6.0};
    REF_DBL d[12];
    REF_DBL m2[6];
    REF_DBL tol = -1.0;

    RSS( ref_matrix_diag_m( m, d ), "diag");

    RSS( ref_matrix_form_m( d, m2 ), "reform m" );
    RWDS( m[0], m2[0], tol, "m[0]");
    RWDS( m[1], m2[1], tol, "m[1]");
    RWDS( m[2], m2[2], tol, "m[2]");
    RWDS( m[3], m2[3], tol, "m[3]");
    RWDS( m[4], m2[4], tol, "m[4]");
    RWDS( m[5], m2[5], tol, "m[5]");
  }

  { /* diag decom, exercise self check r1 */
    REF_DBL m[6]={ 1345234.0,    245.0,    1700.0, 
                            45.0,     5.0,
                                 24000.0};
    REF_DBL d[12];
    REF_DBL m2[6];
    REF_DBL tol = 1.0e-9;

    RSS( ref_matrix_diag_m( m, d ), "diag");

    RSS( ref_matrix_form_m( d, m2 ), "reform m" );
    RWDS( m[0], m2[0], tol, "m[0]");
    RWDS( m[1], m2[1], tol, "m[1]");
    RWDS( m[2], m2[2], tol, "m[2]");
    RWDS( m[3], m2[3], tol, "m[3]");
    RWDS( m[4], m2[4], tol, "m[4]");
    RWDS( m[5], m2[5], tol, "m[5]");
  }

  { /* diag decom, exercise self check r2 */
    REF_DBL m[6]={ 1345234.0,  -10000.0,    3400.0, 
		   2345.0,     -15.0,
		   24.0};
    REF_DBL d[12];
    REF_DBL m2[6];
    REF_DBL tol = 1.0e-9;

    RSS( ref_matrix_diag_m( m, d ), "diag");

    RSS( ref_matrix_form_m( d, m2 ), "reform m" );
    RWDS( m[0], m2[0], tol, "m[0]");
    RWDS( m[1], m2[1], tol, "m[1]");
    RWDS( m[2], m2[2], tol, "m[2]");
    RWDS( m[3], m2[3], tol, "m[3]");
    RWDS( m[4], m2[4], tol, "m[4]");
    RWDS( m[5], m2[5], tol, "m[5]");
  }

  /* jac test code
    m =[
     5.569680  -0.166955  -0.056476
    -0.166955   5.645046   1.230437
    -0.056476   1.230437   2.881886
]
[vect,val]=eig(m)
j0=vect*sqrt(val)

jdet0=det(j0)
j =[
    2.2526284  -0.7038003   0.0032707
    0.6205073   2.2258236   0.5529248
    0.2414020   0.8461442  -1.4517752
]
jdet=det(j)

m
m0 = vect*val*vect'
m1=j0*j0'
   */
  
  { /* 10-1 jacobian identity */
    REF_DBL m[6]={ 1.0,  0.0,  0.0, 
                         1.0,  0.0,
                             100.0};
    REF_DBL j[9];

    RSS( ref_matrix_jacob_m( m, j ), "jacob");

    RWDS( 1.0,j[0],-1,"dadx");
    RWDS( 0.0,j[1],-1,"dady");
    RWDS( 0.0,j[2],-1,"dadz");

    RWDS( 0.0,j[3],-1,"dbdx");
    RWDS( 1.0,j[4],-1,"dbdy");
    RWDS( 0.0,j[5],-1,"dbdz");

    RWDS( 0.0,j[6],-1,"dcdx");
    RWDS( 0.0,j[7],-1,"dcdy");
    RWDS(10.0,j[8],-1,"dcdz");
  }

  { /* polar-1 jacobian at (sqrt(2)/4,sqrt(2)/4) */
    /*
      x=sqrt(2)/4;y=sqrt(2)/4
      r=sqrt(x^2+y^2)
      t=atan2(y,x)
      hz=0.1,ht=0.1
      h0=0.001,hr=h0+2*(1.0-h0)*abs(r-0.5)
      r=[cos(t),-sin(t),0;sin(t),cos(t),0;0,0,1]
      d=[hr^-2,0,0;0,ht^-2,0;0,0,hz^-2]
      m=r*d*r'
      [eigvec,eigval]=eig(m)
      sd=sqrt(eigval)
      j=sd*eigvec'
      m2=j'*j
    */
    REF_DBL m[6]={ 500050.0,  499950.0,  0.0,
                              500050.0,  0.0,
                                       100.0};
    REF_DBL j[9];

    RSS( ref_matrix_jacob_m( m, j ), "jacob");

    RWDS( 7.071067811865476,j[0],-1,"dadx");
    RWDS(-7.07106781186547,j[1],-1,"dady");
    RWDS( 0.0,j[2],-1,"dadz");

    RWDS( 707.1067811865477,j[3],-1,"dbdx");
    RWDS( 707.1067811865477,j[4],-1,"dbdy");
    RWDS(   0.0,j[5],-1,"dbdz");

    RWDS(  0.0,j[6],-1,"dcdx");
    RWDS(  0.0,j[7],-1,"dcdy");
    RWDS(-10.0,j[8],-1,"dcdz");
  }

  { /* det */
    REF_DBL tol = -1.0;
    REF_DBL m[6]={10.0, 0.0, 0.0, 
		        2.0, 0.0,
                             5.0};
    REF_DBL det;

    RSS( ref_matrix_det_m( m, &det ), "comp det" );
    RWDS( 100.0, det, tol, "check det");
  }

  /*
m=[
3.677054186758148e+14  -1.485195394845776e+15  -1.208526355699555e+15
-1.485195394845776e+15   5.998838665164314e+15   4.881347299319029e+15
-1.208526355699555e+15   4.881347299319029e+15   3.972027399714484e+15
]
[d,rconv]=det(m)
   */
  { /* inv act */
    REF_DBL tol = 5.0e22;
    REF_DBL m[6]={ 3.677054186758148e+14,
		   -1.485195394845776e+15,
		   -1.208526355699555e+15,
		   5.998838665164314e+15,
		   4.881347299319029e+15,
		   3.972027399714484e+15};
    
    REF_DBL det;

    RSS( ref_matrix_det_m( m, &det ), "inv");

    RWDS( 9.38923919771131e+28, det, tol, "det");
  }


  { /* inv diag */
    REF_DBL tol = -1.0;
    REF_DBL m[6]={10.0, 0.0, 0.0, 
		        2.0, 0.0,
                             5.0};
    
    REF_DBL inv[6];

    RSS( ref_matrix_inv_m( m, inv ), "inv");

    RWDS(   0.1, inv[0], tol, "inv[0]");
    RWDS(   0.0, inv[1], tol, "inv[1]");
    RWDS(   0.0, inv[2], tol, "inv[2]");
    RWDS(   0.5, inv[3], tol, "inv[3]");
    RWDS(   0.0, inv[4], tol, "inv[4]");
    RWDS(   0.2, inv[5], tol, "inv[5]");
  }

  /*
m =[
   1.1635e+13  -1.1826e+13   1.3501e+13
  -1.1826e+13   1.2021e+13  -1.3723e+13
   1.3501e+13  -1.3723e+13   1.5665e+13
]
[minv,rcond]=inv(m)
   */
  { /* inv trunc act */
    REF_DBL tol = -1.0;
    REF_DBL m[6]={1.1635e+13,  -1.1826e+13,   1.3501e+13, 
                                1.2021e+13,  -1.3723e+13,
                                              1.5665e+13};
    
    REF_DBL inv[6];

    RSS( ref_matrix_inv_m( m, inv ), "inv");

    RWDS(   8.2237e-10, inv[0], tol, "inv[0]");
    RWDS(   1.3934e-09, inv[1], tol, "inv[1]");
    RWDS(   5.1192e-10, inv[2], tol, "inv[2]");
    RWDS(   1.0294e-09, inv[3], tol, "inv[3]");
    RWDS(  -2.9913e-10, inv[4], tol, "inv[4]");
    RWDS(  -7.0318e-10, inv[5], tol, "inv[5]");
  }

  /*
m=[
3.677054186758148e+14  -1.485195394845776e+15  -1.208526355699555e+15
-1.485195394845776e+15   5.998838665164314e+15   4.881347299319029e+15
-1.208526355699555e+15   4.881347299319029e+15   3.972027399714484e+15
]
[minv,rcond]=inv(m)
   */
  { /* inv act */
    REF_DBL tol = -1.0;
    REF_DBL m[6]={ 3.677054186758148e+14,
		   -1.485195394845776e+15,
		   -1.208526355699555e+15,
		   5.998838665164314e+15,
		   4.881347299319029e+15,
		   3.972027399714484e+15};
    
    REF_DBL inv[6];

    RSS( ref_matrix_inv_m( m, inv ), "inv");

    RWDS( 9.36499074558910e-07, inv[0], tol, "inv[0]");
    RWDS(-6.42146778265400e-07, inv[1], tol, "inv[0]");
    RWDS( 1.07409260466118e-06, inv[2], tol, "inv[0]");
    RWDS( 4.85485288834797e-07, inv[3], tol, "inv[0]");
    RWDS(-7.92007026311118e-07, inv[4], tol, "inv[0]");
    RWDS( 1.30012461180195e-06, inv[5], tol, "inv[0]");
  }

  /*
m=[
       11635040597145.2  -11826239282213.9   13500655116445.6
      -11826239282213.9   12020580217385.7  -13722511333105.3
       13500655116445.6  -13722511333105.3   15665410998719.4
]
[minv,rcond]=inv(m)
   */
  { /* inv gen act */
    REF_DBL tol = 1.0e-11;
    REF_INT n=3;
    REF_DBL a[9]= {
       11635040597145.2,  -11826239282213.9,   13500655116445.6,
      -11826239282213.9,   12020580217385.7,  -13722511333105.3,
       13500655116445.6,  -13722511333105.3,   15665410998719.4 };
    REF_DBL inv[9];

    RSS( ref_matrix_inv_gen( n, a, inv ), "gen inv");

    RWDS(  1.16239577174213e-05, inv[0+0*3], tol, "inv[0,0]");
    RWDS(  5.76903477058831e-06, inv[1+0*3], tol, "inv[1,0]");
    RWDS( -4.96414675726866e-06, inv[2+0*3], tol, "inv[2,0]");
    RWDS(  5.76903477058831e-06, inv[0+1*3], tol, "inv[0,1]");
    RWDS(  4.14084289682497e-06, inv[1+1*3], tol, "inv[1,1]");
    RWDS( -1.34455362926181e-06, inv[2+1*3], tol, "inv[2,1]");
    RWDS( -4.96414675726866e-06, inv[0+2*3], tol, "inv[0,2]");
    RWDS( -1.34455362926181e-06, inv[1+2*3], tol, "inv[1,2]");
    RWDS(  3.10037074072063e-06, inv[2+2*3], tol, "inv[2,2]");
  }

  { /* M^(1/2) of eye */
    REF_DBL tol = -1.0;
    REF_DBL eye[6]={ 1.0, 0.0, 0.0, 
		          1.0, 0.0,
		               1.0};
    REF_DBL sqrt_eye[6], inv_sqrt_eye[6];
    RSS( ref_matrix_sqrt_m( eye, sqrt_eye, inv_sqrt_eye ), "sqrt m");

    RWDS( 1.0, sqrt_eye[0], tol, "m[0]");
    RWDS( 0.0, sqrt_eye[1], tol, "m[1]");
    RWDS( 0.0, sqrt_eye[2], tol, "m[2]");
    RWDS( 1.0, sqrt_eye[3], tol, "m[3]");
    RWDS( 0.0, sqrt_eye[4], tol, "m[4]");
    RWDS( 1.0, sqrt_eye[5], tol, "m[5]");

    RWDS( 1.0, inv_sqrt_eye[0], tol, "m[0]");
    RWDS( 0.0, inv_sqrt_eye[1], tol, "m[1]");
    RWDS( 0.0, inv_sqrt_eye[2], tol, "m[2]");
    RWDS( 1.0, inv_sqrt_eye[3], tol, "m[3]");
    RWDS( 0.0, inv_sqrt_eye[4], tol, "m[4]");
    RWDS( 1.0, inv_sqrt_eye[5], tol, "m[5]");
  }

  { /* M^(1/2) of 1,4,16 */
    REF_DBL tol = -1.0;
    REF_DBL m[6]={ 16.0, 0.0, 0.0, 
		         4.0, 0.0,
		              1.0};
    REF_DBL sqrt_m[6], inv_sqrt_m[6];
    RSS( ref_matrix_sqrt_m( m, sqrt_m, inv_sqrt_m ), "sqrt m");

    RWDS( 4.0,  sqrt_m[0], tol, "m[0]");
    RWDS( 0.0,  sqrt_m[1], tol, "m[1]");
    RWDS( 0.0,  sqrt_m[2], tol, "m[2]");
    RWDS( 2.0,  sqrt_m[3], tol, "m[3]");
    RWDS( 0.0,  sqrt_m[4], tol, "m[4]");
    RWDS( 1.0,  sqrt_m[5], tol, "m[5]");

    RWDS( 0.25, inv_sqrt_m[0], tol, "m[0]");
    RWDS( 0.0,  inv_sqrt_m[1], tol, "m[1]");
    RWDS( 0.0,  inv_sqrt_m[2], tol, "m[2]");
    RWDS( 0.5,  inv_sqrt_m[3], tol, "m[3]");
    RWDS( 0.0,  inv_sqrt_m[4], tol, "m[4]");
    RWDS( 1.0,  inv_sqrt_m[5], tol, "m[5]");
  }

  { /* M0 * M1 * M0' where M are Symmetric */
    REF_DBL tol = -1.0;
    REF_DBL m0[6]={ 1.0, 2.0, 3.0, 
	 	         4.0, 5.0,
		              6.0};
    REF_DBL m1[6]={ 3.0, 4.0, 5.0, 
	 	         6.0, 7.0,
		              8.0};
    REF_DBL m[6];
    /*
      m0=[ 1 2 3
           2 4 5
           3 5 6 ];
      m1=[ 3 4 5
           4 6 7
           5 7 8 ];
      m0*m1*m0'
    */

    RSS( ref_matrix_mult_m0m1m0( m0, m1, m ), "m0m1m0" );

    RWDS( 229,  m[0], tol, "m[0]");
    RWDS( 415,  m[1], tol, "m[1]");
    RWDS( 521,  m[2], tol, "m[2]");
    RWDS( 752,  m[3], tol, "m[3]");
    RWDS( 944,  m[4], tol, "m[4]");
    RWDS(1185,  m[5], tol, "m[5]");
  }

  { /* matrix vector product eye*/
    REF_DBL tol = -1.0;
    REF_DBL a[9];
    REF_DBL x[3];
    REF_DBL b[3];

    a[0] = 1.0; a[1] = 0.0; a[2] = 0.0;
    a[3] = 0.0; a[4] = 1.0; a[5] = 0.0;
    a[6] = 0.0; a[7] = 0.0; a[8] =10.0;

    x[0] = 1.0;
    x[1] = 1.0;
    x[2] = 0.1;

    RSS( ref_matrix_vect_mult( a, x, b ), "ax=b" );

    RWDS( 1.0,  b[0], tol, "b[0]");
    RWDS( 1.0,  b[1], tol, "b[1]");
    RWDS( 1.0,  b[2], tol, "b[2]");
  }

  { /* matrix vector product gen*/
    REF_DBL tol = -1.0;
    REF_DBL a[9];
    REF_DBL x[3];
    REF_DBL b[3];
    /*
      a=[1 2 3; 4 5 6; 7 8 9];
      x=[3 4 5]';
      b=a*x
   26
   62
   98
    */

    a[0] = 1.0; a[1] = 2.0; a[2] = 3.0;
    a[3] = 4.0; a[4] = 5.0; a[5] = 6.0;
    a[6] = 7.0; a[7] = 8.0; a[8] = 9.0;

    x[0] = 3.0;
    x[1] = 4.0;
    x[2] = 5.0;

    RSS( ref_matrix_vect_mult( a, x, b ), "ax=b" );

    RWDS( 26.0,  b[0], tol, "b[0]");
    RWDS( 62.0,  b[1], tol, "b[1]");
    RWDS( 98.0,  b[2], tol, "b[2]");
  }

  { /* solve x = 1*/
    REF_DBL tol = -1.0;
    REF_INT rows = 1, cols = 2;
    REF_DBL ab[2]={ 1.0, 1.0 };

    RSS( ref_matrix_solve_ab( rows, cols, ab ), "solve");

    RWDS( 1.0, ab[0+0*cols+rows*rows], tol, "x[0]");
  }

  { /* solve 0.5x = 1*/
    REF_DBL tol = -1.0;
    REF_INT rows = 1, cols = 2;
    REF_DBL ab[2]={ 0.5, 1.0 };

    RSS( ref_matrix_solve_ab( rows, cols, ab ), "solve");

    RWDS( 2.0, ab[0+0*cols+rows*rows], tol, "x[0]");
  }

  { /* solve Ix = [1,2]^t*/
    REF_DBL tol = -1.0;
    REF_INT rows = 2, cols = 3;
    REF_DBL ab[6]={ 1.0, 0.0, 0.0, 1.0, 1.0, 2.0};

    RSS( ref_matrix_solve_ab( rows, cols, ab ), "solve");

    RWDS( 1.0, ab[0+0*cols+rows*rows], tol, "x[0]");
    RWDS( 2.0, ab[1+0*cols+rows*rows], tol, "x[1]");
  }

  { /* solve flip(I)x = [1,2]^t*/
    REF_DBL tol = -1.0;
    REF_INT rows = 2, cols = 3;
    REF_DBL ab[6]={ 0.0, 1.0, 1.0, 0.0, 1.0, 2.0};

    RSS( ref_matrix_solve_ab( rows, cols, ab ), "solve");

    RWDS( 2.0, ab[0+0*cols+rows*rows], tol, "x[0]");
    RWDS( 1.0, ab[1+0*cols+rows*rows], tol, "x[1]");
  }

  { /* solve singular 1 */
    REF_INT rows = 1, cols = 2;
    REF_DBL ab[6]={ 0.0, 1.0};

    REIS( REF_DIV_ZERO, ref_matrix_solve_ab( rows, cols, ab ), "expect sing");
  }

  { /* solve singular 2 */
    REF_INT rows = 2, cols = 3;
    REF_DBL ab[6]={ 1.0, 0.0, 1.0, 0.0, 1.0, 2.0};

    REIS( REF_DIV_ZERO, ref_matrix_solve_ab( rows, cols, ab ), "expect sing");
  }

#define sqrt3 (1.73205080756888)
#define sqrt6 (2.44948974278318)

  { /* imply m from from iso tet nodes */
    REF_DBL tol = -1.0;
    REF_DBL m[6];
    REF_DBL xyz0[]={ 1.0/3.0*sqrt3,0.0,0.0};
    REF_DBL xyz1[]={-1.0/6.0*sqrt3,0.5,0.0};
    REF_DBL xyz2[]={-1.0/6.0*sqrt3,-0.5,0.0};
    REF_DBL xyz3[]={0.0,0.0, 1.0/3.0*sqrt6};

    RSS( ref_matrix_imply_m( m, xyz0, xyz1, xyz2, xyz3), "imply");

    RWDS( 1.0, m[0], tol, "m[0]");
    RWDS( 0.0, m[1], tol, "m[1]");
    RWDS( 0.0, m[2], tol, "m[2]");
    RWDS( 1.0, m[3], tol, "m[3]");
    RWDS( 0.0, m[4], tol, "m[4]");
    RWDS( 1.0, m[5], tol, "m[5]");
  }

  { /* imply m from from short z iso tet nodes */
    REF_DBL tol = -1.0;
    REF_DBL m[6];
    REF_DBL xyz0[]={ 1.0/3.0*sqrt3,0.0,0.0};
    REF_DBL xyz1[]={-1.0/6.0*sqrt3,0.5,0.0};
    REF_DBL xyz2[]={-1.0/6.0*sqrt3,-0.5,0.0};
    REF_DBL xyz3[]={0.0,0.0, 0.1 * 1.0/3.0*sqrt6};

    RSS( ref_matrix_imply_m( m, xyz0, xyz1, xyz2, xyz3), "imply");

    RWDS(   1.0, m[0], tol, "m[0]");
    RWDS(   0.0, m[1], tol, "m[1]");
    RWDS(   0.0, m[2], tol, "m[2]");
    RWDS(   1.0, m[3], tol, "m[3]");
    RWDS(   0.0, m[4], tol, "m[4]");
    RWDS( 100.0, m[5], tol, "m[5]");
  }

  { /* imply m from from right tet nodes */
    REF_DBL tol = -1.0;
    REF_DBL m[6];
    REF_DBL xyz0[]={0.0,0.0,0.0};
    REF_DBL xyz1[]={1.0,0.0,0.0};
    REF_DBL xyz2[]={0.0,1.0,0.0};
    REF_DBL xyz3[]={0.0,0.0,1.0};

    RSS( ref_matrix_imply_m( m, xyz0, xyz1, xyz2, xyz3), "imply");

    RWDS( 1.0, m[0], tol, "m[0]");
    RWDS( 0.5, m[1], tol, "m[1]");
    RWDS( 0.5, m[2], tol, "m[2]");
    RWDS( 1.0, m[3], tol, "m[3]");
    RWDS( 0.5, m[4], tol, "m[4]");
    RWDS( 1.0, m[5], tol, "m[5]");
  }

  { /* imply m from from short z right tet nodes */
    REF_DBL tol = -1.0;
    REF_DBL m[6];
    REF_DBL xyz0[]={0.0,0.0,0.0};
    REF_DBL xyz1[]={1.0,0.0,0.0};
    REF_DBL xyz2[]={0.0,1.0,0.0};
    REF_DBL xyz3[]={0.0,0.0,0.1};

    RSS( ref_matrix_imply_m( m, xyz0, xyz1, xyz2, xyz3), "imply");

    RWDS(   1.0, m[0], tol, "m[0]");
    RWDS(   0.5, m[1], tol, "m[1]");
    RWDS(   5.0, m[2], tol, "m[2]");
    RWDS(   1.0, m[3], tol, "m[3]");
    RWDS(   5.0, m[4], tol, "m[4]");
    RWDS( 100.0, m[5], tol, "m[5]");
  }

  { /* qr wiki 3 */
    REF_DBL tol = -1.0;
    REF_INT n=3;
    REF_DBL a[9]= {12.0, 6.0, -4.0,  -51.0, 167.0, 24.0,  4.0, -68.0, -41.0};
    REF_DBL q[9], r[9];

    /*
A=[12.0, 6.0, -4.0;  
   -51.0, 167.0, 24.0;  
    4.0, -68.0, -41.0]' , [Q, R] = qr(A);
Q=-Q
R=-R
[6.0/7.0, 3.0/7.0, 
 -2.0/7.0; -69.0/175.0, 158./175.0, 6.0/35.0; 
 -58.0/175.0, 6.0/175.0, -33/35.0]'-Q
     */

    RSS( ref_matrix_qr( n, a, q, r ), "qr");

    RWDS(   6.0/7.0,   q[0+0*3], tol, "q[0,0]");
    RWDS(   3.0/7.0,   q[1+0*3], tol, "q[1,0]");
    RWDS(  -2.0/7.0,   q[2+0*3], tol, "q[2,0]");
    RWDS( -69.0/175.0, q[0+1*3], tol, "q[0,1]");
    RWDS( 158.0/175.0, q[1+1*3], tol, "q[1,1]");
    RWDS(   6.0/35.0,  q[2+1*3], tol, "q[2,1]");
    RWDS( -58.0/175.0, q[0+2*3], tol, "q[0,2]");
    RWDS(   6.0/175.0, q[1+2*3], tol, "q[1,2]");
    RWDS( -33.0/35.0,  q[2+2*3], tol, "q[2,2]");

    RWDS(  14.0, r[0+0*3], tol, "r[0,0]");
    RWDS(   0.0, r[1+0*3], tol, "r[1,0]");
    RWDS(   0.0, r[2+0*3], tol, "r[2,0]");
    RWDS(  21.0, r[0+1*3], tol, "r[0,1]");
    RWDS( 175.0, r[1+1*3], tol, "r[1,1]");
    RWDS(   0.0, r[2+1*3], tol, "r[2,1]");
    RWDS( -14.0, r[0+2*3], tol, "r[0,2]");
    RWDS( -70.0, r[1+2*3], tol, "r[1,2]");
    RWDS(  35.0, r[2+2*3], tol, "r[2,2]");

  }

  { /* qr I2 */
    REF_DBL tol = -1.0;
    REF_INT n=2;
    REF_DBL a[4]= {1.0, 0.0, 0.0, 1.0};
    REF_DBL q[4], r[4];


    RSS( ref_matrix_qr( n, a, q, r ), "qr");

    RWDS(   1.0, q[0], tol, "q[0]");
    RWDS(   0.0, q[1], tol, "q[1]");
    RWDS(   0.0, q[2], tol, "q[2]");
    RWDS(   1.0, q[3], tol, "q[3]");

    RWDS(   1.0, r[0], tol, "r[0]");
    RWDS(   0.0, r[1], tol, "r[1]");
    RWDS(   0.0, r[2], tol, "r[2]");
    RWDS(   1.0, r[3], tol, "r[3]");

  }

  { /* diag gen */
    REF_DBL tol = 1.0e-5;
    REF_INT n=2;
    REF_DBL a[4]= {3.5,-0.5,0.21429,0.35714};
    REF_DBL vectors[4], values[2];

    /*
a1 = [ 3 1 ;
       1 5 ]
a2 = [ 10 1;
        1 2 ]
a3 = inv(a1)*a2
[vector3, value3] = eig(a3)
     */

    RSS( ref_matrix_diag_gen( n, a, values, vectors ), "gen diag");

    RWDS( 3.46553, values[0], tol, "val[0]");
    RWDS( 0.39161, values[1], tol, "val[1]");

    RWDS( -0.987300, vectors[0+0*2], tol, "vec[0,0]");
    RWDS(  0.158814, vectors[1+0*2], tol, "vec[1,0]");
    RWDS(  0.068775, vectors[0+1*2], tol, "vec[0,1]");
    RWDS( -0.997632, vectors[1+1*2], tol, "vec[1,1]");

  }

  { /* diag gen */
    REF_DBL tol = 4.0e-5;
    REF_INT n=3;
    REF_DBL a[9]= {1.6972e+05,  -2.5054e+04,  -1.9482e+05,
		   -2.5054e+04,   2.0420e+05,    2.2926e+05,
		   -1.9482e+05,   2.2926e+05,    4.2414e+05};
    REF_DBL vectors[9], values[3];

    /*
a=[
1.6972e+05,  -2.5054e+04,  -1.9482e+05;
-2.5054e+04,   2.0420e+05,    2.2926e+05;
-1.9482e+05,   2.2926e+05,    4.2414e+05]
[val,vec]=eig(a)
     */

    RSS( ref_matrix_diag_gen( n, a, values, vectors ), "gen diag");

    RWDS( 6.3802e+05, values[0], tol*1e5, "val[0]");
    RWDS( 1.6004e+05, values[1], tol*1e5, "val[1]");
    RWDS( 2.6620e+00, values[2], tol, "val[2]");
  }

  { /* diag gen */
    REF_DBL tol = 4.0e-5;
    REF_INT n=3;
    REF_DBL a[9]= {1.697158797858108301e+05, -2.505394046900878675e+04, -1.948194036868485855e+05,
		   -2.505394046900879403e+04, 2.041988806578010262e+05, 2.292606958352241491e+05,
		   -1.948194036868485855e+05, 2.292606958352241199e+05, 4.241375721589912428e+05};
    REF_DBL vectors[9], values[3];

    /*
a=[
1.697158797858108301e+05, -2.505394046900878675e+04, -1.948194036868485855e+05;
-2.505394046900879403e+04, 2.041988806578010262e+05, 2.292606958352241491e+05;
-1.948194036868485855e+05, 2.292606958352241199e+05, 4.241375721589912428e+05]
[val,vec]=eig(a)
     */

    RSS( ref_matrix_diag_gen( n, a, values, vectors ), "gen diag");

    RWDS( 6.3802e+05, values[0], tol*1e5, "val[0]");
    RWDS( 1.6003e+05, values[1], tol*1e5, "val[1]");
    RWDS( 2.9545e-06, values[2], tol, "val[2]");
  }

  { /* diag gen repeat 01*/
    REF_DBL tol = 1.0e-4;
    REF_INT n=3;
    REF_DBL a[9]= {   6.7943,   2.2648,   2.2648,
		      2.2648,   6.7943,  -2.2648,
		      2.2648,  -2.2648,   6.7943 };
    REF_DBL vectors[9], values[3];

    /*
a=[
   6.7943   2.2648   2.2648;
   2.2648   6.7943  -2.2648;
   2.2648  -2.2648   6.7943;
];
[val,vec]=eig(a)
     */

    RSS( ref_matrix_diag_gen( n, a, values, vectors ), "gen diag");

    RWDS( 9.0591, values[0], tol, "val[0]");
    RWDS( 9.0591, values[1], tol, "val[1]");
    RWDS( 2.2647, values[2], tol, "val[2]");
  }

  if (REF_FALSE)
  { /* diag gen repeat 12 */
    REF_DBL tol = 1.0e-4;
    REF_INT n=3;
    REF_DBL a[9]= {      0.7367900,   0.3904900,   0.7062800,
                         0.4569800,   0.3271400,   0.4786600,
                         0.0050863,   0.0029455,   0.0678280 };
    REF_DBL vectors[9], values[3];

    /*
a=[
   0.7367900   0.4569800   0.0050863
   0.3904900   0.3271400   0.0029455
   0.7062800   0.4786600   0.0678280
];
[val,vec]=eig(a)
     */

    RSS( ref_matrix_diag_gen( n, a, values, vectors ), "gen diag");

    RWDS( 1.0067, values[0], tol, "val[0]");
    RWDS( 0.0625, values[1], tol, "val[1]");
    RWDS( 0.0625, values[2], tol, "val[2]");
  }

  { /* inv gen I */
    REF_DBL tol = -1.0;
    REF_INT n=3;
    REF_DBL a[9]= {1.0,0.0,0.0, 0.0,1.0,0.0, 0.0,0.0,1.0 };
    REF_DBL inv[9];

    RSS( ref_matrix_inv_gen( n, a, inv ), "gen inv");

    RWDS(  1.0, inv[0+0*3], tol, "inv[0,0]");
    RWDS(  0.0, inv[1+0*3], tol, "inv[1,0]");
    RWDS(  0.0, inv[2+0*3], tol, "inv[2,0]");
    RWDS(  0.0, inv[0+1*3], tol, "inv[0,1]");
    RWDS(  1.0, inv[1+1*3], tol, "inv[1,1]");
    RWDS(  0.0, inv[2+1*3], tol, "inv[2,1]");
    RWDS(  0.0, inv[0+2*3], tol, "inv[0,2]");
    RWDS(  0.0, inv[1+2*3], tol, "inv[1,2]");
    RWDS(  1.0, inv[2+2*3], tol, "inv[2,2]");
  }

  { /* inv gen strang 3 */
    REF_DBL tol = -1.0;
    REF_INT n=3;
    REF_DBL a[9]= {2.0,-1.0,0.0, -1.0,2.0,-1.0, 0.0,-1.0,2.0 };
    REF_DBL inv[9];

    RSS( ref_matrix_inv_gen( n, a, inv ), "gen inv");

    RWDS(  3.0/4.0, inv[0+0*3], tol, "inv[0,0]");
    RWDS(  1.0/2.0, inv[1+0*3], tol, "inv[1,0]");
    RWDS(  1.0/4.0, inv[2+0*3], tol, "inv[2,0]");
    RWDS(  1.0/2.0, inv[0+1*3], tol, "inv[0,1]");
    RWDS(  1.0,     inv[1+1*3], tol, "inv[1,1]");
    RWDS(  1.0/2.0, inv[2+1*3], tol, "inv[2,1]");
    RWDS(  1.0/4.0, inv[0+2*3], tol, "inv[0,2]");
    RWDS(  1.0/2.0, inv[1+2*3], tol, "inv[1,2]");
    RWDS(  3.0/4.0, inv[2+2*3], tol, "inv[2,2]");
  }

  { /* inv gen 2 */
    REF_DBL tol = -1.0;
    REF_INT n=2;
    REF_DBL a[4]= {4.0,3.0,3.0,2.0};
    REF_DBL inv[4];

    /*
a = [ 4 3 ;
       3 2 ]
inv(a)
     */

    RSS( ref_matrix_inv_gen( n, a, inv ), "gen inv");

    RWDS( -2.0, inv[0+0*2], tol, "inv[0,0]");
    RWDS(  3.0, inv[1+0*2], tol, "inv[1,0]");
    RWDS(  3.0, inv[0+1*2], tol, "inv[0,1]");
    RWDS( -4.0, inv[1+1*2], tol, "inv[1,1]");
  }

  { /* inv gen swap 2 */
    REF_DBL tol = -1.0;
    REF_INT n=2;
    REF_DBL a[4]= {0.1,1.0,2.0,0.2};
    REF_DBL inv[4];

    /*
a = [ 0.1 2.0 ;
      1.0 0.2 ]
inv(a)
     */

    RSS( ref_matrix_inv_gen( n, a, inv ), "gen inv");

    RWDS( -0.1010101010101010, inv[0+0*2], tol, "inv[0,0]");
    RWDS(  0.5050505050505051, inv[1+0*2], tol, "inv[1,0]");
    RWDS(  1.0101010101010102, inv[0+1*2], tol, "inv[0,1]");
    RWDS( -0.0505050505050505, inv[1+1*2], tol, "inv[1,1]");
  }

  { /* inv gen pivots */
    REF_DBL tol = 1.0e-5;
    REF_INT n=3;
    REF_DBL a[9]= {0.00000,  -1.00000,   0.00000,
		   0.73860,   0.00000,   0.67414,
		   0.67414,   0.00000,  -0.73860};
    REF_DBL inv[9];

    /*
a = [ 0.00000  -1.00000   0.00000 ;
      0.73860   0.00000   0.67414 ;
      0.67414   0.00000  -0.73860 ]
inv(a)
     */

    RSS( ref_matrix_inv_gen( n, a, inv ), "gen inv");

    /* orthog: inv = at */
    RWDS( inv[0+0*3], a[0+0*3], tol, "[0,0]");
    RWDS( inv[1+0*3], a[0+1*3], tol, "[1,0]");
    RWDS( inv[2+0*3], a[0+2*3], tol, "[2,0]");
    RWDS( inv[0+1*3], a[1+0*3], tol, "[0,1]");
    RWDS( inv[1+1*3], a[1+1*3], tol, "[1,1]");
    RWDS( inv[2+1*3], a[1+2*3], tol, "[2,1]");
    RWDS( inv[0+2*3], a[2+0*3], tol, "[0,2]");
    RWDS( inv[1+2*3], a[2+1*3], tol, "[1,2]");
    RWDS( inv[2+2*3], a[2+2*3], tol, "[2,2]");

  }

  { /* transpose gen 3 */
    REF_DBL tol = -1.0;
    REF_INT n=3;
    REF_DBL a[9]= {1,2,3,4,5,6,7,8,9};
    REF_DBL at[9];

    RSS( ref_matrix_transpose_gen( n, a, at ), "gen inv");

    RWDS( a[0+0*3], at[0+0*3], tol, "[0,0]");
    RWDS( a[1+0*3], at[0+1*3], tol, "[1,0]");
    RWDS( a[2+0*3], at[0+2*3], tol, "[2,0]");
    RWDS( a[0+1*3], at[1+0*3], tol, "[0,1]");
    RWDS( a[1+1*3], at[1+1*3], tol, "[1,1]");
    RWDS( a[2+1*3], at[1+2*3], tol, "[2,1]");
    RWDS( a[0+2*3], at[2+0*3], tol, "[0,2]");
    RWDS( a[1+2*3], at[2+1*3], tol, "[1,2]");
    RWDS( a[2+2*3], at[2+2*3], tol, "[2,2]");

  }

  { /* orthog ident 3 */
    REF_INT n=3;
    REF_DBL a[9]= {1,0,0,0,1,0,0,0,1};

    RSS( ref_matrix_orthog( n, a ), "orth");
  }

  /*
#!/usr/bin/env octave

a1=rand(3,3);
a2=rand(3,3);

m1=a1*a1'
[m1p, m1d]=eig(m1)
m2=a2*a2'
[m2p, m2d]=eig(m2)
   */

  { /* intersect two metrics */
    REF_DBL m1[6]={ 1.5, 1.6, 1.0, 
		         2.0, 1.3,
		              1.0};
    REF_DBL m2[6]={ 1.1, 0.3, 0.7, 
		         1.1, 0.7,
		              0.8};
    /*
    m1 = [1.5 1.6 1.0
          1.6 2.0 1.3
          1.0 1.3 1.0]
    m2 = [1.1 0.3 0.7
          0.3 1.1 0.7
          0.7 0.7 0.8]
# barral-unpublished-intersection-metric-french
[m1p, m1d] = eig(m1)
m1halfd = m1d;
m1halfd(1,1) = sqrt(m1halfd(1,1));
m1halfd(2,2) = sqrt(m1halfd(2,2));
m1halfd(3,3) = sqrt(m1halfd(3,3));
m1half = m1p*m1halfd*m1p' 
m1halfd(1,1) = 1/m1halfd(1,1);
m1halfd(2,2) = 1/m1halfd(2,2);
m1halfd(3,3) = 1/m1halfd(3,3);
m1neghalf = m1p*m1halfd*m1p' 

m1bar_id_check = m1neghalf'*m1*m1neghalf
m2bar = m1neghalf'*m2*m1neghalf

[m12barp, m12bard]=eig(m2bar)
m12bard(1,1)=max(1.0,m12bard(1,1));
m12bard(2,2)=max(1.0,m12bard(2,2));
m12bard(3,3)=max(1.0,m12bard(3,3));

m12bar = m12barp*m12bard*m12barp'
m12 = m1half'*m12bar*m1half
[m12p, m12d]=eig(m12)
    */
    REF_DBL m12[6];
    REF_DBL tol = -1.0;
    RSS( ref_matrix_intersect( m1, m2, m12 ), "int");
    RWDS(  2.08172628978234, m12[0], tol, "m12[0]");
    RWDS(  1.30947862683631, m12[1], tol, "m12[1]");
    RWDS(  1.18305446069511, m12[2], tol, "m12[2]");
    RWDS(  2.14509000151343, m12[3], tol, "m12[3]");
    RWDS(  1.20858031651830, m12[4], tol, "m12[4]");
    RWDS(  1.05760258074793, m12[5], tol, "m12[5]");
  }

  { /* intersect same */
    REF_DBL m1[6]={ 1.0, 0.0, 0.0, 
                         2.0, 0.0,
                              3.0};
    REF_DBL m2[6]={ 1.0, 0.0, 0.0, 
                         2.0, 0.0,
                              3.0};
    REF_DBL m12[6];
    REF_DBL tol = -1.0;
    RSS( ref_matrix_intersect( m1, m2, m12 ), "int");
    RWDS(   1.0, m12[0], tol, "m12[0]");
    RWDS(   0.0, m12[1], tol, "m12[1]");
    RWDS(   0.0, m12[2], tol, "m12[2]");
    RWDS(   2.0, m12[3], tol, "m12[3]");
    RWDS(   0.0, m12[4], tol, "m12[4]");
    RWDS(   3.0, m12[5], tol, "m12[5]");
  }

  { /* intersect one direction small */
    REF_DBL m1[6]={ 1.0, 0.0, 0.0, 
                         2.0, 0.0,
                              4.0};
    REF_DBL m2[6]={ 8.0, 0.0, 0.0, 
                         1.0, 0.0,
                              1.0};
    REF_DBL m12[6];
    REF_DBL tol = -1.0;
    RSS( ref_matrix_intersect( m1, m2, m12 ), "int");
    RWDS(   8.0, m12[0], tol, "m12[0]");
    RWDS(   0.0, m12[1], tol, "m12[1]");
    RWDS(   0.0, m12[2], tol, "m12[2]");
    RWDS(   2.0, m12[3], tol, "m12[3]");
    RWDS(   0.0, m12[4], tol, "m12[4]");
    RWDS(   4.0, m12[5], tol, "m12[5]");
  }

  { /* intersect one direction big */
    REF_DBL m1[6]={ 0.25, 0.0, 0.0, 
                          1.0, 0.0,
                               1.0};
    REF_DBL m2[6]={ 1.0,  0.0, 0.0, 
                          1.0, 0.0,
                               0.5};
    REF_DBL m12[6];
    REF_DBL tol = -1.0;
    RSS( ref_matrix_intersect( m1, m2, m12 ), "int");
    RWDS(   1.0, m12[0], tol, "m12[0]");
    RWDS(   0.0, m12[1], tol, "m12[1]");
    RWDS(   0.0, m12[2], tol, "m12[2]");
    RWDS(   1.0, m12[3], tol, "m12[3]");
    RWDS(   0.0, m12[4], tol, "m12[4]");
    RWDS(   1.0, m12[5], tol, "m12[5]");
  }

  return 0;
}
