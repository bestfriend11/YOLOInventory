//**********************************************************************
//
// Mouse Shadow
//
//**********************************************************************

#include "precomp_siege.h"
#include "siege_mouse_shadow.h"
#include "siege_engine.h"
#include "siege_viewfrustum.h"
#include "siege_camera.h"
#include "gpmath.h"

using namespace siege;


SiegeMouseShadow::SiegeMouseShadow(void) 
{
	InitVariables();
	Init();
}


SiegeMouseShadow::~SiegeMouseShadow(void) 
{
	Shutdown();
}


void SiegeMouseShadow::Init()
{
	Shutdown();
	InitVariables();
	m_pSelectFrustum = new SiegeViewFrustum(1,1);
}


void SiegeMouseShadow::Shutdown()
{
	Delete( m_pSelectFrustum );
	InitVariables();
}


void SiegeMouseShadow::InitVariables()
{
	m_CameraSpacePosition = vector_3( 0.0f, 0.0f, 0.0f );
	m_bAccurate = false;
	m_Hit = 0;
	m_MinDist = FLOAT_MAX;
	m_FloorMinDist = FLOAT_MAX;
	m_SelectBoxBegin = vector_3( 0.0f, 0.0f, 0.0f );

	m_bMultiSelectEnabled	= true;
	m_bSelectionBox			= false;
	m_bSelectFrustum		= false;
	m_pSelectFrustum		= NULL;
}

void SiegeMouseShadow::Update( const float norm_cursor_x, const float norm_cursor_y )
{
	// Clear any hit location
	m_bAccurate				= false;
	m_MinDist				= FLOAT_MAX;
	m_FloorMinDist			= FLOAT_MAX;
	m_Hit					= 0;

	m_HitColl.clear();
	m_GlobalHitColl.clear();

	// Calculate the current camera space position of the mouse location
	float half_height		= (float)gSiegeEngine.GetCamera().GetViewportHeight() * 0.5f;
	float half_width		= (float)gSiegeEngine.GetCamera().GetViewportWidth() * 0.5f;
	float cursor_z			= half_height / tanf( (gSiegeEngine.GetCamera().GetFieldOfView() * 0.5f) );

	// Build a vector in screen space for distance checks
	// NOTE:  For some reason we are passing the norm_cursor_x in as a negative number.  This requires an extra subtraction here
	// to get valid screen space.
	m_ScreenPos				= vector_3( (float)gSiegeEngine.GetCamera().GetViewportWidth() - ((norm_cursor_x * half_width) + half_width),
										(float)gSiegeEngine.GetCamera().GetViewportHeight() - ((norm_cursor_y * half_height) + half_height),
										0.0f );
	
	m_CameraSpacePosition.x	= (norm_cursor_x * half_height);
	m_CameraSpacePosition.y = (norm_cursor_y * half_height) * ( (float)gSiegeEngine.GetCamera().GetViewportHeight() / (float)gSiegeEngine.GetCamera().GetViewportWidth() ),
	m_CameraSpacePosition.z = cursor_z;

	// Are we dragging the mouse?
	if( m_bSelectionBox )
	{
		if(	FABSF( m_SelectBoxScreenBegin.x - m_ScreenPos.x ) >= gSiegeEngine.GetDragSelectThreshold() &&
			FABSF( m_SelectBoxScreenBegin.y - m_ScreenPos.y ) >= gSiegeEngine.GetDragSelectThreshold() )
		{
			// Allow multi select
			m_bMultiSelectEnabled			= true;

			// Make sure that we notify the selector that we need a frustum
			m_bSelectFrustum				= true;

			// Draw a box to indicate to the user where their selection lasso is
			gSiegeEngine.GetScreenSpaceLines().DrawLine( m_SelectBoxScreenBegin.x, m_SelectBoxScreenBegin.y,
														 m_ScreenPos.x, m_SelectBoxScreenBegin.y, 0xFF00FF00 );
			gSiegeEngine.GetScreenSpaceLines().DrawLine( m_ScreenPos.x, m_SelectBoxScreenBegin.y,
														 m_ScreenPos.x, m_ScreenPos.y, 0xFF00FF00 );
			gSiegeEngine.GetScreenSpaceLines().DrawLine( m_ScreenPos.x, m_ScreenPos.y,
														 m_SelectBoxScreenBegin.x, m_ScreenPos.y, 0xFF00FF00 );
			gSiegeEngine.GetScreenSpaceLines().DrawLine( m_SelectBoxScreenBegin.x, m_ScreenPos.y,
														 m_SelectBoxScreenBegin.x, m_SelectBoxScreenBegin.y, 0xFF00FF00 );

			// Build a frustum to select objects with
			m_pSelectFrustum->SetDimensionsWithNearBox( m_SelectBoxBegin / cursor_z, m_CameraSpacePosition / cursor_z,
														m_pSelectFrustum->GetNearClipDistance(),
														m_pSelectFrustum->GetFarClipDistance(),
														gSiegeEngine.GetCamera().GetCameraPosition(), gSiegeEngine.GetCamera().GetMatrixOrientation() );
		}
		else
		{
			m_bSelectFrustum			= false;
		}
	}
	else
	{
		// Disallow multi select
		m_bMultiSelectEnabled			= false;

		// Make sure that we notify the selector that we don't need a frustum
		m_bSelectFrustum				= false;

		// Save off the camera space position in case the user starts dragging
		m_SelectBoxBegin				= m_CameraSpacePosition;
		m_SelectBoxScreenBegin			= m_ScreenPos;
	} 
}

