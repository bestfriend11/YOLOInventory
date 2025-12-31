#pragma once
#ifndef _UICAMERA_H_
#define _UICAMERA_H_

/**************************************************************************
**
**							UICamera
**
**		Handles user and game interaction with the camera operator.
**
**		Author: James Loe, Chad Queen
**		Date:	04/15/00
**
**************************************************************************/

#include "FuBiDefs.h"
#include "CameraAgent.h"


enum eCameraMode
{
	SET_BEGIN_ENUM( CMODE_, 0 ),

	CMODE_NONE		= 0,
	CMODE_DOLLY		= 1,
	CMODE_ZOOM		= 2,
	CMODE_ORBIT		= 3,
	CMODE_CRANE		= 4,
	CMODE_TRACK		= 5,
	CMODE_NIS		= 6,
	CMODE_HOLD		= 7,

	SET_END_ENUM( CMODE_ ),
	CMODE_DWORDALIGN	= 0x7FFFFFFF,
};


const char* ToString  ( eCameraMode val );
bool        FromString( const char* str, eCameraMode& val );


class UICamera : public Singleton <UICamera>
{
public:

	// Constructor
	UICamera();

	// Destructor
	~UICamera();

	// Persistence
	bool 					Xfer( FuBi::PersistContext& persist );

	// Resetting
	void					Shutdown();

	// Toggle modes of camera usage
FEX void					SetCameraMode( eCameraMode mode, bool bClearOldOrders );
FEX void					SetCameraMode( eCameraMode mode )						{ SetCameraMode( mode, true ); }
FEX eCameraMode				GetCameraMode() const									{ return m_CameraMode; }
FEX eCameraMode				GetPreviousMode() const									{ return m_PreviousMode; }

	// Functions for changing the min/max camera distances
FEX void					SetMinDistance( float min_dist )						{ gSiegeEngine.GetCamera().SetMinDistance( min_dist ); }
FEX void					SetMaxDistance( float max_dist )						{ gSiegeEngine.GetCamera().SetMaxDistance( max_dist ); }

	// Info about normalized screen track locations
	void					SetYScreenTrack( const float ytrack )					{ m_NormYScreenTrack = ytrack; }
	float					GetYScreenTrack()										{ return m_NormYScreenTrack; }

	void					SetXScreenTrack( const float xtrack )					{ m_NormXScreenTrack = xtrack; }
	float					GetXScreenTrack()										{ return m_NormXScreenTrack; }

	// Orbit change degrees
	void					SetOrbitDegreesPerSecond( const float deg )				{ m_orbitDegreesPerSecond = deg; }
	float					GetOrbitDegreesPerSecond()								{ return m_orbitDegreesPerSecond; }

	// Azimuth change degrees
	void					SetAzimuthDegreesPerSecond( const float deg )			{ m_azimuthDegreesPerSecond = deg; }
	float					GetAzimuthDegreesPerSecond()							{ return m_azimuthDegreesPerSecond; }
	void					SetReturnDegreesPerSecond( const float deg )			{ m_returnDegreesPerSecond = deg; }
	float					GetReturnDegreesPerSecond()								{ return m_returnDegreesPerSecond; }

	// Zoom change meters
	void					SetZoomMetersPerSecond( const float meters )			{ m_zoomMetersPerSecond = meters; }
	float					GetZoomMetersPerSecond()								{ return m_zoomMetersPerSecond; }

	// Movement amount before direction modification
	void					SetMetersUntilTrackChange( const float met )			{ m_metersUntilTrackChange = met; }
	float					GetMetersUntilTrackChange()								{ return m_metersUntilTrackChange; }

	// Zoom delta percentage
	void					SetZoomDelta( const float delta )						{ m_ZoomDelta = delta; }
	float					GetZoomDelta()											{ return m_ZoomDelta; }

	// Tracking height offset
	void					SetTrackHeightOffset( const float offset )				{ m_TrackHeightOffset = offset; }
	float					GetTrackHeightOffset()									{ return m_TrackHeightOffset; }

