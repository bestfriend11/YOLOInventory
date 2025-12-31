//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_textbox.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "precomp_gamegui.h"
#include "ui_emotes.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "fuel.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_textbox.h"
#include "ui_slider.h"
#include "LocHelp.h"


UITextBox::UITextBox( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
	: UIWindow( fhWindow, parent_interface, parent )
	, m_buttondown( false )
	, m_bFixedLocation( false )
	, m_font_width( 0 )
	, m_font_height( 0 )
	, m_pFont( NULL )
	, m_justification( JUSTIFY_LEFT )
	, m_scroll_rate( 0.0f )
	, m_scroll_offset( 0.0f )
	, m_max_width( gUIShell.GetScreenWidth()/2 )
	, m_max_height( gUIShell.GetScreenHeight()/2 )	
	, m_pVSlider( 0 )
	, m_bSlider( false )
	, m_lead_element( 0 )
	, m_bStopText( false )	
	, m_borderPadding( 0 )
	, m_bScrollThrough( false )
	, m_bFinished( false )
	, m_bNoStop( false )
	, m_bShowFirst( false )
	, m_bDisableUpdate( false )
	, m_bCenterHeight( false )
	, m_lineSpacer( 0 )	
{
	SetType( UI_TYPE_TEXTBOX );
	SetInputType( TYPE_INPUT_ALL );

	// Load a font
	SetTextColor( 0xFFFFFFFF );
	gpstring sFont;
	fhWindow.Get( "font_color", m_dwColor );	
	if ( fhWindow.Get( "font_type", sFont ) )
	{
		if ( (LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi()) )
		{
			gpstring sLocFont;
			if ( fhWindow.Get( "loc_font", sLocFont ) )
			{
				sLocFont = sFont;
			}			
		}

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
	}
	if ( !m_pFont )
	{
		gperrorf(( "Using default system font for GUI element: %s, change it!", GetName().c_str() ));
		m_pFont = gUITextureManager.GetDefaultFont();
	}

	gUIEmotes.CalculateStringSize( m_pFont, L"W", m_font_width, m_font_height );

	// Is the textbox a fixed box, or a tooltip style box
	fhWindow.Get( "fixed_location", m_bFixedLocation );

	// Determine scroll rate of text ( can be used for dialogue, etc. )
	if ( fhWindow.Get( "scroll_rate", m_scroll_rate ) )
	{
		if ( m_scroll_rate > 0.0f )
		{
			m_scroll_offset = (float)GetRect().bottom-(float)GetRect().top;
		}
	}

	if ( m_scroll_rate == 0.0f )
	{
		m_bFinished = true;
	}

	// Get text justification
	gpstring justify( "left" );
	fhWindow.Get( "justify", justify );
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

	// Should this text box have a slider?
	fhWindow.Get( "has_slider", m_bSlider );

	// Create a common control, if necessary
	bool bCommonCtrl = false;
	if ( fhWindow.Get( "common_control", bCommonCtrl ) )
	{
		if ( bCommonCtrl )
		{
			CreateCommonCtrl();
		}
	}

	fhWindow.Get( "border_padding", m_borderPadding );

	fhWindow.Get( "max_height", m_max_height );
	fhWindow.Get( "max_width", m_max_width );

	fhWindow.Get( "scroll_through", m_bScrollThrough );
	fhWindow.Get( "nostop", m_bNoStop );
	fhWindow.Get( "show_first_line", m_bShowFirst );

	fhWindow.Get( "center_height", m_bCenterHeight );

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

	gpwstring sText;
	if ( fhWindow.Get( "text", sText ) )
	{
		if ( LocMgr::DoesSingletonExist() )
		{
			gLocMgr.TranslateUiText( sText, GetInterfaceParent().c_str(), GetName().c_str() );
			m_pFont->RegisterString( sText );
		}
		SetText( sText );
	}
}
UITextBox::UITextBox()
	: UIWindow()
	, m_buttondown( false )
	, m_bFixedLocation( false )
	, m_font_width( 0 )
	, m_font_height( 0 )
	, m_pFont( NULL )
	, m_justification( JUSTIFY_LEFT )
	, m_scroll_rate( 0.0f )
	, m_scroll_offset( 0.0f )
	, m_max_width( gUIShell.GetScreenWidth()/2 )
	, m_max_height( gUIShell.GetScreenHeight()/2 )	
	, m_pVSlider( 0 )
	, m_bSlider( false )
	, m_lead_element( 0 )
	, m_bStopText( false )	
	, m_borderPadding( 0 )
	, m_bScrollThrough( false )
	, m_bFinished( false )
	, m_bNoStop( false )
	, m_bShowFirst( false )
	, m_bDisableUpdate( false )
	, m_bCenterHeight( false )
	, m_lineSpacer( 0 )
{
	SetTextColor( 0xFFFFFFFF );
	SetType( UI_TYPE_TEXTBOX );
	SetInputType( TYPE_INPUT_ALL );	
}


UITextBox::~UITextBox()
{
	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && m_pFont )
	{
		TextLineVec::iterator i;
		for ( i = m_lines.begin(); i != m_lines.end(); ++i )
		{
			m_pFont->UnregisterString( (*i).sText );
		}
	}
}


