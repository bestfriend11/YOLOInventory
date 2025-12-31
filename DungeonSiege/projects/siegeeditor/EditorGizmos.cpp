//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorGizmos.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "PrecompEditor.h"
#include "EditorGizmos.h"
#include "GizmoManager.h"
#include "EditorCamera.h"
#include "SCIDManager.h"
#include "Dialog_Decal.h"
#include "SiegeEditorView.h"
#include "EditorTerrain.h"
#include "siege_decal.h"
#include "EditorRegion.h"
#include "Preferences.h"
#include "FuelDB.h"
#include "Services.h"


using namespace siege;


EditorGizmos::EditorGizmos()
{
	m_decalHoriz	= 1.0f;
	m_decalVert		= 1.0f;
	m_bCutting		= false;
}


EditorGizmos::~EditorGizmos()
{
}


DWORD EditorGizmos::AddDecal( gpstring sTexture, SiegePos spos )
{
	FuelHandle hRoot( "root" );
	
	gpstring sFilename;
	sFilename += "art\\bitmaps\\decals\\";
	sFilename += sTexture;	

	int index = gSiegeDecalDatabase.CreateDecal( spos, gSiegeEditorView.GetHitFaceNormal(), 
												 gPreferences.GetDecalHorizontal(), gPreferences.GetDecalVertical(),
												 sFilename );

	gpstring sRandom = gEditorTerrain.GenerateRandomGUID();
	siege::database_guid gizmoId( sRandom.c_str() );

	spos.pos += gSiegeEditorView.GetHitFaceNormal();
	gGizmoManager.InsertGizmo( GIZMO_DECAL, spos, gizmoId.GetValue() );
	gGizmoManager.GetGizmo( gizmoId.GetValue() )->index = index; 	

	return gizmoId.GetValue();
}


void EditorGizmos::CreateDecalFromGizmo( Gizmo & gizmo )
{
	SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( gizmo.index );
	gpstring sTexture = gSiegeEngine.Renderer().GetTexturePathname( pDecal->GetDecalTexture() );
	
	SiegePos newPos = gizmo.spos;
	
	float lod = pDecal->GetLevelOfDetail();

	gizmo.index = gSiegeDecalDatabase.CreateDecal(	 newPos, pDecal->GetDecalOrientation(), 
													 pDecal->GetHorizontalMeters(), pDecal->GetVerticalMeters(), 
													 pDecal->GetNearPlane(), pDecal->GetFarPlane(), sTexture );

	pDecal = gSiegeDecalDatabase.GetDecalPointer( gizmo.index );
	pDecal->SetLevelOfDetail( lod );

	gEditorRegion.SetRegionDirty();
}



void EditorGizmos::DeleteDecal( int index )
{
	gSiegeDecalDatabase.DestroyDecal( index );	
}


int EditorGizmos::AddStartingPosition( SiegePos spos )
{
	gpstring sDefault = GetDefaultStartingGroup();
	StartingGroups::iterator i;
	for ( i = m_startGroups.begin(); i != m_startGroups.end(); ++i )
	{
		if ( sDefault.same_no_case( (*i).sGroup ) )
		{
			break;
		}
	}
	
	StartingPos newPos;
	SiegePos camPos;
	camPos.node = spos.node;
	newPos.spos		= spos;
	newPos.camPos	= camPos;
	newPos.azimuth	= 0.5f;
	newPos.orbit	= 0;
	newPos.distance	= 20;
	newPos.id		= GetNextId();
	newPos.gizmoId	= MakeInt( gSCIDManager.GetScid() );
	
	(*i).positions.push_back( newPos );
	
	gGizmoManager.InsertGizmo( GIZMO_STARTINGPOSITION, newPos.spos, newPos.gizmoId );

	return newPos.gizmoId;
}


void EditorGizmos::DeleteStartingPosition( int id )
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		StartingPositions::iterator i;
		for ( i = (*j).positions.begin(); i != (*j).positions.end(); ++i )
		{
			if ( (*i).gizmoId == id )
			{
				gGizmoManager.RemoveGizmo( id );
				(*j).positions.erase( i );
				return;
			}
		}
	}
}


