//////////////////////////////////////////////////////////////////////////////
//
// File     :  AppModule.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains an application class framework.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __APPMODULE_H
#define __APPMODULE_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"
#include "FuBiDefs.h"
#include "Keys.h"
#include "WinGeometry.h"

//////////////////////////////////////////////////////////////////////////////
// forward declarations

namespace kerneltool
{
	class Mutex;
	class Event;
	class Critical;
}

namespace AppModuleNs
{
	class EventCbDb;
	class EventIter;
	class MessageCbDb;
	class FrameCbDb;
	class InputBinderDb;
}

namespace FuBi
{
	class PersistContext;
}

class  Config;
struct ConfigInit;
class  Input;
class  InputBinder;
class  LocMgr;
class  HangBuddy;

//////////////////////////////////////////////////////////////////////////////
// class AppModule declaration

class AppModule : public Singleton <AppModule>
{
public:
	SET_NO_INHERITED( AppModule );

// Types and constants.

	// $ special note: if using OPTION_FPU_CHOP_MODE below, be sure to put
	//                 DEFINE_FAST_FTOL() above your WinMain.

	enum eOptions
	{
		// creation
		OPTION_EXCEPTION_HANDLER        = 1 <<  0,	// install automatic top-level exception handler
		OPTION_APP_SINGLETON_MUTEX      = 1 <<  1,	// use a mutex to make app a singleton
		OPTION_CREATE_MAINWND           = 1 <<  2,	// create a main window
		OPTION_CHILD_WND                = 1 <<  3,	// main window is intended to be a child
		OPTION_FULLSCREEN               = 1 <<  4,	// this app is supposed to run full screen
		OPTION_RESIZABLE_WND            = 1 <<  5,	// this window can be resized by the user (by dragging)
		OPTION_GAME_STYLE_WND           = 1 <<  6,	// this window is meant for a game window (which implies certain behaviors)
		OPTION_MESSAGE_PUMP             = 1 <<  7,	// process messages
		OPTION_EXECUTE_FRAME_ONLY       = 1 <<  8,	// execute one frame per Execute() only
		OPTION_SHUTDOWN_CRITICAL        = 1 <<  9,	// the shutdown procedure is fully critical - ensure it completes
		OPTION_DISABLE_LOCALIZE_WARNING = 1 << 10,	// as part of initialization, turn off the "unlocalized context" thing
		OPTION_NO_ERROR_MESSAGES        = 1 << 11,	// disable errors and warnings
		OPTION_NO_PERF_MESSAGES         = 1 << 12,	// disable performance warnings
		OPTION_REFLECT_INPUT_PARENT     = 1 << 13,	// reflect input messages to parent window
		OPTION_LOCALIZE                 = 1 << 14,	// build localization manager, autodetect language, etc.
		OPTION_FAST_FTOL                = 1 << 15,	// set FPU to 24-bit chop mode so FTOL is fast and happy
		OPTION_ALWAYS_ON_TOP_WINDOWED   = 1 << 16,	// create window to be always on top
		OPTION_ALWAYS_ON_TOP_FULLSCREEN = 1 << 17,	// create window to be always on top
		OPTION_HANG_BUDDY               = 1 << 18,	// enable the hang detector (debughelp.cpp/h)

		// runtime
		OPTION_REPORT_STATUS            = 1 << 19,	// report status as we get it (recommend debug-only)
		OPTION_FREEZE_TIME_DETECT       = 1 << 20,	// check for frozen time (usually a debugger breakpoint) and adjust time
		OPTION_THROTTLE_FULLSCREEN      = 1 << 21,	// throttle frame rate in fullscreen mode
		OPTION_THROTTLE_WINDOWED        = 1 << 22,	// throttle frame rate in windowed mode
		OPTION_ENABLE_MINFPS            = 1 << 23,	// enable use of minimum frame rate
		OPTION_PERSIST_POSITION_WND     = 1 << 24,	// remember where we put the window last
		OPTION_PERSIST_SIZE_WND         = 1 << 25,	// remember what size the window was at last
		OPTION_SCREEN_CONSTRAIN_WND     = 1 << 26,	// constrain the main window to the screen
		OPTION_MOUSE_WRAP               = 1 << 27,	// wrap the mouse at x/y extents when constrained to the screen
		OPTION_MOUSE_FIXED              = 1 << 28,	// mouse cursor is fixed
		OPTION_MOUSE_SMOOTH_FIXED_DRAG  = 1 << 29,	// smooth out x/y dragging on mouse when fixed
		OPTION_BOOST_APP_PRIORITY       = 1 << 30,	// boost priority of app while it's active (needed for perfectly smooth frame rate in jittery system)
		OPTION_DISABLE_APP_PRIORITY     = 1 << 31,	// completely disable the priority option

		// ...

		OPTION_NONE = 0,							// disable all options
		OPTION_ALL  = 0xFFFFFFFF,					// enable all options
	};

