//////////////////////////////////////////////////////////////////////////////
//
// File     :  FileSysUtils.h (GPCore:FileSys)
// Author(s):  Scott Bilas
//
// Summary  :  Contains random file system utility classes and functions.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FILESYSUTILS_H
#define __FILESYSUTILS_H

//////////////////////////////////////////////////////////////////////////////

#include "GpColl.h"
#include <list>

class Config;

namespace FileSys  {  // begin of namespace FileSys

//////////////////////////////////////////////////////////////////////////////

							 /*** WRAPPER FUNCTIONS ***/

//////////////////////////////////////////////////////////////////////////////
// Win32 file function wrappers

// these functions do two things - they track handle usage to prevent leaks
//  and other bad things, plus they will flash little lights on the screen
//  to show that Stuff Is Happening.

// note: these aren't total wrappers - functionality that win9x does not
//       support isn't included (i.e. security, 64-bit i/o, ...)

HANDLE GP_CreateFile       ( LPCTSTR           lpFileName,
							 DWORD             dwDesiredAccess,
							 DWORD             dwShareMode,
							 DWORD             dwCreationDisposition,
							 DWORD             dwFlagsAndAttributes );

BOOL   GP_CloseFileHandle  ( HANDLE            hFile );

HANDLE GP_CreateFileMapping( HANDLE            hFile,
							 DWORD             flProtect,
							 DWORD             dwMaximumSize = 0,
							 LPCTSTR           lpName        = NULL );

BOOL   GP_CloseMapHandle   ( HANDLE            hMap );

LPVOID GP_MapViewOfFile    ( HANDLE            hFileMappingObject,
							 DWORD             dwDesiredAccess,
							 DWORD             dwFileOffset,
							 DWORD             dwNumberOfBytesToMap );

BOOL   GP_UnmapViewOfFile  ( LPCVOID           lpBaseAddress );

BOOL   GP_ReadFile         ( HANDLE            hFile,
							 LPVOID            lpBuffer,
							 DWORD             nNumberOfBytesToRead,
							 LPDWORD           lpNumberOfBytesRead,
							 LPOVERLAPPED      lpOverlapped = NULL );

BOOL   GP_WriteFile        ( HANDLE            hFile,
							 LPCVOID           lpBuffer,
							 DWORD             nNumberOfBytesToWrite,
							 LPDWORD           lpNumberOfBytesWritten,
							 LPOVERLAPPED      lpOverlapped = NULL );

HANDLE GP_FindFirstFile    ( LPCTSTR           lpFileName,
							 LPWIN32_FIND_DATA lpFindFileData );

BOOL   GP_FindNextFile     ( HANDLE            hFindFile,
							 LPWIN32_FIND_DATA lpFindFileData );

BOOL   GP_FindClose        ( HANDLE            hFindFile );

//////////////////////////////////////////////////////////////////////////////
// Fun stuff

enum eFileIndicator
{
	FILEINDICATOR_OPENCLOSE,		// create/destroy file or file mapping
	FILEINDICATOR_MAPUNMAP,			// map/unmap view into file
	FILEINDICATOR_READWRITE,		// read or write on file
	FILEINDICATOR_INDEX,			// find file first/next
};

#	if GP_RETAIL
	inline void GP_Indicate( eFileIndicator, int = 1 )  {  }
#	else // GP_RETAIL
	void GP_Indicate( eFileIndicator indicator, int count = 1 );
#	endif // GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// Helper function declarations

// Verification.

	// special defines for bad things
#	define BAD_FILENAME_CHARS_ONLY       "<>:\"/\\|?*%;"
#	define BAD_FILENAME_CHARS_FULL       BAD_FILENAME_CHARS_ONLY "\t\n\v"
#	define BAD_FILENAME_CHARS_AND_SPACE  BAD_FILENAME_CHARS_FULL " "
#	define WBAD_FILENAME_CHARS_ONLY      L"<>:\"/\\|?*%;"
#	define WBAD_FILENAME_CHARS_FULL      WBAD_FILENAME_CHARS_ONLY L"\t\n\v"
#	define WBAD_FILENAME_CHARS_AND_SPACE WBAD_FILENAME_CHARS_FULL L" "

