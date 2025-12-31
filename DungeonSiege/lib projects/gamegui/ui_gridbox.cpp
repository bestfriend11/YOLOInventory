/**************************************************************************

Filename		: ui_gridbox.cpp

Description		: Control for the gridbox control ( primary use is for 
				  inventory )

Creation Date	: 10/6/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_item.h"
#include "ui_gridbox.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"
#include "ui_textbox.h"
#include "FuBiBitPacker.h"


/**************************************************************************

Function		: UIGridbox()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIGridbox::UIGridbox( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: UIWindow( fhWindow, parent_interface, parent )
, m_pTextBox( NULL )
, m_buttondown( false )
, m_rbuttondown( false )
, m_bItemDetect( true )
, m_bMultipleItems( false )
, m_bIgnoreItem( false )
, m_box_width( 0.0f )
, m_box_height( 0.0f )
, m_rows( 0 )
, m_columns( 0 )
, m_mouseX( 0 )
, m_mouseY( 0 )
, m_id_counter( 0 )
, m_bSwitch( false )
, m_pFont( NULL )
, m_bIsStore( false )
, m_bContinuePickup( true )
, m_bAutoTransfer( false )
, m_bAutoPlaceOnly( false )
, m_bGetJustPlaced( false )
, m_lastSort( SORT_FORWARD )
, m_dwRolloverColor( 0x666F6F6F )
, m_dwOverlapRolloverColor( 0x66FFFFFF )
, m_bConsumesItems( false )
, m_pickupId( 0 )
, m_bDeactivateOverlapped( false )
, m_rolloverHighlight( ITEM_INVALID )
, m_dwRolloverHighlight( 0x6600ff00 )
, m_bLocalPlace( true )
, m_bAutoPlaceNotify( true )
, m_rollover_id( 0 )
, m_dwColor( 0xffffffff )
, m_bRequestPickup( false )
, m_RolloverTimer( 0.0f )
{	
	SetType( UI_TYPE_GRIDBOX );
	SetInputType( TYPE_INPUT_ALL );
	m_rollover_rect = UIRect();

	fhWindow.Get( "box_width",		m_box_width		);
	fhWindow.Get( "box_height",		m_box_height	);
	fhWindow.Get( "rows",			m_rows			);
	fhWindow.Get( "columns",		m_columns		);
	fhWindow.Get( "grid_type",		m_sGridType		);
	fhWindow.Get( "autoplace_only", m_bAutoPlaceOnly );
	fhWindow.Get( "rollover_color", m_dwRolloverColor );
	fhWindow.Get( "rollover_overlap_color", m_dwOverlapRolloverColor );
	fhWindow.Get( "consumes_items", m_bConsumesItems );
	fhWindow.Get( "request_pickup", m_bRequestPickup );

	if (( m_rows >= MAX_ROWS ) || ( m_columns >= MAX_COLUMNS )) {
		gperrorf(( "Error in Gridbox Construction: Unsupported Gridbox size (%dx%d).\n", m_rows, m_columns ));
	}

	for ( int i = 0; i < m_rows; ++i ) {
		for ( int j = 0; j < m_columns; ++j ) {
			m_grid_unit[i][j].bOccupied		=	false;
			m_grid_unit[i][j].itemID		=	-1;
			m_grid_unit[i][j].rect.left		=	GetRect().left+((int)m_box_width*j);
			m_grid_unit[i][j].rect.right	=	m_grid_unit[i][j].rect.left + (int)m_box_width;
			m_grid_unit[i][j].rect.top		=	GetRect().top+((int)m_box_height*i);
			m_grid_unit[i][j].rect.bottom	=	m_grid_unit[i][j].rect.top + (int)m_box_height;
		}
	}

	m_lastItem.alpha = 1.0f;
	m_lastItem.itemID = 0;
	m_lastItem.numItems = 0;	
	m_lastItem.rect = UIRect();
	m_lastItem.secondary_percent = 0;
	m_lastItem.bDrawHighlight = false;	
}
UIGridbox::UIGridbox()
: UIWindow()
, m_pTextBox( NULL )
, m_buttondown( false )
, m_rbuttondown( false )
, m_bItemDetect( true )
, m_bMultipleItems( false )
, m_bIgnoreItem( false )
, m_box_width( 0.0f )
, m_box_height( 0.0f )
, m_rows( 0 )
, m_columns( 0 )
, m_mouseX( 0 )
, m_mouseY( 0 )
, m_id_counter( 0 )
, m_bSwitch( false )
, m_pFont( NULL )
, m_bIsStore( false )
, m_bContinuePickup( true )
, m_bAutoTransfer( false )
, m_bAutoPlaceOnly( false )
, m_bGetJustPlaced( false )
, m_lastSort( SORT_FORWARD )
, m_dwRolloverColor( 0x666F6F6F )
, m_dwOverlapRolloverColor( 0x66FFFFFF )
, m_bConsumesItems( false )
, m_pickupId( 0 )
, m_bDeactivateOverlapped( false )
, m_rolloverHighlight( ITEM_INVALID )
, m_dwRolloverHighlight( 0x6600ff00 )
, m_bLocalPlace( true )
, m_bAutoPlaceNotify( true )
, m_rollover_id( 0 )
, m_dwColor( 0xffffffff )
, m_bRequestPickup( false )
, m_RolloverTimer( 0.0f )
{
	SetType( UI_TYPE_GRIDBOX );
	SetInputType( TYPE_INPUT_ALL );
	m_rollover_rect = UIRect();
	
	for ( int i = 0; i < m_rows; ++i ) 
	{
		for ( int j = 0; j < m_columns; ++j ) 
		{
			m_grid_unit[i][j].bOccupied		=	false;
			m_grid_unit[i][j].itemID		=	-1;
			m_grid_unit[i][j].rect.left		=	GetRect().left+((int)m_box_width*j);
			m_grid_unit[i][j].rect.right	=	m_grid_unit[i][j].rect.left + (int)m_box_width;
			m_grid_unit[i][j].rect.top		=	GetRect().top+((int)m_box_height*i);
			m_grid_unit[i][j].rect.bottom	=	m_grid_unit[i][j].rect.top + (int)m_box_height;
		}
	}
}

/**************************************************************************

Function		: ~UIGridbox()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIGridbox::~UIGridbox()
{	
}


void UIGridbox::Update( double secondsElapsed )
{
	if ( GetJustPlaced() )
	{
		SetJustPlaced( false );
	}

	if ( GetVisible() && GetRolloverTimer() != 0.0f )
	{
		SetRolloverTimer( GetRolloverTimer() - (float)secondsElapsed );
		if ( GetRolloverTimer() <= 0.0f )
		{
			SetRolloverTimer( 0.0f );
			gUIMessenger.Notify( "ui_grid_item_rollover", this );			
			PositionTextBox();
			if ( GetTextBox() )
			{
				GetTextBox()->SetVisible( true );
			}
		}
	}
}

/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UIGridbox::ProcessAction( UI_ACTION action, gpstring parameter )
{	
	UIWindow::ProcessAction( action, parameter );		
	return false;
}


/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: bool

**************************************************************************/
bool UIGridbox::ProcessMessage( UIMessage & msg )
{
	switch ( msg.GetCode() ) 
	{
	case MSG_LBUTTONDOWN:
		{
			m_buttondown = true;				
			ItemHitDetect();
		}
		break;
	case MSG_LBUTTONUP:
		{			
			m_buttondown = false;
		}
		break;
	case MSG_GLOBALLBUTTONUP:
		{
			m_buttondown = false;
		}
		break;
	case MSG_RBUTTONDOWN:
		{
			m_rbuttondown = true;
		}
		break;
	case MSG_RBUTTONUP:
		{
			if ( m_rbuttondown == true ) 
			{
				ItemUse();
			}
		}
		break;
	case MSG_GLOBALRBUTTONUP:
		{
			m_rbuttondown = false;
		}
		break;
	case MSG_ITEMACTIVATE:
		{
			m_bItemDetect = false;
		}
		break;
	case MSG_ITEMDEACTIVATE:	
		{
			if ( GetSwitch() == false ) 
			{
				m_bItemDetect = true;
			}
		}
		break;
	case MSG_ROLLOVER:
		{
			if ( GetVisible() == true ) 
			{
				RolloverDetect();								
				
				if ( GetScale() != 1.0f ) 
				{
					UIItemVec item_list;
					gUIShell.FindActiveItems( item_list );
					UIItemVec::iterator i;
					
					for ( i = item_list.begin(); i != item_list.end(); ++i ) 
					{			
						if ( ((UIItem *)(*i))->GetScale() != GetScale() ) 
						{
							((UIItem *)(*i))->SetScale( GetScale() );
						}
					}
				}
				
			}
		}
		break;
	case MSG_ROLLOFF:
		{
			if ( GetVisible() == true ) 
			{
				SetRolloverTimer( 0.0f );
				gUIMessenger.Notify( "ui_grid_item_rolloff", this );
				if ( GetTextBox() )
				{
					GetTextBox()->SetVisible( false );
				}
				
				
				if ( GetScale() != 1.0f ) 
				{
					UIItemVec item_list;
					gUIShell.FindActiveItems( item_list );
					UIItemVec::iterator i;

					for ( i = item_list.begin(); i != item_list.end(); ++i ) 
					{			
						if ( ((UIItem *)(*i))->GetScale() != 1.0f ) 
						{
							((UIItem *)(*i))->SetScale( 1.0f );
						}
					}
				}
				
			}
		}
		break;
	case MSG_ITEMDEACTIVATESWITCH:
		{
			SetSwitch( false );
		}
		break;
	}

	return true;
}