	// jeez i ran out of bits...
	enum eXOptions
	{
		OPTION_ALWAYS_ACTIVE            = 1 <<  0,	// treat the app as if it's always active - useful for debugging
		OPTION_ALWAYS_MINIMALLY_ACTIVE  = 1 <<  1,	// like "always active" except try to minimize cpu use when inactive
		OPTION_TRACE_MESSAGES           = 1 <<  2,	// dump messages as we receive them (debug flag)
		OPTION_SINGLE_STEP_NEXT_FRAME   = 1 <<  3,	// advance GetSingleStepTime() seconds for the next frame only
		OPTION_FIXED_FRAME_RATE         = 1 <<  4,	// force the frame rate to be a certain number (used for making movies or something)
		OPTION_SHOULD_LOG_FPS           = 1 <<  5,	// should we log FPS data?
		OPTION_ENABLE_LOG_FPS           = 1 <<  6,	// this must also be set - tune this in game by enabling on and off
		OPTION_IS_EXTERNAL_PROFILING    = 1 <<  7,	// true if an external profiler (non-game code) is running
		OPTION_TEST_MODE                = 1 <<  8,	// enable test mode, for anyone who is paying attention
		OPTION_DISABLE_TASK_SWITCH      = 1 <<  9,	// run as exclusive mode, which disables windows key, alt-tab, etc.
        OPTION_DISABLE_WIN_KEYS         = 1 << 10,  // disable just the windows keys
		OPTION_PERFMON_LOG				= 1 << 11,	// this is to enable logging of the perfmon
		

		OPTION_X_NONE = 0,							// disable all options
		OPTION_X_ALL  = 0xFFFFFFFF,					// enable all options
	};

	static const eOptions OPTION_CONSOLE_DEF;		// typical console defaults
	static const eOptions OPTION_WINDOWED_DEF;		// typical windowed app defaults
	static const eOptions OPTION_UTILWND_DEF;		// typical utility program app defaults
	static const eOptions OPTION_FULLSCREEN_DEF;	// typical fullscreen app defaults
	static const eOptions OPTION_CHILD_DEF;			// typical child app defaults (i.e. for editor)

	enum // priorities
	{
		PRI_INIT_CORE     = 0x1000,
		PRI_INIT_WINDOW   = 0x2000,
		PRI_INIT_SERVICES = 0x3000,
		PRI_EXECUTE       = 0x4000,
		PRI_SHUTDOWN      = 0x5000,
		PRI_FINAL         = 0x6000,
	};

	enum // return values
	{
		RET_OK              = 0,					// successful normal operation
		RET_ALREADY_RUNNING = 'DUPE',				// duplicate instance detected and activated, aborted
		RET_FAIL            = 'FAIL',				// handled failure
		RET_FATAL           = 'FATL',				// fatal exception
		RET_UNKNOWN         = 'UNK!',				// unrecognized fatal exception
	};

	// return false to fail the event
	typedef CBFunctor0wRet <bool>					// [return]
			EventCb;

	// return true if the message was handled. if handled, 'returnValue'
	// contains the value to return to Windows.
	typedef CBFunctor2wRet <const MSG&,				// msg
							LRESULT&,				// returnValue
							bool>					// [return]
			MessageCb;

	// return false to abort execution
	typedef CBFunctor2wRet <float,					// deltaTime
							float,					// actualDeltaTime
							bool>					// [return]
			FrameCb;

// Setup.

	// $ these methods are to be used for initialization

	// ctor/dtor
	AppModule( eOptions options = OPTION_NONE, eXOptions xoptions = OPTION_X_NONE );
	virtual ~AppModule( void );

	// persistence - simple variables only
	virtual bool Xfer( FuBi::PersistContext& persist );

