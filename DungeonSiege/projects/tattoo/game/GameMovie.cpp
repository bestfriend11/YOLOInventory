/******************************************************************************************
**
**									GameMovie
**
**		See .h file for details
**
*******************************************************************************************/

#include "Precomp_Game.h"

#include "Bink.h"
#include "FileSys.h"
#include "FileSysUtils.h"
#include "GameMovie.h"
#include "gps_manager.h"
#include "namingkey.h"
#include "RapiOwner.h"
#include "RapiAppModule.h"
#include "RapiImage.h"



// Construction
GameMovie::GameMovie()
	: m_bPlaying( false )
	, m_bSoundAvailable( false )
	, m_bPaused( false )
	, m_bNoFullCopy( false )
	, m_soundLatency( 0 )
	, m_hBinkMovie( NULL )
	, m_activeMovie( 0 )
{
}

// Destruction
GameMovie::~GameMovie()
{
	Shutdown();
}

// Sound initialization
void GameMovie::InitializeSound()
{
	// Tell Bink to use Miles Sound System
	if( gSoundManager.IsEnabled() )
	{
		if( !m_bPlaying )
		{
			m_soundLatency	= gSoundManager.GetSoundLatency();
			gSoundManager.SetSoundLatency( 64 );
		}

		BinkSoundUseMiles( gSoundManager.GetDigitalDriver() );
		BinkSetSoundTrack( 0 );

		// Indicate that we have valid sound
		m_bSoundAvailable	= true;
	}
	else
	{
		// Disable sound
		BinkSetSoundTrack( BINKNOSOUND );

		// No sound is available
		m_bSoundAvailable	= false;
	}
}

// Initialize a movie sequence from a gas address
void GameMovie::InitializeMovieSequence( const char* pGasAddress )
{
	gpassert( !m_bPlaying );
	gpassert( m_movieColl.empty() );
	gpassert( m_activeMovie == 0 );
	gpassert( !m_hMovieMem.IsMapped() );
	gpassert( !m_hMovieFile.IsOpen() );

	// Get handle to movie sequence information
	FastFuelHandle hMovie( pGasAddress );
	if( hMovie.IsValid() )
	{
		FastFuelHandleColl hMovieColl;
		hMovie.ListChildrenNamed( hMovieColl, "movie*" );

		// Go through collection of movies
		for( FastFuelHandleColl::iterator i = hMovieColl.begin(); i != hMovieColl.end(); ++i )
		{
			gpstring movieFileName	= (*i).GetString( "movie_file" );

			// Build proper relative pathname
			if( FileSys::DoesResourceFileExist( movieFileName.c_str() ) )
			{
				m_movieColl.push_back( movieFileName );
			}
		}
	}

	// Try to open the first movie
	if( !m_movieColl.empty() )
	{
		m_hMovieFile.Open( m_movieColl[ m_activeMovie ], FileSys::USAGE_STREAMING );
		gpassert( m_hMovieFile.IsOpen() );

		m_hMovieMem.Map( m_hMovieFile );
		gpassert( m_hMovieMem.IsMapped() );
		m_hMovieMem.Touch();

		// Initialize sound
		InitializeSound();

		// Open the handle to Bink
		m_hBinkMovie	= BinkOpen( (char*)m_hMovieMem.GetData(), BINKFROMMEMORY | BINKSNDTRACK );
		if( !m_hBinkMovie )
		{
			gperrorf(( "Could not open movie!  %s", BinkGetError() ));

			// Close resources
			Shutdown();
			return;
		}

		// Indicate playing status
		m_bPlaying	= true;
	}
}

// Initialize a movie from a file name
void GameMovie::InitializeMovie( const char* pMovieName )
{
	gpassert( !m_bPlaying );
	gpassert( m_movieColl.empty() );
	gpassert( m_activeMovie == 0 );
	gpassert( !m_hMovieMem.IsMapped() );
	gpassert( !m_hMovieFile.IsOpen() );

	// Build proper relative pathname
	if( FileSys::DoesResourceFileExist( pMovieName ) )
	{
		m_hMovieFile.Open( pMovieName, FileSys::USAGE_STREAMING );
		gpassert( m_hMovieFile.IsOpen() );

		m_hMovieMem.Map( m_hMovieFile );
		gpassert( m_hMovieMem.IsMapped() );
		m_hMovieMem.Touch();

		// Initialize sound
		InitializeSound();

		// Open the handle to Bink
		m_hBinkMovie	= BinkOpen( (char*)m_hMovieMem.GetData(), BINKFROMMEMORY | BINKSNDTRACK );
		if( !m_hBinkMovie )
		{
			gperrorf(( "Could not open movie!  %s", BinkGetError() ));

			// Close resources
			Shutdown();
			return;
		}

		// Indicate playing status
		m_bPlaying	= true;		
	}
}

