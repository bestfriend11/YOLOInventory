// Dialog_Path_Batch_Generation.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Path_Batch_Generation.h"
#include "EditorRegion.h"
#include "EditorMap.h"
#include "ImageListDefines.h"
#include "Dialog_Path_Reporter.h"
#include "EditorTuning.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Path_Batch_Generation dialog


Dialog_Path_Batch_Generation::Dialog_Path_Batch_Generation(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Path_Batch_Generation::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Path_Batch_Generation)
	//}}AFX_DATA_INIT
}


void Dialog_Path_Batch_Generation::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Path_Batch_Generation)
	DDX_Control(pDX, IDC_CHECK_TUNING_LOAD, m_tuningLoad);
	DDX_Control(pDX, IDC_TREE_MAPS, m_tree_maps);
	DDX_Control(pDX, IDC_CHECK_MASTER_STATS, m_masterStats);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Path_Batch_Generation, CDialog)
	//{{AFX_MSG_MAP(Dialog_Path_Batch_Generation)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_ALL, OnButtonSelectAll)
	ON_BN_CLICKED(IDC_BUTTON_DESELECT_ALL, OnButtonDeselectAll)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_MAP, OnButtonSelectMap)
	ON_BN_CLICKED(IDC_BUTTON_DESELECT_MAP, OnButtonDeselectMap)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Path_Batch_Generation message handlers

BOOL Dialog_Path_Batch_Generation::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_tree_maps.DeleteAllItems();

	StringVec map_list;
	StringVec region_list;
	StringVec::iterator	i;
	StringVec::iterator	j;
	
	map_list.clear();
	gEditorRegion.GetMapList( map_list );		
	for ( i = map_list.begin(); i != map_list.end(); i++ ) 
	{
		HTREEITEM hParent = m_tree_maps.InsertItem( (char *)(*i).c_str(), IMAGE_CLOSEFOLDER, IMAGE_OPENFOLDER, TVI_ROOT, TVI_SORT );		
		region_list.clear();
		gEditorRegion.GetRegionList( (*i), region_list );
		for ( j = region_list.begin(); j != region_list.end(); ++j ) 
		{
			m_tree_maps.InsertItem( (char *)(*j).c_str(), IMAGE_REGION, IMAGE_REGION, hParent, TVI_SORT );
		}
	}	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Path_Batch_Generation::OnOK() 
{
	MapToRegionMap regionMap;

	HTREEITEM hItem = m_tree_maps.GetNextItem( TVI_ROOT, TVGN_ROOT );
	while ( hItem )
	{	
		HTREEITEM hChild = m_tree_maps.GetNextItem( hItem, TVGN_CHILD );
		while ( hChild )
		{
			if ( m_tree_maps.GetCheck( hChild ) )
			{
				HTREEITEM hParent = m_tree_maps.GetParentItem( hChild );
				if ( !m_tree_maps.GetCheck( hParent ) )
				{
					continue;
				}

				CString rMap	= m_tree_maps.GetItemText( hParent );
				CString rRegion = m_tree_maps.GetItemText( hChild );
				regionMap.insert( MapToRegionPair( rMap.GetBuffer( 0 ), rRegion.GetBuffer( 0 ) ) );
			}	

			hChild = m_tree_maps.GetNextItem( hChild, TVGN_NEXT );
		}

		hItem = m_tree_maps.GetNextItem( hItem, TVGN_NEXT );
	}

	Dialog_Path_Reporter dlgReporter;
	if ( dlgReporter.DoModal() == IDOK )
	{
		gEditorTuning.BatchGenerateTuningReports( regionMap, m_masterStats.GetCheck() ? true : false, m_tuningLoad.GetCheck() ? true : false );	
	}	
	
	CDialog::OnOK();
}

void Dialog_Path_Batch_Generation::OnButtonSelectAll() 
{
	HTREEITEM hItem = m_tree_maps.GetNextItem( TVI_ROOT, TVGN_ROOT );
	while ( hItem )
	{	
		m_tree_maps.SetCheck( hItem );
		HTREEITEM hChild = m_tree_maps.GetNextItem( hItem, TVGN_CHILD );
		while ( hChild )
		{
			m_tree_maps.SetCheck( hChild );
			hChild = m_tree_maps.GetNextItem( hChild, TVGN_NEXT );
		}
		
		hItem = m_tree_maps.GetNextItem( hItem, TVGN_NEXT );
	}	
}

void Dialog_Path_Batch_Generation::OnButtonDeselectAll() 
{
	HTREEITEM hItem = m_tree_maps.GetNextItem( TVI_ROOT, TVGN_ROOT );
	while ( hItem )
	{	
		m_tree_maps.SetCheck( hItem, FALSE );
		HTREEITEM hChild = m_tree_maps.GetNextItem( hItem, TVGN_CHILD );
		while ( hChild )
		{
			m_tree_maps.SetCheck( hChild, FALSE );
			hChild = m_tree_maps.GetNextItem( hChild, TVGN_NEXT );
		}
		
		hItem = m_tree_maps.GetNextItem( hItem, TVGN_NEXT );
	}		
}

void Dialog_Path_Batch_Generation::OnButtonSelectMap() 
{
	HTREEITEM hItem = m_tree_maps.GetSelectedItem();
	m_tree_maps.SetCheck( hItem );
	hItem = m_tree_maps.GetNextItem( hItem, TVGN_CHILD );
	while ( hItem )
	{
		m_tree_maps.SetCheck( hItem );
		hItem = m_tree_maps.GetNextItem( hItem, TVGN_NEXT );
	}		
}

void Dialog_Path_Batch_Generation::OnButtonDeselectMap() 
{
	HTREEITEM hItem = m_tree_maps.GetSelectedItem();
	m_tree_maps.SetCheck( hItem, FALSE );
	hItem = m_tree_maps.GetNextItem( hItem, TVGN_CHILD );
	while ( hItem )
	{
		m_tree_maps.SetCheck( hItem, FALSE );
		hItem = m_tree_maps.GetNextItem( hItem, TVGN_NEXT );
	}	
}

void Dialog_Path_Batch_Generation::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
