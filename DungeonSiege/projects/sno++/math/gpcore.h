//////////////////////////////////////////////////////////////////////////////
//
// File     :  gpcore.h (GPCore:FileSys)
// Author(s):  <Original Author Unknown>, Scott Bilas
//
// Summary  :  This is the standard include file for all GPG projects.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GPCORE_H
#define __GPCORE_H

//////////////////////////////////////////////////////////////////////////////
// windows.h

#pragma warning ( push, 1 )		// ignore any warnings or changes that win32 has
	#include <windows.h>
#pragma warning ( pop )			// back to normal

// don't care about these warnings
#include "IgnoredWarnings.h"

//////////////////////////////////////////////////////////////////////////////
// Build configuration

#if defined( GP_RETAIL ) && GP_RETAIL

#	define GP_DEBUG 0
#	define GP_RELEASE 0

#else // GP_RETAIL

#	ifdef _DEBUG
#	define GP_DEBUG   1
#	define GP_RELEASE 0
#	else
#	define GP_DEBUG   0
#	define GP_RELEASE 1
#	endif

#	ifndef GP_RETAIL
#	define GP_RETAIL 0
#	endif

#endif // GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// Configuration/version macros

#if GP_DEBUG
	#define COMPILE_MODE_TEXT "Development-Debug"
#elif GP_RELEASE
	#define COMPILE_MODE_TEXT "Development-Release"
#elif GP_RETAIL
	#define COMPILE_MODE_TEXT "Production"
#endif

#define MAKEVERSION( major, minor ) MAKELONG( minor, major )

#define GETVERSIONMAJOR HIWORD
#define GETVERSIONMINOR LOWORD

#define REVERSE_FOURCC( x ) (  (((x) & 0x000000FF) << 24) \
							 | (((x) & 0x0000FF00) <<  8) \
							 | (((x) & 0x00FF0000) >>  8) \
							 | (((x) & 0xFF000000) >> 24) )

//////////////////////////////////////////////////////////////////////////////
// C++ specific core includes

#ifdef __cplusplus

//#	include "gpmem.h"
//#	include "gptypes.h"
//#	include "gpassert.h"

#define gpassert(x)
#define gpassertm(x,y)

//#	include "gpglobal.h"
//#	include "gpdebugoutput.h"
#include "commdlg.h"

#endif

//////////////////////////////////////////////////////////////////////////////

#endif // __GPCORE_H

//////////////////////////////////////////////////////////////////////////////
