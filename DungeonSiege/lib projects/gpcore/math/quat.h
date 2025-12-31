#pragma once
#ifndef QUAT_H
#define QUAT_H

//===========================================================================
//
//	Header File: Quat.h
//
//  Definition and manipulation of quaternions
//
// These functions were derived from Shoemake's ArcBall and Polar Decomposition 
// support routines as published in GGems IV as well as Tim Sweeney's examples
//
//===========================================================================

#include "vector_3.h"
#include "matrix_3x3.h"
#include "fubidefs.h"

// 3dsmax has already defined a "Quat" --biddle
#if GP_SIEGEMAX
#define Quat xQuat 
#endif

#ifdef PARANOID_MATH
#define VALIDATE_QUATERNION(q,msg)					\
	gpassertf(										\
		(!_isnan((q).m_x) &&						\
		 !_isnan((q).m_y) &&						\
		 !_isnan((q).m_z) &&						\
		 !_isnan((q).m_w) &&						\
		 (q) != Quat::ZERO)							\
		,("Illegal quat [%f,%f,%f,%f] : "	msg		\
		,(q).m_x,(q).m_y,(q).m_z,(q).m_w))
#else
#define VALIDATE_QUATERNION(q,msg)
#endif

//*******************************************************

struct Quat
{
	FUBI_POD_CLASS( Quat );

	static const Quat ZERO;			// 0, 0, 0, 0
	static const Quat IDENTITY;		// 0, 0, 0, 1
	static const Quat INVALID;		// Inf, Inf, Inf, Inf
	static const Quat INDEFINITE;	// Ind, Ind, Ind, Ind

	// Constructors

	inline Quat(
		const real x,
		const real y,
		const real z,
		const real w)		{ m_x=x;m_y=y;m_z=z;m_w=w; }

	inline Quat(void)		{ m_x=0;m_y=0;m_z=0;m_w=1; }

	// Uninitialized construction a la Casey M.
	inline explicit Quat(do_not_initialize)  {  }

	// Inline?
	Quat(const matrix_3x3 &m);

	void Set( float x, float y, float z, float w )	{  m_x = x;  m_y = y;  m_z = z;  m_w = w;  }
	void Set( const float* elements )				{  ::memcpy( this, elements, sizeof( *this ) );  }

	void BuildMatrix(matrix_3x3& out) const;	// store 3x3 matrix in 'out'
	matrix_3x3 BuildMatrix(void) const	{ matrix_3x3 m( DoNotInitialize ); BuildMatrix( m ); return ( m ); }

	friend void Slerp(Quat& qO, const Quat& qL,const Quat& qR, const real alpha);
	friend void SlerpInPlace(Quat& qL,const Quat& qR, const real alpha);
	friend Quat Slerp_T(const Quat& qL,const Quat& qR, const real alpha)		{  Quat qO( DoNotInitialize );  Slerp( qO, qL, qR, alpha );  return ( qO );  }
	friend bool SlerpTowards(Quat& result, const Quat& qL,const Quat& qR, const real theta);

	inline void RotateVector(vector_3& out, const vector_3& v) const;	// Apply quaternion rotation to vector V 
	inline void RotateXAxis(vector_3& out) const;						// Apply quaternion rotation to vector <1,0,0>
	inline void RotateZAxis(vector_3& out) const;						// Apply quaternion rotation to vector <0,0,1>

	inline vector_3 RotateVector_T(const vector_3& v) const		{  vector_3 out( DoNotInitialize );  RotateVector( out, v );  return ( out );  }
	inline vector_3 RotateXAxis_T(void) const					{  vector_3 out( DoNotInitialize );  RotateXAxis( out );  return ( out );  }
	inline vector_3 RotateZAxis_T(void) const					{  vector_3 out( DoNotInitialize );  RotateZAxis( out );  return ( out );  }

	inline void RotateVectorInPlace(vector_3&v) const;

