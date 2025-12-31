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
#include "quat.h"
#include "gpcoll.h"

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
inline real SignedAngleBetweenUnit(vector_3 const &Unit1, vector_3 const &Unit2);
inline real SignedAngleBetween(vector_3 const &Operand1, vector_3 const &Operand2);
inline float NormAngleBetween( vector_3 const &Operand1, vector_3 const &Operand2, vector_3 const &Normal );
inline real DistanceToPlane(plane_3 const &Plane, vector_3 const &Vector);
inline bool IsOnPlane(plane_3 const &Plane, vector_3 const &Vector,
					  real const Tolerance = RealTolerance);
inline bool IsInsidePlane(plane_3 const &Plane, vector_3 const &Vector,
						  real const Tolerance = RealTolerance);
inline bool IsOutsidePlane(plane_3 const &Plane, vector_3 const &Vector,
						   real const Tolerance = RealTolerance);

inline void CalcFaceNormal( vector_3& normal, const vector_3& v1, const vector_3& v2, const vector_3& v3 );

// Origin is the start of your line, direction is it's directional vector.  Point is
// the point in space that you want to find the closest point on your line to.  Return
// value is the closest point on the line described by Origin+Direction(t) to Point.
inline vector_3 ClosestPointOnLine( vector_3 const &Origin, vector_3 const &Direction, vector_3 const &Point );
inline bool ClosestPointOnLineSegment( vector_3 const &Origin, vector_3 const &Direction, vector_3 const &Point, vector_3& retPoint );

// Sphere relations
inline bool IsIntersecting(sphere_3 const &Operand1,
						   sphere_3 const &Operand2,
						   real const Tolerance = RealTolerance);

// Line relations
inline bool LineIntersectXZ( const vector_3& line1start, const vector_3& line1end,
							 const vector_3& line2start, const vector_3& line2end,
							 vector_3& intersection );

// Box relations

// Determine if a ray hits a box
bool RayIntersectsBox( const vector_3& minBound, const vector_3& maxBound,
					   const vector_3& ray_orig, const vector_3& ray_dir, vector_3& coord );

// Determine if a single dimension ray hits a box
bool YRayIntersectsBox( const vector_3& minBound, const vector_3& maxBound,
					    const vector_3& ray_orig, const float ray_dir, vector_3& coord );

// Determine if a point lies within a box
inline bool PointInBox( const vector_3& minBound, const vector_3& maxBound, const vector_3& point );

// Determine if a point lies within a sphere
inline bool PointInSphere( const vector_3& sphere_pos, const float sphere_radius, const vector_3& point );

// Determine if a point lies within a cone section of a sphere ( think snow-cone :) ) -bk
inline bool PointInSphereConeSection( const vector_3& position, const vector_3 & cone_direction, const float cone_angle, const float sphere_radius, const vector_3 & point );

// Determine if a point lies within a cone
inline bool PointInCone( const vector_3& cone_pos, const vector_3& cone_dir, const float cone_angle, const vector_3& point );

// Method to determine if one axis aligned box touches or intersects another
inline bool BoxIntersectsBox( const vector_3& minBox1, const vector_3& maxBox1,
							  const vector_3& minBox2, const vector_3& maxBox2 );

// Determine the overlapping volume of two axis aligned bounding boxes
inline void BoxIntersection( const vector_3& minBox1, const vector_3& maxBox1,
							 const vector_3& minBox2, const vector_3& maxBox2,
							 vector_3& retMinBox, vector_3& retMaxBox );

// Correct an axis aligned box for any rotation
inline void TransformAxisAlignedBox( const matrix_3x3& mat, const vector_3& trans,
									 const vector_3& minBox, const vector_3& maxBox,
									 vector_3& retMinBox, vector_3& retMaxBox );

// Calculate whether or not a sphere intersects a sphere
inline bool SphereIntersectsSphere( const vector_3& fsphere_pos, const float fsphere_radius,
								    const vector_3& ssphere_pos, const float ssphere_radius );

