// Dialog_Stitch_Manager.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Stitch_Manager.h"
#include "Stitch_Helper.h"
#include "ImageListDefines.h"
#include "EditorRegion.h"
#include "MenuCommands.h"
#include "EditorTerrain.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Stitch_Manager dialog


Dialog_Stitch_Manager::Dialog_Stitch_Manager(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Stitch_Manager::IDD, pParent)
	, m_hItem( 0 )
	, m_hRegionItem( 0 )
{
	//{{AFX_DATA_INIT(Dialog_Stitch_Manager)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Stitch_Manager::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Stitch_Manager)
	DDX_Control(pDX, IDC_LIST_REGION_INFO, m_RegionInfo);
	DDX_Control(pDX, IDC_TREE_STITCH, m_StitchTree);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Stitch_Manager, CDialog)
	//{{AFX_MSG_MAP(Dialog_Stitch_Manager)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_ALL, OnButtonRemoveAll)
	ON_BN_CLICKED(IDC_BUTTON_OPEN, OnButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_STITCH, OnSelchangedTreeStitch)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Stitch_Manager message handlers

BOOL Dialog_Stitch_Manager::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	BuildStitchHelperTree();

	m_RegionInfo.InsertColumn( 0, "Target Region Name", LVCFMT_LEFT, 150 );
	m_RegionInfo.InsertColumn( 1, "Number of Stitches", LVCFMT_LEFT, 150 );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Stitch_Manager::OnButtonRemove() 
{
	CTreeCtrl * pTree = (CTreeCtrl *)GetDlgItem( IDC_TREE_STITCH );
	if ( pTree->GetParentItem( pTree->GetSelectedItem() ) == m_hRegionItem ) {
		gStitchHelper.RemoveNodeIDFromStitchHelper( pTree->GetItemData( pTree->GetSelectedItem() ) );
		pTree->DeleteItem( pTree->GetSelectedItem() );		
	}	
}

void Dialog_Stitch_Manager::OnButtonRemoveAll() 
{	
	CTreeCtrl * pTree = (CTreeCtrl *)GetDlgItem( IDC_TREE_STITCH );
	if ( m_hRegionItem == NULL ) {
		return;
	}
	pTree->DeleteItem( m_hRegionItem );
	gStitchHelper.GetStitchVector().clear();
}



