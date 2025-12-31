/***********************************************************************************/
//
//									Mood
//
//		See .h file for details
//
/***********************************************************************************/

#include "precomp_world.h"

#include "mood.h"
#include "gps_manager.h"
#include "weather.h"
#include "siege_engine.h"
#include "siege_camera.h"

#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "FuBiTraitsimpl.h"
#include "GoSupport.h"


FUBI_EXPORT_ENUM( eMoodRoomType, RT_BEGIN, RT_COUNT );


// Constants
const gpstring		INVALID_MOOD_NAME;		// empty string
const MoodSetting	INVALID_MOOD_SETTING;	// empty setting

FUBI_DECLARE_XFER_TRAITS( GoMoodSetting )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		persist.Xfer( "m_moodName",					obj.m_moodName );
		persist.Xfer( "m_bFrustumTransition",		obj.m_bFrustumTransition );
		persist.Xfer( "m_frustumTotalTransTime",	obj.m_frustumTotalTransTime );
		persist.Xfer( "m_frustumTransTime",			obj.m_frustumTransTime );
		persist.Xfer( "m_frustumTransWidth",		obj.m_frustumTransWidth );
		persist.Xfer( "m_frustumTransHeight",		obj.m_frustumTransHeight );
		persist.Xfer( "m_frustumTransDepth",		obj.m_frustumTransDepth, obj.m_frustumTransHeight );

		return true;
	}
};

static const char* s_MoodRoomTypeStrings[] =
{
	"rt_generic",
	"rt_paddedcell",
	"rt_room",
	"rt_bathroom",
	"rt_livingroom",
	"rt_stoneroom",
	"rt_auditorium",
	"rt_concerthall",
	"rt_cave",
	"rt_arena",
	"rt_hangar",
	"rt_carpetedhallway",
	"rt_hallway",
	"rt_stonecorridor",
	"rt_alley",
	"rt_forest",
	"rt_city",
	"rt_mountains",
	"rt_quarry",
	"rt_plain",
	"rt_parkinglot",
	"rt_sewerpipe",
	"rt_underwater",
	"rt_drugged",
	"rt_dizzy",
	"rt_psychotic",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_MoodRoomTypeStrings ) == RT_COUNT );

static stringtool::EnumStringConverter s_MoodRoomTypeConverter(
		s_MoodRoomTypeStrings, RT_BEGIN, RT_END, RT_INVALID );

const char* ToString( eMoodRoomType val )
	{  return ( s_MoodRoomTypeConverter.ToString( val ) );  }
bool FromString( const char* str, eMoodRoomType& val )
	{  return ( s_MoodRoomTypeConverter.FromString( str, rcast <int&> ( val ) ) );  }

// Conflict management
void ManageMoods( Skrit::HObject skrit, const char* funcName, GoidColl&, bool, float )
{
	CHECK_PRIMARY_THREAD_ONLY;
	SKRIT_CALLV( ManageMoods, skrit, funcName );
}


// MoodSetting construction
MoodSetting::MoodSetting()
	: m_moodName( "" )
	, m_transitionTime( 0.0f )
	, m_bInterior( true )
	, m_bFrustumEnabled( false )
	, m_frustumWidth( 0.0f )
	, m_frustumHeight( 0.0f )
	, m_frustumDepth( 0.0f )
	, m_bFogEnabled( false )
	, m_fogNearDist( 0.0f )
	, m_fogFarDist( 0.0f )
	, m_fogColor( 0xFF000000 )
	, m_fogDensity( 1.0f )
	, m_bRainEnabled( false )
	, m_rainDensity( 0.0f )
	, m_rainLowDetDensity( 0.0f )
	, m_bLightning( false )
	, m_bSnowEnabled( false )
	, m_snowDensity( 0.0f )
	, m_snowLowDetDensity( 0.0f )
	, m_bWindEnabled( false )
	, m_windVelocity( 0.0f )
	, m_windDirection( 0.0f )
	, m_bMusicEnabled( false )
	, m_ambientTrack( "" )
	, m_standardTrack( "" )
	, m_battleTrack( "" )
	, m_ambientIntroDelay( 0.0f )
	, m_ambientRepeatDelay( 0.0f )
	, m_standardIntroDelay( 0.0f )
	, m_standardRepeatDelay( 0.0f )
	, m_battleIntroDelay( 0.0f )
	, m_battleRepeatDelay( 0.0f )
	, m_bSunEnabled( false )
	, m_moodRoomType( RT_GENERIC )
{
}


// Mood construction
Mood::Mood()
	: m_bEnabled( true )
	, m_fogCameraOffset( 0.0f )
	, m_activeSunGo( GOID_INVALID )
	, m_activeFogGo( GOID_INVALID )
	, m_activeRainGo( GOID_INVALID )
	, m_activeSnowGo( GOID_INVALID )
	, m_activeWindGo( GOID_INVALID )
	, m_activeMusicGo( GOID_INVALID )
	, m_pActiveSunMood( NULL )
	, m_pActiveFogMood( NULL )
	, m_pActiveRainMood( NULL )
	, m_pActiveSnowMood( NULL )
	, m_pActiveWindMood( NULL )
	, m_pActiveMusicMood( NULL )
	, m_bSunTransition( false )
	, m_sunTransTime( 0.0f )
	, m_bFogTransition( false )
	, m_fogTotalTransTime( 0.0f )
	, m_fogTransTime( 0.0f )
	, m_fogTransNearDist( 0.0f )
	, m_fogTransFarDist( 0.0f )
	, m_fogTransColor( 0xFFFFFFFF )
	, m_fogTransDensity( 1.0f )
	, m_bRainTransition( false )
	, m_rainTotalTransTime( 0.0f )
	, m_rainTransTime( 0.0f )
	, m_rainTransDensity( 0.0f )
	, m_bSnowTransition( false )
	, m_snowTotalTransTime( 0.0f )
	, m_snowTransTime( 0.0f )
	, m_snowTransDensity( 0.0f )
	, m_bWindTransition( false )
	, m_windTotalTransTime( 0.0f )
	, m_windTransTime( 0.0f )
	, m_windTransVelocity( 0.0f )
	, m_windTransDirection( 0.0f )
	, m_bMusicTransition( false )
	, m_musicTransTime( 0.0f )
	, m_bAmbientTrack( false )
	, m_ambientFadeTime( 0.0f )
	, m_activeAmbientTrack( GPGSound::INVALID_SOUND_ID )
	, m_bAmbientTrackDelay( false )
	, m_ambientTrackDelay( 0.0f )
	, m_bStandardTrack( false )
	, m_standardFadeTime( 0.0f )
	, m_activeStandardTrack( GPGSound::INVALID_SOUND_ID )
	, m_bStandardTrackDelay( false )
	, m_standardTrackDelay( 0.0f )
	, m_bBattleTrack( false )
	, m_battleFadeTime( 0.0f )
	, m_activeBattleTrack( GPGSound::INVALID_SOUND_ID )
	, m_bBattleTrackDelay( false )
	, m_battleTrackDelay( 0.0f )
	, m_bFadeVolume( false )
	, m_fadeVolumeTime( 0.0f )
	, m_fadeVolumeTotalTime( 0.0f )
	, m_fadeVolume( 0 )
	, m_resetFadeVolume( 0 )
	, m_bFogCorrect( false )
	, m_bFogRemoval( false )
	, m_fogCorrectTime( 0.0f )
	, m_fogCorrectTotalTime( 0.0f )
{
	// Initialize moods
	InitMoods( false );
}

// Mood deconstruction
Mood::~Mood()
{
	// Release skrit
	m_moodManager.ReleaseSkrit();
}

