//////////////////////////////////////////////////////////////////////////////
//
// File     :  gpstring.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a std:string class without all the missing parts.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GPSTRING_H
#define __GPSTRING_H

//////////////////////////////////////////////////////////////////////////////

#include "GpCore.h"

#include "gpstring_std.h"
#include <algorithm>

//////////////////////////////////////////////////////////////////////////////
// localization helper declarations

// Unicode conversions.

	// raw
	bool WideCharToMultiByte( UINT codePage, gpstring& out, const wchar_t* in, int len = -1, bool append = false );
	bool WideCharToMultiByte( UINT codePage, gpstring& out, const gpwstring& in, bool append = false );
	bool MultiByteToWideChar( UINT codePage, gpwstring& out, const char* in, int len = -1, bool append = false );
	bool MultiByteToWideChar( UINT codePage, gpwstring& out, const gpstring& in, bool append = false );

	// auto
	bool      ToAnsi   ( gpstring& out, const wchar_t* in, int len = -1, bool append = false );
	bool      ToAnsi   ( gpstring& out, const gpwstring& in, bool append = false );
	bool      ToUnicode( gpwstring& out, const char* in, int len = -1, bool append = false );
	bool      ToUnicode( gpwstring& out, const gpstring& in, bool append = false );
	gpstring  ToAnsi   ( const wchar_t* in, int len = -1, bool append = false );
	gpstring  ToAnsi   ( const gpwstring& in, bool append = false );
	gpwstring ToUnicode( const char* in, int len = -1, bool append = false );
	gpwstring ToUnicode( const gpstring& in, bool append = false );

	// utf-8
	gpstring  UnicodeToUtf8( const wchar_t* in, int len = -1 );
	gpstring  UnicodeToUtf8( const gpwstring& in );
	gpwstring Utf8ToUnicode( const char* in, int len = -1 );
	gpwstring Utf8ToUnicode( const gpstring& in );

// Resource loaders.

	// raw
	bool LoadStringW( gpwstring& out, HINSTANCE instance, UINT id, LANGID language = 0 /*neutral*/ );
	bool LoadStringA( gpstring& out, UINT codePage, HINSTANCE instance, UINT id, LANGID language = 0 /*neutral*/ );

// String resource processors.

	// context struct for helper proc
	struct EnumResLangHelperStruct
	{
		ENUMRESLANGPROC m_Proc;
		LONG_PTR        m_Param;

		EnumResLangHelperStruct( void )									: m_Proc( 0    ), m_Param( 0     )  {  }
		EnumResLangHelperStruct( ENUMRESLANGPROC proc )					: m_Proc( proc ), m_Param( 0     )  {  }
		EnumResLangHelperStruct( ENUMRESLANGPROC proc, LONG_PTR param )	: m_Proc( proc ), m_Param( param )  {  }

		operator LONG_PTR ( void )										{  return ( (LONG_PTR)this );  }
	};

	// helper function for extracting individual string resource id's. the
	// normal windows enumeration functions pull out string resources in chunks
	// of 16 (one for each table). this function will "unwrap" those and call
	// your function back with each string id. usage: use this function instead
	// of your own callback for ::EnumResourceLanguages, and pass in an instance
	// of the EnumResLangHelperStruct for the "param" variable.
	BOOL CALLBACK EnumResLangHelperProc
		(
			HMODULE  module,	// module handle
			LPCTSTR  type,		// resource type
			LPCTSTR  name,		// resource name
			WORD     lang,		// language identifier
			LONG_PTR param		// application-defined parameter
		);

//////////////////////////////////////////////////////////////////////////////
// string traits

template <typename CHAR>
struct StringRawTag
{
	typedef CHAR char_t;
	typedef StringRawTag <CHAR> base_t;
	template <typename STR> static size_t      length( const STR& )      {  return ( 0 );  }		// $ not actually used, just exists so there is a function there
	template <typename STR> static const CHAR* c_str ( const STR& str )  {  return ( str );  }
};

template <typename CHAR>
struct StringManagedTag
{
	typedef CHAR char_t;
	typedef StringManagedTag <CHAR> base_t;
	template <typename STR> static size_t      length( const STR& str )  {  return ( str.length() );  }
	template <typename STR> static const CHAR* c_str ( const STR& str )  {  return ( str.c_str() );  }
};

template <typename T>
struct StringTraits : StringRawTag <char>
{
	typedef StringRawTag <char> base_t;
};

template <> struct StringTraits <const wchar_t* > : StringRawTag     <wchar_t>  {  };
template <> struct StringTraits <std::gp_string > : StringManagedTag <char   >  {  };
template <> struct StringTraits <std::gp_wstring> : StringManagedTag <wchar_t>  {  };
template <> struct StringTraits <gpstring       > : StringManagedTag <char   >  {  };
template <> struct StringTraits <gpwstring      > : StringManagedTag <wchar_t>  {  };
template <> struct StringTraits <gpdumbstring   > : StringManagedTag <char   >  {  };

//////////////////////////////////////////////////////////////////////////////
// string comparison functions

// Fast.

	// ansi
	inline int compare_with_case_fast( const char* l, size_t l_len, const char* r, size_t r_len, size_t n = (size_t)-1 )
	{
		// $ special optimization case

		if ( n == (size_t)-1 )
		{
			n = min( l_len, r_len ) + 1;	// $ include null term in compare
		}
		else
		{
			clamp_max( n, l_len + 1 );
			clamp_max( n, r_len + 1 );
		}

		return ( ::memcmp( l, r, n ) );
	}

	// unicode
	inline int compare_with_case_fast( const wchar_t* l, size_t l_len, const wchar_t* r, size_t r_len, size_t n = (size_t)-1 )
	{
		// $ special optimization case

		if ( n == (size_t)-1 )
		{
			n = min( l_len, r_len ) + 1;	// $ include null term in compare
		}
		else
		{
			clamp_max( n, l_len + 1 );
			clamp_max( n, r_len + 1 );
		}

		return ( ::memcmp( l, r, n * 2 ) );
	}

	// either
	template <typename CHAR>
	inline int same_with_case_fast( const CHAR* l, size_t l_len, const CHAR* r, size_t r_len, size_t n = (size_t)-1 )
		{  return ( compare_with_case_fast( l, l_len, r, r_len, n ) == 0 );  }

