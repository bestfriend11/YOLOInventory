/**************************************************************************

Filename		: ui_item.cpp

Description		: Control for the draggable/setable interface icon

Creation Date	: 10/6/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_item.h"
#include "ui_itemslot.h"
#include "ui_infoslot.h"
#include "ui_gridbox.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"

// These values are for the box height and width that the graphics were intended for
const int box_height	= 32;
const int box_width		= 32;

/**************************************************************************

Function		: UIItem()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIItem::UIItem( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: UIWindow( fhWindow, parent_interface, parent )
, m_buttondown( false )
, m_bDragging( false )
, m_bAcceptInput( true )
, m_mouseX( 0 )
, m_mouseY( 0 )
, m_rows( 0 )
, m_columns( 0 )
, m_box_height( box_height )
, m_box_width( box_width )
, m_stack_number( 1 )
, m_pFont( NULL )
, m_id( 0 )
, m_scale( 1.0f )
, m_pParentGrid( NULL )
, m_bActive( false )
, m_bPlaceValid( true )
, m_bMouseMoved( false )
, m_secondary_texture_index( 0 )
, m_secondary_edge( EDGE_TOP )
, m_secondary_percent_visible( 1.0f )
, m_bSecondaryTexture( false )
{	
	SetColor( 0xFFFFFFFF );
	SetType( UI_TYPE_ITEM );
	SetInputType( TYPE_INPUT_ALL );	
	SetVisible( false );

	gpstring sFont;
	if ( fhWindow.Get( "font_type", sFont ) ) 
	{
		m_pFont = gUITextureManager.LoadFont( sFont );		
	}	
	
	fhWindow.Get( "rows", m_rows		);
	fhWindow.Get( "columns", m_columns );
	fhWindow.Get( "item_type", m_sItemType );
	fhWindow.Get( "slot_type", m_sSlotType );
	fhWindow.Get( "texture", m_sTexture );
}

UIItem::UIItem()
: UIWindow()
, m_buttondown( false )
, m_bDragging( false )
, m_bAcceptInput( true )
, m_mouseX( 0 )
, m_mouseY( 0 )
, m_rows( 0 )
, m_columns( 0 )
, m_box_height( box_height )
, m_box_width( box_width )
, m_stack_number( 1 )
, m_pFont( NULL )
, m_id( 0 )
, m_scale( 1.0f )
, m_pParentGrid( NULL )
, m_bActive( false )
, m_bPlaceValid( true )
, m_bMouseMoved( false )
, m_secondary_texture_index( 0 )
, m_secondary_edge( EDGE_TOP )
, m_secondary_percent_visible( 1.0f )
, m_bSecondaryTexture( false )
{	
	SetColor( 0xFFFFFFFF );
	SetType( UI_TYPE_ITEM );
	SetInputType( TYPE_INPUT_ALL );	
	SetVisible( false );	
}


/**************************************************************************

Function		: ~UIItem()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIItem::~UIItem()
{
	gUIShell.RemoveFromItemMap( this );
	if ( m_secondary_texture_index != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_secondary_texture_index );
	}	
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UIItem::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true ) {
		return true;
	}
	else if ( action == ACTION_ACTIVATEITEM ) {
		if ( parameter.same_no_case( "true") ) {
			SetAcceptInput( false );
			ActivateItem( true );
			SetMousePos( m_mouseX, m_mouseY );
			gUIMessenger.SendUIMessage( UIMessage( MSG_ITEMACTIVATE ) );
		}
		else {
			ActivateItem( false );
		}
		return true;
	}	
	return false;
}


/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: bool

**************************************************************************/
bool UIItem::ProcessMessage( UIMessage & msg )
{
	switch ( msg.GetCode() ) 
	{
	case MSG_LBUTTONDOWN:
		{
			m_buttondown = true;
			SetAcceptInput( true );

			if ( GetDragging() == true ) 
			{			
				gUIMessenger.SendUIMessage( UIMessage( MSG_ITEMPLACE ), this );
				CheckForPlacement();		
				SetAcceptInput( true );	
				m_bMouseMoved = false;
			}
		}
		break;
	case MSG_LBUTTONUP:
		{	
			if ( m_bPlaceValid && GetDragging() && m_bMouseMoved )
			{
				CheckForPlacement();
			}

			m_buttondown = false;
			return false;
		}
		break;
	case MSG_GLOBALLBUTTONUP:
		{
			return false;
		}
		break;
	case MSG_DEACTIVATEITEMS:
		{
			ActivateItem( false );
			gUIMessenger.SendUIMessage( UIMessage( MSG_ITEMDEACTIVATE ) );
		}
		break;
	}
	
	return true;
}


