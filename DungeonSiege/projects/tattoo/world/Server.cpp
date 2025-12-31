/*============================================================================
  ----------------------------------------------------------------------------

	File		: Server.cpp

	Author(s)	: Bartosz Kijanka

	(C)opyright 2000 Gas Powered Games, Inc.

	--------------------------------------------------------------------------
  ----------------------------------------------------------------------------
  ============================================================================*/

#include "Precomp_World.h"

#include "AppModule.h"
#include "AIAction.h"
#include "Config.h"
#include "FuBi.h"
#include "FuBiPersistImpl.h"
#include "GameAuditor.h"
#include "GoActor.h"
#include "GoCore.h"
#include "GoDb.h"
#include "GoDefs.h"
#include "GoParty.h"
#include "GoSupport.h"
#include "MessageDispatch.h"
#include "NetFubi.h"
#include "Player.h"
#include "Server.h"
#include "Siege_Camera.h"
#include "Siege_Engine.h"
#include "StdHelp.h"
#include "UI_GridBox.h"
#include "UI_Shell.h"
#include "UI_Textureman.h"
#include "Victory.h"
#include "WorldMap.h"
#include "WorldOptions.h"
#include "WorldState.h"
#include "WorldTerrain.h"
#include "WorldTime.h"

FUBI_EXPORT_ENUM( eDropInvOption, DIO_BEGIN, DIO_COUNT );

//////////////////////////////////////////////////////////////////////////////
// enum eDropInvOption implementation

static const char* s_DropInvOptionStrings[] =
{
	"dio_invalid",
	"dio_nothing",	
	"dio_equipped",
	"dio_inventory",	
	"dio_all",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_DropInvOptionStrings ) == DIO_COUNT );

static stringtool::EnumStringConverter s_DropInvOptionConverter(
		s_DropInvOptionStrings, DIO_BEGIN, DIO_END );

const char* ToString( eDropInvOption dio )
{
	return ( s_DropInvOptionConverter.ToString( dio ) );
}

bool FromString( const char* str, eDropInvOption& dio )
{
	return ( s_DropInvOptionConverter.FromString( str, rcast <int&> ( dio ) ) );
}


Server :: Server( bool multiplayer )
{
	gpgeneric( "Server :: Server\n" );

	m_MultiPlayerMode              = multiplayer;

	m_Initialized				   = false;
	m_pScreenPlayer                = NULL;
	m_pGoDb                        = new GoDb;
	m_pNetFuBiSend				   = NULL;
	m_pNetFuBiReceive			   = NULL;
	m_pNetPipe					   = NULL;
	m_ScreenPlayerVisibleCount     = 0;              
	m_GhostTimeout                 = 0;              
	m_bAllowStartLocationSelection = true;           
	m_bAllowNewCharactersOnly	   = false;           
	m_bAllowPausing                = true;           
	m_bAllowJIP					   = gConfig.GetBool( "jip", false );
	m_bSettingsDirty               = false;
	m_DropInvOption				   = DIO_INVENTORY;
	m_bAllowRespawn				   = true;
	m_bAllowPlayerKills			   = false;

	m_NextLatencyQueryTime		   = 0;
	m_LatencyQueryFrequency		   = 2;

	FastFuelHandle hSettings( "config:multiplayer:settings" );
	if ( hSettings.IsValid() )
	{
		hSettings.Get( "ghost_timeout", m_GhostTimeout );
	}

	m_PlayerColl.resize( MAX_PLAYERS+1 );		// max players + computer player

	for( DWORD i = 0; i <= MAX_PLAYERS; ++i )
	{
		m_PlayerColl[ i ] = NULL;
		m_PlayerColl[ i ] = new Player( MakePlayerId( 1 << i ) );
		gpassert( m_PlayerColl[ i ] );
	}
}


void Server :: Init( bool multiplayer )
{
	gpgeneric( "Server :: Init\n" );

	//gpassertm( !m_Initialized, "Calling Server :: Init on an initialized server.  Check flow." );

	// disable debug freeze detection when we're doing multiplayer
	gAppModule.SetOptions( AppModule::OPTION_FREEZE_TIME_DETECT, !multiplayer );

	// allow updating the game even if we're alt-tabbed away if multiplayer
	gAppModule.SetOptions( AppModule::OPTION_ALWAYS_MINIMALLY_ACTIVE, multiplayer );

	// do not permit a boosted app priority when in multiplayer
	gAppModule.SetOptions( AppModule::OPTION_DISABLE_APP_PRIORITY, multiplayer );

	// start out in unpaused state (ignore user pause state, just need real pause set right)
	gAppModule.ForceResume();

	for ( int team = 0; team != MAX_PLAYERS; ++team )
	{
		m_Teams[ team ].m_Id		= team;
		m_Teams[ team ].m_Players	= 0;
		m_Teams[ team ].m_TextureId = 0;
	}

	if( !GetComputerPlayer() )
	{
		CreateComputerPlayer();
	}

	m_MultiPlayerMode = multiplayer;

	if( multiplayer )
	{
		bool fullNetPipeInit = false;
		if( !m_pNetPipe )
		{
			fullNetPipeInit = true;
			m_pNetPipe = new NetPipe;
			m_pNetPipe->SetProductVersion( SysInfo::GetExeFileVersion() );
		}

		m_pNetPipe->InitClient( makeFunctor( *this, &Server::OnNetPipeNotify ) );

		if( !m_pNetFuBiSend )
		{
			m_pNetFuBiSend = new NetFuBiSend;
			gpassert( m_pNetFuBiSend );
		}

		if( !m_pNetFuBiReceive )
		{
			m_pNetFuBiReceive = new NetFuBiReceive;
			gpassert( m_pNetFuBiReceive );
		}

		// $$$ this really needs to go into the UI somewhere
		if ( IsLocal() )
		{
			PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				Player * player = *i;
				if ( IsValid(player) && !player->IsComputerPlayer() && (player != GetLocalHumanPlayer()) )
				{
					player->RSSetReadyToPlay( false );
				}
			}
		}

		gpgeneric( "Server - initialized in MP mode.\n" );
	}
	else
	{
		gpgeneric( "Server - initialized in SP mode.\n" );
		gpassert( !GetLocalHumanPlayer() );
		SPCreateHumanPlayer();
	}

	m_Initialized = true;
}


Server :: ~Server()
{
	gpgeneric( "Server :: ~Server\n" );

	Shutdown( SHUTDOWN_FULL );

	// make sure placeholder players are deleted

	for( DWORD i = 0; i <= MAX_PLAYERS; ++i )
	{
		Delete( m_PlayerColl[ i ] );
	}

	// shut down godb cleanly if possible
	if ( !ReportSys::IsFatalOccurring() )
	{
		m_pGoDb->Shutdown();
		gAIAction.Shutdown();
	}

	Delete( m_pGoDb );
}


bool Server :: GetFirstFreePlayerId( PlayerId & pid )
{
	for( DWORD i = 0; i <= MAX_PLAYERS; ++i )
	{
		Player * player = m_PlayerColl[ i ];
		gpassert( player );
		if( !IsValid( player ) )
		{
			pid = player->GetId();
			return( true );
		}
	}
	return false;
}


void Server :: Shutdown( eShutdown sd )
{
	gpgeneric( "Server :: Shutdown\n" );

	if (   (sd == SHUTDOWN_FULL)
		|| (sd == SHUTDOWN_FOR_RELOAD_GAME)
		|| (sd == SHUTDOWN_FOR_NEW_GAME) )
	{
		// Players
		PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			Player * player = *i;
			if ( IsValid( player ) )
			{
				if( (sd == SHUTDOWN_FULL) || ( player->GetId() != PLAYERID_COMPUTER ) )
				{
					MarkPlayerForDeletion( (*i)->GetId() );
				}
				gGoDb.DeletePlayerGos( player->GetId() );
			}
		}
		CommitDeleteRequests();

		// Teams

		// clean up team textures
		for ( TeamSignMap::iterator it = m_TeamSigns.begin(); it != m_TeamSigns.end(); ++it )
		{
			if ( it->second != 0 )
			{
				gDefaultRapi.DestroyTexture( it->second );
			}
		}

		for ( int team = 0; team != MAX_PLAYERS; ++team )
		{
			m_Teams[ team ].m_Id		= team;
			m_Teams[ team ].m_Players	= 0;
			m_Teams[ team ].m_TextureId = 0;
		}
		m_TeamSigns.clear();

		ShutdownTransport( ( sd == SHUTDOWN_FULL ) );

		m_MultiPlayerMode = false;
	}
	else if( (sd == SHUTDOWN_FOR_NEW_MAP) )
	{
		PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			Player * player = *i;
			if ( IsValid( player ) )
			{
				player->SetInitialFrustum( FRUSTUMID_INVALID );
			}
		}

		if( FuBi::NetSendTransport::DoesSingletonExist() )
		{
			gNetFuBiSend.Shutdown();
			gNetFuBiReceive.Shutdown();
		}
	}

	m_Initialized = false;
}