// Tagged redirectors (don't call these directly).

	template <typename LTAG, typename RTAG, typename CHAR> struct StringCompareTagHelper;

	template <>
	struct StringCompareTagHelper <StringRawTag <char>, StringRawTag <char>, char>
	{
		static int CompareWithCase( const char* l, size_t /*l_len*/, const char* r, size_t /*r_len*/, size_t n )
			{  return ( (n == (size_t)-1) ? ::strcmp( l, r ) : ::strncmp( l, r, n ) );  }
		static int CompareNoCase( const char* l, size_t /*l_len*/, const char* r, size_t /*r_len*/, size_t n )
			{  return ( (n == (size_t)-1) ? ::stricmp( l, r ) : ::strnicmp( l, r, n ) );  }
	};

	template <>
	struct StringCompareTagHelper <StringManagedTag <char>, StringRawTag <char>, char>
		: StringCompareTagHelper <StringRawTag <char>, StringRawTag <char>, char>  {  };

	template <>
	struct StringCompareTagHelper <StringRawTag <char>, StringManagedTag <char>, char>
		: StringCompareTagHelper <StringRawTag <char>, StringRawTag <char>, char>  {  };

	template <>
	struct StringCompareTagHelper <StringManagedTag <char>, StringManagedTag <char>, char>
		: StringCompareTagHelper <StringRawTag <char>, StringRawTag <char>, char>
	{
		static int CompareWithCase( const char* l, size_t l_len, const char* r, size_t r_len, size_t n )
			{  return ( compare_with_case_fast( l, l_len, r, r_len, n ) );  }
	};

	template <>
	struct StringCompareTagHelper <StringRawTag <wchar_t>, StringRawTag <wchar_t>, wchar_t>
	{
		static int CompareWithCase( const wchar_t* l, size_t /*l_len*/, const wchar_t* r, size_t /*r_len*/, size_t n )
			{  return ( (n == (size_t)-1) ? ::wcscmp( l, r ) : ::wcsncmp( l, r, n ) );  }
		static int CompareNoCase( const wchar_t* l, size_t /*l_len*/, const wchar_t* r, size_t /*r_len*/, size_t n )
			{  return ( (n == (size_t)-1) ? ::wcsicmp( l, r ) : ::wcsnicmp( l, r, n ) );  }
	};

	template <>
	struct StringCompareTagHelper <StringManagedTag <wchar_t>, StringRawTag <wchar_t>, wchar_t>
		: StringCompareTagHelper <StringRawTag <wchar_t>, StringRawTag <wchar_t>, wchar_t>  {  };

	template <>
	struct StringCompareTagHelper <StringRawTag <wchar_t>, StringManagedTag <wchar_t>, wchar_t>
		: StringCompareTagHelper <StringRawTag <wchar_t>, StringRawTag <wchar_t>, wchar_t>  {  };

	template <>
	struct StringCompareTagHelper <StringManagedTag <wchar_t>, StringManagedTag <wchar_t>, wchar_t>
		: StringCompareTagHelper <StringRawTag <wchar_t>, StringRawTag <wchar_t>, wchar_t>
	{
		static int CompareWithCase( const wchar_t* l, size_t l_len, const wchar_t* r, size_t r_len, size_t n )
			{  return ( compare_with_case_fast( l, l_len, r, r_len, n ) );  }
	};

// Optimizers.

	template <typename T, typename U>
	inline int compare_with_case( const T& l, const U& r, size_t n = (size_t)-1 )
	{
		return ( StringCompareTagHelper <StringTraits <T>::base_t, StringTraits <U>::base_t, StringTraits <T>::char_t >::CompareWithCase(
				 StringTraits <T>::c_str( l ), StringTraits <T>::length( l ),
				 StringTraits <U>::c_str( r ), StringTraits <U>::length( r ),
				 n ) );
	}

	template <typename T, typename U>
	inline bool same_with_case( const T& l, const U& r, size_t n = (size_t)-1 )
	{
		return ( compare_with_case( l, r, n ) == 0 );
	}

	template <typename T, typename U>
	inline int compare_no_case( const T& l, const U& r, size_t n = (size_t)-1 )
	{
		return ( StringCompareTagHelper <StringTraits <T>::base_t, StringTraits <U>::base_t, StringTraits <T>::char_t >::CompareNoCase (
				 StringTraits <T>::c_str( l ), StringTraits <T>::length( l ),
				 StringTraits <U>::c_str( r ), StringTraits <U>::length( r ),
				 n ) );
	}

	template <typename T, typename U>
	inline bool same_no_case( const T& l, const U& r, size_t n = (size_t)-1 )
	{
		return ( compare_no_case( l, r, n ) == 0 );
	}

//////////////////////////////////////////////////////////////////////////////
// std predicates

// operator '<' is no longer available - you must explicitly tell map<>, set<>
//  etc. to use one of these classes instead of allowing the default predicate.

struct string_less
{
	template <typename T, typename U> bool operator () ( const T& x, const U& y ) const
		{  return ( compare_with_case( x, y ) < 0 );  }
};

struct istring_less
{
	template <typename T, typename U> bool operator () ( const T& x, const U& y ) const
		{  return ( compare_no_case( x, y ) < 0 );  }
};

struct string_equal
{
	template <typename T, typename U> bool operator () ( const T& x, const U& y ) const
		{  return ( same_with_case( x, y ) );  }
};

struct istring_equal
{
	template <typename T, typename U> bool operator () ( const T& x, const U& y ) const
		{  return ( same_no_case( x, y ) );  }
};

//////////////////////////////////////////////////////////////////////////////
// class gpbstring_base declaration

class gpbstring_base
{
public:
	SET_NO_INHERITED( gpbstring_base );

// Constants.

	enum eCase
	{
		NO_CASE,
		WITH_CASE,
	};

// Helpers.

	static size_t length( const char* str )				{  return ( ::strlen( str ) );  }
	static size_t length( const wchar_t* str )			{  return ( ::wcslen( str ) );  }

	static char    to_lower( const char c )				{  return ( (char)::tolower( c ) );  }
	static wchar_t to_lower( const wchar_t c )			{  return ( ::towlower( c ) );  }

// Configuration.

	// setup for load_win32
	static void SetCodePage         ( UINT codePage )							{  ms_CodePage          = codePage;  }
	static void SetSystemInstance   ( HINSTANCE inst )							{  ms_SystemInstance    = inst    ;  }
	static void SetLocalizedInstance( HINSTANCE inst )							{  ms_LocalizedInstance = inst    ;  }
	static void SetLanguage         ( LANGID lang )								{  ms_Language          = lang    ;  }

	// query for load_win32
	static UINT      GetCodePage         ( void )								{  return ( ms_CodePage          );  }
	static HINSTANCE GetSystemInstance   ( void )								{  return ( ms_SystemInstance    );  }
	static HINSTANCE GetLocalizedInstance( void )								{  return ( ms_LocalizedInstance );  }
	static LANGID    GetLanguage         ( void )								{  return ( ms_Language          );  }

private:
	static UINT      ms_CodePage;
	static HINSTANCE ms_SystemInstance;
	static HINSTANCE ms_LocalizedInstance;
	static LANGID    ms_Language;
};

