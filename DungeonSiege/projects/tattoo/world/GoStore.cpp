//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoStore.cpp
// Author(s):  Scott Bilas, Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoStore.h"

#include "ContentDb.h"
#include "FormulaHelper.h"
#include "FuBiPersist.h"
#include "GuiHelper.h"
#include "GoActor.h"
#include "GoAspect.h"
#include "GoAttack.h"
#include "GoCore.h"
#include "GoData.h"
#include "GoDefend.h"
#include "GoMagic.h"
#include "GoMind.h"
#include "GoParty.h"
#include "GoSupport.h"
#include "GoInventory.h"
#include "PContentDb.h"
#include "Services.h"
#include "ui_itemslot.h"
#include "ui_listreport.h"
#include "ui_messenger.h"
#include "ui_shell.h"
#include "ui_tab.h"
#include "world.h"
#include "WorldMap.h"

DECLARE_GO_SERVER_COMPONENT( GoStore, "store" );

//////////////////////////////////////////////////////////////////////////////
// class GoStore implementation

GoStore :: GoStore( Go* parent )
	: Inherited( parent )
	, m_pGridbox( NULL )
	, m_bCanSellSelf( false )
	, m_newMember( GOID_INVALID )
	, m_shopper( GOID_INVALID )
	, m_bTransactionPending( false )
	, m_currentPage( 0 )
	, m_Markup( 0 )	
{
	// this space intentionally left blank...
}

GoStore :: GoStore( const GoStore& source, Go* parent )
	: Inherited( source, parent )
	, m_pGridbox( NULL )
	, m_bCanSellSelf( source.m_bCanSellSelf )
	, m_newMember( GOID_INVALID )	
	, m_shopper( GOID_INVALID )
	, m_bTransactionPending( false )
	, m_currentPage( 0 )
	, m_Markup( source.m_Markup )	
{
	// this space intentionally left blank...
}

GoStore :: ~GoStore( void )
{
	// this space intentionally left blank...
}

void GoStore :: RSAddToStore( Goid item, Goid member )
{
	FUBI_RPC_TAG();

	// First, let's make receipts so we don't have pass the goid over the network again.
	GoHandle hItem( item );
	GoHandle hMember( member );
	if ( hMember && hItem && hMember->GetPlayer() == gServer.GetLocalHumanPlayer() )
	{				
		InsertReceipt( item, GetShopper() );		
	}
	
	FUBI_RPC_THIS_CALL( RSAddToStore, RPC_TO_SERVER );

	// Do the actual add here
	SAddToStore( item, member );
	
	SAddRefresh( member );
}

void GoStore :: SAddToStore( Goid item, Goid member )
{
	CHECK_SERVER_ONLY;

	GoHandle hMember( member );
	if ( hMember.IsValid() )
	{	
		GoHandle hItem( item );
		if ( hItem.IsValid() && hItem->HasGui() && !hItem->GetGui()->GetCanSell() )
		{
			// Cannot sell this item
			return;
		}

		hMember->GetInventory()->RSTransfer( GoHandle( item ), GetGo(), IL_MAIN, AO_REFLEX );				
		hMember->GetInventory()->SSetGold( hMember->GetInventory()->GetGold() + GetPrice( item, member ) );
		if ( hMember->GetInventory()->GetGold() > gContentDb.GetMaxPartyGold() )
		{			
			int excess = hMember->GetInventory()->GetGold() - gContentDb.GetMaxPartyGold();		
			SiegePos	dropPos;	
			Quat		dropQuat;
			hMember->GetInventory()->RetrieveDropPositionAndOrientation( NULL, GoHandle( item ), dropPos, dropQuat );	
			hMember->GetMind()->RSDropGold( excess, dropPos, QP_BACK, AO_REFLEX );			
		}

		// Add this item to our master collection
		{
			InsertReceipt( item, member );

			if ( hItem->GetGui()->IsSpellBook() )
			{
				GopColl spells;			
				if ( hItem->GetInventory()->ListItems( IL_ALL, spells ) )
				{
					GopColl::iterator iSpell;
					for ( iSpell = spells.begin(); iSpell != spells.end(); ++iSpell )
					{
						InsertReceipt( (*iSpell)->GetGoid(), member );
					}
				}
			}
		}		
	}
}

void GoStore :: SAddRefresh( Goid member )
{
	CHECK_SERVER_ONLY;

	// Now handle the refresh for everyone.
	GoHandle hMember( member );
	if ( hMember )
	{
		RCAddToStore( hMember->GetPlayer()->GetMachineId() );	
		GoidColl::iterator iShopper;
		for ( iShopper = m_multiShopperColl.begin(); iShopper != m_multiShopperColl.end(); ++iShopper )
		{
			if ( *iShopper != hMember->GetGoid() )
			{
				GoHandle hRemoteShopper( *iShopper );
				if ( hRemoteShopper.IsValid() )
				{
					RCRefreshStoreView( hRemoteShopper->GetPlayer()->GetMachineId() );
				}
			}
		}
	}
}

void GoStore :: RCAddToStore( DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCAddToStore, machineId );

	AddToStore();
}

void GoStore :: AddToStore( void )
{
	UIWindowVec gridboxes = gUIShell.ListWindowsOfType( UI_TYPE_GRIDBOX );
	UIWindowVec::iterator k;
	for ( k = gridboxes.begin(); k != gridboxes.end(); ++k )
	{
		((UIGridbox *)(*k))->SetItemDetect( true );
	}	
	
	RefreshToPage( GetCurrentPage() );
	RefreshTabs();

	SetTransactionPending( false );	
}

