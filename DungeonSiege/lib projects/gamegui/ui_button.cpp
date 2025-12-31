/**************************************************************************

Filename		: ui_button.cpp

Description		: Control for the standard click button

Creation Date	: 9/19/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_button.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"
#include "ui_textbox.h"
#include "ui_combobox.h"


/**************************************************************************

Function		: UIButton()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIButton::UIButton( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: UIWindow( fhWindow, parent_interface, parent )
, m_buttondown( false )
, m_rbuttondown( false )
, m_bStretchCtrl( false )	
, m_bRepeater( false )
, m_fRepeatRate( 1.0f )
, m_fRepeatCurrent( 0 )
, m_fFirstRepeat( 1.0f )
, m_bAllowUserPress( true )
, m_bDrawRollover( false )
, m_dwRolloverColor( 0x5500ff00 )
, m_bDisabled( false )
, m_dwDisableColor( 0x00ffffff )
, m_bTextUp( true )
, m_bLeftRightButton( false )
, m_bIgnoreItems( true )
, m_dwSelectedColor( 0xffffffff )
, m_bPressed( false )
, m_button_center_tex( 0 )
, m_button_left_tex( 0 )
, m_button_right_tex( 0 )
{	
	SetType( UI_TYPE_BUTTON );
	SetInputType( TYPE_INPUT_ALL );
	
	fhWindow.Get( "repeater", m_bRepeater );
	fhWindow.Get( "repeat_rate", m_fRepeatRate );
	fhWindow.Get( "allow_user_press", m_bAllowUserPress );
	fhWindow.Get( "draw_rollover", m_bDrawRollover );
	fhWindow.Get( "rollover_color", m_dwRolloverColor );
	fhWindow.Get( "disable_color", m_dwDisableColor );
	fhWindow.Get( "leftright_button", m_bLeftRightButton );
	fhWindow.Get( "ignore_items", m_bIgnoreItems );

	bool bCommon = false;
	if ( fhWindow.Get( "common_control", bCommon ) ) 
	{
		if ( bCommon ) 
		{
			gpstring sTemplate;
			fhWindow.Get( "common_template", sTemplate );
			CreateCommonCtrl( sTemplate );		
		}
	}	
}
UIButton::UIButton()
: UIWindow()
, m_buttondown( false )
, m_rbuttondown( false )
, m_bStretchCtrl( false )	
, m_bRepeater( false )
, m_fRepeatRate( 1.0f )
, m_fRepeatCurrent( 0 )
, m_fFirstRepeat( 1.0f )
, m_bAllowUserPress( true )
, m_bDrawRollover( false )
, m_dwRolloverColor( 0x5500ff00 )
, m_bDisabled( false )
, m_dwDisableColor( 0x00ffffff )
, m_bTextUp( true )
, m_bLeftRightButton( false )
, m_bIgnoreItems( true )
, m_dwSelectedColor( 0xffffffff )
, m_bPressed( false )
, m_button_center_tex( 0 )
, m_button_left_tex( 0 )
, m_button_right_tex( 0 )
{
	SetType( UI_TYPE_BUTTON );
	SetInputType( TYPE_INPUT_ALL );
}


/**************************************************************************

Function		: ~UIButton()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIButton::~UIButton()
{
	if ( m_button_center_tex != 0 )
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
bool UIButton::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( !m_bDisabled )
	{
		if ( UIWindow::ProcessAction( action, parameter ) == true ) 
		{
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
bool UIButton::ProcessMessage( UIMessage & msg )
{
	UIWindow::ProcessMessage( msg );

	if ( m_bDisabled && msg.GetCode() != MSG_ENABLE )
	{
		return false;
	}

	if ( !m_bIgnoreItems && gUIShell.GetItemActive() )
	{
		return false;
	}

	if ( GetAllowUserPress() )
	{
		switch ( msg.GetCode() ) 
		{
		case MSG_INVISIBLE:
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_ROLLOFF ), this );
			}
			break;
		case MSG_LBUTTONDOWN:
			{
				m_buttondown = true;
				MoveGroupTextDown();
				OnButtonDown();

				m_fFirstRepeat = 2.0f;
			}
			break;
		case MSG_LBUTTONUP:
			{
				if ( m_buttondown == true ) 
				{
					MoveGroupTextUp();		
					gUIMessenger.SendUIMessage( UIMessage( MSG_BUTTONPRESS ), this );			
				}
				m_buttondown = false;
				OnButtonUp();

				if ( !GetHidden() && HitDetect( gUIShell.GetMouseX(), gUIShell.GetMouseY() ) && !gUIShell.IsModalInterfaceActive( GetInterfaceParent() ) )
				{
					gUIMessenger.SendUIMessage( UIMessage( MSG_ROLLOVER ), this );
					return false;
				} 
			}
			break;
		case MSG_GLOBALLBUTTONUP:
			{
				if ( m_buttondown == true )
				{
					gUIMessenger.SendUIMessage( UIMessage( MSG_GLOBALLBUTTONUPOFF ), this );
				}
				m_buttondown = false;			
			}
			break;
		case MSG_RBUTTONDOWN:
			{
				if ( m_bLeftRightButton )
				{
					gUIMessenger.SendUIMessage( UIMessage( MSG_LBUTTONDOWN ), this );
					break;
				}
				m_rbuttondown = true;
			}
			break;
		case MSG_RBUTTONUP:
			{
				if ( m_bLeftRightButton )
				{
					gUIMessenger.SendUIMessage( UIMessage( MSG_LBUTTONUP ), this );
					break;
				}
				if ( m_rbuttondown == true )
				{
					gUIMessenger.SendUIMessage( UIMessage( MSG_RBUTTONPRESS ), this );
				}

				m_rbuttondown = false;
			}
			break;
		case MSG_GLOBALRBUTTONUP:
			{
				if ( m_bLeftRightButton )
				{
					gUIMessenger.SendUIMessage( UIMessage( MSG_GLOBALLBUTTONUP ), this );
					break;
				}
				m_rbuttondown = false;
			}
			break;
		case MSG_ROLLOVER:
			{
				OnButtonRollover();
				if ( m_buttondown == true ) 
				{
					if ( GetToolTipBox() )
					{
						GetToolTipBox()->SetText( L"" );
					}
					gUIMessenger.SendUIMessage( UIMessage( MSG_LBUTTONDOWN ), this );
					return false;
				}
				if ( gUIShell.GetLButtonDown() == true ) 
				{		
					if ( GetToolTipBox() )
					{
						GetToolTipBox()->SetText( L"" );
					}
					return false;
				}						
			}
			break;
		case MSG_ROLLOFF:
			{
				if ( m_buttondown == true ) 
				{
					MoveGroupTextUp();				
				}
				OnButtonRolloff();
			}
			break;		
		}
	}
	
	switch ( msg.GetCode() ) 
	{
	case MSG_ENABLE:
		{
			SetAllowUserPress( true );
			m_bDisabled = false;
		}
		break;
	case MSG_DISABLE:
		{
			SetAllowUserPress( false );
			m_bDisabled = true;
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


void UIButton::DisableButton()
{
	SetAllowUserPress( false );
	m_bDisabled = true;
	gUIMessenger.SendUIMessage( UIMessage( MSG_DISABLE ), this );
	
	if ( GetIsCommonControl() )
	{
		if ( GetCommonTemplate().same_no_case( "button_replay" ) )
		{
			if ( GetTextureIndex() != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( GetTextureIndex() );
			}			
			SetTextureIndex( gUIShell.GetCommonArt( GetCommonTemplate() + "_dis" ) );
		}
	}
}


void UIButton::EnableButton()
{
	SetAllowUserPress( true );
	m_bDisabled = false;
	gUIMessenger.SendUIMessage( UIMessage( MSG_ENABLE ), this );

	if ( GetIsCommonControl() )
	{
		if ( GetCommonTemplate().same_no_case( "button_replay" ) )
		{
			if ( GetTextureIndex() != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( GetTextureIndex() );
			}			
			SetTextureIndex( gUIShell.GetCommonArt( GetCommonTemplate() + "_up" ) );
		}
	}
}


void UIButton::SetVisible( bool bVisible )
{
	if ( !bVisible && GetVisible() )
	{
		gUIMessenger.SendUIMessage( UIMessage( MSG_ROLLOFF ), this );
	}
	else if ( GetRolloverKey().size() != 0 && HitDetect( gUIShell.GetMouseX(), gUIShell.GetMouseY() ) )
	{
		gUIShell.SetRolloverName( GetRolloverKey() );
	}

	UIWindow::SetVisible( bVisible );	
}


void UIButton::MoveGroupTextDown()
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


void UIButton::MoveGroupTextUp()
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


void UIButton::CreateCommonCtrl( gpstring sName, gpstring sTexturePrefix, UIWindow *pParent, eCommonButtons buttonType, bool bSetRect )
{	
	if ( GetTextureIndex() != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( GetTextureIndex() );
	}

	gpstring sTextureName = sTexturePrefix;
	sTextureName =	sTexturePrefix;
	sTextureName += "_up";
	SetTextureIndex( gUIShell.GetCommonArt( sTextureName ) );
	SetHasTexture( true );

	SetIsCommonControl( true );
	SetCommonTemplate( sTexturePrefix );

	{
		UI_EVENT ue;
		ue.msg_code = MSG_LBUTTONUP;
		EVENT_ACTION ea;
		ea.action = ACTION_LOADTEXTURE;
		ea.parameter = sTextureName;
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );
	}
	{
		UI_EVENT ue;
		ue.msg_code = MSG_ROLLOFF;
		EVENT_ACTION ea;
		ea.action = ACTION_LOADTEXTURE;
		ea.parameter = sTextureName;
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );
	}
	{
		UI_EVENT ue;
		ue.msg_code = MSG_ENABLE;
		EVENT_ACTION ea;
		ea.action = ACTION_LOADTEXTURE;
		ea.parameter = sTextureName;
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );		
	}

	sTextureName = sTexturePrefix;
	sTextureName += "_down";

	{
		UI_EVENT ue;
		ue.msg_code = MSG_LBUTTONDOWN;
		EVENT_ACTION ea;
		ea.action = ACTION_LOADTEXTURE;
		ea.parameter = sTextureName;
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );
	}

	sTextureName = sTexturePrefix;
	sTextureName += "_hov";

	{
		UI_EVENT ue;
		ue.msg_code = MSG_ROLLOVER;
		EVENT_ACTION ea;
		ea.action = ACTION_LOADTEXTURE;
		ea.parameter = sTextureName;
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );
	}

	sTextureName = sTexturePrefix;
	sTextureName += "_dis";

	{
		UI_EVENT ue;
		ue.msg_code = MSG_DISABLE;
		EVENT_ACTION ea;
		ea.action = ACTION_LOADTEXTURE;
		ea.parameter = sTextureName;
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );		
	}


	{
		UI_EVENT ue;
		ue.msg_code = MSG_BUTTONPRESS;
		EVENT_ACTION ea;
		ea.action = ACTION_PARENTMESSAGE;
		
		if ( buttonType == SLIDER_INCREMENT ) {
			ea.parameter = "increment_slider";
			m_bRepeater = true;
			m_fRepeatRate = 0.20f;
		}
		else if ( buttonType == SLIDER_DECREMENT ) {
			ea.parameter = "decrement_slider";
			m_bRepeater = true;
			m_fRepeatRate = 0.20f;
		}
		else if ( buttonType == X_EXIT )
		{
			ea.parameter = "onexit";
		}
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );
	}	
	
	if ( bSetRect )
	{
		SetRect(	0, gUIShell.GetRenderer().GetTextureWidth( GetTextureIndex() ), 
					0, gUIShell.GetRenderer().GetTextureHeight( GetTextureIndex() ) );
	}
	
	SetParentWindow( pParent );
	SetName( sName );
}


void UIButton::CreateCommonCtrl( gpstring sTemplate )
{
	if ( sTemplate.empty() ) 
	{
		m_bStretchCtrl = true;	
		SetIsCommonControl( true );
		SetCommonTemplate( "" );
		m_sTemplatePrefix = "button2_";

		if ( m_button_center_tex != 0 )
		{
			gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
		}
		m_button_center_tex	= gUIShell.GetCommonArt( "button2_center_up" );

		if ( m_button_left_tex != 0 )
		{
			gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
		}
		m_button_left_tex	= gUIShell.GetCommonArt( "button2_left_up" );

		if ( m_button_right_tex != 0 )
		{			
			gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
		}
		m_button_right_tex	= gUIShell.GetCommonArt( "button2_right_up" );
	}	
	else { 
		SetIsCommonControl( true );		
		if ( sTemplate.same_no_case( "up" ) )
		{			
			SetCommonTemplate( "up" );
			CreateCommonCtrl( GetName(), "button_up", GetParentWindow(), SLIDER_NONE, false );
		}
		else if ( sTemplate.same_no_case( "down" ) )
		{
			SetCommonTemplate( "down" );			
			CreateCommonCtrl( GetName(), "button_down", GetParentWindow(), SLIDER_NONE, false );
		}
		else if ( sTemplate.same_no_case( "replay" ) )
		{
			SetCommonTemplate( "replay" );
			CreateCommonCtrl( GetName(), "button_replay", GetParentWindow(), SLIDER_NONE, false );
		}
		else if ( sTemplate.same_no_case( "dbldwn" ) )
		{
			SetCommonTemplate( "dbldwn" );
			CreateCommonCtrl( GetName(), "button_dbldwn", GetParentWindow(), SLIDER_NONE, false );
		}
		else if ( sTemplate.same_no_case( "dblup" ) )
		{
			SetCommonTemplate( "dblup" );
			CreateCommonCtrl( GetName(), "button_dblup", GetParentWindow(), SLIDER_NONE, false );
		}
		else if ( sTemplate.same_no_case( "x" ) )
		{
			SetCommonTemplate( "x" );
			CreateCommonCtrl( GetName(), "x_button", GetParentWindow(), SLIDER_NONE, false );			
		}
		else if ( sTemplate.same_no_case( "min" ) )
		{
			SetCommonTemplate( "min" );
			CreateCommonCtrl( GetName(), "min_button", GetParentWindow(), SLIDER_NONE, false );			
		}
		else if ( sTemplate.same_no_case( "mid" ) )
		{
			SetCommonTemplate( "mid" );
			CreateCommonCtrl( GetName(), "mid_button", GetParentWindow(), SLIDER_NONE, false );			
		}
		else if ( sTemplate.same_no_case( "max" ) )
		{
			SetCommonTemplate( "max" );
			CreateCommonCtrl( GetName(), "max_button", GetParentWindow(), SLIDER_NONE, false );			
		}
		else if ( sTemplate.same_no_case( "left" ) )
		{
			SetCommonTemplate( "left" );
			CreateCommonCtrl( GetName(), "button_left", GetParentWindow(), SLIDER_NONE, false );			
		}
		else if ( sTemplate.same_no_case( "right" ) )
		{
			SetCommonTemplate( "right" );
			CreateCommonCtrl( GetName(), "button_right", GetParentWindow(), SLIDER_NONE, false );			
		}
		else if ( sTemplate.same_no_case( "button_3" ) )
		{
			m_bStretchCtrl = true;	
			SetCommonTemplate( "button_3" );
			m_sTemplatePrefix = "button3_";
			m_button_center_tex	= gUIShell.GetCommonArt( "button3_center_up" );
			m_button_left_tex	= gUIShell.GetCommonArt( "button3_left_up" );
			m_button_right_tex	= gUIShell.GetCommonArt( "button3_right_up" );			
		}
		else if ( sTemplate.same_no_case( "button_4" ) )
		{
			m_bStretchCtrl = true;	
			SetCommonTemplate( "button_4" );
			m_sTemplatePrefix = "button4_";
			m_button_center_tex	= gUIShell.GetCommonArt( "button4_center_up" );
			m_button_left_tex	= gUIShell.GetCommonArt( "button4_left_up" );
			m_button_right_tex	= gUIShell.GetCommonArt( "button4_right_up" );			
		}
		else if ( sTemplate.same_no_case( "button_5" ) )
		{
			m_bStretchCtrl = true;	
			SetCommonTemplate( "button_5" );
			m_sTemplatePrefix = "button5_";
			m_button_center_tex	= gUIShell.GetCommonArt( "button5_center_up" );
			m_button_left_tex	= gUIShell.GetCommonArt( "button5_left_up" );
			m_button_right_tex	= gUIShell.GetCommonArt( "button5_right_up" );			
		}
		else if ( sTemplate.same_no_case( "smallx" ) )
		{
			SetCommonTemplate( "smallx" );
			CreateCommonCtrl( GetName(), "button_smallx", GetParentWindow(), SLIDER_NONE, false );
		}
		else if ( sTemplate.same_no_case( "button_stop" ) )
		{
			SetCommonTemplate( "button_stop" );
			CreateCommonCtrl( GetName(), "button_stop", GetParentWindow(), SLIDER_NONE, false );
		}
		else if ( sTemplate.same_no_case( "button_inset" ) )
		{
			m_bStretchCtrl = true;
			SetCommonTemplate( "button_inset" );
			m_sTemplatePrefix = "buttoninset_";
			m_button_center_tex	= gUIShell.GetCommonArt( "buttoninset_center_up" );
			m_button_left_tex	= gUIShell.GetCommonArt( "buttoninset_left_up" );
			m_button_right_tex	= gUIShell.GetCommonArt( "buttoninset_right_up" );			
		}
		else if ( sTemplate.same_no_case( "button_subtab" ) )
		{
			m_bStretchCtrl = true;
			SetCommonTemplate( "button_subtab" );
			m_sTemplatePrefix = "subtab_";
			m_button_center_tex	= gUIShell.GetCommonArt( "subtab_center_up" );
			m_button_left_tex	= gUIShell.GetCommonArt( "subtab_left_up" );
			m_button_right_tex	= gUIShell.GetCommonArt( "subtab_right_up" );			
		}
	}
}


void UIButton::Draw()
{
	if ( m_bDisabled && GetVisible() && HasTexture() && GetAlpha() != 0.0f )
	{
		UIWindow::DrawComponent( GetRect(), GetTextureIndex(), GetAlpha(), GetUVRect(), GetTiledTexture(), m_dwDisableColor );
	}
	else
	{
		UIWindow::Draw();
	}

	if ( GetVisible() && m_bStretchCtrl ) 
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

		DrawComponent( component_rect, m_button_center_tex, GetAlpha(), uv_component_rect, true /*m_bDisabled ? m_dwDisableColor : 0x00ffffff*/ );		

		if ( m_bDisabled || !IsEnabled() )
		{
			gUIShell.DrawColorBox( GetRect(), m_dwDisableColor );
		}
	}

	if ( GetVisible() && m_bDrawRollover && GetRollover() )
	{
		gUIShell.DrawColorBox( GetRect(), m_dwRolloverColor );
	}
}


