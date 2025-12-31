/**************************************************************************

Filename		: ui_text.h

Description		: User interface control for displaying text

Creation Date	: 10/4/99

**************************************************************************/


#pragma once
#ifndef _UI_TEXT_H_
#define _UI_TEXT_H_


// Forward Declarations
class FuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class RapiFont;
class UIAnimation;
class UIMessage;


// Justification
enum JUSTIFICATION 
{
	SET_BEGIN_ENUM( JUSTIFY_, 0 ),

	JUSTIFY_LEFT,
	JUSTIFY_RIGHT,
	JUSTIFY_CENTER,

	SET_END_ENUM( JUSTIFY_ ),
};

FEX const char* ToString  ( JUSTIFICATION e );
    bool        FromString( const char* str, JUSTIFICATION& e );


// This button class contains all functionality for a simple cursor
class UIText : public UIWindow
{
	FUBI_CLASS_INHERIT( UIText, UIWindow );

public:
	// Public Methods
	
	// Constructor
	UIText(	FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIText();

	// Destructor
	virtual ~UIText();

	// Set/Get Text information
	void				SetText( const gpwstring & text, bool bReset = false );
	const gpwstring&	GetText() const						{ return m_text; }

	virtual void		AdjustChildren();
		
	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// Rendering
	virtual void Draw();

	// Color
FEX	void			SetColor( unsigned int color )	{ m_color = color;	};
FEX	unsigned int	GetColor() const				{ return m_color;	};
	virtual void	SetScale( float scale );

	RapiFont *		GetFont() { return m_pFont; }
	void			SetFont( RapiFont * pFont ) { m_pFont = pFont; }
FEX	void			SetFont( const gpstring& sFont );

FEX	JUSTIFICATION	GetJustification() const { return m_justification; }
FEX	void			SetJustification( JUSTIFICATION justify ) { m_justification = justify; }

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

	virtual void	SetRect( int left, int right, int top, int bottom, bool bAnimation = false );

FEX	void			SetAutoSize( bool autosize )	{ m_bAutoSize = autosize; }

	virtual void	ResizeToCurrentResolution( int original_width, int original_height );

private:

	// $ these are private to prevent accidental usage from C++
FEX	void SetText( const gpstring & text, bool bReset = false )  { SetText( ::ToUnicode( text ), bReset ); }
FEX void GetText( gpstring& text ) const		{ text = ::ToAnsi( GetText() ); }

	// Private Member Variables	
	gpwstring		m_text;
	gpwstring		m_drawText;
	RapiFont		*m_pFont;
	int				m_color;
	int				m_size;
	JUSTIFICATION	m_justification;
	bool			m_bAutoSize;
	ResFontMap		m_resFontMap;
	bool			m_lbuttondown;
	bool			m_rbuttondown;
	bool			m_bTruncateText;
	int				m_ParentOffset;
};


#endif