void UIGridbox::DetectItemDrop( int x, int y )
{
	if (( (int)x < GetRect().right	)	&& ( (int)x > GetRect().left		) &&
		( (int)y > GetRect().top	)	&& ( (int)y < GetRect().bottom	) && GetVisible() ) 
	{
		UIItemVec items; 
		gUIShell.FindActiveItems( items );
		if ( items.size() != 0 )
		{
			gUIMessenger.Notify( "multiple_item_drag", this );
		}		
	}
}



/**************************************************************************

Function		: RolloverDetect()

Purpose			: Detects if there was a rollover over an item and selects
				  the item if it detects one underneath the current cursor

Returns			: void

**************************************************************************/
void UIGridbox::RolloverDetect()
{
	int x	= gUIShell.GetMouseX();
	int y	= gUIShell.GetMouseY();

	std::vector< GridItem >::iterator i;
	for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i ) 
	{
		UIItem * pMaster = gUIShell.GetItem( (*i).itemID );
		if ( pMaster )
		{
			int total_row_width = pMaster->GetRows() * GetBoxHeight();
			int total_column_width = pMaster->GetColumns() * GetBoxWidth();
			if (( x < ((*i).rect.left + total_column_width)	)	&& ( x > (*i).rect.left						) &&
				( y > (*i).rect.top							)	&& ( y < ((*i).rect.top + total_row_width)	)) 
			{			
				if ( GetRolloverID() != (*i).itemID && GetTextBox() )
				{
					GetTextBox()->SetVisible( false );					
				}

				SetRolloverID( (*i).itemID );
				gUIShell.SetRolloverItemslotID( (*i).itemID );
				
				m_rollover_rect.left	= (*i).rect.left;
				m_rollover_rect.right	= (*i).rect.left + total_column_width;
				m_rollover_rect.top		= (*i).rect.top;
				m_rollover_rect.bottom	= (*i).rect.top + total_row_width;

				SetRolloverTimer( gUIShell.GetRolloverDelay() );
				
				if ( m_rolloverHighlight != (*i).itemID )
				{
					m_rolloverHighlight = ITEM_INVALID;
				}

				return;
			}
		}
	}

	m_rolloverHighlight = ITEM_INVALID;
	SetRolloverTimer( 0.0f );
	gUIMessenger.Notify( "ui_grid_item_rolloff", this );
	if ( GetTextBox() )
	{
		GetTextBox()->SetVisible( false );
	}

	// Return default rollover id
	SetRolloverID( 1 );
}


float UIGridbox::GetFullRatio()
{
	if ( GetColumns() == 0 || GetRows() == 0 )
	{
		return 0.0f;
	}

	int	boxesUsed = 0;
	std::vector< GridItem >::iterator i;
	for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i ) 
	{
		UIItem * pMaster = gUIShell.GetItem( (*i).itemID );
		if ( pMaster )
		{
			boxesUsed += pMaster->GetRows() * pMaster->GetColumns();
		}
	}

	return ((float)boxesUsed/(float)(GetColumns()*GetRows()));
}


