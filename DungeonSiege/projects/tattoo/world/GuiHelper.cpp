//////////////////////////////////////////////////////////////////////////////
//
// File     :  GuiHelper.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "precomp_world.h"
#include "GuiHelper.h"
#include "FileSys.h"
#include "ui_shell.h"
#include "ui_item.h"
#include "ui_itemslot.h"
#include "ui_gridbox.h"
#include "Go.h"
#include "NamingKey.h"
#include "GoActor.h"
#include "GoCore.h"
#include "GoInventory.h"
#include "GoMagic.h"
#include "GoShmo.h"
#include "AppModule.h"


// Gets an item pointer from the GameGUI for a particular game object
UIItem * GetItemFromGO( Go * pGO, bool bActive, bool bPortrait )
{
	if ( !pGO || !( pGO->HasActor() || pGO->HasPotion() || pGO->HasGui() ) )
	{
		return 0;
	}

	UIItem *pItem = 0;
	gpstring sGUID;
	sGUID.assignf( "_%d", MakeInt( pGO->GetGoid() ) );
	
	if ( pGO->HasActor() && bPortrait && !pGO->GetActor()->GetPortraitIcon().empty() )
	{
		pItem = (UIItem *)gUIShell.FindUIWindow( pGO->GetActor()->GetPortraitIcon() + sGUID, "icons" );			
	}	
	else if ( pGO->HasPotion() && !pGO->GetPotion()->GetInventoryIcon2().empty() )
	{
		pItem = (UIItem *)gUIShell.FindUIWindow( pGO->GetPotion()->GetInventoryIcon2() + sGUID, "icons" );		

		if ( !gUIShell.GetIgnoreItemTexture() ) 
		{
			if ( pItem && pItem->GetTextureIndex() == 0 )
			{
				bool bSuccess = true;
				gpstring sTexture = GetTextureStringFromIcon( pGO, pGO->GetGui()->GetInventoryIcon(), bSuccess );				
				pItem->LoadTexture( sTexture, true );
				if ( pGO->HasPotion() && !pGO->GetPotion()->GetInventoryIcon2().empty() )
				{
					pItem->LoadSecondaryTexture( pGO->GetPotion()->GetInventoryIcon2() );		
					pItem->SetSecondaryPercentVisible( pGO->GetMagic()->GetPotionFullRatio() );
				}
			}
		}
	}
	else 
	{
		pItem = (UIItem *)gUIShell.FindUIWindow( pGO->GetGui()->GetInventoryIcon() + sGUID, "icons" );	
		if ( !gUIShell.GetIgnoreItemTexture() ) 
		{
			if ( pItem && pItem->GetTextureIndex() == 0 )
			{
				bool bSuccess = true;
				gpstring sTexture = GetTextureStringFromIcon( pGO, pGO->GetGui()->GetInventoryIcon(), bSuccess );
				pItem->LoadTexture( sTexture, true );
			}
		}
	}
	if ( !pItem ) 
	{	
		GUInterface *pInterface = gUIShell.FindInterface( "icons" );
		if ( !pInterface )
		{
			gUIShell.ActivateInterface( "ui:interfaces:backend:icons", true );
			pInterface = gUIShell.FindInterface( "icons" );
		}

		FastFuelHandle fhItem( "UI:Interfaces:Backend:icons:icons:template" );
		
		UIWindow *pWindow = 0;
		if ( pGO->HasActor() && bPortrait )
		{
			pWindow = gUIShell.Instantiate( fhItem, pInterface, 0, pGO->GetActor()->GetPortraitIcon() + sGUID );
		}
		else if ( pGO->HasPotion() && !pGO->GetPotion()->GetInventoryIcon2().empty() )
		{		
			pWindow = gUIShell.Instantiate( fhItem, pInterface, 0, pGO->GetPotion()->GetInventoryIcon2() + sGUID );
		}
		else 
		{
			pWindow = gUIShell.Instantiate( fhItem, pInterface, 0, pGO->GetGui()->GetInventoryIcon() + sGUID );			
		}
		bool bSuccess = true;
		
		pItem = (UIItem *)pWindow;
		if ( !gUIShell.GetIgnoreItemTexture() ) 
		{
			if ( pGO->HasActor() && bPortrait )
			{				
				DWORD portraitTex = pGO->GetActor()->GetPortraitTexture();
				if ( portraitTex )
				{
					// note: the actor will destroy its own portrait as well, so
					// need to addref on it.
					gUIShell.GetRenderer().AddTextureReference( portraitTex );
					pItem->SetTextureIndex( portraitTex );
				}
				else
				{
					gpstring sTexture = GetTextureStringFromIcon( pGO, pGO->GetActor()->GetPortraitIcon(), bSuccess );				
					pItem->LoadTexture( sTexture, true );
				}
			}
			else
			{
				gpstring sTexture = GetTextureStringFromIcon( pGO, pGO->GetGui()->GetInventoryIcon(), bSuccess );
				
				pItem->LoadTexture( sTexture, true );
				if ( pGO->HasPotion() && !pGO->GetPotion()->GetInventoryIcon2().empty() )
				{
					pItem->LoadSecondaryTexture( pGO->GetPotion()->GetInventoryIcon2() );		
					pItem->SetSecondaryPercentVisible( pGO->GetMagic()->GetPotionFullRatio() );
				}
			}		
		}
		
		// Get Item type
		if ( pGO->HasActor() && bPortrait ) 
		{
			pItem->SetItemType( "portrait" );
		}
		else if ( pGO->HasGui() ) 
		{	
			pItem->SetItemType( "inventory" );
			pItem->SetRows( pGO->GetGui()->GetInventoryHeight() );
			pItem->SetColumns( pGO->GetGui()->GetInventoryWidth() );
		}		
		else 
		{
			return 0;
		}

		// All gui items will be designed for 640 x 480
		pItem->ResizeToCurrentResolution( 640, 480 );
		
		// Get the slot type		
		pItem->SetSlotType( GetSlotName( pGO ) );		
		pItem->SetMousePos( gAppModule.GetCursorX(), gAppModule.GetCursorY() );	
	}
	
	if ( pItem )
	{
		pItem->SetItemID( MakeInt( pGO->GetGoid() ) );
	}

	if (( !pItem->GetItemType().same_no_case( "portrait" ) ) && ( bActive )) 
	{
		pItem->ActivateItem( true, false );		
	}	

	return pItem;
}


