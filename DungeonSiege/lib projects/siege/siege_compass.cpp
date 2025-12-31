/*******************************************************************************
**
**								SiegeCompass
**
**		See .h file for details
**
*******************************************************************************/

#include "precomp_siege.h"
#include "RapiPrimitive.h"
#include "siege_camera.h"
#include "siege_engine.h"
#include "siege_hotpoint_database.h"
#include "siege_compass.h"
#include "namingkey.h"

using namespace siege;


// Construction
SiegeCompass::SiegeCompass()
	: m_Radius( 0 )
	, m_ScreenX( 0 )
	, m_ScreenY( 0 )
	, m_CardDist( 0 )
	, m_NeedleTexture( 0 )
	, m_NeedleWidth( 0 )
	, m_NeedleHeight( 0 )
	, m_DirectionTexture( 0 )
	, m_DirectionWidth( 0 )
	, m_DirectionHeight( 0 )
	, m_NorthTexture( 0 )
	, m_EastTexture( 0 )
	, m_SouthTexture( 0 )
	, m_WestTexture( 0 )
	, m_CardinalWidth( 0 )
	, m_CardinalHeight( 0 )
	, m_HotpointTexture( 0 )
	, m_HotpointWidth( 0 )
	, m_HotpointHeight( 0 )
	, m_MinimizedTexture( 0 )
	, m_MinimizedWidth( 0 )
	, m_MinimizedHeight( 0 )
	, m_bVisible( false )
	, m_bActive( true )
	, m_bMinimized( false )
	, m_Alpha( 0xFF )	
{
}

// Destruction
SiegeCompass::~SiegeCompass()
{
	// Cleanup textures
	Rapi& renderer = gDefaultRapi;

	renderer.DestroyTexture( m_NeedleTexture );
	renderer.DestroyTexture( m_DirectionTexture );
	renderer.DestroyTexture( m_NorthTexture );
	renderer.DestroyTexture( m_EastTexture );
	renderer.DestroyTexture( m_SouthTexture );
	renderer.DestroyTexture( m_WestTexture );
	renderer.DestroyTexture( m_HotpointTexture );
	renderer.DestroyTexture( m_MinimizedTexture );
}

// Update the compass
void SiegeCompass::Update( const matrix_3x3& cam_orient )
{
	if( !m_bVisible || !m_bActive )
	{
		return;
	}

	Rapi& renderer = gDefaultRapi;
	renderer.SetTextureStageState(	0,
									D3DTOP_SELECTARG1,
									D3DTOP_SELECTARG1,
									D3DTADDRESS_WRAP,
									D3DTADDRESS_WRAP,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									true );
	bool bAlphaBlend = renderer.EnableAlphaBlending( true );

	if( gSiegeHotpointDatabase.IsHotpointActive() && !gSiegeHotpointDatabase.ArrivedAtHotpoint() )
	{
		matrix_3x3 hot_orient	= GetActiveHotpointDirectionMatrix( cam_orient );

		// Draw the hotpoint indicator
		DrawHotpoint( renderer, hot_orient );
	}

	matrix_3x3 dir_orient	= GetNorthDirectionMatrix( cam_orient );

	if ( GetMinimized() )
	{
		// Draw the minimized compass
		DrawMinimized( renderer, dir_orient );
	}
	else
	{
		// Draw the needle
		DrawNeedle( renderer, dir_orient );

		// Draw the direction finder
		DrawDirectionFinder( renderer, dir_orient );

		// Draw the cardinal directions
		DrawCardinals( renderer, dir_orient );
	}

	renderer.EnableAlphaBlending( bAlphaBlend );
}

// See if the given screen point hits the compass
bool SiegeCompass::ScreenPosIntersectsCompass( const int screenx, const int screeny )
{
	// Get the half width and height
	int halfDirWidth	= m_DirectionWidth / 2;
	int halfDirHeight	= m_DirectionHeight / 2;

	if( screenx < (m_ScreenX - halfDirWidth) || screeny < (m_ScreenY - halfDirHeight) )
	{
		return false;
	}

	if( screenx > (m_ScreenX + halfDirWidth) || screeny > (m_ScreenY + halfDirHeight) )
	{
		return false;
	}

	return true;
}