/**************************************************************************

Function		: SetMousePos()

Purpose			: Sets the current mouse position and adjusts the cursor
				  rectangle accordingly

Returns			: void

**************************************************************************/
void UIGridbox::SetMousePos( unsigned int x, unsigned int y )
{
	m_mouseX = x;
	m_mouseY = y;
	
	if ( GetVisible() && gUIShell.IsInterfaceVisible( GetInterfaceParent() ) )
	{			
		if (( (int)x < GetRect().right	)	&& ( (int)x > GetRect().left		) &&
			( (int)y > GetRect().top	)	&& ( (int)y < GetRect().bottom	) && GetVisible() ) { 
			RolloverDetect();			
		}
	}
}


/**************************************************************************

Function		: ItemHitDetect()

Purpose			: Checks to see if there is a valid hit within the grid
				  and picks up an item if it is placed within the grid

Returns			: void

**************************************************************************/
void UIGridbox::ItemHitDetect()
{
	if (( m_bItemDetect == false ) || ( gUIShell.GetGridPlace() == false ) || GetJustPlaced() || m_bAutoPlaceOnly || gUIShell.GetItemActive() || !GetLocalPlace() ) 
	{
		return;
	}
	
	if ( GetConsumesItems() && gUIShell.GetItemActive() )
	{
		return;
	}

	std::vector< GridItem >::iterator i;
	for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i ) 
	{
		UIItem * pMaster = gUIShell.GetItem( (*i).itemID );
		if ( !pMaster )
		{
			continue;
		}

		int total_row_height = pMaster->GetRows() * GetBoxHeight();
		int total_column_width = pMaster->GetColumns() * GetBoxWidth();

		if (( m_mouseX < (*i).rect.left+total_column_width )	&& ( m_mouseX > (*i).rect.left		) &&
			( m_mouseY > (*i).rect.top	)						&& ( m_mouseY < (*i).rect.top+total_row_height )) {			

			m_pickupId = (*i).itemID;

			if ( GetStore() == true ) 
			{
				gUIMessenger.Notify( "check_buyer_gold", this );				
			}			

			if ( GetRequestPickup() == true )
			{
				gUIMessenger.Notify( "request_grid_pickup", this );			
			}

			if ( GetContinuePickup() == false ) 
			{
				return;
			}

			if ( GetAutoTransfer() )
			{
				gUIMessenger.Notify( "check_inventory_space", this );				
				m_lastItem = (*i);
			}

			if ( GetContinuePickup() == false ) {
				return;
			}

			if ( pMaster->GetScale() != GetScale() )
			{
				pMaster->SetScale( GetScale() );
			}			
			
			pMaster->ActivateItem( true );			
			pMaster->SetAcceptInput( false );
			pMaster->SetItemID( (*i).itemID );
			pMaster->SetMousePos( m_mouseX, m_mouseY );
			pMaster->SetNumStacked( (*i).numItems );
			pMaster->SetSecondaryPercentVisible( (*i).secondary_percent );
			for ( int j = 0; j < m_rows; ++j ) {
				for ( int k = 0; k < m_columns; ++k ) {
					if ( m_grid_unit[j][k].itemID == (*i).itemID  ) {
						m_grid_unit[j][k].bOccupied = false;
						m_grid_unit[j][k].itemID	= -1;
					}
				}
			}
			m_grid_items.erase( i );
			
			if ( GetScale() != pMaster->GetScale() )
			{
				pMaster->SetScale( GetScale() );
			}

			if ( GetAutoTransfer() )
			{				
				pMaster->ActivateItem( false );
				gUIMessenger.Notify( "transfer_inventory", this );
			}
			else
			{
				if ( GetConsumesItems() )
				{
					SetJustPlaced( true );
				}

				m_bItemDetect = false;				
				gUIMessenger.Notify( "item_pickup", this );				
			}					
			
			return;			
		}
	}
}



// Use an item, if possible
void UIGridbox::ItemUse()
{
	std::vector< GridItem >::iterator i;
	for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i ) 
	{
		if (( m_mouseX < (*i).rect.right	)	&& ( m_mouseX > (*i).rect.left		) &&
			( m_mouseY > (*i).rect.top	)		&& ( m_mouseY < (*i).rect.bottom	)) 
		{			
			gUIMessenger.Notify( "item_use", this );	
			return;			
		}
	}
}



