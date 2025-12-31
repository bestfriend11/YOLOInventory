//////////////////////////////////////////////////////////////////////////////
//
// File     :  ResHandle.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains generic handle and mgr for resource management. Does
//             automatic magic number checking and acquiring/releasing/verifying
//             of handles.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __RESHANDLE_H
#define __RESHANDLE_H

//////////////////////////////////////////////////////////////////////////////

#include "KernelTool.h"

#include <vector>
#include <iterator>

//////////////////////////////////////////////////////////////////////////////
// documentation

// To use:
//
//   1. Create your object class that must be managed (say it's Foo).
//
//   2. Typedef your handle class: typedef ResHandle <Foo> FooHandle;
//      Typedef your mgr class: typedef ResHandleMgr <FooHandle> FooHandleMgr;
//
//   3. Create a manager class - either derive from FooHandleMgr or create a
//      standalone manager that will own a FooHandleMgr.
//
//   4. Implement methods in the manager that will manage Foo's: create/init,
//      query, and destroy methods. Add caching if you like by keeping some of
//      the handles open.
//
// Notes:
//
//   *  The ResHandle self-dereferencing methods rely upon somewhere in the
//      system an instance of ResHandleMgr <HANDLE> existing. Only one can exist
//      at a time, just make sure it's instantiated before giving out handles.
//
//   *  You will probably need indexes to find objects in the system. For
//      example, if you have a texture manager, you'll probably want a map of
//      strings to handles to avoid needing to iterate over the open texture set
//      to find if it's already open (and needs its refcount incremented).
//
//   *  Keep in mind what kind of maximum handle cap you want. By default it's
//      going to be 16 bits (65K entries) and can be increased via ResHandle
//      template parameter. Also there will be a "too many handles" soft limit
//      that will assert on a "reasonable" maximum hit. This is useful for
//      keeping the allocation system under control. For example, if there are
//      over, say, 1000 texture handles open at once, there must be something
//      wrong in the code perhaps?
//
//   *  These classes currently assume and require that the manager is a
//      singleton. Do not implement separate versions that work around this.
//      It will defeat the safety of the system. If there are to be multiple
//      managers, have them built from slightly separate handle types.

//////////////////////////////////////////////////////////////////////////////
// struct ResHandleFields <BITS_INDEX> declaration

// this is a generic bit container. be sure to choose more bits for the index
// if you think your manager will need more than 65K entries.

template <const UINT BITS_INDEX = 16>
struct ResHandleFields
{
	enum
	{
		// sizes to use for bit fields
		MAX_BITS_INDEX = BITS_INDEX,
		MAX_BITS_MAGIC = 32 - MAX_BITS_INDEX,

		// sizes to compare against for asserting dereferences
		MAX_INDEX = (1 << MAX_BITS_INDEX) - 1,
		MAX_MAGIC = (1 << MAX_BITS_MAGIC) - 1,
	};

	union
	{
		struct
		{
			unsigned m_Index : MAX_BITS_INDEX;		// index into resource array
			unsigned m_Magic : MAX_BITS_MAGIC;		// magic number to check against structure
		};
		UINT m_Handle;
	};

	ResHandleFields( void )							{  }
	ResHandleFields( UINT index, UINT magic )		{  m_Index = index;  m_Magic = magic;  }
};

//////////////////////////////////////////////////////////////////////////////
// struct ResHandleFields16 declaration

// use this if 16 bits is good enough for your indexing. it will be slightly
// faster than a general ResHandleFields <16> because it avoids shifts.

#pragma pack ( push, 1 )
struct ResHandleFields16
{
	enum
	{
		// sizes to use for bit fields
		MAX_BITS_INDEX = 16,
		MAX_BITS_MAGIC = 32 - MAX_BITS_INDEX,

		// sizes to compare against for asserting dereferences
		MAX_INDEX = (1 << MAX_BITS_INDEX) - 1,
		MAX_MAGIC = (1 << MAX_BITS_MAGIC) - 1,
	};

	union
	{
		struct
		{
			WORD m_Index;							// index into resource array
			WORD m_Magic;							// magic number to check against structure
		};
		UINT m_Handle;
	};

