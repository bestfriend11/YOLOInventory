/**************************************************************************

Filename		: ui_listbox.cpp

Description		: Control for the listbox control

Creation Date	: 9/19/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_listbox.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_emotes.h"
#include "ui_messenger.h"
#include "ui_textureman.h"
#include "ui_slider.h"
#include "ui_combobox.h"
#include "ui_animation.h"
#include "ui_text.h"
#include "LocHelp.h"
#include "AppModule.h"
#include "keys.h"


/**************************************************************************

Function		: UIListbox()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIListbox::UIListbox( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: UIWindow( fhWindow, parent_interface, parent )
, m_buttondown( false )
, m_element_height( 0 )
, m_element_width( 0 )
, m_element_type( ELEMENT_TEXT )
, m_lead_element( 0 )
, m_IconWidth( 16 )
, m_IconHeight( 16 )
, m_id_counter( 0 )
, m_max_active( 0 )
, m_left_indent( 0 )
, m_pFont( NULL )
, m_selection_texture_index( 0 )
, m_selection_alpha( 1.0f )
, m_dwColor( 0xFFFFFFFF )
, m_pVSlider( NULL )
, m_pButtonUp( NULL )
, m_pButtonDown( NULL )
, m_pPopupItem( NULL )
, m_bUseDefaultFont( true )
, m_fontHeight( 0 )
, m_bRolloverSelect( false )
, m_dwInvalidColor( 0x55ff0000 )
, m_dwActiveColor( 0x5500ffff )
, m_FlashingIconCount( 0 )
, m_bHitSelect( false )
, m_bFocus( false )
, m_bPermanentFocus( false )
, m_SelectionColor( 0x80AD9473 )
, m_hoveredElement( 0 )
, m_bToolTipOnSelect( false )
{
	SetType( UI_TYPE_LISTBOX );
	SetInputType( TYPE_INPUT_ALL );

	gpstring element;
	fhWindow.Get( "element_type", element );
	if ( element.same_no_case( "window" ) ) 
	{
		SetElementType( ELEMENT_WINDOW );
	}
	else 
	{
		SetElementType( ELEMENT_TEXT );
	}

	gpstring sFont;

	// ------------------------------------------------------------------------
	// For crazy Microsoft compiler bug action, change the following 0 to 1
	// Load up the game, exit at the main screen, and watch the leakage!
	// Problem:
	//	It seems that in very specific cases, the Microsoft compiler does not
	//	handle the temporary allocation of constants used in a conditional
	//	correctly.  In essence, it jumps out of the condition without calling
	//	the destructor for the temporarily allocated constant, thereby causing
	//	a leak.
	// $$$$$ revisit this when fuel no longer takes gpstring -Scott
#if 0
	if( fhWindow.Get( "font_type", sFont ) && !sFont.empty() )
#else
	bool bFontType	= fhWindow.Get( "font_type", sFont ) != FVP_NONE;
	if( bFontType && !sFont.empty() )
#endif
	// ------------------------------------------------------------------------
	{
		m_pFont = gUITextureManager.LoadFont( sFont );
		if ( !m_pFont )
		{
			int font_size = 0;
			fhWindow.Get( "font_size", font_size );

			DWORD dwBgColor = 0;
			if ( fhWindow.Get( "font_bg_color", dwBgColor ) )
			{
				m_pFont = gUITextureManager.LoadFont( sFont, font_size, dwBgColor );
			}
			else
			{
				m_pFont = gUITextureManager.LoadFont( sFont, font_size );
			}
		}
		m_bUseDefaultFont = false;
	}
	else 
	{
		gperrorf(( "Using default system font for GUI element: %s, change it!", GetName().c_str() ));
		m_pFont = gUITextureManager.GetDefaultFont();
	}

	if ( m_pFont )
	{
		int width = 0;
		gUIEmotes.CalculateStringSize( m_pFont, L"TEST", width, m_fontHeight );
	}

	SetHeight( 0 );
	FastFuelHandle hSel = fhWindow.GetChildTyped( "selection_box" );
	if ( hSel )
	{
		gpstring texture;
		hSel.Get( "texture", texture );
		LoadSelectionTexture( texture );
		hSel.Get( "alpha", m_selection_alpha );
	}

	int color = 0xFFFFFFFF;
	fhWindow.Get( "text_color", color );
	SetTextColor( color );

	fhWindow.Get( "selection_color", m_SelectionColor );

	fhWindow.Get( "rollover_select", m_bRolloverSelect );
	fhWindow.Get( "tool_tip_on_select", m_bToolTipOnSelect );

	fhWindow.Get( "left_indent", m_left_indent );

	// Create a common control, if necessary
	bool bCommonCtrl = false;
	if ( fhWindow.Get( "common_control", bCommonCtrl ) )
	{
		if ( bCommonCtrl )
		{
			SetIsCommonControl( true );
			CreateCommonCtrl();
		}
	}

	fhWindow.Get( "invalid_color", m_dwInvalidColor );
	fhWindow.Get( "active_color", m_dwActiveColor );

	fhWindow.Get( "permanent_focus", m_bPermanentFocus );
	if ( m_bPermanentFocus )
	{
		SetHasFocus( true );
	}

	m_pPopupItem = new UIText;
	m_pPopupItem->SetName( GetName() + "_popuptext" );
	m_pPopupItem->SetDrawOrder( GetDrawOrder() + 1 );
	m_pPopupItem->SetTopmost( true );
	m_pPopupItem->SetFont( m_pFont );
	m_pPopupItem->SetBackgroundFill( true );
	m_pPopupItem->SetBorder( true );
	m_pPopupItem->SetBackgroundColor( 0xFF000000 );
	m_pPopupItem->SetBorderColor( 0xFF808080 );
	m_pPopupItem->SetAlpha( 0.6f );
	m_pPopupItem->SetVisible( false );
	gUIShell.AddWindowToInterface( m_pPopupItem, parent_interface );
	AddChild( m_pPopupItem );

	// make sure our slider bar is updated with the anchoring information from the parent
	// we will always want the slider bar to be with the lists.  we also need to pass
	// along topmost information so we don't overright our little buttons.

	ANCHOR_DATA anchorData = GetAnchorData();
	if( anchorData.bBottomAnchor )
	{
		GetChildSliderUpButton()->UpdateAnchorData( anchorData );

		anchorData.bottom_anchor = anchorData.bottom_anchor - GetChildSliderUpButton()->GetHeight();
		GetChildSlider()->UpdateAnchorData( anchorData );
	
		anchorData.bottom_anchor = anchorData.bottom_anchor - GetChildSlider()->GetHeight();
		GetChildSliderDownButton()->UpdateAnchorData( anchorData );
	}

	if( GetTopmost() )
	{
		GetChildSlider()->SetTopmost(true);
		GetChildSliderUpButton()->SetTopmost(true);
		GetChildSliderDownButton()->SetTopmost(true);
	}
}
UIListbox::UIListbox()
: UIWindow()
, m_buttondown( false )
, m_element_height( 0 )
, m_element_width( 0 )
, m_element_type( ELEMENT_TEXT )
, m_lead_element( 0 )
, m_IconWidth( 16 )
, m_IconHeight( 16 )
, m_id_counter( 0 )
, m_max_active( 0 )
, m_left_indent( 0 )
, m_pFont( NULL )
, m_selection_texture_index( 0 )
, m_selection_alpha( 1.0f )
, m_dwColor( 0xFFFFFFFF )
, m_pVSlider( NULL )
, m_pButtonUp( NULL )
, m_pButtonDown( NULL )
, m_pPopupItem( NULL )
, m_bUseDefaultFont( true )
, m_fontHeight( 0 )
, m_bRolloverSelect( false )
, m_dwInvalidColor( 0x55ff0000 )
, m_dwActiveColor( 0x5500ffff )
, m_FlashingIconCount( 0 )
, m_bHitSelect( false )
, m_bFocus( false )
, m_bPermanentFocus( false )
, m_SelectionColor( 0x80AD9473 )
, m_hoveredElement( 0 )
, m_bToolTipOnSelect( false )
{
	SetType( UI_TYPE_LISTBOX );
	SetInputType( TYPE_INPUT_ALL );	
}



/**************************************************************************

Function		: ~UIListbox()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIListbox::~UIListbox()
{
	{for ( ListboxElements::iterator i = m_elements.begin(); i != m_elements.end(); ++i )
	{
		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{
			ListboxLines::iterator j;
			for ( j = (*i).lines.begin(); j != (*i).lines.end(); ++j )
			{
				m_pFont->UnregisterString( (*j).sText );
			}
		}		
	}}
	m_elements.clear();

	if ( m_selection_texture_index != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_selection_texture_index );
	}

	{for ( ListboxIconMap::iterator i = m_Icons.begin(); i != m_Icons.end(); ++i )
	{
		if ( i->second > 0 )
		{
			gUIShell.GetRenderer().DestroyTexture( i->second );
		}
	}}
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UIListbox::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true ) 
	{
		return true;
	}
	else if ( action == ACTION_SETELEMENTHEIGHT ) 
	{
		SetHeight( atoi( parameter.c_str() ) );
		return true;
	}
	else if ( action == ACTION_ADD_ELEMENT ) 
	{
		int strings = stringtool::GetNumDelimitedStrings( parameter, ',' );
		if ( strings == 1 ) 
		{
			AddElement( ToUnicode( parameter ) );
		}
		else 
		{
			gpstring text;
			stringtool::GetDelimitedValue( parameter, ',', 0, text );
			AddElement( ToUnicode( text ) );
		}
		return true;
	}
	else if ( action == ACTION_REMOVE_ELEMENT ) 
	{
		RemoveElement( ToUnicode( parameter ) );
		return true;
	}
	return false;
}


/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: bool

**************************************************************************/
bool UIListbox::ProcessMessage( UIMessage & msg )
{
	UIWindow::ProcessMessage( msg );

	switch ( msg.GetCode() )
	{
	case MSG_HIDE:
		{
			SetHasFocus( false );			
		}
		break;
	case MSG_SHOW:
		{
			if ( GetPermanentFocus() )
			{
				SetHasFocus( true );
			}
		}
		break;
	case MSG_LBUTTONDOWN:
	case MSG_RBUTTONDOWN:
		{
			SetHasFocus( true );
			m_buttondown = true;
		}
		break;
	case MSG_LBUTTONUP:
		{
			if ( m_buttondown && GetParentWindow() && (GetParentWindow()->GetType() == UI_TYPE_COMBOBOX ))
			{
				UIComboBox *cb = (UIComboBox *)GetParentWindow();
				cb->SetText( GetSelectedText() );
				cb->SetSelectedTag( GetSelectedTag() );
				cb->SetExpanded( false );
			}
			m_buttondown = false;
		}
		break;
	case MSG_RBUTTONUP:
		{
			if ( m_buttondown )
			{
				int j = 0;
				for ( int i = m_lead_element; i < (m_lead_element+m_max_active); ++i )
				{
					UIRect element_rect;
					element_rect.left	= GetRect().left;
					element_rect.top	= GetRect().top+((j*GetHeight()));
					element_rect.right	= GetRect().right;
					if ( GetChildSlider() && GetChildSlider()->GetVisible() )
					{
						element_rect.right -= GetChildSlider()->GetRect().Width();
					}
					element_rect.bottom = element_rect.top+(GetHeight());
					j++;

					if (( (int)gUIShell.GetMouseX() < element_rect.right ) && ( (int)gUIShell.GetMouseX() > element_rect.left ) &&
						( (int)gUIShell.GetMouseY() > element_rect.top ) && ( (int)gUIShell.GetMouseY() < element_rect.bottom ))
					{
						ListboxElement * pElement = GetElement( i );
						if ( pElement && pElement->bSelectable )
						{
							DeselectElements();
							pElement->bSelected = true;
							gUIMessenger.SendUIMessage( UIMessage( MSG_CONTEXT ), this );
						}
					}
				}
			}
			m_buttondown = false;
		}
		break;
	case MSG_LDOUBLECLICK:
		{
			bool bHit = false;
			int j = 0;

			for ( int i = m_lead_element; i < (m_lead_element+m_max_active); ++i )
			{
				UIRect element_rect;
				element_rect.left	= GetRect().left;
				element_rect.top	= GetRect().top+((j*GetHeight()));
				element_rect.right	= GetRect().right;
				if ( GetChildSlider() && GetChildSlider()->GetVisible() )
				{
					element_rect.right -= GetChildSlider()->GetRect().Width();
				}
				element_rect.bottom = element_rect.top+(GetHeight());
				j++;

				if (( (int)gUIShell.GetMouseX() < element_rect.right	)	&& ( (int)gUIShell.GetMouseX() > element_rect.left ) &&
					( (int)gUIShell.GetMouseY() > element_rect.top	)		&& ( (int)gUIShell.GetMouseY() < element_rect.bottom ))
				{	
					if ( !GetElement( i ) )
					{
						break;
					}

					bHit = true;
				}
			}		
			
			if ( !bHit )
			{
				return false;
			}			
		}
		break;
	case MSG_GLOBALLBUTTONUP:
	case MSG_GLOBALRBUTTONUP:
		{
			if ( GetPermanentFocus() == false && !m_buttondown && !HitDetect( gUIShell.GetMouseX(), gUIShell.GetMouseY() ) )
			{				
				SetHasFocus( false );				
			}

			m_buttondown = false;
		}
		break;
	case MSG_WHEELUP:
		{
			if ( GetChildSlider() )
			{
				GetChildSlider()->SetValue( GetChildSlider()->GetValue() - 2 );
			}
		}
		break;
	case MSG_WHEELDOWN:
		{
			if ( GetChildSlider() )
			{
				GetChildSlider()->SetValue( GetChildSlider()->GetValue() + 2 );
			}
		}
		break;
	case MSG_KEYPRESS:
		return( !HandleInputMessage( msg ) );					
	}

	return true;
}


