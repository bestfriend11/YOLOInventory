/**************************************************************************

Filename		: ui_combobox.h

Description		: Control for combo box control

Creation Date	: 1/11/00

**************************************************************************/


#pragma once
#ifndef _UI_COMBOBOX_H_
#define _UI_COMBOBOX_H_


// Forward Declarations
class FuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;
class UIListbox;
class UIButton;
class UIDialogBox;

// Public Methods
// This button class contains all functionality for a combo box
class UIComboBox : public UIWindow
{
	FUBI_CLASS_INHERIT( UIComboBox, UIWindow );

public:
	// Public Methods
	
	// Constructor
	UIComboBox(	FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIComboBox();

	// Destructor
	virtual ~UIComboBox();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// Set/Get Text information
	void				SetText( gpwstring text );
	gpwstring			GetText()					{ return m_text; };

FEX	void				SetSelectedTag( int tag )	{ m_SelectedTag = tag; }
FEX	int					GetSelectedTag() const		{ return m_SelectedTag; }

	// Color
FEX	void			SetColor( unsigned int color )	{ m_dwColor = color;	};
FEX	unsigned int	GetColor() const				{ return m_dwColor;		};

	// List expanded
FEX	void SetExpanded( bool bExpanded );
FEX	bool GetExpanded() const { return m_bExpanded; }

	// Text Color
FEX	void			SetTextColor( unsigned int color )	{ m_dwColor = color;	}
FEX	unsigned int	GetTextColor() const				{ return m_dwColor;		}

FEX	void			SetDrawText( bool bDrawText )		{ m_bDrawText = bDrawText; }
FEX	bool			GetDrawText() const					{ return m_bDrawText; }

	// Rendering
	virtual void Draw();

	virtual void SetVisible( bool bVisible );

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

FEX	void SetChildListbox( UIListbox *pListbox )		{ m_pListbox = pListbox; }
FEX	UIListbox *GetChildListbox() const				{ return m_pListbox; }

FEX	bool GetButtonDown() const						{ return m_buttondown; }

private:

	// $ these are private to prevent accidental usage from C++
	void SetText( const gpstring& text )	{ SetText( ::ToUnicode( text ) ); }
	void GetText( gpstring& text )			{ text = ::ToAnsi( GetText() ); }

	// Private Member Variables
	bool			m_bDrawText;
	bool			m_bExpanded;
	bool			m_buttondown;		
	gpwstring		m_text;
	RapiFont		*m_pFont;
	UIListbox		*m_pListbox;
	UIDialogBox		*m_pBackground;
	bool			m_bAutoSize;
	unsigned int	m_MaxAutoSizeElements;
	int				m_SelectedTag;
	bool			m_bTruncateText;
	gpwstring		m_drawText;
	unsigned int	m_dwColor;
};


#endif