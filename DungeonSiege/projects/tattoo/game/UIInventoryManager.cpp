//////////////////////////////////////////////////////////////////////////////
//
// File     :  UIInventoryManager.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "Precomp_Game.h"
#include "aiquery.h"
#include "appmodule.h"
#include "UIEmoteList.h"
#include "UIInventoryManager.h"
#include "UIPartyManager.h"
#include "UIGame.h"
#include "GoInventory.h"
#include "ui.h"
#include "ui_shell.h"
#include "world.h"
#include "UICommands.h"
#include "UIMenuManager.h"
#include "server.h"
#include "GoDb.h"
#include "GoDefs.h"
#include "GoActor.h"
#include "GoAttack.h"
#include "GoCore.h"
#include "GoMind.h"
#include "GoAspect.h"
#include "GoMagic.h"
#include "GoDefend.h"
#include "GoStore.h"
#include "UIStoreManager.h"
#include "ui_editbox.h"
#include "ui_item.h"
#include "ui_infoslot.h"
#include "ui_tab.h"
#include "ui_textbox.h"
#include "Enchantment.h"
#include "player.h"
#include "server.h"
#include "UICommands.h"
#include "GuiHelper.h"
#include "GoDefs.h"
#include "GoData.h"
#include "AiAction.h"
#include "siege_mouse_shadow.h"
#include "Config.h"
#include "job.h"
#include "contentdb.h"
#include "GoSupport.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "FuBiTraitsImpl.h"
#include "siege_compass.h"
#include "WorldState.h"
#include "gps_manager.h"
#include "GoStash.h"


UIInventoryManager::UIInventoryManager()
{
	gWorld.RegisterInventoryUpdateCallback( makeFunctor( *this, &UIInventoryManager::UpdateInventoryCallback ) );
	gWorld.RegisterTestEquipCallback( makeFunctor( *this, &UIInventoryManager::TestEquipCallback ) );
	gWorld.RegisterStashGuiCallback( makeFunctor( *this, &UIInventoryManager::StashGuiCb ) );
	gUIShell.RegisterItemCreationCb( makeFunctor( *this, &UIInventoryManager::ItemCreationCb ) );
	Init();
}


void UIInventoryManager::Init()
{
	m_DestTrade				  = 0;           
	m_SourceTrade             = 0;           
	m_bTradeDestAccept        = false;       
	m_bTradeSourceAccept      = false;       
	m_bLearnedSpell           = false;       
	m_bEquippedStaffPrimary   = false;       
	m_bEquippedStaffSecondary = false;       
	m_UpdateClassGoid         = GOID_INVALID;
	m_floating_item           = GOID_INVALID;
	m_gui_rollover_goid       = GOID_INVALID;
	m_UpdateGridActor         = GOID_INVALID;
	m_bUpdatingInventory	  = false;
	m_bCompletingTrade		  = false;
	m_bSpellBookBusy		  = false;		
	m_bTestEquipSkip		  = false;
	m_Stash					  = GOID_INVALID;			
	m_bStashActive			  = false;	
	m_GoldTransferer		  = GOID_INVALID;

	for ( int i = 0; i != MAX_WEAPON_CONFIGS; ++i )
	{
		for ( int j = 0; j != MAX_PARTY_MEMBERS; ++j )
		{
			m_WeaponConfigs[i].member_slots[j].eSlotType = IL_INVALID;
			m_WeaponConfigs[i].member_slots[j].member = GOID_INVALID;
		}
	}
}


void UIInventoryManager::Update( float /*secondsElapsed*/ )
{
	m_bLearnedSpell = false;
	if ( gUIShell.GetRolloverItemslotID() != 0 )
	{
		SetGUIRolloverGOID( MakeGoid( gUIShell.GetRolloverItemslotID() ) );
	}

	if ( m_UpdateClassGoid != GOID_INVALID )
	{
		GoHandle hObject( m_UpdateClassGoid );
		if ( hObject.IsValid() )
		{
			gRules.UpdateClassDesignation( m_UpdateClassGoid );
		}
		m_UpdateClassGoid = GOID_INVALID;
	}

	bool bCancelled = false;
	if ( ::IsMultiPlayer() && GetSourceTrade() )
	{	
		if ( !GetDestTrade() || !GetDestTrade()->IsInActiveWorldFrustum() || 
			 !::IsAlive( GetSourceTrade()->GetAspect()->GetLifeState() ) || !::IsAlive( GetDestTrade()->GetAspect()->GetLifeState() ) || 
			 GetSourceTrade()->GetLifeState() == LS_ALIVE_UNCONSCIOUS || GetDestTrade()->GetLifeState() == LS_ALIVE_UNCONSCIOUS )
		{
			if ( !GetDestTrade()->IsInActiveWorldFrustum() )
			{
				gpscreen( $MSG$ "Cannot initiate trade with distant players." );
			}

			RSCancelTrade( GetSourceTrade(), GetDestTrade() );
			bCancelled = true;
		}
	}

	if ( ::IsServerLocal() && ::IsMultiPlayer() && !bCancelled )
	{		
		Server::PlayerColl::iterator iPlayer1;
		Server::PlayerColl::iterator iPlayer2;
		Server::PlayerColl players = gServer.GetPlayers();

		for ( iPlayer1 = players.begin(); iPlayer1 != players.end(); ++iPlayer1 )
		{
			if ( IsValid(*iPlayer1) && (*iPlayer1)->GetIsTrading() )
			{
				for ( iPlayer2 = players.begin(); iPlayer2 != players.end(); ++iPlayer2 )
				{
					if ( *iPlayer2 && (*iPlayer2)->GetIsTrading() )
					{
						if ( (*iPlayer1)->GetTradeSource() == (*iPlayer2)->GetTradeDest() )
						{
							if ( (*iPlayer1)->GetTradeState() == Player::TRADE_INVALID ||
								 (*iPlayer2)->GetTradeState() == Player::TRADE_INVALID )
							{
								(*iPlayer1)->SetTradeState( Player::TRADE_NONE );
								(*iPlayer2)->SetTradeState( Player::TRADE_NONE );
								
								RSTradeFailed( GoHandle( (*iPlayer1)->GetTradeSource() ), GoHandle( (*iPlayer1)->GetTradeDest() ) );
	
								RSTradeAccept( false, (*iPlayer1), (*iPlayer2) );
								RSTradeAccept( false, (*iPlayer2), (*iPlayer1) );
							}
							else if ( (*iPlayer1)->GetTradeState() == Player::TRADE_VALID &&
									  (*iPlayer2)->GetTradeState() == Player::TRADE_VALID )
							{
								(*iPlayer1)->SetTradeState( Player::TRADE_NONE );
								(*iPlayer2)->SetTradeState( Player::TRADE_NONE );								
								RSCompleteTrade( GoHandle( (*iPlayer1)->GetTradeSource() ), GoHandle( (*iPlayer1)->GetTradeDest() ) );
							}
						}
					}
				}				
			}
		}				
	}

	UpdateGridHighlights();
	TestEquipment();

	if ( m_bRefreshSpells )
	{
		UpdateMagic();
		m_bRefreshSpells = false;
	}

	if ( GetStashActive() )
	{
		GoHandle hStash( GetStash() );
		if ( hStash )
		{
			// Set the gold value
			UIText * pGold = (UIText *)gUIShell.FindUIWindow( "stash_gold", "party_stash" );
			gpwstring sGold;
			sGold.assignf( L"%d", hStash->GetInventory()->GetGold() );
			pGold->SetText( sGold.c_str() );
		}
	}
}


// Sets the cursor texture to the item underneath the cursor
bool UIInventoryManager::PickupUIItem()
{
	GopColl::iterator i, ibegin = gGoDb.GetMouseShadowedGosBegin(), iend = gGoDb.GetMouseShadowedGosEnd();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( (*i)->HasGui() && ( ((*i)->HasActor() && (*i)->GetActor()->GetCanBePickedUp()) || !(*i)->HasActor() ) )
		{
			if ( !gUIShell.IsInterfaceVisible( "character" ) )
			{
				if ( (*i)->IsSelected() )
				{
					gUIGame.SetDragged( gGoDb.GetSelection() );

					GopColl::const_iterator j, jbegin = gGoDb.GetSelectionBegin(), jend = gGoDb.GetSelectionEnd();
					for ( j = jbegin ; j != jend ; ++j )
					{
						if ( ((*j)->HasActor() && (*j)->GetActor()->GetCanBePickedUp()) || !(*j)->HasActor() )
						{
							UIItem *pItem = GetItemFromGO( *j );
							if ( pItem )
							{
								pItem->SetItemID( MakeInt( (*j)->GetGoid() ) );
							}
						}
					}

					GoHandle hActor( gUIPartyManager.GetSelectedCharacter() );
					if ( hActor->GetInventory() && hActor->GetInventory()->GetGridbox() )
					{
						hActor->GetInventory()->GetGridbox()->SetMultipleItems( true );
					}
				}
				else
				{
					gUIGame.SetDragged( (*i)->GetGoid() );
				
					UIItem *pItem = GetItemFromGO( *i );
					if ( pItem )
					{
						pItem->SetItemID( MakeInt( (*i)->GetGoid() ) );
					}			
				}			
			}
			else
			{
				gUIGame.SetDragged( (*i)->GetGoid() );
				gUIGame.GetUICommands()->CommandGet( (*i)->GetGoid(), false, SOURCE_ACTOR, true );
			}

			// Set the mode
			gUIGame.SetBackendMode( BMODE_DRAG_ITEM );
			gUIGame.SetSelectionBox( false );
			break;
		}
	}

	return ( i != iend );
}


// Find what the user has and equip it in their slots
bool UIInventoryManager::SetAsEquipped( Go * pMember, Go * item, const gpstring sSlot, bool bForce )
{
	UIItemVec item_vec;
	gUIShell.FindActiveItems( item_vec );
	if ( item_vec.size() != 0 )
	{	
		if ( item )
		{
			return true;
		}
		
		UIWindow * pWindow = gUI.GetGameGUI().FindUIWindow( sSlot, "character" );
		if ( pWindow )
		{
			UIItemSlot * pSlot = (UIItemSlot *)pWindow;
			UIItem * pItem = gUIShell.GetItem( pSlot->GetItemID() );
			if ( pItem )
			{
				pItem->RemoveItemParent( pSlot );
			}
			pSlot->ClearItem();
			pSlot->SetSpecialActive( false );			
		}	
		return false;
	}

	UIItem *pItem = 0;

	if( item )
	{
		pItem = GetItemFromGO( item );
		pItem->ActivateItem( false );
	}

	UIWindow *pWindow = gUI.GetGameGUI().FindUIWindow( sSlot, "character" );

	if ( item && pWindow && pItem )
	{
		UIItemSlot	*pSlot	= (UIItemSlot *)pWindow;

		if ( bForce )
		{
			pSlot->SetJustPlaced( false );
		}
		UIItem *pUIItem = (UIItem *)pItem;
		pUIItem->SetItemID( MakeInt( item->GetGoid() ) );

		UIItem * pOldItem = gUIShell.GetItem( pSlot->GetItemID() );
		if ( pOldItem )
		{
			pOldItem->RemoveItemParent( pSlot );
		}

		pSlot->PlaceItem( (UIItem *)pItem, false );

		if ( item->HasMagic() && item->GetMagic()->HasNonInnateEnchantments() )
		{
			pSlot->SetSpecialActive( true );
			pSlot->SetSpecialColor( GetGridMagicColor() );
		}
		else
		{
			pSlot->SetSpecialActive( false );
		}

		return true;
	}
	else if( !item )
	{
		UIItemSlot	*pSlot	= (UIItemSlot *)pWindow;

		GoHandle hItem( MakeGoid( pSlot->GetItemID() ) );
		if ( !(hItem.IsValid() && hItem->HasParent() && hItem->GetParent() == pMember) )
		{
			UIItem * pItem = gUIShell.GetItem( pSlot->GetItemID() );
			if ( pItem )
			{
				pItem->RemoveItemParent( pSlot );
			}
			pSlot->ClearItem();
			pSlot->SetSpecialActive( false );
		}		
	}
	else
	{
		gpassertm( 0, "chad: unhandled case?" );
	}

	return false;
}


void UIInventoryManager::UpdateActiveEquipment( int party_index, bool bForce )
{
	if ( party_index == gUIPartyManager.GetSelectedCharacterIndex() || bForce )
	{
		GoHandle hMember( gUIPartyManager.GetSelectedCharacter() );

		if ( bForce )
		{
			hMember = gUIPartyManager.GetSelectedCharacterFromIndex( party_index )->GetGoid();
		}

		UIWindow * pWindow = 0;
		bool bSuccess = false;

		bSuccess = SetAsEquipped( hMember, hMember->GetInventory()->GetEquipped( ES_FEET ), "boots", bForce );
		pWindow = gUIShell.FindUIWindow( "ghost_boots", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( !bSuccess );
		}

		bSuccess = SetAsEquipped( hMember, hMember->GetInventory()->GetEquipped( ES_FOREARMS ),	"gauntlets", bForce  );
		pWindow = gUIShell.FindUIWindow( "ghost_gauntlets", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( !bSuccess );
		}

		bSuccess = SetAsEquipped( hMember, hMember->GetInventory()->GetEquipped( ES_CHEST ),	"armor", bForce  );
		pWindow = gUIShell.FindUIWindow( "ghost_armor", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( !bSuccess );
		}

		bSuccess = SetAsEquipped( hMember, hMember->GetInventory()->GetEquipped( ES_HEAD ),		"helmet", bForce  );
		pWindow = gUIShell.FindUIWindow( "ghost_helmet", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( !bSuccess );
		}

		bSuccess = SetAsEquipped( hMember, hMember->GetInventory()->GetEquipped( ES_AMULET ),	"amulet", bForce  );
		pWindow = gUIShell.FindUIWindow( "ghost_amulet", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( !bSuccess );
		}

		bSuccess = SetAsEquipped( hMember, hMember->GetInventory()->GetEquipped( ES_RING_0 ),	"ring1", bForce  );
		pWindow = gUIShell.FindUIWindow( "ghost_ring_1", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( !bSuccess );
		}

		bSuccess = SetAsEquipped( hMember, hMember->GetInventory()->GetEquipped( ES_RING_1 ),	"ring2", bForce  );
		pWindow = gUIShell.FindUIWindow( "ghost_ring_2", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( !bSuccess );
		}

		bSuccess = SetAsEquipped( hMember, hMember->GetInventory()->GetEquipped( ES_RING_2 ),	"ring3", bForce  );
		pWindow = gUIShell.FindUIWindow( "ghost_ring_3", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( !bSuccess );
		}

		bSuccess = SetAsEquipped( hMember, hMember->GetInventory()->GetEquipped( ES_RING_3 ),	"ring4", bForce  );
		pWindow = gUIShell.FindUIWindow( "ghost_ring_4", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( !bSuccess );
		}

		bSuccess = SetAsEquipped( hMember, hMember->GetInventory()->GetEquipped( ES_SPELLBOOK ), "spellbook", bForce  );
		pWindow = gUIShell.FindUIWindow( "ghost_book", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( !bSuccess );
		}

		bSuccess = SetAsEquipped( hMember, hMember->GetInventory()->GetItem( IL_ACTIVE_MELEE_WEAPON ), "melee_weapon", bForce  );
		pWindow = gUIShell.FindUIWindow( "ghost_melee", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( !bSuccess );
		}

		bSuccess = SetAsEquipped( hMember, hMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON ),"ranged_weapon", bForce  );
		pWindow = gUIShell.FindUIWindow( "ghost_ranged", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( !bSuccess );
		}

		bSuccess = SetAsEquipped( hMember, hMember->GetInventory()->GetItem( IL_SHIELD ), "shield", bForce  );
		pWindow = gUIShell.FindUIWindow( "ghost_shield", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( !bSuccess );
		}		
	}
}


// Whenever the user interacts with the paper doll and the
// item the character possesses within it, carry out the
// appropriate action ( equip, unequip )
void UIInventoryManager::PaperDollHandleItem( eEquipSlot slot, Goid inventory_item )
{
	GoHandle hItem( inventory_item );
	GoHandle hMember( gUIGame.GetActorWhoCarriesObject( hItem->GetGoid() ) );

	// If the user is dragging items into the paper doll from the inventory
	if ( hMember.IsValid() && hItem.IsValid() && gUIGame.GetBackendMode() == BMODE_DRAG_INVENTORY )
	{
		gUIGame.SetDragged( inventory_item );		
		RSPaperDollHandleItem( hMember->GetGoid(), slot, inventory_item );
	}

	gUIGame.GetDraggedColl().clear();
}


void UIInventoryManager::RSPaperDollHandleItem( Goid member, eEquipSlot slot, Goid inventory_item )
{
	FUBI_RPC_THIS_CALL( RSPaperDollHandleItem, RPC_TO_SERVER );

	GoHandle hMember( member );
	GoHandle hItem( inventory_item );

	if ( hMember.IsValid() && hItem.IsValid() )
	{
		bool bClear = hMember->GetMind()->IsCurrentActionHumanInterruptable();			
		hMember->GetMind()->RSDoJob( MakeJobReq( JAT_EQUIP, JQ_ACTION, bClear ? QP_CLEAR : QP_BACK, AO_USER, inventory_item, slot ) );
	}
}	


void UIInventoryManager::GridItemPlacement( UIGridbox * pGrid )
{
	CalculateGridHighlights( pGrid );

	// Let's see if the grid we're placing on is different than the last grid;
	// if it is, let's take the player and give that item to the other person
	GoHandle hObject( MakeGoid(pGrid->GetLastItem().itemID) );

	GoHandle hOwner;
	{
		// Let's see who owns the current gridbox
		GopColl partyColl;
		Go * pParty = gServer.GetScreenParty();
		if ( pParty )
		{
			partyColl = pParty->GetChildren();
		}

		GopColl::iterator i;
		for ( i = partyColl.begin(); i != partyColl.end(); ++i ) 
		{
			if ( !(*i)->HasActor() )
			{
				continue;
			}

			GoHandle hMember( (*i)->GetGoid() );
			if ( hMember->GetInventory()->GetGridbox() == pGrid ) 
			{
				if ( hMember->GetGoid() != gUIGame.GetActorWhoCarriesObject( MakeGoid(pGrid->GetLastItem().itemID) ) ) 
				{
					gUIGame.GetUICommands()->CommandGive( MakeGoid(pGrid->GetLastItem().itemID), (*i)->GetGoid() );
				}
				hOwner = (*i)->GetGoid();
				break;
			}
		}

		if ( hOwner.IsValid() )
		{
			int charIndex = 0;
			if ( gUIPartyManager.GetCharacterFromGoid( hOwner->GetGoid(), charIndex ) )
			{
				gUIPartyManager.SetSelectedCharacterIndex( charIndex );
			}
		}
	}
	// This item has a new parent
	UIItem * pItem = gUIShell.GetItem( pGrid->GetLastItem().itemID );
	if ( pItem )
	{
		pItem->SetParentGridbox( pGrid );
	}

	// If the character is placing an item into the store
	if ( pGrid->GetStore() ) 
	{		
		gUIStoreManager.SellItemToActiveStore( MakeGoid(pGrid->GetLastItem().itemID) );		
		return;
	}
	else if ( pGrid->GetName().same_no_case( "gridbox_stash" ) )
	{
		GoHandle hStash( GetStash() );
		if ( hStash )
		{
			hStash->GetStash()->RSAddToStash( hStash->GetStash()->GetStashMember(), MakeGoid(pGrid->GetLastItem().itemID) );
		}
		return;
	}

	if ( pGrid->GetName().same_no_case( "tsource_gridbox" ) )
	{
		ProcessAddToDestGridbox();
	}

	if ( hOwner.IsValid() == false ) 
	{
		hOwner = gUIGame.GetActorWhoCarriesObject( MakeGoid(pGrid->GetLastItem().itemID) );
	}	

	GoHandle hItem( MakeGoid( pGrid->GetLastItem().itemID ) );

	if ( !hItem.IsValid() )
	{
		return;
	}

	if ( !hOwner.IsValid() )
	{
		gperrorf( ( "The owner of this item does not exist.  Item: 0x%x, %s", MakeInt(hItem->GetGoid()), hItem->GetTemplateName() ) );		
	}

	if ( hItem->HasGold() )
	{
		pGrid->RemoveID( MakeInt( hItem->GetGoid() ) );
	}

	if ( GetStashActive() && hItem->GetParent() == hOwner )
	{
		return;
	}

	if ( gUIGame.GetBackendMode() == BMODE_DRAG_INVENTORY || gUIGame.GetBackendMode() == BMODE_NONE ) {

		if ( hOwner )
		{
			if ( hOwner->IsScreenPartyMember() )
			{
				hItem->PlayVoiceSound( "put_down", false );
			}

			gUIGame.GetUICommands()->ContextActionOnGameObject( MakeGoid( pGrid->GetLastItem().itemID ), false, SOURCE_INVENTORY );
		}
	}
	else if ( hOwner )
	{		
		gGoDb.DeselectAll();
		gGoDb.Select( hOwner->GetGoid() );
		gUIGame.OrderDraggedRelativeToObject( hOwner->GetGoid() );
		bool noqueueclear = false;
		GoidColl::iterator i;
		for( i = gUIGame.GetDraggedColl().begin(); i != gUIGame.GetDraggedColl().end(); ++i )
		{
			// Loop through dragged objects
			gUIGame.GetUICommands()->ContextActionOnGameObject( *i, noqueueclear, SOURCE_INVENTORY );
			noqueueclear = true;
		}
	}	
}


void UIInventoryManager::GridRequestPickup( UIGridbox * pGrid )
{
	CalculateGridHighlights( pGrid );

	GoHandle hGridItem( gUIInventoryManager.GetGUIRolloverGOID() );
	GoHandle hFloatItem( m_floating_item );

	if ( pGrid->GetStore() )
	{
		if ( gUIStoreManager.GetActiveStoreBuyer() != GOID_INVALID )
		{
			GoHandle hMember( gUIStoreManager.GetActiveStoreBuyer() );
			GoHandle hItem( MakeGoid( pGrid->GetPickupId() ) );
			
			if ( hItem.IsValid() == false )
			{
				return;
			}

			GoHandle hStore( gUIStoreManager.GetActiveStore() );
			if ( hStore->HasStore() )
			{

				if ( gAppModule.GetControlKey() )
				{
					gUIStoreManager.BuyItemFromActiveStore( hItem->GetGoid(), true );		
					pGrid->SetContinuePickup( false );					
					return;
				}
				else
				{
					if ( hStore->GetStore()->GetTransactionPending() )
					{
						pGrid->SetContinuePickup( false );					
					}
					else
					{
						if ( hMember->GetInventory()->GetGold() < hStore->GetStore()->GetPrice( hItem->GetGoid(), hMember->GetGoid() ) ) 
						{
							return;
						}

						pGrid->SetContinuePickup( false );					
						hStore->GetStore()->SetTransactionPending( true );
						hStore->GetStore()->RSRemoveFromStore( hItem->GetGoid(), hMember->GetGoid() );								
						hStore->GetStore()->GetGridbox()->SetItemDetect( true );				
					}				

					gUIGame.SetBackendMode( BMODE_DRAG_INVENTORY );
				}
			}
		}

		return;
	}

	if ( pGrid->GetName().same_no_case( "gridbox_stash" ) )
	{
		GoHandle hItem( MakeGoid( pGrid->GetPickupId() ) );			
		GoHandle hStash( GetStash() );		

		if ( !hItem.IsValid() || !hStash || hStash->GetStash()->GetStashTransfer() )
		{
			return;
		}
		
		hStash->GetStash()->SetStashTransfer( true );
		pGrid->SetContinuePickup( false );					
		hStash->GetStash()->RSRemoveFromStash( hStash->GetStash()->GetStashMember(), hItem->GetGoid() );			
		if ( gAppModule.GetControlKey() )
		{			
			return;
		}
		else
		{					
			pGrid->SetItemDetect( true );				
			gUIGame.SetBackendMode( BMODE_DRAG_INVENTORY );
		}

		return;
	}		

	if ( !hGridItem.IsValid() || !hFloatItem.IsValid() )
	{
		return;
	}

	if ( hGridItem->GetGui()->IsSpellBook() && hFloatItem->IsSpell() )
	{
		if ( hGridItem->GetParent() == hFloatItem->GetParent() && GridLearnSpell( hFloatItem, hGridItem ) )
		{
			UIItem * pItem = GetItemFromGO( hFloatItem, false );
			pItem->ActivateItem( false );
			pGrid->SetContinuePickup( false );
			pGrid->SetIgnoreItem( true );
			pGrid->SetJustPlaced( true );
			return;
		}
	}

	if ( !hGridItem->IsPotion() || !hFloatItem->IsPotion() ||
		 hGridItem->GetMagic()->GetPotionFullRatio() == 1.0f )
	{
		return;
	}

	float fillAmount = 0.0f;
	Enchantment* enchGrid = NULL;
	{
		const EnchantmentVector& enchVec = hGridItem->GetMagic()->GetEnchantmentStorage().GetEnchantments();
		EnchantmentVector::const_iterator i, begin = enchVec.begin(), end = enchVec.end();
		for ( i = begin ; i != end ; ++i )
		{
			enchGrid = (Enchantment*)*i;
			if ( (enchGrid->GetAlterationType() == ALTER_LIFE) || (enchGrid->GetAlterationType() == ALTER_MANA) )
			{
				fillAmount = enchGrid->GetMaxValue() - enchGrid->GetValue();
				break;
			}
		}
	}

	Enchantment* enchFloat = NULL;
	float amountLeft = 0;
	{
		const EnchantmentVector& enchVec = hFloatItem->GetMagic()->GetEnchantmentStorage().GetEnchantments();
		EnchantmentVector::const_iterator i, begin = enchVec.begin(), end = enchVec.end();
		for ( i = begin ; i != end ; ++i )
		{
			enchFloat = (Enchantment*)*i;
			if ( (enchFloat->GetAlterationType() == ALTER_LIFE) || (enchFloat->GetAlterationType() == ALTER_MANA) )
			{
				amountLeft = enchFloat->GetValue();
				break;
			}
		}
	}

	if ( enchFloat->GetAlterationType() != enchGrid->GetAlterationType() )
	{
		return;
	}

	RSCombinePotions( hFloatItem, hGridItem, hGridItem->GetParent(), fillAmount, amountLeft );

	UIItem * pItem = GetItemFromGO( hFloatItem, false );
	pItem->SetSecondaryPercentVisible( hFloatItem->GetMagic()->GetPotionFullRatio() );

	if ( hFloatItem->GetMagic()->IsPotion() && hFloatItem->GetMagic()->HasEnchantments() &&
		//(hSource->GetMagic()->GetPotionFullRatio() == 0.0f || hSource->GetMagic()->GetPotionAmount() < 1.0f) )
		(amountLeft - fillAmount) < 1.0f )
	{
		pItem->ActivateItem( false );
		pItem->SetMarkForDeletion( true );
		gUIShell.SetProcessDeletion( true );
		pGrid->SetItemDetect( true );
		pGrid->SetIgnoreItem( true );
		pGrid->SetJustPlaced( true );

		if ( hFloatItem->HasParent() && hFloatItem->GetParent()->HasInventory() && hFloatItem->GetParent()->GetInventory()->HasGridbox() )
		{
			hFloatItem->GetParent()->GetInventory()->GetGridbox()->SetItemDetect( true );
		}
	}

	pGrid->SetContinuePickup( false );
}