void GoStore :: RSSellAllOfTrait( enum eQueryTrait sellTrait, Goid member )
{
	// $ This function is a lot faster than it was ( and safer ), but it is still very slow considering that RSTransfer and
	// SSetGold is being called on each item.

	FUBI_RPC_TAG();
	
	// Handle all of the local receipt business up front.
	{		
		GoHandle hMember( member );		
		if ( hMember && hMember->GetPlayer() == gServer.GetLocalHumanPlayer() )	
		{
			GopColl items;
			GopColl::iterator i;

			bool bHasItems = false;
			hMember->GetInventory()->ListItems( sellTrait, IL_MAIN, items );
			for ( i = items.begin(); i != items.end(); ++i )
			{
				if ( !(*i)->IsEquipped() )
				{
					bHasItems = true;

					// Add this item to our records		
					InsertReceipt( (*i)->GetGoid(), member );										
				}		
			}		

			if ( bHasItems )
			{
				SetTransactionPending( true );	

				// we actually have items that will be sold, so play the "cha-ching!" sound.
				GetGo()->PlayVoiceSound( "sell", false );
			}
			else
			{
				// early out, no need to have the server do any work.
				GetGridbox()->SetItemDetect( true );
				return;
			}
		}
	}

	FUBI_RPC_THIS_CALL( RSSellAllOfTrait, RPC_TO_SERVER );

	// Now that we're on the server, let's do all the hard work of selling the stuff.
	GopColl items;
	GopColl::iterator i;
	GoHandle hMember( member );
	if ( hMember )
	{
		hMember->GetInventory()->ListItems( sellTrait, IL_MAIN, items );
		for ( i = items.begin(); i != items.end(); ++i )
		{
			if ( !(*i)->IsEquipped() )
			{
				SAddToStore( (*i)->GetGoid(), member );
			}
		}
	}

	// All done, let's refresh the interested parties.
	SAddRefresh( member );
}

void GoStore :: RSRemoveFromStore( Goid item, Goid member, bool bAutoPlace )
{
	FUBI_RPC_TAG();

	// check to see if this is a local store go before proceeding.
	{
		GoHandle hItem( item );
		if ( hItem->IsLocalGo() )
		{
			RemoveLocalItemFromStore( item, bAutoPlace );
			return;
		}
	}

	FUBI_RPC_THIS_CALL( RSRemoveFromStore, RPC_TO_SERVER );

	GoHandle hItem( item );
	GoHandle hMember( member );	
	if ( hMember.IsValid() && hItem.IsValid() )
	{			
		if ( GetGo()->GetInventory()->Contains( hItem ) )
		{
			// only transfer as long as the member doesn't already own it yet - like if it was a purchased local item.
			GetGo()->GetInventory()->RSTransfer( GoHandle( item ), hMember, IL_MAIN, AO_REFLEX, bAutoPlace );				
		}
		else if ( !hMember->GetInventory()->Contains( hItem ) )
		{
			return;
		}

		// insert this into the master purchase list if possible
		InsertReceipt( item, GetGo()->GetGoid() );

		hMember->GetInventory()->SSetGold( hMember->GetInventory()->GetGold() - GetPrice( item, member ) );					
		RCRemoveFromStore( item, hMember->GetPlayer()->GetMachineId() );					

		GoidColl::iterator iShopper;
		for ( iShopper = m_multiShopperColl.begin(); iShopper != m_multiShopperColl.end(); ++iShopper )
		{
			if ( *iShopper != hMember->GetGoid() )
			{
				GoHandle hRemoteShopper( *iShopper );
				if ( hRemoteShopper.IsValid() )
				{
					RCRemoveItemFromGrid( item, hRemoteShopper->GetPlayer()->GetMachineId() );
				}
			}
		}
	}
}

void GoStore :: RCRemoveFromStore( Goid item, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCRemoveFromStore, machineId );	

	RemoveFromStore( item );
}

void GoStore :: RemoveFromStore( Goid item )
{
	gWorld.PurchaseGuiCallback( m_shopper, item );
	GoHandle hItem( item );			
	
	if ( !gUIShell.GetItemActive() )
	{
		UIWindowVec gridboxes = gUIShell.ListWindowsOfType( UI_TYPE_GRIDBOX );
		UIWindowVec::iterator k;
		for ( k = gridboxes.begin(); k != gridboxes.end(); ++k )
		{
			((UIGridbox *)(*k))->SetItemDetect( true );
		}	
	}

	// insert this into the master purchase list if possible
	InsertReceipt( item, GetGo()->GetGoid() );
	
	RefreshTabs();

	m_bTransactionPending = false;
}

void GoStore :: RemoveLocalItemFromStore( Goid item, bool bAutoPlace )
{
	GoHandle hItem( item );
	if ( hItem )
	{
		CharacterLocalItemMap::iterator iCharItem = m_CharacterLocalItemMap.find( GetShopper() );
		if ( iCharItem == m_CharacterLocalItemMap.end() )
		{
			return;
		}

		LocalItems::iterator i;
		for ( i = (*iCharItem).second.localItems.begin(); i != (*iCharItem).second.localItems.end(); ++i )
		{
			if ( (*i).item == item )
			{
				// okay we found the item, so let's see if it is a pcontent item.  If it is, let's prepend the
				// random seed to the front so it generates the right pcontent for the host.
				RSRemoveLocalItemFromStore( (*i).templateName, (*i).randomSeed, GetShopper(), bAutoPlace );
				
				// delete the local go and cleanup our ui; we don't need it anymore
				if ( !(*i).instantRestock )
				{
					gGoDb.SMarkForDeletion( hItem );
					GetGridbox()->RemoveID( MakeInt( item ) );
					(*iCharItem).second.localItems.erase( i );
				}
				return;
			}
		}
	}
}