// Build up a texture location from an icon name using the Naming Key
gpstring GetTextureStringFromIcon( Go * pGO, gpstring sIcon, bool & bSuccess )
{
	gpstring filename;

	bSuccess = true;
	if (!gNamingKey.BuildIMGLocation(sIcon.c_str(), filename) || !FileSys::DoesResourceFileExist( filename.c_str() ) ) {
		// Perhaps we should load a 'special' icon to indicate failure? --biddle
		gpwarningf(("Failed to locate Icon texture: %s | For game object: %S | Scid: 0x%x",sIcon.c_str(), pGO->GetCommon()->GetScreenName().c_str(), MakeInt(pGO->GetScid()) )); 
		gNamingKey.BuildIMGLocation("b_gui_s_notfound", filename);
		bSuccess = false;
	}

	return filename;
}


// Get an item's slot type
gpstring GetSlotName( Go * pGO )
{
	if ( pGO->HasGui() )
	{
		if ( pGO->GetGui()->GetEquipSlot() == ES_FEET ) {
			return "boots";
		}
		else if ( pGO->IsMeleeWeapon() ) 
		{
			return "melee_weapon";	
		}
		else if ( pGO->IsRangedWeapon() ) 
		{
			return "ranged_weapon";	
		}		
		else if ( pGO->IsShield() ) 
		{
			return "shield";
		}
		else if ( pGO->GetGui()->GetEquipSlot() == ES_FOREARMS ) 
		{
			return "gauntlets";
		}
		else if ( pGO->GetGui()->GetEquipSlot() == ES_CHEST ) 
		{
			return "armor";	
		}
		else if ( pGO->GetGui()->GetEquipSlot() == ES_HEAD ) 
		{
			return "helmet";
		}
		else if ( pGO->GetGui()->GetEquipSlot() == ES_AMULET )
		{
			return "amulet";
		}
		else if ( pGO->GetGui()->GetEquipSlot() == ES_RING )
		{
			return "ring";
		}		
		else if ( pGO->GetGui()->GetEquipSlot() == ES_SPELLBOOK )
		{
			return "spellbook";
		}
	}

	if ( pGO->HasMagic() )
	{
		if ( pGO->GetMagic()->IsSpell() ) {
			return "scroll";
		}
		if ( pGO->GetMagic()->IsPotion() ) {
			return "potion";
		}
	}	
	
	if ( pGO->HasActor() ) {
		return "picture";
	}

	return "item";
}


