/*******************************************************************************************
**
**									GPGSound Manager
**
**		See .h file for details
**
********************************************************************************************/

// Core includes
#include "appmodule.h"
#include "gpcore.h"
#include "gpcoll.h"
#include "namingkey.h"
#include "stringtool.h"
#include "reportsys.h"
#include "fuel.h"
#include "matrix_3x3.h"
#include "space_3.h"
#include "config.h"
#include "gpprofiler.h"
#include "gpmem.h"

// GPGSound includes
#include "gps_manager.h"

// Miles Sound System
#include "mss.h"

// Libaries required by GPGSound
#pragma comment ( lib, "mss32.lib" )


using namespace GPGSound;

// Invalid sound identifier
const DWORD GPGSound::INVALID_SOUND_ID		= 0xFFFFFFFF;
const DWORD GPGSound::INVALID_PROVIDER		= 0xFFFFFFFF;
const DWORD GPGSound::INVALID_FILE_INDEX	= 0xFFFFFFFF;
const DWORD GPGSound::INVALID_SED_INDEX		= 0;
const DWORD GPGSound::SOUND_MIN_VOLUME		= 0;
const DWORD GPGSound::SOUND_MAX_VOLUME		= 127;
const float GPGSound::SOUND_TIMER			= 100.0f;


#if !GP_RETAIL

static ReportSys::Context* gSoundContextPtr   = NULL;

ReportSys::Context& GetSoundContext( void )
{
	if ( gSoundContextPtr == NULL )
	{
		static ReportSys::Context s_SoundContext( &gGenericContext, "Sound" );
		gSoundContextPtr = &s_SoundContext;

		s_SoundContext.Enable( false );
	}
	return ( *gSoundContextPtr );
}

bool CheckSoundError()
{
	// Check for potential sound system error
	char* pError	= AIL_last_error();
	if( *pError != 0 )
	{
		// Error has occured
		gperrorf(( "Sound system error: %s", pError ));
		return true;
	}
	return false;
}

#else

#define CheckSoundError()

#endif // !GP_RETAIL

#define gpsound( msg ) DEV_MACRO_REPORT( &GetSoundContext(), msg )
#define gpsoundf( msg ) DEV_MACRO_REPORTF( &GetSoundContext(), msg )


// Construction
SoundManager::SoundManager()
	: m_b3DSoundSupport( false )
	, m_bEnabled( false )
	, m_bStasis( false )
	, m_bFadeStopping( false )
	, m_bFadePausing( false )
	, m_bFadeResuming( false )
	, m_bReleaseTimer( false )
	, m_bStopAllSounds( false )
	, m_bPauseAllSounds( false )
	, m_hDigitalDriver( NULL )
	, m_masterVolume( SOUND_MAX_VOLUME )
	, m_lowSampleVolume( SOUND_MAX_VOLUME )
	, m_normSampleVolume( SOUND_MAX_VOLUME )
	, m_highSampleVolume( SOUND_MAX_VOLUME )
	, m_ambientSampleVolume( SOUND_MAX_VOLUME )
	, m_ambientStreamVolume( SOUND_MAX_VOLUME )
	, m_voiceStreamVolume( SOUND_MAX_VOLUME )
	, m_musicStreamVolume( SOUND_MAX_VOLUME )
	, m_hTimer( 0 )
	, m_fadeTime( 0.0f )
	, m_userData( 0 )
	, m_serviceTime( 0.0f )
	, m_sampleFileExpiration( 30.0f )
	, m_soundLatency( 8 )
	, m_primaryFreq( 44100 )
	, m_primaryBitRate( 16 )
	, m_numPrimaryChannels( 2 )
	, m_h3DProvider( INVALID_PROVIDER )
	, m_defaultProvider( INVALID_PROVIDER )
	, m_eaxProvider( INVALID_PROVIDER )
	, m_bEAXAvailable( false )
	, m_bEAXEnabled( false )
	, m_speakerType( SPK_STEREO )
	, m_roomType( ROOM_GENERIC )
	, m_p3DListener( NULL )
	, m_pSamples( NULL )
	, m_numSamples( 0 )
	, m_p3DSamples( NULL )
	, m_num3DSamples( 0 )
	, m_maxNumStreams( 6 )
{
	// Setup file callback for streaming
	AIL_set_file_callbacks( FileOpenCallback, FileCloseCallback, FileSeekCallback, FileReadCallback );

	// Miles memory hooks
//	AIL_mem_use_malloc( AllocateCallback );
//	AIL_mem_use_free( FreeCallback );
}

// Destruction
SoundManager::~SoundManager()
{
	// Disable the system
	Enable( false, false );
}

// Initialize DirectSound and prepare for playing
bool SoundManager::Initialize( DWORD numPrimaryChannels, DWORD primaryFreq, DWORD primaryBitRate,
							   const gpstring& redistDir, const gpstring& requested3Dprovider, bool bEnable )
{
	// Save off digitial driver information
	m_primaryFreq			= primaryFreq;
	m_primaryBitRate		= primaryBitRate;
	m_numPrimaryChannels	= numPrimaryChannels;

	// Set the redist directory
	m_redistDir				= redistDir;

	// Save off requested 3D provider
	m_requested3DProvider	= requested3Dprovider;

	// Enable the sound system
	if( bEnable )
	{
		return( Enable( true, true ) );
	}
	else
	{
		return true;
	}
}

// Set the sound mix latency
void SoundManager::SetSoundLatency( DWORD msLatency )
{
	// Store the requested latency
	// $$$ For some unknown reason if I go any higher than DS_FRAGMENT_CNT - 3 I start getting
	//	   tons of crackling.  So for now I'll just leave it.
	m_soundLatency	= min_t( msLatency / 8, (DWORD)AIL_get_preference( DIG_DS_FRAGMENT_CNT ) - 3 );

	// Return if we are not currently enabled
	if( !m_bEnabled )
	{
		return;
	}

	// Set the new latency
	AIL_set_preference( DIG_DS_MIX_FRAGMENT_CNT, m_soundLatency );
}

// Disable/enable the sound system
bool SoundManager::Enable( bool bEnable, bool bEnableEAX )
{
	// See if we are doing any worthwhile work
	if( bEnable == m_bEnabled )
	{
		if( bEnableEAX == m_bEAXEnabled )
		{
			return true;
		}
		else if( m_bEnabled )
		{
			// Disable the system for EAX init
			Enable( false, false );
		}
	}

	if( bEnable )
	{
		// See if command line for eax is available
		if( gConfig.HasKey( "enable_eax" ) )
		{
			bEnableEAX = gConfig.GetBool( "enable_eax" );
		}

		// Enable the sound system
		AIL_set_redist_directory( m_redistDir.c_str() );

		// Startup the system
		AIL_startup();

		// Open a digital sound driver
		m_hDigitalDriver		= AIL_open_digital_driver( m_primaryFreq, m_primaryBitRate, m_numPrimaryChannels, 0 );
		if( !m_hDigitalDriver )
		{
			// No sound card
			AIL_shutdown();
			m_bEnabled	= false;
			return false;
		}

		// Set the latency
		AIL_set_preference( DIG_DS_MIX_FRAGMENT_CNT, m_soundLatency );

		// Enumerate the 3D sound providers
		HPROENUM next				= HPROENUM_FIRST;
		SoundProvider providerInfo;
		m_defaultProvider			= INVALID_PROVIDER;
		m_eaxProvider				= INVALID_PROVIDER;

		while( AIL_enumerate_3D_providers( &next, &providerInfo.hProvider, &providerInfo.pProviderName ) )
		{
			// Put this provider into our list
			m_SoundProviderColl.push_back( providerInfo );

			if( !strcmp( providerInfo.pProviderName, "Miles Fast 2D Positional Audio" ) )
			{
				m_defaultProvider	= providerInfo.hProvider;
			}

			if( !strcmp( providerInfo.pProviderName, "Creative Labs EAX (TM)" ) )
			{
				m_eaxProvider		= providerInfo.hProvider;
			}

			if( !strcmp( providerInfo.pProviderName, m_requested3DProvider.c_str() ) )
			{
				m_h3DProvider		= providerInfo.hProvider;
			}
		}

		// Open a 3D sound provider if requested
		if( m_h3DProvider == INVALID_PROVIDER &&
			(m_defaultProvider == INVALID_PROVIDER && m_eaxProvider == INVALID_PROVIDER) )
		{
			m_b3DSoundSupport		= false;
		}
		else
		{
			if( m_h3DProvider == INVALID_PROVIDER )
			{
				if( bEnableEAX )
				{
					// Try EAX
					m_h3DProvider		= m_eaxProvider;
					if( AIL_open_3D_provider( m_h3DProvider ) != 0 )
					{
						m_h3DProvider	= m_defaultProvider;
					}
					else
					{
						m_bEAXAvailable		= true;
						m_bEAXEnabled		= true;
						m_b3DSoundSupport	= true;
					}
				}
				else
				{
					m_h3DProvider	= m_defaultProvider;
				}
			}

			if( !m_b3DSoundSupport )
			{
				if( (AIL_open_3D_provider( m_h3DProvider ) == 0) )
				{
					m_b3DSoundSupport	= true;
				}
				else
				{
					gperrorf(( "3D provider requested could not be opened!  %s", AIL_last_error() ));
					return false;
				}
			}
		}

		// Open all sample handles available
		m_numSamples	= AIL_get_preference( DIG_MIXER_CHANNELS ) - m_maxNumStreams;
		if( m_b3DSoundSupport && !m_bEAXEnabled )
		{
			m_numSamples	/= 2;
		}

		m_pSamples		= new SampleInfo[ m_numSamples ];

		for( unsigned int i = 0; i < m_numSamples; ++i )
		{
			m_pSamples[ i ].hSample		= AIL_allocate_sample_handle( m_hDigitalDriver );
			m_pSamples[ i ].sampleType	= SND_EFFECT_NORM;
			m_pSamples[ i ].sedIndex	= INVALID_SED_INDEX;
			m_pSamples[ i ].bClosed		= true;
			m_pSamples[ i ].sampleIndex	= i;
		}

		// Open all 3D sample handles available
		if( m_b3DSoundSupport )
		{
			// Collect all of the samples
			std::vector< H3DSAMPLE > sampleColl;
			while( m_num3DSamples < m_numSamples && m_num3DSamples <= 24 )
			{
				H3DSAMPLE hSample	= AIL_allocate_3D_sample_handle( m_h3DProvider );
				if( !hSample )
				{
					break;
				}

				sampleColl.push_back( hSample );
				++m_num3DSamples;
			}

			if( m_num3DSamples )
			{
				// Allocate space for our 3D samples
				m_p3DSamples	= new Sample3DInfo[ m_num3DSamples ];

				// Build a list of our 3D sample handles
				i = 0;
				for( std::vector< H3DSAMPLE >::iterator s = sampleColl.begin(); s != sampleColl.end(); ++s, ++i )
				{
					m_p3DSamples[ i ].hSample		= (*s);
					m_p3DSamples[ i ].sampleType	= SND_EFFECT_NORM;
					m_p3DSamples[ i ].sedIndex		= INVALID_SED_INDEX;
					m_p3DSamples[ i ].bClosed		= true;
					m_p3DSamples[ i ].sampleIndex	= m_numSamples + i;
				}

				// Open a listener object
				m_p3DListener		= AIL_open_3D_listener( m_h3DProvider );

				// Set types
				SetSpeakerType( m_speakerType );
				SetRoomType( m_roomType );
			}
			else
			{
				// No support really exists
				AIL_close_3D_provider( m_h3DProvider );
				m_b3DSoundSupport	= false;
				m_bEAXEnabled		= false;
				m_bEAXAvailable		= false;
				m_h3DProvider		= INVALID_PROVIDER;
			}
		}

		m_bEnabled	= true;
	}
	else
	{
		// Release the timer handle
		if( m_hTimer )
		{
			AIL_release_timer_handle( m_hTimer );
			m_hTimer			= 0;
		}

		// Mark all fades as completed
		m_bFadeResuming		= false;
		m_bFadeStopping		= false;
		m_bFadePausing		= false;

		// Signify that the timer has been released
		m_bReleaseTimer		= false;

		// Close all open handles
		for( unsigned int i = 0; i < m_numSamples; ++i )
		{
			AIL_release_sample_handle( m_pSamples[ i ].hSample );
		}
		delete[] m_pSamples;
		m_pSamples		= NULL;
		m_numSamples	= 0;

		for( i = 0; i < m_num3DSamples; ++i )
		{
			AIL_release_3D_sample_handle( m_p3DSamples[ i ].hSample );
		}
		delete[] m_p3DSamples;
		m_p3DSamples	= NULL;
		m_num3DSamples	= 0;

		// Clean up descriptor caches
		m_SampleDescriptorCache.clear();
		m_SampleDescriptorNameMap.clear();

		// Clear the stream cache
		m_StreamCache.clear();

		if( m_b3DSoundSupport )
		{
			// Close the listener
			AIL_close_3D_listener( m_p3DListener );
			m_p3DListener		= NULL;

			// Close the 3D provider
			AIL_close_3D_provider( m_h3DProvider );

			m_h3DProvider		= INVALID_PROVIDER;
			m_b3DSoundSupport	= false;
			m_bEAXAvailable		= false;
			m_bEAXEnabled		= false;
		}

		// Shut down the system
		AIL_shutdown();

		// Clear stop requests
		m_sample2DStopRequest.clear();
		m_sample3DStopRequest.clear();
		m_streamStopRequest.clear();

		m_stopSampleCallbackMap.clear();
		m_stopStreamCallbackMap.clear();

		// Close down all open files
		for( SampleFileCache::iterator f = m_SampleFileCache.begin(); f != m_SampleFileCache.end(); ++f )
		{
			(*f).memHandle.Close();
			(*f).fileHandle.Close();
		}
		m_SampleFileCache.clear();
		m_SampleFileNameMap.clear();

#if !GP_RETAIL

		m_SampleHistoryMap.clear();

#endif

		m_bEnabled	= false;
	}

	// Success
	return true;
}

