#pragma once
/******************************************************************************************
**
**									GameMovie
**
**		Class responsible for movie playback.
**
**		Author: James Loe
**		Date:	06/01/01
**
*******************************************************************************************/

#ifndef _GAME_MOVIE_
#define _GAME_MOVIE_

struct BINK;
typedef BINK* HBINK;

class GameMovie : public Singleton < GameMovie >
{
	typedef std::vector< gpstring >		MovieColl;

public:

	// Construction/destruction
	GameMovie();
	~GameMovie();

	// State information
	bool IsMoviePlaying()					{ return m_bPlaying; }
	bool IsSoundAvailable()					{ return m_bSoundAvailable; }
	bool IsMoviePaused()					{ return m_bPaused; }

	// Initialize movie sound
	void InitializeSound();

	// Initialize a movie sequence from a gas address
	void InitializeMovieSequence( const char* pGasAddress );

	// Initialize a movie from a file name
	void InitializeMovie( const char* pMovieName );

	// Update the movie.  Returns whether or not it is finished.
	bool Update( Rapi* pRenderer, bool& bCopyFullFrame );

	// Restart the current movie in the sequence
	void RestartCurrentMovie();

	// Pause/Unpause the current movie in the sequence
	void PauseCurrentMovie( bool bPause );

	// Skip the current movie.  Returns whether or not the sequence is complete.
	bool SkipCurrentMovie();

	// Restart the entire sequence
	void RestartSequence();

	// Skip the entire sequence
	void SkipSequence();

	// Cleanup movie/file/mem handles
	void Shutdown();

private:

	// State information
	bool				m_bPlaying;
	bool				m_bSoundAvailable;
	bool				m_bPaused;
	bool				m_bNoFullCopy;

	// Sound information
	DWORD				m_soundLatency;

	// Currently playing movie
	HBINK				m_hBinkMovie;
	FileSys::FileHandle	m_hMovieFile;
	FileSys::MemHandle	m_hMovieMem;

	// List of movies in this sequence
	MovieColl			m_movieColl;
	DWORD				m_activeMovie;
};

#define gGameMovie GameMovie::GetSingleton()

#endif // _GAME_MOVIE_
