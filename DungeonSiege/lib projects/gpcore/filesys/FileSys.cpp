//////////////////////////////////////////////////////////////////////////////
//
// File     :  FileSys.cpp (GPCore:FileSys)
// Author(s):  Scott Bilas
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "FileSys.h"

#include "FileSysUtils.h"
#include "ReportSys.h"
#include "StringTool.h"

#include <limits>

namespace FileSys  {  // begin of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
// enum eLocation implementation

const int MAX_ALIAS_EXTENSION_SIZE = 10;

eLocation LocalGetLocation( const char* path, bool firstTry = true )
{
	gpstring localPath( path, GetPathEnd( path ) );
	if ( localPath.empty() )
	{
		localPath = GetCurrentDirectory();
	}

	eLocation location = LOCATION_ERROR;
	switch ( ::GetDriveType( localPath ) )
	{
		case ( DRIVE_REMOVABLE ):
		case ( DRIVE_FIXED ):
		case ( DRIVE_RAMDISK ):
		{
			location = LOCATION_PATH | LOCATION_LOCAL;
		}	break;

		case ( DRIVE_REMOTE ):
		{
			location = LOCATION_PATH | LOCATION_NETWORK;
		}	break;

		case ( DRIVE_CDROM ):
		{
			location = LOCATION_PATH | LOCATION_CD;
		}	break;

		case ( DRIVE_NO_ROOT_DIR ):
		{
			// $ important NT/Win9x difference: NT allows you to pass in
			//   anything for the path here and it will figure out where it's
			//   from. stupid Win9x on the other hand will fail on anything
			//   unless it's _exactly_ the root of the drive. so we have to go
			//   out of our way on this one. this is _really_ inefficient so
			//   only do it when absolutely necessary.

			if ( firstTry )
			{
				// find the root of the drive
				gpstring resolved = GetFullPathName( localPath );
				int offset = 0;
				GetPathType( resolved.c_str(), &offset );
				resolved.erase( offset );

				// try again...
				location = LocalGetLocation( resolved.c_str(), false );
			}
			else
			{
				location = LOCATION_ERROR;
			}
		}	break;

		case ( DRIVE_UNKNOWN ):
		default:
		{
			location = LOCATION_ERROR;
		}	break;
	}

	return ( location );
}

eLocation GetLocation( const char* path )
{
	return ( LocalGetLocation( path ) );
}

//////////////////////////////////////////////////////////////////////////////
// helper function implementations

bool DoesResourceFileExist( const char* check, IFileMgr* mgr )
{
	gpassert( check != NULL );

	if ( mgr == NULL )
	{
		mgr = IFileMgr::GetMasterFileMgr();
		gpassert( mgr != NULL );
	}

	bool does = false;

	int offset;
	const IFileMgr::StringColl* aliases = IFileMgr::GetFileTypeAliases( check, &offset );
	if ( aliases != NULL )
	{
		int size = ::strlen( check );
		char* localCheck = (char*)_alloca( size + MAX_ALIAS_EXTENSION_SIZE + 1 );	// + some room for extension and terminator
		::memcpy( localCheck, check, size );

		IFileMgr::StringColl::const_iterator i, ibegin = aliases->begin(), iend = aliases->end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			::memcpy( localCheck + offset, i->begin(), i->length() + 1 );

			FindHandle find = mgr->Find( localCheck, FINDFILTER_FILES );
			if ( find )
			{
				does = mgr->HasNext( find );
				mgr->Close( find );

				if ( does )
				{
					break;
				}
			}
		}
	}
	else
	{
		FindHandle find = mgr->Find( check, FINDFILTER_FILES );
		if ( find )
		{
			does = mgr->HasNext( find );
			mgr->Close( find );
		}
	}

	return ( does );
}

bool DoesResourcePathExist( const char* check, IFileMgr* mgr )
{
	gpassert( check != NULL );
	if ( stringtool::HasDosWildcards( check ) )
	{
		return ( false );
	}

	if ( mgr == NULL )
	{
		mgr = IFileMgr::GetMasterFileMgr();
		gpassert( mgr != NULL );
	}

	gpstring localCheck( check );
	stringtool::RemoveTrailingBackslash( localCheck );

	bool does = false;

	FindHandle find = mgr->Find( localCheck, FINDFILTER_DIRECTORIES );
	if ( find )
	{
		does = mgr->HasNext( find );
		mgr->Close( find );
	}

	return ( does );
}

