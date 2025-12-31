//////////////////////////////////////////////////////////////////////////////
//
// File     :  FileSys.h (GPCore:FileSys)
// Author(s):  Scott Bilas
//
// Summary  :  This holds all the types and interfaces necessary to acquire
//             and use file resources. Check here for all docs as well.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FILESYS_H
#define __FILESYS_H

//////////////////////////////////////////////////////////////////////////////

#include "FileSysDefs.h"
#include "FileSysHandle.h"
#include "FuBiDefs.h"
#include "GpColl.h"

#include <list>

//////////////////////////////////////////////////////////////////////////////
// special defines

// these are extensions to the file attribute bitfield for storage type
#define FILE_ATTRIBUTE_COMPRESSED_ZLIB 0x40000000
#define FILE_ATTRIBUTE_COMPRESSED_LZO  0x80000000

//////////////////////////////////////////////////////////////////////////////

namespace FileSys  {  // begin of namespace FileSys

//////////////////////////////////////////////////////////////////////////////

							 /*** DOCUMENTATION ***/

//////////////////////////////////////////////////////////////////////////////
// Usage documentation

/*
	Usage:

		Use one of the file resource access or file space iteration classes.
		The IFileMgr interface is provided for inline efficiency but you'll
		likely never need it.

		IMPORTANT: thread safety is maintained within a filemgr but not for
		its handles. this is for performance and code simplicity reasons. there
		is no good reason that two threads that are unserialized would want to
		be trying to Read() from a file handle or have one Close() a memhandle
		while another calls GetData(). so do your own serialization if threads
		are going to share handles. as for allocation and deallocation of those
		handles, no worries there, that's protected.

	Namespace:

		All file system classes and types are enclosed in the FileSys
		namespace. Do not put a "using" directive in any .h files at global
		scope.

	File resource access classes:

		FileHandle
			This provides the familiar Open/Read/Close paradigm that we all
			grew up on.
		AutoFileHandle
			This is a FileHandle that will not leak (auto-closes its handle).
			Meant for stack objects.

		MemHandle
			This provides a pointer to read-only memory. Much simpler to use
			than a FileHandle.
		AutoMemHandle
			This is a MemHandle that will not leak (auto-closes its handle).
			Meant for stack objects.

	Directory/file iteration classes:

		FindHandle
			This is the generic file/dir finder handle. Similar to Win32
			FindFirst/NextFile() functions, but not the same.
		AutoFindHandle
			This is a FindHandle that will not leak (auto-closes its handle).
			Meant for stack objects.

		IndexPtr
			This is a special handle class that is optionally stored in
			FindData if indexing is supported. This pointer can be used
			for either (a) fast access to find files later, or (b) indexing of
			a file mgr by the master file mgr. It's really meant for (b) but it
			turns out that adding support for (a) was easy enough. Note that I'm
			only planning support for IndexPtrs in the MasterFileMgr and the
			TankFileMgr so it may not always be available.

	Error handling:

		Except where noted by !!, functions will return a bool for success or
		failure. On failure, the error code will be in ::GetLastError(). This
		code will never throw exceptions, but the OS may (examples: reading off
		the end of a memory handle == access violation, bad CD's or corrupt hard
		drive == in-page error).

		Invalid handles or other improper use of the file system will assert in
		addition to returning a failure code. Most of the functions that may
		return false are probably due to this so you can probably ignore error
		cases in functions like GetSize() - they will always succeed unless the
		handle itself is invalid.

		All usage-based failure cases will be reported via gperror or
		gpwarning. Any assertions will always be preceded by a more
		informative gpwarning when useful. You can also always check
		::GetLastError() to see what specifically went wrong - it may be set
		by a Win32 file function (e.g. ERROR_PATH_NOT_FOUND) or the file
		managers may set it themselves (e.g.. ERROR_INVALID_HANDLE).

	Performance:

		File resources are always accessed internally via memory mapped files so
		there is absolutely no server-side performance difference between using
		FileHandle or MemHandle. Also note that FileHandles and MemHandles can
		exist and be used simultaneously on the same or overlapping data. It's
		all the same to the underlying file system.

	Const-ness:

		I didn't make any of these functions const because I'm lazy. If anyone
		really needs const-correct methods in these handle classes, let me know.
*/

//////////////////////////////////////////////////////////////////////////////

						  /*** CLASS DECLARATIONS ***/

//////////////////////////////////////////////////////////////////////////////
// class FileHandle declaration

// this class is for accessing files via normal Open/Read/Close semantics.
//  easy and efficient. if you find yourself creating temporary local buffers,
//  Read()ing into them, and then deleting the buffers later - maybe you want
//  to consider using a (Auto)MemHandle as well. open the data with a FileHandle
//  and access it via a MemHandle.

// treat this class like a plain handle (HANDLE) - i.e. it's a dumb class
//  that owns a DWORD for its handle and does nothing special or unexpected.

class FileHandle : public Handle
{
public:
	SET_INHERITED( FileHandle, Handle );

	// ctor/dtor
	inline           FileHandle( void );											// default ctor nulls "*this"
	inline           FileHandle( Null_ );											// for explicit init to null
	inline           FileHandle( int );												// for implicit init to null
	inline explicit  FileHandle( Handle obj );										// copy the given handle
	inline explicit  FileHandle( const char* name, eUsage usage = USAGE_DEFAULT );	// init by opening the file - test IsOpen() after ctor for success
	inline explicit  FileHandle( IndexPtr where, eUsage usage = USAGE_DEFAULT );	// init by opening the file via IndexPtr - test IsOpen() after ctor for success

