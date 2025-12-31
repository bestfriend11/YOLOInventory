/* ========================================================================
   Source File: space_3.cpp
   Description: Implements convenient functions for working with spatial
                relations in 3D
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "precomp_gpcore.h"
#include "space_3.h"

#include "axis_aligned_bounding_box_3.h"
#include "line_segment_3.h"
#include "matrix_3x3.h"
#include "oriented_bounding_box_3.h"


// Determine if a ray hits a box
bool RayIntersectsBox( const vector_3& minBound, const vector_3& maxBound,
							  const vector_3& ray_orig, const vector_3& ray_dir, vector_3& coord )
{
	vector_3	quadrant( 2.0f, 2.0f, 2.0f );
	vector_3	candidateplane;
	bool		inside = true;

	// First we need to generate candidate planes
	if( ray_orig.x < minBound.x ){
		quadrant.x = 1.0f;	// LEFT
		candidateplane.x = minBound.x;
		inside = false;
	}
	else if( ray_orig.x > maxBound.x ){
		quadrant.x = 0.0f;	// RIGHT
		candidateplane.x = maxBound.x;
		inside = false;
	}

	if( ray_orig.y < minBound.y ){
		quadrant.y = 1.0f;	// LEFT
		candidateplane.y = minBound.y;
		inside = false;
	}
	else if( ray_orig.y > maxBound.y ){
		quadrant.y = 0.0f;	// RIGHT
		candidateplane.y = maxBound.y;
		inside = false;
	}

	if( ray_orig.z < minBound.z ){
		quadrant.z = 1.0f;	// LEFT
		candidateplane.z = minBound.z;
		inside = false;
	}
	else if( ray_orig.z > maxBound.z ){
		quadrant.z = 0.0f;	// RIGHT
		candidateplane.z = maxBound.z;
		inside = false;
	}

	// Return if we are inside the box
	if( inside )
	{
		coord = ray_orig;
		return true;
	}

	// Calculate relative distance as T
	float maxT[3];
	if( quadrant.x != 2.0f && ray_dir.x != 0.0f ){
		maxT[0] = (candidateplane.x - ray_orig.x) / ray_dir.x;
	}
	else{
		maxT[0] = -1.0f;
	}

	if( quadrant.y != 2.0f && ray_dir.y != 0.0f ){
		maxT[1] = (candidateplane.y - ray_orig.y) / ray_dir.y;
	}
	else{
		maxT[1] = -1.0f;
	}

	if( quadrant.z != 2.0f && ray_dir.z != 0.0f ){
		maxT[2] = (candidateplane.z - ray_orig.z) / ray_dir.z;
	}
	else{
		maxT[2] = -1.0f;
	}

	// Get the largest T for intersection
	int whichplane = 0;
	if( maxT[0] < maxT[1] ){
		whichplane = 1;
	}
	if( maxT[whichplane] < maxT[2] ){
		whichplane = 2;
	}

	// Return if no possible intersection
	if( maxT[whichplane] < 0.0f ){
		return false;
	}

	// Check for final intersection
	if( whichplane != 0 ){
		// Do X plane intersection
		coord.x = ray_orig.x + maxT[ whichplane ] * ray_dir.x;
		if( (coord.x < minBound.x) || (coord.x > maxBound.x) )
		{
			return false;
		}
	}
	else
	{
		coord.x = candidateplane.x;
	}

	if( whichplane != 1 ){
		// Do Y plane intersection
		coord.y = ray_orig.y + maxT[ whichplane ] * ray_dir.y;
		if( (coord.y < minBound.y) || (coord.y > maxBound.y) )
		{
			return false;
		}
	}
	else
	{
		coord.y = candidateplane.y;
	}

	if( whichplane != 2 ){
		// Do Z plane intersection
		coord.z = ray_orig.z + maxT[ whichplane ] * ray_dir.z;
		if( (coord.z < minBound.z) || (coord.z > maxBound.z) )
		{
			return false;
		}
	}
	else
	{
		coord.z = candidateplane.z;
	}

	return true;
}


// Determine if a single dimension ray hits a box
bool YRayIntersectsBox( const vector_3& minBound, const vector_3& maxBound,
					    const vector_3& ray_orig, const float ray_dir, vector_3& coord )
{
	// Check our XZ dimension
	if( ray_orig.x < minBound.x ){ return false; }
	if( ray_orig.x > maxBound.x ){ return false; }
	if( ray_orig.z < minBound.z ){ return false; }
	if( ray_orig.z > maxBound.z ){ return false; }

	if( ray_dir > 0.0f )
	{
		if( ray_orig.y > maxBound.y )
		{
			return false;
		}
		else
		{
			coord		= ray_orig;
			if( ray_orig.y < minBound.y )
			{
				coord.y	= minBound.y;
			}
			return true;
		}
	}
	else
	{
		if( ray_orig.y < minBound.y )
		{
			return false;
		}
		else
		{
			coord		= ray_orig;
			if( ray_orig.y > maxBound.y )
			{
				coord.y	= maxBound.y;
			}
			return true;
		}
	}
}


/* ************************************************************************
   Function: ClosestPointOnPlane
   Description: Returns the input vector reduced to its planar component
                via the Gram-Schmidt method
   ************************************************************************ */
