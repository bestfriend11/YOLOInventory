//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorRegion.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "PrecompEditor.h"
#include "PrecompEditor.h"
#include "FuelDB.h"
#include "EditorRegion.h"
#include "LogicalNodeCreator.h"
#include "worldterrain.h"
#include "worldindexbuilder.h"
#include "worldmap.h"
#include "EditorTerrain.h"
#include "Stitch_Helper.h"
#include "siege_decal.h"
#include "siege_mesh_door.h"
#include "siege_options.h"
#include "Siege_Loader.h"
#include "EditorCamera.h"
#include "ComponentList.h"
#include "Dialog_Progress_Save.h"
#include "SiegeEditorDoc.h"
#include "GizmoManager.h"
#include "EditorObjects.h"
#include "Server.h"
#include "EditorLights.h"
#include "EditorHotpoints.h"
#include "EditorGizmos.h"
#include "Preferences.h"
#include "RapiPrimitive.h"
#include "ScidManager.h"
#include "Player.h"
#include "EditorTuning.h"
#include "Siege_Camera.h"
#include "FileSysUtils.h"
#include "LeftView.h"
#include "MeshPreviewer.h"
#include "Window_Mesh_Previewer.h"


// Namespaces
using namespace siege;


// Construction
EditorRegion::EditorRegion()
	: m_bRegionEnable( false )
	, m_bSaveBackups( false )
	, m_region_guid( 0 )
	, m_bLoading( false )
	, m_bRegionDirty( false )
	, m_MeshRange( 0 )
{
}