void GoStore :: RSRemoveLocalItemFromStore( const gpstring & templateName, DWORD randomSeed, Goid buyer, bool bAutoPlace )
{
	FUBI_RPC_THIS_CALL( RSRemoveLocalItemFromStore, RPC_TO_SERVER );

	// let's create a base version of the new object
	GoCloneReq cloneReq;
	cloneReq.SetStartingPos( GetGo()->GetPlacement()->GetPosition() );	
	Goid newObject = gGoDb.SCloneGo( cloneReq, templateName, &randomSeed );
	
	GoHandle hObject( newObject );
	if ( hObject )
	{	
		// Let's just add it straight to the buyer's inventory to save us an RPC call.
		GoHandle hBuyer( buyer );		
		hBuyer->GetInventory()->RSAdd( hObject, IL_MAIN, AO_REFLEX, bAutoPlace );		
		
		// Okay, let's officially "buy" this newly created item from the store
		RSRemoveFromStore( newObject, buyer, bAutoPlace );
	}
}

void GoStore :: RCRemoveItemFromGrid( Goid item, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCRemoveItemFromGrid, machineId );	

	if ( GetGridbox() )
	{
		GetGridbox()->RemoveID( MakeInt( item ) );
		GetGridbox()->SetItemDetect( true );
		m_bTransactionPending = false;
	}
}

bool GoStore :: InsertReceipt( Goid item, Goid soldBy )
{
	PlayerId playerId = PLAYERID_INVALID;
	GoHandle hSeller( soldBy );
	playerId = hSeller->GetPlayer()->GetId();
	
	Receipts::iterator i;
	for ( i = m_receipts.begin(); i != m_receipts.end(); ++i )
	{
		if ( (*i).item == item )
		{
			return false;
		}
	}

	TransactionReceipt receipt;
	receipt.item	= item;
	receipt.soldBy	= playerId;
	m_receipts.push_back( receipt );

	// auto add receipts for all the spells in the spellbook so the caller doesn't
	// have to maintain it.
	GoHandle hItem( item );
	if ( hItem && hItem->GetGui()->IsSpellBook() )
	{
		GopColl spells;			
		if ( hItem->GetInventory()->ListItems( IL_ALL, spells ) )
		{
			GopColl::iterator iSpell;
			for ( iSpell = spells.begin(); iSpell != spells.end(); ++iSpell )
			{
				InsertReceipt( (*iSpell)->GetGoid(), soldBy );
			}
		}
	}	

	return true;
}

void GoStore :: RecalcItemCollection( PriceToGoMultiMap & items )
{
	// This function takes both the global and the local inventory items, finds their current 
	// selling price, and places it in a map given by the user.  So when we're done, we have
	// a price sorted collection.  We also take the time to delete any items that were 
	// sold which don't need to be there ( ex. - half consumed potions, but it depends on the store's
	// specification.

	items.clear();
	
	GopColl invItems;
	GopColl::iterator iInvItem;
	GetGo()->GetInventory()->ListItems( IL_MAIN, invItems );
	for ( iInvItem = invItems.begin(); iInvItem != invItems.end(); ++iInvItem )
	{
		if ( !GetGo()->GetInventory()->IsEquipped( (*iInvItem) ) && !(*iInvItem)->IsLocalGo() )
		{
			StringSet::iterator found = m_instantRestockColl.find( (*iInvItem)->GetTemplateName() );
			if ( found != m_instantRestockColl.end() )
			{
				gGoDb.RSMarkForDeletion( *iInvItem );
				continue;
			}

			items.insert( std::make_pair( GetPrice( (*iInvItem)->GetGoid(), GetShopper() ), *iInvItem ) );			
		}
	}

	CharacterLocalItemMap::iterator iCharItem = m_CharacterLocalItemMap.find( GetShopper() );
	if ( iCharItem != m_CharacterLocalItemMap.end() )
	{
		LocalItems::iterator i;
		for ( i = (*iCharItem).second.localItems.begin(); i != (*iCharItem).second.localItems.end(); ++i )
		{
			GoHandle hObject( (*i).item );
			if ( hObject )
			{
				items.insert( std::make_pair( GetPrice( (*i).item, GetShopper() ), hObject.Get() ) );	
			}
		}
	}
}

void GoStore :: RSAddShopper( Goid shopper )
{
	FUBI_RPC_THIS_CALL( RSAddShopper, RPC_TO_SERVER );
	
	GoHandle hShopper( shopper );
	if ( hShopper.IsValid() )
	{
		hShopper->GetPlayer()->SetIsTrading( true );
	}
	
	GoidColl::iterator i;
	for ( i = m_multiShopperColl.begin(); i != m_multiShopperColl.end(); ++i )
	{
		if ( *i == shopper )
		{
			return;
		}
	}

	m_multiShopperColl.push_back( shopper );	
}

void GoStore :: RSRemoveShopper( Goid shopper )
{
	FUBI_RPC_TAG();

	GoHandle hShopper( shopper );
	if ( !hShopper )
	{
		return;
	}

	if ( hShopper->GetPlayer()->GetId() == gServer.GetLocalHumanPlayer()->GetId() )
	{
		// Cleanup
		if ( GetGridbox() )
		{
			GetGridbox()->ResetGrid();
		}

		UIItemSlot * pSlot = (UIItemSlot *)gUIShell.FindUIWindow( "itemslot_store_owner" );
		if ( pSlot )
		{
			UIItem * pItem = gUIShell.GetItem( pSlot->GetItemID() );
			if ( pItem )
			{
				pSlot->ClearItem();
				pItem->RemoveItemParent( pSlot );
			}
		}
	}

	FUBI_RPC_THIS_CALL( RSRemoveShopper, RPC_TO_SERVER );
	
	if ( hShopper.IsValid() )
	{
		hShopper->GetPlayer()->SetIsTrading( false );
	}	

	GoidColl::iterator i;
	for ( i = m_multiShopperColl.begin(); i != m_multiShopperColl.end(); ++i )
	{
		if ( *i == shopper )
		{
			m_multiShopperColl.erase( i );
			return;
		}
	}
}

