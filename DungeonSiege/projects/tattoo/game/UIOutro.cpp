/**************************************************************************

Filename		: UIOutro.cpp

Class			: UIOutro.cpp

Description		: Contains the class which handles the initial menu
				  interface.

Creation Date	: 7/7/2001

**************************************************************************/


// Include Files
#include "Precomp_Game.h"
#include "UIOutro.h"
#include "AppModule.h"
#include "Fuel.h"
#include "InputBinder.h"
#include "WorldState.h"
#include "ui_animation.h"
#include "gps_manager.h"
#include "GameMovie.h"
#include "TimeMgr.h"
#include "TattooGame.h"
#include "UICamera.h"
#include "UIGame.h"
#include "WorldFx.h"
#include "WorldMap.h"
#include "GoCore.h"
#include "GoDefs.h"
#include "MCP.h"
#include "siege_mouse_shadow.h"
#include "GoMind.h"
#include "PContentDb.h"
#include "GoDb.h"
#include "WorldTerrain.h"
#include "Server.h"
#include "siege_frustum.h"
#include "enchantment.h"


UIOutro::UIOutro()
	: m_bIsVisible			( false	)	
{
	m_pInputBinder = new InputBinder( "outro" );

	m_currentSample		= GPGSound::INVALID_SOUND_ID;
	m_initialPause		= 0.0f;
	m_faderPause		= 0.0f;
	m_faderInPause		= 0.0f;
	m_bFading			= false;
	m_bMonologPlaying	= false;
	m_bCreditsRolling	= false;
	m_lastResWidth		= 0;
	m_lastResHeight		= 0;

	gAppModule.RegisterBinder( m_pInputBinder, 103 );

	// Publish information to the Input binder
	PublishInterfaceToInputBinder();
	BindInputToPublishedInterface();	
}


UIOutro::~UIOutro()
{
	if ( m_pInputBinder != NULL )
	{
		Delete( m_pInputBinder );
	}	
}


void UIOutro::SetIsVisible( bool flag )
{ 
	m_bIsVisible = flag;
	m_pInputBinder->SetIsActive( flag );
}


void UIOutro::GameGUICallback( gpstring const & /* message */, UIWindow & /* ui_window */ )
{
	if ( !IsVisible() )
	{
		return;
	}
}


void UIOutro::DeactivateInterfaces()
{		
	SetIsVisible( false );
}


InputBinder & UIOutro::GetInputBinder()
{
	gpassert( m_pInputBinder );
	return( *m_pInputBinder );
}


void UIOutro::Update( double secondsElapsed )
{
	gUI.GetGameGUI().Update( secondsElapsed, gAppModule.GetCursorX(), gAppModule.GetCursorY() );

	if ( m_initialPause <= 0.0f )
	{
		if ( m_bFading )
		{
			m_faderPause -= (float)secondsElapsed;
			if ( m_faderPause <= 0.0f )
			{
				m_bFading = false;
				gWorldStateRequest( WS_SP_OUTRO );
				gDefaultRapi.SetClearColor( 0x00000000 );
				gTimeMgr.AddRapiFader( true, 0.0 );
				gTattooGame.SetRenderSkip( 2 );
				ShowMonologue();				
			}
		}		
	}
	else
	{
		m_initialPause -= (float)secondsElapsed;
		if ( m_initialPause <= 0.0f )
		{
			gTattooGame.RSUserPause( true );
			FadeToBlack();
		}
	}
}


void UIOutro::Draw( double /*seconds_elapsed*/ )
{		
	bool bBlend = gDefaultRapi.EnableAlphaBlending( true );
	
	// Draw the user interface
	gUI.GetGameGUI().Draw();	

	gDefaultRapi.EnableAlphaBlending( bBlend );
}


void UIOutro::PublishInterfaceToInputBinder()
{
	GetInputBinder().PublishVoidInterface( "exit_outro", makeFunctor( *this, &UIOutro::OnEscape ) );
	GetInputBinder().PublishVoidInterface( "skip_current_movie", makeFunctor( *this, &UIOutro::SkipMovie ) );
	GetInputBinder().PublishVoidInterface( "restart_movie", makeFunctor( *this, &UIOutro::RestartMovie ) );
	GetInputBinder().PublishVoidInterface( "pause_movie", makeFunctor( *this, &UIOutro::PauseMovie ) );		
}