	// acquire/release
	inline bool      Open  ( const char* name, eUsage usage = USAGE_DEFAULT );		// open the given file with the given hint for usage and assign to "*this"
	inline bool      Open  ( IndexPtr where, eUsage usage = USAGE_DEFAULT );		// open the given file via IndexPtr with the given hint for usage and assign to "*this"
	inline bool      IsOpen( void );												// test is open or not - note that this just tests for null and doesn't actually verify file is open
	inline bool      Close ( void );												// close the file if it's open - does not null "*this"

	// read
	inline bool      Read    ( void* out, int size, int* bytesRead = NULL );		// read the given amount - optionally returns how much was read in 'bytesRead'
	inline bool      ReadLine( gpstring& out );										// reads up until a newline found and appends to the string - returns false if no newline found (though 'out' may contain data)

	// typed read
	template <typename T>
	inline bool      ReadStructs( T* obj, int count = 1, int* structsRead = NULL );	// read in an array of structs - optionally returns how much was read in 'structsRead' (rounded down)
	template <typename T>
	inline bool      ReadStruct( T& obj );											// read in a single struct - returns false if unable to read entire thing
	template <typename ITER>
	inline bool      ReadIter( ITER out, int count = 1, int* structsRead = NULL );	// read in a struct at a time, store in the out iter

	// read/write access
	inline bool      SetReadWrite( bool set = true );								// lock the file so we can write to it through a MemHandle, or re-protect it as read-only

	// file pointer
	inline bool      Seek        ( int offset, eSeekFrom origin = SEEKFROM_BEGIN );	// set file pointer to given position
	inline bool      Rewind      ( void );											// set file pointer back to start
	inline int       GetPos      ( void );											// get current file pointer (!! returns INVALID_POS_VALUE on failure)
	inline int       GetRemaining( void );											// retrieve remaining count of bytes in file
	inline bool      IsEof       ( void );											// no more data?

	// attribute query
	inline int       GetSize     ( void );											// get how big the file is (!! returns INVALID_SIZE_VALUE on failure)
	inline eLocation GetLocation ( void );											// get bitfield describing where the file was located (!! returns LOCATION_ERROR on failure)
	inline bool      GetFileTimes( FILETIME* creation, FILETIME* lastAccess,		// get one or more timestamps (ok to pass NULL for any of these)
								   FILETIME* lastWrite );
};

//////////////////////////////////////////////////////////////////////////////
// class AutoFileHandle declaration

// treat this class like an auto pointer that owns its data. it will never
//  leak what it's pointing to so if you reassign it or reopen it, know that
//  the old file handle will be closed.

class AutoFileHandle : private FileHandle
{
public:
	SET_INHERITED( AutoFileHandle, FileHandle );

	// ctor/dtor
	inline            AutoFileHandle( void );											// default ctor nulls "*this"
	inline            AutoFileHandle( Null_ );											// for explicit init to null
	inline            AutoFileHandle( int );											// for implicit init to null
	inline explicit   AutoFileHandle( const char* name, eUsage usage = USAGE_DEFAULT );	// init by opening the file - test IsOpen() after ctor for success
	inline explicit   AutoFileHandle( IndexPtr where, eUsage usage = USAGE_DEFAULT );	// init by opening the file via IndexPtr - test IsOpen() after ctor for success
	inline            AutoFileHandle( FileHandle obj );									// copy the given handle and own it
	inline           ~AutoFileHandle( void );											// auto-closes the file if it's open

	// acquire/release
	inline bool       Open   ( const char* name, eUsage usage = USAGE_DEFAULT );		// same as inherited Open() except first it auto-closes "*this" if already open
	inline bool       Open   ( IndexPtr where, eUsage usage = USAGE_DEFAULT );			// same as inherited Open() except first it auto-closes "*this" if already open
	inline bool       Close  ( void );													// same as inherited Close() except then it sets "*this" to null when done
	inline FileHandle Release( void );													// lets go of the owned file handle without closing it and sets "*this" to null (returns old value)

	// reassign
	inline AutoFileHandle& operator = ( FileHandle obj );								// copy the given handle and own it

	// query
	inline FileHandle GetHandle( void );												// get a copy of the owned handle
	inline            operator bool ( void );											// check IsOpen()

	// grant access
	using Inherited::IsOpen;
	using Inherited::Read;
	using Inherited::ReadLine;
	using Inherited::ReadStruct;
	using Inherited::ReadStructs;
	using Inherited::ReadIter;
	using Inherited::SetReadWrite;
	using Inherited::Seek;
	using Inherited::Rewind;
	using Inherited::GetPos;
	using Inherited::GetRemaining;
	using Inherited::IsEof;
	using Inherited::GetSize;
	using Inherited::GetLocation;
	using Inherited::GetFileTimes;

	// no smart pointer semantics for efficiency - prevent all copying
	SET_NO_COPYING( AutoFileHandle );
};

//////////////////////////////////////////////////////////////////////////////
// class MemHandle declaration

// this class is for accessing files via a pointer to read-only memory.

// treat this class like a plain handle (HANDLE) - i.e. it's a dumb class
//  that owns a DWORD for its handle and does nothing special or unexpected.

class MemHandle : public Handle
{
public:
	SET_INHERITED( MemHandle, Handle );

	// ctor/dtor
	inline             MemHandle( void );											// default ctor nulls "*this"
	inline             MemHandle( Null_ );											// for explicit init to null
	inline             MemHandle( int );											// for implicit init to null
	inline explicit    MemHandle( Handle obj );										// copy the given handle
	inline explicit    MemHandle( FileHandle file, int offset = 0,					// init by mapping a view into a file - test IsMapped() after ctor for success
								  int size = MAP_REST_OF_DATA );
	inline explicit    MemHandle( AutoFileHandle& file, int offset = 0,				// init by mapping a view into a file - test IsMapped() after ctor for success
								  int size = MAP_REST_OF_DATA );

