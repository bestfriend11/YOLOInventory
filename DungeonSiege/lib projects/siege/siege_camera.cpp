/***********************************************************************
**
**							SiegeCamera
**
**	See .h file for details
**
***********************************************************************/

#include "precomp_siege.h"

#include "space_3.h"
#include "spline.h"
#include "siege_camera.h"
#include "siege_persist.h"
#include "fubitraitsimpl.h"
#include "fubipersist.h"


using namespace siege;

FUBI_REPLACE_NAME( "SiegeSiegeCamera", SiegeCamera );


// Constructor
SiegeCamera::SiegeCamera()
	: m_viewWidth( 1.0f )
	, m_viewHeight( 1.0f )
	, m_fieldOfView( RadiansFrom( 60 ) )
	, m_maxAzimuth( 85.0f )
	, m_minAzimuth( 40.0f )
	, m_maxDistance( 18.0f )
	, m_minDistance( 4.0f )
	, m_UpVector( 0.0f, 1.0f, 0.0f )
	, m_Frustum( 1, 1 )
{
}

// Destructor
SiegeCamera::~SiegeCamera()
{
}

// Persistence
bool SiegeCamera::Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_viewWidth",         m_viewWidth         );
	persist.Xfer( "m_viewHeight",        m_viewHeight        );
	persist.Xfer( "m_fieldOfView",       m_fieldOfView       );
	persist.Xfer( "m_CameraPosition",    m_CameraPosition    );
	persist.Xfer( "m_TargetPosition",    m_TargetPosition    );
	persist.Xfer( "m_UpVector",          m_UpVector          );
	persist.Xfer( "m_VectorOrientation", m_VectorOrientation );
	persist.Xfer( "m_MatrixOrientation", m_MatrixOrientation );
	persist.Xfer( "m_QuatOrientation",   m_QuatOrientation   );
	persist.Xfer( "m_maxAzimuth",        m_maxAzimuth        );
	persist.Xfer( "m_minAzimuth",        m_minAzimuth        );
	persist.Xfer( "m_maxDistance",       m_maxDistance       );
	persist.Xfer( "m_minDistance",       m_minDistance       );

	if ( persist.IsRestoring() )
	{
		ClearHistory();
	}

	return ( true );
}

// Camera position
void SiegeCamera::SetCameraPosition( const SiegePos& camera )
{
	// Set the new position
	m_CameraPosition		= camera;

	// Build orientations
	BuildOrientations();
}

// Camera position
void SiegeCamera::SetCameraPosition( const vector_3& camera )
{
	// Set the new position
	m_CameraPosition.pos	= camera;
	m_CameraPosition.node	= UNDEFINED_GUID;

	// Build orientations
	BuildOrientations();
}

// Target position
void SiegeCamera::SetTargetPosition( const SiegePos& target )
{
	// Set the new position
	m_TargetPosition		= target;

	// Build orientations
	BuildOrientations();
}

// Target position
void SiegeCamera::SetTargetPosition( const vector_3& target )
{
	// Set the new position
	m_TargetPosition.pos	= target;
	m_TargetPosition.node	= UNDEFINED_GUID;

	// Build orientations
	BuildOrientations();
}

// Combined position
void SiegeCamera::SetCameraAndTargetPosition( const SiegePos& camera, const SiegePos& target )
{
	// Set the new positions
	m_CameraPosition		= camera;
	m_TargetPosition		= target;

	// Build orientations
	BuildOrientations();
}

// Combined position
void SiegeCamera::SetCameraAndTargetPosition( const vector_3& camera, const vector_3& target )
{
	// Set the new positions
	m_CameraPosition.pos	= camera;
	m_CameraPosition.node	= UNDEFINED_GUID;

	m_TargetPosition.pos	= target;
	m_TargetPosition.node	= UNDEFINED_GUID;

	// Build orientations
	BuildOrientations();
}