void EditorGizmos::SaveStartingPositions( FuelHandle & hMap )
{	
	FuelHandle hInfo = hMap->GetChildBlock( "info" );
	FuelHandle hPositions = hInfo->GetChildBlock( "start_positions" );
	if ( hPositions.IsValid() )
	{
		hInfo->DestroyChildBlock( hPositions );
	}	
	hPositions = hInfo->CreateChildBlock( "start_positions", "start_positions.gas" );

	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		if ( (*j).positions.size() == 0 )
		{
			continue;
		}

		FuelHandle hGroup = hPositions->CreateChildBlock( (*j).sGroup );
		hGroup->SetType( "start_group" );
		hGroup->Set( "default", (*j).bDefault );
		hGroup->Set( "description", (*j).sDescription, FVP_QUOTED_STRING );
		hGroup->Set( "dev_only", (*j).bDevOnly );
		hGroup->Set( "screen_name", (*j).sScreenName, FVP_QUOTED_STRING );
		hGroup->Set( "id", (*j).id );

		StartingPositions::iterator i;
		for ( i = (*j).positions.begin(); i != (*j).positions.end(); ++i )
		{	
			FuelHandle hPosition = hGroup->CreateChildBlock( "start_position" );
			hPosition->Set( "id", (*i).id );

			gpstring sPos;
			sPos.assignf( "%f,%f,%f,0x%08X", (*i).spos.pos.x, (*i).spos.pos.y, (*i).spos.pos.z, (*i).spos.node.GetValue() );
			hPosition->Set( "position", sPos.c_str() );

			FuelHandle hCamera = hPosition->CreateChildBlock( "camera" );
			hCamera->Set( "azimuth", (*i).azimuth );
			hCamera->Set( "distance", (*i).distance );
			hCamera->Set( "orbit", (*i).orbit );

			gpstring sCam;
			sCam.assignf( "%f,%f,%f,0x%08X", (*i).camPos.pos.x, (*i).camPos.pos.y, (*i).camPos.pos.z, (*i).camPos.node.GetValue() );
			hCamera->Set( "position", sCam.c_str() );
		}

		FuelHandle hLevels = hGroup->CreateChildBlock( "world_levels" );
		if ( hLevels )
		{
			WorldLevels::iterator iWorld;
			for ( iWorld = (*j).levels.begin(); iWorld != (*j).levels.end(); ++iWorld )
			{
				FuelHandle hLevel = hLevels->CreateChildBlock( (*iWorld).sWorld );
				hLevel->Set( "required_level", (*iWorld).level );
			}
		}
	}
}


