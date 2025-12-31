// Dialog_Object_Equipment.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "SiegeEditor.h"
#include "Dialog_Object_Equipment.h"
#include "ScidManager.h"
#include "ComponentList.h"
#include "Go.h"
#include "ContentDb.h"
#include "GizmoManager.h"
#include "GoInventory.h"
#include "Server.h"
#include "GoCore.h"
#include "EditorObjects.h"
#include "Player.h"
#include "GoDb.h"
#include "GoBody.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Equipment dialog


Dialog_Object_Equipment::Dialog_Object_Equipment(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Object_Equipment::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Object_Equipment)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Object_Equipment::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Object_Equipment)
	DDX_Control(pDX, IDC_EDIT_SCREENNAME, m_screen_name);	
	DDX_Control(pDX, IDC_EDIT_FUELNAME, m_fuel_name);
	DDX_Control(pDX, IDC_LIST_ITEMS, m_list_items);
	DDX_Control(pDX, IDC_COMBO_EQUIPSLOT, m_equip_slot);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Object_Equipment, CDialog)
	//{{AFX_MSG_MAP(Dialog_Object_Equipment)
	ON_NOTIFY(NM_CLICK, IDC_LIST_ITEMS, OnClickListItems)
	ON_CBN_SELCHANGE(IDC_COMBO_EQUIPSLOT, OnEditchangeComboEquipslot)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Equipment message handlers

void Dialog_Object_Equipment::OnClickListItems(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int index = m_equip_slot.GetCurSel();
	if ( index == CB_ERR ) {
		return;
	}

	CString rString;
	m_equip_slot.GetLBText( index, rString );	

	if ( m_list_items.GetSelectionMark() == -1 ) 
	{
		return;
	}

	CString selected_template = m_list_items.GetItemText( m_list_items.GetSelectionMark(), 0 );
	
	EquipMap::iterator iSlot = m_equip_map.find( rString.GetBuffer( rString.GetLength() ) );
	if ( iSlot != m_equip_map.end() ) {
		(*iSlot).second = selected_template.GetBuffer( 0 );

		int selection = m_list_items.GetSelectionMark();
		if ( selection != -1 ) {
			CString rFuelName = m_list_items.GetItemText( selection, 0 );
			CString rScreenName = m_list_items.GetItemText( selection, 1 );
			CString rScid = m_list_items.GetItemText( selection, 2 );
			m_fuel_name.SetWindowText( rFuelName );
			m_screen_name.SetWindowText( rScreenName );
		}
		else {
			m_fuel_name.SetWindowText( "" );
			m_screen_name.SetWindowText( "" );
		}
	}		
	
	*pResult = 0;
}

void Dialog_Object_Equipment::OnEditchangeComboEquipslot() 
{
	int index = m_equip_slot.GetCurSel();
	if ( index == CB_ERR ) {
		return;
	}

	CString rString;
	m_equip_slot.GetLBText( index, rString );
	m_list_items.DeleteAllItems();
	
	eEquipSlot slot = ES_NONE;
	ComponentVec item_vec;
	if ( rString == "es_weapon_hand" ) 
	{
		slot = ES_WEAPON_HAND;
		gComponentList.RetrieveEquipmentList( item_vec, slot );		
	}
	else if ( rString == "es_shield_hand" ) 
	{		
		slot = ES_SHIELD_HAND;
		gComponentList.RetrieveEquipmentList( item_vec, slot );		
	}
	else if ( rString == "es_forearms" ) 
	{		
		slot = ES_FOREARMS;
		gComponentList.RetrieveEquipmentList( item_vec, slot );		
	}
	else if ( rString == "es_feet" ) 
	{		
		slot = ES_FEET;
		gComponentList.RetrieveEquipmentList( item_vec, slot );		
	}
	else if ( rString == "es_chest" ) 
	{		
		slot = ES_CHEST;
		gComponentList.RetrieveEquipmentList( item_vec, slot );		
	}
	else if ( rString == "es_head" ) 
	{		
		slot = ES_HEAD;
		gComponentList.RetrieveEquipmentList( item_vec, slot );		
	}
	else if ( rString == "ring1" ) 
	{
		gComponentList.RetrieveEquipmentList( item_vec, ES_RING );		
		slot = ES_RING_0;
	}
	else if ( rString == "ring2" ) 
	{
		gComponentList.RetrieveEquipmentList( item_vec, ES_RING );
		slot = ES_RING_1;
	}
	else if ( rString == "ring3" ) 
	{
		gComponentList.RetrieveEquipmentList( item_vec, ES_RING );
		slot = ES_RING_2;
	}
	else if ( rString == "ring4" ) 
	{
		gComponentList.RetrieveEquipmentList( item_vec, ES_RING );
		slot = ES_RING_3;
	}
	else if ( rString == "es_amulet" )
	{
		slot = ES_AMULET;		
		gComponentList.RetrieveEquipmentList( item_vec, slot );		
	}
	else if ( rString == "primary_spell" ) 
	{
		gComponentList.SearchAndRetrieveContentNames( item_vec, "", "", "spell", "" );		
	}
	else if ( rString == "secondary_spell" ) 
	{
		gComponentList.SearchAndRetrieveContentNames( item_vec, "", "", "spell", "" );
	}
	
	EquipMap::iterator iSlot = m_equip_map.find( rString.GetBuffer( rString.GetLength() ) );
	gpstring sEquippedTemplate;
	if ( iSlot != m_equip_map.end() ) 
	{
		sEquippedTemplate = (*iSlot).second;
	}

	ComponentVec::iterator i;
	int list_index = 1;
	int selection = -1;
	
	m_list_items.InsertItem( 0, "nothing" );
	m_list_items.SetItem( 0, 1, LVIF_TEXT, "nothing", 0, 0, 0, 0 );
	
	for ( i = item_vec.begin(); i != item_vec.end(); ++i ) {
		m_list_items.InsertItem( list_index, (*i).sFuelName.c_str() );
		m_list_items.SetItem( list_index, 1, LVIF_TEXT, (*i).sScreenName.c_str(), 0, 0, 0, 0 );			
		++list_index;
	}	

	if ( selection != -1 ) {
		CString rFuelName = m_list_items.GetItemText( selection, 0 );
		CString rScreenName = m_list_items.GetItemText( selection, 1 );		
		
		m_fuel_name.SetWindowText( rFuelName );
		m_screen_name.SetWindowText( rScreenName );
	}
	else {	
		ObjectComponent oc = gComponentList.GetObjectComponent( sEquippedTemplate );
		m_fuel_name.SetWindowText( sEquippedTemplate.c_str() );
		m_screen_name.SetWindowText( oc.sScreenName );
	}
}

