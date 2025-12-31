//////////////////////////////////////////////////////////////////////////////
//
// File     :  GpColl.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains GPG's STL-style collections.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GPCOLL_H
#define __GPCOLL_H

//////////////////////////////////////////////////////////////////////////////

#include "GpCore.h"
#include "gpmem_new_off.h"

#include <iterator>
#include <functional>

// define to 1 to enable bounds checking on fast vectors
#define FAST_VECTOR_BOUNDS_CHECKING 0

namespace stdx  {  // begin of namespace stdx

#pragma pack( push, 4 )

//////////////////////////////////////////////////////////////////////////////
// class mallocator declaration

class mallocator
{
public:
	template <typename T>
	T* allocate( size_t num, T*, int extra )
	{
#		if GPMEM_DBG_NEW
		return ( allocate( num, (T*)0, extra, __FILE__, __LINE__ ) );
#		else // GPMEM_DBG_NEW
		return ( (num > 0) ? (T*)::gpmalloc( (num * sizeof( T )) + extra ) : NULL );
#		endif // GPMEM_DBG_NEW
	}

#	if GPMEM_DBG_NEW
	template <typename T>
	T* allocate( size_t num, T*, int extra, const char* file, int line )
	{
		return ( (num > 0) ? (T*)::gpmalloc( (num * sizeof( T )) + extra, file, line ) : NULL );
	}
#	endif // GPMEM_DBG_NEW

	template <typename T>
	T* reallocate( T* mem, size_t num, int extra )
	{
#		if GPMEM_DBG_NEW
		return ( reallocate( mem, num, extra, __FILE__, __LINE__ ) );
#		else // GPMEM_DBG_NEW
		return ( (T*)::gprealloc( mem, (num * sizeof( T )) + extra ) );
#		endif // GPMEM_DBG_NEW
	}

#	if GPMEM_DBG_NEW
	template <typename T>
	T* reallocate( T* mem, size_t num, int extra, const char* file, int line )
	{
		return ( (T*)::gprealloc( mem, (num * sizeof( T)) + extra, file, line ) );
	}
#	endif // GPMEM_DBG_NEW

	template <typename T>
	void deallocate( T* mem )
	{
		::gpfree( mem );
	}
};

//////////////////////////////////////////////////////////////////////////////
// class st_pool_allocator declaration

// this is a non-thread safe (single-threaded) allocator. it inherits from
// mallocator because in debug builds we simply use the global pool. anyway
// this allocator uses one of the single-threaded shared memory pools to do its
// allocations. as such it completely eliminates the overhead from critical
// locks as it relies upon the owner of the collection ensuring serialized
// access to the heap.

class st_pool_allocator : public mallocator
{
#	if !GP_DEBUG

public:

	st_pool_allocator( void );

	template <typename T>
	T* allocate( size_t num, T*, int extra )
	{
		return ( (num > 0) ? (T*)pool_alloc( (num * sizeof( T )) + extra ) : NULL );
	}

	template <typename T>
	T* reallocate( T* mem, size_t num, int extra )
	{
		return ( (T*)pool_realloc( mem, (num * sizeof( T )) + extra ) );
	}

	template <typename T>
	void deallocate( T* mem )
	{
		pool_free( mem );
	}

private:

	// helpers
	void* pool_alloc  ( size_t size );
	void* pool_realloc( void* mem, size_t size );
	void  pool_free   ( void* mem );

	// can't imagine this many threads will need their own pools
	enum  {  MAX_POOLS = 10  };

	// tracks game info end of a memory pool
	struct PoolInfo
	{
		void* m_PoolId;
		DWORD m_OwnerThreadId;

		PoolInfo( void )
		{
			m_PoolId = NULL;
			m_OwnerThreadId = 0;
		}
	};

	// index to the pool i use
	int m_AllocatorIndex;

	// all available pools
	static PoolInfo ms_Pools[ MAX_POOLS ];

#	endif // !GP_DEBUG
};

//////////////////////////////////////////////////////////////////////////////
// class fast_vector <VALUE, ALLOC> declaration

// $ optimizations: write a specialized BYTE version that uses memcpy when
//   possible rather than for loops.

// $ optimizations: add support for an end sentry that can be used by the
//   sorted_vector in conjunction with sedgesort. the end sentry should be
//   stored at m_End, and capacity should always be one greater than size().

// $ note: this class is not meant to be used on types that keep pointers to
//   themselves stored inside of themselves. memory will be moved around and
//   realloc'd so these pointers would be invalidated.

// $ general type requirements: need public default ctor, copy ctor, dtor, and
//   operator =.

// $ optimizations: get rid of all internal calls to size() and use a
//   size_bytes() instead that avoids the idiv and does imul's to put things in
//   byte space rather than sizeof( T ) space.

// $ note: the stuff marked with "$$ alloc" is working around the problem with
//   derive-from-0-size. if you do get_allocator() = other.get_allocator(),
//   for some reason it actually screws up the object itself even though nothing
//   should happen (because it's 0 size). so until i find time to sort this all
//   out, this code is commented out.

template <typename VALUE, typename ALLOC = mallocator>
class fast_vector : protected ALLOC
{
public:

// Types.

	// our own types
	typedef fast_vector <VALUE, ALLOC> this_t;
	typedef VALUE value_t;
	typedef ALLOC alloc_t;

	// these are to make STL happy
	typedef VALUE  value_type;
	typedef ALLOC  allocator_type;
	typedef size_t size_type;

// Iterators.

#	if FAST_VECTOR_BOUNDS_CHECKING

	// $$$$ have each iter refcount the coll so we can assert if attempting
	//      to modify the coll with outstanding iters

	// bounds checking iterators
	class iterator;
	class const_iterator : public std::iterator <std::random_access_iterator_tag, value_t>
	{
	public:
		const_iterator( void )									{  }
		const_iterator( const const_iterator& other )			: m_Iter( other.m_Iter), m_Owner( other.m_Owner )  {  check_bounds();  }
		const_iterator( value_t* ptr, const this_t* owner )		: m_Iter( ptr ),  m_Owner( owner )  {  check_bounds();  }

		const value_t& operator *  ( void ) const				{  check_bounds();  return ( *m_Iter );  }
		const value_t* operator -> ( void ) const				{  check_bounds();  return (  m_Iter );  }

		const value_t& operator [] ( int num ) const			{  return ( *(*this + num) );  }

		const_iterator& operator ++ ( void )					{  ++m_Iter;  check_bounds();  return ( *this );  }
		const_iterator  operator ++ ( int )						{  const_iterator tmp( *this );  ++m_Iter;  return ( tmp );  }
		const_iterator& operator -- ( void )					{  --m_Iter;  check_bounds();  return ( *this );  }
		const_iterator  operator -- ( int )						{  const_iterator tmp( *this );  --m_Iter;  return ( tmp );  }

		const_iterator& operator += ( size_t num )				{  m_Iter += num;  check_bounds();  return ( *this );  }
		const_iterator& operator -= ( size_t num )				{  m_Iter -= num;  check_bounds();  return ( *this );  }

		const_iterator operator + ( size_t num ) const			{  const_iterator tmp = *this;  return ( tmp += num );  }
		const_iterator operator - ( size_t num ) const			{  const_iterator tmp = *this;  return ( tmp -= num );  }

		size_t operator - ( const const_iterator& other ) const	{  gpassert( m_Owner == other.m_Owner );  return ( m_Iter - other.m_Iter );  }
		size_t operator - ( const value_t* other ) const		{  return ( m_Iter - other );  }