void EditorGizmos::LoadStartingPositions( FuelHandle & hMap )
{
	FuelHandle hInfo = hMap->GetChildBlock( "info" );
	if ( !hInfo )
	{
		gperrorf(( "VERY BAD - Map: %s does not have an 'info' directory defined.", hMap->GetName() )); 
		return;
	}

	FuelHandle hPositions = hInfo->GetChildBlock( "start_positions" );
	if ( hPositions.IsValid() == false ) 
	{
		hPositions = hMap->CreateChildBlock( "start_positions", "start_positions.gas" );
	}

	FuelHandleList hlGroups = hPositions->ListChildBlocks( 1, "start_group" );
	FuelHandleList::iterator j;

	m_startGroups.clear();

	for ( j = hlGroups.begin(); j != hlGroups.end(); ++j )
	{
		StartingGroup newGroup;
		newGroup.bDefault = false;				
		newGroup.sGroup = (*j)->GetName();
		(*j)->Get( "description", newGroup.sDescription );
		(*j)->Get( "default", newGroup.bDefault );
		(*j)->Get( "dev_only", newGroup.bDevOnly );
		(*j)->Get( "screen_name", newGroup.sScreenName );
		(*j)->Get( "id", newGroup.id );

		FuelHandleList hlPositions = (*j)->ListChildBlocks( 1, NULL, "start_position" );
		FuelHandleList::iterator i;
		for ( i = hlPositions.begin(); i != hlPositions.end(); ++i ) 
		{
			int fuel_id = -1;
			(*i)->Get( "id", fuel_id );
			
			gpstring sPos;
			gpstring sCamPos;
			float azimuth = 0;
			float orbit = 0;
			float distance = 0;

			(*i)->Get( "position", sPos );
			
			FuelHandle hCamera = (*i)->GetChildBlock( "camera" );
			if ( hCamera.IsValid() )
			{
				hCamera->Get( "azimuth", azimuth );
				hCamera->Get( "distance", distance );
				hCamera->Get( "orbit", orbit );
				hCamera->Get( "position", sCamPos );
			}

			StartingPos newPos;

			stringtool::GetDelimitedValue( sPos, ',', 0, newPos.spos.pos.x );
			stringtool::GetDelimitedValue( sPos, ',', 1, newPos.spos.pos.y );
			stringtool::GetDelimitedValue( sPos, ',', 2, newPos.spos.pos.z );
			
			int nodeId = 0;
			stringtool::GetDelimitedValue( sPos, ',', 3, nodeId );
			newPos.spos.node = siege::database_guid( nodeId );

			stringtool::GetDelimitedValue( sPos, ',', 0, newPos.camPos.pos.x );
			stringtool::GetDelimitedValue( sPos, ',', 1, newPos.camPos.pos.y );
			stringtool::GetDelimitedValue( sPos, ',', 2, newPos.camPos.pos.z );
			
			nodeId = 0;
			stringtool::GetDelimitedValue( sPos, ',', 3, nodeId );
			newPos.camPos.node = siege::database_guid( nodeId );

			newPos.azimuth	= azimuth;
			newPos.distance = distance;
			newPos.orbit	= orbit;
			newPos.id		= fuel_id;

			newPos.gizmoId	= MakeInt( gSCIDManager.GetScid() );

			newGroup.positions.push_back( newPos );

			if ( newPos.spos.node.IsValidSlow() )
			{
				gGizmoManager.InsertGizmo( GIZMO_STARTINGPOSITION, newPos.spos, newPos.gizmoId );
			}
			else
			{
				bool bFound = false;
				siege::SiegeEngine& engine = gSiegeEngine;
				FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
				FrustumNodeColl::iterator i;
				for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
				{
					SiegeNode * pNode = (*i);
					if ( pNode->GetGUID().GetValue() == nodeId )
					{
						bFound = true;
					}
				}

				if ( bFound )
				{
					gGizmoManager.InsertGizmo( GIZMO_STARTINGPOSITION, newPos.spos, newPos.gizmoId );
				}
			}
		}

		FuelHandle hLevels = (*j)->GetChildBlock( "world_levels" );
		if ( hLevels )
		{
			FuelHandleList hlLevels = hLevels->ListChildBlocks( 1 );
			FuelHandleList::iterator iLevel;
			for ( iLevel = hlLevels.begin(); iLevel != hlLevels.end(); ++iLevel )
			{
				WorldLevel level;
				(*iLevel)->Get( "required_level", level.level );
				level.sWorld = (*iLevel)->GetName();

				newGroup.levels.push_back( level );
			}
		}
		
		m_startGroups.push_back( newGroup );
	}
}


int EditorGizmos::GetNextId()
{
	gpstring sDefault = GetDefaultStartingGroup();
	int id = 1;
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		if ( sDefault.same_no_case( (*j).sGroup ) )
		{
			StartingPositions::iterator i;
			i = (*j).positions.begin();
			while ( i != (*j).positions.end() )
			{
				if ( (*i).id == id )
				{
					++id;
					i = (*j).positions.begin();
				}
				else
				{
					++i;
				}
			}
		}
	}

	return id;
}


void EditorGizmos::SetStartingPosition( int id, SiegePos spos )
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		StartingPositions::iterator i;
		for ( i = (*j).positions.begin(); i != (*j).positions.end(); ++i )
		{
			if ( (*i).gizmoId == id )
			{
				(*i).spos = spos;
				return;
			}
		}
	}

	gEditorRegion.SetRegionDirty();
}


SiegePos EditorGizmos::GetStartingPosition( int id )
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		StartingPositions::iterator i;
		for ( i = (*j).positions.begin(); i != (*j).positions.end(); ++i )
		{
			if ( (*i).gizmoId == id )
			{
				return (*i).spos;
			}
		}
	}

	return SiegePos();
}


