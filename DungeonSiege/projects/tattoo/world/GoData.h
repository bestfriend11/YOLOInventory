//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoData.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains data containment classes for the Game Object system.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GODATA_H
#define __GODATA_H

//////////////////////////////////////////////////////////////////////////////

#include "Fuel.h"
#include "GoDefs.h"
#include "GpColl.h"
#include "SkritDefs.h"

//////////////////////////////////////////////////////////////////////////////
// class GoDataComponent declaration

// $ this class should have no derivatives

class GoDataComponent
{
public:
	SET_NO_INHERITED( GoDataComponent );

// Types.

	enum eFlags
	{
		FLAG_NONE        =      0,
		FLAG_REQUIRED    = 1 << 0,		// this parameter cannot accept a default value
		FLAG_LOCALIZE    = 1 << 1,		// this parameter refers to a string that will need to be localized (probably because it appears on screen)
		FLAG_QUOTED      = 1 << 2,		// strings saved with this flag will always be quoted
		FLAG_ADVANCED    = 1 << 3,		// advanced params are meant for more detailed options that aren't normally used (this will reduce clutter in the SE dialogs)
		FLAG_HIDDEN      = 1 << 4,		// hidden tweaky params for engineering use
		FLAG_DEV         = 1 << 5,		//
	};

	enum eType
	{
		TYPE_NONE,						// uninitialized
		TYPE_GENERIC,					// from gas block
		TYPE_SKRIT,						// from skrit
	};

	typedef stdx::fast_vector <gpstring> StringColl;
	typedef stdx::fast_vector <eFlags> FlagColl;

// Setup.

	// ctor/dtor
	GoDataComponent( void );
	GoDataComponent( const GoDataComponent& other );
   ~GoDataComponent( void );

	// type
	eType GetType          ( void ) const							{  return ( GetImplForRead()->m_Type );  }
	bool  IsSchema         ( void ) const							{  return ( GetType() == TYPE_GENERIC );  }
	bool  IsSkrit          ( void ) const							{  return ( GetType() == TYPE_SKRIT );  }
	bool  SkritWantsUpdates( void ) const							{  gpassert( IsSkrit() );  return ( GetImplForRead()->m_SkritWantsUpdates );  }
	bool  SkritWantsDraws  ( void ) const							{  gpassert( IsSkrit() );  return ( GetImplForRead()->m_SkritWantsDraws   );  }

	// construction
	bool InitSchema( FastFuelHandle fuel, bool serverOnly, bool canServerExistOnly, FuBi::StoreHeaderSpec* storeHeaderSpec, FlagColl* flagColl );
	bool InitSchema( Skrit::HObject skrit, bool& serverOnly, FuBi::StoreHeaderSpec* storeHeaderSpec, FlagColl* flagColl );

	// specialization
	bool SpecializeSchema  ( FastFuelHandle fuel );
	bool SpecializeInternal( FastFuelHandle fuel, bool ignoreSchemaChanges = false );
	bool SpecializeData    ( FastFuelHandle fuel, bool ignoreSchemaChanges = false );

// Query.

	// component query
	bool                         IsInitialized      ( void ) const	{  return ( GetType() != TYPE_NONE );  }
	bool                         IsServerOnly       ( void ) const	{  return ( GetImplForRead()->m_ServerOnly );  }
	bool                         CanServerExistOnly ( void ) const	{  return ( GetImplForRead()->m_CanServerExistOnly );  }
	const gpstring&              GetName            ( void ) const;
	const FuBi::HeaderSpec*      GetHeaderSpec      ( void ) const;
	const FuBi::StoreHeaderSpec* GetStoreHeaderSpec ( void ) const	{  return ( GetImplForRead()->m_StoreHeaderSpec );  }
	const FuBi::Store*           GetStore           ( void ) const	{  gpassert( GetImplForRead()->m_Store != NULL );  return ( GetImplForRead()->m_Store );  }
	FuBi::Store*                 GetStoreForWrite   ( void )		{  gpassert( GetImplForWrite()->m_Store != NULL );  return ( GetImplForWrite()->m_Store );  }
	const FuBi::Record*          GetRecord          ( void ) const	{  gpassert( GetImplForRead()->m_Record != NULL );  return ( GetImplForRead()->m_Record );  }
	FuBi::Record*                GetRecordForWrite  ( void )		{  gpassert( GetImplForWrite()->m_Record != NULL );  return ( GetImplForWrite()->m_Record );  }
	Skrit::HObject               GetSkritObject     ( void ) const;
	void                         CopyForWrite       ( void );

#	if !GP_RETAIL
	const gpstring& GetDocs( void ) const							{  return ( GetImplForRead()->m_Docs );  }
#	endif // !GP_RETAIL

