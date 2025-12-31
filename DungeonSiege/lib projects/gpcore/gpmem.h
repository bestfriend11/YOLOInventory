//////////////////////////////////////////////////////////////////////////////
//
// File     :  gpmem.h
// Author(s):  Bartosz Kijanka, Scott Bilas
//
// Summary  :  Contains memory management functions.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GPMEM_H
#define __GPMEM_H

//////////////////////////////////////////////////////////////////////////////

#include "GpCore.h"

//////////////////////////////////////////////////////////////////////////////
// Documentation

/*
	This file is always in use - it's #included by gpcore.h.

	Use SmartHeap in non-debug builds. SmartHeap's debug runtime is damn cool
	but the stupid thing throws all these ridiculous access violations while it
	patches DLL's, which actually gets in the way of debugging. So for debug
	builds use the CRT's memory manager, which is actually really good, though a
	little slow, which is why we're sticking with SmartHeap in non-debug
	builds.
*/

//////////////////////////////////////////////////////////////////////////////

#define GPMEM_DBG_NEW (GP_DEBUG && !NO_GPMEM_DBG_NEW && !GP_SIEGEMAX)

#define PROTECTED_BUFFERS 0

//////////////////////////////////////////////////////////////////////////////
// Debug reporting/tuning

#include <stringfwd>

namespace gpmem  {  // begin of namespace gpmem

// Process memory query.

	// this will append to 'status' a memory system summary. set 'getSysMem' to
	// true to ask NT about the process.

	void GetStatusSummary( gpstring& status );

// virtualalloc redirectors

BYTE *	vaAlloc( DWORD size );
void	vaFree( BYTE * ptr, DWORD size );
void	vaProtect( BYTE * ptr, DWORD size );
void	vaUnprotect( BYTE * ptr, DWORD size );

#if GPMEM_DBG_NEW

// Memory system accumulator query.

	int   GetNumAllocations   ( void );
	int   GetSizeAllocations  ( void );
	int   GetSizeHighWaterMark( void );
	float GetMemWasteRatio    ( void );
	int   GetLastSerialID     ( void );
	int   GetDeltaSerialID    ( bool reset = true );
	int   GetSpikeSerialID    ( int holdMsec, int delta, bool reset = false );

// Allocations database query.

	// determines sort types for queries
	enum eSortType
	{
		SORT_BYSIZE,
		SORT_BYCOUNT,
		SORT_BYWASTESIZE,
		SORT_BYWASTERATIO,
	};

	// each allocation sample consists of:
	//
	//   location: the __FILE__ and __LINE__ of the allocation
	//   size    : total memory allocated at location in bytes
	//   count   : how many allocations have taken place at location
	//
	// these variables are internal, and the output string should be intuitive
	// and easy to read.
	//
	// function params:
	//
	//   status     : where the status string will be appended
	//   limitoutput: limit the output to first n entries of sorted list
	//   sort       : how to sort the output rows

	void GetAllocationsSummary( gpstring& status, int limitoutput, eSortType sort );

// Memory management safety.

	// set normal memory manager checks (the default). in this mode the memory
	// manager will catch a number of common problems like double frees,
	// off-the-end errors, etc.

	void SetDebugNormal( void );

	// warp .0001, scotty - maximum error checking. this will perform full
	// corruption check of the memory pool with *every* alloc/free. very slow
	// but it will occasionally save your ass.
	//
	// if you have a good idea where the problem is, you might want to try to
	// selectively call SetDebugMax() before and SetDebugNormal() after the
	// problem area.

	void SetDebugMax( void );

	// perform a deep and expensive check of the memory pool. this will walk
	// memory pool and check for bad pts, memory overwrites etc. this is the
	// same function that the heap manager will call every alloc/free if memory
	// debugging set to max (as above).
	//
	// it's a good idea to make a call to this function after initialization of
	// structures or loading of complicated data. in other words, make a call to
	// this after doing things which make you really nervous when you're
	// one-time initializing stuff, but don't call this in the main game code
	// unless you're specifically bug-hunting.

	void CheckAll( void );

// Explicit tracking of non-new'd resources.

	void TrackDebugAlloc( int size, bool newAlloc );
	void TrackDebugFree ( int size, bool lastFree );

}  // end of namespace gpmem

#else // GPMEM_DBG_NEW

	inline void TrackDebugAlloc( int, bool )  {  }
	inline void TrackDebugFree ( int, bool )  {  }

}  // end of namespace gpmem

#endif // GPMEM_DBG_NEW

//////////////////////////////////////////////////////////////////////////////
// Memory alloc redirection

#include "gpmem_new_off.h"

// first include these so we get malloc/free out of the way, which we'll be
// redirecting via #define to our own functions
#include <cstdlib>
#include <malloc.h>

// special hack to avoid the operator new overloads, yet still get crt dbg fun
#if GP_DEBUG
#define _MFC_OVERRIDES_NEW
#include <crtdbg.h>
#undef _MFC_OVERRIDES_NEW
#endif // GP_DEBUG

#if GPMEM_DBG_NEW

void* gpmalloc ( size_t bytes, const char* file, int line );
void* gprealloc( void* mem, size_t bytes, const char* file, int line );
void  gpfree   ( void* mem );

inline void* gpmalloc ( size_t bytes )				{  return ( gpmalloc( bytes, NULL, 0 ) );  }
inline void* gprealloc( void* mem, size_t bytes )	{  return ( gprealloc( mem, bytes, NULL, 0 ) );  }

inline void* operator new( size_t bytes, const char* file, int line )
	{  return ( gpmalloc( bytes, file, line ) );  }
inline void operator delete( void* mem, const char* /*file*/, int /*line*/ )
	{  gpfree( mem );  }

inline void* operator new( size_t bytes )
	{  return ( gpmalloc( bytes ) );  }
inline void operator delete( void* mem )
	{  gpfree( mem );  }

#elif GP_DEBUG

void* gprealloc( void* mem, size_t bytes );

inline void* gpmalloc ( size_t bytes )				{  return ( ::malloc( bytes ) );  }
inline void  gpfree   ( void* mem )					{  ::free( mem );  }

#elif GP_ENABLE_STATS

void* gpmalloc ( size_t bytes );
void* gprealloc( void* mem, size_t bytes );
void  gpfree   ( void* mem );

inline void* operator new( size_t bytes )
	{  return ( gpmalloc( bytes ) );  }
inline void operator delete( void* mem )
	{  gpfree( mem );  }

#else

inline void* gpmalloc ( size_t bytes )				{  return ( ::malloc( bytes ) );  }
inline void* gprealloc( void* mem, size_t bytes )	{  return ( ::realloc( mem, bytes ) );  }
inline void  gpfree   ( void* mem )					{  ::free( mem );  }

#endif

#include "gpmem_new_on.h"

//////////////////////////////////////////////////////////////////////////////

// this forces gpmem.cpp to be linked in. do not remove or alter this!
void gpmem_ForceLink( void );
GLOBALSTATIC_BEGIN( global, gpmem );
	gpmem_ForceLink();
GLOBALSTATIC_END( global, gpmem );

//////////////////////////////////////////////////////////////////////////////

#endif  // __GPMEM_H

//////////////////////////////////////////////////////////////////////////////
