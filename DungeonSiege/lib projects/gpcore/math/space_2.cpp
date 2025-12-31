/* ========================================================================
   Source File: space_2.cpp
   Description: Implements convenient functions for working with spatial
                relations in 2D
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "precomp_gpcore.h"
#include "space_2.h"

bool DirectedSegmentPenetratesCircle2D(const Point2& p0, const Point2& p1, const Circle2& circle, float& retval_tau)
{

	// Modified from Eberly's Magic Software 3D library and Foley van Dam
    // set up quadratic Q(t) = a*t^2 + 2*b*t + c

	// where 
	// a = v.v
	// b = v.u
	// c = (u.u) - (r*r)
	

	// u = (p0-co)
	// v = (p1-p0)

    Point2 u = p0 - circle.m_center;
	Point2 v = p1 - p0;

    float c = u.Length2() - (circle.m_radius*circle.m_radius);

	if (c <= 0.0f){
		// We are already inside the circle
		retval_tau = 0.0f;
		return true;
	}

    float b = u.DotProduct(v);

    if ( IsPositive(b) ) {
		// We are pointing away from the circle
		return false;
	}

    float a = v.Length2();

    if ( IsZero(a) ) {
		// The segment is zero length
		return false;
	}

	float discrim = b*b-a*c;

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
