/**************************************************************************

Filename		: ui_listbox.h

Description		: Control for the listbox control

Creation Date	: 10/4/99

**************************************************************************/


#pragma once
#ifndef _UI_LISTBOX_H_
#define _UI_LISTBOX_H_


// Forward Declarations
class FuelHandle;
class Rapi;
class RapiFont;
class UITextureManager;
class Messenger;
class UISlider;
class UIAnimation;
class UIMessage;
class UIButton;
class UIText;


#include "ui_scroll_window.h"

// Structures

struct ListboxText
{
	gpwstring	sText;
	DWORD		dwColor;
};
typedef std::vector< ListboxText > ListboxLines;

struct ListboxElement
{	
	ListboxElement() :	ID( 0 )
					 ,	tag( -1 )
					 ,	bSelected( false )
					 ,	bSelectable( true )
					 ,	textureId( 0 )
					 ,	icon( -1 )
					 ,	flashIcon( false )
					 ,	flashInfinite( false )
					 ,	flashTime( 0.0f ) {}

	ListboxLines	lines;
	int				ID;
	int				tag;
	bool			bSelected;
	bool			bSelectable;
	unsigned int	textureId;
	int				icon;
	bool			flashIcon;
	bool			flashInfinite;
	float			flashTime;
	gpwstring		toolTip;
};
typedef std::vector< ListboxElement > ListboxElements;

typedef std::map< int, unsigned int > ListboxIconMap;


// This list box class contains all functionality for a list box
class UIListbox : public UIWindow
{
	FUBI_CLASS_INHERIT( UIListbox, UIWindow );

public:
	// Public Methods
	
