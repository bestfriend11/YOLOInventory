/**************************************************************************

Filename		: UICharacterDisplay.cpp

Class			: UICharacterDisplay

Description		: Handles the in-game 3D character model display for the 
				  paperdoll.

Last Modified	: 1/19/00

Author			: Chad Queen

**************************************************************************/



// Include Files
#include "precomp_game.h"

#include "AppModule.h"
#include "GoBody.h"
#include "CameraAgent.h"
#include "Go.h"
#include "GoCore.h"
#include "GoDb.h"
#include "Nema_AspectMgr.h"
#include "Nema_KeyMgr.h"
#include "Siege_Camera.h"
#include "UI.h"
#include "UICharacterDisplay.h"
#include "GoAspect.h"
#include "WorldFx.h"
#include "Flamethrower_effect_base.h"
#include "UIStoreManager.h"
#include "UIMenuManager.h"
#include "UIInventoryManager.h"


/**************************************************************************

Function		: UICharacterDisplay()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UICharacterDisplay::UICharacterDisplay( Goid id )
	: m_x( 0.0 )
	, m_y( 0.0 )
	, m_characterX( -0.469f )
	, m_characterY( -0.7f )
	, m_characterDist( 6.3f )
	, m_storeOffset( 0.0f )
{
	// Initialize the camera
	m_Camera.targetPos.pos	= vector_3( 0.0f, 0.0f, 0.0f );
	m_Camera.azimuthAngle	= 0.0f;
	m_Camera.distance		= 5.5f;	
	m_Camera.orbitAngle		= 0.12f;

	// Get our GoHandle
	m_Object = id;

	FastFuelHandle hResolutions( "ui:config:paperdoll_positions:paperdoll_positions" );
	if ( hResolutions.IsValid() )
	{
		FastFuelHandleColl positions;
		FastFuelHandleColl::iterator i;
		hResolutions.ListChildrenTyped( positions, "res_pos" );
		for ( i = positions.begin(); i != positions.end(); ++i )
		{
			DollPos dollPos;

			(*i).Get( "width", dollPos.resWidth );
			(*i).Get( "height", dollPos.resHeight );
			(*i).Get( "x", dollPos.x );
			(*i).Get( "y", dollPos.y );
			(*i).Get( "x_dockbar_offset", dollPos.xDockBarOffset );
			(*i).Get( "y_dockbar_offset", dollPos.yDockBarOffset );
			(*i).Get( "distance", dollPos.dist );
			(*i).Get( "store_offset", dollPos.storeOffset );

			m_dollPositions.push_back( dollPos );
		}
	}

	OnScreenSize();
}


void UICharacterDisplay::OnScreenSize()
{
	DollPosColl::iterator i;
	for ( i = m_dollPositions.begin(); i != m_dollPositions.end(); ++i )
	{
		if ( (*i).resWidth == gAppModule.GetGameWidth() &&
			 (*i).resHeight == gAppModule.GetGameHeight() )
		{
			m_characterX = (*i).x;
			m_characterY = (*i).y;
			m_xDockBarOffset = (*i).xDockBarOffset;
			m_yDockBarOffset = (*i).yDockBarOffset;

			m_characterDist = (*i).dist;
			m_storeOffset = (*i).storeOffset;
		}
	}
}


/**************************************************************************

Function		: ~UICharacterDisplay()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UICharacterDisplay::~UICharacterDisplay()
{
}



/**************************************************************************

Function		: Draw()

Purpose			: Draw the aspects

Returns			: void

**************************************************************************/
void UICharacterDisplay::Draw()
{
	GoHandle hObject( m_Object );
	if( !hObject.IsValid() )
	{
		return;
	}	

	if ( gUIStoreManager.GetStoreActive() || gUIInventoryManager.GetStashActive() )
	{
		m_characterX -= m_storeOffset;
	}
	if ( gUIMenuManager.GetStatusDockTop() )
	{
		m_characterY += m_yDockBarOffset;
	}

	SetTemporaryView();

	Rapi& renderer			= gSiegeEngine.Renderer();

	// Get placement info
	const GoPlacement *pPlacement		= hObject->GetPlacement();

	const siege::SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( pPlacement->GetPosition().node );
	const siege::SiegeNode& node		= handle.RequestObject( gSiegeEngine.NodeCache() );

	// Build up the proper view matrix
	vector_3 worldObjectPos	= pPlacement->GetWorldPosition();
	vector_3 direction		= Normalize( node.GetCurrentOrientation() * pPlacement->GetOrientation().BuildMatrix().GetColumn2_T() ) * m_characterDist;
	matrix_3x3 orientation	= MatrixFromDirection( direction ).Orthonormal_T();

	// Rotate azimuth
	direction				= AxisRotationColumns( orientation.GetColumn0_T(), m_Camera.azimuthAngle ) * direction;

	// Rotate orbit
	direction				= YRotationColumns( m_Camera.orbitAngle ) * direction;

	// Build proper orientation matrix with new direction
	orientation				= MatrixFromDirection( direction ).Orthonormal_T();

	// Now we have a camera setup that looks, in world space, directly at our character.  However, it
	// will draw at the center of the screen unless we translate it, so that is what we need to do.

	// First we need to calculate our distance to the screen
	float half_height		= (float)gSiegeEngine.GetCamera().GetViewportHeight() * 0.5f;
	float half_width		= (float)gSiegeEngine.GetCamera().GetViewportWidth() * 0.5f;

	float depth				= half_height / tanf( (gSiegeEngine.GetCamera().GetFieldOfView() * 0.5f) );

	// Now we need to figure out our offsets to the desired screen location
	float scr_ratio			= (float)gSiegeEngine.GetCamera().GetViewportHeight() / (float)gSiegeEngine.GetCamera().GetViewportWidth();
	float ytrack			= (m_characterY * half_height) * scr_ratio;
	float xtrack			= (m_characterX * half_width) * scr_ratio;

	// With this information, we can calculate our new angle of incidence with the camera
	float angleToYTrack		= atanf( ytrack / depth );
	float angleToXTrack		= atanf( xtrack / depth );

	// Using our new angle, we can plug it in given the distance from camera to target
	// to determine how many meters we need to move the camera
	float cam_off_len		= direction.Length();
	float yMetersToMove		= cam_off_len * SINF( -angleToYTrack );
	float xMetersToMove		= cam_off_len * SINF( -angleToXTrack );

	// Alright!  Now, using our current camera basis vectors, we multiply our distance
	// to move in and voila!
	vector_3 offsetVect		= orientation.GetColumn1_T() * yMetersToMove;
	offsetVect				+= orientation.GetColumn0_T() * xMetersToMove;

	DEBUG_FLOAT_VALIDATE( offsetVect.x );
	DEBUG_FLOAT_VALIDATE( offsetVect.y );
	DEBUG_FLOAT_VALIDATE( offsetVect.z );

	// Setup the camera in world space
	gSiegeEngine.GetCamera().SetCameraAndTargetPosition( worldObjectPos + direction + offsetVect, worldObjectPos + offsetVect );
	gSiegeEngine.GetCamera().UpdateCamera();

	// Now with our camera setup to our liking, we need to setup our world matrix for this object.
	renderer.SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );

	// With the proper matrix setup for the node of this object, we just need to make the
	// object specific modifications.
	renderer.TranslateWorldMatrix( pPlacement->GetPosition().pos );
	renderer.RotateWorldMatrix( pPlacement->GetOrientation().BuildMatrix() );
	renderer.ScaleWorldMatrix( vector_3( hObject->GetAspect()->GetRenderScale() ) );

	// Setup render settings
	bool notexflag = false;
	renderer.SetTextureStageState(	0,
									notexflag ? D3DTOP_DISABLE : D3DTOP_MODULATE,
									notexflag ? D3DTOP_DISABLE : D3DTOP_SELECTARG1,
									D3DTADDRESS_CLAMP,
									D3DTADDRESS_CLAMP,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );

	// Begin the Aspect ( Character Element ) Rendering
	hObject->GetAspect()->GetAspectHandle()->Deform();
	bool bIgnoring = gDefaultRapi.IsIgnoringVertexColor();
	gDefaultRapi.IgnoreVertexColor( true );
	hObject->GetAspect()->GetAspectHandle()->Render( &renderer );
	gDefaultRapi.IgnoreVertexColor( bIgnoring );

	matrix_3x3 cam_orient	= gWorldFx.GetCameraOrientation();
	gWorldFx.SetCameraOrientation( gSiegeEngine.GetCamera().GetMatrixOrientation() );
	gWorldFx.SaveRenderingState();

	// Go get the scripts attached to this go
	GoDb::EffectDb::iterator i, begin, end;
	gGoDb.GetEffectScriptIters( GoHandle( m_Object ), begin, end );

	for( i = begin; i != end; ++i )
	{
		if( gWorldFx.IsScriptOwnerVisible( (*i).second.m_ScriptId ) )
		{
			SEVector effectColl;

			// Get the effects associated with this script
			gWorldFx.GetEffectsForScript( (*i).second.m_ScriptId, effectColl );

			// Go through each effect
			for( SEVector::iterator e = effectColl.begin(); e != effectColl.end(); ++e )
			{
				// Draw effect
				(*e)->Draw();
			}
		}
	}

	for( GopColl::const_iterator c = hObject->GetChildBegin(); c != hObject->GetChildEnd(); ++c )
	{
		gGoDb.GetEffectScriptIters( *c, begin, end );

		for( i = begin; i != end; ++i )
		{
			if( gWorldFx.IsScriptOwnerVisible( (*i).second.m_ScriptId ) )
			{
				SEVector effectColl;

				// Get the effects associated with this script
				gWorldFx.GetEffectsForScript( (*i).second.m_ScriptId, effectColl );

				// Go through each effect
				for( SEVector::iterator e = effectColl.begin(); e != effectColl.end(); ++e )
				{
					// Draw effect
					(*e)->Draw();
				}
			}
		}
	}

	gWorldFx.SetCameraOrientation( cam_orient );
	gWorldFx.RestoreRenderingState();
	
	RestoreView();

	if ( gUIStoreManager.GetStoreActive() || gUIInventoryManager.GetStashActive() )
	{
		m_characterX += m_storeOffset;
	}
	if ( gUIMenuManager.GetStatusDockTop() )
	{
		m_characterY -= m_yDockBarOffset;
	}
}


