/**************************************************************************

Filename		: ui_radiobutton.h

Description		: Control for the radio buttons

Creation Date	: 10/4/99

**************************************************************************/


#pragma once
#ifndef _UI_RADIOBUTTON_H_
#define _UI_RADIOBUTTON_H_


// Forward Declarations
class FuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;


// This radio button class contains all functionality for a simple radio button
class UIRadioButton : public UIWindow
{
	FUBI_CLASS_INHERIT( UIRadioButton, UIWindow );

public:
	// Public Methods
	
	// Constructor
	UIRadioButton( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIRadioButton();

	// Destructor
	virtual ~UIRadioButton();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// Load in a selection texture
FEX	void LoadSelectionTexture( const gpstring& texture );
	void DrawSelectionTexture();

	// Render the control
	virtual void Draw();

	// See if the radio button is currently selected
FEX	bool GetCheck() const { return m_checked; };
FEX	void SetCheck( bool bCheck );

FEX	void SetMidState( bool bSet );
	
FEX	void SetForceCheck( bool bCheck );

FEX	const gpstring& GetRadioGroup() const { return m_sRadioGroup; }
FEX	void			SetRadioGroup( const gpstring& sGroup ) { m_sRadioGroup = sGroup; }

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

	virtual void CreateCommonCtrl( gpstring sTemplate );
	void		 CreateStaticCommonCtrl( gpstring sTexturePrefix );

	void OnButtonDown();
	void OnButtonUp();
	void OnButtonRollover();
	void OnButtonRolloff();

FEX	void SetAllowUserPress( bool bAllow )	{ m_bAllowUserPress = bAllow; }
FEX	bool GetAllowUserPress() const			{ return m_bAllowUserPress; }

FEX	void	SetClickDelay( float delay )	{ m_clickDelay = delay; }
FEX	float	GetClickDelay() const			{ return m_clickDelay; }

	virtual void Update( double seconds );

FEX	void SetInvalid( bool bSet )	{ m_bInvalid = bSet; }
FEX	bool GetInvalid() const			{ return m_bInvalid; }

FEX	void ResetButtonStatus();

	
private:

	// Private Member Variables
	bool				m_buttondown;
	bool				m_checked;
	gpstring			m_sRadioGroup;
	bool				m_bStretchCtrl;
	bool				m_bPopup;
	bool				m_bAllowUserPress;
	bool				m_bRolloverHighlight;
	DWORD				m_highlightColor;
	bool				m_bStrictRollover;
	float				m_clickDelay;
	float				m_secondsElapsed;
	bool				m_bMidState;
	bool				m_bResetDown;
	bool				m_bIgnoreItems;
	bool				m_bSendUncheckToSelf;
	bool				m_bLeftRightButton;

	unsigned int		m_selection_texture;
	bool				m_bSelectionTexture;
	float				m_selection_alpha;
	UINormalizedRect	m_selection_uv;
	
	unsigned int		m_rollover_texture;
	bool				m_bRolloverTexture;
	float				m_rollover_alpha;
	UINormalizedRect	m_rollover_uv;

	DWORD				m_dwInvalidColor;
	bool				m_bInvalid;	

	// For common textures
	unsigned int m_button_center_tex;
	unsigned int m_button_left_tex;
	unsigned int m_button_right_tex;

};


#endif