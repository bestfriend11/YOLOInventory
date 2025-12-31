#pragma once
#if !defined(ORIENTED_BOUNDING_BOX_3_H)
/* ========================================================================
   Header File: oriented_bounding_box_3.h
   Description: 
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#if !defined(VECTOR_3_H)
#include "vector_3.h"
#endif

#if !defined(MATRIX_3X3_H)
#include "matrix_3x3.h"
#endif

/* ************************************************************************
   Class: oriented_bounding_box_3
   Description: Describes an oriented bounding box in 3D
   ************************************************************************ */
class oriented_bounding_box_3
{
public:
	// Read access
	inline vector_3 const &GetCenter(void) const;
	inline matrix_3x3 const &GetOrientation(void) const;
	inline vector_3 const &GetHalfDiagonal(void) const;

	// Write access
	void SetCenter(const vector_3& NewVal)			{ Center = NewVal;			}
	void SetOrientation(const matrix_3x3& NewVal)	{ Orientation = NewVal;		}
	void SetHalfDiagonal(const vector_3& NewVal)	{ HalfDiagonal = NewVal;	}

	// Existence
	inline oriented_bounding_box_3(
		vector_3 const &Center = vector_3(0,0,0),
		vector_3 const &HalfDiagonal = vector_3(0,0,0),
		matrix_3x3 const &Orientation = matrix_3x3()
		);
	
private:
	// Positioning
	vector_3 Center;
	matrix_3x3 Orientation;

	// Bounds
	vector_3 HalfDiagonal;
};

/* ========================================================================
   Inline Functions
   ======================================================================== */

/* ************************************************************************
   Function: GetCenter
   Description: Returns the center of the bounding box
   ************************************************************************ */
inline vector_3 const &oriented_bounding_box_3::
GetCenter(void) const
{
	return(Center);
}
	
/* ************************************************************************
   Function: GetOrientation
   Description: Returns the orientation of the bounding box
   ************************************************************************ */
inline matrix_3x3 const &oriented_bounding_box_3::
GetOrientation(void) const
{
	return(Orientation);
}

/* ************************************************************************
   Function: GetHalfDiagonal
   Description: Returns half the diagonal span of the bounding box as if
                its orientation was the identity matrix
   ************************************************************************ */
inline vector_3 const &oriented_bounding_box_3::
GetHalfDiagonal(void) const
{
	return(HalfDiagonal);
}

/* ************************************************************************
   Function: oriented_bounding_box_3
   Description: Stores the position and bounds of the bounding box
   ************************************************************************ */
inline oriented_bounding_box_3::
oriented_bounding_box_3(vector_3 const &CenterInit,
						vector_3 const &HalfDiagonalInit,
						matrix_3x3 const &OrientationInit)
		: Center(CenterInit),
		  HalfDiagonal(HalfDiagonalInit),
		  Orientation(OrientationInit)
{
}

#define ORIENTED_BOUNDING_BOX_3_H
#endif
