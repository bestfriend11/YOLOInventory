//////////////////////////////////////////////////////////////////////////////
//
// File     :  GpReport.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a set of macros and functions that interface to the
//             ReportSys for common report types.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GPREPORT_H
#define __GPREPORT_H

//////////////////////////////////////////////////////////////////////////////

#include "GpCore.h"

namespace ReportSys  {  // begin of namespace ReportSys

//////////////////////////////////////////////////////////////////////////////
// forward declarations

class Context;
class Sink;

//////////////////////////////////////////////////////////////////////////////
// documentation

/*
	Printf-style formatting:

		for all macros in this file: to use the "f" versions, enclose the
		expression in double-parens, like this:

		gpwarningf(( "Printing out an error '%d'\n", error_code ));

	Localization:

		many messages may be destined for the end user to see and will require
		eventual localization. to internationalize the message, use the $MSG$
		tag right before the quoted string in the message, like this:

		gpwarningf(( $MSG$ "Out of hard drive space in '%s'\n", path ));
		gpreportf(( gMyContext, $MSG$ "Out of hard drive space in '%s'\n", path ));

		here's how it works. the reporting functions just detect a leading $MSG$
		tag in the text and will do a translation on success. this is done
		through a string-string map created at app startup time depending on the
		language chosen.

		building the map is done by grepping through the source tree and
		searching for all $MSG$ tags, then storing the next string it finds in a
		file which is the set of keys to be translated into values by the
		localization folks.

	$$$ put doco in here on which macros are for what, how to use them,
		naming conventions, etc. - also have how to use REPORTX_XX_MACRO stuff.
*/

//////////////////////////////////////////////////////////////////////////////
// reporting

// Primary sinks.

	Sink& GetGlobalSink  ( void );
	Sink& GetDebuggerSink( void );

#	define gGlobalSink   ReportSys::GetGlobalSink()					// all contexts first route through this for everything (holds gDebuggerSink by default)
#	define gDebuggerSink ReportSys::GetDebuggerSink()				// OutputDebugString sink

// Informational contexts.

	Context& GetGenericContext   ( void );
	Context& GetPerfContext      ( void );
	Context& GetPerfLogContext   ( void );
	Context& GetTestLogContext   ( void );
	Context& GetMessageBoxContext( void );
	Context& GetErrorBoxContext  ( void );
	Context& GetScreenContext    ( void );
	Context& GetDebuggerContext  ( void );
	Context& GetNetContext		 ( void );
	Context& GetNetTextContext	 ( void );

#	define gGenericContext    (ReportSys::GetGenericContext   ())	// send it here if you don't know where else to send it
#	define gPerfContext       (ReportSys::GetPerfContext      ())	// these are meant for performance-related errors that do not affect the stability of the game
#	define gPerfLogContext    (ReportSys::GetPerfLogContext   ())	// these are meant for general-purpose performance logging
#	define gTestLogContext    (ReportSys::GetTestLogContext   ())	// meant for general-purpose test logging
#	define gMessageBoxContext (ReportSys::GetMessageBoxContext())	// reports sent here will bring up a dialog
#	define gErrorBoxContext   (ReportSys::GetErrorBoxContext  ())	// reports sent here will bring up an error dialog
#	define gScreenContext     (ReportSys::GetScreenContext    ())	// use this for messages meant for the end user
#	define gDebuggerContext   (ReportSys::GetDebuggerContext  ())	// this will go out to the debugger
#	define gNetContext		  (ReportSys::GetNetContext		  ())	// this is for the net log routed to gNetLog
#	define gNetTextContext    (ReportSys::GetNetTextContext	  ())	// this is for the net log text version (named "net", hung off NetContext)
	
// Error contexts.

