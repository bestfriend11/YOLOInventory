//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorLights.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "PrecompEditor.h"
#include "EditorLights.h"
#include "siege_light_structs.h"
#include "siege_light_database.h"
#include "Preferences.h"
#include "RapiPrimitive.h"
#include "EditorTerrain.h"
#include "EditorRegion.h"

using namespace siege;


EditorLights::EditorLights()
{
	m_bCutting = false;
}


siege::database_guid EditorLights::AddDefaultPointLight( SiegePos spos )
{
	if ( spos.node == UNDEFINED_GUID ) {
		return UNDEFINED_GUID;
	}	
	
	// Grab access to the siege engine
	SiegeEngine & engine = gSiegeEngine;
	siege::database_guid lightGUID = engine.LightDatabase().InsertPointLightSource( spos, vector_3(1,1,1), UNDEFINED_GUID, true, true, 0.0f, 20.0f );

	gGizmoManager.InsertGizmo( GIZMO_POINTLIGHT, spos, lightGUID.GetValue() ); 
	return lightGUID;
}


siege::database_guid EditorLights::AddDefaultSpotlight( SiegePos spos )
{
	if ( spos.node == UNDEFINED_GUID ) {
		return UNDEFINED_GUID;
	}
	
	// Grab access to the siege engine
	SiegeEngine & engine = gSiegeEngine;

	SiegePos sdirection;
	sdirection.node = spos.node;
	sdirection.pos	= vector_3( 0, -1, 0 );

	spos.pos.y		+= 1.0f;

	siege::database_guid lightGUID = engine.LightDatabase().InsertSpotLightSource( spos, sdirection, vector_3(1,1,1), UNDEFINED_GUID, true, true, RadiansFrom( 20 ), RadiansFrom( 45 ) );
	gGizmoManager.InsertGizmo( GIZMO_SPOTLIGHT, spos, lightGUID.GetValue() );
	return lightGUID;
}


siege::database_guid EditorLights::AddDefaultDirectionalLight( SiegePos spos )
{
	if ( spos.node == UNDEFINED_GUID ) {
		return UNDEFINED_GUID;
	}

	// Grab access to the siege engine
	SiegeEngine & engine = gSiegeEngine;

	spos.pos.x = 0.0f;
	spos.pos.y = 1.0f;
	spos.pos.z = 0.0f;

	siege::database_guid lightGUID = engine.LightDatabase().InsertDirectionalLightSource( spos, vector_3(1,1,1), UNDEFINED_GUID, true );
	gGizmoManager.InsertGizmo( GIZMO_DIRLIGHT, spos, lightGUID.GetValue() );
	
	gEditorRegion.SetRegionDirty();

	return lightGUID;
}


siege::database_guid EditorLights::AddLight( siege::database_guid lightGUID, SiegePos spos, SiegePos sdirection, siege::LightDescriptor & ld ) 
{
	if (( spos.node == UNDEFINED_GUID ) && ( sdirection.node == UNDEFINED_GUID )) {
		return UNDEFINED_GUID;
	}

	// Grab access to the siege engine
	SiegeEngine & engine = gSiegeEngine;
	
	float red	= (float)((ld.m_Color & 0x00FF0000) >> 16 ) / 255.0f;
	float green	= (float)((ld.m_Color & 0x0000FF00) >> 8 ) / 255.0f;
	float blue	= (float)( ld.m_Color & 0x000000FF ) / 255.0f;
	vector_3 color( red, green, blue );

	if ( ld.m_Type == siege::LT_POINT ) {
		lightGUID = engine.LightDatabase().InsertPointLightSource(	spos, color, lightGUID, true, true, 
																	ld.m_InnerRadius, ld.m_OuterRadius, 
																	ld.m_Intensity, ld.m_bDrawShadow, 
																	ld.m_bOccludeGeometry, ld.m_bAffectsActors,
																	ld.m_bAffectsItems, ld.m_bAffectsTerrain,
																	ld.m_bOnTimer );
		if ( lightGUID != UNDEFINED_GUID )
		{
			gGizmoManager.InsertGizmo( GIZMO_POINTLIGHT, spos, lightGUID.GetValue() );
		}
	}
	else if ( ld.m_Type == siege::LT_DIRECTIONAL ) {
		lightGUID = engine.LightDatabase().InsertDirectionalLightSource(	sdirection, color, lightGUID, true, true,
																			ld.m_Intensity, ld.m_bDrawShadow, ld.m_bOccludeGeometry,
																			ld.m_bAffectsActors,
																			ld.m_bAffectsItems, ld.m_bAffectsTerrain,
																			ld.m_bOnTimer );
		if ( lightGUID != UNDEFINED_GUID )
		{
			gGizmoManager.InsertGizmo( GIZMO_DIRLIGHT, sdirection, lightGUID.GetValue() );
		}
	}	
	else if ( ld.m_Type == siege::LT_SPOT ) {
		lightGUID = engine.LightDatabase().InsertSpotLightSource(	spos, sdirection, color, lightGUID, true, true,
																	ld.m_InnerRadius, ld.m_OuterRadius, 
																	ld.m_Intensity, ld.m_bDrawShadow, 
																	ld.m_bOccludeGeometry, ld.m_bAffectsActors,
																	ld.m_bAffectsItems, ld.m_bAffectsTerrain,
																	ld.m_bOnTimer );
		if ( lightGUID != UNDEFINED_GUID )
		{
			gGizmoManager.InsertGizmo( GIZMO_SPOTLIGHT, spos, lightGUID.GetValue() );
		}
	}

	return lightGUID;
}


