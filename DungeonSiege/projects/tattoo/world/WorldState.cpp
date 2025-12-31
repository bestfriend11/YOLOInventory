//////////////////////////////////////////////////////////////////////////////
//
// File     :  WorldState.cpp
// Author(s):  Bartosz Kijanka, Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "WorldState.h"

#include "NetFuBi.h"
#include "World.h"
#include "WorldTime.h"
#include "StdHelp.h"
#include "ReportSys.h"
#include "Player.h"
#include "Server.h"
#include "WinX.h"
#include "Services.h"
#include "DrWatson.h"

FUBI_EXPORT_ENUM( eWorldState, WS_BEGIN, WS_COUNT );

//////////////////////////////////////////////////////////////////////////////
// enum eWorldState implementation

static const char* s_WorldStateStrings[] =
{
	"ws_invalid",
	"ws_any",
	"ws_init",
	"ws_intro",
	"ws_logo",
	"ws_main_menu",
	"ws_mega_map",
	"ws_load_map",
	"ws_loading_map",
	"ws_loaded_map",
	"ws_loaded_intro",
	"ws_wait_for_begin",
	"ws_loading_save_game",
	"ws_credits",
	"ws_options",
	"ws_deinit",
	"ws_reloading",
	"ws_game_ended",
	"ws_sp_main_menu",
	"ws_sp_ingame_menu",
	"ws_sp_character_select",
	"ws_sp_map_select",
	"ws_sp_difficulty_select",
	"ws_sp_victory_screen",
	"ws_sp_load_game_screen",
	"ws_sp_save_game_screen",
	"ws_sp_ingame",
	"ws_sp_nis",
	"ws_sp_defeat",
	"ws_sp_victory",
	"ws_sp_outro",
	"ws_mp_provider_select",
	"ws_mp_internet_game",
	"ws_mp_lan_game",
	"ws_mp_match_maker",
	"ws_mp_staging_area_server",
	"ws_mp_staging_area_client",
	"ws_mp_character_select",
	"ws_mp_map_select",
	"ws_mp_save_game_screen",
	"ws_mp_ingame",
	"ws_mp_ingame_jip",
	"ws_mp_session_lost",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_WorldStateStrings ) == WS_COUNT );

static stringtool::EnumStringConverter s_WorldStateConverter(
		s_WorldStateStrings, WS_BEGIN, WS_END, WS_INVALID );

const char* ToString( eWorldState e )
	{  return ( s_WorldStateConverter.ToString( e ) );  }
bool FromString( const char* str, eWorldState& e )
	{  return ( s_WorldStateConverter.FromString( str, rcast <int&> ( e ) ) );  }

bool IsInGame( eWorldState e )
{
	switch ( e )
	{
		case ( WS_SP_INGAME ):
		case ( WS_MP_INGAME ):
		case ( WS_MP_INGAME_JIP ):
		case ( WS_SP_NIS    ):
		case ( WS_MEGA_MAP  ):
		case ( WS_RELOADING ):
		case ( WS_SP_DEFEAT ):
		case ( WS_SP_VICTORY ):
		{
			return ( true );
		}
		break;
	}

	return ( false );
}

bool IsInFrontend( eWorldState e )
{
	switch ( e )
	{
		case ( WS_INTRO ):
		case ( WS_LOGO ):
		case ( WS_MAIN_MENU ):
		case ( WS_CREDITS ):
		case ( WS_OPTIONS ):
		case ( WS_SP_MAIN_MENU ):
		case ( WS_SP_CHARACTER_SELECT ):
		case ( WS_SP_MAP_SELECT ):
		case ( WS_SP_DIFFICULTY_SELECT ):
		case ( WS_MP_PROVIDER_SELECT ):
		case ( WS_MP_INTERNET_GAME ):
		case ( WS_MP_LAN_GAME ):
		case ( WS_MP_MATCH_MAKER ):
		case ( WS_MP_STAGING_AREA_SERVER ):
		case ( WS_MP_STAGING_AREA_CLIENT ):
		case ( WS_MP_CHARACTER_SELECT ):
		case ( WS_MP_MAP_SELECT ):
		{
			return ( true );
		}
		break;
	}
	return( false );
}

bool IsInStagingArea( eWorldState e )
{
	switch ( e )
	{
		case ( WS_MP_STAGING_AREA_SERVER ):
		case ( WS_MP_STAGING_AREA_CLIENT ):
		case ( WS_MP_CHARACTER_SELECT ):
		case ( WS_MP_MAP_SELECT ):
		{
			return ( true );
		}
		break;
	}
	return( false );
}

