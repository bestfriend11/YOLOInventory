//////////////////////////////////////////////////////////////////////////////
//
// File     :  GizmoManager.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "PrecompEditor.h"
#include "Preferences.h"
#include "namingkey.h"
#include "nema_aspectmgr.h"
#include "siege_light_structs.h"
#include "siege_light_database.h"
#include "rapi.h"
#include "siege_camera.h"
#include "GoGizmo.h"
#include "GoData.h"
#include "worldoptions.h"
#include "EditorObjects.h"
#include "winx.h"
#include "plane_3.h"
#include "space_3.h"
#include "line_segment_3.h"
#include "SiegeEditorView.h"
#include "EditorLights.h"
#include "CommandActions.h"
#include "GoInventory.h"
#include "MenuCommands.h"
#include "Object_Property_Sheet.h"
#include "EditorHotpoints.h"
#include "EditorGizmos.h"
#include "siege_decal.h"
#include "RapiPrimitive.h"
#include "EditorTerrain.h"
#include "resource.h"
#include "EditorTuning.h"
#include "Dialog_Sfx_Editor.h"
#include "Dialog_Portraits.h"
#include "EditorRegion.h"
#include "Dialog_Object_Grouping.h"
#include "WorldTerrain.h"
#include "GoEdit.h"
#include "EditorCamera.h"


// Namespaces
using namespace siege;

// Define
const float MAX_MOVE_TOLERANCE = 10000;


GizmoManager::GizmoManager()
	: m_godb_size( 0 )
	, m_pLeadGizmo( 0 )
	, m_precision_movement( 0.1f )
	, m_last_selected( -1 )
	, m_xMoveOffset( 0 )
	, m_yMoveOffset( 0 )
	, m_zMoveOffset( 0 )
	, m_pRolloverGizmo( 0 )
	, m_selectionIndex( 0 )
	, m_bSnapStartPos( false )
	, m_startPosId( 0 )
{
}


GizmoManager::~GizmoManager()
{
}


void GizmoManager::Deinit()
{
	/*
	GizmoVec::iterator j;	
	for ( j = m_gizmos.begin(); j != m_gizmos.end(); ++j ) 
	{
		if ( IsCustomType( (*j).type ) && (*j).hAspect.IsValid() )
		{
			(*j).hAspect->Purge(2);
			for ( int i = 0; i != (*j).hAspect->GetNumSubTextures(); ++i )
			{
				gDefaultRapi.DestroyTexture( (*j).hAspect->GetTexture( i ) );
			}
		}
	}
	*/
}


void GizmoManager::Update()
{	
	int temp = gGoDb.GetGlobalGoCount();
	if ( m_godb_size != gGoDb.GetGlobalGoCount() ) 
	{
		// Load in the loaded objects into the gizmo manager

		GoidColl objects;
		GoidColl::iterator i;
		gGoDb.GetAllGlobalGoids( objects );		
		for ( i = objects.begin(); i != objects.end(); ++i ) 
		{
			GoHandle hObject( *i );

			if ( !hObject->HasParent() || hObject->GetScid() != SCID_SPAWNED ) 
			{
				if ( !hObject->IsOmni() )
				{
					InsertGizmo( hObject );
				}
			}

			if (  hObject->HasParent() && hObject->GetParent()->HasInventory() && (hObject->GetParent()->GetInventory()->GetEquippedSlot(*i) == ES_CHEST) && hObject->HasAspect() ) 
			{
				Gizmo * pParent = GetGizmo( MakeInt( hObject->GetParent()->GetGoid() ) );				
				pParent->hAspect = hObject->GetAspect()->GetAspectHandle();
				pParent->hAspect->UpdateAnimationStateMachine( 0.0f );
			}	
			else 
			{
				Gizmo * pParent = GetGizmo( MakeInt(hObject->GetGoid()) );
				if ( pParent && hObject->HasAspect() && !hObject->HasGizmo() ) 
				{
					pParent->hAspect = hObject->GetAspect()->GetAspectHandle();
					pParent->hAspect->UpdateAnimationStateMachine( 0.0f );
				}
			}						
		}

		m_godb_size = gGoDb.GetGlobalGoCount();

		gPreferences.SetShowObjects( gPreferences.GetShowObjects() );
		gPreferences.SetShowGizmos( gPreferences.GetShowGizmos() );
	}
	

	GizmoVec::iterator j;
	m_selection_frustrum_gizmos.clear();
	for ( j = m_gizmos.begin(); j != m_gizmos.end(); ++j ) 
	{
		if ( IsTypeEditable( (*j).type ) ) 
		{

			if ( (*j).hAspect )
			{
				(*j).hAspect->ForceDeformation();
			}

			if ( (*j).type == GIZMO_OBJECTGIZMO || (*j).type == GIZMO_TUNINGPOINT )
			{
				GoHandle hObject( MakeGoid((*j).id) );
				if ( hObject.IsValid() && hObject->HasParty() )
				{
					(*j).spos = hObject->GetPlacement()->GetPosition();				
				}
			}

			if ( (*j).type == GIZMO_SPOTLIGHT )
			{
				if ( (*j).sdirection.node != (*j).spos.node )
				{
					(*j).sdirection.node = (*j).spos.node;
					gSiegeEngine.LightDatabase().SetLightDirection( siege::database_guid( (*j).id ), (*j).sdirection );										
				}				
			}

			if( gSiegeEngine.GetMouseShadow().UseSelectFrustum() ) 
			{
				vector_3	bcenter( DoNotInitialize );
				matrix_3x3	borient( DoNotInitialize );
				vector_3	bhalf_diag( DoNotInitialize );

				bool bSuccess = false;
				if ( !IsCustomType( (*j).type ) )
				{
					GoHandle hObject( MakeGoid((*j).id) );					
					if ( hObject.IsValid() && hObject->HasAspect() )
					{
						hObject->GetAspect()->GetWorldSpaceOrientedBoundingVolume( borient, bcenter, bhalf_diag );
						bSuccess = true;
					}
				}

				if ( !bSuccess )
				{
					GetWorldSpaceOrientedBoundingVolume( *j, bcenter, bhalf_diag, borient );
				}

				if( !gSiegeEngine.GetMouseShadow().SelectFrustum().CullObject( borient, bcenter, bhalf_diag ) ) 
				{
					m_selection_frustrum_gizmos.push_back( &(*j) );
				}
			}
		}
	}

	if ( m_bSnapStartPos )
	{
		gGizmoManager.AccurateAdjustPositionToTerrain( m_startPos );
		gSiegeEngine.AdjustPointToTerrain( m_startPos );
		gEditorGizmos.SetStartingPosition( m_startPosId, m_startPos );
		SetPosition( m_startPosId, m_startPos );
		m_bSnapStartPos = false;
	}
}


void GizmoManager::Init()
{
	GizmoModelInfo info;
	
	info.sModel		= "m_i_glb_object-pointlight";
	info.sTexture	= "b_i_glb_white";
	m_gizmo_model_info.insert(  GizmoModelInfoPair( GIZMO_POINTLIGHT, info ) );

	info.sModel		= "m_i_glb_object-dirlight";
	info.sTexture	= "b_i_glb_white";	
	m_gizmo_model_info.insert(  GizmoModelInfoPair( GIZMO_SPOTLIGHT, info ) );

	info.sModel		= "m_i_glb_object-decal";
	info.sTexture	= "b_i_glb_cyan";	
	m_gizmo_model_info.insert(  GizmoModelInfoPair( GIZMO_DECAL, info ) );

	info.sModel		= "m_i_glb_object-gogen";
	info.sTexture	= "b_i_glb_cyan";
	m_gizmo_model_info.insert(  GizmoModelInfoPair( GIZMO_HOTPOINT, info ) );

	info.sModel		= "m_i_glb_object-gogen";
	info.sTexture	= "b_i_glb_red";
	m_gizmo_model_info.insert(  GizmoModelInfoPair( GIZMO_STARTINGPOSITION, info ) );
}


void GizmoManager::InsertGizmo( eGizmoType type, SiegePos spos, DWORD id, nema::HAspect * phAspect )
{	
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).id == id ) 
		{
			return;
		}
	}

	Gizmo gizmo;
	gizmo.type		= type;
	gizmo.spos		= spos;
	gizmo.id		= id;	
	gizmo.bSelected	= false;
	gizmo.bVisible	= (IsCustomType( type ) || IsTypeVisible( type ));	

	gizmo.bNodeOverride = false;
	gizmo.index		= 0;

	if ( phAspect ) 
	{
		gizmo.hAspect	= *(phAspect);
		if ( gizmo.hAspect->IsPurged(1) )
		{
			// Just make sure its loaded --- biddle!
			gizmo.hAspect->Reconstitute( 0, true );
		}
	}

	switch ( gizmo.type ) 
	{
	case GIZMO_SPOTLIGHT:
		{			
			SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( spos.node ) );
			SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
			const SiegeLightList lightList = node.GetStaticLights();
			SiegeLightListConstIter iLight = node.FindLightInList( siege::database_guid( id ), lightList );
			gizmo.sdirection.pos = (*iLight).m_Direction;			
			gizmo.sdirection.node = spos.node;
		}
		break;
	case GIZMO_DIRHOTPOINT:
	case GIZMO_DIRLIGHT:
	case GIZMO_HOTPOINT:
	case GIZMO_POINTLIGHT:
	case GIZMO_DECAL:
	case GIZMO_STARTINGPOSITION:
		break;
	default:
		{
			m_godb_size++;		
		}
		break;
	}

	GizmoModelInfoMap::iterator iFound = m_gizmo_model_info.find( type );
	if ( iFound == m_gizmo_model_info.end() ) 
	{
		m_gizmos.push_back( gizmo );		
		return;
	}	
	
	gpstring sAspfile;
	gpstring sTexture;
	if ( gNamingKey.BuildASPLocation( (*iFound).second.sModel, sAspfile ) ) {
		gpstring sID;
		sID.assignf( "%d", id );
		gizmo.hAspect = gAspectStorage.LoadAspect( sAspfile.c_str(), sID.c_str() );				
		
		int texture = 0;

		if ( gNamingKey.BuildIMGLocation( (*iFound).second.sTexture, sTexture ) ) {
			texture = gSiegeEngine.Renderer().CreateTexture( sTexture.c_str(), false, 0, TEX_LOCKED );
		} 
		else {
			gpwarningf( ("Cannot locate bitmap content: %s\n", sTexture.c_str()) );
		}

		gizmo.hAspect->SetTextureCreatedByExternalSource( 0, texture );

		gDefaultRapi.DestroyTexture( texture );

		gizmo.hAspect->Reconstitute( 0, true );		
	}

	m_gizmos.push_back( gizmo );	

	gEditorRegion.SetRegionDirty();
}


void GizmoManager::InsertGizmo( Go * pGo )
{
	eGizmoType gizmo_type = GIZMO_OBJECT;
	MenuFunctions mf;

	if ( pGo->HasGizmo() )
	{
		if ( pGo->HasComponent( "dev_path_point" ) )
		{
			gizmo_type = GIZMO_TUNINGPOINT;
		}
		else
		{
			gizmo_type = GIZMO_OBJECTGIZMO;
		}
	}
		
	if ( pGo->HasGizmo() && IsTypeVisible( gizmo_type ) )
	{		
		pGo->GetEdit()->SetIsVisible( true );		
	}
	else if ( pGo->HasGizmo() )
	{
		pGo->GetEdit()->SetIsVisible( false );
	}
	
	gGizmoManager.InsertGizmo( gizmo_type, pGo->GetPlacement()->GetPosition(), MakeInt(pGo->GetGoid()) );	
}


