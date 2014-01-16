
#ifndef REF_MATRIX_H
#define REF_MATRIX_H

#include "ref_defs.h"

BEGIN_C_DECLORATION

#define ref_matrix_eig( d, e ) ((d)[(e)])
#define ref_matrix_vec( d, xyz, v) ((d)[(xyz)+3+3*(v)])

#define ref_matrix_sqrt_vt_m_v( m, v ) \
  sqrt( (v)[0] * ( (m)[0]*(v)[0] + (m)[1]*(v)[1] + (m)[2]*(v)[2] ) + \
	(v)[1] * ( (m)[1]*(v)[0] + (m)[3]*(v)[1] + (m)[4]*(v)[2] ) + \
	(v)[2] * ( (m)[2]*(v)[0] + (m)[4]*(v)[1] + (m)[5]*(v)[2] ) )

REF_STATUS ref_matrix_sqrt_vt_m_v_deriv( REF_DBL *m_upper_tri, REF_DBL *v, 
					 REF_DBL *f, REF_DBL *df_dv );

REF_STATUS ref_matrix_det_m( REF_DBL *m_upper_tri, REF_DBL *det );

REF_STATUS ref_matrix_diag_m( REF_DBL *m_upper_tri, 
			      REF_DBL *diagonal_system );

REF_STATUS ref_matrix_ascending_eig( REF_DBL *diagonal_system );

REF_STATUS ref_matrix_form_m( REF_DBL *diagonal_system,
			      REF_DBL *m_upper_tri);

REF_STATUS ref_matrix_inv_m( REF_DBL *m_upper_tri,
			     REF_DBL *inv_m_upper_tri);
REF_STATUS ref_matrix_log_m( REF_DBL *m_upper_tri,
			     REF_DBL *log_m_upper_tri);
REF_STATUS ref_matrix_exp_m( REF_DBL *m_upper_tri,
			     REF_DBL *exp_m_upper_tri);
REF_STATUS ref_matrix_sqrt_m( REF_DBL *m_upper_tri,
			      REF_DBL *sqrt_m_upper_tri);
REF_STATUS ref_matrix_average_m( REF_DBL *m0_upper_tri,
				 REF_DBL *m1_upper_tri,
				 REF_DBL *avg_m_upper_tri);
REF_STATUS ref_matrix_mult_m( REF_DBL *m0, REF_DBL *m1,
			      REF_DBL *a);

REF_STATUS ref_matrix_show_m( REF_DBL *m );

REF_STATUS ref_matrix_show_ab( REF_INT rows, REF_INT cols, REF_DBL *ab );
REF_STATUS ref_matrix_solve_ab( REF_INT rows, REF_INT cols, REF_DBL *ab );

REF_STATUS ref_matrix_imply_m( REF_DBL *m_upper_tri, 
			       REF_DBL *xyz0, REF_DBL *xyz1, 
			       REF_DBL *xyz2, REF_DBL *xyz3 );

REF_STATUS ref_matrix_show_aqr( REF_INT n, REF_DBL *a, REF_DBL *q, REF_DBL *r);
REF_STATUS ref_matrix_qr( REF_INT n, REF_DBL *a, REF_DBL *q, REF_DBL *r );

REF_STATUS ref_matrix_show_eig( REF_INT n, REF_DBL *a, 
				REF_DBL *values, REF_DBL *vectors);
REF_STATUS ref_matrix_diag_gen( REF_INT n, REF_DBL *a, 
				REF_DBL *values, REF_DBL *vectors );

REF_STATUS ref_matrix_inv_gen( REF_INT n, REF_DBL *a, REF_DBL *inv );
REF_STATUS ref_matrix_mult_gen( REF_INT n, REF_DBL *a, REF_DBL *b, REF_DBL *r );
REF_STATUS ref_matrix_transpose_gen( REF_INT n, REF_DBL *a, REF_DBL *at );
REF_STATUS ref_matrix_det_gen( REF_INT n, REF_DBL *a, REF_DBL *det );

REF_STATUS ref_matrix_orthog( REF_INT n, REF_DBL *a );

END_C_DECLORATION

#endif /* REF_MATRIX_H */
