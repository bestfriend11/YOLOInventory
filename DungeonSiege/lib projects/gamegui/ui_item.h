/**************************************************************************

Filename		: ui_item.h

Description		: Control for the draggable/setable interface icon;
				  primary use is for inventory items

Creation Date	: 10/6/99

**************************************************************************/


#pragma once
#ifndef _UI_ITEM_H_
#define _UI_ITEM_H_


// Include Files
#include "ui_types.h"
#include "ui_window.h"


// Forward Declarations
class FuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;



// This button class contains all functionality for a simple button
class UIItem : public UIWindow
{
public:
	// Public Methods
	
	// Constructor
	UIItem(	FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIItem();

	// Destructor
	virtual ~UIItem();

	virtual void Update( double secondsElapsed );

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// Set the mouse position
	void SetMousePos( unsigned int x, unsigned int y );

	virtual bool LoadTexture	( gpstring texture, bool bResizeWindow = false );
	void ReloadTextures();

	// Set/Get Drag State
	void SetDragging( bool bDragging )	{ m_bDragging = bDragging;	};
	bool GetDragging()					{ return m_bDragging;		};
	void SetAcceptInput( bool bInput )	{ m_bAcceptInput = bInput;	};
	bool GetAcceptInput()				{ return m_bAcceptInput;	};

	// Set/Get Grid Occupation Sizes
	void	SetRows( int rows ) { m_rows = rows; };
	int		GetRows()			{ return m_rows; };
	void	SetColumns( int columns )	{ m_columns = columns;	};
	int		GetColumns()				{ return m_columns;		};

	// Placement of item
	void	CheckForPlacement();

	// Item typing for different types of grid items
	void		SetItemType( gpstring type )	{ m_sItemType = type; };
	gpstring GetItemType()					{ return m_sItemType; };
	void		SetSlotType( gpstring type ) { m_sSlotType = type; };
	gpstring GetSlotType()					{ return m_sSlotType; };
	
	// Item Identification
	void			SetItemID( unsigned int id )	{ m_id = id; }
	unsigned int	GetItemID()						{ return m_id; }	

	// Make the item active on screen
	void ActivateItem( bool bActivate, bool bPlaceValid = true );
	bool GetActive() { return m_bActive; }

	// Rendering
	virtual void	Draw();
	void			DrawSecondaryItem( UIRect rect, float secondary_percent = -1 );

	// Color
	void			SetColor( unsigned int color )	{ m_dwColor = color;	};
	unsigned int	GetColor()						{ return m_dwColor;		};

	// Stacking Information ( More than 1 means that there are multiple items "stacked" on top
	// of each other so a text representing how many are stacked are drawn
	void			SetNumStacked( int stacked )	{ m_stack_number = stacked; }
	int				GetNumStacked()					{ return m_stack_number;	}

	// Scaling information
	virtual void	SetScale( float scale );
	virtual float	GetScale()						{ return m_scale;			}

	// Secondary Image Manipulation
	EDGE_TYPE		GetSecondaryDynamicEdge() { return m_secondary_edge; }
	void			SetSecondaryDynamicEdge( EDGE_TYPE type ) { m_secondary_edge = type; }
	float			GetSecondaryPercentVisible()	{ return m_secondary_percent_visible; }
	void			SetSecondaryPercentVisible( float percent ) { m_secondary_percent_visible = percent; }
	void			LoadSecondaryTexture( gpstring texture );
	bool			GetHasSecondaryTexture()	{ return m_bSecondaryTexture; }
	void			SetHasSecondaryTexture( bool bTexture ) { m_bSecondaryTexture = bTexture; }

	// Parent Gridbox information
	UIGridbox *		GetParentGridbox()						{ return m_pParentGrid; }
	void			SetParentGridbox( UIGridbox *pGrid )	{ m_pParentGrid = pGrid; }
	int				GetBoxHeight()				{ return m_box_height; }
	void			SetBoxHeight( int height )	{ m_box_height = height; }
	int				GetBoxWidth()				{ return m_box_width; }
	void			SetBoxWidth( int width )	{ m_box_width = width; }

	void			RemoveItemParent( UIWindow * pFrom );
	
#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif
	
private:

	// Private Member Variables
	gpstring		m_sTexture;
	gpstring		m_sSecondTexture;
	bool			m_buttondown;
	bool			m_bDragging;
	bool			m_bAcceptInput;
	int				m_mouseX;
	int				m_mouseY;
	int				m_rows;
	int				m_columns;
	int				m_box_height;
	int				m_box_width;
	gpstring		m_sItemType;
	gpstring		m_sSlotType;
	int				m_stack_number;
	RapiFont		*m_pFont;
	unsigned int	m_dwColor;
	unsigned int	m_id;
	float			m_scale;
	UIGridbox *		m_pParentGrid;
	bool			m_bActive;	
	bool			m_bPlaceValid;
	bool			m_bMouseMoved;		

	// Item Secondary Image
	// Explanation:  All items will have the potential to have a secondary image.  
	// This is to allow the ability of having variable level potions.  The primary image is
	// never changed, only the secondary can be resized

	unsigned int		m_secondary_texture_index;
	float				m_secondary_percent_visible;
	EDGE_TYPE			m_secondary_edge;
	bool				m_bSecondaryTexture;


};



typedef std::vector< UIItem * > UIItemVec;


#endif