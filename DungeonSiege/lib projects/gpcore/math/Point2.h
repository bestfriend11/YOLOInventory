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

#pragma once
#ifndef __Point2_H
#define __Point2_H

//////////////////////////////////////////////////////////////////////////////

#include "GpMath.h"
#include "vector_3.h"
#include "FuBiDefs.h"
#include <map>

// 3dsmax has already defined a "Point2" --biddle
#if GP_SIEGEMAX
#define Point2 xPoint2 
#endif

//////////////////////////////////////////////////////////////////////////////
// struct Point2 declaration

struct Point2
{
	FUBI_POD_CLASS( Point2 );

	float x, y;

	static const Point2 ZERO;			// 0, 0
	static const Point2 ONE;			// 1, 1
	static const Point2 INVALID;		// Inf, Inf, Inf

// Ctors.

	         Point2( void )							{  x = 0.0f;  y = 0.0f; }			// TEMPORARY$$$ - initializes to zero
	         Point2( do_not_initialize )			{  }								// TEMPORARY$$$ - this should be default ctor
	         Point2( float x_, float y_ )			{  x = x_;  y = y_;     }
	         Point2( const vector_3& v_ )			{  x = v_.x;  y = v_.z; }
	explicit Point2( float f )						{  x = f;  y = f;       }
	explicit Point2( const float* elements )		{  x = elements[0];  y = elements[1]; }

	void Set( float x_, float y_ )            {  x = x_;  y = y_;  }
	void Set( float f )                       {  x = f;  y = f;    }
	void Set( const float* elements )         {  x = elements[0];  y = elements[1];  }

// Operators.

	Point2& operator += ( const Point2& v )  {  x += v.x; y += v.y; return ( *this );  }
	Point2& operator += ( float f )          {  x += f;   y += f;   return ( *this );  }
	Point2& operator -= ( const Point2& v )  {  x -= v.x; y -= v.y; return ( *this );  }
	Point2& operator -= ( float f )          {  x -= f;   y -= f;   return ( *this );  }
	Point2& operator *= ( const Point2& v )  {  x *= v.x; y *= v.y; return ( *this );  }
	Point2& operator *= ( float f )          {  x *= f;   y *= f;   return ( *this );  }
	Point2& operator /= ( const Point2& v )  {  x /= v.x; y /= v.y; return ( *this );  }
	Point2& operator /= ( float f )          {  x /= f;   y /= f;   return ( *this );  }
    Point2  operator -  ( void ) const       {  return ( Point2( -x, -y ) );  }

	bool         operator == ( const Point2& v) const  {  return (  IsEqual( v ) );  }
	bool         operator != ( const Point2& v) const  {  return ( !IsEqual( v ) );  }
	const float& operator [] ( int n ) const           {  return ( (&x)[ n ] );  }
	float&       operator [] ( int n )                 {  return ( (&x)[ n ] );  }

	// Other useful methods.

	inline float    DotProduct        ( const Point2& v ) const;					// calculate dot product
	inline void     Project           ( Point2& out, const Point2& v ) const;	// project this vector on v and store in 'out'
	inline float    Length            ( void ) const;								// calculate vector length
	inline float    Length2           ( void ) const;								// calculate length squared
	inline float    Distance2         ( const Point2& v) const;					// calculate distance squared
	inline float    Distance          ( const Point2& v) const;					// calculate distance
	inline bool     IsEqual           ( const Point2& v) const;					// check if points are equal
	inline bool     IsEqual           ( const Point2& v, float epsilon) const;	// check if points are equal
	inline bool		IsExactlyEqual	  ( const Point2& v) const;					// check if points are exactly equal
	inline bool     IsZero            ( float epsilon = FLOAT_TOLERANCE ) const;	// compare to zero
	inline bool     IsZeroLength      ( float epsilon = FLOAT_TOLERANCE ) const;	// compare length2 to zero
	inline void     Round             ( void );										// round x y z to nearest integers
	inline void     DegreesToRadians  ( Point2& out ) const;						// convert to radians and store in 'out'
	inline Point2 DegreesToRadians_T( void ) const;								// convert to radians and store in 'out'
	inline void     RadiansToDegrees  ( Point2& out ) const;						// convert to degrees and store in 'out'
	inline Point2 RadiansToDegrees_T( void ) const;								// convert to degrees and store in 'out'

