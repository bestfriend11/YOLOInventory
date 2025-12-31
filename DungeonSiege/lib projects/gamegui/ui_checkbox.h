/**************************************************************************

Filename		: ui_checkbox.h

Description		: Control for the standard 2-state check box

Creation Date	: 10/4/99

**************************************************************************/


#pragma once
#ifndef _UI_CHECKBOX_H_
#define _UI_CHECKBOX_H_


// Forward Declarations
class FuelHandle;
class FastFuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;


// This check box class contains all functionality for a simple check box
class UICheckbox : public UIWindow
{
	FUBI_CLASS_INHERIT( UICheckbox, UIWindow );

public:
	// Public Methods
	
	// Constructor
	UICheckbox( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UICheckbox();

	// Destructor
	virtual ~UICheckbox();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// Set/Get State 
	void SetState( bool bChecked );
	bool GetState() const								{ return m_checked; }
FEX	void FUBI_RENAME( SetCheckState )( bool bChecked )	{ SetState( bChecked ); }
FEX	bool FUBI_RENAME( GetCheckState )() const			{ return GetState(); }


FEX	void LoadCheckTexture( const gpstring& texture );

	virtual void Draw();

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

FEX	void SetAllowUserPress( bool bAllow )	{ m_bAllowUserPress = bAllow; }
FEX	bool GetAllowUserPress() const			{ return m_bAllowUserPress; }
	
private:

	// Private Member Variables
	bool	m_buttondown;
	bool	m_checked;
	unsigned int m_check_texture;
	UIRect	m_check_rect;
	bool	m_bAllowUserPress;

};


#endif