bool IsLoading( eWorldState e )
{
	switch ( e )
	{
		case ( WS_LOAD_MAP       	):
		case ( WS_LOADING_MAP       ):
		case ( WS_LOADED_MAP        ):
		{
			return ( true );
		}
		break;
	}
	return ( false );
}

bool IsGameStarting( eWorldState e )
{
	switch ( e )
	{
		case ( WS_LOAD_MAP       	):
		case ( WS_LOADING_MAP       ):
		case ( WS_LOADED_MAP        ):
		case ( WS_WAIT_FOR_BEGIN    ):
		case ( WS_LOADING_SAVE_GAME ):
		case ( WS_LOADED_INTRO		):
		{
			return ( true );
		}
		break;
	}

	return ( false );
}

bool IsEndGame( eWorldState e )
{
	switch ( e )
	{
		case ( WS_GAME_ENDED ):
		case ( WS_SP_DEFEAT ):
		case ( WS_SP_VICTORY ):
		case ( WS_MP_SESSION_LOST ):
		case ( WS_SP_OUTRO ):
		{
			return ( true );
		}
		break;
	}

	return ( false );
}

bool IsStagingArea( eWorldState e )
{
	switch ( e )
	{
		case ( WS_MP_STAGING_AREA_SERVER ):
		case ( WS_MP_STAGING_AREA_CLIENT ):
		{
			return ( true );
		}
		break;
	}
	return ( false );
}


//////////////////////////////////////////////////////////////////////////////
// class WorldState implementation

struct Transition
{
	double      m_TimeStarted;
	double      m_TimeStopped;
	eWorldState m_FromState;
	eWorldState m_ToState;
};

static const int  MAX_TRACKER_ENTRIES = 200;
static Transition s_WorldStateTracker[ 200 ];
static int        s_WorldStateTrackerCount = 0;
static int        s_WorldStateTrackerNext  = 0;

static void WorldStateInfoCallback( DrWatson::Info& info )
{
	// early out if nothing to print
	if ( s_WorldStateTrackerCount == 0 )
	{
		return;
	}

	// early out if can't print
	if ( info.m_Writer == NULL )
	{
		return;
	}

	// cache for readability
	bool full = !!(info.m_InfoFlags & DrWatson::INFO_FULL);
	int maxCount = s_WorldStateTrackerCount;
	if ( !full )
	{
		::minimize( maxCount, 10 );
	}

	// header
	info.m_Writer->WriteText( "[WorldState]\n\n" );

	// out each, newest to oldest
	for ( int count = 0, index = s_WorldStateTrackerNext ; count < maxCount ; ++count )
	{
		// work backward, wrapping
		--index;
		if ( index < 0 )
		{
			index = MAX_TRACKER_ENTRIES - 1;
		}

		// print it out
		const Transition& t = s_WorldStateTracker[ index ];
		info.m_Writer->WriteF(
				"    started = %g, stopped = %g, from = %s, to = %s\n",
				t.m_TimeStarted, t.m_TimeStopped,
				::ToString( t.m_FromState ),
				::ToString( t.m_ToState ) );
	}
	info.m_Writer->WriteText( "\n" );
}

WorldState :: WorldState( const TransitionEntry* table, int tableElementCount )
{
	// clear members
	m_RequestId = 0;
	m_DelayedStateTime = 0;

	// we're in the initial state
	m_CurrentState.m_State = WS_INIT;

	// fill transition map
	const TransitionEntry* i, * ibegin = table, * iend = ibegin + tableElementCount;
	for ( i = ibegin ; i != iend ; ++i )
	{
		m_TransitionDb.insert( std::make_pair( i->m_FromState, i->m_ToState ) );
	}

	// register our callback
	DrWatson::RegisterInfoCallback( makeFunctor( WorldStateInfoCallback ) );
}

WorldState :: ~WorldState( void )
{
	// this space intentionally left blank...
}

bool WorldState :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_RequestId", m_RequestId );

	return ( true );
}

