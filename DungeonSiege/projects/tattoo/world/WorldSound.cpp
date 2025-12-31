/*************************************************************************************
**
**								WorldSound
**
**		See .h file for details
**
*************************************************************************************/

#include "precomp_world.h"
#include "gps_manager.h"
#include "WorldSound.h"


// Construction
WorldSound::WorldSound()
{
	// Lookup default falloffs
	FastFuelHandle hDefaults( "config:sound_settings" );
	if( hDefaults.IsValid() )
	{
		m_defaultMinDist	= hDefaults.GetFloat( "default_minimum_falloff" );
		m_defaultMaxDist	= hDefaults.GetFloat( "default_maximum_falloff" );
	}
	else
	{
		m_defaultMinDist	= 1.0f;
		m_defaultMaxDist	= 50.0f;
	}
}

// Destruction
WorldSound::~WorldSound()
{
	Clear();
}

// Enable wrapper for the sound system.  Properly handles stopping and restarting all
// managed samples when the state of the sound system changes.
bool WorldSound::Enable( bool bEnable, bool bEnableEAX )
{
	// If the sound system is being disabled just clear all managed samples
	if( !bEnable )
	{
		Clear();
	}
	else
	{
		// Go through all managed samples and pause them
		for( ManagedSampleList::iterator i = m_managedSampleList.begin(); i != m_managedSampleList.end(); ++i )
		{
			if( !(*i).bPaused )
			{
				// Get the position of this sample
				(*i).currentPos	= gSoundManager.GetSampleOffset( (*i).soundId );
				(*i).bPaused	= true;

				// Stop this sample
				gSoundManager.ClearSampleStopCallback( (*i).soundId );
				gSoundManager.StopSample( (*i).soundId );
				(*i).soundId = GPGSound::INVALID_SOUND_ID;
			}
		}
	}

	// Pass the enable on to the sound system
	return gSoundManager.Enable( bEnable, bEnableEAX );
}

// Update all currently managed samples
void WorldSound::Update( bool bPaused )
{
	for( ManagedSampleList::iterator i = m_managedSampleList.begin(); i != m_managedSampleList.end(); )
	{
		// Get the node that this sound exists in
		siege::SiegeNode* pNode	= gSiegeEngine.IsNodeValid( (*i).soundPos.node );
		if( !pNode ||
			(IsMultiPlayer() && !(pNode->GetFrustumOwnedBitfield() & gSiegeEngine.GetRenderFrustum())) )
		{
			if( !(*i).bPaused )
			{
				// Stop this sound
				gSoundManager.ClearSampleStopCallback( (*i).soundId );
				gSoundManager.StopSample( (*i).soundId );
			}

			// Erase it from our listing
			i	= m_managedSampleList.erase( i );
			continue;
		}

		// Check to see what frustum this sound currently resides in
		if( !(pNode->GetFrustumOwnedBitfield() & gSiegeEngine.GetRenderFrustum()) )
		{
			// Sound is not in the active frustum, so we must pause it
			if( !(*i).bPaused )
			{
				// Get the position of this sample
				(*i).currentPos	= gSoundManager.GetSampleOffset( (*i).soundId );
				(*i).bPaused	= true;

				// Stop this sample
				gSoundManager.ClearSampleStopCallback( (*i).soundId );
				gSoundManager.StopSample( (*i).soundId );
				(*i).soundId = GPGSound::INVALID_SOUND_ID;
			}
		}
		else
		{
			// Sound is in the active frustum, so see if we need to restart it
			if( (*i).bPaused && !bPaused )
			{
				// No longer paused
				(*i).bPaused	= false;

				// Replay
				(*i).soundId	= gSoundManager.PlaySample( (*i).soundFile, pNode->NodeToWorldSpace( (*i).soundPos.pos ), (*i).bLoop,
															(*i).minDist, (*i).maxDist, (*i).type, (*i).currentPos );

				// Check for success
				if( (*i).soundId != GPGSound::INVALID_SOUND_ID )
				{
					// Setup the stop callback
					gSoundManager.SetSampleStopCallback( (*i).soundId, makeFunctor( *this, &WorldSound::SampleStopCallback ) );
				}
				else
				{
					// Erase it from our listing
					i	= m_managedSampleList.erase( i );
					continue;
				}
			}
			else if( gSoundManager.IsSamplePlaying( (*i).soundId ) )
			{
				// Simply update the position of this sound
				gSoundManager.SetSamplePosition( (*i).soundId, pNode->NodeToWorldSpace( (*i).soundPos.pos ) );
			}
		}

		++i;
	}
}

