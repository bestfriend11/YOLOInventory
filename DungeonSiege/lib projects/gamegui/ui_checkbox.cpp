/**************************************************************************

Filename		: ui_checkbox.cpp

Description		: Control for the checkbox

Creation Date	: 10/4/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_window.h"
#include "ui_checkbox.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"
#include "ui_shell.h"

/**************************************************************************

Function		: UICheckbox()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UICheckbox::UICheckbox(	FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: UIWindow( fhWindow, parent_interface, parent )
, m_buttondown( false )
, m_checked( false )
, m_check_texture( 0 )
, m_bAllowUserPress( true )
{	
	SetType( UI_TYPE_CHECKBOX );	
	SetInputType( TYPE_INPUT_ALL );

	gpstring sTexture;
	fhWindow.Get( "check_texture", sTexture );
	if ( sTexture.empty() == false )
	{
		LoadCheckTexture( sTexture );
	}

	fhWindow.Get( "allow_user_press", m_bAllowUserPress );

	gpstring sRect;
	if ( fhWindow.Get( "check_rect", sRect ) )
	{
		int value_int0 = 0;
		int value_int1 = 0;
		int value_int2 = 0;
		int value_int3 = 0;

		// Set the window rectangle
		stringtool::GetDelimitedValue( sRect, ',', 0, value_int0 );
		stringtool::GetDelimitedValue( sRect, ',', 1, value_int1 );
		stringtool::GetDelimitedValue( sRect, ',', 2, value_int2 );
		stringtool::GetDelimitedValue( sRect, ',', 3, value_int3 );
		m_check_rect.left	= value_int0;
		m_check_rect.top	= value_int1;
		m_check_rect.right	= value_int2;
		m_check_rect.bottom = value_int3;
	}
	else
	{
		m_check_rect = GetRect();
	}

}
UICheckbox::UICheckbox()
: UIWindow()
, m_buttondown( false )
, m_checked( false )
, m_check_texture( 0 )
, m_bAllowUserPress( true )
{	
	SetType( UI_TYPE_CHECKBOX );	
	SetInputType( TYPE_INPUT_ALL );	
}


/**************************************************************************

Function		: ~UICheckbox()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UICheckbox::~UICheckbox()
{
	if ( m_check_texture != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_check_texture );
	}
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UICheckbox::ProcessAction( UI_ACTION action, gpstring parameter )
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
bool UICheckbox::ProcessMessage( UIMessage & msg )
{	
	UIWindow::ProcessMessage( msg );

	if ( GetAllowUserPress() )
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
				if ( m_buttondown == true ) 
				{
					m_checked		= !m_checked;
					if ( m_checked == true ) 
					{
						gUIMessenger.SendUIMessage( UIMessage( MSG_CHECK ), this );
					}
					else 
					{
						gUIMessenger.SendUIMessage( UIMessage( MSG_UNCHECK ), this );
					}

					gUIMessenger.SendUIMessage( UIMessage( MSG_BUTTONPRESS ), this );
				}
				m_buttondown	= false;
			}

			if ( !GetHidden() && HitDetect( gUIShell.GetMouseX(), gUIShell.GetMouseY() ) )
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_ROLLOVER ), this );
				return false;
			} 

			break;
		case MSG_GLOBALLBUTTONUP:
			{
				m_buttondown = false;
			}
			break;
		case MSG_ROLLOVER:
			{
				if ( m_buttondown == true ) {
					gUIMessenger.SendUIMessage( UIMessage( MSG_LBUTTONDOWN ), this );
					return false;
				}
				if ( gUIShell.GetLButtonDown() == true ) {
					return false;
				}
			}
			break;
		}	
	}

	// Preprocess message
	if ( GetState() )
	{
		switch ( msg.GetCode() )
		{
		case MSG_LBUTTONDOWN:
			msg.m_code = MSG_LBUTTONDOWNCHECKED;
			break;
		case MSG_LBUTTONUP:
			msg.m_code = MSG_LBUTTONUPCHECKED;
			break;
		case MSG_ROLLOVER:
			msg.m_code = MSG_ROLLOVERCHECKED;
			break;
		case MSG_ROLLOFF:
			msg.m_code = MSG_ROLLOFFCHECKED;
			break;
		}
	}

	return true;
}


void UICheckbox::SetState( bool bChecked )
{
	if ( m_checked && !bChecked )
	{
		m_checked = bChecked;
		gUIMessenger.SendUIMessage( UIMessage( MSG_UNCHECK ), this );
	}
	else if ( !m_checked && bChecked )
	{
		m_checked = bChecked;
		gUIMessenger.SendUIMessage( UIMessage( MSG_CHECK ), this );
	}
}


void UICheckbox::LoadCheckTexture( const gpstring& texture )
{
	if ( m_check_texture )
	{
		gUIShell.GetRenderer().DestroyTexture( m_check_texture );
	}
	m_check_texture = gUITextureManager.LoadTexture( texture, GetInterfaceParent() );
}


void UICheckbox::Draw()
{
	UIWindow::Draw();
	
	if ( GetVisible() )
	{
		UIWindowVec children = GetChildren();
		UIWindowVec::iterator i;
		for ( i = children.begin(); i != children.end(); ++i ) 
		{
			if ( (*i)->GetType() == UI_TYPE_WINDOW )
			{
				(*i)->SetVisible( m_checked );
			}
		}
	}
}


#if !GP_RETAIL
void UICheckbox::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "checkbox" );
		}
	}
}
#endif