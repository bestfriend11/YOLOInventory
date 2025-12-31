//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoStash.cpp
// Author(s):  Chad Queen
//
// Copyright © 2002 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "Precomp_World.h"
#include "GoStash.h"
#include "GoInventory.h"
#include "GoAspect.h"
#include "GoData.h"
#include "ui_shell.h"
#include "ui_gridbox.h"
#include "World.h"
#include "ContentDb.h"
#include "GuiHelper.h"


DECLARE_GO_COMPONENT( GoStash, "stash" );


GoStash :: GoStash( Go* parent )
	: Inherited( parent )
	, m_StashActivator	( GOID_INVALID )
	, m_StashMember		( GOID_INVALID )
	, m_bStashTransfer	( false )
{
}

GoStash :: GoStash( const GoStash& source, Go* parent )
	: Inherited( source, parent )	
	, m_StashActivator	( source.m_StashActivator )
	, m_StashMember		( source.m_StashMember )
	, m_bStashTransfer	( source.m_bStashTransfer )
{
}

GoStash :: ~GoStash( void )
{
	// this space intentionally left blank...
}

void GoStash :: Init( void )
{
	// Let's assign it the proper gridbox now.
	UIGridbox * pGrid = (UIGridbox *)gUIShell.FindUIWindow( "gridbox_stash", "party_stash" );
	if ( pGrid )
	{	
		GetGo()->GetInventory()->SetGridbox( pGrid );
	}
}

void GoStash :: ActivateStash( Goid member, Goid stashActivator )
{	
	GoHandle hMember( member );
	if ( hMember && hMember->GetPlayer() )
	{
		GoHandle hStash( hMember->GetPlayer()->GetStash() );
		if ( hStash )
		{
			m_StashMember		= member;
			m_StashActivator	= stashActivator;
		}
		else
		{
			gperror( "The stash is invalid - you should not have gotten this far. [CQ]" );
		}
	}
	else
	{
		// Stash is invalid, gotta go.
		gperror( "The member is invalid or does not have a player - you should not have gotten this far. [CQ]" );
		return;
	}	

	// Now let's set up the stash interface
	gUIShell.ShowInterface( "party_stash" );
	UIGridbox * pGrid = (UIGridbox *)gUIShell.FindUIWindow( "gridbox_stash", "party_stash" );
	if ( !pGrid )
	{
		gperror( "The gridbox stash does not exist - do you have the latest data (party_stash.gas)? [CQ]" );
		return;
	}	
	else
	{
		GetGo()->GetInventory()->SetGridbox( pGrid );
	}

	RSSetStashMemberAndActivator( member, stashActivator );
	
	RefreshStash();
}

void GoStash :: RefreshStash()
{
	UIGridbox * pGrid = GetGo()->GetInventory()->GetGridbox();
	if ( !pGrid )
	{
		gperror( "The gridbox stash does not exist - you should not have gotten this far [CQ]" );
		return;
	}
	
	pGrid->SetVisible( true );

	UIWindowVec gridboxes = gUIShell.ListWindowsOfType( UI_TYPE_GRIDBOX );
	UIWindowVec::iterator k;
	for ( k = gridboxes.begin(); k != gridboxes.end(); ++k )
	{
		((UIGridbox *)(*k))->SetItemDetect( true );
	}		
}

void GoStash :: CloseStash()
{
	gUIShell.HideInterface( "party_stash" );
	m_StashActivator	= GOID_INVALID;
	m_StashMember		= GOID_INVALID;		
}

void GoStash :: RSAddToStash( Goid member, Goid item )
{
	FUBI_RPC_TAG(); 

	SetStashTransfer( true );
	
	FUBI_RPC_THIS_CALL( RSAddToStash, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	GoHandle hItem( item );
	GoHandle hMember( member );

	if ( hItem && hMember )
	{	
		if ( hItem->HasGold() )
		{
			int gold = GetGo()->GetInventory()->GetGold() + hItem->GetAspect()->GetGoldValue();
			if ( gold > gContentDb.GetMaxPartyGold() )
			{
				GetGo()->GetInventory()->SSetGold( gContentDb.GetMaxPartyGold() );
				hMember->GetInventory()->SSetGold( (hMember->GetInventory()->GetGold() - hItem->GetAspect()->GetGoldValue()) + (gold - gContentDb.GetMaxPartyGold()) );
				hItem->GetAspect()->SSetGoldValue( gold - gContentDb.GetMaxPartyGold(), false );				
			}
			else
			{
				GetGo()->GetInventory()->SSetGold( gold );
				hMember->GetInventory()->SSetGold( hMember->GetInventory()->GetGold() - hItem->GetAspect()->GetGoldValue() );
				hItem->GetAspect()->SSetGoldValue( 0, false );				
			}
		}
		else
		{
			hMember->GetInventory()->RSTransfer( hItem, GetGo(), IL_MAIN, AO_REFLEX );

			// Let's set the position to nothing since the item is now in the "stash" ( it's an omni go )
			SiegePos oldPos = hItem->GetPlacement()->GetPosition();
			hItem->GetPlacement()->SSetPosition( SiegePos::INVALID );		
		}

		if ( hMember->GetPlayer() )
		{
			RCAddToStash( item, hMember->GetPlayer()->GetMachineId() );
		}
	}	
}

void GoStash :: RCAddToStash( Goid item, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCAddToStash, machineId );	

	// Handle gold case 
	GoHandle hItem( item );
	if ( hItem && hItem->HasGold() )
	{		
		GetGo()->GetInventory()->GetGridbox()->RemoveID( MakeInt( hItem->GetGoid() ) );

		// activate the gold if it is not 0 ( the player couldn't fit it all in their stash
		if ( hItem->GetAspect()->GetGoldValue() != 0 )
		{
			GetItemFromGO( hItem, true, false );			
		}
	}

	SetStashTransfer( false );
	RefreshStash();
}