	ResHandleFields16( void )						{  }
	ResHandleFields16( UINT index, UINT magic )		{  m_Index = (WORD)index;  m_Magic = (WORD)magic;  }
};
#pragma pack ( pop )

//////////////////////////////////////////////////////////////////////////////
// class ResHandle declaration

// This is a generic API-type resource handle. fast and stupid, cheap to pass
// around as a parameter to functions. the FIELDS type must provide:
//
//   const MAX_INDEX - max index allowed
//   const MAX_MAGIC - max magic number allowed
//   m_Index         - index to use for dereferencing data
//   m_Magic         - magic number for verifying handle data
//   m_Handle        - a UINT containing all handle data
//   FIELDS ctor 1   - void ctor doing nothing
//   FIELDS ctor 2   - taking a UINT index and a UINT magic for assignment

template <typename HANDLE> class ResHandleMgr;

template <typename DATA, typename FIELDS = ResHandleFields16>
class ResHandle : public FIELDS
{
public:
	// types
	typedef ResHandle    <DATA, FIELDS> ThisType;
	typedef ThisType                    HandleType;
	typedef ResHandleMgr <ThisType>     MgrType;
	typedef DATA                        DataType;

	// null entry
	static ResHandle Null;

// Ctors.

	       ResHandle( void )  {  m_Handle = NULL;  }							// always init to zero
	inline ResHandle( UINT index, UINT magic );									// construct a handle with the given index
	explicit inline ResHandle( UINT handle ) : FIELDS( handle )  {  }			// construct this handle

// Query.

	// simple query
	UINT GetIndex ( void ) const  {  return (  m_Index  );  }					// retrieve resource index
	UINT GetMagic ( void ) const  {  return (  m_Magic  );  }					// retrieve magic number used to check validity against mgr's table
	UINT GetHandle( void ) const  {  return (  m_Handle );  }					// retrieve entire handle as a simple uint
	bool IsNull   ( void ) const  {  return ( !m_Handle );  }					// check to see if it points at nothing
	bool IsValid  ( void ) const  {  return ( GetMgr().IsValid( *this ) );  }	// check to see if it points to a non-null valid entry against mgr's table

	// operators
	operator UINT( void ) const  {  return (  m_Handle );  }					// convenient operator same as GetHandle()

// Access.

	DataType*       Get         ( void )        {  return ( GetMgr().Get( *this ) );  }		// dereference object - assert if invalid or null
	DataType*       GetValid    ( void )        {  return ( GetMgr().GetValid( *this ) );  }// dereference object - returns NULL if invalid or null
	const DataType* GetConst    ( void ) const  {  return ( GetMgr().Get( *this ) );  }		// dereference const object - assert if invalid or null ( $ SCOTTB - different name from other Get() to prevent ambiguity when calling from watch window)
	const DataType* GetValid    ( void ) const  {  return ( GetMgr().GetValid( *this ) );  }// dereference const object - returns NULL if invalid or null

	DataType*       operator -> ( void )        {  return ( Get() );  }						// member dereference operator
	const DataType* operator -> ( void ) const  {  return ( GetConst() );  }				// const member dereference operator

	DataType&       operator *  ( void )        {  return ( *Get() );  }					// member dereference operator
	const DataType& operator *  ( void ) const  {  return ( *GetConst() );  }				// const member dereference operator

	static MgrType& GetMgr      ( void )        {  return ( MgrType::GetSingleton() );  }	// get the managing object for this handle

// Special.

	// for internal use only
	static inline UINT GetNextMagic( void );	// get the next magic number

	// magic number manipulation (needed for save game)
	static UINT GetCurrentMagic( void )			{  return ( ms_AutoMagic );  }
	static void SetCurrentMagic( UINT magic )	{  ms_AutoMagic = magic;  }

private:
	static UINT ms_AutoMagic;	// auto-incrementing magic number for each new handle
};

// keep it a dword for efficiency and consistency (have to use arbitrary RECT type b/c it's a UDT)
COMPILER_ASSERT( sizeof( ResHandle <RECT> ) == sizeof( DWORD ) );

