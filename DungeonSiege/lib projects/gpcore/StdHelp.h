//////////////////////////////////////////////////////////////////////////////
//
// File     :  stdhelp.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains some cute little STL helper functions. Happy joy for
//             the lazier programmers among us.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __STDHELP_H
#define __STDHELP_H

//////////////////////////////////////////////////////////////////////////////

#include "GpCore.h"

#include <algorithm>
#include <stack>

namespace stdx  {  // begin of namespace stdx

//////////////////////////////////////////////////////////////////////////////
// Function objects

// Use these to delete elements from a collection.

	struct delete_by_ptr
		{  template <typename T> void operator () ( T* ptr )  {  Delete( ptr );  }  };

	struct delete_first_by_ptr
		{  template <typename T> void operator () ( T& obj )  {  Delete( obj.first );  }  };

	struct delete_second_by_ptr
		{  template <typename T> void operator () ( T& obj )  {  Delete( obj.second );  }  };

	struct equal_to
	{
		template <typename T, typename U>
		bool operator () ( const T& l, const U& r ) const
		{
			return ( l == r );
		}
	};

// Use these to get at elements of a pair collection (map, set etc.).

	template <typename ITER>
	struct const_iter_first : ITER
	{
		typedef ITER::value_type::first_type T1;
		const_iter_first( void )				{  }
		const_iter_first( const ITER& iter )	: ITER( iter )  {  }
		const T1& operator *  ( void ) const	{  return ( ITER::operator*().first );  }
		const T1* operator -> ( void ) const	{  return ( &**this );  }
	};

	template <typename ITER>
	struct iter_first : const_iter_first <ITER>
	{
		iter_first( void )						{  }
		iter_first( const ITER& iter )			: const_iter_first <ITER> ( iter )  {  }
		T1& operator *  ( void )				{  return ( ITER::operator*().first );  }
		T1* operator -> ( void )				{  return ( &**this );  }
	};

	template <typename ITER>
	struct const_iter_second : ITER
	{
		typedef ITER::value_type::second_type T2;
		const_iter_second( void )				{  }
		const_iter_second( const ITER& iter )	: ITER( iter )  {  }
		const T2& operator *  ( void ) const	{  return ( ITER::operator*().second );  }
		const T2* operator -> ( void ) const	{  return ( &**this );  }
	};

	template <typename ITER>
	struct iter_second : const_iter_second <ITER>
	{
		iter_second( void )						{  }
		iter_second( const ITER& iter )			: const_iter_second <ITER> ( iter )  {  }
		T2& operator *  ( void )				{  return ( ITER::operator*().second );  }
		T2* operator -> ( void )				{  return ( &**this );  }
	};

	template <typename ITER>
	const_iter_first <ITER> make_const_iter_first( const ITER& iter )
		{  return ( const_iter_first <ITER> ( iter ) );  }

	template <typename ITER>
	iter_first <ITER> make_iter_first( const ITER& iter )
		{  return ( iter_first <ITER> ( iter ) );  }

	template <typename ITER>
	const_iter_second <ITER> make_const_iter_second( const ITER& iter )
		{  return ( const_iter_second <ITER> ( iter ) );  }

	template <typename ITER>
	iter_second <ITER> make_iter_second( const ITER& iter )
		{  return ( iter_second <ITER> ( iter ) );  }

// Use these to ease assignment of a pointer to an auto pointer.

	template <typename T>
	std::auto_ptr <T> make_auto_ptr( T* obj )
	{
		return ( std::auto_ptr <T> ( obj ) );
	}

