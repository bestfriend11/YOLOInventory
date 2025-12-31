//////////////////////////////////////////////////////////////////////////////
//
// File     :  TankStructure.cpp
// Author(s):  Scott Bilas
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "TankStructure.h"

#include "FileSysDefs.h"
#include "FuBiSchema.h"
#include "FuBiTraitsImpl.h"
#include "ReportSys.h"
#include "StringTool.h"
#include "StdHelp.h"

#include <objbase.h>

//////////////////////////////////////////////////////////////////////////////
// TankConstants implementations

namespace TankConstants  {  // begin of namespace TankConstants

gpstring ToString( ePriority priority )
{
	gpstring str;

	if ( priority >= PRIORITY_USER )
	{
		str = "User";
		if ( priority != PRIORITY_USER )
		{
			str.appendf( " + %d", priority - PRIORITY_USER );
		}
	}
	else if ( priority >= PRIORITY_PATCH )
	{
		str = "Patch";
		if ( priority != PRIORITY_PATCH )
		{
			str.appendf( " + %d", priority - PRIORITY_PATCH );
		}
	}
	else if ( priority >= PRIORITY_EXPANSION )
	{
		str = "Expansion";
		if ( priority != PRIORITY_EXPANSION )
		{
			str.appendf( " + %d", priority - PRIORITY_EXPANSION );
		}
	}
	else if ( priority >= PRIORITY_LANGUAGE )
	{
		str = "Language";
		if ( priority != PRIORITY_LANGUAGE )
		{
			str.appendf( " + %d", priority - PRIORITY_LANGUAGE );
		}
	}
	else
	{
		str = "Factory";
		if ( priority != PRIORITY_FACTORY )
		{
			str.appendf( " + %d", priority - PRIORITY_FACTORY );
		}
	}

	return ( str );
}

bool FromString( const char* str, ePriority& priority )
{
	const char* begin = str;
	const char* end = begin;
	while ( ::isalpha( *end ) )  ++end;

	if ( same_no_case( "factory", begin, end - begin ) )
	{
		priority = PRIORITY_FACTORY;
	}
	else if ( same_no_case( "language", begin, end - begin ) )
	{
		priority = PRIORITY_LANGUAGE;
	}
	else if ( same_no_case( "expansion", begin, end - begin ) )
	{
		priority = PRIORITY_EXPANSION;
	}
	else if ( same_no_case( "patch", begin, end - begin ) )
	{
		priority = PRIORITY_PATCH;
	}
	else if ( same_no_case( "user", begin, end - begin ) )
	{
		priority = PRIORITY_USER;
	}
	else
	{
		return ( false );
	}

	const char* extra = end;
	while ( ::isspace( *extra ) )  ++extra;

	if ( (*extra == '+') || (*extra == '-') )
	{
		++extra;
		while ( ::isspace( *extra ) )  ++extra;

		int extraValue = 0;
		if ( ::FromString( extra, extraValue ) )
		{
			priority = (ePriority)(priority + extraValue);
		}
		else
		{
			return ( false );
		}
	}

	return ( true );
}

static const stringtool::BitEnumStringConverter::Entry s_TankFlagEntries[] =
{
	{  "none",                   TANKFLAG_NONE                    },
	{  "non_retail",             TANKFLAG_NON_RETAIL              },
	{  "allow_multiplayer_xfer", TANKFLAG_ALLOW_MULTIPLAYER_XFER  },
};

static stringtool::BitEnumStringConverter s_TankFlagConverter(
		s_TankFlagEntries, ELEMENT_COUNT( s_TankFlagEntries ), TANKFLAG_NONE );

gpstring TankConstants::ToString( eTankFlags flags )
	{  return ( s_TankFlagConverter.ToFullString( flags ) );  }
bool TankConstants::FromString( const char* str, eTankFlags& flags )
	{  return ( s_TankFlagConverter.FromFullString( str, rcast <DWORD&> ( flags ) ) );  }


}  // end of namespace TankConstants

//////////////////////////////////////////////////////////////////////////////

