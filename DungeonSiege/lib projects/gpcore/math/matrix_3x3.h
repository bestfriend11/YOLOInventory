//////////////////////////////////////////////////////////////////////////////
//
// File     :  matrix_3x3.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a simple god damn 3x3 matrix class. No Set/Get crap,
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
#ifndef __MATRIX_3X3_H
#define __MATRIX_3X3_H

//////////////////////////////////////////////////////////////////////////////

#include "vector_3.h"

//////////////////////////////////////////////////////////////////////////////
// struct matrix_3x3 declaration

struct matrix_3x3
{
	float v00, v01, v02;
	float v10, v11, v12;
	float v20, v21, v22;

	static const float ZERO_INIT    [9];
	static const float IDENTITY_INIT[9];

	static const matrix_3x3 ZERO;			// all zeros
	static const matrix_3x3 IDENTITY;		// all zeros but 1 at 0,0, 1,1 2,2

// Ctors.

	         matrix_3x3( void )					  {  ::memcpy( this, IDENTITY_INIT, sizeof( *this ) );  }	// TEMPORARY$$$ - initializes to identity matrix
	         matrix_3x3( do_not_initialize )	  {  }														// TEMPORARY$$$ - this should be default ctor
	explicit matrix_3x3( const float* elements )  {  ::memcpy( this, elements, sizeof( *this ) );  }

// Operators.

	matrix_3x3& operator += ( const matrix_3x3& m );
	matrix_3x3& operator -= ( const matrix_3x3& m );

// Row/column accessors.

	void			  GetColumn0( vector_3& out ) const  {  out.x = v00;  out.y = v10;  out.z = v20;  }
	void			  GetColumn1( vector_3& out ) const  {  out.x = v01;  out.y = v11;  out.z = v21;  }
	void			  GetColumn2( vector_3& out ) const  {  out.x = v02;  out.y = v12;  out.z = v22;  }
	inline vector_3	  GetColumn0_T( void ) const;
	inline vector_3	  GetColumn1_T( void ) const;
	inline vector_3	  GetColumn2_T( void ) const;
	inline void		  GetColumns( vector_3& c0, vector_3& c1, vector_3& c2 ) const;

	void			  SetColumn0( const vector_3& v )  {  v00 = v.x;  v10 = v.y;  v20 = v.z;  }
	void			  SetColumn1( const vector_3& v )  {  v01 = v.x;  v11 = v.y;  v21 = v.z;  }
	void			  SetColumn2( const vector_3& v )  {  v02 = v.x;  v12 = v.y;  v22 = v.z;  }
	inline void		  SetColumns( const vector_3& c0, const vector_3& c1, const vector_3& c2 );

	vector_3&		  GetRow0( void )               {  return ( *(vector_3*)&v00 );  }
	vector_3&		  GetRow1( void )               {  return ( *(vector_3*)&v10 );  }
	vector_3&		  GetRow2( void )               {  return ( *(vector_3*)&v20 );  }
	const vector_3&	  GetRow0( void ) const         {  return ( *(vector_3*)&v00 );  }
	const vector_3&	  GetRow1( void ) const         {  return ( *(vector_3*)&v10 );  }
	const vector_3&	  GetRow2( void ) const         {  return ( *(vector_3*)&v20 );  }
	void			  GetRow0( vector_3& v ) const  {  v = GetRow0();  }
	void			  GetRow1( vector_3& v ) const  {  v = GetRow1();  }
	void			  GetRow2( vector_3& v ) const  {  v = GetRow2();  }
	inline void		  GetRows( vector_3& r0, vector_3& r1, vector_3& r2 ) const;

	void			  SetRow0( const vector_3& v )  {  GetRow0() = v;  }
	void			  SetRow1( const vector_3& v )  {  GetRow1() = v;  }
	void			  SetRow2( const vector_3& v )  {  GetRow2() = v;  }
	inline void       SetRows( const vector_3& r0, const vector_3& r1, const vector_3& r2 );

// Other useful methods.

