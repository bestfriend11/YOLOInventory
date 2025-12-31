/**************************************************************************

	UIOptions

	Date		: Oct 23, 2000

	Author		: Adam Swensen

**************************************************************************/


#include "Precomp_Game.h"
#include "UIMenuManager.h"
#include "UIOptions.h"
#include "UI.h"
#include "UICamera.h"
#include "UIFrontend.h"
#include "UIGame.h"
#include "UIGameConsole.h"
#include "UI_Shell.h"
#include "UI_Text.h"
#include "UI_Slider.h"
#include "UI_ComboBox.h"
#include "UI_Checkbox.h"
#include "UI_ChatBox.h"
#include "Rapi.h"
#include "gps_manager.h"
#include "WorldState.h"
#include "AppModule.h"
#include "InputBinder.h"
#include "stringtool.h"
#include "Server.h"
#include "Player.h"
#include "World.h"
#include "WorldOptions.h"
#include "WorldSound.h"
#include "WorldTime.h"
#include "TattooGame.h"
#include "siege_options.h"
#include "Config.h"
#include "Flamethrower.h" 
#include "Flamethrower_interpreter.h"
#include "GoDb.h"
#include "UIZoneMatch.h"
#include "GameState.h"


UIOptions::UIOptions()
	: m_bIsActive				( false )
	, m_bGotCancelVideoOptions	( false )
	, m_bGotCancelAudioOptions	( false )
	, m_bGotCancelGameOptions	( false )
	, m_pInputBinder			( 0 )
	, m_CurrentBindingRowAt		( 0 )
	, m_pSelectedBindingInput	( 0 )
	, m_pSisterBindingInput		( 0 )
	, m_bControlDown			( false )
	, m_bShiftDown				( false )
	, m_bAltDown				( false )
	, m_KeySecondsDown			( 0 )
	, m_bGameBinderWasActive	( false )
	, m_bGameWasPaused			( false )
	, m_bCanShowDismembermentOption( true )
	, m_bCanShowRedBloodOption	( true )
	, CancelZMMessage			( 0 )
	, CancelDropInventory		( 0 )
{
	// activate the options interfaces
	gUIShell.ActivateInterface( "ui:interfaces:backend:options_video", false );
	gUIShell.ActivateInterface( "ui:interfaces:backend:options_game", false );
	gUIShell.ActivateInterface( "ui:interfaces:backend:options_audio", false );
	gUIShell.ActivateInterface( "ui:interfaces:backend:options_input", false );
	gUIShell.ActivateInterface( "ui:interfaces:backend:options_bindings", false );
	gUIShell.ActivateInterface( "ui:interfaces:backend:options_item_filters", false );

	// get input bindings control array
	m_pInputDescriptions = new UIText* [ KEY_BINDINGS_ROWS_VISIBLE ];
	m_pInputPrimary      = new UIText* [ KEY_BINDINGS_ROWS_VISIBLE ];
	m_pInputSecondary    = new UIText* [ KEY_BINDINGS_ROWS_VISIBLE ];

	// register a special input binder for key bindings
	m_pInputBinder = new InputBinder( "options_bindings" );
	gAppModule.RegisterBinder( m_pInputBinder, 0 );
	m_pInputBinder->SetIsActive( false );

	m_pInputBinder->PublishAllKeysInterface ( makeFunctor( *this, &UIOptions::InputKeyCallback ) );
	m_pInputBinder->PublishInputInterface( "grab_mouse", makeFunctor( *this, &UIOptions::InputMouseCallback ) );
	m_pInputBinder->BindInputToPublishedInterface( MouseInput( "left_down",   Keys::Q_IGNORE_KEYS ), "grab_mouse" );
	m_pInputBinder->BindInputToPublishedInterface( MouseInput( "middle_down", Keys::Q_IGNORE_KEYS ), "grab_mouse" );
	m_pInputBinder->BindInputToPublishedInterface( MouseInput( "right_down",  Keys::Q_IGNORE_KEYS ), "grab_mouse" );
	m_pInputBinder->BindInputToPublishedInterface( MouseInput( "x1_down",     Keys::Q_IGNORE_KEYS ), "grab_mouse" );
	m_pInputBinder->BindInputToPublishedInterface( MouseInput( "x2_down",     Keys::Q_IGNORE_KEYS ), "grab_mouse" );
	m_pInputBinder->BindInputToPublishedInterface( MouseInput( "wheel_up",    Keys::Q_IGNORE_KEYS ), "grab_mouse" );
	m_pInputBinder->BindInputToPublishedInterface( MouseInput( "wheel_down",  Keys::Q_IGNORE_KEYS ), "grab_mouse" );

	FastFuelHandle hLocaleSettings( "config:global_settings:locale" );
	if ( hLocaleSettings )
	{
		hLocaleSettings.Get( "red_blood_option", m_bCanShowDismembermentOption );
		hLocaleSettings.Get( "dismemberment_option", m_bCanShowRedBloodOption );
	}

#if DISABLE_GORE
	gUIShell.ShowGroup( "options_game_dismember", false, false, "options_game" );
#endif

	m_sLastTab = "options_video";
}


UIOptions::~UIOptions()
{
	gUI.GetGameGUI().DeactivateInterface( "options_game" );
	gUI.GetGameGUI().DeactivateInterface( "options_video" );
	gUI.GetGameGUI().DeactivateInterface( "options_audio" );
	gUI.GetGameGUI().DeactivateInterface( "options_input" );
	gUI.GetGameGUI().DeactivateInterface( "options_bindings" );
	gUI.GetGameGUI().DeactivateInterface( "options_item_filters" );

	gAppModule.UnregisterBinder( m_pInputBinder );
	Delete( m_pInputBinder );

	Delete( m_pInputDescriptions );
	Delete( m_pInputPrimary );
	Delete( m_pInputSecondary );
}


