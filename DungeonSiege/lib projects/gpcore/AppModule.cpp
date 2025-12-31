//////////////////////////////////////////////////////////////////////////////
//
// File     :  AppModule.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "AppModule.h"

#include "Config.h"
#include "DebugHelp.h"
#include "DrWatson.h"
#include "FuBiPersist.h"
#include "GpMath.h"
#include "GpProfiler.h"
#include "GpStats.h"
#include "InputBinder.h"
#include "KernelTool.h"
#include "LocHelp.h"
#include "QMem.h"
#include "Quantify.h"
#include "ReportSys.h"
#include "StdHelp.h"
#include "StringTool.h"
#include "WinX.h"

#include <zmouse.h>
#include <map>

#if GP_SIEGEMAX
// Need a way to avoid calling the debug CRT when we are building
// a debug 3dsmax/gmax dll --biddle
#define _CrtSetDbgFlag( x )  {}
#endif

//////////////////////////////////////////////////////////////////////////////
// callbacks

// $ these are for overriding default gpreport behavior

static gpwstring MakeTitle( const wchar_t* extra )
{
	gpwstring title = gAppModule.MakeTitle();
	if ( !title.empty() )
	{
		title += L' ';
	}
	title += extra;
	return ( title );
}

static void MessageBoxCb( const ReportSys::Context* context, ReportSys::CallbackSink* sink )
{
	if ( AppModule::DoesSingletonExist() && gAppModule.HasMainWnd() )
	{
		ReportSys::SafeMessageBox( gAppModule.GetMainWnd(),
								   ::ToUnicode( sink->GetBuffer() ),
								   MakeTitle( gpwtranslate( $MSG$ "Message" ) ),
								   MB_ICONINFORMATION );
	}
	else
	{
		ReportSys::CallbackSink::MakeMessageBoxCallback()( context, sink );
	}
}

static void ErrorBoxCb( const ReportSys::Context* context, ReportSys::CallbackSink* sink )
{
	if ( AppModule::DoesSingletonExist() && gAppModule.HasMainWnd() )
	{
		ReportSys::SafeMessageBox( gAppModule.GetMainWnd(),
								   ::ToUnicode( sink->GetBuffer() ),
								   MakeTitle( gpwtranslate( $MSG$ "Critical Error" ) ),
								   MB_ICONERROR );
	}
	else
	{
		ReportSys::CallbackSink::MakeErrorBoxCallback()( context, sink );
	}
}

void PreMessageBoxCb( bool /*forError*/ )
{
	if ( !gAppModule.TestOptions( AppModule::OPTION_ALWAYS_MINIMALLY_ACTIVE ) )
	{
		gAppModule.FreezeTime();
	}
}

void PostMessageBoxCb( bool /*forError*/ )
{
	if ( !gAppModule.TestOptions( AppModule::OPTION_ALWAYS_MINIMALLY_ACTIVE ) )
	{
		gAppModule.ThawTime();
	}
}

//////////////////////////////////////////////////////////////////////////////
// nested AppModule types

AppModule::AutoPause :: AutoPause( void )
{
	gAppModule.Pause();
}

AppModule::AutoPause :: ~AutoPause( void )
{
	gAppModule.Resume();
}

AppModule::AutoFreezeTime :: AutoFreezeTime( void )
{
	gAppModule.FreezeTime();
}

AppModule::AutoFreezeTime :: ~AutoFreezeTime( void )
{
	gAppModule.ThawTime();
}

AppModule::AutoOptionsTracker :: AutoOptionsTracker( void )
{
	m_OldOptions  = gAppModule.m_Options;
	m_OldXOptions = gAppModule.m_XOptions;
}

AppModule::AutoOptionsTracker :: ~AutoOptionsTracker( void )
{
	AppModule::eOptions  newOptions  = gAppModule.m_Options;
	AppModule::eXOptions newXOptions = gAppModule.m_XOptions;

	if (   (m_OldOptions & (AppModule::OPTION_BOOST_APP_PRIORITY | AppModule::OPTION_DISABLE_APP_PRIORITY))
		!= (newOptions   & (AppModule::OPTION_BOOST_APP_PRIORITY | AppModule::OPTION_DISABLE_APP_PRIORITY)) )
	{
		gAppModule.UpdatePriorityClass();
	}

	if (   (m_OldXOptions & AppModule::OPTION_DISABLE_TASK_SWITCH)
		!= (newXOptions   & AppModule::OPTION_DISABLE_TASK_SWITCH) )
	{
#		if 0

		// $ this code is risky and is temporarily disabled. the problem is that
		//   once this is set, we must *guarantee* that we reset the flag upon
		//   crash or exit, otherwise the system will never be able to task
		//   switch again without rebooting (or running the game again).

		if ( SysInfo::IsWin9x() )
		{
			bool notask = newXOptions & AppModule::OPTION_DISABLE_TASK_SWITCH;
			UINT prevState;
			::SystemParametersInfo( SPI_SETSCREENSAVERRUNNING, notask, &prevState, false );
		}
#		endif // 0
	}
}

//////////////////////////////////////////////////////////////////////////////
// nested AppModule collections

class AppModuleNs::EventCbDb   : public std::multimap <int, AppModule::EventCb  >  {  };
class AppModuleNs::MessageCbDb : public std::multimap <int, AppModule::MessageCb>  {  };
class AppModuleNs::FrameCbDb   : public std::multimap <int, AppModule::FrameCb  >  {  };

class AppModuleNs::EventIter : public EventCbDb::iterator
{
public:
	SET_INHERITED( EventIter, EventCbDb::iterator );

	EventIter( void )						{  }
	EventIter( const EventIter& other )		: Inherited( other )  {  }
	EventIter( const Inherited& other )		: Inherited( other )  {  }

	EventIter& operator = ( const EventIter& other )  {  Inherited::operator = ( other );  return ( *this );  }
	EventIter& operator = ( const Inherited& other )  {  Inherited::operator = ( other );  return ( *this );  }
};

class AppModuleNs::InputBinderDb : public std::map <int, InputBinder*>  {  };

//////////////////////////////////////////////////////////////////////////////
// static helpers

static void NudgeMouse( int& oldDelta, int& delta )
{
	// $ if the offset was small enough, the division may have pulled it down to
	//   0 even though the user moved the mouse. so make sure it's still going
	//   to show some motion.

	if ( delta == 0 )
	{
		if ( oldDelta > 0 )
		{
			delta = 1;
		}
		else if ( oldDelta < 0 )
		{
			delta = -1;
		}
	}
}

static void NudgeMouse( int& oldDeltaX, int& deltaX, int& oldDeltaY, int& deltaY )
{
	NudgeMouse( oldDeltaX, deltaX );
	NudgeMouse( oldDeltaY, deltaY );
}

static HHOOK s_KeyboardProcHook = NULL;

static LRESULT CALLBACK LowLevelKeyboardProc( INT nCode, WPARAM wParam, LPARAM lParam )
{
	// $ this is based on KB Q226359 -sb

	// Only do this if app exists and is active.
	if ( (nCode == HC_ACTION) && AppModule::DoesSingletonExist() && gAppModule.IsAppActive() )
	{
		// By returning a non-zero value from the hook procedure, the
		// message does not get passed to the target window
		KBDLLHOOKSTRUCT *pkbhs = (KBDLLHOOKSTRUCT *) lParam;

		// Get some flags
		bool ctlDown = !!(GetAsyncKeyState( VK_CONTROL ) >> ((sizeof(SHORT) * 8) - 1));
		bool altDown = !!(pkbhs->flags & LLKHF_ALTDOWN);
		bool noTask  = gAppModule.TestOptions( AppModule::OPTION_DISABLE_TASK_SWITCH );
		bool noWin   = gAppModule.TestOptions( AppModule::OPTION_DISABLE_WIN_KEYS    );
		
		// Disable CTRL+ESC
		if ( noTask && (pkbhs->vkCode == VK_ESCAPE) && ctlDown )
			return ( TRUE );

		// Disable ALT+TAB
		if ( noTask && (pkbhs->vkCode == VK_TAB) && altDown )
			return ( TRUE );

		// Disable ALT+ESC
		if ( noTask && (pkbhs->vkCode == VK_ESCAPE) && altDown )
			return ( TRUE );

		// Disable the WINDOWS key 
		if ( (noTask || noWin) && (pkbhs->vkCode == VK_LWIN) || (pkbhs->vkCode == VK_RWIN) )
			return ( TRUE );
	}

	return ( CallNextHookEx( s_KeyboardProcHook, nCode, wParam, lParam ) );
}

static void MainInfoCallback( DrWatson::Info& info )
{
	// all of this stuff requires memory allocs
	if ( info.m_InfoFlags & DrWatson::INFO_NO_MALLOC )
	{
		return;
	}

	// and exception info too
	if ( info.m_Sex == NULL )
	{
		return;
	}

	// do minidump - note that in retail watson will do it, so only bother if
	// we're in test mode (so we can get dump files locally)
	gpstring miniDumpName;
	bool doMiniDump = !GP_RETAIL || (AppModule::DoesSingletonExist() && gAppModule.TestOptions( AppModule::OPTION_TEST_MODE ));
	if ( doMiniDump )
	{
		gDbgSymbolEngine.MiniDump( false, &info.m_Sex->GetInformation(), &miniDumpName );
	}
	DWORD miniDumpError = ::GetLastError();

	// need a writer for this part
	if ( info.m_Writer != NULL )
	{
		if ( doMiniDump )
		{
			// output header
			info.m_Writer->WriteText( "[Minidump]\n\n" );

			// minidump?
			if ( miniDumpName.empty() )
			{
				info.m_Writer->WriteF(
						"    FAILED! Error is '%s' (0x%08X)\n\n",
						 stringtool::GetLastErrorText( miniDumpError ).c_str(),
						 miniDumpError );
			}
			else if ( doMiniDump )
			{
				info.m_Writer->WriteF( "    '%s'\n\n", miniDumpName.c_str() );
			}
		}

		// output header
		info.m_Writer->WriteText( "[Trace]\n\n" );

		// dump exception info to contexts
		ReportSys::LocalContext ctx( new ReportSys::FileSysStreamerSink( *info.m_Writer ), true );
		info.m_Sex->Dump( ctx, !!(info.m_InfoFlags & DrWatson::INFO_FULL) );
		info.m_Writer->WriteText( "\n\n" );
	}
}

//////////////////////////////////////////////////////////////////////////////

// Common options.

	const AppModule::eOptions OPTION_BASE = AppModule::OPTION_NONE
#		if !GP_RETAIL
		| AppModule::OPTION_REPORT_STATUS
#		endif
#		if GP_DEBUG
		| AppModule::OPTION_FREEZE_TIME_DETECT
#		endif
		| AppModule::OPTION_EXCEPTION_HANDLER
		| AppModule::OPTION_THROTTLE_WINDOWED
		| AppModule::OPTION_MOUSE_SMOOTH_FIXED_DRAG
#		if GP_USE_HANG_BUDDY
		| AppModule::OPTION_HANG_BUDDY
#		endif
		;

// Default options.

	const AppModule::eOptions AppModule::OPTION_CONSOLE_DEF = OPTION_BASE
		| AppModule::OPTION_DISABLE_LOCALIZE_WARNING
		| AppModule::OPTION_PERSIST_POSITION_WND
		| AppModule::OPTION_SCREEN_CONSTRAIN_WND
		;

	const AppModule::eOptions AppModule::OPTION_WINDOWED_DEF = OPTION_BASE
		| AppModule::OPTION_CREATE_MAINWND
		| AppModule::OPTION_RESIZABLE_WND
		| AppModule::OPTION_MESSAGE_PUMP
		| AppModule::OPTION_SHUTDOWN_CRITICAL
		| AppModule::OPTION_PERSIST_POSITION_WND
		| AppModule::OPTION_PERSIST_SIZE_WND
		| AppModule::OPTION_SCREEN_CONSTRAIN_WND
		| AppModule::OPTION_ALWAYS_ON_TOP_FULLSCREEN
		;

	const AppModule::eOptions AppModule::OPTION_UTILWND_DEF = AppModule::OPTION_WINDOWED_DEF
		| AppModule::OPTION_DISABLE_LOCALIZE_WARNING
		;

	const AppModule::eOptions AppModule::OPTION_FULLSCREEN_DEF = AppModule::OPTION_WINDOWED_DEF
#		if GP_RETAIL
		| AppModule::OPTION_APP_SINGLETON_MUTEX
#		endif
		| AppModule::OPTION_FULLSCREEN
		;

	const AppModule::eOptions AppModule::OPTION_CHILD_DEF = OPTION_BASE
		| AppModule::OPTION_CREATE_MAINWND
		| AppModule::OPTION_CHILD_WND
		| AppModule::OPTION_EXECUTE_FRAME_ONLY
		| AppModule::OPTION_REFLECT_INPUT_PARENT
		;

template <typename T>
void SetReset( T& val, T bit, int set )
{
	if ( set )
	{
		val |= bit;
	}
	else
	{
		val &= Keys::NOT( bit );
	}
}

// Constants.

	// when minimally active, use this frame rate
	const float MINIMALLY_ACTIVE_FPS = 10.0f;

//////////////////////////////////////////////////////////////////////////////
// class AppModule implementation

