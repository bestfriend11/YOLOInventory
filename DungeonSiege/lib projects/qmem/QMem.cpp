//////////////////////////////////////////////////////////////////////////////
//
// File     :  QMem.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_QMem.h"
#include "QMem.h"

#if GP_ENABLE_STATS

#include "DllBinder.h"
#include "FileSysDefs.h"
#include "FileSysUtils.h"
#include "QMemLogDb.h"
#include "QProf.h"
#include "ReportSys.h"
#include "StringTool.h"

#include <extern/zlib-1.1.4/zlib.h>
#include <set>

// get zlib
#if GP_DEBUG
#pragma comment ( lib, "zlibd.lib" )
#else
#pragma comment ( lib, "zlib.lib" )
#endif

//////////////////////////////////////////////////////////////////////////////
// Notes

/*
	$$$$ THESE ARE TASKS I NEED TO DO

	* handle multithreading for qprof delivery by spawning threads to do the
	reporting. probably multipass for this will be easiest.
*/

/*
	$$$$ THESE ARE POTENTIAL USES OF QMEM BEYOND MEMORY ANALYSIS

	* texture usage - include texture allocs and deallocs into the event stream.
	this currently is already set up in gpstats. also do sound buffers and the
	diy file map stuff if possible.

	* texture usage - optionally include the name of the texture as a function
	name! this may be a little crazy but it might just work if you do a local
	entry/exit when detecting a texture alloc/dealloc event.

	* frame rate hitch analysis - keep a 10-frame constantly rotating set of
	perf logging data. do not write out to disk or compress (auto-grow the
	sizes if necessary). then upon seeing a frame delay > x% above average or
	> x% above last delay, write out all recent 10 frames

	* thread lock hitch analysis - rather than logging memory or cpu, lock a
	new kernel lock event type that tracks the time spent in every single thread
	lock. every other function call would get 0 delay. this would enable seeing
	exactly what function setups are running into long duration thread locks.
*/

//////////////////////////////////////////////////////////////////////////////
// Asm hook implementation

// $ NOTE: this code is NOT multiproc-friendly! add some LOCK prefixes if you
//         really want it to be safe.

// internal settings
static void* s_LogStorageBase        = NULL;			// output for log samples this chunk
static DWORD s_LogStorageNextIndex   = 0;				// next index to write to for current chunk
static DWORD s_LogStorageMaxIndex    = 0;				// max index we can write out for current chunk
static QWORD s_LogStorageSampleCount = 0;				// total samples collected so far (reset each Enable/DisableLogging call)

// external settings
static DWORD  s_LogCpuCycles   = 0;						// 1 if we log cpu via rdtsc (it's expensive, ~40 cyc, so off by default)
static HANDLE s_LogStorageFile = INVALID_HANDLE_VALUE;	// output of profiling goes here

// constants
#define LOG_CHUNK_SIZE (1 * 1024 * 1024)
#define SAMPLE_SIZE 20
COMPILER_ASSERT( sizeof( QMem::Sample ) == SAMPLE_SIZE );

// $ special: do NOT use any library functions in here because they will be
//            hooked and cause bad recursion. use win32 rtl functions instead.
//            this includes memory allocs, assertions, etc.

const Z_DEFLATE = true;
const Z_INFLATE = false;

// auto z_stream class
struct z_stream_auto : z_stream
{
	bool m_Deflate;

	z_stream_auto( bool deflate )
	{
		::ZeroMemory( this, sizeof( z_stream ) );
		m_Deflate = deflate;

		zalloc = zalloc_global;
		zfree  = zfree_global;

		if ( m_Deflate )
		{
			::deflateInit( this, Z_DEFAULT_COMPRESSION );
		}
		else
		{
			::inflateInit( this );
		}
	}

   ~z_stream_auto( void )
	{
		if ( m_Deflate )
		{
			::deflateEnd( this );
		}
		else
		{
			::inflateEnd( this );
		}
	}

	static void* zalloc_global( void*, unsigned int items, unsigned int size )
	{
		return ( ::GlobalAlloc( GPTR, items * size ) );
	}

	static void zfree_global( void*, void* address )
	{
		::GlobalFree( address );
	}
};

#if GP_PROFILING

// flush function
extern "C" void __stdcall LogFlush( DWORD maxIndex, bool fullFlush )
{
	// get a compression buffer to use - static so we can continue to use it
	// for the entire session
	static z_stream_auto s_Stream( Z_DEFLATE );

	// get some vars - note the buffer size overflow is from zlib docs + slop
	DWORD currentIndex = (s_LogStorageNextIndex > maxIndex) ? maxIndex : s_LogStorageNextIndex;
	DWORD inSize = currentIndex * sizeof( QMem::Sample );
	DWORD outSize = inSize + (inSize / 1000) + 50;

	// alloc temporary storage
	void* outBuffer = ::GlobalAlloc( GPTR, outSize );

	// compress what we've got into the buffer
	DWORD oldOutSize = s_Stream.total_out;
	s_Stream.next_in   = (BYTE*)s_LogStorageBase;
	s_Stream.avail_in  = inSize;
	s_Stream.next_out  = (BYTE*)outBuffer;
	s_Stream.avail_out = outSize;
	::deflate( &s_Stream, fullFlush ? Z_FINISH : Z_SYNC_FLUSH );
	outSize = s_Stream.total_out - oldOutSize;

	// finish if done
	if ( fullFlush )
	{
		// end and reinit for next session
		::deflateReset( &s_Stream );
	}

	// write it to the file
	DWORD written = 0;
	::WriteFile( s_LogStorageFile, outBuffer, outSize, &written, NULL );

	// free temporary storage
	::GlobalFree( outBuffer );

	// update params
	s_LogStorageNextIndex = 0;
	s_LogStorageSampleCount += currentIndex;
}

// hook function
#define QMEMASM_HOOK 1
#include "QMemAsm.cpp"
#undef QMEMASM_HOOK

// local sampling function
#define QMEMASM_INTERNAL 1
#include "QMemAsm.cpp"
#undef QMEMASM_INTERNAL

