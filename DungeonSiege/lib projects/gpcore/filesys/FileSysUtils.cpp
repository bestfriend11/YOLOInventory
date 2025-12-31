///////////////////////////////////////////////////////////////////////////////
//
// File     :  FileSysUtils.cpp (GPCore:FileSys)
// Author(s):  Scott Bilas
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "FileSysUtils.h"

#include "Config.h"
#include "GpStatsDefs.h"
#include "StdHelp.h"
#include "StringTool.h"
#include "kerneltool.h"

#define TRACK_RESOURCES     0		// turn on resource tracking (used to be !GP_RETAIL)
#define LOG_FILE_OPERATIONS 0		// log every file operation to disk

#if TRACK_RESOURCES
#include "ReportSys.h"
#include <map>
#endif // TRACK_RESOURCES

namespace FileSys  {  // begin of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
// File resource tracking - for finding resource leaks

// call the wrapper functions in FileSysUtils.h to track handles

// note: the Tracker will (when enabled) free up all dependent maps and views
//       if the higher-level handle is closed off. this functionality should
//       not be relied upon as it won't exist in the retail build. it's only
//       there for development modes so that a programming error won't require
//       a Win9x-based developer to reboot after running the game.

#if TRACK_RESOURCES

#if LOG_FILE_OPERATIONS
#	define WRITE_LOG( x ) WriteLog x
#else // LOG_FILE_OPERATIONS
#	define WRITE_LOG( x )
#endif // LOG_FILE_OPERATIONS

class Tracker
{
public:

// Ctor/dtor.

			 Tracker( void )  {  }
			~Tracker( void );

// Tracking functions.

	void CreateFile       ( HANDLE hfile, LPCTSTR name, DWORD access, DWORD share, DWORD creation, DWORD flags );
	void CreateFile       ( HANDLE hfile );
	void CloseFileHandle  ( HANDLE hfile );
	void CreateFileMapping( HANDLE hmap, HANDLE hfile, DWORD protect, DWORD maxSize, LPCTSTR name );
	void CloseMapHandle   ( HANDLE hmap );
	void MapViewOfFile    ( LPVOID view, HANDLE hmap, DWORD access, DWORD offset, DWORD bytes );
	void UnmapViewOfFile  ( LPVOID view );
	void Dump             ( ReportSys::ContextRef = NULL ) const;

#	if LOG_FILE_OPERATIONS
	void WriteLog         ( const char* format, ... );
#	endif // LOG_FILE_OPERATIONS

private:

// Database entry types.

	struct View
	{
		DWORD m_Offset;
		DWORD m_Size;
		int   m_RefCount;
	};
	typedef std::map <LPVOID, View>  ViewColl;

	struct Map
	{
		DWORD    m_MaxSize;
		ViewColl m_Views;
	};
	typedef std::map <HANDLE, Map>  MapColl;

	struct File
	{
		gpstring m_Name;
		MapColl  m_Maps;
	};
	typedef std::map <HANDLE, File> FileColl;

	// reverse indexes
	typedef std::map <HANDLE, HANDLE> MapIndex;
	typedef std::map <LPVOID, HANDLE> ViewIndex;

// Lookup functions.

	bool FindFileInDb   ( HANDLE hfile, FileColl ::iterator& fileFound );
	bool FindMapInDb    ( HANDLE hmap,  FileColl ::iterator& fileFound, MapColl::iterator& mapFound );
	bool FindMapInIndex ( HANDLE hmap,  MapIndex ::iterator& imapFound );
	bool FindViewInDb   ( LPVOID view,  MapColl  ::iterator& mapFound, ViewColl::iterator& viewFound );
	bool FindViewInIndex( LPVOID view,  ViewIndex::iterator& iviewFound );

// Database and indexes.

