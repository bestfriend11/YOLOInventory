//////////////////////////////////////////////////////////////////////////////
//
// File     :  ResHandle.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "ResHandle.h"

#include "StdHelp.h"

//////////////////////////////////////////////////////////////////////////////
// class ResHandleMgrBase implementation

ResHandleMgrBase :: ResHandleMgrBase( UINT reasonableMaximum )
{
	m_ReasonableMaximum = reasonableMaximum;
	InitListRoot();
}

ResHandleMgrBase :: ~ResHandleMgrBase( void )
{
	// shouldn't destroy this with handles still open! will leak memory!
	gpassert( !HasUsedHandles() );
}

void ResHandleMgrBase :: InitListRoot( void )
{
	gpassert( m_DataColl.empty() );

	DataColl::iterator entry = m_DataColl.insert( m_DataColl.end() );
	entry->m_Object    = NULL;
	entry->m_Extra     = NULL;
	entry->m_Magic     = 0;
	entry->m_Flags     = 0;
	entry->m_RefCount  = 0;
	entry->m_NextIndex = 0;
	entry->m_PrevIndex = 0;
}

void ResHandleMgrBase :: DestroyCollections( void )
{
	RwCritical::WriteLock locker( m_Critical );

	// $ this is the only way to force a vector to free its memory, given that
	//   it only grows...

	// destroy and rebuild data collection
	m_DataColl.~vector();
	m_DataColl.DataColl::vector();

	// destroy and rebuild free list
	m_FreeColl.~vector();
	m_FreeColl.FreeColl::vector();

	InitListRoot();
}

UINT ResHandleMgrBase :: Create( UINT magic, void* takeObject, void* extra, bool ownsObject )
{
	RwCritical::WriteLock locker( m_Critical );

	// parameter validation
	gpassert( (magic != 0) && (takeObject != NULL) );

	// acquire slot either from the free list or a new one at the end
	DataColl::iterator entry;
	if ( m_FreeColl.empty() )
	{
		entry = m_DataColl.insert( m_DataColl.end() );

#		if !GP_RETAIL
		if ( (m_ReasonableMaximum != 0) && (GetUsedHandleCount() >= m_ReasonableMaximum) )
		{
			// warn
			gpperff(( "PERFORMANCE WARNING: I'm using %d handles, that's a lot!\n", GetUsedHandleCount() ));

			// add 50% for next time
			m_ReasonableMaximum += m_ReasonableMaximum / 2;
		}
#		endif // !GP_RETAIL
	}
	else
	{
		entry = m_DataColl.begin() + *m_FreeColl.rbegin();
		m_FreeColl.pop_back();
		gpassert( entry->m_Magic == 0 );
	}
	UINT entryIndex = entry - m_DataColl.begin();

	// set up slot
	entry->m_Object   = takeObject;
	entry->m_Extra    = extra;
	entry->m_Magic    = magic;
	entry->m_Flags    = scast <WORD> ( ownsObject ? Entry::FLAG_OWNED : Entry::FLAG_NONE );
	entry->m_RefCount = 1;

	// link it at the end
	DataColl::iterator head = m_DataColl.begin();
	entry->m_NextIndex = 0;											// new entry's next points to head
	entry->m_PrevIndex = head->m_PrevIndex;							// new entry's prev points to head's old prev
	m_DataColl[ entry->m_PrevIndex ].m_NextIndex = entryIndex;		// next of old prev points to new entry
	head->m_PrevIndex = entryIndex;									// prev of head points to new entry

	// done
	return ( entryIndex );
}

void ResHandleMgrBase :: CreateSpecific( UINT index, UINT magic, void* takeObject, void* extra, bool ownsObject )
{
	RwCritical::WriteLock locker( m_Critical );

	// parameter validation
	gpassert( (magic != 0) && (takeObject != NULL) );

	DataColl::iterator entry;

	// check range
	if ( index >= m_DataColl.size() )
	{
		// off the end - alloc into the free list until we get there
		for ( UINT empty = m_DataColl.size() ; empty != index ; ++empty )
		{
			// just insert empty entries and tag them as unused
			entry = m_DataColl.insert( m_DataColl.end() );
			entry->m_Object = NULL;
			entry->m_Extra = NULL;
			entry->m_Magic = 0;

			// add to free list
			m_FreeColl.push_back( empty );
		}

		// ok it's the next one
		entry = m_DataColl.insert( m_DataColl.end() );
	}
	else
	{
		// must already be in free list!
		FreeColl::iterator found = stdx::find( m_FreeColl, index );
		gpassert( found != m_FreeColl.end() );

		// use it
		entry = m_DataColl.begin() + index;

		// remove free entry
		m_FreeColl.erase( found );
	}

	// set up slot
	entry->m_Object   = takeObject;
	entry->m_Extra    = extra;
	entry->m_Magic    = magic;
	entry->m_Flags    = scast <WORD> ( ownsObject ? Entry::FLAG_OWNED : Entry::FLAG_NONE );
	entry->m_RefCount = 1;

	// link it at the end
	DataColl::iterator head = m_DataColl.begin();
	entry->m_NextIndex = 0;										// new entry's next points to head
	entry->m_PrevIndex = head->m_PrevIndex;						// new entry's prev points to head's old prev
	m_DataColl[ entry->m_PrevIndex ].m_NextIndex = index;		// next of old prev points to new entry
	head->m_PrevIndex = index;									// prev of head points to new entry
}

void* ResHandleMgrBase :: Release( void*& extra, UINT index, UINT& refCount )
{
	RwCritical::WriteLock locker( m_Critical );

	// make sure it's valid
	gpassert(   (index < m_DataColl.size())
			 && (m_DataColl[ index ].m_Magic    != 0)
			 && (m_DataColl[ index ].m_RefCount >  0) );

	void* objectToDelete = NULL;
	extra = NULL;

	// dec ref count and see if needs to be deleted
	DataColl::iterator entry = m_DataColl.begin() + index;
	if ( --entry->m_RefCount == 0 )
	{
		// return it if it needs deleting
		if ( entry->m_Flags & Entry::FLAG_OWNED )
		{
			objectToDelete = entry->m_Object;
			extra = entry->m_Extra;
		}

		// clear out entry
		entry->m_Object = NULL;
		entry->m_Extra  = NULL;
		entry->m_Magic  = 0;

		// unlink from list
		m_DataColl[ entry->m_NextIndex ].m_PrevIndex = entry->m_PrevIndex;
		m_DataColl[ entry->m_PrevIndex ].m_NextIndex = entry->m_NextIndex;

		// add to free list
		m_FreeColl.push_back( index );
	}

	// return ref count
	refCount = entry->m_RefCount;

	// done
	return ( objectToDelete );
}

//////////////////////////////////////////////////////////////////////////////