vector_3
ClosestPointOnPlane(plane_3 const &Plane, vector_3 const &Vector)
{
	return(Vector - Plane.GetNormal()*DistanceToPlane(Plane, Vector));
}

/* ************************************************************************
   Function: ReflectInPlane
   Description: Returns the input vector reflected about the plane such
                that (PlaneEquation(x) = -PlaneEquation(Vector)) where
				x is the return value of the function.

   TODO: I still have to prove the rest of the reflection, since
         (PlaneEquation(x) = -PlaneEquation(Vector)) is a class of vectors
		 and not a specific vector.
   ************************************************************************ */
vector_3
ReflectInPlane(plane_3 const &Plane, vector_3 const &Vector)
{
	return(Vector - real(2)*Plane.GetNormal()*
		   DistanceToPlane(Plane, Vector));
}

/* ************************************************************************
   Function: ClipLineSegment
   Description: Clips the input segment to an axis aligned bounding box
   ************************************************************************ */
void
ClipLineSegment(axis_aligned_bounding_box_3 const &BoundingBox,
				line_segment_3 &Segment)
{
	real MinimumT, MaximumT, Sign;

	// Clip to minimum and maximum of the bounding box on this axis
	MinimumT =
		(BoundingBox.GetMinimum().GetX() - Segment.GetBase().GetX()) /
		Segment.GetDirection().GetX();
	MaximumT =
		(BoundingBox.GetMaximum().GetX() - Segment.GetBase().GetX()) /
		Segment.GetDirection().GetX();

	// The segment may be running positve-to-negative along this
	// axis, so we reverse the checks if necessary
	Sign = RealIdentity;
	if(MaximumT < MinimumT)
	{
		Sign = -Sign;
		std::swap(MaximumT, MinimumT);
	}

	// See if the box clips minimum T
	if(Segment.GetMinimumT() < MinimumT)
	{
		vector_3 Normal;
		Normal.SetX(-Sign);
		Segment.SetMinimumT(MinimumT, Normal);
	}

	// See if the box clips maximum T
	if(Segment.GetMaximumT() > MaximumT)
	{
		vector_3 Normal;
		Normal.SetX(Sign);
		Segment.SetMaximumT(MaximumT, Normal);
	}

	// Clip to minimum and maximum of the bounding box on this axis
	MinimumT =
		(BoundingBox.GetMinimum().GetY() - Segment.GetBase().GetY()) /
		Segment.GetDirection().GetY();
	MaximumT =
		(BoundingBox.GetMaximum().GetY() - Segment.GetBase().GetY()) /
		Segment.GetDirection().GetY();

	// The segment may be running positve-to-negative along this
	// axis, so we reverse the checks if necessary
	Sign = RealIdentity;
	if(MaximumT < MinimumT)
	{
		Sign = -Sign;
		std::swap(MaximumT, MinimumT);
	}

	// See if the box clips minimum T
	if(Segment.GetMinimumT() < MinimumT)
	{
		vector_3 Normal;
		Normal.SetY(-Sign);
		Segment.SetMinimumT(MinimumT, Normal);
	}

	// See if the box clips maximum T
	if(Segment.GetMaximumT() > MaximumT)
	{
		vector_3 Normal;
		Normal.SetY(Sign);
		Segment.SetMaximumT(MaximumT, Normal);
	}

	// Clip to minimum and maximum of the bounding box on this axis
	MinimumT =
		(BoundingBox.GetMinimum().GetZ() - Segment.GetBase().GetZ()) /
		Segment.GetDirection().GetZ();
	MaximumT =
		(BoundingBox.GetMaximum().GetZ() - Segment.GetBase().GetZ()) /
		Segment.GetDirection().GetZ();

	// The segment may be running positve-to-negative along this
	// axis, so we reverse the checks if necessary
	Sign = RealIdentity;
	if(MaximumT < MinimumT)
	{
		Sign = -Sign;
		std::swap(MaximumT, MinimumT);
	}

	// See if the box clips minimum T
	if(Segment.GetMinimumT() < MinimumT)
	{
		vector_3 Normal;
		Normal.SetZ(-Sign);
		Segment.SetMinimumT(MinimumT, Normal);
	}

	// See if the box clips maximum T
	if(Segment.GetMaximumT() > MaximumT)
	{
		vector_3 Normal;
		Normal.SetZ(Sign);
		Segment.SetMaximumT(MaximumT, Normal);
	}
}

/* ************************************************************************
   Function: ClipLineSegment
   Description: Clips the input segment to an oriented bounding box
   ************************************************************************ */