	// field query
	eFlags GetFlags( int fieldIndex ) const							{  gpassert( (fieldIndex >= 0) && (fieldIndex < (int)GetImplForRead()->m_FlagColl->size()) );  return ( (*GetImplForRead()->m_FlagColl)[ fieldIndex ] );  }
	eFlags GetFlags( const char* fieldName ) const;

	// $$$ GetRecord() directly cannot work - what if the GetT() should retrieve
	//     a default skrit?

	// internal query
	FastFuelHandle GetInternalField( const char* name ) const		{  const FastFuelHandle* fh = FindInternal( name );  return ( (fh != NULL) ? *fh : FastFuelHandle() );  }

	// collections
#	if !GP_RETAIL
	const StringColl& GetRequiredColl( void ) const					{  return ( GetImplForRead()->m_RequiredColl );  }
#	endif // !GP_RETAIL

	// overriding
#	if !GP_RETAIL
	const BitVector& GetOverrideColl( void ) const					{  return ( GetImplForRead()->m_OverrideColl );  }
#	endif // !GP_RETAIL

private:

	struct Impl;

// Private methods.

#	if !GP_RETAIL
	bool AddInternal( const gpstring& name );
#	endif // !GP_RETAIL

	FastFuelHandle*		  FindInternal( const char* name );
	const FastFuelHandle* FindInternal( const char* name ) const	{  return ( ccast <ThisType*> ( this )->FindInternal( name ) );  }	// $ functionally the same as non-const

	Impl*       GetImplForWrite( void )								{  CopyForWrite();  return ( m_Impl );  }
	const Impl* GetImplForRead ( void ) const						{  return ( m_Impl );  }

// Types.

#	if GP_RETAIL
	typedef stdx::fast_vector <FastFuelHandle> InternalColl;
#	else
	typedef stdx::linear_map <gpstring, FastFuelHandle, istring_less> InternalColl;
#	endif // GP_RETAIL

	struct Impl
	{
		// spec
		eType              m_Type;						// dynamic type of this component
		InternalColl       m_InternalColl;				// fuel blocks that define 'internal' parameters
		Skrit::HAutoObject m_SkritObject;				// skrit representing this component
		bool               m_SkritWantsUpdates  : 1;	// this is a skrit and it wants updates
		bool               m_SkritWantsDraws    : 1;	// this is a skrit and it wants draws
		bool               m_ServerOnly         : 1;	// this is a server-only component
		bool               m_CanServerExistOnly : 1;	// this component is ok to exist only on the server

		// variable storage
		const FlagColl*              m_FlagColl;		// flags for each field
		const FuBi::StoreHeaderSpec* m_StoreHeaderSpec;	// schema spec for this component
		my FuBi::Store*              m_Store;			// variable storage for this component
		my FuBi::Record*             m_Record;			// record for variable access

		// extra
#		if !GP_RETAIL
		gpstring   m_Docs;								// docs for entire component
		StringColl m_RequiredColl;						// set of required components that must be used alongside this one
		BitVector  m_OverrideColl;						// bitfield of fields that have been overridden on instance specialization
#		endif // !GP_RETAIL

		// impl-specific
		mutable int m_RefCount;							// number of outstanding read-only refs on this

		// methods
		Impl( void );
		Impl( Impl& other );
	   ~Impl( void );

		// setup
		void Init( FuBi::StoreHeaderSpec* storeHeaderSpec, FlagColl* flagColl );
	};

// Data.

	my Impl* m_Impl;

	// disable operator =
	GoDataComponent& operator = ( const GoDataComponent& );
};

MAKE_ENUM_BIT_OPERATORS( GoDataComponent::eFlags );

void ToString  ( gpstring& out, GoDataComponent::eFlags flags );
bool FromString( const char* str, GoDataComponent::eFlags& obj );

//////////////////////////////////////////////////////////////////////////////
// class GoDataTemplate declaration

// $ this class should have no derivatives