// Calculate whether or not a sphere intersects a box
inline bool SphereIntersectsBox( const vector_3& sphere_pos, const float sphere_radius,
								 const vector_3& min_box, const vector_3& max_box );

// Calculate whether or not a sphere intersects a cone
inline bool SphereIntersectsCone( const vector_3& cone_pos, const vector_3& cone_dir, const float cone_angle,
								  const vector_3& sphere_pos, const float sphere_radius );

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

bool ClipLineSegment( const vector_3& normal,
					  vector_3 const &Point1, 
					  float const &Point1Eval,
					  vector_3 const &Point2, 
					  vector_3 &Intersect, 
					  const float Tolerance = RealTolerance);


// Clip a triangle, filling the the passed vector with any extra verts as a result of the clip
// i.e. if the point was clipped off of the triangle, then the list would result in 6 vertices,
// three for each triangle.
void ClipTriangles( plane_3 const &Plane, stdx::fast_vector< vector_3 > &VectList );
void InvertedClipTriangles( const vector_3& normal, const float d, stdx::fast_vector< vector_3 >& triList );

// Generate an orientation matrix from a given directional vector
inline matrix_3x3 MatrixFromDirection( vector_3 const &dir );
/*inline*/ Quat		QuatFromDirection( vector_3 const &dir );
// This is a routine optimized for headings (vectors of the form <x,0,z>)
/*inline*/ Quat		QuatFromXZComponents(float x, float z );
/*inline*/ void		QuatFromXZComponents(float x, float z, Quat& q);

// Blend between two matrices
matrix_3x3 BlendMatrices( const matrix_3x3& fromMatrix, const matrix_3x3& toMatrix, const float amount );

// Coordinate system creation (from space_3.cpp)
matrix_3x3 DextralColumns(vector_3 const &XAxis);
matrix_3x3 DextralColumnsY(vector_3 const &YAxis);		// Assume input vector is second column -- biddle
matrix_3x3 SinistralColumns(vector_3 const &XAxis);
matrix_3x3 AxisRotationColumns(
	vector_3 const &Axis, real const AngleInRadians);

// convention: as angle increases, rotation happens clockwise
// create a rotation matrix for rotation around the X axis
matrix_3x3 XRotationColumns(real const AngleInRadians);
// create a rotation matrix for rotation around the Y axis
matrix_3x3 YRotationColumns(real const AngleInRadians);
// create a rotation matrix for rotation around the Z axis
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
	gpassert( IsOne( Unit1.Length() ) && IsOne( Unit2.Length() ) );

	DEBUG_FLOAT_VALIDATE( Unit1.x );
	DEBUG_FLOAT_VALIDATE( Unit1.y );
	DEBUG_FLOAT_VALIDATE( Unit1.z );
	DEBUG_FLOAT_VALIDATE( Unit2.x );
	DEBUG_FLOAT_VALIDATE( Unit2.y );
	DEBUG_FLOAT_VALIDATE( Unit2.z );

	float dot_p	= Unit1.DotProduct( Unit2 );
	if( IsOne( FABSF( dot_p ), 0.001f ) )
	{
		if( dot_p > 0.0f )
		{
			return 0.0f;
		}
		else
		{
			return RADIANS_180;
		}
	}
	else
	{
		return( ACOSF( dot_p ) );
	}
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
	gpassert( !Operand1.IsZero() && !Operand2.IsZero() );
	return( AngleBetweenUnit( Normalize( Operand1 ), Normalize( Operand2 ) ) );
}

/* ************************************************************************
   Function: SignedAngleBetweenUnit
   Description: Returns the single angle between the input vectors, assuming they
                are both of unit length. 
				Sign is positive when the rotation of U1 to U2 about U1xU2 is CCW 
				(ie, right hand rule applied) --biddle
   ************************************************************************ */
