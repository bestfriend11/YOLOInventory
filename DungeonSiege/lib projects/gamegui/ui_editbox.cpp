/**************************************************************************

Filename		: ui_editbox.cpp

Description		: Control for the edit box

Creation Date	: 1/11/00

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "appmodule.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_editbox.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"
#include "keys.h"
#include "ui_interface.h"
#include "LocHelp.h"
#include "imeui.h"
#include "ui_text.h"


/**************************************************************************

Function		: UIEditBox()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIEditBox::UIEditBox( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
	: UIWindow( fhWindow, parent_interface, parent )
	, m_buttondown( false )
	, m_bAllowInput( false )
	, m_bGiveFocus( false )
	, m_bShowHiddenText( false )
	, m_selectionColor( 0xFF0000FF )
	, m_pFont( NULL )
	, m_max_size( 0 )
	, m_max_pixels( 0 )
	, m_bPixelLimit( false )
	, m_font_size( 0 )
	, m_prompt_texture( 0 )
	, m_prompt_width( 0 )
	, m_prompt_index( 0 )
	, m_sel_start( 0 )
	, m_sel_end( 0 )
	, m_bClearSelect( false )
	, m_bPermanentFocus( false )
	, m_bRestricted( false )
	, m_asciiLowBound( 0 )
	, m_asciiHighBound( 0 )
	, m_tabStop( -1 )
	, m_promptFlash( 0.5f )
	, m_promptFlashElapsed( 0.0f )	
	, m_bDrawPrompt( true )		
	, m_bImeToggle( true )
	, m_bAllowIme( true )
	, m_dwToggleIme( false )				
	, m_ImeX( 0 )
	, m_ImeY( 0 )
	, m_bImeEnabled( false )
	, m_imeColor( 0xFFFFFFFF )
	, m_justification( JUSTIFY_LEFT )
{
	SetType( UI_TYPE_EDITBOX );
	SetInputType( TYPE_INPUT_ALL );
	SetColor( 0xFFFFFFFF );	

	gpstring sFont;
	fhWindow.Get( "font_color", m_color );
	fhWindow.Get( "font_size", m_font_size );
	if ( fhWindow.Get( "font_type", sFont ) ) 
	{
		m_pFont = gUITextureManager.LoadFont( sFont, true );
		if ( !m_pFont )
		{
			DWORD dwBgColor = 0;
			if ( fhWindow.Get( "font_bg_color", dwBgColor ) )
			{
				m_pFont = gUITextureManager.LoadFont( sFont, m_font_size, dwBgColor );
			}
			else
			{
				m_pFont = gUITextureManager.LoadFont( sFont, m_font_size );
			}
		}

		if ( m_pFont )
		{
			int width = 0;
			m_pFont->CalculateStringSize( "TEST", width, m_font_size );			
		}
	}
	fhWindow.Get( "max_string_size", m_max_size );
	if ( fhWindow.Get( "text", m_text ) && LocMgr::DoesSingletonExist() )
	{
		gLocMgr.TranslateUiText( m_text, GetInterfaceParent().c_str(), GetName().c_str() );
		m_pFont->RegisterString( m_text );
	}

	if ( m_max_size == 0 )
	{
		m_max_size = m_text.size();
	}

	m_prompt_index = m_text.size();

	FastFuelHandle hPrompt = fhWindow.GetChildNamed( "prompt" );
	if ( hPrompt.IsValid() )
	{
		gpstring sTexture;
		if ( hPrompt.Get( "texture",  sTexture ) )
		{
			m_prompt_texture = gUITextureManager.LoadTexture( sTexture );
		}
		m_prompt_width = 4;
		hPrompt.Get( "width", m_prompt_width );
	}

	fhWindow.Get( "hidden_text", m_bShowHiddenText );
	m_bAllowIme = !m_bShowHiddenText;

	fhWindow.Get( "clear_select", m_bClearSelect );

	fhWindow.Get( "permanent_focus", m_bPermanentFocus );

	if ( m_bImeEnabled )
	{
		ImeUi_EnableIme( false );
		m_bImeEnabled = false;
	}

	if ( m_bPermanentFocus )
	{
		SetAllowInput( true );
	}

	fhWindow.Get( "restrict_input", m_bRestricted );
	if ( m_bRestricted )
	{
		fhWindow.Get( "ascii_low_bound", m_asciiLowBound );
		fhWindow.Get( "ascii_high_bound", m_asciiHighBound );
	}

	fhWindow.Get( "excluded_chars", m_ExcludedChars );

	fhWindow.Get( "allowed_chars", m_AllowedChars );

	{
		UI_EVENT ue;
		ue.msg_code = MSG_CHAR;
		EVENT_ACTION ea;
		ea.action = ACTION_CONSUMEINPUT;		
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );
	}
	{
		UI_EVENT ue;
		ue.msg_code = MSG_KEYPRESS;
		EVENT_ACTION ea;
		ea.action = ACTION_CONSUMEINPUT;
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );
	}
	{
		UI_EVENT ue;
		ue.msg_code = MSG_ENTER;
		EVENT_ACTION ea;
		ea.action = ACTION_EDITSELECT;		
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );
	}

	fhWindow.Get( "prompt_flash", m_promptFlash );	
	fhWindow.Get( "tab_stop", m_tabStop );
	fhWindow.Get( "max_pixels", m_max_pixels );
	fhWindow.Get( "has_pixel_limit", m_bPixelLimit );
	fhWindow.Get( "allow_ime", m_bAllowIme );

	gpstring sJustification;
	if ( fhWindow.Get( "justification", sJustification ) )
	{
		if ( sJustification.same_no_case( "right" ) )
		{
			m_justification = JUSTIFY_RIGHT;
		}
		else if ( sJustification.same_no_case( "center" ) ) 
		{
			m_justification = JUSTIFY_CENTER;
		}
	}
}
UIEditBox::UIEditBox()
	: UIWindow()
	, m_buttondown( false )
	, m_bAllowInput( false )
	, m_bGiveFocus( false )
	, m_bShowHiddenText( false )
	, m_selectionColor( 0xFF0000FF )
	, m_pFont( NULL )
	, m_max_size( 0 )
	, m_max_pixels( 0 )
	, m_bPixelLimit( false )
	, m_font_size( 0 )
	, m_prompt_texture( 0 )
	, m_prompt_width( 0 )
	, m_prompt_index( 0 )
	, m_sel_start( 0 )
	, m_sel_end( 0 )
	, m_bClearSelect( false )
	, m_bPermanentFocus( false )
	, m_bRestricted( false )
	, m_asciiLowBound( 0 )
	, m_asciiHighBound( 0 )
	, m_tabStop( -1 )
	, m_promptFlash( 0.5f )
	, m_promptFlashElapsed( 0.0f )	
	, m_bDrawPrompt( true )		
	, m_bImeToggle( true )
	, m_bAllowIme( true )
	, m_dwToggleIme( false )				
	, m_ImeX( 0 )
	, m_ImeY( 0 )
	, m_bImeEnabled( false )
	, m_imeColor( 0xFFFFFFFF )
{
	SetType( UI_TYPE_EDITBOX );
	SetInputType( TYPE_INPUT_ALL );
	SetColor( 0xFFFFFFFF );	
}


/**************************************************************************

Function		: ~UIEditBox()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIEditBox::~UIEditBox()
{
	if ( m_prompt_texture != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_prompt_texture );
	}
	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && m_pFont )
	{
		if ( m_bImeEnabled )
		{
			ImeUi_EnableIme( false );
		}
		m_pFont->UnregisterString( m_text );
	}
}


void UIEditBox::Update( double seconds )
{
	if ( m_bGiveFocus )
	{
		if ( !GetAllowInput() )
		{
			m_promptFlashElapsed = m_promptFlash;
			m_bDrawPrompt = true;
		}
		SetAllowInput( true );
		m_bGiveFocus = false;
	}

	if ( m_bPermanentFocus && GetVisible() )
	{
		if ( !m_bImeEnabled && LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{			
			SetAllowInput( true );
		}
	}

	if ( !m_bDrawPrompt )
	{
		m_promptFlashElapsed += (float)seconds;
		if ( m_promptFlashElapsed >= m_promptFlash )
		{
			m_bDrawPrompt = true;
		}
	}
	else
	{
		m_promptFlashElapsed -= (float)seconds;
		if ( m_promptFlashElapsed <= 0.0f )
		{
			m_bDrawPrompt = false;
		}
	}

	if ( m_bImeEnabled && !ImeUi_IsEnabled() && m_bAllowIme )
	{
		ImeUi_EnableIme( true );
		ImeUi_SetCompStringAppearance( (void *)m_pFont, GetColor(), &(GetRect()) );			
	}
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UIEditBox::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true )
	{
		return true;
	}
	else if ( action == ACTION_CONSUMEINPUT )
	{
		GUInterface * pInterface = gUIShell.FindInterface( GetInterfaceParent() );		
		if ( GetVisible() && GetAllowInput() && pInterface->bVisible )
		{
			return true;
		}
	}
	else if ( action == ACTION_EDITSELECT )
	{
		GUInterface * pInterface = gUIShell.FindInterface( GetInterfaceParent() );		
		if ( GetVisible() && GetAllowInput() && pInterface->bVisible )
		{
			gUIMessenger.SendUIMessage( UIMessage( MSG_EDITSELECT ), this );
			return true;
		}
	}
	return false;
}


/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: void

**************************************************************************/
bool UIEditBox::ProcessMessage( UIMessage & msg )
{
	UIWindow::ProcessMessage( msg );

	int width	= 0;
	int height	= 0;
	if ( m_pFont )
	{
		m_pFont->CalculateStringSize( m_text.c_str(), width, height );
	}

	switch ( msg.GetCode() )
	{
	case MSG_HIDE:
		{
			SetAllowInput( false );			
		}
		break;
	case MSG_SHOW:
		{
			if ( GetPermanentFocus() )
			{
				SetAllowInput( true );
			}
			else
			{
				GUInterface * pInterface = gUIShell.FindInterface( GetInterfaceParent() );
				if ( pInterface->tabStops.size() > 0 )
				{
					if ( pInterface->tabStops.begin()->second == this )
					{
						GiveFocus();
					}
					else
					{
						SetAllowInput( false );
					}
				}
			}
		}
		break;
	case MSG_LBUTTONDOWN:
		{
			m_buttondown = true;
			DetermineCaretPosition();
			GiveFocus();
		}
		break;
	case MSG_GLOBALLBUTTONDOWN:
		{			
			if ( !GetPermanentFocus() && !HitDetect( gUIShell.GetMouseX(), gUIShell.GetMouseY() ) )
			{
				m_sel_end = m_sel_start;
				SetAllowInput( false );				
			}
		}
		break;
	case MSG_GLOBALLBUTTONUP:
		{
			m_buttondown = false;
		}
		break;
	case MSG_EDITSELECT:
		{	
			if ( !GetAllowInput() )
			{
				return false;
			}
		}
		break;
	case MSG_KEYPRESS:
		{
			if ( GetAllowInput() && GetVisible() && IsEnabled() && gUIShell.IsInterfaceVisible( GetInterfaceParent() ) )
			{
				switch ( msg.GetKey() )
				{
				// Clipboard Copy
				case Keys::KEY_INT_C:
					{
						if ( gAppModule.GetControlKey() )
						{
						}
					}
					break;

				// Clipboard Cut
				case Keys::KEY_INT_X:
					{
						if ( gAppModule.GetControlKey() )
						{
						}
					}
					break;

				// Clipboard paste
				case Keys::KEY_INT_V:
					{
						if ( gAppModule.GetControlKey() )
						{
							if ( ::OpenClipboard( gAppModule.GetMainWnd() ) )
							{
								HANDLE hClipboardText = GetClipboardData( CF_TEXT );
								if ( hClipboardText != NULL )
								{
									LPSTR lpstr = (LPSTR)GlobalLock( hClipboardText );
									SetText( ToUnicode( lpstr ) );
			                        GlobalUnlock( hClipboardText );
									CloseClipboard();
									
									// if we have a max length set in the gas file
									// make sure to enforce it for pasting as well as typing!
									if( GetText().length() > (unsigned int) m_max_size )
									{
										SetText( GetText().left_safe( m_max_size ) );
									}
								}
							}
						}
					}
					break;

				case Keys::KEY_INT_LEFT:
					{
						if ( m_prompt_index != 0 )
						{
							SetPromptIndex( GetPromptIndex() - 1 );
						}
					}
					break;

				case Keys::KEY_INT_RIGHT:
					{
						if ( m_prompt_index != (int)m_text.size() )						
						{
							SetPromptIndex( GetPromptIndex() + 1 );							
						}
					}
					break;

				case Keys::KEY_INT_DELETE:
					{
						if ( (m_prompt_index+1) <= (int)m_text.size() ) 
						{
							gpwstring sNew;
							sNew = m_text.substr( 0, m_prompt_index );
							sNew.append( m_text.substr( m_prompt_index+1, m_text.size() ) );
							if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
							{
								m_pFont->UnregisterString( m_text );
								m_pFont->RegisterString( sNew );
							}
							m_text = sNew;
						}
					}
					break;
					
				case Keys::KEY_INT_ESCAPE:
					{
						gUIMessenger.SendUIMessage( UIMessage( MSG_EDITESCAPE ), this );
					}
					return false;

				case Keys::KEY_INT_BACK:
					{
						if ( m_text.size() != 0 )
						{
							if ( m_prompt_index > 0 )
							{
								gpwstring sNew;
								sNew = m_text.substr( 0, m_prompt_index-1 );
								sNew.append( m_text.substr( m_prompt_index, m_text.size() ) );
								if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
								{
									m_pFont->UnregisterString( m_text );
									m_pFont->RegisterString( sNew );
								}
								m_text = sNew;
								SetPromptIndex( GetPromptIndex() - 1 );							
							}
						}
					}
					break;

				case Keys::KEY_INT_HOME:
					{
						SetPromptIndex( 0 );
					}
					break;

				case Keys::KEY_INT_END:
					{
						SetPromptIndex( m_text.size() );
					}
					break;

				case Keys::KEY_INT_TAB:
					{
						GUInterface * pInterface = gUIShell.FindInterface( GetInterfaceParent() );

						if ( gAppModule.GetShiftKey() )
						{
							for ( UITabStopMap::reverse_iterator findStop = pInterface->tabStops.rbegin(); findStop != pInterface->tabStops.rend(); ++findStop )
							{
								if ( findStop->second == this )
								{
									do
									{
										++findStop;
										if ( findStop == pInterface->tabStops.rend() )
										{
											findStop = pInterface->tabStops.rbegin();
										}
										if ( findStop->second->IsEnabled() && findStop->second->GetVisible() )
										{
											SetAllowInput( false );
											((UIEditBox *)findStop->second)->GiveFocus();
											break;
										}
									} while ( findStop->second != this );

									break;
								}
							}
						}
						else
						{
							for ( UITabStopMap::iterator findStop = pInterface->tabStops.begin(); findStop != pInterface->tabStops.end(); ++findStop )
							{
								if ( findStop->second == this )
								{
									do
									{
										++findStop;
										if ( findStop == pInterface->tabStops.end() )
										{
											findStop = pInterface->tabStops.begin();
										}
										if ( findStop->second->IsEnabled() && findStop->second->GetVisible() )
										{
											SetAllowInput( false );
											((UIEditBox *)findStop->second)->GiveFocus();
											break;
										}
									} while ( findStop->second != this );

									break;
								}
							}
						}
					}
					break;
				}
			}
		}
		break;
	case MSG_CHAR:
		{
			if ( GetAllowInput() && GetVisible() && IsEnabled() ) 
			{			
				bool bOverflow = false;
				if ( (m_text.size() < (unsigned int)m_max_size) || m_bPixelLimit ) 
				{
					unsigned int size = m_text.size();
					wchar_t char_key = (wchar_t)msg.GetKey();
					if ( char_key == 0 ) 
					{
						break;
					}

					if ( m_bRestricted )
					{
						if (( char_key < m_asciiLowBound ) || ( char_key > m_asciiHighBound ))
						{
							break;
						}
					}

					if ( !m_ExcludedChars.empty() && (m_ExcludedChars.find( char_key ) != gpwstring::npos) )
					{
						return true;
					}

					if ( !m_AllowedChars.empty() && (m_AllowedChars.find( char_key ) == gpwstring::npos) )
					{
						return true;
					}

					if ( m_sel_start != m_sel_end )
					{				
						if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
						{
							m_pFont->UnregisterString( m_text );						
						}
						m_text = m_text.left( m_sel_start ) + m_text.right( m_text.size() - m_sel_end );
						if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
						{
							m_pFont->RegisterString( m_text );						
						}
						m_sel_end = m_sel_start;
						m_prompt_index = m_sel_start;
					}

					if ( m_bPixelLimit )
					{
						int charWidth	= 0;
						int charHeight	= 0;
						if ( m_pFont )
						{
							gpwstring sTest = m_text;
							sTest.insertf( m_prompt_index, L"%c", char_key );							
							m_pFont->CalculateStringSize( sTest.c_str(), charWidth, charHeight );
							if ( charWidth > m_max_pixels )
							{
								bOverflow = true;
								goto CheckOverFlow;
							}
						}
					}
					
					if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
					{
						m_pFont->UnregisterString( m_text );						
					}
					m_text.insertf( m_prompt_index, L"%c", char_key );
					if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
					{
						m_pFont->RegisterString( m_text );						
					}

					if ( size != m_text.size() ) 
					{
						SetPromptIndex( GetPromptIndex() + 1 );
					}
				}
				else 
					bOverflow = true;
	CheckOverFlow:
				if ( bOverflow && LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
				{					
					ImeUI_CancelString();					
				}				
			}
		}
		break;
	}

	return true;
}



/**************************************************************************

Function		: Draw()

Purpose			: Draw the window

Returns			: void

**************************************************************************/
void UIEditBox::Draw()
{
	UIWindow::Draw();
	if ( GetVisible() == true ) 
	{
		if ( GetParentWindow() ) 
		{
			if ( GetParentWindow()->GetVisible() == false ) 
			{
				return;
			}
		}

		if ( m_pFont )
		{
			int width		= 0;
			int height		= 0;

			gpwstring showText;
			if ( m_bShowHiddenText )
			{
				showText.assign( m_text.size(), '*' );
			}
			else
			{
				showText = m_text;
			}

			gpwstring sText	= showText;
			m_pFont->CalculateStringSize( showText.c_str(), width, height );
			while ( width > ( GetRect().right-GetRect().left ) )
			{				
				if ( m_prompt_index <= (int)( showText.size() - sText.size()) )
				{
					sText.erase( sText.end_split()-1 );
				}
				else 
				{
					sText.erase( sText.begin_split() );
				}
				m_pFont->CalculateStringSize( sText.c_str(), width, height );								
			}

			m_sDrawnText = sText;
			if ( height == 0 )
			{
				height = m_font_size;
			}
			int y = GetRect().top+(((GetRect().bottom-GetRect().top)/2)-(height/2));

			if ( (m_sel_start != m_sel_end) &&
				 (m_sel_start < (int)sText.size()) )
			{
				int endSelPos = m_sel_end;

				if ( endSelPos > (int)sText.size() )
				{
					endSelPos = sText.size();
				}

				UIRect selRect;

				m_pFont->CalculateStringSize( sText.left( m_sel_start ), width, height );
				selRect.left = GetRect().left + width;
				
				m_pFont->CalculateStringSize( sText.mid( m_sel_start, endSelPos - m_sel_start ), width, height );
				selRect.right = selRect.left + width;

				selRect.top = y;
				selRect.bottom = y + height;

				gUIShell.DrawColorBox( selRect, m_selectionColor );
			}

			int left = GetRect().left;
			if ( !GetAllowInput() )
			{
				switch ( GetJustification() )
				{
				case JUSTIFY_CENTER:
					left = GetRect().left + ((GetRect().right-GetRect().left)/2 - (width/2));
					break;
				case JUSTIFY_RIGHT:
					left = GetRect().right - width;
					break;
				}
			}

			m_pFont->Print( left, y, sText.c_str(), GetColor(), true );
			if ( GetAllowInput() && IsEnabled() ) 
			{
				UIRect prompt;
				 
				DrawPromptTexture( sText, y );
			}
		}
	}
}


void UIEditBox::DetermineCaretPosition()
{
	int x = gUIShell.GetMouseX();

	int closest = 100000;
	int pos = 0;

	for ( int i = 0; i != (int)m_text.size(); ++i )
	{
		gpwstring sText = m_text.substr( 0, i );
		int width = 0;
		int height = 0;
		m_pFont->CalculateStringSize( sText, width, height );

		if ( (abs(x - (GetRect().left+width))) < closest )
		{
			closest = abs(x - (GetRect().left+width));
			if ( (x - (GetRect().left+width)) > 0 )
			{
				pos = i+1;
			}
			else
			{
				pos = i;
			}
		}
	}	

	SetPromptIndex( pos );
}


void UIEditBox::DrawText( const wchar_t * szText )
{
	m_pFont->Print( m_ImeX, m_ImeY, szText, GetImeColor(), true );
}


void UIEditBox::DrawPromptTexture( gpwstring sText, int startY )
{
	int size_dif = 0;
	size_dif = m_text.size() - sText.size();
	
	int width	= 0;
	int height	= 0;
	if ( m_pFont ) 
	{
		gpwstring prompt_text;
		int dif = 0;
		if ( size_dif <= m_prompt_index )
		{
			dif = m_prompt_index-size_dif;
		}
		prompt_text.copy( (unsigned short *)sText.c_str(), dif );
		prompt_text = sText.substr( 0, dif );

		if ( prompt_text.empty() )
		{
			prompt_text = L"A";
			m_pFont->CalculateStringSize( prompt_text.c_str(), width, height );
			width = 0;
		}
		else
		{
			m_pFont->CalculateStringSize( prompt_text.c_str(), width, height );
		}
	}		

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{		
		ImeUi_SetCaretPosition( GetRect().left+width, GetRect().top+(((GetRect().bottom-GetRect().top)/2)-(height/2)) );
	}

	if ( !m_bDrawPrompt )
	{
		return;
	}

	if ( GetVisible() == true ) 
	{
		gUIShell.GetRenderer().SetTextureStageState(	0,
														m_prompt_texture != 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE,
														D3DTOP_MODULATE,
														D3DTADDRESS_CLAMP,	// WRAP
														D3DTADDRESS_CLAMP,
														D3DTEXF_POINT,		// LINEAR
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
		float prompt_alpha = 1.0f;

		Verts[0].x			= GetRect().left+width-0.5f;
		Verts[0].y			= startY-0.5f;
		Verts[0].z			= 0.0f;
		Verts[0].rhw		= 1.0f;
		Verts[0].uv.u		= 0.0f;
		Verts[0].uv.v		= 1.0f;
		Verts[0].color		= (color | (((BYTE)((prompt_alpha)*255.0f))<<24));

		Verts[1].x			= GetRect().left+width-0.5f;
		Verts[1].y			= (startY+height)-0.5f;
		Verts[1].z			= 0.0f;
		Verts[1].rhw		= 1.0f;
		Verts[1].uv.u		= 0.0f;
		Verts[1].uv.v		= 0.0f;
		Verts[1].color		= (color | (((BYTE)((prompt_alpha)*255.0f))<<24));

		Verts[2].x			= GetRect().left+width+m_prompt_width-0.5f;
		Verts[2].y			= startY-0.5f;
		Verts[2].z			= 0.0f;
		Verts[2].rhw		= 1.0f;
		Verts[2].uv.u		= 1.0f;
		Verts[2].uv.v		= 1.0f;
		Verts[2].color		= (color | (((BYTE)((prompt_alpha)*255.0f))<<24));

		Verts[3].x			= GetRect().left+width+m_prompt_width-0.5f;
		Verts[3].y			= (startY+height)-0.5f;
		Verts[3].z			= 0.0f;
		Verts[3].rhw		= 1.0f;
		Verts[3].uv.u		= 1.0f;
		Verts[3].uv.v		= 0.0f;
		Verts[3].color		= (color | (((BYTE)((prompt_alpha)*255.0f))<<24));

		if ( !LocMgr::DoesSingletonExist() || (LocMgr::DoesSingletonExist() && gLocMgr.IsAnsi()) || 
			 (LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && ImeUi_GetCaretStatus()) )
		{		
			gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_prompt_texture, 1 );	
		}
	}
}


void UIEditBox::DrawRect( UIRect rect, DWORD dwColor )
{	
	D3DRECT d3d_rect;
	d3d_rect.x1		= rect.left;
	d3d_rect.y1		= rect.top;
	d3d_rect.x2		= rect.right;
	d3d_rect.y2		= rect.bottom;	

	gUIShell.GetRenderer().SetTextureStageState(	0,
													D3DTOP_SELECTARG2,
													D3DTOP_SELECTARG2,
													D3DTADDRESS_WRAP,
													D3DTADDRESS_WRAP,
													D3DTEXF_LINEAR,
													D3DTEXF_LINEAR,
													D3DTEXF_LINEAR,
													D3DBLEND_SRCALPHA,
													D3DBLEND_INVSRCALPHA,
													false );


	tVertex LineVerts[4];
	memset( LineVerts, 0, sizeof( tVertex ) * 4 );
			
	LineVerts[0].x			= rect.left-0.5f;
	LineVerts[0].y			= rect.top-0.5f;
	LineVerts[0].z			= 0.0f;
	LineVerts[0].rhw		= 1.0f;
	LineVerts[0].color		= dwColor;

	LineVerts[1].x			= rect.left-0.5f;
	LineVerts[1].y			= rect.bottom-0.5f;
	LineVerts[1].z			= 0.0f;
	LineVerts[1].rhw		= 1.0f;
	LineVerts[1].color		= dwColor;

	LineVerts[2].x			= rect.right-0.5f;
	LineVerts[2].y			= rect.top-0.5f;
	LineVerts[2].z			= 0.0f;
	LineVerts[2].rhw		= 1.0f;
	LineVerts[2].color		= dwColor;

	LineVerts[3].x			= rect.right-0.5f;
	LineVerts[3].y			= rect.bottom-0.5f;
	LineVerts[3].z			= 0.0f;
	LineVerts[3].rhw		= 1.0f;
	LineVerts[3].color		= dwColor;

	gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, LineVerts, 4, TVERTEX, 0, 0 );	
}