// Update the position of the begin spot
void SiegeMouseShadow::SetBeginPosition( const float norm_x, const float norm_y )
{
	// Calculate the current camera space position of the mouse location
	float half_height		= (float)gSiegeEngine.GetCamera().GetViewportHeight() * 0.5f;
	float half_width		= (float)gSiegeEngine.GetCamera().GetViewportWidth() * 0.5f;
	float cursor_z			= half_height / tanf( (gSiegeEngine.GetCamera().GetFieldOfView() * 0.5f) );

	// Build a vector in screen space for distance checks
	// NOTE:  For some reason we are passing the norm_cursor_x in as a negative number.  This requires an extra subtraction here
	// to get valid screen space.
	m_SelectBoxScreenBegin	= vector_3( (float)gSiegeEngine.GetCamera().GetViewportWidth() - ((norm_x * half_width) + half_width),
										(float)gSiegeEngine.GetCamera().GetViewportHeight() - ((norm_y * half_height) + half_height),
										0.0f );
	
	m_SelectBoxBegin.x		= (norm_x * half_height);
	m_SelectBoxBegin.y		= (norm_y * half_height) * ( (float)gSiegeEngine.GetCamera().GetViewportHeight() / (float)gSiegeEngine.GetCamera().GetViewportWidth() ),
	m_SelectBoxBegin.z		= cursor_z;
}

// Push a general object hit
void SiegeMouseShadow::PushGeneralHit( DWORD g, const vector_3& pos, const float dist )
{
	if (!m_bMultiSelectEnabled && ((m_FloorMinDist + 0.25f) < dist))
	{
		// Here I am assuming that the ground obscures the lone dude
		return;
	}

	// Need a check to see if the floor obscures this multi-selected character
	float screen_dist	= Length( gDefaultRapi.GetScreenPosition( pos ) - m_ScreenPos );
	if( !m_bAccurate && (screen_dist < m_MinDist) )
	{
		if( !m_bMultiSelectEnabled )
		{
			// Kind of a waste to keep clearing it, but we need to have only one entry...
			m_HitColl.clear();
		}

		m_MinDist		= screen_dist;
		m_HitLocation = pos;
		m_Hit			= g;

		m_HitColl.push_back( g );
	}
	else if( m_bMultiSelectEnabled )
	{
		m_HitColl.push_back( g );
	}

	// Put this object into the global hit list
	m_GlobalHitColl.push_back( g );
}

// Push an accurate object hit
void SiegeMouseShadow::PushAccurateHit( DWORD g, const vector_3& pos, const float dist )
{
	if( !m_bMultiSelectEnabled && ((m_FloorMinDist + 0.25f) < dist) )
	{
		// Here I am assuming that the ground obscures the lone dude
		return;
	}
	if( m_bMultiSelectEnabled )
	{
		// Don't care about accurate selections if we're in multi select
		return;
	}

	// Need a check to see if the floor obscures this multi-selected character
	float screen_dist	= Length( gDefaultRapi.GetScreenPosition( pos ) - m_ScreenPos );
	if( !m_bAccurate || (screen_dist < m_MinDist) )
	{
		// Kind of a waste to keep clearing it, but we need to have only one entry...
		m_HitColl.clear();

		m_bAccurate		= true;
		m_MinDist		= screen_dist;
		m_HitLocation	= pos;
		m_Hit			= g;

		m_HitColl.push_back( g );
	}
}
