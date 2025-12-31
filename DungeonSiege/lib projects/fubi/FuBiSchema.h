//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiSchema.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains code to represent schemas and other database related
//             structures using the FuBi type system.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FUBISCHEMA_H
#define __FUBISCHEMA_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"
#include "FuBiTraits.h"
#include <vector>

class FuelHandle;
class FastFuelHandle;

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// macro definitions

	/* $$$ what about default value, doco, and constraints? */

// Table macros.

#	define DECLARE_CLASS_TABLE_BEGIN( CLS )														\
		friend FuBi::CreateRecordHelper <CLS>;													\
		friend FuBi::CreateRecordHelper <const CLS>;											\
		FUBI_EXPORT static void __cdecl FUBI_GetHeaderSpec( FuBi::ClassHeaderSpec& FUBI_Spec )	\
		{  typedef CLS FUBI_ThisType;

#	define DECLARE_CLASS_TABLE_END()															\
			FUBI_GetHeaderSpec( &FUBI_Spec );													\
		}																						\
		static FuBi::ClassHeaderSpec& FUBI_GetHeaderSpec( FuBi::ClassHeaderSpec* spec = NULL )	\
		{																						\
			static FuBi::ClassHeaderSpec* s_Spec = spec;										\
			gpassert( s_Spec != NULL );															\
			return ( *s_Spec );																	\
		}

#	define CLASS_TABLE_ADD_COLUMN( NAME, VAR )													\
		FUBI_Spec.AddColumn( ((FUBI_ThisType*)0)->VAR, NAME, offsetof( FUBI_ThisType, VAR ) );

//////////////////////////////////////////////////////////////////////////////
// struct ColumnSpec declaration

struct ColumnSpec : ParamSpec
{
#	if !GP_RETAIL
	gpstring m_Documentation;
#	endif // !GP_RETAIL

	ColumnSpec( const TypeSpec& type );

	void SetName( const gpstring& name );
};

//////////////////////////////////////////////////////////////////////////////
// class HeaderSpec declaration

class HeaderSpec
{
public:
	SET_NO_INHERITED( HeaderSpec );

// Types.

	// collections
	typedef stdx::fast_vector <ColumnSpec> ColumnSpecColl;
	typedef ColumnSpecColl::const_iterator ConstIter;

	// callbacks
	typedef CBFunctor1wRet <const gpstring&, const gpstring&> StringCollCb;

// Setup.

	// ctor/dtor
	HeaderSpec( void )								{  m_Digest = 0;  GPDEV_ONLY( m_Docs = NULL );  }
	HeaderSpec( int count, ... );
   ~HeaderSpec( void )								{  }

	// name
	void            SetName( const gpstring& name );
	const gpstring& GetName( void ) const			{  return ( m_Name );  }

	// special callback
	void SetStringCollectorCb( StringCollCb cb )	{  m_StringCollCb = cb;  }
	StringCollCb GetStringCollectorCb( void ) const	{  return ( m_StringCollCb );  }

	// docs
#	if !GP_RETAIL
	void        SetDocs( const char* docs )			{  m_Docs = docs;  }
	const char* GetDocs( void ) const				{  return ( m_Docs );  }
#	endif // !GP_RETAIL

	// trait-ized helpers
	template <typename T>
	int AddColumn( const T& type, const gpstring& name  GPDEV_PARAM( const gpstring& docs = gpstring::EMPTY ) )
				 {  return ( AddColumn( GetTraits( type ).GetVarType(), name  GPDEV_PARAM( docs ) ) );  }

	// helpers
	int AddColumn( eVarType type, TypeSpec::eFlags flags, const gpstring& name  GPDEV_PARAM( const gpstring& docs = gpstring::EMPTY ) )
				 {  return ( AddColumn( TypeSpec( type, flags ), name  GPDEV_PARAM( docs ) ) );  }
	int AddColumn( eVarType type, const gpstring& name  GPDEV_PARAM( const gpstring& docs = gpstring::EMPTY ) )
				 {  return ( AddColumn( TypeSpec( type ), name  GPDEV_PARAM( docs ) ) );  }
	int AddColumn( const TypeSpec& type, const gpstring& name  GPDEV_PARAM( const gpstring& docs ) );

// Modifications.

