/**************************************************************************

Filename		: ui_combobox.cpp

Description		: Control for the combo box

Creation Date	: 1/11/00

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_combobox.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"
#include "ui_combobox.h"
#include "ui_dialogbox.h"
#include "LocHelp.h"


/**************************************************************************

Function		: UIComboBox()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIComboBox::UIComboBox(	FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
	: UIWindow( fhWindow, parent_interface, parent )
	, m_bDrawText( true )
	, m_bExpanded( false )
	, m_buttondown( false )
	, m_pFont( NULL )
	, m_pListbox( NULL )
	, m_pBackground( NULL )	
	, m_bAutoSize( false )
	, m_MaxAutoSizeElements( 0 )
	, m_SelectedTag( -1 )
	, m_bTruncateText( false )
{	
	SetType( UI_TYPE_COMBOBOX );
	SetInputType( TYPE_INPUT_ALL );
	SetColor( 0xFFFFFFFF );		

	if ( fhWindow.GetBool( "has_background", false ) )
	{
		m_pBackground = new UIDialogBox();
		m_pBackground->CreateCommonCtrl( "cpbox_thin_dark" );
	}

	gpstring sFont;
	if ( fhWindow.Get( "font_type", sFont ) )
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
	}	
	else
	{		
		gperrorf(( "Using default system font for GUI element: %s, change it!", GetName().c_str() ));
		m_pFont = gUITextureManager.GetDefaultFont();
	}

	if ( fhWindow.Get( "text", m_text ) && m_text.size() != 0 )
	{
		if ( LocMgr::DoesSingletonExist() )
		{
			gLocMgr.TranslateUiText( m_text, GetInterfaceParent().c_str(), GetName().c_str() );
			m_pFont->RegisterString( m_text );
		}
	}

	fhWindow.Get( "show_text", m_bDrawText );

	m_bAutoSize = fhWindow.GetBool( "auto_size", false );

	m_MaxAutoSizeElements = fhWindow.GetInt( "max_visible_elements", -1 );

	int color = 0xFFFFFFFF;
	fhWindow.Get( "text_color", color );
	SetTextColor( color );

	fhWindow.Get( "truncate", m_bTruncateText );
}

UIComboBox::UIComboBox()
	: UIWindow()	
	, m_bDrawText( true )
	, m_bExpanded( false )
	, m_buttondown( false )
	, m_pFont( NULL )
	, m_pListbox( NULL )
	, m_pBackground( NULL )	
	, m_bAutoSize( false )
	, m_MaxAutoSizeElements( 0 )
	, m_SelectedTag( -1 )
	, m_bTruncateText( false )
{		
	SetType( UI_TYPE_COMBOBOX );
	SetInputType( TYPE_INPUT_ALL );
	SetColor( 0xFFFFFFFF );		
}


/**************************************************************************

Function		: ~UIComboBox()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIComboBox::~UIComboBox()
{
	delete m_pBackground;

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && m_pFont )
	{
		m_pFont->UnregisterString( m_text );
	}
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UIComboBox::ProcessAction( UI_ACTION action, gpstring parameter )
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
bool UIComboBox::ProcessMessage( UIMessage & msg )
{
	UIWindow::ProcessMessage( msg );

	switch ( msg.GetCode() ) 
	{
		case MSG_CREATED:
		{
			UIWindowVec children = GetChildren();
			UIWindowVec::iterator i;
			for ( i = children.begin(); i != children.end(); ++i )
			{
				if ( (*i)->GetType() == UI_TYPE_LISTBOX )
				{
					(*i)->SetVisible( false );
					m_pListbox = (UIListbox *)(*i);
				}
			}
			break;
		}
		
		case MSG_LBUTTONDOWN:
		{
			m_buttondown = true;
			SetExpanded( !GetExpanded() );
			break;
		}

		case MSG_COMBOEXPAND:
		{
			SetExpanded( !GetExpanded() );
			break;
		}

		case MSG_GLOBALLBUTTONDOWN:
		case MSG_GLOBALRBUTTONDOWN:
		{
			if ( GetExpanded() )
			{
				if( !HitDetect( gUIShell.GetMouseX(), gUIShell.GetMouseY() ) )
				{
					bool bMouseOutsideChildren = true;
					for ( UIWindowVec::iterator i = GetChildren().begin(); i != GetChildren().end(); ++i )
					{
						if( (*i)->HitDetect( gUIShell.GetMouseX(), gUIShell.GetMouseY() ) )
						{
							bMouseOutsideChildren = false;
							break;
						}
					}
					if ( bMouseOutsideChildren )
					{
						SetExpanded( false );
					}
				}
			}
			break;
		}
		
		case MSG_ESCAPE:
		{
			SetExpanded( false );
			break;
		}

		case MSG_WHEELUP:
		{
			if ( m_pListbox )
			{
				m_pListbox->SelectElement( GetText() );
				m_pListbox->SelectPreviousElement();
				ListboxElement * pElement = m_pListbox->GetSelectedElement();
				if ( pElement && !pElement->lines.empty() )
				{
					SetText( pElement->lines.begin()->sText );
					SetSelectedTag( pElement->tag );
				}
			}
			if ( IsToolTipVisible() )
			{
				HideToolTip();
			}
			break;
		}

		case MSG_WHEELDOWN:
		{
			if ( m_pListbox )
			{
				m_pListbox->SelectElement( GetText() );
				m_pListbox->SelectNextElement();
				ListboxElement * pElement = m_pListbox->GetSelectedElement();
				if ( pElement && !pElement->lines.empty() )
				{
					SetText( pElement->lines.begin()->sText );
					SetSelectedTag( pElement->tag );
				}
			}
			if ( IsToolTipVisible() )
			{
				HideToolTip();
			}
			break;
		}

	}
	return true;
}



/**************************************************************************

Function		: Draw()

Purpose			: Draw the window

Returns			: void

**************************************************************************/
void UIComboBox::Draw() 
{
	UIWindow::Draw();
	if ( GetVisible() == true )
	{
		if ( m_pBackground && m_pListbox && m_pListbox->GetVisible() )
		{
			m_pBackground->Draw();
		}

		if ( GetParentWindow() )
		{
			if ( GetParentWindow()->GetVisible() == false )
			{
				return;
			}
		}

		if ( m_bDrawText && m_pFont )
		{
			m_pFont->Print( GetRect().left, GetRect().top, m_drawText.c_str(), GetTextColor(), true );				
		}
	}
}


