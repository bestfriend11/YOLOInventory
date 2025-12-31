//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorTerrain.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////



// Include Files
#include "PrecompEditor.h"
#include "EditorTerrain.h"
#include "RapiFont.h"
#include "Rapi.h"
#include "RapiPrimitive.h"
#include "Services.h"
#include "EditorRegion.h"
#include "EditorCamera.h"
#include "EditorObjects.h"
#include "siege_mesh_door.h"
#include "LogicalNodeCreator.h"
#include "Stitch_Helper.h"
#include "WorldMap.h"
#include "CommandActions.h"
#include "namingkey.h"
#include "siege_camera.h"
#include "EditorLights.h"
#include "LeftView.h"
#include "Dialog_Node_Matches.h"
#include "ImageListDefines.h"
#include "Dialog_Node_Match_Progress.h"
#include "Preferences.h"
#include "SiegeEditorView.h"
#include "plane_3.h"
#include "space_3.h"
#include "line_segment_3.h"
#include "WorldTime.h"

// Namespaces
using namespace siege;

// Constants
const float MAX_MOVE_TOLERANCE = 10000;

// Construction
EditorTerrain::EditorTerrain()
	: m_selected_node( UNDEFINED_GUID )
	, m_secondary_node( UNDEFINED_GUID )
	, m_target_node( UNDEFINED_GUID )
	, m_selected_door_id( 0 )
	, m_sNodeSet( "grs01" )
	, m_pFont( 0 )
	, m_SelectedAlpha( 128 )
	, m_bSelectedRotate( false )
	, m_source_door_id( 0 )	
	, m_dest_door_id( 0 )
	, m_tempHitLogicalId( 0 )
	, m_bSelectedIsSnapped( false )
{
	m_pFont = gSiegeEngine.Renderer().CreateFont();	
	m_last_added = UNDEFINED_GUID;	
	ReinitColors();

	// Set up the draw node
	m_DrawSelected = siege::database_guid( GenerateRandomGUID() );	
}


EditorTerrain::~EditorTerrain()
{
	UnloadSelectedNode();
}


void EditorTerrain::ReinitColors()
{
	m_ValueToColorMap.clear();
	m_ColorUsageMap.clear();

	for ( int i = 0; i != 1000; ++i )
	{
		m_ColorUsageMap.insert( std::make_pair( Random( 0, 16777216 ), false ) );
	}
}


void EditorTerrain::InitNodeCallback()
{
	gSiegeEngine.RegisterNodeWorldMembershipChangedCb( makeFunctor( *this, &EditorTerrain::NodeChangeWorld ) );
}


void EditorTerrain::NodeChangeWorld( const siege::SiegeNode& node, unsigned int oldMembership, bool nodeWasDeleted )
{
	if ( GoDb::DoesSingletonExist() )
	{
		gGoDb.NodeChangeMembership( node, oldMembership, nodeWasDeleted );
	}

	if( nodeWasDeleted && GizmoManager::DoesSingletonExist() && !gEditor.IsShuttingDown() )
	{
		if ( EditorRegion::DoesSingletonExist() && !gEditorRegion.GetLoading() )
		{
			DeleteSiegeNode( node.GetGUID() );
		}
	}	
}


void EditorTerrain::LoadSelectedNode()	
{
	if ( !gPreferences.GetModeNodePlacement() )
	{
		return;
	}

	UnloadSelectedNode();

	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( m_DrawSelected ) );
	SiegeNode& node = handle.CreateObject( gSiegeEngine.NodeCache() );	
	node.SetTextureSetAbbr( gEditorTerrain.GetNodeSetCharacters() );
		
	vector_3 targetpos;
	node.SetMeshGUID( GetSecondaryNode() );	
	const SiegeMesh& bmesh = node.GetMeshHandle().RequestObject(gSiegeEngine.MeshCache());	
	node.SetNumLitVertices( bmesh.GetNumberOfVertices() );		
	node.SetAmbientColor( fScaleDWORDColor( 0xffffffff, 1.0f ) );
	node.AccumulateStaticLighting();

	m_SelectedPos = vector_3::ZERO;
	m_SelectedGuid = GetSecondaryNode();

	if ( !bmesh.GetDoorByID( GetDestConnectionID() ) )
	{
		SetDestConnectionID( 1 );
	}
}


void EditorTerrain::DrawSelectedNode()
{
	if ( !gPreferences.GetModeNodePlacement() || !GetSecondaryNode().IsValid() )
	{
		return;
	}

	// If the node isn't loaded yet, let's load it now
	if ( m_SelectedGuid != GetSecondaryNode() )
	{
		LoadSelectedNode();
	}
	
	// Grab the node
	SiegeNodeHandle		handle( gSiegeEngine.NodeCache().UseObject( m_DrawSelected ) );
	SiegeNode&			node		= handle.RequestObject( gSiegeEngine.NodeCache() );		
	const SiegeMesh&	bmesh		= node.GetMeshHandle().RequestObject(gSiegeEngine.MeshCache());
	vector_3			targetpos	= m_SelectedOrient * bmesh.Centroid();		

	// Render the node
	{
		// If possible, let's add the "connect dot".  This dot tells the user where to lead the mouse pointer to make a connection
		if ( gSiegeEditorView.IsMouseOnTerrain() )
		{
			SiegePos spos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
			SiegeNode * pHitNode = gSiegeEngine.IsNodeValid( spos.node );
			if ( pHitNode )
			{
				const SiegeMesh& hitMesh = pHitNode->GetMeshHandle().RequestObject(gSiegeEngine.MeshCache());
				const siege::SiegeMesh::SiegeMeshDoorList& door_list = hitMesh.GetDoors();
				siege::SiegeMesh::SiegeMeshDoorConstIter i;		
				float closestValue = FLOAT_MAX;
				vector_3 closestPoint;
				matrix_3x3 closestOrient;
				for ( i = door_list.begin(); i != door_list.end(); ++i )
				{
					if ( pHitNode->GetDoorByID( (*i)->GetID() ) )
					{
						continue;
					}
					float dx = (*i)->GetCenter().x - spos.pos.x;
					float dy = (*i)->GetCenter().y - spos.pos.y;
					float dz = (*i)->GetCenter().z - spos.pos.z;
					float value = fabs(dx) + fabs(dy) + fabs(dz);
					if ( value < closestValue )
					{
						closestValue = value;
						closestPoint = (*i)->GetCenter();
						closestOrient = (*i)->GetOrientation();
					}
				}

				if ( closestValue != FLOAT_MAX )
				{
					// closestPoint = pHitNode->NodeToWorldSpace( closestPoint );
					gSiegeEngine.Renderer().PushWorldMatrix();
					gSiegeEngine.Renderer().SetWorldMatrix( pHitNode->GetCurrentOrientation(), pHitNode->GetCurrentCenter() );
					gSiegeEngine.Renderer().TranslateWorldMatrix( closestPoint );
					gSiegeEngine.Renderer().RotateWorldMatrix( closestOrient );
					RP_DrawSphere( gSiegeEngine.Renderer(), 0.2f, 20, 0xffff00ff );
					gSiegeEngine.Renderer().PopWorldMatrix();
				}
			}
		}

		// Render the node itself
		gSiegeEngine.Renderer().PushWorldMatrix();		
		SiegeMeshDoor * pDoor = bmesh.GetDoorByID( GetDestConnectionID() );
		if ( pDoor )
		{
			targetpos = m_SelectedOrient * pDoor->GetCenter();
		}	
		gSiegeEngine.Renderer().TranslateWorldMatrix( -targetpos+m_SelectedPos );
		gSiegeEngine.Renderer().RotateWorldMatrix( m_SelectedOrient );		
		bmesh.Render( 0, node.GetTextureListing(), (DWORD *)node.GetStaticLightingData(), m_SelectedAlpha );	
		gSiegeEngine.Renderer().PopWorldMatrix();

		// Draw all doors
		if ( gPreferences.GetVisualDrawDoors() )
		{			
			const siege::SiegeMesh::SiegeMeshDoorList& door_list = bmesh.GetDoors();
			siege::SiegeMesh::SiegeMeshDoorConstIter i;		
			float closestValue = FLOAT_MAX;
			vector_3 closestPoint;
			matrix_3x3 closestOrient;
			for ( i = door_list.begin(); i != door_list.end(); ++i )
			{
				DrawDoor( m_DrawSelected, (*i)->GetID(), -targetpos+m_SelectedPos, m_SelectedOrient, gPreferences.GetVisualValidColor() );		
			}
		}

		// Draw the selected door
		if ( gPreferences.GetVisualDrawDestId() )
		{
			DrawDoor( m_DrawSelected, GetDestConnectionID(), -targetpos+m_SelectedPos, m_SelectedOrient, gPreferences.GetVisualDestDoorColor() );		
		}		

		// Draw the door numbers
		if ( gPreferences.GetVisualDrawDoorNumbers() )
		{
			DrawDoorNumbers( m_DrawSelected );
		}

		// Draw the bounding box of the node
		if ( gPreferences.GetVisualDrawMeshBox() )
		{
			gSiegeEngine.Renderer().PushWorldMatrix();
			gSiegeEngine.Renderer().SetWorldMatrix( m_SelectedOrient, -targetpos+m_SelectedPos );
			DrawNodeBox( bmesh.BBoxCenter(), bmesh.BBoxHalfDiag(), gPreferences.GetVisualDestDoorColor(), gSiegeEngine.Renderer() );
			gSiegeEngine.Renderer().PopWorldMatrix();
		}		
	}
}


void EditorTerrain::CalculateNodePlacement()
{
	if ( !GetSecondaryNode().IsValid() || !gEditorRegion.GetRegionEnable() || GetSelectedSnapped() )
	{
		return;
	}

	// If the node isn't loaded yet, let's load it now
	if ( m_SelectedGuid != GetSecondaryNode() )
	{
		LoadSelectedNode();
	}	

	m_SelectedAlpha = 128;
	SetSelectedNode( UNDEFINED_GUID );

	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( m_DrawSelected ) );
	SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );						
	const SiegeMesh& bmesh = node.GetMeshHandle().RequestObject(gSiegeEngine.MeshCache());

	// Figure out where to draw the node based on the mouse position
	{
		vector_3 moveVector;
		moveVector = vector_3( 0, 1, 0 );
		
		float			d = -moveVector.DotProduct( m_SelectedPos );	
		plane_3			movePlane( moveVector, d );

		// Now let's find the direction of where the user clicked and generate a line segment from it so we can clip it to the plane	
		vector_3		moveDirection = ( gSiegeEngine.GetCamera().GetMatrixOrientation() * gSiegeEngine.GetMouseShadow().CameraSpacePosition() );
		line_segment_3	moveSegment( gSiegeEngine.GetCamera().GetCameraPosition(), moveDirection, 0, 1000 ); 
		ClipLineSegment( movePlane, moveSegment );

		float xDiff = (moveSegment.GetMaximumEndpoint().x - m_SelectedPos.x);
		float yDiff = (moveSegment.GetMaximumEndpoint().y - m_SelectedPos.y);
		float zDiff = (moveSegment.GetMaximumEndpoint().z - m_SelectedPos.z);

		if (( fabsf(xDiff) <= MAX_MOVE_TOLERANCE ) &&
			( fabsf(yDiff) <= MAX_MOVE_TOLERANCE ) &&
			( fabsf(zDiff) <= MAX_MOVE_TOLERANCE ))
		{
			m_SelectedPos.x = m_SelectedPos.x + xDiff;			
			m_SelectedPos.y = m_SelectedPos.y + yDiff;
			m_SelectedPos.z = m_SelectedPos.z + zDiff;	
			
			// If the mouse cursor is over a node, let's "lead the target" to allow easier Y-axis matchups.
			if ( gSiegeEditorView.IsMouseOnTerrain() )
			{
				SiegePos spos;
				spos.pos = m_SelectedPos;
				spos.node = GetTargetNode();				
			
				spos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
				
				if ( spos.node != GetTargetNode() )
				{
					m_SelectedPos = spos.WorldPos();
				}
				else
				{
					m_SelectedPos = spos.pos;
				}			
			}
		}		
	}	

	// Check for source door matchups
	if ( m_SelectedCalcTime == 0.0f && !GetSelectedSnapped() )
	{	
		float closestValue = FLOAT_MAX;
		database_guid closestNode;
		int closestDoor		= 0;
		int closestDestDoor = 0;
		int closestDestId	= 0;
		
		const siege::SiegeMesh::SiegeMeshDoorList& door_list = bmesh.GetDoors();
		siege::SiegeMesh::SiegeMeshDoorConstIter j;		
		
		vector_3 targetpos = m_SelectedOrient * bmesh.Centroid();
		SiegeMeshDoor * pDestDoor = bmesh.GetDoorByID( GetDestConnectionID() );
		if ( pDestDoor )
		{
			targetpos = m_SelectedOrient * pDestDoor->GetCenter();
		}		
		
		for ( j = door_list.begin(); j != door_list.end(); ++j ) 
		{
			SiegeNodeDoor *sdoor = node.GetDoorByID( (*j)->GetID() );
			if ( sdoor != 0 ) 
			{
				continue;
			}			
			
			if ( !gPreferences.GetVisualAllowDoorGuess() && (*j)->GetID() != GetDestConnectionID() )
			{
				continue;
			}

			vector_3 door1_center = (m_SelectedOrient * (*j)->GetCenter()) + (m_SelectedPos-targetpos);						

			FrustumNodeColl nodeColl = gSiegeEngine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
			FrustumNodeColl::iterator iNode;
			for( iNode = nodeColl.begin(); iNode != nodeColl.end(); ++iNode )
			{				
				const SiegeMesh& dest_bmesh = (*iNode)->GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );
				
				const siege::SiegeMesh::SiegeMeshDoorList&		door_list2 = dest_bmesh.GetDoors();
				siege::SiegeMesh::SiegeMeshDoorConstIter			l;	
						
				for ( l = door_list2.begin(); l != door_list2.end(); ++l ) 
				{
					SiegeNodeDoor *sdoor2 = (*iNode)->GetDoorByID( (*l)->GetID() );
					if ( sdoor2 != 0 ) 
					{
						continue;
					}
					vector_3 door2_center = (*l)->GetCenter();
					door2_center = (*iNode)->NodeToWorldSpace( door2_center );

					float dx	=	door1_center.GetX() - door2_center.GetX();
					float dy	=	door1_center.GetY() - door2_center.GetY();
					float dz	=	door1_center.GetZ() - door2_center.GetZ();				

					if ( (( dx < gPreferences.GetVisualSnapSensitivity() ) && ( dx > -gPreferences.GetVisualSnapSensitivity() )) &&
				    //   (( dy < gPreferences.GetVisualSnapSensitivity() ) && ( dy > -gPreferences.GetVisualSnapSensitivity() )) &&
						 (( dz < gPreferences.GetVisualSnapSensitivity() ) && ( dz > -gPreferences.GetVisualSnapSensitivity() )) ) 
					{	
						float absValue = fabs(dx)+fabs(dy)+fabs(dz);
						if ( absValue < closestValue )
						{													
							closestNode = (*iNode)->GetGUID();
							closestDoor = (*l)->GetID();
							closestValue = absValue;
							closestDestId = (*j)->GetID();									
						}
					}
				}
			}
		}		

		if ( closestValue != FLOAT_MAX )
		{
			int selectId = -1;
			bool bFindOther = true;
			bool bInvalid = false;
			if ( DoDoorsMatch( closestNode, closestDoor, GetSecondaryNode(), closestDestId, false ) )
			{		
				selectId = closestDestId;
				bFindOther = false;				
			}
			else if ( gPreferences.GetVisualAllowInvalidMatch() )
			{
				selectId = closestDestId;
				bFindOther = false;				
				bInvalid = true;
			}
			
			if ( bFindOther && gPreferences.GetVisualAllowDoorGuess() )
			{				
				const siege::SiegeMesh::SiegeMeshDoorList& door_list = bmesh.GetDoors();
				siege::SiegeMesh::SiegeMeshDoorConstIter j;				
				for ( j = door_list.begin(); j != door_list.end(); ++j ) 
				{
					if ( DoDoorsMatch( closestNode, closestDoor, GetSecondaryNode(), (*j)->GetID(), false ) )
					{
						selectId = (*j)->GetID();										
					}
				}
			}
				
			if ( selectId != -1 )
			{
				if ( (GetSelectedNode() == closestNode) && (GetSelectedSourceDoorID() == closestDoor) && GetDestConnectionID() == selectId )
				{
					// do nothing here for now.
				}
				else
				{
					SetSelectedNode( closestNode );
					SetSourceDoors( closestNode );
					gLeftView.PublishSourceDoorList();						
					SetSelectedSourceDoorID( closestDoor );							
				}
			
				SetDestConnectionID( selectId );
				
				// We have a match, let's do the pseudo-connect
				{
					// Load the meshes
					SiegeNode * pSourceNode = gSiegeEngine.IsNodeValid( GetSelectedNode() );

					siege::SiegeMeshHandle sHandle( gSiegeEngine.MeshCache().UseObject( pSourceNode->GetMeshGUID() ) );
					siege::SiegeMesh& sMesh	= sHandle.RequestObject( gSiegeEngine.MeshCache() );

					// With the meshes in hand we should now be able to look up the doors we are connecting through
					siege::SiegeMeshDoor* near_door	= sMesh.GetDoorByID( GetSelectedSourceDoorID() );
					siege::SiegeMeshDoor* far_door	= bmesh.GetDoorByID( GetDestConnectionID() );					

					matrix_3x3 inv_far_orient		= far_door->GetOrientation().Transpose_T();
					matrix_3x3 orient				= near_door->GetOrientation() * pSourceNode->GetCurrentOrientation();

					orient.SetColumn_0( -orient.GetColumn_0() );
					orient.SetColumn_2( -orient.GetColumn_2() );					

					// Calculate the orient/offset pair that identifies the transformation from dest node to src node space.
					m_SelectedOrient	= orient * inv_far_orient;
					m_SelectedPos = pSourceNode->NodeToWorldSpace( near_door->GetCenter() );
									
					// Since we could be switching the door we are connecting to, let's set the mouse position to the snap location to prevent popping.
					SiegePos spos;
					spos.node = GetTargetNode();
					spos.pos = m_SelectedPos;
					vector_3 screen = spos.ScreenPos();
					gSiegeEngine.GetMouseShadow().Update( screen.x, screen.y );
					POINT newPoint;
					newPoint.x = screen.x;
					newPoint.y = screen.y;
					ClientToScreen( gSiegeEditorView.GetSafeHwnd(), &newPoint );
					SetCursorPos( newPoint.x, newPoint.y );

					SetSelectedSnapped( true );
				}

				if ( node.GetAmbientColor() != gPreferences.GetVisualValidColor() )
				{
					if ( bInvalid )
					{
						node.SetAmbientColor( fScaleDWORDColor( gPreferences.GetVisualInvalidColor(), 1.0f ) );
					}
					else
					{
						node.SetAmbientColor( fScaleDWORDColor( gPreferences.GetVisualValidColor(), 1.0f ) );
					}
					node.AccumulateStaticLighting();
				}

				m_SelectedAlpha = 200;
			}
			else
			{
				SetDestConnectionID( closestDestId );
				if ( node.GetAmbientColor() != gPreferences.GetVisualInvalidColor() )
				{
					node.SetAmbientColor( fScaleDWORDColor( gPreferences.GetVisualInvalidColor(), 1.0f ) );
					node.AccumulateStaticLighting();
				}
			}
		}
	}
}


