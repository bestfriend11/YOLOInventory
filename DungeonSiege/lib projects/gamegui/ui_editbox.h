/**************************************************************************

Filename		: ui_editbox.h

Description		: Control for the edit box control

Creation Date	: 1/11/00

**************************************************************************/


#pragma once
#ifndef _UI_EDITBOX_H_
#define _UI_EDITBOX_H_


// Forward Declarations
class FuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;
struct IMEUI_VERTEX;

// This button class contains all functionality for a simple button
class UIEditBox : public UIWindow
{
	FUBI_CLASS_INHERIT( UIEditBox, UIWindow );

public:
	// Public Methods
	
	// Constructor
	UIEditBox( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIEditBox();

	// Destructor
	virtual ~UIEditBox();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// Input focus accessors
FEX	void SetAllowInput( bool bInput );
FEX	bool GetAllowInput() const						{ return m_bAllowInput;	}

FEX	void GiveFocus()								{ m_bGiveFocus = true; }

FEX	void EnableIme( bool bSet );
	
	// Set/Get Text information
	void			SetText( gpwstring text );	
	const gpwstring& GetText() const				{ return m_text; }

FEX	void			ShowHiddenText( bool flag )		{ m_bShowHiddenText = flag; m_bAllowIme = !flag; }

FEX	void			SetPromptIndex( int index );
FEX	int				GetPromptIndex() const			{ return m_prompt_index; }

FEX	int				GetTabStop() const				{ return m_tabStop; }

	// Color
FEX	void			SetColor( unsigned int color )	{ m_color = color;	};
FEX	unsigned int	GetColor() const				{ return m_color;	};

	// Control Sizes of the string that can be entered
FEX	void			SetMaxStringSize( int size )	{ m_max_size = size; }
FEX	int				GetMaxStringSize() const		{ return m_max_size; }

	// Rendering
	virtual void Draw();
	void DrawPromptTexture( gpwstring sText, int startY );

FEX	void SetPermanentFocus( bool bFocus )			{ m_bPermanentFocus = bFocus;	}
FEX	bool GetPermanentFocus() const					{ return m_bPermanentFocus;		}	
	
	RapiFont *		GetFont() { return m_pFont; }
	void			SetFont( RapiFont * pFont ) { m_pFont = pFont; }

	// IME-Only Related Functions
	void			SetFontAttributes( unsigned int height, bool bBold );
	void			DrawText( const wchar_t * szText );
FEX	void			SetTextPosition( int x, int y ) { m_ImeX = x; m_ImeY = y; }
	void			DrawRect( UIRect rect, DWORD dwColor );
	void			DrawFans( IMEUI_VERTEX* paVertex, unsigned int num );	

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

	virtual void SetVisible( bool bVisible );

	virtual void Update( double seconds );

	virtual void SetRect( int left, int right, int top, int bottom, bool bAnimation = false );

	virtual void ResizeToCurrentResolution( int original_width, int original_height );

FEX	void	SetImeColor( DWORD dwColor )			{ m_imeColor = dwColor; }
FEX	DWORD	GetImeColor() const						{ return m_imeColor; }

	void DetermineCaretPosition();

	void SetAllowedChars( gpwstring charList )		{  m_AllowedChars = charList;  }
	void SetExcludedChars( gpwstring charList )		{  m_ExcludedChars = charList;  }

	bool DoesTextFit( gpwstring & sText );

	FEX	JUSTIFICATION	GetJustification() const { return m_justification; }
	FEX	void			SetJustification( JUSTIFICATION justify ) { m_justification = justify; }

private:

	// $ these are private to prevent accidental usage from C++
FEX	void SetText( const gpstring& text )	{ SetText( ::ToUnicode( text ) ); }
FEX	void GetText( gpstring& text ) const	{ text = ::ToAnsi( GetText() ); }

	// Private Member Variables
	bool			m_buttondown;
	bool			m_bAllowInput;
	bool			m_bGiveFocus;	
	bool			m_bShowHiddenText;	
	int				m_selectionColor;
	gpwstring		m_text;
	RapiFont		*m_pFont;
	int				m_max_size;
	int				m_max_pixels;
	bool			m_bPixelLimit;	
	int				m_font_size;
	unsigned int	m_prompt_texture;
	int				m_prompt_width;
	int				m_prompt_index;
	int				m_sel_start;
	int				m_sel_end;
	bool			m_bClearSelect;
	gpwstring		m_sDrawnText;
	bool			m_bPermanentFocus;	
	bool			m_bRestricted;
	int				m_asciiLowBound;
	int				m_asciiHighBound;
	int				m_tabStop;
	gpwstring		m_ExcludedChars;
	gpwstring		m_AllowedChars;
	float			m_promptFlash;
	float			m_promptFlashElapsed;
	bool			m_bDrawPrompt;	
	int				m_color;

	bool			m_bImeToggle;
	bool			m_bAllowIme;
	DWORD			m_dwToggleIme;
	int				m_ImeX;
	int				m_ImeY;
	int				m_imeColor;
	bool			m_bImeEnabled;
	
	JUSTIFICATION	m_justification;
};


#endif
