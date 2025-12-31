//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorCamera.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////



// Include Files
#include "PrecompEditor.h"
#include "EditorCamera.h"
#include "Matrix_3x3.h"
#include "Space_3.h"
#include "StdHelp.h"
#include "EditorTerrain.h"
#include "EditorRegion.h"
#include "Preferences.h"
#include "EditorMap.h"
#include "Siege_Camera.h"
#include "CameraAgent.h"


using namespace siege;



// Construction
EditorCamera::EditorCamera( siege::SiegeCamera * pCamera )
	: m_camera( pCamera )
	, m_orbit(0.0f)
	, m_azimuth(RealPi/4)
	, m_cameraDistance(20.0f)
	, m_targetPos(vector_3(0,0,0))
	, m_scrollrate(3)
	, m_zoomMagnitude(5.0f)
	, m_cameraHeightOffset( 0.0f )
{
	m_startingPos		= m_camera->GetTargetPosition();
	m_startingOrbit		= m_orbit;
	m_startingAzimuth	= m_azimuth;
	m_startingDistance	= m_cameraDistance;
	m_scrollMagnitude	= 0.1f;

	CameraEulerPosition pos;
	pos.azimuthAngle	= m_startingAzimuth;
	pos.distance		= m_startingDistance;
	pos.orbitAngle		= m_startingOrbit;
	pos.targetPos.pos	= m_startingPos;
	pos.targetPos.node	= gEditorTerrain.GetTargetNode();
	gpstring sPoint = "";
	gCameraAgent.SubmitCameraPosition( sPoint, pos );

	FuelHandle hCamera( "config:camera_settings" );
	if ( hCamera )
	{
		hCamera->Get( "tracking_height_offset", m_cameraHeightOffset );
	}
}



void EditorCamera::Rotate( int x, int y )
{
	float azimuth	= m_azimuth;
	if ( azimuth == 45.00 ) {
		azimuth = (RealPi)/4;
	}

	float scroll_magnitudeX = ( (float)x - (float)GetCameraXRotatePos() ) / 100.0f;
	float scroll_magnitudeY = ( (float)y - (float)GetCameraYRotatePos() ) / 100.0f;

	float orbit	= m_orbit;

	if ( scroll_magnitudeY > 0 ) {
		m_azimuth = azimuth - (float)scroll_magnitudeY;
		if ( m_azimuth <= -(RealPi/2) + 0.1 ) {
			m_azimuth = (float)(-RealPi/2 + 0.1f);
		}
	}
	else if ( scroll_magnitudeY < 0 ) {
		m_azimuth = azimuth - (float)scroll_magnitudeY;
		if ( m_azimuth >= (RealPi/2) - 0.1 ) {
			m_azimuth = (float)(RealPi/2 - 0.1f);
		}
	}

	if ( scroll_magnitudeX > 0 ) {
		m_orbit = orbit + (float)scroll_magnitudeX;
	}
	else if ( scroll_magnitudeX < 0 ) {
		m_orbit = orbit + (float)scroll_magnitudeX;
	}

	Update();
}



void EditorCamera::Zoom( SCROLL_DIRECTION direction )
{
	switch ( direction ) {
	case N:
	case NW:
	case NE:
		m_cameraDistance += 1.0f;
		break;
	case S:
	case SW:
	case SE:
		m_cameraDistance -= 1.0f;
		break;
	}

	if ( m_cameraDistance <= 0.0f ) {
		m_cameraDistance = 0.1f;
	}

	Update();
}



void EditorCamera::UpdateZoom( SCROLL_DIRECTION direction, double zoom_magnitude )
{
	if ( zoom_magnitude == -1 ) {
		zoom_magnitude = m_zoomMagnitude;
	}

	float zoom_factor	= (float)zoom_magnitude * gPreferences.GetZoomRate();

	switch ( direction ) {
	case N:
	case NW:
	case NE:
		m_cameraDistance += zoom_factor;
		break;
	case S:
	case SW:
	case SE:
		m_cameraDistance -= zoom_factor;
		break;
	}

	if ( m_cameraDistance <= 0.0f ) {
		m_cameraDistance = 0.1f;
	}

	Update();
}