	// minor schema changes
#	if !GP_RETAIL
	void SetConstraint    ( int index, ConstraintSpec* spec );
#	endif // !GP_RETAIL
	void SetDefaultValue  ( int index, const gpstring& defValue, bool isCode = false );
	void SetNoDefaultValue( int index );

// Utility.

	// doco
#	if !GP_RETAIL
	void GenerateDocs( ReportSys::ContextRef context = NULL ) const;
#	endif // !GP_RETAIL

// Query.

	// iteration
	ConstIter GetColumnBegin( void ) const					{  return ( m_ColumnSpecs.begin() );  }
	ConstIter GetColumnEnd  ( void ) const					{  return ( m_ColumnSpecs.end  () );  }

	// attributes
	int   GetColumnCount( void ) const						{  return ( scast <int> ( m_ColumnSpecs.size() ) );  }
	bool  HasColumns    ( void ) const						{  return ( GetColumnCount() > 0 );  }
	DWORD GetDigest     ( void ) const;
	bool  CanSkrit      ( void ) const;

	// searching
	int  FindColumn( const char* name ) const;
	bool HasColumn ( const char* name ) const				{  return ( FindColumn( name ) != INVALID_COLUMN_INDEX );  }

	// retrieval
	const ColumnSpec& GetColumn( int index ) const  		{  gpassert( (index >= 0) && (index < GetColumnCount()) );  return ( m_ColumnSpecs[ index ] );  }
	const ColumnSpec& GetColumn( const char* name ) const	{  gpassert( HasColumn( name ) );  return ( m_ColumnSpecs[ FindColumn( name ) ] );  }

	// comparison
	bool operator == ( const HeaderSpec& other ) const;

#	if !GP_RETAIL
	void Dump( bool sort = false, ReportSys::ContextRef ctx = NULL ) const;
#	endif // !GP_RETAIL

private:
	ColumnSpec& PrivateGetColumn( int index )  {  gpassert( (index >= 0) && (index < GetColumnCount()) );  return ( m_ColumnSpecs[ index ] );  }

	mutable DWORD  m_Digest;				// digest of this header
	gpstring       m_Name;					// name of table
	ColumnSpecColl m_ColumnSpecs;			// table definition
	StringCollCb   m_StringCollCb;			// special string collector callback
#	if !GP_RETAIL
	const char*    m_Docs;					// docs for table
#	endif // !GP_RETAIL
};

//////////////////////////////////////////////////////////////////////////////
// class Record declaration

// basic record interface
class Record
{
public:
	SET_NO_INHERITED( Record );

// Setup.

	// ctor/dtor
	Record( const HeaderSpec* spec,
			eXMode xmode = XMODE_DEFAULT )				: m_Spec( spec ), m_XferMode( xmode ), m_QuotedStrings( false )  {  gpassert( m_Spec != NULL );  }
	Record( const Record& other );
	virtual ~Record( void )								{  }

	// query
	const HeaderSpec* GetHeaderSpec( void ) const		{  return ( m_Spec );  }
	int FindColumn( const char* name ) const			{  return ( GetHeaderSpec()->FindColumn( name ) );  }
	bool HasColumn( const char* name ) const			{  return ( GetHeaderSpec()->HasColumn( name ) );  }

	// xfer mode
	void   SetXferMode( eXMode xmode )					{  m_XferMode = xmode;  }
	eXMode GetXferMode( void ) const					{  return ( m_XferMode );  }

	// quoted strings?
	void SetQuotedStrings( bool set = true )			{  m_QuotedStrings = set;  }
	bool GetQuotedStrings( void ) const					{  return ( m_QuotedStrings );  }

	// do xfer
	bool XferFlags( PersistContext& persist );

// Field access.

	// derived callbacks
	virtual void* OnGetFieldPtr( int index ) = 0;
	virtual const void* OnGetFieldPtr( int index ) const = 0;
	virtual Record* OnClone( void ) const = 0;