void UIButton::SetRect( int left, int right, int top, int bottom, bool bAnimation )
{
	UIWindow::SetRect( left, right, top, bottom, bAnimation );

	for ( UIWindowVec::iterator i = GetChildren().begin(); i != GetChildren().end(); ++i )
	{
		if ( (*i)->GetType() == UI_TYPE_TEXT )
		{
			((UIText *)(*i))->SetRect( left, right, top, bottom, bAnimation );
		}
	}
}


void UIButton::OnButtonDown()
{
	if ( m_bStretchCtrl ) 
	{
		if ( GetCommonTemplate().empty() || GetCommonTemplate().same_no_case( "button_2" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button2_center_down" );
			
			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button2_left_down" );
			
			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button2_right_down" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_3" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button3_center_down" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button3_left_down" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button3_right_down" );
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
		else if ( GetCommonTemplate().same_no_case( "button_5" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button5_center_down" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button5_left_down" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button5_right_down" );
		}	
		else if ( GetCommonTemplate().same_no_case( "button_inset" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "buttoninset_center_down" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "buttoninset_left_down" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "buttoninset_right_down" );
		}	
		else if ( GetCommonTemplate().same_no_case( "button_subtab" ) )
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


void UIButton::OnButtonUp()
{
	if ( m_bStretchCtrl ) 
	{
		if ( GetCommonTemplate().empty() || GetCommonTemplate().same_no_case( "button_2" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button2_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button2_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button2_right_up" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_3" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button3_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button3_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button3_right_up" );
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
		else if ( GetCommonTemplate().same_no_case( "button_5" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button5_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button5_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button5_right_up" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_inset" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "buttoninset_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "buttoninset_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "buttoninset_right_up" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_subtab" ) )
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


void UIButton::OnButtonRollover()
{
	if ( m_bStretchCtrl ) 
	{
		if ( GetCommonTemplate().empty() || GetCommonTemplate().same_no_case( "button_2" ) )
		{	
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button2_center_hov" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button2_left_hov" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button2_right_hov" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_3" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button3_center_hov" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button3_left_hov" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button3_right_hov" );
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
		else if ( GetCommonTemplate().same_no_case( "button_5" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button5_center_hov" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button5_left_hov" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button5_right_hov" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_inset" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "buttoninset_center_hov" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "buttoninset_left_hov" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "buttoninset_right_hov" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_subtab" ) )
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


void UIButton::OnButtonRolloff()
{
	if ( m_bPressed )
	{
		OnButtonDown();
		return;
	}

	if ( m_bStretchCtrl ) 
	{
		if ( GetCommonTemplate().empty() || GetCommonTemplate().same_no_case( "button_2" ) )
		{	
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button2_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button2_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button2_right_up" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_3" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button3_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button3_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button3_right_up" );
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
		else if ( GetCommonTemplate().same_no_case( "button_5" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button5_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button5_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button5_right_up" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_inset" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "buttoninset_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "buttoninset_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "buttoninset_right_up" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_subtab" ) )
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

#if !GP_RETAIL
void UIButton::Save( FuelHandle hSave, bool bChild )
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
#endif // !GP_RETAIL


void UIButton::Update( double seconds )
{	
	UIWindow::Update( seconds );

	if ( m_bRepeater )
	{
		if ( m_buttondown )
		{
			if ( HitDetect( gUIShell.GetMouseX(), gUIShell.GetMouseY() ) )
			{
				m_fRepeatCurrent += (float)seconds;
				
				if ( m_fRepeatCurrent >= m_fRepeatRate * m_fFirstRepeat)
				{
					gUIMessenger.SendUIMessage( UIMessage( MSG_BUTTONPRESS ), this );
					m_fRepeatCurrent = 0;
					m_fFirstRepeat = 1.0f;
				}
			}
		}
	}
}

void UIButton::SetPressedState( bool pressed )
{
	m_bPressed = pressed;
	if ( m_bPressed )
	{
		OnButtonDown();
	}
	else
	{
		OnButtonUp();
	}
}