#endif // GP_PROFILING

//////////////////////////////////////////////////////////////////////////////

namespace QMem  {  // begin of namespace QMem

//////////////////////////////////////////////////////////////////////////////
// QMem API implementations

#if GP_PROFILING

void EnableLogging( HANDLE logStorage, bool logCpuCycles )
{
	// make sure any old logging is cleaned out
	DisableLogging();

	// do we have memory allocated?
	if ( s_LogStorageBase == NULL )
	{
		// nope, go get some
		s_LogStorageBase = ::GlobalAlloc( GMEM_FIXED, LOG_CHUNK_SIZE );

		// auto free the memory we alloc'd
		static struct AutoFreeMem
		{
		   ~AutoFreeMem( void )
			{
				if ( s_LogStorageBase != NULL )
				{
					::GlobalFree( s_LogStorageBase );
				}
			}
		}	s_AutoFreeMem;
	}

	// if not logging cycles, clean the memory so the cycles are zero'd out
	if ( !logCpuCycles )
	{
		::ZeroMemory( s_LogStorageBase, LOG_CHUNK_SIZE );
	}

	// set remaining vars
	s_LogStorageNextIndex = 0;
	s_LogCpuCycles        = logCpuCycles;
	s_LogStorageFile      = logStorage;

	// do this last - it activates logging
	s_LogStorageMaxIndex  = LOG_CHUNK_SIZE / sizeof( Sample );
}

QWORD DisableLogging( void )
{
	// $ early bailout if nothing to do
	if ( s_LogStorageMaxIndex == 0 )
	{
		return ( 0 );
	}

	// first disable logging
	DWORD oldMaxIndex = s_LogStorageMaxIndex;
	s_LogStorageMaxIndex = 0;

	// flush any pending data
	LogFlush( oldMaxIndex, true );

	// return the total
	QWORD oldSampleCount = s_LogStorageSampleCount;
	s_LogStorageSampleCount = 0;
	return ( oldSampleCount );
}

#endif // GP_PROFILING

bool IsLoggingEnabled( void )
{
#	if GP_PROFILING
	return ( s_LogStorageMaxIndex != 0 );
#	else // GP_PROFILING
	return ( false );
#	endif // GP_PROFILING
}

#if GP_PROFILING

static void AddMemEvent( DWORD serialId, LONG delta )
{
	// get sample, early out if not sampling
	DWORD index = SampleAndAdvance();
	if ( index == 0xFFFFFFFF )
	{
		return;
	}
	Sample& sample = ((Sample*)s_LogStorageBase)[ index ];

	// overwrite eip with type
	sample.m_Eip         = EVENT_MEMORY;
	sample.m_MemDelta    = delta;
	sample.m_MemSerialId = serialId;
}

void AddMemAlloc( DWORD serialId, size_t size )
{
	AddMemEvent( serialId, (LONG)size );
}

void AddMemDealloc( DWORD serialId, size_t size )
{
	AddMemEvent( serialId, -(LONG)size );
}

#endif // GP_PROFILING

//////////////////////////////////////////////////////////////////////////////
// struct LogFileHeader implementation

void LogFileHeader :: Init( void )
{
	// clear this
	::ZeroObject( *this );

	// set up simple members
	m_ProductId     = FileSys::PRODUCT_ID;
	m_QMemId        = QMEM_ID;
	m_HeaderVersion = HEADER_VERSION;
	m_LogDataOffset = sizeof( *this );
	m_CpuSpeed      = (QWORD)SysInfo::GetCPUSpeedInHertz();
}

bool LogFileHeader :: IsValidFile( void ) const
{
	return ( m_QMemId == QMEM_ID );
}

//////////////////////////////////////////////////////////////////////////////
// class LogFileWriter implementation

#if GP_PROFILING

LogFileWriter :: LogFileWriter( void )
{
	m_File = INVALID_HANDLE_VALUE;
}

LogFileWriter :: ~LogFileWriter( void )
{
	Close();
}

bool LogFileWriter :: Create( HMODULE module, const char* datFileName )
{
	gpassert( datFileName && *datFileName );

	// clear out any old junk
	Close();

	// open a new file
	m_File = ::CreateFile(
			datFileName,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL );
	if ( m_File == INVALID_HANDLE_VALUE )
	{
		gperrorf(( "Unable to open file '%s', error is '%s'\n",
				   datFileName, stringtool::GetLastErrorText().c_str() ));
		return ( false );
	}

	// skip past where header goes
	::SetFilePointer( m_File, sizeof( m_Header ), NULL, FILE_BEGIN );

	// if null use the current module
	if ( module == NULL )
	{
		module = ::GetModuleHandle( NULL );
	}

	// set header
	m_Header.Init();
	m_Header.m_ImageBase = (DWORD)module;

	// get module's name
	::GetModuleFileName( module, m_Header.m_ImageName, IMAGE_NAME_LENGTH - 1 );

	// get version if available
	m_Header.m_ImageVersion.InitFromResources( m_Header.m_ImageName );

	// done
	return ( true );
}

bool LogFileWriter :: Create( const char* datFileName )
{
	return ( Create( NULL, datFileName ) );
}

void LogFileWriter :: Close( void )
{
	// $ early bailout if nothing to do!
	if ( m_File == INVALID_HANDLE_VALUE )
	{
		return;
	}

	// kill any logging we were doing
	DisableLogging();

	// write the header
	DWORD written;
	::SetFilePointer( m_File, 0, NULL, FILE_BEGIN );
	::WriteFile( m_File, &m_Header, sizeof( m_Header ), &written, NULL );

	// close the file
	::CloseHandle( m_File );
	m_File = INVALID_HANDLE_VALUE;
}

void LogFileWriter :: EnableLogging( bool logCpuCycles )
{
	gpassert( m_File != INVALID_HANDLE_VALUE );
	QMem::EnableLogging( m_File, logCpuCycles );
}

void LogFileWriter :: DisableLogging( void )
{
	gpassert( m_File != INVALID_HANDLE_VALUE );
	m_Header.m_LogEntryCount += QMem::DisableLogging();
}

#endif // GP_PROFILING

//////////////////////////////////////////////////////////////////////////////
// class QProfDll implementation

class QProfDll : public DllBinder, public OnDemandSingleton <QProfDll>
{
public:
	SET_INHERITED( QProfDll, DllBinder );

