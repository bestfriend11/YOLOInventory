//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoInventory.h
// Author(s):  Chad Queen, Bartosz Kijanka, Scott Bilas
//
// Summary  :  Contains the inventory component for Go's.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOINVENTORY_H
#define __GOINVENTORY_H

//////////////////////////////////////////////////////////////////////////////

#include "Go.h"
#include "ui_gridbox.h"

class PContentReq;

//////////////////////////////////////////////////////////////////////////////
// documentation

/*
	Inventory conventions:

	-	"stored" items

		a list of -all- the items the character has

	-	"active" items

		these are stored items which have been made ready for immediate use by
		the actor. by design, there is a max of 4 items an actor can have in the
		active list.

	-	"selected active"

		from the active list, the character can have one item selected. this is
		the item they will use for any default behavior. for example, if a
		weapon is selected, they will attack with the weapon, if a spell is
		selected, they will attack with the spell.
*/

//////////////////////////////////////////////////////////////////////////////
// class GoInventory declaration

class GoInventory : public GoComponent
{
public:
	SET_INHERITED( GoInventory, GoComponent );

// Types.

	// constants
	enum
	{
		MAX_ACTIVE_SIZE = IL_ACTIVE_COUNT,
		MAX_SPELLS      = 12,
	};

	// spewing support
	enum eSpewType
	{
		SPEW_GROUND,		// spew pinata onto ground
		SPEW_DELETE,		// delete, don't spew
		SPEW_IGNORE,		// just leave it where it is
	};

// Setup.

	// ctor/dtor
	GoInventory( Go* parent );
	GoInventory( const GoInventory& source, Go* parent );
	virtual ~GoInventory( void );

// Interface.

FEX eAnimStance GetAnimStance( void );
FEX int			GetAnimStanceInt( void )						{ return ( (int)GetAnimStance() ); }

	// equipment query
FEX bool       IsPackOnly            ( void ) const				{  return ( m_IsPackOnly );  }
FEX Go*        GetEquipped           ( eEquipSlot slot ) const;
FEX eEquipSlot GetEquippedSlot       ( Goid item ) const;
FEX eEquipSlot GetEquippedSlot       ( const Go* item ) const;
FEX bool       IsEquipped            ( const Go* item ) const	{  return ( GetEquippedSlot( item ) != ES_NONE );  }
FEX bool       IsSlotEquipped        ( eEquipSlot slot ) const	{  return ( GetEquipped( slot ) != NULL );  }
FEX bool       IsActiveSpell         ( const Go* item ) const;
FEX bool	   IsMeleeWeaponEquipped ( void ) const;
FEX bool	   IsRangedWeaponEquipped( void ) const;
FEX bool	   IsAnyWeaponEquipped   ( void ) const { return( IsMeleeWeaponEquipped() || IsRangedWeaponEquipped() ); }

	// $ selected magic alignment deals with which magic alignment is currently
	//   active in the spell book gui

	// inventory query
FEX bool               Contains                 	( const Go* item ) const;
FEX eInventoryLocation GetLocation              	( const Go* item ) const;
FEX bool			   LocationContainsTemplate		( eInventoryLocation il, const gpstring & sTemplate );
FEX Go*                GetItem                  	( eInventoryLocation il ) const;
FEX	Go*				   GetItemFromTemplate			( const gpstring & sTemplate ) const;
FEX Go*				   GetSelectedItem          	( void ) const { return ( m_SelectedItem );  }
FEX eInventoryLocation GetSelectedLocation      	( void ) const { return ( m_SelectedActiveLocation );  }
	const gpstring&    GetSpewEquippedKillCount 	( void ) const;
	eSpewType          GetEquippedItemsSpewType 	( PlayerId forPlayerId ) const;
FEX bool               ListItems                	( eInventoryLocation il, GopColl& items ) const;
FEX bool               ListItems                	( eQueryTrait qt, eInventoryLocation il, GopColl& items ) const;
FEX bool               ListItems                	( QtColl & qtc, eInventoryLocation il, GopColl& items ) const;

	// const query
	bool			   GetDropAtUsePoint			( void ) const;

// Inventory modification.