template <typename DATA, typename FIELDS>
ResHandle <DATA, FIELDS> ResHandle <DATA, FIELDS>::Null;

template <typename DATA, typename FIELDS>
UINT ResHandle <DATA, FIELDS> ::ms_AutoMagic = 0;

template <typename DATA, typename FIELDS>
inline ResHandle <DATA, FIELDS> :: ResHandle( UINT index, UINT magic )
	: FIELDS( index, magic )
{
	// verify ranges
	gpassert( index <= MAX_INDEX );
	gpassert( (magic != 0) && (magic <= MAX_MAGIC) );
}

template <typename DATA, typename FIELDS>
inline UINT ResHandle <DATA, FIELDS> :: GetNextMagic( void )
{
	if ( ++ms_AutoMagic > MAX_MAGIC )
	{
		// don't reset to 0 - that is used for "null handle"
		ms_AutoMagic = 1;
	}
	return ( ms_AutoMagic );
}

//////////////////////////////////////////////////////////////////////////////
// class AutoResHandle declaration

// This is a ResHandle that will auto-release on destruction.

template <typename HANDLE>
class AutoResHandle : public HANDLE
{
public:
	AutoResHandle( void )  {  }
	AutoResHandle( HANDLE h ) : HANDLE( h )  {  }

   ~AutoResHandle( void )
	{
		if ( !IsNull() )
		{
			GetMgr().Release( *this );
		}
	}

	SET_NO_COPYING( AutoResHandle );
};

//////////////////////////////////////////////////////////////////////////////
// class ResHandleMgrBase declaration

// This convert handles into data and back. Automatically takes care of handle
// validity checking, memory management, and sanity checks.

// $ future: this class never shrinks its data. now there's not a lot of
//   overhead here - 24 bytes per entry, but potentially with an expanded-then-
//   emptied collection that would also include just as many free list entries,
//   adding another 33%. it's not as simple as just shrinking the vectors
//   because they are sparse arrays and could be completely empty save for one
//   element way out at the end. so it's going to be kind of complicated - only
//   bother if it's _really_ worth it. until then, just call the derivative's
//   FreeAllXXX() method...

class ResHandleMgrBase
{
public:

// Types.

#	pragma pack ( push, 1 )
	struct Entry
	{
		enum
		{
			FLAG_NONE  =      0,
			FLAG_OWNED = 1 << 0,
		};

		void* m_Object;					// the (possibly owned) object - derived will cast to DataType
		void* m_Extra;					// extra data associated with this object - optionally used by derived owner
		UINT  m_Magic;					// associated magic number for handle validation
		WORD  m_Flags;					// object flags defined by the enum above
		WORD  m_RefCount;				// reference count of this object (when it reaches zero we kill it)
		UINT  m_NextIndex;				// next element in list
		UINT  m_PrevIndex;				// prev element in list
	};
#	pragma pack ( pop )

	typedef kerneltool::RwCritical RwCritical;

// Ctors.

	ResHandleMgrBase( UINT reasonableMaximum );
   ~ResHandleMgrBase( void );

// Handle methods.

	// general query
	UINT  GetUsedHandleCount( void ) const  {  return ( m_DataColl.size() - m_FreeColl.size() - 1 );  }	// how many open handles not including root?
	bool  HasUsedHandles    ( void ) const  {  return ( GetUsedHandleCount() != 0 );  }					// any in use?

	// serialized access
	const RwCritical& GetCritical( void ) const  {  return ( m_Critical );  }

	// stl helpers
	size_t size ( void ) const  {  return ( GetUsedHandleCount() );  }
	bool   empty( void ) const  {  return ( size() == 0 );  }

protected:

	// setup
	void InitListRoot( void );
	void DestroyCollections( void );

	// acquisition
	UINT  Create        ( UINT magic, void* takeObject, void* extra, bool ownsObject = true );				// returns index
	void  CreateSpecific( UINT index, UINT magic, void* takeObject, void* extra, bool ownsObject = true );	// must succeed
	void* Release       ( void*& extra, UINT index, UINT& refCount );										// returns object if needs to be deleted

