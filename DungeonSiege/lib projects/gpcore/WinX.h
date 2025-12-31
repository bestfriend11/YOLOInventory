//////////////////////////////////////////////////////////////////////////////
//
// File     :  WinX.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a big pile of helper functions for doing stuff with
//             windows (little 'w').
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __WINX_H
#define __WINX_H

//////////////////////////////////////////////////////////////////////////////

#include "WinGeometry.h"

namespace winx  {  // begin of namespace winx ("windows extensions")

//////////////////////////////////////////////////////////////////////////////
// Helper function declarations

// Windows.

	HWND GetFrontWindow( void );
	HWND GetConsoleWindow( void );
	void CenterWindow( HWND wnd, HWND base = NULL );

// Screen geometry.

	GRect MakeScreenRect( void );
	GRect MakeVirtualScreenRect( void );

// Tracing.

#	if !GP_RETAIL
	void TraceMsg( const MSG& msg, bool reportAll = false, ReportSys::ContextRef ctx = NULL );
#	endif // !GP_RETAIL

// Messages.

    void EatMessages( UINT first, UINT last );
    void EatMessages( UINT event );
    void EatAllUserInput( bool keyboard = true, bool mouse = true );

// Misc.

	UINT GetLegacyMouseWheelMessage( void );
	UINT GetQueryCancelAutoPlayMessage( void );
	bool IsInputMessage( UINT msg );

	enum
	{
		ALLOW_AUTO_PLAY  = 0,
		CANCEL_AUTO_PLAY = 1,
	};

//////////////////////////////////////////////////////////////////////////////
// struct LogFont declaration

// the purpose of this class is to track changes to the logical font. this is
//  generally useful for caching changes to a font selection.

struct LogFont : LOGFONT
{
	LogFont( void )								{  Reset();  }
	LogFont( const char* name, int size )		{  Reset();  SetName( name );  SetHeightPoints( size );  }
   ~LogFont( void )								{  }				// $ nonvirtual due to LOGFONT

	void Reset( void );

	// these return true if there was a change
	bool SetWeight         ( int weight );
	bool SetBold           ( bool enable = true );
	bool SetItalic         ( bool enable = true );
	bool SetUnderline      ( bool enable = true );
	bool SetStrikeout      ( bool enable = true );
	bool SetPitchAndFamily ( BYTE value );
	bool SetHeight         ( int height );

	// uses screen if NULL dc given
	bool SetHeightPoints ( int points, HDC dc = NULL );
	bool SetName         ( const char* name );

	// operators
	operator LOGFONT* ( void )  {  return ( this );  }
	operator const LOGFONT* ( void ) const  {  return ( this );  }
};

//////////////////////////////////////////////////////////////////////////////
// class AutoRedraw declaration

class AutoRedraw
{
public:
	SET_NO_INHERITED( AutoRedraw );

	AutoRedraw( HWND hwnd, bool set = false )
	{
		m_Hwnd = hwnd;
		m_Set = set;
		gpassert( ::IsWindow( m_Hwnd ) );
		::SendMessage( m_Hwnd, WM_SETREDRAW, m_Set, 0 );
	}

   ~AutoRedraw( void )
	{
		::SendMessage( m_Hwnd, WM_SETREDRAW, !m_Set, 0 );
	}

private:
	HWND m_Hwnd;
	bool m_Set;

	SET_NO_COPYING( AutoRedraw );
};

//////////////////////////////////////////////////////////////////////////////
// class Font declaration

// the purpose of this class is to wrap HFONTs. make sure they get deleted etc.

class Font
{
public:
	SET_NO_INHERITED( Font );

	// note: if DeleteObject() fails the font may still be selected into a
	//  DC note that the font must exist in order to be reset.

	Font( void ) : m_Font( NULL)  {  }
   ~Font( void )  {  Reset();  }

	bool  CreateStock( int stockFont = SYSTEM_FONT );
	bool  Create     ( const LogFont& logFont );
	void  Reset      ( void );
	HFONT Release    ( void )  {  HFONT font = m_Font;  m_Font = NULL;  return ( font );  }
	HFONT Get        ( void ) const  {  gpassert( DoesExist() );  return ( m_Font );  }
	bool  DoesExist  ( void ) const  {  return ( m_Font != NULL );  }

	operator HFONT   ( void ) const  {  return ( Get() );  }

private:
	HFONT m_Font;

	SET_NO_COPYING( Font );
};

//////////////////////////////////////////////////////////////////////////////
// class Brush declaration

// the purpose of this class is to wrap HBRUSHes. make sure they get deleted etc.

class Brush
{
public:
	SET_NO_INHERITED( Brush );

	// note: if DeleteObject() fails the brush may still be selected into a
	//  DC note that the brush must exist in order to be reset.

	Brush( void ) : m_Brush( NULL)  {  }
   ~Brush( void )  {  Reset();  }

	bool   CreateHatch( int style, COLORREF fg );
	void   Reset      ( void );
	HBRUSH Get        ( void ) const  {  gpassert( DoesExist() );  return ( m_Brush );  }
	bool   DoesExist  ( void ) const  {  return ( m_Brush != NULL );  }

	operator HBRUSH   ( void ) const  {  return ( Get() );  }

private:
	HBRUSH m_Brush;

	SET_NO_COPYING( Brush );
};

//////////////////////////////////////////////////////////////////////////////
// class ObjectSelector declaration

// adapted from old win32_select_object - auto-unselects object on dtor

class ObjectSelector
{
public:
	SET_NO_INHERITED( ObjectSelector );

	ObjectSelector( HDC dc, HGDIOBJ obj )
	{
		m_DeviceContext = dc;
		m_OldObject = ::SelectObject( m_DeviceContext, obj );		// select object in
	}

   ~ObjectSelector( void )
	{
		::SelectObject( m_DeviceContext, m_OldObject );				// select it back out
	}

private:
	// used for state restoration on destruction
	HDC     m_DeviceContext;
	HGDIOBJ m_OldObject;

	SET_NO_COPYING( ObjectSelector );
};

//////////////////////////////////////////////////////////////////////////////
// class WaitCursor declaration

class WaitCursor
{
public:
	SET_NO_INHERITED( WaitCursor );

	WaitCursor( void )
	{
		if ( ms_WaitCount++ == 0 )
		{
			ms_OldCursor = ::SetCursor( ::LoadCursor( NULL, IDC_WAIT ) );
		}
	}

   ~WaitCursor( void )
	{
		if ( --ms_WaitCount == 0 )
		{
			::SetCursor( ms_OldCursor );
		}
	}

private:
	static int     ms_WaitCount;
	static HCURSOR ms_OldCursor;

	SET_NO_COPYING( WaitCursor );
};

//////////////////////////////////////////////////////////////////////////////
// safe windowsx clone

	// $ these functions are copied from windowsx.h and converted from macros
	//   to inline functions. can't use windowsx because macros don't obey
	//   scope and there are a lot of windowx macros that are the same as
	//   common function names like DeleteFont.

// Message crackers.

#	define HANDLE_MSG(hwnd, message, fn)    \
		case (message): return HANDLE_##message((hwnd), (wParam), (lParam), (fn))

	// void Cls_OnCompacting(HWND hwnd, UINT compactRatio)
#	define HANDLE_WM_COMPACTING(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (UINT)(wParam)), 0)
#	define FORWARD_WM_COMPACTING(hwnd, compactRatio, fn) \
		(void)(fn)((hwnd), WM_COMPACTING, (WPARAM)(UINT)(compactRatio), 0)

	// void Cls_OnWinIniChange(HWND hwnd, LPCTSTR lpszSectionName)
#	define HANDLE_WM_WININICHANGE(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (LPCTSTR)(lParam)), 0)
#	define FORWARD_WM_WININICHANGE(hwnd, lpszSectionName, fn) \
		(void)(fn)((hwnd), WM_WININICHANGE, 0, (LPARAM)(LPCTSTR)(lpszSectionName))

	// void Cls_OnSysColorChange(HWND hwnd)
#	define HANDLE_WM_SYSCOLORCHANGE(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_SYSCOLORCHANGE(hwnd, fn) \
		(void)(fn)((hwnd), WM_SYSCOLORCHANGE, 0, 0)

	// BOOL Cls_OnQueryNewPalette(HWND hwnd)
#	define HANDLE_WM_QUERYNEWPALETTE(hwnd, wParam, lParam, fn) \
		MAKELRESULT((BOOL)(fn)(hwnd), 0)
#	define FORWARD_WM_QUERYNEWPALETTE(hwnd, fn) \
		(BOOL)(DWORD)(fn)((hwnd), WM_QUERYNEWPALETTE, 0, 0)

	// void Cls_OnPaletteIsChanging(HWND hwnd, HWND hwndPaletteChange)
#	define HANDLE_WM_PALETTEISCHANGING(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(wParam)), 0)
#	define FORWARD_WM_PALETTEISCHANGING(hwnd, hwndPaletteChange, fn) \
		(void)(fn)((hwnd), WM_PALETTEISCHANGING, (WPARAM)(HWND)(hwndPaletteChange), 0)

	// void Cls_OnPaletteChanged(HWND hwnd, HWND hwndPaletteChange)
#	define HANDLE_WM_PALETTECHANGED(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(wParam)), 0)
#	define FORWARD_WM_PALETTECHANGED(hwnd, hwndPaletteChange, fn) \
		(void)(fn)((hwnd), WM_PALETTECHANGED, (WPARAM)(HWND)(hwndPaletteChange), 0)

	// void Cls_OnFontChange(HWND hwnd)
#	define HANDLE_WM_FONTCHANGE(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_FONTCHANGE(hwnd, fn) \
		(void)(fn)((hwnd), WM_FONTCHANGE, 0, 0)

	// void Cls_OnSpoolerStatus(HWND hwnd, UINT status, int cJobInQueue)
#	define HANDLE_WM_SPOOLERSTATUS(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (UINT)(wParam), (int)(short)LOWORD(lParam)), 0)
#	define FORWARD_WM_SPOOLERSTATUS(hwnd, status, cJobInQueue, fn) \
		(void)(fn)((hwnd), WM_SPOOLERSTATUS, (WPARAM)(status), MAKELPARAM((cJobInQueue), 0))

	// void Cls_OnDevModeChange(HWND hwnd, LPCTSTR lpszDeviceName)
#	define HANDLE_WM_DEVMODECHANGE(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (LPCTSTR)(lParam)), 0)
#	define FORWARD_WM_DEVMODECHANGE(hwnd, lpszDeviceName, fn) \
		(void)(fn)((hwnd), WM_DEVMODECHANGE, 0, (LPARAM)(LPCTSTR)(lpszDeviceName))

	// void Cls_OnTimeChange(HWND hwnd)
#	define HANDLE_WM_TIMECHANGE(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_TIMECHANGE(hwnd, fn) \
		(void)(fn)((hwnd), WM_TIMECHANGE, 0, 0)

	// void Cls_OnPower(HWND hwnd, int code)
#	define HANDLE_WM_POWER(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (int)(wParam)), 0)
#	define FORWARD_WM_POWER(hwnd, code, fn) \
		(void)(fn)((hwnd), WM_POWER, (WPARAM)(int)(code), 0)

	// BOOL Cls_OnQueryEndSession(HWND hwnd)
#	define HANDLE_WM_QUERYENDSESSION(hwnd, wParam, lParam, fn) \
		MAKELRESULT((BOOL)(fn)(hwnd), 0)
#	define FORWARD_WM_QUERYENDSESSION(hwnd, fn) \
		(BOOL)(DWORD)(fn)((hwnd), WM_QUERYENDSESSION, 0, 0)

	// void Cls_OnEndSession(HWND hwnd, BOOL fEnding)
#	define HANDLE_WM_ENDSESSION(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (BOOL)(wParam)), 0)
#	define FORWARD_WM_ENDSESSION(hwnd, fEnding, fn) \
		(void)(fn)((hwnd), WM_ENDSESSION, (WPARAM)(BOOL)(fEnding), 0)

	// void Cls_OnQuit(HWND hwnd, int exitCode)
#	define HANDLE_WM_QUIT(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (int)(wParam)), 0)
#	define FORWARD_WM_QUIT(hwnd, exitCode, fn) \
		(void)(fn)((hwnd), WM_QUIT, (WPARAM)(exitCode), 0)

	// BOOL Cls_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
#	define HANDLE_WM_CREATE(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (LPCREATESTRUCT)(lParam)) ? 0 : (LRESULT)-1L)
#	define FORWARD_WM_CREATE(hwnd, lpCreateStruct, fn) \
		(BOOL)(DWORD)(fn)((hwnd), WM_CREATE, 0, (LPARAM)(LPCREATESTRUCT)(lpCreateStruct))

