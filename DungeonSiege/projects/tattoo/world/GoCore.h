//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoCore.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains core Go components.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOCORE_H
#define __GOCORE_H

//////////////////////////////////////////////////////////////////////////////

#include "Go.h"
#include "Quat.h"
#include "Siege_Pos.h"
#include "Trigger_Defs.h"


//////////////////////////////////////////////////////////////////////////////
// class Membership declaration

class Membership
{

public:

	Membership();

	Membership( Membership const & membership );

	~Membership();

	typedef stdx::fast_vector< DWORD > SetData;

	void Add( gpstring const & memberList );

FEX	bool Contains( gpstring const & member ) const;

FEX	bool ContainsAny( Membership const & membership ) const;
FEX	bool ContainsAll( Membership const & membership ) const;

	// Persistence
	bool Xfer( FuBi::PersistContext& persist, const char* key );
	void Xfer( FuBi::BitPacker& packer );

	void Clear( void ) { m_Data.clear(); }
	bool IsEmpty( void ) const { return ( m_Data.empty() ); }

	static void StaticInit( void );
	static void StaticXfer( FuBi::PersistContext& persist );

private:

	void PrivateAdd( gpstring const & member );

	SetData m_Data;

	// global infos

	typedef std::map< gpstring, DWORD, istring_less > MemberMap;
	static MemberMap m_MemberMap;
};


//////////////////////////////////////////////////////////////////////////////
// class GoCommon declaration

class GoCommon : public GoComponent
{
public:
	SET_INHERITED( GoCommon, GoComponent );

// Setup.

	// ctor/dtor
	GoCommon( Go* go );
	GoCommon( const GoCommon& source, Go* newGo );
	virtual ~GoCommon( void );

// Interface.

	// const query
	bool            GetIsPContentAllowed	( void ) const;
	bool            GetAllowModifiers		( void ) const;
	bool            GetDisplayRollover		( void ) const;
	bool			GetDisplayRolloverLife	( void ) const;
	const gpstring& GetRolloverHelpKey		( void ) const;
#	if !GP_RETAIL
	const gpstring& GetDevInstanceText		( void ) const;
#	endif // !GP_RETAIL

	// screen name
private:			 // $ only call from skrit!
FEX void             SSetScreenName       ( const gpstring& name )	{  SSetScreenName( ::ToUnicode( name ) );  }
FEX void             SetScreenName        ( const gpstring& name )	{  SetScreenName ( ::ToUnicode( name ) );  }
public:
	void             SSetScreenName       ( const gpwstring& name );
FEX FuBiCookie       RCSetScreenName      ( const gpwstring& name );
	void             SetScreenName        ( const gpwstring& name )	{  m_ScreenName = name;  }
	const gpwstring& GetScreenName        ( void ) const			{  return ( m_ScreenName );  }
FEX	void             GetScreenName        ( gpstring& name ) const	{  name = ::ToAnsi( GetScreenName() );  }
	gpwstring	     GetTemplateScreenName( void );
	const gpwstring& GetNameGender        ( void ) const			{  return ( m_NameGender ); }

	// only store this information locally.  Don't want little billy to 
	// see a spellbook dropped by someone else titled "Anal Intrusion Attack Spells". No sir.
	void			 SetCustomName		  ( const gpwstring& name )	{ m_CustomName = name;		}
	const gpwstring& GetCustomName		  ( void ) const			{ return ( m_CustomName );	}

	// description
	const gpwstring& GetDescription	( void ) const					{  return ( m_Description ); }

	// expiration
FEX	const gpstring& GetForcedExpirationClass( void ) const;
FEX	const gpstring& GetAutoExpirationClass  ( void ) const;