// Initialize the moods
void Mood::InitMoods( bool bReload )
{
	// Read global parent mood setting fuel blocks
	FastFuelHandle hMoods( "world:global:moods" );
	if( bReload )
	{
		// Clear collections
		m_moodSettingMap.clear();
		m_originalMoods.clear();

		// Indicate we want force updates
		ForceUpdateFog( 0.0f );
		ForceUpdateRain( 0.0f );
		ForceUpdateSnow( 0.0f );
		ForceUpdateWind( 0.0f );
		ForceUpdateMusic( 0.0f );
	}

	if( !hMoods.IsValid() )
	{
		gpwarning( "Unable to load any moods!  No mood settings will be available this session." );
		return;
	}

	// Iterate thorugh our mood blocks and build our mapping
	FastFuelHandleColl moodList;
	hMoods.ListChildrenNamed( moodList, "mood_setting*", 2 );
	for( FastFuelHandleColl::iterator i = moodList.begin(); i != moodList.end(); ++i )
	{
		MoodSetting moodSetting;

		// Get the mood name and requested transition time
		moodSetting.m_moodName					= (*i).GetString( "mood_name" );
		moodSetting.m_transitionTime			= (*i).GetFloat( "transition_time" );
		moodSetting.m_bInterior					= (*i).GetBool( "interior" );

		// Look for frustum
		FastFuelHandle hFrustum	= (*i).GetChildNamed( "frustum" );
		if( hFrustum.IsValid() )
		{
			moodSetting.m_bFrustumEnabled		= true;

			// Get the frustum settings
			moodSetting.m_frustumWidth			= hFrustum.GetFloat( "frustum_width" );
			moodSetting.m_frustumHeight			= hFrustum.GetFloat( "frustum_height" );
			moodSetting.m_frustumDepth			= hFrustum.GetFloat( "frustum_depth", moodSetting.m_frustumHeight );
		}

		// Look for fog
		FastFuelHandle hFog	= (*i).GetChildNamed( "fog" );
		if( hFog.IsValid() )
		{
			moodSetting.m_bFogEnabled			= true;

			// Get fog settings
			moodSetting.m_fogNearDist			= hFog.GetFloat( "fog_near_dist" );
			moodSetting.m_fogFarDist			= hFog.GetFloat( "fog_far_dist" );
			if( hFog.HasKey( "fog_lowdetail_near_dist" ) )
			{
				moodSetting.m_fogLowDetNearDist	= hFog.GetFloat( "fog_lowdetail_near_dist" );
			}
			else
			{
				moodSetting.m_fogLowDetNearDist	= moodSetting.m_fogNearDist;
			}
			if( hFog.HasKey( "fog_lowdetail_far_dist" ) )
			{
				moodSetting.m_fogLowDetFarDist	= hFog.GetFloat( "fog_lowdetail_far_dist" );
			}
			else
			{
				moodSetting.m_fogLowDetFarDist	= moodSetting.m_fogFarDist;
			}
			moodSetting.m_fogColor				= hFog.GetInt( "fog_color", 0xFF000000 );
			moodSetting.m_fogDensity			= hFog.GetFloat( "fog_density", 1.0f );
		}

		// Look for rain
		FastFuelHandle hRain = (*i).GetChildNamed( "rain" );
		if( hRain.IsValid() )
		{
			moodSetting.m_bRainEnabled			= true;
			
			// Get rain settings
			moodSetting.m_rainDensity			= hRain.GetFloat( "rain_density", 0.0f );
			if( hRain.HasKey( "rain_lowdetail_density" ) )
			{
				moodSetting.m_rainLowDetDensity	= hFog.GetFloat( "rain_lowdetail_density" );
			}
			else
			{
				moodSetting.m_rainLowDetDensity	= moodSetting.m_rainDensity;
			}
			moodSetting.m_bLightning			= hRain.GetBool( "lightning", false );
		}

		// Look for snow
		FastFuelHandle hSnow = (*i).GetChildNamed( "snow" );
		if( hSnow.IsValid() )
		{
			moodSetting.m_bSnowEnabled			= true;

			// Get snow settings
			moodSetting.m_snowDensity			= hSnow.GetFloat( "snow_density" );
			if( hSnow.HasKey( "snow_lowdetail_density" ) )
			{
				moodSetting.m_snowLowDetDensity	= hFog.GetFloat( "snow_lowdetail_density" );
			}
			else
			{
				moodSetting.m_snowLowDetDensity	= moodSetting.m_snowDensity;
			}
		}

		// Look for wind
		FastFuelHandle hWind = (*i).GetChildNamed( "wind" );
		if( hWind.IsValid() )
		{
			moodSetting.m_bWindEnabled			= true;

			// Get wind settings
			moodSetting.m_windVelocity			= hWind.GetFloat( "wind_velocity" );
			moodSetting.m_windDirection			= hWind.GetFloat( "wind_direction" );
		}

		// Look for music
		FastFuelHandle hMusic = (*i).GetChildNamed( "music" );
		if( hMusic.IsValid() )
		{
			moodSetting.m_bMusicEnabled			= true;

			// Get music settings
			moodSetting.m_ambientTrack			= hMusic.GetString( "ambient_track" );
			moodSetting.m_ambientIntroDelay		= hMusic.GetFloat( "ambient_intro_delay" );
			moodSetting.m_ambientRepeatDelay	= hMusic.GetFloat( "ambient_repeat_delay" );

			moodSetting.m_standardTrack			= hMusic.GetString( "standard_track" );
			moodSetting.m_standardIntroDelay	= hMusic.GetFloat( "standard_intro_delay" );
			moodSetting.m_standardRepeatDelay	= hMusic.GetFloat( "standard_repeat_delay" );

			moodSetting.m_battleTrack			= hMusic.GetString( "battle_track" );
			moodSetting.m_battleIntroDelay		= hMusic.GetFloat( "battle_intro_delay" );
			moodSetting.m_battleRepeatDelay		= hMusic.GetFloat( "battle_repeat_delay" );

			FromString( hMusic.GetString( "room_type" ), moodSetting.m_moodRoomType );
		}

		// Look for sun
		FastFuelHandle hSun = (*i).GetChildNamed( "sun" );
		if( hSun.IsValid() )
		{
			moodSetting.m_bSunEnabled		= true;

			// Get sun settings
			FastFuelHandleColl time_blocks;
			hSun.ListChildren( time_blocks );
			for( FastFuelHandleColl::const_iterator i = time_blocks.begin(); i != time_blocks.end(); ++i )
			{
				// Make a new block
				TimeLight tLight;

				// Get the hour and minute of this block
				sscanf( (*i).GetName(), "%dh%dm", &tLight.m_Time.m_Hour, &tLight.m_Time.m_Minute );

				// Get the color
				tLight.m_Color	= 0xFFFFFFFF;
				(*i).Get( "color", tLight.m_Color, false );

				// Put the new light on our list
				moodSetting.m_timeLightList.push_back( tLight );
			}
		}

		// Put this mood into our mapping
		if( m_moodSettingMap.find( moodSetting.m_moodName ) == m_moodSettingMap.end() )
		{
			m_moodSettingMap.insert( std::pair< gpstring, MoodSetting >( moodSetting.m_moodName, moodSetting ) );
			m_originalMoods.insert( std::pair< gpstring, MoodSetting >( moodSetting.m_moodName, moodSetting ) );
		}
		else
		{
			gperrorf(( "Duplicate mood '%s' found!  Discarding duplicate mood.", moodSetting.m_moodName.c_str() ));
		}
	}

	// Create handle to conflict management skrit
	FastFuelHandle hMoodManager	= hMoods.GetChildNamed( "mood_manager" );
	if( hMoodManager.IsValid() )
	{
		gpstring skritName( hMoodManager.GetString( "skrit" ) );
		if ( !skritName.empty() )
		{
			if( bReload )
			{
				// Release skrit
				m_moodManager.ReleaseSkrit();
			}

			Skrit::CreateReq createReq( skritName );
			createReq.SetPersist( false );			// $ important: this skrit is const, so do not persist (otherwise it fucks up save game)
			m_moodManager = Skrit::HObject( createReq );
		}
	}

	// Cleanup since this should be a one time access
	hMoods.Unload();

	if ( !m_moodManager )
	{
		gperror( "Could not find/compile mood management skrit!  Moods will be disabled." );
		m_bEnabled	= false;
	}
}