namespace Tank  {  // begin of namespace Tank

using namespace TankConstants;

//////////////////////////////////////////////////////////////////////////////
// struct NSTRING implementation

NSTRING* NSTRING :: Create( int len )
{
	gpassert( len >= 0 );

	// create and zero object
	size_t size = GetSize( len );
	BYTE* mem = new BYTE[ size ];
	::ZeroMemory( mem, size );

	// setup
	NSTRING* str = rcast <NSTRING*> ( mem );
	str->m_Length = (WORD)len;

	// return it
	return ( str );
}

NSTRING* NSTRING :: Create( const char* str, int len )
{
	gpassert( str != NULL );
	if ( len < 0 )
	{
		len = ::strlen( str );
	}

	// create object
	NSTRING* nstr = Create( len );
	::strncpy( nstr->m_Text, str, len );

	// return it
	return ( nstr );
}

//////////////////////////////////////////////////////////////////////////////
// struct WNSTRING implementation

WNSTRING* WNSTRING :: Create( int len )
{
	gpassert( len >= 0 );

	// create and zero object
	size_t size = GetSize( len );
	BYTE* mem = new BYTE[ size ];
	::ZeroMemory( mem, size );

	// setup
	WNSTRING* str = rcast <WNSTRING*> ( mem );
	str->m_Length = (WORD)len;

	// return it
	return ( str );
}

WNSTRING* WNSTRING :: Create( const wchar_t* str, int len )
{
	gpassert( str != NULL );
	if ( len < 0 )
	{
		len = ::wcslen( str );
	}

	// create object
	WNSTRING* nstr = Create( len );
	::wcsncpy( nstr->m_Text, str, len );

	// return it
	return ( nstr );
}

//////////////////////////////////////////////////////////////////////////////
// struct RawIndex::Header implementation

void RawIndex::Header :: Init( void )
{
	// clear this
	::ZeroObject( *this );

	// set up simple members
	m_ProductId     = FileSys::PRODUCT_ID;
	m_TankId        = TANK_ID;
	m_HeaderVersion = HEADER_VERSION;
	m_Priority      = PRIORITY_USER;

	// finish
	::CoCreateGuid( &m_GUID );
	::GetSystemTime( &m_UtcBuildTime );
	stringtool::CopyString( m_CopyrightText, ::ToUnicode( COPYRIGHT_TEXT ), ELEMENT_COUNT( m_CopyrightText ) );
	stringtool::CopyString( m_AuthorText, ::ToUnicode( SysInfo::GetUserName() ), ELEMENT_COUNT( m_AuthorText ) );
}

bool RawIndex::Header :: ShouldOverride( const Header& other ) const
{
	// first check priority
	if ( other.m_Priority < m_Priority )
	{
		return ( true );
	}

	// maybe higher minimum version required
	if ( other.m_MinimumVersion < m_MinimumVersion )
	{
		return ( true );
	}

	// ok, well try the build time
	FILETIME fileTime, otherFileTime;
	if (   ::SystemTimeToFileTime( &m_UtcBuildTime, &fileTime )
		&& ::SystemTimeToFileTime( &other.m_UtcBuildTime, &otherFileTime ) )
	{
		// compare it
		if ( ::CompareFileTime( &fileTime, &otherFileTime ) > 0 )
		{
			return ( true );
		}
	}

	// guess it's not
	return ( false );
}

//////////////////////////////////////////////////////////////////////////////
// class TankFile implementation

TankFile :: TankFile( void )
{
	InternalClose();
}

TankFile :: ~TankFile( void )
{
	Close();
}

bool TankFile :: Open( const char* name, bool verifyIndex )
{
	gpassert( name != NULL );	// parameter validation

// Open the tank.

	DiyFileMap tempMap;

	// get some vars
	bool success           = Close();
	File::eAccess faccess  = File::ACCESS_READ_ONLY;
	File::eSharing sharing = File::SHARE_READ_ONLY;
	DWORD flags            = FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN;		// generally sequential access

	// open the file
	if ( !m_TankFile.OpenExisting( name, faccess, sharing, flags ) )
	{
		goto error;
	}

// Figure out if it's PE or RAW and load it.

	if ( !tempMap.Create(
			DiyFileMap::OPTION_NONE,
			DiyFileMap::ACCESS_READ_ONLY,
			m_TankFile,
			0,
			sizeof( Header ) ) )
	{
		::SetLastError( ERROR_BAD_FORMAT );
		goto error;
	}

	if ( !OpenPrivate( tempMap, verifyIndex ) )
	{
		goto error;
	}

	// figure out location of tank
	m_Location = FileSys::GetLocation( name );
	if ( m_Location != FileSys::LOCATION_ERROR )
	{
		// keep
		m_Location |= FileSys::LOCATION_TANK;
		m_FileName    = FileSys::GetFullPathName( name );
		m_FileAccess  = faccess;
		m_FileSharing = sharing;
		m_FileFlags   = flags;
		m_FileSize    = m_TankFile.GetSize();

		// store tank find params
		FileSys::FileFinder finder( name );
		if ( finder.Next() )
		{
			// get the find data for the entire tank
			m_FindData.Fill( finder.Get() );

			// update creation/modified time with actual tank creation time
			::SystemTimeToFileTime( &m_Header.m_UtcBuildTime, &m_FindData.m_CreationTime );
			m_FindData.m_LastWriteTime = m_FindData.m_CreationTime;

			// set its description to be my name
#			if !GP_RETAIL
			m_FindData.m_Description = "[T]" + m_FileName;
#			endif // !GP_RETAIL
		}
		else
		{
			goto error;
		}
	}
	else
	{
		::SetLastError( ERROR_PATH_NOT_FOUND );
		goto error;
	}

	// original success of Close() determines if this is a complete success
	return ( success );

// Error case.

error:

	// out error message
	DWORD error = ::GetLastError();
	if ( error != ERROR_BAD_FORMAT )
	{
		gperrorf(( "TankFile::Open( %s ) - failed to open tank (%s)\n",
					  name, stringtool::GetLastErrorText( error ).c_str() ));
	}

	// close out handles
	tempMap.Close();
	InternalClose( error );

	return ( false );
}

bool TankFile :: Close( void )
{
	bool success = true;

	if ( IsOpen() )
	{
		success = InternalClose() == ERROR_SUCCESS;
	}

	return ( success );
}

bool TankFile :: VerifyIndex( void )
{
	gpassert( IsOpen() );

	if ( m_Header.m_IndexCRC32 != 0 )
	{
		DWORD crc = ::GetCRC32( rcast <const BYTE*> ( m_TankIndex.GetData() ), m_Header.m_IndexSize );
		if ( crc != m_Header.m_IndexCRC32 )
		{
			::SetLastError( ERROR_CRC );
			return ( false );
		}
	}

	return ( true );
}

HANDLE TankFile :: CreateFile( void ) const
{
	if ( !IsOpen() )
	{
		return ( INVALID_HANDLE_VALUE );
	}

	return ( ::CreateFile( 
				m_FileName,
				m_FileAccess,
				m_FileSharing,
				NULL,
				OPEN_EXISTING,
				m_FileFlags,
				NULL ) );
}

struct CompareEntries
{
	const TankFile* m_TankFile;