	// triggers
	bool              CanExpireTriggers();
	bool			  HasInstanceTriggers     ( void ) const		{  return ( m_InstanceTriggers != NULL );  }
	trigger::Storage& GetValidInstanceTriggers( void );
	bool              HasTemplateTriggers     ( void ) const		{  return ( m_TemplateTriggers != NULL );  }
	void			  RefreshTriggers		  ( void );

FEX	Membership const & GetMembership( void ) const					{  return ( m_Membership ); }
FEX	void			SCopyMembership( Goid go );
FEX	void			RCCopyMembership( Goid go );
	void			CopyMembership( const Membership& membership );
	void			CopyMembership( Goid go );
FEX void			SRestoreLastMembership( void );
FEX void			RCRestoreLastMembership( void );
FEX void			RestoreLastMembership( void )					{  m_Membership = m_Membership_last;  m_Membership_last.Clear();  }
	bool			IsMembershipDefault( void ) const				{  return ( m_Membership_last.IsEmpty() );  }

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newGo );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );
	virtual bool         XferCharacter( FuBi::PersistContext& persist );

	// event handlers
	virtual void HandleMessage     ( const WorldMessage& msg );
	virtual void Update            ( float deltaTime );
	virtual void DetectWantsBits   ( void );
	virtual bool CanServerExistOnly( void );
	virtual bool CommitCreation    ( void );

	virtual void XferForSync( FuBi::BitPacker& packer );

#	if !GP_RETAIL
	virtual bool ShouldSaveInternals( void );
	virtual void SaveInternals      ( FuelHandle fuel );
	virtual void DrawDebugHud       ( void );

	void DrawDebugTriggers( void );
	void PadDebugLabelHelper(const trigger::Trigger* in_trig, const char* in_padding, gpstring& out_lbl1, gpstring& out_lbl2) const;
#	endif // !GP_RETAIL

private:

	void UpdateOrDetect( float* deltaTime = NULL );

	// spec
	GowString m_ScreenName;
	GowString m_Description;
	gpwstring m_NameGender;
	gpwstring m_CustomName;
	bool      m_OkToSelfDestruct;

	// state
	my trigger::Storage* m_TemplateTriggers;
	my trigger::Storage* m_InstanceTriggers;
	Membership           m_Membership;
	Membership           m_Membership_last;

	static DWORD     GoCommonToNet( GoCommon* x );
	static GoCommon* NetToGoCommon( DWORD d, FuBiCookie* cookie );

	SET_NO_COPYING( GoCommon );
	FUBI_RPC_CLASS( GoCommon, GoCommonToNet, NetToGoCommon, "" );
};

//////////////////////////////////////////////////////////////////////////////
// class GoPlacement declaration

class GoPlacement : public GoComponent
{
public:
	SET_INHERITED( GoPlacement, GoComponent );

// Setup.

	typedef stdx::fast_vector<SiegePos> UsePointColl;

	// ctor/dtor
	GoPlacement( Go* go );
	GoPlacement( const GoPlacement& source, Go* newGo );
	virtual ~GoPlacement( void );

// Placement.

	// special test for special effects done from GoTarget::GetPlacement in WorldFx.cpp to avoid node validation
	bool IsGuiGo	( void ) const;

	// set position, auto-adjusting
FEX void RCSetPosition( const SiegePos& pos );  FUBI_MEMBER_SET( RCSetPosition, +SERVER );
FEX void SSetPosition ( const SiegePos& pos, bool snapToTerrain = true );
	void SetPosition  ( const SiegePos& pos, bool snapToTerrain = true );

	// const query
	const SiegePos& GetOriginalPosition         ( void ) const;
	bool            HasMovedFromOriginalPosition( void ) const;
	bool            DoesInheritPlacement        ( void ) const;

	// set it to this position, with no adjusting
	void ForceSetPosition( const SiegePos& pos );

	// force recalc of position related infoz
	void ResetPlacement( void )								{  PrivateSetPlacement( &GetPosition(), &GetOrientation(), true );  }
	void ResetPosition ( void )								{  PrivateSetPlacement( &GetPosition(), NULL, true );  }

	// verification
#	if !GP_RETAIL
	bool VerifyPosition( void ) const;
#	endif // !GP_RETAIL

	// set orientation
FEX void RCSetOrientation( const Quat& quat );  FUBI_MEMBER_SET( RCSetOrientation, +SERVER );
FEX void SSetOrientation ( const Quat& quat );
	void SetOrientation  ( const Quat& quat );

	// set both at once
	void RCSetPlacement    ( DWORD machineId, const SiegePos& pos, const Quat& quat );
FEX void RCSetPlacementPeek( SiegePos pos, Quat quat );  FUBI_MEMBER_SET( RCSetPlacementPeek, +SERVER );
FEX void SSetPlacement     ( const SiegePos& pos, const Quat& quat, bool snapToTerrain = true );
	void SetPlacement      ( const SiegePos& pos, const Quat& quat, bool snapToTerrain = true );

