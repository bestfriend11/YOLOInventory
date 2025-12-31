/**************************************************************************

Filename		: ui_itemslot.cpp

Description		: Control for the slots that hold a certain type of item

Creation Date	: 11/10/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_window.h"
#include "ui_itemslot.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"
#include "ui_item.h"
#include "ui_shell.h"
#include "ui_textbox.h"
#include "AppModule.h"


/**************************************************************************

Function		: UIItemSlot()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIItemSlot::UIItemSlot( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: UIWindow( fhWindow, parent_interface, parent )
, m_buttondown( false )
, m_rbuttondown( false )
, m_bAcceptInput( true )
, m_item( 0 )
, m_id( 0 )
, m_bPermanent( false )
, m_pTextBox( NULL )
, m_bEquipSlot( false )
, m_highlightColor( 0xFFFFFFFF )
, m_bActivateHighlight( false )
, m_dwActivateColor( 0xFFFFFFFF )
, m_dwActivateBorderColor( 0xFFFFFFFF )
, m_dwSpecialColor( 0xFFFFFFFF )
, m_bSpecial( false )
, m_bItemPlaced( false )
, m_originalWidth( 0 )
, m_originalHeight( 0 )
, m_bCanEquip( true )
, m_bCanRemove( true )
, m_attemptEquipId( 0 )
{	
	SetType( UI_TYPE_ITEMSLOT );
	SetInputType( TYPE_INPUT_ALL );	
	fhWindow.Get( "slot_type", m_sSlotType );
	fhWindow.Get( "is_equip_slot", m_bEquipSlot );
	fhWindow.Get( "accept_input", m_bAcceptInput );
	fhWindow.Get( "border_color", m_dwActivateBorderColor );
	fhWindow.Get( "activate_color", m_dwActivateColor );	
	fhWindow.Get( "special_color", m_dwSpecialColor );	
}
UIItemSlot::UIItemSlot()
: UIWindow()
, m_buttondown( false )
, m_rbuttondown( false )
, m_bAcceptInput( true )
, m_item( 0 )
, m_id( 0 )
, m_bPermanent( false )
, m_pTextBox( NULL )
, m_bEquipSlot( false )
, m_highlightColor( 0xFFFFFFFF )
, m_bActivateHighlight( false )
, m_dwActivateColor( 0xFFFFFFFF )
, m_dwActivateBorderColor( 0xFFFFFFFF )
, m_dwSpecialColor( 0xFFFFFFFF )
, m_bSpecial( false )
, m_bItemPlaced( false )
, m_originalWidth( 0 )
, m_originalHeight( 0 )
, m_bCanEquip( true )
, m_bCanRemove( true )
, m_attemptEquipId( 0 )
{	
	SetType( UI_TYPE_ITEMSLOT );
	SetInputType( TYPE_INPUT_ALL );	
}


/**************************************************************************

Function		: ~UIItemSlot()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIItemSlot::~UIItemSlot()
{	
}


void UIItemSlot::Update( double seconds )
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

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UIItemSlot::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true ) {
		return true;
	}
	return false;
}


/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: void

**************************************************************************/
bool UIItemSlot::ProcessMessage( UIMessage & msg )
{
	if ( GetAcceptInput() == true ) {
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
		case MSG_RBUTTONDOWN:
			{
				m_rbuttondown = true;
			}
			break;
		case MSG_RBUTTONUP:
			{
				if ( m_rbuttondown == true ) 
				{
					gUIMessenger.SendUIMessage( UIMessage( MSG_RBUTTONPRESS ), this );
					gUIMessenger.Notify( "itemslot_use", this );
				}
			}
			break;
		case MSG_GLOBALLBUTTONUP:
			{
				m_buttondown = false;
			}
			break;
		case MSG_GLOBALRBUTTONUP:
			{
				m_rbuttondown = false;
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
		}
	}
	else {
		switch ( msg.GetCode() ) 
		{
		case MSG_LBUTTONDOWN:
			{
				m_buttondown = true;			
			}
			break;
		case MSG_LBUTTONUP:
			{
				if ( m_buttondown == true ) 
				{		
					gUIMessenger.SendUIMessage( UIMessage( MSG_BUTTONPRESS ), this );
				}

				if ( m_sSlotType.same_no_case( "picture" ) )
				{
					gUIMessenger.Notify( "picture_item_drop", this );
				}

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
					gUIMessenger.SendUIMessage( UIMessage( MSG_RBUTTONPRESS ), this );					
				}
			}
			break;
		case MSG_GLOBALRBUTTONUP:
			{
				m_rbuttondown = false;
			}
			break;
		case MSG_ROLLOVER:
			{	
				SetRolloverTimer( gUIShell.GetRolloverDelay() );

				if ( m_item && m_sSlotType.same_no_case( "picture" ) )
				{
					UIItemVec items;
					gUIShell.FindActiveItems( items );
					if ( items.size() != 0 )
					{
						SetHighlightColor( 0xFF00FF00 );
					}
				}
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
				SetHighlightColor( 0xFFFFFFFF );
			}
			break;
		}
	}
	
	return true;
}


