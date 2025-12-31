//////////////////////////////////////////////////////////////////////////////
//
// File     :  BucketVector.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a specialized vector class that has built-in free list
//             tracking and an internal linked list hooking the buckets
//             together for efficient iteration. The thing even has magic
//             number code built in for added safety.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __BUCKETVECTOR_H
#define __BUCKETVECTOR_H

//////////////////////////////////////////////////////////////////////////////

#include "GpColl.h"
#include "KernelTool.h"

#include <list>
#include <map>

//////////////////////////////////////////////////////////////////////////////
// class BucketVector <T> declaration

// $ this class is simplified from the ResHandleMgr set of classes (slightly
//   faster, smaller, more memory efficient). it could be improved by hoisting
//   some of the code up to a common base, decrease bloat a little - same as
//   ResHandleMgrBase etc.

template <typename T>
class BucketVector
{
private:

// Private types.

	// collection docs:
	//
	//   m_DataColl - flat vector of all entries. unused buckets will have their
	//   magic numbers set to 0. always contains at least one entry which is the
	//   "root" and is never used.
	//
	//   <implicit m_DataColl list> - internal circular doubly-linked list of
	//   objects, referenced by index to work around vector reallocs. used for
	//   fast and constant-time iteration over elements in a sparse array.
	//
	//   m_FreeList - list containing indexes of unused buckets. optimizes
	//   Alloc() to work in constant time. use oldest (from the front) first.
	//   note that i would really like to put a reverse iter into the FreeIndex
	//   here for further efficiency but there's recursive template dependency
	//   problems. <sigh>
	//
	//   m_FreeIndex - index into m_FreeList for AllocSpecific() to work in
	//   logarithmic time when the specific entry is in the free list.

	// bucket type
	struct Entry
	{
		T    m_Object;
		UINT m_Magic;
		UINT m_NextIndex;
		UINT m_PrevIndex;
	};

	// collections
	typedef stdx::fast_vector <Entry> DataColl;				// generic container
	typedef std::list <UINT> FreeList;						// free buckets stored oldest to newest
	typedef std::map <UINT, FreeList::iterator> FreeIndex;	// maps bucket index to free coll index

public:
	SET_NO_INHERITED( BucketVector );

	typedef T value_type;

// Setup.

	// ctor/dtor
	BucketVector( UINT maxMagic, UINT minDistance = 0 )
	{
		InitListRoot();
		m_MaxMagic         = maxMagic;
		m_MinDistance      = minDistance;
		m_IsZeroIndexValid = false;
		m_NextMagic        = 0;
	}

   ~BucketVector( void )  {  }

	// restart/shutdown
	void Shutdown( bool fullRelease = true )
	{
		kerneltool::RwCritical::WriteLock writeLock( m_Critical );

		if ( fullRelease )
		{
			m_DataColl.destroy();
		}
		else
		{
			m_DataColl.clear();
		}

		// destroy other collections
		m_FreeList.clear();
		m_FreeIndex.clear();

		// reinit list root
		InitListRoot();
	}

	// options
	void SetIsZeroIndexValid( bool set = true )				{  m_IsZeroIndexValid = set;  }
	bool GetIsZeroIndexValid( void ) const					{  return ( m_IsZeroIndexValid );  }

	// magic number manipulation (needed for save game)
	UINT GetCurrentMagic( void ) const						{  return ( m_NextMagic );  }
	void SetCurrentMagic( UINT magic )						{  gpassert( empty() || (m_NextMagic == magic) );  m_NextMagic = magic;  }

	// critical access
	kerneltool::RwCritical& GetCritical( void ) const		{  return ( m_Critical );  }

// Handle methods.

	UINT GetNextMagic( void )
	{
		if ( ++m_NextMagic > m_MaxMagic )
		{
			// don't reset to 0 - that is used for "null handle"
			m_NextMagic = 1;
		}
		return ( m_NextMagic );
	}