void Server :: ShutdownTransport( bool full )
{
	if( m_pNetFuBiReceive )
	{
		gNetFuBiSend.Shutdown();
	}
	if( m_pNetFuBiSend )
	{
		gNetFuBiReceive.Shutdown();
	}

	if( m_pNetPipe )
	{
		gNetPipe.Shutdown();
	}

	if( full )
	{
		Delete( m_pNetFuBiReceive );
		Delete( m_pNetFuBiSend );
		Delete( m_pNetPipe );
	}
}


void Server :: Update( float actualDeltaTime )
{
	GPPROFILERSAMPLE( "Server :: Update", SP_MISC );

	SINGLETON_ONCE_PER_SIM;

	CommitDeleteRequests();

	////////////////////////////////////////
	//	update

	if( ::IsInGame( gWorldState.GetCurrentState() ) )
	{
		m_pGoDb->Update( gWorldTime.GetSecondsElapsed(), actualDeltaTime );

		////////////////////////////////////////
		//	player gets update in all modes

		for ( PlayerColl::iterator i = m_PlayerColl.begin(); i != m_PlayerColl.end(); ++i )
		{
			if ( IsValid( *i ) )
			{
				gpassert( !(*i)->IsMarkedForDeletion() );
			 	(*i)->Update();
			}
		}
	}

	// handle delayed disconnects that occurred while loading

	if( gWorldState.GetCurrentState() == WS_LOADED_MAP && !m_NetPipeMessages.empty() )
	{
		for( NetPipeDelayedMessageColl::iterator im = m_NetPipeMessages.begin(); im != m_NetPipeMessages.end(); )
		{
			// $$$ sometimes DPlay connect will come in before session_created; don't process connect until session is created
			bool skip = ((*im)->m_Event == NE_MACHINE_CONNECTED) && !gNetPipe.HasOpenSession();

			if( !skip )
			{
				PrivateOnNetPipeNotify( *(*im ) );
				delete *im;
				im = m_NetPipeMessages.erase( im );
			}
			else
			{
				++im;
			}
		}
		m_NetPipeMessages.clear();
	}

	if( IsMultiPlayer() )
	{
		gWorldOptions.SetLagMCP( GetMaxPlayerLatency() );
	}
}


void Server :: SPCreateHumanPlayer()
{
	gpassert( IsSinglePlayer() );

	// create local human's player
	PlayerId id = MakePlayerId( MakeInt( PLAYERID_COMPUTER ) << 1 );
	gpgeneric( "Single player server - created local human player.\n" );
	CreatePlayer( L"human", id, PC_HUMAN, RPC_TO_SERVER );
	WorldMessage( WE_MP_PLAYER_CREATED, GOID_INVALID, GOID_ANY, MakeInt(id) ).Send();
}


bool Server :: CreateComputerPlayer()
{
	gpassert( !HasPlayer( PLAYERID_COMPUTER ) );
	gpassert( IsLocal() );

	// create the server's player
	gpgeneric( "Single player server - created local computer player.\n" );
	bool result = CreatePlayer( L"computer", PLAYERID_COMPUTER, PC_COMPUTER, RPC_TO_SERVER );

#if !GP_RETAIL
	DebugOnCreatePlayerOnMachine( RPC_TO_LOCAL, PLAYERID_COMPUTER );
#endif

	WorldMessage( WE_MP_PLAYER_CREATED, GOID_INVALID, GOID_ANY, MakeInt(PLAYERID_COMPUTER) ).Send();

	return result;

}


bool Server :: SMarkPlayerForDeletion( PlayerId playerId )
{
	CHECK_SERVER_ONLY;

	return ( RCMarkPlayerForDeletion( playerId ) != RPC_FAILURE );
}


FuBiCookie Server :: RCMarkPlayerForDeletion( PlayerId pid )
{
	FUBI_RPC_CALL_RETRY( RCMarkPlayerForDeletion, RPC_TO_ALL );

	if( IsMultiPlayer() && gWorldState.GetCurrentState() ==  WS_LOADING_MAP )
	{
		gpassert( HasPlayer( pid ) );
		GetPlayer( pid )->MarkForSyncedDeletion();
	}
	else
	{
		MarkPlayerForDeletion( pid );
	}

	return ( RPC_SUCCESS );
}


bool Server :: MarkPlayerForDeletion( PlayerId playerId, bool deletePlaceHolder )
{	
	if( IsInitialized() )
	{
		gpgenericf(( "\nServer :: MarkPlayerForDeletion - playerId = 0x%08x\n", playerId ));
	}

	bool success = HasPlayer( playerId );
	if( success && !stdx::has_any( m_ReqDeletePlayers, playerId ) || ( deletePlaceHolder && !stdx::has_any( m_ReqDeletePlayers, playerId ) ) )
	{
		m_ReqDeletePlayers.push_back( playerId );
		Player * player = GetPlayerUnsafe( playerId );
		if( player && player->IsInitialized() )
		{
			WorldMessage( WE_MP_PLAYER_DESTROYED, GOID_INVALID, GOID_ANY, MakeInt(playerId) ).Send();
		}
	}
	return ( success || deletePlaceHolder );
}


void Server :: MarkAllPlayersForDeletion()
{
	PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( IsValid( *i ) )
		{
			MarkPlayerForDeletion( (*i)->GetId() );
		}
	}
}


UINT32 Server :: GetTotalHumanControlledGOs( void )
{
	UINT32 go_count = 0;

	// Players
	PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Player * player = *i;
		if ( IsValid( player ) )
		{
			if( (player->GetController() == PC_HUMAN) )
			{
				GopColl::const_iterator ip = player->GetPartyColl().begin(), ip_end = player->GetPartyColl().end();
				for(; ip != ip_end; ++ip )
				{
					go_count += (*ip)->GetChildren().size();
				}
			}
		}
	}
	return go_count;
}

void Server :: GetFriendlyHumanCharacters( PlayerId playerId, bool inFrustum, bool checkTeam, GopColl & output )
{
	Player * pPlayer = GetPlayer( playerId );
	FrustumId playerFrustum = GoHandle( pPlayer->GetHero() )->GetWorldFrustumMembership();

	for ( PlayerColl::const_iterator iPlayer = GetPlayers().begin(); iPlayer != GetPlayers().end(); ++iPlayer )
	{
		if( !IsValid( *iPlayer ) || (*iPlayer)->IsComputerPlayer() )
		{
			continue;
		}

		if ( pPlayer->GetIsFriend( *iPlayer ) && ( !checkTeam || ( pPlayer->GetTeam() == (*iPlayer)->GetTeam() ) ) )
		{
			Go * pParty = (*iPlayer)->GetParty();
			if ( pParty )
			{
	 			GopColl partyColl = pParty->GetChildren();
				for ( GopColl::iterator i = partyColl.begin(); i != partyColl.end(); ++i )
				{
					FrustumId tempFrustum = (*i)->GetWorldFrustumMembership();
					if( !inFrustum || ( ( DWORD ) playerFrustum & ( DWORD ) tempFrustum ) )
					{
						output.push_back( *i );
					}
				}
			}
		}
	}
}

bool Server :: IsMarkedForDeletion( PlayerId playerId )
{
	return stdx::has_any( m_ReqDeletePlayers, playerId );
}