void UIInventoryManager::GridRequestPlacement( UIGridbox * pGrid )
{
	if ( pGrid->GetStore() )
	{
		if ( gUIStoreManager.GetActiveStoreBuyer() != GOID_INVALID )
		{
			GoHandle hMember( gUIStoreManager.GetActiveStoreBuyer() );
			GoHandle hItem( MakeGoid( pGrid->GetPickupId() ) );
			
			if ( hItem.IsValid() == false )
			{
				return;
			}

			GoHandle hStore( gUIStoreManager.GetActiveStore() );
			if ( hStore->HasStore() )
			{
				if ( hStore->GetStore()->GetTransactionPending() || !hItem->GetGui()->GetCanSell() )
				{
					pGrid->SetContinuePickup( false );					
				}
			}
		}	
	}

	if ( pGrid->GetName().same_no_case( "gridbox_stash" ) )
	{
		if ( GetStashActive() )
		{
			GoHandle hStash( GetStash() );
			if ( hStash )
			{
				if ( hStash->GetStash()->GetStashTransfer() )
				{
					pGrid->SetContinuePickup( false );
				}
			}
		}
	}	
}

void UIInventoryManager::GridItemAutoplaced( UIGridbox * pGrid )
{
	CalculateGridHighlights( pGrid );
}


FuBiCookie UIInventoryManager::RSCombinePotions( Go * pSource, Go * pDest, Go * pMember, float fillAmount, float amountLeft )
{
	FUBI_RPC_THIS_CALL_RETRY( RSCombinePotions, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	if ( pSource->GetMagic()->IsPotion() && pSource->GetMagic()->HasEnchantments() &&
	    //(pSource->GetMagic()->GetPotionFullRatio() == 0.0f || pSource->GetMagic()->GetPotionAmount() < 1.0f) )
		(amountLeft - fillAmount) < 1.0f )
	{
		gGoDb.SMarkForDeletion( pSource->GetGoid() );
		RCCombinePotionsDest( pDest->GetGoid(), pMember, fillAmount, amountLeft );
	}
	else
	{
		RCCombinePotions( pSource->GetGoid(), pDest->GetGoid(), pMember, fillAmount, amountLeft );
	}

	return RPC_SUCCESS;
}


FuBiCookie UIInventoryManager::RCCombinePotions( Goid source, Goid dest, Go * pMember, float fillAmount, float amountLeft )
{
	FUBI_RPC_THIS_CALL_RETRY( RCCombinePotions, RPC_TO_ALL );

	GoHandle hSource( source );
	GoHandle hDest( dest );

	Enchantment* enchGrid = NULL;
	if ( hDest.IsValid() )
	{
		const EnchantmentVector& enchVec = hDest->GetMagic()->GetEnchantmentStorage().GetEnchantments();
		EnchantmentVector::const_iterator i, begin = enchVec.begin(), end = enchVec.end();
		for ( i = begin ; i != end ; ++i )
		{
			enchGrid = (Enchantment*)*i;
			if ( (enchGrid->GetAlterationType() == ALTER_LIFE) || (enchGrid->GetAlterationType() == ALTER_MANA) )
			{
				break;
			}
		}
	}

	Enchantment* enchFloat = NULL;
	if ( hSource.IsValid() )
	{
		const EnchantmentVector& enchVec = hSource->GetMagic()->GetEnchantmentStorage().GetEnchantments();
		EnchantmentVector::const_iterator i, begin = enchVec.begin(), end = enchVec.end();
		for ( i = begin ; i != end ; ++i )
		{
			enchFloat = (Enchantment*)*i;
			if ( (enchFloat->GetAlterationType() == ALTER_LIFE) || (enchFloat->GetAlterationType() == ALTER_MANA) )
			{
				break;
			}
		}
	}

	if ( amountLeft <= fillAmount )
	{
		gpstring sValue;

		if ( hDest.IsValid() )
		{
			sValue.assignf( "%f", enchGrid->GetValue() + amountLeft );
			enchGrid->SetValue( sValue );
		}

		if ( hSource.IsValid() )
		{
			sValue.assign( "0.0" );
			enchFloat->SetValue( sValue );
		}
	}
	else if ( amountLeft > fillAmount )
	{
		gpstring sValue;

		if ( hDest.IsValid() )
		{
			sValue.assignf( "%f", enchGrid->GetMaxValue() );
			enchGrid->SetValue( sValue );
		}

		if ( hSource.IsValid() )
		{
			sValue.assignf( "%f", amountLeft - fillAmount );
			enchFloat->SetValue( sValue );
		}
	}

	// Update variable level items ( potions )
	if ( pMember->GetInventory()->HasGridbox() )
	{
		pMember->GetInventory()->GetGridbox()->SetSecondaryPercentVisible( MakeInt(hDest->GetGoid()), hDest->GetMagic()->GetPotionFullRatio() );

		UIItem * pItem = GetItemFromGO( hSource, false );
		if ( pItem )
		{
			pItem->SetSecondaryPercentVisible( hSource->GetMagic()->GetPotionFullRatio() );
		}
	}

	return RPC_SUCCESS;
}


FuBiCookie UIInventoryManager::RCCombinePotionsDest( Goid dest, Go * pMember, float fillAmount, float amountLeft )
{
	FUBI_RPC_THIS_CALL_RETRY( RCCombinePotionsDest, RPC_TO_ALL );
	
	GoHandle hDest( dest );

	Enchantment* enchGrid = NULL;
	if ( hDest.IsValid() )
	{
		const EnchantmentVector& enchVec = hDest->GetMagic()->GetEnchantmentStorage().GetEnchantments();
		EnchantmentVector::const_iterator i, begin = enchVec.begin(), end = enchVec.end();
		for ( i = begin ; i != end ; ++i )
		{
			enchGrid = (Enchantment*)*i;
			if ( (enchGrid->GetAlterationType() == ALTER_LIFE) || (enchGrid->GetAlterationType() == ALTER_MANA) )
			{
				break;
			}
		}
	}

	if ( amountLeft <= fillAmount )
	{
		gpstring sValue;

		if ( hDest.IsValid() )
		{
			sValue.assignf( "%f", enchGrid->GetValue() + amountLeft );
			enchGrid->SetValue( sValue );
		}	
	}
	else if ( amountLeft > fillAmount )
	{
		gpstring sValue;

		if ( hDest.IsValid() )
		{
			sValue.assignf( "%f", enchGrid->GetMaxValue() );
			enchGrid->SetValue( sValue );
		}
	
	}

	// Update variable level items ( potions )
	if ( pMember->GetInventory()->HasGridbox() )
	{
		pMember->GetInventory()->GetGridbox()->SetSecondaryPercentVisible( MakeInt(hDest->GetGoid()), hDest->GetMagic()->GetPotionFullRatio() );
	}

	return RPC_SUCCESS;
}


void UIInventoryManager::ItemCreationCb( int item )
{
	GoHandle hItem( MakeGoid( item ) );
	if ( hItem.IsValid() )
	{
		UIItem * pItem = GetItemFromGO( hItem, false, hItem->HasActor() ? true : false );
		if ( pItem )
		{
			gUIShell.AddToItemMap( pItem );
		}
	}
}


void UIInventoryManager::GridItemPickup( UIGridbox * pGrid )
{
	gUIGame.SetBackendMode( BMODE_DRAG_INVENTORY );

	if ( pGrid->GetName().same_no_case( "tsource_gridbox" ) )
	{
		RSRemoveFromDestGridbox( pGrid->GetPickupId(), GetDestTrade()->GetPlayer() );
	}

	GoHandle hGridItem( MakeGoid( pGrid->GetPickupId() ) );
	Go * pParent = 0;
	if ( hGridItem.IsValid() )
	{
		pParent = hGridItem->GetParent();
	}

	if ( pParent )
	{
		int charIndex = 0;
		if ( gUIPartyManager.GetCharacterFromGoid( pParent->GetGoid(), charIndex ) )
		{
			gUIPartyManager.SetSelectedCharacterIndex( charIndex );
		}
	}
	
	if ( !pGrid->GetStore() )
	{
		if ( gAppModule.GetControlKey() && !(gUIShell.GetItemActive() && pGrid->GetJustPlaced()) )
		{
			if ( !gUIPartyManager.GetInventoriesVisible() )
			{
				if ( !gUIShell.IsInterfaceVisible( "store" ) )
				{					
					gUIGame.GetUICommands()->CommandDrop( hGridItem->GetGoid(), pParent->GetPlacement()->GetPosition(), false, true, pParent );
					UIItem * pItem = GetItemFromGO( hGridItem, false );
					if ( pItem )
					{
						pItem->ActivateItem( false );
					}
					pGrid->SetItemDetect( true );
				}
				else if ( hGridItem.IsValid() && hGridItem->GetGui()->GetCanSell() )
				{					
					gUIStoreManager.SellItemToActiveStore( hGridItem->GetGoid() );
					UIItem * pItem = GetItemFromGO( hGridItem, false );
					if ( pItem )
					{
						pItem->ActivateItem( false );
					}
					pGrid->SetItemDetect( true );
				}
			}
			else
			{
				GoidColl openInv;
				GoidColl::iterator iMember;
				gUIPartyManager.GetOpenInventories( openInv );
				for ( iMember = openInv.begin(); iMember != openInv.end(); ++iMember )
				{
					GoHandle hOpenInv( (*iMember) );
					if ( hOpenInv.IsValid() )
					{
						if ( hOpenInv->GetGoid() == hGridItem->GetParent()->GetGoid() )
						{
							GoHandle hTrader;
							if ( iMember == (openInv.end()-1) )
							{
								hTrader = *(openInv.begin());
							}
							else
							{
								hTrader = *(iMember+1);
							}

							if ( hTrader.IsValid() )
							{
								gUIGame.GetUICommands()->CommandGive( hGridItem->GetGoid(), hTrader->GetGoid() );
								UIItem * pItem = GetItemFromGO( hGridItem, false );
								if ( pItem )
								{
									pItem->ActivateItem( false );
									pGrid->SetItemDetect( true );
								}
							}

							break;
						}
					}
				}
			}
		}
	}
	
	if ( hGridItem.IsValid() && hGridItem->GetGui()->IsSpellBook() )
	{
		if ( pParent && pParent->GetInventory()->GetActiveSpellBook() == hGridItem )
		{
			if ( gUIShell.IsInterfaceVisible( "spell" ) )
			{
				gUIPartyManager.SetSpellsActive( false );
			}
		}
	}

	if ( hGridItem.IsValid() )
	{
		hGridItem->PlayVoiceSound( "pick_up", false );
	}

	CalculateGridHighlights( pGrid );	
}



void UIInventoryManager::GridMultipleItemDrag( UIGridbox * pGrid )
{
	gUI.GetGameGUI().GetMessenger()->SendUIMessage( UIMessage( MSG_DEACTIVATEITEMS ) );
	pGrid->SetMultipleItems( false );
	gGoDb.DeselectAll();
	gGoDb.Select( gUIPartyManager.GetSelectedCharacter() );
	gUIGame.OrderDraggedRelativeToObject( gUIPartyManager.GetSelectedCharacter() );
	bool noqueueclear = false;
	GoidColl::iterator i;
	for( i = gUIGame.GetDraggedColl().begin(); i != gUIGame.GetDraggedColl().end(); ++i )
	{
		// Loop through dragged objects
		GoHandle hObject( *i );
		if ( hObject.IsValid() )
		{
			if ( hObject->HasGui() )
			{
				gUIGame.GetUICommands()->CommandGet( *i, noqueueclear, SOURCE_ACTOR );
			}
			noqueueclear = true;
		}
	}
}


void UIInventoryManager::GridItemRollover( UIGridbox * pGrid )
{
	SetGUIRolloverGOID( MakeGoid( pGrid->GetRolloverID() ) );
	DisplayRolloverItemData( GetGUIRolloverGOID(), pGrid->GetTextBox() );

	if ( gUIShell.GetItemActive() )
	{
		GoHandle hGridItem( GetGUIRolloverGOID() );
		if ( hGridItem.IsValid() && hGridItem->GetGui()->IsSpellBook() )
		{
			UIItemVec items;
			gUIShell.FindActiveItems( items );
			if ( items.size() == 1 )
			{
				UIItem * pItem = *(items.begin());
				GoHandle hFloatItem( MakeGoid( pItem->GetItemID() ) );
				if ( hFloatItem.IsValid() && hFloatItem->IsSpell() && hFloatItem->HasParent() &&
					 hFloatItem->GetParent() == hGridItem->GetParent() )
				{
					pGrid->ActivateRolloverHighlight( pGrid->GetRolloverID(), 0x6600ff00 );
				}
			}
		}
	}
}


void UIInventoryManager::GridUseItem( UIGridbox * pGrid )
{
	if ( !gUIPartyManager.GetEquipmentVisible() || !gUIShell.GetGridPlace() )
	{
		return;
	}

	GoHandle hObject( GetGUIRolloverGOID() );
	if (( hObject.IsValid() == false ) || ( hObject->HasGui() == false )) 
	{
		return;
	}

	GopColl partyColl;
	Go * pParty = gServer.GetScreenParty();
	if ( pParty )
	{
		partyColl = pParty->GetChildren();
	}

	GoHandle hStore( gUIStoreManager.GetActiveStore() );
	if ( hStore.IsValid() && pGrid == hStore->GetStore()->GetGridbox() )
	{
		return;
	}

	GopColl::iterator i;
	for ( i = partyColl.begin(); i != partyColl.end(); ++i ) {
		GoHandle hMember( (*i)->GetGoid() );
		GoHandle hObject( GetGUIRolloverGOID() );

		if ( hMember->GetInventory()->GetGridbox() == pGrid && !hMember->GetInventory()->IsPackOnly() && hMember->GetInventory()->GetGridbox()->GetLocalPlace() ) 
		{
			if ( !gUIShell.GetItemActive() )
			{
				if ( hObject->IsPotion() )
				{
					gUIGame.GetUICommands()->CommandUse( hObject->GetGoid(), hMember->GetGoid() );
				}
				else if ( hObject->IsSpell() && !gUIShell.GetItemActive() )
				{
					Go * pSpellBook = hMember->GetInventory()->GetActiveSpellBook();
					if ( !pSpellBook )
					{
						pSpellBook = hMember->GetInventory()->GetEquipped( ES_SPELLBOOK );
					}
					if ( pSpellBook )
					{
						if ( GridLearnSpell( hObject, pSpellBook ) )
						{
							if ( gUIShell.IsInterfaceVisible( "character" ) && !gUIShell.IsInterfaceVisible( "spell" ) )
							{
								hMember->GetInventory()->SetActiveSpellBook( pSpellBook );
								gUIPartyManager.SetSpellsActive( true );
							}

							pGrid->RemoveID( MakeInt( hObject->GetGoid() ) );
						}
					}
					else
					{
						gpscreen( $MSG$ "You do not have a Spell Book equipped." );
					}
				}
				else if ( hObject->GetGui()->IsSpellBook() )
				{
					bool bClose = false;
					if ( hMember->GetInventory()->GetActiveSpellBook() == hObject )
					{
						bClose = true;
					}

					if ( gUIShell.IsInterfaceVisible( "character" ) )
					{
						hMember->GetInventory()->SetActiveSpellBook( hObject );

						if ( bClose && gUIShell.IsInterfaceVisible( "spell" ) )
						{
							gUIPartyManager.SetSpellsActive( false );
						}
						else
						{
							gUIPartyManager.SetSpellsActive( true );
						}
					}
					else
					{
						int index = 0;
						gUIPartyManager.GetCharacterFromGoid( hMember->GetGoid(), index );
						gUIPartyManager.ConstructCharacterInterface( index );
						hMember->GetInventory()->SetActiveSpellBook( hObject );

						if ( bClose && gUIShell.IsInterfaceVisible( "spell" ) )
						{
							gUIPartyManager.SetSpellsActive( false );
						}
						else
						{
							gUIPartyManager.SetSpellsActive( true );
						}
					}

					hObject->PlayVoiceSound( "gui_spellbook_open", false );
				}
				else if ( !gUIShell.GetItemActive() && IsEquippableSlot( hObject->GetGui()->GetEquipSlot() ) )
				{
					gUIGame.SetBackendMode( BMODE_DRAG_INVENTORY );
					bool bCanEquip = hMember->GetInventory()->TestEquipMinReqs( hObject );
					if ( bCanEquip && !gUIPartyManager.IsCharacterInterfaceBusy( hMember, true ) )
					{
						int charIndex = 0;
						if ( gUIPartyManager.GetCharacterFromGoid( hMember->GetGoid(), charIndex ) )
						{
							gUIPartyManager.SetSelectedCharacterIndex( charIndex );
						}
						ContextPlaceItem( true, hMember->GetGoid() );
						ListenerCharacterRolloff();						
					}
					else if ( !bCanEquip && !gUIPartyManager.IsCharacterInterfaceBusy( hMember, true ) )
					{
						hMember->PlayVoiceSound( "gui_inventory_not_equippable", false );
						gpscreen( $MSG$ "Cannot equip item" );
					}
				}
				else if ( hObject->GetGui()->IsLoreBook() )
				{
					gUIPartyManager.LoreBookDialog( hObject );
				}
			}
		}
	}

	CalculateGridHighlights( pGrid );
}


bool UIInventoryManager::GridLearnSpell( Go * pSpell, Go * pSpellBook )
{
	if ( m_bSpellBookBusy )
	{
		return false;
	}	

	GopColl items;
	pSpellBook->GetInventory()->ListItems( IL_ALL_SPELLS, items );
	if ( items.size() >= GoInventory::MAX_SPELLS )
	{
		gpscreen( $MSG$ "This Spell Book is full.\n" );
		return false;
	}
	else if ( pSpellBook->GetInventory()->LocationContainsTemplate( IL_ALL_SPELLS, pSpell->GetTemplateName() ) &&
		      !pSpell->GetMagic()->GetIsOneUse() )
	{
		eInventoryLocation il = pSpellBook->GetInventory()->GetLocation( pSpell );
		if ( il == IL_INVALID )
		{
			gpscreen( $MSG$ "This spell is already inscribed in this Spell Book.\n" );
			return false;
		}
		else
		{
			int i = (int)il - (int)IL_ACTIVE_COUNT;
			gpstring sTemp;
			sTemp.assignf( "spell_slot_%d", i + 1 );
			UIInfoSlot * pTestSlot = (UIInfoSlot *)gUIShell.FindUIWindow( sTemp, "spell" );
			if ( pTestSlot->GetItemID() == MakeInt( pSpell->GetGoid() ) )
			{
				gpscreen( $MSG$ "This spell is already inscribed in this Spell Book.\n" );
				return false;
			}
		}
	}

	int start	= (int)IL_SPELL_1;
	int end		= (int)IL_SPELL_12 + 1;

	for( int is = start; is != end; ++is )
	{
		if ( !pSpellBook->GetInventory()->GetItem( (eInventoryLocation)is ) )
		{
			m_bSpellBookBusy = true;
			pSpellBook->GetParent()->GetInventory()->RSTransfer( pSpell, pSpellBook, (eInventoryLocation)is, AO_USER );
			m_bLearnedSpell = true;
			SetGUIRolloverGOID( GOID_INVALID );

			if ( pSpellBook->GetParent() )
			{
				pSpellBook->GetParent()->GetInventory()->SetActiveSpellBook( pSpellBook );

				int index = 0;
				gUIPartyManager.GetCharacterFromGoid( pSpellBook->GetParent()->GetGoid(), index );
				if ( gUIShell.IsInterfaceVisible( "character" ) )
				{
					gUIPartyManager.SetSpellsActive( true );
				}

				if ( pSpell->GetMagic()->CanOwnerCast() && pSpellBook == pSpellBook->GetParent()->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
				{
					if ( (eInventoryLocation)is == IL_SPELL_1 )
					{
						ActivatePrimarySpell( index );
					}
					else if ( (eInventoryLocation)is == IL_SPELL_2 )
					{
						ActivateSecondarySpell( index );
					}
				}
			}

			int slotIndex = is - (int)IL_ACTIVE_COUNT;
			gpstring sTemp;
			sTemp.assignf( "spell_slot_%d", slotIndex + 1 );
			UIInfoSlot * pSlot = (UIInfoSlot *)gUIShell.FindUIWindow( sTemp, "spell" );
			if ( pSlot )
			{
				pSlot->SetJustPlaced( false );
				pSlot->PlaceItem( GetItemFromGO( pSpell, false ), false, false, false, false );

				gpstring sGroup;
				sGroup.assignf( "spell_slot_%d", pSlot->GetIndex()+1 );
				gUI.GetGameGUI().ShowGroup( sGroup, true );		
				
				sTemp.assignf( "text_box_spell_%d", pSlot->GetIndex() + 1 );
				UITextBox * pText = (UITextBox *)gUI.GetGameGUI().FindUIWindow( sTemp );
				if ( pText )
				{
					gpwstring sName;
					if ( pSpell->GetMagic()->GetMagicClass() == MC_NATURE_MAGIC )
					{
						sName.appendf( L"<c:0x%x>%s</c>", gUIShell.GetTipColor( "nature_magic" ), pSpell->GetCommon()->GetScreenName().c_str() );
					}
					else if ( pSpell->GetMagic()->GetMagicClass() == MC_COMBAT_MAGIC )
					{
						sName.appendf( L"<c:0x%x>%s</c>", gUIShell.GetTipColor( "combat_magic" ), pSpell->GetCommon()->GetScreenName().c_str() );
					}			
					
					pText->SetText( sName );
				}

				sTemp.assignf( "spell_item_%d", pSlot->GetIndex() + 1 );
				bool bSuccess = false;
				gpstring sTexture = GetTextureStringFromIcon( pSpell, pSpell->GetGui()->GetActiveIcon(), bSuccess );		
				if ( !sTexture.empty() )
				{
					UIItemSlot * pIcon = (UIItemSlot *)gUI.GetGameGUI().FindUIWindow( sTemp );
					if ( pIcon )
					{
						pIcon->SetHasTexture( true );
						pIcon->LoadTexture( sTexture );
					}
				}
			}

			gUIGame.SetBackendMode( BMODE_DRAG_INVENTORY );
			return true;
		}
	}

	return false;
}


void UIInventoryManager::CalculateGridHighlights( UIGridbox * pGrid )
{
	GopColl partyColl;
	Go * pParty = gServer.GetScreenParty();
	if ( pParty )
	{
		partyColl = pParty->GetChildren();
	}

	bool bValid = false;
	GoHandle hMember;

	// Check for standard gridbox
	GopColl::iterator i;
	for ( i = partyColl.begin(); i != partyColl.end(); ++i )
	{
		// ignore items
		if ( !(*i)->HasActor() )
		{
			continue;
		}

		hMember = (*i)->GetGoid();
		if ( hMember->GetInventory()->GetGridbox() == pGrid )
		{
			bValid = true;
			break;
		}
	}

	// Check for store gridbox
	if ( !bValid )
	{		
		GoHandle hStore( gUIStoreManager.GetActiveStore() );
		hMember = gUIStoreManager.GetActiveStoreBuyer();
		if ( hStore.IsValid() && hMember.IsValid() && hStore->HasInventory() && hStore->GetStore()->GetGridbox() == pGrid )
		{
			bValid = true;
		}
		else if ( GetStashActive() )
		{
			GoHandle hStash( GetStash() );
			if ( hStash && hStash->GetInventory()->GetGridbox() == pGrid )
			{
				hMember = hStash->GetStash()->GetStashMember();
				bValid = true;
			}
		}
	}

	// Check for trade gridbox
	if ( !bValid )
	{
		if ( GetDestTrade() && GetSourceTrade() )
		{
			hMember = m_SourceTrade;
			bValid = true;
		}
	}

	if ( bValid && hMember.IsValid() )
	{
		pGrid->ResetHighlights();

		GridItemColl gridItems = pGrid->GetGridItems();
		GridItemColl::iterator i;
		for ( i = gridItems.begin(); i != gridItems.end(); ++i )
		{
			GoHandle hObject( MakeGoid( (*i).itemID ) );
			if ( hObject.IsValid() )
			{
				if ( hObject->HasGui() && hObject->GetGui()->GetEquipSlot() != ES_NONE )
				{
					bool bCanEquip = hMember->GetInventory()->IsPackOnly() ? true : hMember->GetInventory()->TestEquipMinReqs( hObject );
					if ( !bCanEquip )
					{
						DWORD dwSecondaryColor = 0;
						if ( hObject->HasMagic() && hObject->GetMagic()->HasNonInnateEnchantments() )
						{
							// if the item cannot be equipped AND it is magical, let's color it half and half.
							dwSecondaryColor = GetGridMagicColor();
						}

						pGrid->ActivateHighlight( (*i).itemID, GetGridInvalidColor(), dwSecondaryColor );

					}
					else if ( hObject->HasMagic() && hObject->GetMagic()->HasNonInnateEnchantments() )
					{
						pGrid->ActivateHighlight( (*i).itemID, GetGridMagicColor() );
					}
				}
				if ( hObject->HasGui() && hObject->GetGui()->IsSpellBook() )
				{
					if ( hMember->GetInventory()->GetActiveSpellBook() == hObject )
					{
						pGrid->ActivateHighlight( (*i).itemID, GetActiveBookColor() );
					}
				}
				if ( hObject->IsSpell() )
				{				
					float skillLevel = hMember->GetActor()->GetSkillLevel( hObject->GetMagic()->GetSkillClass() );					
					if ( hObject->GetMagic()->GetRequiredCastLevel() > skillLevel )
					{
						pGrid->ActivateHighlight( (*i).itemID, GetGridInvalidColor() );						
					}	
				}				
			}
		}
		
			
		UIItemSlot * pSpellSlot = (UIItemSlot *)gUIShell.FindUIWindow( "spellbook", "character" );
		if ( pSpellSlot && hMember->GetInventory()->GetGridbox()->GetVisible() )
		{
			GoHandle hBook( MakeGoid( pSpellSlot->GetItemID() ) );
			if ( hBook.IsValid() )
			{
				if ( hBook->HasGui() && hBook->GetGui()->IsSpellBook() )
				{
					if ( hMember->GetInventory()->GetActiveSpellBook() == hBook || 
						 hMember->GetInventory()->GetActiveSpellBook() == NULL )
					{						
						pSpellSlot->SetSpecialActive( true );
						pSpellSlot->SetSpecialColor( GetActiveBookColor() );
					}
					else
					{
						if ( hBook->HasMagic() && hBook->GetMagic()->HasNonInnateEnchantments() )
						{
							pSpellSlot->SetSpecialActive( true );
							pSpellSlot->SetSpecialColor( GetGridMagicColor() );
						}
						else
						{
							pSpellSlot->SetSpecialActive( false );
						}
					}
				}
			}
			else
			{
				pSpellSlot->SetSpecialActive( false );
			}
		}
	}
}


void UIInventoryManager::CalculateAllGridHighlights()
{
	UIWindowVec grids = gUIShell.ListWindowsOfType( UI_TYPE_GRIDBOX );
	UIWindowVec::iterator i;
	for ( i = grids.begin(); i != grids.end(); ++i )
	{
		CalculateGridHighlights( (UIGridbox *)(*i) );
	}
}


void UIInventoryManager::ItemslotUseItem( UIItemSlot * pSlot )
{
	GoHandle hMember( gUIPartyManager.GetSelectedCharacter() );
	GoHandle hItem( MakeGoid( pSlot->GetItemID() ) );
	if ( hMember.IsValid() && hItem.IsValid() )
	{
		if ( hItem->GetGui()->IsSpellBook() )
		{
			bool bClose = false;
			if ( hMember->GetInventory()->GetActiveSpellBook() == hItem )
			{
				bClose = true;
			}

			hMember->GetInventory()->SetActiveSpellBook( hItem );

			if ( bClose && gUIShell.IsInterfaceVisible( "spell" ) )
			{
				gUIPartyManager.SetSpellsActive( false );
			}
			else
			{
				gUIPartyManager.SetSpellsActive( true );
			}

			hItem->PlayVoiceSound( "gui_spellbook_open", false );
		}
	}
}

void UIInventoryManager::ItemslotEquip( UIItemSlot * pSlot )
{	
	GoHandle hObject( MakeGoid( pSlot->GetAttemptEquipId() ) );
	GoHandle hMember( gUIPartyManager.GetSelectedCharacter() );
	if ( hObject.IsValid() && hMember.IsValid() )
	{
		if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) )
		{
			pSlot->SetCanEquip( false );
			return;
		}
		bool bCanEquip = hMember->GetInventory()->TestEquipMinReqs( hObject );
		pSlot->SetCanEquip( bCanEquip );
		if ( !bCanEquip )
		{
			hMember->PlayVoiceSound( "gui_inventory_not_equippable", false );			
		}

		GoHandle hOwner( gUIGame.GetActorWhoCarriesObject( hObject->GetGoid() ) );
		if ( !hOwner.IsValid() )
		{
			pSlot->SetCanEquip( false );
		}
	}
}