void UIOptions::Activate()
{
	m_bIsActive = true;

	m_bGotCancelVideoOptions = false;
	m_bGotCancelAudioOptions = false;
	m_bGotCancelGameOptions = false;

	m_bTempMuteSound = false;
	m_bTempEnableSound = false;

	if ( gUIGame.IsActive() )
	{
		if ( ::IsSinglePlayer() )
		{
			m_bGameWasPaused = gAppModule.IsUserPaused();
			gTattooGame.RSUserPause( true );
		}

		m_bGameBinderWasActive = gUIGame.GetGameInputBinder().IsActive();
		gUIGame.GetGameInputBinder().SetIsActive( false );
		gUIGame.GetUserInputBinder().SetIsActive( false );
	}

	m_CurrentBindingRowAt = 0;

	for ( int i = 0; i < KEY_BINDINGS_ROWS_VISIBLE; ++i )
	{
		gpstring sTextName( "binding" );
		stringtool::Append( i, sTextName );

		UIText *pText = (UIText *)gUIShell.FindUIWindow( sTextName + "_text" );
		gpassert( pText );
		if ( pText )
		{
			m_pInputDescriptions[i] = pText;
		}

		pText = (UIText *)gUIShell.FindUIWindow( sTextName + "_key0" );
		gpassert( pText );
		if ( pText )
		{
			m_pInputPrimary[i] = pText;
		}

		pText = (UIText *)gUIShell.FindUIWindow( sTextName + "_key1" );
		gpassert( pText );
		if ( pText )
		{
			m_pInputSecondary[i] = pText;
		}
	}

	GetVideoOptions();
	GetAudioOptions();
	GetInputOptions();
	GetGameOptions();

	gUI.GetGameGUI().ShowInterface( m_sLastTab );
	if ( m_sLastTab.same_no_case( "options_game" ) )
	{
		gUIShell.ShowGroup( "options_game_1", true, false, "options_game" );
		gUIShell.ShowGroup( "options_game_more", true, false, "options_game" );
		
		gUIShell.ShowGroup( "options_game_redblood", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_dismember", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_dropinventory", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_zmmessage", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_back", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_2", false, false, "options_game" );
	}

	// register options menu callback
	gUI.GetGameGUI().GetMessenger()->RegisterCallback( makeFunctor( *this, &UIOptions::GameGUICallback ) );
}


void UIOptions::Deactivate()
{
	m_bIsActive = false;

	gUI.GetGameGUI().HideInterface( "options_game" );
	gUI.GetGameGUI().HideInterface( "options_video" );
	gUI.GetGameGUI().HideInterface( "options_audio" );
	gUI.GetGameGUI().HideInterface( "options_input" );
	gUI.GetGameGUI().HideInterface( "options_bindings" );

	gUI.GetGameGUI().GetMessenger()->UnRegisterCallback( makeFunctor( *this, &UIOptions::GameGUICallback ) );

	gUIMenuManager.SetModalActive( false );

	if ( gUIGame.IsActive() )
	{
		if ( ::IsSinglePlayer() && !m_bGameWasPaused )
		{
			gTattooGame.RSUserPause( false );
		}

		gUIGame.GetGameInputBinder().SetIsActive( m_bGameBinderWasActive );
		gUIGame.GetUserInputBinder().SetIsActive( m_bGameBinderWasActive );
	}
	else
	{
		gUI.GetUIFrontend().HideInterfaces();
		gUI.GetGameGUI().ShowInterface( "main_menu" );
	}
}


void UIOptions::Update( double secondsElapsed )
{
	// if CTRL, ALT, or SHIFT are held down for a number of seconds, bind to them only
	if ( m_pInputBinder->IsActive() )
	{
		if ( m_bControlDown || m_bShiftDown || m_bAltDown )
		{
			m_KeySecondsDown += secondsElapsed;
		}
		else
		{
			m_KeySecondsDown = 0;
		}
		if ( (m_bControlDown && m_bShiftDown)
			|| (m_bControlDown && m_bAltDown)
			|| (m_bShiftDown && m_bAltDown) )
		{
			m_KeySecondsDown = 0;
		}

		if ( m_KeySecondsDown >= KEY_HOLD_SET_TIME )
		{
			KeyInput key;
			if ( m_bControlDown )
			{
				key = KeyInput( Keys::KEY_CONTROL, Keys::Q_CONTROL );
			}
			else if ( m_bShiftDown )
			{
				key = KeyInput( Keys::KEY_SHIFT, Keys::Q_SHIFT );
			}
			else if ( m_bAltDown )
			{
				key = KeyInput( Keys::KEY_ALT, Keys::Q_ALT );
			}

			// clear any matching bindings
			InputBinder::BindingGroupMap::iterator iGroup = m_bindings.begin();
			for ( ; iGroup != m_bindings.end(); ++iGroup )
			{
				InputBinder::BindingColl::iterator i = iGroup->second.m_Bindings.begin();
				for ( ; i != iGroup->second.m_Bindings.end(); ++i )
				{
					if ( (*i).m_Input0.HasKey() )
					{
						if ( ((*i).m_Input0.GetKey().GetKey() == key.GetKey()) )
						{
							(*i).m_Input0 = Input( KeyInput( Keys::KEY_NONE, (*i).m_Input0.GetKey().GetQualifiers() ) );
						}
					}
					if ( (*i).m_Input1.HasKey() )
					{
						if ( ((*i).m_Input1.GetKey().GetKey() == key.GetKey()) )
						{
							(*i).m_Input1 = Input( KeyInput( Keys::KEY_NONE, (*i).m_Input1.GetKey().GetQualifiers() ) );
						}
					}
				}
			}

			KeyInput newKey( key.GetKey(), key.GetQualifiers() );
			Keys::QUALIFIER q = newKey.GetQualifiers();
			q |= (m_pSelectedBindingInput->GetKey().GetQualifiers() & Keys::Q_KEY_DOWN);
			q |= (m_pSelectedBindingInput->GetKey().GetQualifiers() & Keys::Q_KEY_UP);
			newKey.SetQualifiers( q );

			// set new binding
			*m_pSelectedBindingInput = Input( newKey );

			CreateBindingsList( m_CurrentBindingRowAt );

			m_pInputBinder->SetIsActive( false );

			gUIMessenger.SendUIMessage( UIMessage( MSG_ROLLOFF ), m_pSelectedBindingWindow );

			m_bControlDown = false;
			m_bShiftDown = false;
			m_bAltDown = false;

			m_KeySecondsDown = 0;
		}
	}
}


void UIOptions::HideInterfaces()
{
	gUI.GetGameGUI().HideInterface( "options_game" );
	gUI.GetGameGUI().HideInterface( "options_video" );
	gUI.GetGameGUI().HideInterface( "options_audio" );
	gUI.GetGameGUI().HideInterface( "options_input" );
	gUI.GetGameGUI().HideInterface( "options_bindings" );
}


void UIOptions::GetVideoOptions()
{
	Rapi &renderer = gUIShell.GetRenderer();

	m_AllowedRes.clear();
	FastFuelHandle hAllowedRes( "config:options:video:allowed_res" );
	if ( hAllowedRes.IsValid() )
	{
		FastFuelFindHandle hFindRes( hAllowedRes );
		
		if ( hFindRes.IsValid() )
		{
			if ( hFindRes.FindFirstKeyAndValue() )
			{
				gpstring key, value;
				while ( hFindRes.GetNextKeyAndValue( key, value ) )
				{
					m_AllowedRes.push_back( value );
				}
			}
		}
	}

	UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_screen_res" );
	if( pCombo )
	{
		gpwstring sResolution;

		int width = gConfig.GetInt( "width",  800 );
		int height = gConfig.GetInt( "height", 600 );
		int bpp = gConfig.GetInt( "bpp", renderer.GetBitsPerPixel() );

		sResolution.assignf( L"%dx%dx%d", width, height, bpp );
		pCombo->SetText( sResolution );

		UIListbox *pListbox = (UIListbox *)gUIShell.FindUIWindow( "listbox_screen_res" );
		if ( pListbox )
		{
			pListbox->RemoveAllElements();

			const DisplayList displayModes = renderer.GetDisplayModeList();

			DisplayList::const_iterator i;
			for ( i = displayModes.begin(); i != displayModes.end(); ++i )
			{
				if ( !gAppModule.IsFullScreen() && gDefaultRapi.GetBitsPerPixel() != i->m_BPP )
				{
					continue;
				}

				bool bAllowed = false;
				sResolution.clear();
				sResolution.assignf( L"%dx%dx%d", i->m_Width, i->m_Height, i->m_BPP );
				for ( ResColl::iterator iRes = m_AllowedRes.begin(); iRes != m_AllowedRes.end(); ++iRes )
				{
					if ( sResolution.same_no_case( ToUnicode( *iRes ) ) )
					{
						bAllowed = true;
						break;
					}
				}
				if ( bAllowed )
				{
					pListbox->AddElement( sResolution );
				}
			}
		}
	}

	pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_shadows" );
	if ( pCombo )
	{
		if ( gSiegeEngine.GetOptions().UseExpensiveShadows() )
		{
			pCombo->SetText( gpwtranslate( $MSG$ "All Complex" ) );
		}
		else if ( gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
		{
			pCombo->SetText( gpwtranslate( $MSG$ "Party Complex" ) );
		}
		else if ( gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() || gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
		{
			pCombo->SetText( gpwtranslate( $MSG$ "Simple" ) );
		}
		else
		{
			pCombo->SetText( gpwtranslate( $MSG$ "None" ) );
		}

		UIListbox *pListbox = (UIListbox *)gUIShell.FindUIWindow( "listbox_shadows" );
		if ( pListbox )
		{
			pListbox->RemoveAllElements();

			pListbox->AddElement( gpwtranslate( $MSG$ "None" ) );
			pListbox->AddElement( gpwtranslate( $MSG$ "Simple" ) );			
			pListbox->AddElement( gpwtranslate( $MSG$ "Party Complex" ) );
			pListbox->AddElement( gpwtranslate( $MSG$ "All Complex" ) );

		}
	}

	UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_colordepth_text" );
	if( pText )
	{
		if ( renderer.IsFullscreen() )
		{
			pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "No" ) );
		}
	}

	pText = (UIText *)gUIShell.FindUIWindow( "button_texture_filtering_text" );
	if( pText )
	{
		if ( renderer.GetMipFilter() == D3DTEXF_POINT )
		{
			pText->SetText( gpwtranslate( $MSG$ "Bilinear" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "Trilinear" ) );
		}
	}

	UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_texture_filtering" );
	if ( pButton )
	{
		if ( gDefaultRapi.GetTrilinearCapable() )
		{
			pButton->EnableButton();
		}
		else
		{
			pButton->DisableButton();
		}
	}

	pText = (UIText *)gUIShell.FindUIWindow( "button_hardwaretl_text" );
	if( pText )
	{
		pText->SetText( gpwtranslate( $MSG$ "On" ) );
	}

	UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_gamma" );
	if( pSlider )
	{
		pSlider->SetMin( 0 );
		pSlider->SetMax( 100 );

		pSlider->SetValue( (int)floorf( (float)pSlider->GetMax() * (renderer.GetGamma().x - OPTION_MIN_GAMMA_VALUE) / (OPTION_MAX_GAMMA_VALUE - OPTION_MIN_GAMMA_VALUE) + 0.5f ) );

		if ( !m_bGotCancelVideoOptions )
		{
			CancelGammaValue = renderer.GetGamma().x;
		}
	}

	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_objectdetail" );
	{
		pSlider->SetMin( 20 );
		pSlider->SetMax( 100 );
		pSlider->SetValue( Round( gWorldOptions.GetObjectDetailLevel() * 100.0f ) );
	}

	m_bGotCancelVideoOptions = true;
}

void UIOptions::SetVideoOptions()
{
	Rapi &renderer = gUIShell.GetRenderer();

	UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_gamma" );
	if( pSlider )
	{
		renderer.SetGamma( (float)(OPTION_MAX_GAMMA_VALUE - OPTION_MIN_GAMMA_VALUE) * ((float)pSlider->GetValue() / pSlider->GetMax()) + OPTION_MIN_GAMMA_VALUE );
	}

	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_objectdetail" );
	if ( pSlider )
	{
		gWorldOptions.SetObjectDetailLevel( pSlider->GetValue() * 0.01f );
	}

	UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_shadows" );
	if ( pCombo )
	{
		if ( pCombo->GetText().same_no_case( gpwtranslate( $MSG$ "All Complex" ) ) )
		{
			if ( !gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleStaticShadows();
			}
			if ( !gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleDynamicShadows();
			}			
			if ( !gSiegeEngine.GetOptions().UseExpensiveShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensiveShadows();
			}
			if ( gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensivePartyShadows();
			}
		}
		else if ( pCombo->GetText().same_no_case( gpwtranslate( $MSG$ "Party Complex" ) ) )
		{
			if ( !gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleStaticShadows();
			}
			if ( !gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleDynamicShadows();
			}
			if ( gSiegeEngine.GetOptions().UseExpensiveShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensiveShadows();
			}
			if ( !gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensivePartyShadows();
			}			
		}
		else if ( pCombo->GetText().same_no_case( gpwtranslate( $MSG$ "Simple" ) ) )
		{
			if ( !gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleStaticShadows();
			}
			if ( !gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleDynamicShadows();
			}
			if ( gSiegeEngine.GetOptions().UseExpensiveShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensiveShadows();
			}
			if ( gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensivePartyShadows();
			}
		}
		else if ( pCombo->GetText().same_no_case( gpwtranslate( $MSG$ "None" ) ) )
		{
			if ( gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleStaticShadows();
			}
			if ( gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleDynamicShadows();
			}
			if ( gSiegeEngine.GetOptions().UseExpensiveShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensiveShadows();
			}
			if ( gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensivePartyShadows();
			}
		}

		// Update shadows
		gGoDb.UpdateShadows();
	}

	bool bFullscreen = false;
	UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_fullscreen_text" );
	if( pText )
	{
		if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "Yes" ) ) )
		{
			bFullscreen = true;
		}
	}

	DisplayMode SetDisplayMode;
	SetDisplayMode.m_Width = renderer.GetWidth();
	SetDisplayMode.m_Height = renderer.GetHeight();
	SetDisplayMode.m_BPP = renderer.GetBitsPerPixel();
	pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_screen_res" );
	if( pCombo )
	{
		gpwstring sResolution;
		const DisplayList displayModes = renderer.GetDisplayModeList();

		DisplayList::const_iterator i;
		for ( i = displayModes.begin(); i != displayModes.end(); ++i )
		{
			sResolution.clear();
			sResolution.assignf( L"%dx%dx%d", i->m_Width, i->m_Height, i->m_BPP );

			if ( sResolution.same_no_case( pCombo->GetText() ) )
			{
				SetDisplayMode = *i;
				break;
			}
		}
	}

	// save it
	gConfig.Set( "width", SetDisplayMode.m_Width );
	gConfig.Set( "height", SetDisplayMode.m_Height );
	gConfig.Set( "bpp", SetDisplayMode.m_BPP );

	// change resolution
	if ( gUIFrontend.IsVisible() == false )
	{
		gAppModule.SetGameSizeFromConfig();
	}

	pText = (UIText *)gUIShell.FindUIWindow( "button_texture_filtering_text" );
	if( pText )
	{
		if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "Bilinear" ) ) )
		{
			renderer.SetMipFilter( D3DTEXF_POINT );
		}
		else
		{
			renderer.SetMipFilter( D3DTEXF_LINEAR );
		}
	}
}


void UIOptions::GetAudioOptions()
{
	UIText *tb = (UIText *)gUIShell.FindUIWindow( "button_sound_text" );
	if( tb )
	{
		if ( gSoundManager.IsEnabled() )
		{
			tb->SetText( gpwtranslate( $MSG$ "On" ) );
			UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_eax", "options_audio" );
			if ( pButton )
			{
				pButton->EnableButton();
			}
		}
		else
		{
			tb->SetText( gpwtranslate( $MSG$ "Off" ) );
			UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_eax", "options_audio" );
			if ( pButton )
			{
				pButton->DisableButton();
			}
		}
	}

	tb = (UIText *)gUIShell.FindUIWindow( "button_eax_text" );
	if ( tb )
	{
		if ( gSoundManager.IsEAXEnabled() )
		{
			tb->SetText( gpwtranslate( $MSG$ "On" ) );
		}
		else
		{
			tb->SetText( gpwtranslate( $MSG$ "Off" ) );
		}	
	}

	UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_mastervolume" );
	if( pSlider )
	{
		pSlider->SetEnabled( gSoundManager.IsEnabled() );

		pSlider->SetMin( 0 );
		pSlider->SetMax( 127 );

		pSlider->SetValue( gSoundManager.GetMasterVolume() );

		if ( !m_bGotCancelAudioOptions )
		{
			CancelMasterVolume = gSoundManager.GetMasterVolume();
		}
	}
	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_musicvolume" );
	if( pSlider )
	{
		pSlider->SetEnabled( gSoundManager.IsEnabled() );

		pSlider->SetMin( 0 );
		pSlider->SetMax( 127 );

		pSlider->SetValue( gSoundManager.GetStreamVolume( GPGSound::STRM_MUSIC ) );

		if ( !m_bGotCancelAudioOptions )
		{
			CancelMusicVolume = gSoundManager.GetStreamVolume( GPGSound::STRM_MUSIC );
		}
	}
	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_sfxvolume" );
	if( pSlider )
	{
		pSlider->SetEnabled( gSoundManager.IsEnabled() );

		pSlider->SetMin( 0 );
		pSlider->SetMax( 127 );

		pSlider->SetValue( gSoundManager.GetSampleVolume( GPGSound::SND_EFFECT_NORM ) );

		if ( !m_bGotCancelAudioOptions )
		{
			CancelSFXVolume = gSoundManager.GetSampleVolume( GPGSound::SND_EFFECT_NORM );
		}
	}
	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_ambientvolume" );
	if( pSlider )
	{
		pSlider->SetEnabled( gSoundManager.IsEnabled() );

		pSlider->SetMin( 0 );
		pSlider->SetMax( 127 );

		pSlider->SetValue( gSoundManager.GetStreamVolume( GPGSound::STRM_AMBIENT ) );

		if ( !m_bGotCancelAudioOptions )
		{
			CancelAmbientVolume = gSoundManager.GetStreamVolume( GPGSound::STRM_AMBIENT );
		}
	}
	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_voicevolume" );
	if( pSlider )
	{
		pSlider->SetEnabled( gSoundManager.IsEnabled() );

		pSlider->SetMin( 0 );
		pSlider->SetMax( 127 );

		pSlider->SetValue( gSoundManager.GetStreamVolume( GPGSound::STRM_VOICE ) );

		if ( !m_bGotCancelAudioOptions )
		{
			CancelVoiceVolume = gSoundManager.GetStreamVolume( GPGSound::STRM_VOICE );
		}
	}

	m_bGotCancelAudioOptions = true;
}

void UIOptions::SetAudioOptions()
{
	UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_mastervolume" );
	if( pSlider )
	{
		gSoundManager.SetMasterVolume( (DWORD)pSlider->GetValue() );
	}
	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_musicvolume" );
	if( pSlider )
	{
		gSoundManager.SetStreamVolume( GPGSound::STRM_MUSIC, (DWORD)pSlider->GetValue() );
	}
	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_sfxvolume" );
	if( pSlider )
	{
		gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_LOW, (DWORD)pSlider->GetValue() );
		gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_NORM, (DWORD)pSlider->GetValue() );
		gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_HIGH, (DWORD)pSlider->GetValue() );
	}
	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_ambientvolume" );
	if( pSlider )
	{
		gSoundManager.SetStreamVolume( GPGSound::STRM_AMBIENT, (DWORD)pSlider->GetValue() );
		gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_AMBIENT, (DWORD)pSlider->GetValue() );
	}
	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_voicevolume" );
	if( pSlider )
	{
		gSoundManager.SetStreamVolume( GPGSound::STRM_VOICE, (DWORD)pSlider->GetValue() );
	}

	UIText *tb = (UIText *)gUIShell.FindUIWindow( "button_sound_text" );
	if( tb )
	{
		if( tb->GetText().same_no_case( gpwtranslate( $MSG$ "On" ) ) )
		{
			gWorldSound.Enable( true, false );
			gWorldOptions.SetSound( true );	
		}
		else
		{
			gWorldSound.Enable( false, false );
			gWorldOptions.SetSound( false );
		}
	}

	tb = (UIText *)gUIShell.FindUIWindow( "button_eax_text" );
	if( tb )
	{
		if( tb->GetText().same_no_case( gpwtranslate( $MSG$ "On" ) ) )
		{
			gWorldSound.Enable( gSoundManager.IsEnabled(), true );			
		}
		else
		{
			gWorldSound.Enable( gSoundManager.IsEnabled(), false );				
		}
	}
}