		bool operator <  ( const const_iterator& other ) const	{  gpassert( m_Owner == other.m_Owner );  return ( m_Iter <  other.m_Iter );  }
		bool operator <= ( const const_iterator& other ) const	{  gpassert( m_Owner == other.m_Owner );  return ( m_Iter <= other.m_Iter );  }

		bool operator >  ( const const_iterator& other ) const	{  gpassert( m_Owner == other.m_Owner );  return ( m_Iter >  other.m_Iter );  }
		bool operator >= ( const const_iterator& other ) const	{  gpassert( m_Owner == other.m_Owner );  return ( m_Iter >= other.m_Iter );  }

		bool operator == ( const const_iterator& other ) const	{  gpassert( m_Owner == other.m_Owner );  return ( m_Iter == other.m_Iter );  }
		bool operator != ( const const_iterator& other ) const	{  gpassert( m_Owner == other.m_Owner );  return ( m_Iter != other.m_Iter );  }

		const this_t* get_owner( void ) const					{  return ( m_Owner );  }

		void check_bounds( const this_t* owner = NULL ) const
		{
			gpassert( (m_Owner != NULL) && ((owner == NULL) || (owner == m_Owner)) );
			gpassert( m_Iter >= m_Owner->m_Begin );
			gpassert( m_Iter <= m_Owner->m_End );
		}

	protected:

		value_t* m_Iter;
		const this_t* m_Owner;
	};

	class iterator : public const_iterator {
	public:
		iterator( void )										{  }
		iterator( value_t* ptr, const this_t* owner )			: const_iterator( ptr, owner )  {  }

		value_t& operator *  ( void ) const						{  check_bounds();  return ( *m_Iter );  }
		value_t* operator -> ( void ) const						{  check_bounds();  return (  m_Iter );  }

		iterator& operator ++ ( void )							{  ++m_Iter;  check_bounds();  return ( *this );  }
		iterator  operator ++ ( int )							{  iterator tmp( *this );  ++m_Iter;  return ( tmp );  }
		iterator& operator -- ( void )							{  --m_Iter;  check_bounds();  return ( *this );  }
		iterator  operator -- ( int )							{  iterator tmp( *this );  --m_Iter;  return ( tmp );  }

		iterator& operator += ( size_t num )					{  m_Iter += num;  check_bounds();  return ( *this );  }
		iterator& operator -= ( size_t num )					{  m_Iter -= num;  check_bounds();  return ( *this );  }

		iterator operator + ( size_t num ) const				{  iterator tmp = *this;  return ( tmp += num );  }
		iterator operator - ( size_t num ) const				{  iterator tmp = *this;  return ( tmp -= num );  }

		size_t operator - ( const iterator& other ) const		{  gpassert( m_Owner == other.m_Owner );  return ( m_Iter - other.m_Iter );  }
		size_t operator - ( const value_t* other ) const		{  return ( m_Iter - other );  }
	};

	friend const_iterator;

#	else // FAST_VECTOR_BOUNDS_CHECKING

	// plain "iterators"
	typedef const value_t* const_iterator;
	typedef value_t* iterator;

#	endif // FAST_VECTOR_BOUNDS_CHECKING

	typedef std::reverse_iterator <const_iterator, value_t, const value_t&, const value_t*, size_t>
			const_reverse_iterator;
	typedef std::reverse_iterator <iterator, value_t, value_t&, value_t*, size_t>
			reverse_iterator;

// Ctor/dtor.

	explicit fast_vector( const alloc_t& alloc = alloc_t() )
		: alloc_t( alloc ), m_Begin( NULL ), m_End( NULL )  {  GPDEBUG_ONLY( m_ReserveLocked = false );  }

	explicit fast_vector( size_t num, const alloc_t& alloc = alloc_t() )
		: alloc_t( alloc )
	{
		GPDEBUG_ONLY( m_ReserveLocked = false );

		if ( num > 0 )
		{
			private_alloc( num );
			private_fill_uninitialized( m_Begin, m_End );
		}
		else
		{
			m_Begin = NULL;
			m_End   = NULL;
		}
	}

	fast_vector( size_t num, const value_t& value, const alloc_t& alloc = alloc_t() )
		: alloc_t( alloc )
	{
		GPDEBUG_ONLY( m_ReserveLocked = false );

		if ( num > 0 )
		{
			private_alloc( num );
			private_fill_uninitialized( m_Begin, m_End, value );
		}
		else
		{
			m_Begin = NULL;
			m_End   = NULL;
		}
	}

	fast_vector( const this_t& other )
		: alloc_t( other )
	{
		GPDEBUG_ONLY( m_ReserveLocked = false );

		if ( !other.empty() )
		{
			private_alloc( other.size() );
			private_fill_uninitialized_iter( m_Begin, m_End, other.m_Begin );
		}
		else
		{
			m_Begin = NULL;
			m_End   = NULL;
		}
	}

	template <typename ITER>
	fast_vector ( ITER ib, ITER ie, const alloc_t& alloc = alloc_t() )
		: alloc_t( alloc )
	{
		GPDEBUG_ONLY( m_ReserveLocked = false );

		if ( ib != ie )
		{
			private_alloc( /*std::distance( ib, ie )*/ ie - ib );
			private_fill_uninitialized_iter( m_Begin, m_End, ib );
		}
		else
		{
			m_Begin = NULL;
			m_End   = NULL;
		}
	}

   ~fast_vector( void )
	{
		private_destroy_and_free();
	}

	const alloc_t& get_allocator( void ) const				{  return ( *this );  }
	alloc_t&       get_allocator( void )					{  return ( *this );  }

// Memory management.

	void destroy( void )
	{
		private_destroy_and_free();
		m_Begin = m_End = NULL;
	}

	void clear( void )
	{
		private_destroy( m_Begin, m_End );
		m_End = m_Begin;
	}

	value_t* release( void*& freeThisPtr )
	{
		value_t* mem = m_Begin;
		freeThisPtr = private_get_memory_base();
		m_Begin = m_End = NULL;
		return ( mem );
	}

	mem_ptr release_as_mem( void*& freeThisPtr )
	{
		mem_ptr mem( m_Begin, size() * sizeof( value_t ) );
		freeThisPtr = private_get_memory_base();
		m_Begin = m_End = NULL;
		return ( mem );
	}

	void reserve( size_t num )
	{
		if ( capacity() < num )
		{
			private_reserve( num );
		}
	}

	void reserve_extra( size_t num )
	{
		reserve( size() + num );
	}

	void reserve_exact( size_t num )
	{
		if ( capacity() != num )
		{
			private_reserve( num );
		}
	}

	void reserve_exact( void )
	{
		reserve_exact( size() );
	}

	void reserve_lock( size_t num )
	{
		// $ this function "locks" the vector at a certain size. it will realloc
		//   the reserve up or down to fit, and set a bit so further allocs will
		//   assert on attempting to grow it.

#		if GP_DEBUG
		m_ReserveLocked = false;
#		endif // GP_DEBUG

		reserve_exact( num );

#		if GP_DEBUG
		m_ReserveLocked = true;
#		endif // GP_DEBUG
	}

	void reserve_lock( void )
	{
		reserve_lock( size() );
	}

	void resize( size_t num, const value_t& value )
	{
		size_t sz = size();
		if ( sz < num )
		{
			private_grow( num - sz );
			private_fill_uninitialized( m_Begin + sz, m_End, value );
		}
		else if ( num < sz )
		{
			private_destroy_end( sz - num );
		}
	}