// Save the currently loaded region
void EditorRegion::SaveRegion( bool bGameObjects, bool bLights, bool bNodes, bool bStitch, bool bStartingPos )
{	
	if ( m_bRegionEnable == false ) 
	{
		// Just save whatever changes were made to fuel
		FuelHandle hRoot( "root" );
		hRoot->GetDB()->SaveChanges();
		return;
	}
	
	// This window monitors our progress
	Dialog_Progress_Save dlgSave;
	dlgSave.Create( IDD_DIALOG_PROGRESS_SAVE );

	// Create Backups of existing gas files, if user specified
	if ( GetSaveBackups() ) 
	{
		BackupGASFiles();
	}	

	// Save out the editing user information
	FuelHandle hRegion = GetRegionHandle()->GetChildBlock( "region" );
	hRegion->Set( "lastmodified", GetUserInformation() );
	gpstring sVer;
	gEditor.GetBuildTimeString( sVer );
	hRegion->Set( "se_version", sVer );	
	hRegion->Set( "guid", (int)GetRegionGUID(), FVP_HEX_INT );	

	// Go through all of the nodes in this region
	FuelHandle hNodes = GetRegionHandle()->GetChildBlock( "terrain_nodes:siege_node_list" );	

	if ( bNodes )
	{
		// Auto connect the nodes
		gEditorTerrain.AutoNodeConnection();
		gEditorTerrain.SaveNodes();
		gEditorTerrain.SaveLogicalFlags();
		gEditorTerrain.SaveLightLocks();

		// Build indicies if neccessary		
		gEditor.GetWorldIndexBuilder()->BuildRegionStreamerIndexes( GetRegionHandle(), false );		

		if ( gEditorMap.GetHasNodeMeshIndex() )
		{
			gEditor.GetWorldIndexBuilder()->BuildMapNodeMeshIndex( GetRegionHandle() );
		}

		// Write out the target node
		hNodes->Set( "targetnode", gEditorTerrain.GetTargetNode().GetValue(), FVP_HEX_INT );

		// Save out hotpoints
		gEditorHotpoints.Save();

		// Save out the decals
		gEditorGizmos.SaveDecalsToGas();		
	}
	
	if ( bGameObjects )
	{
		// Save out the game object information	
		dlgSave.SetDlgItemText( IDC_STATIC_INFO, "Saving Game Objects to File" );

		// Save out the Go information
		FuelHandle hEditObjects = GetRegionHandle()->GetChildBlock( "objects" );
		if ( !hEditObjects.IsValid() )
		{
			hEditObjects = GetRegionHandle()->CreateChildDirBlock( "objects" );		
		}
		else
		{
			FuelHandleList hlChildren = hEditObjects->ListChildBlocks();
			FuelHandleList::iterator iChild;
			for ( iChild = hlChildren.begin(); iChild != hlChildren.end(); ++iChild )
			{
				hEditObjects->DestroyChildBlock( *iChild );
			}
		}	

		// Do the actual saving
		gGoDb.SaveAllGos( hEditObjects );

		// Create files for all metagroups
		ContentDb::StringDb::iterator iMetaGroups;
		ContentDb::StringDb metaGroups;
		gContentDb.GetAllMetaGroupNames( metaGroups );
		FuelHandle hObjects = GetRegionHandle()->GetChildBlock( "objects" );
		for ( iMetaGroups = metaGroups.begin(); iMetaGroups != metaGroups.end(); ++iMetaGroups )
		{			
			gpstring sGasFile = hObjects->GetDB()->GetHomeWritePath();
			sGasFile += hObjects->GetOriginPath();
			sGasFile.appendf( "%s.gas", (*iMetaGroups) );
			HANDLE hFile = ::CreateFile( sGasFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
			CloseHandle( hFile );			
		}

		StringVec files;
		StringVec::iterator iFile;
		gpstring sObjectPath = hObjects->GetDB()->GetHomeWritePath();
		sObjectPath += hObjects->GetOriginPath();
		sObjectPath = sObjectPath.substr( 0, sObjectPath.size()-1 );
		GetFileList( files, sObjectPath );
		for ( iFile = files.begin(); iFile != files.end(); ++iFile )
		{
			bool bFound = false;
			for ( iMetaGroups = metaGroups.begin(); iMetaGroups != metaGroups.end(); ++iMetaGroups )
			{
				if ( (*iMetaGroups).same_no_case( "ui" ) )
				{
					continue;
				}

				if ( (*iFile).size() > 4 )
				{
					gpstring sGasFile = hObjects->GetDB()->GetHomeWritePath();
					sGasFile += hObjects->GetOriginPath();
					sGasFile.appendf( "%s.gas", (*iMetaGroups) );				
					if ( sGasFile.same_no_case( *iFile ) )
					{
						bFound = true;
						break;
					}
				}
				else
				{
					bFound = true;
					break;
				}				
			}

			if ( !bFound )
			{
				::DeleteFile( *iFile );
			}
		}		

		// Build Content Index
		FuelHandle hContent = GetRegionHandle()->GetChildBlock( "index" );
		FuelHandle hStreamerIndex = hContent->GetChildBlock( "streamer_node_content_index" );
		if ( hStreamerIndex.IsValid() )
		{
			hContent->DestroyChildBlock( hStreamerIndex );			
		}
		hContent = hContent->CreateChildBlock( "streamer_node_content_index", "streamer_node_content_index.gas" );				
		gGoDb.BuildNodeContentIndex( hContent );
	}		
	
	if ( bStitch )
	{
		// Save out any important stitch helper information
		gEditor.GetStitchHelper()->Save();
	}

	if ( bLights )
	{
		// Save out lighting information
		gEditorLights.SaveLightToGas();
	}

	if ( bStartingPos )
	{
		gEditorGizmos.SaveStartingPositions( gEditorRegion.GetMapHandle() );
	}		

	// Save out the user defined groups
	gGizmoManager.SaveGroups();

	// Save out the fuel tree	
	dlgSave.SetDlgItemText( IDC_STATIC_INFO, "Saving Changes in Fuel Tree" );	
	FuelHandle hRoot( "root" );
	hRoot->GetDB()->SaveChanges();

	// Calculate our progress bar
	CProgressCtrl * pProgress = (CProgressCtrl *)dlgSave.GetDlgItem( IDC_PROGRESS_SAVE );

	// Write out logical node information
	dlgSave.SetDlgItemText( IDC_STATIC_INFO, "Writing out Logical Node Information" );	
	
	if ( bNodes || bLights )
	{
		RecalculateLNC( hNodes, pProgress );
	}

	dlgSave.DestroyWindow();

	SetRegionDirty( false );
}


bool EditorRegion::LoadRegion( gpstring szName, gpstring szMap, bool bForceRecalc, bool bForceLoadLights, bool bForceLoadDecals, 
							   bool bTuningLoad, bool bForceLoadLogicalFlags, bool bFullRecalc )
{
	// Shut down mesh previewer to prevent any strange texture crashes
	if ( gPreviewer.GetPreviewerWindow() )
	{
		gPreviewer.GetPreviewerWindow()->CloseForLoad();
	}

	gEditorMap.SetHasNodeMeshIndex( false );

	//  This needs to work in order to keep fuel from saving previously loaded regions
	{
		gFuelSys.GetDefaultTextDb()->Reinit();
	}

	m_bLoading = true;
	gWorld.Shutdown( SHUTDOWN_FOR_NEW_GAME, true );
	gGizmoManager.Clear();	
	gEditor.GetLogicalNodeCreator()->ClearLogicalList();
	gEditorTerrain.SetSelectedNode( UNDEFINED_GUID );		

	if ( Server::DoesSingletonExist() == false )
	{
		gWorld.InitServer( false );
	}

	gEditorTerrain.InitNodeCallback();

	gGoDb.SetEditMode( true );

	gEditorRegion.SetRegionEnable( true );

	m_sRegionName	= szName;
	m_sMapName		= szMap;

	siege::SiegeEngine & engine = gSiegeEngine;
	gpassert( engine.LightDatabase().GetLightDatabaseMap().empty() );
	
	gpstring sMapAddress;
	sMapAddress.assignf( "world:maps:%s", m_sMapName.c_str() );
	SetMapHandle( FuelHandle( sMapAddress ) );

	gpstring sRegion;
	sRegion.assignf( "world:maps:%s:regions:%s", m_sMapName.c_str(), m_sRegionName.c_str() );
	FuelHandle hRegion( sRegion );
	
	if ( !hRegion.IsValid() ) 
	{
		MessageBox( gEditor.GetHWnd(), "Cannot load specified region.  Please check if the map/region exist.", "Load Region", MB_OK );
		m_bLoading = false;
		return false;
	}

	SetRegionHandle( hRegion );
	m_RegionDirectory = hRegion->GetOriginPath();

	// Create a light block if one does not exist
	if ( GetRegionHandle()->GetChildBlock( "lights" ) == false )
	{
		FuelHandle hLightDir = GetRegionHandle()->CreateChildDirBlock( "lights" );
		hLightDir->CreateChildBlock( "lights", "lights.gas" );
		hLightDir->GetDB()->SaveChanges();
	}

	FuelHandle hMain = GetMapHandle()->GetChildBlock( "map" );		
	bool bNodeMesh = false;
	hMain->Get( "use_node_mesh_index", bNodeMesh );
	gEditorMap.SetHasNodeMeshIndex( bNodeMesh );

	int meshRange = 0;
	FuelHandle hRegionMain = GetRegionHandle()->GetChildBlock( "region" );
	if ( hRegionMain )
	{
		hRegionMain->Get( "mesh_range", meshRange );		
		SetMeshRange( meshRange );
	}

	gLeftView.InitializeNodeList( true );

	if ( bNodeMesh )
	{	
		gEditorMap.RemapNodeMeshes();
	}

	FuelHandle hContent = GetRegionHandle()->GetChildBlock( "index" );
	if ( hContent )
	{	
		hContent = hContent->GetChildBlock( "streamer_node_content_index" );	
	}

	FuelHandle hRoot( "root" );

	bool bSave = true;
	if ( hContent.IsValid() )
	{
		// Check to see if we can save	
		{
			gpstring writePath;
			stringtool::ReplaceRootPath( hContent->GetOriginPath(), hContent->GetDB()->GetHomeReadPath(), hContent->GetDB()->GetHomeWritePath(), writePath );	
			FileSys::File writeFile;
			writeFile.OpenExisting( writePath, FileSys::File::eAccess::ACCESS_READ_WRITE );
			if ( !writeFile.IsOpen() )
			{
				bSave = false; 
			}
		}
		{
			FuelHandle hNodeIndex = hContent->GetParent();
			hNodeIndex = hNodeIndex->GetChildBlock( "streamer_node_index" );
			if ( hNodeIndex.IsValid() )
			{
				gpstring writePath;
				stringtool::ReplaceRootPath( hNodeIndex->GetOriginPath(), hNodeIndex->GetDB()->GetHomeReadPath(), hNodeIndex->GetDB()->GetHomeWritePath(), writePath );	
				FileSys::File writeFile;
				writeFile.OpenExisting( writePath, FileSys::File::eAccess::ACCESS_READ_WRITE );
				if ( !writeFile.IsOpen() )
				{
					bSave = false; 
				}
			}
		}
	}

	// Recalibrate Node Indicies to clear out any mistakes	
	gEditor.GetWorldIndexBuilder()->BuildRegionStreamerIndexes( hRegion );			
	
	if ( !bSave )
	{
		gpwarning( "Load Warning: Could not recalibrate streamer indices, the index files are likely marked read-only.\n" );
	}
	else
	{
		hRoot->GetDB()->SaveChanges();
		FastFuelHandle hBinRoot( "world:maps" );
		if ( hBinRoot )
		{
			hBinRoot.GetDb()->Unload();
		}
	}
	
	FuelHandle hGuid = GetRegionHandle()->GetChildBlock( "region" );
	if ( hGuid.IsValid() )
	{
		gpstring sGuid;
		if ( !hGuid->Get( "guid", sGuid ) ) 
		{
			GetRegionHandle()->Set( "guid", gEditorTerrain.GenerateRandomGUID(), FVP_HEX_INT );
		}
		siege::database_guid regionGuid( sGuid.c_str() );
		SetRegionGUID( (RegionId)regionGuid.GetValue() );		
	}	
		
	//	Load in map - only the clamped region
	int targetNode = 0;
	gpstring sNodes;
	sNodes.assignf( "%s:terrain_nodes:siege_node_list", GetRegionHandle()->GetAddress().c_str() );
	FuelHandle hNodes( sNodes );
	hNodes->Get( "targetnode", targetNode );	
	SiegePos spos;
	spos.node = siege::database_guid( targetNode );	
	
	gWorldOptions.SetTuningLoad( bTuningLoad );	

	gpstring sLNCFile = GetRegionHandle()->GetDB()->GetHomeWritePath();
	sLNCFile += GetRegionHandle()->GetOriginPath();
	sLNCFile += "terrain_nodes\\siege_nodes.lnc";
	
	if ( bFullRecalc )
	{
		::DeleteFile( sLNCFile );
	}

	//	Clamp map loading to region
	gWorldMap.RestrictStreamingToRegion( GetRegionGUID() );	

	if ( !gWorldMap.Init( GetMapName(), "" ) )
	{
		gpfatal( "This region will not load, please fix the errors before trying to load." );
		SetRegionEnable( false );
		m_bLoading = false;
		return false;
	}

	gWorldTerrain.RCRegisterWorldFrustumOnMachine( RPC_TO_LOCAL, gServer.GetScreenPlayer()->GetId(), 0 );	
	m_frustumId = gSiegeEngine.GetRenderFrustum();	
	siege::SiegeFrustum * pFrustum = gSiegeEngine.GetFrustum( m_frustumId );
	pFrustum->SetDimensions( 10000.0f, 10000.0f, 10000.0f );
	pFrustum->SetInterestRadius( 10000.0f );	
	
	gWorldMap.SAssignStartingPositions();
	pFrustum->SetPosition( spos );		
	gWorldMap.SeekAndDestroyDataRot();	

	if ( bTuningLoad )
	{
		gGoDb.SetEditMode( false );
	}

	gWorldTerrain.ForceWorldLoad();

	if ( bTuningLoad )
	{
		gGoDb.SetEditMode( true );
	}

	pFrustum->SetDimensions( 10000.0f, 10000.0f, 10000.0f );
	pFrustum->SetInterestRadius( 10000.0f );
	pFrustum->SetPosition( spos );		
	gSiegeEngine.UpdateFrustums( 0.0f );

	BuildRegionLNOs( GetRegionHandle(), bForceRecalc );
	
	// Set target node
	gEditorTerrain.SetTargetNode( siege::database_guid( targetNode ) );		

	// Construct Light Sources
	gGizmoManager.LoadLights();

	// Construct Decals
	gGizmoManager.LoadDecals();

	// Construct Hotpoints
	gEditorHotpoints.Load();

	// See if the .lnc file exists, if it does not, let's load up the lights from the gas file
	CFileFind finder;		
	BOOL bFoundLNC = finder.FindFile( sLNCFile );

	// Make sure the bounding values are properly initialized
	gEditorTerrain.ReloadBoundingValues();

	gEditorTerrain.LoadLightLocks();

	if ( !bFoundLNC || bForceLoadLights )
	{
		gEditorLights.LoadLightFromGas();
	}
	if ( !bFoundLNC || bForceLoadDecals )
	{
		gEditorGizmos.LoadDecalsFromGas();
	}	
	
	gEditorTerrain.LoadLogicalFlags();
		
	if ( !bFoundLNC )
	{
		gEditorTerrain.UpdateAllNodeLighting();
	}
	
	// Check real quick for any collisions
	gSCIDManager.CompileScidSet();
	gEditorTerrain.CompileGuidSet();

	// Load in any stitch information for the stitch helper
	gEditor.GetStitchHelper()->Load();

	gEditorGizmos.LoadStartingPositions( gEditorRegion.GetMapHandle() );

	// Set the camera target node position
	gEditorCamera.SetTargetPositionNode( gEditorTerrain.GetTargetNode() );

	if ( engine.GetOptions().SelectAllObjects() == false ) 
	{
		engine.GetOptions().ToggleSelectAllObjects();
	}

	// Set the ambient intensity
	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( gEditorTerrain.GetTargetNode() ) );
	SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
	float objAmbience = 0.0f;	
	hNodes->Get( "object_ambient_intensity", objAmbience );
	float actorAmbience = objAmbience;
	hNodes->Get( "actor_ambient_intensity", actorAmbience );
	gEditorTerrain.SetAmbientColor( node.GetAmbientColor(), node.GetObjectAmbientColor(), node.GetActorAmbientColor() ); 
	
	// Set the default camera position
	gEditorCamera.SetStartingMapCamera();
	gEditorCamera.SetTargetPositionNode( gEditorTerrain.GetTargetNode() );
	
	// Load in the groups
	gGizmoManager.LoadGroups();

	if ( bTuningLoad )
	{
		gGoDb.SetEditMode( false );
	}	

	// Bring all the go's into the world
	gWorldMap.LoadAllNodeContent();

	if ( bTuningLoad )
	{
		gGoDb.SetEditMode( true );
	}	

	gPreferences.SetShowObjects( gPreferences.GetShowObjects() );
	gPreferences.SetShowGizmos( gPreferences.GetShowGizmos() );

	// MFC fun
	gpstring sLoadName = szMap;
	sLoadName += "-";
	sLoadName += szName;
	AfxGetApp()->AddToRecentFileList( sLoadName.c_str() );
	gEditor.GetDocument()->SetRegionTitle( (char *)sLoadName.c_str() );	

	gLeftView.InitializeNodeList();
	gPreferences.SetClearColor( gPreferences.GetClearColor() );

	m_bLoading = false;

	SetRegionDirty();

	return true;
}


