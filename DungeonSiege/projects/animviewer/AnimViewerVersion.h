//////////////////////////////////////////////////////////////////////////////
//
// File     :  AnimViewerVersion.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains AnimViewer version query stuff.
//
// Copyright © 2002 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __ANIMVIEWERVERSION_H
#define __ANIMVIEWERVERSION_H

//////////////////////////////////////////////////////////////////////////////

#include "..\tattoo\services\WorldVersion.h"

//////////////////////////////////////////////////////////////////////////////
// Configuration constants

// $ Configure this section for the auto-gen features.

// @PROJECT = ANIMV

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
#define ANIMV_BUILD_NUMBER 5

// build of day, incremented depending on label in P4 used to "commit" it
#define ANIMV_BUILD_DAY_OF_YEAR 219
#define ANIMV_BUILD_OF_DAY 1

// generated from previous constants
#define ANIMV_VERSION_INT   1,10,0,5
#define ANIMV_VERSION_SHORT "v1.1\0"
#define ANIMV_VERSION_MSQA  "02.08.0801\0"

// generated each time build # is updated
#define BUILD_TIMESTAMP "Thu Aug  8 11:28:29 2002 PST\0"

// special build options
#define SPECIAL_BUILD 1
#define ANIMV_SPECIAL_BUILD "INTERNAL\0"

#endif // RC_INVOKED

// this special constant is available to everything
#define ANIMV_VERSION_TEXT "1.10.0.0005 (msqa:02.08.0801)\0"

//////////////////////////////////////////////////////////////////////////////
// Public constants

#define ANIMV_APP_NAME "DSAnimViewer"
#define ANIMV_INI_NAME "DSAnimViewer"

//////////////////////////////////////////////////////////////////////////////

#endif  // __ANIMVIEWERVERSION_H

//////////////////////////////////////////////////////////////////////////////
