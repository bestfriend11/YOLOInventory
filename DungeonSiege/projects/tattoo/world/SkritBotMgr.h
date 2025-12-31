//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritBotMgr.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains <Summary>
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __SKRITBOTMGR_H
#define __SKRITBOTMGR_H

//////////////////////////////////////////////////////////////////////////////

#if GP_ENABLE_SKRITBOT

#include "SkritDefs.h"
#include "Ui_Types.h"

class UIWindow;
class UIMessage;

//////////////////////////////////////////////////////////////////////////////
// class SkritBotMgr declaration

class SkritBotMgr : public Singleton <SkritBotMgr>
{
public:
	SET_NO_INHERITED( SkritBotMgr );

	SkritBotMgr( void );
   ~SkritBotMgr( void );

	// management
FEX	bool AddSkritBot( const char* skritName );
FEX	void Reload( void );
FEX	void Unload( void );

	// events
	void HandleMessage           ( const WorldMessage& msg );
	void HandleUiAction          ( UI_ACTION action, const gpstring& parameter, UIWindow* window );
	void HandleUiWindowMessage   ( const UIMessage& msg, UIWindow* window );
	void HandleUiInterfaceMessage( const UIMessage& msg, const gpstring& interfce );
	void Update                  ( float deltaTime );
	void SysUpdate               ( float deltaTime );
	void Draw                    ( void );

private:
	struct Entry
	{
		gpstring       m_Name;
		Skrit::HObject m_Object;

		Entry( const gpstring& name, Skrit::HObject object )
			: m_Name( name ), m_Object( object )  {  }
	};

	typedef std::vector <Entry> SkritColl;

	SkritColl m_SkritColl;

	FUBI_SINGLETON_CLASS( SkritBotMgr, "Manager of all Skrit bots." );
	SET_NO_COPYING( SkritBotMgr );
};

#define gSkritBotMgr SkritBotMgr::GetSingleton()

#endif // GP_ENABLE_SKRITBOT

//////////////////////////////////////////////////////////////////////////////

#endif  // __SKRITBOTMGR_H

//////////////////////////////////////////////////////////////////////////////