	void resize( size_t num )
	{
		// $ this initializes with default ctor

		size_t sz = size();
		if ( sz < num )
		{
			private_grow( num - sz );
			private_fill_uninitialized( m_Begin + sz, m_End );
		}
		else if ( num < sz )
		{
			private_destroy_end( sz - num );
		}
	}

	void uninitialized_resize( size_t num )
	{
		// $ this does not initialize and leaves the memory random

		size_t sz = size();
		if ( sz < num )
		{
			private_grow( num - sz );
		}
		else if ( num < sz )
		{
			private_destroy_end( sz - num );
		}
	}

	void swap( this_t& other )
	{
		// $$ alloc

//		if ( get_allocator() == other.get_allocator() )
//		{
			std::swap( m_Begin, other.m_Begin );
			std::swap( m_End,   other.m_End   );
//		}
//		else
//		{
//			this_t tmp;
//			swap( tmp );
//			*this = other;
//			other = tmp;
//		}
	}

	// $ special note: size() and capacity() both call idiv!

	size_t capacity( void ) const							{  return ( private_get_reserve_end() - m_Begin );  }
	size_t size    ( void ) const							{  gpassert( m_End >= m_Begin );  return ( m_End - m_Begin );  }
	bool   empty   ( void ) const							{  gpassert( m_End >= m_Begin );  return ( m_Begin == m_End );  }

	size_t capacity_bytes( void ) const						{  return ( (const BYTE*)private_get_reserve_end() - (const BYTE*)m_Begin );  }
	size_t size_bytes    ( void ) const						{  return ( (const BYTE*)m_End - (const BYTE*)m_Begin );  }

	friend void swap( this_t& l, this_t& r )				{  l.swap( r );  }

// Access.

#	if FAST_VECTOR_BOUNDS_CHECKING
#	define MAKE_ITER( x )   iterator( x, this )
#	define MAKE_CITER( x )  const_iterator( x, this )
#	else // FAST_VECTOR_BOUNDS_CHECKING
#	define MAKE_ITER( x )   (x)
#	define MAKE_CITER( x )  (x)
#	endif // FAST_VECTOR_BOUNDS_CHECKING

	// iterators
	iterator               begin ( void )					{  return ( MAKE_ITER ( m_Begin ) );  }
	const_iterator         begin ( void ) const				{  return ( MAKE_CITER( m_Begin ) );  }
	iterator               end   ( void )					{  return ( MAKE_ITER ( m_End   ) );  }
	const_iterator         end   ( void ) const				{  return ( MAKE_CITER( m_End   ) );  }
	reverse_iterator       rbegin( void )					{  return ( reverse_iterator      ( end  () ) );  }
	const_reverse_iterator rbegin( void ) const				{  return ( const_reverse_iterator( end  () ) );  }
	reverse_iterator       rend  ( void )					{  return ( reverse_iterator      ( begin() ) );  }
	const_reverse_iterator rend  ( void ) const				{  return ( const_reverse_iterator( begin() ) );  }

	// front and back elements
	value_t&       front( void )							{  gpassert( !empty() );  return ( *m_Begin );  }
	const value_t& front( void ) const						{  gpassert( !empty() );  return ( *m_Begin );  }
	value_t&       back ( void )							{  gpassert( !empty() );  return ( *(m_End - 1) );  }
	const value_t& back ( void ) const						{  gpassert( !empty() );  return ( *(m_End - 1) );  }

	// direct access
	value_t&       operator [] ( size_t i )					{  gpassert( i < size() );  return ( *(m_Begin + i) );  }
	const value_t& operator [] ( size_t i ) const			{  gpassert( i < size() );  return ( *(m_Begin + i) );  }

// Back.

	iterator push_back( const value_t& value )
	{
		private_grow( 1 );
		value_t* entry = &back();
		new ( entry ) value_t( value );
		return ( MAKE_ITER( entry ) );
	}

	iterator push_back( void )
	{
		private_grow( 1 );
		value_t* entry = &back();
		new ( entry ) value_t();
		return ( MAKE_ITER( entry ) );
	}

	template <typename ITER>
	iterator push_back_raw( ITER ib, ITER ie )
	{
		size_t count = ie - ib;
		private_grow( count );
		value_t* ob = m_End - count;
		::memcpy( ob, &*ib, count * sizeof( VALUE ) );
		return ( MAKE_ITER( ob ) );
	}

	void pop_back( void )
	{
		back().~VALUE();
		--m_End;
	}

	void pop_back( size_t count )
	{
		erase( end() - count, end() );
	}

	value_t pop_back_t( void )
	{
		value_t temp = back();
		pop_back();
		return ( temp );
	}

// Assignment.

	template <typename ITER>
	void assign( ITER ib, ITER ie )
	{
		for ( value_t* i = m_Begin ; ; ++i, ++ib )
		{
			if ( i == m_End )
			{
				if ( ib != ie )
				{
					size_t extra = /*std::distance( ib, ie )*/ ie - ib;
					private_grow( extra );
					private_fill_uninitialized_iter( m_End - extra, m_End, ib );
				}
				break;
			}
			else if ( ib == ie )
			{
				private_destroy_end( m_End - i );
				break;
			}

			*i = *ib;
		}
	}

	template <typename ITER>
	void assign_new( ITER ib, ITER ie )
	{
		clear();
		private_grow( ie - ib );
		private_fill_uninitialized_iter( m_Begin, m_End, ib );
	}

	void assign( size_t num )
	{
		private_fill_initialized( m_Begin, m_Begin + min( size(), num ) );
		resize( num );
	}

	void assign( size_t num, const value_t& value )
	{
		private_fill_initialized( m_Begin, m_Begin + min( size(), num ), value );
		resize( num, value );
	}

// Searching.

	template <typename T, typename PRED>
	iterator linear_find( const T& key, const PRED& pred )
	{
		for ( iterator i = begin(), e = end() ; i != e ; ++i )
		{
			if ( pred( key, *i ) )
			{
				break;
			}
		}

		return ( i );
	}

	template <typename T, typename PRED>
	const_iterator linear_find( const T& key, const PRED& pred ) const
	{
		return ( ccast <this_t*> ( this )->linear_find( key, pred ) );
	}

	template <typename T>
	iterator linear_find( const T& key )
	{
		return ( linear_find( key, stdx::equal_to() ) );
	}

	template <typename T>
	const_iterator linear_find( const T& key ) const
	{
		return ( linear_find( key, stdx::equal_to() ) );
	}

// Insertion.

	iterator insert( iterator where )
	{
		return ( insert_count( where, (size_t)1 ) );
	}

	iterator insert( iterator where, const value_t& value )
	{
		return ( insert_count( where, (size_t)1, value ) );
	}

	iterator insert_count( iterator where, size_t count )
	{
#		if FAST_VECTOR_BOUNDS_CHECKING
		where.check_bounds( this );
#		endif // FAST_VECTOR_BOUNDS_CHECKING

		int offset = where - m_Begin;
		if ( count != 0 )
		{
			private_grow( count );
			value_t* ob = m_Begin + offset;
			value_t* oe = ob + count;
			if ( m_End != oe )
			{
				::memmove( oe, ob, sizeof( value_t ) * (m_End - oe) );
			}
			private_fill_uninitialized( ob, oe );
		}
		return ( MAKE_ITER( m_Begin + offset ) );
	}

