#pragma once
#ifndef _GPS_MANAGER_H_
#define _GPS_MANAGER_H_

/*******************************************************************************************
**
**									GPGSound Manager
**
**		Central location for external interfacing to the GPGSound library.  Provides
**		a shell interface for Miles Sound System.
**
**		Author:		James Loe
**		Date:		01/25/01
**
********************************************************************************************/

// STL includes
#include <map>
#include <vector>

// Core includes
#include "gpstring.h"
#include "bucketvector.h"
#include "filesys.h"
#include "blindcallback.h"
#include "kerneltool.h"

// Math includes
#include "vector_3.h"

// Sound types
#include "gps_types.h"


// Forward declarations for MSS
struct			_DIG_DRIVER;
typedef struct	_DIG_DRIVER FAR *	HDIGDRIVER;

struct			_SAMPLE;
typedef struct	_SAMPLE FAR *		HSAMPLE;

struct			h3DPOBJECT;
typedef	struct	h3DPOBJECT FAR *	H3DPOBJECT;
typedef			H3DPOBJECT			H3DSAMPLE;

struct			_STREAM;
typedef struct	_STREAM FAR *		HSTREAM;

typedef			DWORD				HPROVIDER;
typedef			signed long			HTIMER;


namespace GPGSound
{
	// 3D sound provider information
	struct SoundProvider
	{
		char*				pProviderName;
		HPROVIDER			hProvider;
	};

	// Sample information
	struct SampleInfo
	{
		HSAMPLE				hSample;
		eSampleType			sampleType;
		DWORD				sedIndex;
		bool				bClosed;
		DWORD				sampleIndex;
	};

	// 3D sample information
	struct Sample3DInfo
	{
		H3DSAMPLE			hSample;
		eSampleType			sampleType;
		DWORD				sedIndex;
		bool				bClosed;
		DWORD				sampleIndex;
	};

	// Stream information
	struct StreamInfo
	{
		HSTREAM				hStream;
		eStreamType			streamType;

		bool				fadeStop;
		float				fadeTime;
		float				fadeCurrentTime;
		DWORD				fadeVolume;
	};

	// Sound effect descriptor
	struct SoundDescriptor
	{
		DWORD				refCount;
		DWORD				fileIndex;
		float				minPlaybackRate;
		float				maxPlaybackRate;
		float				minOverrideDist;
		float				maxOverrideDist;
		DWORD				maxSamples;
	};

	// File pair
	struct FilePair
	{
		FileSys::FileHandle	fileHandle;
		FileSys::MemHandle	memHandle;

		DWORD				refCount;
		float				lastUse;
	};

	// Typedefs for sound caches
	typedef UnsafeBucketVector< SoundDescriptor >		SampleDescriptorCache;
	typedef UnsafeBucketVector< FilePair >				SampleFileCache;
	typedef std::map< gpstring, DWORD, istring_less >	SampleFileNameMap;
	typedef std::map< eSampleType, DWORD >				SampleVolumeCache;
	typedef std::map< eStreamType, DWORD >				StreamVolumeCache;
	typedef std::vector< SoundProvider >				SoundProviderColl;
	typedef std::list< StreamInfo >						StreamCache;
	typedef CBFunctor1< DWORD >							SampleStopCallback;
	typedef std::map< DWORD, SampleStopCallback >		StopSampleCallbackMap;
	typedef stdx::linear_set< HSAMPLE >					Stop2DSampleRequestColl;
	typedef stdx::linear_set< H3DSAMPLE >				Stop3DSampleRequestColl;
	typedef CBFunctor1< DWORD >							StreamStopCallback;
	typedef std::map< DWORD, StreamStopCallback >		StopStreamCallbackMap;
	typedef stdx::linear_set< DWORD >					StopStreamRequestColl;


	// Manages the creation and caching of sounds
	class SoundManager : public Singleton< SoundManager >
	{
	public:

		// Construction and destruction
		SoundManager();
		~SoundManager();

		// Initialize DirectSound and prepare for playing
		bool						Initialize( DWORD numPrimaryChannels, DWORD primaryFreq, DWORD primaryBitRate,
												const gpstring& redistDir, const gpstring& requested3Dprovider, bool bEnable );

		// Timing
		void						SetSampleFileExpirationTime( float sExpire )	{ m_sampleFileExpiration = sExpire; }
		void						SetSoundLatency( DWORD msLatency );
		DWORD						GetSoundLatency()								{ return m_soundLatency * 8; }

		// Disable/enable the sound system
		bool						Enable( bool bEnable, bool bEnableEAX );

