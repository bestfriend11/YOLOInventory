/**************************************************************************

Filename		: ui_messenger.cpp

Description		: This class handles all messaging and notification
				  routines.

Creation Date	: 9/21/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"

#include "ui_messenger.h"
#include "ui_window.h"
#include "ui_tab.h"


UIMessage& MakeUIMessage( eUIMessage code )
{
	static UIMessage s_Message( code );
	s_Message = UIMessage( code );
	return ( s_Message );
}


Messenger::UIWindowMessageCb    Messenger::ms_uiWindowMessageCb;
Messenger::UIInterfaceMessageCb Messenger::ms_uiInterfaceMessageCb;


/**************************************************************************

Function		: Messenger()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
Messenger::Messenger()
{
	m_NotifyCount				= 0;
	m_bCancelNotifyPropagation	= false;
	m_bProcessingMessage		= false;
}


/**************************************************************************

Function		: ~Messenger()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
Messenger::~Messenger()
{
}


/**************************************************************************

Function		: Notify()

Purpose			: Notify all callbacks

Returns			: void

**************************************************************************/
void Messenger::Notify( const gpstring& message, UIWindow *ui_window )
{
	++m_NotifyCount;

	window_callback_list::iterator i;
	for ( i = m_callbacks.begin(); i != m_callbacks.end(); ++i ) {

		(*i)( message, *ui_window );

		if ( m_bCancelNotifyPropagation )
		{
			break;
		}
	}
	--m_NotifyCount;

	if ( m_NotifyCount <= 0 )
	{
		m_bCancelNotifyPropagation	= false;
	}
}


/**************************************************************************

Function		: SendUIMessage()

Purpose			: Sends messages to windows

Returns			: bool

**************************************************************************/
bool Messenger::SendUIMessage( const UIMessage& msg, UIWindow *ui_window )
{
	bool bHandled = false;

	m_bProcessingMessage = true;

	if ( ms_uiWindowMessageCb )
	{
		ms_uiWindowMessageCb( msg, ui_window );
	}

	if ( ui_window == 0 )
	{
		UIWindowMap::iterator i;
		GUInterfaceVec::reverse_iterator j;

		// Is there any modal interfaces activated -- if so, only take care of them
		for ( j = m_UIInterfaces->rbegin(); j != m_UIInterfaces->rend(); ++j )
		{
			if ( (*j)->bModal && (*j)->bVisible && (*j)->bEnabled )
			{
				for ( i = (*j)->ui_windows.begin(); i != (*j)->ui_windows.end(); ++i )
				{
					if ( (*i).second->ReceiveMessage( msg ) == true )
					{
						bHandled = true;
					}
				}
				m_bProcessingMessage = false;
				return bHandled;
			}
		}

		// For non-modal interfaces
		GUInterfaceVec::iterator k;
		for ( k = m_UIInterfaces->begin(); k != m_UIInterfaces->end(); ++k )
		{
			if ( (*k)->bVisible && (*k)->bEnabled )
			{
				for ( i = (*k)->ui_windows.begin(); i != (*k)->ui_windows.end(); ++i )
				{
					if ( (*i).second->ReceiveMessage( msg ) )
					{
						bHandled = true;
					}
				}
			}
		}
	}
	else
	{
		if ( ui_window->ReceiveMessage( msg ) == true )
		{
			bHandled = true;
		}
	}

	m_bProcessingMessage = false;
	return bHandled;
}


bool Messenger::SendUIMessageToGroup( const UIMessage& msg, const gpstring& sGroup, UIWindow * pWindow )
{
	bool bHandled = false;

	m_bProcessingMessage = true;

	UIWindowMap::iterator i;
	GUInterfaceVec::reverse_iterator j;

	// For non-modal interfaces
	GUInterfaceVec::iterator k;
	for ( k = m_UIInterfaces->begin(); k != m_UIInterfaces->end(); ++k )
	{
		for ( i = (*k)->ui_windows.begin(); i != (*k)->ui_windows.end(); ++i ) {
			if ( (!sGroup.empty()) && (sGroup.same_no_case( (*i).second->GetGroup())) ) 
			{
				if ( (*i).second->ReceiveMessage( msg ) == true ) 
				{
					bHandled = true;
				}
			}
			else if ( ((*i).second->GetType() == UI_TYPE_RADIOBUTTON) && (((UIRadioButton *)(*i).second)->GetRadioGroup().same_no_case( sGroup )) ) 
			{
				if ( (*i).second != pWindow && (*i).second->ReceiveMessage( msg ) == true ) 
				{
					bHandled = true;
				}
			}
			else if ( ((*i).second->GetType() == UI_TYPE_TAB) && (((UITab *)(*i).second)->GetRadioGroup().same_no_case( sGroup )) ) 
			{
				if ( (*i).second != pWindow && (*i).second->ReceiveMessage( msg ) == true ) 
				{
					bHandled = true;
				}
			}
			else if ( sGroup.empty() )
			{
				if ( (*i).second != pWindow && (*i).second->ReceiveMessage( msg ) == true )
				{
					bHandled = true;
				}
			}
		}
	}

	m_bProcessingMessage = false;
	return bHandled;
}


bool Messenger::SendUIMessageToInterface( const UIMessage& msg, const gpstring& sInterface )
{
	bool bHandled = false;

	m_bProcessingMessage = true;

	if ( ms_uiInterfaceMessageCb )
	{
		ms_uiInterfaceMessageCb( msg, sInterface );
	}

	UIWindowMap::iterator i;
	GUInterfaceVec::reverse_iterator j;

	// For non-modal interfaces
	GUInterfaceVec::iterator k;
	for ( k = m_UIInterfaces->begin(); k != m_UIInterfaces->end(); ++k )
	{
		if ( (*k)->name.same_no_case( sInterface ) )
		{
			for ( i = (*k)->ui_windows.begin(); i != (*k)->ui_windows.end(); ++i ) {
				if ( (*i).second->ReceiveMessage( msg ) == true ) {
					bHandled = true;
				}
			}
		}
	}

	m_bProcessingMessage = false;
	return bHandled;
}



/**************************************************************************

Function		: RegisterCallback()

Purpose			: Register a callback with the interface

Returns			: void

**************************************************************************/
void Messenger::RegisterCallback( const ui_window_callback & callback )
{
	if ( m_NotifyCount > 0 )
	{
		m_bCancelNotifyPropagation = true;
	}
	
	std::vector< ui_window_callback >::iterator i;
	for ( i = m_callbacks.begin(); i != m_callbacks.end(); ++i )
	{
		if ( (*i).getFunc() == callback.getFunc() )
		{
			gpassertm( 0, "Registering a callback that already exists in gUIMessenger." );
			break;
		}
	}

	m_callbacks.push_back( callback );
}


void Messenger::UnRegisterCallback( const ui_window_callback & callback )
{
	std::vector< ui_window_callback >::iterator i;
	for ( i = m_callbacks.begin(); i != m_callbacks.end(); ++i )
	{
		if ( (*i).getFunc() == callback.getFunc() )
		{
			if ( m_NotifyCount > 0 )
			{
				m_bCancelNotifyPropagation = true;
			}

			m_callbacks.erase( i );
			return;
		}
	}
}