BOOL Dialog_Object_Equipment::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Insert all possible equipment slots
	{
		m_equip_slot.AddString( "es_weapon_hand" );
		m_equip_slot.AddString( "es_shield_hand" );
		m_equip_slot.AddString( "es_forearms" );
		m_equip_slot.AddString( "es_feet" );
		m_equip_slot.AddString( "es_chest" );
		m_equip_slot.AddString( "es_head" );
		m_equip_slot.AddString( "ring1" );
		m_equip_slot.AddString( "ring2" );
		m_equip_slot.AddString( "ring3" );
		m_equip_slot.AddString( "ring4" );
		m_equip_slot.AddString( "primary_spell" );
		m_equip_slot.AddString( "secondary_spell" );

		m_equip_map.insert( EquipPair( "es_weapon_hand", "" ) );
		m_equip_map.insert( EquipPair( "es_shield_hand", "" ) );
		m_equip_map.insert( EquipPair( "es_forearms", "" ) );
		m_equip_map.insert( EquipPair( "es_feet", "" ) );
		m_equip_map.insert( EquipPair( "es_chest", "" ) );
		m_equip_map.insert( EquipPair( "es_head", "" ) );
		m_equip_map.insert( EquipPair( "ring1", "" ) );
		m_equip_map.insert( EquipPair( "ring2", "" ) );
		m_equip_map.insert( EquipPair( "ring3", "" ) );
		m_equip_map.insert( EquipPair( "ring4", "" ) );
		m_equip_map.insert( EquipPair( "primary_spell", "" ) );
		m_equip_map.insert( EquipPair( "secondary_spell", "" ) );
	}

	m_list_items.InsertColumn( 0, "Template Name", LVCFMT_LEFT, 110 );
	m_list_items.InsertColumn( 1, "Screen Name", LVCFMT_LEFT, 120 );	
	
	std::vector< DWORD > objects;
	std::vector< DWORD >::iterator j;
	Scid equipped_scid = SCID_INVALID;
	gGizmoManager.GetSelectedObjectIDs( objects );
	for ( j = objects.begin(); j != objects.end(); ++j ) {
		GoHandle hObject( MakeGoid( *j ) );
		if ( hObject.IsValid() && hObject->HasInventory() ) {

			EquipMap::iterator iSlot;
			{
				iSlot = m_equip_map.find( "es_weapon_hand" );
				if ( iSlot != m_equip_map.end() ) {
					Go * pEquipped = hObject->GetInventory()->GetEquipped( ES_WEAPON_HAND );					
					if ( pEquipped ) {
						(*iSlot).second = pEquipped->GetTemplateName();
					}
				}
			}

			{
				iSlot = m_equip_map.find( "es_shield_hand" );
				if ( iSlot != m_equip_map.end() ) {
					Go * pEquipped = hObject->GetInventory()->GetEquipped( ES_SHIELD_HAND );					
					if ( pEquipped ) {
						(*iSlot).second = pEquipped->GetTemplateName();					
					}
				}
			}

			{
				iSlot = m_equip_map.find( "es_forearms" );
				if ( iSlot != m_equip_map.end() ) {
					Go * pEquipped = hObject->GetInventory()->GetEquipped( ES_FOREARMS );					
					if ( pEquipped ) {
						(*iSlot).second = pEquipped->GetTemplateName();					
					}
				}
			}

			{
				iSlot = m_equip_map.find( "es_feet" );
				if ( iSlot != m_equip_map.end() ) {
					Go * pEquipped = hObject->GetInventory()->GetEquipped( ES_FEET );					
					if ( pEquipped ) {
						(*iSlot).second = pEquipped->GetTemplateName();					
					}
				}
			}

			{
				iSlot = m_equip_map.find( "es_chest" );
				if ( iSlot != m_equip_map.end() ) {
					Go *pEquipped = hObject->GetInventory()->GetEquipped( ES_CHEST );					
					if ( pEquipped ) {
						(*iSlot).second = pEquipped->GetTemplateName();					
					}
				}
			}


			{
				iSlot = m_equip_map.find( "es_head" );
				if ( iSlot != m_equip_map.end() ) {
					Go *pEquipped = hObject->GetInventory()->GetEquipped( ES_HEAD );					
					if ( pEquipped ) {
						(*iSlot).second = pEquipped->GetTemplateName();					
					}
				}
			}


			{
				iSlot = m_equip_map.find( "primary_spell" );
				if ( iSlot != m_equip_map.end() ) {				
					Go * pEquipped = hObject->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL );					
					if ( pEquipped ) {
						(*iSlot).second = pEquipped->GetTemplateName();					
					}
				}
			}

			{
				iSlot = m_equip_map.find( "secondary_spell" );
				if ( iSlot != m_equip_map.end() ) {								
					Go * pEquipped = hObject->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL );					
					if ( pEquipped ) {
						(*iSlot).second = pEquipped->GetTemplateName();					
					}
				}
			}
		}
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Object_Equipment::OnOK() 
{
	{
		std::vector< DWORD > objects;
		std::vector< DWORD >::iterator j;	
		gGizmoManager.GetSelectedObjectIDs( objects );
		for ( j = objects.begin(); j != objects.end(); ++j ) {	

			GoHandle hObject( MakeGoid(*j) );
			if ( !hObject.IsValid() && !hObject->HasInventory() ) {
				continue;
			}

			GopColl items;
			GopColl::iterator k;
			hObject->GetInventory()->ListItems( IL_MAIN, items );
			for ( k = items.begin(); k != items.end(); ++k )
			{
				if ( hObject->GetInventory()->IsEquipped( *k ) )
				{
					hObject->GetInventory()->RSRemove( *k, false );
					gEditorObjects.DeleteObject( (*k)->GetGoid() );						
				}	

				if ( ( hObject->GetInventory()->GetLocation( *k ) == IL_ACTIVE_PRIMARY_SPELL ) ||
					 ( hObject->GetInventory()->GetLocation( *k ) == IL_ACTIVE_SECONDARY_SPELL ) )
				{
					hObject->GetInventory()->RSRemove( *k, false );
					gEditorObjects.DeleteObject( (*k)->GetGoid() );
				}
			}

			hObject->GetInventory()->UnequipAll();		
		}
	}

	EquipMap::iterator iSlot;
	for ( iSlot = m_equip_map.begin(); iSlot != m_equip_map.end(); ++iSlot ) {
		
		gpstring selected_template = (*iSlot).second;
		if ( selected_template.empty() ) 
		{
			continue;
		}	
	
		std::vector< DWORD > objects;
		std::vector< DWORD >::iterator j;	
		gGizmoManager.GetSelectedObjectIDs( objects );
		for ( j = objects.begin(); j != objects.end(); ++j ) {	

			GoHandle hObject( MakeGoid(*j) );
			if ( !hObject.IsValid() || !hObject->GetBody() ) {
				continue;
			}
			
			GoCloneReq createGo( (*iSlot).second, gServer.GetPlayer( ToUnicode( "human" ) )->GetId() );
			createGo.SetStartingPos( hObject->GetPlacement()->GetPosition() );	
			Goid item = gGoDb.SCloneGo( createGo );
			GoHandle hItem( item );				
			
			if (( hItem->IsInsideInventory() == false ) && hObject->HasInventory() ) {

				eEquipSlot slot = ES_NONE;
				FromString( (*iSlot).first.c_str(), slot );

				hObject->GetInventory()->Add( hItem, IL_MAIN, false );				
				
				if( slot != ES_NONE ) {
					
					Go * pGo = hObject->GetInventory()->GetEquipped( slot );
					if ( pGo )
					{
						hObject->GetInventory()->RCUnequip( slot );
						gEditorObjects.DeleteObject( pGo->GetGoid() );
					}

					hObject->GetInventory()->RCEquip( slot, hItem->GetGoid() );					
				}					
				else if ( (*iSlot).first.same_no_case( "primary_spell" ) ) {
					if ( hItem ) {						
						hObject->GetInventory()->SetLocation( hItem, IL_ACTIVE_PRIMARY_SPELL, false ); 						
					}
				}
				else if ( (*iSlot).first.same_no_case( "secondary_spell" ) ) {
					if ( hItem ) {						
						hObject->GetInventory()->SetLocation( hItem, IL_ACTIVE_SECONDARY_SPELL, false ); 						
					}
				}
			}

			hObject->GetBody()->Update( 0 );
		}		
	}
	
	CDialog::OnOK();
}