void EditorTerrain::PlaceSelectedNode()
{
	if ( GetSelectedNode() == UNDEFINED_GUID )
	{
		return;
	}

	AttachSiegeNodes( GetSelectedSourceDoorID(), GetDestConnectionID() );

	if ( !gPreferences.GetModeNodePlacement() || !GetSecondaryNode().IsValid() || !m_DrawSelected.IsValid() )
	{
		return;
	}
	else
	{
		SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( m_DrawSelected ) );
		SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
		if ( node.GetAmbientColor() != 0xffffffff )
		{
			node.SetAmbientColor( fScaleDWORDColor( 0xffffffff, 1.0f ) );
			node.AccumulateStaticLighting();
		}
	}
}


void EditorTerrain::RotateSelectedNode()
{ 
	if ( !gPreferences.GetModeNodePlacement() )
	{
		return;
	}

	if ( !GetSecondaryNode().IsValid() || !m_DrawSelected.IsValid() )
	{
		return;
	}

	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( m_DrawSelected ) );
	SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );						
	const SiegeMesh& bmesh = node.GetMeshHandle().RequestObject(gSiegeEngine.MeshCache());	
	const siege::SiegeMesh::SiegeMeshDoorList& door_list = bmesh.GetDoors();
	siege::SiegeMesh::SiegeMeshDoorConstIter j;	

	if ( IsNodeSnapped() )
	{
		bool bFirst = true;
		bool bStartCheck = false;
		bool bFoundMatch = false;
		int selectId = GetDestConnectionID();
		for ( j = door_list.begin(); j != door_list.end(); ) 
		{
			if ( bStartCheck )
			{
				if ( DoDoorsMatch( GetSelectedNode(), GetSelectedSourceDoorID(), GetSecondaryNode(), (*j)->GetID(), false ) )
				{
					selectId = (*j)->GetID();					
					if ( node.GetAmbientColor() != gPreferences.GetVisualValidColor() )
					{
						node.SetAmbientColor( fScaleDWORDColor( gPreferences.GetVisualValidColor(), 1.0f ) );
						node.AccumulateStaticLighting();
					}
					break;
				}
			}

			if ( selectId == (*j)->GetID() )
			{
				if ( bStartCheck )
				{
					break;
				}
				
				bStartCheck = true;
			}					

			++j;	
			
			if ( j == door_list.end() )
			{
				j = door_list.begin();
			}
		}
		
		SetDestConnectionID( selectId );

		// We have a match, let's do the pseudo-connect
		{
			// Load the meshes
			SiegeNode * pSourceNode = gSiegeEngine.IsNodeValid( GetSelectedNode() );

			siege::SiegeMeshHandle sHandle( gSiegeEngine.MeshCache().UseObject( pSourceNode->GetMeshGUID() ) );
			siege::SiegeMesh& sMesh	= sHandle.RequestObject( gSiegeEngine.MeshCache() );

			// With the meshes in hand we should now be able to look up the doors we are connecting through
			siege::SiegeMeshDoor* near_door	= sMesh.GetDoorByID( GetSelectedSourceDoorID() );
			siege::SiegeMeshDoor* far_door	= bmesh.GetDoorByID( GetDestConnectionID() );					

			matrix_3x3 inv_far_orient		= far_door->GetOrientation().Transpose_T();
			matrix_3x3 orient				= near_door->GetOrientation() * pSourceNode->GetCurrentOrientation();

			orient.SetColumn_0( -orient.GetColumn_0() );
			orient.SetColumn_2( -orient.GetColumn_2() );					

			// Calculate the orient/offset pair that identifies the transformation from dest node to src node space.
			m_SelectedOrient	= orient * inv_far_orient;			
			
			m_SelectedPos = pSourceNode->NodeToWorldSpace( near_door->GetCenter() );			
		}	
	}
	else 
	{
		if ( GetDestConnectionID() == 0 )
		{
			SetDestConnectionID( 1 );
		}

		bool bStartCheck = false;	
		int doorCount = 0;
		for ( j = door_list.begin(); j != door_list.end(); ) 
		{
			if ( bStartCheck )
			{
				if ( gPreferences.GetVisualRotateOrient() )
				{
					m_SelectedOrient = (*j)->GetOrientation();
				}
				SetDestConnectionID( (*j)->GetID() );				
				break;				
			}			

			if ( GetDestConnectionID() == (*j)->GetID() )
			{
				if ( bStartCheck )
				{
					break;
				}
				
				bStartCheck = true;	
				doorCount = 0;
			}					

			++j;		
			++doorCount;

			if ( doorCount > door_list.size() )
			{
				break;
			}
			
			if ( j == door_list.end() )
			{
				j = door_list.begin();
			}
		}
	}
}

void EditorTerrain::UnloadSelectedNode()
{		
	gSiegeEngine.NodeCache().RemoveSlotFromCache( m_DrawSelected );
}


void EditorTerrain::GetSelectedPosPoint( int & x, int & y )
{
	SiegePos spos;
	spos.pos = m_SelectedPos;
	spos.node = GetTargetNode();

	vector_3 screenPos = spos.ScreenPos();
	x = screenPos.x;
	y = screenPos.y;
}


void EditorTerrain::RotateSelectedNodeMesh( int x, int y )
{
	float scroll_magnitudeX = ( (float)x - (float)gEditorCamera.GetCameraXRotatePos() ) / 100.0f;
	
	matrix_3x3 rotateMatrix;
	rotateMatrix.v00 = COSF( scroll_magnitudeX );
	rotateMatrix.v01 = 0.0f;
	rotateMatrix.v02 = -SINF( scroll_magnitudeX );
	rotateMatrix.v10 = 0.0f;
	rotateMatrix.v11 = 1.0f;
	rotateMatrix.v12 = 0.0f;
	rotateMatrix.v20 = SINF( scroll_magnitudeX );
	rotateMatrix.v21 = 0;
	rotateMatrix.v22 = COSF( scroll_magnitudeX );

	m_SelectedOrient = m_SelectedOrient * rotateMatrix;
}


void EditorTerrain::ResetNodeCalcTimer()
{
	if ( !gPreferences.GetModeNodePlacement() || !GetSecondaryNode().IsValid() || !m_DrawSelected.IsValid() )
	{
		return;
	}
	else
	{
		SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( m_DrawSelected ) );
		SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
		if ( node.GetAmbientColor() != 0xffffffff && !GetSelectedSnapped() )
		{
			node.SetAmbientColor( fScaleDWORDColor( 0xffffffff, 1.0f ) );
			node.AccumulateStaticLighting();
		}
	}	

	m_SelectedCalcTime = gPreferences.GetVisualSnapDelay();
}


void EditorTerrain::UpdateNodeCalcTimer()
{
	m_SelectedCalcTime -= gWorldTime.GetSecondsElapsed();
	if ( m_SelectedCalcTime <= 0.0f && m_SelectedCalcTime != -FLOAT_MAX )
	{		
		m_SelectedCalcTime = 0.0f;		
		gEditorTerrain.CalculateNodePlacement();
		m_SelectedCalcTime = -FLOAT_MAX;
	}
}


// Sets the user selected node
void EditorTerrain::SetSelectedNode( siege::database_guid selected_node )
{
	m_selected_node = selected_node;
	ClearAllSelectedNodes();	
}

void EditorTerrain::SetSecondaryNode( siege::database_guid SecondaryNode )
{
	m_secondary_node = SecondaryNode;
	LoadSelectedNode();
}


// Clears all secondary selected nodes
void EditorTerrain::ClearAllSelectedNodes()
{
	m_selected_nodes.clear();
}


// Selects all available nodes
void EditorTerrain::SelectAllNodes()
{	
	FrustumNodeColl nodeColl = gSiegeEngine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator iNode;
	for( iNode = nodeColl.begin(); iNode != nodeColl.end(); ++iNode )
	{
		if ( GetSelectedNode() == UNDEFINED_GUID ) 
		{
			SetSelectedNode( (*iNode)->GetGUID() );
		}
		else 
		{		
			AddNodeAsSelected( (*iNode)->GetGUID() );				
		}
	}
}


void EditorTerrain::SelectAllNodesOfMesh( siege::database_guid meshGUID, bool bClear )
{
	if ( bClear )
	{
		SetSelectedNode( UNDEFINED_GUID );
	}

	FrustumNodeColl nodeColl = gSiegeEngine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator iNode;
	for( iNode = nodeColl.begin(); iNode != nodeColl.end(); ++iNode )
	{
		if ( (*iNode)->GetMeshGUID() == meshGUID ) 
		{		
			if ( GetSelectedNode() == UNDEFINED_GUID ) 
			{
				SetSelectedNode( (*iNode)->GetGUID() );
			}
			else 
			{		
				AddNodeAsSelected( (*iNode)->GetGUID() );				
			}
		}
	}	
}


bool EditorTerrain::IsNodeSelected( siege::database_guid nodeGuid, bool bUnselect )
{
	if ( nodeGuid == GetSelectedNode() )
	{
		m_selected_node = UNDEFINED_GUID;
		return true;
	}

	GuidVec::iterator i;	
	for ( i = m_selected_nodes.begin(); i != m_selected_nodes.end(); ++i )
	{
		if( nodeGuid == (*i) )
		{
			if ( bUnselect )
			{
				m_selected_nodes.erase( i );
			}
			return true;
		}
	}

	return false;
}


void EditorTerrain::RemoveNodeAsSelected( siege::database_guid nodeGuid )
{
	if ( nodeGuid == GetSelectedNode() )
	{
		m_selected_node = UNDEFINED_GUID;
		return;
	}

	GuidVec::iterator i;	
	for ( i = m_selected_nodes.begin(); i != m_selected_nodes.end(); ++i )
	{
		if ( nodeGuid == (*i) )
		{
			m_selected_nodes.erase( i );
			return;
		}
	}
}


// Go to a specified node from a string
void EditorTerrain::GotoNode( gpstring szGUID )
{	
	siege::database_guid nodeGUID( szGUID.c_str() );
	if ( !nodeGUID.IsValid() ) {
		MessageBox( gEditor.GetHWnd(), "Invalid GUID used for goto node", "Warning", MB_OK );
	}

	SiegeNodeHandle source_handle(gSiegeEngine.NodeCache().UseObject(nodeGUID));
	SiegeNode& source_node = source_handle.RequestObject(gSiegeEngine.NodeCache());						

	vector_3 location = source_node.GetCurrentCenter();
	gEditorCamera.SetPosition( (int)location.x, (int)location.y, (int)location.z );
}


