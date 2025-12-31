// Dialog_Mesh_List.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Mesh_List.h"
#include <map>
#include "ComponentList.h"
#include "EditorTerrain.h"
#include "Dialog_New_Mesh_Group.h"
#include "EditorMap.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace siege;

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mesh_List dialog


Dialog_Mesh_List::Dialog_Mesh_List(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Mesh_List::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Mesh_List)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Mesh_List::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Mesh_List)
	DDX_Control(pDX, IDC_LIST_MESH_GROUPS, m_mesh_group);
	DDX_Control(pDX, IDC_LIST_MESH, m_meshes);
	DDX_Control(pDX, IDC_COMBO_GROUPS, m_groups);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Mesh_List, CDialog)
	//{{AFX_MSG_MAP(Dialog_Mesh_List)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_MESH, OnButtonSelectMesh)
	ON_BN_CLICKED(IDC_BUTTON_COPY_TO_CLIPBOARD, OnButtonCopyToClipboard)
	ON_CBN_SELCHANGE(IDC_COMBO_GROUPS, OnSelchangeComboGroups)
	ON_BN_CLICKED(IDC_BUTTON_NEW_GROUP, OnButtonNewGroup)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_GROUP, OnButtonDeleteGroup)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_GROUP, OnButtonSelectGroup)
	ON_BN_CLICKED(IDC_BUTTON_HIDE_GROUP, OnButtonHideGroup)
	ON_BN_CLICKED(IDC_BUTTON_SHOW_GROUP, OnButtonShowGroup)
	ON_BN_CLICKED(IDC_BUTTON_ADD_TO_GROUP, OnButtonAddToGroup)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mesh_List message handlers

BOOL Dialog_Mesh_List::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CListCtrl * pList = (CListCtrl *)GetDlgItem( IDC_LIST_MESH );
	pList->DeleteAllItems();

	pList->InsertColumn( 0, "Mesh GUID", LVCFMT_LEFT, 80 );
	pList->InsertColumn( 1, "SNO File", LVCFMT_LEFT, 200 );

	m_mesh_group.InsertColumn( 0, "Mesh GUID", LVCFMT_LEFT, 80 );
	m_mesh_group.InsertColumn( 1, "SNO File", LVCFMT_LEFT, 200 );
		
	std::map< gpstring, gpstring, istring_less > mesh_list;
	gComponentList.GetMeshToSnoMap( mesh_list );
	std::map< gpstring, gpstring, istring_less >::iterator i;
	int index = 0;
	for ( i = mesh_list.begin(); i != mesh_list.end(); ++i ) 
	{
		pList->InsertItem( index, (*i).first.c_str() );
		pList->SetItem( index, 1, LVIF_TEXT, (*i).second.c_str(), 0, 0, 0, 0 );
		index++;		
	}	
	
	RefreshGroups();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Mesh_List::RefreshGroups()
{
	m_groups.ResetContent();

	FuelHandle hEditor( "config:editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hGroups = hEditor->GetChildBlock( "mesh_groups" );
		if ( hGroups.IsValid() )
		{
			FuelHandleList hlMeshGroups = hGroups->ListChildBlocks( 1, "mesh_group" );
			FuelHandleList::iterator i;
			for ( i = hlMeshGroups.begin(); i != hlMeshGroups.end(); ++i )
			{
				m_groups.AddString( (*i)->GetName() );			
			}
		}
	}
}

void Dialog_Mesh_List::OnButtonSelectMesh() 
{
	CListCtrl * pList = (CListCtrl *)GetDlgItem( IDC_LIST_MESH );
	char szText[256] = "";
	pList->GetItemText( pList->GetSelectionMark(), 0, szText, 256 );
	siege::database_guid mesh_guid( szText );
	gEditorTerrain.ClearAllSelectedNodes();
	gEditorTerrain.SelectAllNodesOfMesh( mesh_guid );
}