	// check for file existence
	bool DoesFileExist( const char* check );		// works with fully qualified or relative paths
	bool DoesFileExist( const wchar_t* check );		// works with fully qualified or relative paths

	// check for directory existence
	bool DoesPathExist( const char* check );		// works with fully qualified or relative paths
	bool DoesPathExist( const wchar_t* check );		// works with fully qualified or relative paths

	// this will tell you if that dir is writable or not
	bool IsDirWritable( const char* path );
	bool IsDirWritable( const wchar_t* path );

	// all valid chars in this?
	bool IsValidFileName( const char* check, bool checkReservedNames = true, bool spacesAllowed = true );
	bool IsValidFileName( const wchar_t* check, bool checkReservedNames = true, bool spacesAllowed = true );

// Path characteristics.

	enum ePathType
	{
		// $ possibly extend this with http:// etc. detection?

		// one of

		PATHTYPE_ABSOLUTE_NORMAL,	// c:\dir style path
		PATHTYPE_ABSOLUTE_SHARE,	// \\computer\share style path
		PATHTYPE_ABSOLUTE_URL,		// protocol://path/ style path

		PATHTYPE_RELATIVE_NORMAL,	// ordinary subdir\file style path
		PATHTYPE_RELATIVE_PARENT,	// ..\..\ relative into parent dir style path
		PATHTYPE_RELATIVE_DRIVE,	// \dir path-absolute but drive-relative style path
		PATHTYPE_RELATIVE_PATH,		// c:xxx path-relative but drive-absolute style path

		PATHTYPE_ILLEGAL,			// illegal name
	};

	// helpers
	inline bool IsAbsolute( ePathType type )  {  return ( (type == PATHTYPE_ABSOLUTE_NORMAL) || (type == PATHTYPE_ABSOLUTE_SHARE) || (type == PATHTYPE_ABSOLUTE_URL) );  }
	inline bool IsRelative( ePathType type )  {  return ( !IsAbsolute( type ) && (type != PATHTYPE_ILLEGAL) );  }

	// analyzes a path and gets its type - note that this function only does
	//  a simple parse and doesn't verify anything about the path existence.
	//  don't rely on this function to figure out actual drive type - use
	//  win32 API GetDriveType() instead for that. pass in a valid rootEndOffset
	//  to get back the offset right after the root of the path ends. will be 0
	//  if relative normal or parent, or if function can't figure out where the
	//  root ends.
	ePathType GetPathType( const char* path, int* rootEndOffset = NULL );
	ePathType GetPathType( const wchar_t* path, int* rootEndOffset = NULL );
	ePathType GetPathType( const char* path, const char*& rootEnd );
	ePathType GetPathType( const wchar_t* path, const wchar_t*& rootEnd );

	// more helpers
	inline bool IsAbsolute( const char*    path )  {  return ( IsAbsolute( GetPathType( path ) ) );  }
	inline bool IsAbsolute( const wchar_t* path )  {  return ( IsAbsolute( GetPathType( path ) ) );  }
	inline bool IsRelative( const char*    path )  {  return ( IsRelative( GetPathType( path ) ) );  }
	inline bool IsRelative( const wchar_t* path )  {  return ( IsRelative( GetPathType( path ) ) );  }

	// finds the start of the filename portion of a path - returns NULL if not found
	const char*    GetFileName( const char* path );
	const wchar_t* GetFileName( const wchar_t* path );

	// finds the start of the extension portion of a path - returns NULL if not found
	const char*    GetExtension( const char* path, bool leadingDot = false );
	const wchar_t* GetExtension( const wchar_t* path, bool leadingDot = false );

	// finds the filename, no extension - returns empty if not found
	gpstring  GetFileNameOnly( const char* path );
	gpwstring GetFileNameOnly( const wchar_t* path );

