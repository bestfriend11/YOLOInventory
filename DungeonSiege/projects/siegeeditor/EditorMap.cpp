//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorMap.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "PrecompEditor.h"
#include "EditorMap.h"
#include "EditorRegion.h"
#include "world.h"
#include "worldterrain.h"
#include "LogicalNodeCreator.h"
#include "GoSupport.h"
#include "BottomView.h"
#include "FuelDb.h"
#include "LeftView.h"


EditorMap::EditorMap()
	: m_selectedStartID( -1 )
	, m_bHasNodeMeshMap( false )
{
	
}


void EditorMap::CreateNewMap( MapCreateInfo & mapCreateInfo )							 
{
	FuelHandle fhMap( "world:maps" );
	fhMap = fhMap->CreateChildDirBlock( mapCreateInfo.sMapName );

	FuelHandle fhInfo = fhMap->CreateChildBlock( "map", "main.gas" );
	fhInfo->SetType( "map" );
	fhInfo->SetName( "map" );	
	fhInfo->Set( "screen_name", mapCreateInfo.sScreenName, FVP_QUOTED_STRING );
	fhInfo->Set( "dev_only", mapCreateInfo.bDevOnly );	
	fhInfo->Set( "description", mapCreateInfo.sDescription, FVP_QUOTED_STRING );
	
	// Set the time of day
	gpstring sTime;
	sTime.assignf( "%dh%dm", mapCreateInfo.startTimeHour, mapCreateInfo.startTimeMinute );
	fhInfo->Set( "timeofday", sTime.c_str() );

	fhInfo->Set( "world_frustum_radius", mapCreateInfo.worldFrustumRadius );
	fhInfo->Set( "world_interest_radius", mapCreateInfo.worldInterestRadius );

	fhMap->CreateChildDirBlock( "regions" );
	fhMap->CreateChildDirBlock( "info" );
	
	FuelHandle hCamera	= fhInfo->GetChildBlock( "camera", true );	
	if ( hCamera.IsValid() ) 
	{		
		// Set To Defaults
		gpstring sPosition;
		sPosition.assignf( "%f,%f,%f,0x%08X", 0.0f, 0.0f, 0.0f, 0 );
		hCamera->Set( "position", sPosition );
		hCamera->Set( "distance", 13.0f );
		hCamera->Set( "azimuth", 70.0f );
		hCamera->Set( "orbit", 0.0f );
		hCamera->Set( "fov", 60.0f );
		hCamera->Set( "min_distance", 3.0f );
		hCamera->Set( "max_distance", 18.0f );		
		hCamera->Set( "min_azimuth", 32.0f );
		hCamera->Set( "max_azimuth", 85.0f );
		hCamera->Set( "nearclip", 0.10f );
		hCamera->Set( "farclip", 70.0f );				
	}	

	// Create the node mesh index
	FuelHandle hIndex = fhMap->GetChildBlock( "index" );
	if ( !hIndex )
	{
		hIndex = fhMap->CreateChildDirBlock( "index" );
	}
	
	SetHasNodeMeshIndex( true );
	fhInfo->Set( "use_node_mesh_index", true );

	fhMap->GetDB()->SaveChanges();
}


void EditorMap::SetWorldFrustumRadius( float radius )
{
	FuelHandle hMapMain = gEditorRegion.GetMapHandle()->GetChildBlock( "map" );
	if ( hMapMain )
	{
		hMapMain->Set( "world_frustum_radius", radius );
	}

	gEditorRegion.SetRegionDirty();
}


float EditorMap::GetWorldFrustumRadius()
{
	float radius = 0;
	FuelHandle hMapMain = gEditorRegion.GetMapHandle()->GetChildBlock( "map" );
	if ( hMapMain )
	{
		hMapMain->Get( "world_frustum_radius", radius );		
	}

	return radius;
}


void EditorMap::SetWorldInterestRadius( float radius )
{
	FuelHandle hMapMain = gEditorRegion.GetMapHandle()->GetChildBlock( "map" );
	if ( hMapMain )
	{
		hMapMain->Set( "world_interest_radius", radius );
	}

	gEditorRegion.SetRegionDirty();
}


