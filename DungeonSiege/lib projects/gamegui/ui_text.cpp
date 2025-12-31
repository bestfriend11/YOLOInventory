/**************************************************************************

Filename		: ui_text.cpp

Description		: User interface control for displaying text

Creation Date	: 10/4/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_emotes.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_text.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"
#include "ui_textureman.h"
#include "LocHelp.h"
#include "AppModule.h"


FUBI_EXPORT_ENUM( JUSTIFICATION, JUSTIFY_BEGIN, JUSTIFY_COUNT );

static const char* s_JUSTIFICATION_Strings[] =
{
	"justify_left",
	"justify_right",
	"justify_center",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_JUSTIFICATION_Strings ) == JUSTIFY_COUNT );

static stringtool::EnumStringConverter s_JUSTIFICATION_Converter( s_JUSTIFICATION_Strings, JUSTIFY_BEGIN, JUSTIFY_END );

const char* ToString( JUSTIFICATION e )
	{  return ( s_JUSTIFICATION_Converter.ToString( e ) );  }
bool FromString( const char* str, JUSTIFICATION& e )
	{  return ( s_JUSTIFICATION_Converter.FromString( str, rcast <int&> ( e ) ) );  }


/**************************************************************************

Function		: UIText()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIText::UIText(	FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: UIWindow( fhWindow, parent_interface, parent )
, m_pFont( NULL )
, m_size( 0 )
, m_justification( JUSTIFY_LEFT )
, m_bAutoSize( false )
, m_lbuttondown( false )
, m_rbuttondown( false )
, m_bTruncateText( false )
, m_ParentOffset( 0 )
{	
	SetType( UI_TYPE_TEXT );
	SetInputType( TYPE_INPUT_MOUSE );

	SetColor( 0xFFFFFFFF );	
	gpstring sFont;
	fhWindow.Get( "font_color", m_color );
	fhWindow.Get( "font_size", m_size );
	if ( fhWindow.Get( "font_type", sFont ) ) {
		m_pFont = gUITextureManager.LoadFont( sFont, true );
		if ( !m_pFont ) 
		{
			DWORD dwBgColor = 0;
			if ( fhWindow.Get( "font_bg_color", dwBgColor ) )
			{
				m_pFont = gUITextureManager.LoadFont( sFont, m_size, dwBgColor );
			}
			else
			{
				m_pFont = gUITextureManager.LoadFont( sFont, m_size );
			}
		}		
	}	

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

	if ( !m_pFont ) 
	{		
		gperrorf(( "Using default system font for GUI element: %s, change it!", GetName().c_str() ));
		m_pFont = gUITextureManager.GetDefaultFont();
	}

	if ( fhWindow.Get( "text", m_text ) && LocMgr::DoesSingletonExist() )
	{
		gLocMgr.TranslateUiText( m_text, GetInterfaceParent().c_str(), GetName().c_str() );
		m_pFont->RegisterString( m_text );								
	}
	
	fhWindow.Get( "truncate", m_bTruncateText );

	if ( parent && (parent->GetType() == UI_TYPE_BUTTON) )
	{
		m_ParentOffset = 16;
	}
	fhWindow.Get( "parent_offset", m_ParentOffset );

	gpstring justify;
	fhWindow.Get( "justify", justify );
	int width	= 0;
	int height	= 0;
	if ( m_pFont ) 
	{
		gUIEmotes.CalculateStringSize( m_pFont, m_text.c_str(), width, height );
	}
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

	AdjustChildren();

	if ( GetParentWindow() ) 
	{
		int x = 0;
		int y = 0;
		if ( fhWindow.Get( "child_x", x ) && fhWindow.Get( "child_y", y ) ) 
		{
			SetRect( GetParentWindow()->GetRect().left+x,
					 GetParentWindow()->GetRect().right,
					 GetParentWindow()->GetRect().top+y,
					 GetParentWindow()->GetRect().bottom );				
		}
	}
	SetPulloutOffset( GetRect().left );

	fhWindow.Get( "auto_size", m_bAutoSize );
	if ( m_bAutoSize )
	{
		SetText( m_text, true );
	}
}
UIText::UIText()
: UIWindow()
, m_pFont( 0 )
, m_size( 0 )
, m_bAutoSize( false )
, m_bTruncateText( false )
, m_lbuttondown( false )
, m_rbuttondown( false )
, m_ParentOffset( 0 )
{	
	SetType( UI_TYPE_TEXT );
	SetInputType( TYPE_INPUT_MOUSE );

	SetColor( 0xFFFFFFFF );	
	gpstring sFont;
	m_justification = JUSTIFY_LEFT;			
}


/**************************************************************************

Function		: ~UIText()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIText::~UIText()
{
	//if ( !gLocMgr.IsAnsi() && m_pFont )
	if ( m_pFont && m_pFont->GetHFont() )
	{
		if ( m_drawText.empty() )
		{
			m_pFont->UnregisterString( m_text );
		}
		else
		{
			m_pFont->UnregisterString( m_drawText );
		}
	}
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UIText::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true ) {
		return true;
	}
	else if ( action == ACTION_SETTEXT ) {		
		SetText( ToUnicode( parameter ) );
		return true;
	}
	else if ( action == ACTION_SETCOLOR ) {
		SetColor( atol( parameter ) );
		return true;
	}
	return false;
}


/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: bool

**************************************************************************/
bool UIText::ProcessMessage( UIMessage & msg )
{	
	UIWindow::ProcessMessage( msg );

	switch ( msg.GetCode() ) 
	{
		case MSG_LBUTTONDOWN:
		{
			m_lbuttondown = true;
			break;
		}

		case MSG_LBUTTONUP:
		{
			if ( m_lbuttondown )
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_BUTTONPRESS ), this );			

				if ( GetParentWindow() && (GetParentWindow()->GetType() == UI_TYPE_CHECKBOX) )
				{
					UICheckbox *pCheckbox = (UICheckbox *)GetParentWindow();
					pCheckbox->SetState( !pCheckbox->GetState() );
				}
			}
			m_lbuttondown = false;
			break;
		}

		case MSG_GLOBALLBUTTONUP:
		{
			m_lbuttondown = false;			
			break;
		}

		case MSG_RBUTTONDOWN:
		{
			m_rbuttondown = true;
			break;
		}

		case MSG_RBUTTONUP:
		{
			if ( m_rbuttondown )
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_RBUTTONPRESS ), this );			
			}
			m_rbuttondown = false;
			break;
		}

		case MSG_GLOBALRBUTTONUP:
		{
			m_rbuttondown = false;			
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
void UIText::Draw() 
{	
	UIWindow::Draw();

	if ( GetVisible() == true )
	{
		if ( GetParentWindow() && GetParentWindow()->GetVisible() == false )
		{
			return;
		}		
		if ( m_pFont )
		{
			gpwstring *pDrawText = &m_text;
			if ( !m_drawText.empty() )
			{
				pDrawText = &m_drawText;
			}

			if ( GetParentWindow() )
			{
				gUIEmotes.Draw( m_pFont, GetRect().left, GetRect().top, pDrawText->c_str(), GetColor(), (UIWindow*) this, true );
			}
			else
			{
				int x = 0;
				int y = GetRect().top;
				int width	= 0;
				int height	= 0;
				
				gUIEmotes.CalculateStringSize( m_pFont, pDrawText->c_str(), width, height );

				y = GetRect().top + ((GetRect().bottom-GetRect().top)/2 - (height/2));

				if ( m_justification == JUSTIFY_RIGHT )
				{
					x = GetRect().right - width;
				}
				else if ( m_justification == JUSTIFY_CENTER )
				{
					x = GetRect().left + ((GetRect().right-GetRect().left)/2 - (width/2));
				}

				if ( x < GetRect().left )
				{
					x = GetRect().left;
				}
				gUIEmotes.Draw( m_pFont, x, y, pDrawText->c_str(), GetColor(), (UIWindow*) this, true );
			}
		}
	}
}


// This function sets the scale of the text
void UIText::SetScale( float scale )
{
	UIWindow::SetScale( scale );
}


void UIText::SetFont( const gpstring& sFont )
{
	m_pFont = gUITextureManager.LoadFont( sFont, true );
	if ( !m_pFont )
	{		
		gperrorf(( "Using default system font for GUI element: %s, change it!", GetName().c_str() ));
		m_pFont = gUITextureManager.GetDefaultFont();
	}
}


void UIText::SetText( const gpwstring & text, bool bReset ) 
{
	if ( m_text.same_no_case( text ) && !bReset )
	{
		return;
	}

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && m_pFont )
	{
		if ( m_drawText.empty() )
		{
			m_pFont->UnregisterString( m_text );
		}
		else
		{
			m_pFont->UnregisterString( m_drawText );
		}
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
		gUIEmotes.CalculateStringSize( m_pFont, m_text.c_str(), width, height );
	}

	if ( m_bAutoSize )
	{
		UIWindow::SetRect( GetRect().left, GetRect().left + width, GetRect().top, GetRect().top + height );
	}

	int justifyWidth = width;

	if ( GetParentWindow() && (GetParentWindow()->GetType() != UI_TYPE_CHECKBOX) )
	{
		switch ( m_justification )
		{
			case JUSTIFY_LEFT:
			{
				if ( m_bTruncateText && (width + m_ParentOffset * 2 > GetParentWindow()->GetRect().Width()) )
				{
					justifyWidth = GetParentWindow()->GetRect().Width() - m_ParentOffset * 2;
				}
				SetPulloutOffset( GetParentWindow()->GetRect().left + m_ParentOffset );
				UIWindow::SetRect(	GetParentWindow()->GetRect().left + m_ParentOffset,
							GetParentWindow()->GetRect().left + justifyWidth + m_ParentOffset,
							(GetParentWindow()->GetRect().bottom - ((GetParentWindow()->GetRect().bottom-GetParentWindow()->GetRect().top)/2))-height/2, 
							GetParentWindow()->GetRect().bottom );
				break;
			}

			case JUSTIFY_RIGHT:
			{
				if ( m_bTruncateText && (width > GetParentWindow()->GetRect().Width()) )
				{
					justifyWidth = GetParentWindow()->GetRect().Width();
				}
				SetPulloutOffset( GetParentWindow()->GetRect().right - justifyWidth );
				UIWindow::SetRect(	GetParentWindow()->GetRect().right - justifyWidth, 
							GetParentWindow()->GetRect().right,
							(GetParentWindow()->GetRect().bottom - ((GetParentWindow()->GetRect().bottom-GetParentWindow()->GetRect().top)/2))-height/2, 
							GetParentWindow()->GetRect().bottom );
				break;
			}

			case JUSTIFY_CENTER:
			{
				if ( m_bTruncateText && (width > GetParentWindow()->GetRect().Width()) )
				{
					justifyWidth = GetParentWindow()->GetRect().Width() - m_ParentOffset * 2;
				}
				SetPulloutOffset( (GetParentWindow()->GetRect().right - ((GetParentWindow()->GetRect().right-GetParentWindow()->GetRect().left)/2)) - justifyWidth/2 );
				UIWindow::SetRect( (GetParentWindow()->GetRect().right - ((GetParentWindow()->GetRect().right-GetParentWindow()->GetRect().left)/2)) - justifyWidth/2, 
							GetParentWindow()->GetRect().right,
							(GetParentWindow()->GetRect().bottom - ((GetParentWindow()->GetRect().bottom-GetParentWindow()->GetRect().top)/2)) - height/2, 
							GetParentWindow()->GetRect().bottom );
				break;
			}
		}
	}

	if ( m_bTruncateText && ((width > GetRect().Width()) && (m_text.size() > 4)) )
	{
		m_drawText = m_text.left( m_text.size() - 2 ) + L"...";
		if ( m_pFont )
		{
			gUIEmotes.CalculateStringSize( m_pFont, m_drawText.c_str(), width, height );
		}

		while ( (width > GetRect().Width()) && (m_drawText.size() > 4) )
		{
			m_drawText = m_drawText.left( m_drawText.size() - 4 ) + L"...";
			if ( m_pFont )
			{
				gUIEmotes.CalculateStringSize( m_pFont, m_drawText.c_str(), width, height );
			}
		}

		SetToolTip( m_text, gUIShell.GetDefaultTooltipName(), true );
	}

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && m_pFont )
	{
		if ( m_drawText.empty() )
		{
			m_pFont->RegisterString( m_text );
		}
		else
		{
			m_pFont->RegisterString( m_drawText );
		}
	}
}


