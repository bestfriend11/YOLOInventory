//////////////////////////////////////////////////////////////////////////////
//
// File     :  TankFileMgr.cpp
// Author(s):  Scott Bilas
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "TankFileMgr.h"

#include "Config.h"
#include "DrWatson.h"
#include "FileSysXfer.h"
#include "GpStatsDefs.h"
#include "ReportSys.h"
#include "StringTool.h"
#include <malloc.h>

#define ZLIB_SUPPORT 1
#define LZO_SUPPORT  1

#if ZLIB_SUPPORT
#include "GpZlib.h"
#endif
#if LZO_SUPPORT
#include "GpLzo.h"
#endif

namespace FileSys  {  // begin of namespace FileSys

using namespace Tank;

//////////////////////////////////////////////////////////////////////////////
// Future

/*
	This class could share a ton of code with PathFileMgr. Hoist out all the
	file/mem stuff into a utility base class. All that pointer arithmetic
	and handle management stuff can be shared.

	Add to that note the sharing of code with MasterFileMgr. All the indexing
	stuff and FindX() functions are nearly identical to MasterFileMgr.
*/

//////////////////////////////////////////////////////////////////////////////
// class CompressedReader declaration and implementation

#if ZLIB_SUPPORT || LZO_SUPPORT

// $ these readers don't need to be thread-safe, they are serialized by the
//   diy maps.

class CompressedReader : public FileSys::Reader
{
public:
	SET_INHERITED( CompressedReader, FileSys::Reader );

	typedef Tank::RawIndex::FileEntry        FileEntry;
	typedef Tank::RawIndex::CompressedHeader CompressedHeader;
	typedef Tank::RawIndex::ChunkHeader      ChunkHeader;

	CompressedReader( FileSys::Reader* takeReader, const TankFileMgr* tankFileMgr, const FileEntry* fileEntry )
	{
		gpassert( takeReader != NULL );
		gpassert( (fileEntry != NULL) && fileEntry->IsCompressed() );

		m_TankFileMgr = tankFileMgr;
		m_Reader      = takeReader;
		m_FileEntry   = fileEntry;
		m_Offset      = 0;
	}

	virtual ~CompressedReader( void )
	{
		delete ( m_Reader );
	}

	virtual int OnInflateProc( mem_ptr out, const_mem_ptr in, const char*& errText ) = 0;

// Overrides.

	enum  {  MAX_REPAIR_TRIES = 5  };

	/*	SPECIAL NOTE

		Ok so apparently a lot of people have either shit systems or they are
		tweaking them so they fuck up on DMA transfers occasionally. "It works
		fine in Quake 3" etc., yeah, this is their first game that actually
		does something with the hardware and it doesn't work, so it's obviously
		our game's fault, right?

		** ANYWAY **

		Dungeon Siege now repairs its (god damned) self. If the compression
		algorithm can detect errors in its data (currently only zlib does this)
		then up to MAX_REPAIR_TRIES times we will re-read in the chunk of data
		and reattempt decompression. If that fails, then it was most likely a
		fuckup during install of the disc, which is also happening for many
		people. This is not a guarantee it will work - when decompressing in
		place a memory overwrite may occur if data is corrupt and the algo
		freaks out. Also, testing shows that zlib's speed derives from a lack of
		safety (which is ok) and it may miss many errors.

		Death to Ov3rCL0CkaRZ!!@!#@#!

		-sb
	*/

	void ReportCorruptFile( const_mem_ptr outBuffer, const_mem_ptr inBuffer, const char* decoderError, bool wasFinalAttempt ) const
	{
		// pick a context, default to logging only
		ReportSys::Context* ctx = &gWarningContext;
		if ( wasFinalAttempt )
		{
			if ( m_TankFileMgr->TestOptions( TankFileMgr::OPTION_DISABLE_DECOMPRESS_FATAL ) )
			{
				// just put up a message box
				ctx = &gErrorBoxContext;
			}
			else
			{
				// last attempt, we have to fatal now
				ReportSys::EnableFatalAsWatson();
				ctx = &gFatalContext;
			}
		}

		// wait until all the text is logged before fatal or dialog box
		ReportSys::AutoReport autoReport( ctx );

		// general error
		ctx->OutputF( $MSG$ "CRC error during decoding!\n"
							"\n"
							"When attempting to decompress data read in from "
							"disk, a CRC error was detected. This may be a "
							"sign of hardware failure, an unstable system, or "
							"a corrupted installation. Please contact "
							"Microsoft technical support for help with "
							"resolving this issue.\n\n" );

		// more specific error
		if ( wasFinalAttempt )
		{
			ctx->OutputF( $MSG$ "%d failed attempts were made to recover from "
								"this problem. It's likely that the data on "
								"the disk is corrupted. This is unrecoverable, "
								"and the app must now shut down.\n\n",
								MAX_REPAIR_TRIES );

			// ok now write out our data
			if ( outBuffer.mem && outBuffer.size && !::IsBadReadPtr( outBuffer.mem, outBuffer.size ) )
			{
				gpstring fileName = DrWatson::GetOutputDirectory() + "OutBuffer.bin";
				FileSys::File file;
				if (   file.CreateAlways( fileName )
					&& file.WriteStruct( outBuffer )
					&& file.Write( outBuffer.mem, outBuffer.size ) )
				{
					DrWatson::RegisterFileForReport( fileName );
				}
			}
			if ( inBuffer.mem && inBuffer.size && !::IsBadReadPtr( inBuffer.mem, inBuffer.size ) )
			{
				gpstring fileName = DrWatson::GetOutputDirectory() + "InBuffer.bin";
				FileSys::File file;
				if (   file.CreateAlways( fileName )
					&& file.WriteStruct( inBuffer )
					&& file.Write( inBuffer.mem, inBuffer.size ) )
				{
					DrWatson::RegisterFileForReport( fileName );
				}
			}
		}
		else
		{
			ctx->OutputF( $MSG$ "We will attempt to recover from this error by "
								"retrying the data copy from disk.\n\n" );
		}

		// part that failed
		ctx->OutputF( $MSG$ "File on disk: '%s'\n"
							"\n"
							"Internal info:\n"
							"  resource = %s\n"
							"  offset = 0x%08X\n"
							"  size = %d\n"
							"  crc = 0x%08X\n"
							"  algo text = %s\n",
							m_TankFileMgr->GetTankFileName().c_str(),
							m_TankFileMgr->GetTankFile().BuildFileName( m_FileEntry, true ).c_str(),
							m_FileEntry->m_Offset,
							m_FileEntry->m_Size,
							m_FileEntry->m_CRC32,
							decoderError ? decoderError : "general corruption" );

		// note the type of failure
		::SetLastError( ERROR_CRC );
	}

	void ReportFileError( const char* operationFmt, ... ) const
	{
		char buffer[ 250 ];
		if ( operationFmt != NULL )
		{
			::_vsnprintf( buffer, ELEMENT_COUNT( buffer ), operationFmt, va_args( operationFmt ) );
		}
		else
		{
			::strcpy( buffer, "unknown" );
		}

		DWORD lastError = ::GetLastError();
		gpwatsonf(( $MSG$ "File system error!\n"
						  "\n"
						  "When attempting to read data from the disk, the "
						  "operating system returned an error code. This may be "
						  "a sign of hardware failure, an unstable system, or a "
						  "corrupted installation. Please contact Microsoft "
						  "technical support for help with resolving this issue.\n"
						  "\n"
						  "File on disk: '%s'\n"
						  "\n"
						  "Internal info:\n"
						  "  resource = %s\n"
						  "  file pointer = 0x%08X\n"
						  "  operation = '%s'\n"
						  "  last error = '%s'\n"
						  "  (0x%08X)\n",
						  m_TankFileMgr->GetTankFileName().c_str(),
						  m_TankFileMgr->GetTankFile().BuildFileName( m_FileEntry, true ).c_str(),
						  m_TankFileMgr->GetTankFile().GetFile().GetPos(),
						  buffer,
						  lastError,
						  stringtool::GetLastErrorText( lastError ).c_str() ));
		::SetLastError( lastError );
	}

