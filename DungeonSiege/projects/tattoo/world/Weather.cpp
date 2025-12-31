/***********************************************************************************/
//
//									Weather
//
//		See .h file for details
//
/***********************************************************************************/

#include "precomp_world.h"

#include "Weather.h"

#include "RapiOwner.h"
#include "gps_manager.h"
#include "namingkey.h"
#include "siege_engine.h"
#include "siege_camera.h"
#include "siege_hotpoint_database.h"
#include "siege_light_database.h"
#include "matrix_3x3.h"
#include "space_3.h"

#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "FuBiTraitsimpl.h"
#include "GoSupport.h"


// Rain construction
Rain::Rain()
	: m_maxDropCount( 0 )
	, m_maxDensity( 1.0f )
	, m_minDropLength( 0.0f )
	, m_maxDropLength( 0.0f )
	, m_minLightningDelay( 0.0f )
	, m_maxLightningDelay( 0.0f )
	, m_minThunderDelay( 0.0f )
	, m_maxThunderDelay( 0.0f )
	, m_dropHeadColor( 0xFFFFFFFF )
	, m_dropTailColor( 0xFFFFFFFF )
	, m_dropSpeed( 0.0f )
	, m_rainRadius( 0.0f )
	, m_ceilingOffset( 0.0f )
	, m_rainSound( "" )
	, m_lightningSound( "" )
	, m_bRaining( false )
	, m_dropCount( 0 )
	, m_density( 0.0f )
	, m_bLightning( true )
	, m_lightningTime( 0.0f )
	, m_bThunder( false )
	, m_thunderTime( 0.0f )
	, m_flashLight( siege::UNDEFINED_GUID )
	, m_flashTime( 0.0f )
	, m_rainSample( GPGSound::INVALID_SOUND_ID )
{
	// Attempt to open rain configuration block
	FastFuelHandle hRain( "config:rain" );

	if( !hRain.IsValid() )
	{
		gperror( "Unable to load rain settings!" );
		return;
	}

	// Maximum rain drop settings
	m_maxDropCount		= hRain.GetInt( "max_drop_count" );
	m_maxDensity		= hRain.GetFloat( "max_density" );

	// Length of each rain drop
	m_minDropLength		= hRain.GetFloat( "min_drop_length" );
	m_maxDropLength		= hRain.GetFloat( "max_drop_length" );

	// Lightning time delays
	m_minLightningDelay	= hRain.GetFloat( "min_lightning_delay" );
	m_maxLightningDelay	= hRain.GetFloat( "max_lightning_delay" );
	m_minThunderDelay	= hRain.GetFloat( "min_thunder_delay" );
	m_maxThunderDelay	= hRain.GetFloat( "max_thunder_delay" );

	// Color
	m_dropHeadColor		= hRain.GetInt( "drop_head_color" );
	m_dropTailColor		= hRain.GetInt( "drop_tail_color" );

	// Speed at which the rain falls
	m_dropSpeed			= hRain.GetFloat( "drop_speed" );

	// Radius in which the rain falls
	m_rainRadius		= hRain.GetFloat( "rain_radius" );

	// Ceiling offset for rain spawn
	m_ceilingOffset		= hRain.GetFloat( "ceiling_offset" );

	// Sound information
	m_rainSound			= hRain.GetString( "rain_sound" );
	m_lightningSound	= hRain.GetString( "lightning_sound" );

	// Initialize lightning timers
	m_lightningTime		= Random( m_minLightningDelay, m_maxLightningDelay );
}

// Rain destruction
Rain::~Rain()
{
}

