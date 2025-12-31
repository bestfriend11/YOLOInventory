/**************************************************************************

Filename		: ui_popupmenu.h

Description		: Control for the popup menu

Creation Date	: 10/6/99

**************************************************************************/

#pragma once
#ifndef _UI_POPUPMENU_H_
#define _UI_POPUPMENU_H_


// Forward Declarations
class FuelHandle;
class Rapi;
class RapiFont;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;
class UIDialogBox;


class UIPopupMenu : public UIWindow
{
public:

	enum
	{
		ELEMENT_PADDING = 16,
		DEFAULT_ELEMENT_HEIGHT = 18,
		INVALID_ELEMENT = -1,
	};
	
	struct MenuElement
	{
		MenuElement( gpwstring setText, int setTag )		{  text = setText;  tag = setTag;  }

		gpwstring	text;
		int			tag;
	};

	// Constructor
	UIPopupMenu( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIPopupMenu();

	// Destructor
	virtual ~UIPopupMenu();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// Render the control
	virtual void Draw();

	// Editor save
#if	!GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

	// Set the current mouse click position
	virtual void SetMousePos( unsigned int x, unsigned int y );

	// Elements
	void AddElement( gpwstring elementText, int tag );
	void RemoveElement( int tag );
	void RemoveElement( gpwstring elementText );
	void RemoveAllElements();

	int GetSelectedElement();
	gpwstring GetSelectedElementText();

	void SetElementHeight( int height )						{  m_ElementHeight = height;  }	

	// Show the menu
	enum eAlignment
	{
		ALIGN_DEFAULT,
		ALIGN_UP_LEFT,
		ALIGN_UP_RIGHT,
		ALIGN_DOWN_LEFT,
		ALIGN_DOWN_RIGHT,
	};

	void Show( int x, int y, eAlignment align = ALIGN_DEFAULT );
	void ShowAtCursor( eAlignment align = ALIGN_DEFAULT );

	void SetDefaultAlignment( eAlignment align )			{  m_Alignment = align;  }
		
private:

	typedef std::vector< MenuElement > ElementColl;

	// Private Member Variables
	UIDialogBox						*m_pBackground;
	RapiFont						*m_pFont;
	unsigned int					m_selection_texture_index;
	float							m_selection_alpha;	
	ElementColl						m_Elements;
	int								m_SelectedElement;
	int								m_ElementHeight;
	eAlignment						m_Alignment;
	bool							m_buttondown;
	bool							m_bAutoSize;
};


#endif