	inline float Norm(void) const;
	inline float Magnitude(void) const;
	inline Quat Inverse(void) const;
	inline Quat Normalize(void) const;

	// Logarithms
	inline void Exp(Quat& lhs);
	inline Quat Exp_T(void);
	inline void Log(Quat& lhs);
	inline Quat Log_T(void);

	// Accessors
	inline real X() const				{ return m_x; }
	inline real Y() const				{ return m_y; }
	inline real Z() const				{ return m_z; }
	inline real W() const				{ return m_w; }

	inline void X( const real Value )		{ m_x = Value; }
	inline void Y( const real Value )		{ m_y = Value; }
	inline void Z( const real Value )		{ m_z = Value; }
	inline void W( const real Value )		{ m_w = Value; }

	// Rotations

	// $$$ doc: what direction do these rotations take place as ang grows?  Clockwise, counterclock etc? -bk
	inline void Rotate(const vector_3& vec, real ang);
	inline void RotateX(const real ang);
	inline void RotateY(const real ang);
	inline void RotateZ(const real ang);


	// Operations

	inline Quat& operator=(const Quat &qR);

	inline Quat& operator*=(const Quat &qR);
	inline Quat operator*(const Quat &qR) const;

	inline Quat& operator*=(float scalar);
	inline Quat operator*(float scalar) const;

	inline Quat& operator+=(Quat const &qR);
	inline Quat operator+(Quat const &qR) const;

	inline Quat& operator-=(Quat const &qR);
	inline Quat operator-(Quat const &qR) const;
	inline Quat operator-(void) const;

	inline bool operator==(const Quat &qR) const;
	inline bool operator!=(const Quat &qR) const;

	inline bool IsEqual( const Quat& qR, float epsilon ) const;

/*
	inline quat operator-(quat const &Operand1);

	inline const quat operator*(quat const Multiplier);
	inline quat operator*(real const Multiplier,    quat const &Multiplicand);	  
	inline quat operator*(quat const &Multiplicand, real const Multiplier);

	inline quat operator/(quat const &Quotient,		real const Divisor);

	inline bool operator==(quat const &Operand1,	  quat const &Operand2);
	inline bool operator!=(quat const &Operand1,	  quat const &Operand2);

*/

	FUBI_EXPORT bool GetIsValid( void )	{ return ((*this!=INVALID) && (*this!=INDEFINITE)); };

	FUBI_EXPORT float FUBI_RENAME( GetW )( void ) const  {  return ( m_w );  }
	FUBI_EXPORT float FUBI_RENAME( GetX )( void ) const  {  return ( m_x );  }
	FUBI_EXPORT float FUBI_RENAME( GetY )( void ) const  {  return ( m_y );  }
	FUBI_EXPORT float FUBI_RENAME( GetZ )( void ) const  {  return ( m_z );  }

	FUBI_EXPORT void FUBI_RENAME( SetW )( float f )  {  m_w = f;  }
	FUBI_EXPORT void FUBI_RENAME( SetX )( float f )  {  m_x = f;  }
	FUBI_EXPORT void FUBI_RENAME( SetY )( float f )  {  m_y = f;  }
	FUBI_EXPORT void FUBI_RENAME( SetZ )( float f )  {  m_z = f;  }

	real m_x;
	real m_y;
	real m_z;
	real m_w;
};

