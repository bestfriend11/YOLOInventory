//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorHotpoints.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "PrecompEditor.h"
#include "RapiOwner.h"
#include "RapiPrimitive.h"
#include "EditorRegion.h"
#include "EditorHotpoints.h"
#include "EditorTerrain.h"
#include "MenuCommands.h"
#include "Dialog_Hotpoint_Properties.h"
#include "siege_hotpoint_database.h"

using namespace siege;

void EditorHotpoints::Draw()
{
	if ( GetCanDraw() )
	{		
		SiegeEngine & engine = gSiegeEngine;
		engine.Renderer().SetWorldMatrix( matrix_3x3(), vector_3() );
		RP_DrawRay( engine.Renderer(), GetCurrentDirection() * 100, 0xFFFF00FF );		
	}
}


void EditorHotpoints::Load()
{
	FuelHandle hEditor = gEditorRegion.GetRegionHandle();
	hEditor = hEditor->GetChildBlock( "editor" );
	if ( hEditor.IsValid() == false )
	{
		return;
	}
	hEditor = hEditor->GetChildBlock( "hotpoints" );
	if ( hEditor.IsValid() == false )
	{
		return;
	}

	{
		siege::SiegeEngine& engine = gSiegeEngine;
		FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
		FrustumNodeColl::iterator n;
		for( n = nodeColl.begin(); n != nodeColl.end(); ++n )
		{		
			(*n)->ClearHotpoints();
		}
	}
	
	int masterId = 0;
	bool bMasterId = false;
	FuelHandle hInfo = gEditorRegion.GetMapHandle()->GetChildBlock( "info" );
	if ( hInfo )
	{
		FuelHandle hMapPoints = hInfo->GetChildBlock( "hotpoints" );		
		if ( hMapPoints.IsValid() )
		{
			FuelHandle hNorth = hMapPoints->GetChildBlock( "north" );
			if ( hNorth )
			{
				if ( hNorth->Get( "id", masterId ) )
				{
					bMasterId = true;
				}
			}
		}
	}
	else
	{
		gperrorf(( "VERY BAD - Map: %s does not have an 'info' directory defined.", gEditorRegion.GetMapHandle()->GetName() )); 
	}

	FuelHandleList hlHotpoints = hEditor->ListChildBlocks( 1 );
	if ( hlHotpoints.size() != 0 )
	{
		FuelHandleList::iterator i = hlHotpoints.begin();
		
		SiegePos sdir;
		gpstring sDirection;
		(*i)->Get( "direction", sDirection );

		stringtool::GetDelimitedValue( sDirection, ',', 0, sdir.pos.x );
		stringtool::GetDelimitedValue( sDirection, ',', 1, sdir.pos.y );
		stringtool::GetDelimitedValue( sDirection, ',', 2, sdir.pos.z );
		SetCurrentDirection( sdir.pos );
		
		int id = 0;
		(*i)->Get( "id", id );

		if ( id != masterId && bMasterId )
		{
			MessageBoxEx( gEditor.GetMainHWnd(), "The region's north vector ID does not match the map.  The region hotpoint ID will be automatically adjusted.", "Region North Vector", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			id = masterId;
		}

		SetSelectedID( id );

		sdir.node	= gEditorTerrain.GetTargetNode();
		vector_3 world_dir	= sdir.WorldPos();
		if( IsZero( world_dir ) )
		{
			gperror( "A directional hotpoint cannot have a zero direction!" );		
		}
		world_dir.Normalize();

		siege::SiegeEngine& engine = gSiegeEngine;
		FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
		FrustumNodeColl::iterator n;
		for( n = nodeColl.begin(); n != nodeColl.end(); ++n )
		{
			(*n)->InsertHotpoint( (*n)->GetTransposeOrientation() * world_dir, id );				
		}		
	}
}


void EditorHotpoints::Save()
{	
	// First, save out the map hotpoints.gas file.
	FuelHandle hInfo = gEditorRegion.GetMapHandle()->GetChildBlock( "info" );
	FuelHandle hMapPoints = hInfo->GetChildBlock( "hotpoints" );
	if ( hMapPoints.IsValid() )
	{		
		hInfo->DestroyChildBlock( hMapPoints );
	}
	hMapPoints = hInfo->CreateChildBlock( "hotpoints", "hotpoints.gas" );

	siege::SiegeHotpointList siegePoints = gSiegeHotpointDatabase.GetHotpointList();
	siege::SiegeHotpointList::iterator iPoint;
	for ( iPoint = siegePoints.begin(); iPoint != siegePoints.end(); ++iPoint )
	{
		FuelHandle hHotpoint = hMapPoints->CreateChildBlock( "north" );
		hHotpoint->SetType( "hotpoint" );
		hHotpoint->Set( "id", (int)((*iPoint).m_Id ), FVP_HEX_INT );
	}	

	// Get access the the editor gas file
	FuelHandle hEditor = gEditorRegion.GetRegionHandle();
	hEditor = hEditor->GetChildBlock( "editor" );
	if ( hEditor.IsValid() == false )
	{
		hEditor = gEditorRegion.GetRegionHandle()->CreateChildDirBlock( "editor" );
	}

	FuelHandle hOld = hEditor->GetChildBlock( "hotpoints" );
	if ( hOld.IsValid() )
	{
		hEditor->DestroyChildBlock( hOld );
	}
	hEditor = hEditor->CreateChildBlock( "hotpoints", "hotpoints.gas" );
	
	siege::SiegeEngine& engine = gSiegeEngine;
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator n;
	for( n = nodeColl.begin(); n != nodeColl.end(); ++n )
	{		
		(*n)->ClearHotpoints();
	}

	// Save out the generic editor information to gas
	gpstring sFuelName;
	sFuelName.assignf( "0x%08X", GetSelectedID() );
	FuelHandle hHotpoint = hEditor->CreateChildBlock( sFuelName );		
	hHotpoint->Set( "id", GetSelectedID(), FVP_HEX_INT );	
			
	// Save out type specific information to gas
	hHotpoint->SetType( "hotpoint_directional" );					
	gpstring sDirection;
	sDirection.assignf( "%f,%f,%f", GetCurrentDirection().x, 
									GetCurrentDirection().y,
									GetCurrentDirection().z );
	hHotpoint->Set( "direction", sDirection );

	SiegePos sdir;
	sdir.pos	= GetCurrentDirection();
	sdir.node	= gEditorTerrain.GetTargetNode();
	vector_3 world_dir	= sdir.WorldPos();
	if( IsZero( world_dir ) )
	{
		return;
	}
	world_dir.Normalize();

	for( n = nodeColl.begin(); n != nodeColl.end(); ++n )
	{					
		(*n)->InsertHotpoint( (*n)->GetTransposeOrientation() * world_dir, GetSelectedID() );
	}	
}


void EditorHotpoints::Delete( int id, bool bFull )
{
	if ( bFull )
	{
		gSiegeHotpointDatabase.RemoveHotpoint( id );
	}

	siege::SiegeEngine& engine = gSiegeEngine;
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator i;
	for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
	{
		(*i)->RemoveHotpoint( id );		
	}
	gGizmoManager.RemoveGizmo( id );
}


void EditorHotpoints::AddDirectionalHotpoint( vector_3 vDir, int id )
{
	gpstring sName;
	SiegePos sdir;
	sdir.pos = vDir;
	sdir.node = gEditorTerrain.GetTargetNode();

	sName = GetSelectedName();
	if ( id == - 1 )
	{
		id = GetSelectedID();
		sName = GetSelectedName();
		if ( id == -1 )
		{
			return;
		}
	}

	if ( sName.empty() )
	{
		sName = "north";
	}

	bool bExists = false;
	siege::SiegeHotpointList hotpoints = gSiegeHotpointDatabase.GetHotpointList();
	siege::SiegeHotpointList::iterator i;
	for ( i = hotpoints.begin(); i != hotpoints.end(); ++i )
	{
		if ( (*i).m_Id == GetSelectedID() )
		{
			bExists = true;
			break;
		}		
	}

	if ( bExists )
	{
		Delete( id, true );	
	}
	gSiegeHotpointDatabase.InsertHotpoint( ToUnicode( sName.c_str() ), id );

	vector_3 world_dir	= sdir.WorldPos();
	if( IsZero( world_dir ) )
	{
		gperror( "A directional hotpoint cannot have a zero direction!" );		
	}
	world_dir.Normalize();

	siege::SiegeEngine& engine = gSiegeEngine;
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator n;
	for( n = nodeColl.begin(); n != nodeColl.end(); ++n )
	{
		(*n)->InsertHotpoint( (*n)->GetTransposeOrientation() * world_dir, id );				
	}

	SetSelectedID( id );
	SetCurrentDirection( sdir.pos );	
}