DWORD g_PRODUCT_ID = 0;
const DWORD& PRODUCT_ID = g_PRODUCT_ID;

void SetProductId( DWORD id )
{
	// reverse it so it's human-readable
	g_PRODUCT_ID = REVERSE_FOURCC( id );
}

RebuildCb g_RebuildCb;

void SetRebuildCb( RebuildCb cb )
{
	g_RebuildCb = cb;
}

void CallRebuildCb( void )
{
	if ( g_RebuildCb )
	{
		g_RebuildCb();
	}
}

//////////////////////////////////////////////////////////////////////////////
// class RecursiveFinder implementation

RecursiveFinder :: RecursiveFinder( const char* spec, int recurseLimit, IFileMgr* mgr )
{
	// set default
	if ( mgr == NULL )
	{
		mgr = IFileMgr::GetMasterFileMgr();
	}
	gpassert( mgr != NULL );
	m_FileMgr = mgr;

	// take limit
	m_RecurseLimit = (recurseLimit == -1) ? std::numeric_limits <int>::max() : recurseLimit;

	// get the file pattern
	const char* pathEnd = GetPathEnd( spec );
	m_FilePattern = pathEnd;

	// get the starting dir
	gpstring dir( spec, pathEnd );
	if ( !dir.empty() )
	{
		stringtool::AppendTrailingBackslash( dir );
	}

	// attempt to open it
	FindHandle root = m_FileMgr->Find( dir + "*", FINDFILTER_DIRECTORIES );
	if ( root )
	{
		// start 'er up
		m_DirFinderColl.push_back( root );
		Advance();
	}
}

RecursiveFinder :: ~RecursiveFinder( void )
{
	FindColl::iterator i, begin = m_DirFinderColl.begin(), end = m_DirFinderColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		m_FileMgr->Close( *i );
	}

	m_FileMgr->Close( m_FileFinder );
}

bool RecursiveFinder :: GetNext( gpstring& out, bool prependPath )
{
	do
	{
		// try to get next file
		if ( m_FileFinder && m_FileMgr->GetNext( m_FileFinder, out, prependPath ) )
		{
			return ( true );
		}
	}
	while( Advance() );		// otherwise walk some more

	// fail
	return ( false );
}

bool RecursiveFinder :: GetNext( FindData& out, bool prependPath )
{
	do
	{
		// try to get next file
		if ( m_FileFinder && m_FileMgr->GetNext( m_FileFinder, out, prependPath ) )
		{
			return ( true );
		}
	}
	while( Advance() );		// otherwise walk some more

	// fail
	return ( false );
}

bool RecursiveFinder :: Advance( void )
{
	// close to be sure
	m_FileMgr->Close( m_FileFinder );
	m_FileFinder = m_FileFinder.Null;

	// early bailout if no root
	if ( m_DirFinderColl.empty() )
	{
		return ( false );
	}

	// find next dir at this level if we haven't reached our limit
	gpstring dirName;
	if ( ((int)m_DirFinderColl.size() <= m_RecurseLimit) && m_FileMgr->GetNext( m_DirFinderColl.back(), dirName, false ) )
	{
		// wildcard on this one too
		dirName += "\\*";

		// open this dir and recurse
		FindHandle dir = m_FileMgr->Find( m_DirFinderColl.back(), dirName, FINDFILTER_DIRECTORIES );
		if ( dir )
		{
			// cool, add it and recurse
			m_DirFinderColl.push_back( dir );
			return ( Advance() );
		}
	}
	else
	{
		// ok let's look for files in here
		m_FileFinder = m_FileMgr->Find( m_DirFinderColl.back(), m_FilePattern, FINDFILTER_FILES );

		// remove this level no matter what
		m_FileMgr->Close( m_DirFinderColl.back() );
		m_DirFinderColl.pop_back();

		// either done or advance dir iter
		return ( m_FileFinder ? true : Advance() );
	}

	// failed
	return ( false );
}

