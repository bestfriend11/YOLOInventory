//////////////////////////////////////////////////////////////////////////////
//
// File     :  vector_3.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a simple god damn vector-3 class. No Set/Get crap,
//             no variable-length-if-we-ever-render-in-4D "flexibility",
//             and elimination of all temporaries and unnecessary copies!
//
//             For general math system docs see gpmath.h. This is important to
//             understand the _T convention and the RISC-style operations.
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
#ifndef __VECTOR_3_H
#define __VECTOR_3_H

//////////////////////////////////////////////////////////////////////////////

#include "GpMath.h"
#include "FuBiDefs.h"

#ifdef PARANOID_MATH
#define VALIDATE_VECTOR_3(v)					\
	gpassertf(									\
		(!_isnan((v).x) &&						\
		 !_isnan((v).y) &&						\
		 !_isnan((v).z)) 						\
		,("Illegal vector_3 [%f,%f,%f] "		\
		,(v).x,(v).y,(v).z))
#else
#define VALIDATE_VECTOR_3(v)
#endif

//////////////////////////////////////////////////////////////////////////////
// struct vector_3 declaration

struct vector_3
{
	FUBI_POD_CLASS( vector_3 );

	float x, y, z;

	static const vector_3 ZERO;			// 0, 0, 0
	static const vector_3 ONE;			// 1, 1, 1
	static const vector_3 INVALID;		// Inf, Inf, Inf
	static const vector_3 UNIT;			// unit vector

	// game-context abstraction
	static const vector_3 EAST;
	static const vector_3 UP;
	static const vector_3 NORTH;

// Ctors.

	         vector_3( void )                            {  x = 0.0f;  y = 0.0f;  z = 0.0f;  }			// TEMPORARY$$$ - initializes to zero
	         vector_3( do_not_initialize )               {  }											// TEMPORARY$$$ - this should be default ctor
	         vector_3( float x_, float y_, float z_ )    {  x = x_;  y = y_;  z = z_; VALIDATE_VECTOR_3( *this ) }
	explicit vector_3( float f )                         {  x = f;  y = f;  z = f; DEBUG_FLOAT_VALIDATE( f ) }
	explicit vector_3( const float* elements )           {  x = elements[0];  y = elements[1];  z = elements[2]; VALIDATE_VECTOR_3( *this ) }

	void Set( float x_, float y_, float z_ )  {  x = x_;  y = y_;  z = z_; VALIDATE_VECTOR_3( *this ) }
	void Set( float f )                       {  x = f;  y = f;  z = f; DEBUG_FLOAT_VALIDATE( f ) }
	void Set( const float* elements )         {  x = elements[0];  y = elements[1];  z = elements[2]; VALIDATE_VECTOR_3( *this ) }

// Operators.

	vector_3& operator += ( const vector_3& v )  {  x += v.x; y += v.y; z += v.z;  return ( *this );  }
	vector_3& operator += ( float f )            {  x += f;   y += f;   z += f;    return ( *this );  }
	vector_3& operator -= ( const vector_3& v )  {  x -= v.x; y -= v.y; z -= v.z;  return ( *this );  }
	vector_3& operator -= ( float f )            {  x -= f;   y -= f;   z -= f;    return ( *this );  }
	vector_3& operator *= ( const vector_3& v )  {  x *= v.x; y *= v.y; z *= v.z;  return ( *this );  }
	vector_3& operator *= ( float f )            {  x *= f;   y *= f;   z *= f;    return ( *this );  }
	vector_3& operator /= ( const vector_3& v )  {  x /= v.x; y /= v.y; z /= v.z;  return ( *this );  }
	vector_3& operator /= ( float f )            {  float inv = INVERSEF(f);  x *= inv;  y *= inv;  z *= inv;  return ( *this );  }
    vector_3  operator -  ( void ) const         {  return ( vector_3( -x, -y, -z ) );  }

