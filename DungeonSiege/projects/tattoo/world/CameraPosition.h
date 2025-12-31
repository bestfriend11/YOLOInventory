#pragma once
#ifndef _CAMERA_POSITION_
#define _CAMERA_POSITION_

/**********************************************************************************
**
**							CameraPosition
**
**		A collection of structures that define different ways that a 
**		camera position can be stored and/or submitted to the CameraAgent.
**
**		Author:		James Loe
**		Date:		04/15/00
**
**********************************************************************************/

#include "FuBiDefs.h"
#include "Matrix_3x3.h"
#include "Quat.h"
#include "Siege_Pos.h"


// $ function implementations are in CameraAgent.cpp


// Basic position.  Two points define the target and camera positions.
struct CameraPosition
{
	FUBI_POD_CLASS( CameraPosition );

	SiegePos	targetPos;
	SiegePos	cameraPos;

	bool Xfer( FuBi::PersistContext& persist );
};

// Polar based position.  Target position is given, camera position is
// determined from azimuth, orbit, and distance information.
struct CameraEulerPosition
{
	SiegePos	targetPos;
	float		orbitAngle;
	float		azimuthAngle;
	float		distance;

	void MakeCameraPosition( CameraPosition& out );
};

// Matrix based position.  Camera position is given, target position is
// determined from the orientation of the camera given in matrix form.
struct CameraMatrixPosition
{
	SiegePos	cameraPos;
	matrix_3x3	cameraOrient;
};

// Quaternion based position.  Camera position is given, target position
// is determined from the quaternion orientation
struct CameraQuatPosition
{
	SiegePos	cameraPos;
	Quat		cameraOrient;
};

// Vector based position.  Camera position is given, target position is
// determined from the directional vector supplied.
struct CameraVectorToTargetPosition
{
	SiegePos	cameraPos;
	vector_3	vectToTarget;
};

// Vector based position.  Target position is given, camera position is
// determined from the directional vector supplied.
struct CameraVectorToCameraPosition
{
	SiegePos	targetPos;
	vector_3	vectToCamera;
};


FUBI_EXPORT CameraQuatPosition& MakeCameraPosition( const SiegePos& siegePos, const Quat& quat );
FUBI_EXPORT CameraPosition& MakeCameraPosition( const SiegePos& siegePos, const Quat& quat, float distance );


#endif