void
ClipLineSegment(oriented_bounding_box_3 const &BoundingBox,
				line_segment_3 &Segment,
				real const Tolerance)
{
	vector_3 PlaneNormal;
	real PlaneD;
	plane_3 CurrentBoxPlane;

	{
		PlaneNormal		= -BoundingBox.GetOrientation().GetColumn_0();
		PlaneD			= BoundingBox.GetHalfDiagonal().GetX() - InnerProduct(BoundingBox.GetCenter(), PlaneNormal);

		CurrentBoxPlane.SetNormal( PlaneNormal );
		CurrentBoxPlane.SetD( PlaneD );

		// Intersect the line with the plane
		ClipLineSegment(CurrentBoxPlane, Segment, Tolerance);

		PlaneNormal		= BoundingBox.GetOrientation().GetColumn_0();
		PlaneD			= BoundingBox.GetHalfDiagonal().GetX() - InnerProduct(BoundingBox.GetCenter(), PlaneNormal);

		CurrentBoxPlane.SetNormal( PlaneNormal );
		CurrentBoxPlane.SetD( PlaneD );

		// Intersect the line with the plane
		ClipLineSegment(CurrentBoxPlane, Segment, Tolerance);
	}

	{
		PlaneNormal		= -BoundingBox.GetOrientation().GetColumn_1();
		PlaneD			= BoundingBox.GetHalfDiagonal().GetY() - InnerProduct(BoundingBox.GetCenter(), PlaneNormal);

		CurrentBoxPlane.SetNormal( PlaneNormal );
		CurrentBoxPlane.SetD( PlaneD );

		// Intersect the line with the plane
		ClipLineSegment(CurrentBoxPlane, Segment, Tolerance);

		PlaneNormal		= BoundingBox.GetOrientation().GetColumn_1();
		PlaneD			= BoundingBox.GetHalfDiagonal().GetY() - InnerProduct(BoundingBox.GetCenter(), PlaneNormal);

		CurrentBoxPlane.SetNormal( PlaneNormal );
		CurrentBoxPlane.SetD( PlaneD );

		// Intersect the line with the plane
		ClipLineSegment(CurrentBoxPlane, Segment, Tolerance);
	}

	{
		PlaneNormal		= -BoundingBox.GetOrientation().GetColumn_2();
		PlaneD			= BoundingBox.GetHalfDiagonal().GetZ() - InnerProduct(BoundingBox.GetCenter(), PlaneNormal);

		CurrentBoxPlane.SetNormal( PlaneNormal );
		CurrentBoxPlane.SetD( PlaneD );

		// Intersect the line with the plane
		ClipLineSegment(CurrentBoxPlane, Segment, Tolerance);

		PlaneNormal		= BoundingBox.GetOrientation().GetColumn_2();
		PlaneD			= BoundingBox.GetHalfDiagonal().GetZ() - InnerProduct(BoundingBox.GetCenter(), PlaneNormal);

		CurrentBoxPlane.SetNormal( PlaneNormal );
		CurrentBoxPlane.SetD( PlaneD );

		// Intersect the line with the plane
		ClipLineSegment(CurrentBoxPlane, Segment, Tolerance);
	}
}

/* ************************************************************************
   Function: ClipLineSegment
   Description: Clips the input line segment to a plane
   ************************************************************************ */
void
ClipLineSegment(plane_3 const &Plane, line_segment_3 &Segment,
				real const Tolerance)
{
	// Determine the angle of incidence between the ray and the plane
	real const Incidence = InnerProduct(Plane.GetNormal(),
										Segment.GetDirection());
	
	if(!IsZero(Incidence, Tolerance))
	{
		// The ray interesects the plane
		// Compute the intersection distance
		real T = -(InnerProduct(Plane.GetNormal(), Segment.GetBase()) +
				   Plane.GetD()) / Incidence;

		// Clip the line to the plane
		if(Incidence < RealZero)
		{
			// Plane is backfacing the segment base, so it clips maximum T
			if(Segment.GetMaximumT() > T)
			{
				Segment.SetMaximumT(T, Plane.GetNormal());
			}
		}
		else
		{
			// Plane is facing the segment base, so it clips minimum T
			if(Segment.GetMinimumT() < T)
			{
				Segment.SetMinimumT(T, Plane.GetNormal());
			}
		}
	}
}

bool ClipLineSegment( const vector_3& normal,
					  vector_3 const &Point1,
					  float const &Point1Eval,
					  vector_3 const &Point2, 
					  vector_3 &Intersect, 
					  const float Tolerance )
{
	// Determine the angle of incidence between the ray and the plane
	vector_3 direction		= Normalize( Point2 - Point1 );
	float const Incidence	= normal.DotProduct( direction );
	
	if(!IsZero(Incidence, Tolerance))
	{
		// The ray interesects the plane
		// Compute the intersection distance
		Intersect = Point1 + (direction * (-Point1Eval / Incidence));
		return true;
	}

	return false;
}