float EditorMap::GetWorldInterestRadius()
{
	float radius = 0;
	FuelHandle hMapMain = gEditorRegion.GetMapHandle()->GetChildBlock( "map" );
	if ( hMapMain )
	{
		hMapMain->Get( "world_interest_radius", radius );		
	}

	return radius;
}


void EditorMap::GetCameraSettings(	 float & y, float & z, float & x, int & nodeId,
									 float & azimuth, float & orbit, 
									 float & distance, float & minDistance, float & maxDistance,
									 float & nearclip, float & farclip, float & fov,
									 float & minAzimuth, float & maxAzimuth )
{
	FuelHandle hMapMain = gEditorRegion.GetMapHandle()->GetChildBlock( "map" );
	FuelHandle hCamera	= hMapMain->GetChildBlock( "camera" );
	if ( hMapMain.IsValid() ) {
		
		if ( hCamera.IsValid() ) 
		{			
			gpstring sPosition;
			if ( hCamera->Get( "position", sPosition ) )
			{
				stringtool::GetDelimitedValue( sPosition, ',', 0, x );
				stringtool::GetDelimitedValue( sPosition, ',', 1, y );
				stringtool::GetDelimitedValue( sPosition, ',', 2, z );
				stringtool::GetDelimitedValue( sPosition, ',', 3, nodeId );
			}			

			hCamera->Get( "distance", distance );
			hCamera->Get( "azimuth", azimuth );
			hCamera->Get( "orbit", orbit );
			hCamera->Get( "fov", fov );
			hCamera->Get( "min_distance", minDistance );
			hCamera->Get( "max_distance", maxDistance );		
			hCamera->Get( "min_azimuth", minAzimuth );
			hCamera->Get( "max_azimuth", maxAzimuth );
			hCamera->Get( "nearclip", nearclip );
			hCamera->Get( "farclip", farclip );		
		}
	}
}
			

void EditorMap::SetCameraSettings(   float & y, float & z, float & x, int & nodeId,
									 float & azimuth, float & orbit, 
									 float & distance, float & minDistance, float & maxDistance,
									 float & nearclip, float & farclip, float & fov,
									 float & minAzimuth, float & maxAzimuth )
{
	FuelHandle hMapMain = gEditorRegion.GetMapHandle()->GetChildBlock( "map" );
	FuelHandle hCamera	= hMapMain->GetChildBlock( "camera", true );
	if ( hMapMain.IsValid() ) {			
		if ( hCamera.IsValid() ) {
			
			gpstring sPosition;
			sPosition.assignf( "%f,%f,%f,0x%08X", x, y, z, nodeId );
			hCamera->Set( "position", sPosition );
			hCamera->Set( "distance", distance );
			hCamera->Set( "azimuth", azimuth );
			hCamera->Set( "orbit", orbit );
			hCamera->Set( "fov", fov );
			hCamera->Set( "min_distance", minDistance );
			hCamera->Set( "max_distance", maxDistance );		
			hCamera->Set( "min_azimuth", minAzimuth );
			hCamera->Set( "max_azimuth", maxAzimuth );
			hCamera->Set( "nearclip", nearclip );
			hCamera->Set( "farclip", farclip );		
		}		
	}

	gEditorRegion.SetRegionDirty();
}


void EditorMap::GetBookmarks( StringVec & bookmarks )
{
	FuelHandle hInfo = gEditorRegion.GetMapHandle()->GetChildBlock( "info" );
	FuelHandle hBookmarks = hInfo->GetChildBlock( "bookmarks" );

	if ( hBookmarks.IsValid() == false ) 
	{
		hBookmarks = hInfo->CreateChildBlock( "bookmarks", "bookmarks.gas" );
	}
	FuelHandleList hlBookmarks = hBookmarks->ListChildBlocks( 1, "bookmark" );
	FuelHandleList::iterator i;
	for ( i = hlBookmarks.begin(); i != hlBookmarks.end(); ++i ) 
	{
		bookmarks.push_back( (*i)->GetName() );
	}
}
	