	bool ReaderSeek( int offset )
	{
		bool rc = m_Reader->Seek( offset );
		if ( !rc )
		{
			ReportFileError( "seek 0x%08X", offset );
		}
		return ( rc );
	}

	bool ReaderRead( mem_ptr mem )
	{
		bool rc = m_Reader->Read( mem.mem, mem.size );

		// random chance of corruption
#		if 0
		if ( mem.size /*&& (RandomFloat() < 0.01)*/ )
		{
			gWarningContext.OutputF( "*** Corrupting data in '%s'!!!\n", m_TankFileMgr->GetTankFile().BuildFileName( m_FileEntry, true ).c_str() );
			*((BYTE*)mem.mem + Random( mem.size - 1 )) = RandomByte();
		}
#		endif

		if ( !rc )
		{
			ReportFileError( "read 0x%08X 0x%08X", mem.mem, mem.size );
		}
		return ( rc );
	}

	bool ReadIntoCopy( int compressedSize, int fileOffset, void* out, int sizeToRead )
	{
		// build a local buffer
		void* buffer = _alloca( compressedSize );
		mem_ptr inBuffer( buffer, compressedSize );

		// iterate to repair transfer errors
		for ( int tries = 1 ; ; ++tries )
		{
			// seek to our data and read into our buffer
			if ( !ReaderSeek( fileOffset ) || !ReaderRead( inBuffer ) )
			{
				return ( false );
			}

			// decompress (include extra overrun allowed)
			mem_ptr outBuffer( out, sizeToRead + DiyFileMap::OVERRUN_ALLOW );
			const char* errText = NULL;
			if ( OnInflateProc( outBuffer, inBuffer, errText ) != sizeToRead )
			{
				bool finalAttempt = tries == MAX_REPAIR_TRIES;
				ReportCorruptFile( outBuffer, inBuffer, errText, finalAttempt );
				if ( finalAttempt )
				{
					return ( false );
				}
			}
			else
			{
				// success!
				break;
			}
		}

		return ( true );
	}

	virtual bool Read( void* out, int size, int* bytesRead = NULL )
	{
		// if not chunked, do the whole thing
		if ( m_FileEntry->GetChunkSize() == 0 )
		{
			gpassert( GetPos() == 0 );
			gpassert( size == (int)m_FileEntry->m_Size );

			if ( !ReadIntoCopy( m_FileEntry->GetCompressedSize(), 0, out, size ) )
			{
				return ( false );
			}
		}
		else if ( size > (int)m_FileEntry->GetChunkSize() )
		{
			gpassert( size > 0 );

			// one chunk at a time
			int pos = GetPos();
			while ( size > 0 )
			{
				int write = min_t( size, (int)m_FileEntry->GetChunkSize() );
				if ( !Seek( pos ) || !Read( out, write ) )
				{
					return ( false );
				}

				size -= write;
				out = (BYTE*)out + write;
				pos += write;
			}
		}
		else
		{
			gpassert( size > 0 );

			// ok figure out which chunk
			const ChunkHeader* chunk = m_FileEntry->GetChunkHeader( m_FileEntry->GetChunkIndex( GetPos() ) );
			gpassert( chunk != NULL );
			gpassert( size == (int)chunk->m_UncompressedSize );

			// compressed?
			if ( chunk->IsCompressed() )
			{
				// last chunk?
				if ( size < (int)m_FileEntry->GetChunkSize() )
				{
					gpassert( chunk->m_ExtraBytes == 0 );

					if ( !ReadIntoCopy( chunk->m_CompressedSize, chunk->m_Offset, out, size ) )
					{
						return ( false );
					}
				}
				else
				{
					// get some vars
					mem_ptr inBuffer( (BYTE*)out + size - chunk->m_CompressedSize, chunk->m_CompressedSize );
					mem_ptr localOut( out, size - chunk->m_ExtraBytes );
					mem_ptr localExtra( (BYTE*)out + size - chunk->m_ExtraBytes, chunk->m_ExtraBytes );

					// iterate to repair transfer errors
					for ( int tries = 1 ; ; ++tries )
					{
						// read into back of buffer
						if ( !ReaderSeek( chunk->m_Offset ) || !ReaderRead( inBuffer ) )
						{
							return ( false );
						}

						// decompress in place
						mem_ptr outBuffer( localOut.mem, localOut.size + chunk->m_ExtraBytes );
						const char* errText = NULL;
						if ( OnInflateProc( outBuffer, inBuffer, errText ) != (int)localOut.size )
						{
							bool finalAttempt = tries == MAX_REPAIR_TRIES;
							ReportCorruptFile( outBuffer, inBuffer, errText, finalAttempt );
							if ( finalAttempt )
							{
								return ( false );
							}
						}
						else
						{
							// read the remainder
							if ( !ReaderRead( localExtra ) )
							{
								return ( false );
							}

							// success!
							break;
						}
					}
				}
			}
			else
			{
				// just read directly in
				if ( !ReaderSeek( chunk->m_Offset ) || !ReaderRead( mem_ptr( out, size ) ) )
				{
					return ( false );
				}
			}
		}

		if ( bytesRead != NULL )
		{
			*bytesRead = size;
		}

		return ( true );
	}

	virtual bool ReadLine( gpstring& /*out*/ )
	{
		gpassert( 0 );		// not supposed to call this!
		return ( false );
	}

	virtual bool Seek( int offset, eSeekFrom origin = SEEKFROM_BEGIN )
	{
		gpassert( (offset == 0) || ::IsAligned( offset, m_FileEntry->GetChunkSize() ) );
		gpassert( origin == SEEKFROM_BEGIN );
		gpassert( (offset >= 0) && (offset <= (int)m_FileEntry->m_Size) );

		m_Offset = offset;
		return ( true );
	}

	virtual int GetPos( void )
	{
		return ( m_Offset );
	}

	virtual int GetSize( void )
	{
		return ( m_FileEntry->m_Size );
	}

private:
	typedef stdx::fast_vector <BYTE> Buffer;

	const TankFileMgr* m_TankFileMgr;
	my Reader*         m_Reader;
	const FileEntry*   m_FileEntry;
	int                m_Offset;

	SET_NO_COPYING( CompressedReader );
};

#endif // ZLIB_SUPPORT || LZO_SUPPORT

#if ZLIB_SUPPORT

class ZLibReader : public CompressedReader
{
public:
	SET_INHERITED( ZLibReader, CompressedReader );

	ZLibReader( FileSys::Reader* takeReader, const TankFileMgr* tankFileMgr, const FileEntry* fileEntry )
		: Inherited( takeReader, tankFileMgr, fileEntry )  {  }

	virtual int OnInflateProc( mem_ptr out, const_mem_ptr in, const char*& errText )
	{
		int rc = Z_ERRNO;

		GPSTATS_SAMPLE( AddFileDecompress( out.size ) );
		if ( m_Streamer.Init() && (m_Streamer.InflateChunk( out, in ) != Z_ERRNO) )
		{
			rc = m_Streamer.End();
		}
		else if ( rc == Z_ERRNO )
		{
			errText = zlib::ZlibRcToString( ::GetLastError() );
		}

		return ( rc );
	}

private:
	zlib::InflateStreamer m_Streamer;