	void        	  Transform    ( vector_3* out, const vector_3* src,			// transform the points
					  		         int count, int srcSize = sizeof( vector_3 ) ) const;
	inline void 	  Transform    ( vector_3& out, const vector_3& v ) const;		// transform one point
	inline void 	  Transform    ( vector_3& v ) const;							// transform one point in place
	inline void 	  Transform    ( vector_3* v, int count ) const;				// simple multi-transform in place
	inline vector_3   Transform_T  ( const vector_3& v ) const;						// return transformed vector
	inline void		  Transform_SIMD( vector_3* out, const vector_3* src );

	float			  Determinant  ( void ) const;									// determine determinant

	void			  Transpose    ( matrix_3x3& out ) const;						// store transpose in out
	inline void		  Transpose    ( void );										// transpose self ($ special: this one is _faster_ than other form)
	inline matrix_3x3 Transpose_T  ( void ) const;									// return transpose of self

	void			  Orthonormal  ( matrix_3x3& out ) const;						// store orthonormal in out
	inline void		  Orthonormal  ( void );										// orthonormalize self
	inline matrix_3x3 Orthonormal_T( void ) const;									// return orthonormalized copy of self

	void			  Invert       ( matrix_3x3& out ) const;						// store inverse in out
	inline void		  Invert       ( void );										// invert self
	inline matrix_3x3 Invert_T     ( void ) const;									// return inverse

	void			  Negate       ( matrix_3x3& out ) const;						// store inverse in out
	inline void		  Negate       ( void );										// invert self
	inline matrix_3x3 Negate_T     ( void ) const;									// return inverse

	bool			  IsOrthonormal( float epsilon = FLOAT_TOLERANCE ) const;		// normalized orthogonal matrix
	bool			  IsEqual      ( const matrix_3x3& out,							// close-enough compare to other matrix
									 float epsilon = FLOAT_TOLERANCE ) const;
	inline bool		  IsEqualExact ( const matrix_3x3& out ) const;					// exact compare to other matrix
	inline bool		  IsZero       ( float epsilon = FLOAT_TOLERANCE ) const;		// compare to zero
	inline bool		  IsIdentity   ( float epsilon = FLOAT_TOLERANCE ) const;		// compare to identity matrix
	inline bool		  IsSymmetric  ( void ) const;									// is matrix symmetric?

// $$$ ENDANGERED $$$

	float        GetElement_00     ( void ) const             {  return ( v00 );  }
	float        GetElement_01     ( void ) const             {  return ( v01 );  }
	float        GetElement_02     ( void ) const             {  return ( v02 );  }
	float        GetElement_10     ( void ) const             {  return ( v10 );  }
	float        GetElement_11     ( void ) const             {  return ( v11 );  }
	float        GetElement_12     ( void ) const             {  return ( v12 );  }
	float        GetElement_20     ( void ) const             {  return ( v20 );  }
	float        GetElement_21     ( void ) const             {  return ( v21 );  }
	float        GetElement_22     ( void ) const             {  return ( v22 );  }
	vector_3     GetColumn_0       ( void ) const             {  vector_3 temp;  GetColumn0( temp );  return ( temp );  }
	vector_3     GetColumn_1       ( void ) const             {  vector_3 temp;  GetColumn1( temp );  return ( temp );  }
	vector_3     GetColumn_2       ( void ) const             {  vector_3 temp;  GetColumn2( temp );  return ( temp );  }
	vector_3     GetRow_0          ( void ) const             {  return ( GetRow0() );  }
	vector_3     GetRow_1          ( void ) const             {  return ( GetRow1() );  }
	vector_3     GetRow_2          ( void ) const             {  return ( GetRow2() );  }
	void         SetElement_00     ( float Value )            {  v00 = Value;  }
	void         SetElement_01     ( float Value )            {  v01 = Value;  }
	void         SetElement_02     ( float Value )            {  v02 = Value;  }
	void         SetElement_10     ( float Value )            {  v10 = Value;  }
	void         SetElement_11     ( float Value )            {  v11 = Value;  }
	void         SetElement_12     ( float Value )            {  v12 = Value;  }
	void         SetElement_20     ( float Value )            {  v20 = Value;  }
	void         SetElement_21     ( float Value )            {  v21 = Value;  }
	void         SetElement_22     ( float Value )            {  v22 = Value;  }
	void         SetRow_0          ( const vector_3& Value )  {  SetRow0( Value );  }
	void         SetRow_1          ( const vector_3& Value )  {  SetRow1( Value );  }
	void         SetRow_2          ( const vector_3& Value )  {  SetRow2( Value );  }
	void         SetColumn_0       ( const vector_3& Value )  {  SetColumn0( Value );  }
	void         SetColumn_1       ( const vector_3& Value )  {  SetColumn1( Value );  }
	void         SetColumn_2       ( const vector_3& Value )  {  SetColumn2( Value );  }
	const float* GetElementsPointer( void ) const             {  return ( &v00 );  }
	float*       GetElementsPointer( void )                   {  return ( &v00 );  }
};

