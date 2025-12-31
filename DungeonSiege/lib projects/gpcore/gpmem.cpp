//////////////////////////////////////////////////////////////////////////////
//
// File     :  gpmem.cpp
// Author(s):  Scott Bilas
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "GpMem.h"

#include "DebugHelp.h"
#include "GpStatsDefs.h"
#include "QMem.h"
#include "StdHelp.h"

//////////////////////////////////////////////////////////////////////////////
// SmartHeap (Release/Retail)

#if !GPMEM_DBG_NEW

#include "smrtheap.h"

#endif // !GPMEM_DBG_NEW

//////////////////////////////////////////////////////////////////////////////
// gpmem function implementations (all)

namespace gpmem  {  // begin of namespace gpmem

	
BYTE *	vaAlloc( DWORD size )
{
#	ifdef PROTECTED_BUFFERS

	size = ( size + 0x1000 ) & 0xfffff000;
	LPVOID buffer = ::VirtualAlloc( NULL, size , MEM_COMMIT, PAGE_READWRITE );
//	gpgenericf(( "va:allocating 0x%08x bytes @ 0x%08x\n", size, buffer ));
	if( !buffer )
	{
//		DWORD ec = ::GetLastError();
		gpassert( 0 );
	}
	return (BYTE*)buffer;

#	else

	return new BYTE[ size ];

#	endif
}


void vaFree( BYTE * buffer, DWORD size )
{
#	ifdef PROTECTED_BUFFERS
	size = ( size + 0x1000 ) & 0xfffff000;
//	gpgenericf(( "va:freeing 0x%08x bytes @ 0x%08x\n", size, buffer ));
	BOOL success = ::VirtualFree( buffer, size, MEM_DECOMMIT );
	if( !success )
	{
//		DWORD ec = ::GetLastError();
		gpassert( success );
	}
#	else
	return delete buffer;
#	endif
}


void vaProtect( BYTE * ptr, DWORD size )
{
#	ifdef PROTECTED_BUFFERS

	DWORD oldMode;
	size = ( size + 0x1000 ) & 0xfffff000;
//	gpgenericf(( "va:protecting 0x%08x bytes @ 0x%08x\n", size, ptr ));
	BOOL success = ::VirtualProtect( (LPVOID)ptr, size, PAGE_READONLY, &oldMode );
	if( !success )
	{
//		DWORD ec = ::GetLastError();
		gpassert( success );
	}
#	else

	// do nothing

#	endif
}


void vaUnprotect( BYTE * ptr, DWORD size )
{
#	ifdef PROTECTED_BUFFERS

	DWORD oldMode;
	size = ( size + 0x1000 ) & 0xfffff000;
//	gpgenericf(( "va:un-protecting 0x%08x bytes @ 0x%08x\n", size, ptr ));
	BOOL success = ::VirtualProtect( (LPVOID)ptr, size, PAGE_READWRITE, &oldMode );
	if( !success )
	{
//		DWORD ec = ::GetLastError();
		gpassert( success );
	}

#	else

	// do nothing

#	endif
}


void GetStatusSummary( gpstring& status )
{
	PROCESS_MEMORY_COUNTERS counters;
	counters.cb = sizeof( counters );
	if ( gPsApiDll.Load( false ) && gPsApiDll.GetProcessMemoryInfo( ::GetCurrentProcess(), &counters, sizeof( counters ) ) )
	{
		status.appendf(
			"WSet: %dK (Pk: %dK), VM: %dK (Pk: %dK), Faults: %d",
			counters.WorkingSetSize >> 10, counters.PeakWorkingSetSize >> 10,
			counters.PagefileUsage >> 10, counters.PeakPagefileUsage >> 10,
			counters.PageFaultCount );
	}

	if ( !status.empty() )
	{
		status += ", ";
	}
	status.appendf(
			"PgSz = %d, AllocGran = %d\n",
			SysInfo::GetSystemPageSize(), SysInfo::GetSystemAllocationGranularity() );

#	if GPMEM_DBG_NEW

	int delta = GetDeltaSerialID();
	status.appendf(
		"Mem Allocs: %d (Delta %d, Spike %d, Waste %d%%), Alloc Size: %d, High Water: %d\n",
		GetNumAllocations(),
		delta,
		GetSpikeSerialID( 2000, delta ),
		(int)(GetMemWasteRatio() * 100.0f),
		GetSizeAllocations(),
		GetSizeHighWaterMark() );

#	endif // GP_MEM_DBG_NEW
}

}  // end of namespace gpmem