void GizmoManager::DrawGizmo( Gizmo & gizmo )
{	
	if ( gizmo.hAspect.IsValid() && gizmo.hAspect && gizmo.hAspect->IsPurged(1) )
	{
		gizmo.hAspect->Reconstitute( 0, true );
	}

	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject(gizmo.spos.node) );
	SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
	gSiegeEngine.Renderer().SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );
	
	if ( gizmo.type != GIZMO_DECAL )
	{
		gSiegeEngine.Renderer().TranslateWorldMatrix( gizmo.spos.pos );	
	}
	else
	{
		SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( gizmo.index );		
		gSiegeEngine.Renderer().TranslateWorldMatrix( pDecal->GetDecalOrigin().pos );	
	}

	gSiegeEngine.Renderer().SetTextureStageState(	0,
													D3DTOP_SELECTARG1,
													D3DTOP_SELECTARG2,
													D3DTADDRESS_WRAP,
													D3DTADDRESS_WRAP,
													D3DTFG_POINT,
													D3DTFN_POINT,
													D3DTFP_POINT,
													D3DBLEND_SRCALPHA,
													D3DBLEND_INVSRCALPHA,
													false );

	DWORD colorop;
	gSiegeEngine.Renderer().GetDevice()->GetTextureStageState( 0, D3DTSS_COLORARG1, &colorop );	
		
	if ( IsCustomType( gizmo.type ) )
	{
		SiegeEngine & engine = gSiegeEngine;

		// Change aspect vertex color to color of light
		if (( gizmo.type == GIZMO_POINTLIGHT ) || ( gizmo.type == GIZMO_SPOTLIGHT ))
		{
			siege::LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( siege::database_guid(gizmo.id) );
			DWORD light_color = ld.m_Color;	
			
			engine.Renderer().GetDevice()->SetRenderState( D3DRENDERSTATE_TEXTUREFACTOR, light_color );
			engine.Renderer().GetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TFACTOR );			
		}
		
		if ( gizmo.type == GIZMO_SPOTLIGHT )
		{
			gSiegeEngine.Renderer().PushWorldMatrix();				
			gSiegeEngine.Renderer().RotateWorldMatrix( MatrixFromDirection( gizmo.sdirection.pos ) * Transpose( MatrixFromDirection( vector_3(0.0f, -1.0f, 0.000001f ) ) ) );
			gizmo.hAspect->Render( &gSiegeEngine.Renderer() );		
			gSiegeEngine.Renderer().PopWorldMatrix();
		}
		else if ( gizmo.type == GIZMO_DECAL )
		{
			SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( gizmo.index );
			gSiegeEngine.Renderer().PushWorldMatrix();				
			gSiegeEngine.Renderer().RotateWorldMatrix( pDecal->GetDecalOrientation() * Transpose( MatrixFromDirection( vector_3(0.0f, -1.0f, 0.000001f ) ) ) );
			gizmo.hAspect->InitializeLighting( 1.0f, 85 );
			gizmo.hAspect->Render( &gSiegeEngine.Renderer() );		
			gSiegeEngine.Renderer().PopWorldMatrix();
		}
		else
		{				
			gizmo.hAspect->Render( &gSiegeEngine.Renderer() );		
		}
		engine.Renderer().GetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	}	
	
	if ( gizmo.bSelected ) 
	{
		// Draw selection box if object is selected
		Quat		object_orientation	= Quat();
 		vector_3	object_origin;
		
		if ( gizmo.type != GIZMO_DECAL )
		{
			object_origin = gizmo.spos.pos;
		}
		else
		{
			SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( gizmo.index );
			object_origin = pDecal->GetDecalOrigin().pos;
		}

		vector_3	aspect_center;
		Quat		aspect_orient;
		vector_3	aspect_half_diag;
		float		scale = 1.0f;

		if ( !IsCustomType( gizmo.type ) ) 
		{
			GoHandle hObject( MakeGoid( gizmo.id ) );
			if ( hObject.IsValid() ) 
			{
				object_orientation = hObject->GetPlacement()->GetOrientation();
				if ( hObject->HasAspect() )
				{
					scale = hObject->GetAspect()->GetRenderScale();
				}
			}			
		}
		
		gSiegeEngine.Renderer().PushWorldMatrix();

		// Setup the object's matrix
		gSiegeEngine.Renderer().SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );

		if ( gizmo.bSelected && gizmo.type == GIZMO_DECAL )
		{
			SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( gizmo.index );
					
			RP_DrawFrustum( gSiegeEngine.Renderer(), pDecal->GetDecalOrigin().pos, pDecal->GetDecalOrientation(),
							pDecal->GetAngle(), pDecal->GetAspect(), pDecal->GetNearPlane() , pDecal->GetFarPlane() );
			
		}

		gSiegeEngine.Renderer().TranslateWorldMatrix( object_origin );

		if ( gizmo.type == GIZMO_SPOTLIGHT )
		{				
			gSiegeEngine.Renderer().RotateWorldMatrix( MatrixFromDirection( gizmo.sdirection.pos ) * Transpose( MatrixFromDirection( vector_3(0.0f, -1.0f, 0.0f ) ) ) );
		}
		else if ( gizmo.type == GIZMO_DECAL )
		{
			SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( gizmo.index );
			gSiegeEngine.Renderer().RotateWorldMatrix( pDecal->GetDecalOrientation() * Transpose( MatrixFromDirection( vector_3(0.0f, -1.0f, 0.0f ) ) ) );
		}
		else
		{
			gSiegeEngine.Renderer().RotateWorldMatrix( object_orientation.BuildMatrix() );
		}


		// Update the aspect and prepare for any rendering	
		if ( gizmo.hAspect.IsValid() ) 
		{		
			gizmo.hAspect->GetOrientedBoundingBox( aspect_orient, aspect_center, aspect_half_diag );
		}
		else if ( !IsCustomType( gizmo.type ) )
		{
			GoHandle hObject( MakeGoid( gizmo.id ) );
			if ( hObject.IsValid() )
			{
				if ( hObject->HasAspect() && !( !hObject->GetAspect()->GetIsVisible() && hObject->HasGizmo() ) && !hObject->GetAspect()->GetForceNoRender() )
				{
					hObject->GetAspect()->GetAspectHandle()->GetOrientedBoundingBox( aspect_orient, aspect_center, aspect_half_diag );
				}
				else if ( hObject->HasGizmo() )
				{
					hObject->GetGizmo()->GetAspectHandle()->GetOrientedBoundingBox( aspect_orient, aspect_center, aspect_half_diag );
				}
			}
		}

		gSiegeEngine.Renderer().TranslateWorldMatrix( aspect_center * scale );
		gSiegeEngine.Renderer().RotateWorldMatrix( aspect_orient.BuildMatrix() );
		DrawObjectBox( vector_3(), aspect_half_diag * scale, vector_3( 0.0, 1.0, 0.0), gSiegeEngine.Renderer() );										 			

		gSiegeEngine.Renderer().PopWorldMatrix();		
	}	

	gSiegeEngine.Renderer().GetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, colorop );	
}


void GizmoManager::DrawGizmos()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( IsTypeVisible( (*i).type ) ) 
		{
			DrawGizmo( *i );
		}		
	}

	if (( m_pRolloverGizmo ) && ( m_pRolloverGizmo->bSelected == false ) && ( IsTypeEditable( m_pRolloverGizmo->type )))
	{			
		DrawGizmoBox( *m_pRolloverGizmo, vector_3( 1.0, 0.0, 1.0 ) );
	}

	gEditorObjects.Draw();
	gEditorTuning.Draw();
	gEditorGizmos.Draw();
}


void GizmoManager::DrawGizmoBox( Gizmo & gizmo, vector_3 color )
{
	if ( !gizmo.bVisible )
	{
		return;
	}

	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject(gizmo.spos.node) );
	SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
	gSiegeEngine.Renderer().SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );
	gSiegeEngine.Renderer().TranslateWorldMatrix( gizmo.spos.pos );	

	gSiegeEngine.Renderer().SetTextureStageState(	0,
													D3DTOP_SELECTARG1,
													D3DTOP_SELECTARG2,
													D3DTADDRESS_WRAP,
													D3DTADDRESS_WRAP,
													D3DTFG_POINT,
													D3DTFN_POINT,
													D3DTFP_POINT,
													D3DBLEND_SRCALPHA,
													D3DBLEND_INVSRCALPHA,
													false );


	Quat		object_orientation	= Quat();
	vector_3	object_origin;

	if ( gizmo.type != GIZMO_DECAL )
	{
		object_origin = gizmo.spos.pos;
	}
	else
	{		
		SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( gizmo.index );
		if ( pDecal )
		{
			object_origin = pDecal->GetDecalOrigin().pos;
		}
	}
 	
	vector_3	aspect_center;
	Quat		aspect_orient;
	vector_3	aspect_half_diag;
	float		scale = 1.0f;

	if ( !IsCustomType( gizmo.type ) ) 
	{
		GoHandle hObject( MakeGoid( gizmo.id ) );
		if ( hObject.IsValid() ) 
		{
			object_orientation = hObject->GetPlacement()->GetOrientation();			
			if ( hObject->HasAspect() )
			{
				scale = hObject->GetAspect()->GetRenderScale();
			}
		}		
	}	
	
	gSiegeEngine.Renderer().PushWorldMatrix();

	// Setup the object's matrix
	gSiegeEngine.Renderer().SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );
	gSiegeEngine.Renderer().TranslateWorldMatrix( object_origin );

	if ( IsRotationType( gizmo.type ) )
	{			
		SiegePos spos = gSiegeEngine.LightDatabase().GetLightNodeSpaceDirection( siege::database_guid(gizmo.id) );
		gSiegeEngine.Renderer().RotateWorldMatrix( MatrixFromDirection( spos.pos ) * Transpose( MatrixFromDirection( vector_3(0.0f, -1.0f, 0.0f ) ) ) );
	}
	else if ( gizmo.type == GIZMO_DECAL )
	{
		SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( gizmo.index );
		gSiegeEngine.Renderer().RotateWorldMatrix( pDecal->GetDecalOrientation() * Transpose( MatrixFromDirection( vector_3(0.0f, -1.0f, 0.0f ) ) ) );		
	}
	else
	{
		gSiegeEngine.Renderer().RotateWorldMatrix( object_orientation.BuildMatrix() );
	}	

	// Update the aspect and prepare for any rendering	
	if ( gizmo.hAspect.IsValid() ) 
	{
		gizmo.hAspect->GetOrientedBoundingBox( aspect_orient, aspect_center, aspect_half_diag );
	}
	else if ( !IsCustomType( gizmo.type ) )
	{
		GoHandle hObject( MakeGoid( gizmo.id ) );
		if ( hObject.IsValid() ) 
		{
			if ( hObject->HasAspect() && !( !hObject->GetAspect()->GetIsVisible() && hObject->HasGizmo() ) && !hObject->GetAspect()->GetForceNoRender() )
			{
				hObject->GetAspect()->GetAspectHandle()->GetOrientedBoundingBox( aspect_orient, aspect_center, aspect_half_diag );;
			}
			else if ( hObject->HasGizmo() )
			{
				hObject->GetGizmo()->GetAspectHandle()->GetOrientedBoundingBox( aspect_orient, aspect_center, aspect_half_diag );;
			}
		}
	}

	gSiegeEngine.Renderer().TranslateWorldMatrix( aspect_center * scale );
	gSiegeEngine.Renderer().RotateWorldMatrix( aspect_orient.BuildMatrix() );
	DrawObjectBox( vector_3(), aspect_half_diag * scale, color, gSiegeEngine.Renderer() );										 			
	gSiegeEngine.Renderer().PopWorldMatrix();
}


bool GizmoManager::GizmoHitDetect( Gizmo & gizmo, bool bSelect )
{	
	bool bDetect = false;
	m_hitSet.clear();

	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( !IsTypeEditable( (*i).type ) || !(*i).bVisible ) 
		{	
			continue;
		}
	
		{				
			SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( (*i).spos.node ) );
			SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );

			// Get the object orientation, position, and scale
			float scale = 1.0f;
			matrix_3x3 object_orientation = node.GetTransposeOrientation();
			if ( !IsCustomType( (*i).type ) ) 
			{
				GoHandle hObject( MakeGoid((*i).id) );
				if ( hObject.IsValid() ) 
				{
					object_orientation = hObject->GetPlacement()->GetOrientation().BuildMatrix();
					if ( hObject->HasAspect() )
					{
						scale = hObject->GetAspect()->GetRenderScale();
					}
				}
			}

			vector_3 object_origin;
			
			if ( (*i).type != GIZMO_DECAL )
			{
				object_origin = (*i).spos.pos;		
			}
			else
			{
				SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( (*i).index );
				object_origin = pDecal->GetDecalOrigin().pos;
			}

			const matrix_3x3	inv_local_orientation		= Transpose( object_orientation );
 			const vector_3		inv_local_origin			= -object_origin;

			gSiegeEngine.Renderer().SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );
			gSiegeEngine.Renderer().PushWorldMatrix();
			
			// Setup the object's matrix
			gSiegeEngine.Renderer().TranslateWorldMatrix( object_origin );
			gSiegeEngine.Renderer().RotateWorldMatrix( object_orientation );

			// Get the mouse trace ray in local node space
			vector_3 ray_origin		= node.WorldToNodeSpace( gSiegeEngine.GetCamera().GetCameraPosition() );
			vector_3 ray_direction	= node.GetTransposeOrientation() * ( gSiegeEngine.GetCamera().GetMatrixOrientation() * gSiegeEngine.GetMouseShadow().CameraSpacePosition() );

			// Continue by moving the ray into local object space
			ray_origin				= inv_local_orientation * ( ray_origin + inv_local_origin );
			ray_direction			= inv_local_orientation * ray_direction;

			// Get the oriented information from the aspect
			vector_3 aspect_center;
			Quat	 aspect_orient;
			vector_3 aspect_half_diag;
		
			if ( (*i).hAspect.IsValid() ) 
			{
				(*i).hAspect->GetOrientedBoundingBox( aspect_orient, aspect_center, aspect_half_diag );
			}		
			else if ( !IsCustomType( (*i).type ) )
			{
				GoHandle hObject( MakeGoid( (*i).id ) );
				if ( hObject.IsValid() ) 
				{
					if ( hObject->HasAspect() && hObject->GetAspect()->GetIsVisible() && !hObject->GetAspect()->GetForceNoRender() )
					{
						hObject->GetAspect()->GetAspectHandle()->GetOrientedBoundingBox( aspect_orient, aspect_center, aspect_half_diag );;
					}
					else if ( hObject->HasGizmo() )
					{
						hObject->GetGizmo()->GetAspectHandle()->GetOrientedBoundingBox( aspect_orient, aspect_center, aspect_half_diag );;
					}
				}
			}
		
			// Do one final space conversion into local aspect space
			Quat inv_asp_orient	= aspect_orient.Inverse();
			vector_3 local_origin;
			inv_asp_orient.RotateVector( local_origin, ray_origin - aspect_center );

			vector_3 local_dir;
			inv_asp_orient.RotateVector( local_dir, ray_direction );

			gSiegeEngine.Renderer().PopWorldMatrix();
			
			vector_3 coord( DoNotInitialize );
			if( RayIntersectsBox( -aspect_half_diag, aspect_half_diag * scale, local_origin, local_dir, coord ) ) 
			{				
				if ( !gPreferences.GetSelectAll() )
				{	
					bool bCanSelect = true;
					GoHandle hObject( MakeGoid( (*i).id ) );
					if ( gPreferences.GetSelectNoninteractive() )
					{
						if ( hObject.IsValid() )
						{
							gpstring sName = hObject->GetDataTemplate()->GetMetaGroupName();
							if ( sName.same_no_case( "non_interactive" ) )
							{
								bCanSelect = false;
							}												
						}
					}
					if ( gPreferences.GetSelectInteractive() )
					{
						if ( hObject.IsValid() )
						{
							gpstring sName = hObject->GetDataTemplate()->GetMetaGroupName();
							if ( sName.same_no_case( "interactive" ) )
							{
								bCanSelect = false;
							}												
						}
					}
					if ( gPreferences.GetSelectGizmos() )
					{
						if ( !hObject.IsValid() )
						{
							bCanSelect = false; 
						}
						else if ( hObject.IsValid() && hObject->HasGizmo() )
						{
							bCanSelect = false;
						}
					}
					if ( gPreferences.GetSelectNonGizmos() )
					{
						if ( hObject.IsValid() && !hObject->HasGizmo() )
						{
							bCanSelect = false;
						}
					}

					if ( bCanSelect == false )
					{
						continue;
					}
				}	
				
				m_hitSet.insert( &(*i) );												
			}
		}
	}

	if ( m_hitSet.size() != 0 )
	{		
		if ( m_pRolloverGizmo )
		{
			GizmoSet::iterator iFound = m_hitSet.find( m_pRolloverGizmo );
			if ( iFound == m_hitSet.end() )
			{
				m_pRolloverGizmo = *(m_hitSet.begin());
			}
		}
		else
		{
			m_pRolloverGizmo = *(m_hitSet.begin());
		}

		gizmo.hAspect		= m_pRolloverGizmo->hAspect;
		gizmo.id			= m_pRolloverGizmo->id;
		gizmo.spos			= m_pRolloverGizmo->spos;
		gizmo.type			= m_pRolloverGizmo->type;
		gizmo.bSelected		= m_pRolloverGizmo->bSelected;
		gizmo.bNodeOverride = m_pRolloverGizmo->bNodeOverride;
						
		if ( bSelect ) 
		{
			gizmo.bSelected				= bSelect;
			m_pRolloverGizmo->bSelected	= bSelect;
		}

		return true;
	}
	else
	{
		m_pRolloverGizmo = 0;
		m_selectionIndex = 0;
	}
		
	return false;
}