AppModule :: AppModule( eOptions options, eXOptions xoptions )
{
	gpstring moduleName = FileSys::GetModuleFileName();
	gpstring fileName = FileSys::GetFileName( moduleName );
	stringtool::RemoveExtension( fileName );

	// spec
	m_AppIcon            = NULL;
	m_AppAccel           = NULL;
	m_AppMenu            = NULL;
	m_AppName            = ::ToUnicode( fileName );
	m_IniName            = fileName;
	m_WndClassName       = MakeUniqueName( "GPGWndClass_" );
	m_ParentWnd          = NULL;
	m_StartupRect        = GRect::ZERO;
	m_Options            = options;
	m_XOptions           = xoptions;
	m_EventCbDb          = new EventCbDb;
	m_MessageCbDb        = NULL;
	m_FrameCbDb          = NULL;
	m_SysFrameCbDb       = NULL;

	// state
	m_ReturnValue        = 0;
	m_Instance           = NULL;
	m_MainWnd            = NULL;
	m_FullScreen         = false;
	m_AppActivateCount   = 1;
	m_UserPaused         = 0;
	m_PauseCount         = 0;
	m_FreezeTimeCount    = 0;
	m_TimeMultiplier     = 1.0f;
	m_SingleStepTime     = 0.005f;
	m_ThrottleFrameRate  = 0;
	m_MinFrameRate       = 0;
	m_FixedFrameRate     = 0;
	m_InactiveUpdateRate = 0;
	m_WindowRect         = GRect::ZERO;
	m_ClientRect         = GRect::ZERO;
	m_GameRect           = GRect::ZERO;
	m_InstantFrameRate   = 0;
	m_FilteredFrameRate  = 0;
	m_Frame              = 0;
	m_SysFrame           = 0;
	m_AbsoluteTime       = 0;
	m_DeltaTime          = 0;
	m_ActualDeltaTime    = 0;
	m_IsMovingSizing     = false;
	m_WasResized         = false;
	m_IsReplacingWnd     = false;
	m_CurrentQualifiers  = Keys::Q_NONE;
	m_CursorX            = 0;
	m_CursorY            = 0;
	m_CursorCenterX      = 0;
	m_CursorCenterY      = 0;
	m_NormalizedCursorX  = 0;
	m_NormalizedCursorY  = 0;
	m_DblClickMask       = 0xFFFFFFFF;
	m_DblClickTime       = 0;
	m_DblClickButton     = -1;
	m_DblClickOrigin     = GPoint::ZERO;
	m_SkipNextChar       = false;
	m_MouseScaleX        = 1.0f;
	m_MouseScaleY        = 1.0f;
	m_MouseScaleZ        = 1.0f;
	m_FixedMouseScaleX   = 1.0f;
	m_FixedMouseScaleY   = 1.0f;
	m_FixedMouseScaleZ   = 1.0f;
	m_MouseCritical      = new kerneltool::Critical;
	m_ReqKeyboardReset   = false;
	m_ReqMouseReset      = false;
	m_RequestQuit        = new Event;
	m_RequestFatalQuit   = new Event;
	m_InstanceMutex      = NULL;
	m_EventIter          = NULL;
	m_Config             = NULL;
	m_ConfigInit         = new ConfigInit;
	m_InputBinderDb      = NULL;
	m_GlobalBinder       = NULL;
	m_LocMgr             = NULL;
	m_HangBuddy          = NULL;

	::ZeroObject( m_LastMsg      );
	::ZeroObject( m_MouseCapture );
	::ZeroObject( m_MouseHistory );

	::GetLocalTime( &m_StartTime );

	// default events
	//
	// $ note: no OnExecute/PrivateExecute - initiated by Run() instead

	// init
	RegisterEventCb( makeFunctor( *this, &AppModule::PrivateInitCore     ), PRI_INIT_CORE         );
	RegisterEventCb( makeFunctor( *this, &AppModule::OnInitCore          ), PRI_INIT_WINDOW   - 1 );
	RegisterEventCb( makeFunctor( *this, &AppModule::PrivateInitWindow   ), PRI_INIT_WINDOW       );
	RegisterEventCb( makeFunctor( *this, &AppModule::OnInitWindow        ), PRI_INIT_SERVICES - 1 );
	RegisterEventCb( makeFunctor( *this, &AppModule::PrivateInitServices ), PRI_INIT_SERVICES     );
	RegisterEventCb( makeFunctor( *this, &AppModule::OnInitServices      ), PRI_EXECUTE       - 1 );

	// shutdown (note it's reversed)
	RegisterEventCb( makeFunctor( *this, &AppModule::OnShutdown          ), PRI_SHUTDOWN     );
	RegisterEventCb( makeFunctor( *this, &AppModule::PrivateKillWindow   ), PRI_FINAL    - 2 );
	RegisterEventCb( makeFunctor( *this, &AppModule::PrivateShutdown     ), PRI_FINAL    - 1 );

	// add a dummy entry for OnExecute() so PRI_EXECUTE comparisons will succeed
	m_EventCbDb->insert( EventCbDb::value_type( PRI_EXECUTE, NULL ) );

	// profiling?
#	if GP_ENABLE_PROFILING
	if ( gQuantifyApi.IsRunning() )
	{
		SetOptions( OPTION_IS_EXTERNAL_PROFILING );
	}
#	endif // GP_ENABLE_PROFILING
#	if GP_PROFILING
	SetOptions( OPTION_IS_EXTERNAL_PROFILING );
#	endif // GP_ENABLE_STATS

	// register main callback
	DrWatson::RegisterInfoCallback( makeFunctor( MainInfoCallback ), false );
}

AppModule :: ~AppModule( void )
{
	delete ( m_EventCbDb        );
	delete ( m_MessageCbDb      );
	delete ( m_FrameCbDb        );
	delete ( m_SysFrameCbDb     );
	delete ( m_MouseCritical    );
	delete ( m_RequestQuit      );
	delete ( m_RequestFatalQuit );
	delete ( m_InstanceMutex    );
	delete ( m_EventIter        );
	delete ( m_Config           );
	delete ( m_ConfigInit       );
	delete ( m_LocMgr           );
	delete ( m_HangBuddy        );

	if ( TestOptions( OPTION_REPORT_STATUS ) )
	{
		gWarningContext.ClearOptions( ReportSys::Context::OPTION_LOCALIZE );
		AssertInstance::ReportSummary( &gWarningContext );
		ReportSys::ReportErrorSummary( &gWarningContext );

#		if GPMEM_DBG_NEW
		gpstring statusSummary;
		gpmem::GetStatusSummary( statusSummary );
		gGenericContext.Output( statusSummary );
#		endif // GPMEM_DBG_NEW
	}
}

bool AppModule :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_TimeMultiplier", m_TimeMultiplier );
	persist.Xfer( "m_SingleStepTime", m_SingleStepTime );
	persist.Xfer( "m_Frame",          m_Frame          );
	persist.Xfer( "m_SysFrame",       m_SysFrame       );
	persist.Xfer( "m_AbsoluteTime",   m_AbsoluteTime   );

	if ( persist.IsRestoring() )
	{
		::SetGlobalSecondsAdjust( PreciseSubtract(m_AbsoluteTime, ::GetSystemSeconds()) );
	}

	return ( true );
}

DWORD AppModule :: GetAppCrc32( void ) const
{
	const IMAGE_DOS_HEADER* dosHeader = (const IMAGE_DOS_HEADER*)::GetModuleHandle( NULL );
	const IMAGE_NT_HEADERS* winHeader = (const IMAGE_NT_HEADERS*)( (const BYTE*)dosHeader + dosHeader->e_lfanew );
	return ( winHeader->OptionalHeader.CheckSum );
}

bool AppModule :: IsInitialized( void ) const
{
	return (   (m_EventIter != NULL)
			&& (*m_EventIter != m_EventCbDb->end())
			&& ((*m_EventIter)->first == PRI_EXECUTE) );
}

bool AppModule :: IsInitializing( void ) const
{
	return (   (m_EventIter != NULL)
			&& (*m_EventIter != m_EventCbDb->end())
			&& ((*m_EventIter)->first < PRI_EXECUTE) );
}

bool AppModule :: IsShuttingDown( void ) const
{
	return (   (m_EventIter != NULL)
			&& (   (*m_EventIter == m_EventCbDb->end())
				|| ((*m_EventIter)->first > PRI_EXECUTE)) );
}

void AppModule :: SetOptions( eOptions options, bool set )
{
	AutoOptionsTracker autoOptionsTracker;
	m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));
}

void AppModule :: ToggleOptions( eOptions options )
{
	AutoOptionsTracker autoOptionsTracker;
	m_Options = (eOptions)(m_Options ^ options);
}

void AppModule :: SetOptions( eXOptions options, bool set )
{
	AutoOptionsTracker autoOptionsTracker;
	m_XOptions = (eXOptions)(set ? (m_XOptions | options) : (m_XOptions & ~options));
}

void AppModule :: ToggleOptions( eXOptions options )
{
	AutoOptionsTracker autoOptionsTracker;
	m_XOptions = (eXOptions)(m_XOptions ^ options);
}

void AppModule :: SaveWindowPos( void )
{
	// save it
	if (   ::IsWindow( m_MainWnd )
		&& Config::DoesSingletonExist()
		&& !IsFullScreen() )
	{
		if ( TestOptions( OPTION_PERSIST_POSITION_WND ) )
		{
			gConfig.Set( "x", m_GameRect.left );
			gConfig.Set( "y", m_GameRect.top );
		}
		if ( TestOptions( OPTION_PERSIST_SIZE_WND ) )
		{
			gConfig.Set( "width",  m_GameRect.Width() );
			gConfig.Set( "height", m_GameRect.Height() );
		}
	}
}

gpwstring AppModule :: MakeTitle( void ) const
{
	gpwstring title = m_AppName;

	// try window if no app name
	if ( title.empty() && (m_MainWnd != NULL) )
	{
		title = ::ToUnicode( winx::GetWindowText( GetMainWnd() ) );
	}

	return ( title );
}

DWORD AppModule :: CalcWindowStyle( void ) const
{
	DWORD style = 0;

	if ( IsFullScreen() )
	{
		style = WS_POPUP | WS_VISIBLE;
	}
	else
	{
		if ( TestOptions( OPTION_CHILD_WND ) )
		{
			style = WS_CHILD | WS_VISIBLE;
		}
		else
		{
			style = WS_OVERLAPPEDWINDOW | WS_BORDER | WS_VISIBLE;
			if ( !TestOptions( OPTION_RESIZABLE_WND ) )
			{
				style &= ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
			}
		}
	}

	return ( style );
}

void AppModule :: AppActivate( bool activate )
{
	// may be called before engine completely up yet
	if ( !IsInitialized() )
	{
		return;
	}

	// process message
	if ( activate )
	{
		// on reset UI
		if ( IsFullScreen() && !TestOptions( OPTION_CHILD_WND ) )
		{
			ResetUserInputs();
		}

		// first time?
		if ( m_AppActivateCount == 0 )
		{
			if ( !TestOptions( OPTION_ALWAYS_MINIMALLY_ACTIVE ) )
			{
				Resume();
			}
			++m_AppActivateCount;

			UpdatePriorityClass();

			ReportSys::CallSafeMessageBoxPost( false );

			// $ this little bit of code is to watch for activation that can
			//   happen while the app is minimized (right-clicking on the app
			//   icon while it's minimized for example).

			if ( (::GetActiveWindow() != NULL) && !IsMinimized() && !IsShuttingDown() )
			{
				// notify subclass
				OnAppActivate( true );
			}
		}
	}
	else
	{
		// first time?
		if ( m_AppActivateCount == 1 )
		{
			if ( !TestOptions( OPTION_ALWAYS_MINIMALLY_ACTIVE ) )
			{
				Pause();
			}
			--m_AppActivateCount;

			UpdatePriorityClass();

			ReportSys::CallSafeMessageBoxPre( false );

			if ( !IsShuttingDown() )
			{
				// notify subclass
				OnAppActivate( false );
			}
		}
	}
}

bool AppModule :: RequestQuit( bool force )
{
	if ( force || OnRequestAppClose() )
	{
		m_RequestQuit->Set();
	}

	return ( GetRequestQuit() );
}

bool AppModule :: GetRequestQuit( void ) const
{
	return ( m_RequestQuit->IsSignaled() || m_RequestFatalQuit->IsSignaled() );
}

void AppModule :: RequestFatalQuit( void )
{
	m_RequestFatalQuit->Set();
}

void AppModule :: SetGameSize( const GSize& size )
{
	GSize localSize( size );

	if ( localSize == GSize::ZERO )
	{
		if ( Config::DoesSingletonExist() )
		{
			localSize.cx = gConfig.GetInt( "width",  640 );
			localSize.cy = gConfig.GetInt( "height", 480 );
		}
		else
		{
			localSize.cx = 640;
			localSize.cy = 640;
		}
	}

	if ( !IsFullScreen() )
	{
		GRect rect( m_GameRect.TopLeft(), localSize );
		if ( ::AdjustWindowRect( &rect, ::GetWindowLong( GetMainWnd(), GWL_STYLE ), HasAppMenu() ) )
		{
			// get screen rect to use
			GRect screenRect( winx::MakeVirtualScreenRect() );

			// make sure it will fit
			if ( (rect.Width() <= screenRect.Width()) && (rect.Height() <= screenRect.Height()) )
			{
				rect.ConstrainTo( screenRect );
				::MoveWindow( GetMainWnd(), rect.left, rect.top, rect.Width(), rect.Height(), TRUE );
				::ShowWindow( GetMainWnd(), SW_RESTORE );
			}
			else
			{
				// don't use new res
				gpmessageboxf(( $MSG$ "You cannot run %S in a resolution (%dx%d) higher than your desktop (%dx%d) when in windowed mode. "
								"(Note that windowed mode uses slightly larger area than fullscreen due to the GDI title and border).\n\n"
								"To work around this problem, you can either lower the resolution for %S, raise the resolution of the desktop, or run full screen.",
								GetAppName().c_str(), localSize.cx, localSize.cy,
								screenRect.Width(), screenRect.Height(), GetAppName().c_str() ));
				localSize = m_GameRect.Size();
			}
		}
	}

	bool isDirty = true;

	// check game size
	if ( m_GameRect.Size() == localSize )
	{
		// check fullscreen
		if ( (Config::DoesSingletonExist()) && (IsFullScreen() == gConfig.GetBool( "fullscreen", m_FullScreen )) )
		{
			// check derived
			if ( !OnGetGameSizeDirty() )
			{
				isDirty = false;
			}
		}
	}

	// update if dirty
	if ( isDirty )
	{
		m_GameRect.ResizeTo( localSize );
		OnScreenChange();
	}
}

void AppModule :: SetFullScreen( bool set )
{
	// do nothing if no change
	if ( set == IsFullScreen() )
	{
		return;
	}

	// beginning replacement
	m_IsReplacingWnd = true;

	// ok kill old window
	PrivateKillWindow();

	// create new one
	SetOptions( OPTION_FULLSCREEN, set );
	EventIter oldIter = *m_EventIter;
	*m_EventIter = m_EventCbDb->begin();
	while ( (*m_EventIter != m_EventCbDb->end()) && ((*m_EventIter)->first < PRI_INIT_WINDOW) )
	{
		++(*m_EventIter);
	}
	ExecPipelineTo( PRI_INIT_SERVICES );
	*m_EventIter = oldIter;

	// ending replacement
	m_IsReplacingWnd = false;
}

gpstring AppModule :: MakeMouseDebugState( void ) const
{
	const char* mmode = "norm";
	if ( TestOptions( OPTION_MOUSE_FIXED ) )
	{
		mmode = "fixed";
	}
	else if ( m_CurrentQualifiers & Keys::Q_ANY_BUTTON )
	{
		mmode = "drag";
	}

	return ( gpstringf( "xy:%s,wrap:%s", mmode,
			 TestOptions( OPTION_MOUSE_WRAP ) ? "yes" : "no" ) );
}

void AppModule :: UserPause( bool pause )
{
	if ( pause && !m_UserPaused )
	{
		m_UserPaused = true;
		OnUserPause( true );
	}
	else if ( !pause && m_UserPaused )
	{
		m_UserPaused = false;
		OnUserPause( false );
	}
}

void AppModule :: Pause( bool pause )
{
	gpassert( m_PauseCount >= 0 );

	if ( pause )
	{
		FreezeTime();

		if ( m_PauseCount++ == 0 )
		{
			OnPause( true );
		}
	}
	else
	{
		if ( --m_PauseCount == 0 )
		{
			OnPause( false );
		}

		ThawTime();
	}

	gpassert( m_PauseCount >= 0 );
}