// Update the rain
void Rain::Update( float secondsElapsed )
{
	// Don't update if we are not raining
	if( !m_bRaining )
	{
		// Make sure sound is stopped
		if( gSoundManager.IsSamplePlaying( m_rainSample ) )
		{
			gSoundManager.StopSample( m_rainSample );
			m_rainSample	= GPGSound::INVALID_SOUND_ID;
		}

		if( m_flashLight != siege::UNDEFINED_GUID )
		{
			gSiegeEngine.LightDatabase().DestroyLight( m_flashLight );
			m_flashLight	= siege::UNDEFINED_GUID;
		}

		// Make sure no drops are still hanging around
		if( !m_rainDropMap.empty() )
		{
			m_rainDropMap.clear();
			m_dropCount	= 0;
		}
		return;
	}

	// Make sure that rain sound is playing
	if( !gSoundManager.IsSamplePlaying( m_rainSample ) )
	{
		m_rainSample	= gSoundManager.PlaySample( m_rainSound, true, GPGSound::SND_EFFECT_AMBIENT );
	}

	// Update volume of rain based on density
	gSoundManager.SetSampleVolume( m_rainSample, FTOL( (float)gSoundManager.GetSampleVolume( GPGSound::SND_EFFECT_AMBIENT ) * (m_density / m_maxDensity) ) );

	// Update lightning
	if( m_bLightning )
	{
		m_lightningTime	-= secondsElapsed;

		if( m_bThunder )
		{
			m_thunderTime	-= secondsElapsed;
			m_flashTime		-= secondsElapsed;

			if( m_thunderTime <= 0.0f )
			{
				// Play thunder sound
				gSoundManager.PlaySample( m_lightningSound, false, GPGSound::SND_EFFECT_AMBIENT );

				// Set thunder state
				m_bThunder	= false;
			}

			if( (m_flashLight != siege::UNDEFINED_GUID) && (m_flashTime <= 0.0f) )
			{
				gSiegeEngine.LightDatabase().DestroyLight( m_flashLight );
				m_flashLight = siege::UNDEFINED_GUID;
			}
		}

		if( m_lightningTime <= 0.0f )
		{
			// Start a lightning strike
			siege::SiegeFrustum* pFrustum = gSiegeEngine.GetFrustum( gSiegeEngine.GetRenderFrustum() );
			if( pFrustum )
			{
				m_flashLight	= gSiegeEngine.LightDatabase().InsertDirectionalLightSource( SiegePos( vector_3::UP, pFrustum->GetPosition().node ),
																							 vector_3( 0.95f, 0.95f, 1.0f ) );
				m_flashTime		= 0.1f;

				// Update lightning time
				m_lightningTime	= Random( m_minLightningDelay, m_maxLightningDelay );

				// Update thunder
				m_bThunder		= true;
				m_thunderTime	= Random( m_minThunderDelay, m_maxThunderDelay );
			}
		}
	}
	else
	{
		if( m_flashLight != siege::UNDEFINED_GUID )
		{
			gSiegeEngine.LightDatabase().DestroyLight( m_flashLight );
			m_flashLight	= siege::UNDEFINED_GUID;
		}
	}

	// Update existing drops
	for( DropMap::iterator i = m_rainDropMap.begin(); i != m_rainDropMap.end(); )
	{
		// Get a handle to the node
		siege::SiegeNode* pNode	= gSiegeEngine.IsNodeValid( (*i).first );
		if( pNode )
		{
			// Calculate a drip vector
			vector_3 drip_vector			= gWeather.GetWind()->GetNodeDirection( pNode->GetGUID() );
			drip_vector.y					-= m_dropSpeed;
			drip_vector						= secondsElapsed * drip_vector;

			if( !drip_vector.IsZero() )
			{
				vector_3 norm_drip				= Normalize( drip_vector );

				// Go through each rain drop
				for( DropList::iterator d = (*i).second.begin(); d != (*i).second.end(); )
				{
					// Save the old head position
					(*d).head_pos	+= drip_vector;
					(*d).tail_pos	= (*d).head_pos - (norm_drip * (*d).tail_length);

					if( (*d).head_pos.y < pNode->GetMinimumBounds().y )
					{
						// Bye bye rain drop!
						d	= (*i).second.erase( d );
						--m_dropCount;
					}
					else
					{
						++d;
					}
				}
			}
		}
		else
		{
			(*i).second.clear();
		}

		// See if we need this entry anymore
		if( (*i).second.empty() )
		{
			i = m_rainDropMap.erase( i );
		}
		else
		{
			++i;
		}
	}

	// Calculate a spawn origin and offset it in Y
	vector_3 origin		= gSiegeEngine.GetCamera().GetCameraPosition() +
						  ( Normalize( gWeather.GetCameraOrientation().GetColumn2_T() ) * (m_rainRadius * 0.75f) );
	origin.y			= gSiegeEngine.GetCamera().GetCameraPosition().y + m_ceilingOffset;

	// Get target node
	const siege::SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( gSiegeEngine.NodeWalker().TargetNodeGUID() );
	const siege::SiegeNode& node		= handle.RequestObject( gSiegeEngine.NodeCache() );

	// Figure out how many drop we want to spawn this update
	int drops_to_spawn		= min_t( m_maxDropCount - m_dropCount, (int)(m_density * secondsElapsed) );
	m_dropCount				+= drops_to_spawn;

	if( drops_to_spawn )
	{
		// Find this node in our list
		DropMap::iterator i = m_rainDropMap.find( node.GetGUID() );
		if( i == m_rainDropMap.end() )
		{
			// Insert a new listing
			m_rainDropMap.insert( std::pair< siege::database_guid, DropList >( node.GetGUID(), DropList() ) );
			i = m_rainDropMap.find( node.GetGUID() );
		}

		// Spawn new drops
		for( int d = 0; d < drops_to_spawn; ++d )
		{
			// Calculate spawn offset
			float thetacos, thetasin;
			SINCOSF( Random( 0.0f, PI2 ), thetasin, thetacos );
			vector_3 new_drop_pos( Random( 0.0f, m_rainRadius )		* thetacos,
								   Random( 0.0f, secondsElapsed )	* -m_dropSpeed,
								   Random( 0.0f, m_rainRadius )		* thetasin );

			RainDrop drop;
			drop.head_pos		= origin + new_drop_pos;
			drop.tail_pos		= drop.head_pos;
			drop.tail_length	= Random( m_minDropLength, m_maxDropLength );

			(*i).second.push_back( drop );
		}
	}
}