	// acquire/release
	inline bool        Map( FileHandle file, int offset = 0,						// map a view into the given file starting at 'offset' for 'size' bytes (use MAP_REST_OF_DATA to do rest of file from offset)
								  int size = MAP_REST_OF_DATA );
	inline bool        Map( AutoFileHandle& file, int offset = 0,					// map a view into the given file starting at 'offset' for 'size' bytes (use MAP_REST_OF_DATA to do rest of file from offset)
								  int size = MAP_REST_OF_DATA );
	inline bool        IsMapped( void );											// test is mapped or not - note that this just tests for null and doesn't actually verify view is mapped
	inline bool        Close   ( void );											// unmap the view if it's open - does not null "*this"

	// operations
	inline bool        Touch  ( int offset = 0, int size = TOUCH_REST_OF_DATA );	// "touch" the given pages to force them to page into memory (use for preloading)
	inline const void* GetData( int offset = 0 );									// get the start of the memory mapped view + 'offset' bytes (!! returns NULL on failure)

	// query
	inline int         GetSize( void );												// get how big the mapped view is (!! returns INVALID_SIZE_VALUE on failure)
};

//////////////////////////////////////////////////////////////////////////////
// class AutoMemHandle declaration

// treat this class like an auto pointer that owns its data. it will never
//  leak what it's pointing to so if you reassign it or remap it, know that
//  the old mem handle will be closed.

class AutoMemHandle : private MemHandle
{
public:
	SET_INHERITED( AutoMemHandle, MemHandle );

	// ctor/dtor
	inline           AutoMemHandle( void );										// default ctor nulls "*this"
	inline           AutoMemHandle( Null_ );									// for explicit init to null
	inline           AutoMemHandle( int );										// for implicit init to null
	inline explicit  AutoMemHandle( FileHandle file, int offset = 0,			// init by mapping a view into a file - test IsMapped() after ctor for success
									int size = MAP_REST_OF_DATA );
	inline explicit  AutoMemHandle( AutoFileHandle& file, int offset = 0,		// init by mapping a view into a file - test IsMapped() after ctor for success
									int size = MAP_REST_OF_DATA );
	inline           AutoMemHandle( MemHandle obj );							// copy the given handle and own it
	inline          ~AutoMemHandle( void );										// auto-closes the map if it's open

	// acquire/release
	inline bool      Map    ( FileHandle file, int offset = 0,					// same as inherited Map() except first it auto-closes "*this" if already open
							  int size = MAP_REST_OF_DATA );
	inline bool      Map    ( AutoFileHandle& file, int offset = 0,				// same as inherited Map() except first it auto-closes "*this" if already open
							  int size = MAP_REST_OF_DATA );
	inline bool      Close  ( void );											// same as inherited Close() except that it sets "*this" to null when done
	inline MemHandle Release( void );											// lets go of the owned mem handle without closing it and sets "*this" to null (returns old value)

	// reassign
	inline AutoMemHandle& operator = ( MemHandle obj );							// copy the given handle and own it

	// query
	inline MemHandle GetHandle( void );											// get a copy of the owned handle
	inline           operator bool ( void );									// check IsMapped()

	// grant access
	using Inherited::IsMapped;
	using Inherited::Touch;
	using Inherited::GetData;
	using Inherited::GetSize;

	// no smart pointer semantics for efficiency - prevent all copying
	SET_NO_COPYING( AutoMemHandle );
};

//////////////////////////////////////////////////////////////////////////////
// class FindHandle declaration

// this class is for accessing files via normal Open/Read/Close semantics.
//  easy and efficient. if you find yourself creating temporary local buffers,
//  Read()ing into them, and then deleting the buffers later - maybe you want
//  to consider using a (Auto)MemHandle.

// treat this class like a plain handle (HANDLE) - i.e. it's a dumb class
//  that owns a DWORD for its handle and does nothing special or unexpected.

// note: these functions will never return "." or "..". so if you want to
//  see if a directory exists, you would have no way to tell the difference
//  between "directory invalid" and "directory empty". both will return false.
//  if you need this distinction, check ::GetLastError(). it will return
//  ERROR_PATH_NOT_FOUND if you gave it an invalid directory, and it will
//  return ERROR_FILE_NOT_FOUND if you gave it a valid directory but there
//  were no files in it.

class FindHandle : public Handle
{
public:
	SET_INHERITED( FindHandle, Handle );

	// ctor/dtor
	inline          FindHandle( void );														// default ctor nulls "*this"
	inline          FindHandle( Null_ );													// for explicit init to null
	inline          FindHandle( int );														// for implicit init to null
	inline explicit FindHandle( Handle obj );												// copy the given handle
	inline explicit FindHandle( const char* pattern, eFindFilter filter = FINDFILTER_ALL );	// init by beginning the find operation - test IsFinding() after ctor for success
	inline          FindHandle( FindHandle base, const char* pattern,						// init by beginning the find operation - test IsFinding() after ctor for success
								eFindFilter filter = FINDFILTER_ALL );

	// acquire/release
	inline bool Find        ( const char* pattern, eFindFilter filter = FINDFILTER_ALL );	// begin the find operation with the given pattern and filter and assign to "*this"

	inline bool Find        ( FindHandle base, const char* pattern,							// begin the find operation relative to 'base' with the given pattern and filter and assign to "*this" - only path info is used from 'base'
							  eFindFilter filter = FINDFILTER_ALL );
	inline bool IsFinding   ( void );														// test is finding or not - note that this just tests for null and doesn't actually verify that we're finding files
	inline bool Close       ( void );														// close the find operation if it's open - does not null "*this"

