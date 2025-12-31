/**************************************************************************
**
**							UICamera
**
**		See .h file for details.
**
**************************************************************************/

#include "precomp_game.h"
#include "appmodule.h"
#include "FuBiPersist.h"
#include "FuBiTraitsImpl.h"
#include "GoCore.h"
#include "GoParty.h"
#include "GoSupport.h"
#include "UICamera.h"
#include "UIGame.h"
#include "Server.h"
#include "siege_engine.h"
#include "siege_mouse_shadow.h"
#include "siege_frustum.h"


FUBI_EXPORT_ENUM( eCameraMode, CMODE_BEGIN, CMODE_COUNT );


static const char* s_CameraModeStrings[] =
{
	"cmode_none",
	"cmode_dolly",
	"cmode_zoom",
	"cmode_orbit",
	"cmode_crane",
	"cmode_track",
	"cmode_nis",
	"cmode_hold",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_CameraModeStrings ) == CMODE_COUNT );

static stringtool::EnumStringConverter s_CameraModeConverter(
		s_CameraModeStrings, CMODE_BEGIN, CMODE_END, CMODE_NONE );

const char* ToString( eCameraMode val )
	{  return ( s_CameraModeConverter.ToString( val ) );  }
bool FromString( const char* str, eCameraMode& val )
	{  return ( s_CameraModeConverter.FromString( str, rcast <int&> ( val ) ) );  }

FUBI_DECLARE_ENUM_TRAITS( eCameraMode, CMODE_NONE );

FUBI_DECLARE_SELF_TRAITS( CameraPosition );


// Camera bounding tolerance (in meters)
const float CAMERA_BOUND_DISTANCE	= 0.85f;

// The name of the track position
const char UI_TRACKPOS_NAME[]		= "ui_trackpos";


// Constructor
UICamera::UICamera()
	: m_CameraMode( CMODE_NONE )
	, m_PreviousMode( CMODE_NONE )
	, m_NormYScreenTrack( 0.0f )
	, m_NormXScreenTrack( 0.0f )
	, m_orbitDegreesPerSecond( RadiansFrom( 60.0f ) )
	, m_azimuthDegreesPerSecond( RadiansFrom( 30.0f ) )
	, m_returnDegreesPerSecond( RadiansFrom( 15.0f ) )
	, m_zoomMetersPerSecond( 8.0f )
	, m_metersUntilTrackChange( 2.0f )
	, m_OrbitOffset( 0.0f )
	, m_ZoomDelta( 0.1f )
	, m_TrackHeightOffset( 0.0f )
	, m_camAvoidanceTime( 0.0f )
	, m_camTiltTime( 0.0f )
	, m_camTiltReturnTime( 0.0f )
	, m_holdLeashDistance( 5.0f )
	, m_bFreeLook( false )
	, m_bSpringLook( false )
	, m_bFollowCam( false )
	, m_bRotateLeft( false )
	, m_bRotateRight( false )
	, m_bRotateUp( false )
	, m_bRotateDown( false )
	, m_bIgnoreInput( false )
	, m_bIgnoreAzimuth( false )
	, m_bIgnoreOrbit( false )
	, m_bIgnoreZoom( false )
	, m_bAutoCorrectingAzimuth( false )
	, m_bAutoCorrectingZoom( false )
	, m_OrbitSpeed( 1.0f )
	, m_AzimuthSpeed( 1.0f )
	, m_bZoomIn( false )
	, m_bZoomOut( false )
	, m_azimuthMod( 0.0f )
	, m_azimuthTotal( 0.0f )
	, m_azimuthModTime( 0.0f )
	, m_azimuthReturnTime( 0.0f )
	, m_zoomMod( 0.0f )
	, m_zoomTotal( 0.0f )
	, m_zoomModTime( 0.0f )
{
}

// Destructor
UICamera::~UICamera()
{
}

bool UICamera::Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_CameraMode",              m_CameraMode              );
	persist.Xfer( "m_PreviousMode",            m_PreviousMode            );
	persist.Xfer( "m_trackPosition",           m_trackPosition           );
	persist.Xfer( "m_teleportPosition",        m_teleportPosition        );
	persist.Xfer( "m_OrbitOffset",             m_OrbitOffset             );
	persist.Xfer( "m_bRotateRight",            m_bRotateRight            );
	persist.Xfer( "m_bRotateLeft",             m_bRotateLeft             );
	persist.Xfer( "m_bRotateUp",               m_bRotateUp               );
	persist.Xfer( "m_bRotateDown",             m_bRotateDown             );
	persist.Xfer( "m_bZoomOut",				   m_bZoomOut				 );
	persist.Xfer( "m_bZoomIn",				   m_bZoomIn				 );
	persist.Xfer( "m_bAutoCorrectingAzimuth",  m_bAutoCorrectingAzimuth  );
	persist.Xfer( "m_bAutoCorrectingZoom",	   m_bAutoCorrectingZoom	 );
	persist.Xfer( "m_bFreeLook",               m_bFreeLook               );
	persist.Xfer( "m_bSpringLook",             m_bSpringLook             );
	persist.Xfer( "m_previousPosition",        m_previousPosition        );
	persist.Xfer( "m_trackDir",                m_trackDir                );
	persist.Xfer( "m_azimuthMod",              m_azimuthMod              );
	persist.Xfer( "m_azimuthTotal",			   m_azimuthTotal			 );
	persist.Xfer( "m_azimuthModTime",		   m_azimuthModTime			 );
	persist.Xfer( "m_azimuthReturnTime",	   m_azimuthReturnTime		 );
	persist.Xfer( "m_OrbitSpeed",              m_OrbitSpeed              );
	persist.Xfer( "m_AzimuthSpeed",            m_AzimuthSpeed            );
	persist.Xfer( "m_zoomMod",                 m_zoomMod                 );
	persist.Xfer( "m_zoomTotal",               m_zoomTotal               );
	persist.Xfer( "m_zoomModTime",			   m_zoomModTime			 );
	persist.Xfer( "m_bFollowCam",              m_bFollowCam              );
	
	// Synchronize the gui with the camera settings
	UIWindow * pTrack = gUIShell.FindUIWindow( "window_camera", "compass_hotpoints" );
	if ( pTrack )
	{
		pTrack->SetVisible( m_CameraMode == CMODE_TRACK ? false : true );
	}

	return ( true );
}