void UIOptions::GetGameOptions()
{
	UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_framerate_text" );
	if( pText )
	{
		if ( gSiegeEngine.GetOptions().ShowFPSEnabled() )
		{
			pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "No" ) );
		}

		if ( !m_bGotCancelGameOptions )
		{
			CancelShowFPS = gSiegeEngine.GetOptions().ShowFPSEnabled();
		}
	}
	pText = (UIText *)gUIShell.FindUIWindow( "button_showtooltips_text" );
	if( pText )
	{
		if ( gUIShell.GetToolTipsVisible() )
		{
			pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "No" ) );
		}

		if ( !m_bGotCancelGameOptions )
		{
			CancelShowToolTips = gUIShell.GetToolTipsVisible();
		}
	}

	UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_gamespeed" );
	if( pSlider )
	{
		pSlider->SetVisible( ::IsSinglePlayer() );
		UIText *pText = (UIText *)gUIShell.FindUIWindow( "text_gamespeed" );
		if ( pText )
		{
			pText->SetVisible( ::IsSinglePlayer() );
		}
		pText = (UIText *)gUIShell.FindUIWindow( "text_gamespeed_minus" );
		if ( pText )
		{
			pText->SetVisible( ::IsSinglePlayer() );
		}
		pText = (UIText *)gUIShell.FindUIWindow( "text_gamespeed_plus" );
		if ( pText )
		{
			pText->SetVisible( ::IsSinglePlayer() );
		}

		pSlider->SetMin( (int)(OPTION_MIN_GAME_SPEED * 10) );
		pSlider->SetMax( (int)(OPTION_MAX_GAME_SPEED * 10) );
		pSlider->SetValue( 0 );
		pSlider->SetValue( (int)(gWorldOptions.GetGameSpeed() * 10 + 0.5f) );
		pSlider->SetTrackMark( 10 );
	}

	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_textscroll" );
	{
		pSlider->SetMin( OPTION_MIN_TEXT_SCROLL );
		pSlider->SetMax( OPTION_MAX_TEXT_SCROLL );

		UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box", "console_output" );
		if ( pChat )
		{
			pChat->SetLineTimeout( gUIMenuManager.GetScrollText() );			
		}

		pSlider->SetValue( (int)gUIMenuManager.GetScrollText() );

		if ( !m_bGotCancelGameOptions && pChat )
		{
			CancelTextScroll = pChat->GetLineTimeout();
		}
	}

	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_maxtext" );
	{
		pSlider->SetMin( OPTION_MIN_CHAT_LINES );
		pSlider->SetMax( OPTION_MAX_CHAT_LINES );

		UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box", "console_output" );
		if ( pChat )
		{
			pChat->SetLineSize( gUIMenuManager.GetMaxText() );			
			gUIGameConsole.ResizeChatWindow();
		}
		pSlider->SetValue( gUIMenuManager.GetMaxText() );

		if ( !m_bGotCancelGameOptions && pChat )
		{
			CancelMaxText = pChat->GetLineSize();
		}
	}

	pText = (UIText *)gUIShell.FindUIWindow( "button_tutorial_text" );
	if( pText )
	{
		if ( gUIMenuManager.GetEnableTips() )
		{
			pText->SetText( gpwtranslate( $MSG$ "On" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "Off" ) );
		}

		if ( !m_bGotCancelGameOptions )
		{
			CancelTutorialTips = gUIMenuManager.GetEnableTips();
		}
	}

	pText = (UIText *)gUIShell.FindUIWindow( "button_difficulty_text" );
	if( pText )
	{
		if ( gWorldOptions.GetDifficulty() == DIFFICULTY_EASY )
		{
			pText->SetText( gpwtranslate( $MSG$ "Easy" ) );
		}
		else if ( gWorldOptions.GetDifficulty() == DIFFICULTY_MEDIUM )
		{
			pText->SetText( gpwtranslate( $MSG$ "Normal" ) );
		}
		else if ( gWorldOptions.GetDifficulty() == DIFFICULTY_HARD )
		{
			pText->SetText( gpwtranslate( $MSG$ "Hard" ) );
		}

		if ( !m_bGotCancelGameOptions )
		{
			CancelDifficulty = gWorldOptions.GetDifficulty();
		}

		if ( ::IsMultiPlayer() && !::IsServerLocal() )
		{
			UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_difficulty", "options_game" );
			if ( pButton )
			{
				pButton->DisableButton();
			}
		}
	}

	pText = (UIText *)gUIShell.FindUIWindow( "button_blood_color_text" );
	if( pText )
	{
		if ( gWorldOptions.IsBloodRed() && gWorldOptions.IsBloodEnabled() )
		{
			pText->SetText( gpwtranslate( $MSG$ "Red" ) );
		}
		else if ( !gWorldOptions.IsBloodRed() && gWorldOptions.IsBloodEnabled() )
		{
			pText->SetText( gpwtranslate( $MSG$ "Green" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "Disabled" ) );
		}
		
		if ( !m_bGotCancelGameOptions )
		{
			CancelRedBlood = gWorldOptions.IsBloodRed();
			CancelBloodEnabled = gWorldOptions.IsBloodEnabled();
		}
	}

	pText = (UIText *)gUIShell.FindUIWindow( "button_dismemberment_text" );
	if( pText )
	{
		if ( gWorldOptions.GetAllowDismemberment() )
		{
			pText->SetText( gpwtranslate( $MSG$ "On" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "Off" ) );
		}
		
		if ( !m_bGotCancelGameOptions )
		{
			CancelDismemberment = gWorldOptions.GetAllowDismemberment();
		}
	}

	pText = (UIText *)gUIShell.FindUIWindow( "button_share_experience_text" );
	if( pText )
	{
		if ( gWorldOptions.GetShareExpEnabled() )
		{
			pText->SetText( gpwtranslate( $MSG$ "On" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "Off" ) );
		}
		
		if ( !m_bGotCancelGameOptions )
		{
			CancelShareExperience = gWorldOptions.GetShareExpEnabled();
		}
	}

	pText = (UIText *)gUIShell.FindUIWindow( "button_priority_boost_text" );
	if ( pText )
	{
		if ( gAppModule.TestOptions( AppModule::OPTION_BOOST_APP_PRIORITY ) )
		{
			pText->SetText( gpwtranslate( $MSG$ "On" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "Off" ) );
		}

		if ( !m_bGotCancelGameOptions )
		{
			CancelPriorityBoost = gAppModule.TestOptions( AppModule::OPTION_BOOST_APP_PRIORITY );
		}
	}

	if( ::IsSinglePlayer() )
	{
		UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_dropinventory", "options_game" );
		if( pList )
		{
			pList->RemoveAllElements();
			pList->AddElement( gpwtranslate( $MSG$ "Nothing" ),			(int)DIO_NOTHING );
			pList->AddElement( gpwtranslate( $MSG$ "Equipped Items" ),	(int)DIO_EQUIPPED );
			pList->AddElement( gpwtranslate( $MSG$ "Inventory Items" ),	(int)DIO_INVENTORY );
			pList->AddElement( gpwtranslate( $MSG$ "All Items" ),		(int)DIO_ALL );
			
			pList->SelectElement( (int) gServer.GetDropInvOption( ) );

			UIComboBox * pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_dropinventory", "options_game" );
			if ( pCombo )
			{
				pCombo->SetText( pList->GetSelectedText() );
			}			

			if ( !m_bGotCancelGameOptions )
			{
				CancelDropInventory = gServer.GetDropInvOption();
			}
		}
	}
	if( IsOnZone() )
	{
		gUIZoneMatch.UpdateGameMessageOptions( "options_game", "options_game_zmmessage" );
		if ( !m_bGotCancelGameOptions )
		{
			CancelZMMessage = (int) gUIZoneMatch.GetGameMessageOptions();
		}
	}
	else
	{
		gUIShell.ShowGroup( "options_game_zmmessage", false, false, "options_game" );
	}

	// Can't edit these in the frontend
	UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_item_filters", "options_game" );
	if ( pButton )
	{
		if ( ::IsInFrontend( gGameState.GetCurrentState() ) )
		{		
			pButton->DisableButton();		
		}
		else
		{
			pButton->EnableButton();
		}
	}

	m_bGotCancelGameOptions = true;
}

void UIOptions::SetGameOptions()
{
	UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_gamespeed" );
	if( pSlider )
	{
		float newSpeed = (float)pSlider->GetValue() / 10;
		if ( newSpeed != gWorldOptions.GetGameSpeed() )
		{
			// $$ speed only allowed to be changed in SP if retail
#			if GP_RETAIL
			if( !::IsMultiPlayer() )
#			endif // GP_RETAIL
			{
				gWorldOptions.RSSetGameSpeed( newSpeed );
			}
		}		
	}

	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_textscroll", "options_game" );
	if ( pSlider )
	{
		float newTextScroll = (float)pSlider->GetValue();
		UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box", "console_output" );
		if ( pChat )
		{
			pChat->SetLineTimeout( newTextScroll );
		}
		gUIMenuManager.SetScrollText( newTextScroll );					
	}	

	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_maxtext", "options_game" );
	if ( pSlider )
	{
		int newMaxText = pSlider->GetValue();
		UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box", "console_output" );
		if ( pChat )
		{
			pChat->SetLineSize( (int)newMaxText );			
			gUIGameConsole.ResizeChatWindow();
		}
		gUIMenuManager.SetMaxText( (int)newMaxText );
	}

	UIText * pText = (UIText *)gUIShell.FindUIWindow( "button_tutorial_text" );
	if( pText )
	{
		if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "On" ) ) )
		{
			gUIMenuManager.SetEnableTips( true );
		}
		else
		{
			gUIMenuManager.SetEnableTips( false ); 
		}		
	}

	pText = (UIText *)gUIShell.FindUIWindow( "button_difficulty_text" );
	if( pText )
	{
		if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "Easy" ) ) )
		{
			gWorldOptions.SSetDifficulty( DIFFICULTY_EASY );
		}
		else if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "Normal" ) ) )
		{
			gWorldOptions.SSetDifficulty( DIFFICULTY_MEDIUM );
		}		
		else if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "Hard" ) ) )
		{
			gWorldOptions.SSetDifficulty( DIFFICULTY_HARD );
		}
	}

	pText = (UIText *)gUIShell.FindUIWindow( "button_blood_color_text" );
	if( pText )
	{
		if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "Red" ) ) )
		{
			gWorldOptions.SetIsBloodEnabled( true );
			gWorldOptions.SetIsBloodRed( true );
		}
		else if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "Green" ) ) )
		{
			gWorldOptions.SetIsBloodEnabled( true );
			gWorldOptions.SetIsBloodRed( false );
		}
		else
		{
			gWorldOptions.SetIsBloodEnabled( false );
		}
	}	

#if !DISABLE_GORE
	pText = (UIText *)gUIShell.FindUIWindow( "button_dismemberment_text" );
	if ( pText )
	{
		if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "On" ) ) )
		{
			gWorldOptions.SetAllowDismemberment( true );
		}
		else
		{
			gWorldOptions.SetAllowDismemberment( false );
		}
	}
#endif

	
	pText = (UIText *)gUIShell.FindUIWindow( "button_share_experience_text" );
	if ( pText )
	{
		if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "On" ) ) )
		{
			gWorldOptions.SetShareExpEnabled( true );
		}
		else
		{
			gWorldOptions.SetShareExpEnabled( false );
		}
	}

	pText = (UIText *)gUIShell.FindUIWindow( "button_priority_boost_text" );
	if ( pText )
	{
		if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "On" ) ) )
		{
			gAppModule.SetOptions( AppModule::OPTION_BOOST_APP_PRIORITY, true );			
		}
		else
		{
			gAppModule.SetOptions( AppModule::OPTION_BOOST_APP_PRIORITY, false );
		}
	}

	if( ::IsSinglePlayer() )
	{
		UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_dropinventory", "options_game" );
		if( pList )
		{
			gServer.SetDropInvOption( (eDropInvOption)pList->GetSelectedTag() );
		}
	}	
	if( IsOnZone() )
	{
		UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_zmmessage", "options_game" );
		if( pList )
		{
			gUIZoneMatch.SetGameMessageOptions( (eInGameMessageOption)pList->GetSelectedTag());
		}
	}
}