void GoStore :: CreatePStoreItemsForTab( const gpstring & sTabType )
{	
	// When the user clicks on a tab, create any needed local go items if the specification calls for it.
	if ( !GetLocalSpec().empty() && CanCreateStoreItemsForTab( sTabType ) )
	{		
		gpstring sItemAddress;
		sItemAddress.assignf( "%s:info:shop_specifications:%s", gWorldMap.MakeMapDirAddress().c_str(), GetLocalSpec().c_str() );
		FastFuelHandle hShopSpec( sItemAddress );
		if ( !hShopSpec )
		{
			gperrorf( ( "The shop_spec for %s, scid 0x%x, was not found in the map: %s. Check your data. [CQ]", 
						GetGo()->GetTemplateName(), MakeInt( GetGo()->GetScid() ), gWorldMap.GetMapName().c_str() ) );
		}
		else
		{
			FastFuelHandleColl tabs;
			FastFuelHandleColl::iterator i;
			hShopSpec.ListChildren( tabs, 1 );
			for ( i = tabs.begin(); i != tabs.end(); ++i )
			{
				// Let's find the items for the tab we want
				if ( sTabType.same_no_case( gpstring().assignf( "tab_%s", (*i).GetName() ) ) )
				{
					CharacterLocalItemMap::iterator iCharItem = m_CharacterLocalItemMap.find( GetShopper() );
					if ( iCharItem == m_CharacterLocalItemMap.end() )
					{
						return;
					}

					// add this to our list so we don't add the items for this tab again.
					(*iCharItem).second.usedTabs.push_back( sTabType );

					FastFuelHandleColl items;
					FastFuelHandleColl::iterator iItem;
					(*i).ListChildren( items, 1 );
					for ( iItem = items.begin(); iItem != items.end(); ++iItem )
					{
						gpstring minPowerFormula, maxPowerFormula, baseTemplate;
						int		createCount		= 0;
						bool	instantRestock	= false;
						(*iItem).Get( "count", createCount );						
						(*iItem).Get( "min_power", minPowerFormula );
						(*iItem).Get( "max_power", maxPowerFormula );
						(*iItem).Get( "il_main", baseTemplate );
						(*iItem).Get( "instant_restock", instantRestock );

						for ( int iCount = 0; iCount != createCount; ++iCount )
						{			
							gpstring templateName = baseTemplate;
							bool isPcontent = false;
							if ( templateName[0] == '#' )
							{
								// First evalulate the min and max power of the item from the given formula
								float minPower = 0.0f, maxPower = 0.0f;
								if ( !minPowerFormula.empty() )
								{		
									FormulaHelper::EvaluateFormula( minPowerFormula, gpstring::EMPTY, minPower, 
																	GetShopper(), GOID_INVALID, GetGo()->GetGoid() );
								}
								if ( !maxPowerFormula.empty() )
								{
									FormulaHelper::EvaluateFormula( maxPowerFormula, gpstring::EMPTY, maxPower, 
																	GetShopper(), GOID_INVALID, GetGo()->GetGoid() );
								}		
								
								if ( minPower != 0.0f && maxPower != 0.0f )
								{
									if ( minPower == maxPower )
									{
										templateName.assignf( "%s/%0.0f", templateName.c_str(), maxPower );
									}
									else
									{
										templateName.assignf( "%s/%0.0f-%0.0f", templateName.c_str(), minPower, maxPower );
									}
								}
								else if ( minPower != 0.0f && maxPower == 0.0f )
								{
									templateName.assignf( "%s/%0.0f", templateName.c_str(), minPower );
								}
								else if ( minPower == 0.0f && maxPower != 0.0f )
								{
									templateName.assignf( "%s/%0.0f", templateName.c_str(), maxPower );
								}
								
								isPcontent = true;
							}

							DWORD randomSeed = GetGoCreateRng().GetSeed();

							// If it is a pcontent object, let's get the random seed and the template name chosen by the pcontentdb															
							PContentReq pcontentReq;
							gpstring newTemplate = templateName;
							if ( isPcontent )
							{								
								const GoDataTemplate* found = gPContentDb.FindTemplateByPContent( pcontentReq, templateName.c_str() + 1 );					
								if ( found == NULL )
								{									
									// pcontent failed to create an object within these parameters, have to continue to the next object.
									continue;
								}			
								else
								{
									newTemplate = found->GetName();
								}
							}								

							GoCloneReq cloneReq		( newTemplate );					
							cloneReq.SetStartingPos	( GetGo()->GetPlacement()->GetPosition() );							
							Goid newObject			= GOID_INVALID;
						
							newObject = gGoDb.CloneLocalGo( cloneReq );
							GoHandle hObject( newObject );		
							if ( hObject )
							{	
								if ( isPcontent )
								{											
									gPContentDb.ApplyPContentRequest( hObject, pcontentReq );																		
								}
							
								// add the local go to the store's inventory to allow it to be sold.
								GetGo()->GetInventory()->Add( hObject, IL_MAIN, AO_REFLEX, false );
								
								LocalItem li;												
								li.item				= newObject;								
								li.templateName		= templateName;								
								li.randomSeed		= randomSeed;
								li.instantRestock	= instantRestock;
								(*iCharItem).second.localItems.push_back( li );

								if ( instantRestock )
								{
									m_instantRestockColl.insert( templateName );
								}
							}																				
						}
					}					
				}
			}
		}	
	}	
}