	void Alloc( T*& object, UINT& index, UINT& magic )
	{
		kerneltool::RwCritical::WriteLock writeLocker( m_Critical );

		// get magic
		magic = GetNextMagic();

		// acquire bucket either from the oldest entry in the free list or a new
		// one at the end. make sure the free list is at least as big as the min
		// allowed distance to make sure we don't get disordered packets messing
		// things up.
		DataColl::iterator entry;
		if ( m_FreeList.size() <= m_MinDistance )
		{
			// get entry and index
			entry = m_DataColl.insert( m_DataColl.end() );
			index = entry - m_DataColl.begin();
		}
		else
		{
			// get index
			index = *m_FreeList.begin();

			// find entry
			entry = m_DataColl.begin() + index;
			gpassert( entry->m_Magic == 0 );

			// free pointers
			m_FreeIndex.erase( m_FreeIndex.find( index ) );
			m_FreeList.pop_front();
		}

		// get params
		object = &entry->m_Object;

		// set up bucket and add to internal linked list
		entry->m_Magic = magic;
		Link( entry, index );
	}

	void Alloc( const T& object, UINT& index, UINT& magic )
	{
		kerneltool::RwCritical::WriteLock writeLocker( m_Critical );

		T* ptr;
		Alloc( ptr, index, magic );
		*ptr = object;
	}

	bool AllocSpecific( T*& object, UINT index, UINT magic )
	{
		kerneltool::RwCritical::WriteLock writeLocker( m_Critical );

		gpassert( magic != 0 );
		DataColl::iterator entry;

		// check range
		if ( index >= m_DataColl.size() )
		{
			// off the end - alloc into the free list until we get there
			for ( UINT empty = m_DataColl.size() ; empty != index ; ++empty )
			{
				// just insert empty entries and tag them as unused
				m_DataColl.insert( m_DataColl.end() )->m_Magic = 0;
				AddFree( empty );
			}

			// ok it's the next one
			entry = m_DataColl.insert( m_DataColl.end() );
			entry->m_Magic = 0;
		}
		else
		{
			// in free list?
			FreeIndex::iterator found = m_FreeIndex.find( index );
			if ( found != m_FreeIndex.end() )
			{
				// easy - use it
				entry = m_DataColl.begin() + index;

				// remove free entries
				m_FreeList.erase( found->second );
				m_FreeIndex.erase( found );
			}
			else
			{
				// it's already been allocated
				entry = m_DataColl.begin() + index;
				gpassert( entry->m_Magic == magic );
				object = &entry->m_Object;

				// return false because we were already alloc'd
				return ( false );
			}
		}

		// get params
		object = &entry->m_Object;

		// set up bucket and add to internal linked list
		gpassert( entry->m_Magic == 0 );
		entry->m_Magic = magic;
		Link( entry, index );

		// insertion succeeded
		return ( true );
	}

	void Free( UINT index )
	{
		kerneltool::RwCritical::WriteLock writeLocker( m_Critical );

		// make sure it's valid
		gpassert( (index < m_DataColl.size()) && (m_DataColl[ index ].m_Magic != 0) );

		// find it
		DataColl::iterator entry = m_DataColl.begin() + index;

		// clear magic to tag it as unused
		entry->m_Magic = 0;

		// unlink from list
		m_DataColl[ entry->m_NextIndex ].m_PrevIndex = entry->m_PrevIndex;
		m_DataColl[ entry->m_PrevIndex ].m_NextIndex = entry->m_NextIndex;

		// add to free list and index
		AddFree( index );
	}

	void Free( UINT index, const T& newValue )
	{
		kerneltool::RwCritical::WriteLock writeLocker( m_Critical );

		Free( index );
		m_DataColl[ index ].m_Object = newValue;
	}