/*
inline 
Quat::Quat(const vector_3& axis, const real ang)	{
	
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
Quat::Rotate(const vector_3& axis, const real ang)
{

	VALIDATE_QUATERNION(*this,"BEFORE rotation(axis,angle)");

	real len = Length(axis);

	if (!IsZero(len)) {
		
		float s, rw;
		SINCOSF( (ang*0.5f), s, rw );

		s	/= len;
		const real rx = axis.x * s;
		const real ry = axis.y * s;
		const real rz = axis.z * s;

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

	VALIDATE_QUATERNION(*this,"AFTER rotation(axis,angle)");
}

inline void 
Quat::RotateX(const real ang)
{

	VALIDATE_QUATERNION(*this,"BEFORE rotationX");

	float s, c;
	SINCOSF( (ang*0.5f), s, c );

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
	VALIDATE_QUATERNION(*this,"AFTER rotationX");

}

inline void 
Quat::RotateY(const real ang)
{

	VALIDATE_QUATERNION(*this,"BEFORE rotationY");

	float s, c;
	SINCOSF( (ang*0.5f), s, c );

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
	VALIDATE_QUATERNION(*this,"AFTER rotationY");
}



inline void 
Quat::RotateZ(const real ang)
{
	VALIDATE_QUATERNION(*this,"BEFORE rotationZ");

	float s, c;
	SINCOSF( (ang*0.5f), s, c );

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
	VALIDATE_QUATERNION(*this,"AFTER rotationZ");

}


inline Quat& 
Quat::operator=(Quat const &qR) {

//	VALIDATE_QUATERNION(*this,"BEFORE assignment (bad src)");
//	VALIDATE_QUATERNION(qR,"BEFORE assignment (bad arg)");

    m_x = qR.m_x;
    m_y = qR.m_y;
    m_z = qR.m_z;
    m_w = qR.m_w;

//	VALIDATE_QUATERNION(*this,"AFTER assignment");

    return (*this);

}

inline Quat& 
Quat::operator*=(Quat const &qR) {

	VALIDATE_QUATERNION(*this,"BEFORE mult with assignment");
	VALIDATE_QUATERNION(qR,"BEFORE mult (bad arg)");

	const Quat qL(*this);

    m_w = qL.m_w * qR.m_w - qL.m_x * qR.m_x - qL.m_y * qR.m_y - qL.m_z * qR.m_z;
    m_x = qL.m_w * qR.m_x + qL.m_x * qR.m_w + qL.m_y * qR.m_z - qL.m_z * qR.m_y;
    m_y = qL.m_w * qR.m_y + qL.m_y * qR.m_w + qL.m_z * qR.m_x - qL.m_x * qR.m_z;
    m_z = qL.m_w * qR.m_z + qL.m_z * qR.m_w + qL.m_x * qR.m_y - qL.m_y * qR.m_x;

	VALIDATE_QUATERNION(*this,"AFTER mult with assignment");

    return (*this);

}

inline Quat 
Quat::operator*(Quat const &qR) const {

	VALIDATE_QUATERNION(*this,"BEFORE mult (bad src)");
	VALIDATE_QUATERNION(qR,"BEFORE mult (bad arg)");

	Quat tmp(DoNotInitialize);

    tmp.m_w = m_w * qR.m_w - m_x * qR.m_x - m_y * qR.m_y - m_z * qR.m_z;
    tmp.m_x = m_w * qR.m_x + m_x * qR.m_w + m_y * qR.m_z - m_z * qR.m_y;
    tmp.m_y = m_w * qR.m_y + m_y * qR.m_w + m_z * qR.m_x - m_x * qR.m_z;
    tmp.m_z = m_w * qR.m_z + m_z * qR.m_w + m_x * qR.m_y - m_y * qR.m_x;

	VALIDATE_QUATERNION(tmp,"AFTER mult");

    return tmp;

}

inline Quat& 
Quat::operator*=(float s) {

	VALIDATE_QUATERNION(*this,"BEFORE mult with assignment");

    m_w = m_w * s;
    m_x = m_x * s;
    m_y = m_y * s;
    m_z = m_z * s;

	VALIDATE_QUATERNION(*this,"AFTER mult with assignment");

    return (*this);

}

inline Quat 
Quat::operator*(float s) const {

	VALIDATE_QUATERNION(*this,"BEFORE mult (bad src)");

	Quat tmp(DoNotInitialize);

    tmp.m_w = m_w * s;
    tmp.m_x = m_x * s;
    tmp.m_y = m_y * s;
    tmp.m_z = m_z * s;

	VALIDATE_QUATERNION(tmp,"AFTER mult");

    return tmp;

}

inline Quat& 
Quat::operator+=(Quat const &qR) {

	VALIDATE_QUATERNION(*this,"BEFORE add with assignment(bad src)");
	VALIDATE_QUATERNION(qR,"BEFORE add with assignment (bad arg)");

//	Quat qL(*this);

    m_w += qR.m_w;
    m_x += qR.m_x;
    m_y += qR.m_y;
    m_z += qR.m_z;

	VALIDATE_QUATERNION(*this,"AFTER add with assignment");

    return (*this);

}

inline Quat 
Quat::operator+(Quat const &qR) const {

	VALIDATE_QUATERNION(*this,"BEFORE add (bad src)");
	VALIDATE_QUATERNION(qR,"BEFORE add (bad arg)");

	Quat tmp(DoNotInitialize);

    tmp.m_w = m_w + qR.m_w;
    tmp.m_x = m_x + qR.m_x;
    tmp.m_y = m_y + qR.m_y;
    tmp.m_z = m_z + qR.m_z;

	VALIDATE_QUATERNION(tmp,"AFTER add");

    return tmp;

}


inline Quat& 
Quat::operator-=(Quat const &qR) {

	VALIDATE_QUATERNION(*this,"BEFORE sub with assignment(bad src)");
	VALIDATE_QUATERNION(qR,"BEFORE sub with assignment (bad arg)");

    m_w -= qR.m_w;
    m_x -= qR.m_x;
    m_y -= qR.m_y;
    m_z -= qR.m_z;

	VALIDATE_QUATERNION(*this,"AFTER sub with assignment");

    return (*this);

}

inline Quat 
Quat::operator-(Quat const &qR) const {

	VALIDATE_QUATERNION(*this,"BEFORE sub (bad src)");
	VALIDATE_QUATERNION(qR,"BEFORE sub (bad arg)");

	Quat tmp(DoNotInitialize);

    tmp.m_w = m_w - qR.m_w;
    tmp.m_x = m_x - qR.m_x;
    tmp.m_y = m_y - qR.m_y;
    tmp.m_z = m_z - qR.m_z;

	VALIDATE_QUATERNION(tmp,"AFTER sub");

    return tmp;

}

inline Quat 
Quat::operator-(void) const {

	VALIDATE_QUATERNION(*this,"BEFORE negate (bad src)");

	Quat tmp(DoNotInitialize);

    tmp.m_w = -m_w;
    tmp.m_x = -m_x;
    tmp.m_y = -m_y;
    tmp.m_z = -m_z;

	VALIDATE_QUATERNION(tmp,"AFTER negate");

    return tmp;

}


inline bool 
Quat::operator==(Quat const &b) const {
	if  ( ::IsEqual(m_w,b.m_w) && ::IsEqual(m_x,b.m_x) && ::IsEqual(m_y,b.m_y) && ::IsEqual(m_z,b.m_z)) {
		return true;
	} 
	return false;
	// What about the conj(q) when W is zero?
	// ie:  IsZero(m_w) &&  IsZero(m_x+b.m_x) &&  IsZero(m_y+b.m_y) && IsZero(m_z+b.m_z) ?
}

inline bool 
Quat::operator!=(Quat const &b) const {
	return (!::IsEqual(m_w,b.m_w) || !::IsEqual(m_x,b.m_x) || !::IsEqual(m_y,b.m_y) || !::IsEqual(m_z,b.m_z));
}

inline bool 
Quat::IsEqual(Quat const &b, float epsilon) const
{
	return (   ::IsEqual( m_w, b.m_w, epsilon )
			&& ::IsEqual( m_x, b.m_x, epsilon )
			&& ::IsEqual( m_y, b.m_y, epsilon )
			&& ::IsEqual( m_z, b.m_z, epsilon ) );
}

inline void
Quat::RotateVector(vector_3& out, const vector_3& v) const
{
	VALIDATE_QUATERNION(*this,"BEFORE rotate vector");
	VALIDATE_VECTOR_3(v);
	gpassert( &out != &v );

	const float r = 2.0f * m_w;
	const float s = (r * m_w) - 1.0f;
	const float t = 2.0f * ((m_x*v.x) + (m_y*v.y) + (m_z*v.z));

	out.x = (s * v.x) + (t * m_x) + (r * (m_y*v.z - m_z*v.y));
	out.y = (s * v.y) + (t * m_y) + (r * (m_z*v.x - m_x*v.z));
	out.z = (s * v.z) + (t * m_z) + (r * (m_x*v.y - m_y*v.x));
}

inline void
Quat::RotateVectorInPlace(vector_3& v) const
{
	VALIDATE_QUATERNION(*this,"BEFORE rotate vector");
	VALIDATE_VECTOR_3(v);

	const float r = 2.0f * m_w;
	const float s = (r * m_w) - 1.0f;
	const float t = 2.0f * ((m_x*v.x) + (m_y*v.y) + (m_z*v.z));

	const float tmp0 = r * (m_z*v.x - m_x*v.z);
	const float tmp1 = r * (m_x*v.y - m_y*v.x);

	v.x = (s * v.x) + (t * m_x) + (r * (m_y*v.z - m_z*v.y));
	v.y = (s * v.y) + (t * m_y) + tmp0;
	v.z = (s * v.z) + (t * m_z) + tmp1;
}

// RotateXAxis is a fast(er) way to determine where the X-axis <1,0,0> will
// be located after a quaternion RotateVector. 
inline void
Quat::RotateXAxis(vector_3& out) const
{
	const real r = 2.0f * m_w;
	const real s = (r * m_w) - 1.0f;
	const real t = 2.0f * m_x;

	out.x = s + (t * m_x);
	out.y =     (t * m_y) + (r *  m_z);
	out.z =     (t * m_z) + (r * -m_y);
}

// RotateZAxis is a fast(er) way to determine where the Z-axis <0,0,1> will
// be located after a quaternion RotateVector. 
inline void
Quat::RotateZAxis(vector_3& out) const
{
	VALIDATE_QUATERNION(*this,"BEFORE RotateZAxis");

	const real r = 2.0f * m_w;
	const real s = (r * m_w) - 1.0f;
	const real t = 2.0f * m_z;

	out.x =       (t * m_x) + (r * (  m_y ));
	out.y =       (t * m_y) + (r * (- m_x ));
	out.z = (s) + (t * m_z);
}

inline Quat
Quat::Inverse(void) const {

	VALIDATE_QUATERNION(*this,"BEFORE invert");

	float squared_normq = ((m_x*m_x)+(m_y*m_y)+(m_z*m_z)+(m_w*m_w));

	gpassert(!IsZero(squared_normq));

	Quat tmp(DoNotInitialize);

	// TODO move to unit quaternions to avoid the 1/norm(q)
	if ( !IsZero(squared_normq) && !::IsEqual(squared_normq,1) )
	{
		float n = INVERSEF( squared_normq );

		tmp.m_w =  m_w * n;
		tmp.m_x = -m_x * n;
		tmp.m_y = -m_y * n;
		tmp.m_z = -m_z * n;

		VALIDATE_QUATERNION(tmp,"AFTER invert");

	}
	else
	{
		tmp.m_w =  m_w;
		tmp.m_x = -m_x;
		tmp.m_y = -m_y;
		tmp.m_z = -m_z;
	}

	return tmp;	// conj(q)/|q|^2

}
/*
*/

