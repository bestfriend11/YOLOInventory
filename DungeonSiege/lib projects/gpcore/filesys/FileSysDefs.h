//////////////////////////////////////////////////////////////////////////////
//
// File     :  FileSysDefs.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains definitions and high level API functions related to
//             storage and the file system.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FILESYSDEFS_H
#define __FILESYSDEFS_H

#include "BlindCallback.h"
#include "GPCore.h"

//////////////////////////////////////////////////////////////////////////////

namespace FileSys  {  // begin of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
// forward declarations

class Reader;
class Writer;

class IFileMgr;

class Handle;
class FileHandle;
class AutoFileHandle;
class MemHandle;
class AutoMemHandle;
class FindHandle;
class AutoFindHandle;
class IndexPtr;

struct FindData;

//////////////////////////////////////////////////////////////////////////////
// helper function declarations

// Verification.

	// check for file existence
	bool DoesResourceFileExist( const char* check, IFileMgr* mgr = NULL );

	// check for directory existence
	bool DoesResourcePathExist( const char* check, IFileMgr* mgr = NULL );

// Configuration.

	// product id
	extern const FOURCC& PRODUCT_ID;
	void SetProductId( DWORD id );

	// callbacks for rebuilding the file system (usually to force-stop sounds)
	typedef CBFunctor0 RebuildCb;
	void SetRebuildCb( RebuildCb cb );
	void CallRebuildCb( void );

//////////////////////////////////////////////////////////////////////////////
// enum eSeekFrom declaration

enum eSeekFrom					// used by Seek()
{
	SEEKFROM_BEGIN   = FILE_BEGIN,		// offset from beginning of file
	SEEKFROM_CURRENT = FILE_CURRENT,	// offset from current position of file pointer
	SEEKFROM_END     = FILE_END,		// offset from end of file
};

//////////////////////////////////////////////////////////////////////////////
// enum eUsage declaration

// this is a hint to FileSys for how to optimize access to the given file.
//  right now it won't do a whole lot other than pass along the hint to
//  Win32 CreateFile() but in the future it may affect caching options within
//  tanks.

enum eUsage						// used by Open()
{
	USAGE_DEFAULT   =      0,		// sequential begin-to-end read access pattern

	// access patterns
	USAGE_RANDOM    = 1 << 0,		// nonsequential random access within this file
	USAGE_STREAMING = 1 << 1,		// streaming access, do not bother caching this data

	// memory protection
	USAGE_READWRITE = 1 << 2,		// allow writing to error data (for fixing up pointers etc.)
};

MAKE_ENUM_BIT_OPERATORS( eUsage );

//////////////////////////////////////////////////////////////////////////////
// enum eLocation declaration

// this describes where a file is stored.

enum eLocation					// where's the file?
{
	// one of
	LOCATION_PATH    = 1 << 0,		// file is by itself on a path somewhere
	LOCATION_TANK    = 1 << 1,		// file is contained in a tank

	// AND one of
	LOCATION_LOCAL   = 1 << 2,		// file is on a local writable disk
	LOCATION_NETWORK = 1 << 3,		// file is on the network
	LOCATION_CD      = 1 << 4,		// file is on a CD/DVD etc.

	// OR one of
	LOCATION_ERROR   = 0,			// error occurred during query
	LOCATION_UNKNOWN = 1 << 31,		// location not been calculated yet
};

MAKE_ENUM_BIT_OPERATORS( eLocation );

eLocation GetLocation( const char* path );

//////////////////////////////////////////////////////////////////////////////
// enum eFindFilter declaration

// this tunes a file find operation

enum eFindFilter				// what to look for?
{
	// one of
	FINDFILTER_DIRECTORIES = 1 << 0,	// include directories
	FINDFILTER_FILES       = 1 << 1,	// include files

	// OR one of
	FINDFILTER_ALL = 0xFFFFFFFF,	// look for all files and directories
};

MAKE_ENUM_BIT_OPERATORS( eFindFilter );

//////////////////////////////////////////////////////////////////////////////
// command constants

const int MAP_REST_OF_DATA   = -1;	// Map() - map the rest of the data
const int TOUCH_REST_OF_DATA = -1;	// Touch() - touch the rest of the data

//////////////////////////////////////////////////////////////////////////////
// error constants

const int INVALID_POS_VALUE  = -1;	// GetPos() - returned in failure case
const int INVALID_SIZE_VALUE = -1;	// GetSize() - returned in failure case

//////////////////////////////////////////////////////////////////////////////
// class Reader declaration

// abstract base class for reading data. note that the interfaces are the same
// as FileHandle.

class Reader
{
public:
	SET_NO_INHERITED( Reader );