//////////////////////////////////////////////////////////////////////////////
// struct FindData implementation

void FindData :: Fill( const WIN32_FIND_DATA& src )
{
	m_Attributes     = src.dwFileAttributes;
	m_CreationTime   = src.ftCreationTime;
	m_LastAccessTime = src.ftLastAccessTime;
	m_LastWriteTime  = src.ftLastWriteTime;
	m_Size           = src.nFileSizeLow;
	m_Name           = src.cFileName;
	m_IndexPtr       = IndexPtr::Null;
#	if !GP_RETAIL
	m_Description    . clear();
#	endif // !GP_RETAIL
}

//////////////////////////////////////////////////////////////////////////////
// class IFileMgr implementation

IFileMgr* IFileMgr::ms_MasterFileMgr = NULL;
IFileMgr::AliasColl IFileMgr::ms_AliasColl;
IFileMgr::CopyLocalColl IFileMgr::ms_CopyLocalColl;

void IFileMgr :: AddFileTypeAlias( const char* fromType, const char* toType )
{
	gpassert( fromType && *fromType && toType && *toType );
	gpassert( ::strlen( toType ) < MAX_ALIAS_EXTENSION_SIZE );	// nothing excessive

	// find existing
	AliasColl::iterator i, ibegin = ms_AliasColl.begin(), iend = ms_AliasColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->m_From.same_no_case( fromType ) )
		{
			// make sure it doesn't exist
			StringColl::const_iterator j, jbegin = i->m_ToColl.begin(), jend = i->m_ToColl.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( j->same_no_case( toType ) )
				{
					return;
				}
			}

			break;
		}
	}

	// add new if none
	if ( i == iend )
	{
		i = ms_AliasColl.insert( ms_AliasColl.end() );
		i->m_From = fromType;
		i->m_From.to_lower();
	}

	// insert entry
	i->m_ToColl.push_back( toType );
	i->m_ToColl.back().to_lower();
}

void IFileMgr :: RemoveFileTypeAlias( const char* fromType, const char* toType )
{
	gpassert( fromType && *fromType && toType && *toType );

	AliasColl::iterator i, ibegin = ms_AliasColl.begin(), iend = ms_AliasColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->m_From.same_no_case( fromType ) )
		{
			StringColl::iterator j, jbegin = i->m_ToColl.begin(), jend = i->m_ToColl.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( j->same_no_case( toType ) )
				{
					i->m_ToColl.erase( j );
					if ( i->m_ToColl.empty() )
					{
						ms_AliasColl.erase( i );
					}
					break;
				}
			}
			break;
		}
	}
}

static const IFileMgr::StringColl* GetFileTypeAliasesHelper( const char* fileName, int length, int* offset )
{
	// check for aliases
	if ( (length > 2) && (fileName[ length - 1 ] == '%') )
	{
		// find begin
		for ( const char* start = fileName + length - 2 ; start != fileName ; --start )
		{
			if ( *start == '%' )
			{
				++start;

				// find an alias set
				const IFileMgr::AliasColl& aliases = IFileMgr::GetFileTypeAliases();
				IFileMgr::AliasColl::const_iterator i, ibegin = aliases.begin(), iend = aliases.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					int startOffset = start - fileName;
					if ( i->m_From.same_no_case( start, length - startOffset - 1 ) )
					{
						if ( offset != NULL )
						{
							*offset = startOffset - 1;
						}

						return ( &i->m_ToColl );
					}
				}

				break;
			}
		}
	}

	return ( NULL );
}

const IFileMgr::StringColl* IFileMgr :: GetFileTypeAliases( const gpstring& fileName, int* offset )
{
	return ( GetFileTypeAliasesHelper( fileName, fileName.length(), offset ) );
}

const IFileMgr::StringColl* IFileMgr :: GetFileTypeAliases( const gpdumbstring& fileName, int* offset )
{
	return ( GetFileTypeAliasesHelper( fileName, fileName.length(), offset ) );
}