void EditorTerrain::AddNodeAsSelected( siege::database_guid nodeGUID )
{
	if ( nodeGUID == GetSelectedNode() ) 
	{
		return;
	}

	std::vector< siege::database_guid >::iterator i;
	for ( i = m_selected_nodes.begin(); i != m_selected_nodes.end(); ++i ) 
	{
		if ( (*i) == nodeGUID ) 
		{
			m_selected_nodes.erase( i );
			return;
		}
	}

	m_selected_nodes.push_back( nodeGUID );
}
void EditorTerrain::PaintAddNodeAsSelected( siege::database_guid nodeGUID ) 
{
	if ( nodeGUID == GetSelectedNode() ) 
	{
		return;
	}

	std::vector< siege::database_guid >::iterator i;
	for ( i = m_selected_nodes.begin(); i != m_selected_nodes.end(); ++i ) 
	{
		if ( (*i) == nodeGUID ) 
		{			
			return;
		}
	}

	m_selected_nodes.push_back( nodeGUID );
}


void EditorTerrain::SetTargetNode( siege::database_guid targetNode ) 
{ 
	if ( !targetNode.IsValid() ) 
	{
		return;
	}

	m_target_node = targetNode;
	FuelHandleList fhTerrain = gEditorRegion.GetRegionHandle()->ListChildBlocks( 2, NULL, "siege_node_list" );
	if ( fhTerrain.size() == 1 ) 
	{
		FuelHandle fhNodes = fhTerrain.front();
		fhNodes->Set( "targetnode", targetNode.GetValue(), FVP_HEX_INT );	
	}

	siege::SiegeFrustum * pFrustum = gSiegeEngine.GetFrustum( gEditorRegion.GetFrustumId() );
	SiegePos spos;
	spos.node = targetNode;
	pFrustum->SetPosition( spos );

	EditorCamera& m_currentCamera	= gEditorCamera;

	// Tell the camera about the new target node	
	m_currentCamera.SetTargetPositionNode( targetNode );

	siege::SiegeCamera& camera	= gSiegeEngine.GetCamera();

	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( targetNode ) );
	SiegeNode& target_node = handle.RequestObject( gSiegeEngine.NodeCache() );

	// Convert both the camera and target positions into our new node's space
	vector_3 newtargetposition	= target_node.WorldToNodeSpace( camera.GetTargetPosition() );
	vector_3 newcameraposition	= target_node.WorldToNodeSpace( camera.GetCameraPosition() );

	// Get the angles
	float orbit, azimuth;
	
	// Calculate the azimuth
	// NOTES:  The method of calculating the azimuth is rather simple, becuase
	// we know that the camera distance is the hypotenuse of a right triangle whose
	// back leg is formed by the distance in Y between the camera and the target.
	// Using this information, the angle calculation is a simple ArcSine.
	azimuth	= ASin( ((newcameraposition - newtargetposition).y / m_currentCamera.GetDistance()) );

	// Calculate the orbit
	// NOTES:  The method of calculating the orbit is quite a bit more difficult.
	// First we must take our base orbit vector and rotate it by the orientation of the
	// new target node so that we have a vector to derive our angle from.  Using the X and
	// Z basis components of our rotated vector, we can get an angle measurement using
	// the ArcTangent, but we still do not have a correct answer.  We must use our X and Z
	// basis components to figure out which quadrant the angle is measured from, and
	// compensate for that quadrant to get an angle from 0 - 2Pi.  Since we know that this
	// angle is the difference in rotation, our final step is to subtract it from our
	// existing orbit.  Voila!
	vector_3 orbitbase( 0, 0, -1 );
	orbitbase	= target_node.GetTransposeOrientation() * orbitbase;

	// Floating point inaccuracy makes this step neccesary
	if( IsZero( orbitbase.x ) )
	{
		orbitbase.x = 0.0f;
	}
	if( IsZero( orbitbase.z ) )
	{
		orbitbase.z = 0.0f;
	}

	// Do quadrant calculations
	if( IsZero( orbitbase.z ) )
	{
		if( orbitbase.x > 0.0f )
		{
			orbit	= (RealPi/2.0f);
		}
		else
		{
			orbit	= (RealPi + (RealPi/2.0f));
		}
	}
	else
	{
		float ratio = orbitbase.x / fabsf( orbitbase.z );
		orbit	= ATan( ratio );

		if( orbitbase.x < 0.0f )
		{
			if( orbitbase.z < 0.0f )
			{
				orbit += (2*RealPi);
			}
			else
			{
				orbit = RealPi - orbit;
			}
		}
		else
		{
			if( orbitbase.z > 0.0f )
			{
				orbit = RealPi - orbit;
			}
		}
	}

	// Set the new angles
	m_currentCamera.SetTargetPosition( newtargetposition );
	m_currentCamera.SetOrbitAngle( m_currentCamera.GetOrbit() - orbit );
	m_currentCamera.SetAzimuthAngle( azimuth );

	gEditorCamera.Update();

	gEditorRegion.SetRegionDirty();	
}


void EditorTerrain::SetSourceDoors( siege::database_guid SourceNode )
{
	// Grab access to the siege engine
	SiegeEngine & engine = gSiegeEngine;

	m_source_doors.clear();
	
	SiegeNode * pNode = gSiegeEngine.IsNodeValid( SourceNode );
	if ( pNode )
	{
		const SiegeMesh& bmesh = pNode->GetMeshHandle().RequestObject(engine.MeshCache());
		
		const siege::SiegeMesh::SiegeMeshDoorList& hDoors = bmesh.GetDoors();
		for( siege::SiegeMesh::SiegeMeshDoorConstIter i = hDoors.begin(); i != hDoors.end(); ++i )
		{
			char szID[1024];
			sprintf( szID, "%d", (*i)->GetID() );
			m_source_doors.push_back( szID );
		}
	}
}


void EditorTerrain::SetDestDoors( siege::database_guid DestNode )
{
	// Grab access to the siege engine
	SiegeEngine & engine = gSiegeEngine;
	m_dest_doors.clear();
	SetSecondaryNode( DestNode );

	if ( DestNode.IsValid() ) {

		SiegeMeshHandle mesh_handle(engine.MeshCache().UseObject(DestNode));

		const SiegeMesh& bmesh = mesh_handle.RequestObject(engine.MeshCache());
	
		const siege::SiegeMesh::SiegeMeshDoorList& hDoors = bmesh.GetDoors();
		for( siege::SiegeMesh::SiegeMeshDoorConstIter i = hDoors.begin(); i != hDoors.end(); ++i )
		{
			char szID[1024];
			sprintf( szID, "%d", (*i)->GetID() );
			m_dest_doors.push_back( szID );
		}
	}
}


void EditorTerrain::AttachSiegeNodes( int iSource, int iDest )
{	
	//gCommandManager.BeginActionSet();
	AttachSiegeNodes( m_selected_node, m_secondary_node, siege::database_guid(), iSource, iDest );
	//gCommandManager.EndActionSet();
}

void EditorTerrain::AttachSiegeNodes(	siege::database_guid nodeGUID, siege::database_guid meshGUID,
										siege::database_guid destGUID, int iSource, int iDest )	
{
	if (( !meshGUID.IsValid() ) || ( !nodeGUID.IsValid() )) {
		return;
	}	
	
	database_guid	inGUID;
	int				numDoors	= 0;

	// Grab access to the siege engine
	SiegeEngine & engine = gSiegeEngine;

	{
		SiegeNodeHandle handle(engine.NodeCache().UseObject(nodeGUID));
		SiegeNode& source_node = handle.RequestObject(engine.NodeCache());
		
		const SiegeMesh& source_bmesh = source_node.GetMeshHandle().RequestObject(engine.MeshCache());

		siege::SiegeMeshDoor *source_mesh_door = source_bmesh.GetDoorByID( iSource );
		if ( !source_mesh_door )
		{
			return;
		}
		
		// Generate a random GUID for the node
		if ( destGUID == UNDEFINED_GUID )
		{
			gpstring szRandom = GenerateRandomGUID();
			inGUID = database_guid(szRandom.c_str());

			while( engine.NodeCache().ObjectExistsInCache( inGUID ) )
			{
				szRandom	= GenerateRandomGUID();
				inGUID		= database_guid( szRandom.c_str() );
			}
		}				
		
		// Create the node and its mesh
		SiegeNodeHandle dest_handle(engine.NodeCache().UseObject(inGUID));
		SiegeNode& dest_node = dest_handle.CreateObject(engine.NodeCache());

		// Need to set the correct texture set info $$$
		dest_node.SetTextureSetAbbr(GetNodeSetCharacters());
		dest_node.SetTextureSetVersion("");
		
		dest_node.SetGUID(inGUID);
		dest_node.SetMeshGUID( meshGUID );
		dest_node.SetRegionID( (int)gEditorRegion.GetRegionGUID() );
		
		const SiegeMesh& dest_bmesh = dest_node.GetMeshHandle().RequestObject(engine.MeshCache());

		dest_node.SetMinimumBounds( dest_bmesh.GetMinimumCorner() );
		dest_node.SetMaximumBounds( dest_bmesh.GetMaximumCorner() );
		dest_node.SetBoundsCamera( true );
		dest_node.SetOccludesLight( true );
		dest_node.SetSection( 1 );

		siege::SiegeMeshDoor *dest_mesh_door = dest_bmesh.GetDoorByID( iDest );
		if ( !dest_mesh_door )
		{
			return;
		}

		// Check to see if there is already a door where the user is try to place another door
		// if there is, remove that previous door
		SiegeNodeDoor *sdoor = source_node.GetDoorByID( iSource );
		if ( sdoor != 0 ) {
			
			// Is the connecting GUID the target node?
			if ( engine.NodeWalker().TargetNodeGUID() == sdoor->GetNeighbour() ) {
				MessageBox( gEditor.GetHWnd(), "You cannot change the target node.", "Node Attach", MB_OK );
				return;
			}

			// Delete the node that is in that place
			DeleteSiegeNode( sdoor->GetNeighbour() );			
		}

		// Get the Number of Doors the new node		
		const	siege::SiegeMesh::SiegeMeshDoorList& hDoors	= dest_bmesh.GetDoors();
		numDoors = hDoors.size();

		// Connect the Doors		
		source_node.AddDoor( source_mesh_door->GetID(), source_mesh_door->GetCenter(),
							 source_mesh_door->GetOrientation(), inGUID, iDest, true );
		dest_node.AddDoor( dest_mesh_door->GetID(), dest_mesh_door->GetCenter(), 
						   dest_mesh_door->GetOrientation(), nodeGUID, iSource, true );	 


		dest_node.SetIsLoadedPhysical( true );
	}

	gSiegeEngine.GetFrustum( gEditorRegion.GetFrustumId() )->RequestUpdate();
	gSiegeEngine.UpdateFrustums( 0.0f, true );
	
	SiegeNode* pDestNode = gSiegeEngine.IsNodeValid( inGUID );
	SiegeNode* pSourceNode = gSiegeEngine.IsNodeValid( nodeGUID );
	if ( pDestNode && pSourceNode )
	{
		m_last_added = pDestNode->GetGUID();
		
		// Attach this new node to our logical tree
		gEditor.GetLogicalNodeCreator()->AttachSiegeNode( engine, inGUID );	

		// Update the node's lighting information
		pDestNode->SetAmbientColor( pSourceNode->GetAmbientColor() );
		pDestNode->SetActorAmbientColor( pSourceNode->GetActorAmbientColor() );
		pDestNode->SetObjectAmbientColor( pSourceNode->GetObjectAmbientColor() );
		gDefaultRapi.AddTextureReference( pSourceNode->GetEnvironmentMap() );
		pDestNode->SetEnvironmentMap( pSourceNode->GetEnvironmentMap() );		
		UpdateNodeLighting( inGUID );	
	}

	gEditorRegion.SetRegionDirty();
}


DWORD EditorTerrain::AddLnoPath( siege::database_guid nodeGuid )
{
	gpstring path = gEditorRegion.GetRegionHandle()->GetAddress();
	stringtool::ReplaceAllChars( path, ':', '\\' );
	stringtool::AppendTrailingBackslash( path );

	gpassert( !path.empty() );
	path.append( "terrain_nodes\\siege_nodes.lnc" );
	
	return gSiegeEngine.NodeDatabase().ReferenceLogicalFile( path, nodeGuid );	
}


void EditorTerrain::AutoNodeConnection()
{
	if ( gEditorRegion.GetRegionHandle().IsValid() == false ) {
		return;
	}

	// Grab access to the siege engine
	SiegeEngine & engine = gSiegeEngine;

	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator i;
	for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
	{	
		AutoNodeConnect( (*i)->GetGUID() );
	}	

	gEditorRegion.SetRegionDirty();
}


void EditorTerrain::AutoNodeConnectSelected()
{
	if ( gEditorRegion.GetRegionHandle().IsValid() == false ) {
		return;
	}

	// Grab access to the siege engine
	SiegeEngine & engine = gSiegeEngine;

	// $ NSO
	// gSiegeEngine.NodeWalker().SetTargetNode( GetTargetNode() );
	// gSiegeEngine.NodeWalker().SetNewTargetNode( GetTargetNode() );

	engine.NodeWalker().RenderVisibleNodes();

	if ( GetSelectedNode().IsValid() )
	{
		AutoNodeConnect( GetSelectedNode() );

		// Go through subselected nodes and do the same thing
		GuidVec::iterator i;
		for( i = m_selected_nodes.begin(); i != m_selected_nodes.end(); ++i )
		{
			AutoNodeConnect( *i );
		}
	}

	gEditorRegion.SetRegionDirty();
}


void EditorTerrain::AutoNodeConnect( siege::database_guid nodeGUID )
{
	// Grab access to the siege engine
	SiegeEngine & engine = gSiegeEngine;

	SiegeNodeHandle source_handle(engine.NodeCache().UseObject(nodeGUID));
	SiegeNode& source_node = source_handle.RequestObject(engine.NodeCache());

	const SiegeMesh& source_bmesh = source_node.GetMeshHandle().RequestObject(engine.MeshCache());
	
	const siege::SiegeMesh::SiegeMeshDoorList& door_list = source_bmesh.GetDoors();
	siege::SiegeMesh::SiegeMeshDoorConstIter j;	
	
	for ( j = door_list.begin(); j != door_list.end(); ++j ) {
		SiegeNodeDoor *sdoor = source_node.GetDoorByID( (*j)->GetID() );
		if ( sdoor != 0 ) {
			continue;
		}
		
		vector_3 door1_center = (*j)->GetCenter();
		door1_center = source_node.NodeToWorldSpace( door1_center );

		FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
		FrustumNodeColl::iterator iNode;
		for( iNode = nodeColl.begin(); iNode != nodeColl.end(); ++iNode )
		{
			if ( nodeGUID == (*iNode)->GetGUID() ) 
			{
				continue;
			}
			
			const SiegeMesh& dest_bmesh = (*iNode)->GetMeshHandle().RequestObject(engine.MeshCache());
			
			const siege::SiegeMesh::SiegeMeshDoorList&		door_list2 = dest_bmesh.GetDoors();
			siege::SiegeMesh::SiegeMeshDoorConstIter			l;	
					
			for ( l = door_list2.begin(); l != door_list2.end(); ++l ) {
				SiegeNodeDoor *sdoor2 = (*iNode)->GetDoorByID( (*l)->GetID() );
				if ( sdoor2 != 0 ) 
				{
					continue;
				}
				vector_3 door2_center = (*l)->GetCenter();
				door2_center = (*iNode)->NodeToWorldSpace( door2_center );

				float dx	=	door1_center.GetX() - door2_center.GetX();
				float dy	=	door1_center.GetY() - door2_center.GetY();
				float dz	=	door1_center.GetZ() - door2_center.GetZ();

				if ( (( dx < 0.025 ) && ( dx > -0.025 )) &&
					 (( dy < 0.025 ) && ( dy > -0.025 )) &&
					 (( dz < 0.025 ) && ( dz > -0.025 )) ) {						
					ConnectSiegeNodes( nodeGUID, (*j)->GetID(), (*iNode)->GetGUID(), (*l)->GetID() );
				}
			}
		}
	}

	gEditorRegion.SetRegionDirty();
}