void EditorCamera::UpdateScrolling( int x, int y, bool bShiftDown )
{
	vector_3 cameraDelta	= ( m_camera->GetMatrixOrientation().GetColumn0_T() * ((float)(m_x_scroll_pos-x) / 10.0f) );
	cameraDelta				+= ( CrossProduct( m_camera->GetMatrixOrientation().GetColumn0_T(), vector_3( 0.0f, 1.0f, 0.0f ) ) * ((float)(m_y_scroll_pos-y) / 10.0f) );

	if( bShiftDown == false )
	{
		m_targetPos	+= cameraDelta;		
	}
	else
	{
		m_targetPos.SetY( m_targetPos.GetY() - ((float)(m_y_scroll_pos-y) / 10.0f) );
	}

	Update();
}


void EditorCamera::Scroll( SCROLL_DIRECTION direction, float magnitude )
{
	switch ( direction ) 
	{
	case N:
	case NW:
	case NE:
		m_targetPos.x += magnitude;
		break;
	case S:
	case SW:
	case SE:
		m_targetPos.x -= magnitude;
		break;
	case E:
		m_targetPos.z += magnitude;
		break;
	case W:
		m_targetPos.z -= magnitude;
		break;
	}

	Update();
}


void EditorCamera::Reset()
{
	m_targetPos			= m_startingPos;
	m_orbit				= m_startingOrbit;
	m_azimuth			= m_startingAzimuth;
	m_cameraDistance	= m_startingDistance;

	Update();
}


void EditorCamera::AngleView()
{
	m_orbit		= m_startingOrbit;
	m_azimuth	= m_startingAzimuth;

	Update();
}


void EditorCamera::OverheadView()
{
	m_azimuth = RealPi/2.01;

	Update();
}


void EditorCamera::SideView()
{
	m_azimuth = 0;
	Update();
}


void EditorCamera::SetPosition( float x, float y, float z )
{
	vector_3 position;
	position.SetX( (float)x );
	position.SetY( (float)y );
	position.SetZ( (float)z );

	// m_camera->SetTargetPosition( position );
	m_targetPos = position;

	Update();
}


void EditorCamera::SetTargetPositionNode( siege::database_guid guid )
{
	m_node = guid;

	Update();
}



// Update the current camera position
void EditorCamera::Update()
{
	if ( gPreferences.GetRestrictCameraToGame() && gEditorRegion.GetRegionEnable() )
	{
		SiegePos newPos;
		newPos.pos = m_targetPos;
		newPos.node = m_node;

		bool bAddOffset = false;
		bool bSnap = gPreferences.GetSnapToGround();
		gPreferences.SetSnapToGround( true );
		if ( gGizmoManager.AccurateAdjustPositionToTerrain( newPos ) )
		{
			bAddOffset = true;
		}
		gPreferences.SetSnapToGround( false );

		if ( newPos.node != m_node )
		{
			SiegeNode * pNode = gSiegeEngine.IsNodeValid( newPos.node );
			if ( pNode )
			{
				m_targetPos = pNode->NodeToWorldSpace( newPos.pos );				
			}
		}
		else
		{
			m_targetPos = newPos.pos;
		}

		if ( bAddOffset )
		{
			m_targetPos.y += m_cameraHeightOffset;
		}
		
		if ( m_azimuth < (DegreesToRadians(m_minAzimuth)) )
		{
			m_azimuth = DegreesToRadians(m_minAzimuth);
		}

		if ( m_azimuth > (DegreesToRadians(m_maxAzimuth)) )
		{
			m_azimuth = DegreesToRadians(m_maxAzimuth);
		}

		if ( m_cameraDistance < (m_minDistance+m_cameraHeightOffset) )
		{
			m_cameraDistance = m_minDistance;
		}

		if ( m_cameraDistance > (m_maxDistance+m_cameraHeightOffset) )
		{
			m_cameraDistance = m_maxDistance;
		}
	}

	CameraEulerPosition pos;
	pos.azimuthAngle	= m_azimuth;
	pos.distance		= m_cameraDistance;
	pos.orbitAngle		= m_orbit;
	pos.targetPos.pos	= m_targetPos;
	pos.targetPos.node	= m_node;
	gpstring sPoint = "point";	

	gCameraAgent.SubmitCameraPosition( sPoint, pos );
	gCameraAgent.SubmitOrder( sPoint );
}

