//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_chatbox.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "precomp_gamegui.h"
#include "ui_chatbox.h"
#include "ui_emotes.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"
#include "keys.h"
#include "LocHelp.h"


UIChatBox::UIChatBox( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
	: UIWindow( fhWindow, parent_interface, parent )			
	, m_pFont( 0 )
	, m_max_size( 0 )
	, m_font_height( 0 )
	, m_pVSlider( 0 )
	, m_pSliderUp( 0 )
	, m_pSliderDown( 0 )
	, m_vertical_pos( 0 )
	, m_bSlider( false )
	, m_bSliderVisible( false )
	, m_lineTimeout( 0.0f )
	, m_groupFadeout( -1.0f )
	, m_lead_element( 0 )
	, m_bScrollDown( true )
	, m_bRetainScrollPos( false )
	, m_bConsole( false )
	, m_lastEntryTime( 0.0f )
	, m_lastActiveTime( 0.0f )
	, m_editBoxActive( false )
	, m_bDisableUpdate( false )
	, m_dwFontColor( 0xffffffff )
	, m_bDeleteTimeouts( true )
	, m_historyPos( 0 )
	, m_bViewHistory( false )
	, m_bNoDraw( false )
	, m_bDrawLeadElement( false )
	, m_bRolloverView( false )
	, m_maxHistory( 0 )
	, m_bLockView( 0 )
	, m_lineSpacer( 0 )
	, m_bMinimized( false )
	, m_lineSize( 0 )
	, m_lineSpeedup( 0.0f )
	, m_wrapTolerance( 200 )
{
	SetType( UI_TYPE_CHATBOX );
	SetInputType( TYPE_INPUT_ALL );
	
	gpstring sFont;		
	if ( fhWindow.Get( "font_type", sFont ) ) 
	{
		fhWindow.Get( "font_color", m_dwFontColor );
		m_pFont = gUITextureManager.LoadFont( sFont, true );		
		if ( !m_pFont )
		{
			DWORD dwBgColor = 0;
			if ( fhWindow.Get( "font_bg_color", dwBgColor ) )
			{
				m_pFont = gUITextureManager.LoadFont( sFont, m_font_height, dwBgColor );
			}
			else
			{
				m_pFont = gUITextureManager.LoadFont( sFont, m_font_height );
			}
		}
		
		if ( m_pFont )
		{
			int width = 0;
			int height = 0;
			m_pFont->CalculateStringSize( "TEST", width, height );
			SetFontHeight( height );
		}
	}
	fhWindow.Get( "max_string_size", m_max_size );

	gpstring sRes;
	if ( fhWindow.Get( "font_640x480", sRes ) )
	{
		m_resFontMap.insert( ResFontPair( 640, sRes ) );
	}

	if ( fhWindow.Get( "font_800x600", sRes ) )
	{
		m_resFontMap.insert( ResFontPair( 800, sRes ) );
	}

	if ( fhWindow.Get( "font_1024x768", sRes ) )
	{
		m_resFontMap.insert( ResFontPair( 1024, sRes ) );
	}

	if ( fhWindow.Get( "font_1280x1024", sRes ) )
	{
		m_resFontMap.insert( ResFontPair( 1280, sRes ) );
	}
	
	if ( fhWindow.Get( "text", m_text ) && m_text.size() != 0 )
	{
		if ( LocMgr::DoesSingletonExist() )
		{
			gLocMgr.TranslateUiText( m_text, GetInterfaceParent().c_str(), GetName().c_str() );
			m_pFont->RegisterString( m_text );
		}
	}
	if ( m_max_size == 0 )
	{
		m_max_size = m_text.size();
	}	

	fhWindow.Get( "has_slider", m_bSlider );

	fhWindow.Get( "line_timeout", m_lineTimeout );

	fhWindow.Get( "group_fade_out", m_groupFadeout );
	
	fhWindow.Get( "scroll_down", m_bScrollDown );

	fhWindow.Get( "retain_scroll_position", m_bRetainScrollPos );

	fhWindow.Get( "console", m_bConsole );

	fhWindow.Get( "delete_timeouts", m_bDeleteTimeouts );

	fhWindow.Get( "max_history", m_maxHistory );

	if ( fhWindow.Get( "line_spacer", m_lineSpacer ) )
	{
		m_font_height += m_lineSpacer;
	}
	else
	{
		if ( !gUIShell.GetLineSpacer( GetName() ) )
		{
			m_font_height += gUIShell.GetLineSpacer( "default" );
		}
		else
		{
			m_font_height += gUIShell.GetLineSpacer( GetName() );
		}
	}

	bool bCommonCtrl = false;
	if ( fhWindow.Get( "common_control", bCommonCtrl ) ) 
	{
		CreateCommonCtrl();		
		return;
	}	
}
UIChatBox::UIChatBox()
  	: UIWindow()
	, m_pFont( 0 )
	, m_max_size( 0 )
	, m_font_height( 0 )
	, m_pVSlider( 0 )
	, m_pSliderUp( 0 )
	, m_pSliderDown( 0 )
	, m_vertical_pos( 0 )
	, m_bSlider( false )
	, m_bSliderVisible( false )
	, m_lineTimeout( 0.0f )
	, m_groupFadeout( -1.0f )
	, m_lead_element( 0 )
	, m_bScrollDown( true )
	, m_bRetainScrollPos( false )
	, m_bConsole( false )
	, m_lastEntryTime( 0.0f )
	, m_lastActiveTime( 0.0f )
	, m_editBoxActive( false )
	, m_bDisableUpdate( false )
	, m_dwFontColor( 0xffffffff )
	, m_bDeleteTimeouts( true )
	, m_historyPos( 0 )
	, m_bViewHistory( false )
	, m_bNoDraw( false )
	, m_bDrawLeadElement( false )
	, m_bRolloverView( false )
	, m_maxHistory( 0 )
	, m_bLockView( 0 )
	, m_lineSpacer( 0 )
	, m_bMinimized( false )
	, m_lineSize( 0 )
	, m_lineSpeedup( 0.0f )
	, m_wrapTolerance( 200 )
{
	SetType( UI_TYPE_EDITBOX );
	SetInputType( TYPE_INPUT_ALL );
}


UIChatBox::~UIChatBox()
{
	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && m_pFont )
	{
		m_pFont->UnregisterString( m_text );
	}
}


