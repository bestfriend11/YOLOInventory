//////////////////////////////////////////////////////////////////////////////
//
// File     :  QMem.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains automatic resource allocation profiling utilities that
//             are meant for output to XML that Quantify can navigate.
//
//             IMPORTANT: this only works if stats tracking is enabled!
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __QMEM_H
#define __QMEM_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"
#include "FuBiDefs.h"

#if GP_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////

namespace QMem  {  // begin of namespace QMem

//////////////////////////////////////////////////////////////////////////////
// QMem low-level API declarations

// Setup.

	// enabling control
#	if GP_PROFILING
	void  EnableLogging   ( HANDLE logStorage, bool logCpuCycles = false );
	QWORD DisableLogging  ( void );
#	endif // GP_PROFILING
	bool  IsLoggingEnabled( void );

// Events.

	// types of events
	enum eEventType
	{
		EVENT_MEMORY						// sample is a memory event
	};

	// memory events
#	if GP_PROFILING
	void AddMemAlloc  ( DWORD serialId, size_t size );
	void AddMemDealloc( DWORD serialId, size_t size );
#	endif // GP_PROFILING

	// each sample looks like this
#	pragma pack ( push, 1 )
	struct Sample
	{
		DWORD m_Eip;						// instruction pointer of function sample, can be used for symbol lookup (if high word is 0 then it's an event)
		DWORD m_CallerEip;					// calling eip of function sample - used to detect function entry/exit
		DWORD m_Esp;						// stack pointer at time of function sample, used to detect thread

		union
		{
			// function samples use this
			QWORD m_CpuTimeStamp;			// cpu time stamp at time of sample (from rdtsc)

			// memory event
			struct
			{
				LONG  m_MemDelta;			// size of memory allocated/deallocated (negative if dealloc)
				DWORD m_MemSerialId;		// serial id of memory allocation, used for comparisons
			};
		};

		bool IsFunctionSample( void ) const		{  return ( !!(m_Eip & 0xFFFFFF00) );  }
	};
#	pragma pack ( pop )

//////////////////////////////////////////////////////////////////////////////
// File logging declarations

	// constants
	enum
	{
		HEADER_VERSION    = MAKEVERSION( 1, 1, 0 ),			// $ change when modifying the static data format
		IMAGE_NAME_LENGTH = 256,
		QMEM_ID           = REVERSE_FOURCC( 'QMem' ),
		RAW_HEADER_PAD    = 16,								// used for padding between end of header and start of raw data
	};

	// raw header structure
	struct LogFileHeader
	{
		// $ all offsets are to the base of Header (this)

		//             ------ Base ------
/*R4 */	FOURCC    m_ProductId;							// ID of product (human readable) - always PRODUCT_ID
/*R4 */	FOURCC    m_QMemId;								// ID of tank (human readable) - always QMEM_ID
/*WW */	DWORD     m_HeaderVersion;						// version of this particular header
/*HO */	DWORD     m_LogDataOffset;						// (Sample*) offset

		//             ------ V1.0 Extra - Basic ------
/*12 */	gpversion m_ImageVersion;						// version of image that was sampled
		DWORD     m_ImageBase;							// base of image, usually 0x400000
		QWORD     m_CpuSpeed;							// CPU speed in hz
		QWORD     m_LogEntryCount;						// count of log entries stored back to back
/*ZST*/	CHAR      m_ImageName[ IMAGE_NAME_LENGTH ];		// name of image file

		//             ------ Padding ------
/*PAD*/	BYTE      m_Reserved[ RAW_HEADER_PAD ];			// be sure to move this past the last header as more are added

		//             ------ Data ------
	//  MemSample m_Samples[ m_LogEntryCount ];			// (COMPRESSED) sample data follows the header and padding at m_LogDataOffset

		void Init       ( void );
		bool IsValidFile( void ) const;
	};

#	if GP_PROFILING

	class LogFileWriter
	{
	public:
		SET_NO_INHERITED( LogFileWriter );

		LogFileWriter( void );
	   ~LogFileWriter( void );

		bool Create( HMODULE module, const char* datFileName );
		bool Create( const char* datFileName );
		void Close ( void );

		void EnableLogging   ( bool logCpuCycles = false );
		void DisableLogging  ( void );
		bool IsLoggingEnabled( void ) const					{  return ( QMem::IsLoggingEnabled() );  }

	private:
		LogFileHeader m_Header;			// header for file
		HANDLE        m_File;			// file we're outputting to

		SET_NO_COPYING( LogFileWriter );
	};

#	endif // GP_PROFILING

	// forward declarations
	class LogDb;
	class SimpleWriter;
	struct Stack;
	struct StackDb;
	struct MemIgnoreDb;
	struct MemDeallocDb;
	struct ThreadEntry;
	struct ClassEntry;
	struct MethodEntry;