void EditorRegion::CreateRegion( RegionCreateInfo & regionCreation )
{		
	//  This needs to work in order to keep fuel from saving previously loaded regions
	{
		gFuelSys.GetDefaultTextDb()->Reinit();
	}
	
	gWorld.Shutdown( SHUTDOWN_FOR_NEW_GAME, true );
	gGizmoManager.Clear();	
	gEditor.GetLogicalNodeCreator()->ClearLogicalList();
	gEditorTerrain.SetSelectedNode( UNDEFINED_GUID );		

	if ( Server::DoesSingletonExist() == false )
	{
		gWorld.InitServer( false );
	}

	gEditorTerrain.InitNodeCallback();
	gGoDb.SetEditMode( true );	

	// Enable Editing
	SetRegionEnable( true );	
	
	regionCreation.sRegionName.to_lower();
	m_sRegionName	= regionCreation.sRegionName;
	m_sMapName		= regionCreation.sMapName;

	// Create all neccessary files and add them to the gas file
	SetupBlankRegion( regionCreation );
	
	// Set up the world frustum
	gWorldTerrain.RCRegisterWorldFrustumOnMachine( RPC_TO_LOCAL, gServer.GetScreenPlayer()->GetId(),	0 );
	m_frustumId = gSiegeEngine.GetRenderFrustum();
	siege::SiegeFrustum * pFrustum = gSiegeEngine.GetFrustum( m_frustumId );
	pFrustum->SetDimensions( 10000.0f, 10000.0f, 10000.0f );
	pFrustum->SetInterestRadius( 10000.0f );

	// Get the target node GUID		
	database_guid inGUID;
	
	gpstring szRandom = gEditorTerrain.GenerateRandomGUID();
	inGUID = database_guid( szRandom.c_str() );

	gWorldMap.InitLoaders();
	gLeftView.InitializeNodeList( true );
	
	// Create the node and its mesh
	SiegeNodeHandle handle(gSiegeEngine.NodeCache().UseObject( inGUID ));
	SiegeNode& node = handle.CreateObject( gSiegeEngine.NodeCache() );						
	node.SetTextureSetAbbr( gEditorTerrain.GetNodeSetCharacters() );
	node.SetTextureSetVersion( "" );
	node.SetGUID( inGUID );
	gpstring sRoot = RemoveDirectories( regionCreation.sSnoFile );
	gpstring sGuid = gComponentList.GetNodeGUID( (char *)sRoot.c_str() );
	database_guid inMeshGUID( sGuid );
	stringtool::RemoveExtension( sRoot );	
	node.SetMeshGUID( inMeshGUID );	
	const SiegeMesh& bmesh = node.GetMeshHandle().RequestObject(gSiegeEngine.MeshCache());
	node.SetNumLitVertices( bmesh.GetNumberOfVertices() );	
	node.SetIsLoadedPhysical( true );
	node.SetAmbientColor( fScaleDWORDColor( 0xffffffff, 1.0f ) );
	node.AccumulateStaticLighting();

	FuelHandle hNodes = GetRegionHandle()->GetChildBlock( "terrain_nodes:siege_node_list" );		
	hNodes->Set( "targetnode", inGUID.GetValue(), FVP_HEX_INT );	
	gpstring sNode;
	sNode.assignf( "%08x", inGUID.GetValue() );	
	
	// Set this node as our target node	
	gEditorTerrain.SetTargetNode( inGUID );

	SiegePos spos;
	spos.node = inGUID; 
	pFrustum->SetPosition( spos );
	
	// Save out the user who originally created the region
	FuelHandle hRegionMain = GetRegionHandle()->GetChildBlock( "region" );
	hRegionMain->Set( "created", GetUserInformation() );	

	// Create the map index folder	
	FuelHandle hIndex = GetMapHandle()->GetChildBlock( "index" );
	if ( !hIndex )
	{
		hIndex = GetMapHandle()->CreateChildDirBlock( "index" );
	}
	if ( hIndex.IsValid() ) 
	{
		if ( !hIndex->GetChildBlock( "stitch_index" ) )
		{
			hIndex->CreateChildBlock( "stitch_index", "stitch_index.gas" );
		}
	}	

	// Load up any existing starting positions
	gEditorGizmos.LoadStartingPositions( gEditorRegion.GetMapHandle() );

	// Add a default starting position
	SiegePos sStartPos;
	sStartPos.node = node.GetGUID();
	sStartPos.pos = node.GetCurrentCenter();
	gGizmoManager.AccurateAdjustPositionToTerrain( sStartPos );
	int startingPosId = gEditorGizmos.AddStartingPosition( sStartPos );
	gGizmoManager.SetStartingPosition( startingPosId, sStartPos );

	// Save the region
	SaveRegion( false, true, true, true, true );
	
	// Reinit binary fuel so we can initialize the map
	{
		FuelHandle hRoot( "root" );
		hRoot->GetDB()->SaveChanges();
		FastFuelHandle hBinRoot( "root" );
		hBinRoot.GetDb()->Unload();
		gWorldMap.Init( GetMapName(), "" );	
	}

	gSiegeEngine.GetFrustum( gEditorRegion.GetFrustumId() )->RequestUpdate();
	gSiegeEngine.UpdateFrustums( 0.0f, true );

	// Attach the new node to our logical node tree
	gEditor.GetLogicalNodeCreator()->AttachSiegeNode( gSiegeEngine, inGUID );
		
	// Set the camera target node position
	gEditorCamera.SetTargetPositionNode( gEditorTerrain.GetTargetNode() );	

	if ( gSiegeEngine.GetOptions().SelectAllObjects() == false ) 
	{
		gSiegeEngine.GetOptions().ToggleSelectAllObjects();
	}	

	gSCIDManager.CompileScidSet();
	gEditorTerrain.CompileGuidSet();

	gEditor.GetStitchHelper()->Load();
	gEditor.GetDocument()->SetRegionTitle( (char *)m_sRegionName.c_str() );
	
	// Tell the world about these changes
	// gWorldMap.OnEditorAddedRegionToMap( GetRegionGUID(), GetRegionName() );
	gWorldMap.OnEditorAddedNodeToMap( GetRegionGUID(), inGUID );

	// Save the first node
	{
		gpstring sGuid;
		sGuid.assignf( "0x%08X", inGUID.GetValue() );
		hNodes = GetRegionHandle()->GetChildBlock("terrain_nodes:siege_node_list" );
		FuelHandle hNode = hNodes->CreateChildBlock( sGuid.c_str() );
		if ( hNode.IsValid() )
		{
			hNode->SetType( "snode" );
			hNode->Set( "guid", inGUID.GetValue(), FVP_HEX_INT );
			hNode->Set( "mesh_guid", node.GetMeshGUID().GetValue(), FVP_HEX_INT );
			if ( !node.GetTextureSetAbbr().empty() )
			{
				hNode->Set( "texsetabbr", node.GetTextureSetAbbr().c_str() );
			}
			if ( !node.GetTextureSetVersion().empty() )
			{
				hNode->Set( "texsetver", node.GetTextureSetVersion().c_str() );
			}
		}
		gEditor.GetWorldIndexBuilder()->BuildRegionStreamerNodeIndex( GetRegionHandle(), GetRegionHandle()->GetChildBlock( "index" ) );
		
		FuelHandle hRoot( "root" );
		hRoot->GetDB()->SaveChanges();
	}

	gEditorCamera.OverheadView();

	// Create the north vector
	gEditorMap.CreateNorthVector( GetMapHandle()->GetAddress() );
	FuelHandle hNorth = GetMapHandle()->GetChildBlock( "info:hotpoints:north" );
	if ( hNorth )
	{
		int id = 0;
		hNorth->Get( "id", id );
		gEditorHotpoints.AddDirectionalHotpoint( vector_3( 1.0f, 0.0f, 0.0f ), id );
		hNorth->GetDB()->SaveChanges();
	}

	gEditorTerrain.ReloadBoundingValues();	
	gPreferences.SetClearColor( gPreferences.GetClearColor() );

	SetRegionDirty();
}


