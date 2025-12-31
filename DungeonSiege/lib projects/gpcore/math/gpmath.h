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

#include "GpCore.h"

#include <cmath>
#include <cfloat>
#include <cstdlib>

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
const float PI					= 3.14159265359f;		// 3DS Max may have already defined PI/Sin/Cos
#endif

const float PIHALF				= 1.57079632679f;
const float PI2					= 6.28318530718f;
const float DEGREES_TO_RADIANS	= PI / 180;
const float RADIANS_TO_DEGREES	= 180 / PI;
const float FLOAT_TOLERANCE		= 0.00001f;
const float FLOAT_MAX			= FLT_MAX;
const float FLOAT_MIN			= -FLT_MAX;				// $ don't use FLT_MIN - that's for "smallest" number
const float RADIANS_45			= 0.785398163397f;
const float RADIANS_90			= 1.57079632679f;
const float RADIANS_135			= 2.35619449019f;
const float RADIANS_180			= 3.14159265359f;
const float RADIANS_225			= 3.92699081699f;
const float RADIANS_270			= 4.71238898038f;
const float RADIANS_315			= 5.49778714378f;
const float RADIANS_360			= 6.28318530718f;
const float SQRT_ONEHALF		= 0.70710678119f;		// sqrt(0.5) == sin(45deg)
const float FLOAT_ONE			= 1.0f;					// For use in floating point assembly
const float FLOAT_TWO			= 2.0f;					// For use in floating point assembly
const float FLOAT_ONEHALF		= 0.5f;					// For use in floating point assembly
const float FLOAT_ONEANDHALF	= 1.5f;					// For use in floating point assembly
const float FLOAT_INFINITE		= (float)0x7FFFFFFF;	// works as int or float, use as an arbitrary test value
const float FLOAT_INDEFINITE	= (float)0xEFC00000;	// use a Signalling SNaN, rather than a quiet QNaN


// Fwd decl
WORD GetFpuStackTop( void );
WORD GetFpuStackTags( void );

#ifdef PARANOID_FPU_STACK
#define DEBUG_FPU_STACK_VALIDATE( TOS )											\
	{																			\
		int val = 7&(8-TOS);													\
		WORD top = GetFpuStackTop();											\
		gpassertf(top == val,("FPU Stack TOP is %d, not %d",top,val));			\
		DWORD tag = GetFpuStackTags();											\
		gpassertm( (tag & 0x0003) != 0x0002 , "FPU ST(0) is invalid" );			\
		gpassertm( (tag & 0x000C) != 0x0008 , "FPU ST(1) is invalid" );			\
		gpassertm( (tag & 0x0030) != 0x0020 , "FPU ST(2) is invalid" );			\
		gpassertm( (tag & 0x00C0) != 0x0080 , "FPU ST(3) is invalid" );			\
		gpassertm( (tag & 0x0300) != 0x0200 , "FPU ST(4) is invalid" );			\
		gpassertm( (tag & 0x0C00) != 0x0800 , "FPU ST(5) is invalid" );			\
		gpassertm( (tag & 0x3000) != 0x2000 , "FPU ST(6) is invalid" );			\
		gpassertm( (tag & 0xC000) != 0x8000 , "FPU ST(7) is invalid" );			\
	}
#else
#define DEBUG_FPU_STACK_VALIDATE( TOS )
#endif

#ifdef GP_DEBUG
#define DEBUG_FLOAT_VALIDATE( f )	gpassertm( !_isnan( f ), "Float validation failed!" )
#else
#define DEBUG_FLOAT_VALIDATE( f )
#endif


//////////////////////////////////////////////////////////////////////////////
// MATH enhancements

// fast float version of fabsf
inline float FABSF( float f )
{
	DEBUG_FLOAT_VALIDATE( f );

#ifdef USE_INTRINSICS

	return fabsf( f );

#else

	__asm
	{
		fld		f
		fabs
		fstp	f
	}
	DEBUG_FLOAT_VALIDATE( f );
	return f;

#endif // USE_INTRINSICS
}