	// BOOL Cls_OnNCCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
#	define HANDLE_WM_NCCREATE(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(BOOL)(fn)((hwnd), (LPCREATESTRUCT)(lParam))
#	define FORWARD_WM_NCCREATE(hwnd, lpCreateStruct, fn) \
		(BOOL)(DWORD)(fn)((hwnd), WM_NCCREATE, 0, (LPARAM)(LPCREATESTRUCT)(lpCreateStruct))

	// void Cls_OnDestroy(HWND hwnd)
#	define HANDLE_WM_DESTROY(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_DESTROY(hwnd, fn) \
		(void)(fn)((hwnd), WM_DESTROY, 0, 0)

	// void Cls_OnNCDestroy(HWND hwnd)
#	define HANDLE_WM_NCDESTROY(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_NCDESTROY(hwnd, fn) \
		(void)(fn)((hwnd), WM_NCDESTROY, 0, 0)

	// void Cls_OnShowWindow(HWND hwnd, BOOL fShow, UINT status)
#	define HANDLE_WM_SHOWWINDOW(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (BOOL)(wParam), (UINT)(lParam)), 0)
#	define FORWARD_WM_SHOWWINDOW(hwnd, fShow, status, fn) \
		(void)(fn)((hwnd), WM_SHOWWINDOW, (WPARAM)(BOOL)(fShow), (LPARAM)(UINT)(status))

	// void Cls_OnSetRedraw(HWND hwnd, BOOL fRedraw)
#	define HANDLE_WM_SETREDRAW(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (BOOL)(wParam)), 0)
#	define FORWARD_WM_SETREDRAW(hwnd, fRedraw, fn) \
		(void)(fn)((hwnd), WM_SETREDRAW, (WPARAM)(BOOL)(fRedraw), 0)

	// void Cls_OnEnable(HWND hwnd, BOOL fEnable)
#	define HANDLE_WM_ENABLE(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (BOOL)(wParam)), 0)
#	define FORWARD_WM_ENABLE(hwnd, fEnable, fn) \
		(void)(fn)((hwnd), WM_ENABLE, (WPARAM)(BOOL)(fEnable), 0)

	// void Cls_OnSetText(HWND hwnd, LPCTSTR lpszText)
#	define HANDLE_WM_SETTEXT(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (LPCTSTR)(lParam)), 0)
#	define FORWARD_WM_SETTEXT(hwnd, lpszText, fn) \
		(void)(fn)((hwnd), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)(lpszText))

	// INT Cls_OnGetText(HWND hwnd, int cchTextMax, LPTSTR lpszText)
#	define HANDLE_WM_GETTEXT(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(int)(fn)((hwnd), (int)(wParam), (LPTSTR)(lParam))
#	define FORWARD_WM_GETTEXT(hwnd, cchTextMax, lpszText, fn) \
		(int)(DWORD)(fn)((hwnd), WM_GETTEXT, (WPARAM)(int)(cchTextMax), (LPARAM)(LPTSTR)(lpszText))

	// INT Cls_OnGetTextLength(HWND hwnd)
#	define HANDLE_WM_GETTEXTLENGTH(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(int)(fn)(hwnd)
#	define FORWARD_WM_GETTEXTLENGTH(hwnd, fn) \
		(int)(DWORD)(fn)((hwnd), WM_GETTEXTLENGTH, 0, 0)

	// BOOL Cls_OnWindowPosChanging(HWND hwnd, LPWINDOWPOS lpwpos)
#	define HANDLE_WM_WINDOWPOSCHANGING(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(BOOL)(fn)((hwnd), (LPWINDOWPOS)(lParam))
#	define FORWARD_WM_WINDOWPOSCHANGING(hwnd, lpwpos, fn) \
		(BOOL)(DWORD)(fn)((hwnd), WM_WINDOWPOSCHANGING, 0, (LPARAM)(LPWINDOWPOS)(lpwpos))

	// void Cls_OnWindowPosChanged(HWND hwnd, const LPWINDOWPOS lpwpos)
#	define HANDLE_WM_WINDOWPOSCHANGED(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (const LPWINDOWPOS)(lParam)), 0)
#	define FORWARD_WM_WINDOWPOSCHANGED(hwnd, lpwpos, fn) \
		(void)(fn)((hwnd), WM_WINDOWPOSCHANGED, 0, (LPARAM)(const LPWINDOWPOS)(lpwpos))

	// void Cls_OnMove(HWND hwnd, int x, int y)
#	define HANDLE_WM_MOVE(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)), 0)
#	define FORWARD_WM_MOVE(hwnd, x, y, fn) \
		(void)(fn)((hwnd), WM_MOVE, 0, MAKELPARAM((x), (y)))

	// void Cls_OnSize(HWND hwnd, UINT state, int cx, int cy)
#	define HANDLE_WM_SIZE(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (UINT)(wParam), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)), 0)
#	define FORWARD_WM_SIZE(hwnd, state, cx, cy, fn) \
		(void)(fn)((hwnd), WM_SIZE, (WPARAM)(UINT)(state), MAKELPARAM((cx), (cy)))

	// void Cls_OnClose(HWND hwnd)
#	define HANDLE_WM_CLOSE(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_CLOSE(hwnd, fn) \
		(void)(fn)((hwnd), WM_CLOSE, 0, 0)

	// BOOL Cls_OnQueryOpen(HWND hwnd)
#	define HANDLE_WM_QUERYOPEN(hwnd, wParam, lParam, fn) \
		MAKELRESULT((BOOL)(fn)(hwnd), 0)
#	define FORWARD_WM_QUERYOPEN(hwnd, fn) \
		(BOOL)(DWORD)(fn)((hwnd), WM_QUERYOPEN, 0, 0)

	// void Cls_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
#	define HANDLE_WM_GETMINMAXINFO(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (LPMINMAXINFO)(lParam)), 0)
#	define FORWARD_WM_GETMINMAXINFO(hwnd, lpMinMaxInfo, fn) \
		(void)(fn)((hwnd), WM_GETMINMAXINFO, 0, (LPARAM)(LPMINMAXINFO)(lpMinMaxInfo))

	// void Cls_OnPaint(HWND hwnd)
#	define HANDLE_WM_PAINT(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_PAINT(hwnd, fn) \
		(void)(fn)((hwnd), WM_PAINT, 0, 0)

	// BOOL Cls_OnEraseBkgnd(HWND hwnd, HDC hdc)
#	define HANDLE_WM_ERASEBKGND(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(BOOL)(fn)((hwnd), (HDC)(wParam))
#	define FORWARD_WM_ERASEBKGND(hwnd, hdc, fn) \
	   (BOOL)(DWORD)(fn)((hwnd), WM_ERASEBKGND, (WPARAM)(HDC)(hdc), 0)

	// BOOL Cls_OnIconEraseBkgnd(HWND hwnd, HDC hdc)
#	define HANDLE_WM_ICONERASEBKGND(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(BOOL)(fn)((hwnd), (HDC)(wParam))
#	define FORWARD_WM_ICONERASEBKGND(hwnd, hdc, fn) \
		(BOOL)(DWORD)(fn)((hwnd), WM_ICONERASEBKGND, (WPARAM)(HDC)(hdc), 0)

	// void Cls_OnNCPaint(HWND hwnd, HRGN hrgn)
#	define HANDLE_WM_NCPAINT(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HRGN)(wParam)), 0)
#	define FORWARD_WM_NCPAINT(hwnd, hrgn, fn) \
		(void)(fn)((hwnd), WM_NCPAINT, (WPARAM)(HRGN)(hrgn), 0)

	// UINT Cls_OnNCCalcSize(HWND hwnd, BOOL fCalcValidRects, NCCALCSIZE_PARAMS * lpcsp)
#	define HANDLE_WM_NCCALCSIZE(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(fn)((hwnd), (BOOL)(0), (NCCALCSIZE_PARAMS *)(lParam))
#	define FORWARD_WM_NCCALCSIZE(hwnd, fCalcValidRects, lpcsp, fn) \
		(UINT)(DWORD)(fn)((hwnd), WM_NCCALCSIZE, 0, (LPARAM)(NCCALCSIZE_PARAMS *)(lpcsp))

	// UINT Cls_OnNCHitTest(HWND hwnd, int x, int y)
#	define HANDLE_WM_NCHITTEST(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam))
#	define FORWARD_WM_NCHITTEST(hwnd, x, y, fn) \
		(UINT)(DWORD)(fn)((hwnd), WM_NCHITTEST, 0, MAKELPARAM((x), (y)))

	// HICON Cls_OnQueryDragIcon(HWND hwnd)
#	define HANDLE_WM_QUERYDRAGICON(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(fn)(hwnd)
#	define FORWARD_WM_QUERYDRAGICON(hwnd, fn) \
		(HICON)(UINT)(DWORD)(fn)((hwnd), WM_QUERYDRAGICON, 0, 0)

#	ifdef _INC_SHELLAPI
	// void Cls_OnDropFiles(HWND hwnd, HDROP hdrop)
#	define HANDLE_WM_DROPFILES(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HDROP)(wParam)), 0)
#	define FORWARD_WM_DROPFILES(hwnd, hdrop, fn) \
		(void)(fn)((hwnd), WM_DROPFILES, (WPARAM)(HDROP)(hdrop), 0)
#	endif  // _INC_SHELLAPI

	// void Cls_OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
#	define HANDLE_WM_ACTIVATE(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (UINT)LOWORD(wParam), (HWND)(lParam), (BOOL)HIWORD(wParam)), 0)
#	define FORWARD_WM_ACTIVATE(hwnd, state, hwndActDeact, fMinimized, fn) \
		(void)(fn)((hwnd), WM_ACTIVATE, MAKEWPARAM((state), (fMinimized)), (LPARAM)(HWND)(hwndActDeact))

	// void Cls_OnActivateApp(HWND hwnd, BOOL fActivate, DWORD dwThreadId)
#	define HANDLE_WM_ACTIVATEAPP(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (BOOL)(wParam), (DWORD)(lParam)), 0)
#	define FORWARD_WM_ACTIVATEAPP(hwnd, fActivate, dwThreadId, fn) \
		(void)(fn)((hwnd), WM_ACTIVATEAPP, (WPARAM)(BOOL)(fActivate), (LPARAM)(dwThreadId))

	// BOOL Cls_OnNCActivate(HWND hwnd, BOOL fActive, HWND hwndActDeact, BOOL fMinimized)
#	define HANDLE_WM_NCACTIVATE(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(BOOL)(fn)((hwnd), (BOOL)(wParam), 0, 0)
#	define FORWARD_WM_NCACTIVATE(hwnd, fActive, hwndActDeact, fMinimized, fn) \
		(BOOL)(DWORD)(fn)((hwnd), WM_NCACTIVATE, (WPARAM)(BOOL)(fActive), 0)

	// void Cls_OnSetFocus(HWND hwnd, HWND hwndOldFocus)
#	define HANDLE_WM_SETFOCUS(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(wParam)), 0)
#	define FORWARD_WM_SETFOCUS(hwnd, hwndOldFocus, fn) \
		(void)(fn)((hwnd), WM_SETFOCUS, (WPARAM)(HWND)(hwndOldFocus), 0)

	// void Cls_OnKillFocus(HWND hwnd, HWND hwndNewFocus)
#	define HANDLE_WM_KILLFOCUS(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(wParam)), 0)
#	define FORWARD_WM_KILLFOCUS(hwnd, hwndNewFocus, fn) \
		(void)(fn)((hwnd), WM_KILLFOCUS, (WPARAM)(HWND)(hwndNewFocus), 0)

	// void Cls_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
#	define HANDLE_WM_KEYDOWN(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (UINT)(wParam), TRUE, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam)), 0)
#	define FORWARD_WM_KEYDOWN(hwnd, vk, cRepeat, flags, fn) \
		(void)(fn)((hwnd), WM_KEYDOWN, (WPARAM)(UINT)(vk), MAKELPARAM((cRepeat), (flags)))

	// void Cls_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
#	define HANDLE_WM_KEYUP(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (UINT)(wParam), FALSE, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam)), 0)
#	define FORWARD_WM_KEYUP(hwnd, vk, cRepeat, flags, fn) \
		(void)(fn)((hwnd), WM_KEYUP, (WPARAM)(UINT)(vk), MAKELPARAM((cRepeat), (flags)))

	// void Cls_OnChar(HWND hwnd, TCHAR ch, int cRepeat)