void AppModule :: ForcePause( bool pause )
{
	if ( pause )
	{
		if ( !IsPaused() )
		{
			m_PauseCount = 0;
			Pause();
		}
	}
	else
	{
		if ( IsPaused() )
		{
			m_PauseCount = 1;
			Resume();
		}
	}
}

void AppModule :: FreezeTime( bool freeze )
{
	gpassert( m_FreezeTimeCount >= 0 );

	if ( freeze )
	{
		if ( m_FreezeTimeCount++ == 0 )
		{
			OnFreezeTime( true );
		}
	}
	else
	{
		if ( --m_FreezeTimeCount == 0 )
		{
			m_AbsoluteTime = PreciseSubtract( ::GetGlobalSeconds(), 0.01 );	// guarantee a 0.01 delta next frame
			OnFreezeTime( false );
		}
	}

	gpassert( m_FreezeTimeCount >= 0 );
}

void AppModule :: RegisterEventCb( EventCb cb, int priority )
{
	gpassert( (m_EventIter == NULL) || ((*m_EventIter != m_EventCbDb->end()) && (priority >= (*m_EventIter)->first)) );
	gpassert( priority != PRI_EXECUTE );		// use FrameCb registration instead

	m_EventCbDb->insert( std::make_pair( priority, cb ) );
}

void AppModule :: AddMessageCb( MessageCb cb, int priority )
{
	GetMessageCbDb().insert( std::make_pair( priority, cb ) );
}

void AppModule :: RemoveMessageCb( MessageCb cb )
{
	gpassert( m_MessageCbDb != NULL );
	MessageCbDb::iterator found = std::find( stdx::make_iter_second( m_MessageCbDb->begin() ),
											 stdx::make_iter_second( m_MessageCbDb->end  () ),
											 cb );
	gpassert( found != m_MessageCbDb->end() );
	m_MessageCbDb->erase( found );
}

void AppModule :: AddFrameCb( FrameCb cb, int priority )
{
	GetFrameCbDb().insert( std::make_pair( priority, cb ) );
}

void AppModule :: RemoveFrameCb( FrameCb cb )
{
	gpassert( m_FrameCbDb != NULL );
	FrameCbDb::iterator found = std::find( stdx::make_iter_second( m_FrameCbDb->begin() ),
										   stdx::make_iter_second( m_FrameCbDb->end  () ),
										   cb );
	gpassert( found != m_FrameCbDb->end() );
	m_FrameCbDb->erase( found );
}

void AppModule :: AddSysFrameCb( FrameCb cb, int priority )
{
	GetSysFrameCbDb().insert( std::make_pair( priority, cb ) );
}

void AppModule :: RemoveSysFrameCb( FrameCb cb )
{
	gpassert( m_SysFrameCbDb != NULL );
	FrameCbDb::iterator found = std::find( stdx::make_iter_second( m_SysFrameCbDb->begin() ),
										   stdx::make_iter_second( m_SysFrameCbDb->end  () ),
										   cb );
	gpassert( found != m_SysFrameCbDb->end() );
	m_SysFrameCbDb->erase( found );
}

void AppModule :: RegisterBinder( InputBinder* binder, int priority )
{
	gpassertm( GetInputBinderDb().find( priority ) == GetInputBinderDb().end(),
			   "A binder with this priority has alrady been registered. Priority collisions not supported." );
	GetInputBinderDb().insert( std::make_pair( priority, binder ) );
}

void AppModule :: UnregisterBinder( InputBinder* binder )
{
	gpassert( m_InputBinderDb != NULL );

	// unregister
	InputBinderDb::iterator found = std::find( stdx::make_iter_second( m_InputBinderDb->begin() ),
											   stdx::make_iter_second( m_InputBinderDb->end  () ),
											   binder );
	gpassert( found != m_InputBinderDb->end() );
	m_InputBinderDb->erase( found );

	// clear all capture for this binder if there were any
	InputBinder** i, ** begin = m_MouseCapture, ** end = begin + ELEMENT_COUNT( m_MouseCapture );
	for ( i = begin ; i != end ; ++i )
	{
		if ( *i == binder )
		{
			*i = NULL;
		}
	}
}

InputBinder& AppModule :: GetGlobalBinder( void )
{
	if ( m_GlobalBinder == NULL )
	{
		m_GlobalBinder = new InputBinder( "global" );
		RegisterBinder( m_GlobalBinder, 1 );
	}
	return ( *m_GlobalBinder );
}

// $$$ log
#if GP_PROFILING
#include "qmem.h"
QMem::LogFileWriter s_Writer;
#endif

static LPTOP_LEVEL_EXCEPTION_FILTER s_OldExceptionFilter = NULL;

bool AppModule :: Init( HINSTANCE instance )
{

// Verification.

	// can't initialize twice or recursively!
	gpassert( IsUninitialized() );

	// check that we aren't child and fullscreen - logic error
	gpassert( !(TestOptions( OPTION_FULLSCREEN ) && TestOptions( OPTION_CHILD_WND )) );

	// check that we aren't child unless we're also mainwnd
	gpassert( !(TestOptions( OPTION_CHILD_WND ) && !TestOptions( OPTION_CREATE_MAINWND)) );

	// check that we have a parent window if we're a child
	gpassert( !(TestOptions( OPTION_CHILD_WND ) && !::IsWindow( m_ParentWnd )) );

	// check that we don't constrain if we're a child - logic error
	gpassert( !(TestOptions( OPTION_CHILD_WND ) && TestOptions( OPTION_PERSIST_POSITION_WND | OPTION_SCREEN_CONSTRAIN_WND )) );

	// these don't make sense together
	gpassert( !(TestOptions( OPTION_MESSAGE_PUMP ) && TestOptions( OPTION_EXECUTE_FRAME_ONLY )) );

	// check that we aren't reflecting if not a child - logic error
	gpassert( !(TestOptions( OPTION_REFLECT_INPUT_PARENT ) && !TestOptions( OPTION_CHILD_WND )) );

// Preliminary setup.

	// take instance
	if ( instance == NULL )
	{
		instance = ::GetModuleHandle( NULL );
	}
	m_Instance = instance;
	gpassert( m_Instance != NULL );

	// set FPU to 24-bit chop mode so FTOL is fast and happy
	if ( TestOptions( OPTION_FAST_FTOL ) )
	{
		SetFpuChopMode();
	}

	// register our keyboard hook (this does not work in win9x) but not if the
	// debugger is running or the keyboard input will slow way down.
	if ( !SysInfo::IsWin9x() && !SysInfo::IsDebuggerPresent() )
    {
		s_KeyboardProcHook = ::SetWindowsHookEx( WH_KEYBOARD_LL, LowLevelKeyboardProc, GetInstance(), 0 );
    }

	// configure strings
	gpstring::SetSystemInstance( m_Instance );
	gpstring::SetLocalizedInstance( m_Instance );

	// install exception handler
	if ( TestOptions( OPTION_EXCEPTION_HANDLER ) )
	{
		s_OldExceptionFilter = ::SetUnhandledExceptionFilter( StaticExceptionFilter );
	}

	// disable warnings if explicitly say to or if we aren't localizing anyway
	if ( TestOptions( OPTION_DISABLE_LOCALIZE_WARNING ) || !TestOptions( OPTION_LOCALIZE ) )
	{
		using namespace ReportSys;

		Mgr::ContextColl contexts;
		gReportSysMgr.QueryContextByName( contexts, "*" );
		Mgr::ContextColl::iterator i, begin = contexts.begin(), end = contexts.end();
		for ( i = begin ; i != end ; ++i )
		{
			(*i)->ClearOptions( Context::OPTION_LOCALIZE );
		}
	}

	// install message hooks so that if a dialog ever comes up time is frozen
	ReportSys::RegisterPreMessageBoxHook( PreMessageBoxCb );
	ReportSys::RegisterPostMessageBoxHook( PostMessageBoxCb );

// App singleton.

	// build registry
	m_Config = new Config( *m_ConfigInit );
	if ( !m_Config->Init() )
	{
		return ( false );
	}

	// update output dir with new info
	DrWatson::InitOutputDirectory( m_Config->ResolvePathVars( "%log_path%" ) );

	// check mutex to ensure that only one copy of the game runs at a time
	if ( !gConfig.GetBool( "multi", !TestOptions( OPTION_APP_SINGLETON_MUTEX ) ) )
	{
		// attempt to create owned mutex
		m_InstanceMutex = new Mutex( MakeUniqueName( "GPGMutex_" ), true );
		if ( ::GetLastError() == ERROR_ALREADY_EXISTS )
		{
			if ( !m_WndClassName.empty() )
			{
				// activate and bring the other app to the top
				HWND hwnd = ::FindWindow( m_WndClassName, NULL );
				if ( hwnd != NULL )
				{
					::ShowWindow( hwnd, SW_SHOWDEFAULT );
					::BringWindowToTop( hwnd );
					::SetForegroundWindow( hwnd );
				}
			}
			m_ReturnValue = RET_ALREADY_RUNNING;
			return ( false );
		}
	}

	// check if we're tracking stats
#	if GP_ENABLE_STATS
	if ( gConfig.GetBool( "gpstats", false ) )
	{
		gGpStatsMgr.Enable();
		gGpStatsMgr.SetOptions( GpStatsMgr::OPTION_DUMP_ON_STAGE_CHANGE );
	}
#	endif // GP_ENABLE_STATS

	// see if we have a fixed random seed
	DWORD seed;
	if ( gConfig.Read( "random_seed", seed ) )
	{
		GetGlobalRng().SetSeed( seed );
	}
	else if ( TestOptions( OPTION_IS_EXTERNAL_PROFILING ) )
	{
		GetGlobalRng().SetSeed( 0 );
	}

// Initialize.

	// log
#	if GP_PROFILING
	s_Writer.Create( gConfig.ResolvePathVars( "%log_path%/qmem.dat" ) );
	s_Writer.EnableLogging();
#	endif

	// init iter
	m_EventIter = new EventIter( m_EventCbDb->begin() );

	// initialize!
	bool success = ExecPipelineTo( PRI_EXECUTE );

	// optionally report results
	if ( TestOptions( OPTION_REPORT_STATUS ) )
	{
		gpgenericf(( "\nInitialization %s.\n%d errors\n%d warnings\n%d failed assertions\n",
					 success ? "succeeded" : "failed",
					 ReportSys::GetErrorCount(),
					 ReportSys::GetWarningCount(),
					 AssertData::ms_FailedCount ));
	}

	// done
	return ( success );
}

bool AppModule :: Run( void )
{
	gpassert( IsInitialized() );

	bool success = false;

	__try
	{
		__try
		{
			// execute
			success = PrivateExecute();
		}
		__except( ::GlobalExceptionFilter( GetExceptionInformation() ) )
		{
			// $ it's already been reported, ignore it but let all other unexpected
			//   exceptions through. unfortunately, given that exception handling is
			//   disabled, we'll not be shutting down properly and will get a
			//   billion memory leak reports, so just ignore all those.
			GPDEBUG_ONLY( _CrtSetDbgFlag( 0 ) );

			// failed
			m_ReturnValue = RET_FATAL;
		}
	}
	__finally
	{
		if ( ::AbnormalTermination() )
		{
			OnFailSafeShutdown();
			GPDEBUG_ONLY( _CrtSetDbgFlag( 0 ) );	// $ as above
			m_ReturnValue = RET_UNKNOWN;
		}
	}

	return ( success );
}

bool AppModule :: Shutdown( void )
{
	GPSTATS_ONLY( gpstats::SetCurrentStage( STAGE_SHUTDOWN ) );

	// ffwd iter to shutdown sequence
	if ( m_EventIter == NULL )
	{
		m_EventIter = new EventIter( m_EventCbDb->begin() );
	}
	while ( (*m_EventIter)->first <= PRI_EXECUTE )
	{
		++(*m_EventIter);
	}

	// finish it out
	return ( ExecPipelineTo( PRI_FINAL ) );
}

int AppModule :: AutoRun( HINSTANCE instance )
{
	if ( Init( instance ) )
	{
		Run();
	}

	Shutdown();

	return ( m_ReturnValue );
}

void AppModule :: SetMouseModeFixed( bool set )
{
	// clear caches
	::ZeroObject( m_MouseHistory );

	// see if it is changing
	if ( (TestOptions( OPTION_MOUSE_FIXED ) != set) )
	{
		// update mode
		SetOptions( OPTION_MOUSE_FIXED, set );

		// force an update now
		bool center = UpdateCursor();

		// reset cursor position to where we turned on fixed mode
		if ( !set && !center )
		{
			::SetCursorPos( m_GameRect.left + m_CursorX, m_GameRect.top + m_CursorY );
		}

		// if not full screen, update show cursor state so we can see it (when
		// not dragging) in the non-client area.
		if ( !IsFullScreen() && !TestOptions( OPTION_CHILD_WND ) )
		{
			::ShowCursor( !set );
		}
	}
}