//////////////////////////////////////////////////////////////////////////////
// matrix_3x3 helper function declarations

// Construction helpers.

inline void       MatrixFromColumns  ( matrix_3x3& out, const vector_3& c0, const vector_3& c1, const vector_3& c2 );
inline matrix_3x3 MatrixFromColumns_T( const vector_3& c0, const vector_3& c1, const vector_3& c2 );
inline void       MatrixFromRows     ( matrix_3x3& out, const vector_3& r0, const vector_3& r1, const vector_3& r2 );
inline matrix_3x3 MatrixFromRows_T   ( const vector_3& r0, const vector_3& r1, const vector_3& r2 );

// High school math.

void              Multiply		( matrix_3x3& out, const matrix_3x3& l, const matrix_3x3& r );
inline matrix_3x3 Multiply_T	( const matrix_3x3& l, const matrix_3x3& r );
void              Multiply		( matrix_3x3& out, const matrix_3x3& l, float r );
inline matrix_3x3 Multiply_T	( const matrix_3x3& l, float r );
inline void       Multiply		( matrix_3x3& out, float l, const matrix_3x3& r );
inline matrix_3x3 Multiply_T	( float l, const matrix_3x3& r );
inline void		  Multiply_SIMD	( matrix_3x3& out, const matrix_3x3& l, const matrix_3x3& r );
void              Add			( matrix_3x3& out, const matrix_3x3& l, const matrix_3x3& r );
inline matrix_3x3 Add_T			( const matrix_3x3& l, const matrix_3x3& r );
void              Subtract		( matrix_3x3& out, const matrix_3x3& l, const matrix_3x3& r );
inline matrix_3x3 Subtract_T	( const matrix_3x3& l, const matrix_3x3& r );

// College math.

	// From Numerical Recipes in C - used to find eigenstuff
void JacobiTransform(matrix_3x3 &a, matrix_3x3 &v, vector_3 &d);

	// Returns a number of distinct eigenvalues - values are stored in
	//  eigenvalues reference parameter $$$ NOT CURRENTLY IMPLEMENTED
//int Eigenvalues ( vector_3& eigenvalues, const matrix_3x3& m );

	// Returns a number of unique eigenvectors, vectors are stored in
	//  eigenvectors matrix
int Eigenvectors( matrix_3x3& eigenvectors, const matrix_3x3& m );

//////////////////////////////////////////////////////////////////////////////
// $$$ ENDANGERED $$$

inline matrix_3x3 operator *    ( const matrix_3x3& l, const matrix_3x3& r )
								  {  return ( Multiply_T( l, r ) );  }
inline matrix_3x3 operator *    ( const matrix_3x3& l, float r )
								  {  return ( Multiply_T( l, r ) );  }
inline matrix_3x3 operator *    ( float l, const matrix_3x3& r )
								  {  return ( Multiply_T( l, r ) );  }
inline matrix_3x3 operator +    ( const matrix_3x3& l, const matrix_3x3& r )
								  {  return ( Add_T( l, r ) );  }