bool UIChatBox::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true )
	{
		return true;
	}
	return false;
}


bool UIChatBox::ProcessMessage( UIMessage & msg )
{
	UIWindow::ProcessMessage( msg );

	switch ( msg.GetCode() ) 
	{
	case MSG_WHEELUP:
		{
			if ( GetVSlider() && !m_bConsole )
			{
				GetVSlider()->SetValue( GetVSlider()->GetValue() - 2 );
			}
		}
		break;
	case MSG_WHEELDOWN:
		{
			if ( GetVSlider() && !m_bConsole )
			{
				GetVSlider()->SetValue( GetVSlider()->GetValue() + 2 );
			}
		}
		break;
	}
	return true;
}


void UIChatBox::Update( double seconds )
{
	if ( !GetDisableUpdate() && !m_bViewHistory && !m_bNoDraw )
	{
		if ( seconds < 1.0f )
		{
			m_lastEntryTime += (float)seconds;
		}
	}

	if( !GetDisableUpdate() )
	{
		if ( seconds < 1.0f )
		{
			m_lastActiveTime += (float)seconds;
		}
	
		if( m_editBoxActive == true )
		{
			m_lastActiveTime = 0.0;
		}

		if( m_groupFadeout > 0 )
		{
			UIWindowVec group	= gUIShell.ListWindowsOfGroup( GetGroup() );
			UIWindowVec::iterator i;
			for ( i = group.begin(); i != group.end(); ++i ) 
			{
				// maybe put in a slow fade out... when we get text that fades too :)
				//(*i)->SetAlpha( max( 0.0f, 1.0f - (m_lastActiveTime)/m_groupFadeout) );		
				(*i)->SetVisible( m_lastActiveTime < m_groupFadeout );
			}

			if( m_bSlider && GetVSlider() )
			{
				GetVSlider()->SetVisible( m_bSliderVisible && m_lastActiveTime < m_groupFadeout );
			}
		}
		

		if (( m_lastEntryTime >= (m_lineTimeout-GetLineSpeedUp()) ) && ( m_lineTimeout != 0 ))
		{	
			if (( m_bConsole ) && ( m_chat.size() != 0 ))
			{			
				if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
				{
					WStringVec::iterator iString;
					for ( iString = (*(m_chat.begin())).vText.begin(); iString != (*(m_chat.begin())).vText.end(); ++iString )
					{
						m_pFont->UnregisterString( *iString );
					}
				}
				
				if ( m_bDeleteTimeouts || GetNumElements() > (unsigned int)m_maxHistory )
				{
					m_chat.erase( m_chat.begin() );
				}
				else
				{
					SetLeadElement( GetLeadElement() + 1 );
					if ( m_pVSlider )
					{
						m_pVSlider->SetValue( GetLeadElement() );
					}
				}
				m_lastEntryTime = 0.0f;	
			}
			else
			{
				SubmitText( gpwstring(), 0xFFFFFFFF );	
				m_lastEntryTime = 0.0f;	
			}
		}	
		else if ( GetNumElements() > (unsigned int)m_maxHistory )
		{
			m_chat.erase( m_chat.begin() );
			SetLeadElement( GetLeadElement()-1 );
		}
	}
}