void GizmoManager::RotateSelection()
{
	if ( m_hitSet.size() > 1 )
	{
		GizmoSet::iterator i; 
		++m_selectionIndex;
		if ( m_selectionIndex >= m_hitSet.size() )
		{
			m_selectionIndex = 0;
		}

		int index = 0;
		for ( i = m_hitSet.begin(); i != m_hitSet.end(); ++i )
		{
			if ( index == m_selectionIndex )
			{
				m_pRolloverGizmo = *i;
				break;
			}	
			++index;
		}
	}

	gEditorRegion.SetRegionDirty();
}


bool GizmoManager::IsLastSelectedHit()
{
	if ( m_last_selected == -1 ) 
	{
		return false;
	}

	Gizmo *pGizmo = GetGizmo( m_last_selected );
	if ( !pGizmo ) 
	{
		return false;
	}

	if ( !IsTypeEditable( pGizmo->type ) ) 
	{	
		return false;
	}
	
	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( pGizmo->spos.node ) );
	SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );

	// Get the object orientation, position, and scale
	matrix_3x3 object_orientation = node.GetTransposeOrientation();
	if ( !IsCustomType( pGizmo->type ) ) 
	{
		GoHandle hObject( MakeGoid(pGizmo->id) );

		if ( hObject.IsValid() ) 
		{
			object_orientation = hObject->GetPlacement()->GetOrientation().BuildMatrix();
		}		
	}
	vector_3 const& object_origin = pGizmo->spos.pos;		

	const matrix_3x3	inv_local_orientation		= Transpose( object_orientation);
 	const vector_3		inv_local_origin			= -object_origin;

	gSiegeEngine.Renderer().SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );
	gSiegeEngine.Renderer().PushWorldMatrix();
	
	// Setup the object's matrix
	gSiegeEngine.Renderer().TranslateWorldMatrix( object_origin );
	gSiegeEngine.Renderer().RotateWorldMatrix( object_orientation );

	// Get the mouse trace ray in local node space
	vector_3 ray_origin		= node.WorldToNodeSpace( gSiegeEngine.GetCamera().GetCameraPosition() );
	vector_3 ray_direction	= node.GetTransposeOrientation() * ( gSiegeEngine.GetCamera().GetMatrixOrientation() * gSiegeEngine.GetMouseShadow().CameraSpacePosition() );

	// Continue by moving the ray into local object space
	ray_origin				= inv_local_orientation * ( ray_origin + inv_local_origin );
	ray_direction			= inv_local_orientation * ray_direction;

	// Get the oriented information from the aspect
	vector_3 aspect_center;
	Quat	 aspect_orient;
	vector_3 aspect_half_diag;

	if ( pGizmo->hAspect.IsValid() ) 
	{
		pGizmo->hAspect->GetOrientedBoundingBox( aspect_orient, aspect_center, aspect_half_diag );
	}		
	else if ( !IsCustomType( pGizmo->type ) )
	{
		GoHandle hObject( MakeGoid( pGizmo->id ) );
		if ( hObject.IsValid() ) 
		{
			if ( hObject->HasAspect() && hObject->GetAspect()->GetIsVisible() && !hObject->GetAspect()->GetForceNoRender() )
			{
				hObject->GetAspect()->GetAspectHandle()->GetOrientedBoundingBox( aspect_orient, aspect_center, aspect_half_diag );
			}
			else if ( hObject->HasGizmo() )
			{
				hObject->GetGizmo()->GetAspectHandle()->GetOrientedBoundingBox( aspect_orient, aspect_center, aspect_half_diag );
			}
		}
	}

	// Do one final space conversion into local aspect space
	Quat inv_asp_orient	= aspect_orient.Inverse();
	vector_3 local_origin;
	inv_asp_orient.RotateVector( local_origin, ray_origin - aspect_center );
	vector_3 local_dir;
	inv_asp_orient.RotateVector( local_dir, ray_direction );
	
	gSiegeEngine.Renderer().PopWorldMatrix();

	vector_3 coord( DoNotInitialize );
	if( RayIntersectsBox( -aspect_half_diag, aspect_half_diag, local_origin, local_dir, coord ) ) 
	{
		return true;
	}
	return false;
}


void GizmoManager::BeginDragSelect()
{
	gSiegeEngine.GetMouseShadow().SetSelectionBox( true );	
}


void GizmoManager::EndDragSelect()
{
	if ( !gSiegeEngine.GetMouseShadow().SelectionBox() )
	{
		return;
	}

	if ( gPreferences.CanSelect() )
	{
		GizmoPtrVec::iterator i;
		for ( i = m_selection_frustrum_gizmos.begin(); i != m_selection_frustrum_gizmos.end(); ++i ) 
		{
			if ( (*i)->bVisible )
			{
				(*i)->bSelected = true;
			}
		}
	}

	if ( gEditorObjects.GetSequenceType() != SEQUENCE_LINK )
	{
		gPreferences.SetLinkMode( false );
	}	
	
	m_selection_frustrum_gizmos.clear();
	gSiegeEngine.GetMouseShadow().SetSelectionBox( false );
	if ( Object_Property_Sheet::DoesSingletonExist() )
	{
		gObjectProperties.Reset();
	}
}


void GizmoManager::GetWorldSpaceOrientedBoundingVolume( Gizmo & gizmo, vector_3 & center, vector_3 & half_diag, matrix_3x3 & orient )
{	
	nema::HAspect		hAspect;
	if ( !gizmo.hAspect.IsValid() && gizmo.type != GIZMO_OBJECTGIZMO && gizmo.type != GIZMO_TUNINGPOINT ) 
	{
		center		= vector_3();
		half_diag	= vector_3();
		orient		= matrix_3x3();
		return;
	}
	else if ( gizmo.type == GIZMO_OBJECTGIZMO || gizmo.type == GIZMO_TUNINGPOINT )
	{
		GoHandle hGizmo( MakeGoid(gizmo.id) );
		if ( hGizmo.IsValid() )
		{
			hAspect = hGizmo->GetGizmo()->GetAspectHandle();
		}
	}
	else
	{
		hAspect = gizmo.hAspect;
	}
	
	vector_3 aspect_center;
	Quat	 aspect_orient;
	vector_3 aspect_half_diag;

	hAspect->GetOrientedBoundingBox( aspect_orient, aspect_center, aspect_half_diag );
	
	// Get the node
	SiegeNodeHandle handle = gSiegeEngine.NodeCache().UseObject( gizmo.spos.node );
	SiegeNode const& node = handle.RequestObject( gSiegeEngine.NodeCache() );
	
	Quat quat;
	center = gizmo.spos.pos;
	vector_3 result;
	quat.RotateVector( result, aspect_center );
	center += result;
	center = node.NodeToWorldSpace( center );

	half_diag = aspect_half_diag;

	orient = node.GetCurrentOrientation() * ( matrix_3x3() * aspect_orient.BuildMatrix() );	
}

void GizmoManager::DeselectAll()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		(*i).bSelected = false;
	}
	if ( Object_Property_Sheet::DoesSingletonExist() )
	{
		gObjectProperties.Reset();
	}
}


void GizmoManager::CalculateMoveOffset( bool bShift )
{
	m_xMoveOffset = 0;
	m_yMoveOffset = 0;
	m_zMoveOffset = 0;
	if ( m_pLeadGizmo )
	{
		vector_3 moveVector;
		if ( bShift ) {
			moveVector = -gSiegeEngine.GetCamera().GetVectorOrientation();
			moveVector.y = 0.0f;
			moveVector.Normalize();		
		}
		else {
			moveVector = vector_3( 0, 1, 0 );
		}
		
		SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( GetLeadGizmo()->spos.node ) );
		SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
		
		float			d = -moveVector.DotProduct( node.NodeToWorldSpace(GetLeadGizmo()->spos.pos) );	
		plane_3			movePlane( moveVector, d );

		// Now let's find the direction of where the user clicked and generate a line segment from it so we can clip it to the plane	
		vector_3		moveDirection = ( gSiegeEngine.GetCamera().GetMatrixOrientation() * gSiegeEngine.GetMouseShadow().CameraSpacePosition() );
		line_segment_3	moveSegment( gSiegeEngine.GetCamera().GetCameraPosition(), moveDirection, 0, 1000 ); 
		ClipLineSegment( movePlane, moveSegment );

		m_xMoveOffset = moveSegment.GetMaximumEndpoint().x - node.NodeToWorldSpace( GetLeadGizmo()->spos.pos ).x;
		m_yMoveOffset = moveSegment.GetMaximumEndpoint().y - node.NodeToWorldSpace( GetLeadGizmo()->spos.pos ).y;
		m_zMoveOffset = moveSegment.GetMaximumEndpoint().z - node.NodeToWorldSpace( GetLeadGizmo()->spos.pos ).z;
	}
}