	// string conversions
	bool GetAsString( int index, gpstring& out, eXfer xfer = XFER_NORMAL ) const;
	bool SetAsString( int index, const char* str );
	bool SetAsString( int index, const gpstring& str );
	bool GetAsString( const char* fieldName, gpstring& out, eXfer xfer = XFER_NORMAL ) const;
	bool SetAsString( const char* fieldName, const char* str );
	bool SetAsString( const char* fieldName, const gpstring& str );

	// basic set/get
	bool GetField( int index, eVarType type, void* obj ) const;
	bool SetField( int index, eVarType type, const void* obj );
	bool GetField( const char* fieldName, eVarType type, void* obj ) const;
	bool SetField( const char* fieldName, eVarType type, const void* obj );

	// typed set/get
	template <typename T>
	bool Get( int index, T& obj ) const						{  return ( GetField( index, Traits <T>::GetVarType(), &obj ) );  }
	template <typename T>
	bool Get( const char* fieldName, T& obj ) const			{  return ( GetField( fieldName, Traits <T>::GetVarType(), &obj ) );  }
	template <typename T>
	bool Set( int index, const T& obj )						{  return ( SetField( index, Traits <T>::GetVarType(), &obj ) );  }
	template <typename T>
	bool Set( const char* fieldName, const T& obj )			{  return ( SetField( fieldName, Traits <T>::GetVarType(), &obj ) );  }

	// forced set/get
	template <typename T>
	const T& GetExact( int index, T* ) const				{  return ( *rcast <const T*> ( OnGetFieldPtr( index ) ) );  }
	template <typename T>
	T& GetExact( int index, T* )							{  return ( *rcast <T*> ( OnGetFieldPtr( index ) ) );  }
	template <typename T>
	const T& GetExact( const char* fieldName, T* ) const	{  return ( *rcast <const T*> ( OnGetFieldPtr( FindColumn( fieldName ) ) ) );  }
	template <typename T>
	T& GetExact( const char* fieldName, T* )				{  return ( *rcast <T*> ( OnGetFieldPtr( FindColumn( fieldName ) ) ) );  }

	// string versions
	bool Set( int index, const char* obj )					{  return ( SetAsString( index, obj ) );  }
	bool Set( const char* fieldName, const char* obj );

	// copying
	void CopyField( int index, const Record& other );

	// comparing
	int CompareField( int index, const Record& other ) const;

	// special
	bool LoadFromFuel  ( FastFuelHandle fuel );
	bool LoadFromString( const char* params );				// must be key1=value1&key2=value2 format

// Documentation.

	// dumping
#	if !GP_RETAIL
	void ReportAsTable( ReportSys::ContextRef ctx = NULL, eXfer xfer = XFER_NORMAL ) const;
	void ReportAsRow  ( ReportSys::ContextRef ctx = NULL, eXfer xfer = XFER_NORMAL ) const;
#	endif // !GP_RETAIL

private:
	const HeaderSpec* m_Spec;
	eXMode            m_XferMode;
	bool              m_QuotedStrings;
};

// Helper methods.

	// hack required because vc++ 6 doesn't properly support friend template functions
	template <typename T>
	struct CreateRecordHelper
	{
		static Record* CreateRecord( T* obj, eXMode xmode = XMODE_DEFAULT )
		{
			Record* record = T::FUBI_GetHeaderSpec().CreateRecord( (void*)obj );
			record->SetXferMode( xmode );
			return ( record );
		}
	};

	// call this to create a record from an object
	template <typename T>
	inline Record* CreateRecord( T* obj, eXMode xmode = XMODE_DEFAULT )
	{
		return ( CreateRecordHelper <T>::CreateRecord( obj, xmode ) );
	}

	// call this to dump a record from an object
	template <typename T>
	inline void ReportAsTable( T* obj, ReportSys::ContextRef ctx = NULL, eXfer xfer = XFER_NORMAL )
	{
		std::auto_ptr <Record> record( CreateRecord( obj, XMODE_QUIET ) );
		record->ReportAsTable( ctx, xfer );
	}

	// call this to dump a record from an object
	template <typename T>
	inline void ReportAsRow( T* obj, ReportSys::ContextRef ctx = NULL, eXfer xfer = XFER_NORMAL )
	{
		std::auto_ptr <const Record> record( CreateRecord( obj, XMODE_QUIET ) );
		record->ReportAsRow( ctx, xfer );
	}