	// set it to this position, with no adjusting
	void ForceSetPlacement( const SiegePos& pos, const Quat& quat );
	void ForceSetPlacement( const SiegePos* pos, const Quat* quat );

	// copy different types of placements
	void CopyPosition   ( const GoPlacement& source )		{  ForceSetPosition( source.GetPosition() );  }
	void CopyOrientation( const GoPlacement& source )		{  SetOrientation( source.GetOrientation() );  }
	void CopyPlacement  ( const GoPlacement& source )		{  CopyPosition( source );  CopyOrientation( source );  }

// Utility.

	// snap to terrain
	void SnapToTerrain( void )								{  SetPosition( GetPosition(), true );  }

	// query
FEX const SiegePos&          GetPosition    ( void ) const;
FEX const SiegePos&          GetRawPosition ( void ) const	{  return ( m_Position );  }
FEX const Quat&              GetOrientation ( void ) const;
FEX float                    GetLiquidHeight( void ) const	{  return ( m_LiquidHeight );  }
FEX siege::eLogicalNodeFlags GetLiquidFlags ( void ) const	{  return ( m_LiquidFlags  );  }

	// utility
	void Get2DDirection( vector_3& out ) const				{  m_Orientation.RotateVector( out, vector_3::NORTH ); }

	// special
	const vector_3& GetWorldPosition          ( void ) const	{  if ( GetGo()->IsSpaceDirty() )  {  PrivateCalcWorldPosition();  }  return ( m_WorldPosCache );  }
FEX bool            GetIsInVisibleNode        ( void ) const;
FEX bool            GetIsNodeInAnyWorldFrustum( void ) const;	// slightly different from IsInAnyWorldFrustum() check. doesn't look at membership bits, only looks at the node i'm on.
	bool            GetIsPlacementDirty       ( void ) const	{  return ( m_PlacementDirty );  }

	// use points
	bool            HasUsePoints( void ) const;				// If the Go has moved, the HasUsePoints will return false
	int             GetUsePoints( UsePointColl& pts ) const;

	// drop points
	bool            HasDropPoints( void ) const;			// If the Go has moved, the HasDropPoints will return false
	int             GetDropPoints( UsePointColl& pts ) const;

// Lighting.

	// lighting
	void UpdateLightingCache( void );

	// liquid info
	void UpdateLiquidInfo( void );

	// lighting query
	short GetRenderIndexCache1( void ) const				{  return ( m_RenderIndexCache[ 0 ] );  }
	short GetRenderIndexCache2( void ) const				{  return ( m_RenderIndexCache[ 1 ] );  }
	short GetRenderIndexCache3( void ) const                {  return ( m_RenderIndexCache[ 2 ] );  }

// Overrides.

	// required overrides
	virtual int          GetCacheIndex ( void );
	virtual GoComponent* Clone         ( Go* newGo );
	virtual bool         Xfer          ( FuBi::PersistContext& persist );
	virtual bool         CommitCreation( void );
	virtual bool         CommitLoad    ( void );
	virtual void         HandleMessage ( const WorldMessage& msg );

	// event handlers
	GPDEBUG_ONLY( virtual bool AssertValid( void ) const; )

	// special debugging
#	if !GP_RETAIL
	static void SetIsRendering( bool set = true )			{  gpassert( ms_IsRendering != set );  ms_IsRendering = set;  }
#	endif // !GP_RETAIL

private:
	void PrivateCalcPosition     ( SiegePos& pos, bool snapToTerrain );
	void PrivateSetPlacement     ( const SiegePos* pos, const Quat* quat, bool alwaysUpdate = false );
	void PrivateCalcWorldPosition( void ) const;

	const gpstring& GetUsePoints( void ) const;
	const gpstring& GetDropPoints( void ) const;

