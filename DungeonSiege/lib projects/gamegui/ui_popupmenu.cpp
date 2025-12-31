/**************************************************************************

Filename		: ui_popupmenu.cpp

Description		: Control for the popup menu

Creation Date	: 10/6/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_popupmenu.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_textureman.h"
#include "ui_animation.h"
#include "ui_dialogbox.h"
#include "LocHelp.h"
#include "AppModule.h"


UIPopupMenu::UIPopupMenu( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
	: UIWindow( fhWindow, parent_interface, parent )
	, m_pBackground( NULL )
	, m_pFont( NULL )
	, m_selection_texture_index( 0 )
	, m_selection_alpha( 0.0f )
	, m_SelectedElement( 0 )
	, m_ElementHeight( DEFAULT_ELEMENT_HEIGHT )
	, m_Alignment( ALIGN_DOWN_RIGHT )
	, m_buttondown( false )		
	, m_bAutoSize( false )
{	
	SetType( UI_TYPE_POPUPMENU );
	SetInputType( TYPE_INPUT_ALL );
	SetVisible( false );

	m_pBackground = new UIDialogBox();
	m_pBackground->CreateCommonCtrl( "cpbox_thin_dark" );

	gpstring sFont;
	int fontSize = 0;
	fhWindow.Get( "font_size", fontSize );
	if ( fhWindow.Get( "font_type", sFont ) )
	{
		m_pFont = gUITextureManager.LoadFont( sFont, true );
		if ( !m_pFont ) 
		{
			DWORD dwBgColor = 0;
			if ( fhWindow.Get( "font_bg_color", dwBgColor ) )
			{
				m_pFont = gUITextureManager.LoadFont( sFont, fontSize, dwBgColor );
			}
			else
			{
				m_pFont = gUITextureManager.LoadFont( sFont, fontSize );
			}
		}		
	}	
	else
	{
		gperrorf(( "Using default system font for GUI element: %s, change it!", GetName().c_str() ));
		m_pFont = gUITextureManager.GetDefaultFont();
	}
	
	fhWindow.Get( "element_height", m_ElementHeight );

	fhWindow.Get( "auto_size", m_bAutoSize );

	gpstring alignment;
	fhWindow.Get( "alignment", alignment );
	if ( alignment.same_no_case( "up_left" ) )
	{
		m_Alignment = ALIGN_UP_LEFT;
	}
	else if ( alignment.same_no_case( "up_right" ) )
	{
		m_Alignment = ALIGN_UP_RIGHT;
	}
	else if ( alignment.same_no_case( "down_left" ) )
	{
		m_Alignment = ALIGN_DOWN_LEFT;
	}
	
	FastFuelHandle hSelectionBox = fhWindow.GetChildNamed( "selection_box" );
	if ( hSelectionBox )
	{
		gpstring texture;
		hSelectionBox.Get( "texture", texture );
		if ( !texture.empty() )
		{
			m_selection_texture_index = gUITextureManager.LoadTexture( texture, GetInterfaceParent() );
		}

		hSelectionBox.Get( "alpha", m_selection_alpha );
	}

}


UIPopupMenu::UIPopupMenu()
	: UIWindow()
	, m_pBackground( NULL )
	, m_pFont( NULL )
	, m_selection_texture_index( 0 )
	, m_selection_alpha( 0.0f )
	, m_SelectedElement( 0 )
	, m_ElementHeight( DEFAULT_ELEMENT_HEIGHT )
	, m_Alignment( ALIGN_DOWN_RIGHT )
	, m_buttondown( false )		
	, m_bAutoSize( false )
{
	SetType( UI_TYPE_POPUPMENU );
	SetInputType( TYPE_INPUT_ALL );
	SetVisible( false );	
}


UIPopupMenu::~UIPopupMenu()
{
	delete m_pBackground;

	RemoveAllElements();

	if ( m_selection_texture_index != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_selection_texture_index );
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Inherited functions

bool UIPopupMenu::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true )
	{
		return true;
	}
	else if ( action == ACTION_SHOWMENU )
	{
		if ( parameter.same_no_case( "true" ) )
		{
			ShowAtCursor();
		}
		else
		{
			SetVisible( false );			
		}
		return true;
	}	
	return false;
}


bool UIPopupMenu::ProcessMessage( UIMessage & msg )
{
	switch ( msg.GetCode() ) 
	{
		case MSG_LBUTTONDOWN:
		{
			m_buttondown = true;
			break;
		}
	
		case MSG_LBUTTONUP:
		{
			if ( m_buttondown == true ) 
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_MENUSELECT ), this );
				SetVisible( false );
			}
			m_buttondown = false;
			break;
		}

		case MSG_GLOBALLBUTTONUP:
		{
			m_buttondown = false;
			break;
		}

		case MSG_GLOBALLBUTTONDOWN:
		case MSG_GLOBALRBUTTONDOWN:
		{
			if ( !UIWindow::HitDetect( gUIShell.GetMouseX(), gUIShell.GetMouseY() ) ) 
			{
				SetVisible( false );
			}		
			break;
		}
		
		case MSG_ROLLOFF:
		{
			m_SelectedElement = INVALID_ELEMENT;
			break;
		}
		
		case MSG_ACTIVATEMENU:
		{
			ShowAtCursor();
			break;
		}
	}

	return true;
}


void UIPopupMenu::SetMousePos( unsigned int x, unsigned int y )
{	
	if ( GetVisible() )
	{
		int elementIndex = 0;
		int elementTop = GetRect().top;
		for ( ElementColl::iterator i = m_Elements.begin(); i != m_Elements.end(); ++i, ++elementIndex )
		{
			if ( ((int)x > GetRect().left) && ((int)x < GetRect().right) &&
				((int)y > elementTop) && ((int)y < elementTop + m_ElementHeight) )
			{
				if ( !i->text.same_no_case( L"-" ) )
				{
					m_SelectedElement = elementIndex;
				}
				return;
			}
			elementTop += m_ElementHeight;
		}
	}
}


void UIPopupMenu::Draw()
{
	UIWindow::Draw();
	if ( GetVisible() )
	{
		m_pBackground->Draw();

		int elementIndex = 0;
		int elementTop = GetRect().top;
		for ( ElementColl::iterator i = m_Elements.begin(); i != m_Elements.end(); ++i, ++elementIndex )
		{
			UIRect element_rect;
			element_rect.left	= GetRect().left;
			element_rect.top	= elementTop;

			if ( i->text.same_no_case( L"-" ) )
			{
				gUIShell.GetRenderer().SetTextureStageState(	0,
																D3DTOP_DISABLE,
																D3DTOP_SELECTARG1,
																D3DTADDRESS_WRAP,
																D3DTADDRESS_WRAP,
																D3DTEXF_LINEAR,
																D3DTEXF_LINEAR,
																D3DTEXF_LINEAR,
																D3DBLEND_SRCALPHA,
																D3DBLEND_INVSRCALPHA,
																false );

				tVertex LineVerts[2];
				memset( LineVerts, 0, sizeof( tVertex ) * 2 );

				LineVerts[0].x			= (float)GetRect().left + 4;
				LineVerts[0].y			= (float)elementTop + m_ElementHeight / 2;
				LineVerts[0].z			= 0.0f;
				LineVerts[0].rhw		= 1.0f;
				LineVerts[0].color		= 0xFF808080;

				LineVerts[1].x			= (float)GetRect().right - 4;
				LineVerts[1].y			= (float)elementTop + m_ElementHeight / 2;
				LineVerts[1].z			= 0.0f;
				LineVerts[1].rhw		= 1.0f;
				LineVerts[1].color		= 0xFF808080;

				gUIShell.GetRenderer().DrawPrimitive( D3DPT_LINESTRIP, LineVerts, 2, TVERTEX, 0, 0 );
			}
			else
			{
				if ( m_SelectedElement == elementIndex ) 
				{
					element_rect.right	= GetRect().right;
					element_rect.bottom = element_rect.top + m_ElementHeight;
					DrawComponent( element_rect, m_selection_texture_index, m_selection_alpha );
				}

				if ( m_pFont )
				{
					int width = 0, height = 0;
					m_pFont->CalculateStringSize( i->text, width, height );

					int topOffset = (m_ElementHeight - height) / 2;

					m_pFont->Print( GetRect().left + ELEMENT_PADDING, elementTop + topOffset, i->text.c_str(), 0xFFFFFFFF, true );
				}
			}

			elementTop += m_ElementHeight;
		}
	}
}


#if !GP_RETAIL
void UIPopupMenu::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "button" );
		}
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Element access

void UIPopupMenu::AddElement( gpwstring elementText, int tag )
{
	bool bFound = false;
	for ( ElementColl::iterator i = m_Elements.begin(); i != m_Elements.end(); ++i )
	{
		if ( elementText.same_no_case( i->text ) || (tag == i->tag) )
		{
			bFound = true;
			break;
		}
	}

	if ( !bFound )
	{
		m_Elements.push_back( MenuElement( elementText, tag ) );

		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{
			m_pFont->RegisterString( elementText );						
		}
	}
}

void UIPopupMenu::RemoveElement( gpwstring elementText )
{
	for ( ElementColl::iterator i = m_Elements.begin(); i != m_Elements.end(); ++i )
	{
		if ( elementText.same_no_case( i->text ) )
		{
			if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
			{
				m_pFont->UnregisterString( i->text );						
			}
			
			m_Elements.erase( i );
			break;
		}
	}
}

void UIPopupMenu::RemoveElement( int tag )
{
	for ( ElementColl::iterator i = m_Elements.begin(); i != m_Elements.end(); ++i )
	{
		if ( tag == i->tag )
		{
			if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
			{
				m_pFont->UnregisterString( i->text );						
			}
			
			m_Elements.erase( i );
			break;
		}
	}
}

void UIPopupMenu::RemoveAllElements()
{
	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		for ( ElementColl::iterator i = m_Elements.begin(); i != m_Elements.end(); ++i )
		{
			m_pFont->UnregisterString( i->text );
		}
	}

	m_Elements.clear();
}


int UIPopupMenu::GetSelectedElement()
{
	int elementIndex = 0;
	for ( ElementColl::iterator i = m_Elements.begin(); i != m_Elements.end(); ++i, ++elementIndex )
	{
		if ( elementIndex == m_SelectedElement ) 
		{
			return ( i->tag );
		}
	}
	return ( INVALID_ELEMENT );
}


gpwstring UIPopupMenu::GetSelectedElementText()
{
	int elementIndex = 0;
	for ( ElementColl::iterator i = m_Elements.begin(); i != m_Elements.end(); ++i, ++elementIndex )
	{
		if ( elementIndex == m_SelectedElement ) 
		{
			return ( i->text );
		}
	}
	return ( gpwstring() );
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////


void UIPopupMenu::ShowAtCursor( eAlignment align )
{
	Show( gUIShell.GetMouseX(), gUIShell.GetMouseY(), align );
}


void UIPopupMenu::Show( int x, int y, eAlignment align )
{
	int maxWidth = 0;
	int maxHeight = m_Elements.size() * m_ElementHeight;

	if ( align == ALIGN_DEFAULT )
	{
		align = m_Alignment;
	}

	if ( m_bAutoSize )
	{
		for ( ElementColl::iterator i = m_Elements.begin(); i != m_Elements.end(); ++i )
		{
			int width = 0, height = 0;
			m_pFont->CalculateStringSize( i->text, width, height );

			if ( width > maxWidth )
			{
				maxWidth = width;
			}
		}
		maxWidth += ELEMENT_PADDING * 2;
	}
	else
	{
		maxWidth = GetRect().Width();
	}

	// set horizontal position
	if ( (align == ALIGN_UP_LEFT) || (align == ALIGN_DOWN_LEFT) )
	{
		GetRect().left = x - maxWidth;
		if ( GetRect().left < 0 )
		{
			GetRect().left = 0;
		}
		GetRect().right = GetRect().left + maxWidth;
	}
	else
	{
		GetRect().right = x + maxWidth;
		if ( GetRect().right > gUIShell.GetScreenWidth() )
		{
			GetRect().right = gUIShell.GetScreenWidth() - 1;
		}
		GetRect().left = GetRect().right - maxWidth;
	}

	// set vertical position
	if ( (align == ALIGN_UP_LEFT) || (align == ALIGN_UP_RIGHT) )
	{
		GetRect().top = y - maxHeight;
		if ( GetRect().top < 0 )
		{
			GetRect().top = 0;
		}
		GetRect().bottom = GetRect().top + maxHeight;
	}
	else
	{
		GetRect().bottom = y + maxHeight;
		if ( GetRect().bottom > gUIShell.GetScreenHeight() )
		{
			GetRect().bottom = gUIShell.GetScreenHeight() - 1;
		}
		GetRect().top = GetRect().bottom - maxHeight;
	}

	m_pBackground->GetRect() = GetRect();

	m_SelectedElement = INVALID_ELEMENT;

	SetVisible( true );
}
