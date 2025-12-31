#include "gpcore.h"
#include "gpdebugoutput.h"
#include "quat.h"

// These functions were derived from Shoemake's ArcBall and Polar Decomposition 
// support routines as published in GGems IV
//
// Some code comments were swiped out of Jeff Landers' Game Developer articles
//

tQuat::tQuat(const matrix_3x3& m)
{

	real d0 = m.GetElement_00();
	real d1 = m.GetElement_11();
	real d2 = m.GetElement_22();

	// check the trace
	real s, tr;

	tr = d0 + d1 + d2;

	if (tr >= 0.0) {

		// here the 1 represents the lower left of the (implied) homogeneous matrix
		// not entirely obvious at all now was it?
		s = sqrt (tr + 1.0);

	    m_w = s * 0.5f;

		s = 0.5f / s;

		m_x = ((m.GetElement_21() - m.GetElement_12()) * s );
		m_y = ((m.GetElement_02() - m.GetElement_20()) * s );
		m_z = ((m.GetElement_10() - m.GetElement_01()) * s );

	} else {

		// trace is negative
		gpassertm(tr >= 0.0,"Converting a matrix with a negative trace");
//		gpwarning("Converting a matrix with a negative trace");

		if (d0 > d1) {
			if (d0 > d2) {

				//largest diagonal element = 00;

				// i = 0 j = 1 k = 2;
				/*
				jk = m.GetElement_12();
				kj = m.GetElement_21();

				ij = m.GetElement_01();
				ji = m.GetElement_10();

				ik = m.GetElement_02();
				ki = m.GetElement_20();

				*/

				s = sqrt ((d0 - (d1 + d2)) + 1.0);
      
				m_x = s * 0.5;
			  
				s = 0.5 / s;

				m_y = (m.GetElement_01() + m.GetElement_10()) * s;
				m_z = (m.GetElement_20() + m.GetElement_02()) * s;
				m_w = (m.GetElement_21() - m.GetElement_12()) * s;

				return;

			}

		} else {

			if (d1 > d2) {

				//largest diagonal element = 11;

				// i = 1 j = 2 k = 0;
				/*
				jk = m.GetElement_20();
				kj = m.GetElement_02();

				ij = m.GetElement_12();
				ji = m.GetElement_21();

				ik = m.GetElement_10();
				ki = m.GetElement_01();
				*/

				s = sqrt((d1 - (d2 + d0)) + 1.0);
      
				m_y = s * 0.5;
			  
				s = 0.5 / s;

				m_z = (m.GetElement_12() + m.GetElement_21()) * s;
				m_x = (m.GetElement_01() + m.GetElement_10()) * s;
				m_w = (m.GetElement_01() - m.GetElement_10()) * s;


				return;

			}
		}
		
		//largest diagonal element = 22;

		// i = 2 j = 0 k = 1;
		/*
		jk = m.GetElement_01();
		kj = m.GetElement_10();

		ij = m.GetElement_20();
		ji = m.GetElement_02();

		ik = m.GetElement_21();
		ki = m.GetElement_12();
		*/

		s = sqrt ((d2 - (d0 + d1)) + 1.0);

		m_y = s * 0.5;
			  
		s = 0.5 / s;

		m_z = (m.GetElement_20() + m.GetElement_02()) * s;
		m_x = (m.GetElement_12() + m.GetElement_21()) * s;
		m_w = (m.GetElement_12() - m.GetElement_21()) * s;


	}
}

/* Construct rotation matrix from (possibly non-unit) quaternion.
 * Assumes matrix is used to multiply column vector on the left:
 * vnew = mat vold.  Works correctly for right-handed coordinate system
 * and right-handed rotations. */
matrix_3x3
tQuat::BuildMatrix(void) {

	matrix_3x3 out;

    real trace2 = (m_x*m_x) + (m_y*m_y) +(m_z*m_z) + (m_w*m_w);

	if (trace2 > 0) {

		real s = 2.0/trace2;

		real xs = m_x*s,	ys = m_y*s,		zs = m_z*s;
		real wx = m_w*xs,	wy = m_w*ys,	wz = m_w*zs;
		real xx = m_x*xs,	xy = m_x*ys,	xz = m_x*zs;
		real yy = m_y*ys,	yz = m_y*zs,	zz = m_z*zs;

		out.SetElement_00(1.0 - (yy + zz));
		out.SetElement_10(xy + wz);
		out.SetElement_20(xz - wy);

		out.SetElement_01(xy - wz);
		out.SetElement_11(1.0 - (xx + zz));
		out.SetElement_21(yz + wx);

		out.SetElement_02(xz + wy);
		out.SetElement_12(yz - wx);
		out.SetElement_22(1.0 - (xx + yy));

	} else {
		// out is already the identity, no need to work the solution for S=0
	}

    return (out);
}

tQuat 
tQuat::Slerp(const tQuat& qL,const tQuat& qR, const real blend) 
{
	tQuat out(DoNotInitialize);

	real cos_omega = qL.m_x*qR.m_x + qL.m_y*qR.m_y + qL.m_z*qR.m_z;

	real alpha, beta;

	if ((1.0 + cos_omega) > RealTolerance) {

		if ((1.0 - cos_omega) < RealTolerance) {
			// Just do a linear interpolation
			alpha = 1.0 - blend;
			beta  = blend;
		} else {
			real omega = acos(cos_omega);
			real sin_omega = sin(omega);
			alpha = sin((1.0 - blend) * omega) / sin_omega;
			beta  = sin(blend * omega) / sin_omega;
		}

		out.m_x = (alpha * qR.m_x) + (beta * qR.m_x);
		out.m_y = (alpha * qR.m_y) + (beta * qR.m_y);
		out.m_z = (alpha * qR.m_z) + (beta * qR.m_z);
		out.m_w = (alpha * qR.m_w) + (beta * qR.m_w);

	} else {

		// THE QUATERNIONS ARE NEARLY OPPOSITE SO TO AVOID A DIVIDE BY ZERO ERROR
		// CALCULATE A PERPENDICULAR QUATERNION AND SLERP THAT DIRECTION

		// Swizzle the right-side quaternion to get a perpenicular and then 
		// blend to that vector

		alpha = sin((1.0 - blend) * RealPi/2);
		beta  = sin(blend * RealPi/2);

		out.m_x = (alpha * qR.m_x) + (beta * -qR.m_y);
		out.m_y = (alpha * qR.m_y) + (beta *  qR.m_x);
		out.m_z = (alpha * qR.m_z) + (beta * -qR.m_w);
		out.m_w = (alpha * qR.m_w) + (beta *  qR.m_z);
	}

	return out;
}
/*

//*******************************************************************8
//*******************************************************************8


  Other people code used to research this Quaternion shit


//*******************************************************************8
//*******************************************************************8


void SlerpQuat(tQuaternion *quat1,tQuaternion *quat2,float slerp, tQuaternion *result)
{
/// Local Variables ///////////////////////////////////////////////////////////
	double omega,cosom,sinom,scale0,scale1;
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