bool AppModule :: FrameUpdateCursor( int* pCursorX, int* pCursorY, bool bFullUpdate )
{
	if( !HasMainWnd() )
	{
		return false;
	}

	kerneltool::Critical::Lock autoLock( *m_MouseCritical );

	int deltaX = 0, deltaY = 0;

// Acquire.

	// get cursor
	GPoint pos;
	::GetCursorPos( pos );
	::ScreenToClient( GetMainWnd(), pos );

	// update cursor and process based on whether we are supposed
	// to center or not
	if ( UpdateCursor( bFullUpdate ) )
	{
		// delta is from the center
		deltaX = pos.x - m_CursorCenterX;
		deltaY = pos.y - m_CursorCenterY;

		// now adjust the center - it has moved!
		if ( !bFullUpdate )
		{
			m_CursorCenterX = pos.x;
			m_CursorCenterY = pos.y;
		}
	}
	else
	{
		// delta is diff from last frame
		deltaX = pos.x - (*pCursorX);
		deltaY = pos.y - (*pCursorY);
	}

// Scale.

	// only scale if fullscreen or fixed mouse
	if ( IsFullScreen() || TestOptions( OPTION_MOUSE_FIXED ) )
	{
		// get scale to use based on mode
		float scaleX = TestOptions( OPTION_MOUSE_FIXED ) ? m_FixedMouseScaleX : m_MouseScaleX;
		float scaleY = TestOptions( OPTION_MOUSE_FIXED ) ? m_FixedMouseScaleY : m_MouseScaleY;

		// apply scaling
		int oldDeltaX = deltaX, oldDeltaY = deltaY;
		deltaX = (int)(deltaX * scaleX);
		deltaY = (int)(deltaY * scaleY);

		// clean up
		NudgeMouse( oldDeltaX, deltaX, oldDeltaY, deltaY );
	}

// Move it.

	// move game cursor
	if ( !TestOptions( OPTION_MOUSE_FIXED ) )
	{
		// remember old pos
		int oldX = (*pCursorX), oldY = (*pCursorY);

		// update pos
		(*pCursorX) += deltaX;
		(*pCursorY) += deltaY;

		// clamp or wrap
		if ( TestOptions( OPTION_MOUSE_WRAP ) )
		{
			wrap_min_max( (*pCursorX), 0, GetGameWidth () - 1 );
			wrap_min_max( (*pCursorY), 0, GetGameHeight() - 1 );
		}
		else
		{
			clamp_min_max( (*pCursorX), 0, GetGameWidth () - 1 );
			clamp_min_max( (*pCursorY), 0, GetGameHeight() - 1 );
		}

		// now reset delta - cursor may have wrapped or clamped
		deltaX = (*pCursorX) - oldX;
		deltaY = (*pCursorY) - oldY;

		if( bFullUpdate )
		{
			UpdateNormalizedCursor( *pCursorX, *pCursorY );
		}
	}

// Smooth.

	// optionally smooth
	if ( TestOptionsEq( OPTION_MOUSE_FIXED | OPTION_MOUSE_SMOOTH_FIXED_DRAG ) )
	{
		// ramp down faster on stop
		int div = 2;
		if ( (deltaX == 0) && (deltaY == 0) )
		{
			div = 4;
		}

		// initial divide
		int oldDeltaX = deltaX, oldDeltaY = deltaY;
		deltaX /= div;
		deltaY /= div;
		div *= 2;

		// smooth out
		GPoint* i, * begin = m_MouseHistory, * end = ARRAY_END( m_MouseHistory );
		for ( i = begin ; i != end ; ++i, div *= 2 )
		{
			deltaX += i->x / div;
			deltaY += i->y / div;
		}

		// restore truncated motion
		NudgeMouse( oldDeltaX, deltaX, oldDeltaY, deltaY );

		// shift it down
		for ( i = begin ; i != (end - 1) ; ++i )
		{
			*i = i[ 1 ];
		}
		i->x = deltaX;
		i->y = deltaY;
	}

// Dispatch.

	if ( bFullUpdate )
	{
		// dispatch
		if ( deltaX != 0 )
		{
			MouseInput mouse( "dx", m_CurrentQualifiers );
			mouse.SetDelta( deltaX );
			Dispatch( mouse );
		}
		if ( deltaY != 0 )
		{
			MouseInput mouse( "dy", m_CurrentQualifiers );
			mouse.SetDelta( deltaY );
			Dispatch( mouse );
		}

		// notify child
		if ( (deltaX != 0) || (deltaY != 0) )
		{
			OnMouseFrameDelta( deltaX, deltaY );
		}
	}

	// Add current position of the cursor into our buffer
	MousePosCache mousePos;
	mousePos.m_CursorX	= (*pCursorX);
	mousePos.m_CursorY	= (*pCursorY);
	mousePos.m_Time		= GetTickCount();
	m_CursorBuffer.push_back( mousePos );

	return true;
}

void AppModule :: ResetUserInputs( bool keyboard, bool mouse )
{
	m_ReqKeyboardReset |= keyboard;
	m_ReqMouseReset    |= mouse;
}

LONG WINAPI AppModule :: StaticExceptionFilter( EXCEPTION_POINTERS* xinfo )
{
	DWORD lastError = ::GetLastError();

	LONG rc = ::GlobalExceptionFilter( xinfo );
	if ( rc != EXCEPTION_CONTINUE_SEARCH )
	{
		return ( rc );
	}

#	if !GP_RETAIL
	if ( xinfo->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT )
#	endif // !GP_RETAIL
	{
		if ( DoesSingletonExist() )
		{
			gAppModule.RequestFatalQuit();
		}

		return ( ::ExceptionDlgFilter( xinfo, lastError, "--==<< Top-level exception filter report >>==--\n" ) );
	}

	return ( EXCEPTION_CONTINUE_SEARCH );
}

LRESULT CALLBACK AppModule :: StaticAppWindowProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
	// special: on creation, set "this" from creation param
	if ( msg == WM_CREATE )
	{
		const CREATESTRUCT* create = (const CREATESTRUCT*)lparam;
		::SetWindowLong( hwnd, GWL_USERDATA, (LONG)create->lpCreateParams );
	}

	// pass it on through
	AppModule* appModule = (AppModule*)::GetWindowLong( hwnd, GWL_USERDATA );
	if ( appModule != NULL )
	{
		return ( appModule->AppWindowProc( hwnd, msg, wparam, lparam ) );
	}
	else
	{
		return ( ::DefWindowProc( hwnd, msg, wparam, lparam ) );
	}
}

LRESULT AppModule :: AppWindowProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
	// optionally trace it
#	if !GP_RETAIL
	if ( TestOptions( OPTION_TRACE_MESSAGES ) )
	{
		MSG wmsg;
		::ZeroObject( wmsg );
		wmsg.hwnd    = hwnd;
		wmsg.message = msg;
		wmsg.wParam  = wparam;
		wmsg.lParam  = lparam;
		winx::TraceMsg( wmsg, true );
	}
#	endif // !GP_RETAIL

	// bail if we're not ready for it yet
	if ( !IsInitialized() )
	{
		return ( ::DefWindowProc( hwnd, msg, wparam, lparam ) );
	}

	// reflect to parent if requested
	if ( TestOptions( OPTION_REFLECT_INPUT_PARENT ) && winx::IsInputMessage( msg ) )
	{
		return ( ::SendMessage( ::GetParent( GetMainWnd() ), msg, wparam, lparam ) );
	}

	// do unicode conversions
	if ( (m_LocMgr != NULL) && !m_LocMgr->ConvertMessageAW( hwnd, msg, wparam, lparam ) )
	{
		return ( 0 );
	}


	// see if anyone else wants it first
	LRESULT lresult = 0;
	if ( m_MessageCbDb != NULL )
	{
		MessageCbDb::iterator i, begin = m_MessageCbDb->begin(), end = m_MessageCbDb->end();
		for ( i = begin ; i != end ; ++i )
		{
			// sanity check
			gpassert(   (hwnd   == m_LastMsg.hwnd)
					 && (msg    == m_LastMsg.message)
					 && (wparam == m_LastMsg.wParam)
					 && (lparam == m_LastMsg.lParam) );

			// try dispatch
			if ( i->second( m_LastMsg, lresult ) )
			{
				// somebody handled it
				return ( lresult );
			}
		}
	}

	// see if subclass wants it
	if ( OnMessage( hwnd, msg, wparam, lparam, lresult ) )
	{
		// guess so...
		return ( lresult );
	}

	// special translations for the way we handle keys
	if ( (msg == WM_KEYDOWN) || (msg == WM_KEYUP) )
	{
		// special: pause key is an extended key
		if ( (wparam == VK_CANCEL) && (lparam & (1 << 24)) )
		{
			wparam = VK_PAUSE;
		}
	}

	// everything is handled by default except in certain special cases
	bool handled = true;

	// $ important note on button qualifiers: we set the down qualifiers
	//   *after* the button has been dispatched for a down message, and reset
	//   them *before* the up message has been dispatched. this makes sure that
	//   the inputs match.

	// crack it ourselves
	switch ( msg )
	{
		case ( WM_CHAR ):
		case ( WM_IME_CHAR ):
		{
			UpdateKeyQualifiers( lparam );

			// this workaround is from kazuyuki, see bug #7810 for reference (to
			// fix IME bug in OfficeXP not sending us CONTROL key up message).
			SetReset( m_CurrentQualifiers, Keys::Q_CONTROL, ::GetKeyState( VK_CONTROL ) & 0x8000 );
			SetReset( m_CurrentQualifiers, Keys::Q_SHIFT, ::GetKeyState( VK_SHIFT ) & 0x8000 );

			if ( !GetControlKey() && !GetSpecialKey() && !m_SkipNextChar )
			{
				Dispatch( (wchar_t)wparam );
			}
			m_SkipNextChar = false;
			OnChar( (wchar_t)wparam, LOWORD( lparam ) );
		}
		break;

		case ( WM_KEYDOWN ):
		{
			m_CurrentQualifiers |= Keys::Q_KEY_DOWN;
			m_SkipNextChar = false;

			UpdateOnKey( wparam, true );
			Dispatch( (Keys::KEY)wparam );
			OnKeyDown( wparam, LOWORD( lparam ), HIWORD( lparam ) );

			SetReset( m_CurrentQualifiers, Keys::Q_KEY_UP | Keys::Q_KEY_DOWN, false );
		}
		break;

		case ( WM_KEYUP ):
		{
			// $ this is super hacka hacka hacka - for some reason Windows does
			//   not send out a WM_KEYDOWN message. so we gotta manufacture one.
			if ( wparam == VK_SNAPSHOT )
			{
				m_CurrentQualifiers |= Keys::Q_KEY_DOWN;
				m_SkipNextChar = false;

				UpdateOnKey( wparam, true );
				Dispatch( (Keys::KEY)wparam );
				OnKeyDown( wparam, LOWORD( lparam ), HIWORD( lparam ) );

				SetReset( m_CurrentQualifiers, Keys::Q_KEY_UP | Keys::Q_KEY_DOWN, false );
			}

			m_CurrentQualifiers |= Keys::Q_KEY_UP;
			UpdateOnKey( wparam, false );

			Dispatch( (Keys::KEY)wparam );
			OnKeyUp( wparam, LOWORD( lparam ), HIWORD(lparam ) );

			SetReset( m_CurrentQualifiers, Keys::Q_KEY_UP | Keys::Q_KEY_DOWN, false );
		}
		break;


		case ( WM_SYSCHAR ):
		{
			if ( !TestOptions( OPTION_CHILD_WND ) && !HasAppMenu() )
			{
				UpdateKeyQualifiers( lparam );
				OnSysChar( (TCHAR)wparam, LOWORD( lparam ) );
			}
			else
			{
				handled = false;
			}
		}
		break;

		case ( WM_SYSKEYDOWN ):
		{
			if ( !TestOptions( OPTION_CHILD_WND ) && !HasAppMenu() )
			{
				UpdateKeyQualifiers( lparam );
				UpdateOnKey( wparam, true );

				// set alt always (required for alt-a etc.)
				m_CurrentQualifiers |= Keys::Q_KEY_DOWN;
				if ( wparam != VK_F10 )
				{
					m_CurrentQualifiers |= Keys::Q_ALT;
				}

				// special: ignore alt-f4
				if ( wparam != VK_F4 )
				{
					bool dispatched = Dispatch( (Keys::KEY)wparam ) != NULL;

					// pass thru F10 to app normally
					if ( wparam == VK_F10 )
					{
						OnKeyDown( wparam, LOWORD( lparam ), HIWORD( lparam ) );
					}
					else
					{
						handled = OnSysKeyDown( wparam, LOWORD( lparam ), HIWORD( lparam ) ) || dispatched || (wparam == VK_MENU);
					}
					SetReset( m_CurrentQualifiers, Keys::Q_KEY_UP | Keys::Q_KEY_DOWN, false );
				}
				else
				{
					handled = false;
				}
			}
			else
			{
				handled = false;
			}
		}
		break;

		case ( WM_SYSKEYUP ):
		{
			if ( !TestOptions( OPTION_CHILD_WND ) && !HasAppMenu() )
			{
				UpdateKeyQualifiers( lparam );
				UpdateOnKey( wparam, false );

				// set alt always (required for alt-a etc.)
				m_CurrentQualifiers |= Keys::Q_KEY_UP;
				if ( wparam != VK_F10 )
				{
					m_CurrentQualifiers |= Keys::Q_ALT;
				}

				Dispatch( (Keys::KEY)wparam );

				if ( wparam == 10 )
				{
					// pass thru F10 to app normally
					OnKeyUp( wparam, LOWORD( lparam ), HIWORD( lparam ) );
				}
				else
				{
					handled = OnSysKeyUp( wparam, LOWORD( lparam ), HIWORD( lparam ) ) || (wparam == VK_MENU);
				}
				SetReset( m_CurrentQualifiers, Keys::Q_ALT | Keys::Q_KEY_UP | Keys::Q_KEY_DOWN, false );
			}
			else
			{
				handled = false;
			}
		}
		break;

		case ( WM_LBUTTONDOWN ):  ProcessMouseButton( false, 0, wparam );  break;
		case ( WM_LBUTTONUP   ):  ProcessMouseButton( true,  0, wparam );  break;
		case ( WM_MBUTTONDOWN ):  ProcessMouseButton( false, 1, wparam );  break;
		case ( WM_MBUTTONUP   ):  ProcessMouseButton( true,  1, wparam );  break;
		case ( WM_RBUTTONDOWN ):  ProcessMouseButton( false, 2, wparam );  break;
		case ( WM_RBUTTONUP   ):  ProcessMouseButton( true,  2, wparam );  break;
		case ( WM_XBUTTONDOWN ):  ProcessMouseButton( false, (HIWORD( wparam ) == XBUTTON1) ? 3 : 4, LOWORD( wparam ) );  break;
		case ( WM_XBUTTONUP   ):  ProcessMouseButton( true,  (HIWORD( wparam ) == XBUTTON1) ? 3 : 4, LOWORD( wparam ) );  break;

		case ( WM_ACTIVATE ):
		{
			// ignore if we're a child or the window is minimized
			if ( !TestOptions( OPTION_CHILD_WND ) && (HIWORD( wparam ) == 0) )
			{
				// make sure the cursor is visible when we're inactive
				if ( LOWORD( wparam ) == WA_INACTIVE )
				{
					while ( ::ShowCursor( TRUE ) < 0 );
				}
				else
				{
					// make sure it's hidden
					while ( ::ShowCursor( FALSE ) >= 0 );

					// now just show it
					if ( !IsFullScreen() )
					{
						::ShowCursor( TRUE );
					}
				}
			}
		}
		break;

		case ( WM_MOUSEMOVE ):
		{
			if ( IsAppActive() )
			{
				// tell derived about it
				OnMouseMove( LOWORD( lparam ), HIWORD( lparam ), wparam );
			}
		}
		break;

		case ( WM_MOUSEWHEEL ):
		{
			int deltaZ = (short)HIWORD( wparam ) / WHEEL_DELTA;
			if ( deltaZ != 0 )
			{
				// get scale to use based on mode
				float scaleZ = TestOptions( OPTION_MOUSE_FIXED ) ? m_FixedMouseScaleZ : m_MouseScaleZ;

				// apply scaling
				int oldDeltaZ = deltaZ;
				deltaZ = (int)(deltaZ * scaleZ);

				// clean up
				NudgeMouse( oldDeltaZ, deltaZ );

				// dispatch
				MouseInput mouse( "dz", m_CurrentQualifiers );
				mouse.SetDelta( deltaZ );
				Dispatch( mouse );

				// specifically send a wheel up/down message as well
				if ( deltaZ > 0 )
				{
					MouseInput wheel( "wheel_up", m_CurrentQualifiers );
					wheel.SetDelta( deltaZ );
					Dispatch( wheel );
				}
				else
				{
					MouseInput wheel( "wheel_down", m_CurrentQualifiers );
					wheel.SetDelta( deltaZ );
					Dispatch( wheel );
				}

				// notify derived
				OnMouseWheel( m_CursorX, m_CursorY, deltaZ, LOWORD( wparam ) );
			}
		}
		break;

		case ( WM_GETMINMAXINFO ):
		{
			if ( !TestOptions( OPTION_CHILD_WND | OPTION_RESIZABLE_WND ) )
			{
				MINMAXINFO* info = rcast <MINMAXINFO*> ( lparam );

				// keep the window from being resized.
				GPoint p = m_WindowRect.Size();
				info->ptMinTrackSize = p;
				info->ptMaxTrackSize = p;
				info->ptMaxSize      = p;

				// in full screen mode, keep the window from being moved
				if ( IsFullScreen() )
				{
					info->ptMaxPosition = m_WindowRect.TopLeft();
				}
			}
			else
			{
				handled = false;
			}
		}
		break;

		case ( WM_CLOSE ):
		{
			// log
#			if GP_PROFILING
			s_Writer.Close();
#			endif

			if ( !TestOptions( OPTION_CHILD_WND ) )
			{
				// if we're minimized, restore
				if ( IsMinimized() )
				{
					::ShowWindow( GetMainWnd(), SW_RESTORE );
					::PostMessage( GetMainWnd(), WM_CLOSE, 0, 0 );
				}
				else if ( RequestQuit() )
				{
					SaveWindowPos();
					handled = false;
				}
			}
			else
			{
				handled = false;
			}
		}
		break;

		case ( WM_DESTROY ):
		{
			m_MainWnd = NULL;

			if ( !m_IsReplacingWnd )
			{
				::PostQuitMessage( 0 );
			}
		}
		break;

		case ( WM_SETCURSOR ):
		{
			// no cursor if we're active but not a child
			if ( !TestOptions( OPTION_CHILD_WND ) && (LOWORD( lparam ) == HTCLIENT) && IsAppActive() )
			{
				::SetCursor( NULL );
			}
			else
			{
				handled = false;
			}
		}
		break;

		case ( WM_MOUSEACTIVATE ):
		{
			// don't let the click go through if it's client area
			if ( LOWORD( lparam ) == HTCLIENT )
			{
				lresult = MA_ACTIVATEANDEAT;
			}
			else
			{
				handled = false;
			}
		}
		break;

		case ( WM_ACTIVATEAPP ):
  		{
			bool lockIt = GP_RETAIL || !TestOptions( OPTION_ALWAYS_ACTIVE );
			if ( lockIt )
			{
				if ( wparam )
				{
					OnRestoreSurfaces();

					// game is activating
					if ( IsFullScreen() )
					{
						::MoveWindow( GetMainWnd(),
									  m_WindowRect.left, m_WindowRect.top,
									  m_WindowRect.Width(), m_WindowRect.Height(),
									  FALSE );
						::ShowWindow( GetMainWnd(), SW_RESTORE );
					}
					AppActivate();
				}
				else
				{
					// game is deactivating
					if ( IsFullScreen() )
					{
						::ShowWindow( GetMainWnd(), SW_MINIMIZE );
					}
					AppDeactivate();
				}
			}
		}
		break;

		case ( WM_SYSCOMMAND ):
		{
			handled = false;

			if ( !TestOptions( OPTION_CHILD_WND ) && !HasAppMenu() )
			{
				int cmdType = wparam & 0xFFF0;
				if ( (cmdType == SC_SCREENSAVE) || (cmdType == SC_SCREENSAVE) )
				{
					// disable screensavers and monitor power downs $$$???necessary???does it even work in the first place???
					handled = true;
				}
			}
		}
		break;

		case ( WM_DISPLAYCHANGE ):
		{
			if ( !IsFullScreen() && IsAppActive() )
			{
				OnRestoreSurfaces();
			}
		}
		break;

	// The following messages aren't processed in full screen mode.

		case ( WM_PAINT ):
		{
			if ( !IsMinimized() && ::IsWindowEnabled( GetMainWnd() ) )
			{
				// get paint context
				PAINTSTRUCT ps;
				::BeginPaint( hwnd, &ps );
				GRect paintRect( ps.rcPaint );

				// check if really anything to paint
				if ( paintRect.IsValid() )
				{
					OnPaint( ps );
				}
				::EndPaint( hwnd, &ps );
			}
			else
			{
				handled = false;
			}
		}
		break;

		case ( WM_NCHITTEST ):
		{
			if ( IsFullScreen() || (TestOptions( OPTION_RESIZABLE_WND ) && (!TestOptions( OPTION_GAME_STYLE_WND ) || (GetAsyncKeyState( VK_MENU ) & (1 << 15)))) )
			{
				handled = false;
			}
			else
			{
				lresult = ::DefWindowProc( hwnd, msg, wparam, lparam );
				switch ( lresult )
				{
					case (HTTOP):
					case (HTTOPLEFT):
					case (HTTOPRIGHT):
					case (HTLEFT):
					case (HTRIGHT):
					case (HTBOTTOM):
					case (HTBOTTOMLEFT):
					case (HTBOTTOMRIGHT):
					{
						// reprocess all sizing areas as no-border areas to kill
						// sizing cursors
						lresult = HTCAPTION;
					}
					break;
				}
			}
		}

		case ( WM_MOVE ):
		{
			// update rects
			if ( !IsFullScreen() && UpdateGeometry() )
			{
				// notify child
				OnMove( GPoint( LOWORD( lparam ), HIWORD( lparam ) ) );
			}
		}
		break;

		case ( WM_MOVING ):
		{
			GRect* rect = (GRect*)lparam;

			if ( IsFullScreen() )
			{
				// keep window at current size & position
				*rect = GetWindowRect();
			}
			else
			{
				// prevent window from being dragged off the screen
				if ( TestOptions( OPTION_SCREEN_CONSTRAIN_WND ) && !(GetAsyncKeyState( VK_MENU ) & (1 << 15)) )
				{
					rect->ConstrainTo( winx::MakeVirtualScreenRect() );
				}
			}
		}

		case ( WM_SIZE ):
		{
			bool resize = !IsFullScreen();
			switch ( wparam )
			{
				case ( SIZE_MINIMIZED ):
				{
					AppDeactivate();
					resize = false;
				}
				break;

				case ( SIZE_RESTORED ):
				case ( SIZE_MAXIMIZED ):
				{
					AppActivate();
				}
				break;
			}

			if ( resize )
			{
				// update rects
				UpdateGeometry();

				// notify child
				OnSize( GSize( LOWORD( lparam ), HIWORD( lparam ) ) );
			}
		}
		break;

		case ( WM_ENTERSIZEMOVE ):
		{
			m_IsMovingSizing = true;
			m_WasResized = false;
		}
		break;

		case ( WM_EXITSIZEMOVE ):
		{
			m_IsMovingSizing = false;

			if ( m_WasResized )
			{
				m_WasResized = false;
				OnScreenChange();
			}
		}
		break;

		case ( WM_COMMAND ):
		{
			OnCommand( LOWORD( wparam ) );
		}
		break;

		default:
		{
			if ( msg == winx::GetLegacyMouseWheelMessage() )
			{
				// $ this is copied from MFC's WINFRM.CPP

				// convert MSH_MOUSEWHEEL to WM_MOUSEWHEEL. this is necessary
				// for win95 systems that use intellimouse drivers.
				WORD keyState = 0;
				keyState |= (::GetKeyState( VK_CONTROL ) < 0) ? MK_CONTROL : 0;
				keyState |= (::GetKeyState( VK_SHIFT   ) < 0) ? MK_SHIFT   : 0;

				HWND hwFocus = ::GetFocus();
				const HWND hwDesktop = ::GetDesktopWindow();

				if ( hwFocus == NULL )
				{
					lresult = ::SendMessage( hwnd, WM_MOUSEWHEEL, (wparam << 16) | keyState, lparam );
				}
				else
				{
					do
					{
						lresult = ::SendMessage( hwFocus, WM_MOUSEWHEEL, (wparam << 16) | keyState, lparam );
						hwFocus = ::GetParent( hwFocus );
					}
					while ( (lresult == 0) && (hwFocus != NULL) && (hwFocus != hwDesktop) );
				}
			}
			else if ( msg == winx::GetQueryCancelAutoPlayMessage() )
			{
				// always cancel any autoplay stuff that comes up while active
				// and full screen (it's ok to be windowed or minimized)
				if ( IsAppActive() && IsFullScreen() )
				{
					lresult = winx::CANCEL_AUTO_PLAY;
				}
				else
				{
					// nah... let it come up.
					handled = false;
				}
			}
			else
			{
				// default processing
				handled = false;
			}
		}
	}

	// if not handled, call def proc
	if ( !handled )
	{
		lresult = ::DefWindowProc( hwnd, msg, wparam, lparam );
	}

	return ( lresult );
}