	iterator insert_count( iterator where, size_t count, const value_t& value )
	{
#		if FAST_VECTOR_BOUNDS_CHECKING
		where.check_bounds( this );
#		endif // FAST_VECTOR_BOUNDS_CHECKING

		int offset = where - m_Begin;
		if ( count != 0 )
		{
			private_grow( count );
			value_t* ob = m_Begin + offset;
			value_t* oe = ob + count;
			if ( m_End != oe )
			{
				::memmove( oe, ob, sizeof( value_t ) * (m_End - oe) );
			}
			private_fill_uninitialized( ob, oe, value );
		}
		return ( MAKE_ITER( m_Begin + offset ) );
	}

	template <typename ITER>
	iterator insert( iterator where, ITER ib, ITER ie )
	{
#		if FAST_VECTOR_BOUNDS_CHECKING
		where.check_bounds( this );
#		endif // FAST_VECTOR_BOUNDS_CHECKING

		int offset = where - m_Begin;
		size_t count = /*std::distance( ib, ie )*/ ie - ib;
		if ( count != 0 )
		{
			private_grow( count );
			value_t* ob = m_Begin + offset;
			value_t* oe = ob + count;
			if ( m_End != oe )
			{
				::memmove( oe, ob, sizeof( value_t ) * (m_End - oe) );
			}
			private_fill_uninitialized_iter( ob, oe, ib );
		}
		return ( MAKE_ITER( m_Begin + offset ) );
	}

// Sorted insertion.

	template <typename PRED>
	std::pair <iterator, bool> sorted_insert_single( const value_t& value, const PRED& pred )
	{
		iterator iter = std::upper_bound( begin(), end(), value, pred );
		bool success = (iter == begin()) || pred( *(iter - 1), value );
		if ( success )
		{
			iter = insert( iter, value );
		}
		else
		{
			--iter;
		}
		return ( std::pair <iterator, bool> ( iter, success ) );
	}

	std::pair <iterator, bool> sorted_insert_single( const value_t& value )
	{
		return ( sorted_insert_single( value, std::less <value_t> () ) );
	}

	template <typename VALUE, typename PRED>
	std::pair <iterator, bool> find_sorted_insert_single( const VALUE& value, const PRED& pred )
	{
		iterator iter = std::upper_bound( begin(), end(), value, pred );
		bool success = (iter == begin()) || pred( *(iter - 1), value );
		if ( !success )
		{
			--iter;
		}
		return ( std::pair <iterator, bool> ( iter, success ) );
	}

	template <typename VALUE>
	std::pair <iterator, bool> find_sorted_insert_single( const VALUE& value )
	{
		return ( find_sorted_insert_single( value, std::less <value_t> () ) );
	}

	template <typename PRED>
	iterator sorted_insert_multi( const value_t& value, const PRED& pred )
	{
		return ( insert( std::upper_bound( begin(), end(), value, pred ), value ) );
	}

	iterator sorted_insert_multi( const value_t& value )
	{
		return ( sorted_insert_multi( value, std::less <value_t> () ) );
	}

	template <typename VALUE, typename PRED>
	iterator find_sorted_insert_multi( const VALUE& value, const PRED& pred )
	{
		return ( std::upper_bound( begin(), end(), value, pred ) );
	}

	template <typename VALUE>
	iterator find_sorted_insert_multi( const VALUE& value )
	{
		return ( find_sorted_insert_multi( value, std::less <value_t> () ) );
	}

	template <typename PRED>
	iterator sorted_reinsert_multi( iterator which, const value_t& value, const PRED& pred )
	{
		// $ this assumes that the collection is already sorted, and will take
		//   an existing value, reassign it, and reinsert it into the collection
		//   where it should go based on the predicate.

#		if FAST_VECTOR_BOUNDS_CHECKING
		which.check_bounds( this );
#		endif // FAST_VECTOR_BOUNDS_CHECKING

		// first figure out where it should go
		iterator where = std::lower_bound( begin(), end(), value, pred );
#		if FAST_VECTOR_BOUNDS_CHECKING
		where.check_bounds( this );
#		endif // FAST_VECTOR_BOUNDS_CHECKING

		// only bother if it's different (use ptr compares to avoid bounds
		// checking asserts)
		if ( (where != which) && (&*where != (&*which + 1)) )
		{
			// delete old
			(&*which)->~VALUE();

			// shift memory over and store new entry
			if ( which < where )
			{
				iterator next( which + 1 );
				::memmove( &*which, &*next, (BYTE*)&*where - (BYTE*)&*next );
				--where;
			}
			else
			{
				iterator next( where + 1 );
				if ( next == which )
				{
					::memcpy( &*which, &*where, sizeof( value_t ) );
				}
				else
				{
					::memmove( &*where, &*next, (BYTE*)&*which - (BYTE*)&*next );
				}
			}

			// copy it in
			new ( &*where ) value_t( value );
		}
		else
		{
			// just take it
			*where = value;
		}

		// done
		return ( where );
	}

	void sorted_reinsert_multi( iterator which, const value_t& value )
	{
		return ( sorted_reinsert_multi( which, value, std::less <value_t> () ) );
	}

// Erasing.

	iterator erase( iterator where )
	{
#		if FAST_VECTOR_BOUNDS_CHECKING
		where.check_bounds( this );
#		endif // FAST_VECTOR_BOUNDS_CHECKING

		(&*where)->~VALUE();
		value_t* where2 = &*(where + 1);
		if ( m_End != where2 )
		{
			::memmove( &*where, where2, sizeof( value_t ) * (m_End - where2) );
		}
		--m_End;
		return ( where );
	}

	iterator erase( iterator ib, iterator ie )
	{
#		if FAST_VECTOR_BOUNDS_CHECKING
		ib.check_bounds( this );
		ie.check_bounds( this );
#		endif // FAST_VECTOR_BOUNDS_CHECKING

		if ( ib != ie )
		{
			private_destroy( &*ib, &*ie );
			size_t endSize = m_End - &*ie;
			if ( endSize != 0 )
			{
				::memmove( &*ib, &*ie, sizeof( value_t ) * endSize );
			}
			m_End = &*ib + endSize;
		}
		return ( ib );
	}

// Sorting.

	template <typename PRED>
	void sort( const PRED& predicate )
	{
		std::sort( m_Begin, m_End, predicate );
	}

	void sort( void )
	{
		std::sort( m_Begin, m_End, std::less <value_t> () );
	}

// Operators.

	this_t& operator = ( const this_t& other )
	{
		if ( this != &other )
		{
			assign( other.m_Begin, other.m_End );

			// $$ alloc
//			get_allocator() = other.get_allocator();
		}
		return ( *this );
	}

#	undef MAKE_ITER
#	undef MAKE_CITER

private:

// Private utility.

	// reserve end is right before the begin pointer
	value_t*& private_get_reserve_end( void )
	{
		return ( m_Begin ? *((value_t**)m_Begin - 1) : m_Begin );
	}

	// reserve end is right before the begin pointer
	const value_t* private_get_reserve_end( void ) const
	{
		return ( ccast <this_t*> ( this )->private_get_reserve_end() );
	}

	// get base of memory
	value_t** private_get_memory_base( void ) const
	{
		return ( m_Begin ? ((value_t**)m_Begin - 1) : NULL );
	}

	// simply allocate exactly how much memory is wanted
	void private_alloc( size_t num )
	{
		m_Begin = (value_t*)((value_t**)allocate( num, (value_t*)NULL, sizeof( value_t* ) ) + 1);
		m_End = m_Begin + num;
		private_get_reserve_end() = m_End;
	}

	// iterate through the given iters and blow them away
	void private_destroy( value_t* ib, value_t* ie )
	{
		gpassert( ib <= ie );
		for ( ; ib != ie ; ++ib )
		{
			ib->~VALUE();
		}
	}

