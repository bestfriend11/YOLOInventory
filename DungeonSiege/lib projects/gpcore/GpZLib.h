//////////////////////////////////////////////////////////////////////////////
//
// File     :  GpZLib.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains wrappers for the zlib compression algorithm.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GPZLIB_H
#define __GPZLIB_H

//////////////////////////////////////////////////////////////////////////////

#include "FileSysDefs.h"

#include <extern/zlib-1.1.4/zlib.h>

namespace zlib  {  // begin of namespace zlib

//////////////////////////////////////////////////////////////////////////////
// helper function declarations

// Performance.

#	if !GP_RETAIL

	struct PerfCounter
	{
		PerfCounter(void)  {  ::ZeroObject( *this );  }

		int    m_BytesDeflatedIn;
		int    m_BytesDeflatedOut;
		double m_SecondsDeflating;

		int    m_BytesInflatedIn;
		int    m_BytesInflatedOut;
		double m_SecondsInflating;
	};

	// for performance monitoring
	void               ResetPerfCounters( void );
	void               SetPerfCounters  ( const PerfCounter& counter );
	const PerfCounter& GetPerfCounters  ( void );

#	endif // !GP_RETAIL

// Deflation.

	// $$ future: shared dictionary support

	// gets worst-case size for a call to Deflate() (assuming zero compression
	// ratio + overhead). if compressing an entire buffer at once, call this
	// first to get the output size to guarantee the call succeeds.
	int GetMaxDeflateSize( int inSize );

	// compresses data all at once, returns Z_ERRNO on failure, size of compressed
	// data otherwise. compressionLevel goes from 1 (fastest) to 9 (best
	// compression). see zlib docs for more info.
	int Deflate( mem_ptr out, const_mem_ptr in, int compressionLevel = Z_DEFAULT_COMPRESSION );

	// decompresses data all at once, returns Z_ERRNO on failure, size of
	// uncompressed data otherwise.
	int Inflate( mem_ptr out, const_mem_ptr in );

	// converts a return from zlib to a string
	const char* ZlibRcToString( int zrc );

//////////////////////////////////////////////////////////////////////////////
// class DeflateStreamer declaration

class DeflateStreamer
{
public:
	SET_NO_INHERITED( DeflateStreamer );

	// ctor/dtor
	DeflateStreamer( void );
   ~DeflateStreamer( void );

	// currently deflating?
	bool IsActive( void );

	// start a deflation
	bool Init( int compressionLevel = Z_DEFAULT_COMPRESSION );

	// returns size of compressed data, Z_ERRNO on failure. to finish, always call
	// extra times with flush == true until it returns 0.
	int DeflateChunk( mem_ptr out, const_mem_ptr in, bool finish, int* inUsed = NULL );

	// returns Z_ERRNO on failure, total_size otherwise
	int End( void );

	// set to true to add a sync flush after each deflation call
	void SetChunkBased( bool wantChunks = true )		{  m_WantChunks = wantChunks;  }
	bool GetChunkbased( void ) const					{  return ( m_WantChunks );  }

private:
	z_stream_s m_Stream;
	bool       m_WantChunks;
	int        m_CompressionLevel;

	SET_NO_COPYING( DeflateStreamer );
};

//////////////////////////////////////////////////////////////////////////////
// class InflateStreamer declaration

class InflateStreamer
{
public:
	SET_NO_INHERITED( InflateStreamer );

	// ctor/dtor
	InflateStreamer( void );
   ~InflateStreamer( void );

	// currently inflating?
	bool IsActive( void );

	// start an inflation
	bool Init( void );

	// returns size of compressed data, Z_ERRNO on failure, 0 on stream end
	int InflateChunk( mem_ptr out, const_mem_ptr in, int* inUsed = NULL );

	// returns Z_ERRNO on failure, total_size otherwise
	int End( void );

private:
	z_stream_s m_Stream;

	SET_NO_COPYING( InflateStreamer );
};

//////////////////////////////////////////////////////////////////////////////
// class Writer declaration

class Writer : public FileSys::Writer
{
public:
	SET_INHERITED( Writer, FileSys::Writer );

	// ctor/dtor
	Writer( FileSys::Writer* finalWriter );
	virtual ~Writer( void );

	// compression enabling
	bool StartCompressing( void );
	bool EndCompressing  ( void );
	bool IsCompressing   ( void ) const  {  return ( m_IsCompressing );  }

	// overrides
	virtual bool Write ( const void* out, int size );
	virtual bool Seek  ( int offset, FileSys::eSeekFrom origin = FileSys::SEEKFROM_BEGIN );
	virtual int  GetPos( void );

private:
	bool Deflate( const void* buffer, int size, bool flush );

	enum  {  BUFFER_SIZE = 1024  };
	typedef std::vector <BYTE> Buffer;

	FileSys::Writer* m_FinalWriter;			// where final output goes (compressed)
	bool             m_IsCompressing;		// true if we're currently compressing output
	Buffer           m_Buffer;				// compress into here, buffer for out to the final writer
	DeflateStreamer  m_Streamer;			// this does the compression

	SET_NO_COPYING( Writer );
};

//////////////////////////////////////////////////////////////////////////////
// class ZLibReader declaration

#if 0
class ZLibReader : public GReader
{
	public:
		SET_INHERITED(ZLibReader, GReader);

		ZLibReader(GReader* finalReader);
		virtual ~ZLibReader(void);

		virtual int Read(void *pBuffer, int iSize, int iCount = 1);
		virtual int GetString(char *pBuffer, int maxchars);
		virtual int Seek(int offset, eSeekType origin);
		virtual int GetSize(void);
		virtual int GetPos(void);

		bool StartDecompressing(void);
		bool EndDecompressing(void);
		bool IsDecompressing(void) const
			{  return (mIsDecompressing);  }

	private:
		enum  {  kBufferSize = 1024  };

		GReader* mReader;
		bool mIsDecompressing;
		vector <byte> mBuffer;
		int mBufferPtr;
		InflateStreamer mStreamer;

		SET_NO_COPYING(ZLibReader);
};
#endif // 0

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace zlib

#endif  // __GPZLIB_H

//////////////////////////////////////////////////////////////////////////////