// Resetting
void UICamera::Shutdown()
{
	SetCameraMode( CMODE_NONE );

	m_trackPosition.cameraPos = SiegePos::INVALID;
	m_trackPosition.targetPos = SiegePos::INVALID;

	gCameraAgent.ClearPosition();
}

// Set the current camera mode
void UICamera::SetCameraMode( eCameraMode mode, bool bClearOldOrders )
{
	// Clear any existing orders
	if( bClearOldOrders )
	{
		gCameraAgent.ClearOrders();
	}

	// If CMODE_TRACK was requested, submit a persistent order to pan
	if( m_CameraMode != CMODE_TRACK && mode == CMODE_TRACK )
	{
		if( m_bFollowCam )
		{
			gCameraAgent.SubmitOrder( UI_TRACKPOS_NAME, COR_PAN, 0.0, true );
		}
		else
		{
			gCameraAgent.SubmitOrder( UI_TRACKPOS_NAME, COR_OFFSETPAN, 0.0, true );
		}
	}
	else if( mode == CMODE_NONE || mode == CMODE_HOLD )
	{
		// reset to avoid a pop when start tracking
		CopySiegeCameraToLocal();
		gSiegeEngine.GetCamera().ClearHistory();
		gCameraAgent.SubmitCameraPosition( UI_TRACKPOS_NAME, m_trackPosition );
	}

	// Set the mode to the requested mode
	m_PreviousMode	= m_CameraMode;
	m_CameraMode	= mode;
}