void GizmoManager::Move( bool bShift, bool /*bControl*/ )
{
	if ( !gPreferences.GetMovementMode() )
	{
		return;
	}

	vector_3 moveVector;
	if ( bShift ) 
	{
		moveVector = -gSiegeEngine.GetCamera().GetVectorOrientation();
		moveVector.y = 0.0f;
		moveVector.Normalize();		
	}
	else 
	{
		moveVector = vector_3( 0, 1, 0 );
	}
	
	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( GetLeadGizmo()->spos.node ) );
	SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
	
	float			d = -moveVector.DotProduct( node.NodeToWorldSpace(GetLeadGizmo()->spos.pos) );	
	plane_3			movePlane( moveVector, d );

	// Now let's find the direction of where the user clicked and generate a line segment from it so we can clip it to the plane	
	vector_3		moveDirection = ( gSiegeEngine.GetCamera().GetMatrixOrientation() * gSiegeEngine.GetMouseShadow().CameraSpacePosition() );
	line_segment_3	moveSegment( gSiegeEngine.GetCamera().GetCameraPosition(), moveDirection, 0, 1000 ); 
	ClipLineSegment( movePlane, moveSegment );

	float x_diff = 0;	
	float y_diff = 0;	
	float z_diff = 0;
	
	x_diff = moveSegment.GetMaximumEndpoint().x - node.NodeToWorldSpace( GetLeadGizmo()->spos.pos ).x;
	y_diff = moveSegment.GetMaximumEndpoint().y - node.NodeToWorldSpace( GetLeadGizmo()->spos.pos ).y;
	z_diff = moveSegment.GetMaximumEndpoint().z - node.NodeToWorldSpace( GetLeadGizmo()->spos.pos ).z;


	if (( fabsf(x_diff) > MAX_MOVE_TOLERANCE ) ||
		( fabsf(y_diff) > MAX_MOVE_TOLERANCE ) ||
		( fabsf(z_diff) > MAX_MOVE_TOLERANCE ))
	{
		return;
	}

	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if (( (*i).bSelected ) && IsTypeEditable( (*i).type ) && (*i).bVisible ) 
		{								
			// For undo
			{
				bool bLight = false;
				if (( (*i).type == GIZMO_POINTLIGHT ) || ( (*i).type == GIZMO_SPOTLIGHT ))
				{
					bLight = true;
				}			
				CommandMove * pCMove = new CommandMove( ACTION_MOVE, (*i).id, (*i).spos, bLight );
				gCommandManager.InsertAction( (CommandAction *)pCMove );
			}

			SiegeNodeHandle objHandle( gSiegeEngine.NodeCache().UseObject( (*i).spos.node ) );
			SiegeNode& objNode = objHandle.RequestObject( gSiegeEngine.NodeCache() );

			vector_3 worldPos;

			if ( bShift )
			{
				m_xMoveOffset = 0;
				m_zMoveOffset = 0;
			}
			else
			{
				m_yMoveOffset = 0;
			}

			switch ( gPreferences.GetMoveLockMode() )
			{
			case MOVE_LOCK_X:
				m_zMoveOffset = 0;
				z_diff = 0;
				m_yMoveOffset = 0;
				y_diff = 0;
				break;
			case MOVE_LOCK_Y:
				m_zMoveOffset = 0;
				z_diff = 0;
				m_xMoveOffset = 0;
				x_diff = 0;
				break;
			case MOVE_LOCK_Z:
				m_xMoveOffset = 0;
				x_diff = 0;
				m_yMoveOffset = 0;
				y_diff = 0;
				break;
			}

			worldPos.x = objNode.NodeToWorldSpace( (*i).spos.pos ).x + x_diff - m_xMoveOffset;			
			worldPos.y = objNode.NodeToWorldSpace( (*i).spos.pos ).y + y_diff - m_yMoveOffset;
			worldPos.z = objNode.NodeToWorldSpace( (*i).spos.pos ).z + z_diff - m_zMoveOffset;			
			(*i).spos.pos = objNode.WorldToNodeSpace( worldPos );		
		
			switch ( (*i).type ) {
			case GIZMO_POINTLIGHT:				
			case GIZMO_SPOTLIGHT:											
				{
					siege::database_guid oldGUID = (*i).spos.node;
					AccurateAdjustPositionToTerrain( (*i).spos );

					if ( (*i).type == GIZMO_SPOTLIGHT )
					{
						if ( (*i).spos.node != oldGUID ) 
						{
							SiegeNodeHandle new_handle( gSiegeEngine.NodeCache().UseObject( (*i).spos.node ) );
							SiegeNode& new_node = new_handle.RequestObject( gSiegeEngine.NodeCache() );
							
							matrix_3x3 visualOrient =	new_node.GetTransposeOrientation() * 
														( objNode.GetCurrentOrientation() * GetOrientation( (*i).id ).BuildMatrix() );
							SetOrientation( (*i).id, Quat( visualOrient ) );
							gSiegeEngine.LightDatabase().SetLightPositionAndDirection( siege::database_guid( (*i).id ), (*i).spos, (*i).sdirection );										
						}
					}
				}
				break;
			case GIZMO_DECAL:
				{
					siege::database_guid oldGUID = (*i).spos.node;
					AccurateAdjustPositionToTerrain( (*i).spos );
					int index = (*i).index;
					SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( (*i).index );
					siege::database_guid decalGuid = pDecal->GetGUID();
					gEditorGizmos.CreateDecalFromGizmo( (*i) );
					gEditorGizmos.DeleteDecal( index );
					pDecal = gSiegeDecalDatabase.GetDecalPointer( (*i).index );
					pDecal->SetGUID( decalGuid );

					if ( (*i).spos.node != oldGUID ) 
					{
						SiegeNodeHandle new_handle( gSiegeEngine.NodeCache().UseObject( (*i).spos.node ) );
						SiegeNode& new_node = new_handle.RequestObject( gSiegeEngine.NodeCache() );

						pDecal = gSiegeDecalDatabase.GetDecalPointer( (*i).index );
						matrix_3x3 visualOrient =	new_node.GetTransposeOrientation() * 
													( objNode.GetCurrentOrientation() * pDecal->GetDecalOrientation() );
						pDecal->SetDecalOrientation( visualOrient );
					}
				}
				break;
			case GIZMO_STARTINGPOSITION:
				{
					AccurateAdjustPositionToTerrain( (*i).spos );
				}
				break;
			case GIZMO_HOTPOINT:			
				{
					AccurateAdjustPositionToTerrain( (*i).spos );
				}
				break;					
			default:
				{
					GoHandle hObject( MakeGoid((*i).id) );
					if ( hObject.IsValid() ) 
					{	
						if ( hObject->HasAspect() && hObject->GetAspect()->GetDoesBlockPath() )
						{
							gWorldTerrain.RequestObjectUnblock( hObject->GetGoid() );
						}
						
						siege::database_guid oldGUID = (*i).spos.node;

						if ( !(*i).bNodeOverride )
						{
							AccurateAdjustPositionToTerrain( (*i).spos );
						}

						if ( gPreferences.GetSnapToGround() )
						{
							hObject->GetPlacement()->SetPosition( (*i).spos, gPreferences.GetSnapToGround() );												
						}
						else
						{
							hObject->GetPlacement()->ForceSetPosition( (*i).spos );												
						}
						
						if ( hObject->GetParent() && hObject->GetParent()->GetInventory() && ( hObject->GetParent()->GetInventory()->GetEquipped( ES_CHEST ) == hObject )) 
						{								
							if ( gPreferences.GetSnapToGround() )
							{
								hObject->GetParent()->GetPlacement()->SetPosition( (*i).spos, gPreferences.GetSnapToGround() );												
							}
							else
							{
								hObject->GetParent()->GetPlacement()->ForceSetPosition( (*i).spos );												
							}
						}
						else if ( hObject->HasInventory() ) 
						{
							Go * pArmor = hObject->GetInventory()->GetEquipped( ES_CHEST );							
							if ( pArmor ) 
							{
								if ( gPreferences.GetSnapToGround() )
								{
									pArmor->GetPlacement()->SetPosition( (*i).spos, gPreferences.GetSnapToGround() );												
								}
								else
								{
									pArmor->GetPlacement()->ForceSetPosition( (*i).spos );												
								}									
								RemoveGizmo( MakeInt(pArmor->GetGoid()) );
							}
						}
						
						if ( (*i).spos.node != oldGUID ) 
						{
							SiegeNodeHandle new_handle( gSiegeEngine.NodeCache().UseObject( (*i).spos.node ) );
							SiegeNode& new_node = new_handle.RequestObject( gSiegeEngine.NodeCache() );

							matrix_3x3 visualOrient =	new_node.GetTransposeOrientation() * 
														( objNode.GetCurrentOrientation() * hObject->GetPlacement()->GetOrientation().BuildMatrix() );
							hObject->GetPlacement()->SetOrientation( Quat(visualOrient) );								

							if ( hObject->GetParent() && hObject->GetParent()->HasInventory() && ( hObject->GetParent()->GetInventory()->GetEquipped( ES_CHEST ) == hObject )) 
							{
								hObject->GetParent()->GetPlacement()->SetOrientation( Quat(visualOrient) );
							}
							else if ( hObject->HasInventory() ) 
							{
								Go * pArmor = hObject->GetInventory()->GetEquipped( ES_CHEST );									
								if ( pArmor ) 
								{
									pArmor->GetPlacement()->SetOrientation( visualOrient );
								}
							}						
						}

						if ( hObject->HasComponent( "dev_path_point" ) )
						{
							gEditorTuning.SetPosition( hObject->GetGoid(), (*i).spos );						
						}

						if ( hObject->HasAspect() && hObject->GetAspect()->GetDoesBlockPath() )
						{
							gWorldTerrain.RequestObjectBlock( hObject->GetGoid() );
						}					
					}
				}
				break;
			}
		}
	}	

	gEditorRegion.SetRegionDirty();
}


void GizmoManager::PrecisionMove( PRECISION_DIRECTION direction )
{	
	if ( !gPreferences.GetMovementMode() )
	{
		return;
	}

	// For undo 
	CommandActions * pCActions = new CommandActions;

	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{			
		if (( (*i).bSelected ) && IsTypeEditable( (*i).type ) && (*i).bVisible ) 
		{			
			if ( gPreferences.GetObjectRotateMode() )
			{
				if ( !gGizmoManager.IsCustomType( (*i).type ) )
				{
					GoHandle hObject( MakeGoid( (*i).id ) );
					if ( hObject.IsValid() )
					{
						Quat orient = hObject->GetPlacement()->GetOrientation();						

						switch ( direction ) {
						case PRECISION_LEFT:
							orient.RotateY( DegreesToRadians( -gPreferences.GetRotationIncrement() ) );
							break;
						case PRECISION_RIGHT:
							orient.RotateY( DegreesToRadians( gPreferences.GetRotationIncrement() ) );
							break;
						case PRECISION_UP:
							orient.RotateX( DegreesToRadians( -gPreferences.GetRotationIncrement() ) );
							break;
						case PRECISION_DOWN:
							orient.RotateX( DegreesToRadians( gPreferences.GetRotationIncrement() ) );
							break;
						case PRECISION_FORWARD:
							orient.RotateZ( DegreesToRadians( -gPreferences.GetRotationIncrement() ) );
							break;
						case PRECISION_BACKWARD:
							orient.RotateZ( DegreesToRadians( gPreferences.GetRotationIncrement() ) );
							break;
						}
						
						hObject->GetPlacement()->SetOrientation( orient );
					}
				}
				else if ( (*i).type == GIZMO_SPOTLIGHT ) 
				{
					Quat orient( MatrixFromDirection( (*i).sdirection.pos ) );
					
					switch ( direction ) {
					case PRECISION_LEFT:
						orient.RotateY( DegreesToRadians( -gPreferences.GetRotationIncrement() ) );						
						break;
					case PRECISION_RIGHT:
						orient.RotateY( DegreesToRadians( gPreferences.GetRotationIncrement() ) );
						break;
					case PRECISION_UP:
						orient.RotateX( DegreesToRadians( -gPreferences.GetRotationIncrement() ) );
						break;
					case PRECISION_DOWN:
						orient.RotateX( DegreesToRadians( gPreferences.GetRotationIncrement() ) );						
						break;
					case PRECISION_FORWARD:
						orient.RotateZ( DegreesToRadians( -gPreferences.GetRotationIncrement() ) );
						break;
					case PRECISION_BACKWARD:
						orient.RotateZ( DegreesToRadians( gPreferences.GetRotationIncrement() ) );
						break;
					}

					orient.BuildMatrix().GetColumn2( (*i).sdirection.pos );
					gEditorLights.SetLightDirection( siege::database_guid((*i).id), (*i).sdirection );
				}
				else if ( (*i).type == GIZMO_DECAL )
				{					
					SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( (*i).index );		
					Quat orient( pDecal->GetDecalOrientation() );

					switch ( direction ) {
					case PRECISION_LEFT:
						orient.RotateY( DegreesToRadians( -gPreferences.GetRotationIncrement() ) );						
						break;
					case PRECISION_RIGHT:
						orient.RotateY( DegreesToRadians( gPreferences.GetRotationIncrement() ) );
						break;
					case PRECISION_UP:
						orient.RotateX( DegreesToRadians( -gPreferences.GetRotationIncrement() ) );
						break;
					case PRECISION_DOWN:
						orient.RotateX( DegreesToRadians( gPreferences.GetRotationIncrement() ) );						
						break;
					case PRECISION_FORWARD:
						orient.RotateZ( DegreesToRadians( -gPreferences.GetRotationIncrement() ) );
						break;
					case PRECISION_BACKWARD:
						orient.RotateZ( DegreesToRadians( gPreferences.GetRotationIncrement() ) );
						break;
					}					
					
					pDecal->SetDecalOrientation( orient.BuildMatrix() );
				}
			}
			else
			{
				// For undo
				{
					bool bLight = false;
					if (( (*i).type == GIZMO_POINTLIGHT ) || ( (*i).type == GIZMO_SPOTLIGHT ))
					{
						bLight = true;
					}			
					CommandMove * pCMove = new CommandMove( ACTION_MOVE, (*i).id, (*i).spos, bLight );
					pCActions->Insert( (CommandAction *)pCMove );
				}

				SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( (*i).spos.node ) );
				SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
				
				vector_3 worldPos;
				worldPos = node.NodeToWorldSpace( (*i).spos.pos );

				switch ( direction ) {
				case PRECISION_LEFT:
					worldPos.x -= m_precision_movement;				
					break;
				case PRECISION_RIGHT:
					worldPos.x += m_precision_movement;				
					break;
				case PRECISION_UP:
					worldPos.z += m_precision_movement;		
					break;
				case PRECISION_DOWN:
					worldPos.z -= m_precision_movement;											
					break;
				case PRECISION_FORWARD:
					worldPos.y += m_precision_movement;							
					break;
				case PRECISION_BACKWARD:
					worldPos.y -= m_precision_movement;							
					break;
				}
				
				(*i).spos.pos = node.WorldToNodeSpace( worldPos );
				
				switch ( (*i).type ) {
				case GIZMO_POINTLIGHT:
					break;
				case GIZMO_SPOTLIGHT:
					{	
						(*i).sdirection = gSiegeEngine.LightDatabase().GetLightNodeSpaceDirection( siege::database_guid( (*i).id ) );			

						siege::database_guid oldGUID = (*i).spos.node;
						AccurateAdjustPositionToTerrain( (*i).spos );

						if ( (*i).type == GIZMO_SPOTLIGHT )
						{
							if ( (*i).spos.node != oldGUID ) 
							{
								SiegeNodeHandle new_handle( gSiegeEngine.NodeCache().UseObject( (*i).spos.node ) );
								SiegeNode& new_node = new_handle.RequestObject( gSiegeEngine.NodeCache() );
								
								matrix_3x3 visualOrient =	new_node.GetTransposeOrientation() * 
															( node.GetCurrentOrientation() * GetOrientation( (*i).id ).BuildMatrix() );
								SetOrientation( (*i).id, Quat( visualOrient ) );
								gSiegeEngine.LightDatabase().SetLightPositionAndDirection( siege::database_guid( (*i).id ), (*i).spos, (*i).sdirection );										
							}
						}
					}
					break;
				case GIZMO_DECAL:
					{
						siege::database_guid oldGUID = (*i).spos.node;
						AccurateAdjustPositionToTerrain( (*i).spos );					
						int index = (*i).index;
						SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( (*i).index );
						siege::database_guid decalGuid = pDecal->GetGUID();
						gEditorGizmos.CreateDecalFromGizmo( (*i) );
						gEditorGizmos.DeleteDecal( index );
						pDecal = gSiegeDecalDatabase.GetDecalPointer( (*i).index );
						pDecal->SetGUID( decalGuid );

						if ( (*i).spos.node != oldGUID ) 
						{
							SiegeNodeHandle new_handle( gSiegeEngine.NodeCache().UseObject( (*i).spos.node ) );
							SiegeNode& new_node = new_handle.RequestObject( gSiegeEngine.NodeCache() );

							SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( (*i).index );
							matrix_3x3 visualOrient =	new_node.GetTransposeOrientation() * 
														( node.GetCurrentOrientation() * pDecal->GetDecalOrientation() );
							pDecal->SetDecalOrientation( visualOrient );
						}
					}
					break;
				case GIZMO_STARTINGPOSITION:
					{
						AccurateAdjustPositionToTerrain( (*i).spos );
					}
					break;
				case GIZMO_HOTPOINT:			
					{
						AccurateAdjustPositionToTerrain( (*i).spos );
					}
					break;
				default:
					{
						GoHandle hObject( MakeGoid((*i).id) );
						if ( hObject.IsValid() ) 
						{					
							siege::database_guid oldGUID = (*i).spos.node;

							AccurateAdjustPositionToTerrain( (*i).spos );

							if ( gPreferences.GetSnapToGround() )
							{
								hObject->GetPlacement()->SetPosition( (*i).spos, gPreferences.GetSnapToGround() );												
							}
							else
							{
								hObject->GetPlacement()->ForceSetPosition( (*i).spos );												
							}						

							if ( hObject->GetParent() && hObject->GetParent()->HasInventory() && ( hObject->GetParent()->GetInventory()->GetEquipped( ES_CHEST ) == hObject )) {
								
								if ( gPreferences.GetSnapToGround() )
								{
									hObject->GetParent()->GetPlacement()->SetPosition( (*i).spos, gPreferences.GetSnapToGround() );												
								}
								else
								{
									hObject->GetParent()->GetPlacement()->ForceSetPosition( (*i).spos );												
								}														
							}
							else if ( hObject->HasInventory() ) 
							{
								Go * pArmor = hObject->GetInventory()->GetEquipped( ES_CHEST );							
								if ( pArmor ) 
								{
									if ( gPreferences.GetSnapToGround() )
									{
										pArmor->GetPlacement()->SetPosition( (*i).spos, gPreferences.GetSnapToGround() );												
									}
									else
									{
										pArmor->GetPlacement()->ForceSetPosition( (*i).spos );												
									}

									RemoveGizmo( MakeInt(pArmor->GetGoid()) );
								}
							}

							if ( (*i).spos.node != oldGUID ) 
							{
								SiegeNodeHandle new_handle( gSiegeEngine.NodeCache().UseObject( (*i).spos.node ) );
								SiegeNode& new_node = new_handle.RequestObject( gSiegeEngine.NodeCache() );						

								matrix_3x3 visualOrient =	new_node.GetTransposeOrientation() * 
															( node.GetCurrentOrientation() * hObject->GetPlacement()->GetOrientation().BuildMatrix() );
								hObject->GetPlacement()->SetOrientation( Quat(visualOrient) );								

								if ( hObject->GetParent() && hObject->GetParent()->HasInventory() && ( hObject->GetParent()->GetInventory()->GetEquipped( ES_CHEST ) == hObject )) 
								{
									hObject->GetParent()->GetPlacement()->SetOrientation( visualOrient );
								}
								else if ( hObject->HasInventory() ) 
								{
									Go * pArmor = hObject->GetInventory()->GetEquipped( ES_CHEST );									
									if ( pArmor ) 
									{
										pArmor->GetPlacement()->SetOrientation( visualOrient );
									}
								}							
							}
						}
					}
					break;
				}
			}
		}
	}

	
	gCommandManager.Insert( pCActions );
	gEditorRegion.SetRegionDirty();
}