	#define PUB public:
	#define PRV private:

	// $$$ when equipping/unequipping visible, crossfade from old to new! -sb

	// equipping items
PUB FEX FuBiCookie RSEquip         ( eEquipSlot slot, Goid item, eActionOrigin origin );
PRV FEX FuBiCookie RCEquip         ( eEquipSlot slot, Goid item, eActionOrigin origin );  FUBI_MEMBER_SET( RCEquip, +SERVER );
PUB     bool       Equip           ( eEquipSlot slot, Goid item, eActionOrigin origin );
PUB     bool       TestEquipMinReqs( Go* item ) const;
PUB FEX bool       TestEquip       ( Goid item, eEquipSlot slot ) const;
PUB FEX bool	   TestEquipPassive( Goid item ) const;

	// unequipping items
PUB FEX FuBiCookie RSUnequip ( eEquipSlot slot, eActionOrigin origin )				{  return ( RSEquip( slot, GOID_NONE, origin ) );  }
PRV FEX FuBiCookie RCUnequip ( eEquipSlot slot, eActionOrigin origin )				{  return ( RCEquip( slot, GOID_NONE, origin ) );  }
PRV     bool       Unequip   ( eEquipSlot slot, eActionOrigin origin )				{  return ( Equip( slot, GOID_NONE, origin ) );  }
PUB     bool       UnequipAll( eActionOrigin origin );

	// autoequipping items - do proper location setting work
PUB FEX FuBiCookie RSAutoEquip( eEquipSlot slot, Goid item, eActionOrigin origin, bool bAutoplace );
PUB FEX FuBiCookie RSAutoEquip( eEquipSlot slot, Goid item, eActionOrigin origin )	{  return ( RSAutoEquip( slot, item, origin, false ) );  }
PRV FEX FuBiCookie RCAutoEquip( eEquipSlot slot, Goid item, eActionOrigin origin, bool bAutoplace );  FUBI_MEMBER_SET( RCAutoEquip, +SERVER );
PRV		void	   AutoEquip  ( eEquipSlot slot, Goid item, eActionOrigin origin, bool bAutoplace );

	// selecting items
PUB FEX FuBiCookie RSSelect( eInventoryLocation il, eActionOrigin origin );
PRV FEX FuBiCookie RCSelect( eInventoryLocation il, eActionOrigin origin );  FUBI_MEMBER_SET( RCSelect, +SERVER );
PRV     void       Select  ( eInventoryLocation il, eActionOrigin origin );

	// adding items
PUB FEX FuBiCookie RSAdd ( Go* item, eInventoryLocation il, eActionOrigin origin, bool autoPlace = false );
PRV FEX FuBiCookie RCAdd ( Go* item, eInventoryLocation il, eActionOrigin origin, bool autoPlace = false );  FUBI_MEMBER_SET( RCAdd, +SERVER );
PUB     bool       Add   ( Go* item, eInventoryLocation il, eActionOrigin origin, bool autoPlace = false );
PUB FEX	bool       CanAdd( const Go* item, eInventoryLocation il, bool autoPlace = false ) const;

	// moving items
PUB FEX FuBiCookie RSSetLocation ( Go* item, eInventoryLocation il, bool autoPlace = false );
PRV FEX FuBiCookie RCSetLocation ( Go* item, eInventoryLocation il, bool autoPlace = false );  FUBI_MEMBER_SET( RCSetLocation, +SERVER );
PRV     bool       SetLocation   ( Go* item, eInventoryLocation il, bool autoPlace = false );
PUB     bool       CanSetLocation( const Go* item, eInventoryLocation il, bool autoPlace = false ) const;

	// removing items
PUB FEX FuBiCookie RSRemove( Go* item, bool drop, Go* dropFor );
PUB FEX FuBiCookie RSRemove( Go* item, bool drop )					{ return( RSRemove( item, drop, NULL ) ); }
PRV FEX FuBiCookie RCRemove( Go* item, const SiegePos& dropPos, const Quat& dropQuat, bool bDrop );
PRV FEX FuBiCookie RCRemove( Go* item, const SiegePos& dropPos );
PRV FEX FuBiCookie RCRemove( Go* item );
PRV     bool       Remove  ( Go* item, const SiegePos* dropPos = NULL, const Quat* dropQuat = NULL, bool bDrop = true );