bool UITextBox::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true ) {
		return true;
	}
	return false;
}

bool UITextBox::ProcessMessage( UIMessage & msg )
{
	UIWindow::ProcessMessage( msg );

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
				if ( GetParentWindow() && (GetParentWindow()->GetType() == UI_TYPE_CHECKBOX) )
				{
					UICheckbox *pCheckbox = (UICheckbox *)GetParentWindow();
					pCheckbox->SetState( !pCheckbox->GetState() );
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
	case MSG_WHEELUP:
		{
			if ( m_pVSlider )
			{
				m_pVSlider->SetValue( m_pVSlider->GetValue() - 2 );
			}
		}
		break;
	case MSG_WHEELDOWN:
		{
			if ( m_pVSlider )
			{
				m_pVSlider->SetValue( m_pVSlider->GetValue() + 2 );
			}
		}
		break;

	}
	return true;
}



void UITextBox::CreateCommonCtrl( gpstring sTemplate )
{
	UNREFERENCED_PARAMETER( sTemplate );
	SetIsCommonControl( true );

	UIButton * pButtonUp	= 0;
	UIButton * pButtonDown	= 0;
	UISlider * pSliderV		= 0;

	if ( GetTextureIndex() != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( GetTextureIndex() );
	}

	if ( !HasTexture() )
	{
		SetTextureIndex( gUIShell.GetCommonArt( "generic_textbox_background" ) );
	}

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
		pButtonUp->SetRect(	GetRect().right, GetRect().right+pButtonUp->GetRect().right, GetRect().top, GetRect().top + pButtonUp->GetRect().bottom );
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
	}
}


void UITextBox::SetVisible( bool bVisible )
{
	if ( gUIShell.GetItemActive() && !m_bFixedLocation )
	{
		UIWindow::SetVisible( false );
	}
	else
	{
		UIWindow::SetVisible( bVisible );
	}
}


void UITextBox::GetColorRangeVec( gpwstring & sText, ColorRanges & colorVec 
)
{
	bool bBegin		= false;
	int beginRange	= 0;
	int endRange	= 0;
	DWORD dwColor	= 0;

	unsigned int macroBegin = sText.find( L'<' );
	unsigned int macroEnd	= sText.find( L'>' );
	while ( macroBegin != gpwstring::npos && macroEnd != gpwstring::npos )
	{
		gpwstring sMacro = sText.substr( macroBegin+1, (macroEnd-macroBegin)-1 );

		if ( !bBegin )
		{
			if ( sMacro[0] == L'c' &&
				 sMacro[1] == L':' )
			{
				bBegin = true;
				beginRange = macroBegin;

				gpwstring sHexColor = sMacro.substr( 2, sMacro.size() );
				stringtool::Get( sHexColor, dwColor );
			}
			else
			{
				return;
			}
		}
		else
		{
			if ( sMacro[0] == L'/' &&
				 sMacro[1] == L'c' )
			{
				bBegin = false;
				endRange = macroBegin-1;
			}
			else
			{
				return;
			}
		}

		gpwstring sBegin	= sText.substr( 0, macroBegin );
		gpwstring sEnd		= sText.substr( macroEnd+1, sText.size() );

		sText	=	sBegin;
		sText	+=	sEnd;

		if ( !bBegin )
		{
			ColorRange colorRange;
			colorRange.beginRange	= beginRange;
			colorRange.endRange		= endRange;
			colorRange.dwColor		= dwColor;

			colorVec.push_back( colorRange );
		}

		macroBegin	= sText.find( L'<' );
		macroEnd	= sText.find( L'>' );
	}
}

