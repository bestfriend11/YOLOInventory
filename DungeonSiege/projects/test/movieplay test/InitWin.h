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
#define SCREENWIDTH  640	// Window Width
#define SCREENHEIGHT 480	// Window Height

// Include Files
#include <windows.h>
#include "Shell.h"

// Initializes the window
HWND InitWindow( HINSTANCE hInstance, int iCmdShow, Shell *pShell );

// The Window Procedure for the main window
LRESULT CALLBACK WindProc( HWND Hwnd, UINT Msg, WPARAM  WinParam,
						   LPARAM  LongParam );


#endif