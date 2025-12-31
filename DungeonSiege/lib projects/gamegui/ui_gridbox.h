/**************************************************************************

Filename		: ui_gridbox.h

Description		: Control for the gridbox control ( primary use is for 
				  inventory ).

Creation Date	: 10/6/99

**************************************************************************/


#pragma once
#ifndef _UI_GRIDBOX_H_
#define _UI_GRIDBOX_H_

#include "ui_types.h"
#include "ui_window.h" 

// Forward Declarations
class FuelHandle;
class Rapi;
class RapiFont;
class UITextureManager;
class Messenger;
class UIItem;
class UIAnimation;
class UIMessage;
class UITextBox;


// Enumerated Data Types
enum eGridSort
{
	SORT_REVERSE,
	SORT_FORWARD,
	SORT_SIZE,
};


// Defines
#define MAX_ROWS		100
#define MAX_COLUMNS		100


// Structures
struct GridUnit
{
	UIRect		rect;
	bool		bOccupied;
	int			itemID;
};

#define ITEM_INVALID -1


// Holds all the items within the gridbox
struct GridItem
{
	FUBI_POD_CLASS( GridItem );

	UIRect				rect;
	float				secondary_percent;
	float				alpha;
	int					itemID;	
	int					numItems;	
	DWORD				dwHighlightColor;
	DWORD				dwSecondaryColor;
	bool				bDrawHighlight;
};


struct PersistGridItem
{
	UIRect				rect;
	float				secondary_percent;
	float				alpha;	
	int					itemGoid;
	int					xoffset;
	int					yoffset;
};


// Type Definitions
typedef std::vector< GridItem > GridItemColl;
typedef std::vector< PersistGridItem > PersistGridItemColl;


// This button class contains all functionality for a simple button
class UIGridbox : public UIWindow
{
	FUBI_CLASS_INHERIT( UIGridbox, UIWindow );

public:
	// Public Methods
	