void UIListbox::Update( double seconds )
{
	UIWindow::Update( seconds );

	if ( m_bPermanentFocus && GetVisible() )
	{
		SetHasFocus( true );		
	}

	if ( m_FlashingIconCount > 0 )
	{
		for ( int i = m_lead_element; i < (m_lead_element + m_max_active); ++i ) 
		{
			ListboxElement * pElement = GetElement( i );
			if ( !pElement ) 
			{
				return;
			}

			if ( pElement->flashIcon )
			{
				pElement->flashTime -= (float)seconds;
				if ( pElement->flashTime < 0 )
				{
					if ( pElement->flashInfinite )
					{
						pElement->flashTime += 1.0f;
					}
					else
					{
						pElement->flashIcon = false;
						--m_FlashingIconCount;
					}
				}
			}
		}
	}

	if ( GetRollover() )
	{
		int element = m_lead_element + (gUIShell.GetMouseY() - GetRect().top) / GetHeight();
		ListboxElement * pElement = GetElement( element );
		if ( pElement && (pElement->lines.size() == 1) )
		{
			if ( m_pPopupItem->GetTag() != element )
			{
				m_pPopupItem->SetTag( element );

				int width = 0, height = 0;
				gUIEmotes.CalculateStringSize( m_pFont, pElement->lines[ 0 ].sText.c_str(), width, height );
				int left = GetRect().left;
				if ( pElement->icon != NO_ICON )
				{
					left += m_IconWidth + 4;
				}
				if ( left + width > GetRect().left + GetRect().Width() )
				{
					if ( left + width > gUIShell.GetScreenWidth() )
					{
						left -= ((left + width) - gUIShell.GetScreenWidth());
					}

					m_pPopupItem->SetText( pElement->lines[ 0 ].sText );
					m_pPopupItem->SetRect( left, left + width, GetRect().top + (element - m_lead_element) * GetHeight(), GetRect().top + (element - m_lead_element + 1) * GetHeight() );
					m_pPopupItem->SetVisible( true );
				}
				else
				{
					m_pPopupItem->SetVisible( false );
				}
			}
		}
		else
		{
			m_pPopupItem->SetTag( -1 );
			m_pPopupItem->SetVisible( false );
		}
	}
	else
	{
		m_pPopupItem->SetTag( -1 );
		m_pPopupItem->SetVisible( false );
	}
}