void Dialog_Mesh_List::OnButtonCopyToClipboard() 
{
	std::map< gpstring, gpstring, istring_less > mesh_list;
	gComponentList.GetMeshToSnoMap( mesh_list );
	std::map< gpstring, gpstring, istring_less >::iterator i;
	int index = 0;
	gpstring sNodes;
	for ( i = mesh_list.begin(); i != mesh_list.end(); ++i ) 
	{		
		sNodes.appendf( "%s\r\n", (*i).second.c_str() );		
	}

	// Open the clipboard, and empty it. 
	if ( !::OpenClipboard( gEditor.GetHWnd() ) ) 
	{
		return; 
	}
	EmptyClipboard();

	LPTSTR  lptstrCopy; 
	HGLOBAL hglbCopy; 

	hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (sNodes.size() + 1) * sizeof(TCHAR) ); 
	if (hglbCopy == NULL) 
	{ 
		CloseClipboard(); 
		return; 
	} 

	// Lock the handle and copy the text to the buffer. 
	lptstrCopy = (char *)GlobalLock(hglbCopy); 
	memcpy(lptstrCopy, sNodes.c_str(), sNodes.size() * sizeof(TCHAR)); 
	lptstrCopy[sNodes.size()] = (TCHAR) 0;    // null character 
	GlobalUnlock( hglbCopy) ; 

	// Place the handle on the clipboard.
	SetClipboardData(CF_TEXT, hglbCopy); 

	CloseClipboard();	
}

void Dialog_Mesh_List::OnSelchangeComboGroups() 
{
	m_mesh_group.DeleteAllItems();

	CString rGroup;
	int sel = m_groups.GetCurSel();
	if ( sel == CB_ERR )
	{
		return;
	}

	m_groups.GetLBText( sel, rGroup );

	FuelHandle hEditor( "config:editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hGroups = hEditor->GetChildBlock( "mesh_groups" );				
		if ( hGroups.IsValid() )
		{			
			FuelHandleList hlMeshGroups = hGroups->ListChildBlocks( 1, "mesh_group" );
			FuelHandleList::iterator i;
			for ( i = hlMeshGroups.begin(); i != hlMeshGroups.end(); ++i )
			{
				if ( gpstring( (*i)->GetName() ).same_no_case( rGroup.GetBuffer( 0 ) ) )
				{
					if ( (*i)->FindFirstKeyAndValue() )
					{
						gpstring sKey;
						gpstring sValue;
						int index = 0;
						while ( (*i)->GetNextKeyAndValue( sKey, sValue ) )
						{
							if ( !sValue.empty() )
							{
								gpstring sSno = sValue + ".sno";
								gpstring sGuid = gComponentList.GetNodeGUID( (char *)sSno.c_str() );
								int sel = m_mesh_group.InsertItem( index, sGuid.c_str() );
								m_mesh_group.SetItem( sel, 1, LVIF_TEXT, sValue.c_str(), 0, 0, 0, 0 );
								index++;
							}
						}
					}				
					break;
				}
			}
		}
	}	
}

void Dialog_Mesh_List::OnButtonNewGroup() 
{
	Dialog_New_Mesh_Group dlg;
	if ( dlg.DoModal() == IDOK )
	{
		m_groups.AddString( GetMeshGroupName().c_str() );

		FuelHandle hEditor( "config:editor" );
		if ( hEditor.IsValid() )
		{
			FuelHandle hGroups = hEditor->GetChildBlock( "mesh_groups" );
			if ( !hGroups.IsValid() )
			{
				hGroups = hEditor->CreateChildBlock( "mesh_groups", "mesh_groups.gas" );				
			}

			if ( hGroups.IsValid() )
			{	
				FuelHandle hGroup = hGroups->CreateChildBlock( GetMeshGroupName() );
				hGroup->SetType( "mesh_group" );
			}
		}
	}	
}