	// destroy just the end objects here
	void private_destroy_end( size_t num )
	{
		gpassert( (num > 0) && (num <= size()) );
		private_destroy( m_End - num, m_End );
		m_End -= num;
	}

	// destroy everything and also free the memory
	void private_destroy_and_free( void )
	{
		if ( m_Begin != NULL )
		{
			private_destroy( m_Begin, m_End );
			deallocate( private_get_memory_base() );
		}
	}

	// fill between the given iters with default ctor
	void private_fill_initialized( value_t* ib, value_t* ie )
	{
		for ( ; ib != ie ; ++ib )
		{
			*ib = value_t();
		}
	}

	// fill between the given iters with copies of given instance
	void private_fill_initialized( value_t* ib, value_t* ie, const value_t& value )
	{
		for ( ; ib != ie ; ++ib )
		{
			*ib = value;
		}
	}

	// fill between the given iters with the given input iter
	template <typename ITER>
	void private_fill_initialized_iter( value_t* ib, value_t* ie, ITER in )
	{
		for ( ; ib != ie ; ++ib, ++in )
		{
			*ib = *in;
		}
	}

	// fill between the given iters (pointing to uninitialized objects) with default ctor
	void private_fill_uninitialized( value_t* ib, value_t* ie )
	{
		for ( ; ib != ie ; ++ib )
		{
			new ( ib ) value_t();
		}
	}

	// fill between the given iters (pointing to uninitialized objects) with copies of given instance
	void private_fill_uninitialized( value_t* ib, value_t* ie, const value_t& value )
	{
		for ( ; ib != ie ; ++ib )
		{
			new ( ib ) value_t( value );
		}
	}

	// fill between the given iters (pointing to uninitialized objects) with the given input iter
	template <typename ITER>
	void private_fill_uninitialized_iter( value_t* ib, value_t* ie, ITER in )
	{
		for ( ; ib != ie ; ++ib, ++in )
		{
			new ( ib ) value_t( *in );
		}
	}

	// grow the allocation by the given number of elements
	void private_grow( size_t extra )
	{
		// $ grow by 50% at a time, or by 'extra', whichever is greater. note
		//   that we're working completely in byte space to avoid the idiv
		//   that size() incurs, as well as minimizing imul's.

		size_t sz = size_bytes();
		size_t ex = extra * sizeof( value_t );

		if ( capacity_bytes() < (sz + ex) )
		{
			// idiv is ok here, reallocs are very rare (quantify sez that about
			// 1% of the time we call push_back() on avg it will realloc)
			size_t count = size();
			private_reserve( count + max_t( extra, count / 2 ) );
		}

		m_End = (value_t*)((BYTE*)m_End + ex);
	}

	// reserve some more memory
	void private_reserve( size_t num )
	{
		gpassert( num != capacity() );
		gpassertm( num >= size(), "Attempting to realloc-down below existing size!" );
		gpassertm( !m_ReserveLocked, "Attempting to realloc a locked vector!" );

		value_t* newMem = (value_t*)((value_t**)reallocate( (value_t*)private_get_memory_base(), num, sizeof( value_t* ) ) + 1);
		if ( newMem != m_Begin )
		{
			m_End = newMem + (m_End - m_Begin);
			m_Begin = newMem;
		}
		private_get_reserve_end() = m_Begin + num;
	}

// Private data.

	// data
	value_t* m_Begin;
	value_t* m_End;

	// special
#	if GP_DEBUG
	bool m_ReserveLocked;
#	endif // GP_DEBUG
};

//////////////////////////////////////////////////////////////////////////////
// class sorted_array <KEY, VALUE, KEYFN, PRED, ALLOC> declaration

template <typename PRED, typename KEYFN>
struct sorted_array_pred_helper
{
	const PRED& m_Predicate;

	sorted_array_pred_helper( const PRED& predicate )
		: m_Predicate( predicate )  {  }

	template <typename T, typename U>
	bool operator () ( const T& l, const U& r ) const
	{
		return ( m_Predicate( KEYFN()( l ), KEYFN()( r ) ) );
	}
};

template <typename KEY, typename VALUE, typename KEYFN, typename PRED, typename COLL = fast_vector <VALUE> >
class sorted_array : protected COLL, private PRED
{
public:

// Types.

	typedef COLL  coll_t;
	typedef VALUE value_t;
	typedef KEY   key_t;
	typedef PRED  pred_t;
	typedef sorted_array <key_t, value_t, KEYFN, pred_t, coll_t> this_t;
	using COLL::alloc_t;

	using COLL::iterator;
	using COLL::const_iterator;
	using COLL::reverse_iterator;
	using COLL::const_reverse_iterator;

	using COLL::get_allocator;
	using COLL::destroy;
	using COLL::clear;
	using COLL::reserve;
	using COLL::reserve_extra;
	using COLL::reserve_exact;
	using COLL::reserve_lock;
	using COLL::uninitialized_resize;

	void swap( this_t& other )
	{
		coll_t::swap( other );
		std::swap( get_predicate(), other.get_predicate() );
	}

	using COLL::capacity;
	using COLL::size;
	using COLL::empty;

	friend void swap( this_t& l, this_t& r )				{  l.swap( r );  }

	using COLL::begin;
	using COLL::end;
	using COLL::rbegin;
	using COLL::rend;
	using COLL::front;
	using COLL::back;
	using COLL::pop_back;
	using COLL::pop_back_t;
	using COLL::find_sorted_insert_single;
	using COLL::find_sorted_insert_multi;
	using COLL::operator [];
	using COLL::erase;

	this_t& operator = ( const this_t& other )
	{
		pred_t::operator = ( other );
		coll_t::operator = ( other );
		return ( *this );
	}

// Ctor/dtor.

	explicit sorted_array( const pred_t& predicate = pred_t(), const alloc_t& alloc = alloc_t() )
		: pred_t( predicate ), coll_t( alloc )  {  }

	template <typename ITER>
	sorted_array( ITER ib, ITER ie, const pred_t& predicate = pred_t(), const alloc_t& alloc = alloc_t() )
		: pred_t( predicate ), coll_t( ib, ie, alloc )  {  sort();  }

	sorted_array( const this_t& other )
		: pred_t( other ), coll_t( other )  {  }

	const pred_t& get_predicate( void ) const				{  return ( *this );  }

// Insertion.

	// $$$ support uninitialized insertion - it just finds a spot for the entry
	//     then allocates a slot and returns an iterator, but does not fill the
	//     contents with a passed in value. only pass in the key. then the
	//     caller is responsible for filling the slot. this is necessary to
	//     avoid double copy ctor's on dumb types with heavy copy ctors but
	//     cheap default ctors.

	iterator unsorted_push_back_generic( const value_t& value )
	{
		return ( push_back( value ) );
	}

	iterator unsorted_push_back_generic( void )
	{
		return ( push_back() );
	}

	iterator unsorted_insert_generic( iterator where, const value_t& value )
	{
		return ( insert( where, value ) );
	}

	iterator unsorted_insert_generic( iterator where )
	{
		return ( insert( where ) );
	}

	std::pair <iterator, bool> sorted_insert_single( const value_t& value )
	{
		iterator iter = upper_bound( KEYFN()( value ) );
		bool success = (iter == begin()) || get_predicate()( KEYFN()( *(iter - 1) ), KEYFN()( value ) );
		if ( success )
		{
			iter = insert( iter, value );
		}
		else
		{
			--iter;
		}
		return ( std::pair <iterator, bool> ( iter, success ) );
	}