void UIChatBox::Draw()
{
	if ( GetDisableUpdate() )
	{
		return;
	}

	if ( !m_bConsole )
	{
		UIWindow::Draw();
	}
	
	m_linesLastVisible = 0;

	if ( GetVisible() == true ) 
	{
		m_bNoDraw = true;
		if ( GetParentWindow() && ( GetParentWindow()->GetVisible() == false )) 
		{			
			return;			
		}

		if ( m_pFont ) 
		{						
			if ( m_bSlider )
			{			
				ChatVec::iterator	i;
				WStringVec::iterator j;
				int index = 0;
				for ( i = m_chat.begin(); i != m_chat.end(); ++i ) 
				{	
					if ( !m_bDeleteTimeouts )
					{
						int y = GetRect().top + ( (index-GetLeadElement()+1) * GetFontHeight() );
						if ( y > GetRect().bottom )
						{
							m_lastEntryTime = m_lineTimeout; 
						}
					}

					for ( j = (*i).vText.begin(); j != (*i).vText.end(); ++j )
					{				
						if (( GetRect().top+(GetFontHeight()*index) >= (GetLeadElement()*m_font_height) ) &&
							( GetRect().top+(GetFontHeight()*(index+1)) <= (GetRect().bottom+(GetLeadElement()*m_font_height)) ))
						{
							int y = GetRect().top+(GetFontHeight()*index) - (GetLeadElement()*m_font_height);

							if ( y >= GetRect().top ) 
							{
								m_bNoDraw = false;
								if ( !m_bDrawLeadElement || GetRolloverView() )
								{
									// draw the background of the console
									m_linesLastVisible++;
									if ( m_bConsole )
									{
										UIRect drawRect = GetRect();
										drawRect.top = y;
										drawRect.bottom = y + m_font_height;
										UIWindow::DrawComponent( drawRect, GetTextureIndex(), GetAlpha() );
									}
									// print the line of text
									gUIEmotes.Draw( m_pFont, GetRect().left, y, (*j).c_str(), (*i).dwColor, (UIWindow*) this, true );
								}
								else
								{
									if ( m_bViewHistory )
									{
										if ( i == (m_chat.end()-(1+m_historyPos-GetLeadElement())) )
										{
											m_linesLastVisible++;
											if ( m_bConsole )
											{
												UIRect drawRect = GetRect();
												drawRect.top = y;
												drawRect.bottom = y + m_font_height;
												UIWindow::DrawComponent( drawRect, GetTextureIndex(), GetAlpha() );
											}
											gUIEmotes.Draw( m_pFont, GetRect().left, GetRect().top, (*j).c_str(), (*i).dwColor, (UIWindow*) this, true );
										}
									}
									else if ( j == ((*i).vText.end()-1) && i == (m_chat.end()-1) )									
									{
										m_linesLastVisible++;
										if ( m_bConsole )
										{
											UIRect drawRect = GetRect();
											drawRect.top = y;
											drawRect.bottom = y + m_font_height;
											UIWindow::DrawComponent( drawRect, GetTextureIndex(), GetAlpha() );
										}
										gUIEmotes.Draw( m_pFont, GetRect().left, GetRect().top, (*j).c_str(), (*i).dwColor, (UIWindow*) this, true );
									}									
								}
							}
						}
						index++;
											
					}					
				}			
			}

			else 
			{
				if ( m_bConsole )
				{
					ChatVec::iterator	i;
					WStringVec::iterator j;
					int index = 0;
					for ( i = m_chat.begin(); i != m_chat.end(); ++i )
					{
						int y = GetRect().top + ( index * GetFontHeight() );
						if ( y > GetRect().bottom )
						{
							m_lastEntryTime = m_lineTimeout; 
						}	
						else
						{
							for ( j = (*i).vText.begin(); j != (*i).vText.end(); ++j )
							{
								m_bNoDraw = false;

								m_linesLastVisible++;
								if ( m_bConsole )
								{
									UIRect drawRect = GetRect();
									drawRect.top = y;
									drawRect.bottom = y + m_font_height;
									UIWindow::DrawComponent( drawRect, GetTextureIndex(), GetAlpha() );
								}
								gUIEmotes.Draw( m_pFont, GetRect().left, y, (*j).c_str(), (*i).dwColor, (UIWindow*) this, true );
								index++;
							}
						}
					}
				}
				else
				{
					ChatVec::reverse_iterator i;
					WStringVec::reverse_iterator j;
					int index = 0;
					for ( i = m_chat.rbegin(); i != m_chat.rend(); ++i ) 
					{
						for ( j = (*i).vText.rbegin(); j != (*i).vText.rend(); ++j ) 
						{
							int y = (GetRect().bottom - GetFontHeight()) - ( GetFontHeight() * index );
							if ( y >= GetRect().top )
							{
								m_bNoDraw = false;
								m_linesLastVisible++;
								if ( m_bConsole )
								{
									UIRect drawRect = GetRect();
									drawRect.top = y;
									drawRect.bottom = y + m_font_height;
									UIWindow::DrawComponent( drawRect, GetTextureIndex(), GetAlpha() );
								}
								gUIEmotes.Draw( m_pFont, GetRect().left, y, (*j).c_str(), (*i).dwColor, (UIWindow*) this, true );
							}
							else
							{
								return;
							}
							index++;
						}					
					}
				}
			}
		}
	}
}


