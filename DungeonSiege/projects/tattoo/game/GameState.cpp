/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: 	GameState.h

	Author(s)	: 	Bartosz Kijanka

	Purpose		:	This file handles game-state-change events.

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/

#include "Precomp_game.h"
#include "GameState.h"

#include "config.h"
#include "fueldb.h"
#include "gamemovie.h"
#include "gamepersist.h"
#include "godb.h"
#include "gps_manager.h"
#include "inputbinder.h"
#include "mcp.h"
#include "messagedispatch.h"
#include "netfubi.h"
#include "player.h"
#include "rapimouse.h"
#include "ratiostack.h"
#include "server.h"
#include "services.h"
#include "siege_compass.h"
#include "siege_frustum.h"
#include "siege_loader.h"
#include "tattoogame.h"
#include "timemgr.h"
#include "timeofday.h"
#include "ui.h"
#include "uicamera.h"
#include "uicharacterselect.h"
#include "uidialoguehandler.h"
#include "uifrontend.h"
#include "uigame.h"
#include "uigameconsole.h"
#include "uimenumanager.h"
#include "uiintro.h"
#include "uioutro.h"
#include "uizonematch.h"
#include "uimultiplayer.h"
#include "uipartymanager.h"
#include "ui_animation.h"
#include "ui_statusbar.h"
#include "Victory.h"
#include "World.h"
#include "WorldFx.h"
#include "worldmap.h"
#include "worldoptions.h"
#include "worldterrain.h"
#include "worldtime.h"

//////////////////////////////////////////////////////////////////////////////
// class GameProgress declaration and implementation

class GameProgress : public RatioStack
{
public:
	GameProgress( bool forLoad )
	{
		m_ForLoad            = forLoad;
		m_RefCount           = 1;
		m_LeaveProgressBarUp = false;
		m_PermitRpcDispatch  = true;
		m_Progress           = 0;

		if( m_ForLoad )
		{
			if( ::IsSinglePlayer() )
			{
				gUIShell.ActivateInterface( FastFuelHandle( "ui:interfaces:backend:load_bar" ) );
			}
			else
			{
				gUIShell.ActivateInterface( FastFuelHandle( "ui:interfaces:backend:multiplayer_load" ) );
			}
			gDefaultRapi.SetClearColor( 0 );
		}

		// don't bother activating interface for save - that's handled by
		// higher powers in ui

		// no mouse while this is going on, looks messy
		gRapiMouse.PauseMouse( true, true );
	}

	virtual ~GameProgress( void )
	{
		// commit anything oustanding
		gSiegeLoadMgr.CommitAll();

		// force to 100%
		Finish( "done" );

		// show it for just a bit
		::Sleep( 50 );

		// now take it away
		if( m_ForLoad && !m_LeaveProgressBarUp )
		{
			if( ::IsSinglePlayer() )
			{
				gUIShell.DeactivateInterface( "load_bar" );
			}
			else
			{
				gUIShell.DeactivateInterface( "multiplayer_load" );
			}
		}

		// reset mouse cursor to the center lower 2/3 of the screen (so as not
		// to overlap the title fading) so we don't have the camera rotating
		// right away if they moved it to the edge during the map load.
		if ( m_ForLoad && gAppModule.IsAppActive() )
		{
			gTattooGame.ResetUserInputs();
		}

		// make sure we're not buffering
		gpassert( !gFuBiSysExports.TestOptions( FuBi::SysExports::OPTION_BUFFER_RPCS ) );

		// now we can have a mouse again
		gRapiMouse.PauseMouse( false );
	}

	virtual void OnRatioChanged( double newValue, const char* newStatus )
	{
		m_Progress = newValue;
		m_Status   = GPDEV_ONLY( (newStatus && *newStatus) ? ::ToUnicode( newStatus ) : ) gpwstring::EMPTY;

		UpdateProgress();

		// spin the net to update progress on other boxes
		if( ::IsMultiPlayer() )
		{
			gServer.UpdateNetworkIO( m_PermitRpcDispatch );
			Sleep( 0 );	// give time to DPlay threads
		}

		// $ uncomment for debugging purposes
		// gpgenericf(( "PROGRESS: %f %.5f (%s)\n", newValue, ::GetSystemSeconds(), m_Status.c_str() ));
	}

	void UpdateProgress( void )
	{
		if( UIGame::DoesSingletonExist() )
		{
			if( m_ForLoad )
			{
				gUIGame.UpdateLoadProgress( (float)m_Progress, true, m_Status );
			}
			else
			{
				gUIGame.UpdateSaveGameProgress( (float)m_Progress, false,
						/*$$$ TEMPORARY UNTIL NEW UI IN $$$
						m_Status $$$*/ gpwstring::EMPTY );
			}
		}
	}

	void Refresh( void )
	{
		// only refreshing on a load makes sense
		if( m_ForLoad )
		{
			UpdateProgress();
		}
	}

	int AddRef( void )
	{
		gpassert( m_RefCount < 20 );
		return ( ++m_RefCount );
	}

	int Release( void )
	{
		gpassert( m_RefCount > 0 );
		return ( --m_RefCount );
	}

	void SetLeaveProgressBarUp( bool set = true )
	{
		m_LeaveProgressBarUp = set;
	}

	void SetPermitRpcDispatch( bool set = true )
	{
		m_PermitRpcDispatch = set;
	}

private:
	bool      m_ForLoad;
	bool      m_LeaveProgressBarUp;
	bool      m_PermitRpcDispatch;
	int       m_RefCount;
	double    m_Progress;
	gpwstring m_Status;
};

//////////////////////////////////////////////////////////////////////////////
// class GameState implementation

