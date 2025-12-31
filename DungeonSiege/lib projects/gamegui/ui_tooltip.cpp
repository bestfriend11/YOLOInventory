//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_tooltip.cpp
// Author(s):  Chad Queen, Adam Swensen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "fuel.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_tooltip.h"



UIToolTip::UIToolTip( FuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
	: UIWindow( fhWindow, parent_interface, parent )
	, m_bFixedLocation( false )
	, m_font_width( 0 )
	, m_font_height( 0 )
	, m_scroll_rate( 0.0f )
	, m_scroll_offset( 0.0f )
	, m_max_height( GetScreenHeight()/2 )
	, m_max_width( GetScreenWidth()/2 ) 
	, m_lead_element( 0 )
	, m_bStopText( false )
	, m_pFont( 0 )
{	
	SetType( UI_TYPE_TOOLTIP );
	SetInputType( TYPE_INPUT_ALL );
	m_buttondown = false;

	// Load a font 
	SetTextColor( 0xFFFFFFFF );	
	gpstring sFont;	
	fhWindow->Get( "font_color", m_dwColor );
	fhWindow->Get( "font_size", m_font_size );
	if ( fhWindow->Get( "font_type", sFont ) ) {
		m_pFont = gUITextureManager.LoadFont( sFont, true );
		if ( !m_pFont )
		{
			m_pFont = gUITextureManager.LoadFont( sFont, m_font_size );
		}
	}	
	if ( !m_pFont )
	{		
		gpwarningf(( "Using default system font for GUI element: %s, change it!", GetName().c_str() ));
		m_pFont = gUITextureManager.GetDefaultFont();
	}

	m_pFont->CalculateStringSize( "W", m_font_width, m_font_height );

	// Is the textbox a fixed box, or a tooltip style box 
	fhWindow->Get( "fixed_location", m_bFixedLocation );

	// Determine scroll rate of text ( can be used for dialogue, etc. )
	if ( fhWindow->Get( "scroll_rate", m_scroll_rate ) )
	{
		if ( m_scroll_rate > 0.0f )
		{
			m_scroll_offset = (float)GetRect().bottom-(float)GetRect().top;
		}
	}
	
	// Get text justification
	gpstring justify( "left" );
	fhWindow->Get( "justify", justify );
	if ( justify.same_no_case( "right" ) )
	{
		m_justification = JUSTIFY_RIGHT;			
	}
	else if ( justify.same_no_case( "center" ) ) 
	{
		m_justification = JUSTIFY_CENTER;				
	}
	else 
	{
		m_justification = JUSTIFY_LEFT;
	}		

	fhWindow->Get( "max_height", m_max_height );
	fhWindow->Get( "max_width", m_max_width );

	gpstring sText;
	if ( fhWindow->Get( "text", sText ) )
	{
		SetText( sText );
	}

}
UIToolTip::UIToolTip()
	: UIWindow()
	, m_bFixedLocation( false )
	, m_font_width( 0 )
	, m_font_height( 0 )
	, m_scroll_rate( 0.0f )
	, m_scroll_offset( 0.0f )
	, m_max_height( GetScreenHeight()/2 )
	, m_max_width( GetScreenWidth()/2 ) 
	, m_lead_element( 0 )
	, m_pFont( 0 )
{
	SetTextColor( 0xFFFFFFFF );	
	SetType( UI_TYPE_TOOLTIP );
	SetInputType( TYPE_INPUT_ALL );
	m_buttondown = false;
}


UIToolTip::~UIToolTip()
{
}


bool UIToolTip::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true )
	{
		return true;
	}	
	return false;
}

bool UIToolTip::ProcessMessage( UIMessage msg )
{
	switch ( msg.GetCode() ) 
	{
	case MSG_LBUTTONDOWN:
		{
			m_buttondown = true;		
		}
		break;
	case MSG_LBUTTONUP:
		{
			if ( m_buttondown == true ) {
				gUIMessenger.SendUIMessage( UIMessage( MSG_BUTTONPRESS ), this );
				if ( m_bFixedLocation == false )
				{
					SetVisible( false );
				}
			}
			m_buttondown = false;
		}
		break;
	case MSG_GLOBALLBUTTONUP:
		{
			m_buttondown = false;
		}
		break;
	case MSG_ROLLOFF:
		{
		}
		break;

	}
	return true;
}