	template <typename PTR, typename T>
	void make_auto_ptr( PTR& ptr, T* obj )
	{
		ptr = std::auto_ptr <PTR::element_type> ( obj );
	}

//////////////////////////////////////////////////////////////////////////////
// Algorithms

template <typename CONTAINER, typename ITER>
inline ITER copy_all( const CONTAINER& cont, ITER out )
	{  return ( std::copy( cont.begin(), cont.end(), out ) );  }

template <typename CONTAINER>
inline std::insert_iterator <CONTAINER> inserter( CONTAINER& cont )
	{  return ( std::insert_iterator <CONTAINER> ( cont, cont.begin() ) );  }

template <typename CONTAINER, typename FUNC>
inline FUNC for_each_const( const CONTAINER& cont, FUNC func )
	{  return ( std::for_each( cont.begin(), cont.end(), func ) );  }

template <typename CONTAINER, typename FUNC>
inline FUNC for_each( CONTAINER& cont, FUNC func )
	{  return ( std::for_each( cont.begin(), cont.end(), func ) );  }

template <typename CONTAINER, typename FUNC>
inline FUNC for_each_reverse_const(const CONTAINER& cont, FUNC func )
	{  return ( std::for_each( cont.rbegin(), cont.rend(), func ) );  }

template <typename CONTAINER, typename FUNC>
inline FUNC for_each_reverse( CONTAINER& cont, FUNC func )
	{  return ( std::for_each( cont.rbegin(), cont.rend(), func ) );  }

template <typename CONTAINER, typename IOUT, typename FUNC>
inline IOUT transform( CONTAINER& cont, IOUT out, FUNC func )
	{  return ( std::transform( cont.begin(), cont.end(), out, func ) );  }

template <typename CONTAINER, typename IOUT, typename FUNC>
inline IOUT transform_const( const CONTAINER& cont, IOUT out, FUNC func )
	{  return ( std::transform( cont.begin(), cont.end(), out, func ) );  }

template <typename CONTAINER, typename T>
inline void replace( CONTAINER& cont, const T& oldVal, const T& newVal )
	{  std::replace( cont.begin(), cont.end(), oldVal, newVal );  }

template <typename CONTAINER, typename T>
inline CONTAINER::const_iterator find_const( const CONTAINER& cont, const T& value )
	{  return ( std::find( cont.begin(), cont.end(), value ) );  }

template <typename CONTAINER, typename T>
inline CONTAINER::iterator find( CONTAINER& cont, const T& value )
	{  return ( std::find( cont.begin(), cont.end(), value ) );  }

template <typename CONTAINER, typename T>
inline bool has_any( const CONTAINER& cont, const T& value )
	{  return ( find_const( cont, value ) != cont.end() );  }

template <typename CONTAINER, typename T>
inline bool has_none( const CONTAINER& cont, const T& value )
	{  return ( find_const( cont, value ) == cont.end() );  }

template <typename CONTAINER>
inline void sort( CONTAINER& cont )
	{  std::sort( cont.begin(), cont.end() );  }

template <typename CONTAINER, typename PRED>
inline void sort( CONTAINER& cont, PRED pred )
	{  std::sort( cont.begin(), cont.end(), pred );  }

template <typename ITER, typename T>
inline ITER binary_search( ITER begin, ITER end, const T& val )
{
	ITER found = std::lower_bound( begin, end, val );
	return ( ( (found != end) && !(val < *found) ) ? found : end );
}

template <typename ITER, typename T, typename PRED>
inline ITER binary_search( ITER begin, ITER end, const T& val, PRED pred )
{
	ITER found = std::lower_bound( begin, end, val, pred );
	return ( ( (found != end) && !pred( val, *found )) ? found : end );
}

template <typename OUTITER, typename COLL>
inline void copy_union( OUTITER out, const COLL& l, const COLL& r )
{
	COLL::const_iterator i, ibegin = l.begin(), iend = r.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		COLL::iterator j, jbegin = r.begin(), jend = r.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			if ( *i == *j )
			{
				*out = *i;
				++out;
			}
		}
	}
}

template <typename COLL, typename T>
inline bool erase_if_found( COLL& coll, const T& key )
{
	bool removed = false;

	COLL::iterator found = coll.find( key );
	if ( found != coll.end() )
	{
		removed = true;
		coll.erase( found );
	}

	return ( removed );
}

//////////////////////////////////////////////////////////////////////////////
// Iterators

template <typename ITER, typename CAST>
struct cast_iterator : public ITER
{
	cast_iterator( void )
		{  }
	cast_iterator( const ITER& iter )
		: ITER( iter )  {  }
	CAST& operator * ( void ) const
		{  return ( rcast <CAST&> ( ITER::operator * () ) );  }
	CAST* operator -> ( void ) const
		{  return ( & operator * () );  }
};

//////////////////////////////////////////////////////////////////////////////
// class public_stack declaration