void UIEditBox::DrawFans( IMEUI_VERTEX* paVertex, unsigned int num )
{
	gUIShell.GetRenderer().SetTextureStageState(	0,
													D3DTOP_SELECTARG2,
													D3DTOP_SELECTARG2,
													D3DTADDRESS_WRAP,
													D3DTADDRESS_WRAP,
													D3DTEXF_LINEAR,
													D3DTEXF_LINEAR,
													D3DTEXF_LINEAR,
													D3DBLEND_SRCALPHA,
													D3DBLEND_INVSRCALPHA,
													false );

	UNREFERENCED_PARAMETER( paVertex );
	UNREFERENCED_PARAMETER( num );
	
//	gUIShell.GetRenderer().GetDevice()->DrawPrimitive( D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, paVertex, num, 0 );
}



void UIEditBox::SetAllowInput( bool bInput )
{
	if ( m_bAllowInput == bInput )
	{
		return;
	}

	m_bAllowInput = bInput;
	
	if ( m_bAllowInput )
	{
		gUIMessenger.SendUIMessage( UIMessage( MSG_EDITHASFOCUS ), this );
	}
	else
	{
		gUIMessenger.SendUIMessage( UIMessage( MSG_EDITLOSTFOCUS ), this );
	}

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && m_bAllowIme )
	{
		if ( m_bAllowInput )
		{
			ImeUi_EnableIme( true );
			m_bImeEnabled = true;
			ImeUi_SetCompStringAppearance( (void *)m_pFont, GetColor(), &(GetRect()) );			
			gpwstring sText = m_sDrawnText;
			
			int width	= 0;
			int height	= 0;
			int size_dif = 0;
			size_dif = m_text.size() - sText.size();			
			if ( m_pFont ) 
			{
				gpwstring prompt_text;
				int dif = 0;
				if ( size_dif <= m_prompt_index )
				{
					dif = m_prompt_index-size_dif;
				}
				prompt_text.copy( (unsigned short *)sText.c_str(), dif );
				prompt_text = sText.substr( 0, dif );
				m_pFont->CalculateStringSize( prompt_text.c_str(), width, height );			
			}	

			ImeUi_SetCaretPosition( GetRect().left+width, GetRect().top+(((GetRect().bottom-GetRect().top)/2)-(height/2)) );

			ImeUi_SetState( m_dwToggleIme );			
		}
		else
		{
			m_dwToggleIme = ImeUi_GetState();			
			ImeUI_CancelString();				
			
			if ( m_bImeEnabled )
			{
				ImeUi_EnableIme( false );
				m_bImeEnabled = false;
			}						
		}
	}
}


