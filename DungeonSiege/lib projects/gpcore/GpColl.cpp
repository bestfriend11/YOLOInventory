//////////////////////////////////////////////////////////////////////////////
//
// File     :  GpColl.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "GpColl.h"

#if !GP_DEBUG
#include "SmrtHeap.h"
#endif // !GP_DEBUG

namespace stdx  {  // begin of namespace stdx

//////////////////////////////////////////////////////////////////////////////
// class st_pool_allocator implementation

#if !GP_DEBUG

st_pool_allocator :: st_pool_allocator( void )
{
	DWORD currentThreadId = ::GetCurrentThreadId();

	// find a pool
	PoolInfo* i, * ibegin = ms_Pools, * iend = ARRAY_END( ms_Pools );
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->m_OwnerThreadId == currentThreadId )
		{
			// that's the one, take it
			m_AllocatorIndex = i - ibegin;
			break;
		}

		if ( i->m_OwnerThreadId == 0 )
		{
			// create pool
			MEM_POOL newPool = MemPoolInit( 0 );
			MEM_POOL_INFO defaultInfo;
			::MemPoolInfo( MemDefaultPool, NULL, &defaultInfo );
			::MemPoolSetPageSize( newPool, defaultInfo.pageSize );

			// found an empty slot, fill it
			m_AllocatorIndex = i - ibegin;
			i->m_OwnerThreadId = currentThreadId;
			i->m_PoolId = newPool;

			// must have valid pool	
			if ( i->m_PoolId == NULL )
			{
				gpwatson( "Unable to allocate a new memory pool!" );
			}

			break;
		}
	}

	// not found? bad!
	if ( i == iend )
	{
		gpwatson( "Out of memory pools!" );
	}
}

void* st_pool_allocator :: pool_alloc( size_t size )
{
	return ( MemAllocPtr( (MEM_POOL)ms_Pools[ m_AllocatorIndex ].m_PoolId, size, 0 ) );
}

void* st_pool_allocator :: pool_realloc( void* mem, size_t size )
{
	if ( mem == NULL )
	{
		return ( pool_alloc( size ) );
	}
	else
	{
		return ( MemReAllocPtr( mem, size, 0 ) );
	}
}

void st_pool_allocator :: pool_free( void* mem )
{
	MemFreePtr( mem );
}

st_pool_allocator::PoolInfo st_pool_allocator::ms_Pools[ st_pool_allocator::MAX_POOLS ];

#endif // !GP_DEBUG

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace stdx

//////////////////////////////////////////////////////////////////////////////