void EditorLights::DeleteLight( siege::database_guid lightGUID )
{	
	gSiegeEngine.LightDatabase().DestroyLight( lightGUID );
}


void EditorLights::SetLightPosition( siege::database_guid lightGUID, SiegePos spos )
{
	SiegeEngine & engine = gSiegeEngine;
	engine.LightDatabase().SetLightPosition( lightGUID, spos );
	gEditorRegion.SetRegionDirty();
}


void EditorLights::SetLightDirection( siege::database_guid lightGUID, SiegePos sdir )
{
	SiegeEngine & engine = gSiegeEngine;
	engine.LightDatabase().SetLightDirection( lightGUID, sdir );
	gEditorRegion.SetRegionDirty();
}

void EditorLights::SetLightActive( siege::database_guid lightGUID, bool bActive )
{
	LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( lightGUID );
	ld.m_bActive = bActive;
	gSiegeEngine.LightDatabase().SetLightDescriptor( lightGUID, ld );
	gEditorRegion.SetRegionDirty();
}


void EditorLights::SetLightColor( siege::database_guid lightGUID, DWORD color )
{
	LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( lightGUID );
	ld.m_Color = color;
	gSiegeEngine.LightDatabase().SetLightDescriptor( lightGUID, ld );
	gEditorRegion.SetRegionDirty();
}


void EditorLights::SetLightIntensity( siege::database_guid lightGUID, float intensity )
{
	LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( lightGUID );
	ld.m_Intensity = intensity;
	gSiegeEngine.LightDatabase().SetLightDescriptor( lightGUID, ld );
	gEditorRegion.SetRegionDirty();
}


void EditorLights::SetLightIRadius( siege::database_guid lightGUID, float iradius )
{
	LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( lightGUID );
	ld.m_InnerRadius = iradius;
	gSiegeEngine.LightDatabase().SetLightDescriptor( lightGUID, ld );
	gEditorRegion.SetRegionDirty();
}


void EditorLights::SetLightOcclusion( siege::database_guid lightGUID, bool bOcclude )
{
	LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( lightGUID );
	ld.m_bOccludeGeometry = bOcclude;
	gSiegeEngine.LightDatabase().SetLightDescriptor( lightGUID, ld );	
	gEditorRegion.SetRegionDirty();
}


void EditorLights::SetLightRadius( siege::database_guid lightGUID, float radius )
{
	LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( lightGUID );
	ld.m_OuterRadius = radius;
	gSiegeEngine.LightDatabase().SetLightDescriptor( lightGUID, ld );
	gEditorRegion.SetRegionDirty();
}


void EditorLights::SetLightShadow( siege::database_guid lightGUID, bool bDrawShadow )
{
	LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( lightGUID );
	ld.m_bDrawShadow = bDrawShadow;
	gSiegeEngine.LightDatabase().SetLightDescriptor( lightGUID, ld );
	gEditorRegion.SetRegionDirty();
}


