#pragma once

#ifndef SPACE_2_H
#define SPACE_2_H

/* ========================================================================
   Header File: space_2.h
   Description: Declares convenient functions for working with spatial
                relations in 2D
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "Point2.H"

/* ========================================================================
   Forward Declarations
   ======================================================================== */

struct Ray2 
{
	Point2 m_pos;
	Point2 m_dir;
	Ray2(const Point2& p,const Point2& d) : m_pos(p), m_dir(d) {};
};

struct Circle2 
{
	Point2 m_center;
	float m_radius;
	Circle2(const Point2& c, float r) : m_center(c), m_radius(r) {};
};

struct OBB2 
{
	Point2 m_center;
	Point2 m_half_diag;
	Point2 m_orient;
	OBB2() {};
};

template <class _II> void ConstructOBB2( const _II& beg, const _II& end, OBB2& obb)
{

	_II it;

	obb.m_center = Point2::ZERO;
	float count = 0;
	for (it = beg;it != end; ++it)
	{
		// Assumes we are in the same node!!!
		obb.m_center += Point2((*it).m_Point);
		count+=1;
	}

	float invSize = INVERSEF( count );
	
	obb.m_center *= invSize;

	// compute covariances of points
	Point2 covmat[2];
	covmat[0] = Point2::ZERO;
	covmat[1] = Point2::ZERO;

	for (it = beg ; it != end ; ++it)
    {
        Point2 diff = (*it).m_Point - obb.m_center;
        covmat[0].x += diff.x*diff.x;
        covmat[0].y += diff.x*diff.y;
        covmat[1].y += diff.y*diff.y;
    }

    covmat[0].x *= invSize;
    covmat[0].y *= invSize;
    covmat[1].x = covmat[0].y;
    covmat[1].y *= invSize;

	/*
		covariance matrix is:
		| XX , XY |
		| XY , YY |

		eigenvector(s) of the covariance matrix 
		define the axes we need
	*/

	Point2 eigvals;

	if (  (IsZero(covmat[0].x) && IsZero(covmat[0].y))
		||(IsZero(covmat[1].x) && IsZero(covmat[1].y)))
	{
		obb.m_half_diag = Point2::ZERO;
		obb.m_center = Point2::ZERO;
		obb.m_orient = Point2(1,0);
	}
	else
	{
		EigenSystem(covmat,eigvals,obb.m_orient);

		Point2 max(-FLOAT_INFINITE,-FLOAT_INFINITE);
		Point2 min( FLOAT_INFINITE, FLOAT_INFINITE);

		for (it = beg ; it != end ; ++it)
		{
			const Point2& diff = (*it).m_Point;

			float range;
			range = diff.DotProduct(obb.m_orient);
			if (range > max.x) max.x = range;
			if (range < min.x) min.x = range;

			range = diff.DotProduct(Point2(-obb.m_orient.y,obb.m_orient.x));
			if (range > max.y) max.y = range;
			if (range < min.y) min.y = range;

		}

		obb.m_half_diag = (max-min)*0.5f;

		// Do the eigenvals match the half_diag?

		// Rotate the eigenvector by the x_axis to get the aligned Major Axis
		obb.m_center = ChangeBasis(obb.m_orient,(max+min) * 0.5f);
	}

}



bool DirectedSegmentPenetratesCircle2D(const Point2& p0,  const Point2& p1,	 const Circle2& c, float& tau);

#endif	// SPACE_2_H