// Speaker control
void SoundManager::SetSpeakerType( eSpeakerType speakerType )
{
	// Save and set the requested speaker type
	m_speakerType	= speakerType;
	
	// No more speaker changing, sorry.  Unilaterally shitty software is making this a pain in the ass when
	// combined with moronic users and retarded system setups.  Therefore, I'm giving the OS complete control
	// over the speaker config so I can never again be accused of changing it.  And if you don't like it,
	// well then the following song is for you:
	//
	// (sung to the tune of Meow Mix)
	// "I hate sound cards"
	// "I hate speakers"
	// "Creative and Microsoft make my life bleaker"
	//
/*
	if( m_b3DSoundSupport			&&
		m_speakerType != SPK_SYSTEM &&
		!m_bEAXEnabled )
	{
		// Set the speaker type
		AIL_set_3D_speaker_type( m_h3DProvider, m_speakerType );
	}
*/
}

// Room type control
void SoundManager::SetRoomType( eRoomType roomType )
{
	// Save and set the requested room type
	m_roomType	= roomType;
	if( m_b3DSoundSupport )
	{
		// Set the speaker type
		AIL_set_3D_room_type( m_h3DProvider, m_roomType );
	}
}

// Update the library for file expiration and stream servicing
void SoundManager::Update( float deltaTime, const vector_3& listener_pos, const vector_3& listener_face, const vector_3& listener_up )
{
	GPPROFILERSAMPLE( "SoundManager :: Update", SP_MISC );
	if( !m_bEnabled )
	{
		return;
	}

	// Call the inactive update
	InactiveUpdate( false );

	// Update our file cache
	for( SampleFileCache::iterator f = m_SampleFileCache.begin(); f != m_SampleFileCache.end(); )
	{
		if( (*f).refCount == 0 )
		{
			// Check timing to see if we should expire this sample
			(*f).lastUse	+= deltaTime;
			if( (*f).lastUse >= m_sampleFileExpiration )
			{
				// Remove this filename from our map
				for( SampleFileNameMap::iterator i = m_SampleFileNameMap.begin(); i != m_SampleFileNameMap.end(); ++i )
				{
					if( (*i).second == f.GetIndex() )
					{
						gpsoundf(( "Closed sample file: %s\n", (*i).first.c_str() ));
						m_SampleFileNameMap.erase( i );
						break;
					}
				}

				// Close handles
				(*f).memHandle.Close();
				(*f).fileHandle.Close();

				// Remove file from our cache
				DWORD delIter = f.GetIndex();
				++f;

				m_SampleFileCache.Free( delIter );
				continue;
			}
		}

		++f;
	}

	// Double buffered collections
	Stop2DSampleRequestColl sample2DStopRequest;
	Stop3DSampleRequestColl sample3DStopRequest;
	StopStreamRequestColl	streamStopRequest;

	{
		// Lock the sound critical
		kerneltool::Critical::Lock autoLock( m_soundCritical );

		// $$$ do swap instead!  copy is lamo!
		sample2DStopRequest	= m_sample2DStopRequest;
		sample3DStopRequest	= m_sample3DStopRequest;
		streamStopRequest	= m_streamStopRequest;

		m_sample2DStopRequest.clear();
		m_sample3DStopRequest.clear();
		m_streamStopRequest.clear();
	}

	// Close requested 2D samples
	for( Stop2DSampleRequestColl::iterator r2d = sample2DStopRequest.begin(); r2d != sample2DStopRequest.end(); ++r2d )
	{
		for( unsigned int sampleIndex = 0; sampleIndex < m_numSamples; ++sampleIndex )
		{
			if( m_pSamples[ sampleIndex ].hSample == (*r2d) )
			{
				// Callback if necessary
				StopSampleCallbackMap::iterator c = m_stopSampleCallbackMap.find( m_pSamples[ sampleIndex ].sampleIndex );
				if( c != m_stopSampleCallbackMap.end() )
				{
					(*c).second( m_pSamples[ sampleIndex ].sampleIndex );

					// Erase the callback
					m_stopSampleCallbackMap.erase( c );
				}

				gpassert( !IsSamplePlaying( m_pSamples[ sampleIndex ].sampleIndex ) );

				// Mark the sample as closed
				gpassert( !m_pSamples[ sampleIndex ].bClosed );
				m_pSamples[ sampleIndex ].bClosed	= true;

				// Release the descriptor
				ReleaseSoundDescriptor( m_pSamples[ sampleIndex ].sedIndex );
				gpsoundf(( "Sample closed ID: %d\n", m_pSamples[ sampleIndex ].sampleIndex ));
			}
		}
	}

	// Close requested 3D samples
	for( Stop3DSampleRequestColl::iterator r3d = sample3DStopRequest.begin(); r3d != sample3DStopRequest.end(); ++r3d )
	{
		for( unsigned int sampleIndex = 0; sampleIndex < m_num3DSamples; ++sampleIndex )
		{
			if( m_p3DSamples[ sampleIndex ].hSample == (*r3d) )
			{
				// Callback if necessary
				StopSampleCallbackMap::iterator c = m_stopSampleCallbackMap.find( m_p3DSamples[ sampleIndex ].sampleIndex );
				if( c != m_stopSampleCallbackMap.end() )
				{
					(*c).second( m_p3DSamples[ sampleIndex ].sampleIndex );

					// Erase the callback
					m_stopSampleCallbackMap.erase( c );
				}

				gpassert( !IsSamplePlaying( m_p3DSamples[ sampleIndex ].sampleIndex ) );

				// Mark the sample as closed
				gpassert( !m_p3DSamples[ sampleIndex ].bClosed );
				m_p3DSamples[ sampleIndex ].bClosed	= true;

				// Release the descriptor
				ReleaseSoundDescriptor( m_p3DSamples[ sampleIndex ].sedIndex );
				gpsoundf(( "3D Sample closed ID: %d\n", m_p3DSamples[ sampleIndex ].sampleIndex ));
			}
		}
	}

	// Close requested streams
	for( StopStreamRequestColl::iterator s = streamStopRequest.begin(); s != streamStopRequest.end(); ++s )
	{
		// Callback if neccesary
		StopStreamCallbackMap::iterator i = m_stopStreamCallbackMap.find( (*s) );
		if( i != m_stopStreamCallbackMap.end() )
		{
			// Call the function
			(*i).second( (*s) );

			// Erase callback
			m_stopStreamCallbackMap.erase( i );
		}

		// Cleanup stream cache
		for( StreamCache::iterator c = m_StreamCache.begin(); c != m_StreamCache.end(); ++c )
		{
			if( (*s) == (DWORD)(*c).hStream )
			{
				// End the stream
				AIL_close_stream( (HSTREAM)(*s) );
				gpsoundf(( "Closed stream ID: %d\n", (*s) ));

				// Erase stream from the cache
				m_StreamCache.erase( c );
				break;
			}
		}
	}

	// Update our internal stream cache
	for( StreamCache::iterator c = m_StreamCache.begin(); c != m_StreamCache.end(); ++c )
	{
		if( (*c).fadeTime > 0.0f )
		{
			// Continue fade on stream
			(*c).fadeCurrentTime	+= deltaTime;

			// Set the volume
			if( (*c).fadeStop )
			{
				AIL_set_stream_volume( (*c).hStream, FTOL( (float)(*c).fadeVolume * max_t( 0.0f, (1.0f - ((*c).fadeCurrentTime / (*c).fadeTime)) ) ) );

				if( (*c).fadeCurrentTime >= (*c).fadeTime )
				{
					StopStream( (DWORD)(*c).hStream );
				}
			}
			else
			{
				AIL_set_stream_volume( (*c).hStream, FTOL( (float)(*c).fadeVolume * max_t( 0.0f, (*c).fadeCurrentTime / (*c).fadeTime) ) );

				if( (*c).fadeCurrentTime >= (*c).fadeTime )
				{
					(*c).fadeTime			= 0.0f;
					(*c).fadeCurrentTime	= 0.0f;

					// Update volume in case settings changed during fade
					SetStreamVolume( (DWORD)(*c).hStream, GetStreamVolume( (*c).streamType ) );
				}
			}
		}
	}

	if( m_b3DSoundSupport )
	{
		// Set the position of the listener
		AIL_set_3D_position( m_p3DListener, listener_pos.x, listener_pos.y, -listener_pos.z );
		AIL_set_3D_orientation( m_p3DListener,
								listener_face.x, listener_face.y, -listener_face.z,
								listener_up.x, listener_up.y, -listener_up.z );

		// Save the listener position
		m_listenerPos		= listener_pos;
	}
}

