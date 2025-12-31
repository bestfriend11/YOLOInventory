/**************************************************************************

Filename		: Main.h

Description		: Function prototypes/defines for the entry point file

Creation Date	: 6/30/99

Author			: Chad Queen

**************************************************************************/

#pragma once 
#ifndef _MAIN_H_
#define _MAIN_H_

// Include Files
#include <wtypes.h>

// For file initialization
OPENFILENAME FileInitialize( HWND hWnd, HINSTANCE hInstance );

// For the file save dialog box
BOOL FileSaveDlg( HWND hWnd, PSTR pstrFilename, PSTR pstrTitlename,
				  OPENFILENAME *ofn );

#endif