	// collection docs:
	//
	//   m_DataColl - flat vector of all entries. unused slots will have their
	//   magic numbers set to 0. always contains at least one entry which is the
	//   "root" and is never used.
	//
	//   m_FreeColl - flat vector containing indexes of unused slots. optimizes
	//   Create() to work in constant time.
	//
	//   <implicit list> - internal circular doubly-linked list of objects,
	//   referenced by index to work around vector reallocs. used for fast
	//   iteration over elements in a sparse array (works in constant time).

	// private types
	typedef std::vector <Entry> DataColl;
	typedef std::vector <UINT>  FreeColl;

	// private data
	RwCritical m_Critical;				// critical locker for the db
	DataColl   m_DataColl;				// our handle data
	FreeColl   m_FreeColl;				// keeps track of free slots in the db
	UINT       m_ReasonableMaximum;		// used for assertions of too many handles

	SET_NO_COPYING( ResHandleMgrBase );
};

//////////////////////////////////////////////////////////////////////////////
// class ResHandleMgr <HANDLE> declaration

// This class is a type-safe version of ResHandleMgrBase, which exists solely
// to collect the common code into a single non-templatized class to cut down
// on bloaty head syndrome.

template <typename HANDLE>
class ResHandleMgr : public ResHandleMgrBase, public Singleton <ResHandleMgr <HANDLE> >
{
public:
	typedef ResHandleMgrBase      Inherited;
	typedef ResHandleMgr <HANDLE> ThisType;
	typedef ThisType              MgrType;
	typedef HANDLE                HandleType;
	typedef HANDLE::DataType      DataType;

// Ctors.

	         ResHandleMgr( UINT reasonableMaximum ) : Inherited( reasonableMaximum )  {  }
	virtual ~ResHandleMgr( void );												// asserts if handles still open, frees them up anyway

	void FreeAllHandles( void );												// frees up all handles
	void FreeAllHandlesAndCompact( void );										// frees up all handles and associated vector memory

// Handle methods.

	// acquisition
	inline HANDLE Create ( DataType* takeObject, bool ownsObject = true );		// returned handle has refcount of 1
	inline UINT   AddRef ( HANDLE handle );										// inc ref count of object, returns new ref count
	       UINT   Release( HANDLE handle );										// dec ref count of object, delete when zero if deleteObject true, returns new ref count

	// dereferencing
	inline bool            IsValid      ( HANDLE handle ) const;				// checks to make sure it's non-null and points to valid data
	inline bool            IsValidUnsafe( HANDLE handle ) const;				// same as above without critical (special use only)
	inline DataType*       Get          ( HANDLE handle );						// dereference object - assert if invalid or null
	inline DataType*       GetValid     ( HANDLE handle );						// dereference object - return null if invalid or null
	inline const DataType* Get          ( HANDLE handle ) const;				// dereference const object - assert if invalid or null
	inline const DataType* GetValid     ( HANDLE handle ) const;				// dereference const object - assert if invalid or null
	inline UINT            GetRefCount  ( HANDLE handle ) const;

	// special
	inline void   CreateSpecific( UINT index, UINT magic, DataType* takeObject, bool ownsObject = true );	// must succeed
	inline Entry& GetRawData( HANDLE handle );

// Iterator classes.

	class iterator;
	class const_iterator;

	// const_iterator class
	class const_iterator : public std::_Bidit <DataType, ptrdiff_t>
	{
	public:
		typedef DataColl::iterator NodePtr;

		const_iterator( void )  {  }
		const_iterator( NodePtr begin, UINT startIndex ) : m_Begin( begin ), m_Iter( begin + startIndex )  {  }
		const_iterator( const iterator& obj ) : m_Begin( obj.m_Begin ), m_Iter( obj.m_Iter )  {  }

		const DataType& operator *  ( void ) const  {  return ( *rcast <const DataType*> ( m_Iter->m_Object ) );  }
		const DataType* operator -> ( void ) const  {  return (  rcast <const DataType*> ( m_Iter->m_Object ) );  }

		const_iterator& operator ++ ( void )  {  Next();  return ( *this );  }
		const_iterator  operator ++ ( int  )  {  const_iterator tmp = *this;  Next();  return ( tmp );  }