	Context& GetWarningContext( void );
	Context& GetErrorContext  ( void );
	Context& GetFatalContext  ( void );

#	define gWarningContext (ReportSys::GetWarningContext())			// handled warnings
#	define gErrorContext   (ReportSys::GetErrorContext  ())			// will assert in all builds - use for unstable pseudo-handled errors
#	define gFatalContext   (ReportSys::GetFatalContext  ())			// will shut down the app

// Macro helpers.

#	define MACRO_REPORT( context, msg )											\
	{																			\
		if ( ReportSys::IsContextEnabled( context ) )							\
		{																		\
			DWORD startFrame;													\
			_asm  {  mov [startFrame], ebp  }									\
			ReportSys::EnterReportMacro( startFrame );							\
			ReportSys::SetNextLocation( __FILE__, __LINE__ );					\
			ReportSys::OutputMacro( context, msg );								\
			if ( ReportSys::LeaveReportMacro() )								\
			{																	\
				_asm  {  int 3  }												\
			}																	\
		}																		\
	}
#	define MACRO_REPORTF( context, msg )										\
	{																			\
		if ( ReportSys::IsContextEnabled( context ) )							\
		{																		\
			DWORD startFrame;													\
			_asm  {  mov [startFrame], ebp  }									\
			ReportSys::EnterReportMacro( startFrame );							\
			ReportSys::SetNextLocation( __FILE__, __LINE__ );					\
			ReportSys::SetNextContext( context );								\
			ReportSys::OutputNextF msg;											\
			if ( ReportSys::LeaveReportMacro() )								\
			{																	\
				_asm  {  int 3  }												\
			}																	\
		}																		\
	}

#	define DEV_MACRO_REPORT( context, msg )		GPDEV_ONLY( MACRO_REPORT( context, msg ) )
#	define DEV_MACRO_REPORTF( context, msg )	GPDEV_ONLY( MACRO_REPORTF( context, msg ) )

// Report macros.

	// special report macro: first param must be the context
#	define gpreport( context, msg )		MACRO_REPORT( context, msg )
#	define gpreportf( context, msg )	MACRO_REPORTF( context, msg )

	// special report macro: first param must be the context (dev-only - compiles out in retail)
#	define gpdevreport( context, msg )	DEV_MACRO_REPORT( context, msg )
#	define gpdevreportf( context, msg )	DEV_MACRO_REPORTF( context, msg )

	// generic output messages (compiles out in retail)
#	define gpgeneric( msg )				DEV_MACRO_REPORT( &gGenericContext, msg )
#	define gpgenericf( msg )			DEV_MACRO_REPORTF( &gGenericContext, msg )

	// performance related messages (compiles out in retail)
#	define gpperf( msg )				DEV_MACRO_REPORT( &gPerfContext, msg )
#	define gpperff( msg )				DEV_MACRO_REPORTF( &gPerfContext, msg )

	// performance related logging (compiles out in retail)
#	define gpperflog( msg )				DEV_MACRO_REPORT( &gPerfLogContext, msg )
#	define gpperflogf( msg )			DEV_MACRO_REPORTF( &gPerfLogContext, msg )

	// test related logging (compiles out in retail)
#	define gptestlog( msg )				DEV_MACRO_REPORT( &gTestLogContext, msg )
#	define gptestlogf( msg )			DEV_MACRO_REPORTF( &gTestLogContext, msg )

	// output for a message box
#	define gpmessagebox( msg )			MACRO_REPORT( &gMessageBoxContext, msg )
#	define gpmessageboxf( msg )			MACRO_REPORTF( &gMessageBoxContext, msg )

	// output for a message box
#	define gperrorbox( msg )			MACRO_REPORT( &gErrorBoxContext, msg )
#	define gperrorboxf( msg )			MACRO_REPORTF( &gErrorBoxContext, msg )

	// output to end user screen (probably scrolling console in retail build)
#	define gpscreen( msg )				MACRO_REPORT( &gScreenContext, msg )
#	define gpscreenf( msg )				MACRO_REPORTF( &gScreenContext, msg )