void EditorTerrain::AutoStitch( int startingId, const gpstring & sRegion, StitchedVector & stitches )
{	
	GuidVec selectedNodes = GetSelectedNodes();	
	siege::database_guid selectedGuid = GetSelectedNode();

	siege::database_guid nextNode = GetSelectedNode();
	while ( nextNode.IsValid() )
	{		
		SiegeNode * pNode = gSiegeEngine.IsNodeValid( nextNode );	
		if ( pNode )
		{
			nextNode = UNDEFINED_GUID;			

			const SiegeMesh& bmesh = pNode->GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );			
			siege::SiegeMesh::SiegeMeshDoorConstIter bdi;
			for ( bdi = bmesh.GetDoors().begin(); bdi != bmesh.GetDoors().end(); ++bdi ) 
			{		
				SiegeNodeDoor * pDoor = pNode->GetDoorByID( (*bdi)->GetID() );				

				if ( !pDoor || !gSiegeEngine.IsNodeValid( pDoor->GetNeighbour() ) )
				{					
					stitches.push_back( startingId );
					gStitchHelper.AddNodeToStitchHelper( pNode->GetGUID(), startingId, (*bdi)->GetID(), sRegion );				
					startingId++;
				}
				else if ( !nextNode.IsValid() )
				{					
					if ( IsNodeSelected( pDoor->GetNeighbour(), true ) )
					{
						nextNode = pDoor->GetNeighbour();						
					}
				}
			}	
			
			RemoveNodeAsSelected( pNode->GetGUID() );
		}		
	} 

	SetSelectedNode( selectedGuid );
	m_selected_nodes = selectedNodes;	
}


void EditorTerrain::SetSelectedSourceDoorID( int id )
{ 
	m_source_door_id = id;
	
	if ( Dialog_Node_Matches::DoesSingletonExist() )
	{
		gDialogNodeMatches.Refresh();
	}
}


void EditorTerrain::ConnectSiegeNodes( siege::database_guid sourceGUID, int iSource, 
									   siege::database_guid destGUID, int iDest ) 
{
	if ( !sourceGUID.IsValid() || !destGUID.IsValid() ) 
	{
		return;
	}

	SiegeEngine & engine = gSiegeEngine;

	SiegeNodeHandle source_handle(engine.NodeCache().UseObject(sourceGUID));
	SiegeNode& source_node = source_handle.RequestObject(engine.NodeCache());						

	const SiegeMesh& source_bmesh = source_node.GetMeshHandle().RequestObject(engine.MeshCache());

	siege::SiegeMeshDoor *source_mesh_door = source_bmesh.GetDoorByID( iSource );

	SiegeNodeHandle dest_handle(engine.NodeCache().UseObject(destGUID));
	SiegeNode& dest_node = dest_handle.RequestObject(engine.NodeCache());						

	SiegeMesh& dest_bmesh = dest_node.GetMeshHandle().RequestObject(engine.MeshCache());

	siege::SiegeMeshDoor *dest_mesh_door = dest_bmesh.GetDoorByID( iDest );
		
	// Connect the Doors
	SiegeNodeDoor * pSourceDoor = source_node.GetDoorByID( source_mesh_door->GetID() );
	if ( pSourceDoor && pSourceDoor->GetConnection() )
	{
		gperrorf( ( "Auto-connect failure.  Tried to connect destination node: 0x%08X, door %d - to - source node: 0x%x, door %d.  A connection already exists there, this means that the doors are too close together or the nodes are overlapping each other", 
					dest_node.GetGUID().GetValue(), dest_mesh_door->GetID(), source_node.GetGUID().GetValue, source_mesh_door->GetID() ) );
		return;
	}

	SiegeNodeDoor * pDestDoor = dest_node.GetDoorByID( dest_mesh_door->GetID() );
	if ( pDestDoor && pDestDoor->GetConnection() )
	{
		gperrorf( ( "Auto-connect failure.  Tried to connect destination node: 0x%08X, door %d - to - source node: 0x%x, door %d.  A connection already exists there, this means that the doors are too close together or the nodes are overlapping each other", 
					dest_node.GetGUID().GetValue(), dest_mesh_door->GetID(), source_node.GetGUID().GetValue, source_mesh_door->GetID() ) );		
		return;
	}
	
	source_node.AddDoor( source_mesh_door->GetID(), source_mesh_door->GetCenter(),
						 source_mesh_door->GetOrientation(), destGUID, iDest, true );
	dest_node.AddDoor( dest_mesh_door->GetID(), dest_mesh_door->GetCenter(), 
					   dest_mesh_door->GetOrientation(), sourceGUID, iSource, true );	 

	dest_node.SetNumLitVertices( dest_bmesh.GetNumberOfVertices() );
	
	// Recalculate the connections for both nodes to maintain the logical tree
	gEditor.GetLogicalNodeCreator()->CalculateNodalConnections( engine, sourceGUID );
	gEditor.GetLogicalNodeCreator()->CalculateNodalConnections( engine, destGUID );

	gEditorRegion.SetRegionDirty();
}


void EditorTerrain::DeleteSiegeNode( database_guid nodeGUID )
{	
	if ( nodeGUID == GetTargetNode() ) {
		return;
	}

	// Grab access to the siege engine
	SiegeEngine & engine = gSiegeEngine;	

	// Block this up so that the handles get released before we remove the slot
	{
		SiegeNodeHandle handle(engine.NodeCache().UseObject(nodeGUID));
		SiegeNode& node = handle.RequestObject(engine.NodeCache());	

		/*
		GoidColl::iterator o;
		GoidColl occupants;
		gGoDb.GetNodeOccupants( node.GetGUID(), occupants );
		
		for ( o = occupants.begin(); o != occupants.end(); ++o )
		{
			gEditorObjects.DeleteObject( *o );
		}

		std::vector< DWORD > gizmos;
		std::vector< DWORD >::iterator iGizmo;
		DecalIndexList decalList = node.GetDecalIndexList();
		DecalIndexList::iterator iDecal;
		for ( iDecal = decalList.begin(); iDecal != decalList.end(); ++iDecal )
		{
			gGizmoManager.GetGizmosOfType( GIZMO_DECAL, gizmos );
			for ( iGizmo = gizmos.begin(); iGizmo != gizmos.end(); ++iGizmo )
			{
				Gizmo * pGizmo = gGizmoManager.GetGizmo( *iGizmo );
				if( pGizmo && pGizmo->index == *iDecal )
				{
					gGizmoManager.RemoveGizmo( pGizmo->id );
					break;
				}				
			}
		}
		*/

		gGizmoManager.RemoveGizmosFromNode( node.GetGUID() );
	
		// $ NSO
		// gSiegeEngine.NodeWalker().RemoveNodeFromActiveList( &node );				
	
		// Remove from Stitch Helper, if applicable
		gEditor.GetStitchHelper()->DeleteNodeFromStitchIndex( nodeGUID );

		/*
		if( !RemoveNodeFromGAS( node.GetGUID() ) ) {
			return;
		}
		*/

		DeselectLogicalIdsForNode( node.GetGUID() );		

		SiegeNodeDoorList door_list = node.GetDoors();
		SiegeNodeDoorIter i;	
		
		SiegeNodeHandle target_handle(engine.NodeCache().UseObject(engine.NodeWalker().TargetNodeGUID()));
		SiegeNode& target_node = target_handle.RequestObject(engine.NodeCache());						

		for ( i = door_list.begin(); i != door_list.end(); ++i ) 
		{		
			/*
			{
				CommandAddNode * pCAction = new CommandAddNode( ACTION_ADDNODE,
																(*i)->GetNeighbour(), 
																node.GetMeshGUID(), 
																node.GetGUID(),
																(*i)->GetNeighbourID(),
																(*i)->GetID() );
				gCommandManager.InsertAction( (CommandAction *)pCAction );				
			}
			*/

			database_guid attachedNode = (*i)->GetNeighbour();

			SiegeNodeHandle attached_handle(engine.NodeCache().UseObject(attachedNode));
			SiegeNode& attached_node = attached_handle.RequestObject(engine.NodeCache());						

			//RemoveDoorFromGAS( (*i)->GetNeighbour(), (*i)->GetNeighbourID() );		
			attached_node.RemoveDoor( (*i)->GetNeighbourID() );				
			node.RemoveDoor( (*i)->GetID() );		
			
			/*
			if ( attached_node.GetLastVisited() != target_node.GetLastVisited() ) 
			{
				/*
				{
					CommandAddNode * pCAction = new CommandAddNode( ACTION_ADDNODE,
																	node.GetGUID(), 
																	attached_node.GetMeshGUID(), 
																	attached_node.GetGUID(),
																	(*i)->GetID(),
																	(*i)->GetNeighbourID() );
					gCommandManager.InsertAction( (CommandAction *)pCAction );				
				}
				*/
			/*
				DeleteSiegeNode( attached_node.GetGUID() );				
			}
			*/
		}
	}

	// Make sure that we don't stay selected to a deleted node
	if( m_selected_node == nodeGUID )
	{
		m_selected_node = UNDEFINED_GUID;
	}

	// Remove the node from the logical tree
	gEditor.GetLogicalNodeCreator()->DetachSiegeNode( nodeGUID );

	gSiegeEngine.GetFrustum( gEditorRegion.GetFrustumId() )->RequestUpdate();
	//gSiegeEngine.UpdateFrustums( 0.0f, true );		

	gEditorRegion.SetRegionDirty();
}


void EditorTerrain::DeleteSelectedNodes()
{
	//gCommandManager.BeginActionSet();

	if ( GetSelectedNode() != UNDEFINED_GUID ) {
		DeleteSiegeNode( GetSelectedNode() );
	}
	GuidVec::iterator i;
	for ( i = m_selected_nodes.begin(); i != m_selected_nodes.end(); ++i ) {
		DeleteSiegeNode( *i );
	}

	ClearAllSelectedNodes();

	//gCommandManager.EndActionSet();
}


/*
void EditorTerrain::AddDoorToGAS( siege::database_guid sourceGUID, int doorID, siege::database_guid destGUID, int destID )
{	
	FuelHandleList fhlTerrain = gEditorRegion.GetRegionHandle()->ListChildBlocks( -1, "terrain_nodes" );
	if ( fhlTerrain.size() == 0 ) {
		fhlTerrain = gEditorRegion.GetRegionHandle()->ListChildBlocks( -1, NULL, "siege_node_list" );
	}
	FuelHandleList::iterator k;
	gpstring faddress;
	for ( k = fhlTerrain.begin(); k != fhlTerrain.end(); ++k ) {
		faddress = (*k)->GetAddress();		
	}
	FuelHandle fhNodes( faddress );
	
	FuelHandleList fblNodes = fhNodes->ListChildBlocks( 2, "snode" );
	FuelHandleList::iterator i;	
	for ( i = fblNodes.begin(); i != fblNodes.end(); ++i ) {
		gpstring szGUID;		
		(*i)->Get( "guid", szGUID );
		database_guid guid1( szGUID.c_str() );		
		if ( guid1 == sourceGUID ) {
			FuelHandle fhDoor = (*i)->CreateChildBlock( "DOOR*" );
			fhDoor->Set( "ID", doorID );
			fhDoor->Set( "FarGUID", destGUID.GetValue(), FVP_HEX_INT );
			fhDoor->Set( "FarDoor", destID );
			return;
		}		
	}
}


void EditorTerrain::RemoveDoorFromGAS( siege::database_guid nodeGUID, int iSource )
{	
	FuelHandle hTerrainNodeDirectory = gEditorRegion.GetRegionHandle()->GetChildBlock( "terrain_nodes" );
	FuelHandleList hTerrainNodeCollectionsList = hTerrainNodeDirectory->ListChildBlocks( 1, "terrain_nodes" );
	
	// current convention has it that there is only one .gas file with the whole terrain node collection for said region
	if( hTerrainNodeCollectionsList.size() != 1 ) {
		return;
	}

	FuelHandle hTerrainNodeCollection = hTerrainNodeCollectionsList.front();

	FuelHandleList NodesList = hTerrainNodeCollection->ListChildBlocks( 1, "snode" );
	if( NodesList.empty() ) {
		gpassert( 0 );	// there should be something here if we want to delete it, but there isn't
		return;
	}

	FuelHandleList::iterator i;
	for ( i = NodesList.begin(); i != NodesList.end(); ++i )
	{
		gpstring szGUID;
		(*i)->Get( "guid", szGUID );
		siege::database_guid testNode( szGUID.c_str() );		
		
		if ( nodeGUID == testNode )
		{
			FuelHandleList fblDoors = (*i)->ListChildBlocks();
			FuelHandleList::iterator	j;
			for( j = fblDoors.begin(); j != fblDoors.end(); ++j )
			{
				int ID;
				(*j)->Get( "id", ID );
				if( ID == iSource )
				{
					// Delete the child
					(*i)->DestroyChildBlock( (*j) );		
					return;
				}
			}
		}
	}	
	return;
}


void EditorTerrain::AddNodeToGAS( UINT32 nodeGUID, UINT32 meshGUID,
							  const gpstring& texSetAbbr, const gpstring& texSetVers, int numDoors ) 
{	
	FuelHandle hTerrainNodeDirectory = gEditorRegion.GetRegionHandle()->GetChildBlock( "terrain_nodes" );
	FuelHandleList hTerrainNodeCollectionsList = hTerrainNodeDirectory->ListChildBlocks( 1, "terrain_nodes" );
	
	// current convention has it that there is only one .gas file with the whole terrain node collection for said region
	if( hTerrainNodeCollectionsList.size() != 1 )
	{
		return;
	}

	FuelHandle hTerrainNodeCollection = hTerrainNodeCollectionsList.front();

	char szName[256] = "";
	sprintf( szName, "%x", nodeGUID );
	FuelHandle fhTerrainNode	= hTerrainNodeCollection->CreateChildBlock( szName );	

	fhTerrainNode->Set	( "texsetabbr",	texSetAbbr );
	fhTerrainNode->Set	( "texsetver",	texSetVers );
	fhTerrainNode->Set	( "numdoors",	numDoors );

	fhTerrainNode->SetType( "snode" );
	fhTerrainNode->Set	( "guid",		nodeGUID, FVP_HEX_INT );
	fhTerrainNode->Set	( "mesh_guid",	meshGUID, FVP_HEX_INT );


	gWorldMap.OnEditorAddedNodeToMap( gEditorRegion.GetRegionGUID(), nodeGUID );
}


bool EditorTerrain::RemoveNodeFromGAS( siege::database_guid nodeGUID )
{
	FuelHandle hTerrainNodeDirectory = gEditorRegion.GetRegionHandle()->GetChildBlock( "terrain_nodes" );
	FuelHandleList hTerrainNodeCollectionsList = hTerrainNodeDirectory->ListChildBlocks( 1, "terrain_nodes" );
	
	// current convention has it that there is only one .gas file with the whole terrain node collection for said region
	if( hTerrainNodeCollectionsList.size() != 1 ) {
		return( false );
	}

	FuelHandle hTerrainNodeCollection = hTerrainNodeCollectionsList.front();

	FuelHandleList NodesList = hTerrainNodeCollection->ListChildBlocks( 1, "snode" );
	if( NodesList.empty() ) {
		gpassert( 0 );	// there should be something here if we want to delete it, but there isn't
		return( false );
	}

	FuelHandleList::iterator i;
	for ( i = NodesList.begin(); i != NodesList.end(); ++i )
	{
		gpstring szGUID;
		(*i)->Get( "guid", szGUID );
		siege::database_guid testNode( szGUID.c_str() );		
		
		if ( nodeGUID == testNode )
		{
			// this is the only way you can exit with success
			return( hTerrainNodeCollection->DestroyChildBlock( (*i) ) );
		}
	}
	
	// gpassert( 0 );
	return( false );
}
*/