	iterator sorted_insert_multi( const value_t& value )
	{
		return ( insert( upper_bound( KEYFN()( value ) ), value ) );
	}

	template <typename ITER>
	void unsorted_push_back_generic( ITER ib, ITER ie )
	{
		for ( ; ib != ie ; ++ib )
		{
			unsorted_push_back_generic( *ib );
		}
	}

	template <typename ITER>
	void sorted_insert_single( ITER ib, ITER ie )
	{
		for ( ; ib != ie ; ++ib )
		{
			sorted_insert_single( *ib );
		}
	}

	template <typename ITER>
	void sorted_insert_multi( ITER ib, ITER ie )
	{
		for ( ; ib != ie ; ++ib )
		{
			sorted_insert_multi( *ib );
		}
	}

	void sort( void )
	{
		coll_t::sort( sorted_array_pred_helper <pred_t, KEYFN> ( get_predicate() ) );
	}

	template <typename ALT_PRED>
	void resort( const ALT_PRED& pred )
	{
		coll_t::sort( sorted_array_pred_helper <ALT_PRED, KEYFN> ( pred ) );
	}

// Removal

	template <typename T>
	bool erase_single( const T& key )
	{
		iterator found = find( key );
		if ( found == end() )
		{
			return ( false );
		}
		erase( found );
		return ( true );
	}

	template <typename T>
	size_t erase_multi( const key_t& key )
	{
		std::pair <iterator, iterator> found = equal_range( key );
		size_t dist = found.second - found.first;
		erase( found.first, found.second );
		return ( dist );
	}

	template <typename ITER>
	size_t erase_single( ITER ib, ITER ie )
	{
		size_t count = 0;
		for ( ; ib != ie ; ++ib )
		{
			count += erase( *ib );
		}
		return ( count );
	}

	template <typename ITER>
	size_t erase_multi( ITER ib, ITER ie )
	{
		size_t count = 0;
		for ( ; ib != ie ; ++ib )
		{
			count += erase_multi( *ib );
		}
		return ( count );
	}

// Searching.

	template <typename T>
	iterator find( const T& key )
	{
		iterator found = lower_bound( key );
		if ( (found != end()) && get_predicate()( key, KEYFN()( *found ) ) )
		{
			found = end();
		}
		return ( found );
	}

	template <typename T>
	const_iterator find( const T& key ) const
	{
		return ( ccast <this_t*> ( this )->find( key ) );
	}

	template <typename T, typename ITER>
	iterator find( ITER ib, ITER ie, const T& key )
	{
		iterator found = lower_bound( ib, ie, key );
		if ( (found != ie) && get_predicate()( key, KEYFN()( *found ) ) )
		{
			found = ie;
		}
		return ( found );
	}

	template <typename T, typename ITER>
	const_iterator find( ITER ib, ITER ie, const T& key ) const
	{
		return ( ccast <this_t*> ( this )->find( ib, ie, key ) );
	}

	template <typename T, typename PRED>
	iterator linear_find( const T& key, const PRED& pred )
	{
		for ( iterator i = begin(), e = end() ; i != e ; ++i )
		{
			if ( pred( key, KEYFN()( *i ) ) )
			{
				break;
			}
		}

		return ( i );
	}

	template <typename T, typename PRED>
	const_iterator linear_find( const T& key, const PRED& pred ) const
	{
		return ( ccast <this_t*> ( this )->linear_find( key, pred ) );
	}

	template <typename T>
	iterator linear_find( const T& key )
	{
		return ( linear_find( key, stdx::equal_to() ) );
	}

	template <typename T>
	const_iterator linear_find( const T& key ) const
	{
		return ( linear_find( key, stdx::equal_to() ) );
	}

	template <typename T>
	size_t count_multi( const T& key ) const
	{
		std::pair <const_iterator, const_iterator> found = equal_range( key );
		return ( found.second - found.first );
	}

	template <typename T, typename ITER>
	ITER lower_bound( ITER ib, ITER ie, const T& key )
	{
		size_t num = ie - ib;
		while ( num > 0 )
		{
			size_t num2 = num / 2;
			ITER mid = ib + num2;
			if ( get_predicate()( KEYFN()( *mid ), key ) )
			{
				ib = ++mid;
				num -= num2 + 1;
			}
			else
			{
				num = num2;
			}
		}
		return ( ib );
	}

	template <typename T>
	iterator lower_bound( const T& key )
	{
		return ( lower_bound( begin(), end(), key ) );
	}

	template <typename T>
	const_iterator lower_bound( const T& key ) const
	{
		return ( lower_bound( begin(), end(), key ) );
	}

	template <typename T, typename ITER>
	ITER upper_bound( ITER ib, ITER ie, const T& key )
	{
		size_t num = ie - ib;
		while ( num > 0 )
		{
			size_t num2 = num / 2;
			ITER mid = ib + num2;
			if ( !get_predicate()( key, KEYFN()( *mid ) ) )
			{
				ib = ++mid;
				num -= num2 + 1;
			}
			else
			{
				num = num2;
			}
		}
		return ( ib );
	}

	template <typename T>
	iterator upper_bound( const T& key )
	{
		return ( upper_bound( begin(), end(), key ) );
	}

	template <typename T>
	const_iterator upper_bound( const T& key ) const
	{
		return ( upper_bound( begin(), end(), key ) );
	}


	template <typename T, typename ITER>
	std::pair <ITER, ITER> equal_range( ITER ib, ITER ie, const T& key )
	{
		size_t num = ie - ib;
		while ( num > 0 )
		{
			size_t num2 = num / 2;
			ITER mid = ib + num2;
			if ( get_predicate()( KEYFN()( *mid ), key ) )
			{
				ib = ++mid;
				num -= num2 + 1;
			}
			else if ( get_predicate()( key, KEYFN()( *mid ) ) )
			{
				num = num2;
			}
			else
			{
				ITER ib2 = lower_bound( ib, mid, key );
				ITER ie2 = upper_bound( ++mid, ib + num, key );
				return ( std::pair <ITER, ITER> ( ib2, ie2 ) );
			}
		}

		return ( std::pair <ITER, ITER> ( ib, ib ) );
	}

	template <typename T>
	std::pair <iterator, iterator> equal_range( const T& key )
	{
		return ( equal_range( begin(), end(), key ) );
	}