	QProfDll( void )
		: Inherited( "QPROF.DLL" )
	{
#		define ADD_ENTRY( f )  AddProc( &f, "qprof_" #f )

		ADD_ENTRY( initialize         );
		ADD_ENTRY( shutdown           );
		ADD_ENTRY( record_fn_entry    );
		ADD_ENTRY( record_fn_exit     );
		ADD_ENTRY( verify_fn_exit     );
		ADD_ENTRY( thread_create      );
		ADD_ENTRY( record_counts      );
		ADD_ENTRY( register_method_id );

#		undef ADD_ENTRY
	}

	CdeclProc2 <int,						// initialize(
			const char*,						// initial_options
			void (*)(qprof_MethodID)>			// callback_get_method_info
		initialize;

	CdeclProcV0								// shutdown(
		shutdown;

	CdeclProcV2 <							// record_fn_entry(
			qprof_MethodID,						// method_id
			qprof_StackID>						// stack_id
		record_fn_entry;

	CdeclProcV1 <							// record_fn_exit(
			qprof_StackID>						// id
		record_fn_exit;

	CdeclProcV1 <							// verify_fn_exit(
			qprof_MethodID>						// id
		verify_fn_exit;

	CdeclProcV1 <							// thread_create(
			qprof_ThreadID>						// id
		thread_create;

	CdeclProcV1 <							// record_counts(
			qprof_Counts>						// arg
		record_counts;

	CdeclProcV7 <							// register_method_id(
			qprof_MethodID,						// id
			const char*,						// demangled_name
			const char*,						// class_name
			int,								// flags
			const char*,						// source_file
			const qprof_LineInfo*,				// line_number_table
			int>								// line_number_size
		register_method_id;

private:
	SET_NO_COPYING( QProfDll );
};

#define gQProfDll QProfDll::GetSingleton()

//////////////////////////////////////////////////////////////////////////////
// logging support classes and functions

gpstring XmlEncodeString( const gpstring& str )
{
	if ( str.find_first_of( "<>&" ) == str.npos )
	{
		return ( str );
	}

	gpstring out( str );
	std::replace( out.begin_split(), out.end_split(), '<', '[' );
	std::replace( out.begin_split(), out.end_split(), '>', ']' );
	std::replace( out.begin_split(), out.end_split(), '&', '@' );
	return ( out );
}

struct StackEntry
{
	DWORD m_MethodId;
	DWORD m_CallerMethodId;
	DWORD m_AllocatedBytes;
	QWORD m_CpuCycles;
	bool  m_Recorded;
	bool  m_Skipped;

	StackEntry( DWORD methodId, DWORD callerMethodId, DWORD allocatedBytes, QWORD cpuCycles )
	{
		m_MethodId       = methodId;
		m_CallerMethodId = callerMethodId;
		m_AllocatedBytes = allocatedBytes;
		m_CpuCycles      = cpuCycles;
		m_Recorded       = false;
		m_Skipped        = false;
	}
};

struct Stack : stdx::fast_vector <StackEntry>
{
	ThreadEntry* m_ThreadEntry;
	DWORD        m_AllocatedBytes;
	bool         m_AllocatedBytesChanged;
	QWORD        m_LastCpuCycles;
	QWORD        m_NextCpuCycles;

	Stack( void )
	{
		m_ThreadEntry           = NULL;
		m_AllocatedBytes        = 0;
		m_AllocatedBytesChanged = false;
		m_LastCpuCycles         = 0;
		m_NextCpuCycles         = 0;
	}
};

struct StackDb : stdx::linear_map <DWORD /*threadId*/, Stack>  {  };

struct MemIgnoreDb : std::set <gpstring, string_less>  {  };

struct MemDeallocDb : std::set <DWORD>  {  };

//////////////////////////////////////////////////////////////////////////////
// class SimpleWriter and derivatives implementations

class SimpleWriter
{
public:
	SimpleWriter( bool xmlOut )						{  m_XmlOut = xmlOut;  }

	bool IsXml( void ) const						{  return ( m_XmlOut );  }

	virtual void Indent    ( void ) = 0;
	virtual void Outdent   ( void ) = 0;
	virtual void OutputLine( char* buffer, int len ) = 0;
	virtual void OutputText( const char* buffer, ... ) = 0;

private:
	bool m_XmlOut;
};

class SimpleReportWriter : public SimpleWriter
{
public:
	SimpleReportWriter( ReportSys::ContextRef ctx, bool xmlOut )
													: SimpleWriter( xmlOut ), m_Context( ctx )  {  }

	virtual void Indent ( void )					{  m_Context->Indent();  }
	virtual void Outdent( void )					{  m_Context->Outdent();  }

	virtual void OutputLine( char* buffer, int len )
	{
		m_Context->Output( buffer, len );
		m_Context->OutputEol();
	}

	virtual void OutputText( const char* buffer, ... )
	{
		m_Context->OutputArgs( buffer, va_args( buffer ) );
	}

private:
	ReportSys::ContextRef m_Context;
};

class SimpleFileWriter : public SimpleWriter
{
public:
	SimpleFileWriter( HANDLE file, bool xmlOut )	: SimpleWriter( xmlOut )  {  m_File = file;  m_Indent = 0;  }
	virtual ~SimpleFileWriter( void )				{  m_File.Release();  }

	virtual void Indent( void )						{  ++m_Indent;  }
	virtual void Outdent( void )					{  --m_Indent;  gpassert( m_Indent >= 0 );  }

	void OutputIndents( void )
	{
		// write out indentation if any
		if ( m_Indent > 0 )
		{
			char* indentBuffer = (char*)::_alloca( m_Indent );
			std::fill( indentBuffer, indentBuffer + m_Indent, '\t' );
			m_File.Write( indentBuffer, m_Indent );
		}
	}