	T* Dereference( UINT index, UINT magic )
	{
		kerneltool::RwCritical::ReadLock readLocker( m_Critical );

		if ( (magic != 0) && (index < m_DataColl.size()) )
		{
			Entry& entry = m_DataColl[ index ];
			if ( entry.m_Magic == magic )
			{
				return ( &entry.m_Object );
			}
		}
		return ( NULL );
	}

	const T* Dereference( UINT index, UINT magic ) const
	{
		// $ just use the other deref function - no difference between the const
		//   and non-const versions.
		return ( ccast <ThisType*> ( this )->Dereference( index, magic ) );
	}

	bool IsValid( UINT index, UINT magic ) const
	{
		kerneltool::RwCritical::ReadLock readLocker( m_Critical );

		gpassert( magic != 0 );
		return ( ((index < m_DataColl.size()) && (m_DataColl[ index ].m_Magic == magic)) || ((index == 0) && m_IsZeroIndexValid) );
	}

	bool IsValidIndex( UINT index ) const
	{
		kerneltool::RwCritical::ReadLock readLocker( m_Critical );

		return ( index < m_DataColl.size() );
	}

	bool GetMagic( UINT index, UINT& magic ) const
	{
		kerneltool::RwCritical::ReadLock readLocker( m_Critical );

		if ( index < m_DataColl.size() )
		{
			magic = m_DataColl[ index ].m_Magic;
			return ( true );
		}
		else
		{
			magic = 0;
			return ( false );
		}
	}

	UINT GetUsedBucketCount( void ) const  {  return ( m_DataColl.size() - m_FreeList.size() - 1 );  }	// how many open buckets not including root?
	bool HasUsedBucket     ( void ) const  {  return ( GetUsedBucketCount() != 0 );  }					// any in use?

	// stl support
	size_t size ( void ) const  {  return ( GetUsedBucketCount() );  }
	bool   empty( void ) const  {  return ( size() == 0 );  }
	void   clear( void )        {  Shutdown();  }

// Iterator classes.

	class iterator;
	class const_iterator;

	// const_iterator class
	class const_iterator : public std::_Bidit <T, ptrdiff_t>
	{
	public:
		typedef DataColl::iterator NodePtr;

		const_iterator( void )  {  }
		const_iterator( NodePtr begin, UINT startIndex ) : m_Begin( begin ), m_Iter( begin + startIndex )  {  }
		const_iterator( const iterator& obj ) : m_Begin( obj.m_Begin ), m_Iter( obj.m_Iter )  {  }

		const T& operator *  ( void ) const  {  return ( m_Iter->m_Object );  }
		const T* operator -> ( void ) const  {  return ( &m_Iter->m_Object );  }

		const_iterator& operator ++ ( void )  {  Next();  return ( *this );  }
		const_iterator  operator ++ ( int  )  {  const_iterator tmp = *this;  Next();  return ( tmp );  }

		const_iterator& operator -- ( void )  {  Prev();  return ( *this );  }
		const_iterator  operator -- ( int  )  {  const_iterator tmp = *this;  Prev();  return ( tmp );  }

		bool operator == ( const const_iterator& obj ) const  {  return ( m_Iter == obj.m_Iter );  }
		bool operator != ( const const_iterator& obj ) const  {  return ( m_Iter != obj.m_Iter );  }

		UINT GetIndex( void ) const  {  return ( m_Iter - m_Begin );  }
		UINT GetMagic( void ) const  {  return ( m_Iter->m_Magic );  }

	protected:
		void Next( void )  {  m_Iter = m_Begin + m_Iter->m_NextIndex;  }
		void Prev( void )  {  m_Iter = m_Begin + m_Iter->m_PrevIndex;  }

		NodePtr m_Begin, m_Iter;
	};

	// iterator class
	class iterator : public const_iterator
	{
	public:
		iterator( void )  {  }
		iterator( NodePtr begin, UINT startIndex ) : const_iterator( begin, startIndex )  {  }