void UIListbox::SetVisible( bool bVisible )
{
	UIWindow::SetVisible( bVisible );

	if ( bVisible && GetChildSlider() && (GetChildSlider()->GetMax() == GetChildSlider()->GetMin()) )
	{
		GetChildSlider()->SetVisible( false );
	}
}


/**************************************************************************

Function		: AddElement()

Purpose			: Add an element to the listbox control

Returns			: void

**************************************************************************/
void UIListbox::AddElement( gpwstring sText, int tag )
{
	AddElement( sText, tag, GetTextColor() );
}


void UIListbox::AddElement( gpwstring sText, int tag, DWORD dwColor )
{
	ListboxElement *pElement = GetTagElement( tag );

	if ( pElement == NULL )
	{
		ListboxElement			element;

		element.ID				= m_id_counter++;
		element.bSelected		= m_elements.empty() ? true : false;
		element.tag				= tag;
		element.icon			= NO_ICON;
		
		ListboxText lbtext;
		lbtext.dwColor			= dwColor;
		lbtext.sText			= sText;
		element.lines.push_back( lbtext );

		m_elements.push_back( element );

		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{
			ListboxLines::iterator i;
			for ( i = element.lines.begin(); i != element.lines.end(); ++i )
			{					
				m_pFont->RegisterString( (*i).sText );						
			}
		}

		// Look to see if the listbox has a child slider bar, if it does
		// notify it to modify itself accordingly
		if ( GetChildSlider() )
		{
			GetChildSlider()->SetRange( GetNumElements() - GetMaxActiveElements() );
			GetChildSlider()->CalculateSlider();

			if ( GetVisible() )
			{
				if ( !GetChildSlider()->GetVisible() && (GetChildSlider()->GetMax() > GetChildSlider()->GetMin()) )
				{
					GetChildSlider()->SetVisible( true );
				}
				else if ( GetChildSlider()->GetVisible() && (GetChildSlider()->GetMax() == GetChildSlider()->GetMin()) )
				{
					GetChildSlider()->SetVisible( false );
				}
			}
		}
	}
	else
	{		
		SetElementText( tag, 0, sText, GetTextColor() );
	}
}


void UIListbox::SetElementText( const int tag, const int line, const gpwstring & sText, const DWORD dwColor )
{
	ListboxElement *pElement = GetTagElement( tag );
	if ( pElement )
	{
		while ( line > (int)pElement->lines.size()-1 )
		{
			ListboxText lt;
			lt.dwColor = GetTextColor();			
			pElement->lines.push_back( lt );
		}

		int index = 0;
		ListboxLines::iterator i;
		for ( i = pElement->lines.begin(); i != pElement->lines.end(); ++i )
		{
			if ( index == line )
			{
				(*i).sText		= sText;
				(*i).dwColor	= dwColor;	
				return;
			}

			++index;
		}		
	}	
}


