#pragma once
#ifndef _PHYSCALC_H_
#define _PHYSCALC_H_


/*=======================================================================================

  Physcalc.h

  purpose:	Utility functions for generic physics calculations which are not dependant on
			propellant.

  author:	Rick Saenz

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/



namespace Physcalc {

	// Utility functions

	// Calculate the range of a projectile on even ground
	float		CalculateFlatRange( float velocity, float angle, float gravity );

	// Test to see if something is hittable
	// direction is target_position - firing_position
	bool		IsInRange( const vector_3 &difference_vector,
							float velocity, float gravity );

	// Calculates the firing Vector to hit a target - false if unhittable with given velocity
	// direction is target_position - firing_position
	bool		CalculateFiringVector(	const vector_3 &difference_vector,
										float velocity,
										float gravity,
										float x_error, float y_error,
										vector_3 &velocity_vector );

	// Calculates the firing angle to hit a target - false if unhittable with given velocity
	// direction is target_position - firing_position
	bool		CalculateFiringAngle(	const vector_3 &difference_vector,
										float velocity,
										float gravity,
										float &angle );

	// Calculates unobstructed flight time in seconds
	// direction is target_position - firing_position
	float		CalculateTimeToImpact( 	const vector_3 &difference_vector,
										float velocity, float firing_angle, float gravity );
}



#endif	//_PHYSCALC_H_
