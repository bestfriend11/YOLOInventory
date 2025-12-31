/**************************************************************************

Filename		: Main.cpp

Description		: Entry point for the COGL Engine test program

Creation Date	: 4/29/99

Author			: Chad Queen

**************************************************************************/


// Include Files
#include "Main.h"
#include "InitWin.h"
#include "CTest.h"
#include "ExceptionHandler.h"



/**************************************************************************

Function		: WinMain()

Purpose			: WinMain wrapped with structured exception handler

Returns			: Integer value of wParam in received message.

Author			: CQ

**************************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	int Result = -1;
	__try
	{
		Result = HandledWinMain( hInstance, hPrevInstance, szCmdLine, iCmdShow );
	}
	__except( RecordExceptionInfo(GetExceptionInformation(), "main thread" ) )
	{
		// Do nothing here.  RecordExceptionInfo does all the work we need.
	}
	return Result;    
}


/**************************************************************************

Function		: HandledWinMain()

Purpose			: Test application entry point.

Returns			: Integer value of wParam in received message.

Author			: CQ

**************************************************************************/
int WINAPI HandledWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow )
{
	HWND			hWnd;
    MSG             msg;
	CTest			*pTest = NULL;
		
	// Create a new test shell object	
	pTest = new CTest();

	// Get the handle to the window
	hWnd	= InitWindow( hInstance, iCmdShow, pTest );
	
	// Give the game the window handles
	pTest->SetHWnd( hWnd );
	pTest->SetHInstance( hInstance );

	// Initialize Main Tester Data ( Renderer, objects, etc. )
	pTest->Init();	
	
	// Main Message Loop
    while ( TRUE ) {
		if ( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ){
			if ( !GetMessage( &msg, NULL, 0, 0 ) )
				return msg.wParam;
	   	
			TranslateMessage( &msg ); 
			DispatchMessage ( &msg ); 
			
		}
		else {
            pTest->Init();
		}
    }
    return msg.wParam;
}


/**************************************************************************

Function		: WindProc()

Purpose			: Handles all window messages.

Returns			: LRESULT CALLBACK received from DefWindowProc();

Author			: CQ

**************************************************************************/
LRESULT CALLBACK WindProc( HWND hWnd, UINT Msg, WPARAM wParam,
						   LPARAM  lParam )
{
    // Pointer to the Test object
	CTest	*pTest;
    pTest	= (CTest*)GetWindowLong( hWnd, GWL_USERDATA );
	static	HINSTANCE hInstance;
	static	HGLRC	hRC;
	static	HDC		hDC;
	
    switch( Msg ) {
    case    WM_CREATE:
        // This gets us our pTest pointer
		SetWindowLong( hWnd, GWL_USERDATA, 
					   (LONG)((CREATESTRUCT *) lParam )->lpCreateParams );
		hInstance = (HINSTANCE)((CREATESTRUCT *) lParam )->hInstance;
		break;
	case	WM_MOVE:
		break;
	case	WM_PAINT:		
		break;
	case	WM_COMMAND:		
		break;
	case	WM_SYSKEYDOWN:
		// Toggle between Fullscreen and Windowed mode
		//if ( WParam == VK_RETURN )				
		break;
	case    WM_DESTROY:
			// Close Window
		if ( pTest )
				delete pTest;
		PostQuitMessage( 0 );
		break;
	}
	return DefWindowProc( hWnd, Msg, wParam, lParam );
}