void UIInventoryManager::ItemslotRequestRemove( UIItemSlot * pSlot )
{
	GoHandle hMember( gUIPartyManager.GetSelectedCharacter() );
	if ( hMember.IsValid() )
	{
		if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) )
		{
			pSlot->SetCanRemove( false );
		}
		else
		{
			pSlot->SetCanRemove( true );
		}
	}
}


void UIInventoryManager::GridItemRolloff( UIGridbox * pGrid )
{
	UNREFERENCED_PARAMETER( pGrid );
	SetGUIRolloverGOID( GOID_INVALID );
}


void UIInventoryManager::SlotRollover( UIWindow * pWindow )
{
	int id = 0;
	if ( pWindow->GetType() == UI_TYPE_ITEMSLOT )
	{
		id = ((UIItemSlot *)pWindow)->GetItemID();
		SetGUIRolloverGOID( MakeGoid( id ) );
		DisplayRolloverItemData( GetGUIRolloverGOID(), ((UIItemSlot *)pWindow)->GetTextBox() );
	}
	else if ( pWindow->GetType() == UI_TYPE_INFOSLOT )
	{
		id = ((UIInfoSlot *)pWindow)->GetItemID();
		SetGUIRolloverGOID( MakeGoid( id ) );
		DisplayRolloverItemData( GetGUIRolloverGOID(), ((UIInfoSlot *)pWindow)->GetTextBox() );
	}
}


void UIInventoryManager::SlotRolloff( UIWindow * pWindow )
{
	UNREFERENCED_PARAMETER( pWindow );
	if ( gUIShell.GetRolloverItemslotID() == 0 )
	{
		SetGUIRolloverGOID( GOID_INVALID );
	}
	else
	{
		SetGUIRolloverGOID( MakeGoid( gUIShell.GetRolloverItemslotID() ) );
	}

}

void UIInventoryManager::ItemslotRemoveItem( UIWindow * pSlot )
{
	UIItemVec items;
	gUIShell.FindActiveItems( items );
	if ( items.size() == 1 )
	{
		UIItem * pItem = *(items.begin());

		UIWindowVec windows = gUIShell.ListWindowsOfGroup( "equipment" );
		UIWindowVec::iterator i;
		for ( i = windows.begin(); i != windows.end(); ++i )
		{
			if ( (*i)->GetType() == UI_TYPE_ITEMSLOT )
			{
				if ( ((UIItemSlot *)(*i))->GetSlotType().same_no_case( pItem->GetSlotType() ) )
				{
					((UIItemSlot *)(*i))->SetActivateColor( GetGridRolloverColor() );
					((UIItemSlot *)(*i))->ActivateSlotHighlight( true );
				}
			}
		}
	}

	eEquipSlot eslot = ES_NONE;
	GoHandle hItem( m_floating_item );
	eslot = hItem->GetGui()->GetEquipSlot();
	if ( eslot == ES_RING )
	{
		if ( ((UIItemSlot *)pSlot)->GetName().same_no_case( "ring1" ) )
		{
			eslot = ES_RING_0;
		}
		else if ( ((UIItemSlot *)pSlot)->GetName().same_no_case( "ring2" ) )
		{
			eslot = ES_RING_1;
		}
		else if ( ((UIItemSlot *)pSlot)->GetName().same_no_case( "ring3" ) )
		{
			eslot = ES_RING_2;
		}
		else if ( ((UIItemSlot *)pSlot)->GetName().same_no_case( "ring4" ) )
		{
			eslot = ES_RING_3;
		}
	}

	UIItemSlot * pItemSlot = (UIItemSlot *)pSlot;
	if ( pItemSlot )
	{
		pItemSlot->SetSpecialActive( false );
	}

	// $ Is this necessary anymore?  Will check later. - CQ
	gUIGame.SetBackendMode( BMODE_DRAG_INVENTORY );

	if ( eslot == ES_NONE )
	{
		return;
	}

	GoHandle hMember( gUIGame.GetActorWhoCarriesObject( m_floating_item ) );

	gpassertf( hMember.IsValid(), ("Invalid actor handle created while determining who carries object") );

	if( hMember.IsValid() )
	{
		gUIGame.SetDragged( m_floating_item );
		GoHandle hObject( m_floating_item );

		if( !(( hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_MELEE_WEAPON ) &&
			( pSlot->GetName().same_no_case( "ranged_weapon" ) )) )
		{
			if ( !(( eslot == ES_SHIELD_HAND ) && ( !hObject->HasAttack() ) && ( hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_RANGED_WEAPON  && hMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON ) )) )
			{
				bool bActivate = false;
				eInventoryLocation location = IL_ACTIVE_MELEE_WEAPON;
				if ( eslot == ES_SPELLBOOK && (hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL ||
								  hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL) )
				{
					int index = 0;
					gUIPartyManager.GetCharacterFromGoid( hMember->GetGoid(), index );					
					location = ActivateNextValid( index );
					bActivate = true;
				}

				Go * object = hMember->GetInventory()->GetEquipped( eslot );
				gUIGame.GetUICommands()->CommandUnequip( eslot, hMember->GetGoid() );

				// If this is a two-handed melee weapon being unequipped, be sure to activate the shield, if available
				Go * pShield = hMember->GetInventory()->GetItem( IL_SHIELD );
				if ( eslot == ES_WEAPON_HAND && object && object->HasAttack() && object->GetAttack()->GetIsTwoHanded() && pShield )
				{
					hMember->GetInventory()->RSEquip( ES_SHIELD_HAND, pShield->GetGoid(), AO_USER );
				}
				else if ( eslot == ES_WEAPON_HAND && object && !object->HasAttack() )
				{
					gperrorf(( "Trying to unequip a %s, which doesn't have an attack component!\n", object->GetTemplateName() ));
				}

				if ( bActivate )
				{
					int index = 0;
					gUIPartyManager.GetCharacterFromGoid( hMember->GetGoid(), index );	
					switch ( location )
					{
						case IL_ACTIVE_MELEE_WEAPON:
							ActivateMeleeWeapon( index );
							break;
						case IL_ACTIVE_RANGED_WEAPON:
							ActivateRangedWeapon( index );
							break;
						case IL_ACTIVE_PRIMARY_SPELL:
							ActivatePrimarySpell( index );
							break;
						case IL_ACTIVE_SECONDARY_SPELL:
							ActivateSecondarySpell( index );
							break;
					}
				}
			}
		}

		GoHandle hItem( m_floating_item );

		if( hMember->GetInventory()->Contains( hItem ) )
		{
			hMember->GetInventory()->RSSetLocation( hItem, IL_MAIN, false );
		}

		if( ( eslot == ES_SHIELD_HAND ) && ( !hItem->HasAttack() ) )
		{
			Go * shield = hMember->GetInventory()->GetShield();
			if( shield )
			{
				hMember->GetInventory()->RSSetLocation( shield, IL_MAIN, false );
			}
		}

		if ( eslot == ES_WEAPON_HAND )
		{
			GoHandle hWeapon( m_floating_item );

			if ( hWeapon.IsValid() && hWeapon->HasAttack() &&
				( hMember->GetInventory()->GetShield() ) &&
				( hMember->GetInventory()->GetSelectedLocation() != IL_ACTIVE_RANGED_WEAPON ) )
			{
				gUIGame.GetUICommands()->CommandEquip( hMember->GetInventory()->GetShield()->GetGoid(), ES_SHIELD_HAND, false, false );
			}
		}

		hMember->GetInventory()->SetInventoryDirty( true );

		if ( hMember->IsScreenPartyMember() )
		{
			hItem->PlayVoiceSound( "pick_up", false );
		}
	}

	if ( hItem->GetGui()->IsSpellBook() )
	{
		if ( hItem->GetParent()->GetInventory()->GetActiveSpellBook() == hItem )
		{			
			if ( gUIShell.IsInterfaceVisible( "spell" ) )
			{
				gUIPartyManager.SetSpellsActive( false );			
			}
		}
	}

	CalculateAllGridHighlights();
}


void UIInventoryManager::ActivateMeleeWeapon( int charIndex )
{
	if ( gUIPartyManager.GetSelectedCharacterFromIndex( charIndex ) == NULL )
	{
		return;
	}

	GoHandle hMember( gUIPartyManager.GetSelectedCharacterFromIndex( charIndex )->GetGoid() );
	if ( !hMember.IsValid() )
	{
		return;
	}

	if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember, true ) ||
		 hMember->GetAspect()->GetLifeState() != LS_ALIVE_CONSCIOUS )
	{
		UpdateInventoryCallback( hMember->GetGoid() );
		return;
	}

	if ( hMember.IsValid() && hMember->HasMind() )
	{
		hMember->GetMind()->RSDoJob( MakeJobReq( JAT_STOP, JQ_ACTION, QP_CLEAR, AO_USER ) );
	}
	else
	{
		return;
	}
	
	hMember->GetInventory()->RSSelect( IL_ACTIVE_MELEE_WEAPON, AO_USER );

	gUIPartyManager.GetSelectedCharacterFromIndex( charIndex )->SetActiveSlot( SELECTION_MELEE_WEAPON );

	if ( ( hMember->GetInventory()->GetEquipped( ES_WEAPON_HAND  ) != hMember->GetInventory()->GetItem( IL_ACTIVE_MELEE_WEAPON ) ) ||
			( hMember->GetInventory()->GetEquipped( ES_WEAPON_HAND  ) == NULL ) )
	{
		Go * es_shield_hand = hMember->GetInventory()->GetEquipped( ES_SHIELD_HAND );

		if ( es_shield_hand && es_shield_hand->HasAttack() )
		{
			gUIGame.GetUICommands()->CommandUnequip( ES_SHIELD_HAND, hMember->GetGoid() );
		}

		Go * ranged = hMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON );
		if ( ranged && ranged->GetGui()->GetEquipSlot() == ES_WEAPON_HAND )
		{
			gUIGame.GetUICommands()->CommandUnequip( ES_WEAPON_HAND, hMember->GetGoid() );
		}

		Go * weapon = hMember->GetInventory()->GetItem( IL_ACTIVE_MELEE_WEAPON );

		if( weapon )
		{
			gUIGame.GetUICommands()->CommandEquip( weapon->GetGoid(), ES_WEAPON_HAND, hMember->GetGoid() );
		}

		if ( weapon && weapon->GetAttack()->GetIsTwoHanded() )
		{
			return;
		}

		if ( hMember->GetInventory()->GetItem( IL_SHIELD ) != NULL )
		{
			gUIGame.GetUICommands()->CommandEquip( hMember->GetInventory()->GetItem( IL_SHIELD )->GetGoid(), ES_SHIELD_HAND, hMember->GetGoid(), true );
		}
	}
}


void UIInventoryManager::ActivateRangedWeapon( int charIndex )
{
	if ( gUIPartyManager.GetSelectedCharacterFromIndex( charIndex ) == NULL )
	{
		return;
	}

	GoHandle hMember( gUIPartyManager.GetSelectedCharacterFromIndex( charIndex )->GetGoid() );
	if ( !hMember.IsValid() )
	{
		return;
	}
	else
	{
		if ( hMember.IsValid() && !hMember->HasAspect() )
		{
			gperrorf( ( "%s: Has no aspect?  Tell Chad.", hMember->GetTemplateName() ) );
		}
	}

	if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember, true ) ||
		 hMember->GetAspect()->GetLifeState() != LS_ALIVE_CONSCIOUS )
	{
		UpdateInventoryCallback( hMember->GetGoid() );
		return;
	}

	if ( hMember.IsValid() && hMember->HasMind() )
	{
		hMember->GetMind()->RSDoJob( MakeJobReq( JAT_STOP, JQ_ACTION, QP_CLEAR, AO_USER ) );
	}
	else
	{
		return;
	}
	
	hMember->GetInventory()->RSSelect( IL_ACTIVE_RANGED_WEAPON, AO_USER );

	gUIPartyManager.GetSelectedCharacterFromIndex( charIndex )->SetActiveSlot( SELECTION_RANGED_WEAPON );

	if ( hMember->GetInventory()->GetEquipped( ES_SHIELD_HAND ) != hMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON ) )
	{
		Go * es_weapon_hand = hMember->GetInventory()->GetEquipped( ES_WEAPON_HAND );
		Go * es_shield_hand = hMember->GetInventory()->GetEquipped( ES_SHIELD_HAND );

		if ( es_weapon_hand && es_weapon_hand->HasAttack() )
		{
			gUIGame.GetUICommands()->CommandUnequip( ES_WEAPON_HAND, hMember->GetGoid() );
		}

		if ( es_shield_hand )
		{
			gUIGame.GetUICommands()->CommandUnequip( ES_SHIELD_HAND, hMember->GetGoid() );
		}

		if ( hMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON ) )
		{
			gUIGame.GetUICommands()->CommandEquip( hMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON )->GetGoid(), ES_SHIELD_HAND, hMember->GetGoid() );
		}
	}
}


void UIInventoryManager::ActivatePrimarySpell( int charIndex )
{
	if ( gUIPartyManager.GetSelectedCharacterFromIndex( charIndex ) == NULL )
	{
		return;
	}

	GoHandle hMember( gUIPartyManager.GetSelectedCharacterFromIndex( charIndex )->GetGoid() );

	if ( !hMember.IsValid() )
	{
		return;
	}

	if ( hMember->GetAspect()->GetLifeState() != LS_ALIVE_CONSCIOUS )
	{
		return;
	}

	if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember, true ) )
	{
		UpdateInventoryCallback( hMember->GetGoid() );
		return;
	}

	if ( hMember->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL ) &&
		 hMember->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL )->HasMagic() &&
		 hMember->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL )->GetMagic()->CanOwnerCast() )
	{
		hMember->GetMind()->RSDoJob( MakeJobReq( JAT_STOP, JQ_ACTION, QP_CLEAR, AO_USER ) );		
		hMember->GetInventory()->RSSelect( IL_ACTIVE_PRIMARY_SPELL, AO_USER );
		hMember->PlayVoiceSound( "active_spell_select", false );

		gUIPartyManager.GetSelectedCharacterFromIndex( charIndex )->SetActiveSlot( SELECTION_PRIMARY_SPELL );
	}
}


void UIInventoryManager::ActivateSecondarySpell( int charIndex )
{
	if ( gUIPartyManager.GetSelectedCharacterFromIndex( charIndex ) == NULL )
	{
		return;
	}

	GoHandle hMember( gUIPartyManager.GetSelectedCharacterFromIndex( charIndex )->GetGoid() );

	if ( !hMember.IsValid() )
	{
		return;
	}

	if ( hMember->GetAspect()->GetLifeState() != LS_ALIVE_CONSCIOUS )
	{
		return;
	}

	if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember, true ) )
	{
		UpdateInventoryCallback( hMember->GetGoid() );
		return;
	}

	if ( hMember->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL ) &&
		 hMember->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL )->HasMagic() &&
		 hMember->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL )->GetMagic()->CanOwnerCast() )
	{
		hMember->GetMind()->RSDoJob( MakeJobReq( JAT_STOP, JQ_ACTION, QP_CLEAR, AO_USER ) );				
		hMember->GetInventory()->RSSelect( IL_ACTIVE_SECONDARY_SPELL, AO_USER );
		hMember->PlayVoiceSound( "active_spell_select", false );
	}

	gUIPartyManager.GetSelectedCharacterFromIndex( charIndex )->SetActiveSlot( SELECTION_SECONDARY_SPELL );
}


eInventoryLocation UIInventoryManager::ActivateNextValid( int charIndex, bool bForwards )
{
	if ( gUIPartyManager.GetSelectedCharacterFromIndex( charIndex ) == NULL )
	{
		return IL_ACTIVE_MELEE_WEAPON;
	}

	GoHandle hMember( gUIPartyManager.GetSelectedCharacterFromIndex( charIndex )->GetGoid() );

	if ( !hMember.IsValid() || gUIPartyManager.IsCharacterInterfaceBusy( hMember ) ||
		 hMember->GetAspect()->GetLifeState() != LS_ALIVE_CONSCIOUS )
	{
		return IL_ACTIVE_MELEE_WEAPON;
	}

	bool bValid = false;
	bool bRotateFull = true;
	eInventoryLocation location = hMember->GetInventory()->GetSelectedLocation();
	eInventoryLocation startLocation = location;
	while ( !bValid )
	{
		int gui_index = 1;

		if ( bForwards )
		{
			switch ( location )
			{
			case IL_ACTIVE_MELEE_WEAPON:
				location = IL_ACTIVE_RANGED_WEAPON;
				gui_index = 2;
				break;
			case IL_ACTIVE_RANGED_WEAPON:
				location = IL_ACTIVE_PRIMARY_SPELL;
				gui_index = 3;
				break;
			case IL_ACTIVE_PRIMARY_SPELL:
				location = IL_ACTIVE_SECONDARY_SPELL;
				gui_index = 4;
				break;
			case IL_ACTIVE_SECONDARY_SPELL:
				location = IL_ACTIVE_MELEE_WEAPON;
				gui_index = 1;
				break;
			}
		}
		else
		{
			switch ( location )
			{
			case IL_ACTIVE_MELEE_WEAPON:
				location = IL_ACTIVE_SECONDARY_SPELL;
				gui_index = 4;
				break;
			case IL_ACTIVE_RANGED_WEAPON:
				location = IL_ACTIVE_MELEE_WEAPON;
				gui_index = 3;
				break;
			case IL_ACTIVE_PRIMARY_SPELL:
				location = IL_ACTIVE_RANGED_WEAPON;
				gui_index = 2;
				break;
			case IL_ACTIVE_SECONDARY_SPELL:
				location = IL_ACTIVE_PRIMARY_SPELL;
				gui_index = 1;
				break;
			}
		}

		gpstring sWindow;
		sWindow.assignf( "awp_radio_button_character_%d_slot_%d", charIndex+1, gui_index );
		UIRadioButton *pRadio = (UIRadioButton *)gUI.GetGameGUI().FindUIWindow( sWindow );

		Go * pItem = hMember->GetInventory()->GetItem( location );

		if ( pRadio && (pItem || (!pItem && location == IL_ACTIVE_MELEE_WEAPON && !bRotateFull)) &&
			 ( !pItem || !pItem->HasMagic() || !pItem->IsSpell() || pItem->GetMagic()->CanOwnerCast() ) )
		{
			pRadio->SetCheck( true );
			hMember->GetInventory()->RSSelect( location, AO_USER );
			UpdateInventoryCallback( hMember->GetGoid() );			
			bValid = true;
		}
		else
		{
			if ( location > IL_ACTIVE_SECONDARY_SPELL )
			{
				location = IL_ACTIVE_MELEE_WEAPON;
			}
		}

		if ( location == startLocation && !bValid )
		{
			bRotateFull = false;
		}
	}

	return location;
}


void UIInventoryManager::ItemslotPlaceItem( UIItemSlot * pSlot )
{	
	if ( MakeGoid( pSlot->GetItemID() ) != GOID_INVALID )
	{
		GoHandle hItem( MakeGoid( pSlot->GetItemID() ) );
		GoHandle hMember( gUIGame.GetActorWhoCarriesObject( hItem->GetGoid() ) );
		if ( !hMember.IsValid() )
		{
			return;
		}

		if( hItem->HasGui() && hItem->GetGui()->GetEquipSlot() != ES_NONE )
		{
			if ( hItem->GetGui()->GetEquipSlot() == ES_RING )
			{
				eEquipSlot slot = ES_RING;
				if ( pSlot->GetName().same_no_case( "ring1" ) )
				{
					slot = ES_RING_0;
				}
				else if ( pSlot->GetName().same_no_case( "ring2" ) )
				{
					slot = ES_RING_1;
				}
				else if ( pSlot->GetName().same_no_case( "ring3" ) )
				{
					slot = ES_RING_2;
				}
				else if ( pSlot->GetName().same_no_case( "ring4" ) )
				{
					slot = ES_RING_3;
				}
				PaperDollHandleItem( slot, hItem->GetGoid() );
			}
			else
			{
				PaperDollHandleItem( hItem->GetGui()->GetEquipSlot(), hItem->GetGoid() );
			}

			if ( hMember->IsScreenPartyMember() )
			{
				hItem->PlayVoiceSound( "put_down", false );
			}
		}

		if ( hItem->HasMagic() && hItem->GetMagic()->HasNonInnateEnchantments() )
		{
			pSlot->SetSpecialActive( true );
			pSlot->SetSpecialColor( GetGridMagicColor() );
		}
		else
		{
			pSlot->SetSpecialActive( false );
		}
	}

	UIItemVec items;
	gUIShell.FindActiveItems( items );

	UIWindowVec windows = gUIShell.ListWindowsOfGroup( "equipment" );
	UIWindowVec::iterator i;
	for ( i = windows.begin(); i != windows.end(); ++i )
	{
		if ( (*i)->GetType() == UI_TYPE_ITEMSLOT )
		{
			UIItemSlot * pEquipslot = (UIItemSlot *)(*i);
			if ( pEquipslot != pSlot && pEquipslot->GetSlotType().same_no_case( pSlot->GetSlotType() ) )
			{
				if ( items.size() != 2 )
				{
					pEquipslot->ActivateSlotHighlight( false );
				}
				else if ( items.size() == 2 )
				{
					pEquipslot->ActivateSlotHighlight( true );
				}
			}
		}
	}

	CalculateAllGridHighlights();
}