	// output to debugger (compiles out in retail)
#	define gpdebugger( msg )			DEV_MACRO_REPORT( &gDebuggerContext, msg )
#	define gpdebuggerf( msg )			DEV_MACRO_REPORTF( &gDebuggerContext, msg )

	// network related logging (compiles out in retail)
#	define gpnet( msg )					MACRO_REPORT( &gNetContext, msg )
#	define gpnetf( msg )				MACRO_REPORTF( &gNetContext, msg )
#	define gpnettext( msg )				MACRO_REPORT( &gNetTextContext, msg )
#	define gpnettextf( msg )			MACRO_REPORTF( &gNetTextContext, msg )

	// warnings
#	define gpwarning( msg )				DEV_MACRO_REPORT( &gWarningContext, msg )
#	define gpwarningf( msg )			DEV_MACRO_REPORTF( &gWarningContext, msg )

	// errors
#	define gperror( msg )				DEV_MACRO_REPORT( &gErrorContext, msg )
#	define gperrorf( msg )				DEV_MACRO_REPORTF( &gErrorContext, msg )

	// fatals
#	define gpfatal( msg )				ReportSys::EnableFatalAsWatson( false );  MACRO_REPORT( &gFatalContext, msg )
#	define gpfatalf( msg )				ReportSys::EnableFatalAsWatson( false );  MACRO_REPORTF( &gFatalContext, msg )
#	define gpwatson( msg )				ReportSys::EnableFatalAsWatson( true  );  MACRO_REPORT( &gFatalContext, msg )
#	define gpwatsonf( msg )				ReportSys::EnableFatalAsWatson( true  );  MACRO_REPORTF( &gFatalContext, msg )

	// warn in release, assert in debug
#	if GP_DEBUG
#	define gpwarnassert( msg )			gpassert( 0, msg )
#	define gpwarnassertf( msg )			gpassertf( 0, msg )
#	define gpassertwarn( exp, msg )		gpassert( exp, msg )
#	define gpassertwarnf( exp, msg )	gpassertf( exp, msg )
#	else
#	define gpwarnassert( msg )			gpwarning( msg )
#	define gpwarnassertf( msg )			gpwarningf( msg )
#	define gpassertwarn( exp, msg )		if ( !(exp) )  {  gpwarning( msg );  }
#	define gpassertwarnf( exp, msg )	if ( !(exp) )  {  gpwarningf( msg );  }
#	endif

// Translation macros.

#	define gptranslate( msg )			ReportSys::TranslateMsgA( msg )
#	define gpwtranslate( msg )			ReportSys::TranslateMsgW( msg )

// Registration macros.

	// sink factory
#	define DECLARE_SINK_FACTORY( TYPE, PROC ) \
		struct SinkFactoryRegistrar_##TYPE  { \
			ReportSys::SinkFactoryRegistrar m_Registrar; \
			SinkFactoryRegistrar_##TYPE( void ) : m_Registrar( #TYPE, PROC )  {  }  }; \
		template struct ::GlobalStatic <SinkFactoryRegistrar_##TYPE>

//////////////////////////////////////////////////////////////////////////////
// ReportSys API declaration

// Helper structs.

	struct ContextRef
	{
		Context* m_Context;

		ContextRef( void )							{  m_Context = &gGenericContext;  }
		ContextRef( int )							{  m_Context = &gGenericContext;  }
		ContextRef( Context& context )				{  m_Context = &context;  }
		ContextRef( Context* context )				{  m_Context = context;  }
		ContextRef( const char* contextName );

		operator Context*    ( void ) const			{  return ( m_Context );  }
		Context& operator *  ( void ) const			{  gpassert( m_Context != NULL );  return ( *m_Context );  }
		Context* operator -> ( void ) const			{  return ( m_Context );  }
	};

	struct AutoTable
	{
		Context* m_Context;

		AutoTable( ContextRef context );
	   ~AutoTable( void );
	};

	struct AutoReport
	{
		Context* m_Context;