void UIOptions::GetInputOptions()
{
	InputBinder *pInputBinder = InputBinder::FindInputBinder( "game" );
	gpassert( pInputBinder );

	m_bindings.clear();
	pInputBinder->GetBindingsForGui( m_bindings );

	// strip any ignore qualifiers
	InputBinder::BindingGroupMap::iterator iGroup = m_bindings.begin();
	for ( ; iGroup != m_bindings.end(); ++iGroup )
	{
		InputBinder::BindingColl::iterator i = iGroup->second.m_Bindings.begin();
		for ( ; i != iGroup->second.m_Bindings.end(); ++i )
		{
			if ( (*i).m_Input0.HasKey() )
			{
				Keys::QUALIFIER qualifiers = (*i).m_Input0.GetKey().GetQualifiers();
				qualifiers &= (Keys::QUALIFIER)( ~Keys::Q_IGNORE_QUALIFIERS );
				// $$ HACKA HACKA HACKA - This is the only command that involves holding down a mouse button
				// and the key at the same time, so I'm hacking the functionality in because there is not
				// enough time to fix the options to handle the ignore qualifiers.
				if ( (*i).m_Name.same_no_case( "formation_increase_spacing" ) ||
					 (*i).m_Name.same_no_case( "formation_decrease_spacing" ) )
				{
					qualifiers |= Keys::Q_IGNORE_BUTTONS;
				}
				(*i).m_Input0 = Input( KeyInput( (*i).m_Input0.GetKey().GetKey(), qualifiers ) );
			}
			else if ( (*i).m_Input0.IsMouse() )
			{
				Keys::QUALIFIER qualifiers = (*i).m_Input0.GetMouse().GetQualifiers();
				qualifiers &= (Keys::QUALIFIER)( ~Keys::Q_IGNORE_QUALIFIERS );
				(*i).m_Input0 = Input( MouseInput( (*i).m_Input0.GetMouse().GetName(), qualifiers ) );
			}

			if ( (*i).m_Input1.HasKey() )
			{
				Keys::QUALIFIER qualifiers = (*i).m_Input1.GetKey().GetQualifiers();
				qualifiers &= (Keys::QUALIFIER)( ~Keys::Q_IGNORE_QUALIFIERS );
				// $$ HACKA HACKA HACKA - This is the only command that involves holding down a mouse button
				// and the key at the same time, so I'm hacking the functionality in because there is not
				// enough time to fix the options to handle the ignore qualifiers.
				if ( (*i).m_Name.same_no_case( "formation_increase_spacing" ) ||
					 (*i).m_Name.same_no_case( "formation_decrease_spacing" ) )
				{
					qualifiers |= Keys::Q_IGNORE_BUTTONS;
				}
				(*i).m_Input1 = Input( KeyInput( (*i).m_Input1.GetKey().GetKey(), qualifiers ) );				
			}
			else if ( (*i).m_Input1.IsMouse() )
			{
				Keys::QUALIFIER qualifiers = (*i).m_Input1.GetMouse().GetQualifiers();
				qualifiers &= (Keys::QUALIFIER)( ~Keys::Q_IGNORE_QUALIFIERS );
				(*i).m_Input1 = Input( MouseInput( (*i).m_Input1.GetMouse().GetName(), qualifiers ) );
			}
		}
	}

	UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "scroll_bindings" );
	if ( pSlider )
	{
		int rowCount = 0;
		InputBinder::BindingGroupMap::iterator i = m_bindings.begin();
		for ( ; i != m_bindings.end(); ++i )
		{
			if ( !i->second.m_ScreenName.empty() )
			{
				++rowCount;
			}
			rowCount += i->second.m_Bindings.size();
		}
		pSlider->SetMax( rowCount - KEY_BINDINGS_ROWS_VISIBLE );
		pSlider->SetMin( 0 );
		pSlider->SetValue( m_CurrentBindingRowAt );
	}

	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_camera_sensitivity" );
	{
		pSlider->SetValue( 0 );
	}

	CreateBindingsList( m_CurrentBindingRowAt );

	UIText * pText = (UIText *)gUIShell.FindUIWindow( "button_inverse_camera_x_text" );
	if( pText )
	{
		if ( gUIGame.GetInvertCameraXAxis() )
		{
			pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "No" ) );
		}
	}
	pText = (UIText *)gUIShell.FindUIWindow( "button_inverse_camera_y_text" );
	if( pText )
	{
		if ( gUIGame.GetInvertCameraYAxis() )
		{
			pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "No" ) );
		}
	}
	pText = (UIText *)gUIShell.FindUIWindow( "button_lock_camera_x_text" );
	if( pText )
	{
		if ( gUICamera.GetIgnoreOrbit() )
		{
			pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "No" ) );
		}
	}
	pText = (UIText *)gUIShell.FindUIWindow( "button_lock_camera_y_text" );
	if( pText )
	{
		if ( gUICamera.GetIgnoreAzimuth() )
		{
			pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "No" ) );
		}
	}

	pText = (UIText *)gUIShell.FindUIWindow( "button_screen_edge_tracking_text" );
	if( pText )
	{
		if ( gUIGame.IsCameraScreenEdgeRotation() )
		{
			pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "No" ) );
		}
	}

	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_mouse_sensitivity" );
	if ( pSlider )
	{
		pSlider->SetMax( 100 );
		pSlider->SetMin( 0 );
		pSlider->SetValue( Round( ((gAppModule.GetMouseScaleX() - 0.5f) * 100) ) );
	}

	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_camera_sensitivity" );
	if ( pSlider )
	{
		FastFuelHandle hCamera( "config:camera_settings" );
		if ( hCamera.IsValid() )
		{
			float currentRate = gUICamera.GetOrbitDegreesPerSecond();
			float defaultRate = RadiansFrom( hCamera.GetFloat( "orbit_degrees_per_second", 60.0f ) );

			pSlider->SetMax( 100 );
			pSlider->SetMin( 0 );
			pSlider->SetValue( Round( (((currentRate - defaultRate) / defaultRate + 0.5f) * 100) ) );
		}
	}
}

void UIOptions::SetInputOptions()
{
	InputBinder *pInputBinder = InputBinder::FindInputBinder( "game" );
	gpassert( pInputBinder );
	pInputBinder->SetBindingsForGui( m_bindings );

	UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_mouse_sensitivity" );
	if ( pSlider )
	{
		float mouseScale = ((float)pSlider->GetValue() / 100) + 0.5f;
		gAppModule.SetMouseScaleX( mouseScale );
		gAppModule.SetMouseScaleY( mouseScale );
	}

	UIText * pText = (UIText *)gUIShell.FindUIWindow( "button_inverse_camera_x_text" );
	if( pText )
	{
		if( pText->GetText().same_no_case( gpwtranslate( $MSG$ "Yes" ) ) )
		{
			gUIGame.SetInvertCameraXAxis( true );
		}
		else
		{
			gUIGame.SetInvertCameraXAxis( false );
		}
	}
	pText = (UIText *)gUIShell.FindUIWindow( "button_inverse_camera_y_text" );
	if( pText )
	{
		if( pText->GetText().same_no_case( gpwtranslate( $MSG$ "Yes" ) ) )
		{
			gUIGame.SetInvertCameraYAxis( true );
		}
		else
		{
			gUIGame.SetInvertCameraYAxis( false );
		}
	}
	pText = (UIText *)gUIShell.FindUIWindow( "button_lock_camera_x_text" );
	if( pText )
	{
		if( pText->GetText().same_no_case( gpwtranslate( $MSG$ "Yes" ) ) )
		{
			gUICamera.SetIgnoreOrbit( true );
		}
		else
		{
			gUICamera.SetIgnoreOrbit( false );
		}
	}
	pText = (UIText *)gUIShell.FindUIWindow( "button_lock_camera_y_text" );
	if( pText )
	{
		if( pText->GetText().same_no_case( gpwtranslate( $MSG$ "Yes" ) ) )
		{
			gUICamera.SetIgnoreAzimuth( true );
		}
		else
		{
			gUICamera.SetIgnoreAzimuth( false );
		}
	}
	pText = (UIText *)gUIShell.FindUIWindow( "button_screen_edge_tracking_text" );
	if( pText )
	{
		if( pText->GetText().same_no_case( gpwtranslate( $MSG$ "Yes" ) ) )
		{
			gUIGame.SetCameraScreenEdgeRotation( true );
		}
		else
		{
			gUIGame.SetCameraScreenEdgeRotation( false );
		}	
	}

	pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_camera_sensitivity" );
	if ( pSlider )
	{
		FastFuelHandle hCamera( "config:camera_settings" );
		if ( hCamera.IsValid() )
		{
			float level = (float)pSlider->GetValue() / 100.0f + 0.5f;

			gUICamera.SetOrbitDegreesPerSecond( RadiansFrom( level * hCamera.GetFloat( "orbit_degrees_per_second", 60.0f ) ) );
			gUICamera.SetAzimuthDegreesPerSecond( RadiansFrom( level * hCamera.GetFloat( "azimuth_degrees_per_second", 30.0f ) ) );
			gUICamera.SetZoomMetersPerSecond( level * hCamera.GetFloat( "zoom_meters_per_second", 8.0f ) );
		}
	}
}

gpwstring UIOptions::CreateKeyScreenString( KeyInput & key )
{
	gpwstring sKey;

	if ( key.GetControlKey() && (key.GetKey() != Keys::KEY_CONTROL) )
	{
		sKey.append( gpwtranslate( $MSG$ "Ctrl" ) );
		sKey.append( L" " );
	}
	if ( key.GetShiftKey() && (key.GetKey() != Keys::KEY_SHIFT) )
	{
		sKey.append( gpwtranslate( $MSG$ "Shift" ) );
		sKey.append( L" " );
	}
	if ( key.GetAltKey() && (key.GetKey() != Keys::KEY_ALT) )
	{
		sKey.append( gpwtranslate( $MSG$ "Alt" ) );
		sKey.append( L" " );
	}
	sKey.append( Keys::ToScreenString( key.GetKey() ) );

	return ( sKey );
}

gpwstring UIOptions::CreateMouseScreenString( MouseInput & mouse )
{
	gpwstring sMouse;

	if ( mouse.GetControlKey() )
	{
		sMouse.append( gpwtranslate( $MSG$ "Ctrl" ) );
		sMouse.append( L" " );
	}
	if ( mouse.GetShiftKey() )
	{
		sMouse.append( gpwtranslate( $MSG$ "Shift" ) );
		sMouse.append( L" " );
	}
	if ( mouse.GetAltKey() )
	{
		sMouse.append( gpwtranslate( $MSG$ "Alt" ) );
		sMouse.append( L" " );
	}

	if ( mouse.GetName().same_no_case( "left_down" ) )
	{
		sMouse.append( gpwtranslate( $MSG$ "Mouse 1" ) );
	}
	else if ( mouse.GetName().same_no_case( "right_down" ) )
	{
		sMouse.append( gpwtranslate( $MSG$ "Mouse 2" ) );
	}
	else if ( mouse.GetName().same_no_case( "middle_down" ) )
	{
		sMouse.append( gpwtranslate( $MSG$ "Mouse 3" ) );
	}
	else if ( mouse.GetName().same_no_case( "x1_down" ) )
	{
		sMouse.append( gpwtranslate( $MSG$ "Mouse 4" ) );
	}
	else if ( mouse.GetName().same_no_case( "x2_down" ) )
	{
		sMouse.append( gpwtranslate( $MSG$ "Mouse 5" ) );
	}
	else if ( mouse.GetName().same_no_case( "wheel_up" ) )
	{
		sMouse.append( gpwtranslate( $MSG$ "Wheel Up" ) );
	}
	else if ( mouse.GetName().same_no_case( "wheel_down" ) )
	{
		sMouse.append( gpwtranslate( $MSG$ "Wheel Down" ) );
	}
	else
	{
		sMouse.append( ReportSys::TranslateW( mouse.GetName() ) );
	}

	return ( sMouse );
}

void UIOptions::CreateBindingsList( int startAt )
{
	int count = 0;
	InputBinder::BindingGroupMap::iterator iGroup;
	for ( iGroup = m_bindings.begin(); iGroup != m_bindings.end(); ++iGroup )
	{
		if ( !iGroup->second.m_ScreenName.empty() )
		{
			if ( count >= (startAt + KEY_BINDINGS_ROWS_VISIBLE) )
			{
				break;
			}
			else if ( count >= startAt )
			{
				m_pInputDescriptions[count - startAt]->SetJustification( JUSTIFY_LEFT );
				m_pInputDescriptions[count - startAt]->SetFont( "b_gui_fnt_14p_copperplate-light" );
				m_pInputDescriptions[count - startAt]->SetText( iGroup->second.m_ScreenName );
				m_pInputPrimary[count - startAt]->SetVisible( false );
				m_pInputSecondary[count - startAt]->SetVisible( false );
			}
			++count;
		}

		InputBinder::BindingColl::iterator i = iGroup->second.m_Bindings.begin();
		for ( ; i != iGroup->second.m_Bindings.end(); ++i )
		{
			if ( count >= (startAt + KEY_BINDINGS_ROWS_VISIBLE) )
			{
				break;
			}
			else if ( count >= startAt )
			{
				m_pInputDescriptions[count - startAt]->SetJustification( JUSTIFY_RIGHT );
				m_pInputDescriptions[count - startAt]->SetFont( "b_gui_fnt_12p_copperplate-light" );
				m_pInputDescriptions[count - startAt]->SetText( (*i).m_ScreenName.c_str() );

				if ( (*i).m_Input0.HasKey() )
				{
					m_pInputPrimary[count - startAt]->SetText( CreateKeyScreenString( (*i).m_Input0.GetKey() ) );
				}
				else if ( (*i).m_Input0.IsMouse() )
				{
					m_pInputPrimary[count - startAt]->SetText( CreateMouseScreenString( (*i).m_Input0.GetMouse() ) );
				}
				else
				{
					m_pInputPrimary[count - startAt]->SetText( L"" );
				}
				m_pInputPrimary[count - startAt]->SetEnabled( true );
				m_pInputPrimary[count - startAt]->SetVisible( true );

				if ( (*i).m_Input1.HasKey() )
				{
					m_pInputSecondary[count - startAt]->SetText( CreateKeyScreenString( (*i).m_Input1.GetKey() ) );
				}
				else if ( (*i).m_Input1.IsMouse() )
				{
					m_pInputSecondary[count - startAt]->SetText( CreateMouseScreenString( (*i).m_Input1.GetMouse() ) );
				}
				else
				{
					m_pInputSecondary[count - startAt]->SetText( L"" );
				}
				m_pInputSecondary[count - startAt]->SetEnabled( true );
				m_pInputSecondary[count - startAt]->SetVisible( true );
			}
			++count;
		}
	}

	if ( count != (startAt + KEY_BINDINGS_ROWS_VISIBLE) )
	{
		// disable the rest
		for ( int index = count; index < (startAt + KEY_BINDINGS_ROWS_VISIBLE); ++index )
		{
			m_pInputDescriptions[count - startAt]->SetText( L"" );

			m_pInputPrimary[count - startAt]->SetText( L"" );
			m_pInputPrimary[count - startAt]->SetVisible( false );

			m_pInputSecondary[count - startAt]->SetText( L"" );
			m_pInputSecondary[count - startAt]->SetVisible( false );
		}
	}

	m_CurrentBindingRowAt = startAt;
}