void UIToolTip::SetVisible( bool bVisible )
{
	UIItemVec items;
	gUIShell.FindActiveItems( items );
	if ( items.size() != 0 )
	{
		UIWindow::SetVisible( false );
	}
	else
	{
		UIWindow::SetVisible( bVisible );
	}
}


bool UIToolTip::EnlargeRect()
{
	if ( (GetRect().right-GetRect().left) >= m_max_width )
	{
		SetRect( GetRect().left, GetRect().right + m_font_width, GetRect().top, GetRect().bottom );
	}
	else if ( (GetRect().bottom-GetRect().top) >= m_max_height )
	{
		SetRect( GetRect().left, GetRect().right, GetRect().top, GetRect().bottom + m_font_height );
	}
	else 
	{
		return false;
	}

	return true;
}


bool UIToolTip::DoesTextFit( gpstring sText )
{
	StringVec testStrings;
	gpstring sCurrent;

	int width	= 0;
	int height	= 0;
	for ( unsigned int i = 0; i != sText.size(); ++i ) 
	{		
		sCurrent.appendf( "%c", sText[i] );

		m_pFont->CalculateStringSize( sCurrent.c_str(), width, height );
		if ( width > (GetRect().right-GetRect().left) )
		{
			int pos = sCurrent.find_last_of( " " );
			if ( pos != (int)gpstring::npos )
			{
				gpstring sTemp = sCurrent.substr( 0, pos );				
				sCurrent = sTemp;			
				testStrings.push_back( sCurrent );
				sCurrent.clear();
				if ( sText[i] != ' ' )
				{
					sCurrent.appendf( "%c", sText[i] );
				}
			}
			else
			{
				return false;
			}
		}		
	}
	
	if ( !(( ((int)testStrings.size()+1) * m_font_height ) <= ( GetRect().bottom-GetRect().top )) )
	{
		return false;
	}
	
	return true;
}


void UIToolTip::CenterWindowToMousePos()
{
	int width	= GetRect().right - GetRect().left;
	int height	= GetRect().bottom - GetRect().top;
	SetRect( GetMouseX() - (width/2), GetMouseX() + (width/2), GetMouseY() - (height/2), GetMouseY() + (height/2) );

	if ( GetRect().left < 0 )
	{
		SetRect( 0, width, GetRect().top, GetRect().bottom );
	}
	else if ( GetRect().right > GetScreenWidth() )
	{
		SetRect( GetScreenWidth()-width, GetScreenWidth(), GetRect().top, GetRect().bottom );
	}

	if ( GetRect().top < 0 )
	{
		SetRect( GetRect().left, GetRect().right, 0, height );
	}
	else if ( GetRect().bottom > GetScreenHeight() )
	{
		SetRect( GetRect().left, GetRect().right, GetScreenHeight()-height, GetScreenHeight() );
	}
}


void UIToolTip::SetText( gpstring sText )
{
	if ( m_scroll_rate > 0.0f )
	{
		m_scroll_offset = (float)GetRect().bottom-(float)GetRect().top;
	}

	m_lines.clear();
	if ( !m_bFixedLocation )
	{
		SetRect( 0, m_font_width, 0, m_font_height );
		while ( !DoesTextFit( sText ) )
		{
			if ( EnlargeRect() == false )
			{
				return;
			}
		}		
	}
		
	TextLine line;
	line.dwColor = GetTextColor();

	int width	= 0;
	int height	= 0;
	for ( unsigned int i = 0; i != sText.size(); ++i ) 
	{		
		line.sText.appendf( "%c", sText[i] );

		m_pFont->CalculateStringSize( line.sText.c_str(), width, height );
		if ( width > (GetRect().right-GetRect().left) )
		{
			int pos = line.sText.find_last_of( " " );
			if ( (unsigned int)pos == gpstring::npos )
			{
				return;
			}

			TextLineVec::iterator j;
			int runningSize = 0;
			for ( j = m_lines.begin(); j != m_lines.end(); ++j )
			{
				runningSize += (*j).sText.size() + 1;
			}
			
			gpstring sTemp = line.sText.substr( 0, pos );
			i = pos + runningSize;
			line.sText = sTemp;
			m_lines.push_back( line );
			line.sText.clear();
			if ( sText[i] != ' ' )
			{
				line.sText.appendf( "%c", sText[i] );
			}
		}		
	}
	
	if ( ( ((int)m_lines.size()+1) * m_font_height ) <= ( GetRect().bottom-GetRect().top ) )
	{
		m_lines.push_back( line );	
	}
	else 
	{
		return;	
	}		
}