bool GoStore :: CanCreateStoreItemsForTab( const gpstring & sTabType )
{
	// Just make sure that the character hasn't already created the items for this tab
	CharacterLocalItemMap::iterator iFound = m_CharacterLocalItemMap.find( GetShopper() );
	if ( iFound != m_CharacterLocalItemMap.end() )
	{
		StringVec::iterator i;
		for ( i = (*iFound).second.usedTabs.begin(); i != (*iFound).second.usedTabs.end(); ++i )
		{
			if ( (*i).same_no_case( sTabType ) )
			{
				return false;
			}
		}
	}

	return true;
}

bool GoStore :: DoesStoreHaveItemsForTab( const gpstring & sTabType )
{
	// Just check the fuel specification to see if it has the tab of this name.
	if ( !GetLocalSpec().empty() && CanCreateStoreItemsForTab( sTabType ) )
	{
		gpstring sItemAddress;
		sItemAddress.assignf( "%s:info:shop_specifications:%s", gWorldMap.MakeMapDirAddress().c_str(), GetLocalSpec().c_str() );
		FastFuelHandle hShopSpec( sItemAddress );
		if ( hShopSpec )
		{			
			FastFuelHandleColl tabs;
			FastFuelHandleColl::iterator i;
			hShopSpec.ListChildren( tabs, 1 );
			for ( i = tabs.begin(); i != tabs.end(); ++i )
			{
				if ( sTabType.same_no_case( gpstring().assignf( "tab_%s", (*i).GetName() ) ) )
				{
					return true;
				}
			}
		}
	}

	return false;
}
			

bool GoStore :: ShowStore( Goid shopper, bool bResetPage )
{
	if ( !GetGo()->HasInventory() )
	{
		gperrorf(( "The store owner SCID: 0x%x does not have an inventory, which is REQUIRED.", MakeInt(GetGo()->GetScid()) ));
		return false;
	}

	if ( !GetGridbox() )
	{
		m_pGridbox = (UIGridbox *)gUIShell.FindUIWindow( "gridbox_store" );
		if ( !GetGridbox() )
		{
			gperror( "The gridbox_store gridbox could not be found.  Is all the ui data in place?" );
			return false;
		}
		
		m_pGridbox->SetStore( true );
		SetGridbox( m_pGridbox );
	}	

	m_shopper = shopper;
	RSAddShopper( m_shopper );

	// Now that the person is a local shopper, let's add them to our local item if they aren't there already.
	// (insert will fail if they are already in the collection)
	CharacterLocalItems charLocalItems;
	m_CharacterLocalItemMap.insert( std::make_pair( shopper, charLocalItems ) );
	
	GoHandle hOwner( GetGo()->GetGoid() );	
	UIItem * pItem = NULL;
	if ( hOwner->HasActor() && !hOwner->GetActor()->GetPortraitIcon().empty() )
	{
		pItem = GetItemFromGO( hOwner, true, true );
	}	

	if ( IsItemStore() )
	{
		gUIShell.ShowInterface( "store" );
		UIItemSlot * pSlot = (UIItemSlot *)gUIShell.FindUIWindow( "itemslot_store_owner" );
		if ( pSlot && pItem )
		{
			pSlot->ClearItem();
			pSlot->PlaceItem( pItem, true, true );
			pItem->ActivateItem( false );
			pSlot->SetAcceptInput( false );
		}

		((UIText *)gUIShell.FindUIWindow( "text_store_name" ))->SetText( GetGo()->GetCommon()->GetScreenName() );		

		Restock();

		if ( !bResetPage )
		{
			RefreshToPage( m_currentPage );
		}
		else
		{
			RefreshToPage( 0 );			
		}		

		RefreshTabs();
	}

	return true;
}

void GoStore :: RCRefreshStoreView( DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCRefreshStoreView, machineId );	

	RefreshToPage( GetCurrentPage() );
	RefreshTabs();	
}