	SET_NO_COPYING( ZLibReader );
};

#endif // ZLIB_SUPPORT

#if LZO_SUPPORT

class LzoReader : public CompressedReader
{
public:
	SET_INHERITED( LzoReader, CompressedReader );

	LzoReader( FileSys::Reader* takeReader, const TankFileMgr* tankFileMgr, const FileEntry* fileEntry )
		: Inherited( takeReader, tankFileMgr, fileEntry )  {  }

	virtual int OnInflateProc( mem_ptr out, const_mem_ptr in, const char*& /*errText*/ )
	{
		GPSTATS_SAMPLE( AddFileDecompress( out.size ) );
		return ( lzo::Inflate( out, in ) );
	}

private:
	zlib::InflateStreamer m_Streamer;

	SET_NO_COPYING( LzoReader );
};

#endif // LZO_SUPPORT

//////////////////////////////////////////////////////////////////////////////
// class TankFileMgr implementation

TankFileMgr::eOptions TankFileMgr::ms_DefaultOptions = TankFileMgr::OPTION_NONE;

TankFileMgr :: TankFileMgr( void )
{
	m_Options = ms_DefaultOptions;

	if ( Config::DoesSingletonExist() )
	{
		if ( gConfig.GetBool( "tank_no_paging", false ) )
		{
			m_Options |= OPTION_NO_PAGING;
		}

		if ( gConfig.GetBool( "tank_no_valloc", false ) )
		{
			m_Options |= OPTION_NO_VIRTUALALLOC;
		}

#		if !GP_RETAIL
		if ( gConfig.GetBool( "tank_validate_crc", false ) )
		{
			m_Options |= OPTION_VALIDATE_CRC;
		}
#		endif // !GP_RETAIL
	}
}

TankFileMgr :: ~TankFileMgr( void )
{
	CloseTank();
}

static void ReportBadIndex( const char* name, DWORD lastError = ::GetLastError() )
{
	gperrorboxf(( $MSG$ "Read error! Resource file is either corrupt or "
						"unreadable, possibly due to hardware failure. Try "
						"replacing or repairing the file (reinstall?) to "
						"fix this problem.\n"
						"\n"
						"File on disk: '%s'\n"
						"\n"
						"Internal info:\n"
						"  last error = 0x%08X\n",
						name,
						lastError ));
}

bool TankFileMgr :: OpenTank( const char* name, bool verifyIndex, bool verifyData )
{
	bool oldSuccess = CloseTank();
	bool success = m_TankFile.Open( name, verifyIndex );

	if ( success )
	{
		if ( verifyData )
		{
			success = VerifyAll();
		}
	}
	else if ( verifyIndex )
	{
		DWORD lastError = ::GetLastError();
		if ( lastError != ERROR_BAD_FORMAT )
		{
			ReportBadIndex( name, lastError );
		}

		// restore last error
		::SetLastError( lastError );
	}

	return ( oldSuccess && success );
}

bool TankFileMgr :: CloseTank( void )
{
	// easy early bailout
	if ( !IsTankOpen() )
	{
		return ( true );
	}

// Check for leaks and deal with them.

	// detect leaks
	bool leakMem  = m_MemHandleMgr .HasUsedHandles();
	bool leakFile = m_FileHandleMgr.HasUsedHandles();
	bool leakFind = m_FindHandleMgr.HasUsedHandles();

#	if ( GP_DEBUG )
	{
		// check for and dump leaks
		gpassertm( !leakMem,  "TankFileMgr: tank closed with memory handles still open!" );
		gpassertm( !leakFile, "TankFileMgr: tank closed with file handles still open!" );
		gpassertm( !leakFind, "TankFileMgr: tank closed with find handles still open!" );

		// dump the leaks
		if ( leakMem || leakFile || leakFind )
		{
			gperror( "** TankFileMgr detected handle leaks **\n" );
//$$			Dump( gErrorContext );
		}
	}
#	endif

	// free up handles
	m_MemHandleMgr .CloseAllHandles( this, MemHandle() );
	m_FileHandleMgr.CloseAllHandles( this, FileHandle() );
	m_FindHandleMgr.CloseAllHandles( this, FindHandle() );

// Close the tank.

	bool success = true;

	// free all open handles
	HandleColl::iterator i, ibegin = m_HandleColl.begin(), iend = m_HandleColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( !::CloseHandle( i->m_File ) )
		{
			success = false;
		}
	}
	m_HandleColl.clear();

	// close the tank
	if ( !m_TankFile.Close() )
	{
		success = false;
	}

	// clear out other vars
	m_ResViewMap.clear();

	// all must succeed for us to consider this a total success
	return ( !leakMem && !leakFile && !leakFind && success );
}

bool TankFileMgr :: CanUseTank( const gpversion& exeVersion )
{
	static bool s_ForceRetail = GP_RETAIL || (Config::DoesSingletonExist() && gConfig.GetBool( "force_retail_content", false ));

	// disallow dev tanks
	if ( s_ForceRetail && (GetTankHeader().m_Flags & TANKFLAG_NON_RETAIL) )
	{
		return ( false );
	}

	// special case for 0'd out version
	if ( exeVersion.IsZero() )
	{
		return ( true );
	}

	// make sure we have minimum version
	return ( !(exeVersion < GetTankMinimumVersion() ) );
}

FILETIME TankFileMgr :: GetTankBuildTime( void ) const
{
	FILETIME fileTime;
	if ( !::SystemTimeToFileTime( &GetTankHeader().m_UtcBuildTime, &fileTime ) )
	{
		::GetFileTime( ccast <ThisType*> ( this )->m_TankFile.GetFile(), &fileTime, NULL, NULL );
	}
	return ( fileTime );
}

gpversion TankFileMgr :: GetTankProductVersion( void ) const
{
	return ( ::ToGpVersion( GetTankHeader().m_ProductVersion ) );
}

gpversion TankFileMgr :: GetTankMinimumVersion( void ) const
{
	return ( ::ToGpVersion( GetTankHeader().m_MinimumVersion ) );
}

const TankFileMgr::RawHeader& TankFileMgr :: GetTankHeader( void ) const
{
	if ( IsTankOpen() )
	{
		return ( m_TankFile.GetHeader() );
	}
	else
	{
		// must have been opened at least once
		gpassert( !m_LastFileName.empty() );
		return ( m_LastHeader );
	}
}

bool TankFileMgr :: HasUsedHandles( void ) const
{
	return (   m_FileHandleMgr.HasUsedHandles()
			|| m_MemHandleMgr .HasUsedHandles()
			|| m_FindHandleMgr.HasUsedHandles() );
}

const TankFileMgr::RawFileEntry* TankFileMgr :: GetRawEntry( FileHandle file ) const
{
	RwCritical::ReadLock readLock( m_RwCritical );

	// get handle data
	const FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
	if ( fileEntry == NULL)  return ( NULL );

	// return pointer
	return ( fileEntry->m_ResView->m_RawFileEntry );
}

const TankFileMgr::RawFileEntry* TankFileMgr :: GetRawEntry( MemHandle mem ) const
{
	RwCritical::ReadLock readLock( m_RwCritical );

	// get handle data
	const MemEntry* memEntry = m_MemHandleMgr.Dereference( mem );
	if ( memEntry == NULL)  return ( NULL );

	// return pointer
	return ( memEntry->m_ResView->m_RawFileEntry );
}

