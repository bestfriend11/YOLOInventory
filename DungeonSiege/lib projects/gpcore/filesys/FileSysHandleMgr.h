//////////////////////////////////////////////////////////////////////////////
//
// File     :  FileSysHandleMgr.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains generic handle mgr for the file system. Does automatic
//             magic number checking and acquiring/releasing/verifying of
//             handles.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FILESYSHANDLEMGR_H
#define __FILESYSHANDLEMGR_H

//////////////////////////////////////////////////////////////////////////////

#include "FileSysHandle.h"
#include "GpColl.h"

namespace FileSys  {  // begin of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
// class HandleMgrBase declaration

// purpose: gathers stuff up that doesn't require a template to reduce bloaty
//  head syndrome. see docs on HandleMgr <T>.

// note on the member template functions here - if i move those out-of-line,
//  the compiler complains that it can't specialize with error C2893 (this
//  error is undocumented in MSDN).

class HandleMgrBase
{
public:
	SET_NO_INHERITED( HandleMgrBase );

	enum  {  MAX_HANDLES = 100  };

	inline  HandleMgrBase( UINT reasonableMaximum = MAX_HANDLES );
	inline ~HandleMgrBase( void );

	inline UINT GetUsedHandleCount( void ) const;		// how many total in use?
	inline bool HasUsedHandles    ( void ) const;		// any in use?

	inline bool empty( void ) const						{  return ( !HasUsedHandles() );  }

	template <typename COLL>							// construct copies of all our handles
	UINT GetHandles( COLL& coll ) const
	{
		MagicColl::iterator i, begin = m_MagicColl.begin(), end = m_MagicColl.end();
		UINT count = 0;
		for ( i = begin ; i != end ; ++i )
		{
			if ( *i != 0 )
			{
				++count;
				coll.insert( coll.end(), Handle( 0, i - begin, *i ) );
			}
		}
		return ( count );
	}

	template <typename MGR, typename HANDLE>			// call fileMgr->Close(HANDLE) for all open handles
	void CloseAllHandles( MGR* fileMgr, HANDLE )
	{
		if ( HasUsedHandles() )
		{
			MagicColl::iterator i, begin = m_MagicColl.begin(), end = m_MagicColl.end();
			for ( i = begin ; i != end ; ++i )
			{
				if ( *i != 0 )
				{
					fileMgr->Close( HANDLE( Handle( 0, i - begin, *i ) ) );
				}
			}
		}
	}

protected:
	typedef stdx::fast_vector <UINT> MagicColl;
	typedef stdx::fast_vector <UINT> FreeColl;

	UINT      m_ReasonableMaximum;		// reasonable max handles to allow (only used for sanity checking asserts)
	MagicColl m_MagicColl;				// corresponding magic numbers
	FreeColl  m_FreeColl;				// keeps track of free slots in the db

	SET_NO_COPYING( HandleMgrBase );
};

//////////////////////////////////////////////////////////////////////////////
// class HandleMgr <T> declaration

// purpose: convert handles into T's and back. automatically takes care of
//  handle validity checking, grow-n-shrink, and sanity checks.

template <typename T>
class HandleMgr : public HandleMgrBase
{
public:
	SET_INHERITED( HandleMgr <T>, HandleMgrBase );

	inline HandleMgr( UINT reasonableMaximum = MAX_HANDLES );

	T*              Acquire    ( Handle& handle, UINT ownerIndex = 0 );
	void            Release    ( Handle handle );
	inline T*       Dereference( Handle handle );
	inline const T* Dereference( Handle handle ) const  {  return ( ccast <ThisType*> ( this )->Dereference( handle ) );  }		// ok - we know it doesn't modify "this"

	typedef stdx::fast_vector <T> TColl;
	typedef TColl::const_iterator const_iterator;

	const_iterator GetBegin( void ) const  {  return ( m_TColl.begin() );  }
	const_iterator GetEnd  ( void ) const  {  return ( m_TColl.end  () );  }

	bool IsElementUsed( const const_iterator& it ) const
	{
		size_t index = it - GetBegin();
		return ( m_MagicColl[ index ] != 0 );
	}

private:
	TColl m_TColl;					// data we're going to get to

