//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_tab.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_tab.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_textureman.h"
#include "ui_animation.h"


UITab::UITab( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
	: UIWindow( fhWindow, parent_interface, parent )	
	, m_buttondown( false )
	, m_checked( false )	
	, m_bStretchCtrl( false )
	, m_row( 0 )
	, m_column( 0 )
	, m_bTextUp( true )
	, m_button_left_tex( 0 )
	, m_button_right_tex( 0 )	
	, m_button_center_tex( 0 )		
	, m_dwDisable( 0x99555555 )		
{	
	SetType( UI_TYPE_TAB );
	SetInputType( TYPE_INPUT_ALL );	
	
	if ( !fhWindow.Get( "radio_group", m_sRadioGroup ) ) 
	{
		gpassertm( 0, "Radio button does not have a radio_group!  That kind of defeats the purpose..." );
	}

	fhWindow.Get( "disable_color", m_dwDisable );

	fhWindow.Get( "row", m_row );
	fhWindow.Get( "column", m_column );

	bool bCommon = false;
	if ( fhWindow.Get( "common_control", bCommon ) ) {
		if ( bCommon ) 
		{
			gpstring sTemplate;
			fhWindow.Get( "common_template", sTemplate );
			CreateCommonCtrl( sTemplate );		
		}
		return;
	}

}
UITab::UITab()
	: UIWindow()
	, m_buttondown( false )
	, m_checked( false )	
	, m_bStretchCtrl( false )
	, m_button_center_tex( 0 )
	, m_button_left_tex( 0 )
	, m_button_right_tex( 0 )
	, m_row( 0 )
	, m_column( 0 )	
	, m_dwDisable( 0xff555555 )	
	, m_bTextUp( true )
{	
	SetType( UI_TYPE_TAB );
	SetInputType( TYPE_INPUT_ALL );		
}


/**************************************************************************

Function		: ~UITab()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UITab::~UITab()
{
	if ( m_button_center_tex  != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
	}

	if ( m_button_left_tex != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
	}	
	
	if ( m_button_right_tex != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
	}

}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UITab::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true ) {
		return true;
	}
	else if ( action == ACTION_SETGROUP ) {
		SetGroup( parameter );
		return true;
	}	
	else if ( action == ACTION_SETSTATE ) {
		int state = 0;
		stringtool::Get( parameter, state );
		if ( state == 0 ) {
			m_checked = false;			
		}
		else if ( state == 1 ) {
			gUIMessenger.SendUIMessageToGroup( UIMessage( MSG_UNCHECK ), GetRadioGroup() );
			gUIMessenger.SendUIMessage( UIMessage( MSG_CHECK ), this );
			m_checked = true;
		}
		return true;
	}
	return false;
}


void UITab::SetCheck( bool bCheck ) 
{
	if ( m_checked == bCheck )
	{
		return;
	}

	if ( bCheck )
	{
		gUIMessenger.SendUIMessageToGroup( UIMessage( MSG_UNCHECK ), GetRadioGroup() );
		gUIMessenger.SendUIMessage( UIMessage( MSG_CHECK ), this );
	}
	else
	{
		gUIMessenger.SendUIMessage( UIMessage( MSG_UNCHECK ), this );
	}
}


void UITab::SetForceCheck( bool bCheck )
{
	m_checked = bCheck;
	if ( m_checked == true )
	{
		gUIMessenger.SendUIMessageToGroup( UIMessage( MSG_UNCHECK ), GetRadioGroup() );
		gUIMessenger.SendUIMessage( UIMessage( MSG_FORCECHECK ), this );
	}
	else 
	{
		gUIMessenger.SendUIMessageToGroup( UIMessage( MSG_UNCHECK ), GetRadioGroup() );
	}
}

/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: bool

**************************************************************************/
bool UITab::ProcessMessage( UIMessage & msg )
{
	UIWindow::ProcessMessage( msg );

	switch ( msg.GetCode() ) 
	{
	case MSG_LBUTTONDOWN:
		{
			MoveGroupTextDown();
			m_buttondown = true;
			OnButtonDown();
		}
		break;
	case MSG_LBUTTONUP:
		{
			if ( m_buttondown == true ) {
				MoveGroupTextUp();		
				gUIMessenger.SendUIMessageToGroup( UIMessage( MSG_UNCHECK ), GetRadioGroup() );
				gUIMessenger.SendUIMessage( UIMessage( MSG_CHECK ), this );
				m_checked = true;
			}
			m_buttondown	= false;
			OnButtonUp();
		}
		break;
	case MSG_GLOBALLBUTTONUP:
		{
			m_buttondown = false;			
		}
		break;
	case MSG_UNCHECK:
		{
			if ( m_checked )
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_UNCHECKED ), this );
			}
			m_checked = false;
		}
		break;
	case MSG_CHECK:
		{
			OnCheck();
			m_checked = true;
		}
		break;
	case MSG_ROLLOVER:
		{
			OnButtonRollover();
			if ( m_buttondown == true ) {
				gUIMessenger.SendUIMessage( UIMessage( MSG_LBUTTONDOWN ), this );
				return false;
			}
			if ( gUIShell.GetLButtonDown() == true ) {
				return false;
			}
			if ( m_checked == true ) {
				return false;
			}			
		}
		break;
	case MSG_ROLLOFF:
		{
			if ( m_buttondown == true ) {
				MoveGroupTextUp();				
			}
			OnButtonRolloff();
			if ( m_checked == true ) {
				return false;
			}			
		}
		break;
	case MSG_FORCECHECK:
		{
			m_checked = true;
		}
		break;
	case MSG_HIDE:
		{	
			m_buttondown = false;
			MoveGroupTextUp();
			gUIMessenger.SendUIMessage( UIMessage( MSG_ROLLOFF ), this );
		}
		break;
	}

	return true;
}



