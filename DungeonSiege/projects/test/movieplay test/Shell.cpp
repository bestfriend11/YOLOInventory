/**************************************************************************

Filename		: Shell.cpp

Description		: Main Test Application Shell

Creation Date	: 6/30/99

**************************************************************************/


// Includes
#include "Shell.h"
#include "InitWin.h"


/**************************************************************************

Function		: Shell()

Purpose			: Constructor 

Returns			: No return type

**************************************************************************/
Shell::Shell()
{	
	m_hWnd					= NULL;
	m_MoviePlay				= NULL;
}


/**************************************************************************

Function		: ~Shell()

Purpose			: Destructor

Returns			: No return type

**************************************************************************/
Shell::~Shell()
{	
	if ( m_MoviePlay )
		delete m_MoviePlay;
}


/**************************************************************************

Function		: Draw()

Purpose			: Draws the current scene

Returns			: void

**************************************************************************/
void Shell::Draw()
{		
	m_MoviePlay->Play( "mslogo.avi", m_hWnd );
}


/**************************************************************************

Function		: Init()

Purpose			: Initializes member objects

Returns			: Void

**************************************************************************/
void Shell::Init()
{	
	m_MoviePlay = new MoviePlay();
}


/**************************************************************************

Function		: SetHWnd()

Purpose			: Sets the current window handle

Returns			: void

**************************************************************************/
void Shell::SetHWnd( HWND hWnd )
{
	m_hWnd = hWnd;
}


/**************************************************************************

Function		: SetHInstance()

Purpose			: Sets the current window instance

Returns			: void

**************************************************************************/
void Shell::SetHInstance( HINSTANCE hInstance )
{
	m_hInstance = hInstance;
}