	inline void     Normalize         ( Point2& out ) const;						// normalize to a unit vector and store in 'out'
	inline void     Normalize         ( void );										// normalize to a unit vector
	inline Point2 Normalize_T         ( void ) const;								// normalize to a unit vector and store in 'out'

// SIMD methods

/*
	inline float	DotProduct_SIMD	  ( const Point2& v ) const;
	inline Point2	Multiply_SIMD	  ( const Point2& r );
	inline Point2 Multiply_SIMD	  ( float r );
*/
// $$$ ENDANGERED $$$

	float        GetX( void ) const                {  return ( x );  }
	float        GetY( void ) const                {  return ( y );  }
	void         SetX( float Value )               {  x = Value;  }
	void         SetY( float Value )               {  y = Value;  }
	const float* GetElementsPointer( void ) const  {  return ( &x );  }
	float*       GetElementsPointer( void )        {  return ( &x );  }

// FuBi exports

	FUBI_EXPORT float FUBI_RENAME( GetX )( void ) const  {  return ( x );  }
	FUBI_EXPORT float FUBI_RENAME( GetY )( void ) const  {  return ( y );  }

	FUBI_EXPORT void FUBI_RENAME( SetX )( float f )  {  x = f;  }
	FUBI_EXPORT void FUBI_RENAME( SetY )( float f )  {  y = f;  }

	FUBI_EXPORT bool FUBI_RENAME( IsZero )( void ) const  {  return ( IsZero() );  }
};


//////////////////////////////////////////////////////////////////////////////
// Declare a Point2 sequence that maps time to a point2
struct TimedPoint {
	float m_Time;
	Point2 m_Point;
	TimedPoint (float t, const Point2& p) : m_Time(t), m_Point(p) {};
};

typedef std::vector< TimedPoint > PointSequence;
static const PointSequence EmptyPointSequence;

//////////////////////////////////////////////////////////////////////////////
// Point2 helper function declarations

// Operations.

inline float    DotProduct       ( const Point2& v1, const Point2& v2 );
inline float    DotProduct_SIMD  ( const Point2& v1, const Point2& v2 );
inline void     HadamardProduct  ( Point2& out, const Point2& v1, const Point2& v2 );
inline Point2 HadamardProduct_T  ( const Point2& v1, const Point2& v2 );
	   void     Slerp            ( Point2& out, const Point2& v0, const Point2& v1, float t );
inline Point2 Slerp_T            ( const Point2& v0, const Point2& v1, float t );

inline Point2 ChangeBasis(const Point2& p1, const Point2& p2);

Point2 Eigenvalues(const Point2 mat[2]);
bool EigenSystem(const Point2 mat[2], Point2& eigval, Point2& eigvec);

// Math operators.

	// $ note that these all return temporaries. the Point2 is so small that
	//   it's probably going to inline just fine and not hurt performance.

inline Point2 operator + ( const Point2& l, const Point2& r  );
inline Point2 operator + ( const Point2& l, float r );
inline Point2 operator + ( float l, const Point2& r );
inline Point2 operator - ( const Point2& l, const Point2& r );
inline Point2 operator - ( const Point2& l, float r );
inline Point2 operator - ( float l, const Point2& r );
inline Point2 operator * ( const Point2& l, const Point2& r );
inline Point2 operator * ( const Point2& l, float r );
inline Point2 operator * ( float l, const Point2& r );
inline Point2 operator / ( const Point2& l, const Point2& r );
inline Point2 operator / ( const Point2& l, float r );
inline Point2 operator / ( float l, const Point2& r );

// Comparison operators.

	// $ note that all conditions must be satisfied and it is possible that
	//   A <= B and B <= A both return false