class GoDataTemplate
{
public:
	SET_NO_INHERITED( GoDataTemplate );

	typedef stdx::fast_vector <GoDataComponent> ComponentColl;
	typedef ComponentColl::const_iterator ComponentIter;
	typedef stdx::fast_vector <GoDataTemplate*> ChildColl;
	typedef ChildColl::const_iterator ChildIter;

// Setup.

	// ctor/dtor
	GoDataTemplate( FastFuelHandle fuel );
   ~GoDataTemplate( void );

	// setup
	bool InitBase( void );				// pre-init step to set up parent/child relationships
	bool Init( void );					// main init
	void PrepFailureDeletion( void );	// this failed to construct for whatever reason - clean up parent/child relationships

#	if !GP_RETAIL
	void PrepMetaGroupDeletion( const GoDataTemplate* oldGodt, const GoDataTemplate* newGodt );
#	endif // !GP_RETAIL

	// specialization support
	int  AddRef      ( bool serialize = true );
	int  Release     ( bool serialize = true );
	int  GetRefCount ( void ) const								{  return ( m_RefCount );  }
	bool IsReferenced( void ) const								{  return ( GetRefCount() > 0 );  }

// Query.

	// standard query
	FastFuelHandle        GetFuelHandle      ( void ) const		{  return ( m_Fuel );  }
	const char*           GetName            ( void ) const		{  return ( m_Fuel.GetName() );  }
	const GoDataTemplate* GetBaseDataTemplate( void ) const		{  return ( m_BaseDataTemplate );  }
	bool                  IsRootTemplate     ( void ) const		{  return ( m_BaseDataTemplate == NULL );  }
	bool                  IsLeafTemplate     ( void ) const		{  return ( m_ChildColl.empty() );  }

	// component query
	const GoDataComponent* FindComponentByName( const char* name ) const;

	// iterators
	ChildIter     GetChildBegin    ( void ) const				{  return ( m_ChildColl.begin() );  }
	ChildIter     GetChildEnd      ( void ) const				{  return ( m_ChildColl.end  () );  }
	ComponentIter GetComponentBegin( void ) const				{  gpassert( IsReferenced() );  return ( m_ComponentColl.begin() );  }
	ComponentIter GetComponentEnd  ( void ) const				{  gpassert( IsReferenced() );  return ( m_ComponentColl.end  () );  }

	// dev function query
#	if !GP_RETAIL
	const gpstring& GetDocs         ( void ) const				{  gpassert( IsReferenced() );  return ( m_Docs );  }
	const gpstring& GetExtraDocs    ( void ) const				{  gpassert( IsReferenced() );  return ( m_ExtraDocs );  }
	const char*     GetMetaGroupName( void ) const				{  gpassert( IsReferenced() );  return ( (m_MetaGroup != NULL) ? m_MetaGroup->GetName() : GetName() );  }
	const char*     GetGroupName    ( void ) const				{  gpassert( IsReferenced() );  return ( (m_Group != NULL) ? m_Group->GetName() : GetName() );  }
	const gpstring& GetCategoryName ( void ) const				{  gpassert( IsReferenced() );  return ( m_CategoryName );  }
#	endif // !GP_RETAIL

private:
	ComponentColl::iterator findComponentByName( const char* name );

	// spec always available
	FastFuelHandle        m_Fuel;				// fuel block of this spec
	const GoDataTemplate* m_BaseDataTemplate;	// template that this specializes (NULL if none)
	ChildColl             m_ChildColl;			// set of templates that specialize this one

	// state
	int                   m_RefCount;			// references on this template, low word is actual refs, high word is pending refs
	ComponentColl         m_ComponentColl;		// vector of components used in this template
#	if !GP_RETAIL
	gpstring              m_Docs;				// docs for this template
	gpstring              m_ExtraDocs;			// extra info for this template (should be fairly descriptive)
	gpstring              m_CategoryName;		// name of the category this template belongs to
	const GoDataTemplate* m_Group;				// NULL if this template defines a group for its branch, otherwise points to group lead
	const GoDataTemplate* m_MetaGroup;			// NULL if this template defines a metagroup for its branch, otherwise points to metagroup lead
#	endif // !GP_RETAIL

	SET_NO_COPYING( GoDataTemplate );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __GODATA_H

//////////////////////////////////////////////////////////////////////////////
