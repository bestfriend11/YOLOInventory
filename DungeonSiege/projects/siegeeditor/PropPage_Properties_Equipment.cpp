// PropPage_Properties_Equipment.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Object_Property_Sheet.h"
#include "PropPage_Properties_Equipment.h"
#include "GoInventory.h"
#include "Preferences.h"
#include "EditorObjects.h"
#include "Server.h"
#include "Player.h"
#include "Dialog_Item_Viewer.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Equipment property page

IMPLEMENT_DYNCREATE(PropPage_Properties_Equipment, CResizablePage)

PropPage_Properties_Equipment::PropPage_Properties_Equipment() : CResizablePage(PropPage_Properties_Equipment::IDD)
{
	m_bApply = false;
	//{{AFX_DATA_INIT(PropPage_Properties_Equipment)
	//}}AFX_DATA_INIT
}

PropPage_Properties_Equipment::~PropPage_Properties_Equipment()
{
}

void PropPage_Properties_Equipment::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Properties_Equipment)
	DDX_Control(pDX, IDC_COMBO_SELECTED, m_selected_slot);
	DDX_Control(pDX, IDC_EDIT_DOCUMENTATION, m_documentation);
	DDX_Control(pDX, IDC_EDIT_SCREENNAME, m_screen_name);
	DDX_Control(pDX, IDC_EDIT_FUELNAME, m_fuel_name);
	DDX_Control(pDX, IDC_COMBO_ITEMS, m_items);
	DDX_Control(pDX, IDC_COMBO_EQUIPSLOT, m_equip_slot);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Properties_Equipment, CResizablePage)
	//{{AFX_MSG_MAP(PropPage_Properties_Equipment)
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_COMBO_EQUIPSLOT, OnEditchangeComboEquipslot)
	ON_CBN_SELCHANGE(IDC_COMBO_ITEMS, OnEditchangeComboItems)
	ON_BN_CLICKED(IDC_RADIO_TEMPLATE, OnRadioTemplate)
	ON_BN_CLICKED(IDC_RADIO_SCREEN_NAME, OnRadioScreenName)
	ON_BN_CLICKED(IDC_RADIO_DOCUMENTATION, OnRadioDocumentation)
	ON_CBN_SELCHANGE(IDC_COMBO_SELECTED, OnSelchangeComboSelected)
	ON_BN_CLICKED(IDC_BUTTON_ADVANCED, OnButtonAdvanced)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Equipment message handlers

void PropPage_Properties_Equipment::OnSize(UINT nType, int cx, int cy) 
{
	CResizablePage::OnSize(nType, cx, cy);
	
	// TODO: Add your message handler code here
	
}

BOOL PropPage_Properties_Equipment::OnInitDialog() 
{
	CResizablePage::OnInitDialog();

	AddAnchor( IDC_EDIT_SCREENNAME,		MIDDLE_CENTER );
	AddAnchor( IDC_EDIT_FUELNAME,		MIDDLE_CENTER );
	AddAnchor( IDC_COMBO_ITEMS,			MIDDLE_CENTER );
	AddAnchor( IDC_COMBO_EQUIPSLOT,		MIDDLE_CENTER );
	AddAnchor( IDC_EDIT_DOCUMENTATION,	MIDDLE_CENTER );
	AddAnchor( IDC_RADIO_TEMPLATE,		MIDDLE_CENTER );
	AddAnchor( IDC_RADIO_SCREEN_NAME,	MIDDLE_CENTER );
	AddAnchor( IDC_RADIO_DOCUMENTATION, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_EQUIPSLOT,	MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_EQUIPPED,		MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_ITEMS,		MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_NAME,			MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_SCREEN_NAME,	MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_DOCUMENTATION,MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_SELECTED,		MIDDLE_CENTER );
	AddAnchor( IDC_COMBO_SELECTED,		MIDDLE_CENTER );
	AddAnchor( IDC_BUTTON_ADVANCED,		MIDDLE_CENTER );

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
		
		for ( int i = (int)IL_ACTIVE_MELEE_WEAPON; i != (int)IL_ALL_SPELLS; ++i )
		{
			m_selected_slot.AddString( ::ToString( (eInventoryLocation)i ) );
		}

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
		
	Reset();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void PropPage_Properties_Equipment::OnEditchangeComboEquipslot() 
{
	int index = m_equip_slot.GetCurSel();
	if ( index == CB_ERR ) {
		return;
	}

	CString rString;
	m_equip_slot.GetLBText( index, rString );
	m_items.ResetContent();
	
	eEquipSlot slot = ES_NONE;
	ComponentVec item_vec;
	bool bSpell = false;
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
		gComponentList.SearchAndRetrieveContentNames( item_vec, "spell", "", "", "" );		
		slot = ES_NONE;
		bSpell = true;
	}
	else if ( rString == "secondary_spell" ) 
	{
		gComponentList.SearchAndRetrieveContentNames( item_vec, "spell", "", "", "" );
		slot = ES_NONE;
		bSpell = true;
	}

	gEditorObjects.SetItemViewerFilter( slot, bSpell );
	
	EquipMap::iterator iSlot = m_equip_map.find( rString.GetBuffer( rString.GetLength() ) );
	gpstring sEquippedTemplate;
	if ( iSlot != m_equip_map.end() ) 
	{
		sEquippedTemplate = (*iSlot).second;
	}

	ComponentVec::iterator i;	
	int selection = -1;
	
	m_items.AddString( "" );	
	for ( i = item_vec.begin(); i != item_vec.end(); ++i ) 
	{
		int newItem = m_items.AddString( (*i).sName.c_str() );
		int id = 0;
		gComponentList.GetMapIdFromTemplateName( (*i).sFuelName, id );
		m_items.SetItemData( newItem, id );		
	}	

	if ( selection != CB_ERR ) 
	{
		DWORD id = m_items.GetItemData( selection );
		gpstring sTemplate;
		gComponentList.GetTemplateNameFromMapId( id, sTemplate );

		ObjectComponent oc = gComponentList.GetObjectComponent( sTemplate );
		
		m_fuel_name.SetWindowText( oc.sFuelName );
		m_screen_name.SetWindowText( oc.sScreenName );
		m_documentation.SetWindowText( oc.sDevName );
	}
	else 
	{	
		ObjectComponent oc = gComponentList.GetObjectComponent( sEquippedTemplate );
		m_fuel_name.SetWindowText( sEquippedTemplate.c_str() );
		m_screen_name.SetWindowText( oc.sScreenName );
		m_documentation.SetWindowText( oc.sDevName );
	}	
}

