#pragma once
#if !defined(SPHERE_3_H)
/* ========================================================================
   Header File: sphere_3.h
   Description: Declares sphere_3
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#if !defined(VECTOR_3_H)
#include "vector_3.h"
#endif

/* ************************************************************************
   Class: sphere_3
   Description: Represents a sphere in 3D
   ************************************************************************ */
class sphere_3
{
public:
	// Read access
	inline real GetRadius(void) const;
	inline vector_3 const &GetCenter(void) const;

	// Write access
	inline sphere_3 &SetRadius(real const Radius);
	inline sphere_3 &SetCenter(vector_3 const &Center);

	// Construction
	inline sphere_3(real const Radius, vector_3 const &Center);

private:
	real Radius;
	vector_3 Center;
};

/* ========================================================================
   Inline Functions
   ======================================================================== */

/* ************************************************************************
   Function: GetRadius
   Description: Returns the radius of the sphere
   ************************************************************************ */
inline real sphere_3::
GetRadius(void) const
{
	return(Radius);
}

/* ************************************************************************
   Function: GetCenter
   Description: Returns the center of the sphere
   ************************************************************************ */
inline vector_3 const &sphere_3::
GetCenter(void) const
{
	return(Center);
}

/* ************************************************************************
   Function: SetRadius
   Description: Sets the radius of the sphere to the input scalar
   ************************************************************************ */
inline sphere_3 &sphere_3::
SetRadius(real const RadiusInit)
{
	Radius = RadiusInit;
	return(*this);
}

/* ************************************************************************
   Function: SetCenter
   Description: Sets the center of the sphere to the input vector
   ************************************************************************ */
inline sphere_3 &sphere_3::
SetCenter(vector_3 const &CenterInit)
{
	Center = CenterInit;
	return(*this);
}

/* ************************************************************************
   Function: sphere_3
   Description: Constructs a sphere with the input radius and center
   ************************************************************************ */
inline sphere_3::
sphere_3(real const RadiusInit, vector_3 const &CenterInit)
		: Radius(RadiusInit),
		  Center(CenterInit)
{
}

#define SPHERE_3_H
#endif
