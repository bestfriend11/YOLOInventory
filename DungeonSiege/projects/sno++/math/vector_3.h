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

#include "gpmath.h"

//////////////////////////////////////////////////////////////////////////////
// struct vector_3 declaration

struct vector_3
{
	float x, y, z;

	static const vector_3 ZERO;			// 0, 0, 0
	static const vector_3 ONE;			// 1, 1, 1
	static const vector_3 INVALID;		// Inf, Inf, Inf

// Ctors.

	         vector_3( void )                            {  x = 0.0f;  y = 0.0f;  z = 0.0f;  }			// TEMPORARY$$$ - initializes to zero
	         vector_3( do_not_initialize )               {  }											// TEMPORARY$$$ - this should be default ctor
	         vector_3( float x_, float y_, float z_ )    {  x = x_;  y = y_;  z = z_;  }
	explicit vector_3( float f )                         {  x = f;  y = f;  z = f;  }
	explicit vector_3( const float* elements )           {  x = elements[0];  y = elements[1];  z = elements[2];  }

	void Set( float x_, float y_, float z_ )  {  x = x_;  y = y_;  z = z_;  }
	void Set( float f )                       {  x = f;  y = f;  z = f;  }
	void Set( const float* elements )         {  x = elements[0];  y = elements[1];  z = elements[2];  }

// Operators.

	vector_3& operator += ( const vector_3& v )  {  x += v.x; y += v.y; z += v.z;  return ( *this );  }
	vector_3& operator += ( float f )            {  x += f;   y += f;   z += f;    return ( *this );  }
	vector_3& operator -= ( const vector_3& v )  {  x -= v.x; y -= v.y; z -= v.z;  return ( *this );  }
	vector_3& operator -= ( float f )            {  x -= f;   y -= f;   z -= f;    return ( *this );  }
	vector_3& operator *= ( const vector_3& v )  {  x *= v.x; y *= v.y; z *= v.z;  return ( *this );  }
	vector_3& operator *= ( float f )            {  x *= f;   y *= f;   z *= f;    return ( *this );  }
	vector_3& operator /= ( const vector_3& v )  {  x /= v.x; y /= v.y; z /= v.z;  return ( *this );  }
	vector_3& operator /= ( float f )            {  x /= f;   y /= f;   z /= f;    return ( *this );  }
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
	inline float    Distance2         ( const vector_3& v) const;					// calculate distance squared
	inline float    Distance          ( const vector_3& v) const;					// calculate distance
	inline bool     IsEqual           ( const vector_3& v) const;					// check if points are equal
	inline bool     IsEqual           ( const vector_3& v, float epsilon) const;	// check if points are equal
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

// $$$ ENDANGERED $$$

	float        GetX( void ) const                {  return ( x );  }
	float        GetY( void ) const                {  return ( y );  }
	float        GetZ( void ) const                {  return ( z );  }
	void         SetX( float Value )               {  x = Value;  }
	void         SetY( float Value )               {  y = Value;  }
	void         SetZ( float Value )               {  z = Value;  }
	const float* GetElementsPointer( void ) const  {  return ( &x );  }
	float*       GetElementsPointer( void )        {  return ( &x );  }
};

//////////////////////////////////////////////////////////////////////////////
// vector_3 helper function declarations

// Operations.

inline float    DotProduct       ( const vector_3& v1, const vector_3& v2 );
inline void     CrossProduct     ( vector_3& out, const vector_3& v1, const vector_3& v2 );
inline vector_3 CrossProduct_T   ( const vector_3& v1, const vector_3& v2 );
inline void     HadamardProduct  ( vector_3& out, const vector_3& v1, const vector_3& v2 );
inline vector_3 HadamardProduct_T( const vector_3& v1, const vector_3& v2 );

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
inline vector_3 Normalize      ( const vector_3& Operand )
								 {  return ( Operand.Normalize_T() );  }
inline bool     IsZero         ( const vector_3& TestVector, float Tolerance = FLOAT_TOLERANCE )
								 {  return ( TestVector.IsZero( Tolerance ) );  }

//////////////////////////////////////////////////////////////////////////////
// struct vector_3 inline implementation

inline float vector_3 :: DotProduct( const vector_3& v ) const
	{  return ( (x * v.x) + (y * v.y) + (z * v.z) );  }

inline void vector_3 :: Project( vector_3& out, const vector_3& v ) const
	{  out = v * DotProduct( v );  }

inline float vector_3 :: Length( void ) const
	{  return ( sqrtf( Length2() ) );  }

inline float vector_3 :: Length2( void ) const
	{  return ( (x * x) + (y * y) + (z * z) );  }

inline float vector_3 :: Distance2( const vector_3& v ) const
	{  return ( (*this - v).Length2() );  }

inline float vector_3 :: Distance( const vector_3& v ) const
	{  return ( (*this - v).Length() );  }

inline bool vector_3 :: IsEqual( const vector_3& v ) const
	{  return ( ::IsEqual( x, v.x ) && ::IsEqual( y, v.y ) && ::IsEqual( z, v.z ) );  }

inline bool vector_3 :: IsEqual( const vector_3& v, float epsilon ) const
	{  return (   ::IsEqual( x, v.x, epsilon )
			   && ::IsEqual( y, v.y, epsilon )
			   && ::IsEqual( z, v.z, epsilon ) );  }