void UIEditBox::EnableIme( bool bSet )
{
	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		m_bAllowIme = bSet;

		m_dwToggleIme = ImeUi_GetState();
		ImeUi_FinalizeString();				

		if ( m_bImeEnabled )
		{
			ImeUi_EnableIme( false );
			m_bImeEnabled = false;
		}	
	}
}

void UIEditBox::SetPromptIndex( int index )
{
	// $$$ this is for highlight selection support - work in progress.
	/*	
	if ( GetVisible() && GetAllowInput() && gAppModule.GetShiftKey() )
	{
		if ( index > m_sel_end )
		{
			if ( m_prompt_index == m_sel_start )
			{
				m_sel_start = m_sel_end;
			}
			m_sel_end = index;
		}
		else if ( index < m_sel_start )
		{
			if ( m_prompt_index == m_sel_end )
			{
				m_sel_end = m_sel_start;
			}
			m_sel_start = index;
		}
		else
		{
			if ( m_prompt_index == m_sel_start )
			{
				m_sel_start = index;
			}
			if ( m_prompt_index == m_sel_end )
			{
				m_sel_end = index;
			}
		}
		m_prompt_index = index;
	}
	else*/
	{
		m_prompt_index = index;
		m_sel_start = index;
		m_sel_end = index;
	}
}


