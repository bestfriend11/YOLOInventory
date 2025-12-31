//////////////////////////////////////////////////////////////////////////////
//
// File     :  PathFileMgr.cpp (GPCore:FileSys)
// Author(s):  Scott Bilas
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "PathFileMgr.h"
#include "FileSysUtils.h"
#include "MasterFileMgr.h"
#include "ReportSys.h"
#include "StringTool.h"

namespace FileSys  {  // begin of namespace FileSys

// $$ future: Switch to the MemReader code instead to collect functionality.

//////////////////////////////////////////////////////////////////////////////
// class PathFileMgr implementation

PathFileMgr :: PathFileMgr( void )
{
	PrivateInit();
}

PathFileMgr :: PathFileMgr( const char* root, bool autoResolve )
{
	PrivateInit();
	SetRoot( root, autoResolve );
}

PathFileMgr :: ~PathFileMgr( void )
{
#	if ( GP_DEBUG )
	{
		// detect leaks
		bool leakMem  = m_MemHandleMgr .HasUsedHandles();
		bool leakFile = m_FileHandleMgr.HasUsedHandles();
		bool leakFind = m_FindHandleMgr.HasUsedHandles();

		// check for and dump leaks
		gpassertm( !leakMem,  "PathFileMgr destroyed with memory handles still open!" );
		gpassertm( !leakFile, "PathFileMgr destroyed with file handles still open!" );
		gpassertm( !leakFind, "PathFileMgr destroyed with find handles still open!" );

		// dump the leaks
		if ( leakMem || leakFile || leakFind )
		{
			gperror( "** PathFileMgr detected handle leaks **\n" );
			Dump( gErrorContext );
		}
	}
#	endif

	// free up handles
	m_MemHandleMgr .CloseAllHandles( this, MemHandle() );
	m_FileHandleMgr.CloseAllHandles( this, FileHandle() );
	m_FindHandleMgr.CloseAllHandles( this, FindHandle() );
}

bool PathFileMgr :: SetRoot( const char* root, bool autoResolve )
{
	// note: it would be wise to only allow this function to run from the local
	//       machine via registry or INI settings, and not expose it via script
	//       or world commands. otherwise local file security could be hacked.

	gpassert( !HasUsedHandles() );		// don't change root while handles are outstanding

	// take it
	m_RootPath = BuildRoot( root, autoResolve );

	// see if it actually exists
	bool success = true;
	if ( !m_RootPath.empty() )
	{
		// figure out its location
		if ( (m_RootLocation = FileSys::GetLocation( m_RootPath.c_str() )) != LOCATION_ERROR )
		{
			// extract root drive
			int offset;
			GetPathType( m_RootPath.c_str(), &offset );
			m_RootDrive.assign( m_RootPath, 0, offset );

			// make sure it's not backslash-terminated
			stringtool::RemoveTrailingBackslash( m_RootDrive );
		}
		else
		{
			gperrorf(( "PathFileMgr::SetRoot() - illegal root path '%s'\n", m_RootPath.c_str() ));
			::SetLastError( ERROR_PATH_NOT_FOUND );
			success = false;
		}
	}
	else
	{
		gperrorf(( "PathFileMgr::SetRoot() - could not connect to root path '%s' (%s)\n",
				      root ? root : "", stringtool::GetLastErrorText().c_str() ));
		success = false;
	}

	if ( !success )
	{
		// reset on error
		m_RootLocation = LOCATION_ERROR;
		m_RootPath .erase();
		m_RootDrive.erase();
	}

	// done
	return ( success );
}

gpstring PathFileMgr :: BuildRoot( const char* root, bool autoResolve )
{
	gpassert( root != NULL );			// be nice to me

	// default to current dir
	if ( *root == '\0' )
	{
		root = ".";
	}

	// attempt to resolve it if asked
	gpstring localRoot = root;
	if ( autoResolve )
	{
		localRoot = GetFullPathName( root );
		if ( localRoot.empty() )
		{
			// revert on failure
			localRoot = root;
		}
	}

	// make sure it's backslash-terminated and lowercase
	stringtool::AppendTrailingBackslash( localRoot );
	localRoot.to_lower();

	// see if it actually exists
	if ( !DoesPathExist( localRoot.c_str() ) )
	{
		// clear on failure
		localRoot.clear();
	}

	// return results
	return ( localRoot );
}

bool PathFileMgr :: HasUsedHandles( void ) const
{
	return (   m_FileHandleMgr.HasUsedHandles()
			|| m_MemHandleMgr .HasUsedHandles()
			|| m_FindHandleMgr.HasUsedHandles() );
}

bool PathFileMgr :: IsReady( void ) const
{
	// $ must successfully call SetRoot() before PathFileMgr will do anything.
	//   check IsReady() at all entry points that can't assume that it is ready.
	//   so any functions that take a handle don't need to bother.

	return ( m_RootLocation != LOCATION_ERROR );
}

bool PathFileMgr :: IsCopyingLocal( const gpstring& name ) const
{
	// this is special support for development systems where we run our
	//  resources off a VSS network shadow. any streaming files that are open
	//  will prevent checkins to VSS from updating the shadow. what this does
	//  is (if the option is enabled) PathFileMgr will, upon opening any file
	//  that is found on a network drive, create a memory buffer and copy it
	//  into there then close the net file. so files will only be locked for
	//  a short time while the app copies them local - this reduces the chance
	//  that a checkin shadow update will fail.

	bool is = false;
	gpassert( name.is_lower() );

	if ( TestOptions( OPTION_COPY_NET_LOCAL ) && (m_RootLocation & LOCATION_NETWORK) )
	{
		// ok now test the extension against registered copylocal types
		const CopyLocalColl& coll = GetCopyLocalTypes();
		if ( !coll.empty() )
		{
			CopyLocalColl::const_iterator i, ibegin = coll.begin(), iend = coll.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( (name.length() > i->length()) && same_with_case( name.end() - i->length(), i->c_str() ) )
				{
					is = true;
					break;
				}
			}
		}
		else
		{
			// if no specific types, then assume they are all copylocal
			is = true;
		}
	}

