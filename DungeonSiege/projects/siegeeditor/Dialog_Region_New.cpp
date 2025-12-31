// Dialog_Region_New.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Region_New.h"
#include "EditorRegion.h"
#include "ImageListDefines.h"
#include "ComponentList.h"
#include "LeftView.h"
#include "MeshPreviewer.h"
#include "FileSysUtils.h"
#include "Dialog_New_Map.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_New dialog


Dialog_Region_New::Dialog_Region_New(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Region_New::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Region_New)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Region_New::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Region_New)
	DDX_Control(pDX, IDC_EDIT_MESH_RANGE, m_MeshRange);
	DDX_Control(pDX, IDC_MAPNAME, m_mapName);
	DDX_Control(pDX, IDC_EDIT_SCID_RANGE, m_scidRange);
	DDX_Control(pDX, IDC_EDIT_REGION_ID, m_regionId);
	DDX_Control(pDX, IDC_ROOTNODES, m_treeNodes);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Region_New, CDialog)
	//{{AFX_MSG_MAP(Dialog_Region_New)
	ON_NOTIFY(NM_CLICK, IDC_ROOTNODES, OnClickRootnodes)
	ON_BN_CLICKED(IDC_BUTTON_NEW_MAP, OnButtonNewMap)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT_SCID_RANGE, OnButtonDefaultScidRange)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT_REGION_ID, OnButtonDefaultRegionId)
	ON_CBN_SELCHANGE(IDC_MAPNAME, OnSelchangeMapname)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT_MESH_RANGE, OnButtonDefaultMeshRange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Forward Declarations
void GetMapDirNameColl( StringVec & maps );
void GetRegionDirNameColl( char *szMap, StringVec & region_list );

/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_New message handlers

BOOL Dialog_Region_New::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	char						szName[MAX_PATH]	= "";
	char						szMap[MAX_PATH]		= "";
	char						szNode[MAX_PATH]	= "";
	StringVec	node_list;
	StringVec::iterator i;
	StringVec	maps;
	StringVec::iterator j;
	
	GetMapDirNameColl( maps );	
	CComboBox *pCombo = (CComboBox *)this->GetDlgItem( IDC_MAPNAME );
	for ( j = maps.begin(); j != maps.end(); j++ ) 
	{
		pCombo->AddString( (*j).c_str() );		
	}	

	CTreeCtrl * pTree = (CTreeCtrl *)this->GetDlgItem( IDC_ROOTNODES );
	pTree->SetImageList( gEditor.GetImageList(), TVSIL_NORMAL );
	pTree->SetImageList( gEditor.GetImageList(), TVSIL_STATE );
	pTree->DeleteAllItems();

	node_list.clear();
	gComponentList.RetrieveExistingNodeCollection( node_list );

	HTREEITEM hNodes = pTree->InsertItem( "Terrain Nodes", IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, TVI_ROOT, TVI_SORT );	
	
	for ( i = node_list.begin(); i != node_list.end(); i++ ) {
		HTREEITEM hParent = gLeftView.CheckParentDirectory( (*i), pTree->m_hWnd, hNodes );
		if ( hParent == 0 ) {
			hParent = hNodes;
		}
		pTree->InsertItem( RemoveDirectories((char *)((*i).c_str())), IMAGE_SN, IMAGE_SN, hParent, TVI_SORT );
	}
	pTree->Expand( hNodes, TVE_EXPAND );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void GetMapDirNameColl( StringVec & maps )
{
	// Reintialize the maps
	{		
		FastFuelHandle maps_parent( "world:maps" );
		maps_parent.Unload();
	}

	FastFuelHandle hMaps( "world:maps" );
	FastFuelHandleColl hlMaps;
	hMaps.ListChildren( hlMaps, 1 );
	FastFuelHandleColl::iterator i;
	for ( i = hlMaps.begin(); i != hlMaps.end(); i++ ) 
	{
		maps.push_back( (*i).GetName() );
	}
}

