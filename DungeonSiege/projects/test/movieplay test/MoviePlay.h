/**************************************************************************

Filename		: MoviePlay.h

Description		: The MoviePlay class uses DirectShow from the DXMedia
				  technologies that allows it to play various movie formats
				  within a window.

Creation Date	: 6/30/99

**************************************************************************/


#pragma once
#ifndef _MOVIE_PLAY_
#define _MOVIE_PLAY_

// Include Files
#include <streams.h>



// Defines
#define WM_GRAPHNOTIFY  WM_USER+13

class MoviePlay
{
public:

	// Public Methods

	// Constructor
	MoviePlay();

	// Destructor
	~MoviePlay();

	// Play a movie
	void Play( char *szFilename, HWND hWnd, int x1 = 0, int y1 = 0, int x2 = 0, int y2 = 0 );

	// Stop playing a movie
	void Stop();

	// Call this when the window procedure gets a WM_GRAPHNOTIFY message
	void HandleNotifications();
 
private:

	// Private methods

	// Release the currently used interfaces
	void ReleaseInterfaces();

	// Private member variables
	bool			m_bPlaying;			// Is a movie playing?

	IBaseFilter		*m_piFilter;
	IGraphBuilder	*m_piGraphBuilder;
	IMediaControl	*m_piMediaControl;
	IMediaEventEx	*m_piMediaEventEx;
	IVideoWindow	*m_piVideoWindow;	
};


#endif