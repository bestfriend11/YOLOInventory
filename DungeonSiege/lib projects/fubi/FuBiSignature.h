//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiSignature.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains specialized scanner/parser classes.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FUBISIGNATURE_H
#define __FUBISIGNATURE_H

//////////////////////////////////////////////////////////////////////////////

// if you get this error then you are #including FuBiSignature.h and you
// shouldn't be! did you really want to #include FuBiDefs.h instead? build
// problems will occur if you try to use FuBiSignature.h directly!
#ifndef __PRECOMP_FUBI_H
#error Do not #include FuBiSignature.h directly! Use FuBiDefs.h instead!
#endif

//////////////////////////////////////////////////////////////////////////////

#include "FuBiTypes.h"
#include <vector>

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// Define YYSTYPE for base classes

struct ScanStruct
{
	enum
	{
		FLAG_SIGNED   = 1 << 0,
		FLAG_UNSIGNED = 1 << 1,
		FLAG_SHORT    = 1 << 2,
		FLAG_INT      = 1 << 3,
		FLAG_LONG     = 1 << 4,
		FLAG_SPECIAL  = 1 << 5,
	};

	int      m_Token;		// what's my token?
	gpstring m_String;		// for identifiers
	int      m_Int;			// for integer constants
	TypeSpec m_Type;		// for types
	DWORD    m_TypeFlags;	// short/long/signed type modifiers
};

// lex/yacc needs this stuff
typedef ScanStruct YYSTYPE;
#define YYSTYPE YYSTYPE

// pick up base classes now that YYSTYPE is available
#if GP_DEBUG
#include "FuBiSignature-L-Debug.h.out"
#include "FuBiSignature-Y-Debug.h.out"
#elif GP_RELEASE
#include "FuBiSignature-L-Release.h.out"
#include "FuBiSignature-Y-Release.h.out"
#elif GP_RETAIL
#include "FuBiSignature-L-Retail.h.out"
#include "FuBiSignature-Y-Retail.h.out"
#elif GP_PROFILING
#include "FuBiSignature-L-Profiling.h.out"
#include "FuBiSignature-Y-Profiling.h.out"
#endif

//////////////////////////////////////////////////////////////////////////////
// class SignatureScanner declaration

class SignatureScanner : public SignatureBaseScanner
{
public:
	SET_INHERITED( SignatureScanner, SignatureBaseScanner );

	SignatureScanner( void );
	virtual ~SignatureScanner( void );

	virtual void Reset( void );

	bool IsPerfect( void ) const  {  return ( m_Perfect );  }

private:
	virtual void Message( eMessageType type, const char* msg, ... );

	bool m_Perfect;

	friend Inherited;
	SET_NO_COPYING( SignatureScanner );
};

//////////////////////////////////////////////////////////////////////////////
// class SignatureParser declaration

class SignatureParser : public SignatureBaseParser
{
public:
	SET_INHERITED( SignatureParser, SignatureBaseParser );

	enum eParse
	{
		PARSE_OK,			// success
		PARSE_ERROR,		// error in parsing
		PARSE_IGNORE,		// don't use this one
	};

	SignatureParser( void );
	virtual ~SignatureParser( void );

	virtual void Reset( void );

	eParse Parse( const char* text, FunctionSpec& spec );

	const TypeSpec& GetTraitType( void ) const			{  return ( m_TraitType );  }

private:
	virtual void           Message  ( eMessageType type, const char* msg, ... );
	virtual int            Scan     ( void )			{  return ( m_Scanner.Scan() );  }
	virtual const YYSTYPE& GetLValue( void ) const		{  return ( m_Scanner.GetLValue() );  }
	virtual bool           IsEof    ( void ) const		{  return ( m_Scanner.IsEof() );  }

	SignatureScanner m_Scanner;
	FunctionSpec*    m_FunctionSpec;
	TypeSpec         m_TraitType;
	bool             m_Perfect;

	friend Inherited;
	SET_NO_COPYING( SignatureParser );
};

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

#endif  // __FUBISIGNATURE_H

//////////////////////////////////////////////////////////////////////////////