#	define HANDLE_WM_CHAR(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (TCHAR)(wParam), (int)(short)LOWORD(lParam)), 0)
#	define FORWARD_WM_CHAR(hwnd, ch, cRepeat, fn) \
		(void)(fn)((hwnd), WM_CHAR, (WPARAM)(TCHAR)(ch), MAKELPARAM((cRepeat),0))

	// void Cls_OnDeadChar(HWND hwnd, TCHAR ch, int cRepeat)
#	define HANDLE_WM_DEADCHAR(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (TCHAR)(wParam), (int)(short)LOWORD(lParam)), 0)
#	define FORWARD_WM_DEADCHAR(hwnd, ch, cRepeat, fn) \
		(void)(fn)((hwnd), WM_DEADCHAR, (WPARAM)(TCHAR)(ch), MAKELPARAM((cRepeat),0))

	// void Cls_OnSysKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
#	define HANDLE_WM_SYSKEYDOWN(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (UINT)(wParam), TRUE, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam)), 0)
#	define FORWARD_WM_SYSKEYDOWN(hwnd, vk, cRepeat, flags, fn) \
		(void)(fn)((hwnd), WM_SYSKEYDOWN, (WPARAM)(UINT)(vk), MAKELPARAM((cRepeat), (flags)))

	// void Cls_OnSysKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
#	define HANDLE_WM_SYSKEYUP(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (UINT)(wParam), FALSE, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam)), 0)
#	define FORWARD_WM_SYSKEYUP(hwnd, vk, cRepeat, flags, fn) \
		(void)(fn)((hwnd), WM_SYSKEYUP, (WPARAM)(UINT)(vk), MAKELPARAM((cRepeat), (flags)))

	// void Cls_OnSysChar(HWND hwnd, TCHAR ch, int cRepeat)
#	define HANDLE_WM_SYSCHAR(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (TCHAR)(wParam), (int)(short)LOWORD(lParam)), 0)
#	define FORWARD_WM_SYSCHAR(hwnd, ch, cRepeat, fn) \
		(void)(fn)((hwnd), WM_SYSCHAR, (WPARAM)(TCHAR)(ch), MAKELPARAM((cRepeat), 0))

	// void Cls_OnSysDeadChar(HWND hwnd, TCHAR ch, int cRepeat)
#	define HANDLE_WM_SYSDEADCHAR(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (TCHAR)(wParam), (int)(short)LOWORD(lParam)), 0)
#	define FORWARD_WM_SYSDEADCHAR(hwnd, ch, cRepeat, fn) \
		(void)(fn)((hwnd), WM_SYSDEADCHAR, (WPARAM)(TCHAR)(ch), MAKELPARAM((cRepeat), 0))

	// void Cls_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
#	define HANDLE_WM_MOUSEMOVE(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)
#	define FORWARD_WM_MOUSEMOVE(hwnd, x, y, keyFlags, fn) \
		(void)(fn)((hwnd), WM_MOUSEMOVE, (WPARAM)(UINT)(keyFlags), MAKELPARAM((x), (y)))

	// void Cls_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
#	define HANDLE_WM_LBUTTONDOWN(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)
#	define FORWARD_WM_LBUTTONDOWN(hwnd, fDoubleClick, x, y, keyFlags, fn) \
		(void)(fn)((hwnd), (fDoubleClick) ? WM_LBUTTONDBLCLK : WM_LBUTTONDOWN, (WPARAM)(UINT)(keyFlags), MAKELPARAM((x), (y)))

	// void Cls_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
#	define HANDLE_WM_LBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)

	// void Cls_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
#	define HANDLE_WM_LBUTTONUP(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)
#	define FORWARD_WM_LBUTTONUP(hwnd, x, y, keyFlags, fn) \
		(void)(fn)((hwnd), WM_LBUTTONUP, (WPARAM)(UINT)(keyFlags), MAKELPARAM((x), (y)))

	// void Cls_OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
#	define HANDLE_WM_RBUTTONDOWN(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)
#	define FORWARD_WM_RBUTTONDOWN(hwnd, fDoubleClick, x, y, keyFlags, fn) \
		(void)(fn)((hwnd), (fDoubleClick) ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN, (WPARAM)(UINT)(keyFlags), MAKELPARAM((x), (y)))

	// void Cls_OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
#	define HANDLE_WM_RBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)

	// void Cls_OnRButtonUp(HWND hwnd, int x, int y, UINT flags)
#	define HANDLE_WM_RBUTTONUP(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)
#	define FORWARD_WM_RBUTTONUP(hwnd, x, y, keyFlags, fn) \
		(void)(fn)((hwnd), WM_RBUTTONUP, (WPARAM)(UINT)(keyFlags), MAKELPARAM((x), (y)))

	// void Cls_OnMButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
#	define HANDLE_WM_MBUTTONDOWN(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)
#	define FORWARD_WM_MBUTTONDOWN(hwnd, fDoubleClick, x, y, keyFlags, fn) \
		(void)(fn)((hwnd), (fDoubleClick) ? WM_MBUTTONDBLCLK : WM_MBUTTONDOWN, (WPARAM)(UINT)(keyFlags), MAKELPARAM((x), (y)))

	// void Cls_OnMButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
#	define HANDLE_WM_MBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)

	// void Cls_OnMButtonUp(HWND hwnd, int x, int y, UINT flags)
#	define HANDLE_WM_MBUTTONUP(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)
#	define FORWARD_WM_MBUTTONUP(hwnd, x, y, keyFlags, fn) \
		(void)(fn)((hwnd), WM_MBUTTONUP, (WPARAM)(UINT)(keyFlags), MAKELPARAM((x), (y)))

	// void Cls_OnNCMouseMove(HWND hwnd, int x, int y, UINT codeHitTest)
#	define HANDLE_WM_NCMOUSEMOVE(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)
#	define FORWARD_WM_NCMOUSEMOVE(hwnd, x, y, codeHitTest, fn) \
		(void)(fn)((hwnd), WM_NCMOUSEMOVE, (WPARAM)(UINT)(codeHitTest), MAKELPARAM((x), (y)))

	// void Cls_OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
#	define HANDLE_WM_NCLBUTTONDOWN(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)
#	define FORWARD_WM_NCLBUTTONDOWN(hwnd, fDoubleClick, x, y, codeHitTest, fn) \
		(void)(fn)((hwnd), (fDoubleClick) ? WM_NCLBUTTONDBLCLK : WM_NCLBUTTONDOWN, (WPARAM)(UINT)(codeHitTest), MAKELPARAM((x), (y)))

	// void Cls_OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
#	define HANDLE_WM_NCLBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)

	// void Cls_OnNCLButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
#	define HANDLE_WM_NCLBUTTONUP(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)
#	define FORWARD_WM_NCLBUTTONUP(hwnd, x, y, codeHitTest, fn) \
		(void)(fn)((hwnd), WM_NCLBUTTONUP, (WPARAM)(UINT)(codeHitTest), MAKELPARAM((x), (y)))

	// void Cls_OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
#	define HANDLE_WM_NCRBUTTONDOWN(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)
#	define FORWARD_WM_NCRBUTTONDOWN(hwnd, fDoubleClick, x, y, codeHitTest, fn) \
		(void)(fn)((hwnd), (fDoubleClick) ? WM_NCRBUTTONDBLCLK : WM_NCRBUTTONDOWN, (WPARAM)(UINT)(codeHitTest), MAKELPARAM((x), (y)) )

	// void Cls_OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
#	define HANDLE_WM_NCRBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)

	// void Cls_OnNCRButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
#	define HANDLE_WM_NCRBUTTONUP(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)
#	define FORWARD_WM_NCRBUTTONUP(hwnd, x, y, codeHitTest, fn) \
		(void)(fn)((hwnd), WM_NCRBUTTONUP, (WPARAM)(UINT)(codeHitTest), MAKELPARAM((x), (y)) )

	// void Cls_OnNCMButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
#	define HANDLE_WM_NCMBUTTONDOWN(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)
#	define FORWARD_WM_NCMBUTTONDOWN(hwnd, fDoubleClick, x, y, codeHitTest, fn) \
		(void)(fn)((hwnd), (fDoubleClick) ? WM_NCMBUTTONDBLCLK : WM_NCMBUTTONDOWN, (WPARAM)(UINT)(codeHitTest), MAKELPARAM((x), (y)) )

	// void Cls_OnNCMButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
#	define HANDLE_WM_NCMBUTTONDBLCLK(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)

	// void Cls_OnNCMButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
#	define HANDLE_WM_NCMBUTTONUP(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0)
#	define FORWARD_WM_NCMBUTTONUP(hwnd, x, y, codeHitTest, fn) \
		(void)(fn)((hwnd), WM_NCMBUTTONUP, (WPARAM)(UINT)(codeHitTest), MAKELPARAM((x), (y)) )

	// int Cls_OnMouseActivate(HWND hwnd, HWND hwndTopLevel, UINT codeHitTest, UINT msg)
#	define HANDLE_WM_MOUSEACTIVATE(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(int)(fn)((hwnd), (HWND)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(lParam))
#	define FORWARD_WM_MOUSEACTIVATE(hwnd, hwndTopLevel, codeHitTest, msg, fn) \
		(int)(DWORD)(fn)((hwnd), WM_MOUSEACTIVATE, (WPARAM)(HWND)(hwndTopLevel), MAKELPARAM((codeHitTest), (msg)))

	// void Cls_OnCancelMode(HWND hwnd)
#	define HANDLE_WM_CANCELMODE(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_CANCELMODE(hwnd, fn) \
		(void)(fn)((hwnd), WM_CANCELMODE, 0, 0)

	// void Cls_OnTimer(HWND hwnd, UINT id)
#	define HANDLE_WM_TIMER(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (UINT)(wParam)), 0)
#	define FORWARD_WM_TIMER(hwnd, id, fn) \
		(void)(fn)((hwnd), WM_TIMER, (WPARAM)(UINT)(id), 0)

	// void Cls_OnInitMenu(HWND hwnd, HMENU hMenu)
#	define HANDLE_WM_INITMENU(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HMENU)(wParam)), 0)
#	define FORWARD_WM_INITMENU(hwnd, hMenu, fn) \
		(void)(fn)((hwnd), WM_INITMENU, (WPARAM)(HMENU)(hMenu), 0)

	// void Cls_OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
#	define HANDLE_WM_INITMENUPOPUP(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HMENU)(wParam), (UINT)LOWORD(lParam), (BOOL)HIWORD(lParam)), 0)
#	define FORWARD_WM_INITMENUPOPUP(hwnd, hMenu, item, fSystemMenu, fn) \
		(void)(fn)((hwnd), WM_INITMENUPOPUP, (WPARAM)(HMENU)(hMenu), MAKELPARAM((item),(fSystemMenu)))

	// void Cls_OnMenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags)
#	define HANDLE_WM_MENUSELECT(hwnd, wParam, lParam, fn)                  \
		((fn)((hwnd), (HMENU)(lParam),  \
		(HIWORD(wParam) & MF_POPUP) ? 0 : (int)(LOWORD(wParam)),           \
		(HIWORD(wParam) & MF_POPUP) ? GetSubMenu((HMENU)lParam, LOWORD(wParam)) : 0, \
		(UINT)(((short)HIWORD(wParam) == -1) ? 0xFFFFFFFF : HIWORD(wParam))), 0)
#	define FORWARD_WM_MENUSELECT(hwnd, hmenu, item, hmenuPopup, flags, fn) \
		(void)(fn)((hwnd), WM_MENUSELECT, MAKEWPARAM((item), (flags)), (LPARAM)(HMENU)((hmenu) ? (hmenu) : (hmenuPopup)))

	// DWORD Cls_OnMenuChar(HWND hwnd, UINT ch, UINT flags, HMENU hmenu)
#	define HANDLE_WM_MENUCHAR(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(fn)((hwnd), (UINT)(LOWORD(wParam)), (UINT)HIWORD(wParam), (HMENU)(lParam))
#	define FORWARD_WM_MENUCHAR(hwnd, ch, flags, hmenu, fn) \
		(DWORD)(fn)((hwnd), WM_MENUCHAR, MAKEWPARAM(flags, (WORD)(ch)), (LPARAM)(HMENU)(hmenu))

	// void Cls_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
#	define HANDLE_WM_COMMAND(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (int)(LOWORD(wParam)), (HWND)(lParam), (UINT)HIWORD(wParam)), 0)
#	define FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, fn) \
		(void)(fn)((hwnd), WM_COMMAND, MAKEWPARAM((UINT)(id),(UINT)(codeNotify)), (LPARAM)(HWND)(hwndCtl))

	// void Cls_OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