const WorldState::TransitionEntry s_Transitions[] =
{

// Basic constants

	//////////////////////////////////////////////////////////
	// WS_INIT

	{ WS_INIT,					WS_MAIN_MENU				},
	{ WS_INIT,					WS_LOAD_MAP					},
	{ WS_INIT,					WS_INTRO					},
	{ WS_INIT,					WS_LOADING_SAVE_GAME		},
	{ WS_INIT,					WS_MP_MATCH_MAKER			},

// Global states

	//////////////////////////////////////////////////////////
	// WS_INTRO

	{ WS_INTRO,					WS_MAIN_MENU				},
	{ WS_INTRO,					WS_LOGO						},

	//////////////////////////////////////////////////////////
	// WS_INTRO

	{ WS_LOGO,					WS_MAIN_MENU				},

	//////////////////////////////////////////////////////////
	// WS_MAIN_MENU

	{ WS_MAIN_MENU,				WS_SP_MAIN_MENU				},
	{ WS_MAIN_MENU,				WS_CREDITS					},
	{ WS_MAIN_MENU,				WS_OPTIONS					},
	{ WS_MAIN_MENU,				WS_MP_PROVIDER_SELECT		},
	{ WS_MAIN_MENU,				WS_DEINIT					},
	{ WS_MAIN_MENU,				WS_INTRO					},
	{ WS_MAIN_MENU,				WS_LOADING_SAVE_GAME		},

	//////////////////////////////////////////////////////////
	// WS_MEGA_MAP

	{ WS_MEGA_MAP,				WS_SP_INGAME				},
	{ WS_MEGA_MAP,				WS_MP_INGAME				},
	{ WS_MEGA_MAP,				WS_MP_INGAME_JIP			},
	{ WS_MEGA_MAP,				WS_DEINIT					},
	{ WS_MEGA_MAP,				WS_SP_NIS					},
	{ WS_MEGA_MAP,				WS_SP_DEFEAT				},
	{ WS_MEGA_MAP,				WS_SP_VICTORY				},
	{ WS_MEGA_MAP,				WS_RELOADING				},
	{ WS_MEGA_MAP,				WS_MAIN_MENU				},
	{ WS_MEGA_MAP,				WS_GAME_ENDED				},
	{ WS_MEGA_MAP,				WS_LOADING_SAVE_GAME		},
	{ WS_MEGA_MAP,				WS_MP_SESSION_LOST			},

	//////////////////////////////////////////////////////////
	// WS_LOADING_MAP

	{ WS_LOAD_MAP,				WS_LOADING_MAP				},
	{ WS_LOADING_MAP,			WS_LOADED_MAP				},
	{ WS_LOADING_MAP,			WS_MP_SESSION_LOST			},

	//////////////////////////////////////////////////////////
	// WS_LOADED_MAP

	{ WS_LOADED_MAP,			WS_WAIT_FOR_BEGIN			},
	{ WS_LOADED_MAP,			WS_LOADED_INTRO				},
	{ WS_LOADED_MAP,			WS_MP_SESSION_LOST			},

	//////////////////////////////////////////////////////////
	// WS_LOADED_INTRO

	{ WS_LOADED_INTRO,			WS_WAIT_FOR_BEGIN			},
	{ WS_LOADED_INTRO,			WS_RELOADING				},

	//////////////////////////////////////////////////////////
	// WS_WAIT_FOR_BEGIN

	{ WS_WAIT_FOR_BEGIN,		WS_SP_INGAME				},
	{ WS_WAIT_FOR_BEGIN,		WS_MP_INGAME				},
	{ WS_WAIT_FOR_BEGIN,		WS_MP_SESSION_LOST			},

	//////////////////////////////////////////////////////////
	// WS_LOADING_SAVE_GAME

	{ WS_LOADING_SAVE_GAME,		WS_INTRO					},
	{ WS_LOADING_SAVE_GAME,		WS_MAIN_MENU				},
	{ WS_LOADING_SAVE_GAME,		WS_SP_INGAME				},
	{ WS_LOADING_SAVE_GAME,		WS_SP_LOAD_GAME_SCREEN		},
	{ WS_LOADING_SAVE_GAME,		WS_MEGA_MAP					},

	//////////////////////////////////////////////////////////
	// WS_CREDITS

	{ WS_CREDITS,				WS_MAIN_MENU				},

	//////////////////////////////////////////////////////////
	// WS_OPTIONS

	{ WS_OPTIONS,				WS_MAIN_MENU				},

	//////////////////////////////////////////////////////////
	// WS_GAME_ENDED

	{ WS_GAME_ENDED,			WS_MAIN_MENU				},
	{ WS_GAME_ENDED,			WS_SP_VICTORY				},
	{ WS_GAME_ENDED,			WS_MP_INTERNET_GAME			},
	{ WS_GAME_ENDED,			WS_MP_LAN_GAME				},
	{ WS_GAME_ENDED,			WS_MP_MATCH_MAKER			},
	{ WS_GAME_ENDED,			WS_MP_STAGING_AREA_SERVER	},
	{ WS_GAME_ENDED,			WS_MP_STAGING_AREA_CLIENT	},
	{ WS_GAME_ENDED,			WS_MP_SESSION_LOST			},
	{ WS_GAME_ENDED,			WS_DEINIT					},

	//////////////////////////////////////////////////////////
	// WS_DEINIT

	// $ none

	//////////////////////////////////////////////////////////
	// WS_RELOADING

	{ WS_RELOADING,				WS_SP_INGAME				},
	{ WS_RELOADING,				WS_MP_INGAME				},
	{ WS_RELOADING,				WS_MEGA_MAP					},
	{ WS_RELOADING,				WS_WAIT_FOR_BEGIN			},

// Single-player states

	//////////////////////////////////////////////////////////
	// WS_SP_MAIN_MENU

	{ WS_SP_MAIN_MENU,			WS_SP_CHARACTER_SELECT		},
	{ WS_SP_MAIN_MENU,			WS_SP_LOAD_GAME_SCREEN		},
	{ WS_SP_MAIN_MENU,			WS_MAIN_MENU				},

	//////////////////////////////////////////////////////////
	// WS_SP_INGAME_MENU

	{ WS_SP_INGAME_MENU,		WS_SP_SAVE_GAME_SCREEN		},
	{ WS_SP_INGAME_MENU,		WS_SP_LOAD_GAME_SCREEN		},

	//////////////////////////////////////////////////////////
	// WS_SP_CHARACTER_SELECT

	{ WS_SP_CHARACTER_SELECT,	WS_SP_MAP_SELECT			},
	{ WS_SP_CHARACTER_SELECT,	WS_SP_MAIN_MENU				},
	{ WS_SP_CHARACTER_SELECT,	WS_SP_DIFFICULTY_SELECT		},

	//////////////////////////////////////////////////////////
	// WS_SP_MAP_SELECT

	{ WS_SP_MAP_SELECT,			WS_LOAD_MAP					},
	{ WS_SP_MAP_SELECT,			WS_SP_CHARACTER_SELECT		},

	//////////////////////////////////////////////////////////
	// WS_SP_DIFFICULTY SELECT

	{ WS_SP_DIFFICULTY_SELECT,	WS_LOAD_MAP					},
	{ WS_SP_DIFFICULTY_SELECT,	WS_SP_CHARACTER_SELECT		},

	//////////////////////////////////////////////////////////
	// WS_SP_LOAD_GAME_SCREEN

	{ WS_SP_LOAD_GAME_SCREEN,	WS_LOAD_MAP					},
	{ WS_SP_LOAD_GAME_SCREEN,	WS_LOADING_SAVE_GAME		},
	{ WS_SP_LOAD_GAME_SCREEN,	WS_SP_MAIN_MENU				},

	//////////////////////////////////////////////////////////
	// WS_SP_SAVE_GAME_SCREEN

	{ WS_SP_SAVE_GAME_SCREEN,	WS_SP_INGAME_MENU			},

	//////////////////////////////////////////////////////////
	// WS_SP_INGAME

	{ WS_SP_INGAME,				WS_SP_NIS					},
	{ WS_SP_INGAME,				WS_MAIN_MENU				},
	{ WS_SP_INGAME,				WS_MEGA_MAP					},
	{ WS_SP_INGAME,				WS_SP_DEFEAT				},
	{ WS_SP_INGAME,				WS_SP_VICTORY				},
	{ WS_SP_INGAME,				WS_LOADING_SAVE_GAME		},
	{ WS_SP_INGAME,				WS_DEINIT					},
	{ WS_SP_INGAME,				WS_RELOADING				},
	{ WS_SP_INGAME,				WS_GAME_ENDED				},


	//////////////////////////////////////////////////////////
	// WS_SP_NIS

	{ WS_SP_NIS,				WS_SP_DEFEAT				},
	{ WS_SP_NIS,				WS_SP_VICTORY				},
	{ WS_SP_NIS,				WS_SP_INGAME				},
	{ WS_SP_NIS,				WS_MEGA_MAP					},
	{ WS_SP_NIS,				WS_DEINIT					},

	//////////////////////////////////////////////////////////
	// WS_SP_DEFEAT

	{ WS_SP_DEFEAT,				WS_MAIN_MENU				},
	{ WS_SP_DEFEAT,				WS_DEINIT					},
	{ WS_SP_DEFEAT,				WS_LOADING_SAVE_GAME		},
	{ WS_SP_DEFEAT,				WS_GAME_ENDED				},
	{ WS_SP_DEFEAT,				WS_SP_INGAME				},

	//////////////////////////////////////////////////////////
	// WS_SP_VICTORY

	{ WS_SP_VICTORY,			WS_MAIN_MENU				},
	{ WS_SP_VICTORY,			WS_SP_OUTRO					},

	//////////////////////////////////////////////////////////
	// WS_SP_OUTRO

	{ WS_SP_OUTRO,				WS_MAIN_MENU				},
	{ WS_SP_OUTRO,				WS_SP_INGAME				},


// Multiplayer states

	//////////////////////////////////////////////////////////
	// WS_MP_PROVIDER_SELECT

	{ WS_MP_PROVIDER_SELECT,	WS_MP_INTERNET_GAME			},
	{ WS_MP_PROVIDER_SELECT,	WS_MP_LAN_GAME				},
	{ WS_MP_PROVIDER_SELECT,	WS_MP_MATCH_MAKER			},
	{ WS_MP_PROVIDER_SELECT,	WS_MAIN_MENU				},

	//////////////////////////////////////////////////////////
	// WS_MP_INTERNET_GAME

	{ WS_MP_INTERNET_GAME,		WS_MP_PROVIDER_SELECT		},
	{ WS_MP_INTERNET_GAME,		WS_MP_STAGING_AREA_SERVER	},
	{ WS_MP_INTERNET_GAME,		WS_MP_STAGING_AREA_CLIENT	},
	{ WS_MP_INTERNET_GAME,		WS_DEINIT					},

	//////////////////////////////////////////////////////////
	// WS_MP_LAN_GAME

	{ WS_MP_LAN_GAME,			WS_MP_PROVIDER_SELECT		},
	{ WS_MP_LAN_GAME,			WS_MP_STAGING_AREA_SERVER	},
	{ WS_MP_LAN_GAME,			WS_MP_STAGING_AREA_CLIENT	},
	{ WS_MP_LAN_GAME,			WS_DEINIT					},

	//////////////////////////////////////////////////////////
	// WS_MP_MATCH_MAKER

	{ WS_MP_MATCH_MAKER,		WS_MP_STAGING_AREA_SERVER	},
	{ WS_MP_MATCH_MAKER,		WS_MP_STAGING_AREA_CLIENT	},
	{ WS_MP_MATCH_MAKER,		WS_MP_PROVIDER_SELECT		},
	{ WS_MP_MATCH_MAKER,		WS_MAIN_MENU				},
	{ WS_MP_MATCH_MAKER,		WS_DEINIT					},

	//////////////////////////////////////////////////////////
	// WS_MP_STAGING_AREA_*

	{ WS_MP_STAGING_AREA_SERVER,		WS_MP_CHARACTER_SELECT		},
	{ WS_MP_STAGING_AREA_SERVER,		WS_MP_MAP_SELECT			},
	{ WS_MP_STAGING_AREA_SERVER,		WS_LOAD_MAP					},
	{ WS_MP_STAGING_AREA_SERVER,		WS_MP_INTERNET_GAME			},
	{ WS_MP_STAGING_AREA_SERVER,		WS_MP_LAN_GAME				},
	{ WS_MP_STAGING_AREA_SERVER,		WS_MP_MATCH_MAKER			},
	{ WS_MP_STAGING_AREA_SERVER,		WS_MP_PROVIDER_SELECT		},
	{ WS_MP_STAGING_AREA_SERVER,		WS_MP_INGAME				},
	{ WS_MP_STAGING_AREA_SERVER,		WS_MP_SESSION_LOST			},
	{ WS_MP_STAGING_AREA_SERVER,		WS_DEINIT					},

	{ WS_MP_STAGING_AREA_CLIENT,		WS_MP_CHARACTER_SELECT		},
	{ WS_MP_STAGING_AREA_CLIENT,		WS_MP_MAP_SELECT			},
	{ WS_MP_STAGING_AREA_CLIENT,		WS_LOAD_MAP					},
	{ WS_MP_STAGING_AREA_CLIENT,		WS_MP_INTERNET_GAME			},
	{ WS_MP_STAGING_AREA_CLIENT,		WS_MP_LAN_GAME				},
	{ WS_MP_STAGING_AREA_CLIENT,		WS_MP_MATCH_MAKER			},
	{ WS_MP_STAGING_AREA_CLIENT,		WS_MP_PROVIDER_SELECT		},
	{ WS_MP_STAGING_AREA_CLIENT,		WS_MP_INGAME				},
	{ WS_MP_STAGING_AREA_CLIENT,		WS_MP_SESSION_LOST			},
	{ WS_MP_STAGING_AREA_CLIENT,		WS_GAME_ENDED				},
	{ WS_MP_STAGING_AREA_CLIENT,		WS_DEINIT					},

	//////////////////////////////////////////////////////////
	// WS_MP_CHARACTER_SELECT

	{ WS_MP_CHARACTER_SELECT,	WS_MP_STAGING_AREA_SERVER			},
	{ WS_MP_CHARACTER_SELECT,	WS_MP_STAGING_AREA_CLIENT			},
	{ WS_MP_CHARACTER_SELECT,	WS_MP_SESSION_LOST					},

	//////////////////////////////////////////////////////////
	// WS_MP_MAP_SELECT

	{ WS_MP_MAP_SELECT,			WS_MP_STAGING_AREA_SERVER			},

	//////////////////////////////////////////////////////////
	// WS_MP_SAVE_GAME_SCREEN

	//////////////////////////////////////////////////////////
	// WS_MP_INGAME

	{ WS_MP_INGAME,				WS_MAIN_MENU				},
	{ WS_MP_INGAME,				WS_MP_MATCH_MAKER			},
	{ WS_MP_INGAME,				WS_MP_STAGING_AREA_SERVER	},
	{ WS_MP_INGAME,				WS_MP_STAGING_AREA_CLIENT	},
	{ WS_MP_INGAME,				WS_MEGA_MAP					},
	{ WS_MP_INGAME,				WS_DEINIT					},
	{ WS_MP_INGAME,				WS_MP_SESSION_LOST			},
	{ WS_MP_INGAME,				WS_MP_INGAME_JIP			},
	{ WS_MP_INGAME,				WS_GAME_ENDED				},
	{ WS_MP_INGAME,				WS_RELOADING				},

	//////////////////////////////////////////////////////////
	// WS_MP_INGAME_JIP

	{ WS_MP_INGAME_JIP,			WS_MP_INGAME				},
	{ WS_MP_INGAME_JIP,			WS_MP_STAGING_AREA_SERVER	},
	{ WS_MP_INGAME_JIP,			WS_MP_STAGING_AREA_CLIENT	},
	{ WS_MP_INGAME_JIP,			WS_MP_SESSION_LOST			},
	{ WS_MP_INGAME_JIP,			WS_GAME_ENDED				},

	//////////////////////////////////////////////////////////
	// WS_MP_SESSION_LOST

	{ WS_MP_SESSION_LOST,		WS_MP_PROVIDER_SELECT		},
	{ WS_MP_SESSION_LOST,		WS_MP_INTERNET_GAME			},
	{ WS_MP_SESSION_LOST,		WS_MP_LAN_GAME				},
	{ WS_MP_SESSION_LOST,		WS_MP_MATCH_MAKER			},
	{ WS_MP_SESSION_LOST,		WS_MAIN_MENU				},
	{ WS_MP_SESSION_LOST,		WS_GAME_ENDED				},
};


