//////////////////////////////////////////////////////////////////////////////
//
// File     :  TankBuilderVersion.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains version info for Tank Builder.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __TANKBUILDERVERSION_H
#define __TANKBUILDERVERSION_H

//////////////////////////////////////////////////////////////////////////////

#include "..\Tattoo\Services\WorldVersion.h"

//////////////////////////////////////////////////////////////////////////////
// Configuration constants

// $ Configure this section for the auto-gen features.

// @PROJECT				= TB1
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
#define TB1_BUILD_NUMBER 61

// build of day, incremented depending on label in P4 used to "commit" it
#define TB1_BUILD_DAY_OF_YEAR 152
#define TB1_BUILD_OF_DAY 2

// generated from previous constants
#define TB1_VERSION_INT   1,9,2,61
#define TB1_VERSION_SHORT "v1.09.2\0"
#define TB1_VERSION_MSQA  "02.06.0202\0"

// generated each time build # is updated
#define BUILD_TIMESTAMP "Sun Jun  2 16:46:07 2002 PST\0"

// special build options
#define SPECIAL_BUILD 1
#define TB1_SPECIAL_BUILD "INTERNAL\0"

#endif // RC_INVOKED

// this special constant is available to everything
#define TB1_VERSION_TEXT "1.09.2.0061 (msqa:02.06.0202)\0"

//////////////////////////////////////////////////////////////////////////////

#endif  // __TANKBUILDERVERSION_H

//////////////////////////////////////////////////////////////////////////////