	bool         operator == ( const vector_3& v) const  {  return (  IsEqual( v ) );  }
	bool         operator != ( const vector_3& v) const  {  return ( !IsEqual( v ) );  }
	const float& operator [] ( int n ) const             {  return ( (&x)[ n ] );  }
	float&       operator [] ( int n )                   {  return ( (&x)[ n ] );  }

// Other useful methods.

	inline float    DotProduct        ( const vector_3& v ) const;					// calculate dot product
	inline void     Project           ( vector_3& out, const vector_3& v ) const;	// project this vector on v and store in 'out'
	inline float    Length            ( void ) const;								// calculate vector length
	inline float    Length2           ( void ) const;								// calculate length squared
	inline float	ApproxLength      ( void ) const;								// approximate length (faster than Length())
	inline float    Distance2         ( const vector_3& v) const;					// calculate distance squared
	inline float    Distance          ( const vector_3& v) const;					// calculate distance
	inline bool     IsEqual           ( const vector_3& v) const;					// check if points are equal
	inline bool     IsEqual           ( const vector_3& v, float epsilon) const;	// check if points are equal
	inline bool		IsExactlyEqual	  ( const vector_3& v) const;					// check if points are exactly equal
	inline bool     IsZero            ( float epsilon = FLOAT_TOLERANCE ) const;	// compare to zero
	inline bool     IsZeroLength      ( float epsilon = FLOAT_TOLERANCE ) const;	// compare length2 to zero
	inline void     Round             ( void );										// round x y z to nearest integers
	inline void     DegreesToRadians  ( vector_3& out ) const;						// convert to radians and store in 'out'
	inline vector_3 DegreesToRadians_T( void ) const;								// convert to radians and store in 'out'
	inline void     RadiansToDegrees  ( vector_3& out ) const;						// convert to degrees and store in 'out'
	inline vector_3 RadiansToDegrees_T( void ) const;								// convert to degrees and store in 'out'

	inline void     Normalize         ( vector_3& out ) const;						// normalize to a unit vector and store in 'out'
	inline void     Normalize         ( void );										// normalize to a unit vector
	inline vector_3 Normalize_T       ( void ) const;								// normalize to a unit vector and store in 'out'

	inline void		RotateX		      ( float radians );							// rotate this vector around the X axis by the given radians
	inline void		RotateY           ( float radians );							// rotate this vector around the Y axis by the given radians
	inline void		RotateZ			  ( float radians );							// rotate this vector around the Z axis by the given radians

	inline vector_3	RotateX_T		  ( float radians ) const;						// rotate this vector around the X axis by the given radians and return result
	inline vector_3	RotateY_T         ( float radians ) const;						// rotate this vector around the Y axis by the given radians and return result
	inline vector_3	RotateZ_T		  ( float radians ) const;						// rotate this vector around the Z axis by the given radians and return result


// SIMD methods

	inline float	DotProduct_SIMD	  ( const vector_3& v ) const;
	inline vector_3	Multiply_SIMD	  ( const vector_3& r );
	inline vector_3 Multiply_SIMD	  ( float r );

// $$$ ENDANGERED $$$

	float        GetX( void ) const                {  return ( x );  }
	float        GetY( void ) const                {  return ( y );  }
	float        GetZ( void ) const                {  return ( z );  }
	void         SetX( float Value )               {  x = Value;  }
	void         SetY( float Value )               {  y = Value;  }
	void         SetZ( float Value )               {  z = Value;  }
	const float* GetElementsPointer( void ) const  {  return ( &x );  }
	float*       GetElementsPointer( void )        {  return ( &x );  }

// FuBi exports

	FUBI_EXPORT float FUBI_RENAME( GetX )( void ) const  {  return ( x );  }
	FUBI_EXPORT float FUBI_RENAME( GetY )( void ) const  {  return ( y );  }
	FUBI_EXPORT float FUBI_RENAME( GetZ )( void ) const  {  return ( z );  }

