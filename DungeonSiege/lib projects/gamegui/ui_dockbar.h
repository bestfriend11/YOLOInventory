/**************************************************************************

Filename		: ui_dockbar.h

Description		: Control for the dockable bar ( like the window's start bar

Creation Date	: 2/24/99

**************************************************************************/


#pragma once
#ifndef _UI_DOCKBAR_H_
#define _UI_DOCKBAR_H_


// Enumerated Data Types
enum DOCKBAR_TYPE
{
	DOCKBAR_SCREEN,
	DOCKBAR_SCREEN_X,
	DOCKBAR_SCREEN_Y,
	DOCKBAR_SCREEN_X_SWITCH_Y,
	DOCKBAR_WINDOW,
};

enum eDockLocation
{
	DL_DEFAULT,
	DL_LEFT,
	DL_TOP,	
	DL_RIGHT,
	DL_BOTTOM,	

	DL_SIZE,
};


// This button class contains all functionality for a simple button
class UIDockbar : public UIWindow
{
public:
	// Public Methods
	
	// Constructor
	UIDockbar( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIDockbar();

	// Destructor
	virtual ~UIDockbar();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);
	
	// Set the current mouse position
	virtual void SetMousePos( unsigned int x, unsigned int y );

	// Dock the bar if within range
	void Dock( bool bAutoDock = false, int x = 0, int y = 0);
	void ForceDock( int x, int y );
	
#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

	virtual void Draw();
	
	void EndDock() { m_buttondown = false; }

	eDockLocation	GetDockLocation()							{ return m_currentLocation; }	

private:

	void			SetDockLocation( eDockLocation location )	{ m_currentLocation = location; }

	// Private Member Variables
	bool			m_buttondown;
	DOCKBAR_TYPE	m_dockbar_type;
	POINT			m_ptStart;
	UIRect			m_startRect;
	eDockLocation	m_currentLocation;

};


#endif