//////////////////////////////////////////////////////////////////////////////
// class gpbstring <CHAR, TRAITS, ALLOC> declaration

// this class is a nice std::gp_string derivative that does a few things:
//
// a. adds new useful functions. for algorithmic stuff, put it in stringtool.
// b. protects operator ==
// c. protects operator <
// d. operator const CHAR* !!! yay!!!
//
// the purpose behind protecting comparison operators is to force the user of
//  these classes to explicitly say whether or not each and every compare is
//  case-sensitive or not. in a primarily case-insensitive project a lot of
//  pain and suffering can come from automatically using a map <string, xyz>
//  where the default sorting/uniqueness test for the map is operator '<',
//  which is case-sensitive.
//
// for STL classes and other templates that require predicate parameters, you
//  should use one of the predicate structs after the string declaration below.
//  for example:
//
//  std::map <std::gp_string, int>
//
//  would be changed to std::map <gpstring, int, string_less> for case-sensitive
//  compares, and std::map <gpstring, int, istring_less> for case-insensitive
//  compares.
//
// note that we're deriving from std::gp_basic_string privately here to prevent
//  automatic "inheritance" of '<' and '==' operators - if you need to get at
//  the base class, just call get_std_string().

template <typename CHAR, typename TRAITS = std::char_traits <CHAR>, typename ALLOC = std::allocator <CHAR> >
class gpbstring : private std::gp_basic_string <CHAR, TRAITS, ALLOC>, public gpbstring_base
{
public:
	typedef gpbstring <CHAR, TRAITS, ALLOC> ThisType;
	typedef std::gp_basic_string <CHAR, TRAITS, ALLOC> Inherited;

// Constants.

	static const ThisType EMPTY;

// Ctors.

	// $ these are just copies of std::gp_basic_string functions - can't inherit ctors

	gpbstring( void )																					{  }
	gpbstring( const Inherited& x )												: Inherited( x )        {  }
	gpbstring( const Inherited& x, size_type p, size_type m )					: Inherited( x, p, m )  {  }
	gpbstring( const ThisType& x )												: Inherited( x )        {  }
	gpbstring( const ThisType& x, size_type p, size_type m )					: Inherited( x, p, m )  {  }
	gpbstring( const CHAR* s, size_type n )										: Inherited( s, n )     {  }
	gpbstring( const CHAR* s )													: Inherited( s )        {  }
	gpbstring( size_type n, CHAR c )											: Inherited( n, c )     {  }
	gpbstring( const_iterator b, const_iterator e )  							: Inherited( b, e )     {  }
	explicit gpbstring( UINT id )												{  load( id );  }

	bool load( UINT codePage, HINSTANCE instance, UINT id, LANGID lang = 0 );	// generic LoadString()

	bool load( UINT id );														// load system if gpstring, load screen if gpwstring
	bool load_system( UINT id );												// load a system string from the EXE's resource table
	bool load_screen( UINT id );												// load a localized screen string from the language resource table

// General helpers.

	void to_lower( void );														// convert entire string to lower case
	void to_upper( void );														// convert entire string to upper case

	bool is_lower( void ) const;												// returns true if entire string is lower case
	bool is_upper( void ) const;												// returns true if entire string is upper case

// Expose public functions.

	// these are all ok for public consumption
	using Inherited::begin;
	using Inherited::begin_freeze;
	using Inherited::begin_split;
	using Inherited::c_str;
	using Inherited::capacity;
	using Inherited::copy;
	using Inherited::data;
	using Inherited::empty;
	using Inherited::end;
	using Inherited::end_freeze;
	using Inherited::end_split;
	using Inherited::erase;
	using Inherited::find;
	using Inherited::find_first_not_of;
	using Inherited::find_first_of;
	using Inherited::find_last_not_of;
	using Inherited::find_last_of;
	using Inherited::get_allocator;
	using Inherited::get_ref_count;
	using Inherited::insert;
	using Inherited::length;
	using Inherited::npos;
	using Inherited::rbegin;
	using Inherited::rbegin_freeze;
	using Inherited::rbegin_split;
	using Inherited::rend;
	using Inherited::rend_freeze;
	using Inherited::rend_split;
	using Inherited::reserve;
	using Inherited::resize;
	using Inherited::rfind;
	using Inherited::size;
	using Inherited::size_type;
	using Inherited::swap;
	using Inherited::iterator;
	using Inherited::const_iterator;

	typedef Inherited::Header Header;

	reference at_freeze( size_type p )											{  return ( *(begin_freeze() + p) );  }
	reference at_split ( size_type p )											{  return ( *(begin_split () + p) );  }

	const_reference operator [] ( int p       ) const							{  return ( Inherited::operator [] ( p ) );  }
	const_reference operator [] ( size_type p ) const							{  return ( Inherited::operator [] ( p ) );  }

// General helpers.

	// recognize these from BASIC? :)
	ThisType mid  ( size_type pos, size_type n = npos ) const;					// get a substring from 'pos' of size 'n'
	ThisType left ( size_type n = npos ) const;									// get the left 'n' chars of the string
	ThisType right( size_type n = npos ) const;									// get the right 'n' chars of the string

	// these will automatically adjust the length so we don't go off the end (slower)
	ThisType mid_safe  ( size_type pos, size_type n = npos ) const;				// get a substring from 'pos' of size 'n'
	ThisType left_safe ( size_type n = npos ) const;							// get the left 'n' chars of the string
	ThisType right_safe( size_type n = npos ) const;							// get the right 'n' chars of the string

	void clear( void );															// does an erase(), but if vector<> can call it clear() why can't we?

	bool starts_with( const CHAR* str, eCase cs ) const;						// returns true if starts with this string
	bool starts_with( CHAR c, eCase cs ) const;
	bool ends_with  ( const CHAR* str, eCase cs ) const;						// returns true if ends with this string
	bool ends_with  ( CHAR c, eCase cs ) const;

	template <typename ITER>
	static ThisType join( const CHAR* s, ITER ib, ITER ie )
	{
		ThisType out;
		for ( ITER i = ib ; i != ie ; ++i )
		{
			if ( i !== ib )
			{
				out += s;
			}
			out += *ib;
		}
		return ( out );
	}

	template <typename ITER>
	static ThisType join( CHAR c, ITER ib, ITER ie )
	{
		ThisType out;
		for ( ITER i = ib ; i != ie ; ++i )
		{
			if ( i !== ib )
			{
				out += c;
			}
			out += *ib;
		}
		return ( out );
	}

	// $$$ split() func!

// Type support.

	operator const CHAR* ( void ) const;										// auto-convert ThisType to C-style string