// Build the orientations
void SiegeCamera::BuildOrientations()
{
	// Update cached world positions
	if( m_CameraPosition.node.IsValidSlow() )
	{
		m_CameraWorldPosition = m_CameraPosition.WorldPos();
	}
	else
	{
		m_CameraWorldPosition = m_CameraPosition.pos;
	}

	if( m_TargetPosition.node.IsValidSlow() )
	{
		m_TargetWorldPosition = m_TargetPosition.WorldPos();
	}
	else
	{
		m_TargetWorldPosition = m_TargetPosition.pos;
	}

	// Build vector orientation
	m_VectorOrientation		= GetTargetPosition() - GetCameraPosition();

	// Build matrix orientation
	if( m_VectorOrientation.IsZero() )
	{
		m_MatrixOrientation	= matrix_3x3::IDENTITY;
		m_VectorOrientation	= vector_3::NORTH;
	}
	else
	{
		m_MatrixOrientation	= MatrixFromDirection( m_VectorOrientation );
	}

	// Build quat orientation
	m_QuatOrientation		= Quat( m_MatrixOrientation );
}

// Update the camera with current settings.
// Updates the frustum, view matrix, and perspective matrix.
void SiegeCamera::UpdateCamera()
{
	// Update cached world positions
	if( m_CameraPosition.node.IsValidSlow() )
	{
		m_CameraWorldPosition = m_CameraPosition.WorldPos();
	}
	else
	{
		m_CameraWorldPosition = m_CameraPosition.pos;
	}

	if( m_TargetPosition.node.IsValidSlow() )
	{
		m_TargetWorldPosition = m_TargetPosition.WorldPos();
	}
	else
	{
		m_TargetWorldPosition = m_TargetPosition.pos;
	}

	// Get the clip planes from Rapi
	const float nearClip	= gSiegeEngine.Renderer().GetClipNearDist();
	const float farClip		= gSiegeEngine.Renderer().GetClipFarDistAdjustedForFog();

	// Build up the current frustum
	m_Frustum.SetDimensionsWithFOV( GetViewportWidth(), GetViewportHeight(), m_fieldOfView, nearClip, farClip, GetCameraPosition(), GetMatrixOrientation() );

	// Set the viewport
	gSiegeEngine.Renderer().SetViewport( m_viewWidth, m_viewHeight );

	// Set the perspective matrix
	gSiegeEngine.Renderer().SetPerspectiveMatrix( m_fieldOfView, nearClip, farClip );

	// Set the view matrix
	gSiegeEngine.Renderer().SetViewMatrix( GetCameraPosition(), GetTargetPosition(), m_UpVector );
}

// Store the current camera info in a historic buffer
void SiegeCamera::StoreHistory()
{
	// Get the current time
	double time						= ::GetGlobalSeconds();

	// See if enough time has elapsed to take a sample
	if( PreciseSubtract(time, m_HistoricBuffer.front().m_timeStamp) >= CAM_HISTORY_INTERVAL )
	{
		// Make a new historic structure to use
		CamHistoryItem newCamHistory;
		newCamHistory.m_cameraPos		= m_CameraPosition;
		newCamHistory.m_targetPos		= m_TargetPosition;
		newCamHistory.m_timeStamp		= time;
		newCamHistory.m_secondsAgo		= 0.0;

		// Put our new historic position into the buffer
		m_HistoricBuffer.push_front( newCamHistory );

		// Erase the last sample if necessary
		if( m_HistoricBuffer.size() > CAM_HISTORY_LENGTH )
		{
			m_HistoricBuffer.pop_back();
		}

		// Go through our buffer and update time
		for( CamHistory::iterator i = m_HistoricBuffer.begin(); i != m_HistoricBuffer.end(); ++i )
		{
			// Get the time that has passed since this stamp was taken
			(*i).m_secondsAgo			= PreciseSubtract( time, (*i).m_timeStamp );
		}
	}
}