	template <typename T>
	std::pair <const_iterator, const_iterator> equal_range( const T& key ) const
	{
		return ( equal_range( begin(), end(), key ) );
	}

#	if GP_DEBUG
	void self_check( bool multi ) const
	{
		// $$ check the last element for MAX_INT for sedge

		if ( !empty() )
		{
			for ( iterator i = begin() + 1 ; i != end() ; ++i )
			{
				gpassert( !get_predicate()( KEYFN()( *i ), KEYFN()( *(i - 1) ) ) );
				if ( multi )
				{
					gpassert( !get_predicate()( KEYFN()( *(i - 1) ), KEYFN()( *i ) ) );
				}
			}
		}

	}
#	endif // GP_DEBUG
};

//////////////////////////////////////////////////////////////////////////////
// class linear_map <T> declaration

// $$$ write some docs!! algo complexity, speed, implementation, etc...

template <typename KEY, typename VALUE>
struct linear_map_keyfn : public std::unary_function <std::pair <const KEY, VALUE>, KEY>
{
	const KEY& operator () ( const VALUE& value ) const
	{
		return ( value.first );
	}
	const KEY& operator () ( const KEY& key ) const
	{
		return ( key );
	}
};

template <
		typename KEY,
		typename VALUE,
		typename PRED = std::less <KEY>,
		typename COLL = fast_vector <std::pair <KEY, VALUE> > >
class linear_map : protected sorted_array <
		KEY,
		std::pair <KEY, VALUE>,
		linear_map_keyfn <KEY, std::pair <KEY, VALUE> >,
		PRED,
		COLL>
{
public:

// Types.

	typedef KEY key_t;
	typedef VALUE value_t;
	typedef std::pair <KEY, VALUE> entry_t;
	typedef PRED pred_t;
	typedef COLL base_coll_t;
	typedef linear_map_keyfn <key_t, entry_t> keyfn_t;
	typedef sorted_array <key_t, entry_t, keyfn_t, pred_t, base_coll_t> coll_t;
	typedef coll_t::alloc_t alloc_t;
	typedef linear_map <key_t, value_t, pred_t, base_coll_t> this_t;
	typedef pred_t key_compare;

	// these are to make STL happy
	typedef key_t   key_type;
	typedef entry_t value_type;
	typedef alloc_t allocator_type;
	typedef size_t  size_type;

	class value_compare : public std::binary_function <entry_t, entry_t, bool>
	{
	public:
		bool operator () ( const entry_t& l, const entry_t& r ) const
		{
			return ( m_Predicate( l.first, r.first ) );
		}

	private:
		friend class this_t;

		value_compare( const pred_t& predicate )
			: m_Predicate( predicate )  {  }
		pred_t m_Predicate;
	};

	using coll_t::iterator;
	using coll_t::const_iterator;
	using coll_t::reverse_iterator;
	using coll_t::const_reverse_iterator;

	using coll_t::get_allocator;
	using coll_t::destroy;
	using coll_t::clear;
	using coll_t::reserve;
	using coll_t::reserve_extra;
	using coll_t::reserve_exact;
	using coll_t::reserve_lock;
	using coll_t::uninitialized_resize;
	using coll_t::swap;
	using coll_t::capacity;
	using coll_t::size;
	using coll_t::empty;

	friend void swap( this_t& l, this_t& r )				{  l.swap( r );  }

	using coll_t::begin;
	using coll_t::end;
	using coll_t::rbegin;
	using coll_t::rend;
	using coll_t::front;
	using coll_t::back;
	using coll_t::pop_back;
	using coll_t::pop_back_t;
	using coll_t::sort;
	using coll_t::resort;
	using coll_t::get_predicate;
	using coll_t::find;
	using coll_t::linear_find;
	using coll_t::lower_bound;
	using coll_t::upper_bound;
	using coll_t::equal_range;

#	if GP_DEBUG
	void self_check( void ) const
	{
		coll_t::self_check( false );
	}
#	endif // GP_DEBUG

// Ctor/dtor.

	explicit linear_map( const pred_t& predicate = pred_t(), const alloc_t& alloc = alloc_t() )
		: coll_t( predicate, alloc )  {  }

	template <typename ITER>
	linear_map( ITER ib, ITER ie, const pred_t& predicate = pred_t(), const alloc_t& alloc = alloc_t() )
		: coll_t( ib, ie, predicate, alloc )  {  }

// Operations.

	entry_t&       at( size_t i )							{  return ( coll_t::operator [] ( i ) );  }
	const entry_t& at( size_t i ) const						{  return ( coll_t::operator [] ( i ) );  }

	template <typename T>
	value_t& operator [] ( const T& key )
		{  iterator i = insert( entry_t( key, value_t() ) ).first;  return ( i->second );  }

	std::pair <iterator, bool> insert( const entry_t& entry )
		{  return ( sorted_insert_single( entry ) );  }

	template <typename ITER>
	void insert( ITER ib, ITER ie )
		{  sorted_insert_single( ib, ie );  }

	template <typename T>
	std::pair <iterator, bool> find_insert( const T& value )
		{  return ( find_sorted_insert_single( value, sorted_array_pred_helper <pred_t, keyfn_t> ( get_predicate() ) ) );  }

	iterator unsorted_push_back( const entry_t& entry )
		{  return ( unsorted_push_back_generic( entry ) );  }

	iterator unsorted_push_back( void )
		{  return ( unsorted_push_back_generic() );  }

	iterator unsorted_insert( iterator where, const entry_t& entry )
		{  return ( unsorted_insert_generic( where, entry ) );  }

	iterator unsorted_insert( iterator where )
		{  return ( unsorted_insert_generic( where ) );  }

	template <typename ITER>
	void unsorted_push_back( ITER ib, ITER ie )
		{  unsorted_push_back_generic( ib, ie );  }

	iterator erase( iterator where )
		{  return ( coll_t::erase( where ) );  }

	iterator erase( iterator ib, iterator ie )
		{  return ( coll_t::erase( ib, ie ) );  }

	bool erase( const key_t& key )
		{  return ( erase_single( key ) );  }

	key_compare key_comp( void ) const
		{  return ( get_predicate() );  }

	value_compare value_comp( void ) const
		{  return ( value_compare( get_predicate() ) );  }
};

//////////////////////////////////////////////////////////////////////////////
// class linear_multimap <T> declaration

template <
		typename KEY,
		typename VALUE,
		typename PRED = std::less <KEY>,
		typename COLL = fast_vector <std::pair <KEY, VALUE> > >
class linear_multimap : public linear_map <KEY, VALUE, PRED, COLL>
{
public:

// Types.

	typedef linear_map <key_t, value_t, pred_t, base_coll_t> base_t;
	typedef linear_multimap <key_t, value_t, pred_t, base_coll_t> this_t;

	using base_t::erase;

	friend void swap( this_t& l, this_t& r )				{  l.swap( r );  }

#	if GP_DEBUG
	void self_check( void ) const
	{
		coll_t::self_check( true );
	}
#	endif // GP_DEBUG

// Ctor/dtor.

	explicit linear_multimap(const pred_t& predicate = pred_t(), const alloc_t& alloc = alloc_t() )
		: base_t( predicate, alloc )  {  }

	template <typename ITER>
	linear_multimap( ITER ib, ITER ie, const pred_t& predicate = pred_t(), const alloc_t& alloc = alloc_t() )
		: base_t( ib, ie, predicate, alloc )  {  }

// Operations.

	iterator insert( const entry_t& entry )
		{  return ( sorted_insert_multi( entry ) );  }

	template <typename ITER>
	void insert( ITER ib, ITER ie )
		{  sorted_insert_multi( ib, ie );  }

	template <typename T>
	iterator find_insert( const T& value )
		{  return ( find_sorted_insert_multi( value, sorted_array_pred_helper <pred_t, keyfn_t> ( get_predicate() ) ) );  }

	size_t erase( const key_t& key )
		{  return ( erase_multi( key ) ); }