// Update the movie.  Returns whether or not it is finished.
bool GameMovie::Update( Rapi* pRenderer, bool& bCopyFullFrame )
{
	if( !m_bPlaying )
	{
		return true;
	}

	if( !BinkWait( m_hBinkMovie ) )
	{
		// Update the frame
		BinkDoFrame( m_hBinkMovie );

		// Get the primary surface
		LPDIRECT3DSURFACE8 pSurface	= pRenderer->GetBackBufferSurface();
		if( pSurface )
		{
			DWORD copyFlags		= 0;
			if( bCopyFullFrame )
			{
				// Copy in a full frame because the app is telling us the old buffer was lost.
				// To make sure that we handle all entry cases, we copy full for two frames once
				// we receive this notification.
				copyFlags			= BINKCOPYALL;
				if( m_bNoFullCopy )
				{
					bCopyFullFrame	= false;
					m_bNoFullCopy	= false;
				}
				else
				{
					m_bNoFullCopy	= true;
				}
			}
			
			// Figure out the Bink equivalent of our pixel format
			switch( pRenderer->GetPrimaryPixelFormat() )
			{
				case PIXEL_ARGB_8888:
				{
					copyFlags	|= BINKSURFACE32A;
					break;
				}

				case PIXEL_RGB_565:
				{
					copyFlags	|= BINKSURFACE565;
					break;
				}

				case PIXEL_ARGB_1555:
				{
					copyFlags	|= BINKSURFACE555;
					break;
				}

				case PIXEL_ARGB_4444:
				{
					copyFlags	|= BINKSURFACE4444;
					break;
				}

				default:
				{
					gperror( "Unrecognized primary buffer pixel format, cannot copy movie bits!" );
				}
			};

			D3DLOCKED_RECT lockedRect;
			if( SUCCEEDED( pSurface->LockRect( &lockedRect, NULL, D3DLOCK_NOSYSLOCK ) ) )
			{
				// Lock and copy current frame to primary buffer
				BinkCopyToBuffer( m_hBinkMovie,
								  lockedRect.pBits,
								  lockedRect.Pitch,
								  pRenderer->GetHeight(),
								  0, 0,
								  copyFlags );

				// Unlock primary
				pSurface->UnlockRect();
			}
		}

		// Check the frame
		if( m_hBinkMovie->FrameNum == m_hBinkMovie->Frames )
		{
			return SkipCurrentMovie();
		}
		else
		{
			// Advance the frame
			BinkNextFrame( m_hBinkMovie );
		}
	}

	return false;
}

// Restart the current movie in the sequence
void GameMovie::RestartCurrentMovie()
{
	if( m_bPlaying )
	{
		// Reset to first frame
		BinkGoto( m_hBinkMovie, 1, NULL );
	}
}

// Pause/Unpause the current movie in the sequence
void GameMovie::PauseCurrentMovie( bool bPause )
{
	if( m_bPlaying )
	{
		if( bPause )
		{
			// Pause the movie
			BinkPause( m_hBinkMovie, 1 );
			m_bPaused	= true;
		}
		else
		{
			// Unpause the movie
			BinkPause( m_hBinkMovie, 0 );
			m_bPaused	= false;
		}
	}
}

// Skip the current movie.  Returns whether or not the sequence is complete.
bool GameMovie::SkipCurrentMovie()
{
	if( m_bPlaying )
	{
		// Close the movie
		BinkClose( m_hBinkMovie );
		m_hBinkMovie	= NULL;

		// Close open file handles
		m_hMovieMem.Close();
		m_hMovieFile.Close();

		// Look for the next movie in this sequence
		++m_activeMovie;
		if( m_activeMovie >= m_movieColl.size() )
		{
			// We are done with the entire sequence
			Shutdown();
			return true;
		}
		else
		{
			m_hMovieFile.Open( m_movieColl[ m_activeMovie ], FileSys::USAGE_STREAMING );
			gpassert( m_hMovieFile.IsOpen() );

			m_hMovieMem.Map( m_hMovieFile );
			gpassert( m_hMovieMem.IsMapped() );
			m_hMovieMem.Touch();

			// Initialize sound
			InitializeSound();

			// Open the handle to Bink
			m_hBinkMovie	= BinkOpen( (char*)m_hMovieMem.GetData(), BINKFROMMEMORY | BINKSNDTRACK );
			if( !m_hBinkMovie )
			{
				gperrorf(( "Could not open movie!  %s", BinkGetError() ));

				// Close resources
				Shutdown();
				return true;
			}
		}

		return false;
	}

	return true;
}

// Restart the entire sequence
void GameMovie::RestartSequence()
{
	if( m_bPlaying )
	{
		// Close the movie
		BinkClose( m_hBinkMovie );
		m_hBinkMovie	= NULL;

		// Open the proper movie file if needed
		if( m_activeMovie != 0 )
		{
			// Close open file handles
			m_hMovieMem.Close();
			m_hMovieFile.Close();

			// Reset sequence to beginning
			m_activeMovie	= 0;

			m_hMovieFile.Open( m_movieColl[ m_activeMovie ], FileSys::USAGE_STREAMING );
			gpassert( m_hMovieFile.IsOpen() );

			m_hMovieMem.Map( m_hMovieFile );
			gpassert( m_hMovieMem.IsMapped() );
			m_hMovieMem.Touch();
		}

		// Initialize sound
		InitializeSound();

		// Open the handle to Bink
		m_hBinkMovie	= BinkOpen( (char*)m_hMovieMem.GetData(), BINKFROMMEMORY | BINKSNDTRACK );
		if( !m_hBinkMovie )
		{
			gperrorf(( "Could not open movie!  %s", BinkGetError() ));

			// Close resources
			Shutdown();
		}
	}
}

// Skip the entire sequence
void GameMovie::SkipSequence()
{
	// Just shut everything down
	Shutdown();
}

// Cleanup movie/file/mem handles
void GameMovie::Shutdown()
{
	if( m_bPlaying )
	{
		// Close the movie
		if( m_hBinkMovie )
		{
			BinkClose( m_hBinkMovie );
			m_hBinkMovie	= NULL;
		}

		// Close open file handles
		if( m_hMovieMem.IsMapped() )
		{
			m_hMovieMem.Close();
		}
		if( m_hMovieFile.IsOpen() )
		{
			m_hMovieFile.Close();
		}

		// Reset the latency
		if( m_bSoundAvailable )
		{
			gSoundManager.SetSoundLatency( m_soundLatency );
		}

		m_bPlaying	= false;
	}

	// Cleanup sequence
	m_movieColl.clear();
	m_activeMovie = 0;
}