inline bool operator <  ( const Point2& l, const Point2& r );
inline bool operator >  ( const Point2& l, const Point2& r );
inline bool operator <= ( const Point2& l, const Point2& r );
inline bool operator >= ( const Point2& l, const Point2& r );

//////////////////////////////////////////////////////////////////////////////
// $$$ ENDANGERED $$$

// legacy code life support

inline float    InnerProduct   ( const Point2& Operand1, const Point2& Operand2 )
								 {  return ( DotProduct( Operand1, Operand2 ) );  }
inline Point2 HadamardProduct  ( const Point2& Operand1, const Point2& Operand2 )
								 {  return ( Operand1 * Operand2 );  }
inline float    Length         ( const Point2& Operand )
								 {  return ( Operand.Length() );  }
inline float    Length2        ( const Point2& Operand )
								 {  return ( Operand.Length2() );  }
inline Point2 Normalize        ( const Point2& Operand )
								 {  return ( Operand.Normalize_T() );  }
inline bool     IsZero         ( const Point2& TestVector, float Tolerance = FLOAT_TOLERANCE )
								 {  return ( TestVector.IsZero( Tolerance ) );  }

//////////////////////////////////////////////////////////////////////////////
// struct Point2 inline implementation

inline float Point2 :: DotProduct( const Point2& v ) const
	{  return ( x * v.x + y * v.y );  }

inline void Point2 :: Project( Point2& out, const Point2& v ) const
	{  out = v * DotProduct( v );  }

inline float Point2 :: Length( void ) const
	{  return ( SQRTF( Length2() ) );  }

inline float Point2 :: Length2( void ) const
	{  return ( (x * x) + (y * y) );  }

inline float Point2 :: Distance2( const Point2& v ) const
	{  return ( (*this - v).Length2() );  }

inline float Point2 :: Distance( const Point2& v ) const
	{  return ( (*this - v).Length() );  }

inline bool Point2 :: IsEqual( const Point2& v ) const
	{  return ( ::IsEqual( x, v.x ) && ::IsEqual( y, v.y ) );  }

inline bool Point2 :: IsEqual( const Point2& v, float epsilon ) const
	{  return (   ::IsEqual( x, v.x, epsilon )
			   && ::IsEqual( y, v.y, epsilon ) );  }

inline bool Point2 :: IsExactlyEqual( const Point2& v ) const
	{  return ( x == v.x && y == v.y); }

inline bool Point2 :: IsZero( float epsilon ) const
	{  return ( ::IsZero( x, epsilon ) && ::IsZero( y, epsilon ) );  }

inline bool Point2 :: IsZeroLength( float epsilon ) const		// cheap inaccurate test
{
	return (   ::IsZero( x, epsilon )
			&& ::IsZero( y, epsilon ) );
}

inline void Point2 :: Round( void )
	{  x = ::RoundToFloat( x );  y = ::RoundToFloat( y );  }

inline void Point2 :: DegreesToRadians( Point2& out ) const
{
	out.x = ::DegreesToRadians( x );
	out.y = ::DegreesToRadians( y );
}

inline Point2 Point2 :: DegreesToRadians_T( void ) const
{
	Point2 temp( DoNotInitialize );
	DegreesToRadians( temp );
	return ( temp );
}

inline void Point2 :: RadiansToDegrees( Point2& out ) const
{
	out.x = ::RadiansToDegrees( x );
	out.y = ::RadiansToDegrees( y );
}

inline Point2 Point2 :: RadiansToDegrees_T( void ) const
{
	Point2 temp( DoNotInitialize );
	RadiansToDegrees( temp );
	return ( temp );
}

inline void Point2 :: Normalize( Point2& out ) const
{
	// Note: It's safe to test len against 0.0f instead of using IsZero().  The
	// coprocessor handles very small numbers (on order of 1-e20) just fine.

	// ...but Normalizing a zero vector is garbage and worthy of an assertion --biddle

	gpassertf((*this)!=ZERO,("Attempting to Normalize a ZERO vector [%f,%f,%f]",x,y));

	float len = Length();
	DEBUG_FLOAT_VALIDATE( len );
	if ( len != 0.0f )
	{
		out = *this * INVERSEF(len);
	}
}

