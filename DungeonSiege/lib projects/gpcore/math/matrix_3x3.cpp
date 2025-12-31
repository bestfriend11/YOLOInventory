//////////////////////////////////////////////////////////////////////////////
//
// File     :  matrix_3x3.cpp
// Author(s):  Scott Bilas
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "matrix_3x3.h"

//////////////////////////////////////////////////////////////////////////////

// $$ optimization note: many of these functions can be easily accelerated
//    with a little 3DNow/KNI via intrinsics.

//////////////////////////////////////////////////////////////////////////////
// struct matrix_3x3 implementation

const float matrix_3x3::ZERO_INIT    [9] =  {  0, 0, 0, 0, 0, 0, 0, 0, 0  };
const float matrix_3x3::IDENTITY_INIT[9] =  {  1, 0, 0, 0, 1, 0, 0, 0, 1  };

const matrix_3x3 matrix_3x3::ZERO    ( ZERO_INIT     );
const matrix_3x3 matrix_3x3::IDENTITY( IDENTITY_INIT );

matrix_3x3& matrix_3x3 :: operator += ( const matrix_3x3& m )
{
	GetRow0() += m.GetRow0();
	GetRow1() += m.GetRow1();
	GetRow2() += m.GetRow2();
	return ( *this );
}

matrix_3x3& matrix_3x3 :: operator -= ( const matrix_3x3& m )
{
	GetRow0() -= m.GetRow0();
	GetRow1() -= m.GetRow1();
	GetRow2() -= m.GetRow2();
	return ( *this );
}

void matrix_3x3 :: Transform( vector_3* out_, const vector_3* src, int count, int srcSize ) const
{
	gpassert( v00 != FLOAT_INFINITE );

	VALIDATE_VECTOR_3(*src);

#if 0
	// Formula: out.x = x*v00 + y*v01 + z*v02
	//          out.y = x*v10 + y*v11 + z*v11
	//          out.z = x*v20 + y*v21 + z*v22
	//
	for ( int i = 0 ; i < count ; ++i, ++out_ )
	{
		float xt = ( src->x * v00 ) + ( src->y * v01 ) + ( src->z * v02 );
		float yt = ( src->x * v10 ) + ( src->y * v11 ) + ( src->z * v12 );
		float zt = ( src->x * v20 ) + ( src->y * v21 ) + ( src->z * v22 );
		out_->x = xt;
		out_->y = yt;
		out_->z = zt;
		src  = (vector_3*)(((BYTE*)src ) + srcSize);
	}

#else

	#define inx  dword ptr [esi]
	#define iny  dword ptr [esi+4]
	#define inz  dword ptr [esi+8]
	#define outx dword ptr [ebx]
	#define outy dword ptr [ebx+4]
	#define outz dword ptr [ebx+8]
	#define v00  dword ptr [edi]
	#define v01  dword ptr [edi+4]
	#define v02  dword ptr [edi+8]
	#define v10  dword ptr [edi+12]
	#define v11  dword ptr [edi+16]
	#define v12  dword ptr [edi+20]
	#define v20  dword ptr [edi+24]
	#define v21  dword ptr [edi+28]
	#define v22  dword ptr [edi+32]

_asm
{
	mov   ecx, count
	mov   esi, src
	mov   edi, this
	mov   ebx, out_
	mov   edx, srcSize

loopit:
	dec   ecx			// predecrement
	jl	  done

	//	ST0 	ST1 	ST2 	ST3 	ST4 	ST5 	ST6 	ST7
	fld   inx			//	x
	fmul  v00			//	x*v00
	fld   iny			//	y		x*v00
	fmul  v01			//	y*v01	x*v00
	fld   inz			//	z		y*v01	x*v00
	fmul  v02			//	z*v02	y*v01	x*v00
	fxch  st(2) 		//	y*v01	x*v00	z*v02
	faddp st(1), st(0)	//	xa		z*v02

	fld   inx			//	x		xa		z*v02
	fmul  v10			//	x*v10	xa		z*v02
	fxch  st(2) 		//	z*v02	xa		x*v10
	faddp st(1), st(0)	//	outx	x*v10
	fld   iny			//	y		outx	x*v10
	fmul  v11			//	y*v11	outx	x*v10
	fld   inz			//	z		y*v11	outx	x*v10
	fmul  v12			//	z*v12	y*v11	outx	x*v10
	fxch  st(3) 		//	x*v10	y*v11	outx	z*v12
	faddp st(1), st(0)	//	ya		outx	z*v12

	fld   inx			//	x		ya		outx	z*v12
	fmul  v20			//	x*v20	ya		outx	z*v12
	fxch  st(3) 		//	z*v12	ya		outx	x*v20
	faddp st(1), st(0)	//	outy	outx	x*v20
	fld   iny			//	y		outy	outx	x*v20
	fmul  v21			//	y*v21	outy	outx	x*v20
	fld   inz			//	z		y*v21	outy	outx	x*v20
	fmul  v22			//	z*v22	y*v21	outy	outx	x*v20
	fxch  st(4)			//	x*v20	y*v21	outy	outx	z*v22
	faddp st(1), st(0)	//	za		outy	outx	z*v22
	fxch  st(1) 		//	outy	za		outx	z*v22
	fstp  outy			//	za		outx	z*v22
	fxch  st(1) 		//	outx	za		z*v22
	fstp  outx			//	za		z*v22
	faddp st(1), st(0)	//	outz
	fstp  outz			//	<EMPTY>

	add   esi, edx
	add   ebx, 12
	jmp   loopit

done:
	}

	#undef inx
	#undef iny
	#undef inz
	#undef outx
	#undef outy
	#undef outz
	#undef v00
	#undef v01
	#undef v02
	#undef v10
	#undef v11
	#undef v12
	#undef v20
	#undef v21
	#undef v22

#endif

	VALIDATE_VECTOR_3(*out_);

}