void UIInventoryManager::InfoslotRemoveItem( UIInfoSlot * pSlot )
{
	m_bSpellBookBusy = true;
	if ( m_floating_item != GOID_INVALID )
	{
		GoHandle hMember( gUIPartyManager.GetSelectedCharacter() );
		if ( pSlot->GetSlotType().same_no_case( "scroll" ) )
		{
			GoHandle hSpell( m_floating_item );
			if ( hSpell.IsValid() )
			{
				Go * pSpellBook = hMember->GetInventory()->GetActiveSpellBook();
				if ( pSpellBook )
				{					
					pSpellBook->GetInventory()->RSTransfer( hSpell, hMember, IL_MAIN, AO_USER );
				}

				hSpell->PlayVoiceSound( "remove_spell", false );
			}
			else
			{
				pSlot->ClearItem();
			}

			gpstring sGroup;
			sGroup.assignf( "spell_slot_%d", pSlot->GetIndex()+1 );
			gUI.GetGameGUI().ShowGroup( sGroup, false );
			
			gpstring sTemp;
			sTemp.assignf( "spell_item_%d", pSlot->GetIndex() + 1 );
			bool bSuccess = false;
			gpstring sTexture = GetTextureStringFromIcon( hSpell, hSpell->GetGui()->GetActiveIcon(), bSuccess );		
			if ( !sTexture.empty() )
			{
				UIItemSlot * pIcon = (UIItemSlot *)gUI.GetGameGUI().FindUIWindow( sTemp );
				if ( pIcon )
				{
					pIcon->SetHasTexture( false );
				}
			}
		}
	}
}


void UIInventoryManager::InfoslotPlaceItem( UIInfoSlot * pSlot )
{	
	if ( MakeGoid( pSlot->GetItemID() ) != GOID_INVALID )
	{
		GoHandle hSpell( MakeGoid( pSlot->GetItemID() ) );				
		if ( hSpell.IsValid() && hSpell->IsSpell() )
		{		
			GoHandle hMember( gUIGame.GetActorWhoCarriesObject( hSpell->GetGoid() ) );
			Go * pSpellBook = hMember->GetInventory()->GetActiveSpellBook();
			if ( !pSpellBook )
			{
				return;
			}

			int index = pSlot->GetIndex();
			index += (int)IL_ACTIVE_COUNT;
			
			hMember->GetInventory()->RSTransfer( hSpell, pSpellBook, (eInventoryLocation)index, AO_USER );
			m_bLearnedSpell = true;

			int charIndex = 0;
			gUIPartyManager.GetCharacterFromGoid( hMember->GetGoid(), charIndex );

			if ( hSpell->GetMagic()->CanOwnerCast() )
			{
				if ( (eInventoryLocation)index == IL_SPELL_1 )
				{
					ActivatePrimarySpell( charIndex );
				}
				else if ( (eInventoryLocation)index == IL_SPELL_2 )
				{
					ActivateSecondarySpell( charIndex );
				}
			}

			hSpell->PlayVoiceSound( "add_spell", false );

			gpstring sGroup;
			sGroup.assignf( "spell_slot_%d", pSlot->GetIndex()+1 );
			gUI.GetGameGUI().ShowGroup( sGroup, true );		

			gpstring sTemp;
			sTemp.assignf( "text_box_spell_%d", pSlot->GetIndex() + 1 );
			UITextBox * pText = (UITextBox *)gUI.GetGameGUI().FindUIWindow( sTemp );
			if ( pText )
			{
				gpwstring sName;
				if ( hSpell->GetMagic()->GetMagicClass() == MC_NATURE_MAGIC )
				{
					sName.appendf( L"<c:0x%x>%s</c>", gUIShell.GetTipColor( "nature_magic" ), hSpell->GetCommon()->GetScreenName().c_str() );
				}
				else if ( hSpell->GetMagic()->GetMagicClass() == MC_COMBAT_MAGIC )
				{
					sName.appendf( L"<c:0x%x>%s</c>", gUIShell.GetTipColor( "combat_magic" ), hSpell->GetCommon()->GetScreenName().c_str() );
				}			
				
				pText->SetText( sName );
			}

			sTemp.assignf( "spell_item_%d", pSlot->GetIndex() + 1 );
			bool bSuccess = false;
			gpstring sTexture = GetTextureStringFromIcon( hSpell, hSpell->GetGui()->GetActiveIcon(), bSuccess );		
			if ( !sTexture.empty() )
			{
				UIItemSlot * pIcon = (UIItemSlot *)gUI.GetGameGUI().FindUIWindow( sTemp );
				if ( pIcon )
				{
					pIcon->SetHasTexture( true );
					pIcon->LoadTexture( sTexture );
				}
			}
		}
	}
}


void UIInventoryManager::InfoslotEquip( UIInfoSlot * pSlot )
{
	if ( m_bSpellBookBusy )
	{
		pSlot->SetCanEquip( false );
		return;
	}	

	GoHandle hSpell( MakeGoid( pSlot->GetAttemptEquipId() ) );
	GoHandle hMember( gUIPartyManager.GetSelectedCharacter() );
	if ( hSpell.IsValid() && hMember.IsValid() && hSpell->IsSpell() )
	{
		GoHandle hMember( gUIGame.GetActorWhoCarriesObject( hSpell->GetGoid() ) );
		if ( !hMember.IsValid() && hSpell->GetParent() && hSpell->GetParent()->HasGui() && hSpell->GetParent()->GetGui()->IsSpellBook() )
		{
			// hMember = gUIGame.GetActorWhoCarriesObject( hSpell->GetParent()->GetGoid() );			
			pSlot->SetCanEquip( false );
			return;
		}

		if ( !hMember->GetInventory()->GetGridbox()->GetLocalPlace() || !gUIShell.GetGridPlace() )
		{
			pSlot->SetCanEquip( false );
			return;
		}

		Go * pSpellBook = hMember->GetInventory()->GetActiveSpellBook();
		if ( !pSpellBook )
		{
			pSlot->SetCanEquip( false );
			return;
		}

		GopColl items;
		pSpellBook->GetInventory()->ListItems( IL_ALL_SPELLS, items );
		if ( pSpellBook->GetInventory()->LocationContainsTemplate( IL_ALL_SPELLS, hSpell->GetTemplateName() ) &&
		     !hSpell->GetMagic()->GetIsOneUse() )

		{
			eInventoryLocation il = pSpellBook->GetInventory()->GetLocation( hSpell );
			if ( il == IL_INVALID )
			{
				pSlot->SetCanEquip( false );
				return;
			}
			else
			{
				int i = (int)il - (int)IL_ACTIVE_COUNT;
				gpstring sTemp;
				sTemp.assignf( "spell_slot_%d", i + 1 );
				UIInfoSlot * pTestSlot = (UIInfoSlot *)gUIShell.FindUIWindow( sTemp, "spell" );
				if ( pTestSlot->GetItemID() == MakeInt( hSpell->GetGoid() ) )
				{
					pSlot->SetCanEquip( false );
					return;
				}
			}
		}
		/*
		else if ( hSpell->GetMagic()->GetCharges() != -1 )
		{
			pSlot->SetCanEquip( true );
			GoHandle hCurrent( MakeGoid( pSlot->GetItemID() ) );
			if ( hCurrent.IsValid() && gpstring( hCurrent->GetTemplateName() ).same_no_case( hSpell->GetTemplateName() ) )
			{
				if ( hCurrent->GetMagic()->GetCharges() != hCurrent->GetMagic()->GetMaxCharges() )
				{
					pSlot->SetCanEquip( false );
					int numCharges = hCurrent->GetMagic()->GetMaxCharges() - hCurrent->GetMagic()->GetCharges();
					if ( hSpell->GetMagic()->GetCharges() <= numCharges )
					{
						hCurrent->GetMagic()->RSSetCharges( hSpell->GetMagic()->GetCharges() );
						// hCurrent->SetCharges( hSpell->GetMagic()->GetCharges() );
					}
					else
					{
						hCurrent->GetMagic()->RSSetCharges( hCurrent->GetMagic()->GetMaxCharges() );
						hSpell->GetMagic()->RSSetCharges( hSpell->GetMagic()->GetCharges() - numCharges );
					}
				}
			}
		}
		*/
	}

	m_bSpellBookBusy = true;
	pSlot->SetCanEquip( true );
}


void UIInventoryManager::InfoslotRequestRemove( UIInfoSlot * pSlot )
{	
	GoHandle hSpell( MakeGoid( pSlot->GetAttemptEquipId() ) );
	if ( hSpell.IsValid() )
	{
		GoHandle hMember( gUIGame.GetActorWhoCarriesObject( hSpell->GetGoid() ) );
		if ( hMember.IsValid() )
		{
			pSlot->SetCanRemove( false );
			return;
		}
		else if ( !gUIShell.GetGridPlace() ||
				  (hSpell.IsValid() && hSpell->HasParent() && hSpell->GetParent()->IsSpellBook() &&
				   hSpell->GetParent()->HasParent())  )
		{
			if ( hSpell->GetParent()->GetParent()->GetInventory()->HasGridbox() && !hSpell->GetParent()->GetParent()->GetInventory()->GetGridbox()->GetLocalPlace() )
			{
				pSlot->SetCanRemove( false );
				return;		
			}			
		}
	}

	pSlot->SetCanRemove( true );
}


void UIInventoryManager::ActivateItem( Goid item )
{
	m_floating_item = item;
}


// Display magic spells and their information in the user interface.
void UIInventoryManager::DisplayMagic()
{
	// Display the appropriate magic information
	if (	!gUIPartyManager.GetSelectedPartyMember() ||
			( gUIPartyManager.GetSelectedPartyMember()->GetGoid() == GOID_INVALID ) &&
			( gUIPartyManager.GetSelectedPartyMember()->IsSpellsExpanded() == false ))
	{
		return;
	}

	gpstring sGroup;

	for ( int j = 0; j < GoInventory::MAX_SPELLS; ++j )
	{
		sGroup.assignf( "spell_slot_%d", j+1 );
		gUI.GetGameGUI().ShowGroup( sGroup, false );
	}

	if ( gUIPartyManager.GetSelectedPartyMember()->GetGoid() == GOID_INVALID ) {
		return;
	}

	GoHandle hMember( gUIPartyManager.GetSelectedPartyMember()->GetGoid() );
	if ( !hMember.IsValid() )
	{
		return;
	}

	// Set the text members in the GameGUI to display this spell's information

	if ( gUIPartyManager.GetSelectedPartyMember()->IsSpellsExpanded() )
	{		
		UIButton * pWindow = (UIButton *)gUIShell.FindUIWindow( "button_spellbook_hide", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( true );
		}
		if ( !hMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
		{
			pWindow->DisableButton();
		}
		else
		{
			pWindow->EnableButton();
		}

		pWindow = (UIButton *)gUIShell.FindUIWindow( "button_spellbook_show", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}

		if ( !hMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
		{
			pWindow->DisableButton();
		}
		else
		{
			pWindow->EnableButton();
		}
	}
	else
	{
		UIButton * pWindow = (UIButton *)gUIShell.FindUIWindow( "button_spellbook_hide", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}
		if ( !hMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
		{
			pWindow->DisableButton();
		}
		else
		{
			pWindow->EnableButton();
		}

		pWindow = (UIButton *)gUIShell.FindUIWindow( "button_spellbook_show", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( true );
		}

		if ( !hMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
		{
			pWindow->DisableButton();
		}
		else
		{
			pWindow->EnableButton();
		}
	}

	GopColl spells;

	Go * pSpellBook = hMember->GetInventory()->GetActiveSpellBook();
	if ( !pSpellBook )
	{
		if ( gUIShell.IsInterfaceVisible( "character" ) )
		{
			gUIPartyManager.SetSpellsActive( false );
		}
		return;
	}

	if ( !hMember->GetInventory()->Contains( pSpellBook ) )
	{
		if ( gUIShell.IsInterfaceVisible( "character" ) )
		{
			gUIPartyManager.SetSpellsActive( false );
		}
		return;
	}

	UIEditBox * pName = (UIEditBox *)gUIShell.FindUIWindow( "edit_spellbook" );
	if ( pSpellBook->GetCommon()->GetCustomName().empty() )
	{
		pName->SetText( gpwtranslate( $MSG$ "Spell Book" ) );
	}
	else
	{
		pName->SetText( pSpellBook->GetCommon()->GetCustomName() );
	}

	int start	= (int)IL_SPELL_1;
	int end		= (int)IL_SPELL_12 + 1;

	for( int is = start; is != end; ++is )
	{
		gpstring sTemp;
		gpwstring sData;
		UITextBox *pText = 0;

		int i = is - (int)IL_ACTIVE_COUNT;

		sTemp.assignf( "spell_slot_%d", i + 1 );
		UIInfoSlot * pSlot = (UIInfoSlot *)gUIShell.FindUIWindow( sTemp, "spell" );

		Go * spell = pSpellBook->GetInventory()->GetItem( (eInventoryLocation)is );
		if ( !spell )
		{
			pSlot->ClearItem();
			pSlot->SetInvalid( false );
			continue;
		}

		if ( pSlot->GetItem() && pSlot->GetItem()->GetActive() )
		{
			continue;
		}

		if ( pSlot->GetItemID() != MakeInt( spell->GetGoid() ) )
		{
			pSlot->ClearItem();
			pSlot->SetJustPlaced( false );
			pSlot->PlaceItem( GetItemFromGO( spell, false ), false, false, false, false );
		}

		gUI.GetGameGUI().ShowGroup( sTemp, true );

		if ( spell->GetMagic()->CanOwnerCast() == false )
		{
			pSlot->SetInvalid( true );
		}
		else
		{
			pSlot->SetInvalid( false );
		}

		sTemp.assignf( "text_box_spell_%d", i + 1 );
		pText = (UITextBox *)gUI.GetGameGUI().FindUIWindow( sTemp );
		if ( pText )
		{
			gpwstring sName;
			if ( spell->GetMagic()->GetMagicClass() == MC_NATURE_MAGIC )
			{
				sName.appendf( L"<c:0x%x>%s</c>", gUIShell.GetTipColor( "nature_magic" ), spell->GetCommon()->GetScreenName().c_str() );
			}
			else if ( spell->GetMagic()->GetMagicClass() == MC_COMBAT_MAGIC )
			{
				sName.appendf( L"<c:0x%x>%s</c>", gUIShell.GetTipColor( "combat_magic" ), spell->GetCommon()->GetScreenName().c_str() );
			}

			sData.assignf( L"%s", sName.c_str() );			
			pText->SetText( sData );
		}

		sTemp.assignf( "spell_item_%d", pSlot->GetIndex() + 1 );
		bool bSuccess = false;
		gpstring sTexture = GetTextureStringFromIcon( spell, spell->GetGui()->GetActiveIcon(), bSuccess );		
		if ( !sTexture.empty() )
		{
			UIItemSlot * pIcon = (UIItemSlot *)gUI.GetGameGUI().FindUIWindow( sTemp );
			if ( pIcon )
			{
				pIcon->SetHasTexture( true );
				pIcon->LoadTexture( sTexture );
			}
		}
	}
}

void UIInventoryManager::UpdateMagic()
{
	// Display the appropriate magic information
	if (	!gUIPartyManager.GetSelectedPartyMember() ||
			( gUIPartyManager.GetSelectedPartyMember()->GetGoid() == GOID_INVALID ) &&
			( gUIPartyManager.GetSelectedPartyMember()->IsSpellsExpanded() == false ))
	{
		return;
	}
		
	GoHandle hMember( gUIPartyManager.GetSelectedPartyMember()->GetGoid() );
	if ( !hMember.IsValid() )
	{
		return;
	}

	// Set the text members in the GameGUI to display this spell's information
	if ( gUIPartyManager.GetSelectedPartyMember()->IsSpellsExpanded() )
	{		
		UIButton * pWindow = (UIButton *)gUIShell.FindUIWindow( "button_spellbook_hide", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( true );
		}
		if ( !hMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
		{
			pWindow->DisableButton();
		}
		else
		{
			pWindow->EnableButton();
		}

		pWindow = (UIButton *)gUIShell.FindUIWindow( "button_spellbook_show", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}

		if ( !hMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
		{
			pWindow->DisableButton();
		}
		else
		{
			pWindow->EnableButton();
		}
	}
	else
	{
		UIButton * pWindow = (UIButton *)gUIShell.FindUIWindow( "button_spellbook_hide", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}
		if ( !hMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
		{
			pWindow->DisableButton();
		}
		else
		{
			pWindow->EnableButton();
		}

		pWindow = (UIButton *)gUIShell.FindUIWindow( "button_spellbook_show", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( true );
		}

		if ( !hMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
		{
			pWindow->DisableButton();
		}
		else
		{
			pWindow->EnableButton();
		}
	}

	// Update AWP Slots if necessary
	{
		int charIndex = 0;
		gUIPartyManager.GetCharacterFromGoid( hMember->GetGoid(), charIndex );
		{
			Go * primary_spell = hMember->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL );
			if ( primary_spell )
			{
				gpstring sRadio;
				sRadio.assignf( "awp_radio_button_character_%d_slot_3", charIndex+1  );
				UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );

				if ( primary_spell->GetMagic()->CanOwnerCast() == false )
				{
					pButton->SetAllowUserPress( false );
					pButton->SetInvalid( true );
				}
				else
				{
					pButton->SetAllowUserPress( true );
					pButton->SetInvalid( false );
				}			
			}
			else
			{
				gpstring sRadio;
				sRadio.assignf( "awp_radio_button_character_%d_slot_3", charIndex+1  );
				UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );
				pButton->SetAllowUserPress( false );
				pButton->SetInvalid( false );
			}
		}
		{
			Go * secondary_spell = hMember->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL );
			if ( secondary_spell )
			{
				gpstring sRadio;
				sRadio.assignf( "awp_radio_button_character_%d_slot_4", charIndex+1  );
				UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );

				if ( secondary_spell->GetMagic()->CanOwnerCast() == false )
				{
					pButton->SetAllowUserPress( false );
					pButton->SetInvalid( true );
				}
				else
				{
					pButton->SetAllowUserPress( true );
					pButton->SetInvalid( false );
				}			
			}
			else
			{
				gpstring sRadio;
				sRadio.assignf( "awp_radio_button_character_%d_slot_4", charIndex+1  );
				UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );
				pButton->SetAllowUserPress( false );
				pButton->SetInvalid( false );
			}
		}
	}


	GopColl spells;

	Go * pSpellBook = hMember->GetInventory()->GetActiveSpellBook();
	if ( !pSpellBook )
	{
		if ( gUIShell.IsInterfaceVisible( "character" ) )
		{
			gUIPartyManager.SetSpellsActive( false );
		}
		return;
	}

	if ( !hMember->GetInventory()->Contains( pSpellBook ) )
	{
		if ( gUIShell.IsInterfaceVisible( "character" ) )
		{
			gUIPartyManager.SetSpellsActive( false );
		}
		return;
	}

	int start	= (int)IL_SPELL_1;
	int end		= (int)IL_SPELL_12 + 1;

	for( int is = start; is != end; ++is )
	{		
		int i = is - (int)IL_ACTIVE_COUNT;

		gpstring sTemp;		
		sTemp.assignf( "spell_slot_%d", i + 1 );
		UIInfoSlot * pSlot = (UIInfoSlot *)gUIShell.FindUIWindow( sTemp, "spell" );

		Go * spell = pSpellBook->GetInventory()->GetItem( (eInventoryLocation)is );
		if ( !spell )
		{
			pSlot->SetInvalid( false );
			continue;
		}
	
		if ( spell->GetMagic()->CanOwnerCast() == false )
		{
			pSlot->SetInvalid( true );
		}
		else
		{
			pSlot->SetInvalid( false );
		}
	}
}


void UIInventoryManager::InitWeaponsConfig( int charIndex )
{
	if ( gUIPartyManager.GetSelectedCharacterFromIndex( charIndex ) && ( gUIPartyManager.GetSelectedCharacterFromIndex( charIndex )->GetGoid() != GOID_INVALID ))
	{
		bool bFound = false;
		UIPartyMember * pPartyMem = gUIPartyManager.GetSelectedCharacterFromIndex( charIndex );
		if ( pPartyMem )
		{
			for ( int i = 0; i != MAX_WEAPON_CONFIGS; ++i )
			{
				if ( m_WeaponConfigs[i].member_slots[charIndex].member == pPartyMem->GetGoid() )
				{
					bFound = true;
					break;
				}
			}

			if ( !bFound )
			{
				m_WeaponConfigs[0].member_slots[charIndex].member = pPartyMem->GetGoid();
				m_WeaponConfigs[0].member_slots[charIndex].eSlotType = IL_ACTIVE_MELEE_WEAPON;

				m_WeaponConfigs[1].member_slots[charIndex].member = pPartyMem->GetGoid();
				m_WeaponConfigs[1].member_slots[charIndex].eSlotType = IL_ACTIVE_RANGED_WEAPON;

				m_WeaponConfigs[2].member_slots[charIndex].member = pPartyMem->GetGoid();
				m_WeaponConfigs[2].member_slots[charIndex].eSlotType = IL_ACTIVE_PRIMARY_SPELL;

				m_WeaponConfigs[3].member_slots[charIndex].member = pPartyMem->GetGoid();
				m_WeaponConfigs[3].member_slots[charIndex].eSlotType = IL_ACTIVE_SECONDARY_SPELL;
			}
		}
	}
}


bool UIInventoryManager::SetWeaponsConfig1()
{
	SetWeaponsConfig( 0 );
	return true;
}
bool UIInventoryManager::SetWeaponsConfig2()
{
	SetWeaponsConfig( 1 );
	return true;
}
bool UIInventoryManager::SetWeaponsConfig3()
{
	SetWeaponsConfig( 2 );
	return true;
}
bool UIInventoryManager::SetWeaponsConfig4()
{
	SetWeaponsConfig( 3 );
	return true;
}
bool UIInventoryManager::SetWeaponsConfig5()
{
	SetWeaponsConfig( 4 );
	return true;
}
bool UIInventoryManager::SetWeaponsConfig6()
{
	SetWeaponsConfig( 5 );
	return true;
}
bool UIInventoryManager::SetWeaponsConfig7()
{
	SetWeaponsConfig( 6 );
	return true;
}
bool UIInventoryManager::SetWeaponsConfig8()
{
	SetWeaponsConfig( 7 );
	return true;
}
bool UIInventoryManager::SetWeaponsConfig9()
{
	SetWeaponsConfig( 8 );
	return true;
}
bool UIInventoryManager::SetWeaponsConfig0()
{
	SetWeaponsConfig( 9 );
	return true;
}
void UIInventoryManager::SetWeaponsConfig( int index )
{
	for ( int i = 0; i != MAX_PARTY_MEMBERS; ++i )
	{
		if ( gUIPartyManager.GetSelectedCharacterFromIndex( i ) && ( gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGoid() != GOID_INVALID ))
		{
			m_WeaponConfigs[index].member_slots[i].member = gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGoid();
			GoHandle hMember( gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGoid() );
			m_WeaponConfigs[index].member_slots[i].eSlotType = hMember->GetInventory()->GetSelectedLocation();
		}
	}
}

bool UIInventoryManager::GetWeaponsConfig1()
{
	GetWeaponsConfig( 0 );
	return true;
}
bool UIInventoryManager::GetWeaponsConfig2()
{
	GetWeaponsConfig( 1 );
	return true;
}
bool UIInventoryManager::GetWeaponsConfig3()
{
	GetWeaponsConfig( 2 );
	return true;
}
bool UIInventoryManager::GetWeaponsConfig4()
{
	GetWeaponsConfig( 3 );
	return true;
}
bool UIInventoryManager::GetWeaponsConfig5()
{
	GetWeaponsConfig( 4 );
	return true;
}
bool UIInventoryManager::GetWeaponsConfig6()
{
	GetWeaponsConfig( 5 );
	return true;
}
bool UIInventoryManager::GetWeaponsConfig7()
{
	GetWeaponsConfig( 6 );
	return true;
}
bool UIInventoryManager::GetWeaponsConfig8()
{
	GetWeaponsConfig( 7 );
	return true;
}
bool UIInventoryManager::GetWeaponsConfig9()
{
	GetWeaponsConfig( 8 );
	return true;
}
bool UIInventoryManager::GetWeaponsConfig0()
{
	GetWeaponsConfig( 9 );
	return true;
}
void UIInventoryManager::GetWeaponsConfig( int index )
{
	if ( gWorldState.GetCurrentState() != WS_MP_INGAME_JIP && ( IsSinglePlayer() || ( gUIGame.GetUIEmoteList() && !gUIGame.GetUIEmoteList()->IsVisible() ) ) )
	{
		for ( int i = 0; i != MAX_PARTY_MEMBERS; ++i )
		{
			if ( m_WeaponConfigs[index].member_slots[i].member != GOID_INVALID )
			{
				GoHandle hMember( m_WeaponConfigs[index].member_slots[i].member );
				if ( hMember.IsValid() && hMember->HasInventory() )
				{
					Go * pItem = hMember->GetInventory()->GetItem( m_WeaponConfigs[index].member_slots[i].eSlotType );
					if ( pItem || m_WeaponConfigs[index].member_slots[i].eSlotType == IL_ACTIVE_MELEE_WEAPON )
					{
						if ( !pItem || !pItem->IsSpell() || (pItem->IsSpell() && pItem->GetMagic()->CanOwnerCast()) )
						{
							if ( hMember->GetAspect()->GetLifeState() == LS_ALIVE_CONSCIOUS && hMember->IsScreenPartyMember() )
							{
								if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember, true ) )
								{
									continue; 
								}

								hMember->GetInventory()->RSSelect( m_WeaponConfigs[index].member_slots[i].eSlotType, AO_USER );
								gpstring sWindow;
								sWindow.assignf( "awp_radio_button_character_%d_slot_%d", i+1, (int)(m_WeaponConfigs[index].member_slots[i].eSlotType) + 1 );
								UIRadioButton *pRadio = (UIRadioButton *)gUI.GetGameGUI().FindUIWindow( sWindow );
								pRadio->SetCheck( true );
							}
						}
					}
				}
			}
		}
	}
}