	// finds the end of the path portion of a path
	const char*    GetPathEnd( const char* path, bool trailingSlash = true );
	const wchar_t* GetPathEnd( const wchar_t* path, bool trailingSlash = true );

	// gets the path only, no filename - returns empty if none
	gpstring  GetPathOnly( const char* path, bool trailingSlash = true );
	gpwstring GetPathOnly( const wchar_t* path, bool trailingSlash = true );

	// cleans up a path, removing .\, double backslashes, and converting '/' to '\'
	void CleanupPath( gpstring& path );
	void CleanupPath( gpwstring& path );

// Finding loggable dir/file.

	// this will simply tell if a filename contains any path info
	bool HasPathInfo( const char* filename );
	bool HasPathInfo( const wchar_t* filename );

	// this will simply tell if a filename contains an extension
	bool HasExtension( const char* filename );
	bool HasExtension( const wchar_t* filename );

	// this will attempt to find a directory that can be written to most likely
	//  for error purposes. it tries directories in this order: exe dir, working
	//  dir, %temp% dir, %tmp% dir, and finally c:\.
	gpstring  GetLocalWritableDir ( bool trailingSlash = true );
	gpwstring GetLocalWritableDirW( bool trailingSlash = true );

	// this will try the temp dir first, returning empty if not writable/found
	gpstring  GetTempWritableDir ( bool trailingSlash = true, bool tryLocalWritableToo = false );
	gpwstring GetTempWritableDirW( bool trailingSlash = true, bool tryLocalWritableToo = false );

	// this will make a writable filename from the generic filename. if the
	// filename has path characters in it, then this uses it as-is, otherwise
	// it gets the path from GetLocalWritableDir() and tacks "filename"  on the
	// end of that.
	gpstring MakeWritableFileName( const char* filename );

	// this will generate a guaranteed unique name
	gpstring  MakeUniqueFileName( const char* base, const char* extension, const char* path = NULL, int maxTries = 1000 );
	gpwstring MakeUniqueFileName( const wchar_t* base, const wchar_t* extension, const wchar_t* path = NULL, int maxTries = 1000 );

// Simple Win32 wrappers.

	// retrieve a full path and filename from a partial/relative one (works on files and paths both)
	gpstring  GetFullPathName( const char* file, int* fileNameOffset = NULL );
	gpwstring GetFullPathName( const wchar_t* file );

	// retrieve the "long" path and filename (convert MICROS~1 into MICROSERF.EXE)
	gpstring GetLongPathName( const char* file );

	// defaults to name of file used to create calling process (i.e. local EXE)
	gpstring GetModuleFileName( HINSTANCE inst = NULL );

	// just gets the directory from above
	gpstring GetModuleDirectory( HINSTANCE inst = NULL, bool trailingSlash = true );

	// current working directory
	gpstring GetCurrentDirectory( bool trailingSlash = true );

	// generate temp filename - note that this may create a file if 'unique == 0' (see win32 docs)
	gpstring MakeTempFileName( const char* path, const char* prefix = "", UINT unique = 0, bool autoDelete = true );

	// same as above except it uses the system's temp path
	gpstring MakeTempPathFileName( const char* prefix = "", UINT unique = 0, bool autoDelete = true );

	// just get a temporary path
	gpstring  GetTempPath ( bool trailingSlash = true );
	gpwstring GetTempPathW( bool trailingSlash = true );

// More advanced wrappers.

	// create the full set of paths required to make this directory
	bool CreateFullDirectory( const char* path );
	bool CreateFullDirectory( const wchar_t* path );

	// create the full set of paths required to make this file
	bool CreateFullFileNameDirectory( const char* fileName );
	bool CreateFullFileNameDirectory( const wchar_t* fileName );

	// set a file as old as it can get
	bool SetFileTimeAncient( HANDLE file );

	// optionally recursively remove a dir and all its contents (use -1 for depth for infinite recursion, but be CAREFUL!!!)
	bool RemoveDirectory( const char* path, int recurseDepth = 0 );