#	define HANDLE_WM_HSCROLL(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(lParam), (UINT)(LOWORD(wParam)), (int)(short)HIWORD(wParam)), 0)
#	define FORWARD_WM_HSCROLL(hwnd, hwndCtl, code, pos, fn) \
		(void)(fn)((hwnd), WM_HSCROLL, MAKEWPARAM((UINT)(int)(code),(UINT)(int)(pos)), (LPARAM)(UINT)(hwndCtl))

	// void Cls_OnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
#	define HANDLE_WM_VSCROLL(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(lParam), (UINT)(LOWORD(wParam)),  (int)(short)HIWORD(wParam)), 0)
#	define FORWARD_WM_VSCROLL(hwnd, hwndCtl, code, pos, fn) \
		(void)(fn)((hwnd), WM_VSCROLL, MAKEWPARAM((UINT)(int)(code), (UINT)(int)(pos)), (LPARAM)(HWND)(hwndCtl))

	// void Cls_OnCut(HWND hwnd)
#	define HANDLE_WM_CUT(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_CUT(hwnd, fn) \
		(void)(fn)((hwnd), WM_CUT, 0, 0)

	// void Cls_OnCopy(HWND hwnd)
#	define HANDLE_WM_COPY(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_COPY(hwnd, fn) \
		(void)(fn)((hwnd), WM_COPY, 0, 0)

	// void Cls_OnPaste(HWND hwnd)
#	define HANDLE_WM_PASTE(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_PASTE(hwnd, fn) \
		(void)(fn)((hwnd), WM_PASTE, 0, 0)

	// void Cls_OnClear(HWND hwnd)
#	define HANDLE_WM_CLEAR(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_CLEAR(hwnd, fn) \
		(void)(fn)((hwnd), WM_CLEAR, 0, 0)

	// void Cls_OnUndo(HWND hwnd)
#	define HANDLE_WM_UNDO(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_UNDO(hwnd, fn) \
		(void)(fn)((hwnd), WM_UNDO, 0, 0)

	// HANDLE Cls_OnRenderFormat(HWND hwnd, UINT fmt)
#	define HANDLE_WM_RENDERFORMAT(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(HANDLE)(fn)((hwnd), (UINT)(wParam))
#	define FORWARD_WM_RENDERFORMAT(hwnd, fmt, fn) \
		(HANDLE)(UINT)(DWORD)(fn)((hwnd), WM_RENDERFORMAT, (WPARAM)(UINT)(fmt), 0)

	// void Cls_OnRenderAllFormats(HWND hwnd)
#	define HANDLE_WM_RENDERALLFORMATS(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_RENDERALLFORMATS(hwnd, fn) \
		(void)(fn)((hwnd), WM_RENDERALLFORMATS, 0, 0)

	// void Cls_OnDestroyClipboard(HWND hwnd)
#	define HANDLE_WM_DESTROYCLIPBOARD(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_DESTROYCLIPBOARD(hwnd, fn) \
		(void)(fn)((hwnd), WM_DESTROYCLIPBOARD, 0, 0)

	// void Cls_OnDrawClipboard(HWND hwnd)
#	define HANDLE_WM_DRAWCLIPBOARD(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_DRAWCLIPBOARD(hwnd, fn) \
		(void)(fn)((hwnd), WM_DRAWCLIPBOARD, 0, 0)

	// void Cls_OnPaintClipboard(HWND hwnd, HWND hwndCBViewer, const LPPAINTSTRUCT lpPaintStruct)
#	define HANDLE_WM_PAINTCLIPBOARD(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(wParam), (const LPPAINTSTRUCT)GlobalLock((HGLOBAL)(lParam))), GlobalUnlock((HGLOBAL)(lParam)), 0)
#	define FORWARD_WM_PAINTCLIPBOARD(hwnd, hwndCBViewer, lpPaintStruct, fn) \
		(void)(fn)((hwnd), WM_PAINTCLIPBOARD, (WPARAM)(HWND)(hwndCBViewer), (LPARAM)(LPPAINTSTRUCT)(lpPaintStruct))

	// void Cls_OnSizeClipboard(HWND hwnd, HWND hwndCBViewer, const LPRECT lprc)
#	define HANDLE_WM_SIZECLIPBOARD(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(wParam), (const LPRECT)GlobalLock((HGLOBAL)(lParam))), GlobalUnlock((HGLOBAL)(lParam)), 0)
#	define FORWARD_WM_SIZECLIPBOARD(hwnd, hwndCBViewer, lprc, fn) \
		(void)(fn)((hwnd), WM_SIZECLIPBOARD, (WPARAM)(HWND)(hwndCBViewer), (LPARAM)(LPRECT)(lprc))

	// void Cls_OnVScrollClipboard(HWND hwnd, HWND hwndCBViewer, UINT code, int pos)
#	define HANDLE_WM_VSCROLLCLIPBOARD(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(wParam), (UINT)LOWORD(lParam), (int)(short)HIWORD(lParam)), 0)
#	define FORWARD_WM_VSCROLLCLIPBOARD(hwnd, hwndCBViewer, code, pos, fn) \
		(void)(fn)((hwnd), WM_VSCROLLCLIPBOARD, (WPARAM)(HWND)(hwndCBViewer), MAKELPARAM((code), (pos)))

	// void Cls_OnHScrollClipboard(HWND hwnd, HWND hwndCBViewer, UINT code, int pos)
#	define HANDLE_WM_HSCROLLCLIPBOARD(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(wParam), (UINT)LOWORD(lParam), (int)(short)HIWORD(lParam)), 0)
#	define FORWARD_WM_HSCROLLCLIPBOARD(hwnd, hwndCBViewer, code, pos, fn) \
		(void)(fn)((hwnd), WM_HSCROLLCLIPBOARD, (WPARAM)(HWND)(hwndCBViewer), MAKELPARAM((code), (pos)))

	// void Cls_OnAskCBFormatName(HWND hwnd, int cchMax, LPTSTR rgchName)
#	define HANDLE_WM_ASKCBFORMATNAME(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (int)(wParam), (LPTSTR)(lParam)), 0)
#	define FORWARD_WM_ASKCBFORMATNAME(hwnd, cchMax, rgchName, fn) \
		(void)(fn)((hwnd), WM_ASKCBFORMATNAME, (WPARAM)(int)(cchMax), (LPARAM)(rgchName))

	// void Cls_OnChangeCBChain(HWND hwnd, HWND hwndRemove, HWND hwndNext)
#	define HANDLE_WM_CHANGECBCHAIN(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(wParam), (HWND)(lParam)), 0)
#	define FORWARD_WM_CHANGECBCHAIN(hwnd, hwndRemove, hwndNext, fn) \
		(void)(fn)((hwnd), WM_CHANGECBCHAIN, (WPARAM)(HWND)(hwndRemove), (LPARAM)(HWND)(hwndNext))

	// BOOL Cls_OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
#	define HANDLE_WM_SETCURSOR(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(BOOL)(fn)((hwnd), (HWND)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(lParam))
#	define FORWARD_WM_SETCURSOR(hwnd, hwndCursor, codeHitTest, msg, fn) \
		(BOOL)(DWORD)(fn)((hwnd), WM_SETCURSOR, (WPARAM)(HWND)(hwndCursor), MAKELPARAM((codeHitTest), (msg)))

	// void Cls_OnSysCommand(HWND hwnd, UINT cmd, int x, int y)
#	define HANDLE_WM_SYSCOMMAND(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (UINT)(wParam), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)), 0)
#	define FORWARD_WM_SYSCOMMAND(hwnd, cmd, x, y, fn) \
		(void)(fn)((hwnd), WM_SYSCOMMAND, (WPARAM)(UINT)(cmd), MAKELPARAM((x), (y)))

	// HWND Cls_MDICreate(HWND hwnd, const LPMDICREATESTRUCT lpmcs)
#	define HANDLE_WM_MDICREATE(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(fn)((hwnd), (LPMDICREATESTRUCT)(lParam))
#	define FORWARD_WM_MDICREATE(hwnd, lpmcs, fn) \
		(HWND)(UINT)(DWORD)(fn)((hwnd), WM_MDICREATE, 0, (LPARAM)(LPMDICREATESTRUCT)(lpmcs))

	// void Cls_MDIDestroy(HWND hwnd, HWND hwndDestroy)
#	define HANDLE_WM_MDIDESTROY(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(wParam)), 0)
#	define FORWARD_WM_MDIDESTROY(hwnd, hwndDestroy, fn) \
		(void)(fn)((hwnd), WM_MDIDESTROY, (WPARAM)(hwndDestroy), 0)

	// NOTE: Usable only by MDI client windows
	// void Cls_MDIActivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate)
#	define HANDLE_WM_MDIACTIVATE(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (BOOL)(lParam == (LPARAM)hwnd), (HWND)(lParam), (HWND)(wParam)), 0)
#	define FORWARD_WM_MDIACTIVATE(hwnd, fActive, hwndActivate, hwndDeactivate, fn) \
		(void)(fn)(hwnd, WM_MDIACTIVATE, (WPARAM)(hwndDeactivate), (LPARAM)(hwndActivate))

	// void Cls_MDIRestore(HWND hwnd, HWND hwndRestore)
#	define HANDLE_WM_MDIRESTORE(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(wParam)), 0)
#	define FORWARD_WM_MDIRESTORE(hwnd, hwndRestore, fn) \
		(void)(fn)((hwnd), WM_MDIRESTORE, (WPARAM)(hwndRestore), 0)

	// HWND Cls_MDINext(HWND hwnd, HWND hwndCur, BOOL fPrev)
#	define HANDLE_WM_MDINEXT(hwnd, wParam, lParam, fn) \
		(LRESULT)(HWND)(fn)((hwnd), (HWND)(wParam), (BOOL)lParam)
#	define FORWARD_WM_MDINEXT(hwnd, hwndCur, fPrev, fn) \
		(HWND)(UINT)(DWORD)(fn)((hwnd), WM_MDINEXT, (WPARAM)(hwndCur), (LPARAM)(fPrev))

	// void Cls_MDIMaximize(HWND hwnd, HWND hwndMaximize)
#	define HANDLE_WM_MDIMAXIMIZE(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(wParam)), 0)
#	define FORWARD_WM_MDIMAXIMIZE(hwnd, hwndMaximize, fn) \
		(void)(fn)((hwnd), WM_MDIMAXIMIZE, (WPARAM)(hwndMaximize), 0)

	// BOOL Cls_MDITile(HWND hwnd, UINT cmd)
#	define HANDLE_WM_MDITILE(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(fn)((hwnd), (UINT)(wParam))
#	define FORWARD_WM_MDITILE(hwnd, cmd, fn) \
		(BOOL)(DWORD)(fn)((hwnd), WM_MDITILE, (WPARAM)(cmd), 0)

	// BOOL Cls_MDICascade(HWND hwnd, UINT cmd)
#	define HANDLE_WM_MDICASCADE(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(fn)((hwnd), (UINT)(wParam))
#	define FORWARD_WM_MDICASCADE(hwnd, cmd, fn) \
		(BOOL)(DWORD)(fn)((hwnd), WM_MDICASCADE, (WPARAM)(cmd), 0)

	// void Cls_MDIIconArrange(HWND hwnd)
#	define HANDLE_WM_MDIICONARRANGE(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_MDIICONARRANGE(hwnd, fn) \
		(void)(fn)((hwnd), WM_MDIICONARRANGE, 0, 0)

	// HWND Cls_MDIGetActive(HWND hwnd)
#	define HANDLE_WM_MDIGETACTIVE(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(fn)(hwnd)
#	define FORWARD_WM_MDIGETACTIVE(hwnd, fn) \
		(HWND)(UINT)(DWORD)(fn)((hwnd), WM_MDIGETACTIVE, 0, 0)

	// HMENU Cls_MDISetMenu(HWND hwnd, BOOL fRefresh, HMENU hmenuFrame, HMENU hmenuWindow)
#	define HANDLE_WM_MDISETMENU(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(fn)((hwnd), (BOOL)(wParam), (HMENU)(wParam), (HMENU)(lParam))
#	define FORWARD_WM_MDISETMENU(hwnd, fRefresh, hmenuFrame, hmenuWindow, fn) \
		(HMENU)(UINT)(DWORD)(fn)((hwnd), WM_MDISETMENU, (WPARAM)((fRefresh) ? (hmenuFrame) : 0), (LPARAM)(hmenuWindow))

	// void Cls_OnChildActivate(HWND hwnd)
