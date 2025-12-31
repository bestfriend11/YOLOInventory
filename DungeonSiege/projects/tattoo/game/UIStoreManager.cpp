//////////////////////////////////////////////////////////////////////////////
//
// File     :  UIStoreManager.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "Precomp_Game.h"
#include "UIStoreManager.h"
#include "GoInventory.h"
#include "GoCore.h"
#include "GoAspect.h"
#include "GoMind.h"
#include "GoDb.h"
#include "GoStore.h"
#include "UIPartyManager.h"
#include "UIGame.h"
#include "UICommands.h"
#include "ui_shell.h"
#include "UIInventoryManager.h"
#include "GoParty.h"
#include "GoConversation.h"
#include "ui_listreport.h"
#include "GameAuditor.h"
#include "siege_compass.h"
#include "Server.h"
#include "Player.h"
#include "AiQuery.h"
#include "UIGameConsole.h"
#include "job.h"
#include "World.h"
#include "gps_manager.h"


UIStoreManager::UIStoreManager()
{
	Init();
	gWorld.RegisterPurchaseGuiCallback( makeFunctor( *this, &UIStoreManager::PurchaseGuiCb ) );
}


void UIStoreManager::Init()
{
	m_ActiveStore		= GOID_INVALID;
	m_ActiveStoreBuyer	= GOID_INVALID;
	m_bStoreActive		= false;
	m_AutoActivator		= GOID_INVALID;
	m_SelectedSellTrait	= QT_ITEM;
}


void UIStoreManager::Update()
{
	if ( m_bStoreActive && gUIShell.IsInterfaceVisible( "store" ) )
	{
		GoHandle hMember( m_ActiveStoreBuyer );
		GoHandle hStore( m_ActiveStore );
		if ( hMember.IsValid() )
		{		
			Job * pJob = hMember->GetMind()->GetFrontJob( JQ_ACTION );
			if ( (pJob && (pJob->GetTraits() & JT_OFFENSIVE)) || !::IsAlive( hMember->GetAspect()->GetLifeState() ) || 
				 !hStore || !hStore->IsInActiveScreenWorldFrustum() )
			{
				DeactivateStore();
			}
		}
	}
}


void UIStoreManager::CheckForStoreAndActivate( Goid object, bool bConversing ) 
{	
	GoHandle hObject( object );

	if ( hObject.IsValid() )
	{
		int index = 0;
		if (( hObject->HasActor() == false ) || !hObject->HasInventory() || gUIPartyManager.IsPlayerPartyMember( hObject->GetGoid(), index ) )
		{
			return;
		}
		
		if (( !bConversing ) && hObject->HasConversation() && hObject->GetConversation()->GetConversationsSize() != 0 )
		{
			return;
		}

		if ( hObject->HasStore() )
		{
			// This person has a store, let's see if a player is selected so we can activate the store
			GoidColl selectedparty;	
			GoidColl::iterator i;
			gUIPartyManager.GetPortraitOrderedParty( selectedparty );
			for ( i = selectedparty.begin(); i != selectedparty.end(); ++i )
			{			
				GoHandle hMember( *i );
				if ( hMember->HasActor() && hMember->IsSelected() )
				{
					for ( int o = 0; o < MAX_PARTY_MEMBERS; ++o )
					{
						if ( hMember->GetGoid() == gUIPartyManager.GetSelectedCharacterFromIndex( o )->GetGoid() )
						{
							// Okay, this person is selected, let's use them
							if ( hMember->HasInventory() && hMember->GetInventory()->IsPackOnly() )
							{
								continue;
							}

							if ( gAIQuery.IsInRange(	hObject->GetPlacement()->GetPosition(), hMember->GetPlacement()->GetPosition(),
														hObject->GetStore()->GetActivateRange() ) )				
							{
								ActivateStore( hObject, hMember );								
							}
						}
					}
				}
			}				
		}		
	}
}