inline matrix_3x3 operator -    ( const matrix_3x3& l, const matrix_3x3& r )
								  {  return ( Subtract_T( l, r ) );  }
inline matrix_3x3 operator -    ( const matrix_3x3& m )
								  {  return ( m.Negate_T() );  }
inline vector_3   operator *    ( const matrix_3x3& l, const vector_3& r )
								  {  return ( l.Transform_T( r ) );  }
inline matrix_3x3 Transpose     ( const matrix_3x3& m )
								  {  return ( m.Transpose_T() );  }
inline float      Determinant   ( const matrix_3x3& m )
								  {  return ( m.Determinant() );  }
inline matrix_3x3 Inverse       ( const matrix_3x3& m )
								  {  return ( m.Invert_T() );  }
inline matrix_3x3 Orthonormalize( const matrix_3x3& m )
								  {  return ( m.Orthonormal_T() );  }
inline bool       IsOrthonormal ( const matrix_3x3& m, float epsilon = FLOAT_TOLERANCE )
								  {  return ( m.IsOrthonormal( epsilon ) );  }
inline bool       IsIdentity    ( const matrix_3x3& m, float epsilon = FLOAT_TOLERANCE )
								  {  return ( m.IsIdentity( epsilon ) );  }
inline bool       IsZero        ( const matrix_3x3& m, float epsilon = FLOAT_TOLERANCE )
								  {  return ( m.IsZero( epsilon ) );  }
inline matrix_3x3 ZeroMatrix    ( void )
								  {  return ( matrix_3x3::ZERO );  }
inline matrix_3x3 MatrixColumns ( const vector_3& c0, const vector_3& c1, const vector_3& c2 )
								  {  return ( MatrixFromColumns_T( c0, c1, c2 ) );  }
inline matrix_3x3 MatrixRows    ( const vector_3& r0, const vector_3& r1, const vector_3& r2 )
								  {  return ( MatrixFromRows_T( r0, r1, r2 ) );  }

//////////////////////////////////////////////////////////////////////////////
// struct matrix_3x3 inline implementation

inline vector_3 matrix_3x3 :: GetColumn0_T( void ) const
{
	vector_3 temp( DoNotInitialize );
	GetColumn0( temp );
	return ( temp );
}

inline vector_3 matrix_3x3 :: GetColumn1_T( void ) const
{
	vector_3 temp( DoNotInitialize );
	GetColumn1( temp );
	return ( temp );
}

inline vector_3 matrix_3x3 :: GetColumn2_T( void ) const
{
	vector_3 temp( DoNotInitialize );
	GetColumn2( temp );
	return ( temp );
}

inline void matrix_3x3 :: GetColumns( vector_3& c0, vector_3& c1, vector_3& c2 ) const
	{  GetColumn0( c0 );  GetColumn1( c1 );  GetColumn2( c2 );  }

inline void matrix_3x3 :: SetColumns( const vector_3& c0, const vector_3& c1, const vector_3& c2 )
	{  SetColumn0( c0 );  SetColumn1( c1 );  SetColumn2( c2 );  }

inline void matrix_3x3 :: GetRows( vector_3& r0, vector_3& r1, vector_3& r2 ) const
	{  GetRow0( r0 );  GetRow1( r1 );  GetRow2( r2 );  }

inline void matrix_3x3 :: SetRows( const vector_3& r0, const vector_3& r1, const vector_3& r2 )
    {  SetRow0( r0 );  SetRow1( r1 );  SetRow2( r2 );  }

inline void matrix_3x3 :: Transform( vector_3& out, const vector_3& v ) const
	{  Transform( &out, &v, 1 );  }

inline void matrix_3x3 :: Transform( vector_3& v ) const
	{  Transform( v, v );  }

inline void matrix_3x3 :: Transform( vector_3* v, int count ) const
	{  Transform( v, v, count );  }

inline vector_3 matrix_3x3 :: Transform_T( const vector_3& v ) const
{
	vector_3 temp( DoNotInitialize );
	Transform( temp, v );
	return ( temp );
}