// Update the selected node lighting in the LNC
void EditorTerrain::UpdateSelectedNodeLighting()
{
	if ( GetSelectedNode() == UNDEFINED_GUID )
	{
		return;
	}

	// Get the engine
	SiegeEngine & engine = gSiegeEngine;

	// Update light for the currently selected node
	engine.LightDatabase().UpdateLighting( GetSelectedNode() );

	// Accumulate static lights for the currently selected node
	SiegeNodeHandle handle( engine.NodeCache().UseObject( GetSelectedNode() ) );
	SiegeNode& node = handle.RequestObject( engine.NodeCache() );

	node.AccumulateStaticLighting();

	// Go through subselected nodes and do the same thing
	std::vector< siege::database_guid >::iterator i;
	for( i = m_selected_nodes.begin(); i != m_selected_nodes.end(); ++i )
	{
		// Update lighting
		engine.LightDatabase().UpdateLighting( (*i) );

		// Accumulate static
		SiegeNodeHandle shandle( engine.NodeCache().UseObject( (*i) ) );
		SiegeNode& snode = shandle.RequestObject( engine.NodeCache() );

		snode.AccumulateStaticLighting();
	}

	gEditorRegion.SetRegionDirty();
}


// Update a specified node's lighting in the LNC
void EditorTerrain::UpdateNodeLighting( siege::database_guid nodeGUID ) 
{
	// Get the engine
	SiegeEngine & engine = gSiegeEngine;

	// Update light for the currently selected node
	engine.LightDatabase().UpdateLighting( nodeGUID );

	// Accumulate static lights for the currently selected node
	SiegeNodeHandle handle( engine.NodeCache().UseObject( nodeGUID ) );
	SiegeNode& node = handle.RequestObject( engine.NodeCache() );

	node.AccumulateStaticLighting();

	gEditorRegion.SetRegionDirty();
}


// Update all node lighting
void EditorTerrain::UpdateAllNodeLighting()
{
	SiegeEngine & engine = gSiegeEngine;
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator iNode;
	for( iNode = nodeColl.begin(); iNode != nodeColl.end(); ++iNode )
	{			
		UpdateNodeLighting( (*iNode)->GetGUID() );		
	}	
}


void EditorTerrain::NodeCaps( bool bVisible )
{
	FuelHandle hEditor( "config:editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hGroups = hEditor->GetChildBlock( "mesh_groups" );				
		if ( hGroups.IsValid() )
		{			
			FuelHandle hCaps = hGroups->GetChildBlock( "caps" );
			if ( hCaps )
			{
				if ( hCaps->FindFirstKeyAndValue() )
				{
					gpstring sKey;
					gpstring sValue;
					int index = 0;
					bool bClear = true;
					while ( hCaps->GetNextKeyAndValue( sKey, sValue ) )
					{						
						sValue += ".sno";
						gpstring sMesh = gComponentList.GetNodeGUID( (char *)sValue.c_str() );
						if ( !sMesh.empty() )
						{
							siege::database_guid mesh_guid( sMesh.c_str() );
							gEditorTerrain.SelectAllNodesOfMesh( mesh_guid, bClear );
							bClear = false;						
						}
					}
				}	
								
				if ( bVisible )
				{
					gEditorTerrain.HideSelectedNodes();	
				}
				else
				{
					GuidVec guid_vec = gEditorTerrain.GetSelectedNodes();
					guid_vec.push_back( GetSelectedNode() );
					GuidVec::iterator i;
					for ( i = guid_vec.begin(); i != guid_vec.end(); ++i ) 
					{
						SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( *i ) );
						SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );		
						node.SetVisible( true );		
					}
				}

				gEditorTerrain.ClearAllSelectedNodes();
			}
		}
	}	
}


// Set Selected Nodes to a selected set
void EditorTerrain::SetTerrainNodeSet( gpstring sSet, gpstring sVersion )
{
	if ( GetSelectedNode() == UNDEFINED_GUID ) {
		return;
	}

	// Get the engine
	SiegeEngine & engine = gSiegeEngine;
	
	// Accumulate static lights for the currently selected node
	SiegeNodeHandle handle( engine.NodeCache().UseObject( GetSelectedNode() ) );
	SiegeNode& node = handle.RequestObject( engine.NodeCache() );
	
	node.SetTextureSetAbbr( sSet );
	node.SetTextureSetVersion( sVersion );	
	node.SetMeshGUID( node.GetMeshGUID() );

	// Go through subselected nodes and do the same thing
	GuidVec::iterator i;
	for( i = m_selected_nodes.begin(); i != m_selected_nodes.end(); ++i )
	{
		SiegeNodeHandle shandle( engine.NodeCache().UseObject( (*i) ) );	
		SiegeNode& snode = shandle.RequestObject( engine.NodeCache() );

		snode.SetTextureSetAbbr( sSet );
		snode.SetTextureSetVersion( sVersion );		
		snode.SetMeshGUID( snode.GetMeshGUID() );
	}

	gEditorRegion.SetRegionDirty();
}



void EditorTerrain::SetSelectedNodeFadeSettings( int nodesection, int nodelevel, int nodeobject )
{
	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( GetSelectedNode() ) );
	SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
	node.SetSection( nodesection );
	node.SetLevel( nodelevel );
	node.SetObject( nodeobject );
	
	GuidVec::iterator i;
	for ( i = m_selected_nodes.begin(); i != m_selected_nodes.end(); ++i ) 
	{
		SiegeNodeHandle shandle( gSiegeEngine.NodeCache().UseObject( (*i) ) );	
		SiegeNode& snode = shandle.RequestObject( gSiegeEngine.NodeCache() );

		snode.SetSection( nodesection );
		snode.SetLevel( nodelevel );
		snode.SetObject( nodeobject );
	}

	gEditorRegion.SetRegionDirty();
}


void EditorTerrain::ShowAllNodes( bool bShow )
{	
	siege::SiegeEngine& engine = gSiegeEngine;
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator i;
	for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
	{
		(*i)->SetVisible( bShow );
		(*i)->SetNodeAlpha( 255 );
	}			
}


void EditorTerrain::HideSelectedNodes()
{
	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( GetSelectedNode() ) );
	SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
	node.SetVisible( false );

	GuidVec::iterator i;
	for ( i = m_selected_nodes.begin(); i != m_selected_nodes.end(); ++i ) 
	{
		SiegeNodeHandle shandle( gSiegeEngine.NodeCache().UseObject( (*i) ) );	
		SiegeNode& snode = shandle.RequestObject( gSiegeEngine.NodeCache() );
		snode.SetVisible( false );		
	}
}


void EditorTerrain::SetAmbientColor( DWORD dwAmbient, DWORD dwObjectAmbient, DWORD dwActorAmbient )
{
	// Get the engine
	siege::SiegeEngine& engine			= gSiegeEngine;

	FuelHandleList fhlTerrain = gEditorRegion.GetRegionHandle()->ListChildBlocks( -1, NULL, "siege_node_list" );
	if ( fhlTerrain.size() != 0 ) {
		gpstring faddress = (*fhlTerrain.begin())->GetAddress();
		FuelHandle fhNodes( faddress );

		FuelHandleList fhlNodes	=	fhNodes->ListChildBlocks( 2, "snode" );
		FuelHandleList::iterator i;	
		for ( i = fhlNodes.begin(); i != fhlNodes.end(); ++i ) {
			gpstring szGUID;
			(*i)->Get( "guid", szGUID );
			database_guid guid( szGUID.c_str() );	
			SiegeNode* pNode = gSiegeEngine.IsNodeValid( guid );
		
			if ( !pNode )
			{
				gperrorf(( "SiegeEditor found a bad node.  This node will not be saved out: 0x%08X", guid.GetValue() ));
				continue;
			}
			
			pNode->SetAmbientColor( dwAmbient );
			pNode->SetObjectAmbientColor( dwObjectAmbient );
			pNode->SetActorAmbientColor( dwActorAmbient );
			pNode->AccumulateStaticLighting();						
		}		
	}

	gEditorRegion.SetRegionDirty();
}


void EditorTerrain::SetEnvironmentMap( gpstring sTexture )
{
	gpstring fullenvmapname;
	gNamingKey.BuildIMGLocation( sTexture.c_str(), fullenvmapname );
	
	// Get the engine
	siege::SiegeEngine& engine			= gSiegeEngine;
	FuelHandleList fhlTerrain = gEditorRegion.GetRegionHandle()->ListChildBlocks( -1, NULL, "siege_node_list" );
	if ( fhlTerrain.size() != 0 ) {
		gpstring faddress = (*fhlTerrain.begin())->GetAddress();
		FuelHandle fhNodes( faddress );

		FuelHandleList fhlNodes	=	fhNodes->ListChildBlocks( 2, "snode" );
		FuelHandleList::iterator i;	
		for ( i = fhlNodes.begin(); i != fhlNodes.end(); ++i ) {
			gpstring szGUID;
			(*i)->Get( "guid", szGUID );
			database_guid guid( szGUID.c_str() );	
			SiegeNodeHandle handle(engine.NodeCache().UseObject( guid ));
			SiegeNode& node = handle.RequestObject(engine.NodeCache());										

			unsigned int envMap = gDefaultRapi.CreateTexture( fullenvmapname, true, 1, TEX_LOCKED );						
			node.SetEnvironmentMap( envMap );			
		}		

		fhNodes->Set( "environment_map", sTexture );
	}

	gEditorRegion.SetRegionDirty();
}


void EditorTerrain::LoadRegionNodes()
{
	/*
	// Get the engine
	siege::SiegeEngine& engine			= gSiegeEngine;

	// Apply any stoppage if collision with terrain occurred
	const siege::SiegeNodeList& hitTestList	= engine.NodeWalker().GetRenderList();
	if( !hitTestList.empty() )
	{
		for( siege::SiegeNodeList::const_iterator i = hitTestList.begin(); i != hitTestList.end(); ++i )
		{
			const siege::SiegeNode& node = (*i).RequestObject( engine.NodeCache() );
			SiegeNodeHandle shandle( gSiegeEngine.NodeCache().UseObject( node.GetGUID() ) );	
			SiegeNode& snode = shandle.RequestObject( gSiegeEngine.NodeCache() );
			
			if ( snode.IsLoadedPhysical() == false )
			{
				gWorldMap.LoadNodePhysical( snode.GetGUID() );
			}
		}		
	}
	*/
}


