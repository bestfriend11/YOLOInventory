#pragma once
#ifndef __MOOD_H
#define __MOOD_H

//////////////////////////////////////////////////////////////////////////////
//
// File     :  mood.h (Tattoo:World)
// Author(s):  Rick Saenz, James Loe
//
// Summary  :  Tracks mood settings and the GO's that request them.
//
// Copyright © 1999-2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "timeofday.h"

// Enumeration that describes different mood room types, mirrors the eRoomType
// enumeration in the sound system but is Fubi exported for use in skrit.
enum eMoodRoomType
{
	RT_INVALID			= -1,
	SET_BEGIN_ENUM( RT_, 0 ),

	RT_GENERIC			= 0,
	RT_PADDEDCELL		= 1,
	RT_ROOM				= 2,
	RT_BATHROOM			= 3,
	RT_LIVINGROOM		= 4,
	RT_STONEROOM		= 5,
	RT_AUDITORIUM		= 6,
	RT_CONCERTHALL		= 7,
	RT_CAVE				= 8,
	RT_ARENA			= 9,
	RT_HANGAR			= 10,
	RT_CARPETEDHALLWAY	= 11,
	RT_HALLWAY			= 12,
	RT_STONECORRIDOR	= 13,
	RT_ALLEY			= 14,
	RT_FOREST			= 15,
	RT_CITY				= 16,
	RT_MOUNTAINS		= 17,
	RT_QUARRY			= 18,
	RT_PLAIN			= 19,
	RT_PARKINGLOT		= 20,
	RT_SEWERPIPE		= 21,
	RT_UNDERWATER		= 22,
	RT_DRUGGED			= 23,
	RT_DIZZY			= 24,
	RT_PSYCHOTIC		= 25,

	SET_END_ENUM( RT_ ),
	RT_DWORDALIGN		= 0x7FFFFFFF,
};

FUBI_EXPORT const char* ToString  ( eMoodRoomType val );
FUBI_EXPORT bool        FromString( const char* str, eMoodRoomType& val );

// Struct that identifies a single mood
struct MoodSetting
{
	FUBI_POD_CLASS( MoodSetting );

	FUBI_VARIABLE( gpstring,		m_, moodName,			"Name of this mood" );
	FUBI_VARIABLE( float,			m_, transitionTime,		"Requested transition time" );
	FUBI_VARIABLE( bool,			m_, bInterior,			"Is this mood for interiors" );

	// Frustum setting
	FUBI_VARIABLE( bool,			m_, bFrustumEnabled,	"Are the custom frustum dimensions enabled" );
	FUBI_VARIABLE( float,			m_, frustumWidth,		"Width of the frustum, in meters" );
	FUBI_VARIABLE( float,			m_, frustumHeight,		"Height of the frustum, in meters" );
	FUBI_VARIABLE( float,			m_, frustumDepth,		"Depth of the frustum, in meters" );

	// Fog setting
	FUBI_VARIABLE( bool,			m_, bFogEnabled,		"Is the fog enabled" );
	FUBI_VARIABLE( float,			m_, fogNearDist,		"Fog near distance" );
	FUBI_VARIABLE( float,			m_, fogFarDist,			"Fog far distance" );
	FUBI_VARIABLE( float,			m_, fogLowDetNearDist,	"Low detail fog near distance" );
	FUBI_VARIABLE( float,			m_, fogLowDetFarDist,	"Low detail fog far distance" );
	FUBI_VARIABLE( DWORD,			m_, fogColor,			"Fog color" );
	FUBI_VARIABLE( float,			m_, fogDensity,			"Fog density" );

	// Rain setting
	FUBI_VARIABLE( bool,			m_, bRainEnabled,		"Is the rain enabled" );
	FUBI_VARIABLE( float,			m_, rainDensity,		"Density of the rain" );
	FUBI_VARIABLE( float,			m_, rainLowDetDensity,	"Low detail density of the rain" );
	FUBI_VARIABLE( bool,			m_, bLightning,			"Lightning state" );

	// Snow setting
	FUBI_VARIABLE( bool,			m_, bSnowEnabled,		"Is the snow enabled" );
	FUBI_VARIABLE( float,			m_, snowDensity,		"Density of the snow" );
	FUBI_VARIABLE( float,			m_, snowLowDetDensity,	"Low detail density of the snow" );

	// Wind setting
	FUBI_VARIABLE( bool,			m_, bWindEnabled,		"Is the wind enabled" );
	FUBI_VARIABLE( float,			m_, windVelocity,		"Velocity of the wind" );
	FUBI_VARIABLE( float,			m_, windDirection,		"Direction of the wind in radians off of North" );