void UIStoreManager::ActivateStore( Go * store, Go * member )
{	
	bool bAlreadyActive = GetStoreActive() && GetActiveStore();
	gUIPartyManager.CloseAllCharacterMenus();	

	if ( store->GetStore()->IsItemStore() )
	{	
		int index = 0;
		gUIShell.ShiftInterface( "character", -22, 0 );
		gUIShell.ShiftInterface( "inventory", -22, 0 );		
		gUIShell.ShiftInterface( "spell", -22, 0 );
		gUIPartyManager.GetCharacterFromGoid( member->GetGoid(), index );
		gUIPartyManager.ConstructCharacterInterface( index, true, true );		

		gpstring sCharacter;
		for ( int i = 0; i != MAX_PARTY_MEMBERS; ++i )
		{
			sCharacter.assignf( "character_%d_min", i+1 );
			gUI.GetGameGUI().ShowGroup( sCharacter, false );
		}
	}

	if ( store->GetStore()->IsItemStore() && !bAlreadyActive )
	{
		store->GetStore()->SelectDefaultTab();
	}		
	
	store->GetStore()->ShowStore( member->GetGoid() );
	SetActiveStore( store->GetGoid() );
	SetActiveStoreBuyer( member->GetGoid() );
	SetStoreActive( true );
	gSiegeEngine.GetCompass().SetVisible( false );
	gUIShell.HideInterface( "compass_hotpoints" );	
	
	m_bStoreActive = true;

	if ( store->GetStore()->IsItemStore() )
	{
		if ( store->GetStore()->GetGridbox() )
		{
			gUIInventoryManager.CalculateGridHighlights( store->GetStore()->GetGridbox() );
		}
		else
		{
			gperrorf( ( "Cannot calculate grid highlights, store: %s, scid: 0x%x does not have a gridbox. [CQ]", store->GetTemplateName(), MakeInt(store->GetScid()) ) );
		}

		if ( !member->GetInventory()->IsPackOnly() )
		{
			gUIPartyManager.SetSpellsActive( gUIPartyManager.GetSelectedPartyMember()->IsSpellsExpanded() );
		}

		gUIPartyManager.MinimizeFieldCommands();
	}	

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
}