// Clip a triangle, filling the the passed vector with any extra verts as a result of the clip
// i.e. if the point was clipped off of the triangle, then the list would result in 6 vertices,
// three for each triangle.
void ClipTriangles( plane_3 const &Plane, stdx::fast_vector< vector_3 > &VectList )
{
	stdx::fast_vector< vector_3 > newVectList;

	unsigned int numVects	= VectList.size();
	newVectList.reserve( numVects<<1 );

	for( unsigned int i = 0; i < numVects; i += 3 )
	{
		const vector_3& vector0	= VectList[i];
		const vector_3& vector1	= VectList[i+1];
		const vector_3& vector2 = VectList[i+2];

		const float eval0		= Plane.Evaluate( vector0 );
		const float eval1		= Plane.Evaluate( vector1 );
		const float eval2		= Plane.Evaluate( vector2 );

		bool bOutside0			= eval0 < FLOAT_TOLERANCE;
		bool bOutside1			= eval1 < FLOAT_TOLERANCE;
		bool bOutside2			= eval2 < FLOAT_TOLERANCE;

		// Check to see if entire triangle is outside
		if( bOutside0 && bOutside1 && bOutside2 )
		{
			continue;
		}

		// Check to see if entire triangle is inside
		if( !bOutside0 && !bOutside1 && !bOutside2 )
		{
			newVectList.push_back( vector0 );
			newVectList.push_back( vector1 );
			newVectList.push_back( vector2 );
			continue;
		}

		// Clip the line segments as needed
		vector_3 intersection( DoNotInitialize );
		vector_3 oldintersection( DoNotInitialize );
		if( bOutside0 != bOutside1 )
		{
			// Clip line segment 01
			if( ClipLineSegment( Plane.GetNormal(), vector0, eval0, vector1, intersection ) )
			{
				if( !bOutside0 )
				{
					newVectList.push_back( vector0 );
					newVectList.push_back( intersection );
				}
				else if( !bOutside1 )
				{
					newVectList.push_back( intersection );
					newVectList.push_back( vector1 );
				}
				oldintersection = intersection;
			}
		}
		if( bOutside1 != bOutside2 )
		{
			// Clip line segment 12
			if( ClipLineSegment( Plane.GetNormal(), vector1, eval1, vector2, intersection ) )
			{
				if( !bOutside1 )
				{
					if( bOutside0 )
					{
						newVectList.push_back( intersection );
					}
					else
					{
						newVectList.push_back( vector1 );
						newVectList.push_back( intersection );
					}
				}
				else if( !bOutside2 )
				{
					if( !bOutside0 )
					{
						newVectList.push_back( vector2 );
						newVectList.push_back( vector2 );
						newVectList.push_back( oldintersection );
						newVectList.push_back( intersection );
					}
					else
					{
						newVectList.push_back( intersection );
						newVectList.push_back( vector2 );
					}
				}
				oldintersection = intersection;
			}
		}
		if( bOutside2 != bOutside0 )
		{
			// Clip line segment 20
			if( ClipLineSegment( Plane.GetNormal(), vector2, eval2, vector0, intersection ) )
			{
				if( !bOutside2 )
				{
					if( !bOutside1 )
					{
						newVectList.push_back( vector2 );
						newVectList.push_back( vector2 );
						newVectList.push_back( intersection );
						newVectList.push_back( oldintersection );
					}
					else
					{
						newVectList.push_back( intersection );
					}
				}
				else if( !bOutside0 )
				{
					if( !bOutside1 )
					{
						newVectList.push_back( vector0 );
						newVectList.push_back( vector0 );
						newVectList.push_back( oldintersection );
						newVectList.push_back( intersection );
					}
					else
					{
						newVectList.push_back( intersection );
					}
				}
			}
		}
	}

	// Assign the new list
	VectList.swap( newVectList );
}

