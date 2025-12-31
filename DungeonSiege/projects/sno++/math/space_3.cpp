/* ========================================================================
   Source File: space_3.cpp
   Description: Implements convenient functions for working with spatial
                relations in 3D
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "gpcore.h"
#include "space_3.h"
#include "matrix_3x3.h"
#include "line_segment_3.h"
#include "oriented_bounding_box_3.h"
#include "axis_aligned_bounding_box_3.h"

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
		Swap(MaximumT, MinimumT);
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
		Swap(MaximumT, MinimumT);
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
		Swap(MaximumT, MinimumT);
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
	real const AngleSin = Sin(AngleInRadians);
	real const AngleCos = Cos(AngleInRadians);

	real const Vx = Axis.GetX();
	real const Vx2 = Square(Vx);
	
	real const Vy = Axis.GetY();
	real const Vy2 = Square(Vy);
	
	real const Vz = Axis.GetZ();
	real const Vz2 = Square(Vz);

	matrix_3x3 AxisRotationMatrix(DoNotInitialize);
	
	AxisRotationMatrix.SetElement_00( Vx2 + AngleCos * (1 - Vx2) );
	AxisRotationMatrix.SetElement_01( Vx * Vy * (1 - AngleCos) - Vz * AngleSin );
	AxisRotationMatrix.SetElement_02( Vz * Vx * (1 - AngleCos) + Vy * AngleSin );
	
	AxisRotationMatrix.SetElement_10( Vx * Vy * (1 - AngleCos) + Vz * AngleSin );
	AxisRotationMatrix.SetElement_11( Vy2 + AngleCos * (1 - Vy2) );
	AxisRotationMatrix.SetElement_12( Vy * Vz * (1 - AngleCos) - Vx * AngleSin );
	
	AxisRotationMatrix.SetElement_20( Vz * Vx * (1 - AngleCos) - Vy * AngleSin );
	AxisRotationMatrix.SetElement_21( Vy * Vz * (1 - AngleCos) + Vx * AngleSin );
	AxisRotationMatrix.SetElement_22( Vz2 + AngleCos * (1 - Vz2) );

	return(AxisRotationMatrix);
}

/* ************************************************************************
   Function: XRotationColumns
   Description: Returns a matrix that rotates column vectors around the X
                axis by the input angle
   ************************************************************************ */
matrix_3x3
XRotationColumns(real const AngleInRadians)
{
	matrix_3x3 XRotationMatrix;

	XRotationMatrix.SetElement_11( Cos(AngleInRadians) );
	XRotationMatrix.SetElement_21( Sin(AngleInRadians) );
	XRotationMatrix.SetElement_12( -XRotationMatrix.GetElement_21() );
	XRotationMatrix.SetElement_22( XRotationMatrix.GetElement_11() );

	return(XRotationMatrix);
}

/* ************************************************************************
   Function: YRotationColumns
   Description: Returns a matrix that rotates column vectors around the Y
                axis by the input angle
   ************************************************************************ */
matrix_3x3
YRotationColumns(real const AngleInRadians)
{
	matrix_3x3 YRotationMatrix;

	YRotationMatrix.SetElement_00( Cos(AngleInRadians) );
	YRotationMatrix.SetElement_02( Sin(AngleInRadians) );
	YRotationMatrix.SetElement_20( -YRotationMatrix.GetElement_02() );
	YRotationMatrix.SetElement_22( YRotationMatrix.GetElement_00() );

	return(YRotationMatrix);
}

/* ************************************************************************
   Function: ZRotationColumns
   Description: Returns a matrix that rotates column vectors around the Z
                axis by the input angle
   ************************************************************************ */
matrix_3x3
ZRotationColumns(real const AngleInRadians)
{
	matrix_3x3 ZRotationMatrix;

	ZRotationMatrix.SetElement_00( Cos(AngleInRadians) );
	ZRotationMatrix.SetElement_10( Sin(AngleInRadians) );
	ZRotationMatrix.SetElement_01( -ZRotationMatrix.GetElement_10() );
	ZRotationMatrix.SetElement_11( ZRotationMatrix.GetElement_00() );

	return(ZRotationMatrix);
}
