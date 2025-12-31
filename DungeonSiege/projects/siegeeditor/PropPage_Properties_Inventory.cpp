// PropPage_Properties_Inventory.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Object_Property_Sheet.h"
#include "PropPage_Properties_Inventory.h"
#include "ComponentList.h"
#include "GoInventory.h"
#include "SCIDManager.h"
#include "Server.h"
#include "EditorObjects.h"
#include "Preferences.h"
#include "Player.h"
#include "Dialog_Item_Viewer.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Inventory property page

IMPLEMENT_DYNCREATE(PropPage_Properties_Inventory, CResizablePage)

PropPage_Properties_Inventory::PropPage_Properties_Inventory() : CResizablePage(PropPage_Properties_Inventory::IDD)
{
	m_bApply = false;
	//{{AFX_DATA_INIT(PropPage_Properties_Inventory)
	//}}AFX_DATA_INIT
}

PropPage_Properties_Inventory::~PropPage_Properties_Inventory()
{
}

void PropPage_Properties_Inventory::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Properties_Inventory)
	DDX_Control(pDX, IDC_LIST_CURRENT_INVENTORY, m_list_current);
	DDX_Control(pDX, IDC_COMBO_ITEMS, m_items);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Properties_Inventory, CResizablePage)
	//{{AFX_MSG_MAP(PropPage_Properties_Inventory)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_BN_CLICKED(IDC_REMOVEALL, OnRemoveall)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_RADIO_TEMPLATE, OnRadioTemplate)
	ON_BN_CLICKED(IDC_RADIO_SCREEN_NAME, OnRadioScreenName)
	ON_BN_CLICKED(IDC_RADIO_DOCUMENTATION, OnRadioDocumentation)
	ON_WM_KILLFOCUS()
	ON_BN_CLICKED(IDC_BUTTON_ADVANCED, OnButtonAdvanced)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Inventory message handlers