void EditorTerrain::GrabSelectedGUID()
{
	gpstring sGuid;
	sGuid.assignf( "0x%08X", gEditorTerrain.GetSelectedNode().GetValue() );

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


void EditorTerrain::CompileGuidSet()
{		
	m_usedGuids.clear();
	
	StringVec regions;
	StringVec::iterator iRegion;
	gpstring sMap = gEditorRegion.GetMapHandle()->GetName();
	gEditorRegion.GetRegionList( sMap, regions );
	for ( iRegion = regions.begin(); iRegion != regions.end(); ++iRegion )
	{
		gpstring sIndex;
		sIndex.assignf( "world:maps:%s:regions:%s:index:streamer_node_index", sMap.c_str(), (*iRegion).c_str() );
		FuelHandle hIndex( sIndex );
		if ( hIndex.IsValid() )
		{
			if ( hIndex->FindFirstKeyAndValue() )
			{
				gpstring sNode;
				gpstring sAddress;
				while ( hIndex->GetNextKeyAndValue( sNode, sAddress ) )
				{
					int guidInt = 0;
					stringtool::Get( sNode, guidInt );	
					
					std::pair< std::set< int >::iterator, bool > pair = m_usedGuids.insert( guidInt );
					/*
					if ( pair.second == false )
					{
						gpstring sCollision;
						sCollision.assignf( "Region: %s - A node with GUID: 0x%08X already exists in this map.  Attempt to auto-fix the collision?", (*iRegion).c_str(), guidInt );
						int result = MessageBoxEx( gEditor.GetHWnd(), sCollision, "Node GUID Collision", MB_YESNO | MB_ICONEXCLAMATION, LANG_ENGLISH );
						if ( result == IDYES )
						{
							//int newGuid = GenerateRandomGUID();
							//gpstring sNew;
							//sNew.assignf( "0x%x", newGuid );								
							// $$$
						}
					}
					*/
				}
			}
		}
	}
	
}


siege::database_guid EditorTerrain::GetValidNodeGuid()
{
	return siege::database_guid();
}


void EditorTerrain::AddSelectedLogicalId( siege::database_guid nodeGuid, unsigned char id )
{
	LogicalNodeColl::iterator i = m_selectedLogicalIds.find( nodeGuid );
	if ( i == m_selectedLogicalIds.end() )
	{
		LogicalIdColl ids;
		ids.insert( id );
		m_selectedLogicalIds.insert( LogicalNodePair( nodeGuid, ids ) );
	}
	else
	{		
		LogicalIdColl::iterator iId = (*i).second.find( id );
		if ( iId == (*i).second.end() )
		{
			(*i).second.insert( id );
		}
		else
		{
			(*i).second.erase( iId );
		}		
	}	
}


void EditorTerrain::DeselectLogicalId( siege::database_guid nodeGuid, unsigned char id )
{
	LogicalNodeColl::iterator i = m_selectedLogicalIds.find( nodeGuid );
	if ( i != m_selectedLogicalIds.end() )
	{
		LogicalIdColl ids = (*i).second;
		LogicalIdColl::iterator iId = ids.find( id );
		if ( iId != ids.end() )
		{
			(*i).second.erase( iId );
		}	
	}
}


void EditorTerrain::DeselectLogicalIdsForNode( siege::database_guid nodeGuid )
{
	LogicalNodeColl::iterator i = m_selectedLogicalIds.find( nodeGuid );
	if ( i != m_selectedLogicalIds.end() )
	{
		m_selectedLogicalIds.erase( i );	
	}
}


void EditorTerrain::DeselectAllLogicalIds()
{	
	m_selectedLogicalIds.clear();
}


void EditorTerrain::SelectAllLogicalIds()
{
	siege::SiegeEngine& engine = gSiegeEngine;
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator i;
	for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
	{
		SiegeLogicalNode ** pLNodes = (*i)->GetLogicalNodes();
		int numLogical = (*i)->GetNumLogicalNodes();
		for ( int j = 0; j != numLogical; ++j )
		{
			LogicalIdColl ids;
			ids.insert( pLNodes[j]->GetID() );
			m_selectedLogicalIds.insert( LogicalNodePair( (*i)->GetGUID(), ids ) );
		}		
	}
}


void EditorTerrain::SetSelectedLogicalFlags( siege::eLogicalNodeFlags flags )
{
	LogicalNodeColl::iterator i; 
	for ( i = m_selectedLogicalIds.begin(); i != m_selectedLogicalIds.end(); ++i )
	{
		SiegeNode* pNode = gSiegeEngine.IsNodeValid( (*i).first );
		if ( pNode )
		{
			LogicalIdColl ids = (*i).second;
			LogicalIdColl::iterator iId;
			for ( iId = ids.begin(); iId != ids.end(); ++iId )
			{
				SiegeLogicalNode ** pLNodes = pNode->GetLogicalNodes();
				if ( pLNodes )
				{
					pLNodes[ (*iId) ]->SetFlags( flags );
				}
			}	
		}
	}		
}


DWORD EditorTerrain::GetSelectedLogicalFlags()
{
	LogicalNodeColl::iterator i; 
	for ( i = m_selectedLogicalIds.begin(); i != m_selectedLogicalIds.end(); ++i )
	{
		SiegeNode* pNode = gSiegeEngine.IsNodeValid( (*i).first );
		if ( pNode )
		{
			LogicalIdColl ids = (*i).second;
			LogicalIdColl::iterator iId;
			for ( iId = ids.begin(); iId != ids.end(); ++iId )
			{
				SiegeLogicalNode ** pLNodes = pNode->GetLogicalNodes();
				if ( pLNodes )
				{
					return (DWORD)(pLNodes[ (*iId) ]->GetFlags());
				}
			}
		}
	}
	return 0;
}


void EditorTerrain::SaveLogicalFlags()
{
	gpstring sRegion = gEditorRegion.GetRegionHandle()->GetAddress();
	FuelHandle hEditor( sRegion + ":editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hFlags = hEditor->GetChildBlock( "logical_flags" );
		if ( hFlags.IsValid() )
		{
			hEditor->DestroyChildBlock( hFlags );
		}
		hFlags = hEditor->CreateChildBlock( "logical_flags", "logical_flags.gas" );

		if ( hFlags.IsValid() )
		{
			siege::SiegeEngine& engine = gSiegeEngine;
			FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
			FrustumNodeColl::iterator i;
			for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
			{
				gpstring sGuid;
				sGuid.assignf( "0x%08X", (*i)->GetGUID().GetValue() );
				FuelHandle hNode = hFlags->CreateChildBlock( sGuid );
				if ( hNode.IsValid() )
				{
					hNode->SetType( "lf_snode" );
					SiegeLogicalNode ** pLNodes = (*i)->GetLogicalNodes();
					int numLogical = (*i)->GetNumLogicalNodes();
					for ( int j = 0; j != numLogical; ++j )
					{						
						gpstring sLogId;
						sLogId.assignf( "%d", pLNodes[j]->GetID() );
						FuelHandle hLogicalNode = hNode->CreateChildBlock( sLogId );
						if ( hLogicalNode.IsValid() )
						{
							hLogicalNode->SetType( "lf_lnode" );
							siege::eLogicalNodeFlags currFlags = (siege::eLogicalNodeFlags)pLNodes[j]->GetFlags();
							if ( currFlags != siege::LF_IS_ANY )
							{
								SetLogicalFlagInBlock( currFlags, LF_HUMAN_PLAYER, hLogicalNode );
								SetLogicalFlagInBlock( currFlags, LF_COMPUTER_PLAYER, hLogicalNode );								
								SetLogicalFlagInBlock( currFlags, LF_DIRT, hLogicalNode );
								SetLogicalFlagInBlock( currFlags, LF_SHALLOW_WATER, hLogicalNode );
								SetLogicalFlagInBlock( currFlags, LF_DEEP_WATER, hLogicalNode );
								SetLogicalFlagInBlock( currFlags, LF_ICE, hLogicalNode );
								SetLogicalFlagInBlock( currFlags, LF_LAVA, hLogicalNode );
								SetLogicalFlagInBlock( currFlags, LF_SIZE1_MOVER, hLogicalNode );
								SetLogicalFlagInBlock( currFlags, LF_SIZE2_MOVER, hLogicalNode );
								SetLogicalFlagInBlock( currFlags, LF_SIZE3_MOVER, hLogicalNode );
								SetLogicalFlagInBlock( currFlags, LF_SIZE4_MOVER, hLogicalNode );
								SetLogicalFlagInBlock( currFlags, LF_MIST, hLogicalNode );								
								SetLogicalFlagInBlock( currFlags, LF_HOVER, hLogicalNode );
								SetLogicalFlagInBlock( currFlags, LF_BOSS, hLogicalNode );									
								SetLogicalFlagInBlock( currFlags, LF_CLEAR, hLogicalNode );										
							}
						}
					}		
				}
			}
		}
	}
}


void EditorTerrain::SetLogicalFlagInBlock(	siege::eLogicalNodeFlags currentFlags, 
											siege::eLogicalNodeFlags testFlag,
											FuelHandle & hSave )
{
	if ( testFlag & currentFlags )
	{
		hSave->Set( "*", ToString( siege::eLogicalNodeFlags( testFlag ) ) );								
	}
}


void EditorTerrain::LoadLogicalFlags()
{
	gpstring sRegion = gEditorRegion.GetRegionHandle()->GetAddress();
	FuelHandle hEditor( sRegion + ":editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hFlags = hEditor->GetChildBlock( "logical_flags" );
		if ( hFlags.IsValid() )
		{
			FuelHandleList hlNodes = hFlags->ListChildBlocks( 1, "lf_snode" );
			FuelHandleList::iterator iNodes;
			for ( iNodes = hlNodes.begin(); iNodes != hlNodes.end(); ++iNodes )
			{
				int nodeGuid = 0;
				stringtool::Get( (*iNodes)->GetName(), nodeGuid );

				SiegeNode* pNode = gSiegeEngine.IsNodeValid( siege::database_guid(nodeGuid) );
				if ( pNode )
				{
					SiegeLogicalNode ** pLNodes = pNode->GetLogicalNodes();
					int numLogical = pNode->GetNumLogicalNodes();

					FuelHandleList hlLNodes = (*iNodes)->ListChildBlocks( 1, "lf_lnode" );
					FuelHandleList::iterator iLnode;
					for ( iLnode = hlLNodes.begin(); iLnode != hlLNodes.end(); ++iLnode )
					{
						int id = 0;
						stringtool::Get( (*iLnode)->GetName(), id );
						siege::eLogicalNodeFlags flags = LF_NONE;
						if ( (*iLnode)->FindFirst( "*" ) )
						{							
							gpstring sFlag;
							while ( (*iLnode)->GetNext( sFlag ) )
							{
								siege::eLogicalNodeFlags flag = LF_NONE;
								::FromString( sFlag, flag );
								flags |= flag;
							}
						}

						if ( id < numLogical )
						{
							DWORD dwFlags = 0xFF000000 & pLNodes[id]->GetFlags();
							pLNodes[id]->SetFlags( dwFlags | flags );
						}
					}
				}
			}
		}
	}
}


void EditorTerrain::SaveLightLocks()
{
	gpstring sRegion = gEditorRegion.GetRegionHandle()->GetAddress();
	FuelHandle hEditor( sRegion + ":editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hFlags = hEditor->GetChildBlock( "light_locks" );
		if ( hFlags.IsValid() )
		{
			hEditor->DestroyChildBlock( hFlags );
		}
		hFlags = hEditor->CreateChildBlock( "light_locks", "light_locks.gas" );

		if ( hFlags.IsValid() )
		{
			siege::SiegeEngine& engine = gSiegeEngine;
			FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
			FrustumNodeColl::iterator i;
			for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
			{
				gpstring sGuid;
				sGuid.assignf( "0x%08X", (*i)->GetGUID().GetValue() );
				hFlags->Set( sGuid, (*i)->IsLightLocked(), FVP_BOOL );
			}
		}
	}
}


void EditorTerrain::LoadLightLocks()
{
	gpstring sRegion = gEditorRegion.GetRegionHandle()->GetAddress();
	FuelHandle hEditor( sRegion + ":editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hFlags = hEditor->GetChildBlock( "light_locks" );
		if ( hFlags.IsValid() )
		{
			if ( hFlags->FindFirstKeyAndValue() )
			{
				gpstring sKey;
				gpstring sValue;				
				while ( hFlags->GetNextKeyAndValue( sKey, sValue ) )
				{
					bool bValue = false;
					if ( sValue.same_no_case( "true" ) )
					{
						bValue = true;
					}

					int nodeGuid = 0;
					stringtool::Get( sKey, nodeGuid );
					SiegeNode* pNode = gSiegeEngine.IsNodeValid( siege::database_guid(nodeGuid) );
					if ( pNode )
					{
						pNode->SetLightLocked( bValue );
					}
				}				
			}
		}
	}
}


void EditorTerrain::ReportNodeDoorMatch()
{
	SiegeNode * pSource = gSiegeEngine.IsNodeValid( m_selected_node );
	if ( pSource )
	{
		NodeMatchData nodeMatch;		
		if ( DoesNodeMatchSourceDoor( m_selected_node, m_source_door_id, 
									 m_secondary_node, nodeMatch ) )
		{
			gpgeneric( "Doors that match:\n" );
			std::vector< int >::iterator iMatch;
			for ( iMatch = nodeMatch.doorMatches.begin(); iMatch != nodeMatch.doorMatches.end(); ++iMatch )
			{
				gpgenericf( ( "%d\n", (*iMatch) ) );
			}
		}
	}	
}

bool EditorTerrain::DoesNodeMatchSelected( siege::database_guid destGuid, NodeMatchData & nodeMatch )
{
	return DoesNodeMatchSourceDoor( m_selected_node, m_source_door_id, destGuid, nodeMatch );
}


bool EditorTerrain::DoesNodeMatchSourceDoor( siege::database_guid sourceGuid, int sourceDoor,
											 siege::database_guid destGuid, NodeMatchData & nodeMatch )
{	
	siege::database_guid destNodeGuid = database_guid(GenerateRandomGUID().c_str());
	while ( gSiegeEngine.NodeCache().ObjectExistsInCache( destNodeGuid ) )
	{		
		destNodeGuid = database_guid( GenerateRandomGUID().c_str() );	
	}				
		
	// Create the node and its mesh
	SiegeNodeHandle dest_handle(gSiegeEngine.NodeCache().UseObject(destNodeGuid));
	SiegeNode& dest_node = dest_handle.CreateObject(gSiegeEngine.NodeCache());		
	dest_node.SetGUID( destNodeGuid );
	dest_node.SetMeshGUID( destGuid );	
		
	const SiegeMesh& dest_bmesh = dest_node.GetMeshHandle().RequestObject(gSiegeEngine.MeshCache());	
	const siege::SiegeMesh::SiegeMeshDoorList& destDoors = dest_bmesh.GetDoors();
	
	int destDoorIndex = 1;
	for( siege::SiegeMesh::SiegeMeshDoorConstIter iDoor = destDoors.begin(); iDoor != destDoors.end(); ++iDoor, ++destDoorIndex )
	{
		if ( DoDoorsMatch( sourceGuid, sourceDoor, destGuid, destDoorIndex ) )
		{
			nodeMatch.doorMatches.push_back( destDoorIndex );			
		}
	}

	if ( nodeMatch.doorMatches.size() == 0 )
	{
		return false;
	}

	return true; 
}


bool EditorTerrain::DoDoorsMatch( siege::database_guid sourceGuid, int sourceDoor,
									  siege::database_guid destGuid, int destDoor, bool bIgnoreY )
{
	// Get the siege nodes and make sure they are still valid
	SiegeNode * pSource = gSiegeEngine.IsNodeValid( sourceGuid );
	
	if ( !pSource )
	{
		return false;
	}
	
	// Get the source door vertices		
	SiegeMesh & sourceBmesh = pSource->GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );
	siege::SiegeMeshDoor * pSourceDoor = sourceBmesh.GetDoorByID( sourceDoor );
	if ( !pSourceDoor )
	{
		return false;
	}
	std::vector< int > sourceVertIndices = pSourceDoor->GetVertexIndices();
	
	// Get the destination door vertices		
	siege::SiegeMeshHandle dHandle( gSiegeEngine.MeshCache().UseObject( destGuid ) );
	siege::SiegeMesh& destBmesh	= dHandle.RequestObject( gSiegeEngine.MeshCache() );
	siege::SiegeMeshDoor * pDestDoor = destBmesh.GetDoorByID( destDoor );
	if ( !pDestDoor )
	{
		return false;
	}

	std::vector<int> destVertIndices = pDestDoor->GetVertexIndices();

	////////////
	
	matrix_3x3 orient				= pSourceDoor->GetOrientation();
	matrix_3x3 inv_far_orient		= pDestDoor->GetOrientation().Transpose_T();

	orient.SetColumn_0( -orient.GetColumn_0() );
	orient.SetColumn_2( -orient.GetColumn_2() );

	// Calculate the orient/offset pair that identifies the transformation from dest node to src node space.
	matrix_3x3 door_orient			= orient * inv_far_orient;
	vector_3 door_offset			= pSourceDoor->GetCenter() + (door_orient * -pDestDoor->GetCenter());

	////////////
	
	// These will hold the normalized SOURCE verts for comparison
	std::vector< vector_3 > sourceVerts;
	{
		int numVerts = sourceVertIndices.size();
		if ( numVerts != 0 )
		{				
			for ( unsigned int iIndex = 0; iIndex < numVerts; ++iIndex )
			{
				vector_3 meshPoint;				
				sVertex & vert = sourceBmesh.GetVertices()[ sourceVertIndices[ iIndex ] ];				
			
				meshPoint.x = vert.x;
				meshPoint.y = vert.y;
				meshPoint.z = vert.z;														

				sourceVerts.push_back( meshPoint );					
			}							
		}
	}

	// These will hold the normalized DESTINATION verts for comparison
	std::vector< vector_3 > destVerts;
	{
		vector_3 firstPoint;
		bool	 bFirst		= true;

		int numVerts = destVertIndices.size();
		if ( numVerts != 0 )
		{		
			for ( unsigned int iIndex = 0; iIndex < numVerts; ++iIndex )
			{					
				int vertIndex = destVertIndices[ iIndex ];

				if ( vertIndex >= destBmesh.GetNumberOfVertices() )
				{
					break;
				}

				sVertex & vert = destBmesh.GetVertices()[ vertIndex ];

				vector_3 meshPoint;						
			
				meshPoint.x = vert.x;
				meshPoint.y = vert.y;
				meshPoint.z = vert.z;
				
				vector_3 worldPos( DoNotInitialize );
				door_orient.Transform( worldPos, meshPoint );
				worldPos	+= door_offset;

				destVerts.push_back( worldPos );				
			}							
		}
	}	

	// Compare the results of the vertices	
	{
		bool bTryReverse = false;
		bool bAbort = false;

		if ( sourceVerts.size() != destVerts.size() )
		{
			return false;
		}
		
		{			
			std::vector< vector_3 >::iterator iFirst;
			std::vector< vector_3 >::iterator iSecond = destVerts.begin();
			for ( iFirst = sourceVerts.begin(); iFirst != sourceVerts.end(); ++iFirst, ++iSecond )
			{	
				float firstX = (*iFirst).x;
				float firstY = (*iFirst).y;
				float firstZ = (*iFirst).z;

				float secondX = (*iSecond).x;
				float secondY = (*iSecond).y;
				float secondZ = (*iSecond).z;
				 
				if ( !bIgnoreY )
				{
					if ( !IsEqual(firstY, secondY) )
					{
						bTryReverse = true;
						break;
					}
				}

				bool bSet = false;
				if ( IsEqual( firstX, secondX ) && IsEqual( firstZ, secondZ ) )
				{
					bSet = true;
				}			

				if ( !bSet )
				{
					bTryReverse = true;
					break;
				}			
			}
		}

		if ( bTryReverse )
		{		
			std::vector< vector_3 >::iterator iFirst;
			std::vector< vector_3 >::reverse_iterator iSecond = destVerts.rbegin();
			for ( iFirst = sourceVerts.begin(); iFirst != sourceVerts.end(); ++iFirst, ++iSecond )
			{	
				float firstX = (*iFirst).x;
				float firstY = (*iFirst).y;
				float firstZ = (*iFirst).z;

				float secondX = (*iSecond).x;
				float secondY = (*iSecond).y;
				float secondZ = (*iSecond).z;				

				if ( !bIgnoreY )
				{
					if ( !IsEqual(firstY, secondY) )
					{
						bAbort = true;
						break;
					}			
				}

				bool bSet = false;
				if ( IsEqual( firstX, secondX ) && IsEqual( firstZ, secondZ ) )
				{
					bSet = true;
				}
			
				if ( !bSet )
				{
					bAbort = true;					
					break;
				}					
			}
		}

		if ( !bAbort )
		{
			return true;
		}
	}	

	return false;
}


