//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritByteCode.inc
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#if SKRITBYTECODE_IS_H
#	define ADD_ENTRY( code, count )  CODE_##code,
#	define ADD_ENTRY_TRAIT( code, count, trait )  CODE_##code,
#elif SKRITBYTECODE_IS_CPP
#	define ADD_ENTRY( code, count )  {  #code, sizeof( #code ) - 1, count, TRAIT_NONE  },
#	define ADD_ENTRY_TRAIT( code, count, trait )  {  #code, sizeof( #code ) - 1, count, trait  },
#else
#	error ! Do NOT include this file directly. Use SkritBytecode.h instead.
#	error ! This file is meant as a helper for SkritBytecode.cpp/h
#endif

#if SKRITBYTECODE_IS_H && SKRITBYTECODE_IS_CPP
#	error ! Improper usage - cannot be both H and CPP.
#endif

//////////////////////////////////////////////////////////////////////////////
// the bytecodes

// Usage:  $$$ move to better location

	/*	Each entry will consist of its name and the number of data bytes
		packed after the opcode that the instruction will use. Note that if it
		uses the stack for its data, that is not included in the data byte
		usage count.

		Ints, floats, and bools are all simple elements stored on the stack at
		four bytes apiece. Strings are special: they are actually indexes into
		a vector of gpstrings owned by the VM that must be converted to const
		char* pointers before any function calls. Pointers are special, but only
		in that they can't really have any arithmetic performed on them. Handles
		are special in that they must be AddRef()'d upon acquisition and
		Release()'d upon exiting the scope they're used in.

		The stack behaves exactly as in C so that memory blocks can be passed
		directly to FuBi. It grows downward, a push will decrement first, then
		store data, and a pop will load data first then increment. Everything
		is dword sized (with exceptions noted). Note that when I say "top" of
		the stack I'm actually talking about the bottom (damn backwards
		conventional terminology).

		The term "stack index" refers to a 0-based index from the "top" of the
		stack. It's always a byte in size and can easily be dereferenced by:

			void* mem = stack_pointer - ((stack_index + 1) * 4)

		Type codes used here:

			B = boolean
			C = const char*
			O = offset into string table (constant)
			F = float
			H = managed class handle
			I = integer
			P = general pointer
			S = (gp)string - actually a string handle
	*/

// General.

	// op  : nop
	// data: none

		ADD_ENTRY( SITNSPIN, 0 )

	// op  : abort skrit with fatal error
	// data: none

		ADD_ENTRY( ABORT, 0 )

// Flow control.

	// op  : return from current function
	// data: none

		ADD_ENTRY_TRAIT( RETURN0, 0, TRAIT_RETURN )

	// op  : pop the top element off the stack and store in eax, then return
	// data: none

		ADD_ENTRY_TRAIT( RETURN, 0, TRAIT_RETURN )

	// op  : call the given function with the vm's current stack as params
	//       - if vararg, # params will appear on top of the stack
	//       - if member, "this" as handle or pointer will appear beneath all the params
	// data: serial ID of function

		ADD_ENTRY      ( CALL,              2 )
		ADD_ENTRY_TRAIT( CALLSINGLETON,     2, TRAIT_MEMBERCALL )
		ADD_ENTRY_TRAIT( CALLMEMBER,        2, TRAIT_MEMBERCALL )
		ADD_ENTRY_TRAIT( CALLSINGLETONBASE, 4, TRAIT_MEMBERCALL | TRAIT_BASEOFFSET )
		ADD_ENTRY_TRAIT( CALLMEMBERBASE,    4, TRAIT_MEMBERCALL | TRAIT_BASEOFFSET )

	// op  : call the given function with the vm's current stack as params
	// data: index of skrit function to call

		ADD_ENTRY      ( CALLSKRIT, 2 )

	// op  : call the given function with the vm's current stack as params
	//       - Object* will appear beneath all the params
	// data: index of skrit function to call

//$$$	ADD_ENTRY      ( CALLFOREIGNSKRIT, 2 )

	// op  : add offset to IP
	// data: (signed) offset from current IP to jump to

		ADD_ENTRY_TRAIT( BRANCH, 2, TRAIT_BRANCH )

	// op  : pop int off stack, if zero then add offset to IP
	// data: (signed) offset from current IP to jump to

		ADD_ENTRY_TRAIT( BRANCHZERO, 2, TRAIT_BRANCH )