/**************************************************************************

Function		: RemoveElement()

Purpose			: Remove an element from the listbox control

Returns			: void

**************************************************************************/
void UIListbox::RemoveElement( gpwstring element )
{
	ListboxElements::iterator removeElement = m_elements.end();

	for ( ListboxElements::iterator i = m_elements.begin(); i != m_elements.end(); ++i )
	{
		if ( removeElement != m_elements.end() )
		{
			--i->ID;
		}
		else if ( (*(i->lines.begin())).sText.same_no_case( element ) )
		{
			removeElement = i;
		}
	}

	if ( removeElement != m_elements.end() )
	{
		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{
			for ( ListboxLines::iterator i = removeElement->lines.begin(); i != removeElement->lines.end(); ++i )
			{
				m_pFont->UnregisterString( (*i).sText );
			}
		}
		m_elements.erase( removeElement );
		--m_id_counter;
	}

	// Look to see if the listbox has a child slider bar, if it does
	// notify it to modify itself accordingly
	if ( GetChildSlider() )
	{
		GetChildSlider()->SetRange( GetNumElements() - GetMaxActiveElements() );
		GetChildSlider()->CalculateSlider();

		if ( GetVisible() )
		{
			if ( !GetChildSlider()->GetVisible() && (GetChildSlider()->GetMax() > GetChildSlider()->GetMin()) )
			{
				GetChildSlider()->SetVisible( true );
			}
			else if ( GetChildSlider()->GetVisible() && (GetChildSlider()->GetMax() == GetChildSlider()->GetMin()) )
			{
				GetChildSlider()->SetVisible( false );
			}
		}
	}
}


void UIListbox::RemoveElement( int tag )
{
	ListboxElements::iterator removeElement = m_elements.end();

	for ( ListboxElements::iterator i = m_elements.begin(); i != m_elements.end(); ++i )
	{
		if ( removeElement != m_elements.end() )
		{
			--i->ID;
		}
		else if ( (*i).tag == tag )
		{
			removeElement = i;
		}
	}

	if ( removeElement != m_elements.end() )
	{
		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{
			for ( ListboxLines::iterator i = removeElement->lines.begin(); i != removeElement->lines.end(); ++i )
			{
				m_pFont->UnregisterString( (*i).sText );
			}
		}
		m_elements.erase( removeElement );
		--m_id_counter;
	}

	// Look to see if the listbox has a child slider bar, if it does
	// notify it to modify itself accordingly
	if ( GetChildSlider() )
	{
		GetChildSlider()->SetRange( GetNumElements() - GetMaxActiveElements() );
		GetChildSlider()->CalculateSlider();

		if ( GetVisible() )
		{
			if ( !GetChildSlider()->GetVisible() && (GetChildSlider()->GetMax() > GetChildSlider()->GetMin()) )
			{
				GetChildSlider()->SetVisible( true );
			}
			else if ( GetChildSlider()->GetVisible() && (GetChildSlider()->GetMax() == GetChildSlider()->GetMin()) )
			{
				GetChildSlider()->SetVisible( false );
			}
		}
	}
}



/**************************************************************************

Function		: RemoveAllElements()

Purpose			: Remove all elements from the listbox control

Returns			: void

**************************************************************************/
void UIListbox::RemoveAllElements()
{
	m_lead_element	= 0;
	m_id_counter	= 0;
	ListboxElements::iterator i;
	for ( i = m_elements.begin(); i != m_elements.end(); ++i )
	{
		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{
			ListboxLines::iterator j;
			for ( j = (*i).lines.begin(); j != (*i).lines.end(); ++j )
			{				
				m_pFont->UnregisterString( (*j).sText );
			}
		}
	}
	m_elements.clear();

	if ( GetChildSlider() )
	{
		GetChildSlider()->SetRange( 0 );
		GetChildSlider()->SetVisible( false );
	}
}


/**************************************************************************

Function		: SetHeight()

Purpose			: Set the element height and configure the listbox
				  so it can determine how many elements it can display
				  at a time.

Returns			: void

**************************************************************************/
void UIListbox::SetHeight( int height )
{
	m_element_height = height;
	m_element_width = GetRect().right - GetRect().left;
	int box_height = GetRect().bottom - GetRect().top;

	if ( height != 0 ) 
	{
		m_max_active = (int)(box_height/height);
	}
	else 
	{
		m_max_active = 0;
	}
}


void UIListbox::SetMaxActiveElements( int max_active )
{
	m_max_active = max_active;

	SetRect( GetRect().left, GetRect().right, GetRect().top, GetRect().top + m_max_active * m_element_height );

	ResizeSlider();
}


void UIListbox::ResizeSlider()
{
	if ( GetChildSlider() && m_pButtonDown && m_pButtonUp )
	{
		m_pButtonUp->SetRect( GetRect().right - m_pButtonUp->GetRect().Width(), GetRect().right, GetRect().top, GetRect().top + m_pButtonUp->GetRect().Height() );
		m_pButtonDown->SetRect( m_pButtonUp->GetRect().left, m_pButtonUp->GetRect().right, GetRect().bottom - m_pButtonDown->GetRect().Height(), GetRect().bottom );
		GetChildSlider()->SetRect( m_pButtonUp->GetRect().left, m_pButtonUp->GetRect().right, m_pButtonUp->GetRect().bottom, m_pButtonDown->GetRect().top );
		GetChildSlider()->SetRange( GetNumElements() - GetMaxActiveElements() );
		GetChildSlider()->CalculateSlider();
		if ( !GetChildSlider()->GetVisible() && (GetChildSlider()->GetMax() > GetChildSlider()->GetMin()) )
		{
			GetChildSlider()->SetVisible( true );
		}
		else if ( GetChildSlider()->GetVisible() && (GetChildSlider()->GetMax() == GetChildSlider()->GetMin()) )
		{
			GetChildSlider()->SetVisible( false );
		}
	}
}