bool UITextBox::GetColorRange( int pos, ColorRanges & colorVec, ColorRange & colorRange )
{
	ColorRanges::iterator i;
	for ( i = colorVec.begin(); i != colorVec.end(); ++i )
	{
		if ( (*i).beginRange <= pos && (*i).endRange >= pos )
		{
			colorRange = (*i);
			colorVec.erase( i );
			return true;
		}
	}

	return false;
}


void UITextBox::ConvertColorRangeToSegments( ColorRanges & colorRanges, 
TextLine & line )
{
	if ( colorRanges.size() != 0 )
	{
		int startPos = 0;
		int xOffset = 0;
		for ( int iChar = 0; iChar != (int)line.sText.size(); ++iChar )
		{
			ColorRanges::iterator iColor;
			for ( iColor = colorRanges.begin(); iColor != colorRanges.end(); ++iColor )
			{
				if ( iChar >= (*iColor).beginRange && iChar <= (*iColor).endRange )
				{
					if ( startPos != iChar )
					{
						ColorSegment colorSegment;
						colorSegment.dwColor = line.dwColor;
						colorSegment.sText = line.sText.substr( startPos, ((*iColor).beginRange-startPos) );
						colorSegment.xOffset = xOffset;
						line.colorSegments.push_back( colorSegment );

						int width = 0, height = 0;
						gUIEmotes.CalculateStringSize( m_pFont, colorSegment.sText, width, height );
						xOffset += width;
					}

					startPos = iChar;

					ColorSegment colorSegment;
					colorSegment.dwColor = (*iColor).dwColor;
					colorSegment.sText = line.sText.substr( startPos, (*iColor).endRange-(*iColor).beginRange+1 );
					colorSegment.xOffset = xOffset;
					line.colorSegments.push_back( colorSegment );

					startPos += (*iColor).endRange-(*iColor).beginRange+1;

					int width = 0, height = 0;
					gUIEmotes.CalculateStringSize( m_pFont, colorSegment.sText, width, height );
					xOffset += width;

					iColor = colorRanges.erase( iColor );
					if ( iColor == colorRanges.end() )
					{
						break;
					}
				}
			}
		}

		if ( startPos != iChar && startPos < (int)line.sText.size() )
		{
			ColorSegment colorSegment;
			colorSegment.dwColor = line.dwColor;
			colorSegment.sText = line.sText.substr( startPos, (iChar-startPos) );
			colorSegment.xOffset = xOffset;
			line.colorSegments.push_back( colorSegment );
		}
	}
}


void UITextBox::PackageColorSegments( int beginPos, TextLine & line, const 
ColorRanges & colorVec )
{
	ColorRanges colorCopy = colorVec;
	if ( colorVec.size() != 0 )
	{
		ColorRanges colorRanges;
		for ( int iChar = 0; iChar != (int)line.sText.size(); ++iChar )
		{
			ColorRange colorRange;
			if ( GetColorRange( iChar+beginPos, colorCopy, colorRange ) )
			{
				int diff = colorRange.endRange - ( beginPos > colorRange.beginRange ? beginPos : colorRange.beginRange );
				colorRange.beginRange = iChar;
				colorRange.endRange = iChar + diff;
				colorRanges.push_back( colorRange );
			}
		}

		ConvertColorRangeToSegments( colorRanges, line );
	}
}


bool UITextBox::EnlargeRect()
{
	if ( (GetRect().Width() + m_font_width) <= m_max_width )
	{
		SetRect( GetRect().left, GetRect().right + m_font_width, GetRect().top, GetRect().bottom );
	}
	else if ( (GetRect().Height() + m_font_height) <= m_max_height )
	{
		SetRect( GetRect().left, GetRect().right, GetRect().top, GetRect().bottom + m_font_height );
	}
	else
	{
		return false;
	}

	return true;
}