// Persist the moods
bool Mood::Xfer( FuBi::PersistContext &persist )
{
	// No need to persist the moodsetting map, as it is all static data
	// However, everything else is potentially dynamic, and must be persisted.

	// Map of GO's that have requested moods to the mood names they have requested
	persist.XferMap( "go_mood_map",				m_goMoodMap );

	// Active setting GO's
	persist.Xfer( "active_sun_go",				m_activeSunGo );
	persist.Xfer( "active_fog_go",				m_activeFogGo );
	persist.Xfer( "active_rain_go",				m_activeRainGo );
	persist.Xfer( "active_snow_go",				m_activeSnowGo );
	persist.Xfer( "active_wind_go",				m_activeWindGo );
	persist.Xfer( "active_music_go",			m_activeMusicGo );

	// Active settings
	if( persist.IsSaving() )
	{
		persist.Xfer( "active_sun_mood",		m_pActiveSunMood ? m_pActiveSunMood->m_moodName : INVALID_MOOD_NAME );
		persist.Xfer( "active_fog_mood",		m_pActiveFogMood ? m_pActiveFogMood->m_moodName : INVALID_MOOD_NAME );
		persist.Xfer( "active_rain_mood",		m_pActiveRainMood ? m_pActiveRainMood->m_moodName : INVALID_MOOD_NAME );
		persist.Xfer( "active_snow_mood",		m_pActiveSnowMood ? m_pActiveSnowMood->m_moodName : INVALID_MOOD_NAME );
		persist.Xfer( "active_wind_mood",		m_pActiveWindMood ? m_pActiveWindMood->m_moodName : INVALID_MOOD_NAME );
		persist.Xfer( "active_music_mood",		m_pActiveMusicMood ? m_pActiveMusicMood->m_moodName : INVALID_MOOD_NAME );
	}
	else if( persist.IsRestoring() )
	{
		// Load in the active mood strings and attempt to convert them to pointers
		gpstring moodName;
		MoodSettingMap::iterator i;

		// Sun
		persist.Xfer( "active_sun_mood",		moodName );
		i	= m_moodSettingMap.find( moodName );
		if( i != m_moodSettingMap.end() )
		{
			m_pActiveSunMood		= &(*i).second;
		}

		// Sun
		persist.Xfer( "active_fog_mood",		moodName );
		i	= m_moodSettingMap.find( moodName );
		if( i != m_moodSettingMap.end() )
		{
			m_pActiveFogMood		= &(*i).second;
		}

		// Sun
		persist.Xfer( "active_rain_mood",		moodName );
		i	= m_moodSettingMap.find( moodName );
		if( i != m_moodSettingMap.end() )
		{
			m_pActiveRainMood		= &(*i).second;
		}

		// Sun
		persist.Xfer( "active_snow_mood",		moodName );
		i	= m_moodSettingMap.find( moodName );
		if( i != m_moodSettingMap.end() )
		{
			m_pActiveSnowMood		= &(*i).second;
		}

		// Sun
		persist.Xfer( "active_wind_mood",		moodName );
		i	= m_moodSettingMap.find( moodName );
		if( i != m_moodSettingMap.end() )
		{
			m_pActiveWindMood		= &(*i).second;
		}

		// Sun
		persist.Xfer( "active_music_mood",		moodName );
		i	= m_moodSettingMap.find( moodName );
		if( i != m_moodSettingMap.end() )
		{
			m_pActiveMusicMood		= &(*i).second;
		}
	}

	// Set transitions to true to force an instant update
	m_bSunTransition		= true;
	m_sunTransTime			= 0.0f;

	m_bFogTransition		= true;
	m_fogTotalTransTime		= 0.0f;
	m_fogTransTime			= 0.0f;

	m_bRainTransition		= true;
	m_rainTotalTransTime	= 0.0f;
	m_rainTransTime			= 0.0f;

	m_bSnowTransition		= true;
	m_snowTotalTransTime	= 0.0f;
	m_snowTransTime			= 0.0f;

	m_bWindTransition		= true;
	m_windTotalTransTime	= 0.0f;
	m_windTransTime			= 0.0f;

	m_bMusicTransition		= true;
	m_musicTransTime		= 0.0f;

	return true;
}

