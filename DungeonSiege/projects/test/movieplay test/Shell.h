/**************************************************************************

Filename		: Shell.h

Description		: Function Prototypes/Includes for Test Application

Creation Date	: 6/30/99

**************************************************************************/


#pragma once 
#ifndef _SHELL_H_
#define _SHELL_H_

// Includes
#include <windows.h>
#include "MoviePlay.h"


// Class Definitions
class Shell
{
public:
	// Public Methods

	// Constructor
	Shell();

	// Destructor
	~Shell();

	// Draws the current scene
	void Draw();

	// Set the current HWNDs
	void SetHWnd( HWND hWnd );

	// Set the current HINSTANCE
	void SetHInstance( HINSTANCE hInstance );

	// Initialize member objects
	void Init();

	
	// Public Member Variables
	MoviePlay			*m_MoviePlay;			 // Movie Player object

private:
	// Private Methods


	// Private Variables
	HWND				m_hWnd;					 // Current Window Handle
	HINSTANCE			m_hInstance;			 // Current Window Instance	
};

#endif