// Setup a temporary viewport
void UICharacterDisplay::SetTemporaryView()
{
	m_OriginalTarget	= gSiegeEngine.GetCamera().GetTargetSiegePos();
	m_OriginalPosition	= gSiegeEngine.GetCamera().GetCameraSiegePos();
}


void UICharacterDisplay::RestoreView()
{
	gSiegeEngine.GetCamera().SetCameraAndTargetPosition( m_OriginalPosition, m_OriginalTarget );
	gSiegeEngine.GetCamera().UpdateCamera();
}


/**************************************************************************

Function		: Update()

Purpose			: Updates the current scene

Returns			: void

**************************************************************************/
void UICharacterDisplay::Update( double delta_time )
{
	if ( delta_time > 0.0f ) 
	{
		vector_3 lightv = -m_SunLight.m_Direction;
		lightv = Normalize(lightv);
	}	
}



/**************************************************************************

Function		: UpdateLocation()

Purpose			: Update location of aspect on the screen

Returns			: void

**************************************************************************/
void UICharacterDisplay::UpdateLocation( int x, int y )
{	
	double height	= gSiegeEngine.GetCamera().GetViewportHeight();
	double width	= gSiegeEngine.GetCamera().GetViewportWidth();
	
	x += FTOL( -width/2.0 );
	y += FTOL( -height/2.0 );

	double new_x = (double)(x)/(double(width)/2.0 );
	double new_y = (double)(y)/(double(height)/2.0 );
	
	UpdateLocation( -new_x, -new_y );
}
void UICharacterDisplay::UpdateLocation( double x, double y )
{	
	double height	= gSiegeEngine.GetCamera().GetViewportHeight();
	double width	= gSiegeEngine.GetCamera().GetViewportWidth();
		
	float half_height = (float)(height/2.0);
	m_x = x*half_height;
	m_y = y*half_height*height/(real)width;
	
	m_y = m_y/((height/width)*100.0f);
	m_x = m_x/((height/width)*100.0f);
}