	Reader( void )							{  }
	virtual ~Reader( void ) = 0				{  }

// Overrides.

	virtual bool Read    ( void* out, int size, int* bytesRead = NULL )    = 0;
	virtual bool ReadLine( gpstring& out )                                 = 0;
	virtual bool Seek    ( int offset, eSeekFrom origin = SEEKFROM_BEGIN ) = 0;
	virtual int  GetPos  ( void )                                          = 0;
	virtual int  GetSize ( void )                                          = 0;

// Helpers.

	// file pos
	bool Rewind      ( void )				{  return ( Seek( 0 ) );  }
	int  GetRemaining( void )				{  return ( GetSize() - GetPos() );  }
	bool IsEof       ( void )				{  return ( GetRemaining() == 0 );  }

	// read individual struct
	template <typename T>
	bool ReadStruct( T& obj )				{  return ( Read( &obj, sizeof( T ) ) );  }

	// read multiple structs
	template <typename T>
	bool ReadStructs( T* obj, int count = 1, int* structsRead = NULL )
	{
		int bytes = sizeof( T ) * count;
		bool rc = Read( obj, bytes, structsRead );
		if ( structsRead != NULL )
		{
			*structsRead /= sizeof( T );
		}
		return ( rc );
	}

	// read into a collection
	template <typename ITER>
	bool ReadIter( ITER out, int count = 1, int* structsRead = NULL )
	{
		for ( int i = 0 ; i < count ; ++i, ++out )
		{
			if ( !ReadStruct( *out ) )
			{
				break;
			}
		}
		if ( structsRead != NULL )
		{
			*structsRead = i;
		}
		return ( i == count );
	}

private:
	SET_NO_COPYING( Reader );
};

//////////////////////////////////////////////////////////////////////////////
// class StreamWriter declaration

class StreamWriter
{
public:
	SET_NO_INHERITED( StreamWriter );

	StreamWriter( void )										{  }
	virtual ~StreamWriter( void ) = 0							{  }

// Overrides.

	virtual bool Write    ( const void* out, int size ) = 0;
	virtual bool WriteText( const char* text, int len = -1 );
	virtual bool WriteArgs( const char* format, va_list args );

// Helpers.

	// write formatted text
	bool WriteF( const char* format, ... )				{  return ( WriteArgs( format, va_args( format ) ) );  }

	// write out a newline
	bool WriteEol( void )								{  return ( WriteText( "\n", 1 ) );  }

	// write individual struct
	template <typename T>
	bool WriteStruct( const T& obj )					{  return ( Write( &obj, sizeof( T ) ) );  }

	// write multiple structs
	template <typename T>
	bool WriteStructs( const T* obj, int count = 1 )	{  return ( Write( obj, sizeof( T ) * count ) );  }

	// write multiple structs
	template <typename ITER>
	bool WriteIter( ITER begin, ITER end )
	{
		for ( ; begin != end ; ++begin )
		{
			if ( !WriteStruct( *begin ) )
			{
				return ( false );
			}
		}
		return ( true );
	}

private:
	SET_NO_COPYING( StreamWriter );
};

//////////////////////////////////////////////////////////////////////////////
// class Writer declaration

class Writer : public StreamWriter
{
public:
	SET_NO_INHERITED( StreamWriter );

	Writer( void )										{  }
	virtual ~Writer( void ) = 0							{  }

// Overrides.

	virtual bool Seek  ( int offset, eSeekFrom origin = SEEKFROM_BEGIN ) = 0;
	virtual int  GetPos( void )                                          = 0;

// Helpers.

	// file pos
	bool Rewind( void )									{  return ( Seek( 0 ) );  }

private:
	SET_NO_COPYING( Writer );
};

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FileSys

#endif  // __FILESYSDEFS_H

//////////////////////////////////////////////////////////////////////////////