	return ( is );
}

#if !GP_RETAIL

void PathFileMgr :: Dump( ReportSys::ContextRef context ) const
{
	Critical::Lock critical( m_Critical );
	ReportSys::AutoReport autoReport( context );

	typedef stdx::linear_set <const MemEntry*> MemColl;
	typedef stdx::linear_map <const FileEntry*, MemColl> FileColl;

// Build map of file handles.

	FileColl files;
	MemHandleMgr::const_iterator mi, mbegin = m_MemHandleMgr.GetBegin(), mend = m_MemHandleMgr.GetEnd();
	for ( mi = mbegin ; mi != mend ; ++mi )
	{
		if ( m_MemHandleMgr.IsElementUsed( mi ) )
		{
			const FileEntry* fileEntry = m_FileHandleMgr.Dereference( mi->m_File );
			files[ fileEntry ].insert( &*mi );
		}
	}

// Dump them.

	MemColl memLeakCatcher;
	FileHandleMgr::const_iterator fi, fbegin = m_FileHandleMgr.GetBegin(), fend = m_FileHandleMgr.GetEnd();
	for ( fi = fbegin ; fi != fend ; ++fi )
	{
		if ( m_FileHandleMgr.IsElementUsed( fi ) )
		{
			// dump file handle info
			context->OutputF( "File: 0x%08X, Map: 0x%08X, View: 0x%08X, Size: %d, FP: %d",
							  fi->m_File, fi->m_Map, fi->m_View, fi->m_Size, fi->m_FilePointer );
#			if ( GP_DEBUG )
			{
				context->OutputF( ", Name: '%s'", fi->m_Name.c_str() );
			}
#			endif // GP_DEBUG
			context->Output( "\n" );

			// dump attached memory files
			MemColl& mems = files[ &*fi ];
			MemColl::const_iterator xi, xbegin = mems.begin(), xend = mems.end();
			for ( xi = xbegin ; xi != xend ; ++xi )
			{
				// dump mem handle info
				ReportSys::AutoIndent autoIndent( context );
				context->OutputF( "Mem: 0x%08X, Size: %d\n", (*xi)->m_Data, (*xi)->m_Size );

				// save it out
				memLeakCatcher.insert( *xi );
			}
		}
	}
	
// Dump leaks - where file was closed but mem handle was not.

	bool first = true;
	for ( mi = mbegin ; mi != mend ; ++mi )
	{
		if (   m_MemHandleMgr.IsElementUsed( mi )
			&& (memLeakCatcher.find( &*mi ) == memLeakCatcher.end()) )
		{
			if ( first )
			{
				first = false;
				context->Output( "\nLeaks detected!\n" );
			}

			// dump mem handle info
			ReportSys::AutoIndent autoIndent( context );
			context->OutputF( "Mem: 0x%08X, Size: %d", mi->m_Data, mi->m_Size );
#			if ( GP_DEBUG )
			{
				context->OutputF( ", Name: '%s'", mi->m_Name.c_str() );
			}
#			endif // GP_DEBUG
			context->Output( "\n" );
		}
	}
}

#endif // !GP_RETAIL