	// world/gui synch
PRV FEX void	   SDeactivateGuiItems();
PRV FEX void	   RCDeactivateGuiItems();

	// transferring items from one inventory to another
PUB FEX FuBiCookie RSTransfer( Go* item, Go* dest, eInventoryLocation il, eActionOrigin origin, bool autoPlace = false );
PRV FEX FuBiCookie RCTransfer( Go* item, Go* dest, eInventoryLocation il, eActionOrigin origin, bool autoPlace = false );
PUB     bool       Transfer  ( Go* item, Go* dest, eInventoryLocation il, eActionOrigin origin, bool autoPlace = false );

	// using items
PUB FEX void	   SBeginUse( Goid item );
PUB FEX void	   RCBeginUse();
PUB FEX bool       SAutoUse( Goid item, eActionOrigin origin, bool bDeleteExpendedItem );
PUB FEX void	   RCRemoveExpendedItem( Goid item );
PUB FEX void	   SEndUse ( Goid item );
PUB FEX void	   RCEndUse();

	// getting items
PUB FEX bool       SAutoGet( Goid item, eActionOrigin origin );
	FEX bool       TestGet ( Goid item, bool silent ) const;
PUB FEX bool       TestGet ( Goid item ) const { return TestGet( item, false ); }

	// gold
	#if !GP_RETAIL
PUB	FEX FuBiCookie RSSetGold   ( int gold );
	#endif
PUB	FEX	void	   SSetGold    ( int gold );
PRV	FEX FuBiCookie RCSetGold   ( int gold );					FUBI_MEMBER_SET( RCSetGold, +SERVER );
PUB		void       SetGold     ( int gold );
PUB	FEX	int        GetGold     ( void ) const					{  return ( m_Gold );  }
PUB	FEX	bool       HasGold     ( void ) const					{  return ( m_Gold > 0 );  }

	#if !GP_RETAIL
PUB	FEX FuBiCookie RSAddGold   ( int gold )						{  return ( RSSetGold( m_Gold + gold ) );  }
	#endif
PUB		void	   SAddGold    ( int gold )						{  SSetGold( m_Gold + gold );	}
PRV	FEX FuBiCookie RCAddGold   ( int gold )						{  return ( RCSetGold( m_Gold + gold ) );  }
PRV		void       AddGold     ( int gold )						{  SetGold( m_Gold + gold );  }
PRV		void       SubtractGold( int gold )						{  AddGold( -gold );  }

	// depositing gold
PUB bool SDepositGold ( Go * item );
PUB void RCDepositGold( Go * item );		// $ only call this from GoGold!!
PRV void DepositGold  ( DWORD amount );

	#undef PUB
	#undef PRV
public:

	// popped the pinata - inventory 'splosion goodness
FEX void DropItemsFor( Goid killerGoid );
FEX void DropItemsAroundBody( void )						{  DropItemsFor( GOID_INVALID );  }
	void RetrieveDropPositionAndOrientation( Go * dropFor, Go * item, SiegePos & spos, Quat & dropQuat );

	// synchronizing gold
	void SSynchronizePartyGold();
FEX	void SSyncRevivedPartyGold();

	// pcontent
FEX void AddDelayedPcontent( void );

	// spells
FEX bool IsSpellInProgress( const gpstring & templateName ) const;

	// adding yet-to-be-created items
FEX	FuBiCookie RSCreateAddItem( eInventoryLocation il, const gpstring & templateName );

// Helpers.

	// weapon
FEX Go*  GetSelectedWeapon      ( void ) const;
FEX Go*  GetSelectedMeleeWeapon ( void ) const;
FEX Go*  GetSelectedRangedWeapon( void ) const;
FEX bool IsMeleeWeaponSelected  ( void ) const;
FEX bool IsRangedWeaponSelected ( void ) const;

