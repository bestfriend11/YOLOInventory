/**************************************************************************

Filename		: ui_itemslot.h

Description		: Control for a slot/rectangle that holds an item of a 
				  certain type

Creation Date	: 11/10/99

**************************************************************************/


#pragma once
#ifndef _UI_ITEMSLOT_H_
#define _UI_ITEMSLOT_H_


#include "ui_window.h"


// Forward Declarations
class FuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIItem;
class UIMessage;
class UITextBox;

// This button class contains all functionality for a simple button
class UIItemSlot : public UIWindow
{
public:
	// Public Methods
	
	// Constructor
	UIItemSlot( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIItemSlot();

	// Destructor
	virtual ~UIItemSlot();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	void		SetSlotType( gpstring type ) { m_sSlotType = type; };
	gpstring GetSlotType()					{ return m_sSlotType; };

	bool		PlaceItem( UIItem *pItem, bool bNotify = true, bool bPermanent = false, bool bDeactivate = false );
	void		RemoveItem();
	void		ClearItem();

	unsigned int	GetItemID()						{ return m_id; }
	void			SetItemID( unsigned int id )	{ m_id = id; }

	void		SetAcceptInput( bool bInput )	{ m_bAcceptInput = bInput; }
	bool		GetAcceptInput()				{ return m_bAcceptInput; }

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

	UITextBox *		GetTextBox();
	void			PositionTextBox();

	virtual void SetMousePos( unsigned int x, unsigned int y );

	virtual void Draw();

	bool IsEquipSlot() { return m_bEquipSlot; }

	virtual void Update( double seconds );

	void SetHighlightColor( DWORD dwColor ) { m_highlightColor = dwColor; }
	DWORD GetHighlightColor() { return m_highlightColor; }

	void SetActivateColor( DWORD dwColor ) { m_dwActivateColor = dwColor; }
	DWORD GetActivateColor() { return m_dwActivateColor; }

	void	SetSpecialColor( DWORD dwColor ) { m_dwSpecialColor = dwColor; }
	DWORD	GetSpecialColor()				 { return m_dwSpecialColor; }

	void	SetSpecialActive( bool bSet )	 { m_bSpecial = bSet; }
	bool	GetSpecialActive()				 { return m_bSpecial; }

	void SetJustPlaced( bool bPlaced )	{ m_bItemPlaced = bPlaced; }
	bool GetJustPlaced()				{ return m_bItemPlaced; }

	virtual void ResizeToCurrentResolution( int original_width, int original_height );

	void SetCanEquip( bool bCanEquip )	{ m_bCanEquip = bCanEquip;	}
	bool GetCanEquip()					{ return m_bCanEquip;		}

	void SetCanRemove( bool bCanRemove ) { m_bCanRemove = bCanRemove; }
	bool GetCanRemove()					 { return m_bCanRemove; }

	DWORD GetAttemptEquipId() { return m_attemptEquipId; }
	void SetAttemptEquipId( DWORD id ) { m_attemptEquipId = id; }
	
	void ActivateSlotHighlight( bool bSet ) { m_bActivateHighlight = bSet; }

	void ResetInput() { m_buttondown = false; }

	float	GetRolloverTimer()				{ return m_RolloverTimer;	}
	void	SetRolloverTimer( float timer )	{ m_RolloverTimer = timer;	}

private:

	// Private Member Variables
	bool			m_buttondown;
	bool			m_rbuttondown;
	bool			m_bAcceptInput;
	gpstring		m_sSlotType;
	int				m_item;
	unsigned int	m_id;
	bool			m_bPermanent;
	UITextBox *		m_pTextBox;
	bool			m_bEquipSlot;
	DWORD			m_highlightColor;
	
	bool			m_bActivateHighlight;
	DWORD			m_dwActivateColor;
	DWORD			m_dwActivateBorderColor;
	DWORD			m_dwSpecialColor;
	bool			m_bSpecial;
	bool			m_bItemPlaced;
	int				m_originalWidth;
	int				m_originalHeight;
	bool			m_bCanEquip;
	bool			m_bCanRemove;
	DWORD			m_attemptEquipId;
	float			m_RolloverTimer;
};


#endif