	// helper class for analyzing dats
	class LogFileReader
	{
	public:
		SET_NO_INHERITED( LogFileReader );

		enum eOptions								
		{
			OPTION_LOG_CPU        = 1 << 0,		// log cpu cycles instead of memory
			OPTION_DEBUG_MODE     = 1 << 1,		// do debug mode - include more info so it's easier to debug problems with the stack walking
			OPTION_CHANGED_ALWAYS = 1 << 2,		// consider all function entries to be "changed" to avoid flattening the stack (for debugging)
			OPTION_SINGLE_THREAD  = 1 << 3,		// only pay attention to the primary thread in the log, ignore the rest
			OPTION_NO_ZERO_ALLOCS = 1 << 4,		// disallow zero allocs - always make them at least one byte
			OPTION_REMOVE_ALLOCS  = 1 << 5,		// remove allocs that match deallocs found later in the stream

			OPTION_NONE = 0,					// disable all options
		};

		LogFileReader( void );
	   ~LogFileReader( void );

		void SetOptions    ( eOptions options, bool set = true )	{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
		void ClearOptions  ( eOptions options )						{  SetOptions( options, false );  }
		void ToggleOptions ( eOptions options )						{  m_Options = (eOptions)(m_Options ^ options);  }
		bool TestOptions   ( eOptions options ) const				{  return ( (m_Options & options) != 0 );  }

		bool Open  ( const char* datFileName, const char* imageName = NULL );
		void Close ( void );
		bool IsOpen( void ) const									{  return ( m_File != INVALID_HANDLE_VALUE );  }

		void AddMemIgnoreMethod( const char* spec );

		bool LogToXml( ReportSys::ContextRef ctx = NULL );
		bool LogToXml( HANDLE xmlFile );

		bool LogToQProf( ReportSys::ContextRef ctx = NULL );
		bool LogToQProf( HANDLE qprofFile );

		bool LogToQuantify( void );

	private:

		typedef CBFunctor2 <const Sample* /*sampleBegin*/, const Sample* /*sampleEnd*/> ProcessCb;

		// commands
		bool process     ( SimpleWriter* writer );
		bool decode      ( ProcessCb cb );
		void destroyState( void );

		// precaching
		void precache( const Sample* sampleBegin, const Sample* sampleEnd );

		// logging
		void log           ( const Sample* sampleBegin, const Sample* sampleEnd );
		void exitStack     ( Stack& stack, DWORD untilCallerMethodId );
		void registerMethod( 
				ThreadEntry*  threadEntry, bool threadEntryNew,
				ClassEntry *  classEntry , bool classEntryNew ,
				MethodEntry*  methodEntry, bool methodEntryNew );

		// spec
		eOptions         m_Options;			// current options
		LogFileHeader    m_Header;			// header for file
		HANDLE           m_File;			// file we're reading from
		my LogDb*        m_LogDb;			// symbol database for scanning sample data
		my MemIgnoreDb*  m_MemIgnoreDb;		// db of methods to ignore when doing memory analysis (for more accurate and direct placement)

		// state
		SimpleWriter*    m_Writer;			// current writer
		my StackDb*      m_StackDb;			// thread/stack database for logging
		my MemDeallocDb* m_MemDeallocDb;	// db of deallocs to be removed from the incoming stream

		SET_NO_COPYING( LogFileReader );
};

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace QMem

//////////////////////////////////////////////////////////////////////////////
// class QMemApi declaration

#if GP_PROFILING

class QMemApi : public AutoSingleton <QMemApi>
{
public:
	SET_NO_INHERITED( QMemApi );

	QMemApi( void );
   ~QMemApi( void )						{  }

	// called by main loop
	void AddFrame( void );

	// called by UI functions (set frameCount == -1 for infinite)
FEX	void BeginProfile( int frameCount = 1 );
FEX	void StopProfile ( void );

	// meant for a function binder
FEX	bool Profile1  ( void )				{  BeginProfile( 1   );  return ( true );  }
FEX	bool Profile10 ( void )				{  BeginProfile( 10  );  return ( true );  }
FEX	bool Profile100( void )				{  BeginProfile( 100 );  return ( true );  }

private:
	bool                m_Profiling;
	int                 m_FrameCount;
	QMem::LogFileWriter m_Writer;

	FUBI_SINGLETON_CLASS( QMemApi, "Interface for QMem." );
	SET_NO_COPYING( QMemApi );
};

#define gQMemApi QMemApi::GetSingleton()

#endif // GP_PROFILING

//////////////////////////////////////////////////////////////////////////////

#endif // GP_ENABLE_STATS

#endif  // __QMEM_H

//////////////////////////////////////////////////////////////////////////////