	// iteration
	inline bool GetNext( gpstring& out, bool prependPath = false );							// retrieve the name of the next match and store in 'out' - optionally prepend 'out' with the pathname
	inline bool GetNext( FindData& out, bool prependPath = false );							// retrieve full info for the next match and store in 'out' - optionally prepend 'out.m_Name'. with the pathname
	inline bool HasNext( void );															// just see if something is there
};

//////////////////////////////////////////////////////////////////////////////
// class AutoFindHandle declaration

// treat this class like an auto pointer that owns its data. it will never
//  leak what it's pointing to so if you reassign it or restart the find it,
//  know that the old find handle will be closed.

class AutoFindHandle : public FindHandle
{
public:
	SET_INHERITED( AutoFindHandle, FindHandle );

	// ctor/dtor
	inline            AutoFindHandle( void );												// default ctor nulls "*this"
	inline            AutoFindHandle( Null_ );												// for explicit init to null
	inline            AutoFindHandle( int );												// for implicit init to null
	inline explicit   AutoFindHandle( const char* pattern,
									  eFindFilter filter = FINDFILTER_ALL );				// init by beginning the find operation - test IsFinding() after ctor for success
	inline            AutoFindHandle( FindHandle base, const char* pattern,					// init by beginning the find operation - test IsFinding() after ctor for success
									  eFindFilter filter = FINDFILTER_ALL );
	inline            AutoFindHandle( AutoFindHandle& base, const char* pattern,			// init by beginning the find operation - test IsFinding() after ctor for success
									  eFindFilter filter = FINDFILTER_ALL );
	inline            AutoFindHandle( FindHandle obj );										// copy the given handle and own it
	inline           ~AutoFindHandle( void );												// auto-closes the find handle if it's open

	// acquire/release
	inline bool       Find( const char* pattern, eFindFilter filter = FINDFILTER_ALL );		// same as inherited Find() except first it auto-closes "*this" if already open
	inline bool       Find( FindHandle base, const char* pattern,							// same as inherited Find() except first it auto-closes "*this" if already open
							eFindFilter filter = FINDFILTER_ALL );
	inline bool       Find( AutoFindHandle& base, const char* pattern,						// same as inherited Find() except first it auto-closes "*this" if already open
							eFindFilter filter = FINDFILTER_ALL );
	inline bool       FindFromSelf( const char* pattern,
									eFindFilter filter = FINDFILTER_ALL );					// same as inherited Find() except first it auto-closes "*this" if already open
	inline bool       Close( void );														// same as inherited Close() except that it sets "*this" to null when done
	inline FindHandle Release( void );														// lets go of the owned mem handle without closing it and sets "*this" to null (returns old value)

	// query
	inline FindHandle GetHandle( void );													// get a copy of the owned handle
	inline            operator bool ( void );												// check for IsFinding()

	// reassign
	inline AutoFindHandle& operator = ( FindHandle obj );									// copy the given handle and own it

	// grant access
	using Inherited::IsFinding;
	using Inherited::GetNext;
	using Inherited::HasNext;

	// no smart pointer semantics for efficiency - prevent all copying
	SET_NO_COPYING( AutoFindHandle );
};

//////////////////////////////////////////////////////////////////////////////
// class RecursiveFinder declaration

class RecursiveFinder
{
public:
	SET_NO_INHERITED( RecursiveFinder );

	RecursiveFinder( const char* spec = "", int recurseLimit = -1, IFileMgr* mgr = NULL );	// -1 == infinite recursion
   ~RecursiveFinder( void );

	bool GetNext( gpstring& out, bool prependPath = false );
	bool GetNext( FindData& out, bool prependPath = false );

	operator bool( void )						{  return ( !m_FileFinder.IsNull() );  }	// file finder is all that matters

private:
	bool Advance( void );

	typedef stdx::fast_vector <FindHandle> FindColl;

	IFileMgr*  m_FileMgr;
	gpstring   m_FilePattern;
	FindHandle m_FileFinder;
	FindColl   m_DirFinderColl;
	int        m_RecurseLimit;

	SET_NO_COPYING( RecursiveFinder );
};

//////////////////////////////////////////////////////////////////////////////
// class IndexPtr declaration

// this class is for opening files directly and quickly - no searching involved
//  in the Open() operation. an IndexPtr is stored in the FindData structure
//  during search operations - this can be stored away locally and used again
//  to access files directly without the filemgr needing to do any searching.
//  the master file index will be built out of IndexPtrs. for typical file
//  operations don't bother with this class. it's exposed out of convenience
//  but will probably not be useful for most clients. note that this is just
//  a pointer, not a handle, and does not have open/close semantics. it also
//  may contain _anything_ so the meanings of its contents are completely
//  specific to the IFileMgr derivative that builds it.

class IndexPtr : public Handle
{
public:
	SET_INHERITED( IndexPtr, Handle );

	// ctor/dtor
	inline             IndexPtr( void );										// default ctor nulls "*this"
	inline             IndexPtr( Null_ );										// for explicit init to null
	inline             IndexPtr( int );											// for implicit init to null
	inline explicit    IndexPtr( Handle obj );									// copy the given handle
};

//////////////////////////////////////////////////////////////////////////////
// struct FindData declaration

// this is a cleaner version of WIN32_FIND_DATA tuned to our needs. see the
//  docs on WIN32_FIND_DATA for these members.

struct FindData
{
	FindData( void )
		{  ZeroMembers( m_Attributes, m_CompressedSize );  }

	void Fill( const WIN32_FIND_DATA& src );

