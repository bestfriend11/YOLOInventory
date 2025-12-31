#pragma once
/*=======================================================================================

  WorldTime

  purpose:	A place to keep all the world time stuff.

  author:	Bartosz Kijanka

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
#ifndef __WORLDTIME_H
#define __WORLDTIME_H


#include "godefs.h"

class AverageAccumulator;
// $ implementation in World.cpp


class WorldTime : public Singleton <WorldTime>
{

public:

	WorldTime();
	~WorldTime();

	bool Xfer( FuBi::PersistContext& persist );

	void Init();
	void ClearTime();

	FUBI_EXPORT double GetTime() const				{  return ( m_Time ); }
	FUBI_MEMBER_DOC(   GetTime, "",
					  "Get the current world time in seconds." );

	FUBI_EXPORT double GetRealTime() const          {  return ( ::GetGlobalSeconds() );  }
	FUBI_MEMBER_DOC(   GetRealTime, "",
					  "Get the 'real' time of the system in seconds." );

	FUBI_EXPORT float GetSecondsElapsed() const 	{  return ( m_SecondsElapsed );  }
	FUBI_MEMBER_DOC(   GetSecondsElapsed, "",
					  "Get the amount of world time elapsed since last simulation iteration." );

	FUBI_EXPORT unsigned int GetSimCount() const 	{  return ( m_SimCount );  }
	FUBI_MEMBER_DOC(         GetSimCount, "",
							"Get the number of simulation iterations." );

	void Update( float SecondsElapsedSinceLastSim );

	void ForceTime( double time );
	//FUBI_EXPORT void RSPause( bool reqPauseState );
	float GetLocalRelativeServerTimeDrift();


private:

	//FUBI_EXPORT void RCPause( bool reqPauseState, double worldTime );
	FUBI_EXPORT void RCSetServerTime( double time );

	double m_TimeBroadcastTime;

	my AverageAccumulator * m_pTimeDrift;


	//-------------------------------------------------
	// current game time
	double m_Time;
	// how much time passed since last simulation/logic pass done
	float m_SecondsElapsed;
	// how many simulation/logic passes were made during the m_Time
	unsigned int m_SimCount;

	FUBI_SINGLETON_CLASS( WorldTime, "This class represents world time and other time-related simulation states." );
};

#define gWorldTime WorldTime::GetSingleton()



#endif