#	define HANDLE_WM_CHILDACTIVATE(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_CHILDACTIVATE(hwnd, fn) \
		(void)(fn)((hwnd), WM_CHILDACTIVATE, 0, 0)

	// BOOL Cls_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
#	define HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(BOOL)(fn)((hwnd), (HWND)(wParam), lParam)
#	define FORWARD_WM_INITDIALOG(hwnd, hwndFocus, lParam, fn) \
		(BOOL)(DWORD)(fn)((hwnd), WM_INITDIALOG, (WPARAM)(HWND)(hwndFocus), (lParam))

	// HWND Cls_OnNextDlgCtl(HWND hwnd, HWND hwndSetFocus, BOOL fNext)
#	define HANDLE_WM_NEXTDLGCTL(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(HWND)(fn)((hwnd), (HWND)(wParam), (BOOL)(lParam))
#	define FORWARD_WM_NEXTDLGCTL(hwnd, hwndSetFocus, fNext, fn) \
		(HWND)(UINT)(DWORD)(fn)((hwnd), WM_NEXTDLGCTL, (WPARAM)(HWND)(hwndSetFocus), (LPARAM)(fNext))

	// void Cls_OnParentNotify(HWND hwnd, UINT msg, HWND hwndChild, int idChild)
#	define HANDLE_WM_PARENTNOTIFY(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (UINT)LOWORD(wParam), (HWND)(lParam), (UINT)HIWORD(wParam)), 0)
#	define FORWARD_WM_PARENTNOTIFY(hwnd, msg, hwndChild, idChild, fn) \
		(void)(fn)((hwnd), WM_PARENTNOTIFY, MAKEWPARAM(msg, idChild), (LPARAM)(hwndChild))

	// void Cls_OnEnterIdle(HWND hwnd, UINT source, HWND hwndSource)
#	define HANDLE_WM_ENTERIDLE(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (UINT)(wParam), (HWND)(lParam)), 0)
#	define FORWARD_WM_ENTERIDLE(hwnd, source, hwndSource, fn) \
		(void)(fn)((hwnd), WM_ENTERIDLE, (WPARAM)(UINT)(source), (LPARAM)(HWND)(hwndSource))

	// UINT Cls_OnGetDlgCode(HWND hwnd, LPMSG lpmsg)
#	define HANDLE_WM_GETDLGCODE(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(fn)(hwnd, (LPMSG)(lParam))
#	define FORWARD_WM_GETDLGCODE(hwnd, lpmsg, fn) \
		(UINT)(DWORD)(fn)((hwnd), WM_GETDLGCODE, (lpmsg ? lpmsg->wParam : 0), (LPARAM)(LPMSG)(lpmsg))

	// HBRUSH Cls_OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
#	define HANDLE_WM_CTLCOLORMSGBOX(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((hwnd), (HDC)(wParam), (HWND)(lParam), CTLCOLOR_MSGBOX)
#	define FORWARD_WM_CTLCOLORMSGBOX(hwnd, hdc, hwndChild, fn) \
		(HBRUSH)(UINT)(DWORD)(fn)((hwnd), WM_CTLCOLORMSGBOX, (WPARAM)(HDC)(hdc), (LPARAM)(HWND)(hwndChild))

#	define HANDLE_WM_CTLCOLOREDIT(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((hwnd), (HDC)(wParam), (HWND)(lParam), CTLCOLOR_EDIT)
#	define FORWARD_WM_CTLCOLOREDIT(hwnd, hdc, hwndChild, fn) \
		(HBRUSH)(UINT)(DWORD)(fn)((hwnd), WM_CTLCOLOREDIT, (WPARAM)(HDC)(hdc), (LPARAM)(HWND)(hwndChild))

#	define HANDLE_WM_CTLCOLORLISTBOX(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((hwnd), (HDC)(wParam), (HWND)(lParam), CTLCOLOR_LISTBOX)
#	define FORWARD_WM_CTLCOLORLISTBOX(hwnd, hdc, hwndChild, fn) \
		(HBRUSH)(UINT)(DWORD)(fn)((hwnd), WM_CTLCOLORLISTBOX, (WPARAM)(HDC)(hdc), (LPARAM)(HWND)(hwndChild))

#	define HANDLE_WM_CTLCOLORBTN(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((hwnd), (HDC)(wParam), (HWND)(lParam), CTLCOLOR_BTN)
#	define FORWARD_WM_CTLCOLORBTN(hwnd, hdc, hwndChild, fn) \
		(HBRUSH)(UINT)(DWORD)(fn)((hwnd), WM_CTLCOLORBTN, (WPARAM)(HDC)(hdc), (LPARAM)(HWND)(hwndChild))

#	define HANDLE_WM_CTLCOLORDLG(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((hwnd), (HDC)(wParam), (HWND)(lParam), CTLCOLOR_DLG)
#	define FORWARD_WM_CTLCOLORDLG(hwnd, hdc, hwndChild, fn) \
		(HBRUSH)(UINT)(DWORD)(fn)((hwnd), WM_CTLCOLORDLG, (WPARAM)(HDC)(hdc), (LPARAM)(HWND)(hwndChild))

#	define HANDLE_WM_CTLCOLORSCROLLBAR(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((hwnd), (HDC)(wParam), (HWND)(lParam), CTLCOLOR_SCROLLBAR)
#	define FORWARD_WM_CTLCOLORSCROLLBAR(hwnd, hdc, hwndChild, fn) \
		(HBRUSH)(UINT)(DWORD)(fn)((hwnd), WM_CTLCOLORSCROLLBAR, (WPARAM)(HDC)(hdc), (LPARAM)(HWND)(hwndChild))

#	define HANDLE_WM_CTLCOLORSTATIC(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((hwnd), (HDC)(wParam), (HWND)(lParam), CTLCOLOR_STATIC)
#	define FORWARD_WM_CTLCOLORSTATIC(hwnd, hdc, hwndChild, fn) \
		(HBRUSH)(UINT)(DWORD)(fn)((hwnd), WM_CTLCOLORSTATIC, (WPARAM)(HDC)(hdc), (LPARAM)(HWND)(hwndChild))

	// void Cls_OnSetFont(HWND hwndCtl, HFONT hfont, BOOL fRedraw)
#	define HANDLE_WM_SETFONT(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HFONT)(wParam), (BOOL)(lParam)), 0)
#	define FORWARD_WM_SETFONT(hwnd, hfont, fRedraw, fn) \
		(void)(fn)((hwnd), WM_SETFONT, (WPARAM)(HFONT)(hfont), (LPARAM)(BOOL)(fRedraw))

	// HFONT Cls_OnGetFont(HWND hwnd)
#	define HANDLE_WM_GETFONT(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(UINT)(HFONT)(fn)(hwnd)
#	define FORWARD_WM_GETFONT(hwnd, fn) \
		(HFONT)(UINT)(DWORD)(fn)((hwnd), WM_GETFONT, 0, 0)

	// void Cls_OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem)
#	define HANDLE_WM_DRAWITEM(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (const DRAWITEMSTRUCT *)(lParam)), 0)
#	define FORWARD_WM_DRAWITEM(hwnd, lpDrawItem, fn) \
		(void)(fn)((hwnd), WM_DRAWITEM, (WPARAM)(((const DRAWITEMSTRUCT *)lpDrawItem)->CtlID), (LPARAM)(const DRAWITEMSTRUCT *)(lpDrawItem))

	// void Cls_OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpMeasureItem)
#	define HANDLE_WM_MEASUREITEM(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (MEASUREITEMSTRUCT *)(lParam)), 0)
#	define FORWARD_WM_MEASUREITEM(hwnd, lpMeasureItem, fn) \
		(void)(fn)((hwnd), WM_MEASUREITEM, (WPARAM)(((MEASUREITEMSTRUCT *)lpMeasureItem)->CtlID), (LPARAM)(MEASUREITEMSTRUCT *)(lpMeasureItem))

	// void Cls_OnDeleteItem(HWND hwnd, const DELETEITEMSTRUCT * lpDeleteItem)
#	define HANDLE_WM_DELETEITEM(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (const DELETEITEMSTRUCT *)(lParam)), 0)
#	define FORWARD_WM_DELETEITEM(hwnd, lpDeleteItem, fn) \
		(void)(fn)((hwnd), WM_DELETEITEM, (WPARAM)(((const DELETEITEMSTRUCT *)(lpDeleteItem))->CtlID), (LPARAM)(const DELETEITEMSTRUCT *)(lpDeleteItem))

	// int Cls_OnCompareItem(HWND hwnd, const COMPAREITEMSTRUCT * lpCompareItem)
#	define HANDLE_WM_COMPAREITEM(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(int)(fn)((hwnd), (const COMPAREITEMSTRUCT *)(lParam))
#	define FORWARD_WM_COMPAREITEM(hwnd, lpCompareItem, fn) \
		(int)(DWORD)(fn)((hwnd), WM_COMPAREITEM, (WPARAM)(((const COMPAREITEMSTRUCT *)(lpCompareItem))->CtlID), (LPARAM)(const COMPAREITEMSTRUCT *)(lpCompareItem))

	// int Cls_OnVkeyToItem(HWND hwnd, UINT vk, HWND hwndListbox, int iCaret)
#	define HANDLE_WM_VKEYTOITEM(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(int)(fn)((hwnd), (UINT)LOWORD(wParam), (HWND)(lParam), (int)(short)HIWORD(wParam))
#	define FORWARD_WM_VKEYTOITEM(hwnd, vk, hwndListBox, iCaret, fn) \
		(int)(DWORD)(fn)((hwnd), WM_VKEYTOITEM, MAKEWPARAM((vk), (iCaret)), (LPARAM)(hwndListBox))

	// int Cls_OnCharToItem(HWND hwnd, UINT ch, HWND hwndListbox, int iCaret)
#	define HANDLE_WM_CHARTOITEM(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(int)(fn)((hwnd), (UINT)LOWORD(wParam), (HWND)(lParam), (int)(short)HIWORD(wParam))
#	define FORWARD_WM_CHARTOITEM(hwnd, ch, hwndListBox, iCaret, fn) \
		(int)(DWORD)(fn)((hwnd), WM_CHARTOITEM, MAKEWPARAM((UINT)(ch), (UINT)(iCaret)), (LPARAM)(hwndListBox))

	// void Cls_OnQueueSync(HWND hwnd)
#	define HANDLE_WM_QUEUESYNC(hwnd, wParam, lParam, fn) \
		((fn)(hwnd), 0)
#	define FORWARD_WM_QUEUESYNC(hwnd, fn) \
		(void)(fn)((hwnd), WM_QUEUESYNC, 0, 0)

	// void Cls_OnCommNotify(HWND hwnd, int cid, UINT flags)
#	define HANDLE_WM_COMMNOTIFY(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (int)(wParam), (UINT)LOWORD(lParam)), 0)
#	define FORWARD_WM_COMMNOTIFY(hwnd, cid, flags, fn) \
		(void)(fn)((hwnd), WM_COMMNOTIFY, (WPARAM)(cid), MAKELPARAM((flags), 0))

	// void Cls_OnDisplayChange(HWND hwnd, UINT bitsPerPixel, UINT cxScreen, UINT cyScreen)
#	define HANDLE_WM_DISPLAYCHANGE(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (UINT)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(wParam)), 0)
#	define FORWARD_WM_DISPLAYCHANGE(hwnd, bitsPerPixel, cxScreen, cyScreen, fn) \
		(void)(fn)((hwnd), WM_DISPLAYCHANGE, (WPARAM)(UINT)(bitsPerPixel), (LPARAM)MAKELPARAM((UINT)(cxScreen), (UINT)(cyScreen)))

	// BOOL Cls_OnDeviceChange(HWND hwnd, UINT uEvent, DWORD dwEventData)
#	define HANDLE_WM_DEVICECHANGE(hwnd, wParam, lParam, fn) \
		(LRESULT)(DWORD)(BOOL)(fn)((hwnd), (UINT)(wParam), (DWORD)(wParam))
#	define FORWARD_WM_DEVICECHANGE(hwnd, uEvent, dwEventData, fn) \
		(BOOL)(DWORD)(fn)((hwnd), WM_DEVICECHANGE, (WPARAM)(UINT)(uEvent), (LPARAM)(DWORD)(dwEventData))

	// void Cls_OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
#	define HANDLE_WM_CONTEXTMENU(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(lParam)), 0)
#	define FORWARD_WM_CONTEXTMENU(hwnd, hwndContext, xPos, yPos, fn) \
		(void)(fn)((hwnd), WM_CONTEXTMENU, (WPARAM)(HWND)(hwndContext), MAKELPARAM((UINT)(xPos), (UINT)(yPos)))