// Play a managed 3D sound
DWORD WorldSound::PlaySample( const gpstring& soundFile, const SiegePos& position, bool bLoop,
							  const float min_dist, const float max_dist, GPGSound::eSampleType type )
{
	// Get the SiegeNode and make sure it is in the world
	siege::SiegeNode* pNode		= gSiegeEngine.IsNodeValid( position.node );
	if( !pNode || !(pNode->GetFrustumOwnedBitfield() & gSiegeEngine.GetRenderFrustum()) )
	{
		return GPGSound::INVALID_SOUND_ID;
	}

	// Create a new managed sample
	ManagedSound nManagedSound;

	nManagedSound.soundFile		= soundFile;
	nManagedSound.soundPos		= position;
	nManagedSound.bLoop			= bLoop;
	nManagedSound.minDist		= IsNegative( min_dist ) ? m_defaultMinDist : min_dist;
	nManagedSound.maxDist		= IsNegative( max_dist ) ? m_defaultMaxDist : max_dist;
	nManagedSound.type			= type;
	nManagedSound.bPaused		= false;
	nManagedSound.currentPos	= 0;

	// Play the new sample
	nManagedSound.soundId		= gSoundManager.PlaySample( soundFile, pNode->NodeToWorldSpace( position.pos ), bLoop, nManagedSound.minDist, nManagedSound.maxDist, type );

	if( nManagedSound.soundId != GPGSound::INVALID_SOUND_ID )
	{
		// Setup the stop callback
		gSoundManager.SetSampleStopCallback( nManagedSound.soundId, makeFunctor( *this, &WorldSound::SampleStopCallback ) );

		// Put it into our managed list
		m_managedSampleList.push_back( nManagedSound );
	}

	// Return sample id
	return nManagedSound.soundId;
}

// Set a stop sample callback
void WorldSound::SetSampleStopCallback( DWORD sampleID, GPGSound::SampleStopCallback callback )
{
	if( sampleID == GPGSound::INVALID_SOUND_ID )
	{
		return;
	}

	GPGSound::StopSampleCallbackMap::iterator i = m_sampleCallbackMap.find( sampleID );
	if( i == m_sampleCallbackMap.end() )
	{
		for( ManagedSampleList::iterator o = m_managedSampleList.begin(); o != m_managedSampleList.end(); ++o )
		{
			// Look for this sample
			if( (*o).soundId == sampleID )
			{
				m_sampleCallbackMap.insert( std::pair< DWORD, GPGSound::SampleStopCallback >( sampleID, callback ) );
				break;
			}
		}

		if( o == m_managedSampleList.end() )
		{
			gSoundManager.SetSampleStopCallback( sampleID, callback );
		}
	}
}

// Clear a stop sample callback
void WorldSound::ClearSampleStopCallback( DWORD sampleID )
{
	GPGSound::StopSampleCallbackMap::iterator i = m_sampleCallbackMap.find( sampleID );
	if( i != m_sampleCallbackMap.end() )
	{
		m_sampleCallbackMap.erase( i );
	}
	else
	{
		gSoundManager.ClearSampleStopCallback( sampleID );
	}
}

// Sample stop callback function
void WorldSound::SampleStopCallback( DWORD sampleId )
{
	for( ManagedSampleList::iterator i = m_managedSampleList.begin(); i != m_managedSampleList.end(); ++i )
	{
		// Look for this sample
		if( (*i).soundId == sampleId )
		{
			m_managedSampleList.erase( i );
			break;
		}
	}

	// Call the stop callback in it is registered
	GPGSound::StopSampleCallbackMap::iterator c = m_sampleCallbackMap.find( sampleId );
	if( c != m_sampleCallbackMap.end() )
	{
		(*c).second( sampleId );
		m_sampleCallbackMap.erase( c );
	}
}