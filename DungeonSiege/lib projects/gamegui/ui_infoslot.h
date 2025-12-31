/**************************************************************************

Filename		: ui_infoslot.h

Description		: Control for a slot/rectangle that holds an item information
				  of a certain type.

Creation Date	: 11/23/99

**************************************************************************/


#pragma once
#ifndef _UI_INFOSLOT_H_
#define _UI_INFOSLOT_H_


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
class UIInfoSlot : public UIWindow
{
public:
	// Public Methods
	
	// Constructor
	UIInfoSlot( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIInfoSlot();

	// Destructor
	virtual ~UIInfoSlot();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	virtual void Update( double seconds );

	void		SetSlotType( gpstring type ) { m_sSlotType = type; };
	gpstring GetSlotType()					{ return m_sSlotType; };

	bool		PlaceItem( UIItem *pItem, bool bNotify = true, bool bPermanent = false, bool bDeactivate = false, bool bRequestEquip = true );
	void		RemoveItem();
	void		ClearItem();

	unsigned int	GetItemID()						{ return m_id; }
	void			SetItemID( unsigned int id )	{ m_id = id; }

	UIItem *	GetItem();

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

	void SetJustPlaced( bool bPlaced )	{ m_bItemPlaced = bPlaced; }
	bool GetJustPlaced()				{ return m_bItemPlaced; }

	void SetCanEquip( bool bCanEquip )	{ m_bCanEquip = bCanEquip;	}
	bool GetCanEquip()					{ return m_bCanEquip;		}

	void SetCanRemove( bool bCanRemove ) { m_bCanRemove = bCanRemove; }
	bool GetCanRemove()					 { return m_bCanRemove; }

	DWORD GetAttemptEquipId() { return m_attemptEquipId; }

	UITextBox *		GetTextBox();
	void			PositionTextBox();

	virtual void SetMousePos( unsigned int x, unsigned int y );

	void SetInvalidColor( DWORD dwColor ) { m_invalidColor = dwColor; }
	DWORD GetInvalidColor() { return m_invalidColor; }
	void SetInvalid( bool bSet ) { m_bInvalid = bSet; }
	bool GetInvalid()			 { return m_bInvalid; }

	void SetHighlightColor( DWORD dwColor ) { m_dwHighlightColor = dwColor; }
	DWORD GetHighlightColor()				{ return m_dwHighlightColor; }
	void SetHighlight( bool bSet ) { m_bHighlight = bSet; }
	bool GetHighlight() { return m_bHighlight; }

	virtual void Draw();

	float	GetRolloverTimer()				{ return m_RolloverTimer;	}
	void	SetRolloverTimer( float timer )	{ m_RolloverTimer = timer;	}
	
private:

	// Private Member Variables
	bool			m_buttondown;
	bool			m_bItemDetect;
	gpstring		m_sSlotType;
	int				m_item;
	unsigned int	m_id;
	bool			m_bItemPlaced;
	bool			m_bPermanent;
	bool			m_bCanEquip;
	DWORD			m_attemptEquipId;
	UITextBox *		m_pTextBox;
	DWORD			m_invalidColor;
	bool			m_bInvalid;
	DWORD			m_dwHighlightColor;
	bool			m_bHighlight; 
	bool			m_bRemoving;
	bool			m_bCanRemove;
	float			m_RolloverTimer;
};


#endif