void Dialog_Mesh_List::OnButtonDeleteGroup() 
{
	CString rGroup;
	int sel = m_groups.GetCurSel();
	if ( sel == CB_ERR )
	{
		return;
	}

	m_groups.GetLBText( sel, rGroup );

	FuelHandle hEditor( "config:editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hGroups = hEditor->GetChildBlock( "mesh_groups" );				
		if ( hGroups.IsValid() )
		{	
			FuelHandle hChild = hGroups->GetChildBlock( rGroup.GetBuffer( 0 ) );
			if ( hChild.IsValid() )
			{
				hGroups->DestroyChildBlock( hChild );
			}
		}
	}

	RefreshGroups();	
	OnSelchangeComboGroups();
}

void Dialog_Mesh_List::OnButtonSelectGroup() 
{		
	bool bClear = true;
	gEditorTerrain.ClearAllSelectedNodes();
	for ( int i = 0; i != m_mesh_group.GetItemCount(); ++i )
	{
		CString rMesh;
		rMesh = m_mesh_group.GetItemText( i, 0 );
		siege::database_guid mesh_guid( rMesh.GetBuffer( 0 ) );			
		
		gEditorTerrain.SelectAllNodesOfMesh( mesh_guid, bClear );
		if ( bClear )
		{
			bClear = false;
		}
	}	
}

void Dialog_Mesh_List::OnButtonHideGroup() 
{
	bool bClear = true;
	gEditorTerrain.ClearAllSelectedNodes();
	for ( int i = 0; i != m_mesh_group.GetItemCount(); ++i )
	{
		CString rMesh;
		rMesh = m_mesh_group.GetItemText( i, 0 );
		siege::database_guid mesh_guid( rMesh.GetBuffer( 0 ) );		
		gEditorTerrain.SelectAllNodesOfMesh( mesh_guid, bClear );
		if ( bClear )
		{
			bClear = false;
		}
	}
	gEditorTerrain.HideSelectedNodes();	
	gEditorTerrain.ClearAllSelectedNodes();
}

void Dialog_Mesh_List::OnButtonShowGroup() 
{
	bool bClear = true;
	gEditorTerrain.ClearAllSelectedNodes();

	for ( int j = 0; j != m_mesh_group.GetItemCount(); ++j )
	{
		CString rMesh;
		rMesh = m_mesh_group.GetItemText( j, 0 );
		siege::database_guid mesh_guid( rMesh.GetBuffer( 0 ) );		
		gEditorTerrain.SelectAllNodesOfMesh( mesh_guid, bClear );
		if ( bClear )
		{
			bClear = false;
		}
	}

	{
		SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( gEditorTerrain.GetSelectedNode() ) );
		SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );	
		node.SetVisible( true );
	}

	GuidVec guid_vec = gEditorTerrain.GetSelectedNodes();
	GuidVec::iterator i;
	for ( i = guid_vec.begin(); i != guid_vec.end(); ++i ) 
	{
		SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( *i ) );
		SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );		
		node.SetVisible( true );		
	}		

	gEditorTerrain.ClearAllSelectedNodes();
}

void Dialog_Mesh_List::OnButtonAddToGroup() 
{
	CString rGroup;
	int sel = m_groups.GetCurSel();
	if ( sel == CB_ERR )
	{
		return;
	}

	m_groups.GetLBText( sel, rGroup );

	FuelHandle hEditor( "config:editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hGroups = hEditor->GetChildBlock( "mesh_groups" );				
		if ( hGroups.IsValid() )
		{	
			FuelHandle hGroup = hGroups->GetChildBlock( rGroup.GetBuffer( 0 ) );
			if ( hGroup.IsValid() )
			{
				POSITION pos = m_meshes.GetFirstSelectedItemPosition();
				if ( pos )
				{
					while ( pos )
					{
						int sel = m_meshes.GetNextSelectedItem( pos );						
						CString rSno;						
						rSno = m_meshes.GetItemText( sel, 1 );
						hGroup->Set( "*", rSno.GetBuffer( 0 ) );
					}
					OnSelchangeComboGroups();						
				}
			}
		}
	}			
}

void Dialog_Mesh_List::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
