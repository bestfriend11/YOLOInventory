//////////////////////////////////////////////////////////////////////////////
//
// File     :  FileSysUtilsAW.cpp
// Author(s):  Scott Bilas
//
// Summary  :  This is a special inline file meant to be #included multiple
//             times by FileSysUtils.cpp ONLY. Use to define ANSI and Unicode
//             versions of functions easily.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#if !defined( FILESYSUTILSA ) && !defined( FILESYSUTILSW )
#error ! Do NOT include this file! It's meant for FileSysUtils.cpp only!
#endif

#ifdef FILESYSUTILSA

#define XCHAR char
#define XSTRING gpstring
#define XANSI( x ) x
#define XC( x ) x
#define XISALPHA isalpha
#define XISALNUM isalnum
#define XISCSYM iscsym
#define XISCSYMF iscsymf
#define XSTRCSPN strcspn
#define XSTRRCSPN strrcspn
#define XSTRCHR strchr
#define XSTRRCHR strrchr
#define XSTRLEN strlen
#define XBAD_FILENAME_CHARS_FULL BAD_FILENAME_CHARS_FULL
#define XBAD_FILENAME_CHARS_AND_SPACE BAD_FILENAME_CHARS_AND_SPACE
#define GetLocalWritableDir GetLocalWritableDir

#else

#define XCHAR wchar_t
#define XSTRING gpwstring
#define XANSI( x ) ::ToAnsi( x )
#define XC( x ) L ## x
#define XISALPHA iswalpha
#define XISALNUM iswalnum
#define XISCSYM iswcsym
#define XISCSYMF iswcsymf
#define XSTRCSPN wcscspn
#define XSTRRCSPN wcsrcspn
#define XSTRCHR wcschr
#define XSTRRCHR wcsrchr
#define XSTRLEN wcslen
#define XBAD_FILENAME_CHARS_FULL WBAD_FILENAME_CHARS_FULL
#define XBAD_FILENAME_CHARS_AND_SPACE WBAD_FILENAME_CHARS_AND_SPACE
#define GetLocalWritableDir GetLocalWritableDirW

#endif

//////////////////////////////////////////////////////////////////////////////

