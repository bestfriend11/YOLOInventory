/**************************************************************************

Filename		: UICharacterDisplay.h

Class			: UICharacterDisplay

Description		: Handles the in-game 3D character model display for the 
				  paperdoll.

Last Modified	: 1/19/00

Author			: Chad Queen

**************************************************************************/


#pragma once
#ifndef _UICHARACTERDISPLAY_H_
#define _UICHARACTERDISPLAY_H_


// Include Files
#include "nema_types.h"
#include "CameraPosition.h"

struct DollPos
{
	float x;
	float y;
	float dist;

	float xDockBarOffset;
	float yDockBarOffset;
	
	float resWidth;
	float resHeight;

	float storeOffset;
};

typedef std::vector< DollPos > DollPosColl;

// Forward Declarations
class GoHandle;


class UICharacterDisplay
{
public:

	// Constructor
	UICharacterDisplay( Goid id );

	// Destructor
	~UICharacterDisplay();

	// Render the aspects
	void Draw();

	// Update the aspect data
	void Update( double delta_time );
	void UpdateLocation( int x, int y );
	void UpdateLocation( double x, double y );

	void SetRotationX( float x ) { m_Camera.orbitAngle += x; }

	void OnScreenSize();

private:

	// Setup a temporary viewport
	void SetTemporaryView();

	void RestoreView();

	Goid						m_Object;
	double						m_x;
	double						m_y;

	siege::SiegeNodeLight		m_SunLight;
	CameraEulerPosition			m_Camera;
	SiegePos					m_OriginalTarget;	
	SiegePos					m_OriginalPosition;
	DollPosColl					m_dollPositions;

	float						m_characterX;
	float						m_characterY;
	float						m_xDockBarOffset;
	float						m_yDockBarOffset;
	float						m_characterDist;
	float						m_storeOffset;
};


#endif