void UIInventoryManager::GetWeaponBinding( Goid weapon, gpstring & sSave, gpstring & sRecall )
{
	for ( int index = 0; index != 10; ++index )
	{
		for ( int i = 0; i != MAX_PARTY_MEMBERS; ++i )
		{
			if ( m_WeaponConfigs[index].member_slots[i].member != GOID_INVALID )
			{
				GoHandle hMember( m_WeaponConfigs[index].member_slots[i].member );
				if ( hMember.IsValid() )
				{
					Go * pItem = hMember->GetInventory()->GetItem( m_WeaponConfigs[index].member_slots[i].eSlotType );
					if ( pItem && pItem->GetGoid() == weapon )
					{
						gpstring sTemp;
						sTemp.assignf( "set_awp_%s%d", index < 9 ? "0" : "", index+1 );
						sSave = sTemp;
						sTemp.assignf( "get_awp_%s%d", index < 9 ? "0" : "", index+1 );
						sRecall = sTemp;
					}
				}
			}
		}
	}
}

bool UIInventoryManager::AwpSlot1()
{
	GopColl::const_iterator i;
	for ( i = gGoDb.GetSelection().begin(); i != gGoDb.GetSelection().end(); ++i )
	{
		int index = 0;
		if ( gUIPartyManager.IsPlayerPartyMember( (*i)->GetGoid(), index ) )
		{
			ActivateMeleeWeapon( index );
		}
	}
	return true;
}


bool UIInventoryManager::AwpSlot2()
{
	GopColl::const_iterator i;
	for ( i = gGoDb.GetSelection().begin(); i != gGoDb.GetSelection().end(); ++i )
	{
		int index = 0;
		if ( gUIPartyManager.IsPlayerPartyMember( (*i)->GetGoid(), index ) )
		{
			ActivateRangedWeapon( index );
		}
	}
	return true;
}


bool UIInventoryManager::AwpSlot3()
{
	GopColl::const_iterator i;
	for ( i = gGoDb.GetSelection().begin(); i != gGoDb.GetSelection().end(); ++i )
	{
		int index = 0;
		if ( gUIPartyManager.IsPlayerPartyMember( (*i)->GetGoid(), index ) )
		{
			ActivatePrimarySpell( index );
		}
	}
	return true;
}


bool UIInventoryManager::AwpSlot4()
{
	GopColl::const_iterator i;
	for ( i = gGoDb.GetSelection().begin(); i != gGoDb.GetSelection().end(); ++i )
	{
		int index = 0;
		if ( gUIPartyManager.IsPlayerPartyMember( (*i)->GetGoid(), index ) )
		{
			ActivateSecondarySpell( index );
		}
	}
	return true;
}


void UIInventoryManager::Xfer( FuBi::PersistContext& persist )
{
	for ( int i = 0; i != MAX_WEAPON_CONFIGS; ++i )
	{
		for ( int j = 0; j != MAX_PARTY_MEMBERS; ++j )
		{
			gpstring sConfig;
			sConfig.assignf( "wc_%d_%d_slot", i, j );
			persist.Xfer( sConfig, m_WeaponConfigs[i].member_slots[j].eSlotType );

			sConfig.assignf( "wc_%d_%d_member", i, j );
			persist.Xfer( sConfig, m_WeaponConfigs[i].member_slots[j].member );
		}
	}
}


bool UIInventoryManager::RotateSelectedSlots( bool bForwards )
{
	if ( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	GoidColl members;
	GoidColl::iterator j;
	gUIPartyManager.GetSelectedPartyMembers( members );
	for ( j = members.begin(); j != members.end(); ++j )
	{
		for ( int i = 0; i != MAX_PARTY_MEMBERS; ++i )
		{
			if ( gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGoid() == (*j) )
			{
				GoHandle hMember( *j );
				if ( hMember.IsValid() )
				{
					if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) ||
						 hMember->GetAspect()->GetLifeState() != LS_ALIVE_CONSCIOUS )
					{
						continue;
					}

					if ( ( hMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
						  hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) )
					{
						continue;
					}

					bool bValid = false;
					eInventoryLocation location = hMember->GetInventory()->GetSelectedLocation();
					while ( !bValid )
					{
						int gui_index = 1;

						if ( bForwards )
						{
							switch ( location )
							{
							case IL_ACTIVE_MELEE_WEAPON:
								location = IL_ACTIVE_RANGED_WEAPON;
								gui_index = 2;
								break;
							case IL_ACTIVE_RANGED_WEAPON:
								location = IL_ACTIVE_PRIMARY_SPELL;
								gui_index = 3;
								break;
							case IL_ACTIVE_PRIMARY_SPELL:
								location = IL_ACTIVE_SECONDARY_SPELL;
								gui_index = 4;
								break;
							case IL_ACTIVE_SECONDARY_SPELL:
								location = IL_ACTIVE_MELEE_WEAPON;
								gui_index = 1;
								break;
							}
						}
						else
						{
							switch ( location )
							{
							case IL_ACTIVE_MELEE_WEAPON:
								location = IL_ACTIVE_SECONDARY_SPELL;
								gui_index = 4;
								break;
							case IL_ACTIVE_RANGED_WEAPON:
								location = IL_ACTIVE_MELEE_WEAPON;
								gui_index = 3;
								break;
							case IL_ACTIVE_PRIMARY_SPELL:
								location = IL_ACTIVE_RANGED_WEAPON;
								gui_index = 2;
								break;
							case IL_ACTIVE_SECONDARY_SPELL:
								location = IL_ACTIVE_PRIMARY_SPELL;
								gui_index = 1;
								break;
							}
						}

						gpstring sWindow;
						sWindow.assignf( "awp_radio_button_character_%d_slot_%d", i+1, gui_index );
						UIRadioButton *pRadio = (UIRadioButton *)gUI.GetGameGUI().FindUIWindow( sWindow );

						Go * pItem = hMember->GetInventory()->GetItem( location );

						if ( pRadio && (pItem || (!pItem && location == IL_ACTIVE_MELEE_WEAPON)) &&
							 ( !pItem || !pItem->HasMagic() || !pItem->IsSpell() || pItem->GetMagic()->CanOwnerCast() ) )
						{
							pRadio->SetCheck( true );
							hMember->GetInventory()->RSSelect( location, AO_USER );
							UpdateInventoryCallback( hMember->GetGoid() );
							bValid = true;
						}
						else
						{
							if ( location > IL_ACTIVE_SECONDARY_SPELL )
							{
								location = IL_ACTIVE_MELEE_WEAPON;
							}
						}
					}
				}
			}
		}
	}
	return true;
}


bool UIInventoryManager::RotateSelectedSlotsPrev()
{
	RotateSelectedSlots( false );

	return true;
}


bool UIInventoryManager::RotatePrimarySpells()
{
	RotateSpellCheck( true );
	return true;
}

bool UIInventoryManager::RotateSecondarySpells()
{
	RotateSpellCheck( false );
	return true;
}


void UIInventoryManager::RotateSpellCheck( bool bPrimarySlot )
{
	if ( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return;
	}
	
	GoidColl members;
	GoidColl::iterator j;
	gUIPartyManager.GetSelectedPartyMembers( members );
	for ( j = members.begin(); j != members.end(); ++j )
	{
		for ( int i = 0; i != MAX_PARTY_MEMBERS; ++i )
		{
			if ( gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGoid() == (*j) )
			{
				GoHandle hMember( *j );
				if ( hMember.IsValid() )
				{
					if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) ||
						 hMember->GetAspect()->GetLifeState() != LS_ALIVE_CONSCIOUS )
					{
						continue;
					}

					if ( ( hMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
						  hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) )
					{
						continue;
					}
				
					Go * pSpellBook = hMember->GetInventory()->GetEquipped( ES_SPELLBOOK );
					if ( !pSpellBook )
					{
						continue;
					}
					
					bool bSpellsExist = false;
					for ( int iSlot = (int)IL_SPELL_3; iSlot <= (int)IL_SPELL_12; ++iSlot )
					{
						if ( pSpellBook->GetInventory()->GetItem( (eInventoryLocation)iSlot ) )
						{
							bSpellsExist = true;
							break;
						}
					}			
					
					if ( bSpellsExist )
					{
						RSRotateSpells( bPrimarySlot, hMember->GetGoid() );						
					}
				}
			}
		}
	}	
}


void UIInventoryManager::RSRotateSpells( bool bPrimarySlot, Goid member )	
{
	FUBI_RPC_THIS_CALL( RSRotateSpells, RPC_TO_SERVER );

	GoHandle hMember( member );
	if ( hMember.IsValid() )
	{
		Go * pSpellBook = hMember->GetInventory()->GetEquipped( ES_SPELLBOOK );
		if ( pSpellBook )
		{
			Go * pActiveSpell	= pSpellBook->GetInventory()->GetItem( bPrimarySlot ? IL_SPELL_1 : IL_SPELL_2 );
			Go * pNextSpell		= 0;

			for ( int iSlot = (int)IL_SPELL_3; iSlot <= (int)IL_SPELL_12; ++iSlot )
			{
				pNextSpell = pSpellBook->GetInventory()->GetItem( (eInventoryLocation)iSlot );
				if ( pNextSpell )
				{
					break;
				}
			}					
			
			if ( !pNextSpell )
			{
				return;
			}			

			pSpellBook->GetInventory()->RSSetLocation( pNextSpell, bPrimarySlot ? IL_SPELL_1 : IL_SPELL_2 );			

			for ( ; iSlot <= (int)IL_SPELL_11; ++iSlot )
			{
				eInventoryLocation nextSlot = (eInventoryLocation)(iSlot+1);
				
				if ( pSpellBook->GetInventory()->GetItem( nextSlot ) )
				{
					pSpellBook->GetInventory()->RSSetLocation( pSpellBook->GetInventory()->GetItem( nextSlot ), (eInventoryLocation)iSlot );
				}
			}		

			eInventoryLocation firstEmpty = IL_SPELL_12;
			for ( iSlot = (int)IL_SPELL_3; iSlot != (int)IL_SPELL_12; ++iSlot )
			{
				if ( !pSpellBook->GetInventory()->GetItem( (eInventoryLocation)iSlot ) )
				{
					firstEmpty = (eInventoryLocation)iSlot;
					break;
				}
			}
			
			if ( pActiveSpell )
			{
				pSpellBook->GetInventory()->RSSetLocation( pActiveSpell, firstEmpty );				
			}
		}
	}

	RCRotateSpells( member );
}


void UIInventoryManager::RCRotateSpells( Goid member )
{
	FUBI_RPC_TAG();

	GoHandle hMember( member );
	if ( !hMember.IsValid() )
	{
		return;
	}

	FUBI_RPC_THIS_CALL( RCRotateSpells, hMember->GetPlayer()->GetMachineId() );

	hMember->GetInventory()->SetInventoryDirty();

	int index = 0;
	gUIPartyManager.GetCharacterFromGoid( member, index );

	if ( hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL )
	{
		ActivatePrimarySpell( index );
	}
	else if ( hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL )
	{
		ActivateSecondarySpell( index );
	}
}



// Drop items that an actor is carrying
void UIInventoryManager::DropSelectedItemsFromActor()
{
	GoHandle hMember( gUIPartyManager.GetSelectedCharacter() );
	if ( !hMember.IsValid() )
	{
		UIItemVec item_vec;
		gUI.GetGameGUI().FindActiveItems( item_vec );
		if ( item_vec.size() != 0 )
		{
			UIItem * pItem = *(item_vec.begin());
			if ( pItem )
			{
				GoHandle hItem( MakeGoid( pItem->GetItemID() ) );
				if ( hItem.IsValid() )
				{
					hMember = gUIGame.GetActorWhoCarriesObject( hItem->GetGoid() );
					if ( !hMember.IsValid() )
					{
						gperrorf( ( "Tell Chad: The selected character is invalid somehow.  Drop request ignored.\n" ) );
						return;
					}
				}
			}
		}
	}

	if ( gUIShell.GetItemActive() )
	{		
		if ( gUIGame.GetUICommands() )
		{
			GoHandle hItem( m_floating_item );
			if ( hItem.IsValid() && hItem->HasParent() )
			{
				gUIGame.GetUICommands()->CommandDrop( m_floating_item, hMember->GetPlacement()->GetPosition(), false, true, hMember );
			}
		}
	}
}


// Drag all user selected items to a character
void UIInventoryManager::DragSelectedItemsToActor( Goid actor )
{
	GPPROFILERSAMPLE( "UIInventoryManager :: DragSelectedItemsToActor", SP_UI );

	if ( !gUIShell.IsInterfaceVisible( "character" ) )
	{
		GoHandle hObject( actor );

		if ( hObject.IsValid() && hObject->HasActor() && IsAlive( hObject->GetLifeState() ) )
		{
			GopColl partyColl;
			Go * pParty = gServer.GetScreenParty();
			if( pParty )
			{
				partyColl = pParty->GetChildren();
			}
			if ( !gUIGame.Contains( actor, partyColl ) ) {
				gUI.GetGameGUI().GetMessenger()->SendUIMessage( UIMessage( MSG_DEACTIVATEITEMS ) );
				return;
			}

			// Dragged onto actor - deselect all characters but this actor and issue command
			gGoDb.DeselectAll();
			gGoDb.Select( actor );
			gUIGame.OrderDraggedRelativeToObject( actor );

			bool noqueueclear = false;
			GoidColl::iterator i;
			GoHandle hMember( actor );
			if ( !hMember.IsValid() ) {
				return;
			}
			if ( hMember->GetInventory()->GetGridbox() ) {
				hMember->GetInventory()->GetGridbox()->SetMultipleItems( false );
			}
			for( i = gUIGame.GetDraggedColl().begin(); i != gUIGame.GetDraggedColl().end(); ++i )	{
				// Loop through dragged objects
				gUIGame.GetUICommands()->ContextActionOnGameObject( *i, noqueueclear, SOURCE_ACTOR );
				noqueueclear = true;
			}
		}

		for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i )
		{
			if (( gUIPartyManager.GetSelectedCharacterFromIndex( i ) == NULL ) || ( gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGoid() == GOID_INVALID ))
			{
				continue;
			}
			else if ( gUIPartyManager.GetSelectedCharacterFromIndex( i )->IsInventoryExpanded() )
			{
				GoHandle hMember( gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGoid() );
				if ( hMember.IsValid() )
				{
					hMember->GetInventory()->GetGridbox()->DetectItemDrop( gUIShell.GetMouseX(), gUIShell.GetMouseY() );
				}
			}
		}

		{			
			gUI.GetGameGUI().GetMessenger()->SendUIMessageToInterface( UIMessage( MSG_DEACTIVATEITEMS ), "icons" );
			gUI.GetGameGUI().GetMessenger()->SendUIMessageToInterface( UIMessage( MSG_DEACTIVATEITEMS ), "multi_inventory" );
			gUI.GetGameGUI().GetMessenger()->SendUIMessageToInterface( UIMessage( MSG_DEACTIVATEITEMS ), "inventory" );
			gUI.GetGameGUI().GetMessenger()->SendUIMessageToInterface( UIMessage( MSG_DEACTIVATEITEMS ), "store" );
			gUI.GetGameGUI().GetMessenger()->SendUIMessageToInterface( UIMessage( MSG_DEACTIVATEITEMS ), "store" );
			gUI.GetGameGUI().GetMessenger()->SendUIMessageToInterface( UIMessage( MSG_DEACTIVATEITEMS ), "multiplayer_trade" );			
		}
	}
}


// Update inventory information and quick-access item slotsss
void UIInventoryManager::UpdateInventoryCallback( Goid object )
{
	if ( m_bUpdatingInventory )
	{
		return;
	}

	m_bUpdatingInventory = true;

	GoHandle hMember;
	GoHandle hObject( object );
	if ( hObject.IsValid() && hObject->IsSpellBook() )
	{
		DisplayMagic();
		m_bSpellBookBusy = false;
		if ( hObject->HasParent() )
		{
			hMember = hObject->GetParent()->GetGoid();
		}
		else
		{
			return;
		}
	}

	if ( !hMember.IsValid() )
	{
		hMember = object;
	}
	
	if ( gUIPartyManager.IsExpertModeEnabled() == false ) 
	{
		for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i )
		{
			if( gUIPartyManager.GetSelectedCharacterFromIndex( i ) == NULL )
			{
				continue;
			}

			if ( hMember->GetGoid() == gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGoid() )
			{
				GoHandle hGO( gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGoid() );
				if ( hGO.IsValid() )
				{
					m_UpdateGridActor = hMember->GetGoid();
					if ( hGO->GetInventory()->GetGridbox()->GetVisible() )
					{
						CalculateGridHighlights( hGO->GetInventory()->GetGridbox() );
					}

					GoHandle hDragged( gUIGame.GetDragged() );
					if ( hDragged.IsValid() && hDragged->HasParent() && hDragged->GetParent() == hGO &&
						 hGO->GetInventory()->GetForceGet() )
					{
						UIItem *pItem = GetItemFromGO( hDragged );
						if ( pItem )
						{
							pItem->SetItemID( MakeInt( hDragged->GetGoid() ) );
						}

						gUIGame.SetDragged( GOID_INVALID );						

						hGO->GetInventory()->RSSetForceGet( false );
					}

					m_UpdateClassGoid = hMember->GetGoid();
					if ( hGO->GetInventory() )
					{
						if ( hGO->GetInventory()->GetActiveSpellBook() &&
							 gUIShell.IsInterfaceVisible( "spell" ) &&
							 gUIPartyManager.GetSelectedCharacterIndex() == i )
						{
							GoHandle hBook( gUIPartyManager.GetFocusSpellBook() );
							if ( hBook && hBook->GetGoid() != hGO->GetInventory()->GetActiveSpellBook()->GetGoid() )
							{
								gUIPartyManager.CloseSpellBook();
							}
						}

						bool bPrimaryExists = hGO->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL ) ? true : false;
						bool bSecondaryExists = hGO->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL ) ? true : false;

						if ( (!bPrimaryExists) || (!bSecondaryExists) )
						{
							GopColl items;
							GopColl::iterator o;
							hGO->GetInventory()->ListItems( IL_ALL_SPELLS, items );

							for ( o = items.begin(); o != items.end(); ++o )
							{
								if (( !bPrimaryExists ) && ( (*o) != hGO->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL ) ))
								{
									hGO->GetInventory()->RSSetLocation( (*o), IL_ACTIVE_PRIMARY_SPELL, false );

									bPrimaryExists = true;

									if ( hGO->GetInventory()->GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL )
									{
										ActivatePrimarySpell( i );
									}
								}
								else if (( !bSecondaryExists )  && ( (*o) != hGO->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL ) ))
								{

									hGO->GetInventory()->RSSetLocation( (*o), IL_ACTIVE_SECONDARY_SPELL, false );

									bSecondaryExists = true;

									if ( hGO->GetInventory()->GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL )
									{
										ActivateSecondarySpell( i );
									}
								}
							}
						}
					}
					
					m_bRefreshSpells = true;

					Go * weapon = hGO->GetInventory()->GetItem( IL_ACTIVE_MELEE_WEAPON );

					bool bDead = hGO->GetAspect()->GetLifeState() != LS_ALIVE_CONSCIOUS;

					if ( weapon )
					{
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_MELEE_WEAPON )->SetHasTexture( true );
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_MELEE_WEAPON )->SetVisible( true );

						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_MELEE_WEAPON )->LoadTexture( gUIPartyManager.GetActiveIconFromGO( weapon ) );

						if( weapon == hGO->GetInventory()->GetSelectedItem() &&
							hGO->GetInventory()->GetSelectedLocation() == IL_ACTIVE_MELEE_WEAPON )
						{
							gpstring sRadio;
							sRadio.assignf( "awp_radio_button_character_%d_slot_1", i+1 );

							UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );							

							if ( !pButton->GetCheck() )
							{
								pButton->SetForceCheck( true );
							}
						}
					}
					else
					{
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_MELEE_WEAPON )->SetHasTexture( false );

						if ( !weapon && hGO->GetInventory()->GetSelectedItem() == NULL )
						{
							gpstring sRadio;
							sRadio.assignf( "awp_radio_button_character_%d_slot_1", i+1 );

							UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );													

							if ( !pButton->GetCheck() )
							{
								pButton->SetForceCheck( true );
							}
						}
					}

					if ( bDead )
					{
						gpstring sRadio;
						sRadio.assignf( "awp_radio_button_character_%d_slot_1", i+1 );
						UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );						
						pButton->SetAllowUserPress( false );
					}
					else
					{
						gpstring sRadio;
						sRadio.assignf( "awp_radio_button_character_%d_slot_1", i+1 );
						UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );						
						pButton->SetAllowUserPress( true );
					}


					Go * ranged = hGO->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON );

					if ( ranged )
					{
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_RANGED_WEAPON )->SetHasTexture( true );
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_RANGED_WEAPON )->SetVisible( true );

						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_RANGED_WEAPON )->LoadTexture( gUIPartyManager.GetActiveIconFromGO( ranged ) );

						gpstring sRadio;
						sRadio.assignf( "awp_radio_button_character_%d_slot_2", i+1  );
						UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );
						pButton->SetAllowUserPress( true );

						if ( ranged == hGO->GetInventory()->GetSelectedItem() &&
							hGO->GetInventory()->GetSelectedLocation() == IL_ACTIVE_RANGED_WEAPON )
						{
							if ( !pButton->GetCheck() )
							{
								pButton->SetForceCheck( true );
							}						
						}
					}
					else
					{
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_RANGED_WEAPON )->SetHasTexture( false );

						gpstring sRadio;
						sRadio.assignf( "awp_radio_button_character_%d_slot_2", i+1  );
						UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );
						pButton->SetAllowUserPress( false );
					}

					if ( bDead )
					{
						gpstring sRadio;
						sRadio.assignf( "awp_radio_button_character_%d_slot_2", i+1 );
						UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );						
						pButton->SetAllowUserPress( false );
					}

					Go * primary_spell = hGO->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL );

					if ( primary_spell )
					{
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_PRIMARY_SPELL )->SetHasTexture( true );
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_PRIMARY_SPELL )->SetVisible( true );
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_PRIMARY_SPELL )->LoadTexture( gUIPartyManager.GetActiveIconFromGO( primary_spell ) );

						gpstring sRadio;
						sRadio.assignf( "awp_radio_button_character_%d_slot_3", i+1  );
						UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );

						if ( primary_spell->GetMagic()->CanOwnerCast() == false )
						{
							pButton->SetAllowUserPress( false );
							pButton->SetInvalid( true );
						}
						else
						{
							pButton->SetAllowUserPress( true );
							pButton->SetInvalid( false );
						}

						if ( (primary_spell == hGO->GetInventory()->GetSelectedItem() && hGO->GetInventory()->GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL ) || m_bEquippedStaffPrimary )
						{
							if ( m_bEquippedStaffPrimary )
							{
								hGO->GetInventory()->RSSelect( IL_ACTIVE_PRIMARY_SPELL, AO_USER );
								m_bEquippedStaffPrimary = false;
							}

							if ( !pButton->GetCheck() )
							{
								pButton->SetForceCheck( true );
							}
						}
					}
					else
					{
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_PRIMARY_SPELL )->SetHasTexture( false );

						gpstring sRadio;
						sRadio.assignf( "awp_radio_button_character_%d_slot_3", i+1  );
						UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );
						pButton->SetAllowUserPress( false );
						pButton->SetInvalid( false );
					}

					if ( bDead )
					{
						gpstring sRadio;
						sRadio.assignf( "awp_radio_button_character_%d_slot_3", i+1 );
						UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );						
						pButton->SetAllowUserPress( false );
					}

					Go * secondary_spell = hGO->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL );

					if ( secondary_spell )
					{
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_SECONDARY_SPELL )->SetVisible( true );
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_SECONDARY_SPELL )->SetHasTexture( true );
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_SECONDARY_SPELL )->LoadTexture( gUIPartyManager.GetActiveIconFromGO( secondary_spell ) );

						gpstring sRadio;
						sRadio.assignf( "awp_radio_button_character_%d_slot_4", i+1  );
						UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );

						if ( secondary_spell->GetMagic()->CanOwnerCast() == false )
						{
							pButton->SetAllowUserPress( false );
							pButton->SetInvalid( true );
						}
						else
						{
							pButton->SetAllowUserPress( true );
							pButton->SetInvalid( false );
						}

						if ( (secondary_spell == hGO->GetInventory()->GetSelectedItem() && hGO->GetInventory()->GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL) || m_bEquippedStaffSecondary )
						{
							if ( m_bEquippedStaffSecondary )
							{
								hGO->GetInventory()->RSSelect( IL_ACTIVE_SECONDARY_SPELL, AO_USER );
								m_bEquippedStaffSecondary = false;
							}							

							if ( !pButton->GetCheck() )
							{
								pButton->SetForceCheck( true );
							}
						}
					}
					else
					{
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_AIS_SECONDARY_SPELL )->SetHasTexture( false );

						gpstring sRadio;
						sRadio.assignf( "awp_radio_button_character_%d_slot_4", i+1  );
						UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );
						pButton->SetAllowUserPress( false );
						pButton->SetInvalid( false );
					}

					if ( bDead )
					{
						gpstring sRadio;
						sRadio.assignf( "awp_radio_button_character_%d_slot_4", i+1 );
						UIRadioButton * pButton = ( UIRadioButton * )gUIShell.FindUIWindow( sRadio );						
						pButton->SetAllowUserPress( false );
					}

					// Update variable level items ( potions )
					if ( hMember.IsValid() )
					{
						GridItemColl grid_items = hMember->GetInventory()->GetGridbox()->GetGridItems();
						GridItemColl::iterator i;
						for ( i = grid_items.begin(); i != grid_items.end(); ++i ) {
							GoHandle hItem( MakeGoid((*i).itemID) );
							if ( hItem.IsValid() && hItem->IsPotion() && hItem->GetMagic()->HasEnchantments() ) {
								hMember->GetInventory()->GetGridbox()->SetSecondaryPercentVisible( (*i).itemID, hItem->GetMagic()->GetPotionFullRatio() );
							}
						}
					}

					if ( hGO->GetAspect()->GetLifeState() == LS_ALIVE_CONSCIOUS )
					{
						if ( hGO->GetInventory()->GetSelectedLocation() != IL_ACTIVE_MELEE_WEAPON &&
							 hGO->GetInventory()->GetItem( hGO->GetInventory()->GetSelectedLocation() ) == NULL )
						{
							ActivateMeleeWeapon( i );
						}
					}

					UpdateActiveEquipment( i );
					CalculateGridHighlights( GetGridboxFromGO( hGO, i ) );
				}
			}
		}
	}
	else {
		for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i )
		{
			if( gUIPartyManager.GetSelectedCharacterFromIndex( i ) == NULL )
			{
				continue;
			}

			if ( hMember->GetGoid() == gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGoid() )
			{
				GoHandle hGO( gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGoid() );
				if ( hGO.IsValid() )
				{
					m_UpdateGridActor = hMember->GetGoid();
					if ( hGO->GetInventory()->GetGridbox()->GetVisible() )
					{
						CalculateGridHighlights( hGO->GetInventory()->GetGridbox() );
					}

					GoHandle hDragged( gUIGame.GetDragged() );
					if ( hDragged.IsValid() && hDragged->HasParent() && hDragged->GetParent() == hGO &&
						 hGO->GetInventory()->GetForceGet() )
					{
						UIItem *pItem = GetItemFromGO( hDragged );
						if ( pItem )
						{
							pItem->SetItemID( MakeInt( hDragged->GetGoid() ) );
						}

						gUIGame.SetDragged( GOID_INVALID );						

						hGO->GetInventory()->RSSetForceGet( false );

						gUIGame.SetBackendMode( BMODE_DRAG_INVENTORY );
					}

					m_UpdateClassGoid = hMember->GetGoid();
					if ( hGO->GetInventory() )
					{
						if ( hGO->GetInventory()->GetActiveSpellBook() &&
							 gUIShell.IsInterfaceVisible( "spell" ) &&
							 gUIPartyManager.GetSelectedCharacterIndex() == i )
						{
							GoHandle hBook( gUIPartyManager.GetFocusSpellBook() );
							if ( hBook && hBook->GetGoid() != hGO->GetInventory()->GetActiveSpellBook()->GetGoid() )
							{
								gUIPartyManager.CloseSpellBook();
							}
						}

						bool bPrimaryExists = hGO->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL ) ? true : false;
						bool bSecondaryExists = hGO->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL ) ? true : false;

						if ( (!bPrimaryExists) || (!bSecondaryExists) )
						{
							GopColl items;
							GopColl::iterator o;
							hGO->GetInventory()->ListItems( IL_ALL_SPELLS, items );

							for ( o = items.begin(); o != items.end(); ++o )
							{
								if (( !bPrimaryExists ) && ( (*o) != hGO->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL ) ))
								{
									hGO->GetInventory()->RSSetLocation( (*o), IL_ACTIVE_PRIMARY_SPELL, false );

									bPrimaryExists = true;

									if ( hGO->GetInventory()->GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL )
									{
										ActivatePrimarySpell( i );
									}
								}
								else if (( !bSecondaryExists )  && ( (*o) != hGO->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL ) ))
								{

									hGO->GetInventory()->RSSetLocation( (*o), IL_ACTIVE_SECONDARY_SPELL, false );

									bSecondaryExists = true;

									if ( hGO->GetInventory()->GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL )
									{
										ActivateSecondarySpell( i );
									}
								}
							}
						}
					}
					
					m_bRefreshSpells = true;

					Go * active = 0;
					Go * selected = hGO->GetInventory()->GetItem( hGO->GetInventory()->GetSelectedLocation() );
					if ( selected )
					{
						Go * pSpellbook = hGO->GetInventory()->GetEquipped( ES_SPELLBOOK );
						if ( (hGO->GetInventory()->Contains( selected ) ||
							 (selected->IsSpell() && pSpellbook && pSpellbook->GetInventory()->Contains( selected ))
							 && hGO->GetInventory()->GetSelectedLocation() != IL_MAIN ) )
						{
							active = hGO->GetInventory()->GetItem( hGO->GetInventory()->GetSelectedLocation() );
						}

						if ( active )
						{
							if ( !gUIStoreManager.GetStoreActive() && !GetStashActive() )
							{
								gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_ACTIVE_MINIMIZED )->SetVisible( true );
							}

							gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_ACTIVE_MINIMIZED )->SetHasTexture( true );
							gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_ACTIVE_MINIMIZED )->LoadTexture( gUIPartyManager.GetActiveIconFromGO( active ) );
						}
						else {
							gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_ACTIVE_MINIMIZED )->SetVisible( false );
							gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_ACTIVE_MINIMIZED )->SetHasTexture( false );
						}
					}
					else
					{
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_ACTIVE_MINIMIZED )->SetVisible( false );
						gUIPartyManager.GetSelectedCharacterFromIndex( i )->GetGUIWindow( GUI_ACTIVE_MINIMIZED )->SetHasTexture( false );
					}
				}

				// Update variable level items ( potions )
				if ( hMember.IsValid() ) {
					GridItemColl grid_items = hMember->GetInventory()->GetGridbox()->GetGridItems();
					GridItemColl::iterator i;
					for ( i = grid_items.begin(); i != grid_items.end(); ++i ) {
						GoHandle hItem( MakeGoid((*i).itemID) );
						if ( hItem.IsValid() && hItem->HasMagic() && hItem->GetMagic()->IsPotion() && hItem->GetMagic()->HasEnchantments() ) {
							hMember->GetInventory()->GetGridbox()->SetSecondaryPercentVisible( (*i).itemID, hItem->GetMagic()->GetPotionFullRatio() );
						}
					}
				}

				if ( hGO->GetAspect()->GetLifeState() == LS_ALIVE_CONSCIOUS )
				{
					if ( hGO->GetInventory()->GetSelectedLocation() != IL_ACTIVE_MELEE_WEAPON &&
						 hGO->GetInventory()->GetItem( hGO->GetInventory()->GetSelectedLocation() ) == NULL  )
					{
						JobQueue jobs = hMember->GetMind()->GetQ( JQ_ACTION );
						JobQueue::iterator j;
						bool bContinue = true;
						for ( j = jobs.begin(); j != jobs.end(); ++j )
						{
							GoHandle hItem( (*j)->GetGoalObject() );;
							if ( (*j)->GetJobAbstractType() == JAT_EQUIP && hItem.IsValid() )
							{
								bContinue = false;
							}
						}

						if ( bContinue )
						{
							ActivateMeleeWeapon( i );
						}
					}
				}

				UpdateActiveEquipment( i );
				CalculateGridHighlights( GetGridboxFromGO( hGO, i ) );
			}
		}
	}

	{
		GopColl partyColl;
		GopColl::iterator iMember;
		Go * pParty = gServer.GetScreenParty();

		if( pParty != NULL )
		{
			partyColl = pParty->GetChildren();

			bool bHealth = false;
			bool bMana = false;
			for ( iMember = partyColl.begin(); iMember != partyColl.end(); ++iMember )
			{
				GopColl InventoryColl;
				if( (*iMember)->IsActor() && !(*iMember)->GetInventory()->IsPackOnly() && (*iMember)->GetInventory()->ListItems( QT_POTION, IL_ALL, InventoryColl ) )
				{
					GopColl ResultColl;
					if( gAIQuery.GetFirstN( (*iMember), QT_LIFE_HEALING, 1, InventoryColl, ResultColl ) )
					{
						bHealth = true;
					}

					if( gAIQuery.GetFirstN( (*iMember), QT_MANA_HEALING, 1, InventoryColl, ResultColl ) )
					{
						bMana = true;
					}

					if ( bHealth && bMana )
					{
						break;
					}
				}
			}

			gUIPartyManager.EnableHealthButton( bHealth );
			gUIPartyManager.EnableManaButton( bMana );
		}
	}

	m_bUpdatingInventory = false;
}

