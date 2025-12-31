#pragma once
#if !defined(LINE_SEGMENT_3_H)
/* ========================================================================
   Header File: line_segment_3.h
   Description: Declares line_segment_3 and the most common functions
                associated with it (more complex operations are declared
				in space_3.h)
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "vector_3.h"


/* ************************************************************************
   Class: line_segment_3
   Description: Represents a 3D line segment

   TODO: Note that this class is experimental, and should not be used
   extensively by anyone until I verify that it is designed
   appropriately.
   ************************************************************************ */
class line_segment_3
{
public:
	// Endpoint read access
	inline vector_3 GetMinimumEndpoint(void) const;
	inline vector_3 GetMaximumEndpoint(void) const;
	
	// Parametric read access
	inline real GetMinimumT(void) const;
	inline real GetMaximumT(void) const;
	inline vector_3 const &GetBase(void) const;
	inline vector_3 const &GetDirection(void) const;
	// NOTE: Direction points from minimum to maximum

	// Parametric write access
	inline void SetMinimumT(real const MinimumT,
							vector_3 const &MinimumNormal);
	inline void SetMaximumT(real const MaximumT,
							vector_3 const &MaximumNormal);

	// Existence
	inline line_segment_3(vector_3 const &Base, vector_3 const &Direction,
						  real const MinimumT, real const MaximumT);
	inline line_segment_3(vector_3 const &MinimumEndpoint,
						  vector_3 const &MaximumEndpoint);
	
private:
	// Endpoint conversion (does not set normals)
	void DefineFromEndpoints(vector_3 const &MinimumEndpoint,
							 vector_3 const &MaximumEndpoint);
	
	// Segment is defined as running from
	// (Base + MinimumT * Direction) to (Base + MaximumT * Direction)
	vector_3 Base, Direction;
	real MinimumT, MaximumT;

	// Normals are kept for clipping information: they are the normals
	// of the last plane to clip the minimum or maximum end of the
	// segment, and are initialized to be (MinimumNormal = Direction)
	// and (MaximumNormal = -Direction) as if the segment were cut
	// by two parallel planes perpendicular to the segment on either end.
	vector_3 MinimumNormal, MaximumNormal;

	// TODO: Add clip history so that, in debug mode, the segment
	//       keeps track of every time it was clipped
};

/* ========================================================================
   Inline Functions
   ======================================================================== */

/* ************************************************************************
   Function: GetMinimumEndpoint
   Description: Returns the current, clipped minimum endpoint of the
                segment
   ************************************************************************ */
inline vector_3 line_segment_3::
GetMinimumEndpoint(void) const
{
	return(Base + MinimumT*Direction);
}

/* ************************************************************************
   Function: GetMaximumEndpoint
   Description: Returns the current, clipped maximum endpoint of the
                segment
   ************************************************************************ */
inline vector_3 line_segment_3::
GetMaximumEndpoint(void) const
{
	return(Base + MaximumT*Direction);
}
	
/* ************************************************************************
   Function: GetMinimumT
   Description: Returns the minimum value of T along the segment that has
                not been clipped
   ************************************************************************ */
inline real line_segment_3::
GetMinimumT(void) const
{
	return(MinimumT);
}

/* ************************************************************************
   Function: GetMaximumT
   Description: Returns the maximum value of T along the segment that has
                not been clipped
   ************************************************************************ */
inline real line_segment_3::
GetMaximumT(void) const
{
	return(MaximumT);
}

/* ************************************************************************
   Function: GetBase
   Description: Returns the base of the segment, used for clipping
                calculations; it is the minimum endpoint before clipping
   ************************************************************************ */
inline vector_3 const &line_segment_3::
GetBase(void) const
{
	return(Base);
}

/* ************************************************************************
   Function: GetDirection
   Description: Returns the direction of the segment starting from the base
                or minimum endpoint and heading toward the maximum endpoint
   ************************************************************************ */
inline vector_3 const &line_segment_3::
GetDirection(void) const
{
	return(Direction);
}

/* ************************************************************************
   Function: SetMinimumT
   Description: Sets the minimum value of T along the segment that has not
                been clipped
   ************************************************************************ */
inline void line_segment_3::
SetMinimumT(real const MinimumTInit,
			vector_3 const &MinimumNormalInit)
{
	MinimumT = MinimumTInit;
	MinimumNormal = MinimumNormalInit;
}

/* ************************************************************************
   Function: SetMaximumT
   Description: Sets the maximum value of T along the segment that has not
                been clipped
   ************************************************************************ */
inline void line_segment_3::
SetMaximumT(real const MaximumTInit,
			vector_3 const &MaximumNormalInit)
{
	MaximumT = MaximumTInit;
	MaximumNormal = MaximumNormalInit;
}

/* ************************************************************************
   Function: line_segment_3
   Description: Creates the default line segment from a base, direction,
                and pair of T's
   ************************************************************************ */
inline line_segment_3::
line_segment_3(vector_3 const &BaseInit, vector_3 const &DirectionInit,
			   real const MinimumTInit, real const MaximumTInit)
		: Base(BaseInit),
		  Direction(DirectionInit),
		  MinimumT(MinimumTInit),
		  MaximumT(MaximumTInit),
		  MinimumNormal(Direction),
		  MaximumNormal(-Direction)
{
}

/* ************************************************************************
   Function: line_segment_3
   Description: Creates the default line segment from two endpoints
   ************************************************************************ */
inline line_segment_3::
line_segment_3(vector_3 const &MinimumEndpoint,
			   vector_3 const &MaximumEndpoint)
{
	DefineFromEndpoints(MinimumEndpoint, MaximumEndpoint);
	MinimumNormal = Direction;
	MaximumNormal = -Direction;
}

/* ************************************************************************
   Function: DefineFromEndpoints
   Description: Defines the segment via endpoints
   ************************************************************************ */
inline void line_segment_3::
DefineFromEndpoints(vector_3 const &MinimumEndpoint,
					vector_3 const &MaximumEndpoint)
{
	Base = MinimumEndpoint;
	Direction = Normalize(MaximumEndpoint - Base);
	MinimumT = RealZero;
	MaximumT = Length(MaximumEndpoint - Base);	
}

#define LINE_SEGMENT_3_H
#endif