void EditorMap::GetBookmarkParameters( gpstring sBookmark, SiegePos & spos, float & azimuth, float & orbit, float & distance, gpstring & sDescription )
{
	FuelHandle hInfo = gEditorRegion.GetMapHandle()->GetChildBlock( "info" );
	FuelHandle hBookmarks = hInfo->GetChildBlock( "bookmarks", true );	
	FuelHandleList hlBookmarks = hBookmarks->ListChildBlocks( 1, "bookmark" );
	FuelHandleList::iterator i;
	for ( i = hlBookmarks.begin(); i != hlBookmarks.end(); ++i ) 
	{
		if ( sBookmark.same_no_case( (*i)->GetName() ) ) 
		{
			
			gpstring sPosition;
			if ( (*i)->Get( "position", sPosition ) )
			{
				::FromString( sPosition.c_str(), spos );
			}

			(*i)->Get( "description", sDescription );
			
			FuelHandle hCamera = (*i)->GetChildBlock( "camera" );
			if ( hCamera.IsValid() == false ) {
				continue;
			}	

			hCamera->Get( "azimuth", azimuth );
			hCamera->Get( "orbit", orbit );
			hCamera->Get( "distance", distance );

			return;
		}
	}
}


void EditorMap::SetBookmarkParameters( gpstring sOldName, gpstring sBookmark, SiegePos spos, float azimuth, float orbit, float distance, gpstring sDescription )
{
	FuelHandle hInfo = gEditorRegion.GetMapHandle()->GetChildBlock( "info" );
	FuelHandle hBookmarks = hInfo->GetChildBlock( "bookmarks", true );	
	FuelHandleList hlBookmarks = hBookmarks->ListChildBlocks( 1, "bookmark" );
	FuelHandleList::iterator i;
	for ( i = hlBookmarks.begin(); i != hlBookmarks.end(); ++i ) {
		if ( sOldName.same_no_case( (*i)->GetName() ) ) {
						
			gpstring sPosition;			
			sPosition.assignf( "%f,%f,%f,0x%08X", spos.pos.x, spos.pos.y, spos.pos.z, spos.node.GetValue() );
			(*i)->Set( "position", sPosition );

			(*i)->Set( "description", sDescription, FVP_QUOTED_STRING );
			
			FuelHandle hCamera = (*i)->GetChildBlock( "camera", true );
			if ( hCamera.IsValid() == false ) {
				continue;
			}

			hCamera->Set( "azimuth", azimuth );
			hCamera->Set( "orbit", orbit );
			hCamera->Set( "distance", distance );

			(*i)->SetName( sBookmark );

			gEditorRegion.SetRegionDirty();

			return;
		}
	}
}


bool EditorMap::AddBookmark( gpstring sBookmark, SiegePos spos, float azimuth, float orbit, float distance )
{
	FuelHandle hInfo = gEditorRegion.GetMapHandle()->GetChildBlock( "info" );
	FuelHandle hBookmarks = hInfo->GetChildBlock( "bookmarks", true );
	FuelHandleList hlBookmarks = hBookmarks->ListChildBlocks( 1, "bookmark" );
	FuelHandleList::iterator i;
	sBookmark.to_lower();
	for ( i = hlBookmarks.begin(); i != hlBookmarks.end(); ++i ) {
		if ( sBookmark.same_no_case( (*i)->GetName() ) ) {
			return false;
		}
	}

	FuelHandle hNew = hBookmarks->CreateChildBlock( sBookmark );
	hNew->SetType( "bookmark" );
	
	gpstring sPosition;
	sPosition.assignf( "%f,%f,%f,0x%08X", spos.pos.x, spos.pos.y, spos.pos.z, spos.node );
	hNew->Set( "position", sPosition );

	gpstring empty;
	hNew->Set( "description", empty );

	FuelHandle hCamera = hNew->GetChildBlock( "camera", true );
	if ( hCamera.IsValid() == false ) 
	{
		return false;
	}

	hCamera->Set( "azimuth", azimuth );
	hCamera->Set( "orbit", orbit );
	hCamera->Set( "distance", distance );

	gEditorRegion.SetRegionDirty();

	return true;
}


