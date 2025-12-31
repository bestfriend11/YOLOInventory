//////////////////////////////////////////////////////////////////////////////
//
// File     :  AtlX.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains wrappers and enhancements for using ATL.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __ATLX_H
#define __ATLX_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"
#include "KernelTool.h"
#include "WinX.h"

#include <vector>

//////////////////////////////////////////////////////////////////////////////
// core includes

// ignore any warnings or changes that win32 has
#pragma warning ( push, 1 )
#ifndef GetMessage
#ifdef UNICODE
#define GetMessage GetMessageW
#else
#define GetMessage GetMessageA
#endif
#endif

// use our own assert
#define _ATL_NO_DEBUG_CRT
#ifdef ATLASSERT
#undef ATLASSERT
#endif
#define ATLASSERT gpassert

// base atl
#include <AtlBase.h>

// module info for HINSTANCE
namespace atlx
{
	struct Module : CComModule, AutoSingleton <Module>
	{
		Module( void )  {  Init( NULL, ::GetModuleHandle( NULL ) );  }
	   ~Module( void )  {  Term();  }
	};

	CComModule& GetModule( void );
	void        SetModule( CComModule* module );
}

// module singleton
#define g_AtlxModule atlx::GetModule()

// atl window classes - _Module is an internal atl requirement
#define _Module g_AtlxModule
#include <AtlWin.h>
#undef _Module

// back to normal
#pragma warning ( pop )
#undef GetMessage

//////////////////////////////////////////////////////////////////////////////

namespace atlx  {  // begin of namespace atlx

// we're using this stuff anyway
using namespace winx;

//////////////////////////////////////////////////////////////////////////////
// safe windowsx clone for atl

	// $ these are ATL versions of winx's handlers.

// Usage macros.

	// put this in the message map, same as MESSAGE_HANDLER except it
	// automatically cracks the messages.
#	define MESSAGE_CRACKER( message, fn ) \
		if ( uMsg == message ) \
		{ \
			bHandled = TRUE; \
			lResult = HANDLE_##message##_ATL( (wParam), (lParam), (bHandled), (fn) ); \
			if(bHandled) \
				return TRUE; \
		}

	// same as BEGIN_MSG_MAP except it uses the hwnd of what it receives for
	// message routine. use this for "mixin" classes.
#	define BEGIN_MSG_MAP_REMOTE(theClass) \
	private: \
		atlx::AutoClearHwnd $m_AutoClear$; \
	public: \
		BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID = 0) \
		{ \
			$m_AutoClear$.Set( m_hWnd ); \
			BOOL bHandled = TRUE; \
			m_hWnd = hWnd; \
			uMsg; \
			wParam; \
			lParam; \
			lResult; \
			bHandled; \
			switch(dwMsgMapID) \
			{ \
			case 0:

// Message crackers.

	// void Cls_OnCompacting(UINT compactRatio, BOOL& handled)
#	define HANDLE_WM_COMPACTING_ATL(wParam, lParam, handled, fn) \
		((fn)((UINT)(wParam), handled), 0L)

	// void Cls_OnWinIniChange(LPCTSTR lpszSectionName, BOOL& handled)
#	define HANDLE_WM_WININICHANGE_ATL(wParam, lParam, handled, fn) \
		((fn)((LPCTSTR)(lParam), handled), 0L)

	// void Cls_OnSysColorChange(BOOL& handled)
#	define HANDLE_WM_SYSCOLORCHANGE_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// BOOL Cls_OnQueryNewPalette(BOOL& handled)
#	define HANDLE_WM_QUERYNEWPALETTE_ATL(wParam, lParam, handled, fn) \
		MAKELRESULT((BOOL)(fn)(handled), 0L)

	// void Cls_OnPaletteIsChanging(HWND hwndPaletteChange, BOOL& handled)
#	define HANDLE_WM_PALETTEISCHANGING_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(wParam), handled), 0L)

	// void Cls_OnPaletteChanged(HWND hwndPaletteChange, BOOL& handled)
#	define HANDLE_WM_PALETTECHANGED_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(wParam), handled), 0L)

	// void Cls_OnFontChange(BOOL& handled)
#	define HANDLE_WM_FONTCHANGE_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// void Cls_OnSpoolerStatus(UINT status, int cJobInQueue, BOOL& handled)
#	define HANDLE_WM_SPOOLERSTATUS_ATL(wParam, lParam, handled, fn) \
		((fn)((UINT)(wParam), (int)(short)LOWORD(lParam), handled), 0L)

	// void Cls_OnDevModeChange(LPCTSTR lpszDeviceName, BOOL& handled)
#	define HANDLE_WM_DEVMODECHANGE_ATL(wParam, lParam, handled, fn) \
		((fn)((LPCTSTR)(lParam), handled), 0L)

	// void Cls_OnTimeChange(BOOL& handled)
#	define HANDLE_WM_TIMECHANGE_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// void Cls_OnPower(int code, BOOL& handled)
#	define HANDLE_WM_POWER_ATL(wParam, lParam, handled, fn) \
		((fn)((int)(wParam), handled), 0L)

	// BOOL Cls_OnQueryEndSession(BOOL& handled)
#	define HANDLE_WM_QUERYENDSESSION_ATL(wParam, lParam, handled, fn) \
		MAKELRESULT((BOOL)(fn)(handled), 0L)

	// void Cls_OnEndSession(BOOL fEnding, BOOL& handled)
#	define HANDLE_WM_ENDSESSION_ATL(wParam, lParam, handled, fn) \
		((fn)((BOOL)(wParam), handled), 0L)

	// void Cls_OnQuit(int exitCode, BOOL& handled)
#	define HANDLE_WM_QUIT_ATL(wParam, lParam, handled, fn) \
		((fn)((int)(wParam), handled), 0L)

	// BOOL Cls_OnCreate(LPCREATESTRUCT lpCreateStruct, BOOL& handled)
#	define HANDLE_WM_CREATE_ATL(wParam, lParam, handled, fn) \
		((fn)((LPCREATESTRUCT)(lParam), handled) ? 0L : (LRESULT)-1L)

	// BOOL Cls_OnNCCreate(LPCREATESTRUCT lpCreateStruct, BOOL& handled)
#	define HANDLE_WM_NCCREATE_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(BOOL)(fn)((LPCREATESTRUCT)(lParam), handled)

	// void Cls_OnDestroy(BOOL& handled)
#	define HANDLE_WM_DESTROY_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// void Cls_OnNCDestroy(BOOL& handled)
#	define HANDLE_WM_NCDESTROY_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// void Cls_OnShowWindow(BOOL fShow, UINT status, BOOL& handled)
#	define HANDLE_WM_SHOWWINDOW_ATL(wParam, lParam, handled, fn) \
		((fn)((BOOL)(wParam), (UINT)(lParam), handled), 0L)

	// void Cls_OnSetRedraw(BOOL fRedraw, BOOL& handled)
#	define HANDLE_WM_SETREDRAW_ATL(wParam, lParam, handled, fn) \
		((fn)((BOOL)(wParam), handled), 0L)

	// void Cls_OnEnable(BOOL fEnable, BOOL& handled)
#	define HANDLE_WM_ENABLE_ATL(wParam, lParam, handled, fn) \
		((fn)((BOOL)(wParam), handled), 0L)

	// void Cls_OnSetText(LPCTSTR lpszText, BOOL& handled)
#	define HANDLE_WM_SETTEXT_ATL(wParam, lParam, handled, fn) \
		((fn)((LPCTSTR)(lParam), handled), 0L)

	// INT Cls_OnGetText(int cchTextMax, LPTSTR lpszText, BOOL& handled)
#	define HANDLE_WM_GETTEXT_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(int)(fn)((int)(wParam), (LPTSTR)(lParam), handled)

	// INT Cls_OnGetTextLength(BOOL& handled)
#	define HANDLE_WM_GETTEXTLENGTH_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(int)(fn)(handled)

	// BOOL Cls_OnWindowPosChanging(LPWINDOWPOS lpwpos, BOOL& handled)
#	define HANDLE_WM_WINDOWPOSCHANGING_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(BOOL)(fn)((LPWINDOWPOS)(lParam), handled)

	// void Cls_OnWindowPosChanged(const LPWINDOWPOS lpwpos, BOOL& handled)
#	define HANDLE_WM_WINDOWPOSCHANGED_ATL(wParam, lParam, handled, fn) \
		((fn)((const LPWINDOWPOS)(lParam), handled), 0L)

	// void Cls_OnMove(int x, int y, BOOL& handled)
#	define HANDLE_WM_MOVE_ATL(wParam, lParam, handled, fn) \
		((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), handled), 0L)

	// void Cls_OnSize(UINT state, int cx, int cy, BOOL& handled)
#	define HANDLE_WM_SIZE_ATL(wParam, lParam, handled, fn) \
		((fn)((UINT)(wParam), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), handled), 0L)

	// void Cls_OnClose(BOOL& handled)
#	define HANDLE_WM_CLOSE_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// BOOL Cls_OnQueryOpen(BOOL& handled)
#	define HANDLE_WM_QUERYOPEN_ATL(wParam, lParam, handled, fn) \
		MAKELRESULT((BOOL)(fn)(handled), 0L)

	// void Cls_OnGetMinMaxInfo(LPMINMAXINFO lpMinMaxInfo, BOOL& handled)
#	define HANDLE_WM_GETMINMAXINFO_ATL(wParam, lParam, handled, fn) \
		((fn)((LPMINMAXINFO)(lParam), handled), 0L)

	// void Cls_OnPaint(BOOL& handled)
#	define HANDLE_WM_PAINT_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// BOOL Cls_OnEraseBkgnd(HDC hdc, BOOL& handled)
#	define HANDLE_WM_ERASEBKGND_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(BOOL)(fn)((HDC)(wParam), handled)

	// BOOL Cls_OnIconEraseBkgnd(HDC hdc, BOOL& handled)
#	define HANDLE_WM_ICONERASEBKGND_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(BOOL)(fn)((HDC)(wParam), handled)

	// void Cls_OnNCPaint(HRGN hrgn, BOOL& handled)
#	define HANDLE_WM_NCPAINT_ATL(wParam, lParam, handled, fn) \
		((fn)((HRGN)(wParam), handled), 0L)

	// UINT Cls_OnNCCalcSize(BOOL fCalcValidRects, NCCALCSIZE_PARAMS * lpcsp, BOOL& handled, BOOL& handled)
#	define HANDLE_WM_NCCALCSIZE_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(fn)((BOOL)(0), (NCCALCSIZE_PARAMS *)(lParam), handled)

	// UINT Cls_OnNCHitTest(int x, int y, BOOL& handled)
#	define HANDLE_WM_NCHITTEST_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), handled)

	// HICON Cls_OnQueryDragIcon(BOOL& handled)
#	define HANDLE_WM_QUERYDRAGICON_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(fn)(handled)

	// void Cls_OnDropFiles(handled, HDROP hdrop, BOOL& handled)
#	define HANDLE_WM_DROPFILES_ATL(wParam, lParam, handled, fn) \
		((fn)((HDROP)(wParam), handled), 0L)

	// void Cls_OnActivate(UINT state, HWND hwndActDeact, handled, BOOL fMinimized, BOOL& handled)
#	define HANDLE_WM_ACTIVATE_ATL(wParam, lParam, handled, fn) \
		((fn)((UINT)LOWORD(wParam), (HWND)(lParam), (BOOL)HIWORD(wParam), handled), 0L)

	// void Cls_OnActivateApp(BOOL fActivate, DWORD dwThreadId, BOOL& handled)
