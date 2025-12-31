//////////////////////////////////////////////////////////////////////////////
//
// File     :  FileSysDiyMap.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "FileSysDiyMap.h"

#include "FileSysXfer.h"
#include "GpStatsDefs.h"
#include "StringTool.h"

namespace FileSys  {  // begin of namespace FileSys

#if !GP_RETAIL
static const int FILL_MAGIC = 0x0DF0ADBA;	// magic number for bounds checking
static const int FILL_SIZE  = 16;			// minimum length of fill to use for bounds checking
#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class DiyFileMap implementation

DiyFileMap DiyFileMap::ms_Root;
bool DiyFileMap::ms_RegisteredHandler = false;
kerneltool::Critical DiyFileMap::ms_ListCritical;

DiyFileMap :: DiyFileMap( void )
{
	PrivateInit();

	if ( this == &ms_Root )
	{
		m_Next = m_Prev = this;
	}
	else if ( !ms_RegisteredHandler )
	{
		::RegisterGlobalExceptionFilter( ValidateMemorySehFilter );
		ms_RegisteredHandler = true;
	}

#	if GP_ENABLE_STATS
	m_OwningSystem = SP_NONE;
#	endif // GP_ENABLE_STATS
}

DiyFileMap :: ~DiyFileMap( void )
{
	// clean up
	Close();

	// sanity checks
	gpassert( (this != &ms_Root) || ((m_Next == this) && (m_Prev == this)) );
	gpassert( (this == &ms_Root) || ((m_Next == NULL) && (m_Next == NULL)) );

	// unhook
	if ( (this == &ms_Root) && ms_RegisteredHandler )
	{
		::UnregisterGlobalExceptionFilter( ValidateMemorySehFilter );
		ms_RegisteredHandler = false;
	}
}

bool DiyFileMap :: Create( eOptions options, eAccess access, Reader* takeReader, DWORD size, DWORD chunkSize )
{
	// sanity checks
	gpassert( this != &ms_Root );
	gpassertm( size < (50 * 1024 * 1024), "Sanity check! Huge DIY allocation!!" );
	gpassert( takeReader != NULL );

	// kill old map
	Close();

	// init options
	m_Options   = options;
	m_Access    = access;
	m_Size      = size;
	m_ChunkSize = chunkSize;
	m_Reader    = takeReader;

	// preprocess chunk
	if ( m_ChunkSize >= size )
	{
		m_ChunkSize = 0;
	}
	else
	{
		gpassert( ::IsAligned( chunkSize, SysInfo::GetSystemPageSize() ) );
	}

	// special debugger-friendly options
	if ( SysInfo::IsDebuggerPresent() )
	{
		if ( SysInfo::IsWin9x() )
		{
			// well to avoid problems with the debugger complaining about
			// access violations, just avoid using paging entirely. not pretty
			// and not fast, but it's best we can do. otherwise you'd have to
			// turn off access violation first chance testing in the debugger
			// and miss all kinds of other stuff. an alternative would be for
			// this code to throw a different special user exception that could
			// be caught in the debugger upon getting an access violation that
			// is not due to page fault on this memory.
			m_Options |= OPTION_NO_PAGING;
		}
		else
		{
			// the debugger in win2k will print out first chance exception
			// messages each time, which is annoying, but at least we don't lose
			// a huge amount of performance. we still don't want to use guard
			// pages full time for win2k because it takes more memory - guard
			// pages don't work on MEM_RESERVE, so we have to use MEM_COMMIT
			// instead, which allocates physical space.
			m_Options |= OPTION_USE_GUARD_PAGE;
		}
	}

	// always check bounds in debug mode
#	if GP_DEBUG
	m_Options |= OPTION_CHECK_BOUNDS;
#	endif // GP_DEBUG

	// if chunking not supported, then don't do paging method
	if ( m_ChunkSize == 0 )
	{
		m_Options |= OPTION_NO_PAGING;
	}

	// if no virtual alloc allowed, that's the same as no paging
	if ( m_Options & OPTION_NO_VIRTUALALLOC )
	{
		m_Options |= OPTION_NO_PAGING;
	}

	// get allocation size
	int prefillSize = 0, postfillSize = 0;
	GetFillSizes( prefillSize, postfillSize );
	int allocSize = m_Size + prefillSize + postfillSize;

	// alloc memory
	if ( m_Options & OPTION_NO_VIRTUALALLOC )
	{
		// simple alloc
		m_Base = new BYTE[ allocSize ];

		// fill badfood
#		if !GP_RETAIL
		if ( m_Options & OPTION_CHECK_BOUNDS )
		{
			FillBadFood( m_Base, prefillSize );
			m_Base += prefillSize;
			FillBadFood( m_Base + m_Size, postfillSize );
		}
#		endif // !GP_RETAIL
	}
	else
	{
		bool useGuard = !SysInfo::IsWin9x() && (m_Options & OPTION_USE_GUARD_PAGE);

		// valloc with the proper protection type
		DWORD originalProtection = useGuard ? (PAGE_GUARD | PAGE_READONLY) : PAGE_NOACCESS;
		m_Base = (BYTE*)::VirtualAlloc(
				NULL,
				allocSize,
				useGuard ? MEM_COMMIT : MEM_RESERVE,
				originalProtection );
		if ( !m_Base )
		{
			gpwatsonf(( "Critical error: VirtualAlloc( %d, %d ) failed with error '%s'!\nSorry but must go sleep now.\n",
						size, chunkSize, stringtool::GetLastErrorText().c_str() ));
		}
		gpmem::TrackDebugAlloc( 0, true );

		// track stats
#		if GP_ENABLE_STATS
		m_OwningSystem = gpstats::GetCurrentSystem();
		GPSTATS_SAMPLE( AddDiyReserve( m_OwningSystem, m_Size ) );
#		endif // GP_ENABLE_STATS

		// fill uninitialized
#		if GP_DEBUG
		if ( useGuard )
		{
			DWORD oldProtection;
			gpverify( ::VirtualProtect( m_Base, allocSize, PAGE_READWRITE, &oldProtection ) );
			::memset( m_Base, 0xCD, allocSize );
			gpverify( ::VirtualProtect( m_Base, allocSize, originalProtection, &oldProtection ) );
		}
#		endif // GP_DEBUG

		// fill badfood
#		if !GP_RETAIL
		if ( m_Options & OPTION_CHECK_BOUNDS )
		{
			FillBadFoodPage( m_Base );
			m_Base += prefillSize;
			FillBadFoodPage( m_Base + m_Size, originalProtection );
			FillBadFoodPage( m_Base + m_Size + postfillSize - SysInfo::GetSystemPageSize() );
		}
#		endif // !GP_RETAIL
	}

	// pre-fill?
	if ( m_Options & OPTION_NO_PAGING )
	{
		if ( !PrepMemory( 0, 0 ) )
		{
			Close();
			return ( false );
		}
	}

	// add to list
	{
		Critical::Lock locker( ms_ListCritical );

		gpassert( m_Next == NULL );
		gpassert( m_Prev == NULL );
		m_Next = ms_Root.m_Next;
		m_Prev = &ms_Root;
		m_Next->m_Prev = this;
		m_Prev->m_Next = this;
	}

	// done
	return ( true );
}

bool DiyFileMap :: Create( eOptions options, eAccess access, HANDLE file, DWORD offset, DWORD size )
{
	std::auto_ptr <FileReader> reader( new FileReader( file, offset, size ) );
	if ( reader->GetSize() == INVALID_SIZE_VALUE )
	{
		return ( false );
	}

	return ( Create( options, access, reader.release(), size, SysInfo::GetSystemPageSize() ) );
}

bool DiyFileMap :: Close( void )
{
	bool wasCreated = IsCreated();
	if ( wasCreated )
	{
		// paranoia check
#		if !GP_RETAIL
		CheckBounds();
#		endif // !GP_RETAIL

		int prefillSize = 0, postfillSize = 0;
		GetFillSizes( prefillSize, postfillSize );
		BYTE* base = m_Base - prefillSize;

		// free the memory
		if ( m_Options & OPTION_NO_VIRTUALALLOC )
		{
			delete [] ( base );
		}
		else
		{
#			if GPMEM_DBG_NEW

			int memFreed = 0;
			int allocSize = m_Size + prefillSize + postfillSize;

			MEMORY_BASIC_INFORMATION mbi;
			for ( BYTE* localBase = base, * end = localBase + allocSize ; localBase < end ; localBase += mbi.RegionSize )
			{
				gpverify( ::VirtualQuery( localBase, &mbi, sizeof( mbi ) ) == sizeof( mbi ) );
				if ( mbi.State == MEM_COMMIT )
				{
					memFreed += mbi.RegionSize;
				}
			}
			gpmem::TrackDebugFree( memFreed, true );

#			endif // GPMEM_DBG_NEW

			gpverify( ::VirtualFree( base, 0, MEM_RELEASE ) );
			GPSTATS_SAMPLE( AddDiyFree( m_OwningSystem, m_Size ) );
		}

		// don't need the reader
		Delete( m_Reader );

		// remove from list
		{
			Critical::Lock locker( ms_ListCritical );

			gpassert( (m_Next != NULL) && (m_Prev != NULL) );
			m_Next->m_Prev = m_Prev;
			m_Prev->m_Next = m_Next;
		}

		// reset vars
		PrivateInit();
	}
	return ( wasCreated );
}

void DiyFileMap :: Touch( const void* base, int size )
{
	// handle def values
	if ( base == NULL )
	{
		base = m_Base;
	}
	if ( size == -1 )
	{
		size = m_Size - ((const BYTE*)base - m_Base);
	}

	// $$$ loop through the chunks in this map, prepping each as they are found
	//     in the decoded bitfield (yet to be added)

	__try
	{
		::MemTouchPages( base, size, true );
	}
	__except( ::GlobalExceptionFilter( GetExceptionInformation(), EXCEPTION_CONTINUE_SEARCH ) )
	{
		// this space intentionally left blank...
	}
}

#if !GP_RETAIL

bool DiyFileMap :: CheckBounds( void )
{
	bool ok = true;

	if ( m_Options & OPTION_CHECK_BOUNDS )
	{
		int prefillSize = 0, postfillSize = 0;
		GetFillSizes( prefillSize, postfillSize );

		if ( m_Options & OPTION_NO_VIRTUALALLOC )
		{
			// easy check
			CheckBadFood( m_Base - prefillSize, prefillSize );
			CheckBadFood( m_Base + m_Size, postfillSize );
		}
		else
		{
			// check first, last, and second to last pages
			CheckBadFoodPage( m_Base - prefillSize );
			CheckBadFoodPage( m_Base + m_Size );
			CheckBadFoodPage( m_Base + m_Size + postfillSize - SysInfo::GetSystemPageSize() );
		}
	}

	return ( ok );
}

#endif // !GP_RETAIL

void DiyFileMap :: PrivateInit( void )
{
	m_Options = OPTION_NONE;
	m_Access  = ACCESS_READ_ONLY;
	m_Base    = NULL;
	m_Size    = 0;
	m_Reader  = NULL;
	m_Next    = NULL;
	m_Prev    = NULL;
}

DWORD DiyFileMap :: GetVirtualAllocType( void ) const
{
	if ( m_Access == ACCESS_READ_ONLY )
	{
		return ( PAGE_READONLY );
	}
	else
	{
		gpassert( m_Access == ACCESS_READ_WRITE );
		return ( PAGE_READWRITE );
	}
}

void DiyFileMap :: GetFillSizes( int& prefillSize, int& postfillSize )
{
	prefillSize = 0;
	postfillSize = 0;

#	if !GP_RETAIL
	if ( m_Options & OPTION_CHECK_BOUNDS )
	{
		if ( m_Options & OPTION_NO_VIRTUALALLOC )
		{
			// just the fill for new'd memory
			prefillSize = FILL_SIZE;
			postfillSize = FILL_SIZE + OVERRUN_ALLOW;
		}
		else
		{
			int pageSize = SysInfo::GetSystemPageSize();

			// an entire page for va'd memory
			prefillSize = pageSize;

			// fill the remainder of the last page plus the next page
			postfillSize = ::GetAlignUp( m_Size + OVERRUN_ALLOW, pageSize ) - m_Size + pageSize;
		}
	}
	else
#	endif // !GP_RETAIL
	{
		postfillSize = OVERRUN_ALLOW;
	}
}

bool DiyFileMap :: PrepMemory( int offset, int size )
{
	gpassert( IsCreated() );
	gpassert( (offset >= 0) && (offset < (int)m_Size) );
	gpassert( (size >= 0) && ((size + offset) <= (int)m_Size) );
	gpassert( (m_ChunkSize == 0) || ((::IsAligned( size, m_ChunkSize ) || ((offset + size) == (int)m_Size)) && ::IsAligned( offset, m_ChunkSize)) );

	// lock for work
	m_Critical.Enter();

	// resolve size
	if ( size == 0 )
	{
		size = m_Size;
	}

	// are we going out to overrun land?
	bool overrun = (offset + size) == (int)m_Size;
	int allocSize = size + (overrun ? OVERRUN_ALLOW : 0);

	// resolve memory
	BYTE* mem = m_Base + offset;

	// first commit the memory
	if ( !(m_Options & OPTION_NO_VIRTUALALLOC) )
	{
		gpverify( ::VirtualAlloc( mem, allocSize, MEM_COMMIT, PAGE_READWRITE ) == mem );
		gpmem::TrackDebugAlloc( allocSize, false );
		GPSTATS_SAMPLE( AddDiyCommit( allocSize ) );

		// the memory may have already been alloc'd, in which case win9x requires
		// an additional request to unprotect it, sheesh.
		DWORD oldProtection;
		gpverify( ::VirtualProtect( mem, allocSize, PAGE_READWRITE, &oldProtection ) );
	}

	// read into there
	bool success = true;
	__try
	{
		if ( !m_Reader->Seek( offset ) || !m_Reader->Read( mem, size ) )
		{
			success = false;
		}
	}
	__except( ValidateMemorySehFilter( GetExceptionInformation() ) )		// may be nested exception caused by accesses to other diy maps inside reader
	{
		success = false;
	}

	// on success do some extra stuff
	if ( success )
	{
		// reset the bad food if we went straight through to the end
#		if !GP_RETAIL
		if ( (m_Options & OPTION_CHECK_BOUNDS) && overrun )
		{
			FillBadFood( mem + size, OVERRUN_ALLOW );
		}
#		endif // !GP_RETAIL

		// now reprotect the memory
		if ( !(m_Options & OPTION_NO_VIRTUALALLOC) )
		{
			DWORD oldProtection;
			gpverify( ::VirtualProtect( mem, allocSize, GetVirtualAllocType(), &oldProtection ) );
		}

		// check everything ok
#		if !GP_RETAIL
		CheckBounds();
#		endif // !GP_RETAIL
	}

	// unlock now that we're done
	m_Critical.Leave();

	// done
	return ( true );
}

bool DiyFileMap :: PageFault( void* address )
{
	// paranoia checks
	gpassert( !(m_Options & OPTION_NO_VIRTUALALLOC) );
	gpassert( !(m_Options & OPTION_NO_PAGING) );
	gpassert( m_ChunkSize != 0 );

	// round down to chunk size and clamp upper range to end
	int offset = GetAlignDown( (BYTE*)address - m_Base, m_ChunkSize );
	int size = min_t( m_ChunkSize, m_Size - offset );

	// fill the memory
	return ( PrepMemory( offset, size ) );
}

#if !GP_RETAIL

void DiyFileMap :: FillBadFood( void* base, int size )
{
	// fill leading bytes
	BYTE* localBase = (BYTE*)GetDwordAlignUp( (int)base );
	int leading = localBase - (BYTE*)base;
	if ( leading )
	{
		::memcpy( base, (const BYTE*)&FILL_MAGIC + 4 - leading, leading );
		size -= leading;
	}

	// fill dwords
	std::fill( (DWORD*)localBase, (DWORD*)localBase + (size >> 2), FILL_MAGIC );

	// fill remaining bytes
	int trailing = size & 3;
	localBase += size - trailing;
	::memcpy( localBase, &FILL_MAGIC, trailing );
}

void DiyFileMap :: FillBadFoodPage( void* base, DWORD protect )
{
	int fillSize = (BYTE*)GetAlignUp( base, SysInfo::GetSystemPageSize() ) - (BYTE*)base;
	if ( fillSize == 0 )
	{
		fillSize = SysInfo::GetSystemPageSize();
	}

	// first commit the page to allow writing bad food
	LPVOID page = ::VirtualAlloc( base, fillSize, MEM_COMMIT, PAGE_READWRITE );
	gpassert( page != NULL );
	gpmem::TrackDebugAlloc( SysInfo::GetSystemPageSize(), false );

	// fill with bad food
	FillBadFood( base, fillSize );

	// now protect it fully
	DWORD oldProtection;
	gpverify( ::VirtualProtect( page, fillSize, protect, &oldProtection ) );
}

bool DiyFileMap :: CheckBadFood( void* base, int size )
{
	// check leading bytes
	BYTE* localBase = (BYTE*)GetDwordAlignUp( base );
	int leading = localBase - (BYTE*)base;
	if ( leading )
	{
		if ( ::memcmp( base, (const BYTE*)&FILL_MAGIC + 4 - leading, leading ) != 0 )
		{
			return ( false );
		}
		size -= leading;
	}

	// fill dwords
	for ( BYTE* end = localBase + (size & ~3) ; localBase != end ; localBase += 4 )
	{
		if ( *(DWORD*)localBase != FILL_MAGIC )
		{
			return ( false );
		}
	}

	// fill remaining bytes
	int trailing = size & 3;
	if ( ::memcmp( localBase, (const BYTE*)&FILL_MAGIC, trailing ) != 0 )
	{
		return ( false );
	}

	// done
	return ( true );
}

bool DiyFileMap :: CheckBadFoodPage( void* base )
{
	int fillSize = (BYTE*)GetAlignUp( base, SysInfo::GetSystemPageSize() ) - (BYTE*)base;
	if ( fillSize == 0 )
	{
		fillSize = SysInfo::GetSystemPageSize();
	}

	// first unprotect the page to allow reading bad food
	DWORD oldProtection;
	gpverify( ::VirtualProtect( base, fillSize, PAGE_READONLY, &oldProtection ) );

	// check bad food
	bool check = CheckBadFood( base, fillSize );

	// reprotect it
	gpverify( ::VirtualProtect( base, fillSize, oldProtection, &oldProtection ) );

	// done
	return ( check );
}

#endif // !GP_RETAIL

long DiyFileMap :: ValidateMemorySehFilter( EXCEPTION_POINTERS* xinfo )
{
	// make sure it's something we're interested in
	if (   (xinfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
		|| (xinfo->ExceptionRecord->ExceptionCode == EXCEPTION_GUARD_PAGE) )
	{
		// get the address of the violation
		DWORD address = xinfo->ExceptionRecord->ExceptionInformation[ 1 ];

		// find our map
		DiyFileMap* map = NULL;
		{
			Critical::Lock locker( ms_ListCritical );
			for ( DiyFileMap* i = ms_Root.m_Next ; i != &ms_Root ; i = i->m_Next )
			{
				// check to see if address falls in this range
				if ( (address >= (DWORD)i->m_Base) && (address < (DWORD)(i->m_Base + i->m_Size)) )
				{
					map = i;
					break;
				}
			}
		}

		// got it?
		if ( map != NULL )
		{
			// make sure we're not writing when it's read-only
			if ( (xinfo->ExceptionRecord->ExceptionInformation[ 0 ] == 0) || (map->m_Access == ACCESS_READ_WRITE) )
			{
				// do it
				if ( map->PageFault( (void*)address ) )
				{
					return ( EXCEPTION_CONTINUE_EXECUTION );
				}
			}
		}
	}

	// nevermind
	return ( EXCEPTION_CONTINUE_SEARCH );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