void EditorLights::SetLightAffectActors( siege::database_guid lightGUID, bool bAffect )	
{
	LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( lightGUID );
	ld.m_bAffectsActors = bAffect;
	gSiegeEngine.LightDatabase().SetLightDescriptor( lightGUID, ld );
	gEditorRegion.SetRegionDirty();
}


void EditorLights::SetLightAffectItems( siege::database_guid lightGUID, bool bAffect )	
{
	LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( lightGUID );
	ld.m_bAffectsItems = bAffect;
	gSiegeEngine.LightDatabase().SetLightDescriptor( lightGUID, ld );
	gEditorRegion.SetRegionDirty();
}


void EditorLights::SetLightAffectTerrain( siege::database_guid lightGUID, bool bAffect )
{
	LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( lightGUID );
	ld.m_bAffectsTerrain = bAffect;
	gSiegeEngine.LightDatabase().SetLightDescriptor( lightGUID, ld );
	gEditorRegion.SetRegionDirty();
}

void EditorLights::SetLightOnTimer( siege::database_guid lightGUID, bool bOnTimer )
{
	LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( lightGUID );
	ld.m_bOnTimer = bOnTimer;
	gSiegeEngine.LightDatabase().SetLightDescriptor( lightGUID, ld );
	gEditorRegion.SetRegionDirty();
}



void EditorLights::ComputeStaticLighting( bool bOcclude )
{
	// Don't do any lighting if we are in ignore mode and we aren't requesting a full calc
	if( !bOcclude && gPreferences.GetIgnoreVertexColor() )
	{
		return;
	}
	
	siege::SiegeEngine& engine = gSiegeEngine;
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator i;
	for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
	{
		const SiegeMesh& bmesh = (*i)->GetMeshHandle().RequestObject(engine.MeshCache());
		(*i)->ComputeLighting( bmesh );		
	}		

	gEditorRegion.SetRegionDirty();
}


void EditorLights::DrawDirectionalLights()
{
	SiegeEngine & engine = gSiegeEngine;
	std::vector< DWORD > light_ids;
	std::vector< DWORD >::iterator j;	

	// Draw the selected directionals differently
	gGizmoManager.GetSelectedDirectionalLightIDs( light_ids );
	if ( light_ids.size() != 0 ) {
		for ( j = light_ids.begin(); j != light_ids.end(); ++j ) {

			LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( siege::database_guid((*j)) );		
			FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
			FrustumNodeColl::iterator i;
			for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
			{		
				engine.Renderer().SetWorldMatrix( (*i)->GetCurrentOrientation(), (*i)->GetCurrentCenter() );
				SiegeLightList::const_iterator l = (*i)->FindLightInList( siege::database_guid(*j), (*i)->GetStaticLights() );
				if ( l != (*i)->GetStaticLights().end() )
				{
					RP_DrawRay( engine.Renderer(), (*l).m_Direction * 5, ld.m_Color );										
				}				
			}
		}
	}
	else {
		gGizmoManager.GetDirectionalLightIDs( light_ids );
		for ( j = light_ids.begin(); j != light_ids.end(); ++j ) {
			
			LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( siege::database_guid((*j)) );					
			FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
			FrustumNodeColl::iterator i;
			for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
			{		
				engine.Renderer().SetWorldMatrix( (*i)->GetCurrentOrientation(), (*i)->GetCurrentCenter() );
				SiegeLightList::const_iterator l = (*i)->FindLightInList( siege::database_guid(*j), (*i)->GetStaticLights() );
				if ( l != (*i)->GetStaticLights().end() )
				{
					RP_DrawRay( engine.Renderer(), (*l).m_Direction * 5, ld.m_Color );
				}				
			}
		}		
	}
}


void EditorLights::DrawLightHuds()
{
	if ( gPreferences.GetDrawLightHuds() )
	{
		SiegeEngine & engine = gSiegeEngine;
		std::vector< DWORD > light_ids;
		std::vector< DWORD >::iterator j;	

		// Draw the selected directionals differently
		gGizmoManager.GetSelectedGizmosOfType( GIZMO_POINTLIGHT, light_ids );
		if ( light_ids.size() != 0 ) 
		{
			for ( j = light_ids.begin(); j != light_ids.end(); ++j ) 
			{
				SiegePos spos = gGizmoManager.GetGizmo( *j )->spos;

				LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( siege::database_guid((*j)) );		
				FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
				FrustumNodeColl::iterator i;
				for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
				{		
					engine.Renderer().SetWorldMatrix( (*i)->GetCurrentOrientation(), (*i)->GetCurrentCenter() );
					SiegeLightList::const_iterator l = (*i)->FindLightInList( siege::database_guid(*j), (*i)->GetStaticLights() );
					if ( l != (*i)->GetStaticLights().end() )
					{					
						gWorld.DrawDebugSphere( spos, ld.m_InnerRadius, ld.m_Color, "inner radius" );
						gWorld.DrawDebugSphere( spos, ld.m_OuterRadius, ld.m_Color, "outer radius" );
					}				
				}
			}
		}	
	}
}