/**************************************************************************

Function		: CheckForPlacement()

Purpose			: Process messages specific to the control

Returns			: void

**************************************************************************/
void UIItem::CheckForPlacement()
{
	UIWindowVec gridbox_list = gUIShell.ListWindowsOfType( UI_TYPE_GRIDBOX );
	UIWindowVec::iterator i;
	for ( i = gridbox_list.begin(); i != gridbox_list.end(); ++i ) {
 		if (( m_mouseX < (*i)->GetRect().right	) && ( m_mouseX > (*i)->GetRect().left		) &&
			( m_mouseY > (*i)->GetRect().top	) && ( m_mouseY < (*i)->GetRect().bottom	)) {
			UIGridbox *gb = (UIGridbox *)(*i);
			
			if (( gb->GetGridType().same_no_case( GetItemType() ) ) && 
				( gb->GetVisible() == true ) && 
				( gUIShell.IsInterfaceVisible( gb->GetInterfaceParent()) == true )) {

				if ( GetDragging() == true ) {
					if ( !gb->GetJustPlaced() && gb->GridHitDetect( GetRect().left, GetRect().top, this ) ) {					
						gUIMessenger.SendUIMessage( UIMessage( MSG_ITEMDEACTIVATE ) );						
					}
				}
				return;
			}			
		}
	}

	UIWindowVec itemslot_list = gUIShell.ListWindowsOfType( UI_TYPE_ITEMSLOT );
	UIWindowVec::iterator j;
	for ( j = itemslot_list.begin(); j != itemslot_list.end(); ++j ) {
		if (( m_mouseX < (*j)->GetRect().right	) && ( m_mouseX > (*j)->GetRect().left		) &&
			( m_mouseY > (*j)->GetRect().top	) && ( m_mouseY < (*j)->GetRect().bottom	)) {
			UIItemSlot *is = (UIItemSlot *)(*j);
			if (( is->GetSlotType().same_no_case( m_sSlotType ) ) &&
				( is->GetVisible() == true ) &&
				( gUIShell.IsInterfaceVisible( is->GetInterfaceParent()) == true )) {

				if ( GetDragging() == true ) 
				{
					if ( is->PlaceItem( this ) )
					{
						ActivateItem( false );					
					}
				}
				return;
			}
			else if ( !is->GetSlotType().same_no_case( m_sSlotType ) && is->IsEquipSlot() )
			{
				is->SetAttemptEquipId( GetItemID() );
				gUIMessenger.Notify( "itemslot_incompatible", is );
			}			
		}		
	}


	UIWindowVec infoslot_list = gUIShell.ListWindowsOfType( UI_TYPE_INFOSLOT );
	UIWindowVec::iterator k;
	for ( k = infoslot_list.begin(); k != infoslot_list.end(); ++k ) {
		if (( m_mouseX < (*k)->GetRect().right	) && ( m_mouseX > (*k)->GetRect().left		) &&
			( m_mouseY > (*k)->GetRect().top	) && ( m_mouseY < (*k)->GetRect().bottom	)) {
			UIInfoSlot *is = (UIInfoSlot *)(*k);
			if (( is->GetSlotType().same_no_case( m_sSlotType ) ) &&
				( is->GetVisible() == true ) &&
				( gUIShell.IsInterfaceVisible( is->GetInterfaceParent()) == true )) {

				if ( GetDragging() == true ) 
				{
					if ( is->PlaceItem( this ) )
					{
						ActivateItem( false );					
					}
				}
				return;
			}			
		}		
	}
}


/**************************************************************************

Function		: ActivateItem()

Purpose			: Sets the item as active

Returns			: void

**************************************************************************/
void UIItem::ActivateItem( bool bActivate, bool bValidPlace )
{
	if ( bActivate == true ) 
	{
		SetDragging( true );
		SetVisible( true );
		gUIMessenger.Notify( "activate_item", this );
		gUIMessenger.SendUIMessage( UIMessage( MSG_ITEMACTIVATE ) );
		SetMousePos( gUIShell.GetMouseX(), gUIShell.GetMouseY() );
		SetHasTexture( true );
		m_bActive = true;
		gUIShell.SetItemActive( true );
		gUIShell.AddActiveItem( this );
		SetParentWindow( NULL );
	}
	else 
	{
		SetDragging( false );
		SetVisible( false );
		m_bActive = false;		
		gUIShell.RemoveActiveItem( this );
		if ( gUIShell.GetNumActiveItems( GetInterfaceParent() ) == 0 )
		{
			gUIShell.SetItemActive( false );
		}
	}

	m_bMouseMoved = false;
	m_bPlaceValid = bValidPlace;
}



