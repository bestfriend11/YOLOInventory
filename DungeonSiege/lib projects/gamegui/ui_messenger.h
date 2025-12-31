/**************************************************************************

Filename		: ui_messenger.h

Description		: This class handles all messaging and notification
				  routines.

Creation Date	: 9/21/99

**************************************************************************/


#pragma once
#ifndef _UI_MESSENGER_H_
#define _UI_MESSENGER_H_


// Include files
#include "ui_types.h"
#include "ui_interface.h"
#include "BlindCallback.h"



class UIMessage
{
public:
	UIMessage( eUIMessage code ) { m_code = code; }

	eUIMessage	m_code;
	int			m_state;
	wchar_t		m_key;
	gpstring	m_sName;

FEX eUIMessage			GetCode () const	{ return m_code;  }
FEX int					GetState() const	{ return m_state; }
FEX int					GetKey  () const	{ return m_key;	  }
FEX const gpstring &	GetName () const	{ return m_sName; }

FEX void SetCode    ( eUIMessage msg )			{ m_code  = msg;			}
FEX void SetMsgState( int state ) 				{ m_state = state;			}
FEX void SetKey	    ( int key ) 				{ m_key   = (wchar_t)key;	}
FEX void SetName    ( const gpstring& name )	{ m_sName = name;			}
};

FEX UIMessage& MakeUIMessage( eUIMessage code );


// Forward Declarations
class UIWindow;


class Messenger : public Singleton <Messenger>
{
public:
	// Public Methods

	// Constructor
	Messenger();

	// Destructor
	~Messenger();

	// Send Messages to the UI windows
	bool SendUIMessage				( const UIMessage& msg, UIWindow *ui_window = 0			);
	bool SendUIMessageToGroup		( const UIMessage& msg, const gpstring& sGroup, UIWindow * pWindow = 0	);
	bool SendUIMessageToInterface	( const UIMessage& msg, const gpstring& sInterface		);

	//bool SendUIMessage( gpstring message, UIWindow *ui_window = 0		);
	//bool SendUIMessage( gpstring message, gpstring parent_interface, gpstring group = gpstring::EMPTY );

	// Notify callbacks
	typedef CBFunctor2< gpstring const &, UIWindow & > ui_window_callback;
	typedef std::vector<ui_window_callback> window_callback_list;
	void	RegisterCallback	( const ui_window_callback & callback );
	void	UnRegisterCallback	( const ui_window_callback & callback );
FEX	void	Notify				( const gpstring& message, UIWindow *ui_window );

	// Initialize Window List
	void	SetUIWindows		( std::vector< GUInterface * > *ui_interfaces ) { m_UIInterfaces = ui_interfaces; };

	// Callbacks
	typedef CBFunctor2< const UIMessage&, UIWindow*       > UIWindowMessageCb;
	typedef CBFunctor2< const UIMessage&, const gpstring& > UIInterfaceMessageCb;
	static void SetUIWindowMessageCb   ( UIWindowMessageCb cb )		{ ms_uiWindowMessageCb = cb; }
	static void SetUIInterfaceMessageCb( UIInterfaceMessageCb cb )	{ ms_uiInterfaceMessageCb = cb; }

FEX	bool	IsProcessingMessage() const	{  return m_bProcessingMessage;  }

private:

	// Private Member variables
	GUInterfaceVec						*m_UIInterfaces;// This list contains all the windows/interfaces of the UI
	std::vector< ui_window_callback >	m_callbacks;	// This list contains all the callbacks
	int									m_NotifyCount;	// number of notification calls active
	bool								m_bCancelNotifyPropagation;

	bool								m_bProcessingMessage;

	// For Callbacks
	static UIWindowMessageCb    ms_uiWindowMessageCb;
	static UIInterfaceMessageCb ms_uiInterfaceMessageCb;

	FUBI_SINGLETON_CLASS( Messenger, "GameGUI message router." );
};


#define gUIMessenger Messenger::GetSingleton()


#endif