// Inactive update
void SoundManager::InactiveUpdate( bool bUpdate )
{
	if( !m_bEnabled )
	{
		return;
	}

	if( m_bReleaseTimer )
	{
		// Release the timer handle
		AIL_release_timer_handle( m_hTimer );
		m_hTimer			= 0;

		// Mark all fades as completed
		m_bFadeResuming		= false;
		m_bFadeStopping		= false;
		m_bFadePausing		= false;

		// Signify that the timer has been released
		m_bReleaseTimer		= false;
	}

	if( m_bStopAllSounds )
	{
		// Stop all sounds
		StopAllSounds( bUpdate );
		m_bStopAllSounds	= false;

		// Reset the volume
		AIL_set_digital_master_volume( m_hDigitalDriver, m_masterVolume );
	}

	if( m_bPauseAllSounds )
	{
		// Pause all sounds
		PauseAllSounds();
		m_bPauseAllSounds	= false;

		// Reset the volume
		AIL_set_digital_master_volume( m_hDigitalDriver, m_masterVolume );
		SetSampleVolume( SND_EFFECT_LOW, m_lowSampleVolume );
		SetSampleVolume( SND_EFFECT_NORM, m_normSampleVolume );
		SetSampleVolume( SND_EFFECT_HIGH, m_highSampleVolume );
		SetSampleVolume( SND_EFFECT_AMBIENT, m_ambientSampleVolume );
	}
}

// Play sound
DWORD SoundManager::PlaySample( const gpstring& soundFile, bool bLoop,
								eSampleType type, DWORD offset )
{
	if( !m_bEnabled || m_bStasis || !gAppModule.IsAppActive() )
	{
		return INVALID_SOUND_ID;
	}

	// Get a handle to use with this sample
	SampleInfo* pSample	= GetFreeSampleHandle( type );

	// Return invalid sound ID if we were unable to find a free sample handle
	if( !pSample )
	{
		return INVALID_SOUND_ID;
	}

	HSAMPLE& hSample	= pSample->hSample;

	// Return invalid sound ID if we were unable to find a free sample handle
	if( !hSample )
	{
		return INVALID_SOUND_ID;
	}

	// Initialize the new handle
	AIL_init_sample( hSample );

	// Build proper relative pathname
	gpstring strPath;
	gpstring strFile( soundFile );
	stringtool::RemoveExtension( strFile );
	gNamingKey.BuildContentLocation( strFile.c_str(), strPath );

	// See if this sound already exists
	SampleFileNameMap::iterator i = m_SampleDescriptorNameMap.find( strPath );
	if( i != m_SampleDescriptorNameMap.end() )
	{
		if( m_SampleDescriptorCache.IsValid( (*i).second ) )
		{
			SoundDescriptor& sed	= m_SampleDescriptorCache.Dereference( (*i).second );

			if( sed.refCount >= sed.maxSamples )
			{
				return INVALID_SOUND_ID;
			}

#if !GP_RETAIL

			// Increment the history count for this sample
			SampleFileNameMap::iterator h = m_SampleHistoryMap.find( strPath );
			if( h != m_SampleHistoryMap.end() )
			{
				(*h).second++;
			}

#endif

			if( m_SampleFileCache.IsValid( sed.fileIndex ) )
			{
				// Set the sample to use this file
				if( !AIL_set_sample_file( hSample, m_SampleFileCache[ sed.fileIndex ].memHandle.GetData(), 0 ) )
				{
					gperrorf(( "Could not set sound to sample %s:  %s", strPath.c_str(), AIL_last_error() ));
					return INVALID_SOUND_ID;
				}

				// Update index
				pSample->sedIndex	= (*i).second;
				pSample->bClosed	= false;

				sed.refCount++;
			}
			else
			{
				gperror( "Sound file was in a descriptor but not in file cache!" );
				return INVALID_SOUND_ID;
			}
		}
		else
		{
			gperror( "Sound descriptor was in name mapping but not in descriptor cache!" );
			return INVALID_SOUND_ID;
		}
	}
	else
	{
		// Create a new sound descriptor
		DWORD sedIndex	= CreateSoundDescriptor( strPath );
		if( !sedIndex )
		{
			return INVALID_SOUND_ID;
		}

		// Set the sample to use this file
		if( !AIL_set_sample_file( hSample, m_SampleFileCache[ m_SampleDescriptorCache[ sedIndex ].fileIndex ].memHandle.GetData(), 0 ) )
		{
			gperrorf(( "Could not set sound to sample %s:  %s", strPath.c_str(), AIL_last_error() ));
			
			// Cleanup
			ReleaseSoundDescriptor( sedIndex );

			// Return invalid
			return INVALID_SOUND_ID;
		}

		// Update index
		pSample->sedIndex	= sedIndex;
		pSample->bClosed	= false;
	}

	// Setup for looping if requested
	if( bLoop )
	{
		AIL_set_sample_loop_count( hSample, 0 );
	}

	// Set sample volume based on type
	AIL_set_sample_volume( hSample, GetSampleVolume( type ) );

	// Set the playback rate
	float playbackRate	= Random( m_SampleDescriptorCache[ pSample->sedIndex ].minPlaybackRate,
								  m_SampleDescriptorCache[ pSample->sedIndex ].maxPlaybackRate );
	if( !IsOne( playbackRate ) )
	{
		float sampleRate = (float)AIL_sample_playback_rate( hSample );
		AIL_set_sample_playback_rate( hSample, FTOL( playbackRate * sampleRate ) );
	}

	// Register the callback
	AIL_register_EOS_callback( hSample, EndOfSampleCallback );

	// Set the sample offset
	AIL_set_sample_position( hSample, offset );

	// Play the sample
	AIL_start_sample( hSample );

	gpsoundf(( "Sample playing: %s with ID: %d\n", strPath.c_str(), pSample->sampleIndex ));

	// Return the id of the sound
	return( pSample->sampleIndex );
}