void EditorMap::RemoveBookmark( gpstring sBookmark )
{
	FuelHandle hInfo = gEditorRegion.GetMapHandle()->GetChildBlock( "info" );
	FuelHandle hBookmarks = hInfo->GetChildBlock( "bookmarks", true );
	FuelHandleList hlBookmarks = hBookmarks->ListChildBlocks( 1, "bookmark" );
	FuelHandleList::iterator i;
	for ( i = hlBookmarks.begin(); i != hlBookmarks.end(); ++i ) 
	{
		if ( sBookmark.same_no_case( (*i)->GetName() ) ) 
		{
			hBookmarks->DestroyChildBlock( *i );
			gEditorRegion.SetRegionDirty();
			return;
		}
	}
}


void EditorMap::GenerateLNCs( MapToRegionMap & regionMap, bool bFullSave, bool bDeleteLNC, bool bLoadLights, bool bLoadDecals, bool bLoadLogical, bool bFullRecalc, bool bRemapMeshRange, bool bConvertGlobalNodes )
{
	MapToRegionMap::iterator i;
	for ( i = regionMap.begin(); i != regionMap.end(); ++i ) 
	{	
		gpstring sMap		= (*i).first;
		gpstring sRegion	= (*i).second; 

		if ( bDeleteLNC ) 
		{
			FuelHandle hRoot( "root" );
			gpstring sFilename = hRoot->GetDB()->GetHomeWritePath();
			sFilename.appendf( "world\\maps\\%s\\regions\\%s\\terrain_nodes\\siege_nodes.lnc", sMap.c_str(), sRegion.c_str() );
			CFileFind finder;
			BOOL bFound = finder.FindFile( sFilename.c_str() );
			if ( bFound ) 
			{
				DeleteFile( sFilename.c_str() );
			}
		}
				
		if ( !bFullSave ) 
		{
			gEditorRegion.LoadRegion( sRegion, sMap, true, bLoadLights, bLoadDecals, false, bLoadLogical, bFullRecalc );
			gEditorRegion.SaveLNC();				
		}
		else 
		{			
			if ( bRemapMeshRange )
			{					
				ReportSys::EnableContext( &gErrorBoxContext, false ); 
				ReportSys::EnableContext( &gPerfContext, false ); 
				ReportSys::EnableContext( &gMessageBoxContext, false ); 				
				ReportSys::EnableContext( &gErrorContext, false ); 
				
				gpstring sAddress;
				sAddress.assignf( "world:maps:%s:regions:%s:region", sMap.c_str(), sRegion.c_str() );				
				FuelHandle hRegionMain( sAddress );
				if ( hRegionMain )
				{
					hRegionMain->Set( "mesh_range", gEditorRegion.GetDefaultMeshRange( sMap, sRegion ) );
				}
				hRegionMain->GetDB()->SaveChanges();
			}

			if ( bConvertGlobalNodes )
			{
				typedef std::map< int, gpstring > MeshToSnoMap;
				MeshToSnoMap globalNodes;

				// load the global nodes
				{
					FastFuelHandle siegeNodes( "world:global:siege_nodes" );
					FastFuelHandleColl siegeNodeList;

					if ( siegeNodes && siegeNodes.ListChildrenNamed( siegeNodeList, "mesh_file*", -1 ) )
					{
						int meshId;
						gpstring filename;

						FastFuelHandleColl::iterator i, begin = siegeNodeList.begin(), end = siegeNodeList.end();
						for ( i = begin ; i != end ; ++i )
						{
							if ( (*i).Get( "guid", meshId, false )
								&& ((*i).Get( "filename", filename ) || (*i).Get( "genericname", filename, false )) )
							{
								globalNodes.insert( std::make_pair( meshId, filename ) );
							}
						}
					}	
				}
				
				{
					// set the mesh range
					gpstring sAddress;
					sAddress.assignf( "world:maps:%s:regions:%s:region", sMap.c_str(), sRegion.c_str() );				
					FuelHandle hRegionMain( sAddress );
					sAddress.assignf( "world:maps:%s:map", sMap.c_str() );
					FuelHandle hMapMain( sAddress );
					if ( hRegionMain && hMapMain )
					{
						hRegionMain->Set( "mesh_range", gEditorRegion.GetDefaultMeshRange( sMap, sRegion ) );						
						hMapMain->Set( "use_node_mesh_index", true );
					}

					// create the mesh index
					FuelHandle hMeshIndex = hRegionMain->GetParentBlock()->GetChildBlock( "index" );
					hMeshIndex = hMeshIndex->CreateChildBlock( "node_mesh_index", "node_mesh_index.gas" );
					
					FuelHandle hNodes = hRegionMain->GetParentBlock()->GetChildBlock( "terrain_nodes:siege_node_list" );
					if ( hNodes )
					{
						FuelHandleList hlNodes = hNodes->ListChildBlocks( 1, "snode" );
						FuelHandleList::iterator i;
						for ( i = hlNodes.begin(); i != hlNodes.end(); ++i )
						{
							int meshId = 0;
							(*i)->Get( "mesh_guid", meshId );

							MeshToSnoMap::iterator found = globalNodes.find( meshId );
							if ( found != globalNodes.end() )
							{								
								hMeshIndex->Set( gpstringf( "0x%08x", meshId ), (*found).second );
							}
						}
					}

					hRegionMain->GetDB()->SaveChanges();
				}
			}
			
			gEditorRegion.LoadRegion( sRegion, sMap, false, bLoadLights, bLoadDecals, false, bLoadLogical, bFullRecalc );
			gGoDb.CommitAllRequests();
			gEditorRegion.SaveRegion( true, true, true, true, true );

			ReportSys::EnableContext( &gErrorBoxContext, true ); 
			ReportSys::EnableContext( &gPerfContext, true ); 
			ReportSys::EnableContext( &gMessageBoxContext, true ); 				
			ReportSys::EnableContext( &gErrorContext, true ); 			
		}
	}	
}