gpstring AppModule :: MakeUniqueName( const char* prefix )
{
	// start with default
	gpstring uniqueName( prefix ? prefix : "" );

	// use module name as postfix
	gpstring moduleName = FileSys::GetModuleFileName();
	const char* fileName = FileSys::GetFileName( moduleName );
	if ( fileName != NULL )
	{
		// append filename to default
		uniqueName += fileName;

		// replace wacky chars with underscore
		gpstring::iterator i, begin = uniqueName.begin_split(), end = uniqueName.end_split();
		for ( i = begin ; i != end ; ++i )
		{
			if ( !::isalnum( *i ) )
			{
				*i = '_';
			}
		}
	}
	else
	{
		// failed for whatever reason, use a default name
		uniqueName += "Default";
	}

	// just in case it's case-sensitive (mutexes are at least)
	uniqueName.to_lower();

	// ok
	return ( uniqueName );
}

bool AppModule :: UpdateCursor( bool centerMouse )
{

// Check state.

	bool center = false;
	if ( IsAppActive() )
	{
		if ( IsFullScreen() && !TestOptions( OPTION_ALWAYS_ACTIVE ) )
		{
			center = true;
		}
		else if ( m_CurrentQualifiers & Keys::Q_ANY_BUTTON )
		{
			center = true;
		}
		else if ( TestOptions( OPTION_MOUSE_FIXED ) )
		{
			center = true;
		}
	}

// Update cursor position.

	if ( center && centerMouse )
	{
		// set new center
		m_CursorCenterX = m_GameRect.Width () / 2;
		m_CursorCenterY = m_GameRect.Height() / 2;

		// calc and set center
		::SetCursorPos( m_GameRect.left + m_CursorCenterX,
						m_GameRect.top  + m_CursorCenterY );
	}

	return ( center );
}

void AppModule :: UpdateNormalizedCursor( int cursorX, int cursorY )
{
	m_NormalizedCursorX = float( cursorX - (GetGameWidth() / 2) )
						/ float( GetGameWidth() / 2 );
	m_NormalizedCursorY = float( (GetGameHeight() / 2) - cursorY )
						/ float( GetGameHeight() / 2 );
}

bool AppModule :: OnUpdate( void )
{
	// flush prefs if they have changed
	gConfig.FlushChanges();

	// fixed frame rate?
	if ( TestOptions( OPTION_FIXED_FRAME_RATE ) )
	{
		// get how much time has passed
		double currentDelta = PreciseSubtract( ::GetGlobalSeconds(), m_AbsoluteTime );

		// here is how much we need to adjust it by
		double globalAdjust = PreciseSubtract( PreciseDivide(1.0, m_FixedFrameRate), currentDelta);

		// do it
		::SetGlobalSecondsAdjust( PreciseAdd(::GetGlobalSecondsAdjust(), globalAdjust) );
	}

	// get our frame time delta and advance
	double lastFrame = m_AbsoluteTime;
	m_AbsoluteTime = ::GetGlobalSeconds();
	m_DeltaTime = scast <float> (PreciseSubtract( m_AbsoluteTime, lastFrame ));

	// a negative delta will occur if the global clock was adjusted... in this rare case, just skip the update. -bk
	if( m_DeltaTime <= 0.0 )
	{
		gpwarning( "*** skipped frame update due to zero DeltaTime\n" );
		return ( true );
	}

	// this one will not be affected by speed bias
	m_ActualDeltaTime = m_DeltaTime;

	// check for min frame rate
	if ( (m_MinFrameRate > 0) && TestOptions( OPTION_ENABLE_MINFPS ) )
	{
		const float MAXIMUM_INTERVAL_LENGTH = 1.0f / m_MinFrameRate;

		if ( m_DeltaTime > MAXIMUM_INTERVAL_LENGTH )
		{
			// ok so frames are taking too long to render. let's stick with the
			// minimum frame rate, ok?
			m_DeltaTime = MAXIMUM_INTERVAL_LENGTH;
			m_ActualDeltaTime = m_DeltaTime;

			// correct the last frame time to hide the debug delay time reduction
			lastFrame = PreciseSubtract( m_AbsoluteTime, m_ActualDeltaTime );
		}
	}

	// check for debug pass time
	if (   (m_DeltaTime > 3.0)
		&& TestOptions( OPTION_FREEZE_TIME_DETECT )
		&& !TestOptions( OPTION_FIXED_FRAME_RATE )
		&& SysInfo::IsDebuggerPresent() )
	{
		// the assumption here is that if a frame took more than 3 seconds, you
		// must have broken into the debugger and are now coming back.
		m_DeltaTime = 1.0f / 30.0f;
		gpgenericf(( "Auto-adjusted time delta for debug resume from %f to %f.\n",
					 m_ActualDeltaTime, m_DeltaTime ));
		m_ActualDeltaTime = m_DeltaTime;

		// correct the last frame time to hide the debug delay time reduction
		lastFrame = PreciseSubtract( m_AbsoluteTime, m_ActualDeltaTime );
	}

	// must have positive time
	gpassert( m_ActualDeltaTime > 0 );

	// throttle frame rate when windowed or minimally active and alt-tabbed away?
	if (   ( IsFullScreen() && TestOptions( OPTION_THROTTLE_FULLSCREEN ))
		|| (!IsFullScreen() && TestOptions( OPTION_THROTTLE_WINDOWED ))
		|| (!IsAppActive () && TestOptions( OPTION_ALWAYS_MINIMALLY_ACTIVE )) )
	{
		float fps = IsAppActive() ? m_ThrottleFrameRate : MINIMALLY_ACTIVE_FPS;

		const float MINIMUM_INTERVAL_LENGTH = 1.0f / fps;

		if ( m_DeltaTime < MINIMUM_INTERVAL_LENGTH )
		{
			if( IsAppActive() )
			{
				// we are going too fast!, sleep for the remainder of the interval
				::AccurateSleep( MINIMUM_INTERVAL_LENGTH - m_DeltaTime );
			}
			else
			{
				Sleep( FTOL( (MINIMUM_INTERVAL_LENGTH - m_DeltaTime) * 1000.0f ) );
			}

			// reassign
			m_AbsoluteTime = ::GetGlobalSeconds();

			// recalculate the delta time, now that we have slept for some period of time
			m_DeltaTime = m_ActualDeltaTime = scast <float> (PreciseSubtract( m_AbsoluteTime, lastFrame ));
		}
	}

	// update instantaneous frame rate
	if ( m_ActualDeltaTime != 0 )
	{
		m_InstantFrameRate = 1.0f / m_ActualDeltaTime;
	}

	// update filtered frame rate
	{
		const float FILTER = 0.7f;		// this controls ratio of new to old (raise it to reduce frame rate impact of old on new)
		const float UPDATE = 0.5f;		// how frequently to update the frame rate (also the sample period)

		static float s_DeltaTime = 0;
		static int   s_NumFrames = 0;

		++s_NumFrames;
		s_DeltaTime += m_ActualDeltaTime;
		if ( s_DeltaTime > UPDATE )
		{
			// update frame rate
			m_FilteredFrameRate = ((1.0f - FILTER) * m_FilteredFrameRate) + (FILTER * (s_NumFrames / s_DeltaTime));

			// reset sample data
			s_DeltaTime = 0;
			s_NumFrames = 0;
		}
	}
  	// add frame delta if we are going to do an enduser fpu log
	if ( TestOptionsEq( OPTION_SHOULD_LOG_FPS | OPTION_ENABLE_LOG_FPS ) )
	{
		m_FrameDeltaBuffer.push_back( m_ActualDeltaTime );
	}

// Non-paused frame update.

	// profiler update
#	if GP_ENABLE_PROFILING
	gQuantifyApi.AddFrame();
#	endif // GP_ENABLE_PROFILING
#	if GP_PROFILING && GP_ENABLE_STATS
	gQMemApi.AddFrame();
#	endif // GP_PROFILING && GP_ENABLE_STATS

	// stats update
	GPSTATS_SAMPLE( AddFrame() );

	// begin the frame
	if ( !OnBeginFrame() )
	{
		return ( false );
	}

	// update the cursor
	FrameUpdateCursor( &m_CursorX, &m_CursorY, true );

	// pass time along to derived
	if ( !OnSysFrame( m_DeltaTime, m_ActualDeltaTime ) )
	{
		return ( false );
	}

	// and hooks
	if ( m_SysFrameCbDb != NULL )
	{
		FrameCbDb::iterator i, begin = m_SysFrameCbDb->begin(), end = m_SysFrameCbDb->end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( !i->second( m_DeltaTime, m_ActualDeltaTime ) )
			{
				return ( false );
			}
		}
	}

	// advance frame
	++m_SysFrame;