// Play sound with position
DWORD SoundManager::PlaySample( const gpstring& soundFile, const vector_3& position, bool bLoop, 
							    const float min_dist, const float max_dist,
								eSampleType type, DWORD offset )
{
	if( !m_bEnabled || m_bStasis || !gAppModule.IsAppActive() )
	{
		return INVALID_SOUND_ID;
	}

	if( m_b3DSoundSupport )
	{
		// Get a handle to use with this sample
		Sample3DInfo* pSample	= GetFree3DSampleHandle( type );

		// Return invalid sound ID if we were unable to find a free sample handle
		if( !pSample )
		{
			return INVALID_SOUND_ID;
		}

		H3DSAMPLE& hSample		= pSample->hSample;
		if( !hSample )
		{
			return INVALID_SOUND_ID;
		}

		// Build proper relative pathname
		gpstring strPath;
		gpstring strFile( soundFile );
		stringtool::RemoveExtension( strFile );
		gNamingKey.BuildContentLocation( strFile.c_str(), strPath );

		// See if this sound already exists
		SampleFileNameMap::iterator i = m_SampleDescriptorNameMap.find( strPath );
		if( i != m_SampleDescriptorNameMap.end() )
		{
			if( m_SampleDescriptorCache.IsValid( (*i).second ) )
			{
				SoundDescriptor& sed	= m_SampleDescriptorCache.Dereference( (*i).second );

				if( sed.refCount >= sed.maxSamples )
				{
					return INVALID_SOUND_ID;
				}

#if !GP_RETAIL

				// Increment the history count for this sample
				SampleFileNameMap::iterator h = m_SampleHistoryMap.find( strPath );
				if( h != m_SampleHistoryMap.end() )
				{
					(*h).second++;
				}

#endif

				if( m_SampleFileCache.IsValid( sed.fileIndex ) )
				{
					// Set the sample to use this file
					if( !AIL_set_3D_sample_file( hSample, m_SampleFileCache[ sed.fileIndex ].memHandle.GetData() ) )
					{
						if( !m_bEAXEnabled )
						{
							gperrorf(( "Could not set 3D sound to sample %s:  %s", strPath.c_str(), AIL_last_error() ));
						}
						return INVALID_SOUND_ID;
					}

					// Update index
					pSample->sedIndex	= (*i).second;
					pSample->bClosed	= false;

					sed.refCount++;
				}
				else
				{
					gperror( "Sound file was in a descriptor but not in file cache!" );
					return INVALID_SOUND_ID;
				}
			}
			else
			{
				gperror( "Sound descriptor was in name mapping but not in descriptor cache!" );
				return INVALID_SOUND_ID;
			}
		}
		else
		{
			// Create a new sound descriptor
			DWORD sedIndex	= CreateSoundDescriptor( strPath );
			if( !sedIndex )
			{
				return INVALID_SOUND_ID;
			}

			// Set the sample to use this file
			if( !AIL_set_3D_sample_file( hSample, m_SampleFileCache[ m_SampleDescriptorCache[ sedIndex ].fileIndex ].memHandle.GetData() ) )
			{
				if( !m_bEAXEnabled )
				{
					gperrorf(( "Could not set 3D sound to sample %s:  %s", strPath.c_str(), AIL_last_error() ));
				}
				
				// Cleanup
				ReleaseSoundDescriptor( sedIndex );

				// Return invalid
				return INVALID_SOUND_ID;
			}

			// Update index
			pSample->sedIndex	= sedIndex;
			pSample->bClosed	= false;
		}

		// Setup for looping if requested
		if( bLoop )
		{
			AIL_set_3D_sample_loop_count( hSample, 0 );
		}

		// Set sample volume based on type
		AIL_set_3D_sample_volume( hSample, GetSampleVolume( type ) );

		// Set the falloff
		SoundDescriptor& descriptor = m_SampleDescriptorCache.Dereference( pSample->sedIndex );
		if( !IsNegative( descriptor.minOverrideDist ) &&
			!IsNegative( descriptor.maxOverrideDist ) )
		{
			AIL_set_3D_sample_distances( hSample, descriptor.minOverrideDist, descriptor.maxOverrideDist );
		}
		else
		{
			AIL_set_3D_sample_distances( hSample, min_dist, max_dist );
		}

		// Set the playback rate
		float playbackRate	= Random( descriptor.minPlaybackRate,
									  descriptor.maxPlaybackRate );
		if( !IsOne( playbackRate ) )
		{
			float sampleRate = (float)AIL_3D_sample_playback_rate( hSample );
			AIL_set_3D_sample_playback_rate( hSample, FTOL( playbackRate * sampleRate ) );
		}

		// Disable room effects for better performance
		if( !m_bEAXEnabled || (m_roomType == ROOM_GENERIC) )
		{
			AIL_set_3D_sample_effects_level( hSample, 0.0f );
		}
		else
		{
			AIL_set_3D_sample_effects_level( hSample, EAX_REVERBMIX_USEDISTANCE );
		}

		// Register the callback
		AIL_register_3D_EOS_callback( hSample, EndOf3DSampleCallback );

		if( offset )
		{
			if( offset <= AIL_3D_sample_length( hSample ) )
			{
				// Set the play offset
				AIL_set_3D_sample_offset( hSample, offset );

				// Resume play
				AIL_resume_3D_sample( hSample );
			}
			else
			{
				// Play the sample
				AIL_start_3D_sample( hSample );
			}
		}
		else
		{
			// Play the sample
			AIL_start_3D_sample( hSample );
		}

		// Set the position
		SetSamplePosition( pSample->sampleIndex, position );

		gpsoundf(( "3D Sample playing: %s with ID: %d\n", strPath.c_str(), pSample->sampleIndex ));

		// Return the id of the sound
		return( pSample->sampleIndex );
	}
	else
	{
		return PlaySample( soundFile, bLoop, type );
	}
}

// Play stream
DWORD SoundManager::PlayStream( const gpstring& streamFile, eStreamType type, bool bLoop )
{
	if( !m_bEnabled || m_bStasis || streamFile.empty() || !gAppModule.IsAppActive() )
	{
		return INVALID_SOUND_ID;
	}

	// If the file stream has an extension, just build the content location.
	gpstring strPath;
	if( streamFile.find( '.' ) != gpstring::npos )
	{
		gNamingKey.BuildContentLocation( streamFile.c_str(), strPath );
	}
	else
	{
		// $ This code is necessary because we could be trying to load a WAV
		//   or MP3. Yet Miles requires the .mp3 extension in order to use the
		//   right decoder, so we can't simply use the FileSys aliases to load
		//   in a %SND% file or whatever. Instead we must do the slower work of
		//   detecting which it is *in advance* and then pass the results on
		//   to Miles. Necessary price to pay, but not terribly expensive
		//   considering (a) that sounds aren't played all that often and (b)
		//   we'll usually "hit" with the WAV first.
		//
		//   FUTURE: update filesys to support aliases in the find functions,
		//   and then just use Find()/GetNext() to get the name to pass into
		//   miles. -sb

		// Try it as a WAV first
		if (   !gNamingKey.BuildWAVLocation( streamFile, strPath )
			|| !FileSys::DoesResourceFileExist( strPath ) )
		{
			// Doesn't exist, so assume that this is an MP3 format file
			gNamingKey.BuildMP3Location( streamFile, strPath );
		}
	}

	if( !FileSys::DoesResourceFileExist( strPath ) )
	{
		gperrorf(( "Trying to play stream for a file that doesn't exist!  %s", strPath.c_str() ));
	}

	// Open the stream
	HSTREAM hStream	= AIL_open_stream( m_hDigitalDriver, strPath.c_str(), 0 );

	// Make sure we have a valid stream
	if( hStream )
	{
		// Put the new stream into our cache
		StreamInfo streamInfo;
		streamInfo.hStream			= hStream;
		streamInfo.streamType		= type;
		streamInfo.fadeTime			= 0.0f;
		streamInfo.fadeCurrentTime	= 0.0f;
		streamInfo.fadeVolume		= 0;

		m_StreamCache.push_back( streamInfo );

		// Setup the loop
		if( bLoop )
		{
			AIL_set_stream_loop_count( hStream, 0 );
		}

		// Set stream volume based on type
		AIL_set_stream_volume( hStream, GetStreamVolume( type ) );

		// Register for stream stop callback
		AIL_register_stream_callback( hStream, EndOfStreamCallback );

		// Play the stream
		AIL_start_stream( hStream );

		gpsoundf(( "Playing stream: %s with ID: %d\n", strPath.c_str(), hStream ));

		// Return
		return( (DWORD)hStream );
	}
	else
	{
		// $ if miles were to return an error code i could avoid this hacky str compare

		const char* lastErr = AIL_last_error();
		if ( same_no_case( "Error getting sound format.", lastErr ) )
		{
			gperrorf(( "Could not open audio stream! %s\n"
					   "\n"
					   "Note that this error is often caused by Miles drivers missing from the 'system\\mss' folder.\n", lastErr ));
		}
		else
		{
			gperrorf(( "Could not open audio stream %s! %s", strPath.c_str(), lastErr ));
		}
	}

	return INVALID_SOUND_ID;
}

// Play a stream and fade it in
DWORD SoundManager::FadePlayStream( const gpstring& streamFile, eStreamType type, bool bLoop, float fadeTimeInSeconds )
{
	// Play the stream
	DWORD streamID	= PlayStream( streamFile, type, bLoop );
	if( streamID != INVALID_SOUND_ID && IsPositive( fadeTimeInSeconds ) )
	{
		// Set the volume of the stream to minimum
		SetStreamVolume( streamID, SOUND_MIN_VOLUME );

		for( StreamCache::iterator s = m_StreamCache.begin(); s != m_StreamCache.end(); ++s )
		{
			if( (*s).hStream == (HSTREAM)streamID )
			{
				// Mark this stream for close on the next update
				(*s).fadeStop			= false;
				(*s).fadeTime			= fadeTimeInSeconds;
				(*s).fadeCurrentTime	= 0.0f;
				(*s).fadeVolume			= GetStreamVolume( type );
				break;
			}
		}
	}

	// Return
	return( streamID );
}

// Sound info
float SoundManager::GetSampleLengthInSeconds( const gpstring& soundFile )
{
	// Build proper relative pathname
	gpstring strPath;
	gpstring strFile( soundFile );
	stringtool::RemoveExtension( strFile );
	gNamingKey.BuildContentLocation( strFile.c_str(), strPath );

	// Final filename
	gpstring fileName;

	// Open the fuel block to the sound information
	FastFuelHandle hSound( FilePathToFuelAddress( strPath ) );
	if( hSound.IsValid() )
	{
		// Sound descriptor block has been found
		gpstring soundName;
		hSound.Get( "sound_effect_file", soundName );
		gNamingKey.BuildContentLocation( soundName, fileName );

		// Add .wav extension
		fileName	+= ".wav";
	}
	else
	{
		// No descriptor was found, so treat this as a basic sound file
		fileName	= strPath + ".wav";
	}

	// Open the file
	FilePair filePair;

	// Attempt to open the requested sound file
	if( !filePair.fileHandle.Open( fileName ) )
	{
		gperrorf(( "Could not open sound file %s", fileName.c_str() ));
		return 0.0f;
	}

	if( !filePair.memHandle.Map( filePair.fileHandle ) )
	{
		gperrorf(( "Could not memory map sound file %s", fileName.c_str() ));
		filePair.fileHandle.Close();
		return 0.0f;
	}

	// Touch the memory
	filePair.memHandle.Touch();

	// Get the information about this wav file
	AILSOUNDINFO soundInfo;
	AIL_WAV_info( filePair.memHandle.GetData(), &soundInfo );
	
	// Close the file
	filePair.memHandle.Close();
	filePair.fileHandle.Close();

	// Assert that we have what we expect
	gpassert( soundInfo.format == WAVE_FORMAT_PCM );
	gpassert( soundInfo.data_len <= (DWORD)filePair.memHandle.GetSize() );

	// Calculate the length, in seconds, of this sound
	return( (float)soundInfo.data_len / (float)((soundInfo.rate * ((soundInfo.bits / 4) * soundInfo.channels)) / 2) );
}