// Move the compass in screen space.
void SiegeCompass::MoveCompass( const int deltax, const int deltay )
{
	gpassert( ((m_ScreenX+deltax) < (int)gSiegeEngine.GetCamera().GetViewportWidth()) && ((m_ScreenX+deltax) > 0) );
	gpassert( ((m_ScreenY+deltay) < (int)gSiegeEngine.GetCamera().GetViewportHeight()) && ((m_ScreenY+deltay) > 0) );

	m_ScreenX	= max_t( 0, min_t( (int)gSiegeEngine.GetCamera().GetViewportWidth(), m_ScreenX+deltax ) );
	m_ScreenY	= max_t( 0, min_t( (int)gSiegeEngine.GetCamera().GetViewportHeight(), m_ScreenY+deltay ) );
}

// Set the radius of the compass
void SiegeCompass::SetCompassRadius( const int radius )
{
	// Save the radius
	m_Radius	= radius;

	// Compute local screen coordinates
	m_ScreenX	= gSiegeEngine.GetCamera().GetViewportWidth() - radius;
	m_ScreenY	= radius;
}

// Needle texture
void SiegeCompass::SetNeedleTexture( const gpstring& texname )
{
	gpstring strPath;
	gNamingKey.BuildIMGLocation( texname.c_str(), strPath );

	// Set the new needle texture
	m_NeedleTexture	= gDefaultRapi.CreateTexture( strPath, false, 0, TEX_LOCKED );
	if( m_NeedleTexture )
	{
		// Set dimensions
		m_NeedleWidth	= gDefaultRapi.GetTextureWidth( m_NeedleTexture );
		m_NeedleHeight	= gDefaultRapi.GetTextureHeight( m_NeedleTexture );
	}
}

// Direction texture
void SiegeCompass::SetDirectionTexture( const gpstring& texname )
{
	gpstring strPath;
	gNamingKey.BuildIMGLocation( texname.c_str(), strPath );

	// Set the new needle texture
	m_DirectionTexture	= gDefaultRapi.CreateTexture( strPath, false, 0, TEX_LOCKED );

	// Set dimensions
	m_DirectionWidth	= gDefaultRapi.GetTextureWidth( m_DirectionTexture );
	m_DirectionHeight	= gDefaultRapi.GetTextureHeight( m_DirectionTexture );
}

// Cardinal direction textures
void SiegeCompass::SetCardinalTextures( const gpstring& northname,
									    const gpstring& eastname,
										const gpstring& southname,
										const gpstring& westname )
{
	// Set the cardinal directional textures
	gpstring strPath;

	gNamingKey.BuildIMGLocation( northname.c_str(), strPath );
	m_NorthTexture		= gDefaultRapi.CreateTexture( strPath, false, 0, TEX_LOCKED );

	gNamingKey.BuildIMGLocation( eastname.c_str(), strPath );
	m_EastTexture		= gDefaultRapi.CreateTexture( strPath, false, 0, TEX_LOCKED );

	gNamingKey.BuildIMGLocation( southname.c_str(), strPath );
	m_SouthTexture		= gDefaultRapi.CreateTexture( strPath, false, 0, TEX_LOCKED );

	gNamingKey.BuildIMGLocation( westname.c_str(), strPath );
	m_WestTexture		= gDefaultRapi.CreateTexture( strPath, false, 0, TEX_LOCKED );

	// Set dimensions
	m_CardinalWidth		= gDefaultRapi.GetTextureWidth( m_NorthTexture );
	m_CardinalHeight	= gDefaultRapi.GetTextureHeight( m_NorthTexture );
}

// Hotpoint texture
void SiegeCompass::SetHotpointTexture( const gpstring& texname )
{
	// Set the new needle texture
	gpstring strPath;
	gNamingKey.BuildIMGLocation( texname.c_str(), strPath );

	m_HotpointTexture	= gDefaultRapi.CreateTexture( strPath, false, 0, TEX_LOCKED );

	// Set dimensions
	m_HotpointWidth		= gDefaultRapi.GetTextureWidth( m_HotpointTexture );
	m_HotpointHeight	= gDefaultRapi.GetTextureHeight( m_HotpointTexture );
}

// Minimized texture
void SiegeCompass::SetMinimizedTexture( const gpstring& texname )
{
	// Set the new needle texture
	gpstring strPath;
	gNamingKey.BuildIMGLocation( texname.c_str(), strPath );

	m_MinimizedTexture	= gDefaultRapi.CreateTexture( strPath, false, 0, TEX_LOCKED );

	// Set dimensions
	m_MinimizedWidth	= gDefaultRapi.GetTextureWidth( m_MinimizedTexture );
	m_MinimizedHeight	= gDefaultRapi.GetTextureHeight( m_MinimizedTexture );
}