	// Music setting
	FUBI_VARIABLE( bool,			m_, bMusicEnabled,		"Is the music enabled" );
	FUBI_VARIABLE( gpstring,		m_, ambientTrack,		"Ambient sound track" );
	FUBI_VARIABLE( gpstring,		m_, standardTrack,		"Standard music track" );
	FUBI_VARIABLE( gpstring,		m_, battleTrack,		"Battle music track" );
	FUBI_VARIABLE( float,			m_, ambientIntroDelay,	"Ambient track startup delay, in seconds" );
	FUBI_VARIABLE( float,			m_, ambientRepeatDelay,	"Ambient track repeat delay, in seconds" );
	FUBI_VARIABLE( float,			m_, standardIntroDelay,	"Standard track startup delay, in seconds" );
	FUBI_VARIABLE( float,			m_, standardRepeatDelay,"Standard track repeat delay, in seconds" );
	FUBI_VARIABLE( float,			m_, battleIntroDelay,	"Battle track startup delay, in seconds" );
	FUBI_VARIABLE( float,			m_, battleRepeatDelay,	"Battle track repeat delay, in seconds" );
	FUBI_VARIABLE( eMoodRoomType,	m_, moodRoomType,		"Room type for this mood" );

	// Sun setting
	bool							m_bSunEnabled;
	TimeLightList					m_timeLightList;

	// Construction
	MoodSetting();
};

struct GoMoodSetting
{
	// Active mood for this GO
	gpstring	m_moodName;

	// Frustum transition information
	bool		m_bFrustumTransition;
	float		m_frustumTotalTransTime;
	float		m_frustumTransTime;
	float		m_frustumTransWidth;
	float		m_frustumTransHeight;
	float		m_frustumTransDepth;
};


// Class that maintains a listing of mood settings and tracks go's that have requested moods
class Mood : public Singleton< Mood >
{
	// Typedefs
	typedef	std::map< gpstring, MoodSetting, istring_less >	MoodSettingMap;
	typedef std::map< Goid, GoMoodSetting >					GoMoodMap;

public:

	// Construction and destruction
	Mood();
	~Mood();

	// Initialize the moods
	void							InitMoods( bool bReload );
	
	// Persist the moods
	bool							Xfer( FuBi::PersistContext &persist );

	// Enabled state
	void							SetEnabled( bool bEnabled )						{ m_bEnabled = bEnabled; }
	bool							GetEnabled()									{ return m_bEnabled; }

	// Update the mood
	void							Update( float secondsElapsed );

	// Shutdown
	void							Shutdown();

	// Register a mood request from a GO
	void							RegisterMoodRequest( const Goid go, const gpstring& moodName );

	// Unregister a mood request
	void							UnregisterMoodRequest( const Goid go );

	// Get the requested mood for a given GO
	FUBI_EXPORT const gpstring&		GetGoRequestedMood( Goid go );
	FUBI_EXPORT const MoodSetting&	GetGoMoodSetting( Goid go );
	FUBI_EXPORT const MoodSetting&	GetMoodSetting( const gpstring& moodName );
	FUBI_EXPORT const MoodSetting&	GetOriginalMoodSetting( const gpstring& moodName );

	// Force a given mood setting
	FUBI_EXPORT void				SetMood( const gpstring& moodName, float transTime );

	// Set mood settings to the requested setting of the given GO
	FUBI_EXPORT void				SetActiveSunGo( Goid go, float transTime );
	FUBI_EXPORT Goid				GetActiveSunGo()								{ return m_activeSunGo; }

	FUBI_EXPORT void				SetActiveFogGo( Goid go, float transTime );
	FUBI_EXPORT Goid				GetActiveFogGo()								{ return m_activeFogGo; }

	FUBI_EXPORT void				SetActiveRainGo( Goid go, float transTime );
	FUBI_EXPORT Goid				GetActiveRainGo()								{ return m_activeRainGo; }

	FUBI_EXPORT void				SetActiveSnowGo( Goid go, float transTime );
	FUBI_EXPORT Goid				GetActiveSnowGo()								{ return m_activeSnowGo; }

	FUBI_EXPORT void				SetActiveWindGo( Goid go, float transTime );
	FUBI_EXPORT Goid				GetActiveWindGo()								{ return m_activeWindGo; }

	FUBI_EXPORT void				SetActiveMusicGo( Goid go, float transTime );
	FUBI_EXPORT Goid				GetActiveMusicGo()								{ return m_activeMusicGo; }

	// Force updates of mood settings
	FUBI_EXPORT void				ForceUpdateFog( float transTime );
	FUBI_EXPORT void				ForceUpdateRain( float transTime );
	FUBI_EXPORT void				ForceUpdateSnow( float transTime );
	FUBI_EXPORT void				ForceUpdateWind( float transTime );
	FUBI_EXPORT void				ForceUpdateMusic( float transTime );

	// Ambient track control
	FUBI_EXPORT void				PlayAmbientTrack( float fadeTime )				{ m_bAmbientTrack = true; m_ambientFadeTime = fadeTime; }
	FUBI_EXPORT bool				IsAmbientTrackPlaying()							{ return m_bAmbientTrack; }
	FUBI_EXPORT void				StopAmbientTrack( float fadeTime )				{ m_bAmbientTrack = false; m_ambientFadeTime = fadeTime; }