void Server :: CommitDeleteRequests( bool addPlaceHolders )
{
	// $ early bailout if nothing to do
	if ( m_ReqDeletePlayers.empty() )
	{
		return;
	}

	
	if( gServer.GetScreenPlayer() != NULL )
	{
		PlayerIdColl::iterator playerIter, playerIterbegin = m_ReqDeletePlayers.begin(), playerIterend = m_ReqDeletePlayers.end();
		for ( playerIter = playerIterbegin ; playerIter != playerIterend ; ++playerIter )
		{
			if( gServer.GetScreenPlayer()->GetId() == (*playerIter) )
			{
				PlayerId ScreenPlayer = (*playerIter);

				m_ReqDeletePlayers.erase(playerIter);
				m_ReqDeletePlayers.push_back(ScreenPlayer);
				break;
			}
		}
	}

	// delete player's GOs
	PlayerIdColl::iterator i, ibegin = m_ReqDeletePlayers.begin(), iend = m_ReqDeletePlayers.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Player * player = m_PlayerColl[ MakeIndex( *i ) ];
		Team * team = GetTeam( player->GetId() );
		if ( team )
		{
			team->m_Players &= ~MakeInt( player->GetId() );
		}
		
		if( !player->IsComputerPlayer() && player->IsInitialized() )
		{
			Go * hero = GetGo( player->GetHero() );
			if ( hero && same_no_case( hero->GetTemplateName(), "farmgirl" ) )
			{
				gpscreenf(( $MSG$ "Player %S has left the game with all her worldly possessions.\n", player->GetName().c_str() ));
			}
			else
			{
				gpscreenf(( $MSG$ "Player %S has left the game with all his worldly possessions.\n", player->GetName().c_str() ));
			}
		}

		Delete( m_PlayerColl[ MakeIndex( *i ) ] );
	}

	// clear the request coll
	m_ReqDeletePlayers.clear();

	// replace deleted with placeholders

	if( addPlaceHolders )
	{
		for( DWORD i = 0; i <= MAX_PLAYERS; ++i )
		{
			if( m_PlayerColl[ i ] == NULL )
			{
				m_PlayerColl[ i ] = new Player( MakePlayerId( 1 << i ) );
				gpassert( m_PlayerColl[ i ] );
			}
		}
	}
}


Player * Server :: GetPlayer( gpwstring const & sName )
{
	Player* player = NULL;

	PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if( IsValid(*i) && (*i)->GetName().same_no_case( sName ) )
		{
			player = *i;
			break;
		}
	}

	return ( player );
}


Player * Server :: GetPlayer( PlayerId playerId )
{
	Player* player = NULL;

	gpassert( IsPower2( MakeInt( playerId ) ) );
	DWORD index = MakeIndex( playerId );
	if ( index < m_PlayerColl.size() )
	{
		player = m_PlayerColl[ index ];
		gpassert( (player == NULL) || (player->GetId() == playerId) );
	}

	return( ( player && player->IsInitialized() ) ? player : NULL );
}



Player * Server :: GetPlayerUnsafe( PlayerId playerId )
{
	Player* player = NULL;

	gpassert( IsPower2( MakeInt( playerId ) ) );
	DWORD index = MakeIndex( playerId );
	if ( index < m_PlayerColl.size() )
	{
		player = m_PlayerColl[ index ];
		gpassert( (player == NULL) || (player->GetId() == playerId) );
	}

	return( player );
}


DWORD Server :: GetNumHumanPlayers()
{
	DWORD count = 0;
	PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( IsValid( *i ) )
		{
			if( (*i)->GetController() == PC_HUMAN )
			{
				++count;
			}
		}
	}
	return count;
}


int Server :: GetPlayersOnMachine( PlayerColl& out, DWORD machineId )
{
	int oldSize = out.size();

	PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( IsValid( *i ) )
		{
			if (	( (*i)->GetMachineId() == machineId) ||
					( machineId == RPC_TO_ALL ) ||
					( (machineId == RPC_TO_OTHERS) && ( (*i)->GetMachineId() != RPC_TO_LOCAL ) ) )
			{
				out.push_back( *i );
			}
		}
	}

	return ( out.size() - oldSize );
}


bool Server :: HasPlayersOnMachine( DWORD machineId )
{
	PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if( IsValid( *i ) && !(*i)->IsMarkedForDeletion() && ( (*i)->GetMachineId() == machineId ) )
		{
			return true;
		}
	}
	return false;
}


Player * Server :: GetHumanPlayerOnMachine( DWORD machineId )
{
	PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Player* player = *i;
		if ( IsValid( player ) )
		{
			if ( machineId == RPC_TO_LOCAL )
			{
				if ( player->IsLocal() )
				{
					return ( player );
				}
			}
			else if ( (player->GetMachineId() == machineId) && !player->IsComputerPlayer() )
			{
				return ( player );
			}
		}
	}

	return ( NULL );
}


Player * Server :: GetLocalHumanPlayer()
{
	Player* player = NULL;

	// $$$ cache this

	PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( IsValid(*i) && (*i)->IsLocalHumanPlayer() )
		{
			player = *i;
			break;
		}
	}

	return ( player );
}


float Server :: GetMaxPlayerLatency() const
{
	float maxLatency = 0;

	PlayerColl::const_iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( IsValid(*i ) )
		{
			maxLatency = max( maxLatency, (*i)->GetAverageLatency() );
		}
	}
	return( maxLatency );
}


void Server :: OnWorldStateChange( eWorldState state )
{
//	gpassert( m_Initialized );

	Player* player = GetLocalHumanPlayer();
	if ( IsValid( player ) )
	{
		player->RSSetWorldState( state );
	}

	if ( IsLocal() )
	{
		player = GetComputerPlayer();
		if ( IsValid( player ) )
		{
			player->RSSetWorldState( state );
		}
	}
}


void Server :: SInitTeams()
{
	CHECK_SERVER_ONLY;

	// remove all alliances
	for ( PlayerColl::iterator i = m_PlayerColl.begin(); i != m_PlayerColl.end(); ++i )
	{
		if ( IsValid(*i) )
		{
			(*i)->SSetFriendTo( 0 );
			(*i)->SSetFriendToMe( 0 );
		}
	}

	RCInitTeams( RPC_TO_ALL );

	SSetDefaultTeams();
}


FuBiCookie Server :: RCInitTeams( DWORD machineId )
{
	FUBI_RPC_CALL_RETRY( RCInitTeams, machineId );

	if ( gVictory.GetTeamMode() == Victory::UNDEFINED_TEAMS )
	{
		ResetTeams();

		// load team default sign textures
		FastFuelHandle hTeamSigns( "ui:config:multiplayer:team_signs" );
		if ( hTeamSigns )
		{
			FastFuelFindHandle findSigns( hTeamSigns );

			if ( findSigns.FindFirstKeyAndValue() )
			{
				gpstring key, textureName;

				while ( findSigns.GetNextKeyAndValue( key, textureName ) )
				{
					unsigned int index = gUITextureManager.LoadTexture( textureName );
					if ( index )
					{
						m_TeamSigns.insert( TeamSignMap::value_type( textureName, index ) );
					}
				}
			}
		}

		if ( m_TeamSigns.empty() )
		{
			gpassertm( m_TeamSigns.size(), "Found no team sign textures in 'ui:config:multiplayer:team_signs'. This will prevent teams from being chosen." );
		}
	}
	else if ( gVictory.GetTeamMode() == Victory::DEFINED_TEAMS )
	{
		ResetTeams();

		int team = 0;
		for ( Victory::TeamMap::iterator i = gVictory.GetTeams().begin(); i != gVictory.GetTeams().end(); ++i, ++team )
		{
			if ( team >= MAX_PLAYERS )
			{
				break;
			}

			unsigned int index = gUITextureManager.LoadTexture( i->second.m_TextureName );

			m_TeamSigns.insert( TeamSignMap::value_type( i->second.m_TextureName, index ) );

			m_Teams[team].m_TextureId = index;
		}
	}
	else
	{
		ResetTeams();
	}

	return RPC_SUCCESS;
}


void Server :: SSetDefaultTeams()
{
	if ( gVictory.GetTeamMode() != Victory::NO_TEAMS )
	{
		for ( PlayerColl::iterator i = m_PlayerColl.begin(); i != m_PlayerColl.end(); ++i )
		{
			if( IsValid(*i) && !(*i)->IsComputerPlayer() )
			{
				RSSelectNextTeam( (*i)->GetId() );
			}
		}
	}
	else
	{
		int teamId = 0;
		for ( PlayerColl::iterator i = m_PlayerColl.begin(); i != m_PlayerColl.end(); ++i )
		{
			if( IsValid(*i) && !(*i)->IsComputerPlayer() )
			{
				RSJoinTeam( (TeamId)teamId, (*i)->GetId() );
				if ( gVictory.GetGameType()->GetAllowHumanPlayerKills() )
				{
					++teamId;
				}
			}
		}
	}
}