void UIInventoryManager::UpdateGridHighlights()
{
	if ( m_UpdateGridActor != GOID_INVALID )
	{
		// Refresh the active actor grid
		GoHandle hMember( m_UpdateGridActor );
		if ( hMember.IsValid() && hMember->HasInventory() && hMember->GetInventory()->HasGridbox() )
		{			
			CalculateGridHighlights( hMember->GetInventory()->GetGridbox() );
		}
		m_UpdateGridActor = GOID_INVALID;

		// Refresh the store grid
		if ( gUIStoreManager.GetStoreActive() )
		{
			GoHandle hStore( gUIStoreManager.GetActiveStore() );
			if ( hStore && hStore->HasStore() && hStore->GetStore()->IsItemStore() )
			{
				CalculateGridHighlights( hStore->GetStore()->GetGridbox() );
			}
		}

		// Refresh the active stash grid
		if ( GetStashActive() )
		{
			UIGridbox * pGrid = (UIGridbox *)gUIShell.FindUIWindow( "gridbox_stash", "party_stash" );
			if ( pGrid )
			{
				CalculateGridHighlights( pGrid );
			}
		}
	}
}


void UIInventoryManager::TestEquipCallback( Goid actor )
{
	m_TestEquipActors.push_back( actor ); 
}

void UIInventoryManager::TestEquipment()
{
	if ( m_TestEquipActors.size() != 0 && m_bTestEquipSkip )
	{
		GoidColl::iterator iMember;
		for ( iMember = m_TestEquipActors.begin(); iMember != m_TestEquipActors.end(); ++iMember )
		{
			GoHandle hMember( *iMember );
			if ( hMember && hMember->IsScreenPartyMember() )
			{
				GopColl inventory;
				GopSet dropItems;
				GopSet::iterator iDrop;
				GopColl::iterator iItem;
				hMember->GetInventory()->ListItems( IL_ALL, inventory );
				for ( iItem = inventory.begin(); iItem != inventory.end(); ++iItem )
				{
					if ( (hMember->GetInventory()->IsEquipped( *iItem ) ||
						  hMember->GetInventory()->GetLocation( *iItem ) == IL_SHIELD ||
						  hMember->GetInventory()->GetLocation( *iItem ) == IL_ACTIVE_MELEE_WEAPON ||
						  hMember->GetInventory()->GetLocation( *iItem ) == IL_ACTIVE_RANGED_WEAPON)							
						  && !hMember->GetInventory()->TestEquipMinReqs( *iItem ) )
					{
						dropItems.insert( *iItem );
					}
				}

				for ( iDrop = dropItems.begin(); iDrop != dropItems.end(); ++iDrop )
				{
					SiegePos dropPos;
					Quat dropQuat;
					if( gAIQuery.FindSpotRelativeToSource(	hMember,
															gWorldOptions.GetDropItemsDistanceMin(),
															gWorldOptions.GetDropItemsDistanceMax(),
															gWorldOptions.GetInventorySpewVerticalConstraint(),
															dropPos ) )
					{
						gAIQuery.GetRandomOrientation( dropQuat );
					}

					hMember->GetMind()->RSDrop( (*iDrop), dropPos, QP_FRONT, AO_REFLEX );								
				}
			}
		}
	
		m_TestEquipActors.clear();
		m_bTestEquipSkip = false;
	}
	else if ( m_TestEquipActors.size() != 0 )
	{
		m_bTestEquipSkip = true;
	}
}


void UIInventoryManager::DisplayRolloverItemData( Goid item, UITextBox * pBox )
{
	GoHandle hItem( item );
	if ( hItem.IsValid() && hItem->HasGui() ) {

		pBox->SetText( L"" );

		gpwstring sTemp = hItem->GetGui()->MakeToolTipString();

		if ( hItem->HasParent() )
		{				
			if ( hItem->GetParent()->HasStore() && !hItem->GetParent()->IsScreenPartyMember() )
			{
				 hItem->GetParent()->GetStore()->AppendToolTipString( item, sTemp );
			}
			else if ( gUIStoreManager.GetActiveStore() != GOID_INVALID )
			{
				GoHandle hStore( gUIStoreManager.GetActiveStore() );
				if ( hStore.IsValid() )
				{
					if ( !sTemp.empty() )
					{
						sTemp += L'\n';
					}

					if ( hItem->HasGui() && hItem->GetGui()->GetCanSell() )
					{
						sTemp.appendf( gpwtranslate( $MSG$ "Sell Value %d" ), hStore->GetStore()->GetPrice( hItem->GetGoid(), hItem->GetParent()->GetGoid() ) );
					}
					else
					{
						sTemp.appendf( gpwtranslate( $MSG$ "<c:0x%x>This item cannot be sold</c>" ), gUIShell.GetTipColor( "required" ) );
					}
				}
			}
		}

		if ( !sTemp.empty() )
		{
			pBox->SetToolTipText( sTemp );
		}
		else
		{
			pBox->SetVisible( false );
		}
	}
}


void UIInventoryManager::DisplayWorldRolloverItemData( Goid item )
{	
	GoHandle hItem( item );

	if ( hItem && hItem->HasGui() )
	{
		UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "gui_rollover_textbox" );		

		bool bDisplay = hItem->GetGui()->GetIsIdentified() || hItem->IsSpell() || !hItem->HasMagic() || 
						(hItem->HasMagic() && !hItem->GetMagic()->HasNonInnateEnchantments());

		if ( !hItem->HasParent() && bDisplay )
		{
			SiegePos spos = hItem->GetPlacement()->GetPosition();
			matrix_3x3 orient;
			vector_3 center, halfDiag;
			hItem->GetAspect()->GetNodeSpaceOrientedBoundingVolume( orient, spos.pos, halfDiag );
			halfDiag = orient * halfDiag;
			float radius = (spos.pos.y - hItem->GetPlacement()->GetPosition().pos.y) + halfDiag.y + 0.1f;
			spos.pos.y += radius;							
			vector_3 pos = spos.ScreenPos();
			
			pTextBox->SetVisible( true );			
			gUIInventoryManager.DisplayRolloverItemData( hItem->GetGoid(), pTextBox );
			int halfWidth	= (int)(pTextBox->GetRect().Width() / 2);
			int height		= pTextBox->GetRect().Height();
			pTextBox->SetRect( (int)pos.x - halfWidth, (int)pos.x + halfWidth, (int)pos.y - height, (int)pos.y );
		}
	}
}


void UIInventoryManager::AwpSlotRollover( UIWindow * pWindow )
{
	GoHandle hMember( gUIPartyManager.GetSelectedCharacterFromIndex( pWindow->GetIndex() )->GetGoid() );

	unsigned int pos = pWindow->GetName().find_last_of( "_" );
	if ( pos != gpstring::npos )
	{
		int slot = 0;
		stringtool::Get( pWindow->GetName().substr( pos+1, pWindow->GetName().size() ).c_str(), slot );
		ACTIVE_SELECTION selection = ACTIVE_SELECTION(slot-1);
		switch ( selection )
		{
		case SELECTION_MELEE_WEAPON:
			if ( hMember->GetInventory()->GetItem( IL_ACTIVE_MELEE_WEAPON ) )
			{
				SetGUIRolloverGOID( hMember->GetInventory()->GetItem( IL_ACTIVE_MELEE_WEAPON )->GetGoid() );
			}
			break;
		case SELECTION_RANGED_WEAPON:
			if ( hMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON ) )
			{
				SetGUIRolloverGOID( hMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON )->GetGoid() );
			}
			break;
		case SELECTION_PRIMARY_SPELL:
			if ( hMember->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL ) )
			{
				SetGUIRolloverGOID( hMember->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL )->GetGoid() );
			}
			break;
		case SELECTION_SECONDARY_SPELL:
			if ( hMember->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL ) )
			{
				SetGUIRolloverGOID( hMember->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL )->GetGoid() );
			}
			break;
		}
		
		if ( selection == -1 && hMember->GetInventory()->GetSelectedItem() )
		{
			SetGUIRolloverGOID( hMember->GetInventory()->GetSelectedItem()->GetGoid() );
		}
		else if ( selection == -1 )
		{
			SetGUIRolloverGOID( GOID_INVALID );
		}

		GoHandle hObject( GetGUIRolloverGOID() );
		if ( hObject.IsValid() || selection == SELECTION_MELEE_WEAPON )
		{
			gpstring sSkillName;
			if( !hObject.IsValid() )
			{
				sSkillName = hMember->GetAttack()->GetSkillClass();
			}
			else
			{
				if( hObject->HasAttack() )
				{
					sSkillName = hObject->GetAttack()->GetSkillClass();
				}

				if( sSkillName.empty() && hObject->HasMagic() )
				{
					sSkillName = hObject->GetMagic()->GetSkillClass();
				}
			}

			gpwstring sSkill;
			Skill * pSkill = 0;
			hMember->GetActor()->GetSkill( sSkillName, &pSkill );
			if ( pSkill )
			{				
				sSkill.assignf( gpwtranslate( $MSG$ "%s, Level: %0.2f" ), pSkill->GetScreenName().c_str(), pSkill->GetLevel() );
			}

			pWindow->SetToolTip( sSkill, pWindow->GetHelpBoxName(), true );
			pWindow->PositionTextBoxOutside();
		}
		else
		{
			pWindow->SetToolTip( gpwstring::EMPTY, pWindow->GetHelpBoxName(), true );
		}
	}
}


void UIInventoryManager::AwpSlotRolloff( UIWindow * pWindow )
{
	GoHandle hMember( gUIPartyManager.GetSelectedCharacterFromIndex( pWindow->GetIndex() )->GetGoid() );

	Goid rolloffGoid = GOID_INVALID;
	unsigned int pos = pWindow->GetName().find_last_of( "_" );
	if ( pos != gpstring::npos )
	{
		int slot = 0;
		stringtool::Get( pWindow->GetName().substr( pos+1, pWindow->GetName().size() ).c_str(), slot );
		ACTIVE_SELECTION selection = ACTIVE_SELECTION(slot-1);
		switch ( selection )
		{
		case SELECTION_MELEE_WEAPON:
			if ( hMember->GetInventory()->GetItem( IL_ACTIVE_MELEE_WEAPON ) )
			{
				rolloffGoid = hMember->GetInventory()->GetItem( IL_ACTIVE_MELEE_WEAPON )->GetGoid();
			}
			break;
		case SELECTION_RANGED_WEAPON:
			if ( hMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON ) )
			{
				rolloffGoid = hMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON )->GetGoid();
			}
			break;
		case SELECTION_PRIMARY_SPELL:
			if ( hMember->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL ) )
			{
				rolloffGoid = hMember->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL )->GetGoid();
			}
			break;
		case SELECTION_SECONDARY_SPELL:
			if ( hMember->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL ) )
			{
				rolloffGoid =  hMember->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL )->GetGoid();
			}
			break;
		}
	}

	if ( rolloffGoid == GetGUIRolloverGOID() )
	{
		SetGUIRolloverGOID( GOID_INVALID );
	}
}


void UIInventoryManager::ListenerCharacterRollover()
{
	if ( gUIShell.GetItemActive() )
	{
		UIItemVec items;
		gUIShell.FindActiveItems( items );
		UIItem * pItem = *(items.begin());

		UIWindowVec windows = gUIShell.ListWindowsOfGroup( "equipment" );
		UIWindowVec::iterator i;
		for ( i = windows.begin(); i != windows.end(); ++i )
		{
			if ( (*i)->GetType() == UI_TYPE_ITEMSLOT )
			{
				if ( ((UIItemSlot *)(*i))->GetSlotType().same_no_case( pItem->GetSlotType() ) )
				{
					GoHandle hMember( gUIPartyManager.GetSelectedCharacter() );
					GoHandle hItem( MakeGoid( pItem->GetItemID() ) );
					if ( hItem.IsValid() && hMember.IsValid() )
					{
						if ( !hMember->GetInventory()->TestEquipMinReqs( hItem ) )
						{
							((UIItemSlot *)(*i))->SetActivateColor( GetGridInvalidColor() );
						}
						else
						{
							((UIItemSlot *)(*i))->SetActivateColor( GetGridRolloverColor() );
						}

						((UIItemSlot *)(*i))->ActivateSlotHighlight( true );
					}
				}
				else if ( pItem->GetSlotType().same_no_case( "scroll" ) &&
						  ((UIItemSlot *)(*i))->GetSlotType().same_no_case( "spellbook" ) &&
						  ((UIItemSlot *)(*i))->GetItemID() != 0 )
				{
					GoHandle hItem( MakeGoid( pItem->GetItemID() ) );
					if ( hItem.IsValid() && hItem->HasParent() )
					{
						((UIItemSlot *)(*i))->SetActivateColor( GetGridRolloverColor() );
						((UIItemSlot *)(*i))->ActivateSlotHighlight( true );
					}					
				}
			}
		}
	}
}


void UIInventoryManager::ListenerCharacterRolloff()
{
	UIWindowVec windows = gUIShell.ListWindowsOfGroup( "equipment" );
	UIWindowVec::iterator i;
	for ( i = windows.begin(); i != windows.end(); ++i )
	{
		if ( (*i)->GetType() == UI_TYPE_ITEMSLOT )
		{
			((UIItemSlot *)(*i))->ActivateSlotHighlight( false );
		}
	}
}


void UIInventoryManager::ContextPlaceItem( bool bUseRollover, Goid member )
{
	UIItem * pItem = 0;

	if ( !bUseRollover )
	{
		UIItemVec items;
		gUIShell.FindActiveItems( items );

		if ( items.size() == 1 )
		{
			pItem = *(items.begin());
		}
	}
	else
	{
		pItem = gUIShell.FindItemByID( MakeInt( GetGUIRolloverGOID() ) );
	}

	if ( pItem )
	{
		pItem->SetScale( 1.0f );  // precaution to prevent getting small inventory items
		GoHandle hObject( MakeGoid( pItem->GetItemID() ) );
		GoHandle hMember( member );
		if ( !hMember.IsValid() )
		{
			hMember = gUIPartyManager.GetSelectedCharacter();
		}
		else
		{
			int index = 0;
			if ( gUIPartyManager.GetCharacterFromGoid( member, index ) )
			{
				UpdateActiveEquipment( index, true );
			}

			gUIPartyManager.SetSelectedCharacterIndex( index );
		}

		if ( hObject.IsValid() && hMember.IsValid() )
		{
			if ( !hMember->GetInventory()->TestEquipMinReqs( hObject ) )
			{
				hMember->PlayVoiceSound( "gui_inventory_not_equippable", false );
				gpscreen( $MSG$ "Cannot equip item" );
				return;
			}
		}

		UIWindowVec windows = gUIShell.ListWindowsOfGroup( "equipment" );
		UIWindowVec::iterator i;
		for ( i = windows.begin(); i != windows.end(); ++i )
		{
			if ( (*i)->GetType() == UI_TYPE_ITEMSLOT )
			{
				UIItemSlot * pSlot = (UIItemSlot *)(*i);
				if ( pSlot->GetSlotType().same_no_case( pItem->GetSlotType() ) && !pSlot->GetSlotType().same_no_case( "ring" ) )
				{
					POINT pt;
					pt.x = gUIShell.GetMouseX();
					pt.y = gUIShell.GetMouseY();
					if ( pSlot->GetRect().Contains( pt ) == false || !gUIShell.IsInterfaceVisible( "character" ) )
					{
						pSlot->SetJustPlaced( false );
						pSlot->RemoveItem();
						pSlot->PlaceItem( pItem );
						pItem->ActivateItem( false );
						return;
					}
				}
				else if ( pItem->GetSlotType().same_no_case( "scroll" ) &&
						  pSlot->GetSlotType().same_no_case( "spellbook" ) &&
						  pSlot->GetItemID() != 0 )
				{
					GoHandle hSlotItem( MakeGoid( pSlot->GetItemID() ) );
					if ( hSlotItem.IsValid() && hSlotItem->GetGui()->IsSpellBook() )
					{
						if ( hObject->IsSpell() )
						{
							if ( GridLearnSpell( hObject, hSlotItem ) )
							{
								UIItem * pItem = GetItemFromGO( hObject, false );
								pItem->ActivateItem( false );
								pSlot->SetCanEquip( false );
								pSlot->SetJustPlaced( true );
								return;
							}
						}
					}
				}
				else if ( pSlot->GetSlotType().same_no_case( pItem->GetSlotType() ) &&
						  pSlot->GetSlotType().same_no_case( "ring" ) && bUseRollover )
				{
					POINT pt;
					pt.x = gUIShell.GetMouseX();
					pt.y = gUIShell.GetMouseY();
					if ( (pSlot->GetRect().Contains( pt ) == false || !gUIShell.IsInterfaceVisible( "character" )) && (pSlot->GetItemID() == 0 || pSlot->GetName().same_no_case( "ring4" ) ) )
					{
						pSlot->SetJustPlaced( false );
						pSlot->RemoveItem();
						pSlot->PlaceItem( pItem );
						pItem->ActivateItem( false );
						return;
					}
				}
			}
		}
	}
}


void UIInventoryManager::ButtonEquipSpell( UIButton * pButton )
{
	UIWindowVec windows = gUIShell.ListWindowsOfGroup( pButton->GetGroup() );
	UIWindowVec::iterator i;
	for ( i = windows.begin(); i != windows.end(); ++i )
	{
		if ( (*i)->GetType() == UI_TYPE_ITEMSLOT )
		{
			UIItemVec items;
			gUIShell.FindActiveItems( items );
			GoHandle hMember( gUIPartyManager.GetSelectedCharacter() );
			if ( items.size() == 0  && !m_bLearnedSpell )
			{
				Goid spell = MakeGoid( ((UIItemSlot *)(*i))->GetItemID() );
				GoHandle hSpell( spell );
				if ( hSpell.IsValid() && hMember.IsValid() )
				{
					ButtonUnEquipSpell( pButton );
				}
			}
			else if ( items.size() == 1 )
			{
				UIItem * pItem = *(items.begin());
				if ( pItem )
				{
					GoHandle hObject( MakeGoid( pItem->GetItemID() ) );
					if ( hObject.IsValid() && hObject->IsSpell() )
					{
						Go * pSpellBook = hMember->GetInventory()->GetActiveSpellBook();
						if ( !pSpellBook )
						{
							return;
						}

						GopColl items;
						pSpellBook->GetInventory()->ListItems( IL_ALL_SPELLS, items );
						if ( items.size() >= GoInventory::MAX_SPELLS )
						{
							gpscreen( $MSG$ "This Spell Book is full. You must remove a spell to learn another spell or use another Spell Book.\n" );
							return;
						}
						else if ( hMember->GetInventory()->GetLocation( hObject ) != IL_MAIN )
						{
							return;
						}
						else if ( hMember->GetInventory()->LocationContainsTemplate( IL_ALL_SPELLS, hObject->GetTemplateName() ) )
						{
							return;
						}

						int index = pButton->GetIndex();
						index += (int)IL_ACTIVE_COUNT;

						hMember->GetInventory()->RSTransfer( hObject, pSpellBook, (eInventoryLocation)index, AO_USER );

						m_bLearnedSpell = true;
						pItem->ActivateItem( false );
					}
					else
					{
						return;
					}
				}
			}
		}
	}
}