	// window creation spec
	void SetAppIcon      ( HICON icon )					{  gpassert( icon != NULL );  m_AppIcon = icon;   }
	void SetAppIcon      ( WORD iconId )				{  SetAppIcon( MAKEINTRESOURCE( iconId ) );  }
	void SetAppIcon      ( LPCTSTR iconName )			{  SetAppIcon( ::LoadIcon( GetInstance(), iconName ) );  }
	void SetAppAccel     ( HACCEL accel )				{  gpassert( accel != NULL );  m_AppAccel = accel;  }
	void SetAppAccel     ( WORD accelId )				{  SetAppAccel( MAKEINTRESOURCE( accelId ) );  }
	void SetAppAccel     ( LPCTSTR accelName )			{  SetAppAccel( ::LoadAccelerators( GetInstance(), accelName ) );  }
	void SetAppMenu      ( HMENU menu )					{  gpassert( menu != NULL );  m_AppMenu = menu;  }
	void SetAppMenu      ( WORD menuId )				{  SetAppMenu( MAKEINTRESOURCE( menuId ) );  }
	void SetAppMenu      ( LPCTSTR menuName )			{  SetAppMenu( ::LoadMenu( GetInstance(), menuName ) );  }
	void SetAppName      ( const gpstring& name )		{  m_AppName = ::ToUnicode( name );  }
	void SetAppName      ( const gpwstring& name )		{  m_AppName = name;  }
	void SetAppName      ( UINT nameId )				{  gpwstring str( nameId );  gpassert( !str.empty() );  SetAppName( str );  }
	void SetAppTitleExtra( const gpwstring& extra )		{  m_AppTitleExtra = extra;  }
	void SetIniName      ( const gpstring& name )		{  m_IniName = name;  }
	void SetIniName      ( UINT nameId )				{  gpstring str( nameId );  gpassert( !str.empty() );  SetIniName( str );  }
	void SetWndClassName ( const gpstring& name )		{  m_WndClassName = name;  }
	void SetParentWnd    ( HWND parent )				{  gpassert( ::IsWindow( parent ) );  m_ParentWnd = parent;  }
	void SetStartupRect  ( const GRect& rect )			{  m_StartupRect = rect;  }

	// query of above
	HICON  GetAppIcon ( void ) const					{  return ( m_AppIcon  );  }
	HACCEL GetAppAccel( void ) const					{  return ( m_AppAccel );  }
	HMENU  GetAppMenu ( void ) const					{  return ( m_AppMenu  );  }
	bool   HasAppMenu ( void ) const					{  return ( m_AppMenu != NULL );  }
	DWORD  GetAppCrc32( void ) const;

	// config setup
	ConfigInit& GetConfigInit( void )					{  return ( *m_ConfigInit );  }

	// initialize stages
	bool IsUninitialized( void ) const					{  return ( m_EventIter == NULL );  }
	bool IsInitialized  ( void ) const;
	bool IsInitializing ( void ) const;
	bool IsShuttingDown ( void ) const;

	// configuration
	void SetOptions   ( eOptions options, bool set = true );
	void ClearOptions ( eOptions options )					{  SetOptions( options, false );  }
	void ToggleOptions( eOptions options );
	bool TestOptions  ( eOptions options ) const			{  return ( (m_Options & options) != 0 );  }
	bool TestOptionsEq( eOptions options ) const			{  return ( (m_Options & options) == options );  }
	void SetOptions   ( eXOptions options, bool set = true );
	void ClearOptions ( eXOptions options )					{  SetOptions( options, false );  }
	void ToggleOptions( eXOptions options );
	bool TestOptions  ( eXOptions options ) const			{  return ( (m_XOptions & options) != 0 );  }
	bool TestOptionsEq( eXOptions options ) const			{  return ( (m_XOptions & options) == options );  }

	// double click detection enabling
	void EnableDblClickDetect ( int button, bool set = true )	{  m_DblClickMask = set ? (m_DblClickMask | MakeDblClickMask( button )) : (m_DblClickMask & ~MakeDblClickMask( button ));  }
	void DisableDblClickDetect( int button )					{  EnableDblClickDetect( button, false );  }

	// persistence
	void SaveWindowPos( void );

// State.

	// $ these methods are to be used after the app is running

	// general
	int              GetReturnValue   ( void ) const	{  return ( m_ReturnValue );  }
	HINSTANCE        GetInstance      ( void ) const	{  gpassert( m_Instance != NULL );  return ( m_Instance );  }
	bool             HasMainWnd       ( void ) const	{  return ( m_MainWnd != NULL );  }
	bool			 HasVisibleMainWnd( void ) const	{  return ( HasMainWnd() && ::IsWindowVisible( GetMainWnd() ) );  }
	void             SetMainWnd       ( HWND w )		{  gpassert( ::IsWindow( w ) );  m_MainWnd = w;  }
	HWND             GetMainWnd       ( void ) const	{  gpassert( ::IsWindow( m_MainWnd ) );  return ( m_MainWnd );  }
	const gpwstring& GetAppName       ( void ) const	{  return ( m_AppName );  }
	gpwstring        MakeTitle        ( void ) const;
	DWORD            CalcWindowStyle  ( void ) const;

	// window info
	bool         IsFullScreen ( void ) const			{  return ( m_FullScreen );  }
	bool         IsMinimized  ( void ) const			{  return ( ::IsIconic( GetMainWnd() ) != 0 );  }
	const GRect& GetGameRect  ( void ) const			{  return ( m_GameRect );  }
	int          GetGameWidth ( void ) const			{  return ( m_GameRect.Width() );  }
	int          GetGameHeight( void ) const			{  return ( m_GameRect.Height() );  }
	GRect        GetClientRect( void ) const			{  return ( m_GameRect.SizeRect() );  }
	const GRect& GetWindowRect( void ) const			{  return ( m_WindowRect );  }