void GoStore :: RefreshStoreView()
{
	// First, get the active tab 	
	gpstring sTabType = GetActiveTabType();	
	
	// Since we are showing this tab, let's create all the necessary local items specific to this character first
	CreatePStoreItemsForTab( sTabType );	

	PriceToGoMultiMap items;
	RecalcItemCollection( items );
	m_pGridbox->ResetGrid();

	int gridVolume		= (int)((float)(m_pGridbox->GetRows() * m_pGridbox->GetColumns()) * 0.8f);
	int totalItemVolume = 0;
	int page			= 0;

	PriceToGoMultiMap::reverse_iterator i;	
	for ( i = items.rbegin(); i != items.rend(); ++i )
	{		
		Go * pStoreItem = (*i).second;
		
		if (( pStoreItem->IsShield()	&& sTabType.same_no_case( "tab_shields" ) )	||
			( pStoreItem->IsArmor()		&& !pStoreItem->IsShield() && sTabType.same_no_case( "tab_armor" ) )	||
			( pStoreItem->IsWeapon()	&& sTabType.same_no_case( "tab_weapon" ) )	||
			( pStoreItem->IsSpell()		&& sTabType.same_no_case( "tab_magic" ) )		||
			( pStoreItem->IsPotion()	&& sTabType.same_no_case( "tab_potion" ) ))
		{
			if ( page == m_currentPage )
			{				
				UIItem *pItem = GetItemFromGO( pStoreItem, false );
				pItem->SetItemID( MakeInt( pStoreItem->GetGoid() ) );
				if ( pItem )
				{
					if ( !m_pGridbox->AutoItemPlacement( pItem->GetName().c_str(), true ) )
					{
						pItem->ActivateItem( false );
					}
				}			
			}

			totalItemVolume += pStoreItem->GetGui()->GetInventoryWidth() * pStoreItem->GetGui()->GetInventoryHeight();
			if ( totalItemVolume >= gridVolume )
			{
				page++;
				if ( page > m_currentPage )
				{
					break;
				}
				totalItemVolume = 0;			
			}	
		}
		else if ( sTabType.same_no_case( "tab_misc" ) && !( pStoreItem->IsShield() || pStoreItem->IsArmor() ||
															pStoreItem->IsWeapon() || pStoreItem->IsSpell() ||
															pStoreItem->IsPotion() ) )
		{
			if ( page == m_currentPage )
			{
				UIItem *pItem = GetItemFromGO( pStoreItem, false );
				pItem->SetItemID( MakeInt( pStoreItem->GetGoid() ) );
				if ( pItem )
				{
					if ( !m_pGridbox->AutoItemPlacement( pItem->GetName().c_str(), true ) )
					{
						pItem->ActivateItem( false );
					}
				}			
			}

			totalItemVolume += pStoreItem->GetGui()->GetInventoryWidth() * pStoreItem->GetGui()->GetInventoryHeight();
			if ( totalItemVolume >= gridVolume )
			{
				page++;
				if ( page > m_currentPage )
				{
					break;
				}

				totalItemVolume = 0;				
			}	
		}				
	}	

	m_pGridbox->SetVisible( true );

	UIWindowVec gridboxes = gUIShell.ListWindowsOfType( UI_TYPE_GRIDBOX );
	UIWindowVec::iterator k;
	for ( k = gridboxes.begin(); k != gridboxes.end(); ++k )
	{
		((UIGridbox *)(*k))->SetItemDetect( true );
	}

	// Refresh the button states
	{
		UIButton * pButton = (UIButton * )gUIShell.FindUIWindow( "button_previous", "store" );
		if ( m_currentPage == 0 )
		{				
			pButton->DisableButton();		
		}
		else
		{
			pButton->EnableButton();		
		}
		
		pButton = (UIButton * )gUIShell.FindUIWindow( "button_next", "store" );
		if ( m_currentPage < page )
		{			
			pButton->EnableButton();
		}
		else
		{
			pButton->DisableButton();
		}
	}
}

void GoStore :: SelectDefaultTab( void )
{	
	UIWindowVec tabs = gUIShell.ListWindowsOfRadioGroup( "store" );
	UIWindowVec::iterator j;
	gpstring sTabType;	

	PriceToGoMultiMap items;
	PriceToGoMultiMap::iterator i;	
	RecalcItemCollection( items );

	for ( j = tabs.begin(); j != tabs.end(); ++j )
	{			
		for ( i = items.begin(); i != items.end(); ++i )
		{
			Go * pItem = (*i).second;			

			if (( pItem->IsShield()	&& (*j)->GetName().same_no_case( "tab_shields" ) )	||
				( pItem->IsArmor()	&& !pItem->IsShield() && (*j)->GetName().same_no_case( "tab_armor" ) ) ||
				( pItem->IsWeapon()	&& (*j)->GetName().same_no_case( "tab_weapon" ) )	||
				( pItem->IsSpell()	&& (*j)->GetName().same_no_case( "tab_magic" ) )		||
				( pItem->IsPotion()	&& (*j)->GetName().same_no_case( "tab_potion" ) ))
			{
				UITab * pTab = (UITab *)(*j);
				pTab->SetCheck( true );
				return;
			}
			else if ( (*j)->GetName().same_no_case( "tab_misc" ) && !(	pItem->IsShield() || pItem->IsArmor() ||
																		pItem->IsWeapon() || pItem->IsSpell() ||
																		pItem->IsPotion() ) )
			{
				UITab * pTab = (UITab *)(*j);
				pTab->SetCheck( true );
				return;
			}
		}
	}
}

gpstring GoStore :: GetActiveTabType( void )
{	
		// First, get the active tab 
	UIWindowVec tabs = gUIShell.ListWindowsOfRadioGroup( "store" );
	UIWindowVec::iterator j;
	gpstring sTabType;
	for ( j = tabs.begin(); j != tabs.end(); ++j )
	{			
		if ( ((UITab *)(*j))->GetCheck() )
		{
			return (*j)->GetName();				
		}
	}	

	return gpstring::EMPTY;
}

void GoStore :: RefreshTabs( void )
{
	UIWindowVec tabs = gUIShell.ListWindowsOfRadioGroup( "store" );
	UIWindowVec::iterator j;
	for ( j = tabs.begin(); j != tabs.end(); ++j )
	{
		(*j)->SetEnabled( DoesStoreHaveItemsForTab( (*j)->GetName() ) );		
	}

	PriceToGoMultiMap::iterator i;
	PriceToGoMultiMap items;
	RecalcItemCollection( items );
	for ( i = items.begin(); i != items.end(); ++i )
	{			
		Go * pItem = (*i).second;
		for ( j = tabs.begin(); j != tabs.end(); ++j )
		{
			if (( pItem->IsShield() && (*j)->GetName().same_no_case( "tab_shields"	) )						||
				( pItem->IsArmor()	&& !pItem->IsShield() && (*j)->GetName().same_no_case( "tab_armor" ) )	||
				( pItem->IsWeapon() && (*j)->GetName().same_no_case( "tab_weapon"	) )						||
				( pItem->IsSpell()	&& (*j)->GetName().same_no_case( "tab_magic"	) )						||
				( pItem->IsPotion() && (*j)->GetName().same_no_case( "tab_potion"	) ) )
			{
				(*j)->SetEnabled( true );
				break;
			}			
			else if ( (*j)->GetName().same_no_case( "tab_misc" ) && !(	pItem->IsShield() || pItem->IsArmor() ||
																		pItem->IsWeapon() || pItem->IsSpell() ||
																		pItem->IsPotion() ) )
			{
				(*j)->SetEnabled( true );
				break;
			}
		}
	}	
}