#	define HANDLE_WM_ACTIVATEAPP_ATL(wParam, lParam, handled, fn) \
		((fn)((BOOL)(wParam), (DWORD)(lParam), handled), 0L)

	// BOOL Cls_OnNCActivate(BOOL fActive, HWND hwndActDeact, BOOL fMinimized, BOOL& handled)
#	define HANDLE_WM_NCACTIVATE_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(BOOL)(fn)((BOOL)(wParam), 0L, 0L, handled)

	// void Cls_OnSetFocus(HWND hwndOldFocus, BOOL& handled)
#	define HANDLE_WM_SETFOCUS_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(wParam), handled), 0L)

	// void Cls_OnKillFocus(HWND hwndNewFocus, BOOL& handled)
#	define HANDLE_WM_KILLFOCUS_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(wParam), handled), 0L)

	// void Cls_OnKey(UINT vk, BOOL fDown, int cRepeat, UINT flags, BOOL& handled)
#	define HANDLE_WM_KEYDOWN_ATL(wParam, lParam, handled, fn) \
		((fn)((UINT)(wParam), TRUE, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam), handled), 0L)

	// void Cls_OnKey(UINT vk, BOOL fDown, int cRepeat, UINT flags, BOOL& handled)
#	define HANDLE_WM_KEYUP_ATL(wParam, lParam, handled, fn) \
		((fn)((UINT)(wParam), FALSE, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam), handled), 0L)

	// void Cls_OnChar(TCHAR ch, int cRepeat, BOOL& handled)
#	define HANDLE_WM_CHAR_ATL(wParam, lParam, handled, fn) \
		((fn)((TCHAR)(wParam), (int)(short)LOWORD(lParam), handled), 0L)

	// void Cls_OnDeadChar(TCHAR ch, int cRepeat, BOOL& handled)
#	define HANDLE_WM_DEADCHAR_ATL(wParam, lParam, handled, fn) \
		((fn)((TCHAR)(wParam), (int)(short)LOWORD(lParam), handled), 0L)

	// void Cls_OnSysKey(UINT vk, BOOL fDown, int cRepeat, UINT flags, BOOL& handled)
#	define HANDLE_WM_SYSKEYDOWN_ATL(wParam, lParam, handled, fn) \
		((fn)((UINT)(wParam), TRUE, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam), handled), 0L)

	// void Cls_OnSysKey(UINT vk, BOOL fDown, int cRepeat, UINT flags, BOOL& handled)
#	define HANDLE_WM_SYSKEYUP_ATL(wParam, lParam, handled, fn) \
		((fn)((UINT)(wParam), FALSE, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam), handled), 0L)

	// void Cls_OnSysChar(TCHAR ch, int cRepeat, BOOL& handled)
#	define HANDLE_WM_SYSCHAR_ATL(wParam, lParam, handled, fn) \
		((fn)((TCHAR)(wParam), (int)(short)LOWORD(lParam), handled), 0L)

	// void Cls_OnSysDeadChar(TCHAR ch, int cRepeat, BOOL& handled)
#	define HANDLE_WM_SYSDEADCHAR_ATL(wParam, lParam, handled, fn) \
		((fn)((TCHAR)(wParam), (int)(short)LOWORD(lParam), handled), 0L)

	// void Cls_OnMouseMove(int x, int y, UINT keyFlags, BOOL& handled)