inline float
Quat::Norm(void) const {
	return ((m_x*m_x)+(m_y*m_y)+(m_z*m_z)+(m_w*m_w));
}

inline float
Quat::Magnitude(void) const {
	return SQRTF(Norm());
}

inline Quat
Quat::Normalize(void) const {

	VALIDATE_QUATERNION(*this,"BEFORE normalize");

	// TODO move to unit quaternions to avoid the 1/norm(q)
	float n = 1.0f/Magnitude();

	return Quat(n*m_x,n*m_y,n*m_z,n*m_w);	// q/|q|
}

inline void
Quat::Exp(Quat& lhs) {
	float a  = SQRTF(m_x*m_x+m_y*m_y+m_z*m_z);
	float s,c;
	SINCOSF( a, s, c );
	if (a>=0.00001f)
	{
		s /= a;
		lhs.m_x = s*m_x;
		lhs.m_y = s*m_y;
		lhs.m_z = s*m_z;
		lhs.m_w = c;
	}
	else
	{
		lhs.m_x = m_x;
		lhs.m_y = m_y;
		lhs.m_z = m_z;
		lhs.m_w = c;
	}
}

inline void
Quat::Log(Quat& lhs) {
	if (fabsf(m_w)<1.0f)
	{
		float a  = ACOSF(m_w);
		float s  = SINF(a);
		if (fabsf(s) >= 0.00001f) 
		{
			float t = a/s;
			lhs.m_x = t*m_x;
			lhs.m_y = t*m_y;
			lhs.m_z = t*m_z;
			lhs.m_w = 0;
			return;
		}
	}
	lhs.m_x = m_x;
	lhs.m_y = m_y;
	lhs.m_z = m_z;
	lhs.m_w = 0;
}