		const_iterator& operator -- ( void )  {  Prev();  return ( *this );  }
		const_iterator  operator -- ( int  )  {  const_iterator tmp = *this;  Prev();  return ( tmp );  }

		bool operator == ( const const_iterator& obj ) const  {  return ( m_Iter == obj.m_Iter );  }
		bool operator != ( const const_iterator& obj ) const  {  return ( m_Iter != obj.m_Iter );  }

		UINT GetIndex( void ) const  {  return ( m_Iter - m_Begin );  }

	protected:
		void Next( void )  {  m_Iter = m_Begin + m_Iter->m_NextIndex;  }
		void Prev( void )  {  m_Iter = m_Begin + m_Iter->m_PrevIndex;  }

		friend MgrType;

		NodePtr m_Begin, m_Iter;
	};

	// iterator class
	class iterator : public const_iterator
	{
	public:
		iterator( void )  {  }
		iterator( NodePtr begin, UINT startIndex ) : const_iterator( begin, startIndex )  {  }

		DataType& operator *  ( void ) const  {  return ( *rcast <DataType*> ( m_Iter->m_Object ) );  }
		DataType* operator -> ( void ) const  {  return (  rcast <DataType*> ( m_Iter->m_Object ) );  }

		iterator& operator ++ ( void )  {  Next();  return ( *this );  }
		iterator  operator ++ ( int  )  {  iterator tmp = *this;  Next();  return ( tmp );  }

		iterator& operator -- ( void )  {  Prev();  return ( *this );  }
		iterator  operator -- ( int  )  {  iterator tmp = *this;  Prev();  return ( tmp );  }

		bool operator == ( const iterator& obj ) const  {  return ( m_Iter == obj.m_Iter );  }
		bool operator != ( const iterator& obj ) const  {  return ( m_Iter != obj.m_Iter );  }

		ResHandleMgrBase::Entry& GetRawData( void )  {  return ( *m_Iter );  }
	};

	// const_reverse_iterator class
	typedef std::reverse_bidirectional_iterator
			<const_iterator, DataType, const DataType&, const DataType*, ptrdiff_t>
			const_reverse_iterator;

	// reverse_iterator class
	typedef std::reverse_bidirectional_iterator
			<iterator, DataType, DataType&, DataType*, ptrdiff_t>
			reverse_iterator;

// Iterator methods.

	// forward iterator endpoints
	iterator               begin ( void )        {  return ( iterator              ( m_DataColl.begin(), m_DataColl.begin()->m_NextIndex ) );  }
	const_iterator         begin ( void ) const  {  return ( const_iterator        ( m_DataColl.begin(), m_DataColl.begin()->m_NextIndex ) );  }
	iterator               end   ( void )        {  return ( iterator              ( m_DataColl.begin(), 0 ) );  }
	const_iterator         end   ( void ) const  {  return ( const_iterator        ( m_DataColl.begin(), 0 ) );  }

	// reverse iterator endpoints
	reverse_iterator       rbegin( void )        {  return ( reverse_iterator      ( end() ) );  }
	const_reverse_iterator rbegin( void ) const  {  return ( const_reverse_iterator( end() ) );  }
	reverse_iterator       rend  ( void )        {  return ( reverse_iterator      ( begin() ) );  }
	const_reverse_iterator rend  ( void ) const  {  return ( const_reverse_iterator( begin() ) );  }

protected:
	// derived callback
	virtual void* OnCreateExtra( void )  {  return ( NULL );  }
	virtual void  OnDeleteExtra( void* /*extra*/ )  {  }

private:
	SET_NO_COPYINGX( ResHandleMgr );
};

//////////////////////////////////////////////////////////////////////////////
// class ResHandleExtraMgr <HANDLE> declaration

// This class extends ResHandleMgr <> by allowing usage of the void* "extra"
// pointer to contain extra indexing data. typical use of this will be for
// an owner class to store index iterators in here so that upon releasing
// an object for deletion it can quickly remove the index entries.

