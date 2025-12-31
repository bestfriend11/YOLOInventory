//////////////////////////////////////////////////////////////////////////////
//
// File     :  GpReport.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "GpReport.h"

#include "AppModule.h"
#include "DebugHelp.h"
#include "KernelTool.h"
#include "ReportSys.h"
#include "StdHelp.h"
#include "WinX.h"

#include <list>

namespace ReportSys  {  // begin of namespace ReportSys

//////////////////////////////////////////////////////////////////////////////
// primary sink implementations

static Sink* gGlobalSinkPtr = NULL;
static Sink* gDebuggerSinkPtr = NULL;

Sink& GetGlobalSink( void )
{
	if ( gGlobalSinkPtr == NULL )
	{
		static MultiSink sGlobalSink;
		sGlobalSink.SetName( "Global" );
		gGlobalSinkPtr = &sGlobalSink;

#		if ( !GP_RETAIL )
		{
			sGlobalSink.AddSink( &gDebuggerSink, false );
		}
#		endif
	}

	return ( *gGlobalSinkPtr );
}

Sink& GetDebuggerSink( void )
{
	if ( gDebuggerSinkPtr == NULL )
	{
		static DebuggerSink sDebuggerSink( "Debugger" );
		gDebuggerSinkPtr = &sDebuggerSink;
	}

	return ( *gDebuggerSinkPtr );
}

//////////////////////////////////////////////////////////////////////////////
// informational context implementations

static Context* gGenericContextPtr    = NULL;
static Context* gPerfContextPtr       = NULL;
static Context* gPerfLogContextPtr    = NULL;
static Context* gTestLogContextPtr    = NULL;
static Context* gMessageBoxContextPtr = NULL;
static Context* gErrorBoxContextPtr   = NULL;
static Context* gScreenContextPtr     = NULL;
static Context* gDebuggerContextPtr   = NULL;
static Context* gNetContextPtr        = NULL;
static Context* gNetTextContextPtr    = NULL;


Context& GetGenericContext( void )
{
	if ( gGenericContextPtr == NULL )
	{
		static Context sGenericContext( "Generic" );
		gGenericContextPtr = &sGenericContext;

		sGenericContext.SetType( "Log" );
	}
	return ( *gGenericContextPtr );
}

static Sink* gPerfAssertSinkPtr = NULL;

Context& GetPerfContext( void )
{
	if ( gPerfContextPtr == NULL )
	{
		static Context sPerfContext( "Perf" );
		gPerfContextPtr = &sPerfContext;

		sPerfContext.SetOptions( Context::OPTION_AUTOREPORT );
		sPerfContext.SetType   ( "Log" );
		EnablePerfAssert();
	}
	return ( *gPerfContextPtr );
}

Context& GetPerfLogContext( void )
{
	if ( gPerfLogContextPtr == NULL )
	{
		static Context sPerfLogContext( "PerfLog" );
		gPerfLogContextPtr = &sPerfLogContext;

		sPerfLogContext.SetType( "Log" );
	}
	return ( *gPerfLogContextPtr );
}

Context& GetTestLogContext( void )
{
	if ( gTestLogContextPtr == NULL )
	{
		static Context sTestLogContext( "TestLog" );
		gTestLogContextPtr = &sTestLogContext;

		sTestLogContext.SetType( "Log" );
	}
	return ( *gTestLogContextPtr );
}

void EnablePerfAssert( bool enable )
{
	if ( IsPerfAssertEnabled() != enable )
	{
		if ( enable )
		{
			gPerfAssertSinkPtr = new CallbackSink( CallbackSink::MakeAssertCallback(), "Perf", "Perf" );
			gPerfContext.AddSink( gPerfAssertSinkPtr, true );
		}
		else
		{
			gPerfContext.RemoveSink( gPerfAssertSinkPtr );
			gPerfAssertSinkPtr = NULL;
		}
	}
}

bool IsPerfAssertEnabled( void )
{
	return ( gPerfAssertSinkPtr != NULL );
}

Context& GetMessageBoxContext( void )
{
	if ( gMessageBoxContextPtr == NULL )
	{
		static Context sMessageBoxContext( "MessageBox" );
		gMessageBoxContextPtr = &sMessageBoxContext;

		sMessageBoxContext.AddSink   ( new CallbackSink( CallbackSink::MakeMessageBoxCallback(), "MessageBox", "MessageBox" ), true );
		sMessageBoxContext.SetOptions( Context::OPTION_AUTOREPORT | Context::OPTION_LOCALIZE );
		sMessageBoxContext.SetType   ( "MessageBox" );
	}
	return ( *gMessageBoxContextPtr );
}

Context& GetErrorBoxContext( void )
{
	if ( gErrorBoxContextPtr == NULL )
	{
		static Context sErrorBoxContext( "ErrorBox" );
		gErrorBoxContextPtr = &sErrorBoxContext;

		sErrorBoxContext.AddSink   ( gErrorContext.GetSink(), false );
		sErrorBoxContext.AddSink   ( new CallbackSink( CallbackSink::MakeErrorBoxCallback(), "ErrorBox", "ErrorBox" ), true );
		sErrorBoxContext.SetOptions( Context::OPTION_AUTOREPORT );
		sErrorBoxContext.SetTraits ( Context::TRAIT_ERROR );
		sErrorBoxContext.SetColor  ( gpcolor( 255, 0, 0 ) );
		sErrorBoxContext.SetType   ( "MessageBox" );
	}
	return ( *gErrorBoxContextPtr );
}

Context& GetScreenContext( void )
{
	if ( gScreenContextPtr == NULL )
	{
		static Context sScreenContext( "Screen" );
		gScreenContextPtr = &sScreenContext;

		sScreenContext.SetOptions( Context::OPTION_AUTOREPORT | Context::OPTION_LOCALIZE );
		sScreenContext.SetType   ( "Log" );
	}
	return ( *gScreenContextPtr );
}

Context& GetDebuggerContext( void )
{
	if ( gDebuggerContextPtr == NULL )
	{
		static LocalContext sDebuggerContext( "Debugger" );			// just out to debugger
		gDebuggerContextPtr = &sDebuggerContext;

		sDebuggerContext.AddSink( &gDebuggerSink, false );
		sDebuggerContext.SetType( "Log" );
	}
	return ( *gDebuggerContextPtr );
}

Context& GetNetContext( void )
{
	if ( gNetContextPtr == NULL )
	{
		static LocalContext sNetContext( "NetRaw" );				// straight to nowhere (for now)
		gNetContextPtr = &sNetContext;

		sNetContext.Enable( false );
	}
	return ( *gNetContextPtr );
}

Context& GetNetTextContext( void )
{
	if ( gNetTextContextPtr == NULL )
	{
		static Context sNetTextContext( &gGenericContext, "Net" );
		gNetTextContextPtr = &sNetTextContext;

		sNetTextContext.Enable( false );
	}
	return ( *gNetTextContextPtr );
}

//////////////////////////////////////////////////////////////////////////////
// error context implementations

static Context* gWarningContextPtr = NULL;
static Context* gErrorContextPtr   = NULL;
static Context* gFatalContextPtr   = NULL;

void ConstructContexts( void )
{
	gGenericContext;
	gPerfContext;
	gPerfLogContext;
	gTestLogContext;
	gMessageBoxContext;
	gErrorBoxContext;
	gScreenContext;
	gDebuggerContext;
	gNetContext;
	gNetTextContext;
	gWarningContext;
	gErrorContext;
	gFatalContext;
}

struct AutoInit
{
	AutoInit( void )
	{
		ConstructContexts();
	}
};

static AutoInit s_AutoInit;

Context& GetWarningContext( void )
{
	if ( gWarningContextPtr == NULL )
	{
		static Context sWarningContext( "Warning" );
		gWarningContextPtr = &sWarningContext;

		sWarningContext.SetOptions( Context::OPTION_AUTOREPORT );
		sWarningContext.SetTraits ( Context::TRAIT_WARNING );
		sWarningContext.SetColor  ( gpcolor( 255, 255, 0 ) );
		sWarningContext.SetType   ( "Log" );

		ConstructContexts();
	}
	return ( *gWarningContextPtr );
}

static Sink* gErrorAssertSinkPtr = NULL;

Context& GetErrorContext( void )
{
	if ( gErrorContextPtr == NULL )
	{
		static Context sErrorContext( "Error" );
		gErrorContextPtr = &sErrorContext;

		sErrorContext.SetOptions( Context::OPTION_AUTOREPORT );
		sErrorContext.SetTraits ( Context::TRAIT_ERROR );
		sErrorContext.SetColor  ( gpcolor( 200, 0, 0 ) );
		sErrorContext.SetType   ( "Log" );
		EnableErrorAssert();

		ConstructContexts();
	}
	return ( *gErrorContextPtr );
}

void EnableErrorAssert( bool enable )
{
	if ( IsErrorAssertEnabled() != enable )
	{
		if ( enable )
		{
			gErrorAssertSinkPtr = new CallbackSink( CallbackSink::MakeAssertCallback(), "Assert", "Assert" );
			gErrorContext.AddSink( gErrorAssertSinkPtr, true );
		}
		else
		{
			gErrorContext.RemoveSink( gErrorAssertSinkPtr );
			gErrorAssertSinkPtr = NULL;
		}
	}
}

bool IsErrorAssertEnabled( void )
{
	return ( gErrorAssertSinkPtr != NULL );
}

static Sink* gFatalSinkPtr = NULL;

Context& GetFatalContext( void )
{
	if ( gFatalContextPtr == NULL )
	{
		static LocalContext sFatalContext( "Fatal" );		// fatals cannot be turned off globally
		gFatalContextPtr = &sFatalContext;

		sFatalContext.AddSink   ( &gGlobalSink, false );
		sFatalContext.SetOptions( Context::OPTION_AUTOREPORT | Context::OPTION_LOCALIZE );
		sFatalContext.SetTraits ( Context::TRAIT_ERROR );
		sFatalContext.SetColor  ( gpcolor( 255, 0, 0 ) );
		sFatalContext.SetType   ( "Log" );
		EnableFatalException();

		ConstructContexts();
	}
	return ( *gFatalContextPtr );
}

void EnableFatalException( bool enable )
{
	if ( IsFatalExceptionEnabled() != enable )
	{
		if ( enable )
		{
			gFatalSinkPtr = new CallbackSink( CallbackSink::MakeFatalCallback(), "Fatal", "Fatal" );
			gFatalContext.AddSink( gFatalSinkPtr, true );
		}
		else
		{
			gFatalContext.RemoveSink( gFatalSinkPtr );
			gFatalSinkPtr = NULL;
		}
	}
}

bool IsFatalExceptionEnabled( void )
{
	return ( gFatalSinkPtr != NULL );
}

static bool s_FatalAsWatson = false;

void EnableFatalAsWatson( bool enable )
{
	s_FatalAsWatson = enable;
}

bool IsFatalAsWatsonEnabled( void )
{
	return ( s_FatalAsWatson );
}

//////////////////////////////////////////////////////////////////////////////
// helper struct implementations

ContextRef :: ContextRef( const char* contextName )
{
	m_Context = gReportSysMgr.FindContext( contextName );
}

AutoTable :: AutoTable( ContextRef context )
	: m_Context( context )
{
	m_Context->BeginTable();
}

AutoTable :: ~AutoTable( void )
{
	m_Context->EndTable();
}

AutoReport :: AutoReport( ContextRef context )
	: m_Context( context )
{
	m_Context->BeginReport();
}

AutoReport :: ~AutoReport( void )
{
	m_Context->EndReport();
}

AutoIndent :: AutoIndent( ContextRef context, bool autoIndent )
	: m_Context( context )
{
	m_Count = 0;
	if ( autoIndent )
	{
		Indent();
	}
}

AutoIndent :: ~AutoIndent( void )
{
	while ( m_Count > 0 )
	{
		m_Context->Outdent();
		--m_Count;
	}
}

void AutoIndent :: Indent( void )
{
	++m_Count;
	m_Context->Indent();
}

//////////////////////////////////////////////////////////////////////////////
// reporting function implementations

typedef std::vector <DWORD> DwordColl;

DECL_THREAD const char* gNextFile             = NULL;
DECL_THREAD int         gNextLine             = 0;
DECL_THREAD Context*    gNextContext          = NULL;
DECL_THREAD bool        gNextReportDbgBreak   = false;
DECL_THREAD int         gReportMacroLevel     = 0;
DECL_THREAD DWORD       gReportMacroFrames[ 100 ];
DECL_THREAD DWORD       gReportMacroFramesTop = 0;

bool OutputRaw( ContextRef context, const char* msg, int len )
{
	if ( context )
	{
		if ( context->IsEnabled() )
		{
			context->SetLocation( gNextFile, gNextLine );
			bool rc = context->OutputRaw( msg, len );
			context->ResetLocation();
			gNextFile = NULL;
			gNextLine = 0;
			return ( rc );
		}
		else
		{
			return ( true );
		}
	}
	else
	{
		return ( false );
	}
}

bool Output( ContextRef context, const char* msg, int len )
{
	if ( context )
	{
		if ( context->IsEnabled() )
		{
			context->SetLocation( gNextFile, gNextLine );
			bool rc = context->Output( msg, len );
			context->ResetLocation();
			gNextFile = NULL;
			gNextLine = 0;
			return ( rc );
		}
		else
		{
			return ( true );
		}
	}
	else
	{
		return ( false );
	}
}

bool Output( ContextRef context, const gpstring& msg )
{
	return ( Output( context, msg, msg.length() ) );
}

bool OutputF( ContextRef context, const char* fmt, ... )
{
	return ( OutputArgs( context, fmt, va_args( fmt ) ) );
}

bool OutputEol( ContextRef context )
{
	return ( context->OutputEol() );
}

bool OutputArgs( ContextRef context, const char* fmt, va_list args )
{
	if ( context )
	{
		if ( context->IsEnabled() )
		{
			context->SetLocation( gNextFile, gNextLine );
			bool rc = context->OutputArgs( fmt, args );
			context->ResetLocation();
			gNextFile = NULL;
			gNextLine = 0;
			return ( rc );
		}
		else
		{
			return ( true );
		}
	}
	else
	{
		return ( false );
	}
}

void SetNextLocation( const char* file, int line )
{
	gNextFile = file;
	gNextLine = line;
}

void SetNextContext( ContextRef context )
{
	gNextContext = context;
}

void OutputMacro( ContextRef context, const char* msg )
{
	if ( context->IsEnabled() )
	{
		Output( context, msg );
	}
}

void OutputNextF( const char* fmt, ... )
{
	if ( gNextContext == NULL )
	{
		gNextContext = &gGenericContext;
	}

	if ( gNextContext->IsEnabled() )
	{
		OutputArgs( gNextContext, fmt, va_args( fmt ) );
	}

	gNextContext = NULL;
}

void EnterReportMacro( DWORD frame )
{
	gReportMacroFrames[ gReportMacroFramesTop++ ] = frame;
	gpassert( gReportMacroFramesTop != ELEMENT_COUNT( gReportMacroFrames ) );
}

bool LeaveReportMacro( void )
{
	--gReportMacroFramesTop;
	gpassert( gReportMacroFramesTop >= 0 );

	bool dbgBreak = gNextReportDbgBreak;
	gNextReportDbgBreak = false;
	return ( dbgBreak );
}

DWORD GetReportMacroFrame( void )
{
	return ( (gReportMacroFramesTop == 0) ? 0 : gReportMacroFrames[ gReportMacroFramesTop - 1 ] );
}

void SetNextReportDbgBreak( void )
{
	gNextReportDbgBreak = true;
}

void EnableContext( ContextRef context, bool enable )
{
	if ( context )
	{
		context->Enable( enable );
	}
}

void ToggleContext( ContextRef context )
{
	if ( context )
	{
		context->Enable( !context->IsEnabled() );
	}
}

bool IsContextEnabled( ContextRef context )
{
	return ( context ? context->IsEnabled() : false );
}

int GetReportCount( ContextRef context )
{
	return ( context ? context->GetReportCount() : 0 );
}

void ResetReportCount( ContextRef context )
{
	if ( context )
	{
		context->ResetReportCount();
	}
}

void BeginReport( ContextRef context )
{
	if ( context )
	{
		context->BeginReport();
	}
}

void EndReport( ContextRef context )
{
	if ( context )
	{
		context->EndReport();
	}
}

void Indent( ContextRef context )
{
	if ( context )
	{
		context->Indent();
	}
}

void Outdent( ContextRef context )
{
	if ( context )
	{
		context->Outdent();
	}
}

static int gWarningCount = 0;
static int gErrorCount = 0;
static int gTotalWarningCount = 0;
static int gTotalErrorCount = 0;
static bool gWarningsEnabled = true;
static bool gErrorsEnabled = true;

void IncWarningCount( void )
{
	++gWarningCount;
	++gTotalWarningCount;
}

void IncErrorCount( void )
{
	++gErrorCount;
	++gTotalErrorCount;
}

int GetWarningCount( void )
{
	return ( gWarningCount );
}

int GetErrorCount( void )
{
	return ( gErrorCount );
}

int GetTotalWarningCount( void )
{
	return ( gTotalWarningCount );
}

int GetTotalErrorCount( void )
{
	return ( gTotalErrorCount );
}

void ResetWarningCount( void )
{
	gWarningCount = 0;
}

void ResetErrorCount( void )
{
	gErrorCount = 0;
}

void EnableWarningContexts( bool enable )
{
	gWarningsEnabled = enable;

	Mgr::ContextColl contexts;
	gReportSysMgr.QueryContextByTraits( contexts, Context::TRAIT_WARNING );
	Mgr::ContextColl::iterator i, begin = contexts.begin(), end = contexts.end();
	for ( i = begin ; i != end ; ++i )
	{
		(*i)->Enable( enable );
	}
}

void EnableErrorContexts( bool enable )
{
	gErrorsEnabled = enable;

	Mgr::ContextColl contexts;
	gReportSysMgr.QueryContextByTraits( contexts, Context::TRAIT_ERROR );
	Mgr::ContextColl::iterator i, begin = contexts.begin(), end = contexts.end();
	for ( i = begin ; i != end ; ++i )
	{
		(*i)->Enable( enable );
	}
}

void ToggleWarningContexts( void )
{
	EnableWarningContexts( !AreWarningsEnabled() );
}

void ToggleErrorContexts( void )
{
	EnableErrorContexts( !AreErrorsEnabled() );
}

bool AreWarningsEnabled( void )
{
	return ( gWarningsEnabled );
}

bool AreErrorsEnabled( void )
{
	return ( gErrorsEnabled );
}

void ReportErrorSummary( ContextRef context )
{
	if ( (gTotalErrorCount > 0) || (gTotalWarningCount > 0) )
	{
		context->OutputF( "Errors detected: %d error%s, %d warning%s.\n",
						  gTotalErrorCount,   (gTotalErrorCount   == 1) ? "" : "s" ,
						  gTotalWarningCount, (gTotalWarningCount == 1) ? "" : "s" );
	}
}

//////////////////////////////////////////////////////////////////////////////
// message box implementations

struct HookColl : std::list <MessageBoxHook>
{
	HookColl*& m_Ptr;