void UIStoreManager::TabStoreChange()
{
	GoHandle hStore( GetActiveStore() );
	if ( hStore.IsValid() )
	{
		hStore->GetStore()->ShowStore( GetActiveStoreBuyer(), true );
		if ( hStore->GetStore()->IsItemStore() )
		{
			gUIInventoryManager.CalculateGridHighlights( hStore->GetStore()->GetGridbox() );
		}

		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
}


void UIStoreManager::TransferInventory()
{
	if ( GetActiveStoreBuyer() != GOID_INVALID ) 
	{
		GoHandle hMember( GetActiveStoreBuyer() );
		GoHandle hStore( GetActiveStore() );
		if ( hMember->GetInventory()->GetGridbox()->GetAutoTransfer() )
		{
			SellItemToActiveStore( MakeGoid(hMember->GetInventory()->GetGridbox()->GetLastItem().itemID) );
		}
		else
		{	
			BuyItemFromActiveStore( MakeGoid(hStore->GetStore()->GetGridbox()->GetLastItem().itemID) );
		}

		if ( hStore->GetStore()->IsItemStore() )
		{
			gUIInventoryManager.CalculateGridHighlights( hStore->GetStore()->GetGridbox() );
		}
	}
}


void UIStoreManager::GridCheckBuyerGold( UIGridbox * pGrid )
{
	if ( pGrid->GetStore() ) {
		if ( GetActiveStoreBuyer() != GOID_INVALID ) 
		{
			GoHandle hMember( GetActiveStoreBuyer() );
			GoHandle hItem( gUIInventoryManager.GetGUIRolloverGOID() );
			if ( hItem )
			{
				GoHandle hStore( GetActiveStore() );
				if ( gUIInventoryManager.GetGUIRolloverGOID() == GOID_INVALID ) 
				{					
					return;
				}
				if ( hMember->GetInventory()->GetGold() >= hStore->GetStore()->GetPrice( hItem->GetGoid(), hMember->GetGoid() ) ) 
				{
					pGrid->SetContinuePickup( true );
				}
				else 
				{
					pGrid->SetContinuePickup( false );
					hMember->PlayVoiceSound( "gui_inventory_not_equippable" );
				}
			}
			else
			{
				pGrid->SetContinuePickup( false );
				hMember->PlayVoiceSound( "gui_inventory_not_equippable" );
			}
		}
	}
}


void UIStoreManager::SellItemToActiveStore( Goid item )
{
	if (( GetActiveStoreBuyer() != GOID_INVALID ) || ( item == GOID_INVALID )) 
	{
		GoHandle hMember( GetActiveStoreBuyer() );
		GoHandle hItem( item );
		GoHandle hStore( GetActiveStore() );	

		if ( hStore.IsValid() )
		{
			hStore->PlayVoiceSound( "sell", false );
		}

		if ( !hItem->HasGold() )
		{		
			hStore->GetStore()->SetTransactionPending( true );
			hStore->GetStore()->RSAddToStore( hItem->GetGoid(), hMember->GetGoid() );							
			hStore->GetStore()->GetGridbox()->SetItemDetect( false );

			if ( hStore->GetStore()->IsItemStore() )
			{
				gUIInventoryManager.CalculateGridHighlights( hStore->GetStore()->GetGridbox() );
			}
		}
	}
}

void UIStoreManager::DialogStoreSellAll( eQueryTrait sellTrait )
{ 
	if ( GetEnableSellAllDialog() )
	{
		gUIStoreManager.SetSelectedSellTrait( sellTrait );

		switch ( sellTrait )
		{
		case QT_ITEM:
			gUIGame.ShowDialog( DIALOG_CONFIRM_SELL_ALL, gpwtranslate( $MSG$ "Are you sure you wish to sell all inventory items?" ) );
			break;
		case QT_POTION:
			gUIGame.ShowDialog( DIALOG_CONFIRM_SELL_ALL, gpwtranslate( $MSG$ "Are you sure you wish to sell all potions?" ) );
			break;
		case QT_SPELL:
			gUIGame.ShowDialog( DIALOG_CONFIRM_SELL_ALL, gpwtranslate( $MSG$ "Are you sure you wish to sell all spells?" ) );
			break;
		case QT_ARMOR_WEARABLE:
			gUIGame.ShowDialog( DIALOG_CONFIRM_SELL_ALL, gpwtranslate( $MSG$ "Are you sure you wish to sell all armor?" ) );
			break;
		case QT_WEAPON:
			gUIGame.ShowDialog( DIALOG_CONFIRM_SELL_ALL, gpwtranslate( $MSG$ "Are you sure you wish to sell all weapons?" ) );
			break;
		case QT_SHIELD:
			gUIGame.ShowDialog( DIALOG_CONFIRM_SELL_ALL, gpwtranslate( $MSG$ "Are you sure you wish to sell all shields?" ) );
			break;		
		}	
	}
	else
	{
		StoreSellAll( sellTrait );
	}	
}

void UIStoreManager::StoreSellAll( eQueryTrait sellTrait ) 
{
	GoHandle hMember( GetActiveStoreBuyer() );		
	GoHandle hStore( GetActiveStore() );
	if ( hStore && hMember )
	{				
		hStore->GetStore()->GetGridbox()->SetItemDetect( false );
		hStore->GetStore()->RSSellAllOfTrait( sellTrait, hMember->GetGoid() );							
		if ( hStore->GetStore()->IsItemStore() )
		{
			gUIInventoryManager.CalculateGridHighlights( hStore->GetStore()->GetGridbox() );
		}
	}	
}

void UIStoreManager::BuyItemFromActiveStore( Goid item, bool bAutoPlace )
{
	if (( GetActiveStoreBuyer() != GOID_INVALID ) || ( item == GOID_INVALID )) 
	{
		GoHandle hMember( GetActiveStoreBuyer() );
		GoHandle hItem( item );
		GoHandle hStore( GetActiveStore() );

		if ( hStore->GetStore()->GetTransactionPending() )
		{
			return;
		}

		if ( hMember->GetInventory()->GetGold() < hStore->GetStore()->GetPrice( hItem->GetGoid(), hMember->GetGoid() ) ) 
		{
			return;
		}
		
		if( hMember->GetInventory()->CanAdd( hItem, IL_MAIN, bAutoPlace ) )
		{			
			hStore->GetStore()->SetTransactionPending( true );
			hStore->GetStore()->RSRemoveFromStore( hItem->GetGoid(), hMember->GetGoid(), true );								
		}

		if ( hStore->GetStore()->IsItemStore() )
		{
			gUIInventoryManager.CalculateGridHighlights( hStore->GetStore()->GetGridbox() );
		}
	}
}


void UIStoreManager::GridCheckDestInvSpace( UIGridbox * pGrid )
{
	if ( pGrid->GetStore() ) 
	{
		if ( GetActiveStoreBuyer() != GOID_INVALID ) 
		{
			GoHandle hMember( GetActiveStoreBuyer() );
			GoHandle hItem( gUIInventoryManager.GetGUIRolloverGOID() );
			if ( gUIInventoryManager.GetGUIRolloverGOID() == GOID_INVALID ) 
			{					
				return;
			}

			if ( hMember->GetInventory()->CanAdd( hItem, IL_MAIN, true ) )
			{
				pGrid->SetContinuePickup( true );
			}
			else {
				pGrid->SetContinuePickup( false );
			}
		}
	}
}



void UIStoreManager::DisableAutoTransfer()
{
	UIWindowVec gridboxes = gUIShell.ListWindowsOfType( UI_TYPE_GRIDBOX );
	UIWindowVec::iterator i;
	for ( i = gridboxes.begin(); i != gridboxes.end(); ++i )
	{
		((UIGridbox *)(*i))->SetAutoTransfer( false );
	}

	UIRadioButton * pBuy = (UIRadioButton *)gUIShell.FindUIWindow( "button_buy" );
	pBuy->SetCheck( false );
	
	UIRadioButton * pSell = (UIRadioButton *)gUIShell.FindUIWindow( "button_sell" );
	pSell->SetCheck( false );

	gUIGame.GetUICommands()->SetCursor( CURSOR_GUI_STANDARD );	
}


void UIStoreManager::ButtonHire()
{
	UIListReport * pList = (UIListReport *)gUIShell.FindUIWindow( "listreport_hires" );
	int tag = pList->GetSelectedElementTag();
	if (( tag == 1 ) || ( tag == -1 ))
	{
		return;
	}
	Goid hire = MakeGoid(tag);

	if ( GetActiveStoreBuyer() != GOID_INVALID ) 
	{
		Player * pPlayer = gServer.GetScreenPlayer();
		if ( pPlayer )
		{
			Go * pParty	= pPlayer->GetParty();
			if ( pParty )
			{
				if ( pParty->GetChildren().size() == MAX_PARTY_MEMBERS ) 
				{
					gpscreen( $MSG$ "Your party is currently full.\n" );
					return;
				}
			}
		}
		
		GoHandle hMember( GetActiveStoreBuyer() );
		GoHandle hHire( hire );
		GoHandle hStore( GetActiveStore() );

		if ( hMember->GetInventory()->GetGold() >= hHire->GetAspect()->GetGoldValue() )
		{	
			int talkerValue = hHire->GetAspect()->GetGoldValue();
			hMember->GetInventory()->SSetGold( hMember->GetInventory()->GetGold() - talkerValue );			
			
			hHire->GetAspect()->SSetGoldValue( 0, false );
		
			hStore->GetStore()->TransferHireToBuyer( hHire, hMember );		
		}		
	}
}


int UIStoreManager::GetBuyerGold()
{
	GoidColl members;	
	gUIPartyManager.GetSelectedPartyMembers( members );
	GoidColl::iterator i = members.begin();
	GoHandle hMember( *i );
	if ( hMember.IsValid() )
	{
		return hMember->GetInventory()->GetGold();
	}

	return 0;
}


void UIStoreManager::ButtonBuyPackmule()
{
	if ( GetActiveStoreBuyer() != GOID_INVALID ) 
	{
		Player * pPlayer = gServer.GetScreenPlayer();
		if ( pPlayer )
		{
			Go * pParty	= pPlayer->GetParty();
			if ( pParty )
			{
				if ( pParty->GetChildren().size() == MAX_PARTY_MEMBERS ) 
				{
					gpscreen( $MSG$ "Your party is currently full.\n" );
					return;
				}
			}
		}
		
		GoHandle hMember( GetActiveStoreBuyer() );		
		GoHandle hStore( GetActiveStore() );

		if ( hMember->GetInventory()->GetGold() >= hStore->GetStore()->GetHireCost() )
		{	
			int talkerValue = hStore->GetStore()->GetHireCost();			
			hMember->GetInventory()->SSetGold( hMember->GetInventory()->GetGold() - talkerValue );			
			hStore->GetStore()->TransferHireToBuyer( hMember );		
		}	
	}
}


void UIStoreManager::ButtonAcceptMember()
{
	Player * pPlayer = gServer.GetScreenPlayer();
	if ( pPlayer )
	{
		Go * pParty	= pPlayer->GetParty();
		if ( pParty )
		{
			if ( pParty->GetChildren().size() == MAX_PARTY_MEMBERS ) 
			{
				gUIGame.ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "Your party is currently full.\n" ) );
				return;
			}
		}
	}

	GoHandle hTalker( gUIDialogueHandler.GetTalkerGUID() );
	GoHandle hMember( gUIDialogueHandler.GetMemberGUID() );
	if ( !hMember.IsValid() )
	{
		return;
	}

	if ( hTalker->GetAspect()->GetGoldValue() != 0 )
	{
		ButtonHireMember();
	}
	else
	{
		if ( hTalker.IsValid() && hMember.IsValid() )
		{
			hMember->GetParent()->GetParty()->RSAddMemberNow( hTalker );
			gGoDb.Select( hTalker->GetGoid() );	
		}

		gpstring sTalker;
		sTalker.assignf( "party_accept_0x%x", hTalker->GetScid() );
		gGameAuditor.Set( sTalker.c_str(), true );	
		gUIGame.GetUICommands()->CommandTalk( hMember, hTalker );
	}
}