void Dialog_Region_New::OnOK() 
{
	// Retreive Map name
	CComboBox *pCombo = (CComboBox *)this->GetDlgItem( IDC_MAPNAME );
	CString rMap;
	if ( pCombo->GetCurSel() != CB_ERR ) 
	{
		pCombo->GetLBText( pCombo->GetCurSel(), rMap );
	}
	if ( !strcmp( "", rMap.GetBuffer( rMap.GetLength() ) ) ) 
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please Select a Map.", "Map Selection", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}
	
	// Retrieve Region name
	CString rRegionName;
	this->GetDlgItemText( IDC_REGIONNAME, rRegionName );
	if ( !strcmp( "", rRegionName.GetBuffer( rRegionName.GetLength() ) ) ) 
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please Type in a Region Name.", "Region Name", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	// Check if region name is valid
	{
		gpstring sTemp = rRegionName.GetBuffer( 0 );
		if( !FileSys::IsValidFileName( sTemp, false, false ) )
		{
			MessageBoxEx( this->GetSafeHwnd(), "Invalid Region Name.  Please do not use these characters: " BAD_FILENAME_CHARS_ONLY " and space.", "Region Name", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			return;			
		}
	}
	
	// Get Target node
	CTreeCtrl * pTree = (CTreeCtrl *)this->GetDlgItem( IDC_ROOTNODES );
	CString rNode = pTree->GetItemText( pTree->GetSelectedItem() );
		
	if ( !IsSNOFile( rNode.GetBuffer( rNode.GetLength() ) ) ) 
	{						
		MessageBoxEx( this->GetSafeHwnd(), "Please Select a Root Node", "Root Node", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}
	
	// Make sure the region name isn't being used by a different region
	gpstring region_name;
	region_name.assign( rRegionName.GetBuffer( rRegionName.GetLength() ) );
	StringVec region_list;
	StringVec::iterator	i;
	GetRegionDirNameColl( rMap.GetBuffer( rMap.GetLength() ), region_list );
	for ( i = region_list.begin(); i != region_list.end(); ++i ) 
	{
		if ( region_name.same_no_case( (*i) ) ) 
		{
			MessageBoxEx( this->GetSafeHwnd(), "That region name is already in use, please select a different name.", "Region Name", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			return;
		}
	}
	
	// Make sure the scid range is valid
	CString sScidRange;
	m_scidRange.GetWindowText( sScidRange );
	int scidRange = 0;
	stringtool::Get( sScidRange.GetBuffer( 0 ), scidRange );
	if ( !gEditorRegion.IsValidScidRange( rMap.GetBuffer( 0 ), scidRange ) )
	{
		MessageBoxEx( this->GetSafeHwnd(), "The scid range you chose is already in use, please choose another or use the default.", "Region Scid Range", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	// Make sure the region id is valid
	CString sRegionId;
	m_regionId.GetWindowText( sRegionId );
	int regionId = 0;
	stringtool::Get( sRegionId.GetBuffer( 0 ), regionId );
	if ( !gEditorRegion.IsValidRegionId( rMap.GetBuffer( 0 ), regionId ) )
	{
		MessageBoxEx( this->GetSafeHwnd(), "The region ID you chose is already in use, please choose another or use the default.", "Region ID", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	// Make sure the region id is valid
	CString sMeshRange;
	m_MeshRange.GetWindowText( sMeshRange );
	int meshRange = 0;
	stringtool::Get( sMeshRange.GetBuffer( 0 ), meshRange );
	if ( !gEditorRegion.IsValidMeshRange( rMap.GetBuffer( 0 ), meshRange ) )
	{
		MessageBoxEx( this->GetSafeHwnd(), "The mesh range you chose is already in use, please choose another or use the default.", "Region ID", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	RegionCreateInfo regionInfo;
	regionInfo.regionId		= regionId;
	regionInfo.scidRange	= scidRange;
	regionInfo.sMapName		= rMap.GetBuffer( 0 );
	regionInfo.sRegionName	= rRegionName.GetBuffer( 0 );
	regionInfo.sSnoFile		= rNode.GetBuffer( 0 );
	regionInfo.meshRange	= meshRange;
	gEditorRegion.CreateRegion( regionInfo );	
	
	CDialog::OnOK();
}


void GetRegionDirNameColl( char *szMap, StringVec & region_list )
{
	// Reintialize the maps
	{		
		FastFuelHandle maps_parent( "world:maps" );
		maps_parent.Unload();
	}

	FastFuelHandle fhMaps( "world:maps" );
	FastFuelHandleColl map_list;
	fhMaps.ListChildren( map_list, 1 );
	FastFuelHandleColl::iterator i;
	for ( i = map_list.begin(); i != map_list.end(); i++ ) 
	{
		gpstring name = (*i).GetName();
		if ( name.same_no_case( szMap ) ) 
		{
			FastFuelHandleColl fhlRegions;
			(*i).ListChildrenTyped( fhlRegions, "region", 3 );
			FastFuelHandleColl::iterator j;
			for ( j = fhlRegions.begin(); j != fhlRegions.end(); ++j ) 
			{
				gpstring region_name = (*j).GetParent().GetName();
				region_list.push_back( region_name );
			}
		}
	}
}

LRESULT Dialog_Region_New::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{		
	return CDialog::WindowProc(message, wParam, lParam);
}

void Dialog_Region_New::OnClickRootnodes(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CString rText = m_treeNodes.GetItemText( m_treeNodes.GetSelectedItem() );
	if ( IsSNOFile( rText.GetBuffer( rText.GetLength() ) ) ) 
	{
		gpstring sGuid, sSno;
		sGuid = gComponentList.GetNodeGUID( rText.GetBuffer( rText.GetLength() ) );
		gPreviewer.LoadPreviewNode( sGuid ); 
	}
	*pResult = 0;
}

void Dialog_Region_New::OnButtonNewMap() 
{
	Dialog_New_Map dlgNewMap;
	dlgNewMap.DoModal();

	StringVec	maps;
	StringVec::iterator j;	
	GetMapDirNameColl( maps );		
	CComboBox *pCombo = (CComboBox *)this->GetDlgItem( IDC_MAPNAME );
	pCombo->ResetContent();
	for ( j = maps.begin(); j != maps.end(); j++ ) 
	{
		pCombo->AddString( (*j).c_str() );		
	}
}

void Dialog_Region_New::OnButtonDefaultScidRange() 
{
	CString rMap;
	m_mapName.GetWindowText( rMap );	
	m_scidRange.SetWindowText( gpstringf( "0x%08x", gEditorRegion.GetDefaultScidRange( rMap.GetBuffer( 0 ) ) ) );	
}

void Dialog_Region_New::OnButtonDefaultRegionId() 
{
	CString rMap;
	m_mapName.GetWindowText( rMap );	
	m_regionId.SetWindowText( gpstringf( "0x%08x", gEditorRegion.GetDefaultRegionId( rMap.GetBuffer( 0 ) ) ) );		
}

void Dialog_Region_New::OnSelchangeMapname() 
{
	CString rMap;
	m_mapName.GetWindowText( rMap );
	m_scidRange.SetWindowText( gpstringf( "0x%08x", gEditorRegion.GetDefaultScidRange( rMap.GetBuffer( 0 ) ) ) );
	m_regionId.SetWindowText( gpstringf( "0x%08x", gEditorRegion.GetDefaultRegionId( rMap.GetBuffer( 0 ) ) ) );	
	m_MeshRange.SetWindowText( gpstringf( "0x%08x", gEditorRegion.GetDefaultMeshRange( rMap.GetBuffer( 0 ) ) ) );			
}

void Dialog_Region_New::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);	
}

void Dialog_Region_New::OnButtonDefaultMeshRange() 
{
	CString rMap;
	m_mapName.GetWindowText( rMap );	
	m_MeshRange.SetWindowText( gpstringf( "0x%08x", gEditorRegion.GetDefaultMeshRange( rMap.GetBuffer( 0 ) ) ) );			
}