template <typename HANDLE, typename EXTRA>
class ResHandleExtraMgr : public ResHandleMgr <HANDLE>
{
public:
	typedef ResHandleMgr <HANDLE>             Inherited;
	typedef ResHandleExtraMgr <HANDLE, EXTRA> ThisType;

// Ctors.

	         ResHandleExtraMgr( UINT reasonableMaximum ) : Inherited( reasonableMaximum )  {  }
	virtual ~ResHandleExtraMgr( void );											// asserts if handles still open, frees them up anyway

// Handle methods.

	// acquisition
	inline       EXTRA* GetExtra( HANDLE handle );
	inline const EXTRA* GetExtra( HANDLE handle ) const;

private:
	virtual void* OnCreateExtra( void )         {  return ( new EXTRA );  }
	virtual void  OnDeleteExtra( void* extra )  {  delete( rcast <EXTRA*> ( extra ) );  }

	SET_NO_COPYINGX( ResHandleExtraMgr );
};

//////////////////////////////////////////////////////////////////////////////
// class ResHandleMgr <HANDLE> inline and template implementation

template <typename HANDLE>
ResHandleMgr <HANDLE> :: ~ResHandleMgr( void )
{
	// should close all handles before shutting down!
	gpassert( !HasUsedHandles() );
	// free them up anyway
	FreeAllHandlesAndCompact();
}

template <typename HANDLE>
void ResHandleMgr <HANDLE> :: FreeAllHandles( void )
{
	RwCritical::WriteLock locker( m_Critical );

	UINT index;
	while ( (index = m_DataColl.begin()->m_NextIndex) != 0 )
	{
		DataColl::iterator entry = m_DataColl.begin() + index;
		if ( entry->m_RefCount > 1 )
		{
			entry->m_RefCount = 1;
		}

		Release( HANDLE( index, entry->m_Magic ) );
	}
}

template <typename HANDLE>
void ResHandleMgr <HANDLE> :: FreeAllHandlesAndCompact( void )
{
	RwCritical::WriteLock locker( m_Critical );

	// to maximize efficiency - just iterate over all the objects, delete each
	// and then blow out all the collections at once.

	iterator i, ibegin = begin(), iend = end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i.m_Iter->m_Flags & Entry::FLAG_OWNED )
		{
			// delete owned object
			delete ( &*i );

			// delete extra object
			void* extra = i.m_Iter->m_Extra;
			if ( extra != NULL )
			{
				OnDeleteExtra( extra );
			}
		}
	}

	DestroyCollections();
}

template <typename HANDLE>
inline HANDLE ResHandleMgr <HANDLE> :: Create( DataType* takeObject, bool ownsObject )
{
	// create new entry
	UINT magic = HANDLE::GetNextMagic();
	UINT index = Inherited::Create( magic, takeObject, OnCreateExtra(), ownsObject );

	// return new handle
	return ( HANDLE( index, magic ) );
}

template <typename HANDLE>
inline void ResHandleMgr <HANDLE> :: CreateSpecific( UINT index, UINT magic, DataType* takeObject, bool ownsObject )
{
	Inherited::CreateSpecific( index, magic, takeObject, OnCreateExtra(), ownsObject );
}

template <typename HANDLE>
inline UINT ResHandleMgr <HANDLE> :: AddRef( HANDLE handle )
{
	RwCritical::ReadLock locker( m_Critical );

	gpassert( IsValidUnsafe( handle ) );
	return ( ++m_DataColl[ handle.GetIndex() ].m_RefCount );
}

template <typename HANDLE>
UINT ResHandleMgr <HANDLE> :: Release( HANDLE handle )
{
	gpassert( IsValid( handle ) );

	// release and delete on request
	UINT refCount;
	void* extra;
	DataType* data = rcast <DataType*> ( Inherited::Release( extra, handle.GetIndex(), refCount ) );
	if ( data != NULL )
	{
		// delete owned object
		delete ( data );

		// delete extra object
		if ( extra != NULL )
		{
			OnDeleteExtra( extra );
		}
	}

	// return new ref count
	return ( refCount );
}

template <typename HANDLE>
inline bool ResHandleMgr <HANDLE>::IsValid( HANDLE handle ) const
{
	RwCritical::ReadLock locker( m_Critical );
	return ( IsValidUnsafe( handle ) );
}