FileHandle PathFileMgr :: Open( const char* name, eUsage usage )
{
	// $ note on error handling in this function - i don't bother to check
	//   the return values from the close/unmap functions (like i do in
	//   PathFileMgr::Close) because this is a closed function where unmapping
	//   is unlikely to fail. i do check them in PathFileMgr::Close() because
	//   Bad Things have plenty of time to happen between an Open() and a
	//   Close().

	Critical::Lock critical( m_Critical );
	gpassert( name != NULL );	// parameter validation

// Analyze and build filename.

	gpstring fileName;
	eLocation location;
	if ( !ResolveFileName( fileName, &location, name ) )
	{
		// failed due to illegal path name or something - abort
		gperrorf(( "PathFileMgr::Open( %s ) - ResolveFileName() failed (%s)\n",
					  name, stringtool::GetLastErrorText().c_str() ));
		return ( FileHandle::Null );
	}

// Prep some vars.

	// acquire an empty handle
	FileHandle fileHandle;
	FileEntry* fileEntry = m_FileHandleMgr.Acquire( fileHandle );

	// init handle data
	fileEntry->m_File     = INVALID_HANDLE_VALUE;
	fileEntry->m_Map      = NULL;
	fileEntry->m_View     = NULL;
	fileEntry->m_Location = location;

	// remember the name in debug mode
	GPDEBUG_ONLY( fileEntry->m_Name = name );

	// setup some basic file parameters
	DWORD access   = GENERIC_READ | ((usage & USAGE_READWRITE) ? GENERIC_WRITE : 0);
	DWORD sharing  = FILE_SHARE_READ;
	DWORD creation = OPEN_EXISTING;
	DWORD flags    = FILE_ATTRIBUTE_NORMAL;
	DWORD error    = ERROR_SUCCESS;
	DWORD mapType  = (usage & USAGE_READWRITE) ? PAGE_WRITECOPY : PAGE_READONLY;
	DWORD viewType = (usage & USAGE_READWRITE) ? FILE_MAP_COPY : FILE_MAP_READ;

	// alloc these vars before 'goto'
	int offset = 0;
	const StringColl* aliases = NULL;

// Open the file, map and view.

	// check for aliases
	if ( (aliases = GetFileTypeAliases( fileName, &offset )) != NULL )
	{
		gpstring localFileName( fileName );

		// now loop over aliases
		StringColl::const_iterator i, ibegin = aliases->begin(), iend = aliases->end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// swap alias
			localFileName.replace( offset, gpstring::npos, *i );

			// attempt open
			fileEntry->m_File = GP_CreateFile( localFileName, access, sharing, creation, flags );
			if ( fileEntry->m_File != INVALID_HANDLE_VALUE )
			{
				fileName = localFileName;
				break;
			}
		}
	}

	// update flags for usage
	bool copyLocal = IsCopyingLocal( fileName );
	if ( copyLocal )
	{
		// if copying local then it will always be sequential (memcpy)
		flags |= FILE_FLAG_SEQUENTIAL_SCAN;
	}
	else
	{
		// modify for client requested usage
		flags |= (usage & USAGE_RANDOM) ? FILE_FLAG_RANDOM_ACCESS : FILE_FLAG_SEQUENTIAL_SCAN;
	}

	// update copying flags
	if ( usage & USAGE_READWRITE )
	{
		// $$$$$
	}

	// open the file directly
	if ( fileEntry->m_File == INVALID_HANDLE_VALUE )
	{
		fileEntry->m_File = GP_CreateFile( fileName, access, sharing, creation, flags );
		if ( fileEntry->m_File == INVALID_HANDLE_VALUE )  goto error;
	}

	// get its size
	fileEntry->m_Size = ::GetFileSize( fileEntry->m_File, NULL );
	if ( fileEntry->m_Size == 0xFFFFFFFF )  goto error;
	fileEntry->m_FilePointer = 0;

	// only bother this stuff if there's data out there
	if ( fileEntry->m_Size > 0 )
	{
		// open the map
		fileEntry->m_Map = GP_CreateFileMapping( fileEntry->m_File, mapType );
		if ( fileEntry->m_Map == NULL )  goto error;

		// map the view
		fileEntry->m_View = GP_MapViewOfFile( fileEntry->m_Map, viewType, 0, 0 );
		if ( fileEntry->m_View == NULL )  goto error;

		// special handling to prevent files being continuously locked over
		//  the network - if it's a vss-shadow dir then people won't be able
		//  to check files in that are streaming. so create a local pagefile-
		//  backed mapping to copy the file into, and then use that as the
		//  permanent mapping. this allows us to reuse the rest of the file
		//  map-based code.
		if ( copyLocal )
		{
			HANDLE localMap  = NULL;
			LPVOID localView = NULL;

			// create pagefile-backed memory buffer to copy file into
			localMap = GP_CreateFileMapping( INVALID_HANDLE_VALUE, PAGE_READWRITE, fileEntry->m_Size );
			if ( localMap == NULL )  goto localError;

			// map into it
			localView = GP_MapViewOfFile( localMap, FILE_MAP_WRITE, 0, 0);
			if ( localView == NULL )  goto localError;

			// copy file into it
			::memcpy( localView, fileEntry->m_View, fileEntry->m_Size );

			// remap the view as read-only for file system to use (optional)
			if ( !(usage & USAGE_READWRITE) )
			{
				gpverify( GP_UnmapViewOfFile( localView ) );
				localView = GP_MapViewOfFile( localMap, FILE_MAP_READ, 0, 0 );
				if ( localView == NULL )  goto localError;
			}

			// close old handles
			gpverify( GP_UnmapViewOfFile( fileEntry->m_View ) );
			gpverify( GP_CloseMapHandle ( fileEntry->m_Map  ) );
			gpverify( GP_CloseFileHandle( fileEntry->m_File ) );

			// now swap it out
			fileEntry->m_File = INVALID_HANDLE_VALUE;
			fileEntry->m_Map  = localMap;
			fileEntry->m_View = localView;

			// update its debug name
			GPDEBUG_ONLY( fileEntry->m_Name = "<pagefile>" );
			goto localOk;

		localError:

			// preserve error code
			error = ::GetLastError();

			// release win32 API handles
			if ( localView != NULL )
			{
				gpverify( GP_UnmapViewOfFile( localView ) );
			}
			if ( localMap != NULL )
			{
				gpverify( GP_CloseMapHandle ( localMap ) );
			}

			goto error;

		localOk:

			;  // just a jump target
		}
	}

	// all done
	return ( fileHandle );