//////////////////////////////////////////////////////////////////////////////
// header placed when stats in use

#if GP_ENABLE_STATS

#pragma pack ( push, 1 )
struct MemHeader
{
	DWORD m_System;
	DWORD m_SerialId;
};
#pragma pack ( pop )

COMPILER_ASSERT( sizeof( LONG ) == sizeof( DWORD ) );
#define MemHeaderSize sizeof( MemHeader )

#else // GP_ENABLE_STATS

// it's important not to allocate extra memory if we're not going to need it so
// that the damage detection on free can run in debug and get accurate results.
#define MemHeaderSize 0

#endif // GP_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////
// class GpMemMgr declaration and implementation

// $$ note on this section - it is *highly* dependent upon the particular
//    implementation of the CRT's memory manager. if the internals change then
//    this set of functions will likely fail without warning. revisit this code
//    every service pack or upgrade to make sure it's still in sync.

#include "stringtool.h"

static int CalcWastePtr( const void* mem, int size )
{
	// search backwards from end for non-fill
	const void* found = ::memrchr_not( mem, 0xCD, size );
	if ( found == NULL )
	{
		// all waste!
		return ( size );
	}
	else
	{
		// return waste
		return ( size - (((const BYTE*)found + 1) - (const BYTE*)mem) );
	}
}

#if GPMEM_DBG_NEW

#include <set>
#include <string>

#define _CRTBLD
#include "crtsrc\mtdll.h"
#include "crtsrc\dbgint.h"

namespace gpmem  {  // begin of namespace gpmem

struct HeapLocker
{
	HeapLocker( void )  {  _mlock  ( _HEAP_LOCK );  }
   ~HeapLocker( void )  {  _munlock( _HEAP_LOCK );  }
};

inline const _CrtMemBlockHeader* HeaderFromPointer( const void* mem )
	{  return ( scast <const _CrtMemBlockHeader*> ( mem ) - 1 );  }

inline const void* PointerFromHeader( const _CrtMemBlockHeader* header )
	{  return ( header + 1 );  }

struct GpMemMgr
{
	struct MemStats
	{
		int   m_AllocCount;
		int   m_AllocBytes;
		int   m_HighWaterBytes;
		INT64 m_TotalBytesWasted;
		INT64 m_TotalBytesFreed;

		MemStats( void )
		{
			::ZeroObject( *this );
		}

		void Alloc( int bytes )
		{
			++m_AllocCount;
			m_AllocBytes += bytes;
			m_HighWaterBytes = max( m_HighWaterBytes, m_AllocBytes );
		}

		void Free( int bytes, int waste )
		{
			--m_AllocCount;
			m_AllocBytes -= bytes;
			m_TotalBytesWasted += waste;
			m_TotalBytesFreed += bytes;
		}

		void Realloc( int oldBytes, int newBytes, int waste )
		{
			m_AllocBytes += newBytes - oldBytes;
			m_HighWaterBytes = max( m_HighWaterBytes, m_AllocBytes );

			// treat all realloc's as always a full malloc/free for stats purposes
			m_TotalBytesWasted += waste;
			m_TotalBytesFreed += oldBytes;
		}
	};

	static MemStats m_BlockStats[ _MAX_BLOCKS ];
	static MemStats m_GlobalStats;
	static int      m_LastSerialID;

	GpMemMgr( void )
	{
		// lock other threads
		HeapLocker locker;

		// set up debug params
		SetDebugNormal();

		// initialize self with startup stats
		_CrtMemState state;
		_CrtMemCheckpoint( &state );
		for ( int i = 0 ; i < _MAX_BLOCKS ; ++i )
		{
			m_BlockStats[ i ].m_AllocCount     = state.lCounts[ i ];
			m_BlockStats[ i ].m_AllocBytes     = state.lSizes [ i ];
			m_BlockStats[ i ].m_HighWaterBytes = state.lSizes [ i ];

			m_GlobalStats.m_AllocCount += m_BlockStats[ i ].m_AllocCount;
			m_GlobalStats.m_AllocBytes += m_BlockStats[ i ].m_AllocBytes;
		}
		m_GlobalStats.m_HighWaterBytes = state.lHighWaterCount;

		// hook all allocations
		_CrtSetAllocHook( Hook );
	}

