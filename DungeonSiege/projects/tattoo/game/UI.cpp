/*=======================================================================================

  UI

  author:	Bartosz Kijanka, Chad Queen

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/

#include "Precomp_Game.h"

#include "AppModule.h"
#include "Config.h"
#include "InputBinder.h"
#include "MessageDispatch.h"
#include "NetPipe.h"
#include "Player.h"
#include "Server.h"
#include "Services.h"
#include "Siege_Console.h"
#include "TattooGame.h"
#include "UI.h"
#include "UI_Shell.h"
#include "UICamera.h"
#include "UICharacterSelect.h"
#include "UIFrontEnd.h"
#include "UIIntro.h"
#include "UIOutro.h"
#include "UIGame.h"
#include "UIMultiPlayer.h"
#include "UIPartyManager.h"
#include "UIUserConsole.h"
#include "UIMenuManager.h"
#include "UIOptions.h"
#include "UICommands.h"
#include "WorldMessage.h"
#include "WorldOptions.h"
#include "WorldState.h"
#include "WorldTime.h"
#include "LocHelp.h"


UI::UI( )
{
#	if !GP_RETAIL
	// create console later
	m_UIUserConsole = new UIUserConsole;
	m_UIUserConsole->SetIsVisible( false );
#	endif // !GP_RETAIL

	// Initialize the GameGUI
	m_GameGUI = new UIShell( gDefaultRapi );
	gAppModule.RegisterBinder( m_GameGUI->GetInputBinder(), 50 );
	m_GameGUI->GetInputBinder()->SetIsActive( true );

	// Set the screen size information
	m_GameGUI->Resize( gAppModule.GetGameWidth(), gAppModule.GetGameHeight() );

	// create game later
	m_UIGame        = new UIGame;
	m_UIFrontend    = new UIFrontend;
	m_UIMultiplayer = new UIMultiplayer;
	m_UIOptions		= new UIOptions;
	m_UIIntro		= new UIIntro;
	m_UIOutro		= new UIOutro;

	m_UIFrontend->SetIsVisible( true );
	m_UIMultiplayer->SetIsVisible( false );
	m_UIGame->SetIsVisible( false );
	m_UIOutro->SetIsVisible( false );

	m_PreTurtlePausing = false;

	gMessageDispatch.RegisterBroadcastCallback( "ui", makeFunctor( *this, &UI::HandleBroadcast ) );
}




UI::~UI()
{
	Delete ( m_UIMultiplayer       );
	Delete ( m_UIGame              );
	Delete ( m_UIFrontend          );
	Delete ( m_UIOptions           );
	Delete ( m_UIIntro			   );
	Delete ( m_UIOutro			   );
	Delete ( m_GameGUI             );
#	if !GP_RETAIL
	Delete ( m_UIUserConsole       );
#	endif // !GP_RETAIL

	if( MessageDispatch::DoesSingletonExist() )
	{
		gMessageDispatch.UnregisterBroadcastCallback( "ui" );
	}
}




void UI::Update( double seconds )
{
	GPPROFILERSAMPLE( "UI :: Update", SP_UI );

#	if !GP_RETAIL
	gFooterConsole.Clear();

	// update console layer
	if( m_UIUserConsole->IsVisible() )
	{
		m_UIUserConsole->Update( seconds );
	}
#	endif // !GP_RETAIL


	// Update UI Layers as needed
	if( m_UIFrontend->IsVisible() )
	{
		m_UIFrontend->Update( seconds );
	}
	else if ( gUIFrontend.GetBeginLoadMap() && gUIFrontend.GetSelectedMapName().size() != 0 )
	{
		gUIFrontend.BeginLoadMap();
	}
	else if( m_UIMultiplayer->IsVisible() )
	{
		m_UIMultiplayer->Update( seconds );
	}	
	else if( m_UIGame->IsVisible() )
	{
		if ( gWorldState.GetCurrentState() != WS_SP_NIS )
		{
			m_UIGame->Update( seconds );

			if( NetPipe::DoesSingletonExist() && gNetPipe.HasTurtle() )
			{
				///////////////////////////////////////////////////////////
				//	update net lag icons
				if( gServer.IsMultiPlayer() && NetPipe::DoesSingletonExist() )
				{
					if( IsInGame( gWorldState.GetCurrentState() ) && gWorldTime.GetTime() > 1.0 )
					{
						if( gServer.IsLocal() )
						{
							if( gNetPipe.GetOutboundTrafficBackupDuration() > 2.0 )
							{
								if( !gUIMenuManager.IsServerTrafficBacklogIconVisible() )
								{
									gUIMenuManager.ShowServerTrafficBacklogIcon( true );
									// Pause game here
									gTattooGame.SPause( true );
									m_PreTurtlePausing = gServer.GetAllowPausing();
									gServer.SSetAllowPausing( false );
								}
							}
							else if( gNetPipe.GetOutboundTrafficBackupDuration() < 0.2 )
							{
								if( gUIMenuManager.IsServerTrafficBacklogIconVisible() )
								{
									gUIMenuManager.ShowServerTrafficBacklogIcon( false );
									// unpause game here
									gServer.SSetAllowPausing( m_PreTurtlePausing );
									gTattooGame.SPause( false );
								}
							}
						}
						else
						{
							float serverTimeoutTime = gTattooGame.IsUserPaused() ? 5.5f : 1.5f;

							NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
							if( session && session->HasMachine( RPC_TO_SERVER ) )
							{
								if( session->GetMachine( RPC_TO_SERVER )->GetTimeElapsedSinceLastReceive() > serverTimeoutTime )
								{
									if( !gUIMenuManager.IsServerTimeoutIconVisible() )
									{
										gUIMenuManager.ShowServerTimeoutIcon( true );
									}
								}
								else if( session->GetMachine( RPC_TO_SERVER )->GetTimeElapsedSinceLastReceive() < 0.5 )
								{
									if( gUIMenuManager.IsServerTimeoutIconVisible() )
									{
										gUIMenuManager.ShowServerTimeoutIcon( false );
									}
								}
							}
						}
					}
				}
			}
		}

		gUICamera.Update( (float)seconds );
	}	
	else if ( m_UIIntro->IsVisible() )
	{
		m_UIIntro->Update( seconds );
	}
	else if ( m_UIOutro->IsVisible() )
	{
		m_UIOutro->Update( seconds );
	}

	// Handle the subtitling system, if necessary
	if ( gWorldState.GetCurrentState() == WS_SP_NIS && UIDialogueHandler::DoesSingletonExist() ) 
	{
		gUIDialogueHandler.NisUpdate( (float)seconds );
		if( !UpdateShell( seconds, gAppModule.GetCursorX(), gAppModule.GetCursorY() ) )
		{
			return;
		}
	}

	// If the options are active, let's update them as well.
	if( m_UIOptions->IsActive() )
	{
		m_UIOptions->Update( seconds );
	}	
}


void UI::HandleBroadcast( WorldMessage & msg )
{
	GPPROFILERSAMPLE( "UI :: HandleBroadcast", SP_UI );

	if( ::IsMultiPlayer() && UIMultiplayer::DoesSingletonExist() )
	{
		gUIMultiplayer.HandleBroadcast( msg );
	}
}


bool UI::UpdateShell( double seconds, int cursorX, int cursorY )
{
	eWorldState current = gWorldState.GetCurrentState();
	eWorldState pending = gWorldState.GetPendingState();

	gUIShell.Update( seconds, cursorX, cursorY );

	return( current == gWorldState.GetCurrentState() && pending == gWorldState.GetPendingState() );
}


void UI::Draw( double seconds )
{
	//GPPROFILERSAMPLE( "UI::Draw()" );

	if( gUIGame.IsVisible() )
	{
		gUIPartyManager.DrawStatusBars();
	}

#	if !GP_RETAIL
	if( m_UIUserConsole->IsVisible() )
	{
		m_UIUserConsole->Draw( seconds );
	}
#	endif // !GP_RETAIL
	
	if( m_UIFrontend->IsVisible() )
	{
		m_UIFrontend->Draw( seconds );
	}
	else if ( !m_UIFrontend->GetBeginLoadMap() && !IsInGame( gWorldState.GetCurrentState() ) )
	{
		m_UIFrontend->SetBeginLoadMap( true );
	}

	if( m_UIMultiplayer->IsVisible() )
	{
		m_UIMultiplayer->Draw( seconds );
	}
	
	if( m_UIGame->IsVisible() && gWorldState.GetCurrentState() != WS_SP_NIS )
	{
		gUIShell.SetGuiDisabled( false );
		m_UIGame->Draw( seconds );
	}
	else if ( gWorldState.GetCurrentState() == WS_SP_NIS )
	{
		gUIShell.SetGuiDisabled( true );
		gUIShell.DrawInterface( "nis_subtitle" );
		if ( gUIMenuManager.GetExitDialogActive() )
		{
			gUIShell.SetGuiDisabled( false );
			gUIGame.GetUICommands()->SetCursor( CURSOR_GUI_STANDARD );
			gUIShell.DrawInterface( "exit_dialog" );
			gUIShell.DrawInterface( "cursors" );
			if( !UpdateShell( seconds, gAppModule.GetCursorX(), gAppModule.GetCursorY() ) )
			{
				return;
			}
		}
	}
	
	if ( m_UIIntro->IsVisible() && gWorldState.GetCurrentState() != WS_SP_NIS )
	{
		m_UIIntro->Draw( seconds );
	}
	else if ( gWorldState.GetCurrentState() == WS_SP_NIS )
	{
		gUIShell.DrawInterface( "intro_ds" );
	}

	if ( m_UIOutro->IsVisible() )
	{
		m_UIOutro->Draw( seconds );
	}
	
	gUI.GetGameGUI().DrawItems();	
}


void UI::OnAppActivate( bool activate )
{
#	if !GP_RETAIL
	if( m_UIUserConsole->IsVisible() ) 
	{
		m_UIUserConsole->OnAppActivate( activate );
	}
#	endif // !GP_RETAIL
	if( m_UIFrontend->IsVisible() ) 
	{
		m_UIFrontend->OnAppActivate( activate );
	}
	if( m_UIMultiplayer->IsVisible() ) 
	{
		m_UIMultiplayer->OnAppActivate( activate );
	}
	if( m_UIIntro->IsVisible() ) 
	{
		m_UIIntro->OnAppActivate( activate );
	}
	if( m_UIOutro->IsVisible() ) 
	{
		m_UIOutro->OnAppActivate( activate );
	}
	if( m_UIGame->IsVisible() ) 
	{
		m_UIGame->OnAppActivate( activate );
	}

	if ( UIShell::DoesSingletonExist() )
	{
		gUIShell.OnAppActivate( activate );
	}

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		gUIShell.ResetImeEditBuffer();
	}
}


bool UI::RequestQuitGame() const
{
	if ( m_UIFrontend->IsVisible() || m_UIMultiplayer->IsVisible() )
	{
		return ( true );
	}

	return ( m_UIGame->RequestQuit() );
}