// Creates a blank region with all appropriate files
// and directories
void EditorRegion::SetupBlankRegion( RegionCreateInfo & regionCreation )
{
	gpstring sMapAddress;
	gpstring sRegionAddress;

	sMapAddress.assignf( "world:maps:%s", regionCreation.sMapName );
	SetMapHandle( FuelHandle( sMapAddress ) );

	{
		FuelHandle hMain = GetMapHandle()->GetChildBlock( "map" );		
		bool bNodeMesh = false;
		hMain->Get( "use_node_mesh_index", bNodeMesh );
		gEditorMap.SetHasNodeMeshIndex( bNodeMesh );
		if ( bNodeMesh )
		{			
			gEditorMap.RemapNodeMeshes();
		}
	}
	
	FuelHandle hRegions = GetMapHandle()->GetChildBlock( "regions" );
	SetRegionHandle( hRegions->CreateChildDirBlock( regionCreation.sRegionName ) );		
	m_RegionDirectory = GetRegionHandle()->GetOriginPath();
		
	FuelHandle hMain = GetRegionHandle()->CreateChildBlock( "region", "main.gas" );
	hMain->SetType( "region" );
	hMain->SetName( "region" );	
	hMain->Set( "description", (gpstring)"none" );
	hMain->Set( "notes", (gpstring)"none" );
	
	hMain->Set( "scid_range", regionCreation.scidRange, FVP_HEX_INT );
	hMain->Set( "mesh_range", regionCreation.meshRange, FVP_HEX_INT );
	SetMeshRange( regionCreation.meshRange );

	hMain->Set( "guid", regionCreation.regionId, FVP_HEX_INT );
	SetRegionGUID( (RegionId)regionCreation.regionId );
	gWorldMap.RestrictStreamingToRegion( GetRegionGUID() );	
	
	FuelHandle hNodes =	 GetRegionHandle()->CreateChildDirBlock( "terrain_nodes" );
	FuelHandle hNodeList = hNodes->CreateChildBlock( "siege_node_list", "nodes.gas" );
	hNodeList->SetType( "terrain_nodes" );
	hNodeList->Set( "ambient_intensity", 1.0f );				
	hNodeList->Set( "object_ambient_intensity", 1.0f );
	hNodeList->Set( "actor_ambient_intensity", 1.0f );
	hNodeList->Set( "ambient_color", 0xffffffff, FVP_HEX_INT );
	hNodeList->Set( "object_ambient_color", 0xffffffff, FVP_HEX_INT );
	hNodeList->Set( "actor_ambient_color", 0xffffffff, FVP_HEX_INT );
	
	// Create the appropriate files and directories needed for a new blank region
	GetRegionHandle()->CreateChildDirBlock( "objects" );		
	GetRegionHandle()->CreateChildDirBlock( "index" );

	FuelHandle hLight =	GetRegionHandle()->CreateChildDirBlock( "lights" );
	FuelHandle hMainLight = hLight->CreateChildBlock( "lights", "lights.gas" );	
	if ( !hMainLight.IsValid() ) 
	{
		MessageBox( gEditor.GetHWnd(), "File Open in Region Creation Failed.", "Error", MB_OK );
	}
	
	// Create the map stitch index folder
	FuelHandle hIndex;
	if ( !GetMapHandle()->GetChildBlock( "index", hIndex ) ) 
	{
		GetMapHandle()->CreateChildDirBlock( "index" );
		if ( hIndex.IsValid() ) 
		{
			hIndex->CreateChildBlock( "stitch_index", "stitch_index.gas" );
		}		
	}
	
	// Reset the file system and GAS tree in order to read in the new 
	// files that were just created.
	GetRegionHandle()->GetDB()->Save( false );
}


int EditorRegion::GetDefaultScidRange( gpstring sMap )
{
	gpstring sMapAddress;
	sMapAddress.assignf( "world:maps:%s", sMap.c_str() );
	FuelHandle hMap( sMapAddress );
	int tempRange = 0;
	if ( hMap )
	{
		FuelHandleList hlRegions = hMap->ListChildBlocks( 3, "region" );
		FuelHandleList::iterator a;
		int range = 0;		
		for ( a = hlRegions.begin(); a != hlRegions.end(); ++a )
		{			
			(*a)->Get( "scid_range", range );
			if ( range > tempRange )
			{
				tempRange = range;
			}
		}	
	}

	return ++tempRange;
}


int EditorRegion::GetDefaultRegionId( gpstring sMap )
{
	gpstring sMapAddress;
	sMapAddress.assignf( "world:maps:%s", sMap.c_str() );
	FuelHandle hMap( sMapAddress );
	if ( hMap )
	{
		FuelHandleList hlRegions = hMap->ListChildBlocks( 3, "region" );
		FuelHandleList::iterator a;
		int guid = 0;
		int tempGuid = 0;
		for ( a = hlRegions.begin(); a != hlRegions.end(); ++a )
		{			
			(*a)->Get( "guid", guid );
			if ( guid > tempGuid )
			{
				tempGuid = guid;
			}
		}	

		return ++tempGuid;
	}	

	return 0;
}


