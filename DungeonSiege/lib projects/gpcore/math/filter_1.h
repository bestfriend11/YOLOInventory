#pragma once
#if !defined(FILTER_1_H)
#define FILTER_1_H
/* ========================================================================
   Header File: filter_1.h
   Description: Declares basic math functions, concepts lifted from Peachey
   ======================================================================== */

//========================================================================
template <typename T>
inline T FilterStep(T const &threshold, T const &x) {

	return (T(x >= threshold));

}

//========================================================================
template <typename T>
inline T FilterPulse(T const &a, T const &b, T const &x) {

	return (FilterStep(a,x) - FilterStep(b,x));

}

//========================================================================
template <typename T>
inline T FilterClamp(T const &a, T const &b, T const &x) {

	return (x < a ? a : (x > b ? b : x));

}


//========================================================================
template <typename T, typename U, typename V>
inline T FilterSmoothStep(U const &a, V const &b, T const &x) {

	if (x < a) return T(0);

	if (x >= b) return T(1);

	T t = (x-a)/(b-a);

	return ((T(3)-T(2)*t)*t*t);

}

//========================================================================
template <typename T>
inline T FilterSmoothPulse(T const &a, T const &b, T const &c, T const &d, T const &x) {

	return (FilterSmoothStep(a,b,x) - FilterSmoothStep(c,d,x));


}


//========================================================================
template <typename T>
inline T FilterModulo(T const &a, T const &b) {

	int n = (int)(a/b);
	return ( (a - n*b < 0) ? a+b: a);

}

//========================================================================
template <typename T>
inline T FilterBSpline(T const &a) {

	T t,tt;

	if (a < 0) {
		t = -a;
	} else {
		t = a;
	}

	if (t < T(1)) {
		tt = t*t;
		return ((T(0.5)*tt*t) - tt + (T(2)/T(3)));
	} else if (t < T(2)) {
		t = T(2)-t;
		return ((T(1)/T(6)) * (t * t * t));
	}
	return (T(0));


}

//========================================================================
template <typename T>
inline T FilterSinc(const T& a)

{
	T x = a*RealPi;
	if(x != 0) return(T(sin(x)) / x);
	return(T(1));
}

template <typename T>
inline T FilterLanczos3(const T& a)

{
	T t;
	if(a < 0) {
		t = -a;
	} else {
		t = a;
	}

	if(t < T(3)) return FilterSinc(t) * FilterSinc(t* (T(1)/T(3)));

	return(T(0));
}

//========================================================================

#define B (T(1)/T(3))
#define C (T(1)/T(3))

template <typename T>
inline T FilterMitchell(T const &a) {

	T t,tt;


	if (a < 0) {
		t = -a;
	} else {
		t = a;
	}

	tt = t*t;

	if (t < T(1)) {

		t =  (((T(12)  - (T(9) * B)  - (T(6) * C)) * (t * tt))
			+ ((T(-18) + (T(12) * B) + (T(6) * C)) * tt)
			+  (T(6)   - (T(2) * B)));

		return (t/T(6));

	} else if (t < T(2)) {

		t =  ((((T(-1) * B) - (T(6) * C)) * (t * tt))
			+ (((T(6) * B) + (T(30) * C)) * tt)
			+ (((T(-12) * B) - (T(48) * C)) * t)
			+  ((T(8) * B) + (T(24) * C)));

		return (t/T(6));
	}

	return (T(0));


}

#undef B
#undef C

#endif