	// checksum retrieval
	bool GetModuleChecksums( DWORD& headerCrc, DWORD& actualCrc, HINSTANCE inst = NULL );

// File finders.

	// this function will parse a ';'-delimited string of paths and extract all
	// files in it into a vector. a trailing /*/ on the path will make it
	// recursive, and the last spec can be a wildcard to match, which defaults
	// to "*". forward or backward slashes are supported. example path specs:
	//
	// test/*/*.ds - find all *.ds files recursively from base of test.
	// abc\        - find all files in the abc dir
	// abc\*/      - find all files in the abc dir recursively
	// xyzzy       - if xyzzy is a dir, find all files in it, otherwise use the file

	struct PathSpec
	{
		gpwstring m_PathName;
		gpwstring m_FileName;
		bool      m_Recursive;

		PathSpec( void )
		{
			m_Recursive = false;
		}

		bool Init( gpwstring spec, const wchar_t* defPattern = L"*", bool testExist = true );
	};

	int FindPathsFromSpec(									// returns # paths found
			const wchar_t* pathSpec,						// path spec to search on
			std::list <gpwstring> & foundPaths );			// all the paths we find go here

	int FindFilesFromSpec(									// returns # files found
			const wchar_t* pathSpec,						// path spec to search on
			std::list <gpwstring> & foundFiles,				// all the files we find go here
			bool parseSemicolons = true,					// sometimes semicolons can be used in paths
			const wchar_t* defPattern = L"*",				// default file pattern to use when unspecified
			std::list <gpwstring> * badPaths = NULL );		// (optional) bad paths in spec go here

//////////////////////////////////////////////////////////////////////////////

							/*** CLASS DECLARATIONS ***/

//////////////////////////////////////////////////////////////////////////////
// class File declaration

// purpose: to wrap a win32 file.

class File
{
public:
	SET_NO_INHERITED( File );

// Ctor/dtor.

	File( void )  {  m_Handle = INVALID_HANDLE_VALUE;  }
   ~File( void )  {  Close();  }

// Add some type safety to Win32 functions.

	enum eAccess
	{
		ACCESS_READ_ONLY  = GENERIC_READ,
		ACCESS_WRITE_ONLY = GENERIC_WRITE,
		ACCESS_READ_WRITE = GENERIC_READ | GENERIC_WRITE,
	};

	enum eSharing
	{
		SHARE_NONE       = 0,
		SHARE_READ_ONLY  = FILE_SHARE_READ,
		SHARE_WRITE_ONLY = FILE_SHARE_WRITE,
		SHARE_READ_WRITE = FILE_SHARE_READ | FILE_SHARE_WRITE,
	};

	enum eCreation
	{
		CREATION_NEW               = CREATE_NEW,
		CREATION_ALWAYS            = CREATE_ALWAYS,
		CREATION_OPEN_EXISTING     = OPEN_EXISTING,
		CREATION_OPEN_ALWAYS       = OPEN_ALWAYS,
		CREATION_TRUNCATE_EXISTING = TRUNCATE_EXISTING,
	};

// Create the file handle.