	static int CalcWaste( const void* userData )
	{
		return ( CalcWastePtr( userData, HeaderFromPointer( userData )->nDataSize ) );
	}

	static int Hook( int                    allocType,
					 void*                  userData,
					 size_t                 size,
					 int                    blockType,
					 long                   requestNumber,
					 const unsigned char* /*filename*/,
					 int                  /*lineNumber*/ )
	{
		MemStats& stats = m_BlockStats[ blockType ];

		switch ( allocType )
		{
			case ( _HOOK_ALLOC ):
			{
				// check for screwy memory and sanity
				gpassertf( !IsMemoryBadFood( (DWORD)size ), ( "Memory allocation attempted with bad food (0x%08X) for the size!", size ));
				gpassertf( size < (1024 * 1024 * 10), ( "Sanity check: a %d byte allocation is HUGE!", size ));

				// do the alloc
				stats.Alloc( size );
				m_GlobalStats.Alloc( size );
				m_LastSerialID = requestNumber;
			}
			break;

			case ( _HOOK_REALLOC ):
			{
				// check for screwy memory and sanity
				int oldSize = HeaderFromPointer( userData )->nDataSize;
				gpassertf( !IsMemoryBadFood( (DWORD)size ), ( "Memory reallocation (from %d) attempted with bad food (0x%08X) for the size!", oldSize, size ));
				gpassertf( size < (1024 * 1024 * 10), ( "Sanity check: a %d byte reallocation (from %d) is HUGE!", size, oldSize ));

				// check waste
				int waste = CalcWaste( userData );

				// update stats
				stats.Realloc( oldSize, size, waste );
				m_GlobalStats.Realloc( oldSize, size, waste );
			}
			break;

			case ( _HOOK_FREE ):
			{
				// check waste
				int waste = CalcWaste( userData );

				// update stats
				size = HeaderFromPointer( userData )->nDataSize;
				stats.Free( size, waste );
				m_GlobalStats.Free( size, waste );
			}
			break;
		}

		// never fail the alloc
		return ( 1 );
	}
};

// GpMemMgr statics
GpMemMgr::MemStats GpMemMgr::m_BlockStats[ _MAX_BLOCKS ];
GpMemMgr::MemStats GpMemMgr::m_GlobalStats;
int                GpMemMgr::m_LastSerialID = 0;

// our static instance of the memory manager
static GpMemMgr sMemMgr;

//////////////////////////////////////////////////////////////////////////////
// gpmem function implementations (debug)

int GetNumAllocations( void )
{
	return ( GpMemMgr::m_GlobalStats.m_AllocCount );
}

int GetSizeAllocations( void )
{
	return ( GpMemMgr::m_GlobalStats.m_AllocBytes );
}

int GetSizeHighWaterMark( void )
{
	return ( GpMemMgr::m_GlobalStats.m_HighWaterBytes );
}

float GetMemWasteRatio( void )
{
	if ( GpMemMgr::m_GlobalStats.m_TotalBytesFreed == 0 )
	{
		return ( 0 );
	}
	return ( (float)((double)GpMemMgr::m_GlobalStats.m_TotalBytesWasted / (double)GpMemMgr::m_GlobalStats.m_TotalBytesFreed) );
}

int GetLastSerialID( void )
{
	return ( GpMemMgr::m_LastSerialID );
}

int GetDeltaSerialID( bool reset )
{
	static int last = 0;
	int now = GetLastSerialID();
	int delta = now - last;

	if ( reset )
	{
		last = now;
	}

	return ( delta );
}

int GetSpikeSerialID( int holdMsec, int delta, bool reset )
{
	static DWORD lastTest = 0;
	static int lastSpike = 0;

	if ( reset )
	{
		lastTest = 0;
	}

	DWORD now = ::GetTickCount();
	if ( ((int)(now - lastTest) > holdMsec) || (delta > lastSpike) )
	{
		lastSpike = delta;
		lastTest = now;
	}

	return ( lastSpike );
}

// sample class for walking heap
struct Sample
{
	const void* m_Mem;
	const char* m_File;
	int         m_Line;
	int         m_Size;
	int         m_Waste;
	int         m_Count;

	// simple equality test
	bool operator == ( void* mem )
	{
		return ( m_Mem == mem );
	}