/**************************************************************************

Function		: GridHitDetect()

Purpose			: Checks to see if there is a valid hit within the grid
				  and that there is space within the grid.

Returns			: void

**************************************************************************/
bool UIGridbox::GridHitDetect( int x, int y, UIItem *pItem, bool bNotify, bool bSnap, bool bAutoPlace, bool bOverride )
{
	if ( !gUIShell.GetGridPlace() || !(GetLocalPlace() || bAutoPlace) )
	{
		return false;
	}

	if ( !bAutoPlace && m_bAutoPlaceOnly && !bOverride ) 
	{		
		return false;
	}

	if ( pItem->GetScale() != GetScale() && pItem->GetTextureIndex() != 0 )
	{
		pItem->SetScale( GetScale() );
	}

	SetIgnoreItem( false );
	SetContinuePickup( true );

	if ( GetMultipleItems() == true ) 
	{
		gUIMessenger.Notify( "multiple_item_drag", this );
		return false;
	}

	if ( GetConsumesItems() && !bAutoPlace )
	{
		m_lastItem.itemID				= pItem->GetItemID();		
		m_lastItem.alpha				= pItem->GetAlpha();
		m_lastItem.numItems				= pItem->GetNumStacked();
		m_lastItem.secondary_percent	= pItem->GetSecondaryPercentVisible();
		m_lastItem.bDrawHighlight		= false;
		m_lastItem.rect					= UIRect();

		SetContinuePickup( true );
		m_pickupId = pItem->GetItemID();
		gUIMessenger.Notify( "grid_request_placement", this );
		if ( GetContinuePickup() )
		{
			SetJustPlaced( true );
			pItem->ActivateItem( false );
			gUIMessenger.Notify( "item_placement", this );					
		}
		return true;
	}
	
	std::vector< POINT > occupation_areas;
	
	int i, j;
	POINT area;
	area.x = 0;
	area.y = 0;

	bool bSet = false;
	double closest = 65535;
	for ( i = 0; i < m_rows; ++i ) {
		for ( j = 0; j < m_columns; ++j ) {
			double dist = PointDistance(	m_grid_unit[i][j].rect.left,
											m_grid_unit[i][j].rect.top,
											x,
											y	);
			if ( dist < closest ) {
				if ( bAutoPlace == false ) {
					closest = dist;
					area.x = j;
					area.y = i;
					bSet = true;
				}
				else {
					if ( m_grid_unit[i][j].bOccupied == false ) {
						closest = dist;
						area.x = j;
						area.y = i;
						bSet = true;
					}
				}
			}
		}
	}

	if ( !bSet ) {
		return false;
	}

	j = area.x;
	i = area.y;	

	int height	= pItem->GetRows();
	int width	= pItem->GetColumns();

	if ( (j + width ) > m_columns ) {
		return false;
	}
	if ( (i + height ) > m_rows ) {
		return false;
	}

	int occupantID = -1;
	int o = 0;
	int l = 0;
	for ( o = 0; o < width; o++ ) {
		for ( l = 0; l < height; l++ ) {
			if ( m_grid_unit[i+l][j+o].bOccupied == true ) {
				if (( m_grid_unit[i+l][j+o].itemID != occupantID ) && ( occupantID != -1 )) {
					return false;
				}
				if ( bAutoPlace == true ) {
					return false;
				}
			}			
						
			if ( m_grid_unit[i+l][j+o].itemID != -1 ) {
				occupantID = m_grid_unit[i+l][j+o].itemID;
			}
			area.x = i+l;
			area.y = j+o;
			occupation_areas.push_back( area );		
		}		
	}	
	
	if ( bSnap == true ) {

		if ( ContainsID( pItem->GetItemID() ) )
		{
			gperror( ( "The gridbox already contains this id.  Tell Chad." ) );
			pItem->ActivateItem( false );
			return false;
		}
		
		GridItem new_item;
		new_item.itemID		= pItem->GetItemID();		
		new_item.alpha		= pItem->GetAlpha();
		new_item.numItems	= pItem->GetNumStacked();
		new_item.secondary_percent = pItem->GetSecondaryPercentVisible();
		new_item.dwHighlightColor = 0;
		new_item.dwSecondaryColor = 0;
		new_item.bDrawHighlight = false;		

		// Now that we know the area that we wish to occupy is valid,
		// let's take all those grid boxes and fill in the occupant information.
		PickupOverlapped( occupantID, pItem, bNotify );

		if ( GetContinuePickup() == false )
		{
			SetIgnoreItem( false );
			SetContinuePickup( true );
			return false;
		}

		pItem->SetParentWindow( this );

		std::vector< POINT >::iterator k;
		for ( k = occupation_areas.begin(); k != occupation_areas.end(); ++k ) {
			m_grid_unit[(*k).x][(*k).y].bOccupied	= true;
			m_grid_unit[(*k).x][(*k).y].itemID		= new_item.itemID;					
		}

		// Also give the item its new rectangle position
		int item_width	= pItem->GetRect().right	-	pItem->GetRect().left;
		int item_height	= pItem->GetRect().bottom	-	pItem->GetRect().top;
		
		new_item.rect.left		= m_grid_unit[i][j].rect.left; 
		new_item.rect.right		= m_grid_unit[i][j].rect.left + item_width;
		new_item.rect.top		= m_grid_unit[i][j].rect.top;
		new_item.rect.bottom	= m_grid_unit[i][j].rect.top + item_height;		
		
		m_grid_items.push_back( new_item );
			
		m_lastItem = new_item;
		if ( occupantID == -1 ) 
		{			
			SetSwitch( false );
			if ( GetAutoPlaceNotify() )
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_ITEMDEACTIVATESWITCH ) );
			}
		}
		else 
		{
			SetSwitch( true );
			SetSwitchItem( new_item );		
		}		

		SetJustPlaced( true );
		
		if ( bNotify == true ) 
		{
			if ( occupantID != -1 ) 
			{
				m_pickupId = m_lastItem.itemID;
				gUIMessenger.Notify( "item_pickup", this );
			}
			gUIMessenger.Notify( "item_placement", this );								
		}
	}

	return true;
}


/**************************************************************************

Function		: PickupOverlapped()

Purpose			: If there is another item underneath it, pick it up

Returns			: bool

**************************************************************************/
bool UIGridbox::PickupOverlapped( int occupantID, UIItem *pItem, bool bNotify )
{
	pItem->ActivateItem( false );

	if ( occupantID == -1 ) 
	{
		return false;
	}

	if ( GetStore() == true ) 
	{
		SetRolloverID( occupantID );
		gUIMessenger.Notify( "ui_grid_item_rollover", this );
		PositionTextBox();
		if ( GetTextBox() )
		{
			GetTextBox()->SetVisible( true );
		}
		gUIMessenger.Notify( "check_buyer_gold", this );
	}
	else if ( bNotify )
	{		
		gUIMessenger.Notify( "request_grid_pickup", this );			
	}

	if ( GetContinuePickup() == false ) 
	{
		if ( pItem && !GetIgnoreItem() )
		{
			pItem->ActivateItem( true );
		}

		return false;
	}

	std::vector< GridItem >::iterator i;
	for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i ) 
	{
		if ( (*i).itemID == occupantID ) 
		{	
			UIItem * pMaster = gUIShell.GetItem( (*i).itemID );
			if ( !pMaster )
			{
				continue;
			}

			if ( pMaster->GetScale() != GetScale() )
			{
				pMaster->SetScale( GetScale() );
			}
			
			pMaster->ActivateItem( GetDeactivateOverlapped() ? false : true );			
			pMaster->SetAcceptInput( false );
			pMaster->SetItemID( (*i).itemID );
			pMaster->SetMousePos( m_mouseX, m_mouseY );
			pMaster->SetNumStacked( (*i).numItems );
			pMaster->SetSecondaryPercentVisible( (*i).secondary_percent );
			for ( int j = 0; j < m_rows; ++j ) 
			{
				for ( int k = 0; k < m_columns; ++k ) 
				{
					if ( m_grid_unit[j][k].itemID == (*i).itemID  )
					{
						m_grid_unit[j][k].bOccupied = false;
						m_grid_unit[j][k].itemID	= -1;
					}
				}
			}			
			
			m_grid_items.erase( i );
			return true;			
		}	
	}
	return false;
}


void UIGridbox::Sort( eGridSort sort )
{	
	if ( gUIShell.GetGridPlace() == false || GetLocalPlace() == false )
	{
		return;
	}

	std::vector< GridItem > originalItems = m_grid_items;
	ResetGrid();
	bool bSorted = true;
	if ( sort == SORT_FORWARD )
	{
		std::vector< GridItem >::iterator i;
		for ( i = originalItems.begin(); i != originalItems.end(); ++i )
		{
			UIItem * pMaster = gUIShell.GetItem( (*i).itemID );
			if ( pMaster )
			{
				if ( AutoItemPlacement( pMaster->GetName(), true, (*i).itemID, (*i).secondary_percent ) == false )
				{				
					bSorted = false;
					break;
				}
			}
		}
	}
	else if ( sort == SORT_REVERSE )
	{
		std::vector< GridItem >::reverse_iterator i;
		for ( i = originalItems.rbegin(); i != originalItems.rend(); ++i )
		{
			UIItem * pMaster = gUIShell.GetItem( (*i).itemID );
			if ( pMaster )
			{
				if ( AutoItemPlacement( pMaster->GetName(), true, (*i).itemID, (*i).secondary_percent ) == false )
				{		
					bSorted = false;
					break;
				}
			}
		}		
	}

	if ( !bSorted )
	{
		AssembleGrid( originalItems, true );
	}
}