/**************************************************************************

Function		: Draw()

Purpose			: Render the radio button and its selection texture, if
				  applicable

Returns			: void

**************************************************************************/
void UITab::Draw() 
{ 
	if ( GetVisible() == true ) {
		UIWindow::Draw();

		if ( m_bStretchCtrl ) 
		{
			UIRect component_rect;
			int width	= gUIShell.GetRenderer().GetTextureWidth( m_button_left_tex );

			component_rect.left		= GetRect().left;
			component_rect.right	= GetRect().left + width;
			component_rect.top		= GetRect().top;
			component_rect.bottom	= GetRect().bottom;
			DrawComponent( component_rect, m_button_left_tex, GetAlpha() );

			component_rect.left		= GetRect().right - width;
			component_rect.right	= GetRect().right;
			component_rect.top		= GetRect().top;
			component_rect.bottom	= GetRect().bottom;
			DrawComponent( component_rect, m_button_right_tex, GetAlpha() );

			component_rect.left		= GetRect().left + width;
			component_rect.right	= GetRect().right - width;
			component_rect.top		= GetRect().top;
			component_rect.bottom	= GetRect().bottom;

			int sideWidth	= gUIShell.GetRenderer().GetTextureWidth( m_button_center_tex );	
			UINormalizedRect uv_component_rect;
			uv_component_rect.top	= 0.0f;
			uv_component_rect.left	= 0.0f;
			uv_component_rect.right	= (float)( component_rect.right-component_rect.left ) / (float)sideWidth;
			uv_component_rect.bottom= 1.0f;

			DrawComponent( component_rect, m_button_center_tex, GetAlpha(), uv_component_rect, true );		

			if ( !IsEnabled() )
			{
				gUIShell.DrawColorBox( GetRect(), GetDisableColor() );
			}
		}		
	}
}


#if !GP_RETAIL
void UITab::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "radio_button" );
			hWindow->Set( "radio_group", m_sRadioGroup );
		}
	}
}
#endif


void UITab::MoveGroupTextDown()
{
	if ( m_bTextUp )
	{
		m_bTextUp = false;
		if ( !GetChildren().empty() )
		{
			UIWindowVec group = GetChildren();
			UIWindowVec::iterator i;
			for ( i = group.begin(); i != group.end(); ++i )
			{
				if ( (*i)->GetType() == UI_TYPE_TEXT )
				{
					++(*i)->GetRect().top;
					++(*i)->GetRect().bottom;
				}
			}
		}
	}
}


void UITab::MoveGroupTextUp()
{
	if ( !m_bTextUp )
	{
		m_bTextUp = true;
		if ( !GetChildren().empty() )
		{
			UIWindowVec group = GetChildren();
			UIWindowVec::iterator i;
			for ( i = group.begin(); i != group.end(); ++i )
			{
				if ( (*i)->GetType() == UI_TYPE_TEXT )
				{
					--(*i)->GetRect().top;
					--(*i)->GetRect().bottom;
				}
			}
		}
	}
}