template <typename HANDLE>
inline bool ResHandleMgr <HANDLE>::IsValidUnsafe( HANDLE handle ) const
{
	// check for common invalid handle problems
	gpassert( !IsMemoryBadFood( handle ) );

	// check handle validity against collections
	return (   !handle.IsNull()
			&& (handle.GetIndex() < m_DataColl.size())
			&& (m_DataColl[ handle.GetIndex() ].m_Magic == handle.GetMagic()) );
}

template <typename HANDLE>
inline ResHandleMgr <HANDLE>::DataType* ResHandleMgr <HANDLE> :: Get( HANDLE handle )
{
	RwCritical::ReadLock locker( m_Critical );

	gpassert( IsValidUnsafe( handle ) );
	return ( rcast <DataType*> ( m_DataColl[ handle.GetIndex() ].m_Object ) );
}

template <typename HANDLE>
inline ResHandleMgr <HANDLE>::DataType* ResHandleMgr <HANDLE> :: GetValid( HANDLE handle )
{
	RwCritical::ReadLock locker( m_Critical );
	return ( IsValidUnsafe( handle ) ? rcast <DataType*> ( m_DataColl[ handle.GetIndex() ].m_Object ) : NULL );
}

template <typename HANDLE>
inline const ResHandleMgr <HANDLE>::DataType* ResHandleMgr <HANDLE> :: Get( HANDLE handle ) const
{
	RwCritical::ReadLock locker( m_Critical );

	gpassert( IsValidUnsafe( handle ) );
	return ( rcast <const DataType*> ( m_DataColl[ handle.GetIndex() ].m_Object ) );
}

template <typename HANDLE>
inline const ResHandleMgr <HANDLE>::DataType* ResHandleMgr <HANDLE> :: GetValid( HANDLE handle ) const
{
	RwCritical::ReadLock locker( m_Critical );
	return ( IsValidUnsafe( handle ) ? rcast <const DataType*> ( m_DataColl[ handle.GetIndex() ].m_Object ) : NULL );
}

template <typename HANDLE>
inline UINT ResHandleMgr <HANDLE> :: GetRefCount( HANDLE handle ) const
{
	RwCritical::ReadLock locker( m_Critical );

	gpassert( IsValidUnsafe( handle ) );
	return ( m_DataColl[ handle.GetIndex() ].m_RefCount );
}

template <typename HANDLE>
inline ResHandleMgrBase::Entry& ResHandleMgr <HANDLE> :: GetRawData( HANDLE handle )
{
	RwCritical::ReadLock locker( m_Critical );

	gpassert( IsValidUnsafe( handle ) );
	return ( m_DataColl[ handle.GetIndex() ] );
}

//////////////////////////////////////////////////////////////////////////////
// class ResHandleExtraMgr <HANDLE, EXTRA> inline and template implementation

template <typename HANDLE, typename EXTRA>
ResHandleExtraMgr <HANDLE, EXTRA> :: ~ResHandleExtraMgr( void )
{
	// should close all handles before shutting down!
	gpassert( !HasUsedHandles() );
	// free them up anyway
	FreeAllHandlesAndCompact();
}

template <typename HANDLE, typename EXTRA>
inline EXTRA* ResHandleExtraMgr <HANDLE, EXTRA> :: GetExtra( HANDLE handle )
{
	RwCritical::ReadLock locker( m_Critical );

	gpassert( IsValidUnsafe( handle ) );
	return ( rcast <EXTRA*> ( m_DataColl[ handle.GetIndex() ].m_Extra ) );
}

template <typename HANDLE, typename EXTRA>
inline const EXTRA* ResHandleExtraMgr <HANDLE, EXTRA> :: GetExtra( HANDLE handle ) const
{
	RwCritical::ReadLock locker( m_Critical );

	gpassert( IsValidUnsafe( handle ) );
	return ( rcast <const EXTRA*> ( m_DataColl[ handle.GetIndex() ].m_Extra ) );
}

//////////////////////////////////////////////////////////////////////////////

#endif  // __RESHANDLE_H

//////////////////////////////////////////////////////////////////////////////
