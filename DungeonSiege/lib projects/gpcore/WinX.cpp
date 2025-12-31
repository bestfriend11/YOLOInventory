//////////////////////////////////////////////////////////////////////////////
//
// File     :  WinX.cpp
// Author(s):  Scott Bilas
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "WinX.h"

#include "AtlX.h"
#include "StdHelp.h"

#include <dde.h>
#include <zmouse.h>

//////////////////////////////////////////////////////////////////////////////
// namespace winx

namespace winx  {  // begin of namespace winx

//////////////////////////////////////////////////////////////////////////////
// Helper function implementations

HWND GetFrontWindow( void )
{
	// try normal windows
	HWND front = ::GetActiveWindow();
	if ( front != NULL )
	{
		front = ::GetLastActivePopup( front );
	}

	// hmm...console?
	if ( front == NULL )
	{
		front = GetConsoleWindow();
	}

	return ( front );
}

HWND GetConsoleWindow( void )
{
	// credit to David Lowndes for this code...

	static LPCTSTR temptitle = _T("{B25F9D49-CEED-4b09-8EBA-964800E45756}");
	TCHAR title[512];
	if ( ::GetConsoleTitle( title, ELEMENT_COUNT( title ) ) == 0 )
	{
		return ( NULL );
	}
	::SetConsoleTitle( temptitle );
	HWND wnd = ::FindWindow( NULL, temptitle );
	::SetConsoleTitle( title );
	return ( wnd );
}

void CenterWindow( HWND wnd, HWND base )
{
	CWindow( wnd ).CenterWindow( base );
}

GRect MakeScreenRect( void )
{
	return ( GRect( GPoint::ZERO, GSize( ::GetSystemMetrics( SM_CXSCREEN ), ::GetSystemMetrics( SM_CYSCREEN ) ) ) );
}

GRect MakeVirtualScreenRect( void )
{
	GRect screen( GPoint( ::GetSystemMetrics( SM_XVIRTUALSCREEN  ),
						  ::GetSystemMetrics( SM_YVIRTUALSCREEN  ) ),
				  GSize ( ::GetSystemMetrics( SM_CXVIRTUALSCREEN ),
						  ::GetSystemMetrics( SM_CYVIRTUALSCREEN ) ) );
	if ( screen.IsZero() )
	{
		screen = MakeScreenRect();
	}
	return ( screen );
}

UINT GetLegacyMouseWheelMessage( void )
{
	// $ this is copied from MFC's WINFRM.CPP

	static UINT s_LegacyMouseWheelMessage =
		(   ( (::GetVersion() & 0x80000000) && LOBYTE( LOWORD( ::GetVersion() ) == 4 ))
		 || (!(::GetVersion() & 0x80000000) && LOBYTE( LOWORD( ::GetVersion() ) == 3 )) )
		? ::RegisterWindowMessage( MSH_MOUSEWHEEL ) : 0;

	return ( s_LegacyMouseWheelMessage );
}

UINT GetQueryCancelAutoPlayMessage( void )
{
	static UINT s_QueryCancelAutoPlayMessage = ::RegisterWindowMessage( "QueryCancelAutoPlay" );
	return ( s_QueryCancelAutoPlayMessage );
}

void EatMessages( UINT first, UINT last )
{
	MSG tempMessage;
	while( ::PeekMessage( &tempMessage, NULL, first, last, PM_REMOVE ) );
}

void EatMessages( UINT event )
{
	EatMessages( event, event );
}

void EatAllUserInput( bool keyboard, bool mouse )
{
	//$ just the down stuff (activations) - let the up's through

	// WM_KEYDOWN     - nuke
	// WM_KEYUP       - preserve
	// WM_CHAR        - nuke
	// WM_DEADCHAR    - nuke
	// WM_SYSKEYDOWN  - nuke
	// WM_SYSKEYUP    - preserve
	// WM_SYSCHAR     - nuke
	// WM_SYSDEADCHAR - nuke

	if ( keyboard )
	{
		EatMessages( WM_KEYDOWN );
		EatMessages( WM_CHAR, WM_SYSKEYDOWN );
		EatMessages( WM_SYSCHAR, WM_SYSDEADCHAR );
	}

	// WM_MOUSEMOVE     - nuke
	// WM_LBUTTONDOWN   - nuke
	// WM_LBUTTONUP     - preserve
	// WM_LBUTTONDBLCLK - nuke
	// WM_RBUTTONDOWN   - nuke
	// WM_RBUTTONUP     - preserve
	// WM_RBUTTONDBLCLK - nuke
	// WM_MBUTTONDOWN   - nuke
	// WM_MBUTTONUP     - preserve
	// WM_MBUTTONDBLCLK - nuke
	// WM_MOUSEWHEEL    - nuke
	// WM_XBUTTONDOWN   - nuke
	// WM_XBUTTONUP     - preserve
	// WM_XBUTTONDBLCLK - nuke

	if ( mouse )
	{
		EatMessages( WM_MOUSEMOVE, WM_LBUTTONDOWN );
		EatMessages( WM_LBUTTONDBLCLK, WM_RBUTTONDOWN );
		EatMessages( WM_RBUTTONDBLCLK, WM_MBUTTONDOWN );
		EatMessages( WM_MBUTTONDBLCLK, WM_XBUTTONDOWN );
		EatMessages( WM_XBUTTONDBLCLK );
	}
}

bool IsInputMessage( UINT msg )
{
	return (   ((msg >= WM_KEYFIRST  ) && (msg <= WM_KEYLAST  ))
			|| ((msg >= WM_MOUSEFIRST) && (msg <= WM_MOUSELAST))
			|| (msg == WM_MOUSEWHEEL) || (msg == WM_MOUSEACTIVATE) );
}

/////////////////////////////////////////////////////////////////////////////
// Message conversion

#if !GP_RETAIL

struct MessageString
{
	UINT        m_Msg;
	const char* m_Text;
};

bool operator < ( const MessageString& l, const MessageString& r )
	{  return ( l.m_Msg < r.m_Msg );  }
bool operator < ( const MessageString& l, UINT r )
	{  return ( l.m_Msg < r );  }
bool operator < ( UINT l, const MessageString& r )
	{  return ( l < r.m_Msg );  }

static MessageString s_Messages[] =
{
#	define DEFINE_MESSAGE( wm )  {  wm, #wm  }

	DEFINE_MESSAGE( WM_ACTIVATE          ),
	DEFINE_MESSAGE( WM_ACTIVATEAPP       ),
	DEFINE_MESSAGE( WM_ASKCBFORMATNAME   ),
	DEFINE_MESSAGE( WM_CANCELMODE        ),
	DEFINE_MESSAGE( WM_CAPTURECHANGED    ),
	DEFINE_MESSAGE( WM_CHANGECBCHAIN     ),
	DEFINE_MESSAGE( WM_CHAR              ),
	DEFINE_MESSAGE( WM_CHARTOITEM        ),
	DEFINE_MESSAGE( WM_CHILDACTIVATE     ),
	DEFINE_MESSAGE( WM_CLEAR             ),
	DEFINE_MESSAGE( WM_CLOSE             ),
	DEFINE_MESSAGE( WM_COMMAND           ),
	DEFINE_MESSAGE( WM_COMPACTING        ),
	DEFINE_MESSAGE( WM_COMPAREITEM       ),
	DEFINE_MESSAGE( WM_CONTEXTMENU       ),
	DEFINE_MESSAGE( WM_COPY              ),
	DEFINE_MESSAGE( WM_COPYDATA          ),
	DEFINE_MESSAGE( WM_CREATE            ),
	DEFINE_MESSAGE( WM_CTLCOLORBTN       ),
	DEFINE_MESSAGE( WM_CTLCOLORDLG       ),
	DEFINE_MESSAGE( WM_CTLCOLOREDIT      ),
	DEFINE_MESSAGE( WM_CTLCOLORLISTBOX   ),
	DEFINE_MESSAGE( WM_CTLCOLORMSGBOX    ),
	DEFINE_MESSAGE( WM_CTLCOLORSCROLLBAR ),
	DEFINE_MESSAGE( WM_CTLCOLORSTATIC    ),
	DEFINE_MESSAGE( WM_CUT               ),
	DEFINE_MESSAGE( WM_DDE_ACK           ),
	DEFINE_MESSAGE( WM_DDE_ADVISE        ),
	DEFINE_MESSAGE( WM_DDE_DATA          ),
	DEFINE_MESSAGE( WM_DDE_EXECUTE       ),
	DEFINE_MESSAGE( WM_DDE_INITIATE      ),
	DEFINE_MESSAGE( WM_DDE_POKE          ),
	DEFINE_MESSAGE( WM_DDE_REQUEST       ),
	DEFINE_MESSAGE( WM_DDE_TERMINATE     ),
	DEFINE_MESSAGE( WM_DDE_UNADVISE      ),
	DEFINE_MESSAGE( WM_DEADCHAR          ),
	DEFINE_MESSAGE( WM_DELETEITEM        ),
	DEFINE_MESSAGE( WM_DESTROY           ),
	DEFINE_MESSAGE( WM_DESTROYCLIPBOARD  ),
	DEFINE_MESSAGE( WM_DEVICECHANGE      ),
	DEFINE_MESSAGE( WM_DEVMODECHANGE     ),
	DEFINE_MESSAGE( WM_DISPLAYCHANGE     ),
	DEFINE_MESSAGE( WM_DRAWCLIPBOARD     ),
	DEFINE_MESSAGE( WM_DRAWITEM          ),
	DEFINE_MESSAGE( WM_DROPFILES         ),
	DEFINE_MESSAGE( WM_ENABLE            ),
	DEFINE_MESSAGE( WM_ENDSESSION        ),
	DEFINE_MESSAGE( WM_ENTERIDLE         ),
	DEFINE_MESSAGE( WM_ENTERMENULOOP     ),
	DEFINE_MESSAGE( WM_ERASEBKGND        ),
	DEFINE_MESSAGE( WM_EXITMENULOOP      ),
	DEFINE_MESSAGE( WM_FONTCHANGE        ),
	DEFINE_MESSAGE( WM_GETDLGCODE        ),
	DEFINE_MESSAGE( WM_GETFONT           ),
	DEFINE_MESSAGE( WM_GETICON           ),
	DEFINE_MESSAGE( WM_GETICON           ),
	DEFINE_MESSAGE( WM_GETMINMAXINFO     ),
	DEFINE_MESSAGE( WM_GETTEXT           ),
	DEFINE_MESSAGE( WM_GETTEXTLENGTH     ),
	DEFINE_MESSAGE( WM_HELP              ),
	DEFINE_MESSAGE( WM_HOTKEY            ),
	DEFINE_MESSAGE( WM_HSCROLL           ),
	DEFINE_MESSAGE( WM_HSCROLLCLIPBOARD  ),
	DEFINE_MESSAGE( WM_ICONERASEBKGND    ),
	DEFINE_MESSAGE( WM_INITDIALOG        ),
	DEFINE_MESSAGE( WM_INITMENU          ),
	DEFINE_MESSAGE( WM_INITMENUPOPUP     ),
	DEFINE_MESSAGE( WM_KEYDOWN           ),
	DEFINE_MESSAGE( WM_KEYLAST           ),
	DEFINE_MESSAGE( WM_KEYUP             ),
	DEFINE_MESSAGE( WM_KILLFOCUS         ),
	DEFINE_MESSAGE( WM_LBUTTONDBLCLK     ),
	DEFINE_MESSAGE( WM_LBUTTONDOWN       ),
	DEFINE_MESSAGE( WM_LBUTTONUP         ),
	DEFINE_MESSAGE( WM_MBUTTONDBLCLK     ),
	DEFINE_MESSAGE( WM_MBUTTONDOWN       ),
	DEFINE_MESSAGE( WM_MBUTTONUP         ),
	DEFINE_MESSAGE( WM_MDIACTIVATE       ),
	DEFINE_MESSAGE( WM_MDICASCADE        ),
	DEFINE_MESSAGE( WM_MDICREATE         ),
	DEFINE_MESSAGE( WM_MDIDESTROY        ),
	DEFINE_MESSAGE( WM_MDIGETACTIVE      ),
	DEFINE_MESSAGE( WM_MDIICONARRANGE    ),
	DEFINE_MESSAGE( WM_MDIMAXIMIZE       ),
	DEFINE_MESSAGE( WM_MDINEXT           ),
	DEFINE_MESSAGE( WM_MDIREFRESHMENU    ),
	DEFINE_MESSAGE( WM_MDIRESTORE        ),
	DEFINE_MESSAGE( WM_MDISETMENU        ),
	DEFINE_MESSAGE( WM_MDITILE           ),
	DEFINE_MESSAGE( WM_MEASUREITEM       ),
	DEFINE_MESSAGE( WM_MENUCHAR          ),
	DEFINE_MESSAGE( WM_MENUSELECT        ),
	DEFINE_MESSAGE( WM_MOUSEACTIVATE     ),
	DEFINE_MESSAGE( WM_MOUSEMOVE         ),
	DEFINE_MESSAGE( WM_MOUSEWHEEL        ),
	DEFINE_MESSAGE( WM_MOVE              ),
	DEFINE_MESSAGE( WM_MOVING            ),
	DEFINE_MESSAGE( WM_MOVING            ),
	DEFINE_MESSAGE( WM_NCACTIVATE        ),
	DEFINE_MESSAGE( WM_NCCALCSIZE        ),
	DEFINE_MESSAGE( WM_NCCREATE          ),
	DEFINE_MESSAGE( WM_NCDESTROY         ),
	DEFINE_MESSAGE( WM_NCHITTEST         ),
	DEFINE_MESSAGE( WM_NCLBUTTONDBLCLK   ),
	DEFINE_MESSAGE( WM_NCLBUTTONDOWN     ),
	DEFINE_MESSAGE( WM_NCLBUTTONUP       ),
	DEFINE_MESSAGE( WM_NCMBUTTONDBLCLK   ),
	DEFINE_MESSAGE( WM_NCMBUTTONDOWN     ),
	DEFINE_MESSAGE( WM_NCMBUTTONUP       ),
	DEFINE_MESSAGE( WM_NCMOUSEMOVE       ),
	DEFINE_MESSAGE( WM_NCPAINT           ),
	DEFINE_MESSAGE( WM_NCRBUTTONDBLCLK   ),
	DEFINE_MESSAGE( WM_NCRBUTTONDOWN     ),
	DEFINE_MESSAGE( WM_NCRBUTTONUP       ),
	DEFINE_MESSAGE( WM_NEXTDLGCTL        ),
	DEFINE_MESSAGE( WM_NOTIFY            ),
	DEFINE_MESSAGE( WM_PAINT             ),
	DEFINE_MESSAGE( WM_PAINTCLIPBOARD    ),
	DEFINE_MESSAGE( WM_PALETTECHANGED    ),
	DEFINE_MESSAGE( WM_PALETTEISCHANGING ),
	DEFINE_MESSAGE( WM_PARENTNOTIFY      ),
	DEFINE_MESSAGE( WM_PASTE             ),
	DEFINE_MESSAGE( WM_POWER             ),
	DEFINE_MESSAGE( WM_POWERBROADCAST    ),
	DEFINE_MESSAGE( WM_PRINT             ),
	DEFINE_MESSAGE( WM_PRINT             ),
	DEFINE_MESSAGE( WM_PRINTCLIENT       ),
	DEFINE_MESSAGE( WM_PRINTCLIENT       ),
	DEFINE_MESSAGE( WM_QUERYDRAGICON     ),
	DEFINE_MESSAGE( WM_QUERYENDSESSION   ),
	DEFINE_MESSAGE( WM_QUERYNEWPALETTE   ),
	DEFINE_MESSAGE( WM_QUERYOPEN         ),
	DEFINE_MESSAGE( WM_QUEUESYNC         ),
	DEFINE_MESSAGE( WM_QUIT              ),
	DEFINE_MESSAGE( WM_RBUTTONDBLCLK     ),
	DEFINE_MESSAGE( WM_RBUTTONDOWN       ),
	DEFINE_MESSAGE( WM_RBUTTONUP         ),
	DEFINE_MESSAGE( WM_RENDERALLFORMATS  ),
	DEFINE_MESSAGE( WM_RENDERFORMAT      ),
	DEFINE_MESSAGE( WM_SETCURSOR         ),
	DEFINE_MESSAGE( WM_SETFOCUS          ),
	DEFINE_MESSAGE( WM_SETFONT           ),
	DEFINE_MESSAGE( WM_SETHOTKEY         ),
	DEFINE_MESSAGE( WM_SETICON           ),
	DEFINE_MESSAGE( WM_SETREDRAW         ),
	DEFINE_MESSAGE( WM_SETTEXT           ),
	DEFINE_MESSAGE( WM_SETTINGCHANGE     ),
	DEFINE_MESSAGE( WM_SHOWWINDOW        ),
	DEFINE_MESSAGE( WM_SIZE              ),
	DEFINE_MESSAGE( WM_SIZECLIPBOARD     ),
	DEFINE_MESSAGE( WM_SIZING            ),
	DEFINE_MESSAGE( WM_SIZING            ),
	DEFINE_MESSAGE( WM_SPOOLERSTATUS     ),
	DEFINE_MESSAGE( WM_STYLECHANGED      ),
	DEFINE_MESSAGE( WM_STYLECHANGED      ),
	DEFINE_MESSAGE( WM_STYLECHANGING     ),
	DEFINE_MESSAGE( WM_STYLECHANGING     ),
	DEFINE_MESSAGE( WM_SYSCHAR           ),
	DEFINE_MESSAGE( WM_SYSCOLORCHANGE    ),
	DEFINE_MESSAGE( WM_SYSCOMMAND        ),
	DEFINE_MESSAGE( WM_SYSDEADCHAR       ),
	DEFINE_MESSAGE( WM_SYSKEYDOWN        ),
	DEFINE_MESSAGE( WM_SYSKEYUP          ),
	DEFINE_MESSAGE( WM_TCARD             ),
	DEFINE_MESSAGE( WM_TIMECHANGE        ),
	DEFINE_MESSAGE( WM_TIMER             ),
	DEFINE_MESSAGE( WM_UNDO              ),
	DEFINE_MESSAGE( WM_VKEYTOITEM        ),
	DEFINE_MESSAGE( WM_VSCROLL           ),
	DEFINE_MESSAGE( WM_VSCROLLCLIPBOARD  ),
	DEFINE_MESSAGE( WM_WINDOWPOSCHANGED  ),
	DEFINE_MESSAGE( WM_WINDOWPOSCHANGING ),
	DEFINE_MESSAGE( WM_WININICHANGE      ),
	DEFINE_MESSAGE( WM_APPCOMMAND        ),
	DEFINE_MESSAGE( WM_CHANGEUISTATE     ),
	DEFINE_MESSAGE( WM_GETOBJECT         ),
	DEFINE_MESSAGE( WM_IME_REQUEST       ),
	DEFINE_MESSAGE( WM_MENUCOMMAND       ),
	DEFINE_MESSAGE( WM_MENUDRAG          ),
	DEFINE_MESSAGE( WM_MENUGETOBJECT     ),
	DEFINE_MESSAGE( WM_MENURBUTTONUP     ),
	DEFINE_MESSAGE( WM_MOUSEHOVER        ),
	DEFINE_MESSAGE( WM_MOUSELEAVE        ),
	DEFINE_MESSAGE( WM_NCMOUSEHOVER      ),
	DEFINE_MESSAGE( WM_NCMOUSELEAVE      ),
	DEFINE_MESSAGE( WM_NCXBUTTONDBLCLK   ),
	DEFINE_MESSAGE( WM_NCXBUTTONDOWN     ),
	DEFINE_MESSAGE( WM_NCXBUTTONUP       ),
	DEFINE_MESSAGE( WM_QUERYUISTATE      ),
	DEFINE_MESSAGE( WM_UNINITMENUPOPUP   ),
	DEFINE_MESSAGE( WM_UPDATEUISTATE     ),
	DEFINE_MESSAGE( WM_XBUTTONDBLCLK     ),
	DEFINE_MESSAGE( WM_XBUTTONDOWN       ),
	DEFINE_MESSAGE( WM_XBUTTONUP         ),

#	undef DEFINE_MESSAGE
};

void TraceMsg( const MSG& msg, bool reportAll, ReportSys::ContextRef ctx )
{
	// $ this code is adapted from MFC's _AfxTraceMsg() in AFXTRACE.CPP

	// don't report very frequently sent messages
	if ( !reportAll )
	{
		switch ( msg.message )
		{
			case ( WM_MOUSEMOVE         ):
			case ( WM_NCMOUSEMOVE       ):
			case ( WM_NCHITTEST         ):
			case ( WM_SETCURSOR         ):
			case ( WM_CTLCOLORBTN       ):
			case ( WM_CTLCOLORDLG       ):
			case ( WM_CTLCOLOREDIT      ):
			case ( WM_CTLCOLORLISTBOX   ):
			case ( WM_CTLCOLORMSGBOX    ):
			case ( WM_CTLCOLORSCROLLBAR ):
			case ( WM_CTLCOLORSTATIC    ):
			case ( WM_ENTERIDLE         ):
			case ( WM_CANCELMODE        ):
			case ( 0x0118               ): // WM_SYSTIMER (caret blink)
			{
				return;
			}
		}
	}

	// get some vars
	gpstring messageName;

	// find message
	if ( msg.message >= 0xC000 )
	{
		// window message registered with 'RegisterWindowMessage' (actually a
		// USER atom)
		char buffer[ 100 ];
		if ( ::GetClipboardFormatNameA( msg.message, buffer, ELEMENT_COUNT( buffer ) ) )
		{
			messageName = buffer;
		}
	}
	else if ( msg.message >= WM_USER )
	{
		// user message
		messageName.assignf( "WM_USER+0x%04X", msg.message - WM_USER );
	}
	else
	{
		// sort
		static bool s_Sorted = false;
		if ( !s_Sorted )
		{
			std::sort( s_Messages, ARRAY_END( s_Messages ) );
			s_Sorted = true;
		}

		// a system message
		const MessageString* found = stdx::binary_search( s_Messages, ARRAY_END( s_Messages ), msg.message );
		if ( found != ARRAY_END( s_Messages ) )
		{
			messageName = found->m_Text;
		}
	}

	if ( !messageName.empty() )
	{
		ReportSys::OutputF( ctx, "hwnd = 0x%08X, msg = %s (w = 0x%08X, l = 0x%08X)\n",
							msg.hwnd, messageName.c_str(), msg.wParam, msg.lParam );
	}
	else
	{
		ReportSys::OutputF( ctx, "hwnd = 0x%08X, msg = 0x%08X (w = 0x%08X, l = 0x%08X)\n",
							msg.hwnd, msg.message, msg.wParam, msg.lParam );
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class LogFont implementation

void LogFont :: Reset( void )
{
	ZeroObject( *this );
	lfWeight  = 400;				// normal
	lfCharSet = DEFAULT_CHARSET;	// default set
}

bool LogFont :: SetWeight( int weight )
{
	// parameter validation
	gpassertm( (weight >= 0) && (weight <= 1000), "LogFont: invalid weight" );

	// do it
	bool rc = false;
	if ( lfWeight != weight )
	{
		lfWeight = weight;
		rc = true;
	}
	return ( rc );
}

bool LogFont :: SetBold( bool enable )
{
	return ( SetWeight( enable ? FW_BOLD : FW_NORMAL ) );
}

template <typename T1, typename T2>
bool Change( T1& compare, T2 test )
{
	bool rc = false;
	if ( compare != scast <T1> ( test ) )
	{
		compare = scast <T1> ( test );
		rc = true;
	}
	return ( rc );
}

bool LogFont :: SetItalic( bool enable )
{
	return ( Change( lfItalic, enable ) );
}

bool LogFont :: SetUnderline( bool enable )
{
	return ( Change( lfUnderline, enable ) );
}

bool LogFont :: SetStrikeout( bool enable )
{
	return ( Change( lfStrikeOut, enable ) );
}

bool LogFont :: SetPitchAndFamily( BYTE value )
{
	return ( Change( lfPitchAndFamily, value ) );
}

bool LogFont :: SetHeight( int height )
{
	return ( Change( lfHeight, height ) );
}

bool LogFont :: SetHeightPoints( int points, HDC dc )
{
	//$$$ handle this better with auto classes! at least add error handling!

	HDC hdc;
	if ( dc == NULL )
	{
		hdc = ::GetDC( NULL );
	}
	else
	{
		hdc = dc;
	}

	POINT pt;
	pt.y = ::GetDeviceCaps( hdc, LOGPIXELSY ) * points;
	pt.y /= 72;    // 72 points/inch, 10 decipoints/point
	::DPtoLP( hdc, &pt, 1 );
	POINT ptOrg = { 0, 0 };
	::DPtoLP( hdc, &ptOrg, 1 );

	int height = pt.y - ptOrg.y;
	if ( height > 0 )
	{
		height = -height;
	}

	if ( dc == NULL )
	{
		::ReleaseDC( NULL, hdc );
	}

	return ( SetHeight( height ) );
}

bool LogFont :: SetName( const char* name )
{
	gpassert( name != NULL );

	// do it
	bool rc = false;
	if ( ::strcmp( lfFaceName, name ) != 0 )
	{
		::strcpy( lfFaceName, name );
		rc = true;
	}
	return (rc);
}

//////////////////////////////////////////////////////////////////////////////
// class Font implementation

bool Font :: Create( const LogFont& logFont )
{
	Reset();
	m_Font = ::CreateFontIndirect( &logFont );
	return ( DoesExist() );
}

bool Font :: CreateStock( int stockFont )
{
	Reset();
	m_Font = (HFONT)::GetStockObject( stockFont );
	return ( DoesExist() );
}

void Font :: Reset( void )
{
	if ( DoesExist() )
	{
		gpverify( ::DeleteObject( Get() ) );
		m_Font = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////
// class Brush implementation

bool Brush :: CreateHatch( int style, COLORREF fg )
{
	Reset();
	m_Brush = ::CreateHatchBrush( style, fg );
	return ( DoesExist() );
}

void Brush :: Reset( void )
{
	if ( DoesExist() )
	{
		gpverify( ::DeleteObject( Get() ) );
		m_Brush = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////
// class WaitCursor implementation

int WaitCursor::ms_WaitCount = 0;
HCURSOR WaitCursor::ms_OldCursor = NULL;

//////////////////////////////////////////////////////////////////////////////
// wrappers

bool SetClipboardText( const char* text, int len )
{
	gpassert( text != NULL );

	bool success = false;

	// auto-detect length
	if ( len == -1 )
	{
		len = ::strlen( text );
	}

	// clipboard has to work with global mem
	HGLOBAL hbuf = ::GlobalAlloc( GHND | GMEM_DDESHARE, len + 1 );
	if ( hbuf != NULL )
	{
		char* buf = rcast <char*> ( GlobalLock( hbuf ) );
		if ( buf != NULL )
		{
			// copy out the text
			::memcpy( buf, text, len );
			buf[ len ] = '\0';
			::GlobalUnlock( hbuf );

			// put it in the clipboard
			if ( ::OpenClipboard( NULL ) )
			{
				if ( ::EmptyClipboard() && ::SetClipboardData( CF_TEXT, hbuf ) )
				{
					success = true;
				}

				::CloseClipboard();
			}
		}

		// free memory if clipboard didn't take it
		if ( !success )
		{
			::GlobalFree( hbuf );
		}
	}

	// done
	return ( success );
}

bool SetClipboardText( const gpstring& text )
{
	return ( SetClipboardText( text, text.length() ) );
}

gpstring GetClipboardText( void )
{
	gpstring out;

	if ( ::OpenClipboard( NULL ) )
	{
		HGLOBAL hmem = ::GetClipboardData( CF_TEXT );
		if ( hmem != NULL )
		{
			const char* str = rcast <const char*> ( ::GlobalLock( hmem ) );
			if ( str != NULL )
			{
				out = str;
			}
		}

		::CloseClipboard();
	}

	return ( out );
}

gpstring GetWindowText( HWND hwnd )
{
	gpassert( ::IsWindow( hwnd ) );
	gpstring text;
	text.resize( ::GetWindowTextLength( hwnd ) );
	::GetWindowText( hwnd, text.begin_split(), text.length() + 1 );
	return ( text );
}

//////////////////////////////////////////////////////////////////////////////
// namespace Edit

namespace Edit  {  // begin of namespace Edit

void SetLimitText( HWND hwnd, int maxText )
{
	gpassert( ::IsWindow( hwnd ) );

	if ( SysInfo::IsWin9x() )
	{
		clamp_max( maxText, 0x7FFE );
	}
	else
	{
		clamp_max( maxText, 0x7FFFFFFE );
	}

	::SendMessage( hwnd, EM_LIMITTEXT, maxText, 0L );
}

void SetLimitTextMax( HWND hwnd )
{
	if ( SysInfo::IsWin9x() )
	{
		SetLimitText( hwnd, 0x7FFE );
	}
	else
	{
		SetLimitText( hwnd, 0x7FFFFFFE );
	}
}

bool CopyTextToClipboard( HWND hwnd )
{
	gpassert( ::IsWindow( hwnd ) );

	bool success = false;
	DWORD error = ERROR_SUCCESS;

	// get some params
	int len = GetTextLength( hwnd ) + 1;

	// clipboard has to work with global mem
	HGLOBAL htext = ::GlobalAlloc( GHND | GMEM_DDESHARE, len );
	if ( htext != NULL )
	{
		char* text = rcast <char*> ( GlobalLock( htext ) );
		if ( text != NULL )
		{
			// copy out the text
			GetText( hwnd, text, len );
			::GlobalUnlock( htext );

			// put it in the clipboard
			if ( ::OpenClipboard( hwnd ) )
			{
				if ( ::EmptyClipboard() && ::SetClipboardData( CF_TEXT, htext ) )
				{
					success = true;
				}
				else
				{
					// preserve error code
					error = ::GetLastError();
				}

				::CloseClipboard();
			}
			else
			{
				// preserve error code
				error = ::GetLastError();
			}
		}
		else
		{
			// preserve error code
			error = ::GetLastError();
		}

		// free memory if clipboard didn't take it
		if ( !success )
		{
			::GlobalFree( htext );
		}
	}

	// restore error code
	if ( !success && (error != ERROR_SUCCESS) )
	{
		::SetLastError( error );
	}

	// done
	return ( success );
}

void AppendText( HWND hwnd, LPCTSTR text, bool preserveSel )
{
	int oldStart, oldEnd;
	GetSel( hwnd, oldStart, oldEnd );
	SetSelAtEnd( hwnd );
	ReplaceSel( hwnd, text );
	if ( preserveSel )
	{
		SetSel( hwnd, oldStart, oldEnd );
	}
}

}	// end of namespace Edit

//////////////////////////////////////////////////////////////////////////////
// namespace ListBox

namespace ListBox  {  // begin of namespace ListBox

gpstring GetText( HWND hwnd, int index )
{
	gpassert( ::IsWindow( hwnd ) );
	gpstring text;

	int len = GetTextLen( hwnd, index );
	gpassert( len != LB_ERR );
	if ( len != LB_ERR )
	{
		text.resize( len );
		::SendMessage( hwnd, LB_GETTEXT, (WPARAM)index, (LPARAM)text.begin() );
	}
	return ( text );
}

}	// end of namespace ListBox

//////////////////////////////////////////////////////////////////////////////
// namespace ComboBox

namespace ComboBox  {  // begin of namespace ComboBox

gpstring GetLBText( HWND hwnd, int index )
{
	gpassert( ::IsWindow( hwnd ) );
	gpstring text;

	int len = GetLBTextLen( hwnd, index );
	gpassert( len != CB_ERR );
	if ( len != CB_ERR )
	{
		text.resize( len );
		::SendMessage( hwnd, CB_GETLBTEXT, (WPARAM)index, (LPARAM)text.begin() );
	}
	return ( text );
}

}	// end of namespace ComboBox

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace winx

//////////////////////////////////////////////////////////////////////////////
