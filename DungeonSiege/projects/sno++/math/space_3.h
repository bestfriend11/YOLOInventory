#pragma once
#if !defined(SPACE_3_H)
/* ========================================================================
   Header File: space_3.h
   Description: Declares convenient functions for working with spatial
                relations in 3D
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "vector_3.h"
#include "plane_3.h"
#include "sphere_3.h"

/* ========================================================================
   Forward Declarations
   ======================================================================== */
struct matrix_3x3; // from matrix_3x3.h
class axis_aligned_bounding_box_3; // from axis_aligned_bounding_box_3.h
class oriented_bounding_box_3; // from oriented_bounding_box_3.h
class line_segment_3; // from line_segment_3.h
class plane_3; // from plane_3.h

// Vector relations (from space_3.h)
inline real AngleBetweenUnit(vector_3 const &Unit1, vector_3 const &Unit2);
inline real AngleBetween(vector_3 const &Operand1, vector_3 const &Operand2);
inline real DistanceToPlane(plane_3 const &Plane, vector_3 const &Vector);
inline bool IsOnPlane(plane_3 const &Plane, vector_3 const &Vector,
					  real const Tolerance = RealTolerance);
inline bool IsInsidePlane(plane_3 const &Plane, vector_3 const &Vector,
						  real const Tolerance = RealTolerance);
inline bool IsOutsidePlane(plane_3 const &Plane, vector_3 const &Vector,
						   real const Tolerance = RealTolerance);
// Origin is the start of your line, direction is it's directional vector.  Point is
// the point in space that you want to find the closest point on your line to.  Return
// value is the closest point on the line described by Origin+Direction(t) to Point.
inline vector_3 ClosestPointOnLine( vector_3 const &Origin, vector_3 const &Direction, vector_3 const &Point );
inline bool ClosestPointOnLineSegment( vector_3 const &Origin, vector_3 const &Direction, vector_3 const &Point, vector_3& retPoint );

// Sphere relations
inline bool IsIntersecting(sphere_3 const &Operand1,
						   sphere_3 const &Operand2,
						   real const Tolerance = RealTolerance);

// Determine if a ray hits a box
bool RayIntersectsBox( const vector_3& minBound, const vector_3& maxBound,
					   const vector_3& ray_orig, const vector_3& ray_dir, vector_3& coord );

// Determine if a point lies within a box
inline bool PointInBox( const vector_3& minBound, const vector_3& maxBound, const vector_3& point );

// Method to determine if one axis aligned box touches or intersects another
inline bool BoxIntersectsBox( const vector_3& minBox1, const vector_3& maxBox1,
							  const vector_3& minBox2, const vector_3& maxBox2 );

// Correct an axis aligned box for any rotation
inline void CorrectBoxForRotation( vector_3& minBox, vector_3& maxBox );

// Clipping (from space_3.cpp)
vector_3 ClosestPointOnPlane(plane_3 const &Plane, vector_3 const &Vector);
vector_3 ReflectInPlane(plane_3 const &Plane, vector_3 const &Vector);
// TODO: Should these return line segments instead of modifying references?
void ClipLineSegment(axis_aligned_bounding_box_3 const &ToBoundingBox,
					 line_segment_3 &ClipSegment,
					 real const Tolerance = RealTolerance);
void ClipLineSegment(oriented_bounding_box_3 const &ToBoundingBox,
					 line_segment_3 &ClipSegment,
					 real const Tolerance = RealTolerance);
void ClipLineSegment(plane_3 const &ToPlane,
					 line_segment_3 &ClipSegment,
					 real const Tolerance = RealTolerance);

// Coordinate system creation (from space_3.cpp)
matrix_3x3 DextralColumns(vector_3 const &XAxis);
matrix_3x3 DextralColumnsY(vector_3 const &YAxis);		// Assume input vector is second column -- biddle
matrix_3x3 SinistralColumns(vector_3 const &XAxis);
matrix_3x3 AxisRotationColumns(
	vector_3 const &Axis, real const AngleInRadians);
matrix_3x3 XRotationColumns(real const AngleInRadians);
matrix_3x3 YRotationColumns(real const AngleInRadians);
matrix_3x3 ZRotationColumns(real const AngleInRadians);

/* ========================================================================
   Constants
   ======================================================================== */
vector_3 const GlobalXAxis(real(1), real(0), real(0));
vector_3 const GlobalYAxis(real(0), real(1), real(0));
vector_3 const GlobalZAxis(real(0), real(0), real(1));

/* ========================================================================
   Inline Functions
   ======================================================================== */

/* ************************************************************************
   Function: AngleBetweenUnit
   Description: Returns the angle between the input vectors, assuming they
                are both of unit length
   ************************************************************************ */
inline real
AngleBetweenUnit(vector_3 const &Unit1, vector_3 const &Unit2)
{
	return(ACos(InnerProduct(Unit1, Unit2)));
}

/* ************************************************************************
   Function: AngleBetween
   Description: Returns the angle between the input vectors.  The angle
                is undefined when one or both of the vectors is the
				zero vector.

   TODO: Should the angle be defined when one or both of the vectors is a
         zero vector?
   ************************************************************************ */
