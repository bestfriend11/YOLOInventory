//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiSchemaImpl.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_FuBi.h"
#include "FuBiSchemaImpl.h"

#include "FuBi.h"
#include "StdHelp.h"

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// class Store implementation

Store :: Store( const Store& other )
	: m_Variables( other.m_Variables ),
	  m_Handles( other.m_Handles )
{
	Strings::const_iterator i, begin = other.m_Strings.begin(), end = other.m_Strings.end();
	for ( i = begin ; i != end ; ++i )
	{
		m_Strings.push_back( new gpstring( **i ) );
	}
}

int Store :: AddLargeVariable( int size, const void* value )
{
	gpassert( (size > 0) && (size < 1000) );

	int index = scast <int> ( m_Variables.size() );
	int size4 = GetDwordAlignUp( size ) / 4;

	if ( value != NULL )
	{
		m_Variables.resize( index + size4 );
		::memcpy( &*(m_Variables.begin() + index), value, size );
	}
	else
	{
		m_Variables.resize( index + size4, 0 );
	}

	return ( index );
}

int Store :: AddGeneric( eVarType type, eColl* coll )
{
	if ( type == VAR_STRING )
	{
		if ( coll != NULL )
		{
			*coll = COLL_STRINGS;
		}
		return ( AddString() );
	}

	// $$$ only POD types supported for now
	gpassert( gFuBiSysExports.IsPod( type ) );

	if ( coll != NULL )
	{
		*coll = COLL_VARIABLES;
	}
	return ( AddLargeVariable( gFuBiSysExports.GetVarTypeSizeBytes( type ) ) );
}

void Store :: FreeStrings( int count )
{
	gpassert( IsValidCount( count, m_Strings ) );
	Strings::iterator end = m_Strings.end(), begin = end - count;
	std::for_each( begin, end, stdx::delete_by_ptr() );
	m_Strings.erase( begin, end );
}

void Store :: FreeHandles( int count )
{
	gpassert( IsValidCount( count, m_Handles ) );

	Handles::iterator i, end = m_Handles.end(), begin = end - count;
	for ( i = begin ; i != end ; ++i )
	{
		i->ReleaseFull();
	}
	m_Handles.erase( begin, end );
}

void Store :: FreeAll( void )
{
	FreeVariables( scast <int> ( m_Variables.size() ) );
	FreeStrings  ( scast <int> ( m_Strings  .size() ) );
	FreeHandles  ( scast <int> ( m_Handles  .size() ) );
}

bool Store :: IsEmpty( void ) const
{
	return ( m_Variables.empty() && m_Strings.empty() && m_Handles.empty() );
}

void* Store :: GetGenericPtr( eColl coll, int index )
{
	switch ( coll )
	{
		case ( COLL_VARIABLES ):  return (  GetVariablePtr( index ) );
		case ( COLL_STRINGS   ):  return ( &GetString     ( index ) );
		case ( COLL_HANDLES   ):  return ( &GetHandle     ( index ) );
	}
	gpassert( 0 );
	return ( NULL );
}

void Store :: SetVariable( int index, const void* value, int size )
{
	gpassert( IsValidIndex( index, m_Variables ) && IsValidIndex( index + (size / 4), m_Variables ) );
	::memcpy( &*(m_Variables.begin() + index), value, size );
}

void Store :: SetHandle( int index, DWORD value )
{
	gpassert( IsValidIndex( index, m_Handles ) );

	HandleEntry& entry = m_Handles[ index ];
	HandleEntry newHandle( entry.m_Type );
	newHandle.Take( value );
	entry.TransferCopy( newHandle );
}

Store& Store :: operator = ( const Store& other )
{
	m_Variables = other.m_Variables;
	m_Handles = other.m_Handles;

	// copy/add strings
	Strings::const_iterator i, begin = other.m_Strings.begin(), end = other.m_Strings.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( (i - begin) == scast <int> ( m_Strings.size() ) )
		{
			m_Strings.push_back( new gpstring( **i ) );
		}
		else
		{
			*m_Strings[ i - begin ] = **i;
		}
	}

	// erase the balance
	Strings::iterator ebegin = m_Strings.begin() + other.m_Strings.size(), eend = m_Strings.end();
	std::for_each( ebegin, eend, stdx::delete_by_ptr() );
	m_Strings.erase( ebegin, eend );

	// done
	return ( *this );
}

//////////////////////////////////////////////////////////////////////////////
// struct Store::HandleEntry implementation

const FuBi::ClassSpec* Store::HandleEntry :: FindClass( void ) const
{
	// get class
	const FuBi::ClassSpec* spec = gFuBiSysExports.FindClass( m_Type );

	// validate
	gpassert( spec != NULL );
	gpassert( spec->m_Flags & FuBi::ClassSpec::FLAG_MANAGED );
	gpassert( spec->m_HandleAddRefProc != NULL );
	gpassert( spec->m_HandleReleaseProc != NULL );
	gpassert( m_Handle != NULL );

	// return it
	return ( spec );
}

void Store::HandleEntry :: Take( DWORD handle )
{
	if ( (m_Handle = handle) != NULL )
	{
		AddRef();
	}
}

void Store::HandleEntry :: TransferCopy( const HandleEntry& other )
{
	ReleaseFull();
	*this = other;
}

void Store::HandleEntry :: AddRef( void )
{
	(*FindClass()->m_HandleAddRefProc)( m_Handle );
	++m_RefCount;
}

void Store::HandleEntry :: ReleaseFull( void )
{
	if ( m_RefCount != 0 )
	{
		const FuBi::ClassSpec* spec = FindClass();
		while ( m_RefCount-- )
		{
			(*spec->m_HandleReleaseProc)( m_Handle );
		}
		m_Handle = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////
// class StoreHeaderSpec implementation

void StoreHeaderSpec :: GetOffset( int index, Store::eColl& coll, int& collIndex ) const
{
	gpassert( (index >= 0) && (index < scast <int> ( m_Locations.size() )) );
	const Location& location = m_Locations[ index ];
	coll      = location.m_Coll;
	collIndex = location.m_Index;
}

int StoreHeaderSpec :: AddColumn( const TypeSpec& type, const char* name, Store::eColl coll, int index  GPDEV_PARAM( const char* docs ) )
{
	m_Locations.push_back( Location( coll, index ) );
    return ( Inherited::AddColumn( type, name  GPDEV_PARAM( docs ) ) );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
