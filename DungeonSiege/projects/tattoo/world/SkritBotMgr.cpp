//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritBotMgr.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"

#if GP_ENABLE_SKRITBOT

#include "SkritBotMgr.h"
#include "SkritObject.h"
#include "Ui_Window.h"
#include "Ui_Messenger.h"

//////////////////////////////////////////////////////////////////////////////
// class SkritBotMgr implementation

DECLARE_EVENT
{
	SKRIT_IMPORT void OnBotHandleMessage( Skrit::HObject skrit, eWorldEvent, const WorldMessage& )
		{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnBotHandleMessage, skrit );  }
	SKRIT_IMPORT void OnBotHandleUiAction( Skrit::HObject skrit, UI_ACTION, const gpstring&, UIWindow* )
		{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnBotHandleUiAction, skrit );  }
	SKRIT_IMPORT void OnBotHandleUiWindowMessage( Skrit::HObject skrit, eUIMessage, const UIMessage&, UIWindow* )
		{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnBotHandleUiWindowMessage, skrit );  }
	SKRIT_IMPORT void OnBotHandleUiInterfaceMessage( Skrit::HObject skrit, eUIMessage, const UIMessage&, const gpstring& )
		{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnBotHandleUiInterfaceMessage, skrit );  }
	SKRIT_IMPORT void OnBotUpdate( Skrit::HObject skrit, float )
		{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnBotUpdate, skrit );  }
	SKRIT_IMPORT void OnBotSysUpdate( Skrit::HObject skrit, float )
		{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnBotSysUpdate, skrit );  }
	SKRIT_IMPORT void OnBotDraw( Skrit::HObject skrit )
		{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnBotDraw, skrit );  }
}

SkritBotMgr :: SkritBotMgr( void )
{
	UIWindow ::SetUIActionCb          ( makeFunctor( *this, &SkritBotMgr::HandleUiAction           ) );
	Messenger::SetUIWindowMessageCb   ( makeFunctor( *this, &SkritBotMgr::HandleUiWindowMessage    ) );
	Messenger::SetUIInterfaceMessageCb( makeFunctor( *this, &SkritBotMgr::HandleUiInterfaceMessage ) );
}

SkritBotMgr :: ~SkritBotMgr( void )
{
	UIWindow ::SetUIActionCb          ( NULL );
	Messenger::SetUIWindowMessageCb   ( NULL );
	Messenger::SetUIInterfaceMessageCb( NULL );

	SkritColl::iterator i, ibegin = m_SkritColl.begin(), iend = m_SkritColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		i->m_Object.ReleaseSkrit();
	}
}

bool SkritBotMgr :: AddSkritBot( const char* skritName )
{
	gpassert( skritName != NULL );

	// $ early bailout
	if ( *skritName == '\0' )
	{
		return ( false );
	}

	// build skrit
	Skrit::GetReq req( skritName );
	req.SetPersist( false );
	Skrit::HObject object( req );
	if ( object )
	{
		m_SkritColl.push_back( Entry( skritName, object ) );
	}

	return ( !object.IsNull() );
}

void SkritBotMgr :: Reload( void )
{
	for ( SkritColl::iterator i = m_SkritColl.begin() ; i != m_SkritColl.end() ; )
	{
		// release old
		i->m_Object.ReleaseSkrit();

		// create new
		Skrit::GetReq req( i->m_Name );
		req.SetPersist( false );
		if ( i->m_Object.CreateSkrit( req ) )
		{
			++i;
		}
		else
		{
			i = m_SkritColl.erase( i );
		}
	}
}

void SkritBotMgr :: Unload( void )
{
	for ( SkritColl::iterator i = m_SkritColl.begin() ; i != m_SkritColl.end() ; )
	{
		i->m_Object.ReleaseSkrit();
	}
	m_SkritColl.clear();
}

void SkritBotMgr :: HandleMessage( const WorldMessage& msg )
{
	SkritColl::iterator i, ibegin = m_SkritColl.begin(), iend = m_SkritColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		FuBiEvent::OnBotHandleMessage( i->m_Object, msg.GetEvent(), msg );
	}
}

void SkritBotMgr :: HandleUiAction( UI_ACTION action, const gpstring& parameter, UIWindow* window )
{
	SkritColl::iterator i, ibegin = m_SkritColl.begin(), iend = m_SkritColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		FuBiEvent::OnBotHandleUiAction( i->m_Object, action, parameter, window );
	}
}

void SkritBotMgr :: HandleUiWindowMessage( const UIMessage& msg, UIWindow* window )
{
	SkritColl::iterator i, ibegin = m_SkritColl.begin(), iend = m_SkritColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		FuBiEvent::OnBotHandleUiWindowMessage( i->m_Object, msg.m_code, msg, window );
	}
}

void SkritBotMgr :: HandleUiInterfaceMessage( const UIMessage& msg, const gpstring& interfce )
{
	SkritColl::iterator i, ibegin = m_SkritColl.begin(), iend = m_SkritColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		FuBiEvent::OnBotHandleUiInterfaceMessage( i->m_Object, msg.m_code, msg, interfce );
	}
}

void SkritBotMgr :: Update( float deltaTime )
{
	SkritColl::iterator i, ibegin = m_SkritColl.begin(), iend = m_SkritColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		FuBiEvent::OnBotUpdate( i->m_Object, deltaTime );
	}
}

void SkritBotMgr :: SysUpdate( float deltaTime )
{
	SkritColl::iterator i, ibegin = m_SkritColl.begin(), iend = m_SkritColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		FuBiEvent::OnBotSysUpdate( i->m_Object, deltaTime );
		i->m_Object->Poll( deltaTime, true );
	}
}

void SkritBotMgr :: Draw( void )
{
	SkritColl::iterator i, ibegin = m_SkritColl.begin(), iend = m_SkritColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		FuBiEvent::OnBotDraw( i->m_Object );
	}
}

//////////////////////////////////////////////////////////////////////////////

#endif // GP_ENABLE_SKRITBOT

//////////////////////////////////////////////////////////////////////////////
