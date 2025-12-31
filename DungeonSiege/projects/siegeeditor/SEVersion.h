//////////////////////////////////////////////////////////////////////////////
//
// File     :  SEVersion.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains SE version query stuff.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __SEVERSION_H
#define __SEVERSION_H

//////////////////////////////////////////////////////////////////////////////

#include "..\tattoo\services\WorldVersion.h"

//////////////////////////////////////////////////////////////////////////////
// Configuration constants

// $ Configure this section for the auto-gen features.

// @PROJECT = SE1

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
#define SE1_BUILD_NUMBER 343

// build of day, incremented depending on label in P4 used to "commit" it
#define SE1_BUILD_DAY_OF_YEAR 276
#define SE1_BUILD_OF_DAY 1

// generated from previous constants
#define SE1_VERSION_INT   1,12,0,343
#define SE1_VERSION_SHORT "v1.12\0"
#define SE1_VERSION_MSQA  "02.10.0401\0"

// generated each time build # is updated
#define BUILD_TIMESTAMP "Fri Oct  4 10:14:01 2002 PST\0"

// special build options
#define SPECIAL_BUILD 1
#define SE1_SPECIAL_BUILD "INTERNAL\0"

#endif // RC_INVOKED

// this special constant is available to everything
#define SE1_VERSION_TEXT "1.12.0.0343 (msqa:02.10.0401)\0"

//////////////////////////////////////////////////////////////////////////////
// Public constants

#define SE1_APP_NAME "Siege Editor"
#define SE1_INI_NAME "SiegeEditor"

//////////////////////////////////////////////////////////////////////////////

#endif  // __SEVERSION_H

//////////////////////////////////////////////////////////////////////////////
