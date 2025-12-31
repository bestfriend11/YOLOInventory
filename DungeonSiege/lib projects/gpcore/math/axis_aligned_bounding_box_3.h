#pragma once
#if !defined(AXIS_ALIGNED_BOUNDING_BOX_3_H)
/* ========================================================================
   Header File: axis_aligned_bounding_box_3.h
   Description: Declares axis_aligned_bounding_box_3
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#if !defined(VECTOR_3_H)
#include "vector_3.h"
#endif

/* ************************************************************************
   Class: axis_aligned_bounding_box_3
   Description: Describes a 3D bounding box aligned along the primary axes
   ************************************************************************ */
class axis_aligned_bounding_box_3
{
public:
	// Read access
	inline vector_3 const &GetMinimum(void) const;
	inline vector_3 const &GetMaximum(void) const;

	// Existence
	inline axis_aligned_bounding_box_3(vector_3 const &Minimum,
									   vector_3 const &Maximum);	
	
private:
	// Bounds
	vector_3 Minimum;
	vector_3 Maximum;
};

/* ========================================================================
   Inline Functions
   ======================================================================== */

/* ************************************************************************
   Function: GetMinimum
   Description: Returns the minimum bound, on all axes, of the bounding box
   ************************************************************************ */
inline vector_3 const &axis_aligned_bounding_box_3::
GetMinimum(void) const
{
	return(Minimum);
}

/* ************************************************************************
   Function: GetMaximum
   Description: Returns the maximum bound, on all axes, of the bounding box
   ************************************************************************ */
inline vector_3 const &axis_aligned_bounding_box_3::
GetMaximum(void) const
{
	return(Maximum);
}

/* ************************************************************************
   Function: axis_aligned_bounding_box_3
   Description: Stores the bounds of the bounding box
   ************************************************************************ */
inline axis_aligned_bounding_box_3::
axis_aligned_bounding_box_3(vector_3 const &MinimumInit,
							vector_3 const &MaximumInit)
		: Minimum(MinimumInit),
		  Maximum(MaximumInit)
{
}

#define AXIS_ALIGNED_BOUNDING_BOX_3_H
#endif