// Error case.

error:

	// get error code unless already set
	if ( error == ERROR_SUCCESS )
	{
		error = ::GetLastError();
	}

	// release win32 API handles
	if ( fileEntry->m_View != NULL )
	{
		GP_UnmapViewOfFile( fileEntry->m_View );
	}
	if ( fileEntry->m_Map  != NULL )
	{
		GP_CloseMapHandle ( fileEntry->m_Map  );
	}
	if ( fileEntry->m_File != INVALID_HANDLE_VALUE )
	{
		GP_CloseFileHandle( fileEntry->m_File );
	}

	// release owned handle
	m_FileHandleMgr.Release( fileHandle );

	// failed... restore error code to original value and return error.
	::SetLastError( error );
	return ( FileHandle::Null );
}

FileHandle PathFileMgr :: Open( IndexPtr /*where*/, eUsage /*usage*/ )
{
	Critical::Lock critical( m_Critical );

	// $ IndexPtr's are not available (nor necessary) with the PathFileMgr so
	//   calling this function is a coding error on the client side. Call the
	//   other Open() instead.

	gpassertm( 0, "Not supported!" );

	::SetLastError( ERROR_NOT_SUPPORTED );
	return ( FileHandle::Null );
}

bool PathFileMgr :: Close( FileHandle file )
{
	Critical::Lock critical( m_Critical );

// Find and verify entry data.

	// get handle data
	FileEntry* fileEntry = file ? m_FileHandleMgr.Dereference( file ) : NULL;
	if ( fileEntry == NULL)  return ( false );

	// verify that all maps closed
#	if ( GP_DEBUG )
	{
		MemHandleMgr::const_iterator i, begin = m_MemHandleMgr.GetBegin(), end = m_MemHandleMgr.GetEnd();
		for ( i = begin ; i != end ; ++i )
		{
			if ( m_MemHandleMgr.IsElementUsed( i ) && (i->m_File == file) )
			{
				gperrorf(( "Closed a file handle ('%s') with memory maps still open!", i->m_Name.c_str() ));
			}
		}
	}
#	endif

// Ok close it off and free the handle.

	// $ note: only store the error from the first failure, otherwise later
	//         errors may mask the true problem. that is, if the view fails to
	//         unmap, then the map may fail to close due to the view still
	//         being open, etc.

	bool success = true;
	DWORD error = ERROR_SUCCESS;

	// the mapped view
	if (   ( fileEntry->m_View != NULL )
		&& !GP_UnmapViewOfFile( fileEntry->m_View ) )
	{
		if ( success )
		{
			error = ::GetLastError();
		}
		success = false;
	}

	// the file map
	if (   ( fileEntry->m_Map != NULL )
		&& !GP_CloseMapHandle( fileEntry->m_Map ) )
	{
		if ( success )
		{
			error = ::GetLastError();
		}
		success = false;
	}

	// the file (may be the pagefile)
	if (   ( ( fileEntry->m_File != NULL ) && ( fileEntry->m_File != INVALID_HANDLE_VALUE ) )
		&& !GP_CloseFileHandle( fileEntry->m_File ) )
	{
		if ( success )
		{
			error = ::GetLastError();
		}
		success = false;
	}

// Return results.

	// update error code
	if ( !success )
	{
		::SetLastError( error );
		gperrorf(( "PathFileMgr::Close() failed (%s)\n",
					  stringtool::GetLastErrorText( error ).c_str() ));
	}

	// free the handle
	m_FileHandleMgr.Release( file );

	// return success/failure, error stored in ::GetLastError()
	return ( success );
}

bool PathFileMgr :: Read( FileHandle file, void* out, int size, int* bytesRead )
{
	Critical::Lock critical( m_Critical );
	gpassert( ( out != NULL ) && ( size >= 0 ) );

// Find and verify entry data.

	// get handle data
	FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
	if ( fileEntry == NULL)  return ( false );

// Read in data.

	bool success = true;

	// clamp to max allowable size to read
	if ( clamp_max( size, (int)fileEntry->m_Size - (int)fileEntry->m_FilePointer ) )
	{
		// it's an error condition to read too much data
		success = false;
		::SetLastError( ERROR_INVALID_PARAMETER );
	}

	// only bother if something to do
	if ( size > 0 )
	{
		// "read" the data
		::memcpy( out, (const BYTE*)fileEntry->m_View + fileEntry->m_FilePointer, size );
		// adjust the "file pointer"
		fileEntry->m_FilePointer += size;
	}

// Done.

	// update output read data
	if ( bytesRead != NULL )
	{
		*bytesRead = size;
	}

	// success/fail
	return ( success );
}