gpstring EditorGizmos::GetDefaultStartingGroup()
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		if ( (*j).bDefault )
		{
			return (*j).sGroup;
		}
	}

	StartingGroup defaultGroup;
	defaultGroup.bDefault = true;
	defaultGroup.sGroup = "default";
	defaultGroup.sDescription = "This is the default group.";
	defaultGroup.bDevOnly = false;
	defaultGroup.id = 0;

	m_startGroups.push_back( defaultGroup );
	return defaultGroup.sGroup;
}


StringVec EditorGizmos::GetStartingGroups()
{
	StringVec groups;
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		groups.push_back( (*j).sGroup );
	}

	return groups;
}


gpstring EditorGizmos::GetStartingGroup( int id )
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		StartingPositions::iterator i;
		for ( i = (*j).positions.begin(); i != (*j).positions.end(); ++i )
		{
			if ( (*i).gizmoId == id )
			{
				return (*j).sGroup;
			}
		}
	}

	return "";
}


void EditorGizmos::AddStartingGroup( gpstring sGroup, gpstring sDescription )
{
	StartingGroup newGroup;
	newGroup.bDefault = false;
	newGroup.sDescription = sDescription;
	newGroup.sGroup = sGroup;
	newGroup.bDevOnly = false;
	m_startGroups.push_back( newGroup );
	gEditorRegion.SetRegionDirty();
}
	

void EditorGizmos::EditStartingGroup( gpstring sGroup, gpstring sDescription, bool bDefault, bool bDevOnly, gpstring sScreenName )
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		if ( sGroup.same_no_case( (*j).sGroup ) )
		{
			(*j).bDefault = bDefault;
			(*j).sDescription = sDescription;			
			(*j).bDevOnly = bDevOnly;
			(*j).sScreenName = sScreenName;
		}
		else if ( (*j).bDefault && bDefault )
		{
			(*j).bDefault = false;
		}
	}

	gEditorRegion.SetRegionDirty();
}

void EditorGizmos::DeleteStartingGroup( gpstring sGroup )
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		if ( sGroup.same_no_case( (*j).sGroup ) )
		{
			std::vector< DWORD > ids;
			StartingPositions::iterator i;
			for ( i = (*j).positions.begin(); i != (*j).positions.end(); ++i )
			{
				ids.push_back( (*i).gizmoId );				
			}

			std::vector< DWORD >::iterator k;
			for ( k = ids.begin(); k != ids.end(); ++k )
			{
				DeleteStartingPosition( *k );
			}

			m_startGroups.erase( j );

			gEditorRegion.SetRegionDirty();
			return;
		}
	}
}


int EditorGizmos::GetStartingPositionFromGizmo( int gizmoId )
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		StartingPositions::iterator i;
		for ( i = (*j).positions.begin(); i != (*j).positions.end(); ++i )
		{
			if ( (*i).gizmoId == gizmoId )
			{
				return (*i).id;
			}
		}
	}

	return 0;
}


void EditorGizmos::GetStartingGroupProperties( gpstring sGroup, gpstring & sDescription, bool & bDefault, bool & bDevOnly, gpstring & sScreenName )
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		if ( sGroup.same_no_case( (*j).sGroup ) )
		{
			sScreenName = (*j).sScreenName;
			sDescription = (*j).sDescription;
			bDefault = (*j).bDefault;
			bDevOnly = (*j).bDevOnly;
		}
	}
}


void EditorGizmos::AddStartingGroupWorldLevel( gpstring sGroup, gpstring sWorld, int level )
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		if ( sGroup.same_no_case( (*j).sGroup ) )
		{
			WorldLevels::iterator iLevel;
			for ( iLevel = (*j).levels.begin(); iLevel != (*j).levels.end(); ++iLevel )
			{
				if ( (*iLevel).sWorld.same_no_case( sWorld ) )
				{
					(*iLevel).level = level;
					return;
				}
			}

			WorldLevel worldLevel;
			worldLevel.level = level;
			worldLevel.sWorld = sWorld;
			(*j).levels.push_back( worldLevel );
			gEditorRegion.SetRegionDirty();
			return;
		}
	}
}


