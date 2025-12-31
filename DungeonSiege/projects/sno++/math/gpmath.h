//////////////////////////////////////////////////////////////////////////////
//
// File     :  gpmath.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains GPG math wrappers. Many are pointless and should die.
//
//             Important: beware of ENDANGERED functions. These are permitted
//             to live for legacy code purposes.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef __GPMATH_H
#define __GPMATH_H

//////////////////////////////////////////////////////////////////////////////

#include "gpcore.h"

#include <math.h>
#include <float.h>
#include <algorithm>

//////////////////////////////////////////////////////////////////////////////
// General math library documentation

/*
	The math library functions have to be high performance given that they'll
	eventually end up inside of inner loops. To maintain performance try to do
	the following:

	* Use variables by direct access. Get/Set functions are bad when working
	  with such low-level structures.

	* Use uninitialized constructors. $$$ TEMPORARY BEGIN $$$ this can be done
	  by creating a vector like vector v(DoNotInitialize) but in the future
	  will be the default ctor $$$ TEMPORARY END $$$. Most of the time a vector
	  or matrix type will _not_ need to be initialized to zero or identity
	  because it will be immediately overwritten anyway.

	* Use the RISC-style functions. Generally a RISC instruction looks like
	  this: OPERATION OUT, IN1 [, IN2], where OUT is the output register, and
	  IN1 and the optional IN2 are input registers. All parameters are explicit,
	  and there are no temporary C++ objects created, as you would get if you,
	  say, multiplied two matrices together using the '*' operator. Certain
	  functions (like vector_3::Normalize) may act on themselves - check the
	  function implementations to see what's allowed.

	* Put temporaries outside of loops. Many operations require temporary
	  variables to store their data in. Declaring these outside a loop then
	  using them in the RISC-style functions will improve performance.

	* Sometimes it's inconvenient to use the RISC-style functions. In that case
	  generally a "_T" function will be provided that returns OUT as a temporary
	  variable. For example, vector_3::Normalize_T will return a normalized
	  temporary vector_3.

	* Use FTOL() explicitly rather than casting your floats to integers. The
	  compiler's intrinsic ftol() function will get called on a float-to-int
	  cast, and it's really really slow.
*/

//////////////////////////////////////////////////////////////////////////////
// Helpful constants

// general constants

#ifndef _GFLOAT_H
const float PI					= 3.14159265359f;	// 3DS Max may have already defined PI/Sin/Cos
#endif

const float PIHALF				= 1.57079632679f;
const float PI2					= 6.28318530718f;
const float DEGREES_TO_RADIANS	= PI / 180;
const float RADIANS_TO_DEGREES	= 180 / PI;
const float FLOAT_TOLERANCE		= 0.00001f;
const float FLOAT_MAX			= FLT_MAX;
const float FLOAT_MIN			= -FLT_MAX;			// $ don't use FLT_MIN - that's for "smallest" number
const float RADIANS_45			= 0.785398163397f;
const float RADIANS_90			= 1.57079632679f;
const float RADIANS_135			= 2.35619449019f;
const float RADIANS_180			= 3.14159265359f;
const float RADIANS_225			= 3.92699081699f;
const float RADIANS_270			= 4.71238898038f;
const float RADIANS_315			= 5.49778714378f;
const float RADIANS_360			= 6.28318530718f;

// works as int or float, use as an arbitrary test value
const float FLOAT_INFINITE		= ((float)0x7FFFFFFF);

//////////////////////////////////////////////////////////////////////////////
// Simple functions

// convert a float to an int without all the overhead of the RTL FTOL() which
//  saves and restores the FPU state

inline int FTOL( float f )
{
	int i;
	__asm
	{
		fld		dword ptr [f]
		fistp	[i]
	}
	return i;
}

inline float DegreesToRadians( float deg )
							   {  return ( deg * DEGREES_TO_RADIANS );  }
inline float RadiansToDegrees( float rad )
							   {  return ( rad * RADIANS_TO_DEGREES );  }