/**************************************************************************

Function		: Draw()

Purpose			: Render the listbox and its elements appropriately

Returns			: void

**************************************************************************/
void UIListbox::Draw()
{
	if ( GetVisible() == true ) 
	{
		UIWindow::Draw();
		int elementCount = 0;
		for ( int i = m_lead_element; i < (m_lead_element + m_max_active); ++i ) 
		{
			ListboxElement * pElement = GetElement( i );
			if ( !pElement ) 
			{
				return;
			}

			UIRect element_rect;
			element_rect.left	= GetRect().left;
			element_rect.top	= GetRect().top + (elementCount * GetHeight());

			if ( pElement->bSelected ) 
			{
				element_rect.right	= GetRect().right;
				element_rect.bottom = element_rect.top + GetHeight();

				if ( m_selection_texture_index != 0 )
				{
					DrawComponent( element_rect, m_selection_texture_index, m_selection_alpha );
				}
				else
				{
					gUIShell.DrawColorBox( element_rect, m_SelectionColor );
				}
			}

			int shiftLeft = m_left_indent;
			if ( pElement->icon != NO_ICON )
			{
				ListboxIconMap::iterator findIcon = m_Icons.find( pElement->icon );
				if ( findIcon != m_Icons.end() )
				{
					if ( !pElement->flashIcon || (pElement->flashIcon && ( (pElement->flashTime - (int)pElement->flashTime) > 0.5f )) )
					{
						UIRect icon_rect;
						icon_rect.left = GetRect().left;
						icon_rect.right = GetRect().left + m_IconWidth;
						icon_rect.top = element_rect.top;
						icon_rect.bottom = element_rect.top + m_IconHeight;
						DrawComponent( icon_rect, findIcon->second, 1.0f );
					}
					shiftLeft += m_IconWidth + 4;
				}
			}

			if ( GetElementType() == ELEMENT_TEXT )  
			{
				if ( m_pFont ) 
				{
					int centerLine = (int)(GetHeight() - m_fontHeight * pElement->lines.size()) / 2;

					ListboxLines::iterator k;
					int counter = 0;
					for ( k = pElement->lines.begin(); k != pElement->lines.end(); ++k )
					{
						int width = 0, height = 0;
						gUIEmotes.CalculateStringSize( m_pFont, (*k).sText.c_str(), width, height );

						if ( (shiftLeft + width) < GetRect().Width() )
						{
							gUIEmotes.Draw( m_pFont, element_rect.left + shiftLeft, element_rect.top + (m_fontHeight * counter) + centerLine, (*k).sText.c_str(), (*k).dwColor, (UIWindow*) this, true );
						}
						else
						{
							gpwstring drawText = (*k).sText;
							while ( ((shiftLeft + width) > GetRect().Width()) && (drawText.size() > 1) )
							{
								drawText = drawText.left( drawText.size() - 1 );
								if ( m_pFont )
								{
									gUIEmotes.CalculateStringSize( m_pFont, drawText.c_str(), width, height );
								}
							}
							gUIEmotes.Draw( m_pFont, element_rect.left + shiftLeft, element_rect.top + (m_fontHeight * counter) + centerLine, drawText.c_str(), (*k).dwColor, (UIWindow*) this, true );
						}

						counter++;
					}
				}
				elementCount++;
			}
			if ( GetElementType() == ELEMENT_WINDOW ) 
			{
				DrawComponent( element_rect, pElement->textureId, 1.0f );
			}
		}
	}
}

gpwstring UIListbox::GetElementText( int id )
{
	gpwstring text;
	ListboxElement* element = GetElement( id );
	if( element )
	{
		text.assign( (*(element->lines.begin())).sText );
	}

	return text;
}

gpwstring UIListbox::GetElementTextFromTag( int tag )
{
	gpwstring text;
	ListboxElement* element = GetTagElement( tag );
	if( element )
	{
		text.assign( (*(element->lines.begin())).sText );
	}

	return text;
}

/**************************************************************************

Function		: GetElement()

Purpose			: Retrieves an element using its id

Returns			: gpstring

**************************************************************************/
ListboxElement * UIListbox::GetElement( int id )
{
	ListboxElements::iterator i;
	for ( i = m_elements.begin(); i != m_elements.end(); ++i ) 
	{
		if ( (*i).ID == id ) 
		{
			return &(*i);
		}
	}
	return NULL;
}


ListboxElement * UIListbox::GetTagElement( int tag )
{
	if ( tag == INVALID_TAG )
	{
		return NULL;
	}

	ListboxElements::iterator i;
	for ( i = m_elements.begin(); i != m_elements.end(); ++i ) 
	{
		if ( i->tag == tag ) 
		{
			return i;
		}
	}
	return NULL;
}


ListboxElement * UIListbox::GetTextElement( gpwstring text )
{
	for ( ListboxElements::iterator i = m_elements.begin(); i != m_elements.end(); ++i )
	{
		if ( (*(i->lines.begin())).sText.same_no_case( text ) )
		{
			return i;
		}
	}
	return NULL;
}


/**************************************************************************

Function		: SetMousePos()

Purpose			: Sets the current mouse position and adjusts the listbox
				  rectangle/elements accordingly

Returns			: void

**************************************************************************/
void UIListbox::SetMousePos( unsigned int x, unsigned int y )
{
	if ( (GetChildSlider() && GetChildSlider()->GetButtonDown())
		|| (GetChildSliderUpButton() && GetChildSliderUpButton()->GetButtonDown())
		|| (GetChildSliderDownButton() && GetChildSliderDownButton()->GetButtonDown()) )
	{
		// if there is a slider, and it is being scrolled, do not allow selection of list items
		return;
	}

	bool shouldUpdateSelect = false;
	if ( GetButtonDown() || (((GetParentWindow()) && (GetParentWindow()->GetType() == UI_TYPE_COMBOBOX )) || m_bRolloverSelect) )
	{
		shouldUpdateSelect = true;
	}
		
	bool bHit = false;
	int j = 0;
	for ( int i = m_lead_element; i < (m_lead_element+m_max_active); ++i )
	{
		UIRect element_rect;
		element_rect.left	= GetRect().left;
		element_rect.top	= GetRect().top+((j*GetHeight()));
		element_rect.right	= GetRect().right;
		if ( GetChildSlider() && GetChildSlider()->GetVisible() )
		{
			element_rect.right -= GetChildSlider()->GetRect().Width();
		}
		element_rect.bottom = element_rect.top+(GetHeight());
		j++;

		if (( (int)x < element_rect.right	) && ( (int)x > element_rect.left ) &&
			( (int)y > element_rect.top	) && ( (int)y < element_rect.bottom	))
		{
			ListboxElement * pElement = GetElement( i );
			if ( pElement && pElement->bSelectable )
			{
				// update the tooltip
				if( m_hoveredElement != i && m_bToolTipOnSelect == false )
				{
					m_hoveredElement = i;
					SetToolTip( GetElement(i)->toolTip , GetHelpBoxName(), GetHasToolTip() ); // set the help text
					UpdateHelpText( ); // make sure we get the new help text !
				}
				
				// update the selection...  if wanted
				if( shouldUpdateSelect )
				{
					DeselectElements();
					GetElement( i )->bSelected = true;
					gUIMessenger.SendUIMessage( UIMessage( MSG_SELECT ), this );					
					bHit = true;
				}
			}
		}
	}
	
	// update the tooltip
	if( m_bToolTipOnSelect == true )
	{
		SetToolTip( GetSelectedElement()->toolTip , GetHelpBoxName(), GetHasToolTip() ); // set the help text
		UpdateHelpText( ); // make sure we get the new help text !
	}

	if( shouldUpdateSelect )
	{
		SetHitSelect( bHit );
	}
}