const IFileMgr::StringColl* IFileMgr :: GetFileTypeAliases( const char* fileName, int* offset )
{
	gpassert( fileName != NULL );
	return ( GetFileTypeAliasesHelper( fileName, ::strlen( fileName ), offset ) );
}

bool IFileMgr :: IsAliasedFrom( const char* fileName, const char* alias )
{
	const char* ext = GetExtension( fileName );
	if ( !ext )
	{
		return ( false );
	}

	const StringColl* coll = GetFileTypeAliases( alias );
	if ( coll != NULL )
	{
		StringColl::const_iterator i, ibegin = coll->begin(), iend = coll->end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i->same_no_case( ext ) )
			{
				return ( true );
			}
		}
	}

	return ( false );
}

void IFileMgr :: AddCopyLocalType( const char* type )
{
	gpassert( type && *type );

#	if GP_DEBUG
	CopyLocalColl::const_iterator i, ibegin = ms_CopyLocalColl.begin(), iend = ms_CopyLocalColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		gpassert( !i->same_no_case( type ) );
	}
#	endif // GP_DEBUG

	ms_CopyLocalColl.push_back( type );
	ms_CopyLocalColl.back().to_lower();
}

void IFileMgr :: RemoveCopyLocalType( const char* type )
{
	gpassert( type && *type );

	CopyLocalColl::iterator i, ibegin = ms_CopyLocalColl.begin(), iend = ms_CopyLocalColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->same_no_case( type ) )
		{
			ms_CopyLocalColl.erase( i );
			break;
		}
	}
}

void IFileMgr :: SetMasterFileMgr( ThisType* mgr )
{
	gpassert( ( ms_MasterFileMgr == NULL ) && ( mgr != NULL ) );
	ms_MasterFileMgr = mgr;
}

void IFileMgr :: ReleaseMasterFileMgr( ThisType* mgr )
{
	gpassert( ( ms_MasterFileMgr != NULL ) && ( ms_MasterFileMgr == mgr ) );
	ms_MasterFileMgr = NULL;
}

//////////////////////////////////////////////////////////////////////////////
// class CmdProcessor implementation

#if !GP_RETAIL

CmdProcessor :: CmdProcessor( IFileMgr* mgr, ReportSys::ContextRef ctx )
	: m_Context( ctx )
{
	SetFileMgr( mgr );
}

void CmdProcessor :: SetFileMgr( IFileMgr* mgr )
{
	m_FileMgr = mgr ? mgr : IFileMgr::GetMasterFileMgr();
}

void CmdProcessor :: SetReportContext( ReportSys::ContextRef ctx )
{
	m_Context = ctx;
}

bool CmdProcessor :: Dir( const char* pattern )
{
	gpassert( pattern != NULL );

	// set pattern
	if ( *pattern == '\0' )
	{
		pattern = "*";
	}
	gpstring localPattern( pattern );
	if ( !m_CurrentDir.empty() )
	{
		localPattern.insert( 0U, 1U, '\\' );
		localPattern.insert( 0, m_CurrentDir );
	}

	// get vars
	int totalSize = 0, dirCount = 0, fileCount = 0;
	FindDataList findDataList;

	// dir of subdirectories
	bool ok = false;
	FindHandle find = m_FileMgr->Find( localPattern, FINDFILTER_DIRECTORIES );
	if ( find )
	{
		// out header
		OutputDirHeader( localPattern, dirCount, findDataList );

		// out dir
		ok = true;
		GetFindResults( find, totalSize, dirCount, findDataList );

		// close
		m_FileMgr->Close( find );
	}

	// dir of files
	find = m_FileMgr->Find( localPattern, FINDFILTER_FILES );
	if ( find )
	{
		// out header
		if ( !ok )
		{
			OutputDirHeader( localPattern, dirCount, findDataList );
		}

		// out dir
		ok = true;
		GetFindResults( find, totalSize, fileCount, findDataList );

		// close
		m_FileMgr->Close( find );
	}

	// output
	int nameWidth = 0;
	FindDataList::const_iterator i, ibegin = findDataList.begin(), iend = findDataList.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		::maximize( nameWidth, (int)i->m_Name.length() );
	}
	for ( i = ibegin ; i != iend ; ++i )
	{
		OutputDirEntry( *i, nameWidth );
	}

	// summary
	m_Context->OutputF( "%12d bytes in %d files and %d dirs\n\n", totalSize, fileCount, dirCount );

	// done
	return ( ok );
}

