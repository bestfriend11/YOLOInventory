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



#pragma once
#ifndef __EDITORCAMERA_H
#define __EDITORCAMERA_H


// Include Files
#include "CameraPosition.h"
#include <set>
#include "SiegeEditorTypes.h"
#include "GoDefs.h"


// Enumerated Data Types

// Specifies the different camera scrolling directions
enum SCROLL_DIRECTION {
	N,
	NW,
	NE,
	S,
	SW,
	SE,
	E,
	W,
};


class EditorCamera : public Singleton <EditorCamera>
{
public:
	// Public Methods

	// Constructor
	EditorCamera( siege::SiegeCamera *camera );

	// Main Update Loop
	void Update();

	// Scroll Camera
	void UpdateScrolling( int x, int y, bool bShiftDown = false );
	void Scroll( SCROLL_DIRECTION direction, float magnitude );

	// Rotate Camera
	void Rotate( int x, int y );

	// Zoom In/Out Camera
	void Zoom( SCROLL_DIRECTION direction );
	void UpdateZoom( SCROLL_DIRECTION direction, double zoom_magnitude = -1 );

	// Set/Get Scroll Rate
	int		GetScrollRate()						{ return m_scrollrate; };
	void	SetScrollRate( int scrollrate )		{ m_scrollrate = scrollrate; };
	double	GetRotateRate()						{ return m_scrollMagnitude; };
	void	SetRotateRate( float scroll_mag )	{ m_scrollMagnitude = scroll_mag; };
	double	GetZoomRate()						{ return m_zoomMagnitude; };
	void	SetZoomRate( float zoom_mag )		{ m_zoomMagnitude = zoom_mag; };

	// Set its position
	void	SetPosition( float x, float y, float z );
	void	SetTargetPositionNode( siege::database_guid guid );	

	// Reset Camera
	void Reset();

	// Angle View
	void AngleView();

	// Overhead View
	void OverheadView();

	// Side View
	void SideView();

	// Get the current camera position
	vector_3	GetPosition()	{ return m_targetPos; };
	float		GetOrbit()		{ return m_orbit; };
	float		GetAzimuth()	{ return m_azimuth; };
	float		GetDistance()	{ return m_cameraDistance; };

	// Set camera information
	void		SetTargetPosition( const vector_3& pos )			{ m_targetPos = pos; }
	void		SetOrbitAngle( float orbit )						{ m_orbit = orbit; }
	void		SetAzimuthAngle( float azimuth )					{ m_azimuth = azimuth; }
	void		SetTargetNodePosition( siege::database_guid node )	{ m_node = node; }
	void		SetDistance( float distance )						{ m_cameraDistance = distance; }

	void		SetStartingMapCamera();

	CameraEulerPosition GetCurrentCameraPosition();
	
	void		SetCameraXScrollPos( int x )	{ m_x_scroll_pos = x; }
	int			GetCameraXScrollPos()			{ return m_x_scroll_pos; }
	void		SetCameraYScrollPos( int y )	{ m_y_scroll_pos = y; }
	int			GetCameraYScrollPos()			{ return m_y_scroll_pos; }

	void		SetCameraXRotatePos( int x )	{ m_x_rotate_pos = x; }
	int			GetCameraXRotatePos()			{ return m_x_rotate_pos; }
	void		SetCameraYRotatePos( int y )	{ m_y_rotate_pos = y; }
	int			GetCameraYRotatePos()			{ return m_y_rotate_pos; }

	void SetGoCameraToEditorCamera( Go * pObject );
	void SetEditorCameraToGoCamera( Go * pObject );
	void SetCameraTargetToGo( Goid object );
	
	void DrawCameraTarget();

private:
	// Private Methods

	// Private Member Variables
	siege::SiegeCamera	*m_camera;
	float				m_orbit;
	float				m_azimuth;
	float				m_cameraDistance;
	vector_3			m_targetPos;
	int					m_scrollrate;
	siege::database_guid	m_node;

	vector_3			m_startingPos;
	float				m_startingOrbit;
	float				m_startingAzimuth;
	float				m_startingDistance;
	float				m_scrollMagnitude;
	float				m_zoomMagnitude;

	// For Dragging Purposes
	int					m_x_scroll_pos, m_y_scroll_pos;
	int					m_x_rotate_pos, m_y_rotate_pos;

	// For Game Camera Purposes	
	float m_minDistance;
	float m_maxDistance;	
	float m_minAzimuth;
	float m_maxAzimuth;
	float m_cameraHeightOffset;
};


#define gEditorCamera EditorCamera::GetSingleton()


#endif