bool PathFileMgr :: ReadLine( FileHandle file, gpstring& out )
{
	Critical::Lock critical( m_Critical );

// Find and verify entry data.

	// get handle data
	FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
	if ( fileEntry == NULL)  return ( false );

// Perform the read.

	// read the string
	const void* base = (const BYTE*)fileEntry->m_View + fileEntry->m_FilePointer;
	int size = fileEntry->m_Size - fileEntry->m_FilePointer;
	int skip = stringtool::ReadLine( base, size, out );

	// if didn't copy/skip anything, then say we reached EOF and return false
	if ( skip == 0 )
	{
		::SetLastError( ERROR_HANDLE_EOF );
		return ( false );
	}
	else
	{
		// update the file pointer
		fileEntry->m_FilePointer += skip;

		// done!
		return ( true );
	}
}

bool PathFileMgr :: SetReadWrite( FileHandle file, bool set )
{
	UNREFERENCED_PARAMETER( file );
	UNREFERENCED_PARAMETER( set );
	GP_Unimplemented$$$();
	return ( false );
}

bool PathFileMgr :: Seek( FileHandle file, int offset, eSeekFrom origin )
{
	Critical::Lock critical( m_Critical );

// Find and verify entry data.

	// get handle data
	FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
	if ( fileEntry == NULL)  return ( false );

// Perform seek.

	switch (origin)
	{
		case (SEEKFROM_BEGIN  ):  /* offset ok as is */                  break;
		case (SEEKFROM_CURRENT):  offset += fileEntry->m_FilePointer;    break;
		case (SEEKFROM_END    ):  offset  = fileEntry->m_Size - offset;  break;

		default:
		{
			gpassert( 0 );  // $ should never get here
			return ( false );
		}
	}

	bool success = true;

	// make sure it's in range
	if ( clamp_min_max( offset, 0, (int)fileEntry->m_Size ) )
	{
		// it's an error condition to set the file pointer out of range
		success = false;
		::SetLastError( ERROR_INVALID_PARAMETER );
	}

	// update file pointer
	fileEntry->m_FilePointer = offset;

// Done.

	// success/fail
	return ( success );
}

int PathFileMgr :: GetPos( FileHandle file )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
	if ( fileEntry == NULL)  return ( INVALID_POS_VALUE );

	// return pointer
	return ( fileEntry->m_FilePointer );
}

int PathFileMgr :: GetSize( FileHandle file )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
	if ( fileEntry == NULL)  return ( INVALID_SIZE_VALUE );

	// return size
	return ( fileEntry->m_Size );
}

eLocation PathFileMgr :: GetLocation( FileHandle file )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
	if ( fileEntry == NULL)  return ( LOCATION_ERROR );

	// return location
	return ( fileEntry->m_Location );
}

bool PathFileMgr :: GetFileTimes( FileHandle file, FILETIME* creation, FILETIME* lastAccess, FILETIME* lastWrite )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
	if ( fileEntry == NULL)  return ( false );

	// query times
	return ( ::GetFileTime( fileEntry->m_File, creation, lastAccess, lastWrite ) != FALSE );
}

MemHandle PathFileMgr :: Map( FileHandle file, int offset, int size )
{
	Critical::Lock critical( m_Critical );

	// verify that file exists
	FileEntry* fileEntry = file ? m_FileHandleMgr.Dereference( file ) : NULL;
	if ( fileEntry == NULL)  return ( MemHandle::Null );

// Prep some vars.

	// acquire an empty handle
	MemHandle memHandle;
	MemEntry* memEntry = m_MemHandleMgr.Acquire( memHandle );

	// init handle data
	memEntry->m_File = file;

	// remember the name in debug mode
	GPDEBUG_ONLY( memEntry->m_Name = fileEntry->m_Name );

// "Open" the map - really just some pointer addition.

	// set size if necessary
	if ( size == MAP_REST_OF_DATA )  // map rest of the file?
	{
		size = fileEntry->m_Size - offset;
	}

	// make sure our view is in range to check for potential access violations
	bool success = true;
	if ( clamp_min_max( size, 0, (int)fileEntry->m_Size ) )
	{
		success = false;
	}
	if ( clamp_min_max( offset, 0, (int)(fileEntry->m_Size - size) ) )
	{
		success = false;
	}
	if ( !success )
	{
		// error!
		::SetLastError( ERROR_INVALID_PARAMETER );
		gpassertm( 0, "Attempting to map data out of range" );

		// release owned handle
		m_MemHandleMgr.Release( memHandle );

		// failed...
		return ( MemHandle::Null );
	}

	// fill out remaining data
	memEntry->m_Data = (const BYTE*)fileEntry->m_View + offset;
	memEntry->m_Size = size;

	// return handle
	return ( memHandle );
}