bool TankFileMgr :: ShouldOverride( const TankFileMgr& other ) const
{
	return ( GetTankHeader().ShouldOverride( other.GetTankHeader() ) );
}

#pragma warning ( disable : 4509 )

bool TankFileMgr :: VerifyIndex( bool reportError )
{
	gpassert( IsTankOpen() );

	if ( !m_TankFile.VerifyIndex() )
	{
		if ( reportError )
		{
			ReportBadIndex( GetTankFileName() );
		}
		return ( false );
	}

	return ( true );
}

bool TankFileMgr :: VerifyAll( ProgressCb progressCb )
{
	gpassert( IsTankOpen() );
	bool success = true;

	RecursiveFinder finder( "*", -1, this );
	gpstring path;

	__try
	{
		while ( finder.GetNext( path, true ) )
		{
			if ( !VerifyFile( path ) || (progressCb && !progressCb()) )
			{
				success = false;
				break;
			}
		}
	}
	__except( ::GlobalExceptionFilter( GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER, EXCEPTION_EXECUTE_HANDLER ) )
	{
		success = false;
	}

	return ( success );
}

bool TankFileMgr :: VerifyFile( const char* path )
{
	bool success = true;

	bool oldOption = TestOptions( OPTION_DISABLE_DECOMPRESS_FATAL );
	SetOptions( OPTION_DISABLE_DECOMPRESS_FATAL );

	__try
	{
		// get at our file
		FileSys::FileHandle fh = Open( path );
		FileSys::MemHandle  mh = Map( fh );

		// only bother to check if crc was calc'd and stored properly
		UINT32 realCrc = GetRawEntry( mh )->m_CRC32;
		if ( realCrc != 0 )
		{
			// calc crc
			UINT32 crc = ::GetCRC32( GetData( mh ), GetSize( mh ) );

			// same?
			success = realCrc == crc;
		}

		// close 'em
		Close( mh );
		Close( fh );
	}
	__except( ::GlobalExceptionFilter( GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER, EXCEPTION_EXECUTE_HANDLER ) )
	{
		success = false;
	}

	SetOptions( OPTION_DISABLE_DECOMPRESS_FATAL, oldOption );

	return ( success );
}

bool TankFileMgr :: Enable( bool enable )
{
	bool success = true;

	if ( IsEnabled() != enable )
	{
		if ( enable )
		{
			// try to reopen using old filename
			if ( !OpenTank( m_LastFileName ) )
			{
				success = false;
			}
		}
		else
		{
			// release any handles
			FileSys::CallRebuildCb();

			// save the filename and close the tank
			m_LastFileName = m_TankFile.GetFileName();
			m_LastHeader   = m_TankFile.GetHeader();
			CloseTank();
		}
	}

	if ( success )
	{
		Inherited::Enable( enable );
	}

	return ( success );
}

FileHandle TankFileMgr :: Open( const char* name, eUsage usage )
{

// Open it the old fashioned way.

	FileHandle fileHandle;

	// walk the path
	const char* lastComponent;
	const RawDirEntry* rawDirEntry = FindIndex( NULL, name, lastComponent );
	if ( rawDirEntry != NULL )
	{
		// get the name as lowercase
		gpstring fileName( lastComponent );
		fileName.to_lower();

		// check for aliases
		int offset = 0;
		const StringColl* aliases = GetFileTypeAliases( fileName, &offset );
		const RawFileEntry* fileEntry = NULL;
		if ( aliases != NULL )
		{
			// now loop over aliases
			StringColl::const_iterator i, ibegin = aliases->begin(), iend = aliases->end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				// swap alias
				fileName.replace( offset, gpstring::npos, *i );

				// attempt open
				fileEntry = m_TankFile.FindFileIn( rawDirEntry, fileName );
				if ( fileEntry != NULL )
				{
					break;
				}
			}

			// not found? go back to original
			if ( fileEntry == NULL )
			{
				fileName = lastComponent;
				fileName.to_lower();
			}
		}

		// not found yet? try direct
		if ( fileEntry == NULL )
		{
			fileEntry = m_TankFile.FindFileIn( rawDirEntry, fileName );
		}

		// find the component as a file
		fileHandle = OpenDirect( fileEntry, usage );
	}

	return ( fileHandle );
}

FileHandle TankFileMgr :: Open( IndexPtr where, eUsage usage )
{
	return ( OpenDirect( GetRawFileEntry( where ), usage ) );
}

bool TankFileMgr :: Close( FileHandle file )
{
	RwCritical::WriteLock writeLock( m_RwCritical );

// Find and verify entry data.

	// get handle data
	FileEntry* fileEntry = file ? m_FileHandleMgr.Dereference( file ) : NULL;
	if ( fileEntry == NULL)  return ( false );

	// get resource view
	ResView* resView = fileEntry->m_ResView;
	gpassert( resView != NULL );

	// verify that all maps closed
#	if ( GP_DEBUG )
	{
		MemHandleMgr::const_iterator i, begin = m_MemHandleMgr.GetBegin(), end = m_MemHandleMgr.GetEnd();
		for ( i = begin ; i != end ; ++i )
		{
			if ( m_MemHandleMgr.IsElementUsed( i ) && (i->m_ResView == resView) && (i->m_FileEntry == fileEntry) )
			{
				gperrorf(( "Closed a file handle ('%s') with memory maps still open!", resView->m_RawFileEntry->m_Name.c_str() ));
			}
		}
	}
#	endif

// Ok close it off and free the handle.

	// remove from the resviewmap if the last one
	if ( resView->m_RefCount == 1 )
	{
		// find and remove
		ResViewMap::iterator found = m_ResViewMap.find( resView->m_RawFileEntry );
		gpassert( found != m_ResViewMap.end() );
		m_ResViewMap.erase( found );

		// remove from handle coll if it's there
		HandleColl::iterator i, ibegin = m_HandleColl.begin(), iend = m_HandleColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i->m_Owner == resView )
			{
				i->m_Owner = NULL;
				break;
			}
		}
	}

	// release owned handle
	fileEntry->Release();
	m_FileHandleMgr.Release( file );

	// all ok
	return ( true );
}

void ExceptedMemCpy( void* out, const void* in, size_t size )
{
	__try
	{
		::memcpy( out, in, size );
	}
	__except( ::GlobalExceptionFilter( GetExceptionInformation(), EXCEPTION_CONTINUE_SEARCH ) )
	{
		// this space intentionally left blank...
	}
}

bool TankFileMgr :: Read( FileHandle file, void* out, int size, int* bytesRead )
{
	gpassert( ( out != NULL ) && ( size >= 0 ) );

// Find and verify entry data.

	bool success = true;
	int oldFilePointer = 0;
	ResView* resView = NULL;

	{
		// lock access to the entry
		RwCritical::ReadLock readLock( m_RwCritical );

		// get handle data
		FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
		if ( fileEntry == NULL)  return ( false );

		// get resource view
		resView = fileEntry->m_ResView;
		gpassert( resView != NULL );

		// clamp to max allowable size to read
		oldFilePointer = fileEntry->m_FilePointer;
		if ( clamp_max( size, (int)(resView->m_RawFileEntry->m_Size - oldFilePointer) ) )
		{
			// it's an error condition to read too much data
			success = false;
			::SetLastError( ERROR_INVALID_PARAMETER );
		}

		// adjust the "file pointer"
		fileEntry->m_FilePointer += size;
	}

// Read in data.

	// only bother if something to do
	if ( size > 0 )
	{
		// "read" the data
		// $$$ optimize this by using separate Read() function in the map
		//     that avoids the SEH slowdown
		ExceptedMemCpy( out, (const BYTE*)resView->m_FileMap.GetData() + oldFilePointer, size );
	}

	// update output read data
	if ( bytesRead != NULL )
	{
		*bytesRead = size;
	}

	// success/fail
	return ( success );
}