void UIStoreManager::ButtonDeclineMember()
{
	GoHandle hTalker( gUIDialogueHandler.GetTalkerGUID() );
	GoHandle hMember( gUIPartyManager.GetSelectedCharacter() );
	if ( !hMember.IsValid() )
	{
		GoidColl members;
		gUIPartyManager.GetSelectedPartyMembers( members );
		hMember = *(members.begin());
	}

	gpstring sTalker;
	sTalker.assignf( "party_decline_0x%x", hTalker->GetScid() );
	gGameAuditor.Set( sTalker.c_str(), true );

	if ( hMember.IsValid() && hTalker.IsValid() )
	{
		gUIGame.GetUICommands()->CommandTalk( hMember, hTalker );
	}
	else
	{
		gperrorf( ( "Error in Declining the member.  Info - Selected Member = %d, Member Goid 0x%x, Talker Goid 0x%x", 
					gUIPartyManager.GetSelectedCharacterIndex(), MakeInt( gUIPartyManager.GetSelectedCharacter() ), 
					MakeInt( gUIDialogueHandler.GetTalkerGUID() ) ) );
	}
}


void UIStoreManager::ButtonShop()
{
	CheckForStoreAndActivate( gUIDialogueHandler.GetTalkerGUID(), true );
	gUIDialogueHandler.ExitDialogue();
	gSiegeEngine.GetCompass().SetVisible( false );
	gUIShell.HideInterface( "compass_hotpoints" );	
	gUIGameConsole.EnableConsoleUpdate( false );
}