	ThisType& append ( const CHAR* s, size_type m )								{  Inherited::append( s, m );  return ( *this );  }
	ThisType& append ( const CHAR* s )											{  Inherited::append( s );  return ( *this );  }
	ThisType& append ( size_type m, CHAR c )									{  Inherited::append( m, c );  return ( *this );  }
	ThisType& append ( const_iterator f, const_iterator l )						{  Inherited::append( f, l );  return ( *this );  }
	ThisType& append ( const Inherited& x )										{  Inherited::append( x );  return ( *this );  }
	ThisType& append ( const Inherited& x, size_type p, size_type m )			{  Inherited::append( x, p, m );  return ( *this );  }
	ThisType& append ( const ThisType& x )										{  Inherited::append( x.get_std_string() );  return ( *this );  }
	ThisType& append ( const ThisType& x, size_type p, size_type m )			{  Inherited::append( x.get_std_string(), p, m );  return ( *this );  }
	ThisType& appendf( const CHAR* format, ... );

	ThisType& assign ( const CHAR* s, size_type n )								{  Inherited::assign( s, n );  return ( *this );  }
	ThisType& assign ( const CHAR* s )											{  Inherited::assign( s );  return ( *this );  }
	ThisType& assign ( size_type n, CHAR c )									{  Inherited::assign( n, c );  return ( *this );  }
	ThisType& assign ( const_iterator f, const_iterator l )						{  Inherited::assign( f, l );  return ( *this );  }
	ThisType& assign ( const Inherited& x )										{  Inherited::assign( x );  return ( *this );  }
	ThisType& assign ( const Inherited& x, size_type p, size_type m )			{  Inherited::assign( x, p, m );  return ( *this );  }
	ThisType& assign ( const ThisType& x )										{  Inherited::assign( x.get_std_string() );  return ( *this );  }
	ThisType& assign ( const ThisType& x, size_type p, size_type m )			{  Inherited::assign( x.get_std_string(), p, m );  return ( *this );  }
	ThisType& assignf( const CHAR* format, ... );

	ThisType& erase( size_type p0 = 0, size_type m = npos )						{  Inherited::erase( p0, m );  return ( *this );  }

	ThisType& insert ( size_type p0, const CHAR* s, size_type m )				{  Inherited::insert( p0, s, m );  return ( *this );  }
	ThisType& insert ( size_type p0, const CHAR* s )							{  Inherited::insert( p0, s );  return ( *this );  }
	ThisType& insert ( size_type p0, size_type m, CHAR c )						{  Inherited::insert( p0, m, c );  return ( *this );  }
	ThisType& insert ( size_type p0, const Inherited& x )						{  Inherited::insert( p0, x );  return ( *this );  }
	ThisType& insert ( size_type p0, const Inherited& x, size_type p, size_type m )	{  Inherited::insert( p0, x, p, m );  return ( *this );  }
	ThisType& insert ( size_type p0, const ThisType& x )						{  Inherited::insert( p0, x.get_std_string() );  return ( *this );  }
	ThisType& insert ( size_type p0, const ThisType& x, size_type p, size_type m )	{  Inherited::insert( p0, x.get_std_string(), p, m );  return ( *this );  }
	ThisType& insertf( size_type p0, const CHAR* format, ... );

	ThisType& replace( size_type p0, size_type n0, const Inherited& x )			{  Inherited::replace( p0, n0, x );  return ( *this );  }
	ThisType& replace( size_type p0, size_type n0, const Inherited& x, size_type p, size_type m )	{  Inherited::replace( p0, n0, x, p, m );  return ( *this );  }
	ThisType& replace( size_type p0, size_type n0, const ThisType& x )			{  Inherited::replace( p0, n0, x.get_std_string() );  return ( *this );  }
	ThisType& replace( size_type p0, size_type n0, const ThisType& x, size_type p, size_type m )	{  Inherited::replace( p0, n0, x.get_std_string(), p, m );  return ( *this );  }
	ThisType& replace( size_type p0, size_type n0, const CHAR* s, size_type m )	{  Inherited::replace( p0, n0, s, m );  return ( *this );  }
	ThisType& replace( size_type p0, size_type n0, const CHAR* s )				{  Inherited::replace( p0, n0, s );  return ( *this );  }
	ThisType& replace( size_type p0, size_type n0, size_type m, CHAR c )		{  Inherited::replace( p0, n0, m, c );  return ( *this );  }
	ThisType& replace( iterator f, iterator l, const Inherited& x )				{  Inherited::replace( f, l, x );  return ( *this );  }
	ThisType& replace( iterator f, iterator l, const ThisType& x )				{  Inherited::replace( f, l, x.get_std_string() );  return ( *this );  }
	ThisType& replace( iterator f, iterator l, const CHAR* s, size_type m )		{  Inherited::replace( f, l, s, m );  return ( *this );  }
	ThisType& replace( iterator f, iterator l, const CHAR* s )					{  Inherited::replace( f, l, s );  return ( *this );  }
	ThisType& replace( iterator f, iterator l, size_type m, CHAR c )			{  Inherited::replace( f, l, m, c );  return ( *this );  }
	ThisType& replace( iterator f1, iterator l1, const_iterator f2, const_iterator l2 )	{  Inherited::replace( f1, l1, f2, l2 );  return ( *this );  }

	ThisType substr( size_type p = 0, size_type m = npos ) const				{  return ( ThisType( *this, p, m ) );  }

	void swap( Inherited& x )													{  Inherited::swap( x );  }
	void swap( ThisType& x )													{  Inherited::swap( x.get_std_string() );  }

	size_type find             ( const ThisType& x, size_type p = 0    ) const	{  return ( Inherited::find( x.get_std_string(), p ) );  }
	size_type rfind            ( const ThisType& x, size_type p = npos ) const	{  return ( Inherited::rfind( x.get_std_string(), p ) );  }
	size_type find_first_of    ( const ThisType& x, size_type p = 0    ) const	{  return ( Inherited::find_first_of( x.get_std_string(), p ) );  }
	size_type find_last_of     ( const ThisType& x, size_type p = npos ) const	{  return ( Inherited::find_last_of( x.get_std_string(), p ) );  }
	size_type find_first_not_of( const ThisType& x, size_type p = 0    ) const	{  return ( Inherited::find_first_not_of( x.get_std_string(), p ) );  }
	size_type find_last_not_of ( const ThisType& x, size_type p = npos ) const	{  return ( Inherited::find_last_not_of( x.get_std_string(), p ) );  }

	ThisType& operator = ( const CHAR* s )										{  Inherited::operator = ( s );  return ( *this );  }
	ThisType& operator = ( CHAR c )												{  Inherited::operator = ( c );  return ( *this );  }
	ThisType& operator = ( const Inherited& x )									{  Inherited::operator = ( x );  return ( *this );  }
	ThisType& operator = ( const ThisType& str )								{  Inherited::operator = ( str.get_std_string() );  return ( *this );  }

