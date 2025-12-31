#pragma once
/*************************************************************************************
**
**								WorldSound
**
**		This class is responsible for managing 3D sounds in the game by
**		updating their positions each frame, as well as updating their
**		play state when frustums switch.
**
**		Author:		James Loe
**		Date:		5/28/01
**
*************************************************************************************/
#ifndef _WORLD_SOUND_
#define _WORLD_SOUND_

#include "gps_types.h"

class WorldSound : public Singleton <WorldSound>
{
	struct ManagedSound
	{
		// Identifier of this sample
		DWORD					soundId;

		// Information about this sample
		gpstring				soundFile;
		SiegePos				soundPos;
		bool					bLoop;
		float					minDist;
		float					maxDist;
		GPGSound::eSampleType	type;

		// State information
		bool					bPaused;
		DWORD					currentPos;
	};

	typedef std::list< ManagedSound >	ManagedSampleList;

public:

	// Construction/Destruction
	WorldSound();
	~WorldSound();

	// Enable wrapper for the sound system.  Properly handles stopping and restarting all
	// managed samples when the state of the sound system changes.
	bool	Enable( bool bEnable, bool bEnableEAX );

	// Update all currently managed samples
	void	Update( bool bPaused = false );

	// Play a managed 3D sound
	DWORD	PlaySample( const gpstring& soundFile, const SiegePos& position, bool bLoop = false,
					    const float min_dist = -1.0f, const float max_dist = -1.0f,
					    GPGSound::eSampleType type = GPGSound::SND_EFFECT_NORM );

	// Sample callbacks
	void	SetSampleStopCallback( DWORD sampleID, GPGSound::SampleStopCallback callback );
	void	ClearSampleStopCallback( DWORD sampleID );

	// Cleanup
	void	Clear()										{ m_managedSampleList.clear(); }

	// Default falloff distances
	float	GetDefaultMinDist()							{ return m_defaultMinDist; }
	void	SetDefaultMinDist( float defaultMinDist )	{ m_defaultMinDist = defaultMinDist; }

	float	GetDefaultMaxDist()							{ return m_defaultMaxDist; }
	void	SetDefaultMaxDist( float defaultMaxDist )	{ m_defaultMaxDist = defaultMaxDist; }

private:

	// Sample stop callback function
	void SampleStopCallback( DWORD sampleId );

	// Collection of currently managed samples
	ManagedSampleList				m_managedSampleList;

	// Map of sample callbacks
	GPGSound::StopSampleCallbackMap	m_sampleCallbackMap;

	// Default distances
	float							m_defaultMinDist;
	float							m_defaultMaxDist;

};

#define gWorldSound WorldSound::GetSingleton()

#endif // _WORLD_SOUND_