void UIInventoryManager::ButtonUnEquipSpell( UIButton * pButton )
{
	UIItemVec items;
	gUIShell.FindActiveItems( items );
	if ( items.size() == 0 )
	{
		UIWindowVec windows = gUIShell.ListWindowsOfGroup( pButton->GetGroup() );
		UIWindowVec::iterator i;
		for ( i = windows.begin(); i != windows.end(); ++i )
		{
			if ( (*i)->GetType() == UI_TYPE_ITEMSLOT )
			{
				Goid spell = MakeGoid( ((UIItemSlot *)(*i))->GetItemID() );
				GoHandle hSpell( spell );
				GetItemFromGO( hSpell );
				gUIGame.SetBackendMode( BMODE_DRAG_INVENTORY );

				gUIShell.ShowGroup( pButton->GetGroup(), false );

				GoHandle hMember( gUIPartyManager.GetSelectedCharacter() );

				if ( hMember->GetInventory()->GetSelectedItem() == hSpell )
				{
					int index = 0;
					gUIPartyManager.GetCharacterFromGoid( hMember->GetGoid(), index );
					ActivateMeleeWeapon( index );
				}

				Go * pSpellBook = hMember->GetInventory()->GetActiveSpellBook();
				if ( pSpellBook )
				{
					pSpellBook->GetInventory()->RSTransfer( hSpell, hMember, IL_MAIN, AO_USER );
				}
			}
		}
	}
}


void UIInventoryManager::SetGUIRolloverGOID( Goid object )
{
	m_gui_rollover_goid = object;

	if ( m_gui_rollover_goid == GOID_INVALID )
	{
		UIWindowVec gridboxes = gUIShell.ListWindowsOfType( UI_TYPE_GRIDBOX );
		UIWindowVec::iterator k;
		for ( k = gridboxes.begin(); k != gridboxes.end(); ++k )
		{
			((UIGridbox *)(*k))->GetTextBox()->SetVisible( false );
		}
	}
}


void UIInventoryManager::RSReqInitiateTrade( Go * pSource, Go * pDest, Go * pItem )
{
	FUBI_RPC_THIS_CALL( RSReqInitiateTrade, RPC_TO_SERVER );

	// Early out
	if ( !pSource || !pDest || !pSource->GetPlayer() || !pDest->GetPlayer() ||
		( pSource->GetPlayer()->GetHero() != pSource->GetGoid() ) || ( pDest->GetPlayer()->GetHero() != pDest->GetGoid() ) )
	{
		return;
	}

	if( pSource->GetPlayer()->GetIsTrading() || pDest->GetPlayer()->GetIsTrading() )
	{
		if ( pDest->GetPlayer()->GetIsTrading() )
		{
			RCPlayerBusyDialog( pSource->GetPlayer()->GetMachineId() );
		}	

		return;
	}

	// Can't trade with anyone outside our frustum or if they are dead.
	if ( !pDest->IsInActiveWorldFrustum() ||  !::IsAlive( pSource->GetAspect()->GetLifeState() ) || !::IsAlive( pDest->GetAspect()->GetLifeState() ) || 
	      pSource->GetLifeState() == LS_ALIVE_UNCONSCIOUS || pDest->GetLifeState() == LS_ALIVE_UNCONSCIOUS )
	{
		return;
	}

	if ( pSource->IsGhost() || pDest->IsGhost() )
	{
		return;
	}

	pSource->GetPlayer()->SSetTradeGoldAmount( 0 );
	pDest->GetPlayer()->SSetTradeGoldAmount( 0 );

	if (pItem->HasGold())
	{
		pSource->GetPlayer()->SSetTradeGoldAmount( pItem->GetAspect()->GetGoldValue() );
	}

	RCAckInitiateTrade( pDest, pSource, pItem, pSource->GetPlayer()->GetMachineId(), pDest->GetPlayer()->GetMachineId() );
	RCAckInitiateTrade( pSource, pDest, pItem, pSource->GetPlayer()->GetMachineId(), pSource->GetPlayer()->GetMachineId() );	// brings up trade GUI

	pDest->GetPlayer()->SetIsTrading( true );
	pSource->GetPlayer()->SetIsTrading( true );

	return;
}


void UIInventoryManager::RCPlayerBusyDialog( DWORD machineID )
{
	FUBI_RPC_THIS_CALL( RCPlayerBusyDialog, machineID );

	if ( !gUIShell.IsInterfaceVisible( "multiplayer_trade" ) )
	{
		gUIGame.ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "This player is currently busy." ) );
	}
}


void UIInventoryManager::RCAckInitiateTrade( Go * pSource, Go * pDest, Go * pItem, DWORD initiateID, DWORD machineID )
{
	FUBI_RPC_THIS_CALL( RCAckInitiateTrade, machineID );
	
	if ( !pSource || !pDest )
	{
		return;
	}

	// Show the interface
	gUIShell.ShowInterface( "multiplayer_trade" );
	gUIShell.ShowGroup( "multi_inventory_human_1_gold", true, false, "multi_inventory" );
	

	// Set the destination Go name
	UIText * pText = (UIText *)gUIShell.FindUIWindow( "dest_name_text" );
	pText->SetText( pDest->GetCommon()->GetScreenName() );	

	pText = (UIText *)gUIShell.FindUIWindow( "text_dest_offer" );
	gpwstring sDest;
	sDest.assignf( gpwtranslate( $MSG$ "%s is offering in exchange:" ), pDest->GetCommon()->GetScreenName().c_str() );
	if ( pText )
	{
		pText->SetText( sDest );
	}

	sDest.assignf( gpwtranslate( $MSG$ "%s accepts your offer" ), pDest->GetCommon()->GetScreenName().c_str() );
	pText = (UIText *)gUIShell.FindUIWindow( "dest_accept_text", "multiplayer_trade" );
	if ( pText )
	{
		pText->SetText( sDest );
	}

	UIWindow * pWindow = gUIShell.FindUIWindow( "window_accept_check", "multiplayer_trade" );
	if ( pWindow )
	{
		pWindow->SetVisible( false );
	}
	
	// Reset the gold values to zero
	pText = (UIText *)gUIShell.FindUIWindow( "dest_gold_text" );
	pText->SetText( gpwtranslate( $MSG$ "Gold : 0" ) );
	pText = (UIText *)gUIShell.FindUIWindow( "source_gold_text" );
	pText->SetText( gpwtranslate( $MSG$ "Gold : 0" ) );

	UIButton * pZeroGold = (UIButton *)gUIShell.FindUIWindow( "button_zero_gold" );
	if ( pZeroGold )
	{
		pZeroGold->DisableButton();
	}

	gUIPartyManager.ActivateSingleInventory( pSource );

	gSiegeEngine.GetCompass().SetVisible( false );
	gUIShell.HideInterface( "compass_hotpoints" );

	UIGridbox * pDestGrid = (UIGridbox *)gUIShell.FindUIWindow( "tdest_gridbox" );
	pDestGrid->SetItemDetect( true );
	pDestGrid->ResetGrid();

	UIGridbox * pSourceGrid = (UIGridbox *)gUIShell.FindUIWindow( "tsource_gridbox" );
	pSourceGrid->SetItemDetect( false );
	pSourceGrid->ResetGrid();
	
	UIRadioButton * pSourceButton = (UIRadioButton *)gUIShell.FindUIWindow( "tsource_button_accept" );

	pSourceButton->SetCheck( false );
	
	m_bTradeDestAccept		= false;
	m_bTradeSourceAccept	= false;
	m_SourceTrade			= pSource->GetGoid();
	m_DestTrade				= pDest->GetGoid();

	if ( initiateID == machineID )
	{
		UIGridbox * pGrid = (UIGridbox *)gUIShell.FindUIWindow( "tsource_gridbox", "multiplayer_trade" );
		if ( pGrid )
		{	
			if (pItem->HasGold())
			{
				gpwstring sGold;
				DWORD gold = pItem->GetAspect()->GetGoldValue();
				sGold.assignf( gpwtranslate( $MSG$ "Gold : %d" ), gold );				
				pText = (UIText *)gUIShell.FindUIWindow( "source_gold_text" );
				pText->SetText( sGold );
				if ( pZeroGold )
				{
					pZeroGold->EnableButton();
				}
			}
			else
			{
				pGrid->AutoItemPlacement( pSource->GetInventory()->BuildUiName( pItem ), true );				
				ProcessAddToDestGridbox();
			}

			UIItem * pUIItem = GetItemFromGO( pItem, false );
			pUIItem->ActivateItem( false );
			pGrid->SetItemDetect( true );
		}
	}
	else
	{
		UIGridbox * pGrid = (UIGridbox *)gUIShell.FindUIWindow( "tsource_gridbox", "multiplayer_trade" );
		if ( pGrid )
		{	
			if (pItem->HasGold())
			{
				gpwstring sGold;
				DWORD gold = pItem->GetAspect()->GetGoldValue();
				sGold.assignf( gpwtranslate( $MSG$ "Gold : %d" ), gold );				
				pText = (UIText *)gUIShell.FindUIWindow( "dest_gold_text" );
				pText->SetText( sGold );
				if ( pZeroGold )
				{
					pZeroGold->EnableButton();
				}
			}
			pGrid->SetItemDetect( true );
		}
	}	
	
	if ( GetSourceTrade() )
	{
		GetSourceTrade()->GetInventory()->GetGridbox()->SetItemDetect( true );
	}
}


void UIInventoryManager::RSTradeAccept( bool bAccept, Player * sourcePlayer, Player * destPlayer )
{
	FUBI_RPC_THIS_CALL( RSTradeAccept, RPC_TO_SERVER );

	RCSetTradeSourceAccept( bAccept, sourcePlayer->GetMachineId() );
	RCSetTradeDestAccept( bAccept, destPlayer->GetMachineId() );
}


void UIInventoryManager::RCSetTradeDestAccept( bool bAccept, DWORD machineID )
{
	FUBI_RPC_THIS_CALL( RCSetTradeDestAccept, machineID );

	if ( !GetSourceTrade() || !GetDestTrade() )
	{
		return;
	}

	m_bTradeDestAccept = bAccept;	
	
	UIRadioButton * pSourceButton = (UIRadioButton *)gUIShell.FindUIWindow( "tsource_button_accept" );
	if ( pSourceButton->GetCheck() && bAccept == false )
	{
		pSourceButton->SetCheck( false );
	}
	
	UIWindow * pWindow = gUIShell.FindUIWindow( "window_accept_check", "multiplayer_trade" );
	if ( pWindow )
	{
		pWindow->SetVisible( bAccept );
	}
}


void UIInventoryManager::RCSetTradeSourceAccept( bool bAccept, DWORD machineID )
{
	FUBI_RPC_THIS_CALL( RCSetTradeSourceAccept, machineID );

	if ( !GetSourceTrade() || !GetDestTrade() )
	{
		return;
	}

	m_bTradeSourceAccept = bAccept;

	if ( m_bTradeDestAccept && m_bTradeSourceAccept )
	{
		RSAttemptTradeCompletion( GetSourceTrade(), GetDestTrade() );
	}
}


void UIInventoryManager::RSCancelTrade( Go * pSource, Go * pDest )
{
	FUBI_RPC_THIS_CALL( RSCancelTrade, RPC_TO_SERVER );

	if ( pSource && pSource->GetPlayer() )
	{
		pSource->GetPlayer()->SetIsTrading( false );
		RCCancelTrade( pSource->GetPlayer()->GetMachineId() );		
		pSource->GetPlayer()->SSetTradeGoldAmount( 0 );
	}
	
	if ( pDest && pDest->GetPlayer() )
	{
		pDest->GetPlayer()->SetIsTrading( false );
		RCCancelTrade( pDest->GetPlayer()->GetMachineId() );		
		pDest->GetPlayer()->SSetTradeGoldAmount( 0 );
	}

	return;
}


void UIInventoryManager::RCCancelTrade( DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCCancelTrade, machineId );

	if ( !GetSourceTrade() )
	{
		return;
	}

	if ( gUIShell.IsInterfaceVisible( "multiplayer_trade" ) )
	{
		gUIShell.HideInterface( "multiplayer_trade" );		

		m_bCompletingTrade = true;
		gUIPartyManager.CloseAllCharacterMenus();
		m_bCompletingTrade = false;
	}

	gUIShell.ShowGroup( "multi_inventory_human_1_gold", false, false, "multi_inventory" );
	
	UIRadioButton * pSourceButton = (UIRadioButton *)gUIShell.FindUIWindow( "tsource_button_accept" );

	pSourceButton->SetCheck( false );
	
	if ( GetSourceTrade() )
	{
		UIGridbox * pSourceGrid = (UIGridbox *)gUIShell.FindUIWindow( "tsource_gridbox" );		

		// Give the destination items to the source
		GridItemColl gridItems = pSourceGrid->GetGridItems();
		GridItemColl::iterator i;
		for ( i = gridItems.begin(); i != gridItems.end(); ++i )
		{
			GoHandle hItem( MakeGoid((*i).itemID) );
			if ( hItem.IsValid() )
			{
				if ( GetSourceTrade()->GetInventory()->CanSetLocation( hItem, IL_MAIN, true ) )
				{
					GetSourceTrade()->GetInventory()->RSSetLocation( hItem, IL_MAIN, true );
				}
				else
				{
					// We can't autoplace it so we're going to add it and then drop it to the ground
					GetSourceTrade()->GetInventory()->RSSetLocation( hItem, IL_MAIN, false );
					GetSourceTrade()->GetInventory()->RSRemove( hItem, true );
				}
			}
		}		
	}

	if ( GetSourceTrade() )
	{
		GetSourceTrade()->GetInventory()->GetGridbox()->SetItemDetect( true );
	}

	UIGridbox * pSourceGrid = (UIGridbox *)gUIShell.FindUIWindow( "tsource_gridbox" );
	UIGridbox * pDestGrid = (UIGridbox *)gUIShell.FindUIWindow( "tdest_gridbox" );
	pSourceGrid->ResetGrid();
	pDestGrid->ResetGrid();

	m_bTradeDestAccept		= false;
	m_bTradeSourceAccept	= false;
	m_DestTrade				= GOID_INVALID;
	m_SourceTrade			= GOID_INVALID;

	gpscreen( $MSG$ "The trade has been cancelled" );
}


void UIInventoryManager::RSAttemptTradeCompletion( Go * pSource, Go * pDest )
{
	FUBI_RPC_THIS_CALL( RSAttemptTradeCompletion, RPC_TO_SERVER );

	pSource->GetPlayer()->SetTradeState( Player::TRADE_PROCESSING );
	pSource->GetPlayer()->SetTradeSource( pSource->GetGoid() );
	pSource->GetPlayer()->SetTradeDest( pDest->GetGoid() );

	pDest->GetPlayer()->SetTradeState( Player::TRADE_PROCESSING );
	pDest->GetPlayer()->SetTradeSource( pDest->GetGoid() );
	pDest->GetPlayer()->SetTradeDest( pSource->GetGoid() );	

	RCAttemptTradeCompletion( pSource, pDest );
	RCAttemptTradeCompletion( pDest, pSource );
}


void UIInventoryManager::RCAttemptTradeCompletion( Go * pSource, Go * /* pDest */ )
{
	FUBI_RPC_THIS_CALL( RCAttemptTradeCompletion, pSource->GetPlayer()->GetMachineId() );

	if ( !GetSourceTrade() )
	{		
		return;
	}

	UIGridbox * pDestGrid = (UIGridbox *)gUIShell.FindUIWindow( "tdest_gridbox" );

	GridItemColl gridItems = pDestGrid->GetGridItems();
	GridItemColl::iterator i;	

	for ( i = gridItems.begin(); i != gridItems.end(); ++i )
	{
		GoHandle hItem( MakeGoid((*i).itemID) );
		if ( hItem.IsValid() )
		{			
			if ( !GetSourceTrade()->GetInventory()->GetGridbox()->AutoItemPlacement( GetSourceTrade()->GetInventory()->BuildUiName( hItem ), true, MakeInt( hItem->GetGoid() ) ) )
			{
				GridItemColl::iterator j;
				for ( j = gridItems.begin(); j != gridItems.end(); ++j )
				{
					GetSourceTrade()->GetInventory()->GetGridbox()->RemoveID( (*j).itemID );
				}

				RSSetTradeInvalid( GetSourceTrade() );
				return;
			}
		}
	}	

	GridItemColl::iterator j;
	for ( j = gridItems.begin(); j != gridItems.end(); ++j )
	{
		GetSourceTrade()->GetInventory()->GetGridbox()->RemoveID( (*j).itemID );
	}

	RSSetTradeValid( GetSourceTrade() );
}


void UIInventoryManager::RSSetTradeValid( Go * pSource )
{
	FUBI_RPC_THIS_CALL( RSSetTradeValid, RPC_TO_SERVER );

	pSource->GetPlayer()->SetTradeState( Player::TRADE_VALID );
}


void UIInventoryManager::RSSetTradeInvalid( Go * pSource )
{
	FUBI_RPC_THIS_CALL( RSSetTradeInvalid, RPC_TO_SERVER );

	pSource->GetPlayer()->SetTradeState( Player::TRADE_INVALID );
}


void UIInventoryManager::RSTradeFailed( Go * pSource, Go * pDest )
{
	FUBI_RPC_THIS_CALL( RSTradeFailed, RPC_TO_SERVER );

	RCTradeFailed( pSource->GetPlayer()->GetMachineId() );
	RCTradeFailed( pDest->GetPlayer()->GetMachineId() );
}


void UIInventoryManager::RCTradeFailed( DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCTradeFailed, machineId );

	gpscreen( $MSG$ "There was not enough room in inventory for all items to be traded." );
}


void UIInventoryManager::RSTradeZeroGold( Go * pSource, Go * pDest )
{
	FUBI_RPC_THIS_CALL( RSTradeZeroGold, RPC_TO_SERVER );

	pSource->GetPlayer()->SSetTradeGoldAmount( 0 );

	if ( pSource )
	{
		RCTradeZeroGoldSource( pSource->GetPlayer()->GetMachineId() );
	}

	if ( pDest )
	{
		RCTradeZeroGoldDest( pDest->GetPlayer()->GetMachineId() );
	}
}


void UIInventoryManager::RCTradeZeroGoldSource( DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCTradeZeroGoldSource, machineId );

	gpwstring sGold;
	sGold.assign( gpwtranslate( $MSG$ "Gold : 0" ) );
	UIText * pText = (UIText *)gUIShell.FindUIWindow( "source_gold_text" );
	pText->SetText( sGold );
	UIButton * pZeroGold = (UIButton *)gUIShell.FindUIWindow( "button_zero_gold" );
	if ( pZeroGold )
	{
		pZeroGold->DisableButton();
	}
}


void UIInventoryManager::RCTradeZeroGoldDest( DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCTradeZeroGoldDest, machineId );

	gpwstring sGold;
	sGold.assign( gpwtranslate( $MSG$ "Gold : 0" ) );
	UIText * pText = (UIText *)gUIShell.FindUIWindow( "dest_gold_text" );
	pText->SetText( sGold );
}


void UIInventoryManager::RSCompleteTrade( Go * pSource, Go * pDest )
{
	FUBI_RPC_THIS_CALL( RSCompleteTrade, RPC_TO_SERVER );
	if ( !pSource || !pDest )
	{
		return;
	}

	// unless this check occurs this call be executed twice , once for each client
	if ( ( pSource->GetPlayer()->GetIsTrading() == false ) || ( pDest->GetPlayer()->GetIsTrading() == false ) )
	{
		return;
	}

	int destGold = 0;
	int sourceGold = 0;

	sourceGold = pSource->GetInventory()->GetGold()
				- pSource->GetPlayer()->GetTradeGoldAmount()
				+ pDest->GetPlayer()->GetTradeGoldAmount();

	destGold = pDest->GetInventory()->GetGold()
				- pDest->GetPlayer()->GetTradeGoldAmount()
				+ pSource->GetPlayer()->GetTradeGoldAmount();

	pSource->GetInventory()->SSetGold(sourceGold);
	pDest->GetInventory()->SSetGold(destGold);

	pSource->GetPlayer()->SSetTradeGoldAmount( 0 );
	pDest->GetPlayer()->SSetTradeGoldAmount( 0 );

	// Complete trade if items in grid
	RCCompleteTrade( pSource, pDest );
	pSource->GetPlayer()->SetIsTrading( false );

	RCCompleteTrade( pDest, pSource );
	pDest->GetPlayer()->SetIsTrading( false );

	return;
}


void UIInventoryManager::RCCompleteTrade( Go * pSource, Go * pDest )
{
	FUBI_RPC_THIS_CALL( RCCompleteTrade, pSource->GetPlayer()->GetMachineId() );

	if ( !pSource || !pDest )
	{
		return;
	}

	if ( !GetSourceTrade() || !GetDestTrade() )
	{
		return;
	}

	UIGridbox * pDestGrid = (UIGridbox *)gUIShell.FindUIWindow( "tdest_gridbox" );

	bool bError = false;

	// Give the destination items to the source
	GridItemColl gridItems = pDestGrid->GetGridItems();
	GridItemColl::iterator i;
	
	for ( i = gridItems.begin(); i != gridItems.end(); ++i )
	{
		GoHandle hItem( MakeGoid((*i).itemID) );
		if ( hItem.IsValid() )
		{
			if ( GetSourceTrade()->GetInventory()->CanAdd( hItem, IL_MAIN, true ) )
			{
				GetDestTrade()->GetInventory()->RSTransfer( hItem, GetSourceTrade(), IL_MAIN, AO_USER, true );
			}
			else
			{
				gpscreen( $MSG$ "There was not enough room in inventory for all items to be traded." );
				bError = true;

				// They can't add it so let's put it back into our own
				GetDestTrade()->GetInventory()->RSRemove( hItem, false );

				// Can we autoplace it?
				if ( GetDestTrade()->GetInventory()->CanAdd( hItem, IL_MAIN, true ) )
				{
					GetDestTrade()->GetInventory()->RSAdd( hItem, IL_MAIN, AO_USER, true );
				}
				else
				{
					// We can't autoplace it so we're going to add it and then drop it to the ground
					GetDestTrade()->GetInventory()->RSAdd( hItem, IL_MAIN, AO_USER, false );
					GetDestTrade()->GetInventory()->RSRemove( hItem, true );
				}
			}
		}
	}

	if ( GetSourceTrade() )
	{
		GetSourceTrade()->GetInventory()->GetGridbox()->SetItemDetect( true );
	}

	UIGridbox * pSourceGrid = (UIGridbox *)gUIShell.FindUIWindow( "tsource_gridbox" );	
	pSourceGrid->ResetGrid();
	pDestGrid->ResetGrid();

	m_bTradeDestAccept		= false;
	m_bTradeSourceAccept	= false;
	
	m_bCompletingTrade = true;
	gUIPartyManager.CloseAllCharacterMenus();
	m_bCompletingTrade = false;

	gUIShell.HideInterface( "multiplayer_trade" );
	gUIShell.ShowGroup( "multi_inventory_human_1_gold", false, false, "multi_inventory" );

	if ( !bError )
	{
		gpscreen( $MSG$ "The trade has been completed" );		
	}

	m_DestTrade			= GOID_INVALID;
	m_SourceTrade		= GOID_INVALID;
}


void UIInventoryManager::ProcessAddToDestGridbox()
{
	if ( !GetSourceTrade() || !GetDestTrade() )
	{
		return;
	}
	
	// When the source places items into their gridbox, take those items and place them into Dest's destination
	// gridbox and update trade gold amount
	UIGridbox * pSourceGrid = (UIGridbox *)gUIShell.FindUIWindow( "tsource_gridbox" );

	//Update the source grid box and server with new gold value
	GoHandle hItem(MakeGoid(pSourceGrid->GetLastItem().itemID));
	if (hItem.IsValid() && hItem->HasGold())
	{
		gpwstring sGold;
		int gold = hItem->GetAspect()->GetGoldValue();
		sGold.assignf( gpwtranslate( $MSG$ "Gold : %d" ), GetSourceTrade()->GetPlayer()->GetTradeGoldAmount()+gold );		
		UIText * pText = (UIText *)gUIShell.FindUIWindow( "source_gold_text" );
		pText->SetText( sGold );
		UIButton * pZeroGold = (UIButton *)gUIShell.FindUIWindow( "button_zero_gold" );
		if ( pZeroGold )
		{
			pZeroGold->EnableButton();
		}

		GetSourceTrade()->GetPlayer()->RSSetTradeGoldAmount(GetSourceTrade()->GetPlayer()->GetTradeGoldAmount()+gold);
	}

	if ( GetDestTrade()->GetPlayer() == NULL )
	{
		return;
	}

	GridItem item = pSourceGrid->GetLastItem();
	item.rect.top		-= gUIMenuManager.GetTradeShift();
	item.rect.bottom	-= gUIMenuManager.GetTradeShift();

	RSAddToDestGridbox( item, GetDestTrade()->GetGoid(), GetSourceTrade()->GetGoid() );

	return;
}