// Update the mood
void Mood::Update( float secondsElapsed )
{
	if( !m_bEnabled )
	{
		return;
	}

	// Go through the entire GoMoodMap and update any potential frustum transitions
	for( GoMoodMap::iterator m = m_goMoodMap.begin(); m != m_goMoodMap.end(); ++m )
	{
		if( (*m).second.m_bFrustumTransition )
		{
			// Get the frustum id from the GoDb
			siege::SiegeFrustum* pFrustum			= NULL;
			FrustumId fId							= gGoDb.GetGoFrustum( (*m).first );
			if( fId != FRUSTUMID_INVALID )
			{
				pFrustum = gSiegeEngine.GetFrustum( MakeInt( fId ) );

				// Find the mood
				MoodSettingMap::iterator mood = m_moodSettingMap.find( (*m).second.m_moodName );
				if( mood != m_moodSettingMap.end() )
				{
					// Calculate the current transition level
					(*m).second.m_frustumTransTime		+= secondsElapsed;
					float blend							= 1.0f;
					if( !IsZero( (*m).second.m_frustumTotalTransTime ) )
					{
						blend			= min_t( 1.0f, (*m).second.m_frustumTransTime / (*m).second.m_frustumTotalTransTime );
					}

					// Update the frustum dimensions
					pFrustum->SetDimensions( BlendValue( (*m).second.m_frustumTransWidth,  (*mood).second.m_frustumWidth,  blend ),
											 BlendValue( (*m).second.m_frustumTransDepth,  (*mood).second.m_frustumDepth,  blend ),
											 BlendValue( (*m).second.m_frustumTransHeight, (*mood).second.m_frustumHeight, blend ) );

					// Check for completion
					if( IsOne( blend ) )
					{
						(*m).second.m_bFrustumTransition = false;
					}
				}
			}
		}
	}

	// Get the party
	Player* player	= NULL;
	if( Server::DoesSingletonExist() && ((player = gServer.GetScreenPlayer()) != NULL) )
	{
		Go* party	= player->GetParty();
		if( party )
		{
			// Build collection of party members
			GoidColl coll;
			party->GetChildren().Translate( coll );

			// Call conflict management skrit
			if( !coll.empty() )
			{
				ManageMoods( m_moodManager, "ManageMoods$", coll, false, secondsElapsed );
			}
		}
	}

	// Check for sun transition
	if( m_bSunTransition && m_pActiveSunMood )
	{
		// See if the sun is enabled
		if( m_pActiveSunMood->m_bSunEnabled )
		{
			// Use the time light list that was specified
			gTimeOfDay.SetTimeLightList( m_pActiveSunMood->m_timeLightList, m_sunTransTime );
		}
		else
		{
			// Use the default time light list
			gTimeOfDay.SetTimeLightList( gTimeOfDay.GetDefaultTimeLightList(), m_sunTransTime );
		}

		// Clear transition values
		m_sunTransTime		= 0.0f;
		m_bSunTransition	= false;
	}

	// Calculate the current camera offset
	if( m_bFogCorrect )
	{
		// Calculate blend times
		m_fogCorrectTime	+= secondsElapsed;
		float blend			= 1.0f;
		if( !IsZero( m_fogCorrectTotalTime ) )
		{
			blend			= min_t( 1.0f, m_fogCorrectTime / m_fogCorrectTotalTime );
		}

		// Invert if we are removing the fog
		if( m_bFogRemoval )
		{
			blend			= 1.0f - blend;
		}

		// Calculate the blended offset
		m_fogCameraOffset	= blend * (gSiegeEngine.GetCamera().GetVectorOrientation().Length() - gSiegeEngine.GetCamera().GetMaxDistance());

		// See if transition is complete
		if( IsOne( blend ) )
		{
			m_fogCorrectTotalTime	= 0.0f;
			m_fogCorrectTime		= 0.0f;
			m_bFogCorrect			= false;
		}
	}
	else
	{
		// No correction, just use the standard offset
		m_fogCameraOffset	= gSiegeEngine.GetCamera().GetVectorOrientation().Length() - gSiegeEngine.GetCamera().GetMaxDistance();
	}

	// Check for fog transition
	if( m_pActiveFogMood && m_bFogTransition )
	{
		// Set the requested enable state
		if( !gDefaultRapi.GetFogActivatedState() && m_pActiveFogMood->m_bFogEnabled )
		{
			gDefaultRapi.SetFogActivatedState( true );
		}

		// Calculate blend times
		m_fogTransTime		+= secondsElapsed;
		float blend			= 1.0f;
		if( !IsZero( m_fogTotalTransTime ) )
		{
			blend			= min_t( 1.0f, m_fogTransTime / m_fogTotalTransTime );
		}

		// Calculate interpolated color value
		DWORD color			= InterpolateColor( m_pActiveFogMood->m_fogColor, m_fogTransColor, blend );

		// Set new fog values
		gDefaultRapi.SetClearColor( color );
		gDefaultRapi.SetFogColor( color );

		if( m_pActiveFogMood->m_bFogEnabled )
		{
			gDefaultRapi.SetFogDensity( BlendValue( m_fogTransDensity, m_pActiveFogMood->m_fogDensity, blend ) );
			gDefaultRapi.SetFogDistances( BlendValue( m_fogTransNearDist,
													  BlendValue( m_pActiveFogMood->m_fogLowDetNearDist,
																  m_pActiveFogMood->m_fogNearDist,
																  gWorldOptions.GetObjectDetailLevel() ),
													  blend ) + m_fogCameraOffset,
										  BlendValue( m_fogTransFarDist,
													  BlendValue( m_pActiveFogMood->m_fogLowDetFarDist,
																  m_pActiveFogMood->m_fogFarDist,
																  gWorldOptions.GetObjectDetailLevel() ),
													  blend ) + m_fogCameraOffset );
		}
		else
		{
			gDefaultRapi.SetFogDensity( BlendValue( m_fogTransDensity, 1.0f, blend ) );
			gDefaultRapi.SetFogDistances( BlendValue( m_fogTransNearDist, gDefaultRapi.GetClipNearDist(), blend ) + m_fogCameraOffset,
										  BlendValue( m_fogTransFarDist, gDefaultRapi.GetClipFarDist(), blend ) + m_fogCameraOffset );
		}

		// See if transition is complete
		if( IsOne( blend ) )
		{
			m_fogTotalTransTime	= 0.0f;
			m_fogTransTime		= 0.0f;
			m_bFogTransition	= false;

			if( !m_pActiveFogMood->m_bFogEnabled )
			{
				gDefaultRapi.SetFogActivatedState( false );
			}
		}
	}
	else
	{
		if( m_pActiveFogMood && m_pActiveFogMood->m_bFogEnabled )
		{
			gDefaultRapi.SetFogDistances( BlendValue( m_pActiveFogMood->m_fogLowDetNearDist,
													  m_pActiveFogMood->m_fogNearDist,
													  gWorldOptions.GetObjectDetailLevel() ) + m_fogCameraOffset,
										  BlendValue( m_pActiveFogMood->m_fogLowDetFarDist,
													  m_pActiveFogMood->m_fogFarDist,
													  gWorldOptions.GetObjectDetailLevel() ) + m_fogCameraOffset );
		}
		else
		{
			gDefaultRapi.SetFogDistances( gDefaultRapi.GetClipNearDist() + m_fogCameraOffset,
										  gDefaultRapi.GetClipFarDist() + m_fogCameraOffset );
		}
	}

	// Check for rain transition
	if( m_bRainTransition && m_pActiveRainMood )
	{
		// Get rain handle
		Rain& rain	= *gWeather.GetRain();

		// Set the requested enable state
		if( !rain.GetRaining() && m_pActiveRainMood->m_bRainEnabled )
		{
			rain.SetRaining( true );
		}
		if( rain.GetLightning() != m_pActiveRainMood->m_bLightning )
		{
			rain.SetLightning( m_pActiveRainMood->m_bLightning );
		}

		// Calculate blend times
		m_rainTransTime		+= secondsElapsed;
		float blend			= 1.0f;
		if( !IsZero( m_rainTotalTransTime ) )
		{
			blend			= min_t( 1.0f, m_rainTransTime / m_rainTotalTransTime );
		}

		if( m_pActiveRainMood->m_bRainEnabled )
		{
			rain.SetDensity( BlendValue( m_rainTransDensity,
										 BlendValue( m_pActiveRainMood->m_rainLowDetDensity,
													 m_pActiveRainMood->m_rainDensity,
													 gWorldOptions.GetObjectDetailLevel() ),
										 blend ) );
		}
		else
		{
			rain.SetDensity( BlendValue( m_rainTransDensity, 0.0f, blend ) );
		}

		// See if transition is complete
		if( IsOne( blend ) )
		{
			m_rainTotalTransTime	= 0.0f;
			m_rainTransTime			= 0.0f;
			m_bRainTransition		= false;

			if( !m_pActiveRainMood->m_bRainEnabled )
			{
				rain.SetRaining( false );
			}
		}
	}

	// Check for snow transition
	if( m_bSnowTransition && m_pActiveSnowMood )
	{
		// Get rain handle
		Snow& snow	= *gWeather.GetSnow();

		// Set the requested enable state
		if( !snow.GetSnowing() && m_pActiveSnowMood->m_bSnowEnabled )
		{
			snow.SetSnowing( true );
		}

		// Calculate blend times
		m_snowTransTime		+= secondsElapsed;
		float blend			= 1.0f;
		if( !IsZero( m_snowTotalTransTime ) )
		{
			blend			= min_t( 1.0f, m_snowTransTime / m_snowTotalTransTime );
		}

		if( m_pActiveSnowMood->m_bSnowEnabled )
		{
			snow.SetDensity( BlendValue( m_snowTransDensity,
										 BlendValue( m_pActiveSnowMood->m_snowLowDetDensity,
													 m_pActiveSnowMood->m_snowDensity,
													 gWorldOptions.GetObjectDetailLevel() ),
										 blend ) );
		}
		else
		{
			snow.SetDensity( BlendValue( m_snowTransDensity, 0.0f, blend ) );
		}

		// See if transition is complete
		if( IsOne( blend ) )
		{
			m_snowTotalTransTime	= 0.0f;
			m_snowTransTime			= 0.0f;
			m_bSnowTransition		= false;

			if( !m_pActiveSnowMood->m_bSnowEnabled )
			{
				snow.SetSnowing( false );
			}
		}
	}

	// Check for wind transition
	if( m_bWindTransition && m_pActiveWindMood )
	{
		// Get rain handle
		Wind& wind	= *gWeather.GetWind();

		// Set the requested enable state
		if( !wind.GetBlowing() && m_pActiveWindMood->m_bWindEnabled )
		{
			wind.SetBlowing( true );
		}

		// Calculate blend times
		m_windTransTime		+= secondsElapsed;
		float blend			= 1.0f;
		if( !IsZero( m_windTotalTransTime ) )
		{
			blend			= min_t( 1.0f, m_windTransTime / m_windTotalTransTime );
		}

		if( m_pActiveWindMood->m_bWindEnabled )
		{
			wind.SetVelocity( BlendValue( m_windTransVelocity, m_pActiveWindMood->m_windVelocity, blend ) );
			wind.SetAngularDirection( BlendValue( m_windTransDirection, m_pActiveWindMood->m_windDirection, blend ) );
		}
		else
		{
			wind.SetVelocity( BlendValue( m_windTransVelocity, 0.0f, blend ) );
			wind.SetAngularDirection( BlendValue( m_windTransDirection, 0.0f, blend ) );
		}

		// See if transition is complete
		if( IsOne( blend ) )
		{
			m_windTotalTransTime	= 0.0f;
			m_windTransTime			= 0.0f;
			m_bWindTransition		= false;

			if( !m_pActiveWindMood->m_bWindEnabled )
			{
				wind.SetBlowing( false );
			}
		}
	}

	if( m_bAmbientTrackDelay && m_pActiveMusicMood )
	{
		m_ambientTrackDelay	-= secondsElapsed;
		if( m_ambientTrackDelay <= 0.0f )
		{
			gpassert( m_activeAmbientTrack == GPGSound::INVALID_SOUND_ID );

			// Play stream
			if( IsZero( m_pActiveMusicMood->m_ambientRepeatDelay ) )
			{
				m_activeAmbientTrack	= gSoundManager.PlayStream( m_pActiveMusicMood->m_ambientTrack, GPGSound::STRM_AMBIENT, true );
			}
			else
			{
				m_activeAmbientTrack	= gSoundManager.PlayStream( m_pActiveMusicMood->m_ambientTrack, GPGSound::STRM_AMBIENT, false );
				gSoundManager.SetStreamStopCallback( m_activeAmbientTrack, makeFunctor( *this, &Mood::StreamStopCallback ) );
			}

			// Clear state
			m_bAmbientTrackDelay = false;
		}
	}

	if( m_bStandardTrackDelay && m_pActiveMusicMood )
	{
		m_standardTrackDelay	-= secondsElapsed;
		if( m_standardTrackDelay <= 0.0f )
		{
			gpassert( m_activeStandardTrack == GPGSound::INVALID_SOUND_ID );

			// Play stream
			if( IsZero( m_pActiveMusicMood->m_standardRepeatDelay ) )
			{
				m_activeStandardTrack	= gSoundManager.PlayStream( m_pActiveMusicMood->m_standardTrack, GPGSound::STRM_MUSIC, true );
			}
			else
			{
				m_activeStandardTrack	= gSoundManager.PlayStream( m_pActiveMusicMood->m_standardTrack, GPGSound::STRM_MUSIC, false );
				gSoundManager.SetStreamStopCallback( m_activeStandardTrack, makeFunctor( *this, &Mood::StreamStopCallback ) );
			}

			// Clear state
			m_bStandardTrackDelay = false;
		}
	}

	if( m_bBattleTrackDelay && m_pActiveMusicMood )
	{
		m_battleTrackDelay	-= secondsElapsed;
		if( m_battleTrackDelay <= 0.0f )
		{
			gpassert( m_activeBattleTrack == GPGSound::INVALID_SOUND_ID );

			// Play stream
			if( IsZero( m_pActiveMusicMood->m_battleRepeatDelay ) )
			{
				m_activeBattleTrack	= gSoundManager.PlayStream( m_pActiveMusicMood->m_battleTrack, GPGSound::STRM_MUSIC, true );
			}
			else
			{
				m_activeBattleTrack	= gSoundManager.PlayStream( m_pActiveMusicMood->m_battleTrack, GPGSound::STRM_MUSIC, false );
				gSoundManager.SetStreamStopCallback( m_activeBattleTrack, makeFunctor( *this, &Mood::StreamStopCallback ) );
			}

			// Clear state
			m_bBattleTrackDelay = false;
		}
	}

	// Check for music transition
	if( m_bMusicTransition && m_pActiveMusicMood )
	{
		// Update the active room type
		if( ((DWORD)gSoundManager.GetRoomType() != (DWORD)m_pActiveMusicMood->m_moodRoomType) &&
			((DWORD)m_pActiveMusicMood->m_moodRoomType != 0xFFFFFFFF) )
		{
			gSoundManager.SetRoomType( (GPGSound::eRoomType)m_pActiveMusicMood->m_moodRoomType );
		}

		if( m_bAmbientTrack && !m_pActiveMusicMood->m_ambientTrack.same_no_case( m_activeAmbientTrackName ) )
		{
			// Fade the current stream using transition time
			if( gSoundManager.IsStreamPlaying( m_activeAmbientTrack ) )
			{
				gSoundManager.FadeStopStream( m_activeAmbientTrack, m_musicTransTime );
			}

			m_bAmbientTrackDelay	= false;
			m_ambientTrackDelay		= 0;

			// Play the new stream
			if( !m_pActiveMusicMood->m_ambientTrack.empty() )
			{
				if( IsZero( m_pActiveMusicMood->m_ambientIntroDelay ) )
				{
					if( IsZero( m_pActiveMusicMood->m_ambientRepeatDelay ) )
					{
						m_activeAmbientTrack	= gSoundManager.FadePlayStream( m_pActiveMusicMood->m_ambientTrack, GPGSound::STRM_AMBIENT, true, m_musicTransTime );
					}
					else
					{
						m_activeAmbientTrack	= gSoundManager.FadePlayStream( m_pActiveMusicMood->m_ambientTrack, GPGSound::STRM_AMBIENT, false, m_musicTransTime );
						gSoundManager.SetStreamStopCallback( m_activeAmbientTrack, makeFunctor( *this, &Mood::StreamStopCallback ) );
					}
				}
				else
				{
					m_bAmbientTrackDelay	= true;
					m_ambientTrackDelay		= m_pActiveMusicMood->m_ambientIntroDelay;
				}

				m_activeAmbientTrackName	= m_pActiveMusicMood->m_ambientTrack;
			}
			else
			{
				m_activeAmbientTrack		= GPGSound::INVALID_SOUND_ID;
				m_activeAmbientTrackName.clear();
			}
		}

		if( m_bStandardTrack && !m_pActiveMusicMood->m_standardTrack.same_no_case( m_activeStandardTrackName ) )
		{
			// Fade the current stream using transition time
			if( gSoundManager.IsStreamPlaying( m_activeStandardTrack ) )
			{
				gSoundManager.FadeStopStream( m_activeStandardTrack, m_musicTransTime );
			}

			m_bStandardTrackDelay	= false;
			m_standardTrackDelay	= 0;

			// Play the new stream
			if( !m_pActiveMusicMood->m_standardTrack.empty() )
			{
				if( IsZero( m_pActiveMusicMood->m_standardIntroDelay ) )
				{
					if( IsZero( m_pActiveMusicMood->m_standardRepeatDelay ) )
					{
						m_activeStandardTrack	= gSoundManager.FadePlayStream( m_pActiveMusicMood->m_standardTrack, GPGSound::STRM_MUSIC, true, m_musicTransTime );
					}
					else
					{
						m_activeStandardTrack	= gSoundManager.FadePlayStream( m_pActiveMusicMood->m_standardTrack, GPGSound::STRM_MUSIC, false, m_musicTransTime );
						gSoundManager.SetStreamStopCallback( m_activeStandardTrack, makeFunctor( *this, &Mood::StreamStopCallback ) );
					}
				}
				else
				{
					m_bStandardTrackDelay	= true;
					m_standardTrackDelay	= m_pActiveMusicMood->m_standardIntroDelay;
				}

				m_activeStandardTrackName	= m_pActiveMusicMood->m_standardTrack;
			}
			else
			{
				m_activeStandardTrack		= GPGSound::INVALID_SOUND_ID;
				m_activeStandardTrackName.clear();
			}
		}

		if( m_bBattleTrack && !m_pActiveMusicMood->m_battleTrack.same_no_case( m_activeBattleTrackName ) )
		{
			// Fade the current stream using transition time
			if( gSoundManager.IsStreamPlaying( m_activeBattleTrack ) )
			{
				gSoundManager.FadeStopStream( m_activeBattleTrack, m_musicTransTime );
			}

			m_bBattleTrackDelay		= false;
			m_battleTrackDelay		= 0;

			// Play the new stream
			if( !m_pActiveMusicMood->m_battleTrack.empty() )
			{
				if( IsZero( m_pActiveMusicMood->m_battleIntroDelay ) )
				{
					if( IsZero( m_pActiveMusicMood->m_battleRepeatDelay ) )
					{
						m_activeBattleTrack	= gSoundManager.FadePlayStream( m_pActiveMusicMood->m_battleTrack, GPGSound::STRM_MUSIC, true, m_musicTransTime );
					}
					else
					{
						m_activeBattleTrack	= gSoundManager.FadePlayStream( m_pActiveMusicMood->m_battleTrack, GPGSound::STRM_MUSIC, false, m_musicTransTime );
						gSoundManager.SetStreamStopCallback( m_activeBattleTrack, makeFunctor( *this, &Mood::StreamStopCallback ) );
					}
				}
				else
				{
					m_bBattleTrackDelay		= true;
					m_battleTrackDelay		= m_pActiveMusicMood->m_battleIntroDelay;
				}

				m_activeBattleTrackName		= m_pActiveMusicMood->m_battleTrack;
			}
			else
			{
				m_activeBattleTrack			= GPGSound::INVALID_SOUND_ID;
				m_activeBattleTrackName.clear();
			}
		}

		// Clear transition info
		m_musicTransTime	= 0.0f;
		m_bMusicTransition	= false;
	}

	// Update music
	if( m_pActiveMusicMood )
	{
		if( !m_bAmbientTrackDelay )
		{
			// Update ambient track
			bool bMusicPlaying				= gSoundManager.IsStreamPlaying( m_activeAmbientTrack );
			if( m_bAmbientTrack && !bMusicPlaying && !m_pActiveMusicMood->m_ambientTrack.empty() )
			{
				m_activeAmbientTrackName	= m_pActiveMusicMood->m_ambientTrack;
				if( !IsZero( m_pActiveMusicMood->m_ambientRepeatDelay ) )
				{
					m_activeAmbientTrack		= gSoundManager.FadePlayStream( m_pActiveMusicMood->m_ambientTrack, GPGSound::STRM_AMBIENT, false, m_ambientFadeTime );
					gSoundManager.SetStreamStopCallback( m_activeAmbientTrack, makeFunctor( *this, &Mood::StreamStopCallback ) );
				}
				else
				{
					m_activeAmbientTrack		= gSoundManager.FadePlayStream( m_pActiveMusicMood->m_ambientTrack, GPGSound::STRM_AMBIENT, true, m_ambientFadeTime );
				}
			}
			else if( !m_bAmbientTrack && bMusicPlaying )
			{
				gSoundManager.FadeStopStream( m_activeAmbientTrack, m_ambientFadeTime );
				m_activeAmbientTrack		= GPGSound::INVALID_SOUND_ID;
				m_activeAmbientTrackName.clear();
			}
		}

		if( !m_bStandardTrackDelay )
		{
			// Update standard track
			bool bMusicPlaying				= gSoundManager.IsStreamPlaying( m_activeStandardTrack );
			if( m_bStandardTrack && !bMusicPlaying && !m_pActiveMusicMood->m_standardTrack.empty() )
			{
				m_activeStandardTrackName	= m_pActiveMusicMood->m_standardTrack;
				if( !IsZero( m_pActiveMusicMood->m_standardRepeatDelay ) )
				{
					m_activeStandardTrack		= gSoundManager.FadePlayStream( m_pActiveMusicMood->m_standardTrack, GPGSound::STRM_MUSIC, false, m_standardFadeTime );
					gSoundManager.SetStreamStopCallback( m_activeStandardTrack, makeFunctor( *this, &Mood::StreamStopCallback ) );
				}
				else
				{
					m_activeStandardTrack		= gSoundManager.FadePlayStream( m_pActiveMusicMood->m_standardTrack, GPGSound::STRM_MUSIC, true, m_standardFadeTime );
				}
			}
			else if( !m_bStandardTrack && bMusicPlaying )
			{
				gSoundManager.FadeStopStream( m_activeStandardTrack, m_standardFadeTime );
				m_activeStandardTrack		= GPGSound::INVALID_SOUND_ID;
				m_activeStandardTrackName.clear();
			}
		}

		if( !m_bBattleTrackDelay )
		{
			// Update battle track
			bool bMusicPlaying				= gSoundManager.IsStreamPlaying( m_activeBattleTrack );
			if( m_bBattleTrack && !bMusicPlaying && !m_pActiveMusicMood->m_battleTrack.empty() )
			{
				m_activeBattleTrackName		= m_pActiveMusicMood->m_battleTrack;
				if( !IsZero( m_pActiveMusicMood->m_battleRepeatDelay ) )
				{
					m_activeBattleTrack			= gSoundManager.FadePlayStream( m_pActiveMusicMood->m_battleTrack, GPGSound::STRM_MUSIC, false, m_battleFadeTime );
					gSoundManager.SetStreamStopCallback( m_activeBattleTrack, makeFunctor( *this, &Mood::StreamStopCallback ) );
				}
				else
				{
					m_activeBattleTrack			= gSoundManager.FadePlayStream( m_pActiveMusicMood->m_battleTrack, GPGSound::STRM_MUSIC, true, m_battleFadeTime );
				}
			}
			else if( !m_bBattleTrack && bMusicPlaying )
			{
				gSoundManager.FadeStopStream( m_activeBattleTrack, m_battleFadeTime );
				m_activeBattleTrack			= GPGSound::INVALID_SOUND_ID;
				m_activeBattleTrackName.clear();
			}
		}
	}

	if( m_bFadeVolume )
	{
		// Calculate blend times
		m_fadeVolumeTime	+= secondsElapsed;
		float blend			= 1.0f;
		if( !IsZero( m_fadeVolumeTotalTime ) )
		{
			blend			= min_t( 1.0f, m_fadeVolumeTime / m_fadeVolumeTotalTime );
		}

		// Set the new volume
		gSoundManager.SetStreamVolume( GPGSound::STRM_MUSIC, FTOL( BlendValue( (float)m_resetFadeVolume, (float)m_fadeVolume, blend ) ) );

		// Check for completion
		if( IsOne( blend ) )
		{
			m_bFadeVolume	= false;
		}
	}
}