	// Camera avoidance buffer time
	void					SetCameraAvoidanceTime( const float time )				{ m_camAvoidanceTime = time; }
	float					GetCameraAvoidanceTime()								{ return m_camAvoidanceTime; }

	// Camera tilt buffer time
	void					SetCameraTiltTime( const float time )					{ m_camTiltTime = time; }
	float					GetCameraTiltTime()										{ return m_camTiltTime; }

	// Camera tilt return buffer time
	void					SetCameraTiltReturnTime( const float time )				{ m_camTiltReturnTime = time; }
	float					GetCameraTiltReturnTime()								{ return m_camTiltReturnTime; }

	// Hold mode camera leash distance
	void					SetHoldLeashDistance( const float dist )				{ m_holdLeashDistance = dist; }
	float					GetHoldLeashDistance()									{ return m_holdLeashDistance; }

	// Toggle left/right rotation
	void					SetRotateLeft( const bool rotate, const float speed )	{ m_bRotateLeft = rotate; m_OrbitSpeed = speed; }
	bool					IsRotatingLeft()										{ return m_bRotateLeft; }

	void					SetRotateRight( const bool rotate, const float speed )	{ m_bRotateRight = rotate; m_OrbitSpeed = speed; }
	bool					IsRotatingRight()										{ return m_bRotateRight; }

	void					SetRotateUp( const bool rotate, const float speed )		{ m_bRotateUp = rotate; m_AzimuthSpeed = speed; }
	bool					IsRotatingUp()											{ return m_bRotateUp; }

	void					SetRotateDown( const bool rotate, const float speed )	{ m_bRotateDown = rotate; m_AzimuthSpeed = speed; }
	bool					IsRotatingDown()										{ return m_bRotateDown; }

	void					SetZoomingIn( const bool zoom )							{ m_bZoomIn = zoom; }
	bool					IsZoomingIn()											{ return m_bZoomIn; }

	void					SetZoomingOut( const bool zoom )						{ m_bZoomOut = zoom; }
	bool					IsZoomingOut()											{ return m_bZoomOut; }

	// Updating
	void					Update( float secondsElapsed );

	// Tell the camera about user mouse usage
	void					MouseDeltaX( int deltaX );
	void					MouseDeltaY( int deltaY );

	// Returns how much of a modification (in meters) was made
	float					MouseDeltaZ( int deltaZ );

	// Tell the camera about a large teleport so it doesn't go crazy
	void					Teleport();
	void					SetTeleportPosition( const CameraPosition& pos, const SiegePos& posIdentifier = SiegePos() ){ m_teleportPosition = pos; m_teleportPositionId = posIdentifier; }

	// Queries to make sure we are using the correct teleport location!
	CameraPosition			GetTeleportPosition()	{ return m_teleportPosition; }
	SiegePos				GetTeleportPositionId()	{ return m_teleportPositionId; }

	// Put the camera in free look mode
	void					SetFreeLook( const bool bLook );
	bool					GetFreeLook()									{ return m_bFreeLook; }

	// Spring look
	void					SetSpringLook( const bool bSpring )				{ m_bSpringLook = bSpring; }
	bool					GetSpringLook()									{ return m_bSpringLook; }

	// Follow cam
	void					SetFollowCam( const bool bFollow );
	bool					GetFollowCam()									{ return m_bFollowCam; }

	// Input monitoring
	void					SetIgnoreInput( const bool bIgnore )			{ m_bIgnoreInput = bIgnore; }
	bool					GetIgnoreInput()								{ return m_bIgnoreInput; }

	void					SetIgnoreAzimuth( const bool bIgnore )			{ m_bIgnoreAzimuth = bIgnore; }
	bool					GetIgnoreAzimuth()								{ return m_bIgnoreAzimuth; }

	void					SetIgnoreOrbit( const bool bIgnore )			{ m_bIgnoreOrbit = bIgnore; }
	bool					GetIgnoreOrbit()								{ return m_bIgnoreOrbit; }

