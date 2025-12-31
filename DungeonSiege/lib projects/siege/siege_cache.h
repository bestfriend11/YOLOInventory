#pragma once
#ifndef _SIEGE_CACHE_
#define _SIEGE_CACHE_
/* ========================================================================
   Header File: siege_cache.h
   Derived From: soul_cache.h
   Declares: siege::cache_slot
             siege::cache
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */

#include "siege_database_guid.h"
#include "kerneltool.h"
#include <map>

/* ========================================================================
   Forward Declarations
   ======================================================================== */

namespace siege
{
	// from siege_cache_handle.h
	template <class cached_object> class cache_handle;
};

/* ************************************************************************
   Class: cache_slot
   Description: Describes a known object that may either be in use or
                loaded, or neither
   ************************************************************************ */
namespace siege
{
	template <class cached_object>
	class cache_slot
	{
	public:
	
		// Access
		inline cached_object const *GetCachedObject(void) const		{ return m_pCachedObject; }
		inline cached_object *GetCachedObject(void)					{ return m_pCachedObject; }

		// I/O control
		bool Load();
		bool Create();

		void Unload()
		{
			delete m_pCachedObject;
			m_pCachedObject = NULL;
		}

		// Reference counting
		void IncrementReferenceCount(void)							{ ++m_ReferenceCount; }
		void DecrementReferenceCount(void)							{ gpassert( m_ReferenceCount > 0 );
																	  --m_ReferenceCount; }
		
		unsigned int GetReferenceCount(void)						{ return m_ReferenceCount; }

		database_guid GetSlotGuid(void) const						{ return m_GUID; }
	
		// Existence
		explicit cache_slot(database_guid const &GUID)
				: m_GUID( GUID )
				, m_ReferenceCount( 0 )
				, m_pCachedObject( 0 )
		{}

		~cache_slot(void)
		{
			gpassert( m_ReferenceCount == 0 );
			Unload();
		}
	
	private:
		// Slot
		database_guid	m_GUID;
		cached_object*	m_pCachedObject;

		// Referencing
		unsigned int	m_ReferenceCount;
	};
};

/* ************************************************************************
   Class: shared_mesh_cache
   Description: Holds and controls all objects of the designated type
   ************************************************************************ */
namespace siege
{
	template <class cached_object>
	class cache
	{
	public:

		// Types
		typedef cache_slot<cached_object> slot;
		typedef cache_handle<cached_object> handle;
		
		// Usage
		handle UseObject(database_guid const &GUID);
		const handle UseObject(database_guid const &GUID) const;

		// Does the given object exist in our current cache?
		bool ObjectExistsInCache(database_guid const &GUID) const;

		// Gets a valid object if one exists
		cached_object* GetValidObjectInCache(database_guid const &GUID);

		// Frees the object and deletes it.  Any stored handles are
		// still valid, and if you RequestObject() on those handles,
		// the object will be reloaded.
		bool RemoveObjectFromCache( database_guid const &GUID );

		// Completely removes and object from the cache.  Frees the
		// object, and removes the slot from the cache.  All handles
		// to this object are now invalid, and any attempt to use
		// them will probably cause an access violation.
		bool RemoveSlotFromCache( database_guid const &GUID );

		// Removes all slots from cache.  Flush(..) would remove all objects from Cache
		bool Clear();

		// Access
		cached_object&			GetSlot( cache_slot<cached_object> *Slot );
		cached_object const&	GetSlot( cache_slot<cached_object> *Slot ) const;

		cached_object&			CreateSlot( cache_slot<cached_object> *Slot );
		cached_object const&	CreateSlot( cache_slot<cached_object> *Slot ) const;

		// Existence
		cache( bool bPrimaryThreadOnly );
		~cache(void);

	private:

		// Slot storage
		typedef std::map< database_guid, slot* > slots;
		slots					Slots;
		cached_object*			m_pInvalidObject;
		bool					m_bPrimaryThreadOnly;
	};
};

#endif
