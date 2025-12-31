// Dialog_Node_Matches.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Node_Matches.h"
#include "LeftView.h"
#include "siege_engine.h"
#include "siege_node.h"
#include "EditorTerrain.h"
#include "ImageListDefines.h"
#include "namingkey.h"
#include "SiegeEditorShell.h"
#include "Dialog_Node_Match_Progress.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace siege;

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_Matches dialog


Dialog_Node_Matches::Dialog_Node_Matches(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Node_Matches::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Node_Matches)
	//}}AFX_DATA_INIT
}


void Dialog_Node_Matches::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Node_Matches)
	DDX_Control(pDX, IDC_TREE_MATCHES, m_Matches);
	DDX_Control(pDX, IDC_EDIT_DOOR, m_SelectedDoor);
	DDX_Control(pDX, IDC_EDIT_SNO, m_SelectedSno);
	DDX_Control(pDX, IDC_EDIT_SEARCH_LEVEL, m_SearchLevel);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Node_Matches, CDialog)
	//{{AFX_MSG_MAP(Dialog_Node_Matches)
	ON_BN_CLICKED(IDC_BUTTON_MATCH, OnButtonMatch)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_MATCHES, OnSelchangedTreeMatches)
	ON_BN_CLICKED(IDC_BUTTON_ATTACH, OnButtonAttach)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_Matches message handlers

BOOL Dialog_Node_Matches::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_Matches.SetImageList( gEditor.GetImageList(), TVSIL_NORMAL );
	
	Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void Dialog_Node_Matches::Refresh()
{
	SiegeNode * pSource = gSiegeEngine.IsNodeValid( gEditorTerrain.GetSelectedNode() );
	if ( pSource )
	{
		gpstring sSno = gComponentList.GetMeshSnoFile( pSource->GetMeshGUID().ToString() );
		m_SelectedSno.SetWindowText( sSno.c_str() );

		gpstring sDoor;
		sDoor.assignf( "%d", gEditorTerrain.GetSelectedSourceDoorID() );

		m_SelectedDoor.SetWindowText( sDoor.c_str() );
	}	

	m_SearchLevel.SetWindowText( "1" );	
}

void Dialog_Node_Matches::OnButtonMatch() 
{
	Dialog_Node_Match_Progress dlgProgress;
	dlgProgress.Create( IDD_DIALOG_NODE_MATCH_PROGRESS );

	m_Matches.DeleteAllItems();

	int searchLevel = 1;
	CString rLevel;
	m_SearchLevel.GetWindowText( rLevel );
	if ( rLevel.IsEmpty() )
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please enter a search level (Default is 1).", "Node Matcher", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		m_SearchLevel.SetWindowText( "1" );
		return;
	}

	stringtool::Get( rLevel.GetBuffer( 0 ), searchLevel );

	NodeMatchDataColl nodeMatchData;
	gEditorTerrain.FindNodeMatches( searchLevel, nodeMatchData );

	NodeMatchDataColl::iterator iMatch;
	for ( iMatch = nodeMatchData.begin(); iMatch != nodeMatchData.end(); ++iMatch )
	{
		gpstring sSno;
		gNamingKey.BuildSNOLocation( (*iMatch).sSno, sSno );
		
		sSno = sSno.substr( gpstring( "art\\" ).size(), sSno.size()  );

		HTREEITEM hParent = gLeftView.CheckParentDirectory( sSno, m_Matches.m_hWnd, TVI_ROOT );
		if ( hParent == 0 ) 
		{
			hParent = TVI_ROOT;
		}
		HTREEITEM hItem = m_Matches.InsertItem( (*iMatch).sSno.c_str(), IMAGE_SN, IMAGE_SN, hParent, TVI_SORT );		
		m_Matches.SetItemData( hItem, (*iMatch).meshGuid.GetValue() );	
		
		std::vector< int >::iterator iDoor;
		for ( iDoor = (*iMatch).doorMatches.begin(); iDoor != (*iMatch).doorMatches.end(); ++iDoor )
		{
			gpstring sDoor;
			sDoor.assignf( "Door %d", (*iDoor) );
			HTREEITEM hDoor = m_Matches.InsertItem( sDoor.c_str(), IMAGE_LTBLUEBOX, IMAGE_LTBLUEBOX, hItem, TVI_SORT );
			m_Matches.SetItemData( hDoor, (*iDoor) );
		}

		m_Matches.Expand( hParent, TVE_EXPAND );
	}

	m_Matches.Expand( m_Matches.GetChildItem( TVI_ROOT ), TVE_EXPAND );

	dlgProgress.DestroyWindow();
}

void Dialog_Node_Matches::OnSelchangedTreeMatches(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	
	int data = m_Matches.GetItemData( m_Matches.GetSelectedItem() );
	if ( data != 0 )
	{
		int nImage = 0;
		int nSelImage = 0;
		m_Matches.GetItemImage( m_Matches.GetSelectedItem(), nImage, nSelImage );
		CString rSno;
		if ( nImage == IMAGE_LTBLUEBOX )
		{
			rSno = m_Matches.GetItemText( m_Matches.GetParentItem( m_Matches.GetSelectedItem() ) );			
		}
		else
		{
			rSno = m_Matches.GetItemText( m_Matches.GetSelectedItem() );
		}

		gLeftView.PublishDestDoorList( rSno.GetBuffer( 0 ) );

		if ( nImage == IMAGE_LTBLUEBOX )
		{
			gLeftView.SetDestDoor( m_Matches.GetItemData( m_Matches.GetSelectedItem() ) );
		}
	}
	
	*pResult = 0;
}

void Dialog_Node_Matches::OnButtonAttach() 
{
	gLeftView.OnAttach();	
}