	SET_NO_COPYING( HandleMgr );
};

//////////////////////////////////////////////////////////////////////////////
// class HandleMgrBase inline/template implementation

inline HandleMgrBase :: HandleMgrBase( UINT reasonableMaximum )
	{  m_ReasonableMaximum = reasonableMaximum;  }

inline HandleMgrBase :: ~HandleMgrBase( void )
	{  gpassertm( !HasUsedHandles(), "HandleMgr destroyed with handles still open!" );  }

inline UINT HandleMgrBase :: GetUsedHandleCount( void ) const
	{  return ( m_MagicColl.size() - m_FreeColl.size() );  }

inline bool HandleMgrBase :: HasUsedHandles( void ) const
	{  return ( GetUsedHandleCount() != 0 );  }

//////////////////////////////////////////////////////////////////////////////
// class HandleMgr <T> inline/template implementation

// note on the below functions - Acquire and Release only deal with handles
//  that have already been verified so no need to do the ultra paranoia
//  check and have error codes (just assert for code safety). only Dereference
//  gets that honor.

template <typename T>
inline HandleMgr <T> :: HandleMgr( UINT reasonableMaximum )
	: Inherited( reasonableMaximum )  {  }

template <typename T>
T* HandleMgr <T> :: Acquire( Handle& handle, UINT ownerIndex )
{
	UINT index;
	if ( m_FreeColl.empty() )
	{
		index = m_MagicColl.size();
		handle = Handle( ownerIndex, index );
		m_MagicColl.push_back( handle.GetMagic() );
		m_TColl.push_back();
	}
	else
	{
		index = *m_FreeColl.rbegin();
		m_FreeColl.pop_back();
		handle = Handle( ownerIndex, index );
		gpassert( m_MagicColl[ index ] == 0 );
		m_MagicColl[ index ] = handle.GetMagic();
	}

	gpassertm( GetUsedHandleCount() < m_ReasonableMaximum, "I'm using a lot of handles!" );

	return ( &m_TColl[ index ] );
}

template <typename T>
void HandleMgr <T> :: Release( Handle handle )
{
	// which one?
	UINT index = handle.GetResourceIndex();

	// make sure it's valid
	gpassert(   handle.AssertValid()
			 && ( index < m_TColl.size() )
			 && ( m_MagicColl[ index ] == handle.GetMagic() ) )

	// ok remove it - tag as unused and add to free list
	m_MagicColl[ index ] = 0;
	m_FreeColl.push_back( index );
}

template <typename T>
inline T* HandleMgr <T> :: Dereference( Handle handle )
{
	gpassertm( handle.AssertValid(), "Attempt to dereference an invalid FileSys handle" );
	if ( handle.IsNull() )
	{
		return ( NULL );
	}

	UINT index = handle.GetResourceIndex();
	T* entry = NULL;

	if (   ( index >= m_TColl.size() )
		|| ( m_MagicColl[ index ] != handle.GetMagic() ) )
	{
		::SetLastError( ERROR_INVALID_HANDLE );
		gpassertm( 0, "Attempt to dereference an invalid FileSys handle" );
	}
	else
	{
		entry = &m_TColl[ index ];
	}

	return ( entry );
}

//////////////////////////////////////////////////////////////////////////////
// class ObjectPool declaration

// $ notes:
//
// 1. each T is constructed and destructed exactly once. no copy ctors are
//    ever called (requires fast_vector).
//
// 2. this is not thread-safe, so make sure that any allocs and frees are done
//    under a serializer.
//
// 3. currently it's grow-only. shouldn't need anything fancy.

template <typename T>
class ObjectPool
{
public:
	SET_NO_INHERITED( ObjectPool );

	T* Alloc( void )
	{
		ObjectColl::iterator i, ibegin = m_ObjectPool.begin(), iend = m_ObjectPool.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( !i->m_Used )
			{
				break;
			}
		}
		if ( i == iend )
		{
			i = m_ObjectPool.push_back();
			gpassertm( m_ObjectPool.size() < 50, "I'm using a lot of objects!!!" );
		}

		i->m_Used = true;
		return ( i->m_Object );
	}

	void Free( T* object )
	{
		ObjectColl::iterator i, ibegin = m_ObjectPool.begin(), iend = m_ObjectPool.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i->m_Object = object )
			{
				i->m_Used = false;
				break;
			}
		}

		gpassert( i != iend );
	}

private:
	struct Entry
	{
		my T* m_Object;
		bool  m_Used;

		Entry( void )
		{
			m_Object = new T;
			m_Used = false;
		}

	   ~Entry( void )
		{
			Delete( m_Object );
		}
	};

	typedef stdx::fast_vector <Entry> ObjectColl;

	ObjectColl m_ObjectPool;

	SET_NO_COPYING( ObjectPool );
};

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FileSys

#endif  // __FILESYSHANDLEMGR_H

//////////////////////////////////////////////////////////////////////////////