	inline bool Create( LPCTSTR   name,
						eAccess   access   = ACCESS_READ_ONLY,
						eSharing  sharing  = SHARE_READ_ONLY,
						eCreation creation = CREATION_OPEN_EXISTING,
						DWORD     flags    = FILE_ATTRIBUTE_NORMAL );
	bool CreateNew    ( LPCTSTR  name,
						eAccess  access    = ACCESS_READ_WRITE,
						eSharing sharing   = SHARE_READ_ONLY,
						DWORD    flags     = FILE_ATTRIBUTE_NORMAL )
						{  return ( Create( name, access, sharing, CREATION_NEW, flags ) );  }
	bool CreateAlways ( LPCTSTR  name,
						eAccess  access    = ACCESS_READ_WRITE,
						eSharing sharing   = SHARE_READ_ONLY,
						DWORD    flags     = FILE_ATTRIBUTE_NORMAL )
						{  return ( Create( name, access, sharing, CREATION_ALWAYS, flags ) );  }
	bool OpenExisting ( LPCTSTR  name,
						eAccess  access    = ACCESS_READ_ONLY,
						eSharing sharing   = SHARE_READ_ONLY,
						DWORD    flags     = FILE_ATTRIBUTE_NORMAL )
						{  return ( Create( name, access, sharing, CREATION_OPEN_EXISTING, flags ) );  }
	bool OpenAlways   ( LPCTSTR  name,
						eAccess  access    = ACCESS_READ_ONLY,
						eSharing sharing   = SHARE_READ_ONLY,
						DWORD    flags     = FILE_ATTRIBUTE_NORMAL )
						{  return ( Create( name, access, sharing, CREATION_OPEN_ALWAYS, flags ) );  }
	bool OpenTruncate ( LPCTSTR  name,
						eAccess  access    = ACCESS_READ_WRITE,
						eSharing sharing   = SHARE_READ_ONLY,
						DWORD    flags     = FILE_ATTRIBUTE_NORMAL )
						{  return ( Create( name, access, sharing, CREATION_TRUNCATE_EXISTING, flags ) );  }

	bool IsOpen       ( void ) const
						{  return ( m_Handle != INVALID_HANDLE_VALUE );  }

	HANDLE GetHandle  ( void ) const
						{  gpassert( IsOpen() );  return ( m_Handle );  }
	operator HANDLE   ( void ) const
						{  return ( GetHandle() );  }

	inline bool Close ( void );

	HANDLE Release    ( void );

	File& operator =  ( HANDLE h );

// Console helpers for lazy people who don't like to rtfm.

	bool CreateConsoleIn ( void )  {  return ( OpenExisting( "CONIN$", ACCESS_READ_WRITE, SHARE_READ_WRITE ) );  }
	bool CreateConsoleOut( void )  {  return ( OpenExisting( "CONOUT$", ACCESS_READ_WRITE, SHARE_READ_WRITE ) );  }

// The glorious Win32 file API, wrapped up all pretty.

	// file attributes
	DWORD GetSize    ( void ) const
					   {  return ( ::GetFileSize( *this, NULL ) );  }
	bool  GetSize    ( DWORD& size ) const
					   {  return ( ( size = GetSize() ) != 0xFFFFFFFF );  }
	bool  SetSize    ( DWORD size )
					   {  return ( SeekBegin( size ) && !!::SetEndOfFile( *this ) );  }
	bool  GetTime    ( FILETIME* creation, FILETIME* lastAccess, FILETIME* lastWrite )
					   {  return ( ::GetFileTime( *this, creation, lastAccess, lastWrite ) != FALSE );  }

	// file pointer
	bool  Seek       ( int offset, DWORD how = FILE_CURRENT )
					   {  return ( ::SetFilePointer( *this, offset, NULL, how ) != 0xFFFFFFFF );  }
	bool  SeekBegin  ( int offset = 0 )
					   {  return ( Seek( offset, FILE_BEGIN ) );  }
	bool  SeekEnd    ( int offset = 0 )
					   {  return ( Seek( offset, FILE_END ) );  }
	DWORD GetPos     ( void ) const
					   {  return ( ::SetFilePointer( *this, 0, NULL, FILE_CURRENT ) );  }
	bool  GetPos     ( DWORD& offset )  const
					   {  return ( ( offset = GetPos() ) != 0xFFFFFFFF );  }

	// reading
	bool  Read ( LPVOID buffer, int size, DWORD& read )
				 {  return ( GP_ReadFile( *this, buffer, size, &read ) != FALSE );  }
	bool  Read ( LPVOID buffer, int size )
				 {  DWORD dummy;  return ( Read( buffer, size, dummy ) );  }