inline bool vector_3 :: IsZero( float epsilon ) const
	{  return ( ::IsZero( x, epsilon ) && ::IsZero( y, epsilon ) && ::IsZero( z, epsilon ) );  }

inline bool vector_3 :: IsZeroLength( float epsilon ) const		// cheap inaccurate test
{
	return (   ::IsZero( x, epsilon )
			&& ::IsZero( y, epsilon )
			&& ::IsZero( z, epsilon ) );
}

inline void vector_3 :: Round( void )
	{  x = ::Round( x );  y = ::Round( y );  z = ::Round( z );  }

inline void vector_3 :: DegreesToRadians( vector_3& out ) const
{
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
	// Note: It's safe to test len against 0.0f instead of using IsZero().  The
	// coprocessor handles very small numbers (on order of 1-e20) just fine.

	float len = Length();
	if ( len != 0.0f )
	{
		out = *this * (1.0f / len);
	}
}

inline void vector_3 :: Normalize( void )
	{  Normalize( *this );  }

inline vector_3 vector_3 :: Normalize_T( void ) const
{
	vector_3 temp( DoNotInitialize );
	Normalize( temp );
	return ( temp );
}

//////////////////////////////////////////////////////////////////////////////
// vector_3 helper function inline implementations

inline float DotProduct( const vector_3& v1, const vector_3& v2 )
	{  return ( v1.DotProduct( v2 ) );  }

inline void CrossProduct( vector_3& out, const vector_3& v1, const vector_3& v2 )
{
	gpassert( (&out != &v1) && (&out != &v2) ); // $ cannot operate on self
	out.x = (v1.y * v2.z) - (v1.z * v2.y),
	out.y = (v1.z * v2.x) - (v1.x * v2.z),
	out.z = (v1.x * v2.y) - (v1.y * v2.x);
}

inline vector_3 CrossProduct_T( const vector_3& v1, const vector_3& v2 )
{
	vector_3 temp( DoNotInitialize );
	CrossProduct( temp, v1, v2 );
	return ( temp );
}

inline void HadamardProduct( vector_3& out, const vector_3& v1, const vector_3& v2 )
{
	out.x = v1.x * v2.x;
	out.y = v1.y * v2.y;
	out.z = v1.z * v2.z;
}

inline vector_3 HadamardProduct_T( const vector_3& v1, const vector_3& v2 )
{
	vector_3 temp( DoNotInitialize );
	HadamardProduct( temp, v1, v2 );
	return ( temp );
}

inline vector_3 operator + ( const vector_3& l, const vector_3& r  )
  { return ( vector_3( l.x + r.x, l.y + r.y, l.z + r.z ) ); }

inline vector_3 operator + ( const vector_3& l, float r )
  { return ( vector_3( l.x + r, l.y + r, l.z + r ) ); }

inline vector_3 operator + ( float l, const vector_3& r )
  { return ( vector_3( l + r.x, l + r.y, l + r.z ) ); }

inline vector_3 operator - ( const vector_3& l, const vector_3& r )
  { return ( vector_3( l.x - r.x, l.y - r.y, l.z - r.z ) ); }

inline vector_3 operator - ( const vector_3& l, float r )
  { return ( vector_3( l.x - r, l.y - r, l.z - r ) ); }

inline vector_3 operator - ( float l, const vector_3& r )
  { return ( vector_3( l - r.x, l - r.y, l - r.z ) ); }

inline vector_3 operator * ( const vector_3& l, const vector_3& r )
  { return ( vector_3( l.x * r.x, l.y * r.y, l.z * r.z ) ); }

inline vector_3 operator * ( const vector_3& l, float r )
  { return ( vector_3( l.x * r, l.y * r, l.z * r ) ); }

inline vector_3 operator * ( float l, const vector_3& r )
  { return ( vector_3( l * r.x, l * r.y, l * r.z ) ); }

inline vector_3 operator / ( const vector_3& l, const vector_3& r )
  { return ( vector_3( l.x / r.x, l.y / r.y, l.z / r.z ) ); }

inline vector_3 operator / ( const vector_3& l, float r )
  { return ( vector_3( l.x / r, l.y / r, l.z / r ) ); }

inline vector_3 operator / ( float l, const vector_3& r )
  { return ( vector_3( l / r.x, l / r.y, l / r.z ) ); }

inline bool operator < ( const vector_3& l, const vector_3& r )
  { return ( ( l.x < r.x ) && ( l.y < r.y ) && ( l.z < r.z ) ); }

inline bool operator > ( const vector_3& l, const vector_3& r )
  { return ( ( l.x > r.x ) && ( l.y > r.y ) && ( l.z > r.z ) ); }

inline bool operator <= ( const vector_3& l, const vector_3& r )
  { return ( ( l.x <= r.x ) && ( l.y <= r.y ) && ( l.z <= r.z ) ); }

inline bool operator >= ( const vector_3& l, const vector_3& r )
  { return ( ( l.x >= r.x ) && ( l.y >= r.y ) && ( l.z >= r.z ) ); }

//////////////////////////////////////////////////////////////////////////////

#endif  // __VECTOR_3_H

//////////////////////////////////////////////////////////////////////////////