	FUBI_EXPORT void FUBI_RENAME( SetX )( float f )  {  x = f;  }
	FUBI_EXPORT void FUBI_RENAME( SetY )( float f )  {  y = f;  }
	FUBI_EXPORT void FUBI_RENAME( SetZ )( float f )  {  z = f;  }

	FUBI_EXPORT bool FUBI_RENAME( IsZero )( void ) const  {  return ( IsZero() );  }
};

//////////////////////////////////////////////////////////////////////////////
// vector_3 helper function declarations

// Operations.

inline float    DotProduct       ( const vector_3& v1, const vector_3& v2 );
inline float    DotProduct_SIMD  ( const vector_3& v1, const vector_3& v2 );
inline void     CrossProduct     ( vector_3& out, const vector_3& v1, const vector_3& v2 );
inline vector_3 CrossProduct_T   ( const vector_3& v1, const vector_3& v2 );
inline void     HadamardProduct  ( vector_3& out, const vector_3& v1, const vector_3& v2 );
inline vector_3 HadamardProduct_T( const vector_3& v1, const vector_3& v2 );
	   void     Slerp            ( vector_3& out, const vector_3& v0, const vector_3& v1, float t );
	   void     SlerpInPlace     ( vector_3& v0, const vector_3& v1, float t );
inline vector_3 Slerp_T          ( const vector_3& v0, const vector_3& v1, float t );

// Math operators.

	// $ note that these all return temporaries. the vector_3 is so small that
	//   it's probably going to inline just fine and not hurt performance.

inline vector_3 operator + ( const vector_3& l, const vector_3& r  );
inline vector_3 operator + ( const vector_3& l, float r );
inline vector_3 operator + ( float l, const vector_3& r );
inline vector_3 operator - ( const vector_3& l, const vector_3& r );
inline vector_3 operator - ( const vector_3& l, float r );
inline vector_3 operator - ( float l, const vector_3& r );
inline vector_3 operator * ( const vector_3& l, const vector_3& r );
inline vector_3 operator * ( const vector_3& l, float r );
inline vector_3 operator * ( float l, const vector_3& r );
inline vector_3 operator / ( const vector_3& l, const vector_3& r );
inline vector_3 operator / ( const vector_3& l, float r );
inline vector_3 operator / ( float l, const vector_3& r );

// Comparison operators.

	// $ note that all conditions must be satisfied and it is possible that
	//   A <= B and B <= A both return false

inline bool operator <  ( const vector_3& l, const vector_3& r );
inline bool operator >  ( const vector_3& l, const vector_3& r );
inline bool operator <= ( const vector_3& l, const vector_3& r );
inline bool operator >= ( const vector_3& l, const vector_3& r );

//////////////////////////////////////////////////////////////////////////////
// $$$ ENDANGERED $$$

// legacy code life support

inline float    InnerProduct   ( const vector_3& Operand1, const vector_3& Operand2 )
								 {  return ( DotProduct( Operand1, Operand2 ) );  }
inline vector_3 CrossProduct   ( const vector_3& Operand1, const vector_3& Operand2 )
								 {  return ( CrossProduct_T( Operand1, Operand2 ) );  }
inline vector_3 HadamardProduct( const vector_3& Operand1, const vector_3& Operand2 )
								 {  return ( Operand1 * Operand2 );  }
inline float    Length         ( const vector_3& Operand )
								 {  return ( Operand.Length() );  }
inline float    Length2        ( const vector_3& Operand )
								 {  return ( Operand.Length2() );  }
inline float	ApproxLength   ( const vector_3& Operand )
								 {  return ( Operand.ApproxLength() );  }
inline vector_3 Normalize      ( const vector_3& Operand )
								 {  return ( Operand.Normalize_T() );  }
inline bool     IsZero         ( const vector_3& TestVector, float Tolerance = FLOAT_TOLERANCE )
								 {  return ( TestVector.IsZero( Tolerance ) );  }

//////////////////////////////////////////////////////////////////////////////
// struct vector_3 inline implementation