bool PathFileMgr :: Close( MemHandle mem )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	MemEntry* memEntry = mem ? m_MemHandleMgr.Dereference( mem ) : NULL;
	if ( memEntry == NULL)  return ( false );

	// free the handle
	m_MemHandleMgr.Release( mem );

	// always ok
	return ( true );
}

int PathFileMgr :: GetSize( MemHandle mem )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	MemEntry* memEntry = m_MemHandleMgr.Dereference( mem );
	if ( memEntry == NULL)  return ( INVALID_SIZE_VALUE );

	// return size
	return ( memEntry->m_Size );
}

bool PathFileMgr :: Touch( MemHandle mem, int offset, int size )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	MemEntry* memEntry = m_MemHandleMgr.Dereference( mem );
	if ( memEntry == NULL)  return ( false );

	// set size if necessary
	if ( size == TOUCH_REST_OF_DATA )  // touch rest of view?
	{
		size = memEntry->m_Size - offset;
	}

	bool success = true;

	// make sure our memory is in range to check for potential access violations
	if ( clamp_min_max( size, 0, (int)memEntry->m_Size ) )
	{
		success = false;
		::SetLastError( ERROR_INVALID_PARAMETER );
		gpassertm( 0, "Attempting to touch data out of range" );
	}
	if ( clamp_min_max( offset, 0, (int)(memEntry->m_Size - size) ) )
	{
		success = false;
		::SetLastError( ERROR_INVALID_PARAMETER );
		gpassertm( 0, "Attempting to touch data out of range" );
	}

	// figure out if we're pagefile-backed memory
	bool isPageFile = false;
	HANDLE file = m_FileHandleMgr.Dereference( memEntry->m_File )->m_File;
	if ( file == INVALID_HANDLE_VALUE )
	{
		isPageFile = true;
	}

	// touch the pages
	MemTouchPages( (const BYTE*)memEntry->m_Data + offset, size, isPageFile );

	return ( success );
}

const void* PathFileMgr :: GetData( MemHandle mem, int offset )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	MemEntry* memEntry = m_MemHandleMgr.Dereference( mem );
	if ( memEntry == NULL)  return ( NULL );

	// return pointer - this will throw if deref'd out of range
	return ( (const BYTE*)memEntry->m_Data + offset );
}

FindHandle PathFileMgr :: Find( const char* pattern, eFindFilter filter )
{
	Critical::Lock critical( m_Critical );

// Analyze and build pattern.

	// default to all files
	if ( (pattern == NULL) || (*pattern == '\0') )
	{
		pattern = "*";
	}

	// convert to pathed name
	gpstring fullPattern;
	if ( !ResolveFileName( fullPattern, NULL, pattern ) )
	{
		// failed due to illegal path name or something - abort
		return ( FindHandle::Null );
	}

	// postfix a '*' if we really want a path
	if ( (*fullPattern.rbegin() == '\\') || (*fullPattern.rbegin() == '/') )
	{
		fullPattern += '*';
	}

// Prep some vars.

	// acquire an empty handle
	FindHandle findHandle;
	FindEntry* findEntry = m_FindHandleMgr.Acquire( findHandle );

	// copy spec in
	findEntry->m_Pattern = pattern;
	findEntry->m_Filter  = filter;

	// must be lower case
	findEntry->m_Pattern.to_lower();

// Start the find going.

	// start up with first file
	if ( !findEntry->First( fullPattern.c_str() ) )
	{
		DWORD error = ::GetLastError();				// preserve error code
		findEntry->Close();							// release win32 API handle
		m_FindHandleMgr.Release( findHandle );		// release owned handle
		findHandle = FindHandle::Null;				// null it out
		::SetLastError( error );					// restore error code

		if ( (error != ERROR_FILE_NOT_FOUND) && (error != ERROR_PATH_NOT_FOUND) )
		{
			gperrorf(( "PathFileMgr::Find( %s ) failed (%s)\n",
						  pattern, stringtool::GetLastErrorText( error ).c_str() ));
		}
	}

	// all done
	return ( findHandle );
}

FindHandle PathFileMgr :: Find( FindHandle base, const char* pattern, eFindFilter filter )
{
	// a null base means root $ early bailout
	if ( !base.IsFinding() )
	{
		return ( Find( pattern, filter ) );
	}

	Critical::Lock critical( m_Critical );

	// default to all files
	if ( (pattern == NULL) || (*pattern == '\0') )
	{
		pattern = "*";
	}

	// get handle data
	FindEntry* findEntry = m_FindHandleMgr.Dereference( base );
	if ( findEntry == NULL)  return ( FindHandle::Null );

	// build a new search spec based on old one by replacing the tail end
	gpstring newPattern = findEntry->m_Pattern;
	size_t tail = newPattern.find_last_of( "\\/" );
	if ( tail != gpstring::npos )
	{
		newPattern.erase( tail + 1 );
		newPattern.append( pattern );
	}
	else
	{
		// it was in the root before
		newPattern = pattern;
	}

	// must be lowercase
	newPattern.to_lower();

	// now do the find to get a spanky new handle
	return ( Find( newPattern.c_str(), filter ) );
}