void UIGridbox::AutoSort()
{
	Sort( SORT_REVERSE );
}


/**************************************************************************

Function		: Draw()

Purpose			: Draw the gridbox control with all its child item

Returns			: void

**************************************************************************/
void UIGridbox::Draw()
{
	UIWindow::Draw();
	std::vector< GridItem >::iterator i;
	for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i ) 
	{		
		DrawItem( (*i) );
	}

	if ( GetVisible() )
	{
		DrawRolloverBoxes();
	}

	// Code below is for TESTING only
	/*
	for ( int j = 0; j < m_rows; ++j ) {
		for ( int k = 0; k < m_columns; ++k ) {
			if (( GetVisible() == true ) && ( HasTexture() ) && (m_grid_unit[j][k].bOccupied == true )) {
				unsigned int test = GetTextureman()->LoadTexture( "" );
				GetRenderer()->SetTextureStageState(	0,
														test != 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE,
														D3DTOP_BLENDDIFFUSEALPHA,
														D3DTADDRESS_WRAP,
														D3DTADDRESS_WRAP,
														D3DTEXF_POINT,	
														D3DTEXF_POINT,
														D3DTEXF_POINT,
														D3DBLEND_SRCALPHA,
														D3DBLEND_INVSRCALPHA,
														true );
			
						
				tVertex Verts[4];
				memset( Verts, 0, sizeof( tVertex ) * 4 );

				vector_3 default_color( 1.0f, 1.0f, 1.0f );
				DWORD color	= MAKEDWORDCOLOR( default_color );
				color		&= 0x00FFFFFF;

				Verts[0].x			= m_grid_unit[j][k].rect.left;
				Verts[0].y			= m_grid_unit[j][k].rect.top;				
				Verts[0].z			= 0.0f;
				Verts[0].uv.u	= 0;
				Verts[0].uv.v	= 0;
				Verts[0].color		= (color | (((BYTE)((GetAlpha())*255.0f))<<24));
				Verts[0].rhw		= 1.0f;

				Verts[1].x			= m_grid_unit[j][k].rect.left;
				Verts[1].y			= m_grid_unit[j][k].rect.bottom;
				Verts[1].z			= 0.0f;
				Verts[1].uv.u	= 0;
				Verts[1].uv.v	= 1;
				Verts[1].color		= (color | (((BYTE)((GetAlpha())*255.0f))<<24));
				Verts[1].rhw		= 1.0f;

				Verts[2].x			= m_grid_unit[j][k].rect.right;
				Verts[2].y			= m_grid_unit[j][k].rect.top;
				Verts[2].z			= 0.0f;
				Verts[2].uv.u	= 1;
				Verts[2].uv.v	= 0;
				Verts[2].color		= (color | (((BYTE)((GetAlpha())*255.0f))<<24));
				Verts[2].rhw		= 1.0f;

				Verts[3].x			= m_grid_unit[j][k].rect.right;
				Verts[3].y			= m_grid_unit[j][k].rect.bottom;
				Verts[3].z			= 0.0f;
				Verts[3].uv.u	= 1;
				Verts[3].uv.v	= 1;
				Verts[3].color		= (color | (((BYTE)((GetAlpha())*255.0f))<<24));
				Verts[3].rhw		= 1.0f;
	
				GetRenderer()->DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &test, 1 );
			}
		}
	}
	*/

}


void UIGridbox::DrawRolloverBoxes()
{
	if ( m_bAutoPlaceOnly )
	{
		return;
	}

	POINT mousePt;
	mousePt.x = gUIShell.GetMouseX();
	mousePt.y = gUIShell.GetMouseY();
	if ( GetRect().Contains( mousePt ) )
	{
		UIItemVec items;
		gUIShell.FindActiveItems( items );
		if ( items.size() == 1 )
		{			
			UIItem * pItem = (*(items.begin()));
			
			if ( pItem->GetScale() != GetScale() )
			{
				pItem->SetScale( GetScale() );
			}

			int x = pItem->GetRect().left;
			int y = pItem->GetRect().top;	
			std::vector< POINT > occupation_areas;
		
			int i, j;
			POINT area;
			area.x = 0;
			area.y = 0;

			bool bSet = false;
			double closest = 65535;
			for ( i = 0; i < m_rows; ++i ) {
				for ( j = 0; j < m_columns; ++j ) {
					double dist = PointDistance(	m_grid_unit[i][j].rect.left,
													m_grid_unit[i][j].rect.top,
													x,
													y	);
					if ( dist < closest ) 
					{	
						closest = dist;
						area.x = j;
						area.y = i;
						bSet = true;
					}			
				}
			}

			if ( !bSet ) {
				return;
			}

			j = area.x;
			i = area.y;	

			int height	= pItem->GetRows();
			int width	= pItem->GetColumns();

			if ( (j + width ) > m_columns ) {
				return;
			}
			if ( (i + height ) > m_rows ) {
				return;
			}

			int occupantID = -1;
			int o = 0;
			int l = 0;
			for ( o = 0; o < width; o++ ) {
				for ( l = 0; l < height; l++ ) {
					if ( m_grid_unit[i+l][j+o].bOccupied == true ) {
						if (( m_grid_unit[i+l][j+o].itemID != occupantID ) && ( occupantID != -1 )) {
							return;
						}
					}			
								
					if ( m_grid_unit[i+l][j+o].itemID != -1 ) {
						occupantID = m_grid_unit[i+l][j+o].itemID;
					}
					area.x = i+l;
					area.y = j+o;
					occupation_areas.push_back( area );		
				}		
			}

			std::vector< POINT >::iterator iPts;
			for ( iPts = occupation_areas.begin(); iPts != occupation_areas.end(); ++iPts )
			{
				if ( m_grid_unit[(*iPts).x][(*iPts).y].bOccupied )
				{
					gUIShell.DrawColorBox( m_grid_unit[(*iPts).x][(*iPts).y].rect, m_dwOverlapRolloverColor );					
				}
				else
				{
					gUIShell.DrawColorBox( m_grid_unit[(*iPts).x][(*iPts).y].rect, m_dwRolloverColor );
				}
			}
		}
	}
}


