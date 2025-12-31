/* ========================================================================
   Source File: siege_cache.cpp
   Derived From: soul_cache.cpp
   Defines: siege::cache_slot,
            siege::cache
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "precomp_siege.h"
#include "gpcore.h"
#include "siege_engine.h"
#include "siege_node.h"
#include "siege_node_io.h"
#include "siege_mesh.h"
#include "siege_mesh_io.h"
#include "siege_cache.h"
#include "siege_cache_handle.h"
#include "siege_database.h"
#include "siege_node_io.h"

using namespace siege;

// Thread checking macro
#	if GP_DEBUG
#	define CHECK_CACHE_PRIMARY_THREAD_ONLY \
	if ( m_bPrimaryThreadOnly && !gSiegeEngine.IsPrimaryThread() ) \
	{ \
		gperror( "This function can only be called from the primary thread!\n" ); \
	}
#	else // GP_DEBUG
#	define CHECK_CACHE_PRIMARY_THREAD_ONLY
#	endif // GP_DEBUG

/* ========================================================================
   Instantion Macro
   ======================================================================== */
#define SIEGE_CACHE_CPP(class_name) \
template cache_slot<class_name>; \
template cache<class_name>

template <class cached_object>
bool cache_slot<cached_object>::
Load()
{
	// Unload if necessary
	if( m_pCachedObject )
	{
		return true;
	}

	// Load
	m_pCachedObject =  new cached_object;
	
	if( m_pCachedObject )
	{
		if( !cached_object::io_handler::Read( *m_pCachedObject, m_GUID ) )
		{
			Unload();
			return false;
		}
	}

	return true;
}

template <class cached_object>
bool cache_slot<cached_object>::
Create()
{
	// Return if already created
	if( m_pCachedObject )
	{
		return true;
	}

	// Create
	m_pCachedObject =  new cached_object;

	// Attempt to load this slot, if it can be loaded.  Return value is ignored.
	cached_object::io_handler::Read( *m_pCachedObject, m_GUID, false );

	return true;
}

template <class cached_object>
cache<cached_object>::handle cache<cached_object>::
UseObject(database_guid const &GUID)
{
	CHECK_CACHE_PRIMARY_THREAD_ONLY;

	slot* Slot = NULL;

	slots::iterator s = Slots.find( GUID );
	if( s != Slots.end() )
	{
		Slot = (*s).second;
	}

	if(!Slot)
	{
		Slot = new slot(GUID);
		gpassert(Slot);
		Slots.insert( std::pair< database_guid, slot* >( GUID, Slot ) );
	}

	gpassert(Slot);
	return(handle(*Slot));
}

template <class cached_object>
const cache<cached_object>::handle cache<cached_object>::
UseObject(database_guid const &GUID) const
{
	CHECK_CACHE_PRIMARY_THREAD_ONLY;

	slot* Slot = NULL;

	slots::const_iterator s = Slots.find( GUID );
	if( s != Slots.end() )
	{
		Slot = (*s).second;
	}

	gpassert(Slot);
	return(handle(*Slot));
}

// Access
template <class cached_object>
cached_object& cache<cached_object>::
GetSlot( cache_slot<cached_object> *Slot )
{
	CHECK_CACHE_PRIMARY_THREAD_ONLY;

	cached_object* pObject	= Slot->GetCachedObject();
	if(!pObject)
	{
		if( !Slot->Load() )
		{
			// Return an empty object
			return(*m_pInvalidObject);
		}

		// Set our new object
		pObject		= Slot->GetCachedObject();
	}

	return(*pObject);
}

template <class cached_object>
cached_object const& cache<cached_object>::
GetSlot( cache_slot<cached_object> *Slot ) const
{
	CHECK_CACHE_PRIMARY_THREAD_ONLY;

	cached_object* pObject	= Slot->GetCachedObject();
	if(!pObject)
	{
		if( !Slot->Load() )
		{
			// Return an empty object
			return(*m_pInvalidObject);
		}

		// Set our new object
		pObject		= Slot->GetCachedObject();

	}

	return(*pObject);
}