// Shutdown
void Mood::Shutdown()
{
	// Clear fog states
	gDefaultRapi.SetFogActivatedState( false );
	gDefaultRapi.SetClearColor( 0x00000000 );

	// Clear rain
	gWeather.GetRain()->SetRaining( false );
	gWeather.GetRain()->SetLightning( false );

	// Clear snow
	gWeather.GetSnow()->SetSnowing( false );

	// Clear wind
	gWeather.GetWind()->SetBlowing( false );

	// Stop music
	if( gSoundManager.IsStreamPlaying( m_activeAmbientTrack ) )
	{
		gSoundManager.StopStream( m_activeAmbientTrack );
		m_activeAmbientTrack		= GPGSound::INVALID_SOUND_ID;
		m_activeAmbientTrackName.clear();
	}
	if( gSoundManager.IsStreamPlaying( m_activeStandardTrack ) )
	{
		gSoundManager.StopStream( m_activeStandardTrack );
		m_activeStandardTrack		= GPGSound::INVALID_SOUND_ID;
		m_activeStandardTrackName.clear();
	}
	if( gSoundManager.IsStreamPlaying( m_activeBattleTrack ) )
	{
		gSoundManager.StopStream( m_activeBattleTrack );
		m_activeBattleTrack			= GPGSound::INVALID_SOUND_ID;
		m_activeBattleTrackName.clear();
	}
}