void UIChatBox::SubmitText( gpwstring sText, DWORD dwColor, bool bBlank )
{
	if ( !m_pFont )
	{
		return;
	}

	if ( !bBlank && sText.empty() )
	{
		return;
	}

	// enable our emotes for all chats!
	gUIEmotes.PrepareString( sText );

	UIChatMessage chat;
	chat.dwColor = dwColor;
	
	gpwstring sLine;
	gpwstring sWord;
	int lineWidth = 0;
	int width	= 0;
	int height	= 0;
	int numLines = 0;
	for ( unsigned int i = 0; i != sText.size(); ++i ) 
	{
		sWord.appendf( L"%c", sText[i] );
		gUIEmotes.CalculateStringSize( m_pFont, sWord.c_str(), width, height );
		if ( (width + lineWidth) > GetRect().Width() )
		{
			if ( sText[i] == ' ' )
			{
				sWord.erase( sWord.end_split() - 1 );
				sLine += sWord;
				sWord.clear();
			}
			else if ( sLine.empty() || (int)width >= m_wrapTolerance )
			{
				sLine.append( sWord );
				sLine.erase( sLine.end_split() - 1 );
				sWord = sWord.right( 1 );
			}

			chat.vText.push_back( sLine );
			numLines++;

			if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && m_pFont )
			{
				m_pFont->RegisterString( sLine );
			}
	
			sLine.clear();
			lineWidth = 0;
		}
		else if ( sText[i] == ' ' )
		{
			sLine += sWord;
			lineWidth += width;
			sWord.clear();
		}
	}
	
	if ( (width + lineWidth) > 0 )
	{
		sLine += sWord;

		chat.vText.push_back( sLine );
		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && m_pFont )
		{
			m_pFont->RegisterString( sLine );
		}
		m_chat.push_back( chat );

		++numLines;
	}

	if ( numLines != 1 && (m_linesLastVisible + numLines) > GetLineSize() )
	{
		int diff = (numLines + m_linesLastVisible) - GetLineSize();
		SetLeadElement( GetLeadElement() + diff );
	}

	if ( !m_bDeleteTimeouts )
	{
		RefreshSlider();
	}	

	if ( m_bViewHistory && !m_bLockView )
	{
		ViewHistoryEnd();
	}
}