void EditorLights::ReinsertLightSource( siege::database_guid lightGUID ) 
{	
	// Get the light descriptor
	LightDescriptor ld	= gSiegeEngine.LightDatabase().GetLightDescriptor( lightGUID );
	
	if( ld.m_Type == LT_POINT )	{
		SiegePos spos = gSiegeEngine.LightDatabase().GetLightNodeSpacePosition( lightGUID );
		gSiegeEngine.LightDatabase().SetLightPosition( lightGUID, spos );
	}
	else if( ld.m_Type == LT_DIRECTIONAL ) {
		SiegePos spos = gSiegeEngine.LightDatabase().GetLightNodeSpaceDirection( lightGUID );
		gSiegeEngine.LightDatabase().SetLightDirection( lightGUID, spos );
	}
}


bool EditorLights::GetColor( DWORD & dwColor )
{
	std::vector< DWORD > light_ids;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedLightIDs( light_ids );
	for ( i = light_ids.begin(); i != light_ids.end(); ++i ) 
	{
		LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( siege::database_guid(*i) );
		dwColor = ld.m_Color;
		return true;		
	}
	return false;
}


void EditorLights::SetColor( DWORD & dwColor )
{
	std::vector< DWORD > light_ids;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedLightIDs( light_ids );
	for ( i = light_ids.begin(); i != light_ids.end(); ++i ) 
	{
		LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( siege::database_guid(*i) );
		ld.m_Color = dwColor;
		gSiegeEngine.LightDatabase().SetLightDescriptor( siege::database_guid(*i), ld );		
	}

	gEditorRegion.SetRegionDirty();
}

void EditorLights::SetLightProperties(	vector_3 & vDir, float & iradius, float & oradius, float & intensity,
										bool & bOcclude, bool & bDrawShadows, bool & bActive, bool & bAffectActors,
										bool & bAffectItems, bool & bAffectTerrain, bool & bOnTimer )
{
	std::vector< DWORD > light_ids;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedLightIDs( light_ids );
	for ( i = light_ids.begin(); i != light_ids.end(); ++i ) 
	{
		LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( siege::database_guid(*i) );
		ld.m_InnerRadius		= iradius;
		ld.m_OuterRadius		= oradius;
		ld.m_Intensity			= intensity;
		ld.m_bOccludeGeometry	= bOcclude;
		ld.m_bDrawShadow		= bDrawShadows;
		ld.m_bActive			= bActive;
		ld.m_bAffectsActors		= bAffectActors;
		ld.m_bAffectsItems		= bAffectItems;
		ld.m_bAffectsTerrain	= bAffectTerrain;
		ld.m_bOnTimer			= bOnTimer;
		gSiegeEngine.LightDatabase().SetLightDescriptor( siege::database_guid(*i), ld );		

		if ( (ld.m_Type == LT_DIRECTIONAL) || (ld.m_Type == LT_SPOT) ) {
			SiegePos spos;
			spos.node = gEditorTerrain.GetTargetNode();
			spos.pos = vDir;
			gSiegeEngine.LightDatabase().SetLightDirection( siege::database_guid(*i), spos );
			gGizmoManager.GetGizmo( *i )->sdirection = spos;
		}
	}

	gEditorRegion.SetRegionDirty();
}


