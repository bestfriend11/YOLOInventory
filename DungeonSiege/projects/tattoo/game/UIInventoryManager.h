//////////////////////////////////////////////////////////////////////////////
//
// File     :  UIInventoryManager.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __UIInventoryManager_H
#define __UIInventoryManager_H

#include "FubiDefs.h"
#include "UIGame.h"

#define MAX_WEAPON_CONFIGS 10

class Go;
enum eInventoryLocation;

struct SelectedSlot
{
	eInventoryLocation	eSlotType;
	Goid				member;
};
struct MemberSelectedSlots
{
	SelectedSlot member_slots[MAX_PARTY_MEMBERS];
};


class UIInventoryManager : public Singleton <UIInventoryManager>
{
public:

	UIInventoryManager();
	
	void Init();

	void Update( float secondsElapsed );	

	//********************* Inventory Management *******************

	// For rollover information queries
	Goid	GetGUIRolloverGOID		()						{ return m_gui_rollover_goid;	}
	void		SetGUIRolloverGOID		( Goid object );
	
	// When a user activates a gui item
	void		ActivateItem( Goid item );

	// If the user clicks on an item in the world, Pick up it's GUI bitmap
	bool		PickupUIItem();
	
	// Update inventory on the GUI
	void UpdateInventoryCallback( Goid object );
	void UpdateGridHighlights();
	void TestEquipCallback( Goid actor );
	void TestEquipment();
	
	// Drag all selected items to the actor	
	void DragSelectedItemsToActor( Goid actor );	

	// Drop all selected items from the actore
	void DropSelectedItemsFromActor();	
	
	// The user is placing an item on a gridbox
	void GridItemPlacement( UIGridbox * pGrid );

	// The user is picking up an item on a gridbox
	void GridItemPickup( UIGridbox * pGrid );

	// The user has drag selected multiple items, left clicked on them ( which brings up all the little inventory bitmaps ),
	// and has now clicked on their gridbox gui
	void GridMultipleItemDrag( UIGridbox * pGrid );

	// The user is requesting to pick something up
	void GridRequestPickup( UIGridbox * pGrid );
	void GridRequestPlacement( UIGridbox * pGrid );

	void GridItemAutoplaced( UIGridbox * pGrid );

	void CalculateGridHighlights( UIGridbox * pGrid );
	void CalculateAllGridHighlights();

	// The user has right clicked on an item in the gridbox, so try to use it
	void GridUseItem( UIGridbox * pGrid );
	bool GridLearnSpell( Go * pSpell, Go * pSpellBook );

	void ItemslotUseItem( UIItemSlot * pSlot );

	void ItemslotEquip( UIItemSlot * pSlot );

	void ItemslotRequestRemove( UIItemSlot * pSlot );

	void InfoslotEquip( UIInfoSlot * pSlot );	

	void InfoslotRequestRemove( UIInfoSlot * pSlot );

	// The user's cursor rolls over/off a gridbox item
	void GridItemRollover( UIGridbox * pGrid );
	void GridItemRolloff( UIGridbox * pGrid );

	// For management of gold transfer
	// Pickup gold and attach GUI bitmap to cursor
	void GoldTransfer( Goid transferer );
	void GoldTransferStash();
	void GoldTransferSelectedMember();
	void ButtonGoldTransferInc();
	void ButtonGoldTransferDec();
	void ButtonGoldTransferComplete();
FEX	void RSGoldTransferComplete( Go * pMember, int gold );
FEX	void RCGoldTransferComplete( Go * pMember );

	// Display the rollover box
	void AwpSlotRollover( UIWindow * pWindow );
	void AwpSlotRolloff( UIWindow * pWindow );
	void DisplayRolloverItemData( Goid id, UITextBox * pBox );	
	void DisplayWorldRolloverItemData( Goid item );

	void ConvertPersistItems( UIGridbox * pGrid );

FEX	FuBiCookie RSCombinePotions( Go * pSource, Go * pDest, Go * pMember, float fillAmount, float amountLeft );
FEX	FuBiCookie RCCombinePotions( Goid source, Goid dest, Go * pMember, float fillAmount, float amountLeft );
FEX	FuBiCookie RCCombinePotionsDest( Goid dest, Go * pMember, float fillAmount, float amountLeft );

	void ItemCreationCb( int item );

	//********************* Equipment Management *******************

	// Construct the paper doll elements
	bool SetAsEquipped( Go * pMember, Go * item, const gpstring sSlot, bool bForce = false );

	// Update paper doll equipment with any new equipment while paper doll is visible
	void UpdateActiveEquipment( int party_index, bool bForce = false );

