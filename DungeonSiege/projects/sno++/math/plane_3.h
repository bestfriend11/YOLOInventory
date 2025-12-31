#pragma once
#if !defined(PLANE_3_H)
/* ========================================================================
   Header File: plane_3.h
   Description: Declares plane_3
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "vector_3.h"
#include "matrix_3x3.h"

/* ========================================================================
   Forward Declarations
   ======================================================================== */
class plane_3; // from plane_3.h

// Non-member arithmetic operations (from vector_3.h)
inline plane_3 operator+(vector_3 const &Vector, plane_3 const &Plane);
inline plane_3 operator+(plane_3 const &Plane, vector_3 const &Vector);
inline plane_3 operator-(vector_3 const &Vector, plane_3 const &Plane);
inline plane_3 operator-(plane_3 const &Plane, vector_3 const &Vector);
inline plane_3 operator-(plane_3 const &Plane);
inline plane_3 operator*(matrix_3x3 const &Transform, plane_3 const &Plane);
inline plane_3 operator*(plane_3 const &Plane, real const Scalar);
inline plane_3 operator*(real const Scalar, plane_3 const &Plane);
inline plane_3 operator/(plane_3 const &Plane, real const Scalar);

/* ************************************************************************
   Class: plane_3
   Description: Represents a plane in 3 dimensions, defined as
                (Normal^Tp + D = 0) for all points p on the plane.

				All arithmetic operations f() on the plane are defined
				to hold (Normal^T(f(p)) + D = 0) true.  I'm not positive
				that this is the most productive definition of planar
				arithmetic operations, but it is useful for
				pre-transforming and translating planes to optimize
				comparisons or operations on several vectors, since that
				seemed to be the most common usage for planar arithmetic
				operations.

   TODO: Note that this class is experimental, and should not be used
         extensively by anyone until I verify that it is designed
		 appropriately.
   ************************************************************************ */
class plane_3
{
public:
	// Read access
	inline vector_3 const &GetNormal(void) const;
	inline real GetD(void) const;

	// Write access
	inline void SetNormal(vector_3 const &Normal);
	inline void SetD(real const D);

	// Member arithmetic operations
	inline plane_3 &operator+=(vector_3 const &Vector);
	inline plane_3 &operator-=(vector_3 const &Vector);
	inline plane_3 &operator*=(real const Scalar);
	inline plane_3 &operator/=(real const Scalar);

	// The plane equation (Normal^Tp + D)
	inline real Evaluate(vector_3 const &Vector) const;

	// Existence
	inline plane_3(void);
	inline plane_3(vector_3 const &Normal, real const D);

private:
	// Value
	vector_3 Normal;
	real D;
};


/* ========================================================================
   Inline Functions
   ======================================================================== */

/* ************************************************************************
   Function: GetNormal
   Description: Returns the normal to the plane
   ************************************************************************ */
inline vector_3 const &plane_3::
GetNormal(void) const
{
	return(Normal);
}

/* ************************************************************************
   Function: GetD
   Description: Returns the d value of the plane
   ************************************************************************ */
inline real plane_3::
GetD(void) const
{
	return(D);
}

/* ************************************************************************
   Function: SetNormal
   Description: Sets the normal of the plane to the input vector
   ************************************************************************ */
inline void plane_3::
SetNormal(vector_3 const &NormalInit)
{
	Normal = NormalInit;
}

/* ************************************************************************
   Function: SetD
   Description: Sets the d value of the plane (see GetD()) to the input
                scalar
   ************************************************************************ */
inline void plane_3::
SetD(real const DInit)
{
	D = DInit;
}

/* ************************************************************************
   Function: operator+= and operator+
   Description: Moves the plane by the input vector such that, for any
                point p on the original plane,
				(Normal^T(p + Vector) + D = 0)
   ************************************************************************ */
inline plane_3 &plane_3::
operator+=(vector_3 const &Vector)
{
	SetD(GetD() + InnerProduct(GetNormal(), Vector));
	return(*this);
}