inline float vector_3 :: DotProduct( const vector_3& v ) const
{
	VALIDATE_VECTOR_3(*this);
	VALIDATE_VECTOR_3(v);
	return ( x * v.x + y * v.y + z * v.z );
}

inline void vector_3 :: Project( vector_3& out, const vector_3& v ) const
{
	out = v * DotProduct( v );
}

inline float vector_3 :: Length( void ) const
{
	return ( SQRTF( Length2() ) );
}

inline float vector_3 :: Length2( void ) const
{
	VALIDATE_VECTOR_3(*this);
	return ( (x * x) + (y * y) + (z * z) );
}

inline float vector_3 :: ApproxLength( void ) const
{
	return ( FASTSQRTF( Length2() ) );
}

inline float vector_3 :: Distance2( const vector_3& v ) const
{
	VALIDATE_VECTOR_3(*this);
	VALIDATE_VECTOR_3(v);
	return ( (*this - v).Length2() );
}

inline float vector_3 :: Distance( const vector_3& v ) const
{
	VALIDATE_VECTOR_3(*this);
	VALIDATE_VECTOR_3(v);
	return ( (*this - v).Length() );
}

inline bool vector_3 :: IsEqual( const vector_3& v ) const
{
	return ( ::IsEqual( x, v.x ) && ::IsEqual( y, v.y ) && ::IsEqual( z, v.z ) );
}

inline bool vector_3 :: IsEqual( const vector_3& v, float epsilon ) const
{  return (   ::IsEqual( x, v.x, epsilon )
		   && ::IsEqual( y, v.y, epsilon )
		   && ::IsEqual( z, v.z, epsilon ) );
}

inline bool vector_3 :: IsExactlyEqual( const vector_3& v ) const
{
	return ( x == v.x && y == v.y && z == v.z );
}

inline bool vector_3 :: IsZero( float epsilon ) const
{
	return ( ::IsZero( x, epsilon ) && ::IsZero( y, epsilon ) && ::IsZero( z, epsilon ) );
}

inline bool vector_3 :: IsZeroLength( float epsilon ) const		// cheap inaccurate test
{
	return (   ::IsZero( x, epsilon )
			&& ::IsZero( y, epsilon )
			&& ::IsZero( z, epsilon ) );
}

inline void vector_3 :: Round( void )
{
	x = ::RoundToFloat( x );  y = ::RoundToFloat( y );  z = ::RoundToFloat( z );
}

inline void vector_3 :: DegreesToRadians( vector_3& out ) const
{
	VALIDATE_VECTOR_3(*this);
	out.x = ::DegreesToRadians( x );
	out.y = ::DegreesToRadians( y );
	out.z = ::DegreesToRadians( z );
}

inline vector_3 vector_3 :: DegreesToRadians_T( void ) const
{
	vector_3 temp( DoNotInitialize );
	DegreesToRadians( temp );
	return ( temp );
}

inline void vector_3 :: RadiansToDegrees( vector_3& out ) const
{
	VALIDATE_VECTOR_3(*this);
	out.x = ::RadiansToDegrees( x );
	out.y = ::RadiansToDegrees( y );
	out.z = ::RadiansToDegrees( z );
}

inline vector_3 vector_3 :: RadiansToDegrees_T( void ) const
{
	vector_3 temp( DoNotInitialize );
	RadiansToDegrees( temp );
	return ( temp );
}

inline void vector_3 :: Normalize( vector_3& out ) const
{

	VALIDATE_VECTOR_3(*this);

	// Note: It's safe to test len against 0.0f instead of using IsZero().  The
	// coprocessor handles very small numbers (on order of 1-e20) just fine.

	// ...but Normalizing a zero vector is garbage and worthy of an assertion --biddle

	gpassertf((*this)!=ZERO,("Attempting to Normalize a ZERO vector [%f,%f,%f]",x,y,z));

	float len = Length();
	DEBUG_FLOAT_VALIDATE( len );
	if ( len != 0.0f )
	{
		out = *this * INVERSEF(len);
	}
}