void EditorGizmos::RemoveStartingGroupWorldLevel( gpstring sGroup, gpstring sWorld )
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		if ( sGroup.same_no_case( (*j).sGroup ) )
		{
			WorldLevels::iterator iLevel;
			for ( iLevel = (*j).levels.begin(); iLevel != (*j).levels.end(); ++iLevel )
			{
				if ( (*iLevel).sWorld.same_no_case( sWorld ) )
				{
					(*j).levels.erase( iLevel );
					gEditorRegion.SetRegionDirty();
					return;
				}
			}
		}
	}	
}


void EditorGizmos::RemoveAllStartingGroupWorldLevels( gpstring sGroup )
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		if ( sGroup.same_no_case( (*j).sGroup ) )
		{
			(*j).levels.clear();
			gEditorRegion.SetRegionDirty();
			return;
		}
	}
}


void EditorGizmos::GetStartingGroupWorldLevels( gpstring sGroup, WorldLevels & worldLevels )
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		if ( sGroup.same_no_case( (*j).sGroup ) )
		{
			worldLevels = (*j).levels;
			return;
		}
	}
}


void EditorGizmos::SetPositionStartGroup( int id, gpstring sGroup )
{
	StartingPos SPos;
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		StartingPositions::iterator i;
		for ( i = (*j).positions.begin(); i != (*j).positions.end(); ++i )
		{
			if ( (*i).gizmoId == id )
			{
				SPos = (*i);
				(*j).positions.erase( i );
				break;
			}
		}
	}

	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		if ( (*j).sGroup.same_no_case( sGroup ) )
		{
			(*j).positions.push_back( SPos );
			gEditorRegion.SetRegionDirty();
			return;
		}
	}
}


void EditorGizmos::GetPositionStartGroup( int id, gpstring & sGroup )
{
	StartingPos SPos;
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		StartingPositions::iterator i;
		for ( i = (*j).positions.begin(); i != (*j).positions.end(); ++i )
		{
			if ( (*i).gizmoId == id )
			{
				sGroup = (*j).sGroup;
				return;
			}
		}
	}
}


void EditorGizmos::SetStartingPosCamera( int id, SiegePos spos, float azimuth, float orbit, float distance )
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		StartingPositions::iterator i;
		for ( i = (*j).positions.begin(); i != (*j).positions.end(); ++i )
		{
			if ( (*i).gizmoId == id )
			{
				(*i).camPos		= spos;			
				(*i).azimuth	= azimuth;
				(*i).orbit		= orbit;
				(*i).distance	= distance;
				gEditorRegion.SetRegionDirty();
				return;
			}
		}
	}
}


void EditorGizmos::SetSelectedCameraPos()
{
	if ( gGizmoManager.GetLeadGizmo() )
	{
		SiegePos camPos;
		camPos.node = GetStartingPosition( gGizmoManager.GetLeadGizmo()->id ).node;

		SiegeNodeHandle handle(gSiegeEngine.NodeCache().UseObject(camPos.node));
		SiegeNode& node = handle.RequestObject(gSiegeEngine.NodeCache());			

		camPos.pos = node.WorldToNodeSpace( gEditorCamera.GetPosition() );		
		gEditorGizmos.SetStartingPosCamera( gGizmoManager.GetLeadGizmo()->id, camPos, 
											gEditorCamera.GetAzimuth(), 
											gEditorCamera.GetOrbit(), 
											gEditorCamera.GetDistance() );
	
	}
}


void EditorGizmos::SetCameraToSelectedPos()
{
	StartingGroups::iterator j;
	for ( j = m_startGroups.begin(); j != m_startGroups.end(); ++j )
	{
		StartingPositions::iterator i;
		for ( i = (*j).positions.begin(); i != (*j).positions.end(); ++i )
		{
			if ( (*i).gizmoId == gGizmoManager.GetLeadGizmo()->id )
			{
				gEditorCamera.SetAzimuthAngle( (*i).azimuth );
				gEditorCamera.SetOrbitAngle( (*i).orbit );
				gEditorCamera.SetDistance( (*i).distance );

				SiegeNodeHandle handle(gSiegeEngine.NodeCache().UseObject((*i).camPos.node));
				SiegeNode& node = handle.RequestObject(gSiegeEngine.NodeCache());				
				gEditorCamera.SetTargetPosition( node.NodeToWorldSpace( (*i).camPos.pos ) );

				gEditorCamera.Update();
				return;
			}
		}	
	}
}


