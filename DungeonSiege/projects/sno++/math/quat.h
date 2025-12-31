#pragma once
#ifndef QUAT_H
#define QUAT_H

//===========================================================================
//
//	Header File: tQuat.h
//
//  Definition and manipulation of quaternions
//
// These functions were derived from Shoemake's ArcBall and Polar Decomposition 
// support routines as published in GGems IV
//
//===========================================================================

#include "vector_3.h"
#include "matrix_3x3.h"

//*******************************************************

class tQuat {

public:


	// Constructors

	inline tQuat(
		const real x,
		const real y,
		const real z,
		const real w)		{ m_x=x;m_y=y;m_z=z;m_w=w; }

	inline tQuat(void)		{ m_x=0;m_y=0;m_z=0;m_w=1; }

	// Uninitialized construction a la Casey M.
	enum do_not_initialize {DoNotInitialize};
	inline explicit tQuat(do_not_initialize const MustBeDoNotInitialize) {
					gpassertm(MustBeDoNotInitialize == DoNotInitialize,"Improper quat construction");
					}

	// Inline?
	tQuat(const matrix_3x3 &m);

	matrix_3x3 BuildMatrix(void) const;		// return 3x3 matrix

	friend tQuat Slerp(const tQuat& qL,const tQuat& qR, const real alpha);

	inline vector_3 RotateVector(const vector_3& v) const;
	inline void RotateDoubleVector(const vector_3& v,double &x, double& y, double& z) const;

	// Accessors
	inline real X() const				{ return m_x; }
	inline real Y() const				{ return m_y; }
	inline real Z() const				{ return m_z; }
	inline real W() const				{ return m_w; }

	inline void X( const real Value )		{ m_x = Value; }
	inline void Y( const real Value )		{ m_y = Value; }
	inline void Z( const real Value )		{ m_z = Value; }
	inline void W( const real Value )		{ m_w = Value; }

	// Constants
	inline const tQuat zero(void)			{return tQuat(0,0,0,0);}		
	inline const tQuat ident(void)			{return tQuat(0,0,0,1);}		

	// Rotations
	inline void Rotate(const vector_3& vec, real ang);
	inline void RotateX(const real ang);
	inline void RotateY(const real ang);
	inline void RotateZ(const real ang);


	// Operations

	inline tQuat operator*=(const tQuat &qR);
	inline tQuat operator*(const tQuat &qR) const;

	inline tQuat operator+=(tQuat const &qR);
	inline tQuat operator+(tQuat const &qR) const;

	inline tQuat operator-=(tQuat const &qR);
	inline tQuat operator-(tQuat const &qR) const;
	inline tQuat operator-(void) const;

	inline bool operator==(const tQuat &qR) const;
	inline bool operator!=(const tQuat &qR) const;

/*
	inline quat operator-(quat const &Operand1);

	inline const quat operator*(quat const Multiplier);
	inline quat operator*(real const Multiplier,    quat const &Multiplicand);	  
	inline quat operator*(quat const &Multiplicand, real const Multiplier);

	inline quat operator/(quat const &Quotient,		real const Divisor);

	inline bool operator==(quat const &Operand1,	  quat const &Operand2);
	inline bool operator!=(quat const &Operand1,	  quat const &Operand2);

*/

private:

	real m_x;
	real m_y;
	real m_z;
	real m_w;

};

/*
inline 
tQuat::tQuat(const vector_3& axis, const real ang)	{
	
	real len = Length(axis);

	if (IsZero(len)) { 

		// Set it to the identity
		m_x = 0;
		m_y = 0;
		m_z = 0;
		m_w = 1;

	} else {

		real half_ang(ang*0.5);

		m_w = cos(half_ang);
		m_x = axis.GetX() * (sin(half_ang) / len);
		m_y = axis.GetY() * (sin(half_ang) / len);
		m_z = axis.GetZ() * (sin(half_ang) / len);

	}

}
*/