//////////////////////////////////////////////////////////////////////////////
// class ClassHeaderSpec declaration

class ClassHeaderSpec : private HeaderSpec
{
public:
	SET_INHERITED( ClassHeaderSpec, HeaderSpec );
	class Record;

// Setup.

	// ctor/dtor
	ClassHeaderSpec( void )							{  }
   ~ClassHeaderSpec( void )							{  }

	// schema
	int GetOffset( int index ) const				{  gpassert( (index >= 0) && (index < scast <int> ( m_Offsets.size() )) );  return ( m_Offsets[ index ] );  }

	// trait-ized helpers
	template <typename T>
	int AddColumn( const T& type, const char* name, int offset  GPDEV_PARAM( const char* docs = NULL ) )
				 {  return ( AddColumn( GetTraits( type ).GetVarType(), name, offset  GPDEV_PARAM( docs ) ) );  }

	// helpers
	int AddColumn( eVarType type, TypeSpec::eFlags flags, const char* name, int offset  GPDEV_PARAM( const char* docs = NULL ) )
				 {  return ( AddColumn( TypeSpec( type, flags ), name, offset  GPDEV_PARAM( docs ) ) );  }
	int AddColumn( eVarType type, const char* name, int offset  GPDEV_PARAM( const char* docs = NULL ) )
				 {  return ( AddColumn( TypeSpec( type ), name, offset  GPDEV_PARAM( docs ) ) );  }
	int AddColumn( const TypeSpec& type, const char* name, int offset  GPDEV_PARAM( const char* docs ) );

// Query support.

	const HeaderSpec* GetHeaderSpec( void ) const	{  return ( this );  }

	Record* CreateRecord( void* object );

// Doco.

	// doc the schema
#	if !GP_RETAIL
	void GenerateDocs( ReportSys::ContextRef context = NULL );
#	endif // !GP_RETAIL

// Types.

	class Record : public FuBi::Record
	{
	public:
		SET_INHERITED( Record, FuBi::Record );

		Record( void* object, ClassHeaderSpec* spec )
			: Inherited( spec->GetHeaderSpec() )
		{
			gpassert( object != NULL );  m_Object = object;
			gpassert( spec   != NULL );  m_Spec   = spec;
		}

		template <typename T>
		Record( T* object )
			: Inherited( object->FUBI_GetHeaderSpec() )
		{
			gpassert( object != NULL );
			m_Object = object;
			m_Spec = object->FUBI_GetHeaderSpec();
		}

		Record( const Record& other )
			: Inherited( other )
		{
			m_Object = other.m_Object;
			m_Spec = other.m_Spec;
		}

		virtual ~Record( void )  {  }

		virtual void* OnGetFieldPtr( int index )
			{  return ( rcast <BYTE*> ( m_Object ) + m_Spec->GetOffset( index ) );  }
		virtual const void* OnGetFieldPtr( int index ) const
			{  return ( ccast <ThisType*> ( this )->OnGetFieldPtr( index ) );  }
		virtual Inherited* OnClone( void ) const
			{  return ( new Record( *this ) );  }

	private:
		void*            m_Object;
		ClassHeaderSpec* m_Spec;
	};

// Access.

	using Inherited::SetName;
	using Inherited::GetName;
	using Inherited::GetColumnBegin;
	using Inherited::GetColumnEnd;
	using Inherited::GetColumnCount;
	using Inherited::GetDigest;
	using Inherited::CanSkrit;
	using Inherited::FindColumn;
	using Inherited::HasColumn;
	using Inherited::GetColumn;
#	if !GP_RETAIL
	using Inherited::SetDocs;
	using Inherited::GetDocs;
#	endif // !GP_RETAIL

private:
	typedef stdx::fast_vector <int> OffsetColl;

	OffsetColl m_Offsets;
};

inline ClassHeaderSpec::Record* ClassHeaderSpec :: CreateRecord( void* object )
{
	return ( new Record( object, this ) );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

#endif  // __FUBISCHEMA_H

//////////////////////////////////////////////////////////////////////////////