void UIChatBox::Clear()
{ 
	m_chat.clear(); 
	SetLeadElement( 0 );
	m_historyPos = 0;
}


void UIChatBox::RefreshSlider()
{
	// make sure that the chat box is active when we move around!
	RegisterActiveChat( true );

	if ( m_bSlider && GetVSlider() && !m_bConsole )
	{
		bool cancelDownScroll = m_bRetainScrollPos && (GetVSlider()->GetValue() < GetVSlider()->GetMax());

		GetVSlider()->SetRange( GetNumElements() - GetMaxActiveElements() );

		if( m_bScrollDown && !cancelDownScroll )
		{
			GetVSlider()->SetValue( GetNumElements() - GetMaxActiveElements() );
		}

		GetVSlider()->CalculateSlider();

		if ( m_bSliderVisible && !GetVSlider()->GetVisible() && (GetVSlider()->GetMax() > GetVSlider()->GetMin()) )
		{
			GetVSlider()->SetVisible( true );
		}
		else if ( GetVSlider()->GetVisible() && (GetVSlider()->GetMax() == GetVSlider()->GetMin()) )
		{
			GetVSlider()->SetVisible( false );
		}

		if ( m_pSliderUp && m_pSliderDown )
		{
			m_pSliderUp->SetRect(	GetRect().right, 
									GetRect().right + m_pSliderUp->GetRect().Width(),
									GetRect().top,
									GetRect().top + m_pSliderUp->GetRect().Height() );
			m_pSliderDown->SetRect( GetRect().right,
									GetRect().right + m_pSliderDown->GetRect().Width(),
									GetRect().bottom - m_pSliderDown->GetRect().Height(),
									GetRect().bottom );
			GetVSlider()->SetRect(	m_pSliderUp->GetRect().left,
									m_pSliderUp->GetRect().right,
									m_pSliderUp->GetRect().bottom,
									m_pSliderDown->GetRect().top );
		}
	}
}