// Arithmetic.

	// op  : add/subtract/multiply/divide top two items on the stack
	// data: none
	// note: ordering goes *(top - 1) <op> *(top)

		ADD_ENTRY_TRAIT( ADDI, 0, TRAIT_MATH | TRAIT_INT | TRAIT_FLOAT )		// int + int
		ADD_ENTRY_TRAIT( ADDF, 0, TRAIT_MATH | TRAIT_INT | TRAIT_FLOAT )		// float + float

		ADD_ENTRY_TRAIT( SUBTRACTI, 0, TRAIT_MATH | TRAIT_INT | TRAIT_FLOAT )	// int - int
		ADD_ENTRY_TRAIT( SUBTRACTF, 0, TRAIT_MATH | TRAIT_INT | TRAIT_FLOAT )	// float - float

		ADD_ENTRY_TRAIT( MULTIPLYI, 0, TRAIT_MATH | TRAIT_INT | TRAIT_FLOAT )	// int * int
		ADD_ENTRY_TRAIT( MULTIPLYF, 0, TRAIT_MATH | TRAIT_INT | TRAIT_FLOAT )	// float * float

		ADD_ENTRY_TRAIT( DIVIDEI, 0, TRAIT_MATH | TRAIT_INT | TRAIT_FLOAT )		// int / int
		ADD_ENTRY_TRAIT( DIVIDEF, 0, TRAIT_MATH | TRAIT_INT | TRAIT_FLOAT )		// float / float

		ADD_ENTRY_TRAIT( MODULOI, 0, TRAIT_MATH | TRAIT_INT | TRAIT_FLOAT )		// int % int
		ADD_ENTRY_TRAIT( MODULOF, 0, TRAIT_MATH | TRAIT_INT | TRAIT_FLOAT )		// float % float

		ADD_ENTRY_TRAIT( POWERI, 0, TRAIT_MATH | TRAIT_INT | TRAIT_FLOAT )		// int ** int
		ADD_ENTRY_TRAIT( POWERF, 0, TRAIT_MATH | TRAIT_INT | TRAIT_FLOAT )		// float ** float

	// op  : concatenate top (minus one) two items on the stack, store in top item
	// data: none
	// note: ordering goes *(top) = (*top - 2) + (*top - 1)

		ADD_ENTRY( ADDSS, 0 )													// string + string (append)
		ADD_ENTRY( ADDSC, 0 )													// string + const char* (append)
		ADD_ENTRY( ADDCS, 0 )													// const char* + string (append)
		ADD_ENTRY( ADDCC, 0 )													// const char* + const char* (append)

	// op  : negate top item on stack
	// data: none

		ADD_ENTRY_TRAIT( NEGATEI, 0, TRAIT_MATH | TRAIT_INT | TRAIT_FLOAT )		// -int
		ADD_ENTRY_TRAIT( NEGATEF, 0, TRAIT_MATH | TRAIT_INT | TRAIT_FLOAT )		// -float

// Logic.

	// op  : bitwise integer-only operations to top two items on the stack
	// data: none
	// note: ordering goes *(top - 1) <op> *(top)

		ADD_ENTRY_TRAIT( BANDI, 0, TRAIT_BITWISE | TRAIT_REFERENCE )			// int & int
		ADD_ENTRY_TRAIT( BXORI, 0, TRAIT_BITWISE | TRAIT_REFERENCE )			// int ^ int
		ADD_ENTRY_TRAIT( BORI,  0, TRAIT_BITWISE | TRAIT_REFERENCE )			// int | int

	// op  : bitwise "not" top item on stack
	// data: none

		ADD_ENTRY_TRAIT( BNOTI, 0, TRAIT_BITWISE | TRAIT_REFERENCE )			// ~int

	// op  : binary integer-only operations to top two items on the stack
	// data: none
	// note: ordering goes *(top - 1) <op> *(top)

		ADD_ENTRY_TRAIT( ANDIB, 0, TRAIT_LOGIC | TRAIT_REFERENCE )				// int-or-bool && int-or-bool
		ADD_ENTRY_TRAIT( ORIB,  0, TRAIT_LOGIC | TRAIT_REFERENCE )				// int-or-bool || int-or-bool

	// op  : logical "not" top item on stack
	// data: none

		ADD_ENTRY_TRAIT( NOTIB, 0, TRAIT_LOGIC | TRAIT_REFERENCE )				// !int-or-bool