template <class cached_object>
cached_object& cache<cached_object>::
CreateSlot( cache_slot<cached_object> *Slot )
{
	CHECK_CACHE_PRIMARY_THREAD_ONLY;

	cached_object* pObject	= Slot->GetCachedObject();
	if(!pObject)
	{
		gpverify( Slot->Create() );
		pObject		= Slot->GetCachedObject();
	}

	return(*pObject);
}

template <class cached_object>
cached_object const& cache<cached_object>::
CreateSlot( cache_slot<cached_object> *Slot ) const
{
	CHECK_CACHE_PRIMARY_THREAD_ONLY;

	cached_object* pObject	= Slot->GetCachedObject();
	if(!pObject)
	{
		gpverify( Slot->Create() );
		pObject		= Slot->GetCachedObject();

	}

	return(*pObject);
}

template <class cached_object>
bool cache<cached_object>::
ObjectExistsInCache(database_guid const &GUID) const
{
	CHECK_CACHE_PRIMARY_THREAD_ONLY;

	slots::const_iterator s = Slots.find( GUID );
	if( s != Slots.end() )
	{
		return true;
	}

	return false;
}

template <class cached_object>
cached_object* cache<cached_object>::
GetValidObjectInCache(database_guid const &GUID)
{
	slots::const_iterator s = Slots.find( GUID );
	if( s != Slots.end() )
	{
		return( (*s).second->GetCachedObject() );
	}

	return NULL;
}

// Remove an object from the cache and delete it
template <class cached_object>
bool cache<cached_object>::
RemoveObjectFromCache( database_guid const &GUID )
{
	CHECK_CACHE_PRIMARY_THREAD_ONLY;

	slots::const_iterator s = Slots.find( GUID );
	if( s != Slots.end() )
	{
		(*s).second->Unload();
		return true;
	}

	return false;
}

// Remove a slot from the cache
template <class cached_object>
bool cache<cached_object>::
RemoveSlotFromCache( database_guid const &GUID )
{
	CHECK_CACHE_PRIMARY_THREAD_ONLY;

	slots::iterator s = Slots.find( GUID );
	if( s != Slots.end() )
	{
		// Unload the slot to clean up any owned references
		(*s).second->Unload();

#if GP_DEBUG
		if( (*s).second->GetReferenceCount() != 0 )
		{
			char buf[512];
			sprintf( buf, "RemoveSlotFromCache() called on an object that is still referenced %d time(s)!", (*s).second->GetReferenceCount() );

			gpassertm( ((*s).second->GetReferenceCount() == 0), buf );
		}
#endif

		// Delete the slot and remove it from our slot mapping
		delete (*s).second;
		Slots.erase( s );
		return true;
	}

	return false;
}

// remove all slots from cache
template <class cached_object>
bool cache<cached_object>::
Clear()
{
	CHECK_CACHE_PRIMARY_THREAD_ONLY;

	slots::iterator s;
	for( s = Slots.begin(); s != Slots.end(); )
	{
		(*s).second->Unload();

#if GP_DEBUG
		if( (*s).second->GetReferenceCount() != 0 )
		{
			char buf[512];
			sprintf( buf, "Clear() called on an object that is still referenced %d time(s)!", (*s).second->GetReferenceCount() );

			gpassertm( ((*s).second->GetReferenceCount() == 0), buf );
		}
#endif


		delete (*s).second;
		s = Slots.erase( s );
	}

	return true;
}

template <class cached_object>
cache<cached_object>::
cache( bool bPrimaryThreadOnly )
{
	m_pInvalidObject		= new cached_object;
	m_bPrimaryThreadOnly	= bPrimaryThreadOnly;
}

template <class cached_object>
cache<cached_object>::
~cache(void)
{
	for( slots::iterator SlotIterator = Slots.begin(); SlotIterator != Slots.end(); ++SlotIterator )
	{
		if( (*SlotIterator).second )
		{
			delete (*SlotIterator).second;
		}
	}

	delete m_pInvalidObject;
}


// force template instantiation to generate code
SIEGE_CACHE_CPP( siege::SiegeNode );
SIEGE_CACHE_CPP( siege::SiegeMesh );