void PropPage_Properties_Equipment::OnEditchangeComboItems() 
{
	int index = m_equip_slot.GetCurSel();
	if ( index == CB_ERR ) {
		return;
	}

	CString rString;
	m_equip_slot.GetLBText( index, rString );
	if ( m_items.GetCurSel() == CB_ERR ) 
	{
		return;
	}

	CString selected_template;
	m_items.GetLBText( m_items.GetCurSel(), selected_template );
	
	{
		DWORD id = m_items.GetItemData( m_items.GetCurSel() );
		gpstring sTemplate;
		gComponentList.GetTemplateNameFromMapId( id, sTemplate );
		ObjectComponent oc = gComponentList.GetObjectComponent( sTemplate );
		selected_template = oc.sFuelName.c_str();
	}
		
	
	EquipMap::iterator iSlot = m_equip_map.find( rString.GetBuffer( rString.GetLength() ) );
	if ( iSlot != m_equip_map.end() ) 
	{
		(*iSlot).second = selected_template.GetBuffer( 0 );

		int selection = m_items.GetCurSel();
		if ( selection != CB_ERR ) 
		{
			DWORD id = m_items.GetItemData( selection );
			gpstring sTemplate;
			gComponentList.GetTemplateNameFromMapId( id, sTemplate );

			ObjectComponent oc = gComponentList.GetObjectComponent( sTemplate );
			
			m_fuel_name.SetWindowText( oc.sFuelName );
			m_screen_name.SetWindowText( oc.sScreenName );
			m_documentation.SetWindowText( oc.sDevName );
		}
		else 
		{
			m_fuel_name.SetWindowText( "" );
			m_screen_name.SetWindowText( "" );
			m_documentation.SetWindowText( "" );
		}
	}
	
	m_bApply = true;
	gObjectProperties.Evaluate();
}


