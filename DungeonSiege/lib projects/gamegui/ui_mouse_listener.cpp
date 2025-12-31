/**************************************************************************

Filename		: ui_mouse_listener.cpp

Description		: Control for the mouse listener, see .h for details.

Creation Date	: 4/27/00

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_window.h"
#include "ui_mouse_listener.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"


/**************************************************************************

Function		: UIMouseListener()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIMouseListener::UIMouseListener( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: UIWindow( fhWindow, parent_interface, parent )
, m_buttondown( false )
, m_rbuttondown( false )
, m_delta_x( 0 )
, m_delta_y( 0 )
, m_start_x( 0 )
, m_start_y( 0 )
{	
	SetType( UI_TYPE_MOUSE_LISTENER );
	SetInputType( TYPE_INPUT_ALL );	
}
UIMouseListener::UIMouseListener()
: UIWindow()
, m_buttondown( false )
, m_rbuttondown( false )
, m_delta_x( 0 )
, m_delta_y( 0 )
, m_start_x( 0 )
, m_start_y( 0 )
{	
	SetType( UI_TYPE_MOUSE_LISTENER );
	SetInputType( TYPE_INPUT_ALL );	
}


/**************************************************************************

Function		: ~UIMouseListener()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIMouseListener::~UIMouseListener()
{
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UIMouseListener::ProcessAction( UI_ACTION action, gpstring parameter )
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
bool UIMouseListener::ProcessMessage( UIMessage & msg )
{
	switch ( msg.GetCode() ) 
	{
	case MSG_LBUTTONDOWN:
		{
			m_buttondown = true;
			m_start_x = GetMouseX();
			m_start_y = GetMouseY();
			gUIMessenger.Notify( "listener_lbutton_down", this );
			gUIMessenger.Notify( "listener_pressed", this );
		}
		break;
	case MSG_LBUTTONUP:
		{
			gUIMessenger.Notify( "listener_lbutton_up", this );		
			if ( m_buttondown )
			{
				gUIMessenger.Notify( "listener_pressed", this );
			}			
			m_buttondown = false;
		}
		break;
	case MSG_GLOBALLBUTTONUP:
		{
			m_buttondown = false;
		}
		break;
	case MSG_RBUTTONDOWN:
		{
			m_start_x = GetMouseX();
			m_start_y = GetMouseY();
			m_rbuttondown = true;
			gUIMessenger.Notify( "listener_rbutton_down", this );
		}
		break;
	case MSG_RBUTTONUP:
		{
			m_rbuttondown = false;
			gUIMessenger.Notify( "listener_rbutton_up", this );
		}
		break;
	case MSG_GLOBALRBUTTONUP:
		{
			m_rbuttondown = false;
		}
		break;
	case MSG_ROLLOVER:
		{
			gUIMessenger.Notify( "listener_rollover", this );
		}
		break;
	case MSG_ROLLOFF:
		{
			gUIMessenger.Notify( "listener_rolloff", this );
		}
		break;
	}

	return true;
}



void UIMouseListener::SetMousePos( unsigned int x, unsigned int y )
{
	//m_delta_x = x - m_start_x;
	//m_delta_y = y - m_start_y;
	m_delta_x = x - GetMouseX();
	m_delta_y = y - GetMouseY();
	UIWindow::SetMousePos( x, y );
	if ( m_buttondown || m_rbuttondown ) {
		gUIMessenger.Notify( "listener_mouse_move", this );		
	}
}


#if !GP_RETAIL
void UIMouseListener::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "listener" );
		}
	}
}
#endif