bool CmdProcessor :: Cd( const char* dir )
{
	gpassert( dir != NULL );

	// get dir, make it nice
	gpstring localDir( dir );
	stringtool::RemoveTrailingBackslash( localDir );
	std::replace( localDir.begin_split(), localDir.end_split(), '/', '\\' );

	// parent dir?
	if ( localDir.same_with_case( ".." ) )
	{
		localDir = m_CurrentDir;
		size_t end = localDir.rfind( '\\' );
		if ( end == gpstring::npos )
		{
			end = 0;
		}
		localDir.erase( end );
	}
	else if ( localDir.same_with_case( "." ) )
	{
		localDir = m_CurrentDir;
	}
	else
	{
		// if relative, insert current directory
		if ( !localDir.empty() && (localDir[ 0 ] != '\\') && !m_CurrentDir.empty() )
		{
			localDir.insert( 0U, 1U, '\\' );
			localDir.insert( 0, m_CurrentDir );
		}
	}

	// see if it exists
	bool success = false;
	if ( localDir.empty() || DoesResourcePathExist( localDir, m_FileMgr ) )
	{
		// take it
		m_CurrentDir = localDir;
		success = true;
	}

	// done
	return ( success );
}

int CmdProcessor :: Copy( const char* pattern, const char* outDir )
{
	gpassert( pattern != NULL );

	// get out dir
	gpstring localOutDir;
	if ( outDir && *outDir )
	{
		localOutDir = outDir;
		stringtool::AppendTrailingBackslash( localOutDir );
	}
	else
	{
		localOutDir = GetCurrentDirectory();
	}

	// set pattern
	if ( *pattern == '\0' )
	{
		pattern = "*";
	}
	gpstring localPattern( pattern );
	if ( !m_CurrentDir.empty() )
	{
		localPattern.insert( 0U, 1U, '\\' );
		localPattern.insert( 0, m_CurrentDir );
	}

	// get vars
	int totalSize = 0, fileCount = 0;

	// dir of files
	FindHandle find = m_FileMgr->Find( localPattern, FINDFILTER_FILES );
	if ( find )
	{
		// for each entry...
		gpstring name;
		while ( m_FileMgr->GetNext( find, name, true ) )
		{
			bool opened = false;

			// attempt open and map
			FileHandle file = m_FileMgr->Open( name );
			if ( file )
			{
				MemHandle mem = m_FileMgr->Map( file );
				if ( mem )
				{
					opened = true;

					// attempt create of output file
					gpstring outFileName( localOutDir + GetFileName( name ) );
					File outFile;
					if ( outFile.CreateNew( outFileName ) )
					{
						// have to touch the file - windows catches exceptions in the WriteFile function
						m_FileMgr->Touch( mem );

						// attempt write
						if ( !outFile.Write( m_FileMgr->GetData( mem ), m_FileMgr->GetSize( mem ) ) )
						{
							gperrorf(( "Unable to write %d bytes to output file '%s', error is '%s'\n",
									   m_FileMgr->GetSize( mem ), outFileName.c_str(), stringtool::GetLastErrorText().c_str() ));
						}

						// update counts
						++fileCount;
						totalSize += m_FileMgr->GetSize( mem );
					}
					else
					{
						gperrorf(( "Unable to create output file '%s', error is '%s'\n",
								   outFileName.c_str(), stringtool::GetLastErrorText().c_str() ));
					}

					m_FileMgr->Close( mem );
				}
				m_FileMgr->Close( file );
			}

			// report opening error
			if ( !opened )
			{
				gperrorf(( "Unable to open file '%s' for output, error is '%s'\n",
						   name.c_str(), stringtool::GetLastErrorText().c_str() ));
			}
		}

		// close
		m_FileMgr->Close( find );
	}

	// summary
	m_Context->OutputF( "%12d bytes in %d files copied to '%s'\n\n",
						totalSize, fileCount, localOutDir.c_str() );

	// done
	return ( fileCount );
}

