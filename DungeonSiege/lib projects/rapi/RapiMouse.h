#pragma once
#ifndef _RAPI_MOUSE_
#define _RAPI_MOUSE_

/**********************************************************************************************
**
**										RapiMouse
**
**		Class responsible for maintaining an asynchronous mouse cursor independent of
**		the main Rapi framerate.
**
**		Author:	James Loe
**
**		Copyright (c) 1999-2001,   Gas Powered Games
**
**********************************************************************************************/

class RapiMouseThread;

class RapiMouse : public Singleton< RapiMouse >
{

public:

	// Construction/destruction
	RapiMouse( Rapi* pRapi );
	~RapiMouse();

	// Enable/Disable mouse
	void Enable();
	void Disable();

	// Set manual copy state
	void SetManualCopy( bool bManualCopy )			{ m_bManualCopy = bManualCopy; }
	bool GetManualCopy()							{ return m_bManualCopy; }

	// Pause the mouse thread
	void PauseMouse( bool bPause, bool bWait = false );

	// Is mouse thread paused?
	bool IsMousePaused() const;

	// Submit an image to be used as the cursor
	void SetCursorImage( unsigned int cursor, int hotpointx, int hotpointy );
	void SetWaitImage( unsigned int cursor, int hotpointx, int hotpointy );
	void SetWaitStateAllowed( bool bWaitAllowed )	{ m_bWaitStateAllowed = bWaitAllowed; }

	// Set the current position of the cursor
	void SetCursorPosition( int x, int y );

	// Update the cursor on the screen
	void UpdateCursor( bool bPrimary = true );

	// Create an image cursor from file
	unsigned int CreateCursorImage( const gpstring& filename );
	unsigned int CreateCursorImage( RapiMemImage* pImage );

	// Destroy the given cursor image
	void DestroyCursorImage( unsigned int cursor );

	// Access to internal thread lock
	kerneltool::Critical&	GetMouseCritical()		{ return m_mouseCritical; }

private:

	// Setup internals for new cursor
	void SetCursorImage( RapiMemImage* pCursor );

	// Blit helper
	void Blit( LPDIRECT3DSURFACE8 destSurface, RECT* pDestRect, LPDIRECT3DSURFACE8 srcSurface, RECT* pSrcRect );

	// Blend helper
	void Blend( LPDIRECT3DSURFACE8 destSurface, RECT* pDestRect, RapiMemImage* pSrcImage, RECT* pSrcRect );


	// Critical
	kerneltool::Critical	m_mouseCritical;
	kerneltool::Critical	m_imageCritical;

	// Owner Rapi
	Rapi*					m_pRapi;

	// Mouse thread
	RapiMouseThread*		m_pMouseThread;

	// Cursor images
	RapiMemImage*			m_pImage;
	MemImageBucketVector	m_pImageBucketVector;

	// Primary copy surface
	int						m_primaryImageWidth;
	int						m_primaryImageHeight;
	LPDIRECT3DSURFACE8		m_primaryImage;
	LPDIRECT3DSURFACE8		m_blendedImage;

	// Cursor position
	int						m_cursorX;
	int						m_cursorY;
	int						m_hotpointX;
	int						m_hotpointY;

	// Rectangles
	GRect					m_primaryRect;
	GRect					m_cursorRect;

	// Pause state
	bool					m_bMarkedForResume;

	// Primary copy state
	bool					m_bUpdate;
	bool					m_bManualCopy;

	// Image changing
	unsigned int			m_newImage;
	int						m_newHotpointX;
	int						m_newHotpointY;
	bool					m_bCursorChange;

	// Wait image
	unsigned int			m_waitImage;
	unsigned int			m_oldImage;
	int						m_waitHotpointX;
	int						m_oldHotpointX;
	int						m_waitHotpointY;
	int						m_oldHotpointY;
	bool					m_bWaitStateAllowed;
	double					m_waitTime;

};

#define gRapiMouse RapiMouse::GetSingleton()

#endif
