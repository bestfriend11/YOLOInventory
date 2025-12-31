//////////////////////////////////////////////////////////////////////////////
//
// File     :  vector_3.cpp
// Author(s):  Scott Bilas
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "vector_3.h"

//////////////////////////////////////////////////////////////////////////////

FUBI_REPLACE_NAME( "vector_3", Vector );

//////////////////////////////////////////////////////////////////////////////
// struct vector_3 implementation

const vector_3 vector_3::ZERO   ( 0.0f, 0.0f, 0.0f );
const vector_3 vector_3::ONE    ( 1.0f, 1.0f, 1.0f );
const vector_3 vector_3::INVALID( FLOAT_INFINITE, FLOAT_INFINITE, FLOAT_INFINITE );
const vector_3 vector_3::UNIT   ( 0.577350269f, 0.577350269f, 0.577350269f );

const vector_3 vector_3::EAST ( 1.0f, 0.0f, 0.0f );
const vector_3 vector_3::UP   ( 0.0f, 1.0f, 0.0f );
const vector_3 vector_3::NORTH( 0.0f, 0.0f, 1.0f );

void Slerp( vector_3& out, const vector_3& v0, const vector_3& v1, float t )
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
	out.z = (alpha * v0.z) + (beta * v1.z);

	DEBUG_FLOAT_VALIDATE( out.x );
	DEBUG_FLOAT_VALIDATE( out.y );
	DEBUG_FLOAT_VALIDATE( out.z );

}

void SlerpInPlace( vector_3& v0, const vector_3& v1, float t )
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

	v0.x = (alpha * v0.x) + (beta * v1.x);
	v0.y = (alpha * v0.y) + (beta * v1.y);
	v0.z = (alpha * v0.z) + (beta * v1.z);

	DEBUG_FLOAT_VALIDATE( v0.x );
	DEBUG_FLOAT_VALIDATE( v0.y );
	DEBUG_FLOAT_VALIDATE( v0.z );

}
//////////////////////////////////////////////////////////////////////////////