void GizmoManager::MoveUpdate()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) {			
		if (( (*i).bSelected ) && IsTypeEditable( (*i).type ) && (*i).bVisible ) {			
			switch ( (*i).type ) {
			case GIZMO_POINTLIGHT:
				{					
					gEditorLights.SetLightPosition( siege::database_guid( (*i).id ), (*i).spos );
				}
				break;
			case GIZMO_SPOTLIGHT:
				{	
					gSiegeEngine.LightDatabase().SetLightPositionAndDirection( siege::database_guid( (*i).id ), (*i).spos, (*i).sdirection );										
				}
				break;				
			case GIZMO_STARTINGPOSITION:
				{
					gEditorGizmos.SetStartingPosition( (*i).id, (*i).spos );
				}
				break;
			}			
		}
	}

	gEditorRegion.SetRegionDirty();
}


bool GizmoManager::AccurateAdjustPositionToTerrain( SiegePos & spos, bool bAdjustToAny )
{	
	// Get the object
	vector_3 worldSpacePosition			= spos.WorldPos();

	// Get the engine
	siege::SiegeEngine& engine			= gSiegeEngine;

	
	float currentLength		= FLOAT_MAX;
	SiegePos currentPos		= spos;

	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator i;

	bool bHit = false;

	for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
	{
		if ( (*i)->GetVisibleThisFrame() ) 
		{
			vector_3 node_ray_origin		= (*i)->WorldToNodeSpace( worldSpacePosition );
			node_ray_origin.y				= (*i)->GetMinimumBounds().y - 0.01;

			float ray_t						= FLOAT_MAX;
			vector_3 normal( DoNotInitialize );
			if( (*i)->HitTest( node_ray_origin, vector_3::UP, ray_t, normal, bAdjustToAny ? LF_IS_ANY : LF_IS_FLOOR ) )
			{
				SiegePos newPos( node_ray_origin + (vector_3::UP * ray_t), (*i)->GetGUID() );
				float diffLength	= engine.GetDifferenceVector( spos, newPos ).Length();
				if( diffLength < currentLength )
				{
					currentPos		= newPos;
					currentLength	= diffLength;
				}

				bHit = true;
			}
		}
	}

	if ( currentPos.node != UNDEFINED_GUID ) 
	{
		vector_3 oldCenter;
		vector_3 newCenter;
		if ( spos.node != currentPos.node ) 
		{
			SiegeNodeHandle old_handle( gSiegeEngine.NodeCache().UseObject( spos.node ) );
			SiegeNode& old_node = old_handle.RequestObject( gSiegeEngine.NodeCache() );
			oldCenter = old_node.GetCurrentCenter();				

			SiegeNodeHandle new_handle( gSiegeEngine.NodeCache().UseObject( currentPos.node ) );
			SiegeNode& new_node = new_handle.RequestObject( gSiegeEngine.NodeCache() );
			newCenter = new_node.GetCurrentCenter();				
		}

		float y = spos.pos.y;
		spos = currentPos;
		
		if ( !gPreferences.GetSnapToGround() ) 
		{
			spos.pos.y = y + ( oldCenter.y - newCenter.y );
		}		
	}	
	
	return bHit;
}


void GizmoManager::HitDetectLeadGizmo()
{	
	if ( m_pRolloverGizmo && IsTypeEditable( m_pRolloverGizmo->type ) && m_pRolloverGizmo->bVisible && m_pRolloverGizmo->bSelected ) 
	{
		m_pLeadGizmo = m_pRolloverGizmo;
		return;
	}
	m_pLeadGizmo = 0;
}


void GizmoManager::CursorRollover()
{
	GRect rect = winx::GetWindowRect( gEditor.GetHWnd() );
	POINT pt;
	GetCursorPos( &pt );
	GRect curRect;
	curRect.top		= pt.y;
	curRect.left	= pt.x;
	curRect.bottom	= pt.y;
	curRect.right	= pt.x;
	if ( rect.Contains( curRect ) == false ) 
	{
		return;
	}
	
	Gizmo gizmo;
	HCURSOR hCursor;
	if ( GizmoHitDetect( gizmo, false ) &&  gAppModule.GetMainWnd() == GetFocus() ) 
	{
		if ( gPreferences.GetLinkMode() )
		{
			hCursor = AfxGetApp()->LoadCursor( MAKEINTRESOURCE(IDC_CURSOR_LINK) );
		}
		else
		{			
			switch ( gizmo.type ) {
			case GIZMO_STARTINGPOSITION:
				hCursor = AfxGetApp()->LoadCursor( MAKEINTRESOURCE(IDC_POINTER_SP) );
				break;
			case GIZMO_DECAL:
				hCursor = AfxGetApp()->LoadCursor( MAKEINTRESOURCE(IDC_POINTER_DECAL) );
				break;
			case GIZMO_HOTPOINT:
				hCursor = AfxGetApp()->LoadCursor( MAKEINTRESOURCE(IDC_POINTER_HP) );
				break;
			case GIZMO_POINTLIGHT:
			case GIZMO_SPOTLIGHT:
				hCursor = AfxGetApp()->LoadCursor( MAKEINTRESOURCE(IDC_POINTER_LIGHT) );
				break;
			case GIZMO_OBJECTGIZMO:
			case GIZMO_TUNINGPOINT:
				hCursor = AfxGetApp()->LoadCursor( MAKEINTRESOURCE(IDC_POINTER_GOGEN) );
				break;
			case GIZMO_OBJECT:
				hCursor = AfxGetApp()->LoadCursor( MAKEINTRESOURCE(IDC_POINTER_GO) );
				break;
			}			
		}
	}
	else 
	{
		hCursor = AfxGetApp()->LoadStandardCursor( IDC_ARROW );
	}
	
	HCURSOR hCurrentCur = GetCursor();
	if ( hCurrentCur != hCursor ) 
	{
		SetCursor( hCursor );
	}
}


void GizmoManager::DrawObjectBox( vector_3 c, vector_3 d, vector_3 colorv, Rapi& renderer ) 
{	
	vector_3 p0 = c + vector_3( d.GetX(), d.GetY(),-d.GetZ());
	vector_3 p1 = c + vector_3(-d.GetX(), d.GetY(),-d.GetZ());
	vector_3 p2 = c + vector_3(-d.GetX(),-d.GetY(),-d.GetZ());
	vector_3 p3 = c + vector_3( d.GetX(),-d.GetY(),-d.GetZ());
	vector_3 p4 = c + vector_3( d.GetX(), d.GetY(), d.GetZ());
	vector_3 p5 = c + vector_3(-d.GetX(), d.GetY(), d.GetZ());
	vector_3 p6 = c + vector_3(-d.GetX(),-d.GetY(), d.GetZ());
	vector_3 p7 = c + vector_3( d.GetX(),-d.GetY(), d.GetZ());

	renderer.SetTextureStageState(	0,
									D3DTOP_DISABLE,
									D3DTOP_SELECTARG2,
									D3DTADDRESS_WRAP,
									D3DTADDRESS_WRAP,
									D3DTFG_POINT,
									D3DTFN_POINT,
									D3DTFP_POINT,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );

	sVertex verts[2];
	memset( verts, 0, sizeof( sVertex ) * 2 );

	verts[0].color	= MAKEDWORDCOLOR( colorv );
	verts[1].color	= verts[0].color;

	memcpy( &verts[0], &p0, sizeof( float ) * 3 );
	memcpy( &verts[1], &p1, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, 0, 0 );
	memcpy( &verts[0], &p2, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, 0, 0 );
	memcpy( &verts[1], &p3, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, 0, 0 );
	memcpy( &verts[0], &p0, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, 0, 0 );

	memcpy( &verts[0], &p4, sizeof( float ) * 3 );
	memcpy( &verts[1], &p5, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, 0, 0 );
	memcpy( &verts[0], &p6, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, 0, 0 );
	memcpy( &verts[1], &p7, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, 0, 0 );
	memcpy( &verts[0], &p4, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, 0, 0 );

	memcpy( &verts[0], &p0, sizeof( float ) * 3 );
	memcpy( &verts[1], &p4, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, 0, 0 );
	memcpy( &verts[0], &p1, sizeof( float ) * 3 );
	memcpy( &verts[1], &p5, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, 0, 0 );
	memcpy( &verts[0], &p2, sizeof( float ) * 3 );
	memcpy( &verts[1], &p6, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, 0, 0 );
	memcpy( &verts[0], &p3, sizeof( float ) * 3 );
	memcpy( &verts[1], &p7, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, 0, 0 );
}


int GizmoManager::GetNumGizmosSelected()
{
	int num_gizmos = 0;
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) {
		if ( (*i).bSelected ) {
			num_gizmos++;
		}
	}
	return num_gizmos;		
}


int GizmoManager::GetNumObjectsSelected()
{
	int num_gizmos = 0;
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && !IsCustomType( (*i).type ) ) 
		{
			num_gizmos++;
		}
	}
	return num_gizmos;		
}


void GizmoManager::GetSelectedGizmoIDs( std::vector< DWORD > & gizmo_ids )
{	
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{		
		if ( (*i).bSelected && (*i).bVisible ) 
		{
			gizmo_ids.push_back( (*i).id );
		}
	}
}


void GizmoManager::GetSelectedObjectIDs( std::vector< DWORD > & object_ids )
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bVisible )
		{
			if ( (*i).bSelected && !IsCustomType( (*i).type ) ) 			
			{
				object_ids.push_back( (*i).id );
			}
		}
	}
}


bool GizmoManager::DeleteGizmo( DWORD id, bool bForce )
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) {
		if ( (*i).id == id ) {

			switch ( (*i).type ) {
			case GIZMO_DECAL:
				{
					if ( gPreferences.GetModeDecals() || bForce )
					{
						if ( (*i).hAspect.IsValid() )
						{
							(*i).hAspect->Purge(2);
						}
						gEditorGizmos.DeleteDecal( (*i).index );						
						m_gizmos.erase( i );
					}
				}
				break;
			case GIZMO_POINTLIGHT:
			case GIZMO_SPOTLIGHT:
			case GIZMO_DIRLIGHT:
				{
					if ( gPreferences.GetModeLights() || bForce )
					{
						if ( (*i).hAspect.IsValid() )
						{
							(*i).hAspect->Purge(2);
						}
						gEditorLights.DeleteLight( siege::database_guid(id) );
						m_gizmos.erase( i );
					}
				}
				break;
			case GIZMO_HOTPOINT:
				{
					if ( gPreferences.GetModeGizmos() || bForce )
					{
						if ( (*i).hAspect.IsValid() )
						{
							(*i).hAspect->Purge(2);
						}
						gEditorHotpoints.Delete( (*i).id );					
					}
				}
				break;
			case GIZMO_STARTINGPOSITION:
				{
					if ( gPreferences.GetModeGizmos() || bForce )
					{
						if ( (*i).hAspect.IsValid() )
						{
							(*i).hAspect->Purge(2);
						}
						gEditorGizmos.DeleteStartingPosition( (*i).id );
					}
				}
				break;		
			case GIZMO_OBJECTGIZMO:
			case GIZMO_TUNINGPOINT:
				{
					if ( gPreferences.GetModeGizmos() || gPreferences.GetModeTuningPoints() || bForce )
					{
						gEditorObjects.DeleteObject( MakeGoid(id) );
					}
				}
				break;
			default:
				{					
					if ( gPreferences.GetModeObjects() || bForce )
					{
						gEditorObjects.DeleteObject( MakeGoid(id) );
					}
				}
				break;		
			}
			
			return true;
		}
	}

	return false;
}


