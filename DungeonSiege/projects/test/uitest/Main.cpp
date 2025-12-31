/**************************************************************************

Filename		: Main.cpp

Description		: Entry point for editor

Creation Date	: 6/30/99

**************************************************************************/


#include "Precomp_UITest.h"

#include "Fuel.h"
#include "Shell.h"
#include "UI_Shell.h"


/**************************************************************************

Function		: WinMain()

Purpose			: Create the main game object and begin execution.

Returns			: Integer value of wParam in received message.

**************************************************************************/
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, PSTR /*szCmdLine*/, int iCmdShow )
{
	// setup Shell
	Shell shell( hInstance );
	if ( !shell.Init( iCmdShow ) )
	{
		return ( -1 );
	}

	// set up dialog
	FuelHandle dialog( "UI:interfaces:frontend:map_loader" );
	g_UIShell.ActivateInterface( dialog );

	// exec
	int rc = shell.Main();

	// done
	return ( rc );
}