// Stop a sound
bool SoundManager::StopSample( DWORD sampleID )
{
	if( !m_bEnabled || (sampleID == INVALID_SOUND_ID || sampleID >= m_numSamples + m_num3DSamples) )
	{
		return false;
	}

	// Request a stop of this sample
	if( sampleID < m_numSamples )
	{
		HSAMPLE hSample	= (HSAMPLE)INVALID_SOUND_ID;

		{
			// Lock the sound critical
			kerneltool::Critical::Lock autoLock( m_soundCritical );

			if( !m_pSamples[ sampleID ].bClosed &&
				m_sample2DStopRequest.find( m_pSamples[ sampleID ].hSample ) == m_sample2DStopRequest.end() )
			{
				// Add the stop request
				hSample	= m_pSamples[ sampleID ].hSample;
				m_sample2DStopRequest.insert( hSample );
			}
		}

		if( hSample != (HSAMPLE)INVALID_SOUND_ID )
		{
			// Stop the sample
			AIL_end_sample( hSample );
		}
	}
	else
	{
		H3DSAMPLE h3dSample	= (H3DSAMPLE)INVALID_SOUND_ID;

		{
			DWORD sampleIndex	= sampleID - m_numSamples;
			gpassert( m_p3DSamples[ sampleIndex ].sampleIndex == sampleID );

			// Lock the sound critical
			kerneltool::Critical::Lock autoLock( m_soundCritical );

			if( !m_p3DSamples[ sampleIndex ].bClosed &&
				m_sample3DStopRequest.find( m_p3DSamples[ sampleIndex ].hSample ) == m_sample3DStopRequest.end() )
			{
				// Add the stop request
				h3dSample = m_p3DSamples[ sampleIndex ].hSample;
				m_sample3DStopRequest.insert( h3dSample );
			}
		}

		if( h3dSample != (H3DSAMPLE)INVALID_SOUND_ID )
		{
			// Stop the sample
			AIL_end_3D_sample( h3dSample );
		}
	}

	return true;
}

// Stop a stream
bool SoundManager::StopStream( DWORD streamID )
{
	if( !m_bEnabled || streamID == INVALID_SOUND_ID )
	{
		return false;
	}

	// Lock the sound critical
	kerneltool::Critical::Lock autoLock( m_soundCritical );

	// Put this stream ID into our stream stop collection
	m_streamStopRequest.insert( streamID );
	return false;
}

void SoundManager::StopAllStreams()
{
	if( !m_bEnabled )
	{
		return;
	}

	// Request end to all streams
	for( StreamCache::iterator s = m_StreamCache.begin(); s != m_StreamCache.end(); ++s )
	{
		StopStream( (DWORD)(*s).hStream );
	}

	// Force update to clean everything up
	Update( 0.0f );
}

// Fade and then stop a stream
bool SoundManager::FadeStopStream( DWORD streamID, float fadeTimeInSeconds )
{
	if( !m_bEnabled || streamID == INVALID_SOUND_ID )
	{
		return false;
	}

	if( IsPositive( fadeTimeInSeconds ) )
	{
		// Clean out our internal stream cache
		for( StreamCache::iterator s = m_StreamCache.begin(); s != m_StreamCache.end(); ++s )
		{
			if( (*s).hStream == (HSTREAM)streamID )
			{
				// Mark this stream for close on the next update
				(*s).fadeStop			= true;
				(*s).fadeTime			= fadeTimeInSeconds;
				(*s).fadeCurrentTime	= 0.0f;
				(*s).fadeVolume			= AIL_stream_volume( (HSTREAM)(*s).hStream );
				break;
			}
		}
	}
	else
	{
		return StopStream( streamID );
	}

	return true;
}

// Stop all sounds
void SoundManager::StopAllSounds( bool bUpdate, bool bExpireFiles )
{
	if( !m_bEnabled )
	{
		return;
	}

	// Request end to all samples
	DWORD numSamples	= m_numSamples + m_num3DSamples;
	for( DWORD i = 0; i < numSamples; ++i )
	{
		StopSample( i );
	}

	// Request end to all streams
	for( StreamCache::iterator s = m_StreamCache.begin(); s != m_StreamCache.end(); ++s )
	{
		StopStream( (DWORD)(*s).hStream );
	}

	if( bUpdate )
	{
		if( bExpireFiles )
		{
			// Save the old expiration time and set it to 0.0
			float savedExpiration	= m_sampleFileExpiration;
			m_sampleFileExpiration	= 0.0f;

			// Update
			Update( 0.0f );

			// Reset expiration times
			m_sampleFileExpiration	= savedExpiration;
		}
		else
		{
			// Update
			Update( 0.0f );
		}
	}
}

// Pause all sounds
void SoundManager::PauseAllSounds()
{
	if( !m_bEnabled )
	{
		return;
	}

	// Request end to all samples
	for( DWORD i = 0; i < m_numSamples; ++i )
	{
		if( IsSamplePlaying( m_pSamples[ i ].sampleIndex ) )
		{
			AIL_register_EOS_callback( m_pSamples[ i ].hSample, NULL );
			AIL_stop_sample( m_pSamples[ i ].hSample );
		}
	}

	for( i = 0; i < m_num3DSamples; ++i )
	{
		if( IsSamplePlaying( m_p3DSamples[ i ].sampleIndex ) )
		{
			AIL_register_3D_EOS_callback( m_p3DSamples[ i ].hSample, NULL );
			AIL_stop_3D_sample( m_p3DSamples[ i ].hSample );
		}
	}

	// Request end to all streams
	for( StreamCache::iterator s = m_StreamCache.begin(); s != m_StreamCache.end(); ++s )
	{
		AIL_pause_stream( (*s).hStream, 1 );
	}
}

// Stop all sounds after fading
void SoundManager::FadeStopAllSounds( float fadeTime )
{
	if( !m_bEnabled )
	{
		return;
	}

	if( m_hTimer )
	{
		// Release the timer handle
		AIL_release_timer_handle( m_hTimer );
		m_hTimer		= 0;

		// Mark all fades as completed
		m_bFadeResuming	= false;
		m_bFadeStopping	= false;
		m_bFadePausing	= false;

		// Signify that the timer has been released
		m_bReleaseTimer	= false;

		// Correct fade time
		m_userData		= FTOL( fadeTime * (SOUND_TIMER - ((float)m_userData / m_fadeTime)) );
	}
	else
	{
		m_userData		= FTOL( fadeTime * SOUND_TIMER );
	}

	// Create a timer
	m_hTimer			= AIL_register_timer( FadeStopCallback );
	m_fadeTime			= fadeTime;
	m_bFadeStopping		= true;

	// Register the number
	AIL_set_timer_user( m_hTimer, m_userData );

	// Setup callback frequency
	AIL_set_timer_period( m_hTimer, FTOL( 1000000.0f / SOUND_TIMER ) );

	// Start the timer
	AIL_start_timer( m_hTimer );
}

// Pause all sounds after fading
void SoundManager::FadePauseAllSounds( float fadeTime )
{
	if( !m_bEnabled )
	{
		return;
	}

	if( m_hTimer )
	{
		if( m_bFadeStopping )
		{
			gperror( "Cannot pause sounds after a request for all stop!" );
			return;
		}

		// Release the timer handle
		AIL_release_timer_handle( m_hTimer );
		m_hTimer		= 0;

		// Mark all fades as completed
		m_bFadeResuming	= false;
		m_bFadeStopping	= false;
		m_bFadePausing	= false;

		// Signify that the timer has been released
		m_bReleaseTimer	= false;

		// Correct fade time
		m_userData		= FTOL( fadeTime * (SOUND_TIMER - ((float)m_userData / m_fadeTime)) );
	}
	else
	{
		m_userData		= FTOL( fadeTime * SOUND_TIMER );
	}

	// Request pause on all voice streams
	for( StreamCache::iterator s = m_StreamCache.begin(); s != m_StreamCache.end(); ++s )
	{
		if( (*s).streamType == STRM_VOICE )
		{
			AIL_pause_stream( (*s).hStream, 1 );
		}
	}

	// Create a timer
	m_hTimer			= AIL_register_timer( FadePauseCallback );
	m_fadeTime			= fadeTime;
	m_bFadePausing		= true;

	// Register the number
	AIL_set_timer_user( m_hTimer, m_userData );

	// Setup callback frequency
	AIL_set_timer_period( m_hTimer, FTOL( 1000000.0f / SOUND_TIMER ) );

	// Start the timer
	AIL_start_timer( m_hTimer );
}

// Resume sounds
void SoundManager::ResumeAllSounds()
{
	if( !m_bEnabled )
	{
		return;
	}

	// Force update to clean everything up
	Update( 0.0f );

	// Request end to all samples
	for( DWORD i = 0; i < m_numSamples; ++i )
	{
		if( IsSamplePlaying( m_pSamples[ i ].sampleIndex ) )
		{
			AIL_register_EOS_callback( m_pSamples[ i ].hSample, EndOfSampleCallback );
			AIL_resume_sample( m_pSamples[ i ].hSample );
		}
	}

	for( i = 0; i < m_num3DSamples; ++i )
	{
		if( IsSamplePlaying( m_p3DSamples[ i ].sampleIndex ) )
		{
			AIL_register_3D_EOS_callback( m_p3DSamples[ i ].hSample, EndOf3DSampleCallback );
			AIL_resume_3D_sample( m_p3DSamples[ i ].hSample );
		}
	}

	// Request end to all streams
	for( StreamCache::iterator s = m_StreamCache.begin(); s != m_StreamCache.end(); ++s )
	{
		AIL_pause_stream( (*s).hStream, 0 );
	}
}

// Resume sounds
void SoundManager::FadeResumeAllSounds( float fadeTime )
{
	if( !m_bEnabled )
	{
		return;
	}

	if( m_hTimer )
	{
		if( m_bFadeStopping )
		{
			gperror( "Cannot resume sounds after a request for all stop!" );
			return;
		}

		// Release the timer handle
		AIL_release_timer_handle( m_hTimer );
		m_hTimer			= 0;

		if( m_bFadePausing )
		{
			// Request pause on all voice streams
			for( StreamCache::iterator s = m_StreamCache.begin(); s != m_StreamCache.end(); ++s )
			{
				if( (*s).streamType == STRM_VOICE )
				{
					AIL_pause_stream( (*s).hStream, 0 );
				}
			}
		}

		// Mark all fades as completed
		m_bFadeResuming		= false;
		m_bFadeStopping		= false;
		m_bFadePausing		= false;
		m_bStopAllSounds	= false;
		m_bPauseAllSounds	= false;

		// Signify that the timer has been released
		m_bReleaseTimer		= false;

		// Correct fade time
		m_userData			= FTOL( fadeTime * (SOUND_TIMER - ((float)m_userData / m_fadeTime)) );
	}
	else
	{
		// Make sure volume is 0
		AIL_set_digital_master_volume( m_hDigitalDriver, 0 );
		for( unsigned int i = 0; i < m_num3DSamples; ++i )
		{
			AIL_set_3D_sample_volume( m_p3DSamples[ i ].hSample, 0 );
		}

		// Resume all playing sounds
		ResumeAllSounds();

		m_userData		= FTOL( fadeTime * SOUND_TIMER );
	}

	// Create a timer
	m_hTimer			= AIL_register_timer( FadeResumeCallback );
	m_fadeTime			= fadeTime;
	m_bFadeResuming		= true;

	// Register the number
	AIL_set_timer_user( m_hTimer, m_userData );

	// Setup callback frequency
	AIL_set_timer_period( m_hTimer, FTOL( 1000000.0f / SOUND_TIMER ) );

	// Start the timer
	AIL_start_timer( m_hTimer );
}