	virtual void OutputLine( char* buffer, int len )
	{
		/* this function assumes that buffer has more than 'len' length
		   available so it can cap it with a newline. */

		OutputIndents();

		// tack on newline
		buffer[ len++ ] = '\r';
		buffer[ len++ ] = '\n';

		// write data
		m_File.Write( buffer, len );
	}

	virtual void OutputText( const char* buffer, ... )
	{
		OutputIndents();
		m_File.WriteArgs( buffer, va_args( buffer ) );
	}

private:
	FileSys::File m_File;
	int           m_Indent;
};

//////////////////////////////////////////////////////////////////////////////
// class LogFileReader implementation

LogFileReader :: LogFileReader( void )
{
	m_Options     = OPTION_NONE;
	m_File        = INVALID_HANDLE_VALUE;
	m_LogDb       = NULL;
	m_MemIgnoreDb = NULL;
}

LogFileReader :: ~LogFileReader( void )
{
	Close();
}

bool LogFileReader :: Open( const char* datFileName, const char* imageName )
{
	gpassert( datFileName && *datFileName );

	// clear out any old junk
	Close();

	// open the file
	m_File = ::CreateFile(
			datFileName,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			0,
			NULL );
	if ( m_File == INVALID_HANDLE_VALUE )
	{
		gperrorf(( "Unable to open file '%s', error is '%s'\n",
				   datFileName, stringtool::GetLastErrorText().c_str() ));
		return ( false );
	}

	// read in the full header
	DWORD read = 0;
	if ( !::ReadFile( m_File, &m_Header, sizeof( m_Header ), &read, NULL ) )
	{
		gperrorf(( "Unable to read from file '%s', error is '%s'\n",
				   datFileName, stringtool::GetLastErrorText().c_str() ));
		Close();
		return ( false );
	}

	// check validity
	if ( !m_Header.IsValidFile() )
	{
		gperrorf(( "This is not a QMem format data file or it's corrupt ('%s')\n", datFileName ));
		Close();
		return ( false );
	}

	// get the filename of the exe file next
	gpstring localImageName;
	if ( (imageName == NULL) || (*imageName == '\0') )
	{
		localImageName = m_Header.m_ImageName;
		imageName = localImageName;
	}

	// build a db for this
	m_LogDb = new LogDb( imageName, m_Header.m_ImageBase );

	// build db for ignorables
	m_MemIgnoreDb = new MemIgnoreDb;

	// done
	return ( true );
}

void LogFileReader :: Close( void )
{
	// $ early bailout if nothing to do!
	if ( !IsOpen() )
	{
		return;
	}

	// delete the db's
	Delete( m_LogDb );
	Delete( m_MemIgnoreDb );

	// close the file
	::CloseHandle( m_File );
	m_File = INVALID_HANDLE_VALUE;
}

void LogFileReader :: AddMemIgnoreMethod( const char* spec )
{
	gpassert( IsOpen() );
	m_MemIgnoreDb->insert( spec );
}

bool LogFileReader :: LogToXml( ReportSys::ContextRef ctx )
{
	SimpleReportWriter writer( ctx, true );
	return ( process( &writer ) );
}

bool LogFileReader :: LogToXml( HANDLE xmlFile )
{
	SimpleFileWriter writer( xmlFile, true );
	return ( process( &writer ) );
}

bool LogFileReader :: LogToQProf( ReportSys::ContextRef ctx )
{
	SimpleReportWriter writer( ctx, false );
	return ( process( &writer ) );
}

bool LogFileReader :: LogToQProf( HANDLE xmlFile )
{
	SimpleFileWriter writer( xmlFile, false );
	return ( process( &writer ) );
}

bool LogFileReader :: LogToQuantify( void )
{
	// no writer means use qprof instead
	return ( process( NULL ) );
}

bool LogFileReader :: process( SimpleWriter* writer )
{
	// take writer and build new data structures
	m_Writer       = writer;
	m_StackDb      = new StackDb;
	m_MemDeallocDb = new MemDeallocDb;

// Precache.

	// precache deallocs?
	if ( TestOptions( OPTION_REMOVE_ALLOCS ) && !TestOptions( OPTION_LOG_CPU ) )
	{
		// out status
		gpgeneric( "Precaching: " );

		// decode and process
		if ( !decode( makeFunctor( *this, &LogFileReader::precache ) ) )
		{
			destroyState();
			return ( false );
		}

		// done
		gpgenericf(( "100%% (Found %d temp allocs)\n", m_MemDeallocDb->size() ));
	}

// Log.

	// out status
	gpgeneric( "Processing: " );

	// out the header
	if ( m_Writer != NULL )
	{
		if ( m_Writer->IsXml() )
		{
			m_Writer->OutputText( "<?xml version=\"1.0\"?>\n" );
			m_Writer->OutputText( "<TRACE>\n" );
			if ( TestOptions( OPTION_DEBUG_MODE ) )
			{
				m_Writer->Indent();
			}
		}
		else
		{
			m_Writer->OutputText( "qprof_initialize( \"\", NULL );\n" );
		}
	}
	else
	{
		// attempt to initialize qprof
		if ( !gQProfDll.Load() )
		{
			destroyState();
			return ( false );
		}
		int rc = gQProfDll.initialize( "", NULL );
		if ( rc < 0 )
		{
			gperrorf(( "Unable to initialize the qprof interface, error code is %d\n", rc ));
			destroyState();
			return ( false );
		}
	}

	// decode and process
	if ( !decode( makeFunctor( *this, &LogFileReader::log ) ) )
	{
		destroyState();
		return ( false );
	}

	// flush remaining entries
	for ( StackDb::iterator istackdb = m_StackDb->begin() ; istackdb != m_StackDb->end() ; ++istackdb )
	{
		exitStack( istackdb->second, 0 );
	}

	// finish
	if ( m_Writer != NULL )
	{
		if ( m_Writer->IsXml() )
		{
			if ( TestOptions( OPTION_DEBUG_MODE ) )
			{
				m_Writer->Outdent();
			}
			m_Writer->OutputText( "</TRACE>\n" );
		}
		else
		{
			m_Writer->OutputText( "qprof_shutdown();\n" );
		}
	}
	else if ( !TestOptions( OPTION_DEBUG_MODE ) )
	{
		// done with it for now
		gQProfDll.shutdown();
	}

	// done
	gpgeneric( "100%\n" );
	destroyState();
	return ( true );
}

bool LogFileReader :: decode( ProcessCb cb )
{
	gpassert( cb );
	gpassert( IsOpen() );

	// go right past the header
	::SetFilePointer( m_File, sizeof( m_Header ), NULL, FILE_BEGIN );

	// get some vars - note that we usually get about 95% compression so the
	// in-buffer can be lots smaller
	z_stream_auto decoder( Z_INFLATE );
	const int compressedChunkSize = LOG_CHUNK_SIZE / 10;
	void* outBuffer = ::GlobalAlloc( GMEM_FIXED, LOG_CHUNK_SIZE );
	void* inBuffer = ::GlobalAlloc( GMEM_FIXED, compressedChunkSize );
	bool success = true;

	// set up initial storage
	decoder.next_out = (BYTE*)outBuffer;
	decoder.avail_out = LOG_CHUNK_SIZE;

	// decompress a chunk at a time
	for ( QWORD sampleCount = m_Header.m_LogEntryCount ; sampleCount != 0 ; )
	{
		// read in more data?
		if ( decoder.avail_in == 0 )
		{
			if ( !::ReadFile( m_File, inBuffer, compressedChunkSize, (DWORD*)&decoder.avail_in, NULL ) )
			{
				gperrorf(( "Error reading from file: %s\n", stringtool::GetLastErrorText().c_str() ));
				success = false;
				break;
			}
			decoder.next_in = (BYTE*)inBuffer;
		}

		// always keep decompressing
		int rc = ::inflate( &decoder, Z_SYNC_FLUSH );
		if ( (rc != Z_OK) && (rc != Z_STREAM_END) )
		{
			gperrorf(( "Error decompressing from file\n" ));
			success = false;
			break;
		}

		// output status
		gpgenericf(( "%d%% - ", (int)(100.0 * ((__int64)m_Header.m_LogEntryCount - (__int64)sampleCount) / (__int64)m_Header.m_LogEntryCount) ));

		// output more data?
		if ( decoder.avail_out != LOG_CHUNK_SIZE )
		{
			// get pointers, only do full samples
			int size = LOG_CHUNK_SIZE - decoder.avail_out;
			int count = size / sizeof( Sample );

			// process
			cb( (const Sample*)outBuffer, (const Sample*)outBuffer + count );
			sampleCount -= count;

			// move remainder over
			int remain = size - (count * sizeof( Sample ));
			if ( remain > 0 )
			{
				::memmove( outBuffer, (BYTE*)outBuffer + size - remain, remain );
			}
			decoder.next_out = (BYTE*)outBuffer + remain;
			decoder.avail_out = LOG_CHUNK_SIZE - remain;
		}
	}

	// free buffers
	::GlobalFree( outBuffer );
	::GlobalFree( inBuffer );

	// done
	return ( success );
}

void LogFileReader :: destroyState( void )
{
	m_Writer = NULL;
	Delete( m_StackDb );
	Delete( m_MemDeallocDb );
}

void LogFileReader :: precache( const Sample* sampleBegin, const Sample* sampleEnd )
{
	gpassert( TestOptions( OPTION_REMOVE_ALLOCS ) );
	gpassert( !TestOptions( OPTION_LOG_CPU ) );

	// loop through all samples
	for ( const Sample* isample = sampleBegin ; isample != sampleEnd ; ++isample )
	{
		// memory event?
		if ( (isample->m_Eip == EVENT_MEMORY) && (isample->m_MemDelta < 0) )
		{
			// add serial to the db
			m_MemDeallocDb->insert( isample->m_MemSerialId );
		}
	}
}

void LogFileReader :: log( const Sample* sampleBegin, const Sample* sampleEnd )
{
	// get some type vars
	bool isqprof = (m_Writer == NULL) || !m_Writer->IsXml();
	bool isxml   = (m_Writer != NULL) &&  m_Writer->IsXml();

	// loop through all samples
	for ( const Sample* isample = sampleBegin ; isample != sampleEnd ; ++isample )
	{
		// get some db vars
		ThreadEntry* threadEntry = NULL, * callerThreadEntry = NULL;
		ClassEntry * classEntry  = NULL, * callerClassEntry  = NULL;
		MethodEntry* methodEntry = NULL, * callerMethodEntry = NULL;
		bool threadEntryNew       = false, classEntryNew       = false, methodEntryNew       = false;
		bool callerThreadEntryNew = false, callerClassEntryNew = false, callerMethodEntryNew = false;

		// memory event?
		if ( isample->m_Eip == EVENT_MEMORY )
		{
			// skip completely if not gathering memory data
			if ( TestOptions( OPTION_LOG_CPU ) )
			{
				continue;
			}

			// only register allocs
			if ( isample->m_MemDelta < 0 )
			{
				continue;
			}

			// if we're removing allocs, check the db and skip if found
			if ( TestOptions( OPTION_REMOVE_ALLOCS ) )
			{
				MemDeallocDb::iterator found = m_MemDeallocDb->find( isample->m_MemSerialId );
				if ( found != m_MemDeallocDb->end() )
				{
					m_MemDeallocDb->erase( found );
					continue;
				}
			}

			// get the thread - we only need the thread data for a memory event
			// so we can assign the alloc to a particular thread.
			m_LogDb->AddThread( isample->m_Esp, threadEntry, threadEntryNew );

			// it should already exist because we won't get a memory alloc
			// registered without a previous fn call on this thread being done.
			gpassert( !threadEntryNew );
		}
		else
		{
			// if we're here we can't have an event type
			gpassert( isample->IsFunctionSample() );

			// add all to the db
			m_LogDb->AddMethod(
					isample->m_Eip, isample->m_Esp,
					threadEntry, threadEntryNew,
					classEntry , classEntryNew ,
					methodEntry, methodEntryNew );
		}

		// $$$$$$$ for now, we have to do a single thread for qprof because
		// it requires that all data delivered for a particular thread be
		// delivered on a *separate* thread (within this tool exe)

		// if we're only interested in the first thread, skip if it's not that
		if ( (threadEntry->m_ThreadId != 1) && (TestOptions( OPTION_SINGLE_THREAD ) || /*$$$TEMP*/ isqprof) )
		{
			// first saw the method on another thread - register it!
			if ( isample->IsFunctionSample() )
			{
				registerMethod( threadEntry, threadEntryNew, classEntry, classEntryNew, methodEntry, methodEntryNew );
			}

			// now we can skip
			continue;
		}

		// find our stack
		Stack& stack = (*m_StackDb)[ threadEntry->m_ThreadId ];

		// configure stack entry if necessary
		gpassert( (stack.m_ThreadEntry == NULL) == threadEntryNew );
		if ( threadEntryNew )
		{
			stack.m_ThreadEntry = threadEntry;
		}
		gpassert( stack.m_ThreadEntry == threadEntry );

		// process sample
		if ( isample->m_Eip == EVENT_MEMORY )
		{
			// $ memory sample
			gpassert( !TestOptions( OPTION_LOG_CPU ) );

			// can't have an empty stack and get here, we have to be in a func
			gpassert( !stack.empty() );

			// get memory size, should not be a dealloc at this point
			DWORD allocSize = isample->m_MemDelta;
			gpassert( allocSize >= 0 );

			// adjust for zero if requested or skip otherwise
			if ( allocSize == 0 )
			{
				if ( TestOptions( OPTION_NO_ZERO_ALLOCS ) )
				{
					allocSize = 1;
				}
				else
				{
					// skip it entirely
					continue;
				}
			}

			// xml requires absolute times, qprof wants relative, and memory
			// is coming in as relative
			if ( isxml )
			{
				stack.m_AllocatedBytes += allocSize;
			}
			else
			{
				// current function for qprof is already on top of the stack,
				// we need to get to it now before it exits because unlike xml
				// there is no timing emitted on exit
				gpassert( !stack.empty() );
				stack.back().m_AllocatedBytes = allocSize;
			}

			// it changed
			stack.m_AllocatedBytesChanged = true;
		}
		else
		{
			// $ function entry sample

			// we also need the caller method id
			m_LogDb->AddMethod(
					isample->m_CallerEip, isample->m_Esp,
					callerThreadEntry, callerThreadEntryNew,
					callerClassEntry , callerClassEntryNew ,
					callerMethodEntry, callerMethodEntryNew );
			gpassert( !callerThreadEntryNew );
			gpassert( callerThreadEntry == threadEntry );
			DWORD callerMethodId = callerMethodEntry->m_MethodId;

			// set cpu cycles if necessary
			if ( TestOptions( OPTION_LOG_CPU ) )
			{
				// xml requires absolute times, qprof wants relative, and cpu
				// is coming in as absolute
				stack.m_NextCpuCycles = isample->m_CpuTimeStamp;
				if ( !isxml )
				{
					stack.m_NextCpuCycles -= stack.m_LastCpuCycles;
				}
				stack.m_LastCpuCycles = isample->m_CpuTimeStamp;
			}

			// figure out entry vs. exit by finding the common calling function
			exitStack( stack, callerMethodId );

			// now that we're at the same level of indentation as our next
			// function entry point, out new db entries as we discover them in
			// the log stream
			registerMethod( threadEntry, threadEntryNew, classEntry, classEntryNew, methodEntry, methodEntryNew );
			registerMethod( callerThreadEntry, callerThreadEntryNew, callerClassEntry, callerClassEntryNew, callerMethodEntry, callerMethodEntryNew );

			// if it's the first entry in this stack, be sure to add the base method
			if ( stack.empty() )
			{
				stack.push_back( StackEntry( callerMethodEntry->m_MethodId, 0, 0, 0 ) );
			}

			// add it
			stack.push_back( StackEntry(
					methodEntry->m_MethodId,
					callerMethodId,
					stack.m_AllocatedBytes,
					stack.m_NextCpuCycles ) );

			// because qprof is relative, clean out values for further iters
			if ( isqprof )
			{
				stack.m_AllocatedBytes = 0;
				stack.m_NextCpuCycles  = 0;
			}
		}
	}
}

void LogFileReader :: exitStack( Stack& stack, DWORD untilCallerMethodId )
{
	char  buffer[ 2048 ];
	char* bufferIter = NULL;
	int   bufferCount = 0;

	// detect if something has changed, requiring stack output
	bool changed
			=    TestOptions( OPTION_CHANGED_ALWAYS )
			  || TestOptions( OPTION_LOG_CPU )
			  || stack.m_AllocatedBytesChanged;

	// on a change, record all deferred entries
	if ( changed )
	{
		bool isqprof = (m_Writer == NULL) || !m_Writer->IsXml();
		stack.m_AllocatedBytesChanged = false;

		// first preprocess list, looking for entries to skip if doing mem analysis
		if ( !TestOptions( OPTION_LOG_CPU ) && isqprof )
		{
			DWORD allocBytesKeeper = 0;

			Stack::reverse_iterator i, ibegin = stack.rbegin(), iend = stack.rend();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( !i->m_Recorded )
				{
					// should we skip?
					bool skip = m_MemIgnoreDb->find( m_LogDb->GetMethodName( i->m_MethodId ) ) != m_MemIgnoreDb->end();

					// check to be sure we CAN skip
					if ( skip )
					{
						if ( (i + 1) == iend )
						{
							// if no base method, can't skip
							skip = false;
						}
						else if ( (i + 1)->m_Recorded )
						{
							// there is a base method and it's already recorded,
							// so re-output the additional counts.
							allocBytesKeeper += i->m_AllocatedBytes;
							i->m_Recorded = true;
							i->m_Skipped = true;
							++i;
							i->m_AllocatedBytes += allocBytesKeeper;
							if ( m_Writer != NULL )
							{
								if ( TestOptions( OPTION_DEBUG_MODE ) )
								{
									m_Writer->Outdent();
								}
								m_Writer->OutputText( "qprof_record_counts( %d )\n", allocBytesKeeper );
								if ( TestOptions( OPTION_DEBUG_MODE ) )
								{
									m_Writer->Indent();
								}
							}
							else
							{
								gQProfDll.record_counts( allocBytesKeeper );
							}
							break;
						}
					}

					// ok do it
					if ( skip )
					{
						// found one we can skip
						i->m_Recorded = true;
						i->m_Skipped = true;
						allocBytesKeeper += i->m_AllocatedBytes;
					}
					else
					{
						// add mem credit taken to this topmost non-skipped entry
						i->m_AllocatedBytes += allocBytesKeeper;

						// no longer need to keep going, credit has been assigned
						break;
					}
				}
			}
		}

		// next process list, outputting stack entry records for non-skipped
		{
			Stack::iterator i, ibegin = stack.begin(), iend = stack.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( !i->m_Recorded )
				{
					i->m_Recorded = true;

					if ( m_Writer != NULL )
					{
						bufferIter = buffer;
						bufferCount = ELEMENT_COUNT( buffer ) - 2;		// -2 for trailing \r\n

						if ( m_Writer->IsXml() )
						{
							// build base data
							advance_snprintf( bufferIter, bufferCount,
									"<methodEntry threadIdRef=\"%d\" methodIdRef=\"%d\"",
									stack.m_ThreadEntry->m_ThreadId, i->m_MethodId );

							// build counter data
							if ( TestOptions( OPTION_LOG_CPU ) )
							{
								advance_snprintf( bufferIter, bufferCount,
										" time=\"%.9f\"", (__int64)i->m_CpuCycles / (double)m_Header.m_CpuSpeed );
							}
							else
							{
								advance_snprintf( bufferIter, bufferCount,
										" time=\"%.6f\"", i->m_AllocatedBytes * 0.000001 );
							}

							// build extra debug data
							if ( TestOptions( OPTION_DEBUG_MODE ) )
							{
								advance_snprintf( bufferIter, bufferCount,
										" methodName=\"%s\" className=\"%s\" threadName=\"%s\" stackId=\"%d\"",
										XmlEncodeString( m_LogDb->GetMethodName( i->m_MethodId ) ).c_str(),
										XmlEncodeString( m_LogDb->GetClassNameByMethodId( i->m_MethodId ) ).c_str(),
										XmlEncodeString( stack.m_ThreadEntry->m_ThreadName ).c_str(),
										(i->m_MethodId, i - stack.begin() + 1) );
							}
							advance_snprintf( bufferIter, bufferCount, "/>" );
						}
						else
						{
							// build base data
							advance_snprintf( bufferIter, bufferCount,
									"qprof_record_fn_entry( %d, %d )",
									 i->m_MethodId, i - stack.begin() + 1 );

							// build extra debug data
							if ( TestOptions( OPTION_DEBUG_MODE ) )
							{
								advance_snprintf( bufferIter, bufferCount,
										" // %s", m_LogDb->GetMethodName( i->m_MethodId ).c_str() );
							}
						}

						// output result
						m_Writer->OutputLine( buffer, bufferIter - buffer );

						// extra
						if ( !m_Writer->IsXml() )
						{
							// output counts
							if ( TestOptions( OPTION_LOG_CPU ) )
							{
								m_Writer->OutputText( "qprof_record_counts( %d )\n", (DWORD)i->m_CpuCycles );
							}
							else
							{
								m_Writer->OutputText( "qprof_record_counts( %d )\n", i->m_AllocatedBytes );
							}
						}

						// we're entering this method
						if ( TestOptions( OPTION_DEBUG_MODE ) )
						{
							m_Writer->Indent();
						}
					}
					else
					{
						// record function entry, id base is 1
						gQProfDll.record_fn_entry( i->m_MethodId, i - stack.begin() + 1 );

						// record timings
						if ( TestOptions( OPTION_LOG_CPU ) )
						{
							gQProfDll.record_counts( (DWORD)i->m_CpuCycles );
						}
						else
						{
							gQProfDll.record_counts( i->m_AllocatedBytes );
						}
					}
				}
			}
		}
	}

	// find the caller on the stack
	Stack::reverse_iterator istack;
	if ( untilCallerMethodId != 0 )
	{
		for ( istack = stack.rbegin() ; istack != stack.rend() ; ++istack )
		{
			// if we have the same caller then we're at the same level
			if ( istack->m_CallerMethodId == untilCallerMethodId )
			{
				++istack;
				break;
			}
		}

		// if not found, then skip entirely (we're entering a nested function, not leaving)
		if ( istack == stack.rend() )
		{
			return;
		}
	}
	else
	{
		// if id is 0 then flush all
		istack = stack.rend();
	}

	// pop entries until we get there
	for ( Stack::reverse_iterator ipop = stack.rbegin() ; ipop != istack ; ++ipop )
	{
		// only output exit data if entry data was done and not skipped
		if ( ipop->m_Recorded && !ipop->m_Skipped )
		{
			if ( m_Writer != NULL )
			{
				// we're leaving this method
				if ( TestOptions( OPTION_DEBUG_MODE ) )
				{
					m_Writer->Outdent();
				}

				bufferIter = buffer;
				bufferCount = ELEMENT_COUNT( buffer ) - 2;		// -2 for trailing \r\n

				if ( m_Writer->IsXml() )
				{
					// build base data
					advance_snprintf( bufferIter, bufferCount,
							"<methodExit threadIdRef=\"%d\" methodIdRef=\"%d\"",
							stack.m_ThreadEntry->m_ThreadId, ipop->m_MethodId );

					// build counter data
					if ( TestOptions( OPTION_LOG_CPU ) )
					{
						advance_snprintf( bufferIter, bufferCount,
								" time=\"%.9f\"", (__int64)stack.m_NextCpuCycles / (double)m_Header.m_CpuSpeed );
					}
					else
					{
						advance_snprintf( bufferIter, bufferCount,
								" time=\"%.6f\"", stack.m_AllocatedBytes * 0.000001 );
					}

					// build extra debug data
					if ( TestOptions( OPTION_DEBUG_MODE ) )
					{
						advance_snprintf( bufferIter, bufferCount,
								" methodName=\"%s\" className=\"%s\" threadName=\"%s\" stackId=\"%d\"",
								XmlEncodeString( m_LogDb->GetMethodName( ipop->m_MethodId ) ).c_str(),
								XmlEncodeString( m_LogDb->GetClassNameByMethodId( ipop->m_MethodId ) ).c_str(),
								XmlEncodeString( stack.m_ThreadEntry->m_ThreadName ).c_str(),
								ipop.base() - stack.begin() - 1 );
					}
					advance_snprintf( bufferIter, bufferCount, "/>" );
				}
				else
				{
					// build base data
					if ( stack.size() > 1 )
					{
						advance_snprintf( bufferIter, bufferCount,
								"qprof_record_fn_exit( %d )",
								ipop.base() - stack.begin() - 1 );
					}
					else
					{
						advance_snprintf( bufferIter, bufferCount,
								"qprof_verify_fn_exit( %d )",
								ipop->m_MethodId );
					}

					// build extra debug data
					if ( TestOptions( OPTION_DEBUG_MODE ) )
					{
						advance_snprintf( bufferIter, bufferCount,
								" // %s", m_LogDb->GetMethodName( ipop->m_MethodId ).c_str() );
					}
				}

				// output result
				m_Writer->OutputLine( buffer, bufferIter - buffer );
			}
			else
			{
				if ( stack.size() > 1 )
				{
					// record function exit, use caller's id
					gQProfDll.record_fn_exit( ipop.base() - stack.begin() - 1 );
				}
				else
				{
					// last one, no source stack to use, do what qprof.h says
					gQProfDll.verify_fn_exit( ipop->m_MethodId );
				}
			}
		}

		// remove it (this is safe, no reallocs)
		stack.pop_back();
	}
}