	// spell
FEX Go*  GetSelectedSpell( void ) const;
FEX bool IsSpellSelected ( void ) const;

	// shield
FEX Go* GetShield   ( void ) const;
FEX Go* GetBestArmor( void ) const;

	// Custom head support for heros (eg. Ulora)
	bool	        HasCustomHead( void )						{ return m_CustomHeadName.empty();	}
	nema::HAspect   GetCustomHeadAspect( void )					{ return m_CustomHeadAspect;		}
	const gpstring& GetCustomHead( void )						{ return m_CustomHeadName;			}
	bool			SetCustomHead( const gpstring& name);
FEX void			SSetCustomHead( const gpstring& ch );
FEX void			RCSetCustomHead( const gpstring& ch );
	void            UpdateCustomHeadTexture();

	// general
	int GetInventoryDb( GopColl& storedItems ) const;

	// auto-helpers
FEX bool SBeginGive( Goid actor, eActionOrigin origin );
FEX bool SAutoGive( Goid actor, Goid item, eActionOrigin origin );
FEX bool SEndGive( Goid actor, Goid item, eActionOrigin origin );

FEX bool GetForceGet() { return m_bForceGet; }
FEX	void RSSetForceGet( bool bSet );
FEX	void RCSetForceGet( bool bSet );
	void SetForceGet( bool bSet ) { m_bForceGet = bSet; }


// GUI.

	// $ the inventory has a gridbox assigned to this go. it can use the gridbox
	//   to see if various items can fit inside the inventory, and then in turn
	//   add the items to the the go's inventory (and go's gui inventory).

FEX	UIGridbox* GetGridbox   ( void ) const					{  gpassert( m_GridBox != NULL );  return ( m_GridBox );  }
FEX	bool       HasGridbox   ( void ) const					{  return ( m_GridBox != NULL );  }
FEX	void       SetGridbox   ( UIGridbox* box )				{  m_GridBox = box;  }
FEX	int        GetGridWidth ( void ) const					{  return ( m_GridWidth );  }
FEX	int        GetGridHeight( void ) const					{  return ( m_GridHeight ); }

FEX	int		   GetGridAreaMax	( void ) const				{  return ( m_GridWidth * m_GridHeight ); }
FEX	int		   GetGridAreaUsed	( void ) const;
FEX	int		   GetGridAreaFree	( void ) const				{  return ( max( 0, GetGridAreaMax() - GetGridAreaUsed() ) ); }
FEX	float      GetFullRatio		( void ) const;

	void	   ProcessGridboxRequests( void );

	// $ when an update is made to the inventory that requires gui notification,
	//   set the dirty bit here and the callback will be notified next update.

	void SetInventoryDirty( bool set = true );
FEX	void SSetInventoryDirty( bool set );
FEX void RCSetInventoryDirty( bool set );

	bool IsInventoryDirty ( void ) const					{  return ( m_InventoryDirty );  }

	void SetGoldDirty( bool set = true );
	bool IsGoldDirty( void ) const							{  return ( m_GoldDirty ); }

	// Get the name the GameGUI uses to interface with the item
	static gpstring BuildUiName( const Go* item );

	void	SetActiveSpellBook( Go * pBook )				{  gpassert( (pBook == NULL) || Contains( pBook ) );  m_ActiveSpellBook = pBook;  }
	Go*		GetActiveSpellBook( void ) const				{  return ( m_ActiveSpellBook );  }

FEX	void	ReportItemSwitching( Go * pToEquip );

// Special.

	// $ do not call these unless you know what you're doing!
	void UpdateChildrenPositions( void );

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newParent );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );
	virtual bool         XferCharacter( FuBi::PersistContext& persist );

	// special post-xfer support
	bool XferGridbox( FuBi::PersistContext& persist );

	// event handlers
	virtual bool CommitCreation( void );
	virtual void Shutdown      ( void );
	virtual void HandleMessage ( const WorldMessage& msg );
	virtual void Update        ( float deltaTime );
	virtual void UnlinkChild   ( Go* child );

#	if !GP_RETAIL
	virtual bool ShouldSaveInternals( void );
	virtual void SaveInternals      ( FuelHandle fuel );
