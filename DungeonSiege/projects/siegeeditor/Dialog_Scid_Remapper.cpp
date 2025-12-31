// Dialog_Scid_Remapper.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Scid_Remapper.h"
#include "EditorRegion.h"
#include "ImageListDefines.h"
#include "SCIDManager.h"
#include "worldindexbuilder.h"
#include "FuBiSchemaImpl.h"
#include "FuBiPersistImpl.h"
#include "GoData.h"
#include "GoEdit.h"
#include "GoSupport.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Scid_Remapper dialog


Dialog_Scid_Remapper::Dialog_Scid_Remapper(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Scid_Remapper::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Scid_Remapper)
	//}}AFX_DATA_INIT
}


void Dialog_Scid_Remapper::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Scid_Remapper)
	DDX_Control(pDX, IDC_EDIT_SCID_RANGE, m_scidRange);
	DDX_Control(pDX, IDC_TREE_REGIONS, m_regions);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Scid_Remapper, CDialog)
	//{{AFX_MSG_MAP(Dialog_Scid_Remapper)
	ON_BN_CLICKED(IDC_BUTTON_REMAP_SCIDS, OnButtonRemapScids)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Scid_Remapper message handlers

BOOL Dialog_Scid_Remapper::OnInitDialog() 
{
	CDialog::OnInitDialog();
		
	m_regions.SetImageList( gEditor.GetImageList(), TVSIL_NORMAL );
	m_regions.SetImageList( gEditor.GetImageList(), TVSIL_STATE );
	m_regions.DeleteAllItems();

	StringVec			map_list;
	StringVec			region_list;
	StringVec::iterator	i;
	StringVec::iterator	j;
	
	map_list.clear();
	gEditorRegion.GetMapList( map_list );		
	for ( i = map_list.begin(); i != map_list.end(); i++ ) 
	{
		HTREEITEM hParent = m_regions.InsertItem( (char *)(*i).c_str(), IMAGE_CLOSEFOLDER, IMAGE_OPENFOLDER, TVI_ROOT, TVI_SORT );		
		region_list.clear();
		gEditorRegion.GetRegionList( (*i), region_list );
		for ( j = region_list.begin(); j != region_list.end(); ++j ) 
		{
			m_regions.InsertItem( (char *)(*j).c_str(), IMAGE_REGION, IMAGE_REGION, hParent, TVI_SORT );
		}
	}	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Scid_Remapper::OnButtonRemapScids() 
{	
	CString rRegion = m_regions.GetItemText( m_regions.GetSelectedItem() );
	HTREEITEM hParent = m_regions.GetParentItem( m_regions.GetSelectedItem() );
	CString rMap	= m_regions.GetItemText( hParent );	
	
	CString rScidRange;
	m_scidRange.GetWindowText( rScidRange );
	int scidRange = 0;
	stringtool::Get( rScidRange, scidRange );


	FuelHandle hMaps( "world:maps" );
	FuelHandleList hlMaps = hMaps->ListChildBlocks( 1 );	
	FuelHandleList::iterator i;
	for ( i = hlMaps.begin(); i != hlMaps.end(); ++i )
	{
		gpstring sMap;
		sMap = (*i)->GetName();

		if ( sMap.same_no_case( rMap.GetBuffer( 0 ) ) )
		{
			if ( (*i)->GetChildBlock( "regions" ).IsValid() )
			{
				FuelHandleList hlRegions = (*i)->GetChildBlock( "regions" )->ListChildBlocks( 1 );
				FuelHandleList::iterator j;
				for ( j = hlRegions.begin(); j != hlRegions.end(); ++j )
				{
					gpstring sRegion = (*j)->GetName();					
					if ( sRegion.same_no_case( rRegion.GetBuffer( 0 ) ) )
					{
						RemapScidsForRegion( (*j), scidRange );
						return;
					}
				}
			}
		}
	}	
}


void Dialog_Scid_Remapper::RemapScidsForRegion( FuelHandle hRegion, int scidRange )
{
	gSCIDManager.ClearScids();
	gSCIDManager.SetScidRange( scidRange );

	FuelHandle hRegionSettings = hRegion->GetChildBlock( "region" );
	if ( hRegionSettings.IsValid() )
	{
		hRegionSettings->Set( "scid_range", scidRange );
	}

	FuelHandle hIndex = hRegion->GetChildBlock( "index" );
	FuelHandle hObjects = hRegion->GetChildBlock( "objects" );
	FuelHandleList hlObjects = hObjects->ListChildBlocks( 1 );
	FuelHandleList::iterator i;
	for ( i = hlObjects.begin(); i != hlObjects.end(); ++i )
	{
		gpstring sOldScid = (*i)->GetName();
		int oldScid = 0;
		stringtool::Get( sOldScid.c_str(), oldScid );
		gpstring sScid;
		sScid = ToString( gSCIDManager.GetScid() );
		(*i)->SetName( sScid.c_str() );
	}	
	
	gEditor.GetWorldIndexBuilder()->BuildRegionStreamerNodeContentIndex( hRegion, hIndex );		

	gSCIDManager.ClearScids();
}