// Draw the rain
void Rain::Draw()
{
	// Return if there is nothing to draw
	if( m_rainDropMap.empty() )
	{
		return;
	}
	
	// Disable texturing
	gDefaultRapi.SetTextureStageState(0,
								D3DTOP_SELECTARG2,
								D3DTOP_SELECTARG2,
								D3DTADDRESS_WRAP,
								D3DTADDRESS_WRAP,
								D3DTEXF_POINT,
								D3DTEXF_POINT,
								D3DTEXF_POINT,
								D3DBLEND_SRCALPHA,
								D3DBLEND_INVSRCALPHA,
								false );

	// Go through each node that has drops
	for( DropMap::iterator i = m_rainDropMap.begin(); i != m_rainDropMap.end(); ++i )
	{
		// Get a handle to the node
		siege::SiegeNode* pNode	= gSiegeEngine.IsNodeValid( (*i).first );
		if( pNode )
		{
			// Set the world matrix for these drops
			gDefaultRapi.SetWorldMatrix( pNode->GetCurrentOrientation(), pNode->GetCurrentCenter() );

			// Attempt to lock a buffer of vertices
			unsigned int index	 = 0;
			unsigned int startVert;
			sVertex* pVerts;
			if( gDefaultRapi.LockBufferedVertices( (*i).second.size() * 2, startVert, pVerts ) )
			{
				// Go through each rain drop
				for( DropList::iterator d = (*i).second.begin(); d != (*i).second.end(); ++d, index += 2 )
				{
					// Draw the drop
					pVerts[index].x			= (*d).head_pos.x;	
					pVerts[index].y			= (*d).head_pos.y;	
					pVerts[index].z			= (*d).head_pos.z;	
					pVerts[index].color		= m_dropHeadColor;
					
					pVerts[index+1].x		= (*d).tail_pos.x;	
					pVerts[index+1].y		= (*d).tail_pos.y;	
					pVerts[index+1].z		= (*d).tail_pos.z;	
					pVerts[index+1].color	= m_dropTailColor;	
				}

				// Unlock buffer
				gDefaultRapi.UnlockBufferedVertices();
				gDefaultRapi.DrawBufferedPrimitive( D3DPT_LINELIST, startVert, index );
			}
		}
	}
}