void WorldState :: Update()
{
	if ( m_DelayedState.m_State != WS_INVALID )
	{
		if ( gWorldTime.GetTime() >= m_DelayedStateTime )
		{
			gpassert( !IsStateChangePending() );
			m_PendingState = m_DelayedState;
			m_DelayedState = State();
			m_DelayedStateTime = 0;
		}
		else
		{
			// $ early bailout
			return;
		}
	}

	bool changedState = false;
	while ( IsStateChangePending() )
	{
		changedState = true;

#		if !GP_RETAIL
		State dbgCurrentState = m_CurrentState;
		State dbgPendingState = m_PendingState;
#		endif // GP_DEBUG

		// start new tracker entry
		Transition& transition   = s_WorldStateTracker[ s_WorldStateTrackerNext ];
		transition.m_TimeStarted = ::GetSystemSeconds();
		transition.m_TimeStopped = 0;
		transition.m_FromState   = m_CurrentState.m_State;
		transition.m_ToState     = m_PendingState.m_State;

		// advance
		if ( s_WorldStateTrackerCount < MAX_TRACKER_ENTRIES )
		{
			++s_WorldStateTrackerCount;
		}
		++s_WorldStateTrackerNext;
		if ( s_WorldStateTrackerNext >= MAX_TRACKER_ENTRIES )
		{
			s_WorldStateTrackerNext = 0;
		}

		// copy pending state to allow subrequests as part of transition
		State pendingState = m_PendingState;
		eWorldState oldState = m_CurrentState.m_State;

		// shift down
		m_PreviousState = m_CurrentState;
		m_CurrentState = pendingState;
		m_PendingState = State();

#		if !GP_RETAIL
		gpstring report;
		report.assignf(	"\n"
						"==========================================================================================================================\n"
						"| BEGIN  - WORLD STATE TRANSITION: %s --> %s\n"
						"|\n"
						"| requested in file '%s', line %d\n"
						"|\n",
						 ToString( dbgCurrentState.m_State ), ToString( dbgPendingState.m_State ),
						 dbgPendingState.m_File.c_str(), dbgPendingState.m_Line );
		gpgeneric( report.c_str() );
#		endif

		// notify of transition
		OnTransitionTo( oldState, pendingState.m_State );

		// broadcast the change
		WorldMessage( WE_WORLD_STATE_TRANSITION_DONE, GOID_INVALID, GOID_ANY, pendingState.m_RequestId ).Send();

		// out status
#		if !GP_RETAIL
		report.assignf(	"|\n"
						"|\n"
						"| END - WORLD STATE TRANSITION.  State = %s\n"
						"==========================================================================================================================\n\n",  ToString( gWorldState.GetCurrentState() ) );
		gpgeneric( report.c_str() );
//		gpgameplay( report.c_str() );
#		endif // GP_DEBUG

		// record finish time
		transition.m_TimeStopped = ::GetSystemSeconds();

		// just to reduce MP send lag, always flush pipes on state changes
		if ( Server::DoesSingletonExist() && gServer.IsMultiPlayer() && gServer.IsLocal() && FuBi::NetSendTransport::DoesSingletonExist() )
		{
			gNetFuBiSend.ForceUpdate();
		}
	}

	// eat any pending user input that happened during the state change
	if ( changedState )
	{
		winx::EatAllUserInput();
	}
}