	// writing
	bool  Write( LPCVOID buffer, int size, DWORD& written )
				 {  return ( GP_WriteFile( *this, buffer, size, &written ) != FALSE );  }
	bool  Write( LPCVOID buffer, int size )
				 {  DWORD dummy;  return ( Write( buffer, size, dummy ) );  }

	// advanced reading
	gpstring ReadIntoString( int size );
	gpstring ReadFileIntoString( void );

	// advanced writing
	bool WriteText( const char* text, int len = -1 );
	bool WriteArgs( const char* format, va_list args );
	bool WriteF   ( const char* format, ... )
					{  return ( WriteArgs( format, va_args( format ) ) );  }

	template <typename T>
	bool WriteStructs( const T* obj, int count = 1 )
					   {  return ( Write( obj, sizeof( T ) * count ) );  }

	template <typename T>
	bool WriteStruct( const T& obj )
					  {  return ( Write( &obj, sizeof( T ) ) );  }

private:
	HANDLE m_Handle;

	SET_NO_COPYING( File );
};

//////////////////////////////////////////////////////////////////////////////
// class FileMap declaration

// purpose: to wrap a win32 file map.

class FileMap
{
public:
	SET_NO_INHERITED( FileMap );

// Ctor/dtor.

	 FileMap( void )  {  m_Handle = NULL;  }
	~FileMap( void )  {  Close();  }

// Add some type safety to Win32 functions.

	enum eProtect
	{
		PROTECT_READ_ONLY     = PAGE_READONLY,
		PROTECT_READ_WRITE    = PAGE_READWRITE,
		PROTECT_COPY_ON_WRITE = PAGE_WRITECOPY,
	};

// Create the file map handle.

	inline bool Create  ( HANDLE   file,
						  eProtect protect      = PROTECT_READ_ONLY,
						  DWORD    maxSize      = 0,
						  LPCTSTR  name         = NULL,
						  DWORD    extraProtect = 0 );

	bool   IsMapped     ( void ) const
						  {  return ( m_Handle != NULL );  }

	inline bool Close   ( void );
	HANDLE Release      ( void );

	FileMap& operator = ( HANDLE h )
						  {  Close();  m_Handle = h;  return ( *this );  }

	HANDLE GetHandle    ( void ) const
						  {  gpassert( IsMapped() );  return ( m_Handle );  }
	operator HANDLE     ( void ) const
						  {  return ( GetHandle() );  }

private:
	HANDLE m_Handle;

	SET_NO_COPYING( FileMap );
};

//////////////////////////////////////////////////////////////////////////////
// class MapView declaration

// purpose: to wrap a win32 memory-mapped file view.

class MapView
{
public:
	SET_NO_INHERITED( MapView );

// Ctor/dtor.

	 MapView( void )  {  m_Data = NULL;  m_AlignedData = NULL;  }
	~MapView( void )  {  Close();  }

// Add some type safety to Win32 functions.

	enum eAccess
	{
		ACCESS_READ_ONLY     = FILE_MAP_READ,
		ACCESS_READ_WRITE    = FILE_MAP_WRITE, // $ not a bug - FILE_MAP_WRITE means r+w
		ACCESS_COPY_ON_WRITE = FILE_MAP_COPY,
	};

// Map the view.

	bool        Create   ( HANDLE  hmap,							// this auto-aligns to system granularity
						   eAccess access  = ACCESS_READ_ONLY,
						   DWORD   offset  = 0,
						   DWORD   mapSize = 0 );

	bool        IsViewed ( void ) const
						   {  return ( m_AlignedData != NULL );  }

	inline bool Close    ( void );
	HANDLE      Release  ( void );

// Access its data

	void*       GetData  ( void )
						   {  GP_Indicate( FILEINDICATOR_MAPUNMAP );  gpassert( IsViewed() );  return ( m_Data );  }
	const void* GetData  ( void ) const
						   {  GP_Indicate( FILEINDICATOR_MAPUNMAP );  gpassert( IsViewed() );  return ( m_Data );  }

private:
	LPVOID m_Data;
	LPVOID m_AlignedData;