bool PathFileMgr :: GetNext( FindHandle find, gpstring& out, bool prependPath )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	FindEntry* findEntry = m_FindHandleMgr.Dereference( find );
	if ( findEntry == NULL)  return ( false );

	// find next
	if ( !findEntry->Next( prependPath ) )
	{
		return ( false );
	}

	// copy it over
	out = findEntry->m_FindData.cFileName;

	// return success
	return ( true );
}

bool PathFileMgr :: GetNext( FindHandle find, FindData& out, bool prependPath )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	FindEntry* findEntry = m_FindHandleMgr.Dereference( find );
	if ( findEntry == NULL)  return ( false );

	// find next
	if ( !findEntry->Next( prependPath ) )
	{
		return ( false );
	}

	// copy it over
	out.Fill( findEntry->m_FindData );

	// set its name to be the path
#	if !GP_RETAIL
	out.m_Description = "[P]" + GetRootPath();
#	endif // !GP_RETAIL

	// return success
	return ( true );
}

bool PathFileMgr :: HasNext( FindHandle find )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	FindEntry* findEntry = m_FindHandleMgr.Dereference( find );
	if ( findEntry == NULL)  return ( false );

	// return if we found somethin
	return ( findEntry->Next( false ) );
}

bool PathFileMgr :: Close( FindHandle find )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	FindEntry* findEntry = find ? m_FindHandleMgr.Dereference( find ) : NULL;
	if ( findEntry == NULL)  return ( false );

	// close the handle and free it
	BOOL rc = GP_FindClose( findEntry->m_Find );
	m_FindHandleMgr.Release( find );

	// report an error if one
	if ( !rc )
	{
		gperrorf(( "PathFileMgr::Close() - FindClose() failed (%s)\n",
				      stringtool::GetLastErrorText().c_str() ));
	}

	// return success/failure, error stored in ::GetLastError()
	return ( rc != FALSE );
}

bool PathFileMgr :: QueryFileInfo( IndexPtr where, FindData& out )
{
	Critical::Lock critical( m_Critical );
	bool success = false;

	// $ note that GetFileAttributesEx() would probably be more efficient here
	//   but it's not supported on Win95 (98/NT only).

	// get the filename
	const char* queryName;
	where.GetValidPointer( queryName );

	// fully qualify it
	gpstring fileName;
	if ( ResolveFileName( fileName, NULL, queryName ) )
	{
		// get info on it
		FileFinder finder( fileName.c_str() );
		if ( finder.Next() )
		{
			out.Fill( finder.Get() );
			success = true;

			// set its name to be the path
#			if !GP_RETAIL
			out.m_Description = "[P]" + GetRootPath();
#			endif // !GP_RETAIL
		}
	}

	return ( success );
}

bool PathFileMgr :: ResolveFileName( gpstring& resolved, eLocation* location, const char* name )
{
	// figure out what we're dealing with
	ePathType pathType = GetPathType( name );

	// discard illegal requests
	if ( (pathType != PATHTYPE_RELATIVE_NORMAL) && !TestOptions( OPTION_EXTERNAL_ADDRESSING ) )
	{
		::SetLastError( ERROR_INVALID_PARAMETER );
		return ( false );
	}

	// if relative path then we must have a root path
	if ( IsRelative( pathType ) && !IsReady() )
	{
		// default to current
		SetRoot( "." );
	}

	// build the filename (prefix with root dir)
	switch ( pathType )
	{
		case ( PATHTYPE_RELATIVE_NORMAL ):
		case ( PATHTYPE_RELATIVE_PARENT ):
		{
			// ordinary relative pathing - just prepend root
			resolved = m_RootPath;
			resolved += name;

			if ( location != NULL )
			{
				*location = m_RootLocation;
			}
		}	break;

		case ( PATHTYPE_RELATIVE_DRIVE ):
		{
			// special case - only need drive name
			resolved = m_RootDrive + name;
			if ( location != NULL )
			{
				*location = m_RootLocation;
			}
		}	break;

		case ( PATHTYPE_RELATIVE_PATH ):
		case ( PATHTYPE_ILLEGAL ):
		{
			// we never support this type - too easy to not get what you want
			::SetLastError( ERROR_INVALID_PARAMETER );
			return ( false );
		}	break;

		default:
		{
			// assume absolute
			resolved = name;

			// depending on option choose how to find its location (using Win32 func is slower)
			if ( location != NULL )
			{
				if ( TestOptions( OPTION_QUERY_ABS_LOCATIONS ) )
				{
					*location = FileSys::GetLocation( name );
				}
				else
				{
					*location = m_RootLocation;
				}
			}
		}	break;
	}

	// always lower case
	resolved.to_lower();

	// must be ok
	return ( true );
}

void PathFileMgr :: PrivateInit( void )
{
	m_RootLocation = LOCATION_ERROR;
	m_Options      = OPTION_QUERY_ABS_LOCATIONS;

#	if ( !GP_RETAIL )
	{
		// external addressing ok in development modes
		SetOptions( OPTION_EXTERNAL_ADDRESSING );
	}
#	endif
}