void UIListbox::SetElementSelectable( int tag, bool bSelectable )
{
	ListboxElement * pElement = GetTagElement( tag );
	if ( pElement )
	{
		pElement->bSelectable = bSelectable;
	}
}

void UIListbox::SelectElement( gpwstring sElement )
{
	bool bFoundElement = false;

	int index = 0;
	ListboxElements::iterator i;
	for ( i = m_elements.begin(); i != m_elements.end(); ++i, ++index )
	{
		if ( sElement.same_no_case( (*((*i).lines.begin())).sText ) )
		{
			bFoundElement = true;
			break;
		}
	}

	if ( bFoundElement && (*i).bSelectable )
	{
		DeselectElements();
		(*i).bSelected = true;

		// make sure the selected element is visible in the list
		if ( (index < m_lead_element) || (index >= m_lead_element + m_max_active) )
		{
			m_lead_element = index;
			if ( ((int)m_elements.size() - 1 - m_lead_element) < m_max_active )
			{
				m_lead_element = (int)m_elements.size() - m_max_active;
			}
		}

		if ( GetChildSlider() )
		{
			GetChildSlider()->SetValue( m_lead_element );
		}
	}
}


void UIListbox::SelectElement( int tag )
{
	bool bFoundElement = false;

	int index = 0;
	ListboxElements::iterator i;
	for ( i = m_elements.begin(); i != m_elements.end(); ++i, ++index )
	{
		if ( tag == (*i).tag )
		{
			bFoundElement = true;
			break;
		}
	}

	if ( bFoundElement && (*i).bSelectable )
	{
		DeselectElements();
		(*i).bSelected = true;

		// make sure the selected element is visible in the list
		if ( (index < m_lead_element) || (index >= m_lead_element + m_max_active) )
		{
			m_lead_element = index;
			if ( ((int)m_elements.size() - 1 - m_lead_element) < m_max_active )
			{
				m_lead_element = (int)m_elements.size() - m_max_active;
			}
		}

		if ( GetChildSlider() )
		{
			GetChildSlider()->SetValue( m_lead_element );
		}
	}
}


void UIListbox::ClearSelection()
{
	ListboxElements::iterator i;
	for ( i = m_elements.begin(); i != m_elements.end(); ++i )
	{
		(*i).bSelected = false;
	}
}


/**************************************************************************

Function		: DeselectElements()

Purpose			: Deselects all elements

Returns			: void

**************************************************************************/
void UIListbox::DeselectElements()
{
	ListboxElements::iterator i;
	for ( i = m_elements.begin(); i != m_elements.end(); ++i ) 
	{
		(*i).bSelected = false;
	}
}


/**************************************************************************

Function		: LoadSelectionTexture()

Purpose			: Load in a texture for the selection

Returns			: void

**************************************************************************/
void UIListbox::LoadSelectionTexture( const gpstring& texture )
{
	if ( m_selection_texture_index != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_selection_texture_index );
	}
	m_selection_texture_index = gUITextureManager.LoadTexture( texture, GetInterfaceParent() );
}


void UIListbox::AddIcon( const gpstring& texture, int icon )
{
	ListboxIconMap::iterator findIcon = m_Icons.find( icon );
	if ( findIcon == m_Icons.end() )
	{
		unsigned int textureId = gUITextureManager.LoadTexture( texture, GetInterfaceParent() );
		m_Icons.insert( ListboxIconMap::value_type( icon, textureId ) );
	}
}


void UIListbox::SetElementIcon( int tag, int icon )
{
	ListboxElements::iterator i;
	for ( i = m_elements.begin(); i != m_elements.end(); ++i ) 
	{
		if ( i->tag == tag )
		{
			i->icon = icon;
			break;
		}
	}
}


void UIListbox::SetElementIcon( gpwstring text, int icon )
{
	ListboxElements::iterator i;
	for ( i = m_elements.begin(); i != m_elements.end(); ++i ) 
	{
		if ( i->lines.begin()->sText.same_no_case( text ) )
		{
			i->icon = icon;
			break;
		}
	}
}


void UIListbox::FlashElementIcon( int tag, float duration )
{
	ListboxElements::iterator i;
	for ( i = m_elements.begin(); i != m_elements.end(); ++i ) 
	{
		if ( i->tag == tag )
		{
			if ( duration == 0 )
			{
				if ( i->flashIcon )
				{
					--m_FlashingIconCount;
				}
				i->flashTime = 0.0f;
				i->flashIcon = false;
				i->flashInfinite = false;
			}
			else if ( duration < 0 )
			{
				++m_FlashingIconCount;
				i->flashTime = 1.0f;
				i->flashIcon = true;
				i->flashInfinite = true;
			}
			else
			{
				++m_FlashingIconCount;
				i->flashTime = duration;
				i->flashIcon = true;
				i->flashInfinite = false;
			}

			break;
		}
	}
}

// set the tooltip for this element
void UIListbox :: SetElementTagToolTip( int tag, const gpwstring& tip )
{
	ListboxElement* element = GetTagElement( tag );
	if( element )
	{
		element->toolTip.assign( tip );
	}
}

// set the tooltip for this element
void UIListbox :: SetElementTextToolTip( gpwstring textName, const gpwstring& tip )
{
	ListboxElement* element = GetTextElement( textName );
	if( element )
	{
		element->toolTip.assign( tip );
	}
}