void GizmoManager::DeleteSelectedGizmos()
{
	std::vector< DWORD > gizmo_ids;
	GetSelectedGizmoIDs( gizmo_ids );
	std::vector< DWORD >::iterator i;
	for ( i = gizmo_ids.begin(); i != gizmo_ids.end(); ++i ) {
		DeleteGizmo( *i );
	}
}


bool GizmoManager::IsTypeVisible( eGizmoType type ) 
{
	switch ( type ) 
	{
	case GIZMO_POINTLIGHT:
		return gPreferences.GetShowLights();					
	case GIZMO_SPOTLIGHT:
		return gPreferences.GetShowLights();
	case GIZMO_OBJECTGIZMO:
	case GIZMO_STARTINGPOSITION:
	case GIZMO_HOTPOINT:
		return gPreferences.GetShowGizmos();
	case GIZMO_OBJECT:	
		return gPreferences.GetShowObjects();	
	case GIZMO_DECAL:		
		return gPreferences.GetShowGizmos();
	case GIZMO_TUNINGPOINT:
		return gPreferences.GetShowTuningPoints();
	case GIZMO_DIRHOTPOINT:
	case GIZMO_DIRLIGHT:
		// You should never "see" directional lights in the gizmo manager
		return false;
	}

	return false;
}


bool GizmoManager::IsTypeEditable( eGizmoType type )
{
	if ( IsTypeVisible( type ) )
	{
		switch ( type ) {
		case GIZMO_POINTLIGHT:
			return gPreferences.GetModeLights();					
		case GIZMO_SPOTLIGHT:
			return gPreferences.GetModeLights();
		case GIZMO_OBJECTGIZMO:
		case GIZMO_HOTPOINT:
		case GIZMO_STARTINGPOSITION:		
			return gPreferences.GetModeGizmos();
		case GIZMO_OBJECT:
			return gPreferences.GetModeObjects();
		case GIZMO_TUNINGPOINT:
			return gPreferences.GetModeTuningPoints();
		case GIZMO_DECAL:
			return gPreferences.GetModeDecals();
		case GIZMO_DIRHOTPOINT:
		case GIZMO_DIRLIGHT:
			// You should never "see" directional lights in the gizmo manager
			return false;
		}
	}
	return false;
}


void GizmoManager::SetOrientation( DWORD id, Quat orient )
{
	Gizmo * pGizmo = GetGizmo( id );
	if ( pGizmo )
	{
		switch ( pGizmo->type ) 
		{
		case GIZMO_TUNINGPOINT:
		case GIZMO_OBJECTGIZMO:
		case GIZMO_OBJECT:
			{
				GoHandle hObject( MakeGoid(pGizmo->id) );
				if ( hObject.IsValid() && hObject->HasPlacement() ) 
				{
					SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( hObject->GetPlacement()->GetPosition().node ) );
					SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
					hObject->GetPlacement()->SetOrientation( orient.BuildMatrix() );
				}		
			}
			break;
		case GIZMO_SPOTLIGHT:
			{
				orient.BuildMatrix().GetColumn2( pGizmo->sdirection.pos );
				gEditorLights.SetLightDirection( siege::database_guid(id), pGizmo->sdirection );
			}
			break;
		}
	}

	gEditorRegion.SetRegionDirty();
}


Quat GizmoManager::GetOrientation( DWORD id )
{
	Gizmo * pGizmo = GetGizmo( id );
	if ( pGizmo )
	{
		switch ( pGizmo->type ) 
		{
		case GIZMO_TUNINGPOINT:
		case GIZMO_OBJECTGIZMO:
		case GIZMO_OBJECT:
			{
				GoHandle hObject( MakeGoid(id) );
				if ( hObject.IsValid() && hObject->HasPlacement() ) 
				{
					return hObject->GetPlacement()->GetOrientation();
				}
			}
			break;
		case GIZMO_SPOTLIGHT:
			{
				return Quat( MatrixFromDirection( pGizmo->sdirection.pos ) );
			}
			break;
		}
	}

	return Quat();
}


void GizmoManager::SetOrientation( matrix_3x3 orientation )
{
	if ( !GetLeadGizmo() ) 
	{
		GizmoVec::iterator i;
		for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
		{
			if ( (*i).bSelected && (*i).bVisible ) 
			{
				SetLeadGizmo( &(*i) );						
			}
		}
	}

	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && (*i).bVisible ) 
		{
			switch ( (*i).type ) 
			{			
			case GIZMO_SPOTLIGHT:
				{
					orientation.GetColumn2( (*i).sdirection.pos );
					gEditorLights.SetLightDirection( siege::database_guid((*i).id), (*i).sdirection );
				}
				break;
			}		
		}
	}

	gEditorRegion.SetRegionDirty();
}


void GizmoManager::SetOrientation( Quat orientation )
{
	if ( !GetLeadGizmo() ) 
	{
		GizmoVec::iterator i;
		for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
		{
			if ( (*i).bSelected && (*i).bVisible ) 
			{
				SetLeadGizmo( &(*i) );						
			}
		}
	}

	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && (*i).bVisible ) 
		{
			switch ( (*i).type ) 
			{
			case GIZMO_TUNINGPOINT:
			case GIZMO_OBJECTGIZMO:
			case GIZMO_OBJECT:
				{
					GoHandle hObject( MakeGoid((*i).id) );
					if ( hObject.IsValid() && hObject->HasPlacement() ) {

						SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( hObject->GetPlacement()->GetPosition().node ) );
						SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
						hObject->GetPlacement()->SetOrientation( orientation );
					}		
				}
				break;
			}			
		}
	}

	gEditorRegion.SetRegionDirty();
}


matrix_3x3 GizmoManager::GetOrientation()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && (*i).bVisible ) 
		{
			switch ( (*i).type ) 
			{
			case GIZMO_TUNINGPOINT:
			case GIZMO_OBJECTGIZMO:
			case GIZMO_OBJECT:
				{
					GoHandle hObject( MakeGoid((*i).id) );
					if ( hObject.IsValid() && hObject->HasPlacement() ) 
					{
						SetLeadGizmo( &(*i) );
						return (matrix_3x3)hObject->GetPlacement()->GetOrientation().BuildMatrix();
					}		
				}
				break;		
			case GIZMO_SPOTLIGHT:
				{
					return MatrixFromDirection( (*i).sdirection.pos );
				}
				break;
			}		
		}
	}
	return matrix_3x3();
}


Quat GizmoManager::GetQuatOrientation()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && (*i).bVisible ) 
		{
			switch ( (*i).type ) 
			{
			case GIZMO_TUNINGPOINT:
			case GIZMO_OBJECTGIZMO:
			case GIZMO_OBJECT:
				{
					GoHandle hObject( MakeGoid((*i).id) );
					if ( hObject.IsValid() && hObject->HasPlacement() ) 
					{
						SetLeadGizmo( &(*i) );
						return hObject->GetPlacement()->GetOrientation();
					}		
				}
				break;				
			}		
		}
	}
	return Quat();

}

Goid GizmoManager::GetFirstObjectWithInventory()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && (*i).bVisible ) 
		{
			if ( (*i).type == GIZMO_OBJECT ) 
			{
				GoHandle hObject( MakeGoid((*i).id) );
				if ( hObject.IsValid() && hObject->HasInventory() )
				{
					return hObject->GetGoid();
				}
			}
		}
	}
	return GOID_INVALID;
}



void GizmoManager::SetPosition( float x, float y, float z )
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) {
		if ( (*i).bSelected && (*i).bVisible ) {
			if ( !IsCustomType( (*i).type ) )
			{
				GoHandle hObject( MakeGoid((*i).id) );
				if ( hObject.IsValid() ) 
				{
					SiegePos spos;
					spos.node = hObject->GetPlacement()->GetPosition().node;
					spos.pos = vector_3( x, y, z );
					hObject->GetPlacement()->SetPosition( spos );						
					(*i).spos.pos = vector_3( x, y, z );
				}				
			}
		}
	}

	gEditorRegion.SetRegionDirty();
}
void GizmoManager::SetPosition( SiegePos spos )
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) {
		if ( (*i).bSelected && (*i).bVisible ) {
			SetPosition( (*i).id, spos );		
		}
	}

	gEditorRegion.SetRegionDirty();
}
void GizmoManager::SetPosition( int id, SiegePos spos )
{
	Gizmo * pGizmo = GetGizmo( id );
	if ( pGizmo && pGizmo->bVisible )
	{
		siege::database_guid oldGUID = pGizmo->spos.node;
		SiegeNodeHandle objHandle( gSiegeEngine.NodeCache().UseObject( pGizmo->spos.node ) );
		SiegeNode& objNode = objHandle.RequestObject( gSiegeEngine.NodeCache() );
		pGizmo->spos = spos;

		if ( !IsCustomType( pGizmo->type ) )
		{
			GoHandle hObject( MakeGoid(pGizmo->id) );
			if ( hObject.IsValid() ) {

				hObject->GetPlacement()->SetPosition( spos, gPreferences.GetSnapToGround() );

				if ( hObject->GetParent() && hObject->GetParent()->GetInventory() && ( hObject->GetParent()->GetInventory()->GetEquipped( ES_CHEST ) == hObject )) 
				{
					hObject->GetParent()->GetPlacement()->SetPosition( pGizmo->spos, gPreferences.GetSnapToGround() );
				}
				else if ( hObject->HasInventory() ) 
				{
					Go * pArmor = hObject->GetInventory()->GetEquipped( ES_CHEST );						
					if ( pArmor ) 
					{
						pArmor->GetPlacement()->SetPosition( pGizmo->spos, gPreferences.GetSnapToGround() );
						RemoveGizmo( MakeInt(pArmor->GetGoid()) );
					}
				}					
				if ( pGizmo->spos.node != oldGUID ) 
				{
					SiegeNodeHandle new_handle( gSiegeEngine.NodeCache().UseObject( pGizmo->spos.node ) );
					SiegeNode& new_node = new_handle.RequestObject( gSiegeEngine.NodeCache() );

					if ( hObject->HasInventory() ) {
						matrix_3x3 visualOrient =	new_node.GetTransposeOrientation() * 
													( objNode.GetCurrentOrientation() * hObject->GetPlacement()->GetOrientation().BuildMatrix() );
						hObject->GetPlacement()->SetOrientation( Quat(visualOrient) );								

						if ( hObject->GetParent() && hObject->GetParent()->GetInventory() && ( hObject->GetParent()->GetInventory()->GetEquipped( ES_CHEST ) == hObject )) {
							hObject->GetParent()->GetPlacement()->SetOrientation( Quat(visualOrient) );
						}
						else if ( hObject->HasInventory() ) {
							Go * pArmor = hObject->GetInventory()->GetEquipped( ES_CHEST );								
							if ( pArmor ) {
								pArmor->GetPlacement()->SetOrientation( Quat(visualOrient) );
							}
						}
					}
				}
			}								
		}
	}

	gEditorRegion.SetRegionDirty();
}


vector_3 GizmoManager::GetPosition()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && (*i).bVisible ) 
		{
			if ( !IsCustomType( (*i).type ) )
			{
				GoHandle hObject( MakeGoid((*i).id) );
				if ( hObject.IsValid() ) {
					return hObject->GetPlacement()->GetPosition().pos;
				}				
			}
		}
	}
	return vector_3();
}


void GizmoManager::GetPosition( SiegePos & spos )
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && (*i).bVisible ) 
		{
			if ( !IsCustomType( (*i).type ) )
			{
				GoHandle hObject( MakeGoid((*i).id) );
				if ( hObject.IsValid() ) 
				{
					spos = hObject->GetPlacement()->GetPosition();
				}				
			}
		}
	}
}


void GizmoManager::SetBodyScale( float scale ) 
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && (*i).bVisible ) 
		{
			switch ( (*i).type ) 
			{
			case GIZMO_OBJECT:
				{
					GoHandle hObject( MakeGoid((*i).id) );
					if ( hObject.IsValid() && hObject->HasAspect() ) {
						hObject->GetAspect()->SetRenderScaleMultiplier( scale );
					}
				}
				break;		
			}
		}
	}	

	gEditorRegion.SetRegionDirty();
}


float GizmoManager::GetBodyScale()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && (*i).bVisible ) 
		{
			switch ( (*i).type ) 
			{
			case GIZMO_OBJECT:
				{
					GoHandle hObject( MakeGoid((*i).id) );
					if ( hObject.IsValid() && hObject->HasAspect() ) {
						return hObject->GetAspect()->GetRenderScaleMultiplier();
					}
				}
				break;		
			}
		}
	}

	// Default
	return 1.0f;
}