	CompareEntries( const TankFile* tankFile )  {  m_TankFile = tankFile;  }

	int Compare( const gpstring& lname, DWORD ro )
	{
		// first deref the pointer
		const void* rv = m_TankFile->GetIndexOffset( ro );

		// get the name
		gpassert( m_TankFile->IsDirEntry( rv ) || m_TankFile->IsFileEntry( rv ) );
		const NSTRING* rname = m_TankFile->IsDirEntry( rv ) ? &rcast <const RawIndex::DirEntry *> ( rv )->m_Name
															: &rcast <const RawIndex::FileEntry*> ( rv )->m_Name;
		// return comparison
		return ( compare_with_case( lname, *rname ) );
	}

	bool operator () ( const gpstring& lname, DWORD ro )  {  return ( Compare( lname, ro ) < 0 );  }
	bool operator () ( DWORD lo, const gpstring& rname )  {  return ( Compare( rname, lo ) > 0 );  }
};

const void* TankFile :: FindIn( const DirEntry* dirEntry, const gpstring& name ) const
{
	// preconditions and parameter validation
	gpassert( IsOpen() );
	gpassert( name.is_lower() );

	// check for local/parent
	if ( name[ 0 ] == '.' )
	{
		if ( name[ 1 ] == '\0' )
		{
			return ( dirEntry );
		}
		else if ( (name[ 1 ] == '.') && (name[ 2 ] == '\0') )
		{
			if ( dirEntry->m_ParentOffset != 0 )
			{
				return ( GetIndexOffset( dirEntry->m_ParentOffset ) );
			}
			else
			{
				return ( dirEntry );
			}
		}
	}

	// bsearch for the entry
 	const DWORD* begin = dirEntry->GetOffsets();
	const DWORD* end   = begin + dirEntry->m_ChildCount;
	const DWORD* found = stdx::binary_search( begin, end, name, CompareEntries( this ) );

	// return it if found
	return ( (found != end) ? GetIndexOffset( *found ) : NULL );
}

const RawIndex::DirEntry* TankFile :: GetParent( const void* entry ) const
{
	// first element of entry is the parent offset
	DWORD parentOffset = *rcast <const DWORD*> ( entry );
	if ( parentOffset != 0 )
	{
		// get parent
		const void* rawEntry = GetIndexOffset( parentOffset );

		// must be a dir, return it
		gpassert( IsDirEntry( rawEntry ) );
		return ( rcast <const DirEntry*> ( rawEntry ) );
	}
	else
	{
		// already at root
		return ( NULL );
	}
}

const NSTRING& TankFile :: GetName( const void* entry ) const
{
	return ( IsDirEntry( entry ) ? rcast <const DirEntry *> ( entry )->m_Name
								 : rcast <const FileEntry*> ( entry )->m_Name );
}

void TankFile :: BuildFileName( gpstring& str, const void* entry, bool prependPath ) const
{
	int totalInsert = 0;

	// figure out how long path prefix is going to be (don't include root)
	if ( prependPath )
	{
		for (  const DirEntry* i = GetParent( entry )
			 ; (i != NULL) && (i->m_ParentOffset != 0)
			 ; i = GetParent( i ) )
		{
			totalInsert += i->m_Name.length() + 1;		// plus slash
		}
	}

	// get the name
	const NSTRING& name = GetName( entry );

	// create string and copy in the filename
	str.resize( totalInsert + name.length() );
	::memcpy( str.begin_split() + totalInsert, name.c_str(), name.length() );

	// copy in a dir at a time
	if ( prependPath )
	{
		for (  const DirEntry* i = GetParent( entry )
			 ; (i != NULL) && (i->m_ParentOffset != 0)
			 ; i = GetParent( i ) )
		{
			str.at_split( --totalInsert ) = '\\';

			const NSTRING& thisDir = i->m_Name;
			int thisLen = thisDir.length();

			totalInsert -= thisLen;
			::memcpy( str.begin_split() + totalInsert, thisDir.c_str(), thisLen );
		}
	}
}

bool TankFile :: ShouldOverride( const TankFile& other ) const
{
	gpassert( IsOpen() && other.IsOpen() );
	return ( m_Header.ShouldOverride( other.GetHeader() ) );
}

#if !GP_RETAIL

template <typename T>
void OutputField( ReportSys::ContextRef ctx, const char* name, const T& value, FuBi::eXfer xfer = FuBi::XFER_NORMAL )
{
	ReportSys::AutoReport autoReport( ctx );
	ctx->OutputField( name );
	ctx->OutputFieldObj( value, xfer );
}

struct ReportEntry
{
	gpstring m_FileName;
	DWORD    m_Size;
	DWORD    m_CRC32;
};

void TankFile :: Dump( ReportSys::ContextRef ctx )
{
	gpassert( IsOpen() );

	// build report sink
	ReportSys::TextTableSink sink;
	ReportSys::LocalContext lctx( &sink, false );
	lctx.SetSchema( new ReportSys::Schema( 2, "Name", "Value" ), true );

	// get local time
	FILETIME utcFileTime;
	::SystemTimeToFileTime( &m_Header.m_UtcBuildTime, &utcFileTime );
	FILETIME fileTime;
	::FileTimeToLocalFileTime( &utcFileTime, &fileTime );
	SYSTEMTIME localBuildTime;
	::FileTimeToSystemTime( &fileTime, &localBuildTime );

	// store header
	OutputField( lctx, "ProductId",       REVERSE_FOURCC( m_Header.m_ProductId ), FuBi::XFER_FOURCC );
	OutputField( lctx, "TankId",          REVERSE_FOURCC( m_Header.m_TankId ), FuBi::XFER_FOURCC );
	OutputField( lctx, "HeaderVersion",   gpstringf( "%d.%d", GETVERSIONMAJOR( m_Header.m_HeaderVersion ), GETVERSIONMINOR( m_Header.m_HeaderVersion ) ) );
	OutputField( lctx, "DirSetOffset",    m_Header.m_DirSetOffset, FuBi::XFER_HEX );
	OutputField( lctx, "FileSetOffset",   m_Header.m_FileSetOffset, FuBi::XFER_HEX );
	OutputField( lctx, "IndexSize",       m_Header.m_IndexSize );
	OutputField( lctx, "DataOffset",      m_Header.m_DataOffset, FuBi::XFER_HEX );
	OutputField( lctx, "ProductVersion",  ::ToString( m_Header.m_ProductVersion, gpversion::MODE_AUTO_LONG ) );
	OutputField( lctx, "MinimumVersion",  ::ToString( m_Header.m_MinimumVersion, gpversion::MODE_AUTO_LONG ) );
	OutputField( lctx, "Priority",        ::ToString( (ePriority)m_Header.m_Priority ) );
	OutputField( lctx, "Flags",           ::ToString( (eTankFlags)m_Header.m_Flags ) );
	OutputField( lctx, "CreatorId",       m_Header.m_CreatorId, FuBi::XFER_FOURCC );
	OutputField( lctx, "GUID",            m_Header.m_GUID );
	OutputField( lctx, "IndexCRC32",      m_Header.m_IndexCRC32, FuBi::XFER_HEX );
	OutputField( lctx, "DataCRC32",       m_Header.m_DataCRC32, FuBi::XFER_HEX );
	OutputField( lctx, "UtcBuildTime",    m_Header.m_UtcBuildTime );
	OutputField( lctx, "LocalBuildTime",  localBuildTime );
	OutputField( lctx, "CopyrightText",   ::ToAnsi( m_Header.m_CopyrightText ) );
	OutputField( lctx, "BuildText",       ::ToAnsi( m_Header.m_BuildText ) );
	OutputField( lctx, "TitleText",       ::ToAnsi( m_Header.m_TitleText ) );
	OutputField( lctx, "AuthorText",      ::ToAnsi( m_Header.m_AuthorText ) );

	if ( m_Description.empty() )
	{
		OutputField( lctx, "DescriptionText", gpstring::EMPTY );
	}

	// out header
	ctx->OutputEol();
	ctx->OutputRepeatWidth( sink.CalcDividerWidth(), "-==- " );
	ctx->Output( "\n\nHeader:\n\n" );

	// out table
	sink.OutputReport( ctx );

	// out desc
	if ( !m_Description.empty() )
	{
		ctx->Output( "\nDescriptionText:\n\n" );
		ReportSys::AutoIndent autoIndent( ctx );
		ctx->Output( ::ToAnsi( m_Description ) );
	}

	// output files
	ctx->Output( "\n\n" );
	ctx->OutputRepeatWidth( sink.CalcDividerWidth(), "-==- " );
	ctx->Output( "\n\nFiles:\n\n" );
	ReportSys::AutoIndent autoIndent( ctx );
	if ( m_FileSet->m_Count != 0 )
	{
		// sort by offset
		typedef std::multimap <DWORD, ReportEntry> ReportDb;
		ReportDb db;

		const DWORD* i, * begin = m_FileSet->m_Offsets, * end = begin + m_FileSet->m_Count;
		for ( i = begin ; i != end ; ++i )
		{
			const FileEntry* file = rcast <const FileEntry*> ( GetFileSetOffset( *i ) );
			gpstring fileName( file->m_Name );
			if ( file->m_ParentOffset != 0 )
			{
				const DirEntry* dir = rcast <const DirEntry*> ( GetIndexOffset( file->m_ParentOffset ) );
				while ( dir->m_ParentOffset != 0 )
				{
					fileName.insert( (size_t)0, (size_t)1, '\\' );
					fileName.insert( 0, dir->m_Name );
					dir = rcast <const DirEntry*> ( GetIndexOffset( dir->m_ParentOffset ) );
				}
			}

			ReportEntry reportEntry;
			reportEntry.m_FileName = fileName;
			reportEntry.m_Size = file->m_Size;
			reportEntry.m_CRC32 = file->m_CRC32;
			db.insert( std::make_pair( file->m_Offset, reportEntry ) );
		}

		ReportDb::const_iterator j, jbegin = db.begin(), jend = db.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			ctx->OutputF(
					"%s (sz=%d, off=0x%08X, crc=0x%08X)\n",
					j->second.m_FileName.c_str(),
					j->second.m_Size,
					j->first,
					j->second.m_CRC32 );
		}
	}
	else
	{
		ctx->Output( "No files in this tank!\n" );
	}
	ctx->OutputEol();
}

