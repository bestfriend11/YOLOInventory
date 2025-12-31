/**************************************************************************

Filename		: ui_slider.h

Description		: Control for the slider/scrollbar control 

Creation Date	: 9/19/99

**************************************************************************/


#pragma once
#ifndef _UI_SLIDER_H_
#define _UI_SLIDER_H_


// Forward Declarations
class FuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;
class UIButton;
class UIText;


// This slider class contains all functionality for a slider/scrollbar
class UISlider : public UIWindow
{
public:
	// Public Methods
	
	// Constructor
	UISlider( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UISlider();

	// Destructor
	virtual ~UISlider();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	void DrawSliderButton();
	void DrawTrackSlider();

	// Render the control
	virtual void Draw();

	virtual void SetVisible( bool bVisible );

	// Set the current mouse click position
	virtual void SetMousePos( unsigned int x, unsigned int y );

	virtual void SetRect( int left, int right, int top, int bottom, bool bAnimation = false );

	void IncrementElement( int scroll_value );

	// Dynamic Sizing Accessors
	void SetDynamicButton( bool bDynamic )	{ m_button_dynamic = bDynamic;	}
	bool GetDynamicButton()					{ return m_button_dynamic;		}

	void CreateSliderButtons();

	void ResizeSliderButtons();

	void CreateCommonCtrl( gpstring sName, gpstring sTemplate, UIWindow *pWindow, bool bVertical );
	
	bool IsVertical() { return m_bVertical; }

	void CalculateSlider();

	void CalculateRelativeSliderPos();

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

	int GetMin()						{ return m_minvalue; }
	void SetMin( int min );

	int GetMax()						{ return m_maxvalue; }
	void SetMax( int max );

	void SetRange( int max, int min = 0 );

	int GetValue()						{ return m_value; }
	void SetValue( int value );

	bool GetButtonDown()				{ return m_buttondown; }

	void SetTrackMark( int value )		{ m_trackMarkValue = value; m_bDrawTrackMark = true; }

	void PositionPopupValueText();

	UIText * GetPopupValueText()		{ return m_pPopupValue; }

private:

	enum
	{
		SLIDER_BUTTON_DEFAULT_SIZE = 16,
		SLIDER_BUTTON_MIN_SIZE = 16,
		MIN_TRACK_SLIDER_WIDTH = 48,
	};

	// Private Member Variables
	bool			m_buttondown;
	UIRect			m_button_rect;
	double			m_button_alpha;
	int				m_button_spacer;
	int				m_button_size;
	double			m_fButtonSpacer;
	bool			m_button_dynamic;
	bool			m_bVertical;
	bool			m_bButtonVisible;
	int				m_button_mouse_offset;
	bool			m_bListSlider;
	bool			m_bTrackSlider;
	bool			m_bRepeaterButtons;

	int				m_minvalue;
	int				m_maxvalue;
	int				m_stepvalue;
	int				m_value;

	bool			m_bDrawTrackMark;
	int				m_trackMarkValue;

	UIButton *		m_pScrollDown;
	UIButton *		m_pScrollUp;

	bool			m_bShowPopupValue;
	UIText *		m_pPopupValue;

	// Textures
	unsigned int	m_button_texture_index_top;
	unsigned int	m_button_texture_index_mid;
	unsigned int	m_button_texture_index_bottom;
	unsigned int	m_track_center_texture_index;
	unsigned int	m_track_left_texture_index;
	unsigned int	m_track_right_texture_index;
	unsigned int	m_track_button_texture_index;
	unsigned int	m_track_mark_texture_index;

	DWORD			m_dwDisableColor;
};


#endif