/**************************************************************************

Function		: SetMousePos()

Purpose			: Sets the current mouse position and adjusts the cursor
				  rectangle accordingly

Returns			: void

**************************************************************************/
void UIItem::SetMousePos( unsigned int x, unsigned int y )
{
	if ( ((unsigned int)m_mouseX > (x+gUIShell.GetDragTolerance()) ||
		  (unsigned int)m_mouseX < (x-gUIShell.GetDragTolerance())) 
		  
		||
		
		((unsigned int)m_mouseY > (y+gUIShell.GetDragTolerance()) ||
		 (unsigned int)m_mouseY < (y-gUIShell.GetDragTolerance()))
		 )
	{
		if ( GetDragging() )
		{
			m_bMouseMoved = true;
		}
	}
	else
	{
		if ( !GetDragging() )
		{
			m_bMouseMoved = false;
		}
	}

	m_mouseX = x;
	m_mouseY = y;
	
	if (( GetDragging() == true ) || ( GetVisible() == true )) {
		int width	= GetRect().right-GetRect().left;
		int height	= GetRect().bottom-GetRect().top;

		int left	= m_mouseX - width/2;
		int right	= left + width;
		int top		= m_mouseY - height/2;
		int bottom	= top +	height;
		SetRect( left, right, top, bottom );
	}
}


/**************************************************************************

Function		: Draw()

Purpose			: Draw the window

Returns			: void

**************************************************************************/
void UIItem::Draw() 
{
	if ( GetVisible() == true ) 
	{		
		if ( GetParentWindow() ) 
		{
			if ( GetParentWindow()->GetVisible() == false ) 
			{
				return;
			}
		}		
		
		DrawSecondaryItem( GetRect() );

		// Reload the texture if needed ( extremely rare case )
		if ( GetTextureIndex() == 0 )
		{
			ReloadTextures();
		}

		UIWindow::Draw();
		
		if ( m_stack_number > 1 )
		{
			char szNumber[32] = "";
			sprintf( szNumber, "%d", m_stack_number );
			int width	= 0;
			int height	= 0;
			if ( m_pFont ) 
			{
				m_pFont->CalculateStringSize( szNumber, width, height );
				m_pFont->Print( GetRect().right-width, GetRect().bottom-height, szNumber, GetColor(), true );		
			}
		}
	}
}


void UIItem::Update( double /* secondsElapsed */ )
{
	if (( GetDragging() == true ) || ( GetVisible() == true )) 
	{	
		SetMousePos( gUIShell.GetMouseX(), gUIShell.GetMouseY() );
	}
	else if ( !GetActive() && GetParentWindow() == 0 )
	{		
		SetMarkForDeletion( true );
		gUIShell.SetProcessDeletion( true );
	}
}


