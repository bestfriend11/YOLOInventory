#pragma once
/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: 	WorldState.h

	Author(s)	: 	Bartosz Kijanka, Scott Bilas

	Purpose		: 	This file contains an interface definition for creating a
				 	class that helps synchronize state changes throughout the
				 	game. Typically, the GUI and the World need to synchronize
				 	work when transitioning from one state to the next.  In
				 	multiplayer this becomes even more of an issue because the
				 	local client will have to synch to the server.  Example: the
				 	client shouldn't begin game-play until the server says it's
				 	OK ( once all the other clients have been loaded. )

					Some semantics notes:

					The World has implicit states that it is in.  You can be
					"loading", "playing", "managing inventory", waiting for
					authorization from the server or any one of a large number
					of other states.  There are situations during the game's
					execution where it becomes critical that these events are
					synchronized.  With other systems, those being mainly the
					GUI and networking.

					The GUI has a set of global states, as does the World, but
					although the GUI states may reflect the World states, there
					isn't a 1:1 correspondence between the two.  So, we create a
					third-party object that will synchronize the two.

					Since both the GUI and the World can be seen as state
					machines, we can describe them with a connected graph and a
					transition table.

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#ifndef __WORLDSTATE_H
#define __WORLDSTATE_H

//////////////////////////////////////////////////////////////////////////////

#include "FuBiDefs.h"

#include <map>

//////////////////////////////////////////////////////////////////////////////
// enum eWorldState declaration

enum eWorldState
{
	SET_BEGIN_ENUM( WS_, 0 ),

// Basic constants

	WS_INVALID,						// generic "no state" variable
	WS_ANY,
	WS_INIT,						// initial state of machine

// Global states

	WS_INTRO,
	WS_LOGO,
	WS_MAIN_MENU,					// main game menu in front end
	WS_MEGA_MAP,
	WS_LOAD_MAP,
	WS_LOADING_MAP,
	WS_LOADED_MAP,
	WS_LOADED_INTRO,
	WS_WAIT_FOR_BEGIN,
	WS_LOADING_SAVE_GAME,
	WS_CREDITS,
	WS_OPTIONS,
	WS_DEINIT,
	WS_RELOADING,					// specifically and ONLY used for the "reload" command
	WS_GAME_ENDED,

// Single-player states

	WS_SP_MAIN_MENU,
	WS_SP_INGAME_MENU,
	WS_SP_CHARACTER_SELECT,
	WS_SP_MAP_SELECT,
	WS_SP_DIFFICULTY_SELECT,
	WS_SP_VICTORY_SCREEN,
	WS_SP_LOAD_GAME_SCREEN,
	WS_SP_SAVE_GAME_SCREEN,
	WS_SP_INGAME,
	WS_SP_NIS,
	WS_SP_DEFEAT,
	WS_SP_VICTORY,
	WS_SP_OUTRO,

// Multiplayer states

	WS_MP_PROVIDER_SELECT,
	WS_MP_INTERNET_GAME,
	WS_MP_LAN_GAME,
	WS_MP_MATCH_MAKER,
	WS_MP_STAGING_AREA_SERVER,
	WS_MP_STAGING_AREA_CLIENT,
	WS_MP_CHARACTER_SELECT,
	WS_MP_MAP_SELECT,
	WS_MP_SAVE_GAME_SCREEN,
	WS_MP_INGAME,
	WS_MP_INGAME_JIP,
	WS_MP_SESSION_LOST,

	SET_END_ENUM( WS_ ),
};

FEX const char* ToString		( eWorldState e );
    bool        FromString		( const char* str, eWorldState& e );
FEX bool        IsInGame		( eWorldState e );
FEX bool		IsInFrontend	( eWorldState e );
FEX bool		IsInStagingArea	( eWorldState e );
FEX bool		IsLoading		( eWorldState e );
FEX bool        IsGameStarting	( eWorldState e );
FEX bool        IsEndGame		( eWorldState e );
FEX bool		IsStagingArea	( eWorldState e );

//////////////////////////////////////////////////////////////////////////////
// class WorldState declaration