void UIOutro::BindInputToPublishedInterface()
{
	GetInputBinder().BindDefaultInputsToPublishedInterfaces();
}


bool UIOutro::OnEscape()
{	
	return( false );
}


void UIOutro::SetAsActive( bool flag )
{
	if ( flag == true )
	{			
		m_currentSample = GPGSound::INVALID_SOUND_ID;
		gUIShell.GetMessenger()->RegisterCallback( makeFunctor( *this, &UIOutro :: GameGUICallback ) );				
		
		FastFuelHandle hOutroSettings( "ui:config:outro_settings:outro_settings" );
		if ( hOutroSettings.IsValid() )
		{
			hOutroSettings.Get( "fade_after_duration", m_initialPause );
			hOutroSettings.Get( "fade_out_duration", m_faderPause );
			hOutroSettings.Get( "fade_in_duration", m_faderInPause );
			hOutroSettings.Get( "credits_sample", m_sCreditsSample );
		}
	}
	else
	{		
		gUI.GetGameGUI().GetMessenger()->UnRegisterCallback( makeFunctor( *this, &UIOutro :: GameGUICallback ) );				
		DeactivateInterfaces();
		gSoundManager.StopStream( m_currentSample );
		m_currentSample = GPGSound::INVALID_SOUND_ID;
	}
}


void UIOutro::StopSampleCB( UINT32 /* param */ )
{
	if ( !m_bMonologPlaying )
	{		
		gWorldStateRequest( WS_MAIN_MENU );
	}
	else
	{
		HideMonologue();				
	}
}


void UIOutro::FadeToBlack()
{
	gTimeMgr.AddRapiFader( false, m_faderInPause );	
	m_bFading = true;
}


void UIOutro::ShowMonologue()
{
	gGameMovie.InitializeMovieSequence( "movies:outro" );
	m_bMonologPlaying = true;	
}


void UIOutro::HideMonologue()
{	
	if ( m_bMonologPlaying )
	{
		m_bMonologPlaying = false;
		RollCredits();		
	}
	else if ( m_bCreditsRolling )
	{
		CreditsFinish();
	}
}


void UIOutro::RollCredits()
{		
	m_currentSample = gSoundManager.PlayStream( m_sCreditsSample, GPGSound::STRM_MUSIC );
	gGameMovie.InitializeMovieSequence( "movies:credits_movie" );
	m_bCreditsRolling = true;
}


void UIOutro::CreditsFinish()
{
	if ( m_bCreditsRolling )
	{			
		m_bCreditsRolling = false;	
		FadeInGame();
	}
}


void UIOutro::FadeInGame()
{
	gTimeMgr.AddRapiFader( true, m_faderInPause );
	gWorldStateRequest( WS_SP_INGAME );	
}


bool UIOutro::SkipMovie()
{
	if ( m_bMonologPlaying || m_bCreditsRolling )
	{
		gGameMovie.SkipCurrentMovie();		
	}
	return true;
}


bool UIOutro::RestartMovie()
{
	gGameMovie.RestartSequence();
	return true;
}


bool UIOutro::PauseMovie()
{
	if ( gGameMovie.IsMoviePlaying() )
	{
		if ( m_bCreditsRolling )
		{
			gGameMovie.PauseCurrentMovie( !gGameMovie.IsMoviePaused() );
		}
		else
		{
			gGameMovie.SkipCurrentMovie();

			if ( m_bMonologPlaying )
			{
				HideMonologue();
			}
		}
	}
	return true;
}


void UIOutro::SetOutroGameSize()
{
	m_lastResWidth	= gAppModule.GetGameWidth();
	m_lastResHeight = gAppModule.GetGameHeight();
	gAppModule.SetGameSize( GSize( 800, 600 ) );
}


void UIOutro::SetLastGameSize()
{
	gAppModule.SetGameSize( GSize( m_lastResWidth, m_lastResHeight ) );
}