// Updating
void UICamera::Update( float secondsElapsed )
{
	if( m_trackPosition.cameraPos.node == siege::UNDEFINED_GUID ||
		m_trackPosition.targetPos.node == siege::UNDEFINED_GUID )
	{
		CopySiegeCameraToLocal();
	}

	if( gCameraAgent.IsTeleportRequested() )
	{
		// Teleport
		SetTeleportPosition( gCameraAgent.GetTeleportPosition() );
		Teleport();

		// Reset the teleport
		gCameraAgent.ClearTeleportRequest();
	}

	if( m_CameraMode == CMODE_TRACK || m_CameraMode == CMODE_HOLD || m_CameraMode == CMODE_NIS )
	{
		// Setup our tracking position
		SiegePos track_position;

		Go* party = gServer.GetScreenParty();
		if ( party != NULL )
		{
			// Use the screen party's position as our target
			track_position.pos				= party->GetPlacement()->GetWorldPosition();
			track_position.pos.y			+= m_TrackHeightOffset;

			// Get the camera offset
			vector_3 cam_offset				= GetTrackCameraPosition() - GetTrackTargetPosition();
			float angle_diff				= 0.0f;

			if( !m_trackDir.IsZero() && !m_bFreeLook && m_bFollowCam )
			{
				vector_3 cam_diff			= cam_offset;
				cam_diff.y					= 0.0f;
				cam_diff.Normalize();

				vector_3 norm_axis( cam_diff.z, 0.0f, -cam_diff.x );

				// Correct for zero cross
				if( norm_axis.IsZero() )
				{
					norm_axis.Set( 0.0f, 0.0f, 1.0f );
				}
				else
				{
					norm_axis.Normalize();
				}

				// Track dir is a node space direction in the space of m_previousPosition
				// Because of this, we need to convert it into world space before we use it
				matrix_3x3 current_orient( DoNotInitialize );
				SiegeNode* pNode			= gSiegeEngine.IsNodeValid( m_previousPosition.node );
				if( pNode )
				{
					current_orient			= pNode->GetCurrentOrientation();
				}
				else
				{
					current_orient			= matrix_3x3::IDENTITY;
				}

				angle_diff					= NormAngleBetween( cam_diff, current_orient * m_trackDir.RotateY_T( m_OrbitOffset ), norm_axis );

				if( !IsZero( angle_diff ) )
				{
					// Calculate the maximum amount of rotation allowed this frame
					float rot_maximum		= (m_orbitDegreesPerSecond * secondsElapsed) * max( 1.0f, angle_diff < RADIANS_180 ? (angle_diff / m_orbitDegreesPerSecond) : ((RADIANS_360 - angle_diff) / m_orbitDegreesPerSecond) );

					if( (angle_diff > rot_maximum) && (angle_diff < (RADIANS_360 - rot_maximum)) )
					{
						if( angle_diff < RADIANS_180 )
						{
							angle_diff		= rot_maximum;
						}
						else
						{
							angle_diff		= RADIANS_360 - rot_maximum;
						}
					}
				}
			}

			if( !m_bIgnoreInput )
			{
				if( !m_bIgnoreOrbit )
				{
					if( m_bRotateLeft )
					{
						angle_diff					-= ((m_orbitDegreesPerSecond * m_OrbitSpeed) * secondsElapsed);
					}
					if( m_bRotateRight )
					{
						angle_diff					+= ((m_orbitDegreesPerSecond * m_OrbitSpeed) * secondsElapsed);
					}
				}

				if( !m_bIgnoreAzimuth && !m_bAutoCorrectingAzimuth )
				{
					if( m_bRotateUp )
					{
						// Increase azimuth
						m_azimuthMod				= -(m_azimuthDegreesPerSecond * m_AzimuthSpeed) * secondsElapsed;
					}
					if( m_bRotateDown )
					{
						// Decrease azimuth
						m_azimuthMod				= (m_azimuthDegreesPerSecond * m_AzimuthSpeed) * secondsElapsed;
					}
				}

				if( !m_bIgnoreZoom && !m_bAutoCorrectingZoom )
				{
					if( m_bZoomIn )
					{
						// Zoom in
						m_zoomMod					= m_zoomMetersPerSecond * secondsElapsed;
						m_zoomTotal					= 0.0f;
					}
					if( m_bZoomOut )
					{
						m_zoomMod					= -m_zoomMetersPerSecond * secondsElapsed;
						m_zoomTotal					= 0.0f;
					}
				}
			}

			if( m_CameraMode == CMODE_HOLD )
			{
				// See what the distance is between the current camera position
				SiegePos nodalTrackPos;
				nodalTrackPos.FromWorldPos( track_position.pos, party->GetPlacement()->GetPosition().node );

				vector_3 trackDiff	= gSiegeEngine.GetDifferenceVector( m_trackPosition.targetPos, nodalTrackPos );
				float trackDist		= trackDiff.Length();
				if( trackDist > m_holdLeashDistance )
				{
					// Scale the difference vector to bring it into the radius of the leash
					trackDiff.Normalize();
					trackDiff		*= (trackDist - m_holdLeashDistance);

					gpassert( m_trackPosition.targetPos.node == m_trackPosition.cameraPos.node );
					m_trackPosition.targetPos.pos	+= trackDiff;
				}

				// Convert camera tracking position into node space
				m_trackPosition.cameraPos.FromWorldPos( m_trackPosition.targetPos.WorldPos() + cam_offset.RotateY_T( angle_diff ), party->GetPlacement()->GetPosition().node );
				m_trackPosition.targetPos.FromWorldPos( m_trackPosition.targetPos.WorldPos(), party->GetPlacement()->GetPosition().node );
			}
			else
			{
				m_trackPosition.targetPos		= track_position;
				m_trackPosition.cameraPos		= track_position;
				m_trackPosition.cameraPos.pos	+= cam_offset.RotateY_T( angle_diff );

				// Convert camera tracking position into node space
				m_trackPosition.cameraPos.FromWorldPos( m_trackPosition.cameraPos.pos, party->GetPlacement()->GetPosition().node );
				m_trackPosition.targetPos.FromWorldPos( m_trackPosition.targetPos.pos, party->GetPlacement()->GetPosition().node );
			}

			if( m_bFollowCam )
			{
				if( m_previousPosition.node == siege::UNDEFINED_GUID )
				{
					m_previousPosition		= m_trackPosition.targetPos;
				}
				else
				{
					vector_3 diff_vect		= gSiegeEngine.GetDifferenceVector( m_trackPosition.targetPos, m_previousPosition );
					diff_vect.y				= 0.0f;
					if( !diff_vect.IsZero() && (diff_vect.Length() > m_metersUntilTrackChange) )
					{
						// Get the new tracking direction
						m_trackDir			= diff_vect.Normalize_T();
						m_previousPosition	= m_trackPosition.targetPos;
					}
				}
			}

			if( m_CameraMode != CMODE_NIS )
			{
				// Calculate the distance in meters that we need to raise this position to put
				// our track position at a predetermined location at the bottom of the screen.
				ApplyScreenTrackOffsets( secondsElapsed, m_CameraMode == CMODE_HOLD ? false : true );
			}
		}

		gpstring trackpos	= UI_TRACKPOS_NAME;
		gCameraAgent.SubmitCameraPosition( trackpos, m_trackPosition );
	}
	else
	{
		gpstring trackpos	= UI_TRACKPOS_NAME;
		gCameraAgent.SubmitCameraPosition( trackpos, m_trackPosition );
		gCameraAgent.SubmitOrder( trackpos, COR_SNAP );
	}
}