	// sort by location only
	struct LessByLocation
	{
		bool operator () ( const Sample& s1, const Sample& s2 )
		{
			return ( ::strcmp( s1.m_File, s2.m_File ) < 0 );
		}
	};

	// sort by size then by memory location
	struct MoreBySize
	{
		bool operator () ( const Sample& s1, const Sample& s2 )
		{
			return (   (s1.m_Size > s2.m_Size)
					|| (!(s2.m_Size > s1.m_Size) && (s1.m_Mem > s2.m_Mem )) );
		}
	};

	// sort by count then by memory location
	struct MoreByCount
	{
		bool operator () ( const Sample& s1, const Sample& s2 )
		{
			return (   (s1.m_Count > s2.m_Count)
					|| (!(s2.m_Count > s1.m_Count) && (s1.m_Mem > s2.m_Mem )) );
		}
	};

	// sort by waste total bytes then by memory location
	struct MoreByWasteSize
	{
		bool operator () ( const Sample& s1, const Sample& s2 )
		{
			return (   (s1.m_Waste > s2.m_Waste)
					|| (!(s2.m_Waste > s1.m_Waste) && (s1.m_Mem > s2.m_Mem )) );
		}
	};

	// sort by waste percentage of total then by memory location
	struct MoreByWasteRatio
	{
		bool operator () ( const Sample& s1, const Sample& s2 )
		{
			float r1 = (float)s1.m_Waste / s1.m_Size;
			float r2 = (float)s2.m_Waste / s2.m_Size;

			return (   (r1 > r2)
					|| (!(r2 > r1) && (s1.m_Mem > s2.m_Mem )) );
		}
	};
};

// helper to put all allocations into the set
template <typename SET>
void GetAllocations( SET& coll )
{
	// lock other threads
	HeapLocker locker;

	// this will let us get to the first memory block data (hacka hacka hacka)
	void* dummy = malloc( 1 );

	// this loop code is adapted from _CrtMemDumpAllObjectsSince in
	// dbgheap.c. now obviously it's BAD to do this but i don't have any
	// other way to access the memory blocks aside from doing a dump and
	// then parsing out the strings... note that adding objects to a set
	// while we're iterating over it is ok. the memory manager adds objects
	// to the front of the list, and we've already passed that when we
	// start so no big deal.
	Sample sample;
	for (   const _CrtMemBlockHeader* iter = HeaderFromPointer( (BYTE*)dummy - MemHeaderSize )
		  ; iter != NULL
		  ; iter = iter->pBlockHeaderNext )
	{
		if (   _BLOCK_TYPE(iter->nBlockUse) != _IGNORE_BLOCK
			&& _BLOCK_TYPE(iter->nBlockUse) != _FREE_BLOCK )
		{
			sample.m_Mem   = PointerFromHeader( iter );
			sample.m_Size  = iter->nDataSize;
			sample.m_Waste = GpMemMgr::CalcWaste( sample.m_Mem );
			sample.m_Count = 1;

			if ( iter->szFileName != NULL )
			{
				sample.m_File  = iter->szFileName;
				sample.m_Line  = iter->nLine;
			}
			else
			{
				sample.m_File  = "<unknown file>";
				sample.m_Line  = 0;
			}

			// insert
			std::pair <SET::iterator, bool> result = coll.insert( sample );
			if ( !result.second )
			{
				// it was already there - update values
				result.first->m_Size += sample.m_Size;
				result.first->m_Waste += sample.m_Waste;
				++result.first->m_Count;
			}
		}
	}

	// free up that temp block
	free( dummy );
}

// helper to summarize a set
template <typename SET>
void GetAllocationsSummary( SET& coll, gpstring& status, int outputLimit )
{
	// get allocations
	std::set <Sample, Sample::LessByLocation> byLocation;
	GetAllocations( byLocation );

	// resort items
	stdx::copy_all( byLocation, stdx::inserter( coll ) );

	// get some vars
	int count = 0;
	char tempstr[ 5000 ];

	// dump the summary info to the string
	SET::iterator i, begin = coll.begin(), end = coll.end();
	for ( i = begin ; (i != end) && (count < outputLimit) ; ++i, ++count )
	{
		const char* fileName = strrchr( i->m_File, '\\' );
		fileName = fileName ? ( fileName + 1 ) : i->m_File;

		int wastePct = 0;
		if ( i->m_Size > 0 )
		{
			wastePct = (int)((100.0f * i->m_Waste) / i->m_Size);
		}

		sprintf( tempstr, "%10d K in %6d allocs @ %2d%% waste from %-35s line %d\n",
			i->m_Size >> 10, i->m_Count, wastePct, fileName, i->m_Line );
		status += tempstr;
	}
}

void GetAllocationsSummary( gpstring& status, int outputlimit, eSortType sort )
{
	if ( sort == SORT_BYSIZE )
	{
		std::set <Sample, Sample::MoreBySize> bySize;
		GetAllocationsSummary( bySize, status, outputlimit );
	}
	else if ( sort == SORT_BYCOUNT )
	{
		std::set <Sample, Sample::MoreByCount> byCount;
		GetAllocationsSummary( byCount, status, outputlimit );
	}
	else if ( sort == SORT_BYWASTESIZE )
	{
		std::set <Sample, Sample::MoreByWasteSize> byCount;
		GetAllocationsSummary( byCount, status, outputlimit );
	}
	else if ( sort == SORT_BYWASTERATIO )
	{
		std::set <Sample, Sample::MoreByWasteRatio> byCount;
		GetAllocationsSummary( byCount, status, outputlimit );
	}
	else
	{
		gpassert( 0 );	// shouldn't get here!
	}
}

void SetDebugNormal()
{
	_CrtSetDbgFlag(  _CRTDBG_LEAK_CHECK_DF
				   | _CRTDBG_ALLOC_MEM_DF );
}

void SetDebugMax()
{
	_CrtSetDbgFlag(  _CRTDBG_LEAK_CHECK_DF
				   | _CRTDBG_ALLOC_MEM_DF
				   | _CRTDBG_CHECK_ALWAYS_DF );
}

void CheckAll()
{
	_CrtCheckMemory();
}

void TrackDebugAlloc( int size, bool newAlloc )
{
	HeapLocker locker;

	if ( newAlloc )
	{
		sMemMgr.m_GlobalStats.Alloc( size );
	}
	else
	{
		sMemMgr.m_GlobalStats.Realloc( 0, size, 0 );
	}
}

void TrackDebugFree( int size, bool lastFree )
{
	HeapLocker locker;

	if ( lastFree )
	{
		sMemMgr.m_GlobalStats.Free( size, 0 );
	}
	else
	{
		sMemMgr.m_GlobalStats.Realloc( size, 0, 0 );
	}
}

}  // end of namespace gpmem