bool UITextBox::DoesTextFit( gpwstring sText )
{
	std::vector< gpwstring > testStrings;
	gpwstring sCurrent;

	int width	= 0;
	int height	= 0;
	int lastSpace = (int)gpwstring::npos;
	for ( unsigned int i = 0; i != sText.size(); ++i )
	{
		if ( sText[i] == L' ' )
		{
			lastSpace = i;
		}

		sCurrent.appendf( L"%c", sText[i] );

		gUIEmotes.CalculateStringSize( m_pFont, sCurrent.c_str(), width, height );
		if ( width > GetRect().Width() )
		{
			int pos = 0;
			if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
			{
				pos = i;
			}
			else
			{
				if ( lastSpace != (int)gpwstring::npos )
				{
					i = pos = lastSpace;
					lastSpace = (int)gpwstring::npos;
				}
				else
				{
					pos = i;
				}
			}

			if ( pos != (int)gpwstring::npos )
			{
				gpwstring sTemp = sCurrent.substr( 0, pos );
				sCurrent = sTemp;
				testStrings.push_back( sCurrent );
				sCurrent.clear();
				if ( sText[i] != L' ' )
				{
					sCurrent.appendf( L"%c", sText[i] );
				}
			}
			else
			{
				return false;
			}
		}
	}

	if ( !(( ((int)testStrings.size()+1) * m_font_height ) <= ( GetRect().Height() )) )
	{
		return false;
	}

	return true;
}


bool UITextBox::DoesLineTextFit( gpwstring sText )
{
	if ( m_pFont )
	{
		int width	= 0;
		int height	= 0;
		gUIEmotes.CalculateStringSize( m_pFont, sText.c_str(), width, height );
		if ( width <= GetRect().Width() )
		{
			return true;
		}
	}

	return false;
}


void UITextBox::CenterWindowToMousePos()
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


void UITextBox::SetText( gpwstring sText )
{
	m_bFinished = false;
	if ( m_scroll_rate > 0.0f )
	{
		m_scroll_offset = (float)GetRect().bottom-(float)GetRect().top;

		if ( m_bScrollThrough || m_bShowFirst )
		{
			m_scroll_offset -= m_font_height;
		}
	}
	else
	{
		m_bFinished = true;
	}

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		TextLineVec::iterator iClear;
		for ( iClear = m_lines.begin(); iClear != m_lines.end(); ++iClear )
		{
			m_pFont->UnregisterString( (*iClear).sText );
		}
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

	// Extract out all the color information
	ColorRanges colorVec;
	GetColorRangeVec( sText, colorVec );

	int largestLineWidth = 0;
	int width	= 0;
	int height	= 0;
	int numNewLines = 0;
	for ( unsigned int i = 0; i != sText.size(); ++i )
	{
		bool bNewline = false;
		if (( (sText[i] == L'\\') && (sText[i+1] == L'n') ) || sText[i] == L'\n' )
		{
			numNewLines++;
			bNewline = true;
			if ( sText[i] != L'\n' )
			{
				++i;
			}
		}
		else
		{
			if ( i >= sText.size() )
			{
				break;
			}
			line.sText.appendf( L"%c", sText[i] );
		}

		/*
		int errorMargin = 0;
		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{
			int tempHeight = 0;
			gpwstring sTemp;
			sTemp += sText[i];
			m_pFont->CalculateStringSize( sTemp, errorMargin, tempHeight );
		}
		*/

		gUIEmotes.CalculateStringSize( m_pFont, line.sText, width, height );
		if ( width > (GetRect().right-GetRect().left) )
		{
			int pos = 0;			
			if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
			{
				pos = line.sText.size()-1;
				gpstring sDbcs = ToAnsi( line.sText.substr( pos-1, 1 ) );
				if ( !IsDBCSLeadByte( (char)sDbcs[0] ) )
				{
					pos = line.sText.find_last_of( L" " );
					
					if ( (unsigned int)pos == gpwstring::npos )
					{
						pos = line.sText.size()-1;
					}
					else
					{
						pos++;
						i = i - (line.sText.size() - pos );
					}	
				}			
			}
			else
			{
				pos = line.sText.find_last_of( L" " );
				if ( (unsigned int)pos == gpwstring::npos )
				{
					pos = line.sText.size()-1;
				}
				else
				{
					pos++;
					i = i - (line.sText.size() - pos );
				}				
			}

			if ( (unsigned int)pos == gpwstring::npos )
			{
				return;
			}			

			gpwstring sTemp = line.sText.substr( 0, pos );			
			line.sText = sTemp;
			int beginPos = i - line.sText.size() + 1;

			PackageColorSegments( beginPos, line, colorVec );
			m_lines.push_back( line );
			line.colorSegments.clear();

			gUIEmotes.CalculateStringSize( m_pFont, line.sText, width, height );
			if ( width > largestLineWidth )
			{
				largestLineWidth = width;
			}

			line.sText.clear();
			if ( sText[i] != L' ' )
			{
				line.sText.appendf( L"%c", sText[i] );
			}
		}
		else if ( bNewline )
		{
			int beginPos = i - line.sText.size();
			PackageColorSegments( beginPos, line, colorVec );

			m_lines.push_back( line );
			line.colorSegments.clear();
			if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
			{
				m_pFont->RegisterString( line.sText );
			}
			line.sText.clear();
		}
	}

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		m_pFont->RegisterString( line.sText );
	}

	int beginPos = i - line.sText.size() ;
	PackageColorSegments( beginPos, line, colorVec );

	m_lines.push_back( line );

	gUIEmotes.CalculateStringSize( m_pFont, line.sText.c_str(), width, height );
	if ( width > largestLineWidth )
	{
		largestLineWidth = width;
	}

	if ( !m_bFixedLocation )
	{
		GetRect().right = largestLineWidth + m_borderPadding * 2;
		GetRect().bottom += m_borderPadding * 2;
	}

	if ( m_bSlider && m_pVSlider )
	{
		m_pVSlider->SetMax( GetNumElements() - GetMaxActiveElements() );
		m_pVSlider->SetMin( 0 );
		m_pVSlider->CalculateSlider();
	}
}