bool TankFileMgr :: ReadLine( FileHandle file, gpstring& out )
{

// Find and verify entry data.

	int maxSize = 0;
	int filePointer = 0;
	ResView* resView = NULL;

	{
		// lock access to the entry
		RwCritical::ReadLock readLock( m_RwCritical );

		// get handle data
		FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
		if ( fileEntry == NULL)  return ( false );

		// get resource view
		resView = fileEntry->m_ResView;
		gpassert( resView != NULL );

		// check range
		filePointer = fileEntry->m_FilePointer;
		maxSize = resView->m_RawFileEntry->m_Size - filePointer;
		if ( maxSize == 0 )
		{
			::SetLastError( ERROR_HANDLE_EOF );
			return ( false );
		}
	}

// Perform the read.

	// chunk-wise decoding
	do
	{
		// make sure we have enough decoded
		int size = min( 1024, maxSize );

		// read the string
		const void* base = (const BYTE*)resView->m_FileMap.GetData() + filePointer;
		bool foundEol = false;
		int copied = stringtool::ReadLine( base, size, out, &foundEol );

		// advance
		filePointer += copied;
		maxSize -= copied;

		// found the '\n'?
		if ( foundEol )
		{
			break;
		}
	}
	while ( maxSize > 0 );

	// now get the entry again and update it
	{
		// lock access to the entry
		RwCritical::ReadLock readLock( m_RwCritical );

		// get handle data
		FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
		gpassert( fileEntry != NULL );

		// update
		fileEntry->m_FilePointer = filePointer;
	}

	// done!
	return ( true );
}

bool TankFileMgr :: SetReadWrite( FileHandle file, bool set )
{
	UNREFERENCED_PARAMETER( file );
	UNREFERENCED_PARAMETER( set );
	GP_Unimplemented$$$();
	return ( false );
}

