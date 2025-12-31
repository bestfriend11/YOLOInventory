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

#include "Precomp_Exe.h"

#include "GpMath.h"
#include "TattooGame.h"
#include "TattooVersion.h"

//////////////////////////////////////////////////////////////////////////////
// Setup configuration and stamp the EXE

#define COMMENT_TEXT \
    " -- " DS1_PRODUCT_NAME " v" DS1_VERSION_TEXT \
    " * Configured as " COMPILE_MODE_TEXT \
    " * Compiled on " __DATE__ " at " __TIME__ " --" \
    " * " COPYRIGHT_TEXT
#pragma comment ( user  , COMMENT_TEXT )
#pragma comment ( exestr, COMMENT_TEXT )

#define THANKS_TEXT \
    "\r\n\r\n " \
    "-- A special Gas Powered thanks to:\r\n" \
    "   * Jean-loup Gailly and Mark Adler for zlib, a free data compression " \
         "library\r\n" \
    "--\r\n"

// add this back in when we are actually using PNG
//
//    "   * Glenn Randers-Pehrson, Andreas Eric Dilger, and Guy Eric Schalnat " \
//         "for libpng, a free implementation of the PNG graphics standard\r\n" \
//

#pragma comment ( user, THANKS_TEXT )
#pragma comment ( exestr, THANKS_TEXT )

//////////////////////////////////////////////////////////////////////////////
// Exports

/*  The purpose of this section is to force the linker to include code in the
	output EXE. This is necessary for modules that have been written
	*exclusively* for Skrit access. These functions will be "linked to"
	indirectly through the EXE's export table. And they won't even appear there
	unless *somebody* in the EXE actually references at least one symbol from
	the CPP file where the function is defined.

	So: this file just #includes files, and each class that wants to be force-
	included must use the FUBI_FORCE_LINK_CLASS() macro, which will cause this
	file (Exports.cpp) to link it into the main.
*/

#include "SkritSupport.h"

//////////////////////////////////////////////////////////////////////////////
// WinMain()

// override compiler's FTOL
DEFINE_FAST_FTOL();

// this is the entry point for the game
int APIENTRY WinMain( HINSTANCE hinstance, HINSTANCE, LPSTR, int )
{
	// create the app and go!
	return ( TattooGame().AutoRun( hinstance ) );
}

//////////////////////////////////////////////////////////////////////////////
