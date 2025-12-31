// Dialog_Object_Inventory.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "SiegeEditor.h"
#include "Dialog_Object_Inventory.h"
#include "ComponentList.h"
#include "GizmoManager.h"
#include "Go.h"
#include "ContentDb.h"
#include "GoInventory.h"
#include "SCIDManager.h"
#include "Server.h"
#include "GoCore.h"
#include "EditorObjects.h"
#include "GoDb.h"
#include "Player.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Inventory dialog


Dialog_Object_Inventory::Dialog_Object_Inventory(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Object_Inventory::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Object_Inventory)
	//}}AFX_DATA_INIT
}


void Dialog_Object_Inventory::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Object_Inventory)
	DDX_Control(pDX, IDC_LIST_CURRENT_INVENTORY, m_list_current);
	DDX_Control(pDX, IDC_LIST_AVAILABLE_INVENTORY, m_list_available);
	//}}AFX_DATA_Mhttp://formen.ign.com/news/26191.htmlAP
}


BEGIN_MESSAGE_MAP(Dialog_Object_Inventory, CDialog)
	//{{AFX_MSG_MAP(Dialog_Object_Inventory)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_BN_CLICKED(IDC_REMOVEALL, OnRemoveall)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Inventory message handlers

void Dialog_Object_Inventory::OnRemove() 
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
}

void Dialog_Object_Inventory::OnRemoveall() 
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
}

void Dialog_Object_Inventory::OnAdd() 
{
	if ( m_list_available.GetSelectionMark() == -1 ) {
		return;
	}

	int item_count = m_list_current.GetItemCount();

	CString rText;
	rText = m_list_available.GetItemText( m_list_available.GetSelectionMark(), 0 );
	m_list_current.InsertItem( item_count, rText );	
	int test = m_list_available.GetItemData( m_list_available.GetSelectionMark() );
	m_list_current.SetItemData( item_count, m_list_available.GetItemData( m_list_available.GetSelectionMark() ) );
	
	rText = m_list_available.GetItemText( m_list_available.GetSelectionMark(), 1 );
	m_list_current.SetItem( item_count, 1, LVIF_TEXT, rText, 0, 0, 0, 0 );
	
	rText = m_list_available.GetItemText( m_list_available.GetSelectionMark(), 2 );
	m_list_current.SetItem( item_count, 2, LVIF_TEXT, rText, 0, 0, 0, 0 );	

	++item_count;
}

void Dialog_Object_Inventory::OnOK() 
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
						hObject->GetInventory()->Add( hItem, IL_MAIN, false );														
					}
				}								
			}
		}
	}
	
	
	CDialog::OnOK();
}

BOOL Dialog_Object_Inventory::OnInitDialog() 
{
	if ( gGizmoManager.GetNumObjectsSelected() == 0 ) {
		return TRUE;
	}

	CDialog::OnInitDialog();
	
	m_list_available.DeleteAllItems();

	m_list_available.InsertColumn( 0, "Template Name", LVCFMT_LEFT, 80 );
	m_list_available.InsertColumn( 1, "Screen Name", LVCFMT_LEFT, 80 );	

	m_list_current.InsertColumn( 0, "Template Name", LVCFMT_LEFT, 80 );
	m_list_current.InsertColumn( 1, "Screen Name", LVCFMT_LEFT, 80 );
	
	ComponentVec items;
	ComponentVec::iterator i;
	gComponentList.RetrieveObjectList( items );
	int index = 0;
	for ( i = items.begin(); i != items.end(); ++i ) {
		m_list_available.InsertItem( index, (*i).sFuelName.c_str() );
		m_list_available.SetItem( index, 1, LVIF_TEXT, (*i).sScreenName.c_str(), 0, 0, 0, 0 );
		index++;		
	}

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
		m_list_current.SetItemData( index, MakeInt((*i).object) );
		index++;		
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