		// EAX
		bool						IsEAXAvailable()								{ return m_bEAXAvailable; }
		bool						IsEAXEnabled()									{ return m_bEAXEnabled; }

		// Speaker and room type control
		void						SetSpeakerType( eSpeakerType speakerType );
		eSpeakerType				GetSpeakerType()								{ return m_speakerType; }

		void						SetRoomType( eRoomType roomType );
		eRoomType					GetRoomType()									{ return m_roomType; }

		// Put the sound system into stasis
		void						SetStasis( bool bStasis )						{ m_bStasis = bStasis; }

		// Update file expiration
		void						Update( float deltaTime,
											const vector_3& listener_pos	= vector_3::ZERO,
											const vector_3& listener_face	= vector_3::ZERO,
											const vector_3& listener_up		= vector_3::ZERO );
		void						InactiveUpdate( bool bUpdate = true );

		// Play sound
		DWORD						PlaySample( const gpstring& soundFile, bool bLoop = false,
												eSampleType type = SND_EFFECT_NORM, DWORD offset = 0 );
		DWORD						PlaySample( const gpstring& soundFile, const vector_3& position, bool bLoop = false,
												const float min_dist = 15.0f, const float max_dist = 50.0f,
												eSampleType type = SND_EFFECT_NORM, DWORD offset = 0 );

		// Play stream
		DWORD						PlayStream( const gpstring& streamFile, eStreamType type, bool bLoop = false );
		DWORD						FadePlayStream( const gpstring& streamFile, eStreamType type, bool bLoop, float fadeTimeInSeconds );

		// Sound info
		float						GetSampleLengthInSeconds( const gpstring& soundFile );

		// Stop a sound
		bool						StopSample( DWORD sampleID );
		bool						StopStream( DWORD streamID );
		void						StopAllStreams();
		bool						FadeStopStream( DWORD streamID, float fadeTimeInSeconds );

		// Stop/Pause all sounds
		void						StopAllSounds( bool bUpdate = true, bool bExpireFiles = false );
		void						PauseAllSounds();

		// Fade then stop/pause all sounds
		void						FadeStopAllSounds( float fadeTime );
		void						FadePauseAllSounds( float fadeTime );

		// Resume sounds
		void						ResumeAllSounds();
		void						FadeResumeAllSounds( float fadeTime );

		// Set the position of a sound
		bool						SetSamplePosition( DWORD sampleID, const vector_3& position );

		// Set a stop sound callback
		void						SetSampleStopCallback( DWORD sampleID, SampleStopCallback callback );
		void						ClearSampleStopCallback( DWORD sampleID );
		void						SetStreamStopCallback( DWORD streamID, StreamStopCallback callback );
		void						ClearStreamStopCallback( DWORD streamID );

		// Offset information
		DWORD						GetSampleOffset( DWORD sampleID );

		// Retrieve volume information
		DWORD						GetMasterVolume();
		DWORD						GetSampleVolume( eSampleType type );
		DWORD						GetStreamVolume( eStreamType type );

		// Volume control (in the range 0 (mute) - 127 (max))
		bool						SetMasterVolume( DWORD volume );

		bool						SetSampleVolume( eSampleType type, DWORD volume );
		bool						SetSampleVolume( DWORD sampleID, DWORD volume );

		bool						SetStreamVolume( eStreamType type, DWORD volume );
		bool						SetStreamVolume( DWORD streamID, DWORD volume );

		// Is this sound playing?
		bool						IsSamplePlaying( DWORD sampleID );
		bool						IsStreamPlaying( DWORD streamID );

		// Retrieve a list of installed 3D sound providers
		const SoundProviderColl&	Get3DSoundProviders()							{ return m_SoundProviderColl; }
		HPROVIDER					GetActive3DSoundProvider()						{ return m_h3DProvider; }
		bool						Validate3DSoundProvider( HPROVIDER requested3Dprovider );

		// Enabled state
		bool						IsEnabled()										{ return m_bEnabled; }

		// Access to the digital driver handle
		HDIGDRIVER					GetDigitalDriver()								{ return m_hDigitalDriver; }

#if !GP_RETAIL

		const SampleFileNameMap&	GetSampleHistoryMap()							{ return m_SampleHistoryMap; }

#endif

	private:

		// Sound descriptors
		DWORD						CreateSoundDescriptor( const gpstring& descriptorName );
		void						ReleaseSoundDescriptor( DWORD sedIndex );

		// Get a free sample handle
		SampleInfo*					GetFreeSampleHandle( eSampleType type );
		Sample3DInfo*				GetFree3DSampleHandle( eSampleType type );