		T& operator *  ( void ) const  {  return ( m_Iter->m_Object );  }
		T* operator -> ( void ) const  {  return ( &m_Iter->m_Object );  }

		iterator& operator ++ ( void )  {  Next();  return ( *this );  }
		iterator  operator ++ ( int  )  {  iterator tmp = *this;  Next();  return ( tmp );  }

		iterator& operator -- ( void )  {  Prev();  return ( *this );  }
		iterator  operator -- ( int  )  {  iterator tmp = *this;  Prev();  return ( tmp );  }

		bool operator == ( const iterator& obj ) const  {  return ( m_Iter == obj.m_Iter );  }
		bool operator != ( const iterator& obj ) const  {  return ( m_Iter != obj.m_Iter );  }
	};

	// const_reverse_iterator class
	typedef std::reverse_bidirectional_iterator
			<const_iterator, T, const T&, const T*, ptrdiff_t>
			const_reverse_iterator;

	// reverse_iterator class
	typedef std::reverse_bidirectional_iterator
			<iterator, T, T&, T*, ptrdiff_t>
			reverse_iterator;

// Iterator methods.

	// forward iterator endpoints
	iterator               begin ( void )        {  return ( iterator              ( m_DataColl.begin(), m_DataColl.begin()->m_NextIndex ) );  }
	const_iterator         begin ( void ) const  {  return ( const_iterator        ( ccast <DataColl&> ( m_DataColl ).begin(), m_DataColl.begin()->m_NextIndex ) );  }
	iterator               end   ( void )        {  return ( iterator              ( m_DataColl.begin(), 0 ) );  }
	const_iterator         end   ( void ) const  {  return ( const_iterator        ( ccast <DataColl&> ( m_DataColl ).begin(), 0 ) );  }

	// reverse iterator endpoints
	reverse_iterator       rbegin( void )        {  return ( reverse_iterator      ( end() ) );  }
	const_reverse_iterator rbegin( void ) const  {  return ( const_reverse_iterator( end() ) );  }
	reverse_iterator       rend  ( void )        {  return ( reverse_iterator      ( begin() ) );  }
	const_reverse_iterator rend  ( void ) const  {  return ( const_reverse_iterator( begin() ) );  }

	// special iterator query
	iterator GetIteratorAt( UINT index, bool seekNext = false )
	{
		// $$ optz: can speed this up by using the free list index. look up the
		//    upper bound of this index (or whatever) and iterate from there -1
		//    or +1 or whatever works, don't have time to think about it now.

		kerneltool::RwCritical::ReadLock readLocker( m_Critical );

		// if out of bounds, just return the start
		size_t size = m_DataColl.size();
		if ( (index == 0) || (index >= size) )
		{
			return ( begin() );
		}

		// if valid, it's cool then, return it
		if ( m_DataColl[ index ].m_Magic != 0 )
		{
			return ( iterator( m_DataColl.begin(), index ) );
		}

		// if not seeking next, return end
		if ( !seekNext )
		{
			return ( end() );
		}

		// ok now iterate until we find the next valid index
		for ( ++index ; index < size ; ++index )
		{
			if ( m_DataColl[ index ].m_Magic != 0 )
			{
				return ( iterator( m_DataColl.begin(), index ) );
			}
		}

		// still not found after we went to the end? just return the first one
		return ( begin() );
	}
	const_iterator GetIteratorAt( UINT index, bool seekNext = false ) const
	{
		return ( ccast <ThisType&> ( *this ).GetIteratorAt( index, seekNext ) );
	}

	T& UnsafeDereference( UINT index )
	{
		gpassert( index < m_DataColl.size() );
		gpassert( m_IsZeroIndexValid || (m_DataColl[ index ].m_Magic != 0) );
		return ( m_DataColl[ index ].m_Object );
	}