	FileColl  m_Files;		// main db of entries
	MapIndex  m_Maps;		// reverse index of maps to files
	ViewIndex m_Views;		// reverse index of views to maps
};

Tracker :: ~Tracker( void )
{
	if ( !m_Files.empty() )
	{
		ReportSys::DebuggerSink sink;
		ReportSys::LocalContext ctx( &sink, false );
		Dump( ctx );
	}
}

void Tracker :: CreateFile( HANDLE hfile, LPCTSTR name, DWORD access,
							DWORD share, DWORD creation, DWORD flags )
{
	UNREFERENCED_PARAMETER( access );
	UNREFERENCED_PARAMETER( share );
	UNREFERENCED_PARAMETER( creation );
	UNREFERENCED_PARAMETER( flags );

	WRITE_LOG(( "0x%08X =        CreateFile( \"%s\", 0x%08X, 0x%08X, 0x%08X, 0x%08X )",
				hfile, name ? name : "<NULL>", access, share, creation, flags ));

	// skip invalid handles - they are the result of a failed open and are ok
	if ( hfile != INVALID_HANDLE_VALUE )
	{
		// insert in database
		gpassertm( m_Files.find( hfile ) == m_Files.end(), "Duplicate file handle entry");
		m_Files[ hfile ].m_Name = ( name == NULL ) ? "" : name;
	}
}

void Tracker :: CreateFile( HANDLE hfile )
{
	WRITE_LOG(( "0x%08X =        CreateFile( ??? )", hfile ));

	// skip invalid handles - they are the result of a failed open and are ok
	if ( hfile != INVALID_HANDLE_VALUE )
	{
		// insert in database
		gpassertm( m_Files.find( hfile ) == m_Files.end(), "Duplicate file handle entry");
		m_Files[ hfile ];
	}
}

void Tracker :: CloseFileHandle( HANDLE hfile )
{
	WRITE_LOG(( "               CloseFileHandle( 0x%08X )", hfile ));

	gpassert( hfile != INVALID_HANDLE_VALUE );
	FileColl::iterator fileFound;
	if ( FindFileInDb( hfile, fileFound ) )
	{
		// safety: don't leak handles
		MapColl& maps = fileFound->second.m_Maps;
		if ( !maps.empty() )
		{
			gpassertm( 0, "File was closed with maps still open" );
			MapColl::iterator i, begin = maps.begin(), end = maps.end();
			for ( i = begin ; i != end ; ++i )
			{
				CloseMapHandle( i->first );
			}
		}

		// remove from database
		m_Files.erase( fileFound );
	}
	else
	{
		gpassertm( 0, "Untracked file was closed" );
	}
}

void Tracker :: CreateFileMapping( HANDLE hmap, HANDLE hfile, DWORD protect,
									 DWORD maxSize, LPCTSTR name )
{
	UNREFERENCED_PARAMETER( protect );
	UNREFERENCED_PARAMETER( name );

	WRITE_LOG(( "0x%08X = CreateFileMapping( 0x%08X, 0x%08X, 0x%08X, %d, %s )",
				hmap, hfile, protect, maxSize, name ? name : "<NULL>" ));

	if ( hmap != NULL )
	{
		if ( hfile == INVALID_HANDLE_VALUE )
		{
			m_Files[ hfile ]; 	// force pagefile into file array
		}

		FileColl::iterator fileFound;
		if ( FindFileInDb( hfile, fileFound ) )
		{
			// insert in database
			MapColl& maps = fileFound->second.m_Maps;
			gpassertm( maps.find( hmap ) == maps.end(), "Duplicate map handle entry" );
			maps[ hmap ].m_MaxSize = maxSize;
		}

		// insert in index
		gpassertm( m_Maps.find( hmap ) == m_Maps.end(), "Duplicate map handle entry" );
		m_Maps[ hmap ] = hfile;
	}
}

void Tracker :: CloseMapHandle( HANDLE hmap )
{
	WRITE_LOG(( "                CloseMapHandle( 0x%08X )", hmap ));

	// find by index
	MapIndex::iterator imapFound;
	if ( FindMapInIndex( hmap, imapFound ) )
	{
		// find in db
		FileColl::iterator fileFound;
		MapColl::iterator mapFound;
		if (   FindFileInDb( imapFound->second, fileFound )
			&& FindMapInDb ( hmap, fileFound, mapFound ) )
		{
			// safety: don't leak handles
			ViewColl& views = mapFound->second.m_Views;
			if ( !views.empty() )
			{
				gpassertm( 0, "Map was closed with views still open" );
				ViewColl::iterator i, begin = views.begin(), end = views.end();
				for ( i = begin ; i != end ; ++i )
				{
					UnmapViewOfFile( i->first );
				}
			}

			// remove from database
			fileFound->second.m_Maps.erase( mapFound );

			// special for pagefile
			if ( ( fileFound->first == INVALID_HANDLE_VALUE ) && fileFound->second.m_Maps.empty() )
			{
				m_Files.erase( fileFound );
			}
		}

		// remove from index
		m_Maps.erase( imapFound );
	}
	else
	{
		gpassertm( 0, "Untracked map was closed" );
	}
}

void Tracker :: MapViewOfFile( LPVOID view, HANDLE hmap, DWORD access, DWORD offset, DWORD bytes )
{
	UNREFERENCED_PARAMETER( access );

	WRITE_LOG(( "0x%08X =     MapViewOfFile( 0x%08X, 0x%08X, %d, %d )",
				view, hmap, access, offset, bytes ));

	gpassert( hmap != NULL );
	if ( view != NULL )
	{
		MapIndex::iterator imapFound;
		FileColl::iterator fileFound;
		MapColl ::iterator mapFound;
		if (   FindMapInIndex( hmap, imapFound )
			&& FindFileInDb  ( imapFound->second, fileFound )
			&& FindMapInDb   ( hmap, fileFound, mapFound ) )
		{
			// build entry
			View entry;
			entry.m_Offset = offset;
			entry.m_Size = bytes;
			entry.m_RefCount = 1;

			// insert in database
			std::pair <ViewColl::iterator, bool> rc
					= mapFound->second.m_Views.insert( std::make_pair( view, entry ) );
			if ( !rc.second )
			{
				// already there - adjust for new view that overlaps
				View& existing = rc.first->second;
				existing.m_Size = max( existing.m_Size, entry.m_Size );
				++existing.m_RefCount;
			}
		}

		// insert in index
#		if GP_DEBUG
		ViewIndex::iterator dupCheck = m_Views.find( view );
		if ( dupCheck != m_Views.end() )
		{
			gpassertm( dupCheck->second == hmap, "Duplicate view ptr map handle mismatch" );
		}
#		endif // GP_DEBUG

			// ok, now really insert it this time
		m_Views[ view ] = hmap;
	}
}

void Tracker :: UnmapViewOfFile( LPVOID view )
{
	WRITE_LOG(( "               UnmapViewOfFile(0x%08X)", view ));

	// find by index
	ViewIndex::iterator iviewFound;
	if ( FindViewInIndex( view, iviewFound ) )
	{
		bool remove = true;

		// find in db
		MapIndex::iterator imapFound;
		FileColl::iterator fileFound;
		MapColl ::iterator mapFound;
		ViewColl::iterator viewFound;
		if (   FindMapInIndex( iviewFound->second, imapFound )
			&& FindFileInDb  ( imapFound->second, fileFound )
			&& FindMapInDb   ( imapFound->first, fileFound, mapFound )
			&& FindViewInDb  ( view, mapFound, viewFound ) )
		{
			// dec ref count and optionally remove from database
			View& existing = viewFound->second;
			if ( --existing.m_RefCount == 0 )
			{
				mapFound->second.m_Views.erase( viewFound );
			}
			else
			{
				remove = false;
			}
		}

		// remove from index
		if ( remove )
		{
			m_Views.erase( iviewFound );
		}
	}
	else
	{
		gpassertm( 0, "Untracked view was closed" );
	}
}

void Tracker :: Dump( ReportSys::ContextRef ctx ) const
{
	if ( m_Files.empty() )
	{
		gpgeneric( "FileSys::Tracker: no files being tracked!\n" );
		return;
	}

	ReportSys::AutoReport autoReport( ctx );
	ctx->Output( "--- Dumping FileSys::Tracker --- \n\n" );

	FileColl::const_iterator i, begin = m_Files.begin(), end = m_Files.end();
	for ( i = begin ; i != end ; ++i )
	{
		const MapColl& maps = i->second.m_Maps;

		const char* name = "<pagefile>";
		if ( i->first != INVALID_HANDLE_VALUE )
		{
			name = i->second.m_Name;
		}
		ctx->OutputF( "### File still open: H = 0x%08X '%s' (%d open maps)\n",
						i->first, name, maps.size() );

		MapColl::const_iterator j, jbegin = maps.begin(), jend = maps.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			const ViewColl& views = j->second.m_Views;

			ctx->OutputF( "  - Map still open: H = 0x%08X, MaxSize = %d (%d open views)\n",
							j->first, j->second.m_MaxSize, views.size() );

			ViewColl::const_iterator k, kbegin = views.begin(), kend = views.end();
			for ( k = kbegin ; k != kend ; ++k )
			{
				ctx->OutputF( "    - View still open: Addr = 0x%08X, Offset = 0x%08X, Size = %d, RefCount = %d\n",
								k->first, k->second.m_Offset, k->second.m_Size, k->second.m_RefCount );
			}
		}
	}
	ctx->Output( "\n--- Done --- \n" );
}

