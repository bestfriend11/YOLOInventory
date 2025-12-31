/**************************************************************************

Filename		: ui_infoslot.cpp

Description		: Control for the slots that hold a certain type of item
				  and its information

Creation Date	: 11/10/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_infoslot.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"
#include "ui_item.h"
#include "ui_textbox.h"



/**************************************************************************

Function		: UIInfoSlot()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIInfoSlot::UIInfoSlot( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: UIWindow( fhWindow, parent_interface, parent )
, m_buttondown( false )
, m_bItemDetect( true )
, m_item( 0 )
, m_id( 0 )
, m_bItemPlaced( false )
, m_bPermanent( false )
, m_bCanEquip( true )
, m_attemptEquipId( 0 )
, m_pTextBox( 0 )
, m_invalidColor( 0x55ff0000 )
, m_bInvalid( false )
, m_dwHighlightColor( 0x666F6F6F )
, m_bHighlight( false )
, m_bRemoving( false )
, m_bCanRemove( true )
, m_RolloverTimer( 0.0f )
{	
	SetType( UI_TYPE_INFOSLOT );
	SetInputType( TYPE_INPUT_ALL );	
	fhWindow.Get( "slot_type", m_sSlotType );
}
UIInfoSlot::UIInfoSlot()
: UIWindow()
, m_buttondown( false )
, m_bItemDetect( true )
, m_item( 0 )
, m_id( 0 )
, m_bItemPlaced( false )
, m_bPermanent( false )
, m_bCanEquip( true )
, m_attemptEquipId( 0 )
, m_pTextBox( 0 )
, m_invalidColor( 0x55ff0000 )
, m_bInvalid( false )
, m_dwHighlightColor( 0x666F6F6F )
, m_bHighlight( false )
, m_bRemoving( false )
, m_bCanRemove( true )
, m_RolloverTimer( 0.0f )
{	
	SetType( UI_TYPE_INFOSLOT );
	SetInputType( TYPE_INPUT_ALL );	
}


/**************************************************************************

Function		: ~UIInfoSlot()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIInfoSlot::~UIInfoSlot()
{	
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UIInfoSlot::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true ) {
		return true;
	}
	return false;
}


void UIInfoSlot::Update( double seconds )
{		
	if ( GetVisible() && GetTextBox() && m_item )
	{
		POINT pt;
		pt.x = gUIShell.GetMouseX();
		pt.y = gUIShell.GetMouseY();
		if ( GetRect().Contains( pt ) )
		{
			if ( GetRolloverTimer() != 0.0f )
			{
				SetRolloverTimer( GetRolloverTimer() - (float)seconds );
				if ( GetRolloverTimer() <= 0.0f )
				{
					SetRolloverTimer( 0.0f );

					gUIMessenger.Notify( "ui_slot_item_rollover", this );				
					PositionTextBox();
					GetTextBox()->SetVisible( true );
				}
			}
		}	
	}
	
	SetJustPlaced( false );
}


/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: void

**************************************************************************/
bool UIInfoSlot::ProcessMessage( UIMessage & msg )
{
	switch ( msg.GetCode() ) 
	{
	case MSG_LBUTTONDOWN:
		{
			m_buttondown = true;
			if ( !gUIShell.GetItemActive() )
			{
				RemoveItem();
				gUIMessenger.SendUIMessage( UIMessage( MSG_BUTTONPRESS ), this );
			}
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
	case MSG_ROLLOVER:
		{
			if ( m_buttondown == true ) 
			{				
				return false;
			}
			if ( gUIShell.GetLButtonDown() == true ) 
			{
				return false;
			}

			SetRolloverTimer( gUIShell.GetRolloverDelay() );								
		}
		break;
	case MSG_ROLLOFF:
		{
			SetRolloverTimer( 0.0f );
			gUIMessenger.Notify( "ui_slot_item_rolloff", this );
			if ( GetTextBox() )
			{					
				PositionTextBox();
				GetTextBox()->SetVisible( false );
			}
		}
		break;
	case MSG_ITEMACTIVATE:
		{
			m_bItemDetect = false;
		}
		break;
	case MSG_ITEMDEACTIVATE:
		{
			m_bItemDetect = true;
		}
		break;
	}
	
	return true;
}


/**************************************************************************

Function		: PlaceItem()

Purpose			: Place an item within the item slot

Returns			: void

**************************************************************************/
bool UIInfoSlot::PlaceItem( UIItem *pItem, bool bNotify, bool bPermanent, bool bDeactivate, bool bRequestEquip )
{
	if ( m_bRemoving )
	{
		return false;
	}

	if ( GetJustPlaced() )
	{
		// Reset ownership
		UIItem * pOwnedItem = gUIShell.GetItem( m_item );
		if ( pOwnedItem )
		{
			pOwnedItem->SetParentWindow( this );
		}

		UIWindowVec gridboxes = gUIShell.ListWindowsOfType( UI_TYPE_GRIDBOX );
		UIWindowVec::iterator i;

		UIItemVec items;
		gUIShell.FindActiveItems( items );

		for ( i = gridboxes.begin(); i != gridboxes.end(); ++i ) {
			if ( items.size() == 0 )
			{
				((UIGridbox *)(*i))->SetItemDetect( true );
			}
		}
		return false;
	}

	m_bPermanent = bPermanent;

	if ( bRequestEquip )
	{
		m_attemptEquipId = pItem->GetItemID();
		gUIMessenger.Notify( "infoslot_equip", this );	
		if ( GetCanEquip() == false )
		{
			return false;
		}
	}
	
	if (( m_item != 0 ) && ( bNotify == true )) 
	{
		UIItem * pOld = gUIShell.GetItem( m_item );
		if ( !pOld )
		{
			return false;
		}
		
		pItem->ActivateItem( true );			
		pOld->SetItemID( GetItemID() );
		RemoveItem();
		if ( !GetCanRemove() )
		{
			// We need to check for the improper case when a removeitem can fail, so it doesn't try to stack items [fixes #13432]
			gperror( "Cannot remove item from the infoslot, even though we are trying to place a new one inside." );
			return false;
		}

		if ( bDeactivate )
		{
			pOld->ActivateItem( false );
		}
	}
	else 
	{
		gUIMessenger.SendUIMessage( UIMessage( MSG_ITEMDEACTIVATE ) );
	}
	
	m_item = pItem->GetItemID();
	SetItemID( pItem->GetItemID() );
	SetHasTexture( true );	
	pItem->SetScale( 1.0f );  // Normalize scale
	pItem->SetParentWindow( this );

	UIWindowVec gridboxes = gUIShell.ListWindowsOfType( UI_TYPE_GRIDBOX );
	UIWindowVec::iterator i;

	UIItemVec items;
	gUIShell.FindActiveItems( items );

	for ( i = gridboxes.begin(); i != gridboxes.end(); ++i ) 
	{
		if ( items.size() == 0 )
		{
			((UIGridbox *)(*i))->SetItemDetect( true );
		}
	}

	if ( bNotify == true ) 
	{
		gUIMessenger.Notify( "infoslot_place", this );
		gUIMessenger.SendUIMessage( UIMessage( MSG_ITEMPLACE ), this );
	}

	SetJustPlaced( true );
	return true;
}



/**************************************************************************

Function		: RemoveItem()

Purpose			: Remove and item within the item slot

Returns			: void

**************************************************************************/
void UIInfoSlot::RemoveItem() 
{
	if ( GetJustPlaced() )
	{
		return;
	}

	if ( m_item != 0 )
	{
		m_attemptEquipId = m_item;
		gUIMessenger.Notify( "infoslot_request_remove", this );	
		if ( GetCanRemove() == false )
		{
			return;
		}
	}

	UIItemVec item_vec;
	gUIShell.FindActiveItems( item_vec );
	if ( ( item_vec.size() != 0 ) ) 
	{	
		UIItemVec::iterator i;
		for ( i = item_vec.begin(); i != item_vec.end(); ++i ) 
		{
			if ( !(*i)->GetSlotType().same_no_case( GetSlotType() ) ) 
			{
				return;
			}
		}
	}
	if ( m_item != 0 ) 
	{
		UIItem * pItem = gUIShell.GetItem( m_item );
		if ( !pItem )
		{
			return;
		}

		m_bInvalid = false;
		m_bHighlight = false;
		pItem->ActivateItem( true );			
		pItem->SetAcceptInput( false );
		pItem->SetHasTexture( true );
		pItem->SetItemID( GetItemID() );		
		
		if ( m_bPermanent == false ) 
		{
			m_bRemoving = true;
			m_item = 0;
			SetItemID( 0 );
			gUIMessenger.Notify( "infoslot_remove", this );
			gUIMessenger.SendUIMessage( UIMessage( MSG_REMOVEITEM ), this );
			m_bRemoving = false;
		}
	}
}


/**************************************************************************

Function		: ClearItem()

Purpose			: Blow away any item in the slot

Returns			: void

**************************************************************************/
void UIInfoSlot::ClearItem()
{
	if ( m_item != 0 ) 
	{
		UIItem * pItem = gUIShell.GetItem( m_item );
		if ( !pItem )
		{
			m_item = 0;
			SetItemID( 0 );
			m_bInvalid = false;
			m_bHighlight = false;
			return;
		}

		m_attemptEquipId = pItem->GetItemID();

		if ( Messenger::DoesSingletonExist() )
		{
			gUIMessenger.Notify( "infoslot_request_remove", this );	
			if ( GetCanRemove() == false )
			{
				return;
			}
		}	

		pItem->ActivateItem( false );		
		pItem->SetAcceptInput( false );		
		m_item = 0;
		SetItemID( 0 );
		m_bInvalid = false;
		m_bHighlight = false;
	}
}


UIItem * UIInfoSlot::GetItem() 
{ 
	UIItem * pItem = gUIShell.GetItem( m_item );
	return pItem;
}



void UIInfoSlot::PositionTextBox()
{
	if ( GetTextBox() && m_item )
	{
		int twidth	= GetTextBox()->GetRect().right - GetTextBox()->GetRect().left;
		int theight	= GetTextBox()->GetRect().bottom - GetTextBox()->GetRect().top;

		int rollover_width	= GetRect().right - GetRect().left;		

		RECT rect = GetRect();		
		GetTextBox()->GetRect().left = (rect.right - ( rollover_width/2 )) - (twidth/2);
		GetTextBox()->GetRect().right = GetTextBox()->GetRect().left + twidth;
		GetTextBox()->GetRect().bottom = rect.top;
		GetTextBox()->GetRect().top = GetTextBox()->GetRect().bottom - theight;

		if ( GetTextBox()->GetRect().left < 0 )
		{
			GetTextBox()->GetRect().left = 0;
			GetTextBox()->GetRect().right = twidth; 
		}

		if ( GetTextBox()->GetRect().right > gUIShell.GetScreenWidth() )
		{
			GetTextBox()->GetRect().right = gUIShell.GetScreenWidth();
			GetTextBox()->GetRect().left = gUIShell.GetScreenWidth() - twidth;
		}

		if ( GetTextBox()->GetRect().top < 0 )
		{
			GetTextBox()->GetRect().top = GetRect().bottom;
			GetTextBox()->GetRect().bottom = GetTextBox()->GetRect().top + theight;
		}							   
	}
}


UITextBox * UIInfoSlot::GetTextBox()
{
	if ( m_sSlotType.same_no_case( "picture" ) )
	{
		return 0;
	}

	if ( !m_pTextBox )
	{
		m_pTextBox = (UITextBox *)gUIShell.FindUIWindow( "gui_rollover_textbox" );
	}
	return m_pTextBox;
}


void UIInfoSlot::SetMousePos( unsigned int x, unsigned int y )
{
	if ( ((int)x >= GetRect().left) && ((int)x <= GetRect().right) &&
		 ((int)y >= GetRect().top)  && ((int)y <= GetRect().bottom) )
	{
		gUIMessenger.Notify( "ui_slot_item_rollover", this );		
		if ( GetTextBox() && m_item )
		{
			PositionTextBox();
			GetTextBox()->SetVisible( true );
		}
	}
}


#if !GP_RETAIL
void UIInfoSlot::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "infoslot" );
			hWindow->Set( "slot_type", m_sSlotType );
		}
	}
}
#endif


void UIInfoSlot::Draw()
{
	if ( GetVisible() )
	{
		UIWindow::Draw();	

		if ( m_bInvalid && m_item )
		{
			gUIShell.DrawColorBox( GetRect(), m_invalidColor );
		}

		if ( m_bHighlight && m_item )
		{
			gUIShell.DrawColorBox( GetRect(), m_dwHighlightColor );
		}
	}
}