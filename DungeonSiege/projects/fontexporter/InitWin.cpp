/**************************************************************************

Filename		: InitWin.cpp

Description		: Initialize the main window viewport.

Creation Date	: 6/30/99

**************************************************************************/


// Include Files
#include "InitWin.h"
#include "Main.h"
#include "resource.h"
#include "FontExporter.h"


/**************************************************************************

Function		: InitWin()

Purpose			: Register and create the main window

Returns			: Handle to the main window

**************************************************************************/
HWND InitWindow( HINSTANCE hInstance, int iCmdShow, FontExporter *pExporter )
{

    WNDCLASSEX  WndClass;
	HWND		hWnd;
				   	
	WndClass.cbSize			= sizeof( WNDCLASSEX );
    WndClass.style			= NULL;
    WndClass.lpfnWndProc	= WindProc;
    WndClass.cbClsExtra		= 0;
    WndClass.cbWndExtra		= sizeof( FontExporter *);
    WndClass.hInstance		= hInstance;
    WndClass.hIcon			= LoadIcon( hInstance, MAKEINTRESOURCE(IDI_ICON1) );
	WndClass.hCursor		= LoadCursor( NULL, IDC_ARROW );
    WndClass.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
    WndClass.lpszMenuName	= "TestApp";
    WndClass.lpszClassName	= "TestApp";
    WndClass.hIconSm		= LoadIcon( hInstance, MAKEINTRESOURCE(IDI_ICON1) );
	RegisterClassEx( &WndClass );	

	// Create window
	RECT rClient = { 0, 0, SCREENWIDTH, SCREENHEIGHT };
	AdjustWindowRect( &rClient, 0, 0 );
    hWnd = CreateWindowEx(	0, "TestApp", "Gas Powered Font Exporter",
							WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE, 
							CW_USEDEFAULT, SW_SHOW,
							rClient.right+6, rClient.bottom+25, NULL, NULL,
							hInstance, pExporter );

	// Display window    
	UpdateWindow( hWnd );

	return hWnd; 
}