#endif // !GP_RETAIL

bool TankFile :: OpenPrivate( DiyFileMap& tempMap, bool verifyIndex )
{
	bool success = false;

	__try
	{
		__try
		{
			const Header* header = rcast <const Header*> ( tempMap.GetData() );
			if ( (header->m_ProductId == FileSys::PRODUCT_ID) && (header->m_TankId == TANK_ID) )
			{
				// remap to get description if any
				if ( !header->m_DescriptionText.empty() )
				{
					if ( !tempMap.Create(
							DiyFileMap::OPTION_NONE,
							DiyFileMap::ACCESS_READ_ONLY,
							m_TankFile,
							0,
							sizeof( Header ) - sizeof( WNSTRING ) + header->m_DescriptionText.GetSize() ) )
					{
						return ( false );
					}
				}

				m_Header = *header;
				m_Description = header->m_DescriptionText;
				success = OpenDirect( verifyIndex );
			}
			else
			{
				success = OpenPE( tempMap, verifyIndex );
			}
		}
		__except( ::GlobalExceptionFilter( GetExceptionInformation(), EXCEPTION_CONTINUE_SEARCH ) )
		{
			// this space intentionally left blank...
		}
	}
	__except( (GetExceptionCode() == EXCEPTION_GPG_FATAL) ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER )
	{
		// this space intentionally left blank...
	}

	return ( success );
}