void UIItemSlot::Draw()
{
	if ( GetVisible() && GetAlpha() != 0.0f )
	{
		if ( m_bSpecial )
		{
			gUIShell.DrawColorBox( GetRect(), m_dwSpecialColor );
		}

		if ( m_item && m_bEquipSlot )
		{
			UIItem * pItem = gUIShell.GetItem( m_item );
			if ( !pItem )
			{
				return;
			}

			int itemWidth	= pItem->GetColumns() * pItem->GetBoxWidth();
			int itemHeight	= pItem->GetRows() * pItem->GetBoxHeight();

			int width		= GetRect().right	- GetRect().left;
			int height		= GetRect().bottom	- GetRect().top;
			
			float scaleWRatio = (float)width/(float)itemWidth;			
			float scaleHRatio = (float)height/(float)itemHeight;

			if ( scaleWRatio > scaleHRatio )
			{
				scaleWRatio = scaleHRatio;
			}

			if ( scaleWRatio > 1.0f )
			{
				scaleWRatio = 1.0f;			
			}			
			
			UIRect rect;
			UINormalizedRect uvrect;
		
			itemWidth = (int)( (float)itemWidth * (float)scaleWRatio );			
			itemHeight = (int)( (float)itemHeight * (float)scaleWRatio );

			rect.left	= (GetRect().left + width/2) - itemWidth/2;
			rect.top	= (GetRect().top + height/2) - itemHeight/2;
			rect.right	= rect.left + itemWidth;
			rect.bottom	= rect.top + itemHeight;			

			uvrect.left		= 0;
			uvrect.right	= (float)(pItem->GetColumns() * pItem->GetBoxWidth()) / (float)(pItem->GetRect().right-pItem->GetRect().left);
			uvrect.top		= 1.0f-(float)(pItem->GetRows() * pItem->GetBoxHeight()) / (float)(pItem->GetRect().bottom-pItem->GetRect().top);
			uvrect.bottom	= 1.0f;		

			UIWindow::DrawComponent( rect, pItem->GetTextureIndex(), pItem->GetAlpha(), uvrect );
		}
		else if ( HasTexture() && GetTextureIndex() )
		{
			UIWindow::DrawComponent( GetRect(), GetTextureIndex(), GetAlpha(), GetUVRect(), false, GetHighlightColor() );
		}
		else
		{
			UIWindow::Draw();
		}	

		if ( m_bActivateHighlight )
		{
			DrawColorBox( GetRect(), m_dwActivateColor, m_dwActivateBorderColor );
		}		

		UIWindowVec children = GetChildren();
		UIWindowVec::iterator i;
		for ( i = children.begin(); i != children.end(); ++i )
		{
			(*i)->Draw();
		}
	}
}


