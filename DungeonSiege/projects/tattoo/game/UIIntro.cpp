/**************************************************************************

Filename		: UIIntro.cpp

Class			: UIIntro.cpp

Description		: Contains the class which handles the initial menu
				  interface.

Creation Date	: 4/3/2001

**************************************************************************/


// Include Files
#include "Precomp_Game.h"
#include "UIIntro.h"
#include "AppModule.h"
#include "Fuel.h"
#include "InputBinder.h"
#include "WorldState.h"
#include "ui_animation.h"
#include "gps_manager.h"
#include "GameMovie.h"


UIIntro::UIIntro()
	: m_bIsVisible			( false	)	
{
	m_pInputBinder = new InputBinder( "intro" );

	m_currentSample = 0;

	m_bExiting = false;

	gAppModule.RegisterBinder( m_pInputBinder, 102 );

	m_bMonologPlaying = false;

	// Publish information to the Input binder
	PublishInterfaceToInputBinder();
	BindInputToPublishedInterface();	
}


UIIntro::~UIIntro()
{
	if ( m_pInputBinder != NULL )
	{
		Delete( m_pInputBinder );
	}	
}


void UIIntro::SetIsVisible( bool flag )
{ 
	m_bIsVisible = flag;
	m_pInputBinder->SetIsActive( flag );
}


void UIIntro::GameGUICallback( gpstring const & message, UIWindow & /* ui_window */ )
{
	if ( !IsVisible() )
	{
		return;
	}

	if ( message.same_no_case( "end_microsoft_fade" ) )
	{
		if ( m_sCurrentScene.same_no_case( "microsoft_fade_in" ) )
		{
			ActivateBeat( "microsoft_beat" );
		}
		else if ( m_sCurrentScene.same_no_case( "microsoft_fade_out" ) )		
		{
			gUIShell.HideInterface( "intro_microsoft" );
			FadeInGasPowered();
		}
	}
	else if ( message.same_no_case( "end_gaspowered_fade" ) )
	{
		if ( m_sCurrentScene.same_no_case( "gaspowered_fade_in" ) )
		{
			ActivateBeat( "gaspowered_beat" );
		}
		else if ( m_sCurrentScene.same_no_case( "gaspowered_fade_out" ) )
		{
			gUIShell.HideInterface( "intro_gaspowered" );
			ActivateBeat( "monolog_beat" );
		}
	}
	else if ( message.same_no_case( "end_loading_fade" ) )
	{
		ActivateBeat( "intro_beat" );
	}	
}


void UIIntro::DeactivateInterfaces()
{	
	gUIShell.MarkInterfaceForDeactivation( "intro_gaspowered" );
	gUIShell.MarkInterfaceForDeactivation( "intro_microsoft" );	
	SetIsVisible( false );
}


InputBinder & UIIntro::GetInputBinder()
{
	gpassert( m_pInputBinder );
	return( *m_pInputBinder );
}


void UIIntro::Update( double secondsElapsed )
{
	gUI.GetGameGUI().Update( secondsElapsed, gAppModule.GetCursorX(), gAppModule.GetCursorY() );	

	m_currentBeat -= (float)secondsElapsed;
	if ( m_currentBeat <= 0.0f )
	{
		BeatComplete();
	}
}


void UIIntro::ActivateBeat( gpstring sScene )
{
	SceneToBeatMap::iterator i = m_beatMap.find( sScene );
	if ( i == m_beatMap.end() )
	{
		return;
	}

	m_sCurrentScene = sScene;
	m_currentBeat	= (*i).second;
}