inline plane_3
operator+(plane_3 const &Plane, vector_3 const &Vector)
{
	return(plane_3(Plane.GetNormal(),
				   Plane.GetD() + InnerProduct(Plane.GetNormal(), Vector)));
}

inline plane_3
operator+(vector_3 const &Vector, plane_3 const &Plane)
{
	return(Plane + Vector);
}

/* ************************************************************************
   Function: operator-=
   Description: Moves the plane by the inverse of the input vector such
                that, for any point p on the original plane,
				(Normal^T(p - Vector) + D = 0)
   ************************************************************************ */
inline plane_3 &plane_3::
operator-=(vector_3 const &Vector)
{
	SetD(GetD() - InnerProduct(GetNormal(), Vector));
	return(*this);
}

inline plane_3
operator-(plane_3 const &Plane, vector_3 const &Vector)
{
	return(plane_3(Plane.GetNormal(),
				   Plane.GetD() - InnerProduct(Plane.GetNormal(), Vector)));
}

inline plane_3
operator-(vector_3 const &Vector, plane_3 const &Plane)
{
	return(Plane - Vector);
}

/* ************************************************************************
   Function: unary operator-
   Description: Inverts the plane such that, for any point p on the
                original plane, (Normal^T(-p) + D = 0)
   ************************************************************************ */
inline plane_3
operator-(plane_3 const &Plane)
{
	return(plane_3(-Plane.GetNormal(), Plane.GetD()));
}

/* ************************************************************************
   Function: operator*= and operator*
   Description: Multiplies the plane by a scalar such that, for any point p
                on the original plane, (Normal^T(Scalar * p) + D = 0)
   ************************************************************************ */
inline plane_3 &plane_3::
operator*=(real const Scalar)
{
	SetD(GetD() / Scalar);
	return(*this);
}

inline plane_3
operator*(plane_3 const &Plane, real const Scalar)
{
	return(plane_3(Plane.GetNormal(), Plane.GetD() / Scalar));
}

inline plane_3
operator*(real const Scalar, plane_3 const &Plane)
{
	return(Plane * Scalar);
}

/* ************************************************************************
   Function: operator*
   Description: Transforms the plane by a matrix such that, for any point
                p on the original plane, (Normal^T(Transform * p) + D) = 0
   ************************************************************************ */
inline plane_3
operator*(matrix_3x3 const &Transform, plane_3 const &Plane)
{
	return(plane_3(Transform * Plane.GetNormal(), Plane.GetD()));
}

/* ************************************************************************
   Function: operator/=
   Description: Divides the plane by a scalar.  See operator*=.
   ************************************************************************ */
inline plane_3 &plane_3::
operator/=(real const Scalar)
{
	*this *= RealInverse(Scalar);
	return(*this);
}

inline plane_3
operator/(plane_3 const &Plane, real const Scalar)
{
	return(Plane * RealInverse(Scalar));
}

/* ************************************************************************
   Function: Evaluate
   Description: Returns the value of the left-hand side of the plane
                equation for the input vector (Normal^Tp + D)
   ************************************************************************ */
inline real plane_3::
Evaluate(vector_3 const &Vector) const
{
	return(InnerProduct(GetNormal(), Vector) + GetD());
}

/* ************************************************************************
   Function: plane_3
   Description: Construct the default plane, passing through the origin
                with normal along the Z axis                
   ************************************************************************ */
inline plane_3::
plane_3(void)
		: Normal(RealZero, RealZero, RealIdentity),
		  D(RealZero)
{
}

/* ************************************************************************
   Function: plane_3
   Description: Construct a plane based on input parameters
   ************************************************************************ */
inline plane_3::
plane_3(vector_3 const &NormalInit, real const DInit)
		: Normal(NormalInit),
		  D(DInit)
{
}

#define PLANE_3_H
#endif
