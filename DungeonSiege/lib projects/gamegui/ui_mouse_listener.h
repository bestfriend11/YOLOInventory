/**************************************************************************

Filename		: ui_mouse_listener.h

Description		: This is a ui window mouse listener.  Now what the hell is
				  that, you may ask?  Well, basically you create a window with
				  a rectangle and when it gets focus ( you click on it and hold )
				  it will report to your notification callbacks and you can
				  extract delta information, position, etc. 

Creation Date	: 4/27/2000

**************************************************************************/


#pragma once
#ifndef _UI_MOUSE_LISTENER_H_
#define _UI_MOUSE_LISTENER_H_


// Forward Declarations
class FuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;


// This button class contains all functionality for a simple button
class UIMouseListener : public UIWindow
{
public:
	// Public Methods
	
	// Constructor
	UIMouseListener( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIMouseListener();

	// Destructor
	virtual ~UIMouseListener();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// Set the current mouse position and record deltas
	virtual void SetMousePos( unsigned int x, unsigned int y );

	void SetDeltaX( int x ) { m_delta_x = x; }
	int  GetDeltaX()		{ return m_delta_x; }

	void SetDeltaY( int y ) { m_delta_y = y; }
	int  GetDeltaY()		{ return m_delta_y; }

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif
	
private:

	// Private Member Variables
	bool m_buttondown;
	bool m_rbuttondown;

	int m_delta_x;
	int m_delta_y;

	int m_start_x;
	int m_start_y;

};


#endif