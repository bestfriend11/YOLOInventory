#pragma once

// $$$ ENDANGERED
#if 0
#if !defined(INTERVAL_H)
/* ========================================================================
   Header File: interval.h
   Description: Declares interval

   TODO: Many of the functions in here can be converted to use the scalar
         math functions once those functions are also templatized
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "gpassert.h"

/* ========================================================================
   Forward Declarations
   ======================================================================== */
template <class value_init> class interval; // from interval.h

// from interval.h
template <class value_init>
inline interval<value_init> operator+(interval<value_init> const &Operand1,
									  interval<value_init> const &Operand2);
template <class value_init>
inline interval<value_init> operator-(interval<value_init> const &Operand1);
template <class value_init>
inline interval<value_init> operator-(interval<value_init> const &Operand1,
									  interval<value_init> const &Operand2);
template <class value_init>
inline interval<value_init> operator*(interval<value_init> const &Operand1,
									  interval<value_init> const &Operand2);
template <class value_init>
inline interval<value_init> operator/(interval<value_init> const &Operand1,
									  interval<value_init> const &Operand2);
// TODO: It is an error to use value_init in place of
// interval<value_init>::value here, but I cannot avoid it if I want
// these declarations to occur before the class, which I do (for
// readability)  Those who wish to flog me for this may flog me.
template <class value_init>
inline interval<value_init> operator+(interval<value_init> const &Operand1,
									  value_init const &Operand2);
template <class value_init>
inline interval<value_init> operator-(interval<value_init> const &Operand1,
									  value_init const &Operand2);
template <class value_init>
inline interval<value_init> operator*(interval<value_init> const &Operand1,
									  value_init const &Operand2);
template <class value_init>
inline interval<value_init> operator/(interval<value_init> const &Operand1,
									  value_init const &Operand2);

template <class from_value, class to_value>
inline to_value MapValueBetweenLinearIntervals(
	interval<from_value> const &FromInterval,
	from_value const &Value,
	interval<to_value> const &ToInterval);

template <class value_init>
inline interval<value_init> ReversedInterval(
	interval<value_init> const &Interval);

/* ************************************************************************
   Class: interval
   Description: A class that represents an interval closed on the minimum
                and open on the maximum.  I decided this somewhat
				arbitrarily since it seemed to be bset done this way, but
				I am not sure if there is some topological reason to choose
				one open/closed convention over another.  The entire class
			    probably looks quite lame to somebody who knows interval
				analysis well, since I don't, so feel free to give me some
				tips (or wait for me to do my reading homework, one or the
				other)

   TODO: It would be nice if all template functions didn't have to be
         inlined.  And technically, they probaby don't have to be, because
		 it's a pretty safe bet that nobody will be using this with
		 numbers that aren't real32's or real64's, and thus I could
		 explicitly instantiate those two and just be done with it.
   ************************************************************************ */
template <class value_init>
class interval
{
public:
	// Types
	typedef value_init value;

	// Queries
	inline value const &GetMinimum(void) const;
	inline value const &GetMaximum(void) const;

	inline value GetLength(void) const;
	
	inline bool IsEmpty(void) const;
	inline bool Contains(value const &Value) const;
	inline bool EndsBefore(value const &Value) const;
	inline bool StartsAfter(value const &Value) const;

	inline bool Intersects(interval<value> const &Interval) const;
	inline bool Contains(interval<value> const &Interval) const;
	inline bool EndsBefore(interval<value> const &Interval) const;
	inline bool StartsAfter(interval<value> const &Interval) const;

	// Modification
	inline void SetMinimum(value const &Value);
	inline void SetMaximum(value const &Value);
	inline void SetLength(value const &Value);
	inline void EnsureInclusion(value const &Value);
	inline void EnsureInclusion(interval<value> const &Interval);

	// Existence
	inline interval(void);
	inline interval(value const &Minimum, value const &Maximum);
	
private:
	value Minimum;
	value Maximum;
};

/* ========================================================================
   Inline Functions
   ======================================================================== */
template <class value_init>
inline interval<value_init>
operator+(interval<value_init> const &Operand1,
		  interval<value_init> const &Operand2)
{
	return(interval<value_init>(
		Operand1.GetMinimum() + Operand2.GetMinimum(),
		Operand2.GetMaximum() + Operand2.GetMaximum()));
}

template <class value_init>
inline interval<value_init>
operator-(interval<value_init> const &Operand1)
{
	return(interval<value_init>(
		-Operand1.GetMaximum(), -Operand1.GetMinimum()));
}

template <class value_init>
inline interval<value_init>
operator-(interval<value_init> const &Operand1,
		  interval<value_init> const &Operand2)
{
	return(interval<value_init>(
		Operand1.GetMinimum() - Operand2.GetMaximum(),
		Operand1.GetMaximum() - Operand2.GetMinimum()));
}

template <class value_init>
inline interval<value_init>
operator*(interval<value_init> const &Operand1,
		  interval<value_init> const &Operand2)
{
	NOT_YET_IMPLEMENTED;
}

template <class value_init>
inline interval<value_init>
operator/(interval<value_init> const &Operand1,
		  interval<value_init> const &Operand2)
{
	NOT_YET_IMPLEMENTED;
}

template <class value_init>
inline interval<value_init>
operator+(interval<value_init> const &Operand1,
		  interval<value_init>::value const &Operand2)
{
	return(interval<value_init>(Operand1.GetMinimum() + Operand2,
								Operand1.GetMaximum() + Operand2));
}