int UITextBox::ReformatTooltip( const gpwstring & sText, int lineNum, DWORD dwColor )
{
	int height = 0, width = 0;
	gpwstring sTemp;
	int runningSize = 0;

	for ( unsigned int i = 0; i != sText.size(); ++i )
	{
		sTemp += sText[i];
		gUIEmotes.CalculateStringSize( m_pFont, sTemp, width, height );

		if ( width > m_max_width )
		{
			int pos = 0;
			if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
			{
				sTemp = sTemp.substr( 0, sTemp.size() - 1 );
				pos = sTemp.size();
			}
			else
			{
				pos = sTemp.find_last_of( L" " );
			}

			if ( (unsigned int)pos == gpwstring::npos )
			{
				return lineNum;
			}

			i = pos + runningSize;

			sTemp = sTemp.substr( 0, pos );
			SetLineText( lineNum++, sTemp, dwColor );

			// Handle running size
			{
				runningSize += sTemp.size();
				if ( !LocMgr::DoesSingletonExist() || ( LocMgr::DoesSingletonExist() && gLocMgr.IsAnsi() ) )
				{
					runningSize += 1;
				}
				else
				{
					i--;
				}
			}

			sTemp.clear();
		}
	}

	SetLineText( lineNum++, sTemp, dwColor );

	return lineNum;
}


int UITextBox::SetLineText( int lineNum, gpwstring sText, DWORD dwColor )
{
	ColorRanges colorVec;
	GetColorRangeVec( sText, colorVec );

	int height = 0, width = 0;
	gUIEmotes.CalculateStringSize( m_pFont, sText, width, height );
	if ( !m_bFixedLocation && (width > m_max_width) )
	{
		return ReformatTooltip( sText, lineNum, dwColor );
	}

	if ( lineNum == 0 )
	{
		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{
			TextLineVec::iterator iClear;
			for ( iClear = m_lines.begin(); iClear != m_lines.end(); ++iClear )
			{
				m_pFont->UnregisterString( (*iClear).sText );
			}
		}
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

			if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
			{
				m_pFont->UnregisterString( (*i).sText );
				m_pFont->RegisterString( sText );
			}

			(*i).sText = sText;
			PackageColorSegments( 0, *i, colorVec );

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
		PackageColorSegments( 0, line, colorVec );
		m_lines.push_back( line );
	}

	if ( !m_bFixedLocation )
	{
		SetRect( 0, 0, 0, 0 );
		for ( i = m_lines.begin(); i != m_lines.end(); ++i )
		{
			int width	= 0;
			int height	= 0;
			gUIEmotes.CalculateStringSize( m_pFont, (*i).sText.c_str(), width, height );
			if ( width > (GetRect().right - GetRect().left) )
			{
				SetRect( 0, width, 0, 0 );
			}
		}
		SetRect( GetRect().left, GetRect().right + (m_borderPadding * 2), 0, m_lines.size() * m_font_height + (m_borderPadding * 2) );
	}

	return ++lineNum;
}