		AutoReport( ContextRef context );
	   ~AutoReport( void );
	};

	struct AutoIndent
	{
		Context* m_Context;
		int      m_Count;

		AutoIndent( ContextRef context, bool autoIndent = true );
	   ~AutoIndent( void );

		void Indent( void );
	};

// Reporting.

	// $ these are wrappers to minimize #includes of ReportSys.h.

	// general output
	bool OutputRaw ( ContextRef context, const char* msg, int len = -1 );
	bool Output    ( ContextRef context, const char* msg, int len = -1 );
	bool Output    ( ContextRef context, const gpstring& msg );
	bool OutputF   ( ContextRef context, const char* fmt, ... );
	bool OutputEol ( ContextRef context );
	bool OutputArgs( ContextRef context, const char* fmt, va_list args );

	// special macro support - do not call these directly!
	void  SetNextLocation( const char* file, int line );						// (internal) next report macro location
	void  SetNextContext( ContextRef context );									// (internal) next report macro context
	void  OutputMacro( ContextRef context, const char* msg );					// (internal) ordinary output to next context
	void  OutputNextF( const char* fmt, ... );									// (internal) formatted output to next context
	void  EnterReportMacro( DWORD frame );										// (internal) entering a report macro (give frame)
	bool  LeaveReportMacro( void );												// (internal) leaving a report macro (returns true if should dbg break)
	DWORD GetReportMacroFrame( void );											// (external) used for assert trace
	void  SetNextReportDbgBreak( void );										// (external) sets current report macro to dbg break on finish

	// properties
	void EnableContext   ( ContextRef context, bool enable = true );
	void ToggleContext   ( ContextRef context );
	bool IsContextEnabled( ContextRef context );
	int  GetReportCount  ( ContextRef context );
	void ResetReportCount( ContextRef context );
	void BeginReport     ( ContextRef context );
	void EndReport       ( ContextRef context );
	void Indent          ( ContextRef context );
	void Outdent         ( ContextRef context );

	// special error support
	void IncWarningCount        ( void );
	void IncErrorCount          ( void );
	int  GetWarningCount        ( void );
	int  GetErrorCount          ( void );
	int  GetTotalWarningCount   ( void );
	int  GetTotalErrorCount     ( void );
	void ResetWarningCount      ( void );
	void ResetErrorCount        ( void );
	void EnableWarningContexts  ( bool enable = true );
	void EnableErrorContexts    ( bool enable = true );
	void ToggleWarningContexts  ( void );
	void ToggleErrorContexts    ( void );
	bool AreWarningsEnabled     ( void );
	bool AreErrorsEnabled       ( void );
	void ReportErrorSummary     ( ContextRef context = NULL );
	void EnableErrorAssert      ( bool enable = true );
	bool IsErrorAssertEnabled   ( void );
	void EnablePerfAssert       ( bool enable = true );
	bool IsPerfAssertEnabled    ( void );
	void EnableFatalException   ( bool enable = true );
	bool IsFatalExceptionEnabled( void );
	void EnableFatalAsWatson    ( bool enable = true );
	bool IsFatalAsWatsonEnabled ( void );

// Message box.

	// safe message box - hook this!
	int SafeMessageBox( HWND parent, const char* msg, const char* title, UINT style = MB_OK );
	int SafeMessageBox( HWND parent, const wchar_t* msg, const wchar_t* title, UINT style = MB_OK );
	int SafeMessageBox( const char* msg, const char* title, UINT style = MB_OK );
	int SafeMessageBox( const wchar_t* msg, const wchar_t* title, UINT style = MB_OK );

	// hooks for above
	typedef void (*MessageBoxHook)( bool forError );
	void RegisterPreMessageBoxHook ( MessageBoxHook hook );
	void RegisterPostMessageBoxHook( MessageBoxHook hook );
	void UnregisterMessageBoxHook  ( MessageBoxHook hook );
	void CallSafeMessageBoxPre     ( bool forError );
	void CallSafeMessageBoxPost    ( bool forError );