bool TankFile :: OpenDirect( bool verifyIndex )
{
	// get at the index
	if ( !m_TankIndex.Create( DiyFileMap::OPTION_NONE,
							  DiyFileMap::ACCESS_READ_ONLY,
							  m_TankFile,
							  m_Header.m_DirSetOffset,
							  m_Header.m_IndexSize ) )
	{
		return ( false );
	}
	const BYTE* base = rcast <const BYTE*> ( m_TankIndex.GetData() );

	// map headers
	m_DirSet  = rcast <const DirSet*>  ( base );
	m_FileSet = rcast <const FileSet*> ( base + m_Header.m_FileSetOffset - m_Header.m_DirSetOffset );

	// verify?
	if ( verifyIndex && !VerifyIndex() )
	{
		return ( false );
	}

	// map data
	m_DataOffset = m_Header.m_DataOffset;

	// setup cache
	m_DirFirst  = rcast <const DirEntry* > ( GetIndexOffset  ( m_DirSet->m_Offsets[ 0 ] ) );
	m_DirLast   = rcast <const DirEntry* > ( GetIndexOffset  ( m_DirSet->m_Offsets[ m_DirSet->m_Count - 1 ] ) );
	m_FileFirst = rcast <const FileEntry*> ( GetFileSetOffset( m_FileSet->m_Offsets[ 0 ] ) );
	m_FileLast  = rcast <const FileEntry*> ( GetFileSetOffset( m_FileSet->m_Offsets[ m_FileSet->m_Count - 1 ] ) );

	// looks ok
	return ( true );
}