void UIToolTip::SetLineText( int lineNum, gpstring sText, DWORD dwColor )
{
	if ( lineNum == 0 )
	{
		m_lines.clear();
	}

	TextLineVec::iterator i;
	int index = 0;
	bool bExists = false;
	for ( i = m_lines.begin(); i != m_lines.end(); ++i ) 
	{
		if ( index == lineNum )
		{
			(*i).dwColor = dwColor;
			(*i).sText = sText;
			bExists = true;
		}
		index++;
	}

	if ( !bExists )
	{
		for ( int j = 0; j != (lineNum-index); ++j )
		{
			TextLine line;
			m_lines.push_back( line );
		}

		TextLine line;
		line.sText = sText;
		line.dwColor = dwColor;
		m_lines.push_back( line );
	}

	if ( !m_bFixedLocation )
	{
		SetRect( 0, 0, 0, 0 );
		for ( i = m_lines.begin(); i != m_lines.end(); ++i ) 		
		{
			int width	= 0;
			int height	= 0;
			m_pFont->CalculateStringSize( (*i).sText.c_str(), width, height );
			if ( width > (GetRect().right - GetRect().left) )
			{
				SetRect( 0, width, 0, 0 );
			}
		}		
		SetRect( GetRect().left, GetRect().right, 0, m_lines.size() * m_font_height );		
	}
}

void UIToolTip::Draw()
{
	UIWindow::Draw();
	if ( GetVisible() )
	{
		TextLineVec::iterator i;
		int index = 0;
		for ( i = m_lines.begin(); i != m_lines.end(); ++i ) 
		{
			int x_offset = GetRect().left;
			int y_offset = GetRect().top + ( m_font_height * index ) + (int)floor((double)m_scroll_offset);

			switch ( m_justification )
			{
			case JUSTIFY_LEFT:
				{
					x_offset = GetRect().left;
				}
				break;
			case JUSTIFY_RIGHT:
				{
					int width	= 0;
					int height	= 0;
					m_pFont->CalculateStringSize( (*i).sText.c_str(), width, height );
					x_offset = GetRect().right - width;
				}
				break;
			case JUSTIFY_CENTER:
				{
					int width	= 0;
					int height	= 0;
					m_pFont->CalculateStringSize( (*i).sText.c_str(), width, height );
					x_offset = (((GetRect().right-GetRect().left) - width)/2) + GetRect().left;
				}
				break;
			}

			if (( (y_offset+m_font_height-1) <= GetRect().bottom ) && ( y_offset >= GetRect().top ))
			{
				m_pFont->Print( x_offset, y_offset, (*i).sText.c_str(), (*i).dwColor );
			}

			++index;
		}
	}
}


void UIToolTip::Update( double seconds )
{
	if ( m_scroll_rate != 0.0f )
	{
		m_scroll_offset -= (m_scroll_rate*(float)seconds);
		if ( m_scroll_offset < 0 )
		{
			m_scroll_offset = 0;
		}
	}
}


void UIToolTip::SetMousePos( unsigned int x, unsigned int y )
{
	UIWindow::SetMousePos( x, y );	
}


unsigned int UIToolTip::GetNumElements()
{
	return m_lines.size();
}


unsigned int UIToolTip::GetMaxActiveElements()
{
	return ( (GetRect().bottom-GetRect().top)/m_font_height );
}


void UIToolTip::SetScrolling( bool bScrolling )
{
	m_bStopText = bScrolling;
	if ( m_bStopText )
	{
		m_scroll_offset = 0;		
	}
}


void UIToolTip::Save( FuelHandle hSave, bool bChild )
{
	UIWindow::Save( hSave, bChild );
}