void UIChatBox::CreateCommonCtrl( gpstring sTemplate )
{
	UNREFERENCED_PARAMETER( sTemplate );
	SetIsCommonControl( true );
	
	UIButton * pButtonUp	= 0;
	UIButton * pButtonDown	= 0;
	UISlider * pSliderV		= 0;

	if ( m_bSlider ) 
	{
		gpstring sName;
		pButtonDown = (UIButton *)gUIShell.CreateDefaultWindowOfType( UI_TYPE_BUTTON );
		sName = GetName();
		sName += "_button_down";
		pButtonDown->CreateCommonCtrl( sName, "button_down", this, SLIDER_INCREMENT );
		pButtonDown->SetRect(	GetRect().right, GetRect().right+pButtonDown->GetRect().right,
								GetRect().bottom-(pButtonDown->GetRect().bottom), GetRect().bottom );		
		pButtonDown->SetDrawOrder( GetDrawOrder() + 1 );
		gUIShell.AddWindowToInterface( (UIWindow *)pButtonDown, GetInterfaceParent() );

		pButtonUp = (UIButton *)gUIShell.CreateDefaultWindowOfType( UI_TYPE_BUTTON );
		sName = GetName();
		sName += "_button_up";
		pButtonUp->CreateCommonCtrl( sName, "button_up", this, SLIDER_DECREMENT );
		pButtonUp->SetRect(	GetRect().right, GetRect().right+pButtonUp->GetRect().right,
							GetRect().top, GetRect().top+pButtonUp->GetRect().bottom );		
		pButtonUp->SetDrawOrder( GetDrawOrder() + 1 );
		gUIShell.AddWindowToInterface( (UIWindow *)pButtonUp, GetInterfaceParent() );

		pSliderV = (UISlider *)gUIShell.CreateDefaultWindowOfType( UI_TYPE_SLIDER );
		sName = GetName();
		sName += "_slider_vertical";
		pSliderV->CreateCommonCtrl( sName, "slider", this, true );
		pSliderV->SetRect( pButtonUp->GetRect().left, pButtonUp->GetRect().right,
						   pButtonUp->GetRect().bottom, pButtonDown->GetRect().top );
		pSliderV->SetDrawOrder( GetDrawOrder() + 1 );
		gUIShell.AddWindowToInterface( (UIWindow *)pSliderV, GetInterfaceParent() );

		pButtonUp->SetParentWindow( pSliderV );
		pButtonDown->SetParentWindow( pSliderV );
		pSliderV->AddChild( pButtonUp );
		pSliderV->AddChild( pButtonDown );

		m_pVSlider = pSliderV;
		m_pSliderUp = pButtonUp;
		m_pSliderDown = pButtonDown;
		
		AddChild( (UIWindow *)m_pVSlider );
		m_pVSlider->SetRange( m_maxHistory );

		m_pVSlider->SetVisible( false );
	}
}


int UIChatBox::GetTotalElementsSize()
{
	ChatVec::iterator	i;
	int index = 0;
	for ( i = m_chat.begin(); i != m_chat.end(); ++i ) 
	{
		index += (*i).vText.size();		
	}
			
	return ( index * GetFontHeight() );
}


int	UIChatBox::GetElementsSize()
{	
	return (GetRect().bottom-GetRect().top);
}


unsigned int UIChatBox::GetNumElements()
{
	ChatVec::iterator	i;
	int index = 0;
	for ( i = m_chat.begin(); i != m_chat.end(); ++i ) 
	{
		index += (*i).vText.size();		
	}
	return index;	
}


unsigned int UIChatBox::GetMaxActiveElements()
{
	return ((GetRect().bottom-GetRect().top)/GetFontHeight());
}


void UIChatBox::SetLeadElement( int lead )	
{ 
	m_lead_element = lead; 	
}


void UIChatBox::SetText( gpwstring text )
{ 	
	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && m_pFont )
	{
		m_pFont->UnregisterString( m_text );
		m_pFont->RegisterString( text );
	}
	m_text = text; 
}