void UIEditBox::SetFontAttributes( unsigned int height, bool bBold )
{
	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		m_pFont = gUITextureManager.ChangeFontAttributes( m_pFont, height, bBold );
	}
}


#if !GP_RETAIL
void UIEditBox::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "edit_box" );
			hWindow->Set( "font_color", m_color );
			hWindow->Set( "font_size", m_font_size );
			
			if ( m_pFont )
			{
				hWindow->Set( "font_type", m_pFont->GetTextureName() );
			}

			hWindow->Set( "max_string_size", m_max_size );
			hWindow->Set( "text", m_text );

			FuelHandle hPrompt = hWindow->CreateChildBlock( "prompt" );
			gpstring sPath = gUIShell.GetRenderer().GetTexturePathname( m_prompt_index );
			gpstring sName = gUIShell.GetTextureNamingKeyName( sPath );
			hPrompt->Set( "texture", sName );			
			hPrompt->Set( "width", m_prompt_width );

			hWindow->Set( "clear_select", m_bClearSelect );
			hWindow->Set( "permanent_focus", m_bPermanentFocus );
		}
	}
}
#endif


void UIEditBox::SetText( gpwstring text )	
{ 
	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		m_pFont->UnregisterString( m_text );
		m_pFont->RegisterString( text );
	}

	m_text = text; 
	m_prompt_index = text.size(); 
}