bool TankFileMgr :: Seek( FileHandle file, int offset, eSeekFrom origin )
{
	RwCritical::ReadLock readLock( m_RwCritical );

// Find and verify entry data.

	// get handle data
	FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
	if ( fileEntry == NULL)  return ( false );

	// get resource view
	ResView* resView = fileEntry->m_ResView;
	gpassert( resView != NULL );

// Perform seek.

	switch (origin)
	{
		case (SEEKFROM_BEGIN  ):  /* offset ok as is */                                break;
		case (SEEKFROM_CURRENT):  offset += fileEntry->m_FilePointer;                  break;
		case (SEEKFROM_END    ):  offset  = resView->m_RawFileEntry->m_Size - offset;  break;

		default:
		{
			gpassert( 0 );  // $ should never get here
			return ( false );
		}
	}

	bool success = true;

	// make sure it's in range
	if ( clamp_min_max( offset, 0, (int)resView->m_RawFileEntry->m_Size ) )
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

int TankFileMgr :: GetPos( FileHandle file )
{
	RwCritical::ReadLock readLock( m_RwCritical );

	// get handle data
	FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
	if ( fileEntry == NULL)  return ( INVALID_POS_VALUE );

	// return pointer
	return ( fileEntry->m_FilePointer );
}

int TankFileMgr :: GetSize( FileHandle file )
{
	RwCritical::ReadLock readLock( m_RwCritical );

	// get handle data
	FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
	if ( fileEntry == NULL)  return ( INVALID_SIZE_VALUE );

	// get resource view
	ResView* resView = fileEntry->m_ResView;
	gpassert( resView != NULL );

	// return size
	return ( resView->m_RawFileEntry->m_Size );
}

eLocation TankFileMgr :: GetLocation( FileHandle file )
{
	RwCritical::ReadLock readLock( m_RwCritical );

	// get handle data - this is just for verification, don't really need entry
	FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
	if ( fileEntry == NULL)  return ( LOCATION_ERROR );

	// return location of entire tank
	return ( m_TankFile.GetLocation() );
}

bool TankFileMgr :: GetFileTimes( FileHandle file, FILETIME* creation, FILETIME* lastAccess, FILETIME* lastWrite )
{
	RwCritical::ReadLock readLock( m_RwCritical );

	// get handle data - this is just for verification, don't really need entry
	FileEntry* fileEntry = m_FileHandleMgr.Dereference( file );
	if ( fileEntry == NULL)  return ( false );

	// query times
	if ( creation != NULL )
	{
		*lastWrite = fileEntry->m_ResView->m_RawFileEntry->m_FileTime;
	}
	if ( lastAccess != NULL )
	{
		*lastAccess = m_TankFile.GetFindData().m_LastAccessTime;
	}
	if ( lastWrite != NULL )
	{
		*lastWrite = fileEntry->m_ResView->m_RawFileEntry->m_FileTime;
	}
	return ( true );
}

MemHandle TankFileMgr :: Map( FileHandle file, int offset, int size )
{
	RwCritical::WriteLock writeLock( m_RwCritical );

	// verify that file exists
	FileEntry* fileEntry = file ? m_FileHandleMgr.Dereference( file ) : NULL;
	if ( fileEntry == NULL)  return ( MemHandle::Null );

	// get resource view
	ResView* resView = fileEntry->m_ResView;
	gpassert( resView != NULL );

// Prep some vars.

	// acquire an empty handle
	MemHandle memHandle;
	MemEntry* memEntry = m_MemHandleMgr.Acquire( memHandle );

	// init handle data
	memEntry->m_ResView = resView;
	resView->AddRef();

// "Open" the map - really just some pointer addition.

	// set size if necessary
	if ( size == MAP_REST_OF_DATA )  // map rest of the file?
	{
		size = resView->m_RawFileEntry->m_Size - offset;
	}

	// make sure our view is in range to check for potential access violations
	bool success = true;
	if ( clamp_min_max( size, 0, (int)resView->m_RawFileEntry->m_Size ) )
	{
		success = false;
	}
	if ( clamp_min_max( offset, 0, (int)(resView->m_RawFileEntry->m_Size - size) ) )
	{
		success = false;
	}
	if ( !success )
	{
		// error!
		::SetLastError( ERROR_INVALID_PARAMETER );
		gpassertm( 0, "Attempting to map data out of range" );

		// release owned handle
		memEntry->Release();
		m_MemHandleMgr.Release( memHandle );

		// failed...
		return ( MemHandle::Null );
	}

	// fill out remaining data
	memEntry->m_Data = (const BYTE*)resView->m_FileMap.GetData() + offset;
	memEntry->m_Size = size;
	GPDEBUG_ONLY( memEntry->m_FileEntry = fileEntry );

	// return handle
	return ( memHandle );
}

bool TankFileMgr :: Close( MemHandle mem )
{
	RwCritical::WriteLock writeLock( m_RwCritical );

	// get handle data
	MemEntry* memEntry = mem ? m_MemHandleMgr.Dereference( mem ) : NULL;
	if ( memEntry == NULL)  return ( false );

	// this should not be the last one - files still outstanding!
	gpassert( memEntry->m_ResView->m_RefCount > 1 );

	// free the handle
	memEntry->Release();
	m_MemHandleMgr.Release( mem );

	// always ok
	return ( true );
}

int TankFileMgr :: GetSize( MemHandle mem )
{
	RwCritical::ReadLock readLock( m_RwCritical );

	// get handle data
	MemEntry* memEntry = m_MemHandleMgr.Dereference( mem );
	if ( memEntry == NULL)  return ( INVALID_SIZE_VALUE );

	// return size
	return ( memEntry->m_Size );
}

bool TankFileMgr :: Touch( MemHandle mem, int offset, int size )
{
	ResView* resView = NULL;
	const void* data = NULL;
	bool success = true;

	{
		RwCritical::ReadLock readLock( m_RwCritical );

		// get handle data
		MemEntry* memEntry = m_MemHandleMgr.Dereference( mem );
		if ( memEntry == NULL)  return ( INVALID_SIZE_VALUE );

		// get resource view
		resView = memEntry->m_ResView;
		gpassert( resView != NULL );

		// get data
		data = memEntry->m_Data;

		// set size if necessary
		if ( size == TOUCH_REST_OF_DATA )  // touch rest of view?
		{
			size = memEntry->m_Size - offset;
		}

		// make sure our memory is in range to check for potential access violations
		if ( clamp_min_max( size, 0, memEntry->m_Size ) )
		{
			success = false;
			::SetLastError( ERROR_INVALID_PARAMETER );
			gpassertm( 0, "Attempting to touch data out of range" );
		}
		if ( clamp_min_max( offset, 0, memEntry->m_Size - size ) )
		{
			success = false;
			::SetLastError( ERROR_INVALID_PARAMETER );
			gpassertm( 0, "Attempting to touch data out of range" );
		}
	}

	// touch the pages
	resView->m_FileMap.Touch( (const BYTE*)data + offset, size );

	return ( success );
}

const void* TankFileMgr :: GetData( MemHandle mem, int offset )
{
	RwCritical::ReadLock readLock( m_RwCritical );

	// get handle data
	MemEntry* memEntry = m_MemHandleMgr.Dereference( mem );
	if ( memEntry == NULL)  return ( NULL );

	// return pointer - this will throw if deref'd out of range
	return ( (const BYTE*)memEntry->m_Data + offset );
}

FindHandle TankFileMgr :: Find( const char* pattern, eFindFilter filter )
{
	FindHandle find;

	const char* lastComponent;
	const RawDirEntry* rawDirEntry = FindIndex( NULL, pattern, lastComponent );
	if ( rawDirEntry != NULL )
	{
		find = FindHelp( rawDirEntry, lastComponent, filter );
	}

	return ( find );
}

FindHandle TankFileMgr :: Find( FindHandle base, const char* pattern, eFindFilter filter )
{
	// a null base means root $ early bailout
	if ( !base.IsFinding() )
	{
		return ( Find( pattern, filter ) );
	}

// Find the file.

	FindHandle find;
	const RawDirEntry* currentDir = NULL;

	// look up dir from that handle
	{
		RwCritical::ReadLock readLock( m_RwCritical );

		// get handle data
		FindEntry* findEntry = m_FindHandleMgr.Dereference( base );
		if ( findEntry == NULL)
		{
			return ( find );
		}
		currentDir = findEntry->m_CurrentDir;
	}

	// re-search from new base
	const char* lastComponent;
	const RawDirEntry* rawDirEntry = FindIndex( currentDir, pattern, lastComponent );
	if ( rawDirEntry != NULL )
	{
		find = FindHelp( rawDirEntry, lastComponent, filter );
	}

	return ( find );
}

bool TankFileMgr :: GetNext( FindHandle find, gpstring& out, bool prependPath )
{
	RwCritical::ReadLock readLock( m_RwCritical );

	bool success = false;

	// get handle data
	FindEntry* findEntry = m_FindHandleMgr.Dereference( find );
	if ( findEntry != NULL)
	{
		// get next
		success = findEntry->Next( &out, prependPath );
	}

	return ( success );
}

bool TankFileMgr :: GetNext( FindHandle find, FindData& out, bool prependPath )
{
	RwCritical::ReadLock readLock( m_RwCritical );

// Advance the search.

	// get handle data
	FindEntry* findEntry = m_FindHandleMgr.Dereference( find );
	if ( findEntry == NULL)  return ( false );

	// copy in basic info
	out = m_TankFile.GetFindData();

	// find the next one
	if ( !findEntry->Next( &out.m_Name, prependPath ) )
	{
		return ( false );
	}

// Fill out the extra data.

	// update the attributes
	if ( findEntry->m_CurrentIsDir )
	{
		FillFindData( out, rcast <const RawDirEntry*> ( findEntry->m_CurrentEntry ), NULL );
	}
	else
	{
		FillFindData( out, NULL, rcast <const RawFileEntry*> ( findEntry->m_CurrentEntry ) );
	}

	// update the indexptr for future fast access
	out.m_IndexPtr.SetPointer( findEntry->m_CurrentEntry );

	// should never fail at this point
	return ( true );
}

bool TankFileMgr :: HasNext( FindHandle find )
{
	RwCritical::ReadLock readLock( m_RwCritical );

	bool success = false;

	// get handle data
	FindEntry* findEntry = m_FindHandleMgr.Dereference( find );
	if ( findEntry != NULL)
	{
		// get next
		success = findEntry->Next( NULL, false );
	}

	return ( success );
}

bool TankFileMgr :: Close( FindHandle find )
{
	RwCritical::WriteLock writeLock( m_RwCritical );

	bool success = false;

	// get handle data
	FindEntry* findEntry = find ? m_FindHandleMgr.Dereference( find ) : NULL;
	if ( findEntry != NULL)
	{
		// release it
		m_FindHandleMgr.Release( find );
		success = true;
	}

	return ( success );
}

bool TankFileMgr :: QueryFileInfo( IndexPtr where, FindData& out )
{
	bool success = false;

	// get the data
	const RawFileEntry* rawFileEntry = NULL;
	const RawDirEntry* rawDirEntry = NULL;
	success = GetRawEntry( where, rawFileEntry, rawDirEntry );
	if ( success )
	{
		// copy in common data
		out = m_TankFile.GetFindData();

		// $ note that out.m_Name should NOT be filled in. the QueryFileInfo()
		//   function is only meant to be called from FileSys internally, not
		//   client code. those should use the FindX() functions instead. all we
		//   have to worry about is the first few entries.

		FillFindData( out, rawDirEntry, rawFileEntry );
	}

	return ( success );
}

const TankFileMgr::RawFileEntry* TankFileMgr :: GetRawFileEntry( IndexPtr indexPtr )
{
	const RawFileEntry* rawFileEntry = NULL;

	// gotta have a tank open
	if ( IsTankOpen() )
	{
		// make sure the indexptr really is a file entry
		rawFileEntry = rcast <const RawFileEntry*> ( indexPtr.GetHandle() );
		if ( !m_TankFile.IsFileEntry( rawFileEntry ) || rawFileEntry->IsInvalidFile() )
		{
			gpassertm( 0, "Attempt to dereference an invalid TankFileMgr IndexPtr as a file entry" );
			::SetLastError( ERROR_INVALID_HANDLE );
			rawFileEntry = NULL;
		}
	}
	else
	{
		gpassertm( 0, "TankFileMgr has not opened a file!" );
		::SetLastError( ERROR_NOT_READY );
	}

	return ( rawFileEntry );
}

const TankFileMgr::RawDirEntry* TankFileMgr :: GetRawDirEntry( IndexPtr indexPtr )
{
	const RawDirEntry* rawDirEntry = NULL;

	// gotta have a tank open
	if ( IsTankOpen() )
	{
		// make sure the indexptr really is a dir entry
		rawDirEntry = rcast <const RawDirEntry*> ( indexPtr.GetHandle() );
		if ( !m_TankFile.IsDirEntry( rawDirEntry ) )
		{
			gpassertm( 0, "Attempt to dereference an invalid TankFileMgr IndexPtr as a dir entry" );
			::SetLastError( ERROR_INVALID_HANDLE );
			rawDirEntry = NULL;
		}
	}
	else
	{
		gpassertm( 0, "TankFileMgr has not opened a file!" );
		::SetLastError( ERROR_NOT_READY );
	}

	return ( rawDirEntry );
}

bool TankFileMgr :: GetRawEntry( IndexPtr indexPtr, const RawFileEntry*& rawFileEntry, const RawDirEntry*& rawDirEntry )
{
	if ( IsTankOpen() && m_TankFile.IsDirEntry( (const void*)indexPtr.GetHandle() ) )
	{
		rawDirEntry = GetRawDirEntry( indexPtr );
		return ( rawDirEntry != NULL );
	}
	else
	{
		rawFileEntry = GetRawFileEntry( indexPtr );
		return ( rawFileEntry != NULL );
	}
}

FileHandle TankFileMgr :: OpenDirect( const RawFileEntry* rawFileEntry, eUsage usage )
{
	UNREFERENCED_PARAMETER( usage );		// $ reserved for future caching systems

	RwCritical::WriteLock writeLock( m_RwCritical );

	if ( rawFileEntry == NULL )
	{
		::SetLastError( ERROR_FILE_NOT_FOUND );
		return ( FileHandle::Null );
	}
	gpassert( !rawFileEntry->IsInvalidFile() );

// Prep some vars.

	// acquire an empty handle
	FileHandle fileHandle;
	FileEntry* fileEntry = m_FileHandleMgr.Acquire( fileHandle );

	// init handle data
	fileEntry->m_FilePointer = 0;
	fileEntry->m_ResView = NULL;

// Open it.

	// see if we've already got this file open
	ResViewMap::iterator dupFind = m_ResViewMap.find( rawFileEntry );
	if ( dupFind != m_ResViewMap.end() )
	{
		// woohoo! inc ref count and just keep it
		fileEntry->m_ResView = dupFind->second;
		fileEntry->m_ResView->AddRef();
	}
	else
	{
		// create new one and take it
		ResView* resView = new ResView;
		fileEntry->m_ResView = resView;

		// get a file handle to use for reading
		HandleColl::iterator i, ibegin = m_HandleColl.begin(), iend = m_HandleColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i->m_Owner == NULL )
			{
				i->m_Owner = resView;
				break;
			}
		}
		if ( i == iend )
		{
			i = m_HandleColl.insert( m_HandleColl.end() );
			i->m_File = m_TankFile.CreateFile();
			i->m_Owner = resView;
		}

		// set it up
		if ( (i->m_File != INVALID_HANDLE_VALUE) && resView->Init( m_Options, usage, this, rawFileEntry, i->m_File ) )
		{
			// save it in the map
			m_ResViewMap.insert( std::make_pair( rawFileEntry, resView ) );

			// do crc validation? (set this while debugging)
#			if GP_DEBUG
			static bool s_ValidateCrc = false;
			if ( s_ValidateCrc )
			{
				int crc = GetCRC32( resView->m_FileMap.GetData(), resView->m_RawFileEntry->m_Size );
				gpassertm( crc == (int)resView->m_RawFileEntry->m_CRC32, "CRC validation error!!!" );
			}
#			endif // GP_DEBUG
		}
		else
		{
			// uh oh, let it go
			fileEntry->Release();
			m_FileHandleMgr.Release( fileHandle );
			i->m_Owner = NULL;
			fileHandle = FileHandle::Null;
		}
	}