	// app activation info
	void AppActivate       ( bool activate = true );
	void AppDeactivate     ( void )						{  AppActivate( false );  }
	bool IsAppActive       ( void ) const				{  return ( m_AppActivateCount > 0 );  }

	// execution status
FEX	bool RequestQuit     ( bool force = false );
	bool GetRequestQuit  ( void ) const;
	void RequestFatalQuit( void );

	// geometry changing
	void SetGameSize          ( const GSize& size );
	void SetGameSizeFromConfig( void )					{  SetGameSize( GSize::ZERO );  }
	void SetFullScreen        ( bool set = true );
	void SetWindowed          ( void )					{  SetFullScreen( false );  }
	void ToggleFullScreen     ( void )					{  SetFullScreen( !IsFullScreen() );  }

	// input state
FEX	bool     GetControlKey       ( void ) const			{  return ( (m_CurrentQualifiers & Keys::Q_CONTROL ) != 0 );  }
FEX	bool     GetAltKey           ( void ) const			{  return ( (m_CurrentQualifiers & Keys::Q_ALT     ) != 0 );  }
FEX	bool     GetShiftKey         ( void ) const			{  return ( (m_CurrentQualifiers & Keys::Q_SHIFT   ) != 0 );  }
FEX	bool     GetSpecialKey       ( void ) const			{  return ( (m_CurrentQualifiers & Keys::Q_SPECIAL ) != 0 );  }
FEX	bool     GetLButton          ( void ) const			{  return ( (m_CurrentQualifiers & Keys::Q_LBUTTON ) != 0 );  }
FEX	bool     GetMButton          ( void ) const			{  return ( (m_CurrentQualifiers & Keys::Q_MBUTTON ) != 0 );  }
FEX	bool     GetRButton          ( void ) const			{  return ( (m_CurrentQualifiers & Keys::Q_RBUTTON ) != 0 );  }
FEX	bool     GetXButton1         ( void ) const			{  return ( (m_CurrentQualifiers & Keys::Q_XBUTTON1) != 0 );  }
FEX	bool     GetXButton2         ( void ) const			{  return ( (m_CurrentQualifiers & Keys::Q_XBUTTON2) != 0 );  }
FEX	int      GetCursorX          ( void ) const			{  return ( m_CursorX );  }
FEX	int      GetCursorY          ( void ) const			{  return ( m_CursorY );  }
FEX	float    GetNormalizedCursorX( void ) const			{  return ( m_NormalizedCursorX );  }
FEX	float    GetNormalizedCursorY( void ) const			{  return ( m_NormalizedCursorY );  }
	gpstring MakeMouseDebugState ( void ) const;
	void     SetSkipNextChar     ( bool skip = true )	{  m_SkipNextChar = skip;  }
	void     ResetUserInputs     ( bool keyboard = true, bool mouse = true );

	// mouse sensitivity
	void  SetMouseScaleX( float value )					{  m_MouseScaleX = value;  }
	void  SetMouseScaleY( float value )					{  m_MouseScaleY = value;  }
	void  SetMouseScaleZ( float value )					{  m_MouseScaleZ = value;  }
	float GetMouseScaleX( void ) const					{  return ( m_MouseScaleX );  }
	float GetMouseScaleY( void ) const					{  return ( m_MouseScaleY );  }
	float GetMouseScaleZ( void ) const					{  return ( m_MouseScaleZ );  }

	// fixed mouse sensitivity
	void  SetFixedMouseScaleX( float value )			{  m_FixedMouseScaleX = value;  }
	void  SetFixedMouseScaleY( float value )			{  m_FixedMouseScaleY = value;  }
	void  SetFixedMouseScaleZ( float value )			{  m_FixedMouseScaleZ = value;  }
	float GetFixedMouseScaleX( void ) const				{  return ( m_FixedMouseScaleX );  }
	float GetFixedMouseScaleY( void ) const				{  return ( m_FixedMouseScaleY );  }
	float GetFixedMouseScaleZ( void ) const				{  return ( m_FixedMouseScaleZ );  }

// Time.

	// user pausing
FEX	void UserPause      ( bool pause = true );
	void UserResume     ( void )						{  UserPause( false );  }
FEX	bool IsUserPaused   ( void )						{  return ( m_UserPaused );  }

	// pausing
	void Pause      ( bool pause = true );
	void Resume     ( void )							{  Pause( false );  }
	bool IsPaused   ( void )							{  return ( m_PauseCount > 0 );  }
	void ForcePause ( bool pause = true );
	void ForceResume( void )							{  ForcePause( false );  }

	// time freezing
	void FreezeTime  ( bool freeze = true );
	void ThawTime    ( void )							{  FreezeTime( false );  }
	bool IsTimeFrozen( void )							{  return ( m_FreezeTimeCount > 0 );  }