// Set the position of a sound
bool SoundManager::SetSamplePosition( DWORD sampleID, const vector_3& position )
{
	if( !m_bEnabled )
	{
		return false;
	}

	// Make sure this is a 3D sound
	if( (sampleID < m_numSamples) || (sampleID == INVALID_SOUND_ID) )
	{
		return false;
	}

	// Set the position of this sample
	gpassert( m_p3DSamples[ sampleID - m_numSamples ].sampleIndex == sampleID );
	AIL_set_3D_position( m_p3DSamples[ sampleID - m_numSamples ].hSample, position.x, position.y, -position.z );

	// Set the orientation to point at the listener
	matrix_3x3 orient;
	vector_3 listenerDir	= m_listenerPos - position;
	if( !listenerDir.IsZero() )
	{
		orient = MatrixFromDirection( m_listenerPos - position );
	}

	AIL_set_3D_orientation( m_p3DSamples[ sampleID - m_numSamples ].hSample,
							orient.v02, orient.v12, -orient.v22,
							orient.v01, orient.v11, -orient.v21 );

	return true;
}

// Set a stop sample callback
void SoundManager::SetSampleStopCallback( DWORD sampleID, SampleStopCallback callback )
{
	if( sampleID == INVALID_SOUND_ID )
	{
		return;
	}

	StopSampleCallbackMap::iterator i = m_stopSampleCallbackMap.find( sampleID );
	if( i == m_stopSampleCallbackMap.end() )
	{
		// Put this callback into our mapping
		if( IsSamplePlaying( sampleID ) )
		{
			m_stopSampleCallbackMap.insert( std::pair< DWORD, SampleStopCallback >( sampleID, callback ) );
		}
	}
}

// Clear a stop sample callback
void SoundManager::ClearSampleStopCallback( DWORD sampleID )
{
	m_stopSampleCallbackMap.erase( sampleID );
}

// Set a stop stream callback
void SoundManager::SetStreamStopCallback( DWORD streamID, StreamStopCallback callback )
{
	if( streamID == INVALID_SOUND_ID )
	{
		return;
	}

	StopStreamCallbackMap::iterator i = m_stopStreamCallbackMap.find( streamID );
	if( i == m_stopStreamCallbackMap.end() )
	{
		// Put this callback into our mapping
		if( IsStreamPlaying( streamID ) )
		{
			m_stopStreamCallbackMap.insert( std::pair< DWORD, StreamStopCallback >( streamID, callback ) );
		}
	}
}

// Clear a stop sample callback
void SoundManager::ClearStreamStopCallback( DWORD streamID )
{
	m_stopStreamCallbackMap.erase( streamID );
}

// Offset information
DWORD SoundManager::GetSampleOffset( DWORD sampleID )
{
	if( !m_bEnabled )
	{
		return 0;
	}

	if( IsSamplePlaying( sampleID ) )
	{
		if( sampleID < m_numSamples )
		{
			return( AIL_sample_position( m_pSamples[ sampleID ].hSample ) );
		}
		else
		{
			gpassert( m_p3DSamples[ sampleID - m_numSamples ].sampleIndex == sampleID );
			return( AIL_3D_sample_offset( m_p3DSamples[ sampleID - m_numSamples ].hSample ) );
		}
	}

	return 0;
}

// Retrieve master volume
DWORD SoundManager::GetMasterVolume()
{
	return m_masterVolume;
}

// Retrieve sample volume
DWORD SoundManager::GetSampleVolume( eSampleType type )
{
	switch( type )
	{
		case SND_EFFECT_LOW:
		{
			return m_lowSampleVolume;
		}

		case SND_EFFECT_NORM:
		{
			return m_normSampleVolume;
		}

		case SND_EFFECT_HIGH:
		{
			return m_highSampleVolume;
		}

		case SND_EFFECT_AMBIENT:
		{
			return m_ambientSampleVolume;
		}

		default:
		{
			gperror( "Unrecognized sample type!" );
			return 0;
		}
	};
}

// Retrieve stream volume
DWORD SoundManager::GetStreamVolume( eStreamType type )
{
	switch( type )
	{
		case STRM_AMBIENT:
		{
			return m_ambientStreamVolume;
		}

		case STRM_VOICE:
		{
			return m_voiceStreamVolume;
		}

		case STRM_MUSIC:
		{
			return m_musicStreamVolume;
		}

		default:
		{
			gperror( "Unrecognized stream type!" );
			return 0;
		}
	};
}

// Master volume
bool SoundManager::SetMasterVolume( DWORD volume )
{
	m_masterVolume	= volume;

	if( m_bEnabled )
	{
		// Set the volume for the active digital sound driver
		AIL_set_digital_master_volume( m_hDigitalDriver, volume );
	}

	return true;
}

// Volume control
bool SoundManager::SetSampleVolume( eSampleType type, DWORD volume )
{
	switch( type )
	{
		case SND_EFFECT_LOW:
		{
			m_lowSampleVolume		= volume;
			break;
		}

		case SND_EFFECT_NORM:
		{
			m_normSampleVolume		= volume;
			break;
		}

		case SND_EFFECT_HIGH:
		{
			m_highSampleVolume		= volume;
			break;
		}

		case SND_EFFECT_AMBIENT:
		{
			m_ambientSampleVolume	= volume;
			break;
		}

		default:
		{
			gperror( "Unrecognized sample type!" );
			return false;
		}
	};

	// Go through all open samples and update their volumes
	for( unsigned int i = 0; i < m_numSamples; ++i )
	{
		if( m_pSamples[ i ].sampleType == type &&
			IsSamplePlaying( m_pSamples[ i ].sampleIndex ) )
		{
			AIL_set_sample_volume( m_pSamples[ i ].hSample, volume );
		}
	}

	// Go through all open 3D samples and update their volumes
	for( i = 0; i < m_num3DSamples; ++i )
	{
		if( m_p3DSamples[ i ].sampleType == type &&
			IsSamplePlaying( m_p3DSamples[ i ].sampleIndex ) )
		{
			AIL_set_3D_sample_volume( m_p3DSamples[ i ].hSample, volume );
		}
	}

	return true;
}

// Volume control
bool SoundManager::SetSampleVolume( DWORD sampleID, DWORD volume )
{
	if( !m_bEnabled || sampleID == INVALID_SOUND_ID )
	{
		return false;
	}

	// Set the volume of this individual sample
	if( IsSamplePlaying( sampleID ) )
	{
		if( sampleID < m_numSamples )
		{
			AIL_set_sample_volume( m_pSamples[ sampleID ].hSample, volume );
		}
		else
		{
			gpassert( m_p3DSamples[ sampleID - m_numSamples ].sampleIndex == sampleID );
			AIL_set_3D_sample_volume( m_p3DSamples[ sampleID - m_numSamples ].hSample, volume );
		}
	}

	return true;
}

// Stream volume control
bool SoundManager::SetStreamVolume( eStreamType type, DWORD volume )
{
	switch( type )
	{
		case STRM_AMBIENT:
		{
			m_ambientStreamVolume	= volume;
			break;
		}

		case STRM_VOICE:
		{
			m_voiceStreamVolume		= volume;
			break;
		}

		case STRM_MUSIC:
		{
			m_musicStreamVolume		= volume;
			break;
		}

		default:
		{
			gperror( "Unrecognized stream type!" );
			return false;
		}
	};

	// Go through all open streams and update their volumes
	for( StreamCache::iterator i = m_StreamCache.begin(); i != m_StreamCache.end(); ++i )
	{
		if( (*i).streamType == type )
		{
			AIL_set_stream_volume( (*i).hStream, volume );
		}
	}

	return true;
}

// Stream volume control
bool SoundManager::SetStreamVolume( DWORD streamID, DWORD volume )
{
	if( !m_bEnabled || streamID == INVALID_SOUND_ID )
	{
		return false;
	}
	
	// Set the volume of this individual sample
	if( IsStreamPlaying( streamID ) )
	{
		AIL_set_stream_volume( (HSTREAM)streamID, volume );
	}

	return true;
}

// Is this sound playing?
bool SoundManager::IsSamplePlaying( DWORD sampleID )
{
	if( !m_bEnabled || sampleID == INVALID_SOUND_ID )
	{
		return false;
	}

	// See if this sound is playing
	if( sampleID < m_numSamples )
	{
		DWORD status = AIL_sample_status( m_pSamples[ sampleID ].hSample );
		if( (status == SMP_PLAYING) || (status == SMP_STOPPED) )
		{
			return true;
		}
	}
	else
	{
		gpassert( m_p3DSamples[ sampleID - m_numSamples ].sampleIndex == sampleID );
		DWORD status = AIL_3D_sample_status( m_p3DSamples[ sampleID - m_numSamples ].hSample );
		if( (status == SMP_PLAYING) || (status == SMP_STOPPED) )
		{
			return true;
		}
	}
	
	return false;
}

// Is this stream playing?
bool SoundManager::IsStreamPlaying( DWORD streamID )
{
	if( !m_bEnabled || streamID == INVALID_SOUND_ID )
	{
		return false;
	}

	// See if this stream is playing
	for( StreamCache::iterator c = m_StreamCache.begin(); c != m_StreamCache.end(); ++c )
	{
		if( streamID == (DWORD)(*c).hStream )
		{
			DWORD status = AIL_stream_status( (HSTREAM)streamID );
			if( (status == SMP_PLAYING) ||
				(status == SMP_STOPPED) ||
				(status == SMP_DONE) )
			{
				return true;
			}
		}
	}
	
	return false;
}