// Mouse movement in X
void UICamera::MouseDeltaX( int deltaX )
{
	// Don't do any work if we are in a no work mode
	if( m_CameraMode == CMODE_NONE	||
		m_CameraMode == CMODE_ZOOM	||
		m_CameraMode == CMODE_CRANE	||
		m_CameraMode == CMODE_NIS )
	{
		return;
	}

	if( m_bIgnoreInput || m_bIgnoreOrbit || gCameraAgent.IsTeleportRequested() )
	{
		return;
	}

	// Setup the basics
	matrix_3x3 cam_orient		= GetTrackPositionOrientation();
	vector_3 cameraDelta;
	vector_3 targetDelta;
	vector_3 cam_diff( DoNotInitialize );

	switch( m_CameraMode )
	{
	case CMODE_DOLLY:
		{
		// Translate horizontally along the x-axis of the current camera
		cameraDelta		= cam_orient.GetColumn0_T();
		cameraDelta		*= (float)deltaX / -30.0f;
		targetDelta		= cameraDelta;
		break;
		}


	case CMODE_ORBIT:
		{
		// Rotate around the target position
		cam_diff		= GetTrackCameraPosition() - GetTrackTargetPosition();
		cam_diff		= cam_diff.RotateY_T( RadiansFrom( (float)deltaX / 3.0f ) );
		cameraDelta		= (GetTrackTargetPosition() + cam_diff) - GetTrackCameraPosition();
		break;
		}

	case CMODE_TRACK:
	case CMODE_HOLD:
		{
		if( m_bFreeLook )
		{
			// Rotate around the target position
			cam_diff		= GetTrackCameraPosition() - GetTrackTargetPosition();
			cam_diff		= cam_diff.RotateY_T( RadiansFrom( (float)deltaX ) );
			cameraDelta		= (GetTrackTargetPosition() + cam_diff) - GetTrackCameraPosition();
		}
		break;
		}
	}

	SetTrackPositionDeltas( cameraDelta, targetDelta );
}

// Mouse movement in Y
void UICamera::MouseDeltaY( int deltaY )
{
	// Don't do any work if we are in a no work mode
	if( m_CameraMode == CMODE_NONE ||
		m_CameraMode == CMODE_NIS )
	{
		return;
	}

	if( m_bIgnoreInput || m_bIgnoreAzimuth || m_bAutoCorrectingAzimuth || gCameraAgent.IsTeleportRequested() )
	{
		return;
	}

	// Setup the basics
	matrix_3x3 cam_orient		= GetTrackPositionOrientation();
	vector_3 cameraDelta;
	vector_3 targetDelta;
	vector_3 cam_diff( DoNotInitialize );
	float deltaTest;

	switch( m_CameraMode )
	{
	case CMODE_DOLLY:
		{
		cameraDelta		= CrossProduct( cam_orient.GetColumn0_T(), vector_3( 0.0f, 1.0f, 0.0f ) );
		cameraDelta		*= (float)deltaY / -30.0f;
		targetDelta		= cameraDelta;
		break;
		}

	case CMODE_ZOOM:
		{
		cameraDelta		= cam_orient.GetColumn2_T();
		cameraDelta		*= (float)deltaY / -30.0f;

		deltaTest		= Length( GetTrackTargetPosition() - (GetTrackCameraPosition() + cameraDelta) );
		if( (deltaTest < 0.1f) && (deltaY < 0) )
		{
			cameraDelta = vector_3( 0.0f, 0.0f, 0.0f );
		}
		else if( (deltaTest > 400.0f) && (deltaY > 0) )
		{
			cameraDelta = vector_3( 0.0f, 0.0f, 0.0f );
		}
		break;
		}

	case CMODE_ORBIT:
		{
		// Calculate the angle of the current azimuth in degrees
		cam_diff		= GetTrackCameraPosition() - GetTrackTargetPosition();
		targetDelta		= cam_diff;
		targetDelta.y	= 0.0f;
		deltaTest		= DegreesFrom( AngleBetweenUnit( Normalize( targetDelta ), Normalize( cam_diff ) ) );
		targetDelta		= vector_3( 0.0f, 0.0f, 0.0f );

		// Check bounds
		float degChange	= (float)deltaY / 5.0f;

		if( (deltaTest + FABSF( degChange )) > 89.0f )
		{
			if( cam_diff.y > 0 && deltaY > 0 )
			{
				cameraDelta	= targetDelta;
			}
			else if( cam_diff.y < 0.0f && deltaY < 0 )
			{
				cameraDelta	= targetDelta;
			}
			else
			{
				cam_diff	= AxisRotationColumns( Normalize( CrossProduct( vector_3( 0.0f, 1.0f, 0.0f ), Normalize( cam_diff ) ) ), RadiansFrom( -degChange ) ) * cam_diff;
				cameraDelta	= (GetTrackTargetPosition() + cam_diff) - GetTrackCameraPosition();
			}
		}
		else
		{
			// Bounds check passed, calculate new azimuth offset
			cam_diff	= AxisRotationColumns( Normalize( CrossProduct( vector_3( 0.0f, 1.0f, 0.0f ), Normalize( cam_diff ) ) ), RadiansFrom( -degChange ) ) * cam_diff;
			cameraDelta	= (GetTrackTargetPosition() + cam_diff) - GetTrackCameraPosition();
		}
		break;
		}

	case CMODE_CRANE:
		{
		// Move along a defined up vector (0,1,0)
		cameraDelta		= vector_3( 0.0f, 1.0f, 0.0f );
		cameraDelta		*= (float)deltaY / -30.0f;
		targetDelta		= cameraDelta;
		break;
		}

	case CMODE_TRACK:
	case CMODE_HOLD:
		{
		if( m_bFreeLook )
		{
			m_azimuthMod		-= RadiansFrom( (float)deltaY / 5.0f );
		}
		break;
		}
	}

	SetTrackPositionDeltas( cameraDelta, targetDelta );
}