bool DoesFileExist( const XCHAR* check )
{
	FileFinder findfile( XANSI( check ) );

	// keep searching until we find a non-directory that matches
	while ( findfile.Next() )
	{
		if ( !( findfile.Get().dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			return ( true );
		}
	}
	return ( false );
}

bool DoesPathExist( const XCHAR* check )
{
	gpassert( check != NULL );
	bool found = false;

	if ( *check != XC( '\0' ) )
	{
		// append a backslash if there isn't one then make it search for all
		XSTRING path( check );
		stringtool::AppendTrailingBackslash( path );
		path += XC( '*' );

		// check for any and all files in this dir - if they exist, then the
		//  dir exists.  alternate method would be to look for "path\."
		if ( FileFinder( XANSI( path ) ).Next() )
		{
			found = true;
		}
	}
	return ( found );
}

bool IsValidFileName( const XCHAR* check, bool checkReservedNames, bool spacesAllowed )
{
	// $ this is from MSDN help "File Name Conventions"
	//   in Platform SDK: Files and I/O

	// check for obvious bad
	if ( (check == NULL) || (*check == XC( '\0' )) )
	{
		return ( false );
	}

	// check for bad chars
	if ( check[ ::XSTRCSPN( check, spacesAllowed ? XBAD_FILENAME_CHARS_FULL : XBAD_FILENAME_CHARS_AND_SPACE ) ] != XC( '\0' ) )
	{
		return ( false );
	}

	// $ this is from MSDN help "Chapter 17 - File Systems"
	//   in Win2k Pro Resource Kit System Configuration and Management

	static const XCHAR* RESERVED_NAMES[] =
	{
		XC( "aux" ), XC( "com1" ), XC( "com2" ), XC( "com3" ), XC( "com4" ), XC( "con" ),
		XC( "lpt1" ), XC( "lpt2" ), XC( "lpt3" ), XC( "nul" ), XC( "prn" ),
	};

	// check for reserved names
	if ( checkReservedNames )
	{
		const XCHAR** found = stdx::binary_search( RESERVED_NAMES, ARRAY_END( RESERVED_NAMES ), check, istring_less() );
		if ( found != ARRAY_END( RESERVED_NAMES ) )
		{
			return ( false );
		}
	}

	// guess it's ok
	return ( true );
}

ePathType GetPathType( const XCHAR* path, int* rootEndOffset )
{
	gpassert( path != NULL );

	// preinit this - probably not going to have a root anyway
	if ( rootEndOffset != NULL )
	{
		*rootEndOffset = 0;
	}

	// easy bailout
	if ( *path == XC( '\0' ) )
	{
		return ( PATHTYPE_ILLEGAL );
	}

	// leading slash locks us in to easy tests
	if ( (*path == XC( '\\' )) || (*path == XC( '/' )) )
	{
		++path;
		if ( (*path == XC( '\\' )) || (*path == XC( '/' )) )
		{
			if ( rootEndOffset != NULL )
			{
				++path;
				const XCHAR* begin = path - 2;

				// find end of this root
				stringtool::NextPathComponent( path );
				stringtool::NextPathComponent( path );
				*rootEndOffset = path - begin;
			}
			return ( PATHTYPE_ABSOLUTE_SHARE );
		}
		else
		{
			if ( rootEndOffset != NULL )
			{
				*rootEndOffset = 1;
			}
			return ( PATHTYPE_RELATIVE_DRIVE );
		}
	}

	// 'A:' style also easy
	if ( (path[ 1 ] == XC( ':' )) && XISALPHA( path[ 0 ] ) )
	{
		// may be either c:abc or c:\abc so check for it
		if ( (path[ 2 ] == XC( '\\' )) || (path[ 2 ] == XC( '/' )) )
		{
			if ( rootEndOffset != NULL )
			{
				*rootEndOffset = 3;
			}
			return ( PATHTYPE_ABSOLUTE_NORMAL );
		}
		else
		{
			if ( rootEndOffset != NULL )
			{
				*rootEndOffset = 2;
			}

			// still may be c: or c:abc
			return ( (path[ 2 ] == XC( '\0' )) ? PATHTYPE_ABSOLUTE_NORMAL : PATHTYPE_RELATIVE_PATH );
		}
	}

	// check for URL style
	if ( XISCSYMF( *path ) )
	{
		for ( const XCHAR* iter = path + 1 ; *iter != XC( '\0' ) ; ++iter )
		{
			if ( !XISCSYM( *iter ) )
			{
				// check the remainder of the '://' pattern
				if ( (iter[ 0 ] == XC( ':' )) && (iter[ 1 ] == XC( '/' )) && (iter[ 2 ] == XC( '/' )) )
				{
					// this is a url!
					if ( rootEndOffset != NULL )
					{
						*rootEndOffset = iter - path + 3;
					}
					return ( PATHTYPE_ABSOLUTE_URL );
				}

				// must not be url, stop searching!
				break;
			}
		}
	}

	// general algorithm: go through the pathname, keeping track of what level
	//  we're in by adding one for each subdir\ and subtracting one for each
	//  ..\ and then return if level < 0. note that because this function is
	//  sort of a security thing (i.e. prevent people from popping out of a
	//  root dir acting as a sandbox) we check for both types of slashes.

	int level = 0;

	for ( ; ; )
	{
		// skip extra slashes (legal and harmless in win32 paths)
		while ( (*path == XC( '\\' )) || (*path == XC( '/' )) )
		{
			++path;
		}

		// check for relative paths
		if ( *path == XC( '.' ) )
		{
			// check for "..\"
			if (   (path[ 1 ] == XC( '.' ))
				&& ( (path[ 2 ] == XC( '\\' )) || (path[ 2 ] == XC( '/' )) || (path[ 2 ] == XC( '\0' )) ) )
			{
				// it's "..\" so up the level
				if ( --level < 0 )
				{
					// just abort right now. path may go back down again into
					//  the same dir name but that's not worth the effort to track.
					return ( PATHTYPE_RELATIVE_PARENT );
				}

				// advance
				path += 3;
				continue;
			}
			else if ( (path[ 1 ] == XC( '\\' )) || (path[ 1 ] == XC( '/' )) )
			{
				// it's .\ so don't do anything with the level
				path += 2;
				continue;
			}
			else if ( path[ 1 ] == XC( '\0' ) )
			{
				// it's . so just abort here, leaving the level alone
				break;
			}
		}

		// not .\ or ..\ so find the next path delimiter
		++level;
		stringtool::NextPathComponent( path );
		if ( *path == XC( '\0' ) )
		{
			break;		// all done!
		}

		// advance past the slash
		++path;
	}

	// must be normal eh
	return ( PATHTYPE_RELATIVE_NORMAL );
}

ePathType GetPathType( const XCHAR* path, const XCHAR*& rootEnd )
{
	int rootEndOffset = 0;
	ePathType type = GetPathType( path, &rootEndOffset );
	rootEnd = path + rootEndOffset;
	return ( type );
}

const XCHAR* GetFileName( const XCHAR* path )
{
	gpassert( path != NULL );

	// special: check for \\machinename or \\machinename\pathname (not allowed)
	if ( (path[ 0 ] == XC( '\\' )) && (path[ 1 ] == XC( '\\' )) )
	{
		// find end of share name
		path += 2;
		path += ::XSTRCSPN( path, XC( "\\/" ) );

		// now find the filename
		return ( GetFileName( path ) );
	}

	int offset = ::XSTRRCSPN( path, XC( "\\/:" ) );
	if ( offset != -1 )
	{
		path += offset + 1;
	}

	return ( (*path == XC( '\0' )) ? NULL : path );
}

const XCHAR* GetExtension( const XCHAR* path, bool leadingDot )
{
	const XCHAR* ext = GetFileName( path );
	if ( ext != NULL )
	{
		ext = ::XSTRRCHR( ext, XC( '.' ) );
		if ( (ext != NULL) && !leadingDot )
		{
			++ext;
		}
	}

	return ( ext );
}

XSTRING GetFileNameOnly( const XCHAR* path )
{
	XSTRING nameOnly;

	const XCHAR* filename = GetFileName( path );
	if ( filename != NULL )
	{
		const XCHAR* end = ::XSTRRCHR( filename, XC( '.' ) );
		if ( end != NULL )
		{
			nameOnly.assign( filename, end );
		}
		else
		{
			nameOnly.assign( filename );
		}
	}

	return ( nameOnly );
}

const XCHAR* GetPathEnd( const XCHAR* path, bool trailingSlash )
{
	const XCHAR* end = GetFileName( path );
	if ( end == NULL )
	{
		end = path + XSTRLEN( path );
	}
	if ( !trailingSlash )
	{
		while ( (end != path) && ((end[ -1 ] == XC( '/' )) || (end[ -1 ] == XC( '\\' ))) )
		{
			--end;
		}
	}
	return ( end );
}

XSTRING GetPathOnly( const XCHAR* path, bool trailingSlash )
{
	return ( XSTRING( path, GetPathEnd( path, trailingSlash ) ) );
}

void CleanupPath( XSTRING& path )
{
	// remove any embedded quotes
	size_t found = 0;
	for ( ; ; )
	{
		found = path.find( XC( '\"' ), found );
		if ( found != XSTRING::npos )
		{
			path.erase( found, 1 );
		}
		else
		{
			break;
		}
	}

	// remove bordering spaces
	stringtool::RemoveBorderingWhiteSpace( path );

 	// $ early bailout if nothing to do
	if ( path.empty() )
	{
		return;
	}

	// first convert forward slashes to back slashes
	std::replace( path.begin_split(), path.end_split(), XC( '/' ), XC( '\\' ) );

	// next remove any extra slashes. note: skip a leading \\ for unc paths.
	for ( XSTRING::iterator i = path.begin_split() + 1 ; i != path.end_split() ; )
	{
		if ( (i[ 0 ] == XC( '\\' )) && (i[ 1 ] == XC( '\\' )) )
		{
			i = path.erase( i );
		}
		else
		{
			++i;
		}
	}

	// remove embedded '\.\'
	found = 0;
	for ( ; ; )
	{
		found = path.find( XC( "\\.\\" ), found );
		if ( found != XSTRING::npos )
		{
			path.erase( found, 2 );		// just erase the \. part
		}
		else
		{
			break;
		}
	}

	// remove leading .\ or if all set to . then clear
	if ( path[ 0 ] == XC( '.' ) )
	{
		if ( path[ 1 ] == XC( '\\' ) )
		{
			path.erase( 0, 2 );
		}
		else if ( path[ 1 ] == XC( '\0' ) )
		{
			path.clear();
		}
	}

	// remove trailing \.
	size_t sz = path.size();
	if ( (sz > 2) && (path[ sz - 1 ] == XC( '.' )) && (path[ sz - 2 ] == XC( '\\' )) )
	{
		path.erase( sz - 2 );
	}
}

bool HasPathInfo( const XCHAR* filename )
{
	gpassert( filename != NULL );
	return ( filename[ ::XSTRCSPN( filename, XC( "\\:/" ) ) ] != XC( '\0' ) );
}

bool HasExtension( const XCHAR* filename )
{
	return ( GetExtension( filename ) != NULL );
}

XSTRING MakeUniqueFileName( const XCHAR* base, const XCHAR* extension, const XCHAR* path, int maxTries )
{
	XSTRING localBase;
	if ( (base == NULL) || (*base == XC( '\0' )) )
	{
		localBase = XC( "file_" );
	}
	else
	{
		localBase = base;
		gpassert( localBase.find_first_of( XC( "\\/:" ) ) == XSTRING::npos );
	}

	gpassert( (extension != NULL) && (*extension != XC( '\0' )) );

	XSTRING localPath;
	if ( path == NULL )
	{
		localPath = GetLocalWritableDir();
	}
	else
	{
		localPath = path;
		stringtool::AppendTrailingBackslash( localPath );
	}

	XSTRING fileName, tryName;
	int digits = stringtool::GetWidthInDigits( maxTries );
	for (int i = 0 ; i < maxTries ; ++i)
	{
		tryName.assignf( XC( "%s%s%0*d.%s" ), localPath.c_str(), localBase.c_str(), digits, i, extension );
		if ( !DoesFileExist( tryName ) )
		{
			fileName = tryName;
			break;
		}
	}

	return ( fileName );
}

bool CreateFullDirectory( const XCHAR* path )
{
	const XCHAR* begin = path;

	// find end of root first
	if ( GetPathType( path, begin ) == PATHTYPE_ILLEGAL )
	{
		return ( false );
	}

	// find end of first subpath
	const XCHAR* iter = begin;
	const XCHAR* end = stringtool::NextPathComponent( iter );

	// create one subdir at a time
	while ( *begin != XC( '\0' ) )
	{
		// test and create
		XSTRING subPath( path, end );
		if ( !subPath.empty() && !DoesPathExist( subPath ) && !::CreateDirectory( XANSI( subPath ), NULL ) )
		{
			return ( false );
		}

		// advance
		begin = iter;
		end   = stringtool::NextPathComponent( iter );
	}

	return ( true );
}

bool CreateFullFileNameDirectory( const XCHAR* fileName )
{
	const XCHAR* pathEnd = GetPathEnd( fileName );
	return ( CreateFullDirectory( XSTRING( fileName, pathEnd - fileName ) ) );
}

//////////////////////////////////////////////////////////////////////////////

#undef XCHAR
#undef XSTRING
#undef XANSI
#undef XC
#undef XISALPHA
#undef XISALNUM
#undef XISCSYM
#undef XISCSYMF
#undef XSTRCSPN
#undef XSTRRCSPN
#undef XSTRCHR
#undef XSTRRCHR
#undef XSTRLEN
#undef XBAD_FILENAME_CHARS_FULL
#undef XBAD_FILENAME_CHARS_AND_SPACE
#undef GetLocalWritableDir

//////////////////////////////////////////////////////////////////////////////