void UIIntro::BeatComplete()
{
	if ( m_sCurrentScene.empty() )
	{
		return;
	}
	
	gpstring sCurrentScene = m_sCurrentScene;
	
	if ( sCurrentScene.same_no_case( "intro_beat" ) )
	{
		m_sCurrentScene.clear();
		FadeInMicrosoft();
	}
	else if ( sCurrentScene.same_no_case( "microsoft_beat" ) )
	{
		m_sCurrentScene.clear();
		FadeOutMicrosoft();
	}
	else if ( sCurrentScene.same_no_case( "gaspowered_beat" ) )
	{
		m_sCurrentScene.clear();
		FadeOutGasPowered();
	}
	else if ( sCurrentScene.same_no_case( "monolog_beat" ) )
	{
		m_sCurrentScene.clear();	
		gWorldStateRequest( WS_WAIT_FOR_BEGIN );
	}
}


void UIIntro::Draw( double /*seconds_elapsed*/ )
{		
	bool bBlend = gDefaultRapi.EnableAlphaBlending( true );
	
	// Draw the user interface
	gUI.GetGameGUI().Draw();	

	gDefaultRapi.EnableAlphaBlending( bBlend );
}


void UIIntro::PublishInterfaceToInputBinder()
{
	GetInputBinder().PublishVoidInterface( "exit_intro", makeFunctor( *this, &UIIntro::OnEscape ) );
	GetInputBinder().PublishVoidInterface( "selection_down", makeFunctor( *this, &UIIntro::OnEscape ) );
	GetInputBinder().PublishVoidInterface( "context_down", makeFunctor( *this, &UIIntro::OnEscape ) );
	GetInputBinder().PublishVoidInterface( "special_down", makeFunctor( *this, &UIIntro::OnEscape ) );
}


void UIIntro::BindInputToPublishedInterface()
{
	GetInputBinder().BindDefaultInputsToPublishedInterfaces();
}


bool UIIntro::OnEscape()
{
	if ( !m_bExiting )
	{
		m_bExiting = true;
		SetIsVisible( false );
		SetAsActive( false );
		HideMonologue();	
		gGameMovie.SkipSequence();
	}

	return( false );
}


void UIIntro::SetAsActive( bool flag )
{
	if ( flag == true )
	{
		m_bExiting = false;
		m_currentSample = 0;

		gUIShell.GetMessenger()->RegisterCallback( makeFunctor( *this, &UIIntro :: GameGUICallback ) );
		
		gUIShell.MarkInterfaceForActivation( "ui:interfaces:intro:intro_gaspowered", false );
		gUIShell.MarkInterfaceForActivation( "ui:interfaces:intro:intro_microsoft", false );		

		m_beatMap.clear();
		m_fadeMap.clear();

		FastFuelHandle hIntroSettings( "ui:config:intro_settings:intro_settings" );
		if ( hIntroSettings.IsValid() )
		{
			FastFuelHandle hFades = hIntroSettings.GetChildNamed( "fades" );
			if ( hFades.IsValid() )
			{
				FastFuelFindHandle hFind( hFades );

				gpstring sKey;
				gpstring sValue;

				if ( hFind.FindFirstKeyAndValue() )
				{
					while ( hFind.GetNextKeyAndValue( sKey, sValue ) )
					{
						FadeData fd;

						float fValue = 0.0f;
						stringtool::GetDelimitedValue( sValue, ',', 0, fValue );
						fd.m_duration = fValue;

						stringtool::GetDelimitedValue( sValue, ',', 1, fValue );
						fd.m_beginAlpha = fValue;

						stringtool::GetDelimitedValue( sValue, ',', 2, fValue );
						fd.m_endAlpha = fValue;

						m_fadeMap.insert( SceneFadePair( sKey, fd ) );
					}
				}
			}

			FastFuelHandle hBeats = hIntroSettings.GetChildNamed( "beats" );
			if ( hBeats.IsValid() )
			{
				gpstring sKey;
				gpstring sValue;

				FastFuelFindHandle hFind( hBeats );
				if ( hFind.FindFirstKeyAndValue() )
				{
					while ( hFind.GetNextKeyAndValue( sKey, sValue ) )
					{
						float duration = 0.0f;
						stringtool::Get( sValue, duration );
						m_beatMap.insert( SceneToBeatPair( sKey, duration ) );
					}
				}
			}
		}
	}
	else
	{
		gUI.GetGameGUI().GetMessenger()->UnRegisterCallback( makeFunctor( *this, &UIIntro :: GameGUICallback ) );		
		
		gUIShell.MarkInterfaceForDeactivation( "intro_gaspowered" );
		gUIShell.MarkInterfaceForDeactivation( "intro_microsoft" );
		
		SetIsVisible( false );
	}
}