int	EditorRegion::GetDefaultMeshRange( gpstring sMap, gpstring sRegion )
{
	gpstring sMapAddress;
	sMapAddress.assignf( "world:maps:%s", sMap.c_str() );
	FuelHandle hMap( sMapAddress );
	if ( hMap )
	{
		FuelHandleList hlRegions = hMap->ListChildBlocks( 3, "region" );
		FuelHandleList::iterator a;
		int range = 0;
		int tempRange = 0;
		for ( a = hlRegions.begin(); a != hlRegions.end(); ++a )
		{		
			if ( sRegion.same_no_case( (*a)->GetParent()->GetName() ) )
			{
				if ( (*a)->Get( "mesh_range", range ) )
				{
					return range;
				}
			}

			(*a)->Get( "mesh_range", range );
			if ( range > tempRange )
			{
				tempRange = range;
			}						
		}	

		return ++tempRange;
	}	

	return 0;
}


bool EditorRegion::IsValidScidRange( gpstring sMap, int scidRange )
{
	gpstring sMapAddress;
	sMapAddress.assignf( "world:maps:%s", sMap.c_str() );
	FuelHandle hMap( sMapAddress );
	if ( hMap )
	{
		FuelHandleList hlRegions = hMap->ListChildBlocks( 3, "region" );
		FuelHandleList::iterator a;
		int range = 0;	
		for ( a = hlRegions.begin(); a != hlRegions.end(); ++a )
		{			
			(*a)->Get( "scid_range", range );
			if ( range == scidRange )
			{
				return false;
			}
		}	

		return true;
	}

	return false;
}


bool EditorRegion::IsValidRegionId( gpstring sMap, int regionId )
{
	gpstring sMapAddress;
	sMapAddress.assignf( "world:maps:%s", sMap.c_str() );
	FuelHandle hMap( sMapAddress );
	if ( hMap )
	{
		FuelHandleList hlRegions = hMap->ListChildBlocks( 3, "region" );
		FuelHandleList::iterator a;
		int guid = 0;	
		for ( a = hlRegions.begin(); a != hlRegions.end(); ++a )
		{			
			(*a)->Get( "guid", guid );
			if ( regionId == guid )
			{
				return false;
			}
		}
		
		return true;
	}

	return false;
}


bool EditorRegion::IsValidMeshRange( gpstring sMap, int meshRange )
{
	gpstring sMapAddress;
	sMapAddress.assignf( "world:maps:%s", sMap.c_str() );
	FuelHandle hMap( sMapAddress );
	if ( hMap )
	{
		FuelHandleList hlRegions = hMap->ListChildBlocks( 3, "region" );
		FuelHandleList::iterator a;
		int range = 0;	
		for ( a = hlRegions.begin(); a != hlRegions.end(); ++a )
		{			
			(*a)->Get( "mesh_range", range );
			if ( meshRange == range )
			{
				return false;
			}
		}
		
		return true;
	}

	return false;
}


void EditorRegion::GetMapList( StringVec & map_list )
{
	// Reintialize the maps
	{		
		FastFuelHandle maps_parent( "world:maps" );
		maps_parent.Unload();
	}
	FastFuelHandle maps_parent( "world:maps" );
	m_regions.clear();

	gpassert( maps_parent.IsValid() );
	FastFuelHandleColl maps;
	maps_parent.ListChildren( maps, 1 );
	for( FastFuelHandleColl::iterator j = maps.begin(); j != maps.end(); ++j ) 
	{				
		gpstring name;
		m_maps.push_back( (*j).GetName() );
		map_list.push_back( (*j).GetName() );
	}	
}


void EditorRegion::GetRegionList( gpstring map_name, StringVec & region_list )
{
	gpstring sMap;
	sMap.assignf( "world:maps:%s:regions", map_name.c_str() );

	// Reintialize the regions
	{			
		FastFuelHandle hMap( sMap );
		if ( hMap )
		{
			hMap.Unload();
		}
	}
	FastFuelHandle hMap( sMap );	
	
	if ( hMap.IsValid() )
	{
		FastFuelHandleColl hlRegions;
		hMap.ListChildren( hlRegions, 1 );
		FastFuelHandleColl::iterator j;
		for( j = hlRegions.begin(); j != hlRegions.end(); ++j ) 
		{		
			REGION_DATA rData;
			rData.fh_region = *j;
			rData.map_name = map_name;
			m_regions.push_back( rData );
			region_list.push_back( (*j).GetName() );
		}
	}
}