	ThisType& operator += ( const CHAR* s )										{  Inherited::operator += ( s );  return ( *this );  }
	ThisType& operator += ( CHAR c )											{  Inherited::operator += ( c );  return ( *this );  }
	ThisType& operator += ( const Inherited& x )								{  Inherited::operator += ( x );  return ( *this );  }
	ThisType& operator += ( const ThisType& str )								{  Inherited::operator += ( str.get_std_string() );  return ( *this );  }

// Special.

	// parent class access (retrieve base class if you *really* want to)
	const Inherited& get_std_string( void ) const								{  return ( *this );  }
		  Inherited& get_std_string( void )										{  return ( *this );  }

// Comparison.

	// use npos for 'n' in these to compare entire strings

	int  compare_no_case  ( const CHAR* str, size_type n = npos ) const			{  return ( ::compare_no_case  ( *this, str, n ) );  }
	int  compare_with_case( const CHAR* str, size_type n = npos ) const			{  return ( ::compare_with_case( *this, str, n ) );  }
	bool same_no_case     ( const CHAR* str, size_type n = npos ) const			{  return ( ::same_no_case     ( *this, str, n ) );  }
	bool same_with_case   ( const CHAR* str, size_type n = npos ) const			{  return ( ::same_with_case   ( *this, str, n ) );  }

	int  compare_no_case  ( const ThisType& str, size_type n = npos ) const		{  return ( ::compare_no_case  ( *this, str, n ) );  }
	int  compare_with_case( const ThisType& str, size_type n = npos ) const		{  return ( ::compare_with_case( *this, str, n ) );  }
	bool same_no_case     ( const ThisType& str, size_type n = npos ) const		{  return ( ::same_no_case     ( *this, str, n ) );  }
	bool same_with_case   ( const ThisType& str, size_type n = npos ) const		{  return ( ::same_with_case   ( *this, str, n ) );  }

// CString emulation.

	// $ these are included for compatibility with ATL/WTL/MFC code that uses
	//   CString. do not call these functions directly.

	void  Empty             ( void )											{  clear();  }
	CHAR* GetBuffer         ( int minBufLength );
	void  ReleaseBuffer     ( int newLength = -1 );
	CHAR* GetBufferSetLength( int newLength )									{  return ( GetBuffer( newLength ) );  }
};

template <typename CHAR, typename TRAITS, typename ALLOC>
const gpbstring <CHAR, TRAITS, ALLOC> gpbstring <CHAR, TRAITS, ALLOC>::EMPTY;

//////////////////////////////////////////////////////////////////////////////
// gpbstring inline implementations

template <typename CHAR, typename TRAITS, typename ALLOC> inline
gpbstring <CHAR, TRAITS, ALLOC> gpbstring <CHAR, TRAITS, ALLOC>
	:: mid( size_type pos, size_type n ) const
{
	return ( substr( pos, n ) );
}

template <typename CHAR, typename TRAITS, typename ALLOC> inline
gpbstring <CHAR, TRAITS, ALLOC> gpbstring <CHAR, TRAITS, ALLOC>
	:: left( size_type n ) const
{
	return ( substr( 0, n ) );
}

template <typename CHAR, typename TRAITS, typename ALLOC> inline
gpbstring <CHAR, TRAITS, ALLOC> gpbstring <CHAR, TRAITS, ALLOC>
	:: right( size_type n ) const
{
	return ( substr( (n == npos ) ? 0 : (length() - n), n) );
}

template <typename CHAR, typename TRAITS, typename ALLOC> inline
gpbstring <CHAR, TRAITS, ALLOC> gpbstring <CHAR, TRAITS, ALLOC>
	:: mid_safe( size_type pos, size_type n ) const
{
	clamp_min_max( pos, (size_type)0, length() );
	clamp_min_max( n,   (size_type)0, length() - pos );
	return ( substr( pos, n ) );
}

template <typename CHAR, typename TRAITS, typename ALLOC> inline
gpbstring <CHAR, TRAITS, ALLOC> gpbstring <CHAR, TRAITS, ALLOC>
	:: left_safe( size_type n ) const
{
	return ( mid_safe( 0, n ) );
}

template <typename CHAR, typename TRAITS, typename ALLOC> inline
gpbstring <CHAR, TRAITS, ALLOC> gpbstring <CHAR, TRAITS, ALLOC>
	:: right_safe( size_type n ) const
{
	return ( mid_safe( (n == npos) ? 0 : (length() - n), n) );
}

template <typename CHAR, typename TRAITS, typename ALLOC> inline
void gpbstring <CHAR, TRAITS, ALLOC>
	:: clear( void )
{
	_Tidy();
}

template <typename CHAR, typename TRAITS, typename ALLOC> inline
bool gpbstring <CHAR, TRAITS, ALLOC>
	:: starts_with( const CHAR* str, eCase cs ) const
{
	gpassert( str != null );
	size_t len = length( str )
	if ( len > length() )
	{
		return ( false );
	}
	return ( cs ? same_with_case( *this, str, len ) : same_no_case( *this, str, len ) );
}

template <typename CHAR, typename TRAITS, typename ALLOC> inline
bool gpbstring <CHAR, TRAITS, ALLOC>
	:: starts_with( CHAR c, eCase cs ) const
{
	if ( empty() )
	{
		return ( false );
	}

	if ( cs )
	{
		return ( (*this)[ 0 ] == c );
	}
	else
	{
		return ( to_lower( (*this)[ 0 ] ) == to_lower( c ) );
	}
}

template <typename CHAR, typename TRAITS, typename ALLOC> inline
bool gpbstring <CHAR, TRAITS, ALLOC>
	:: ends_with( const CHAR* str, eCase cs ) const
{
	gpassert( str != null );
	size_t len = length( str );
	if ( len > length() )
	{
		return ( false );
	}
	return ( cs ? same_with_case( end() - len, str, len ) : same_no_case( end() - len, str, len ) );
}

template <typename CHAR, typename TRAITS, typename ALLOC> inline
bool gpbstring <CHAR, TRAITS, ALLOC>
	:: ends_with( CHAR c, eCase cs ) const
{
	if ( empty() )
	{
		return ( false );
	}

	if ( cs )
	{
		return ( (*this)[ length() - 1 ] == c );
	}
	else
	{
		return ( to_lower( (*this)[ length() - 1 ] ) == to_lower( c ) );
	}
}

template <typename CHAR, typename TRAITS, typename ALLOC> inline
gpbstring <CHAR, TRAITS, ALLOC>
	:: operator const CHAR* ( void ) const
{
	return ( c_str() );
}

template <typename CHAR, typename TRAITS, typename ALLOC>
CHAR* gpbstring <CHAR, TRAITS, ALLOC>
	:: GetBuffer( int minBufLength )
{
	_Split();
	resize( minBufLength );
	return ( ccast <CHAR*> ( c_str() ) );
}