	bool IsDirectory( void ) const
		{  return ( (m_Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 );  }
	bool IsFile( void ) const
		{  return ( !IsDirectory() );  }

	// $ note: be careful when inserting new members in here - update the ctor
	//         accordingly, and do not allow non-POD types in the set that
	//         ZeroMembers() clears.

	DWORD    m_Attributes;			// attributes of resource
	FILETIME m_CreationTime;		// when it was created (in UTC time)
	FILETIME m_LastAccessTime;		// when it was last accessed (in UTC time)
	FILETIME m_LastWriteTime;		// when it was last written (in UTC time)
	DWORD    m_Size;				// total size of resource
	DWORD    m_CompressedSize;		// compressed size of resource (0 if not compressed or compression not supported)
	gpstring m_Name;				// full name of file including relative path (optional depending on Find() param)
	IndexPtr m_IndexPtr;			// use for fast access to reopen file
#	if !GP_RETAIL
	gpstring m_Description;			// description field, used for whatever the filemgr wants to stick there
#	endif // !GP_RETAIL
};

//////////////////////////////////////////////////////////////////////////////
// class IFileMgr declaration

// see FileHandle, MemHandle, and FindHandle for docs on these functions

class IFileMgr
{
public:
	SET_NO_INHERITED( IFileMgr );

// Types.

	typedef stdx::fast_vector <gpstring> StringColl;

	struct Alias
	{
		gpstring   m_From;
		StringColl m_ToColl;
	};

	typedef stdx::fast_vector <Alias> AliasColl;
	typedef stdx::fast_vector <gpstring> CopyLocalColl;

// Setup.

			inline 		IFileMgr( void );
	virtual inline 	   ~IFileMgr( void );

	virtual bool		Enable   ( bool enable = true )							{  m_Enabled = enable;  return ( true );  }
			bool		Disable  ( void )										{  return ( Enable( false ) );  }
			bool		IsEnabled( void ) const									{  return ( m_Enabled );  }

// File i/o functions - standard read-only file pointer data access.

	virtual FileHandle	Open ( const char* name, eUsage usage = USAGE_DEFAULT ) = 0;
	virtual FileHandle	Open ( IndexPtr where, eUsage usage = USAGE_DEFAULT ) = 0;
	virtual bool		Close( FileHandle file ) = 0;

	virtual bool		Read    ( FileHandle file, void* out, int size, int* bytesRead = NULL ) = 0;
	virtual bool		ReadLine( FileHandle file, gpstring& out ) = 0;

	virtual bool		SetReadWrite( FileHandle file, bool set = true ) = 0;

	virtual bool		Seek  ( FileHandle file, int offset, eSeekFrom origin = SEEKFROM_BEGIN ) = 0;
	virtual int			GetPos( FileHandle file ) = 0;

	virtual int			GetSize     ( FileHandle file ) = 0;
	virtual eLocation	GetLocation ( FileHandle file ) = 0;
	virtual bool		GetFileTimes( FileHandle file, FILETIME* creation, FILETIME* lastAccess, FILETIME* lastWrite ) = 0;

// Memory mapped i/o functions - read-only access via memory pointer.

	virtual MemHandle	Map  ( FileHandle file, int offset = 0, int size = MAP_REST_OF_DATA ) = 0;
	virtual bool		Close( MemHandle mem ) = 0;

	virtual int			GetSize( MemHandle mem ) = 0;
	virtual bool		Touch  ( MemHandle mem, int offset = 0, int size = TOUCH_REST_OF_DATA ) = 0;
	virtual const void* GetData( MemHandle mem, int offset = 0 ) = 0;

// File/directory iteration functions.

	virtual FindHandle	Find      ( const char* pattern, eFindFilter filter = FINDFILTER_ALL ) = 0;
	virtual FindHandle	Find      ( FindHandle base, const char* pattern, eFindFilter filter = FINDFILTER_ALL ) = 0;
	virtual bool		GetNext   ( FindHandle find, gpstring& out, bool prependPath = false ) = 0;
	virtual bool		GetNext   ( FindHandle find, FindData& out, bool prependPath = false ) = 0;
	virtual bool		HasNext   ( FindHandle find ) = 0;
	virtual bool		Close     ( FindHandle find ) = 0;

// For internal FileSys use only.

	// fill in everything in 'out' except name. this will be called by an
	//  indexing system (like MasterFileMgr). the 'where' will be pointing to
	//  either an actual IndexPtr returned from a GetNext() find operation, or
	//  if that was NULL, it must be cast to a const char* for the relative
	//  pathname.
	virtual bool		QueryFileInfo( IndexPtr where, FindData& out ) = 0;

// Global FileSys configuration.

	// leave off the '.' when using aliases
	static void              AddFileTypeAlias   ( const char* fromType, const char* toType );
	static void              RemoveFileTypeAlias( const char* fromType, const char* toType );
	static const AliasColl&  GetFileTypeAliases ( void )  {  return ( ms_AliasColl );  }
	static const StringColl* GetFileTypeAliases ( const gpstring& fileName, int* offset = NULL );
	static const StringColl* GetFileTypeAliases ( const gpdumbstring& fileName, int* offset = NULL );
	static const StringColl* GetFileTypeAliases ( const char* fileName, int* offset = NULL );
	static bool              IsAliasedFrom      ( const char* fileName, const char* alias );

	// copy-local aliases for loading files across the network
	static void                 AddCopyLocalType   ( const char* type );
	static void                 RemoveCopyLocalType( const char* type );
	static const CopyLocalColl& GetCopyLocalTypes  ( void )  {  return ( ms_CopyLocalColl );  }

// Other.

	static inline ThisType* GetMasterFileMgr    ( void );		// this always returns valid pointer (asserts otherwise)
	static inline bool      HasMasterFileMgr    ( void );
	static void             SetMasterFileMgr    ( ThisType* mgr );
	static void             ReleaseMasterFileMgr( ThisType* mgr );