bool UIOptions::GetSelectedBindingEntry( gpstring const & textName )
{
	// textName is of format "binding#_key#"
	int binding = atoi( textName.mid( 7, 1 ).c_str() );
	int key = atoi( textName.mid( 12, 1 ).c_str() );

	int count = 0;
	InputBinder::BindingGroupMap::iterator iGroup = m_bindings.begin();
	for ( ; iGroup != m_bindings.end(); ++iGroup )
	{
		if ( !iGroup->second.m_ScreenName.empty() )
		{
			++count;
		}

		InputBinder::BindingColl::iterator i = iGroup->second.m_Bindings.begin();
		for ( ; i != iGroup->second.m_Bindings.end(); ++i )
		{
			if ( count >= (m_CurrentBindingRowAt + KEY_BINDINGS_ROWS_VISIBLE) )
			{
				break;
			}
			else if ( count >= m_CurrentBindingRowAt )
			{
				if ( binding == (count - m_CurrentBindingRowAt) )
				{
					if ( key == 0 )
					{
						m_pSelectedBindingInput = &(*i).m_Input0;
						m_pSisterBindingInput = &(*i).m_Input1;
						m_sSelectedBindingName = (*i).m_Name;
					}
					else
					{
						m_pSelectedBindingInput = &(*i).m_Input1;
						m_pSisterBindingInput = &(*i).m_Input0;
						m_sSelectedBindingName = (*i).m_Name;
					}

					return ( true );
				}
			}
			++count;
		}
	}

	m_pSelectedBindingInput = NULL;

	return ( false );
}


bool UIOptions::InputKeyCallback( const KeyInput & key )
{
	// escape key terminates. printscreen not allowed either
	if ( (key.GetKey() == Keys::KEY_ESCAPE) || (key.GetKey() == Keys::KEY_PRINTSCREEN) )
	{
		CreateBindingsList( m_CurrentBindingRowAt );

		m_pInputBinder->SetIsActive( false );

		gUIMessenger.SendUIMessage( UIMessage( MSG_ROLLOFF ), m_pSelectedBindingWindow );

		m_bControlDown = false;
		m_bShiftDown = false;
		m_bAltDown = false;

		return ( true );
	}

	if ( (key.GetQualifiers() & Keys::Q_KEY_DOWN) && (key.GetKey() != Keys::KEY_CONTROL) && (key.GetKey() != Keys::KEY_SHIFT) && (key.GetKey() != Keys::KEY_ALT) )
	{
		// clear any matching bindings
		InputBinder::BindingGroupMap::iterator iGroup = m_bindings.begin();
		for ( ; iGroup != m_bindings.end(); ++iGroup )
		{
			InputBinder::BindingColl::iterator i = iGroup->second.m_Bindings.begin();
			for ( ; i != iGroup->second.m_Bindings.end(); ++i )
			{
				if ( (*i).m_Input0.HasKey() )
				{
					if ( ((*i).m_Input0.GetKey().GetKey() == key.GetKey())
						&& ((*i).m_Input0.GetKey().GetAltKey() == key.GetAltKey()) && ((*i).m_Input0.GetKey().GetControlKey() == key.GetControlKey()) && ((*i).m_Input0.GetKey().GetShiftKey() == key.GetShiftKey()) )
					{
						(*i).m_Input0 = Input( KeyInput( Keys::KEY_NONE, (*i).m_Input0.GetKey().GetQualifiers() ) );
					}
				}
				if ( (*i).m_Input1.HasKey() )
				{
					if ( ((*i).m_Input1.GetKey().GetKey() == key.GetKey())
						&& ((*i).m_Input1.GetKey().GetAltKey() == key.GetAltKey()) && ((*i).m_Input1.GetKey().GetControlKey() == key.GetControlKey()) && ((*i).m_Input1.GetKey().GetShiftKey() == key.GetShiftKey()) )
					{
						(*i).m_Input1 = Input( KeyInput( Keys::KEY_NONE, (*i).m_Input1.GetKey().GetQualifiers() ) );
					}
				}
			}
		}

		KeyInput newKey( key.GetKey() );
		Keys::QUALIFIER q = (key.GetQualifiers() & (Keys::Q_CONTROL | Keys::Q_SHIFT | Keys::Q_ALT));
		q |= (m_pSelectedBindingInput->GetKey().GetQualifiers() & (Keys::Q_KEY_DOWN | Keys::Q_KEY_UP));
		q |= (m_pSisterBindingInput->GetKey().GetQualifiers() & (Keys::Q_KEY_DOWN | Keys::Q_KEY_UP));

		// $$ HACKA HACKA HACKA - This is the only command that involves holding down a mouse button
		// and the key at the same time, so I'm hacking the functionality in because there is not
		// enough time to fix the options to handle the ignore qualifiers.
		if ( m_sSelectedBindingName.same_no_case( "formation_increase_spacing" ) ||
			 m_sSelectedBindingName.same_no_case( "formation_decrease_spacing" ) )
		{
			q |= Keys::Q_IGNORE_BUTTONS;
		}
		m_sSelectedBindingName.clear();

		newKey.SetQualifiers( q );

		// set new binding
		*m_pSelectedBindingInput = Input( newKey );

		CreateBindingsList( m_CurrentBindingRowAt );

		m_pInputBinder->SetIsActive( false );

		gUIMessenger.SendUIMessage( UIMessage( MSG_ROLLOFF ), m_pSelectedBindingWindow );

		m_bControlDown = false;
		m_bShiftDown = false;
		m_bAltDown = false;

		return ( true );
	}

	if ( (key.GetQualifiers() & Keys::Q_KEY_DOWN) && (key.GetKey() == Keys::KEY_CONTROL) )
	{
		m_bControlDown = true;
	}
	else if ( (key.GetQualifiers() & Keys::Q_KEY_UP) && (key.GetKey() == Keys::KEY_CONTROL) )
	{
		m_bControlDown = false;
	}
	else if ( (key.GetQualifiers() & Keys::Q_KEY_DOWN) && (key.GetKey() == Keys::KEY_SHIFT) )
	{
		m_bShiftDown = true;
	}
	else if ( (key.GetQualifiers() & Keys::Q_KEY_UP) && (key.GetKey() == Keys::KEY_SHIFT) )
	{
		m_bShiftDown = false;
	}
	else if ( (key.GetQualifiers() & Keys::Q_KEY_DOWN) && (key.GetKey() == Keys::KEY_ALT) )
	{
		m_bAltDown = true;
	}
	else if ( (key.GetQualifiers() & Keys::Q_KEY_UP) && (key.GetKey() == Keys::KEY_ALT) )
	{
		m_bAltDown = false;
	}

	gpwstring temp;
	if ( m_bControlDown )
	{
		temp.append( gpwtranslate( $MSG$ "Ctrl" ) );
		temp.append( L" " );
	}
	if ( m_bShiftDown )
	{
		temp.append( gpwtranslate( $MSG$ "Shift" ) );
		temp.append( L" " );
	}
	if ( m_bAltDown )
	{
		temp.append( gpwtranslate( $MSG$ "Alt" ) );
		temp.append( L" " );
	}
	temp.append( L"_" );
	m_pSelectedBindingWindow->SetText( temp );

	return ( true );
}

bool UIOptions::InputMouseCallback( const Input & input )
{
	if ( input.IsMouse() )
	{
		if ( input.GetMouse().GetName().same_no_case( "left_down" ) )
		{
			m_pInputBinder->SetIsActive( false );

			CreateBindingsList( m_CurrentBindingRowAt );

			m_bControlDown = false;
			m_bShiftDown = false;
			m_bAltDown = false;

			gUIMessenger.SendUIMessage( UIMessage( MSG_ROLLOFF ), m_pSelectedBindingWindow );
			return ( false );
		}
		else if ( input.GetMouse().GetName().same_no_case( "right_down" ) )
		{
			return ( true );
		}

		// check if the selected binding can be bound to a mouse button
		if ( ((m_pSelectedBindingInput->GetKey().GetQualifiers() & Keys::Q_KEY_DOWN) &&
			  (m_pSelectedBindingInput->GetKey().GetQualifiers() & Keys::Q_KEY_UP) ) ||
			 ((m_pSisterBindingInput->GetKey().GetQualifiers() & Keys::Q_KEY_DOWN) &&
			  (m_pSisterBindingInput->GetKey().GetQualifiers() & Keys::Q_KEY_UP) ) )
		{
			return ( true );
		}

		// clear any matching bindings
		InputBinder::BindingGroupMap::iterator iGroup = m_bindings.begin();
		for ( ; iGroup != m_bindings.end(); ++iGroup )
		{
			InputBinder::BindingColl::iterator i = iGroup->second.m_Bindings.begin();
			for ( ; i != iGroup->second.m_Bindings.end(); ++i )
			{
				if ( (*i).m_Input0.IsMouse() )
				{
					if ( ((*i).m_Input0.GetMouse().GetName().same_no_case( input.GetMouse().GetName() ))
						&& ((*i).m_Input0.GetMouse().GetQualifiers() == input.GetMouse().GetQualifiers()) )
					{
						(*i).m_Input0 = Input();
					}
				}
				if ( (*i).m_Input1.IsMouse() )
				{
					if ( ((*i).m_Input1.GetMouse().GetName().same_no_case( input.GetMouse().GetName() ))
						&& ((*i).m_Input1.GetMouse().GetQualifiers() == input.GetMouse().GetQualifiers()) )
					{
						(*i).m_Input1 = Input();
					}
				}
			}
		}

		// set new binding
		*m_pSelectedBindingInput = input;

		CreateBindingsList( m_CurrentBindingRowAt );

		m_pInputBinder->SetIsActive( false );

		m_bControlDown = false;
		m_bShiftDown = false;
		m_bAltDown = false;

		gUIMessenger.SendUIMessage( UIMessage( MSG_ROLLOFF ), m_pSelectedBindingWindow );
	}

	return ( true );
}


void UIOptions::SetDefaults()
{
	RestoreDefaultVideoOptions();
	RestoreDefaultAudioOptions();
	RestoreDefaultInputOptions();
	RestoreDefaultGameOptions();
}


void UIOptions::SetDefaultShadows()
{
	FastFuelHandle hVideo( "config:options:video" );
	if ( hVideo.IsValid() )
	{
		bool bForceSimple = false;
		if ( !gDefaultRapi.IsMultiTexBlend() )
		{
			bForceSimple = true;
		}

		gpstring shadowType;
		hVideo.Get( "video_shadows", shadowType );
		if ( shadowType.same_no_case( "complex" ) && !bForceSimple )
		{
			if ( !gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleStaticShadows();
			}
			if ( !gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleDynamicShadows();
			}
			if ( !gSiegeEngine.GetOptions().UseExpensiveShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensiveShadows();
			}
			if ( gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensivePartyShadows();
			}			
		}
		else if ( shadowType.same_no_case( "complex_party" ) && !bForceSimple )
		{
			if ( !gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleStaticShadows();
			}
			if ( !gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleDynamicShadows();
			}
			if ( gSiegeEngine.GetOptions().UseExpensiveShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensiveShadows();
			}
			if ( !gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensivePartyShadows();
			}
		}
		else if ( shadowType.same_no_case( "simple" ) || ( bForceSimple && shadowType.same_no_case( "complex" ) ) )
		{
			if ( !gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleStaticShadows();
			}
			if ( !gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleDynamicShadows();
			}
			if ( gSiegeEngine.GetOptions().UseExpensiveShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensiveShadows();
			}
			if ( gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensivePartyShadows();
			}
		}			
		else
		{
			if ( gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleStaticShadows();
			}
			if ( gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleDynamicShadows();
			}
			if ( gSiegeEngine.GetOptions().UseExpensiveShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensiveShadows();
			}
			if ( gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
			{
				gSiegeEngine.GetOptions().ToggleExpensivePartyShadows();
			}			
		}

		// Update shadows
		gGoDb.UpdateShadows();
	}
}

void UIOptions::RestoreDefaultVideoOptions()
{
	FastFuelHandle hVideo( "config:options:video" );
	if ( hVideo.IsValid() )
	{
		gpstring screenRes;
		hVideo.Get( "screen_res", screenRes );

		bool bAllowed = false;
		const DisplayList displayModes = gUIShell.GetRenderer().GetDisplayModeList();

		DisplayList::const_iterator i;
		for ( i = displayModes.begin(); i != displayModes.end(); ++i )
		{				
			gpstring sResolution;
			sResolution.assignf( "%dx%dx%d", i->m_Width, i->m_Height, i->m_BPP );
			for ( ResColl::iterator iRes = m_AllowedRes.begin(); iRes != m_AllowedRes.end(); ++iRes )
			{
				if ( sResolution.same_no_case( screenRes ) )
				{
					bAllowed = true;
					break;
				}
			}					
		}

		if ( !bAllowed )
		{
			hVideo.Get( "screen_res_alternate", screenRes );
		}

		for ( ResColl::iterator iRes = m_AllowedRes.begin(); iRes != m_AllowedRes.end(); ++iRes )
		{
			if ( screenRes.same_no_case( *iRes ) )
			{			
				UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_screen_res" );
				if ( pCombo )
				{
					pCombo->SetText( ToUnicode( screenRes ) );
				}
				break;			
			}
		}
	
		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_texture_filtering_text" );
		if ( pText )
		{			
			if ( m_sDefaultFiltering.same_no_case( "bilinear" ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "Bilinear" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "Trilinear" ) );
			}
		}

		UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_shadows" );
		if ( pCombo )
		{
			bool bForceSimple = false;
			if ( !gDefaultRapi.IsMultiTexBlend() )
			{
				bForceSimple = true;
			}

			gpstring shadowType;
			hVideo.Get( "video_shadows", shadowType );
			if ( shadowType.same_no_case( "complex" ) && !bForceSimple )
			{
				pCombo->SetText( gpwtranslate( $MSG$ "All Complex" ) );
			}
			else if ( shadowType.same_no_case( "complex_party" ) && !bForceSimple )
			{
				pCombo->SetText( gpwtranslate( $MSG$ "Party Complex" ) );
			}
			else if ( shadowType.same_no_case( "simple" ) || ( bForceSimple && shadowType.same_no_case( "complex" ) ) || ( bForceSimple && shadowType.same_no_case( "complex_party" ) ) )
			{
				pCombo->SetText( gpwtranslate( $MSG$ "Simple" ) );
			}			
			else
			{
				pCombo->SetText( gpwtranslate( $MSG$ "None" ) );
			}
		}

		UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_gamma" );
		if( pSlider )
		{
			pSlider->SetValue( (int)floorf( (float)pSlider->GetMax() * (hVideo.GetFloat( "gamma", 1.0f ) - OPTION_MIN_GAMMA_VALUE) / (OPTION_MAX_GAMMA_VALUE - OPTION_MIN_GAMMA_VALUE) + 0.5f ) );
		}

		pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_objectdetail" );		
		if( pSlider )
		{
			pSlider->SetValue( (int)(hVideo.GetFloat( "object_detail", 1.0f )) * 100 );
		}
	}
}