#if LOG_FILE_OPERATIONS

void Tracker :: WriteLog( const char* format, ... )
{
	static bool sNewSession = true;
	if (sNewSession)
	{
		sNewSession = false;
		WriteLog("--- NEW SESSION ---");
	}

	FILE* file = ::fopen( "file_operations.log", "at" );
	if ( file )
	{
		auto_dynamic_vsnprintf printer( format, va_args( format ) );

		time_t ltime;
		::time( &ltime );
		char timebuf[ 100 ];
		::strcpy( timebuf, ctime( &ltime ) );
		int i = ::strlen( timebuf );
		if ( i > 0 )
		{
			timebuf[ i - 1 ] = '\0';       // remove trailing '\n'
		}

		::fprintf( file, "%s: %s\n", timebuf, printer.c_str() );
		::fclose( file );
	}
}

#endif // LOG_FILE_OPERATIONS

bool Tracker :: FindFileInDb( HANDLE hfile, FileColl::iterator& fileFound )
{
	fileFound = m_Files.find( hfile );
	gpassertm( fileFound != m_Files.end(), "Cannot find file in tracker db" );
	return (fileFound != m_Files.end());
}

bool Tracker :: FindMapInDb( HANDLE hmap, FileColl::iterator& fileFound, MapColl::iterator& mapFound )
{
	MapColl& maps = fileFound->second.m_Maps;
	mapFound = maps.find( hmap );
	gpassertm( mapFound != maps.end(), "Cannot find map in tracker db" );
	return ( mapFound != maps.end() );
}

bool Tracker :: FindMapInIndex( HANDLE hmap, MapIndex::iterator& imapFound )
{
	imapFound = m_Maps.find( hmap );
	gpassertm( imapFound != m_Maps.end(), "Cannot find map in tracker index" );
	return ( imapFound != m_Maps.end() );
}

bool Tracker :: FindViewInDb( LPVOID view, MapColl::iterator& mapFound, ViewColl::iterator& viewFound )
{
	ViewColl& views = mapFound->second.m_Views;
	viewFound = views.find( view );
	gpassertm( viewFound != views.end(), "Cannot find view in tracker db" );
	return ( viewFound != views.end() );
}

bool Tracker :: FindViewInIndex( LPVOID view, ViewIndex::iterator& iviewFound )
{
	iviewFound = m_Views.find( view );
	gpassertm( iviewFound != m_Views.end(), "Cannot find view in tracker index" );
	return ( iviewFound != m_Views.end() );
}

#undef WRITE_LOG

DEFINE_POST_GLOBAL( Tracker, Tracker );
#define gTracker GetTracker()

#endif // TRACK_RESOURCES



//////////////////////////////////////////////////////////////////////////////
// Win32 file function wrappers

HANDLE GP_CreateFile( LPCTSTR lpFileName,
						DWORD   dwDesiredAccess,
						DWORD   dwShareMode,
						DWORD   dwCreationDisposition,
						DWORD   dwFlagsAndAttributes )
{
	HANDLE h = ::CreateFile( lpFileName, dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, dwFlagsAndAttributes, NULL );
#	if TRACK_RESOURCES
	gTracker.CreateFile( h, lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, dwFlagsAndAttributes );
#	endif // TRACK_RESOURCES
	return ( h );
}

BOOL GP_CloseFileHandle( HANDLE hFile )
{
#	if TRACK_RESOURCES
	gTracker.CloseFileHandle( hFile );
#	endif // TRACK_RESOURCES
	return ( ::CloseHandle( hFile ) );
}

HANDLE GP_CreateFileMapping( HANDLE hFile,
							 DWORD flProtect,
							 DWORD dwMaximumSize,
							 LPCTSTR lpName )
{
	HANDLE h = ::CreateFileMapping( hFile, NULL, flProtect, 0, dwMaximumSize, lpName );
#	if TRACK_RESOURCES
	gTracker.CreateFileMapping( h, hFile, flProtect, dwMaximumSize, lpName );
#	endif // TRACK_RESOURCES
	return ( h );
}

BOOL GP_CloseMapHandle( HANDLE hMap )
{
#	if TRACK_RESOURCES
	gTracker.CloseMapHandle( hMap );
#	endif // TRACK_RESOURCES
	return ( ::CloseHandle( hMap ) );
}