// Adjust time for paused systems.

	if ( IsSingleStep() )
	{
		// fixed delta
		m_DeltaTime = m_SingleStepTime;
	}
	else if ( IsTimeFrozen() || IsUserPaused() )
	{
		// no delta
		m_DeltaTime = 0;
	}
	else
	{
		// scale game time
		m_DeltaTime *= m_TimeMultiplier;
	}

	// pass time along to derived
	OnFrame( m_DeltaTime, m_ActualDeltaTime );

	// and hooks
	if ( m_FrameCbDb != NULL )
	{
		FrameCbDb::iterator i, begin = m_FrameCbDb->begin(), end = m_FrameCbDb->end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( !i->second( m_DeltaTime, m_ActualDeltaTime ) )
			{
				return ( false );
			}
		}
	}

// Cleanup.

	// advance frame
	++m_Frame;

	// end the frame
	if ( !OnEndFrame() )
	{
		return ( false );
	}

	// reset this if set for next time
	ClearSingleStep();

	// done
	return ( true );
}

bool AppModule :: PrivateInitCore( void )
{
	// register messagebox and debugger callbacks
	gMessageBoxContext.ClearSink();
	gMessageBoxContext.SetSink( new ReportSys::CallbackSink( makeFunctor( MessageBoxCb ), "MessageBox", "MessageBox" ), true );
	gErrorBoxContext.ClearSink();
	gErrorBoxContext.SetSink( new ReportSys::CallbackSink( makeFunctor( ErrorBoxCb ), "ErrorBox", "ErrorBox" ), true );

	// setup our main thread in registry
	kerneltool::Thread::RegisterCurrentThread( "Main" );

	// configure contexts
	bool errorReporting = !TestOptions( OPTION_NO_ERROR_MESSAGES );
	bool perfReporting = !TestOptions( OPTION_NO_PERF_MESSAGES );
	gConfig.Read( "report_errors", errorReporting, errorReporting );
  	gConfig.Read( "report_perfs", perfReporting, errorReporting );
	if ( gConfig.GetBool( "demo", false ) || TestOptions( OPTION_IS_EXTERNAL_PROFILING ) )
	{
		errorReporting = perfReporting = false;
	}
	ReportSys::EnableContext( &gWarningContext, errorReporting );
	ReportSys::EnableContext( &gErrorContext, errorReporting );
	ReportSys::EnableContext( &gPerfContext, perfReporting );

	// test mode?
	if ( gConfig.GetBool( "test", false ) )
	{
		SetOptions( OPTION_TEST_MODE );
	}

	// test options
	if ( TestOptions( OPTION_TEST_MODE ) )
	{
		SetOptions( OPTION_HANG_BUDDY );
	}

	// set up localization
	if ( TestOptions( OPTION_LOCALIZE ) && Config::DoesSingletonExist() && !gConfig.GetBool( "noloc", false ) )
	{
		// build it
		m_LocMgr = new LocMgr;
		if ( m_LocMgr->Init() )
		{
			m_LocMgr->LoadExeTranslations();
		}
		else
		{
			Delete( m_LocMgr );
		}
	}

	// if we're profiling, kill the hang buddy so it doesn't skew results
	if ( TestOptions( OPTION_IS_EXTERNAL_PROFILING ) )
	{
		ClearOptions( OPTION_HANG_BUDDY );
	}

	// set up our hang buddy
	if ( TestOptions( OPTION_HANG_BUDDY ) )
	{
		static const DWORD DUMP_KEYS [] = {  VK_SHIFT, VK_CONTROL, 'H'  };
		static const DWORD NUKE_KEYS [] = {  VK_SHIFT, VK_CONTROL, VK_BACK  };
		static const DWORD DEBUG_KEYS[] = {  VK_SHIFT, VK_CONTROL, 'B'  };

		static const DWORD HANG_WAKEUP_DELAY_MSEC = 250;

		// build it
		m_HangBuddy = new HangBuddy(
			DUMP_KEYS, ELEMENT_COUNT( DUMP_KEYS ),
			NUKE_KEYS, ELEMENT_COUNT( NUKE_KEYS ),
			DEBUG_KEYS, ELEMENT_COUNT( DEBUG_KEYS ),
			HANG_WAKEUP_DELAY_MSEC );
	}

	// set up throttle
	if ( gConfig.Read( "maxfps", m_ThrottleFrameRate ) )
	{
		SetOptions( OPTION_THROTTLE_FULLSCREEN, m_ThrottleFrameRate != 0 );
		SetOptions( OPTION_THROTTLE_WINDOWED  , m_ThrottleFrameRate != 0 );
	}

	// adjust for min
	if ( m_ThrottleFrameRate <= 0 )
	{
		m_ThrottleFrameRate = 75.0f;
	}

	// set up fixed frame rate
	if ( gConfig.Read( "fixedfps", m_FixedFrameRate ) )
	{
		SetOptions( OPTION_FIXED_FRAME_RATE );
	}

	// set up frame log
	if ( gConfig.GetBool( "fpslog", false ) )
	{
		SetOptions( OPTION_SHOULD_LOG_FPS );
	}

	// special support for no-deactivate
	bool alwaysActive = false;
	gConfig.Read( "always_active", alwaysActive );
	SetOptions( OPTION_ALWAYS_ACTIVE, alwaysActive );

	// special support for always on top
	bool alwaysOnTop = false;
	if ( gConfig.Read( "always_on_top", alwaysOnTop ) )
	{
		SetOptions( OPTION_ALWAYS_ON_TOP_WINDOWED,   alwaysOnTop );
		SetOptions( OPTION_ALWAYS_ON_TOP_FULLSCREEN, alwaysOnTop );
	}
	else if ( SysInfo::IsDebuggerPresent() )
	{
		SetOptions( OPTION_ALWAYS_ON_TOP_WINDOWED,   false );
		SetOptions( OPTION_ALWAYS_ON_TOP_FULLSCREEN, false );
	}

	// ok
	return ( true );
}

bool AppModule :: PrivateInitWindow( void )
{
	// set the console title if there is one
	if ( !m_AppName.empty() )
	{
		::SetConsoleTitle( ::ToAnsi( m_AppName ) );
	}

	// bail early if we're not making a window
	if ( !TestOptions( OPTION_CREATE_MAINWND ) )
	{
		return ( true );
	}

// Get full screen flag.

	// default is from options
	m_FullScreen = TestOptions( OPTION_FULLSCREEN );

	// get fullscreen flag (only when not a child window)
	if ( !m_IsReplacingWnd && !TestOptions( OPTION_CHILD_WND ) && Config::DoesSingletonExist() )
	{
		m_FullScreen = gConfig.GetBool( "fullscreen", m_FullScreen );
	}

	// make sure derived agrees (may not be able to do it in the mode user wants)
	bool fullscreen = m_FullScreen;
	bool modeFail = !OnGetFullScreen( fullscreen );
	if ( !modeFail )
	{
		// make sure we are in the mode we expect
		if ( TestOptions( OPTION_CHILD_WND ) && fullscreen )
		{
			// can't run in windowed mode, yet we require it (probably a
			// voodoo 2) so must fail
			modeFail = true;
		}
		else
		{
			// ok, accept the new mode
			m_FullScreen = fullscreen;
		}
	}

	// get screen and virtual screen rects
	GRect screenRect  = winx::MakeScreenRect();
	GRect vscreenRect = winx::MakeVirtualScreenRect();

	// get size
	if ( m_StartupRect.IsZero() )
	{
		if ( Config::DoesSingletonExist() )
		{
			m_GameRect.right  = gConfig.GetInt( "width",  640 );
			m_GameRect.bottom = gConfig.GetInt( "height", 480 );
		}
		else
		{
			m_GameRect.right  = 640;
			m_GameRect.bottom = 480;
		}
	}
	else
	{
		m_GameRect.right  = m_StartupRect.Width();
		m_GameRect.bottom = m_StartupRect.Height();
	}

	// verify it will fit on the screen
	if ( !m_FullScreen )
	{
		GRect rect( m_GameRect );
		if ( ::AdjustWindowRect( &rect, CalcWindowStyle(), HasAppMenu() ) )
		{
			// will it fit?
			if ( (rect.Width() > screenRect.Width()) || (rect.Height() > screenRect.Height()) )
			{
				if ( TestOptions( OPTION_GAME_STYLE_WND ) )
				{
					// nope, gotta go full screen
					m_FullScreen = true;
					gpmessageboxf(( $MSG$ "In order to run %S in windowed mode, the desktop resolution (currently %dx%d) must be set "
									"higher than the startup resolution (%dx%d) required by the game. "
									"(Note that windowed mode uses slightly larger area than fullscreen due to the GDI title and border).\n\n"
									"To work around this problem, you can either raise the resolution of the desktop, or run full screen.\n\n"
									"Now switching to fullscreen mode...",
									GetAppName().c_str(), screenRect.Width(), screenRect.Height(),
									m_GameRect.Width(), m_GameRect.Height(), GetAppName().c_str() ));
				}
				else
				{
					// constrain instead
					m_GameRect.right  = rect.left + screenRect.Width() - (rect.Width() - m_GameRect.Width());
					m_GameRect.left   = rect.left;
					m_GameRect.bottom = rect.top + screenRect.Height() - (rect.Height() - m_GameRect.Height());
					m_GameRect.top    = rect.top;
				}
			}
		}
	}

	// let derived adjust it
	GSize gameSize( m_GameRect.Size() );
	if ( !OnGetGameSize( gameSize ) )
	{
		return ( false );
	}
	m_GameRect.ResizeTo( gameSize );

	// report failure
	if ( modeFail )
	{
		if ( IsFullScreen() )
		{
			gpfatal( $MSG$ "Unable to run in full screen mode." );
		}
		else
		{
			gpfatal( $MSG$ "Unable to run in windowed mode." );
		}
	}

// Register window class.

	WNDCLASS wndClass;
	wndClass.style          = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc    = StaticAppWindowProc;
	wndClass.cbClsExtra		= 0;
	wndClass.cbWndExtra		= 0;
	wndClass.hInstance		= GetInstance();
	wndClass.hIcon			= m_AppIcon;
	wndClass.hCursor		= ::LoadCursor( NULL, IDC_WAIT );
	wndClass.hbrBackground	= (HBRUSH)::GetStockObject( BLACK_BRUSH );
	wndClass.lpszMenuName	= NULL;
	wndClass.lpszClassName	= m_WndClassName;

	// register it
	if ( !::RegisterClass( &wndClass ) )
	{
		gpwatsonf(( "Could not register WNDCLASS '%s', error is 0x%08X ('%s').",
  				    m_WndClassName.c_str(), ::GetLastError(), stringtool::GetLastErrorText().c_str() ));
	}

	// build our style for the main window
	DWORD style = CalcWindowStyle();

// Optionally create main window.

	// create the window right now if we're fullscreen. black out the screen as
	// fast as possible so users knows we're "busy". helpful during res switch
	// testing for d3d rasterizer.
	//
	// $$$ issue: what if user changes res upwards for game? like they run the
	//     desktop at 800x600 but the game at 1024x768. test this. may need to
	//     resize the full screen window on a res change upwards
	if ( IsFullScreen() )
	{
		// set rects
		m_WindowRect = m_ClientRect = screenRect;

		// create main window
		m_MainWnd = ::CreateWindowEx( TestOptions( OPTION_ALWAYS_ON_TOP_FULLSCREEN ) ? WS_EX_TOPMOST : 0,
									  m_WndClassName,
									  ::ToAnsi( GetAppName() + m_AppTitleExtra ),
									  style,
									  m_WindowRect.left, m_WindowRect.top,
									  m_WindowRect.Width(), m_WindowRect.Height(),
									  NULL, m_AppMenu, GetInstance(), this );

		// check for failure
		if ( m_MainWnd == NULL )
		{
			gpfatalf(( $MSG$ "Unable to create main window in full screen mode, error is 0x%08X ('%s').",
					   ::GetLastError(), stringtool::GetLastErrorText().c_str() ));
		}
	}

// Get main window rect.

	// get origin
	GPoint origin( GPoint::ZERO );
	if ( !IsFullScreen() )
	{
		if ( m_StartupRect.TopLeft() == GPoint::ZERO )
		{
			if ( Config::DoesSingletonExist() )
			{
				// get positioning
				if ( !gConfig.Read( "x", origin.x, CW_USEDEFAULT ) )
				{
					if ( gConfig.GetString( "x" ).same_no_case( "center" ) )
					{
						origin.x = (screenRect.Width() / 2) - (gameSize.cx / 2);
					}
				}
				if ( !gConfig.Read( "y", origin.y, CW_USEDEFAULT ) )
				{
					if ( gConfig.GetString( "y" ).same_no_case( "center" ) )
					{
						origin.y = (screenRect.Height() / 2) - (gameSize.cy / 2);
					}
				}
			}
		}
		else
		{
			origin.x = m_StartupRect.left;
			origin.y = m_StartupRect.top;
		}
	}

	// adjust and constrain to virtual screen
	if ( !IsFullScreen() )
	{
		// take the size
		m_ClientRect.ResizeTo( gameSize );
		m_WindowRect.ResizeTo( gameSize );

		// adjust for window components
		if ( ::AdjustWindowRect( &m_WindowRect, style, HasAppMenu() ) )
		{
			// adjust and constrain if not a child
			if ( !TestOptions( OPTION_CHILD_WND ) )
			{
				if ( origin.x != CW_USEDEFAULT )
				{
					origin.x += m_WindowRect.left;
					if ( origin.x < vscreenRect.left )
					{
						origin.x = vscreenRect.left;
					}
					if ( (origin.x + m_WindowRect.Width()) > vscreenRect.right )
					{
						origin.x = vscreenRect.right - m_WindowRect.Width();
					}
				}
				if ( origin.y != CW_USEDEFAULT )
				{
					origin.y += m_WindowRect.top;
					if ( origin.y < vscreenRect.top )
					{
						origin.y = vscreenRect.top;
					}
					if ( (origin.y + m_WindowRect.Height()) > vscreenRect.bottom )
					{
						origin.y = vscreenRect.bottom - m_WindowRect.Height();
					}
				}
			}
		}
	}

// Create main window.

	// already created it if full screen
	if ( !IsFullScreen() )
	{
		// create it
		m_MainWnd = ::CreateWindowEx( TestOptions( OPTION_ALWAYS_ON_TOP_WINDOWED ) ? WS_EX_TOPMOST : 0,
									  m_WndClassName,
									  ::ToAnsi( GetAppName() + m_AppTitleExtra ),
									  style,
									  origin.x, origin.y,
									  m_WindowRect.Width(), m_WindowRect.Height(),
									  m_ParentWnd, m_AppMenu, GetInstance(), this );

		// check for failure
		if ( m_MainWnd == NULL )
		{
			gpfatalf(( $MSG$ "Unable to create main window in windowed mode, error is 0x%08X ('%s').",
					   ::GetLastError(), stringtool::GetLastErrorText().c_str() ));
		}
	}

	// set the icons
	if ( m_AppIcon != NULL )
	{
		::SendMessage( GetMainWnd(), WM_SETICON, ICON_BIG,   LPARAM( m_AppIcon ) );
		::SendMessage( GetMainWnd(), WM_SETICON, ICON_SMALL, LPARAM( m_AppIcon ) );
	}

	// now update rects
	UpdateGeometry();

	// check some options
	SetOptions( OPTION_DISABLE_TASK_SWITCH, gConfig.GetBool( "noalttab", false ) );
	SetOptions( OPTION_DISABLE_WIN_KEYS,    gConfig.GetBool( "nowinkeys", false ) );

	// done
	return ( true );
}