	const T& UnsafeDereference( UINT index ) const
	{
		// $ just use the other deref function - no difference between the const
		//   and non-const versions.
		return ( ccast <ThisType*> ( this )->UnsafeDereference( index ) );
	}

	T CriticalUnsafeDereference( UINT index ) const
	{
		kerneltool::RwCritical::ReadLock readLocker( m_Critical );

		gpassert( index < m_DataColl.size() );
		gpassert( m_IsZeroIndexValid || (m_DataColl[ index ].m_Magic != 0) );
		return ( m_DataColl[ index ].m_Object );
	}

private:

	void InitListRoot( void )
	{
		gpassert( m_DataColl.empty() );

		DataColl::iterator entry = m_DataColl.insert( m_DataColl.end() );
		entry->m_Magic     = 0;
		entry->m_NextIndex = 0;
		entry->m_PrevIndex = 0;
	}

	void Link( DataColl::iterator entry, UINT index )
	{
		DataColl::iterator head = m_DataColl.begin();
		entry->m_NextIndex = 0;										// new entry's next points to head
		entry->m_PrevIndex = head->m_PrevIndex;						// new entry's prev points to head's old prev
		m_DataColl[ entry->m_PrevIndex ].m_NextIndex = index;		// next of old prev points to new entry
		head->m_PrevIndex = index;									// prev of head points to new entry
	}

	void AddFree( UINT index )
	{
		gpassert( m_FreeIndex.find( index ) == m_FreeIndex.end() );
		m_FreeIndex[ index ] = m_FreeList.insert( m_FreeList.end(), index );
	}

	// spec
	UINT m_MaxMagic;					// max magic for when counter should roll over
	UINT m_MinDistance;					// min distance between next go and last go in id's
	bool m_IsZeroIndexValid;			// set to true if you want to consider the zero index as an actual valid slot

	// state
	UINT      m_NextMagic;				// next magic number to use
	DataColl  m_DataColl;				// our handle data
	FreeList  m_FreeList;				// keeps track of free buckets in the db
	FreeIndex m_FreeIndex;				// db of free buckets sorted by index

	// sync
	mutable kerneltool::RwCritical m_Critical;	// thread safety

	SET_NO_COPYING( BucketVector );
};

//////////////////////////////////////////////////////////////////////////////
// class UnsafeBucketVector <T> declaration

template <typename T>
class UnsafeBucketVector : public BucketVector <T>
{
public:
	SET_INHERITED( UnsafeBucketVector <T>, BucketVector <T> );

	UnsafeBucketVector( UINT minDistance = 0 )	: Inherited( 1, minDistance )  {  }

	UINT     Alloc( T*& object )						{  UINT index, magic;  Inherited::Alloc( object, index, magic );  return ( index );  }
	UINT     Alloc( T object )							{  UINT index, magic;  Inherited::Alloc( object, index, magic );  return ( index );  }

	bool     AllocSpecific( T*& object, UINT index )	{  return ( Inherited::AllocSpecific( object, index, 1 ) );  }
	bool     AllocSpecific( T object, UINT index )		{  T* ptr;  bool rc = AllocSpecific( ptr, index );  if ( rc )  {  *ptr = object;  }  return ( rc );  }

	T&       Dereference( UINT index )					{  return ( UnsafeDereference( index ) );  }
	const T& Dereference( UINT index ) const			{  return ( UnsafeDereference( index ) );  }

	T*       SafeDereference( UINT index )				{  return ( Inherited::Dereference( index, 1 ) );  }
	const T* SafeDereference( UINT index ) const		{  return ( Inherited::Dereference( index, 1 ) );  }

	bool     IsValid( UINT index ) const				{  return ( Inherited::IsValid( index, 1 ) );  }

	T        operator [] ( UINT index ) const			{  return ( CriticalUnsafeDereference( index ) );  }

private:
	SET_NO_COPYING( UnsafeBucketVector );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __BUCKETVECTOR_H

//////////////////////////////////////////////////////////////////////////////