// Construct the stitching helper tree
void Dialog_Stitch_Manager::BuildStitchHelperTree()
{	
	CTreeCtrl * pTree = (CTreeCtrl *)GetDlgItem( IDC_TREE_STITCH );
	pTree->SetImageList( gEditor.GetImageList(), TVSIL_NORMAL );
	pTree->SetImageList( gEditor.GetImageList(), TVSIL_STATE );
	StitchVector::iterator i;

	{
		HTREEITEM hParent = pTree->InsertItem( gEditorRegion.GetRegionName().c_str(), IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER );
		m_hRegionItem = hParent;
		for ( i = gStitchHelper.GetStitchVector().begin(); i != gStitchHelper.GetStitchVector().end(); ++i ) {
			
			gpstring sTemp;
			sTemp.assignf( "Node ID: 0x%08X", (*i).nodeID );
			
			HTREEITEM hItem = pTree->InsertItem( sTemp.c_str(), IMAGE_3BOX, IMAGE_3BOX, hParent );
			m_hItem = hItem;
			pTree->SetItemData( hItem, (*i).nodeID );

			sTemp.assignf( "Node GUID: %s", (*i).nodeGUID.ToString().c_str() );
			pTree->InsertItem( sTemp.c_str(), IMAGE_3BOX, IMAGE_3BOX, hItem );

			sTemp.assignf( "Node Door: %d", (*i).doorID );
			pTree->InsertItem( sTemp.c_str(), IMAGE_3BOX, IMAGE_3BOX, hItem );	
			
			sTemp.assignf( "Target Region: %s", (*i).sRegion.c_str() );
			pTree->InsertItem( sTemp.c_str(), IMAGE_3BOX, IMAGE_3BOX, hItem );
		}
	}

	StringVec region_list;
	StringVec::iterator j;
	gEditorRegion.GetRegionList( gEditorRegion.GetMapName().c_str(), region_list );
	for ( j = region_list.begin(); j != region_list.end(); ++j ) {
		
		if ( (*j).same_no_case( gEditorRegion.GetRegionName() ) ) {
			continue;
		}

		HTREEITEM hParent = pTree->InsertItem( (*j).c_str(), IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER );
		StitchVector region_stitches;
		gStitchHelper.GetMapStitchVector( (*j).c_str(), region_stitches );
		for ( i = region_stitches.begin(); i != region_stitches.end(); ++i ) {					
			gpstring sTemp;
			sTemp.assignf( "Node ID: 0x%08X", (*i).nodeID );
			
			HTREEITEM hItem = pTree->InsertItem( sTemp.c_str(), IMAGE_LTBLUEBOX, IMAGE_LTBLUEBOX, hParent );
			pTree->SetItemData( hItem, 0 );

			sTemp.assignf( "Node GUID: %s", (*i).nodeGUID.ToString().c_str() );
			pTree->InsertItem( sTemp.c_str(), IMAGE_LTBLUEBOX, IMAGE_LTBLUEBOX, hItem );

			sTemp.assignf( "Node Door: %d", (*i).doorID );
			pTree->InsertItem( sTemp.c_str(), IMAGE_LTBLUEBOX, IMAGE_LTBLUEBOX, hItem );

			sTemp.assignf( "Target Region: %s", (*i).sRegion.c_str() );
			pTree->InsertItem( sTemp.c_str(), IMAGE_LTBLUEBOX, IMAGE_LTBLUEBOX, hItem );
		}
	}
}
void Dialog_Stitch_Manager::OnButtonOpen() 
{
	CTreeCtrl * pTree = (CTreeCtrl *)GetDlgItem( IDC_TREE_STITCH );
	if ( pTree->GetParentItem( pTree->GetSelectedItem() ) == m_hRegionItem ) 
	{
		int nodeID = pTree->GetItemData( pTree->GetSelectedItem() );

		StitchVector::iterator i;
		for ( i = gStitchHelper.GetStitchVector().begin(); i != gStitchHelper.GetStitchVector().end(); ++i ) {
			if ( (*i).nodeID == nodeID )
			{
				gEditorTerrain.SetSelectedNode( (*i).nodeGUID );
				MenuFunctions mf;
				mf.NodeProperties();
			}
		}
	}		
}

void Dialog_Stitch_Manager::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}

void Dialog_Stitch_Manager::OnSelchangedTreeStitch(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	
	HTREEITEM hItem	= m_StitchTree.GetSelectedItem();
	if ( hItem == NULL )
	{
		return;
	}

	HTREEITEM hParent = m_StitchTree.GetParentItem( hItem );
	while ( (hParent != TVI_ROOT && hParent != NULL) )
	{
		hItem = hParent;
		hParent = m_StitchTree.GetParentItem( hItem );		
	}

	CString rRegion = m_StitchTree.GetItemText( hItem );
	if ( !rRegion.IsEmpty() )
	{
		StringVec region_list;
		StringVec::iterator j;
		gEditorRegion.GetRegionList( gEditorRegion.GetMapName().c_str(), region_list );
		for ( j = region_list.begin(); j != region_list.end(); ++j ) 
		{
			if ( (*j).same_no_case( rRegion.GetBuffer( 0 ) ) )
			{
				StitchVector region_stitches;
				StitchVector::iterator i;
				gStitchHelper.GetMapStitchVector( (*j).c_str(), region_stitches );
				RegionToStitchMap regionToStitchMap;
				for ( i = region_stitches.begin(); i != region_stitches.end(); ++i ) 
				{	
					RegionToStitchMap::iterator iRs = regionToStitchMap.find( (*i).sRegion );
					if ( iRs != regionToStitchMap.end() )
					{
						(*iRs).second++;
						continue;
					}
					else
					{
						regionToStitchMap.insert( std::make_pair( (*i).sRegion, 1 ) );
					}					
				}

				m_RegionInfo.DeleteAllItems();

				RegionToStitchMap::iterator iRs;
				for ( iRs = regionToStitchMap.begin(); iRs != regionToStitchMap.end(); ++iRs )
				{
					int item = m_RegionInfo.InsertItem( 0, (*iRs).first.c_str() );
					gpstring sNum;
					sNum.assignf( "%d", (*iRs).second );
					m_RegionInfo.SetItemText( item, 1, sNum );
				}
			}
		}		
	}	
	
	*pResult = 0;
}