void EditorLights::GetLightProperties(	vector_3 & vDir, float & iradius, float & oradius, float & intensity,
										bool & bOcclude, bool & bDrawShadows, bool & bActive, bool & bAffectActors,
										bool & bAffectItems, bool & bAffectTerrain, bool & bOnTimer )
{
	std::vector< DWORD > light_ids;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedLightIDs( light_ids );
	for ( i = light_ids.begin(); i != light_ids.end(); ++i ) {
		LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( siege::database_guid(*i) );
		iradius			= ld.m_InnerRadius;
		oradius			= ld.m_OuterRadius;
		intensity		= ld.m_Intensity;
		bOcclude		= ld.m_bOccludeGeometry;
		bDrawShadows	= ld.m_bDrawShadow;
		bActive			= ld.m_bActive;		
		bAffectActors	= ld.m_bAffectsActors;
		bAffectItems	= ld.m_bAffectsItems;
		bAffectTerrain	= ld.m_bAffectsTerrain;
		bOnTimer		= ld.m_bOnTimer;

		if ( ld.m_Type != LT_POINT ) {
			// SiegePos spos = gSiegeEngine.LightDatabase().GetLightNodeSpaceDirection( siege::database_guid(*i) );
			SiegePos spos = gSiegeEngine.LightDatabase().GetLightNodeSpacePosition( siege::database_guid(*i) );
			
			SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( spos.node ) );
			SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
			const SiegeLightList lightList = node.GetStaticLights();
			SiegeLightListConstIter iLight = node.FindLightInList( siege::database_guid(*i), lightList );

			vDir = (*iLight).m_Direction;
		}
		return;
	}
}

void EditorLights::Cut()
{	
	Copy();
	m_bCutting = true;

	LightClipboard::iterator i;
	for ( i = m_clipboard.begin(); i != m_clipboard.end(); ++i )
	{	
		gGizmoManager.DeleteGizmo( (*i).source.GetValue() );
	}
}


void EditorLights::Copy()
{
	m_bCutting = false;
	m_clipboard.clear();
	std::vector< DWORD > objects;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedGizmosOfType( GIZMO_POINTLIGHT, objects );
	gGizmoManager.GetSelectedGizmosOfType( GIZMO_SPOTLIGHT, objects );

	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		LightClipboardObject co;
		co.spos			= gSiegeEngine.LightDatabase().GetLightNodeSpacePosition( siege::database_guid(*i) );
		co.descriptor	= gSiegeEngine.LightDatabase().GetLightDescriptor( siege::database_guid(*i) );		
		
		if ( co.descriptor.m_Type == LT_SPOT )
		{
			co.sdirection	= gSiegeEngine.LightDatabase().GetLightNodeSpaceDirection( siege::database_guid(*i) );
		}

		co.source = siege::database_guid(*i);
		
		m_clipboard.push_back( co );
	}
}


void EditorLights::Paste()
{
	if ( m_clipboard.size() == 0 ) 
	{
		return;
	}

	std::vector< vector_3 > offsets;
	{
		LightClipboard::iterator iLead = m_clipboard.begin();
		SiegePos spLead	= (*iLead).spos;
		{
			SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( spLead.node ) );
			SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
			spLead.pos = node.NodeToWorldSpace( spLead.pos );
		}		

		LightClipboard::iterator i;
		for ( i = m_clipboard.begin(); i != m_clipboard.end(); ++i ) 
		{
			SiegePos spos = (*i).spos;
			SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( spos.node ) );
			SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
			spos.pos = node.NodeToWorldSpace( spos.pos );						
			spos.pos.x -= spLead.pos.x;
			spos.pos.y -= spLead.pos.y;
			spos.pos.z -= spLead.pos.z;
			offsets.push_back( spos.pos );					
		}
	}

	SiegePos spLead;
	spLead = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
	
	std::vector< vector_3 >::iterator j = offsets.begin();
	LightClipboard::iterator i;
		
	for ( i = m_clipboard.begin(); i != m_clipboard.end(); ++i ) 
	{
		SiegePos spos = (*i).spos;
		spos.node = spLead.node;		
		spos.pos.x = spLead.pos.x;		
		spos.pos.z = spLead.pos.z;
		gGizmoManager.AccurateAdjustPositionToTerrain( spos );
		spos.pos.x += (*j).x;
		spos.pos.y += (*j).y;
		spos.pos.z += (*j).z;
		siege::database_guid sguid = AddLight( siege::database_guid(), spos, (*i).sdirection, (*i).descriptor );

		if ( sguid == UNDEFINED_GUID )
		{
			if ( MessageBoxEx( NULL, "A light could not be pasted on this terrain.  It is possible that the light's Y position offset on one node is so high that the light does not affect the node you are pasting to.  Would you like to try adding the light with the default Y offset?", "Paste Light", MB_YESNO | MB_ICONEXCLAMATION, LANG_ENGLISH ) == IDYES )
			{
				spos.pos.y = 0.0f;
				sguid = AddLight( siege::database_guid(), spos, (*i).sdirection, (*i).descriptor );
				if ( sguid == UNDEFINED_GUID )
				{
					MessageBoxEx( NULL, "The light could not be added, even at the default Y offset.  You may have to add the light manually.", "Paste Light", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
				}
			}
		}

		gGizmoManager.SelectGizmo( sguid.GetValue() );
		++j;
	}

	if ( m_bCutting )
	{
		m_bCutting = false;
	}	
}