int WorldState :: Request( eWorldState next, double time )
{
	int requestId = REQUESTID_INVALID;

	// end loading thread immediately on key transition requests
	if( ( next == WS_MP_STAGING_AREA_SERVER ) ||
		( next == WS_GAME_ENDED ) ||
		( next == WS_MP_SESSION_LOST ) )
	{
		if( gSiegeEngine.IsLoadingThreadRunning() )
		{
			gSiegeEngine.StopLoadingThread();
		}
	}

	///////////////////////////////////////////////////////////////////
	//	WS_MP_SESSION_LOST overrides all other state requests - it can happen in too many
	//	states in MP, so forget about the transition table, and also clear out any other
	//	pending transitions -bk

	if( next == WS_MP_SESSION_LOST )
	{
#		if !GP_RETAIL
		gpstring report;
		report.assignf(	"\n"
						"\n"
						"| EXCEPTION - losing session while pending state == %s\n"
						"|\n"
						"|\n",
						 ToString( m_PendingState.m_State ) );
		gpgeneric( report.c_str() );
#		endif

		gpassert( IsMultiPlayer() );
		m_DelayedState.m_State		= WS_INVALID;
		m_DelayedStateTime			= 0;
		m_PendingState.m_State		= WS_MP_SESSION_LOST;
		m_PendingState.m_RequestId	= ++requestId;
		return ( requestId );
	}

	///////////////////////////////////////////////////////////////////
	//	process normal states

	if ( !IsStateChangePending() )
	{
		// make sure it's a valid transition
		std::pair <TransitionDb::const_iterator, TransitionDb::const_iterator> rc
				= m_TransitionDb.equal_range( m_CurrentState.m_State );
		for ( ; rc.first != rc.second ; ++rc.first )
		{
			if ( rc.first->second == next )
			{
				break;
			}
		}

		// valid?
		if ( rc.first != rc.second )
		{
			// get a new id for this request
			++m_RequestId;
			if ( m_RequestId == 0 )
			{
				++m_RequestId;
			}
			requestId = m_RequestId;

			// use it
			if ( time > 0 )
			{
				m_DelayedState				= m_PendingState;
				m_DelayedState.m_State		= next;
				m_DelayedState.m_RequestId	= requestId;
				m_DelayedStateTime			= PreciseAdd(gWorldTime.GetTime(), time);
			}
			else
			{
				m_PendingState.m_State		= next;
				m_PendingState.m_RequestId	= requestId;
			}
		}
		else
		{
			gperrorf(( "WorldState::Request( %s ): transition from '%s' is "
					   "illegal at this time (requested by [%s,%d]). Request "
					   "ignored.\nCheck the WorldState.vsd diagram for allowed "
					   "transitions, and check your usage and requests of "
					   "WorldState.\n",
					   ToString( next ), ToString( m_CurrentState.m_State ),
					   m_PendingState.m_File.c_str(), m_PendingState.m_Line ));
		}

		if( gServer.IsLocal() && IsMultiPlayer() && NetPipe::DoesSingletonExist() && gNetPipe.HasOpenSession() )
		{
			NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
			if( session )
			{
				session->GetInfo().m_WorldState = (DWORD)next;
				session->SUpdate();
			}
		}
	}
	else
	{
		gperrorf(( "WorldState::Request( %s ): can't request a state transition "
				   "if the last one (%s requested by [%s,%d]) hasn't taken "
				   "place. Request ignored.\n",
				   ToString( next ),
				   ToString( m_PendingState.m_State ),
				   m_PendingState.m_File.c_str(), m_PendingState.m_Line ));
	}

	return ( requestId );
}


void WorldState :: SSetWorldStateOnMachine( DWORD machineId, eWorldState oldState, eWorldState newState, double atTime )
{
	CHECK_SERVER_ONLY;

	RCSetWorldStateOnMachine( machineId, oldState, newState, atTime );

	for( Server::PlayerColl::iterator i = gServer.GetPlayersBegin(); i != gServer.GetPlayersEnd(); ++i )
	{
		if( IsValid( *i ) && ((*i)->GetMachineId() == machineId ))
		{
			gpgenericf(( "WorldState :: SSetWorldStateOnMachine - set state for '%S' to %s\n", (*i)->GetName().c_str(), ToString( newState ) ));
			(*i)->SetLastServerAssignedWorldState( newState );
		}
	}

	if( gServer.IsMultiPlayer() && FuBi::NetSendTransport::DoesSingletonExist() )
	{
		gNetFuBiSend.ForceUpdate();
	}
}


FuBiCookie WorldState :: RCSetWorldStateOnMachine( DWORD machineId, eWorldState oldState, eWorldState newState, double atTime )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSetWorldStateOnMachine, machineId );


	if_gpassertf(	( oldState == gWorldState.GetCurrentState() ) || ( oldState == WS_ANY ),
					( "Server :: RCSetWorldState( old = %s, new = %s ): this method was called during an unexpected WorldState (%s).",
				    ToString( oldState ), ToString( newState ), ToString( gWorldState.GetCurrentState() ) ) )
	{
		gPostWorldStateRequest( newState, atTime );
	}

	// in a MP situation, if we're the client, process the request immediately
	if( gServer.IsMultiPlayer() && gFuBiSysExports.IsDispatching() )
	{
		Update();
	}

	return ( RPC_SUCCESS );
}


void WorldState :: RCSetWorldStateIfInGame( DWORD machineId, eWorldState newState )
{
	FUBI_RPC_THIS_CALL( RCSetWorldStateIfInGame, machineId );

	if( IsInGame( gWorldState.GetCurrentState() ) )
	{
		if( (GetCurrentState() != newState) && (GetPendingState() != newState ) )
		{
			gPostWorldStateRequest( newState, 0 );
		}

		// in a MP situation, if we're the client, process the request immediately
		if( gServer.IsMultiPlayer() && gFuBiSysExports.IsDispatching() )
		{
			Update();
		}
	}
}


#if !GP_RETAIL

int WorldState :: Request( eWorldState next, const char* file, int line, double time )
{
	if ( !IsStateChangePending() )
	{
		m_PendingState.m_File = file;
		m_PendingState.m_Line = line;
	}

	return ( Request( next, time ) );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