bool SoundManager::Validate3DSoundProvider( HPROVIDER requested3Dprovider )
{
	if( !m_bEnabled )
	{
		return false;
	}

	bool bSuccess = (AIL_open_3D_provider( requested3Dprovider ) == 0);
	if( bSuccess )
	{
		AIL_close_3D_provider( requested3Dprovider );
	}

	return bSuccess;
}

// Create sound descriptor
DWORD SoundManager::CreateSoundDescriptor( const gpstring& descriptorName )
{
	SoundDescriptor sed;
	gpstring fileName;

	// Set our reference count
	sed.refCount	= 1;

	// Open the fuel block to the sound information
	FastFuelHandle hSound( FilePathToFuelAddress( descriptorName ) );
	if( hSound.IsValid() )
	{
		// Sound descriptor block has been found
		gpstring soundName;
		hSound.Get( "sound_effect_file", soundName );
		gNamingKey.BuildContentLocation( soundName, fileName );

		// Add .wav extension
		fileName	+= ".wav";

		// Read in default descriptor data
		sed.minPlaybackRate	= hSound.GetFloat( "min_playback_rate", 1.0f );
		sed.maxPlaybackRate = hSound.GetFloat( "max_playback_rate", 1.0f );
		sed.minOverrideDist	= hSound.GetFloat( "min_override_dist", -1.0f );
		sed.maxOverrideDist	= hSound.GetFloat( "max_override_dist", -1.0f );
		sed.maxSamples		= (DWORD)hSound.GetInt( "max_simultaneous_samples", -1 );
	}
	else
	{
		// No descriptor was found, so treat this as a basic sound file
		fileName	= descriptorName + ".wav";

		// Setup defaults
		sed.minPlaybackRate	= 1.0f;
		sed.maxPlaybackRate	= 1.0f;
		sed.minOverrideDist	= -1.0f;
		sed.maxOverrideDist	= -1.0f;
		sed.maxSamples		= 0xFFFFFFFF;
	}

	// See if this file is already open
	SampleFileNameMap::iterator f = m_SampleFileNameMap.find( fileName );
	if( f != m_SampleFileNameMap.end() )
	{
		gpassert( m_SampleFileCache.IsValid( (*f).second ) );

		// Take a reference to this existing file
		sed.fileIndex	= (*f).second;
		m_SampleFileCache.Dereference( (*f).second ).refCount++;
	}
	else
	{
		// Open the file
		FilePair filePair;

		// Attempt to open the requested sound file
		if( !filePair.fileHandle.Open( fileName ) )
		{
			gperrorf(( "Could not open sound file %s", fileName.c_str() ));
			return 0;
		}

		if( !filePair.memHandle.Map( filePair.fileHandle ) )
		{
			gperrorf(( "Could not memory map sound file %s", fileName.c_str() ));
			filePair.fileHandle.Close();
			return 0;
		}

		// Touch the memory
		filePair.memHandle.Touch();

#if GP_DEBUG

		// Get the information about this wav file
		AILSOUNDINFO soundInfo;
		AIL_WAV_info( filePair.memHandle.GetData(), &soundInfo );

		// Assert that we have what we expect
		gpassert( soundInfo.format == WAVE_FORMAT_PCM );
		gpassert( soundInfo.data_len <= (DWORD)filePair.memHandle.GetSize() );

		if( ((char*)filePair.memHandle.GetData() + filePair.memHandle.GetSize()) !=
			((char*)soundInfo.data_ptr + soundInfo.data_len) )
		{
			gperrorf(( "Sound file '%s' has extraneous data (%d bytes) on the end, please remove!", fileName.c_str(),
					((char*)filePair.memHandle.GetData() + filePair.memHandle.GetSize()) - ((char*)soundInfo.data_ptr + soundInfo.data_len) ));
		}

#endif

		// Put this new handle into our file cache
		filePair.refCount	= 1;
		DWORD fileID		= m_SampleFileCache.Alloc( filePair );
		sed.fileIndex		= fileID;

		// Put the name onto our file name mapping
		m_SampleFileNameMap.insert( std::pair< gpstring, DWORD >( fileName, fileID ) );
		gpsoundf(( "Opened sample file: %s\n", fileName.c_str() ));
	}

	// Allocate a new space for this descriptor
	DWORD sedIndex	= m_SampleDescriptorCache.Alloc( sed );
	m_SampleDescriptorNameMap.insert( std::pair< gpstring, DWORD >( descriptorName, sedIndex ) );

#if !GP_RETAIL

	// Increment the history count for this sample
	SampleFileNameMap::iterator h = m_SampleHistoryMap.find( descriptorName );
	if( h != m_SampleHistoryMap.end() )
	{
		(*h).second++;
	}
	else
	{
		m_SampleHistoryMap.insert( std::pair< gpstring, DWORD >( descriptorName, 1 ) );
	}

#endif

	return sedIndex;
}

// Release sound descriptor
void SoundManager::ReleaseSoundDescriptor( DWORD sedIndex )
{
	gpassert( m_SampleDescriptorCache.IsValid( sedIndex ) );

	// Get the descriptor that we're interested in
	if( m_SampleDescriptorCache.IsValid( sedIndex ) )
	{
		SoundDescriptor& sed	= m_SampleDescriptorCache.Dereference( sedIndex );

		// Reduce the reference count
		sed.refCount--;
		if( sed.refCount == 0 )
		{
			gpassert( m_SampleFileCache.IsValid( sed.fileIndex ) );

			// Release file reference
			if( m_SampleFileCache.IsValid( sed.fileIndex ) )
			{
				FilePair& filePair	= m_SampleFileCache.Dereference( sed.fileIndex );

				// Update the reference counts of the file pair
				filePair.refCount--;

				if( filePair.refCount == 0 )
				{
					filePair.lastUse = 0.0f;
				}
			}

			// Release from name mapping
			for( SampleFileNameMap::iterator i = m_SampleDescriptorNameMap.begin(); i != m_SampleDescriptorNameMap.end(); ++i )
			{
				if( (*i).second == sedIndex )
				{
					m_SampleDescriptorNameMap.erase( i );
					break;
				}
			}

			// Release from descriptor mapping
			m_SampleDescriptorCache.Free( sedIndex );
		}
	}
}

// Get a free sample handle
SampleInfo* SoundManager::GetFreeSampleHandle( eSampleType type )
{
	// Lowest priority, furthest along sound
	long currentTimeLeft	= INT_MAX;
	SampleInfo* pSample		= NULL;
	eSampleType currentType	= type;

	// Go through all the samples
	for( unsigned int i = 0; i < m_numSamples; ++i )
	{
		DWORD status	= AIL_sample_status( m_pSamples[ i ].hSample );
		if( status == SMP_DONE && m_pSamples[ i ].bClosed )
		{
			// Setup the sample type and return the sample handle
			m_pSamples[ i ].sampleType	= type;
			return &m_pSamples[ i ];
		}
		else if( status == SMP_PLAYING )
		{
			// Make sure that the sound priority is equal to or lower than requested
			if( m_pSamples[ i ].sampleType <= currentType )
			{
				// Get the current play time of the sample in question
				long total_time, current_time;
				AIL_sample_ms_position( m_pSamples[ i ].hSample, &total_time, &current_time );

				// Calculate it percentage of completion
				long time_left = total_time - current_time;
				if( time_left < currentTimeLeft )
				{
					// New replacement sample
					pSample			= &m_pSamples[ i ];
					currentTimeLeft	= time_left;
					currentType		= pSample->sampleType;
				}
			}
		}
	}

	// Make sure we have a valid sample
	if( pSample )
	{
		// Stop playing this samples since we know it was playing
		StopSampleImmediately( pSample->sampleIndex );

		// Set the new sample type
		pSample->sampleType	= type;

		// Return the valid sample
		return pSample;
	}

	return NULL;
}

// Get a free 3D sample handle
Sample3DInfo* SoundManager::GetFree3DSampleHandle( eSampleType type )
{
	// Lowest priority, furthest along sound
	long currentTimeLeft	= INT_MAX;
	Sample3DInfo* pSample	= NULL;
	eSampleType currentType	= type;

	// Go through all the samples
	for( unsigned int i = 0; i < m_num3DSamples; ++i )
	{
		DWORD status	= AIL_3D_sample_status( m_p3DSamples[ i ].hSample );
		if( status == SMP_DONE && m_p3DSamples[ i ].bClosed )
		{
			// Setup the sample type and return the sample handle
			m_p3DSamples[ i ].sampleType	= type;
			return &m_p3DSamples[ i ];
		}
		else if( status == SMP_PLAYING )
		{
			// Make sure that the sound priority is equal to or lower than requested
			if( m_p3DSamples[ i ].sampleType <= currentType )
			{
				// Get the current play time of the sample in question
				long current_time	= AIL_3D_sample_offset( m_p3DSamples[ i ].hSample );
				long total_time		= AIL_3D_sample_length( m_p3DSamples[ i ].hSample );

				// Calculate it percentage of completion
				long time_left = total_time - current_time;
				if( time_left < currentTimeLeft )
				{
					// New replacement sample
					pSample			= &m_p3DSamples[ i ];
					currentTimeLeft	= time_left;
					currentType		= pSample->sampleType;
				}
			}
		}
	}

	// Make sure we have a valid sample
	if( pSample )
	{
		// Stop playing this samples since we know it was playing
		StopSampleImmediately( pSample->sampleIndex );

		// Set the new sample type
		pSample->sampleType	= type;

		// Return the valid sample
		return pSample;
	}

	return NULL;
}

// Stop the sample
bool SoundManager::StopSample( HSAMPLE hSample )
{
	gpassert( m_bEnabled );

	// Lock the sound critical
	kerneltool::Critical::Lock autoLock( m_soundCritical );

	// Put this sample on our 2D sample stop request list
	m_sample2DStopRequest.insert( hSample );
	return true;
}

// Stop the sample
bool SoundManager::StopSample( H3DSAMPLE hSample )
{
	gpassert( m_bEnabled );

	// Lock the sound critical
	kerneltool::Critical::Lock autoLock( m_soundCritical );

	// Put this sample on our 3D sample stop request list
	m_sample3DStopRequest.insert( hSample );
	return true;
}