GameState :: GameState()
	: Inherited( s_Transitions, ELEMENT_COUNT( s_Transitions ) )
{
	m_MPGameType = WS_INVALID;
	m_Progress = NULL;
	m_LastTimeJipPlayerPresent = 0;

	gMessageDispatch.RegisterBroadcastCallback( "gamestate", makeFunctor( *this, &GameState::HandleBroadcast ) );
}


GameState :: ~GameState()
{
    // this space intentionally left blank...

	if( MessageDispatch::DoesSingletonExist() )
	{
		gMessageDispatch.UnregisterBroadcastCallback( "gamestate" );
	}
}


void GameState :: Update()
{
	Inherited::Update();

	// spin the progress bar
	if( m_Progress != NULL )
	{
		m_Progress->Refresh();
	}
}


void GameState :: OnTransitionTo( eWorldState from, eWorldState to )
{
	// change from ingame to out of game
	gAppModule.SetOptions( AppModule::OPTION_ENABLE_LOG_FPS, IsInGame( to ) );

	////////////////////////////////////////////////////////////////////////////////
	//
	//	do any necessary cleanup before transition
	//

	// we're leaving from ingame in any way
	if ( IsInGame( from ) && !IsInGame( to ) )
	{
		// Let's save our party right now in case it's dirty
		gTattooGame.AutoSaveParty();
	}

	//	we're exiting in-game to front-end or desktop (NOTE this does NOT include
	//  session-lost exiting in MP game)
	if(	( IsInGame( from )
			&& !IsInGame( to )
			&& !IsEndGame( to )
			&& (to != WS_LOADING_SAVE_GAME) )
		|| ( IsEndGame( from )
				&& !IsEndGame( to )
				&& (to != WS_LOADING_SAVE_GAME)
				&& !(from == WS_SP_OUTRO && to == WS_SP_INGAME) ) )
	{
		// Streamer not needed - keep it off until we do need it
		if( gSiegeEngine.IsLoadingThreadRunning() )
		{
			gSiegeEngine.StopLoadingThread();
		}

		gSoundManager.StopAllSounds();

		if( gUIMultiplayer.GetMultiplayerMode() == MM_GUN )
		{
			// should only be called when exiting
			if ( to == WS_MAIN_MENU )
			{
				gUIZoneMatch.Disconnect();
			}
		}

		gUICamera.Shutdown();
		gUIOutro.SetAsActive( false );
		gUIPartyManager.DeconstructUIParty();

		gUIGame.SetAsActive( false );		
		gUIGame.SetIsVisible( false );
		gUIGame.GetGameInputBinder().SetIsActive( false );
		gUIGame.GetUserInputBinder().SetIsActive( false );

		gAppModule.SetGameSize( GSize( 800, 600 ) );

		gDefaultRapi.SetFogActivatedState( false );
		gDefaultRapi.SetClearColor( 0x00000000 );
		
		gSoundManager.StopAllSounds( true, true );

		if( gUIZoneMatch.IsHosting() )
		{
			gUIZoneMatch.OnEndGame();
		}

		if( (to == WS_MAIN_MENU) )
		{
			// first go to black while we shut down
			gDefaultRapi.ClearAllBuffers();

			gWorld.Shutdown( SHUTDOWN_FOR_NEW_GAME, false );
			gUIGame.Deinit();
			// on exiting, let's uncache all bin fuel
			FastFuelHandle root( "root" );
			root.GetDb()->Unload();
			gWorld.Init();
			gWorld.InitServer( false );
		}
		else if( (to == WS_MP_MATCH_MAKER ) || (to == WS_MP_INTERNET_GAME ) || (to == WS_MP_LAN_GAME ) )
		{
			// first go to black while we shut down
			gDefaultRapi.ClearAllBuffers();

			gWorld.Shutdown( SHUTDOWN_FOR_NEW_GAME, false );
			gUIGame.Deinit();
			// on exiting, let's uncache all bin fuel
			FastFuelHandle root( "root" );
			root.GetDb()->Unload();
			gWorld.Init();
			gWorld.InitServer( true );
		}

		if( !Server::DoesSingletonExist() )	// $$$ temp catch-all -bk
		{
			gWorld.InitServer( false );
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	//
	//	process the actual transition
	//

	// on transition from character select screen, set the player name
	if( ( from == WS_SP_CHARACTER_SELECT ) )
	{
		gpassert( gServer.GetScreenPlayer() );
		if( !gUIFrontend.GetEnteredName().empty() )
		{
			gServer.GetScreenPlayer()->RSSetName( gUIFrontend.GetEnteredName() );
			gServer.GetScreenPlayer()->RSSetHeroName( gUIFrontend.GetEnteredName() );
		}
	}

	switch( to )
	{
		case WS_INTRO:
		{
			GPSTATS_ONLY( gpstats::SetCurrentStage( STAGE_FRONT_END ) );

			if( !gConfig.GetBool( "nointro" ) )
			{
				gGameMovie.InitializeMovieSequence( "movies:intro_movie" );
			}
			else
			{
				gWorldStateRequest( WS_MAIN_MENU );
			}
			break;
		}

		case WS_LOGO:
		{
			gUIFrontend.GuiLogoIn();
			break;
		}

		case WS_MAIN_MENU:
		{
			gpassert( !gSiegeEngine.IsLoadingThreadRunning() );

			gUIFrontend.StartFrontendMusic();

			if( gServer.IsMultiPlayer() )
			{
				// shut only the server here because we need the GoDb intact to preserve the UI Gos
				gServer.Shutdown( SHUTDOWN_FOR_NEW_GAME );
				gServer.ShutdownTransport( true );
				gServer.Init( false );
			}

			gpassert( !NetPipe::DoesSingletonExist() );

			gUIMultiplayer.DeactivateInterfaces();

			if( from != WS_SP_MAIN_MENU && from != WS_MP_PROVIDER_SELECT )
			{
				gUIFrontend.DeactivateInterfaces();
			}

			gUIFrontend.InitGui();

			if( !gUIFrontend.IsVisible() )
			{
				gUIShell.ShowInterface( "main_menu" );
				gUIFrontend.SetIsVisible( true );
			}

			gpassert( !NetPipe::DoesSingletonExist() );

			if( from == WS_SP_MAIN_MENU )
			{
				gUIFrontend.GuiBackToMain();
			}
			else if( from == WS_MP_PROVIDER_SELECT )
			{
				gUIFrontend.GuiBackToMainFromMP();
			}
			else
			{
				gUIFrontend.GuiFlyin();
			}

#if			!GP_RETAIL
			FastFuelHandle root( "root" );
			root.GetDb()->Dump();
#endif
			
			UICursor * pWait = gUIShell.LoadUICursor( "cursor_wait", "cursors" );
			if ( pWait )
			{
				pWait->SetVisible( false );
				gRapiMouse.SetWaitImage(	pWait->GetTextureIndex(), pWait->GetHotspotX(),
											pWait->GetHotspotY() );
				gRapiMouse.SetWaitStateAllowed( true );
			}

			break;
		}

		case WS_SP_MAIN_MENU:
		{
			gpassert( !gSiegeEngine.IsLoadingThreadRunning() );
            gWorldOptions.SetLagMCP( 0.0f );
			break;
		}

		case WS_SP_CHARACTER_SELECT:
		{
			gpassert( !gSiegeEngine.IsLoadingThreadRunning() );
			gpassert( !NetPipe::DoesSingletonExist() );
			gpassert( Server::DoesSingletonExist() );
			gpassert( gServer.IsSinglePlayer() );
			gpassert( gServer.GetComputerPlayer() != NULL );
			break;
		}

		case WS_LOAD_MAP:
		{
			GPDEV_ONLY( GameSaver::ClearSaveHistory() );

			// first go to black while we shut everything down
			gDefaultRapi.ClearAllBuffers();

			gUIFrontend.StopFrontendMusic();

			// create a default world frustum for this player so they can load the world
			if( gServer.IsLocal() )
			{
				gpassert( gWorldState.GetCurrentState() != WS_MP_INGAME_JIP );
				Server::PlayerColl::const_iterator i, ibegin = gServer.GetPlayers().begin(), iend = gServer.GetPlayers().end();
				for( i = ibegin ; i != iend ; ++i )
				{
					if( IsValid( *i ) )
					{
						(*i)->SCreateInitialFrustum();
					}
				}
			}

			if( ::IsMultiPlayer() )
			{
				gUI.GetUIMultiplayer().HideInterfaces();
				gUI.GetUIMultiplayer().SetIsVisible( false );
				gNetPipe.SetConnectionTimeout( 60 );
			}

			gWorldTime.ClearTime();
			gSoundManager.SetStasis( true );

			// state transitions are a great place to assert critical points
			gpassert( !gUI.GetUIGame().IsVisible() );

			// We have to wipe out these go's so we don't waste all that time resizing graphics we don't need on res switch
			if( !::IsMultiPlayer() )
			{
				gUIFrontend.UnloadData();
				gUIFrontend.DeactivateInterfaces();
			}

			// flamethrower needs to commit any deletions it has
			gWorldFx.Update( 0, 0 );

			// switch to in-game res
			gAppModule.SetGameSizeFromConfig();			

			// must have a map
			gpassert( gWorldMap.IsInitialized() );

			// start loading now
			if ( gServer.GetLocalHumanPlayer() && gServer.GetLocalHumanPlayer()->IsJIP() )
			{
				gUIGame.SetJipLoad( true );
			}
			else
			{
				gUIGame.SetJipLoad( false );
			}
			StartProgress( true );		

			break;
		};

		case WS_LOADING_MAP:
		{
			GPSTATS_ONLY( gpstats::SetCurrentStage( STAGE_LOAD_MAP ) );

			// while everyone is getting the map ready, only permit progress updates to come through
			if( ::IsServerRemote() )
			{
				gFuBiSysExports.EnableRpcBuffering();
				gFuBiSysExports.AddRpcBufferExceptions( "Server::*" );
				gFuBiSysExports.AddRpcBufferExceptions( "Player::*" );
				gFuBiSysExports.AddRpcBufferExceptions( "WorldTime::*" );
			}

			// most goes to map loading, the rest is gui finishing work
			RatioSample ratioSample( "load_map", 0, 0.95 );

			// load our indexes
			{
				RatioSample ratioSample( "", 0, 0.21 );
				gWorldMap.CheckIndexesLoaded();
			}

			if( gWorldState.IsStateChangePending() )
			{
				break;
			}

			// queue up load requests
			{
				RatioSample ratioSample( "", 0, ::IsServerRemote() ? 0.44 : 0.74 );
				gWorldTerrain.ForceWorldLoad( true, true );
			}
			
			if( gWorldState.IsStateChangePending() )
			{
				break;
			}

			// commit incoming rpc's to create go's
			if( ::IsServerRemote() )
			{
				RatioSample ratioSample( "create_rpc_gos", 0, 0.30 );
				m_Progress->SetPermitRpcDispatch( false );
				gFuBiSysExports.DisableRpcBuffering();
				m_Progress->SetPermitRpcDispatch( true );
			}

			gpassert( !gWorldState.IsStateChangePending() );

			// commit any pending go's
			{
				RatioSample ratioSample( "commit_godb" );
				gGoDb.CommitAllRequests();
			}

			gpassert( !gWorldState.IsStateChangePending() );

			// Update static lighting
			siege::SiegeFrustum* pFrustum = gSiegeEngine.GetFrustum( gSiegeEngine.GetRenderFrustum() );
			if( pFrustum )
			{
				for( siege::FrustumNodeColl::iterator n = pFrustum->GetFrustumNodeColl().begin(); n != pFrustum->GetFrustumNodeColl().end(); ++n )
				{
					(*n)->RequestLightingAccum( true );
				}
			}

			gpassert( !gWorldState.IsStateChangePending() );

			// Update timed light color
			gSiegeEngine.LightDatabase().SetTimedLightColor( gTimeOfDay.GetCurrentTimeLightColor() );

			if( !gWorldState.IsStateChangePending() )
			{
				gWorldStateRequest( WS_LOADED_MAP );
			}

			gSoundManager.SetStasis( false );

			break;
		}

		case WS_WAIT_FOR_BEGIN:
		{
			gRapiMouse.SetWaitStateAllowed( false );

			gWorldTime.ClearTime();

			if( ::IsMultiPlayer() && UICharacterSelect::DoesSingletonExist() )
			{
				gUICharacterSelect.DeleteSelectionGos();
			}

			gGoDb.CommitAllRequests();

			if( ::IsSinglePlayer() )
			{
				if( gWorldMap.GetShowIntro() && from == WS_LOADED_INTRO )
				{
					// switch to in-game res
					gUIIntro.SetAsActive( false );
				}
			}

			{
				gUI.GetUIGame().SetIsVisible( true );
			}

			if( !gUI.GetUIGame().IsActive() )
			{
				RatioSample ratioSample( "load_ingame_gui" );
				gUI.GetUIGame().SetAsActive( true );
			}

			gSiegeEngine.GetCompass().SetVisible( true );

			break;
		}

		case WS_LOADED_INTRO:
		{
			gUIIntro.SetIsVisible( true );
			gUIIntro.SetAsActive( true );
			gUIIntro.FadeOutLoadingScreen();

			// have to abort our progress a bit early
			m_Progress->SetLeaveProgressBarUp();
			StopProgress();

			break;
		}

		case WS_LOADED_MAP:
		{
			Server::PlayerColl::const_iterator i, ibegin = gServer.GetPlayers().begin(), iend = gServer.GetPlayers().end();
			for( i = ibegin ; i != iend ; ++i )
			{
				if( IsValid( *i ) && (*i)->IsMarkedForSyncedDeletion() )
				{
					gServer.MarkPlayerForDeletion( (*i)->GetId() );
				}
			}
			break;
		}

		////////////////////////////////////////////////////////////////////////////////
		//

		case WS_MP_PROVIDER_SELECT:
		{
			gUIMultiplayer.DeactivateInterfaces();

            gWorldOptions.SetLagMCP( 0.0f );

			////////////////////////////////////////
			//	make sure we have a squeaky clean server

			gServer.Shutdown( SHUTDOWN_FOR_NEW_GAME );
			gServer.Init( true );

			////////////////////////////////////////
			//	state validation

			gpassert( NetPipe::DoesSingletonExist() );
			gpassert( Server::DoesSingletonExist() );
			gpassert( gServer.IsMultiPlayer() );
			gpassert( gServer.GetComputerPlayer() != NULL );
			gpassert( gServer.GetLocalHumanPlayer() == NULL );

			gRapiMouse.SetWaitStateAllowed( true );

			break;
		}

		////////////////////////////////////////////////////////////////////////////////
		//	transition to MP provider states

		case WS_MP_INTERNET_GAME:
		case WS_MP_LAN_GAME:
		case WS_MP_MATCH_MAKER:
		{
			////////////////////////////////////////
			//	state validation

			gpassert( !gSiegeEngine.IsLoadingThreadRunning() );
			gpassert( Server::DoesSingletonExist() );

			if ( from == WS_INIT )
			{
				gServer.Shutdown( SHUTDOWN_FOR_NEW_GAME );
				gServer.Init( true );
			}

			if( !gServer.IsInitialized() )
			{
				gServer.Init( true );
			}

			gpassert( gServer.IsMultiPlayer() );
			gpassert( NetPipe::DoesSingletonExist() );
			gpassert( gServer.GetComputerPlayer() != NULL );
			gpassert( gServer.GetLocalHumanPlayer() == NULL );

			////////////////////////////////////////
			//	init GUI

			gUIFrontend.UnloadData();
			gUIFrontend.DeactivateInterfaces();

			gUIMultiplayer.ActivateInterfaces();
			gUIMultiplayer.SetIsVisible( true );

			////////////////////////////////////////
			//

			switch( to )
			{
				case WS_MP_INTERNET_GAME:
				{
					gNetPipe.SetCompressed( true );
					gNetFuBiSend.SetSendDelay( 0.20f );
					m_MPGameType = WS_MP_INTERNET_GAME;
					gUIMultiplayer.ShowInternetGamesInterface();
				}
				break;

				case WS_MP_LAN_GAME:
				{
					gNetPipe.SetCompressed( false );
					gNetFuBiSend.SetSendDelay( 0.016f );	// limit delay to 60fps
					m_MPGameType = WS_MP_LAN_GAME;
					gUIMultiplayer.ShowLanGamesInterface();
				}
				break;

				case WS_MP_MATCH_MAKER:
				{
					gNetPipe.SetCompressed( true );
					gNetFuBiSend.SetSendDelay( 0.20f );
					m_MPGameType = WS_MP_MATCH_MAKER;
					gUIMultiplayer.ShowMatchMakerInterface();

					if ( from == WS_MP_SESSION_LOST )
					{
						gUIZoneMatch.OnExitStagingArea();
					}
				}
				break;
			};
/*
			if( (from == WS_MP_SESSION_LOST) && !gUIMultiplayer.RequestedLeaveGame() )
			{
				// if we lost the session in game, we show this message after the ui has been reactivated
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "You have been disconnected from the game." ) );
			}
*/
			break;
		}

		////////////////////////////////////////////////////////////////////////////////
		//

		case WS_MP_STAGING_AREA_SERVER:
		case WS_MP_STAGING_AREA_CLIENT:
		{
			gUIFrontend.StartFrontendMusic();

			////////////////////////////////////////
			//	state validation			

			gpassert( !gSiegeEngine.IsLoadingThreadRunning() );
			gpassert( Server::DoesSingletonExist() );
			gpassert( gServer.IsMultiPlayer() );
			gpassert( NetPipe::DoesSingletonExist() );
			gpassert( gServer.GetComputerPlayer() != NULL );

			if( ((from == WS_MP_INGAME) || (from == WS_GAME_ENDED )) )
			{
				// first go to black while we shut everything down
				gDefaultRapi.ClearAllBuffers();

				gWorld.Shutdown( SHUTDOWN_FOR_NEW_MAP, false );
				gServer.Init( true );
			}

			gNetPipe.SetConnectionTimeout( 20 );

			NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

#if !GP_RETAIL
			
			if( !session )
			{
				gperror( "UI FLOW ERROR: at this point we must have an open session.  Preceeding code should not set state to WS_MP_STAGING_AREA unless session is open.  Exe will crash now." );
			}
			
/*			if( !IsInGame( from ) && gNetPipe.GetOpenSession()->IsHost() )
			{
				gpassert( gNetPipe.GetOpenSession()->IsLocked() );
			}
*/
#endif

			////////////////////////////////////////
			//	

			gUIMultiplayer.ActivateInterfaces();
			gUIMultiplayer.SetIsVisible( true );

			if( to == WS_MP_STAGING_AREA_SERVER )
			{
				gUIMultiplayer.ShowStagingAreaServer();
			}
			else
			{
				gUIMultiplayer.ShowStagingAreaClient();
			}

			// Make sure the session still exists.
			NetPipeSessionHandle openSession = gNetPipe.GetOpenSessionHandle();

			if( openSession )  // if the staging area failed, it will close the open session, process SESSION_LOST
			{
				if( openSession->IsHost() )
				{
					openSession->SetIsLocked( false );
				}

				// MP games always played at same speed for added safety
				gWorldOptions.SetGameSpeed( 1.0f );
			}
			break;
		}

		////////////////////////////////////////////////////////////////////////////////
		//	enter in-game state

		case WS_SP_INGAME:
		case WS_MP_INGAME:
		{
			// Do not allow the wait state cursor to show in-game.  The call to disable this in WS_WAIT_FOR_BEGIN 
			// doesn't catch all cases, like when continuing or loading from the frontend.
			gRapiMouse.SetWaitStateAllowed( false );

			if( from != WS_SP_OUTRO )
			{
				gpassert( gSiegeEngine.IsLoadingThreadRunning() );
				gUIMenuManager.GuiPause( gTattooGame.IsUserPaused() );
			}
			else
			{
				gUIOutro.SetIsVisible( false );
				gUIOutro.SetAsActive( false );
				gUIOutro.SetLastGameSize();
				gUIGame.GetGameInputBinder().SetIsActive( true );
				gUIGame.GetUserInputBinder().SetIsActive( true );
				gUI.GetUIGame().SetIsVisible( true );
				gUIShell.ShowInterface( "cursors" );
				gUIShell.MarkInterfaceForActivation( "ui:interfaces:backend:end_game", true );
				
				// restart the loader
				gSiegeEngine.StartLoadingThread();
			}

			GPSTATS_ONLY( gpstats::SetCurrentStage( STAGE_IN_GAME ) );
			gpassertm( (to==WS_SP_INGAME) || UICharacterSelect::DoesSingletonExist(), "UICharacterSelect must always be present in MP ( for JIP )" );

			if( from == WS_WAIT_FOR_BEGIN )
			{
				////////////////////////////////////////
				//	SP + MP

				gWorldTime.ClearTime();
				gpassert( gWorldTime.GetTime() == 0 );

				////////////////////////////////////////
				//	load bar bye bye

				StopProgress();
				gDefaultRapi.SetFade( 0 );
				gUIFrontend.DeactivateInterfaces();
				gUIMultiplayer.DeactivateInterfaces();
	
				// just in case it's left over from the intro
				gUIShell.DeactivateInterface( "load_bar" );

				gUI.GetUIGame().GetGameInputBinder().SetIsActive( true );
				gUI.GetUIGame().GetGameInputBinder().SetInputRepeatEnabled( IsSinglePlayer() );
				gUI.GetUIGame().GetUserInputBinder().SetIsActive( true );
				gUI.GetUIGame().GetUserInputBinder().SetInputRepeatEnabled( IsSinglePlayer() );

				gMessageDispatch.Update();
				gTimeMgr.AddRapiFader( true, 2.5f );

				// skip a couple frames to account for pending state changes
				// and a possible initial nis
				gTattooGame.SetRenderSkip( 2 );
			}
			else if( from == WS_MP_INGAME_JIP )
			{
				if ( ::IsServerLocal() )
				{
					gTattooGame.SPause( false );
				}
				gTattooGame.Pause( false );
				gUIMenuManager.GuiPause( gTattooGame.IsUserPaused() );

				gUIMultiplayer.HideJIPInterface();

				gUIShell.HideInterface( "jip_glass" );
			}

			// MP: on entry into game, open session for JIP, if allowed
			if( gServer.IsMultiPlayer() )
			{
				if( gServer.GetAllowJIP() )
				{
					NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
					session->SetIsLocked( false );
				}
				gNetPipe.SetConnectionTimeout( 20 );
			}

			gUIGameConsole.EnableConsoleUpdate( true );
			gUIAnimation.SetDisableUpdate( false );

			break;
		}

		////////////////////////////////////////////////////////////////////////////////
		//	enter ingame JIP state...

		case WS_MP_INGAME_JIP:
		{
			gpassert( (from == WS_MP_INGAME) || ( from == WS_MEGA_MAP ) );
			gpgeneric( "\n\n******* Entered JIP mode\n\n" );

			gUIMultiplayer.ShowJIPInterface();

			gTattooGame.Pause( true );
			gUIMenuManager.GuiPause( true );

			gUIPartyManager.CloseAllCharacterMenus();

			gUIShell.ShowInterface( "jip_glass" );

			gWorldOptions.SetDisableHighlight( true );

			UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "exit_game_button" );
			if ( pButton && ::IsServerLocal() )
			{
				pButton->SetVisible( false );
			}

			if( gServer.IsLocal() )
			{
				Server::PlayerColl::const_iterator i, ibegin = gServer.GetPlayers().begin(), iend = gServer.GetPlayers().end();
				for( i = ibegin ; i != iend ; ++i )
				{
					if( IsValid( *i ) )
					{
						(*i)->SetLastServerAssignedWorldState( WS_INVALID );
					}
				}

				m_LastTimeJipPlayerPresent = GetGlobalSeconds();
			}

			break;
		}

		////////////////////////////////////////////////////////////////////////////////
		//

		case WS_LOADING_SAVE_GAME:
		{			
			GameLoader loader( GameLoader::OPTION_WORLD_SAVE );
			gpwstring saveGame = gTattooGame.TakeLastSaveGame();
			if( loader.BeginLoad( saveGame ) )
			{
				// first go to black while we shut these things down
				gDefaultRapi.ClearAllBuffers();

				// shut down the front end and game gui if it's up
				bool fromFrontEnd = gUIFrontend.IsVisible();
				if( fromFrontEnd )
				{
					gUIFrontend.UnloadData();				
					gUIFrontend.DeactivateInterfaces();					
				}
				else
				{
					gUIGame.SetAsActive( false );
					gUIGame.Deinit();
				}

				// Shut down sounds and put system in stasis
				gSoundManager.StopAllSounds();
				gSoundManager.SetStasis( true );

				// nuke other in-game stuff
				gUIPartyManager.DeconstructUIParty();

				// switch to in-game res
				gAppModule.SetGameSizeFromConfig();

				GPSTATS_ONLY( gpstats::SetCurrentStage( STAGE_LOAD_GAME ) );

				// start loading now
				StartProgress( true );

				// build our UI
				{
					RatioSample ratioSample( "load_ingame_gui", 0, fromFrontEnd ? 0.10 : 0.01 );
					gUIGame.SetAsActive( true );
				}

				// load 'er up
				{
					RatioSample ratioSample( "load_world" );
					loader.LoadWorld();

					gTattooGame.ResetUserInputs();
				}

				if( GoDb::DoesSingletonExist() )
				{
					gGoDb.SetSelectionChangedCb( makeFunctor( *(UIGame::GetSingletonPtr()), &UIGame::SelectionChanged ) );
				}

				gUIShell.HideInterface( "defeat_menu" );
				gUIGame.GetGameInputBinder().SetIsActive( true );
				gUIGame.GetUserInputBinder().SetIsActive( true );
				gUIMenuManager.GuiPause( gAppModule.IsUserPaused() );

				StopProgress();

				gUIPartyManager.PostXfer();

				// Release stasis on sound system
				gSoundManager.SetStasis( false );

				// restart the loader
				gSiegeEngine.StartLoadingThread();

				// back to ingame
				gWorldStateRequest( WS_SP_INGAME );
				if( loader.GetLoadedWorldState() != WS_SP_INGAME )
				{
					// force-go to the state we saved at
					gWorldState.Update();
					gWorldStateRequest( loader.GetLoadedWorldState() );
				}

				// report
				gpscreen( $MSG$ "Game loaded successfully\n" );
			}
			else
			{
				if ( ::IsInGame( from ) )
				{
					gUIGame.ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "Game failed to load." ) );
				}
				else
				{
					gUIFrontend.ShowDialog( gpwtranslate( $MSG$ "Game failed to load." ) );					
				}				
				
				gWorldStateRequest( (from == WS_INIT) ? WS_INTRO : from );
			}

			break;
		}

		case WS_SP_NIS:
		{			
			// enter nis mode
			gAppModule.ForceResume();
			gAppModule.UserResume();
			gUICamera.SetCameraMode( CMODE_NIS );
			gCameraAgent.SetInterpolationMode( CI_TIME );
			gCameraAgent.ClearOrders();
			gUICamera.SetIgnoreInput( true );
			gUIGame.SetSelectionBox( false );
			gSiegeEngine.GetCompass().SetVisible( false );
			gUI.GetUIGame().GetGameInputBinder().SetIsActive( false );
			gUI.GetUIGame().GetUserInputBinder().SetIsActive( false );
			gUI.GetUIGame().GetNisInputBinder().SetIsActive( true );
			gUIGameConsole.EnableConsoleUpdate( false );
			gUIAnimation.SetDisableUpdate( true );
			gUIDialogueHandler.StopQuestDialogue( true );
			break;
		}

		case WS_MEGA_MAP:
		{
			gUIMenuManager.ActivateMegaMap();			
			break;
		}

		case WS_SP_DEFEAT:
		{
			gUIGame.GetGameInputBinder().SetIsActive( false );
			gUIGame.GetUserInputBinder().SetIsActive( false );
			gTattooGame.RSUserPause( true );
			break;
		}

		case WS_SP_VICTORY:
		{
			gUIGame.GetGameInputBinder().SetIsActive( false );
			gUIGame.GetUserInputBinder().SetIsActive( false );

			gUI.GetUIGame().SetIsVisible( false );
			// gUI.GetUIGame().SetAsActive( false );
			gUIShell.HideInterface( "cursors" );

			if( gWorldMap.GetShowIntro() )
			{
				gUIOutro.SetIsVisible( true );
				gUIOutro.SetAsActive( true );
			}
			else
			{
				gWorldStateRequest( WS_MAIN_MENU );
			}

			break;
		}

		case WS_SP_OUTRO:
		{
			gUIPartyManager.CloseAllCharacterMenus();
			gUIOutro.SetOutroGameSize();
			break;
		}

		case WS_GAME_ENDED:
		{
			if( gSiegeEngine.IsLoadingThreadRunning() )
			{
				gSiegeEngine.StopLoadingThread();
			}

			if( ::IsSinglePlayer() )
			{
				if( gVictory.GameEnded() )
				{
					gWorldStateRequest( WS_SP_VICTORY );
				}
				else
				{
					gWorldStateRequest( WS_MAIN_MENU );
				}
			}
			else // multiplayer
			{
				gVictory.SetGameEnded( true );

				gpassert( !gSiegeEngine.IsLoadingThreadRunning() );

				gNetPipe.SetConnectionTimeout( 90 );	// 90 second to get through this state before timing out

				NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

				if( session && session->IsHost() )
				{
					session->SetIsLocked( true );
				}

				if ( gUIZoneMatch.IsHosting() )
				{
					gUIZoneMatch.OnEndGame();
				}

				gUI.GetUIGame().SetAsActive( false );
				gUI.GetUIGame().SetIsVisible( false );
				gUI.GetUIGame().GetGameInputBinder().SetIsActive( false );
				gUI.GetUIGame().GetUserInputBinder().SetIsActive( false );
		
				gNetFuBiSend.SetMute( true );
				gNetFuBiReceive.SetMute( true );

				//	shut down all the gos to kill the textures before the res change so they won't get rebuilt for no good reason
				gGoDb.Shutdown();

				if( gUIMenuManager.GetRequestedClientExit() )
				{
					gNetPipe.CloseOpenSession( false );
				}

				gAppModule.SetGameSize( GSize( 800, 600 ) );

				gUIMultiplayer.ActivateEndGameScreen();

				gSoundManager.StopAllSounds();

				gNetFuBiSend.SetMute( false );
				gNetFuBiReceive.SetMute( false );

				gUIFrontend.StartFrontendMusic();
			}
			break;
		}

		case WS_MP_SESSION_LOST:
		{
			gpassert( !gSiegeEngine.IsLoadingThreadRunning() );

			gFuBiSysExports.DisableRpcBuffering( false );
			gFuBiSysExports.FlushRpcBuffers( true );

			gServer.Shutdown( SHUTDOWN_FOR_NEW_MAP );

			if(	from == WS_LOADING_MAP ||
				from == WS_LOADED_MAP ||
				from == WS_LOAD_MAP ||
				from == WS_WAIT_FOR_BEGIN )
			{
				////////////////////////////////////////
				//	load bar bye bye

				StopProgress();
				gUIGame.Deinit();
				gUIFrontend.DeactivateInterfaces();
				gUIMultiplayer.DeactivateInterfaces();
	
				// just in case it's left over from the intro
				gUIShell.DeactivateInterface( "multiplayer_load" );

				gUI.GetUIGame().GetGameInputBinder().SetIsActive( true );
				gUI.GetUIGame().GetGameInputBinder().SetInputRepeatEnabled( IsSinglePlayer() );
				gUI.GetUIGame().GetUserInputBinder().SetIsActive( true );
				gUI.GetUIGame().GetUserInputBinder().SetInputRepeatEnabled( IsSinglePlayer() );

				gMessageDispatch.Update();			/// $$ why is this here?
			}

			if(	from == WS_MP_INGAME ||
				from == WS_MP_INGAME_JIP ||
				from == WS_MP_STAGING_AREA_SERVER || 
				from == WS_MP_STAGING_AREA_CLIENT || 
				from == WS_LOADING_MAP || 
				from == WS_LOADED_MAP ||
				from == WS_LOAD_MAP ||
				from == WS_WAIT_FOR_BEGIN )
			{
				if ( IsInGame( from ) && gUIMultiplayer.RequestedLeaveGame() )
				{
					gWorldStateRequest( WS_GAME_ENDED );
				}
				else
				{
					// Don't want to update the gui in-between frames.
					gUIGame.SetAsActive( false );		
					gUIGame.SetIsVisible( false );
				
					if( gUIMultiplayer.GetMultiplayerMode() == MM_GUN )
					{
						gWorldStateRequest( WS_MP_MATCH_MAKER );
					}
					else if( gUIMultiplayer.GetMultiplayerMode() == MM_NET )
					{
						gWorldStateRequest( WS_MP_INTERNET_GAME );
					}
					else if( gUIMultiplayer.GetMultiplayerMode() == MM_LAN )
					{
						gWorldStateRequest( WS_MP_LAN_GAME );
					}
				}
			}
			else if( from == WS_GAME_ENDED )
			{
				gUIMultiplayer.EndGameDisableChat();
			}
			break;
		}

		case WS_DEINIT:
		{
			gGoDb.Shutdown();
			break;
		}
	}

	switch( from )
	{
		case WS_SP_INGAME:
		case WS_MP_INGAME:
		{
			if( gUICamera.GetFreeLook() )
			{
				gUICamera.SetFreeLook( false );
			}
			break;
		}

		case WS_SP_NIS:
		{
			// reset the interpolation mode
			gCameraAgent.SetInterpolationMode( CI_SMOOTH );

			// exit nis mode and force an update
			gUICamera.SetCameraMode( gUICamera.GetPreviousMode(), false );
			gUICamera.Update( 0 );

			// clear camera agent
			gCameraAgent.ResetCameraAffectorModification();
			gCameraAgent.ResetTargetAffectorModification();

			// take current as history
			gSiegeEngine.GetCamera().FillCameraHistoryWithCurrent();
			gSiegeEngine.GetCamera().FillTargetHistoryWithCurrent();

			// re-enable input
			gUICamera.SetIgnoreInput( false );
			gUI.GetUIGame().GetGameInputBinder().SetIsActive( true );
			gUI.GetUIGame().GetUserInputBinder().SetIsActive( true );
			gUI.GetUIGame().GetNisInputBinder().SetIsActive( false );

			// reset other happy things
			gUIDialogueHandler.ExitDialogue();
			gSiegeEngine.GetCompass().SetVisible( true );
			gUIGame.SetEscNisToFrontEnd( false );

			break;
		}

		case WS_MP_STAGING_AREA_SERVER:
		case WS_MP_STAGING_AREA_CLIENT:
		{
			if( to == WS_LOAD_MAP )
			{
				gUIMultiplayer.SavePlayerSettings();
			}
			break;
		}

		case WS_MEGA_MAP:
		{
			gUIMenuManager.DeactivateMegaMap();
			break;
		}

		case WS_SP_DEFEAT:
		{
			if( to == WS_SP_INGAME )
			{
				gUI.GetUIGame().GetGameInputBinder().SetIsActive( true );
				gUI.GetUIGame().GetUserInputBinder().SetIsActive( true );

				gUIShell.HideInterface( "defeat_menu" );
			}
			break;
		}

		case WS_MP_INGAME_JIP:
		{
			gUIMultiplayer.HideJIPInterface();

			gWorldOptions.SetDisableHighlight( false );
			UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "exit_game_button" );
			if ( pButton && ::IsServerLocal() )
			{
				pButton->SetVisible( true );		
			}
			break;
		}			
	}

	// notify server of our change in state
	if( Server::DoesSingletonExist() )
	{
		gServer.OnWorldStateChange( to );
	}
}