void PropPage_Properties_Equipment::Reset()
{	
	if ( !GetSafeHwnd() )
	{
		return;
	}

	std::vector< DWORD > objects;
	std::vector< DWORD >::iterator j;
	Scid equipped_scid = SCID_INVALID;
	gGizmoManager.GetSelectedObjectIDs( objects );
	for ( j = objects.begin(); j != objects.end(); ++j ) {
		GoHandle hObject( MakeGoid( *j ) );
		if ( hObject.IsValid() && hObject->HasInventory() ) {

			gpstring sSelected = ::ToString( hObject->GetInventory()->GetSelectedLocation() );
			int found = m_selected_slot.FindString( 0, sSelected.c_str() );
			if ( found != CB_ERR )
			{
				m_selected_slot.SetCurSel( found );
			}

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

	m_bApply = false;
	gObjectProperties.Evaluate();
}


void PropPage_Properties_Equipment::Apply()
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

			if ( m_selected_slot.GetCurSel() != CB_ERR )
			{
				CString rSlot;
				m_selected_slot.GetLBText( m_selected_slot.GetCurSel(), rSlot );
				eInventoryLocation il;
				::FromString( rSlot.GetBuffer( 0 ), il );
				hObject->GetInventory()->RSSelect( il, AO_REFLEX );
			}

			GopColl items;
			GopColl::iterator k;
			hObject->GetInventory()->ListItems( IL_ALL, items );
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

			hObject->GetInventory()->UnequipAll( AO_REFLEX );		
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

				hObject->GetInventory()->Add( hItem, IL_MAIN, AO_REFLEX, false );				
				
				if( slot != ES_NONE ) {
					
					Go * pGo = hObject->GetInventory()->GetEquipped( slot );
					if ( pGo )
					{
						hObject->GetInventory()->RSUnequip( slot, AO_REFLEX );
						gEditorObjects.DeleteObject( pGo->GetGoid() );
					}

					hObject->GetInventory()->RSEquip( slot, hItem->GetGoid(), AO_REFLEX );					
				}					
				else if ( (*iSlot).first.same_no_case( "primary_spell" ) ) {
					if ( hItem ) {						
						hObject->GetInventory()->RSSetLocation( hItem, IL_ACTIVE_PRIMARY_SPELL, false ); 						
					}
				}
				else if ( (*iSlot).first.same_no_case( "secondary_spell" ) ) {
					if ( hItem ) {						
						hObject->GetInventory()->RSSetLocation( hItem, IL_ACTIVE_SECONDARY_SPELL, false ); 						
					}
				}
			}

			hObject->GetBody()->Update( 0 );
		}		
	}

	m_bApply = false;
}


void PropPage_Properties_Equipment::Cancel() 
{
}

void PropPage_Properties_Equipment::OnRadioTemplate() 
{
	if ( ((CButton *)GetDlgItem(IDC_RADIO_TEMPLATE))->GetCheck() )
	{
		gPreferences.SetContentDisplay( BLOCK_NAME );
		((CButton *)GetDlgItem(IDC_RADIO_SCREEN_NAME))->SetCheck( FALSE );
		((CButton *)GetDlgItem(IDC_RADIO_DOCUMENTATION))->SetCheck( FALSE );
		OnEditchangeComboEquipslot();
	}	
}

void PropPage_Properties_Equipment::OnRadioScreenName() 
{
	if ( ((CButton *)GetDlgItem(IDC_RADIO_SCREEN_NAME))->GetCheck() )
	{
		gPreferences.SetContentDisplay( SCREEN_NAME );
		((CButton *)GetDlgItem(IDC_RADIO_TEMPLATE))->SetCheck( FALSE );
		((CButton *)GetDlgItem(IDC_RADIO_DOCUMENTATION))->SetCheck( FALSE );
		OnEditchangeComboEquipslot();
	}	
}

void PropPage_Properties_Equipment::OnRadioDocumentation() 
{
	if ( ((CButton *)GetDlgItem(IDC_RADIO_DOCUMENTATION))->GetCheck() )
	{
		gPreferences.SetContentDisplay( DEV_NAME );
		((CButton *)GetDlgItem(IDC_RADIO_TEMPLATE))->SetCheck( FALSE );
		((CButton *)GetDlgItem(IDC_RADIO_SCREEN_NAME))->SetCheck( FALSE );
		OnEditchangeComboEquipslot();
	}	
}

void PropPage_Properties_Equipment::OnSelchangeComboSelected() 
{
	m_bApply = true;	
}

BOOL PropPage_Properties_Equipment::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR * pNm = (NMHDR *)lParam;
	if ( pNm->code == 0xffffff37 )
	{
		if ( gObjectProperties.Evaluate() )
		{
			int result = MessageBoxEx( this->GetSafeHwnd(), "You have changed the properties on this page.  Would you like to apply the changes?", "Apply Changes?", MB_YESNO | MB_ICONQUESTION, LANG_ENGLISH );
			if ( result == IDYES ) 
			{
				gObjectProperties.Apply();
			}	
		}
	}
	
	return CResizablePage::OnNotify(wParam, lParam, pResult);
}

void PropPage_Properties_Equipment::OnButtonAdvanced() 
{
	Dialog_Item_Viewer dlg;
	if ( dlg.DoModal() == IDOK )
	{
		int sel = m_items.FindString( 0, gEditorObjects.GetViewerSelectedTemplate() );
		/*
		if ( sel == CB_ERR )
		{
			sel = m_items.AddString( gEditorObjects.GetViewerSelectedTemplate() );
		}
		*/
		m_items.SetCurSel( sel );
		OnEditchangeComboItems();
	}	
}