// Mouse movement in Z
float UICamera::MouseDeltaZ( int deltaZ )
{
	if( m_bIgnoreInput || m_bIgnoreZoom || gCameraAgent.IsTeleportRequested() )
	{
		return 0.0f;
	}

	// Z movement only affects tracking mode
	if( m_CameraMode == CMODE_TRACK || m_CameraMode == CMODE_HOLD )
	{
		// If we are in auto correction, remove
		m_bAutoCorrectingZoom		= false;
		m_zoomModTime				= 0.0f;
		m_zoomTotal					= 0.0f;

		siege::SiegeCamera& camera	= gSiegeEngine.GetCamera();

		// $$$ use deltaZ for more than just sign - also use its magnitude
		//     to alter the zoom size. we have the ability to scale mouse
		//     sensitivity in x,y,z in appmodule now. -sb

		// Calculate the delta for this zoom
		const float cam_dist		= Length( GetTrackTargetPosition() - GetTrackCameraPosition() );
		float zoom_delta			= m_ZoomDelta * cam_dist;

		if( (deltaZ > 0) && (cam_dist > camera.GetMinDistance()) )
		{
			// Zoom in control
			if( cam_dist - zoom_delta < camera.GetMinDistance() )
			{
				zoom_delta			= cam_dist - camera.GetMinDistance();
			}
		}
		else if( (deltaZ < 0) && (cam_dist < camera.GetMaxDistance()) )
		{
			// Zoom out control
			if( cam_dist + zoom_delta > camera.GetMaxDistance() )
			{
				zoom_delta			= camera.GetMaxDistance() - cam_dist;
			}
			zoom_delta				= -zoom_delta;
		}
		else
		{
			zoom_delta				= 0.0f;
		}

		// Zoom in
		m_zoomMod					+= zoom_delta;
		return zoom_delta;
	}

	return 0.0f;
}

// Tell the camera about a large teleport so it doesn't go crazy
void UICamera::Teleport()
{
	// Clear existing camera information as it will not be relevant after the teleport
	gCameraAgent.ClearOrders();
	gCameraAgent.ClearCameraPositionCache();
	gSiegeEngine.GetCamera().ClearHistory();

	// Set the new teleport position
	m_trackPosition	= m_teleportPosition;
	m_trackPosition.targetPos.pos.y	+= m_TrackHeightOffset;
	m_trackPosition.cameraPos.pos.y += m_TrackHeightOffset;

	// Adjust to account for offsets
	ApplyScreenTrackOffsets( 0.0f, true );

	// Set the new track position
	gSiegeEngine.GetCamera().SetCameraAndTargetPosition( m_trackPosition.cameraPos, m_trackPosition.targetPos );

	// Submit the new tracking position
	gpstring trackpos	= UI_TRACKPOS_NAME;
	gCameraAgent.SubmitCameraPosition( trackpos, m_trackPosition );

	// Setup a snap order to put the camera in the new location
	gCameraAgent.SubmitOrder( trackpos, COR_PAN );

	// Resubmit tracking orders if it is required
	if( m_CameraMode == CMODE_TRACK || m_CameraMode == CMODE_HOLD )
	{
		gCameraAgent.SubmitOrder( trackpos, COR_OFFSETPAN, 0.0, true );
	}
}

// Follow cam
void UICamera::SetFollowCam( const bool bFollow )
{
	m_bFollowCam	= bFollow;

	if( m_CameraMode == CMODE_TRACK || m_CameraMode == CMODE_HOLD )
	{
		if( m_bFollowCam )
		{
			// Submit a pan order
			gCameraAgent.ClearOrders();
			gCameraAgent.SubmitOrder( UI_TRACKPOS_NAME, COR_PAN, 0.0, true );
		}
		else
		{
			// Submit an offsetpan order
			gCameraAgent.ClearOrders();
			gCameraAgent.SubmitOrder( UI_TRACKPOS_NAME, COR_OFFSETPAN, 0.0, true );
		}
	}
}

// Change the camera's free look mode
void UICamera::SetFreeLook( const bool bLook )
{
	// Set our state variable
	m_bFreeLook		= bLook;

	// Setup
	if( bLook )
	{
		// Lock the mouse
		gAppModule.SetMouseModeFixed();

		// Change the camera mode to offset pan
		if( m_CameraMode == CMODE_TRACK || m_CameraMode == CMODE_HOLD )
		{
			if( m_bFollowCam )
			{
				// Save the current camera position
				CopySiegeCameraToLocal();

				// Setup a new offset pan
				gCameraAgent.ClearOrders();
				gpstring trackpos	= UI_TRACKPOS_NAME;
				gCameraAgent.SubmitCameraPosition( trackpos, m_trackPosition );
				gCameraAgent.SubmitOrder( trackpos, COR_OFFSETPAN, 0.0, true );
			}
		}
		else
		{
			// Set the camera mode to orbit
			SetCameraMode( CMODE_ORBIT );
		}
	}
	else
	{
		if( m_CameraMode == CMODE_ORBIT )
		{
			if( m_PreviousMode == CMODE_NONE )
			{
				gAppModule.SetMouseModeFixed( false );
			}

			// Restore the camera to its previous mode
			SetCameraMode( m_PreviousMode );
		}
		else
		{
			gAppModule.SetMouseModeFixed( false );

			if( m_CameraMode == CMODE_TRACK || m_CameraMode == CMODE_HOLD )
			{
				if( m_bFollowCam )
				{
					// Weight the history
					gSiegeEngine.GetCamera().FillCameraHistoryWithCurrent();
					gCameraAgent.ResetCameraAffectorModification();

					// Setup a new offset pan
					gCameraAgent.ClearOrders();
					gpstring trackpos	= UI_TRACKPOS_NAME;
					gCameraAgent.SubmitOrder( trackpos, COR_PAN, 0.0, true );

					// Figure out our orbit modifier
					if( !m_bSpringLook )
					{
						// Get the camera offset
						vector_3 cam_offset				= GetTrackCameraPosition() - GetTrackTargetPosition();

						if( !m_trackDir.IsZero() && !m_bFreeLook )
						{
							vector_3 cam_diff			= cam_offset;
							cam_diff.y					= 0.0f;
							cam_diff.Normalize();

							vector_3 norm_axis( cam_diff.z, 0.0f, -cam_diff.x );

							// Correct for zero cross
							if( norm_axis.IsZero() )
							{
								norm_axis.Set( 0.0f, 0.0f, 1.0f );
							}
							else
							{
								norm_axis.Normalize();
							}

							matrix_3x3 current_orient( DoNotInitialize );
							SiegeNode* pNode			= gSiegeEngine.IsNodeValid( m_previousPosition.node );
							if( pNode )
							{
								current_orient			= pNode->GetCurrentOrientation();
							}
							else
							{
								current_orient			= matrix_3x3::IDENTITY;
							}

							m_OrbitOffset				= NormAngleBetween( cam_diff, current_orient * m_trackDir, -norm_axis );
						}
					}
					else
					{
						m_OrbitOffset	= 0.0f;
					}
				}
			}
		}
	}
}