////////////////////////////////////////////////////////////////////////////////
//
void GameState :: HandleBroadcast( WorldMessage & msg )
{
	////////////////////////////////////////
	//	early-out

	if( !Server::DoesSingletonExist() || !gServer.IsLocal() )
	{
		return;
	}

	////////////////////////////////////////
	//	check auto-save party

	// lost session?? save the party quick before we lose the players!
	if ( msg.HasEvent( WE_MP_SESSION_TERMINATED ) )
	{
		gTattooGame.CheckAutoSaveParty( true );
	}

	////////////////////////////////////////
	//	check for synched world states

	DWORD playerCount						= 0;
	DWORD worldStatePlayerMatchCount		= 0;

	DWORD jipPlayerCount					= 0;
	DWORD worldStateJIPPlayerMatchCount		= 0;

	eWorldState localState		= gWorldState.GetCurrentState();
	eWorldState firstJIPState	= WS_INVALID;

	Server::PlayerColl::const_iterator i, ibegin = gServer.GetPlayers().begin(), iend = gServer.GetPlayers().end();

	for( i = ibegin ; i != iend ; ++i )
	{
		if( IsValid( *i ) )
		{
			++playerCount;

			if( (*i)->GetWorldState() == localState )
			{
				++worldStatePlayerMatchCount;
			}

			if( (*i)->IsJIP() )
			{
				++jipPlayerCount;

				if( firstJIPState == WS_INVALID )
				{
					firstJIPState = (*i)->GetWorldState();
					worldStateJIPPlayerMatchCount = 1;
				}
				else
				{
					if( (*i)->GetWorldState() == firstJIPState )
					{
						++worldStateJIPPlayerMatchCount;
					}
				}
			}
		}
	}

	bool jipPlayersPresent				= jipPlayerCount > 0;
	bool allPlayerWorldStateMatch		= playerCount == worldStatePlayerMatchCount;
	bool allJIPPlayerWorldStateMatch	= jipPlayersPresent && ( jipPlayerCount == worldStateJIPPlayerMatchCount );
	eWorldState allJIPPlayerWorldState	= allJIPPlayerWorldStateMatch ? firstJIPState : WS_INVALID;


	//	special case - if we have machines connected but no players for them yet, assume they will be jipping... if something
	//	happens to go wrong, the timeout will boot the machine... but until then, consider them valid jippers

	if( gServer.IsMultiPlayer() && NetPipe::DoesSingletonExist() )
	{
		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
		if( session )
		{
			for( NetPipeMachineColl::iterator i = session->GetMachinesBegin(); i != session->GetMachinesEnd(); ++i )
			{
				gpassert( (*i)->IsRemote() );
				if( (*i)->IsRemote() && !gServer.HasPlayersOnMachine( (*i)->m_Id ) )
				{
					jipPlayersPresent = true;
					break;
				}
			}
		}
	}


	if( jipPlayersPresent )
	{
		m_LastTimeJipPlayerPresent = GetGlobalSeconds();
	}

	////////////////////////////////////////
	//	SP

	if( gServer.IsSinglePlayer() )
	{
		switch( msg.GetEvent() )
		{
			case WE_WORLD_STATE_TRANSITION_DONE:
			{
				gpassert( gServer.IsLocal() );

				// check for advancement. first all clients must get to WS_LOADED_MAP,
				// then we tell them to WS_WAIT_FOR_BEGIN, then we tell them to go
				// into the game.

				if( (localState == WS_LOAD_MAP) || (localState == WS_LOADED_MAP) || (localState == WS_WAIT_FOR_BEGIN) )
				{
					if( allPlayerWorldStateMatch )
					{
						eWorldState next;

						if( localState == WS_LOAD_MAP )
						{
							gWorldState.SSetWorldStateOnMachine( RPC_TO_LOCAL, localState, WS_LOADING_MAP );
						}
						else if( localState == WS_LOADED_MAP )
						{
							next = WS_WAIT_FOR_BEGIN;
							gServer.SPlaceHeroesInWorld();
							if( gWorldMap.GetShowIntro() && !gConfig.GetBool( "noloadintro" ) )
							{
								next = WS_LOADED_INTRO;
							}
							gWorldState.SSetWorldStateOnMachine( RPC_TO_LOCAL, localState, next );
						}
						else if( localState == WS_WAIT_FOR_BEGIN )
						{
							gWorldState.SSetWorldStateOnMachine( RPC_TO_LOCAL, localState, WS_SP_INGAME );
						}
					}
				}
				break;
			}
		};
	}

	////////////////////////////////////////
	//	MP

	if( gServer.IsMultiPlayer() )
	{
		////////////////////////////////////////////////////////////////////////////////
		//	normal MP events

		// if we're loading...
		if( gWorldState.GetCurrentState() != WS_MP_INGAME_JIP )
		{
			switch( msg.GetEvent() )
			{
				////////////////////////////////////////
				//	WE_MP_PLAYER_WORLD_STATE_CHANGED
				//	WE_MP_MACHINE_DISCONNECTED

				case WE_MP_MACHINE_DISCONNECTED:
				case WE_MP_PLAYER_WORLD_STATE_CHANGED:
				{
					gpassert( gServer.IsLocal() );

					if( (localState == WS_LOAD_MAP) || (localState == WS_LOADED_MAP) || (localState == WS_WAIT_FOR_BEGIN) )
					{
						if( allPlayerWorldStateMatch )
						{
							if( localState == WS_LOAD_MAP )
							{
								gWorldState.SSetWorldStateOnMachine( RPC_TO_ALL, localState, WS_LOADING_MAP );
							}
							else if( localState == WS_LOADED_MAP )
							{
								gServer.SPlaceHeroesInWorld();
								gWorldState.SSetWorldStateOnMachine( RPC_TO_ALL, localState, WS_WAIT_FOR_BEGIN );
							}
							else if( localState == WS_WAIT_FOR_BEGIN )
							{
								gWorldState.SSetWorldStateOnMachine( RPC_TO_ALL, localState, WS_MP_INGAME );
							}
						}
					}
					break;
				}

				////////////////////////////////////////
				//	WE_MP_PLAYER_DESTROYED

				case WE_MP_PLAYER_DESTROYED:
				{
					if ( IsInGame( gWorldState.GetCurrentState() ) )
					{
						if ( gServer.IsLocal() )
						{
							gVictory.RCClearPlayerStats( MakePlayerId( msg.GetData1() ) );
						}
					}
					break;
				}
			}
		}
		////////////////////////////////////////////////////////////////////////////////
		//	JIP MP events
		else if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
		{
			switch( msg.GetEvent() )
			{
				case WE_MP_PLAYER_DESTROYED:
				{
					gpgeneric( "JIP: GameState :: HandleBroadcast( WE_MP_PLAYER_DESTROYED )\n" );
					gpgenericf(( "\tjipPlayersPresent = %s : allPlayerWorldStateMatch = %s : allJIPPlayerWorldStateMatch = %s : allJIPPlayerWorldState = %s\n",
								jipPlayersPresent ? "TRUE" : "FALSE",
								allPlayerWorldStateMatch ? "TRUE" : "FALSE",
								allJIPPlayerWorldStateMatch ? "TRUE" : "FLASE",
								ToString( allJIPPlayerWorldState ) ));

					if( !jipPlayersPresent )
					{
						gWorldState.SSetWorldStateOnMachine( RPC_TO_ALL, localState, WS_MP_INGAME );
					}
					break;
				}

				////////////////////////////////////////
				//	WE_MP_PLAYER_READY

				case WE_MP_PLAYER_READY:
				{
					Player * player = gServer.GetPlayer( MakePlayerId( msg.GetData1() ) );
					gpassert( player );
					if( IsInGame( gWorldState.GetCurrentState() ) && player->IsJIP() )
					{
						gpgenericf(( "JIP: player %S ready\n", player->GetName().c_str() ));

						gWorldState.SSetWorldStateOnMachine( player->GetMachineId(), WS_MP_STAGING_AREA_CLIENT, WS_LOAD_MAP );
					}
					break;
				}

				////////////////////////////////////////
				//	WE_MP_PLAYER_WORLD_STATE_CHANGED

				//	This event will be received whenever a Player claims their worldstate changed.  For remote players,
				//	this is the result of an RPC being received that tells us the remote Player's WorldState has changed.
				case WE_MP_PLAYER_WORLD_STATE_CHANGED:
				{
					gpgeneric( "JIP: GameState :: HandleBroadcast( WE_MP_PLAYER_WORLD_STATE_CHANGED )\n" );

					gpgenericf(( "\tjipPlayersPresent = %s : allPlayerWorldStateMatch = %s : allJIPPlayerWorldStateMatch = %s : allJIPPlayerWorldState = %s\n",
								jipPlayersPresent ? "TRUE" : "FALSE",
								allPlayerWorldStateMatch ? "TRUE" : "FALSE",
								allJIPPlayerWorldStateMatch ? "TRUE" : "FLASE",
								ToString( allJIPPlayerWorldState ) ));

					Player * player = gServer.GetPlayer( MakePlayerId( msg.GetData1() ) );
					gpassert( player );

					if( IsInGame( gWorldState.GetCurrentState() ) )
					{
						gpassert( player );

						// move remote machine from WS_LOAD_MAP to WS_LOADING_MAP
						if( player->IsJIP() && ( player->GetWorldState() == WS_LOAD_MAP ) )
						{
							gpassert( player->IsRemote() );

							player->SCreateInitialFrustum();
							gGoDb.ClientSyncAllClientsGos( player );

							gWorldState.SSetWorldStateOnMachine( player->GetMachineId(), WS_LOAD_MAP, WS_LOADING_MAP );
						}
					}
					break;
				}

				////////////////////////////////////////
				//	WE_MP_TERRAIN_FULLY_LOADED

				case WE_MP_TERRAIN_FULLY_LOADED:
				{
					gpassert( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP );
					CHECK_SERVER_ONLY;

					// Since all nodes have been loaded, it's implied that the new JIP player(s) have had all the Go synch and
					// creation RPCs sent.
					// Since there is an unkown amount of data queued to be sent to the remote JIP machine, we
					// will manually tell remote JIP machine to cahange state.  We'll get an event when remote machine
					// changes state.  When we get this event, it's implied that there is no more data queued to be sent to
					// this JIP machine.

					if( allJIPPlayerWorldStateMatch )
					{
						//
						//	once ALL jipers have finished loading
						//	+	place all their heros into the world
						//	+	lock out session util we're all in-game again
						//
						if( allJIPPlayerWorldState == WS_LOADED_MAP )
						{
							gpgeneric( "JIP: GameState :: HandleBroadcast( WE_MP_TERRAIN_FULLY_LOADED ) - allJIPPlayerWorldState == WS_LOADED_MAP \n" );

							NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
							if( session && !session->IsLocked() )
							{
								session->SetIsLocked( true );
							}

							Server::PlayerColl::const_iterator i, ibegin = gServer.GetPlayers().begin(), iend = gServer.GetPlayers().end();
							for( i = ibegin ; i != iend ; ++i )
							{
								if( IsValid( *i ) && (*i)->IsJIP() && !(*i)->AddedPlayerToWorld() )
								{
									gpassert( (*i)->IsRemote() );
									// add this player's hero to the world
									(*i)->SAddPlayerToWorld( true );
									gWorldState.SSetWorldStateOnMachine( (*i)->GetMachineId(), WS_LOADED_MAP, WS_WAIT_FOR_BEGIN );
								}
							}
						}
						// move remote machines from WS_WAIT_FOR_BEGIN to WS_MP_INGAME when ready
						else if( allJIPPlayerWorldState == WS_WAIT_FOR_BEGIN )
						{
							gpgeneric( "JIP: GameState :: HandleBroadcast( WE_MP_TERRAIN_FULLY_LOADED ) - allJIPPlayerWorldState == WS_WAIT_FOR_BEGIN \n" );

							// if all the JIP players are waiting for begin, we know what each JIP player:
							//	+	has a frustum
							//	+	has a hero created and placed
							//	+	has had the server load his entire frustum, therefore has had all Go creation and synch RPC sent to it
							//	+	has processed all said RPCs because it was able to ACK back state change to WS_WAIT_FOR_BEGIN, which was
							//		initiated by a RPC from the server

							// when ALL JIP machines are WS_WAIT_FOR_BEGIN, move everyone to WS_MP_INGAME

							Server::PlayerColl::const_iterator i, ibegin = gServer.GetPlayers().begin(), iend = gServer.GetPlayers().end();
							for( i = ibegin ; i != iend ; ++i )
							{
								if( IsValid( *i ) && !(*i)->IsComputerPlayer() && ( (*i)->GetLastServerAssignedWorldState() != WS_MP_INGAME ) )
								{
									if( (*i)->IsJIP() )
									{
										// tell remote machine to enter ingame
										gpassert( (*i)->IsRemote() );
										gWorldState.SSetWorldStateOnMachine( (*i)->GetMachineId(), WS_WAIT_FOR_BEGIN, WS_MP_INGAME );
										(*i)->RCSetJIP( false );
									}
									else
									{
										gpassert( (*i)->GetWorldState() == WS_MP_INGAME_JIP );
										// tell non-jippers to enter back into games
										gWorldState.SSetWorldStateOnMachine( (*i)->GetMachineId(), WS_MP_INGAME_JIP, WS_MP_INGAME );
									}
								}
							}
						}
					}

					// $ watchdog
					if( !jipPlayersPresent && ( PreciseSubtract(GetGlobalSeconds(), m_LastTimeJipPlayerPresent) > 10.0 ) )
					{
						gpassert( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP );
						gpwarning( "JIP:  GameState :: HandleBroadcast( WE_MP_TERRAIN_FULLY_LOADED ) - watchdog kicking in; no Jippers, and we're still waiting.  Going back to ingame.\n" );
						gWorldState.SSetWorldStateOnMachine( RPC_TO_ALL, localState, WS_MP_INGAME );
					}

					break;
				}
			}; // switch
		} // else if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	}
}


void GameState :: StartProgress( bool forLoad )
{
	if( m_Progress != NULL )
	{
		m_Progress->AddRef();
	}
	else
	{
		m_Progress = new GameProgress( forLoad );
	}
}


void GameState :: StopProgress( void )
{
	if( (m_Progress != NULL) && (m_Progress->Release() == 0) )
	{
		Delete( m_Progress );
	}
}


//////////////////////////////////////////////////////////////////////////////