// Recalculate all the LNC information
void EditorRegion::RecalculateLNC( FuelHandle hNodes, CProgressCtrl * pProgress )
{
	FuelHandleList hlNodes = hNodes->ListChildBlocks( 1, "snode" );

	pProgress->SetRange( 1, hlNodes.size() );
	pProgress->SetStep( 1 );

	// Write out the logical information
	gpstring logical_dir( hNodes->GetDB()->GetHomeWritePath() );
	logical_dir.append( m_RegionDirectory );
	logical_dir.append( "terrain_nodes\\" );	

	// Save out the information for this node
	gpstring filename	= logical_dir;
	filename				+= "siege_nodes.lnc";

	gpstring sRelLnc = m_RegionDirectory;
	sRelLnc.append( "terrain_nodes\\siege_nodes.lnc" );

	gSiegeEngine.NodeDatabase().CloseLogicalFile( sRelLnc );
	
	// Create the file		
	HANDLE hFile  = ::CreateFile( filename.c_str(),
								  GENERIC_WRITE,
								  0,
								  0,
								  CREATE_ALWAYS,
								  FILE_ATTRIBUTE_NORMAL,
								  0 );		

	FrustumNodeColl nodeColl = gSiegeEngine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();				

	// Write the number of nodes in the file
	unsigned int num_lnodes	= nodeColl.size();

	unsigned long temp_bytes_written = 0;
	::WriteFile( hFile, &num_lnodes, sizeof( unsigned int ), &temp_bytes_written, 0 );

	// Write the index
	unsigned int current_offset	= sizeof( unsigned int ) + ( (sizeof( unsigned int ) * 3) * num_lnodes);
	
	// Apply any stoppage if collision with terrain occurred	
	FrustumNodeColl::iterator i;
	for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
	{		
		database_guid nodeGuid( (*i)->GetGUID() );

		unsigned int guid_value	= nodeGuid.GetValue();			
		::WriteFile( hFile, &guid_value, sizeof( unsigned int ), &temp_bytes_written, 0 );

		// Get the sizeof of this node
		unsigned int sizeOfLNO = gEditor.GetLogicalNodeCreator()->GetSizeOfLNO( nodeGuid );

		// Add the size of the lighting information
		SiegeNodeHandle handle(gSiegeEngine.NodeCache().UseObject(nodeGuid));
		SiegeNode& node = handle.RequestObject(gSiegeEngine.NodeCache());	

		// Get info about this node
		unsigned int numLights		= node.GetStaticLights().size();
		unsigned int numVertices	= node.GetNumLitVertices();

		// Calculate the size of the lighting info
		sizeOfLNO	+= sizeof( unsigned int ) * 2;

		sizeOfLNO	+= numLights * sizeof( unsigned int );
		sizeOfLNO	+= numLights * (sizeof( vector_3 ) * 2);
		sizeOfLNO	+= numLights * sizeof( siege::LightDescriptor );

		for( SiegeLightList::const_iterator l = node.GetStaticLights().begin(); l != node.GetStaticLights().end(); ++l )
		{
			if( (*l).m_Descriptor.m_bAffectsTerrain && (*l).m_pLightEffect )
			{
				sizeOfLNO	+= (sizeof( float ) * numVertices);
			}
		}

		// Get the number of hotpoint and write it out
		sizeOfLNO	+= sizeof( unsigned int );

		unsigned int numHotpoints	= node.GetHotpointMap().size();
		sizeOfLNO	+= numHotpoints * sizeof( unsigned int );
		sizeOfLNO	+= numHotpoints * sizeof( vector_3 );

		// Decals
		sizeOfLNO	+= sizeof( unsigned int );

		for( siege::DecalIndexList::const_iterator dil = node.GetDecalIndexList().begin(); dil != node.GetDecalIndexList().end(); ++dil )
		{
			siege::SiegeDecal* pDecal	= gSiegeDecalDatabase.GetDecalPointer( (*dil) );

			if( pDecal )
			{
				// version info
				sizeOfLNO	+= sizeof( unsigned int );

				// projection information
				sizeOfLNO	+= sizeof( database_guid );
				sizeOfLNO	+= sizeof( float );
				sizeOfLNO	+= sizeof( bool );
				sizeOfLNO	+= sizeof( BYTE );
				sizeOfLNO	+= sizeof( vector_3 );
				sizeOfLNO	+= sizeof( database_guid );
				sizeOfLNO	+= sizeof( matrix_3x3 );
				sizeOfLNO	+= sizeof( float );
				sizeOfLNO	+= sizeof( float );
				sizeOfLNO	+= sizeof( float );
				sizeOfLNO	+= sizeof( float );

				// texture information
				char tex_name[ 256 ];
				memset( tex_name, 0, 256 );

				const char* pTex	= gDefaultRapi.GetTexturePathname( pDecal->GetDecalTexture() );
				_splitpath( pTex, NULL, NULL, tex_name, NULL );

				sizeOfLNO	+= strlen( tex_name ) + 1;

				// geometry
				sizeOfLNO	+= sizeof( unsigned int );

				for( siege::DecalRenderColl::iterator drm = pDecal->GetDecalRenderColl().begin(); drm != pDecal->GetDecalRenderColl().end(); ++drm )
				{
					// node guid
					sizeOfLNO	+= sizeof( database_guid );

					unsigned int numVertices	= (*drm).renderSet.m_numVertices;
					sizeOfLNO	+= sizeof( unsigned int );

					for( unsigned int o = 0; o < numVertices; ++o )
					{
						sizeOfLNO	+= sizeof( DecalVertex );
					}
				}
			}
			else
			{
				gperror( "Decal in node could not be written because it could not be found in the database!" );
			}
		}

		// Write the offset and size			
		::WriteFile( hFile, &current_offset, sizeof( unsigned int ), &temp_bytes_written, 0 );			
		::WriteFile( hFile, &sizeOfLNO, sizeof( unsigned int ), &temp_bytes_written, 0 );

		// Add the size of this LNO to the offset for the next node
		current_offset	+= sizeOfLNO;
	}

	// Write the data
	FrustumNodeColl::iterator iNode;
	for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
	{		
		database_guid nodeGuid( (*i)->GetGUID() );
		
		// Write the file out to disk
		gEditor.GetLogicalNodeCreator()->WriteLogicalInformation( gSiegeEngine, nodeGuid, hFile );

		// Write out lighting info
		SiegeNodeHandle handle(gSiegeEngine.NodeCache().UseObject(nodeGuid));
		SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );

		// Get the number of vertices and write it out
		unsigned int numVertices	= node.GetNumLitVertices();			
		::WriteFile( hFile, &numVertices, sizeof( unsigned int ), &temp_bytes_written, 0 );

		// Get the number of static lights and write it out
		unsigned int numLights		= node.GetStaticLights().size();			
		::WriteFile( hFile, &numLights, sizeof( unsigned int ), &temp_bytes_written, 0 );			

		// Go through all of the lights and write out their information
		for( siege::SiegeLightList::const_iterator l = node.GetStaticLights().begin(); l != node.GetStaticLights().end(); ++l )
		{
			// Write out the guid that identifies this light
			unsigned int guid_value	= (*l).m_Guid.GetValue();				
			::WriteFile( hFile, &guid_value, sizeof( unsigned int ), &temp_bytes_written, 0 );

			// Write out the position relative to this node				
			::WriteFile( hFile, (void*)&(*l).m_Position, sizeof( vector_3 ), &temp_bytes_written, 0 );

			// Write out the direction relative to this node				
			::WriteFile( hFile, (void*)&(*l).m_Direction, sizeof( vector_3 ), &temp_bytes_written, 0 );

			// Write out the descriptor
			gpassertm( (*l).m_Descriptor.m_Subtype == LS_STATIC, "Cannot save out dynamic light!" );				
			::WriteFile( hFile, (void*)&(*l).m_Descriptor, sizeof( LightDescriptor ), &temp_bytes_written, 0 );

			// Write out the intensity map
			if( (*l).m_Descriptor.m_bAffectsTerrain && (*l).m_pLightEffect )
			{					
				::WriteFile( hFile, (*l).m_pLightEffect, sizeof( float ) * numVertices, &temp_bytes_written, 0 );
			}
		}

		// Get the number of hotpoint and write it out
		unsigned int numHotpoints	= node.GetHotpointMap().size();			
		::WriteFile( hFile, &numHotpoints, sizeof( unsigned int ), &temp_bytes_written, 0 );

		// Go through all of the hotpoints and save them to the file
		for( siege::SiegeHotpointMap::const_iterator h = node.GetHotpointMap().begin(); h != node.GetHotpointMap().end(); ++h )
		{
			// Save out the id
			unsigned int id	= (*h).first;				
			::WriteFile( hFile, &id, sizeof( unsigned int ), &temp_bytes_written, 0 );

			// Save out the direction
			vector_3 direction = (*h).second;				
			::WriteFile( hFile, &direction, sizeof( vector_3 ), &temp_bytes_written, 0 );
		}

		// Decals
		unsigned int numDecals	= node.GetDecalIndexList().size();			
		::WriteFile( hFile, &numDecals, sizeof( unsigned int ), &temp_bytes_written, 0 );

		for( siege::DecalIndexList::const_iterator dil = node.GetDecalIndexList().begin(); dil != node.GetDecalIndexList().end(); ++dil )
		{
			SiegeDecal* pDecal	= gSiegeDecalDatabase.GetDecalPointer( (*dil) );

			if( pDecal )
			{
				// Write the version info					
				::WriteFile( hFile, (void*)&siege::DECAL_VERSION, sizeof( unsigned int ), &temp_bytes_written, 0 );

				// Write the unique guid					
				::WriteFile( hFile, &pDecal->GetGUID(), sizeof( database_guid ), &temp_bytes_written, 0 );

				// Write the level of detail
				float floatWrite = pDecal->GetLevelOfDetail();
				::WriteFile( hFile, &floatWrite, sizeof( float ), &temp_bytes_written, 0 );

				// Write active state
				bool boolWrite	= pDecal->GetActive();					
				::WriteFile( hFile, &boolWrite, sizeof( bool ), &temp_bytes_written, 0 );

				// Write alpha
				BYTE byteWrite	= pDecal->GetAlpha();					
				::WriteFile( hFile, &byteWrite, sizeof( BYTE ), &temp_bytes_written, 0 );

				// Write in the projection information					
				::WriteFile( hFile, &pDecal->GetDecalOrigin().pos, sizeof( vector_3 ), &temp_bytes_written, 0 );					
				::WriteFile( hFile, &pDecal->GetDecalOrigin().node, sizeof( database_guid ), &temp_bytes_written, 0 );				
				::WriteFile( hFile, &pDecal->GetDecalOrientation(), sizeof( matrix_3x3 ), &temp_bytes_written, 0 );

				floatWrite = pDecal->GetAngle();					
				::WriteFile( hFile, &floatWrite, sizeof( float ), &temp_bytes_written, 0 );

				floatWrite = pDecal->GetAspect();					
				::WriteFile( hFile, &floatWrite, sizeof( float ), &temp_bytes_written, 0 );

				floatWrite = pDecal->GetNearPlane();					
				::WriteFile( hFile, &floatWrite, sizeof( float ), &temp_bytes_written, 0 );

				floatWrite = pDecal->GetFarPlane();
				::WriteFile( hFile, &floatWrite, sizeof( float ), &temp_bytes_written, 0 );

				// Write in the texture information
				char tex_name[ 256 ];
				memset( tex_name, 0, 256 );

				const char* pTex	= gDefaultRapi.GetTexturePathname( pDecal->GetDecalTexture() );
				_splitpath( pTex, NULL, NULL, tex_name, NULL );
				
				::WriteFile( hFile, (void*)tex_name, strlen( tex_name ) + 1, &temp_bytes_written, 0 );

				// Write geometry
				unsigned int numDecalSets	= pDecal->GetDecalRenderColl().size();					
				::WriteFile( hFile, &numDecalSets, sizeof( unsigned int ), &temp_bytes_written, 0 );

				for( siege::DecalRenderColl::iterator drm = pDecal->GetDecalRenderColl().begin(); drm != pDecal->GetDecalRenderColl().end(); ++drm )
				{
					// Write the node guid
					database_guid node	= (*drm).node;						
					::WriteFile( hFile, &node, sizeof( database_guid ), &temp_bytes_written, 0 );
					
					::WriteFile( hFile, &(*drm).renderSet.m_numVertices, sizeof( unsigned int ), &temp_bytes_written, 0 );

					for( unsigned int o = 0; o < (*drm).renderSet.m_numVertices; ++o )
					{
						siege::DecalVertex vert;
						vert.x	= (*drm).renderSet.m_pVertices[o].x;
						vert.y	= (*drm).renderSet.m_pVertices[o].y;
						vert.z	= (*drm).renderSet.m_pVertices[o].z;
						vert.u	= (*drm).renderSet.m_pVertices[o].uv.u;
						vert.v	= (*drm).renderSet.m_pVertices[o].uv.v;

						vert.wl	= (*drm).renderSet.m_pPerspCorrect[o];
						vert.li = (*drm).renderSet.m_pLightInterp[o];
						
						::WriteFile( hFile, &vert, sizeof( DecalVertex ), &temp_bytes_written, 0 );							
					}
				}
			}
			else
			{
				gperror( "Could not write decal to disk because it is not in the collection!" );
			}
		}

		// Show Progress on Progress Bar
		pProgress->StepIt();
	}

	// Close the file
	if ( hFile != NULL && hFile != INVALID_HANDLE_VALUE )
	{
		::CloseHandle( hFile );
	}		

	gSiegeEngine.NodeDatabase().OpenLogicalFile( sRelLnc );
}