inline Quat
Quat::Exp_T(void) {
	Quat tmp(DoNotInitialize);
	Exp(tmp);
	return tmp;
}

inline Quat
Quat::Log_T(void) {
	Quat tmp(DoNotInitialize);
	Log(tmp);
	return tmp;
}

inline void
SlerpInPlace(Quat& qL,const Quat& qR, const real blend)
{
	VALIDATE_QUATERNION(qL,"Can't slerp (bad left side)");
	VALIDATE_QUATERNION(qR,"Can't slerp (bad right side)");

	real cos_omega = (qL.m_x*qR.m_x) + (qL.m_y*qR.m_y) + (qL.m_z*qR.m_z) + (qL.m_w*qR.m_w);

	real alpha, beta;

	if(cos_omega < 0.0f )
	{
		// Need to use the negative of qR, since its const we invert where ever it's used
		// qR = -qR;

		cos_omega = -cos_omega;
		
		if ((1.0 - cos_omega) < 0.001 /*RealTolerance*/)
		{
			// Just do a linear interpolation
			alpha = 1.0f - blend;
			beta  = -blend;
		}
		else
		{
			real omega = ACOSF(cos_omega);
			real inv_sin_omega = INVERSEF( SINF(omega) );

			alpha =  SINF((1.0f - blend) * omega) * inv_sin_omega;
			beta  = -SINF(blend * omega) * inv_sin_omega;
		}


		qL.m_x = (alpha * qL.m_x) + (beta * qR.m_x);
		qL.m_y = (alpha * qL.m_y) + (beta * qR.m_y);
		qL.m_z = (alpha * qL.m_z) + (beta * qR.m_z);
		qL.m_w = (alpha * qL.m_w) + (beta * qR.m_w);

	} 
	else
	{
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


		qL.m_x = (alpha * qL.m_x) + (beta * qR.m_x);
		qL.m_y = (alpha * qL.m_y) + (beta * qR.m_y);
		qL.m_z = (alpha * qL.m_z) + (beta * qR.m_z);
		qL.m_w = (alpha * qL.m_w) + (beta * qR.m_w);

	}


	VALIDATE_QUATERNION(qL,"Can't slerp (output)");
}

#endif