void EditorLights::ClearClipboard()
{
	m_clipboard.clear();
}


void EditorLights::SaveLightToGas()
{	
	FuelHandle hLights;
	FuelHandle hLightsDir = gEditorRegion.GetRegionHandle()->GetChildBlock( "lights" );
	if ( hLightsDir.IsValid() == false ) 
	{
		return;
	}
		
	hLights = hLightsDir->GetChildBlock( "lights" );
	if ( hLights.IsValid() ) {
		hLightsDir->DestroyChildBlock( hLights );
	}		
	hLights = hLightsDir->CreateChildBlock( "lights", "lights.gas" );			
	
	SiegeEngine & engine = gSiegeEngine;	
	std::map< database_guid, LightConstruct >::iterator i;
	for (	i = engine.LightDatabase().GetLightDatabaseMap().begin(); 
			i != engine.LightDatabase().GetLightDatabaseMap().end();
			++i ) {
				
		LightDescriptor ld = engine.LightDatabase().GetLightDescriptor( (*i).first );

		if ( ld.m_Subtype == LS_DYNAMIC )
		{
			continue;
		}

		gpstring sName = "light_";
		sName.appendf( "0x%08X", (*i).first.GetValue() );
		FuelHandle hLight = hLights->CreateChildBlock( sName );
		
		switch ( ld.m_Type ) 
		{
		case LT_POINT:
			hLight->SetType( "point" );
			break;
		case LT_DIRECTIONAL:
			hLight->SetType( "directional" );
			break;
		case LT_SPOT:
			hLight->SetType( "spot" ); 
			break;
		}	
		
		hLight->Set( "color", (int)ld.m_Color, FVP_HEX_INT );
		hLight->Set( "active", ld.m_bActive );
		hLight->Set( "inner_radius", ld.m_InnerRadius );
		hLight->Set( "outer_radius", ld.m_OuterRadius );
		hLight->Set( "intensity", ld.m_Intensity );
		hLight->Set( "draw_shadow", ld.m_bDrawShadow );
		hLight->Set( "occlude_geometry", ld.m_bOccludeGeometry );
		hLight->Set( "affects_actors", ld.m_bAffectsActors );
		hLight->Set( "affects_items", ld.m_bAffectsItems );
		hLight->Set( "affects_terrain", ld.m_bAffectsTerrain );
		hLight->Set( "on_timer", ld.m_bOnTimer );
				
		SiegePos spos;				
		if (( ld.m_Type == LT_POINT ) || ( ld.m_Type == LT_SPOT )) {
			spos = engine.LightDatabase().GetLightNodeSpacePosition( (*i).first );
			
			FuelHandle hPos = hLight->CreateChildBlock( "position" );	
			hPos->Set( "x", spos.pos.x );
			hPos->Set( "y", spos.pos.y );
			hPos->Set( "z", spos.pos.z );
			hPos->Set( "node", spos.node.GetValue(), FVP_HEX_INT );
		}		
		if (( ld.m_Type == LT_DIRECTIONAL ) || ( ld.m_Type == LT_SPOT )) {
			spos = engine.LightDatabase().GetLightNodeSpaceDirection( (*i).first );			

			if ( ld.m_Type == LT_DIRECTIONAL )
			{
				spos = engine.LightDatabase().GetLightNodeSpacePosition( (*i).first );			
				SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( spos.node ) );
				SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
				const SiegeLightList lightList = node.GetStaticLights();
				SiegeLightListConstIter iLight = node.FindLightInList( (*i).first, lightList );
				spos.pos = (*iLight).m_Direction;
			}

			FuelHandle hDir = hLight->CreateChildBlock( "direction" );	
			hDir->Set( "x", spos.pos.x );
			hDir->Set( "y", spos.pos.y );
			hDir->Set( "z", spos.pos.z );
			hDir->Set( "node", spos.node.GetValue(), FVP_HEX_INT );			
		}							
	}
}