LPVOID GP_MapViewOfFile( HANDLE hFileMappingObject,
						 DWORD dwDesiredAccess,
						 DWORD dwFileOffset,
						 DWORD dwNumberOfBytesToMap )
{
	LPVOID d = ::MapViewOfFile( hFileMappingObject, dwDesiredAccess, 0, dwFileOffset, dwNumberOfBytesToMap );
#	if TRACK_RESOURCES
	gTracker.MapViewOfFile( d, hFileMappingObject, dwDesiredAccess, dwFileOffset, dwNumberOfBytesToMap );
#	endif // TRACK_RESOURCES
	return ( d );
}

BOOL GP_UnmapViewOfFile( LPCVOID lpBaseAddress )
{
#	if TRACK_RESOURCES
	gTracker.UnmapViewOfFile( ccast <LPVOID> ( lpBaseAddress ) );
#	endif // TRACK_RESOURCES
	return ( ::UnmapViewOfFile( lpBaseAddress ) );
}

// $ note for all GP_* below - no need to track this stuff, just do indicators.

BOOL GP_ReadFile( HANDLE hFile,
				  LPVOID lpBuffer,
				  DWORD nNumberOfBytesToRead,
				  LPDWORD lpNumberOfBytesRead,
				  LPOVERLAPPED lpOverlapped )
{
	GP_Indicate( FILEINDICATOR_READWRITE, nNumberOfBytesToRead );
	GPSTATS_SAMPLE( AddFileRead( nNumberOfBytesToRead ) );

#if !GP_RETAIL

	kerneltool::EnterFileOp();

	BOOL bRet=  ::ReadFile( hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped ) ;

	kerneltool::LeaveFileOp();

	return bRet;
#else

	return ( ::ReadFile( hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped ) );
#endif

}

BOOL GP_WriteFile( HANDLE hFile,
				   LPCVOID lpBuffer,
				   DWORD nNumberOfBytesToWrite,
				   LPDWORD lpNumberOfBytesWritten,
				   LPOVERLAPPED lpOverlapped )
{
	GP_Indicate( FILEINDICATOR_READWRITE, nNumberOfBytesToWrite );
	return ( ::WriteFile( hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped ) );
}

HANDLE GP_FindFirstFile( LPCTSTR lpFileName,
						 LPWIN32_FIND_DATA lpFindFileData )
{
	GP_Indicate( FILEINDICATOR_INDEX );
	return ( ::FindFirstFile( lpFileName, lpFindFileData ) );
}

BOOL GP_FindNextFile( HANDLE hFindFile,
						LPWIN32_FIND_DATA lpFindFileData )
{
	GP_Indicate( FILEINDICATOR_INDEX );
	return ( ::FindNextFile( hFindFile, lpFindFileData ) );
}

BOOL GP_FindClose( HANDLE hFindFile )
{
	GP_Indicate( FILEINDICATOR_INDEX );
	return ( ::FindClose( hFindFile ) );
}

//////////////////////////////////////////////////////////////////////////////
// Fun stuff

// this dandy little function makes blinky lights on the screen so we know
//  when we're hitting the file system.

#if !GP_RETAIL

