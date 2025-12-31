/* ========================================================================
   Source File: siege_cache_handle.cpp
   Derived From: soul_cache_handle.cpp
   Defines: siege::cache_handle
            siege::cache_read_lock
			siege::cache_read_write_lock
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

using namespace siege;

/* ========================================================================
   Instantion Macro
   ======================================================================== */
#define SIEGE_CACHE_HANDLE_CPP(class_name) \
template cache_handle<class_name>;

/* ************************************************************************
   Function: operator=
   Description: Dereferences a handle, and rereferences the handle to copy
   ************************************************************************ */
template <class cached_object>
cache_handle<cached_object> &cache_handle<cached_object>::
operator=(cache_handle<cached_object> const &CopyHandle)
{
	if(this != &CopyHandle)
	{
		gpassert(Slot);
		Slot->DecrementReferenceCount();

		Slot = CopyHandle.Slot;
		gpassert(Slot);
		Slot->IncrementReferenceCount();
	}

	return(*this);
}

// force template instantiation to generate code
SIEGE_CACHE_HANDLE_CPP( siege::SiegeNode );
SIEGE_CACHE_HANDLE_CPP( siege::SiegeMesh );
