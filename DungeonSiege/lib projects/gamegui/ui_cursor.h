/**************************************************************************

Filename		: ui_cursor.h

Description		: User interface cursor control

Creation Date	: 9/22/99

**************************************************************************/


#pragma once
#ifndef _UI_CURSOR_H_
#define _UI_CURSOR_H_


// Forward Declarations
class FastFuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;


// This button class contains all functionality for a simple cursor
class UICursor : public UIWindow
{
public:
	// Public Methods
	
	// Constructor
	UICursor( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UICursor();

	// Destructor
	virtual ~UICursor();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// Set the mouse hotspot
	void SetHotspot( unsigned int x, unsigned int y );

	// Set the current mouse position
	virtual void SetMousePos( unsigned int x, unsigned int y );

	// Load in an animated cursor file
	void LoadAnimatedTexture( gpstring texture, bool bResize = false );

	// Get a list of the animation textures
	virtual void GetAnimationTextures( std::vector< unsigned int > & textures );

	virtual void SetVisible( bool bVisible );

	virtual void Draw();

	void Activate();

	unsigned int GetHotspotX() { return m_hotspotX; }
	unsigned int GetHotspotY() { return m_hotspotY; }
	
private:

	// Private Member Variables
	unsigned int	m_hotspotX;
	unsigned int	m_hotspotY;
	std::vector< unsigned int > m_animated_textures;
};




#endif