void EditorMap::GetStartPositions( StringVec & positions )
{	
	FuelHandle hInfo = gEditorRegion.GetMapHandle()->GetChildBlock( "info" );
	FuelHandle hPositions = hInfo->GetChildBlock( "start_positions" );
	if ( hPositions.IsValid() == false ) 
	{
		hPositions = hInfo->CreateChildBlock( "start_positions", "start_positions.gas" );
	}
	FuelHandleList hlPositions = hPositions->ListChildBlocks( 1, NULL, "start_position" );
	FuelHandleList::iterator i;
	for ( i = hlPositions.begin(); i != hlPositions.end(); ++i ) 
	{
		int fuel_id = -1;
		(*i)->Get( "id", fuel_id );
		gpstring sID;
		sID.assignf( "%d", fuel_id );
		positions.push_back( sID );
	}
}


void EditorMap::GetPositionParameters( int id, SiegePos & spos, SiegePos & cameraPos, float & azimuth, float & orbit, float & distance )
{
	FuelHandle hInfo = gEditorRegion.GetMapHandle()->GetChildBlock( "info" );	
	FuelHandle hPosition = hInfo->GetChildBlock( "start_positions", true );
	FuelHandleList hlPositions = hPosition->ListChildBlocks( 1, NULL, "start_position" );
	FuelHandleList::iterator i;
	for ( i = hlPositions.begin(); i != hlPositions.end(); ++i ) 
	{
		int fuel_id = 0;
		(*i)->Get( "id", fuel_id );
		if ( id == fuel_id ) 
		{			
			gpstring sPosition;
			(*i)->Get( "position", sPosition );
			::FromString( sPosition.c_str(), spos );
		
			gpstring sCamera;
			vector_3 camera;
			FuelHandle hCamera = (*i)->GetChildBlock( "camera" );
			if ( hCamera.IsValid() )
			{
				hCamera->Get( "azimuth", azimuth );
				hCamera->Get( "orbit", orbit );
				hCamera->Get( "distance", distance );
				gpstring sCamera;
				(*i)->Get( "position", sCamera );
				::FromString( sCamera.c_str(), cameraPos );
			}
			
			return;
		}
	}
}


