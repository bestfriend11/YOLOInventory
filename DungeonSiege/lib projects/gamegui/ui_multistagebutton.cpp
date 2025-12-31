/**************************************************************************

Filename		: ui_multistagebutton.cpp

Description		: Control for multi-stage button

Creation Date	: 10/5/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_window.h"
#include "ui_multistagebutton.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"


/**************************************************************************

Function		: UIMultiStageButton()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIMultiStageButton::UIMultiStageButton( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: UIWindow( fhWindow, parent_interface, parent )
{	
	SetType( UI_TYPE_MULTISTAGEBUTTON );
	SetInputType( TYPE_INPUT_ALL );
	m_buttondown = false;
	SetState( 0 );
	SetNumStates( 0 );
}
UIMultiStageButton::UIMultiStageButton()
: UIWindow()
{	
	SetType( UI_TYPE_MULTISTAGEBUTTON );
	SetInputType( TYPE_INPUT_ALL );
	m_buttondown = false;
	SetState( 0 );
	SetNumStates( 0 );
}


/**************************************************************************

Function		: ~UIMultiStageButton()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIMultiStageButton::~UIMultiStageButton()
{
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UIMultiStageButton::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true ) {
		return true;
	}
	else if ( action == ACTION_SETNUMSTATES ) {
		SetNumStates( atoi( parameter.c_str() ) );
		return true;
	}
	return false;
}


/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: bool

**************************************************************************/
bool UIMultiStageButton::ProcessMessage( UIMessage & msg )
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
				if ( GetState() < (GetNumStates()-1) ) {
					SetState( GetState() + 1 );
				}
				else {
					SetState( 0 );
				}
				UIMessage state_msg( MSG_STATECHANGE );
				state_msg.m_state = GetState();				
				gUIMessenger.SendUIMessage( state_msg, this );			
			}
			m_buttondown = false;
		}
		break;
	case MSG_GLOBALLBUTTONUP:
		{
			m_buttondown = false;
		}
		break;
	case MSG_ROLLOVER:
		{
			if ( m_buttondown == true ) 
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_LBUTTONDOWN ), this );
			}
		}
		break;
	}
	
	return true;
}


#if !GP_RETAIL
void UIMultiStageButton::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "button_multistage" );
		}
	}
}
#endif