// Comparisons.

	// op  : compare top two items on the stack (push 0/1 result)
	// data: none
	// note: ordering goes *(top - 1) <op> *(top)

		ADD_ENTRY_TRAIT( ISEQUALI, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING | TRAIT_ENUM | TRAIT_REFERENCE )		// int == int
		ADD_ENTRY_TRAIT( ISEQUALF, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING | TRAIT_ENUM | TRAIT_REFERENCE )		// float == float
		ADD_ENTRY_TRAIT( ISEQUALB, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING | TRAIT_ENUM | TRAIT_REFERENCE )		// bool == bool
		ADD_ENTRY_TRAIT( ISEQUALS, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING | TRAIT_ENUM | TRAIT_REFERENCE )		// string == string (case-sensitive)
		ADD_ENTRY_TRAIT( ISEQUALC, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING | TRAIT_ENUM | TRAIT_REFERENCE )		// const char* == const char* (case-sensitive)

		ADD_ENTRY_TRAIT( ISNOTEQUALI, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING | TRAIT_ENUM | TRAIT_REFERENCE  )		// int != int
		ADD_ENTRY_TRAIT( ISNOTEQUALF, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING | TRAIT_ENUM | TRAIT_REFERENCE  )		// float != float
		ADD_ENTRY_TRAIT( ISNOTEQUALB, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING | TRAIT_ENUM | TRAIT_REFERENCE  )		// bool != bool
		ADD_ENTRY_TRAIT( ISNOTEQUALS, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING | TRAIT_ENUM | TRAIT_REFERENCE  )		// string != string (case-sensitive)
		ADD_ENTRY_TRAIT( ISNOTEQUALC, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING | TRAIT_ENUM | TRAIT_REFERENCE  )		// const char* != const char* (case-sensitive)

		ADD_ENTRY_TRAIT( ISAPPROXEQUALI_DUMMY, 0, TRAIT_DUMMY | TRAIT_COMPARE | TRAIT_FLOAT | TRAIT_STRING | TRAIT_CSTRING )
		ADD_ENTRY_TRAIT( ISAPPROXEQUALF,       0,               TRAIT_COMPARE | TRAIT_FLOAT | TRAIT_STRING | TRAIT_CSTRING )		// float approximately equal to float (within FLOAT_TOLERANCE)
		ADD_ENTRY_TRAIT( ISAPPROXEQUALB_DUMMY, 0, TRAIT_DUMMY | TRAIT_COMPARE | TRAIT_FLOAT | TRAIT_STRING | TRAIT_CSTRING )
		ADD_ENTRY_TRAIT( ISAPPROXEQUALS,       0,               TRAIT_COMPARE | TRAIT_FLOAT | TRAIT_STRING | TRAIT_CSTRING )		// string != string (case-insensitive)
		ADD_ENTRY_TRAIT( ISAPPROXEQUALC,       0,               TRAIT_COMPARE | TRAIT_FLOAT | TRAIT_STRING | TRAIT_CSTRING )		// const char* != const char* (case-insensitive)

		ADD_ENTRY_TRAIT( ISGREATERI, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )			// int > int
		ADD_ENTRY_TRAIT( ISGREATERF, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )			// float > float
		ADD_ENTRY_TRAIT( ISGREATERB, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )			// bool > bool
		ADD_ENTRY_TRAIT( ISGREATERS, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )			// string > string
		ADD_ENTRY_TRAIT( ISGREATERC, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )			// const char* > const char*

		ADD_ENTRY_TRAIT( ISLESSI, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )				// int < int
		ADD_ENTRY_TRAIT( ISLESSF, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )				// float < float
		ADD_ENTRY_TRAIT( ISLESSB, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )				// bool < bool
		ADD_ENTRY_TRAIT( ISLESSS, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )				// string < string
		ADD_ENTRY_TRAIT( ISLESSC, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )				// const char* < const char*

		ADD_ENTRY_TRAIT( ISGREATEREQUALI, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )		// int >= int
		ADD_ENTRY_TRAIT( ISGREATEREQUALF, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )		// float >= float
		ADD_ENTRY_TRAIT( ISGREATEREQUALB, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )		// bool >= bool
		ADD_ENTRY_TRAIT( ISGREATEREQUALS, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )		// string >= string
		ADD_ENTRY_TRAIT( ISGREATEREQUALC, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )		// const char* >= const char*

		ADD_ENTRY_TRAIT( ISLESSEQUALI, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )			// int <= int
		ADD_ENTRY_TRAIT( ISLESSEQUALF, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )			// float <= float
		ADD_ENTRY_TRAIT( ISLESSEQUALB, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )			// bool <= bool
		ADD_ENTRY_TRAIT( ISLESSEQUALS, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )			// string <= string
		ADD_ENTRY_TRAIT( ISLESSEQUALC, 0, TRAIT_COMPARE | TRAIT_INT | TRAIT_FLOAT | TRAIT_BOOL | TRAIT_STRING | TRAIT_CSTRING| TRAIT_ENUM )			// const char* <= const char*

