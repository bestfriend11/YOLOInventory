//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_chatbox.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef _UI_CHATBOX_H_
#define _UI_CHATBOX_H_


// Forward Declarations
class FuelHandle;
class FastFuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;
class UISlider;
class UIButton;

typedef std::vector< gpwstring > WStringVec;

struct UIChatMessage
{
	// Holds all the lines of text ( used for fast lookup and slider )
	WStringVec	vText;

	// Color of the current chat text
	DWORD		dwColor;
};

typedef std::vector< UIChatMessage > ChatVec;

// This button class contains all functionality for a simple button
class UIChatBox : public UIWindow
{
public:
	// Public Methods
	
	// Constructor
	UIChatBox(	FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIChatBox();

	// Destructor
	virtual ~UIChatBox();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// Set/Get Text information
	void				SetText( gpwstring text );
	gpwstring			GetText()					{ return m_text; };
	void				SubmitText( gpwstring sText, DWORD dwColor, bool bBlank = false );
	void				Clear();	
	void				SetTextVisible( bool bVisible ) { m_bDrawText = bVisible; }

	// Control Sizes of the string that can be entered
	void			SetMaxStringSize( int size )	{ m_max_size = size; }
	int				GetMaxStringSize()				{ return m_max_size; }

	// Rendering
	virtual void Draw();	
	virtual void Update( double seconds );

	// Common Control
	virtual void CreateCommonCtrl( gpstring sTemplate = gpstring::EMPTY );

	// Font Information
	void			SetFontHeight( int height )			{ m_font_height = height; }
	int				GetFontHeight()						{ return m_font_height; }

	// Vertical Positioning
	void			SetVerticalPos( int pos )			{ m_vertical_pos = pos; }
	int				GetVerticalPos()					{ return m_vertical_pos; }

	UISlider *		GetVSlider()						{ return m_pVSlider;	}
	void			SetSliderVisible( bool sliderOn )	{ m_bSliderVisible = sliderOn; }
	void			SetIsConsole( bool isConsole )		{ m_bConsole = isConsole; }
	

	void			RefreshSlider();
	void			RegisterActiveChat( bool active )	{ if( active ) {m_lastActiveTime = 0.0f;} else {m_lastActiveTime = m_groupFadeout;} }
	void			SetEditBoxActive( bool active )		{ m_editBoxActive = active; }

	int				GetElementsSize();
	int				GetTotalElementsSize();

	virtual unsigned int		GetNumElements();
	virtual unsigned int		GetMaxActiveElements();

	virtual int		GetLeadElement()			{ return m_lead_element; }
	virtual void	SetLeadElement( int lead );

	virtual int		GetElementHeight() { return m_font_height; }

	virtual bool	GetScrollDown()					{ return m_bScrollDown; }
	virtual void	SetScrollDown( bool bDown )		{ m_bScrollDown = bDown; }

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

	void SetDisableUpdate( bool bDisable )  { m_bDisableUpdate = bDisable; }
	bool GetDisableUpdate()					{ return m_bDisableUpdate; }

	DWORD GetFontColor() { return m_dwFontColor; }
	void  SetFontColor( DWORD dwColor ) { m_dwFontColor = dwColor; }

	void ViewHistoryUp();
	void ViewHistoryDown();
	void ViewHistoryEnd();

	void SetDrawLeadElement( bool bSet ) { m_bDrawLeadElement = bSet; ViewHistoryEnd(); }
	bool GetDrawLeadElement()			 { return m_bDrawLeadElement; }

	void SetRolloverView( bool bSet )	{ m_bRolloverView = bSet; }
	bool GetRolloverView()				{ return m_bRolloverView; }
	
	void SetLockView( bool bSet )		{ m_bLockView = bSet; if ( !bSet ) { ViewHistoryEnd(); } }
	bool GetLockView()					{ return m_bLockView; }

	float GetLineTimeout() { return m_lineTimeout; }
	void SetLineTimeout( float timeout ) { m_lineTimeout = timeout; }

	float GetLineSpeedUp() { return m_lineSpeedup; }
	void  SetLineSpeedUp( float speedUp ) { m_lineSpeedup = speedUp; }

	int GetLineSize();
	void SetLineSize( int line );

	virtual void	ResizeToCurrentResolution( int original_width, int original_height );

	void SetMinimized( bool bSet );
	bool GetMinimized()				{ return m_bMinimized; }
	
private:

	// Private Member Variables		
	gpwstring		m_text;
	RapiFont		*m_pFont;
	int				m_max_size;	
	int				m_font_height;
	ChatVec			m_chat;
	UISlider 		*m_pVSlider;
	UIButton		*m_pSliderUp;
	UIButton		*m_pSliderDown;
	int				m_vertical_pos;
	bool			m_bSlider;
	bool			m_bSliderVisible;
	float			m_lineTimeout;
	float			m_groupFadeout;
	int				m_lead_element;
	bool			m_bScrollDown;
	bool			m_bRetainScrollPos;
	bool			m_bConsole;
	float			m_lastEntryTime;
	float			m_lastActiveTime;// this should get updated with: resize, scroll, new entries, and if the edit box is open
	bool			m_editBoxActive;// this tells us if our partner is active or not, if it is active, then we won't time out.
	bool			m_bDisableUpdate;
	DWORD			m_dwFontColor;
	bool			m_bDeleteTimeouts;
	int				m_historyPos;
	bool			m_bViewHistory;
	bool			m_bNoDraw;
	bool			m_bDrawText;
	bool			m_bDrawLeadElement;
	bool			m_bRolloverView;
	int				m_maxHistory;
	bool			m_bLockView;
	int				m_linesLastVisible;
	int				m_lineSpacer;
	ResFontMap		m_resFontMap;
	bool			m_bMinimized;
	int				m_lineSize;
	float			m_lineSpeedup;
	int				m_wrapTolerance;
};


#endif