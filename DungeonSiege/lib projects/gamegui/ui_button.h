/**************************************************************************

Filename		: ui_button.h

Description		: Control for the standard click button

Creation Date	: 9/19/99

**************************************************************************/


#pragma once
#ifndef _UI_BUTTON_H_
#define _UI_BUTTON_H_


// Forward Declarations
class FuelHandle;
class FastFuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;


#include "ui_window.h"


// This button class contains all functionality for a simple button
class UIButton : public UIWindow
{
	FUBI_CLASS_INHERIT( UIButton, UIWindow );

public:
	// Public Methods
	
	// Constructor
	UIButton( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIButton();

	void CreateCommonCtrl( gpstring sName, gpstring sTexturePrefix, UIWindow *pParent = 0, eCommonButtons buttonType = SLIDER_NONE, bool bSetRect = true );
	virtual void CreateCommonCtrl( gpstring sTemplate = gpstring::EMPTY );

	// Destructor
	virtual ~UIButton();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	virtual void Draw();

	virtual void	SetRect( int left, int right, int top, int bottom, bool bAnimation = false );

	void OnButtonDown();
	void OnButtonUp();
	void OnButtonRollover();
	void OnButtonRolloff();

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

	virtual void Update( double seconds );

FEX	void SetAllowUserPress( bool bAllow )	{ m_bAllowUserPress = bAllow; }
FEX	bool GetAllowUserPress() const			{ return m_bAllowUserPress; }

FEX	bool GetButtonDown() const				{ return m_buttondown; }	

FEX	void DisableButton();
FEX	void EnableButton();
FEX	bool GetDisabled() const { return m_bDisabled; }

	virtual void SetVisible( bool bVisible );				

	bool IsRepeater() const					{ return m_bRepeater; }
FEX	bool FUBI_RENAME( GetIsRepeater )() const{ return IsRepeater(); }
FEX	void SetIsRepeater( bool repeater )		{ m_bRepeater = repeater; }

FEX	void SetPressedState( bool pressed );
FEX	bool GetPressedState() const			{ return m_bPressed; }

private:

	void MoveGroupTextDown();
	void MoveGroupTextUp();

	// Private Member Variables
	bool m_buttondown;
	bool m_rbuttondown;
	bool m_bStretchCtrl;
	bool m_bRepeater;
	float m_fRepeatRate;
	float m_fRepeatCurrent;
	float m_fFirstRepeat;
	gpstring m_sTemplatePrefix;
	bool m_bAllowUserPress;
	bool m_bDrawRollover;
	DWORD m_dwRolloverColor;
	bool m_bDisabled;
	DWORD m_dwDisableColor;
	bool m_bTextUp;
	bool m_bLeftRightButton;
	bool m_bIgnoreItems;
	DWORD m_dwSelectedColor;
	bool m_bPressed;

	// For common textures

	unsigned int m_button_center_tex;
	unsigned int m_button_left_tex;
	unsigned int m_button_right_tex;
};


#endif