template <class value_init>
inline interval<value_init>
operator-(interval<value_init> const &Operand1,
		  interval<value_init>::value const &Operand2)
{
	return(interval<value_init>(Operand1.GetMinimum() - Operand2,
								Operand1.GetMaximum() - Operand2));
}

template <class value_init>
inline interval<value_init>
operator*(interval<value_init> const &Operand1,
		  interval<value_init>::value const &Operand2)
{
	if(IsPositive(Operand2))
	{
		return(interval<value_init>(Operand1.GetMinimum() * Operand2,
									Operand1.GetMaximum() * Operand2));
	}
	else
	{
		return(interval<value_init>(Operand1.GetMaximum() * Operand2,
									Operand1.GetMinimum() * Operand2));
	}
}

template <class value_init>
inline interval<value_init>
operator/(interval<value_init> const &Operand1,
		  interval<value_init>::value const &Operand2)
{
	// TODO: This really needs to be called Inverse, but the damn
	// windows API clobbers it... there has to be some way I can
	// prevent that
	interval<value_init>::value InverseOperand2 = RealInverse(Operand2);
	if(IsPositive(InverseOperand2))
	{
		return(interval<value_init>(Operand1.GetMinimum() * InverseOperand2,
									Operand1.GetMaximum() * InverseOperand2));
	}
	else
	{
		return(interval<value_init>(Operand1.GetMaximum() * InverseOperand2,
									Operand1.GetMinimum() * InverseOperand2));
	}
}

template <class value_init>
inline interval<value_init>::value const &interval<value_init>::
GetMinimum(void) const
{
	return(Minimum);
}

template <class value_init>
inline interval<value_init>::value const &interval<value_init>::
GetMaximum(void) const
{
	return(Maximum);
}

template <class value_init>
inline interval<value_init>::value interval<value_init>::
GetLength(void) const
{
	return(GetMaximum() - GetMinimum());
}

template <class value_init>
inline bool interval<value_init>::
IsEmpty(void) const
{
	return(GetMinimum() > GetMaximum());
}

template <class value_init>
inline bool interval<value_init>::
Contains(value const &Value) const
{
	return((Value >= GetMinimum()) && (Value <= GetMaximum()));
}

template <class value_init>
inline bool interval<value_init>::
EndsBefore(value const &Value) const
{
	return(GetMaximum() < Value);
}

template <class value_init>
inline bool interval<value_init>::
StartsAfter(value const &Value) const
{
	return(GetMinimum() > Value);
}

template <class value_init>
inline bool interval<value_init>::
Intersects(interval<value> const &Interval) const
{
	return(!(((GetMinimum() < Interval.GetMinimum()) &&
			  (GetMaximum() < Interval.GetMaximum())) ||
			 ((GetMinimum() > Interval.GetMinimum()) &&
			  (GetMaximum() > Interval.GetMaximum()))));
}

template <class value_init>
inline bool interval<value_init>::
Contains(interval<value> const &Interval) const
{
	return((Interval.GetMinimum() >= GetMinimum()) &&
		   (Interval.GetMaximum() <= GetMaximum()));
}

template <class value_init>
inline bool interval<value_init>::
EndsBefore(interval<value> const &Interval) const
{
	return(GetMaximum() < Interval.GetMinimum());	
}

template <class value_init>
inline bool interval<value_init>::
StartsAfter(interval<value> const &Interval) const
{
	return(GetMinimum() > Interval.GetMaximum());
}

template <class value_init>
inline void interval<value_init>::
SetMinimum(value const &Value)
{
	Minimum = Value;
}

template <class value_init>
inline void interval<value_init>::
SetMaximum(value const &Value)
{
	Maximum = Value;
}

template <class value_init>
inline void interval<value_init>::
SetLength(value const &Value)
{
	SetMaximum(GetMinimum() + Value);
}

template <class value_init>
inline void interval<value_init>::
EnsureInclusion(value const &Value)
{
	if(GetMinimum() > Value)
	{
		SetMinimum(Value);
	}

	if(GetMaximum() < Value)
	{
		SetMaximum(Value);
	}
}

template <class value_init>
inline void interval<value_init>::
EnsureInclusion(interval<value> const &Interval)
{
	if(GetMinimum() > Interval.GetMinimum())
	{
		SetMinimum(Interval.GetMinimum());
	}

	if(GetMaximum() < Interval.GetMaximum())
	{
		SetMaximum(Interval.GetMaximum());
	}
}

template <class value_init>
inline interval<value_init>::
interval(void)
		: Minimum(1),
		  Maximum(-1)
{
	// TODO: Traits so I can get the minimum and maximum values here
}

template <class value_init>
inline interval<value_init>::
interval(value const &MinimumInit, value const &MaximumInit)
		: Minimum(MinimumInit),
		  Maximum(MaximumInit)
{
	// TODO: Should I assert that minimum is less than maximum here?
}

template <class from_value, class to_value>
inline to_value
MapValueBetweenLinearIntervals(interval<from_value> const &FromInterval,
							   from_value const &Value,
							   interval<to_value> const &ToInterval)
{
	return(ToInterval.GetMinimum() +
		   (ToInterval.GetLength() *
			((Value - FromInterval.GetMinimum()) / FromInterval.GetLength())));
}

template <class value_init>
inline interval<value_init>
ReversedInterval(interval<value_init> const &Interval)
{
	return(interval<value_init>(Interval.GetMaximum(), Interval.GetMinimum()));
}

#define INTERVAL_H
#endif
#endif // 0
