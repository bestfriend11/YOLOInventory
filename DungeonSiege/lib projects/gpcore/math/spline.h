#pragma once
#ifndef _SPLINE_H_
#define _SPLINE_H_

/*******************************************************************************************
**
**									Spline helpers
**
**		A collection of functions designed to aid in the use of splines.
**
**		Author:		James Loe
**		Date:		01/10/01
**
*******************************************************************************************/

#include "vector_3.h"


// Quadratic bezier spline helper
inline vector_3 QuadraticBezier( const float t, const vector_3& p0, const vector_3& p1, const vector_3& p2 )
{
	DEBUG_FLOAT_VALIDATE( t );
	gpassert( !IsNegative( t ) && t <= 1.00001f );

	float it	= 1.0f - t;
	return( (p2 * (t * t))		   + 
			(p1 * (2.0f * t * it)) + 
			(p0 * (it * it))	   );
}

// Cubic bezier spline helper
inline vector_3 CubicBezier( const float t, const vector_3& p0, const vector_3& p1, const vector_3& p2, const vector_3& p3 )
{
	DEBUG_FLOAT_VALIDATE( t );
	gpassert( !IsNegative( t ) && t <= 1.00001f );

	float it	= 1.0f - t;
	float t2	= t * t;
	float it2	= it * it;
	return( (p3 * (t2 * t))			+
			(p2 * (3.0f * t2 * it))	+
			(p1 * (3.0f * t * it2))	+
			(p0 * (it2 * it))		);			
}

// Quartic bezier spline helper
inline vector_3 QuarticBezier( const float t, const vector_3& p0, const vector_3& p1, const vector_3& p2, const vector_3& p3, const vector_3& p4 )
{
	DEBUG_FLOAT_VALIDATE( t );
	gpassert( !IsNegative( t ) && t <= 1.00001f );

	float it	= 1.0f - t;
	float t2	= t * t;
	float it2	= it * it;
	return( (p4 * (t2 * t2))			 +
			(p3 * (4.0f * t2 * t * it))	 +
			(p2 * (6.0f * t2 * it2))	 +
			(p1 * (4.0f * t * it2 * it)) +
			(p0 * (it2 * it2))			 );
}

// General bezier spline helper
inline vector_3 GeneralBezier( const float t, std::vector< vector_3 >& vp )
{
	DEBUG_FLOAT_VALIDATE( t );
	gpassert( !IsNegative( t ) && t <= 1.00001f );

	// Setup for n-size bezier spline
	vector_3 vRet;
	int n			= vp.size() - 1;

	// Check for 1.0f manually
	if( IsOne( t ) )
	{
		return vp.back();
	}

	// Check for valid control points
	if( n >= 2 )
	{
		float tk		= 1.0f;
		float tnk		= powf( 1.0f - t, (float)n );

		for( int i = 0; i <= n; ++i )
		{
			// Setup counters
			int rn		= n;
			int in		= i;
			int rni		= n - i;

			// Calculate blending for this control point
			float blend	= tk * tnk;
			tk			*= t;
			tnk			/= (1.0f - t);

			while( rn >= 1 )
			{
				blend	*= rn;
				--rn;

				if( in > 1 )
				{
					blend	/= in;
					--in;
				}
				if( rni > 1 )
				{
					blend	/= rni;
					--rni;
				}
			}

			// Accumulate the blending into our return vector
			vRet	+= vp[i] * blend;
		}
	}
	else
	{
		if( n == 0 )
		{
			// 1 point was given, just return that point
			return vp[0];
		}
		if( n == 1 )
		{
			// 2 points were given, linearly interpolate with t
			return vp[0] + ((vp[1] - vp[0]) * t);
		}
	}

	// Return blended position
	return vRet;
}


#endif