void GP_Indicate( eFileIndicator indicator, int count )
{
	// this is not implemented yet

	UNREFERENCED_PARAMETER( indicator );
	UNREFERENCED_PARAMETER( count );

#	if 0

	static AsyncLight sReadWrite(
			RGB( 213,  31,  61 ),		// color
			CRect( 6, 21, 21, 26 ),		// rect to flash
			50000 );					// increments required to flash
	static AsyncLight sOpenClose(
			RGB( 233, 149,  17 ),		// color
			CRect( 6, 29, 21, 34 ),		// rect to flash
			10 );						// increments required to flash
	static AsyncLight sMapUnmap(
			RGB( 192,   0, 192 ),  		// color
			CRect( 4, 21, 6, 26 ), 		// rect to flash
			1 );                   		// increments required to flash
	static AsyncLight sIndex(
			RGB( 58,  17, 233 ),   		// color
			CRect( 6, 37, 21, 42 ),		// rect to flash
			50 );                  		// increments required to flash

	static AsyncLight* sLights[] =
	{
		&sOpenClose, &sMapUnmap, &sReadWrite, &sIndex,
	};

	sLights[ indicator ]->Add(count);

#	endif // 0
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// Helper function implementations

#define FILESYSUTILSA
#include "FileSysUtilsAW.cpp"
#undef FILESYSUTILSA
#define FILESYSUTILSW
#include "FileSysUtilsAW.cpp"
#undef FILESYSUTILSW

bool IsDirWritable( const char* path )
{
	gpstring test = MakeTempFileName( path );
	return ( !test.empty() );
}

bool IsDirWritable( const wchar_t* path )
{
	gpstring test = MakeTempFileName( ::ToAnsi( path ) );
	return ( !test.empty() );
}

gpstring GetLocalWritableDir( bool trailingSlash )
{
	gpstring path;

	// try exe dir
	path = GetModuleDirectory( NULL, trailingSlash );
	if ( !IsDirWritable( path ) )
	{
		// try current dir
		path = GetCurrentDirectory( trailingSlash );
		if ( !IsDirWritable( path ) )
		{
			// try temps
			path = GetTempWritableDir( trailingSlash, false );
			if ( !IsDirWritable( path ) )
			{
				// try c:\ finally
				path = "c:\\";
				if ( !IsDirWritable( path ) )
				{
					path.erase();
				}
			}
		}
	}

	return ( path );
}

gpwstring GetLocalWritableDirW( bool trailingSlash )
{
	gpwstring str = ::ToUnicode( GetLocalWritableDir( false ) );
	if ( trailingSlash )
	{
		stringtool::AppendTrailingBackslash( str );
	}
	return ( str );
}

gpstring GetTempWritableDir( bool trailingSlash, bool tryLocalWritableToo )
{
	// try windows's temp dir first
	gpstring path = GetTempPath( trailingSlash );
	if ( !IsDirWritable( path ) )
	{
		// try %temp%
		path = stringtool::GetEnvironmentVariable( "temp" );
		if ( !IsDirWritable( path.c_str() ) )
		{
			// try %tmp%
			path = stringtool::GetEnvironmentVariable( "tmp" );
			if ( !IsDirWritable( path.c_str() ) )
			{
				// try c:\ finally
				path = "c:\\";
				if ( !IsDirWritable( path.c_str() ) )
				{
					path.erase();
				}
			}
		}

		// update slash
		stringtool::IncludeTrailingBackslash( path, trailingSlash );
	}

	// not found? maybe try other paths too
	if ( path.empty() && tryLocalWritableToo )
	{
		path = GetLocalWritableDir( trailingSlash );
	}

	return ( path );
}

gpwstring GetTempWritableDirW( bool trailingSlash )
{
	gpwstring str = ::ToUnicode( GetTempWritableDir( false ) );
	if ( trailingSlash )
	{
		stringtool::AppendTrailingBackslash( str );
	}
	return ( str );
}

gpstring MakeWritableFileName( const char* filename )
{
	gpassert( ( filename != NULL ) && ( *filename != '\0' ) );

	// path info?
	if ( HasPathInfo( filename ) )
	{
		// yup - use as is
		return ( filename );
	}
	else
	{
		// nope - get writable dir
		return ( GetLocalWritableDir( true ) + filename );
	}
}

gpstring GetFullPathName( const char* file, int* fileNameOffset )
{
	char path[ MAX_PATH ];
	LPTSTR ptr;
	if ( ::GetFullPathName( file, ELEMENT_COUNT( path ), path, &ptr ) == 0 )
	{
		path[ 0 ] = '\0';
		ptr = path;
	}

	if ( fileNameOffset != NULL )
	{
		*fileNameOffset = ptr - path;
	}

	return ( path );
}

gpwstring GetFullPathName( const wchar_t* file )
{
	return ( ::ToUnicode( GetFullPathName( ::ToAnsi( file ) ) ) );
}

gpstring GetLongPathName( const char* file )
{
	char path[ MAX_PATH ];
	if ( ::GetLongPathName( file, path, MAX_PATH ) == 0 )
	{
		return ( file );
	}
	return ( path );
}

gpstring GetModuleFileName( HINSTANCE inst )
{
	char path[ MAX_PATH ];
	if ( ::GetModuleFileName( inst, path, ELEMENT_COUNT( path ) ) == 0 )
	{
		return ( gpstring::EMPTY );
	}

	// always return long path name - the stupid EBU installer runs the game
	// with a short filename. @*&$&*@% (#10380)
	return ( GetLongPathName( path ) );
}

gpstring GetModuleDirectory( HINSTANCE inst, bool trailingSlash )
{
	gpstring path = GetModuleFileName( inst );
	if ( !path.empty() )
	{
		size_t found = path.find_last_of( "\\/:" );
		if ( found != gpstring::npos )
		{
			path.erase( trailingSlash ? ( found + 1 ) : found );
		}
	}
	return ( path );
}

gpstring GetCurrentDirectory( bool trailingSlash )
{
	char path[ MAX_PATH ];
	if ( ::GetCurrentDirectory( ELEMENT_COUNT( path ), path ) == 0 )
	{
		path[ 0 ] = '\0';
	}

	gpstring str( path );
	if ( trailingSlash )
	{
		stringtool::AppendTrailingBackslash( str );
	}
	return ( str );
}

gpstring MakeTempFileName( const char* path, const char* prefix, UINT unique, bool autoDelete )
{
	gpassert( ( path != NULL ) && ( prefix != NULL ) );
	char buffer[ MAX_PATH ];

	gpstring str;
	if ( ::GetTempFileName( path, prefix, unique, buffer ) )
	{
		str = buffer;
	}
	if ( !str.empty() && autoDelete )
	{
		::DeleteFile( str.c_str() );
	}
	return ( str );
}

gpstring MakeTempPathFileName( const char* prefix, UINT unique, bool autoDelete )
{
	return ( MakeTempFileName( GetTempWritableDir(), prefix, unique, autoDelete ) );
}

gpstring GetTempPath( bool trailingSlash )
{
	char buffer[ MAX_PATH ];
	gpstring str;

	if ( ::GetTempPath( ELEMENT_COUNT( buffer ), buffer ) != 0 )
	{
		// for some reason this comes back as microso~1 format
		::GetLongPathName( buffer, buffer, ELEMENT_COUNT( buffer ) );
		str = buffer;
	}
	
	stringtool::IncludeTrailingBackslash( str, trailingSlash );

	return ( str );
}

gpwstring GetTempPathW( bool trailingSlash )
{
	gpwstring str = ::ToUnicode( GetTempPath( false ) );
	if ( trailingSlash )
	{
		stringtool::AppendTrailingBackslash( str );
	}
	return ( str );
}

bool SetFileTimeAncient( HANDLE file )
{
	SYSTEMTIME stime;
	::GetLocalTime( &stime );
	stime.wYear -= 20;

	FILETIME ftime;
	::SystemTimeToFileTime( &stime, &ftime );

	return ( !!::SetFileTime( file, &ftime, &ftime, &ftime ) );
}

bool RemoveDirectory( const char* path, int recurseDepth )
{
	gpassert( path != NULL );

	bool success = true;

	// if it's empty, it can't be right
	if ( *path == '\0' )
	{
		return ( false );
	}

	// append a backslash if there isn't one then make it search for all
	gpstring localPath( path );
	stringtool::AppendTrailingBackslash( localPath );
	gpstring subDir( localPath );
	localPath += '*';

	// find everything
	FileFinder finder( localPath );
	gpstring fileName;
	while ( finder.Next( fileName ) )
	{
		if ( finder.Get().dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			if ( !fileName.same_with_case( "." ) && !fileName.same_with_case( ".." ) )
			{
				if ( recurseDepth != 0 )
				{
					// recurse into there
					if ( !RemoveDirectory( subDir + fileName, (recurseDepth < 0) ? recurseDepth : (recurseDepth - 1) ) )
					{
						success = false;
					}
				}
				else
				{
					// not recursing - subdir existence == failure
					success = false;
				}
			}
		}
		else if ( !::DeleteFile( subDir + fileName ) )
		{
			success = false;
		}
	}

	// try to remove the dir now
	stringtool::RemoveTrailingBackslash( subDir );
	if ( !::RemoveDirectory( subDir ) )
	{
		success = false;
	}

	// done
	return ( success );
}

bool PathSpec :: Init( gpwstring spec, const wchar_t* defPattern, bool testExist )
{
	// reset
	*this = PathSpec();

	// always backslashes
	CleanupPath( spec );

	// get pattern
	bool success = false;
	if ( !stringtool::HasDosWildcards( spec ) && (!testExist || DoesPathExist( spec )) )
	{
		// it's a plain path
		m_PathName = spec;
		stringtool::AppendTrailingBackslash( m_PathName );
		m_FileName = defPattern;
		success = true;
	}
	else
	{
		const wchar_t* end = GetPathEnd( spec );
		if ( end != NULL )
		{
			// take values
			m_FileName = end;
			m_PathName.assign( spec, end );

			// default for filespec
			if ( m_FileName.empty() )
			{
				m_FileName = defPattern;
			}

			// check for recursion request
			if ( !m_PathName.empty() && (m_PathName.rbegin()[ 0 ] == L'\\') && (m_PathName.rbegin()[ 1 ] == L'*') )
			{
				m_Recursive = true;
				m_PathName.erase( m_PathName.length() - 2 );
			}

			// success depends on path's existence
			if ( !testExist || DoesPathExist( m_PathName ) )
			{
				success = true;
			}
		}
		else if ( !testExist || stringtool::HasDosWildcards( spec ) || DoesFileExist( spec ) )
		{
			// no path, just file
			m_FileName = spec;
			success = true;
		}
	}

	// done
	return ( success );
}

#pragma warning ( disable : 4509 )

bool GetModuleChecksums( DWORD& headerCrc, DWORD& actualCrc, HINSTANCE inst )
{
	FileSys::MemoryFile file;
	bool success = file.Create( FileSys::GetModuleFileName( inst ) );

	if ( success )
	{
		__try
		{
			// get headers
			const BYTE* imageBase = (const BYTE*)file.GetData();
			const IMAGE_DOS_HEADER* dosHeader = (const IMAGE_DOS_HEADER*)imageBase;
			const IMAGE_NT_HEADERS* winHeader = (const IMAGE_NT_HEADERS*)( imageBase + dosHeader->e_lfanew );

			// copy the crc from the header
			headerCrc = winHeader->OptionalHeader.CheckSum;

			// ok now checksum everything after the crc
			const void* crcBase = (const BYTE*)&winHeader->OptionalHeader.CheckSum + sizeof( winHeader->OptionalHeader.CheckSum );
			actualCrc = ::GetCRC32( crcBase, file.GetSize() - ((const BYTE*)crcBase - (const BYTE*)file.GetData()) );
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
			success = false;
		}
	}

	return ( success );
}

void FindFilesOnPath( const PathSpec& pathSpec, std::list <gpwstring> & foundFiles )
{
	// find files in this dir
	FileFinder fileFinder( pathSpec.m_PathName + pathSpec.m_FileName );
	gpwstring file;
	while ( fileFinder.Next( file ) )
	{
		if ( fileFinder.IsFile() )
		{
			foundFiles.push_back( pathSpec.m_PathName + file );
		}
	}

	// recurse
	if ( pathSpec.m_Recursive )
	{
		FileFinder pathFinder( pathSpec.m_PathName + L"*" );
		while ( pathFinder.Next( file ) )
		{
			if ( pathFinder.IsDirectory() && !file.same_no_case( L"." ) && !file.same_no_case( L".." ) )
			{
				PathSpec localPathSpec( pathSpec );
				localPathSpec.m_PathName += file + L'\\';
				FindFilesOnPath( localPathSpec, foundFiles );
			}
		}
	}
}

void FindFilesOnPath(
		const gpwstring&		path,
		std::list <gpwstring> &	foundFiles,
		const wchar_t*			defPattern,
		std::list <gpwstring> *	badPaths )
{
	PathSpec pathSpec;
	if ( pathSpec.Init( path, defPattern ) )
	{
		FindFilesOnPath( pathSpec, foundFiles );
	}
	else if ( badPaths != NULL )
	{
		badPaths->push_back( path );
	}
}

int FindPathsFromSpec(
		const wchar_t* pathSpec,
		std::list <gpwstring> & foundPaths )
{
	gpassert( pathSpec != NULL );

	int oldSize = foundPaths.size();

	bool inQuote = false;
	gpwstring path;
	for ( ; ; )
	{
		size_t found = ::wcscspn( pathSpec, inQuote ? L"\"" : L";\"" );
		wchar_t delim = pathSpec[ found ];
		if ( delim == L'\0' )
		{
			// use rest of string
			path += pathSpec;
			if ( !path.empty() )
			{
				foundPaths.push_back( path );
			}

			// all done
			break;
		}

		// add all except the delimiter
		path.insert( path.end_split(), pathSpec, pathSpec + found );
		pathSpec += found + 1;

		// begin/endquote?
		if ( delim == L'\"' )
		{
			// toggle parser state
			inQuote = !inQuote;
		}
		else if ( delim == L';' )
		{
			// take string
			if ( !path.empty() )
			{
				foundPaths.push_back( path );
			}
			path.clear();
		}
	}

	return ( foundPaths.size() - oldSize );
}

int FindFilesFromSpec(
		const wchar_t*			pathSpec,
		std::list <gpwstring> &	foundFiles,
		bool					parseSemicolons,
		const wchar_t*			defPattern,
		std::list <gpwstring> *	badPaths )
{
	gpassert( pathSpec != NULL );

	int oldSize = foundFiles.size();

	if ( parseSemicolons )
	{
		std::list <gpwstring> foundPaths;
		FindPathsFromSpec( pathSpec, foundPaths );

		std::list <gpwstring>::const_iterator i, ibegin = foundPaths.begin(), iend = foundPaths.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			FindFilesOnPath( *i, foundFiles, defPattern, badPaths );
		}
	}
	else
	{
		FindFilesOnPath( pathSpec, foundFiles, defPattern, badPaths );
	}

	return ( foundFiles.size() - oldSize );
}