void UIStoreManager::ButtonHireMember()
{
	Player * pPlayer = gServer.GetScreenPlayer();
	if ( pPlayer )
	{
		Go * pParty	= pPlayer->GetParty();
		if ( pParty )
		{
			if ( pParty->GetChildren().size() == MAX_PARTY_MEMBERS ) 
			{
				gpscreen( $MSG$ "Your party is currently full.\n" );
				return;
			}
		}
	}

	GoHandle hTalker( gUIDialogueHandler.GetTalkerGUID() );
	
	GoHandle hMember( gUIDialogueHandler.GetMemberGUID() );
	if ( hMember->GetInventory()->GetGold() >= hTalker->GetAspect()->GetGoldValue() )
	{	
		int talkerValue = hTalker->GetAspect()->GetGoldValue();
		hMember->GetInventory()->SSetGold( hMember->GetInventory()->GetGold() - talkerValue );			
				
		hTalker->GetAspect()->SSetGoldValue( 0, false );
	
		if ( hTalker.IsValid() && hMember.IsValid() )
		{
			hMember->GetParent()->GetParty()->RSAddMemberNow( hTalker );
			gGoDb.Select( hTalker->GetGoid() );
		}

		gpstring sTalker;
		sTalker.assignf( "party_accept_0x%x", hTalker->GetScid() );
		gGameAuditor.Set( sTalker.c_str(), true );
		gUIGame.GetUICommands()->CommandTalk( hMember, hTalker );
	}
	else
	{		
		gpstring sTalker;
		sTalker.assignf( "party_accept_no_money_0x%x", hTalker->GetScid() );
		gGameAuditor.Set( sTalker.c_str(), true );
		gUIGame.GetUICommands()->CommandTalk( hMember, hTalker );
	}
}


void UIStoreManager::ButtonPackRehire()
{
	Player * pPlayer = gServer.GetScreenPlayer();
	if ( pPlayer )
	{
		Go * pParty	= pPlayer->GetParty();
		if ( pParty )
		{
			if ( pParty->GetChildren().size() == MAX_PARTY_MEMBERS ) 
			{
				gpscreen( $MSG$ "Your party is currently full.\n" );
				return;
			}
		}

		GoHandle hPack( gUIPartyManager.GetRehireObject() );
	
		if ( hPack.IsValid() )
		{
			pParty->GetParty()->RSAddMemberNow( hPack );
			gGoDb.Select( hPack->GetGoid() );	
		}
	}			
}