void InvertedClipTriangles( const vector_3& normal, const float d, stdx::fast_vector< vector_3 >& triList )
{
	stdx::fast_vector< vector_3 > newTriList;

	unsigned int numVects	= triList.size();
	newTriList.reserve( numVects<<1 );

	for( unsigned int i = 0; i < numVects; i += 3 )
	{
		const vector_3& vector0	= triList[i];
		const vector_3& vector1	= triList[i+1];
		const vector_3& vector2 = triList[i+2];

		const float eval0		= normal.DotProduct( vector0 ) + d;
		const float eval1		= normal.DotProduct( vector1 ) + d;
		const float eval2		= normal.DotProduct( vector2 ) + d;

		bool bOutside0			= eval0 > -FLOAT_TOLERANCE;
		bool bOutside1			= eval1 > -FLOAT_TOLERANCE;
		bool bOutside2			= eval2 > -FLOAT_TOLERANCE;

		// Check to see if entire triangle is outside
		if( bOutside0 && bOutside1 && bOutside2 )
		{
			continue;
		}

		// Check to see if entire triangle is inside
		if( !bOutside0 && !bOutside1 && !bOutside2 )
		{
			newTriList.push_back( vector0 );
			newTriList.push_back( vector1 );
			newTriList.push_back( vector2 );
			continue;
		}

		// Clip the line segments as needed
		vector_3 intersection( DoNotInitialize );
		vector_3 oldintersection( DoNotInitialize );
		if( bOutside0 != bOutside1 )
		{
			// Clip line segment 01
			if( ClipLineSegment( normal, vector0, eval0, vector1, intersection ) )
			{
				if( !bOutside0 )
				{
					newTriList.push_back( vector0 );
					newTriList.push_back( intersection );
				}
				else if( !bOutside1 )
				{
					newTriList.push_back( intersection );
					newTriList.push_back( vector1 );
				}
				oldintersection = intersection;
			}
		}
		if( bOutside1 != bOutside2 )
		{
			// Clip line segment 12
			if( ClipLineSegment( normal, vector1, eval1, vector2, intersection ) )
			{
				if( !bOutside1 )
				{
					if( bOutside0 )
					{
						newTriList.push_back( intersection );
					}
					else
					{
						newTriList.push_back( vector1 );
						newTriList.push_back( intersection );
					}
				}
				else if( !bOutside2 )
				{
					if( !bOutside0 )
					{
						newTriList.push_back( vector2 );
						newTriList.push_back( vector2 );
						newTriList.push_back( oldintersection );
						newTriList.push_back( intersection );
					}
					else
					{
						newTriList.push_back( intersection );
						newTriList.push_back( vector2 );
					}
				}
				oldintersection = intersection;
			}
		}
		if( bOutside2 != bOutside0 )
		{
			// Clip line segment 20
			if( ClipLineSegment( normal, vector2, eval2, vector0, intersection ) )
			{
				if( !bOutside2 )
				{
					if( !bOutside1 )
					{
						newTriList.push_back( vector2 );
						newTriList.push_back( vector2 );
						newTriList.push_back( intersection );
						newTriList.push_back( oldintersection );
					}
					else
					{
						newTriList.push_back( intersection );
					}
				}
				else if( !bOutside0 )
				{
					if( !bOutside1 )
					{
						newTriList.push_back( vector0 );
						newTriList.push_back( vector0 );
						newTriList.push_back( oldintersection );
						newTriList.push_back( intersection );
					}
					else
					{
						newTriList.push_back( intersection );
					}
				}
			}
		}
	}

	// Assign the new list
	triList.swap( newTriList );
}

/* ************************************************************************
   Function: DextralColumns
   Description: Returns a dextral coordinate system with the input vector
                as its first column 
   ************************************************************************ */
matrix_3x3
DextralColumns(vector_3 const &XAxis)
{
	vector_3 CrossVector(RealIdentity, RealIdentity, RealIdentity);

	real ax	= AbsoluteValue( XAxis.GetX() );
	real ay = AbsoluteValue( XAxis.GetY() );
	real az = AbsoluteValue( XAxis.GetZ() );

	if( ax > ay && ax > az )
	{
		CrossVector.SetX( RealZero );
	}
	else if( ay > ax && ay > az )
	{
		CrossVector.SetY( RealZero );
	}
	else if( az > ax && az > ay )
	{
		CrossVector.SetZ( RealZero );
	}

	vector_3 const YAxis = Normalize(CrossProduct(CrossVector, XAxis));
	vector_3 const ZAxis = Normalize(CrossProduct(XAxis, YAxis));

	matrix_3x3 DextralMatrix(MatrixColumns(XAxis, YAxis, ZAxis));

	gpassert(IsOrthonormal(DextralMatrix));
	return(DextralMatrix);
}

/* ************************************************************************
   Function: DextralColumnsY
   Description: Returns a dextral coordinate system with the input vector
                as its second column -- added by biddle
   ************************************************************************ */
matrix_3x3
DextralColumnsY(vector_3 const &YAxis)
{
	vector_3 CrossVector(RealIdentity, RealIdentity, RealIdentity);

	real ax	= AbsoluteValue( YAxis.GetX() );
	real ay = AbsoluteValue( YAxis.GetY() );
	real az = AbsoluteValue( YAxis.GetZ() );

	if( ax > ay && ax > az )
	{
		CrossVector.SetX( RealZero );
	}
	else if( ay > ax && ay > az )
	{
		CrossVector.SetY( RealZero );
	}
	else if( az > ax && az > ay )
	{
		CrossVector.SetZ( RealZero );
	}

	vector_3 const ZAxis = Normalize(CrossProduct(CrossVector, YAxis));
	vector_3 const XAxis = Normalize(CrossProduct(YAxis, ZAxis));

	matrix_3x3 DextralMatrix(MatrixColumns(XAxis, YAxis, ZAxis));

	gpassert(IsOrthonormal(DextralMatrix));
	return(DextralMatrix);

}

/* ************************************************************************
   Function: SinistralColumns
   Description: Returns a sinistral coordinate system with the input vector
                as its first column

   TODO: I admit that this function only exists because the word Sinistral
         is so cool.  I don't think anyone's ever going to actually call
		 it.
   ************************************************************************ */