UIGridbox * GetGridboxFromGO( Go * pGO, int index )
{
	UIGridbox *pGridbox = 0;
	char szGridName[256] = "";
	sprintf( szGridName, "char_%d_gridbox", MakeInt( pGO->GetGoid() ) );
	pGridbox = (UIGridbox *)gUIShell.FindUIWindow( szGridName );
	if ( pGridbox ) {
		return pGridbox;
	}

	char szGridboxParentName[256] = "";
	GoInventory *pInventory = pGO->GetInventory();
	if( pInventory == NULL ) {
		return 0;
	}
	sprintf( szGridboxParentName, "gridbox_%dx%d", pInventory->GetGridHeight(), pInventory->GetGridWidth() );
	UIGridbox *pParentGrid = (UIGridbox *)gUIShell.FindUIWindow( szGridboxParentName );
	if ( !pParentGrid ) {
		return 0;
	}

	GUInterface *pInterface = gUIShell.FindInterface( "inventory" );
	if ( !pInterface ) {
		return 0;
	}

	FastFuelHandle fhBox( "UI:Interfaces:Backend:inventory:inventory:gridbox_template" );
	UIWindow *pWindow = gUIShell.Instantiate( fhBox, pInterface, 0, szGridName );
	pGridbox = (UIGridbox *)pWindow;
	pGridbox->SetColumns( pParentGrid->GetColumns() );
	pGridbox->SetRows( pParentGrid->GetRows() );
	pGridbox->SetBoxHeight( pParentGrid->GetBoxHeight() );
	pGridbox->SetBoxWidth( pParentGrid->GetBoxWidth() );
	pGridbox->SetGridType( pParentGrid->GetGridType() );
	pGridbox->SetPulloutOffset( pParentGrid->GetPulloutOffset() );
	pGridbox->LoadTexture( pParentGrid->GetTextureIndex() );
	pGridbox->SetHasTexture( pParentGrid->HasTexture() );
	pGridbox->SetIndex( index );
	pGridbox->SetVisible( true );
	pGridbox->SetRect(	pParentGrid->GetRect().left, pParentGrid->GetRect().right,
						pParentGrid->GetRect().top, pParentGrid->GetRect().bottom );
	pGridbox->SetUVRect(	pParentGrid->GetUVRect().left, pParentGrid->GetUVRect().right,
							pParentGrid->GetUVRect().top, pParentGrid->GetUVRect().bottom );
	pGridbox->SetTiledTexture( pParentGrid->GetTiledTexture() );

	pGridbox->SetName( szGridName );
	pGridbox->SetConsumable( pParentGrid->IsConsumable() );
	pGridbox->ResetGrid();
	pGridbox->SetDrawOrder( pWindow->GetDrawOrder() );

	return pGridbox;
}


bool ExtractFirstTagFromString( gpwstring & sString, gpwstring & sTag )
{
	unsigned int macroBegin = sString.find( L'<' );
	unsigned int macroEnd	= sString.find( L'>' );
	if ( macroBegin != gpwstring::npos && macroEnd != gpwstring::npos )
	{
		sTag = sString.substr( macroBegin+1, (macroEnd-macroBegin)-1 );

		gpwstring sBegin	= sString.substr( 0, macroBegin );
		gpwstring sEnd		= sString.substr( macroEnd+1, sString.size() );		

		sString = sBegin;
		sString += sEnd;

		return true;
	}

	return false;
}