inline void 
tQuat::Rotate(const vector_3& axis, const real ang)
{

	real len = Length(axis);

	if (!IsZero(len)) { 

		const real half_ang(ang*0.5);

		const real s = sin(half_ang) / len;

		const real rw = cos(half_ang);
		const real rx = axis.GetX() * s;
		const real ry = axis.GetY() * s;
		const real rz = axis.GetZ() * s;

		const real tmp_w(m_w);
		const real tmp_x(m_x);
		const real tmp_y(m_y);

		m_w = (rw * m_w) - (rx * m_x + ry * m_y + rz * m_z );

		m_x = (rw * m_x) + (rx * tmp_w) + ((ry *   m_z) - (rz * tmp_y));

		m_y = (rw * m_y) + (ry * tmp_w) + ((rz * tmp_x) - (rx *   m_z));

		m_z = (rw * m_z) + (rz * tmp_w) + ((rx * tmp_y) - (ry * tmp_x));


/*
		Operand1.GetY()*Operand2.GetZ() - Operand1.GetZ()*Operand2.GetY(),
		Operand1.GetZ()*Operand2.GetX() - Operand1.GetX()*Operand2.GetZ(),
		Operand1.GetX()*Operand2.GetY() - Operand1.GetY()*Operand2.GetX()));

*/
	}

}

inline void 
tQuat::RotateX(const real ang)
{
	const real half_ang(ang*0.5);

	const real s(sin(half_ang));
	const real c(cos(half_ang));

	real tmp = m_w;

	m_w = ( c * tmp ) - (s * m_x );
	m_x = ( c * m_x ) + (s * tmp );

	tmp = m_y;

	m_y = ( c * tmp ) - ( s * m_z );
	m_z = ( c * m_z ) + ( s * tmp );

/*
	W( c * tmp - s * X() );
	X( c * X() + s * tmp );

	tmp = Y();

	Y( c * tmp - s * Z() );
	Z( c * Z() + s * tmp );
*/
}

inline void 
tQuat::RotateY(const real ang)
{
	const real half_ang(ang*0.5);

	const real s(sin(half_ang));
	const real c(cos(half_ang));

	real tmp = m_w;

	m_w = ( c * tmp ) - ( s * m_y );
	m_y = ( c * m_y ) + ( s * tmp );

	tmp = m_z;

	m_z = ( c * tmp ) - (s * m_x );
	m_x = ( c * m_x ) + (s * tmp );
/*
	real tmp(W());

	W( c * tmp - s * Y() );
	Y( c * Y() + s * tmp );

	tmp = Z();

	Z( c * tmp - s * X() );
	X( c * X() + s * tmp );
*/
}



inline void 
tQuat::RotateZ(const real ang)
{
	const real half_ang(ang*0.5);

	const real s(sin(half_ang));
	const real c(cos(half_ang));

	real tmp = m_w;

	m_w = ( c * tmp ) - ( s * m_z );
	m_z = ( c * m_z ) + ( s * tmp );

	tmp = m_x;

	m_x = ( c * tmp ) - (s * m_y );
	m_y = ( c * m_y ) + (s * tmp );

/*
	real tmp(W());

	W( c * tmp - s * Z() );
	Z( c * Z() + s * tmp );

	tmp = X();

	X( c * tmp - s * Y() );
	Y( c * Y() + s * tmp );
*/
}


inline tQuat 
tQuat::operator*=(tQuat const &qR) {

	const tQuat qL(*this);

    m_w = qL.m_w * qR.m_w - qL.m_x * qR.m_x - qL.m_y * qR.m_y - qL.m_z * qR.m_z;
    m_x = qL.m_w * qR.m_x + qL.m_x * qR.m_w + qL.m_y * qR.m_z - qL.m_z * qR.m_y;
    m_y = qL.m_w * qR.m_y + qL.m_y * qR.m_w + qL.m_z * qR.m_x - qL.m_x * qR.m_z;
    m_z = qL.m_w * qR.m_z + qL.m_z * qR.m_w + qL.m_x * qR.m_y - qL.m_y * qR.m_x;

    return (*this);

}