	// Handle equipping/unequipping items on the paperdoll using the gui
	void PaperDollHandleItem( eEquipSlot slot, Goid inventory_item = GOID_INVALID );
FEX	void RSPaperDollHandleItem( Goid member, eEquipSlot slot, Goid inventory_item );
	
	// The user's cursor rolls over/off a itemslot item
	void SlotRollover( UIWindow * pSlot );
	void SlotRolloff( UIWindow * pSlot );

	// The user is removing an item from an itemslot ( usually paperdoll equip slot )
	void ItemslotRemoveItem( UIWindow * pSlot );
	
	// The user is placing an item in an itemslot
	void ItemslotPlaceItem( UIItemSlot * pSlot );

	void InfoslotRemoveItem( UIInfoSlot * pSlot );
	void InfoslotPlaceItem( UIInfoSlot * pSlot );

	// When user clicks on the quick weapon change radio group GUI
	void ActivateMeleeWeapon( int charIndex );
	void ActivateRangedWeapon( int charIndex );
	void ActivatePrimarySpell( int charIndex );
	void ActivateSecondarySpell( int charIndex );
	eInventoryLocation ActivateNextValid( int charIndex, bool bForwards = true );

	// When the user manipulates the paperdoll region
	void ListenerCharacterRollover();
	void ListenerCharacterRolloff();

	void ContextPlaceItem( bool bUseRollover = false, Goid member = GOID_INVALID );

	// For hot-keying weapon/spell configurations
	void InitWeaponsConfig( int charIndex );

	bool GetWeaponsConfig1();
	bool GetWeaponsConfig2();
	bool GetWeaponsConfig3();
	bool GetWeaponsConfig4();
	bool GetWeaponsConfig5();
	bool GetWeaponsConfig6();
	bool GetWeaponsConfig7();
	bool GetWeaponsConfig8();
	bool GetWeaponsConfig9();
	bool GetWeaponsConfig0();

	void GetWeaponsConfig( int index );

	void GetWeaponBinding( Goid weapon, gpstring & sSave, gpstring & sRecall );

	bool SetWeaponsConfig1();
	bool SetWeaponsConfig2();
	bool SetWeaponsConfig3();
	bool SetWeaponsConfig4();
	bool SetWeaponsConfig5();
	bool SetWeaponsConfig6();
	bool SetWeaponsConfig7();
	bool SetWeaponsConfig8();
	bool SetWeaponsConfig9();
	bool SetWeaponsConfig0();

	void Xfer( FuBi::PersistContext& persist );

	bool AwpSlot1();
	bool AwpSlot2();
	bool AwpSlot3();
	bool AwpSlot4();

	void SetWeaponsConfig( int index );

	bool RotateSelectedSlots( bool bForwards = true );
	bool RotateSelectedSlotsPrev();

	bool RotatePrimarySpells();	
	bool RotateSecondarySpells();

	void RotateSpellCheck( bool bPrimarySlot );
FEX	void RSRotateSpells( bool bPrimarySlot, Goid member );	
FEX	void RCRotateSpells( Goid member );

	//********************* Magic Management ***********************
	
	void DisplayMagic();	
	void UpdateMagic();
	void ButtonEquipSpell( UIButton * pButton );		// Set one of the learned spells as active
	void ButtonUnEquipSpell( UIButton * pButton );		// Transform the learned spell back into a scroll	
	void ListSpells( UIWindow * pWindow );
	void ListSelectSpell( UIWindow * pWindow );
	void HideSpellList();
	void ListenerSpellBookRollover();
	void ListenerSpellBookRolloff();
	
	//********************* Trade Management ***********************
	
	// initiate trade between two clients
	FEX void RSReqInitiateTrade( Go * pSource, Go * pDest, Go * pItem );
	FEX void RCAckInitiateTrade( Go * pSource, Go * pDest, Go * pItem, DWORD initiateID, DWORD machineId );
	FEX void RCPlayerBusyDialog( DWORD machineID );

	// cancel trade between two clients
	FEX void RSCancelTrade( Go * pSource, Go * pDest );	
	FEX void RCCancelTrade( DWORD machineId );

	FEX void RSAttemptTradeCompletion( Go * pSource, Go * pDest );
	FEX void RCAttemptTradeCompletion( Go * pSource, Go * pDest );
	FEX void RSSetTradeValid( Go * pSource );
	FEX void RSSetTradeInvalid( Go * pSource );

	FEX void RSTradeFailed( Go * pSource, Go * pDest );
	FEX void RCTradeFailed( DWORD machineId );