matrix_3x3
SinistralColumns(vector_3 const &XAxis)
{
	// TODO: Make an actual sinistral matrix generator
	matrix_3x3 SinistralMatrix(DextralColumns(XAxis));

	// Convert the dextral coordinate system to a sinistral one
	// via inversion of the Z axis
	SinistralMatrix.SetElement_02( -SinistralMatrix.GetElement_02() );
	SinistralMatrix.SetElement_12( -SinistralMatrix.GetElement_12() );
	SinistralMatrix.SetElement_22( -SinistralMatrix.GetElement_22() );

	gpassert(IsOrthonormal(SinistralMatrix));
	return(SinistralMatrix);
}

/* ************************************************************************
   Function: AxisRotationColumns
   Description: Returns a matrix that rotates column vectors around an
                arbitrary axis by the input angle
   ************************************************************************ */
matrix_3x3
AxisRotationColumns(vector_3 const &Axis, real const AngleInRadians)
{
	matrix_3x3 mat( DoNotInitialize );

#ifndef	USE_SLOW_MATH

	__asm
	{
		mov		ecx, Axis			// Load axis

		fld		AngleInRadians		// Load the angle
		fsincos						// Calculate sine and cosine

		fld1						// Load 1.0f
		fsub	st,st(1)			// 1.0f - AngleCos

		// ---------------------------------------------------------

		fld		dword ptr [ecx+8]	// Load the z
		fmul	st,st(3)			// Multiply by AngleSin

		fld		dword ptr [ecx]		// Load the x
		fmul	dword ptr [ecx+4]	// Multiply by y

		fmul	st,st(2)			// Multiply by 1-AngleCos

		fld		st					// Copy x*y*1-AngleCos
		fsub	st,st(2)			// Subtract z*AngleSin

		fstp	mat.v01				// Store in v01

		faddp	st(1),st			// Add z*AngleSin
		fstp	mat.v10				// Store in v10

		// ---------------------------------------------------------

		fld		dword ptr [ecx+4]	// Load the y
		fmul	st,st(3)			// Multiply by AngleSin

		fld		dword ptr [ecx]		// Load the x
		fmul	dword ptr [ecx+8]	// Multiply by z

		fmul	st,st(2)			// Multiply by 1-AngleCos

		fld		st					// Copy x*z*1-AngleCos
		fsub	st,st(2)			// Subtract y*AngleSin

		fstp	mat.v20				// Store in v20

		faddp	st(1),st			// Add y*AngleSin
		fstp	mat.v02				// Store in v02

		// ---------------------------------------------------------

		fld		dword ptr [ecx]		// Load the x
		fmul	st,st(3)			// Multiply by AngleSin

		fld		dword ptr [ecx+4]	// Load the y
		fmul	dword ptr [ecx+8]	// Multiply by z

		fmul	st,st(2)			// Multiply by 1-AngleCos

		fld		st					// Copy y*z*1-AngleCos
		fsub	st,st(2)			// Subtract x*AngleSin

		fstp	mat.v12				// Store in v12

		faddp	st(1),st			// Add x*AngleSin
		fstp	mat.v21				// Store in v21

		// ---------------------------------------------------------

		fld		dword ptr [ecx]		// Load x
		fmul	st,st				// Square x
		fld1						// Load 1.0f
		fsub	st,st(1)			// 1.0f - x2
		fmul	st,st(3)			// AngleCos * (1.0f - x2)
		faddp	st(1),st			// x2 + (AngleCos * (1.0f - x2))
		fstp	mat.v00				// Store in v00

		// ---------------------------------------------------------

		fld		dword ptr [ecx+4]	// Load y
		fmul	st,st				// Square y
		fld1						// Load 1.0f
		fsub	st,st(1)			// 1.0f - y2
		fmul	st,st(3)			// AngleCos * (1.0f - y2)
		faddp	st(1),st			// y2 + (AngleCos * (1.0f - y2))
		fstp	mat.v11				// Store in v11

		// ---------------------------------------------------------

		fld		dword ptr [ecx+8]	// Load z
		fmul	st,st				// Square z
		fld1						// Load 1.0f
		fsub	st,st(1)			// 1.0f - z2
		fmul	st,st(3)			// AngleCos * (1.0f - z2)
		faddp	st(1),st			// z2 + (AngleCos * (1.0f - z2))
		fstp	mat.v22				// Store in v22

		// ---------------------------------------------------------

		ffree	st					// Free 1.0f - AngleCos
		ffree	st(1)				// Free AngleCos
		ffree	st(2)				// Free AngleSin

	}

#else

	float AngleSin, AngleCos;
	SINCOSF( AngleInRadians, AngleSin, AngleCos );

	float Vx	= Axis.x;
	float Vx2	= Square(Vx);
	
	float Vy	= Axis.y;
	float Vy2	= Square(Vy);
	
	float Vz	= Axis.z;
	float Vz2	= Square(Vz);

	mat.SetElement_00( Vx2 + AngleCos * (1 - Vx2) );
	mat.SetElement_01( Vx * Vy * (1 - AngleCos) - Vz * AngleSin );
	mat.SetElement_02( Vz * Vx * (1 - AngleCos) + Vy * AngleSin );
	
	mat.SetElement_10( Vx * Vy * (1 - AngleCos) + Vz * AngleSin );
	mat.SetElement_11( Vy2 + AngleCos * (1 - Vy2) );
	mat.SetElement_12( Vy * Vz * (1 - AngleCos) - Vx * AngleSin );
	
	mat.SetElement_20( Vz * Vx * (1 - AngleCos) - Vy * AngleSin );
	mat.SetElement_21( Vy * Vz * (1 - AngleCos) + Vx * AngleSin );
	mat.SetElement_22( Vz2 + AngleCos * (1 - Vz2) );

#endif

	DEBUG_FPU_STACK_VALIDATE(0);
	return mat;
}