	SET_NO_COPYING( MapView);
};

//////////////////////////////////////////////////////////////////////////////
// class MemoryFile declaration

// this is meant to map an entire file into memory. very simple. allows choice
//  of read/read+write and sharing. only works on files that already exist
//  (and are non-NULL length). everything else is default. generally want to
//  use this thing as read-only.

class MemoryFile
{
public:
	SET_NO_INHERITED( MemoryFile );

// Ctor/dtor.

	 MemoryFile( void )  {  }
	~MemoryFile( void )  {  Close();  }

// Add some type safety to Win32 functions.

	enum eAccess
	{
		ACCESS_READ_ONLY,
		ACCESS_READ_WRITE,
		ACCESS_COPY_ON_WRITE,
	};

	enum eSharing
	{
		SHARE_NONE       = 0,
		SHARE_READ_ONLY  = FILE_SHARE_READ,
		SHARE_WRITE_ONLY = FILE_SHARE_WRITE,
		SHARE_READ_WRITE = FILE_SHARE_READ | FILE_SHARE_WRITE,
	};

// Create the memory mapped file.

	bool        Create ( LPCTSTR  name,
						 eAccess  access       = ACCESS_READ_ONLY,
						 eSharing sharing      = SHARE_READ_ONLY,
						 DWORD    flags        = FILE_ATTRIBUTE_NORMAL,
						 bool*    wasFileEmpty = NULL,                   // can "fail" if zero length
						 DWORD    extraProtect = 0 );

	inline bool Create ( LPCTSTR  name,
						 bool*    wasFileEmpty );                        // can "fail" if zero length

	inline bool IsOpen ( void ) const;

	inline bool Close  ( void );

// Access.

	DWORD       GetSize( void ) const
						 {  return ( m_File.GetSize() );  }
	bool        GetSize( DWORD& size ) const
						 {  return ( m_File.GetSize(size) );  }

	void*       GetData( void )
						 {  return ( m_View.GetData() );  }
	const void* GetData( void ) const
						 {  return ( m_View.GetData() );  }

private:
	File    m_File;
	FileMap m_Map;
	MapView m_View;

	SET_NO_COPYING( MemoryFile );
};

//////////////////////////////////////////////////////////////////////////////
// class FileFinder declaration

// purpose: to wrap findfirst/nextfile().

class FileFinder
{
public:
	SET_NO_INHERITED( FileFinder );

// Ctor/dtor.

	 FileFinder( const char* filename = "" )
				 : m_Handle( NULL ), m_FileName( filename )  {  }
	 FileFinder( const wchar_t* filename )
				 : m_Handle( NULL ), m_FileName( ::ToAnsi( filename ) )  {  }
	~FileFinder( void )
				 {  Close();  }

	void Init ( const char* filename );
	void Init ( const wchar_t* filename );
	bool Close( void );

// Iteration.

	bool Next( void );
	bool Next( gpstring& str );
	bool Next( gpwstring& str );

// Access.

	const WIN32_FIND_DATA& Get( void ) const
		{  return ( m_FindData );  }
	bool IsDirectory( void ) const
		{  return ( (Get().dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 );  }
	bool IsFile( void ) const
		{  return ( !IsDirectory() );  }

private:
	gpstring        m_FileName;			// spec for filename
	HANDLE          m_Handle;			// handle to FindFileFirst()
	WIN32_FIND_DATA m_FindData;			// data passed back from FindFileX() functions

	SET_NO_COPYING( FileFinder );
};

//////////////////////////////////////////////////////////////////////////////
// class FileVersionInfo declaration

class FileVersionInfo
{
public:
	SET_NO_INHERITED( FileVersionInfo );

// Ctor/dtor.

	 FileVersionInfo( const char* filename, int index = 0 )						{  m_Valid = false;  Init( filename, index );  }
	 FileVersionInfo( void )													{  m_Valid = false;  }
	~FileVersionInfo( void )													{  }

	bool Init( const char* filename = "", int index = 0 );						// null file = currently active module, "index" is translation char set to use