void EditorCamera::SetGoCameraToEditorCamera( Go * pObject )
{
	siege::SiegeEngine & engine = gSiegeEngine;
	SiegePos spos;
	spos.pos = engine.GetCamera().GetCameraPosition();
	spos.node = gEditorTerrain.GetTargetNode();
	Quat orient = engine.GetCamera().GetQuatOrientation();
	gGizmoManager.AccurateAdjustPositionToTerrain( spos );
		
	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( spos.node ) );
	SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );			
	Quat newOrient( node.GetTransposeOrientation() * orient.BuildMatrix() );	

	bool bSnap = gPreferences.GetSnapToGround();
	gPreferences.SetSnapToGround( false );

	gGizmoManager.SetPosition( MakeInt( pObject->GetGoid() ), spos );	
	pObject->GetPlacement()->SetOrientation( newOrient );	
}


void EditorCamera::SetEditorCameraToGoCamera( Go * pObject )
{
	SiegePos spos = pObject->GetPlacement()->GetPosition();
	Quat orient = pObject->GetPlacement()->GetOrientation();
	CameraQuatPosition pos;
	pos.cameraOrient			= orient;
	pos.cameraPos				= spos;
	gpstring sPoint				= "point";
	gCameraAgent.SubmitCameraPosition( sPoint, pos );
	gCameraAgent.SubmitOrder( sPoint );
}


void EditorCamera::SetCameraTargetToGo( Goid object )
{
	GoHandle hObject( object );
	if ( hObject )
	{
		SiegePos spos = hObject->GetPlacement()->GetPosition();
		SiegeNode * pNode = gSiegeEngine.IsNodeValid( spos.node	);
		if ( pNode )
		{
			spos.pos = pNode->NodeToWorldSpace( spos.pos );
		}

		m_targetPos = spos.pos;

		Update();
	}
}