    // time advancement
    void   SetTimeMultiplier    ( float multiplier )	{  m_TimeMultiplier = multiplier;  }
    float  GetTimeMultiplier    ( void ) const			{  return ( m_TimeMultiplier );  }
	void   SetSingleStep        ( bool set = true )		{  SetOptions( OPTION_SINGLE_STEP_NEXT_FRAME, set );  }
	void   ClearSingleStep      ( void )				{  SetSingleStep( false );  }
	bool   IsSingleStep         ( void ) const			{  return ( TestOptions( OPTION_SINGLE_STEP_NEXT_FRAME ) );  }
	void   SetSingleStepTime    ( float delta )			{  m_SingleStepTime = delta;  }
	float  GetSingleStepTime    ( void ) const			{  return ( m_SingleStepTime );  }
	void   SetThrottleFrameRate ( float fps )			{  gpassert( fps > 0 );  m_ThrottleFrameRate = fps;  }
	float  GetThrottleFrameRate ( void ) const			{  return ( m_ThrottleFrameRate );  }
	void   SetMinFrameRate      ( float fps )			{  gpassert( fps > 0 );  m_MinFrameRate = fps;  }
	float  GetMinFrameRate      ( void ) const			{  return ( m_MinFrameRate );  }
	void   SetFixedFrameRate    ( float fps )			{  gpassert( fps > 0 );  m_FixedFrameRate = fps;  }
	float  GetFixedFrameRate    ( void ) const			{  return ( m_FixedFrameRate );  }
	double GetAbsoluteTime      ( void ) const			{  return ( m_AbsoluteTime );  }
	float  GetDeltaTime         ( void ) const			{  return ( m_DeltaTime );  }
	float  GetActualDeltaTime   ( void ) const			{  return ( m_ActualDeltaTime );  }
	void   SetInactiveUpdateRate( float ups )			{  gpassert( ups >= 0 );  m_InactiveUpdateRate = ups;  }
	float  GetInactiveUpdateRate( void ) const			{  return ( m_InactiveUpdateRate );  }

	// frame rate
	float GetInstantFrameRate ( void ) const			{  return ( m_InstantFrameRate );  }
	float GetFilteredFrameRate( void ) const			{  return ( m_FilteredFrameRate );  }

	// when was app first started up?
	const SYSTEMTIME& GetStartTime( void ) const		{  return ( m_StartTime );  }

	// pause helper
	struct AutoPause
	{
		AutoPause( void );
	   ~AutoPause( void );
	};

	// freeze time helper
	struct AutoFreezeTime
	{
		AutoFreezeTime( void );
	   ~AutoFreezeTime( void );
	};

	// auto update helper for when options change
	struct AutoOptionsTracker
	{
		eOptions  m_OldOptions;
		eXOptions m_OldXOptions;

		AutoOptionsTracker( void );
	   ~AutoOptionsTracker( void );
	};

	friend AutoOptionsTracker;

// Registration.

	// init/shutdown event hooking - priority is relative to the PRI_*
	// constants.
	void RegisterEventCb( EventCb cb, int priority );

	// window procedure hooking - priority is arbitrary
	void AddMessageCb   ( MessageCb cb, int priority = 0 );
	void RemoveMessageCb( MessageCb cb );

	// frame procedure hooking - priority is arbitrary
	void AddFrameCb      ( FrameCb cb, int priority = 0 );
	void AddSysFrameCb   ( FrameCb cb, int priority = 0 );
	void RemoveFrameCb   ( FrameCb cb );
	void RemoveSysFrameCb( FrameCb cb );

	// input binder registration
	void RegisterBinder  ( InputBinder* binder, int priority );
	void UnregisterBinder( InputBinder* binder );

	// global binder access
	InputBinder& GetGlobalBinder( void );

// Operation.

	// staged execution
	virtual bool Init    ( HINSTANCE instance = NULL );
	virtual bool Run     ( void );
	virtual bool Shutdown( void );

	// easy run
	virtual int AutoRun( HINSTANCE instance = NULL );

	// mouse state
	virtual void SetMouseModeFixed( bool set = true );
	virtual bool FrameUpdateCursor( int* pCursorX, int* pCursorY, bool bFullUpdate = true );

// Utility.