// Type conversions.

	// op  : convert among float, int, bool, string, const char*
	// data: stack index of item to convert

		ADD_ENTRY( INTTOBOO, 1 )								// int -> bool
		ADD_ENTRY( INTTOFLT, 1 )								// int -> float
		ADD_ENTRY( FLTTOINT, 1 )								// float -> int
		ADD_ENTRY( FLTTODBL, 1 )								// float -> double
		ADD_ENTRY( OFFTOPCC, 1 )								// string constant offset -> const char*
		ADD_ENTRY( STRTOPCC, 1 )								// gpstring* -> const char*

	// op  : convert const char* to gpstring* - store cstr in str then replace with str*
	// data: word = index of string to store in, plus eStore byte for var location store, plus stack index of item to convert

		ADD_ENTRY( CSTRTOSTR, 4 )								// const char* -> gpstring*

	// op  : convert handle to pointer
	// data: word - eVarType of managed class, byte - stack index of item to convert

		ADD_ENTRY( HDLTOPTR, 3 )

	// op  : offset derived to base class
	// data: word - offset, byte - stack index of item to convert

		ADD_ENTRY( BASEADJUST, 3 )

	// op  : mask the high bytes on the value at the top of the stack. necessary for function calls.
	// data: none

		ADD_ENTRY( MASK3, 0 )									// mask top three bytes
		ADD_ENTRY( MASK2, 0 )									// mask top two bytes

// Stack operations.

	// op  : push a constant given by the data onto the expression stack
	// data: constant value - type-dependent

		ADD_ENTRY( PUSH1, 1 )
		ADD_ENTRY( PUSH2, 2 )
		ADD_ENTRY( PUSH4, 4 )

	// op  : push "this" (skrit object pointer) onto the expression stack
	// data: none

		ADD_ENTRY( PUSHTHIS, 0 )

	// op  : push currently executing skrit virtual machine pointer onto the expression stack
	// data: none

		ADD_ENTRY( PUSHVM, 0 )

	// op  : push "owner" (arbitrary dword set by caller) onto the expression stack
	// data: none

		ADD_ENTRY( PUSHOWNER, 0 )

	// op  : push the next parameter from the incoming args onto the expression stack and advance the pointers
	// data: type-dependent

		ADD_ENTRY( PUSHPRM, 0 )

	// op  : skip to the next param in the incoming args
	// data: none

		ADD_ENTRY( SKIPPRM, 0 )

	// op  : swap the top two elements on the stack
	// data: none

		ADD_ENTRY( SWAP, 0 )

	// op  : pop the top x elements off a stack and throw them away
	// data: number of stack entries to toss

		ADD_ENTRY( POP,  1 )

	// op  : pop the top element off the expression stack and throw it away
	// data: none

		ADD_ENTRY( POP1, 0 )

	// op  : load variable/string/handle and push it onto the expression stack
	// data: word = index of variable/string/handle to load, plus eStore byte for var location store

		ADD_ENTRY( LOADVAR,  3 )
		ADD_ENTRY( LOADSTR,  3 )		// load gpstring* from string collection
		ADD_ENTRY( LOADCSTR, 3 )		// load char* from string collection
		ADD_ENTRY( LOADHDL,  3 )

	// op  : store variable/string/handle from top of expression stack and pop it off
	// data: word = index of variable/string/handle to store, plus eStore byte for var location store

		ADD_ENTRY( STOREVAR,  3 )
		ADD_ENTRY( STORESTR,  3 )		// store gpstring* in string collection
		ADD_ENTRY( STORECSTR, 3 )		// store char* in string collection
		ADD_ENTRY( STOREHDL,  3 )

// Storage operations.

	// op  : alloc space for a new simple variable or string
	// data: none - initialized to zero/empty

		ADD_ENTRY( ALLOCVAR, 0 )
		ADD_ENTRY( ALLOCSTR, 0 )

	// op  : alloc space for a new handle
	// data: eVarType of managed class, handle is initialized to zero

		ADD_ENTRY( ALLOCHDL, 2 )

	// op  : free the last x elements from a stack
	// data: number of entries to toss

		ADD_ENTRY( FREEVAR, 1 )
		ADD_ENTRY( FREESTR, 1 )
		ADD_ENTRY( FREEHDL, 1 )

// State operations.

	// op  : set the state index to the new value
	// data: new state

		ADD_ENTRY( SETSTATE, 2 )

//////////////////////////////////////////////////////////////////////////////

#undef ADD_ENTRY
#undef ADD_ENTRY_TRAIT

#ifdef SKRITBYTECODE_IS_H
#undef SKRITBYTECODE_IS_H
#endif
#ifdef SKRITBYTECODE_IS_CPP
#undef SKRITBYTECODE_IS_CPP
#endif

//////////////////////////////////////////////////////////////////////////////