/**************************************************************************

Function		: PlaceItem()

Purpose			: Place an item within the item slot

Returns			: void

**************************************************************************/
bool UIItemSlot::PlaceItem( UIItem *pItem, bool bNotify, bool bPermanent, bool bDeactivate )
{
	if ( GetJustPlaced() && !GetSlotType().same_no_case( "picture" ) )
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

		for ( i = gridboxes.begin(); i != gridboxes.end(); ++i ) 
		{
			if ( items.size() == 0 )
			{
				((UIGridbox *)(*i))->SetItemDetect( true );
			}
		}
		return false;
	}

	m_bPermanent = bPermanent;

	if ( !m_bEquipSlot )
	{
		if ( GetTextureIndex() != 0 )
		{
			gUIShell.GetRenderer().DestroyTexture( GetTextureIndex() );
		}
		gUIShell.GetRenderer().AddTextureReference( pItem->GetTextureIndex() );
		SetTextureIndex( pItem->GetTextureIndex() );	
	}
	else
	{
		if ( !bNotify ) 
		{
			SetCanEquip( true );
		}
		else
		{
			m_attemptEquipId = pItem->GetItemID();
			gUIMessenger.Notify( "itemslot_equip", this );	

			if ( GetCanEquip() == false )
			{
				return false;
			}
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

	if ( !m_bEquipSlot )
	{
		if ((( pItem->GetRows() > 0 ) && ( pItem->GetColumns() > 0 )) &&
			!(( pItem->GetRows() == 1 ) && ( pItem->GetColumns() == 1)))
		{
			double bottom	= 1.0f - ((double)(pItem->GetRows() * pItem->GetBoxHeight()) / (double)(pItem->GetRect().bottom-pItem->GetRect().top));
			double right	= (double)(pItem->GetColumns() * pItem->GetBoxWidth()) / (double)(pItem->GetRect().right-pItem->GetRect().left);
			SetUVRect(	pItem->GetUVRect().left, right, bottom, 1.0f );
		}	
	}

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

	if ( IsEquipSlot() )
	{
		if ( items.size() == 0 )
		{
			ActivateSlotHighlight( false );
		}
		else
		{
			ActivateSlotHighlight( true );
		}
	}

	if ( bNotify == true ) 
	{
		gUIMessenger.Notify( "place_item", this );
		gUIMessenger.SendUIMessage( UIMessage( MSG_ITEMPLACE ), this );
	}

	SetCanRemove( true );
	SetJustPlaced( true );
	return true;
}



/**************************************************************************

Function		: RemoveItem()

Purpose			: Remove and item within the item slot

Returns			: void

**************************************************************************/
void UIItemSlot::RemoveItem() 
{
	if ( GetJustPlaced() )
	{
		return;
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
		if ( IsEquipSlot() )
		{
			gUIMessenger.Notify( "itemslot_request_remove", this );	
			if ( GetCanRemove() == false )
			{
				return;
			}
		}

		UIItem * pItem = gUIShell.GetItem( m_item );
		if ( !pItem )
		{
			return;
		}

		pItem->ActivateItem( true );			
		pItem->SetAcceptInput( false );
		pItem->SetHasTexture( true );
		pItem->SetItemID( GetItemID() );		
		
		if ( m_bPermanent == false ) 
		{			
			SetHasTexture( false );			
			m_item = 0;
			SetItemID( 0 );
			gUIMessenger.Notify( "remove_item", this );
			gUIMessenger.SendUIMessage( UIMessage( MSG_REMOVEITEM ), this );
		}
	}	
}


/**************************************************************************

Function		: ClearItem()

Purpose			: Blow away any item in the slot

Returns			: void

**************************************************************************/
void UIItemSlot::ClearItem()
{
	if ( m_item != 0 ) 
	{
		UIItem * pItem = gUIShell.GetItem( m_item );
		if ( !pItem )
		{
			SetHasTexture( false );	
			m_item = 0;
			SetItemID( 0 );
			return;
		}

		pItem->ActivateItem( false );		
		pItem->SetAcceptInput( false );		
		SetHasTexture( false );	
		m_item = 0;
		SetItemID( 0 );
	}
}


#if !GP_RETAIL
void UIItemSlot::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "itemslot" );
			hWindow->Set( "slot_type", m_sSlotType );
		}
	}
}
#endif



void UIItemSlot::PositionTextBox()
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


UITextBox * UIItemSlot::GetTextBox()
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


void UIItemSlot::SetMousePos( unsigned int x, unsigned int y )
{
	if ( ((int)x >= GetRect().left) && ((int)x <= GetRect().right) &&
		 ((int)y >= GetRect().top)  && ((int)y <= GetRect().bottom) )
	{
		gUIMessenger.Notify( "ui_slot_item_rollover", this );		
		if (( GetTextBox() ) && ( m_item ) && ( !m_sSlotType.same_no_case( "picture" )))
		{
			PositionTextBox();
			GetTextBox()->SetVisible( true );
		}
	}
}


void UIItemSlot::ResizeToCurrentResolution( int original_width, int original_height )
{
	UIWindow::ResizeToCurrentResolution( original_width, original_height );
	m_originalWidth = original_width;
	m_originalHeight = original_height;
}