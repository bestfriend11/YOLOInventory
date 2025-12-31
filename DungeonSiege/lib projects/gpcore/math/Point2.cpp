//////////////////////////////////////////////////////////////////////////////
//
// File     :  Point2.h
// Author(s):  biddle (from Scott Bilas' vector_3)
//
// Summary  :  vector
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "Point2.h"

//////////////////////////////////////////////////////////////////////////////
// struct Point2 implementation

const Point2 Point2::ZERO   ( 0.0f, 0.0f );
const Point2 Point2::ONE    ( 1.0f, 1.0f );
const Point2 Point2::INVALID( FLOAT_INFINITE, FLOAT_INFINITE );

void Slerp( Point2& out, const Point2& v0, const Point2& v1, float t )
{
	float cos_omega = DotProduct( v0, v1 );

	float alpha, beta;

	if ( FABSF( cos_omega ) > 0.999 )
	{
		// Vectors are essentially either equal or opposite
		// Just do a linear interpolation
		alpha = 1.0f - t;
		beta  = t;
	}
	else
	{

		float omega = ACOSF( cos_omega );

		DEBUG_FLOAT_VALIDATE( omega );

		float sin_omega = SINF( omega );

		DEBUG_FLOAT_VALIDATE( sin_omega );

		alpha = SINF( (1.0f - t) * omega ) / sin_omega;
		beta  = SINF( t * omega ) / sin_omega;
	}

	out.x = (alpha * v0.x) + (beta * v1.x);
	out.y = (alpha * v0.y) + (beta * v1.y);
}


Point2 Eigenvalues(const Point2 m[2]) 
{
	// input matrix
	//
	// | m0.x, m0.y |
	// | m1.x, m1.y |

	// characteristic poly 
	//
	// t*t - (m0.x + m1.y) * t + (m0.x*m1.y - m0.y*m1.x) = 0

	// charpoly roots
	//
	// t = (-b +/- sqrt(b*b-4ac)) / 2a
	//
	// a = 1
	// b = (m0.x + m1.y)
	// c = (m0.x*m1.y - m0.y*m1.x)
	//

	float b = (m[0].x + m[1].y);
	float c = (m[0].x*m[1].y - m[0].y*m[1].x);

	gpassert(!IsNegative((b*b)-(4.0f*c)));

	float d = SQRTF((b*b)-(4*c));

	return Point2((-b + d)*0.5f,(-b - d)*0.5f);

}

bool EigenSystem(const Point2 m[2], Point2& eigvals, Point2& eigvec )
{
	// input matrix
	//
	// | m0.x, m0.y |
	// | m1.x, m1.y |

	// characteristic poly for eigenvalues
	//
	// t*t - (m0.x + m1.y) * t + (m0.x*m1.y - m0.y*m1.x) = 0

	// charpoly roots
	//
	// t = (-b +/- sqrt(b*b-4ac)) / 2a
	//
	// a = 1
	// b = (m0.x + m1.y)
	// c = (m0.x*m1.y - m0.y*m1.x)
	//

	float negb = (m[0].x + m[1].y);		
	float c = (m[0].x*m[1].y - m[0].y*m[1].x);

	gpassert(!IsNegative((negb*negb)-(4.0f*c)));

	float d = SQRTF((negb*negb)-(4*c));

	eigvals.x = (negb + d)*0.5f;
	eigvals.y = (negb - d)*0.5f;

	// | m0.x, m0.y | - | eval.x    0     | = 0
	// | m1.x, m1.y |   |   0     eval.x  |

	// Need to solve:

	// (M - E0 * I) * V0 = 0		// eigenval 0
	// (M - E1 * I) * V1 = 0		// eigenval 1 

	float diff0 = m[0].x-eigvals.x;
	float diff1 = m[1].y-eigvals.x;

	if (IsZero(diff0)||IsZero(diff1)) {

		eigvec.x = 1.0f; eigvec.y = 0.0f;

	} 
	else
	{

		eigvec.x = -m[0].y/diff0;
		eigvec.y = -m[1].x * eigvec.x /diff1;

		eigvec.Normalize();

	}

	return true;
}