void EditorRegion::SaveLNC()
{
	// This window monitors our progress
	Dialog_Progress_Save dlgSave;
	dlgSave.Create( IDD_DIALOG_PROGRESS_SAVE );

	FuelHandle hNodes = GetRegionHandle()->GetChildBlock( "terrain_nodes:siege_node_list" );
	
	// Calculate our progress bar
	CProgressCtrl * pProgress = (CProgressCtrl *)dlgSave.GetDlgItem( IDC_PROGRESS_SAVE );
	
	// Write out logical node information
	dlgSave.SetDlgItemText( IDC_STATIC_INFO, "Writing out Logical Node Information" );	

	// Save out the information for this node
	RecalculateLNC( hNodes, pProgress );

	dlgSave.DestroyWindow();
}


bool EditorRegion::BuildRegionLNOs( FuelHandle const & hRegionDir, bool bForceRecalc )
{
	gpgenericf(( "WorldTerrain::BuildRegionLNO - building LNOs for nodes in region %s\n", hRegionDir->GetAddress().c_str() ));

	SiegeEngine & engine = gSiegeEngine;

	//siege::database_guid TargetNodeGUID( gWorldMapGetTargetNodeAddress() );
	siege::database_guid TargetNodeGUID;

	//----- get nodes list for region

	FuelHandle hNodesDir = hRegionDir->GetChildBlock( "terrain_nodes" );
	gpassert( hNodesDir.IsValid() );

	FuelHandleList TerrainNodes;
	if( !hNodesDir->ListChildBlocks( 1, NULL, "siege_node_list", TerrainNodes ) ) {
		gpassertm( 0, "This region doesn't seem to have any nodes." );
		return( false );
	}
	gpassert( TerrainNodes.size() == 1 );

	FuelHandle hNodeCollection = TerrainNodes.front();

	FuelHandleList nodes;

	if( !hNodeCollection->ListChildBlocks( 1, "snode", NULL, nodes ) ) {
		gpassert( 0 );	// no nodes defined!
		return( false );
	}

	std::vector< siege::database_guid > traversalList;

	//----- build LNO for each node

	bool bSuccess = true;

	// Go through the nodes in this map and setup their doors as a first pass
	for( FuelHandleList::iterator iNode = nodes.begin(); iNode != nodes.end(); ++iNode )
	{
		// get guid
		gpstring sNodeGUID;
		if( !(*iNode)->Get( "guid", sNodeGUID, false ) ) {
			gpassert( 0 );
		}
		siege::database_guid NodeGUID( sNodeGUID.c_str() );
		gpassert( NodeGUID.IsValid() );

		traversalList.push_back( NodeGUID );		
	}

	// Do the LNC for this node
	gpstring sFilename = hRegionDir->GetDB()->GetHomeWritePath();
	sFilename.append( hRegionDir->GetOriginPath() );

	sFilename += "terrain_nodes\\";
	sFilename += "siege_nodes.lnc";

	// Open the file
	HANDLE hFile = NULL;

	// Go through all the nodes in a secondary pass and generate or read in LNO information
	for( std::vector< siege::database_guid >::iterator g = traversalList.begin(); g != traversalList.end(); ++g )
	{
		// get a handle to the SiegeNode
		SiegeNode* pNode = gSiegeEngine.IsNodeValid( *g );
		if ( !pNode )
		{
			gperrorf(( "SiegeEditor found a bad node.  This node will not be saved out: 0x%08X", (*g).GetValue() ));
			continue;
		}

		if( !pNode->VerifyChecksum() )
		{
			gpgeneric( "SiegeEditor will force recalc this region due to new mesh data.\n" );
			bForceRecalc = true;
			break;;
		}
	}
	
	if( !bForceRecalc )
	{		
		hFile = ::CreateFile( sFilename.c_str(), GENERIC_READ, FILE_SHARE_READ,
							  0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
							  0 );		
	}

	// Read in the offset table
	OffsetTableMap lno_map;
	if( hFile )
	{
		BuildFileOffsetTable( hFile, &lno_map );
	}

	// Go through all the nodes in a secondary pass and generate or read in LNO information
	for( g = traversalList.begin(); g != traversalList.end(); ++g )
	{
		// get a handle to the SiegeNode
		SiegeNode* pNode = gSiegeEngine.IsNodeValid( *g );
		bool bUpdateLighting = false;

		// Attach node to the tree
		if( hFile )
		{
			OffsetTableMap::iterator i = lno_map.find( (*g).GetValue() );
			if( i != lno_map.end() )
			{
				// Reset the file
				LARGE_INTEGER offset;
				offset.QuadPart = (*i).second;
				offset.LowPart = ::SetFilePointer( (HANDLE)hFile, offset.LowPart, &offset.HighPart, FILE_BEGIN );				

				if( !gEditor.GetLogicalNodeCreator()->ReadLogicalInformation( engine, (*g) ) )
				{
					// If read fails, means that checksum failed.  Do it the hard way....
					gEditor.GetLogicalNodeCreator()->AttachSiegeNode( engine, (*g) );

					// Tell the system about updates
					bUpdateLighting = true;
				}
			}
			else
			{
				// If read fails, means that checksum failed.  Do it the hard way....
				gEditor.GetLogicalNodeCreator()->AttachSiegeNode( engine, (*g) );

				// Tell the system about updates
				bUpdateLighting = true;
			}

			// Reset the file			

			LARGE_INTEGER offset;
			offset.QuadPart = 0;
			offset.LowPart = ::SetFilePointer( (HANDLE)hFile, offset.LowPart, &offset.HighPart, FILE_BEGIN );

		}
		else
		{
			// Attach node to the tree
			gEditor.GetLogicalNodeCreator()->AttachSiegeNode( engine, (*g) );

			// Tell the system about updates
			bUpdateLighting = true;
		}

		// Accumulate static
		pNode->AccumulateStaticLighting();

		if( bUpdateLighting )
		{
			// Tell the node about it verts before we recalc the lighting
			SiegeMesh& bmesh = pNode->GetMeshHandle().RequestObject( engine.MeshCache() );
			pNode->SetNumLitVertices( bmesh.GetNumberOfVertices() );

			// Update all lighting for this node
			engine.LightDatabase().UpdateLighting( (*g) );			

			// Accumulate static
			pNode->AccumulateStaticLighting();
		}
		
		gpgenericf(( "Region_IO::LoadRegionSiegeNodes - built logical nodes for node guid %s\n", (*g).ToString().c_str() ));
	}

	const BadNodeMap& badNodeMap	= gEditor.GetLogicalNodeCreator()->GetBadNodeMap();
	for( BadNodeMap::const_iterator b = badNodeMap.begin(); b != badNodeMap.end(); ++b )
	{
		// get a handle to the SiegeNode
		SiegeNodeHandle hNode( engine.NodeCache().UseObject( (*b).first ));
		SiegeNode& Node = hNode.RequestObject( engine.NodeCache() );

		for( SiegeNodeDoorConstIter d = Node.GetDoors().begin(); d != Node.GetDoors().end(); ++d )
		{
			if( (*d)->GetNeighbour() == UNDEFINED_GUID )
			{
				continue;
			}

			if( (*b).second.find( (*d)->GetNeighbour() ) != (*b).second.end() )
			{
				(*d)->SetAsInaccurate( true );
			}
		}
	}

	// Close the file
	if ( hFile != NULL )
	{
		::CloseHandle( (HANDLE)hFile );		
	}

	return( bSuccess );
}