#if !GP_RETAIL
void UIText::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "text" );

			if ( m_pFont )
			{
				hWindow->Set( "font_type", m_pFont->GetTextureName() );
			}
			hWindow->Set( "font_color", m_color );
			hWindow->Set( "font_size", m_size );
			hWindow->Set( "text", m_text );

			switch ( m_justification )
			{
			case JUSTIFY_LEFT:
				hWindow->Set( "justify", "left" );
				break;
			case JUSTIFY_RIGHT:
				hWindow->Set( "justify", "right" );
				break;
			case JUSTIFY_CENTER:
				hWindow->Set( "justify", "center" );
				break;
			}
		}
	}
}
#endif


void UIText::AdjustChildren()
{
	int width	= 0;
	int height	= 0;
	if ( m_pFont )
	{
		gUIEmotes.CalculateStringSize( m_pFont, m_text.c_str(), width, height );
	}

	int justifyWidth = width;

	if ( GetParentWindow() && (GetParentWindow()->GetType() != UI_TYPE_CHECKBOX) )
	{
		switch ( m_justification )
		{
			case JUSTIFY_LEFT:
			{
				if ( m_bTruncateText && (width + m_ParentOffset * 2 > GetParentWindow()->GetRect().Width()) )
				{
					justifyWidth = GetParentWindow()->GetRect().Width() - m_ParentOffset * 2;
				}
				SetPulloutOffset( GetParentWindow()->GetRect().left + m_ParentOffset );
				SetRect(	GetParentWindow()->GetRect().left + m_ParentOffset,
							GetParentWindow()->GetRect().left + justifyWidth + m_ParentOffset,
							(GetParentWindow()->GetRect().bottom - ((GetParentWindow()->GetRect().bottom-GetParentWindow()->GetRect().top)/2))-height/2, 
							GetParentWindow()->GetRect().bottom );
				break;
			}

			case JUSTIFY_RIGHT:
			{
				if ( m_bTruncateText && (width > GetParentWindow()->GetRect().Width()) )
				{
					justifyWidth = GetParentWindow()->GetRect().Width();
				}
				SetPulloutOffset( GetParentWindow()->GetRect().right - justifyWidth );
				SetRect(	GetParentWindow()->GetRect().right - justifyWidth, 
							GetParentWindow()->GetRect().right,
							(GetParentWindow()->GetRect().bottom - ((GetParentWindow()->GetRect().bottom-GetParentWindow()->GetRect().top)/2))-height/2, 
							GetParentWindow()->GetRect().bottom );
				break;
			}

			case JUSTIFY_CENTER:
			{
				if ( m_bTruncateText && (width > GetParentWindow()->GetRect().Width()) )
				{
					justifyWidth = GetParentWindow()->GetRect().Width() - m_ParentOffset * 2;
				}
				SetPulloutOffset( (GetParentWindow()->GetRect().right - ((GetParentWindow()->GetRect().right-GetParentWindow()->GetRect().left)/2)) - justifyWidth/2 );
				SetRect( (GetParentWindow()->GetRect().right - ((GetParentWindow()->GetRect().right-GetParentWindow()->GetRect().left)/2)) - justifyWidth/2, 
							GetParentWindow()->GetRect().right,
							(GetParentWindow()->GetRect().bottom - ((GetParentWindow()->GetRect().bottom-GetParentWindow()->GetRect().top)/2)) - height/2, 
							GetParentWindow()->GetRect().bottom );
				break;
			}
		}
	}
}