void UIIntro::FadeOutLoadingScreen()
{
	UIWindow * pWindow = gUIShell.FindUIWindow( "loading_bar", "load_bar" );
	if ( pWindow )
	{		
		SceneFadeMap::iterator i = m_fadeMap.find( "loadbar_fade_out" );
		if ( i != m_fadeMap.end() )
		{
			gUIAnimation.AddAlphaAnimation( (*i).second.m_duration, (*i).second.m_beginAlpha, (*i).second.m_endAlpha, pWindow );
		}
	}	
}


void UIIntro::FadeInMicrosoft()
{
	gUIShell.ShowInterface( "intro_microsoft" );
	UIWindow * pWindow = gUIShell.FindUIWindow( "ms_panel_1", "intro_microsoft" );
	if ( pWindow )
	{
		SceneFadeMap::iterator i = m_fadeMap.find( "microsoft_fade_in" );
		if ( i != m_fadeMap.end() )
		{
			gUIAnimation.AddAlphaAnimation( (*i).second.m_duration, (*i).second.m_beginAlpha, (*i).second.m_endAlpha, pWindow );
		}
	}

	m_sCurrentScene = "microsoft_fade_in";
}


void UIIntro::FadeOutMicrosoft()
{
	UIWindow * pWindow = gUIShell.FindUIWindow( "ms_panel_1", "intro_microsoft" );
	if ( pWindow )
	{
		SceneFadeMap::iterator i = m_fadeMap.find( "microsoft_fade_out" );
		if ( i != m_fadeMap.end() )
		{
			gUIAnimation.AddAlphaAnimation( (*i).second.m_duration, (*i).second.m_beginAlpha, (*i).second.m_endAlpha, pWindow );
		}
	}

	m_sCurrentScene = "microsoft_fade_out";
}


void UIIntro::FadeInGasPowered()
{
	gUIShell.ShowInterface( "intro_gaspowered" );
	UIWindow * pWindow = gUIShell.FindUIWindow( "gp_panel_1", "intro_gaspowered" );
	if ( pWindow )
	{
		SceneFadeMap::iterator i = m_fadeMap.find( "gaspowered_fade_in" );
		if ( i != m_fadeMap.end() )
		{
			gUIAnimation.AddAlphaAnimation( (*i).second.m_duration, (*i).second.m_beginAlpha, (*i).second.m_endAlpha, pWindow );
		}
	}

	m_sCurrentScene = "gaspowered_fade_in";
}


void UIIntro::FadeOutGasPowered()
{
	UIWindow * pWindow = gUIShell.FindUIWindow( "gp_panel_1", "intro_gaspowered" );
	if ( pWindow )
	{
		SceneFadeMap::iterator i = m_fadeMap.find( "gaspowered_fade_out" );
		if ( i != m_fadeMap.end() )
		{
			gUIAnimation.AddAlphaAnimation( (*i).second.m_duration, (*i).second.m_beginAlpha, (*i).second.m_endAlpha, pWindow );
		}
	}

	m_sCurrentScene = "gaspowered_fade_out";
}

	
void UIIntro::ShowMonologue()
{
	if ( !m_bExiting )
	{
		gGameMovie.InitializeMovieSequence( "movies:intro" );
		m_bMonologPlaying = true;
	}
}


void UIIntro::HideMonologue()
{
	if ( m_bMonologPlaying || m_bExiting )
	{
		if ( gWorldState.GetPendingState() != WS_WAIT_FOR_BEGIN )
		{
			gWorldStateRequest( WS_WAIT_FOR_BEGIN );
		}	
	}

	m_bMonologPlaying = false;
}