// Fill in the passed map with parsed offset table data from the LNC file
void EditorRegion::BuildFileOffsetTable( HANDLE hFile, OffsetTableMap* pMap )
{
	// Read the first dword of the file to get the size of the map
	unsigned int sizeOfMap = 0;

	unsigned long temp_bytes_read = 0;
	::ReadFile( (HANDLE)hFile, &sizeOfMap, sizeof( unsigned int ), &temp_bytes_read, 0 );

	// Go through it
	for( unsigned int i = 0; i < sizeOfMap; ++i )
	{
		// Read the guid
		unsigned int guid_uint = 0;		
		::ReadFile( (HANDLE)hFile, &guid_uint, sizeof( unsigned int ), &temp_bytes_read, 0 );

		// Read in the offset
		unsigned int offset = 0;		
		::ReadFile( (HANDLE)hFile, &offset, sizeof( unsigned int ), &temp_bytes_read, 0 );

		// Seek past the size
		LARGE_INTEGER li;
		li.QuadPart = sizeof( unsigned int );
		li.LowPart = ::SetFilePointer( (HANDLE)hFile, li.LowPart, &li.HighPart, FILE_CURRENT );
		if( li.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR ) 
		{
			li.QuadPart = -1;
		}		

		// Put the new pair of stuff into the map
		pMap->insert( std::make_pair( guid_uint, offset ) );
	}
}


// Backup GAS Files
void EditorRegion::BackupGASFiles()
{
	StringVec files;
	gpstring new_path = GetRegionHandle()->GetDB()->GetHomeWritePath();
	new_path.append( GetRegionHandle()->GetOriginPath() );

		
	stringtool::RemoveTrailingBackslash( new_path );
	GetFileList( files,(char *)new_path.c_str() );
	StringVec::iterator i;
	for ( i = files.begin(); i != files.end(); ++i ) {
		gpstring sFile = (*i);
		unsigned int nPos = sFile.find( "." );
		if (( nPos != -1 ) && ( sFile.size() >= (nPos+3) )) {			
			gpstring sExt = sFile.substr( nPos+1, nPos+3 );
			if (( sExt.same_no_case( "gas" ) ) || ( sExt.same_no_case( "GAS" ) )) {
				gpstring sNewFile = sFile.substr( 0, nPos );
				char szNum[1024] = "";
				int backup_num = 0;
				sprintf( szNum, "%s.%d", sNewFile.c_str(), backup_num );
				
				gpassert( stringtool::IsAbsolutePath( sNewFile ) );
				while ( CopyFile( sFile.c_str(), szNum, TRUE ) == FALSE )
				{
					backup_num++;	
					sprintf( szNum, "%s.%d", sNewFile.c_str(), backup_num );
				}
			}
		}
	}
}


void EditorRegion::GetFileList( StringVec & files, const char *szCurrentDirectory )
{
	WIN32_FIND_DATA win32FindData;
	HANDLE			hFile;
	if ( !SetCurrentDirectory( szCurrentDirectory ) )
	{
		return;
	}

	// Get the first file
	hFile = FindFirstFile( "*.*", &win32FindData );
	if ( win32FindData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY ) {
		files.push_back( win32FindData.cFileName );
	}

	// Parse through the remaining files
	gpstring temp;
	while ( FindNextFile( hFile, &win32FindData ) ) {
		temp = szCurrentDirectory;	
		temp += "\\";
		temp += win32FindData.cFileName;
		if ( win32FindData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY ) {
			files.push_back( temp );
		}
		else if ( strcmp( win32FindData.cFileName, ".." ) ) {
			SetCurrentDirectory( temp.c_str() );
			GetFileList( files, temp.c_str() );			 
			SetCurrentDirectory( szCurrentDirectory );
		}
	}
}


void EditorRegion::SetRegionName( gpstring sNewName )
{
	gpstring sPath = GetRegionHandle()->GetOriginPath();
	GetRegionHandle()->GetParentBlock()->CreateChildDirBlock( sNewName );
	RenameRegion( GetRegionHandle(), GetRegionName(), sNewName );
	
	m_sRegionName = sNewName;
	FuelHandle hRegion = GetRegionHandle()->GetChildBlock( "region" );
	hRegion->Set( "name", sNewName );	
	GetRegionHandle()->GetDB()->SaveChanges();			

	gpstring sOldFile = GetRegionHandle()->GetDB()->GetHomeWritePath();
	gpstring sNewFile = GetRegionHandle()->GetDB()->GetHomeWritePath();

	sOldFile += sPath;
	sOldFile += "terrain_nodes\\siege_nodes.lnc";

	sNewFile += GetRegionHandle()->GetOriginPath();
	sNewFile += "terrain_nodes\\siege_nodes.lnc";

	gpassert( stringtool::IsAbsolutePath( sOldFile ) );
	gpassert( stringtool::IsAbsolutePath( sNewFile ) );
	CopyFile( sOldFile, sNewFile, false );
}


void EditorRegion::RenameRegion( FuelHandle hBlock, gpstring sRegionName, gpstring sNewName )
{
	FuelHandleList hChildren = hBlock->ListChildBlocks( 1 );
	FuelHandleList::iterator i;
	for ( i = hChildren.begin(); i != hChildren.end(); ++i ) {
		RenameRegion( *i, sRegionName, sNewName );
	}	

	gpstring sAddress = hBlock->GetAddress();
	gpstring sName = sRegionName;
	gpstring sNew = sNewName;
	int index = sAddress.find( sName );
	if ( index != -1 ) {
		sAddress.replace( index, sName.size(), "" );		
		sAddress.insert( index, sNew );
	}
//	hBlock->SetAddress( sAddress.c_str() );	


	gpstring sOrigin = hBlock->GetOriginPath();
	int index2 = sOrigin.find( sName );
	if ( index2 != -1 ) {
		sOrigin.replace( index2, sName.size(), "" );		
		sOrigin.insert( index2, sNew );
	}
//	hBlock->SetOrigin( sOrigin.c_str() );

	SetRegionDirty();
}


gpstring EditorRegion::GetRegionDescription()
{
	gpstring sDescription;
	FuelHandle hMain = m_fhRegion->GetChildBlock( "region" );
	if ( hMain.IsValid() ) 
	{
		hMain->Get( "description", sDescription );
	}
	return sDescription;
}


void EditorRegion::SetRegionDescription( gpstring sDescription )
{
	FuelHandle hMain = m_fhRegion->GetChildBlock( "region" );
	if ( hMain.IsValid() ) 
	{
		hMain->Set( "description", sDescription, FVP_QUOTED_STRING );
	}

	SetRegionDirty();
}


gpstring EditorRegion::GetRegionNotes()
{
	gpstring sNotes;
	FuelHandle hMain = m_fhRegion->GetChildBlock( "region" );
	if ( hMain.IsValid() ) 
	{
		hMain->Get( "notes", sNotes );
	}
	return sNotes;
}


void EditorRegion::SetRegionNotes( gpstring sNotes )
{
	FuelHandle hMain = m_fhRegion->GetChildBlock( "region" );
	if ( hMain.IsValid() ) 
	{
		hMain->Set( "notes", sNotes, FVP_QUOTED_STRING );
	}

	SetRegionDirty();
}

gpstring GetUserInformation()
{
	gpstring str;
	str.assignf( "%s,%s", SysInfo::GetComputerName(), SysInfo::GetUserName() );

	return ( str );
}