void Server :: SSetTeams( DWORD machineId )
{
	RCInitTeams( machineId );

	for ( int team = 0; team != MAX_PLAYERS; ++team )
	{
		gpstring texture;

		for ( TeamSignMap::iterator i = m_TeamSigns.begin(); i != m_TeamSigns.end(); ++i )
		{
			if ( i->second == m_Teams[team].m_TextureId )
			{
				texture = i->first;
				break;
			}
		}
		RCSetTeam( m_Teams[team], texture, machineId );
	}
}


FuBiCookie Server :: RCSetTeam( const Team & team, gpstring const & texture, DWORD machineId )
{
	FUBI_RPC_CALL_RETRY( RCSetTeam, machineId );

	m_Teams[team.m_Id] = team;
	
	TeamSignMap::iterator findSign = m_TeamSigns.find( texture );
	if ( findSign != m_TeamSigns.end() )
	{
		m_Teams[team.m_Id].m_TextureId = findSign->second;
	}
	else
	{
		m_Teams[team.m_Id].m_TextureId = 0;
	}

	return RPC_SUCCESS;
}


FuBiCookie Server :: RCSetTeamSign( TeamId teamId, gpstring const & texture )
{
	FUBI_RPC_CALL_RETRY( RCSetTeamSign, RPC_TO_ALL );

	TeamSignMap::iterator findSign = m_TeamSigns.find( texture );
	if ( findSign == m_TeamSigns.end() )
	{
		return RPC_FAILURE;
	}

	m_Teams[ MakeInt(teamId) ].m_TextureId = findSign->second;

	return RPC_SUCCESS;
}


