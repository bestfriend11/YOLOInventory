#include "precomp_gpcore.h"
#include "quat.h"

// These functions were derived from Shoemake's ArcBall and Polar Decomposition
// support routines as published in GGems IV
//
// Some code comments were swiped out of Jeff Landers' Game Developer articles
//
const Quat Quat::ZERO			( 0.0f, 0.0f, 0.0f, 0.0f );
const Quat Quat::IDENTITY		( 0.0f, 0.0f, 0.0f, 1.0f );
const Quat Quat::INVALID		( FLOAT_INFINITE, FLOAT_INFINITE, FLOAT_INFINITE, FLOAT_INFINITE );
const Quat Quat::INDEFINITE		( FLOAT_INDEFINITE, FLOAT_INDEFINITE, FLOAT_INDEFINITE, FLOAT_INDEFINITE );

Quat::Quat(const matrix_3x3& m)
{

	float s;

	if ((m.v00 + m.v11 + m.v22) >= 0.0f) {

		s = SQRTF(m.v00 + m.v11 + m.v22 + 1.0f);

		m_w = s * 0.5f;

		s = 0.5f/s;

		m_x = ((m.v21 - m.v12) * s );
		m_y = ((m.v02 - m.v20) * s );
		m_z = ((m.v10 - m.v01) * s );

	} else {

		// Need to swizzle the columns to get a matrix with a positive trace
		// There are TONS of places that just provide this code...
		// NO ONE EXPLAINS IT! they all just quote our buddy Ken Shoemake!

		// The token pasting trick derived from Shoemake's original code

		#define swizzle(i,j,k,I,J,K)							\
		s = SQRTF( m.v##I##I - m.v##J##J - m.v##K##K + 1.0f );	\
		m_##i = s*0.5f;											\
		s = 0.5f / s;											\
		m_##j = (m.v##I##J + m.v##J##I) * s;					\
		m_##k = (m.v##K##I + m.v##I##K) * s;					\
		m_w   = (m.v##K##J - m.v##J##K) * s;

		if ((m.v00 > m.v11) && (m.v00 > m.v22)) {

			swizzle(x,y,z,0,1,2)

		} else if (m.v11 > m.v22) {

			swizzle(y,z,x,1,2,0)

		} else {

			swizzle(z,x,y,2,0,1)

		}
	}

	gpassertf((
		(*this != INDEFINITE) &&
		(*this != ZERO)
		),("Attempting to return garbage quat [%f,%f,%f,%f] from matrix construction", m_x,m_y,m_z,m_w));

}

void
Quat::BuildMatrix(matrix_3x3& out) const {

	VALIDATE_QUATERNION(*this,"Can't build a matrix (bad src)");

    float norm = (m_x*m_x) + (m_y*m_y) +(m_z*m_z) + (m_w*m_w);

	if (IsZero(norm))
	{
		out = matrix_3x3::IDENTITY;
		return;
	}

	float s = 2.0f/norm;

	float xs = m_x*s,	ys = m_y*s,		zs = m_z*s;
	float wx = m_w*xs,	wy = m_w*ys,	wz = m_w*zs;
	float xx = m_x*xs,	xy = m_x*ys,	xz = m_x*zs;
	float yy = m_y*ys,	yz = m_y*zs,	zz = m_z*zs;

	out.v00 = (1.0f - (yy + zz));
	out.v10 = (xy + wz);
	out.v20 = (xz - wy);

	out.v01 = (xy - wz);
	out.v11 = (1.0f - (xx + zz));
	out.v21 = (yz + wx);

	out.v02 = (xz + wy);
	out.v12 = (yz - wx);
	out.v22 = (1.0f - (xx + yy));
}

void
Slerp(Quat& qO, const Quat& qL,const Quat& qR, const real blend)
{
	gpassert( (&qO != &qL) && (&qO != &qR) );
	VALIDATE_QUATERNION(qL,"Can't slerp (bad left side)");
	VALIDATE_QUATERNION(qR,"Can't slerp (bad right side)");

/*
	if (qL == qR) {
		gpwarningf(("Slerping EQUAL quaternions [%f,%f,%f,%f]->[%f,%f,%f,%f] waste of time",
			qL.m_x,qL.m_y,qL.m_z,qL.m_w,
			qR.m_x,qR.m_y,qR.m_z,qR.m_w
			))

		return qL;
	}
*/
	real cos_omega = (qL.m_x*qR.m_x) + (qL.m_y*qR.m_y) + (qL.m_z*qR.m_z) + (qL.m_w*qR.m_w);

	real alpha, beta;

	if(cos_omega < 0.0f )
	{
		// Flip start quaternion
		qO = -qR;
		cos_omega = -cos_omega;

	} else {
		qO = qR;
	}


	if ((1.0 - cos_omega) < 0.001 /*RealTolerance*/) {
		// Just do a linear interpolation
		alpha = 1.0f - blend;
		beta  = blend;
	} else {

		real omega = ACOSF(cos_omega);
		real inv_sin_omega = INVERSEF( SINF(omega) );

		alpha = SINF((1.0f - blend) * omega) * inv_sin_omega;
		beta  = SINF(blend * omega) * inv_sin_omega;
	}


	qO.m_x = (alpha * qL.m_x) + (beta * qO.m_x);
	qO.m_y = (alpha * qL.m_y) + (beta * qO.m_y);
	qO.m_z = (alpha * qL.m_z) + (beta * qO.m_z);
	qO.m_w = (alpha * qL.m_w) + (beta * qO.m_w);

	VALIDATE_QUATERNION(qO,"Can't slerp (output)");
}

bool SlerpTowards(Quat& result, const Quat& qL,const Quat& qR, const real theta)
{

	// Rotate qL towards qR by (at most) angle theta
	// return TRUE iff result is set to qR

	VALIDATE_QUATERNION(qL,"Can't SlerpTheta (bad left side)");
	VALIDATE_QUATERNION(qR,"Can't SlerpTheta (bad right side)");

	Quat tempQ(DoNotInitialize);

	float cos_omega = (qL.m_x*qR.m_x) + (qL.m_y*qR.m_y) + (qL.m_z*qR.m_z) + (qL.m_w*qR.m_w);

	float alpha, beta;

	if(cos_omega < 0.0f )
	{
		// Flip start quaternion
		tempQ = -qR;
		cos_omega = -cos_omega;

	}
	else
	{
		tempQ = qR;
	}

	if ((1.0 - cos_omega) < 0.001 /*RealTolerance*/)
	{
		result = qR;
		return true;
	}
	else
	{

		float omega = ACOSF(cos_omega);
		float inv_sin_omega = INVERSEF( SINF(omega) );

		float diff = omega - theta;

		if (!IsPositive(diff))
		{
			result = qR;
			return true;
		}

		alpha = SINF(diff)	* inv_sin_omega;
		beta  = SINF(theta) * inv_sin_omega;
	}

	result.m_x = (alpha * qL.m_x) + (beta * tempQ.m_x);
	result.m_y = (alpha * qL.m_y) + (beta * tempQ.m_y);
	result.m_z = (alpha * qL.m_z) + (beta * tempQ.m_z);
	result.m_w = (alpha * qL.m_w) + (beta * tempQ.m_w);

	return false;

}

/*

//*******************************************************************8
//*******************************************************************8


  Other people's code used to research this Quaternion shit


//*******************************************************************8
//*******************************************************************8


void SlerpQuat(tQuaternion *quat1,tQuaternion *quat2,float slerp, tQuaternion *result)
{
/// Local Variables ///////////////////////////////////////////////////////////
	real omega,cosom,sinom,scale0,scale1;
///////////////////////////////////////////////////////////////////////////////
	// USE THE DOT PRODUCT TO GET THE COSINE OF THE ANGLE BETWEEN THE
	// QUATERNIONS
	cosom = quat1->x * quat2->x +
			quat1->y * quat2->y +
			quat1->z * quat2->z +
			quat1->w * quat2->w;

	// MAKE SURE WE ARE TRAVELING ALONG THE SHORTER PATH
	if ((1.0 + cosom) > DELTA)
	{
		if ((1.0 - cosom) > DELTA) {
			omega = acos(cosom);
			sinom = sin(omega);
			scale0 = sin((1.0 - slerp) * omega) / sinom;
			scale1 = sin(slerp * omega) / sinom;
		} else {
			scale0 = 1.0 - slerp;
			scale1 = slerp;
		}
		result->x = scale0 * quat1->x + scale1 * quat2->x;
		result->y = scale0 * quat1->y + scale1 * quat2->y;
		result->z = scale0 * quat1->z + scale1 * quat2->z;
		result->w = scale0 * quat1->w + scale1 * quat2->w;
	} else {
		result->x = -quat2->y;
		result->y = quat2->x;
		result->z = -quat2->w;
		result->w = quat2->z;
		scale0 = sin((1.0 - slerp) * (float)HALF_PI);
		scale1 = sin(slerp * (float)HALF_PI);
		result->x = scale0 * quat1->x + scale1 * result->x;
		result->y = scale0 * quat1->y + scale1 * result->y;
		result->z = scale0 * quat1->z + scale1 * result->z;
		result->w = scale0 * quat1->w + scale1 * result->w;
	}

}
// SlerpQuat  /////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Name: D3DMath_SlerpQuaternions()
// Desc: Compute a quaternion which is the spherical linear interpolation
//       between two other quaternions by dvFraction.
//-----------------------------------------------------------------------------
VOID D3DMath_QuaternionSlerp( FLOAT& Qx, FLOAT& Qy, FLOAT& Qz, FLOAT& Qw,
                              FLOAT Ax, FLOAT Ay, FLOAT Az, FLOAT Aw,
                              FLOAT Bx, FLOAT By, FLOAT Bz, FLOAT Bw,
                              FLOAT fAlpha )
{
    FLOAT fScale1;
    FLOAT fScale2;

    // Compute dot product, aka cos(theta):
    FLOAT fCosTheta = Ax*Bx + Ay*By + Az*Bz + Aw*Bw;

    if( fCosTheta < 0.0f )
    {
        // Flip start quaternion
        Ax = -Ax; Ay = -Ay; Ax = -Az; Aw = -Aw;
        fCosTheta = -fCosTheta;
    }

    if( fCosTheta + 1.0f > 0.05f )
    {
        // If the quaternions are close, use linear interploation
        if( 1.0f - fCosTheta < 0.05f )
        {
            fScale1 = 1.0f - fAlpha;
            fScale2 = fAlpha;
        }
        else // Otherwise, do spherical interpolation
        {
            FLOAT fTheta    = (FLOAT)acos( fCosTheta );
            FLOAT fSinTheta = (FLOAT)sin( fTheta );

            fScale1 = (FLOAT)sin( fTheta * (1.0f-fAlpha) ) / fSinTheta;
            fScale2 = (FLOAT)sin( fTheta * fAlpha ) / fSinTheta;
        }
    }
    else
    {
        Bx = -Ay;
        By =  Ax;
        Bz = -Aw;
        Bw =  Az;
        fScale1 = (FLOAT)sin( g_PI * (0.5f - fAlpha) );
        fScale2 = (FLOAT)sin( g_PI * fAlpha );			<<<<<<<<------------ Bug? should be Alpha/2
    }

    Qx = fScale1 * Ax + fScale2 * Bx;
    Qy = fScale1 * Ay + fScale2 * By;
    Qz = fScale1 * Az + fScale2 * Bz;
    Qw = fScale1 * Aw + fScale2 * Bw;
}
*/
/*
			real omega = acos(cos_omega);
			real sin_omega = sin(omega);
			alpha = sin((1.0 - blend) * omega) / sin_omega;
			beta  = sin(blend * omega) / sin_omega;

		scale0 = sin((1.0 - slerp) * (float)HALF_PI);
		scale1 = sin(slerp * (float)HALF_PI);

        fScale1 = (FLOAT)sin( g_PI * (0.5f - fAlpha) );
        fScale2 = (FLOAT)sin( g_PI * fAlpha );

*/