void GizmoManager::SelectGizmo( DWORD object )
{
	if ( gPreferences.CanSelect() )
	{
		GizmoVec::iterator i;
		for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
		{
			if ( (*i).id == object ) 
			{
				if ( gPreferences.GetLinkMode() )
				{
					gEditorObjects.LinkSelectedObjectsToDest( MakeGoid((*i).id) );
					if ( gEditorObjects.GetSequenceType() != SEQUENCE_LINK )
					{
						gPreferences.SetLinkMode( false );
					}
					return;
				}

				if ( !gPreferences.GetSelectAll() )
				{	
					bool bSelect = true;
					GoHandle hObject( MakeGoid( object ) );
					if ( gPreferences.GetSelectNoninteractive() )
					{
						if ( hObject.IsValid() )
						{
							gpstring sName = hObject->GetDataTemplate()->GetMetaGroupName();
							if ( sName.same_no_case( "non_interactive" ) )
							{
								bSelect = false;
							}												
						}
					}
					if ( gPreferences.GetSelectInteractive() )
					{
						if ( hObject.IsValid() )
						{
							gpstring sName = hObject->GetDataTemplate()->GetMetaGroupName();
							if ( sName.same_no_case( "interactive" ) )
							{
								bSelect = false;
							}												
						}
					}
					if ( gPreferences.GetSelectGizmos() )
					{
						if ( !hObject.IsValid() )
						{
							bSelect = false; 
						}
						else if ( hObject.IsValid() && hObject->HasGizmo() )
						{
							bSelect = false;
						}
					}
					if ( gPreferences.GetSelectNonGizmos() )
					{
						if ( hObject.IsValid() && !hObject->HasGizmo() )
						{
							bSelect = false;
						}
					}

					(*i).bSelected = bSelect;

					if ( bSelect && !IsCustomType( (*i).type ) && gPreferences.GetHudSelected() )
					{
						gWorldOptions.SetDebugHUDObject( MakeGoid( (*i).id ) );
					}
				}
				else
				{
					if ( Dialog_Sfx_Editor::DoesSingletonExist() )
					{
						if( gSfxDialog.GetSelectPrimaryTarget() )
						{
							gSfxDialog.SetPrimaryTarget( object );
						}
						else if ( gSfxDialog.GetSelectSecondaryTarget() )
						{
							gSfxDialog.SetSecondaryTarget( object );
						}
					}

					if ( Dialog_Portraits::DoesSingletonExist() && (*i).type == GIZMO_OBJECT )
					{						
						gDialogPortraits.Select( MakeGoid( (*i).id ) );
					}
					(*i).bSelected = true;

					if ( !IsCustomType( (*i).type ) && gPreferences.GetHudSelected() )
					{
						gWorldOptions.SetDebugHUDObject( MakeGoid( (*i).id ) );
					}
				}
			}
		}
	}

	if ( Object_Property_Sheet::DoesSingletonExist() )
	{
		gObjectProperties.Reset();
	}	
}


void GizmoManager::ForceSelectGizmo( DWORD object )
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).id == object ) 
		{
			if ( !IsCustomType( (*i).type ) && gPreferences.GetHudSelected() )
			{
				gWorldOptions.SetDebugHUDObject( MakeGoid( (*i).id ) );
			}

			(*i).bSelected = true;
		}
	}
}


void GizmoManager::DeselectGizmo( DWORD object )
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).id == object ) 
		{
			(*i).bSelected = false;
		}
	}
	if ( Object_Property_Sheet::DoesSingletonExist() )
	{
		gObjectProperties.Reset();
	}
}


Gizmo * GizmoManager::GetGizmo( DWORD id )
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).id == id ) 
		{
			return &(*i);
		}
	}	
	return 0;
}


void GizmoManager::RemoveGizmo( DWORD id ) 
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).id == id ) 
		{
			m_gizmos.erase( i );
			return;
		}
	}		
}


void GizmoManager::LoadLights()
{
	SiegeEngine & engine = gSiegeEngine;
	
	std::map< database_guid, LightConstruct >::iterator i;
	for (	i = engine.LightDatabase().GetLightDatabaseMap().begin(); 
			i != engine.LightDatabase().GetLightDatabaseMap().end();
			++i ) 
	{
				
		SiegePos spos;
		
		siege::LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( (*i).first );

		if ( ld.m_Subtype == LS_STATIC )
		{
			if ( ld.m_Type == siege::LT_POINT ) {
				spos = engine.LightDatabase().GetLightNodeSpacePosition( (*i).first );
				InsertGizmo( GIZMO_POINTLIGHT, spos, (*i).first.GetValue() );
			}		
			else if ( ld.m_Type == siege::LT_SPOT ) {
				spos = engine.LightDatabase().GetLightNodeSpacePosition( (*i).first );			
				InsertGizmo( GIZMO_SPOTLIGHT, spos, (*i).first.GetValue() );
			}
			else if ( ld.m_Type == siege::LT_DIRECTIONAL ) {
				spos = engine.LightDatabase().GetLightNodeSpaceDirection( (*i).first );			
				InsertGizmo( GIZMO_DIRLIGHT, spos, (*i).first.GetValue() );
			}		
		}
	}
}


void GizmoManager::LoadDecals()
{
	const siege::DecalCollection& decals = gSiegeDecalDatabase.GetDecalCollection();
	siege::DecalCollection::const_iterator i;
	for ( i = decals.begin(); i != decals.end(); ++i )
	{	
		gpstring sRandom = gEditorTerrain.GenerateRandomGUID();
		siege::database_guid gizmoId( sRandom.c_str() );

		SiegePos spos = (*i)->GetDecalOrigin();		
		InsertGizmo( GIZMO_DECAL, spos, gizmoId.GetValue() );
		GetGizmo( gizmoId.GetValue() )->index = i.GetIndex();		
	}
}


void GizmoManager::RemoveGizmosFromNode( siege::database_guid nodeGuid )
{
	GizmoVec gizmos = m_gizmos;
	GizmoVec::iterator i;
	for ( i = gizmos.begin(); i != gizmos.end(); ++i ) 
	{
		if ( (*i).spos.node == nodeGuid )
		{
			if ( (*i).type == GIZMO_DIRLIGHT )
			{
				(*i).spos.node = gEditorTerrain.GetTargetNode();
				continue;
			}

			DeleteGizmo( (*i).id, true );
		}
	}
}


void GizmoManager::GetDirectionalLightIDs( std::vector< DWORD > & light_ids )
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).type == GIZMO_DIRLIGHT ) 
		{
			light_ids.push_back( (*i).id );
		}
	}
}


void GizmoManager::GetSelectedDirectionalLightIDs( std::vector< DWORD > & light_ids )
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if (( (*i).type == GIZMO_DIRLIGHT ) && ( (*i).bSelected == true )) 
		{
			light_ids.push_back( (*i).id );
		}
	}
}



void GizmoManager::GetSelectedLightIDs( std::vector< DWORD > & light_ids ) 
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if (( ( (*i).type == GIZMO_DIRLIGHT ) || ((*i).type == GIZMO_SPOTLIGHT) || ((*i).type == GIZMO_POINTLIGHT) )
			&& ( (*i).bSelected == true )) 
		{
			light_ids.push_back( (*i).id );
		}
	}
}


void GizmoManager::GetGizmosOfType( eGizmoType type, std::vector< DWORD > & gizmo_ids )
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i )
	{
		if ( (*i).type == type )
		{
			gizmo_ids.push_back( (*i).id );
		}
	}
}


void GizmoManager::GetSelectedGizmosOfType( eGizmoType type, std::vector< DWORD > & gizmo_ids )
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i )
	{
		if ( (*i).type == type && (*i).bSelected )
		{
			gizmo_ids.push_back( (*i).id );
		}
	}
}


void GizmoManager::Cut()
{	
	gEditorLights.ClearClipboard();
	gEditorObjects.Cut();
	gEditorGizmos.CutDecal();
	gEditorLights.Cut();
	gEditorRegion.SetRegionDirty();
}


void GizmoManager::Copy()
{
	gEditorLights.ClearClipboard();
	gEditorObjects.Copy();
	gEditorGizmos.CopyDecal();
	gEditorLights.Copy();
	gEditorRegion.SetRegionDirty();
}


void GizmoManager::Paste()
{
	DeselectAll();
	gEditorObjects.Paste();
	gEditorLights.Paste();
	gEditorGizmos.PasteDecal();
	gEditorRegion.SetRegionDirty();
}


void GizmoManager::Clear()
{
	m_gizmos.clear();
}


int GizmoManager::GetSelectedGizmoID()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{	
		if ( (*i).bSelected && (*i).bVisible ) 
		{
			return (*i).id;
		}
	}

	return GIZMO_INVALID;
}


void GizmoManager::ShowAllObjects()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( !IsCustomType( (*i).type )  )
		{
			GoHandle hObject( MakeGoid( (*i).id ) );
			hObject->GetEdit()->SetIsVisible( true );		
		}

		(*i).bVisible = true;		
	}
}


void GizmoManager::HideSelectedObjects()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && !IsCustomType( (*i).type ) )
		{
			GoHandle hObject( MakeGoid( (*i).id ) );
			if ( hObject.IsValid() )
			{
				hObject->GetEdit()->SetIsVisible( false );				
				(*i).bVisible = false;
			}
		}
	}
}


void GizmoManager::ShowSelectedObjects()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && !IsCustomType( (*i).type ) )
		{
			GoHandle hObject( MakeGoid( (*i).id ) );
			if ( hObject.IsValid() )
			{
				hObject->GetEdit()->SetIsVisible( true );				
				(*i).bVisible = true;
			}
		}
	}
}


void GizmoManager::ShowGizmosOfType( eGizmoType type, bool bShow )
{
	std::vector< DWORD > gizmo_ids;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetGizmosOfType( type, gizmo_ids );
	for ( i = gizmo_ids.begin(); i != gizmo_ids.end(); ++i )
	{
		GoHandle hObject( MakeGoid(*i) );
		if ( hObject.IsValid() )
		{
			hObject->GetEdit()->SetIsVisible( bShow );
		}
		GetGizmo( *i )->bVisible = bShow;		
	}	
}


bool GizmoManager::IsCustomType( eGizmoType type )
{
	if (( type == GIZMO_POINTLIGHT ) || ( type == GIZMO_SPOTLIGHT ) || ( type == GIZMO_HOTPOINT ) || ( type == GIZMO_DIRHOTPOINT ) || ( type == GIZMO_STARTINGPOSITION ) || ( type == GIZMO_DECAL ) || ( type == GIZMO_DIRLIGHT ) )
	{
		return true;
	}

	return false;
}


bool GizmoManager::IsRotationType( eGizmoType type )
{
	if ( type == GIZMO_SPOTLIGHT )
	{
		return true;
	}

	return false;
}


bool GizmoManager::IsObjectSelected()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && !IsCustomType( (*i).type ) )
		{
			return true;
		}
	}
	return false;
}


bool GizmoManager::IsLightSelected()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && (*i).type == GIZMO_POINTLIGHT || (*i).type == GIZMO_SPOTLIGHT )
		{
			return true;
		}
	}
	return false;
}


bool GizmoManager::DoesSelectionHaveInventory()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && !IsCustomType( (*i).type ) )
		{
			GoHandle hObject( MakeGoid( (*i).id ) );
			if ( hObject.IsValid() && hObject->HasInventory() )
			{				
				return true;
			}
		}
	}
	return false;
}


bool GizmoManager::DoesSelectionHaveEquipment()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && !IsCustomType( (*i).type ) )
		{
			GoHandle hObject( MakeGoid( (*i).id ) );
			if ( hObject.IsValid() && hObject->HasInventory() && !hObject->GetInventory()->IsPackOnly() )
			{				
				return true;
			}
		}
	}
	return false;
}


bool GizmoManager::DoesSelectionHaveAspect()
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).bSelected && !IsCustomType( (*i).type ) )
		{
			GoHandle hObject( MakeGoid( (*i).id ) );
			if ( hObject.IsValid() && hObject->HasAspect() )
			{				
				return true;
			}
		}
	}
	return false;
}


void GizmoManager::DragCopy()
{
	std::vector< DWORD > gizmoIds;
	std::vector< DWORD >::iterator i;
	GetSelectedGizmoIDs( gizmoIds );	
	DeselectAll();
	static int numDrags = 0;
	numDrags++;
	gpgenericf( ( "Number of drag copies: %d\n", numDrags ) );

	DWORD newLead = 0;	
	int selSize = 0;
	for ( i = gizmoIds.begin(); i != gizmoIds.end(); ++i ) 
	{
		if ( !IsCustomType( GetGizmo( *i )->type ) )
		{
			GoHandle hObject( MakeGoid( *i ) );
			if ( hObject.IsValid() )
			{
				Goid newObj = gEditorObjects.AddObject( hObject->GetGoid(), hObject->GetPlacement()->GetPosition() );					
				SelectGizmo( MakeInt(newObj) );			
				
				if ( m_pLeadGizmo && GetLeadGizmo()->id == *i )
				{
					newLead = MakeInt( newObj );
				}

				selSize++;
			}
		}
		else if ( GetGizmo( *i )->type == GIZMO_POINTLIGHT || GetGizmo( *i )->type == GIZMO_SPOTLIGHT )
		{
			siege::LightDescriptor	descriptor;
			SiegePos				spos;
			SiegePos				sdirection;
			spos					= gSiegeEngine.LightDatabase().GetLightNodeSpacePosition( siege::database_guid(*i) );
			descriptor				= gSiegeEngine.LightDatabase().GetLightDescriptor( siege::database_guid(*i) );		

			if ( descriptor.m_Type == LT_SPOT ) 
			{
				sdirection	= gSiegeEngine.LightDatabase().GetLightNodeSpaceDirection( siege::database_guid(*i) );
			}

			siege::database_guid sguid = gEditorLights.AddLight( siege::database_guid(), spos, sdirection, descriptor );
			SelectGizmo( sguid.GetValue() );

			if ( GetLeadGizmo() )
			{
				if ( GetLeadGizmo()->id == *i )
				{
					newLead = sguid.GetValue();
				}
			}

			selSize++;
		}
		else if ( GetGizmo( *i )->type == GIZMO_DECAL )
		{
			if ( !gSiegeEditorView.IsMouseOnTerrain() )
			{
				return;
			}

			Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );
			SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( pGizmo->index );
			gpstring sTexture = gSiegeEngine.Renderer().GetTexturePathname( pDecal->GetDecalTexture() );
			gpstring sKeyName = sTexture;
			int pos = sTexture.find_last_of( "\\" );
			if ( pos != gpstring::npos )
			{
				sKeyName = sTexture.substr( pos+1, sTexture.size() );
			}		

			DWORD id = gEditorGizmos.AddDecal( sKeyName, GetGizmo( *i )->spos );		
			SelectGizmo( id );	
			
			if ( GetLeadGizmo() && GetLeadGizmo()->id == *i )
			{
				newLead = id;
			}

			selSize++;			
		}
	}

	if ( selSize != 0 )
	{
		SetLeadGizmo( GetGizmo( newLead ) );
	}

	gEditorRegion.SetRegionDirty();
}