void UIOptions::RestoreDefaultAudioOptions()
{
	FastFuelHandle hAudio( "config:options:audio" );
	if ( hAudio.IsValid() )
	{
		UIText *tb = (UIText *)gUIShell.FindUIWindow( "button_sound_text" );
		if( tb )
		{
			if ( hAudio.GetBool( "sound_enabled", true ) )
			{
				tb->SetText( gpwtranslate( $MSG$ "On" ) );

				m_bTempMuteSound = true;

				UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_mastervolume" );
				if( pSlider )
				{
					gSoundManager.SetMasterVolume( (DWORD)pSlider->GetValue() );
				}
			}
			else
			{
				tb->SetText( gpwtranslate( $MSG$ "Off" ) );

				m_bTempMuteSound = false;

				gSoundManager.SetMasterVolume( (DWORD)0 );
			}
		}

		tb = (UIText *)gUIShell.FindUIWindow( "button_eax_text" );
		if( tb )
		{
			if ( hAudio.GetBool( "eax_enabled", true ) )
			{
				tb->SetText( gpwtranslate( $MSG$ "On" ) );			
			}
			else
			{
				tb->SetText( gpwtranslate( $MSG$ "Off" ) );				
			}

			if ( hAudio.GetBool( "sound_enabled", true ) )
			{
				UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_eax", "options_audio" );
				if ( pButton )
				{
					pButton->EnableButton();
				}
			}
		}

		UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_mastervolume" );
		if( pSlider )
		{
			pSlider->SetValue( hAudio.GetInt( "master_volume", 127 ) );

			if ( hAudio.GetBool( "sound_enabled", true ) )
			{
				pSlider->SetEnabled( true );
			}
		}
		pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_musicvolume" );
		if( pSlider )
		{
			pSlider->SetValue( hAudio.GetInt( "music_volume", 127 ) );
			if ( hAudio.GetBool( "sound_enabled", true ) )
			{
				pSlider->SetEnabled( true );
			}
		}
		pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_sfxvolume" );
		if( pSlider )
		{
			pSlider->SetValue( hAudio.GetInt( "sfx_volume", 127 ) );
			if ( hAudio.GetBool( "sound_enabled", true ) )
			{
				pSlider->SetEnabled( true );
			}
		}
		pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_ambientvolume" );
		if( pSlider )
		{
			pSlider->SetValue( hAudio.GetInt( "ambient_volume", 127 ) );
			if ( hAudio.GetBool( "sound_enabled", true ) )
			{
				pSlider->SetEnabled( true );
			}
		}
		pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_voicevolume" );
		if( pSlider )
		{
			pSlider->SetValue( hAudio.GetInt( "voice_volume", 127 ) );
			if ( hAudio.GetBool( "sound_enabled", true ) )
			{
				pSlider->SetEnabled( true );
			}
		}
	}
}

void UIOptions::RestoreDefaultInputOptions()
{
	FastFuelHandle hInput( "config:options:input" );
	if ( hInput.IsValid() )
	{
		UIText * pText = (UIText *)gUIShell.FindUIWindow( "button_inverse_camera_x_text" );
		if( pText )
		{
			if ( hInput.GetBool( "invert_camera_x_axis", false ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "No" ) );
			}
		}

		pText = (UIText *)gUIShell.FindUIWindow( "button_inverse_camera_y_text" );
		if( pText )
		{
			if ( hInput.GetBool( "invert_camera_y_axis", false ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "No" ) );
			}
		}

		pText = (UIText *)gUIShell.FindUIWindow( "button_lock_camera_x_text" );
		if( pText )
		{
			if ( hInput.GetBool( "lock_camera_x_axis", false ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "No" ) );
			}
		}

		pText = (UIText *)gUIShell.FindUIWindow( "button_lock_camera_y_text" );
		if( pText )
		{
			if ( hInput.GetBool( "lock_camera_y_axis", false ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "No" ) );
			}
		}

		pText = (UIText *)gUIShell.FindUIWindow( "button_screen_edge_tracking_text" );
		if( pText )
		{
			if ( hInput.GetBool( "screen_edge_tracking", true ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "No" ) );
			}
		}

		UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_mouse_sensitivity" );
		if ( pSlider )
		{
			pSlider->SetValue( hInput.GetInt( "mouse_rate", 50 ) );
		}

		pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_camera_sensitivity" );
		if ( pSlider )
		{
			pSlider->SetValue( hInput.GetInt( "camera_rate", 50 ) );
		}
	}
}

void UIOptions::RestoreDefaultGameOptions()
{
	FastFuelHandle hGame( "config:options:game" );
	if ( hGame.IsValid() )
	{
		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_showtooltips_text" );
		if( pText )
		{
			gUIShell.SetToolTipsVisible( hGame.GetBool( "show_tooltips", true ) );
			if ( gUIShell.GetToolTipsVisible() )
			{
				pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "No" ) );
			}
		}

		UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_gamespeed" );
		if( pSlider )
		{
			pSlider->SetValue( (int)floorf( (float)pSlider->GetMax() * (hGame.GetFloat( "game_speed", 1.0f ) - OPTION_MIN_GAME_SPEED) / (OPTION_MAX_GAME_SPEED - OPTION_MIN_GAME_SPEED) + 0.5f ) );
		}

		pText = (UIText *)gUIShell.FindUIWindow( "button_framerate_text" );
		if( pText )
		{
			if ( gSiegeEngine.GetOptions().ShowFPSEnabled() != hGame.GetBool( "show_framerate", false ) )
			{
				gSiegeEngine.GetOptions().ToggleFPS();
			}

			if ( gSiegeEngine.GetOptions().ShowFPSEnabled() )
			{
				pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "No" ) );
			}
		}

		pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_textscroll" );
		if ( pSlider )
		{
			pSlider->SetValue( (int)OPTION_MAX_TEXT_SCROLL - (int)hGame.GetFloat( "text_scroll" ) );
		}

		pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_maxtext" );
		if ( pSlider )
		{
			pSlider->SetValue( hGame.GetInt( "max_text" ) );
		}

		pText = (UIText *)gUIShell.FindUIWindow( "button_tutorial_text" );
		if( pText )
		{
			gUIMenuManager.SetEnableTips( hGame.GetBool( "tutorial_tips", true ) );			
			if ( gUIMenuManager.GetEnableTips() )
			{
				pText->SetText( gpwtranslate( $MSG$ "On" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "Off" ) );
			}
		}

		pText = (UIText *)gUIShell.FindUIWindow( "button_difficulty_text" );
		if( pText )
		{
			gpstring sDifficulty;
			sDifficulty = hGame.GetString( "difficulty" );
			if ( sDifficulty.same_no_case( "Easy" ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "Easy" ) );
				gWorldOptions.SetDifficulty( DIFFICULTY_EASY );
			}
			else if ( sDifficulty.same_no_case( "Normal" ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "Normal" ) );
				gWorldOptions.SetDifficulty( DIFFICULTY_MEDIUM );
			}
			else if ( sDifficulty.same_no_case( "Hard" ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "Hard" ) );
				gWorldOptions.SetDifficulty( DIFFICULTY_HARD );
			}
		}

		pText = (UIText *)gUIShell.FindUIWindow( "button_blood_color_text" );
		if( pText )
		{
			gpstring sBlood = hGame.GetString( "blood" );				
			
			if ( sBlood.same_no_case( "red" ) )
			{	
				gWorldOptions.SetIsBloodEnabled( true );
				gWorldOptions.SetIsBloodRed( true );
				pText->SetText( gpwtranslate( $MSG$ "Red" ) );
			}
			else if ( sBlood.same_no_case( "green" ) )
			{
				gWorldOptions.SetIsBloodEnabled( true );
				gWorldOptions.SetIsBloodRed( false );
				pText->SetText( gpwtranslate( $MSG$ "Green" ) );
			}
			else if ( sBlood.same_no_case( "disabled" ) )
			{
				gWorldOptions.SetIsBloodEnabled( false );
				pText->SetText( gpwtranslate( $MSG$ "Disabled" ) );
			}			
		}

#if !DISABLE_GORE
		pText = (UIText *)gUIShell.FindUIWindow( "button_dismemberment_text" );
		if( pText )
		{
			gWorldOptions.SetAllowDismemberment( hGame.GetBool( "dismemberment", true ) );
			if ( gWorldOptions.GetAllowDismemberment() )
			{
				pText->SetText( gpwtranslate( $MSG$ "On" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "Off" ) );
			}
		}
#endif

		pText = (UIText *)gUIShell.FindUIWindow( "button_share_experience_text" );
		if ( pText )
		{
			gWorldOptions.SetShareExpEnabled( hGame.GetBool( "share_experience", true ) );
			
			if ( gWorldOptions.GetShareExpEnabled() )
			{
				pText->SetText( gpwtranslate( $MSG$ "On" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "Off" ) );
			}
		}

		pText = (UIText *)gUIShell.FindUIWindow( "button_priority_boost_text" );
		if ( pText )
		{
			gAppModule.SetOptions( AppModule::OPTION_BOOST_APP_PRIORITY, hGame.GetBool( "priority_boost", false ) );
			if ( gAppModule.TestOptions( AppModule::OPTION_BOOST_APP_PRIORITY ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "On" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "Off" ) );
			}
		}
		
		if( ::IsSinglePlayer() )
		{
			UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_dropinventory", "options_game" );
			if( pList )
			{				
				eDropInvOption dropInventory;
						
				gpstring sDropInventory = hGame.GetString( "drop_inventory" );			
				if ( sDropInventory.same_no_case( "nothing" ) )
				{	
					dropInventory = DIO_NOTHING;

				}
				else if ( sDropInventory.same_no_case( "equipped" ) )
				{	
					dropInventory = DIO_EQUIPPED;

				}
				else if ( sDropInventory.same_no_case( "inventory" ) )
				{	
					dropInventory = DIO_INVENTORY;

				}
				else
				{	
					dropInventory = DIO_ALL;
				}
				

				gServer.SetDropInvOption( dropInventory );

				pList->SelectElement( (int) dropInventory );

				UIComboBox * pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_dropinventory", "options_game" );
				if ( pCombo )
				{
					pCombo->SetText( pList->GetSelectedText() );
				}			
			}
		}
		if( IsOnZone() )
		{
			UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_zmmessage", "options_game" );
			if( pList )
			{		
				gUIZoneMatch.SetGameMessageOptions( ZONE_MESSAGE_OPTION_EVERYONE );
				pList->SelectElement( (int) ZONE_MESSAGE_OPTION_EVERYONE );

				UIComboBox * pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_zmmessage", "options_game" );
				if ( pCombo )
				{
					pCombo->SetText( pList->GetSelectedText() );
				}			
			}
		}

	}
}

void UIOptions::RestoreDefaultInputBindings()
{
	InputBinder *pInputBinder = InputBinder::FindInputBinder( "game" );
	gpassert( pInputBinder );

	pInputBinder->ClearBindings();
	pInputBinder->BindDefaultInputsToPublishedInterfaces();

	GetInputOptions();
}


