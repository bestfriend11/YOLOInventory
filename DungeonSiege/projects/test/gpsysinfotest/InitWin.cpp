/**************************************************************************

Filename		: InitWin.cpp

Description		: Initialize the main window viewport.

Creation Date	: 4/27/99

Author			: Chad Queen

**************************************************************************/


// Include Files
#include "InitWin.h"
#include "Main.h"
#include "CTest.h"


/**************************************************************************

Function		: InitWin()

Purpose			: Register and create the main window

Returns			: Handle to the main window

Author			: CQ

**************************************************************************/
HWND InitWindow( HINSTANCE hInstance, int iCmdShow, CTest *pTest )
{

    WNDCLASSEX  WndClass;
	HWND		hWnd;
				   	
	WndClass.cbSize			= sizeof( WNDCLASSEX );
    WndClass.style			= NULL;
    WndClass.lpfnWndProc	= WindProc;
    WndClass.cbClsExtra		= 0;
    WndClass.cbWndExtra		= sizeof( CTest* );
    WndClass.hInstance		= hInstance;
    WndClass.hIcon			= LoadIcon( hInstance, MAKEINTRESOURCE(101) );
	WndClass.hCursor		= LoadCursor( NULL, IDC_ARROW );
	
	// Get Brush - If any
    WndClass.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );

	// Assign strings
    WndClass.lpszMenuName  = "TestProgram";
    WndClass.lpszClassName = "TestProgram";

    WndClass.hIconSm       = NULL;

	RegisterClassEx( &WndClass );

	// Create window
    hWnd = CreateWindowEx( 0, "TestProgram", "Test Application",
                           WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
						   CW_USEDEFAULT, SW_SHOW,
                           SCREENWIDTH, SCREENHEIGHT, NULL, LoadMenu( hInstance, MAKEINTRESOURCE(102)),
                           hInstance, NULL );

	// Display window
    ShowWindow( hWnd, iCmdShow );
	UpdateWindow( hWnd );

	return hWnd; 
}