// Done!

	return ( fileHandle );
}

const TankFileMgr::RawDirEntry* TankFileMgr :: FindIndex( const RawDirEntry* base, const char* name, const char*& lastComponent )
{
	{	// $ scope

		// pick the root
		if ( base == NULL )
		{
			base = m_TankFile.GetRootDir();
		}

		// for each path component in the name
		const char* nameBegin = name, * nameIter = nameBegin;
		const char* nameEnd = stringtool::NextPathComponent( nameIter );
		gpstring search;
		while ( *nameEnd != '\0' )
		{
			// find the component as a child
			search.assign( nameBegin, nameEnd );
			search.to_lower();
			const void* found = m_TankFile.FindIn( base, search );
			if ( found == NULL )
			{
				goto error;
			}

			// advance
			nameBegin = nameIter;
			nameEnd   = stringtool::NextPathComponent( nameIter );

			// if it's a file and we're expecting a dir then bail right here
			if ( m_TankFile.IsFileEntry( found ) && (*nameEnd != '\0') )
			{
				goto error;
			}
			base = rcast <const RawDirEntry*> ( found );
		}

		// return results
		lastComponent = nameBegin;
		return ( base );
	}

// Error case.

error:
	::SetLastError( ERROR_FILE_NOT_FOUND );
	return ( NULL );
}

FindHandle TankFileMgr :: FindHelp( const RawDirEntry* rawDirEntry, const char* pattern, eFindFilter filter )
{
	RwCritical::WriteLock writeLock( m_RwCritical );

// Prep some vars.

	// acquire an empty handle
	FindHandle findHandle;
	FindEntry* findEntry = m_FindHandleMgr.Acquire( findHandle );

	// copy basic spec in
	findEntry->m_TankFile     = &m_TankFile;
	findEntry->m_CurrentDir   = rawDirEntry;
	findEntry->m_CurrentEntry = NULL;
	findEntry->m_CurrentIsDir = false;
	findEntry->m_EntryIter    = rawDirEntry->GetOffsets();
	findEntry->m_EntryEnd     = findEntry->m_EntryIter + rawDirEntry->m_ChildCount;
	findEntry->m_Filter       = filter;

// Analyze and build pattern.

	// default to all files
	if ( (pattern == NULL) || (*pattern == '\0') )
	{
		pattern = "*";
	}
	else
	{
		// pattern can have no path info in it
		gpassert( pattern[ strcspn( pattern, "\\/" ) ] == '\0' );
	}

	// $ note below - there's no point checking for a "no wildcards" case for
	//   a simple string compare match. the normal wildcard case will handle it
	//   fine, plus it's going to be very rare that someone does a find with no
	//   wildcards. of course this can be optimized if needed.

	// test for simplest case first
	if ( same_with_case( pattern, "*" ) || same_with_case( pattern, "*.*" ) )
	{
		// pattern clear for an all match
		findEntry->m_Pattern.erase();
	}
	else
	{
		// copy pattern in and make it lower case
		findEntry->m_Pattern = pattern;
		findEntry->m_Pattern.to_lower();
	}

// Start the find going.

	findEntry->First();

	// all done
	return ( findHandle );
}