	FUBI_EXPORT void				PlayStandardTrack( float fadeTime )				{ m_bStandardTrack = true; m_standardFadeTime = fadeTime; }
	FUBI_EXPORT bool				IsStandardTrackPlaying()						{ return m_bStandardTrack; }
	FUBI_EXPORT void				StopStandardTrack( float fadeTime )				{ m_bStandardTrack = false; m_standardFadeTime = fadeTime; }

	FUBI_EXPORT void				PlayBattleTrack( float fadeTime )				{ m_bBattleTrack = true; m_battleFadeTime = fadeTime; }
	FUBI_EXPORT bool				IsBattleTrackPlaying()							{ return m_bBattleTrack; }
	FUBI_EXPORT void				StopBattleTrack( float fadeTime )				{ m_bBattleTrack = false; m_battleFadeTime = fadeTime; }

	// Volume control
	FUBI_EXPORT void				FadeMoodVolume( float fadeTime, DWORD volume );
	FUBI_EXPORT void				ResetMoodVolume( float fadeTime );

	// Fog control
	FUBI_EXPORT void				RemoveFogCorrection( float transTime );
	FUBI_EXPORT void				RestoreFogCorrection( float transTime );

	// Editor helper functions
	void AddMood( FuelHandle & hMood );
	bool DoesMoodExist( const gpstring & sMoodName );

private:

	// Stream stop callback
	void							StreamStopCallback( DWORD streamId );

	// Blends values using a given modifier [0.0f - 1.0f]
	inline float					BlendValue( float orig, float dest, float blend );

	// Enabled state
	bool							m_bEnabled;

	// Map of available moods
	MoodSettingMap					m_moodSettingMap;
	MoodSettingMap					m_originalMoods;

	// Map of GO's that have requested moods to the mood names they have requested
	GoMoodMap						m_goMoodMap;

	// Current camera offset used for fog correction
	float							m_fogCameraOffset;

	// Active setting GO's
	Goid							m_activeSunGo;
	Goid							m_activeFogGo;
	Goid							m_activeRainGo;
	Goid							m_activeSnowGo;
	Goid							m_activeWindGo;
	Goid							m_activeMusicGo;

	// Active settings
	MoodSetting*					m_pActiveSunMood;
	MoodSetting*					m_pActiveFogMood;
	MoodSetting*					m_pActiveRainMood;
	MoodSetting*					m_pActiveSnowMood;
	MoodSetting*					m_pActiveWindMood;
	MoodSetting*					m_pActiveMusicMood;

	// Sun transition information
	bool							m_bSunTransition;
	float							m_sunTransTime;

	// Fog transition information
	bool							m_bFogTransition;
	float							m_fogTotalTransTime;
	float							m_fogTransTime;
	float							m_fogTransNearDist;
	float							m_fogTransFarDist;
	DWORD							m_fogTransColor;
	float							m_fogTransDensity;

	// Rain transition information
	bool							m_bRainTransition;
	float							m_rainTotalTransTime;
	float							m_rainTransTime;
	float							m_rainTransDensity;

	// Snow transition information
	bool							m_bSnowTransition;
	float							m_snowTotalTransTime;
	float							m_snowTransTime;
	float							m_snowTransDensity;

	// Wind transition information
	bool							m_bWindTransition;
	float							m_windTotalTransTime;
	float							m_windTransTime;
	float							m_windTransVelocity;
	float							m_windTransDirection;

	// Music transition information
	bool							m_bMusicTransition;
	float							m_musicTransTime;

	// Active music handles
	bool							m_bAmbientTrack;
	float							m_ambientFadeTime;
	DWORD							m_activeAmbientTrack;
	gpstring						m_activeAmbientTrackName;
	bool							m_bAmbientTrackDelay;
	float							m_ambientTrackDelay;

	bool							m_bStandardTrack;
	float							m_standardFadeTime;
	DWORD							m_activeStandardTrack;
	gpstring						m_activeStandardTrackName;
	bool							m_bStandardTrackDelay;
	float							m_standardTrackDelay;

	bool							m_bBattleTrack;
	float							m_battleFadeTime;
	DWORD							m_activeBattleTrack;
	gpstring						m_activeBattleTrackName;
	bool							m_bBattleTrackDelay;
	float							m_battleTrackDelay;

	// Volume information
	bool							m_bFadeVolume;
	float							m_fadeVolumeTime;
	float							m_fadeVolumeTotalTime;
	DWORD							m_fadeVolume;
	DWORD							m_resetFadeVolume;

	// Fog information
	bool							m_bFogCorrect;
	bool							m_bFogRemoval;
	float							m_fogCorrectTime;
	float							m_fogCorrectTotalTime;

	// Mood manager
	Skrit::HObject					m_moodManager;

	FUBI_SINGLETON_CLASS( Mood, "Dungeon Siege mood system." );
};

// Conflict management
SKRIT_IMPORT void ManageMoods( Skrit::HObject skrit, const char* funcName, GoidColl&, bool, float );

#define gMood Mood::GetSingleton()


#endif	//__MOOD_H