bool TankFile :: OpenPE( DiyFileMap& tempMap, bool verifyIndex )
{
	UNREFERENCED_PARAMETER( tempMap );
	UNREFERENCED_PARAMETER( verifyIndex );

	::SetLastError( ERROR_BAD_FORMAT );
	return ( false );
}

DWORD TankFile :: InternalClose( DWORD error )
{
	if ( !m_TankIndex.Close() && (error == ERROR_SUCCESS) )
	{
		error = ::GetLastError();
	}
	if ( !m_TankFile.Close() && (error == ERROR_SUCCESS) )
	{
		error = ::GetLastError();
	}

	// clear out other vars
	m_FileName.clear();
	m_Location    = FileSys::LOCATION_UNKNOWN;
	m_FileAccess  = File::ACCESS_READ_ONLY;
	m_FileSharing = File::SHARE_READ_ONLY;
	m_FileFlags   = 0;
	m_FileSize    = 0;
	::ZeroObject( m_Header );
	m_DirSet = NULL;
	m_FileSet = NULL;
	m_DataOffset = 0;

	// clear cache
	m_DirFirst  = NULL;
	m_DirLast   = NULL;
	m_FileFirst = NULL;
	m_FileLast  = NULL;

	// restore error to the first thing that failed
	::SetLastError( error );

	// return error code (may be new)
	return ( error );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Tank

//////////////////////////////////////////////////////////////////////////////