void GoStore::Restock()
{
	GopColl::iterator i;
	GopColl items;
	GetGo()->GetInventory()->ListItems( IL_MAIN, items );

	TemplateToStockMap::iterator j;
	for ( j = m_restockColl.begin(); j != m_restockColl.end(); ++j )
	{
		int count = 0;
		for ( i = items.begin(); i != items.end(); ++i )
		{
			if ( (*j).first.same_no_case( (*i)->GetTemplateName() ) )
			{
				count++;
			}
		}

		if ( count != (*j).second )
		{
			int diff = (*j).second - count;
			if ( diff > 0 )
			{
				for ( int k = 0; k != diff; ++k )
				{
					GetGo()->GetInventory()->RSCreateAddItem( IL_MAIN, (*j).first );
				}
			}
		}
	}		
}

void GoStore :: TransferHireToBuyer( Go * pHire, Go * pBuyer )
{
	GoHandle hOwner( GetGo()->GetParent()->GetGoid() );
	hOwner->RemoveChild( pHire );

	if ( pBuyer->GetParent()->HasParty() )
	{
		if ( gContentDb.GetDefaultMuleTemplate().same_no_case( pHire->GetTemplateName() ) )
		{
			RSAddPackmule( pHire, pBuyer );
			GoHandle hPackmule( m_newMember );
			if ( !hPackmule.IsValid() )
			{
				gpassert( !"The packmule you hired is not a valid object, go bug Chad." );
			}
			else
			{
				pBuyer->GetParent()->GetParty()->RSAddMemberNow( hPackmule );
				gGoDb.Select( hPackmule->GetGoid() );
			}
			m_newMember = GOID_INVALID;

		}
		else
		{
			pBuyer->GetParent()->GetParty()->RSAddMemberNow( pHire );
			gGoDb.Select( pHire->GetGoid() );
		}
	}
}

void GoStore :: TransferHireToBuyer( Go * pBuyer )
{
	GoHandle hOwner( GetGo()->GetParent()->GetGoid() );
	GoHandle hHire;

	GopColl::iterator i;
	for ( i = hOwner->GetChildBegin(); i != hOwner->GetChildEnd(); ++i )
	{
		if ( (*i) != GetGo() )
		{
			hHire = (*i)->GetGoid();
			hOwner->RemoveChild( hHire );
			break;
		}
	}
	
	if ( pBuyer->GetParent()->HasParty() )
	{
		if ( hHire->HasInventory() && hHire->GetInventory()->IsPackOnly() )
		{
			RSAddPackmule( hHire, pBuyer );
			GoHandle hPackmule( m_newMember );
			if ( !hPackmule.IsValid() )
			{
				gpassert( !"The packmule you hired is not a valid object, go bug Chad." );
			}
			else
			{
				pBuyer->GetParent()->GetParty()->RSAddMemberNow( hPackmule );
				gGoDb.Select( hPackmule->GetGoid() );
			}
			m_newMember = GOID_INVALID;

		}
		else
		{
			pBuyer->GetParent()->GetParty()->RSAddMemberNow( hHire );
			gGoDb.Select( hHire->GetGoid() );
		}

		hHire->GetAspect()->SSetGoldValue( 0, false );
	}
}

int GoStore :: GetHireCost()
{
	GoHandle hOwner( GetGo()->GetParent()->GetGoid() );
	GopColl::iterator i;
	for ( i = hOwner->GetChildBegin(); i != hOwner->GetChildEnd(); ++i )
	{
		if ( (*i) != GetGo() )
		{
			return (*i)->GetAspect()->GetGoldValue();			
		}
	}

	return 0;
}

int GoStore :: GetNumHires()
{
	GoHandle hOwner( GetGo()->GetParent()->GetGoid() );
	if ( hOwner->HasParty() )
	{
		return hOwner->GetChildren().size()-1;
	}

	return 0;
}

int GoStore :: GetPrice( Goid item, Goid shopper, bool bNoChildren )
{
	GoHandle hItem		( item );
	GoHandle hShopper	( shopper );
	if ( hItem && hShopper )
	{
		int goldValue =	hItem->GetAspect()->GetGoldValue();		
		Receipts::iterator i;	
		bool bFound = false;
		for ( i = m_receipts.begin(); i != m_receipts.end(); ++i )
		{			
			if ( (*i).item == item )
			{
				if ( (*i).soldBy != hShopper->GetPlayer()->GetId() )
				{
					goldValue += (int)(goldValue * GetItemMarkup());
				}
				bFound = true;
				break;
			}
		}

		Go * pParent = (hItem->IsSpell() && hItem->GetParent()->IsSpellBook()) ? hItem->GetParent()->GetParent() : hItem->GetParent();

		if ( !bFound && pParent->GetGoid() == GetGo()->GetGoid() )
		{
			goldValue += (int)(goldValue * GetItemMarkup());
		}
		
		if ( hItem->IsSpellBook() && !bNoChildren )
		{
			GopColl spells;			
			if ( hItem->GetInventory()->ListItems( IL_ALL, spells ) )
			{
				GopColl::iterator iSpell;
				for ( iSpell = spells.begin(); iSpell != spells.end(); ++iSpell )
				{
					goldValue += GetPrice( (*iSpell)->GetGoid(), shopper );
				}
			}
		}

		if ( hItem->IsPotion() && hItem->HasMagic() )
		{
			int newGoldValue = (int)((float)hItem->GetMagic()->GetPotionFullRatio() * hItem->GetAspect()->GetGoldValue() );
			if ( hItem->GetMagic()->GetPotionFullRatio() != 1.0 || goldValue < newGoldValue )
			{
				goldValue = newGoldValue;
			}			
		}

		return goldValue;
	}

	return 0;
}

