/******************************************************************************************
**
**									TimeOfDay
**	
**			See .h file for details.
**
******************************************************************************************/

#include "precomp_world.h"
#include "timeofday.h"
#include "siege_engine.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "FuBiTraitsimpl.h"


bool
TimeInfo::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "m_Hour",		m_Hour		);
	persist.Xfer( "m_Minute",	m_Minute	);

	return true;
}


bool
TimeLight::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer	( "m_Time",		m_Time		);
	persist.XferHex	( "m_Color",	m_Color		);

	return true;
}


// Construction
TimeOfDay::TimeOfDay( const char* pTimeFuelAddress )
	: m_Minute( 0.0f )
	, m_bInTransition( false )
	, m_CurrentTransitionTime( 0.0f )
	, m_TotalTransitionTime( 0.0f )
	, m_PreviousColor( 0 )
	, m_bUpdateLight( false )
{
	// Try to load our time info from a fuel block
	FastFuelHandle hTime( pTimeFuelAddress );
	if( hTime.IsValid() )
	{
		// Get the number of real minutes per world hour
		hTime.Get( "RealMinutesPerWorldHour", m_RealMinutesPerWorldHour, false );

		// Get individual time blocks
		FastFuelHandleColl time_blocks;
		hTime.ListChildren( time_blocks );
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
			m_DefaultLightList.push_back( tLight );
		}
	}

	// Set the time light list to default
	m_TimeLightList	= m_DefaultLightList;
}

// Destruction
TimeOfDay::~TimeOfDay()
{
}


FUBI_DECLARE_SELF_TRAITS( TimeInfo );
FUBI_DECLARE_SELF_TRAITS( TimeLight );

bool
TimeOfDay::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer	( "m_RealMinutesPerWorldHour",		m_RealMinutesPerWorldHour	);
	persist.Xfer	( "m_Minute",						m_Minute					);

	persist.Xfer	( "m_Time",							m_Time						);

	persist.Xfer	( "m_bInTransition",				m_bInTransition				);
	persist.Xfer	( "m_CurrentTransitionTime",		m_CurrentTransitionTime		);
	persist.Xfer	( "m_TotalTransitionTime",			m_TotalTransitionTime		);
	persist.XferHex	( "m_PreviousColor",				m_PreviousColor				);

	if( persist.IsRestoring() )
	{
		m_bUpdateLight = true;
	}

	return true;
}


// Update
void TimeOfDay::Update( const float deltaT )
{
	if( gServer.IsLocal() )
	{
		// Update the minute
		m_Minute			+= deltaT;

		// If the minute actually changed, do a full update
		if( m_Minute >= m_RealMinutesPerWorldHour )
		{
			// Update the minute
			int minutes_changed	= FTOL( m_Minute / m_RealMinutesPerWorldHour );

			m_Time.m_Minute	+= minutes_changed;
			m_Minute		-= minutes_changed * m_RealMinutesPerWorldHour;

			// Update the time
			if( m_Time.m_Minute >= MINUTES_IN_AN_HOUR )
			{
				int hours_passed	= m_Time.m_Minute / MINUTES_IN_AN_HOUR;
				m_Time.m_Minute		-= hours_passed * MINUTES_IN_AN_HOUR;
				m_Time.m_Hour		+= hours_passed;

				if( m_Time.m_Hour >= HOURS_IN_A_DAY )
				{
					m_Time.m_Hour	%= HOURS_IN_A_DAY;
				}
			}

			RCSetTime( m_Time.m_Hour, m_Time.m_Minute );
		}
	}

	if( m_bUpdateLight )
	{
		// Update light
		if( m_bInTransition )
		{
			m_CurrentTransitionTime += deltaT;
			if( m_CurrentTransitionTime >= m_TotalTransitionTime )
			{
				m_bInTransition	= false;
				m_bUpdateLight	= false;
				gSiegeEngine.LightDatabase().SetTimedLightColor( GetCurrentTimeLightColor() );
			}
			else
			{
				DWORD currentColor	= InterpolateColor( m_PreviousColor, GetCurrentTimeLightColor(), 1.0f - (m_CurrentTransitionTime / m_TotalTransitionTime) );
				gSiegeEngine.LightDatabase().SetTimedLightColor( currentColor );
			}
		}
		else
		{
			gSiegeEngine.LightDatabase().SetTimedLightColor( GetCurrentTimeLightColor() );

			// Reset update notification
			m_bUpdateLight	= false;
		}
	}
}

// Get the color of the sun at the requested time
DWORD TimeOfDay::GetTimeLightColor( int hour, int minute )
{
	TimeInfo		tTime;
	tTime.m_Hour	= hour;
	tTime.m_Minute	= minute;

	// Need two times to interpolate between
	TimeLight		firstLight;
	TimeLight		secondLight;

	int				firstLightDiff	= INT_MIN;
	int				secondLightDiff	= INT_MAX;

	for( TimeLightList::iterator i = m_TimeLightList.begin(); i != m_TimeLightList.end(); ++i )
	{
		int	diff = tTime.GetMinuteDifference( (*i).m_Time );
		if( diff == 0 )
		{
			return (*i).m_Color;
		}
		else if( diff < 0 )
		{
			// Modify first light
			if( diff > firstLightDiff )
			{
				firstLight		= (*i);
				firstLightDiff	= diff;
			}
		}
		else if( diff > 0 )
		{
			// Modify second light
			if( diff < secondLightDiff )
			{
				secondLight		= (*i);
				secondLightDiff	= diff;
			}
		}
	}

	// Make sure we got both lights properly
	if( firstLightDiff == INT_MIN || secondLightDiff == INT_MAX )
	{
		gperror( "Invalid time light color table!" );
		return 0xFFFFFFFF;
	}

	// Figure out how much we need to interpolate
	firstLightDiff			= abs( firstLightDiff );
	float percentageInterp	= 1.0f - ((float)firstLightDiff / (float)(firstLightDiff + secondLightDiff));

	// Return our color
	return( InterpolateColor( firstLight.m_Color, secondLight.m_Color, percentageInterp ) );
}

// Set the time
void TimeOfDay::SetTime( const TimeInfo& time )
{
	// Store the time
	m_Time					= time;
	m_bUpdateLight			= true;
}

// Set the time
void TimeOfDay::RCSetTime( DWORD hour, DWORD minute )
{
	FUBI_RPC_CALL( RCSetTime, RPC_TO_ALL );

	// Store the time
	m_Time.m_Hour			= hour;
	m_Time.m_Minute			= minute;
	m_bUpdateLight			= true;
}

// Set the time light list, and interpolate over the given time
void TimeOfDay::SetTimeLightList( const TimeLightList& tList, const float smoothT )
{
	// Setup transition information
	m_bInTransition			= true;
	m_bUpdateLight			= true;
	m_CurrentTransitionTime	= 0.0f;
	m_TotalTransitionTime	= smoothT;
	m_PreviousColor			= GetCurrentTimeLightColor();

	// Set the new list
	m_TimeLightList			= tList;
}

// Get the color based on the current time
DWORD TimeOfDay::GetCurrentTimeLightColor()
{
	return GetTimeLightColor( m_Time.m_Hour, m_Time.m_Minute );
}