// KERNEL Macro APIs.

	inline HMODULE GetInstanceModule( HINSTANCE hinstance )						{  return ( (HMODULE)hinstance );  }
	inline HGLOBAL GlobalPtrHandle  ( const void* p )							{  return ( ::GlobalHandle( p ) );  }
	inline bool    GlobalLockPtr    ( const void* p )							{  return ( !!::GlobalLock( GlobalPtrHandle( p ) ) );  }
	inline bool    GlobalUnlockPtr  ( const void* p )							{  return ( !!::GlobalUnlock( GlobalPtrHandle( p ) ) );  }
	inline void*   GlobalAllocPtr   ( UINT flags, SIZE_T bytes )				{  return ( ::GlobalLock( GlobalAlloc( flags, bytes ) ) );  }
	inline void*   GlobalReAllocPtr ( const void* p, SIZE_T bytes, UINT flags )	{  GlobalUnlockPtr( p );  return ( ::GlobalLock( ::GlobalReAlloc( GlobalPtrHandle( p ), bytes, flags ) ) );  }
	inline bool    GlobalFreePtr    ( const void* p )							{  GlobalUnlockPtr( p );  return ( !!::GlobalFree( GlobalPtrHandle( p ) ) );  }

// GDI Macro APIs.

	/*
#	define     DeletePen(hpen)                         DeleteObject((HGDIOBJ)(HPEN)(hpen))
#	define     SelectPen(hdc, hpen)                    ((HPEN)SelectObject((hdc), (HGDIOBJ)(HPEN)(hpen)))
#	define     GetStockPen(i)                          ((HPEN)GetStockObject(i))

#	define     DeleteBrush(hbr)                        DeleteObject((HGDIOBJ)(HBRUSH)(hbr))
#	define     SelectBrush(hdc, hbr)                   ((HBRUSH)SelectObject((hdc), (HGDIOBJ)(HBRUSH)(hbr)))
#	define     GetStockBrush(i)                        ((HBRUSH)GetStockObject(i))

#	define     DeleteRgn(hrgn)                         DeleteObject((HGDIOBJ)(HRGN)(hrgn))

#	define     CopyRgn(hrgnDst, hrgnSrc)               CombineRgn(hrgnDst, hrgnSrc, 0, RGN_COPY)
#	define     IntersectRgn(hrgnResult, hrgnA, hrgnB)  CombineRgn(hrgnResult, hrgnA, hrgnB, RGN_AND)
#	define     SubtractRgn(hrgnResult, hrgnA, hrgnB)   CombineRgn(hrgnResult, hrgnA, hrgnB, RGN_DIFF)
#	define     UnionRgn(hrgnResult, hrgnA, hrgnB)      CombineRgn(hrgnResult, hrgnA, hrgnB, RGN_OR)
#	define     XorRgn(hrgnResult, hrgnA, hrgnB)        CombineRgn(hrgnResult, hrgnA, hrgnB, RGN_XOR)

#	define     DeletePalette(hpal)                     DeleteObject((HGDIOBJ)(HPALETTE)(hpal))

#	define     DeleteFont(hfont)                       DeleteObject((HGDIOBJ)(HFONT)(hfont))
#	define     SelectFont(hdc, hfont)                  ((HFONT)SelectObject((hdc), (HGDIOBJ)(HFONT)(hfont)))
#	define     GetStockFont(i)                         ((HFONT)GetStockObject(i))

#	define     DeleteBitmap(hbm)                       DeleteObject((HGDIOBJ)(HBITMAP)(hbm))
#	define     SelectBitmap(hdc, hbm)                  ((HBITMAP)SelectObject((hdc), (HGDIOBJ)(HBITMAP)(hbm)))

#	define     InsetRect(lprc, dx, dy)                 InflateRect((lprc), -(dx), -(dy))
	*/

// Clipboard APIs.

	bool     SetClipboardText( const char* text, int len = -1 );
	bool     SetClipboardText( const gpstring& text );
	gpstring GetClipboardText( void );

// USER Macro APIs.

	/*
#	define     GetWindowInstance(hwnd) ((HMODULE)GetWindowLong(hwnd, GWL_HINSTANCE))
	*/

	inline DWORD GetWindowStyle( HWND hwnd )									{  gpassert( ::IsWindow( hwnd ) );  return ( (DWORD)GetWindowLong( hwnd, GWL_STYLE ) );  }
	inline GRect GetWindowRect ( HWND hwnd )									{  gpassert( ::IsWindow( hwnd ) );  GRect rect;  ::GetWindowRect( hwnd, rect );  return ( rect );  }
	inline GRect GetClientRect ( HWND hwnd )									{  gpassert( ::IsWindow( hwnd ) );  GRect rect;  ::GetClientRect( hwnd, rect );  return ( rect );  }

	/*
#	define     GetWindowExStyle(hwnd)  ((DWORD)GetWindowLong(hwnd, GWL_EXSTYLE))
#	define     GetWindowOwner(hwnd)    GetWindow(hwnd, GW_OWNER)
	*/

	inline HWND GetFirstChild  ( HWND hwnd )									{  gpassert( ::IsWindow( hwnd ) );  return ( ::GetTopWindow( hwnd ) );  }
	inline HWND GetFirstSibling( HWND hwnd )									{  gpassert( ::IsWindow( hwnd ) );  return ( ::GetWindow( hwnd, GW_HWNDFIRST ) );  }
	inline HWND GetLastSibling ( HWND hwnd )									{  gpassert( ::IsWindow( hwnd ) );  return ( ::GetWindow( hwnd, GW_HWNDLAST ) );  }
	inline HWND GetNextSibling ( HWND hwnd )									{  gpassert( ::IsWindow( hwnd ) );  return ( ::GetWindow( hwnd, GW_HWNDNEXT ) );  }
	inline HWND GetPrevSibling ( HWND hwnd )									{  gpassert( ::IsWindow( hwnd ) );  return ( ::GetWindow( hwnd, GW_HWNDPREV ) );  }

	/*
#	define     GetWindowID(hwnd)            GetDlgCtrlID(hwnd)
	*/

	inline void    SetWindowRedraw( HWND hwnd, bool redraw )					{  gpassert( ::IsWindow( hwnd ) );  ::SendMessage( hwnd, WM_SETREDRAW, (WPARAM)redraw, 0 );  }
	inline WNDPROC SubclassWindow ( HWND hwnd, WNDPROC newFunc )				{  gpassert( ::IsWindow( hwnd ) );  return ( (WNDPROC)::SetWindowLong( hwnd, GWL_WNDPROC, (LONG)newFunc ) );  }

	/*
#	define     IsMinimized(hwnd)        IsIconic(hwnd)
#	define     IsMaximized(hwnd)        IsZoomed(hwnd)
#	define     IsRestored(hwnd)    ((GetWindowStyle(hwnd) & (WS_MINIMIZE | WS_MAXIMIZE)) == 0)
	*/

	gpstring        GetWindowText   ( HWND hwnd );
	inline bool     SetWindowText   ( HWND hwnd, const char* str )				{  gpassert( ::IsWindow( hwnd ) );  return ( !!::SetWindowText( hwnd, str ) );  }
	inline gpstring GetDlgItemText  ( HWND hwnd, int id )						{  gpassert( ::IsWindow( hwnd ) );  return ( GetWindowText( ::GetDlgItem( hwnd, id ) ) );  }
	inline bool     SetDlgItemText  ( HWND hwnd, int id, const char* str )		{  gpassert( ::IsWindow( hwnd ) );  return ( !!::SetDlgItemText( hwnd, id, str ) );  }
	inline float    GetDlgItemFloat ( HWND hwnd, int id )						{  return ( (float)::atof( GetDlgItemText( hwnd, id ) ) );  }
	inline bool     SetDlgItemFloat ( HWND hwnd, int id, float f )				{  gpassert( ::IsWindow( hwnd ) );  return ( !!SetWindowText( ::GetDlgItem( hwnd, id ), gpstringf( "%f", f ) ) );  }
	inline double   GetDlgItemDouble( HWND hwnd, int id )						{  return ( ::atof( GetDlgItemText( hwnd, id ) ) );  }
	inline bool     SetDlgItemDouble( HWND hwnd, int id, double d )				{  gpassert( ::IsWindow( hwnd ) );  return ( !!SetWindowText( ::GetDlgItem( hwnd, id ), gpstringf( "%f", d ) ) );  }
	inline HFONT    GetWindowFont   ( HWND hwnd )								{  gpassert( ::IsWindow( hwnd ) );  return ( FORWARD_WM_GETFONT( hwnd, SendMessage ) );  }
	inline void     SetWindowFont   ( HWND hwnd, HFONT hfont, bool redraw )		{  gpassert( ::IsWindow( hwnd ) );  FORWARD_WM_SETFONT( hwnd, hfont, redraw, SendMessage );  }

	/*
#	define     MapWindowRect(hwndFrom, hwndTo, lprc) \
	                    MapWindowPoints((hwndFrom), (hwndTo), (POINT *)(lprc), 2)
#	define     IsLButtonDown()  (GetKeyState(VK_LBUTTON) < 0)
#	define     IsRButtonDown()  (GetKeyState(VK_RBUTTON) < 0)
#	define     IsMButtonDown()  (GetKeyState(VK_MBUTTON) < 0)

#	define     SubclassDialog(hwndDlg, lpfn) \
	             ((DLGPROC)SetWindowLong(hwndDlg, DWL_DLGPROC, (LPARAM)(DLGPROC)(lpfn)))
	*/

	inline bool SetDlgMsgResult( HWND hwnd, UINT msg, LRESULT result )
	{
		gpassert( ::IsWindow( hwnd ) );
		switch ( msg )
		{
			case ( WM_CTLCOLORMSGBOX    ):
			case ( WM_CTLCOLOREDIT      ):
			case ( WM_CTLCOLORLISTBOX   ):
			case ( WM_CTLCOLORBTN       ):
			case ( WM_CTLCOLORDLG       ):
			case ( WM_CTLCOLORSCROLLBAR ):
			case ( WM_CTLCOLORSTATIC    ):
			case ( WM_COMPAREITEM       ):
			case ( WM_VKEYTOITEM        ):
			case ( WM_CHARTOITEM        ):
			case ( WM_QUERYDRAGICON     ):
			case ( WM_INITDIALOG        ):
			{
				return ( !!result );
			}
		}
		::SetWindowLong( hwnd, DWL_MSGRESULT, (LPARAM)result);
		return ( true );
	}

	/*
#	define     DefDlgProcEx(hwnd, msg, wParam, lParam, pfRecursion) \
	    (*(pfRecursion) = TRUE, DefDlgProc(hwnd, msg, wParam, lParam))

#	define     CheckDefDlgRecursion(pfRecursion) \
	    if (*(pfRecursion)) { *(pfRecursion) = FALSE; return FALSE; }
	*/

// Static control message APIs.

	namespace Static
	{
		inline bool     Enable       ( HWND hwnd, bool enable )					{  gpassert( ::IsWindow( hwnd ) );  return ( !!::EnableWindow( hwnd, enable ) );  }
		inline int      GetText      ( HWND hwnd, LPTSTR text, int maxCount )	{  gpassert( ::IsWindow( hwnd ) );  return ( ::GetWindowText( hwnd, text, maxCount ) );  }
		inline gpstring GetText      ( HWND hwnd )								{  return ( GetWindowText( hwnd ) );  }
		inline int      GetTextLength( HWND hwnd )								{  gpassert( ::IsWindow( hwnd ) );  return ( ::GetWindowTextLength( hwnd ) );  }
		inline bool     SetText      ( HWND hwnd, LPCTSTR text )				{  gpassert( ::IsWindow( hwnd ) );  return ( !!::SetWindowText( hwnd, text ) );  }
		inline HICON    SetIcon      ( HWND hwnd, HICON icon )					{  gpassert( ::IsWindow( hwnd ) );  return ( (HICON)::SendMessage( hwnd, STM_SETICON, (WPARAM)icon, 0 ) );  }
		inline HICON    GetIcon      ( HWND hwnd )								{  gpassert( ::IsWindow( hwnd ) );  return ( (HICON)::SendMessage( hwnd, STM_GETICON, 0, 0 ) );  }
	}