void PropPage_Properties_Inventory::OnRemove() 
{
	int sel = m_list_current.GetSelectionMark();
	if ( sel != LB_ERR )
	{
		DWORD dwGoid = m_list_current.GetItemData( sel );				
		GoHandle hItem( MakeGoid( dwGoid ) );
		if ( hItem.IsValid() )
		{
			GoHandle hObject( gGizmoManager.GetFirstObjectWithInventory() );
			hObject->GetInventory()->RSRemove( hItem, false );			
			gEditorObjects.DeleteObject( hItem->GetGoid() );
		}
		m_list_current.DeleteItem( sel );
	}
	m_bApply = true;
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Inventory::OnRemoveall() 
{
	for ( int i = 0; i != m_list_current.GetItemCount(); ++i )
	{
		DWORD dwGoid = m_list_current.GetItemData( i );				
		GoHandle hItem( MakeGoid( dwGoid ) );		
		if ( hItem.IsValid() )
		{
			GoHandle hObject( gGizmoManager.GetFirstObjectWithInventory() );
			hObject->GetInventory()->RSRemove( hItem, false );
			gEditorObjects.DeleteObject( hItem->GetGoid() );			
		}
	}

	m_list_current.DeleteAllItems();
	
	m_bApply = true;
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Inventory::OnAdd() 
{
	if ( m_items.GetCurSel() == CB_ERR ) 
	{
		return;
	}

	int item_count = m_list_current.GetItemCount();
	CString rText;	
	m_items.GetLBText( m_items.GetCurSel(), rText );

	DWORD id = m_items.GetItemData( m_items.GetCurSel() );
	gpstring sTemplate;
	gComponentList.GetTemplateNameFromMapId( id, sTemplate );
	ObjectComponent oc = gComponentList.GetObjectComponent( sTemplate );

	if ( !sTemplate.empty() )
	{
		int newItem = m_list_current.InsertItem( item_count, oc.sFuelName );	
		m_list_current.SetItem( newItem, 1, LVIF_TEXT, oc.sScreenName, 0, 0, 0, 0 );	
		m_list_current.SetItem( newItem, 2, LVIF_TEXT, oc.sDevName, 0, 0, 0, 0 );	
	}
	else
	{
		m_list_current.InsertItem( item_count, rText );			
	}

	++item_count;	

	m_bApply = true;
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Inventory::OnRadioTemplate() 
{
	if ( ((CButton *)GetDlgItem(IDC_RADIO_TEMPLATE))->GetCheck() )
	{
		gPreferences.SetContentDisplay( BLOCK_NAME );
		((CButton *)GetDlgItem(IDC_RADIO_SCREEN_NAME))->SetCheck( FALSE );
		((CButton *)GetDlgItem(IDC_RADIO_DOCUMENTATION))->SetCheck( FALSE );
		Reset();
	}	
	
}

void PropPage_Properties_Inventory::OnRadioScreenName() 
{
	if ( ((CButton *)GetDlgItem(IDC_RADIO_SCREEN_NAME))->GetCheck() )
	{
		gPreferences.SetContentDisplay( SCREEN_NAME );
		((CButton *)GetDlgItem(IDC_RADIO_TEMPLATE))->SetCheck( FALSE );
		((CButton *)GetDlgItem(IDC_RADIO_DOCUMENTATION))->SetCheck( FALSE );
		Reset();
	}	
}

void PropPage_Properties_Inventory::OnRadioDocumentation() 
{
	if ( ((CButton *)GetDlgItem(IDC_RADIO_DOCUMENTATION))->GetCheck() )
	{
		gPreferences.SetContentDisplay( DEV_NAME );
		((CButton *)GetDlgItem(IDC_RADIO_TEMPLATE))->SetCheck( FALSE );
		((CButton *)GetDlgItem(IDC_RADIO_SCREEN_NAME))->SetCheck( FALSE );
		Reset();
	}	
}

BOOL PropPage_Properties_Inventory::OnInitDialog() 
{
	CResizablePage::OnInitDialog();
	
	AddAnchor( IDC_REMOVE, MIDDLE_CENTER );
	AddAnchor( IDC_REMOVEALL, MIDDLE_CENTER );
	AddAnchor( IDC_ADD, MIDDLE_CENTER );
	AddAnchor( IDC_RADIO_TEMPLATE, MIDDLE_CENTER );
	AddAnchor( IDC_RADIO_SCREEN_NAME, MIDDLE_CENTER );
	AddAnchor( IDC_RADIO_DOCUMENTATION, MIDDLE_CENTER );
	AddAnchor( IDC_LIST_CURRENT_INVENTORY, MIDDLE_CENTER );
	AddAnchor( IDC_COMBO_ITEMS, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_INVENTORY, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_DISPLAY, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_ITEMS, MIDDLE_CENTER );
	AddAnchor( IDC_BUTTON_ADVANCED, MIDDLE_CENTER );

	if ( gGizmoManager.GetNumObjectsSelected() == 0 ) {
		return TRUE;
	}	
	
	m_list_current.InsertColumn( 0, "Template Name", LVCFMT_LEFT, 90 );
	m_list_current.InsertColumn( 1, "Screen Name", LVCFMT_LEFT, 80 );
	m_list_current.InsertColumn( 2, "Documentation", LVCFMT_LEFT, 100 );

	Reset();
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void PropPage_Properties_Inventory::Reset()
{
	if ( !GetSafeHwnd() )
	{
		return;
	}
	m_items.ResetContent();
	ComponentVec items;
	ComponentVec::iterator i;
	gComponentList.RetrieveObjectList( items );
	int index = 0;
	for ( i = items.begin(); i != items.end(); ++i ) 
	{
		int newItem = m_items.AddString( (*i).sName.c_str() );
		int id = 0;
		gComponentList.GetMapIdFromTemplateName( (*i).sFuelName, id );
		m_items.SetItemData( newItem, id );			
	}

	m_list_current.DeleteAllItems();
	items.clear();
	gComponentList.RetrieveObjectInventoryVec( gGizmoManager.GetFirstObjectWithInventory(), items );
	index = 0;
	for ( i = items.begin(); i != items.end(); ++i ) {

		GoHandle hObject( gGizmoManager.GetFirstObjectWithInventory() );
		GoHandle hItem( (*i).object );
		if ( hObject.IsValid() && hObject->HasInventory() )
		{
			if ( hObject->GetInventory()->IsEquipped( hItem ) )
			{
				continue;
			}
		}

		m_list_current.InsertItem( index, (*i).sFuelName.c_str() );
		m_list_current.SetItem( index, 1, LVIF_TEXT, (*i).sScreenName.c_str(), 0, 0, 0, 0 );
		m_list_current.SetItem( index, 2, LVIF_TEXT, (*i).sDevName.c_str(), 0, 0, 0, 0 );
		m_list_current.SetItemData( index, MakeInt((*i).object) );
		index++;		
	}		
	m_bApply = false;
	gObjectProperties.Evaluate();
}


void PropPage_Properties_Inventory::Apply()
{
	std::vector< DWORD > objects;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedObjectIDs( objects );
	for ( i = objects.begin(); i != objects.end(); ++i ) 
	{
		GoHandle hObject( MakeGoid(*i) );
		if ( hObject.IsValid() && hObject->HasInventory() )
		{
			GopColl items;
			GopColl::iterator k;
			hObject->GetInventory()->ListItems( IL_MAIN, items );
			for ( k = items.begin(); k != items.end(); ++k )
			{
				if ( hObject->GetInventory()->IsEquipped( *k ) )
				{
					continue;
				}
				hObject->GetInventory()->RSRemove( *k, false );
				gEditorObjects.DeleteObject( (*k)->GetGoid() );			
			}
		}

		for ( int j = 0; j != m_list_current.GetItemCount(); ++j ) 
		{	
			CString rTemplate = m_list_current.GetItemText( j, 0 );
					
			if ( hObject.IsValid() && ( hObject->HasActor() || hObject->IsContainer() ) ) 
			{				
				GoCloneReq createGo( rTemplate.GetBuffer( 0 ), gServer.GetPlayer( ToUnicode( "human" ) )->GetId() );
				createGo.SetStartingPos( hObject->GetPlacement()->GetPosition() );	
				Goid item = gGoDb.SCloneGo( createGo );
				GoHandle hItem( item );				
				
				if ( hObject->GetInventory()->IsEquipped( hItem ) )
				{
					continue;
				}

				if( hItem.IsValid() && (hItem->IsInsideInventory() == false) && hObject->HasInventory() ) {			
					if ( hObject->GetInventory()->Contains( hItem ) == false )
					{
						hObject->GetInventory()->Add( hItem, IL_MAIN, AO_REFLEX, false );														
					}
				}								
			}
		}
	}

	m_bApply = false;
}


void PropPage_Properties_Inventory::Cancel()
{
}



void PropPage_Properties_Inventory::OnKillFocus(CWnd* pNewWnd) 
{
	CResizablePage::OnKillFocus(pNewWnd);
	
	if ( gObjectProperties.Evaluate() )
	{
		int result = MessageBoxEx( pNewWnd->GetSafeHwnd(), "You have changed the properties on this page.  Would you like to apply the changes?", "Apply Changes?", MB_YESNO | MB_ICONQUESTION, LANG_ENGLISH );
		if ( result == IDYES ) 
		{
			gObjectProperties.Apply();
		}	
	}	
}

BOOL PropPage_Properties_Inventory::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
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

void PropPage_Properties_Inventory::OnButtonAdvanced() 
{
	gEditorObjects.SetItemViewerFilter( ES_NONE, false );
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
	}	
}
