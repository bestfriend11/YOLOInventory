//////////////////////////////////////////////////////////////////////////////
//
// File     :  gpcoredefs.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains base definitions used by gpcore.h and RC files.
//
// Copyright © 2002 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GPCOREDEFS_H
#define __GPCOREDEFS_H

//////////////////////////////////////////////////////////////////////////////
// Build configuration

#if defined( GP_RETAIL ) && GP_RETAIL

#	define GP_DEBUG 0
#	define GP_RELEASE 0

#else // GP_RETAIL

#	ifdef _DEBUG
#		define GP_DEBUG   1
#		define GP_RELEASE 0
#		define GP_RETAIL  0
#	elif defined( GP_PROFILING )
#		define GP_DEBUG   0
#		define GP_RELEASE 0
#		define GP_RETAIL  1
#	else
#		define GP_DEBUG   0
#		define GP_RELEASE 1
#		define GP_RETAIL  0
#	endif

#endif // GP_RETAIL

// make sure it's always defined to something
#ifndef GP_PROFILING
#define GP_PROFILING 0
#endif

// some win32 .h defines this, we can't have that!
#ifdef DEBUG
#undef DEBUG
#endif

// need these always defined for siegemax
#if !defined( GP_SIEGEMAX )
#	if defined( SIEGEMAX )
#		define GP_SIEGEMAX 1
#	else
#		define GP_SIEGEMAX 0
#	endif
#endif

//////////////////////////////////////////////////////////////////////////////
// General macros

#if GP_DEBUG
#	if GP_PROFILING
#	define COMPILE_MODE_TEXT "Development-Debug (Profiling)\0"
#	else
#	define COMPILE_MODE_TEXT "Development-Debug\0"
#	endif
#elif GP_RELEASE
#	if GP_PROFILING
#	define COMPILE_MODE_TEXT "Development-Release (Profiling)\0"
#	else
#	define COMPILE_MODE_TEXT "Development-Release\0"
#	endif
#elif GP_RETAIL
#	if GP_PROFILING
#	define COMPILE_MODE_TEXT "Retail (Profiling)\0"
#	else
#	define COMPILE_MODE_TEXT "Retail\0"
#	endif
#endif

#define COMPANY_NAME   "Gas Powered Games\0"
#define COPYRIGHT_TEXT "Copyright (C) 1998-2002 Gas Powered Games. All rights reserved.\0"

//////////////////////////////////////////////////////////////////////////////

#endif  // __GPCOREDEFS_H

//////////////////////////////////////////////////////////////////////////////