	SiegePos				 m_Position;				// current position
	Quat					 m_Orientation;				// current orientation
	short					 m_RenderIndexCache[ 3 ];	// siege lighting cache info
	float					 m_LiquidHeight;			// height of the liquid here (0.0f if none)
	siege::eLogicalNodeFlags m_LiquidFlags;				// logical node flags for the liquid
	mutable vector_3		 m_WorldPosCache;			// cached world position
	bool					 m_PlacementDirty;			// has placement changed since this was created? (for scids only)

#	if !GP_RETAIL
	static bool ms_IsRendering;							// true if we're rendering now (so no change placement!!)
#	endif // !GP_RETAIL

	static DWORD        GoPlacementToNet( GoPlacement* x );
	static GoPlacement* NetToGoPlacement( DWORD d, FuBiCookie* cookie );

	SET_NO_COPYING( GoPlacement );
	FUBI_RPC_CLASS( GoPlacement, GoPlacementToNet, NetToGoPlacement, "" );
};

//////////////////////////////////////////////////////////////////////////////
// class GoGui declaration

class GoGui : public GoComponent
{
public:
	SET_INHERITED( GoGui, GoComponent );

// Setup.

	// ctor/dtor
	GoGui( Go* parent );
	GoGui( const GoGui& source, Go* parent );
	virtual ~GoGui( void );

// Interface.

	// const query
FEX	const gpstring& GetLastInventoryIcon    ( void ) const;		// get icon 2 or 1 depending on which is available
FEX	const gpstring& GetActiveIcon           ( void ) const;		// bitmap displayed in quick-select window
FEX	bool            IsSpellBook             ( void ) const;
FEX	int             GetInventoryMaxStackable( void ) const;		// maximum items of this type that may be stacked in an inventory slot
FEX	eEquipSlot      GetEquipSlot            ( void ) const;		// equipment slot(s) this object/weapon uses
FEX	bool			IsDroppable				( void ) const;
FEX	bool			IsLoreBook				( void ) const;
FEX	const gpstring& GetLoreKey				( void ) const;

	// minimum required skill for equipping
	int             GetEquipRequirements( EquipRequirementColl& coll ) const;
FEX	const gpstring& GetEquipRequirements( void ) const;
FEX	void            SetEquipRequirements( const gpstring& requirements );
FEX	void            AddEquipRequirements( const gpstring& requirements );

	// inventory icon
FEX	const gpstring& GetInventoryIcon( void ) const				{  return ( m_InventoryIcon );  }
FEX	void            SetInventoryIcon( const gpstring& icon );

	// inventory dimensions
FEX	int  GetInventoryWidth ( void ) const						{  return ( m_InventoryWidth );  }
FEX	int  GetInventoryHeight( void ) const						{  return ( m_InventoryHeight );  }
FEX	void SetInventoryWidth ( int width );
FEX	void SetInventoryHeight( int height );

	// tooltips
FEX	const gpstring& GetToolTipColorName( void ) const;
FEX	DWORD           GetToolTipColor    ( void ) const;
	gpwstring       MakeToolTipString  ( void ) const;

	// power level
FEX	void  SetItemPower( float power )							{  gpassert( power >= 0 );  m_ItemPower = power;  }
FEX	float GetItemPower( void ) const							{  return ( m_ItemPower );  }

	// identification
FEX	void SetIsIdentified( bool bSet )							{  m_bIdentified = bSet;  }
FEX	bool GetIsIdentified( void ) const							{  return ( m_bIdentified );  }

	// variation
FEX	void            SetVariation( const gpstring& name )		{  m_Variation = name;  }
FEX	const gpstring& GetVariation( void ) const					{  return ( m_Variation.GetString() );  }

	// buy/sell
FEX	bool GetCanSell()					{ return m_bCanSell; }
FEX void SetCanSell( bool bCanSell )	{ m_bCanSell = bCanSell; }

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newParent );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );

private:
	GoString m_InventoryIcon;		// bitmap displayed when dragging or in inventory
	int      m_InventoryWidth;		// width in boxes inside inventory
	int      m_InventoryHeight;		// height in boxes inside inventory
	GoString m_EquipRequirements;	// string describing the requirements necessary to equip this item
	float    m_ItemPower;			// power of this item according to pcontent rules
	bool	 m_bIdentified;			// is this object identified
	GoString m_Variation;			// variation of this object (when created via pcontent)
	bool	 m_bCanSell;			// can this item be sold in stores?

	SET_NO_COPYING( GoGui );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __GOCORE_H

//////////////////////////////////////////////////////////////////////////////