void EditorMap::SetPositionParameters( int id, SiegePos spos, SiegePos cameraPos, float azimuth, float orbit, float distance )
{
	FuelHandle hInfo = gEditorRegion.GetMapHandle()->GetChildBlock( "info" );
	FuelHandle hPositions = hInfo->GetChildBlock( "start_positions", true );
	FuelHandleList hlPositions = hPositions->ListChildBlocks( 1, NULL, "start_position" );
	FuelHandleList::iterator i;

	if ( id <= 0 ) 
	{
		return;
	}

	if ( id != m_selectedStartID ) 
	{
		for ( i = hlPositions.begin(); i != hlPositions.end(); ++i ) 
		{
			int fuel_id = 0;
			(*i)->Get( "id", fuel_id );
			if ( id == fuel_id ) 
			{
				MessageBox( gEditor.GetMainHWnd(), "A start position with this ID already exists, please select another ID or delete the existing conflicting ID", "Error", MB_OK );
				return;
			}
		}
	}

	for ( i = hlPositions.begin(); i != hlPositions.end(); ++i )
	{		
		int fuel_id = 0;
		(*i)->Get( "id", fuel_id );
		if ( m_selectedStartID == fuel_id ) 
		{						
			gpstring sPosition;
			sPosition.assignf( "%f,%f,%f,0x%08X", spos.pos.x, spos.pos.y, spos.pos.z, spos.node.GetValue() );

			(*i)->Set( "position", sPosition );
		
			FuelHandle hCamera = (*i)->GetChildBlock( "camera" );		
			if ( hCamera.IsValid() == false )
			{
				hCamera = (*i)->CreateChildBlock( "camera" );
			}

			hCamera->Set( "azimuth", azimuth );
			hCamera->Set( "orbit", orbit );
			hCamera->Set( "distance", distance );

			gpstring sCameraPos;
			
			sCameraPos.assignf( "%f,%f,%f,0x%08X", cameraPos.pos.x, cameraPos.pos.y, cameraPos.pos.z, cameraPos.node.GetValue() );

			hCamera->Set( "position", sCameraPos );
			
			gpstring sId;
			stringtool::Set( id, sId );
			(*i)->Set( "id", sId );

			gEditorRegion.SetRegionDirty();

			return;	
		}			
	}
}


bool EditorMap::AddStartPosition( int id, SiegePos spos, SiegePos cameraPos, float azimuth, float orbit, float distance )
{
	FuelHandle hInfo = gEditorRegion.GetMapHandle()->GetChildBlock( "info" );
	FuelHandle hPositions = hInfo->GetChildBlock( "start_positions" );
	if ( hPositions.IsValid() == false ) 
	{
		hPositions = hInfo->CreateChildBlock( "start_positions", "start_positions.gas" );
	}
	FuelHandleList hlPositions = hPositions->ListChildBlocks( 1, NULL, "start_position" );
	FuelHandleList::iterator i;

	for ( i = hlPositions.begin(); i != hlPositions.end(); ++i ) 
	{
		int fuel_id = -1;
		(*i)->Get( "id", fuel_id );
		if ( fuel_id == id ) {		
			return false;
		}
	}

	FuelHandle hNew = hPositions->CreateChildBlock( "start_position" );	
	
	gpstring sPosition;
	sPosition.assignf( "%f,%f,%f,0x%08X", spos.pos.x, spos.pos.y, spos.pos.z, spos.node.GetValue() );
	hNew->Set( "position", sPosition );

	FuelHandle hCamera = hNew->GetChildBlock( "camera" );		
	if ( hCamera.IsValid() == false )
	{
		hCamera = hNew->CreateChildBlock( "camera" );
	}

	hCamera->Set( "azimuth", azimuth );
	hCamera->Set( "orbit", orbit );
	hCamera->Set( "distance", distance );

	gpstring sCameraPos;
	sCameraPos.assignf( "%f,%f,%f,0x%08X", cameraPos.pos.x, cameraPos.pos.y, cameraPos.pos.z, cameraPos.node.GetValue() );
	hCamera->Set( "position", sCameraPos );

	gpstring sId;
	stringtool::Set( id, sId );
	hNew->Set( "id", sId );

	gEditorRegion.SetRegionDirty();

	return true;
}


void EditorMap::RemoveStartPosition( int id )
{
	FuelHandle hInfo = gEditorRegion.GetMapHandle()->GetChildBlock( "info" );
	FuelHandle hPositions = hInfo->GetChildBlock( "start_positions", true );
	FuelHandleList hlPositions = hPositions->ListChildBlocks( 1, NULL, "start_position" );
	FuelHandleList::iterator i;
	for ( i = hlPositions.begin(); i != hlPositions.end(); ++i ) 
	{
		int fuel_id = -1;
		(*i)->Get( "id", fuel_id );
		if ( id == fuel_id ) 
		{
			hPositions->DestroyChildBlock( *i );
			gEditorRegion.SetRegionDirty();
			return;
		}
	}
}