/**************************************************************************

Function		: DrawItem()

Purpose			: Draw an item in the gridbox

Returns			: void

**************************************************************************/
void UIGridbox::DrawItem( GridItem item )
{
	if ( GetVisible() == true ) 
	{
		UIItem * pMaster = gUIShell.GetItem( item.itemID );
		if ( !pMaster )
		{
			return;
		}

		if ( pMaster->GetParentWindow() != this )
		{
			pMaster->SetParentWindow( this );
		}

		if ( item.bDrawHighlight )
		{
			UIRect colorBox;
			colorBox.left = item.rect.left;
			colorBox.top = item.rect.top;
			colorBox.right = colorBox.left + (int)((float)( pMaster->GetColumns() * pMaster->GetBoxWidth() ) * GetScale() );
			colorBox.bottom = colorBox.top + (int)((float)( pMaster->GetRows() * pMaster->GetBoxHeight() ) * GetScale() );			
			gUIShell.DrawColorBox( colorBox,	item.dwHighlightColor, 
												item.dwSecondaryColor ? item.dwSecondaryColor : item.dwHighlightColor );
		}

		if ( pMaster->GetTextureIndex() == 0 )
		{
			pMaster->ReloadTextures();
		}

		// Draw any secondary images, if they exist
		pMaster->DrawSecondaryItem( item.rect, item.secondary_percent );

		gUIShell.GetRenderer().SetTextureStageState(	0,
														pMaster->GetTextureIndex() != 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE,
														D3DTOP_MODULATE,
														D3DTADDRESS_CLAMP, // WRAP
														D3DTADDRESS_CLAMP,
														D3DTEXF_POINT, //LINEAR
														D3DTEXF_POINT,
														D3DTEXF_POINT,
														D3DBLEND_SRCALPHA,
														D3DBLEND_INVSRCALPHA,
														true );

		tVertex Verts[4];
		memset( Verts, 0, sizeof( tVertex ) * 4 );

		vector_3 default_color( 1.0f, 1.0f, 1.0f );
		DWORD color	= MAKEDWORDCOLOR( default_color );
		color		&= 0x00FFFFFF;

		Verts[0].x			= item.rect.left-0.5f;
		Verts[0].y			= item.rect.top-0.5f;
		Verts[0].z			= 0.0f;
		Verts[0].rhw		= 1.0f;
		Verts[0].uv.u		= 0.0f;
		Verts[0].uv.v		= 1.0f;
		Verts[0].color		= (color | (((BYTE)((item.alpha)*255.0f))<<24));

		Verts[1].x			= item.rect.left-0.5f;
		Verts[1].y			= item.rect.bottom-0.5f;
		Verts[1].z			= 0.0f;
		Verts[1].rhw		= 1.0f;
		Verts[1].uv.u		= 0.0f;
		Verts[1].uv.v		= 0.0f;
		Verts[1].color		= (color | (((BYTE)((item.alpha)*255.0f))<<24));

		Verts[2].x			= item.rect.right-0.5f;
		Verts[2].y			= item.rect.top-0.5f;
		Verts[2].z			= 0.0f;
		Verts[2].rhw		= 1.0f;
		Verts[2].uv.u		= 1.0f;
		Verts[2].uv.v		= 1.0f;
		Verts[2].color		= (color | (((BYTE)((item.alpha)*255.0f))<<24));

		Verts[3].x			= item.rect.right-0.5f;
		Verts[3].y			= item.rect.bottom-0.5f;
		Verts[3].z			= 0.0f;
		Verts[3].rhw		= 1.0f;
		Verts[3].uv.u		= 1.0f;
		Verts[3].uv.v		= 0.0f;
		Verts[3].color		= (color | (((BYTE)((item.alpha)*255.0f))<<24));

		unsigned int texture_index = pMaster->GetTextureIndex();
		gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &texture_index, 1 );

		if ( item.numItems > 1 ) 
		{
			char szNumber[32] = "";
			sprintf( szNumber, "%d", item.numItems );
			int width	= 0;
			int height	= 0;

			if ( m_pFont ) 
			{
				m_pFont->CalculateStringSize( szNumber, width, height );
				m_pFont->Print( item.rect.right-width, item.rect.bottom-height, szNumber, GetColor(), true );
			}
		}
		
		if ( m_rolloverHighlight == item.itemID )
		{
			UIRect colorBox;
			colorBox.left = item.rect.left;
			colorBox.top = item.rect.top;
			colorBox.right = colorBox.left + (int)((float)( pMaster->GetColumns() * pMaster->GetBoxWidth() ) * GetScale() );
			colorBox.bottom = colorBox.top + (int)((float)( pMaster->GetRows() * pMaster->GetBoxHeight() ) * GetScale() );
			gUIShell.DrawColorBox( colorBox, m_dwRolloverHighlight );
		}
	}
}




/**************************************************************************

Function		: SetRect()

Purpose			: Set the grid rectangle and it's box rectangles

Returns			: void

**************************************************************************/
void UIGridbox::SetRect( int left, int right, int top, int bottom, bool bAnimation )
{	
	UNREFERENCED_PARAMETER( bAnimation );
	
	int xdelta = left - GetRect().left;
	int ydelta = top - GetRect().top;
	
	UIWindow::SetRect( left, right, top, bottom );	
	for ( int i = 0; i < m_rows; ++i ) 
	{
		for ( int j = 0; j < m_columns; ++j ) 
		{
			int abs_box_width	= (int)m_box_width;
			int abs_box_height	= (int)m_box_height;
			m_grid_unit[i][j].rect.left		=	GetRect().left+(abs_box_width*j);
			m_grid_unit[i][j].rect.right	=	m_grid_unit[i][j].rect.left + abs_box_width;
			m_grid_unit[i][j].rect.top		=	GetRect().top+(abs_box_height*i);
			m_grid_unit[i][j].rect.bottom	=	m_grid_unit[i][j].rect.top + abs_box_height;
		}
	}


	std::vector< GridItem >::iterator k;
	for ( k = m_grid_items.begin(); k != m_grid_items.end(); ++k ) 
	{
		int width	= (*k).rect.right - (*k).rect.left;
		int height	= (*k).rect.bottom - (*k).rect.top;
		(*k).rect.left		=	(*k).rect.left + xdelta;
		(*k).rect.right		=	(*k).rect.left + width;
		(*k).rect.top		=	(*k).rect.top + ydelta;
		(*k).rect.bottom	=	(*k).rect.top + height;
	}	
}



