#pragma once
#ifndef _SIEGE_CAMERA_
#define _SIEGE_CAMERA_

/***********************************************************************
**
**							SiegeCamera
**
**	Provides the SiegeEngine and its clients with basic camera
**	functionality.
**
**  Author:			James Loe
**  Date:			04/14/00
**
***********************************************************************/

#include "siege_defs.h"
#include "siege_pos.h"
#include "fubidefs.h"
#include "vector_3.h"
#include "matrix_3x3.h"
#include "quat.h"
#include "siege_viewfrustum.h"

namespace siege
{
	// Struct that defines each item in our historic buffer
	struct CamHistoryItem
	{
		SiegePos		m_cameraPos;
		SiegePos		m_targetPos;
		double			m_timeStamp;
		double			m_secondsAgo;
	};

	// Define how often we take a historic sample and how many samples to take
	const float CAM_HISTORY_INTERVAL			= 0.01667f;
	const int	CAM_HISTORY_LENGTH				= 20;

	// Typedef the historic buffer
	typedef std::list< CamHistoryItem >			CamHistory;


	class SiegeCamera
	{
	public:

		// Construction
		SiegeCamera();

		// Destruction
		~SiegeCamera();

		// Persistence
		bool				Xfer( FuBi::PersistContext& persist );

		// Viewport
		void				SetViewportWidth( unsigned int width );
		void				SetViewportHeight( unsigned int height );
	FEX	unsigned int		GetViewportWidth();
	FEX	unsigned int		GetViewportHeight();

		void				SetViewportNormalizedWidth( float width )	{ m_viewWidth = width; }
		void				SetViewportNormalizedHeight( float height )	{ m_viewHeight = height; }
	FEX	float				GetViewportNormalizedWidth()				{ return m_viewWidth; }
	FEX	float				GetViewportNormalizedHeight()				{ return m_viewHeight; }

		// Field of view
		void				SetFieldOfView( float fovInRadians )		{ m_fieldOfView = fovInRadians; }
	FEX	float				GetFieldOfView()							{ return m_fieldOfView; }			

		// Azimuth limits
		void				SetMaxAzimuth( float azimuth )				{ m_maxAzimuth = azimuth; }
		void				SetMinAzimuth( float azimuth )				{ m_minAzimuth = azimuth; }
	FEX	float				GetMaxAzimuth()								{ return m_maxAzimuth; }
	FEX	float				GetMinAzimuth()								{ return m_minAzimuth; }

		// Distance limits
		void				SetMaxDistance( float distance )			{ m_maxDistance = distance; }
		void				SetMinDistance( float distance )			{ m_minDistance = distance; }
	FEX	float				GetMaxDistance()							{ return m_maxDistance; }
	FEX	float				GetMinDistance()							{ return m_minDistance; }

		// Camera position
		void				SetCameraPosition( const SiegePos& camera );
		void				SetCameraPosition( const vector_3& camera );
	FEX	const vector_3&		GetCameraPosition() const					{ return m_CameraWorldPosition; }
	FEX	const SiegePos&		GetCameraSiegePos() const					{ return m_CameraPosition; }

		// Target position
		void				SetTargetPosition( const SiegePos& target );
		void				SetTargetPosition( const vector_3& target );
	FEX	const vector_3&		GetTargetPosition() const					{ return m_TargetWorldPosition; }
	FEX	const SiegePos&		GetTargetSiegePos() const					{ return m_TargetPosition; }

		// Combined position
		void				SetCameraAndTargetPosition( const SiegePos& camera, const SiegePos& target );
		void				SetCameraAndTargetPosition( const vector_3& camera, const vector_3& target );

		// Up vector
		void				SetUpVector( const vector_3& upvect )		{ m_UpVector = upvect; }
	FEX	const vector_3&		GetUpVector() const							{ return m_UpVector; }

		// Frustum
		SiegeViewFrustum&	GetFrustum()								{ return m_Frustum; }

		// Orientation
		void				BuildOrientations();
	FEX	const vector_3&		GetVectorOrientation()						{ return m_VectorOrientation; }
	FEX	const matrix_3x3&	GetMatrixOrientation()						{ return m_MatrixOrientation; }
	FEX	const Quat&			GetQuatOrientation()						{ return m_QuatOrientation; }

		// Update the camera with current settings.
		// Updates the viewport, frustum, view matrix, and perspective matrix.
		void				UpdateCamera();

		// Store the current camera info in a historic buffer
		void				StoreHistory();

		// Clear the current history of the camera
		void				ClearHistory()								{ m_HistoricBuffer.clear(); }
		void				Clear()										{ ClearHistory();
																		  SetCameraAndTargetPosition( vector_3::ZERO, vector_3::ZERO );
																		}

		// Get the position of the camera as it looked secondsAgo
		CamHistory&			GetHistoricBuffer()							{ return m_HistoricBuffer; }
		void				GetHistoricPositions( vector_3& camHistory, vector_3& tarHistory );

		// Functions to weight the history.  Fills either the camera history or the target history
		// with the current camera or target position.
		void				FillCameraHistoryWithCurrent();
		void				FillTargetHistoryWithCurrent();

	private:

		// Frustum
		SiegeViewFrustum	m_Frustum;

		// Viewport specific information
		float				m_viewWidth;
		float				m_viewHeight;
		float				m_fieldOfView;

		// Positional information
		SiegePos			m_CameraPosition;
		SiegePos			m_TargetPosition;

		vector_3			m_CameraWorldPosition;
		vector_3			m_TargetWorldPosition;
		vector_3			m_UpVector;

		// Orientation information
		vector_3			m_VectorOrientation;
		matrix_3x3			m_MatrixOrientation;
		Quat				m_QuatOrientation;

		// Azimuth and distance limits
		float				m_maxAzimuth;
		float				m_minAzimuth;
		float				m_maxDistance;
		float				m_minDistance;

		// Historical buffer of positions
		CamHistory			m_HistoricBuffer;
	};
}

#endif