void EditorMap::SetStartTime( int hour, int minute )
{
	FuelHandle hMap = gEditorRegion.GetMapHandle()->GetChildBlock( "map" );
	if ( hMap.IsValid() ) 
	{
		gpstring sTimeOfDay;
		sTimeOfDay.assignf( "%dh%dm", hour, minute );
		hMap->Set( "timeofday", sTimeOfDay );				
	}	

	gEditorRegion.SetRegionDirty();
}


void EditorMap::GetStartTime( int & hour, int & minute )
{
	FuelHandle hMap = gEditorRegion.GetMapHandle()->GetChildBlock( "map" );
	if ( hMap.IsValid() ) 
	{
		gpstring sTimeOfDay;
		hMap->Get( "timeofday", sTimeOfDay );
		if ( sTimeOfDay.empty() )
		{
			return;
		}
		gpstring sHour		= sTimeOfDay.substr( 0, 2 );
		gpstring sMinute	= sTimeOfDay.substr( 3, 2 );
		stringtool::Get( sHour, hour );
		stringtool::Get( sMinute, minute );		
	}
}


void EditorMap::ValidateBookmarks( gpstring sMap )
{
	StringVec errors;
	std::vector< int > nodeIds;
	std::vector< int >::iterator i;
	BuildMapNodeList( sMap, nodeIds );

	gpstring sMapPath;
	sMapPath.assignf( "world:maps:%s", sMap.c_str() );
	FuelHandle hMap( sMapPath );

	FuelHandle hInfo = hMap->GetChildBlock( "info" );
	FuelHandle hBookmarks = hInfo->GetChildBlock( "bookmarks", true );
	FuelHandleList hlBookmarks = hBookmarks->ListChildBlocks( 1, "bookmark" );
	FuelHandleList::iterator j;
	for ( j = hlBookmarks.begin(); j != hlBookmarks.end(); ++j ) 
	{
		SiegePos spos;
		gpstring sPosition;
		(*j)->Get( "position", sPosition );
		::FromString( sPosition.c_str(), spos );				

		bool bFound = false;
		for ( i = nodeIds.begin(); i != nodeIds.end(); ++i )
		{
			if ( (int)spos.node.GetValue() == *i )
			{
				bFound = true;
			}
		}

		if ( !bFound )
		{
			gpstring sError;
			sError.assignf( "Map: %s -- Node \"0x%08X\" for Bookmark \"%s\" does not exist.", sMap.c_str(), spos.node.GetValue(), (*j)->GetName() );
			errors.push_back( sError );
		}
	}

	CString rText;
	gBottomView.GetConsoleEditCtrl().GetWindowText( rText );
	rText.Insert( rText.GetLength(), "\r\n\r\nInvalid Bookmarks:" );
	rText.Insert( rText.GetLength(), "\r\n------------------" );
	
	StringVec::iterator k;
	for ( k = errors.begin(); k != errors.end(); ++k )
	{
		rText.Insert( rText.GetLength(), "\r\n" );
		rText.Insert( rText.GetLength(), (*k).c_str() );
		rText.Insert( rText.GetLength(), "\r\n" );
	}
	rText.Insert( rText.GetLength(), "\r\n" );

	gBottomView.GetConsoleEditCtrl().SetWindowText( rText );
}