// Draw the needle
void SiegeCompass::DrawNeedle( Rapi& renderer, const matrix_3x3& orient )
{
	UNREFERENCED_PARAMETER( orient );

	tVertex	pVerts[4];
	memset( pVerts, 0, sizeof( tVertex ) * 4 );

	// Calculate half of our needle dimensions
	int halfNeedleWidth		= m_NeedleWidth / 2;
	int halfNeedleHeight	= m_NeedleHeight / 2;

	// Fill in our verts with the information for the needle
	pVerts[0].x		= (float)(m_ScreenX - halfNeedleWidth);
	pVerts[0].y		= (float)(m_ScreenY + halfNeedleHeight);
	pVerts[0].z		= 0.0f;
	pVerts[0].rhw	= 1.0f;
	pVerts[0].uv.u	= 0.0f;
	pVerts[0].uv.v	= 0.0f;
	pVerts[0].color	= 0xFFFFFFFF;

	pVerts[1].x		= (float)(m_ScreenX + halfNeedleWidth);
	pVerts[1].y		= (float)(m_ScreenY + halfNeedleHeight);
	pVerts[1].z		= 0.0f;
	pVerts[1].rhw	= 1.0f;
	pVerts[1].uv.u	= 1.0f;
	pVerts[1].uv.v	= 0.0f;
	pVerts[1].color	= 0xFFFFFFFF;

	pVerts[2].x		= (float)(m_ScreenX - halfNeedleWidth);
	pVerts[2].y		= (float)(m_ScreenY - halfNeedleHeight);
	pVerts[2].z		= 0.0f;
	pVerts[2].rhw	= 1.0f;
	pVerts[2].uv.u	= 0.0f;
	pVerts[2].uv.v	= 1.0f;
	pVerts[2].color	= 0xFFFFFFFF;

	pVerts[3].x		= (float)(m_ScreenX + halfNeedleWidth);
	pVerts[3].y		= (float)(m_ScreenY - halfNeedleHeight);
	pVerts[3].z		= 0.0f;
	pVerts[3].rhw	= 1.0f;
	pVerts[3].uv.u	= 1.0f;
	pVerts[3].uv.v	= 1.0f;
	pVerts[3].color	= 0xFFFFFFFF;

	// Render
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, pVerts, 4, TVERTEX, &m_NeedleTexture, 1 );
}

// Draw the direction finder
void SiegeCompass::DrawDirectionFinder( Rapi& renderer, const matrix_3x3& orient )
{
	tVertex	pVerts[4];
	memset( pVerts, 0, sizeof( tVertex ) * 4 );

	// Calculate half of our needle dimensions
	float halfDirectionWidth	= (float)m_DirectionWidth * 0.5f;
	float halfDirectionHeight	= (float)m_DirectionHeight * 0.5f;

	// Fill in our verts with the information for the needle
	vector_3 offset	= orient * vector_3( -halfDirectionWidth, halfDirectionHeight, 0.0f );
	pVerts[0].x		= (float)m_ScreenX + offset.x;
	pVerts[0].y		= (float)m_ScreenY + offset.y;
	pVerts[0].z		= 0.0f;
	pVerts[0].rhw	= 1.0f;
	pVerts[0].uv.u	= 0.0f;
	pVerts[0].uv.v	= 0.0f;
	pVerts[0].color	= 0xFFFFFFFF;

	offset			= orient * vector_3( halfDirectionWidth, halfDirectionHeight, 0.0f );
	pVerts[1].x		= (float)m_ScreenX + offset.x;
	pVerts[1].y		= (float)m_ScreenY + offset.y;
	pVerts[1].z		= 0.0f;
	pVerts[1].rhw	= 1.0f;
	pVerts[1].uv.u	= 1.0f;
	pVerts[1].uv.v	= 0.0f;
	pVerts[1].color	= 0xFFFFFFFF;

	offset			= orient * vector_3( -halfDirectionWidth, -halfDirectionHeight, 0.0f );
	pVerts[2].x		= (float)m_ScreenX + offset.x;
	pVerts[2].y		= (float)m_ScreenY + offset.y;
	pVerts[2].z		= 0.0f;
	pVerts[2].rhw	= 1.0f;
	pVerts[2].uv.u	= 0.0f;
	pVerts[2].uv.v	= 1.0f;
	pVerts[2].color	= 0xFFFFFFFF;

	offset			= orient * vector_3( halfDirectionWidth, -halfDirectionHeight, 0.0f );
	pVerts[3].x		= (float)m_ScreenX + offset.x;
	pVerts[3].y		= (float)m_ScreenY + offset.y;
	pVerts[3].z		= 0.0f;
	pVerts[3].rhw	= 1.0f;
	pVerts[3].uv.u	= 1.0f;
	pVerts[3].uv.v	= 1.0f;
	pVerts[3].color	= 0xFFFFFFFF;

	// Render
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, pVerts, 4, TVERTEX, &m_DirectionTexture, 1 );
}