// Snow construction
Snow::Snow()
	: m_maxFlakeCount( 0 )
	, m_maxDensity( 1.0f )
	, m_flakeSize( 0.0f )
	, m_snowPerturb( 0.0f )
	, m_minFlakeSpeed( 0.0f )
	, m_maxFlakeSpeed( 0.0f )
	, m_snowRadius( 0.0f )
	, m_ceilingOffset( 0.0f )
	, m_bSnowing( false )
	, m_flakeCount( 0 )
	, m_density( 0.0f )
{
	// Attempt to open snow configuration block
	FastFuelHandle hSnow( "config:snow" );

	if( !hSnow.IsValid() )
	{
		gperror( "Unable to load snow settings!" );
		return;
	}

	// Maximum snow flake settings
	m_maxFlakeCount		= hSnow.GetInt( "max_flake_count" );
	m_maxDensity		= hSnow.GetFloat( "max_density" );

	// Size of each flake
	m_flakeSize			= hSnow.GetFloat( "flake_size" );

	// Maximum perturb for snow direction
	m_snowPerturb		= hSnow.GetFloat( "snow_perturb" );

	// Speed at which the snow falls
	m_minFlakeSpeed		= hSnow.GetFloat( "min_flake_speed" );
	m_maxFlakeSpeed		= hSnow.GetFloat( "max_flake_speed" );

	// Radius in which the snow falls
	m_snowRadius		= hSnow.GetFloat( "snow_radius" );

	// Ceiling offset for rain spawn
	m_ceilingOffset		= hSnow.GetFloat( "ceiling_offset" );

	// Texture for the snow flakes
	gpstring texName;
	if( gNamingKey.BuildContentLocation( hSnow.GetString( "snow_texture" ), texName ) )
	{
		texName			+= ".%img%";
		m_flakeTexture	= gDefaultRapi.CreateTexture( texName, false, 0, TEX_LOCKED );
	}
}

// Rain destruction
Snow::~Snow()
{
	// Destroy texture
	if( m_flakeTexture )
	{
		gDefaultRapi.DestroyTexture( m_flakeTexture );
	}
}

// Update the rain
void Snow::Update( float secondsElapsed )
{
	// Don't update if we are not raining
	if( !m_bSnowing )
	{
		// Make sure no flakes are still hanging around
		if( !m_snowFlakeMap.empty() )
		{
			m_snowFlakeMap.clear();
			m_flakeCount	= 0;
		}
		return;
	}

	// Update existing flakes
	for( FlakeMap::iterator i = m_snowFlakeMap.begin(); i != m_snowFlakeMap.end(); )
	{
		// Get a handle to the node
		siege::SiegeNode* pNode	= gSiegeEngine.IsNodeValid( (*i).first );
		if( pNode )
		{
			vector_3 wind_direction			= gWeather.GetWind()->GetNodeDirection( pNode->GetGUID() );

			// Go through each snow flake
			for( FlakeList::iterator f = (*i).second.begin(); f != (*i).second.end(); )
			{
				// Add the offset
				(*f).pos		+= ( (wind_direction + (*f).fall_vector) * secondsElapsed );

				if( (*f).pos.y < pNode->GetMinimumBounds().y )
				{
					// Bye bye snow flake!
					f	= (*i).second.erase( f );
					--m_flakeCount;
				}
				else
				{
					++f;
				}
			}
		}
		else
		{
			(*i).second.clear();
		}

		// See if we need this entry anymore
		if( (*i).second.empty() )
		{
			i = m_snowFlakeMap.erase( i );
		}
		else
		{
			++i;
		}
	}

	// Calculate a spawn origin and offset it in Y
	vector_3 origin		= gSiegeEngine.GetCamera().GetCameraPosition() +
						  ( Normalize( gWeather.GetCameraOrientation().GetColumn2_T() ) * (m_snowRadius * 0.75f) );
	origin.y			= gSiegeEngine.GetCamera().GetCameraPosition().y + m_ceilingOffset;

	// Get target node
	const siege::SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( gSiegeEngine.NodeWalker().TargetNodeGUID() );
	const siege::SiegeNode& node		= handle.RequestObject( gSiegeEngine.NodeCache() );

	// Figure out how many drop we want to spawn this update
	int flakes_to_spawn		= min_t( m_maxFlakeCount - m_flakeCount, (int)(m_density * secondsElapsed) );
	m_flakeCount			+= flakes_to_spawn;

	if( flakes_to_spawn )
	{
		// Find this node in our list
		FlakeMap::iterator i = m_snowFlakeMap.find( node.GetGUID() );
		if( i == m_snowFlakeMap.end() )
		{
			// Insert a new listing
			m_snowFlakeMap.insert( std::pair< siege::database_guid, FlakeList >( node.GetGUID(), FlakeList() ) );
			i = m_snowFlakeMap.find( node.GetGUID() );
		}

		// Spawn new flakes
		for( int f = 0; f < flakes_to_spawn; ++f )
		{
			float thetacos, thetasin;
			SINCOSF( Random( 0.0f, PI2 ), thetasin, thetacos );

			SnowFlake flake;
			flake.pos			= origin + vector_3( Random( 0.0f, m_snowRadius )	* thetacos,
													 Random( 0.0f, secondsElapsed )	* -m_maxFlakeSpeed,
													 Random( 0.0f, m_snowRadius )	* thetasin );

			flake.fall_vector	= vector_3( Random( -m_snowPerturb, m_snowPerturb ),
											Random( -m_minFlakeSpeed, -m_maxFlakeSpeed ),
											Random( -m_snowPerturb, m_snowPerturb ) );

			(*i).second.push_back( flake );
		}
	}
}

