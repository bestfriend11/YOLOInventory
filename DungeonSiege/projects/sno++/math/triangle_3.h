#pragma once
#if !defined(TRIANGLE_3_H)
/* ========================================================================
   Header File: triangle_3.h
   Declares: Useful triangle functions

   WARNING: THESE ARE NEW FUNCTIONS FOR THE MATH LIBRARY.  THEY HAVE NOT
            YET BEEN TESTED.  THEY SHOULD ABSOLUTELY POSITIVELY NOT BE
			USED BY ANYONE UNTIL I HAVE TESTED THEM.  YOU HAVE BEEN
			WARNED.  - cmu
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "vector_3.h"

/* ========================================================================
   Forward Declarations
   ======================================================================== */
namespace math
{
	// Intersections
	inline bool RayIntersectsTriangle(
		vector_3 const &RayOrigin, vector_3 const &RayDirection,
		vector_3 const &Vertex0, vector_3 const &Vertex1,
		vector_3 const &Vertex2);
	bool RayIntersectsTriangle(
		vector_3 const &RayOrigin, vector_3 const &RayDirection,
		vector_3 const &Vertex0, vector_3 const &Vertex1,
		vector_3 const &Vertex2,
		real &TCoordinate, real &UCoordinate, real &VCoordinate);

	// Culled Intersections
	inline bool RayIntersectsCulledTriangle(
		vector_3 const &RayOrigin, vector_3 const &RayDirection,
		vector_3 const &Vertex0, vector_3 const &Vertex1,
		vector_3 const &Vertex2);
	bool RayIntersectsCulledTriangle(
		vector_3 const &RayOrigin, vector_3 const &RayDirection,
		vector_3 const &Vertex0, vector_3 const &Vertex1,
		vector_3 const &Vertex2,
		real &TCoordinate, real &UCoordinate, real &VCoordinate);
};

/* ========================================================================
   Inline Functions
   ======================================================================== */
inline bool math::
RayIntersectsTriangle(
	vector_3 const &RayOrigin, vector_3 const &RayDirection,
	vector_3 const &Vertex0, vector_3 const &Vertex1,
	vector_3 const &Vertex2)
{
	real TCoordinate, UCoordinate, VCoordinate;
	return(RayIntersectsTriangle(RayOrigin, RayDirection,
								 Vertex0, Vertex1, Vertex2,
								 TCoordinate, UCoordinate, VCoordinate));
}

inline bool math::
RayIntersectsCulledTriangle(
	vector_3 const &RayOrigin, vector_3 const &RayDirection,
	vector_3 const &Vertex0, vector_3 const &Vertex1,
	vector_3 const &Vertex2)
{
	real TCoordinate, UCoordinate, VCoordinate;
	return(RayIntersectsCulledTriangle(
		RayOrigin, RayDirection,
		Vertex0, Vertex1, Vertex2,
		TCoordinate, UCoordinate, VCoordinate));
}

#define TRIANGLE_3_H
#endif