float matrix_3x3 :: Determinant( void ) const
{
	// determinant is found via LaPlace expansion

	return ( v00 * (v11 * v22 - v12 * v21) -
			 v01 * (v10 * v22 - v12 * v20) +
			 v02 * (v10 * v21 - v11 * v20) );
}

void matrix_3x3 :: Transpose( matrix_3x3& out ) const
{
	gpassert( this != &out ); // $ cannot operate on self

	out.v00 = v00;
	out.v10 = v01;
	out.v20 = v02;

	out.v01 = v10;
	out.v11 = v11;
	out.v21 = v12;

	out.v02 = v20;
	out.v12 = v21;
	out.v22 = v22;
}

void matrix_3x3 :: Orthonormal( matrix_3x3& out ) const
{
	gpassert( this != &out ); // $ cannot operate on self

	// TODO: This orthonormalization is neither fast nor numerically stable.
	//       It should be replaced with some real code.

	vector_3& x = out.GetRow0();
	vector_3& y = out.GetRow1();
	vector_3& z = out.GetRow2();

	GetRow0().Normalize( x );
	CrossProduct( z, x, GetRow1() );
	z.Normalize();
	CrossProduct( y, z, x );
	y.Normalize();
}

void matrix_3x3 :: Invert( matrix_3x3& out ) const
{
	gpassert( this != &out ); // $ cannot operate on self

	// TODO: This inverse is neither fast nor numerically stable.
	//       It should be replaced with some real code.

	// $$$ find real code by Richard Carling in Graphics Gems

	float det = Determinant();
	gpassert( !::IsZero( det ) );
	float invdet = 1.0f / det;

	out.v00 =  (v11 * v22 - v12 * v21) * invdet;
	out.v10 = -(v10 * v22 - v12 * v20) * invdet;
	out.v20 =  (v10 * v21 - v11 * v20) * invdet;

	out.v01 = -(v01 * v22 - v02 * v21) * invdet;
	out.v11 =  (v00 * v22 - v02 * v20) * invdet;
	out.v21 = -(v00 * v21 - v01 * v20) * invdet;

	out.v02 =  (v01 * v12 - v02 * v11) * invdet;
	out.v12 = -(v00 * v12 - v02 * v10) * invdet;
	out.v22 =  (v00 * v11 - v01 * v10) * invdet;
}

