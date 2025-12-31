/*********************************************************************************
**
**						oriented_bounding_box_3
**
**		See .h file for additional information
**
*********************************************************************************/

#include "precomp_gpcore.h"
#include "oriented_bounding_box_3.h"
#include "space_3.h"


// Collision
bool oriented_bounding_box_3::CheckForContact( const oriented_bounding_box_3& collide_box )
{
	matrix_3x3 Relative_Rotation	= Transpose( m_Orientation );
	vector_3 Relative_Translation	= Relative_Rotation * ( collide_box.GetCenter() - m_Center );
	Relative_Rotation				= Multiply_T( Relative_Rotation, collide_box.GetOrientation() );

	const vector_3& collide_hdiag	= collide_box.GetHalfDiagonal();

	matrix_3x3 Bf( DoNotInitialize );
  
	// Bf = fabs(B)
	Bf.v00	= FABSF( Relative_Rotation.v00 ) + FLOAT_TOLERANCE;
	Bf.v01	= FABSF( Relative_Rotation.v01 ) + FLOAT_TOLERANCE;
	Bf.v02	= FABSF( Relative_Rotation.v02 ) + FLOAT_TOLERANCE;
	Bf.v10	= FABSF( Relative_Rotation.v10 ) + FLOAT_TOLERANCE;
	Bf.v11	= FABSF( Relative_Rotation.v11 ) + FLOAT_TOLERANCE;
	Bf.v12	= FABSF( Relative_Rotation.v12 ) + FLOAT_TOLERANCE;
	Bf.v20	= FABSF( Relative_Rotation.v20 ) + FLOAT_TOLERANCE;
	Bf.v21	= FABSF( Relative_Rotation.v21 ) + FLOAT_TOLERANCE;
	Bf.v22	= FABSF( Relative_Rotation.v22 ) + FLOAT_TOLERANCE;

	// if any of these tests are one-sided, then the polyhedra are disjoint
	int r = 1;

	// A1 x A2 = A0
	r &= ( FABSF( Relative_Translation.x ) <= ( m_HalfDiagonal.x + collide_hdiag.x * Bf.v00 + collide_hdiag.y * Bf.v01 + collide_hdiag.z * Bf.v02 ) );
	if( !r ) { return false; }

	// B1 x B2 = B0
	float s = Relative_Translation.x * Relative_Rotation.v00 + Relative_Translation.y * Relative_Rotation.v10 + Relative_Translation.z * Relative_Rotation.v20;
	r &= ( FABSF( s ) <= ( collide_hdiag.x + m_HalfDiagonal.x * Bf.v00 + m_HalfDiagonal.y * Bf.v10 + m_HalfDiagonal.z * Bf.v20 ) );
	if( !r ) { return false; }

	// A2 x A0 = A1
	r &= ( FABSF( Relative_Translation.y ) <= ( m_HalfDiagonal.y + collide_hdiag.x * Bf.v10 + collide_hdiag.y * Bf.v11 + collide_hdiag.z * Bf.v12 ) );
	if( !r ) { return false; }

	// A0 x A1 = A2
	r &= ( FABSF( Relative_Translation.z ) <= ( m_HalfDiagonal.z + collide_hdiag.x * Bf.v20 + collide_hdiag.y * Bf.v21 + collide_hdiag.z * Bf.v22 ) );
	if( !r ) { return false; }

	// B2 x B0 = B1
	s = Relative_Translation.x * Relative_Rotation.v01 + Relative_Translation.y * Relative_Rotation.v11 + Relative_Translation.z * Relative_Rotation.v21;
	r &= ( FABSF( s ) <= ( collide_hdiag.y + m_HalfDiagonal.x * Bf.v01 + m_HalfDiagonal.y * Bf.v11 + m_HalfDiagonal.z * Bf.v21 ) );
	if( !r ) { return false; }

	// B0 x B1 = B2
	s = Relative_Translation.x * Relative_Rotation.v02 + Relative_Translation.y * Relative_Rotation.v12 + Relative_Translation.z * Relative_Rotation.v22;
	r &= ( FABSF( s ) <= ( collide_hdiag.z + m_HalfDiagonal.x * Bf.v02 + m_HalfDiagonal.y * Bf.v12 + m_HalfDiagonal.z * Bf.v22 ) );
	if( !r ) { return false; }

	// A0 x B0
	s = Relative_Translation.z * Relative_Rotation.v10 - Relative_Translation.y * Relative_Rotation.v20;
	r &= ( FABSF( s ) <= ( m_HalfDiagonal.y * Bf.v20 + m_HalfDiagonal.z * Bf.v10 + collide_hdiag.y * Bf.v02 + collide_hdiag.z * Bf.v01 ) );
	if( !r ) { return false; }

	// A0 x B1
	s = Relative_Translation.z * Relative_Rotation.v11 - Relative_Translation.y * Relative_Rotation.v21;
	r &= ( FABSF( s ) <= ( m_HalfDiagonal.y * Bf.v21 + m_HalfDiagonal.z * Bf.v11 + collide_hdiag.x * Bf.v02 + collide_hdiag.z * Bf.v00 ) );
	if( !r ) { return false; }

	// A0 x B2
	s = Relative_Translation.z * Relative_Rotation.v12 - Relative_Translation.y * Relative_Rotation.v22;
	r &= ( FABSF( s ) <= ( m_HalfDiagonal.y * Bf.v22 + m_HalfDiagonal.z * Bf.v12 + collide_hdiag.x * Bf.v01 + collide_hdiag.y * Bf.v00 ) );
	if( !r ) { return false; }

	// A1 x B0
	s = Relative_Translation.x * Relative_Rotation.v20 - Relative_Translation.z * Relative_Rotation.v00;
	r &= ( FABSF( s ) <= ( m_HalfDiagonal.x * Bf.v20 + m_HalfDiagonal.z * Bf.v00 + collide_hdiag.y * Bf.v12 + collide_hdiag.z * Bf.v11 ) );
	if( !r ) { return false; }

	// A1 x B1
	s = Relative_Translation.x * Relative_Rotation.v21 - Relative_Translation.z * Relative_Rotation.v01;
	r &= ( FABSF( s ) <= ( m_HalfDiagonal.x * Bf.v21 + m_HalfDiagonal.z * Bf.v01 + collide_hdiag.x * Bf.v12 + collide_hdiag.z * Bf.v10 ) );
	if( !r ) { return false; }

	// A1 x B2
	s = Relative_Translation.x * Relative_Rotation.v22 - Relative_Translation.z * Relative_Rotation.v02;
	r &= ( FABSF( s ) <= ( m_HalfDiagonal.x * Bf.v22 + m_HalfDiagonal.z * Bf.v02 + collide_hdiag.x * Bf.v11 + collide_hdiag.y * Bf.v10 ) );
	if( !r ) { return false; }

	// A2 x B0
	s = Relative_Translation.y * Relative_Rotation.v00 - Relative_Translation.x * Relative_Rotation.v10;
	r &= ( FABSF( s ) <= ( m_HalfDiagonal.x * Bf.v10 + m_HalfDiagonal.y * Bf.v00 + collide_hdiag.y * Bf.v22 + collide_hdiag.z * Bf.v21 ) );
	if( !r ) { return false; }

	// A2 x B1
	s = Relative_Translation.y * Relative_Rotation.v01 - Relative_Translation.x * Relative_Rotation.v11;
	r &= ( FABSF( s ) <= ( m_HalfDiagonal.x * Bf.v11 + m_HalfDiagonal.y * Bf.v01 + collide_hdiag.x * Bf.v22 + collide_hdiag.z * Bf.v20 ) );
	if( !r ) { return false; }

	// A2 x B2
	s = Relative_Translation.y * Relative_Rotation.v02 - Relative_Translation.x * Relative_Rotation.v12;
	r &= ( FABSF( s ) <= ( m_HalfDiagonal.x * Bf.v12 + m_HalfDiagonal.y * Bf.v02 + collide_hdiag.x * Bf.v21 + collide_hdiag.y * Bf.v20 ) );
	if( !r ) { return false; }

	return true;
}