inline void vector_3 :: Normalize( void )
{
	Normalize( *this );
}

inline vector_3 vector_3 :: Normalize_T( void ) const
{
	vector_3 temp( DoNotInitialize );
	Normalize( temp );
	return ( temp );
}

inline void vector_3 :: RotateX( float radians )
{
	VALIDATE_VECTOR_3(*this);
	float sinrad, cosrad;
	SINCOSF( radians, sinrad, cosrad );

	float yt = ( y * cosrad ) + ( z * -sinrad );
	z = ( y * sinrad ) + ( z * cosrad );
	y = yt;
}

inline void vector_3 :: RotateY( float radians )
{
	VALIDATE_VECTOR_3(*this);
	float sinrad, cosrad;
	SINCOSF( radians, sinrad, cosrad );
	
	float xt = ( x * cosrad ) + ( z * sinrad );
	z = ( x * -sinrad ) + ( z * cosrad );
	x = xt;
}

inline void vector_3 :: RotateZ( float radians )
{
	VALIDATE_VECTOR_3(*this);
	float sinrad, cosrad;
	SINCOSF( radians, sinrad, cosrad );

	float xt = ( x * cosrad ) + ( y * -sinrad );
	y = ( x * sinrad ) + ( y * cosrad );
	x = xt;
}

inline vector_3 vector_3 :: RotateX_T( float radians ) const
{
	vector_3 temp( *this );
	temp.RotateX( radians );
	return ( temp );
}

inline vector_3 vector_3 :: RotateY_T( float radians ) const
{
	vector_3 temp( *this );
	temp.RotateY( radians );
	return ( temp );
}

inline vector_3 vector_3 :: RotateZ_T( float radians ) const
{
	vector_3 temp( *this );
	temp.RotateZ( radians );
	return ( temp );
}


//////////////////////////////////////////////////////////////////////////////
// vector_3 helper function inline implementations

inline float DotProduct( const vector_3& v1, const vector_3& v2 )
{
	VALIDATE_VECTOR_3(v1);
	VALIDATE_VECTOR_3(v2);
	return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
}

inline void CrossProduct( vector_3& out, const vector_3& v1, const vector_3& v2 )
{
	VALIDATE_VECTOR_3(v1);
	VALIDATE_VECTOR_3(v2);
	gpassert( (&out != &v1) && (&out != &v2) ); // $ cannot operate on self
	out.x = (v1.y * v2.z) - (v1.z * v2.y),
	out.y = (v1.z * v2.x) - (v1.x * v2.z),
	out.z = (v1.x * v2.y) - (v1.y * v2.x);
}

inline vector_3 CrossProduct_T( const vector_3& v1, const vector_3& v2 )
{
	VALIDATE_VECTOR_3(v1);
	VALIDATE_VECTOR_3(v2);
	vector_3 temp( DoNotInitialize );
	CrossProduct( temp, v1, v2 );
	return ( temp );
}

inline void HadamardProduct( vector_3& out, const vector_3& v1, const vector_3& v2 )
{
	VALIDATE_VECTOR_3(v1);
	VALIDATE_VECTOR_3(v2);
	out.x = v1.x * v2.x;
	out.y = v1.y * v2.y;
	out.z = v1.z * v2.z;
}

inline vector_3 HadamardProduct_T( const vector_3& v1, const vector_3& v2 )
{
	VALIDATE_VECTOR_3(v1);
	VALIDATE_VECTOR_3(v2);
	vector_3 temp( DoNotInitialize );
	HadamardProduct( temp, v1, v2 );
	return ( temp );
}

inline vector_3 Slerp_T( const vector_3& v1, const vector_3& v2, float t )
{
	VALIDATE_VECTOR_3(v1);
	VALIDATE_VECTOR_3(v2);
	vector_3 temp( DoNotInitialize );
	Slerp( temp, v1, v2, t );
	return ( temp );
}