	void SetMasterFileMgr( void )  {  SetMasterFileMgr( this );  }

private:
	bool m_Enabled;

	static ThisType*     ms_MasterFileMgr;  // singleton
	static AliasColl     ms_AliasColl;
	static CopyLocalColl ms_CopyLocalColl;

	SET_NO_COPYING( IFileMgr );
};

//////////////////////////////////////////////////////////////////////////////
// class CmdProcessor declaration

#if !GP_RETAIL

// just a little dos command line style processor

class CmdProcessor
{
public:
	SET_NO_INHERITED( CmdProcessor );

	// $$ future: add sorting/filtering options...
	// $$ future: add recursion, '/f' option, etc. (see 4nt)

	CmdProcessor( IFileMgr* mgr = NULL, ReportSys::ContextRef ctx = NULL );
   ~CmdProcessor( void )  {  }

	void SetFileMgr( IFileMgr* mgr );
	void SetReportContext( ReportSys::ContextRef );

	bool Dir ( const char* pattern = "*" );
	bool Cd  ( const char* dir );
	int  Copy( const char* pattern, const char* outDir = NULL );

	const gpstring& GetCurrentDir( void ) const  {  return ( m_CurrentDir );  }

private:
	typedef std::list <FindData> FindDataList;

	void GetFindResults ( FindHandle find, int& totalSize, int& totalFiles, FindDataList& findDataList );
	void OutputDirEntry ( const FindData& findData, int nameWidth );
	void OutputDirHeader( const char* dir, int& totalDirs, FindDataList& findDataList );

	IFileMgr* m_FileMgr;
	ReportSys::ContextRef m_Context;
	gpstring m_CurrentDir;

	SET_NO_COPYING( CmdProcessor );
};

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