#if !GP_RETAIL
void UIChatBox::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "chat_box" );			
			hWindow->Set( "font_type", m_pFont->GetTextureName() );			
			hWindow->Set( "max_string_size", m_max_size );
			hWindow->Set( "text", m_text );
			hWindow->Set( "has_slider", m_bSlider );
			hWindow->Set( "scroll_down", m_bScrollDown );
			hWindow->Set( "line_timeout", m_lineTimeout );
		}
	}
}
#endif


void UIChatBox::ViewHistoryUp()
{
	if ( !m_bViewHistory )
	{
		m_bViewHistory = true;
		m_historyPos = GetLeadElement();
	}

	for ( int i = 0; i != GetLineSize(); ++i )
	{
		if ( GetLeadElement() - 1 >= 0 || m_bDrawLeadElement )
		{
			SetLeadElement( GetLeadElement() - 1 );	
			if ( m_pVSlider )
			{
				m_pVSlider->SetValue( GetLeadElement() );
			}			
		}
	}
}


void UIChatBox::ViewHistoryDown()
{
	if ( !m_bViewHistory )
	{
		m_bViewHistory = true;
		m_historyPos = GetLeadElement();
	}

	for ( int i = 0; i != GetLineSize(); ++i )
	{
		if ( GetLeadElement() + 1 <= m_historyPos )
		{
			SetLeadElement( GetLeadElement() + 1 );
			if ( m_pVSlider )
			{
				m_pVSlider->SetValue( GetLeadElement() );
			}
		}
		else
		{
			ViewHistoryEnd();
		}
	}
}


void UIChatBox::ViewHistoryEnd() 
{
	if ( m_bViewHistory )
	{
		int maxPos = (int)(GetTotalElementsSize() / GetFontHeight());

		if ( m_historyPos > maxPos )
		{
			m_historyPos = maxPos;
		}

		SetLeadElement( m_historyPos );
		if ( m_pVSlider )
		{
			m_pVSlider->SetValue( GetLeadElement() );
		}
	}
	m_bViewHistory = false;		
}


int UIChatBox::GetLineSize()
{
	int size = 0;	
	if ( m_pFont )
	{
		int width = 0, height = 0;
		m_pFont->CalculateStringSize( L"W", width, height );
		size = (GetRect().bottom-GetRect().top) / GetFontHeight();
	}

	return size;
}


void UIChatBox::SetLineSize( int line )
{
	if ( m_pFont )
	{
		int width = 0, height = 0;
		m_pFont->CalculateStringSize( L"W", width, height );	
		SetRect( GetRect().left, GetRect().right, GetRect().top, GetRect().top+(line*GetFontHeight()) );
	}
}


void UIChatBox::ResizeToCurrentResolution( int original_width, int original_height )
{
	UIWindow::ResizeToCurrentResolution( original_width, original_height );

	if ( m_pFont )
	{
		ResFontMap::iterator iFound = m_resFontMap.find( gUIShell.GetScreenWidth() );
		if ( iFound != m_resFontMap.end() )
		{			
			RapiFont * pLastFont = m_pFont;
			m_pFont = gUITextureManager.LoadFont( (*iFound).second, false );
			if ( !m_pFont )
			{
				m_pFont = pLastFont;
			}
			else if ( m_pFont != pLastFont )
			{
				gUITextureManager.DestroyFont( pLastFont );
			}

			int width = 0;
			int height = 0;
			m_pFont->CalculateStringSize( "TEST", width, height );

			if ( m_lineSpacer != 0 )
			{
				height += m_lineSpacer;
			}
			else
			{
				if ( !gUIShell.GetLineSpacer( GetName() ) )
				{
					height += gUIShell.GetLineSpacer( "default" );
				}
				else
				{
					height += gUIShell.GetLineSpacer( GetName() );
				}
			}

			SetFontHeight( height );

			AdjustChildren();
		}
	}
}


void UIChatBox::SetMinimized( bool bSet )
{	
	if ( bSet )
	{
		SetLeadElement( GetNumElements() );			
	}
	
	m_lastEntryTime = 0.0f;
	m_bMinimized = bSet;
}