//////////////////////////////////////////////////////////////////////////////
//
// File     :  GpAssert.h
// Author(s):  Bartosz Kijanka (original version), Scott Bilas (++steroids)
//
// Summary  :  Contains run-time code assertions.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GPASSERT_H
#define __GPASSERT_H

//////////////////////////////////////////////////////////////////////////////
// unwanted?

#ifdef NOGPCORE

#include <cassert>
#define gpassert ( exp )           assert( exp )
#define gpassertm( exp, message )  assert( exp )

#else

//////////////////////////////////////////////////////////////////////////////
// wrap c-style assert macro

#include "GpCore.h"

// take over old assert
#ifdef assert
#undef assert
#endif

#define assert gpassert

//////////////////////////////////////////////////////////////////////////////
// assertion macros

// note: to use the "f" versions of the below macros, enclose the exp in
//  double-parens, like this:
//
// gpassertf( exp, ( "Error '%d'\n", error_code ) );

// note: the difference between an assertion and a verification is that the
//  assert will compile out in non-debug modes, whereas a verify will still
//  evaluate the expression. non-debug modes will never show the assert dialog
//  though.

#ifdef _DEBUG	// $ don't use GP_DEBUG, it may not be set up yet

// assert on expression and provide nice string if it fails
#define gpassertm( exp, msg )         gpassert_raw( exp, msg, "Assertion" )
#define gpverifym( exp, msg )         gpassert_raw( exp, msg, "Verification" )
#define if_gpassertm( exp, msg )      if_gpassert_raw( exp, msg, "(If)Assertion" )
#define if_not_gpassertm( exp, msg )  if_not_gpassert_raw( exp, msg, "(If)Assertion" )

// assert on expression and provide formatted string if it fails
#define gpassertf( exp, msg )         gpassert_formatted( exp, msg, "Assertion" )
#define gpverifyf( exp, msg )         gpassert_formatted( exp, msg, "Verification" )
#define if_gpassertf( exp, msg )      if_gpassert_formatted( exp, msg, "(If)Assertion" )
#define if_not_gpassertf( exp, msg )  if_not_gpassert_formatted( exp, msg, "(If)Assertion" )

// assert on expression
#define gpassert( exp )         gpassertm( exp, 0 )
#define gpverify( exp )         gpverifym( exp, 0 )
#define if_gpassert( exp )      if_gpassertm( exp, 0 )
#define if_not_gpassert( exp )  if_not_gpassertm( exp, 0 )

#else // _DEBUG

#define gpassertm( exp, msg )
#define gpverifym( exp, msg )         (exp)
#define if_gpassertm( exp, msg )      if ( exp )
#define if_not_gpassertm( exp, msg )  if ( !(exp) )

#define gpassertf( exp, msg )
#define gpverifyf( exp, msg )         (exp)
#define if_gpassertf( exp, msg )      if ( exp )
#define if_not_gpassertf( exp, msg )  if ( !(exp) )

#define gpassert( exp )
#define gpverify( exp )         (exp)
#define if_gpassert( exp )      if ( exp )
#define if_not_gpassert( exp )  if ( !(exp) )

#endif // _DEBUG

//////////////////////////////////////////////////////////////////////////////
// debugger output macros

// for talking to the debugger (like ODS except w/ formatting)
void OutputDebugStringF( const char* format, ... );

// reporting macros
#if GP_RETAIL
#	define OutputDebugStringF( args )
#else
#	define OutputDebugStringF( args ) OutputDebugStringF args
#endif

//////////////////////////////////////////////////////////////////////////////
// assertion support

namespace ReportSys
{
	class Context;
}

struct AssertInstance
{
	int                    m_IgnoreCount;
	int                    m_FailCount;
	const char*            m_File;
	int                    m_Line;
	AssertInstance*        m_Next;
	static AssertInstance* ms_Root;

	AssertInstance( const char* file = NULL, int line = 0, bool autoRegister = false )
	{
		m_IgnoreCount = 0;
		m_FailCount   = 0;
		m_File        = file;
		m_Line        = line;
		m_Next        = NULL;

		if ( autoRegister )
		{
			Register();
		}
	}

	void Register( void )
	{
		m_Next = (AssertInstance*)::InterlockedExchange( (LONG*)&ms_Root, (DWORD)this );
	}

	bool operator < ( const AssertInstance& other ) const;

	static void ReportSummary( ReportSys::Context* context = NULL );
};

struct AssertData
{
	enum  {  BUFFER_SIZE = 1000  };

	enum eStyle
	{
		STYLE_NORMAL,							// an ordinary assert dialog
		STYLE_HATCHED,							// special hatched line dialog

		STYLE_DEFAULT = STYLE_NORMAL,
	};