//////////////////////////////////////////////////////////////////////////////
// class File implementation

HANDLE File :: Release( void )
{
#	if TRACK_RESOURCES
	if ( IsOpen() )
	{
		gTracker.CloseFileHandle( m_Handle );
	}
#	endif // TRACK_RESOURCES

	HANDLE h = m_Handle;
	m_Handle = INVALID_HANDLE_VALUE;
	return ( h );
}

File& File :: operator = ( HANDLE h )
{
	Close();
	m_Handle = h;

#	if TRACK_RESOURCES
	if ( IsOpen() )
	{
		gTracker.CreateFile( m_Handle );
	}
#	endif // TRACK_RESOURCES

	return ( *this );
}

gpstring File :: ReadIntoString( int size )
{
	gpstring str;
	str.resize( size );
	if ( !Read( str.begin_split(), size ) )
	{
		str.clear();
	}
	return ( str );
}

gpstring File :: ReadFileIntoString( void )
{
	DWORD oldPos = GetPos();
	SeekBegin();

	gpstring str;
	int size = GetSize();
	str.resize( size );
	if ( !Read( str.begin_split(), size ) )
	{
		str.clear();
	}

	Seek( oldPos );
	return ( str );
}

bool File :: WriteText( const char* text, int len )
{
	gpassert( text != NULL );
	if ( len == -1 )
	{
		len = ::strlen( text );
	}
	int search = len;

	// write it out, converting \n to \r\n
	const char* found;
	const char* iter = text;
	while ( (found = ::strnchr( iter, '\n', search )) != NULL )
	{
		// only do it if not already preceded by \r
		if ( (found == iter) || (found[ -1 ] != '\r') )
		{
			// write out line and terminator
			int write = found - text;
			if ( !Write( text, write ) || !Write( "\r\n", 2 ) )
			{
				return ( false );
			}
			++write;
			len -= write;
			text += write;
		}

		// next
		++found;
		search -= found - iter;
		iter = found;
	}

	// write out remainder if any
	return ( (len > 0) ? Write( text, len ) : true );
}