bool EditorTerrain::FindNodeMatches( int searchLevel, NodeMatchDataColl & nodeMatchData )
{
	CTreeCtrl & wndTree = gLeftView.GetMainView();

	// wndTree.GetItemImage
	HTREEITEM hSelected = wndTree.GetSelectedItem();
	HTREEITEM hHasChild = wndTree.GetChildItem( hSelected );
	HTREEITEM hParent	= hSelected;
	if ( !hHasChild )
	{
		hParent = wndTree.GetParentItem( hSelected );
	}
		 
	int currentLevel = 1;
	while ( currentLevel != searchLevel )
	{
		HTREEITEM hNewParent = wndTree.GetParentItem( hParent );
		if ( !hNewParent )
		{
			HRESULT hResult = MessageBoxEx( gEditor.GetHWnd(), "You are about to search all existing nodes, which is very time and machine intensive, are you sure you wish to continue?", "Node Matcher", MB_YESNO | MB_ICONEXCLAMATION, LANG_ENGLISH );
			if ( hResult == IDNO )
			{
				return false;
			}			
			break;
		}

		hParent = hNewParent;

		++currentLevel;
	}

	if ( wndTree.GetParentItem( hParent ) == NULL )
	{
		HRESULT hResult = MessageBoxEx( gEditor.GetHWnd(), "You are about to search all existing nodes, which is very time and machine intensive, are you sure you wish to continue?", "Node Matcher", MB_YESNO | MB_ICONEXCLAMATION, LANG_ENGLISH );
		if ( hResult == IDNO )
		{
			return false;
		}
	}
	
	if ( hParent )
	{
		HTREEITEM hItem = wndTree.GetChildItem( hParent );
		while ( hItem )
		{			
			CString rSno = wndTree.GetItemText( hItem );
			gDialogNodeMatchProgress.SetCurrentNode( rSno.GetBuffer( 0 ) );
			gpstring sGuid = gComponentList.GetNodeGUID( rSno.GetBuffer( rSno.GetLength() ) );
			if ( !sGuid.empty() )
			{					
				NodeMatchData matchData;
				if ( DoesNodeMatchSelected( siege::database_guid( sGuid.c_str() ), matchData  ) )
				{					
					matchData.sSno = rSno.GetBuffer( 0 );
					matchData.meshGuid = siege::database_guid( sGuid.c_str() );
					nodeMatchData.push_back( matchData );
				}
			}

			HTREEITEM hChild = wndTree.GetChildItem( hItem );
			if ( hChild )
			{
				RecursiveFindNodeMatches( hChild, nodeMatchData );
			}

			hItem = wndTree.GetNextSiblingItem( hItem );			
		}
	}
	
	return true;
}


void EditorTerrain::RecursiveFindNodeMatches( HTREEITEM hItem, NodeMatchDataColl & nodeMatchData )
{	
	while ( hItem )
	{
		CTreeCtrl & wndTree = gLeftView.GetMainView();

		CString rSno = wndTree.GetItemText( hItem );		
		gDialogNodeMatchProgress.SetCurrentNode( rSno.GetBuffer( 0 ) );
		gpstring sGuid = gComponentList.GetNodeGUID( rSno.GetBuffer( rSno.GetLength() ) );
		
		if ( !sGuid.empty() )
		{
			NodeMatchData matchData;
			if ( DoesNodeMatchSelected( siege::database_guid( sGuid.c_str() ), matchData ) )
			{				
				matchData.sSno = rSno.GetBuffer( 0 );
				matchData.meshGuid = siege::database_guid( sGuid.c_str() );
				nodeMatchData.push_back( matchData );
			}
		}

		HTREEITEM hChild = wndTree.GetChildItem( hItem );
		if ( hChild )
		{
			RecursiveFindNodeMatches( hChild, nodeMatchData );
		}

		hItem = wndTree.GetNextSiblingItem( hItem );		
	}
}


void EditorTerrain::SaveNodes()
{
	FuelHandle hNodeList = gEditorRegion.GetRegionHandle()->GetChildBlock( "terrain_nodes:siege_node_list" );
	if ( hNodeList.IsValid() )
	{
		hNodeList->DestroyAllChildBlocks( false );
	}

	siege::SiegeEngine& engine = gSiegeEngine;
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator i;
	for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
	{
		SiegeNode * pNode = (*i);
		gpstring sNode;
		sNode.assignf( "0x%08X", pNode->GetGUID().GetValue() );
		FuelHandle hNode = hNodeList->CreateChildBlock( sNode.c_str() );
		if ( hNode.IsValid() )
		{
			hNode->SetType( "snode" );
			hNode->Set( "guid", pNode->GetGUID().GetValue(), FVP_HEX_INT );
			hNode->Set( "mesh_guid", pNode->GetMeshGUID().GetValue(), FVP_HEX_INT );

			if ( !pNode->GetTextureSetAbbr().empty() )
			{
				hNode->Set( "texsetabbr", pNode->GetTextureSetAbbr().c_str() );
			}
			if ( !pNode->GetTextureSetVersion().empty() )
			{
				hNode->Set( "texsetver", pNode->GetTextureSetVersion().c_str() );
			}

			hNode->Set( "occludes_camera", pNode->GetOccludesCamera() );
			hNode->Set( "bounds_camera", pNode->GetBoundsCamera() );
			hNode->Set( "camera_fade", pNode->GetCameraFade() );
			hNode->Set( "nodesection", pNode->GetSection(), FVP_HEX_INT );
			hNode->Set( "nodelevel", pNode->GetLevel(), FVP_HEX_INT );
			hNode->Set( "nodeobject", pNode->GetObject(), FVP_HEX_INT );	
			hNode->Set( "occludes_light", pNode->GetOccludesLight() );

			SiegeNodeDoorList doorList = pNode->GetDoors();
			SiegeNodeDoorIter iDoor;	
			
			for ( iDoor = doorList.begin(); iDoor != doorList.end(); ++iDoor )
			{
				FuelHandle hDoor = hNode->CreateChildBlock( "door*" );
				hDoor->Set( "id", (*iDoor)->GetID() );
				hDoor->Set( "fardoor", (*iDoor)->GetNeighbourID() );
				hDoor->Set( "farguid", (*iDoor)->GetNeighbour().GetValue(), FVP_HEX_INT );
			}	
			
			if( !pNode->GetLightingData() )
			{
				gEditorLights.ComputeStaticLighting( true );			
			}
		}
	}
}


void EditorTerrain::ReloadBoundingValues()
{
	siege::SiegeEngine& engine = gSiegeEngine;
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator i;
	for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
	{
		SiegeNode * pNode = (*i);
		SiegeMesh & bmesh = pNode->GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );
		pNode->SetMinimumBounds( bmesh.GetMinimumCorner() );
		pNode->SetMaximumBounds( bmesh.GetMaximumCorner() );
		pNode->SetCurrentOrientation( pNode->GetCurrentOrientation() );
	}
}


void EditorTerrain::DrawSelectedLogicalNodeBoxes()
{
	LogicalNodeColl::iterator i;
	for ( i = m_selectedLogicalIds.begin(); i != m_selectedLogicalIds.end(); ++i )
	{
		SiegeNodeHandle shandle( gSiegeEngine.NodeCache().UseObject( (*i).first ) );	
		SiegeNode& snode = shandle.RequestObject( gSiegeEngine.NodeCache() );		

		if ( snode.GetGUID().IsValid() )
		{
			vector_3 node_origin		= snode.GetCurrentCenter();
			matrix_3x3 node_orientation = snode.GetCurrentOrientation();
			gSiegeEngine.Renderer().SetWorldMatrix( node_orientation, node_origin );			

			LogicalIdColl::iterator j;
			for ( j = (*i).second.begin(); j != (*i).second.end(); ++j )
			{	
				int index = *j;
				for ( int k = 0; k != snode.GetNumLogicalNodes(); ++k )
				{
					if ( snode.GetLogicalNodes()[k] && snode.GetLogicalNodes()[k]->GetID() == index )
					{
						RP_DrawBox( gSiegeEngine.Renderer(), snode.GetLogicalNodes()[ k ]->GetLogicalMesh()->GetMinimumBounds(), snode.GetLogicalNodes()[ (*j) ]->GetLogicalMesh()->GetMaximumBounds(), 0xFFFF0000 );
					}
				}				
			}
		}
	}	
}


void EditorTerrain::DrawLogicalFlags()
{
	siege::SiegeEngine& engine = gSiegeEngine;
	if ( engine.GetFrustum( gEditorRegion.GetFrustumId() ) )
	{		
		FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
		FrustumNodeColl::iterator i;
		for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
		{
			SiegeNode * pNode = (*i);
			if ( pNode->GetVisible() )
			{
				vector_3 node_origin		= pNode->GetCurrentCenter();
				matrix_3x3 node_orientation = pNode->GetCurrentOrientation();
				gSiegeEngine.Renderer().SetWorldMatrix( node_orientation, node_origin );			
				
				for ( unsigned int j = 0; j != pNode->GetNumLogicalNodes(); ++j )
				{		
					if ( pNode->GetLogicalNodes()[ j ] )
					{
						DWORD dwColor = 0;
						DWORD flags = pNode->GetLogicalNodes()[ j ]->GetFlags();						
						if ( (flags | 0xff000000 ) == 0xff000000 )
						{
							continue;
						}
						
						ValueToColorMap::iterator iValueColor = m_ValueToColorMap.find( flags );
						if ( iValueColor == m_ValueToColorMap.end() )
						{
							ColorUsageMap::iterator iColor;
							for ( iColor = m_ColorUsageMap.begin(); iColor != m_ColorUsageMap.end(); ++iColor )
							{
								if ( (*iColor).second == false )
								{
									m_ValueToColorMap.insert( std::make_pair( flags, (*iColor).first ) );
									(*iColor).second = true;
									dwColor = (*iColor).first;
									break;
								}
							}
						}	
						else
						{
							dwColor = (*iValueColor).second;
						}

						dwColor |= 0xff000000;
						
						RP_DrawFilledBox( gSiegeEngine.Renderer(), pNode->GetLogicalNodes()[ j ]->GetLogicalMesh()->GetMinimumBounds(), pNode->GetLogicalNodes()[ j ]->GetLogicalMesh()->GetMaximumBounds()+0.1f, dwColor );
					}
				}
			}
		}
	}
}


void EditorTerrain::DrawDoorNumbers( siege::database_guid nodeGuid )
{
	if ( nodeGuid == UNDEFINED_GUID ) 
	{
		return;
	}

	SiegeEngine & engine = gSiegeEngine;

	SiegeNode * pNode = gSiegeEngine.IsNodeValid( nodeGuid );
	if ( !pNode )
	{
		if ( nodeGuid == m_DrawSelected )
		{
			SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( nodeGuid ) );
			SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );	
			pNode = &node;
		}
		else
		{
			return;
		}		
	}

	SiegeMesh& bmesh = pNode->GetMeshHandle().RequestObject(engine.MeshCache());			

	int i = 1;
	for ( siege::SiegeMesh::SiegeMeshDoorConstIter bdi = bmesh.GetDoors().begin(); bdi != bmesh.GetDoors().end(); ++bdi )
	{		
		SiegeMeshDoor * pDoor = bmesh.GetDoorByID( (*bdi)->GetID() );		
		if ( !pDoor ) 
		{
			continue; 
		}
		vector_3 vPos = pDoor->GetCenter();
		SiegePos pos;
		pos.node = nodeGuid;
		pos.pos = vPos;
		pos.pos.y += 0.4f;
		char szNum[16] = "";
		sprintf( szNum, "%d", i );
		
		if ( m_DrawSelected == nodeGuid )
		{
			pos.pos = m_SelectedOrient * pos.pos;
			vector_3 targetpos;
			SiegeMeshDoor * pDestDoor = bmesh.GetDoorByID( GetDestConnectionID() );
			if ( pDestDoor )
			{
				targetpos = m_SelectedOrient * pDestDoor->GetCenter();
			}	
			pos.pos += m_SelectedPos-targetpos;	
			pos.node = GetTargetNode();			
		}

		engine.GetLabels().DrawLabel( m_pFont, pos, szNum, 0.0f, 0.8f );
		++i;
	}
}