void UIGridbox::SetScale( float scale )
{
	float currentScale = GetScale();	
	UIWindow::SetScale( scale );
	scale = scale / currentScale;

	int rect_width = (int)((GetRect().right - GetRect().left) * scale);
	int rect_height = (int)((GetRect().bottom - GetRect().top) * scale);
	
	m_box_width *= scale;
	m_box_height *= scale;

	for ( int i = 0; i < m_rows; ++i ) {
		for ( int j = 0; j < m_columns; ++j ) {			
			m_grid_unit[i][j].rect.left		=	(int)(GetRect().left+(m_box_width*j));
			m_grid_unit[i][j].rect.right	=	(int)(m_grid_unit[i][j].rect.left + m_box_width);
			m_grid_unit[i][j].rect.top		=	(int)(GetRect().top+(m_box_width*i));
			m_grid_unit[i][j].rect.bottom	=	(int)(m_grid_unit[i][j].rect.top + m_box_width);
		}
	}

	std::vector< GridItem >::iterator k;
	for ( k = m_grid_items.begin(); k != m_grid_items.end(); ++k ) {
		int x_change = (int)(((*k).rect.left - GetRect().left) * scale);
		int y_change = (int)(((*k).rect.top -  GetRect().top) * scale);
		
		int width	= (int)(((*k).rect.right - (*k).rect.left)*scale);
		int height	= (int)(((*k).rect.bottom - (*k).rect.top)*scale);
		(*k).rect.left		=	GetRect().left + x_change;
		(*k).rect.right		=	(*k).rect.left + width;
		(*k).rect.top		=	GetRect().top + y_change;
		(*k).rect.bottom	=	(*k).rect.top + height;
	}

	UIWindow::SetRect( GetRect().left, GetRect().left+rect_width, GetRect().top, GetRect().top+rect_height );	

	/*
	if ( GetScale() != 1.0f ) {
		UIWindowVec item_list = gUIShell.ListWindowsOfType( UI_TYPE_ITEM );
		UIWindowVec::iterator o;
		for ( o = item_list.begin(); o != item_list.end(); ++o ) {			
			if ( ((UIItem *)(*o))->GetScale() != GetScale() ) {
				((UIItem *)(*o))->SetScale( GetScale() );
			}
		}
	}
	*/
}



/**************************************************************************

Function		: ResetGrid()

Purpose			: Clear all items out of the grid

Returns			: void

**************************************************************************/
void UIGridbox::ResetGrid()
{	
	{
		std::vector< GridItem >::iterator i;
		for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i ) 
		{ 
			UIItem * pMaster = gUIShell.GetItem( (*i).itemID );
			if ( pMaster )
			{
				pMaster->ActivateItem( false );
				pMaster->RemoveItemParent( this );
			}
		}
	}

	m_grid_items.clear();
	for ( int i = 0; i < m_rows; ++i ) 
	{
		for ( int j = 0; j < m_columns; ++j ) 
		{
			m_grid_unit[i][j].bOccupied		=	false;
			m_grid_unit[i][j].itemID		=	-1;
		}
	}
}


void UIGridbox::PurgeItems()
{
	std::vector< GridItem >::iterator i;
	for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i ) 
	{ 
		UIItem * pMaster = gUIShell.GetItem( (*i).itemID );
		if ( pMaster )
		{
			pMaster->ActivateItem( false );
			pMaster->RemoveItemParent( this );
		}
	}
}


/**************************************************************************

Function		: AssembleGrid()

Purpose			: Place the given items within the current gridbox

Returns			: void

**************************************************************************/
void UIGridbox::AssembleGrid( std::vector< GridItem > grid_items, bool bClear )
{
	if ( bClear == true ) {
		ResetGrid();
	}
	std::vector< GridItem >::iterator i;
	for ( i = grid_items.begin(); i != grid_items.end(); ++i ) 
	{		
		AddGridItem( *i );
	}
	m_bItemDetect = true;
}


void UIGridbox::AssembleGrid( PersistGridItemColl & gridItems )
{
	ResetGrid();

	m_tempPersistItems.clear();
	m_tempPersistItems = gridItems;

	m_grid_items.clear();
	gUIMessenger.Notify( "convert_persist_items", this );

	AssembleGrid( m_grid_items, true );
}


/**************************************************************************

Function		: AddGridItem()

Purpose			: Add an item to the current gridbox

Returns			: void

**************************************************************************/
void UIGridbox::AddGridItem( GridItem item )
{	
	UIItem * pMaster = gUIShell.GetItem( item.itemID );
	if ( pMaster )
	{
		pMaster->SetItemID( item.itemID );
		GridHitDetect( item.rect.left, item.rect.top, pMaster, false, true, false, true  );		
	}
}


/**************************************************************************

Function		: AutoItemPlacement()

Purpose			: Automatically place an item

Returns			: bool

**************************************************************************/
bool UIGridbox::AutoItemPlacement( const char * item_name, bool bSnap, unsigned int id, float secPercent )
{
	UIWindow *pWindow	= gUIShell.FindUIWindow( item_name );
	UIItem *pItem		= (UIItem *)pWindow;

	bool bGridPlace = gUIShell.GetGridPlace();
	gUIShell.SetGridPlace( true );
	bool bLocalPlace = GetLocalPlace();
	SetLocalPlace( true );

	if ( !pItem ) {
		gpassertm( 0, "Not auto-placing a valid item, most likely an inventoryicon content issue" );
		return false;
	}

	if ( secPercent != -1.0 )
	{
		pItem->SetSecondaryPercentVisible( secPercent );
	}

	if ( id != 0 ) {
		pItem->SetItemID( id );
	}	

	for ( int i = 0; i < m_rows; ++i ) {
		for ( int j = 0; j < m_columns; ++j ) {			
			if ( GridHitDetect(	m_grid_unit[i][j].rect.left, m_grid_unit[i][j].rect.top,
								pItem, false, bSnap, true ) ) {
				gUIShell.SetGridPlace( bGridPlace );

				if ( bSnap && m_bAutoPlaceNotify )
				{
					gUIMessenger.Notify( "grid_autoplaced", this );
				}

				return true;
			}
		}
	}		

	gUIShell.SetGridPlace( bGridPlace );
	SetLocalPlace( bLocalPlace );
	return false;
}