bool File :: WriteArgs( const char* format, va_list args )
{
	auto_dynamic_vsnprintf printer( format, args );
	return ( WriteText( printer, printer.length() ) );
}

//////////////////////////////////////////////////////////////////////////////
// class FileMap implementation

HANDLE FileMap :: Release( void )
{
#	if TRACK_RESOURCES
	if ( IsMapped() )
	{
		gTracker.CloseMapHandle( m_Handle );
	}
#	endif // TRACK_RESOURCES

	HANDLE h = m_Handle;
	m_Handle = NULL;
	return ( h );
}

//////////////////////////////////////////////////////////////////////////////
// class MapView implementation

bool MapView :: Create( HANDLE hmap, eAccess access, DWORD offset, DWORD mapSize )
{

// Align.

	// $ according to MapViewOfFile() docs in SDK, offset must be aligned to
	//   system allocation granularity

	// align as directed
	DWORD granularity = SysInfo::GetSystemInfo().dwAllocationGranularity;
	DWORD aligned = GetAlignDown( offset, granularity );

	// update size unless we're using the entire map
	if ( mapSize != 0 )
	{
		mapSize += offset - aligned;
	}

// Map.

	gpassert( !IsViewed() && (hmap != NULL) );
	m_AlignedData = GP_MapViewOfFile( hmap, access, aligned, mapSize );
	m_Data = rcast <BYTE*> ( m_AlignedData ) + offset - aligned;

// Done.

	return ( IsViewed() );
}

HANDLE MapView :: Release( void )
{
#	if TRACK_RESOURCES
	if ( IsViewed() )
	{
		gTracker.UnmapViewOfFile( ccast <LPVOID> ( m_AlignedData ) );
	}
#	endif // TRACK_RESOURCES

	LPVOID d = m_AlignedData;
	m_Data = NULL;
	m_AlignedData = NULL;
	return ( d );
}

//////////////////////////////////////////////////////////////////////////////
// class MemoryFile implementation