// Draw the rain
void Snow::Draw()
{
	// Return if there is nothing to draw
	if( m_snowFlakeMap.empty() )
	{
		return;
	}
	
	// Enable texturing
	gDefaultRapi.SetTextureStageState(0,
								D3DTOP_SELECTARG1,
								D3DTOP_SELECTARG1,
								D3DTADDRESS_CLAMP,
								D3DTADDRESS_CLAMP,
								D3DTEXF_LINEAR,
								D3DTEXF_LINEAR,
								D3DTEXF_LINEAR,
								D3DBLEND_SRCALPHA,
								D3DBLEND_ONE,
								true );

	// Set the snow texture
	gDefaultRapi.SetTexture( 0, m_flakeTexture );

	// Go through each node that has drops
	for( FlakeMap::iterator i = m_snowFlakeMap.begin(); i != m_snowFlakeMap.end(); ++i )
	{
		// Get a handle to the node
		siege::SiegeNode* pNode	= gSiegeEngine.IsNodeValid( (*i).first );
		if( pNode )
		{
			// Set the world matrix for these flakes
			gDefaultRapi.SetWorldMatrix( pNode->GetCurrentOrientation(), pNode->GetCurrentCenter() );

			// Calculate offset vectors
			vector_3 xVect	= (pNode->GetTransposeOrientation() * gWeather.GetCameraOrientation().GetColumn0_T()) * (m_flakeSize * 2.0f);
			vector_3 yVect	= (pNode->GetTransposeOrientation() * gWeather.GetCameraOrientation().GetColumn1_T()) * (m_flakeSize * 2.0f);

			// Attempt to lock a buffer of vertices
			unsigned int index	= 0;
			unsigned int startVert;
			sVertex* pVerts;
			if( gDefaultRapi.LockBufferedVertices( (*i).second.size() * 3, startVert, pVerts ) )
			{
				// Go through each rain drop
				for( FlakeList::iterator f = (*i).second.begin(); f != (*i).second.end(); ++f, index += 3 )
				{
					// Draw the flake
					pVerts[index].x			= (*f).pos.x;	
					pVerts[index].y			= (*f).pos.y;	
					pVerts[index].z			= (*f).pos.z;
					pVerts[index].uv.u		= 0.0f;
					pVerts[index].uv.v		= 1.0f;
					pVerts[index].color		= 0xFFFFFFFF;

					pVerts[index+1].x		= (*f).pos.x + yVect.x;	
					pVerts[index+1].y		= (*f).pos.y + yVect.y;	
					pVerts[index+1].z		= (*f).pos.z + yVect.z;
					pVerts[index+1].uv.u	= 0.0f;
					pVerts[index+1].uv.v	= -1.0f;
					pVerts[index+1].color	= 0xFFFFFFFF;

					pVerts[index+2].x		= (*f).pos.x + xVect.x;	
					pVerts[index+2].y		= (*f).pos.y + xVect.y;	
					pVerts[index+2].z		= (*f).pos.z + xVect.z;
					pVerts[index+2].uv.u	= 2.0f;
					pVerts[index+2].uv.v	= 1.0f;
					pVerts[index+2].color	= 0xFFFFFFFF;
				}

				// Unlock buffer
				gDefaultRapi.UnlockBufferedVertices();
				gDefaultRapi.DrawBufferedPrimitive( D3DPT_TRIANGLELIST, startVert, index );
			}
		}
	}
}