void EditorGizmos::SaveDecalsToGas()
{
	FuelHandle hDecals = gEditorRegion.GetRegionHandle()->GetChildBlock( "decals" );
	if ( !hDecals.IsValid() )
	{
		hDecals = gEditorRegion.GetRegionHandle()->CreateChildDirBlock( "decals" );
	}

	FuelHandle hMain = hDecals->GetChildBlock( "decals" );
	if ( hMain.IsValid() )
	{
		hDecals->DestroyChildBlock( hMain );
	}
	hMain = hDecals->CreateChildBlock( "decals", "decals.gas" );

	std::vector< DWORD > decals;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetGizmosOfType( GIZMO_DECAL, decals );
	for ( i = decals.begin(); i != decals.end(); ++i )
	{
		SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( gGizmoManager.GetGizmo( *i )->index );		
		gpstring sTexture = gSiegeEngine.Renderer().GetTexturePathname( pDecal->GetDecalTexture() );
		gpstring sKeyName = sTexture;
		int pos = sTexture.find_last_of( "\\" );
		if ( pos != gpstring::npos )
		{
			sKeyName = sTexture.substr( pos+1, sTexture.size() );
		}

		stringtool::RemoveExtension( sKeyName );

		FuelHandle hDecal = hMain->CreateChildBlock( "*" );
		hDecal->SetType( "decal" );

		hDecal->Set( "texture", sTexture.c_str() );
		hDecal->Set( "horizontal_meters", pDecal->GetHorizontalMeters() );
		hDecal->Set( "vertical_meters", pDecal->GetVerticalMeters() );
		hDecal->Set( "near_plane", pDecal->GetNearPlane() );
		hDecal->Set( "far_plane", pDecal->GetFarPlane() );
		hDecal->Set( "guid", pDecal->GetGUID().GetValue(), FVP_HEX_INT );
		hDecal->Set( "lod", pDecal->GetLevelOfDetail() );
				
		matrix_3x3 orient = pDecal->GetDecalOrientation();
		gpstring sOrient;
		sOrient.assignf(	"%f,%f,%f,%f,%f,%f,%f,%f,%f", 
							orient.v00, orient.v01, orient.v02,
							orient.v10, orient.v11, orient.v12,
							orient.v20, orient.v21, orient.v22 );

		hDecal->Set( "decal_orientation", sOrient.c_str() );
		
		gpstring sOrigin;
		sOrigin.assignf( "%f,%f,%f,0x%08X",	pDecal->GetDecalOrigin().pos.x, pDecal->GetDecalOrigin().pos.y,
											pDecal->GetDecalOrigin().pos.z, pDecal->GetDecalOrigin().node.GetValue() );		
		hDecal->Set( "decal_origin", sOrigin.c_str() );

		gpstring sPos;
		sPos.assignf( "%f,%f,%f,0x%08X",	gGizmoManager.GetGizmo( *i )->spos.pos.x, 
										gGizmoManager.GetGizmo( *i )->spos.pos.y,
										gGizmoManager.GetGizmo( *i )->spos.node.GetValue() );		
	}
}


