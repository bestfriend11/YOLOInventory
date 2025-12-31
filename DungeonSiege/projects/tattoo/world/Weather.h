#pragma once
#ifndef _WEATHER_H_
#define _WEATHER_H_

/*=======================================================================================

  Weather

  purpose:

  author:	Rick Saenz, James Loe

  (C)opyright Gas Powered Games 1999-2001

---------------------------------------------------------------------------------------*/

#include "vector_3.h"


// Class responsible for maintaining and rendering rain
class Rain 
{
	struct RainDrop
	{
		vector_3	head_pos;
		vector_3	tail_pos;
		float		tail_length;
	};

	typedef		std::list< RainDrop >						DropList;
	typedef		std::map< siege::database_guid, DropList >	DropMap;

public:

	// Construction/Destruction
	Rain();
	~Rain();

	// Update the rain
	void		Update( float secondsElapsed );

	// Draw the rain
	void		Draw();

	// Get/Set raining state
	void		SetRaining( bool raining )					{ m_bRaining = raining; }
	bool		GetRaining()								{ return m_bRaining; }

	// Get/Set the current density
	void		SetDensity( float density )					{ m_density = min_t( m_maxDensity, density ); }
	float		GetDensity()								{ return m_density; }

	// Get/Set whether or not to have lightning
	void		SetLightning( bool lightning )				{ m_bLightning = lightning; }
	bool		GetLightning()								{ return m_bLightning; }

private:

// Configurable settings

	// Maximum rain drop settings
	int						m_maxDropCount;
	float					m_maxDensity;

	// Length of each rain drop
	float					m_minDropLength;
	float					m_maxDropLength;

	// Lightning time delays
	float					m_minLightningDelay;
	float					m_maxLightningDelay;
	float					m_minThunderDelay;
	float					m_maxThunderDelay;

	// Color
	DWORD					m_dropHeadColor;
	DWORD					m_dropTailColor;

	// Speed at which the rain falls
	float					m_dropSpeed;

	// Radius in which rain falls
	float					m_rainRadius;

	// Ceiling offset for rain spawn
	float					m_ceilingOffset;

	// Sound information
	gpstring				m_rainSound;
	gpstring				m_lightningSound;

// Status settings

	// Raining state
	bool					m_bRaining;

	// Current density
	float					m_density;

	// Lightning state
	bool					m_bLightning;

	// Current drop count
	int						m_dropCount;

// Misc. data

	// Lightning data
	float					m_lightningTime;
	bool					m_bThunder;
	float					m_thunderTime;
	siege::database_guid	m_flashLight;
	float					m_flashTime;

	// Handle to playing rain sample
	DWORD					m_rainSample;

	// Rain drop list
	DropMap					m_rainDropMap;

};


// Class responsible for maintaining and rendering snow
class Snow
{
	struct SnowFlake
	{
		vector_3	pos;
		vector_3	fall_vector;
	};

	typedef		std::list< SnowFlake >						FlakeList;
	typedef		std::map< siege::database_guid, FlakeList >	FlakeMap;

public:

	// Construction/Destruction
	Snow();
	~Snow();

	// Update the snow
	void		Update( float secondsElapsed );

	// Draw the snow
	void		Draw();

	// Get/Set snowing state
	void		SetSnowing( bool snowing )					{ m_bSnowing = snowing; }
	bool		GetSnowing()								{ return m_bSnowing; }

	// Get/Set the current density
	void		SetDensity( float density )					{ m_density = min_t( m_maxDensity, density ); }
	float		GetDensity()								{ return m_density; }

private:

// Configurable settings

	// Maximum snow flake settings
	int			m_maxFlakeCount;
	float		m_maxDensity;

	// Size of each flake
	float		m_flakeSize;

	// Maximum perturb for snow direction
	float		m_snowPerturb;

	// Speed at which the rain falls
	float		m_minFlakeSpeed;
	float		m_maxFlakeSpeed;

	// Radius in which rain falls
	float		m_snowRadius;

	// Ceiling offset for snow spawn
	float		m_ceilingOffset;

	// Texture used for the snow flakes
	DWORD		m_flakeTexture;

// Status settings

	// Snowing state
	bool		m_bSnowing;

	// Current density
	float		m_density;

	// Current flake count
	int			m_flakeCount;


// Misc. data

	// Snow flake map
	FlakeMap	m_snowFlakeMap;

};


// Class responsible for maintaining wind
class Wind
{
public:

	// Construction/Destruction
	Wind();
	~Wind();

	// Update the snow
	void					Update( float secondsElapsed );

	// Get/Set snowing state
	void					SetBlowing( bool blowing )				{ m_bBlowing = blowing; }
	bool					GetBlowing()							{ return m_bBlowing; }

	// Get/Set the current velocity
	void					SetVelocity( float velocity )			{ m_velocity = min_t( m_maxVelocity, velocity ); }
	float					GetVelocity()							{ return m_velocity; }

	// Get/Set the angular direction
	void					SetAngularDirection( float direction )	{ m_windDirection = direction; }
	float					GetAngularDirection()					{ return m_windDirection; }

	// Get the current direction
	vector_3				GetWorldDirection();
	vector_3				GetNodeDirection( siege::database_guid );

private:

// Configurable settings

	// Maximum wind velocity
	float					m_maxVelocity;

	// Wind sound
	gpstring				m_windSound;

// Status settings

	// Blowing state
	bool					m_bBlowing;

	// Current wind velocity
	float					m_velocity;

	// Current wind direction (in radian angle off of North)
	float					m_windDirection;

// Misc. data

	// Wind sample
	DWORD					m_windSample;

};


// Class for maintaining all weather
class Weather : public Singleton < Weather >
{
public:

	// Construction/Destruction
	Weather();
	~Weather();

	// Update the weather
	void				Update( float secondsElapsed );

	// Draw the weather
	void				Draw();

	// Set/Get the current camera orientation
	void				SetCameraOrientation( const matrix_3x3& orient )	{ m_cameraOrient = orient; }
	const matrix_3x3&	GetCameraOrientation()								{ return m_cameraOrient; }

	// Set/Get the enabled state
	void				SetEnabled( bool bEnabled );
	bool				GetEnabled()										{ return m_bEnabled; }

	// Access to weather components
	Rain*				GetRain()											{ return m_pRain; }
	Snow*				GetSnow()											{ return m_pSnow; }
	Wind*				GetWind()											{ return m_pWind; }

private:
	
	// Enabled state
	bool				m_bEnabled;

	// Weather settings
	Rain*				m_pRain;
	Snow*				m_pSnow;
	Wind*				m_pWind;

	// Camera orientation
	matrix_3x3			m_cameraOrient;

	FUBI_SINGLETON_CLASS( Weather, "Dungeon Siege weather system." );
};

#define gWeather Weather::GetSingleton()

#endif	// _WEATHER_H_