// Register a mood request from a GO
void Mood::RegisterMoodRequest( const Goid go, const gpstring& moodName )
{
	// Make sure the requested mood exists
	MoodSettingMap::iterator m = m_moodSettingMap.find( moodName );
	if( m == m_moodSettingMap.end() )
	{
		gperrorf(( "GO requested mood '%s', but that mood does not exist!  Disregarding request.", moodName.c_str() ));
		return;
	}

	// Create mood setting for this GO
	GoMoodSetting moodSetting;
	moodSetting.m_moodName					= moodName;

	// Get the frustum id from the GoDb
	siege::SiegeFrustum* pFrustum			= NULL;
	FrustumId fId							= gGoDb.GetGoFrustum( go );
	if( fId != FRUSTUMID_INVALID )
	{
		pFrustum = gSiegeEngine.GetFrustum( MakeInt( fId ) );
	}

	if( (*m).second.m_bFrustumEnabled && pFrustum )
	{
		moodSetting.m_bFrustumTransition	= true;
		moodSetting.m_frustumTotalTransTime	= (*m).second.m_transitionTime;
		moodSetting.m_frustumTransTime		= 0.0f;
		moodSetting.m_frustumTransWidth		= pFrustum->GetWidth();
		moodSetting.m_frustumTransHeight	= pFrustum->GetHeight();
		moodSetting.m_frustumTransDepth		= pFrustum->GetDepth();
	}
	else
	{
		moodSetting.m_bFrustumTransition	= false;
		moodSetting.m_frustumTotalTransTime	= 0.0f;
		moodSetting.m_frustumTransTime		= 0.0f;
		moodSetting.m_frustumTransWidth		= 0.0f;
		moodSetting.m_frustumTransHeight	= 0.0f;
		moodSetting.m_frustumTransDepth		= 0.0f;
	}

	// See if this go is already in our mapping
	GoMoodMap::iterator i = m_goMoodMap.find( go );
	if( i != m_goMoodMap.end() )
	{
		// Set this GO's requested mood to the new mood
		(*i).second		= moodSetting;
	}
	else
	{
		// Insert this go into the request mapping
		m_goMoodMap.insert( std::pair< Goid, GoMoodSetting >( go, moodSetting ) );
	}
}

// Unregister a mood request
void Mood::UnregisterMoodRequest( const Goid go )
{
	// See if we can find this go in our mood mapping
	GoMoodMap::iterator i = m_goMoodMap.find( go );
	if( i != m_goMoodMap.end() )
	{
		// Erase this entry
		m_goMoodMap.erase( i );
	}

	// See if any of the active mood GO's point to this go
	if( m_activeSunGo == go )
	{
		m_activeSunGo = GOID_INVALID;
	}
	if( m_activeFogGo == go )
	{
		m_activeFogGo = GOID_INVALID;
	}
	if( m_activeRainGo == go )
	{
		m_activeRainGo = GOID_INVALID;
	}
	if( m_activeSnowGo == go )
	{
		m_activeSnowGo = GOID_INVALID;
	}
	if( m_activeWindGo == go )
	{
		m_activeWindGo = GOID_INVALID;
	}
	if( m_activeMusicGo == go )
	{
		m_activeMusicGo = GOID_INVALID;
	}
}

// Get the requested mood for a given GO
const gpstring& Mood::GetGoRequestedMood( Goid go )
{
	// Find the GO
	GoMoodMap::iterator i = m_goMoodMap.find( go );
	if( i != m_goMoodMap.end() )
	{
		return (*i).second.m_moodName;
	}

	return INVALID_MOOD_NAME;
}

// Get the mood setting for a given GO
const MoodSetting& Mood::GetGoMoodSetting( Goid go )
{
	// Find the GO
	GoMoodMap::iterator i = m_goMoodMap.find( go );
	if( i != m_goMoodMap.end() )
	{
		// Find the mood
		MoodSettingMap::iterator m = m_moodSettingMap.find( (*i).second.m_moodName );
		if( m != m_moodSettingMap.end() )
		{
			return (*m).second;
		}
	}

	return INVALID_MOOD_SETTING;
}

// Get the mood setting with the given name
const MoodSetting& Mood::GetMoodSetting( const gpstring& moodName )
{
	// Find the mood
	MoodSettingMap::iterator i = m_moodSettingMap.find( moodName );
	if( i != m_moodSettingMap.end() )
	{
		return (*i).second;
	}

	return INVALID_MOOD_SETTING;
}

// Get the original mood setting with the given name
const MoodSetting& Mood::GetOriginalMoodSetting( const gpstring& moodName )
{
	// Find the mood
	MoodSettingMap::iterator i = m_originalMoods.find( moodName );
	if( i != m_originalMoods.end() )
	{
		return (*i).second;
	}

	return INVALID_MOOD_SETTING;
}

// Force a given mood setting
void Mood::SetMood( const gpstring& moodName, float transTime )
{
	// Find the requested mood
	MoodSettingMap::iterator i = m_moodSettingMap.find( moodName );
	if( i != m_moodSettingMap.end() )
	{
		// Check the sun setting
		if( m_pActiveSunMood	!= &(*i).second )
		{
			m_pActiveSunMood		= &(*i).second;
			m_bSunTransition		= true;
			m_sunTransTime			= transTime;

			// Clear the active GO if there is one
			m_activeSunGo			= GOID_INVALID;
		}

		// Check the fog setting
		if( m_pActiveFogMood	!= &(*i).second )
		{
			m_pActiveFogMood		= &(*i).second;
			m_bFogTransition		= true;
			m_fogTotalTransTime		= transTime;
			m_fogTransTime			= 0.0f;

			// Clear the active GO if there is one
			m_activeFogGo			= GOID_INVALID;

			// Save current fog params for transition
			m_fogTransColor			= gDefaultRapi.GetFogColor();
			m_fogTransNearDist		= gDefaultRapi.GetFogNearDist() - m_fogCameraOffset;
			m_fogTransFarDist		= gDefaultRapi.GetFogFarDist() - m_fogCameraOffset;
			m_fogTransDensity		= gDefaultRapi.GetFogDensity();
		}

		// Check the rain setting
		if( m_pActiveRainMood	!= &(*i).second )
		{
			m_pActiveRainMood		= &(*i).second;
			m_bRainTransition		= true;
			m_rainTotalTransTime	= transTime;
			m_rainTransTime			= 0.0f;

			// Clear the active GO if there is one
			m_activeRainGo			= GOID_INVALID;

			// Save current rain params for transition
			m_rainTransDensity		= gWeather.GetRain()->GetDensity();
		}

		// Check the snow setting
		if( m_pActiveSnowMood	!= &(*i).second )
		{
			m_pActiveSnowMood		= &(*i).second;
			m_bSnowTransition		= true;
			m_snowTotalTransTime	= transTime;
			m_snowTransTime			= 0.0f;

			// Clear the active GO if there is one
			m_activeSnowGo			= GOID_INVALID;

			// Save current snow params for transition
			m_snowTransDensity		= gWeather.GetSnow()->GetDensity();
		}

		// Check the wind setting
		if( m_pActiveWindMood	!= &(*i).second )
		{
			m_pActiveWindMood		= &(*i).second;
			m_bWindTransition		= true;
			m_windTotalTransTime	= transTime;
			m_windTransTime			= 0.0f;

			// Clear the active GO if there is one
			m_activeWindGo			= GOID_INVALID;

			// Save current wind params for transition
			m_windTransVelocity		= gWeather.GetWind()->GetVelocity();
			m_windTransDirection	= gWeather.GetWind()->GetAngularDirection();
		}

		// Check the music setting
		if( m_pActiveMusicMood	!= &(*i).second )
		{
			m_pActiveMusicMood	= &(*i).second;
			m_bMusicTransition	= true;
			m_musicTransTime	= transTime;

			// Clear the active GO if there is one
			m_activeMusicGo		= GOID_INVALID;
		}
	}
	else
	{
		gperrorf(( "Mood '%s' was requested, but does not exist!  Disregarded request.", moodName.c_str() ));
	}
}