void UITextBox::SetToolTipText( gpwstring sText )
{
	gpwstring sNewLine;
	DWORD dwColor = 0xFFFFFFFF;
	int lineNum = 0;
	for ( unsigned int i = 0; i != sText.size(); ++i )
	{
		if (( (sText[i] == L'\\') && (sText[i+1] == L'n') ) || sText[i] == L'\n' )
		{
			lineNum = SetLineText( lineNum, sNewLine, dwColor );
			dwColor = 0xFFFFFFFF;
			sNewLine.clear();

			if ( (sText[i] == L'\\') && (sText[i+1] == L'n') )
			{
				i++;
			}
		}
		else if ( sText[i] == L'<' && !( sText[i+1] == L'c' && sText[i+2] == L':' ) &&
									 !( sText[i+1] == L'/' && sText[i+2] == L'c' ) )
		{
			gpwstring sColor;
			++i;
			while ( sText[i] != L'>' )
			{
				sColor.appendf( L"%c", sText[i] );
				++i;
			}

			dwColor = gUIShell.GetTipColor( ToAnsi( sColor.c_str() ) );
		}
		else
		{
			sNewLine.appendf( L"%c", sText[i] );
		}
	}

	SetLineText( lineNum, sNewLine, dwColor );
}


void UITextBox::Draw()
{
	UIWindow::Draw();
	if ( GetVisible() )
	{
		if ( m_bFinished && m_bNoStop )
		{
			return;
		}

		bool bDrawn = false;
		TextLineVec::iterator i;
		int index = 0;
		for ( i = m_lines.begin(); i != m_lines.end(); ++i )
		{
			int lineSpacer = 0;
			if ( i == m_lines.begin() )
			{
				lineSpacer = 0;
			}

			int x_offset = GetRect().left;
			int y_offset = GetRect().top + ( m_font_height * index ) + (int)floor((double)m_scroll_offset);

			if ( m_bFinished )
			{
				if ( index < GetLeadElement() )
				{
					++index;
					continue;
				}
				else
				{
					y_offset = GetRect().top + ( (index-GetLeadElement()) * m_font_height );
				}
			}

			switch ( m_justification )
			{
			case JUSTIFY_LEFT:
				{
					x_offset = GetRect().left + m_borderPadding;
					if ( m_bCenterHeight )
					{
						y_offset = ((int)GetRect().Height() - ((int)m_lines.size() * (int)m_font_height)) / 2 + GetRect().top + index * m_font_height;
					}
				}
				break;
			case JUSTIFY_RIGHT:
				{
					int width	= 0;
					int height	= 0;
					gUIEmotes.CalculateStringSize( m_pFont, (*i).sText.c_str(), width, height );
					x_offset = GetRect().right - width - m_borderPadding;
					if ( m_bCenterHeight )
					{
						y_offset = ((int)GetRect().Height() - ((int)m_lines.size() * (int)m_font_height)) / 2 + GetRect().top + index * m_font_height;
					}
				}
				break;
			case JUSTIFY_CENTER:
				{
					int width	= 0;
					int height	= 0;
					gUIEmotes.CalculateStringSize( m_pFont, (*i).sText.c_str(), width, height );
					x_offset = ( ((GetRect().right-GetRect().left) - width) / 2 ) + GetRect().left;
					if ( m_bCenterHeight )
					{
						y_offset = (GetRect().Height() - ((int)m_lines.size() * m_font_height)) / 2 + GetRect().top + index * m_font_height;
					}
				}
				break;
			}

			y_offset += lineSpacer * index;

			if (( (y_offset+m_font_height-1) <= GetRect().bottom ) && ( y_offset >= GetRect().top ))
			{
				if ( (*i).colorSegments.size() == 0 )
				{
					gUIEmotes.Draw( m_pFont, x_offset, y_offset, (*i).sText.c_str(), (*i).dwColor, (UIWindow*) this, true );
				}
				else
				{
					ColorSegments::iterator iColor;
					for ( iColor = (*i).colorSegments.begin(); iColor != (*i).colorSegments.end(); ++iColor )
					{
						gUIEmotes.Draw( m_pFont, x_offset+(*iColor).xOffset, y_offset, (*iColor).sText, (*iColor).dwColor, (UIWindow*) this, true );
					}
				}
				bDrawn = true;
			}

			++index;
		}

		if ( m_bNoStop && !bDrawn )
		{
			m_bFinished = true;
		}
	}
}


