/**************************************************************************

Filename		: InitWin.h

Description		: Function prototypes to initialize the main window 
				  viewport.

Creation Date	: 4/29/99

Author			: Chad Queen

**************************************************************************/

#ifndef _INITWIN_H_
#define _INITWIN_H_

// Defines
#define SCREENWIDTH  500	// Window Width
#define SCREENHEIGHT 480	// Window Height

// Include Files
#include <windows.h>
#include "CTest.h"

// Initializes the window
HWND InitWindow( HINSTANCE hInstance, int iCmdShow, CTest *pTest );

// The Window Procedure for the main window
LRESULT CALLBACK WindProc( HWND Hwnd, UINT Msg, WPARAM  WinParam,
						   LPARAM  LongParam );

#endif