void UIOptions::GameGUICallback( gpstring const & message, UIWindow & ui_window )
{
	if ( !IsActive() )
	{
		return;
	}

	if ( message.same_no_case( "set_options" ) )
	{
		SetVideoOptions();
		SetAudioOptions();
		SetInputOptions();
		SetGameOptions();

		m_bTempMuteSound = false;

		gTattooGame.SaveKeyConfig();
		gTattooGame.SavePrefsConfig();

		// close options dialog
		Deactivate();
	}
	else if ( message.same_no_case( "cancel_options" ) )
	{
		// restore cancelled video options
		gUIShell.GetRenderer().SetGamma( CancelGammaValue );

		// restore cancelled audio options
		gSoundManager.SetMasterVolume( (DWORD)CancelMasterVolume );
		gSoundManager.SetStreamVolume( GPGSound::STRM_MUSIC, (DWORD)CancelMusicVolume );
		gSoundManager.SetStreamVolume( GPGSound::STRM_AMBIENT, (DWORD)CancelAmbientVolume );
		gSoundManager.SetStreamVolume( GPGSound::STRM_VOICE, (DWORD)CancelVoiceVolume );
		gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_LOW, (DWORD)CancelSFXVolume );
		gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_NORM, (DWORD)CancelSFXVolume );
		gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_HIGH, (DWORD)CancelSFXVolume );
		gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_AMBIENT, (DWORD)CancelAmbientVolume );

		m_bTempMuteSound = false;

		if ( m_bTempEnableSound && gSoundManager.IsEnabled() )
		{
			gWorldSound.Enable( false, false );
			gWorldOptions.SetSound( false );
		}

		// restore cancelled game options
		if ( CancelShowFPS != gSiegeEngine.GetOptions().ShowFPSEnabled() )
		{
			gSiegeEngine.GetOptions().ToggleFPS();
		}
		gUIShell.SetToolTipsVisible( CancelShowToolTips );
		gUIMenuManager.SetEnableTips( CancelTutorialTips );
		gWorldOptions.SetDifficulty( CancelDifficulty );

		UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box", "console_output" );
		if ( pChat )
		{
			pChat->SetLineTimeout( CancelTextScroll );
			pChat->SetLineSize( CancelMaxText );
			gUIGameConsole.ResizeChatWindow();
		}

		gWorldOptions.SetIsBloodEnabled( CancelBloodEnabled );
		gWorldOptions.SetIsBloodRed( CancelRedBlood );

#if !DISABLE_GORE
		gWorldOptions.SetAllowDismemberment( CancelDismemberment );