// fast float version of sinf
inline float SINF( float f )
{
	DEBUG_FLOAT_VALIDATE( f );

#ifdef USE_INTRINSICS

	return sinf( f );

#else

	__asm
	{
		fld		f
		fsin
		fstp	f
	}
	DEBUG_FLOAT_VALIDATE( f );
	return f;

#endif // USE_INTRINSICS
}

// fast float version of cosf
inline float COSF( float f )
{
	DEBUG_FLOAT_VALIDATE( f );

#ifdef USE_INTRINSICS

	return cosf( f );

#else

	__asm
	{
		fld		f
		fcos
		fstp	f
	}
	DEBUG_FLOAT_VALIDATE( f );
	return f;

#endif // USE_INTRINSICS
}

// fast float version of tanf
inline float TANF( float f )
{
	DEBUG_FLOAT_VALIDATE( f );

#ifdef USE_INTRINSICS

	return tanf( f );

#else

	__asm
	{
		fld		f
		fptan
		ffreep	st
		fstp	f
	}
	DEBUG_FLOAT_VALIDATE( f );
	return f;

#endif // USE_INTRINSICS
}

// fast float version of atan2f
inline float ATAN2F( float y, float x )
{
	DEBUG_FLOAT_VALIDATE( y );
	DEBUG_FLOAT_VALIDATE( x );

#ifdef USE_INTRINSICS

	return atan2f( y, x );

#else

	__asm
	{
		fld		y
		fld		x
		fpatan
		fstp	x
	}
	DEBUG_FLOAT_VALIDATE( x );
	return x;

#endif // USE_INTRINSICS
}

// fast floating point method for retrieving the sine and cosine
// of a given number.  It is approximately twice as fast as
// calling sinf and cosf seperately and utilizes the built
// in fsincos instruction to maintain full accuracy.
inline void SINCOSF( const float f, float& sin, float& cos )
{
	DEBUG_FLOAT_VALIDATE( f );
	__asm
	{
		fld		f
		mov		eax, dword ptr [sin]

		fsincos
		mov		ecx, dword ptr [cos]

		fstp	dword ptr [ecx]
		fstp	dword ptr [eax]
	}
	DEBUG_FLOAT_VALIDATE( sin );
	DEBUG_FLOAT_VALIDATE( cos );
}

// Fast float version of sqrtf
// In my tests, this version of square root is approximately twice as fast
// as a full 32-bit sqrtf(), and maintains full accuracy out to the hundred-
// thousandth place.  Average error is approximately 0.00002f, worst case
// error is approximately 0.00006f.
inline float SQRTF( float f )
{
	DEBUG_FLOAT_VALIDATE( f );

#ifdef USE_INTRINSICS

	return sqrtf( f );

#else


	__asm
	{
		fld		f
		fsqrt
		fstp	f
	}
	DEBUG_FLOAT_VALIDATE( f );
	return f;

#endif // USE_INTRINSICS
}

// fast float version of asinf
inline float ASINF( float f )
{
	DEBUG_FLOAT_VALIDATE( f );
	return ATAN2F( f, SQRTF( 1.0f - (f*f) ) );
}

// fast float version of acosf
inline float ACOSF( float f )
{
	DEBUG_FLOAT_VALIDATE( f );
	return ATAN2F( SQRTF( 1.0f - (f*f) ), f );
}

// faster but less accurate float sqrt
// This is a very crude but very quick square root approximation.  Use where
// speed is absolutely essential and accuracy is not.  Average error is
// approximately 0.15f, worst case error is approximately 0.7f.
inline float FASTSQRTF( float f )
{
	DEBUG_FLOAT_VALIDATE( f );
	__asm
	{
		mov		eax, f
		sar		eax, 1
		add		eax, 0x1FC00000
		mov		dword ptr [f], eax
	}
	DEBUG_FLOAT_VALIDATE( f );
	return f;
}

// This is a normal floating point reciprocal.  A good central
// location to replace with a fast SIMD reciprocal if we
// ever get a chance.
inline float INVERSEF( const float f )
{
	return 1.0f / f;
}

//////////////////////////////////////////////////////////////////////////////
// Simple functions

inline float DegreesToRadians( float deg ) 							{ return ( deg * DEGREES_TO_RADIANS );  }