template <typename CHAR, typename TRAITS, typename ALLOC>
void gpbstring <CHAR, TRAITS, ALLOC>
	:: ReleaseBuffer( int newLength )
{
	_Split();
	if ( newLength == -1 )
	{
		newLength = ::strlen( *this );
	}
	resize( newLength );
}

//////////////////////////////////////////////////////////////////////////////
// class gpdumbstring declaration

// purpose: use gpdumbstring when you're concerned about memory usage. the
//  std::gp_string class allocates a *minimum* of 33 bytes for itself, and
//  always rounds its size up to the next 32 byte - 1 boundary. it's also
//  reference counted. these are good things for general string ops but really
//  suck up memory. so if all you need is a simple string class that just owns a
//  char* and automatically deletes it on destruction, this one's for you. no
//  ref counting, reserved memory, etc. god damn simple.
//
// stylistic note: i'm copying std::gp_string methods whenever possible.
//
// note: this class is templatized to support different allocators only. this
//  would be very useful if you know you only require an add-only allocator...
//  however to save space the allocator is _not_ kept local and is only used
//  statically. msvc++ always stores zero-sized classes with 4 bytes anyway.
//
// implementation note: m_String points to the actual string, but two bytes
//  backwards is the length. string lengths should never exceed USHRT_MAX.
//
// note: this class has the same goals as gpstring. no operator < or == as a
//  result.

#pragma pack ( push, 1 )
struct DumbHeader
{
	WORD m_Length;
};
#pragma pack ( pop )

template <typename ALLOC = std::allocator <char> >
class gpdumbstring_t
{
public:
	SET_NO_INHERITED( gpdumbstring_t );

	#if GPSTRING_TRACKING
	typedef std::TrackingStringHeader <DumbHeader, char> Header;
	#else // GPSTRING_TRACKING
	typedef DumbHeader Header;
	#endif // GPSTRING_TRACKING

// Typedef boilerplate. Required for compatibility with std::gp_string.

	// simple types
	typedef ALLOC::size_type       size_type;
	typedef ALLOC::difference_type difference_type;
	typedef ALLOC::pointer         pointer;
	typedef ALLOC::const_pointer   const_pointer;
	typedef ALLOC::reference       reference;
	typedef ALLOC::const_reference const_reference;
	typedef ALLOC::value_type      value_type;
	typedef ALLOC::pointer         iterator;
	typedef ALLOC::const_pointer   const_iterator;

	// reverse iterators
	typedef std::reverse_iterator <const_iterator, value_type, const_reference, const_pointer, difference_type> const_reverse_iterator;
	typedef std::reverse_iterator <iterator, value_type, reference, pointer, difference_type> reverse_iterator;

	// constants
	static const size_type npos;

// Ctors.

	gpdumbstring_t( void )										   {  m_String = null_str();          }
	gpdumbstring_t( const ThisType& x, size_type p, size_type m )  {  internal_assign( x, p, m );     }
	gpdumbstring_t( const ThisType& x )							   {  internal_assign( x, 0, npos );  }
	gpdumbstring_t( const char* s, size_type n )				   {  internal_assign( s, n );        }
	gpdumbstring_t( const char* s )								   {  internal_assign( s, npos );     }
	gpdumbstring_t( size_type n, char c )						   {  internal_assign( n, c );        }
	gpdumbstring_t( const_iterator b, const_iterator e )		   {  internal_assign( b, e - b );    }
   ~gpdumbstring_t( void )										   {  dealloc();  }

// Query.

	size_type   size  ( void ) const  {  return ( get_length( m_String ) );  }
	size_type   length( void ) const  {  return ( get_length( m_String ) );  }
	bool        empty ( void ) const  {  return ( size() == 0 );  }
	const char* c_str ( void ) const  {  return ( m_String );  }
	const char* data  ( void ) const  {  return ( m_String );  }

	operator const char* ( void ) const  {  return ( m_String );  }

// Assignment.

	ThisType& assign( const ThisType& x, size_type p, size_type m )  {  auto_free af( *this );  internal_assign( x, p, m );     return ( *this );  }
	ThisType& assign( const ThisType& x )							 {                                   assign( x, 0, npos );  return ( *this );  }
	ThisType& assign( const char* s, size_type n )					 {  auto_free af( *this );  internal_assign( s, n );        return ( *this );  }
	ThisType& assign( const char* s )								 {                                   assign( s, npos );     return ( *this );  }
	ThisType& assign( size_type n, char c )							 {              dealloc();  internal_assign( n, c );        return ( *this );  }
	ThisType& assign( const_iterator b, const_iterator e )			 {  auto_free af( *this );  internal_assign( b, e - b );    return ( *this );  }

	ThisType& operator = ( const ThisType& x )        {  return ( assign( x ) );  }
	ThisType& operator = ( const gpstring& x )        {  return ( assign( x.c_str(), x.length() ) );  }
	ThisType& operator = ( const std::gp_string& x )  {  return ( assign( x.c_str(), x.length() ) );  }
	ThisType& operator = ( const char* s )            {  return ( assign( s ) );  }
	ThisType& operator = ( char c )                   {  return ( assign( 1, c ) );  }

// Appending.

	ThisType& append( const ThisType& x )					{  return ( append( x, 0, npos ) ); }
	ThisType& append( const char* s )						{  return ( append( s, npos ) );  }
	ThisType& append( const_iterator b, const_iterator e )  {  return ( append( b, e - b ) );  }

	ThisType& operator += ( const ThisType& x )  {  return ( append( x ) );  }
	ThisType& operator += ( const char* s )		 {  return ( append( s ) );  }
	ThisType& operator += ( char c )			 {  return ( append( 1, c ) );  }

	ThisType& append( const ThisType& x, size_type p, size_type m )
	{
		resolve_length( x, p, m );
		if ( m != 0 )
		{
			auto_free af( *this );
			int oldSize = size();
			realloc_keep( oldSize + m );
			::memcpy( m_String + oldSize, x.m_String + p, m );
		}
		return ( *this );
	}

	ThisType& append( const char* s, size_type m )
	{
		resolve_length( s, m );
		if ( m != 0 )
		{
			auto_free af( *this );
			int oldSize = size();
			realloc_keep( oldSize + m );
			::memcpy( m_String + oldSize, s, m );
		}
		return ( *this );
	}

	ThisType& append( size_type m, char c )
	{
		if ( m != 0 )
		{
			auto_free af( *this );
			int oldSize = size();
			realloc_keep( oldSize + m );
			::memset( m_String + oldSize, c, m );
		}
		return ( *this );
	}