inline void matrix_3x3 :: Transpose( void )
{
	std::swap( v01, v10 );
	std::swap( v02, v20 );
	std::swap( v12, v21 );
}

inline matrix_3x3 matrix_3x3 :: Transpose_T( void ) const
{
	matrix_3x3 temp( DoNotInitialize );
	Transpose( temp );
	return ( temp );
}

inline void matrix_3x3 :: Orthonormal( void )
{
	matrix_3x3 temp( DoNotInitialize );
	Orthonormal( temp );
	*this = temp;
}

inline matrix_3x3 matrix_3x3 :: Orthonormal_T( void ) const
{
	matrix_3x3 temp( DoNotInitialize );
	Orthonormal( temp );
	return ( temp );
}

inline void matrix_3x3 :: Invert( void )
{
	matrix_3x3 temp( DoNotInitialize );
	Invert( temp );
	*this = temp;
}

inline matrix_3x3 matrix_3x3 :: Invert_T( void ) const
{
	matrix_3x3 temp( DoNotInitialize );
	Invert( temp );
	return ( temp );
}

inline void matrix_3x3 :: Negate( void )
	{  Negate( *this );  }

inline matrix_3x3 matrix_3x3 :: Negate_T( void ) const
{
	matrix_3x3 temp( DoNotInitialize );
	Negate( temp );
	return ( temp );
}

inline bool matrix_3x3 :: IsEqualExact( const matrix_3x3& out ) const
	{  return ( ::memcmp( this, &out, sizeof( *this ) ) == 0 );  }

inline bool matrix_3x3 :: IsZero( float epsilon ) const
	{  return ( IsEqual( ZERO, epsilon ) );  }

inline bool matrix_3x3 :: IsIdentity( float epsilon ) const
	{  return ( IsEqual( IDENTITY, epsilon ) );  }

inline bool matrix_3x3 :: IsSymmetric( void ) const
	{  return ( (v10 == v01) && (v20 == v02) && (v21 == v12) );  }

//////////////////////////////////////////////////////////////////////////////
// matrix_3x3 helper function inline implementations

inline void MatrixFromColumns( matrix_3x3& out, const vector_3& c0, const vector_3& c1, const vector_3& c2 )
	{  out.SetColumns( c0, c1, c2 );  }

inline matrix_3x3 MatrixFromColumns_T( const vector_3& c0, const vector_3& c1, const vector_3& c2 )
{
	matrix_3x3 temp( DoNotInitialize );
	MatrixFromColumns( temp, c0, c1, c2 );
	return ( temp );
}

inline void MatrixFromRows( matrix_3x3& out, const vector_3& r0, const vector_3& r1, const vector_3& r2 )
	{  out.SetRows( r0, r1, r2 );  }

inline matrix_3x3 MatrixFromRows_T( const vector_3& r0, const vector_3& r1, const vector_3& r2 )
{
	matrix_3x3 temp( DoNotInitialize );
	MatrixFromRows( temp, r0, r1, r2 );
	return ( temp );
}

inline matrix_3x3 Multiply_T( const matrix_3x3& l, const matrix_3x3& r )
{
	matrix_3x3 temp( DoNotInitialize );
	Multiply( temp, l, r );
	return ( temp );
}

inline matrix_3x3 Multiply_T( const matrix_3x3& l, float r )
{
	matrix_3x3 temp( DoNotInitialize );
	Multiply( temp, l, r );
	return ( temp );
}

inline void Multiply( matrix_3x3& out, float l, const matrix_3x3& r )
{
	Multiply( out, r, l );
}

inline matrix_3x3 Multiply_T( float l, const matrix_3x3& r )
{
	matrix_3x3 temp( DoNotInitialize );
	Multiply( temp, l, r );
	return ( temp );
}

inline matrix_3x3 Add_T( const matrix_3x3& l, const matrix_3x3& r )
{
	matrix_3x3 temp( DoNotInitialize );
	Add( temp, l, r );
	return ( temp );
}