FuBiCookie Server :: RSSelectNextTeam( PlayerId player, bool reverse )
{
	FUBI_RPC_CALL_RETRY( RSSelectNextTeam, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	if ( m_TeamSigns.empty() )
	{
		return RPC_FAILURE;
	}

	DWORD textureId = 0;

	Team * pTeam = GetTeam( player );
	if ( pTeam )
	{
		textureId = pTeam->m_TextureId;
	}

	// get the next team texture
	TeamSignMap::iterator nextSign = m_TeamSigns.begin();
	for ( TeamSignMap::iterator i = m_TeamSigns.begin(); i != m_TeamSigns.end(); ++i )
	{
		if ( textureId == i->second )
		{
			if ( reverse )
			{
				nextSign = i;
				if ( nextSign == m_TeamSigns.begin() )
				{
					nextSign = m_TeamSigns.end();
					--nextSign;
				}
				else
				{
					--nextSign;
				}
			}
			else
			{
				nextSign = ++i;
				if ( nextSign == m_TeamSigns.end() )
				{
					nextSign = m_TeamSigns.begin();
				}
			}
			break;
		}
	}

	return RSSelectTeam( player, nextSign->first );
}


FuBiCookie Server :: RSSelectTeam( PlayerId player, gpstring const & texture )
{
	FUBI_RPC_CALL_RETRY( RSSelectTeam, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	TeamSignMap::iterator findSign = m_TeamSigns.find( texture );
	if ( findSign == m_TeamSigns.end() )
	{
		return RPC_FAILURE;
	}

	bool foundTeam = false;
	for ( int team = 0; team != MAX_PLAYERS; ++team )
	{
		if ( m_Teams[team].m_TextureId == findSign->second )
		{
			// the sign is being used, join this team
			RSJoinTeam( TeamId(m_Teams[team].m_Id), player );
			foundTeam = true;
			break;
		}
	}

	if ( !foundTeam )
	{
		Team * pTeam = GetTeam( player );
		if ( pTeam && (pTeam->m_Players == MakeInt( player )) )
		{
			// i'm the only one on the team, just change the sign
			RCSetTeamSign( TeamId(pTeam->m_Id), findSign->first );
		}
		else
		{
			// find the next free team slot
			for ( int team = 0; team != MAX_PLAYERS; ++team )
			{
				if ( m_Teams[team].m_Players == 0 )
				{
					// join this team
					RSJoinTeam( TeamId(m_Teams[team].m_Id), player );

					RCSetTeamSign( TeamId(m_Teams[team].m_Id), findSign->first );

					break;
				}
			}
		}
	}

	return RPC_SUCCESS;
}


FuBiCookie Server :: RSJoinTeam( TeamId teamId, PlayerId player )
{
	FUBI_RPC_CALL_RETRY( RSJoinTeam, RPC_TO_SERVER );

	gpassertm( MakeInt(teamId) < MAX_PLAYERS, "invalid team number" );

	CHECK_SERVER_ONLY;

	Team * leaveTeam = GetTeam( player );
	if( leaveTeam )
	{
		// un-ally all remaining player with said player
		for( DWORD ip = 0; ip != MAX_PLAYERS; ++ip )
		{
			DWORD allyId = 1 << ip;
			if( leaveTeam->m_Players & allyId )
			{
				Player * ally = GetPlayer( PlayerId( allyId ) );
				ally->SSetFriendTo( ally->GetFriendTo() & ~MakeInt(player) );
				ally->SSetFriendToMe( ally->GetFriendToMe() & ~MakeInt(player) );
			}
		}
	}

	RCJoinTeam( teamId, player );

	// ally with new team members
	for( DWORD ip = 0; ip != MAX_PLAYERS; ++ip )
	{
		DWORD allyId = 1 << ip;
		if( m_Teams[MakeInt(teamId)].m_Players & allyId )
		{
			Player * ally = GetPlayer( PlayerId( allyId ) );
			ally->SSetFriendTo( m_Teams[MakeInt(teamId)].m_Players );
			ally->SSetFriendToMe( m_Teams[MakeInt(teamId)].m_Players );
		}
	}

	return RPC_SUCCESS;
}


FuBiCookie Server :: RCJoinTeam( TeamId teamId, PlayerId player )
{
	FUBI_RPC_CALL_RETRY( RCJoinTeam, RPC_TO_ALL );

	Team * team = GetTeam( player );
	if( team )
	{
		// remove player from team
		team->m_Players = team->m_Players & ~MakeInt(player);
	}

	// add player to new team
	m_Teams[MakeInt(teamId)].m_Players |= MakeInt(player);

	Player * pPlayer = GetPlayer( player );
	if ( pPlayer )
	{
		pPlayer->SetIsDirty();
	}

	return RPC_SUCCESS;
}


Team * Server :: GetTeam( PlayerId playerId )
{
	for( DWORD i = 0; i != MAX_PLAYERS; ++i )
	{
		if( m_Teams[i].m_Players & MakeInt(playerId) )
		{
			return &m_Teams[i];
		}
	}

	return NULL;
}


Server::TeamIdColl Server :: GetUsedTeams()
{
	Server::TeamIdColl usedTeams;

	for ( int i = 0; i != MAX_PLAYERS; ++i )
	{
		if ( m_Teams[ i ].m_Players > 0 )
		{
			usedTeams.push_back( TeamId(m_Teams[ i ].m_Id) );
		}
	}

	return usedTeams;
}


void Server :: ResetTeams()
{
	for ( TeamSignMap::iterator i = m_TeamSigns.begin(); i != m_TeamSigns.end(); ++i )
	{
		if ( i->second != 0 )
		{
			gUIShell.GetRenderer().DestroyTexture( i->second );
		}
	}
	m_TeamSigns.clear();

	for ( int team = 0; team != MAX_PLAYERS; ++team )
	{
		m_Teams[ team ].m_Players = 0;
		m_Teams[ team ].m_TextureId = 0;
	}
}


unsigned int Server :: GetMaxPlayers()
{
	gpassert( IsMultiPlayer() );
	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	return( session ? session->GetNumMachinesMax() : MAX_PLAYERS );
}


void Server :: SSetMaxPlayers( unsigned int maxPlayers )
{
	gpassert( IsMultiPlayer() );
	CHECK_SERVER_ONLY;

	RCSetMaxPlayers( maxPlayers );

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	if( session )
	{
		session->SUpdate();
	}
}


FuBiCookie Server :: RCSetMaxPlayers( unsigned int maxPlayers )
{
#if GP_DEBUG
	FUBI_RPC_TAG();
	gpassert( IsMultiPlayer() );
#endif

	FUBI_RPC_CALL_RETRY( RCSetMaxPlayers, RPC_TO_ALL );

	SetMaxPlayers( maxPlayers );

	return RPC_SUCCESS;
}


void Server :: SetMaxPlayers( unsigned int maxPlayers )
{
	gpassert( IsMultiPlayer() );

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	if( session )
	{
		session->SetNumMachinesMax( maxPlayers );
	}

	SetSettingsDirty();
	return;
}


gpwstring const & Server :: GetGamePassword()
{
	gpassert( IsMultiPlayer() );
	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

	return( session ? session->GetSessionPassword() : gpwstring::EMPTY );
}


void Server :: SSetGamePassword( gpwstring const & password )
{
	gpassert( IsMultiPlayer() );
	CHECK_SERVER_ONLY;

	RCSetGamePassword( password );

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	if( session )
	{
		session->SUpdate();	
	}
}


FuBiCookie Server :: RCSetGamePassword( gpwstring const & password )
{
#if GP_DEBUG
	FUBI_RPC_TAG();
	gpassert( IsMultiPlayer() );
#endif

	FUBI_RPC_CALL_RETRY( RCSetGamePassword, RPC_TO_ALL );

	SetGamePassword( password );

	return RPC_SUCCESS;
}


void Server :: SetGamePassword( gpwstring const & password )
{
	gpassert( IsMultiPlayer() );

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	if( session )
	{
		session->SetSessionPassword( password );
	}
	SetSettingsDirty();
}


void Server :: SSetAllowNewCharactersOnly( bool allow )
{
	gpassert( IsMultiPlayer() );
	CHECK_SERVER_ONLY;

	RCSetAllowNewCharactersOnly( allow );
}

FuBiCookie Server :: RCSetAllowNewCharactersOnly( bool allow )
{
#if GP_DEBUG
	FUBI_RPC_TAG();
	gpassert( IsMultiPlayer() );
#endif

	FUBI_RPC_CALL_RETRY( RCSetAllowNewCharactersOnly, RPC_TO_ALL );

	m_bAllowNewCharactersOnly = allow;

	SetSettingsDirty();

	return RPC_SUCCESS;
}


void Server :: SSetAllowStartLocationSelection( bool allow )
{
	gpassert( IsMultiPlayer() );

	CHECK_SERVER_ONLY;

	RCSetAllowStartLocationSelection( allow );
}

FuBiCookie Server :: RCSetAllowStartLocationSelection( bool allow )
{
#if GP_DEBUG
	FUBI_RPC_TAG();
	gpassert( IsMultiPlayer() );
#endif

	FUBI_RPC_CALL_RETRY( RCSetAllowStartLocationSelection, RPC_TO_ALL );

	m_bAllowStartLocationSelection = allow;

	SetSettingsDirty();

	return RPC_SUCCESS;
}


bool Server :: GetAllowPausing()
{
	return m_bAllowPausing && ( gWorldState.GetCurrentState() != WS_MP_INGAME_JIP );
}


void Server :: SSetAllowPausing( bool bSet )
{
	gpassert( IsMultiPlayer() );

	CHECK_SERVER_ONLY;

	RCSetAllowPausing( bSet );
}


void Server :: RCSetAllowPausing( bool bSet )
{
#if GP_DEBUG
	FUBI_RPC_TAG();
	gpassert( IsMultiPlayer() );
#endif
	FUBI_RPC_THIS_CALL( RCSetAllowPausing, RPC_TO_ALL );

	m_bAllowPausing = bSet;

	SetSettingsDirty();
}


void Server :: SSetAllowJIP( bool bSet )
{
	gpassert( IsMultiPlayer() );

	CHECK_SERVER_ONLY;

	if( IsInGame( gWorldState.GetCurrentState() ) )
	{
		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
		if( session )
		{
			session->SetIsLocked( !bSet );
		}
	}
	RCSetAllowJIP( bSet );
}


void Server :: RCSetAllowJIP( bool bSet )
{
#if GP_DEBUG
	FUBI_RPC_TAG();
	gpassert( IsMultiPlayer() );
#endif
	FUBI_RPC_THIS_CALL( RCSetAllowJIP, RPC_TO_ALL );

	m_bAllowJIP = bSet;

	SetSettingsDirty();
}


Go* Server :: GetScreenParty()
{
	Go* party = NULL;

	Player * pPlayer = GetScreenPlayer();
	if( pPlayer )
	{
		party = pPlayer->GetParty();
	}

	return party;
}


Go* Server :: GetScreenHero()
{
	Go* hero = NULL;

	if ( HasScreenPlayer() )
	{
		hero = GoHandle( GetScreenPlayer()->GetHero() );
	}

	return ( hero );
}

Go* Server :: GetScreenStash()
{
	return ( GetScreenPlayer() ? GoHandle( GetScreenPlayer()->GetStash() ).Get() : NULL );
}


bool Server :: GetScreenFormation( Formation * & formation )
{
	formation = GetScreenFormation();
	return formation != NULL;
}


Formation * Server :: GetScreenFormation()
{
	Go* party = GetScreenParty();
	Formation * formation = NULL;

	if( party )
	{
		formation = party->GetParty()->GetFormation();
	}

	return formation;
}


void Server :: SSetGhostTimeout( float timeout )
{
	CHECK_SERVER_ONLY;

	RCSetGhostTimeout( timeout );
}


void Server :: RCSetGhostTimeout( float timeout )
{
	FUBI_RPC_THIS_CALL( RCSetGhostTimeout, RPC_TO_ALL );

	SetGhostTimeout( timeout );
}


void Server :: SSetAllowRespawn( bool bSet )
{
	CHECK_SERVER_ONLY;

	RCSetAllowRespawn( bSet );
}


void Server :: RCSetAllowRespawn( bool bSet )
{
	FUBI_RPC_THIS_CALL( RCSetAllowRespawn, RPC_TO_ALL );

	SetAllowRespawn( bSet );
}


void Server :: SSetDropInvOption( eDropInvOption option )
{
	CHECK_SERVER_ONLY;

	RCSetDropInvOption( option );
}


void Server :: RCSetDropInvOption( eDropInvOption option )
{
	FUBI_RPC_THIS_CALL( RCSetDropInvOption, RPC_TO_ALL );

	SetDropInvOption( option );

	SetSettingsDirty();
}


void Server :: SPlaceHeroesInWorld()
{
	CHECK_SERVER_ONLY;

	PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if( IsValid(*i) )
		{
			(*i)->SAddPlayerToWorld();
		}
	}
}


#if !GP_RETAIL

void Server :: DebugCheckToldMachineToCreatePlayer( DWORD machineId, PlayerId pid )
{
	gpassert( IsLocal() );

	if( !DebugToldMachineToCreatePlayer( machineId, pid ) )
	{
		Player * player = GetPlayer( pid );
		gperrorf(( "Ordering problem!  Haven't yet told machineid 0x%08x about player %S, id 0x%08x\n", machineId, player->GetName().c_str(), pid ));
	}
}


bool Server :: DebugToldMachineToCreatePlayer( DWORD machineId, PlayerId pid )
{
	gpassert( IsLocal() );

	if( IsSinglePlayer() )
	{
		return true;
	}
	else if( !gNetPipe.HasOpenSession() )
	{
		return true;
	}

	if( pid == PLAYERID_COMPUTER )
	{
		return true;
	}

	if( machineId == RPC_TO_ALL )
	{
		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
		for( NetPipeMachineColl::iterator iMachine = session->GetMachinesBegin(); iMachine != session->GetMachinesEnd(); ++iMachine )
		{
			gpassert( (*iMachine)->IsRemote() );
			MachinePidArray::iterator i = m_MachinePidArray.find( (*iMachine)->m_Id );
			if( i != m_MachinePidArray.end() )
			{
				if( (*i).second.linear_find( MakeInt( pid ) ) == (*i).second.end() )
				{
					return false;
				}
			}
			else
			{
				return false;
			}

		}
		return true;
	}
	else
	{
		MachinePidArray::iterator i = m_MachinePidArray.find( machineId );
		if( i != m_MachinePidArray.end() )
		{
			return( (*i).second.linear_find( MakeInt( pid ) ) != (*i).second.end() );
		}
		else
		{
			return false;
		}
	}
}


void Server :: DebugOnMachineConnected( DWORD machineId )
{
	if( IsLocal() )
	{
		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
		NetPipeMachine * machine = session->GetMachine( machineId );
		if( machine->IsHost() )
		{
			DebugOnCreatePlayerOnMachine( RPC_TO_LOCAL, PLAYERID_COMPUTER );
		}
		else
		{
			DebugOnCreatePlayerOnMachine( machineId, PLAYERID_COMPUTER );
		}
	}
}


void Server :: DebugOnMachineDisconnected( DWORD machineId )
{
	MachinePidArray::iterator i = m_MachinePidArray.find( machineId );
	if( i != m_MachinePidArray.end() )
	{
		m_MachinePidArray.erase( i );
	}
}


void Server :: DebugOnCreatePlayerOnMachine( DWORD machineId, PlayerId pid )
{
	bool bRegistered = false;
	MachinePidArray::iterator i = m_MachinePidArray.find( machineId );
	if( i == m_MachinePidArray.end() )
	{
		PidColl playerIds;
		playerIds.push_back( MakeInt( pid ) );
		gpassert(0xdddddddd != machineId);
		m_MachinePidArray.insert( std::make_pair( machineId, playerIds ) );
		bRegistered = true;
	}
	else
	{
		if( (*i).second.linear_find( MakeInt( pid ) ) == (*i).second.end() )
		{
			(*i).second.push_back( MakeInt( pid ) );
			bRegistered = true;
		}
	}
	gpgenericf(( "DebugOnCreatePlayerOnMachine - machineId 0x%08x, PlayerId 0x%08x - %s\n", machineId, pid, bRegistered ? "Registered" : "Already registered" ));
}

#endif


FuBiCookie Server :: RCSyncOnMachineHelper( DWORD machineId, const_mem_ptr syncBlock )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSyncOnMachineHelper, machineId );

	FuBi::SerialBinaryReader reader( syncBlock );
	FuBi::PersistContext persist( &reader );
	XferForSync( persist );

	return ( RPC_SUCCESS );
}