void UITextBox::Update( double seconds )
{
	if ( GetDisableUpdate() )
	{
		return;
	}

	// If we're a scrolling textbox...
	if ( m_scroll_rate != 0.0f )
	{
		// If we're finished scrolling all of our text, let's early out
		if ( m_bFinished )
		{
			if ( m_bNoStop )
			{
				// All the text in a continually scrolling box is gone
				gUIMessenger.Notify( "textbox_nostop_done", this );
			}
			return;
		}

		// temp is our total pixel height
		int temp = ((int)m_lines.size()) * m_font_height;

		// height is the heigh of the textbox itself
		int height = GetRect().bottom-GetRect().top;

		// scroll offset represents how many pixels we need to scroll the box.
		m_scroll_offset -= (m_scroll_rate*(float)seconds);

		// If the temp height is greater than the textbox height and the height-scroll offset is greater than temp,	
		// and we're not a continuous scrolling box, then reset our scroll offset to a set amount to keep it from moving
		// any farther.
		if (( temp > height ) && ( (height-m_scroll_offset) > temp-1 ))
		{
			if ( !m_bNoStop )
			{
				m_scroll_offset = (float)height - ((float)temp);
			}
		}

		// Let's see if we're done scrolling
		bool bDone = false;
		if ( !m_bScrollThrough )
		{
			bDone = ( (height-m_scroll_offset) >= (GetNumElements() * m_font_height) && temp > height && !m_bNoStop );
		}
		else
		{
			bDone = ( (height-m_scroll_offset) >= temp && temp > height && !m_bNoStop );
		}

		if ( bDone )
		{
			// We're done scrolling, let's set up the scroll bar accordingly
			int y_offset = GetRect().top + (int)floor((double)m_scroll_offset) + m_font_height;
			if ( y_offset <= GetRect().top )
			{
				m_bFinished = true;
				SetLeadElement( GetNumElements() - GetMaxActiveElements() );
				if ( m_pVSlider )
				{
					m_pVSlider->SetValue( GetLeadElement() );
					m_pVSlider->CalculateSlider();
				}
				return;
			}
		}

		// Not much text here, no need to do any fancy scrolling.
		if ( m_scroll_offset < 0 && temp <= GetRect().bottom-GetRect().top && !m_bScrollThrough )
		{
			m_bFinished = true;
			m_scroll_offset = 0;
			SetLeadElement( 0 );
		}
	}
}


void UITextBox::SetMousePos( unsigned int x, unsigned int y )
{
	UIWindow::SetMousePos( x, y );
}


unsigned int UITextBox::GetNumElements()
{
	return m_lines.size();
}


unsigned int UITextBox::GetMaxActiveElements()
{
	return ( (GetRect().bottom-GetRect().top)/m_font_height );
}


void UITextBox::SetScrolling( bool bScrolling )
{
	m_bStopText = bScrolling;
	if ( m_bStopText )
	{
		m_scroll_offset = 0;
		m_bFinished = true;
	}	
}


void UITextBox::RecalculateSlider()
{
	if ( m_pVSlider )
	{
		m_pVSlider->SetValue( GetLeadElement() );
		m_pVSlider->CalculateSlider();
	}
}


void UITextBox::SetChildSlider( UISlider * pSlider )
{
	m_pVSlider = pSlider;
	m_bSlider = true;
}


#if !GP_RETAIL
void UITextBox::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "text_box" );

			if ( m_pFont )
			{
				hWindow->Set( "font_type", m_pFont->GetTextureName() );
			}

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

			hWindow->Set( "fixed_location", m_bFixedLocation );
			hWindow->Set( "scroll_rate", m_scroll_rate );
			hWindow->Set( "has_slider", m_bSlider );
			hWindow->Set( "max_height", m_max_height );
			hWindow->Set( "max_width", m_max_width );
			hWindow->Set( "center_height", m_bCenterHeight );
		}
	}
}
#endif