bool AppModule :: PrivateInitServices( void )
{
	// just center the cursor if fullscreen
	if ( IsFullScreen() )
	{
		m_CursorX = GetGameWidth () / 2;
		m_CursorY = GetGameHeight() / 2;

		::SetCursorPos( m_GameRect.left + m_CursorX, m_GameRect.top + m_CursorY );
	}

	return ( true );
}

bool AppModule :: PrivateExecute( void )
{

// Preliminary setup.

	// set priority initially
	UpdatePriorityClass();

	// reset cursor in window to normal now that we're initialized
	if ( (m_Frame == 0) && TestOptions( OPTION_CREATE_MAINWND ) )
	{
		if ( TestOptions( OPTION_CHILD_WND ) )
		{
			::SetClassLong( GetMainWnd(), GCL_HCURSOR, NULL );
		}
		else
		{
			::SetClassLong( GetMainWnd(), GCL_HCURSOR, (LONG)::LoadCursor( NULL, IDC_ARROW ) );
		}
	}

	// do derived
	if ( !OnExecute() )
	{
		return ( false );
	}

	// bail early if we're not pumping messages or executing frames
	if ( !TestOptions( OPTION_MESSAGE_PUMP ) && !TestOptions( OPTION_EXECUTE_FRAME_ONLY ) )
	{
		return ( true );
	}

	// init some timing vars first time through
	if ( m_Frame == 0 )
	{
		m_AbsoluteTime = PreciseSubtract( ::GetGlobalSeconds(), 0.01 );		// guarantee a 0.01 delta first frame
	}

// Message pump.

	// cursor must be hidden
	if ( HasMainWnd() && IsFullScreen() )
	{
		::ShowCursor( FALSE );
	}

	// repeat until asked to quit
	while ( !GetRequestQuit() )
	{
		// process reset messages that might have been set during update
		if ( m_ReqKeyboardReset || m_ReqMouseReset )
		{
			PrivateResetUserInputs();
		}

		// process windows messages
		if ( TestOptions( OPTION_MESSAGE_PUMP ) )
		{
			MSG msg;
			while ( ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
			{
				// quit message
				if ( msg.message == WM_QUIT )
				{
					m_ReturnValue = msg.wParam;
					RequestQuit( true );
					break;
				}

				// check for accelerator message or modeless dialog message before
				// dispatching to our window proc
				if (   ((m_AppAccel == NULL) || !::TranslateAccelerator( msg.hwnd, m_AppAccel, &msg ))
					&& !OnPreTranslateMessage( msg ) )
				{
					::TranslateMessage( &msg );
					m_LastMsg = msg;
					::DispatchMessage( &msg );
				}

				// process reset messages that might have been set during dispatch
				if ( m_ReqKeyboardReset || m_ReqMouseReset )
				{
					PrivateResetUserInputs();
				}
			}
		}

		// with all messages processed, we can clear the cursor buffer
		{
			kerneltool::Critical::Lock autoLock( *m_MouseCritical );
			m_CursorBuffer.clear();
		}

// Time update.

		// check state
		bool doUpdate = IsAppActive() && OnCheckFocus();	// make sure we're the active app and let derived see if we have focus (test coop level etc)

		// special case for active
		if ( !doUpdate && TestOptions( OPTION_ALWAYS_MINIMALLY_ACTIVE ) )
		{
			doUpdate = true;
		}

		// all messages processed - give engine some time (unless we're shutting down)
		if (   doUpdate													// do we want updates now?
			&& !GetRequestQuit()										// unless we're shutting down
			&& (!HasMainWnd() || ::IsWindowEnabled( GetMainWnd() )) )	// and we must either not have a window or if we have one don't update while disabled
		{
			// do the update
			if ( !OnUpdate() )
			{
				return ( false );
			}

			// abort if we're only doing one frame at a time
			if ( TestOptions( OPTION_EXECUTE_FRAME_ONLY ) )
			{
				break;
			}
		}
		else
		{
			// convert update rate to period, default to infinite wait
			DWORD period = INFINITE;
			if ( ::IsPositive( m_InactiveUpdateRate ) )
			{
				period = (DWORD)(1000.0f / m_InactiveUpdateRate);
			}

			// the game is inactive (user has alt-tabbed away probably). wait
			// for more messages to come in or a request to shut down before
			// continuing. this prevents 100% cpu usage while game is in the
			// background.
			HANDLE stop[ 2 ];
			stop[ 0 ] = m_RequestQuit->GetValid();
			stop[ 1 ] = m_RequestFatalQuit->GetValid();
			DWORD rc = ::MsgWaitForMultipleObjects( 2, stop, FALSE, period, QS_ALLINPUT );

			// provide an inactive update if we timed out
			if ( rc == WAIT_TIMEOUT )
			{
				if ( !OnInactiveUpdate() )
				{
					return ( false );
				}
			}
		}
	}

	// finish up
	if ( HasMainWnd() && GetRequestQuit() )
	{
		// derived clear the screen
		OnClearScreen();

		// paint the game screen black
		HDC dc = ::GetDC( GetMainWnd() );
		if ( dc != NULL )
		{
			::FillRect( dc, m_ClientRect.SizeRect(), (HBRUSH)::GetStockObject( BLACK_BRUSH ) );
			::ReleaseDC( GetMainWnd(), dc );
		}

		// set cursor to C_WAIT and show it while game shuts down
		HCURSOR cursor = ::LoadCursor( NULL, IDC_WAIT );
		::SetClassLong( GetMainWnd(), GCL_HCURSOR, (LONG)cursor );
		::SetCursor( cursor );
		::ShowCursor( TRUE );
	}

	// test for fatal exit, report if not reported already
	bool hasFatal = m_RequestFatalQuit->IsSignaled();
	if ( hasFatal && !ReportSys::IsFatalOccurring() )
	{
		gperrorboxf(( $MSG$ "A fatal error has occurred and %S must shut down.", GetAppName().c_str() ));
	}

	// done
	return ( !hasFatal );
}

bool AppModule :: PrivateKillWindow( void )
{
	// persist if we can
	SaveWindowPos();

	// destroy the window if it remains
	if ( ::IsWindow( m_MainWnd ) )
	{
		::DestroyWindow( m_MainWnd );
	}

	// unregister our window class
	::UnregisterClass( m_WndClassName, m_Instance );

	// we're ok
	return ( true );
}

bool AppModule :: PrivateShutdown( void )
{
	// kill the binder if any
	Delete( m_GlobalBinder );
	Delete( m_InputBinderDb );

	// remove two seconds from the front to let it settle
	{
		FloatBuffer::iterator i, ibegin = m_FrameDeltaBuffer.begin(), iend = m_FrameDeltaBuffer.end();
		double offset = 0;
		for ( i = ibegin ; i != iend ; ++i )
		{
			offset += *i;
			if ( offset > 2.0 )
			{
				break;
			}
		}
		m_FrameDeltaBuffer.erase( m_FrameDeltaBuffer.begin(), i );
	}

	// remove two seconds from the back to clean out back-end crap
	{
		FloatBuffer::reverse_iterator i, ibegin = m_FrameDeltaBuffer.rbegin(), iend = m_FrameDeltaBuffer.rend();
		double offset = 0;
		for ( i = ibegin ; i != iend ; ++i )
		{
			offset += *i;
			if ( offset > 2.0 )
			{
				break;
			}
		}
		m_FrameDeltaBuffer.erase( i.base(), m_FrameDeltaBuffer.end() );
	}

	// dump frame stats
	if ( !m_FrameDeltaBuffer.empty() )
	{
		// gather some stats
		float minDelta = FLOAT_MAX, maxDelta = 0;
		double total = 0;
		FloatBuffer::iterator i, ibegin = m_FrameDeltaBuffer.begin(), iend = m_FrameDeltaBuffer.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			maximize( maxDelta, *i );
			minimize( minDelta, *i );
			total += *i;
		}
		double avgDelta = total / m_FrameDeltaBuffer.size();

		// build context if none
		ReportSys::LogFileSink <> * sink = new ReportSys::LogFileSink <> ( "fps.log" );
		sink->DisablePrefix();
		ReportSys::LocalContext ctx( sink, true, "FPS Log", "Log" );
		ReportSys::AutoReport autoReport( &ctx );

		// output header
		ctx.OutputF( "FPS Summary\n"
					 "-----------\n"
					 "\n"
					 "Total Sample Time  : %.2f s (%d frames)\n"
					 "Min Frame Delta    : %.2f ms (%.2f fps)\n"
					 "Max Frame Delta    : %.2f ms (%.2f fps)\n"
					 "Average Frame Delta: %.2f ms (%.2f fps)\n"
					 "\n"
					 "-----------\n"
					 "\n",
					 total, m_FrameDeltaBuffer.size(),
					 minDelta * 1000.0f, (minDelta == 0) ? 0 : (1.0f / minDelta),
					 maxDelta * 1000.0f, (maxDelta == 0) ? 0 : (1.0f / maxDelta),
					 avgDelta * 1000.0f, (avgDelta == 0) ? 0 : (1.0f / avgDelta) );

		// output deltas
		for ( i = ibegin ; i != iend ; ++i )
		{
			ctx.OutputF( "%f\n", *i );
		}
		ctx.OutputEol();
	}

	// clean out any x handler we installed otherwise if an exception fires
	// during shutdown of static dtors it will end up recursing with new AV's
	// because statics that dialog depends on will be gone.
	if ( s_OldExceptionFilter != NULL )
	{
		::SetUnhandledExceptionFilter( s_OldExceptionFilter );
	}

	// we're ok
	return ( true );
}

void AppModule :: ProcessMouseButton( bool up, int button, WPARAM wparam )
{
	static const MouseDispatch s_MouseDispatch[ 5 ] =
	{
		{  "left_up"         , &AppModule::OnLButtonUp       ,
		   "left_down"       , &AppModule::OnLButtonDown     ,
		   "left_dbl_click"  , &AppModule::OnLButtonDblClick ,  },
		{  "middle_up"       , &AppModule::OnMButtonUp       ,
		   "middle_down"     , &AppModule::OnMButtonDown     ,
		   "middle_dbl_click", &AppModule::OnMButtonDblClick ,  },
		{  "right_up"        , &AppModule::OnRButtonUp       ,
		   "right_down"      , &AppModule::OnRButtonDown     ,
		   "right_dbl_click" , &AppModule::OnRButtonDblClick ,  },
		{  "x1_up"           , &AppModule::OnXButton1Up      ,
		   "x1_down"         , &AppModule::OnXButton1Down    ,
		   "x1_dbl_click"    , &AppModule::OnXButton1DblClick,  },
		{  "x2_up"           , &AppModule::OnXButton2Up      ,
		   "x2_down"         , &AppModule::OnXButton2Down    ,
		   "x2_dbl_click"    , &AppModule::OnXButton2DblClick,  },
	};

	// update cursor
	FrameUpdateCursor( &m_CursorX, &m_CursorY, true );

	// save mouse coords
	int cursorX			= m_CursorX;
	int cursorY			= m_CursorY;
	float normCursorX	= m_NormalizedCursorX;
	float normCursorY	= m_NormalizedCursorY;

	// look up mouse position info given the time of this button message
	GetCursorPositionAtTime( m_CursorX, m_CursorY, m_LastMsg.time );

	// update normalized coordinates
	UpdateNormalizedCursor( m_CursorX, m_CursorY );

	// process
	if ( up )
	{
		bool wasDown = !!(m_CurrentQualifiers & Keys::Q_ANY_BUTTON);
		UpdateMouseQualifiers( wparam, cursorX, cursorY );

		// make sure it was down before - may be catching an errant click
		if ( wasDown )
		{
			Dispatch( s_MouseDispatch[ button ].m_UpName, button, false );
			(this->*s_MouseDispatch[ button ].m_UpProc)( m_CursorX, m_CursorY, wparam );
		}
	}
	else
	{
		// don't pass through the activation click
		if ( IsAppActive() )
		{
			// special: if the double-click was not handled, treat it as a
			// single click (cancel the dbl click).
			bool dblClick = false;
			if ( CheckDblClick( button ) )
			{
				dblClick = (Dispatch( s_MouseDispatch[ button ].m_DoubleName ) != NULL);
				if ( dblClick )
				{
					(this->*s_MouseDispatch[ button ].m_DoubleProc)( m_CursorX, m_CursorY, wparam );
				}
				else
				{
					m_DblClickButton = -1;
				}
			}

			if ( !dblClick )
			{
				Dispatch( s_MouseDispatch[ button ].m_DownName, button, true );
				(this->*s_MouseDispatch[ button ].m_DownProc)( m_CursorX, m_CursorY, wparam );
			}

			UpdateMouseQualifiers( wparam, cursorX, cursorY );
		}
	}

	// restore mouse coords if we aren't about to reset them
	if ( !m_ReqMouseReset )
	{
		m_CursorX			= cursorX;
		m_CursorY			= cursorY;
		m_NormalizedCursorX	= normCursorX;
		m_NormalizedCursorY	= normCursorY;
	}
}