void matrix_3x3 :: Negate( matrix_3x3& out ) const
{
	out.v00 = -v00;  out.v01 = -v01;  out.v02 = -v02;
	out.v10 = -v10;  out.v11 = -v11;  out.v12 = -v12;
	out.v20 = -v20;  out.v21 = -v21;  out.v22 = -v22;
}

bool matrix_3x3 :: IsOrthonormal( float epsilon ) const
{
	const vector_3& XAxis = GetRow0();
	const vector_3& YAxis = GetRow1();
	const vector_3& ZAxis = GetRow2();

	matrix_3x3 id = *this * Transpose_T();

	return (   ::IsOne( XAxis.Length(), epsilon )
			&& ::IsOne( YAxis.Length(), epsilon )
			&& ::IsOne( ZAxis.Length(), epsilon )
			&& id.IsIdentity(epsilon)
			&& ::IsZero( DotProduct( XAxis, YAxis ), epsilon )
			&& ::IsZero( DotProduct( XAxis, ZAxis ), epsilon )
			&& ::IsZero( DotProduct( ZAxis, YAxis ), epsilon ) );
}

bool matrix_3x3 :: IsEqual( const matrix_3x3& out, float epsilon ) const
{
	return (   ::IsEqual( v00, out.v00, epsilon )
			&& ::IsEqual( v01, out.v01, epsilon )
			&& ::IsEqual( v02, out.v02, epsilon )
			&& ::IsEqual( v10, out.v10, epsilon )
			&& ::IsEqual( v11, out.v11, epsilon )
			&& ::IsEqual( v12, out.v12, epsilon )
			&& ::IsEqual( v20, out.v20, epsilon )
			&& ::IsEqual( v21, out.v21, epsilon )
			&& ::IsEqual( v22, out.v22, epsilon ) );
}


//////////////////////////////////////////////////////////////////////////////
// matrix_3x3 helper function implementations

void Multiply( matrix_3x3& out, const matrix_3x3& l, const matrix_3x3& r )
{
	// can't use l or r as out
	gpassert( (&out != &l) && (&out != &r) );

	out.v00 = (l.v00 * r.v00) + (l.v01 * r.v10) + (l.v02 * r.v20);
	out.v01 = (l.v00 * r.v01) + (l.v01 * r.v11) + (l.v02 * r.v21);
	out.v02 = (l.v00 * r.v02) + (l.v01 * r.v12) + (l.v02 * r.v22);

	out.v10 = (l.v10 * r.v00) + (l.v11 * r.v10) + (l.v12 * r.v20);
	out.v11 = (l.v10 * r.v01) + (l.v11 * r.v11) + (l.v12 * r.v21);
	out.v12 = (l.v10 * r.v02) + (l.v11 * r.v12) + (l.v12 * r.v22);

	out.v20 = (l.v20 * r.v00) + (l.v21 * r.v10) + (l.v22 * r.v20);
	out.v21 = (l.v20 * r.v01) + (l.v21 * r.v11) + (l.v22 * r.v21);
	out.v22 = (l.v20 * r.v02) + (l.v21 * r.v12) + (l.v22 * r.v22);
}

void Multiply( matrix_3x3& out, const matrix_3x3& l, float r )
{
	out.GetRow0() = l.GetRow0() * r;
	out.GetRow1() = l.GetRow1() * r;
	out.GetRow2() = l.GetRow2() * r;
}

void Add( matrix_3x3& out, const matrix_3x3& l, const matrix_3x3& r )
{
	out.GetRow0() = l.GetRow0() + r.GetRow0();
	out.GetRow1() = l.GetRow1() + r.GetRow1();
	out.GetRow2() = l.GetRow2() + r.GetRow2();
}

void Subtract( matrix_3x3& out, const matrix_3x3& l, const matrix_3x3& r )
{
	out.GetRow0() = l.GetRow0() - r.GetRow0();
	out.GetRow1() = l.GetRow1() - r.GetRow1();
	out.GetRow2() = l.GetRow2() - r.GetRow2();
}