void EditorTerrain::DrawDoor( siege::database_guid nodeGUID, int ID, DWORD dwColor )
{
	if ( nodeGUID == UNDEFINED_GUID ) 
	{
		return;
	}

	SiegeEngine & engine = gSiegeEngine;	

	SiegeNode * pNode = gSiegeEngine.IsNodeValid( nodeGUID );
	if ( !pNode )
	{
		return;
	}

	vector_3 node_origin		= pNode->GetCurrentCenter();
	matrix_3x3 node_orientation = pNode->GetCurrentOrientation();

	DrawDoor( nodeGUID, ID, node_origin, node_orientation, dwColor );		
}


void EditorTerrain::DrawDoor( siege::database_guid nodeGuid, int id, vector_3 nodeOrigin, matrix_3x3 nodeOrient, DWORD dwColor )
{
	SiegeNode * pNode = gSiegeEngine.IsNodeValid( nodeGuid );
	if ( !pNode )
	{
		if ( nodeGuid == m_DrawSelected )
		{
			SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( nodeGuid ) );
			SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );	
			pNode = &node;
		}
		else
		{
			return;
		}
	}

	SiegeMesh& bmesh = pNode->GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );			
	Rapi& mRenderer = gSiegeEngine.Renderer();

	bool bIgnoreVertexColor = false;
	if ( mRenderer.IsIgnoringVertexColor() == true ) 
	{
		mRenderer.IgnoreVertexColor( false );
		bIgnoreVertexColor = true;
	}

	mRenderer.SetPerspectiveMatrix( gSiegeEngine.GetCamera().GetFieldOfView(), 0.0f, 0.0f );
	mRenderer.SetTextureStageState(	0,
									D3DTOP_DISABLE,
									D3DTOP_SELECTARG2,
									D3DTADDRESS_WRAP,
									D3DTADDRESS_WRAP,
									D3DTFG_LINEAR,
									D3DTFN_LINEAR,
									D3DTFP_LINEAR,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );
	
	mRenderer.SetWorldMatrix( nodeOrient, nodeOrigin );

	for( siege::SiegeMesh::SiegeMeshDoorConstIter bdi = bmesh.GetDoors().begin(); bdi != bmesh.GetDoors().end(); ++bdi ) 
	{		
		if ( (*bdi)->GetID() == id ) 
		{
			int numIndices		= (*bdi)->GetVertexIndices().size();

			if( !numIndices )
			{
				continue;		
			}

			unsigned int	startIndex;
			sVertex*		pVerts;
			if( mRenderer.LockBufferedVertices( numIndices, startIndex, pVerts ) )
			{
				DWORD color	= 0xFFFF0000;

				if ( dwColor == 0 )
				{
					SiegeNodeDoor* pSpaceDoor	= pNode->GetDoorByID( (*bdi)->GetID() );
					if( pSpaceDoor )
					{
						if( pSpaceDoor->GetNeighbour() == UNDEFINED_GUID )
						{
							color	= 0xFFFF0000;
						}
						else if( pSpaceDoor->IsInaccurate() )
						{
							color	= 0xFFFFFF00;
						}
						else
						{
							color	= 0xFF00FF00;
						}
					}

					if ( (*bdi)->GetID() == (unsigned)id ) 
					{
						if( !pSpaceDoor ){
							color	= 0xFFFF0000;
						}
					}
				}
				else
				{
					color = dwColor;
				}

				for( unsigned int j = 0; j < numIndices; ++j )
				{
					pVerts[ j ]			= bmesh.GetVertices()[ (*bdi)->GetVertexIndices()[ j ] ];
					pVerts[ j ].color	= color;
				}

				mRenderer.UnlockBufferedVertices();

				mRenderer.DrawBufferedPrimitive( D3DPT_LINESTRIP, startIndex, numIndices );

				mRenderer.PushWorldMatrix();

				mRenderer.TranslateWorldMatrix( (*bdi)->GetCenter() );
				mRenderer.RotateWorldMatrix( (*bdi)->GetOrientation() );
				RP_DrawRay( mRenderer, vector_3( 0.0f, 0.0f, 0.25f ), pVerts[0].color );

				mRenderer.PopWorldMatrix();
				mRenderer.PushWorldMatrix();

				mRenderer.TranslateWorldMatrix( (*bdi)->GetCenter() + (Normalize( (*bdi)->GetOrientation().GetColumn2_T() ) * 0.25f) );
				matrix_3x3 orient	= (*bdi)->GetOrientation();
				orient.SetColumn1( (*bdi)->GetOrientation().GetColumn2_T() );
				orient.SetColumn2( (*bdi)->GetOrientation().GetColumn1_T() );
				mRenderer.RotateWorldMatrix( orient );
				RP_DrawPyramid( mRenderer, 0.1f, 0.2f, pVerts[0].color );

				mRenderer.PopWorldMatrix();
			}
		}
	}

	mRenderer.SetPerspectiveMatrix( gSiegeEngine.GetCamera().GetFieldOfView(), 
									gSiegeEngine.Renderer().GetClipNearDist(), 
									gSiegeEngine.Renderer().GetClipFarDist() );
	
	
	if ( bIgnoreVertexColor == true ) 
	{
		mRenderer.IgnoreVertexColor( true );
	}
}


void EditorTerrain::DrawStitchDoors()
{
	StitchVector::iterator i;
	StitchVector stitches = gStitchHelper.GetStitchVector();
	for ( i = stitches.begin(); i != stitches.end(); ++i )
	{
		// $ need to find a better method of handling this ( that's fast )
		/*
		if ( !(*i).nodeGUID.IsValidSlow() )
		{		
			gperrorf(( "Invalid Stitch Node: 0x%08X", (*i).nodeGUID.GetValue() ));
			return;
		}
		*/
		
		DrawDoor( (*i).nodeGUID, (*i).doorID, 0xFFFFFF00 );

		SiegeEngine & engine = gSiegeEngine;	
		
		SiegeNode* pNode = gSiegeEngine.IsNodeValid( (*i).nodeGUID );
		if ( pNode )
		{		
			SiegeMesh& bmesh = pNode->GetMeshHandle().RequestObject(engine.MeshCache());			
			
			for( siege::SiegeMesh::SiegeMeshDoorConstIter bdi = bmesh.GetDoors().begin(); bdi != bmesh.GetDoors().end(); ++bdi ) {
							
				SiegeMeshDoor * pDoor = bmesh.GetDoorByID( (*bdi)->GetID() );		
				if ( !pDoor ) {
					continue; 
				}

				if ( (*bdi)->GetID() == (*i).doorID )
				{
					vector_3 vPos = pDoor->GetCenter();
					SiegePos pos;
					pos.node = (*i).nodeGUID;
					pos.pos = vPos;
					pos.pos.y += 0.4f;
					char szNum[16] = "";
					sprintf( szNum, "0x%08X", (*i).nodeID );
					engine.GetLabels().DrawLabel( m_pFont, pos, szNum, 0.0f, 0.8f );				
				}
			}
		}
	}
}


void EditorTerrain::DrawSelectedNodeBoxes()
{
	// Render Node Box
	SiegeEngine & engine = gSiegeEngine;	
	
	// Draw the selected node children
	std::vector< siege::database_guid >::iterator i;
	for ( i = m_selected_nodes.begin(); i != m_selected_nodes.end(); ++i ) {
		SiegeNodeHandle handle( engine.NodeCache().UseObject(*i) );
		SiegeNode& node = handle.RequestObject( engine.NodeCache() );

		if (!( node.GetMeshGUID() == UNDEFINED_GUID )) {
			const SiegeMesh& bmesh = node.GetMeshHandle().RequestObject(engine.MeshCache());
			
			vector_3 node_origin		= node.GetCurrentCenter();
			matrix_3x3 node_orientation = node.GetCurrentOrientation();
			engine.Renderer().SetWorldMatrix( node_orientation, node_origin );
			DrawNodeBox(bmesh.BBoxCenter(),bmesh.BBoxHalfDiag(), vector_3(0,1,0.5), engine.Renderer() );
		}				
	}

	// Draw the master selected node
	{
		SiegeNode* pNode = gSiegeEngine.IsNodeValid( m_selected_node );
		if ( pNode )
		{		
			if (!( pNode->GetMeshGUID() == UNDEFINED_GUID )) 
			{
				const SiegeMesh& bmesh = pNode->GetMeshHandle().RequestObject(engine.MeshCache());
				vector_3 node_origin		= pNode->GetCurrentCenter();
				matrix_3x3 node_orientation = pNode->GetCurrentOrientation();
				engine.Renderer().SetWorldMatrix( node_orientation, node_origin );
				DrawNodeBox(bmesh.BBoxCenter(),bmesh.BBoxHalfDiag(), vector_3(0,0.4f,1), engine.Renderer() );
			}
		}
	}
}


void EditorTerrain::DrawNodeBox( vector_3 c, vector_3 d, vector_3 colorv, Rapi& renderer ) 
{
	DrawNodeBox( c, d, MAKEDWORDCOLOR( colorv ), renderer );
}


void EditorTerrain::DrawNodeBox( vector_3 c, vector_3 d, DWORD dwColor, Rapi& renderer )
{
	vector_3 p0 = c + vector_3( d.GetX(), d.GetY(),-d.GetZ());
	if ( d.GetY() < 1 ) {
		p0 = c + vector_3( d.GetX(), d.GetY()+1,-d.GetZ());
	}
	vector_3 p1 = c + vector_3(-d.GetX(), d.GetY(),-d.GetZ());
	if ( d.GetY() < 1 ) {
		p1 = c + vector_3(-d.GetX(), d.GetY()+1,-d.GetZ());
	}

	vector_3 p2 = c + vector_3(-d.GetX(),-d.GetY()+0.1f,-d.GetZ());
	vector_3 p3 = c + vector_3( d.GetX(),-d.GetY()+0.1f,-d.GetZ());
	vector_3 p4 = c + vector_3( d.GetX(), d.GetY(), d.GetZ());
	if ( d.GetY() < 1 ) {
		p4 = c + vector_3( d.GetX(), d.GetY()+1, d.GetZ());
	}
	
	vector_3 p5 = c + vector_3(-d.GetX(), d.GetY(), d.GetZ());
	if ( d.GetY() < 1 ) {	
		p5 = c + vector_3(-d.GetX(), d.GetY()+1, d.GetZ());
	}

	vector_3 p6 = c + vector_3(-d.GetX(),-d.GetY()+0.1f, d.GetZ());
	vector_3 p7 = c + vector_3( d.GetX(),-d.GetY()+0.1f, d.GetZ());

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

	verts[0].color	= dwColor;
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


gpstring EditorTerrain::GenerateRandomGUID()
{	
	gpstring sRandom;

	int random = RandomInt();
	while ( m_usedGuids.find( random ) != m_usedGuids.end() )
	{
		random = RandomInt();
	}
	m_usedGuids.insert( random );

	sRandom.assignf( "0x%08x", random );
	return sRandom;
}


void EditorTerrain::DrawGrid()
{
	if ( gPreferences.GetEnableGrid() )
	{
		int size		= gPreferences.GetGridSize();
		int intervals	= gPreferences.GetGridInterval();
		DWORD color		= gPreferences.GetGridColor();
		vector_3 origin	= gPreferences.GetGridOrigin();

		SiegeNode * pNode = gSiegeEngine.IsNodeValid( gEditorTerrain.GetTargetNode() );
		if ( !pNode )
		{
			return;
		}

		gSiegeEngine.Renderer().SetWorldMatrix( pNode->GetCurrentOrientation(), pNode->GetCurrentCenter() );
		gSiegeEngine.Renderer().PushWorldMatrix();
		gSiegeEngine.Renderer().TranslateWorldMatrix( origin );	

		// Draw origin of the grid
		RP_DrawSphere( gSiegeEngine.Renderer(), 0.3f, 20, color );
		RP_DrawReferenceGrid( gSiegeEngine.Renderer(), size, intervals, color );	

		if ( gPreferences.GetGridDrawLabelIntervals() )
		{
			float meterIntervals = (float)size / (float)intervals;

			{				
				float total = 0.0f;
				while ( total <= (float)size/2.0f )
				{
					SiegePos spos;
					spos.pos.x = total + origin.x;
					spos.pos.y = origin.y;
					spos.pos.z = origin.z;
					spos.node = gEditorTerrain.GetTargetNode();
					
					gpstring sTemp;
					sTemp.assignf( "%f", total );
					gSiegeEngine.GetLabels().DrawLabel( m_pFont, spos, sTemp.c_str(), 0.0f, 0.8f );					

					total += meterIntervals;
				}			
			}
			{				
				float total = 0.0f;
				while ( total <= (float)size/2.0f )
				{
					SiegePos spos;
					spos.pos.x = origin.x - total;
					spos.pos.y = origin.y;
					spos.pos.z = origin.z;
					spos.node = gEditorTerrain.GetTargetNode();
					
					gpstring sTemp;
					sTemp.assignf( "%f", total );
					gSiegeEngine.GetLabels().DrawLabel( m_pFont, spos, sTemp.c_str(), 0.0f, 0.8f );					

					total += meterIntervals;
				}			
			}
			{				
				float total = 0.0f;
				while ( total <= (float)size/2.0f )
				{
					SiegePos spos;
					spos.pos.x = origin.x;
					spos.pos.y = origin.y;
					spos.pos.z = origin.z + total;
					spos.node = gEditorTerrain.GetTargetNode();
					
					gpstring sTemp;
					sTemp.assignf( "%f", total );
					gSiegeEngine.GetLabels().DrawLabel( m_pFont, spos, sTemp.c_str(), 0.0f, 0.8f );					

					total += meterIntervals;
				}			
			}
			{				
				float total = 0.0f;
				while ( total <= (float)size/2.0f )
				{
					SiegePos spos;
					spos.pos.x = origin.x;
					spos.pos.y = origin.y;
					spos.pos.z = origin.z - total;
					spos.node = gEditorTerrain.GetTargetNode();
					
					gpstring sTemp;
					sTemp.assignf( "%f", total );
					gSiegeEngine.GetLabels().DrawLabel( m_pFont, spos, sTemp.c_str(), 0.0f, 0.8f );					

					total += meterIntervals;
				}			
			}
		}	

		gSiegeEngine.Renderer().PopWorldMatrix();
	}
}


void EditorTerrain::DrawFrustum()
{
	// $$$ Todo: Add a customizable frustum the level designers can use to 
	/* Test code
	siege::SiegeFrustum* pFrustum	= gSiegeEngine.GetFrustum( gSiegeEngine.GetRenderFrustum() );
	if( pFrustum )
	{		
		RP_DrawFrustum( gSiegeEngine.Renderer(), pFrustum->GetPosition().WorldPos(), matrix_3x3(), 0.0f, 1.0f, 5, 6, 0xffff0000, 0xff00ff00 );
	}
	*/
}
