/**************************************************************************

Filename		: InitWin.cpp

Description		: Initialize the main window viewport.

Creation Date	: 6/30/99

**************************************************************************/


// Include Files
#include "InitWin.h"
#include "Main.h"
#include "Shell.h"
#include "resource.h"


/**************************************************************************

Function		: InitWin()

Purpose			: Register and create the main window

Returns			: Handle to the main window

**************************************************************************/
HWND InitWindow( HINSTANCE hInstance, int iCmdShow, Shell *pShell )
{

    WNDCLASSEX  WndClass;
	HWND		hWnd;
				   	
	WndClass.cbSize			= sizeof( WNDCLASSEX );
    WndClass.style			= NULL;
    WndClass.lpfnWndProc	= WindProc;
    WndClass.cbClsExtra		= 0;
    WndClass.cbWndExtra		= sizeof( Shell* );
    WndClass.hInstance		= hInstance;
    WndClass.hIcon			= LoadIcon( hInstance, "IDI_ICON1" );
	WndClass.hCursor		= LoadCursor( NULL, IDC_ARROW );
    WndClass.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
    WndClass.lpszMenuName	= "TestApp";
    WndClass.lpszClassName	= "TestApp";
    WndClass.hIconSm		= LoadIcon( hInstance, "IDI_ICON1" );
	RegisterClassEx( &WndClass );	

	// Create window
	RECT rClient = { 0, 0, SCREENWIDTH, SCREENHEIGHT };
	AdjustWindowRect( &rClient, 0, 0 );
    hWnd = CreateWindowEx( 0, "TestApp", "Movie Player",
                           WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
						   CW_USEDEFAULT, SW_SHOW,
                           rClient.right, rClient.bottom, NULL, NULL,
                           hInstance, pShell );

	// Display window
    ShowWindow( hWnd, iCmdShow );
	UpdateWindow( hWnd );

	return hWnd; 
}