	static int  ms_AssertLevel;
	static int  ms_FailedCount;
	static int  ms_GlobalIgnoreCount;
	static char ms_Buffer[ BUFFER_SIZE ];

	__declspec ( thread ) static bool ms_TempBad;

	const char* m_Type;
	const char* m_Expression;
	const char* m_File;
	int         m_Line;
	const char* m_Message;
	DWORD       m_StartFrame;
	int         m_FailCount;
	eStyle      m_Style;

	AssertData( const char* type, const char* exp, const char* file, int line,
				const char* msg, DWORD startFrame, int failCount,
				eStyle style = STYLE_HATCHED )
	{
		m_Type       = type;
		m_Expression = exp;
		m_File       = file;
		m_Line       = line;
		m_Message    = msg;
		m_StartFrame = startFrame;
		m_FailCount  = failCount;
		m_Style      = style;
	}

	static void PrintF( const char* format, ... );

	void FormatAssertion ( char* buffer, int maxLen, bool convertNewlines ) const;
	bool Execute         ( int* ignore = NULL, bool errorReport = true, bool incErrorCount = true );
	bool DoSimpleDialog  ( void );
	bool DoAdvancedDialog( bool& fatal, int* ignore = NULL );

	static bool CanDoAdvancedDialog( void );
	static bool IsAssertOccurring( void );
};

#define gpassert_raw_guts( exp, msg, type ) \
{ \
	static AssertInstance sInstance( __FILE__, __LINE__, true ); \
	++AssertData::ms_FailedCount; \
	DWORD startFrame; \
	_asm  {  mov [startFrame], ebp  } \
	if ( AssertData( type, #exp, __FILE__, __LINE__, msg, startFrame, ++sInstance.m_FailCount ).Execute( &sInstance.m_IgnoreCount ) ) \
	{ \
		_asm  {  int 3  } \
	} \
}

#define gpassert_formatted_guts( exp, msg, type ) \
{ \
	static AssertInstance sInstance( __FILE__, __LINE__, true ); \
	++AssertData::ms_FailedCount; \
	AssertData::PrintF msg; \
	DWORD startFrame; \
	_asm  {  mov [startFrame], ebp  } \
	if ( AssertData( type, #exp, __FILE__, __LINE__, AssertData::ms_Buffer, startFrame, ++sInstance.m_FailCount ).Execute( &sInstance.m_IgnoreCount ) ) \
	{ \
		_asm  {  int 3  } \
	} \
}

#define gpassert_raw( exp, msg, type ) \
{ \
	bool bad = !(exp); \
	if ( bad ) \
	{ \
		gpassert_raw_guts( exp, msg, type ); \
	} \
}

#define gpassert_formatted( exp, msg, type ) \
{ \
	bool bad = !(exp); \
	if ( bad ) \
	{ \
		gpassert_formatted_guts( exp, msg, type ); \
	} \
}

#define if_gpassert_raw( exp, msg, type ) \
	AssertData::ms_TempBad = !(exp); \
	if ( AssertData::ms_TempBad ) \
	{ \
		gpassert_raw_guts( exp, msg, type ); \
	} \
	if ( !AssertData::ms_TempBad )

#define if_gpassert_formatted( exp, msg, type ) \
	AssertData::ms_TempBad = !(exp); \
	if ( AssertData::ms_TempBad ) \
	{ \
		gpassert_formatted_guts( exp, msg, type ); \
	} \
	if ( !AssertData::ms_TempBad )

#define if_not_gpassert_raw( exp, msg, type ) \
	AssertData::ms_TempBad = !(exp); \
	if ( AssertData::ms_TempBad ) \
	{ \
		gpassert_raw_guts( exp, msg, type ); \
	} \
	if ( AssertData::ms_TempBad )

#define if_not_gpassert_formatted( exp, msg, type ) \
	AssertData::ms_TempBad = !(exp); \
	if ( AssertData::ms_TempBad ) \
	{ \
		gpassert_formatted_guts( exp, msg, type ); \
	} \
	if ( AssertData::ms_TempBad )

// use this to explicitly call up the assertion box - set "localStorage" to
// true if the 'file' param may go out of scope (if attached to something other
// than __FILE__ for example).
bool gpassert_box( const char*        exp,
				   const char*        msg,
				   const char*        file,
				   int                line,
				   const char*        type,
				   bool               localStorage  = false,
				   bool               errorReport   = true,
				   bool               incErrorCount = true,
				   DWORD              startFrame    = 0,
				   bool               dbgBreak      = true,
				   AssertData::eStyle style         = AssertData::STYLE_DEFAULT );

//////////////////////////////////////////////////////////////////////////////

#endif  // NOGPCORE
#endif  // __GPASSERT_H

//////////////////////////////////////////////////////////////////////////////