inline float Round           ( float f )
							   {  return ( (float)FTOL(f + 0.5f) );  }
inline int   Round           ( int i )
							   {  return ( FTOL(i + 0.5f) );  }
inline bool  IsZero          ( float f, float tolerance )
							   {  return ( fabsf( f ) < tolerance );  }
inline bool  IsZero          ( float f )
							   {  return ( fabsf( f ) < FLOAT_TOLERANCE );  }
inline bool  IsOne           ( float f, float tolerance )
							   {  return ( fabsf( f - 1.0f ) < tolerance );  }
inline bool  IsOne           ( float f )
							   {  return ( fabsf( f - 1.0f ) < FLOAT_TOLERANCE );  }
inline bool  IsEqual         ( float l, float r, float tolerance )
							   {  return ( fabsf( l - r ) < tolerance );  }
inline bool  IsEqual         ( float l, float r )
							   {  return ( fabsf( l - r ) < FLOAT_TOLERANCE );  }
inline bool  IsPositive      ( float f, float epsilon )
							   {  return ( f > epsilon );  }
inline bool  IsPositive      ( float f )
							   {  return ( f > FLOAT_TOLERANCE );  }
inline bool  IsNegative      ( float f, float epsilon )
							   {  return ( f < -epsilon );  }
inline bool  IsNegative      ( float f )
							   {  return ( f < -FLOAT_TOLERANCE );  }
inline float log2f           ( float f )
							   {  return ( logf( f ) / logf( 2 ) );  }

//////////////////////////////////////////////////////////////////////////////
// power-of-two functions

// return TRUE if the number is an even power of 2 we don't need a loop for
//  this, just some simple boolean math. note that 0 is not a power of 2.
template <typename T>
inline BOOL IsPower2( T n )
{
	return n && ((n & (n-1)) == 0);
}

// return the shift value for the passed power-of-two dimension
#pragma warning ( disable : 4035 )
inline int GetShift( int n )
{
	__asm
	{
		mov		edx, [n]
		bsr		eax, edx
	}
}
#pragma warning ( default : 4035 )

#pragma warning ( disable : 4035 )
inline int GetZeroShift( int n )
{
	__asm
	{
		mov		edx, [n]
		bsf		eax, edx
	}
}
#pragma warning ( default : 4035 )

// return the next power of two that contains the passed in number
template <typename T>
inline T MakePower2Up( T n )
{
	if ( IsPower2( n ) )
	{
		return ( n );
	}
	return ( T( (n == 0) ? 1 : (2 << GetShift( n )) ) );
}

// return the previous power of two that contains the passed in number - this
//  is useful for finding the most significant bit
template <typename T>
inline T MakePower2Down( T n )
{
	if ( IsPower2( n ) )
	{
		return ( n );
	}
	return ( T( (n == 0) ? 0 : (1 << GetShift(n)) ) );
}

//////////////////////////////////////////////////////////////////////////////
// alignment functions

// note that "alignment" should be a power of 2 for these to work

// variable-alignment
template <typename T> inline bool IsAligned   ( T  value, size_t alignment)  {  return ( !(value & (alignment - 1)) );  }
template <typename T> inline T    GetAlignUp  ( T  value, size_t alignment)  {  return ( (value + alignment - 1) & ~(alignment - 1) );  }
template <typename T> inline T    GetAlignDown( T  value, size_t alignment)  {  return ( value & ~(alignment - 1) );  }
template <typename T> inline void AlignUp     ( T& value, size_t alignment)  {  value = (value + alignment - 1) & ~(alignment - 1);  }
template <typename T> inline void AlignDown   ( T& value, size_t alignment)  {  value = value & ~(alignment - 1);  }

// DWORD-alignment functions
template <typename T> inline bool IsDwordAligned   ( T  value )  {  return ( !(value & 3) );  }
template <typename T> inline T    GetDwordAlignUp  ( T  value )  {  return ( (value + 3) & ~3 );  }
template <typename T> inline T    GetDwordAlignDown( T  value )  {  return ( value & ~3 );  }
template <typename T> inline void DwordAlignUp     ( T& value )  {  value = (value + 3) & ~3;  }
template <typename T> inline void DwordAlignDown   ( T& value )  {  value &= ~3;  }