// Get the current world space orientation of the track position
matrix_3x3 UICamera::GetTrackPositionOrientation()
{
	return( MatrixFromDirection( GetTrackTargetPosition() - GetTrackCameraPosition() ) );
}

// Get the current camera track position (in world space)
vector_3 UICamera::GetTrackCameraPosition()
{
	// Get the node
	SiegeNode* pNode	= gSiegeEngine.IsNodeValid( m_trackPosition.cameraPos.node );
	if( pNode )
	{
		// Convert to world space
		return pNode->NodeToWorldSpace( m_trackPosition.cameraPos.pos );
	}
	else
	{
		return m_trackPosition.cameraPos.pos;
	}
}

// Get the current target track position (in world space)
vector_3 UICamera::GetTrackTargetPosition()
{
	// Get the node
	SiegeNode* pNode	= gSiegeEngine.IsNodeValid( m_trackPosition.targetPos.node );
	if( pNode )
	{
		// Convert to world space
		return pNode->NodeToWorldSpace( m_trackPosition.targetPos.pos );
	}
	else
	{
		return m_trackPosition.targetPos.pos;
	}
}

// Use world space deltas to modify the current tracking position
void UICamera::SetTrackPositionDeltas( const vector_3& cameraDelta, const vector_3& targetDelta )
{
	// Calculate our new position
	vector_3 nCameraPos	= GetTrackCameraPosition() + cameraDelta;
	vector_3 nTargetPos	= GetTrackTargetPosition() + targetDelta;

	if( gSiegeEngine.IsNodeValid( m_trackPosition.cameraPos.node ) )
	{
		m_trackPosition.cameraPos.FromWorldPos( nCameraPos, m_trackPosition.cameraPos.node );
	}
	else
	{
		m_trackPosition.cameraPos.pos	= nCameraPos;
	}

	if( gSiegeEngine.IsNodeValid( m_trackPosition.targetPos.node ) )
	{
		m_trackPosition.targetPos.FromWorldPos( nTargetPos, m_trackPosition.targetPos.node );
	}
	else
	{
		m_trackPosition.targetPos.pos	= nTargetPos;
	}
}

void UICamera::CopySiegeCameraToLocal()
{
	// If the track point is invalid, set it up using the current position of the camera
	m_trackPosition.cameraPos.node	= m_trackPosition.targetPos.node	= gSiegeEngine.NodeWalker().TargetNodeGUID();
	m_trackPosition.cameraPos.pos	= gSiegeEngine.GetCamera().GetCameraPosition();
	m_trackPosition.targetPos.pos	= gSiegeEngine.GetCamera().GetTargetPosition();
}