void UIStoreManager::DeactivateStore()
{
	gSiegeEngine.GetCompass().SetVisible( true );
	gUIShell.ShowInterface( "compass_hotpoints" );
	gUIGame.SetBackendMode( BMODE_NONE );
	Goid store = GetActiveStore();
	Goid buyer = GetActiveStoreBuyer();

	GoHandle hStore( store );
	if ( hStore.IsValid() && hStore->HasStore() && m_bStoreActive )
	{		
		hStore->GetStore()->RSRemoveShopper( buyer );
	}

	if ( store != GOID_INVALID && buyer != GOID_INVALID )
	{
		gUIDialogueHandler.SetTalkerGUID( store );
		gUIDialogueHandler.SetMemberGUID( buyer );		
	}

	gUIDialogueHandler.ExitDialogue( true, true );
	gUIPartyManager.CloseAllCharacterMenus();	
	gUIDialogueHandler.SetTalkerGUID( store );
	gUIDialogueHandler.SetMemberGUID( buyer );	
	SetActiveStoreBuyer( GOID_INVALID );
	SetAutoActivator( GOID_INVALID );

	if ( m_bStoreActive && !gUIShell.IsInterfaceVisible( "dialogue_box" ) )
	{		
		gUIShell.ShiftInterface( "character", 22, 0 );
		gUIShell.ShiftInterface( "inventory", 22, 0 );		
		gUIShell.ShiftInterface( "spell", 22, 0 );		
		m_bStoreActive = false;				
	}
}


void UIStoreManager::PurchaseGuiCb( Goid member, Goid item )
{
	GoHandle hStore( GetActiveStore() );
	GoHandle hMember( member );
	if ( hMember.IsValid() && hStore.IsValid() && hStore->HasStore() && m_bStoreActive && !gUIShell.GetItemActive() )
	{		
		UIItem * pItem = gUIShell.GetItem( MakeInt( item ) );
		if ( pItem && !hMember->GetInventory()->GetGridbox()->ContainsID( MakeInt( item ) ) )
		{
			pItem->ActivateItem( true );
		}

		hStore->GetStore()->GetGridbox()->RemoveID( MakeInt( item ) );

		hStore->GetStore()->GetGridbox()->SetItemDetect( true );
	}	
}


void UIStoreManager::DisplayNextPage()
{
	GoHandle hStore( GetActiveStore() );
	if ( hStore.IsValid() )
	{
		hStore->GetStore()->RefreshToPage( hStore->GetStore()->GetCurrentPage() + 1 );
		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
}


void UIStoreManager::DisplayPreviousPage()
{
	GoHandle hStore( GetActiveStore() );
	if ( hStore.IsValid() )
	{
		hStore->GetStore()->RefreshToPage( hStore->GetStore()->GetCurrentPage() - 1 );
		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
}


void UIStoreManager::HandleStoreSwitch( Goid newMember )
{
	GoHandle hMember( newMember );
	GoHandle hStore	( GetActiveStore() );

	if ( hStore.IsValid() && hMember->IsInActiveScreenWorldFrustum() )
	{
		// First, check to see the party member is within range
		if ( !gAIQuery.IsInRange(	hStore->GetPlacement()->GetPosition(), hMember->GetPlacement()->GetPosition(),
									hStore->GetStore()->GetActivateRange() ) )				
		{
			if ( gUIStoreManager.GetStoreActive() )
			{					
				if ( hMember->IsInActiveWorldFrustum() )
				{							
					// They are not in range, but they are in the frustum.  Have them walk up to talk to the shop keep to
					// activate the store

					gUIGame.GetUICommands()->CommandTalk( hMember, hStore );
					gUIStoreManager.SetAutoActivator( hMember->GetGoid() );
				}
				else
				{
					// No dice, player can't make it to the store, shut it down.
					gUIStoreManager.DeactivateStore();
				}
			}
		}
		else if ( gUIStoreManager.GetStoreActive() )
		{	
			// In range, just activate									
			gUIStoreManager.ActivateStore( hStore, hMember );							
		}
	}
	else if ( gUIStoreManager.GetStoreActive() )
	{
		gUIStoreManager.DeactivateStore();
	}
}