	FEX void RSTradeZeroGold( Go * pSource, Go * pDest );
	FEX void RCTradeZeroGoldSource( DWORD machineId );
	FEX void RCTradeZeroGoldDest( DWORD machineId );

	// complete trade between two clients
	FEX void RSCompleteTrade( Go * pSource, Go * pDest );	
	FEX void RCCompleteTrade( Go * pSource, Go * pDest );	

	FEX void RSTradeAccept( bool bAccept, Player * sourcePlayer, Player * destPlayer );	
	FEX void RCSetTradeSourceAccept	( bool bAccept, DWORD machineId );
	FEX void RCSetTradeDestAccept	( bool bAccept, DWORD machineId );
	bool GetTradeSourceAccept()											{ return m_bTradeSourceAccept; }	
	bool GetTradeDestAccept()											{ return m_bTradeDestAccept; }
	
	// In our destination gridbox, we want to show our remote player what's in our client's source gridbox.
	void ProcessAddToDestGridbox();
	FEX void RSAddToDestGridbox		( GridItem item, Goid destTrade, Goid sourceTrade );
	FEX void RCAddToDestGridbox		( GridItem item, DWORD machineDestId );	
	FEX void RSRemoveFromDestGridbox( int item, Player * destPlayer );
	FEX void RCRemoveFromDestGridbox( int item, Player * destPlayer );
	
	Go * GetSourceTrade();
	Go * GetDestTrade();

	bool GetIsCompletingTrade() { return m_bCompletingTrade; }

	//********************* Visualization ***********************

	void SetGridInvalidColor( DWORD dwColor ) { m_dwGridInvalidColor = dwColor; }
	DWORD GetGridInvalidColor() { return m_dwGridInvalidColor; }

	void SetActiveBookColor( DWORD dwColor ) { m_dwActiveBookColor = dwColor; }
	DWORD GetActiveBookColor() { return m_dwActiveBookColor; }

	void SetGridRolloverColor( DWORD dwColor ) { m_dwGridRollover = dwColor; }
	DWORD GetGridRolloverColor() { return m_dwGridRollover; }

	void SetGridRolloverOverlapColor( DWORD dwColor ) { m_dwGridRolloverOverlap = dwColor; }
	DWORD GetGridRolloverOverlapColor() { return m_dwGridRolloverOverlap; }

	void SetGridMagicColor( DWORD dwColor ) { m_dwGridMagicColor = dwColor; }
	DWORD GetGridMagicColor() { return m_dwGridMagicColor; }

	//********************* Stash Support ***********************

FEX	void SActivateStash( Goid member, Goid stashActivator );
FEX	void RCActivateStash( Goid member, Goid stashActivator, DWORD machineId );
	void ActivateStash( Goid member, Goid stashActivator );
	void SwitchMemberStash( Goid member );
	void SetStashActive( bool bSet )	{ m_bStashActive = bSet;	}
	bool GetStashActive()				{ return m_bStashActive;	}
	void SetStash( Goid stash )			{ m_Stash = stash;			}
	Goid GetStash()						{ return m_Stash;			}		
	Goid GetStashMember();
	Goid GetStashActivator();
	void CloseStash();	
	void StashGuiCb( Goid member, Goid item );
	void HandleStashSwitch( Goid newMember );

private:

	bool					m_bTradeDestAccept;
	bool					m_bTradeSourceAccept;
	bool					m_bLearnedSpell;
	bool					m_bEquippedStaffPrimary;
	bool					m_bEquippedStaffSecondary;
	Goid					m_DestTrade;
	Goid					m_SourceTrade;
	Goid					m_floating_item;
	Goid					m_gui_rollover_goid;
	MemberSelectedSlots		m_WeaponConfigs[MAX_WEAPON_CONFIGS];
	DWORD					m_dwGridInvalidColor;
	DWORD					m_dwActiveBookColor;
	DWORD					m_dwGridRollover;
	DWORD					m_dwGridRolloverOverlap;
	DWORD					m_dwGridMagicColor;
	Goid					m_UpdateClassGoid;
	Goid					m_UpdateGridActor;
	bool					m_bUpdatingInventory;
	bool					m_bCompletingTrade;
	bool					m_bRefreshSpells;
	bool					m_bSpellBookBusy;	
	eInventoryLocation		m_ItemLocationToSelect;
	GoidColl				m_TestEquipActors;
	bool					m_bTestEquipSkip;
	Goid					m_GoldTransferer;

	// Stash related Variables
	bool					m_bStashActive;	
	Goid					m_Stash;

	FUBI_SINGLETON_CLASS( UIInventoryManager, "This is the UIInventoryManager singleton" );

};


#define gUIInventoryManager UIInventoryManager::GetSingleton()


#endif