void UICamera::ApplyScreenTrackOffsets( float secondsElapsed, bool bApplyOffset )
{
	// Get our camera location offset vector
	SiegePos target_pos		= m_trackPosition.targetPos;
	vector_3 worldCamTrack	= m_trackPosition.cameraPos.WorldPos();
	vector_3 worldTarTrack	= m_trackPosition.targetPos.WorldPos();
	vector_3 cam_offset		= worldCamTrack - worldTarTrack;
	matrix_3x3 orient		= MatrixFromDirection( cam_offset );
	vector_3 offsetVect;

	if( !cam_offset.IsZero() )
	{
		bool bAutoAzimuth	= false;

		// Clear azimuth timer if the user has requested some azimuth change or is in freelook mode
		if( !IsZero( m_azimuthMod ) || m_bFreeLook )
		{
			m_azimuthModTime	= 0.0f;
			m_azimuthReturnTime	= 0.0f;
			m_azimuthTotal		= 0.0f;
		}

ANGLEZOOMCHECK:

		bool bUserZoomOut	= false;

		// Calculate the azimuth angle difference between the requested point and the up vector
		float azimuth_diff	= AngleBetween( (worldCamTrack - worldTarTrack), vector_3::UP );

		// Calculate the maximum allowable difference
		float max_diff		= RadiansFrom( 90 - gSiegeEngine.GetCamera().GetMaxAzimuth() );
		float min_diff		= RadiansFrom( 90 - gSiegeEngine.GetCamera().GetMinAzimuth() );

		if( (azimuth_diff + m_azimuthMod) < max_diff )
		{
			m_azimuthMod	= max_diff - azimuth_diff;
		}

		if( (azimuth_diff + m_azimuthMod) > min_diff )
		{
			m_azimuthMod	= min_diff - azimuth_diff;
		}

		if( !m_bAutoCorrectingZoom )
		{
			// Calculate the length of the distance between the requested point and the up vector
			float zoom_dist		= Length( worldCamTrack - worldTarTrack );

			if( (zoom_dist - m_zoomMod) < gSiegeEngine.GetCamera().GetMinDistance() )
			{
				m_zoomMod		= zoom_dist - gSiegeEngine.GetCamera().GetMinDistance();
			}

			if( (zoom_dist - m_zoomMod) > gSiegeEngine.GetCamera().GetMaxDistance() )
			{
				m_zoomMod		= zoom_dist - gSiegeEngine.GetCamera().GetMaxDistance();
			}

			if( IsNegative( m_zoomMod, 0.001f ) )
			{
				bUserZoomOut	= true;
			}
		}

		// Build an offset vect that represents our azimuth/zoom changes for the camera
		vector_3 azi_zoom_offset		= worldCamTrack - worldTarTrack;
		orient							= MatrixFromDirection( azi_zoom_offset );

		// Add in any azimuth adjustment
		if( !IsZero( m_azimuthMod ) )
		{
			azi_zoom_offset				= AxisRotationColumns( orient.GetColumn0_T(), m_azimuthMod ) * azi_zoom_offset;
		}

		// Add in any zoom adjustment
		if( !IsZero( m_zoomMod ) )
		{
			azi_zoom_offset				= azi_zoom_offset - (Normalize( azi_zoom_offset ) * m_zoomMod);
		}

		// Clear modification indicators
		m_azimuthMod					= 0.0f;
		m_zoomMod						= 0.0f;

		// Apply our offset to the camera position
		// Build camera position
		worldCamTrack					= worldTarTrack + azi_zoom_offset;
		m_trackPosition.cameraPos.FromWorldPos( worldCamTrack, m_trackPosition.cameraPos.node );
		m_trackPosition.targetPos.FromWorldPos( worldTarTrack, m_trackPosition.targetPos.node );

		if( !bAutoAzimuth )
		{
			if( CheckForTerrainOcclusion( worldCamTrack, target_pos.WorldPos(), false, true ) )
			{
				m_azimuthReturnTime		= 0.0f;
				m_azimuthModTime		+= secondsElapsed;
				if( m_azimuthModTime > m_camTiltTime  )
				{
					m_azimuthMod		-= secondsElapsed * m_azimuthDegreesPerSecond;
					m_azimuthTotal		+= secondsElapsed * m_azimuthDegreesPerSecond;
					bAutoAzimuth		= true;
					goto ANGLEZOOMCHECK;
				}
			}
			else
			{
				m_azimuthModTime		= 0.0f;
				m_azimuthReturnTime		+= secondsElapsed;
				if( m_azimuthReturnTime > m_camTiltReturnTime && IsPositive( m_azimuthTotal ) )
				{
					float degreeChange	= secondsElapsed * m_returnDegreesPerSecond;
					vector_3 new_offset	= worldTarTrack + (AxisRotationColumns( orient.GetColumn0_T(), degreeChange ) * azi_zoom_offset);
					if( !CheckForTerrainOcclusion( new_offset, target_pos.WorldPos(), false, true ) )
					{
						m_azimuthMod		+= degreeChange;
						m_azimuthTotal		-= degreeChange;
						if( m_azimuthTotal < 0.0f )
						{
							m_azimuthMod	+= m_azimuthTotal;
							m_azimuthTotal	= 0.0f;
						}
						bAutoAzimuth		= true;
						goto ANGLEZOOMCHECK;
					}
				}
			}
		}

		bool bZoomBackAttempt	= false;

COLLISIONCHECK:

		// Get world space camera vectors
		vector_3 worldCam		= m_trackPosition.cameraPos.WorldPos();
		worldCamTrack			= worldCam;
		worldTarTrack			= target_pos.WorldPos();

		if( CheckForTerrainOcclusion( worldCamTrack, worldTarTrack, true, false ) )
		{
			if( bUserZoomOut )
			{
				worldCamTrack	= m_trackPosition.cameraPos.WorldPos();
				worldTarTrack	= m_trackPosition.targetPos.WorldPos();
				m_azimuthMod	-= secondsElapsed * (m_azimuthDegreesPerSecond * 2.0f);
				goto ANGLEZOOMCHECK;
			}
		}

		// Convert modified camera tracking values back to node space
		SiegePos nCameraPos;
		nCameraPos.FromWorldPos( worldCamTrack, m_trackPosition.cameraPos.node );

		// Get the distance between current camera location and new location
		float camLen	= Length( m_trackPosition.cameraPos.pos - nCameraPos.pos );
		if( !IsZero( camLen ) )
		{
			// Some distance has been requested, which we can only honor in two cases.  Either the
			// camera delay timer has passed, or the camera is current in a modified state.
			m_zoomModTime	+= secondsElapsed;
			if( (m_zoomModTime > m_camAvoidanceTime) || m_bAutoCorrectingZoom )
			{
				// Set the new track position
				m_trackPosition.cameraPos	= nCameraPos;

				// Indicate that we are in auto correction
				m_bAutoCorrectingZoom		= true;
				m_zoomTotal					+= camLen;
			}
		}
		else
		{
			// Nothing blocking the camera
			m_zoomModTime	= 0.0f;
			if( m_bAutoCorrectingZoom )
			{
				if( !bZoomBackAttempt )
				{
					// Attempt to zoom back out
					float zoomAmount	= min_t( m_zoomTotal, m_zoomMetersPerSecond * secondsElapsed );
					m_zoomTotal			-= zoomAmount;
					worldCamTrack		= (m_trackPosition.cameraPos.WorldPos() + (Normalize( worldCamTrack - worldTarTrack ) * zoomAmount));
					m_trackPosition.cameraPos.FromWorldPos( worldCamTrack, m_trackPosition.cameraPos.node );

					// Repeat collision check
					bZoomBackAttempt	= true;
					goto COLLISIONCHECK;
				}

				// If successfully zoomed all the way back to original spot, the no more correction
				// is needed
				if( IsZero( m_zoomTotal ) )
				{
					m_bAutoCorrectingZoom	= false;
				}
			}
		}

		if( bApplyOffset )
		{
			// First we need to calculate our distance to the screen
			float half_height	= (float)gSiegeEngine.GetCamera().GetViewportHeight() * 0.5f;
			float half_width	= (float)gSiegeEngine.GetCamera().GetViewportWidth() * 0.5f;

			float depth			= half_height / tanf( (gSiegeEngine.GetCamera().GetFieldOfView() * 0.5f) );

			// Now we need to figure out our offsets to the desired screen location
			float scr_ratio		= (float)gSiegeEngine.GetCamera().GetViewportHeight() / (float)gSiegeEngine.GetCamera().GetViewportWidth();
			float ytrack		= (m_NormYScreenTrack * half_height) * scr_ratio;
			float xtrack		= (m_NormXScreenTrack * half_width) * scr_ratio;

			// With this information, we can calculate our new angle of incidence with the camera
			float angleToYTrack	= atanf( ytrack / depth );
			float angleToXTrack = atanf( xtrack / depth );

			// Using our new angle, we can plug it in given the distance from camera to target
			// to determine how many meters we need to move the camera
			float cam_off_len	= cam_offset.Length();
			float yMetersToMove	= cam_off_len * SINF( -angleToYTrack );
			float xMetersToMove	= cam_off_len * SINF( -angleToXTrack );

			// Alright!  Now, using our current camera basis vectors, we multiply our distance
			// to move in and voila!
			offsetVect			= orient.GetColumn1_T() * yMetersToMove;
			offsetVect			+= orient.GetColumn0_T() * xMetersToMove;

			DEBUG_FLOAT_VALIDATE( offsetVect.x );
			DEBUG_FLOAT_VALIDATE( offsetVect.y );
			DEBUG_FLOAT_VALIDATE( offsetVect.z );

			worldTarTrack	+= offsetVect;
			worldCamTrack	+= offsetVect;
			m_trackPosition.cameraPos.FromWorldPos( worldCamTrack, m_trackPosition.cameraPos.node );
			m_trackPosition.targetPos.FromWorldPos( worldTarTrack, m_trackPosition.targetPos.node );
		}
	}
	else
	{
		gpassertm( 0, "Cannot apply screen track because camera offset is zero!" );
	}
}