void AppModule :: GetCursorPositionAtTime( int& cursorX, int& cursorY, DWORD time )
{
	kerneltool::Critical::Lock autoLock( *m_MouseCritical );

	// Go through the buffered positions and look for the closest positions
	CursorBuffer::iterator j = m_CursorBuffer.end();
	for( CursorBuffer::iterator i = m_CursorBuffer.begin(); i != m_CursorBuffer.end(); ++i )
	{
		// Find the first position that is beyond the current requested time
		if( (*i).m_Time > time )
		{
			if( j != m_CursorBuffer.end() )
			{
				// Linearly interpolate between the two nearest positions in the cache
				float interpolate	= (float)(PreciseSubtract(time, (*j).m_Time)) / (float)(PreciseSubtract( (*i).m_Time, (*j).m_Time));
				cursorX				= (*j).m_CursorX + FTOL( interpolate * ((float)(*i).m_CursorX - (float)(*j).m_CursorX) );
				cursorY				= (*j).m_CursorY + FTOL( interpolate * ((float)(*i).m_CursorY - (float)(*j).m_CursorY) );
			}
			else
			{
				// First position in the cache was after the click, just use it's position since that is the closest
				cursorX				= (*i).m_CursorX;
				cursorY				= (*i).m_CursorY;
			}

			return;
		}

		// Save previous iterator
		j = i;
	}

	// If we get here, there was no position in the cache that we could use, so use current
	cursorX	= m_CursorX;
	cursorY = m_CursorY;
}

bool AppModule :: UpdateGeometry( void )
{
	GRect oldWnd = m_WindowRect;

	if ( !::IsIconic( GetMainWnd() ) )
	{
		::GetWindowRect( GetMainWnd(), m_WindowRect );
		::GetClientRect( GetMainWnd(), m_ClientRect );
		::ClientToScreen( GetMainWnd(), m_ClientRect.TopLeft() );
		::ClientToScreen( GetMainWnd(), m_ClientRect.BottomRight() );

		if ( IsFullScreen() )
		{
			m_GameRect.MoveTo( m_ClientRect.TopLeft() );
		}
		else
		{
			m_GameRect = m_ClientRect;
		}

		// notify child
		if ( m_WindowRect.Size() != oldWnd.Size() )
		{
			if ( m_IsMovingSizing )
			{
				m_WasResized = true;
			}
			else
			{
				OnScreenChange();
			}
		}
	}

	return ( m_WindowRect != oldWnd );
}


void AppModule :: UpdateKeyQualifiers( LPARAM lparam )
{
	SetReset( m_CurrentQualifiers, Keys::Q_ALT, HIWORD(lparam) & KF_ALTDOWN );

	// $$ disabling key repeat because it causes some strange anomolies with
	// input, like having to hit enter twice in some cases because the first
	// enter did not have its defined callback called. the problem was this
	// code didn't use the HIWORD before, so it didn't do anything. then all
	// the other code here worked around that bug, so when this bug was fixed,
	// the rest broke. next time around, do it right from the start.
//	SetReset( m_CurrentQualifiers, Keys::Q_REPEAT, HIWORD(lparam) & KF_REPEAT );
}

void AppModule :: UpdateMouseQualifiers( WPARAM wparam, int cursorX, int cursorY, bool processKeyboard )
{
	Keys::QUALIFIER oldMouseDown = m_CurrentQualifiers & Keys::Q_ANY_BUTTON;

	// update qualifiers
	if ( processKeyboard )
	{
		SetReset( m_CurrentQualifiers, Keys::Q_SHIFT,    wparam & MK_SHIFT    );
		SetReset( m_CurrentQualifiers, Keys::Q_CONTROL,  wparam & MK_CONTROL  );
	}
	SetReset( m_CurrentQualifiers, Keys::Q_LBUTTON,  wparam & MK_LBUTTON  );
	SetReset( m_CurrentQualifiers, Keys::Q_MBUTTON,  wparam & MK_MBUTTON  );
	SetReset( m_CurrentQualifiers, Keys::Q_RBUTTON,  wparam & MK_RBUTTON  );
	SetReset( m_CurrentQualifiers, Keys::Q_XBUTTON1, wparam & MK_XBUTTON1 );
	SetReset( m_CurrentQualifiers, Keys::Q_XBUTTON2, wparam & MK_XBUTTON2 );

	// changed mouse button state?
	if ( !oldMouseDown != !(m_CurrentQualifiers & Keys::Q_ANY_BUTTON ) )
	{
		// mess with the cursor
		bool center = UpdateCursor();

		// if cursor is up and we aren't centering, reset cursor to where it
		// went down
		if ( oldMouseDown && !center )
		{
			::SetCursorPos( m_GameRect.left + cursorX, m_GameRect.top + cursorY );
		}

		// if not full screen, update show cursor state so we can see it (when
		// not dragging) in the non-client area.
		if ( !IsFullScreen() && !TestOptions( OPTION_CHILD_WND ) )
		{
			::ShowCursor( oldMouseDown );
		}
	}
}

bool AppModule :: UpdateOnKey( WPARAM key, bool down )
{
	bool handled = true;
	switch ( key )
	{
		case ( VK_SHIFT ):
		{
			SetReset( m_CurrentQualifiers, Keys::Q_SHIFT, down );
		}	break;
		case ( VK_CONTROL ):
		{
			SetReset( m_CurrentQualifiers, Keys::Q_CONTROL, down );
		}	break;
		case ( VK_MENU ):
		{
			SetReset( m_CurrentQualifiers, Keys::Q_ALT, down );
		}	break;
		default:
		{
			handled = false;
		}
	}
	return ( handled );
}

bool AppModule :: CheckDblClick( int button )
{
	// get some vars
	GSize size;
	GRect rect;

	// check button
	if ( m_DblClickButton != button )
	{
		goto different;
	}

	// check time
	if ( (::GetTickCount() - m_DblClickTime) > ::GetDoubleClickTime() )
	{
		goto different;
	}

	// build rect for comparing
	size.cx = ::GetSystemMetrics( SM_CXDOUBLECLK );
	size.cy = ::GetSystemMetrics( SM_CYDOUBLECLK );
	rect.Set( m_DblClickOrigin - (size / 2), size );

	// see if we're within it
	if ( !rect.Contains( GPoint( m_CursorX, m_CursorY ) ) )
	{
		goto different;
	}

	// yeh it's a double-click - clear results and return
	m_DblClickButton = -1;
	return ( true );

different:

	// only bother with detection if it's in the mask
	if ( m_DblClickMask & MakeDblClickMask( button ) )
	{
		// this might be the first click
		m_DblClickTime = ::GetTickCount();
		m_DblClickButton = button;
		m_DblClickOrigin.x = m_CursorX;
		m_DblClickOrigin.y = m_CursorY;
	}

	// not a double click, maybe next time!
	return ( false );
}

bool AppModule :: ExecPipelineTo( int priority )
{
	// $ basic goal here is to abort on any failed event and return false.
	//   exception is during the shutdown procedures. we want all of them to
	//   at least run (if not succeed) if OPTION_SHUTDOWN_CRITICAL is set.

	bool success = false;

	__try
	{
		__try
		{
			success = true;

			// execute pipeline callbacks up to but not including the given priority
			for (  bool shouldBreak = false
				 ; (*m_EventIter != m_EventCbDb->end()) && ((*m_EventIter)->first < priority) && !shouldBreak
				 ; ++(*m_EventIter) )
			{
				__try
				{
					__try
					{
						if ( !(*m_EventIter)->second() )
						{
							success = false;
							break;
						}
					}
					__except( ::GlobalExceptionFilter( GetExceptionInformation(),
							  EXCEPTION_EXECUTE_HANDLER,
							  EXCEPTION_EXECUTE_HANDLER,
							  true ) )
					{
						ReportSys::SetFatalIsOccurring();
						success = false;
						break;
					}
				}
				__finally
				{
					if ( ::AbnormalTermination() )
					{
						success = false;
						shouldBreak = true;
					}
				}
			}

			// check failure
			if ( !success )
			{
				m_ReturnValue = RET_FAIL;
			}
		}
		__except( ::GlobalExceptionFilter( GetExceptionInformation() ) )
		{
			// $ it's already been reported, ignore it but let all other unexpected
			//   exceptions through. unfortunately, given that exception handling is
			//   disabled, we'll not be shutting down properly and will get a
			//   billion memory leak reports, so just ignore all those.
			GPDEBUG_ONLY( _CrtSetDbgFlag( 0 ) );

			// failed
			m_ReturnValue = RET_FATAL;
		}
	}
	__finally
	{
		if ( ::AbnormalTermination() )
		{
			OnFailSafeShutdown();
			GPDEBUG_ONLY( _CrtSetDbgFlag( 0 ) );		// $ as above
			m_ReturnValue = RET_UNKNOWN;
		}
	}

	return ( success );
}

InputBinder* AppModule :: Dispatch( Keys::KEY key ) const
{
	return ( Dispatch( KeyInput( key, m_CurrentQualifiers ) ) );
}

InputBinder* AppModule :: Dispatch( const char* name ) const
{
	return ( Dispatch( MouseInput( name, m_CurrentQualifiers  ) ) );
}

InputBinder* AppModule :: Dispatch( const char* name, int button, bool down )
{
	InputBinder* handled = NULL;

	if ( down )
	{
		handled = Dispatch( name );
		if ( handled != NULL )
		{
			m_MouseCapture[ button ] = handled;
		}
	}
	else
	{
		handled = m_MouseCapture[ button ];
		if ( (handled != NULL) && handled->IsActive() )
		{
			if ( !handled->ProcessInput( MouseInput( name, m_CurrentQualifiers ) ) )
			{
				handled = NULL;
			}
		}
		else
		{
			handled = Dispatch( name );
		}
		m_MouseCapture[ button ] = NULL;
	}

	return ( handled );
}

InputBinder* AppModule :: Dispatch( wchar_t c ) const
{
	GPPROFILERSAMPLE( "AppModule::Dispatch( wchar_t )", SP_MISC );

	InputBinder* handled = NULL;

	if ( m_InputBinderDb != NULL )
	{
		for ( InputBinderDb::iterator i = m_InputBinderDb->begin(); i != m_InputBinderDb->end(); ++i )
		{
			if ( i->second->IsActive() && i->second->ProcessCharInput( c ) )
			{
				handled = i->second;
				break;
			}
		}
	}

	return ( handled );
}

InputBinder* AppModule :: Dispatch( const Input& input ) const
{
	GPPROFILERSAMPLE( "AppModule::Dispatch( Input )", SP_MISC );

	InputBinder* handled = NULL;

	if ( m_InputBinderDb != NULL )
	{
		for ( InputBinderDb::iterator i = m_InputBinderDb->begin(); i != m_InputBinderDb->end(); ++i )
		{
			if ( i->second->IsActive() && i->second->ProcessInput( input ) )
			{
				handled = i->second;
				break;
			}
		}
	}

	return ( handled );
}

AppModule::MessageCbDb& AppModule :: GetMessageCbDb( void )
{
	if ( m_MessageCbDb == NULL )
	{
		m_MessageCbDb = new MessageCbDb;
	}
	return ( *m_MessageCbDb );
}

AppModule::FrameCbDb& AppModule :: GetFrameCbDb( void )
{
	if ( m_FrameCbDb == NULL )
	{
		m_FrameCbDb = new FrameCbDb;
	}
	return ( *m_FrameCbDb );
}

AppModule::FrameCbDb& AppModule :: GetSysFrameCbDb( void )
{
	if ( m_SysFrameCbDb == NULL )
	{
		m_SysFrameCbDb = new FrameCbDb;
	}
	return ( *m_SysFrameCbDb );
}

AppModule::InputBinderDb& AppModule :: GetInputBinderDb( void )
{
	if ( m_InputBinderDb == NULL )
	{
		m_InputBinderDb = new InputBinderDb;
	}
	return ( *m_InputBinderDb );
}

void AppModule :: UpdatePriorityClass( void )
{
	DWORD priority = NORMAL_PRIORITY_CLASS;

	// boost process to keep things smooth even if other crap is running in the
	// background on the machine. common with outlook, winamp, etc...
	if (   TestOptions( OPTION_BOOST_APP_PRIORITY )
		&& !TestOptions( OPTION_DISABLE_APP_PRIORITY )
		&& IsAppActive() )
	{
		if ( SysInfo::IsOsWin9x() )
		{
			priority = HIGH_PRIORITY_CLASS;
		}
		else
		{
			priority = ABOVE_NORMAL_PRIORITY_CLASS;
		}
	}

	::SetPriorityClass( ::GetCurrentProcess(), priority );
}

void AppModule :: PrivateResetUserInputs( void )
{
	// don't bother if we're a tool
	if ( TestOptions( OPTION_CHILD_WND ) )
	{
		return;
	}

	// clear out all pending keyboard and mouse click input
	winx::EatAllUserInput( m_ReqKeyboardReset, m_ReqMouseReset );

	// mouse-specific
	if ( m_ReqMouseReset )
	{
		// reset our UI flags
		::ZeroObject( m_MouseCapture );
		::ZeroObject( m_MouseHistory );
		m_DblClickTime = 0;
		m_DblClickButton = -1;
		m_DblClickOrigin = GPoint::ZERO;
		UpdateMouseQualifiers( 0, m_CursorX, m_CursorY, false );

		// center the mouse cursor down the screen 2/3rds
		m_CursorX = GetGameWidth () / 2;
		m_CursorY = (GetGameHeight() * 2) / 3;

		// update normalized coords for new cursor
		UpdateNormalizedCursor( m_CursorX, m_CursorY );

		// this will force a proper position update
		if ( IsAppActive() )
		{
			SetMouseModeFixed( true );
			SetMouseModeFixed( false );
		}

		// nuke any pending events
		{
			kerneltool::Critical::Lock autoLock( *m_MouseCritical );
			m_CursorBuffer.clear();
		}
	}

	// reset flags
	if ( m_ReqMouseReset )
	{
		m_CurrentQualifiers &= Keys::NOT( Keys::Q_ANY_KEY );
	}
	if ( m_ReqMouseReset )
	{
		m_CurrentQualifiers &= Keys::NOT( Keys::Q_ANY_BUTTON );
	}

	// tell derived
	OnResetUserInputs( m_ReqKeyboardReset, m_ReqMouseReset );

	// clear request
	m_ReqKeyboardReset = false;
	m_ReqMouseReset = false;
}

//////////////////////////////////////////////////////////////////////////////