// this allows access to internal implementation

template <typename T, typename CONT = deque <T> >
class public_stack : public std::stack <T, CONT>
{
public:
	typedef std::stack <T, CONT> StackType;
	typedef CONT SubContainerType;
	typedef CONT::const_iterator const_iterator;
	typedef CONT::iterator iterator;

	CONT& get_container( void )  {  return ( c );  }
	const CONT& get_container( void ) const  {  return ( c );  }

	void pop( void )          {  gpassert( !empty() );  c.erase( c.end() - 1 );  }
	void pop( size_t count )  {  gpassert( size() >= count );  c.erase( c.end() - count, c.end() );  }

	const T& top( void ) const  {  gpassert( !empty() );  return ( c.back() );  }
	T&       top( void )        {  gpassert( !empty() );  return ( c.back() );  }

	const T& top( size_t index ) const  {  gpassert( size() >= (index + 1) );  return ( *(c.end() - (index + 1)) );  }
	T&       top( size_t index )        {  gpassert( size() >= (index + 1) );  return ( *(c.end() - (index + 1)) );  }

	const_iterator top_iter( size_t index ) const  {  gpassert( size() >= (index + 1) );  return ( c.end() - (index + 1) );  }
	iterator       top_iter( size_t index )        {  gpassert( size() >= (index + 1) );  return ( c.end() - (index + 1) );  }

	void clear( void )  {  c.clear();  }
};

//////////////////////////////////////////////////////////////////////////////
// member function callbacks

// mem_fun1_arg

	template <typename RET, typename T, typename ARG>
	class mem_fun1_arg_t
	{
	public:
		explicit mem_fun1_arg_t( RET (T::*proc)(ARG), ARG arg )
			: m_Proc( proc ), m_Arg( arg )  {  }
		RET operator () ( T *obj ) const
			{  return ( (obj->*m_Proc)( m_Arg ) );  }

	private:
		RET (T::*m_Proc)( ARG );
		ARG m_Arg;
	};

	template <typename RET, typename T, typename ARG>
	inline mem_fun1_arg_t <RET, T, ARG>
	mem_fun1_arg( RET (T::*proc)( ARG ), ARG arg )
		{  return ( mem_fun1_arg_t <RET, T, ARG> ( proc, arg ) );  }

// vmem_fun1_arg

	template <typename T, typename ARG>
	class vmem_fun1_arg_t
	{
	public:
		explicit vmem_fun1_arg_t( void (T::*proc)(ARG), ARG arg )
			: m_Proc( proc ), m_Arg( arg )  {  }
		void operator () ( T *obj ) const
			{  (obj->*m_Proc)( m_Arg );  }

	private:
		void (T::*m_Proc)( ARG );
		ARG m_Arg;
	};

	template <typename T, typename ARG>
	inline vmem_fun1_arg_t <T, ARG>
	vmem_fun1_arg( void (T::*proc)( ARG ), ARG arg )
		{  return ( vmem_fun1_arg_t <T, ARG> ( proc, arg ) );  }

// vmem_fun1_arg_check

	template <typename T, typename ARG>
	class vmem_fun1_arg_check_t
	{
	public:
		explicit vmem_fun1_arg_check_t( void (T::*proc)(ARG), ARG arg )
			: m_Proc( proc ), m_Arg( arg )  {  }
		void operator () ( T *obj ) const
			{  if ( obj != NULL )  (obj->*m_Proc)( m_Arg );  }

	private:
		void (T::*m_Proc)( ARG );
		ARG m_Arg;
	};

	template <typename T, typename ARG>
	inline vmem_fun1_arg_check_t <T, ARG>
	vmem_fun1_arg_check( void (T::*proc)( ARG ), ARG arg )
		{  return ( vmem_fun1_arg_check_t <T, ARG> ( proc, arg ) );  }