// WORD-alignment functions
template <typename T> inline bool IsWordAligned   ( T  value )  {  return ( !(value & 1) );  }
template <typename T> inline T    GetWordAlignUp  ( T  value )  {  return ( (value + 1) & ~1 );  }
template <typename T> inline T    GetWordAlignDown( T  value )  {  return ( value & ~1 );  }
template <typename T> inline void WordAlignUp     ( T& value )  {  value = (value + 1) & ~1;  }
template <typename T> inline void WordAlignDown   ( T& value )  {  value &= ~1;  }

//////////////////////////////////////////////////////////////////////////////
// $$$ ENDANGERED $$$

// legacy code life support

typedef float real;

float const RealMinimum   = FLOAT_MIN;
float const RealMaximum   = FLOAT_MAX;
float const RealPi        = PI;
float const RealEpsilon   = FLT_EPSILON;
float const RealTolerance = FLOAT_TOLERANCE;
float const RealZero      = 0.0f;
float const RealIdentity  = 1.0f;
float const RealDegToRad  = DEGREES_TO_RADIANS;
float const RealRadToDeg  = RADIANS_TO_DEGREES;

enum do_not_initialize  {  DoNotInitialize  };

// $$$ many pointless wrappers follow. kill these!

inline void  Swap         ( float &Operand1, float &Operand2 )
							{  std::swap( Operand1, Operand2 );  }
inline float Square       ( float const Operand )
							{  return ( Operand * Operand );  }
inline float Cube         ( float const Operand )
							{  return ( Operand * Operand * Operand );  }
inline float LogBase2     ( float const Operand )
							{  return ( log2f( Operand ) );  }
inline float LogBase10    ( float const Operand )
							{  return ( log10f( Operand ) );  }
inline float NaturalLog   ( float const Operand )
							{  return ( logf( Operand ) );  }
inline float AbsoluteValue( float const Operand )
							{  return ( fabsf( Operand ) );  }
inline float SquareRoot   ( float const Operand )
							{  return ( sqrtf( Operand ) );  }
inline float Minimum      ( float const Operand1, float const Operand2 )
							{  return ( Operand1 < Operand2 ? Operand1 : Operand2 );  }
inline float Maximum      ( float const Operand1, float const Operand2 )
							{  return ( Operand1 > Operand2 ? Operand1 : Operand2 );  }
inline float RealInverse  ( float const Operand )
							{  return ( RealIdentity / Operand );  }
inline float Modulus      ( float const Dividend, float const Divisor )
							{  return ( fmodf( Dividend, Divisor ) );  }
inline bool  IsIdentity   ( float const Value, float const Tolerance )
							{  return ( AbsoluteValue( Value - RealIdentity ) < Tolerance );  }
inline float RadiansFrom  ( float const Degrees )
							{  return ( DegreesToRadians( Degrees ) );  }
inline float DegreesFrom  ( float const Radians )
							{  return ( RadiansToDegrees( Radians ) );  }
inline float ASin         ( float const AngleInRadians )
							{  return ( asinf( AngleInRadians ) );  }
inline float ACos         ( float const AngleInRadians )
							{  return ( acosf( AngleInRadians ) );  }
inline float ATan         ( float const AngleInRadians )
							{  return ( atanf( AngleInRadians ) );  }

#ifndef _GFLOAT_H
inline float Sin          ( float const AngleInRadians )
							{  return ( sinf( AngleInRadians ) );  }
inline float Cos          ( float const AngleInRadians )
							{  return ( cosf( AngleInRadians ) );  }
inline float Tan          ( float const AngleInRadians )
							{  return ( tanf( AngleInRadians ) );  }
#endif

//////////////////////////////////////////////////////////////////////////////

#endif  // __GPMATH_H

//////////////////////////////////////////////////////////////////////////////