#endif // GPMEM_DBG_NEW

//////////////////////////////////////////////////////////////////////////////
// Memory alloc redirection

#include "gpmem_new_off.h"

#if (GP_DEBUG && !GP_SIEGEMAX)
void* realloc_hack( void* mem, size_t bytes, const char* file, int line )
{
	// $ important note: this special handling of debug-only realloc is
	//   necessary because we're still using sp3 crt libraries in debug mode
	//   to work around the slow small alloc problem in win2k (even with sp2
	//   applied). win2k sp2 fixed the slow small alloc deletion problem but
	//   allocs are very slow so in debug mode we need to use vc++'s fast sbh
	//   (small block heap) allocator. unfortunately post sp3 this feature is
	//   disabled on win2k, so in order to keep it we have to use sp3 libraries.
	//   but this introduces a new problem - realloc() on sbh has bugs in it
	//   that were fixed for sp4 of vc++. so for debug builds, we just won't
	//   use realloc and emulate it instead, dig?

	gpassert( bytes > 0 );

	// alloc new
	void* newMem = ::_malloc_dbg( bytes, _NORMAL_BLOCK, file, line );

	// if old, copy over and free
	if ( mem != NULL )
	{
		// get old size
		size_t oldSize = _msize_dbg( mem, _NORMAL_BLOCK );
		gpassert( oldSize != 0 );

		// copy over mem
		::memcpy( newMem, mem, min( oldSize, bytes ) );

		// free old mem
		::_free_dbg( mem, _NORMAL_BLOCK );
	}

	// done
	return ( newMem );
}
#endif // GP_DEBUG

#if GP_ENABLE_STATS

static DWORD s_MemSerialId = 0;