	// auto safety
	struct AutoSafeMessageBox
	{
		bool m_ForError;

		AutoSafeMessageBox( bool forError )  {  m_ForError = forError;  CallSafeMessageBoxPre ( m_ForError );  }
	   ~AutoSafeMessageBox( void )           {  CallSafeMessageBoxPost( m_ForError );  }
	};

// Special message box (c) 2000 Mike Biddlecombe.

	class HeyBatterBox
	{
	public:
		SET_NO_INHERITED( HeyBatterBox );

		HeyBatterBox( HWND parent, const char* msg, UINT style = MB_YESNO );
		HeyBatterBox( const char* msg, UINT style = MB_YESNO );
	   ~HeyBatterBox( void );

		// returns true if user wants to try again, false if they cancelled
		bool TryAgain( void );

	private:
		void Init( HWND parent, const char* msg, UINT style );

		// stats
		int m_Strikes;
		int m_Outs;
		int m_Innings;
		int m_Games;

		// spec
		HWND      m_Parent;
		gpstring* m_Message;		// fwd-declared to avoid gpstring.h include
		UINT      m_Style;

		SET_NO_COPYING( HeyBatterBox );
	};

// Fatals.

	struct xfatal
	{
		const char* m_Message;

		xfatal( const char* msg )
			: m_Message( msg )  {  }
	};

	// returns true if we're in the middle of a fatal exception
	bool IsFatalOccurring( void );

	// sets the fatal to occur - returns true if we were already in one
	bool SetFatalIsOccurring( void );

	// for dumping the stack
	void DumpStack( ContextRef context = NULL );

// Startup registration structs.

	template <typename T>
	struct Registrar : GlobalRegistrar <Registrar <T> >
	{
		const char* m_Name;
		T           m_Data;

		Registrar( const char* name, T data )
		{
			m_Name = name;
			m_Data = data;
		}
	};

// Registration instantiations.

	// sink factory proc type
	typedef Sink* (*SinkFactoryProc)( const char* name, const char* type, const char* params );

	// registrar for sink factory
	typedef Registrar <SinkFactoryProc> SinkFactoryRegistrar;

// Localization.

	// the prefix for localizable strings
#	define $MSG$ "$MSG$"

	// returns true and advances past $MSG$ tag if any
	inline bool ShouldLocalizeCheck( const char* seaText )
	{
		gpassert( seaText != NULL );
		if (   (seaText[ 0 ] == '$')
			&& (seaText[ 1 ] == 'M')
			&& (seaText[ 2 ] == 'S')
			&& (seaText[ 3 ] == 'G')
			&& (seaText[ 4 ] == '$') )
		{
			return ( true );
		}
		return ( false );
	}

	// returns true and advances past $MSG$ tag if any
	inline bool ShouldLocalize( const char*& seaText )
	{
		if ( ShouldLocalizeCheck( seaText ) )
		{
			seaText += ELEMENT_COUNT( $MSG$ ) - 1;
			return ( true );
		}
		return ( false );
	}

	// translates a MSG-prefixed string from seattlese to the current locale
	const char* TranslateMsgA( const char* seaText );
	gpwstring   TranslateMsgW( const char* seaText );
	void        TranslateMsg ( gpstring& out, const char* seaText );
	void        TranslateMsg ( gpwstring& out, const char* seaText );

	// translates a non-MSG-prefixed string from seattlese to the current locale
	const char* TranslateA   ( const char* seaText );
	gpwstring   TranslateW   ( const char* seaText );
	gpwstring   TranslateW   ( const wchar_t* seaText );
	void        Translate    ( gpstring& out, const char* seaText );
	void        Translate    ( gpwstring& out, const char* seaText );
	void        Translate    ( gpwstring& out, const wchar_t* seaText );

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace ReportSys

#endif  // __GPREPORT_H

//////////////////////////////////////////////////////////////////////////////