// Stop the sample immediately
bool SoundManager::StopSampleImmediately( DWORD sampleID )
{
	DWORD sedIndex	= 0;

	// Request a stop of this sample
	if( sampleID < m_numSamples )
	{
		if( !m_pSamples[ sampleID ].bClosed )
		{
			// Stop the sample
			AIL_end_sample( m_pSamples[ sampleID ].hSample );

			// Mark the sample as closed
			sedIndex	= m_pSamples[ sampleID ].sedIndex;
			m_pSamples[ sampleID ].bClosed	= true;

			{
				// Lock the sound critical
				kerneltool::Critical::Lock autoLock( m_soundCritical );

				// Make sure this sample is not waiting to be stopped
				for( Stop2DSampleRequestColl::iterator r = m_sample2DStopRequest.begin(); r != m_sample2DStopRequest.end(); ++r )
				{
					if( m_pSamples[ sampleID ].hSample == (*r) )
					{
						m_sample2DStopRequest.erase( r );
						break;
					}
				}
			}

			gpsoundf(( "Sample immediately closed ID: %d\n", sampleID ));
		}
		else
		{
			return true;
		}
	}
	else
	{
		DWORD sampleIndex	= sampleID - m_numSamples;
		gpassert( m_p3DSamples[ sampleIndex ].sampleIndex == sampleID );
		if( !m_p3DSamples[ sampleIndex ].bClosed )
		{
			// Stop the sample
			AIL_end_3D_sample( m_p3DSamples[ sampleIndex ].hSample );

			// Mark the sample as closed
			gpassert( m_p3DSamples[ sampleIndex ].sampleIndex == sampleID );
			sedIndex	= m_p3DSamples[ sampleIndex ].sedIndex;
			m_p3DSamples[ sampleIndex ].bClosed	= true;

			{
				// Lock the sound critical
				kerneltool::Critical::Lock autoLock( m_soundCritical );

				// Make sure this sample is not waiting to be stopped
				for( Stop3DSampleRequestColl::iterator r = m_sample3DStopRequest.begin(); r != m_sample3DStopRequest.end(); ++r )
				{
					if( m_p3DSamples[ sampleIndex ].hSample == (*r) )
					{
						m_sample3DStopRequest.erase( r );
						break;
					}
				}
			}

			gpsoundf(( "3D sample immediately closed ID: %d\n", sampleID ));
		}
		else
		{
			return true;
		}
	}

	// Callback if necessary
	StopSampleCallbackMap::iterator c = m_stopSampleCallbackMap.find( sampleID );
	if( c != m_stopSampleCallbackMap.end() )
	{
		(*c).second( sampleID );

		// Erase the callback
		m_stopSampleCallbackMap.erase( c );
	}

	// Clean up sound descriptor
	ReleaseSoundDescriptor( sedIndex );
	return true;
}

// Memory allocation callback
void* WINAPI SoundManager::AllocateCallback( DWORD memAmount )
{
	return gpmalloc( memAmount );
}

// Memory free callback
void WINAPI SoundManager::FreeCallback( void* memPtr )
{
	gpfree( memPtr );
}

// File open callback
DWORD WINAPI SoundManager::FileOpenCallback( char const* pFileName, DWORD* pHandle )
{
	// Open the requested file
	FileSys::FileHandle* pFileHandle	= new FileSys::FileHandle;

	// Open file
	if( !pFileHandle->Open( pFileName ) )
	{
		delete pFileHandle;
		(*pHandle)	= 0;
		return 0;
	}

	(*pHandle)	= (DWORD)pFileHandle;
	return 1;
}

// File close callback
void WINAPI SoundManager::FileCloseCallback( DWORD hFile )
{
	// Close the requested file
	FileSys::FileHandle* pFileHandle	= (FileSys::FileHandle*)hFile;

	if( pFileHandle )
	{
		if( !pFileHandle->Close() )
		{
			gperror( "Could not close!" );
		}
		delete pFileHandle;
	}
}

// File seek callback
long WINAPI SoundManager::FileSeekCallback( DWORD hFile, long offset, DWORD type )
{
	// Get the file handle
	FileSys::FileHandle* pFileHandle	= (FileSys::FileHandle*)hFile;
	if( pFileHandle )
	{
		// Seek through the file
		FileSys::eSeekFrom seekFrom = FileSys::SEEKFROM_BEGIN;
		switch( type )
		{
		case AIL_FILE_SEEK_BEGIN:
			seekFrom	= FileSys::SEEKFROM_BEGIN;
			break;

		case AIL_FILE_SEEK_CURRENT:
			seekFrom	= FileSys::SEEKFROM_CURRENT;
			break;

		case AIL_FILE_SEEK_END:
			seekFrom	= FileSys::SEEKFROM_END;
			break;
		}

		// Perform seek and return current position, regardless of success
		if( !pFileHandle->Seek( offset, seekFrom ) )
		{
			gperror( "Could not seek!" );
		}
		return pFileHandle->GetPos();
	}
	
	return 0;
}

// File read callback
DWORD WINAPI SoundManager::FileReadCallback( DWORD hFile, void* pBuffer, DWORD bytesToRead )
{
	// Get the file handle
	FileSys::FileHandle* pFileHandle	= (FileSys::FileHandle*)hFile;

	int bytesRead	= 0;
	if( pFileHandle )
	{
		// Read the data
		if( !pFileHandle->Read( pBuffer, bytesToRead, &bytesRead ) )
		{
			gperror( "Could not read!" );
		}
	}

	// Return actual bytes read
	return bytesRead;
}

// End of sample callback
void WINAPI SoundManager::EndOfSampleCallback( HSAMPLE hSample )
{
	// Call stop
	gSoundManager.StopSample( hSample );
}

// End of 3D sample callback
void WINAPI SoundManager::EndOf3DSampleCallback( H3DSAMPLE hSample )
{
	// Call stop
	gSoundManager.StopSample( hSample );
}

// End of stream callback
void WINAPI SoundManager::EndOfStreamCallback( HSTREAM hStream )
{
	// Call stop
	gSoundManager.StopStream( (DWORD)hStream );
}

// Timer callback
void WINAPI SoundManager::FadeStopCallback( DWORD info )
{
	// See if we are done
	if( !info )
	{
		// Signify to the main thread that we wish to release the timer and stop all sounds
		gSoundManager.m_bReleaseTimer	= true;
		gSoundManager.m_bStopAllSounds	= true;
	}
	else
	{
		// Calculate our previous blend amount
		float prevblend	= ((float)info / SOUND_TIMER) / gSoundManager.m_fadeTime;

		// Decrement call count
		--info;

		// Calculate blend amount
		float blend	= ((float)info / SOUND_TIMER) / gSoundManager.m_fadeTime;

		// Calculate new volume
		AIL_set_digital_master_volume( gSoundManager.m_hDigitalDriver, FTOL( (float)gSoundManager.m_masterVolume * blend ) );
		for( unsigned int i = 0; i < gSoundManager.m_num3DSamples; ++i )
		{
			AIL_set_3D_sample_volume( gSoundManager.m_p3DSamples[ i ].hSample,
									  FTOL( ((float)AIL_3D_sample_volume( gSoundManager.m_p3DSamples[ i ].hSample ) / prevblend) * blend ) );
		}

		// Set new call count
		AIL_set_timer_user( gSoundManager.m_hTimer, info );
		gSoundManager.m_userData = info;
	}
}

// Timer callback
void WINAPI SoundManager::FadePauseCallback( DWORD info )
{
	// See if we are done
	if( !info )
	{
		// Signify to the main thread that we wish to release the timer and pause all sounds
		gSoundManager.m_bReleaseTimer	= true;
		gSoundManager.m_bPauseAllSounds	= true;
	}
	else
	{
		// Calculate our previous blend amount
		float prevblend	= ((float)info / SOUND_TIMER) / gSoundManager.m_fadeTime;

		// Decrement call count
		--info;

		// Calculate blend amount
		float blend	= ((float)info / SOUND_TIMER) / gSoundManager.m_fadeTime;

		// Calculate new volume
		AIL_set_digital_master_volume( gSoundManager.m_hDigitalDriver, FTOL( (float)gSoundManager.m_masterVolume * blend ) );
		for( unsigned int i = 0; i < gSoundManager.m_num3DSamples; ++i )
		{
			AIL_set_3D_sample_volume( gSoundManager.m_p3DSamples[ i ].hSample,
									  FTOL( ((float)AIL_3D_sample_volume( gSoundManager.m_p3DSamples[ i ].hSample ) / prevblend) * blend ) );
		}

		// Set new call count
		AIL_set_timer_user( gSoundManager.m_hTimer, info );
		gSoundManager.m_userData = info;
	}
}

void WINAPI SoundManager::FadeResumeCallback( DWORD info )
{
	// See if we are done
	if( !info )
	{
		// Signify to the main thread that we wish to release the timer
		gSoundManager.m_bReleaseTimer	= true;
	}
	else
	{
		// Decrement call count
		--info;

		// Calculate blend amount
		float blend		= 1.0f - (((float)info / SOUND_TIMER) / gSoundManager.m_fadeTime);

		// Calculate new volume
		AIL_set_digital_master_volume( gSoundManager.m_hDigitalDriver, FTOL( (float)gSoundManager.m_masterVolume * blend ) );
		for( unsigned int i = 0; i < gSoundManager.m_num3DSamples; ++i )
		{
			DWORD blendVolume	= 0;

			switch( gSoundManager.m_p3DSamples[ i ].sampleType )
			{
				case SND_EFFECT_LOW:
				{
					blendVolume	= gSoundManager.m_lowSampleVolume;
					break;
				}

				case SND_EFFECT_NORM:
				{
					blendVolume	= gSoundManager.m_normSampleVolume;
					break;
				}

				case SND_EFFECT_HIGH:
				{
					blendVolume	= gSoundManager.m_highSampleVolume;
					break;
				}

				case SND_EFFECT_AMBIENT:
				{
					blendVolume	= gSoundManager.m_ambientSampleVolume;
					break;
				}
			};

			AIL_set_3D_sample_volume( gSoundManager.m_p3DSamples[ i ].hSample, FTOL( (float)blendVolume * blend ) );
		}

		// Set new call count
		AIL_set_timer_user( gSoundManager.m_hTimer, info );
		gSoundManager.m_userData = info;
	}
}

