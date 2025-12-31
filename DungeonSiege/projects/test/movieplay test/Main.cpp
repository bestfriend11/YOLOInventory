/**************************************************************************

Filename		: Main.cpp

Description		: Entry point for editor

Creation Date	: 6/30/99

**************************************************************************/


// Include Files
#include <windows.h>
#include "Main.h"
#include "InitWin.h"
#include "Shell.h"



/**************************************************************************

Function		: WinMain()

Purpose			: Create the main game object and begin execution.

Returns			: Integer value of wParam in received message.

**************************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    HWND			hWnd;
    MSG             msg;
	Shell			*pShell = NULL;	

	// Create a new test shell object	
	pShell = new Shell();

	// Get the handle to the window
	hWnd	= InitWindow( hInstance, iCmdShow, pShell );
	
	// Give the game the window handles
	pShell->SetHWnd( hWnd );
	pShell->SetHInstance( hInstance );

	// Initialize Main Tester Data ( Renderer, objects, etc. )
	pShell->Init();	
	
	// Draw 
	pShell->Draw();		
	
	// Main Message Loop
    while ( TRUE ) {
		if ( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ){
			if ( !GetMessage( &msg, NULL, 0, 0 ) )
				return msg.wParam;			   	
			TranslateMessage( &msg ); 
			DispatchMessage ( &msg ); 			
		}
		else {            					
		}
    }
    return msg.wParam;
}


/**************************************************************************

Function		: WindProc()

Purpose			: Handles all window messages.

Returns			: LRESULT CALLBACK received from DefWindowProc();

**************************************************************************/
LRESULT CALLBACK WindProc( HWND hWnd, UINT Msg, WPARAM wParam,
						   LPARAM  lParam )
{
    // Pointer to the Shell object
	Shell	*pShell;
    pShell	= (Shell*)GetWindowLong( hWnd, GWL_USERDATA );
	static	HINSTANCE		hInstance;
						
	switch( Msg ) {
    case    WM_CREATE:
        // This gets us our pTest pointer
		SetWindowLong( hWnd, GWL_USERDATA, 
					   (LONG)((CREATESTRUCT *) lParam )->lpCreateParams );
		hInstance = (HINSTANCE)((CREATESTRUCT *) lParam )->hInstance;
		break;	
	case	WM_GRAPHNOTIFY:
		pShell->m_MoviePlay->HandleNotifications();
		break;
	case	WM_COMMAND:		
		break;
	case	WM_KEYDOWN:		
		break;
	case    WM_DESTROY:
		// Close Window
		if ( pShell ) {
			pShell->m_MoviePlay->Stop();
			delete pShell;
		}
		PostQuitMessage( 0 );
		break;	
	}
	return DefWindowProc( hWnd, Msg, wParam, lParam );
}