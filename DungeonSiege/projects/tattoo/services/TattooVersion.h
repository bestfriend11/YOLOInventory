//////////////////////////////////////////////////////////////////////////////
//
// File     :  TattooVersion.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains Tattoo version query stuff.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __TATTOOVERSION_H
#define __TATTOOVERSION_H

//////////////////////////////////////////////////////////////////////////////

#include "WorldVersion.h"

//////////////////////////////////////////////////////////////////////////////
// Configuration constants

// $ Configure this section for the auto-gen features.

// @PROJECT				= DS1
// @SPECIAL_BUILD_TYPE	= INTERNAL

//////////////////////////////////////////////////////////////////////////////
// Private version constants

// $ This section is auto-generated. Do not modify.

#ifdef RC_INVOKED

// these defines can only be used in the resource compiler - code is supposed to
// query directly from version resource, NOT from this .h file. that way we can
// stamp the module with a separate tool and everything still works.
//
// normal version format: A.BB.C.DDDD (major, minor, revision, build # to date)
// msqa version format  : YY.MM.DD.NN (year, month, day, build # of day)
//

// incremented each build
#define DS1_BUILD_NUMBER 1467

// build of day, incremented depending on label in P4 used to "commit" it
#define DS1_BUILD_DAY_OF_YEAR 276
#define DS1_BUILD_OF_DAY 1

// generated from previous constants
#define DS1_VERSION_INT   1,12,0,1467
#define DS1_VERSION_SHORT "v1.12\0"
#define DS1_VERSION_MSQA  "02.10.0401\0"

// generated each time build # is updated
#define BUILD_TIMESTAMP "Fri Oct  4 10:14:04 2002 PST\0"

// special build options
#define SPECIAL_BUILD 1
#define DS1_SPECIAL_BUILD "INTERNAL\0"

#endif // RC_INVOKED

// this special constant is available to everything
#define DS1_VERSION_TEXT "1.12.0.1467 (msqa:02.10.0401)\0"

//////////////////////////////////////////////////////////////////////////////
// Public constants

#define DS1_APP_NAME "Dungeon Siege"
#define DS1_INI_NAME "DungeonSiege"

//////////////////////////////////////////////////////////////////////////////
// Other helpers

#ifndef RC_INVOKED

inline bool IsDs1VersionRtm( const gpversion& newVer )
{
	// save game compatibility issues:
	//
	//   - siege transition switched to storing doubles rather than floats for
	//     1.09b and above.

	MSQAVersion rtmVer;
	rtmVer.m_VersionExternal = 0;
	rtmVer.m_VersionInternal = 5;
	rtmVer.m_Year            = 2002;
	rtmVer.m_Month           = 3;
	rtmVer.m_Day             = 8;
	rtmVer.m_BuildOfDay      = 1;

	return ( newVer == rtmVer );
}

#endif // RC_INVOKED

//////////////////////////////////////////////////////////////////////////////

#endif  // __TATTOOVERSION_H

//////////////////////////////////////////////////////////////////////////////