void UIText::SetRect( int left, int right, int top, int bottom, bool bAnimation )
{
	UIWindow::SetRect( left, right, top, bottom, bAnimation );

	if ( !GetParentWindow() )
	{
		UIWindow::SetRect( left, right, top, bottom, bAnimation );
	}
	else
	{
		int width	= 0;
		int height	= 0;
		if ( m_pFont )
		{
			gUIEmotes.CalculateStringSize( m_pFont, m_text.c_str(), width, height );
		}

		int justifyWidth = width;

		if ( GetParentWindow() && (GetParentWindow()->GetType() != UI_TYPE_CHECKBOX) )
		{
			switch ( m_justification )
			{
				case JUSTIFY_LEFT:
				{
					if ( m_bTruncateText && (width + m_ParentOffset * 2 > GetParentWindow()->GetRect().Width()) )
					{
						justifyWidth = GetParentWindow()->GetRect().Width() - m_ParentOffset * 2;
					}
					SetPulloutOffset( GetParentWindow()->GetRect().left + m_ParentOffset );
					UIWindow::SetRect(	GetParentWindow()->GetRect().left + m_ParentOffset,
								GetParentWindow()->GetRect().left + justifyWidth + m_ParentOffset,
								(GetParentWindow()->GetRect().bottom - ((GetParentWindow()->GetRect().bottom-GetParentWindow()->GetRect().top)/2))-height/2, 
								GetParentWindow()->GetRect().bottom );
					break;
				}

				case JUSTIFY_RIGHT:
				{
					if ( m_bTruncateText && (width > GetParentWindow()->GetRect().Width()) )
					{
						justifyWidth = GetParentWindow()->GetRect().Width();
					}
					SetPulloutOffset( GetParentWindow()->GetRect().right - justifyWidth );
					UIWindow::SetRect(	GetParentWindow()->GetRect().right - justifyWidth, 
								GetParentWindow()->GetRect().right,
								(GetParentWindow()->GetRect().bottom - ((GetParentWindow()->GetRect().bottom-GetParentWindow()->GetRect().top)/2))-height/2, 
								GetParentWindow()->GetRect().bottom );
					break;
				}

				case JUSTIFY_CENTER:
				{
					if ( m_bTruncateText && (width > GetParentWindow()->GetRect().Width()) )
					{
						justifyWidth = GetParentWindow()->GetRect().Width() - m_ParentOffset * 2;
					}
					SetPulloutOffset( (GetParentWindow()->GetRect().right - ((GetParentWindow()->GetRect().right-GetParentWindow()->GetRect().left)/2)) - justifyWidth/2 );
					UIWindow::SetRect( (GetParentWindow()->GetRect().right - ((GetParentWindow()->GetRect().right-GetParentWindow()->GetRect().left)/2)) - justifyWidth/2, 
								GetParentWindow()->GetRect().right,
								(GetParentWindow()->GetRect().bottom - ((GetParentWindow()->GetRect().bottom-GetParentWindow()->GetRect().top)/2)) - height/2, 
								GetParentWindow()->GetRect().bottom );
					break;
				}
			}
		}
	}

	if ( m_bTruncateText )
	{
		SetText( m_text, true );
	}
}


void UIText::ResizeToCurrentResolution( int original_width, int original_height )
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

			AdjustChildren();
		}
	}
}