					 /*** CLASS INLINE IMPLEMENTATIONS ***/

//////////////////////////////////////////////////////////////////////////////
// class FileHandle inline implementation

inline FileHandle :: FileHandle( void )
	{  /* let inherited null "*this" */ }

inline FileHandle :: FileHandle( Null_ )
	{  /* let inherited null "*this" */ }

inline FileHandle :: FileHandle( int )
	{  /* let inherited null "*this" */ }

inline FileHandle :: FileHandle( Handle obj )
	: Inherited( obj )  {  }

inline FileHandle :: FileHandle( const char* name, eUsage usage )
	{  Open( name, usage );  }

inline FileHandle :: FileHandle( IndexPtr where, eUsage usage )
	{  Open( where, usage );  }

inline bool FileHandle :: Open( const char* name, eUsage usage )
{
	*this = IFileMgr::GetMasterFileMgr()->Open( name, usage );
	return ( IsOpen() );
}

inline bool FileHandle :: Open( IndexPtr where, eUsage usage )
{
	*this = IFileMgr::GetMasterFileMgr()->Open( where, usage );
	return ( IsOpen() );
}

inline bool FileHandle :: IsOpen( void )
	{  return ( !IsNull() );  }

inline bool FileHandle :: Close( void )
{
	if ( !IsOpen() )
	{
		return ( true );
	}
	bool rc = IFileMgr::GetMasterFileMgr()->Close( *this );
	*this = Null;
	return ( rc );
}

inline bool FileHandle :: Read( void* out, int size, int* bytesRead )
	{  return ( IFileMgr::GetMasterFileMgr()->Read( *this, out, size, bytesRead ) );  }

inline bool FileHandle :: ReadLine( gpstring& out )
	{  return ( IFileMgr::GetMasterFileMgr()->ReadLine( *this, out ) );  }

template <typename T>
inline bool FileHandle :: ReadStruct( T& obj )
{
	return ( Read( &obj, sizeof( T ) ) );
}

template <typename T>
inline bool FileHandle :: ReadStructs( T* obj, int count, int* structsRead )
{
	int bytes = sizeof( T ) * count;
	bool rc = Read( obj, bytes, structsRead );
	if ( structsRead != NULL )
	{
		*structsRead /= sizeof( T );
	}
	return ( rc );
}

template <typename ITER>
inline bool FileHandle :: ReadIter( ITER out, int count, int* structsRead )
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

inline bool FileHandle :: Seek( int offset, eSeekFrom origin )
	{  return ( IFileMgr::GetMasterFileMgr()->Seek( *this, offset, origin ) );  }

inline bool FileHandle :: SetReadWrite( bool set )
	{  return ( IFileMgr::GetMasterFileMgr()->SetReadWrite( *this, set ) );  }

inline bool FileHandle :: Rewind( void )
	{  return ( Seek( 0 ) );  }

inline int FileHandle :: GetPos( void )
	{  return ( IFileMgr::GetMasterFileMgr()->GetPos( *this ) );  }

inline int FileHandle :: GetRemaining( void )
	{  return ( GetSize() - GetPos() );  }

inline bool FileHandle :: IsEof( void )
	{  return ( GetRemaining() == 0 );  }

inline int FileHandle :: GetSize( void )
	{  return ( IFileMgr::GetMasterFileMgr()->GetSize( *this ) );  }

inline eLocation FileHandle :: GetLocation( void )
	{  return ( IFileMgr::GetMasterFileMgr()->GetLocation( *this ) );  }

inline bool FileHandle :: GetFileTimes( FILETIME* creation, FILETIME* lastAccess, FILETIME* lastWrite )
	{  return ( IFileMgr::GetMasterFileMgr()->GetFileTimes( *this, creation, lastAccess, lastWrite ) );  }

//////////////////////////////////////////////////////////////////////////////
// class AutoFileHandle inline implementation

inline AutoFileHandle :: AutoFileHandle( void )
	{  /* let inherited null "*this" */  }

inline AutoFileHandle :: AutoFileHandle( Null_ )
	{  /* let inherited null "*this" */ }

inline AutoFileHandle :: AutoFileHandle( int )
	{  /* let inherited null "*this" */ }

inline AutoFileHandle :: AutoFileHandle( const char* name, eUsage usage )
	: Inherited( name, usage )  {  }

inline AutoFileHandle :: AutoFileHandle( IndexPtr where, eUsage usage )
	: Inherited( where, usage )  {  }

inline AutoFileHandle :: AutoFileHandle( FileHandle obj )
	: Inherited( obj )  {  }

inline AutoFileHandle :: ~AutoFileHandle( void )
	{  Close();  }

inline bool AutoFileHandle :: Open( const char* name, eUsage usage )
{
	Close();
	return ( Inherited::Open( name, usage ) );
}

inline bool AutoFileHandle :: Open( IndexPtr where, eUsage usage )
{
	Close();
	return ( Inherited::Open( where, usage ) );
}

inline bool AutoFileHandle :: Close( void )
{
	return ( Inherited::Close() );
}

inline FileHandle AutoFileHandle :: Release( void )
{
	FileHandle old = *this;
	scast <Inherited&> ( *this ) = Null;
	return ( old );
}

inline AutoFileHandle& AutoFileHandle :: operator = ( FileHandle obj )
{
	if ( *this != obj )
	{
		Close();
		scast <Inherited&> ( *this ) = obj;
	}
	return ( *this );
}

inline FileHandle AutoFileHandle :: GetHandle( void )
	{  return ( *this );  }

inline AutoFileHandle :: operator bool( void )
	{  return ( IsOpen() );  }

//////////////////////////////////////////////////////////////////////////////
// class MemHandle inline implementation

inline MemHandle :: MemHandle( void )
	{  /* let inherited null "*this" */  }

inline MemHandle :: MemHandle( Null_ )
	{  /* let inherited null "*this" */  }

inline MemHandle :: MemHandle( int )
	{  /* let inherited null "*this" */  }

inline MemHandle :: MemHandle( Handle obj )
	: Inherited( obj )  {  }

inline MemHandle :: MemHandle( FileHandle file, int offset, int size )
	{  Map( file, offset, size );  }

inline MemHandle :: MemHandle( AutoFileHandle& file, int offset, int size )
	{  Map( file.GetHandle(), offset, size );  }

inline bool MemHandle :: Map( FileHandle file, int offset, int size )
{
	*this = IFileMgr::GetMasterFileMgr()->Map( file, offset, size );
	return ( IsMapped() );
}

inline bool MemHandle :: Map( AutoFileHandle& file, int offset, int size )
{
	*this = IFileMgr::GetMasterFileMgr()->Map( file.GetHandle(), offset, size );
	return ( IsMapped() );
}

inline bool MemHandle :: IsMapped( void )
	{  return ( !IsNull() );  }

inline bool MemHandle :: Close( void )
{
	if ( !IsMapped() )
	{
		return ( true );
	}
	bool rc = IFileMgr::GetMasterFileMgr()->Close( *this );
	*this = Null;
	return ( rc );
}

inline bool MemHandle :: Touch( int offset, int size )
	{  return ( IFileMgr::GetMasterFileMgr()->Touch( *this, offset, size ) );  }

inline const void* MemHandle :: GetData( int offset )
	{  return ( IFileMgr::GetMasterFileMgr()->GetData( *this, offset ) );  }

inline int MemHandle :: GetSize( void )
	{  return ( IFileMgr::GetMasterFileMgr()->GetSize( *this ) );  }

//////////////////////////////////////////////////////////////////////////////
// class AutoMemHandle inline implementation

inline AutoMemHandle :: AutoMemHandle( void )
	{  /* let inherited null "*this" */  }

inline AutoMemHandle :: AutoMemHandle( Null_ )
	{  /* let inherited null "*this" */  }

inline AutoMemHandle :: AutoMemHandle( int )
	{  /* let inherited null "*this" */  }

inline AutoMemHandle :: AutoMemHandle( FileHandle file, int offset, int size )
	: Inherited( file, offset, size )  {  }

inline AutoMemHandle :: AutoMemHandle( AutoFileHandle& file, int offset, int size )
	: Inherited( file.GetHandle(), offset, size )  {  }

inline AutoMemHandle :: AutoMemHandle( MemHandle obj )
	: Inherited( obj )  {  }

inline AutoMemHandle :: ~AutoMemHandle( void )
	{  Close();  }

inline bool AutoMemHandle :: Map( FileHandle file, int offset, int size )
{
	Close();
	return ( Inherited::Map( file, offset, size ) );
}

inline bool AutoMemHandle :: Map( AutoFileHandle& file, int offset, int size )
{
	Close();
	return ( Inherited::Map( file.GetHandle(), offset, size ) );
}

inline bool AutoMemHandle :: Close( void )
{
	return ( Inherited::Close() );
}

inline MemHandle AutoMemHandle :: Release( void )
{
	MemHandle old = *this;
	scast <Inherited&> ( *this ) = Null;
	return ( old );
}

inline AutoMemHandle& AutoMemHandle :: operator = ( MemHandle obj )
{
	if ( *this != obj )
	{
		Close();
		scast <Inherited&> ( *this ) = obj;
	}
	return ( *this );
}

inline MemHandle AutoMemHandle :: GetHandle( void )
	{  return ( *this);  }

inline AutoMemHandle :: operator bool( void )
	{  return ( IsMapped() );  }

//////////////////////////////////////////////////////////////////////////////
// class FindHandle inline implementation

inline FindHandle :: FindHandle( void )
	{  /* let inherited null "*this" */ }

inline FindHandle :: FindHandle( Null_ )
	{  /* let inherited null "*this" */ }

inline FindHandle :: FindHandle( int )
	{  /* let inherited null "*this" */ }

inline FindHandle :: FindHandle( Handle obj )
	: Inherited( obj )  {  }

inline FindHandle :: FindHandle( const char* pattern, eFindFilter filter )
	{  Find( pattern, filter );  }

inline FindHandle :: FindHandle( FindHandle base, const char* pattern, eFindFilter filter )
	{  Find( base, pattern, filter );  }

inline bool FindHandle :: Find( const char* pattern, eFindFilter filter )
{
	*this = IFileMgr::GetMasterFileMgr()->Find( pattern, filter );
	return ( IsFinding() );
}

inline bool FindHandle :: Find( FindHandle base, const char* pattern, eFindFilter filter )
{
	*this = IFileMgr::GetMasterFileMgr()->Find( base, pattern, filter );
	return ( IsFinding() );
}

inline bool FindHandle :: IsFinding( void )
	{  return ( !IsNull() );  }

inline bool FindHandle :: Close( void )
{
	if ( !IsFinding() )
	{
		return ( true );
	}
	bool rc = IFileMgr::GetMasterFileMgr()->Close( *this );
	*this = Null;
	return ( rc );
}

inline bool FindHandle :: GetNext( gpstring& out, bool prependPath )
	{  return ( IFileMgr::GetMasterFileMgr()->GetNext( *this, out, prependPath ) );  }

inline bool FindHandle :: GetNext( FindData& out, bool prependPath )
	{  return ( IFileMgr::GetMasterFileMgr()->GetNext( *this, out, prependPath ) );  }

inline bool FindHandle :: HasNext( void )
	{  return ( IFileMgr::GetMasterFileMgr()->HasNext( *this ) );  }

//////////////////////////////////////////////////////////////////////////////
// class AutoFindHandle inline implementation

inline AutoFindHandle :: AutoFindHandle( void )
	{  /* let inherited null "*this" */ }

inline AutoFindHandle ::  AutoFindHandle( Null_ )
	{  /* let inherited null "*this" */ }

inline AutoFindHandle ::  AutoFindHandle( int )
	{  /* let inherited null "*this" */ }

inline AutoFindHandle ::  AutoFindHandle( const char* pattern, eFindFilter filter )
	: Inherited( pattern, filter )  {  }

inline AutoFindHandle ::  AutoFindHandle( FindHandle base, const char* pattern, eFindFilter filter )
	: Inherited( base, pattern, filter )  {  }

inline AutoFindHandle ::  AutoFindHandle( AutoFindHandle& base, const char* pattern, eFindFilter filter )
	: Inherited( base.GetHandle(), pattern, filter )  {  }

inline AutoFindHandle ::  AutoFindHandle( FindHandle obj )
	: Inherited( obj )  {  }

inline AutoFindHandle :: ~AutoFindHandle( void )
	{  Close();  }

inline bool AutoFindHandle :: Find( const char* pattern, eFindFilter filter )
{
	Close();
	return ( Inherited::Find( pattern, filter ) );
}

inline bool AutoFindHandle :: Find( FindHandle base, const char* pattern, eFindFilter filter )
{
	FindHandle old = *this;
	bool rc = Inherited::Find( base, pattern, filter );
	old.Close();
	return ( rc );
}

inline bool AutoFindHandle :: Find( AutoFindHandle& base, const char* pattern, eFindFilter filter )
{
	FindHandle old = *this;
	bool rc = Inherited::Find( base.GetHandle(), pattern, filter );
	old.Close();
	return ( rc );
}

inline bool AutoFindHandle :: FindFromSelf( const char* pattern, eFindFilter filter )
{
	return ( Find( *this, pattern, filter ) );
}

inline bool AutoFindHandle :: Close( void )
{
	return ( Inherited::Close() );
}

inline FindHandle AutoFindHandle :: Release( void )
{
	FindHandle old = *this;
	scast <Inherited&> ( *this ) = Null;
	return ( old );
}

inline AutoFindHandle& AutoFindHandle :: operator = ( FindHandle obj )
{
	if ( *this != obj )
	{
		Close();
		scast <Inherited&> ( *this ) = obj;
	}
	return ( *this );
}

inline FindHandle AutoFindHandle :: GetHandle( void )
	{  return ( *this );  }

inline AutoFindHandle :: operator bool( void )
	{  return ( IsFinding() );  }

//////////////////////////////////////////////////////////////////////////////
// class IndexPtr inline implementation

inline IndexPtr :: IndexPtr( void )
	{  /* let inherited null "*this" */ }
inline IndexPtr :: IndexPtr( Null_ )
	{  /* let inherited null "*this" */ }
inline IndexPtr :: IndexPtr( int )
	{  /* let inherited null "*this" */ }
inline IndexPtr :: IndexPtr( Handle obj )
	: Inherited( obj )  {  }

//////////////////////////////////////////////////////////////////////////////
// class IFileMgr inline implementation

inline IFileMgr :: IFileMgr( void )
{
	m_Enabled = true;
}

inline IFileMgr :: ~IFileMgr( void )
	{  }

inline IFileMgr* IFileMgr :: GetMasterFileMgr( void )
{
	gpassert( ms_MasterFileMgr != NULL );
	return ( ms_MasterFileMgr );
}

inline bool IFileMgr :: HasMasterFileMgr( void )
{
	return ( ms_MasterFileMgr != NULL );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FileSys

//////////////////////////////////////////////////////////////////////////////

#if !GP_RETAIL
FEX void DbgDir( const char* path );
#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

#endif  // __FILESYS_H

//////////////////////////////////////////////////////////////////////////////