void Server :: XferForSync( FuBi::PersistContext& persist )
{
	// update this as more stuff needs to be synced

	persist.Xfer( "", m_bAllowStartLocationSelection		);
	persist.Xfer( "", m_bAllowNewCharactersOnly	    		);
	persist.Xfer( "", m_bAllowPausing                		);
	persist.Xfer( "", m_bAllowJIP							);
	persist.Xfer( "", m_bAllowRespawn						);
	persist.Xfer( "", *(DWORD*)&m_DropInvOption				);

	gpwstring password = GetGamePassword();
	DWORD maxPlayers = GetMaxPlayers();
	if( persist.IsRestoring() )
	{
		persist.Xfer( "", maxPlayers );
		SetMaxPlayers( maxPlayers );

		persist.Xfer( "", password );
		SetGamePassword( password );
	}
	else
	{
		persist.Xfer( "", maxPlayers );
		persist.Xfer( "", password );
	}
}


FuBiCookie Server :: RSCreatePlayer( const wchar_t * name, ePlayerController Controller )
{
	FUBI_RPC_TAG();

	////////////////////////////////////////
	//	validation

	gpgenericf(( "Server :: RSCreatePlayer called - player %S\n", name ));

#if !GP_RETAIL

	// $$ paranoia check
	if( IsMultiPlayer() )
	{
		if( ( !NetPipe::DoesSingletonExist() || !gNetPipe.HasOpenSession() ) )
		{
			gperror( "You're asking the server for a player before you've established a connection." );
		}
		else
		{
			NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
			if( !session->IsHost() )
			{
				gpassert( IsRemote() );
			}
		}
	}
#endif

	FUBI_RPC_CALL_RETRY( RSCreatePlayer, RPC_TO_SERVER );

	// $$$ security

	////////////////////////////////////////
	//	create actual player

	PlayerId playerId;
	if( !GetFirstFreePlayerId( playerId ) )
	{
		gperror( "Can't create any more players.  Horrible!  Category 5 badness.  Save the .dslogs and report this." );
		return RPC_SUCCESS;
	}

	gpwstring playerName = name;
	int nameCount = 1;
	while( GetPlayer( playerName ) )
	{
		++nameCount;
		playerName = name;
		playerName.appendf( L"%d", nameCount );
	}

	if( IsMultiPlayer() && IsLocal() && NetPipe::DoesSingletonExist() )
	{	
		if( RPC_CALLER == RPC_TO_LOCAL )
		{
			RCCreatePlayerOnMachine( RPC_TO_LOCAL, playerName.c_str(), playerId, Controller, RPC_TO_LOCAL );
		}
		else
		{
			// $$ 	super duper special case of RPC_TO_ALL... RPC_ALL wiil only send to players we know about, so we must directly
			//		use the machineid in one instance
			
			//	create locally
			RCCreatePlayerOnMachine( RPC_TO_LOCAL, playerName.c_str(), playerId, Controller, RPC_CALLER );

			// create remotely
			NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
			for( NetPipeMachineColl::iterator iMachine = session->GetMachinesBegin(); iMachine != session->GetMachinesEnd(); ++iMachine )
			{
				RCCreatePlayerOnMachine( (*iMachine)->m_Id, playerName.c_str(), playerId, Controller, RPC_CALLER );
			}
		}
	}
	else
	{
		gpassert( RPC_CALLER == RPC_TO_LOCAL );
		RCCreatePlayerOnMachine( RPC_TO_LOCAL, playerName.c_str(), playerId, Controller, RPC_TO_LOCAL );
	}

	// tracking of sync needed

	gpassert( IsLocal() );
	if( IsMultiPlayer() && NetPipe::DoesSingletonExist() && gNetPipe.HasOpenSession() )
	{
		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

		if( session )
		{
			NetPipeMachine * machine = session->GetMachine( RPC_CALLER );
			if( machine )
			{
				machine->m_PlayersSynced |= MakeInt( playerId );
				machine->m_LocalPlayerCreated = true;
			}

			SSyncPlayers();
		}
	}

	WorldMessage( WE_MP_PLAYER_CREATED, GOID_INVALID, GOID_ANY, MakeInt(playerId) ).Send();

	Player * pPlayer	= GetPlayer( playerId );
	gpassert( pPlayer );

	////////////////////////////////////////
	//	teams

	pPlayer->RCSetJIP( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP );

	if ( gVictory.GetTeamMode() == Victory::NO_TEAMS )
	{
		SSetDefaultTeams();
	}
	else if ( !m_TeamSigns.empty() )
	{
		RSSelectTeam( playerId, m_TeamSigns.begin()->first );
	}

	////////////////////////////////////////
	//	starting positions

	gWorldMap.SAssignStartingPositions();

	return RPC_SUCCESS;
}


FuBiCookie Server :: RCCreatePlayerOnMachine(	DWORD createOnMachineId ,
		 										const wchar_t * name,
												PlayerId playerId,
												ePlayerController Controller,
												DWORD PlayerHostMachineId )
{
	FUBI_RPC_TAG();

	gpassert( createOnMachineId != RPC_TO_ALL );

#if !GP_RETAIL

	if( IsMultiPlayer() && NetPipe::DoesSingletonExist() )
	{
		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	}

	if( IsLocal() && NetPipe::DoesSingletonExist() )
	{
		DebugOnCreatePlayerOnMachine( createOnMachineId, playerId );
	}

#endif

	gpassert( createOnMachineId != RPC_TO_ALL );

	gpgenericf(( "\nServer :: RCCreatePlayerOnMachine\n\tname = %S, location = %s, playerid = %d, machineid = 0x%08x, controller = %s\n",
				 name,
				 (RPC_TO_LOCAL == PlayerHostMachineId) ? "LOCAL" : "REMOTE",
				 playerId,
				 PlayerHostMachineId,
				 ToString( Controller ) ));

	FUBI_RPC_CALL_RETRY( RCCreatePlayerOnMachine, createOnMachineId );

	if( !gServer.IsLocal() )
	{
		if( PlayerHostMachineId == FuBi::RPC_TO_SERVER_LOCAL )
		{
			PlayerHostMachineId = FuBi::RPC_TO_SERVER_REMOTE;
		}
		else if( createOnMachineId == PlayerHostMachineId )
		{
			PlayerHostMachineId = FuBi::RPC_TO_LOCAL;
		}
		else
		{
			PlayerHostMachineId = 0;
		}
	}

	return CreatePlayer( name, playerId, Controller, PlayerHostMachineId ) ? RPC_SUCCESS : RPC_FAILURE;
}