#endif

		gWorldOptions.SetShareExpEnabled( CancelShareExperience );
		
		gAppModule.SetOptions( AppModule::OPTION_BOOST_APP_PRIORITY, CancelPriorityBoost );

		gServer.SetDropInvOption( (eDropInvOption) CancelDropInventory );

		if( IsOnZone() )
		{
			gUIZoneMatch.SetGameMessageOptions( (eInGameMessageOption) CancelZMMessage );
		}

		// $$$ maxtext

		Deactivate();
	}
	else if ( message.same_no_case( "show_options_video" ) )
	{
		HideInterfaces();
		gUI.GetGameGUI().ShowInterface( "options_video" );
		m_sLastTab = "options_video";
	}
	else if ( message.same_no_case( "show_options_audio" ) )
	{
		HideInterfaces();
		gUI.GetGameGUI().ShowInterface( "options_audio" );
		m_sLastTab = "options_audio";
	}
	else if ( message.same_no_case( "show_options_input" ) )
	{
		HideInterfaces();
		gUI.GetGameGUI().ShowInterface( "options_input" );
		m_sLastTab = "options_input";
	}
	else if ( message.same_no_case( "show_options_game" ) )
	{
		HideInterfaces();
		gUI.GetGameGUI().ShowInterface( "options_game" );		
		gUIShell.ShowGroup( "options_game_1", true, false, "options_game" );
		gUIShell.ShowGroup( "options_game_2", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_redblood", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_dismember", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_zmmessage", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_dropinventory", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_back", false, false, "options_game" );			
		gUIShell.ShowGroup( "options_game_more", true, false, "options_game" );					

		m_sLastTab = "options_game";
	}
	else if ( message.same_no_case( "toggle_option_texture_filtering" ) )
	{
		UIText *tb = (UIText *)gUIShell.FindUIWindow( "button_texture_filtering_text" );
		if( tb )
		{
			if( tb->GetText().same_no_case( gpwtranslate( $MSG$ "Bilinear" ) ) )
			{
				tb->SetText( gpwtranslate( $MSG$ "Trilinear" ) );
			}
			else
			{
				tb->SetText( gpwtranslate( $MSG$ "Bilinear" ) );
			}
		}
	}
	else if ( message.same_no_case( "slider_change_gamma" ) )
	{
		UISlider *pSlider = (UISlider *)&ui_window;
		if( pSlider )
		{
			gUIShell.GetRenderer().SetGamma( (float)(OPTION_MAX_GAMMA_VALUE - OPTION_MIN_GAMMA_VALUE) * ((float)pSlider->GetValue() / pSlider->GetMax()) + OPTION_MIN_GAMMA_VALUE );
		}
	}
	else if ( message.same_no_case( "toggle_option_audio_sound" ) )
	{
		UIText *tb = (UIText *)gUIShell.FindUIWindow( "button_sound_text" );
		if( tb )
		{
			bool bSoundEnabled = false;
			if( tb->GetText().same_no_case( gpwtranslate( $MSG$ "On" ) ) )
			{
				tb->SetText( gpwtranslate( $MSG$ "Off" ) );

				m_bTempMuteSound = true;

				gSoundManager.SetMasterVolume( (DWORD)0 );

				UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_eax", "options_audio" );
				if ( pButton )
				{
					pButton->DisableButton();
				}
				bSoundEnabled = false;
			}
			else
			{
				tb->SetText( gpwtranslate( $MSG$ "On" ) );

				m_bTempMuteSound = false;

				if ( !gSoundManager.IsEnabled() )
				{
					m_bTempEnableSound = true;
					gWorldSound.Enable( true, false );
					gWorldOptions.SetSound( true );
				}

				UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_mastervolume" );
				if( pSlider )
				{
					gSoundManager.SetMasterVolume( (DWORD)pSlider->GetValue() );
				}

				UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_eax", "options_audio" );
				if ( pButton )
				{
					pButton->EnableButton();
				}
				bSoundEnabled = true;
			}	

			UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_mastervolume" );
			if( pSlider )
			{
				pSlider->SetEnabled( bSoundEnabled );			
			}
			pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_musicvolume" );
			if( pSlider )
			{
				pSlider->SetEnabled( bSoundEnabled );
			}
			pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_sfxvolume" );
			if( pSlider )
			{
				pSlider->SetEnabled( bSoundEnabled );
			}
			pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_ambientvolume" );
			if( pSlider )
			{
				pSlider->SetEnabled( bSoundEnabled );
			}
			pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_voicevolume" );
			if( pSlider )
			{
				pSlider->SetEnabled( bSoundEnabled );
			}
		}
	}
	else if ( message.same_no_case( "toggle_option_audio_eax" ) )
	{
		UIText *tb = (UIText *)gUIShell.FindUIWindow( "button_eax_text" );
		if( tb )
		{
			if( tb->GetText().same_no_case( gpwtranslate( $MSG$ "On" ) ) )
			{
				tb->SetText( gpwtranslate( $MSG$ "Off" ) );
				gWorldSound.Enable( gSoundManager.IsEnabled(), false );		

				m_bTempMuteSound = true;

				if ( ::IsInGame( gWorldState.GetCurrentState() ) )
				{
					gSoundManager.SetMasterVolume( (DWORD)0 );
				}
			}
			else 
			{
				bool bEnabled = gSoundManager.IsEnabled();			
				gWorldSound.Enable( bEnabled, true );

				if ( ::IsInGame( gWorldState.GetCurrentState() ) )
				{
					gSoundManager.SetMasterVolume( (DWORD)0 );
				}

				if ( !gSoundManager.IsEAXEnabled() )				
				{
					gpwstring sError = gpwtranslate( $MSG$ "Cannot detect EAX(TM) Audio support. Please make sure your sound card supports EAX(TM) Audio, and that you have the latest drivers installed." );
					if ( gWorldState.GetCurrentState() == WS_MAIN_MENU )
					{
						gUIFrontend.ShowDialog( sError );
					}
					else
					{
						gUIGame.ShowDialog( DIALOG_OK, sError );
					}

					UISlider *pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_mastervolume" );
					if( pSlider )
					{
						gSoundManager.SetMasterVolume( (DWORD)pSlider->GetValue() );
					}
				}
				else
				{
					tb->SetText( gpwtranslate( $MSG$ "On" ) );				
				}
			}
		}
	}
	else if ( message.same_no_case( "slider_change_volume_master" ) )
	{
		UISlider *pSlider = (UISlider *)&ui_window;
		if( pSlider )
		{
			if ( !m_bTempMuteSound )
			{
				gSoundManager.SetMasterVolume( (DWORD)pSlider->GetValue() );
			}
		}
	}
	else if ( message.same_no_case( "slider_change_volume_music" ) )
	{
		UISlider *pSlider = (UISlider *)&ui_window;
		if( pSlider )
		{
			gSoundManager.SetStreamVolume( GPGSound::STRM_MUSIC, (DWORD)pSlider->GetValue() );
		}
	}
	else if ( message.same_no_case( "slider_change_volume_sfx" ) )
	{
		UISlider *pSlider = (UISlider *)&ui_window;
		if( pSlider )
		{
			gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_LOW, (DWORD)pSlider->GetValue() );
			gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_NORM, (DWORD)pSlider->GetValue() );
			gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_HIGH, (DWORD)pSlider->GetValue() );
		}
	}
	else if ( message.same_no_case( "slider_change_volume_ambient" ) )
	{
		UISlider *pSlider = (UISlider *)&ui_window;
		if( pSlider )
		{
			gSoundManager.SetStreamVolume( GPGSound::STRM_AMBIENT, (DWORD)pSlider->GetValue() );
			gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_AMBIENT, (DWORD)pSlider->GetValue() );
		}
	}
	else if ( message.same_no_case( "slider_change_volume_voice" ) )
	{
		UISlider *pSlider = (UISlider *)&ui_window;
		if( pSlider )
		{
			gSoundManager.SetStreamVolume( GPGSound::STRM_VOICE, (DWORD)pSlider->GetValue() );
		}
	}
	else if ( message.same_no_case( "show_options_bindings" ) )
	{
		HideInterfaces();
		gUI.GetGameGUI().ShowInterface( "options_bindings" );
	}
	else if ( message.same_no_case( "slider_scroll_bindings" ) )
	{
		UISlider *pSlider = (UISlider *)&ui_window;
		if( pSlider )
		{
			CreateBindingsList( pSlider->GetValue() );
		}
	}
	else if ( message.same_no_case( "bind_key" ) )
	{
		if ( GetSelectedBindingEntry( ui_window.GetName() ) )
		{
			ui_window.SetEnabled( false );
			m_pSelectedBindingWindow = (UIText *)&ui_window;
			m_pSelectedBindingWindow->SetText( L"_" );
			m_pInputBinder->SetIsActive( true );
		}
	}
	else if ( message.same_no_case( "stop_bind" ) )
	{
		/*
		if ( m_pInputBinder->IsActive() )
		{
			m_pInputBinder->SetIsActive( false );

			CreateBindingsList( m_CurrentBindingRowAt );

			m_bControlDown = false;
			m_bShiftDown = false;
			m_bAltDown = false;
		}
		*/
	}
	else if ( message.same_no_case( "delete_bound_key" ) )
	{
		if ( GetSelectedBindingEntry( ui_window.GetName() ) )
		{
			*m_pSelectedBindingInput = Input();

			((UIText *)&ui_window)->SetText( L"" );
		}
	}
	else if ( message.same_no_case( "scroll_input_bindings_up" ) )
	{
		if ( ui_window.GetParentWindow() && (ui_window.GetParentWindow()->GetType() == UI_TYPE_SLIDER) )
		{
			UISlider *pSlider = (UISlider *)ui_window.GetParentWindow();
			pSlider->SetValue( pSlider->GetValue() - 2 );
		}
	}
	else if ( message.same_no_case( "scroll_input_bindings_down" ) )
	{
		if ( ui_window.GetParentWindow() && (ui_window.GetParentWindow()->GetType() == UI_TYPE_SLIDER) )
		{
			UISlider *pSlider = (UISlider *)ui_window.GetParentWindow();
			pSlider->SetValue( pSlider->GetValue() + 2 );
		}
	}	
	else if ( message.same_no_case( "toggle_option_input_camera_inverse_x" ) )
	{
		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_inverse_camera_x_text" );
		if( pText )
		{
			if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "No" ) ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "No" ) );
			}
		}
	}
	else if ( message.same_no_case( "toggle_option_input_camera_inverse_y" ) )
	{
		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_inverse_camera_y_text" );
		if( pText )
		{
			if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "No" ) ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "No" ) );
			}
		}
	}
	else if ( message.same_no_case( "toggle_option_input_camera_lock_x" ) )
	{
		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_lock_camera_x_text" );
		if( pText )
		{
			if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "No" ) ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "No" ) );
			}
		}
	}
	else if ( message.same_no_case( "toggle_option_input_camera_lock_y" ) )
	{
		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_lock_camera_y_text" );
		if( pText )
		{
			if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "No" ) ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "No" ) );
			}
		}
	}
	else if ( message.same_no_case( "toggle_option_input_screen_edge_tracking" ) )
	{
		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_screen_edge_tracking_text" );
		if( pText )
		{
			if ( pText->GetText().same_no_case( gpwtranslate( $MSG$ "No" ) ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "No" ) );
			}
		}
	}
	else if ( message.same_no_case( "toggle_option_game_onscreenframerate" ) )
	{
		gSiegeEngine.GetOptions().ToggleFPS();

		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_framerate_text" );
		if( pText )
		{
			if ( gSiegeEngine.GetOptions().ShowFPSEnabled() )
			{
				pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "No" ) );
			}
		}
	}
	else if ( message.same_no_case( "toggle_option_game_showtooltips" ) )
	{
		gUIShell.SetToolTipsVisible( !gUIShell.GetToolTipsVisible() );

		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_showtooltips_text" );
		if( pText )
		{
			if ( gUIShell.GetToolTipsVisible() )
			{
				pText->SetText( gpwtranslate( $MSG$ "Yes" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "No" ) );
			}
		}
	}
	else if ( message.same_no_case( "toggle_option_game_tutorialtips" ) )
	{
		gUIMenuManager.SetEnableTips( !gUIMenuManager.GetEnableTips() );

		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_tutorial_text" );
		if( pText )
		{
			if ( gUIMenuManager.GetEnableTips() )
			{
				pText->SetText( gpwtranslate( $MSG$ "On" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "Off" ) );
			}
		}
	}	
	else if ( message.same_no_case( "toggle_option_game_difficulty" ) )
	{
		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_difficulty_text" );
		if ( pText )
		{
			if ( gWorldOptions.GetDifficulty() == DIFFICULTY_EASY )
			{
				gWorldOptions.SetDifficulty( DIFFICULTY_MEDIUM );
				pText->SetText( gpwtranslate( $MSG$ "Normal" ) );
			}
			else if ( gWorldOptions.GetDifficulty() == DIFFICULTY_MEDIUM )
			{
				gWorldOptions.SetDifficulty( DIFFICULTY_HARD );
				pText->SetText( gpwtranslate( $MSG$ "Hard" ) );
			}
			else if ( gWorldOptions.GetDifficulty() == DIFFICULTY_HARD )
			{
				gWorldOptions.SetDifficulty( DIFFICULTY_EASY );
				pText->SetText( gpwtranslate( $MSG$ "Easy" ) );
			}		
		}
	}
	else if ( message.same_no_case( "toggle_option_game_bloodcolor" ) )
	{
		if ( !gWorldOptions.IsBloodRed() && gWorldOptions.IsBloodEnabled() )
		{
			gWorldOptions.SetIsBloodEnabled( false );
		}
		else
		{
			gWorldOptions.SetIsBloodEnabled( true );
			gWorldOptions.SetIsBloodRed( !gWorldOptions.IsBloodRed() );
		}

		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_blood_color_text" );
		if ( pText )
		{
			if ( gWorldOptions.IsBloodRed() && gWorldOptions.IsBloodEnabled() )
			{
				pText->SetText( gpwtranslate( $MSG$ "Red" ) );
			}
			else if ( !gWorldOptions.IsBloodRed() && gWorldOptions.IsBloodEnabled() )
			{
				pText->SetText( gpwtranslate( $MSG$ "Green" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "Disabled" ) );
			}
		}
	}
	else if ( message.same_no_case( "toggle_option_game_dismemberment" ) )
	{
		gWorldOptions.SetAllowDismemberment( !gWorldOptions.GetAllowDismemberment() );
		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_dismemberment_text" );
		if ( pText )
		{
			if ( gWorldOptions.GetAllowDismemberment() )
			{
				pText->SetText( gpwtranslate( $MSG$ "On" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "Off" ) );
			}
		}
	}
	else if ( message.same_no_case( "toggle_option_game_share_experience" ) )
	{
		gWorldOptions.SetShareExpEnabled( !gWorldOptions.GetShareExpEnabled() );
		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_share_experience_text" );
		if ( pText )
		{
			if ( gWorldOptions.GetShareExpEnabled() )
			{
				pText->SetText( gpwtranslate( $MSG$ "On" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "Off" ) );
			}
		}
	}
	else if ( message.same_no_case( "toggle_option_game_priority_boost" ) )
	{
		gAppModule.SetOptions( AppModule::OPTION_BOOST_APP_PRIORITY, !gAppModule.TestOptions( AppModule::OPTION_BOOST_APP_PRIORITY ) );
		UIText *pText = (UIText *)gUIShell.FindUIWindow( "button_priority_boost_text" );
		if ( pText )
		{
			if ( gAppModule.TestOptions( AppModule::OPTION_BOOST_APP_PRIORITY ) )
			{
				pText->SetText( gpwtranslate( $MSG$ "On" ) );
			}
			else
			{
				pText->SetText( gpwtranslate( $MSG$ "Off" ) );
			}
		}
	}
	else if ( message.same_no_case( "select_option_game_dropinventory" ) )
	{
		if( ::IsSinglePlayer() )
		{
			UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_dropinventory", "options_game" );
			if( pList )
			{
				gServer.SetDropInvOption( (eDropInvOption)pList->GetSelectedTag() );
			}
		}
	}
	else if ( message.same_no_case( "select_option_game_zmmessage" ) )
	{
		if( IsOnZone() )
		{
			UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_zmmessage", "options_game" );
			if( pList )
			{
				gUIZoneMatch.SetGameMessageOptions( (eInGameMessageOption)pList->GetSelectedTag());
			}
		}
	}
	else if ( message.same_no_case( "slider_change_gamespeed" ) )
	{
		UIText * pText = (UIText *)((UISlider *)&ui_window)->GetPopupValueText();
		if ( pText )
		{
			pText->SetText( gpwstringf( L"%1.1f", (float)((UISlider *)&ui_window)->GetValue() / 10 ) );
		}
	}
	else if ( message.same_no_case( "slider_change_maxtext" ) )
	{
		UIText * pText = (UIText *)((UISlider *)&ui_window)->GetPopupValueText();
		if ( pText )
		{
			//pText->SetText( gpwstringf( gpwtranslate( $MSG$ "%d Lines" ), ((UISlider *)&ui_window)->GetValue() ) );
		}
	}
	else if ( message.same_no_case( "default_options_video" ) )
	{
		RestoreDefaultVideoOptions();
	}
	else if ( message.same_no_case( "default_options_audio" ) )
	{
		RestoreDefaultAudioOptions();
	}
	else if ( message.same_no_case( "default_options_input" ) )
	{
		RestoreDefaultInputOptions();
	}
	else if ( message.same_no_case( "default_options_game" ) )
	{
		RestoreDefaultGameOptions();
	}
	else if ( message.same_no_case( "default_options_bindings" ) )
	{
		RestoreDefaultInputBindings();
	}
	else if ( message.same_no_case( "options_game_more" ) )
	{		
		gUIShell.ShowGroup( "options_game_1", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_more", false, false, "options_game" );

		if ( m_bCanShowRedBloodOption )
		{
			gUIShell.ShowGroup( "options_game_redblood", true, false, "options_game" );
		}

		if ( m_bCanShowDismembermentOption )
		{
#if !DISABLE_GORE
			gUIShell.ShowGroup( "options_game_dismember", true, false, "options_game" );		
#endif
		}
		if(  ::IsSinglePlayer() )
		{
			gUIShell.ShowGroup( "options_game_dropinventory", true, false, "options_game" );
		}
		else
		{
			gUIShell.ShowGroup( "options_game_dropinventory", false, false, "options_game" );
		}
		
		if( IsOnZone() )
		{
			gUIShell.ShowGroup( "options_game_zmmessage", true, false, "options_game" );
		}
		else
		{
			gUIShell.ShowGroup( "options_game_zmmessage", false, false, "options_game" );
		}

		gUIShell.ShowGroup( "options_game_2", true, false, "options_game" );
		gUIShell.ShowGroup( "options_game_back", true, false, "options_game" );
	}
	else if ( message.same_no_case( "options_game_back" ) )
	{		
		gUIShell.ShowGroup( "options_game_1", true, false, "options_game" );
		gUIShell.ShowGroup( "options_game_more", true, false, "options_game" );
		
		gUIShell.ShowGroup( "options_game_redblood", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_dismember", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_dropinventory", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_zmmessage", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_back", false, false, "options_game" );
		gUIShell.ShowGroup( "options_game_2", false, false, "options_game" );
	}
	else if ( message.same_no_case( "modal_active" ) )
	{
		if ( UIMenuManager::DoesSingletonExist() )
		{
			gUIMenuManager.SetModalActive( true );
		}
	}
	else if ( message.same_no_case( "modal_inactive" ) )
	{
		if ( UIMenuManager::DoesSingletonExist() )
		{
			gUIMenuManager.SetModalActive( false );
		}
	}
	else if ( message.same_no_case( "show_item_filters" ) )
	{
		Player * pPlayer = gServer.GetScreenPlayer();
		if ( !pPlayer )
		{
			return;
		}

		gUIShell.ShowInterface( "options_item_filters" );		
		
		UICheckbox * pCheckbox = NULL;
		
		// Initialize the loot pickup filter options
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_enchanted_armor_pickup", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootPickupFilterOption( LTF_ENCHANTED_ARMOR ) );
	
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_enchanted_weapons_pickup", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootPickupFilterOption( LTF_ENCHANTED_WEAPONS ) );
		
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_gold_pickup", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootPickupFilterOption( LTF_GOLD ) );
	
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_health_potions_pickup", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootPickupFilterOption( LTF_HEALTH_POTIONS ) );
	
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_jewelry_pickup", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootPickupFilterOption( LTF_JEWELRY ) );
		
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_lore_books_pickup", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootPickupFilterOption( LTF_LOREBOOKS ));
		
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_mana_potions_pickup", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootPickupFilterOption( LTF_MANA_POTIONS ) );
		
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_normal_armor_pickup", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootPickupFilterOption( LTF_NORMAL_ARMOR ) );
		
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_normal_weapons_pickup", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootPickupFilterOption( LTF_NORMAL_WEAPONS ) );
		
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_quest_items_pickup", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootPickupFilterOption( LTF_QUEST_ITEMS ) );
		
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_spells_pickup", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootPickupFilterOption( LTF_SPELLS ) );
		
		// Initialize the loot item label filter options
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_enchanted_armor_label", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootLabelFilterOption( LTF_ENCHANTED_ARMOR ) );
	
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_enchanted_weapons_label", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootLabelFilterOption( LTF_ENCHANTED_WEAPONS ) );
		
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_gold_label", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootLabelFilterOption( LTF_GOLD ) );
	
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_health_potions_label", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootLabelFilterOption( LTF_HEALTH_POTIONS ) );
	
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_jewelry_label", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootLabelFilterOption( LTF_JEWELRY ) );
		
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_lore_books_label", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootLabelFilterOption( LTF_LOREBOOKS ));
		
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_mana_potions_label", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootLabelFilterOption( LTF_MANA_POTIONS ) );
		
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_normal_armor_label", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootLabelFilterOption( LTF_NORMAL_ARMOR ) );
		
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_normal_weapons_label", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootLabelFilterOption( LTF_NORMAL_WEAPONS ) );
		
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_quest_items_label", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootLabelFilterOption( LTF_QUEST_ITEMS ) );
		
		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_spells_label", "options_item_filters" );
		pCheckbox->SetState( pPlayer->GetLootLabelFilterOption( LTF_SPELLS ) );
	}
	else if ( message.same_no_case( "item_filters_close" ) )
	{
		gUIShell.HideInterface( "options_item_filters" );		
	}
	else if ( message.same_no_case( "item_filters_ok" ) )
	{
		Player * pPlayer = gServer.GetScreenPlayer();
		if ( !pPlayer )
		{
			return;
		}

		gUIShell.HideInterface( "options_item_filters" );

		eLootFilter filter = LTF_MISC;

		UICheckbox * pCheckbox = NULL;
		
		// Set the filter for pickup
		{
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_enchanted_armor_pickup", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_ENCHANTED_ARMOR : LTF_INVALID;
			
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_enchanted_weapons_pickup", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_ENCHANTED_WEAPONS : LTF_INVALID;
			
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_gold_pickup", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_GOLD : LTF_INVALID;
			
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_health_potions_pickup", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_HEALTH_POTIONS : LTF_INVALID;		

			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_jewelry_pickup", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_JEWELRY : LTF_INVALID;
				
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_lore_books_pickup", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_LOREBOOKS : LTF_INVALID;		

			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_mana_potions_pickup", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_MANA_POTIONS : LTF_INVALID;
				
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_normal_armor_pickup", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_NORMAL_ARMOR : LTF_INVALID;
			
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_normal_weapons_pickup", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_NORMAL_WEAPONS : LTF_INVALID;
				
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_quest_items_pickup", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_QUEST_ITEMS : LTF_INVALID;
			
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_spells_pickup", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_SPELLS : LTF_INVALID;

			pPlayer->RSSetLootPickupFilter( filter );
		}

		// Set the filter for labels
		{
			filter = LTF_MISC;

			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_enchanted_armor_label", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_ENCHANTED_ARMOR : LTF_INVALID;
			
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_enchanted_weapons_label", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_ENCHANTED_WEAPONS : LTF_INVALID;
			
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_gold_label", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_GOLD : LTF_INVALID;
			
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_health_potions_label", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_HEALTH_POTIONS : LTF_INVALID;		

			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_jewelry_label", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_JEWELRY : LTF_INVALID;
				
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_lore_books_label", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_LOREBOOKS : LTF_INVALID;		

			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_mana_potions_label", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_MANA_POTIONS : LTF_INVALID;
				
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_normal_armor_label", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_NORMAL_ARMOR : LTF_INVALID;
			
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_normal_weapons_label", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_NORMAL_WEAPONS : LTF_INVALID;
				
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_quest_items_label", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_QUEST_ITEMS : LTF_INVALID;
			
			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_filter_spells_label", "options_item_filters" );
			filter |= pCheckbox->GetState() ? LTF_SPELLS : LTF_INVALID;		

			pPlayer->RSSetLootLabelFilter( filter );
		}		
	}
}