/* ************************************************************************
   Function: XRotationColumns
   Description: Returns a matrix that rotates column vectors around the X
                axis by the input angle
   ************************************************************************ */
matrix_3x3
XRotationColumns(real const AngleInRadians)
{
	matrix_3x3 xmat;

#ifndef USE_SLOW_MATH

	__asm
	{
		fld		AngleInRadians		// Load the angle
		fsincos						// Calculate the sine and cosine

		fst		xmat.v11			// Store cosine
		fstp	xmat.v22			// Store cosine

		fst		xmat.v21			// Store sine
		fchs						// Change sign
		fstp	xmat.v12			// Store negative sine
	}

#else

	float sinrad, cosrad;
	SINCOSF( AngleInRadians, sinrad, cosrad );

	xmat.SetElement_11( cosrad );
	xmat.SetElement_21( sinrad );
	xmat.SetElement_12( -xmat.GetElement_21() );
	xmat.SetElement_22( xmat.GetElement_11() );

#endif

	DEBUG_FPU_STACK_VALIDATE(0);
	return xmat;
}

/* ************************************************************************
   Function: YRotationColumns
   Description: Returns a matrix that rotates column vectors around the Y
                axis by the input angle
   ************************************************************************ */
matrix_3x3
YRotationColumns(real const AngleInRadians)
{
	matrix_3x3 ymat;

#ifndef USE_SLOW_MATH

	__asm
	{
		fld		AngleInRadians		// Load the angle
		fsincos						// Calculate the sine and cosine

		fst		ymat.v00			// Store the cosine
		fstp	ymat.v22			// Store the cosine

		// $$$$ v02 should really store the negative sine and v20 the positive

		fst		ymat.v02			// Store the sine
		fchs						// Change sign
		fstp	ymat.v20			// Store the negative sine
	}

#else

	float sinrad, cosrad;
	SINCOSF( AngleInRadians, sinrad, cosrad );

	ymat.SetElement_00( cosrad );
	ymat.SetElement_02( sinrad );
	ymat.SetElement_20( -ymat.GetElement_02() );
	ymat.SetElement_22( ymat.GetElement_00() );

#endif

	DEBUG_FPU_STACK_VALIDATE(0);
	return ymat;
}

/* ************************************************************************
   Function: ZRotationColumns
   Description: Returns a matrix that rotates column vectors around the Z
                axis by the input angle
   ************************************************************************ */
matrix_3x3
ZRotationColumns(real const AngleInRadians)
{
	matrix_3x3 zmat;

#ifndef USE_SLOW_MATH

	__asm
	{
		fld		AngleInRadians		// Load the angle
		fsincos						// Calculate the sine and cosine

		fst		zmat.v00			// Store the cosine
		fstp	zmat.v11			// Store the cosine

		fst		zmat.v10			// Store the sine
		fchs						// Change the sign
		fstp	zmat.v01			// Store the negative sine
	}

#else

	float sinrad, cosrad;
	SINCOSF( AngleInRadians, sinrad, cosrad );

	zmat.SetElement_00( cosrad );
	zmat.SetElement_10( sinrad );
	zmat.SetElement_01( -zmat.GetElement_10() );
	zmat.SetElement_11( zmat.GetElement_00() );

#endif

	DEBUG_FPU_STACK_VALIDATE(0);
	return zmat;
}

//****************************************************
//****************************************************
//****************************************************

