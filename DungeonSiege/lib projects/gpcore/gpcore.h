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

// clear these
#ifdef WINVER
#undef WINVER
#endif
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

// we're forcing windows sdk v5 headers
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500

#pragma warning ( push, 1 )		// ignore any warnings or changes that win32 has
#ifndef _INC_WINDOWS
	// tune windows.h
#	define WIN32_LEAN_AND_MEAN
#	define WIN32_EXTRA_LEAN
#	define STRICT
	// don't include this stuff (some of these are from AFXV_W32.H and may be deprecated - marked with '?')
#	define NOCRYPT				// - Windows crypto API
#	define NOSERVICE			// - All Service Controller routines, SERVICE_ equates, etc.
#	define NOMCX				// - Modem Configuration Extensions
#	define NOIME				// <imm.h>
#	define NOSOUND				// - Sound driver routines
#	define NOCOMM				// - COMM driver routines
#	define NOKANJI				// - Kanji support stuff.
#	define NORPC				// ?
#	define NOPROXYSTUB			// ?
#	define NOIMAGE				// ?
#	define NOTAPE				// ?
#	define NOMETAFILE			// - typedef METAFILEPICT
#	define NOMINMAX				// - Macros min(a,b) and max(a,b)
	// ok we're ready, finally include The Devil
#	include <windows.h>
#endif  // _INC_WINDOWS
#pragma warning ( pop )			// back to normal

// now UNDEFINE all those stupid Windows hacks (from winbase.h). add to this
// list as you see fit
#undef DefineHandleTable
#undef LimitEmsPages
#undef SetSwapAreaSize
#undef LockSegment
#undef UnlockSegment
#undef GetCurrentTime
#undef Yield
#undef GetMessage

// UNDEFINE these terrible macros lest anyone be tempted to use their vileness
#undef __min
#undef __max

// this is a helper to reconstruct unicode/ansi function names
#ifdef _UNICODE
#define MAKE_UNICODE_FNAME( x ) x##W
#else
#define MAKE_UNICODE_FNAME( x ) x##A
#endif

// but some code needs MessageBox 'defined' - so do this instead (but not for
// apps that have already redef'd messagebox
#ifndef __AFX_H__
#undef MessageBox
inline int MessageBox( HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType )
	{  return ( ::MAKE_UNICODE_FNAME( MessageBox )( hWnd, lpText, lpCaption, uType ) );  }
#endif

//////////////////////////////////////////////////////////////////////////////
// Special includes

// basic definitions
#include "GpCoreDefs.h"

// don't care about these warnings
#include "IgnoredWarnings.h"

// tune our options per user
#include "Local\UserCore.h"

// tune the builds based on retail options
#include "RetailOptions.h"

// special warning for building retail by accident
#if !GP_ALLOW_BUILD
#error "We can't build retail yet!"
#endif

//////////////////////////////////////////////////////////////////////////////
// Types

// Ordinary types.

	typedef unsigned __int64  UINT64;   // unsigned 64-bit int
	typedef   signed __int64   INT64;   //   signed 64-bit int
	typedef unsigned int      UINT32;   // unsigned 32-bit int
	typedef   signed int       INT32;   //   signed 32-bit int
	typedef unsigned short    UINT16;   // unsigned 16-bit int
	typedef   signed short     INT16;   //   signed 16-bit int
	typedef unsigned char      UINT8;   // unsigned  8-bit int
	typedef   signed char       INT8;   //   signed  8-bit int

	typedef unsigned __int64   QWORD;   // quad-word

	typedef DWORD             FOURCC;   // four character code

// Forward declarations.

	namespace std
	{
		template <class CHAR> struct char_traits;
		template <class T> class allocator;
	}

	template <typename CHAR, typename TRAITS = std::char_traits <CHAR>, typename ALLOC = std::allocator <CHAR> >
	class gpbstring;

	typedef gpbstring <char> gpstring;
	typedef gpbstring <wchar_t> gpwstring;

	template <typename ALLOC = std::allocator <char> >
	class gpdumbstring_t;

	typedef gpdumbstring_t <> gpdumbstring;

//////////////////////////////////////////////////////////////////////////////
// General macros

#if GP_DEBUG
#	define USE_SLOW_MATH
#else
#	define USE_INTRINSICS	// Define to allow the compiler to use intrinsic math functions
#endif

#define MAKEVERSION( major, minor, sub ) MAKELONG( MAKEWORD( sub, minor ), major )

#define GETVERSIONMAJOR HIWORD
#define GETVERSIONMINOR LOWORD

#define REVERSE_FOURCC( x ) (  (((x) & 0x000000FF) << 24) \
							 | (((x) & 0x0000FF00) <<  8) \
							 | (((x) & 0x00FF0000) >>  8) \
							 | (((x) & 0xFF000000) >> 24) )

//////////////////////////////////////////////////////////////////////////////
// C++ specific special macros

#ifdef __cplusplus

// this will allow you to define functionality that can be in an H file
// yet still act as if it was stored in the CPP file. in other words, if you
// need a statically allocated object, but don't want to put it in a CPP
// file (to keep it near the class it modifies, for instance) then create a
// class and do the work in its ctor, then use this macro on it.
#define GLOBALSTATIC( cls ) \
	template struct ::GlobalStatic <cls>
#define GLOBALSTATIC_BEGINX( unique, cls ) \
	struct GlobalStatic_##unique##_##cls  {
#define GLOBALSTATIC_ENDX( unique, cls ) \
	};  GLOBALSTATIC( GlobalStatic_##unique##_##cls )

// these make GLOBALSTATIC even easier to use - just put the static code
// between the begin and end tags...
#define GLOBALSTATIC_BEGIN( unique, cls ) \
	GLOBALSTATIC_BEGINX( unique, cls ) \
	GlobalStatic_##unique##_##cls( void )  {
#define GLOBALSTATIC_END( unique, cls ) \
	; }  GLOBALSTATIC_ENDX( unique, cls )

// use these to force symbols into the linker, but not actually call them.
#define GLOBALSTATIC_BEGIN_FORCELINK( unique, cls ) \
	GLOBALSTATIC_BEGINX( unique, cls ) \
	void ForceLink( void )  {
#define GLOBALSTATIC_END_FORCELINK( unique, cls ) \
	; }  GLOBALSTATIC_ENDX( unique, cls )

// here is the helper class
template <typename T>
struct GlobalStatic
{
	T m_Instance;
	static GlobalStatic <T> ms_Instance;
};
template <typename T> GlobalStatic <T> GlobalStatic <T>::ms_Instance;

// delete and clear the pointer
template <typename T> inline void Delete( T*& p )
	{  delete ( p );  p = 0;  }

// delete and clear the array object
template <typename T> inline void DeleteArray( T*& p )
	{  delete [] ( p );  p = 0;  }

#endif // __cplusplus

//////////////////////////////////////////////////////////////////////////////
// C++ specific core includes

#ifdef __cplusplus

#	include "GpMem.h"
#	include "GpAssert.h"
#	include "GpGlobal.h"
#	include "GpReport.h"
#	include "GpString.h"

#endif // __cplusplus

//////////////////////////////////////////////////////////////////////////////

#endif // __GPCORE_H

//////////////////////////////////////////////////////////////////////////////