inline real
SignedAngleBetweenUnit(vector_3 const &Unit1, vector_3 const &Unit2)
{
	gpassert( IsOne( Unit1.Length() ) && IsOne( Unit2.Length() ) );

	DEBUG_FLOAT_VALIDATE( Unit1.x );
	DEBUG_FLOAT_VALIDATE( Unit1.y );
	DEBUG_FLOAT_VALIDATE( Unit1.z );
	DEBUG_FLOAT_VALIDATE( Unit2.x );
	DEBUG_FLOAT_VALIDATE( Unit2.y );
	DEBUG_FLOAT_VALIDATE( Unit2.z );

	float dot_p	= Unit1.DotProduct( Unit2 );
	if( IsOne( FABSF( dot_p ), 0.001f ) )
	{
		if( dot_p > 0.0f )
		{
			return 0.0f;
		}
		else
		{
			return RADIANS_180;
		}
	}
	else
	{
		float theta = ACOSF( dot_p );
		float sign;
		if (!IsEqual(FABSF(Unit1.y),1.0f,0.001f))
		{
			sign = vector_3(-Unit1.z, Unit1.y, Unit1.x).DotProduct(Unit2);
		}
		else
		{
			sign = -Unit1.y * Unit2.z;
		}
		return( (sign<0) ?  -theta : theta );
	}
}

/* ************************************************************************
   Function: SignedAngleBetween
   Description: Returns the angle between the input vectors.  The angle
                is undefined when one or both of the vectors is the
				zero vector.
				Sign is positive when the rotation of U1 to U2 about U1xU2 is CCW 
				(ie, right hand rule applied) --biddle

   TODO: Should the angle be defined when one or both of the vectors is a
         zero vector?
   ************************************************************************ */
inline real
SignedAngleBetween(vector_3 const &Operand1, vector_3 const &Operand2)
{
	gpassert( !Operand1.IsZero() && !Operand2.IsZero() );
	return( SignedAngleBetweenUnit( Normalize( Operand1 ), Normalize( Operand2 ) ) );
}