void UIInventoryManager::RSAddToDestGridbox( GridItem item, Goid destTrade, Goid sourceTrade )
{
	FUBI_RPC_THIS_CALL( RSAddToDestGridbox, RPC_TO_SERVER );

	GoHandle hItem( MakeGoid(item.itemID) );
	GoHandle hDest( destTrade );
	GoHandle hSource( sourceTrade );
	if ( !hItem  )
	{
		gperrorf( ( "Trade Error: Tried to add an item, but it is invalid ( Goid: 0x%x )", item.itemID ) );
		return;
	}

	if ( !hDest )
	{
		gperrorf( ( "Trade Error: Tried to add an item to a destination gridbox but the destination trader is invalid ( Goid: 0x%x )", MakeInt(destTrade) ) );
		return;
	}

	if ( !hSource )
	{
		gperrorf( ( "Trade Error: Tried to add an item to a destination gridbox but the source trader is invalid ( Goid: 0x%x )", MakeInt(sourceTrade) ) );
		return;
	}

	if ( hItem && !hItem->HasParent() )
	{
		gperrorf( ( "Trade Error: Tried to add %s, Goid: 0x%x to the destination gridbox, but the item is invalid!", hItem->GetTemplateName(), MakeInt(hItem->GetGoid()) ) );		
		RSCancelTrade( hSource, hDest );
		return;
	}

	if ( !hDest->GetPlayer() )
	{
		gperrorf( ( "Trade Error: The destination player ( Goid: 0x%x ) is invalid", MakeInt(hDest->GetGoid()) ) );		
		RSCancelTrade( hSource, hDest );
		return;
	}
	if ( !hItem->GetParent()->GetPlayer() )
	{
		gperrorf( ( "Trade Error: The item owner player ( Goid: 0x%x ) is invalid", MakeInt(hItem->GetParent()->GetGoid()) ) );		
		RSCancelTrade( hSource, hDest );
		return;
	}

	RSTradeAccept( false, hItem->GetParent()->GetPlayer(), hDest->GetPlayer() );
	RSTradeAccept( false, hDest->GetPlayer(), hItem->GetParent()->GetPlayer() );

	RCAddToDestGridbox( item, hDest->GetPlayer()->GetMachineId() );
}


void UIInventoryManager::RCAddToDestGridbox( GridItem item, DWORD machineID )
{
	FUBI_RPC_THIS_CALL( RCAddToDestGridbox, machineID );

	if ( !GetSourceTrade() || !GetDestTrade() )
	{
		return;
	}

	GoHandle hItem( MakeGoid(item.itemID) );
	if ( hItem.IsValid() )
	{
		if (hItem->HasGold())
		{
			gpwstring sGold;
			DWORD gold = hItem->GetAspect()->GetGoldValue();
			sGold.assignf( gpwtranslate( $MSG$ "Gold : %d" ), gold );			
			UIText * pText = (UIText *)gUIShell.FindUIWindow( "dest_gold_text" );
			pText->SetText( sGold );
		}
		else
		{			
			UIGridbox * pDestGrid = (UIGridbox *)gUIShell.FindUIWindow( "tdest_gridbox" );
			UIGridbox * pSourceGrid = (UIGridbox *)gUIShell.FindUIWindow( "tsource_gridbox" );

			int width = item.rect.right - item.rect.left;
			int height = item.rect.bottom - item.rect.top;
			item.rect.left -= pSourceGrid->GetRect().left - pDestGrid->GetRect().left;
			item.rect.right = item.rect.left + width;

			item.rect.top -= pSourceGrid->GetRect().top - pDestGrid->GetRect().top;
			item.rect.bottom = item.rect.top + height;

			item.rect.top += gUIMenuManager.GetTradeShift();
			item.rect.bottom += gUIMenuManager.GetTradeShift();
			
			GetItemFromGO( hItem, false, false );
			pDestGrid->SetDeactivateOverlapped( true );
			pDestGrid->AddGridItem( item );
			pDestGrid->SetDeactivateOverlapped( false );
			CalculateGridHighlights( pDestGrid );
		}
	}
}


void UIInventoryManager::RSRemoveFromDestGridbox( int item, Player * destPlayer )
{
	FUBI_RPC_THIS_CALL( RSRemoveFromDestGridbox, RPC_TO_SERVER );

	GoHandle hItem( MakeGoid(item) );
	RSTradeAccept( false, hItem->GetParent()->GetPlayer(), destPlayer );
	RSTradeAccept( false, destPlayer, hItem->GetParent()->GetPlayer() );

	RCRemoveFromDestGridbox( item, destPlayer );
}


void UIInventoryManager::RCRemoveFromDestGridbox( int item, Player * destPlayer )
{
	FUBI_RPC_THIS_CALL( RCRemoveFromDestGridbox, destPlayer->GetMachineId() );

	if ( !GetSourceTrade() || !GetDestTrade() )
	{
		return;
	}

	// When the source places items into their gridbox, take those items and place them into Dest's destination
	// gridbox
	UIGridbox * pDestGrid = (UIGridbox *)gUIShell.FindUIWindow( "tdest_gridbox" );
	pDestGrid->RemoveID( item );

	return;
}


void UIInventoryManager::GoldTransfer( Goid transferer )
{
	GoHandle hMember( transferer );
	if ( hMember.IsValid() )
	{
		UIItemVec items;
		gUIShell.FindActiveItems( items );
		if ( items.size() == 0 )
		{
			if ( hMember->GetInventory()->GetGold() != 0 )
			{
				gUIMenuManager.SetGoldTradeDialogActive( true );
				UIEditBox * pEdit = (UIEditBox *)gUIShell.FindUIWindow( "edit_gold" );
				gpwstring sGold;
				sGold.assignf( L"%d", hMember->GetInventory()->GetGold()-hMember->GetPlayer()->GetTradeGoldAmount() );
				pEdit->SetText( sGold.c_str() );
				m_GoldTransferer = hMember->GetGoid();
			}
			else
			{
				gUIMenuManager.SetGoldInvalidDialogActive( true );
			}
		}
	}
}


void UIInventoryManager::GoldTransferStash()
{
	GoldTransfer( gServer.GetScreenPlayer()->GetStash() );
}


void UIInventoryManager::GoldTransferSelectedMember()
{
	GoldTransfer( gUIPartyManager.GetSelectedCharacter() );
}


void UIInventoryManager::ButtonGoldTransferInc()
{
	UIEditBox * pEdit = (UIEditBox *)gUIShell.FindUIWindow( "edit_gold" );
	gpwstring sText = pEdit->GetText();
	int gold = 0;
	if ( !sText.empty() )
	{
		stringtool::Get( sText, gold );
	}

	int max_gold = 0;
	GoHandle hMember( m_GoldTransferer );
	if ( hMember.IsValid() )
	{
		max_gold = hMember->GetInventory()->GetGold() - hMember->GetPlayer()->GetTradeGoldAmount();
	}
	if (( gold < max_gold ) && ( gold <= gContentDb.GetMaxPartyGold() ))
	{
		gold++;
	}
	sText.assignf( L"%d", gold );
	pEdit->SetText( sText );
}


void UIInventoryManager::ButtonGoldTransferDec()
{
	UIEditBox * pEdit = (UIEditBox *)gUIShell.FindUIWindow( "edit_gold" );
	gpwstring sText = pEdit->GetText();
	unsigned int gold = 0;

	if ( !sText.empty() )
	{
		stringtool::Get( sText, gold );
	}
	if ( gold > 0 )
	{
		gold--;
	}
	if ( gold <= 0 )
	{
		gold = 0;
	}

	sText.assignf( L"%d", gold );
	pEdit->SetText( sText );
}


void UIInventoryManager::ButtonGoldTransferComplete()
{
	gUIMenuManager.SetGoldTradeDialogActive( false );
	UIEditBox * pEdit = (UIEditBox *)gUIShell.FindUIWindow( "edit_gold" );
	gpstring sText = ToAnsi( pEdit->GetText() );
	int gold = 0;
	if ( sText.empty() )
	{
		return;
	}

	stringtool::Get( sText.c_str(), gold );
	if ( gold != 0 )
	{
		GoHandle hMember( m_GoldTransferer );
		if ( hMember.IsValid() )
		{
			RSGoldTransferComplete( hMember, gold );
		}
	}
	pEdit->SetText( L"" );
}


void UIInventoryManager::RSGoldTransferComplete( Go * pMember, int gold )
{
	FUBI_RPC_THIS_CALL( RSGoldTransferComplete, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	if ( gold > (pMember->GetInventory()->GetGold()-pMember->GetPlayer()->GetTradeGoldAmount()) && pMember->GetInventory()->GetGold() != 0 )
	{
		gold = pMember->GetInventory()->GetGold() - pMember->GetPlayer()->GetTradeGoldAmount();
	}


	if ( pMember->HasStash() )
	{
		GoHandle hPartyMember( pMember->GetStash()->GetStashMember() );
		if ( hPartyMember )
		{
			if ( (hPartyMember->GetInventory()->GetGold() + gold) > gContentDb.GetMaxPartyGold() )
			{
				gold -= (hPartyMember->GetInventory()->GetGold() + gold) - gContentDb.GetMaxPartyGold();
			}
		}
	}

	if ( gold != 0 )
	{
		GoHandle hItem( pMember->GetPlayer()->GetTradeGold() );

		if ( hItem.IsValid() )
		{
			hItem->GetAspect()->SSetGoldValue( gold, false );

			if ( pMember->HasStash() )
			{
				pMember->GetStash()->RSRemoveFromStash( pMember->GetStash()->GetStashMember(), hItem->GetGoid() );			
			}
		}	

		RCGoldTransferComplete( pMember );
	}
}


void UIInventoryManager::RCGoldTransferComplete( Go * pMember )
{
	FUBI_RPC_THIS_CALL( RCGoldTransferComplete, pMember->GetPlayer()->GetMachineId() );

	GoHandle hItem( pMember->GetPlayer()->GetTradeGold() );

	if ( hItem.IsValid() )
	{
		UIItem * pItem = GetItemFromGO( hItem, true );
		m_floating_item = hItem->GetGoid();
		pItem->SetItemID( MakeInt( m_floating_item ) );

		gUIGame.SetBackendMode( BMODE_DRAG_INVENTORY);		
	}	
}


Go * UIInventoryManager::GetSourceTrade()	
{ 
	GoHandle hSourceTrade( m_SourceTrade );
	if ( hSourceTrade.IsValid() )
	{
		return hSourceTrade.Get();
	}

	return NULL; 
}


Go * UIInventoryManager::GetDestTrade()
{ 
	GoHandle hDestTrade( m_DestTrade );
	if ( hDestTrade.IsValid() )
	{
		return hDestTrade.Get();
	}

	return NULL; 
}



void UIInventoryManager::ConvertPersistItems( UIGridbox * pGrid )
{
	GridItemColl convertedItems;

	PersistGridItemColl::iterator i;
	PersistGridItemColl & gridItems = pGrid->GetPersistGridItems();
	for ( i = gridItems.begin(); i != gridItems.end(); ++i )
	{
		GoHandle hObject( MakeGoid((*i).itemGoid) );
		if ( hObject.IsValid() )
		{
			GridItem item;
			item.alpha				= (*i).alpha;
			item.rect				= (*i).rect;
			item.secondary_percent	= (*i).secondary_percent;
			item.numItems			= 0;
			item.itemID				= (*i).itemGoid;

			int shiftX = (*i).xoffset - pGrid->GetRect().left;
			int shiftY = (*i).yoffset - pGrid->GetRect().top;
			item.rect.left = (*i).rect.left - shiftX;
			item.rect.right = item.rect.left + ((*i).rect.right-(*i).rect.left);
			item.rect.top = (*i).rect.top - shiftY;
			item.rect.bottom = item.rect.top + ((*i).rect.bottom-(*i).rect.top);

			convertedItems.push_back( item );
		}
	}

	gUIShell.SetGridPlace( true );
	pGrid->SetGridItems( convertedItems );
}


void UIInventoryManager::ListSpells( UIWindow * pWindow )
{
	//gUIMessenger.SendUIMessage( UIMessage( MSG_LBUTTONUP ), pWindow );
	UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_spells" );
	if ( pList && !gUIShell.IsInterfaceVisible( "character" ) )
	{
		pList->RemoveAllElements();
		pList->SetVisible( true );
		UIPartyMember * pMember = gUIPartyManager.GetSelectedCharacterFromIndex( pWindow->GetIndex() );
		if ( pMember )
		{
			GoHandle hMember( pMember->GetGoid() );
			if ( hMember.IsValid() )
			{
				gpstring sWindowName;
				sWindowName.assignf( "awp_radio_button_character_%d_slot_active", pWindow->GetIndex()+1 );
				if ( sWindowName.same_no_case( pWindow->GetName() ) && 
					 ( hMember->GetInventory()->GetSelectedLocation() != IL_ACTIVE_PRIMARY_SPELL &&
					   hMember->GetInventory()->GetSelectedLocation() != IL_ACTIVE_SECONDARY_SPELL ) )
				{
					pList->SetVisible( false );
					return;
				}

				Go * pSpellBook = hMember->GetInventory()->GetEquipped( ES_SPELLBOOK );
				if ( pSpellBook )
				{
					int start	= (int)IL_SPELL_1;
					int end		= (int)IL_SPELL_12 + 1;

					for( int is = start; is != end; ++is )
					{
						Go * spell = pSpellBook->GetInventory()->GetItem( (eInventoryLocation)is );
						if ( !spell )
						{
							continue;
						}

						DWORD color = pList->GetTextColor();
						if ( !spell->GetMagic()->CanOwnerCast() )
						{
							color = pList->GetInvalidColor();
						}
						else if ( spell == hMember->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL ) ||
							      spell == hMember->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL ) )
						{
							color = pList->GetActiveColor();
						}

						pList->AddElement( spell->GetCommon()->GetScreenName(), MakeInt( spell->GetGoid() ), color );
					}
				}
			}
		}

		unsigned int pos = pWindow->GetName().find_last_of( "_" );
		int index = 0;
		if ( pos != gpstring::npos )
		{
			gpstring sNum = pWindow->GetName().substr( pos+1, pWindow->GetName().size() );
			stringtool::Get( sNum.c_str(), index );
		}

		pList->AutoSize( pWindow, index );
	}
}


void UIInventoryManager::ListSelectSpell( UIWindow * pWindow )
{
	UIListbox * pList = (UIListbox *)pWindow;

	if ( pList )
	{
		Goid spell = MakeGoid( pList->GetSelectedTag() );
		GoHandle hSpell( spell );
		if ( hSpell.IsValid() )
		{
			GoHandle hMember( gUIGame.GetActorWhoCarriesObject( hSpell->GetParent()->GetGoid() ) );
			if ( hMember.IsValid() )
			{
				Go * pSpellBook = hMember->GetInventory()->GetEquipped( ES_SPELLBOOK );
				if ( pSpellBook )
				{
					// Primary Slot = 3
					if ( pList->GetIndex() == 3 )
					{
						eInventoryLocation ilNewSpell = pSpellBook->GetInventory()->GetLocation( hSpell );
						Go * pOldSpell = hMember->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL );
						if ( pOldSpell )
						{
							pSpellBook->GetInventory()->RSSetLocation( pOldSpell, ilNewSpell, false );
						}
						pSpellBook->GetInventory()->RSSetLocation( hSpell, IL_SPELL_1, false );

						int charIndex = 0;
						gUIPartyManager.GetCharacterFromGoid( hMember->GetGoid(), charIndex );
						if ( hSpell->GetMagic()->CanOwnerCast() )
						{
							ActivatePrimarySpell( charIndex );
						}
						else
						{
							ActivateMeleeWeapon( charIndex );
						}

						gpstring sCharacter;
						sCharacter.assignf( "awp_radio_button_character_%d_slot_3", charIndex+1 );
						{
							UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( sCharacter );
							if ( pRadio )
							{
								pRadio->ResetButtonStatus();
							}
						}
					}
					// Secondary Slot = 4
					else if ( pList->GetIndex() == 4 )
					{
						eInventoryLocation ilNewSpell = pSpellBook->GetInventory()->GetLocation( hSpell );
						Go * pOldSpell = hMember->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL );
						if ( pOldSpell )
						{
							pSpellBook->GetInventory()->RSSetLocation( pOldSpell, ilNewSpell, false );
						}
						pSpellBook->GetInventory()->RSSetLocation( hSpell, IL_SPELL_2, false );

						int charIndex = 0;
						gUIPartyManager.GetCharacterFromGoid( hMember->GetGoid(), charIndex );
						if ( hSpell->GetMagic()->CanOwnerCast() )
						{
							ActivateSecondarySpell( charIndex );
						}
						else
						{
							ActivateMeleeWeapon( charIndex );
						}

						gpstring sCharacter;
						sCharacter.assignf( "awp_radio_button_character_%d_slot_4", charIndex+1 );
						{
							UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( sCharacter );
							if ( pRadio )
							{
								pRadio->ResetButtonStatus();
							}
						}
					}

					else
					{
						int charIndex = 0;
						gUIPartyManager.GetCharacterFromGoid( hMember->GetGoid(), charIndex );

						if ( hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL )
						{
							eInventoryLocation ilNewSpell = pSpellBook->GetInventory()->GetLocation( hSpell );
							Go * pOldSpell = hMember->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL );
							if ( pOldSpell )
							{
								pSpellBook->GetInventory()->RSSetLocation( pOldSpell, ilNewSpell, false );
							}
							pSpellBook->GetInventory()->RSSetLocation( hSpell, IL_SPELL_1, false );

							if ( hSpell->GetMagic()->CanOwnerCast() )
							{
								ActivatePrimarySpell( charIndex );
							}
							else
							{
								ActivateMeleeWeapon( charIndex );
							}
						}
						else if ( hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL )
						{
							eInventoryLocation ilNewSpell = pSpellBook->GetInventory()->GetLocation( hSpell );
							Go * pOldSpell = hMember->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL );
							if ( pOldSpell )
							{
								pSpellBook->GetInventory()->RSSetLocation( pOldSpell, ilNewSpell, false );
							}
							pSpellBook->GetInventory()->RSSetLocation( hSpell, IL_SPELL_2, false );

							if ( hSpell->GetMagic()->CanOwnerCast() )
							{
								ActivateSecondarySpell( charIndex );
							}
							else
							{
								ActivateMeleeWeapon( charIndex );
							}
						}

						gpstring sCharacter;
						sCharacter.assignf( "awp_radio_button_character_%d_slot_active", charIndex+1 );
						{
							UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( sCharacter );
							if ( pRadio )
							{
								pRadio->ResetButtonStatus();
							}
						}
					}
				}

				hMember->GetInventory()->SetInventoryDirty( true );
			}
		}
		pList->SetVisible( false );
	}
}


void UIInventoryManager::HideSpellList()
{
	UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_spells" );
	if (( pList ) && !pList->GetRollover() )
	{
		pList->SetVisible( false );
	}
}


void UIInventoryManager::ListenerSpellBookRollover()
{
	GoHandle hMember( gUIPartyManager.GetSelectedPartyMember()->GetGoid() );
	if ( hMember.IsValid() )
	{
		Go * pSpellBook = hMember->GetInventory()->GetActiveSpellBook();
		if ( !pSpellBook )
		{
			pSpellBook = hMember->GetInventory()->GetEquipped( ES_SPELLBOOK );
		}

		if ( pSpellBook )
		{

			UIItemVec items;
			gUIShell.FindActiveItems( items );
			if ( items.size() != 0 )
			{
				UIItem * pItem = *(items.begin());
				Goid first = MakeGoid( pItem->GetItemID() );
				GoHandle hItem( first );
				if ( hItem.IsValid() && !(hItem->HasMagic() && hItem->GetMagic()->GetIsOneUse()) )
				{
					int start	= (int)IL_SPELL_1;
					int end		= (int)IL_SPELL_12 + 1;

					for( int is = start; is != end; ++is )
					{
						gpstring sTemp;
						gpwstring sData;

						int i = is - (int)IL_ACTIVE_COUNT;
						sTemp.assignf( "spell_slot_%d", i + 1 );
						UIInfoSlot * pSlot = (UIInfoSlot *)gUIShell.FindUIWindow( sTemp, "spell" );

						Go * spell = pSpellBook->GetInventory()->GetItem( (eInventoryLocation)is );

						if ( spell )
						{
							if ( gpstring( spell->GetTemplateName() ).same_no_case( hItem->GetTemplateName() ) )
							{
								pSlot->SetHighlight( true );
							}
						}
					}
				}
			}
		}
	}
}


void UIInventoryManager::ListenerSpellBookRolloff()
{
	int start	= (int)IL_SPELL_1;
	int end		= (int)IL_SPELL_12 + 1;

	for( int is = start; is != end; ++is )
	{
		gpstring sTemp;
		gpwstring sData;

		int i = is - (int)IL_ACTIVE_COUNT;
		sTemp.assignf( "spell_slot_%d", i + 1 );
		UIInfoSlot * pSlot = (UIInfoSlot *)gUIShell.FindUIWindow( sTemp, "spell" );

		pSlot->SetHighlight( false );
	}
}


void UIInventoryManager::SActivateStash( Goid member, Goid stashActivator )
{
	CHECK_SERVER_ONLY;
	GoHandle hMember( member );
	if ( hMember && hMember->GetPlayer() )
	{
		GoHandle hStash( hMember->GetPlayer()->GetStash() );
		if ( hStash )
		{
			RCActivateStash( member, stashActivator, hMember->GetPlayer()->GetMachineId() );
		}
	}
}


void UIInventoryManager::RCActivateStash( Goid member, Goid stashActivator, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCActivateStash, machineId );	
	ActivateStash( member, stashActivator );	
}

void UIInventoryManager::ActivateStash( Goid member, Goid stashActivator )
{
	// Let's make sure all of our parameters are valid before opening the stash
	GoHandle hMember( member );	
	if ( hMember && hMember->GetPlayer() )
	{
		GoHandle hStash( hMember->GetPlayer()->GetStash() );		
		if ( hStash )
		{	
			if ( GetStashActive() )
			{
				CloseStash();
			}		
			
			gUIPartyManager.CloseAllCharacterMenus();	

			// Let's set up the character interface
			{	
				int index = 0;
				gUIShell.ShiftInterface( "character", -22, 0 );
				gUIShell.ShiftInterface( "inventory", -22, 0 );		
				gUIPartyManager.GetCharacterFromGoid( member, index );
				gUIPartyManager.ConstructCharacterInterface( index, true, true );
				gUIPartyManager.SetSpellsActive( false );

				gpstring sCharacter;
				for ( int i = 0; i != MAX_PARTY_MEMBERS; ++i )
				{
					sCharacter.assignf( "character_%d_min", i+1 );
					gUI.GetGameGUI().ShowGroup( sCharacter, false );
				}
			}

			// Now let's disable the buttons that can't be used while utilizing the stash
			UIButton * pWindow = (UIButton *)gUIShell.FindUIWindow( "button_spellbook_hide", "character" );
			if ( pWindow )
			{
				pWindow->DisableButton();
			}
			pWindow = (UIButton *)gUIShell.FindUIWindow( "button_spellbook_show", "character" );
			if ( pWindow )
			{
				pWindow->DisableButton();
			}	
			
			hStash->GetStash()->ActivateStash( member, stashActivator );	
			SetStashActive( true );
			SetStash( hStash->GetGoid() );
			
			// Calculate any needed highlights
			UIGridbox * pGrid = hStash->GetInventory()->GetGridbox();
			if ( pGrid )
			{
				gUIInventoryManager.CalculateGridHighlights( pGrid );
			}			

			// Hide interfaces that may get in the way
			gSiegeEngine.GetCompass().SetVisible( false );
			gUIShell.HideInterface( "compass_hotpoints" );	
		}
	}
}


void UIInventoryManager::SwitchMemberStash( Goid member )
{
	GoHandle hStash( GetStash() );
	if ( hStash )
	{
		ActivateStash( member, hStash->GetStash()->GetStashActivator() );
	}
}


Goid UIInventoryManager::GetStashMember()
{
	if ( GetStashActive() )
	{
		GoHandle hStash( GetStash() );
		if ( hStash )
		{
			return hStash->GetStash()->GetStashMember();
		}
	}

	return GOID_INVALID;
}


Goid UIInventoryManager::GetStashActivator()
{
	if ( GetStashActive() )
	{
		GoHandle hStash( GetStash() );
		if ( hStash )
		{
			return hStash->GetStash()->GetStashActivator();
		}
	}

	return GOID_INVALID;
}


void UIInventoryManager::CloseStash()
{
	if ( GetStashActive() )
	{		
		GoHandle hStash( GetStash() );
		if ( hStash )
		{
			// Store cleanup
			hStash->GetStash()->CloseStash();
		}	

		// Disable Store
		SetStashActive( false );
		SetStash( GOID_INVALID );

		// Restore GUI back to normal
		gUIPartyManager.CloseAllCharacterMenus();	
		gSiegeEngine.GetCompass().SetVisible( true );
		gUIGame.SetBackendMode( BMODE_NONE );
		gUIShell.ShowInterface( "compass_hotpoints" );	
		gUIShell.ShiftInterface( "character", 22, 0 );
		gUIShell.ShiftInterface( "inventory", 22, 0 );					
	}
}


void UIInventoryManager::StashGuiCb( Goid member, Goid item )
{
	GoHandle hStash( GetStash() );
	GoHandle hMember( member );
	if ( hMember && hStash && hStash->HasStash() && GetStashActive() && !gUIShell.GetItemActive() )
	{		
		UIItem * pItem = gUIShell.GetItem( MakeInt( item ) );
		if ( pItem && !hMember->GetInventory()->GetGridbox()->ContainsID( MakeInt( item ) ) )
		{
			pItem->ActivateItem( true );
		}

		hStash->GetInventory()->GetGridbox()->RemoveID( MakeInt( item ) );

		hStash->GetInventory()->GetGridbox()->SetItemDetect( true );
	}
}


void UIInventoryManager::HandleStashSwitch( Goid newMember )
{
	GoHandle hStashActivator( GetStashActivator()	);
	GoHandle hMember		( newMember				);
	GoHandle hStash			( GetStash()			);

	if ( hStashActivator.IsValid() && hMember->IsInActiveScreenWorldFrustum() )
	{
		
		if ( !gAIQuery.IsInRange(	hStashActivator->GetPlacement()->GetPosition(), hMember->GetPlacement()->GetPosition(),
									hStash->GetStash()->GetActivateRange() ) )				
		{
			if ( GetStashActive() && !hMember->IsInActiveWorldFrustum() )
			{					
				CloseStash();										
			}
		}
		else if ( GetStashActive() )
		{													
			SwitchMemberStash( hMember->GetGoid() );
		}
	}
	else if ( GetStashActive() )
	{
		CloseStash();								
	}
}