//Not inlined while developing/debugging --biddle
Quat QuatFromDirection( vector_3 const &dir )
{
	// Too often we are given a meaningless 'zero' vector to work with -- biddle
	if (IsZero(dir)) {
		gpwarningf(("Can't compute a Quat for DirVec [%f,%f,%f]",dir.x,dir.y,dir.z) );
	}

	if (IsZero(dir)) {

		// There is no rotation, return an identity
		return Quat::IDENTITY;

	}
/*
	// v0 = vector_3(0,0,1);					// The implied 'zero' axis
	vector_3 v1 = Normalize(dir);

	// Look for 180 degree rotations 
	if (IsEqual(v1.z,-1.0f)) {

		// This is a 180deg rotation, we can pick anything that works
		return Quat( 0.0f, 1.0f, 0.0f, 0.0f );

	} else if (IsEqual(v1.z, 1.0f)) {

		// There is no rotation, return an identity
		return Quat::IDENTITY;

	}
*/

	// The 'dir' is often not normalized, doing a sqrt is cheaper
	// than normalizing somewhere else (unless the 'other' code is
	// normalizing too) -- biddle

	float theta = ATAN2F( dir.x, dir.z ) * 0.5f;

	float base = SQRTF(dir.x*dir.x + dir.z*dir.z);

	float phi = -ATAN2F( dir.y, base ) * 0.5f;

	// Calculate our components
	float A, B;
	SINCOSF( theta, B, A );
	float C, D;
	SINCOSF( phi, D, C );

	DEBUG_FPU_STACK_VALIDATE(0);

	return Quat( A*D, B*C, -B*D, A*C );

	/*

	// Revised method from Melax in Game Gems

	//vector_3 c = vector_3( -v1.y,v1.x, 0.0f);	// CrossProduct(v0,v1);
	// float d = DotProduct(v0,v1);

	float s = sqrtf((1.0f+v1.z)*2.0f);

	float i = 1.0f/s;

	return Quat( -v1.y*i, v1.x*i, 0.0f, 0.5f*s ); //return Quat( c.x*s, c.y*s, c.z*s, w);

	*/
}

Quat QuatFromXZComponents( float x, float z )
{
	// Too often we are given a meaningless 'zero' vector to work with -- biddle
	if( IsZero( x ) && IsZero( z ) )
	{
		gpwarningf(("Can't compute a Quat for XZ components (%f,%f)",x,z) );
	}

	// Look for 180 degree rotations 
	if( IsZero( x ) )
	{
		if( IsNegative( z ) )
		{
			// This is a 180deg rotation, we can pick anything that works
			return Quat( 0.0f, 1.0f, 0.0f, 0.0f );
		}
		else
		{
			// There is no rotation, return an identity
			// NOTE: This is the fall-through case for a ZERO vector
			return Quat::IDENTITY;
		}
	}

#ifndef USE_SLOW_MATH

	Quat	quat( DoNotInitialize );
	__asm
	{
		fld		x
		mov		dword ptr [quat], 0

		fld		z
		mov		dword ptr [quat+8], 0

		fpatan							// Partial arctangent to get theta
		fmul	FLOAT_ONEHALF			// Divide theta by 2

		fsincos							// Get the sine and cosine

		fstp	dword ptr [quat+12]		// W is cos(theta/2)
		fstp	dword ptr [quat+4]		// Y is sin(theta/2)
	}

	DEBUG_FPU_STACK_VALIDATE(0);
	return quat;

#else

	float theta = atan2f( x, z ) * 0.5f;

	// Calculate our components
	float A, B;
	SINCOSF( theta, B, A );

	DEBUG_FPU_STACK_VALIDATE(0);
	return Quat(0, B, 0, A);

#endif
}

void QuatFromXZComponents( float x, float z, Quat& q )
{
	// Too often we are given a meaningless 'zero' vector to work with -- biddle
	if( IsZero( x ) && IsZero( z ) )
	{
		gpwarningf(("Can't compute a Quat for XZ components (%f,%f)",x,z) );
	}

	// Look for 180 degree rotations 
	if( IsZero( x ) )
	{
		if( IsNegative( z ) )
		{
			// This is a 180deg rotation, we can pick anything that works
			q = Quat( 0.0f, 1.0f, 0.0f, 0.0f );
		}
		else
		{
			// There is no rotation, return an identity
			// NOTE: This is the fall-through case for a ZERO vector
			q = Quat::IDENTITY;
		}
	}

#ifndef USE_SLOW_MATH

	__asm
	{
		mov		eax,q
		fld		x
		mov		dword ptr [eax], 0

		fld		z
		mov		dword ptr [eax+8], 0

		fpatan							// Partial arctangent to get theta
		fmul	FLOAT_ONEHALF			// Divide theta by 2

		fsincos							// Get the sine and cosine

		fstp	dword ptr [eax+12]		// W is cos(theta/2)
		fstp	dword ptr [eax+4]			// Y is sin(theta/2)
	}

	DEBUG_FPU_STACK_VALIDATE(0);
	return;

#else

	float theta = atan2f( x, z ) * 0.5f;

	// Calculate our components
	float A, B;
	SINCOSF( theta, B, A );

	q = Quat(0, B, 0, A);

	DEBUG_FPU_STACK_VALIDATE(0);

#endif
}


// Blend between two matrices
matrix_3x3 BlendMatrices( const matrix_3x3& fromMatrix, const matrix_3x3& toMatrix, const float amount )
{
	// Build quaternions
	Quat fromQuat( fromMatrix );
	Quat toQuat( toMatrix );

	// Slerp
	Quat resultQuat( DoNotInitialize );
	Slerp( resultQuat, fromQuat, toQuat, amount );

	// Return matrix
	return resultQuat.BuildMatrix().Orthonormal_T();
}

