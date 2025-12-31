//////////////////////////////////////////////////////////////////////////////
//
// File     :  FileSysDiyMap.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a do-it-yerdamnedself memory-mapped file system. No,
//             of course this isn't necessary, because Win2k has a very nice
//             one that works fine. But if you gotta run on Win9x, bend over
//             and use this stuff. It does have additional advantages - we
//             now get notified of every page fault, which means we can
//             optimize lots better, all automatically.
//
//             Why Win9x is a problem for memory-mapped files: I found out
//             through experimentation that when you memory-map a file in
//             Win9x, the map creation succeeds, but upon the first view being
//             mapped, it reserves the entire size of the map in address space.
//             This is done to solve problems with synchronizing interprocess
//             communication in the 1G shared virtual address space (according
//             to Andy Glaister @ MS). On Win2k of course you only reserve
//             what you map a view of. Well anyway our resource files are way
//             big, and won't fit. Not with Outlook running, or nVidia drivers
//             sucking up address space.
//
//             Note: this is a totally naive implementation. It's simple, and
//             it's all we need.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FILESYSDIYMAP_H
#define __FILESYSDIYMAP_H

//////////////////////////////////////////////////////////////////////////////

#include "FileSysDefs.h"
#include "FileSysUtils.h"
#include "KernelTool.h"

namespace FileSys  {  // begin of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
// class DiyFileMap declaration

class DiyFileMap
{
public:
	SET_NO_INHERITED( DiyFileMap );

// Types.

	enum eOptions
	{
		OPTION_NO_PAGING       = 1 << 0,			// don't use virtual paging method (slower, only for test/safemode purposes - default if debugger running on Win9x)
		OPTION_NO_VIRTUALALLOC = 1 << 1,			// does not use VirtualAlloc() for memory, uses "new" instead (implies the OPTION_NO_PAGING option)
		OPTION_USE_GUARD_PAGE  = 1 << 2,			// use guard pages if available, though it uses more memory if sparse access (can't reserve and use guard pages)
		OPTION_NO_CACHING      = 1 << 3,			// don't cache old data, just chuck it (use this for streaming optimizations)

#		if !GP_RETAIL
		OPTION_CHECK_BOUNDS    = 1 << 4,			// enable bounds-checking code that checks caps placed at ends of allocation to look for overwrites
#		endif // !GP_RETAIL

		// ...

		OPTION_NONE = 0,
	};

	enum eAccess
	{
		// $ copy-on-write not necessary, just use read/write

		ACCESS_READ_ONLY  = PAGE_READONLY,
		ACCESS_READ_WRITE = PAGE_READWRITE,
	};

	enum 
	{
		OVERRUN_ALLOW = 4,							// extra to allocate at the end to allow for overruns of trailing chunks of decompression routines
	};

// Setup.

	// ctor/dtor
	DiyFileMap( void );
   ~DiyFileMap( void );

// API.

	// management
	bool Create   ( eOptions options, eAccess access, Reader* takeReader, DWORD size, DWORD chunkSize );
	bool Create   ( eOptions options, eAccess access, HANDLE file, DWORD offset, DWORD size );
	bool Close    ( void );
	bool IsCreated( void ) const							{  return ( m_Base != NULL );  }

	// query
	const void* GetData( void ) const						{  return ( m_Base );  }
	void*       GetData( void )								{  return ( m_Base );  }

	// operations
	void Touch( const void* base = NULL, int size = -1 );

	// debugging
#	if !GP_RETAIL
	bool CheckBounds( void );
#	endif // !GP_RETAIL

private:

	void  PrivateInit        ( void );
	DWORD GetVirtualAllocType( void ) const;
	void  GetFillSizes       ( int& prefillSize, int& postfillSize );
	bool  PrepMemory         ( int offset, int size );
	bool  PageFault          ( void* address );

#	if !GP_RETAIL
	void  FillBadFood     ( void* base, int size );
	void  FillBadFoodPage ( void* base, DWORD protect = PAGE_NOACCESS );
	bool  CheckBadFood    ( void* base, int size );
	bool  CheckBadFoodPage( void* base );
#	endif // !GP_RETAIL

	static long CALLBACK ValidateMemorySehFilter( EXCEPTION_POINTERS* xinfo );

// Local data.

	typedef kerneltool::Critical Critical;

	// spec
	eOptions m_Options;								// current options for how the map behaves
	eAccess  m_Access;								// access level for entire map
	BYTE*    m_Base;								// base of allocation
	DWORD    m_Size;								// size of allocation
	DWORD    m_ChunkSize;							// size of each chunk (0 for not chunked)

	// state
	my Reader* m_Reader;							// pulls data in and stores in our memory
	Critical   m_Critical;							// locker for reading/writing our data (reader can only go one at a time)

	// global tracking
	ThisType* m_Next;								// next in the list (NULL-less ring)
	ThisType* m_Prev;								// prev in the list

	// stats
#	if GP_ENABLE_STATS
	eSystemProperty m_OwningSystem;					// owning system
#	endif // GP_ENABLE_STATS

// Static data.

	// for registering top-level SEH
	static bool ms_RegisteredHandler;

	// global tracking
	static ThisType ms_Root;						// root node
	static Critical ms_ListCritical;				// locker for linked list

	SET_NO_COPYING( DiyFileMap );
};

MAKE_ENUM_BIT_OPERATORS( DiyFileMap::eOptions );

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FileSys

#endif  // __FILESYSDIYMAP_H

//////////////////////////////////////////////////////////////////////////////