// Draw the cardinal directions
void SiegeCompass::DrawCardinals( Rapi& renderer, const matrix_3x3& orient )
{
	tVertex	pVerts[4];
	memset( pVerts, 0, sizeof( tVertex ) * 4 );

	// Calculate half of our cardinal dimensions
	float halfCardinalWidth		= (float)m_CardinalWidth * 0.5f;
	float halfCardinalHeight	= (float)m_CardinalHeight * 0.5f;

	vector_3 offset_vect		= orient * vector_3( 0.0f, (float)-m_CardDist, 0.0f );
	matrix_3x3 rotation			= ZRotationColumns( RADIANS_90 );

	pVerts[0].z		= 0.0f;
	pVerts[0].rhw	= 1.0f;
	pVerts[0].uv.u	= 0.0f;
	pVerts[0].uv.v	= 0.0f;
	pVerts[0].color	= 0xFFFFFFFF;

	pVerts[1].z		= 0.0f;
	pVerts[1].rhw	= 1.0f;
	pVerts[1].uv.u	= 1.0f;
	pVerts[1].uv.v	= 0.0f;
	pVerts[1].color	= 0xFFFFFFFF;

	pVerts[2].z		= 0.0f;
	pVerts[2].rhw	= 1.0f;
	pVerts[2].uv.u	= 0.0f;
	pVerts[2].uv.v	= 1.0f;
	pVerts[2].color	= 0xFFFFFFFF;

	pVerts[3].z		= 0.0f;
	pVerts[3].rhw	= 1.0f;
	pVerts[3].uv.u	= 1.0f;
	pVerts[3].uv.v	= 1.0f;
	pVerts[3].color	= 0xFFFFFFFF;

	// Draw north
	float cent_x	= m_ScreenX + offset_vect.x;
	float cent_y	= m_ScreenY + offset_vect.y;

	pVerts[0].x		= cent_x - halfCardinalWidth;
	pVerts[0].y		= cent_y + halfCardinalHeight;
	pVerts[1].x		= cent_x + halfCardinalWidth;
	pVerts[1].y		= cent_y + halfCardinalHeight;
	pVerts[2].x		= cent_x - halfCardinalWidth;
	pVerts[2].y		= cent_y - halfCardinalHeight;
	pVerts[3].x		= cent_x + halfCardinalWidth;
	pVerts[3].y		= cent_y - halfCardinalHeight;

	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, pVerts, 4, TVERTEX, &m_NorthTexture, 1 );

	// Draw east
	offset_vect		= rotation * offset_vect;
	cent_x			= m_ScreenX + offset_vect.x;
	cent_y			= m_ScreenY + offset_vect.y;

	pVerts[0].x		= cent_x - halfCardinalWidth;
	pVerts[0].y		= cent_y + halfCardinalHeight;
	pVerts[1].x		= cent_x + halfCardinalWidth;
	pVerts[1].y		= cent_y + halfCardinalHeight;
	pVerts[2].x		= cent_x - halfCardinalWidth;
	pVerts[2].y		= cent_y - halfCardinalHeight;
	pVerts[3].x		= cent_x + halfCardinalWidth;
	pVerts[3].y		= cent_y - halfCardinalHeight;

	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, pVerts, 4, TVERTEX, &m_EastTexture, 1 );

	// Draw south
	offset_vect		= rotation * offset_vect;
	cent_x			= m_ScreenX + offset_vect.x;
	cent_y			= m_ScreenY + offset_vect.y;

	pVerts[0].x		= cent_x - halfCardinalWidth;
	pVerts[0].y		= cent_y + halfCardinalHeight;
	pVerts[1].x		= cent_x + halfCardinalWidth;
	pVerts[1].y		= cent_y + halfCardinalHeight;
	pVerts[2].x		= cent_x - halfCardinalWidth;
	pVerts[2].y		= cent_y - halfCardinalHeight;
	pVerts[3].x		= cent_x + halfCardinalWidth;
	pVerts[3].y		= cent_y - halfCardinalHeight;

	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, pVerts, 4, TVERTEX, &m_SouthTexture, 1 );

	// Draw west
	offset_vect		= rotation * offset_vect;
	cent_x			= m_ScreenX + offset_vect.x;
	cent_y			= m_ScreenY + offset_vect.y;

	pVerts[0].x		= cent_x - halfCardinalWidth;
	pVerts[0].y		= cent_y + halfCardinalHeight;
	pVerts[1].x		= cent_x + halfCardinalWidth;
	pVerts[1].y		= cent_y + halfCardinalHeight;
	pVerts[2].x		= cent_x - halfCardinalWidth;
	pVerts[2].y		= cent_y - halfCardinalHeight;
	pVerts[3].x		= cent_x + halfCardinalWidth;
	pVerts[3].y		= cent_y - halfCardinalHeight;

	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, pVerts, 4, TVERTEX, &m_WestTexture, 1 );
}

