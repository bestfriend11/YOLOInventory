//////////////////////////////////////////////////////////////////////////////
//
// File     :  AtlX.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "AtlX.h"

#include "AppModule.h"
#include "Config.h"
#include "FuBiTraitsImpl.h"
#include "StdHelp.h"

namespace atlx  {  // begin of namespace atlx

//////////////////////////////////////////////////////////////////////////////
// module singleton

CComModule* gs_ModulePtr = NULL;

CComModule& GetModule( void )
{
	if ( gs_ModulePtr == NULL )
	{
		gs_ModulePtr = &Module::GetSingleton();
	}
	return ( *gs_ModulePtr );
}

void SetModule( CComModule* module )
{
	gs_ModulePtr = module;
}

//////////////////////////////////////////////////////////////////////////////
// class Dialog implementation

// collection of managed modeless dialogs
typedef std::vector <HWND> HwndColl;
DEFINE_POST_GLOBAL( HwndColl, HwndColl );
#define gHwndColl GetHwndColl()

void Dialog :: AddModelessDialog( HWND hwnd )
{
	gpassert( stdx::has_none( gHwndColl, hwnd )  );
	gHwndColl.push_back( hwnd );
}

void Dialog :: RemoveModelessDialog( HWND hwnd )
{
	HwndColl::iterator found = stdx::find( gHwndColl, hwnd );
	gpassert( found != gHwndColl.end() );
	if ( found != gHwndColl.end() )
	{
		gHwndColl.erase( found );
	}
}

bool Dialog :: ProcessModelessDialogMsg( MSG& msg )
{
	bool handled = false;

	// see if the active window is in our modeless dialog set
	HWND active = ::GetActiveWindow();
	if ( stdx::has_any( gHwndColl, active ) )
	{
		// see if it's a dialog message
		handled = !!::IsDialogMessage( active, &msg );
	}

	// return whether or not we processed it
	return ( handled );
}

//////////////////////////////////////////////////////////////////////////////
// eEdge implementation

eEdge GetOtherEdge( eEdge edge )
{
	switch ( edge )
	{
		case ( EDGE_LEFT   ):  return ( EDGE_RIGHT  );
		case ( EDGE_TOP    ):  return ( EDGE_BOTTOM );
		case ( EDGE_RIGHT  ):  return ( EDGE_LEFT   );
		case ( EDGE_BOTTOM ):  return ( EDGE_TOP    );
	}
	gpassert( 0 );
	return ( EDGE_LEFT );
}

long& GetEdge( GRect& rect, eEdge edge )
{
	switch ( edge )
	{
		case ( EDGE_LEFT   ):  return ( rect.left   );
		case ( EDGE_TOP    ):  return ( rect.top    );
		case ( EDGE_RIGHT  ):  return ( rect.right  );
		case ( EDGE_BOTTOM ):  return ( rect.bottom );
	}
	gpassert( 0 );
	return ( rect.left );
}

const long& GetEdge( const GRect& rect, eEdge edge )
{
	switch ( edge )
	{
		case ( EDGE_LEFT   ):  return ( rect.left   );
		case ( EDGE_TOP    ):  return ( rect.top    );
		case ( EDGE_RIGHT  ):  return ( rect.right  );
		case ( EDGE_BOTTOM ):  return ( rect.bottom );
	}
	gpassert( 0 );
	return ( rect.left );
}

//////////////////////////////////////////////////////////////////////////////
// class ResizableBase implementation

ResizableBase :: ResizableBase( void )
{
	m_SizeSaved = false;
	m_Options = OPTION_NONE;
}

void ResizableBase :: SetMinimumSize( const SIZE& size )
{
	m_OriginalSize = size;
	m_SizeSaved = true;
}

void ResizableBase :: FixEdgeToParent( HWND hwnd, eEdge edge )
{
	PrivateFixEdgeToParent( hwnd, edge, false );
}

void ResizableBase :: FixPosToParent( HWND hwnd, eCorner corner )
{
	PrivateFixCornerToParent( hwnd, corner, true );
}

void ResizableBase :: FixCornerToParent( HWND hwnd, eCorner corner )
{
	PrivateFixCornerToParent( hwnd, corner, false );
}

void ResizableBase :: FixEdgeToParent( HWND hwnd, eEdge edge, float ratio )
{
	Constraint constraint;
	constraint.FixEdgeToParent( hwnd, edge, false, ratio );
	m_ConstraintColl.push_back( constraint );
}

void ResizableBase :: FixWndToParent( HWND hwnd, eEdge edge )
{
	FixEdgeToParent( hwnd, edge );
	Constraint constraint;
	constraint.FixEdgeToSelf( hwnd, GetOtherEdge( edge ) );
	m_ConstraintColl.push_back( constraint );
}

void ResizableBase :: FixWndToParent( HWND hwnd, eEdge edge, float ratio )
{
	FixEdgeToParent( hwnd, edge, ratio );
	Constraint constraint;
	constraint.FixEdgeToSelf( hwnd, GetOtherEdge( edge ) );
	m_ConstraintColl.push_back( constraint );
}

void ResizableBase :: Reset( void )
{
	m_ConstraintColl.clear();
	m_SizeSaved = false;
}

void ResizableBase :: PrivateFixEdgeToParent( HWND hwnd, eEdge edge, bool moveOnly )
{
	Constraint constraint;
	constraint.FixEdgeToParent( hwnd, edge, moveOnly );
	m_ConstraintColl.push_back( constraint );
}

void ResizableBase :: PrivateFixCornerToParent( HWND hwnd, eCorner corner, bool moveOnly )
{
	switch ( corner )
	{
		case ( CORNER_TOPLEFT ):
		{
			// $ don't do anything - this is the default
		}	break;
		case ( CORNER_TOPRIGHT ):
		{
			PrivateFixEdgeToParent( hwnd, atlx::EDGE_RIGHT, moveOnly );
		}	break;
		case ( CORNER_BOTTOMRIGHT ):
		{
			PrivateFixEdgeToParent( hwnd, atlx::EDGE_BOTTOM, moveOnly );
			PrivateFixEdgeToParent( hwnd, atlx::EDGE_RIGHT, moveOnly );
		}	break;
		case ( CORNER_BOTTOMLEFT ):
		{
			PrivateFixEdgeToParent( hwnd, atlx::EDGE_BOTTOM, moveOnly );
		}	break;
		default:
		{
			gpassert( 0 );		// should never get here!
		}
	}
}

BOOL ResizableBase :: OnCreate( Window window, LPCREATESTRUCT /*createStruct*/, BOOL& handled )
{
	handled = false;
	CheckGetSize( window );
	return ( TRUE );
}

void ResizableBase :: OnDestroy( Window /*window*/, BOOL& handled )
{
	handled = false;
	Reset();
}

BOOL ResizableBase :: OnInitDialog( Window window, HWND /*focus*/, LPARAM /*param*/, BOOL& handled )
{
	handled = false;
	CheckGetSize( window );
	return ( TRUE );
}

void ResizableBase :: OnSize( Window window, UINT state, int /*cx*/, int /*cy*/, BOOL& handled )
{
	handled = false;

	if ( state != 0 )
	{
		return;
	}

	CheckGetSize( window );

	// $$ future: buffer up movewindow directives and do them in a batch rather
	//    than one at a time. multiple constraints may be applied to the same
	//    window, which will cause lots of excess flashing.

	// run through op codes
	ConstraintColl::iterator i, ibegin = m_ConstraintColl.begin(), iend = m_ConstraintColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		GRect child = i->m_ChildWnd.GetWindowRect();
		bool moveOnly = false;
		if ( i->m_Place == PLACE_FIXED_SELF )
		{
			switch ( i->m_ChildEdge )
			{
				case ( EDGE_LEFT   ):  child.left   = child.right  - i->m_ChildRect.Width ();  break;
				case ( EDGE_TOP    ):  child.top    = child.bottom - i->m_ChildRect.Height();  break;
				case ( EDGE_RIGHT  ):  child.right  = child.left   + i->m_ChildRect.Width ();  break;
				case ( EDGE_BOTTOM ):  child.bottom = child.top    + i->m_ChildRect.Height();  break;
				default:               gpassert( 0 );
			}
		}
		else if ( !i->m_RelativeWnd.IsNull() )
		{
			GRect relativeRect = i->m_RelativeWnd.GetClientScreenRect();
			switch ( i->m_Place )
			{
				case ( PLACE_FIXED_MOVE_ABS ):
					moveOnly = true;
					// $ fallthrough
				case ( PLACE_FIXED_ABS ):
				{
					GetEdge( child, i->m_ChildEdge ) = GetEdge( relativeRect,      i->m_RelativeEdge )
													 + GetEdge( i->m_ChildRect,    i->m_ChildEdge )
													 - GetEdge( i->m_RelativeRect, i->m_RelativeEdge );
				}	break;

				case ( PLACE_FIXED_MOVE_RATIO ):
					moveOnly = true;
					// $ fallthrough
				case ( PLACE_FIXED_RATIO ):
				{
					if ( (i->m_ChildEdge == EDGE_LEFT) || (i->m_ChildEdge == EDGE_RIGHT) )
					{
						GetEdge( child, i->m_ChildEdge ) = relativeRect.left
														 + (int)(relativeRect.Width() * i->m_Ratio);
					}
					else
					{
						GetEdge( child, i->m_ChildEdge ) = relativeRect.top
														 + (int)(relativeRect.Height() * i->m_Ratio);
					}
				}	break;
			}
		}

		window.ScreenToClient( child );
		if ( moveOnly )
		{
			i->m_ChildWnd.SetWindowPos( NULL, child.left, child.top, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
		}
		else
		{
			i->m_ChildWnd.MoveWindow( child );
		}
	}
}

void ResizableBase :: OnGetMinMaxInfo( Window window, MINMAXINFO* minMaxInfo, BOOL& handled )
{
	handled = false;

	if ( m_SizeSaved )
	{
		minMaxInfo->ptMinTrackSize.x = m_OriginalSize.cx;
		minMaxInfo->ptMinTrackSize.y = m_OriginalSize.cy;

		InvalidateGrip( window );
	}
}

void ResizableBase :: OnPaint( Window window, BOOL& handled )
{
	if ( !TestOptions( OPTION_NOGRIP ) )
	{
		PAINTSTRUCT paint;
		HDC hdc = window.BeginPaint( &paint );
		::DrawFrameControl( hdc, GetClientGripRect( window ), DFC_SCROLL, DFCS_SCROLLSIZEGRIP );
		window.EndPaint( &paint );
	}
	else
	{
		handled = false;
	}
}

UINT ResizableBase :: OnNcHitTest( Window window, int x, int y, BOOL& handled )
{
	handled = false;
	UINT ht = FORWARD_WM_NCHITTEST( window, x, y, ::DefWindowProc );

	if ( !TestOptions( OPTION_NOGRIP ) && ((ht == HTCLIENT) || (ht == HTNOWHERE)) )
	{
		// get grip
		GRect rect = window.GetClientToScreen( GetClientGripRect( window ) );

		// extend out to window edge
		GRect wndRect = window.GetWindowRect();
		rect.right = wndRect.right;
		rect.bottom = wndRect.bottom;

		// test
		if ( rect.Contains( GPoint( x, y ) ) )
		{
			ht = HTBOTTOMRIGHT;
			handled = true;
		}
	}
	return ( ht );
}

void ResizableBase :: InvalidateGrip( Window window )
{
	if ( !TestOptions( OPTION_NOGRIP ) )
	{
		window.InvalidateRect( GetClientGripRect( window ), TRUE );
	}
}

GRect ResizableBase :: GetClientGripRect( Window window )
{
	GRect rect = window.GetClientRect();
	rect.left = rect.right - GRIP_SIZE;
	rect.top  = rect.bottom - GRIP_SIZE;
	return ( rect );
}

void ResizableBase :: CheckGetSize( Window window )
{
	// if first time in here, save the size for minimum tracking
	if ( !m_SizeSaved )
	{
		m_OriginalSize = window.GetWindowSize();
		m_SizeSaved = true;
	}
}

//////////////////////////////////////////////////////////////////////////////
// class ResizableBase::Constraint implementation

ResizableBase::Constraint :: Constraint( void )
{
	// this space intentionally left blank...
}

void ResizableBase::Constraint :: FixEdgeToParent( HWND hwnd, eEdge edge, bool moveOnly )
{
	m_ChildWnd     = hwnd;
	m_ChildEdge    = edge;
	m_ChildRect    = m_ChildWnd.GetWindowRect();
	m_RelativeWnd  = m_ChildWnd.GetParent();
	m_Place        = moveOnly ? PLACE_FIXED_MOVE_ABS : PLACE_FIXED_ABS;
	m_RelativeEdge = edge;
	m_RelativeRect = m_RelativeWnd.GetClientScreenRect();

	if ( moveOnly )
	{
		SetMoveOnly();
	}
}

void ResizableBase::Constraint :: FixEdgeToParent( HWND hwnd, eEdge edge, bool moveOnly, float ratio )
{
	m_ChildWnd     = hwnd;
	m_ChildEdge    = edge;
	m_ChildRect    = m_ChildWnd.GetWindowRect();
	m_RelativeWnd  = m_ChildWnd.GetParent();
	m_Place        = moveOnly ? PLACE_FIXED_MOVE_RATIO : PLACE_FIXED_RATIO;
	m_Ratio        = ratio;

	if ( moveOnly )
	{
		SetMoveOnly();
	}
}

void ResizableBase::Constraint :: FixEdgeToSelf( HWND hwnd, eEdge edge )
{
	m_ChildWnd     = hwnd;
	m_ChildEdge    = edge;
	m_ChildRect    = m_ChildWnd.GetWindowRect();
	m_Place        = PLACE_FIXED_SELF;
}

void ResizableBase::Constraint :: SetMoveOnly( void )
{
	if ( m_ChildEdge == EDGE_RIGHT )
	{
		m_ChildEdge = EDGE_LEFT;
	}
	else if ( m_ChildEdge == EDGE_BOTTOM )
	{
		m_ChildEdge = EDGE_TOP;
	}
}

//////////////////////////////////////////////////////////////////////////////
// class PositionSaverBase implementation

PositionSaverBase::SaverColl PositionSaverBase::ms_SaverColl;

bool SaveAllProc( void )
{
	PositionSaverBase::SaveAll();
	return ( true );
}

PositionSaverBase :: PositionSaverBase( void )
{
	m_Options = OPTION_SCREEN_CONSTRAIN;
	m_Initialized = false;

	// register for shutdown
	if ( AppModule::DoesSingletonExist() )
	{
		gAppModule.RegisterEventCb( makeFunctor( SaveAllProc ), AppModule::PRI_SHUTDOWN - 1 );
	}
}

bool PositionSaverBase :: IsActive( void ) const
{
	return ( Config::DoesSingletonExist() && !TestOptions( OPTION_DISABLE ) );
}

void PositionSaverBase :: Save( Window window )
{
	if ( IsActive() )
	{
		gpstring name = MakeName( window );
		if ( !name.empty() )
		{
			GRect rect = window.GetWindowRect();
			if ( !TestOptions( OPTION_SCREEN_SPACE ) )
			{
				window.ScreenToParent( rect );
			}

			gConfig.Set( "window_state\\" + name, rect );
		}
	}
}

void PositionSaverBase :: Restore( Window window )
{
	if ( IsActive() )
	{
		gpstring name = MakeName( window );
		if ( !name.empty() )
		{
			GRect rect;
			if ( gConfig.Read( "window_state\\" + name, rect ) )
			{
				if ( !TestOptions( OPTION_SCREEN_SPACE ) )
				{
					window.ParentToScreen( rect );
				}

				if ( TestOptions( OPTION_SCREEN_CONSTRAIN ) )
				{
					rect.ConstrainTo( winx::MakeVirtualScreenRect() );
				}

				DWORD flags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER;
				if ( TestOptions( OPTION_POSITION_ONLY ) )
				{
					flags |= SWP_NOSIZE;
				}
				window.SetWindowPos( NULL, rect, flags );
				m_Initialized = true;
			}
		}
	}
}

void PositionSaverBase :: SaveAll( void )
{
	SaverColl::iterator i, begin = ms_SaverColl.begin(), end = ms_SaverColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		i->first->Save( i->second );
	}
}