void JacobiTransform( matrix_3x3& a, matrix_3x3& v, vector_3& d )
{
	int		n=3;
	int		j,iq,ip,i;
	float	tresh,theta,tau,t,sm,s,h,g,c;

//	vector_3	b,z;
	float		b[3];
	float		z[3];
	float*		ae	= &a.v00;
	float*		ve	= &v.v00;
	float*		de	= &d.x;

//	v(1,0) = v(0,1) = v(2,0) = v(0,2) =	v(1,2) = v(2,1) = 0.0;
//	v(0,0) = v(1,1) = v(2,2) = 1.0;

	b[0] = de[0] = ae[0];  z[0] = 0.0;
	b[1] = de[1] = ae[4];  z[1] = 0.0;
	b[2] = de[2] = ae[8];  z[2] = 0.0;


	for(i=0; i<50; i++)	{

		sm=0.0;

		for(ip=0;ip<n;ip++) {
			for(iq=ip+1;iq<n;iq++) {
				sm+=FABSF(ae[(ip*3)+iq]);
			}
		}

		if (sm == 0.0) 	{
		      return;
		}

		if (i < 3) {
			tresh=0.2f*sm/(n*n);
		} else {
			tresh=0.0f;
		}

		for(ip=0; ip<n; ip++) {

			for(iq=ip+1; iq<n; iq++) {

				g = 100.0f*FABSF(ae[(ip*3)+iq]);

				if (i>3 && fabs(de[ip])+g==fabs(de[ip]) && fabs(de[iq])+g==fabs(de[iq])) {

					ae[(ip*3)+iq]=0.0;

				} else if (fabs(ae[(ip*3)+iq]) > tresh) {

					h = de[iq]-de[ip];

					if (fabs(h)+g == fabs(h)) {
						t=(ae[(ip*3)+iq])/h;
					} else {
						theta=0.5f*h/(ae[(ip*3)+iq]);
						t=1.0f/(FABSF(theta)+sqrtf(1.0f+theta*theta));
						if (theta < 0.0f)
							t = -t;
					}

					c=1.0f/sqrtf(1+t*t);
					s=t*c;
					tau=s/(1.0f+c);
					h=t*ae[(ip*3)+iq];
					z[ip] -= h;
					z[iq] += h;
					de[ip] -= h;
					de[iq] += h;
					ae[(ip*3)+iq] = 0.0;

					#define ROTATE(a,i,j,k,l)	g=a[(i*3)+j];				\
												h=a[(k*3)+l];				\
												a[(i*3)+j]=g-s*(h+g*tau);	\
												a[(k*3)+l]=h+s*(g-h*tau);

					for(j=0;j<ip;j++) {
						ROTATE(ae,j,ip,j,iq);
					}
					for(j=ip+1;j<iq;j++) {
						ROTATE(ae,ip,j,j,iq);
					}
					for(j=iq+1;j<n;j++) {
						ROTATE(ae,ip,j,iq,j);
					}
					for(j=0;j<n;j++) {
						ROTATE(ve,j,ip,j,iq);
					}

					#undef ROTATE
				}
			}
		}

		b[0] += z[0]; de[0] = b[0]; z[0] = 0.0;
		b[1] += z[1]; de[1] = b[1]; z[1] = 0.0;
		b[2] += z[2]; de[2] = b[2]; z[2] = 0.0;
	}

	gpassertm( 0, "JacobiTransform(): too many iterations" );
}

int Eigenvectors( matrix_3x3& eigenvectors, const matrix_3x3& m )
{
	if ( IsZero( m.Determinant() ) )
	{
		return (0);		// singular matrix
	}

	// Check to see if matrix is symmetric

	if ( m.IsSymmetric() )
	{
		// Use Jacobi method
		matrix_3x3	temp( m );		// JacobiTransform is destructive
		vector_3	eigenvalues;

		JacobiTransform( temp, eigenvectors, eigenvalues );

		return ( 3 );

	}
	else
	{
		gpassertm( 0, "Non-symmetric Eigenvectors() not implemented yet" );
	}

	return(0);

}

//////////////////////////////////////////////////////////////////////////////