void EditorGizmos::LoadDecalsFromGas()
{
	FuelHandle hDecals = gEditorRegion.GetRegionHandle()->GetChildBlock( "decals" );
	if ( hDecals.IsValid() )
	{
		hDecals = hDecals->GetChildBlock( "decals" );
		if ( hDecals.IsValid() )
		{
			FuelHandleList hlDecals = hDecals->ListChildBlocks( 1, "decal" );
			FuelHandleList::iterator i;
			for ( i = hlDecals.begin(); i != hlDecals.end(); ++i )
			{
				float horizMeters = 0;
				float vertMeters = 0;
				float nearPlane = 0;
				float farPlane = 0;
				matrix_3x3 orient;
				SiegePos sorigin;
				gpstring sTexture;				
				gpstring sTemp;
				int decalGuid = 0;
				float lod = 1.0f;

				(*i)->Get( "texture", sTexture );
				(*i)->Get( "horizontal_meters", horizMeters );
				(*i)->Get( "vertical_meters", vertMeters );
				(*i)->Get( "near_plane", nearPlane );
				(*i)->Get( "far_plane", farPlane );
				(*i)->Get( "decal_orientation", sTemp );
				(*i)->Get( "guid", decalGuid );
				(*i)->Get( "lod", lod );

				stringtool::GetDelimitedValue( sTemp, ',', 0, orient.v00 );
				stringtool::GetDelimitedValue( sTemp, ',', 1, orient.v01 );
				stringtool::GetDelimitedValue( sTemp, ',', 2, orient.v02 );
				stringtool::GetDelimitedValue( sTemp, ',', 3, orient.v10 );
				stringtool::GetDelimitedValue( sTemp, ',', 4, orient.v11 );
				stringtool::GetDelimitedValue( sTemp, ',', 5, orient.v12 );
				stringtool::GetDelimitedValue( sTemp, ',', 6, orient.v20 );
				stringtool::GetDelimitedValue( sTemp, ',', 7, orient.v21 );
				stringtool::GetDelimitedValue( sTemp, ',', 8, orient.v22 );

				(*i)->Get( "decal_origin", sTemp );
				int nodeId = 0;
				stringtool::GetDelimitedValue( sTemp, ',', 0, sorigin.pos.x );
				stringtool::GetDelimitedValue( sTemp, ',', 1, sorigin.pos.y );
				stringtool::GetDelimitedValue( sTemp, ',', 2, sorigin.pos.z );
				stringtool::GetDelimitedValue( sTemp, ',', 3, nodeId );
				sorigin.node = siege::database_guid( nodeId );

				int index = gSiegeDecalDatabase.CreateDecal( sorigin, orient, horizMeters, vertMeters, 
															 nearPlane, farPlane, sTexture );

				gpstring sRandom = gEditorTerrain.GenerateRandomGUID();
				siege::database_guid gizmoId( sRandom.c_str() );

				gGizmoManager.InsertGizmo( GIZMO_DECAL, sorigin, gizmoId.GetValue() );
				gGizmoManager.GetGizmo( gizmoId.GetValue() )->index = index; 
				SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( index );
				pDecal->SetGUID( siege::database_guid( decalGuid ) );
				pDecal->SetLevelOfDetail( lod );
			}
		}
	}
}


void EditorGizmos::GrabDecalGuid()
{
	std::vector< DWORD > objects;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedObjectIDs( objects );
	if ( objects.size() != 0 )
	{
		i = objects.begin();		
		Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );
		SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( pGizmo->index );
		
		gpstring sGuid;
		sGuid.assignf( "0x%08X", pDecal->GetGUID().GetValue() );

		// Open the clipboard, and empty it. 
		if ( !OpenClipboard( gEditor.GetHWnd() ) ) 
		{
			return; 
		}
		EmptyClipboard();

		LPTSTR  lptstrCopy; 
		HGLOBAL hglbCopy; 

		hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (sGuid.size() + 1) * sizeof(TCHAR) ); 
		if (hglbCopy == NULL) 
		{ 
			CloseClipboard(); 
			return; 
		} 

		// Lock the handle and copy the text to the buffer. 
		lptstrCopy = (char *)GlobalLock(hglbCopy); 
		memcpy(lptstrCopy, sGuid.c_str(), sGuid.size() * sizeof(TCHAR)); 
		lptstrCopy[sGuid.size()] = (TCHAR) 0;    // null character 
		GlobalUnlock( hglbCopy) ; 

		// Place the handle on the clipboard. 

	    SetClipboardData(CF_TEXT, hglbCopy); 

		CloseClipboard();		
    } 					
}


void EditorGizmos::RecalcDecals()
{
	std::vector< DWORD > decals;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetGizmosOfType( GIZMO_DECAL, decals );
	for ( i = decals.begin(); i != decals.end(); ++i )
	{
		SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( gGizmoManager.GetGizmo( *i )->index );
		siege::database_guid decalGuid = pDecal->GetGUID();

		Gizmo * pGizmo = gGizmoManager.GetGizmo( (*i) );
		int index = pGizmo->index;
		CreateDecalFromGizmo( *pGizmo );
		DeleteDecal( index );

		pDecal = gSiegeDecalDatabase.GetDecalPointer( pGizmo->index );
		pDecal->SetGUID( decalGuid );
	}
}