inline matrix_3x3 Subtract_T( const matrix_3x3& l, const matrix_3x3& r )
{
	matrix_3x3 temp( DoNotInitialize );
	Subtract( temp, l, r );
	return ( temp );
}

//////////////////////////////////////////////////////////////////////////////

#endif  // __MATRIX_3X3_H

//////////////////////////////////////////////////////////////////////////////



#if 0

/* ************************************************************************
   Function: OuterProduct
   Description: Returns the outer product of two vectors, with the first
                operand assumed to be a row vector
   ************************************************************************ */
inline matrix_3x3
OuterProduct(vector_3 const &Operand1, vector_3 const &Operand2)
{
	matrix_3x3 Result(DoNotInitialize);

	Result.SetElement_00( Operand1.GetX() * Operand2.GetX() );
	Result.SetElement_01( Operand1.GetX() * Operand2.GetY() );
	Result.SetElement_02( Operand1.GetX() * Operand2.GetZ() );

	Result.SetElement_10( Operand1.GetY() * Operand2.GetX() );
	Result.SetElement_11( Operand1.GetY() * Operand2.GetY() );
	Result.SetElement_12( Operand1.GetY() * Operand2.GetZ() );

	Result.SetElement_20( Operand1.GetZ() * Operand2.GetX() );
	Result.SetElement_21( Operand1.GetZ() * Operand2.GetY() );
	Result.SetElement_22( Operand1.GetZ() * Operand2.GetZ() );

	return(Result);
}


/* ************************************************************************
   Function: SkewSymmetricMatrix
   Description: Returns a skew-symmetric matrix defined as performing the
                cross-product of the given vector with any column vector
   ************************************************************************ */
inline matrix_3x3
SkewSymmetricMatrix(vector_3 const &CrossVector)
{
	matrix_3x3 SkewSymmetricMatrix(DoNotInitialize);

	SkewSymmetricMatrix.SetElement_00( RealZero );
	SkewSymmetricMatrix.SetElement_01( -CrossVector.GetZ() );
	SkewSymmetricMatrix.SetElement_02( CrossVector.GetY() );

	SkewSymmetricMatrix.SetElement_10( CrossVector.GetZ() );
	SkewSymmetricMatrix.SetElement_11( RealZero );
	SkewSymmetricMatrix.SetElement_12( -CrossVector.GetX() );

	SkewSymmetricMatrix.SetElement_20( -CrossVector.GetY() );
	SkewSymmetricMatrix.SetElement_21( CrossVector.GetX() );
	SkewSymmetricMatrix.SetElement_22( RealZero );

	return(SkewSymmetricMatrix);
}

/* ************************************************************************
   Function: DiagonalMatrix
   Description: Returns a matrix with the given element repeated along the
                diagonal
   ************************************************************************ */
inline matrix_3x3
DiagonalMatrix(float const DiagonalElement)
{
	return(matrix_3x3() * DiagonalElement);
}

/* ************************************************************************
   Function: DiagonalMatrix
   Description: Returns a matrix with the given vector on the diagonal
   ************************************************************************ */
inline matrix_3x3
DiagonalMatrix(vector_3 const &DiagonalElements)
{
	matrix_3x3 Result(ZeroMatrix());
	
	Result.SetElement_00( DiagonalElements.GetX() );
	Result.SetElement_11( DiagonalElements.GetY() );
	Result.SetElement_22( DiagonalElements.GetZ() );
	
	return(Result);
}

/* ************************************************************************
   Function: DiagonalMatrix
   Description: Returns a matrix with the given elements on the diagonal
   ************************************************************************ */
inline matrix_3x3
DiagonalMatrix(float const Element00,
			   float const Element11,
			   float const Element22)
{
	matrix_3x3 Result(ZeroMatrix());
	
	Result.SetElement_00( Element00 );
	Result.SetElement_11( Element11 );
	Result.SetElement_22( Element22 );
	
	return(Result);
}



#endif // 0