/**************************************************************************

Function		: ContainsID()

Purpose			: Does the gridbox contain a particular id

Returns			: bool

**************************************************************************/
bool UIGridbox::ContainsID( unsigned int id )
{
	std::vector< GridItem >::iterator i;
	for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i ) {
		if ( (*i).itemID == (int)id ) {
			return true;
		}
	}
	return false;
}


/**************************************************************************

Function		: RemoveID()

Purpose			: Remove a grid item by id

Returns			: void

**************************************************************************/
UIItem * UIGridbox::RemoveID( unsigned int id )
{
	std::vector< GridItem >::iterator i;
	for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i ) {
		if ( (*i).itemID == (int)id ) 
		{	
			UIItem * pMaster = gUIShell.GetItem( (*i).itemID );
			if ( pMaster )
			{			
				for ( int j = 0; j < m_rows; ++j ) 
				{
					for ( int k = 0; k < m_columns; ++k ) 
					{
						if ( m_grid_unit[j][k].itemID == (*i).itemID  ) 
						{
							m_grid_unit[j][k].bOccupied = false;
							m_grid_unit[j][k].itemID	= -1;
						}
					}
				}			
				m_grid_items.erase( i );

				RolloverDetect();

				return pMaster;
			}
		}
	}	
	return 0;
}


void UIGridbox::SetSecondaryPercentVisible( int id, float percent ) 
{
	std::vector< GridItem >::iterator i;
	for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i ) 
	{
		if ( (*i).itemID == id ) 
		{
			(*i).secondary_percent = percent;
		}
	}	
}


void UIGridbox::ResizeToCurrentResolution( int original_width, int original_height )
{
	UIWindow::ResizeToCurrentResolution( original_width, original_height );
}


/**************************************************************************

Function		: PointDistance()

Purpose			: Distance between two points

Returns			: double

**************************************************************************/
double PointDistance( int x1, int y1, int x2, int y2 )
{
	return sqrt( pow((x2-x1), 2) + pow((y2-y1), 2) );
}


#if !GP_RETAIL
void UIGridbox::Save( FuelHandle hSave, bool bChild )
{
	UIWindow::Save( hSave, bChild );
	if ( GetParentWindow() && ( bChild == false ))
	{
		return;
	}	

	if ( hSave.IsValid() )
	{
		FuelHandle hWindow = hSave->GetChildBlock( GetName(), true );
		if ( hWindow.IsValid() )
		{
			hWindow->SetType( "gridbox" );
			hWindow->Set( "box_width", m_box_width );
			hWindow->Set( "box_height", m_box_height );
			hWindow->Set( "rows", m_rows );
			hWindow->Set( "columns", m_columns );
			hWindow->Set( "grid_type", m_sGridType );
		}
	}
}
#endif


void UIGridbox::PositionTextBox()
{
	if ( GetTextBox() )
	{
		int width	= GetTextBox()->GetRect().right - GetTextBox()->GetRect().left;
		int height	= GetTextBox()->GetRect().bottom - GetTextBox()->GetRect().top;

		int rollover_width	= m_rollover_rect.right		- m_rollover_rect.left;		

		GetTextBox()->GetRect().left = (m_rollover_rect.right - ( rollover_width/2 )) - (width/2);
		GetTextBox()->GetRect().right = GetTextBox()->GetRect().left + width;
		GetTextBox()->GetRect().bottom = m_rollover_rect.top;
		GetTextBox()->GetRect().top = GetTextBox()->GetRect().bottom - height;

		if ( GetTextBox()->GetRect().left < 0 )
		{
			GetTextBox()->GetRect().left = 0;
			GetTextBox()->GetRect().right = width; 
		}

		if ( GetTextBox()->GetRect().right > gUIShell.GetScreenWidth() )
		{
			GetTextBox()->GetRect().right = gUIShell.GetScreenWidth();
			GetTextBox()->GetRect().left = gUIShell.GetScreenWidth() - width;
		}

		if ( GetTextBox()->GetRect().top < 0 )
		{
			GetTextBox()->GetRect().top = m_rollover_rect.bottom;
			GetTextBox()->GetRect().bottom = GetTextBox()->GetRect().top + height;
		}							   
	}
}


UITextBox * UIGridbox::GetTextBox()
{
	if ( !m_pTextBox )
	{
		m_pTextBox = (UITextBox *)gUIShell.FindUIWindow( "gui_rollover_textbox" );
	}
	return m_pTextBox;
}


PersistGridItemColl & UIGridbox::PackagePersistGridItems()
{
	float scale = GetScale();
	SetScale( 1.0f );

	PersistGridItemColl gridItems;
	
	GridItemColl::iterator i;
	for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i )
	{
		PersistGridItem persistItem;
		persistItem.alpha = (*i).alpha;
		persistItem.itemGoid = (*i).itemID;
		persistItem.rect = (*i).rect;
		persistItem.secondary_percent = (*i).secondary_percent;
		persistItem.xoffset = GetRect().left;
		persistItem.yoffset = GetRect().top;
		gridItems.push_back( persistItem );
	}	

	m_tempPersistItems.clear();
	m_tempPersistItems = gridItems;	

	SetScale( scale );

	return m_tempPersistItems;
}

bool UIGridbox::GetItem( int id, GridItem & item )
{
	GridItemColl::iterator i;
	for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i )
	{
		if ( (*i).itemID == id )
		{
			item = *i;
			return true;
		}
	}	

	return false;
}

void UIGridbox::ActivateHighlight( int item, DWORD dwFirstColor, DWORD dwSecondColor )
{
	GridItemColl::iterator i;
	for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i )
	{
		if ( item == (*i).itemID ) 
		{
			(*i).dwHighlightColor	= dwFirstColor;
			(*i).dwSecondaryColor	= dwSecondColor;
			(*i).bDrawHighlight		= true;
			return;
		}
	}
}

void UIGridbox::ActivateHighlight( int item, DWORD dwColor )
{
	ActivateHighlight( item, dwColor, dwColor );
}


void UIGridbox::ResetHighlights()
{
	GridItemColl::iterator i;
	for ( i = m_grid_items.begin(); i != m_grid_items.end(); ++i )
	{
		(*i).bDrawHighlight	= false;
	}
}


void UIGridbox::Xfer( FuBi::BitPacker& packer )
{
	for ( int j = 0; j < m_rows; ++j ) 
	{
		for ( int k = 0; k < m_columns; ++k ) 
		{
			packer.XferBit( m_grid_unit[j][k].bOccupied );
		}
	}				
}