//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiSchemaImpl.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains specializations of the Schema types.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FUBISCHEMAIMPL_H
#define __FUBISCHEMAIMPL_H

//////////////////////////////////////////////////////////////////////////////

#include "FuBiSchema.h"

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// class Store declaration

class Store
{
public:
	SET_NO_INHERITED( Store );

	enum eColl
	{
		COLL_VARIABLES,
		COLL_STRINGS,
		COLL_HANDLES,
	};

// Setup.

	// ctor/dtor
	Store( void )													{  }
	Store( const Store& other );
   ~Store( void )													{  FreeAll();  }

	// adding vars - each returns an index
	int AddVariable     ( DWORD value = 0 )							{  m_Variables.push_back( value );                  return ( scast <int> ( m_Variables.size() ) - 1 );  }
	int AddString       ( const gpstring& value = gpstring::EMPTY )	{  m_Strings  .push_back( new gpstring( value ) );  return ( scast <int> ( m_Strings  .size() ) - 1 );  }
	int AddHandle       ( eVarType type )							{  m_Handles  .push_back( type  );                  return ( scast <int> ( m_Handles  .size() ) - 1 );  }
	int AddLargeVariable( int size, const void* value = NULL );
	int AddGeneric      ( eVarType type, eColl* coll = NULL );

	// freeing vars
	void FreeVariables( int count )									{  gpassert( IsValidCount( count, m_Variables ) );  m_Variables.erase( m_Variables.end() - count, m_Variables.end() );  }
	void FreeStrings  ( int count );
	void FreeHandles  ( int count );
	void FreeAll      ( void );

// Query.

	// general
	bool IsEmpty( void ) const;

	// getters
	const void*     GetVariablePtr( int index ) const				{  gpassert( IsValidIndex( index, m_Variables ) );  return ( &*(m_Variables.begin() + index) );  }
	void*           GetVariablePtr( int index )						{  gpassert( IsValidIndex( index, m_Variables ) );  return ( &*(m_Variables.begin() + index) );  }
	DWORD           GetVariable   ( int index ) const				{  gpassert( IsValidIndex( index, m_Variables ) );  return (  m_Variables[ index ] );  }
	const gpstring& GetString     ( int index ) const				{  gpassert( IsValidIndex( index, m_Strings   ) );  return ( *m_Strings  [ index ] );  }
	gpstring&       GetString     ( int index )						{  gpassert( IsValidIndex( index, m_Strings   ) );  return ( *m_Strings  [ index ] );  }
	DWORD           GetHandle     ( int index ) const				{  gpassert( IsValidIndex( index, m_Handles   ) );  return (  m_Handles  [ index ].m_Handle );  }
	DWORD&          GetHandle     ( int index )						{  gpassert( IsValidIndex( index, m_Handles   ) );  return (  m_Handles  [ index ].m_Handle );  }
	const void*     GetGenericPtr ( eColl coll, int index ) const	{  return ( ccast <ThisType*> ( this )->GetGenericPtr( coll, index ) );  }
	void*           GetGenericPtr ( eColl coll, int index );

	// setters
	void SetVariable( int index, DWORD value )						{  gpassert( IsValidIndex( index, m_Variables ) );   m_Variables[ index ] = value;  }
	void SetVariable( int index, const void* value, int size );
	void SetString  ( int index, const gpstring& value )			{  gpassert( IsValidIndex( index, m_Strings   ) );  *m_Strings  [ index ] = value;  }
	void SetHandle  ( int index, DWORD value );

	// operators
	Store& operator = ( const Store& other );

private:

	template <typename T>
	bool IsValidIndex( int index, T& coll ) const					{  return ( (index >= 0) && (index < scast <int> ( coll.size() )) );  }
	template <typename T>
	bool IsValidCount( int index, T& coll ) const					{  return ( (index >= 0) && (index <= scast <int> ( coll.size() )) );  }

// Types.

	struct HandleEntry
	{
		DWORD    m_Handle;				// the handle data
		UINT     m_RefCount;			// how many times we've locally addref'd
		eVarType m_Type;				// class type of this handle

		HandleEntry( eVarType type )
		{
			m_Handle   = 0;
			m_RefCount = 0;
			m_Type     = type;
		}

		const FuBi::ClassSpec* FindClass( void ) const;

		void Take        ( DWORD handle );
		void TransferCopy( const HandleEntry& other );
		void AddRef      ( void );
		void ReleaseFull ( void );
	};