void EditorGizmos::CutDecal()
{
	CopyDecal();
	m_bCutting = true;
}


void EditorGizmos::CopyDecal()
{
	m_bCutting = false;
	m_clipboard.clear();
	std::vector< DWORD > objects;	
	gGizmoManager.GetSelectedGizmosOfType( GIZMO_DECAL, m_clipboard );	
}


void EditorGizmos::PasteDecal()
{
	if ( !gSiegeEditorView.IsMouseOnTerrain() )
	{
		return;
	}

	SiegePos spos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();

	int x_diff = 0;
	int y_diff = 0;
	int z_diff = 0;

	std::vector< DWORD > newObjects;

	std::vector< DWORD >::iterator i;
	for ( i = m_clipboard.begin(); i != m_clipboard.end(); ++i )
	{
		Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );
		SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( pGizmo->index );
		if ( i == m_clipboard.begin() )
		{
			SiegeNodeHandle Srchandle( gSiegeEngine.NodeCache().UseObject( spos.node ) );
			SiegeNode& Srcnode = Srchandle.RequestObject( gSiegeEngine.NodeCache() );	
			spos.pos = Srcnode.NodeToWorldSpace( spos.pos );

			SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( pGizmo->spos.node ) );
			SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );	

			x_diff = spos.pos.x - node.NodeToWorldSpace( pGizmo->spos.pos ).x;
			y_diff = spos.pos.y - node.NodeToWorldSpace( pDecal->GetDecalOrigin().pos ).y;
			z_diff = spos.pos.z - node.NodeToWorldSpace( pGizmo->spos.pos ).z;
		}

		SiegeNodeHandle objHandle( gSiegeEngine.NodeCache().UseObject( pGizmo->spos.node ) );
		SiegeNode& objNode = objHandle.RequestObject( gSiegeEngine.NodeCache() );

		SiegePos newPos;
		vector_3 worldPos;
				
		if ( m_clipboard.size() != 1 )
		{
			worldPos.x = objNode.NodeToWorldSpace( pGizmo->spos.pos ).x + x_diff;
			worldPos.y = objNode.NodeToWorldSpace( pGizmo->spos.pos ).y + y_diff;
			worldPos.z = objNode.NodeToWorldSpace( pGizmo->spos.pos ).z + z_diff;					
						
			newPos.pos = objNode.WorldToNodeSpace( worldPos );
			newPos.node = pGizmo->spos.node;
		}
		else
		{
			newPos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();			
		}
		
		gGizmoManager.AccurateAdjustPositionToTerrain( newPos );
		
		gpstring sTexture = gSiegeEngine.Renderer().GetTexturePathname( pDecal->GetDecalTexture() );
		gpstring sKeyName = sTexture;
		int pos = sTexture.find_last_of( "\\" );
		if ( pos != gpstring::npos )
		{
			sKeyName = sTexture.substr( pos+1, sTexture.size() );
		}		

		DWORD id = AddDecal( sKeyName, newPos );		
		newObjects.push_back( id );
		gGizmoManager.SelectGizmo( id );		
	}

	if ( m_bCutting )
	{
		std::vector< DWORD > objects = m_clipboard;
		for ( i = objects.begin(); i != objects.end(); ++i )
		{	
			Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );			
			DeleteDecal( pGizmo->index );
		}

		m_clipboard.clear();
		m_clipboard = newObjects;
		m_bCutting = false;
	}		
}

void EditorGizmos::Draw()
{
	std::vector< DWORD >::iterator i;
	for ( i = m_clipboard.begin(); i != m_clipboard.end(); ++i )
	{
		Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );
		if ( pGizmo )
		{
			gpstring sLabel;
			if ( m_bCutting )
			{
				sLabel = "X";
			}
			else
			{
				sLabel = "C";
			}
			
			SiegePos spos = pGizmo->spos;
			spos.pos.y += 2.0f;
			spos.pos.x += 0.3f;		
			
			gSiegeEngine.GetLabels().DrawLabel( &gServices.GetConsoleFont(), spos, sLabel );
		}
	}
}