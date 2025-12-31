/**************************************************************************

Filename		: ui_statusbar.h

Description		: Control for the status bar indicator/window

Creation Date	: 1/10/99

**************************************************************************/


#pragma once
#ifndef _UI_STATUSBAR_H_
#define _UI_STATUSBAR_H_


// Forward Declarations
class FuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;


struct UIFloatRect
{
	UIFloatRect() { left = 0.0f; right = 0.0f; top = 0.0f; bottom = 0.0f; }

	float left;
	float right;
	float top;
	float bottom;
};


// This button class contains all functionality for a simple button
class UIStatusBar : public UIWindow
{
public:
	// Public Methods
	
	// Constructor
	UIStatusBar( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIStatusBar();

	// Destructor
	virtual ~UIStatusBar();

	// Updating
	virtual void		Update( double seconds );

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	void	SetStatus( float fStatus );
	float	GetStatus()							{ return m_fStatus;						}
	void	SetStatusPercentage( int iStatus );
	int		GetStatusPercentage()				{ return ( FTOL( m_fStatus * 100 ) );	}

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

	virtual void SetRect( int left, int right, int top, int bottom, bool bAnimation = false );
	UIFloatRect & GetFloatRect() { return m_floatRect; }
	void SetFloatRect( float left, float right, float top, float bottom );

	virtual void Draw();

	virtual bool HitDetect( int x, int y );

	virtual void ResizeToCurrentResolution( int original_width, int original_height );

	void SetEdge( EDGE_TYPE edge ) { m_edge = edge; }
	EDGE_TYPE GetEdge() { return m_edge; }

	float GetMaxSize() { return m_max_size; }

	virtual void SetScale( float scale );

	void	SetBaseMax( float baseMax ) { m_baseMax = baseMax; }
	float	GetBaseMax()				{ return m_baseMax; }
	
private:

	// Private Member Variables	
	float		m_fStatus;
	EDGE_TYPE	m_edge;
	float		m_max_size;
	bool		m_bUVMap;
	UIFloatRect m_floatRect;
	bool		m_bDrawOutline;
	float		m_baseMax;
	bool		m_bDrawAdvance;
	DWORD		m_dwAdvanceColor;
	float		m_advanceDuration;
	float		m_elapsedAdvanceDuration;
	UIRect		m_advanceRect;
};


#endif