static void* gpalloc_track( void* mem, size_t bytes )
{
	if ( mem )
	{
		// get our vars
		eSystemProperty system = gpstats::GetCurrentSystem();
		DWORD serialId = ::InterlockedIncrement( (LONG*)&s_MemSerialId );

		// log the events
		GPSTATS_SAMPLE( AddMemAlloc( system, bytes ) );
#		if GP_PROFILING
		QMem::AddMemAlloc( serialId, bytes );
#		endif // GP_PROFILING

		// configure header
		MemHeader* header = (MemHeader*)mem;
		header->m_System = system;
		header->m_SerialId = serialId;

		// reset memory to point to actual post-header data
		mem = header + 1;
	}

	// done
	return ( mem );
}

static size_t gpfree_track( void*& mem )
{
	// find the header
	MemHeader* header = (MemHeader*)mem - 1;

	// get our vars
	eSystemProperty system = (eSystemProperty)header->m_System;
#	if GP_DEBUG
	size_t bytes = _msize_dbg( header, _NORMAL_BLOCK ) - sizeof( MemHeader );
#	else // GP_DEBUG
	size_t bytes = ::MemSizePtr( header ) - sizeof( MemHeader );
#	endif // GP_DEBUG

	// log the events
	GPSTATS_SAMPLE( AddMemDealloc( system, bytes, CalcWastePtr( mem, bytes ) ) );
#	if GP_PROFILING
	QMem::AddMemDealloc( header->m_SerialId, bytes );
#	endif // GP_PROFILING

	// reset memory to point to memory at header
	mem = header;

	// done
	return ( bytes );
}

#else // GP_ENABLE_STATS

static void* gpalloc_track( void* mem, size_t )  {  return ( mem );  }
static void gpfree_track( void* )  {  }

#endif // GP_ENABLE_STATS

#if (GPMEM_DBG_NEW && !GP_SIEGEMAX)

// $$ future - use _CLIENT_BLOCK and then hook the leak dump to include the
//    system field so we can get more context on those "unknown" ones.

void* gpmalloc( size_t bytes, const char* file, int line )
{
	return ( gpalloc_track( ::_malloc_dbg( bytes + MemHeaderSize, _NORMAL_BLOCK, file, line ), bytes ) );
}

void* gprealloc( void* mem, size_t bytes, const char* file, int line )
{
	if ( mem )
	{
		gpfree_track( mem );
		return ( gpalloc_track( ::realloc_hack( mem, bytes + MemHeaderSize, file, line ), bytes ) );
	}
	else
	{
		return ( gpmalloc( bytes ) );
	}
}

void gpfree( void* mem )
{
	if ( mem )
	{
		gpfree_track( mem );
		::_free_dbg( mem, _NORMAL_BLOCK );
	}
}

#elif (GP_DEBUG && !GP_SIEGEMAX)

void* gprealloc( void* mem, size_t bytes )
{
	return ( realloc_hack( mem, bytes, __FILE__, __LINE__ ) );
}

#elif GP_ENABLE_STATS

void* gpmalloc( size_t bytes )
{
	void* mem = gpalloc_track( ::malloc( bytes + sizeof( MemHeader ) ), bytes );
	if ( mem )
	{
		::memset( mem, 0xCD, bytes );
	}
	return ( mem );
}

void* gprealloc( void* mem, size_t bytes )
{
	if ( mem )
	{
		size_t oldSize = gpfree_track( mem );
		void* newMem = ::realloc( mem, bytes + sizeof( MemHeader ) );
		newMem = gpalloc_track( newMem, bytes );
		if ( oldSize < bytes )
		{
			::memset( (BYTE*)newMem + oldSize, 0xCD, bytes - oldSize );
		}
		return ( newMem );
	}
	else
	{
		return ( gpmalloc( bytes ) );
	}
}

void gpfree( void* mem )
{
	if ( mem )
	{
		gpfree_track( mem );
		::MemFreePtr( mem );
	}
}

#elif (GP_DEBUG && GP_SIEGEMAX)

// Need to define 'gprealloc' --biddle
void* gprealloc( void* mem, size_t bytes )
{
	if ( mem )
	{
		void* newMem = ::realloc( mem, bytes );
		return ( newMem );
	}
	else
	{
		return ( gpmalloc( bytes ) );
	}
}

#endif

//////////////////////////////////////////////////////////////////////////////

// this forces gpmem.cpp to be linked in. do not remove or alter this!
void gpmem_ForceLink( void )  {  }

//////////////////////////////////////////////////////////////////////////////