void LogFileReader :: registerMethod(
		ThreadEntry*  threadEntry, bool threadEntryNew,
		ClassEntry *  classEntry , bool classEntryNew ,
		MethodEntry*  methodEntry, bool methodEntryNew )
{
	if ( threadEntryNew )
	{
		if ( m_Writer != NULL )
		{
			if ( m_Writer->IsXml() )
			{
				m_Writer->OutputText(
						"<threadStart threadName=\"%s\" threadId=\"%d\"/>\n",
						XmlEncodeString( threadEntry->m_ThreadName ).c_str(),
						threadEntry->m_ThreadId );
			}
			else
			{
				m_Writer->OutputText( "qprof_thread_create( %d )\n", threadEntry->m_ThreadId );
			}
		}
		else
		{
			gQProfDll.thread_create( threadEntry->m_ThreadId );
		}
	}
	if ( classEntryNew )
	{
		if ( m_Writer != NULL )
		{
			if ( m_Writer->IsXml() )
			{
				m_Writer->OutputText(
						"<classDef name=\"%s\" sourceName=\"\" classId=\"%d\"/>\n",
						XmlEncodeString( classEntry->m_ClassName ).c_str(),
						classEntry->m_ClassId );
			}
		}
	}
	if ( methodEntryNew )
	{
		if ( m_Writer != NULL )
		{
			if ( m_Writer->IsXml() )
			{
				m_Writer->OutputText(
						"<methodDef name=\"%s\" signature=\"\" methodId=\"%d\" classIdRef=\"%d\"/>\n",
						XmlEncodeString( methodEntry->m_MethodName ).c_str(),
						methodEntry->m_MethodId,
						methodEntry->m_ClassId );
			}
			else
			{
				m_Writer->OutputText( "qprof_register_method_id( %d, \"%s\", \"%s\", QPROF_METHOD_FLAGS_NONE, \"%s\", %d, 1 )\n",
						methodEntry->m_MethodId,
						methodEntry->m_MethodName.c_str(),
						classEntry->m_ClassName.c_str(),
						methodEntry->m_SourceFile.c_str(),
						methodEntry->m_SourceLine );
			}
		}
		else
		{
			qprof_LineInfo info;
			info.code_offset = 0;
			info.line_number = methodEntry->m_SourceLine;

			gQProfDll.register_method_id(
					methodEntry->m_MethodId,
					methodEntry->m_MethodName,
					classEntry->m_ClassName,
					QPROF_METHOD_FLAGS_NONE,
					methodEntry->m_SourceFile,
					&info,
					1 );
		}
	}
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace QMem

//////////////////////////////////////////////////////////////////////////////
// class QMemApi implementation

#if GP_PROFILING

QMemApi :: QMemApi( void )
{
	m_Profiling  = false;
	m_FrameCount = 0;
}

void QMemApi :: AddFrame( void )
{
	if ( m_Profiling && (m_FrameCount != -1) && (m_FrameCount-- == 0) )
	{
		StopProfile();
	}
}

void QMemApi :: BeginProfile( int frameCount )
{
	m_Profiling  = true;
	m_FrameCount = frameCount;

	SYSTEMTIME time;
	::GetLocalTime( &time );

	gpstring fileName;
	fileName.assignf(
			"%d_frames_%04d_%02d_%02d_%02d_%02d_%02d.dat",
			time.wYear,
			time.wMonth,
			time.wDay,
			time.wHour,
			time.wMinute,
			time.wSecond );

	m_Writer.Create( fileName );
}

void QMemApi :: StopProfile( void )
{
	if ( m_Profiling )
	{
		m_Profiling = false;
		m_Writer.DisableLogging();
	}
}

#endif // GP_PROFILING

//////////////////////////////////////////////////////////////////////////////

#endif // GP_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////