void GizmoManager::ShowGizmo( DWORD id, bool bShow )
{
	Gizmo * pGizmo = GetGizmo( id );
	if ( pGizmo )
	{
		pGizmo->bVisible = bShow;
		
		if ( !IsCustomType( pGizmo->type ) )
		{
			GoHandle hObject( MakeGoid(id) );
			if ( hObject.IsValid() )
			{
				hObject->GetEdit()->SetIsVisible( bShow );				
			}
		}
	}
}
	

void GizmoManager::SelectGroupings( StringVec groups )
{
	StringVec::iterator i;
	for ( i = groups.begin(); i != groups.end(); ++i )
	{
		GizmoGroupMap::iterator iGroup = m_gizmoGroups.find( *i );
		if ( iGroup != m_gizmoGroups.end() )
		{
			GizmoIdVec::iterator iId;
			for ( iId = (*iGroup).second.begin(); iId != (*iGroup).second.end(); ++iId )
			{
				ForceSelectGizmo( *iId );
			}
		}
		else
		{
			// Assume this means it's a metagroup or category
			GizmoVec::iterator j;
			for ( j = m_gizmos.begin(); j != m_gizmos.end(); ++j ) 
			{
				if ( !IsCustomType( (*j).type ) )
				{
					GoHandle hObject( MakeGoid( (*j).id ) );
					if ( hObject.IsValid() )
					{
						if ( (*i).same_no_case( hObject->GetDataTemplate()->GetMetaGroupName() ) ||
							 (*i).same_no_case( hObject->GetDataTemplate()->GetCategoryName() ) )
						{
							ForceSelectGizmo( (*j).id );
						}
					}
				}
			}
		}
	}
}


void GizmoManager::DeselectGroupings( StringVec groups )
{
	StringVec::iterator i;
	for ( i = groups.begin(); i != groups.end(); ++i )
	{
		GizmoGroupMap::iterator iGroup = m_gizmoGroups.find( *i );
		if ( iGroup != m_gizmoGroups.end() )
		{
			GizmoIdVec::iterator iId;
			for ( iId = (*iGroup).second.begin(); iId != (*iGroup).second.end(); ++iId )
			{
				DeselectGizmo( *iId );
			}
		}
		else
		{
			// Assume this means its a metagroup
			GizmoVec::iterator j;
			for ( j = m_gizmos.begin(); j != m_gizmos.end(); ++j ) 
			{
				if ( !IsCustomType( (*j).type ) )
				{
					GoHandle hObject( MakeGoid((*j).id) );
					if ( hObject.IsValid() )
					{
						if ( (*i).same_no_case( hObject->GetDataTemplate()->GetMetaGroupName() ) ||
							 (*i).same_no_case( hObject->GetDataTemplate()->GetCategoryName() ) )
						{
							DeselectGizmo( (*j).id );
						}
					}				
				}
			}
		}
	}
}


void GizmoManager::ShowGroupings( StringVec groups )
{
	StringVec::iterator i;
	for ( i = groups.begin(); i != groups.end(); ++i )
	{
		GizmoGroupMap::iterator iGroup = m_gizmoGroups.find( *i );
		if ( iGroup != m_gizmoGroups.end() )
		{
			GizmoIdVec::iterator iId;
			for ( iId = (*iGroup).second.begin(); iId != (*iGroup).second.end(); ++iId )
			{
				ShowGizmo( *iId, true );
			}
		}
		else
		{
			// Assume this means its a metagroup
			GizmoVec::iterator j;
			for ( j = m_gizmos.begin(); j != m_gizmos.end(); ++j ) 
			{
				if ( !IsCustomType( (*j).type ) )
				{
					GoHandle hObject( MakeGoid((*j).id) );
					if ( hObject.IsValid() )
					{
						if ( (*i).same_no_case( hObject->GetDataTemplate()->GetMetaGroupName() ) ||
							 (*i).same_no_case( hObject->GetDataTemplate()->GetCategoryName() ) )
						{
							ShowGizmo( (*j).id, true );
						}
					}					
				}
			}
		}
	}
}


void GizmoManager::HideGroupings( StringVec groups )
{
	StringVec::iterator i;
	for ( i = groups.begin(); i != groups.end(); ++i )
	{
		GizmoGroupMap::iterator iGroup = m_gizmoGroups.find( *i );
		if ( iGroup != m_gizmoGroups.end() )
		{
			GizmoIdVec::iterator iId;
			for ( iId = (*iGroup).second.begin(); iId != (*iGroup).second.end(); ++iId )
			{
				ShowGizmo( *iId, false );
			}
		}
		else
		{
			// Assume this means its a metagroup
			GizmoVec::iterator j;
			for ( j = m_gizmos.begin(); j != m_gizmos.end(); ++j ) 
			{
				if ( !IsCustomType( (*j).type ) )
				{
					GoHandle hObject( MakeGoid((*j).id) );
					if ( hObject.IsValid() )
					{
						if ( (*i).same_no_case( hObject->GetDataTemplate()->GetMetaGroupName() ) ||
								 (*i).same_no_case( hObject->GetDataTemplate()->GetCategoryName() ) )
						{
							ShowGizmo( (*j).id, false );						
						}
					}
				}
			}
		}
	}
}


void GizmoManager::RandomRotateSelection()
{
	std::vector< DWORD > gizmoIds;
	std::vector< DWORD >::iterator i;
	GetSelectedGizmoIDs( gizmoIds );
	for ( i = gizmoIds.begin(); i != gizmoIds.end(); ++i ) 
	{		
		Quat orient = GetOrientation( *i );
		orient.RotateY( DegreesToRadians( Random( 0.0f, 359.0f ) ) );
		SetOrientation( *i, orient );		
	}
}


void GizmoManager::RandomScaleSelection( float min, float max )
{
	std::vector< DWORD > gizmoIds;
	std::vector< DWORD >::iterator i;
	GetSelectedGizmosOfType( GIZMO_OBJECT, gizmoIds );
	for ( i = gizmoIds.begin(); i != gizmoIds.end(); ++i ) 
	{		
		GoHandle hObject( MakeGoid( *i ) );
		if ( hObject.IsValid() )
		{
			hObject->GetAspect()->SetRenderScaleMultiplier( ::Random( min, max ) );
		}
	}
}


bool GizmoManager::NewGroup( gpstring sGroup )
{
	GizmoGroupMap::iterator i = m_gizmoGroups.find( sGroup );
	if ( i == m_gizmoGroups.end() )
	{
		GizmoIdVec idVec;
		m_gizmoGroups.insert( std::make_pair( sGroup, idVec ) );
		return true;
	}

	return false;
}


void GizmoManager::RemoveGroups( StringVec groups )
{
	StringVec::iterator i;
	for ( i = groups.begin(); i != groups.end(); ++i )
	{
		GizmoGroupMap::iterator iGroup = m_gizmoGroups.find( *i );
		if ( iGroup != m_gizmoGroups.end() )
		{
			m_gizmoGroups.erase( iGroup );
		}
	}
}


void GizmoManager::AddSelectionToGroup( StringVec groups )
{
	StringVec::iterator i;
	for ( i = groups.begin(); i != groups.end(); ++i )
	{
		GizmoGroupMap::iterator iGroup = m_gizmoGroups.find( *i );

		std::vector< DWORD > objects;
		std::vector< DWORD >::iterator iId;
		GetSelectedGizmoIDs( objects );
		for ( iId = objects.begin(); iId != objects.end(); ++iId )
		{
			(*iGroup).second.push_back( *iId );
		}
	}		
}


void GizmoManager::GetGizmoGroupNames( StringVec & groups )
{
	GizmoGroupMap::iterator i;
	for ( i = m_gizmoGroups.begin(); i != m_gizmoGroups.end(); ++i )
	{
		groups.push_back( (*i).first );
	}
}


void GizmoManager::RotateSelected( int mouseDeltaX )
{
	if ( mouseDeltaX == 0 )
	{
		return;
	}

	float radians = 1.0f;
	if ( mouseDeltaX < 0 )
	{
		radians = DegreesToRadians( gPreferences.GetRotationIncrement() );
	}
	else
	{
		radians = DegreesToRadians( -gPreferences.GetRotationIncrement() );
	}	

	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject(m_mouseRotatePos.node) );
	SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
	vector_3 worldMasterPos = node.NodeToWorldSpace( m_mouseRotatePos.pos );

	std::vector< DWORD > gizmoIds;
	std::vector< DWORD >::iterator i;
	GetSelectedGizmoIDs( gizmoIds );
	for ( i = gizmoIds.begin(); i != gizmoIds.end(); ++i ) 
	{
		Gizmo * pGizmo = GetGizmo( *i );
		if ( pGizmo )
		{
			SiegeNodeHandle child_handle( gSiegeEngine.NodeCache().UseObject(pGizmo->spos.node) );
			SiegeNode& child_node = child_handle.RequestObject( gSiegeEngine.NodeCache() );
			vector_3 childPos = child_node.NodeToWorldSpace( pGizmo->spos.pos );
			
			vector_3 newPos = gSiegeEngine.GetDifferenceVector( m_mouseRotatePos, pGizmo->spos );
			newPos.RotateY( radians );
			vector_3 finalPos;
			finalPos.x = m_mouseRotatePos.pos.x + newPos.x;
			finalPos.y = m_mouseRotatePos.pos.y;
			finalPos.z = m_mouseRotatePos.pos.z + newPos.z;
			
			SiegePos sposNew;
			sposNew.pos = finalPos;
			sposNew.node = node.GetGUID();

			AccurateAdjustPositionToTerrain( sposNew );			

			SetPosition( *i, sposNew );
		}
	}	

	gEditorRegion.SetRegionDirty();
}


void GizmoManager::SaveGroups()
{
	GizmoGroupMap::iterator i;
	for ( i = m_gizmoGroups.begin(); i != m_gizmoGroups.end(); ++i )
	{
		FuelHandle hConfig( "config:editor" );
		if ( hConfig.IsValid() )
		{
			FuelHandle hGroups = hConfig->GetChildBlock( "groups" );
			if ( !hGroups.IsValid() )
			{
				hGroups = hConfig->CreateChildDirBlock( "groups" );
			}

			FuelHandle hMapGroup = hGroups->GetChildBlock( gEditorRegion.GetMapName() );
			if ( !hMapGroup.IsValid() )
			{
				hMapGroup = hGroups->CreateChildDirBlock( gEditorRegion.GetMapName() );
			}
			
			FuelHandle hRegionGroup = hMapGroup->GetChildBlock( gEditorRegion.GetRegionName() );
			if ( hRegionGroup.IsValid() )
			{
				hMapGroup->DestroyChildBlock( hRegionGroup );
			}
			
			gpstring sGroup = gEditorRegion.GetRegionName();
			sGroup += "_groups.gas";
			hRegionGroup = hMapGroup->CreateChildBlock( gEditorRegion.GetRegionName(), sGroup );
			
			FuelHandle hGroup = hRegionGroup->CreateChildBlock( (*i).first );
			if ( hGroup.IsValid() )
			{			
				GizmoIdVec::iterator iObj;
				for ( iObj = (*i).second.begin(); iObj != (*i).second.end(); ++iObj )
				{
					hGroup->Add( "*", (*iObj), FVP_HEX_INT );			
				}
			}
		}		
	}	
}


void GizmoManager::LoadGroups()
{
	FuelHandle hConfig( "config:editor" );
	if ( hConfig.IsValid() )
	{
		FuelHandle hGroups = hConfig->GetChildBlock( "groups" );
		if ( hGroups.IsValid() )
		{	
			FuelHandle hMapGroup = hGroups->GetChildBlock( gEditorRegion.GetMapName() );
			if ( hMapGroup.IsValid() )
			{			
				FuelHandle hRegionGroup = hMapGroup->GetChildBlock( gEditorRegion.GetRegionName() );
				if ( hRegionGroup.IsValid() )
				{					
					FuelHandleList hlGroups = hRegionGroup->ListChildBlocks( 1 );
					FuelHandleList::iterator iGroup; 

					for ( iGroup = hlGroups.begin(); iGroup != hlGroups.end(); ++iGroup )
					{	
						GizmoIdVec objects;
						
						if ( (*iGroup)->FindFirst( "*" ) )
						{
							gpstring sValue;
							while ( (*iGroup)->GetNext( sValue ) )
							{
								int id = 0;
								stringtool::Get( sValue.c_str(), id );
								objects.push_back( id );
							}
						}

						m_gizmoGroups.insert( std::make_pair( (*iGroup)->GetName(), objects ) );
					}
				}
			}
		}
	}
}


void GizmoManager::SetShowObjects( bool bShow )
{
	GizmoVec::iterator i;
	for ( i = m_gizmos.begin(); i != m_gizmos.end(); ++i ) 
	{
		if ( (*i).type == GIZMO_OBJECT )
		{
			GoHandle hObject( MakeGoid( (*i).id ) );
			if ( hObject.IsValid() && hObject->GetEdit()->IsVisible() )
			{
				(*i).bVisible = bShow;
			}
		}
	}
}


void GizmoManager::SetStartingPosition( int id, SiegePos spos )
{
	m_bSnapStartPos = true;
	m_startPos = spos;
	m_startPosId = id;
	gEditorRegion.SetRegionDirty();
}