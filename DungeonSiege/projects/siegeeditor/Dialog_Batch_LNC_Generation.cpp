// Dialog_Batch_LNC_Generation.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Batch_LNC_Generation.h"
#include "EditorRegion.h"
#include "ImageListDefines.h"
#include "ComponentList.h"
#include "nema_aspectmgr.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace siege;

/////////////////////////////////////////////////////////////////////////////
// Dialog_Batch_LNC_Generation dialog


Dialog_Batch_LNC_Generation::Dialog_Batch_LNC_Generation(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Batch_LNC_Generation::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Batch_LNC_Generation)
	//}}AFX_DATA_INIT
}


void Dialog_Batch_LNC_Generation::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Batch_LNC_Generation)	
	DDX_Control(pDX, IDC_CHECK_CONVERT_GLOBAL_MAP, m_ConvertGlobalNodes);
	DDX_Control(pDX, IDC_CHECK_REMAP_TO_MESH_RANGE, m_MeshRangeRemap);
	DDX_Control(pDX, IDC_CHECK_FULL_RECALC, m_full_recalc);
	DDX_Control(pDX, IDC_CHECK_LOAD_LOGICAL, m_load_logical);
	DDX_Control(pDX, IDC_CHECK_LOAD_LIGHTS, m_load_lights);
	DDX_Control(pDX, IDC_CHECK_LOAD_DECALS, m_load_decals);
	DDX_Control(pDX, IDC_TREE_MAPS, m_tree_maps);
	DDX_Control(pDX, IDC_CHECK_FULLSAVE, m_full_save);
	DDX_Control(pDX, IDC_CHECK_DELETELNC, m_delete_lnc);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Batch_LNC_Generation, CDialog)
	//{{AFX_MSG_MAP(Dialog_Batch_LNC_Generation)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_ALL, OnButtonSelectAll)
	ON_BN_CLICKED(IDC_BUTTON_DESELECT_ALL, OnButtonDeselectAll)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Batch_LNC_Generation message handlers

void Dialog_Batch_LNC_Generation::OnOK() 
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
					hChild = m_tree_maps.GetNextItem( hChild, TVGN_NEXT );
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

	gEditorMap.GenerateLNCs( regionMap, m_full_save.GetCheck() ? true : false, m_delete_lnc.GetCheck() ? true : false, 
							 m_load_lights.GetCheck() ? true : false, m_load_decals.GetCheck() ? true : false, m_load_logical.GetCheck() ? true : false,
							 m_full_recalc.GetCheck() ? true : false, m_MeshRangeRemap.GetCheck() ? true : false, m_ConvertGlobalNodes.GetCheck() ? true : false );		
	
	CDialog::OnOK();
}