	HookColl( HookColl*& ptr )
		: m_Ptr( ptr )  {  m_Ptr = this;  }
   ~HookColl( void )
		{  m_Ptr = NULL;  }
};

static HookColl* gMessageBoxPreHooksPtr = NULL;
static HookColl* gMessageBoxPostHooksPtr = NULL;

// pre accessor
HookColl& GetMessageBoxPreHooks( void )
{
	if ( gMessageBoxPreHooksPtr == NULL )
	{
		static HookColl sMessageBoxPreHooks( gMessageBoxPreHooksPtr );
	}
	return ( *gMessageBoxPreHooksPtr );
}

// post accessor
HookColl& GetMessageBoxPostHooks( void )
{
	if ( gMessageBoxPostHooksPtr == NULL )
	{
		static HookColl sMessageBoxPostHooks( gMessageBoxPostHooksPtr );
	}
	return ( *gMessageBoxPostHooksPtr );
}

int SafeMessageBox( HWND parent, const char* msg, const char* title, UINT style )
{
	AutoSafeMessageBox autoSafe( true );
	kerneltool::Thread::AutoPauser autoThreadPauser;

	if ( AppModule::DoesSingletonExist() && gAppModule.IsFullScreen() )
	{
		style |= MB_TOPMOST;
	}

	if ( parent == NULL )
	{
		parent = winx::GetFrontWindow();
	}

	return ( ::MessageBoxA( parent, msg, title, style | MB_TASKMODAL | MB_SETFOREGROUND ) );
}

int SafeMessageBox( HWND parent, const wchar_t* msg, const wchar_t* title, UINT style )
{
	AutoSafeMessageBox autoSafe( true );
	kerneltool::Thread::AutoPauser autoThreadPauser;

	if ( AppModule::DoesSingletonExist() && gAppModule.IsFullScreen() )
	{
		style |= MB_TOPMOST;
	}

	if ( parent == NULL )
	{
		parent = winx::GetFrontWindow();
	}

	return ( ::MessageBoxW( parent, msg, title, style | MB_TASKMODAL | MB_SETFOREGROUND ) );
}

int SafeMessageBox( const char* msg, const char* title, UINT style )
{
	return ( SafeMessageBox( NULL, msg, title, style ) );
}

int SafeMessageBox( const wchar_t* msg, const wchar_t* title, UINT style )
{
	return ( SafeMessageBox( NULL, msg, title, style ) );
}

void RegisterPreMessageBoxHook( MessageBoxHook hook )
{
	GetMessageBoxPreHooks().push_back( hook );
}

void RegisterPostMessageBoxHook( MessageBoxHook hook )
{
	GetMessageBoxPostHooks().push_back( hook );
}

void UnregisterMessageBoxHook( MessageBoxHook hook )
{
	HookColl::iterator found = stdx::find( GetMessageBoxPreHooks(), hook );
	if ( found != GetMessageBoxPreHooks().end() )
	{
		GetMessageBoxPreHooks().erase( found );
	}
	else
	{
		found = stdx::find( GetMessageBoxPostHooks(), hook );
		if ( found != GetMessageBoxPostHooks().end() )
		{
			GetMessageBoxPostHooks().erase( found );
		}
	}
}

static int gs_SafeMessageBoxRefs = 0;

void CallSafeMessageBoxPre( bool forError )
{
	if ( gs_SafeMessageBoxRefs++ == 0 )
	{
		if ( gMessageBoxPreHooksPtr != NULL )
		{
			HookColl::const_iterator i,
									 begin = GetMessageBoxPreHooks().begin(),
									 end   = GetMessageBoxPreHooks().end();
			for ( i = begin ; i != end ; ++i )
			{
				(*i)( forError );
			}
		}
	}
}

void CallSafeMessageBoxPost( bool forError )
{
	if ( --gs_SafeMessageBoxRefs == 0 )
	{
		if ( gMessageBoxPostHooksPtr != NULL )
		{
			HookColl::const_iterator i,
									 begin = GetMessageBoxPostHooks().begin(),
									 end   = GetMessageBoxPostHooks().end();
			for ( i = begin ; i != end ; ++i )
			{
				(*i)( forError );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// special message box implementations

HeyBatterBox :: HeyBatterBox( HWND parent, const char* msg, UINT style )
{
	Init( parent, msg, style );
}

HeyBatterBox :: HeyBatterBox( const char* msg, UINT style )
{
	Init( winx::GetFrontWindow(), msg, style );
}

HeyBatterBox :: ~HeyBatterBox( void )
{
	delete ( m_Message );
}

bool HeyBatterBox :: TryAgain( void )
{
	// update stats
	++m_Strikes;
	m_Outs    += m_Strikes / 3;
	m_Strikes %= 3;
	m_Innings += m_Outs    / 3;
	m_Outs    %= 3;
	m_Games   += m_Innings / 9;
	m_Innings %= 9;

	// format caption
	gpstring title;
	if ( m_Games > 0 )
	{
		title.assignf( "Game: %d, Inning: %d, Outs: %d, Strikes: %d", m_Games + 1, m_Innings + 1, m_Outs, m_Strikes );
	}
	else if ( m_Innings > 0 )
	{
		title.assignf( "Inning: %d, Outs: %d, Strikes: %d", m_Innings + 1, m_Outs, m_Strikes );
	}
	else if ( m_Outs > 0 )
	{
		title.assignf( "Outs: %d, Strikes: %d", m_Outs, m_Strikes );
	}
	else
	{
		title.assignf( "Strikes: %d", m_Strikes );
	}

	// do it
	int rc = SafeMessageBox( m_Parent, *m_Message, title, m_Style );
	switch ( m_Style )
	{
		case ( MB_CANCELTRYCONTINUE ):
		case ( MB_ABORTRETRYIGNORE ):
		case ( MB_RETRYCANCEL ):
		{
			return ( rc == IDRETRY );
		}	break;

		case ( MB_OKCANCEL ):
		{
			return ( rc == IDOK );
		}	break;

		case ( MB_YESNO ):
		case ( MB_YESNOCANCEL ):
		{
			return ( rc == IDYES );
		}	break;

		default:
		{
			gpassert( 0 );
		}
	}
	return ( true );
}

void HeyBatterBox :: Init( HWND parent, const char* msg, UINT style )
{
	// init stats
	m_Strikes = 0;
	m_Outs    = 0;
	m_Innings = 0;
	m_Games   = 0;

	// init spec
	m_Parent  = parent;
	m_Message = new gpstring;
	m_Style   = style;

	// build box text
	if ( (msg != NULL) && (*msg != '\0') )
	{
		*m_Message = msg;
		*m_Message += "\n\n";
	}
	*m_Message += "Take another swing?";
}

//////////////////////////////////////////////////////////////////////////////
// fatal implementation

void DumpStack( ContextRef context )
{
	gDbgSymbolEngine.StackTrace( context );
}

//////////////////////////////////////////////////////////////////////////////
// localization implementation

const char* TranslateMsgA( const char* seaText )
{
	gpverify( ShouldLocalize( seaText ) );
	if ( Mgr::DoesSingletonExist() )
	{
		return ( gReportSysMgr.TranslateA( seaText ) );
	}
	else
	{
		return ( seaText );
	}
}

gpwstring TranslateMsgW( const char* seaText )
{
	gpverify( ShouldLocalize( seaText ) );
	if ( Mgr::DoesSingletonExist() )
	{
		return ( gReportSysMgr.TranslateW( seaText ) );
	}
	else
	{
		return ( ::ToUnicode( seaText ) );
	}
}

void TranslateMsg( gpstring& out, const char* seaText )
{
	out = TranslateMsgA( seaText );
}

void TranslateMsg( gpwstring& out, const char* seaText )
{
	out = TranslateMsgW( seaText );
}

const char* TranslateA( const char* seaText )
{
	gpassert( !ShouldLocalize( seaText ) );
	if ( Mgr::DoesSingletonExist() )
	{
		return ( gReportSysMgr.TranslateA( seaText ) );
	}
	else
	{
		return ( seaText );
	}
}

gpwstring TranslateW( const char* seaText )
{
	gpassert( !ShouldLocalize( seaText ) );
	if ( Mgr::DoesSingletonExist() )
	{
		return ( gReportSysMgr.TranslateW( seaText ) );
	}
	else
	{
		return ( ::ToUnicode( seaText ) );
	}
}

gpwstring TranslateW( const wchar_t* seaText )
{
	gpassert( !ShouldLocalizeCheck( ::ToAnsi( seaText ) ) );
	if ( Mgr::DoesSingletonExist() )
	{
		return ( gReportSysMgr.TranslateW( seaText ) );
	}
	else
	{
		return ( seaText );
	}
}

void Translate( gpstring& out, const char* seaText )
{
	out = TranslateA( seaText );
}

void Translate( gpwstring& out, const char* seaText )
{
	out = TranslateW( seaText );
}

void Translate( gpwstring& out, const wchar_t* seaText )
{
	out = TranslateW( seaText );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace ReportSys

//////////////////////////////////////////////////////////////////////////////