// Button control message APIs.

	namespace Button
	{
		inline bool     Enable       ( HWND hwnd, bool enable )					{  gpassert( ::IsWindow( hwnd ) );  return ( !!::EnableWindow( hwnd, enable ) );  }
		inline int      GetText      ( HWND hwnd, LPTSTR text, int maxCount )	{  gpassert( ::IsWindow( hwnd ) );  return ( ::GetWindowText( hwnd, text, maxCount ) );  }
		inline gpstring GetText      ( HWND hwnd )								{  return ( GetWindowText( hwnd ) );  }
		inline int      GetTextLength( HWND hwnd )								{  gpassert( ::IsWindow( hwnd ) );  return ( ::GetWindowTextLength( hwnd ) );  }
		inline bool     SetText      ( HWND hwnd, LPCTSTR text )				{  gpassert( ::IsWindow( hwnd ) );  return ( !!::SetWindowText( hwnd, text ) );  }
		inline int      GetCheck     ( HWND hwnd )								{  gpassert( ::IsWindow( hwnd ) );  return ( (int)::SendMessage( hwnd, BM_GETCHECK, 0, 0 ) );  }
		inline void     SetCheck     ( HWND hwnd, DWORD check )					{  gpassert( ::IsWindow( hwnd ) );  ::SendMessage( hwnd, BM_SETCHECK, check, 0 );  }
		inline bool     GetCheckBool ( HWND hwnd )								{  return ( GetCheck( hwnd ) == BST_CHECKED );  }
		inline void     SetCheckBool ( HWND hwnd, bool check )					{  SetCheck( hwnd, check ? BST_CHECKED : BST_UNCHECKED );  }

		/*
#		define GetState(hwndCtl)            ((int)(DWORD)SNDMSG((hwndCtl), BM_GETSTATE, 0, 0))
#		define SetState(hwndCtl, state)     ((UINT)(DWORD)SNDMSG((hwndCtl), BM_SETSTATE, (WPARAM)(int)(state), 0))
#		define SetStyle(hwndCtl, style, fRedraw) ((void)SNDMSG((hwndCtl), BM_SETSTYLE, (WPARAM)LOWORD(style), MAKELPARAM(((fRedraw) ? TRUE : FALSE), 0)))
		*/
	}

// Edit control message APIs.

	namespace Edit
	{
		inline bool     Enable         ( HWND hwnd, bool enable )				{  gpassert( ::IsWindow( hwnd ) );  return ( !!::EnableWindow( hwnd, enable ) );  }
		inline int      GetText        ( HWND hwnd, LPTSTR text, int maxCount )	{  gpassert( ::IsWindow( hwnd ) );  return ( ::GetWindowText( hwnd, text, maxCount ) );  }
		inline gpstring GetText        ( HWND hwnd )							{  return ( GetWindowText( hwnd ) );  }
		inline int      GetTextLength  ( HWND hwnd )							{  gpassert( ::IsWindow( hwnd ) );  return ( ::GetWindowTextLength( hwnd ) );  }
		inline bool     SetText        ( HWND hwnd, LPCTSTR text )				{  gpassert( ::IsWindow( hwnd ) );  return ( !!::SetWindowText( hwnd, text ) );  }
		inline int      GetLimitText   ( HWND hwnd )							{  gpassert( ::IsWindow( hwnd ) );  return ( ::SendMessage( hwnd, EM_GETLIMITTEXT, 0, 0 ) );  }
		       void     SetLimitText   ( HWND hwnd, int maxText );
		       void     SetLimitTextMax( HWND hwnd );
		/*
#		define GetLineCount(hwndCtl)              ((int)(DWORD)SNDMSG((hwndCtl), EM_GETLINECOUNT, 0, 0))
#		define GetLine(hwndCtl, line, lpch, cchMax) ((*((int *)(lpch)) = (cchMax)), ((int)(DWORD)SNDMSG((hwndCtl), EM_GETLINE, (WPARAM)(int)(line), (LPARAM)(LPTSTR)(lpch))))
#		define GetRect(hwndCtl, lprc)             ((void)SNDMSG((hwndCtl), EM_GETRECT, 0, (LPARAM)(RECT *)(lprc)))
#		define SetRect(hwndCtl, lprc)             ((void)SNDMSG((hwndCtl), EM_SETRECT, 0, (LPARAM)(const RECT *)(lprc)))
#		define SetRectNoPaint(hwndCtl, lprc)      ((void)SNDMSG((hwndCtl), EM_SETRECTNP, 0, (LPARAM)(const RECT *)(lprc)))
		*/

		inline void GetSel       ( HWND hwnd, int& start, int& end )			{  gpassert( ::IsWindow( hwnd ) );  ::SendMessage( hwnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end );  }
		inline void SetSel       ( HWND hwnd, int start, int end )				{  gpassert( ::IsWindow( hwnd ) );  ::SendMessage( hwnd, EM_SETSEL, start, end );  }
		inline void SetSelAll    ( HWND hwnd )									{  SetSel( hwnd, 0, -1 );  }
		inline void SetSelNone   ( HWND hwnd )									{  SetSel( hwnd, -1, -1 );  }
		inline void SetSelAtBegin( HWND hwnd )									{  SetSel( hwnd, 0, 0 );  }
		inline void SetSelAtEnd  ( HWND hwnd )									{  gpassert( ::IsWindow( hwnd ) );  int len = GetTextLength( hwnd );  SetSel( hwnd, len, len );  }
		inline void ReplaceSel   ( HWND hwnd, LPCTSTR replace )					{  gpassert( ::IsWindow( hwnd ) );  ::SendMessage( hwnd, EM_REPLACESEL, 0, (LPARAM)replace );  }

		/*
#		define GetModify(hwndCtl)                 ((BOOL)(DWORD)SNDMSG((hwndCtl), EM_GETMODIFY, 0, 0))
#		define SetModify(hwndCtl, fModified)      ((void)SNDMSG((hwndCtl), EM_SETMODIFY, (WPARAM)(UINT)(fModified), 0))
		*/

		inline bool ScrollCaret( HWND hwnd )									{  gpassert( ::IsWindow( hwnd ) );  return ( !!::SendMessage( hwnd, EM_SCROLLCARET, 0, 0) );  }

		/*
#		define LineFromChar(hwndCtl, ich)         ((int)(DWORD)SNDMSG((hwndCtl), EM_LINEFROMCHAR, (WPARAM)(int)(ich), 0))
#		define LineIndex(hwndCtl, line)           ((int)(DWORD)SNDMSG((hwndCtl), EM_LINEINDEX, (WPARAM)(int)(line), 0))
#		define LineLength(hwndCtl, line)          ((int)(DWORD)SNDMSG((hwndCtl), EM_LINELENGTH, (WPARAM)(int)(line), 0))
		*/
		
		inline void Scroll( HWND hwnd, int dv, int dh )							{  gpassert( ::IsWindow( hwnd ) );  ::SendMessage( hwnd, EM_LINESCROLL, (WPARAM)(dh), (LPARAM)(dv) );  }

		/*
#		define CanUndo(hwndCtl)                   ((BOOL)(DWORD)SNDMSG((hwndCtl), EM_CANUNDO, 0, 0))
#		define Undo(hwndCtl)                      ((BOOL)(DWORD)SNDMSG((hwndCtl), EM_UNDO, 0, 0))
#		define EmptyUndoBuffer(hwndCtl)           ((void)SNDMSG((hwndCtl), EM_EMPTYUNDOBUFFER, 0, 0))
#		define SetPasswordChar(hwndCtl, ch)       ((void)SNDMSG((hwndCtl), EM_SETPASSWORDCHAR, (WPARAM)(UINT)(ch), 0))
#		define SetTabStops(hwndCtl, cTabs, lpTabs) ((void)SNDMSG((hwndCtl), EM_SETTABSTOPS, (WPARAM)(int)(cTabs), (LPARAM)(const int *)(lpTabs)))
#		define FmtLines(hwndCtl, fAddEOL)         ((BOOL)(DWORD)SNDMSG((hwndCtl), EM_FMTLINES, (WPARAM)(BOOL)(fAddEOL), 0))
#		define GetHandle(hwndCtl)                 ((HLOCAL)(UINT)(DWORD)SNDMSG((hwndCtl), EM_GETHANDLE, 0, 0))
#		define SetHandle(hwndCtl, h)              ((void)SNDMSG((hwndCtl), EM_SETHANDLE, (WPARAM)(UINT)(HLOCAL)(h), 0))
		*/

		inline int GetFirstVisibleLine( HWND hwnd )								{  gpassert( ::IsWindow( hwnd ) );  return ( (int)(DWORD)::SendMessage( hwnd, EM_GETFIRSTVISIBLELINE, 0, 0 ) );  }

		/*
#		define SetReadOnly(hwndCtl, fReadOnly)    ((BOOL)(DWORD)SNDMSG((hwndCtl), EM_SETREADONLY, (WPARAM)(BOOL)(fReadOnly), 0))
#		define GetPasswordChar(hwndCtl)           ((TCHAR)(DWORD)SNDMSG((hwndCtl), EM_GETPASSWORDCHAR, 0, 0))
#		define SetWordBreakProc(hwndCtl, lpfnWordBreak) ((void)SNDMSG((hwndCtl), EM_SETWORDBREAKPROC, 0, (LPARAM)(EDITWORDBREAKPROC)(lpfnWordBreak)))
#		define GetWordBreakProc(hwndCtl)          ((EDITWORDBREAKPROC)SNDMSG((hwndCtl), EM_GETWORDBREAKPROC, 0, 0))
		*/

		bool CopyTextToClipboard( HWND hwnd );
		void AppendText( HWND hwnd, LPCTSTR text, bool preserveSel = false );
		inline void ScrollToTop( HWND hwnd )									{  int lines = GetFirstVisibleLine( hwnd );  if ( lines > 0 )  Scroll( hwnd, -lines, 0 );  }
	}

// ScrollBar control message APIs

	namespace ScrollBar
	{
		/*
		// NOTE: flags parameter is a collection of ESB_* values, NOT a boolean!
#		define Enable(hwndCtl, flags)            EnableScrollBar((hwndCtl), SB_CTL, (flags))
#		define Show(hwndCtl, fShow)              ShowWindow((hwndCtl), (fShow) ? SW_SHOWNORMAL : SW_HIDE)
#		define SetPos(hwndCtl, pos, fRedraw)     SetScrollPos((hwndCtl), SB_CTL, (pos), (fRedraw))
#		define GetPos(hwndCtl)                   GetScrollPos((hwndCtl), SB_CTL)
#		define SetRange(hwndCtl, posMin, posMax, fRedraw)    SetScrollRange((hwndCtl), SB_CTL, (posMin), (posMax), (fRedraw))
#		define GetRange(hwndCtl, lpposMin, lpposMax)         GetScrollRange((hwndCtl), SB_CTL, (lpposMin), (lpposMax))
		*/
	}