bool Server :: CreatePlayer(	const gpwstring&  name,
								PlayerId          playerId,
								ePlayerController controller,
								DWORD             machineId )
{
	gpgenericf((	"\nServer :: CreatePlayer\n\tname = %S, playerid = %d, machineid = 0x%08x, controller = %s\n\n",
					name.c_str(),
					playerId,
					machineId,
					ToString( controller ) ));

	// guarantee it will fit
	gpassert( IsPower2( MakeInt( playerId ) ) );

	DWORD index = MakeIndex( playerId );

	// should alwas have a dummy player in place
	gpassert( m_PlayerColl[ index ] != NULL );
	if( HasPlayer( playerId ) )
	{
		Player * player = GetPlayer( playerId );
		gpgenericf((	"\nServer :: CreatePlayer - implicit deletion of player : name = %S, playerid = %d, machineid = 0x%08x, controller = %s\n\n",
						player->GetName().c_str(),
						playerId,
						player->GetMachineId(),
						ToString( player->GetController() ) ));
	}
	MarkPlayerForDeletion( playerId, true );
	CommitDeleteRequests( false );
	gpassert( m_PlayerColl[ index ] == NULL );

	// make the darn player
	Player* player = new Player( name, machineId, controller, playerId );

	gpassert( m_PlayerColl.size() >= max_t( index + 1, m_PlayerColl.size() ) );

	// add to our collection
	gpassert( m_PlayerColl[ index ] == NULL );
	m_PlayerColl[ index ] = player;

	if( gServer.IsRemote() )
	{
		WorldMessage( WE_MP_PLAYER_CREATED, GOID_INVALID, GOID_ANY, MakeInt(playerId) ).Send();
	}

	// done
	return ( true );
}


void Server :: UpdateNetworkIO( bool dispatchReceived )
{
	if( NetPipe::DoesSingletonExist() )
	{
		if( IsMultiPlayer() )
		{
			// update keepalive
			gNetPipe.UpdateCallbacks();
			gNetPipe.UpdateLatencies();

			NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

			if( session )
			{
				if( gServer.IsLocal() )
				{
					for( NetPipeMachineColl::iterator i = session->GetMachinesBegin(); i != session->GetMachinesEnd(); ++i )
					{
						gpassert( (*i)->IsRemote() );
						if( ( (*i)->m_Id != RPC_TO_LOCAL ) && ( (*i)->GetTimeElapsedSinceLastSend() > 5.0f ) )
						{
							(*i)->m_LastSendTime = GetGlobalSeconds();
							RCKeepMeAlive( (*i)->m_Id );
						}
					}
				}
				else if( gServer.IsRemote() )
				{
					if( session->GetMachine( RPC_TO_SERVER )->GetTimeElapsedSinceLastSend() > 5.0f )
					{
						// because this won't register right away, force it
						session->GetMachine( RPC_TO_SERVER )->m_LastSendTime = GetGlobalSeconds();
						RSKeepMeAlive();
					}
				}

				if( m_pNetFuBiReceive && dispatchReceived )
				{
					m_pNetFuBiReceive->Update();
				}
				if( m_pNetFuBiSend )
				{
					m_pNetFuBiSend->Update();
				}

				//	if set to do so, query latency of all remote players

				if( gServer.IsLocal() && m_LatencyQueryFrequency )
				{
					double currentTime = GetGlobalSeconds();

					if( currentTime >= m_NextLatencyQueryTime )
					{
						PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
						for ( i = ibegin ; i != iend ; ++i )
						{
							Player * player = *i;
							if( IsValid(player) && !player->IsLocal() )
							{
								player->RCQueryLatency( (float)currentTime );
							}
						}
						m_NextLatencyQueryTime = PreciseAdd( currentTime, double( 1.0f / float( m_LatencyQueryFrequency ) ));
					}
				}
			}
		}
	}
}


void Server :: ResetTransport()
{
 	gpassert( IsMultiPlayer() );

	gNetFuBiSend.Reset();
	gNetFuBiReceive.Reset();
	
	gNetPipe.Shutdown();
	gNetPipe.InitClient( makeFunctor( *this, &Server::OnNetPipeNotify ) );
}


void Server :: OnNetPipeNotify( NetPipeMessage const & msg )
{
	gpassert( IsMultiPlayer() );

	if( !m_Initialized )
	{
		return;
	}

	gpgenericf(( "Server :: OnNetPipeNotify - event = %s\n", ::ToString( msg.m_Event ) ));

	if( msg.m_Event == NE_MACHINE_CONNECTED )
	{
		if( !gNetPipe.HasOpenSession() )
		{
			m_NetPipeMessages.push_back( new NetPipeMessage( msg ) );
		}
	}

	if( gWorldState.GetCurrentState() ==  WS_LOADING_MAP )
	{
		switch( msg.m_Event )
		{
			case NE_MACHINE_DISCONNECTED:
			case NE_MACHINE_DISCONNECTED_BANNED:
			case NE_MACHINE_DISCONNECTED_KICKED:
			case NE_SESSION_TERMINATED:
			{
				m_NetPipeMessages.push_back( new NetPipeMessage( msg ) );
			}
			break;

			default:
			{
				PrivateOnNetPipeNotify( msg );
			}
			break;
		}
	}
	else
	{
		PrivateOnNetPipeNotify( msg );
	}
}