// Calculate the historic positions
void SiegeCamera::GetHistoricPositions( vector_3& camHistory, vector_3& tarHistory )
{
	// If the historic buffer is empty, just use the camera position
	if( m_HistoricBuffer.empty() )
	{
		camHistory	= m_CameraWorldPosition;
		tarHistory	= m_TargetWorldPosition;
	}
	else
	{
		std::vector< vector_3 > cameraPosColl;
		std::vector< vector_3 > targetPosColl;
		cameraPosColl.reserve( m_HistoricBuffer.size() );
		targetPosColl.reserve( m_HistoricBuffer.size() );
		float averageTimeDist	= 0.0f;
		double previousTime		= m_HistoricBuffer.front().m_timeStamp;

		for( CamHistory::const_iterator h = m_HistoricBuffer.begin(); h != m_HistoricBuffer.end(); ++h )
		{
			if( (*h).m_cameraPos.node.IsValidSlow() )
			{
				cameraPosColl.push_back( (*h).m_cameraPos.WorldPos() );
			}
			else
			{
				cameraPosColl.push_back( (*h).m_cameraPos.pos );
			}

			if( (*h).m_targetPos.node.IsValidSlow() )
			{
				targetPosColl.push_back( (*h).m_targetPos.WorldPos() );
			}
			else
			{
				targetPosColl.push_back( (*h).m_targetPos.pos );
			}

			averageTimeDist	+= (float)(PreciseSubtract(previousTime, (*h).m_timeStamp));
			previousTime	= (*h).m_timeStamp;
		}

		// Calculate the average distance between samples
		averageTimeDist	/= m_HistoricBuffer.size();

		// Use splines to generate a good weighted average position for camera blending
		float norm = 0.1f - min_t( 1.0f, (averageTimeDist / (CAM_HISTORY_LENGTH * CAM_HISTORY_INTERVAL)) ) * 0.1f;
		camHistory = GeneralBezier( norm, cameraPosColl );
		tarHistory = GeneralBezier( norm, targetPosColl );
	}
}

// Weight the history by filling the camera block up with the current camera position
void SiegeCamera::FillCameraHistoryWithCurrent()
{
	if( m_HistoricBuffer.empty() )
	{
		// Make a new historic structure to use
		CamHistoryItem newCamHistory;
		newCamHistory.m_cameraPos		= m_CameraPosition;
		newCamHistory.m_targetPos		= m_TargetPosition;
		newCamHistory.m_timeStamp		= ::GetGlobalSeconds();
		newCamHistory.m_secondsAgo		= 0.0;

		m_HistoricBuffer.push_back( newCamHistory );
	}
	else
	{
		for( CamHistory::iterator i = m_HistoricBuffer.begin(); i != m_HistoricBuffer.end(); ++i )
		{
			(*i).m_targetPos	= m_TargetPosition;
		}
	}
}

// Weight the history by filling the target block up with the current camera position
void SiegeCamera::FillTargetHistoryWithCurrent()
{
	if( m_HistoricBuffer.empty() )
	{
		// Make a new historic structure to use
		CamHistoryItem newCamHistory;
		newCamHistory.m_cameraPos		= m_CameraPosition;
		newCamHistory.m_targetPos		= m_TargetPosition;
		newCamHistory.m_timeStamp		= ::GetGlobalSeconds();
		newCamHistory.m_secondsAgo		= 0.0;

		m_HistoricBuffer.push_back( newCamHistory );
	}
	else
	{
		for( CamHistory::iterator i = m_HistoricBuffer.begin(); i != m_HistoricBuffer.end(); ++i )
		{
			(*i).m_targetPos	= m_TargetPosition;
		}
	}
}


// Viewport
void SiegeCamera::SetViewportWidth( unsigned int width )
{ 
	m_viewWidth = ( (float)width / (float)gSiegeEngine.Renderer().GetWidth() );
}


void SiegeCamera::SetViewportHeight( unsigned int height )	
{ 
	m_viewHeight = ( (float)height / (float)gSiegeEngine.Renderer().GetHeight() );
}


unsigned int SiegeCamera::GetViewportWidth()							
{ 
	return FTOL( m_viewWidth * (float)gSiegeEngine.Renderer().GetWidth() ); 
}


unsigned int SiegeCamera::GetViewportHeight()				
{ 
	return FTOL( m_viewHeight * (float)gSiegeEngine.Renderer().GetHeight() );
}


