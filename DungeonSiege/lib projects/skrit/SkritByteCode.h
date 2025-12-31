//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritByteCode.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains Skrit bytecode enums
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __SKRITBYTECODE_H
#define __SKRITBYTECODE_H

//////////////////////////////////////////////////////////////////////////////

#include "GPCore.h"

namespace Skrit  {  // begin of namespace Skrit
namespace Op     {  // begin of namespace Op

//////////////////////////////////////////////////////////////////////////////
// Traits enum

enum Traits
{
	TRAIT_NONE       =       0,				// default traits
	TRAIT_RETURN     = 1 <<  0,				// this is a return opcode
	TRAIT_MATH       = 1 <<  1,				// this is a math opcode (+/- etc)
	TRAIT_BITWISE    = 1 <<  2,				// this is a bitwise opcode (&/| etc)
	TRAIT_LOGIC      = 1 <<  3,				// this is a logic opcode (&&/|| etc)
	TRAIT_BRANCH     = 1 <<  4,				// this is a branching opcode
	TRAIT_COMPARE    = 1 <<  5,				// this is a comparison opcode (>=/< etc)
	TRAIT_MEMBERCALL = 1 <<  6,				// this is a nonstatic member function call opcode
	TRAIT_BASEOFFSET = 1 <<  7,				// this op requires a base offset added to the derived pointer
	TRAIT_INT        = 1 <<  8,				// op ok for ints
	TRAIT_FLOAT      = 1 <<  9,				// op ok for floats
	TRAIT_BOOL       = 1 << 10,				// op ok for bools
	TRAIT_STRING     = 1 << 11,				// op ok for gpstrings
	TRAIT_CSTRING    = 1 << 12,				// op ok for 'c' strings
	TRAIT_ENUM       = 1 << 13,				// op ok for enums
	TRAIT_REFERENCE  = 1 << 14,				// op ok for pointers and handles

	TRAIT_DUMMY      = 1 << 31,				// placeholder opcode (illegal to call)
};

//////////////////////////////////////////////////////////////////////////////
// Code enum

enum Code
{
	SET_BEGIN_ENUM( CODE_, 0 ),
#	define   SKRITBYTECODE_IS_H 1
#	include "SkritBytecode.inc"
	SET_END_ENUM( CODE_ ),
};

COMPILER_ASSERT( CODE_END < 256 );		// must be a BYTE code, eh?

//////////////////////////////////////////////////////////////////////////////
// Code data query

struct CodeData
{
	const char* m_Name;
	int         m_NameLen;
	int         m_DataSizeBytes;
	Traits      m_Traits;
};

const CodeData& GetData( Code code );

int GetMaxNameLen( void );
int GetMaxDataSizeBytes( void );

inline const char* ToString( Code code )
	{  return ( GetData( code ).m_Name );  }
inline Traits GetTraits( Code code )
	{  return ( GetData( code ).m_Traits );  }

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Op

MAKE_ENUM_BIT_OPERATORS ( Op::Traits );
MAKE_ENUM_MATH_OPERATORS( Op::Code   );

}  // end of namespace Skrit

#endif  // __SKRITBYTECODE_H

//////////////////////////////////////////////////////////////////////////////