	void					SetIgnoreZoom( const bool bIgnore )				{ m_bIgnoreZoom = bIgnore; }
	bool					GetIgnoreZoom()									{ return m_bIgnoreZoom; }

private:

// Private utility.

	// Get the current world space orientation of the track position
	matrix_3x3				GetTrackPositionOrientation();

	// Get the current positions (in world space) of the trackposition
	vector_3				GetTrackCameraPosition();
	vector_3				GetTrackTargetPosition();

	// Use world space deltas to modify the current tracking position
	void					SetTrackPositionDeltas( const vector_3& cameraDelta, const vector_3& targetDelta );

	// Copy current siege cam position local
	void					CopySiegeCameraToLocal();

	// Apply the screen tracks to the current position.  Returns whether or not the camera should be terrain bounded.
	void					ApplyScreenTrackOffsets( float secondsElapsed, bool bApplyOffset );

	// Track the given positions through potentially camera blocking geometry
	bool					CheckForTerrainOcclusion( vector_3& worldCamPos, const vector_3& worldTarPos, bool bModifyPos, bool bAvoidCam );

// Data.

	// Mode that the camera is currently in
	eCameraMode				m_CameraMode;
	eCameraMode				m_PreviousMode;

	// Current track position
	CameraPosition			m_trackPosition;

	// Current teleport position
	CameraPosition			m_teleportPosition;
	// Teleport Id is set to the position of the last teleport
	// this way we can check to make sure the camera pos is valid
	SiegePos				m_teleportPositionId;

	// Normalized screen space camera tracking locations
	float					m_NormXScreenTrack;
	float					m_NormYScreenTrack;

	// Degrees that the camera can orbit in one second
	float					m_orbitDegreesPerSecond;

	// Degrees that the camera can change azimuth in one second
	float					m_azimuthDegreesPerSecond;
	float					m_returnDegreesPerSecond;

	// Meters that the camera can zoom in one second
	float					m_zoomMetersPerSecond;

	// Meters before tracking direction change
	float					m_metersUntilTrackChange;

	// Orbit offset.  Used for maintaining a user offset position.
	float					m_OrbitOffset;

	// Normalized percentage to scale the zoom factor
	float					m_ZoomDelta;

	// Tracking height offset specified in meters
	float					m_TrackHeightOffset;

	// Amount of time to buffer camera avoidance
	float					m_camAvoidanceTime;

	// Amount of time to buffer camera tilt
	float					m_camTiltTime;

	// Amount of time to buffer camera tilt return
	float					m_camTiltReturnTime;

	// Distance of the hold mode leash
	float					m_holdLeashDistance;

	// Rotation/Zoom toggles
	bool					m_bRotateRight;
	bool					m_bRotateLeft;
	bool					m_bRotateUp;
	bool					m_bRotateDown;
	bool					m_bZoomOut;
	bool					m_bZoomIn;

	bool					m_bIgnoreInput;
	bool					m_bIgnoreAzimuth;
	bool					m_bIgnoreOrbit;
	bool					m_bIgnoreZoom;

	bool					m_bAutoCorrectingAzimuth;
	bool					m_bAutoCorrectingZoom;

	// Rotation speeds
	float					m_OrbitSpeed;
	float					m_AzimuthSpeed;

	// Look mode info
	bool					m_bFreeLook;
	bool					m_bSpringLook;

	// Follow cam mode
	bool					m_bFollowCam;

	// Tracking informatin
	SiegePos				m_previousPosition;
	vector_3				m_trackDir;

	// Amount of requested azimuth change
	float					m_azimuthMod;
	float					m_azimuthTotal;
	float					m_azimuthModTime;
	float					m_azimuthReturnTime;

	// Amount of requested zoom change
	float					m_zoomMod;
	float					m_zoomTotal;
	float					m_zoomModTime;

	// Node cache
	siege::SiegeNodeColl	m_nodeStack;

	// Singleton
	FUBI_SINGLETON_CLASS( UICamera, "User interface controller for the camera." );
	SET_NO_COPYING( UICamera );
};

#define gUICamera UICamera::GetSingleton()


#endif
