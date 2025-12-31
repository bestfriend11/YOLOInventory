//////////////////////////////////////////////////////////////////////////////
//
// File     :  TankBuilder.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "TankBuilder.h"

#include "DllBinder.h"
#include "GpLzo.h"
#include "GpZlib.h"
#include "StdHelp.h"
#include "StringTool.h"

// set this to 1 to turn on extra crazy debugging paranoia checks
#define DEBUG_UBER_PARANOIA GP_DEBUG

namespace Tank  {  // begin of namespace Tank

static const int   DEFAULT_CHUNK_SIZE            = 16 * 1024;	// 16K
static const float DEFAULT_MIN_COMPRESSION_RATIO = 0.05f;		// 5%
static const int   DEFAULT_STARTING_OVERHEAD     = 16;			// lzo::GetMaxDeflateSize( DEFAULT_CHUNK_SIZE );

#pragma optimize ( "", off )

//////////////////////////////////////////////////////////////////////////////
// class Builder implementation

Builder::FilterDb Builder::ms_FilterDb;

Builder :: Builder( eOptions options )
{
	m_Options = options;
	m_TankHeader.Init();

	m_IsFiltering = false;
	m_CurrentFile = NULL;
	m_Deflater    = new Deflater;
}

Builder :: Builder( const Builder& other )
{
	*this = other;
}

Builder :: ~Builder( void )
{
	AbortTank();
	delete ( m_Deflater );
}

Builder& Builder :: operator = ( const Builder& other )
{
	gpassert( !other.IsTankOpen() );

	m_Options         = other.m_Options;
	m_TankHeader      = other.m_TankHeader;
	m_DescriptionText = other.m_DescriptionText;
	m_TankFileName    = other.m_TankFileName;

	return ( *this );
}

void Builder :: SetTitleText( const wchar_t* title )
{
	gpassert( title != NULL );
	stringtool::CopyString( m_TankHeader.m_TitleText, title, ELEMENT_COUNT( m_TankHeader.m_TitleText ) );
}

void Builder :: SetAuthorText( const wchar_t* author )
{
	gpassert( author != NULL );
	stringtool::CopyString( m_TankHeader.m_AuthorText, author, ELEMENT_COUNT( m_TankHeader.m_AuthorText ) );
}

void Builder :: SetCopyrightText( const wchar_t* copyright )
{
	gpassert( copyright != NULL );
	stringtool::CopyString( m_TankHeader.m_CopyrightText, copyright, ELEMENT_COUNT( m_TankHeader.m_CopyrightText ) );
}

bool Builder :: CreateTank( const wchar_t* tankName, bool always )
{
	using namespace FileSys;
	gpassert( !IsTankOpen() );
	gpassert( !IsFileOpen() );
	int pad = 0;

// Open the tank.

	m_TankFileName = GetFullPathName( tankName );
	if ( m_TankFileName.empty() )
	{
		m_TankFileName = tankName;
	}

	File::eAccess   access   = File::ACCESS_READ_WRITE;
	File::eSharing  sharing  = File::SHARE_READ_ONLY;
	File::eCreation creation = always ? File::CREATION_ALWAYS : File::CREATION_NEW;
	std::auto_ptr <WNSTRING> descText;

	if ( TestOptions( OPTION_USE_TEMP_FILE ) )
	{
		// open the file
		if ( !m_File.Create( ::ToAnsi( m_TankFileName ) + ".tmp", access, sharing, creation ) )  goto error;
	}
	else
	{
		// remove old file
		if ( !RemoveExistingTank() )  goto error;

		// open the actual file
		if ( !m_File.Create( ::ToAnsi( m_TankFileName ), access, sharing, creation ) )  goto error;
	}

// Precalc settings.

	// $$$ check PE options and handle!!

	// build desc string
	descText = stdx::make_auto_ptr( WNSTRING::Create( m_DescriptionText, m_DescriptionText.length() ) );

	// adjust data offset accordingly
	m_TankHeader.m_DataOffset = sizeof( m_TankHeader ) - sizeof( WNSTRING ) + descText->GetSize() + RawIndex::RAW_HEADER_PAD;
	pad = ::GetPadToDword( m_TankHeader.m_DataOffset );
	m_TankHeader.m_DataOffset += pad;

// Write header.

	// write out header minus trailing empty str
	if ( !m_File.Write( &m_TankHeader, sizeof( m_TankHeader ) - sizeof( WNSTRING ) ) )  goto error;

	// write out desc
	if ( !m_File.Write( descText.get(), descText->GetSize() ) )  goto error;

	// align to dword
	char nulls[ RawIndex::RAW_HEADER_PAD ];
	::ZeroObject( nulls );
	if ( !m_File.Write( nulls, pad ) )  goto error;

	// write out padding
	COMPILER_ASSERT( RawIndex::RAW_HEADER_PAD >= 3 );
	if ( !m_File.WriteStruct( nulls ) )  goto error;
	gpassert( ::IsDwordAligned( m_File.GetPos() ) );

	// done
	return ( true );

// Error handling.

error:

	// cleanup (preserve last error through abort)
	DWORD err = ::GetLastError();
	AbortTank();
	::SetLastError( err );

	// failed
	return ( false );
}

bool Builder :: CommitTank( void )
{
	gpassert( IsTankOpen() );
	gpassert( !IsFileOpen() );
	DWORD pos, offsetPos, count, base, indexSize;
	OffsetColl::iterator oi;
	OffsetColl offsets;
	std::vector <BYTE> indexBuffer;
	bool success = true;

// Write dirs.

	// write out more padding before starting the dirs
	char nulls[ RawIndex::RAW_HEADER_PAD ];
	::ZeroObject( nulls );
	if ( !m_File.WriteStruct( nulls ) )  goto error;

	// update dirset location
	base = m_TankHeader.m_DirSetOffset = m_File.GetPos();

	// write out dirset offset placeholders
	count = m_RootDir.GetDirCount();
	if ( !m_File.WriteStruct( count ) )  goto error;
	offsetPos = m_File.GetPos();
	offsets.resize( count );
	if ( !m_File.Write( &*offsets.begin(), offsets.size() * sizeof( DWORD ) ) )  goto error;

	// write out dirs
	oi = offsets.begin();
	if ( !m_RootDir.WriteDirs( base, 0, oi, m_File ) )  goto error;
	gpassert( oi == offsets.end() );

	// backpatch offsets
	pos = m_File.GetPos();
	if ( !m_File.Seek( offsetPos, FILE_BEGIN ) )  goto error;
	if ( !m_File.Write( &*offsets.begin(), offsets.size() * sizeof( DWORD ) ) )  goto error;
	if ( !m_File.Seek( pos, FILE_BEGIN ) )  goto error;

// Write files.

	// pad with more nulls
	if ( !m_File.WriteStruct( nulls ) )  goto error;

	// backpatch fileset location
	pos = m_TankHeader.m_FileSetOffset = m_File.GetPos();

	// write out fileset offset placeholders
	count = m_RootDir.GetFileCount();
	if ( !m_File.WriteStruct( count ) )  goto error;
	offsetPos = m_File.GetPos();
	offsets.resize( count );
	if ( !m_File.Write( &*offsets.begin(), offsets.size() * sizeof( DWORD ) ) )  goto error;

	// write out files
	oi = offsets.begin();
	if ( !m_RootDir.WriteFiles( pos, base, 0, oi, m_File ) )  goto error;
	gpassert( oi == offsets.end() );

	// remember this for the size
	indexSize = m_File.GetPos() - base;

	// backpatch offsets
	if ( !m_File.Seek( offsetPos, FILE_BEGIN ) )  goto error;
	if ( !m_File.Write( &*offsets.begin(), offsets.size() * sizeof( DWORD ) ) )  goto error;

	// backpatch index size
	m_TankHeader.m_IndexSize = indexSize;

	// $$$ backpatch PE info
	// $$$ if PE, also store a version resource in .rsrc section

	// re-read index and calc CRC for it
	indexBuffer.resize( indexSize );
	if ( !m_File.Seek( base, FILE_BEGIN ) )  goto error;
	if ( !m_File.Read( indexBuffer.begin(), indexSize ) )  goto error;
	m_TankHeader.m_IndexCRC32 = ::GetCRC32( indexBuffer.begin(), indexSize );

	// re-write header minus trailing empty str
	if ( !m_File.Seek( 0, FILE_BEGIN ) )  goto error;
	if ( !m_File.Write( &m_TankHeader, sizeof( m_TankHeader ) - sizeof( WNSTRING ) ) )  goto error;

	// close
	m_File.Close();
	m_RootDir.Clear();
	m_CurrentFile = NULL;

	// rename tmp to tnk if necessary
	if ( TestOptions( OPTION_USE_TEMP_FILE ) && RemoveExistingTank() )
	{
		gpstring tmpName( ::ToAnsi( m_TankFileName ) + ".tmp" );
		if ( !::MoveFile( tmpName, ::ToAnsi( m_TankFileName ) ) )
		{
			gperrorf(( "Unable to rename TANK file '%s' to '%S', error is '%s' (0x%08X)\n",
					   tmpName.c_str(), m_TankFileName.c_str(),
					   stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
			success = false;
		}
	}

	// clear
	m_TankFileName.clear();

	// done
	return ( success );

// Error handling.

error:

	// cleanup (preserve last error through abort)
	DWORD err = ::GetLastError();
	AbortTank();
	::SetLastError( err );

	// failed
	return ( false );
}

bool Builder :: AbortTank( void )
{
	// check to see if already closed
	if ( !IsTankOpen() )
	{
		::SetLastError( ERROR_INVALID_HANDLE );
		return ( false );
	}

	// close it
	m_File.Close();

	// clear dir info
	m_RootDir.Clear();
	m_CurrentFile = NULL;

	// delete failed tank
	bool success;
	if ( TestOptions( OPTION_USE_TEMP_FILE ) )
	{
		success = !!::DeleteFile( ::ToAnsi( m_TankFileName ) + ".tmp" );
	}
	else
	{
		success = !!::DeleteFile( ::ToAnsi( m_TankFileName ) );
	}

	// clear the name
	m_TankFileName.clear();

	// done
	return ( success );
}

class GpLzoDll : public DllBinder, public OnDemandSingleton <GpLzoDll>
{
public:
	SET_INHERITED( GpLzoDll, DllBinder );

	GpLzoDll( void )
		: Inherited( "GPLZO.DLL" )
	{
		AddProc( &Deflate, "_Deflate@16" );

		Load();
	}

	DllProc4 <int,							// Deflate(
			void*,								// out
			size_t,								// outSize
			const void*,						// in
			size_t>								// inSize )
		Deflate;

private:
	SET_NO_COPYING( GpLzoDll );
};

#define gGpLzoDll GpLzoDll::GetSingleton()

bool Builder :: BeginFile( FileSpec fileSpec )
{
	gpassert( IsTankOpen() );
	gpassert( !IsFileOpen() );

	// must be lowercase
	fileSpec.m_FileName.to_lower();

	// go out to the dir
	m_CurrentFile = m_RootDir.GetFile( fileSpec.m_FileName, fileSpec.m_DirTime );
	if ( m_CurrentFile == NULL )
	{
		if ( ::GetLastError() == ERROR_FILE_EXISTS )
		{
			gperrorf(( "Attempted to add duplicate file to directory: '%s'\n", fileSpec.m_FileName.c_str() ));
			return ( false );
		}
		else
		{
			gperrorf(( "Badly formed path spec: '%s'\n", fileSpec.m_FileName.c_str() ));
			return ( false );
		}
	}

	// take spec
	m_CurrentFileSpec = fileSpec;

	// check for lzo compatibility
	if ( (m_CurrentFileSpec.m_DataFormat == TankConstants::DATAFORMAT_LZO) && !gGpLzoDll )
	{
		gperror( "Fatal error! LZO compression is not supported, sorry! Switching to zlib instead...\n" );
		m_CurrentFileSpec.m_DataFormat = TankConstants::DATAFORMAT_ZLIB;
	}

	// figure out min compression ratio and chunk size
	if ( m_CurrentFileSpec.m_DataFormat != TankConstants::DATAFORMAT_RAW )
	{
		if ( m_CurrentFileSpec.m_MinCompressionRatio < 0 )
		{
			m_CurrentFileSpec.m_MinCompressionRatio = DEFAULT_MIN_COMPRESSION_RATIO;
		}

		// set chunk size
		if ( m_CurrentFileSpec.m_ChunkSize < 0 )
		{
			m_CurrentFileSpec.m_ChunkSize = DEFAULT_CHUNK_SIZE;
		}

		// make sure it's a multiple of page size
		::AlignUp( m_CurrentFileSpec.m_ChunkSize, SysInfo::GetSystemPageSize() );
	}
	else
	{
		m_CurrentFileSpec.m_MinCompressionRatio = 0;
		m_CurrentFileSpec.m_ChunkSize = 0;
	}

	// figure out compression level
	if ( m_CurrentFileSpec.m_DataFormat == TankConstants::DATAFORMAT_ZLIB )
	{
		if ( m_CurrentFileSpec.m_CompressionLevel < 0 )
		{
			m_CurrentFileSpec.m_CompressionLevel = Z_DEFAULT_COMPRESSION;
		}
	}
	else
	{
		m_CurrentFileSpec.m_CompressionLevel = 0;
	}

	// delay our compressing?
	m_IsFiltering =    (m_CurrentFileSpec.m_ChunkSize > 0)
					|| (m_CurrentFileSpec.m_MinCompressionRatio > 0)
					|| (m_CurrentFileSpec.m_DataFormat == TankConstants::DATAFORMAT_LZO);

	// set params
	m_CurrentFile->m_FileEntry.m_Offset   = m_File.GetPos() - m_TankHeader.m_DataOffset;
	m_CurrentFile->m_FileEntry.m_FileTime = m_CurrentFileSpec.m_FileTime;
	m_CurrentFile->m_FileEntry.m_Format   = (WORD)m_CurrentFileSpec.m_DataFormat;
	m_CurrentFile->m_FileEntry.m_Flags    = (WORD)m_CurrentFileSpec.m_FileFlags;
	m_CurrentFile->m_ChunkSize            = m_CurrentFileSpec.m_ChunkSize;

	// clear filter
	m_FilterBuffer.clear();

	// set is filtering flag
	m_IsFiltering |= !fileSpec.m_Filters.empty();

	// done
	return ( true );
}

bool Builder :: WriteFile( const_mem_ptr data )
{
	// $$$$$ flush for each chunk if we're not doing a min compression ratio!!

	gpassert( IsFileOpen() );
	gpassert( (data.size == 0) || (data.mem != NULL) );

	// nothing useful to do?
	if ( data.size == 0 )
	{
		return ( true );
	}

	// delaying the write?
	if ( m_IsFiltering )
	{
		m_FilterBuffer.push_back_raw( (const BYTE*)data.mem, (const BYTE*)data.mem + data.size );

		// special case if we're just "filtering" so we can do chunked
		// compression but don't care about maintaining a minimum compression
		// ratio for the file. this way, we can write out in chunks rather than
		// buffering the whole file.
		if ( (m_CurrentFileSpec.m_ChunkSize == 0) || (m_CurrentFileSpec.m_MinCompressionRatio != 0) )
		{
			return ( true );
		}
		if ( (int)m_FilterBuffer.size() < m_CurrentFileSpec.m_ChunkSize )
		{
			return ( true );
		}
		if ( !m_CurrentFileSpec.m_Filters.empty() )
		{
			return ( true );
		}

		// ok no min compression ratio, we're chunked, we aren't doing any
		// filtering, and there's enough data.
		int chunkCount = m_FilterBuffer.size() / m_CurrentFileSpec.m_ChunkSize;
		mem_ptr data( &*m_FilterBuffer.begin(), chunkCount * m_CurrentFileSpec.m_ChunkSize );
		bool success = WriteFile( data, true );
		if ( success )
		{
			// move the leftover to the front
			m_FilterBuffer.erase( m_FilterBuffer.begin(), m_FilterBuffer.begin() + data.size );
		}
		else
		{
			m_FilterBuffer.clear();
		}

		return ( success );
	}

	// done
	return ( WriteFile( data, false ) );
}

bool Builder :: EndFile( void )
{
	gpassert( IsFileOpen() );
	bool success = true;

	// finish filtering
	const_mem_ptr flushData;
	if ( m_IsFiltering )
	{
		// done
		m_IsFiltering = false;

		// filter the buffer
		gpstring newName;
		FilterColl::const_iterator i, ibegin = m_CurrentFileSpec.m_Filters.begin(), iend = m_CurrentFileSpec.m_Filters.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// get the filter
			FilterDb::iterator found = ms_FilterDb.find( *i );
			gpassert( found != ms_FilterDb.end() );
			Filter* filter = found->second;

			// do the filter
			const char* oldName = FileSys::GetFileName( m_CurrentFileSpec.m_FileName );
			newName = oldName;
			if ( !filter->OnFilter( m_CurrentFileSpec.m_FileName, newName, m_FilterBuffer ) )
			{
				// error during filtering, abort it
				gperrorf(( "Filter failed on file '%s', excluding from tank.\n", m_CurrentFileSpec.m_FileName.c_str() ));
				success = false;
				goto cleanup;
			}

			// see if the name changed
			if ( !newName.same_no_case( oldName ) )
			{
				// change it
				m_CurrentFile = m_CurrentFile->m_ParentDir->ChangeFileName( m_CurrentFile, newName );
				if ( m_CurrentFile == NULL )
				{
					// some kind of collision or bad naming probably, abort
					success = false;
					goto cleanup;
				}
			}
		}

		// store
		flushData.mem = &*m_FilterBuffer.begin();
		flushData.size = m_FilterBuffer.size();
	}

	// flush any remaining data
	if ( !WriteFile( flushData, true ) )
	{
		success = false;
		goto cleanup;
	}

cleanup:

	// only write padding on "large" files (small files go to the front and
	// should be as closely packed as possible)
	if ( m_CurrentFile->m_FileEntry.m_Size >= LARGE_FILE )
	{
		// pad to dword + add in 4 bytes extra padding
		static char nulls[ 7 ];
		if ( !m_File.Write( nulls, 4 + GetPadToDword( m_File.GetPos() ) ) )
		{
			success = false;
		}
	}
	else if ( m_CurrentFile->m_FileEntry.m_Size == 0 )
	{
		// special: zero-size files don't have an offset
		m_CurrentFile->m_FileEntry.m_Offset = 0;
	}

	// if failed, mark the file entry as bad
	if ( !success )
	{
		m_CurrentFile->m_FileEntry.m_Flags |= FILEFLAG_INVALID;
	}

	// close file
	m_CurrentFile = NULL;
	m_CurrentFileSpec = FileSpec();

	// done
	return ( success );
}

bool Builder :: AddFile( const FileSpec& fileSpec, const_mem_ptr data )
{
	// add the file (note that each of these steps may fail)
	bool rc = BeginFile( fileSpec ) && WriteFile( data ) && EndFile();

	// kill leftovers if one of the previous steps failed
	m_CurrentFile = NULL;
	m_CurrentFileSpec = FileSpec();

	// done
	return ( rc );
}

void Builder :: RegisterFilter( FOURCC fourCc, Filter* takeFilter )
{
	if ( FindFilter( fourCc ) != NULL )
	{
		UnregisterFilter( fourCc );
	}

	ms_FilterDb[ fourCc ] = takeFilter;
}

void Builder :: RegisterFilterChain( FOURCC fourCc, const FOURCC* links, int linkCount )
{
	class FilterChain : public Builder::Filter
	{
	public:
		FilterChain( const FOURCC* links, int linkCount )
			: m_FilterColl( links, links + linkCount )
		{
			// this space intentionally left blank...
		}

		virtual bool OnFilter( const gpstring& fileName, gpstring& newName, Buffer& buffer )
		{
			FilterColl::const_iterator i, ibegin = m_FilterColl.begin(), iend = m_FilterColl.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				Builder::Filter* filter = Builder::FindFilter( *i );
				if ( filter != NULL )
				{
					if ( !filter->OnFilter( fileName, newName, buffer ) )
					{
						return ( false );
					}
				}
			}

			return ( true );
		}

	private:
		typedef std::vector <FOURCC> FilterColl;

		FilterColl m_FilterColl;
	};

#	if GP_DEBUG
	const FOURCC* i, * ibegin = links, * iend = ibegin + linkCount;
	for ( i = ibegin ; i != iend ; ++i )
	{
		gpassert( FindFilter( *i ) != NULL );
	}
#	endif // GP_DEBUG

	RegisterFilter( fourCc, new FilterChain( links, linkCount ) );
}

Builder::Filter* Builder :: FindFilter( FOURCC fourCc )
{
	FilterDb::iterator found = ms_FilterDb.find( fourCc );
	return ( found != ms_FilterDb.end() ? found->second : NULL );
}

void Builder :: UnregisterFilter( FOURCC fourCc )
{
	FilterDb::iterator found = ms_FilterDb.find( fourCc );
	if ( found != ms_FilterDb.end() )
	{
		ms_FilterDb.erase( found );
	}
}

bool Builder :: WriteFile( const_mem_ptr data, bool flush )
{
	// save raw in case compression filter changes it
	const_mem_ptr rawData = data;

	// check to see if it's zero length and no need to compress
	if ( flush && (data.size == 0) && (m_CurrentFile->m_FileEntry.m_Size == 0) )
	{
		m_CurrentFileSpec.m_DataFormat = TankConstants::DATAFORMAT_RAW;
		m_CurrentFile->m_FileEntry.m_Format = (WORD)TankConstants::DATAFORMAT_RAW;
	}

	// compress?
	if ( m_CurrentFile->m_FileEntry.IsCompressed() )
	{
		// get local chunk size
		int chunkSize = m_CurrentFileSpec.m_ChunkSize;
		if ( chunkSize == 0 )
		{
			chunkSize = data.size;
		}

		// compress each chunk
		int outOffset = 0;
		while ( data.size > 0 )
		{
			// calc how much to write
			int write = min_t( data.size, (size_t)chunkSize );

			// alloc space for chunk + header
			int maxDeflateSize = max_t( zlib::GetMaxDeflateSize( write ), lzo::GetMaxDeflateSize( write ) );
			m_DeflateBuffer.resize( outOffset + maxDeflateSize );

			// get a starting overhead (0 if we don't worry about overhead)
			int overhead = ((write < chunkSize) || (m_CurrentFileSpec.m_ChunkSize == 0)) ? 0 : DEFAULT_STARTING_OVERHEAD;

			// retry compression until we figure out the overhead (lazy brute force)
			int outSize = 0;
			for ( ; ; overhead *= 2 )
			{
				int localWrite = write - overhead;

				// notify on too big
				if ( overhead > 256 )
				{
					gpwarningf(( "Warning: overhead >= %d is bad!!!\n", overhead ));
				}

				// format-specific processing
				if ( m_CurrentFile->m_FileEntry.m_Format == TankConstants::DATAFORMAT_ZLIB )
				{
					// build compressor if not ready
					if ( !m_Deflater->IsActive() && !m_Deflater->Init( m_CurrentFileSpec.m_CompressionLevel ) )
					{
						gperror( "Fatal error! Unable to initialize Zlib compressor\n" );
						return ( false );
					}

					// compress
					int inUsed = 0;
					outSize = m_Deflater->DeflateChunk(
							mem_ptr( &*(m_DeflateBuffer.begin() + outOffset), m_DeflateBuffer.size() - outOffset ),
							const_mem_ptr( data.mem, localWrite ), flush, &inUsed );
					if ( inUsed != (int)localWrite )
					{
						gperrorf(( "Fatal error! Zlib deflation gave unexpected 'used' size of %d instead of %d!\n",
								   inUsed, localWrite ));
						return ( false );
					}

					// end if we're flushing
					if ( flush )
					{
						// end it
						outSize = m_Deflater->End();

						// check for error
						if ( outSize < 0 )
						{
							gperrorf(( "Fatal error! Deflation flushed incompletely, return is %d!\n", outSize ));
							return ( false );
						}
					}
				}
				else if ( m_CurrentFile->m_FileEntry.m_Format == TankConstants::DATAFORMAT_LZO )
				{
					// lzo is not a streaming format - only call once
					gpassert( (m_CurrentFile->m_FileEntry.m_Size == 0) && flush );

					// compress it
					outSize = gGpLzoDll.Deflate(
							&*(m_DeflateBuffer.begin() + outOffset), m_DeflateBuffer.size() - outOffset,
							data.mem, localWrite );
					if ( outSize == -1 )
					{
						gperror( "Fatal error! LZO deflation failed!\n" );
						return ( false );
					}
				}

				// if not testing for overhead, just finish here
				if ( overhead == 0 )
				{
					break;
				}

				// if it's bigger than the source, just abort
				if ( (outSize + overhead) >= write )
				{
					break;
				}

				// alloc memory and ready the data
				m_TestBuffer.resize( maxDeflateSize );
				mem_ptr out( &*m_TestBuffer.begin(), write );
				mem_ptr in( &*(m_TestBuffer.begin() + write - outSize), outSize );
				::memcpy( in.mem, &*(m_DeflateBuffer.begin() + outOffset), in.size );

				// test for success
				if ( m_CurrentFile->m_FileEntry.m_Format == TankConstants::DATAFORMAT_ZLIB )
				{
					if ( zlib::Inflate( out, in ) == localWrite )
					{
						break;
					}
				}
				else if ( m_CurrentFile->m_FileEntry.m_Format == TankConstants::DATAFORMAT_LZO )
				{
					if ( lzo::Inflate( out, in ) == localWrite )
					{
						// test decompression to make sure it worked (there is a possibility that it can
						// successfully inflate yet overwrite itself a little - increasing the overhead
						// always fixes)
						if ( ::memcmp( out.mem, data.mem, localWrite ) == 0 )
						{
							break;
						}
					}
				}
			}

			// check for no compression on this chunk (though if we're chunking,
			// make sure that we aren't >= the size of the uncompressed data)
			float ratio = 1.0f - ((float)(outSize + overhead) / (float)write);
			if (   ((m_CurrentFileSpec.m_MinCompressionRatio != 0) && (ratio < m_CurrentFileSpec.m_MinCompressionRatio))
				|| ((m_CurrentFileSpec.m_ChunkSize != 0) && ((outSize + overhead) >= write)) )
			{
				// don't use it, revert to uncompressed
				outSize = write;
				::memcpy( &*(m_DeflateBuffer.begin() + outOffset), data.mem, write );
			}
			else
			{
				// write out the extra
				::memcpy( &*(m_DeflateBuffer.begin() + outOffset + outSize), (const BYTE*)data.mem + write - overhead, overhead );

#				if DEBUG_UBER_PARANOIA
				if ( overhead != 0 )
				{
					// $ attempt to decompress on top of itself, make sure data is ok

					// get some memory
					m_TestBuffer.resize( (int)(write * 1.5f) );

					// cap the ends
					*(DWORD*)(&*m_TestBuffer.begin()) = 0xBaadF00d;
					*(DWORD*)(&*(m_TestBuffer.begin() + write + 4)) = 0xBaadF00d;

					// build our ptrs
					mem_ptr out( &*(m_TestBuffer.begin() + 4), write + 4 );
					mem_ptr in( &*(m_TestBuffer.begin() + write + 4 - outSize), outSize );
					::memcpy( in.mem, &*(m_DeflateBuffer.begin() + outOffset), in.size );

					// assert we didn't overwrite in the memcpy (duh)
					gpassert( *(DWORD*)(&*m_TestBuffer.begin()) == 0xBaadF00d );
					gpassert( *(DWORD*)(&*(m_TestBuffer.begin() + write + 4)) == 0xBaadF00d );

					// test that inflation completes
					if ( m_CurrentFile->m_FileEntry.m_Format == TankConstants::DATAFORMAT_ZLIB )
					{
						gpverify( zlib::Inflate( out, in ) == (write - overhead) );
					}
					else if ( m_CurrentFile->m_FileEntry.m_Format == TankConstants::DATAFORMAT_LZO )
					{
						gpverify( lzo::Inflate( out, in ) == (write - overhead) );
					}

					// memcpy in the extra
					::memcpy( &*(m_TestBuffer.begin() + write + 4 - overhead), &*(m_DeflateBuffer.begin() + outOffset + outSize), overhead );

					// check for overrun/underrun
					gpassert( *(DWORD*)(&*m_TestBuffer.begin()) == 0xBaadF00d );
					gpassert( *(DWORD*)(&*(m_TestBuffer.begin() + write + 4)) == 0xBaadF00d );

					// check that the memory is the same
					gpassert( ::memcmp( out.mem, data.mem, write ) == 0 );
				}
#				endif // DEBUG_UBER_PARANOIA
			}

			// take it
			ChunkHeader chunkHeader;
			chunkHeader.m_UncompressedSize = write;
			chunkHeader.m_CompressedSize = outSize;
			chunkHeader.m_ExtraBytes = overhead;
			m_CurrentFile->m_ChunkColl.push_back( chunkHeader );

			// advance
			outOffset += outSize + overhead;
			data.mem = (const BYTE*)data.mem + write;
			data.size -= write;

			// must chunk due to use of overhead?
			if ( overhead > 0 )
			{
				m_CurrentFile->m_MustChunk = true;
			}
		}

		// update size
		m_CurrentFile->m_CompressedSize += outOffset;

		// redirect file output
		data.mem = &*m_DeflateBuffer.begin();
		data.size = outOffset;
	}

	// update total size by uncompressed size
	m_CurrentFile->m_FileEntry.m_Size += rawData.size;

	// throw away compressed version?
	if ( flush && (m_CurrentFile->m_FileEntry.m_Format != TankConstants::DATAFORMAT_RAW) )
	{
		// calc ratio
		float ratio = 1.0f - ((float)m_CurrentFile->m_CompressedSize / (float)m_CurrentFile->m_FileEntry.m_Size);

		// check min compression
		if (   (m_CurrentFileSpec.m_MinCompressionRatio != 0)
			&& (ratio < m_CurrentFileSpec.m_MinCompressionRatio) )
		{
			// don't use it
			m_CurrentFileSpec.m_DataFormat = TankConstants::DATAFORMAT_RAW;
			m_CurrentFile->m_FileEntry.m_Format = (WORD)TankConstants::DATAFORMAT_RAW;
			data = rawData;
		}
	}

	// update crc based on uncompressed data
	AddCRC32( rcast <unsigned int&> ( m_CurrentFile->m_FileEntry.m_CRC32 ), rawData.mem, rawData.size );
	AddCRC32( rcast <unsigned int&> ( m_TankHeader.m_DataCRC32 ), rawData.mem, rawData.size );

	// write the data
	return ( m_File.Write( data.mem, data.size ) );
}

bool Builder :: RemoveExistingTank( void )
{
	bool success = true;

	// test for overwrite
	if ( FileSys::DoesFileExist( m_TankFileName ) )
	{
		gpstring tankFileName( ::ToAnsi( m_TankFileName ) );

		if ( TestOptions( OPTION_BACKUP_OLD ) )
		{
			gpstring backupName( tankFileName + ".bak" );
			if ( FileSys::DoesFileExist( backupName ) && !::DeleteFile( backupName ) )
			{
				gperrorf(( "Unable to delete old TANK backup file '%s', error is '%s' (0x%08X)\n",
						   backupName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
				success = false;
			}
			else if ( !::MoveFile( tankFileName, tankFileName + ".bak" ) )
			{
				gperrorf(( "Unable to rename old TANK file '%S' to backup file, error is '%s' (0x%08X)\n",
						   m_TankFileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
				success = false;
			}
		}
		else if ( !::DeleteFile( tankFileName ) )
		{
			// could it be read-only?
			if ( ::GetFileAttributes( tankFileName ) & FILE_ATTRIBUTE_READONLY )
			{
				gperrorf(( "Unable to delete old TANK file '%S', FILE IS READ-ONLY! - error is '%s' (0x%08X)\n",
						   m_TankFileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
			}
			else
			{
				gperrorf(( "Unable to delete old TANK file '%S', error is '%s' (0x%08X)\n",
						   m_TankFileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
			}
			success = false;
		}
	}

	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// struct Builder::FileSpec implementation

Builder::FileSpec :: FileSpec( const gpstring& fileName, eDataFormat dataFormat, eFileFlags fileFlags )
	: m_FileName( fileName )
{
	m_DataFormat          = dataFormat;
	m_FileFlags           = fileFlags;
	m_CompressionLevel    = -1;
	m_ChunkSize           = -1;
	m_MinCompressionRatio = -1;

	// just init with current time
	SYSTEMTIME systemTime;
	::GetSystemTime( &systemTime );
	::SystemTimeToFileTime( &systemTime, &m_FileTime );
	m_DirTime = m_FileTime;
}

//////////////////////////////////////////////////////////////////////////////
// struct Builder::DirData implementation

void Builder::DirData :: Clear( void )
{
	stdx::for_each( m_SubDirs, stdx::delete_second_by_ptr() );
	m_Files.clear();
	m_SubDirs.clear();
	m_Offset = 0;
}

Builder::FileData* Builder::DirData :: GetFile( const gpstring& fileName, const FILETIME& dirTime )
{
	Builder::FileData* fileData = NULL;

	// skip any leading slashes
	size_t begin = fileName.find_first_not_of( "/\\" );

	// find first dir end
	size_t end = fileName.find_first_of( "/\\", begin );
	if ( end != gpstring::npos )
	{
		gpstring dirName( fileName.c_str() + begin, end - begin );
		if ( m_Files.find( dirName ) != m_Files.end() )
		{
			::SetLastError( ERROR_FILE_EXISTS );
		}
		else if ( FileSys::IsValidFileName( dirName ) )
		{
			// add it
			std::pair <DirColl::iterator, bool> rc = m_SubDirs.insert( DirColl::value_type( dirName, NULL ) );
			if ( rc.second )
			{
				rc.first->second = new DirData;
			}
			DirData& dirData = *rc.first->second;

			// get file data
			fileData = dirData.GetFile( gpstring( fileName.c_str() + end ), dirTime );

			// check for newer file time
			if ( ::CompareFileTime( &dirData.m_FileTime, &dirTime ) < 0 )
			{
				dirData.m_FileTime = dirTime;
			}
		}
		else
		{
			::SetLastError( ERROR_INVALID_NAME );
		}
	}
	else
	{
		gpstring localName( fileName.c_str() + begin );
		if ( m_SubDirs.find( localName ) != m_SubDirs.end() )
		{
			::SetLastError( ERROR_FILE_EXISTS );
		}
		else if ( FileSys::IsValidFileName( localName ) )
		{
			std::pair <FileColl::iterator, bool> rc = m_Files.insert( FileColl::value_type( localName, FileData( this ) ) );
			if ( rc.second )
			{
				fileData = &rc.first->second;
			}
			else
			{
				::SetLastError( ERROR_FILE_EXISTS );
			}
		}
		else
		{
			::SetLastError( ERROR_INVALID_NAME );
		}
	}

	return ( fileData );
}

Builder::FileData* Builder::DirData :: ChangeFileName( FileData* file, const gpstring& newName )
{
	gpassert( file != NULL );
	gpassert( !newName.empty() );

	// just add a new one first
	FileData* newEntry = GetFile( newName, m_FileTime );
	if ( newEntry != NULL )
	{
		// copy data over to new entry
		*newEntry = *file;

		// ok now find the old one and erase it
		FileColl::iterator i, ibegin = m_Files.begin(), iend = m_Files.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( &i->second == file )
			{
				m_Files.erase( i );
				break;
			}
		}
		gpassert( i != iend );
	}

	// done
	return ( newEntry );
}

DWORD Builder::DirData :: GetDirCount( void ) const
{
	DWORD count = 1;

	DirColl::const_iterator i, begin = m_SubDirs.begin(), end = m_SubDirs.end();
	for ( i = begin ; i != end ; ++i )
	{
		count += i->second->GetDirCount();
	}

	return ( count );
}

DWORD Builder::DirData :: GetFileCount( void ) const
{
	DWORD count = m_Files.size();

	DirColl::const_iterator i, begin = m_SubDirs.begin(), end = m_SubDirs.end();
	for ( i = begin ; i != end ; ++i )
	{
		count += i->second->GetFileCount();
	}

	return ( count );
}

bool Builder::DirData :: WriteDirs( DWORD base, DWORD parentOffset, OffsetColl::iterator& io, FileSys::File& out, const char* name )
{
	// out parent offset and save it
	m_Offset = out.GetPos() - base;
	*io = m_Offset;
	++io;
	if ( !out.WriteStruct( parentOffset ) )  return ( false );

	// out child count
	DWORD count = m_Files.size() + m_SubDirs.size();
	if ( !out.WriteStruct( count ) )  return ( false );

	// out dir file time
	if ( !out.WriteStruct( m_FileTime ) )  return ( false );

	// out my name
	std::auto_ptr <NSTRING> nameStr( NSTRING::Create( name ) );
	if ( !out.Write( nameStr.get(), nameStr->GetSize() ) )  return ( false );

	// out placeholder offsets (remember where we put them)
	DWORD offsetPos = out.GetPos();
	OffsetColl offsets( m_Files.size() + m_SubDirs.size(), 0 );
	if ( !out.Write( &*offsets.begin(), offsets.size() * sizeof( DWORD ) ) )  return ( false );

	// for each dir
	FileColl::iterator fi, fbegin = m_Files.begin(), fend = m_Files.end();
	DirColl::iterator di, dbegin = m_SubDirs.begin(), dend = m_SubDirs.end();
	OffsetColl::iterator localIo = offsets.begin();
	for ( fi = fbegin, di = dbegin ; ; ++localIo )
	{
		int compare;
		if ( fi == fend )
		{
			if ( di == dend )
			{
				break;
			}
			compare = 1;
		}
		else if ( di == dend )
		{
			compare = -1;
		}
		else
		{
			compare = fi->first.compare_no_case( di->first );
			gpassert( compare != 0 );
		}

		if ( compare < 0 )
		{
			// remember this offset
			fi->second.m_OffsetOffset = offsetPos + (sizeof( DWORD ) * (localIo - offsets.begin()));

			// advance file
			++fi;
		}
		else
		{
			// save offset
			*localIo = out.GetPos() - base;

			// out child
			if ( !di->second->WriteDirs( base, m_Offset, io, out, di->first ) )  return ( false );

			// advance dir
			++di;
		}
	}
	gpassert( localIo == offsets.end() );

	// now backpatch
	DWORD pos = out.GetPos();
	if ( !out.Seek( offsetPos, FILE_BEGIN ) )  return ( false );
	if ( !out.Write( &*offsets.begin(), offsets.size() * sizeof( DWORD ) ) )  return ( false );
	if ( !out.Seek( pos, FILE_BEGIN ) )  return ( false );

	// done
	return ( true );
}

bool Builder::DirData :: WriteFiles( DWORD base, DWORD dirBase, DWORD parentOffset, OffsetColl::iterator& io, FileSys::File& out )
{
	// write local files
	{
		FileColl::iterator i, begin = m_Files.begin(), end = m_Files.end();
		for ( i = begin ; i != end ; ++i )
		{
			// save local offset
			DWORD localOffset = out.GetPos() - base;
			*io = localOffset;
			++io;
			localOffset = out.GetPos() - dirBase;

			// update file entry and write it
			FileEntry& fileEntry = i->second.m_FileEntry;
			fileEntry.m_ParentOffset = parentOffset;
			if ( !out.Write( &fileEntry, sizeof( fileEntry ) - sizeof( NSTRING ) ) )  return ( false );

			// out the name
			std::auto_ptr <NSTRING> nameStr( NSTRING::Create( i->first ) );
			if ( !out.Write( nameStr.get(), nameStr->GetSize() ) )  return ( false );

			// compressed? add header, but skip this if it's a bad file
			if ( !fileEntry.IsInvalidFile() && fileEntry.IsCompressed() )
			{
				// build header
				CompressedHeader header;
				header.m_CompressedSize = i->second.m_CompressedSize;
				header.m_ChunkSize = i->second.m_ChunkSize;

				// if all-in-one or streamed, set to 0 by convention
				if ( (header.m_ChunkSize == fileEntry.m_Size) && !i->second.m_MustChunk )
				{
					header.m_ChunkSize = 0;
				}

				// write it out
				if ( !out.WriteStruct( header ) )  return ( false );

				// if chunked, then output the chunk data (update offsets as we go)
				if ( header.m_ChunkSize != 0 )
				{
					gpassert( !i->second.m_ChunkColl.empty() );
					ChunkColl::iterator j, jbegin = i->second.m_ChunkColl.begin(), jend = i->second.m_ChunkColl.end();
					int chunkOffset = 0;
					for ( j = jbegin ; j != jend ; ++j )
					{
						j->m_Offset = chunkOffset;
						if ( !out.WriteStruct( *j ) )  return ( false );
						chunkOffset += j->m_CompressedSize + j->m_ExtraBytes;
					}
				}

			}

			// backpatch
			DWORD pos = out.GetPos();
			if ( !out.Seek( i->second.m_OffsetOffset, FILE_BEGIN ) )  return ( false );
			if ( !out.WriteStruct( localOffset ) )  return ( false );
			if ( !out.Seek( pos, FILE_BEGIN ) )  return ( false );
		}
	}

	// tell all children to do the same
	{
		DirColl::iterator i, begin = m_SubDirs.begin(), end = m_SubDirs.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( !i->second->WriteFiles( base, dirBase, i->second->m_Offset, io, out ) )
			{
				return ( false );
			}
		}
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Tank

//////////////////////////////////////////////////////////////////////////////