void UIComboBox::SetVisible( bool bVisible )
{
	if ( bVisible == GetVisible() )
	{
		return;
	}

	UIWindow::SetVisible( bVisible );

	SetExpanded( false );
}


#if !GP_RETAIL
void UIComboBox::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "combo_box" );
			if ( m_pFont )
			{
				hWindow->Set( "font_type", m_pFont->GetTextureName() );			
			}
		}
	}
}
#endif


void UIComboBox::SetExpanded( bool bExpanded )
{
	m_bExpanded = bExpanded;

	if ( !m_pListbox )
	{
		gpassertm( m_pListbox, "A ComboBox must have a child Listbox defined." );
		return;
	}

	if ( bExpanded )
	{
		if ( m_bAutoSize )
		{
			int numElements = min_t( m_MaxAutoSizeElements, m_pListbox->GetNumElements() );
			if ( (numElements * m_pListbox->GetElementHeight()) != m_pListbox->GetRect().Height() )
			{
				m_pListbox->SetMaxActiveElements( numElements );
			}
		}
		if ( m_pBackground )
		{
			m_pBackground->GetRect() = m_pListbox->GetRect();
		}

		m_pListbox->SelectElement( m_text );
		m_pListbox->SetButtonDown( true );
	}

	if ( !bExpanded || (m_pListbox->GetNumElements() > 0) )
	{
		m_pListbox->SetVisible( bExpanded );
		m_pListbox->SetConsumable( bExpanded );
	}
}


void UIComboBox::SetText( gpwstring text )	
{
	if ( text.same_no_case( m_text ) )
	{
		return;
	}

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && m_pFont )
	{
		m_pFont->UnregisterString( m_text );
		m_pFont->RegisterString( text );
	}

	if ( m_bTruncateText && GetHasToolTip() )
	{
		ClearToolTip();
	}

	m_text = text;
	m_drawText.clear();
	int width	= 0;
	int height	= 0;
	if ( m_pFont )
	{
		m_pFont->CalculateStringSize( m_text.c_str(), width, height );
	}

	if ( m_bTruncateText && ((width > GetRect().Width()) && (m_text.size() > 4)) )
	{
		m_drawText = m_text.left( m_text.size() - 2 ) + L"...";
		if ( m_pFont )
		{
			m_pFont->CalculateStringSize( m_drawText.c_str(), width, height );
		}

		while ( (width > GetRect().Width()) && (m_drawText.size() > 4) )
		{
			m_drawText = m_drawText.left( m_drawText.size() - 4 ) + L"...";
			if ( m_pFont )
			{
				m_pFont->CalculateStringSize( m_drawText.c_str(), width, height );
			}
		}

		SetToolTip( m_text, gUIShell.GetDefaultTooltipName(), true );
	}
	else
	{
		m_drawText = m_text;
	}

	gUIMessenger.SendUIMessage( UIMessage( MSG_CHANGE ), this );
}