// Set mood settings to the requested setting of the given GO
void Mood::SetActiveSunGo( Goid go, float transTime )
{
	// Find the GO
	GoMoodMap::iterator i = m_goMoodMap.find( go );
	if( i != m_goMoodMap.end() )
	{
		// Find the mood
		MoodSettingMap::iterator m = m_moodSettingMap.find( (*i).second.m_moodName );
		if( m != m_moodSettingMap.end() )
		{
			// Check the sun setting
			if( m_pActiveSunMood	!= &(*m).second )
			{
				m_pActiveSunMood	= &(*m).second;
				m_bSunTransition	= true;
				m_sunTransTime		= transTime;
			}

			// Set the active GO
			m_activeSunGo			= go;
		}
	}
}

void Mood::SetActiveFogGo( Goid go, float transTime )
{
	// Find the GO
	GoMoodMap::iterator i = m_goMoodMap.find( go );
	if( i != m_goMoodMap.end() )
	{
		// Find the mood
		MoodSettingMap::iterator m = m_moodSettingMap.find( (*i).second.m_moodName );
		if( m != m_moodSettingMap.end() )
		{
			// Check the fog setting
			if( m_pActiveFogMood	!= &(*m).second )
			{
				m_pActiveFogMood		= &(*m).second;
				m_bFogTransition		= true;
				m_fogTotalTransTime		= transTime;
				m_fogTransTime			= 0.0f;

				// Save current fog params for transition
				m_fogTransColor			= gDefaultRapi.GetFogColor();
				m_fogTransNearDist		= gDefaultRapi.GetFogNearDist() - m_fogCameraOffset;
				m_fogTransFarDist		= gDefaultRapi.GetFogFarDist() - m_fogCameraOffset;
				m_fogTransDensity		= gDefaultRapi.GetFogDensity();
			}

			// Set the active GO
			m_activeFogGo				= go;
		}
	}
}

void Mood::SetActiveRainGo( Goid go, float transTime )
{
	// Find the GO
	GoMoodMap::iterator i = m_goMoodMap.find( go );
	if( i != m_goMoodMap.end() )
	{
		// Find the mood
		MoodSettingMap::iterator m = m_moodSettingMap.find( (*i).second.m_moodName );
		if( m != m_moodSettingMap.end() )
		{
			// Check the rain setting
			if( m_pActiveRainMood	!= &(*m).second )
			{
				m_pActiveRainMood		= &(*m).second;
				m_bRainTransition		= true;
				m_rainTotalTransTime	= transTime;
				m_rainTransTime			= 0.0f;

				// Save current rain params for transition
				m_rainTransDensity		= gWeather.GetRain()->GetDensity();
			}

			// Set the active GO
			m_activeRainGo			= go;
		}
	}
}

void Mood::SetActiveSnowGo( Goid go, float transTime )
{
	// Find the GO
	GoMoodMap::iterator i = m_goMoodMap.find( go );
	if( i != m_goMoodMap.end() )
	{
		// Find the mood
		MoodSettingMap::iterator m = m_moodSettingMap.find( (*i).second.m_moodName );
		if( m != m_moodSettingMap.end() )
		{
			// Check the snow setting
			if( m_pActiveSnowMood	!= &(*m).second )
			{
				m_pActiveSnowMood		= &(*m).second;
				m_bSnowTransition		= true;
				m_snowTotalTransTime	= transTime;
				m_snowTransTime			= 0.0f;

				// Save current snow params for transition
				m_snowTransDensity		= gWeather.GetSnow()->GetDensity();
			}

			// Set the active GO
			m_activeSnowGo			= go;
		}
	}
}

void Mood::SetActiveWindGo( Goid go, float transTime )
{
	// Find the GO
	GoMoodMap::iterator i = m_goMoodMap.find( go );
	if( i != m_goMoodMap.end() )
	{
		// Find the mood
		MoodSettingMap::iterator m = m_moodSettingMap.find( (*i).second.m_moodName );
		if( m != m_moodSettingMap.end() )
		{
			// Check the wind setting
			if( m_pActiveWindMood	!= &(*m).second )
			{
				m_pActiveWindMood		= &(*m).second;
				m_bWindTransition		= true;
				m_windTotalTransTime	= transTime;
				m_windTransTime			= 0.0f;

				// Save current wind params for transition
				m_windTransVelocity		= gWeather.GetWind()->GetVelocity();
				m_windTransDirection	= gWeather.GetWind()->GetAngularDirection();
			}

			// Set the active GO
			m_activeWindGo				= go;
		}
	}
}

void Mood::SetActiveMusicGo( Goid go, float transTime )
{
	// Find the GO
	GoMoodMap::iterator i = m_goMoodMap.find( go );
	if( i != m_goMoodMap.end() )
	{
		// Find the mood
		MoodSettingMap::iterator m = m_moodSettingMap.find( (*i).second.m_moodName );
		if( m != m_moodSettingMap.end() )
		{
			// Check the music setting
			if( m_pActiveMusicMood	!= &(*m).second )
			{
				m_pActiveMusicMood	= &(*m).second;
				m_bMusicTransition	= true;
				m_musicTransTime	= transTime;
			}

			// Set the active GO
			m_activeMusicGo			= go;
		}
	}
}

void Mood::ForceUpdateFog( float transTime )
{
	if( !m_bFogTransition )
	{
		m_bFogTransition		= true;
		m_fogTotalTransTime		= transTime;
		m_fogTransTime			= 0.0f;

		// Save current fog params for transition
		m_fogTransColor			= gDefaultRapi.GetFogColor();
		m_fogTransNearDist		= gDefaultRapi.GetFogNearDist() - m_fogCameraOffset;
		m_fogTransFarDist		= gDefaultRapi.GetFogFarDist() - m_fogCameraOffset;
		m_fogTransDensity		= gDefaultRapi.GetFogDensity();
	}
}

void Mood::ForceUpdateRain( float transTime )
{
	if( !m_bRainTransition )
	{
		m_bRainTransition		= true;
		m_rainTotalTransTime	= transTime;
		m_rainTransTime			= 0.0f;

		// Save current rain params for transition
		m_rainTransDensity		= gWeather.GetRain()->GetDensity();
	}
}

void Mood::ForceUpdateSnow( float transTime )
{
	if( !m_bSnowTransition )
	{
		m_bSnowTransition		= true;
		m_snowTotalTransTime	= transTime;
		m_snowTransTime			= 0.0f;

		// Save current snow params for transition
		m_snowTransDensity		= gWeather.GetSnow()->GetDensity();
	}
}

void Mood::ForceUpdateWind( float transTime )
{
	if( !m_bWindTransition )
	{
		m_bWindTransition		= true;
		m_windTotalTransTime	= transTime;
		m_windTransTime			= 0.0f;

		// Save current wind params for transition
		m_windTransVelocity		= gWeather.GetWind()->GetVelocity();
		m_windTransDirection	= gWeather.GetWind()->GetAngularDirection();
	}
}

void Mood::ForceUpdateMusic( float transTime )
{
	if( !m_bMusicTransition )
	{
		m_bMusicTransition		= true;
		m_musicTransTime		= transTime;
	}
}

// Volume control
void Mood::FadeMoodVolume( float fadeTime, DWORD volume )
{
	// Setup our fade information
	m_resetFadeVolume			= gSoundManager.GetStreamVolume( GPGSound::STRM_MUSIC );
	if( m_resetFadeVolume > volume )
	{
		m_bFadeVolume			= true;
		m_fadeVolumeTime		= 0.0f;
		m_fadeVolumeTotalTime	= fadeTime;
		m_fadeVolume			= volume;
	}
}