// Is the given point inside me
bool oriented_bounding_box_3::PointInOrientedBox( const vector_3& point )
{
	// Calculate the local origin and run a basic axis-aligned test on the point
	return PointInBox( -m_HalfDiagonal, m_HalfDiagonal, (Transpose( m_Orientation ) * ( point - m_Center )) );
}

// Does ray intersect box
bool oriented_bounding_box_3::RayIntersectsOrientedBox( const vector_3& origin, const vector_3& dir, vector_3* pReturnCoord )
{
	// Calculate the inverse orientation
	matrix_3x3 inverseOrientation	= Transpose( m_Orientation );

	// Calculate the local origin and direction and run a basic axis-aligned test on the ray
	vector_3 coord( DoNotInitialize );
	bool retValue = RayIntersectsBox( -m_HalfDiagonal, m_HalfDiagonal, inverseOrientation * ( origin - m_Center ), inverseOrientation * dir, coord );

	// Put the return coordinate back into the space of the client if requested
	if( retValue && pReturnCoord )
	{
		*pReturnCoord	= m_Center + ( m_Orientation * coord );
	}

	return retValue;
}

// Does segment intersect box
bool oriented_bounding_box_3::SegmentIntersectsOrientedBox( const vector_3& origin, const vector_3& end, vector_3* pReturnCoord )
{
	// Call the ray intersection to generate an intersection point if it exists
	vector_3 intersection_point( DoNotInitialize );
	vector_3 dir	= end - origin;

	if( RayIntersectsOrientedBox( origin, dir, &intersection_point ) )
	{
		// See if the length to the intersection point is within the endpoints of our segment
		if( Length2( intersection_point - origin ) <= dir.Length2() )
		{
			// Fill in return coordinate if requested
			if( pReturnCoord )
			{
				*pReturnCoord	= intersection_point;
			}
			return true;
		}
	}

	return false;
}