// Draw the hotpoint indicator
void SiegeCompass::DrawHotpoint( Rapi& renderer, const matrix_3x3& orient )
{
	tVertex	pVerts[4];
	memset( pVerts, 0, sizeof( tVertex ) * 4 );

	// Calculate half of our needle dimensions
	float halfHotpointWidth		= (float)m_HotpointWidth * 0.5f;
	float halfHotpointHeight	= (float)m_HotpointHeight * 0.5f;

	// Fill in our verts with the information for the needle
	vector_3 offset	= orient * vector_3( -halfHotpointWidth, halfHotpointHeight, 0.0f );
	pVerts[0].x		= (float)m_ScreenX + offset.x;
	pVerts[0].y		= (float)m_ScreenY + offset.y;
	pVerts[0].z		= 0.0f;
	pVerts[0].rhw	= 1.0f;
	pVerts[0].uv.u	= 0.0f;
	pVerts[0].uv.v	= 0.0f;
	pVerts[0].color	= 0xFFFFFFFF;

	offset			= orient * vector_3( halfHotpointWidth, halfHotpointHeight, 0.0f );
	pVerts[1].x		= (float)m_ScreenX + offset.x;
	pVerts[1].y		= (float)m_ScreenY + offset.y;
	pVerts[1].z		= 0.0f;
	pVerts[1].rhw	= 1.0f;
	pVerts[1].uv.u	= 1.0f;
	pVerts[1].uv.v	= 0.0f;
	pVerts[1].color	= 0xFFFFFFFF;

	offset			= orient * vector_3( -halfHotpointWidth, -halfHotpointHeight, 0.0f );
	pVerts[2].x		= (float)m_ScreenX + offset.x;
	pVerts[2].y		= (float)m_ScreenY + offset.y;
	pVerts[2].z		= 0.0f;
	pVerts[2].rhw	= 1.0f;
	pVerts[2].uv.u	= 0.0f;
	pVerts[2].uv.v	= 1.0f;
	pVerts[2].color	= 0xFFFFFFFF;

	offset			= orient * vector_3( halfHotpointWidth, -halfHotpointHeight, 0.0f );
	pVerts[3].x		= (float)m_ScreenX + offset.x;
	pVerts[3].y		= (float)m_ScreenY + offset.y;
	pVerts[3].z		= 0.0f;
	pVerts[3].rhw	= 1.0f;
	pVerts[3].uv.u	= 1.0f;
	pVerts[3].uv.v	= 1.0f;
	pVerts[3].color	= 0xFFFFFFFF;

	// Render
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, pVerts, 4, TVERTEX, &m_HotpointTexture, 1 );
}