void PositionSaverBase :: OnDestroy( Window window, BOOL& handled )
{
	handled = false;
	Save( window );
	Unregister();
}

BOOL PositionSaverBase :: OnInitDialog( Window window, HWND /*focus*/, LPARAM /*param*/, BOOL& handled )
{
	handled = false;
	m_Initialized = false;

	Restore( window );
	ms_SaverColl.push_back( Entry( this, window ) );

	return ( TRUE );
}

gpstring PositionSaverBase :: MakeName( Window window ) const
{
	gpstring name = m_Name;
	if ( name.empty() )
	{
		name = window.GetWindowText();
	}
	return ( name );
}

void PositionSaverBase :: Unregister( void )
{
	SaverColl::iterator i, begin = ms_SaverColl.begin(), end = ms_SaverColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->first == this )
		{
			ms_SaverColl.erase( i );
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// class MsgThreadBase implementation

bool MsgThreadBase :: Start( bool wait, const char* threadName )
{
	bool rc = BeginThread( threadName ? threadName : "MsgPump" );
	if ( rc && wait )
	{
		m_StartEvent.Wait();
	}
	return ( rc );
}

void MsgThreadBase :: Stop( bool wait )
{
	m_StopEvent.Set();
	if ( wait )
	{
		WaitForExit();
	}
}

DWORD MsgThreadBase :: Execute( void )
{
	if ( OnCreate() )
	{
		m_StartEvent.Set();
		bool quit = false;

		while ( !quit )
		{
			HANDLE stop = m_StopEvent.GetValid();
			DWORD rc = ::MsgWaitForMultipleObjects( 1, &stop, FALSE, INFINITE, QS_ALLINPUT );
			if ( rc == WAIT_OBJECT_0 )
			{
				m_StopEvent.Reset();
				OnDestroy();
			}

			MSG msg;
			while ( ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
			{
				if ( msg.message == WM_QUIT )
				{
					m_StartEvent.Reset();
					quit = true;
					break;
				}

				if ( !Dialog::ProcessModelessDialogMsg( msg ) )
				{
					::TranslateMessage( &msg );
					::DispatchMessage ( &msg );
				}
			}
		}
	}

	if ( m_QuitCb )
	{
		m_QuitCb();
	}

	return ( 0 );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace atlx

//////////////////////////////////////////////////////////////////////////////