ListboxElement * UIListbox::GetSelectedElement()
{
	ListboxElements::iterator i;
	for ( i = m_elements.begin(); i != m_elements.end(); ++i ) 
	{
		if ( (*i).bSelected == true ) 
		{
			return i;
		}
	}
	return NULL;
}


/**************************************************************************

Function		: GetSelectedText()

Purpose			: Get the text of the selected item

Returns			: gpstring

**************************************************************************/
gpwstring UIListbox::GetSelectedText()
{
	ListboxElements::iterator i;
	for ( i = m_elements.begin(); i != m_elements.end(); ++i ) 
	{
		if ( (*i).bSelected == true ) 
		{
			return (*((*i).lines.begin())).sText;
		}
	}
	return gpwstring();
}


int UIListbox::GetSelectedTag()
{
	ListboxElements::iterator i;
	for ( i = m_elements.begin(); i != m_elements.end(); ++i ) 
	{
		if ( (*i).bSelected == true ) 
		{
			return (*i).tag;
		}
	}
	return ( INVALID_TAG );
}


void UIListbox::AutoSize( UIWindow * pWindow, int index )
{
	int maxWidth = 0;
	int maxHeight = 0;
	ListboxElements::iterator i;
	for ( i = m_elements.begin(); i != m_elements.end(); ++i ) 
	{
		ListboxLines::iterator j;
		for ( j = (*i).lines.begin(); j != (*i).lines.end(); ++j ) 
		{
			int width	= 0;
			int height	= 0;
			gUIEmotes.CalculateStringSize( m_pFont, (*j).sText, width, height );
			if ( width > maxWidth )
			{
				maxWidth = width;
			}

			if ( GetElementHeight() != 0 )
			{
				maxHeight += m_element_height;
			}
			else
			{
				maxHeight += height;
			}
		}
	}

	SetIndex( index );

	if ( pWindow )
	{
		if ( pWindow->GetRect().left >= gAppModule.GetGameWidth()/2 )
		{
			SetRect( pWindow->GetRect().right-maxWidth, pWindow->GetRect().right, pWindow->GetRect().bottom, pWindow->GetRect().bottom+maxHeight );
		}
		else
		{
			SetRect( pWindow->GetRect().left, pWindow->GetRect().left+maxWidth, pWindow->GetRect().bottom, pWindow->GetRect().bottom+maxHeight );
		}
		
		if ( GetRect().bottom >= gAppModule.GetGameHeight() )
		{
			SetRect( GetRect().left, GetRect().right, pWindow->GetRect().top-maxHeight, pWindow->GetRect().top );
		}
	}
	else
	{
		SetRect( GetRect().left, GetRect().left+maxWidth, GetRect().top, GetRect().top+maxHeight );
	}
}

void UIListbox::CreateCommonCtrl( gpstring sTemplate )
{
	UNREFERENCED_PARAMETER( sTemplate );
	UIButton * pButtonUp	= 0;
	UIButton * pButtonDown	= 0;
	UISlider * pSliderV		= 0;

	gpstring sName;
	pButtonDown = (UIButton *)gUIShell.CreateDefaultWindowOfType( UI_TYPE_BUTTON );
	sName = GetName();
	sName += "_button_down";
	pButtonDown->CreateCommonCtrl( sName, "button_down", this, SLIDER_INCREMENT );
	pButtonDown->SetRect(	GetRect().right - pButtonDown->GetRect().right, GetRect().right,
							GetRect().bottom - (pButtonDown->GetRect().bottom), GetRect().bottom );
	pButtonDown->SetDrawOrder( GetDrawOrder() + 1 );
	pButtonDown->SetConsumable( true );
	gUIShell.AddWindowToInterface( (UIWindow *)pButtonDown, GetInterfaceParent() );
	pButtonDown->SetGroup( GetGroup() );

	pButtonUp = (UIButton *)gUIShell.CreateDefaultWindowOfType( UI_TYPE_BUTTON );
	sName = GetName();
	sName += "_button_up";
	pButtonUp->CreateCommonCtrl( sName, "button_up", this, SLIDER_DECREMENT );
	pButtonUp->SetRect(	pButtonDown->GetRect().left, pButtonDown->GetRect().right,
						GetRect().top, GetRect().top + pButtonUp->GetRect().bottom );
	pButtonUp->SetDrawOrder( GetDrawOrder() + 1 );
	pButtonUp->SetConsumable( true );
	gUIShell.AddWindowToInterface( (UIWindow *)pButtonUp, GetInterfaceParent() );
	pButtonUp->SetGroup( GetGroup() );

	pSliderV = (UISlider *)gUIShell.CreateDefaultWindowOfType( UI_TYPE_SLIDER );
	sName = GetName();
	sName += "_slider_vertical";
	pSliderV->CreateCommonCtrl( sName, "slider", this, true );
	pSliderV->SetRect( pButtonDown->GetRect().left, pButtonDown->GetRect().right,
					   pButtonUp->GetRect().bottom, pButtonDown->GetRect().top );
	pSliderV->SetDrawOrder( GetDrawOrder() + 1 );
	pSliderV->SetConsumable( true );
	gUIShell.AddWindowToInterface( (UIWindow *)pSliderV, GetInterfaceParent() );
	pSliderV->SetGroup( GetGroup() );

	pButtonUp->SetParentWindow( pSliderV );
	pButtonDown->SetParentWindow( pSliderV );
	pSliderV->AddChild( pButtonUp );
	pSliderV->AddChild( pButtonDown );
	pSliderV->SetParentWindow( this );
	AddChild( (UIWindow *)pSliderV );

	m_pVSlider = pSliderV;
	m_pButtonUp = pButtonUp;
	m_pButtonDown = pButtonDown;

	m_pVSlider->SetVisible( false );
}


#if !GP_RETAIL
void UIListbox::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "listbox" );
			if ( m_pFont )
			{
				hWindow->Set( "font_type", m_pFont->GetTextureName() );
			}
			hWindow->Set( "text_color", GetTextColor(), FVP_HEX_INT );
		}
	}
}
#endif