void Server :: PrivateOnNetPipeNotify( NetPipeMessage const & msg )
{
	gpassert( IsMultiPlayer() );

	switch( msg.m_Event )
	{
	    case NE_NETWORK_UPDATE_MAX_LATENCY:
        {
			// $$$
			// take half of the max latency (one way) + the send delay + 5ms for buffer
	        // unless it's just too high, which is probably because of one loser, so screw 'em
//			float lagmcp = Minimum(static_cast<float>(msg.m_Double / 2.0 + gNetFuBiSend.GetSendDelay() + .005), .2f);

			// We can't simply ignore the maximum lag delay, we'll need to pause the game or 
			/// something if it gets way out of synch. We can't just say screw 'em --biddle
			if( gServer.IsLocal() )
			{
				//float lagmcp = static_cast<float>( (msg.m_Double / 2.0) + gNetFuBiSend.GetSendDelay() + .005);
				//gWorldOptions.SetLagMCP(lagmcp);
			}
	        break;
	    }
	        
		case NE_MACHINE_CONNECTED:
		{
			NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
			if( session && session->IsLocked() )
			{
				gperror( "The moon is full, the cows are in pasture.  The owl is barking.  Don't panic.  Ignore, but tell Bart." );
				gNetPipe.DisconnectMachine(	msg.m_Int );
				break;
			}

			OnConnectionCreated( msg.m_Int );
#if !GP_RETAIL
			DebugOnMachineConnected( msg.m_Int );
#endif
		}
		break;

		case NE_MACHINE_DISCONNECTED:
		case NE_MACHINE_DISCONNECTED_BANNED:
		case NE_MACHINE_DISCONNECTED_KICKED:
			OnConnectionDestroyed( msg.m_Int, msg.m_Event );
			break;

		case NE_FAILED_CONNECT:
			// HRESULT for failure goes in data1
			WorldMessage( WE_MP_FAILED_CONNECT, GOID_INVALID, GOID_ANY, msg.m_Int ).Send();
			break;

		case NE_SESSION_CONNECTED:
			WorldMessage( WE_MP_SESSION_CONNECTED, GOID_INVALID, GOID_ANY, msg.m_Int ).Send();
			break;

		case NE_SESSION_TERMINATED:
			OnConnectionDestroyed( RPC_TO_SERVER, msg.m_Int );
			break;

		case NE_SESSION_CHANGED:
			WorldMessage( WE_MP_SESSION_CHANGED, GOID_INVALID, GOID_ANY, msg.m_Int ).Send();
			break;

		case NE_SESSION_ADDED:
			WorldMessage( WE_MP_SESSION_ADDED, GOID_INVALID, GOID_ANY, msg.m_Int ).Send();
			break;

		case NE_SESSION_CREATED:
			WorldMessage( WE_MP_SESSION_CREATED, GOID_INVALID, GOID_ANY ).Send();
			break;

		default:
			gpassertm( 0, "Unhandled NetPipeNotify message." );
			break;
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
void Server :: SSyncPlayers()
{
	CHECK_SERVER_ONLY;

	if( !( IsMultiPlayer() && NetPipe::DoesSingletonExist() && gNetPipe.HasOpenSession() ) )
	{
		return;
	}

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

	if( !session )
	{
		return;
	}

	for( PlayerColl::iterator i = m_PlayerColl.begin() ; i != m_PlayerColl.end() ; ++i )
	{
		if( IsValid(*i) && !(*i)->IsComputerPlayer() )
		{
			NetPipeMachine * machine = session->GetMachine( (*i)->GetMachineId() );

			for( PlayerColl::iterator j = m_PlayerColl.begin(); j != m_PlayerColl.end(); ++j )
			{
				if( IsValid(*j) && !(*j)->IsComputerPlayer() )
				{
					if( machine )
					{
						gpassert( machine->m_LocalPlayerCreated );
						if( ( machine->m_PlayersSynced & MakeInt( (*j)->GetId() ) ) == 0 )
						{
							gpgenericf(( "Server :: SSafeSyncPlayers - syncing player %S, id 0x%08x for machine 0x%08x\n", (*j)->GetName().c_str(), (*j)->GetId(), machine->m_Id ));
							(*j)->SSyncOnMachine( machine->m_Id, true );
						}
					}
				}
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
void Server :: OnConnectionCreated( DWORD const machineId )
{
	gpgenericf(( "Server :: OnConnectionCreated - machineId = 0x%08x\n", machineId ));
	gpassert( IsMultiPlayer() );

	////////////////////////////////////////
	//	if we're about to go into JIP, flush godb and worldmessages before we send any sync updates out

	if( IsLocal() && IsInGame( gWorldState.GetCurrentState() ) )
	{
		gpassertm( !IsLocal() || ( IsLocal() && m_bAllowJIP ), "Shouldn't get this far if JIP is disabled" );

		if( (gWorldState.GetCurrentState() != WS_MP_INGAME_JIP) && (gWorldState.GetPendingState() != WS_MP_INGAME_JIP ) )
		{
			gMessageDispatch.Update( true );
			gGoDb.Update( 0, 0 );
		}
	}

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

	////////////////////////////////////////
	//	put all other clients in JIP mode - if server is in-game and they're in-game

	if( session && session->IsHost() && IsInGame( gWorldState.GetCurrentState() ) )
	{
		gpassertm( !IsLocal() || ( IsLocal() && m_bAllowJIP ), "Shouldn't get this far if JIP is disabled" );

		gWorldState.RCSetWorldStateIfInGame( RPC_TO_ALL, WS_MP_INGAME_JIP );
		gWorldState.Update();
	}

	////////////////////////////////////////
	//	is we're the server, synch globals with joiner

	if( session && session->IsHost() && FuBi::IsMachineRemote( machineId ) )
	{
		gpassert( !session->IsLocked() );	// $$$ this fires with skritbot

		SSyncOnMachine( machineId );
		
		gWorldTerrain.SSyncOnMachine( machineId );
		
		gWorldMap.SSyncOnMachine( machineId );

		gVictory.SSyncOnMachine( machineId );

		gGameAuditor.SSyncOnMachine( machineId );

		gWorldOptions.SSyncOnMachine( machineId );

		gGoDb.SSyncOnMachine( machineId );

		SSetTeams( machineId );
	}

	WorldMessage( WE_MP_MACHINE_CONNECTED, GOID_INVALID, GOID_ANY, machineId ).Send();
}


////////////////////////////////////////////////////////////////////////////////
//	
void Server :: OnConnectionDestroyed( DWORD const machineId, DWORD const event )
{
	gpgenericf(( "Server :: OnConnectionDestroyed - machineId = 0x%08x\n", machineId ));

	gpassert( m_Initialized );
	gpassert( IsMultiPlayer() );

	if( !HasPlayersOnMachine( machineId ) )
	{
		gpgeneric( "Server :: OnConnectionDestroyed - called when no known players on that machine\n" );
		return;
	}

#if !GP_RETAIL
	DebugOnMachineDisconnected( machineId );
#endif

	bool serverDisconnected = false;

	// if we lost connection to the server, shut the whole show down
	if( machineId == RPC_TO_SERVER )
	{
		serverDisconnected = true;

		gpscreen( $MSG$ "Server has left the game. Terminating session.\n" );

		gNetFuBiSend.Shutdown();
		gNetFuBiReceive.Shutdown();

		if( gWorldState.GetPendingState() != WS_MP_SESSION_LOST )
		{
			gWorldStateRequest( WS_MP_SESSION_LOST );
			WorldMessage( WE_MP_SESSION_TERMINATED, GOID_INVALID, GOID_ANY, event ).Send();
		}

		PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if( IsValid( *i ) && !(*i)->IsComputerPlayer() )
			{
				(*i)->MarkForSyncedDeletion();
			}
		}
	}
	//	if we lost connection to one of the clients, just clean up buffers... if we're the server, tell others to nuke
	//	the machine-associated players
	else
	{
		gNetFuBiReceive.OnMachineDisconnected( machineId );
		gNetFuBiSend.OnMachineDisconnected( machineId );

		//	if I'm the server, tell all other players to destroy this player...

		if( gServer.IsLocal() )
		{
			PlayerColl destroy;

			if ( GetPlayersOnMachine( destroy, machineId ) )
			{
				PlayerColl::iterator i, ibegin = destroy.begin(), iend = destroy.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					SMarkPlayerForDeletion( (*i)->GetId() );
				}
			}
			// $$$$$ don't move this... has to come after player is marked for deletion so leaving player doesn't get picked up in player iteration -bk
			WorldMessage( WE_MP_MACHINE_DISCONNECTED, GOID_INVALID, GOID_ANY, machineId ).Send();
		}
	}

	CommitDeleteRequests();
}


////////////////////////////////////////////////////////////////////////////////
//	
void Server :: SSyncOnMachine( DWORD const machineId )
{
	CHECK_SERVER_ONLY;

	gpgeneric( "\n\n================================================================================\n" );
	gpgenericf(( "Server :: SSyncOnMachine - machineId = 0x%08x\n", machineId ));


	FuBi::SerialBinaryWriter writer;
	FuBi::PersistContext persist( &writer );
	XferForSync( persist );
	RCSyncOnMachineHelper( machineId, writer.GetBuffer() );

	// tell remote machine to create each local player...

	PlayerColl::const_iterator i, ibegin = GetPlayers().begin(), iend = GetPlayers().end();

	for ( i = ibegin ; i != iend ; ++i )
	{
		Player* player = *i;
		if ( IsValid(player) && (player->GetId() != PLAYERID_COMPUTER) )
		{
			RCCreatePlayerOnMachine(	machineId,
										player->GetName().c_str(),
										player->GetId(),
										player->GetController(),
										player->GetMachineId() );

			// we only want to do a full xfer for players who are not already
			// created in-game. that way their imported characters will get
			// sync'd on new joiners properly. people in-game already will get
			// sync'd separately via the all-clients sync.
			player->SSyncOnMachine( machineId, player->GetHero() == GOID_INVALID );
		}
	}

	gpgeneric( "END - Server :: SSyncOnMachine --------------------------------------------------------\n" );
}


void Server :: RSKeepMeAlive()
{
	FUBI_RPC_CALL( RSKeepMeAlive, RPC_TO_SERVER );

	// the simple act of calling this will register in NetPipe and reset timeout timers
}


void Server :: RCKeepMeAlive( DWORD machineId )
{
	FUBI_RPC_CALL( RCKeepMeAlive, machineId );
}


#if !GP_RETAIL

void Server :: DebugListPlayers()
{
	gpgeneric( "Players:\n" );

	PlayerColl::iterator i, ibegin = m_PlayerColl.begin(), iend = m_PlayerColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( IsValid( *i ) )
		{
			gpgenericf(( "name = %S, id = %d, location = %s, controller = %s, machine_id = 0x%08x\n",
							(*i)->GetName().c_str(),
							(*i)->GetId(),
							(*i)->IsLocal() ? "LOCAL" : "REMOTE",
							ToString( (*i)->GetController() ),
							(*i)->GetMachineId() ));
		}
	}
}

#endif // !GP_RETAIL