bool UIEditBox::DoesTextFit( gpwstring & sText )
{
	if ( m_pFont && m_bPixelLimit && m_max_pixels != 0 )
	{
		int width = 0;
		int height = 0;
		m_pFont->CalculateStringSize( sText.c_str(), width, height );
		if ( width > m_max_pixels )
		{
			return false;
		}
	}

	return true;
}


void UIEditBox::SetVisible( bool bVisible )
{
	UIWindow::SetVisible( bVisible );
	if ( !bVisible && LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		ImeUi_EnableIme( false );
		m_bImeEnabled = false;
	}
	else if ( bVisible && LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && m_bAllowIme )
	{
		ImeUi_EnableIme( true );
		m_bImeEnabled = true;
		ImeUi_SetCompStringAppearance( (void *)m_pFont, GetColor(), &(GetRect()) );	
	}
}


void UIEditBox::SetRect( int left, int right, int top, int bottom, bool bAnimation  )
{
	UIWindow::SetRect( left, right, top, bottom, bAnimation );
	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		ImeUi_SetCompStringAppearance( (void *)m_pFont, GetColor(), &(GetRect()) );	
	}
}


void UIEditBox::ResizeToCurrentResolution( int original_width, int original_height )
{
	UIWindow::ResizeToCurrentResolution( original_width, original_height );
	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		ImeUi_SetCompStringAppearance( (void *)m_pFont, GetColor(), &(GetRect()) );	
	}
}