void SiegeCompass::DrawMinimized( Rapi & renderer, const matrix_3x3& orient )
{
	tVertex	pVerts[4];
	memset( pVerts, 0, sizeof( tVertex ) * 4 );

	// Calculate half of our needle dimensions
	float halfMinimizedWidth	= (float)m_MinimizedWidth * 0.5f;
	float halfMinimizedHeight	= (float)m_MinimizedHeight * 0.5f;

	// Fill in our verts with the information for the needle
	vector_3 offset	= orient * vector_3( -halfMinimizedWidth, halfMinimizedHeight, 0.0f );
	pVerts[0].x		= (float)m_ScreenX + offset.x;
	pVerts[0].y		= (float)m_ScreenY + offset.y;
	pVerts[0].z		= 0.0f;
	pVerts[0].rhw	= 1.0f;
	pVerts[0].uv.u	= 0.0f;
	pVerts[0].uv.v	= 0.0f;
	pVerts[0].color	= 0xFFFFFFFF;

	offset			= orient * vector_3( halfMinimizedWidth, halfMinimizedHeight, 0.0f );
	pVerts[1].x		= (float)m_ScreenX + offset.x;
	pVerts[1].y		= (float)m_ScreenY + offset.y;
	pVerts[1].z		= 0.0f;
	pVerts[1].rhw	= 1.0f;
	pVerts[1].uv.u	= 1.0f;
	pVerts[1].uv.v	= 0.0f;
	pVerts[1].color	= 0xFFFFFFFF;

	offset			= orient * vector_3( -halfMinimizedWidth, -halfMinimizedHeight, 0.0f );
	pVerts[2].x		= (float)m_ScreenX + offset.x;
	pVerts[2].y		= (float)m_ScreenY + offset.y;
	pVerts[2].z		= 0.0f;
	pVerts[2].rhw	= 1.0f;
	pVerts[2].uv.u	= 0.0f;
	pVerts[2].uv.v	= 1.0f;
	pVerts[2].color	= 0xFFFFFFFF;

	offset			= orient * vector_3( halfMinimizedWidth, -halfMinimizedHeight, 0.0f );
	pVerts[3].x		= (float)m_ScreenX + offset.x;
	pVerts[3].y		= (float)m_ScreenY + offset.y;
	pVerts[3].z		= 0.0f;
	pVerts[3].rhw	= 1.0f;
	pVerts[3].uv.u	= 1.0f;
	pVerts[3].uv.v	= 1.0f;
	pVerts[3].color	= 0xFFFFFFFF;

	// Render
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, pVerts, 4, TVERTEX, &m_MinimizedTexture, 1 );
}

// Get the orientation matrix using the current camera and targetnode north vector
matrix_3x3 SiegeCompass::GetNorthDirectionMatrix( const matrix_3x3& cam_orient )
{
	vector_3 cam_dir	= cam_orient.GetColumn2_T();
	cam_dir.y			= 0.0;

	vector_3 north		= gSiegeHotpointDatabase.GetHotpointDirection( L"north", gSiegeEngine.NodeWalker().TargetNodeGUID() );
	if( north.IsZero() || cam_dir.IsZero() )
	{
		return matrix_3x3::IDENTITY; 
	}

	vector_3 norm_axis( cam_dir.z, 0.0f, -cam_dir.x );

	// Correct for zero cross
	if( norm_axis.IsZero() )
	{
		norm_axis.Set( 0.0f, 0.0f, 1.0f );
	}
	else
	{
		norm_axis.Normalize();
	}

	return ZRotationColumns( NormAngleBetween( north, cam_dir, norm_axis ) );
}

// Get the orientation matrix using the current camera and active hotpoint direction
matrix_3x3 SiegeCompass::GetActiveHotpointDirectionMatrix( const matrix_3x3& cam_orient )
{
	vector_3 cam_dir	= cam_orient.GetColumn2_T();
	cam_dir.y			= 0.0;

	vector_3 hot_dir	= gSiegeHotpointDatabase.GetActiveHotpointDirection();
	if( hot_dir.IsZero() || cam_dir.IsZero() )
	{
		return matrix_3x3::IDENTITY; 
	}

	vector_3 norm_axis( cam_dir.z, 0.0f, -cam_dir.x );

	// Correct for zero cross
	if( norm_axis.IsZero() )
	{
		norm_axis.Set( 0.0f, 0.0f, 1.0f );
	}
	else
	{
		norm_axis.Normalize();
	}

	return ZRotationColumns( NormAngleBetween( hot_dir, cam_dir, norm_axis ) );
}