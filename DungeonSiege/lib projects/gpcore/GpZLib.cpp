//////////////////////////////////////////////////////////////////////////////
//
// File     :  GpZLib.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "GpZLib.h"

#if (GP_DEBUG && !GP_SIEGEMAX)
#pragma comment ( lib, "zlibd.lib" )
#else
#pragma comment ( lib, "zlib.lib" )
#endif

namespace zlib  {  // begin of namespace zlib

//////////////////////////////////////////////////////////////////////////////
// helper function implementations

#if !GP_RETAIL

static PerfCounter s_PerfCounter;

void ResetPerfCounters( void )
{
	s_PerfCounter = PerfCounter();
}

void SetPerfCounters( const PerfCounter& counter )
{
	s_PerfCounter = counter;
}

const PerfCounter& GetPerfCounters( void )
{
	return ( s_PerfCounter );
}

struct AutoPerf
{
	bool   m_Deflating;
	double m_StartTime;

	AutoPerf( bool deflating )
	{
		m_Deflating = deflating;
		m_StartTime = ::GetSystemSeconds();
	}

   ~AutoPerf( void )
	{
		if ( m_Deflating )
		{
			s_PerfCounter.m_SecondsDeflating += ::GetSystemSeconds() - m_StartTime;
		}
		else
		{
			s_PerfCounter.m_SecondsInflating += ::GetSystemSeconds() - m_StartTime;
		}
	}
};

#define AUTO_PERF_INFLATE AutoPerf autoPerf( false )
#define AUTO_PERF_DEFLATE AutoPerf autoPerf( true )

#else // !GP_RETAIL

#define AUTO_PERF_INFLATE
#define AUTO_PERF_DEFLATE

#endif // !GP_RETAIL

int GetMaxDeflateSize( int inSize )
{
	// $ from zlib docs

	return ( inSize + (inSize / 1000)  // 1.1%
					+ 1                // ceil()
					+ 12);             // header
}

RECURSE_DECLARE( Deflate );

int Deflate( mem_ptr out, const_mem_ptr in, int compressionLevel )
{
	RECURSE_LOCK_FOR_WRITE( Deflate );

	int rc = Z_ERRNO;

	static DeflateStreamer s_Streamer;
	if ( s_Streamer.Init( compressionLevel ) && (s_Streamer.DeflateChunk( out, in, true ) != Z_ERRNO) )
	{
		rc = s_Streamer.End();
	}

	return ( rc );
}

RECURSE_DECLARE( Inflate );

int Inflate( mem_ptr out, const_mem_ptr in )
{
	RECURSE_LOCK_FOR_WRITE( Inflate );

	int rc = Z_ERRNO;

	static InflateStreamer s_Streamer;
	if ( s_Streamer.Init() && (s_Streamer.InflateChunk( out, in ) != Z_ERRNO) )
	{
		rc = s_Streamer.End();
	}

	return ( rc );
}

const char* ZlibRcToString( int zrc )
{
	switch ( zrc )
	{
		case ( Z_OK            ):  return ( "Z_OK"            );
		case ( Z_STREAM_END    ):  return ( "Z_STREAM_END"    );
		case ( Z_NEED_DICT     ):  return ( "Z_NEED_DICT"     );
		case ( Z_ERRNO         ):  return ( "Z_ERRNO"         );
		case ( Z_STREAM_ERROR  ):  return ( "Z_STREAM_ERROR"  );
		case ( Z_DATA_ERROR    ):  return ( "Z_DATA_ERROR"    );
		case ( Z_MEM_ERROR     ):  return ( "Z_MEM_ERROR"     );
		case ( Z_BUF_ERROR     ):  return ( "Z_BUF_ERROR"     );
		case ( Z_VERSION_ERROR ):  return ( "Z_VERSION_ERROR" );
	}

	return ( "Z_UNKNOWN" );
}

//////////////////////////////////////////////////////////////////////////////
// class ZDeflateStreamer implementation

DeflateStreamer :: DeflateStreamer( void )
{
	m_WantChunks = false;

	// init it right away to alloc memory
	::ZeroObject( m_Stream );
	::deflateInit( &m_Stream, Z_DEFAULT_COMPRESSION );
	m_CompressionLevel = Z_DEFAULT_COMPRESSION;
}

DeflateStreamer :: ~DeflateStreamer( void )
{
	End();

	// free up memory
	::deflateEnd( &m_Stream );
}

bool DeflateStreamer :: IsActive( void )
{
	return ( m_Stream.state != NULL );
}

bool DeflateStreamer :: Init( int compressionLevel )
{
	AUTO_PERF_DEFLATE;

	End();

	// rebuild if compression level changed
	if ( m_CompressionLevel != compressionLevel )
	{
		::deflateEnd( &m_Stream );
		int rc = ::deflateInit( &m_Stream, compressionLevel );
		if ( rc != Z_OK )
		{
			::SetLastError( rc );
			return ( false );
		}
		m_CompressionLevel = compressionLevel;
	}

	return ( true );
}

int DeflateStreamer :: DeflateChunk( mem_ptr out, const_mem_ptr in, bool finish, int* inUsed )
{
	AUTO_PERF_DEFLATE;

	// passing in 0 means continue to use the existing input buffer (hopefully
	// there is more output space available now)
	if ( in.size != 0 )
	{
		// $ cast away const for bad zlib.h
		m_Stream.next_in = (BYTE*)in.mem;
		m_Stream.avail_in = in.size;
	}

	// receive (maybe) new out buffer
	m_Stream.next_out = (BYTE*)out.mem;
	m_Stream.avail_out = out.size;

	// save old values
	int oldIn = m_Stream.total_in;
	int oldOut = m_Stream.total_out;

	// deflate
	int streamState = ::deflate( &m_Stream, finish ? Z_FINISH : (m_WantChunks ? Z_SYNC_FLUSH : Z_NO_FLUSH) );
	if ( inUsed != NULL )
	{
		*inUsed = m_Stream.total_in - oldIn;
	}
	else
	{
		::SetLastError( streamState );
	}

	// return how much new compressed data there is
	int rc = Z_ERRNO;
	if ((streamState == Z_OK) || (streamState == Z_STREAM_END))
	{
		rc = m_Stream.total_out - oldOut;
	}

	return ( rc );
}

int DeflateStreamer :: End( void )
{
	AUTO_PERF_DEFLATE;

	int rc = m_Stream.total_out;

#	if !GP_RETAIL
	s_PerfCounter.m_BytesDeflatedIn += m_Stream.total_in;
	s_PerfCounter.m_BytesDeflatedOut += m_Stream.total_out;
#	endif // !GP_RETAIL

	// don't end it - reset it. this will save us the time of alloc/realloc if
	// the streamer is used again.
	int resetRc = ::deflateReset( &m_Stream );
	if ( resetRc != Z_OK )
	{
		::SetLastError( resetRc );
		rc = Z_ERRNO;
	}

	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
// class InflateStreamer implementation

InflateStreamer :: InflateStreamer( void )
{
	::ZeroObject( m_Stream );
}

InflateStreamer :: ~InflateStreamer( void )
{
	End();
}

bool InflateStreamer :: IsActive( void )
{
	return ( m_Stream.state != NULL );
}

bool InflateStreamer :: Init( void )
{
	AUTO_PERF_INFLATE;

	End();

	return ( ::inflateInit( &m_Stream ) == Z_OK );
}

int InflateStreamer :: InflateChunk( mem_ptr out, const_mem_ptr in, int* inUsed )
{
	AUTO_PERF_INFLATE;

	// passing in 0 means continue to use the existing input buffer (hopefully
	// there is more output space available now)
	if ( in.size != 0 )
	{
		// $ cast away const for bad zlib.h
		m_Stream.next_in = (BYTE*)in.mem;
		m_Stream.avail_in = in.size;
	}

	// receive (maybe) new out buffer
	m_Stream.next_out = (BYTE*)out.mem;
	m_Stream.avail_out = out.size;

	// save old values
	int oldIn = m_Stream.total_in;
	int oldOut = m_Stream.total_out;

	// inflate
	int streamState = ::inflate( &m_Stream, Z_SYNC_FLUSH );
	if ( inUsed != NULL )
	{
		*inUsed = m_Stream.total_in - oldIn;
	}

	// return how much new compressed data there is
	int rc = Z_ERRNO;
	if ( (streamState == Z_OK) || (streamState == Z_STREAM_END) )
	{
		rc = m_Stream.total_out - oldOut;
	}
	else
	{
		gperror( "InflateStreamer :: InflateChunk - chunk decompression failed." );
		::SetLastError( streamState );
	}

	return ( rc );
}

int InflateStreamer :: End( void )
{
	AUTO_PERF_DEFLATE;

	int rc = Z_ERRNO;
	int inflateRc = ::inflateEnd( &m_Stream );
	if ( inflateRc == Z_OK )
	{
#		if !GP_RETAIL
		s_PerfCounter.m_BytesInflatedIn += m_Stream.total_in;
		s_PerfCounter.m_BytesInflatedOut += m_Stream.total_out;
#		endif // !GP_RETAIL

		rc = m_Stream.total_out;
	}
	else
	{
		::SetLastError( inflateRc );
	}

	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
// class Writer implementation

Writer :: Writer( FileSys::Writer* finalWriter )
{
	// parameter validation
	gpassert( finalWriter != NULL );

	// setup
	m_FinalWriter = finalWriter;
	m_IsCompressing = false;
	m_Buffer.resize( BUFFER_SIZE );
}

Writer :: ~Writer( void )
{
	// must flush before nuking it! note that an exception generated during
	// writing will throw this gpassert too.
	gpassert( !IsCompressing() );
}

bool Writer :: StartCompressing( void )
{
	gpassert( !IsCompressing() );

	m_IsCompressing = true;
	return ( m_Streamer.Init() );
}

bool Writer :: EndCompressing( void )
{
	gpassert( IsCompressing() );

	m_IsCompressing = false;
	return ( Deflate( NULL, 0, true ) && (m_Streamer.End() != Z_ERRNO) );
}

bool Writer :: Write( const void* buffer, int size )
{
	gpassert( (buffer != NULL) && (size >= 0) );

	// $ early bailout - just write it direct
	if ( !IsCompressing() )
	{
		return ( m_FinalWriter->Write( buffer, size ) );
	}

	const BYTE* in = scast <const BYTE*> ( buffer ), * end = in + size;
	while ( in < end )
	{
		// $$ this isn't the most efficient code in the world. zlib does a lot 
		//    of intelligent internal buffering to help out, but really we'd 
		//    bypass a lot of if/else statements if we created an input buffer 
		//    and then sent big chunks out to zlib rather than the piddly-ass 
		//    4-byte stuff we're most likely to run into 90% of the time.

		int write = min_t( end - in, scast <int> ( m_Buffer.size() ) );
		if ( !Deflate( in, write, false ) )
		{
			return ( false );
		}

		in += write;
	}

	return ( true );
}

bool Writer :: Seek( int offset, FileSys::eSeekFrom origin )
{
	gpassert( !IsCompressing() );
	return ( m_FinalWriter->Seek( offset, origin ) );
}

int Writer :: GetPos( void )
{
	gpassert( !IsCompressing() );
	return ( m_FinalWriter->GetPos() );
}

bool Writer :: Deflate( const void* buffer, int size, bool flush )
{
	const BYTE* in = scast <const BYTE*> ( buffer ), * ein = in + size;
	while ( flush || (in < ein) )
	{
		int inused, written = m_Streamer.DeflateChunk(
				mem_ptr( m_Buffer.begin(), m_Buffer.size() ),
				const_mem_ptr( in, ein - in ), flush, &inused );
		if ( written <= 0 )
		{
			return ( false );
		}

		// write out bits
		written = m_FinalWriter->Write( m_Buffer.begin(), written );
		if ( written <= 0 )
		{
			return ( false );
		}

		in += inused;
	}
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// class ZLibReader implementation

#if 0
ZLibReader :: ZLibReader(GReader* finalReader)
{
		// parameter validation
	gpassert(finalReader != NULL);

		// setup
	mReader = finalReader;
	mIsDecompressing = false;
	mBufferPtr = 0;
}

ZLibReader :: ~ZLibReader( void )
{
	EndDecompressing();
}

int ZLibReader :: Read(void *pBuffer, int iSize, int iCount)
{
	gpassert((pBuffer != NULL) && (iSize >= 0) && (iCount >= 0));

	if (!IsDecompressing())
	{
		return (mReader->Read(pBuffer, iSize, iCount));
	}

	byte* out = scast <byte*> (pBuffer), * eout = out + (iSize * iCount);
	const byte* in = m_Buffer.begin() + mBufferPtr, * ein = m_Buffer.end();
	while (out < eout)
	{
		if (in == ein)
		{
				// grow/shrink buffer if necessary
			m_Buffer.resize(min_t(scast <int> (kBufferSize), mReader->GetRemaining()));
			if (m_Buffer.empty())
			{
				return (0);
			}
				// refill buffer
			if (mReader->Read(m_Buffer.begin(), m_Buffer.size(), 1) != 1)
			{
				return (0);
			}

				// reset pointers
			mBufferPtr = 0;
			in = m_Buffer.begin();
			ein = m_Buffer.end();
		}

		// $$ this isn't the most efficient code in the world. zlib does a lot of
		//    intelligent internal buffering to help out, but really we'd bypass
		//    a lot of if/else statements if we created an output buffer and then
		//    sent big chunks out to zlib rather than the piddly-ass 4-byte stuff
		//    we're most likely to run into 90% of the time. ok, not "we". i. me.

		int inused;
		int read = m_Streamer.InflateChunk(out, eout - out, in, ein - in, inused);
		if (read == Z_ERRNO)
		{
			return (0);
		}

		out += read;
		in += inused;
	}

		// remember where we left off
	mBufferPtr = in - m_Buffer.begin();

	return (iCount);
}

int ZLibReader :: GetString(char* pBuffer, int maxchars)
{
		// i'm too lazy to write a GetString() for ZLib - it requires the ability
		//  to seek backwards in the stream after it's been decompressed (i.e. a
		//  sliding output buffer). so screw it, nobody's better call this function
		//  anyway...
	gpassert(!IsDecompressing());
	return (mReader->GetString(pBuffer, maxchars));
}

int ZLibReader :: Seek(int offset, eSeekType origin)
{
	gpassert(!IsDecompressing());
	return (mReader->Seek(offset, origin));
}

int ZLibReader :: GetSize( void )
{
	gpassert(!IsDecompressing());
	return (mReader->GetPos());
}

int ZLibReader :: GetPos( void )
{
	gpassert(!IsDecompressing());
	return (mReader->GetPos());
}

bool ZLibReader :: StartDecompressing( void )
{
	if (!IsDecompressing())
	{
		mIsDecompressing = true;
		if (m_Streamer.Init() == Z_ERRNO)
		{
			return (false);
		}
		mBufferPtr = 0;
	}

	return (true);
}

bool ZLibReader :: EndDecompressing( void )
{
	if (IsDecompressing())
	{
		mIsDecompressing = false;
		if (m_Streamer.End() == Z_ERRNO)
		{
			return (false);
		}
		m_Buffer.clear();
	}

	return (true);
}
#endif // 0

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace zlib

//////////////////////////////////////////////////////////////////////////////