// Takes to rays and returns the angle between the two in the range [0-2PI].
// The positive/negative sides of the unit circle are determined by the plane evaluated using
// the given normal and the first operand.
inline float
NormAngleBetween( vector_3 const &Operand1, vector_3 const &Operand2, vector_3 const &Normal )
{
	DEBUG_FLOAT_VALIDATE( Operand1.x );
	DEBUG_FLOAT_VALIDATE( Operand1.y );
	DEBUG_FLOAT_VALIDATE( Operand1.z );
	DEBUG_FLOAT_VALIDATE( Operand2.x );
	DEBUG_FLOAT_VALIDATE( Operand2.y );
	DEBUG_FLOAT_VALIDATE( Operand2.z );
	DEBUG_FLOAT_VALIDATE( Normal.x );
	DEBUG_FLOAT_VALIDATE( Normal.y );
	DEBUG_FLOAT_VALIDATE( Normal.z );

	// First, get the angle
	float angle = AngleBetween( Operand1, Operand2 );
	DEBUG_FLOAT_VALIDATE( angle );

	// Build our plane
	plane_3 nPlane( Normal, -Normal.DotProduct( Operand1 ) );
	if( nPlane.Evaluate( Operand2 ) < -FLOAT_TOLERANCE )
	{
		// Outside the plane (on the negative side of the circle)
		angle = PI2 - angle;
	}

	return angle;
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


// Calculate the face normal from three positional verts
inline void
CalcFaceNormal( vector_3& normal, const vector_3& v1, const vector_3& v2, const vector_3& v3 )
{
	__asm
	{
		mov		eax, dword ptr [v1]
		fld		dword ptr [eax+4]		//v1.y
		mov		ecx, dword ptr [v3]	
		fld		dword ptr [ecx+4]		//v3.y
		fsub	st(0), st(1)
		mov		edx, dword ptr [v2]
		fld		dword ptr [edx+4]		//v2.y
		fsubrp	st(2), st(0)
		mov		esi, dword ptr [normal]

		fld		dword ptr [eax+8]		// v1.z
		fld		dword ptr [ecx+8]		// v3.z
		fsub	st(0), st(1)
		fld		dword ptr [edx+8]		// v2.z
		fsubrp	st(2), st(0)

		fld		st(0)
		fmul	st(0), st(4)
		fld		st(2)
		fmul	st(0), st(4)
		fsubp	st(1), st(0)
		fstp	dword ptr [esi]
		//////////////////////////////////////

		fld		dword ptr [eax]			// v1.x
		fld		dword ptr [ecx]			// v3.x
		fsub	st(0), st(1)
		fmul	st(3), st(0)
		fxch	st(2)

		fld		dword ptr [edx]			// v2.x
		fsubrp	st(2), st(0)
		fmul	st(0), st(1)
		fsubp	st(3), st(0)
		fxch	st(2)
		fstp	dword ptr [esi+4]
		/////////////////////////////////////

		fmulp	st(3), st(0)
		fmulp	st(1), st(0)

		fsubrp	st(1), st(0)
		fstp	dword ptr [esi+8]
	}
}

/* ************************************************************************
   Function: ClosestPointOnLine
   Description: Returns the closest point on the given line to the given
				Point in space.
   ************************************************************************ */
inline vector_3 
ClosestPointOnLine( vector_3 const &Origin, vector_3 const &Direction, vector_3 const &Point )
{
	float ray_mag	= (Point - Origin).DotProduct( Direction ) / Direction.Length2();
	if( ray_mag <= 0.0f )
	{
		return Origin;
	}

	return( Origin + ( ray_mag * Direction ) );
}

// The variation is used by the calcbsp routine in the exporter --biddle
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
	real const SquaredRadii		= Square(Operand1.GetRadius() + Operand2.GetRadius());
	real const SquaredDistance	= Length2(Operand2.GetCenter() - Operand1.GetCenter());
	
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

// Determine if a point lies within a sphere
inline bool PointInSphere( const vector_3& sphere_pos, const float sphere_radius, const vector_3& point )
{
	return( Length2( point - sphere_pos ) <= (sphere_radius * sphere_radius) );
}

// Determine if a point lies within a cone
inline bool PointInCone( const vector_3& cone_pos, const vector_3& cone_dir, const float cone_angle, const vector_3& point )
{
	// Find the closest point along the axis of the cone
	vector_3 closest_point	= ClosestPointOnLine( cone_pos, cone_dir, point );

	// Use the length to determine if point lies within cone
	return( Length2( point - closest_point ) <= (Length2( closest_point - cone_pos ) * tanf( cone_angle )) );
}

// Determine if a point lies within a cone section of a sphere ( think snow-cone :) ) -bk
inline bool PointInSphereConeSection( const vector_3& position, const vector_3 & cone_direction, const float cone_angle, const float sphere_radius, const vector_3 & point )
{
	return( PointInSphere( position, sphere_radius, point ) && PointInCone( position, cone_direction, cone_angle, point ) );
}

// Determine is segment from p0 to p1 intersect the sphere,
// if there is an intersection, it's located at p0 + tau*(p1-p0)
inline bool DirectedSegmentPenetratesSphereAtOrigin(const vector_3& p0, const vector_3& p1, const float sphere_radius, float& retval_tau);

inline bool DirectedSegmentPenetratesSphere(const vector_3& p0, const vector_3& p1, const float s_radius, const vector_3& s_center, float& retval_tau)
			{
			return DirectedSegmentPenetratesSphereAtOrigin(p0-s_center,p1-s_center,s_radius,retval_tau);
			};

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

// Determine the overlapping volume of two axis aligned bounding boxes
inline void BoxIntersection( const vector_3& minBox1, const vector_3& maxBox1,
							 const vector_3& minBox2, const vector_3& maxBox2,
							 vector_3& retMinBox, vector_3& retMaxBox )
{
	retMinBox.x = max_t( minBox1.x, minBox2.x );
	retMaxBox.x = min_t( maxBox1.x, maxBox2.x );
	if( retMinBox.x >= retMaxBox.x )
	{
		retMinBox.x = retMaxBox.x = 0.0f;
	}

	retMinBox.y = max_t( minBox1.y, minBox2.y );
	retMaxBox.y = min_t( maxBox1.y, maxBox2.y );
	if( retMinBox.y >= retMaxBox.y )
	{
		retMinBox.y = retMaxBox.y = 0.0f;
	}

	retMinBox.z = max_t( minBox1.z, minBox2.z );
	retMaxBox.z = min_t( maxBox1.z, maxBox2.z );
	if( retMinBox.z >= retMaxBox.z )
	{
		retMinBox.z = retMaxBox.z = 0.0f;
	}
}

// Macros for helping out the TransformBox call below
#define MODIFYBOXX( a, b, min, max )	( (a < b) ? ( min.x += a, max.x += b ) : ( min.x += b, max.x += a ) )
#define MODIFYBOXY( a, b, min, max )	( (a < b) ? ( min.y += a, max.y += b ) : ( min.y += b, max.y += a ) )
#define MODIFYBOXZ( a, b, min, max )	( (a < b) ? ( min.z += a, max.z += b ) : ( min.z += b, max.z += a ) )

// Correct an axis aligned box for any rotation
inline void TransformAxisAlignedBox( const matrix_3x3& mat, const vector_3& trans,
									 const vector_3& minBox, const vector_3& maxBox,
									 vector_3& retMinBox, vector_3& retMaxBox )
{
	// Get the translation out of the way by starting there
	retMinBox = retMaxBox = trans;

	float  a, b;
	
	// Do the X components
	a	= mat.v00 * minBox.x;
	b	= mat.v00 * maxBox.x;
	MODIFYBOXX( a, b, retMinBox, retMaxBox );

	a	= mat.v01 * minBox.y;
	b	= mat.v01 * maxBox.y;
	MODIFYBOXX( a, b, retMinBox, retMaxBox );

	a	= mat.v02 * minBox.z;
	b	= mat.v02 * maxBox.z;
	MODIFYBOXX( a, b, retMinBox, retMaxBox );

	// Do the Y components
	a	= mat.v10 * minBox.x;
	b	= mat.v10 * maxBox.x;
	MODIFYBOXY( a, b, retMinBox, retMaxBox );

	a	= mat.v11 * minBox.y;
	b	= mat.v11 * maxBox.y;
	MODIFYBOXY( a, b, retMinBox, retMaxBox );

	a	= mat.v12 * minBox.z;
	b	= mat.v12 * maxBox.z;
	MODIFYBOXY( a, b, retMinBox, retMaxBox );

	// Do the Z components
	a	= mat.v20 * minBox.x;
	b	= mat.v20 * maxBox.x;
	MODIFYBOXZ( a, b, retMinBox, retMaxBox );

	a	= mat.v21 * minBox.y;
	b	= mat.v21 * maxBox.y;
	MODIFYBOXZ( a, b, retMinBox, retMaxBox );

	a	= mat.v22 * minBox.z;
	b	= mat.v22 * maxBox.z;
	MODIFYBOXZ( a, b, retMinBox, retMaxBox );
}

// Calculate whether or not a sphere intersects a sphere
inline bool SphereIntersectsSphere( const vector_3& fsphere_pos, const float fsphere_radius,
								    const vector_3& ssphere_pos, const float ssphere_radius )
{
	// Calculate the squared radius of both spheres
	float SquareRadius	= fsphere_radius + ssphere_radius;
	SquareRadius		*= SquareRadius;

	// Calculate the squared distance between the two sphere centers and see if it is within range
	return( Length2( fsphere_pos - ssphere_pos ) < SquareRadius );
}

inline bool SphereIntersectsBox( const vector_3& sphere_pos, const float sphere_radius, 
								 const vector_3& min_box, const vector_3& max_box )
{
	// Calculate the squared radius of the sphere
	const float SquareRadius = ( sphere_radius * sphere_radius );

	// Add up the cumulative difference between dimensions
	float accum = 0.0f;

	// Do the X coordinate difference
	if( sphere_pos.x < min_box.x )
	{
		float xdiff	= (sphere_pos.x - min_box.x);
		accum += ( xdiff * xdiff );
	}
	else if( sphere_pos.x > max_box.x )
	{
		float xdiff	= (sphere_pos.x - max_box.x);
		accum += ( xdiff * xdiff );
	}

	// Do the Y coordinate difference
	if( sphere_pos.y < min_box.y )
	{
		float ydiff	= (sphere_pos.y - min_box.y);
		accum += ( ydiff * ydiff );
	}
	else if( sphere_pos.y > max_box.y )
	{
		float ydiff	= (sphere_pos.y - max_box.y);
		accum += ( ydiff * ydiff );
	}

	// Do the Z coordinate difference
	if( sphere_pos.z < min_box.z )
	{
		float zdiff	= (sphere_pos.z - min_box.z);
		accum += ( zdiff * zdiff );
	}
	else if( sphere_pos.z > max_box.z )
	{
		float zdiff	= (sphere_pos.z - max_box.z);
		accum += ( zdiff * zdiff );
	}

	if( accum <= SquareRadius )
	{
		return true;
	}
	else
	{
		return false;
	}
}

// Calculate whether or not a sphere intersects a cone
inline bool SphereIntersectsCone( const vector_3& cone_pos, const vector_3& cone_dir, const float cone_angle,
								  const vector_3& sphere_pos, const float sphere_radius )
{
	// Find the difference between the cone position and sphere position
	vector_3 offset			= sphere_pos - cone_pos;
	float squaredOffsetDist	= Length2( offset );
	float squaredSphereRad	= sphere_radius * sphere_radius;

	// Test if cone_pos is in the sphere
	if( squaredOffsetDist - squaredSphereRad <= 0.0f )
	{
		return true;
	}

	// Test if sphere_pos is in the cone
	float dir_ip			= cone_dir.DotProduct( offset );
	float squareddir_ip		= dir_ip * dir_ip;
	float cone_cos, cone_sin;
	SINCOSF( cone_angle, cone_sin, cone_cos );

	if( squareddir_ip >= (squaredOffsetDist * (cone_cos * cone_cos)) &&	dir_ip > 0.0f )
	{
		return true;
	}

	// Do a plane intersection if neither point satifies the conditions
	float p_test			= (cone_cos * dir_ip) + (cone_sin * SquareRoot( FABSF( squaredOffsetDist - squareddir_ip ) ));
	float discriminant		= (p_test * p_test) - (squaredOffsetDist - squaredSphereRad);

	return( discriminant >= 0.0f && p_test >= 0.0f );
}

// Algorithm pulled from Franklin Antonio's article in GG3 and modified to handle floating
// point input.  The actual intersection calculation was heavily modified from the original.
inline bool LineIntersectXZ( const vector_3& line1start, const vector_3& line1end,
							 const vector_3& line2start, const vector_3& line2end,
							 vector_3& intersection )
{
	float Ax	= line1end.x - line1start.x;
	float Bx	= line2start.x - line2end.x;
	float x1lo, x1hi, y1lo, y1hi;

	if( Ax < 0.0f )
	{
		x1lo	= line1end.x;
		x1hi	= line1start.x;
	}
	else
	{
		x1hi	= line1end.x;
		x1lo	= line1start.x;
	}

	if( Bx > 0.0f )
	{
		if( x1hi < line2end.x || line2start.x < x1lo )
		{
			return false;
		}
	}
	else
	{
		if( x1hi < line2start.x || line2end.x < x1lo )
		{
			return false;
		}
	}

	float Ay	= line1end.z - line1start.z;
	float By	= line2start.z - line2end.z;

	if( Ay < 0.0f )
	{
		y1lo	= line1end.z;
		y1hi	= line1start.z;
	}
	else
	{
		y1hi	= line1end.z;
		y1lo	= line1start.z;
	}

	if( By > 0.0f )
	{
		if( y1hi < line2end.z || line2start.z < y1lo )
		{
			return false;
		}
	}
	else
	{
		if( y1hi < line2start.z || line2end.z < y1lo )
		{
			return false;
		}
	}

	float Cx	= line1start.x - line2start.x;
	float Cy	= line1start.z - line2start.z;

	float d		= (By * Cx) - (Bx * Cy);
	float f		= (Ay * Bx) - (Ax * By);

	if( f > 0.0f )
	{
		if( d < 0.0f || d > f )
		{
			return false;
		}
	}
	else if( d > 0.0f || d < f )
	{
		return false;
	}
	
	float e = (Ax * Cy) - (Ay * Cx);
	if( f > 0.0f )
	{
		if( e < 0.0f || e > f )
		{
			return false;
		}
	}
	else if( e > 0.0f || e < f )
	{
		return false;
	}
	
	// Compute intersection coordinates
	if( fabs( f ) < 0.001f )
	{
		return false;
	}

	float num		= (d * Ax) / f;
	intersection.x	= line1start.x + num;

	num				= (d * Ay) / f;
	intersection.z	= line1start.z + num;

	return true;
}

// Generate an orientation matrix from a given directional vector
inline matrix_3x3 MatrixFromDirection( vector_3 const &dir )
{
	matrix_3x3 retMat( DoNotInitialize );

	// Build initial axis set
	retMat.SetColumn2( Normalize( dir ) );

	// Always zero
	retMat.v10			= 0.0f;

	if( IsZero( retMat.v02 ) && IsZero( retMat.v22 ) )
	{
		retMat.v00		= 0.0f;
		retMat.v20		= 1.0f;
	}
	else
	{
		// Get the length of the cross product of norm( dir ) with the up vector
		float normLen	= INVERSEF( SQRTF( (retMat.v22 * retMat.v22) + (retMat.v02 * retMat.v02) ) );

		// Set the axis components and normalize them
		retMat.v00		= retMat.v22 * normLen;
		retMat.v20		= -(retMat.v02 * normLen);
	}

	// Finish by setting the components described by the cross of our two vectors
	retMat.v01			= (retMat.v12 * retMat.v20);
	retMat.v11			= (retMat.v22 * retMat.v00) - (retMat.v02 * retMat.v20);
	retMat.v21			= -(retMat.v12 * retMat.v00);

	// Make sure everything is okay
#ifdef PARANOID_MATH
	gpassertf( IsEqual( retMat.Determinant(), 1.0f ), ( "MatrixFromDirection() output matrix has determinant of %f, which should be 1.0f", retMat.Determinant() ));
#endif

	// Return  matrix to caller
	return retMat;
}

inline bool DirectedSegmentPenetratesSphereAtOrigin(const vector_3& p0, const vector_3& p1, const float sphere_radius, float& retval_tau)
{
	// This is the 3D version of the 2D penetration algorithm
	
	// Modified from Eberly's Magic Software 3D library and Foley van Dam
    // set up quadratic Q(t) = a*t^2 + 2*b*t + c

	// where 
	// a = v.v
	// b = v.u
	// c = (u.u) - (r*r)
	

	// u = (p0-co) -- here the center CO is the origin <0,0,0> so u == p0
	// v = (p1-p0)

	vector_3 v = p1 - p0;

    float c = p0.Length2() - Square(sphere_radius);

	if (c <= 0.0f){
		// We are already inside the circle
		retval_tau = 0.0f;
		return true;
	}

    float b = p0.DotProduct(v);

    if ( IsPositive(b) ) {
		// We are pointing away from the circle
		return false;
	}

    float a = v.Length2();

    if ( IsZero(a) ) {
		// The segment is zero length
		return false;
	}

	float discrim = Square(b)-a*c;

    if ( IsNegative(discrim)  )
    {
        return false;
	}
	else if (IsZero(discrim))
	{
		float tmp = -b;
		if  (tmp > a) 
		{
			return false;
		}
		retval_tau = tmp/a;
		return true;
	}

	// b is always negative at this point and we are 
	// never inside the circle so we can eliminate the positive root
	// and get the smaller of the two values
	float root = SQRTF(discrim);
	float tmp = -b - root;

	if  (tmp > a)
	{
		return false;
	}
	retval_tau = tmp/a;
	return true;

}

#define SPACE_3_H
#endif