class WorldState : public Singleton <WorldState>
{
public:

// Types.

	struct TransitionEntry
	{
		eWorldState m_FromState;
		eWorldState m_ToState;
	};

	enum
	{
		REQUESTID_INVALID = 0
	};

// Setup.

	// ctor/dtor
	WorldState( const TransitionEntry* table, int tableElementCount );
	virtual ~WorldState( void );

	// persistence
	bool Xfer( FuBi::PersistContext& persist );

// Commands.

	// call this to commit state changes
	virtual void Update();

	// call this to change states - returns an int cookie which is used as Data1
	// in the resulting WorldMessage broadcast, or REQUESTID_INVALID if illegal.
	int Request( eWorldState next, double time );
FEX	int FUBI_RENAME( Request )( eWorldState next, float time )	{  return ( Request( next, time ) );  }
FEX	int Request( eWorldState next )								{  return ( Request( next, 0 ) );  }
FEX void ForceUpdate()											{  Update(); }

	void SSetWorldStateOnMachine( DWORD machineId, eWorldState oldState, eWorldState newState, double atTime = 0 );

FEX void RCSetWorldStateIfInGame( DWORD machineId, eWorldState );
	FUBI_MEMBER_SET( RCSetWorldStateIfInGame, +SERVER );


private:

FEX	FuBiCookie RCSetWorldStateOnMachine( DWORD machineId, eWorldState oldState, eWorldState newState, double atTime = 0 );
	FUBI_MEMBER_SET( RCSetWorldStateOnMachine, +SERVER );

public:

	// debug helper
#	if !GP_RETAIL
	int Request( eWorldState next, const char* file, int line, double time = 0 );
#	endif // !GP_RETAIL

	// state query
FEX eWorldState GetPreviousState    ( void ) const		{  return ( m_PreviousState.m_State );  }
FEX eWorldState GetCurrentState     ( void ) const		{  return ( m_CurrentState.m_State );  }
FEX eWorldState GetPendingState     ( void ) const		{  return ( m_PendingState.m_State );  }
FEX bool        IsStateChangePending( void )			{  return ( m_PendingState.m_State != WS_INVALID );  }

	// this is executed during a state transition
	virtual void OnTransitionTo( eWorldState from, eWorldState to ) = 0;

private:

// Private types.

	struct State
	{
		eWorldState m_State;				// state this represents
		int         m_RequestId;			// id of the request that gave us this state

#		if !GP_RETAIL
		gpstring	m_File;					// file that requested the current state change
		int			m_Line;					// line that requested the current state change
#		endif // !GP_RETAIL

		State( void )
		{
			m_State = WS_INVALID;
			m_RequestId = REQUESTID_INVALID;

			GPDEV_ONLY( m_Line = 0 );
		}
	};

// Private data.

	typedef std::multimap <eWorldState, eWorldState> TransitionDb;

	// spec
	TransitionDb m_TransitionDb;

	// state
	State m_PreviousState;					// where we were last
	State m_CurrentState;					// where we are now
	State m_PendingState;					// where we're going next (WS_INVALID if no transition requested)
	int   m_RequestId;						// id to be assigned to the next request

	// delayed state change
	State  m_DelayedState;					// delayed state to go to
	double m_DelayedStateTime;				// absolute time to change to this delayed state

	FUBI_SINGLETON_CLASS( WorldState, "Object to track the current major state of the game." );
	SET_NO_COPYING( WorldState );
};

#define gWorldState WorldState::GetSingleton()

#if GP_RETAIL
#define gWorldStateRequest( a ) gWorldState.Request( a )
#define gPostWorldStateRequest( a, t ) gWorldState.Request( a, t )
#else // GP_RETAIL
#define gWorldStateRequest( a ) gWorldState.Request( a, __FILE__, __LINE__ )
#define gPostWorldStateRequest( a, t ) gWorldState.Request( a, __FILE__, __LINE__, t )
#endif // GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

#endif  // __WORLDSTATE_H

//////////////////////////////////////////////////////////////////////////////
