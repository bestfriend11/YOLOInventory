#pragma once
#ifndef _SIEGE_CACHE_HANDLE_
#define _SIEGE_CACHE_HANDLE_
/* ========================================================================
   Header File: siege_cache_handle.h
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "GpCore.h"

/* ========================================================================
   Forward Declarations
   ======================================================================== */
namespace siege
{
	template <class cached_object> class cache;
	template <class cached_object> class cache_slot;
	template <class cached_object> class cache_handle;
};

/* ************************************************************************
   Class: cache_handle
   Description: Holds a reference to an object that can be accessed by request
   ************************************************************************ */
namespace siege
{
	template <class cached_object>
	class cache_handle
	{
	public:
		
		// Existence
		cache_handle &operator=(cache_handle const &CopyHandle);

		cache_handle(cache_handle const &CopyHandle)
				: Slot(CopyHandle.Slot)
		{
			gpassert(Slot);
			Slot->IncrementReferenceCount();
		}

		~cache_handle(void)
		{
			gpassert(Slot);
			Slot->DecrementReferenceCount();
		}

		// Request object
		inline cached_object& RequestObject( cache<cached_object> &Cache )
		{
			gpassert( Slot );
			return( Cache.GetSlot( Slot ) );
		}

		inline cached_object const & RequestObject( cache<cached_object> const &Cache ) const
		{
			gpassert( Slot );
			return( Cache.GetSlot( Slot ) );
		}

		inline cached_object& CreateObject( cache<cached_object> &Cache )
		{
			gpassert( Slot );
			return( Cache.CreateSlot( Slot ) );
		}

		inline cached_object const & CreateObject( cache<cached_object> const &Cache ) const
		{
			gpassert( Slot );
			return( Cache.CreateSlot( Slot ) );
		}

		// Get the reference count of the current object.
		unsigned int GetReferenceCountOfObject(void)	{ return Slot->GetReferenceCount(); }

		// Get the slot guid
		database_guid GetGuid(void)						{ return Slot->GetSlotGuid(); }
		const database_guid GetGuid(void) const			{ return Slot->GetSlotGuid(); }

	protected:
		// Only the cache can construct handles
		friend cache<cached_object>;

		cache_handle(cache_slot<cached_object> &SlotInit)
				: Slot(&SlotInit)
		{
			gpassert(Slot);
			Slot->IncrementReferenceCount();
		}

		cache_slot<cached_object> *Slot;
	};	
};


#endif