	template <typename T>
	size_t count( const T& key ) const
		{  return ( count_multi( key ) );  }
};

//////////////////////////////////////////////////////////////////////////////
// class linear_set <T> declaration

template <typename KEY>
struct linear_set_keyfn : public std::unary_function <KEY, KEY>
{
	const KEY& operator () ( const KEY& key ) const
	{
		return ( key );
	}
};

template <
		typename KEY,
		typename PRED = std::less <KEY>,
		typename COLL = fast_vector <KEY> >
class linear_set : protected sorted_array <
		KEY,
		KEY,
		linear_set_keyfn <KEY>,
		PRED,
		COLL>
{
public:

// Types.

	typedef KEY key_t;
	typedef PRED pred_t;
	typedef COLL base_coll_t;
	typedef linear_set_keyfn <key_t> keyfn_t;
	typedef sorted_array <key_t, key_t, keyfn_t, pred_t, base_coll_t> coll_t;
	typedef coll_t::alloc_t alloc_t;
	typedef linear_set <key_t, pred_t, base_coll_t> this_t;
	typedef pred_t key_compare;
	typedef pred_t value_compare;

	// these are to make STL happy
	typedef key_t   key_type;
	typedef key_t   value_type;
	typedef alloc_t allocator_type;
	typedef size_t  size_type;

	using coll_t::iterator;
	using coll_t::const_iterator;
	using coll_t::reverse_iterator;
	using coll_t::const_reverse_iterator;

	using coll_t::get_allocator;
	using coll_t::destroy;
	using coll_t::clear;
	using coll_t::reserve;
	using coll_t::reserve_extra;
	using coll_t::reserve_exact;
	using coll_t::reserve_lock;
	using coll_t::uninitialized_resize;
	using coll_t::swap;
	using coll_t::capacity;
	using coll_t::size;
	using coll_t::empty;

	friend void swap( this_t& l, this_t& r )				{  l.swap( r );  }

	using coll_t::begin;
	using coll_t::end;
	using coll_t::rbegin;
	using coll_t::rend;
	using coll_t::front;
	using coll_t::back;
	using coll_t::pop_back;
	using coll_t::pop_back_t;
	using coll_t::sort;
	using coll_t::resort;
	using coll_t::get_predicate;
	using coll_t::find;
	using coll_t::linear_find;
	using coll_t::lower_bound;
	using coll_t::upper_bound;
	using coll_t::equal_range;

#	if GP_DEBUG
	void self_check( void ) const
	{
		coll_t::self_check( false );
	}
#	endif // GP_DEBUG

// Ctor/dtor.

	explicit linear_set( const pred_t& predicate = pred_t(), const alloc_t& alloc = alloc_t() )
		: coll_t( predicate, alloc )  {  }

	template <typename ITER>
	linear_set( ITER ib, ITER ie, const pred_t& predicate = pred_t(), const alloc_t& alloc = alloc_t() )
		: coll_t( ib, ie, predicate, alloc )  {  }

// Operations.

	key_t&       at( size_t i )								{  return ( coll_t::operator [] ( i ) );  }
	const key_t& at( size_t i ) const						{  return ( coll_t::operator [] ( i ) );  }

	std::pair <iterator, bool> insert( const key_t& key )
		{  return ( sorted_insert_single( key ) );  }

	template <typename ITER>
	void insert( ITER ib, ITER ie )
		{  sorted_insert_single( ib, ie );  }

	template <typename T>
	std::pair <iterator, bool> find_insert( const T& value )
		{  return ( find_sorted_insert_single( value, get_predicate() ) );  }

	iterator unsorted_push_back( const key_t& key )
		{  return ( unsorted_push_back_generic( key ) );  }

	iterator unsorted_push_back( void )
		{  return ( unsorted_push_back_generic() );  }

	iterator unsorted_insert( iterator where, const key_t& key )
		{  return ( unsorted_insert_generic( where, key ) );  }

	iterator unsorted_insert( iterator where )
		{  return ( unsorted_insert_generic( where ) );  }

	template <typename ITER>
	void unsorted_push_back( ITER ib, ITER ie )
		{  unsorted_push_back_generic( ib, ie );  }

	iterator erase( iterator where )
		{  return ( coll_t::erase( where ) );  }

	iterator erase( iterator ib, iterator ie )
		{  return ( coll_t::erase( ib, ie ) );  }

	bool erase( const key_t& key )
		{  return ( erase_single( key ) );  }

	key_compare key_comp( void ) const
		{  return ( key_compare() );  }

	value_compare value_comp( void ) const
		{  return ( key_comp() );  }
};

//////////////////////////////////////////////////////////////////////////////
// class linear_multiset <T> declaration

template<
		typename KEY,
		typename PRED = std::less <KEY>,
		typename COLL = fast_vector <KEY> >
class linear_multiset : public linear_set <KEY, PRED, COLL>
{
public:

// Types.

	typedef linear_set <key_t, pred_t, base_coll_t> base_t;
	typedef linear_multiset <key_t, pred_t, base_coll_t> this_t;

	using base_t::erase;

	friend void swap( this_t& l, this_t& r )				{  l.swap( r );  }

#	if GP_DEBUG
	void self_check( void ) const
	{
		coll_t::self_check( true );
	}
#	endif // GP_DEBUG

// Ctor/dtor.

	explicit linear_multiset( const pred_t& predicate = pred_t(), const alloc_t& alloc = alloc_t() )
		: base_t( predicate, alloc )  {  }

	template <typename ITER>
	linear_multiset( ITER ib, ITER ie, const pred_t& predicate = pred_t(), const alloc_t& alloc = alloc_t() )
		: base_t( ib, ie, predicate, alloc )  {  }

// Operations.

	iterator insert( const key_t& key )
		{  return ( sorted_insert_multi( key ) );  }

	template <typename ITER>
	void insert( ITER ib, ITER ie )
		{  sorted_insert_multi( ib, ie );  }

	template <typename T>
	iterator find_insert( const T& value )
		{  return ( find_sorted_insert_multi( value, get_predicate() ) );  }

	size_t erase( const key_t& key )
		{  return ( erase_multi( key ) ); }

	template <typename T>
	size_t count( const T& key ) const
		{  return ( count_multi( key ) );  }
};

//////////////////////////////////////////////////////////////////////////////

#include "gpmem_new_on.h"

//////////////////////////////////////////////////////////////////////////////

#pragma pack( pop )

}  // end of namespace stdx

//////////////////////////////////////////////////////////////////////////////

namespace std
{

	// note: these functions are here specifically to cause ambiguity. i
	// can't figure out how to make it work so that if you do std::swap() on
	// something in the stdx namespace it calls the static friend method
	// instead. so this code will force you to use the stdx::swap instead.

template <typename VALUE, typename ALLOC>
inline void swap( stdx::fast_vector <VALUE, ALLOC> &,
				  stdx::fast_vector <VALUE, ALLOC> & )  {  }

template <typename KEY, typename VALUE, typename KEYFN, typename PRED, typename COLL>
inline void swap( stdx::sorted_array <KEY, VALUE, KEYFN, PRED, COLL> &,
				  stdx::sorted_array <KEY, VALUE, KEYFN, PRED, COLL> & )  {  }

template <typename KEY, typename VALUE, typename PRED, typename COLL>
inline void swap( stdx::linear_map <KEY, VALUE, PRED, COLL> &,
				  stdx::linear_map <KEY, VALUE, PRED, COLL> & )  {  }

template <typename KEY, typename VALUE, typename PRED, typename COLL>
inline void swap( stdx::linear_multimap <KEY, VALUE, PRED, COLL> &,
				  stdx::linear_multimap <KEY, VALUE, PRED, COLL> & )  {  }

template <typename KEY, typename PRED, typename COLL>
inline void swap( stdx::linear_set <KEY, PRED, COLL> &,
				  stdx::linear_set <KEY, PRED, COLL> & )  {  }

template <typename KEY, typename PRED, typename COLL>
inline void swap( stdx::linear_multiset <KEY, PRED, COLL> &,
				  stdx::linear_multiset <KEY, PRED, COLL> & )  {  }

}

//////////////////////////////////////////////////////////////////////////////

#endif  // __GPCOLL_H

//////////////////////////////////////////////////////////////////////////////
