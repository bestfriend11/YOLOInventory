#include "precomp_world.h"

#include "Physcalc.h"

#include "vector_3.h"
#include "space_3.h"



namespace Physcalc {


float
CalculateFlatRange( float velocity, float angle, float gravity )
{
	return (( velocity * velocity ) * Sin(2*angle) )/ gravity;
}



bool
IsInRange(	const vector_3 &difference_vector,
			float velocity, float gravity )
{
	const float x = difference_vector.x;
	const float z = difference_vector.z;

	const float x0 = (float)sqrt(x * x + z * z);
	const float x2 = x0 * x0;
	const float y0 = difference_vector.y;

	const float SQRT_x2_PLUS_y2 = (float)sqrt( x2 + y0 * y0 );

	const float SIN_2theta_MINUS_phi = ((y0 + ((gravity * x2 ) / 
								(velocity*velocity))) / SQRT_x2_PLUS_y2 );

	if( SIN_2theta_MINUS_phi > 1.0f ) {
		return false;
	}
	return true;
}



bool	CalculateFiringVector(	const vector_3 &difference_vector,
								float velocity,
								float gravity,
								float x_error, float y_error,
								vector_3 &velocity_vector )
{
	const float x = difference_vector.x;
	const float z = difference_vector.z;

	const float x0 = (float)sqrt(x * x + z * z);
	const float x2 = x0 * x0;
	const float y0 = difference_vector.y;

	vector_3	nDirection;

	if( !IsZero( difference_vector ) ) {
		nDirection = Normalize( difference_vector );
	}

	const float SQRT_x2_PLUS_y2 = (float)sqrt( x2 + y0 * y0 );

	const float SIN_2theta_MINUS_phi = ((y0 + ((gravity * x2 ) / 
								(velocity*velocity))) / SQRT_x2_PLUS_y2 );

	if( SIN_2theta_MINUS_phi > 1.0f ) {
		return false;	// can't be hit, too low of velocity
	}

	const float right_term = ASin( SIN_2theta_MINUS_phi );
	const float left_term = ASin( y0 / SQRT_x2_PLUS_y2 );

	float thetasin, thetacos;
	SINCOSF( 0.5f * ( left_term + right_term ), thetasin, thetacos );

	// now that we have the angle determine the velocity vector
	
	vector_3	z_axis( nDirection );
	vector_3	y_axis( vector_3::UP );
	vector_3	x_axis = CrossProduct( y_axis, z_axis);

	if( IsZero(Length(x_axis)) )
	{
		x_axis = vector_3::NORTH;
	} else {
		x_axis = Normalize(x_axis);
	}

	z_axis = CrossProduct( x_axis, y_axis );
	matrix_3x3	orientation;
	orientation = MatrixColumns( x_axis, y_axis, z_axis );

	const vector_3	velocity_direction( 0, velocity * thetasin, velocity * thetacos );

	if( ( x_error != 0 ) || ( y_error != 0 ) ) {
		float Sin_phi, Cos_phi;
		SINCOSF( y_error, Sin_phi, Cos_phi );
		float Sin_rho, Cos_rho;
		SINCOSF( x_error, Sin_rho, Cos_rho );

		const vector_3 col_0( Cos_rho,	Sin_phi*Sin_rho,	-Cos_phi*Sin_rho);
		const vector_3 col_1( 0,		Cos_phi,			Sin_phi );
		const vector_3 col_2( Sin_rho,	-Cos_rho*Sin_phi,	Cos_phi*Cos_rho );

		const matrix_3x3 error( MatrixColumns( col_0, col_1, col_2 ) );

		orientation = orientation * error;
	}

	velocity_vector = orientation * velocity_direction;

	return true;
}



bool
CalculateFiringAngle(const vector_3 &difference_vector,
					float velocity, 
					float gravity,
					float &angle )
{
	const float x = difference_vector.x;
	const float z = difference_vector.z;

	const float x0 = (float)sqrt(x * x + z * z);
	const float x2 = x0 * x0;
	const float y0 = difference_vector.y;

	const float SQRT_x2_PLUS_y2 = (float)sqrt( x2 + y0 * y0 );

	const float SIN_2theta_MINUS_phi = ((y0 + ((gravity * x2 ) / 
								(velocity*velocity))) / SQRT_x2_PLUS_y2 );

	if( SIN_2theta_MINUS_phi > 1.0f ) {
		return false;	// can't be hit, too low of velocity
	}

	const float right_term = ASin( SIN_2theta_MINUS_phi );
	const float left_term = ASin( y0 / SQRT_x2_PLUS_y2 );

	angle = 0.5f * ( left_term + right_term );

	return true;
}



float
CalculateTimeToImpact( 	const vector_3 &difference_vector,
						float velocity, float firing_angle, float gravity )
{
	const float y0 = difference_vector.y;

	const float Velocity_SIN_theta = velocity * Sin( firing_angle );
	const float RootPart = (float)sqrt( (Velocity_SIN_theta * Velocity_SIN_theta) - 2 * gravity * y0 );

	const float solution1 = ( Velocity_SIN_theta - RootPart ) / gravity;
	const float solution2 = ( Velocity_SIN_theta + RootPart ) / gravity;

	if( ( y0 != 0 ) ) {
		if( solution1 > solution2 ) {
			return solution1;
		} else {
			return solution2;
		}
	}

	if( solution1 < solution2 ) {
		return solution1;
	} 

	if( solution1 <= 0 ) {
		return solution2;
	}

	if( solution2 <= 0 ) {
		return solution1;
	}
	return 0;
}


}	// namespace Physcalc