// ListBox control message APIs

	namespace ListBox
	{
		/*
#		define Enable(hwndCtl, fEnable)            EnableWindow((hwndCtl), (fEnable))
#		define GetCount(hwndCtl)                   ((int)(DWORD)SNDMSG((hwndCtl), LB_GETCOUNT, 0, 0))
#		define ResetContent(hwndCtl)               ((BOOL)(DWORD)SNDMSG((hwndCtl), LB_RESETCONTENT, 0, 0))
		*/

		inline int AddString   ( HWND hwnd, const char* str )					{  gpassert( ::IsWindow( hwnd ) );  int rc = (int)SendMessage( hwnd, LB_ADDSTRING, 0, (LPARAM)str );  gpassert( (rc != LB_ERR) && (rc != LB_ERRSPACE) );  return ( rc );  }
		inline int InsertString( HWND hwnd, int index, const char* str )		{  gpassert( ::IsWindow( hwnd ) );  int rc = (int)SendMessage( hwnd, LB_INSERTSTRING, (int)index, (LPARAM)str );  gpassert( (rc != LB_ERR) && (rc != LB_ERRSPACE) );  return ( rc );  }

		/*
#		define AddItemData(hwndCtl, data)          ((int)(DWORD)SNDMSG((hwndCtl), LB_ADDSTRING, 0, (LPARAM)(data)))
#		define InsertItemData(hwndCtl, index, data) ((int)(DWORD)SNDMSG((hwndCtl), LB_INSERTSTRING, (WPARAM)(int)(index), (LPARAM)(data)))
		*/

		inline int DeleteString( HWND hwnd, int index )							{  gpassert( ::IsWindow( hwnd ) );  int rc = (int)SendMessage( hwnd, LB_DELETESTRING, (WPARAM)index, 0 );  gpassert( rc != LB_ERR );  return ( rc );  }
		inline int GetTextLen  ( HWND hwnd, int index )							{  gpassert( ::IsWindow( hwnd ) );  int rc = (int)SendMessage( hwnd, LB_GETTEXTLEN, (WPARAM)index, 0 );  gpassert( rc != LB_ERR );  return ( rc );  }
		gpstring   GetText     ( HWND hwnd, int index );

		/*
#		define GetItemData(hwndCtl, index)         ((LRESULT)(DWORD)SNDMSG((hwndCtl), LB_GETITEMDATA, (WPARAM)(int)(index), 0))
#		define SetItemData(hwndCtl, index, data)   ((int)(DWORD)SNDMSG((hwndCtl), LB_SETITEMDATA, (WPARAM)(int)(index), (LPARAM)(data)))
#		define FindString(hwndCtl, indexStart, lpszFind) ((int)(DWORD)SNDMSG((hwndCtl), LB_FINDSTRING, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))
#		define FindItemData(hwndCtl, indexStart, data) ((int)(DWORD)SNDMSG((hwndCtl), LB_FINDSTRING, (WPARAM)(int)(indexStart), (LPARAM)(data)))
#		define SetSel(hwndCtl, fSelect, index)     ((int)(DWORD)SNDMSG((hwndCtl), LB_SETSEL, (WPARAM)(BOOL)(fSelect), (LPARAM)(index)))
#		define SelItemRange(hwndCtl, fSelect, first, last)    ((int)(DWORD)SNDMSG((hwndCtl), LB_SELITEMRANGE, (WPARAM)(BOOL)(fSelect), MAKELPARAM((first), (last))))
		*/

		inline int      GetCurSel    ( HWND hwnd )								{  gpassert( ::IsWindow( hwnd ) );  return ( (int)::SendMessage( hwnd, LB_GETCURSEL, 0, 0 ) );  }
		inline gpstring GetCurSelText( HWND hwnd )								{  int sel = GetCurSel( hwnd );  return ( (sel != LB_ERR) ? GetText( hwnd, sel ) : "" );  }
		inline bool     SetCurSel    ( HWND hwnd, int index )					{  gpassert( ::IsWindow( hwnd ) );  int rc = (int)SendMessage( hwnd, LB_SETCURSEL, (WPARAM)index, 0 );  gpassert( (index == -1) || (rc != LB_ERR) );  return ( (index == -1) || (rc != LB_ERR) );  }

		/*
#		define SelectString(hwndCtl, indexStart, lpszFind) ((int)(DWORD)SNDMSG((hwndCtl), LB_SELECTSTRING, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))
#		define SelectItemData(hwndCtl, indexStart, data)   ((int)(DWORD)SNDMSG((hwndCtl), LB_SELECTSTRING, (WPARAM)(int)(indexStart), (LPARAM)(data)))
#		define GetSel(hwndCtl, index)              ((int)(DWORD)SNDMSG((hwndCtl), LB_GETSEL, (WPARAM)(int)(index), 0))
#		define GetSelCount(hwndCtl)                ((int)(DWORD)SNDMSG((hwndCtl), LB_GETSELCOUNT, 0, 0))
#		define GetTopIndex(hwndCtl)                ((int)(DWORD)SNDMSG((hwndCtl), LB_GETTOPINDEX, 0, 0))
#		define GetSelItems(hwndCtl, cItems, lpItems) ((int)(DWORD)SNDMSG((hwndCtl), LB_GETSELITEMS, (WPARAM)(int)(cItems), (LPARAM)(int *)(lpItems)))
#		define SetTopIndex(hwndCtl, indexTop)      ((int)(DWORD)SNDMSG((hwndCtl), LB_SETTOPINDEX, (WPARAM)(int)(indexTop), 0))
#		define SetColumnWidth(hwndCtl, cxColumn)   ((void)SNDMSG((hwndCtl), LB_SETCOLUMNWIDTH, (WPARAM)(int)(cxColumn), 0))
#		define GetHorizontalExtent(hwndCtl)        ((int)(DWORD)SNDMSG((hwndCtl), LB_GETHORIZONTALEXTENT, 0, 0))
#		define SetHorizontalExtent(hwndCtl, cxExtent)     ((void)SNDMSG((hwndCtl), LB_SETHORIZONTALEXTENT, (WPARAM)(int)(cxExtent), 0))
#		define SetTabStops(hwndCtl, cTabs, lpTabs) ((BOOL)(DWORD)SNDMSG((hwndCtl), LB_SETTABSTOPS, (WPARAM)(int)(cTabs), (LPARAM)(int *)(lpTabs)))
#		define GetItemRect(hwndCtl, index, lprc)   ((int)(DWORD)SNDMSG((hwndCtl), LB_GETITEMRECT, (WPARAM)(int)(index), (LPARAM)(RECT *)(lprc)))
#		define SetCaretIndex(hwndCtl, index)       ((int)(DWORD)SNDMSG((hwndCtl), LB_SETCARETINDEX, (WPARAM)(int)(index), 0))
#		define GetCaretIndex(hwndCtl)              ((int)(DWORD)SNDMSG((hwndCtl), LB_GETCARETINDEX, 0, 0))
#		define FindStringExact(hwndCtl, indexStart, lpszFind) ((int)(DWORD)SNDMSG((hwndCtl), LB_FINDSTRINGEXACT, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))
#		define SetItemHeight(hwndCtl, index, cy)   ((int)(DWORD)SNDMSG((hwndCtl), LB_SETITEMHEIGHT, (WPARAM)(int)(index), MAKELPARAM((cy), 0)))
#		define GetItemHeight(hwndCtl, index)       ((int)(DWORD)SNDMSG((hwndCtl), LB_GETITEMHEIGHT, (WPARAM)(int)(index), 0))
#		define Dir(hwndCtl, attrs, lpszFileSpec)   ((int)(DWORD)SNDMSG((hwndCtl), LB_DIR, (WPARAM)(UINT)(attrs), (LPARAM)(LPCTSTR)(lpszFileSpec)))
		*/
	}


// ComboBox control message APIs

	namespace ComboBox
	{
		/*
#		define Enable(hwndCtl, fEnable)       EnableWindow((hwndCtl), (fEnable))
		*/

		inline int      GetText      ( HWND hwnd, LPTSTR text, int maxCount )	{  gpassert( ::IsWindow( hwnd ) );  return ( ::GetWindowText( hwnd, text, maxCount ) );  }
		inline gpstring GetText      ( HWND hwnd )								{  return ( GetWindowText( hwnd ) );  }
		inline int      GetTextLength( HWND hwnd )								{  gpassert( ::IsWindow( hwnd ) );  return ( ::GetWindowTextLength( hwnd ) );  }
		inline bool     SetText      ( HWND hwnd, LPCTSTR text )				{  gpassert( ::IsWindow( hwnd ) );  return ( !!::SetWindowText( hwnd, text ) );  }

		/*
#		define LimitText(hwndCtl, cchLimit)   ((int)(DWORD)SNDMSG((hwndCtl), CB_LIMITTEXT, (WPARAM)(int)(cchLimit), 0))
#		define GetEditSel(hwndCtl)            ((DWORD)SNDMSG((hwndCtl), CB_GETEDITSEL, 0, 0))
#		define SetEditSel(hwndCtl, ichStart, ichEnd) ((int)(DWORD)SNDMSG((hwndCtl), CB_SETEDITSEL, 0, MAKELPARAM((ichStart), (ichEnd))))
#		define GetCount(hwndCtl)              ((int)(DWORD)SNDMSG((hwndCtl), CB_GETCOUNT, 0, 0))
#		define ResetContent(hwndCtl)          ((int)(DWORD)SNDMSG((hwndCtl), CB_RESETCONTENT, 0, 0))
		*/

		inline int AddString   ( HWND hwnd, const char* text )					{  gpassert( ::IsWindow( hwnd ) );  return ( (int)::SendMessage( hwnd, CB_ADDSTRING, 0, (LPARAM)text ) );  }
		inline int InsertString( HWND hwnd, int index, const char* text )		{  gpassert( ::IsWindow( hwnd ) );  return ( (int)::SendMessage( hwnd, CB_INSERTSTRING, (WPARAM)index, (LPARAM)text ) );  }

		/*
#		define AddItemData(hwndCtl, data)     ((int)(DWORD)SNDMSG((hwndCtl), CB_ADDSTRING, 0, (LPARAM)(data)))
#		define InsertItemData(hwndCtl, index, data) ((int)(DWORD)SNDMSG((hwndCtl), CB_INSERTSTRING, (WPARAM)(int)(index), (LPARAM)(data)))
#		define DeleteString(hwndCtl, index)   ((int)(DWORD)SNDMSG((hwndCtl), CB_DELETESTRING, (WPARAM)(int)(index), 0))
        */

		inline int GetLBTextLen( HWND hwnd, int index )							{  gpassert( ::IsWindow( hwnd ) );  return ( (int)::SendMessage( hwnd, CB_GETLBTEXTLEN, (WPARAM)index, 0 ) );  }
        gpstring   GetLBText   ( HWND hwnd, int index );

        /*
#		define GetItemData(hwndCtl, index)        ((LRESULT)(DWORD)SNDMSG((hwndCtl), CB_GETITEMDATA, (WPARAM)(int)(index), 0))
#		define SetItemData(hwndCtl, index, data)  ((int)(DWORD)SNDMSG((hwndCtl), CB_SETITEMDATA, (WPARAM)(int)(index), (LPARAM)(data)))
#		define FindString(hwndCtl, indexStart, lpszFind)  ((int)(DWORD)SNDMSG((hwndCtl), CB_FINDSTRING, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))
#		define FindItemData(hwndCtl, indexStart, data)    ((int)(DWORD)SNDMSG((hwndCtl), CB_FINDSTRING, (WPARAM)(int)(indexStart), (LPARAM)(data)))
		*/

		inline int      GetCurSel    ( HWND hwnd )								{  gpassert( ::IsWindow( hwnd ) );  return ( (int)::SendMessage( hwnd, CB_GETCURSEL, 0, 0 ) );  }
		inline gpstring GetCurSelText( HWND hwnd )								{  int sel = GetCurSel( hwnd );  return ( (sel != CB_ERR) ? GetLBText( hwnd, sel ) : "" );  }
		inline bool     SetCurSel    ( HWND hwnd, int index )					{  gpassert( ::IsWindow( hwnd ) );  int rc = (int)SendMessage( hwnd, CB_SETCURSEL, (WPARAM)index, 0 );  gpassert( (index == -1) || (rc != CB_ERR) );  return ( (index == -1) || (rc != CB_ERR) );  }

		/*
#		define SelectString(hwndCtl, indexStart, lpszSelect)  ((int)(DWORD)SNDMSG((hwndCtl), CB_SELECTSTRING, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszSelect)))
#		define SelectItemData(hwndCtl, indexStart, data)      ((int)(DWORD)SNDMSG((hwndCtl), CB_SELECTSTRING, (WPARAM)(int)(indexStart), (LPARAM)(data)))
#		define Dir(hwndCtl, attrs, lpszFileSpec)  ((int)(DWORD)SNDMSG((hwndCtl), CB_DIR, (WPARAM)(UINT)(attrs), (LPARAM)(LPCTSTR)(lpszFileSpec)))
#		define ShowDropdown(hwndCtl, fShow)       ((BOOL)(DWORD)SNDMSG((hwndCtl), CB_SHOWDROPDOWN, (WPARAM)(BOOL)(fShow), 0))
#		define FindStringExact(hwndCtl, indexStart, lpszFind)  ((int)(DWORD)SNDMSG((hwndCtl), CB_FINDSTRINGEXACT, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))
#		define GetDroppedState(hwndCtl)           ((BOOL)(DWORD)SNDMSG((hwndCtl), CB_GETDROPPEDSTATE, 0, 0))
#		define GetDroppedControlRect(hwndCtl, lprc) ((void)SNDMSG((hwndCtl), CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)(RECT *)(lprc)))
#		define GetItemHeight(hwndCtl)             ((int)(DWORD)SNDMSG((hwndCtl), CB_GETITEMHEIGHT, 0, 0))
#		define SetItemHeight(hwndCtl, index, cyItem) ((int)(DWORD)SNDMSG((hwndCtl), CB_SETITEMHEIGHT, (WPARAM)(int)(index), (LPARAM)(int)cyItem))
#		define GetExtendedUI(hwndCtl)             ((UINT)(DWORD)SNDMSG((hwndCtl), CB_GETEXTENDEDUI, 0, 0))
#		define SetExtendedUI(hwndCtl, flags)      ((int)(DWORD)SNDMSG((hwndCtl), CB_SETEXTENDEDUI, (WPARAM)(UINT)(flags), 0))
		*/
	}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace winx

#endif  // __WINX_H

//////////////////////////////////////////////////////////////////////////////
