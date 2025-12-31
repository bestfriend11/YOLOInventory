#pragma once
#if !defined(FILTER_1_H)
#define FILTER_1_H
/* ========================================================================
   Header File: filter_1.h
   Description: Declares basic math functions, concepts lifted from Peachey
   ======================================================================== */

#include "gpmath.h"


//========================================================================
inline real FilterStep(real const &threshold, real const &x) {

	return (real(x >= threshold));

}

//========================================================================
inline real FilterPulse(real const &a, real const &b, real const &x) {

	return (FilterStep(a,x) - FilterStep(b,x));

}

//========================================================================
inline real FilterClamp(real const &a, real const &b, real const &x) {

	return (x < a ? a : (x > b ? b : x));

}


//========================================================================
inline real FilterSmoothStep(real const &a, real const &b, real const &x) {

	if (x < a) return real(0);

	if (x >= b) return real(1);

	real t = (x-a)/(b-a);

	return ((real(3)-real(2)*t)*t*t);

}

//========================================================================
inline real FilterSmoothPulse(real const &a, real const &b, real const &c, real const &d, real const &x) {

	return (FilterSmoothStep(a,b,x) - FilterSmoothStep(c,d,x));


}


//========================================================================
inline real FilterModulo(real const &a, real const &b) {

	int n = (int)(a/b);
	return ( (a - n*b < 0) ? a+b: a);

}

//========================================================================
inline real FilterBSpline(real const &a) {

	real t,tt;

	if (a < 0) {
		t = -a;
	} else {
		t = a;
	}

	if (t < real(1)) {
		tt = t*t;
		return ((real(0.5)*tt*t) - tt + (real(2)/real(3)));
	} else if (t < real(2)) {
		t = real(2)-t;
		return ((real(1)/real(6)) * (t * t * t));
	}
	return (real(0));


}

//========================================================================
inline real FilterSinc(const real& a)

{
	real x = a*RealPi;
	if(x != 0) return(sin(x) / x);
	return(real(1));
}

inline real FilterLanczos3(const real& a)

{
	real t;
	if(a < 0) {
		t = -a;
	} else {
		t = a;
	}

	if(t < real(3)) return FilterSinc(t) * FilterSinc(t* (real(1)/real(3)));

	return(0.0);
}

//========================================================================

#define B (real(1)/real(3))
#define C (real(1)/real(3))

inline real FilterMitchell(real const &a) {

	real t,tt;


	if (a < 0) {
		t = -a;
	} else {
		t = a;
	}

	tt = t*t;

	if (t < real(1)) {

		t =  (((real(12)  - (real(9) * B)  - (real(6) * C)) * (t * tt))
			+ ((real(-18) + (real(12) * B) + (real(6) * C)) * tt)
			+  (real(6)   - (real(2) * B)));

		return (t/real(6));

	} else if (t < real(2)) {

		t =  ((((real(-1) * B) - (real(6) * C)) * (t * tt))
			+ (((real(6) * B) + (real(30) * C)) * tt)
			+ (((real(-12) * B) - (real(48) * C)) * t)
			+  ((real(8) * B) + (real(24) * C)));

		return (t/real(6));
	}

	return (real(0));


}

#endif