void EditorMap::BuildMapNodeList( gpstring sMap, std::vector< int > nodeIds )
{
	if ( gEditorRegion.GetMapHandle().IsValid() == false ) 
	{
		return;
	}

	FuelHandleList hlNodeIndex = gEditorRegion.GetMapHandle()->ListChildBlocks( 4, "streamer_node_index" );
	FuelHandleList::iterator i;
	for ( i = hlNodeIndex.begin(); i != hlNodeIndex.end(); ++i )
	{		
		if ( (*i)->FindFirstKeyAndValue() )
		{
			gpstring sKey;
			gpstring sValue;
			(*i)->GetNextKeyAndValue( sKey, sValue );

			int nodeID = 0;
			stringtool::Get( sKey, nodeID );
			nodeIds.push_back( nodeID );
		}
	}
}

	
void EditorMap::CreateNorthVector( gpstring sMapAddress )
{
	FuelHandle hMap( sMapAddress );
	if ( hMap )
	{
		FuelHandle hHotpoint = hMap->GetChildBlock( "info:hotpoints" );
		if ( !hHotpoint )
		{
			hHotpoint = hMap->GetChildBlock( "info" );
			if ( !hHotpoint )
			{
				hHotpoint = hMap->CreateChildDirBlock( "info" );
			}

			hHotpoint = hHotpoint->CreateChildBlock( "hotpoints", "hotpoints.gas" );
		}			
		
		if ( hHotpoint )
		{
			FuelHandle hNorth = hHotpoint->GetChildBlock( "north" );
			if ( !hNorth )
			{
				hNorth = hHotpoint->CreateChildBlock( "north" );
				hNorth->SetType( "hotpoint" );
				hNorth->Set( "id", 1 );
			}
		}		
	}

	gEditorRegion.SetRegionDirty();
}


void EditorMap::RemapNodeMeshes()
{
	if ( gEditorRegion.GetMapHandle() && GetHasNodeMeshIndex() )
	{
		FuelHandle hRegions = gEditorRegion.GetMapHandle()->GetChildBlock( "regions" );
		FuelHandleList hlRegions = hRegions->ListChildBlocks( 1 );
		FuelHandleList::iterator iRegion;		
		for ( iRegion = hlRegions.begin(); iRegion != hlRegions.end(); ++iRegion )
		{
			if ( gEditorRegion.GetRegionHandle()->GetAddress().same_no_case( (*iRegion)->GetAddress() ) )
			{
				gpstring sNodeMesh = (*iRegion)->GetAddress();
				sNodeMesh.appendf( ":index:node_mesh_index" );
				FuelHandle hNodeMesh( sNodeMesh );
				if ( hNodeMesh )
				{	
					FuelHandle hNodes = (*iRegion)->GetChildBlock( "terrain_nodes:siege_node_list" );
					FuelHandleList hlNodes;
					if ( hNodes )
					{
						hlNodes = hNodes->ListChildBlocks( 1 );
					}
					
					std::map< gpstring, gpstring, istring_less > stringMap;
					std::map< gpstring, gpstring, istring_less >::iterator i;
					if ( hNodeMesh->FindFirstKeyAndValue() )
					{
						gpstring sOldGuid, sFile;
						while ( hNodeMesh->GetNextKeyAndValue( sOldGuid, sFile ) )
						{	
							gpstring sSno = sFile + ".sno";
							gpstring sGuid = gComponentList.GetNodeGUID( (char *)sSno.c_str() );
							stringMap.insert( std::make_pair( sGuid, sFile ) );

							if ( !sGuid.same_no_case( sOldGuid ) )
							{
								RemapMesh( hlNodes, sOldGuid, sGuid );
							}
						}					
					}
					
					FuelHandle hIndex = hNodeMesh->GetParent();
					hIndex->DestroyChildBlock( hNodeMesh );
					FuelHandle hNewRegion = hIndex->CreateChildBlock( "node_mesh_index", "node_mesh_index.gas" );				
					for ( i = stringMap.begin(); i != stringMap.end(); ++i )
					{
						hNewRegion->Set( (*i).first, (*i).second );
					}
				}
			}
		}	
	}

	gEditorRegion.SetRegionDirty();
}


void EditorMap::RemapMesh( FuelHandleList & hlNodes, gpstring sOldGuid, gpstring sNewGuid )
{	
	FuelHandleList::iterator i;
	for ( i = hlNodes.begin(); i != hlNodes.end(); )
	{
		gpstring sMeshGuid;
		(*i)->Get( "mesh_guid", sMeshGuid );
		if ( sMeshGuid.same_no_case( sOldGuid ) )
		{
			(*i)->Set( "mesh_guid", sNewGuid );
			i = hlNodes.erase( i );
			if ( i == hlNodes.end() )
			{
				return;
			}			
		}
		else
		{
			++i;
		}
	}	
}