	ThisType& replace( size_type p, const gpstring& s )
	{
		if ( !s.empty() )
		{
			size_t len = p + s.length();
			if ( len != size() )
			{
				auto_free af( *this );
				realloc_keep( len );
			}

			::memcpy( m_String + p, s.c_str(), s.length() );
		}

		return ( *this );
	}

// $$$ Missing: insert, erase, replace, resize, copy, find*, rfind*

// Erasing.

	void clear( void )  {  dealloc();  m_String = null_str();  }

// Comparison.

	int  compare_no_case  ( const char* str, size_type n = npos ) const  {  return ( ::compare_no_case  ( *this, str, n ) );  }
	int  compare_with_case( const char* str, size_type n = npos ) const  {  return ( ::compare_with_case( *this, str, n ) );  }
	bool same_no_case     ( const char* str, size_type n = npos ) const  {  return ( ::same_no_case     ( *this, str, n ) );  }
	bool same_with_case   ( const char* str, size_type n = npos ) const  {  return ( ::same_with_case   ( *this, str, n ) );  }

	int  compare_no_case  ( const ThisType& str, size_type n = npos ) const  {  return ( ::compare_no_case  ( *this, str, n ) );  }
	int  compare_with_case( const ThisType& str, size_type n = npos ) const  {  return ( ::compare_with_case( *this, str, n ) );  }
	bool same_no_case     ( const ThisType& str, size_type n = npos ) const  {  return ( ::same_no_case     ( *this, str, n ) );  }
	bool same_with_case   ( const ThisType& str, size_type n = npos ) const  {  return ( ::same_with_case   ( *this, str, n ) );  }

// Substring access.

	ThisType substr( size_type p = 0, size_type m = npos ) const  {  return ( ThisType( *this, p, m ) );  }

	ThisType mid  ( size_type pos, size_type n ) const  {  return ( substr( pos, n ) );  }
	ThisType left ( size_type n ) const                 {  return ( substr( 0, n ) );  }
	ThisType right( size_type n ) const                 {  return ( substr( (n == npos) ? 0 : (length() - n), n) );  }

	ThisType mid_safe( size_type pos, size_type n ) const
	{
		clamp_min_max( pos, (size_type)0, length() );
		clamp_min_max( n,   (size_type)0, length() - pos );
		return ( substr( pos, n ) );
	}

	ThisType left_safe ( size_type n ) const  {  return ( mid_safe( 0, n ) );  }
	ThisType right_safe( size_type n ) const  {  return ( mid_safe( (n == npos) ? 0 : (length() - n), n) );  }

// Element access.

	reference       at          ( size_type p )        {  gpassert( p <= size() );  return ( m_String[ p ] );  }
	const_reference at          ( size_type p ) const  {  gpassert( p <= size() );  return ( m_String[ p ] );  }
	reference       operator [] ( size_type p )        {  return ( at( p ) );  }
	const_reference operator [] ( size_type p ) const  {  return ( at( p ) );  }

// General utility.

	void swap( ThisType& x )  {  std::swap( m_String, x.m_String );  }
	friend void swap( ThisType& x, ThisType& y )  {  x.swap( y );  }

	void to_lower( void )  {  if ( !empty() )  {  _strlwr( begin() );  }  }
	void to_upper( void )  {  if ( !empty() )  {  _strupr( begin() );  }  }

	bool is_lower( void ) const  {  return ( std::find_if( begin(), end(), isupper ) == end() );  }
	bool is_upper( void ) const  {  return ( std::find_if( begin(), end(), islower ) == end() );  }

// Iterators.

	iterator       begin( void )        {  return ( m_String );  }
	const_iterator begin( void ) const  {  return ( m_String );  }
	iterator       end  ( void )        {  return ( m_String + size() );  }
	const_iterator end  ( void ) const  {  return ( m_String + size() );  }

	reverse_iterator       rbegin( void )        {  return ( reverse_iterator      ( end  () ) );  }
	const_reverse_iterator rbegin( void ) const  {  return ( const_reverse_iterator( end  () ) );  }
	reverse_iterator       rend  ( void )        {  return ( reverse_iterator      ( begin() ) );  }
	const_reverse_iterator rend  ( void ) const  {  return ( const_reverse_iterator( begin() ) );  }

private:
	// purpose of this class is to remember the old pointer in case client uses
	//  "this" on functions like append, assign, etc.
	struct auto_free
	{
		char* m_String;

		auto_free( ThisType& t ) : m_String( t.m_String )  {  }
	   ~auto_free( void )  {  ThisType::dealloc( m_String );  }
	};

	friend auto_free;

	static void resolve_length( const ThisType& x, size_type p, size_type& m )
	{
		gpassert( p <= x.size() );
		if ( m == npos )
		{
			m = x.size() - p;
		}
		gpassert( (m + p) <= x.size() );
	}

	static void resolve_length( const char* s, size_type& n )
	{
		gpassert( s != NULL );
		if ( n == npos )
		{
			n = ::strlen( s );
		}
	}

	void internal_assign( const ThisType& x, size_type p, size_type m )
	{
		resolve_length( x, p, m );
		alloc_copy( x.m_String + p, m );
	}

	void internal_assign( const char* s, size_type n )
	{
		resolve_length( s, n );
		alloc_copy( s, n );
	}

	void internal_assign( size_type n, char c )
	{
		alloc( n );
		if ( n != 0 )
		{
			::memset( m_String, c, n );
		}
	}

	void alloc( size_type n )
	{
		if ( n == 0 )
		{
			m_String = null_str();
		}
		else
		{
			gpassert( n < USHRT_MAX );
			Header* header = (Header*)ms_Alloc.allocate( n + sizeof( Header ) + 1, 0 );
			header->m_Length = (WORD)n;
			m_String = (char*)(header + 1);
			m_String[ n ] = '\0';

#			if GPSTRING_TRACKING
			header->Link();
#			endif // GPSTRING_TRACKING
		}
	}

	void alloc_copy( const char* s, size_type n )
	{
		alloc( n );
		if ( n != 0 )
		{
			::memcpy( m_String, s, n );
		}
	}

	void realloc_keep( size_type n )
	{
		size_type oldSize = size();
		if ( oldSize != n )
		{
			char* oldStr = m_String;
			alloc( n );
			::memcpy( m_String, oldStr, min( oldSize, n ) );
		}
	}

	void realloc_toss( size_type n )
	{
		if ( size() != n )
		{
			alloc( n );
		}
	}

	static size_type get_length( char* str )
	{
		return ( ((Header*)str)[ -1 ].m_Length );
	}

	static void dealloc( char* str )
	{
		if ( str != null_str() )
		{
			Header* header = (Header*)str - 1;

#			if GPSTRING_TRACKING
			header->Unlink( true );
#			endif // GPSTRING_TRACKING

			ms_Alloc.deallocate( header, header->m_Length + sizeof( Header ) + 1 );
		}
	}

