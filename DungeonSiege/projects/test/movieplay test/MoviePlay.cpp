/**************************************************************************

Filename		: MoviePlay.cpp

Description		: This file contains the class functions for playing 
				  various movie types using DirectShow

Creation Date	: 6/30/99

**************************************************************************/


// Include Files
#include "MoviePlay.h"
#include <windows.h>
#include <mmsystem.h>


/**************************************************************************

Function		: MoviePlay()

Purpose			: Constructor 

Returns			: No return type

**************************************************************************/
MoviePlay::MoviePlay()
: m_piGraphBuilder(NULL)
, m_piMediaControl(NULL)
, m_piMediaEventEx(NULL)
, m_piVideoWindow(NULL)
, m_bPlaying(false)
, m_piFilter(NULL)
{
}


/**************************************************************************

Function		: ~MoviePlay()

Purpose			: Destructor 

Returns			: No return type

**************************************************************************/
MoviePlay::~MoviePlay()
{
}


/**************************************************************************

Function		: Play()

Purpose			: Plays a movie file

Returns			: void

**************************************************************************/
void MoviePlay::Play( char *szFilename, HWND hWnd, int x1, int y1, int x2, int y2 )
{
	
	CoInitialize(NULL);
	
	WCHAR wFile[MAX_PATH];
	MultiByteToWideChar( CP_ACP, 0, szFilename, -1, wFile, MAX_PATH );
	HRESULT hRet = CoCreateInstance(	CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, 
										IID_IGraphBuilder, (void **)&m_piGraphBuilder );

	if ( hRet == 0 ) { 
		// QueryInterface for some basic interfaces
		m_piGraphBuilder->QueryInterface( IID_IMediaControl, (void **)&m_piMediaControl );
		m_piGraphBuilder->QueryInterface( IID_IMediaEventEx, (void **)&m_piMediaEventEx );
		m_piGraphBuilder->QueryInterface( IID_IVideoWindow, (void **)&m_piVideoWindow );

		// Have the graph construct its the appropriate graph automatically
		hRet = m_piGraphBuilder->RenderFile( wFile, NULL );
		m_piVideoWindow->put_Owner( (OAHWND)hWnd );
		m_piVideoWindow->put_WindowStyle(WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);

		// Have the graph signal event via window callbacks for performance
		m_piMediaEventEx->SetNotifyWindow( (OAHWND)hWnd, WM_GRAPHNOTIFY, 0 );

		// If the user did not input a size, choose the entire client rectangle as default
		if ( ( x1 == 0 ) && ( y1 == 0 ) && ( x2 == 0 ) && ( y2 == 0 ) ) {
			RECT rect;
			GetClientRect( hWnd, &rect );
			x1 = rect.left;
			y1 = rect.top;
			x2 = rect.right;
			y2 = rect.bottom;
		}
        m_piVideoWindow->SetWindowPosition( x1, y1, x2, y2 );

		// Run the graph if RenderFile succeeded
		if ( hRet >= 0 ) {
		  m_piMediaControl->Run();
		  m_bPlaying = true;
		}
	} 
	
	CoUninitialize();
}


/**************************************************************************

Function		: Stop()

Purpose			: Stops playing a movie file

Returns			: void

**************************************************************************/
void MoviePlay::Stop()
{
	if ( m_bPlaying ) {
		if ( m_piMediaControl ) {
			m_piMediaControl->Stop();
		}

		if ( m_piVideoWindow ) {
			m_piVideoWindow->put_Visible(OAFALSE);

		}
         
		ReleaseInterfaces();
	}
	m_bPlaying = false;
}


/**************************************************************************

Function		: HandleNotifications()

Purpose			: Handles notifications from the windows procedure.  
				  Currently only looks for completion of the movie, then
				  neatly releases all of the interfaces

Returns			: void

**************************************************************************/
void MoviePlay::HandleNotifications()
{
	LONG	evCode;
	LONG	evParam1;
	LONG	evParam2;
	HRESULT	hRet;	
		
	
	while ( SUCCEEDED( m_piMediaEventEx->GetEvent(&evCode, &evParam1, &evParam2, 0 ))) { 
		hRet = m_piMediaEventEx->FreeEventParams(evCode, evParam1, evParam2);
    
		// Has the movie finished playing
		if ( (EC_COMPLETE == evCode) || (EC_USERABORT == evCode) ) { 
			m_piVideoWindow->put_Visible( OAFALSE );
			ReleaseInterfaces();
			break;
		} 
	} 	
}



/**************************************************************************

Function		: ReleaseInterfaces()

Purpose			: Releases the currently used interfaces.

Returns			: void

**************************************************************************/
void MoviePlay::ReleaseInterfaces()
{
	if ( m_piVideoWindow ) {
		m_piVideoWindow->Release();
		m_piVideoWindow = NULL;
	}

	if ( m_piGraphBuilder ) {
		m_piGraphBuilder->Release();
		m_piGraphBuilder = NULL;
	}

	if ( m_piMediaControl ) {
		m_piMediaControl->Release();
		m_piMediaControl = NULL;
	}

	if ( m_piMediaEventEx ) {
		m_piMediaEventEx->Release();
		m_piMediaEventEx = NULL;
	}

	if ( m_piFilter ) {
		m_piFilter->Release();
		m_piFilter = NULL;
	}
}