inline float RadiansToDegrees( float rad ) 							{ return ( rad * RADIANS_TO_DEGREES );  }

inline bool  IsZero          ( float f, float tolerance )			{ return ( FABSF( f ) < tolerance );  }

inline bool  IsZero          ( float f )							{ return ( FABSF( f ) < FLOAT_TOLERANCE );  }

inline bool  IsOne           ( float f, float tolerance )			{ return ( FABSF( f - 1.0f ) < tolerance );  }

inline bool  IsOne           ( float f )							{ return ( FABSF( f - 1.0f ) < FLOAT_TOLERANCE );  }

inline bool  IsEqual         ( float l, float r, float tolerance )	{ return ( FABSF( l - r ) < tolerance );  }

inline bool  IsEqual         ( float l, float r )					{ return ( FABSF( l - r ) < FLOAT_TOLERANCE );  }

inline bool  IsPositive      ( float f, float epsilon )				{ return ( f > epsilon );  }

inline bool  IsPositive      ( float f )							{ return ( f > FLOAT_TOLERANCE );  }

inline bool  IsNegative      ( float f, float epsilon )				{ return ( f < -epsilon );  }

inline bool  IsNegative      ( float f )							{ return ( f < -FLOAT_TOLERANCE );  }

inline float log2f           ( float f )							{ return ( logf( f ) / logf( 2 ) );  }

//////////////////////////////////////////////////////////////////////////////
// FPU mode

inline WORD GetFpuControlWord( void )
{
	WORD w;
	__asm fnstcw [ w ]
	return ( w );
}

inline void SetFpuControlWord( WORD w )
{
	__asm fldcw [ w ]
}

inline void SetFpuChopMode( void )
{
	WORD mask = 0x0F00, mode = 0x0C00;

	WORD old = GetFpuControlWord();
	mode &= mask;
	SetFpuControlWord( (WORD)((old & ~mask) | mode) );
}

#define ENTER_FPU_PRECISEMODE \
	WORD fpuPrecise_oldControlWord = GetFpuControlWord(); \
	WORD fpuPrecise_mask = 0x0F00; \
	WORD fpuPrecise_mode = 0x0F00; \
	fpuPrecise_mode &= fpuPrecise_mask; \
	if ( (fpuPrecise_oldControlWord & ~fpuPrecise_mask) != fpuPrecise_mode ) \
	{ SetFpuControlWord( (WORD)((fpuPrecise_oldControlWord & ~fpuPrecise_mask) | fpuPrecise_mode) ); }

#define EXIT_FPU_PRECISEMODE \
	if ( GetFpuControlWord() != fpuPrecise_oldControlWord ) \
	{ SetFpuControlWord( fpuPrecise_oldControlWord ); }																	

class AutoFpuMode
{
	WORD m_OldControlWord;

public:
	AutoFpuMode( WORD mask = 0x0F00, WORD mode = 0x0C00 )		// 24-bit precision, chop mode
	{
		m_OldControlWord = GetFpuControlWord();
		mode &= mask;
		if ( (m_OldControlWord & ~mask) != mode )
		{
			SetFpuControlWord( (WORD)((m_OldControlWord & ~mask) | mode) );
		}
	}

   ~AutoFpuMode( void )
	{
		if ( GetFpuControlWord() != m_OldControlWord )
		{
			SetFpuControlWord( m_OldControlWord );
		}
	}
};

inline WORD GetFpuStackTop( void )
{
	WORD w;

	__asm
	{
		fnstsw [ w ]
	}
	return (WORD)((w >> 11) & 0x0007);
}

inline WORD GetFpuStackTags( void )
{
	WORD env[14];

	__asm 
	{
		fnstenv  env 
	}

	return env[4];
}

//////////////////////////////////////////////////////////////////////////////
// FPU functions

// $ these functions switch into full precision mode to do the math, then 

inline double PreciseAdd( double x, double y )
{
	ENTER_FPU_PRECISEMODE;
	double result = ( x + y );
	EXIT_FPU_PRECISEMODE;

	return result;
}