void Mood::ResetMoodVolume( float fadeTime )
{
	// Setup for reset of music volume
	if( m_resetFadeVolume != gSoundManager.GetStreamVolume( GPGSound::STRM_MUSIC ) )
	{
		m_bFadeVolume			= true;
		m_fadeVolumeTime		= 0.0f;
		m_fadeVolumeTotalTime	= fadeTime;
		m_fadeVolume			= m_resetFadeVolume;
		m_resetFadeVolume		= gSoundManager.GetStreamVolume( GPGSound::STRM_MUSIC );
	}
}

// Fog control
void Mood::RemoveFogCorrection( float transTime )
{
	m_bFogCorrect				= true;
	m_bFogRemoval				= true;
	m_fogCorrectTime			= 0.0f;
	m_fogCorrectTotalTime		= transTime;
}

void Mood::RestoreFogCorrection( float transTime )
{
	m_bFogCorrect				= true;
	m_bFogRemoval				= false;
	m_fogCorrectTime			= 0.0f;
	m_fogCorrectTotalTime		= transTime;
}

// Stream stop callback
void Mood::StreamStopCallback( DWORD streamId )
{
	if( m_pActiveMusicMood )
	{
		// Try to find this stream
		if( m_activeAmbientTrack	== streamId )
		{
			m_bAmbientTrackDelay	= true;
			m_ambientTrackDelay		= m_pActiveMusicMood->m_ambientRepeatDelay;
			m_activeAmbientTrack	= GPGSound::INVALID_SOUND_ID;
		}

		if( m_activeStandardTrack	== streamId )
		{
			m_bStandardTrackDelay	= true;
			m_standardTrackDelay	= m_pActiveMusicMood->m_standardRepeatDelay;
			m_activeStandardTrack	= GPGSound::INVALID_SOUND_ID;
		}

		if( m_activeBattleTrack		== streamId )
		{
			m_bBattleTrackDelay		= true;
			m_battleTrackDelay		= m_pActiveMusicMood->m_battleRepeatDelay;
			m_activeBattleTrack		= GPGSound::INVALID_SOUND_ID;
		}
	}
}

// Blends values using a given modifier [0.0f - 1.0f]
float Mood::BlendValue( float orig, float dest, float blend )
{
	return( (orig * (1.0f - blend)) + (dest * blend) );
}

// Add a mood manually ( for editor moods )
void Mood::AddMood( FuelHandle & hMood )
{
	if ( !hMood )
	{
		gperror( "Invalid fuelhandle being used to add a mood." );
		return;
	}
	
	MoodSetting moodSetting;

	// Get the mood name and requested transition time
	moodSetting.m_moodName					= hMood->GetString( "mood_name" );
	moodSetting.m_transitionTime			= hMood->GetFloat( "transition_time" );
	moodSetting.m_bInterior					= hMood->GetBool( "interior" );

	// If it already exists, let's reload it.
	MoodSettingMap::iterator iFind = m_moodSettingMap.find( moodSetting.m_moodName );
	if( iFind != m_moodSettingMap.end() )
	{
		m_moodSettingMap.erase( iFind );
	}

	// Look for frustum
	FuelHandle hFrustum	= hMood->GetChildBlock( "frustum" );
	if( hFrustum.IsValid() )
	{
		moodSetting.m_bFrustumEnabled		= true;

		// Get the frustum settings
		moodSetting.m_frustumWidth			= hFrustum->GetFloat( "frustum_width" );
		moodSetting.m_frustumHeight			= hFrustum->GetFloat( "frustum_height" );
		moodSetting.m_frustumDepth			= hFrustum->GetFloat( "frustum_depth", moodSetting.m_frustumHeight );
	}

	// Look for fog
	FuelHandle hFog	= hMood->GetChildBlock( "fog" );
	if( hFog.IsValid() )
	{
		moodSetting.m_bFogEnabled			= true;

		// Get fog settings
		moodSetting.m_fogNearDist			= hFog->GetFloat( "fog_near_dist" );
		moodSetting.m_fogFarDist			= hFog->GetFloat( "fog_far_dist" );
		if( hFog->HasKey( "fog_lowdetail_near_dist" ) )
		{
			moodSetting.m_fogLowDetNearDist	= hFog->GetFloat( "fog_lowdetail_near_dist" );
		}
		else
		{
			moodSetting.m_fogLowDetNearDist	= moodSetting.m_fogNearDist;
		}
		if( hFog->HasKey( "fog_lowdetail_far_dist" ) )
		{
			moodSetting.m_fogLowDetFarDist	= hFog->GetFloat( "fog_lowdetail_far_dist" );
		}
		else
		{
			moodSetting.m_fogLowDetFarDist	= moodSetting.m_fogFarDist;
		}
		moodSetting.m_fogColor				= hFog->GetInt( "fog_color", 0xFF000000 );
		moodSetting.m_fogDensity			= hFog->GetFloat( "fog_density", 1.0f );
	}

	// Look for rain
	FuelHandle hRain = hMood->GetChildBlock( "rain" );
	if( hRain.IsValid() )
	{
		moodSetting.m_bRainEnabled			= true;
		
		// Get rain settings
		moodSetting.m_rainDensity			= hRain->GetFloat( "rain_density", 0.0f );
		moodSetting.m_bLightning			= hRain->GetBool( "lightning", false );
	}

	// Look for snow
	FuelHandle hSnow = hMood->GetChildBlock( "snow" );
	if( hSnow.IsValid() )
	{
		moodSetting.m_bSnowEnabled			= true;

		// Get snow settings
		moodSetting.m_snowDensity			= hSnow->GetFloat( "snow_density" );
	}

	// Look for wind
	FuelHandle hWind = hMood->GetChildBlock( "wind" );
	if( hWind.IsValid() )
	{
		moodSetting.m_bWindEnabled			= true;

		// Get wind settings
		moodSetting.m_windVelocity			= hWind->GetFloat( "wind_velocity" );
		moodSetting.m_windDirection			= hWind->GetFloat( "wind_direction" );
	}

	// Look for music
	FuelHandle hMusic = hMood->GetChildBlock( "music" );
	if( hMusic.IsValid() )
	{
		moodSetting.m_bMusicEnabled			= true;

		// Get music settings
		moodSetting.m_ambientTrack			= hMusic->GetString( "ambient_track" );
		moodSetting.m_ambientIntroDelay		= hMusic->GetFloat( "ambient_intro_delay" );
		moodSetting.m_ambientRepeatDelay	= hMusic->GetFloat( "ambient_repeat_delay" );

		moodSetting.m_standardTrack			= hMusic->GetString( "standard_track" );
		moodSetting.m_standardIntroDelay	= hMusic->GetFloat( "standard_intro_delay" );
		moodSetting.m_standardRepeatDelay	= hMusic->GetFloat( "standard_repeat_delay" );

		moodSetting.m_battleTrack			= hMusic->GetString( "battle_track" );
		moodSetting.m_battleIntroDelay		= hMusic->GetFloat( "battle_intro_delay" );
		moodSetting.m_battleRepeatDelay		= hMusic->GetFloat( "battle_repeat_delay" );

		FromString( hMusic->GetString( "room_type" ), moodSetting.m_moodRoomType );
	}

	// Look for sun
	FuelHandle hSun = hMood->GetChildBlock( "sun" );
	if( hSun.IsValid() )
	{
		moodSetting.m_bSunEnabled		= true;

		// Get sun settings
		FuelHandleList time_blocks = hSun->ListChildBlocks();
		for( FuelHandleList::const_iterator i = time_blocks.begin(); i != time_blocks.end(); ++i )
		{
			// Make a new block
			TimeLight tLight;

			// Get the hour and minute of this block
			sscanf( (*i)->GetName(), "%dh%dm", &tLight.m_Time.m_Hour, &tLight.m_Time.m_Minute );

			// Get the color
			tLight.m_Color	= 0xFFFFFFFF;
			(*i)->Get( "color", tLight.m_Color, false );

			// Put the new light on our list
			moodSetting.m_timeLightList.push_back( tLight );
		}
	}

	// Put this mood into our mapping
	if( m_moodSettingMap.find( moodSetting.m_moodName ) == m_moodSettingMap.end() )
	{
		m_moodSettingMap.insert( std::pair< gpstring, MoodSetting >( moodSetting.m_moodName, moodSetting ) );
		m_originalMoods.insert( std::pair< gpstring, MoodSetting >( moodSetting.m_moodName, moodSetting ) );
	}
	else
	{
		gperrorf(( "Duplicate mood '%s' found!  Discarding duplicate mood.", moodSetting.m_moodName.c_str() ));
	}
}


bool Mood::DoesMoodExist( const gpstring & sMoodName )
{
	if( m_moodSettingMap.find( sMoodName ) != m_moodSettingMap.end() )
	{
		return true;
	}

	return false;
}