	bool IsValid( void ) const													{  return ( m_Valid );  }

// Query.

	const VS_FIXEDFILEINFO& GetFixedInfo( void ) const;

	bool     GetTranslation    ( LANGID& lang, UINT& codePage, int index = 0 ) const;
	gpstring GetTranslationText( int index = 0 ) const;

	const char* GetEntry( const char* entry ) const;

private:
	bool QueryValue( const char* path, void*& data, UINT* len = NULL ) const;

	template <typename T>
	bool Query( const char* path, T*& data, UINT* len = NULL ) const			{  return ( QueryValue( path, *(void**)&data, len ) );  }

	bool			   m_Valid;
	stdx::fast_vector <BYTE> m_Data;
	gpstring		   m_StringPrefix;

	SET_NO_COPYING( FileVersionInfo );
};

//////////////////////////////////////////////////////////////////////////////
// class DirectoryKeeper declaration

class DirectoryKeeper
{
public:
	DirectoryKeeper( void )  {  m_OldPath = GetCurrentDirectory();  }
   ~DirectoryKeeper( void )  {  ::SetCurrentDirectory( m_OldPath );  }

private:
	gpstring m_OldPath;
};

//////////////////////////////////////////////////////////////////////////////

					 /*** CLASS INLINE IMPLEMENTATIONS ***/

//////////////////////////////////////////////////////////////////////////////
// class File inline implementation

inline bool File :: Create( LPCTSTR name, eAccess access, eSharing sharing,
							eCreation creation, DWORD flags )
{
	gpassert( !IsOpen() && (name != NULL) && (*name != '\0') );
	m_Handle = GP_CreateFile( name, access, sharing, creation, flags );
	return ( IsOpen() );
}

inline bool File :: Close( void )
{
	bool rc = true;
	if ( IsOpen() )
	{
		rc = GP_CloseFileHandle( m_Handle ) != FALSE;
		m_Handle = INVALID_HANDLE_VALUE;
	}
	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
// class FileMap inline implementation

inline bool FileMap :: Create( HANDLE   file,
							   eProtect protect,
							   DWORD    maxSize,
							   LPCTSTR  name,
							   DWORD    extraProtect )
{
	gpassert( !IsMapped() && ( file != NULL ) );
	m_Handle = GP_CreateFileMapping( file, protect | extraProtect, maxSize, name );
	return ( IsMapped() );
}

inline bool FileMap :: Close( void )
{
	bool rc = true;
	if ( IsMapped() )
	{
		rc = GP_CloseMapHandle( m_Handle ) != FALSE;
		m_Handle = NULL;
	}
	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
// class MapView inline implementation

inline bool MapView :: Close( void )
{
	bool rc = true;
	if ( IsViewed() )
	{
		rc = GP_UnmapViewOfFile( m_AlignedData ) != FALSE;
		m_Data = NULL;
		m_AlignedData = NULL;
	}
	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
// class MemoryFile inline implementation

inline bool MemoryFile :: Create( LPCTSTR name, bool* wasFileEmpty )
{
	return ( Create( name, ACCESS_READ_ONLY, SHARE_READ_ONLY, FILE_ATTRIBUTE_NORMAL, wasFileEmpty ) );
}

inline bool MemoryFile :: IsOpen( void ) const
{
#	if GP_DEBUG
	bool f = m_File.IsOpen();
	bool m = m_Map .IsMapped();
	bool v = m_View.IsViewed();
	gpassert( ( f == m ) && ( m == v ) );
#	endif // GP_DEBUG

	return ( m_View.IsViewed() );
}

inline bool MemoryFile :: Close( void )
{
	bool success = true;
	if ( !m_View.Close() )
	{
		success = false;
	}
	if ( !m_Map.Close() )
	{
		success = false;
	}
	if ( !m_File.Close() )
	{
		success = false;
	}
	return ( success );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FileSys

#endif  // __FILESYSUTILS_H

//////////////////////////////////////////////////////////////////////////////
