//////////////////////////////////////////////////////////////////////////////
//
// File     :  UIGameConsole.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __UIGAMECONSOLE_H

enum eChatTag
{
	CHAT_PLAYERS_END = 9, // this is the max number of players + 1.

	// -1 is reserved for invalid players, so we can't use it.
	CHAT_ALL	= -2,
	CHAT_TEAM	= -3,
	CHAT_FRIEND = -4,// CHAT_FRIEND must be last!!!!!  we programmatically create enums after(below) this!!!!!
	
};

enum eChatSize
{
	CHAT_SIZE_CURRENT,
	CHAT_SIZE_MIN,
	CHAT_SIZE_MID,
	CHAT_SIZE_MAX,
};

#include "GpConsole.h"

class UIGameConsole : public Singleton <UIGameConsole>
{
public:

	UIGameConsole();
	~UIGameConsole() {};

	void Init();

	// This is the entry point were the text from the edit box first hits
	void ReceiveConsoleText( gpwstring const & sText );
	
		void SubmitChat( gpwstring const & sChat,  const char * listboxParentInterface = "game_console_mp", bool sendToAllAsIs = false );

			// If the text is formatted for chatting, then we need to send the message as such
			FUBI_EXPORT void RSChat( gpwstring const & sChat );
			FUBI_EXPORT void RCChat( gpwstring const & sChat );
			FUBI_MEMBER_SET( RCChat, +SERVER );

			FUBI_EXPORT void RSChatPlayer( gpwstring const & sChat, Player * player );
			FUBI_EXPORT void RCChatPlayer( gpwstring const & sChat, DWORD machineId );
			FUBI_MEMBER_SET( RCChatPlayer, +SERVER );		

		// If the text is formatted for a command of some sort, try to execute the command (return false if it's silent)
		bool SubmitCommand( gpstring sCommand, bool enable = true );
		
	void EnableConsoleUpdate( bool bEnable );

	bool ChatHistoryUp();
	bool ChatHistoryDown();
	bool ChatHistoryClear();
	bool ChatHistoryLock();

	void ConsoleRollover();
	void ConsoleRolloff();

	void ResizeChatWindow( eChatSize chatSize = CHAT_SIZE_CURRENT);
	void ResizeWindowTemplate( const char* windowName, const char* sizeTemplate, const char* interfaceName, int offsetx=0, int offsety=0 );

private:
	
	void Chat( gpwstring const & sChat );

#if !GP_RETAIL
	std::auto_ptr< GpConsole_command > m_gpc_disband;
#endif

	FUBI_SINGLETON_CLASS( UIGameConsole, "Controls the retail user interface." );
	SET_NO_COPYING( UIGameConsole );

	eChatSize	m_chatWindowSize;
	double		m_timeLastActive;

};


#define gUIGameConsole UIGameConsole::GetSingleton()


#endif