//////////////////////////////////////////////////////////////////////////////
//
// File     :  RetailOptions.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains macros to configure the final retail Dungeon Siege.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __RETAILOPTIONS_H
#define __RETAILOPTIONS_H

//////////////////////////////////////////////////////////////////////////////
// configuration macros

// disallow all unimplemented code
#define GP_RETAIL_FULLY_IMPLEMENTED 0

// only use dr. watson tool for exception reporting
#define GP_RETAIL_USE_DRWATSON_ONLY 0

// disable retail from building at all
#define GP_RETAIL_ALLOW_BUILD 1

// disable error reporting from skrit
#define GP_RETAIL_ERROR_FREE 0

// enable the hang buddy in retail mode
#define GP_RETAIL_USE_HANG_BUDDY 1

// enable external profiler integration
#define GP_RETAIL_ENABLE_PROFILING 1

// enable direct Skrit calls via user console
#define GP_RETAIL_DIRECT_SKRIT_ENTRY 1

// enable the skritbot
#define GP_RETAIL_ENABLE_SKRITBOT 1

// enable stats gathering in general
#if SCOTT_BILAS
#define GP_RETAIL_DISABLE_STATS 0
#else // SCOTT_BILAS
#define GP_RETAIL_DISABLE_STATS 1
#endif // SCOTT_BILAS

// enable special dev features in general
#define GP_RETAIL_ENABLE_DEV_FEATURES 0

//////////////////////////////////////////////////////////////////////////////
// macro reinterpretation

#define GP_FULLY_IMPLEMENTED    (GP_RETAIL && GP_RETAIL_FULLY_IMPLEMENTED)
#define GP_USE_DRWATSON_ONLY    (GP_RETAIL && GP_RETAIL_USE_DRWATSON_ONLY)
#define GP_ALLOW_BUILD          (!GP_RETAIL || GP_RETAIL_ALLOW_BUILD)
#define GP_ERROR_FREE           (GP_RETAIL && GP_RETAIL_ERROR_FREE)
#define GP_USE_HANG_BUDDY       (!GP_RETAIL || GP_RETAIL_USE_HANG_BUDDY)
#define GP_ENABLE_PROFILING     (!GP_RETAIL || GP_RETAIL_ENABLE_PROFILING)
#define GP_DIRECT_SKRIT_ENTRY   (!GP_RETAIL || GP_RETAIL_DIRECT_SKRIT_ENTRY)
#define GP_ENABLE_SKRITBOT      (!GP_RETAIL || GP_RETAIL_ENABLE_SKRITBOT)
#define GP_ENABLE_STATS         (!GP_SIEGEMAX && ((!GP_RETAIL) || GP_PROFILING || (!GP_RETAIL_DISABLE_STATS)))
#define GP_ENABLE_DEV_FEATURES  (!GP_RETAIL || GP_RETAIL_ENABLE_DEV_FEATURES)

// bkijanka
#define GP_DISABLE_MULTIPLAYER 0

//////////////////////////////////////////////////////////////////////////////

#endif  // __RETAILOPTIONS_H

//////////////////////////////////////////////////////////////////////////////
