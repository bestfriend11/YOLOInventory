//////////////////////////////////////////////////////////////////////////////
//
// File     :  Exports.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "PrecompEditor.h"

//////////////////////////////////////////////////////////////////////////////
// Exports

/*  The purpose of this file is to force the linker to include code in the
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
#include "Weather.h"

//////////////////////////////////////////////////////////////////////////////