// Track the given positions through potentially camera blocking geometry
bool UICamera::CheckForTerrainOcclusion( vector_3& worldCamPos, const vector_3& worldTarPos, bool bModifyPos, bool bAvoidCam )
{
	siege::SiegeFrustum* pRenderFrustum		= gSiegeEngine.GetFrustum( gSiegeEngine.GetRenderFrustum() );
	if( pRenderFrustum )
	{
		siege::FrustumNodeColl& camCollideList	= pRenderFrustum->GetFrustumNodeColl();
		if( !camCollideList.empty() )
		{
			vector_3 test_direction			= worldCamPos - worldTarPos;
			vector_3 dist_direction			= test_direction.Normalize_T() * CAMERA_BOUND_DISTANCE;
			test_direction					+= dist_direction;
			float current_ray_t				= FLOAT_MAX;

			for( siege::FrustumNodeColl::iterator i = camCollideList.begin(); i != camCollideList.end(); ++i )
			{
				siege::SiegeNode& node		= **i;

				if( (node.GetNodeAlpha() != 255) || 
					(!node.GetBoundsCamera() && !bAvoidCam) ||
					(!node.GetOccludesCamera() && bAvoidCam) )
				{
					continue;
				}

				float ray_t					= FLOAT_MAX;
				vector_3 normal( DoNotInitialize );

				vector_3 node_ray_origin	= node.WorldToNodeSpace( worldTarPos );
				vector_3 node_ray_dir		= node.GetTransposeOrientation() * test_direction;
				if( node.HitTest( node_ray_origin, node_ray_dir, ray_t, normal, siege::LF_IS_FLOOR | siege::LF_IS_WALL ) )
				{
					if( ray_t < current_ray_t && ray_t <= (1.0f+FLOAT_TOLERANCE) )
					{
						vector_3 tarOffset	= test_direction * ray_t;
						float tarOffLen		= tarOffset.Length();
						if( tarOffLen < (CAMERA_BOUND_DISTANCE*2.0f) )
						{
							tarOffset		*= ((CAMERA_BOUND_DISTANCE*2.0f) / tarOffLen);
						}

						if( bModifyPos )
						{
							worldCamPos			= (worldTarPos + tarOffset) - dist_direction;
							current_ray_t		= ray_t;
						}
						else
						{
							return true;
						}
					}
				}
			}

			if( current_ray_t < FLOAT_MAX )
			{
				return true;
			}
		}
	}

	return false;
}
