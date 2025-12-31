#pragma once
#ifndef ORIENTED_BOUNDING_BOX_3_H
#define ORIENTED_BOUNDING_BOX_3_H
/*********************************************************************************
**
**						oriented_bounding_box_3
**
**		Defines functinality for an oriented bounding volume.
**
*********************************************************************************/

#include "vector_3.h"
#include "matrix_3x3.h"

class oriented_bounding_box_3
{
public:

	// Existence
	inline oriented_bounding_box_3(
		vector_3 const &Center			= vector_3::ZERO,
		vector_3 const &HalfDiagonal	= vector_3::ZERO,
		matrix_3x3 const &Orientation	= matrix_3x3::IDENTITY )
		: m_Center(Center)
		, m_HalfDiagonal(HalfDiagonal)
		, m_Orientation(Orientation)
	{
	}
	inline oriented_bounding_box_3( do_not_initialize )
		: m_Center( DoNotInitialize )
		, m_HalfDiagonal( DoNotInitialize )
		, m_Orientation( DoNotInitialize )
	{
	}

	// Read access
	inline vector_3 const &GetCenter() const			{ return m_Center; }
	inline matrix_3x3 const &GetOrientation() const		{ return m_Orientation; }
	inline vector_3 const &GetHalfDiagonal() const		{ return m_HalfDiagonal; }

	inline vector_3 &GetCenter()						{ return m_Center; }
	inline matrix_3x3 &GetOrientation()					{ return m_Orientation; }
	inline vector_3 &GetHalfDiagonal()					{ return m_HalfDiagonal; }

	// Write access
	void SetCenter( const vector_3& center )			{ m_Center			= center; }
	void SetOrientation( const matrix_3x3& orient )		{ m_Orientation		= orient; }
	void SetHalfDiagonal( const vector_3& diag )		{ m_HalfDiagonal	= diag; }

	// Collision
	bool CheckForContact( const oriented_bounding_box_3& collide_box );

	// Is the given point inside me
	bool PointInOrientedBox( const vector_3& point );

	// Does ray intersect box
	bool RayIntersectsOrientedBox( const vector_3& origin, const vector_3& dir, vector_3* pReturnCoord = NULL );

	// Does segment intersect box
	bool SegmentIntersectsOrientedBox( const vector_3& origin, const vector_3& end, vector_3* pReturnCoord = NULL );

private:

	// Positioning
	vector_3	m_Center;
	matrix_3x3	m_Orientation;

	// Bounds
	vector_3	m_HalfDiagonal;
};

#endif