void GoStash :: RSRemoveFromStash( Goid member, Goid item )
{
	FUBI_RPC_TAG(); 

	SetStashTransfer( true );

	FUBI_RPC_THIS_CALL( RSRemoveFromStash, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	GoHandle hItem( item );
	GoHandle hMember( member );

	if ( hItem && hMember )
	{	
		if ( hItem->HasGold() )
		{
			int gold = hMember->GetInventory()->GetGold() + hItem->GetAspect()->GetGoldValue();			
			if ( gold > gContentDb.GetMaxPartyGold() )
			{
				hMember->GetInventory()->SSetGold( gContentDb.GetMaxPartyGold() );
				GetGo()->GetInventory()->SSetGold( GetGo()->GetInventory()->GetGold() + (gold-gContentDb.GetMaxPartyGold()) );				
			}
			else
			{
				hMember->GetInventory()->SSetGold( gold );
				GetGo()->GetInventory()->SSetGold( GetGo()->GetInventory()->GetGold() - hItem->GetAspect()->GetGoldValue() );				
			}		
		}
		else
		{
			SDisableOmniStatus( item );
			GetGo()->GetInventory()->RSTransfer( hItem, hMember, IL_MAIN, AO_REFLEX );											
		}	

		if ( hMember->GetPlayer() )
		{
			RCRemoveFromStash( item, hMember->GetPlayer()->GetMachineId() );
		}
	}	
}

void GoStash :: RCRemoveFromStash( Goid item, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCRemoveFromStash, machineId );
	SetStashTransfer( false );

	// Handle gold case 
	GoHandle hItem( item );
	if ( hItem && hItem->HasGold() )
	{	
		// activate the gold if it is not 0 ( the player couldn't fit it all in their stash
		if ( hItem->GetAspect()->GetGoldValue() != 0 )
		{
			GetItemFromGO( hItem, true, false );			
		}
	}
	else
	{
		gWorld.StashGuiCallback( GetStashMember(), item );
	}

	RefreshStash();
}


void GoStash :: SDisableOmniStatus( Goid item )
{
	CHECK_SERVER_ONLY;

	RCDisableOmniStatus( item );
}


void GoStash :: RCDisableOmniStatus( Goid item )
{
	FUBI_RPC_THIS_CALL( RCDisableOmniStatus, RPC_TO_ALL );

	// demote to omni status if necessary
	GoHandle hItem( item );
	if ( hItem && hItem->IsOmni() )
	{
		hItem->SetIsOmni( false );
	}
}


void GoStash :: RSSetStashMemberAndActivator( Goid member, Goid stashActivator )
{
	FUBI_RPC_THIS_CALL( RSSetStashMemberAndActivator, RPC_TO_SERVER );

	m_StashMember		= member;
	m_StashActivator	= stashActivator;
}


float GoStash :: GetActivateRange( void ) const
{
	static AutoConstQuery <float> s_Query( "activate_range" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}


GoComponent* GoStash :: Clone( Go* newParent )
{
	return ( new GoStash( *this, newParent ) );
}

bool GoStash :: Xfer( FuBi::PersistContext& persist )
{
	if ( persist.IsFullXfer() )
	{
		persist.Xfer( "m_StashActivator", m_StashActivator );
		persist.Xfer( "m_StashMember",    m_StashMember    );
		persist.Xfer( "m_bStashTransfer", m_bStashTransfer );

		if ( persist.IsRestoring() )
		{
			Init();
		}
	}	

	return ( true );
}

DWORD GoStash :: GoStashToNet( GoStash* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoStash* GoStash :: NetToGoStash( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetStash() : NULL );
}