bool UIListbox::HandleInputMessage( UIMessage & msg )
{
	if ( GetHasFocus() && GetVisible() )
	{
		bool bHandled = false;
		switch ( msg.GetKey() )
		{
		// Up
		case Keys::KEY_UP:
			{
				SelectPreviousElement();	
				bHandled = true;
			}
			break;
		// Down
		case Keys::KEY_DOWN:
			{
				SelectNextElement();
				bHandled = true;
			}
			break;
		// PageUp
		case Keys::KEY_PAGEUP:
			{
				SelectPreviousPage();
				bHandled = true;
			}
			break;
		// PageDown
		case Keys::KEY_PAGEDOWN:
			{
				SelectNextPage();
				bHandled = true;
			}
			break;
		case Keys::KEY_HOME:
			{
				SelectFirstElement();
				bHandled = true;
			}
			break;
		case Keys::KEY_END:
			{
				SelectLastElement();
				bHandled = true;
			}
			break;
		}	

		if ( bHandled )
		{
			gUIMessenger.SendUIMessage( UIMessage( MSG_SELECT ), this );					
		}

		return bHandled;
	}

	return false;
}


void UIListbox::SelectPreviousElement()
{
	if ( m_elements.size() > 1 )
	{
		ListboxElements::iterator i;
		int currentElement = 0;
		for ( i = m_elements.begin(); i != m_elements.end(); ++i )
		{
			if ( (*i).bSelected && i != m_elements.begin() )
			{
				--currentElement;

				if ( currentElement < m_lead_element || currentElement > m_lead_element+(int)GetMaxActiveElements() )
				{
					m_lead_element--;
					if ( GetChildSlider() )
					{
						GetChildSlider()->SetValue( GetLeadElement() );
						GetChildSlider()->CalculateSlider();
					}
				}

				(*i).bSelected = false;
				--i;				
				(*i).bSelected = true;
				return;				
			}

			++currentElement;
		}
	}
}


void UIListbox::SelectNextElement()
{
	if ( m_elements.size() > 1 )
	{
		ListboxElements::iterator i;
		int currentElement = 0;
		for ( i = m_elements.begin(); i != m_elements.end(); ++i )
		{
			if ( (*i).bSelected )
			{
				ListboxElements::iterator iTest = i;
				++iTest;
				++currentElement;
				if ( iTest != m_elements.end() )
				{
					if ( currentElement < m_lead_element || (currentElement+1) > m_lead_element+(int)GetMaxActiveElements() )
					{
						m_lead_element++;
						if ( GetChildSlider() )
						{
							GetChildSlider()->SetValue( GetLeadElement() );
							GetChildSlider()->CalculateSlider();
						}
					}

					(*i).bSelected = false;
					++i;
					(*i).bSelected = true;
					return;				
				}
			}

			++currentElement;
		}
	}
}


void UIListbox::SelectPreviousPage()
{	
	if ( m_elements.size() > 1 )
	{
		int currentElement = 0;
		ListboxElements::iterator i;
		for ( i = m_elements.begin(); i != m_elements.end(); ++i )
		{		
			if ( (*i).bSelected )
			{
				int numIterations = GetMaxActiveElements()-1;
				ListboxElements::iterator iTest = i;				
				while ( iTest != m_elements.begin() )
				{
					--currentElement; 
					numIterations--;
					--iTest;
					if ( numIterations == 0 || iTest == m_elements.begin() )
					{
						if ( currentElement < m_lead_element || currentElement > m_lead_element+(int)GetMaxActiveElements() )
						{
							m_lead_element = currentElement;
							if ( GetChildSlider() )
							{
								GetChildSlider()->SetValue( GetLeadElement() );
								GetChildSlider()->CalculateSlider();
							}
						}

						(*i).bSelected = false;
						(*iTest).bSelected = true;
						return;
					}					

					ListboxElements::iterator iEnd = iTest;
					iEnd--;
					if ( iEnd == m_elements.begin() )
					{
						if ( currentElement < m_lead_element || currentElement > m_lead_element+(int)GetMaxActiveElements() )
						{
							m_lead_element = currentElement;
							if ( GetChildSlider() )
							{
								GetChildSlider()->SetValue( GetLeadElement() );
								GetChildSlider()->CalculateSlider();
							}
						}

						(*i).bSelected = false;
						(*iEnd).bSelected = true;
						return;
					}
				}
			}

			++currentElement;
		}
	}
}


void UIListbox::SelectNextPage()
{
	if ( m_elements.size() > 1 )
	{
		int currentElement = 0;
		ListboxElements::iterator i;
		for ( i = m_elements.begin(); i != m_elements.end(); ++i )
		{		
			if ( (*i).bSelected )
			{
				int numIterations = GetMaxActiveElements()-1;
				ListboxElements::iterator iTest = i;				
				while ( iTest != m_elements.end() )
				{
					++currentElement; 
					numIterations--;
					++iTest;
					if ( numIterations == 0 )
					{
						if ( currentElement < m_lead_element || (currentElement+1) > m_lead_element+(int)GetMaxActiveElements() )
						{
							m_lead_element = currentElement;
							if ( GetChildSlider() )
							{
								GetChildSlider()->SetValue( GetLeadElement() );
								GetChildSlider()->CalculateSlider();
							}
						}

						(*i).bSelected = false;
						(*iTest).bSelected = true;
						return;
					}					

					ListboxElements::iterator iEnd = iTest;
					iEnd++;
					if ( iEnd == m_elements.end() )
					{
						if ( currentElement < m_lead_element || (currentElement+1) > m_lead_element+(int)GetMaxActiveElements() )
						{
							m_lead_element = currentElement;
							if ( GetChildSlider() )
							{
								GetChildSlider()->SetValue( GetLeadElement() );
								GetChildSlider()->CalculateSlider();
							}
						}

						(*i).bSelected = false;
						(*iTest).bSelected = true;
						return;
					}
				}
			}

			++currentElement;
		}
	}
}


void UIListbox::SelectFirstElement()
{
	if ( m_elements.size() > 1 )
	{
		m_lead_element = 0;
		if ( GetChildSlider() )
		{
			GetChildSlider()->SetValue( GetLeadElement() );
			GetChildSlider()->CalculateSlider();
		}
		DeselectElements();
		ListboxElements::iterator i;
		i = m_elements.begin();		
		(*i).bSelected = true;					
	}
}


void UIListbox::SelectLastElement()
{
	if ( m_elements.size() > 1 )
	{
		m_lead_element = GetNumElements() - GetMaxActiveElements();
		if ( GetChildSlider() )
		{
			GetChildSlider()->SetValue( GetLeadElement() );
			GetChildSlider()->CalculateSlider();
		}
		DeselectElements();
		ListboxElements::iterator i;
		i = m_elements.end()-1;		
		(*i).bSelected = true;					
	}
}