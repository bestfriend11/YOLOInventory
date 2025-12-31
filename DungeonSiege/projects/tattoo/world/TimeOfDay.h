#pragma once
#ifndef _TIME_OF_DAY_H_
#define _TIME_OF_DAY_H_

/******************************************************************************************
**
**									TimeOfDay
**
**		Class responsible for maintaining the current time of the day in engine and
**		notifying those things withtin the game of the changes that need to occur as
**		a result of the time (i.e. what color the sun is ).
**
**		Author:		James Loe
**		Date:		08/11/00
**
******************************************************************************************/

const int	MINUTES_IN_AN_HOUR	= 60;
const int	HOURS_IN_A_DAY		= 24;


class TimeInfo
{
public:

	TimeInfo()
		: m_Hour( 0 )
		, m_Minute( 0 ){}

	bool	Xfer( FuBi::PersistContext &persist );

	// Current time
	int	m_Hour;
	int	m_Minute;

	// Get the closest total hour distance (in range [12 - -12])
	int GetClosestHourDifference( const TimeInfo& time )
	{
		int hour_diff	= time.m_Hour - m_Hour;
		if( abs( hour_diff ) > (HOURS_IN_A_DAY/2) )
		{
			if( hour_diff > 0 )		{ hour_diff -= HOURS_IN_A_DAY; }
			else if( hour_diff < 0 ){ hour_diff += HOURS_IN_A_DAY; }
		}
		return hour_diff;
	}

	// Get time difference
	int GetMinuteDifference( const TimeInfo& time )
	{
		return (GetClosestHourDifference( time ) * MINUTES_IN_AN_HOUR) + (time.m_Minute - m_Minute);
	}
};

struct TimeLight
{
	bool	Xfer( FuBi::PersistContext &persist );

	// Time described by this structure
	TimeInfo		m_Time;

	// Color of the light at that time
	DWORD			m_Color;
};

typedef std::list< TimeLight >		TimeLightList;

class TimeOfDay : public Singleton <TimeOfDay>
{
public:

	// Construction and destruction
	TimeOfDay( const char* pTimeFuelAddress );
	~TimeOfDay();

	bool	Xfer( FuBi::PersistContext &persist );

	// Update with elapsed game time
	void					Update( const float deltaT );

	// Information about the hour/minute
FEX	unsigned int			GetHour() const							{ return m_Time.m_Hour; }
FEX	unsigned int			GetMinute() const						{ return m_Time.m_Minute; }

	// Information about the time
	TimeInfo				GetTime() const							{ return m_Time; }
	void					SetTime( const TimeInfo& time );
FEX	void					RCSetTime( DWORD hour, DWORD minute );

	// Information about the default time light list
	const TimeLightList&	GetDefaultTimeLightList() const			{ return m_DefaultLightList; }

	// Information about the current time light list
	const TimeLightList&	GetTimeLightList() const				{ return m_TimeLightList; }
	void					SetTimeLightList( const TimeLightList& tList, const float smoothT );

	// Get the color of the sun at the requested time
	DWORD					GetTimeLightColor( int hour, int minute );

	// Get the color based on the current time
	DWORD					GetCurrentTimeLightColor();

private:

	// Store the minute as a float for accumulation purposes
	float					m_RealMinutesPerWorldHour;
	float					m_Minute;

	// Store the combined time as an info structure
	TimeInfo				m_Time;

	// Table that stores light information
	TimeLightList			m_TimeLightList;
	TimeLightList			m_DefaultLightList;

	// Update light state
	bool					m_bUpdateLight;

	// Transition information
	bool					m_bInTransition;
	float					m_CurrentTransitionTime;
	float					m_TotalTransitionTime;
	DWORD					m_PreviousColor;

	FUBI_SINGLETON_CLASS( TimeOfDay, "Time of day" );
};


#define gTimeOfDay TimeOfDay::GetSingleton()

#endif