void CmdProcessor :: GetFindResults( FindHandle find, int& totalSize, int& totalFiles, FindDataList& findDataList )
{
	FindData data;
	while ( m_FileMgr->GetNext( find, data ) )
	{
		findDataList.push_back( data );
		++totalFiles;
		totalSize += data.m_Size;
	}
}

void CmdProcessor :: OutputDirEntry( const FindData& findData, int nameWidth )
{
	static const int SIZE_WIDTH = 10;

	// adjust to local time
	FILETIME utcTime;
	::FileTimeToLocalFileTime( &findData.m_LastWriteTime, &utcTime );
	SYSTEMTIME time;
	::FileTimeToSystemTime( &utcTime, &time );

	// calc hour
	int hour = time.wHour % 12;
	if ( hour == 0 )
	{
		hour = 12;
	}

	// get size
	gpstring sizeText;
	if ( findData.m_Attributes & FILE_ATTRIBUTE_DIRECTORY )
	{
		sizeText.appendf( "%*s", SIZE_WIDTH, "<DIR>  " );
	}
	else
	{
		sizeText.appendf( "%*d", SIZE_WIDTH, findData.m_Size );
	}

	// add compression info
	if ( (findData.m_Attributes & FILE_ATTRIBUTE_COMPRESSED) && (findData.m_Size > 0) )
	{
		int ratio = 100 - int( findData.m_CompressedSize * 100.0f / findData.m_Size );
		sizeText.appendf( " %3d%%", ratio );
	}
	else
	{
		sizeText.appendf( "     " );
	}

	// get compression char
	char compressChar = '_';
	if ( findData.m_Attributes & FILE_ATTRIBUTE_COMPRESSED )
	{
		if ( findData.m_Attributes & FILE_ATTRIBUTE_COMPRESSED_ZLIB )
		{
			compressChar = 'Z';
		}
		else if ( findData.m_Attributes & FILE_ATTRIBUTE_COMPRESSED_LZO )
		{
			compressChar = 'L';
		}
		else
		{
			compressChar = 'C';
		}
	}

	// output the entry
	m_Context->OutputF(
			"%2d/%02d/%02d  %2d:%02d%c  %s   %c%c  %-*.*s  %s\n",
			time.wMonth, time.wDay, time.wYear % 100,
			hour, time.wMinute, (time.wHour > 12) ? 'p' : 'a',
			sizeText.c_str(),
			(findData.m_Attributes & FILE_ATTRIBUTE_DIRECTORY) ? 'D' : '_',
			compressChar,
			nameWidth, nameWidth, findData.m_Name.c_str(),
			findData.m_Description.c_str() );
}

void CmdProcessor :: OutputDirHeader( const char* dir, int& totalDirs, FindDataList& findDataList )
{
	m_Context->OutputF( "\n Directory of \\%s\n\n", dir );

	// build parent dir name
	gpstring parentDir( dir );
	size_t found = parentDir.find_last_of( "\\" );
	if ( found == gpstring::npos )
	{
		return;
	}

	gpstring pattern( parentDir.substr( found + 1 ) );
	parentDir.erase( found );

	// look up time/date of parent
	FindHandle find = m_FileMgr->Find( parentDir );
	FindData data;
	m_FileMgr->GetNext( find, data );
	m_FileMgr->Close( find );

	// out the entries
	if (   stringtool::IsDosWildcardMatch( pattern, "." )
		|| stringtool::IsDosWildcardMatch( pattern, ".." ) )
	{
		data.m_Name = ".";
		findDataList.push_back( data );
		data.m_Name = "..";
		findDataList.push_back( data );
		totalDirs += 2;
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FileSys

//////////////////////////////////////////////////////////////////////////////

#if !GP_RETAIL

void DbgDir( const char* path )
{
	FileSys::CmdProcessor().Dir( path );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