inline double PreciseSubtract( double x, double y )
{
	ENTER_FPU_PRECISEMODE;
	double result = ( x - y );
	EXIT_FPU_PRECISEMODE;

	return result;
}

inline double PreciseMultiply( double x, double y )
{
	ENTER_FPU_PRECISEMODE;
	double result = ( x * y );
	EXIT_FPU_PRECISEMODE;

	return result;
}

inline double PreciseDivide( double x, double y )
{
	ENTER_FPU_PRECISEMODE;
	double result = ( x / y );
	EXIT_FPU_PRECISEMODE;

	return result;
}

//////////////////////////////////////////////////////////////////////////////
// FTOL enhancements

/*	Purpose:

	This function replaces the standard CRT function _ftol with a much
	faster version that just assumes the FPU is set up how we already need
	it. _ftol is an intrinsic called by the compiler when you cast from a
	float to an int.

	This version is at least 6x faster, but still suffers from needing to
	play with the stack frame. If you use the inlined FTOL() (shown below)
	you'll get another 2x perf boost.

	Note: this function does *not* push 'f' onto the FPU stack (ST0) because
	the intrinsic call will do that for you through an fld. The compiler
	assumes that _ftol pops ST0 off, converts it to an int, and stores the
	result into eax. This is a Microsoft-specific convention.

	Note: this relies upon setting the FPU to "chop mode" rather than its
	default rounding mode.

	Usage: just put a DEFINE_FAST_FTOL() somewhere in one of your project's
	CPP files. also instantiate a default AutoFpuMode somewhere too, probably
	inside your WinMain() or something.
*/

#define DEFINE_FAST_FTOL()			   \
	extern "C" int ftol( float /*f*/ ) \
	{								   \
		int i;						   \
		__asm						   \
		{							   \
			fistp [ i ]				   \
		}							   \
		return ( i );				   \
	}

/*	For reference, here is the old _ftol source from the CRT:

	__ftol:

		push        ebp
		mov         ebp,esp
		add         esp,0F4h
		wait
		fnstcw      word ptr [ebp-2]
		wait
		mov         ax,word ptr [ebp-2]
		or          ah,0Ch
		mov         word ptr [ebp-4],ax
		fldcw       word ptr [ebp-4]
		fistp       qword ptr [ebp-0Ch]
		fldcw       word ptr [ebp-2]
		mov         eax,dword ptr [ebp-0Ch]
		mov         edx,dword ptr [ebp-8]
		leave
		ret
*/

// inline version of FTOL is in gpglobal.h

// these will round numbers to nearest
inline int   Round       ( float f )  {  return ( FTOL( f + (( f >= 0.0f ) ? 0.5f : -0.5f)) );  }
inline float RoundToFloat( float f )  {  return ( (float)Round( f ) );  }

//////////////////////////////////////////////////////////////////////////////

// ENDANGERED ENDANGERED ENDANGERED ENDANGERED ENDANGERED ENDANGERED ENDANGERED ENDANGERED ENDANGERED ENDANGERED

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

// ENDANGERED ENDANGERED ENDANGERED ENDANGERED ENDANGERED ENDANGERED ENDANGERED ENDANGERED ENDANGERED ENDANGERED

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
							{  return ( FABSF( Operand ) );  }
inline float SquareRoot   ( float const Operand )
							{  return ( SQRTF( Operand ) );  }
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
							{  return ( ASINF( AngleInRadians ) );  }
inline float ACos         ( float const AngleInRadians )
							{  return ( ACOSF( AngleInRadians ) );  }
inline float ATan         ( float const AngleInRadians )
							{  return ( atanf( AngleInRadians ) );  }

#ifndef _GFLOAT_H
inline float Sin          ( float const AngleInRadians )
							{  return ( SINF( AngleInRadians ) );  }
inline float Cos          ( float const AngleInRadians )
							{  return ( COSF( AngleInRadians ) );  }
inline float Tan          ( float const AngleInRadians )
							{  return ( TANF( AngleInRadians ) );  }
#endif

//////////////////////////////////////////////////////////////////////////////

#endif  // __GPMATH_H

//////////////////////////////////////////////////////////////////////////////