// Wind construction
Wind::Wind()
	: m_maxVelocity( 0.0f )
	, m_bBlowing( false )
	, m_velocity( 0.0f )
	, m_windDirection( 0.0f )
	, m_windSample( GPGSound::INVALID_SOUND_ID )
{
	// Attempt to open wind configuration block
	FastFuelHandle hWind( "config:wind" );

	if( !hWind.IsValid() )
	{
		gperror( "Unable to load wind settings!" );
		return;
	}

	// Maximum wind velocity
	m_maxVelocity		= hWind.GetFloat( "max_velocity" );

	// Wind sound
	m_windSound			= hWind.GetString( "wind_sound" );
}

// Destruction
Wind::~Wind()
{
}

// Update the snow
void Wind::Update( float secondsElapsed )
{
	UNREFERENCED_PARAMETER( secondsElapsed );

	if( !m_bBlowing )
	{
		// Make sure sound is stopped
		if( gSoundManager.IsSamplePlaying( m_windSample ) )
		{
			gSoundManager.StopSample( m_windSample );
			m_windSample	= GPGSound::INVALID_SOUND_ID;
		}
		return;
	}

	// Make sure that wind sound is playing
	if( !gSoundManager.IsSamplePlaying( m_windSample ) )
	{
		m_windSample	= gSoundManager.PlaySample( m_windSound, true, GPGSound::SND_EFFECT_AMBIENT );
	}

	// Update volume of wind based on velocity
	gSoundManager.SetSampleVolume( m_windSample, FTOL( (float)GPGSound::SOUND_MAX_VOLUME * (m_velocity / m_maxVelocity) ) );
}

// Get the direction of the wind
vector_3 Wind::GetWorldDirection()
{
	return GetNodeDirection( gSiegeEngine.NodeWalker().TargetNodeGUID() );
}

vector_3 Wind::GetNodeDirection( siege::database_guid node )
{
	if( !m_bBlowing || !node.IsValidSlow() )
	{
		return vector_3::ZERO;
	}

	// Get the north direction
	vector_3 direction = gSiegeHotpointDatabase.GetNodalHotpointDirection( L"north", node );

	// Rotate the vector
	return( (YRotationColumns( m_windDirection ) * direction) * m_velocity );
}


// Weather construction
Weather::Weather()
	: m_bEnabled( true )
{
	// Create weather components
	m_pRain		= new Rain();
	m_pWind		= new Wind();
	m_pSnow		= new Snow();
}

// Destruction
Weather::~Weather()
{
	// Destroy weather components
	Delete( m_pRain );
	Delete( m_pWind );
	Delete( m_pSnow );
}

// Update the weather
void Weather::Update( float secondsElapsed )
{
	GPSTATS_GROUP( GROUP_UPDATE_FX );

	m_pRain->Update( secondsElapsed );
	m_pWind->Update( secondsElapsed );
	m_pSnow->Update( secondsElapsed );
}

// Draw the weather
void Weather::Draw()
{
	GPSTATS_GROUP( GROUP_RENDER_FX );

	// Disable Z writes
	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE,	FALSE );

	// Draw the weather components
	m_pRain->Draw();
	m_pSnow->Draw();

	// Enable Z writes
	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE,	TRUE );
}

// Set/Get the enabled state
void Weather::SetEnabled( bool bEnabled )
{
	m_bEnabled	= bEnabled;

	if( !bEnabled )
	{
		// Disable each component
		m_pRain->SetRaining( false );
		m_pSnow->SetSnowing( false );
		m_pWind->SetBlowing( false );
	}
	else
	{
		// Enable each component
		m_pRain->SetRaining( true );
		m_pSnow->SetSnowing( true );
		m_pWind->SetBlowing( true );
	}
}