// Draw a secondary item; if one exists
void UIItem::DrawSecondaryItem( UIRect rect, float secondary_percent )
{
	// Draw a secondary texture, if one exists
	if ( m_secondary_texture_index != 0 ) {
		gUIShell.GetRenderer().SetTextureStageState(	0,
														m_secondary_texture_index != 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE,
														D3DTOP_MODULATE,
														D3DTADDRESS_CLAMP, // WRAP
														D3DTADDRESS_CLAMP,
														D3DTEXF_LINEAR, ///POINT
														D3DTEXF_LINEAR,
														D3DTEXF_LINEAR,
														D3DBLEND_SRCALPHA,
														D3DBLEND_INVSRCALPHA,
														true );

		// The item is activated
		if ( secondary_percent == -1 ) {
			secondary_percent = m_secondary_percent_visible;
		}

		UINormalizedRect uv_rect;
		uv_rect.left = 0.0f;
		uv_rect.top	 = 1.0f;
		uv_rect.right = 1.0f;
		uv_rect.bottom = 0.0f;

		switch ( m_secondary_edge ) {
		case EDGE_TOP:
			{					
				rect.top		= (int)(rect.bottom - ( (rect.bottom-rect.top) * secondary_percent ));
				uv_rect.top		= (float)(uv_rect.bottom - ( (uv_rect.bottom-uv_rect.top) * secondary_percent ));
			}
			break;
		case EDGE_BOTTOM:
			{				
				rect.bottom		+= (int)(rect.bottom		* secondary_percent);
				uv_rect.bottom	+= (float)(uv_rect.bottom	* secondary_percent);
			}
			break;
		case EDGE_LEFT:
			{
				rect.left		+= (int)(rect.left		* secondary_percent);
				uv_rect.left	+= (float)(uv_rect.left		* secondary_percent);
			}
			break;
		case EDGE_RIGHT:
			{
				rect.right		-= (int)(rect.right		* secondary_percent);
				uv_rect.right	-= (float)(uv_rect.right	* secondary_percent);
			}
			break;
		}
		
		
		tVertex Verts[4];
		memset( Verts, 0, sizeof( tVertex ) * 4 );
		
		DWORD color = 0xFFFFFFFF;
		Verts[0].x			= rect.left-0.5f;
		Verts[0].y			= rect.top-0.5f;
		Verts[0].z			= 0.0f;
		Verts[0].rhw		= 1.0f;
		Verts[0].uv.u		= (float)uv_rect.left;
		Verts[0].uv.v		= (float)uv_rect.top;	
		Verts[0].color		= color;

		Verts[1].x			= rect.left-0.5f;
		Verts[1].y			= rect.bottom-0.5f;
		Verts[1].z			= 0.0f;
		Verts[1].rhw		= 1.0f;
		Verts[1].uv.u		= (float)uv_rect.left;
		Verts[1].uv.v		= (float)uv_rect.bottom;
		Verts[1].color		= color;

		Verts[2].x			= rect.right-0.5f;
		Verts[2].y			= rect.top-0.5f;
		Verts[2].z			= 0.0f;
		Verts[2].rhw		= 1.0f;
		Verts[2].uv.u		= (float)uv_rect.right;
		Verts[2].uv.v		= (float)uv_rect.top;	
		Verts[2].color		= color;

		Verts[3].x			= rect.right-0.5f;
		Verts[3].y			= rect.bottom-0.5f;
		Verts[3].z			= 0.0f;
		Verts[3].rhw		= 1.0f;
		Verts[3].uv.u		= (float)uv_rect.right;
		Verts[3].uv.v		= (float)uv_rect.bottom;
		Verts[3].color		= color;
		
		gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_secondary_texture_index, 1 );
	}
}




// Sets the current scale of the item
void UIItem::SetScale( float scale )
{
	float currentScale = m_scale;
	m_scale = scale;
	
	scale = scale / currentScale;

	int width	= (int)((GetRect().right	- GetRect().left) * scale);
	int height	= (int)((GetRect().bottom	- GetRect().top) * scale);
	SetRect( GetRect().left, GetRect().left+width, GetRect().top, GetRect().top+height );
}


bool UIItem::LoadTexture( gpstring texture, bool bResizeWindow )
{
	m_sTexture = texture;
	return UIWindow::LoadTexture( texture, bResizeWindow );
}


// Load the secondary texture for the item ( if one exists )
void UIItem::LoadSecondaryTexture( gpstring texture )
{		
	m_sSecondTexture = texture;
	if ( m_secondary_texture_index != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_secondary_texture_index );
	}
	m_secondary_texture_index = gUITextureManager.LoadTexture( texture, GetInterfaceParent() );		
	SetHasSecondaryTexture( true );
}

void UIItem::ReloadTextures()
{
	LoadTexture( m_sTexture );
	if ( !m_sSecondTexture.empty() )
	{
		LoadSecondaryTexture( m_sSecondTexture );
	}
}


void UIItem::RemoveItemParent( UIWindow * pFrom )
{
	if ( pFrom == GetParentWindow() )
	{
		SetParentWindow( NULL );
		gUIMessenger.Notify( "does_item_have_parent", this );				
	}
}


#if !GP_RETAIL
void UIItem::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "item" );
			hWindow->Set( "rows", m_rows );
			hWindow->Set( "columns", m_columns );
			hWindow->Set( "item_type", m_sItemType );
			hWindow->Set( "slot_type", m_sSlotType );
		}
	}
}
#endif