inline tQuat 
tQuat::operator*(tQuat const &qR) const {

	tQuat tmp(DoNotInitialize);

    tmp.m_w = m_w * qR.m_w - m_x * qR.m_x - m_y * qR.m_y - m_z * qR.m_z;
    tmp.m_x = m_w * qR.m_x + m_x * qR.m_w + m_y * qR.m_z - m_z * qR.m_y;
    tmp.m_y = m_w * qR.m_y + m_y * qR.m_w + m_z * qR.m_x - m_x * qR.m_z;
    tmp.m_z = m_w * qR.m_z + m_z * qR.m_w + m_x * qR.m_y - m_y * qR.m_x;

    return tmp;

}

inline tQuat 
tQuat::operator+=(tQuat const &qR) {

//	tQuat qL(*this);

    m_w += qR.m_w;
    m_x += qR.m_x;
    m_y += qR.m_y;
    m_z += qR.m_z;

    return (*this);

}

inline tQuat 
tQuat::operator+(tQuat const &qR) const {

	tQuat tmp(DoNotInitialize);

    tmp.m_w = m_w + qR.m_w;
    tmp.m_x = m_x + qR.m_x;
    tmp.m_y = m_y + qR.m_y;
    tmp.m_z = m_z + qR.m_z;

    return tmp;

}


inline tQuat 
tQuat::operator-=(tQuat const &qR) {

//	tQuat qL(*this);

    m_w -= qR.m_w;
    m_x -= qR.m_x;
    m_y -= qR.m_y;
    m_z -= qR.m_z;

    return (*this);

}

inline tQuat 
tQuat::operator-(tQuat const &qR) const {

	tQuat tmp(DoNotInitialize);

    tmp.m_w = m_w - qR.m_w;
    tmp.m_x = m_x - qR.m_x;
    tmp.m_y = m_y - qR.m_y;
    tmp.m_z = m_z - qR.m_z;

    return tmp;

}

inline tQuat 
tQuat::operator-(void) const {

	tQuat tmp(DoNotInitialize);

    tmp.m_w = -m_w;
    tmp.m_x = -m_x;
    tmp.m_y = -m_y;
    tmp.m_z = -m_z;

    return tmp;

}


inline bool 
tQuat::operator==(tQuat const &b) const {
	return ( IsZero(m_w-b.m_w) &&  IsZero(m_x-b.m_x) &&  IsZero(m_y-b.m_y) &&  IsZero(m_z-b.m_z));
}

inline bool 
tQuat::operator!=(tQuat const &b) const {
	return (!IsZero(m_w-b.m_w) || !IsZero(m_x-b.m_x) || !IsZero(m_y-b.m_y) || !IsZero(m_z-b.m_z));
}


inline vector_3
tQuat::RotateVector(const vector_3& v) const {

	const real r = 2.0f * m_w;
	const real s = (r * m_w) - 1.0f;
	const real t = 2.0f * ((m_x*v.GetX()) + (m_y*v.GetY()) + (m_z*v.GetZ()));

	return vector_3(
		(s * v.GetX()) + (t * m_x) + (r * (m_y*v.GetZ() - m_z*v.GetY())),
		(s * v.GetY()) + (t * m_y) + (r * (m_z*v.GetX() - m_x*v.GetZ())),
		(s * v.GetZ()) + (t * m_z) + (r * (m_x*v.GetY() - m_y*v.GetX()))
		);

}

#ifdef APPLY_FIXUP
void
tQuat::RotateDoubleVector(const vector_3& v,double &x, double& y, double& z) const {

	const double r = 2.0f * m_w;
	const double s = (r * m_w) - 1.0f;
	const double t = 2.0f * ((m_x*v.GetX()) + (m_y*v.GetY()) + (m_z*v.GetZ()));

	
	x =	(s * v.GetX()) + (t * m_x) + (r * (m_y*v.GetZ() - m_z*v.GetY()));
	y = (s * v.GetY()) + (t * m_y) + (r * (m_z*v.GetX() - m_x*v.GetZ()));
	z =	(s * v.GetZ()) + (t * m_z) + (r * (m_x*v.GetY() - m_y*v.GetX()));

}
#endif

#endif

