/**************************************************************************

Filename		: InitWin.h

Description		: Function prototypes to initialize the main window 
				  viewport.

Creation Date	: 6/30/99

**************************************************************************/

#pragma once 
#ifndef _INITWIN_H_
#define _INITWIN_H_

// Defines
#define SCREENWIDTH  256	// Window Width
#define SCREENHEIGHT 256	// Window Height

// Include Files
#include <windows.h>

// Forward Declarations
class FontExporter;


// Initializes the window
HWND InitWindow( HINSTANCE hInstance, int iCmdShow, FontExporter *pExporter );

// The Window Procedure for the main window
LRESULT CALLBACK WindProc( HWND Hwnd, UINT Msg, WPARAM  WinParam,
						   LPARAM  LongParam );

BOOL CALLBACK ExporterProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );


#endif