void EditorLights::LoadLightFromGas()
{
	// Clear out any existing lights
	SiegeEngine & engine = gSiegeEngine;	
	engine.LightDatabase().Clear();

	FuelHandle hLights;
	FuelHandle hLightsDir = gEditorRegion.GetRegionHandle()->GetChildBlock( "lights" );
	if ( hLightsDir.IsValid() == false ) 
	{
		return;
	}
		
	hLights = hLightsDir->GetChildBlock( "lights" );
	if ( hLights.IsValid() == false )
	{
		return;
	}

	FuelHandleList hlLights = hLights->ListChildBlocks( 1 );
	FuelHandleList::iterator i;
	for ( i = hlLights.begin(); i != hlLights.end(); ++i ) 
	{
		LightDescriptor ld;
		SiegePos spos;
		SiegePos sdir;
		
		gpstring sGUID = (*i)->GetName();
		int pos = sGUID.find( "light_" );
		if ( pos != gpstring::npos )
		{
			sGUID = sGUID.substr( sizeof( "light_" )-1, sGUID.size() );			
		}
		siege::database_guid lightGUID( sGUID.c_str() );
		gpstring sType = (*i)->GetType();
		if ( sType.same_no_case( "point" ) ) 
		{
			ld.m_Type = LT_POINT;
		}
		else if ( sType.same_no_case( "spot" ) ) 
		{
			ld.m_Type = LT_SPOT;
		}
		else if ( sType.same_no_case( "directional" ) ) 
		{
			ld.m_Type = LT_DIRECTIONAL;
		}

		ld.m_Subtype = LS_STATIC;	
		int color = 0;
		(*i)->Get( "color", color );
		ld.m_Color	= color;	
		(*i)->Get( "active", ld.m_bActive );
		(*i)->Get( "inner_radius", ld.m_InnerRadius );
		(*i)->Get( "outer_radius", ld.m_OuterRadius );
		(*i)->Get( "intensity", ld.m_Intensity );
		(*i)->Get( "draw_shadow", ld.m_bDrawShadow );
		(*i)->Get( "occlude_geometry", ld.m_bOccludeGeometry );	
		(*i)->Get( "affects_actors", ld.m_bAffectsActors );
		(*i)->Get( "affects_items", ld.m_bAffectsItems );
		(*i)->Get( "affects_terrain", ld.m_bAffectsTerrain );
		(*i)->Get( "on_timer", ld.m_bOnTimer );

		FuelHandle hPos = (*i)->GetChildBlock( "position" );
		if ( hPos.IsValid() ) 
		{
			hPos->Get( "x", spos.pos.x );
			hPos->Get( "y", spos.pos.y );
			hPos->Get( "z", spos.pos.z );

			int nodeID = 0;
			hPos->Get( "node", nodeID );
			spos.node = siege::database_guid( nodeID );
		}

		FuelHandle hDir = (*i)->GetChildBlock( "direction" );
		if ( hDir.IsValid() )
		{
			hDir->Get( "x", sdir.pos.x );
			hDir->Get( "y", sdir.pos.y );
			hDir->Get( "z", sdir.pos.z );
			
			int nodeID = 0;
			hDir->Get( "node", nodeID );
			sdir.node = siege::database_guid( nodeID );
		}

		AddLight( lightGUID, spos, sdir, ld );
	}
}


void EditorLights::GrabSelectedLightGuid()
{
	std::vector< DWORD > light_ids;
	gGizmoManager.GetSelectedLightIDs( light_ids );
	if ( light_ids.size() == 1 )
	{		
		gpstring sGuid;
		if ( !gGizmoManager.GetLeadGizmo() )
		{
			sGuid.assignf( "0x%08X", *(light_ids.begin()) );
		}
		else if ( *(light_ids.begin()) == gGizmoManager.GetLeadGizmo()->id )
		{			
			sGuid.assignf( "0x%08X", gGizmoManager.GetLeadGizmo()->id );
		}		
		else
		{
			return;
		}

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