BOOL Dialog_Batch_LNC_Generation::OnInitDialog() 
{
	CDialog::OnInitDialog();
		
	//m_tree_maps.SetImageList( gEditor.GetImageList(), TVSIL_NORMAL );
	//m_tree_maps.SetImageList( gEditor.GetImageList(), TVSIL_STATE );
	m_tree_maps.DeleteAllItems();

	StringVec			map_list;
	StringVec			region_list;
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

void Dialog_Batch_LNC_Generation::OnButtonSelectAll() 
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

void Dialog_Batch_LNC_Generation::OnButtonDeselectAll() 
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

void Dialog_Batch_LNC_Generation::ConvertNodeIndex( MapToRegionMap & regionMap )
{
	MapToRegionMap::iterator i;
	for ( i = regionMap.begin(); i != regionMap.end(); ++i )
	{
		gpstring sMap		= (*i).first;
		gpstring sRegion	= (*i).second; 

		gpstring sMain;
		sMain.assignf( "world:maps:%s:start_positions", sMap.c_str() );
		FuelHandle hMain( sMain );
		if ( hMain.IsValid() )
		{
			FuelHandleList hlPositions = hMain->ListChildBlocks( 1 );
			FuelHandleList::iterator j;
			for ( j = hlPositions.begin(); j != hlPositions.end(); ++j )
			{
				gpstring sDesc;
				(*j)->Get( "description", sDesc );
				(*j)->Set( "description", sDesc, FVP_QUOTED_STRING );
			}
		}

		/*
		// Let's fix up the regions main.gas first
		{
			gpstring sMain;
			sMain.assignf( "world:maps:%s:regions:%s:region", sMap.c_str(), sRegion.c_str() );
			FuelHandle hMain( sMain );
			if ( hMain.IsValid() )
			{
				gpstring sCreated;
				gpstring sDesc;
				gpstring sGuid;
				int		 guid;
				gpstring sLastMod;
				gpstring sNotes;
				gpstring sScidRange;
				gpstring sVer;

				hMain->Get( "created", sCreated );
				hMain->Get( "description", sDesc );
				hMain->Get( "guid", guid );
				hMain->Get( "lastmodified", sLastMod );
				hMain->Get( "notes", sNotes );
				hMain->Get( "scid_range", sScidRange );
				hMain->Get( "se_version", sVer );

				hMain->ClearKeysAndChildren();

				hMain->Set( "created", sCreated );
				hMain->Set( "description", sDesc );
				sGuid.assignf( "0x%08X", guid );
				hMain->Set( "guid", sGuid );
				hMain->Set( "lastmodified", sLastMod );
				hMain->Set( "notes", sNotes );
				hMain->Set( "scid_range", sScidRange );
				hMain->Set( "se_version", sVer );
			}
		}


		gpstring sIndex;
		sIndex.assignf( "world:maps:%s:regions:%s:index:streamer_node_index", sMap.c_str(), sRegion.c_str() );
		FuelHandle hIndex( sIndex );
		if ( hIndex.IsValid() )
		{
			if ( hIndex->FindFirstKeyAndValue() )
			{
				StringVec guids;
				StringVec::iterator j;

				gpstring sNode;
				gpstring sAddress;
				while ( hIndex->GetNextKeyAndValue( sNode, sAddress ) )
				{
					guids.push_back( sNode );
				}

				FuelHandle hParent = hIndex->GetParent();
				hParent->DestroyChildBlock( hIndex );
				hIndex = hParent->CreateChildBlock( "streamer_node_index", "streamer_node_index.gas" );
				if ( hIndex.IsValid() )
				{
					for ( j = guids.begin(); j != guids.end(); ++j )
					{
						hIndex->Add( "*", *j, FVP_HEX_INT );
					}
				}
			}
		}	
		
		gpstring sNodes;
		sNodes.assignf( "world:maps:%s:regions:%s:terrain_nodes:siege_node_list", sMap.c_str(), sRegion.c_str() );
		FuelHandle hNodes( sNodes );
		if ( hNodes.IsValid() )
		{
			FuelHandleList hlNodes = hNodes->ListChildBlocks( 1 );
			FuelHandleList::iterator iNode;
			for ( iNode = hlNodes.begin(); iNode != hlNodes.end(); ++iNode )
			{
				int guid = 0;
				(*iNode)->Get( "guid", guid );
				gpstring sNew;
				sNew.assignf( "0x%08X", guid );
				(*iNode)->SetName( sNew );
			}
		}
		*/
	}
}


void Dialog_Batch_LNC_Generation::PruneBuildToRegions( MapToRegionMap & regionMap )
{
	MapToRegionMap::iterator i;

	std::set< int > delRegionIds;

	// Delete unneeded regions
	{
		std::set< gpstring, istring_less > usedRegions;
		std::set< gpstring, istring_less > usedMaps;
		
		for ( i = regionMap.begin(); i != regionMap.end(); ++i )
		{	
			gpstring sMap		= (*i).first;
			gpstring sRegion	= (*i).second; 

			usedRegions.insert( sRegion );
			usedMaps.insert( sMap );		
		}

		StringVec::iterator j;
		StringVec::iterator k;
		StringVec maps;
		StringVec regions;
		gEditorRegion.GetMapList( maps );		
		
		for ( j = maps.begin(); j != maps.end(); ++j )
		{
			gEditorRegion.GetRegionList( (*j).c_str(), regions );

			if ( usedMaps.find( *j ) == usedMaps.end() ) 
			{
				gpstring sMap;
				sMap.assignf( "world:maps:%s", (*j).c_str() );
				FuelHandle hMap( sMap );
				if ( hMap.IsValid() )
				{
					gpstring sFile = hMap->GetOriginPath();
					::DeleteFile( sFile );
				}
			}
			else
			{
				for ( k = regions.begin(); k != regions.end(); ++k )
				{
					gpstring sRegion;
					sRegion.assignf( "world:maps:%s:regions:%s", (*j).c_str(), (*k).c_str() );
					FuelHandle hRegion( sRegion );
					if ( hRegion.IsValid() )
					{
						FuelHandle hMain;
						int regionId = 0;
						hMain = hRegion->GetChildBlock( "region" );
						hMain->Get( "guid", regionId );
						delRegionIds.insert( regionId );
						gpstring sFile = hRegion->GetOriginPath();
						::DeleteFile( sFile );
					}					
				}
			}
		}	
	}

	std::set< gpstring, istring_less > usedFiles;
	for ( i = regionMap.begin(); i != regionMap.end(); ++i )
	{
		gpstring sMap		= (*i).first;
		gpstring sRegion	= (*i).second; 

		gEditorRegion.LoadRegion( sRegion, sMap );	
		
		// Get Node Files
		{
			siege::SiegeEngine& engine = gSiegeEngine;
			if ( engine.GetFrustum( gEditorRegion.GetFrustumId() ) )
			{		
				FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
				FrustumNodeColl::iterator iNode;
				for( iNode = nodeColl.begin(); iNode != nodeColl.end(); ++iNode )
				{
					SiegeNode * pNode = (*iNode);
					NodeTexColl textures = pNode->GetTextureListing();
					NodeTexColl::iterator iTex;
					for ( iTex = textures.begin(); iTex != textures.end(); ++iTex )
					{
						gpstring sTexture = gSiegeEngine.Renderer().GetTexturePathname( (*iTex).textureId );
						int pos = sTexture.find_last_of( "\\" );
						if ( pos != gpstring::npos )
						{
							sTexture = sTexture.substr( pos+1, sTexture.size() );
						}

						// $$$ IMPORTANT: cannot use hard-coded .psd or this will fail on tank -sb
						sTexture += ".psd";

						usedFiles.insert( sTexture );						
					}

					usedFiles.insert( gComponentList.GetMeshSnoFile( pNode->GetMeshGUID().ToString().c_str() ) );
				}
			}
		}
		
		// Get Object Files
		{
			GoidColl objects;
			GoidColl::iterator iObj;
			gGoDb.GetAllGlobalGoids( objects );		
			for ( iObj = objects.begin(); iObj != objects.end(); ++iObj ) 
			{
				GoHandle hObject( *iObj );
				gpstring sAspect = gAspectStorage.GetAspectName( hObject->GetAspect()->GetAspectHandle() );
				sAspect += ".asp";
				usedFiles.insert( sAspect );
				int numTex = hObject->GetAspect()->GetAspectHandle()->GetNumSubTextures();
				for ( int iTex = 0; iTex != numTex; ++iTex )
				{
					int texture = hObject->GetAspect()->GetAspectHandle()->GetTexture( iTex );
					gpstring sTexture = gSiegeEngine.Renderer().GetTexturePathname( texture );
					usedFiles.insert( sTexture );
				}
			}			
		}
	}
}
void Dialog_Batch_LNC_Generation::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);		
}