#	define HANDLE_WM_MOUSEMOVE_ATL(wParam, lParam, handled, fn) \
		((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnLButtonDown(BOOL fDoubleClick, int x, int y, UINT keyFlags, BOOL& handled)
#	define HANDLE_WM_LBUTTONDOWN_ATL(wParam, lParam, handled, fn) \
		((fn)(FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnLButtonDown(BOOL fDoubleClick, int x, int y, UINT keyFlags, BOOL& handled)
#	define HANDLE_WM_LBUTTONDBLCLK_ATL(wParam, lParam, handled, fn) \
		((fn)(TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnLButtonUp(int x, int y, UINT keyFlags, BOOL& handled)
#	define HANDLE_WM_LBUTTONUP_ATL(wParam, lParam, handled, fn) \
		((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnRButtonDown(BOOL fDoubleClick, int x, int y, UINT keyFlags, BOOL& handled)
#	define HANDLE_WM_RBUTTONDOWN_ATL(wParam, lParam, handled, fn) \
		((fn)(FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnRButtonDown(BOOL fDoubleClick, int x, int y, UINT keyFlags, BOOL& handled)
#	define HANDLE_WM_RBUTTONDBLCLK_ATL(wParam, lParam, handled, fn) \
		((fn)(TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnRButtonUp(int x, int y, UINT flags, BOOL& handled)
#	define HANDLE_WM_RBUTTONUP_ATL(wParam, lParam, handled, fn) \
		((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnMButtonDown(BOOL fDoubleClick, int x, int y, UINT keyFlags, BOOL& handled)
#	define HANDLE_WM_MBUTTONDOWN_ATL(wParam, lParam, handled, fn) \
		((fn)(FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnMButtonDown(BOOL fDoubleClick, int x, int y, UINT keyFlags, BOOL& handled)
#	define HANDLE_WM_MBUTTONDBLCLK_ATL(wParam, lParam, handled, fn) \
		((fn)(TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnMButtonUp(int x, int y, UINT flags, BOOL& handled)
#	define HANDLE_WM_MBUTTONUP_ATL(wParam, lParam, handled, fn) \
		((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnNCMouseMove(int x, int y, UINT codeHitTest, BOOL& handled)
#	define HANDLE_WM_NCMOUSEMOVE_ATL(wParam, lParam, handled, fn) \
		((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnNCLButtonDown(BOOL fDoubleClick, int x, int y, UINT codeHitTest, BOOL& handled)
#	define HANDLE_WM_NCLBUTTONDOWN_ATL(wParam, lParam, handled, fn) \
		((fn)(FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnNCLButtonDown(BOOL fDoubleClick, int x, int y, UINT codeHitTest, BOOL& handled)
#	define HANDLE_WM_NCLBUTTONDBLCLK_ATL(wParam, lParam, handled, fn) \
		((fn)(TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnNCLButtonUp(int x, int y, UINT codeHitTest, BOOL& handled)
#	define HANDLE_WM_NCLBUTTONUP_ATL(wParam, lParam, handled, fn) \
		((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnNCRButtonDown(BOOL fDoubleClick, int x, int y, UINT codeHitTest, BOOL& handled)
#	define HANDLE_WM_NCRBUTTONDOWN_ATL(wParam, lParam, handled, fn) \
		((fn)(FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnNCRButtonDown(BOOL fDoubleClick, int x, int y, UINT codeHitTest, BOOL& handled)
#	define HANDLE_WM_NCRBUTTONDBLCLK_ATL(wParam, lParam, handled, fn) \
		((fn)(TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnNCRButtonUp(int x, int y, UINT codeHitTest, BOOL& handled)
#	define HANDLE_WM_NCRBUTTONUP_ATL(wParam, lParam, handled, fn) \
		((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnNCMButtonDown(BOOL fDoubleClick, int x, int y, UINT codeHitTest, BOOL& handled)
#	define HANDLE_WM_NCMBUTTONDOWN_ATL(wParam, lParam, handled, fn) \
		((fn)(FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnNCMButtonDown(BOOL fDoubleClick, int x, int y, UINT codeHitTest, BOOL& handled)
#	define HANDLE_WM_NCMBUTTONDBLCLK_ATL(wParam, lParam, handled, fn) \
		((fn)(TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// void Cls_OnNCMButtonUp(int x, int y, UINT codeHitTest, BOOL& handled)
#	define HANDLE_WM_NCMBUTTONUP_ATL(wParam, lParam, handled, fn) \
		((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam), handled), 0L)

	// int Cls_OnMouseActivate(HWND hwndTopLevel, UINT codeHitTest, UINT msg, BOOL& handled)
#	define HANDLE_WM_MOUSEACTIVATE_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(int)(fn)((HWND)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), handled)

	// void Cls_OnCancelMode(BOOL& handled)
#	define HANDLE_WM_CANCELMODE_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// void Cls_OnTimer(UINT id, BOOL& handled)
#	define HANDLE_WM_TIMER_ATL(wParam, lParam, handled, fn) \
		((fn)((UINT)(wParam), handled), 0L)

	// void Cls_OnInitMenu(HMENU hMenu, BOOL& handled)
#	define HANDLE_WM_INITMENU_ATL(wParam, lParam, handled, fn) \
		((fn)((HMENU)(wParam), handled), 0L)

	// void Cls_OnInitMenuPopup(HMENU hMenu, UINT item, BOOL fSystemMenu, BOOL& handled)
#	define HANDLE_WM_INITMENUPOPUP_ATL(wParam, lParam, handled, fn) \
		((fn)((HMENU)(wParam), (UINT)LOWORD(lParam), (BOOL)HIWORD(lParam), handled), 0L)

	// void Cls_OnMenuSelect(HMENU hmenu, int item, HMENU hmenuPopup, UINT flags, BOOL& handled)
#	define HANDLE_WM_MENUSELECT_ATL(wParam, lParam, handled, fn)                  \
		((fn)((HMENU)(lParam),  \
		(HIWORD(wParam) & MF_POPUP) ? 0L : (int)(LOWORD(wParam)),           \
		(HIWORD(wParam) & MF_POPUP) ? GetSubMenu((HMENU)lParam, LOWORD(wParam)) : 0L, \
		(UINT)(((short)HIWORD(wParam) == -1) ? 0xFFFFFFFF : HIWORD(wParam)), handled), 0L)

	// DWORD Cls_OnMenuChar(UINT ch, UINT flags, HMENU hmenu, BOOL& handled)
#	define HANDLE_WM_MENUCHAR_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(fn)((UINT)(LOWORD(wParam)), (UINT)HIWORD(wParam), (HMENU)(lParam), handled)

	// void Cls_OnCommand(int id, HWND hwndCtl, UINT codeNotify, BOOL& handled)
#	define HANDLE_WM_COMMAND_ATL(wParam, lParam, handled, fn) \
		((fn)((int)(LOWORD(wParam)), (HWND)(lParam), (UINT)HIWORD(wParam), handled), 0L)

	// void Cls_OnHScroll(HWND hwndCtl, UINT code, int pos, BOOL& handled)
#	define HANDLE_WM_HSCROLL_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(lParam), (UINT)(LOWORD(wParam)), (int)(short)HIWORD(wParam), handled), 0L)

	// void Cls_OnVScroll(HWND hwndCtl, UINT code, int pos, BOOL& handled)
#	define HANDLE_WM_VSCROLL_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(lParam), (UINT)(LOWORD(wParam)),  (int)(short)HIWORD(wParam), handled), 0L)

	// void Cls_OnCut(BOOL& handled)
#	define HANDLE_WM_CUT_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// void Cls_OnCopy(BOOL& handled)
#	define HANDLE_WM_COPY_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// void Cls_OnPaste(BOOL& handled)
#	define HANDLE_WM_PASTE_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// void Cls_OnClear(BOOL& handled)
#	define HANDLE_WM_CLEAR_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// void Cls_OnUndo(BOOL& handled)
#	define HANDLE_WM_UNDO_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// HANDLE Cls_OnRenderFormat(UINT fmt, BOOL& handled)
#	define HANDLE_WM_RENDERFORMAT_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(HANDLE)(fn)((UINT)(wParam), handled)

	// void Cls_OnRenderAllFormats(BOOL& handled)
#	define HANDLE_WM_RENDERALLFORMATS_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// void Cls_OnDestroyClipboard(BOOL& handled)
#	define HANDLE_WM_DESTROYCLIPBOARD_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// void Cls_OnDrawClipboard(BOOL& handled)
#	define HANDLE_WM_DRAWCLIPBOARD_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// void Cls_OnPaintClipboard(HWND hwndCBViewer, const LPPAINTSTRUCT lpPaintStruct, BOOL& handled)
#	define HANDLE_WM_PAINTCLIPBOARD_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(wParam), (const LPPAINTSTRUCT)GlobalLock((HGLOBAL)(lParam)), handled), GlobalUnlock((HGLOBAL)(lParam)), 0L)

	// void Cls_OnSizeClipboard(HWND hwndCBViewer, const LPRECT lprc, BOOL& handled)
#	define HANDLE_WM_SIZECLIPBOARD_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(wParam), (const LPRECT)GlobalLock((HGLOBAL)(lParam)), handled), GlobalUnlock((HGLOBAL)(lParam)), 0L)

	// void Cls_OnVScrollClipboard(HWND hwndCBViewer, UINT code, int pos, BOOL& handled)
#	define HANDLE_WM_VSCROLLCLIPBOARD_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(wParam), (UINT)LOWORD(lParam), (int)(short)HIWORD(lParam), handled), 0L)

	// void Cls_OnHScrollClipboard(HWND hwndCBViewer, UINT code, int pos, BOOL& handled)
#	define HANDLE_WM_HSCROLLCLIPBOARD_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(wParam), (UINT)LOWORD(lParam), (int)(short)HIWORD(lParam), handled), 0L)

	// void Cls_OnAskCBFormatName(int cchMax, LPTSTR rgchName, BOOL& handled)
#	define HANDLE_WM_ASKCBFORMATNAME_ATL(wParam, lParam, handled, fn) \
		((fn)((int)(wParam), (LPTSTR)(lParam), handled), 0L)

	// void Cls_OnChangeCBChain(HWND hwndRemove, HWND hwndNext, BOOL& handled)
#	define HANDLE_WM_CHANGECBCHAIN_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(wParam), (HWND)(lParam), handled), 0L)

	// BOOL Cls_OnSetCursor(HWND hwndCursor, UINT codeHitTest, UINT msg, BOOL& handled)
#	define HANDLE_WM_SETCURSOR_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(BOOL)(fn)((HWND)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), handled)

	// void Cls_OnSysCommand(UINT cmd, int x, int y, BOOL& handled)
#	define HANDLE_WM_SYSCOMMAND_ATL(wParam, lParam, handled, fn) \
		((fn)((UINT)(wParam), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), handled), 0L)

	// HWND Cls_MDICreate(const LPMDICREATESTRUCT lpmcs, BOOL& handled)
#	define HANDLE_WM_MDICREATE_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(fn)((LPMDICREATESTRUCT)(lParam), handled)

	// void Cls_MDIDestroy(HWND hwndDestroy, BOOL& handled)
#	define HANDLE_WM_MDIDESTROY_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(wParam), handled), 0L)

	// NOTE: Usable only by MDI client window, BOOL& bHandleds
	// void Cls_MDIActivate(BOOL fActive, HWND hwndActivate, handled, HWND hwndDeactivate, BOOL& handled)
#	define HANDLE_WM_MDIACTIVATE_ATL(wParam, lParam, handled, fn) \
		((fn)((BOOL)(lParam == (LPARAM)hwnd), (HWND)(lParam), (HWND)(wParam), handled), 0L)

	// void Cls_MDIRestore(HWND hwndRestore, BOOL& handled)
#	define HANDLE_WM_MDIRESTORE_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(wParam), handled), 0L)

	// HWND Cls_MDINext(HWND hwndCur, BOOL fPrev, BOOL& handled)
#	define HANDLE_WM_MDINEXT_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(HWND)(fn)((HWND)(wParam), (BOOL)lParam, handled)

	// void Cls_MDIMaximize(HWND hwndMaximize, BOOL& handled)
#	define HANDLE_WM_MDIMAXIMIZE_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(wParam), handled), 0L)

	// BOOL Cls_MDITile(UINT cmd, BOOL& handled)
#	define HANDLE_WM_MDITILE_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(fn)((UINT)(wParam), handled)

	// BOOL Cls_MDICascade(UINT cmd, BOOL& handled)
#	define HANDLE_WM_MDICASCADE_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(fn)((UINT)(wParam), handled)

	// void Cls_MDIIconArrange(BOOL& handled)
#	define HANDLE_WM_MDIICONARRANGE_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// HWND Cls_MDIGetActive(BOOL& handled)
#	define HANDLE_WM_MDIGETACTIVE_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(fn)(handled)

	// HMENU Cls_MDISetMenu(BOOL fRefresh, HMENU hmenuFrame, HMENU hmenuWindow, BOOL& handled)
#	define HANDLE_WM_MDISETMENU_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(fn)((BOOL)(wParam), (HMENU)(wParam), (HMENU)(lParam), handled)

	// void Cls_OnChildActivate(BOOL& handled)
#	define HANDLE_WM_CHILDACTIVATE_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// BOOL Cls_OnInitDialog(HWND hwndFocus, LPARAM lParam, BOOL& handled)
#	define HANDLE_WM_INITDIALOG_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(BOOL)(fn)((HWND)(wParam), lParam, handled)

	// HWND Cls_OnNextDlgCtl(HWND hwndSetFocus, BOOL fNext, BOOL& handled)
#	define HANDLE_WM_NEXTDLGCTL_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(HWND)(fn)((HWND)(wParam), (BOOL)(lParam), handled)

	// void Cls_OnParentNotify(UINT msg, HWND hwndChild, int idChild, BOOL& handled)
#	define HANDLE_WM_PARENTNOTIFY_ATL(wParam, lParam, handled, fn) \
		((fn)((UINT)LOWORD(wParam), (HWND)(lParam), (UINT)HIWORD(wParam), handled), 0L)

	// void Cls_OnEnterIdle(UINT source, HWND hwndSource, BOOL& handled)
#	define HANDLE_WM_ENTERIDLE_ATL(wParam, lParam, handled, fn) \
		((fn)((UINT)(wParam), (HWND)(lParam), handled), 0L)

	// UINT Cls_OnGetDlgCode(LPMSG lpmsg, BOOL& handled)
#	define HANDLE_WM_GETDLGCODE_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(fn)((LPMSG)(lParam), handled)

	// HBRUSH Cls_OnCtlColor(HDC hdc, HWND hwndChild, int type, BOOL& handled)
#	define HANDLE_WM_CTLCOLORMSGBOX_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((HDC)(wParam), (HWND)(lParam), CTLCOLOR_MSGBOX, handled)

#	define HANDLE_WM_CTLCOLOREDIT_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((HDC)(wParam), (HWND)(lParam), CTLCOLOR_EDIT, handled)

#	define HANDLE_WM_CTLCOLORLISTBOX_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((HDC)(wParam), (HWND)(lParam), CTLCOLOR_LISTBOX, handled)

#	define HANDLE_WM_CTLCOLORBTN_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((HDC)(wParam), (HWND)(lParam), CTLCOLOR_BTN, handled)

#	define HANDLE_WM_CTLCOLORDLG_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((HDC)(wParam), (HWND)(lParam), CTLCOLOR_DLG, handled)

#	define HANDLE_WM_CTLCOLORSCROLLBAR_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((HDC)(wParam), (HWND)(lParam), CTLCOLOR_SCROLLBAR, handled)

#	define HANDLE_WM_CTLCOLORSTATIC_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((HDC)(wParam), (HWND)(lParam), CTLCOLOR_STATIC, handled)

	// void Cls_OnSetFont(HWND hwndCtl, HFONT hfont, BOOL fRedraw, BOOL& handled)
#	define HANDLE_WM_SETFONT_ATL(wParam, lParam, handled, fn) \
		((fn)((HFONT)(wParam), (BOOL)(lParam), handled), 0L)

	// HFONT Cls_OnGetFont(BOOL& handled)
#	define HANDLE_WM_GETFONT_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(UINT)(HFONT)(fn)(handled)

	// void Cls_OnDrawItem(const DRAWITEMSTRUCT * lpDrawItem, BOOL& handled)
#	define HANDLE_WM_DRAWITEM_ATL(wParam, lParam, handled, fn) \
		((fn)((const DRAWITEMSTRUCT *)(lParam), handled), 0L)

	// void Cls_OnMeasureItem(MEASUREITEMSTRUCT * lpMeasureItem, BOOL& handled)
#	define HANDLE_WM_MEASUREITEM_ATL(wParam, lParam, handled, fn) \
		((fn)((MEASUREITEMSTRUCT *)(lParam), handled), 0L)

	// void Cls_OnDeleteItem(const DELETEITEMSTRUCT * lpDeleteItem, BOOL& handled)
#	define HANDLE_WM_DELETEITEM_ATL(wParam, lParam, handled, fn) \
		((fn)((const DELETEITEMSTRUCT *)(lParam), handled), 0L)

	// int Cls_OnCompareItem(const COMPAREITEMSTRUCT * lpCompareItem, BOOL& handled)
#	define HANDLE_WM_COMPAREITEM_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(int)(fn)((const COMPAREITEMSTRUCT *)(lParam), handled)

	// int Cls_OnVkeyToItem(UINT vk, HWND hwndListbox, int iCaret, BOOL& handled)
#	define HANDLE_WM_VKEYTOITEM_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(int)(fn)((UINT)LOWORD(wParam), (HWND)(lParam), (int)(short)HIWORD(wParam), handled)

	// int Cls_OnCharToItem(UINT ch, HWND hwndListbox, int iCaret, BOOL& handled)
#	define HANDLE_WM_CHARTOITEM_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(int)(fn)((UINT)LOWORD(wParam), (HWND)(lParam), (int)(short)HIWORD(wParam), handled)

	// void Cls_OnQueueSync(BOOL& handled)
#	define HANDLE_WM_QUEUESYNC_ATL(wParam, lParam, handled, fn) \
		((fn)(handled), 0L)

	// void Cls_OnCommNotify(int cid, UINT flags, BOOL& handled)
#	define HANDLE_WM_COMMNOTIFY_ATL(wParam, lParam, handled, fn) \
		((fn)((int)(wParam), (UINT)LOWORD(lParam), handled), 0L)

	// void Cls_OnDisplayChange(UINT bitsPerPixel, UINT cxScreen, UINT cyScreen, BOOL& handled)
#	define HANDLE_WM_DISPLAYCHANGE_ATL(wParam, lParam, handled, fn) \
		((fn)((UINT)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(wParam), handled), 0L)

	// BOOL Cls_OnDeviceChange(UINT uEvent, DWORD dwEventData, BOOL& handled)
#	define HANDLE_WM_DEVICECHANGE_ATL(wParam, lParam, handled, fn) \
		(LRESULT)(DWORD)(BOOL)(fn)((UINT)(wParam), (DWORD)(wParam), handled)

	// void Cls_OnContextMenu(HWND hwndContext, UINT xPos, UINT yPos, BOOL& handled)
#	define HANDLE_WM_CONTEXTMENU_ATL(wParam, lParam, handled, fn) \
		((fn)((HWND)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), handled), 0L)

//////////////////////////////////////////////////////////////////////////////
// helpers

struct AutoClearHwnd
{
	HWND* m_hWnd;

	AutoClearHwnd( void )
		{  m_hWnd = NULL;  }
   ~AutoClearHwnd( void )
		{  if ( m_hWnd != NULL )  *m_hWnd = NULL;  }
	void Set( HWND& set )
		{  m_hWnd = &set;  }
};

//////////////////////////////////////////////////////////////////////////////
// class Window declaration

// purpose: this is a more feature-packed CWindow.

class Window : public CWindow
{
public:
	SET_INHERITED( Window, CWindow );

	Window( HWND hwnd = NULL ) : Inherited( hwnd )		{  }

	bool IsWindow( void ) const							{  return ( !!::IsWindow( *this ) );  }
	bool IsNull  ( void ) const							{  return ( m_hWnd == NULL );  }

	ThisType& operator = ( HWND hwnd )					{  m_hWnd = hwnd;  return *this;  }

// Coordinate mapping.

	using Inherited::ScreenToClient;
	using Inherited::ClientToScreen;

	BOOL ScreenToParent( POINT* point ) const			{  Window parent = GetParent();  return ( parent.IsNull() ? TRUE : parent.ScreenToClient( point ) );  }
	BOOL ScreenToParent( RECT * rect  ) const			{  Window parent = GetParent();  return ( parent.IsNull() ? TRUE : parent.ScreenToClient( rect ) );  }
	BOOL ParentToScreen( POINT* point ) const			{  Window parent = GetParent();  return ( parent.IsNull() ? TRUE : parent.ClientToScreen( point ) );  }
	BOOL ParentToScreen( RECT * rect  ) const			{  Window parent = GetParent();  return ( parent.IsNull() ? TRUE : parent.ClientToScreen( rect ) );  }

	GRect  GetScreenToClient( GRect rect ) const		{  ScreenToClient( rect );  return ( rect );  }
	GPoint GetScreenToClient( GPoint pt  ) const		{  ScreenToClient( pt   );  return ( pt   );  }
	GRect  GetClientToScreen( GRect rect ) const		{  ClientToScreen( rect );  return ( rect );  }
	GPoint GetClientToScreen( GPoint pt  ) const		{  ClientToScreen( pt   );  return ( pt   );  }

	GRect  GetScreenToParent( GRect rect ) const		{  ScreenToParent( rect );  return ( rect );  }
	GPoint GetScreenToParent( GPoint pt  ) const		{  ScreenToParent( pt   );  return ( pt   );  }
	GRect  GetParentToScreen( GRect rect ) const		{  ParentToScreen( rect );  return ( rect );  }
	GPoint GetParentToScreen( GPoint pt  ) const		{  ParentToScreen( pt   );  return ( pt   );  }

// Window size and position.

	using Inherited::GetClientRect;
	using Inherited::GetWindowRect;

	GRect GetClientRect      ( void ) const				{  GRect rect;  GetClientRect( rect );  return ( rect );  }
	GRect GetClientScreenRect( void ) const				{  return ( GetClientToScreen( GetClientRect() ) );  }
	BOOL  GetClientSize      ( SIZE* size ) const		{  gpassert( size != NULL );  GRect rect;  BOOL rc = GetClientRect( rect );  *size = rect.Size();  return ( rc );  }
	GSize GetClientSize      ( void ) const				{  GSize size;  GetClientSize( size );  return ( size );  }

	GRect GetWindowRect( void ) const					{  GRect rect;  GetWindowRect( rect );  return ( rect );  }
	BOOL  GetWindowSize( SIZE* size ) const				{  gpassert( size != NULL );  GRect rect;  BOOL rc = GetWindowRect( rect );  *size = rect.Size();  return ( rc );  }
	GSize GetWindowSize( void ) const					{  GSize size;  GetWindowSize( size );  return ( size );  }

	GRect GetChildRect( HWND child ) const				{  return ( GetScreenToClient( Window( child ).GetWindowRect() ) );  }

// Attributes.

	using Inherited::GetWindowText;

	gpstring GetWindowText( void ) const				{  return ( winx::GetWindowText( *this ) );  }

	void  SetWindowFont( HFONT hfont, bool redraw )		{  winx::SetWindowFont( *this, hfont, redraw );  }
	HFONT GetWindowFont( void ) const					{  return ( winx::GetWindowFont( *this ) );  }

// Window state.

	bool   SetForeground  ( void )						{  gpassert( IsWindow() );  return ( !!::SetForegroundWindow( *this ) );  }
	Window SetActiveWindow( void )						{  return ( Inherited::SetActiveWindow() );  }
	Window SetCapture     ( void )						{  return ( Inherited::SetCapture() );  }
	Window SetFocus       ( void )						{  return ( Inherited::SetFocus() );  }
	bool   SetAlwaysOnTop ( bool set = true )			{  return ( !!Inherited::SetWindowPos( set ? HWND_TOPMOST : HWND_NOTOPMOST, &GRect::ZERO, SWP_NOMOVE | SWP_NOSIZE ) );  }

// Window access.

	Window ChildWindowFromPoint  ( POINT point ) const					{  return ( Inherited::ChildWindowFromPoint( point ) );  }
	Window ChildWindowFromPointEx( POINT point, UINT flags ) const		{  return ( Inherited::ChildWindowFromPointEx( point, flags ) );  }

	Window GetTopWindow      ( void ) const				{  return ( Inherited::GetTopWindow() );  }
	Window GetWindow         ( UINT cmd ) const			{  return ( Inherited::GetWindow( cmd ) );  }
	Window GetLastActivePopup( void ) const				{  return ( Inherited::GetLastActivePopup() );  }

	Window GetParent( void ) const						{  return ( Inherited::GetParent() );  }
	Window SetParent( HWND newParent )					{  return ( Inherited::SetParent( newParent ) );  }

	Window GetFirstChild  ( void ) const				{  return ( winx::GetFirstChild  ( *this ) );  }
	Window GetFirstSibling( void ) const				{  return ( winx::GetFirstSibling( *this ) );  }
	Window GetLastSibling ( void ) const				{  return ( winx::GetLastSibling ( *this ) );  }
	Window GetNextSibling ( void ) const				{  return ( winx::GetNextSibling ( *this ) );  }
	Window GetPrevSibling ( void ) const				{  return ( winx::GetPrevSibling ( *this ) );  }

// Clipboard functions.

	Window SetClipboardViewer( void )					{  return ( Inherited::SetClipboardViewer() );  }

// Misc. Operations

	Window GetDescendantWindow( int id ) const			{  return ( Inherited::GetDescendantWindow( id ) );  }
	Window GetTopLevelParent  ( void ) const			{  return ( Inherited::GetTopLevelParent() );  }
	Window GetTopLevelWindow  ( void ) const			{  return ( Inherited::GetTopLevelWindow() );  }
};

//////////////////////////////////////////////////////////////////////////////
// class WindowImpl declaration

template <typename T, typename BASE = Window, typename TRAITS = CControlWinTraits>
class ATL_NO_VTABLE WindowImpl : public CWindowImpl <T, BASE, TRAITS>
{
public:
	typedef WindowImpl <T, BASE, TRAITS> ThisType;
	typedef CWindowImpl <T, BASE, TRAITS> Inherited;
};

//////////////////////////////////////////////////////////////////////////////
// class Dialog declaration

class Dialog : public Window
{
public:
	SET_INHERITED( Dialog, Window );

	Dialog( HWND hwnd = NULL ) : Inherited( hwnd )		{  }

	ThisType& operator = ( HWND hwnd )					{  m_hWnd = hwnd;  return *this;  }

// Window tree access.

	Window GetDlgItem( int id ) const					{  return ( Inherited::GetDlgItem( id ) );  }

// Dialog-box item methods.

	using Inherited::GetDlgItemText;

	Window   GetNextDlgGroupItem ( HWND ctl, BOOL prev = FALSE ) const	{  return ( Inherited::GetNextDlgGroupItem( ctl, prev ) );  }
	Window   GetNextDlgTabItem   ( HWND ctl, BOOL prev = FALSE ) const	{  return ( Inherited::GetNextDlgTabItem( ctl, prev ) );  }
	GRect    GetDlgItemWindowRect( UINT id ) const						{  return ( GetDlgItem( id ).GetWindowRect() );  }
	GRect    GetDlgItemRect      ( UINT id ) const						{  return ( GetScreenToClient( GetDlgItemWindowRect( id ) ) );  }
	GRect    GetDlgItemRect      ( HWND child ) const					{  return ( GetChildRect( child ) );  }
	gpstring GetDlgItemText      ( int id ) const						{  return ( winx::GetDlgItemText( *this, id ) );  }
	float    GetDlgItemFloat     ( int id ) const						{  return ( winx::GetDlgItemFloat( *this, id ) );  }
	bool     SetDlgItemFloat     ( int id, float f )					{  return ( winx::SetDlgItemFloat( *this, id, f ) );  }
	double   GetDlgItemDouble    ( int id ) const						{  return ( winx::GetDlgItemDouble( *this, id ) );  }
	bool     SetDlgItemDouble    ( int id, double d )					{  return ( winx::SetDlgItemDouble( *this, id, d ) );  }

// Modeless dialog handling.

	static void AddModelessDialog       ( HWND hwnd );
	static void RemoveModelessDialog    ( HWND hwnd );
	static bool ProcessModelessDialogMsg( MSG& msg );
};

//////////////////////////////////////////////////////////////////////////////
// class DialogImpl declaration

template <typename T, typename BASE = Dialog>
class ATL_NO_VTABLE DialogImpl : public CDialogImpl <T, BASE>
{
public:
	typedef DialogImpl <T, BASE> ThisType;
	typedef CDialogImpl <T, BASE> Inherited;
	typedef CDialogImplBaseT <BASE> InheritedBase;

	using Inherited::DoModal;
	using Inherited::Create;

	int DoModal( const DLGTEMPLATE* dlgTemplate, HWND parent = ::GetActiveWindow(), LPARAM param = 0 )
	{
		gpassert( m_hWnd == NULL );
		g_AtlxModule.AddCreateWndData( &m_thunk.cd, rcast <InheritedBase*> ( this ) );

#		if GP_DEBUG
		m_bModal = true;
#		endif // GP_DEBUG

		return ( ::DialogBoxIndirectParam(
				g_AtlxModule.GetResourceInstance(),
				dlgTemplate,
				parent,
				(DLGPROC)T::StartDialogProc,
				param ) );
	}

	Dialog Create( const DLGTEMPLATE* dlgTemplate, HWND parent, LPARAM param = 0 )
	{
		gpassert( m_hWnd == NULL );
		g_AtlxModule.AddCreateWndData( &m_thunk.cd, rcast <InheritedBase*> ( this ) );

#		if GP_DEBUG
		m_bModal = false;
#		endif // GP_DEBUG

		HWND hWnd = ::CreateDialogIndirectParam(
				g_AtlxModule.GetResourceInstance(),
				dlgTemplate,
				parent,
				(DLGPROC)T::StartDialogProc,
				param );
		gpassert( m_hWnd == hWnd );
		return ( hWnd );
	}

private:
	BEGIN_MSG_MAP( ThisType )
		MESSAGE_CRACKER( WM_INITDIALOG, OnInitDialog )
		MESSAGE_CRACKER( WM_DESTROY,    OnDestroy )
	END_MSG_MAP()

	BOOL OnInitDialog( HWND /*focus*/, LPARAM /*init*/, BOOL& handled )
		{  handled = FALSE;  Dialog::AddModelessDialog( *this );  return ( TRUE );  }
	void OnDestroy( BOOL& handled )
		{  handled = FALSE;  Dialog::RemoveModelessDialog( *this );  }
};

//////////////////////////////////////////////////////////////////////////////
// class Static declaration

class Static : public Window
{
public:
	SET_INHERITED( Static, Window );

	Static( HWND hwnd = NULL ) : Inherited( hwnd )				{  }

	ThisType& operator =  ( HWND hwnd )							{  m_hWnd = hwnd;  return *this;  }

	int      GetText      ( LPTSTR text, int maxCount ) const	{  return ( GetWindowText( text, maxCount ) );  }
	gpstring GetText      ( void ) const						{  return ( GetWindowText() );  }
	int      GetTextLength( void ) const						{  return ( GetWindowTextLength() );  }
	BOOL     SetText      ( LPCTSTR text )						{  return ( SetWindowText( text ) );  }
	BOOL     ClearText    ( void )								{  return ( SetText( "" ) );  }
	HICON    GetIcon      ( void ) const						{  return ( winx::Static::GetIcon( *this ) );  }
	HICON    SetIcon      ( HICON icon )						{  return ( winx::Static::SetIcon( *this, icon ) );  }
};

//////////////////////////////////////////////////////////////////////////////
// class StaticImpl declaration

template <typename T, typename BASE = Static, typename TRAITS = CControlWinTraits>
class ATL_NO_VTABLE StaticImpl : public WindowImpl <T, BASE, TRAITS>
{
public:
	typedef StaticImpl <T, BASE, TRAITS> ThisType;
	typedef WindowImpl <T, BASE, TRAITS> Inherited;

	DECLARE_WND_SUPERCLASS( NULL, "Static" );
};

//////////////////////////////////////////////////////////////////////////////
// class Button declaration

class Button : public Window
{
public:
	SET_INHERITED( Button, Window );

	Button( HWND hwnd = NULL ) : Inherited( hwnd )				{  }

	ThisType& operator = ( HWND hwnd )							{  m_hWnd = hwnd;  return *this;  }

	int      GetText      ( LPTSTR text, int maxCount ) const	{  return ( GetWindowText( text, maxCount ) );  }
	gpstring GetText      ( void ) const						{  return ( GetWindowText() );  }
	int      GetTextLength( void ) const						{  return ( GetWindowTextLength() );  }
	BOOL     SetText      ( LPCTSTR text )						{  return ( SetWindowText( text ) );  }
	BOOL     ClearText    ( void )								{  return ( SetText( "" ) );  }
	int      GetCheck     ( void )								{  return ( winx::Button::GetCheck( *this ) );  }
	void     SetCheck     ( DWORD check )						{  winx::Button::SetCheck( *this, check );  }
	bool     GetCheckBool ( void )								{  return ( winx::Button::GetCheckBool( *this ) );  }
	void     SetCheckBool ( bool check )						{  winx::Button::SetCheckBool( *this, check );  }

	/*
#	define GetState(hwndCtl)            ((int)(DWORD)SNDMSG((hwndCtl), BM_GETSTATE, 0L, 0L))
#	define SetState(hwndCtl, state)     ((UINT)(DWORD)SNDMSG((hwndCtl), BM_SETSTATE, (WPARAM)(int)(state), 0L))
#	define SetStyle(hwndCtl, style, fRedraw) ((void)SNDMSG((hwndCtl), BM_SETSTYLE, (WPARAM)LOWORD(style), MAKELPARAM(((fRedraw) ? TRUE : FALSE), 0)))
	*/
};

//////////////////////////////////////////////////////////////////////////////
// class ButtonImpl declaration

template <typename T, typename BASE = Button, typename TRAITS = CControlWinTraits>
class ATL_NO_VTABLE ButtonImpl : public WindowImpl <T, BASE, TRAITS>
{
public:
	typedef ButtonImpl <T, BASE, TRAITS> ThisType;
	typedef WindowImpl <T, BASE, TRAITS> Inherited;

	DECLARE_WND_SUPERCLASS( NULL, "Button" );
};

//////////////////////////////////////////////////////////////////////////////
// class Edit declaration

class Edit : public Window
{
public:
	SET_INHERITED( Edit, Window );

	Edit( HWND hwnd = NULL ) : Inherited( hwnd )					{  }

	ThisType& operator = ( HWND hwnd )								{  m_hWnd = hwnd;  return *this;  }

	int      GetText            ( LPTSTR text, int maxCount ) const	{  return ( GetWindowText( text, maxCount ) );  }
	gpstring GetText            ( void ) const						{  return ( GetWindowText() );  }
	int      GetTextLength      ( void ) const						{  return ( GetWindowTextLength() );  }
	BOOL     SetText            ( LPCTSTR text )					{  return ( SetWindowText( text ) );  }
	BOOL     ClearText          ( void )							{  return ( SetText( "" ) );  }
	bool     CopyTextToClipboard( void ) const						{  return ( winx::Edit::CopyTextToClipboard( *this ) );  }
	int      GetLimitText       ( void ) const						{  return ( winx::Edit::GetLimitText( *this ) );  }
	void     SetLimitText       ( int maxText ) const				{  winx::Edit::SetLimitText( *this, maxText );  }
	void     SetLimitTextMax    ( void ) const						{  winx::Edit::SetLimitTextMax( *this );  }

	/*
#	define GetLineCount(hwndCtl)              ((int)(DWORD)SNDMSG((hwndCtl), EM_GETLINECOUNT, 0L, 0L))
#	define GetLine(hwndCtl, line, lpch, cchMax) ((*((int *)(lpch)) = (cchMax)), ((int)(DWORD)SNDMSG((hwndCtl), EM_GETLINE, (WPARAM)(int)(line), (LPARAM)(LPTSTR)(lpch))))
#	define GetRect(hwndCtl, lprc)             ((void)SNDMSG((hwndCtl), EM_GETRECT, 0L, (LPARAM)(RECT *)(lprc)))
#	define SetRect(hwndCtl, lprc)             ((void)SNDMSG((hwndCtl), EM_SETRECT, 0L, (LPARAM)(const RECT *)(lprc)))
#	define SetRectNoPaint(hwndCtl, lprc)      ((void)SNDMSG((hwndCtl), EM_SETRECTNP, 0L, (LPARAM)(const RECT *)(lprc)))
	*/

	void GetSel       ( int& start, int& end )						{  winx::Edit::GetSel( *this, start, end );  }
	void SetSel       ( int start, int end )						{  winx::Edit::SetSel( *this, start, end );  }
	void SetSelNone   ( void )										{  winx::Edit::SetSelNone( *this );  }
	void SetSelAtBegin( void )										{  winx::Edit::SetSelAtBegin( *this );  }
	void SetSelAtEnd  ( void )										{  winx::Edit::SetSelAtEnd( *this );  }
	void ReplaceSel   ( LPCTSTR replace )							{  winx::Edit::ReplaceSel( *this, replace );  }

	/*
#	define GetModify(hwndCtl)                 ((BOOL)(DWORD)SNDMSG((hwndCtl), EM_GETMODIFY, 0L, 0L))
#	define SetModify(hwndCtl, fModified)      ((void)SNDMSG((hwndCtl), EM_SETMODIFY, (WPARAM)(UINT)(fModified), 0L))
#	define ScrollCaret(hwndCtl)               ((BOOL)(DWORD)SNDMSG((hwndCtl), EM_SCROLLCARET, 0, 0L))
#	define LineFromChar(hwndCtl, ich)         ((int)(DWORD)SNDMSG((hwndCtl), EM_LINEFROMCHAR, (WPARAM)(int)(ich), 0L))
#	define LineIndex(hwndCtl, line)           ((int)(DWORD)SNDMSG((hwndCtl), EM_LINEINDEX, (WPARAM)(int)(line), 0L))
#	define LineLength(hwndCtl, line)          ((int)(DWORD)SNDMSG((hwndCtl), EM_LINELENGTH, (WPARAM)(int)(line), 0L))
#	define Scroll(hwndCtl, dv, dh)            ((void)SNDMSG((hwndCtl), EM_LINESCROLL, (WPARAM)(dh), (LPARAM)(dv)))
#	define CanUndo(hwndCtl)                   ((BOOL)(DWORD)SNDMSG((hwndCtl), EM_CANUNDO, 0L, 0L))
#	define Undo(hwndCtl)                      ((BOOL)(DWORD)SNDMSG((hwndCtl), EM_UNDO, 0L, 0L))
#	define EmptyUndoBuffer(hwndCtl)           ((void)SNDMSG((hwndCtl), EM_EMPTYUNDOBUFFER, 0L, 0L))
#	define SetPasswordChar(hwndCtl, ch)       ((void)SNDMSG((hwndCtl), EM_SETPASSWORDCHAR, (WPARAM)(UINT)(ch), 0L))
#	define SetTabStops(hwndCtl, cTabs, lpTabs) ((void)SNDMSG((hwndCtl), EM_SETTABSTOPS, (WPARAM)(int)(cTabs), (LPARAM)(const int *)(lpTabs)))
#	define FmtLines(hwndCtl, fAddEOL)         ((BOOL)(DWORD)SNDMSG((hwndCtl), EM_FMTLINES, (WPARAM)(BOOL)(fAddEOL), 0L))
#	define GetHandle(hwndCtl)                 ((HLOCAL)(UINT)(DWORD)SNDMSG((hwndCtl), EM_GETHANDLE, 0L, 0L))
#	define SetHandle(hwndCtl, h)              ((void)SNDMSG((hwndCtl), EM_SETHANDLE, (WPARAM)(UINT)(HLOCAL)(h), 0L))
	*/

	int GetFirstVisibleLine( void )									{  return ( winx::Edit::GetFirstVisibleLine( *this ) );  }

	/*
#	define SetReadOnly(hwndCtl, fReadOnly)    ((BOOL)(DWORD)SNDMSG((hwndCtl), EM_SETREADONLY, (WPARAM)(BOOL)(fReadOnly), 0L))
#	define GetPasswordChar(hwndCtl)           ((TCHAR)(DWORD)SNDMSG((hwndCtl), EM_GETPASSWORDCHAR, 0L, 0L))
#	define SetWordBreakProc(hwndCtl, lpfnWordBreak) ((void)SNDMSG((hwndCtl), EM_SETWORDBREAKPROC, 0L, (LPARAM)(EDITWORDBREAKPROC)(lpfnWordBreak)))
#	define GetWordBreakProc(hwndCtl)          ((EDITWORDBREAKPROC)SNDMSG((hwndCtl), EM_GETWORDBREAKPROC, 0L, 0L))
	*/

	bool CopyTextToClipboard( void )								{  return ( winx::Edit::CopyTextToClipboard( *this ) );  }
	void AppendText( LPCTSTR text, bool preserveSel = false )		{  winx::Edit::AppendText( *this, text, preserveSel );  }
	void ScrollToTop( void )										{  winx::Edit::ScrollToTop( *this );  }
};

//////////////////////////////////////////////////////////////////////////////
// class EditImpl declaration

template <typename T, typename BASE = Edit, typename TRAITS = CControlWinTraits>
class ATL_NO_VTABLE EditImpl : public WindowImpl <T, BASE, TRAITS>
{
public:
	typedef EditImpl <T, BASE, TRAITS> ThisType;
	typedef WindowImpl <T, BASE, TRAITS> Inherited;

	DECLARE_WND_SUPERCLASS( NULL, "Edit" );
};

//////////////////////////////////////////////////////////////////////////////
// class ScrollBar declaration

class ScrollBar : public Window
{
public:
	SET_INHERITED( ScrollBar, Window );

	ScrollBar( HWND hwnd = NULL ) : Inherited( hwnd )				{  }

	ThisType& operator = ( HWND hwnd )								{  m_hWnd = hwnd;  return *this;  }

	/*
	// NOTE: flags parameter is a collection of ESB_* values, NOT a boolean!
#	define Show(hwndCtl, fShow)              ShowWindow((hwndCtl), (fShow) ? SW_SHOWNORMAL : SW_HIDE)
#	define SetPos(hwndCtl, pos, fRedraw)     SetScrollPos((hwndCtl), SB_CTL, (pos), (fRedraw))
#	define GetPos(hwndCtl)                   GetScrollPos((hwndCtl), SB_CTL)
#	define SetRange(hwndCtl, posMin, posMax, fRedraw)    SetScrollRange((hwndCtl), SB_CTL, (posMin), (posMax), (fRedraw))
#	define GetRange(hwndCtl, lpposMin, lpposMax)         GetScrollRange((hwndCtl), SB_CTL, (lpposMin), (lpposMax))
	*/
};

//////////////////////////////////////////////////////////////////////////////
// class ScrollBarImpl declaration

template <typename T, typename BASE = ScrollBar, typename TRAITS = CControlWinTraits>
class ATL_NO_VTABLE ScrollBarImpl : public WindowImpl <T, BASE, TRAITS>
{
public:
	typedef ScrollBarImpl <T, BASE, TRAITS> ThisType;
	typedef WindowImpl <T, BASE, TRAITS> Inherited;

	DECLARE_WND_SUPERCLASS( NULL, "ScrollBar" );
};

//////////////////////////////////////////////////////////////////////////////
// class ListBox declaration

class ListBox : public Window
{
public:
	SET_INHERITED( ListBox, Window );

	ListBox( HWND hwnd = NULL ) : Inherited( hwnd )				{  }

	ThisType& operator = ( HWND hwnd )							{  m_hWnd = hwnd;  return *this;  }

	/*
#	define GetCount(hwndCtl)                   ((int)(DWORD)SNDMSG((hwndCtl), LB_GETCOUNT, 0L, 0L))
#	define ResetContent(hwndCtl)               ((BOOL)(DWORD)SNDMSG((hwndCtl), LB_RESETCONTENT, 0L, 0L))
	*/

	int AddString   ( const char* str )							{  return ( winx::ListBox::AddString( *this, str ) );  }
	int InsertString( int index, const char* str )				{  return ( winx::ListBox::InsertString( *this, index, str ) );  }

	/*
#	define AddItemData(hwndCtl, data)          ((int)(DWORD)SNDMSG((hwndCtl), LB_ADDSTRING, 0L, (LPARAM)(data)))
#	define InsertItemData(hwndCtl, index, data) ((int)(DWORD)SNDMSG((hwndCtl), LB_INSERTSTRING, (WPARAM)(int)(index), (LPARAM)(data)))
	*/

	int      DeleteString( int index )							{  return ( winx::ListBox::DeleteString( *this, index ) );  }
	int      GetTextLen  ( int index ) const					{  return ( winx::ListBox::GetTextLen( *this, index ) );  }
	gpstring GetText     ( int index ) const					{  return ( winx::ListBox::GetText( *this, index ) );  }

	/*
#	define GetItemData(hwndCtl, index)         ((LRESULT)(DWORD)SNDMSG((hwndCtl), LB_GETITEMDATA, (WPARAM)(int)(index), 0L))
#	define SetItemData(hwndCtl, index, data)   ((int)(DWORD)SNDMSG((hwndCtl), LB_SETITEMDATA, (WPARAM)(int)(index), (LPARAM)(data)))
#	define FindString(hwndCtl, indexStart, lpszFind) ((int)(DWORD)SNDMSG((hwndCtl), LB_FINDSTRING, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))
#	define FindItemData(hwndCtl, indexStart, data) ((int)(DWORD)SNDMSG((hwndCtl), LB_FINDSTRING, (WPARAM)(int)(indexStart), (LPARAM)(data)))
#	define SetSel(hwndCtl, fSelect, index)     ((int)(DWORD)SNDMSG((hwndCtl), LB_SETSEL, (WPARAM)(BOOL)(fSelect), (LPARAM)(index)))
#	define SelItemRange(hwndCtl, fSelect, first, last)    ((int)(DWORD)SNDMSG((hwndCtl), LB_SELITEMRANGE, (WPARAM)(BOOL)(fSelect), MAKELPARAM((first), (last))))
	*/

	int      GetCurSel    ( void ) const						{  return ( winx::ListBox::GetCurSel( *this ) );  }
	gpstring GetCurSelText( void ) const						{  return ( winx::ListBox::GetCurSelText( *this ) );  }
	bool     SetCurSel    ( int index )							{  return ( winx::ListBox::SetCurSel( *this, index ) );  }

	/*
#	define SelectString(hwndCtl, indexStart, lpszFind) ((int)(DWORD)SNDMSG((hwndCtl), LB_SELECTSTRING, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))
#	define SelectItemData(hwndCtl, indexStart, data)   ((int)(DWORD)SNDMSG((hwndCtl), LB_SELECTSTRING, (WPARAM)(int)(indexStart), (LPARAM)(data)))
#	define GetSel(hwndCtl, index)              ((int)(DWORD)SNDMSG((hwndCtl), LB_GETSEL, (WPARAM)(int)(index), 0L))
#	define GetSelCount(hwndCtl)                ((int)(DWORD)SNDMSG((hwndCtl), LB_GETSELCOUNT, 0L, 0L))
#	define GetTopIndex(hwndCtl)                ((int)(DWORD)SNDMSG((hwndCtl), LB_GETTOPINDEX, 0L, 0L))
#	define GetSelItems(hwndCtl, cItems, lpItems) ((int)(DWORD)SNDMSG((hwndCtl), LB_GETSELITEMS, (WPARAM)(int)(cItems), (LPARAM)(int *)(lpItems)))
#	define SetTopIndex(hwndCtl, indexTop)      ((int)(DWORD)SNDMSG((hwndCtl), LB_SETTOPINDEX, (WPARAM)(int)(indexTop), 0L))
#	define SetColumnWidth(hwndCtl, cxColumn)   ((void)SNDMSG((hwndCtl), LB_SETCOLUMNWIDTH, (WPARAM)(int)(cxColumn), 0L))
#	define GetHorizontalExtent(hwndCtl)        ((int)(DWORD)SNDMSG((hwndCtl), LB_GETHORIZONTALEXTENT, 0L, 0L))
#	define SetHorizontalExtent(hwndCtl, cxExtent)     ((void)SNDMSG((hwndCtl), LB_SETHORIZONTALEXTENT, (WPARAM)(int)(cxExtent), 0L))
#	define SetTabStops(hwndCtl, cTabs, lpTabs) ((BOOL)(DWORD)SNDMSG((hwndCtl), LB_SETTABSTOPS, (WPARAM)(int)(cTabs), (LPARAM)(int *)(lpTabs)))
#	define GetItemRect(hwndCtl, index, lprc)   ((int)(DWORD)SNDMSG((hwndCtl), LB_GETITEMRECT, (WPARAM)(int)(index), (LPARAM)(RECT *)(lprc)))
#	define SetCaretIndex(hwndCtl, index)       ((int)(DWORD)SNDMSG((hwndCtl), LB_SETCARETINDEX, (WPARAM)(int)(index), 0L))
#	define GetCaretIndex(hwndCtl)              ((int)(DWORD)SNDMSG((hwndCtl), LB_GETCARETINDEX, 0L, 0L))
#	define FindStringExact(hwndCtl, indexStart, lpszFind) ((int)(DWORD)SNDMSG((hwndCtl), LB_FINDSTRINGEXACT, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))
#	define SetItemHeight(hwndCtl, index, cy)   ((int)(DWORD)SNDMSG((hwndCtl), LB_SETITEMHEIGHT, (WPARAM)(int)(index), MAKELPARAM((cy), 0)))
#	define GetItemHeight(hwndCtl, index)       ((int)(DWORD)SNDMSG((hwndCtl), LB_GETITEMHEIGHT, (WPARAM)(int)(index), 0L))
#	define Dir(hwndCtl, attrs, lpszFileSpec)   ((int)(DWORD)SNDMSG((hwndCtl), LB_DIR, (WPARAM)(UINT)(attrs), (LPARAM)(LPCTSTR)(lpszFileSpec)))
	*/
};

//////////////////////////////////////////////////////////////////////////////
// class ListBoxImpl declaration

template <typename T, typename BASE = ListBox, typename TRAITS = CControlWinTraits>
class ATL_NO_VTABLE ListBoxImpl : public WindowImpl <T, BASE, TRAITS>
{
public:
	typedef ListBoxImpl <T, BASE, TRAITS> ThisType;
	typedef WindowImpl <T, BASE, TRAITS> Inherited;

	DECLARE_WND_SUPERCLASS( NULL, "ListBox" );
};

//////////////////////////////////////////////////////////////////////////////
// class ComboBox declaration

class ComboBox : public Window
{
public:
	SET_INHERITED( ComboBox, Window );

	ComboBox( HWND hwnd = NULL ) : Inherited( hwnd )			{  }

	ThisType& operator = ( HWND hwnd )							{  m_hWnd = hwnd;  return *this;  }

	int      GetText      ( LPTSTR text, int maxCount ) const	{  return ( GetWindowText( text, maxCount ) );  }
	gpstring GetText      ( void ) const						{  return ( GetWindowText() );  }
	int      GetTextLength( void ) const						{  return ( GetWindowTextLength() );  }
	BOOL     SetText      ( LPCTSTR text )						{  return ( SetWindowText( text ) );  }
	BOOL     ClearText    ( void )								{  return ( SetText( "" ) );  }

	/*
#	define LimitText(hwndCtl, cchLimit)   ((int)(DWORD)SNDMSG((hwndCtl), CB_LIMITTEXT, (WPARAM)(int)(cchLimit), 0L))
#	define GetEditSel(hwndCtl)            ((DWORD)SNDMSG((hwndCtl), CB_GETEDITSEL, 0L, 0L))
#	define SetEditSel(hwndCtl, ichStart, ichEnd) ((int)(DWORD)SNDMSG((hwndCtl), CB_SETEDITSEL, 0L, MAKELPARAM((ichStart), (ichEnd))))
#	define GetCount(hwndCtl)              ((int)(DWORD)SNDMSG((hwndCtl), CB_GETCOUNT, 0L, 0L))
#	define ResetContent(hwndCtl)          ((int)(DWORD)SNDMSG((hwndCtl), CB_RESETCONTENT, 0L, 0L))
	*/

	int      AddString   ( const char* text )					{  return ( winx::ComboBox::AddString( *this, text ) );  }
	int      InsertString( int index, const char* text )		{  return ( winx::ComboBox::InsertString( *this, index, text ) );  }

	/*
#	define AddItemData(hwndCtl, data)     ((int)(DWORD)SNDMSG((hwndCtl), CB_ADDSTRING, 0L, (LPARAM)(data)))
#	define InsertItemData(hwndCtl, index, data) ((int)(DWORD)SNDMSG((hwndCtl), CB_INSERTSTRING, (WPARAM)(int)(index), (LPARAM)(data)))
#	define DeleteString(hwndCtl, index)   ((int)(DWORD)SNDMSG((hwndCtl), CB_DELETESTRING, (WPARAM)(int)(index), 0L))
	*/

	int      GetLBTextLen( int index ) const					{  return ( winx::ComboBox::GetLBTextLen( *this, index ) );  }
	gpstring GetLBText   ( int index ) const					{  return ( winx::ComboBox::GetLBText( *this, index ) );  }

	/*
#	define GetItemData(hwndCtl, index)        ((LRESULT)(DWORD)SNDMSG((hwndCtl), CB_GETITEMDATA, (WPARAM)(int)(index), 0L))
#	define SetItemData(hwndCtl, index, data)  ((int)(DWORD)SNDMSG((hwndCtl), CB_SETITEMDATA, (WPARAM)(int)(index), (LPARAM)(data)))
#	define FindString(hwndCtl, indexStart, lpszFind)  ((int)(DWORD)SNDMSG((hwndCtl), CB_FINDSTRING, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))
#	define FindItemData(hwndCtl, indexStart, data)    ((int)(DWORD)SNDMSG((hwndCtl), CB_FINDSTRING, (WPARAM)(int)(indexStart), (LPARAM)(data)))
	*/

	int      GetCurSel    ( void ) const						{  return ( winx::ComboBox::GetCurSel( *this ) );  }
	gpstring GetCurSelText( void ) const						{  return ( winx::ComboBox::GetCurSelText( *this ) );  }
	bool     SetCurSel    ( int index )							{  return ( winx::ComboBox::SetCurSel( *this, index ) );  }

	/*
#	define SelectString(hwndCtl, indexStart, lpszSelect)  ((int)(DWORD)SNDMSG((hwndCtl), CB_SELECTSTRING, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszSelect)))
#	define SelectItemData(hwndCtl, indexStart, data)      ((int)(DWORD)SNDMSG((hwndCtl), CB_SELECTSTRING, (WPARAM)(int)(indexStart), (LPARAM)(data)))
#	define Dir(hwndCtl, attrs, lpszFileSpec)  ((int)(DWORD)SNDMSG((hwndCtl), CB_DIR, (WPARAM)(UINT)(attrs), (LPARAM)(LPCTSTR)(lpszFileSpec)))
#	define ShowDropdown(hwndCtl, fShow)       ((BOOL)(DWORD)SNDMSG((hwndCtl), CB_SHOWDROPDOWN, (WPARAM)(BOOL)(fShow), 0L))
#	define FindStringExact(hwndCtl, indexStart, lpszFind)  ((int)(DWORD)SNDMSG((hwndCtl), CB_FINDSTRINGEXACT, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))
#	define GetDroppedState(hwndCtl)           ((BOOL)(DWORD)SNDMSG((hwndCtl), CB_GETDROPPEDSTATE, 0L, 0L))
#	define GetDroppedControlRect(hwndCtl, lprc) ((void)SNDMSG((hwndCtl), CB_GETDROPPEDCONTROLRECT, 0L, (LPARAM)(RECT *)(lprc)))
#	define GetItemHeight(hwndCtl)             ((int)(DWORD)SNDMSG((hwndCtl), CB_GETITEMHEIGHT, 0L, 0L))
#	define SetItemHeight(hwndCtl, index, cyItem) ((int)(DWORD)SNDMSG((hwndCtl), CB_SETITEMHEIGHT, (WPARAM)(int)(index), (LPARAM)(int)cyItem))
#	define GetExtendedUI(hwndCtl)             ((UINT)(DWORD)SNDMSG((hwndCtl), CB_GETEXTENDEDUI, 0L, 0L))
#	define SetExtendedUI(hwndCtl, flags)      ((int)(DWORD)SNDMSG((hwndCtl), CB_SETEXTENDEDUI, (WPARAM)(UINT)(flags), 0L))
	*/

	void     push_front( const char* text )						{  InsertString( 0, text );  }
	void     push_back( const char* text )						{  AddString( text );  }
};

//////////////////////////////////////////////////////////////////////////////
// class ComboBoxImpl declaration

template <typename T, typename BASE = ComboBox, typename TRAITS = CControlWinTraits>
class ATL_NO_VTABLE ComboBoxImpl : public WindowImpl <T, BASE, TRAITS>
{
public:
	typedef ComboBoxImpl <T, BASE, TRAITS> ThisType;
	typedef WindowImpl <T, BASE, TRAITS> Inherited;

	DECLARE_WND_SUPERCLASS( NULL, "ComboBox" );
};

//////////////////////////////////////////////////////////////////////////////
// class ListViewImpl declaration

template <typename T, typename BASE = ListView, typename TRAITS = CControlWinTraits>
class ATL_NO_VTABLE ListViewImpl : public WindowImpl <T, BASE, TRAITS>
{
public:
	typedef ListViewImpl <T, BASE, TRAITS> ThisType;
	typedef WindowImpl <T, BASE, TRAITS> Inherited;

	DECLARE_WND_SUPERCLASS( NULL, WC_LISTVIEW );
};

//////////////////////////////////////////////////////////////////////////////
// placement enums

enum eEdge
{
	EDGE_LEFT,
	EDGE_TOP,
	EDGE_RIGHT,
	EDGE_BOTTOM,
};

eEdge GetOtherEdge( eEdge edge );
long& GetEdge( GRect& rect, eEdge edge );
const long& GetEdge( const GRect& rect, eEdge edge );

enum eCorner
{
	CORNER_TOPLEFT,
	CORNER_TOPRIGHT,
	CORNER_BOTTOMRIGHT,
	CORNER_BOTTOMLEFT,
};

enum ePlace
{
	PLACE_FIXED_ABS,			// relative to edge of other window
	PLACE_FIXED_RATIO,			// ratio between edges
	PLACE_FIXED_SELF,			// relative to other edge of local window

	PLACE_FIXED_MOVE_ABS,  		// relative to edge of other window (use move command)
	PLACE_FIXED_MOVE_RATIO,		// ratio between edges (use move command)
};

//////////////////////////////////////////////////////////////////////////////
// class ResizableBase declaration

class ResizableBase
{
public:
	SET_NO_INHERITED( ResizableBase );

	enum eOptions
	{
		OPTION_NOGRIP = 1 << 0,						// don't render "grip"
		// ...

		OPTION_NONE = 0,							// disable all options
	};

	ResizableBase( void );

	void SetMinimumSize( const SIZE& size );

	// local options
	void SetOptions   ( eOptions options, bool set = true )	{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void ClearOptions ( eOptions options )					{  SetOptions( options, false );  }
	void ToggleOptions( eOptions options )					{  m_Options = (eOptions)(m_Options ^ options);  }
	bool TestOptions  ( eOptions options ) const			{  return ( (m_Options & options) != 0 );  }

	void FixEdgeToParent  ( HWND hwnd, eEdge edge );
	void FixCornerToParent( HWND hwnd, eCorner corner );
	void FixEdgeToParent  ( HWND hwnd, eEdge edge, float ratio );
	void FixPosToParent   ( HWND hwnd, eCorner corner );

	void FixWndToParent( HWND hwnd, eEdge edge );
	void FixWndToParent( HWND hwnd, eEdge edge, float ratio );

	void Reset( void );

protected:

	void PrivateFixEdgeToParent  ( HWND hwnd, eEdge edge, bool moveOnly );
	void PrivateFixCornerToParent( HWND hwnd, eCorner corner, bool moveOnly );

	enum  {  GRIP_SIZE = 15  };

	struct Constraint
	{
		Window m_ChildWnd;			// window to constrain
		eEdge  m_ChildEdge;			// which edge to constrain
		GRect  m_ChildRect;			// original rect of this window
		Window m_RelativeWnd;		// relative to which window
		ePlace m_Place;				// how to place

		// additional info
		union
		{
			// for PLACE_FIXED_ABS
			struct
			{
				eEdge m_RelativeEdge;	// which edge of window to be relative to
				GRect m_RelativeRect;	// rect of relative window
			};

			// for PLACE_FIXED_RATIO
			float m_Ratio;				// ratio between edges
		};

		Constraint( void );

		void FixEdgeToParent( HWND hwnd, eEdge edge, bool moveOnly );
		void FixEdgeToParent( HWND hwnd, eEdge edge, bool moveOnly, float ratio );
		void FixEdgeToSelf  ( HWND hwnd, eEdge edge );
		void SetMoveOnly    ( void );
	};

	BOOL  OnCreate         ( Window window, LPCREATESTRUCT createStruct, BOOL& handled );
	void  OnDestroy        ( Window window, BOOL& handled );
	BOOL  OnInitDialog     ( Window window, HWND focus, LPARAM param, BOOL& handled );
	void  OnSize           ( Window window, UINT state, int cx, int cy, BOOL& handled );
	void  OnGetMinMaxInfo  ( Window window, MINMAXINFO* minMaxInfo, BOOL& handled );
	void  OnPaint          ( Window window, BOOL& handled );
	UINT  OnNcHitTest      ( Window window, int x, int y, BOOL& handled );
	void  InvalidateGrip   ( Window window );
	GRect GetClientGripRect( Window window );
	void  CheckGetSize     ( Window window );

	typedef std::vector <Constraint> ConstraintColl;

	ConstraintColl m_ConstraintColl;
	GSize          m_OriginalSize;
	bool           m_SizeSaved;
	eOptions       m_Options;				// general options for this context

	SET_NO_COPYING( ResizableBase );
};

MAKE_ENUM_BIT_OPERATORS( ResizableBase::eOptions );

//////////////////////////////////////////////////////////////////////////////
// class ResizableImpl declaration

template <typename T>
class ResizableImpl : public WindowImpl <T>, public ResizableBase
{
public:
	SET_INHERITED( ResizableImpl, WindowImpl <T> );

protected:
	BEGIN_MSG_MAP_REMOTE( ThisType )
		MESSAGE_CRACKER( WM_CREATE,        OnCreate        )
		MESSAGE_CRACKER( WM_DESTROY,       OnDestroy       )
		MESSAGE_CRACKER( WM_INITDIALOG,    OnInitDialog    )
		MESSAGE_CRACKER( WM_SIZE,          OnSize          )
		MESSAGE_CRACKER( WM_GETMINMAXINFO, OnGetMinMaxInfo )
		MESSAGE_CRACKER( WM_PAINT,         OnPaint         )
		MESSAGE_CRACKER( WM_NCHITTEST,     OnNcHitTest     )
	END_MSG_MAP()

	BOOL OnCreate( LPCREATESTRUCT createStruct, BOOL& handled )
		{  return ( ResizableBase::OnCreate( *this, createStruct, handled ) );  }
	void OnDestroy( BOOL& handled )
		{  ResizableBase::OnDestroy( *this, handled );  }
	BOOL OnInitDialog( HWND focus, LPARAM param, BOOL& handled )
		{  return ( ResizableBase::OnInitDialog( *this, focus, param, handled ) );  }
	void OnSize( UINT state, int cx, int cy, BOOL& handled )
		{  ResizableBase::OnSize( *this, state, cx, cy, handled );  }
	void OnGetMinMaxInfo( MINMAXINFO* minMaxInfo, BOOL& handled )
		{  ResizableBase::OnGetMinMaxInfo( *this, minMaxInfo, handled );  }
	void OnPaint( BOOL& handled )
		{  ResizableBase::OnPaint( *this, handled );  }
	UINT OnNcHitTest( int x, int y, BOOL& handled )
		{  return ( ResizableBase::OnNcHitTest( *this, x, y, handled ) );  }
};

//////////////////////////////////////////////////////////////////////////////
// class PositionSaverBase declaration

class PositionSaverBase
{
public:
	SET_NO_INHERITED( PositionSaverBase );

	enum eOptions
	{
		OPTION_NONE             =      0,
		OPTION_SCREEN_SPACE     = 1 << 0,		// use screen space instead of parent space
		OPTION_SCREEN_CONSTRAIN = 1 << 1,		// constrain the position to the screen
		OPTION_POSITION_ONLY    = 1 << 2,		// only remember the position
		OPTION_DISABLE          = 1 << 3,		// disable this object
	};

	PositionSaverBase( void );
   ~PositionSaverBase( void )						{  Unregister();  }

	void SetName( const char* name )				{  m_Name = name;  }
	void ClearName( void )							{  m_Name.clear();  }

	bool IsActive( void ) const;
	bool IsInitialized( void ) const				{  return ( m_Initialized );  }

	void SetOptions   ( eOptions options, bool set = true )	{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void ClearOptions ( eOptions options )					{  SetOptions( options, false );  }
	void ToggleOptions( eOptions options )					{  m_Options = (eOptions)(m_Options ^ options);  }
	bool TestOptions  ( eOptions options ) const			{  return ( (m_Options & options) != 0 );  }

	void Save( Window window );
	void Restore( Window window );

	static void SaveAll( void );

protected:
	void  OnDestroy   ( Window window, BOOL& handled );
	BOOL  OnInitDialog( Window window, HWND focus, LPARAM param, BOOL& handled );

private:
	gpstring MakeName( Window window ) const;
	void Unregister( void );

	typedef std::pair <PositionSaverBase*, Window> Entry;
	typedef std::vector <Entry> SaverColl;

	eOptions m_Options;
	gpstring m_Name;
	bool     m_Initialized;

	static SaverColl ms_SaverColl;

	SET_NO_COPYING( PositionSaverBase );
};

MAKE_ENUM_BIT_OPERATORS( PositionSaverBase::eOptions );

//////////////////////////////////////////////////////////////////////////////
// class PositionSaver declaration

template <typename T>
class PositionSaverImpl : public WindowImpl <T>, public PositionSaverBase
{
public:
	SET_INHERITED( PositionSaverImpl, WindowImpl <T> );

protected:
	BEGIN_MSG_MAP_REMOTE( ThisType )
		MESSAGE_CRACKER( WM_DESTROY,    OnDestroy    )
		MESSAGE_CRACKER( WM_INITDIALOG, OnInitDialog )
	END_MSG_MAP()

	void OnDestroy( BOOL& handled )
		{  PositionSaverBase::OnDestroy( *this, handled );  }
	BOOL OnInitDialog( HWND focus, LPARAM param, BOOL& handled )
		{  return ( PositionSaverBase::OnInitDialog( *this, focus, param, handled ) );  }
};

//////////////////////////////////////////////////////////////////////////////
// class Dc declaration

class Dc
{
public:
	SET_NO_INHERITED( Dc );

	HDC m_hDc;

// Setup.

	Dc( HDC hdc = NULL )						{  m_hDc = hdc;  }

	Dc& operator = ( HDC hdc )					{  m_hDc = hdc;  return ( *this );  }

	void Attach ( HDC hdc )						{  m_hDc = hdc;  }
	HDC  Detach ( void )						{  HDC old = m_hDc;  m_hDc = NULL;  return ( old );  }
	bool Release( HWND hwnd )					{  gpassert( ::IsWindow( hwnd ) );  return ( ( m_hDc != NULL ) ? !!::ReleaseDC( hwnd, Detach() ) : false );  }
	bool Delete ( void )						{  return ( ( m_hDc != NULL ) ? !!::DeleteDC( Detach() ) : false );  }

// Attributes.

	operator HDC( void ) const					{  return ( m_hDc );  }

	Window GetWindow( void ) const				{  return ( ::WindowFromDC( *this ) );  }

	HPEN     GetCurrentPen    ( void ) const	{  gpassert( m_hDc != NULL );  return ( (HPEN)    ::GetCurrentObject( m_hDc, OBJ_PEN    ) );  }
	HBRUSH   GetCurrentBrush  ( void ) const	{  gpassert( m_hDc != NULL );  return ( (HBRUSH)  ::GetCurrentObject( m_hDc, OBJ_BRUSH  ) );  }
	HPALETTE GetCurrentPalette( void ) const	{  gpassert( m_hDc != NULL );  return ( (HPALETTE)::GetCurrentObject( m_hDc, OBJ_PAL    ) );  }
	HFONT    GetCurrentFont   ( void ) const	{  gpassert( m_hDc != NULL );  return ( (HFONT)   ::GetCurrentObject( m_hDc, OBJ_FONT   ) );  }
	HBITMAP  GetCurrentBitmap ( void ) const	{  gpassert( m_hDc != NULL );  return ( (HBITMAP) ::GetCurrentObject( m_hDc, OBJ_BITMAP ) );  }

// Text functions.

	bool TextOut( int x, int y, const char* str, int count )
		{  gpassert( str != NULL );  return ( !!::TextOut( *this, x, y, str, count ) );  }
	bool TextOut( int x, int y, const char* str )
		{  gpassert( str != NULL );  return ( TextOut( x, y, str, ::strlen( str ) ) );  }
	bool TextOut( int x, int y, const gpstring& str )
		{  return ( TextOut( x, y, str, str.length() ) );  }

// Helpers.

	class Selector
	{
	public:
		Selector( HDC dc, HGDIOBJ obj )		{  gpassert( (dc != NULL) && (obj != NULL) );  m_Dc = dc;  m_Old = ::SelectObject( m_Dc, obj );  }
	   ~Selector( void )					{  ::SelectObject( m_Dc, m_Old );  }

	private:
		HDC     m_Dc;
		HGDIOBJ m_Old;

		SET_NO_COPYING( Selector );
	};
};

//////////////////////////////////////////////////////////////////////////////
// class ClientDc declaration

class ClientDc : public Dc
{
public:
	SET_INHERITED( ClientDc, Dc );

	CWindow m_Wnd;

// Setup.

	ClientDc( HWND hwnd )  {  m_Wnd = hwnd;  Attach( m_Wnd.GetDC() );  }
   ~ClientDc( void )       {  Release( m_Wnd );  }
};

//////////////////////////////////////////////////////////////////////////////
// class WindowDc declaration

class WindowDc : public Dc
{
public:
	SET_INHERITED( WindowDc, Dc );

	CWindow m_Wnd;

// Setup.

	WindowDc( HWND hwnd )  {  m_Wnd = hwnd;  Attach( m_Wnd.GetWindowDC() );  }
   ~WindowDc( void )       {  Release( m_Wnd );  }
};

//////////////////////////////////////////////////////////////////////////////
// class PaintDc declaration

class PaintDc : public Dc
{
public:
	SET_INHERITED( PaintDc, Dc );

	CWindow     m_Wnd;
	PAINTSTRUCT m_Ps;

	PaintDc( HWND hwnd )  {  m_Wnd = hwnd;  Attach( m_Wnd.BeginPaint( &m_Ps ) );  }
   ~PaintDc( void )       {  m_Wnd.EndPaint( &m_Ps );  }
};

//////////////////////////////////////////////////////////////////////////////
// class MsgThreadBase declaration

class MsgThreadBase : public kerneltool::Thread
{
public:
	SET_INHERITED( MsgThreadBase, kerneltool::Thread );

	MsgThreadBase( void )							{  m_QuitCb = NULL;  }
	virtual ~MsgThreadBase( void )					{  }

	bool Start( bool wait = true, const char* threadName = NULL );
	void Stop ( bool wait = true );

	void SetQuitCallback( CBFunctor0 cb )			{  m_QuitCb = cb;  }

protected:
	virtual bool  OnCreate ( void )					{  return ( true );  }
	virtual void  OnDestroy( void )					{  }

private:
	virtual DWORD Execute( void );

	kerneltool::Event m_StartEvent;
	kerneltool::Event m_StopEvent;
	CBFunctor0        m_QuitCb;

	SET_NO_COPYING( MsgThreadBase );
};

//////////////////////////////////////////////////////////////////////////////
// class MsgThread <WND> declaration

template <typename WND, typename CREATEINFO = HWND /*parent*/>
class MsgThread : public MsgThreadBase
{
public:
	SET_INHERITED( MsgThread, MsgThreadBase );

	// ctor/dtor
	MsgThread( void )												{  m_Wnd = NULL;  }
	virtual ~MsgThread( void )										{  }

	// setup
	void Init( WND* wnd )											{  m_Wnd = wnd;  }

	// commands
	bool Start( const CREATEINFO& createInfo, bool wait = true,
				const char* threadName = NULL )						{  m_CreateInfo = createInfo;  return ( Inherited::Start( wait, threadName ) );  }

private:
	virtual bool OnCreate( void )									{  gpassert( m_Wnd != NULL );  return ( m_Wnd->ThreadCreate( m_CreateInfo ) );  }
	virtual void OnDestroy( void )									{  gpassert( m_Wnd != NULL );  m_Wnd->DestroyWindow();  }

	WND*       m_Wnd;
	CREATEINFO m_CreateInfo;

	SET_NO_COPYING( MsgThread );
};

//////////////////////////////////////////////////////////////////////////////
// class AsyncWnd declaration

template <typename CLS, typename BASE, typename CREATEINFO = HWND /*parent*/>
class AsyncWnd : public BASE
{
public:
	SET_INHERITED( AsyncWnd, BASE );
	typedef CREATEINFO CreateInfo;
	typedef BASE AtlInherited;

	/*PRI*/ AsyncWnd( void )										{  m_Thread.Init( this );  }
	/*PRI*/ virtual ~AsyncWnd( void )								{  if ( IsWindow() )  {  m_Thread.Stop();  }  }

	/*PRI*/ void SetQuitCallback( CBFunctor0 cb )					{  m_Thread.SetQuitCallback( cb );  }

	/*PRI*/ virtual bool Create( const CreateInfo& createInfo,
								 const char* threadName = NULL )	{  return ( m_Thread.Start( createInfo, false, threadName ? threadName : "AsyncWnd" ) );  }
	/*AUX*/ virtual bool ThreadCreate( const CreateInfo& createInfo ) = 0;

private:
	typedef atlx::MsgThread <ThisType, CreateInfo> Thread;

	BEGIN_MSG_MAP( ThisType )
		CHAIN_MSG_MAP( Inherited )
		MESSAGE_CRACKER( WM_CLOSE,   OnClose   )
		MESSAGE_CRACKER( WM_DESTROY, OnDestroy )
	END_MSG_MAP()

	/*AUX*/ void OnClose ( BOOL& /*handled*/ )						{  m_Thread.Stop( false );  }
	/*AUX*/ void OnDestroy( BOOL& /*handled*/ )						{  ::PostQuitMessage( 0 );  }

	Thread m_Thread;				// thread for message processing

	SET_NO_COPYING( AsyncWnd );
};

//////////////////////////////////////////////////////////////////////////////
// class AutoRedrawT <T> declaration

template <typename T>
class AutoRedrawT
{
public:
	SET_NO_INHERITED( AutoRedrawT );

	AutoRedrawT( T& obj, bool set = false )
	{
		m_Obj = &obj;
		m_Set = set;
		m_Obj->SetRedraw( m_Set );
	}

   ~AutoRedrawT( void )
	{
		m_Obj->SetRedraw( !m_Set );
	}

private:
	T*   m_Obj;
	bool m_Set;

	SET_NO_COPYING( AutoRedrawT );
};

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace atlx

#endif  // __ATLX_H

//////////////////////////////////////////////////////////////////////////////