void GoStore :: SetAutoTransfer( bool bTransfer )
{
	UIGridbox * pGrid = (UIGridbox *)gUIShell.FindUIWindow( "gridbox_store" );
	pGrid->SetAutoTransfer( bTransfer );
}

bool GoStore :: IsItemStore() const
{
	GoHandle hOwner( GetGo()->GetGoid() );
	if (( hOwner->GetParent() && hOwner->GetParent()->HasParty() ) || GetCanSellSelf() )
	{
		return false;
	}

	return true;
}

float GoStore :: GetActivateRange( void ) const
{
	static AutoConstQuery <float> s_Query( "activate_range" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring & GoStore :: GetLocalSpec( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "local_spec" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

void GoStore :: AppendToolTipString( Goid item, gpwstring & sTooltip )
{
	if ( !sTooltip.empty() )
	{
		sTooltip += L'\n';
	}

	GoHandle hItem( item );
	int price = GetPrice( item, GetShopper() );
	if ( price > gContentDb.GetMaxPartyGold() )
	{
		sTooltip.appendf( gpwtranslate( $MSG$ "Priceless" ) );
	}
	else
	{
		sTooltip.appendf( gpwtranslate( $MSG$ "Buy Value %d" ), price );
	}
}

FuBiCookie GoStore :: RSAddPackmule( Go * pHire, Go * pBuyer )
{
	FUBI_RPC_THIS_CALL_RETRY( RSAddPackmule, RPC_TO_SERVER );
	CHECK_SERVER_ONLY;

	gGoDb.SMarkForDeletion( pHire->GetGoid() );
	GoCloneReq cloneReq( gContentDb.GetDefaultPackmuleTemplate(), pBuyer->GetPlayer()->GetId() );
	cloneReq.SetStartingPos( pHire->GetPlacement()->GetPosition() );
	cloneReq.SetStartingOrient( pHire->GetPlacement()->GetOrientation() );

	m_newMember = gGoDb.SCloneGo( cloneReq );

	GoHandle hNewMember( m_newMember );
	if ( hNewMember.IsValid() )
	{
		hNewMember->GetCommon()->SetScreenName( pHire->GetCommon()->GetScreenName() );
	}

	gGoDb.CommitAllRequests( false, true );

	return( RPC_SUCCESS );
}

FUBI_DECLARE_XFER_TRAITS( GoStore::TransactionReceipt )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		persist.EnterBlock( key );						
		persist.Xfer( "item",			obj.item	);
		persist.Xfer( "soldBy",			obj.soldBy	);
		
		persist.LeaveBlock();
		return ( true );
	}
};

FUBI_DECLARE_XFER_TRAITS( GoStore::LocalItem )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		persist.EnterBlock( key );						
		persist.Xfer( "templateName",	obj.templateName	);
		persist.Xfer( "randomSeed",		obj.randomSeed		);
		persist.Xfer( "item",			obj.item			);
		persist.Xfer( "instant_restock",obj.instantRestock	);
		
		persist.LeaveBlock();
		return ( true );
	}
};

FUBI_DECLARE_XFER_TRAITS( GoStore::CharacterLocalItems )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		persist.EnterBlock( key );				
		persist.XferVector( "usedTabs",		obj.usedTabs	);			
		persist.XferVector( "localItems",	obj.localItems	);			
		
		persist.LeaveBlock();
		return ( true );
	}
};

GoComponent* GoStore :: Clone( Go* newParent )
{
	return ( new GoStore( *this, newParent ) );
}

bool GoStore :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "can_sell_self",   m_bCanSellSelf  );
	persist.Xfer( "item_markup",	 m_Markup );

	if ( persist.IsFullXfer() )
	{
		persist.XferVector	( "m_receipts",					m_receipts				);
		persist.Xfer		( "m_newMember",				m_newMember				);
		persist.Xfer		( "m_shopper",					m_shopper				);
		persist.Xfer		( "m_currentPage",				m_currentPage			);
		persist.XferVector	( "m_multiShopperColl",			m_multiShopperColl		);	
		persist.XferMap		( "m_CharacterLocalItemMap",	m_CharacterLocalItemMap );	
		persist.XferSet		( "m_instantRestockColl",		m_instantRestockColl	);
	}

	if ( persist.IsRestoring() )
	{
		FastFuelHandle hRestock = GetData()->GetInternalField( "item_restock" );
		if ( hRestock )
		{
			FastFuelFindHandle fh( hRestock );
			if ( fh.FindFirstKeyAndValue() )
			{
				gpstring sKey;
				gpstring sValue;
				while ( fh.GetNextKeyAndValue( sKey, sValue ) )
				{
					int value = 0;
					stringtool::Get( sValue, value );
					m_restockColl.insert( TemplateToStockPair( sKey, value ) );
				}
			}
		}		
	}
	return ( true );
}

void GoStore :: HandleMessage( const WorldMessage& msg )
{
	gpassert( !msg.IsCC() );

	if ( msg.GetEvent() == WE_ENTERED_WORLD && !Services::IsEditor() )
	{
		Restock();
	}
}

DWORD GoStore :: GoStoreToNet( GoStore* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoStore* GoStore :: NetToGoStore( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetStore() : NULL );
}

//////////////////////////////////////////////////////////////////////////////