// mem_fun1_arg_ref

	template <typename RET, typename T, typename ARG>
	class mem_fun1_arg_ref_t
	{
	public:
		explicit mem_fun1_arg_ref_t( RET (T::*proc)(ARG), ARG arg )
			: m_Proc( proc ), m_Arg( arg )  {  }
		RET operator () ( T& obj ) const
			{  return ( (obj.*m_Proc)( m_Arg ) );  }
	private:
		RET (T::*m_Proc)( ARG );
		ARG m_Arg;
	};

	template <typename RET, typename T, typename ARG>
	inline mem_fun1_arg_ref_t <RET, T, ARG>
	mem_fun1_arg_ref( RET (T::*proc)( ARG ), ARG arg )
		{  return ( mem_fun1_arg_ref_t <RET, T, ARG> ( proc, arg ) );  }

// vmem_fun1_arg_ref

	template <typename T, typename ARG>
	class vmem_fun1_arg_ref_t
	{
	public:
		explicit vmem_fun1_arg_ref_t( void (T::*proc)(ARG), ARG arg )
			: m_Proc( proc ), m_Arg( arg )  {  }
		void operator () ( T& obj ) const
			{  (obj.*m_Proc)( m_Arg );  }
	private:
		void (T::*m_Proc)( ARG );
		ARG m_Arg;
	};

	template <typename T, typename ARG>
	inline vmem_fun1_arg_ref_t <T, ARG>
	vmem_fun1_arg_ref( void (T::*proc)( ARG ), ARG arg )
		{  return ( vmem_fun1_arg_ref_t <T, ARG> ( proc, arg ) );  }

// vmem_fun

	template <typename T>
	class vmem_fun_t
	{
	public:
		explicit vmem_fun_t( void (T::*proc)() )
			: m_Proc( proc )  {  }
		void operator () ( T* obj ) const
			{  (obj->*m_Proc)();  }
	private:
		void (T::*m_Proc)();
	};

	template <typename T>
	inline vmem_fun_t <T>
	vmem_fun( void (T::*proc)() )
		{  return ( vmem_fun_t <T> ( proc ) );  }

// vmem_fun_check

	template <typename T>
	class vmem_fun_check_t
	{
	public:
		explicit vmem_fun_check_t( void (T::*proc)() )
			: m_Proc( proc )  {  }
		void operator () ( T* obj ) const
			{  if ( obj != NULL )  (obj->*m_Proc)();  }
	private:
		void (T::*m_Proc)();
	};

	template <typename T>
	inline vmem_fun_check_t <T>
	vmem_fun_check( void (T::*proc)() )
		{  return ( vmem_fun_check_t <T> ( proc ) );  }

// vmem_fun_ref

	template <typename T>
	class vmem_fun_ref_t
	{
	public:
		explicit vmem_fun_ref_t( void (T::*proc)() )
			: m_Proc( proc )  {  }
		void operator () ( T& obj ) const
			{  (obj.*m_Proc)();  }
	private:
		void (T::*m_Proc)();
	};

	template <typename T>
	inline vmem_fun_ref_t <T>
	vmem_fun_ref( void (T::*proc)() )
		{  return ( vmem_fun_ref_t <T> ( proc ) );  }

// vmem_fun1

	template <typename T, typename ARG>
	class vmem_fun1_t
	{
	public:
		explicit vmem_fun1_t( void (T::*proc)( ARG ) )
			: m_Proc( proc )  {  }
		void operator () ( T* obj, ARG arg ) const
			{  (obj->*m_Proc)( arg );  }
	private:
		void (T::*m_Proc)( ARG );
	};

	template <typename T, typename ARG>
	inline vmem_fun1_t <T, ARG>
	vmem_fun1( void (T::*proc)( ARG ) )
		{  return ( vmem_fun1_t <T, ARG> ( proc ) );  }

// vmem_fun1_ref

	template <typename T, typename ARG>
	class vmem_fun1_ref_t
	{
	public:
		explicit vmem_fun1_ref_t( void (T::*proc)( ARG ) )
			: m_Proc( proc )  {  }
		void operator () ( T& obj, ARG arg ) const
			{  (obj.*m_Proc)( arg );  }
	private:
		void (T::*m_Proc)( ARG );
	};

	template <typename T, typename ARG>
	inline vmem_fun1_ref_t <T, ARG>
	vmem_fun1_ref( void (T::*proc)( ARG ) )
		{  return ( vmem_fun1_ref_t <T, ARG> ( proc ) );  }

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace stdx

#endif  // __STDHELP_H

//////////////////////////////////////////////////////////////////////////////