inline vector_3 operator + ( const vector_3& l, const vector_3& r  )
{
	VALIDATE_VECTOR_3(l);
	VALIDATE_VECTOR_3(r);
	return ( vector_3( l.x + r.x, l.y + r.y, l.z + r.z ) );
}

inline vector_3 operator + ( const vector_3& l, float r )
{
	VALIDATE_VECTOR_3(l);
	DEBUG_FLOAT_VALIDATE(r);
	return ( vector_3( l.x + r, l.y + r, l.z + r ) );
}

inline vector_3 operator + ( float l, const vector_3& r )
{
	DEBUG_FLOAT_VALIDATE(l);
	VALIDATE_VECTOR_3(r);
	return ( vector_3( l + r.x, l + r.y, l + r.z ) );
}

inline vector_3 operator - ( const vector_3& l, const vector_3& r )
{
	VALIDATE_VECTOR_3(l);
	VALIDATE_VECTOR_3(r);
	return ( vector_3( l.x - r.x, l.y - r.y, l.z - r.z ) );
}

inline vector_3 operator - ( const vector_3& l, float r )
{ 
	VALIDATE_VECTOR_3(l);
	DEBUG_FLOAT_VALIDATE(r);
	return ( vector_3( l.x - r, l.y - r, l.z - r ) );
}

inline vector_3 operator - ( float l, const vector_3& r )
{ 
	DEBUG_FLOAT_VALIDATE(l);
	VALIDATE_VECTOR_3(r);
	return ( vector_3( l - r.x, l - r.y, l - r.z ) ); 
}

inline vector_3 operator * ( const vector_3& l, const vector_3& r )
{ 
	VALIDATE_VECTOR_3(l);
	VALIDATE_VECTOR_3(r);
	return ( vector_3( l.x * r.x, l.y * r.y, l.z * r.z ) );
}

inline vector_3 operator * ( const vector_3& l, float r )
{ 
	VALIDATE_VECTOR_3(l);
	DEBUG_FLOAT_VALIDATE(r);
	return ( vector_3( l.x * r, l.y * r, l.z * r ) ); 
}

inline vector_3 operator * ( float l, const vector_3& r )
{ 
	DEBUG_FLOAT_VALIDATE(l);
	VALIDATE_VECTOR_3(r);
	return ( vector_3( r.x * l, r.y * l, r.z * l ) ); 
}

inline vector_3 operator / ( const vector_3& l, const vector_3& r )
{ 
	VALIDATE_VECTOR_3(l);
	VALIDATE_VECTOR_3(r);
	return( vector_3( l.x / r.x, l.y / r.y, l.z / r.z ) );
}

inline vector_3 operator / ( const vector_3& l, float r )
{ 
	VALIDATE_VECTOR_3(l);
	float inv = INVERSEF(r);
	DEBUG_FLOAT_VALIDATE(inv);
	return ( vector_3( l.x * inv, l.y * inv, l.z * inv ) ); 
}

inline vector_3 operator / ( float l, const vector_3& r )
{ 
	return( vector_3( l / r.x, l / r.y, l / r.z ) );
}

inline bool operator < ( const vector_3& l, const vector_3& r )
{ 
	return ( ( l.x < r.x ) && ( l.y < r.y ) && ( l.z < r.z ) ); 
}

inline bool operator > ( const vector_3& l, const vector_3& r )
{ 
	return ( ( l.x > r.x ) && ( l.y > r.y ) && ( l.z > r.z ) ); 
}

inline bool operator <= ( const vector_3& l, const vector_3& r )
{ 
	return ( ( l.x <= r.x ) && ( l.y <= r.y ) && ( l.z <= r.z ) ); 
}

inline bool operator >= ( const vector_3& l, const vector_3& r )
{ 
	return ( ( l.x >= r.x ) && ( l.y >= r.y ) && ( l.z >= r.z ) ); 
}

//////////////////////////////////////////////////////////////////////////////

#endif  // __VECTOR_3_H

//////////////////////////////////////////////////////////////////////////////