		// Stop functions that take direct Miles handles
		bool						StopSample( HSAMPLE hSample );
		bool						StopSample( H3DSAMPLE hSample );

		bool						StopSampleImmediately( DWORD sampleID );

		// Memory allocation callbacks
		static void* WINAPI			AllocateCallback( DWORD memAmount );
		static void WINAPI			FreeCallback( void* memPtr );

		// File access callbacks
		static DWORD WINAPI			FileOpenCallback( char const* pFileName, DWORD* pHandle );
		static void	WINAPI			FileCloseCallback( DWORD hFile );
		static long WINAPI			FileSeekCallback( DWORD hFile, long offset, DWORD type );
		static DWORD WINAPI			FileReadCallback( DWORD hFile, void* pBuffer, DWORD bytesToRead );

		// End of sound callbacks
		static void WINAPI			EndOfSampleCallback( HSAMPLE hSample );
		static void WINAPI			EndOf3DSampleCallback( H3DSAMPLE hSample );
		static void WINAPI			EndOfStreamCallback( HSTREAM hStream );

		// Timer callback
		static void WINAPI			FadeStopCallback( DWORD info );
		static void WINAPI			FadePauseCallback( DWORD info );
		static void WINAPI			FadeResumeCallback( DWORD info );

		// File caching
		SampleDescriptorCache		m_SampleDescriptorCache;
		SampleFileNameMap			m_SampleDescriptorNameMap;
		SampleFileCache				m_SampleFileCache;
		SampleFileNameMap			m_SampleFileNameMap;

#if !GP_RETAIL

		SampleFileNameMap			m_SampleHistoryMap;

#endif

		// 3D Sound support
		bool						m_b3DSoundSupport;

		// Enabled state
		bool						m_bEnabled;
		bool						m_bStasis;

		// Fade state
		bool						m_bFadeStopping;
		bool						m_bFadePausing;
		bool						m_bFadeResuming;
		bool						m_bReleaseTimer;

		// Stop/Pause requests
		bool						m_bStopAllSounds;
		bool						m_bPauseAllSounds;

		// Enumerated list of 3D sound providers
		SoundProviderColl			m_SoundProviderColl;

		// Handle to digital sound driver
		HDIGDRIVER					m_hDigitalDriver;

		// Volume
		DWORD						m_masterVolume;

		DWORD						m_lowSampleVolume;
		DWORD						m_normSampleVolume;
		DWORD						m_highSampleVolume;
		DWORD						m_ambientSampleVolume;

		DWORD						m_ambientStreamVolume;
		DWORD						m_voiceStreamVolume;
		DWORD						m_musicStreamVolume;

		// Timer
		HTIMER						m_hTimer;
		float						m_fadeTime;
		DWORD						m_userData;

		// Timing
		float						m_serviceTime;
		float						m_sampleFileExpiration;
		DWORD						m_soundLatency;

		// Driver information
		gpstring					m_redistDir;
		DWORD						m_primaryFreq;
		DWORD						m_primaryBitRate;
		DWORD						m_numPrimaryChannels;

		// Handle to 3D provider
		HPROVIDER					m_h3DProvider;
		HPROVIDER					m_defaultProvider;
		HPROVIDER					m_eaxProvider;
		gpstring					m_requested3DProvider;

		bool						m_bEAXAvailable;
		bool						m_bEAXEnabled;

		// Speaker and room type
		eSpeakerType				m_speakerType;
		eRoomType					m_roomType;

		// Handle to the 3D listener
		H3DPOBJECT					m_p3DListener;
		vector_3					m_listenerPos;

		// Array of handles to 2D sound samples
		SampleInfo*					m_pSamples;
		unsigned int				m_numSamples;

		// Array of handles to 3D sound samples
		Sample3DInfo*				m_p3DSamples;
		unsigned int				m_num3DSamples;

		// Cache of streams
		StreamCache					m_StreamCache;
		unsigned int				m_maxNumStreams;

		// Mapping of stop sample callbacks
		StopSampleCallbackMap		m_stopSampleCallbackMap;
		StopStreamCallbackMap		m_stopStreamCallbackMap;
		
		// Stop lists for multithreaded access
		Stop2DSampleRequestColl		m_sample2DStopRequest;
		Stop3DSampleRequestColl		m_sample3DStopRequest;
		StopStreamRequestColl		m_streamStopRequest;

		// Critical section
		kerneltool::Critical		m_soundCritical;
	};

};

#define gSoundManager GPGSound::SoundManager::GetSingleton()


#endif