inline real
AngleBetween(vector_3 const &Operand1, vector_3 const &Operand2)
{
	return(AngleBetweenUnit(Normalize(Operand1), Normalize(Operand2)));
}

/* ************************************************************************
   Function: DistanceToPlane
   Description: Returns a metric of the distance from the input vector to
                the input plane, such that (DistanceToPlane(p - n * x) = 0)
				where n is the plane normal, and x is the distance returned
				from the function.
   ************************************************************************ */
inline real
DistanceToPlane(plane_3 const &Plane, vector_3 const &Vector)
{
	return(Plane.Evaluate(Vector));
}

/* ************************************************************************
   Function: IsOnPlane
   Description: Returns true if the input vector is within Tolerance of
                the input plane
   ************************************************************************ */
inline bool
IsOnPlane(plane_3 const &Plane, vector_3 const &Vector,
		  real const Tolerance)
{
	return(IsZero(DistanceToPlane(Plane, Vector), Tolerance));
}

/* ************************************************************************
   Function: IsInsidePlane
   Description: Returns true if the input vector is within Tolerance of
                the positive halfspace created by the input plane
   ************************************************************************ */
inline bool
IsInsidePlane(plane_3 const &Plane, vector_3 const &Vector,
			  real const Tolerance)
{
	return(Plane.Evaluate(Vector) > -Tolerance);
}

/* ************************************************************************
   Function: IsOutsidePlane
   Description: Returns true if the input vector is within Tolerance of
                the negative halfspace created by the input plane
   ************************************************************************ */
inline bool
IsOutsidePlane(plane_3 const &Plane, vector_3 const &Vector,
			   real const Tolerance)
{
	return(Plane.Evaluate(Vector) < Tolerance);
}

/* ************************************************************************
   Function: ClosestPointOnLine
   Description: Returns the closest point on the given line to the given
				Point in space.
   ************************************************************************ */
inline vector_3 
ClosestPointOnLine( vector_3 const &Origin, vector_3 const &Direction, vector_3 const &Point )
{
	return( Origin + (( InnerProduct( (Point - Origin), Direction ) / InnerProduct( Direction, Direction ) ) * Direction) );
}

inline bool 
ClosestPointOnLineSegment( vector_3 const &Origin, vector_3 const &Direction, vector_3 const &Point, vector_3& retPoint )
{
	float dotProduct	= DotProduct( (Point - Origin), Direction ) / Length2( Direction );
	if( !IsNegative( dotProduct ) && (dotProduct <= 1.0f) )
	{
		retPoint = Origin + ( dotProduct * Direction );
		return true;
	}
	else
	{
		return false;
	}
}

/* ************************************************************************
   Function: IsIntersecting
   Description: Tests two spheres to see if they're intersecting
   ************************************************************************ */
inline bool
IsIntersecting(sphere_3 const &Operand1, sphere_3 const &Operand2,
			   real const Tolerance)
{
	real const SquaredRadii =
		Square(Operand1.GetRadius() + Operand2.GetRadius());
	real const SquaredDistance =
		Length2(Operand2.GetCenter() - Operand1.GetCenter());
	
	return((SquaredRadii - Tolerance) < SquaredDistance);
}


// Determine if a point lies within a box
inline bool PointInBox( const vector_3& minBound, const vector_3& maxBound, const vector_3& point )
{
	if( point.x < minBound.x ){
		return false;
	}
	if( point.x > maxBound.x ){
		return false;
	}
	if( point.y < minBound.y ){
		return false;
	}
	if( point.y > maxBound.y ){
		return false;
	}
	if( point.z < minBound.z ){
		return false;
	}
	if( point.z > maxBound.z ){
		return false;
	}
	return true;
}

// Method to determine if one axis aligned box touches or intersects another
inline bool BoxIntersectsBox( const vector_3& minBox1, const vector_3& maxBox1,
							  const vector_3& minBox2, const vector_3& maxBox2 )
{
	if( (maxBox2.x < minBox1.x || maxBox2.z < minBox1.z) ||
		(minBox2.x > maxBox1.x || minBox2.z > maxBox1.z) ||
		(maxBox2.y < minBox1.y || minBox2.y > maxBox1.y) )
	{
		return false;
	}
	else
	{
		return true;
	}
}

// Correct an axis aligned box for rotation
inline void CorrectBoxForRotation( vector_3& minBox, vector_3& maxBox )
{
	if( minBox.x > maxBox.x )
	{
		float minX	= minBox.x;
		minBox.x	= maxBox.x;
		maxBox.x	= minX;
	}

	if( minBox.y > maxBox.y )
	{
		float minY	= minBox.y;
		minBox.y	= maxBox.y;
		maxBox.y	= minY;
	}

	if( minBox.z > maxBox.z )
	{
		float minZ	= minBox.z;
		minBox.z	= maxBox.z;
		maxBox.z	= minZ;
	}
}
#define SPACE_3_H
#endif