	// Constructor
	UIGridbox( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIGridbox();

	// Destructor
	virtual ~UIGridbox();

	virtual void Update( double secondsElapsed );

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// Get/Set Rows/Columns
FEX	void	SetRows( int rows ) 		{ m_rows = rows; };
FEX	int		GetRows() const				{ return m_rows; };
FEX	void	SetColumns( int columns )	{ m_columns = columns;		};
FEX	int		GetColumns() const			{ return m_columns;			};
FEX	void	SetBoxHeight( int height )	{ m_box_height = (float)height;	};
FEX	int		GetBoxHeight() const		{ return (int)m_box_height;	};
FEX	void	SetBoxWidth( int width )	{ m_box_width = (float)width;	};
FEX	int		GetBoxWidth() const			{ return (int)m_box_width;	};

	virtual void	SetScale( float scale );	

	// Sets the current mouse position
	virtual void	SetMousePos( unsigned int x, unsigned int y );

	// Checks if an item may be placed into the grid given it's dimensions
	bool	GridHitDetect(	int x, int y, UIItem *pItem, bool bNotify = true,
							bool bSnap = true, bool bAutoPlace = false, bool bOverride = false );

	// Checks if an item can be picked up from the grid
	void	ItemHitDetect();

	// Checks if the user is trying to use an item and sends the appropriate notification message
	// to the game
	void	ItemUse();

	// Draw Items
	virtual void	Draw();
	void			DrawItem( GridItem item );
	void			DrawSecondaryItem( GridItem item );
	void			DrawRolloverBoxes();

	// Resizes the individual boxes as well
	virtual void	ResizeToCurrentResolution( int original_width, int original_height );

	// Grid types - Only items that are the same type as the grid may be placed in it
FEX	void			SetGridType( const gpstring& type ) { m_sGridType = type; };
FEX	const gpstring&	GetGridType() const					{ return m_sGridType; };

	// Set the gridbox rect
	virtual void	SetRect( int left, int right, int top, int bottom, bool bAnimation = false );

	// Reset the gridbox; delete all item controls inside of it
	void ResetGrid();
	void PurgeItems();

	// Assemble a gridbox given a vector of grid items
	void AssembleGrid( std::vector< GridItem > grid_items, bool bClear = false );
	void AssembleGrid( PersistGridItemColl & gridItems );

	// Get the grid items within the current grid box
	void				SetGridItems( GridItemColl items ) { m_grid_items = items; }
	GridItemColl		& GetGridItems() { return m_grid_items; };
	PersistGridItemColl & PackagePersistGridItems();
	PersistGridItemColl & GetPersistGridItems() { return m_tempPersistItems; }

	bool GetItem( int id, GridItem & item );
	GridItem GetLastItem() { return m_lastItem; };

	// Add an item to the current grid box 
	void AddGridItem( GridItem item );
	
	// Place an item into the gridbox in an available area
	bool AutoItemPlacement( const char * item_name, bool bSnap = true, unsigned int id = 0, float secPercent = -1.0 );
	
	// Pick up overlapped items; if any exist
	bool PickupOverlapped( int occupantID, UIItem *pItem, bool bNotify = true );

	// Returns whether or not our gridbox contains a particular id
	bool ContainsID( unsigned int id );
	UIItem * RemoveID( unsigned int id );

	// Get a item switch over ID in case two objects overlapped
	bool		GetSwitch() { return m_bSwitch; }
	void		SetSwitch( bool bSwitch ) { m_bSwitch = bSwitch; }
	GridItem	GetSwitchItem() { return m_switchItem; }
	void		SetSwitchItem( GridItem switchItem ) { m_switchItem = switchItem; }

	// If there are multiple items selected, just auto-place them
	void		SetMultipleItems( bool bMultiple )	{ m_bMultipleItems = bMultiple; }
	bool		GetMultipleItems()					{ return m_bMultipleItems;		}

	// Rollover information
	void			SetRolloverID( int id )			{ m_rollover_id = id;	};
	int				GetRolloverID()					{ return m_rollover_id;	};
	float			GetRolloverTimer()				{ return m_RolloverTimer;	}
	void			SetRolloverTimer( float timer )	{ m_RolloverTimer = timer;	}
	void			RolloverDetect();

	void			SetSecondaryPercentVisible( int id, float percent );

	// Is gridbox a store gridbox? All store gridboxes require pickup request
FEX	bool			GetStore() const				{ return m_bIsStore;	};
FEX	void			SetStore( bool bStore )			{ m_bIsStore = bStore; m_bRequestPickup = bStore; };

FEX	bool			GetRequestPickup()				{ return m_bRequestPickup; }
FEX	void			SetRequestPickup( bool bSet )	{ m_bRequestPickup = bSet; }

FEX	bool			GetContinuePickup() const		{ return m_bContinuePickup; }
FEX	void			SetContinuePickup( bool bPickup ) { m_bContinuePickup = bPickup; }

FEX	void			SetItemDetect( bool bDetect )	{ m_bItemDetect = bDetect; }

FEX	bool			GetAutoTransfer() const			{ return m_bAutoTransfer; }
FEX	void			SetAutoTransfer( bool bTransfer ) { m_bAutoTransfer = bTransfer; }

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

FEX	UITextBox *		GetTextBox();
	void			PositionTextBox();	

	void			SetJustPlaced( bool bPlaced )	{ m_bGetJustPlaced = bPlaced; }
	bool			GetJustPlaced()					{ return m_bGetJustPlaced; }

	void			SetLastSort( eGridSort sort )	{ m_lastSort = sort; }
	eGridSort		GetLastSort()					{ return m_lastSort; }

	void			Sort( eGridSort sort );
	void			AutoSort();

	void			DetectItemDrop( int x, int y );

	PersistGridItemColl GetTempPersistItems() { return m_tempPersistItems; }

FEX	float			GetFullRatio();

FEX	void			SetIgnoreItem( bool bSet )	{ m_bIgnoreItem = bSet; }
FEX	bool			GetIgnoreItem()				{ return m_bIgnoreItem; }
	
	void			ActivateHighlight( int item, DWORD dwFirstColor, DWORD dwSecondColor );	
	void			ActivateHighlight( int item, DWORD dwColor );
	void			ResetHighlights();
	void			ActivateRolloverHighlight( int item, DWORD dwColor ) {	m_rolloverHighlight = item;
																			m_dwRolloverHighlight = dwColor; }

FEX	void			SetConsumesItems( bool bSet )			{ m_bConsumesItems = bSet; }
FEX	bool			GetConsumesItems() const				{ return m_bConsumesItems; }

	int				GetPickupId() { return m_pickupId; }

FEX	bool			GetDeactivateOverlapped() const			{ return m_bDeactivateOverlapped; }
FEX	void			SetDeactivateOverlapped( bool bSet )	{ m_bDeactivateOverlapped = bSet; }

FEX	void			SetLocalPlace( bool bSet )				{ m_bLocalPlace = bSet; }
FEX	bool			GetLocalPlace() const					{ return m_bLocalPlace; }

FEX	void			SetAutoPlaceNotify( bool bSet )			{ m_bAutoPlaceNotify = bSet; }
FEX	bool			GetAutoPlaceNotify() const				{ return m_bAutoPlaceNotify; }
	
	
	void			Xfer( FuBi::BitPacker& packer );

FEX	void			SetColor( DWORD color )					{ m_dwColor = color;	};
FEX	DWORD			GetColor() const						{ return m_dwColor;		};
		
private:

	// Private Member Variables	
	UITextBox *		m_pTextBox;
	bool			m_buttondown;
	bool			m_rbuttondown;
	bool			m_bItemDetect;
	bool			m_bMultipleItems;
	bool			m_bIgnoreItem;
	float			m_box_width;
	float			m_box_height;	
	int				m_rows;
	int				m_columns;
	int				m_mouseX;
	int				m_mouseY;	
	int				m_id_counter;
	bool			m_bSwitch;
	RapiFont		*m_pFont;		
	UIRect			m_rollover_rect;
	bool			m_bIsStore;
	bool			m_bContinuePickup;
	bool			m_bAutoTransfer;
	bool			m_bAutoPlaceOnly;
	bool			m_bGetJustPlaced;
	eGridSort		m_lastSort;
	DWORD			m_dwRolloverColor;	
	DWORD			m_dwOverlapRolloverColor;
	bool			m_bConsumesItems;
	int				m_pickupId;
	bool			m_bDeactivateOverlapped;
	int				m_rolloverHighlight;
	DWORD			m_dwRolloverHighlight;
	bool			m_bLocalPlace;
	bool			m_bAutoPlaceNotify;
	int				m_rollover_id;
	DWORD			m_dwColor;
	bool			m_bRequestPickup;
	float			m_RolloverTimer;

	GridItem		m_lastItem;
	GridItem		m_switchItem;	
	GridUnit		m_grid_unit[MAX_ROWS][MAX_COLUMNS];
	gpstring		m_sGridType;
	GridItemColl	m_grid_items;
	PersistGridItemColl	m_tempPersistItems;
};


double PointDistance( int x1, int y1, int x2, int y2 );


#endif