	void dealloc( void )
	{
		dealloc( m_String );
	}

	static char* null_str()  {  static char c[ sizeof( Header ) + 2 ];  return ( c + sizeof( Header ) );  }

	static ALLOC ms_Alloc;
	char* m_String;
};

template <typename ALLOC> ALLOC gpdumbstring_t <ALLOC>::ms_Alloc;
template <typename ALLOC> const gpdumbstring_t <ALLOC>::size_type gpdumbstring_t <ALLOC>::npos = (size_type)-1;

typedef gpdumbstring_t <> gpdumbstring;

//////////////////////////////////////////////////////////////////////////////
// gpxstring helper function declarations

// Types.

	typedef gpbstring <char> gpstring;
	typedef gpbstring <wchar_t> gpwstring;

// Concatenation operators.

	template <typename CHAR, typename TRAITS, typename ALLOC> inline
	gpbstring <CHAR, TRAITS, ALLOC> operator + ( const gpbstring <CHAR, TRAITS, ALLOC>& l, const gpbstring <CHAR, TRAITS, ALLOC>& r )
		{  return ( gpbstring <CHAR, TRAITS, ALLOC> ( l ) += r );  }

	template <typename CHAR, typename TRAITS, typename ALLOC> inline
	gpbstring <CHAR, TRAITS, ALLOC> operator + ( const gpbstring <CHAR, TRAITS, ALLOC>& l, const std::gp_basic_string <CHAR, TRAITS, ALLOC>& r )
		{  return ( gpbstring <CHAR, TRAITS, ALLOC> ( l ) += r );  }

	template <typename CHAR, typename TRAITS, typename ALLOC> inline
	gpbstring <CHAR, TRAITS, ALLOC> operator + ( const std::gp_basic_string <CHAR, TRAITS, ALLOC>& l, const gpbstring <CHAR, TRAITS, ALLOC>& r )
		{  return ( gpbstring <CHAR, TRAITS, ALLOC> ( l ) += r );  }

	template <typename CHAR, typename TRAITS, typename ALLOC> inline
	gpbstring <CHAR, TRAITS, ALLOC> operator + ( const CHAR* l, const gpbstring <CHAR, TRAITS, ALLOC>& r )
		{  return ( gpbstring <CHAR, TRAITS, ALLOC> ( l ) += r );  }

	template <typename CHAR, typename TRAITS, typename ALLOC> inline
	gpbstring <CHAR, TRAITS, ALLOC> operator + ( const gpbstring <CHAR, TRAITS, ALLOC>& l, const CHAR* r )
		{  return ( gpbstring <CHAR, TRAITS, ALLOC> ( l ) += r );  }

	template <typename CHAR, typename TRAITS, typename ALLOC> inline
	gpbstring <CHAR, TRAITS, ALLOC> operator + ( CHAR l, const gpbstring <CHAR, TRAITS, ALLOC>& r )
		{  return ( gpbstring <CHAR, TRAITS, ALLOC> ( 1, l ) += r );  }

	template <typename CHAR, typename TRAITS, typename ALLOC> inline
	gpbstring <CHAR, TRAITS, ALLOC> operator + ( const gpbstring <CHAR, TRAITS, ALLOC>& l, CHAR r )
		{  return ( gpbstring <CHAR, TRAITS, ALLOC> ( l ) += r );  }

	template <typename ALLOC> inline
	gpdumbstring_t <ALLOC> operator + ( const gpdumbstring_t <ALLOC>& l, const gpdumbstring_t <ALLOC>& r )
		{  return ( gpdumbstring_t <ALLOC> ( l ) += r );  }

	template <typename ALLOC> inline
	gpdumbstring_t <ALLOC> operator + ( const gpdumbstring_t <ALLOC>& l, const std::gp_string& r )
		{  return ( gpdumbstring_t <ALLOC> ( l ) += r );  }

	template <typename ALLOC> inline
	gpdumbstring_t <ALLOC> operator + ( const std::gp_string& l, const gpdumbstring_t <ALLOC>& r )
		{  return ( gpdumbstring_t <ALLOC> ( l ) += r );  }

	template <typename ALLOC> inline
	gpdumbstring_t <ALLOC> operator + ( const CHAR* l, const gpdumbstring_t <ALLOC>& r )
		{  return ( gpdumbstring_t <ALLOC> ( l ) += r );  }

	template <typename ALLOC> inline
	gpdumbstring_t <ALLOC> operator + ( const gpdumbstring_t <ALLOC>& l, const CHAR* r )
		{  return ( gpdumbstring_t <ALLOC> ( l ) += r );  }

	template <typename ALLOC> inline
	gpdumbstring_t <ALLOC> operator + ( CHAR l, const gpdumbstring_t <ALLOC>& r )
		{  return ( gpdumbstring_t <ALLOC> ( 1, l ) += r );  }

	template <typename ALLOC> inline
	gpdumbstring_t <ALLOC> operator + ( const gpdumbstring_t <ALLOC>& l, CHAR r )
		{  return ( gpdumbstring_t <ALLOC> ( l ) += r );  }

// Insertion operators.

	template <typename CHAR, typename TRAITS, typename ALLOC> inline
	std::basic_ostream <CHAR, TRAITS>& operator << ( std::basic_ostream <CHAR, TRAITS>& out, const gpbstring <CHAR, TRAITS, ALLOC>& data )
		{  return ( out << data.get_std_string() );  }

	template <typename ALLOC> inline
	std::ostream& operator << ( std::ostream& out, const gpdumbstring_t <ALLOC>& data )
		{  return ( out << data.c_str() );  }

// Swap.

	template <typename CHAR, typename TRAITS, typename ALLOC> inline
	void swap( std::gp_basic_string <CHAR, TRAITS, ALLOC>& x, gpbstring <CHAR, TRAITS, ALLOC>& y )
		{  y.swap( x );  }
	template <typename CHAR, typename TRAITS, typename ALLOC> inline
	void swap( gpbstring <CHAR, TRAITS, ALLOC>& x, std::gp_basic_string <CHAR, TRAITS, ALLOC>& y )
		{  x.swap( y );  }
	template <typename CHAR, typename TRAITS, typename ALLOC> inline
	void swap( gpbstring <CHAR, TRAITS, ALLOC>& x, gpbstring <CHAR, TRAITS, ALLOC>& y )
		{  x.swap( y );  }

// Formatting.

	// easier than doing gpstring().assignf()...
	gpstring  gpstringf ( const char* format, ... );
	gpwstring gpwstringf( const wchar_t* format, ... );

//////////////////////////////////////////////////////////////////////////////

#endif  // __GPSTRING_H

//////////////////////////////////////////////////////////////////////////////