//////////////////////////////////////////////////////////////////////////////
// class PathFileMgr::FindEntry implementation

bool PathFileMgr::FindEntry :: First( const char* pattern )
{
	gpassert( (pattern != NULL) && (*pattern != '\0') );
	bool success = false;

	// this will fail if the directory does not exist
	m_Find = GP_FindFirstFile( pattern, &m_FindData );
	if ( m_Find != INVALID_HANDLE_VALUE )
	{
		// $ dir may exist but not hold files that we want (that's ok)
		success = true;

		_strlwr( m_FindData.cFileName );
		if ( IsMatch() )
		{
			// data already available next time around
			m_Mode = MODE_FIRST;
		}
		else
		{
			// if it's not a match then need to keep looking
			m_Mode = MODE_NORMAL;
			if ( Next( false ) )
			{
				// data already available next time around
				m_Mode = MODE_FIRST;
			}
			else if ( ::GetLastError() == ERROR_NO_MORE_FILES )
			{
				m_Mode = MODE_EMPTY;
			}
			else
			{
				success = false;
			}
		}
	}

	return ( success );
}

bool PathFileMgr::FindEntry :: Next( bool prependPath )
{
	bool success = false;

	if ( m_Mode == MODE_FIRST )
	{
		// data is already there - use it
		m_Mode = MODE_NORMAL;
		success = true;
	}
	else if ( m_Mode == MODE_EMPTY )
	{
		// nothing there
		::SetLastError( ERROR_NO_MORE_FILES );
	}
	else
	{
		// keep looking until we find a filtered match
		gpassert( m_Find != INVALID_HANDLE_VALUE );
		while ( GP_FindNextFile( m_Find, &m_FindData ) )
		{
			_strlwr( m_FindData.cFileName );
			if ( IsMatch() )
			{
				success = true;
				break;
			}
		}
	}

	// optionally modify filename to include relative path prefix
	if ( success && prependPath )
	{
		// look for path terminator
		size_t tail = m_Pattern.find_last_of( "\\/" );

		// if there isn't one then it's in the root and don't need to prefix
		if ( tail != gpstring::npos )
		{
			// win32 file system should never allow going past MAX_PATH
			++tail;
			gpassert( (tail + ::strlen( m_FindData.cFileName ) + 1) <= MAX_PATH );

			// move the filename over and copy the prefix in
			::memmove( m_FindData.cFileName + tail,
					   m_FindData.cFileName,
					   ::strlen( m_FindData.cFileName ) + 1 );
			::memcpy( m_FindData.cFileName, m_Pattern.c_str(), tail );
		}
	}

	return ( success );
}

bool PathFileMgr::FindEntry :: Close( void )
{
	bool rc = true;

	if ( m_Find != INVALID_HANDLE_VALUE )
	{
		rc = !!GP_FindClose( m_Find );
		m_Find = INVALID_HANDLE_VALUE;
	}

	return ( rc );
}

// this function checks to see if what we're filtering on makes the current
//  contents of m_FindData a match. it also filters out '.' and '..' for
//  client convenience and consistency across file systems.

bool PathFileMgr::FindEntry :: IsMatch( void ) const
{
	bool currentIsDir = !!(m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

	// early out on metafiles
	if ( !currentIsDir && (m_FindData.cFileName[ 0 ] == '$') )
	{
		return ( false );
	}

	// check basic type match
	bool match = currentIsDir ? !!(m_Filter & FINDFILTER_DIRECTORIES)
							  : !!(m_Filter & FINDFILTER_FILES);
	if ( match )
	{
		// special dir check
		if ( currentIsDir )
		{
			char c0 = m_FindData.cFileName[ 0 ],
			     c1 = m_FindData.cFileName[ 1 ],
			     c2 = m_FindData.cFileName[ 2 ];

			// remove '.' and '..' from list
			if (   (c0 == '.')
				&& (    (c1 == '\0')
				    || ((c1 == '.') && (c2 == '\0')) ) )
			{
				return ( false );
			}
		}

		// check match, but only if necessary
		if ( !m_Pattern.empty() )
		{
			// $ note: the extra wildcard checking is required because windows
			//   apparently only checks the first three characters of a file
			//   extension. so if you look for *.nnk or *.nn? or even *.??? and
			//   there is a file abc.nnkxyz then windows will match it. so we
			//   have to do an extra level of checking here, and this code is
			//   almost identical to the TankFileMgr's code as a result.
			//
			// [this is now in the wiki at http://engrtest/wiki/?BugsInWindows]

			// paranoia check to see that we're lowercase here
			gpassert( m_Pattern.is_lower() );
			gpassert( gpstring( m_FindData.cFileName ).is_lower() );

			// check match
			match = stringtool::IsDosWildcardMatch( FileSys::GetFileName( m_Pattern ), m_FindData.cFileName, true );
		}
	}

	return ( match );
}

//////////////////////////////////////////////////////////////////////////////

}	// end of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