// Returns the Current Camera Position
CameraEulerPosition EditorCamera::GetCurrentCameraPosition()
{
	CameraEulerPosition pos;
	pos.azimuthAngle	= m_azimuth;
	pos.distance		= m_cameraDistance;
	pos.orbitAngle		= m_orbit;

	// Convert the origin to local space
	SiegePos	newCamPos, newTarPos;

	siege::SiegeEngine & engine = gSiegeEngine;
	// Need to convert our world space values to nodespace values
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator iNode;
	for( iNode = nodeColl.begin(); iNode != nodeColl.end(); ++iNode )
	{	
		SiegeNode& node = *(*iNode);

		const vector_3 origin		= engine.GetCamera().GetCameraPosition();
		const vector_3 direction	= m_targetPos - engine.GetCamera().GetCameraPosition();

		vector_3 coord( DoNotInitialize );

		if( RayIntersectsBox(	node.GetWorldSpaceMinimumBounds(),
								node.GetWorldSpaceMaximumBounds(),
								origin, direction, coord ) )
		{
			newCamPos.node = newTarPos.node	= node.GetGUID();
			newCamPos.pos				= node.WorldToNodeSpace( origin );
			newTarPos.pos				= node.WorldToNodeSpace( engine.GetCamera().GetTargetPosition() );
			break;
		}
	}

	if ( newTarPos.node == UNDEFINED_GUID ) {
		for( iNode = nodeColl.begin(); iNode != nodeColl.end(); ++iNode )
		{
			SiegeNode& node = *(*iNode);

			const vector_3 origin		= engine.GetCamera().GetCameraPosition();
			const vector_3 direction	= vector_3( 0, -1, 0 );

			vector_3 coord( DoNotInitialize );

			if( RayIntersectsBox(	node.GetWorldSpaceMinimumBounds(),
									node.GetWorldSpaceMaximumBounds(),
									origin, direction, coord ) )
			{
				newCamPos.node = newTarPos.node	= node.GetGUID();
				newCamPos.pos				= node.WorldToNodeSpace( origin );
				newTarPos.pos				= node.WorldToNodeSpace( engine.GetCamera().GetTargetPosition() );
				break;
			}
		}
	}

	pos.targetPos.pos	= newTarPos.pos;
	pos.targetPos.node	= newTarPos.node;

	// Get the important angle information
	{
		EditorCamera& m_currentCamera	= gEditorCamera;
		siege::SiegeCamera& camera	= gSiegeEngine.GetCamera();

		SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( pos.targetPos.node ) );
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

		//m_currentCamera.SetTargetPosition( newtargetposition );
		//m_currentCamera.SetOrbitAngle( m_currentCamera.GetOrbit() - orbit );
		//m_currentCamera.SetAzimuthAngle( azimuth );

		pos.azimuthAngle	=	azimuth;
		pos.orbitAngle		=	m_currentCamera.GetOrbit() - orbit;

	}

	if ( pos.targetPos.node == UNDEFINED_GUID ) {
		MessageBox( gEditor.GetHWnd(), "This camera position is invalid ( not over a node or looking at a node ).  This position will NOT look right in the game, so delete it!", "Camera Error", MB_OK );
	}
	return pos;
}


void EditorCamera::SetStartingMapCamera()
{
	float x				= 0;
	float y				= 0;
	float z				= 0;
	int   nodeId		= 0;
	float azimuth		= 0;
	float orbit			= 0;
	float distance		= 0;
	float minDistance	= 0;
	float maxDistance	= 0;
	float nearclip		= 0;
	float farclip		= 0;
	float fov			= 0;
	float minAzimuth	= 0;
	float maxAzimuth	= 0;
	
	gEditorMap.GetCameraSettings( y, z, x, nodeId, azimuth, orbit, distance, m_minDistance, m_maxDistance, 
								 nearclip, farclip, fov, m_minAzimuth, m_maxAzimuth );

	SetPosition( x, y, z );
	SetAzimuthAngle( azimuth );
	SetOrbitAngle( orbit );
	SetDistance( distance );

	if ( nodeId != 0 )
	{
		SetTargetNodePosition( siege::database_guid(nodeId) );
	}
	else
	{
		SetTargetNodePosition( gEditorTerrain.GetTargetNode() );
	}
}


void EditorCamera::DrawCameraTarget()
{
	vector_3 targetOffset = m_camera->GetTargetPosition();
	SiegePos sposOffset = m_camera->GetTargetSiegePos();

	SiegePos sposTarget = m_camera->GetTargetSiegePos();
	sposTarget.pos.y -= m_cameraHeightOffset;
	sposTarget.pos.y += 0.1f;

	vector_3 vTarget = m_camera->GetTargetPosition();
	vTarget.y -= m_cameraHeightOffset;

	gSiegeEngine.GetWorldSpaceLines().DrawLine( vTarget, targetOffset, 0xff00ffff );
	gSiegeEngine.GetLabels().DrawLabel( gEditorTerrain.GetFont(), sposTarget, "Target Position", 0.0f, 0.2f );
	gSiegeEngine.GetLabels().DrawLabel( gEditorTerrain.GetFont(), sposOffset, "Height Offset", 0.0f, 0.2f );

	gSiegeEngine.GetNodeSpaceLines().DrawCircle( vTarget, m_camera->GetTargetSiegePos().node, 0.1f, 0.0f, PI2 );
	gSiegeEngine.GetNodeSpaceLines().DrawCircle( sposOffset.pos, sposOffset.node, 0.1f, 0.0f, PI2 );
}