	// collection types
	typedef stdx::fast_vector <DWORD>        Variables;
	typedef stdx::fast_vector <my gpstring*> Strings;		// must be to avoid realloc-invalidation for Skrit
	typedef stdx::fast_vector <HandleEntry>  Handles;

// Data.

	// collections
	Variables m_Variables;			// simple variables
	Strings   m_Strings;			// strings
	Handles   m_Handles;			// handles
};

//////////////////////////////////////////////////////////////////////////////
// class StoreHeaderSpec declaration

class StoreHeaderSpec : public HeaderSpec
{
public:
	SET_INHERITED( StoreHeaderSpec, HeaderSpec );

// Setup.

	StoreHeaderSpec( void )							{  }
   ~StoreHeaderSpec( void )							{  }

	// schema
	void GetOffset( int index, Store::eColl& coll, int& collIndex ) const;

	// trait-ized helpers
	template <typename T>
	int AddColumn( const T& type, const char* name, Store::eColl coll, int index  GPDEV_PARAM( const char* docs = NULL ) )
				 {  return ( AddColumn( FuBi::GetTraits( type ).GetVarType(), name, coll, index  GPDEV_PARAM( docs ) ) );  }

	// helpers
	int AddColumn( eVarType type, TypeSpec::eFlags flags, const char* name, Store::eColl coll, int index  GPDEV_PARAM( const char* docs = NULL ) )
				 {  return ( AddColumn( TypeSpec( type, flags ), name, coll, index  GPDEV_PARAM( docs ) ) );  }
	int AddColumn( eVarType type, const char* name, Store::eColl coll, int index  GPDEV_PARAM( const char* docs = NULL ) )
				 {  return ( AddColumn( TypeSpec( type ), name, coll, index  GPDEV_PARAM( docs ) ) );  }
	int AddColumn( const TypeSpec& type, const char* name, Store::eColl coll, int index  GPDEV_PARAM( const char* docs ) );

// Types.

	class Record : public FuBi::Record
	{
	public:
		SET_INHERITED( Record, FuBi::Record );

		Record( Store* store, const StoreHeaderSpec* spec, eXMode xmode = XMODE_DEFAULT )
			: Inherited( spec, xmode )
		{
			gpassert( store != NULL );  m_Store = store;
			gpassert( spec  != NULL );  m_Spec  = spec;
		}

		Record( const Record& other )
			: Inherited( other )
		{
			m_Store = ccast <Store*> ( other.m_Store );
			m_Spec  = other.m_Spec;
		}

		virtual ~Record( void )		{  }

		virtual void* OnGetFieldPtr( int index )
		{
			Store::eColl coll;
			int collIndex;
			m_Spec->GetOffset( index, coll, collIndex );

			Store* store = rcast <Store*> ( m_Store );
			return ( store->GetGenericPtr( coll, collIndex ) );
		}

		virtual const void* OnGetFieldPtr( int index ) const
		{
			return ( ccast <ThisType*> ( this )->OnGetFieldPtr( index ) );
		}

		virtual Inherited* OnClone( void ) const
		{
			return ( new Record( *this ) );
		}

	private:
		Store*                 m_Store;
		const StoreHeaderSpec* m_Spec;
	};

private:

// Types.

	struct Location
	{
		Store::eColl m_Coll;		// which collection?
		int          m_Index;		// index into collection

		Location( Store::eColl coll, int index )
			{  m_Coll = coll;  m_Index = index;  }
	};
	typedef stdx::fast_vector <Location> LocationColl;

// Data.

	LocationColl m_Locations;
};

//////////////////////////////////////////////////////////////////////////////
// class TableSchema declaration

class TableSchema
{
public:
	SET_NO_INHERITED( TableSchema );

	TableSchema( void );
	virtual ~TableSchema( void );

private:
	SET_NO_COPYING( TableSchema );
};

//////////////////////////////////////////////////////////////////////////////
// db

#if 0
struct Table
{
	struct Row
	{
		void*            m_Object;			// object containing data
		TableOffsetSpec* m_TableRowSpec;	// spec of offsets for where data is located
	};

	typedef stdx::fast_vector <void*> RowColl;

	RowColl m_Rows;
};
#endif

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

#endif  // __FUBISCHEMAIMPL_H

//////////////////////////////////////////////////////////////////////////////