	// Constructor
	UIListbox( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIListbox();

	// Destructor
	virtual ~UIListbox();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	virtual void		Update( double seconds );

	// Add/Remove Elements from a listbox
	void AddElement		( gpwstring sText, int tag = INVALID_TAG );
	void AddElement		( gpwstring sText, int tag, DWORD dwColor );
	void SetElementText	( const int tag, const int line, const gpwstring & sText, const DWORD dwColor );
	void RemoveElement	( gpwstring element );
FEX	void RemoveElement  ( int tag );
FEX	void RemoveAllElements();

	// Set the lead id
	virtual void	SetLeadElement	( int id )	{ m_lead_element = id;		};
	virtual int		GetLeadElement()			{ return m_lead_element;	};	

	// Get Element 
	ListboxElement * GetElement( int id );
	ListboxElement * GetTagElement( int tag );
	ListboxElement * GetTextElement( gpwstring text );

	// Select Element
FEX	void SetElementSelectable( int tag, bool bSelectable = false );
	void SelectElement( gpwstring sElement );
FEX	void SelectElement( int tag );

FEX	void ClearSelection();
	
	// Deselect All Elements
FEX	void	DeselectElements();

	// Element types
	void			SetElementType( UI_ELEMENT_TYPE type )	{ m_element_type = type; };
	UI_ELEMENT_TYPE	GetElementType()						{ return m_element_type; };

	// Set/Get Element height/width
	// void	SetHeight( float height );
FEX	void	SetHeight( int height );
FEX	int		GetHeight() const				{ return m_element_height;		};
FEX	int		GetWidth() const				{ return m_element_width;		};
	
	// Render the control
	virtual void Draw();

	virtual void SetVisible( bool bVisible );

	// Set the current mouse click position
	virtual void SetMousePos( unsigned int x, unsigned int y );

	// Load in the texture for the selection
FEX	void LoadSelectionTexture( const gpstring& texture );
	
	// Load icon elements
FEX	void AddIcon( const gpstring& texture, int icon );

FEX	void SetElementIcon( int tag, int icon );
	void SetElementIcon( gpwstring text, int icon );

FEX	void FlashElementIcon( int tag, float duration = -1.0f );
FEX void SetElementTextToolTip( gpwstring textName, const gpwstring& tip );
FEX void SetElementTagToolTip( int tag, const gpwstring& tip );
FEX void SetElementHelpBox( const gpstring& helpBox );

	// Get the number of elements in the listbox
	virtual unsigned int GetNumElements()		{ return m_elements.size(); };
	virtual unsigned int GetMaxActiveElements()	{ return m_max_active;		};
	void SetMaxActiveElements( int max_active );

	// Text Color
FEX	void			SetTextColor( unsigned int color )	{ m_dwColor = color;	};
FEX	unsigned int	GetTextColor() const				{ return m_dwColor;		};
	
	// Get Selected Item
	ListboxElement * GetSelectedElement();
	gpwstring		 GetSelectedText();
FEX	int				 GetSelectedTag();
	gpwstring		 GetElementText( int id );
	gpwstring		 GetElementTextFromTag( int tag );


	// Set the child slider, if applicable
	void			SetChildSlider( UISlider *childSlider ) { m_pVSlider = childSlider; };
	UISlider		*GetChildSlider()						{ return m_pVSlider;		};
	UIButton		*GetChildSliderUpButton()				{ return m_pButtonUp;		};
	UIButton		*GetChildSliderDownButton()				{ return m_pButtonDown;		};

FEX	void	ResizeSlider();

	virtual int		GetElementHeight() { return m_element_height; }

	virtual void CreateCommonCtrl( gpstring sTemplate = gpstring::EMPTY );

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

FEX	bool GetButtonDown() const				{ return m_buttondown; }
FEX	void SetButtonDown( bool bButtonDown )	{ m_buttondown = bButtonDown; }

FEX	void AutoSize( UIWindow * pWindow, int index );

FEX	void SetInvalidColor( DWORD dwColor ) { m_dwInvalidColor = dwColor; }
FEX	DWORD GetInvalidColor() const { return m_dwInvalidColor; }

FEX	void SetActiveColor( DWORD dwColor ) { m_dwActiveColor = dwColor; }
FEX	DWORD GetActiveColor() const { return m_dwActiveColor; }

FEX	void SetHitSelect( bool bSelect ) { m_bHitSelect = bSelect; }
FEX	bool GetHitSelect() const		  { return m_bHitSelect; }

FEX	bool HandleInputMessage( UIMessage & msg );
FEX	void SelectPreviousElement();
FEX	void SelectNextElement();
FEX	void SelectPreviousPage();
FEX	void SelectNextPage();
FEX	void SelectFirstElement();
FEX	void SelectLastElement();

	enum {  INVALID_TAG = -1, NO_ICON = -1  };

FEX	void SetHasFocus( bool bSet )	{ m_bFocus = bSet; }
FEX	bool GetHasFocus() const		{ return m_bFocus; }

FEX	bool GetPermanentFocus() const	{ return m_bPermanentFocus; }

private:

	// $ these are private to prevent accidental usage from C++
FEX	void AddElement		( const gpstring& sText, int tag = INVALID_TAG )  {  AddElement( ::ToUnicode( sText ), tag );  }
FEX	void AddElement		( const gpstring& sText, int tag, DWORD dwColor )  {  AddElement( ::ToUnicode( sText ), tag, dwColor );  }
FEX	void SetElementText	( const int tag, const int line, const gpstring & sText, const DWORD dwColor )  {  SetElementText( tag, line, ::ToUnicode( sText ), dwColor );  }
FEX	void RemoveElement	( const gpstring& element )  {  RemoveElement( ::ToUnicode( element ) );  }
FEX	void SelectElement	( const gpstring& sElement )  {  SelectElement( ::ToUnicode( sElement ) );  }
FEX	void SetElementIcon	( const gpstring& text, int icon )  {  SetElementIcon( ::ToUnicode( text ), icon );  }
FEX	void GetSelectedText( gpstring& text )  {  text = ::ToAnsi( GetSelectedText() );  }

	// Private Member Variables
	bool						m_buttondown;
	int							m_element_height;
	int							m_element_width;
	UI_ELEMENT_TYPE				m_element_type;
	int							m_lead_element;
	ListboxElements				m_elements;	
	ListboxIconMap				m_Icons;
	int							m_IconWidth;
	int							m_IconHeight;
	int							m_id_counter;	
	int							m_max_active;
	int							m_left_indent;
	RapiFont					*m_pFont;	
	unsigned int				m_selection_texture_index;
	float						m_selection_alpha;	
	unsigned int				m_dwColor;
	UISlider					*m_pVSlider;
	UIButton					*m_pButtonUp;
	UIButton					*m_pButtonDown;
	UIText						*m_pPopupItem;
	bool						m_bUseDefaultFont;
	int							m_fontHeight;
	bool						m_bRolloverSelect;
	DWORD						m_dwInvalidColor;
	DWORD						m_dwActiveColor;
	int							m_FlashingIconCount;
	bool						m_bHitSelect;
	bool						m_bFocus;
	bool						m_bPermanentFocus;
	DWORD						m_SelectionColor;
	int							m_hoveredElement; // for tooltips.
	bool						m_bToolTipOnSelect; // should the tooltip be displayed on the selected item? or the hovered item
};


#endif