FEX	void Dump						( bool verbose, ReportSys::Context * ctx );
	void DumpDb( bool verbose, ReportSys::ContextRef ctx = NULL );
#	endif // !GP_RETAIL

	GPDEBUG_ONLY( virtual bool AssertValid( void ) const; )

private:
	enum eGroup
	{
		GROUP_ALL,			// consider all entries
		GROUP_ONEOF,		// consider only one
		GROUP_GOLD,			// this is gold! (special case)
		GROUP_DETECT,		// autodetect it
	};

	void UpdateAnimStance( void );
	void UpdateAspect    ( void );
	int  QueryItems      ( eInventoryLocation il, GopColl* items = NULL ) const;
	bool AutoPlace       ( Go* item );
	bool CanAutoPlace    ( const Go* item ) const;
	bool PrivateEquip    ( eEquipSlot slot, Go* go );
	void PrivateUnequip  ( eEquipSlot slot, bool unlink = true );
	void ClearSelected   ( Go* child );
	void AddPcontent     ( FastFuelHandle group, eGroup type = GROUP_DETECT, bool isStandaloneLoad = false, bool isRootQuery = true );
	void AddStorePcontent( FastFuelHandle base );

	// workhorse function
	Go* LoadItem(	const gpstring& ilText,
					const gpstring& templateText,
					eEquipSlot      slot             = ES_NONE,
					bool            isPContent       = false,
					bool            isStandaloneLoad = false,
					PContentReq*    pcontentReq      = NULL,
					bool			bAutoPlace		 = true,
					Goid            explicitGoid     = GOID_INVALID );

	typedef std::map <Go*, eInventoryLocation> InventoryDb;
	typedef stdx::fast_vector <int> IntColl;
	typedef const Go* CGo;

	// spec
	int              m_GridWidth;						// inventory grid width in squares
	int              m_GridHeight;						// inventory grid height in squares
	bool             m_IsPackOnly;						// true if this is a pack-only object
	RpGoInvRangeColl m_RangeColl;						// range of aspects to use based on load
	GridItemColl	 m_GridItems;						// grid items to process

	// state
	Go*                m_Equipped[ ES_EQUIP_COUNT ];	// a list of all the specific points to which we can equip items
	InventoryDb        m_InventoryDb;					// items in the "bag" so to speak
	eInventoryLocation m_SelectedActiveLocation;		// which active slot has been selected (by the user from the active box)
	int                m_Gold;							// how much gold we've got
	IntColl            m_GoldPiles;						// this is a spec for how gold will be spit out in piles when generated by pcontent
	bool			   m_bForceGet;						// this flag can make TestGet return true no matter what, and makes SAutoGet not autoplace, thereby "forcing" a valid get to take place
	bool               m_IgnoreEquipMinReqs;			// temporarily ignore min equip requirements

	// cache
	Go*				m_SelectedItem;						// currently selected go
	eActionOrigin	m_SelectedItemActionOrigin;			// how it got selected
	eAnimStance		m_CurrentStance;					// stance required by current equipment set
	UIGridbox*		m_GridBox;							// grid box assigned to this go for gui inventory management
	bool			m_InventoryDirty;					// if true, next update will do the callback
	bool			m_GoldDirty;						// if true, next update will synchronize party gold
	Go*				m_ActiveSpellBook;					// stores which of the spell books in the user's inventory that is open

	// Custom head was moved from GoBody to GoInventory	 -- biddle
	bool PrivateSetCustomHead( bool force_wear );
	// custom head mesh to use (if we have one)
	GoString          m_CustomHeadName;
	nema::HAspect     m_CustomHeadAspect;

	static DWORD        GoInventoryToNet( GoInventory* x );
	static GoInventory* NetToGoInventory( DWORD d, FuBiCookie* cookie );

	SET_NO_COPYING( GoInventory );
	FUBI_RPC_CLASS( GoInventory, GoInventoryToNet, NetToGoInventory, "" );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __GOINVENTORY_H

//////////////////////////////////////////////////////////////////////////////