void TankFileMgr :: FillFindData( FindData& out, const RawDirEntry* dirEntry, const RawFileEntry* fileEntry )
{
	// always read-only
	out.m_Attributes |= FILE_ATTRIBUTE_READONLY;

	if ( fileEntry != NULL )
	{
		gpassert( dirEntry == NULL );

		out.m_Attributes &= ~FILE_ATTRIBUTE_DIRECTORY;
		out.m_Size = fileEntry->GetUncompressedSize();
		out.m_CompressedSize = fileEntry->GetCompressedSize();
		out.m_CreationTime = fileEntry->m_FileTime;
		out.m_LastWriteTime = fileEntry->m_FileTime;

		// update the compression attributes
		if ( fileEntry->IsCompressed() )
		{
			out.m_Attributes |= FILE_ATTRIBUTE_COMPRESSED;
			out.m_CompressedSize = fileEntry->GetCompressedSize();

			if ( fileEntry->m_Format == DATAFORMAT_ZLIB )
			{
				out.m_Attributes |= FILE_ATTRIBUTE_COMPRESSED_ZLIB;
			}
			else if ( fileEntry->m_Format == DATAFORMAT_LZO )
			{
				out.m_Attributes |= FILE_ATTRIBUTE_COMPRESSED_LZO;
			}
		}
		else
		{
			out.m_Attributes &= ~FILE_ATTRIBUTE_COMPRESSED;
		}
	}
	else
	{
		gpassert( dirEntry != NULL );

		out.m_Attributes |= FILE_ATTRIBUTE_DIRECTORY;
		out.m_Attributes &= ~FILE_ATTRIBUTE_COMPRESSED;
		out.m_Size = 0;
		out.m_CompressedSize = 0;
		out.m_CreationTime = dirEntry->m_FileTime;
		out.m_LastWriteTime = dirEntry->m_FileTime;
	}
}

//////////////////////////////////////////////////////////////////////////////
// class TankFileMgr::ResView implementation

void TankFileMgr::ResView :: Release( void )
{
	// i'm lazy so sometimes i delete null
	if ( this != NULL )
	{
		gpassert( m_RefCount >= 0 );
		if ( --m_RefCount == 0 )
		{
			delete ( this );
		}
	}
}

bool TankFileMgr::ResView :: Init( eOptions options, eUsage usage, const TankFileMgr* tankFileMgr, const RawFileEntry* rawFileEntry, HANDLE file )
{
	m_RawFileEntry = rawFileEntry;

	// get sizes
	DWORD offset = m_RawFileEntry->m_Offset + tankFileMgr->GetTankFile().GetDataOffset();
	DWORD size   = m_RawFileEntry->GetUncompressedSize();
	DWORD csize  = m_RawFileEntry->GetCompressedSize();

	// $ bail early for zero size
	if ( size == 0 )
	{
		return ( true );
	}

	// build our reader
	std::auto_ptr <Reader> fileReader( new FileReader( file, offset, csize, tankFileMgr->GetTankFile().GetFileSize() ) );

	// compressed? build meta-reader
	int chunkSize = m_RawFileEntry->GetChunkSize();
	switch ( (eDataFormat)m_RawFileEntry->m_Format )
	{
#		if ZLIB_SUPPORT
		case ( TankConstants::DATAFORMAT_ZLIB ):
		{
			std::auto_ptr <Reader> metaReader(
					new ZLibReader( fileReader.release(), tankFileMgr, m_RawFileEntry ) );
			fileReader = metaReader;
		}
		break;
#		endif // ZLIB_SUPPORT

#		if LZO_SUPPORT
		case ( TankConstants::DATAFORMAT_LZO ):
		{
			std::auto_ptr <Reader> metaReader(
					new LzoReader( fileReader.release(), tankFileMgr, m_RawFileEntry ) );
			fileReader = metaReader;
		}
		break;
#		endif // LZO_SUPPORT

		case ( TankConstants::DATAFORMAT_RAW ):
		{
			// use a chunk size the same as our system page size (so we don't read it all in at once)
			chunkSize = SysInfo::GetSystemPageSize();
		}
		break;

		default:
		{
			// unknown format
			gpassert( 0 );
			return ( false );
		}
	}

	// get options
	DiyFileMap::eOptions mapOptions = DiyFileMap::OPTION_NONE;
	if ( usage & USAGE_STREAMING )
	{
		mapOptions |= DiyFileMap::OPTION_NO_CACHING;
	}
	if ( options & OPTION_NO_PAGING )
	{
		mapOptions |= DiyFileMap::OPTION_NO_PAGING;
	}
	if ( options & OPTION_NO_VIRTUALALLOC )
	{
		mapOptions |= DiyFileMap::OPTION_NO_VIRTUALALLOC;
	}

	// create map
	bool success = m_FileMap.Create(
			mapOptions,
			(usage & USAGE_READWRITE) ? DiyFileMap::ACCESS_READ_WRITE : DiyFileMap::ACCESS_READ_ONLY,
			fileReader.release(),
			size,
			chunkSize );

	// test crc
#	if !GP_RETAIL
	if ( success && (options & OPTION_VALIDATE_CRC) )
	{
		DWORD crc = ::GetCRC32( m_FileMap.GetData(), size );
		if ( crc != m_RawFileEntry->m_CRC32 )
		{
			gperrorf(( "CRC32 mismatch on file '%s'! Got 0x%08X, expected 0x%08X!!\n",
					   m_RawFileEntry->m_Name.c_str(), crc, m_RawFileEntry->m_CRC32 ));
		}
	}
#	endif // !GP_RETAIL

	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// class TankFileMgr::FindEntry implementation

void TankFileMgr::FindEntry :: First( void )
{
	// $ this function cannot fail - the directory must exist otherwise we
	//   never would have made it this far. files not being found is not
	//   an error condition.

	gpassert( m_CurrentDir != NULL );
	if ( m_EntryIter != m_EntryEnd )
	{
		Resolve();

		if ( IsMatch() )
		{
			// data already available next time around
			m_Mode = MODE_FIRST;
		}
		else
		{
			// if it's not a match then need to keep looking - Next() will set to EMPTY when it runs out
			m_Mode = MODE_NORMAL;
			if ( Next( NULL, false ) )
			{
				m_Mode = MODE_FIRST;
			}
		}
	}
	else
	{
		::SetLastError( ERROR_NO_MORE_FILES );		// someone may want to check this
		m_Mode = MODE_EMPTY;
	}
}

bool TankFileMgr::FindEntry :: Next( gpstring* filename, bool prependPath )
{
	bool success = false;

// Find the file.

	if ( m_Mode == MODE_FIRST )
	{
		// data is already there - use it (may be empty though)
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
		// keep looking until we find a match or run out of entries
		for ( ; ; )
		{
			++m_EntryIter;
			if ( m_EntryIter == m_EntryEnd )
			{
				::SetLastError( ERROR_NO_MORE_FILES );
				m_Mode = MODE_EMPTY;
				break;
			}

			Resolve();

			if ( IsMatch() )
			{
				success = true;
				break;
			}
		}
	}

// Tell the caller about it.

	// copy over filename
	if ( success && (filename != NULL) )
	{
		m_TankFile->BuildFileName( *filename, m_CurrentEntry, prependPath );
	}

	return ( success );
}

bool TankFileMgr::FindEntry :: IsMatch( void ) const
{
	// early out on metafiles
	const Tank::NSTRING& name = GetName();
	if ( !m_CurrentIsDir && (name[ 0 ] == '$') )
	{
		return ( false );
	}

	// check basic type match
	bool match = m_CurrentIsDir ? !!(m_Filter & FINDFILTER_DIRECTORIES)
								: !!(m_Filter & FINDFILTER_FILES);

	// match only if necessary
	if ( match && !m_Pattern.empty() )
	{
		// paranoia check to see that we're lowercase here (the 'name' var *definitely* is)
		gpassert( m_Pattern.is_lower() );

		// check match
		match = stringtool::IsDosWildcardMatch( m_Pattern, name, true );
	}

	// make sure it's not invalid if file
	if ( match && !m_CurrentIsDir )
	{
		if ( rcast <const RawFileEntry*> ( m_CurrentEntry )->IsInvalidFile() )
		{
			match = false;
		}
	}

	return ( match );
}

//////////////////////////////////////////////////////////////////////////////

}	// end of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