inline void Point2 :: Normalize( void )
	{  Normalize( *this );  }

inline Point2 Point2 :: Normalize_T( void ) const
{
	Point2 temp( DoNotInitialize );
	Normalize( temp );
	return ( temp );
}

//////////////////////////////////////////////////////////////////////////////
// Point2 helper function inline implementations

inline float DotProduct( const Point2& v1, const Point2& v2 )
{
	return (v1.x * v2.x) + (v1.y * v2.y);
}

inline void HadamardProduct( Point2& out, const Point2& v1, const Point2& v2 )
{
	out.x = v1.x * v2.x;
	out.y = v1.y * v2.y;
}

inline Point2 HadamardProduct_T( const Point2& v1, const Point2& v2 )
{
	Point2 temp( DoNotInitialize );
	HadamardProduct( temp, v1, v2 );
	return ( temp );
}

inline Point2 Slerp_T( const Point2& v0, const Point2& v1, float t )
{
	Point2 temp( DoNotInitialize );
	Slerp( temp, v0, v1, t );
	return ( temp );
}


inline Point2 ChangeBasis(const Point2& p1, const Point2& p2)
{
	// returns p2 mapped into the orthonormal basis defined by <p1.x,p1.y>,<-p1.y,p1.x>
	// Fast 2d matrix multiply... without the whole matrix -biddle
	return Point2(p1.x*p2.x - p1.y*p2.y, p1.y*p2.x + p1.x*p2.y);
}

inline Point2 operator + ( const Point2& l, const Point2& r  )
  { return ( Point2( l.x + r.x, l.y + r.y ) ); }

inline Point2 operator + ( const Point2& l, float r )
  { return ( Point2( l.x + r, l.y + r ) ); }

inline Point2 operator + ( float l, const Point2& r )
  { return ( Point2( l + r.x, l + r.y ) ); }

inline Point2 operator - ( const Point2& l, const Point2& r )
  { return ( Point2( l.x - r.x, l.y - r.y ) ); }

inline Point2 operator - ( const Point2& l, float r )
  { return ( Point2( l.x - r, l.y - r ) ); }

inline Point2 operator - ( float l, const Point2& r )
  { return ( Point2( l - r.x, l - r.y ) ); }

inline Point2 operator * ( const Point2& l, const Point2& r )
  { return ( Point2( l.x * r.x, l.y * r.y ) ); }

inline Point2 operator * ( const Point2& l, float r )
  { return ( Point2( l.x * r, l.y * r ) ); }

inline Point2 operator * ( float l, const Point2& r )
  { return ( Point2( r.x * l, r.y * l ) ); }

inline Point2 operator / ( const Point2& l, const Point2& r )
  { return ( Point2( l.x / r.x, l.y / r.y ) ); }

inline Point2 operator / ( const Point2& l, float r )
  { return ( Point2( l.x / r, l.y / r ) ); }

inline Point2 operator / ( float l, const Point2& r )
  { return ( Point2( l / r.x, l / r.y ) ); }

inline bool operator < ( const Point2& l, const Point2& r )
  { return ( ( l.x < r.x ) && ( l.y < r.y ) ); }

inline bool operator > ( const Point2& l, const Point2& r )
  { return ( ( l.x > r.x ) && ( l.y > r.y ) ); }

inline bool operator <= ( const Point2& l, const Point2& r )
  { return ( ( l.x <= r.x ) && ( l.y <= r.y ) ); }

inline bool operator >= ( const Point2& l, const Point2& r )
  { return ( ( l.x >= r.x ) && ( l.y >= r.y ) ); }

//////////////////////////////////////////////////////////////////////////////


#endif  // __Point2_H

//////////////////////////////////////////////////////////////////////////////