bool MemoryFile :: Create( LPCTSTR  name,
						   eAccess  access,
						   eSharing sharing,
						   DWORD    flags,
						   bool*    wasFileEmpty,
						   DWORD    extraProtect )
{
	gpassert( !IsOpen() );

	// init all these with defaults just to keep the compiler from complaining
	File   ::eAccess  fileAccess  = File::ACCESS_READ_ONLY;
	File   ::eSharing fileSharing = scast <File::eSharing> (sharing);
	FileMap::eProtect mapProtect  = FileMap::PROTECT_READ_ONLY;
	MapView::eAccess  viewAccess  = MapView::ACCESS_READ_ONLY;

	// ok now set those up for real
	switch ( access )
	{
		case ( ACCESS_READ_ONLY ):
		{
			fileAccess = File   ::ACCESS_READ_ONLY;
			mapProtect = FileMap::PROTECT_READ_ONLY;
			viewAccess = MapView::ACCESS_READ_ONLY;
		}	break;

		case ( ACCESS_READ_WRITE ):
		{
			fileAccess = File   ::ACCESS_READ_WRITE;
			mapProtect = FileMap::PROTECT_READ_WRITE;
			viewAccess = MapView::ACCESS_READ_WRITE;
		}	break;

		case ( ACCESS_COPY_ON_WRITE ):
		{
			fileAccess = File   ::ACCESS_READ_WRITE;
			mapProtect = FileMap::PROTECT_COPY_ON_WRITE;
			viewAccess = MapView::ACCESS_COPY_ON_WRITE;
		}	break;

		default:
		{
			gpassert( 0 );
		}
	}

	bool success = true;
	if (   !m_File.OpenExisting( name, fileAccess, fileSharing, flags )
		|| !m_Map .Create      ( m_File, mapProtect, 0, NULL, extraProtect )
		|| !m_View.Create      ( m_Map,  viewAccess ) )
	{
		success = false;

		DWORD err = ::GetLastError();

		if ( wasFileEmpty != NULL )
		{
			*wasFileEmpty = m_File.IsOpen() && ( m_File.GetSize() == 0 );
		}

		Close();
		::SetLastError( err );
	}
	else if ( wasFileEmpty != NULL )
	{
		*wasFileEmpty = false;
	}

	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// class FileFinder implementation

void FileFinder :: Init( const char* filename )
{
	gpassert( filename != NULL );						// parameter validation
	Close();
	m_FileName = filename;
}

void FileFinder :: Init( const wchar_t* filename )
{
	gpassert( filename != NULL );						// parameter validation
	Close();
	m_FileName = ::ToAnsi( filename );
}

bool FileFinder :: Close(void)
{
	bool rc = true;
	if ( m_Handle != NULL)
	{
		rc = GP_FindClose( m_Handle ) != FALSE;

		m_FileName.erase();
		m_Handle = NULL;
	}
	return ( rc );
}

bool FileFinder :: Next(void)
{
	gpassert( !m_FileName.empty() );
	if ( m_Handle != NULL )
	{
		if ( !GP_FindNextFile( m_Handle, &m_FindData ) )
		{
			Close();
			return ( false );
		}
	}
	else
	{
		m_Handle = GP_FindFirstFile( m_FileName.c_str(), &m_FindData );
		if ( m_Handle == INVALID_HANDLE_VALUE )
		{
			m_Handle = NULL;
			return ( false );
		}
	}
	return ( true );
}

bool FileFinder :: Next( gpstring& str )
{
	if ( Next() )
	{
		str = Get().cFileName;
		return ( true );
	}
	else
	{
		str.erase();
		return ( false );
	}
}

bool FileFinder :: Next( gpwstring& str )
{
	gpstring astr;
	bool rc = Next( astr );
	str = ::ToUnicode( astr );
	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
// class FileVersionInfo implementation

// force linking with version.lib
#pragma comment(lib, "version")

bool FileVersionInfo :: Init( const char* filename, int index )
{
	// assume failure
	m_Valid = false;

	// get the filename
	gpstring localFile;
	if ( (filename == NULL) || (*filename == '\0') )
	{
		localFile = GetModuleFileName();
		filename = localFile.c_str();
	}

	// now get the version info size and alloc space for it
	DWORD temp, size = ::GetFileVersionInfoSize( ccast <char*> ( filename ), &temp );
	if ( size > 0 )
	{
		// alloc memory buffer
		m_Data.resize( size );

		// pull in the data
		if ( ::GetFileVersionInfo( ccast <char*> ( filename ), 0, m_Data.size(), m_Data.begin() ) )
		{
			// we're ok
			m_Valid = true;

			// then finally set up the stringfileinfo prefix
			m_StringPrefix = "\\StringFileInfo\\";
			m_StringPrefix.append( GetTranslationText( index ) );
			m_StringPrefix.append( "\\" );
		}
	}

	return ( IsValid() );
}

const VS_FIXEDFILEINFO& FileVersionInfo :: GetFixedInfo( void ) const
{
	const VS_FIXEDFILEINFO* info = NULL;

	if ( IsValid() )
	{
		// get the raw data
		UINT len;
		if ( Query( "\\", info, &len ) )
		{
			gpassert( len >= sizeof( VS_FIXEDFILEINFO ) );
		}
		else
		{
			gpwarning( "FileVersionInfo::GetFixedInfo() - Query( \"\\\" ) failed\n" );
		}
	}

	if ( info == NULL )
	{
		static VS_FIXEDFILEINFO sNullInfo;	// $ compiler initializes to all zeros
		info = &sNullInfo;
	}

	return ( *info );
}

bool FileVersionInfo :: GetTranslation( LANGID& lang, UINT& codePage, int index ) const
{
	if ( !IsValid() )
	{
		return ( false );
	}

	// get the translations
	DWORD* trans;
	UINT len;
	if ( !Query( "\\VarFileInfo\\Translation", trans, &len ) )
	{
		return ( false );
	}

	// check range
	gpassert( (index >= 0) && (scast <int> ( len / sizeof(DWORD) ) >= (index + 1)) );

	// now grab the value for the index we want
	DWORD item = trans[ index ];
	lang = LOWORD( item );
	codePage = HIWORD( item );

	// done
	return ( true );
}

gpstring FileVersionInfo :: GetTranslationText( int index ) const
{
	gpstring text;

	LANGID lang;
	UINT codePage;
	if ( GetTranslation( lang, codePage, index ) )
	{
		text.assignf( "%04x%04x", lang, codePage );
	}

	return ( text );
}

const char* FileVersionInfo :: GetEntry( const char* entry ) const
{
	gpassert( (entry != NULL) && (*entry != '\0') );

	if ( !IsValid() )
	{
		return ( NULL );
	}

	// build path to query
	gpstring path( m_StringPrefix );
	path.append( entry );

	// get the entry
	const char* data = NULL;
	Query( path.c_str(), data );

	// return it (may be null if missed)
	return ( data );
}

bool FileVersionInfo :: QueryValue( const char* path, void*& data, UINT* len ) const
{
	UINT localLen;
	if ( len == NULL )
	{
		len = &localLen;
	}

	if ( ::VerQueryValue( (void*)m_Data.begin(), (char*)path, &data, len ) )
	{
		gpassert( data != NULL );
		gpassert( len > 0 );
		return ( true );
	}
	else
	{
		gpwarning( "VerQueryValue( \"\\\" ) failed\n" );
		data = NULL;
		*len = 0;
		return ( false );
	}
}

//////////////////////////////////////////////////////////////////////////////

}	// end of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