	// system callbacks
	static LONG    WINAPI   StaticExceptionFilter( EXCEPTION_POINTERS* xinfo );
	static LRESULT CALLBACK StaticAppWindowProc  ( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
	LRESULT                 AppWindowProc        ( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

	// generate a unique name based on the filename
	static gpstring MakeUniqueName( const char* prefix );

	// build a double click mask
	static int MakeDblClickMask( int button )	{  gpassert( (button >= 0) && (button < 5) );  return ( 1 << button );  }

protected:

// Internal methods.

	void SetReturnValue( int returnValue )		{  m_ReturnValue = returnValue;  }
	void SetDeltaTime( float deltaTime )		{  m_DeltaTime = deltaTime;  }

	// $ the methods in this section are meant for derived classes to override.
	//   they do not do anything so you don't need to call the inherited.

// Message callbacks.

	// $ return true if 'handled'

	// $$$ what about click and drag detecting? "drag mode" for auto-setcursorpos..?
	// $$$ get the UIMode stuff from gameengine.cpp/h

	virtual bool OnChar            ( wchar_t /*ch*/, int /*repeatCount*/ )					{  return ( false );  }
	virtual bool OnKeyDown         ( UINT /*vkey*/, int /*repeatCount*/, UINT /*flags*/ )	{  return ( false );  }
	virtual bool OnKeyUp           ( UINT /*vkey*/, int /*repeatCount*/, UINT /*flags*/ )	{  return ( false );  }

	virtual bool OnSysChar         ( TCHAR /*ch*/, int /*repeatCount*/ )					{  return ( false );  }
	virtual bool OnSysKeyDown      ( UINT /*vkey*/, int /*repeatCount*/, UINT /*flags*/ )	{  return ( false );  }
	virtual bool OnSysKeyUp        ( UINT /*vkey*/, int /*repeatCount*/, UINT /*flags*/ )	{  return ( false );  }

	virtual bool OnLButtonDown     ( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }
	virtual bool OnLButtonUp       ( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }
	virtual bool OnLButtonDblClick ( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }

	virtual bool OnMButtonDown     ( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }
	virtual bool OnMButtonUp       ( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }
	virtual bool OnMButtonDblClick ( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }

	virtual bool OnRButtonDown     ( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }
	virtual bool OnRButtonUp       ( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }
	virtual bool OnRButtonDblClick ( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }

	virtual bool OnXButton1Down    ( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }
	virtual bool OnXButton1Up      ( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }
	virtual bool OnXButton1DblClick( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }

	virtual bool OnXButton2Down    ( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }
	virtual bool OnXButton2Up      ( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }
	virtual bool OnXButton2DblClick( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }

	virtual bool OnMouseMove       ( int /*x*/, int /*y*/, UINT /*keyFlags*/ )				{  return ( false );  }
	virtual bool OnMouseFrameDelta ( int /*deltaX*/, int /*deltaY*/ )						{  return ( false );  }
	virtual bool OnMouseWheel      ( int /*x*/, int /*y*/, int /*z*/, UINT /*keyFlags*/ )	{  return ( false );  }

	virtual bool OnPaint           ( PAINTSTRUCT& /*ps*/ )									{  return ( false );  }
	virtual bool OnMove            ( const GPoint& /*newOrigin*/ )							{  return ( false );  }
	virtual bool OnSize            ( const GSize& /*newSize*/ )								{  return ( false );  }

	virtual bool OnCommand         ( int /*id*/ )											{  return ( false );  }

// Special callbacks.

	// $ return false on failure

	// pipeline
	virtual bool OnInitCore    ( void )														{  return ( true );  }
	virtual bool OnInitWindow  ( void )														{  return ( true );  }
	virtual bool OnInitServices( void )														{  return ( true );  }
	virtual bool OnExecute     ( void )														{  return ( true );  }
	virtual bool OnShutdown    ( void )														{  return ( true );  }

	// special pipeline
	virtual void OnFailSafeShutdown( void )													{  }
	virtual void OnRestoreSurfaces ( void )													{  }

	// init callbacks
	virtual bool OnGetFullScreen   ( bool& /*full*/ )										{  return ( true );  }
	virtual bool OnGetGameSize     ( GSize& /*size*/ )										{  return ( true );  }
	virtual bool OnGetGameSizeDirty( void )													{  return ( false );  }

	// execution events
	virtual void OnAppActivate        ( bool /*activate*/ )									{  }
	virtual void OnUserPause          ( bool /*pause*/ )									{  }
	virtual void OnPause              ( bool /*pause*/ )									{  }
	virtual void OnFreezeTime         ( bool /*freeze*/ )									{  }
	virtual bool OnRequestAppClose    ( void )												{  return ( true );  }
	virtual bool OnScreenChange       ( void )												{  return ( true );  }
	virtual bool OnCheckFocus         ( void )												{  return ( true );  }
	virtual bool OnPreTranslateMessage( MSG& /*msg*/ )										{  return ( false );  }
	virtual bool OnMessage            ( HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wparam*/,
										LPARAM /*lparam*/, LRESULT& /*returnValue*/ )		{  return ( false );  }
	virtual bool OnUpdate             ( void );
	virtual bool OnInactiveUpdate     ( void )												{  return ( true );  }
	virtual bool OnFrame              ( float /*deltaTime*/, float /*actualDeltaTime*/ )	{  return ( true );  }
	virtual bool OnSysFrame           ( float /*deltaTime*/, float /*actualDeltaTime*/ )	{  return ( true );  }
	virtual bool OnBeginFrame         ( void )												{  return ( true );  }
	virtual bool OnEndFrame           ( void )												{  return ( true );  }
	virtual bool OnClearScreen        ( void )												{  return ( true );  }
	virtual void OnResetUserInputs    ( bool /*keyboard*/, bool /*mouse*/ )					{  }

private:

	// private pipeline
	bool PrivateInitCore    ( void );
	bool PrivateInitWindow  ( void );
	bool PrivateInitServices( void );
	bool PrivateExecute     ( void );
	bool PrivateKillWindow  ( void );
	bool PrivateShutdown    ( void );

	// un-namespace'd types
	typedef kerneltool::Mutex          Mutex;
	typedef kerneltool::Event          Event;
	typedef kerneltool::Critical       Critical;
	typedef AppModuleNs::EventCbDb     EventCbDb;
	typedef AppModuleNs::EventIter     EventIter;
	typedef AppModuleNs::MessageCbDb   MessageCbDb;
	typedef AppModuleNs::FrameCbDb     FrameCbDb;
	typedef AppModuleNs::InputBinderDb InputBinderDb;
	typedef bool (ThisType::*MouseProc)( int, int, UINT );

	// internal table type
	struct MouseDispatch
	{
		const char* m_UpName;
		MouseProc   m_UpProc;
		const char* m_DownName;
		MouseProc   m_DownProc;
		const char* m_DoubleName;
		MouseProc   m_DoubleProc;
	};

	struct MousePosCache
	{
		int			m_CursorX;
		int			m_CursorY;
		DWORD		m_Time;
	};

	// utility
	void           ProcessMouseButton	  ( bool up, int button, WPARAM wparam );
	void		   GetCursorPositionAtTime( int& cursorX, int& cursorY, DWORD time );
	bool           UpdateGeometry         ( void );
	void           UpdateKeyQualifiers    ( LPARAM lparam );
	void           UpdateMouseQualifiers  ( WPARAM wparam, int cursorX, int cursorY, bool processKeyboard = true );
	bool           UpdateOnKey            ( WPARAM key, bool down );
	bool           UpdateCursor           ( bool centerMouse = true );
	void           UpdateNormalizedCursor ( int cursorX, int cursorY );
	bool           CheckDblClick          ( int button );
	bool           ExecPipelineTo         ( int priority );
	InputBinder*   Dispatch               ( Keys::KEY key ) const;						// key
	InputBinder*   Dispatch               ( const char* name ) const;						// mouse
	InputBinder*   Dispatch               ( const char* name, int button, bool down );	// mouse (special: for button messages)
	InputBinder*   Dispatch               ( wchar_t c ) const;							// unicode char
	InputBinder*   Dispatch               ( const Input& input ) const;					// general
	MessageCbDb&   GetMessageCbDb         ( void );
	FrameCbDb&     GetFrameCbDb           ( void );
	FrameCbDb&     GetSysFrameCbDb        ( void );
	InputBinderDb& GetInputBinderDb       ( void );
	void           UpdatePriorityClass    ( void );
	void           PrivateResetUserInputs ( void );

// Specification.

	// window/app params
	HICON     m_AppIcon;					// icon to use for main window
	HACCEL    m_AppAccel;					// accelerator to use in message loop
	HMENU     m_AppMenu;					// menu to use on main window
	gpwstring m_AppName;					// name of the app
	gpwstring m_AppTitleExtra;				// extra info to append to app title
	gpstring  m_IniName;					// name of the app's ini file to use (defaults to exe name)
	gpstring  m_WndClassName;				// window class name for main window
	HWND      m_ParentWnd;					// parent window for main window
	GRect     m_StartupRect;				// startup rect to use for main window

	// configuration
	eOptions  m_Options;					// current options
	eXOptions m_XOptions;					// current extended options
	gpstring  m_Prefix;						// prefix used for environment

	// hooks
	my EventCbDb*   m_EventCbDb;			// hooked event procs
	my MessageCbDb* m_MessageCbDb;			// hooked window procs
	my FrameCbDb*   m_FrameCbDb;			// hooked frame procs
	my FrameCbDb*   m_SysFrameCbDb;			// hooked frame procs

// State.

	// $ note: mouse positions m_CursorX/Y are in normal windows coordinates,
	//         where 0,0 is the top left of the screen. the m_Normalized coords
	//         are required by siege, where 0,0 is the center of the screen,
	//         x goes to the right and y goes up.

	typedef stdx::fast_vector <float>			FloatBuffer;
	typedef stdx::fast_vector <MousePosCache>	CursorBuffer;

	// general
	int       m_ReturnValue;				// return value for WinMain()
	HINSTANCE m_Instance;					// instance handle of this app
	HWND      m_MainWnd;					// main game window
	bool      m_FullScreen;					// true if we're fullscreen

    // time and state
	int    m_AppActivateCount;				// >0 if the app is active and has focus
	bool   m_UserPaused;					// has app been paused by the user? (but still runs)
	int    m_PauseCount;					// >0 if the app is officially paused
	int    m_FreezeTimeCount;				// >0 if time is frozen (only freezes time, no pause)
    float  m_TimeMultiplier;				// deltas multiplied by this per frame
    float  m_SingleStepTime;				// amount of time a single step takes
	float  m_ThrottleFrameRate;				// frame rate to throttle to
	float  m_MinFrameRate;					// minimum frame rate to allow
	float  m_FixedFrameRate;				// frame rate to fix to
	float  m_InactiveUpdateRate;			// rate at which to update when inactive (in hz)

	// geometry
	GRect m_WindowRect;						// rect of entire window (screen coords)
	GRect m_ClientRect;						// rect of client area (screen coords)
	GRect m_GameRect;						// rect of game area of client (screen coords)

	// statistics
	SYSTEMTIME  m_StartTime;				// time that we started the app up
	float       m_InstantFrameRate;			// instantaneous frame rate
	float       m_FilteredFrameRate;		// frame rate run through filter function
	FloatBuffer m_FrameDeltaBuffer;			// previous frame deltas
	int         m_Frame;					// current frame number (actually a sim count)
	int         m_SysFrame;					// current absolute frame number
	double      m_AbsoluteTime;				// current time absolute from app start
	float       m_DeltaTime;				// delta in time since last frame (may be adjusted for game speed)
	float       m_ActualDeltaTime;			// the true delta since last frame

	// input state
	bool            m_IsMovingSizing;		// true if we're currently moving/sizing
	bool            m_WasResized;			// true if we resized while moving/sizing
	bool            m_IsReplacingWnd;		// true if the window is being "replaced" (destroyed & recreated)
	Keys::QUALIFIER m_CurrentQualifiers;	// current qualifiers being held down (mouse, ctrl keys etc)
	GPoint          m_LastMousePos;			// last place we knew the mouse was
	InputBinder*    m_MouseCapture[ 5 ];	// current input binders that have the mouse capture (per button)
	int             m_CursorX;				// last cursor x position
	int             m_CursorY;				// last cursor y position
	int             m_CursorCenterX;		// where we think the x center of the screen is
	int             m_CursorCenterY;		// where we think the y center of the screen is
	GPoint          m_MouseHistory[ 3 ];	// history of deltas for smoothing mouse drags
	float           m_NormalizedCursorX;	// normalized (-1 -> 1) last cursor x position
	float           m_NormalizedCursorY;	// normalized (-1 -> 1) last cursor y position
	CursorBuffer	m_CursorBuffer;			// history of the async updated mouse positions
	int             m_DblClickMask;			// which mouse buttons to detect double-clicks for
	int             m_DblClickTime;			// when we started detecting double click
	int             m_DblClickButton;		// which button is the candidate
	GPoint          m_DblClickOrigin;		// where the double click may have originated
	bool            m_SkipNextChar;			// special: skip the next WM_CHAR we receive
	float           m_MouseScaleX;			// mouse sensitivity in x direction
	float           m_MouseScaleY;			// mouse sensitivity in y direction
	float           m_MouseScaleZ;			// mouse sensitivity in z (wheel) direction
	float           m_FixedMouseScaleX;		// mouse sensitivity in x direction to use when mouse is fixed
	float           m_FixedMouseScaleY;		// mouse sensitivity in y direction to use when mouse is fixed
	float           m_FixedMouseScaleZ;		// mouse sensitivity in z (wheel) direction to use when mouse is fixed
	Critical*	    m_MouseCritical;		// mouse update critical
	bool            m_ReqKeyboardReset;		// request for a keyboard reset
	bool            m_ReqMouseReset;		// request for a mouse reset

	// cache
	MSG m_LastMsg;							// last message we received

// Objects.

	// general
	my Event*     m_RequestQuit;			// event that is fired when a quit is requested
	my Event*     m_RequestFatalQuit;		// event that is fired when a fatal quit is requested
	my Mutex*     m_InstanceMutex;			// singleton mutex for one app instance
	my EventIter* m_EventIter;				// next event in startup sequence

	// services
	my Config*        m_Config;				// game configuration
	my ConfigInit*    m_ConfigInit;			// setup for game configuration
	my InputBinderDb* m_InputBinderDb;		// database of input binders
	my InputBinder*   m_GlobalBinder;		// the global (highest priority) binder
	my LocMgr*        m_LocMgr;				// localization manager for language-specific resourcing
	my HangBuddy*     m_HangBuddy;			// i can dump stacks on a hang!

	FUBI_SINGLETON_CLASS( AppModule, "Base application class." );
	SET_NO_COPYING( AppModule );
};

MAKE_ENUM_BIT_OPERATORS( AppModule::eOptions );
MAKE_ENUM_BIT_OPERATORS( AppModule::eXOptions );

#define gAppModule AppModule::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

#endif  // __APPMODULE_H

//////////////////////////////////////////////////////////////////////////////