void UITab::CreateCommonCtrl( gpstring sTemplate )
{
	UNREFERENCED_PARAMETER( sTemplate );
	SetIsCommonControl( true );
	if ( sTemplate.empty() )
	{
		m_bStretchCtrl = true;
		SetCommonTemplate( "" );
		m_button_center_tex	= gUIShell.GetCommonArt( "tab_center_up" );
		m_button_left_tex	= gUIShell.GetCommonArt( "tab_left_up" );
		m_button_right_tex	= gUIShell.GetCommonArt( "tab_right_up" );
	}	
	else if ( sTemplate.same_no_case( "button_4" ) )
	{
		m_bStretchCtrl = true;
		SetCommonTemplate( "button_4" );
		m_button_center_tex	= gUIShell.GetCommonArt( "button4_center_up" );
		m_button_left_tex	= gUIShell.GetCommonArt( "button4_left_up" );
		m_button_right_tex	= gUIShell.GetCommonArt( "button4_right_up" );
	}
	else if ( sTemplate.same_no_case( "subtab" ) )
	{
		m_bStretchCtrl = true;
		SetCommonTemplate( "subtab" );
		m_button_center_tex	= gUIShell.GetCommonArt( "subtab_center_up" );
		m_button_left_tex	= gUIShell.GetCommonArt( "subtab_left_up" );
		m_button_right_tex	= gUIShell.GetCommonArt( "subtab_right_up" );
	}
}


void UITab::OnButtonDown()
{
	if ( m_bStretchCtrl ) 
	{
		if ( GetCommonTemplate().empty() )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "tab_center_down" );
			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "tab_left_down" );
			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "tab_right_down" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_4" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button4_center_down" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button4_left_down" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button4_right_down" );
		}
		else if ( GetCommonTemplate().same_no_case( "subtab" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "subtab_center_down" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "subtab_left_down" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "subtab_right_down" );
		}
	}
}


void UITab::OnButtonUp()
{
	if ( m_bStretchCtrl ) 
	{
		if ( GetCommonTemplate().empty() )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "tab_center_up" );
			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "tab_left_up" );
			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "tab_right_up" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_4" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button4_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button4_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button4_right_up" );
		}
		else if ( GetCommonTemplate().same_no_case( "subtab" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "subtab_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "subtab_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "subtab_right_up" );
		}
	}
}


void UITab::OnButtonRollover()
{
	if ( m_bStretchCtrl ) 
	{
		if ( GetCommonTemplate().empty() )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "tab_center_hov" );
			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "tab_left_hov" );
			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "tab_right_hov" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_4" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button4_center_hov" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button4_left_hov" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button4_right_hov" );
		}
		else if ( GetCommonTemplate().same_no_case( "subtab" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "subtab_center_hov" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "subtab_left_hov" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "subtab_right_hov" );
		}
	}
}


void UITab::OnButtonRolloff()
{
	if ( m_bStretchCtrl ) 
	{
		if ( GetCommonTemplate().empty() )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "tab_center_up" );
			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "tab_left_up" );
			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "tab_right_up" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_4" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button4_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button4_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button4_right_up" );
		}
		else if ( GetCommonTemplate().same_no_case( "subtab" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "subtab_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "subtab_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "subtab_right_up" );
		}
	}
}


void UITab::OnCheck()
{
	if ( GetRow() != 0 )
	{
		UIWindowVec tabs = gUIShell.ListWindowsOfRadioGroup( GetRadioGroup() );
		UIWindowVec::iterator i;
		UIWindowVec bottom_row;
		UIWindowVec current_row;
		int currentRow = GetRow();
		for ( i = tabs.begin(); i != tabs.end(); ++i )
		{
			if ( ((UITab *)(*i))->GetRow() == 0 )
			{
				bottom_row.push_back( *i );
			}

			if ( ((UITab *)(*i))->GetRow() == GetRow() )
			{
				current_row.push_back( *i );
			}
		}

		if ( current_row.size() != bottom_row.size() )
		{
			return;
		}

		UIWindowVec::iterator j = current_row.begin();
		for ( i = bottom_row.begin(); i != bottom_row.end(); ++i )
		{
			UIRect tempRect = (*j)->GetRect();
			(*j)->SetRect( (*i)->GetRect().left, (*i)->GetRect().right, (*i)->GetRect().top, (*i)->GetRect().bottom );
			(*i)->SetRect( tempRect.left, tempRect.right, tempRect.top, tempRect.bottom );

			((UITab *)(*i))->SetRow( currentRow );
			((UITab *)(*j))->SetRow( 0 );			

			++j;
		}			
	}

	UIWindowVec tabs = gUIShell.ListWindowsOfRadioGroup( GetRadioGroup() );
	UIWindowVec::iterator i;		
	for ( i = tabs.begin(); i != tabs.end(); ++i )
	{
		UIWindowVec::iterator k;
		UIWindowVec children = (*i)->GetChildren();			

		for ( k = children.begin(); k != children.end(); ++k )
		{
			(*k)->AdjustChildren();
		}
	}
}