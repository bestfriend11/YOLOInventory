//////////////////////////////////////////////////////////////////////////////
//
// File     :  Main.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_AnimViewer.h"

#include "GpMath.h"
#include "viewer.h"
#include "Shellapi.h"

//////////////////////////////////////////////////////////////////////////////
// WinMain()

// this is the entry point for the game
int APIENTRY WinMain( HINSTANCE hinstance, HINSTANCE, LPSTR, int )
{
	// create the app and go!

	return ( Viewer().AutoRun( hinstance ) );
}

//////////////////////////////////////////////////////////////////////////////
