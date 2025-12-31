/**************************************************************************

Filename		: UIFrontend.cpp

Class			: UIFrontend

Description		: Contains the class which handles the initial menu
				  interface.

Creation Date	: 10/12/99

**************************************************************************/


// Include Files
#include "Precomp_Game.h"
#include "UIFrontEnd.h"

#include "AppModule.h"
#include "ContentDb.h"
#include "FileSysUtils.h"
#include "Fuel.h"
#include "GameMovie.h"
#include "GoActor.h"
#include "GoAspect.h"
#include "GoDb.h"
#include "InputBinder.h"
#include "NamingKey.h"
#include "Nema_AspectMgr.h"
#include "Player.h"
#include "RapiImage.h"
#include "Server.h"
#include "siege_camera.h"
#include "siege_options.h"
#include "TankMgr.h"
#include "TattooGame.h"
#include "UI_ChatBox.h"
#include "UI_EditBox.h"
#include "UI_Messenger.h"
#include "UI_Mouse_Listener.h"
#include "UI_Shell.h"
#include "UI_TextBox.h"
#include "UICharacterSelect.h"
#include "UIMultiPlayer.h"
#include "UIOptions.h"
#include "UIZoneMatch.h"
#include "World.h"
#include "WorldMap.h"
#include "WorldState.h"
#include "WorldFx.h"
#include "Flamethrower_effect_base.h"
#include "MessageDispatch.h"
#include "Gps_manager.h"


using namespace nema;

/**************************************************************************

Function		: UIFrontend()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIFrontend::UIFrontend()
	: m_bIsVisible			( false	)
	, m_pCharacterSelect	( 0 )
	, m_orthoMeterPerPixel	( 0.00375f )
	, m_bBeginLoad			( false )
	, m_bDrawCharSelect		( false )
	, m_bProviderInit		( false )
	, m_dwQuickColor		( 0xffffffff )
	, m_dwAutoColor			( 0xffffffff )
	, m_LogoTimeout			( 10.0f )
	, m_LogoCurrentTime		( 0.0f )
	, m_bLogoOut			( false )
	, m_bGuiInitialized		( false )
	, m_NextWorldState		( WS_INVALID )
	, m_FrontendMusic		( GPGSound::INVALID_SOUND_ID )
	, m_logoSample			( GPGSound::INVALID_SOUND_ID )
	, m_bVersionSet			( false )
{
	m_pInputBinder = new InputBinder( "frontend" );

	gAppModule.RegisterBinder( m_pInputBinder, 100 );

	// Publish information to the Input binder
	PublishInterfaceToInputBinder();
	BindInputToPublishedInterface();

	m_pCharacterSelect = NULL;

	InitGui();

	FastFuelHandle hCamera( "ui:config:object_settings:object_settings:camera" );
	if ( hCamera.IsValid() )
	{
		m_Camera.targetPos.pos	= vector_3( 0.0f, 0.0f, 0.0f );
		hCamera.Get( "distance", m_Camera.distance );
		hCamera.Get( "orbit", m_Camera.orbitAngle );
		hCamera.Get( "azimuth", m_Camera.azimuthAngle );
	}
	else
	{
		m_Camera.targetPos.pos	= vector_3( 0.0f, 0.0f, 0.0f );
		m_Camera.distance		= 6.0f;
		m_Camera.orbitAngle		= 0.0f;
		m_Camera.azimuthAngle	= 0.0;
	}

	FastFuelHandle hSettings( "ui:config:object_settings:object_settings" );
	if ( hSettings.IsValid() )
	{
		hSettings.Get( "ortho_meter_per_pixel", m_orthoMeterPerPixel );

		float widthOfScene = 0.0f;
		if ( hSettings.Get( "width_of_scene", widthOfScene ) )
		{
			float fov = gSiegeEngine.GetCamera().GetFieldOfView();
			m_Camera.distance = ( widthOfScene / 2 ) / ( (float)tan( RadiansToDegrees( fov ) ) );
		}
	}

	FastFuelHandle hGlobalSettings( "config:global_settings:timeouts" );
	if ( hGlobalSettings.IsValid() )
	{
		hGlobalSettings.Get( "ui_logo_time", m_LogoTimeout );
	}

	InitGuiLights();
	InitTexSwapMap();

#	if !GP_RETAIL

	gUIShell.ActivateInterface( "ui:interfaces:frontend:debug_help", false );

#	endif

}




/**************************************************************************

Function		: ~UIFrontend()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIFrontend::~UIFrontend()
{
	if ( m_pInputBinder != NULL )
	{
		Delete( m_pInputBinder );
	}
	if ( m_pCharacterSelect != NULL )
	{
		Delete( m_pCharacterSelect );
	}

	UnloadData();

#	if !GP_RETAIL

	gUIShell.MarkInterfaceForDeactivation( "debug_help" );

#	endif

}




void UIFrontend::SetIsVisible( bool flag )
{
	m_bIsVisible = flag;
	m_pInputBinder->SetIsActive( flag );
}



/**************************************************************************

Function		: GameGUICallback()

Purpose			: Receives notification messages from the GUI.

Returns			: void

**************************************************************************/
void UIFrontend::GameGUICallback( gpstring const & message, UIWindow & ui_window )
{
	if ( !IsVisible() )
	{
		return;
	}

	GuiToLightMap::iterator iGui;
	for ( iGui = m_guiLightMap.begin(); iGui != m_guiLightMap.end(); ++iGui )
	{
		(*iGui).second.ReceiveNotification( message );
		GuiLightColl::iterator iLights;
		for ( iLights = (*iGui).second.m_guiLights.begin(); iLights != (*iGui).second.m_guiLights.end(); ++iLights )
		{
			(*iLights).ReceiveNotification( message );
		}
	}

	if ( message.same_no_case( "back_to_main" ) )
	{
		SetIsVisible( true );
		gUIShell.HideInterface( "single_player" );

		PlayFrontendSound( "frontend_small_button" );
		PlayFrontendSound( "frontend_menu_roll" );

		gWorldStateRequest( WS_MAIN_MENU );
	}
	else if ( message.same_no_case( "to_main_menu" ) )
	{
		gUI.GetUIFrontend().SetIsVisible( true );
		gUI.GetUIFrontend().HideInterfaces();
		//GuiFlyin();
		gWorldStateRequest( WS_MAIN_MENU );
	}
	else if( message.same_no_case( "map_button_ok" )		)
	{
		PlayFrontendSound( "frontend_small_button" );

		UIListbox *lb = (UIListbox *)gUI.GetGameGUI().FindUIWindow( "map_listbox" );
		if( lb )
		{
			if ( lb->GetSelectedTag() != -1 )
			{
				gUIShell.HideInterface( "map_chooser" );

				GuiLoadMapToDifficulty();

				SetSelectedMapName( m_mapInfoColl[ lb->GetSelectedTag() ].m_InternalName );
			}
		}
	}
	else if ( message.same_no_case( "map_button_cancel" )	)
	{
		gUIShell.HideInterface( "map_chooser" );

		GuiBackToHeroSelect();

		PlayFrontendSound( "frontend_small_button" );
		PlayFrontendSound( "frontend_menu_open" );

		gWorldStateRequest( WS_SP_CHARACTER_SELECT );
	}
	else if ( message.same_no_case( "difficulty_easy" ) )
	{
		PlayFrontendSound( "frontend_big_button" );		

		if ( !gUIFrontend.ShowCDCheckDialog() )
		{
			return;
		}

		gWorldOptions.SetDifficulty( DIFFICULTY_EASY );
		CheckLoadMap();
	}
	else if ( message.same_no_case( "difficulty_medium" ) )
	{
		PlayFrontendSound( "frontend_big_button" );		

		if ( !gUIFrontend.ShowCDCheckDialog() )
		{
			return;
		}

		gWorldOptions.SetDifficulty( DIFFICULTY_MEDIUM );
		CheckLoadMap();
	}
	else if ( message.same_no_case( "difficulty_hard" ) )
	{
		PlayFrontendSound( "frontend_big_button" );		

		if ( !gUIFrontend.ShowCDCheckDialog() )
		{
			return;
		}

		gWorldOptions.SetDifficulty( DIFFICULTY_HARD );
		CheckLoadMap();
	}
	else if ( message.same_no_case( "difficulty_back" ) )
	{
		if ( gWorldState.GetCurrentState() == WS_SP_MAP_SELECT )
		{
			GuiDifficultyToLoadMap();
		}
		else
		{
			gWorldStateRequest( WS_SP_CHARACTER_SELECT );
			GuiDifficultyToHeroSelect();
		}

		gUIShell.HideInterface( "difficulty_menu" );

		PlayFrontendSound( "frontend_small_button" );
		PlayFrontendSound( "frontend_menu_roll" );
	}
	else if ( message.same_no_case( "exit_button_press" ) )
	{
		PlayFrontendSound( "frontend_small_button" );

		gAppModule.RequestQuit();
		return;
	}
	else if ( message.same_no_case( "show_options" ) )
	{
		PlayFrontendSound( "frontend_big_button" );

		gUIShell.HideInterface( "main_menu" );
		gUIOptions.Activate();
	}
	else if ( message.same_no_case( "multi_player_button_press" ) )
	{
#		if !GP_DISABLE_MULTIPLAYER

#		if !GP_RETAIL
		if( !HasDungeonSiegeSpecialEnableDsrMpFile() )
		{
			return;
		}
#		endif

		gUIShell.HideInterface( "main_menu" );

		gWorldStateRequest( WS_MP_PROVIDER_SELECT );

		GuiMultiplayer();

		PlayFrontendSound( "frontend_big_button" );
		PlayFrontendSound( "frontend_menu_roll" );

#		endif
	}
	else if ( message.same_no_case( "provider_matchmaker" ) )
	{
		if ( !gUIZoneMatch.Init() )
		{
			gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "ZoneMatch could not be started." ) );
			return;
		}

		MultiplayerPrep();

		PlayFrontendSound( "frontend_big_button" );

		gWorldStateRequest( WS_MP_MATCH_MAKER );
	}
	else if ( message.same_no_case( "provider_internet" ) )
	{
		MultiplayerPrep();

		PlayFrontendSound( "frontend_big_button" );

		gWorldStateRequest( WS_MP_INTERNET_GAME );
	}
	else if ( message.same_no_case( "provider_lan" ) )
	{
		MultiplayerPrep();

		PlayFrontendSound( "frontend_big_button" );

		gWorldStateRequest( WS_MP_LAN_GAME );
	}
	else if ( message.same_no_case( "mp_dialog_ok" ) )
	{
		gUIShell.HideInterface( "mp_dialog" );
	}
	else if ( message.same_no_case( "multi_player_to_main" ) )
	{
		gUIShell.HideInterface( "multiplayer_provider" );

		gWorldStateRequest( WS_MAIN_MENU );

		PlayFrontendSound( "frontend_small_button" );
		PlayFrontendSound( "frontend_menu_roll" );
	}
	else if ( message.same_no_case( "about_button_press" ) )
	{
		PlayFrontendSound( "frontend_big_button" );

		gUIShell.HideInterface( "main_menu" );
		gUI.GetGameGUI().ShowInterface( "about_dialog" );
	}
	else if ( message.same_no_case( "close_about" ) )
	{
		PlayFrontendSound( "frontend_tiny_button" );

		gUIShell.HideInterface( "about_dialog" );
		gUI.GetGameGUI().ShowInterface( "main_menu" );
	}
	else if ( message.same_no_case( "continue_button_press" ) )
	{
		if ( gWorldState.GetPendingState() != WS_LOADING_SAVE_GAME )
		{
			PlayFrontendSound( "frontend_big_button" );
			if ( gTattooGame.HasRecentSaveGame() )
			{
				gTattooGame.LoadRecentGame();
			}
			else
			{
				gUIShell.ShowInterface( "continue_dialog" );
			}
		}
	}
	else if ( message.same_no_case( "exit_continue" ) )
	{
		gUIShell.HideInterface( "continue_dialog" );
	}
	else if ( message.same_no_case( "show_single_player_menu" ) )
	{
		gUIShell.HideInterface( "main_menu" );

		GuiSinglePlayer();

		PlayFrontendSound( "frontend_big_button" );
		PlayFrontendSound( "frontend_menu_roll" );

		gWorldStateRequest( WS_SP_MAIN_MENU );
	}
	else if ( message.same_no_case( "load_game_to_single_player" ) )
	{
		gUIShell.HideInterface( "load_game" );

		GuiBackToSPFromLG();

		PlayFrontendSound( "frontend_small_button" );
		PlayFrontendSound( "frontend_menu_roll" );

		gWorldStateRequest( WS_SP_MAIN_MENU );
	}
	else if ( message.same_no_case( "show_character_selection" ) )
	{
		AppModule::AutoFreezeTime freezeTime;

		gUIShell.HideInterface( "single_player" );

		m_pCharacterSelect->LoadCustomSelections();
		GuiStartGame();
		gWorldStateRequest( WS_SP_CHARACTER_SELECT );

		PlayFrontendSound( "frontend_big_button" );
		PlayFrontendSound( "frontend_menu_open" );
	}
	else if ( message.same_no_case( "next_character" ) )
	{
		PlayFrontendSound( "frontend_tiny_button" );
		m_pCharacterSelect->SetNextActiveCharacter();
	}
	else if ( message.same_no_case( "previous_character" ) )
	{
		PlayFrontendSound( "frontend_tiny_button" );
		m_pCharacterSelect->SetPreviousActiveCharacter();
	}
	else if ( message.same_no_case( "char_next_pants" ) )
	{
		PlayFrontendSound( "frontend_tiny_button" );
		m_pCharacterSelect->SelectNextPantsTexture();
	}
	else if ( message.same_no_case( "char_prev_pants" ) )
	{
		PlayFrontendSound( "frontend_tiny_button" );
		m_pCharacterSelect->SelectPreviousPantsTexture();
	}
	else if ( message.same_no_case( "char_next_shirt" ) )
	{
		PlayFrontendSound( "frontend_tiny_button" );
		m_pCharacterSelect->SelectNextShirtTexture();
	}
	else if ( message.same_no_case( "char_prev_shirt" ) )
	{
		PlayFrontendSound( "frontend_tiny_button" );
		m_pCharacterSelect->SelectPreviousShirtTexture();
	}
	else if ( message.same_no_case( "char_prev_face" ) )
	{
		PlayFrontendSound( "frontend_tiny_button" );
		m_pCharacterSelect->SelectPreviousFaceTexture();
	}
	else if ( message.same_no_case( "char_next_face" ) )
	{
		PlayFrontendSound( "frontend_tiny_button" );
		m_pCharacterSelect->SelectNextFaceTexture();
	}
	else if ( message.same_no_case( "char_prev_hair" ) )
	{
		PlayFrontendSound( "frontend_tiny_button" );
		m_pCharacterSelect->SelectPreviousHairTexture();
	}
	else if ( message.same_no_case( "char_next_hair" ) )
	{
		PlayFrontendSound( "frontend_tiny_button" );
		m_pCharacterSelect->SelectNextHairTexture();
	}
	else if ( message.same_no_case( "char_prev_head" ) )
	{
		PlayFrontendSound( "frontend_tiny_button" );
		m_pCharacterSelect->SelectPreviousHeadTexture();
	}
	else if ( message.same_no_case( "char_next_head" ) )
	{
		PlayFrontendSound( "frontend_tiny_button" );
		m_pCharacterSelect->SelectNextHeadTexture();
	}
	else if ( message.same_no_case( "load_single_player" ) )
	{
		// get save infos
		gTattooGame.GetCampaignSaveGameInfos( m_saveInfoColl, true );

		if ( m_saveInfoColl.size() == 0 )
		{
			gUIShell.ShowInterface( "continue_dialog" );
		}
		else
		{
			gUI.GetUIFrontend().HideInterfaces();
			gWorldStateRequest( WS_SP_LOAD_GAME_SCREEN );
		
			GuiLoadGame();			
			PlayFrontendSound( "frontend_menu_roll" );
		}
		PlayFrontendSound( "frontend_big_button" );
	}
	else if ( message.same_no_case( "load_select_game" ) )
	{
		LoadGameSelect();
	}
	else if ( message.same_no_case( "load_game_ok" ) )
	{
		PlayFrontendSound( "frontend_small_button" );

		UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "load_game_listbox" );
		if( pListbox )
		{
			int tag = pListbox->GetSelectedTag();
			if ( m_saveInfoColl.size() != 0 && tag != -1 )
			{
				if( !gTattooGame.LoadGame( m_saveInfoColl[ tag ]->m_FileName ) )
				{
					// $$$ need a real error dialog here... -sb
					gpassertm( 0, "Load game failed." );
				}
			}
		}
	}
	else if ( message.same_no_case( "on_change_map" ) )
	{
		AppModule::AutoFreezeTime freezeTime;

		PlayFrontendSound( "frontend_small_button" );

		gpwstring sName = GetEnteredName();
		if ( !IsNameValid( sName ) )
		{
			SelectNameDialog();
			return;
		}
		else
		{
			gpwstring errorMsg;
			if ( !TattooGame::IsValidSaveScreenName( sName, &errorMsg ) )
			{
				ShowDialog( errorMsg );
				return;
			}
		}

		PlayFrontendSound( "frontend_menu_close" );

		// tell player object who was selected as the Hero...
		m_pCharacterSelect->AcceptCharacterSelection();

		gUIShell.HideInterface( "character_select" );

		SetMapStrings();

		UIListbox *lb = (UIListbox *)gUI.GetGameGUI().FindUIWindow( "map_listbox" );
		if( lb )
		{
			if ( lb->GetNumElements() == 1 )
			{
				GuiHeroSelectToDifficulty();
				SetSelectedMapName( m_mapInfoColl.front().m_InternalName );
				gWorldStateRequest( WS_SP_DIFFICULTY_SELECT );
			}
			else
			{
				if ( !gAppModule.GetShiftKey() )
				{
					GuiLoadMap();
					gWorldStateRequest( WS_SP_MAP_SELECT );
				}
				else
				{
					GuiHeroSelectToDifficulty();
					SetSelectedMapName( "map_world" );
					gWorldStateRequest( WS_SP_DIFFICULTY_SELECT );
				}
			}
		}
	}
	else if ( message.same_no_case( "listener_mouse_move" ) )
	{
		m_pCharacterSelect->RotateY( ((float)(((UIMouseListener *)&ui_window)->GetDeltaX())/100.0f) );
	}
	else if ( message.same_no_case( "submit_text" ) )
	{
		UIEditBox * pEdit = (UIEditBox *)&ui_window;
		UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "chatbox" );
		pChat->SubmitText( pEdit->GetText(), 0xFFFFFFFF );
	}
	else if ( message.same_no_case( "back_to_single_player" ) )
	{
		PlayFrontendSound( "frontend_small_button" );
		PlayFrontendSound( "frontend_menu_roll" );

		gUIShell.HideInterface( "character_select" );

		GuiBackToSinglePlayer();

		gWorldStateRequest( WS_SP_MAIN_MENU );
	}
	else if ( message.same_no_case( "button_down" ) )
	{
		GuiTextureSwap( ui_window.GetName(), ui_window.GetIndex(), "mousedown" );
	}
	else if ( message.same_no_case( "button_up" ) )
	{
		if ( gUIShell.IsInterfaceVisible( "main_menu" ) )
		{
			UIWindow * pWindow = gUIShell.FindUIWindow( "legal_info", "main_menu" );
			pWindow->SetVisible( true );
		}

		GuiTextureSwap( ui_window.GetName(), ui_window.GetIndex(), "mouseout" );
		UITextBox * pBox = (UITextBox *)gUIShell.FindUIWindow( "frontend_help", "frontend_help" );
		if ( pBox )
		{
			pBox->SetText( L"" );
		}
	}
	else if ( message.same_no_case( "button_rollover" ) )
	{
		if ( gUIShell.IsInterfaceVisible( "main_menu" ) )
		{
			UIWindow * pWindow = gUIShell.FindUIWindow( "legal_info", "main_menu" );
			pWindow->SetVisible( false );
		}

		GuiTextureSwap( ui_window.GetName(), ui_window.GetIndex(), "mouseover" );
	}
	else if ( message.same_no_case( "exit_select_name" ) )
	{
		PlayFrontendSound( "frontend_tiny_button" );
		gUIShell.HideInterface( "select_name_dialog" );
		UIEditBox *pEdit = (UIEditBox *)gUIShell.FindUIWindow( "name_edit_box" );
		if ( pEdit )
		{
			pEdit->SetAllowInput( true );
		}
	}
	else if ( message.same_no_case( "delete_save_game" ) )
	{
		gUIShell.ShowInterface( "save_delete_confirm" );
	}
	else if ( message.same_no_case( "save_delete_cancel" ) )
	{
		gUIShell.HideInterface( "save_delete_confirm" );
	}
	else if ( message.same_no_case( "save_delete_confirm" ) )
	{
		gpwstring fileName;

		UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "load_game_listbox" );
		if ( pListbox )
		{
			int tag = pListbox->GetSelectedTag();
			gpwstring fileName = m_saveInfoColl[ tag ]->m_FileName;
			if ( fileName.empty() )
			{
				return;
			}

			gTattooGame.DeleteSaveGame( fileName );
		}

		UIWindow * pWindow = gUIShell.FindUIWindow( "preview_window", "load_game" );
		if ( pWindow )
		{
			gUIShell.GetRenderer().DestroyTexture( pWindow->GetTextureIndex() );
			pWindow->SetTextureIndex( 0 );
			pWindow->SetHasTexture( false );
		}

		gUIShell.HideInterface( "save_delete_confirm" );

		// get save infos
		m_saveInfoColl.clear();
		gTattooGame.GetCampaignSaveGameInfos( m_saveInfoColl, true );

		ConstructSaveList();

		LoadGameCompletionCb( 0 );

		if ( pListbox )
		{
			if ( pListbox->GetNumElements() == 0 )
			{
				UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "delete_button", "load_game" );
				if ( pButton )
				{
					pButton->DisableButton();
				}

				pButton = (UIButton *)gUIShell.FindUIWindow( "load_button", "load_game" );
				if ( pButton )
				{
					pButton->DisableButton();
				}
			}
		}		
	}
	else if ( message.same_no_case( "button_credits" ) )
	{
		gUIShell.HideInterface( "main_menu" );
		gWorldStateRequest( WS_CREDITS );
		gGameMovie.InitializeMovieSequence( "movies:credits_movie" );
	}
	else if ( message.same_no_case( "select_map" ) )
	{
		UIListbox *lb = (UIListbox *)gUI.GetGameGUI().FindUIWindow( "map_listbox" );
		if( lb )
		{
			if ( lb->GetSelectedTag() != -1 )
			{
				UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "map_description", "map_chooser" );
				pTextBox->SetText( m_mapInfoColl[ lb->GetSelectedTag() ].m_Description );
			}
		}
	}
	else if ( message.same_no_case( "hide_fade_screen" ) )
	{
		gUIShell.HideInterface( "fade_screen" );
	}
	else if ( message.same_no_case( "fe_dialog_ok" ) )
	{
		gUIShell.HideInterface( "frontend_dialog" );
	}
	else if ( message.same_no_case( "about_dialog" ) )
	{
		HKEY hKey = NULL;
		char szTemp[256] = "";
		DWORD dwType = 0;
		DWORD dwSize = sizeof( szTemp );	
		RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Microsoft Games\\DungeonSiege\\1.0", 0, KEY_READ, &hKey );

		RegQueryValueEx( hKey, "PID", 0, &dwType, (unsigned char *)szTemp, &dwSize );	
		RegCloseKey( hKey );		

		gpwstring sAbout;		
		sAbout.assignf( gpwtranslate( $MSG$ "Dungeon Siege\nVersion %S\n\n(c) 2002 Gas Powered Games Corp. All rights reserved. Gas Powered Games, the GPG logo, and Dungeon Siege are the exclusive trademarks of Gas Powered Games Corp.\n\nPortions (c) 2002 Microsoft Corporation. All rights reserved.\n\nThis product is licensed. Your product identification number is: %s.\n\nUses Bink Video. Copyright (c) 1997-2002 by RAD Game Tools, Inc.\n\nWarning: This computer program is protected by copyright law and international treaties. Unauthorized reproduction or distribution of this program, or any portion of it, may result in severe civil and criminal penalties, and will be prosecuted to the maximum extent possible under the law." ),
						SysInfo::GetExeFileVersionText( gpversion::MODE_AUTO_SHORT ).c_str(), ToUnicode( szTemp ).c_str() );
		((UITextBox *)&ui_window)->SetText( sAbout );
	}
}


void UIFrontend::MultiplayerPrep()
{
	DeactivateInterfaces();
	SetIsVisible( false );
	gUIMultiplayer.SetIsVisible( true );
}


void UIFrontend::CheckLoadMap()
{
	HideInterfaces();

	// go to black right away
	gDefaultRapi.ClearAllBuffers();

	if ( gWorldMap.SSet( GetSelectedMapName(), NULL, RPC_TO_ALL ) )
	{
		SetIsVisible( false );
	}
	else
	{
		SetSelectedMapName( "" );
		ShowDialog( gpwtranslate( $MSG$ "The selected map could not be loaded." ) );

		if ( gWorldState.GetCurrentState() == WS_SP_MAP_SELECT )
		{
			GuiDifficultyToLoadMap();
		}
		else
		{
			gWorldStateRequest( WS_SP_CHARACTER_SELECT );
			GuiDifficultyToHeroSelect();
		}
	}
}


void UIFrontend::BeginLoadMap()
{
	if ( GetBeginLoadMap() )
	{	
		SetSelectedMapName( "" );
		gWorldStateRequest( WS_LOAD_MAP );
		SetBeginLoadMap( false );
	}
}

void UIFrontend::LoadGameSelect()
{
	if ( m_saveInfoColl.size() == 0 )
	{
		return;
	}

	gpwstring sFilename;

	UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "load_game_listbox" );
	if( pListbox )
	{
		sFilename = pListbox->GetSelectedText();
	}

	UIWindow * pWindow = gUIShell.FindUIWindow( "preview_window", "load_game" );
	if ( pWindow )
	{
		gUIShell.GetRenderer().DestroyTexture( pWindow->GetTextureIndex() );
	}

	UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "loadsave_game_name_text" );
	if( pTextBox )
	{
		int tag = pListbox->GetSelectedTag();
		if ( tag == -1 )
		{
			tag = pListbox->GetTag();
		}
		if ( tag != -1 )
		{
			pTextBox->SetText( L"" );
			gpwstring sTemp;
			sTemp.assignf( gpwtranslate( $MSG$ "Hero: %s" ), m_saveInfoColl[ tag ]->m_HeroName.c_str() );
			pTextBox->SetLineText( 0, sTemp );

			sTemp.assignf( gpwtranslate( $MSG$ "Map: %s" ), m_saveInfoColl[ tag ]->m_MapScreenName.c_str() );
			pTextBox->SetLineText( 1, sTemp );

			sTemp.assignf( gpwtranslate( $MSG$ "Elapsed Time: %s" ), m_saveInfoColl[ tag ]->m_ElapsedTimeText.c_str() );
			pTextBox->SetLineText( 2, sTemp );

			pListbox->SelectElement( tag );
		}

		if ( pWindow )
		{
			unsigned int tex = m_saveInfoColl[tag]->CreateThumbnailTexture();
			pWindow->SetTextureIndex( tex );
			pWindow->SetVisible( true );
			pWindow->SetHasTexture( true );
		}

		UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "delete_button", "load_game" );
		if ( (tag != -1) && (m_saveInfoColl[ tag ]->m_IsQuickSave || m_saveInfoColl[ tag ]->m_IsAutoSave) )
		{
			pButton->DisableButton();
		}
		else
		{
			pButton->EnableButton();
		}
	}

	UIText * pText = (UIText *)gUIShell.FindUIWindow( "load_game_name_text" );
	if( pText )
	{
		pText->SetText( sFilename );
	}
}


void UIFrontend::ConstructSaveList()
{
	UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "load_game_listbox" );
	if( pListbox )
	{		
		// re-sort based on save time
		m_saveInfoColl.sort( SaveInfo::GreaterByFileTime() );

		// add each
		pListbox->RemoveAllElements();

		// autosave first
		SaveInfoColl::iterator i;
		int listIndex = 0;
		for( i = m_saveInfoColl.begin(); i != m_saveInfoColl.end(); ++i )
		{
			if ( (*i)->m_IsAutoSave )
			{
				pListbox->AddElement( (*i)->m_ScreenName, listIndex, m_dwAutoColor );
				break;
			}

			++listIndex;
		}

		// quicksave second
		listIndex = 0;
		for( i = m_saveInfoColl.begin(); i != m_saveInfoColl.end(); ++i )
		{
			if ( (*i)->m_IsQuickSave )
			{
				pListbox->AddElement( (*i)->m_ScreenName, listIndex, m_dwQuickColor );
				break;
			}

			++listIndex;
		}

		// and the rest
		listIndex = 0;
		for( i = m_saveInfoColl.begin(); i != m_saveInfoColl.end(); ++i )
		{
			if ( !( (*i)->m_IsQuickSave || (*i)->m_IsAutoSave ) )
			{
				pListbox->AddElement( (*i)->m_ScreenName, listIndex );
			}

			++listIndex;
		}

		UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_save_delete", "loadsave_game" );
		if ( pButton )
		{
			if ( m_saveInfoColl.empty() )
			{
				pButton->SetEnabled( false );
				pButton->SetAlpha( 0.6f );
			}
			else
			{
				pButton->SetEnabled( true );
				pButton->SetAlpha( 1.0f );
			}
		}
	}
}


bool UIFrontend::ShowCDCheckDialog()
{
	if ( !gTattooGame.CheckCdInDrive() )
	{
		gUI.GetGameGUI().GetMessenger()->RegisterCallback( makeFunctor( *this, &UIFrontend::CDCheckGUICallback ) );

		gUI.GetGameGUI().MarkInterfaceForActivation( "ui:interfaces:frontend:cdcheck_dialog", true );			
		return false;
	}	
	
	return true;
}


void UIFrontend::HideCDCheckDialog()
{	
	gUI.GetGameGUI().GetMessenger()->UnRegisterCallback( makeFunctor( *this, &UIFrontend::CDCheckGUICallback ) );
	gUI.GetGameGUI().MarkInterfaceForDeactivation( "cdcheck_dialog" );
}


void UIFrontend::CDCheckGUICallback( gpstring const & message, UIWindow & /* ui_window */ )
{
	if ( message.same_no_case( "cdcheck_cancel" ) )
	{
		HideCDCheckDialog();
	}
	else if ( message.same_no_case( "cdcheck_okay" ) )
	{
		HideCDCheckDialog();
		ShowCDCheckDialog();
	}
}


/**************************************************************************

Function		: DeactivateInterfaces()

Purpose			: Clear all the active interfaces

Returns			: void

**************************************************************************/
void UIFrontend::DeactivateInterfaces()
{
	HideInterfaces();

	gUI.GetGameGUI().MarkInterfaceForDeactivation( "main_menu" );
	gUI.GetGameGUI().MarkInterfaceForDeactivation( "single_player" );
	gUI.GetGameGUI().MarkInterfaceForDeactivation( "load_game" );
	gUI.GetGameGUI().MarkInterfaceForDeactivation( "character_select" );
	gUI.GetGameGUI().MarkInterfaceForDeactivation( "map_chooser" );
	gUI.GetGameGUI().MarkInterfaceForDeactivation( "select_name_dialog" );
	gUI.GetGameGUI().MarkInterfaceForDeactivation( "multiplayer_provider" );
	gUI.GetGameGUI().MarkInterfaceForDeactivation( "difficulty_menu" );
	gUI.GetGameGUI().MarkInterfaceForDeactivation( "about_dialog" );
	gUI.GetGameGUI().MarkInterfaceForDeactivation( "continue_dialog" );
	gUI.GetGameGUI().MarkInterfaceForDeactivation( "frontend_help" );
	gUI.GetGameGUI().MarkInterfaceForDeactivation( "frontend_dialog" );
	gUI.GetGameGUI().MarkInterfaceForDeactivation( "save_delete_confirm" );	
	gUI.GetGameGUI().MarkInterfaceForDeactivation( "fade_screen" );	
	gUI.GetGameGUI().GetMessenger()->UnRegisterCallback( makeFunctor( *this, &UIFrontend::GameGUICallback ) );

	Delete( m_pCharacterSelect );

	m_mapInfoColl.clear();
	m_saveInfoColl.clear();

	SetIsVisible( false );

	m_bProviderInit		= false;
	m_bGuiInitialized	= false;

	if ( gWorldState.GetPreviousState() != WS_CREDITS && gWorldState.GetPreviousState() != WS_LOADING_SAVE_GAME )
	{
		m_bVersionSet		= false;
	}
}


void UIFrontend::HideInterfaces()
{
	gUI.GetGameGUI().HideInterface( "main_menu" 		 );
	gUI.GetGameGUI().HideInterface( "single_player"      );
	gUI.GetGameGUI().HideInterface( "load_game"		     );
	gUI.GetGameGUI().HideInterface( "character_select"   );
	gUI.GetGameGUI().HideInterface( "map_chooser"        );
	gUI.GetGameGUI().HideInterface( "select_name_dialog" );
	gUI.GetGameGUI().HideInterface( "multiplayer_provider" );
	gUI.GetGameGUI().HideInterface( "difficulty_menu" );
	gUI.GetGameGUI().HideInterface( "about_dialog" );
	gUI.GetGameGUI().HideInterface( "save_delete_confirm" );	
}


void UIFrontend::UnloadData()
{
	gWorldFx.SetAllowUndefinedNodeGUID( false );

	if ( GoDb::DoesSingletonExist() )
	{
		gGoDb.Shutdown();
	}

	TextureColl::iterator i;
	for ( i = m_preloadTextures.begin(); i != m_preloadTextures.end(); ++i )
	{
		gUIShell.GetRenderer().DestroyTexture( *i );
	}
	m_preloadTextures.clear();

	gUI.GetGameGUI().GetMessenger()->UnRegisterCallback( makeFunctor( *this, &UIFrontend::GameGUICallback ) );
}


/**************************************************************************

Function		: GetInputBinder()

Purpose			: Retrieves access to the input binder

Returns			: void

**************************************************************************/
InputBinder & UIFrontend::GetInputBinder()
{
	gpassert( m_pInputBinder );
	return( *m_pInputBinder );
}



/**************************************************************************

Function		: Update()

Purpose			: Updates the UI ( animations, etc. )

Returns			: void

**************************************************************************/
void UIFrontend::Update( double secondsElapsed )
{
	gWorldTime.Update( (float)secondsElapsed );
	gMessageDispatch.Update();	

	// flamethrower does its own timeslicing
	gWorldFx.Update( (float)secondsElapsed, (float)secondsElapsed );	

	gUI.GetGameGUI().Update( secondsElapsed, gAppModule.GetCursorX(), gAppModule.GetCursorY() );

	if( (gWorldState.GetCurrentState() == WS_MP_PROVIDER_SELECT ||
		 gWorldState.GetPreviousState() == WS_MP_PROVIDER_SELECT) && !m_bProviderInit )
	{
		GoHandle hMenuBars( m_guiMenuBars );
		gpstring sTexture = "b_gui_fe_m_mn_3d_text-menubars2";
		hMenuBars->GetAspect()->SetCurrentAspectTexture( 11, sTexture.c_str() );
		hMenuBars->GetAspect()->SetCurrentAspectTexture( 13, sTexture.c_str() );
		hMenuBars->GetAspect()->SetCurrentAspectTexture( 15, sTexture.c_str() );
		m_bProviderInit = true;
	}

	if( (gWorldState.GetCurrentState() == WS_SP_CHARACTER_SELECT) && !gWorldState.IsStateChangePending() )
	{
		m_pCharacterSelect->Update( secondsElapsed );
	}
	
	GoHandle hMainMenu( m_guiMainMenu );
	GoHandle hHeroMenu( m_guiHeroMenu );
	GoHandle hRightSide( m_guiRightSide );
	GoHandle hLeftSide( m_guiLeftSide );
	GoHandle hMenuBars( m_guiMenuBars );
	GoHandle hBackButton( m_guiBackButton );
	GoHandle hLoadMap( m_guiLoadMap );
	if ( hMainMenu.IsValid() )
	{

		hMainMenu->Update( (float)secondsElapsed );
		hHeroMenu->Update( (float)secondsElapsed );
		hRightSide->Update( (float)secondsElapsed );
		hLeftSide->Update( (float)secondsElapsed );
		hMenuBars->Update( (float)secondsElapsed );
		hBackButton->Update( (float)secondsElapsed );
		hLoadMap->Update( (float)secondsElapsed );	
	}
	
	if ( gWorldState.GetCurrentState() == WS_LOGO )
	{
		GoHandle hLogo( m_guiLogo );
		if ( hLogo.IsValid() && hLogo->GetAspect()->GetAspectHandle().IsValid() )
		{
			hLogo->Update( (float)secondsElapsed );			
		}

		m_LogoCurrentTime += (float)secondsElapsed;

		if ( m_LogoCurrentTime >= m_LogoTimeout )
		{
			GuiLogoOut();
			m_LogoCurrentTime = 0.0f;
		}
	}

	UpdateGuiLights( (float)secondsElapsed );

	if ( !::IsInGame( gWorldState.GetCurrentState() ) && gWorldState.GetCurrentState() != WS_INTRO &&
		 gWorldState.GetCurrentState() != WS_LOGO && !gSoundManager.IsStreamPlaying( m_FrontendMusic ) )
	{
		StartFrontendMusic();
		gSoundManager.Update( 0 );
	}
}





/**************************************************************************

Function		: Draw()

Purpose			: Render all the UI interfaces/windows

Returns			: void

**************************************************************************/
void UIFrontend::Draw( double /*seconds_elapsed*/ )
{
	if ( gWorldState.GetCurrentState() != WS_INTRO )
	{
		GoHandle hBackdrop( m_guiBackdrop );
		GoHandle hMainMenu( m_guiMainMenu );
		GoHandle hHeroMenu( m_guiHeroMenu );
		GoHandle hLeftside( m_guiLeftSide );
		GoHandle hMenuBars( m_guiMenuBars );
		GoHandle hRightSide( m_guiRightSide );
		GoHandle hBackButton( m_guiBackButton );
		GoHandle hLoadMap( m_guiLoadMap );

		if ( hBackdrop.IsValid() && hMainMenu.IsValid() )
		{
			gSiegeEngine.Renderer().SetTextureStageState(	0,
															D3DTOP_MODULATE,
															D3DTOP_SELECTARG1,
															D3DTADDRESS_CLAMP,
															D3DTADDRESS_CLAMP,
															D3DTEXF_LINEAR,
															D3DTEXF_LINEAR,
															D3DTEXF_LINEAR,
															D3DBLEND_SRCALPHA,
															D3DBLEND_INVSRCALPHA,
															false );

			SetTemporaryView();
			gSiegeEngine.Renderer().SetWorldMatrix( matrix_3x3() , vector_3(0,0,0) );

			CalculateGuiLights( hBackdrop );
			CalculateGuiLights( hMainMenu );
			CalculateGuiLights( hHeroMenu );
			CalculateGuiLights( hLeftside );
			CalculateGuiLights( hMenuBars );
			CalculateGuiLights( hRightSide );
			CalculateGuiLights( hBackButton );

			if ( gWorldState.GetCurrentState() == WS_SP_MAP_SELECT ||
				 gWorldState.GetPreviousState() == WS_SP_MAP_SELECT ||
				 gWorldState.GetCurrentState() == WS_SP_LOAD_GAME_SCREEN ||
				 gWorldState.GetPreviousState() == WS_SP_LOAD_GAME_SCREEN )
			{
				CalculateGuiLights( hLoadMap );
			}

			gDefaultRapi.SetOrthoMatrix( m_orthoMeterPerPixel );

			hBackdrop->GetAspect()->GetAspectHandle()->Render( &gSiegeEngine.Renderer() );
			DrawEffects( hBackdrop );

			hRightSide->GetAspect()->GetAspectHandle()->Render( &gSiegeEngine.Renderer() );
			DrawEffects( hRightSide );
			hLeftside->GetAspect()->GetAspectHandle()->Render( &gSiegeEngine.Renderer() );
			DrawEffects( hLeftside );
			
			if ( gWorldState.GetCurrentState() != WS_LOGO && gWorldState.GetCurrentState() != WS_INTRO )
			{
				hHeroMenu->GetAspect()->GetAspectHandle()->Render( &gSiegeEngine.Renderer() );
				DrawEffects( hHeroMenu );

				hMainMenu->GetAspect()->GetAspectHandle()->Render( &gSiegeEngine.Renderer() );
				DrawEffects( hMainMenu );

				hMenuBars->GetAspect()->GetAspectHandle()->Render( &gSiegeEngine.Renderer() );
				DrawEffects( hMenuBars );

				hBackButton->GetAspect()->GetAspectHandle()->Render( &gSiegeEngine.Renderer() );
				DrawEffects( hBackButton );
			}

			if( gWorldState.GetCurrentState() == WS_SP_CHARACTER_SELECT || m_bDrawCharSelect )
			{
				m_bDrawCharSelect = true;
				m_pCharacterSelect->Draw();
			}

			gSiegeEngine.Renderer().SetTextureStageState(	0,
															D3DTOP_MODULATE,
															D3DTOP_SELECTARG1,
															D3DTADDRESS_CLAMP,
															D3DTADDRESS_CLAMP,
															D3DTEXF_LINEAR,
															D3DTEXF_LINEAR,
															D3DTEXF_LINEAR,
															D3DBLEND_SRCALPHA,
															D3DBLEND_INVSRCALPHA,
															false );

			SetTemporaryView();
			gSiegeEngine.Renderer().SetWorldMatrix( matrix_3x3() , vector_3(0,0,0) );
			gDefaultRapi.SetOrthoMatrix( m_orthoMeterPerPixel );

			if ( gWorldState.GetCurrentState() == WS_SP_MAP_SELECT ||
				 gWorldState.GetPreviousState() == WS_SP_MAP_SELECT ||
				 gWorldState.GetCurrentState() == WS_SP_LOAD_GAME_SCREEN ||
				 gWorldState.GetPreviousState() == WS_SP_LOAD_GAME_SCREEN )
			{
				hLoadMap->GetAspect()->GetAspectHandle()->Render( &gSiegeEngine.Renderer() );
				DrawEffects( hLoadMap );
			}
		}
	}

	if ( gWorldState.GetCurrentState() != WS_INTRO )
	{
		gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZENABLE, FALSE );
	}

	// Draw the user interface
	gUI.GetGameGUI().Draw();

	if ( gWorldState.GetCurrentState() != WS_INTRO )
	{
		gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZENABLE, TRUE );
	}
	
	if ( gWorldState.GetCurrentState() == WS_LOGO )
	{
		bool bAlphaBlend = gDefaultRapi.EnableAlphaBlending( true );

		GoHandle hLogo( m_guiLogo );
		if ( hLogo.IsValid() )
		{
			gSiegeEngine.Renderer().SetTextureStageState(	0,
															D3DTOP_MODULATE,
															D3DTOP_SELECTARG1,
															D3DTADDRESS_CLAMP,
															D3DTADDRESS_CLAMP,
															D3DTEXF_LINEAR,
															D3DTEXF_LINEAR,
															D3DTEXF_LINEAR,
															D3DBLEND_SRCALPHA,
															D3DBLEND_INVSRCALPHA,
															false );

			SetTemporaryView();
			gSiegeEngine.Renderer().SetWorldMatrix( matrix_3x3() , vector_3(0,0,0) );

			CalculateGuiLights( hLogo );

			gDefaultRapi.SetOrthoMatrix( m_orthoMeterPerPixel );

			hLogo->GetAspect()->GetAspectHandle()->Render( &gSiegeEngine.Renderer() );
			DrawEffects( hLogo);
		}

		gDefaultRapi.EnableAlphaBlending( bAlphaBlend );
	}	

	SetBeginLoadMap( false );	
}


void UIFrontend::DrawEffects( Go * pObject )
{
	matrix_3x3 cam_orient	= gWorldFx.GetCameraOrientation();
	gWorldFx.SetCameraOrientation( gSiegeEngine.GetCamera().GetMatrixOrientation() );
	gWorldFx.SaveRenderingState();

	// Go get the scripts attached to this go
	GoDb::EffectDb::iterator i, begin, end;
	gGoDb.GetEffectScriptIters( pObject, begin, end );

	for( i = begin; i != end; ++i )
	{
		if( gWorldFx.IsScriptOwnerVisible( (*i).second.m_ScriptId ) )
		{
			SEVector effectColl;

			// Get the effects associated with this script
			gWorldFx.GetEffectsForScript( (*i).second.m_ScriptId, effectColl );

			// Go through each effect
			for( SEVector::iterator e = effectColl.begin(); e != effectColl.end(); ++e )
			{
				// Draw effect
				(*e)->Draw();
			}
		}
	}

	gWorldFx.SetCameraOrientation( cam_orient );
	gWorldFx.RestoreRenderingState();

	SetTemporaryView();
	gSiegeEngine.Renderer().SetWorldMatrix( matrix_3x3() , vector_3(0,0,0) );
	gDefaultRapi.SetOrthoMatrix( m_orthoMeterPerPixel );
}


void UIFrontend::CalculateGuiLights( Go * pGo )
{
	pGo->GetAspect()->GetAspectHandle()->Deform();

	GuiToLightMap::iterator iGui = m_guiLightMap.find( pGo->GetTemplateName() );
	if ( iGui != m_guiLightMap.end() )
	{
		pGo->GetAspect()->GetAspectHandle()->InitializeLighting( (*iGui).second.m_ambience, 255 );
		GuiLightColl::iterator iLights;
		for ( iLights = (*iGui).second.m_guiLights.begin(); iLights != (*iGui).second.m_guiLights.end(); ++iLights )
		{
			pGo->GetAspect()->GetAspectHandle()->CalculateShading( (*iLights).m_nls, gSiegeEngine.GetOptions().IsLightRaysDrawingEnabled() );
		}
	}
}


void UIFrontend::SetTemporaryView()
{
	// Build the camera position using the angles
	vector_3 cameraPos;

	float d;
	SINCOSF( m_Camera.azimuthAngle, cameraPos.y, d );
	SINCOSF( m_Camera.orbitAngle, cameraPos.x, cameraPos.z );

	d			*= m_Camera.distance;
	cameraPos.x	*= d;
	cameraPos.y	*= m_Camera.distance;
	cameraPos.z	*= d;

	gSiegeEngine.GetCamera().SetCameraAndTargetPosition( cameraPos, m_Camera.targetPos.pos );

	gSiegeEngine.GetCamera().UpdateCamera();
}


void UIFrontend::PublishInterfaceToInputBinder()
{
	GetInputBinder().PublishVoidInterface( "skip_current_movie", makeFunctor( *this, &UIFrontend::OnEscape ) );
	GetInputBinder().PublishVoidInterface( "restart_movie", makeFunctor( *this, &UIFrontend::RestartMovie ) );
	GetInputBinder().PublishVoidInterface( "pause_movie", makeFunctor( *this, &UIFrontend::PauseMovie ) );
	GetInputBinder().PublishVoidInterface( "selection_down", makeFunctor( *this, &UIFrontend :: SkipMovie ) );
	GetInputBinder().PublishVoidInterface( "special_down", makeFunctor( *this, &UIFrontend :: SkipMovie ) );
	GetInputBinder().PublishVoidInterface( "context_down", makeFunctor( *this, &UIFrontend :: SkipMovie ) );
}


void UIFrontend::BindInputToPublishedInterface()
{
	GetInputBinder().BindDefaultInputsToPublishedInterfaces();
}


bool UIFrontend::SkipMovie()
{
	if ( gGameMovie.IsMoviePlaying() )
	{
		gGameMovie.SkipCurrentMovie();
	}
	else if ( gWorldState.GetCurrentState() == WS_LOGO )
	{
		GuiLogoOut();
	}
	return true;
}

bool UIFrontend::OnEscape()
{
	if ( gGameMovie.IsMoviePlaying() )
	{
		gGameMovie.SkipCurrentMovie();
	}
	else if ( gWorldState.GetCurrentState() == WS_LOGO )
	{
		GuiLogoOut();
	}
	else if ( gUIShell.IsInterfaceVisible( "single_player" ) )
	{
		gUI.GetUIFrontend().SetIsVisible( true );
		gUIShell.HideInterface( "single_player" );
		//GuiBackToMain();
		PlayFrontendSound( "frontend_menu_roll" );
		gWorldStateRequest( WS_MAIN_MENU );
	}
	else if ( gUIShell.IsInterfaceVisible( "load_game" ) )
	{
		gUIShell.HideInterface( "load_game" );
		GuiBackToSPFromLG();
		PlayFrontendSound( "frontend_menu_roll" );
		gWorldStateRequest( WS_SP_MAIN_MENU );

	}
	else if ( gUIShell.IsInterfaceVisible( "character_select" ) )
	{
		gUIShell.HideInterface( "character_select" );
		GuiBackToSinglePlayer();
		PlayFrontendSound( "frontend_menu_roll" );
		gWorldStateRequest( WS_SP_MAIN_MENU );
	}
	else if ( gUIShell.IsInterfaceVisible( "map_chooser" ) )
	{
		gUIShell.HideInterface( "map_chooser" );
		GuiBackToHeroSelect();
		PlayFrontendSound( "frontend_menu_roll" );
		gWorldStateRequest( WS_SP_CHARACTER_SELECT );
	}
	else if ( gUIShell.IsInterfaceVisible( "difficulty_menu" ) )
	{
		if ( gWorldState.GetCurrentState() == WS_SP_MAP_SELECT )
		{
			GuiDifficultyToLoadMap();
		}
		else
		{
			gWorldStateRequest( WS_SP_CHARACTER_SELECT );
			GuiDifficultyToHeroSelect();
		}
		PlayFrontendSound( "frontend_menu_roll" );
		gUIShell.HideInterface( "difficulty_menu" );
	}
	else if ( gUIShell.IsInterfaceVisible( "multiplayer_provider" ) )
	{
		gUI.GetUIFrontend().SetIsVisible( true );
		gUIShell.HideInterface( "multiplayer_provider" );
		//GuiBackToMainFromMP();
		PlayFrontendSound( "frontend_menu_roll" );
		gWorldStateRequest( WS_MAIN_MENU );
	}

	return false;
}


bool UIFrontend::RestartMovie()
{
	gGameMovie.RestartSequence();
	return false;
}


bool UIFrontend::PauseMovie()
{
	if ( gGameMovie.IsMoviePlaying() )
	{
		if ( gWorldState.GetCurrentState() == WS_CREDITS )
		{
			gGameMovie.PauseCurrentMovie( !gGameMovie.IsMoviePaused() );
		}
		else
		{
			gGameMovie.SkipCurrentMovie();
		}
	}
	return false;
}


bool MapHandleCompare( const FastFuelHandle& l, const FastFuelHandle& r )
{
	gpstring lstr = l.GetString( "name" );
	gpstring rstr = r.GetString( "name" );
	return ( lstr.compare_no_case( rstr ) < 0 );
}


void UIFrontend::SetMapStrings()
{
	UIListbox *listbox = (UIListbox *)gUI.GetGameGUI().FindUIWindow( "map_listbox", "map_chooser" );
	if ( !listbox ) {
		return;
	}
	listbox->RemoveAllElements();

	// get maps
	gTankMgr.GetMapInfos( m_mapInfoColl );

	// re-sort based on map screen name
	m_mapInfoColl.sort( MapInfo::LessByScreenName() );

	// add each single player map
	MapInfoColl::iterator i = m_mapInfoColl.begin();
	int listIndex = 0;
	bool devOnly = false;
	while ( i != m_mapInfoColl.end() )
	{
		// only skip multiplayer maps in retail builds
		bool dontDoit = GP_RETAIL || gWorldOptions.GetForceRetailContent();
		if ( dontDoit && i->m_IsMultiplayerOnly )
		{
			i = m_mapInfoColl.erase( i );
			continue;
		}

		// add divider
		if ( !devOnly && i->m_IsDevOnly )
		{
			listbox->AddElement( L"---", -1 );
			devOnly = true;
		}

		listbox->AddElement( i->m_ScreenName, listIndex );
		++listIndex;
		++i;
	}
}


gpwstring UIFrontend::GetEnteredName()
{
	UIEditBox *pEdit = (UIEditBox *)gUIShell.FindUIWindow( "name_edit_box", "character_select" );
	if ( pEdit )
	{
		if ( !pEdit->GetText().empty() )
		{
			gpwstring sName = pEdit->GetText();
			IsNameValid( sName );
			return sName;
		}
	}
	return L"";
}


bool UIFrontend::IsNameValid( gpwstring & sName )
{
	if ( sName.size() == 0 )
	{
		return false;
	}

	gpstring sAnsiName = ToAnsi( sName );
	stringtool::RemoveLeadingWhiteSpace( sAnsiName );
	stringtool::RemoveTrailingWhiteSpace( sAnsiName );

	sName = ToUnicode( sAnsiName );
	if ( sName.size() != 0 )
	{
		return true;
	}

	return false;
}


void UIFrontend::SelectNameDialog()
{
	gUIShell.ShowInterface( "select_name_dialog" );
}


void UIFrontend::InitGuiLights()
{
	m_guiLightMap.clear();
	FastFuelHandle hLight( "ui:config:frontend_lights:frontend_lights" );
	if ( hLight.IsValid() )
	{
		FastFuelHandleColl hlScenes;
		hLight.ListChildren( hlScenes );
		FastFuelHandleColl::iterator i;
		for ( i = hlScenes.begin(); i != hlScenes.end(); ++i )
		{
			GuiLightScene lightScene;

			// Get the base ambience
			(*i).Get( "ambience", lightScene.m_ambience );

			// Load in ambience notifications
			FastFuelHandle hAmbientNotify = (*i).GetChildNamed( "notifications" );
			if ( hAmbientNotify.IsValid() )
			{
				FastFuelHandleColl hlMessages;
				FastFuelHandleColl::iterator iNotify;
				hAmbientNotify.ListChildren( hlMessages );
				for ( iNotify = hlMessages.begin(); iNotify != hlMessages.end(); ++iNotify )
				{
					NotifyAmbience na;
					na.bRamp			= false;
					na.finalAmbience	= 0;
					na.startAmbience	= 0;
					na.rampDuration		= 0.0f;
					na.secondsElapsed	= 0.0f;
					na.bTriggered		= false;

					(*iNotify).Get( "ambience", na.finalAmbience );
					(*iNotify).Get( "start_ambience", na.startAmbience );
					(*iNotify).Get( "ramp", na.bRamp );
					(*iNotify).Get( "ramp_duration", na.rampDuration );

					lightScene.m_notifyAmbience.insert( NotifyAmbiencePair( (*iNotify).GetName(), na ) );
				}
			}

			// Get the lightsource data
			FastFuelHandleColl hlLights;
			(*i).ListChildrenNamed( hlLights, "light*" );
			FastFuelHandleColl::iterator j;
			for ( j = hlLights.begin(); j != hlLights.end(); ++j )
			{
				// Read in base lightsource information
				nema::LightSource nls;

				gpstring sTemp;
				(*j).Get( "lightsource_type", sTemp );
				if ( sTemp.same_no_case( "infinite" ) )
				{
					nls.m_Type = NLS_INFINITE_LIGHT;
				}
				else if ( sTemp.same_no_case( "point" ) )
				{
					nls.m_Type = NLS_POINT_LIGHT;
				}
				else if ( sTemp.same_no_case( "spot" ) )
				{
					nls.m_Type = NLS_SPOT_LIGHT;
				}

				(*j).Get( "color", nls.m_Color );
				(*j).Get( "intensity", nls.m_Intensity );

				if ( nls.m_Type == NLS_INFINITE_LIGHT || nls.m_Type == NLS_SPOT_LIGHT )
				{
					if ( !(*j).Get( "direction", sTemp ) )
					{
						gperrorf( ("No direction specified for infinite or spot light in: %s", (*i).GetName()) );
					}

					stringtool::GetDelimitedValue( sTemp, ',', 0, nls.m_Direction.x );
					stringtool::GetDelimitedValue( sTemp, ',', 1, nls.m_Direction.y );
					stringtool::GetDelimitedValue( sTemp, ',', 2, nls.m_Direction.z );
				}

				if ( nls.m_Type == NLS_SPOT_LIGHT || nls.m_Type == NLS_POINT_LIGHT )
				{
					if ( (*j).Get( "position", sTemp ) )
					{
						stringtool::GetDelimitedValue( sTemp, ',', 0, nls.m_Position.x );
						stringtool::GetDelimitedValue( sTemp, ',', 1, nls.m_Position.y );
						stringtool::GetDelimitedValue( sTemp, ',', 2, nls.m_Position.z );
					}
				}

				if ( nls.m_Type == NLS_POINT_LIGHT )
				{
					(*j).Get( "inner_radius", nls.m_InnerRadius );
					(*j).Get( "outer_radius", nls.m_OuterRadius );
				}

				if ( nls.m_Type == NLS_SPOT_LIGHT )
				{
					(*j).Get( "inner_cone", nls.m_InnerCone );
					(*j).Get( "outer_cone", nls.m_OuterCone );
				}

				// Build our gui light
				GuiLight guiLight( nls );

				// Read in notification modifications
				FastFuelHandle hNotifications = (*j).GetChildNamed( "notifications" );
				if ( hNotifications.IsValid() )
				{
					FastFuelHandleColl hlNotifications;
					hNotifications.ListChildren( hlNotifications );
					FastFuelHandleColl::iterator iNotify;
					for ( iNotify = hlNotifications.begin(); iNotify != hlNotifications.end(); ++iNotify )
					{
						nema::LightSource notifySource = nls;

						gpstring sTemp;
						(*iNotify).Get( "lightsource_type", sTemp );
						if ( sTemp.same_no_case( "infinite" ) )
						{
							notifySource.m_Type = NLS_INFINITE_LIGHT;
						}
						else if ( sTemp.same_no_case( "point" ) )
						{
							notifySource.m_Type = NLS_POINT_LIGHT;
						}
						else if ( sTemp.same_no_case( "spot" ) )
						{
							notifySource.m_Type = NLS_SPOT_LIGHT;
						}

						(*iNotify).Get( "color", notifySource.m_Color );
						(*iNotify).Get( "intensity", notifySource.m_Intensity );

						if ( notifySource.m_Type == NLS_INFINITE_LIGHT || notifySource.m_Type == NLS_SPOT_LIGHT )
						{
							if ( (*iNotify).Get( "direction", sTemp ) )
							{
								stringtool::GetDelimitedValue( sTemp, ',', 0, notifySource.m_Direction.x );
								stringtool::GetDelimitedValue( sTemp, ',', 1, notifySource.m_Direction.y );
								stringtool::GetDelimitedValue( sTemp, ',', 2, notifySource.m_Direction.z );
							}
						}

						if ( nls.m_Type == NLS_SPOT_LIGHT || nls.m_Type == NLS_POINT_LIGHT )
						{
							if ( (*iNotify).Get( "position", sTemp ) )
							{
								stringtool::GetDelimitedValue( sTemp, ',', 0, notifySource.m_Position.x );
								stringtool::GetDelimitedValue( sTemp, ',', 1, notifySource.m_Position.y );
								stringtool::GetDelimitedValue( sTemp, ',', 2, notifySource.m_Position.z );
							}
						}

						if ( nls.m_Type == NLS_POINT_LIGHT )
						{
							(*iNotify).Get( "inner_radius", notifySource.m_InnerRadius );
							(*iNotify).Get( "outer_radius", notifySource.m_OuterRadius );
						}

						if ( nls.m_Type == NLS_SPOT_LIGHT )
						{
							(*iNotify).Get( "inner_cone", notifySource.m_InnerCone );
							(*iNotify).Get( "outer_cone", notifySource.m_OuterCone );
						}

						NotifyToLight ntl;
						ntl.bRamp			= false;
						ntl.startIntensity	= 0.0f;
						ntl.endIntensity	= 0.0f;
						ntl.rampDuration	= 0.0f;
						ntl.secondsElapsed	= 0.0f;
						ntl.bTriggered		= 0.0f;
						ntl.nls				= notifySource;

						(*iNotify).Get( "intensity", ntl.endIntensity );
						(*iNotify).Get( "start_intensity", ntl.startIntensity );
						(*iNotify).Get( "ramp", ntl.bRamp );
						(*iNotify).Get( "ramp_duration", ntl.rampDuration );

						guiLight.m_notifyLights.insert( NotifyToLightPair( (*iNotify).GetName(), ntl ) );
					}
				}

				lightScene.m_guiLights.push_back( guiLight );
			}

			m_guiLightMap.insert( GuiToLightPair( (*i).GetName(), lightScene ) );
		}
	}
}


void UIFrontend::UpdateGuiLights( float secondsElapsed )
{
	GuiToLightMap::iterator iGui;
	for ( iGui = m_guiLightMap.begin(); iGui != m_guiLightMap.end(); ++iGui )
	{
		NotifyAmbienceMap::iterator iNa;
		for ( iNa = (*iGui).second.m_notifyAmbience.begin(); iNa != (*iGui).second.m_notifyAmbience.end(); ++iNa )
		{
			if ( (*iNa).second.bTriggered && (*iNa).second.bRamp )
			{
				(*iNa).second.secondsElapsed += secondsElapsed;
				if ( (*iNa).second.secondsElapsed >= (*iNa).second.rampDuration )
				{
					(*iGui).second.m_ambience = (*iNa).second.finalAmbience ;
					(*iNa).second.bTriggered = false;
				}
				else
				{
					DWORD dwStep = (DWORD)( ((*iNa).second.finalAmbience - (*iNa).second.startAmbience) / (*iNa).second.rampDuration );

					float amount = (float)( ((float)dwStep * (float)(*iNa).second.secondsElapsed) / (float)( (*iNa).second.finalAmbience - (*iNa).second.startAmbience ) );

					if ( amount > 1.0f )
					{
						amount = 1.0f;
					}

					(*iGui).second.m_ambience = InterpolateColor( (*iNa).second.finalAmbience, (*iNa).second.startAmbience, amount );
				}
			}
		}

		GuiLightColl::iterator iLights;
		for ( iLights = (*iGui).second.m_guiLights.begin(); iLights != (*iGui).second.m_guiLights.end(); ++iLights )
		{
			NotifyToLightMap::iterator iNl;
			for ( iNl = (*iLights).m_notifyLights.begin(); iNl != (*iLights).m_notifyLights.end(); ++iNl )
			{
				if ( (*iNl).second.bTriggered && (*iNl).second.bRamp )
				{
					if ( (*iNl).second.secondsElapsed >= (*iNl).second.rampDuration )
					{
						(*iLights).m_nls.m_Intensity = (*iNl).second.endIntensity ;
						(*iNl).second.bTriggered = false;
					}
					else
					{
						float dwStep = ((*iNl).second.endIntensity - (*iNl).second.startIntensity) / (*iNl).second.rampDuration;
						(*iLights).m_nls.m_Intensity = dwStep * (*iNl).second.secondsElapsed;

						if ( (*iLights).m_nls.m_Intensity > 1.0f )
						{
							(*iLights).m_nls.m_Intensity = 1.0f;
						}
					}
					(*iNl).second.secondsElapsed += secondsElapsed;
				}

			}
		}
	}
}


void GuiLight::ReceiveNotification( gpstring sNotify )
{
	NotifyToLightMap::iterator i = m_notifyLights.find( sNotify );
	if ( i != m_notifyLights.end() )
	{
		if ( (*i).second.bRamp )
		{
			(*i).second.bTriggered = true;
			(*i).second.secondsElapsed = 0.0f;
			m_nls = (*i).second.nls;
			m_nls.m_Intensity = (*i).second.startIntensity;
		}
		else
		{
			m_nls = (*i).second.nls;
		}
	}
}


void GuiLightScene::ReceiveNotification( gpstring sNotify )
{
	NotifyAmbienceMap::iterator i = m_notifyAmbience.find( sNotify );
	if ( i != m_notifyAmbience.end() )
	{
		if ( (*i).second.bRamp )
		{
			(*i).second.bTriggered = true;
			(*i).second.secondsElapsed = 0.0f;
			m_ambience = (*i).second.startAmbience;
		}
		else
		{
			m_ambience = (*i).second.finalAmbience;
		}
	}
}

void MiscAnimSetupHelper(GoHandle& hgo, DWORD subanim)
{
	HAspect hasp = hgo->GetAspect()->GetAspectHandle();
	hasp->SetNextChore( CHORE_MISC, AS_PLAIN, subanim, 0 );
	hasp->ResetAnimationCompletionCallback();
	hasp->UpdateAnimationStateMachine( 0.0f );
}

void UIFrontend::GuiFlyin()
{
	AppModule::AutoFreezeTime freezeTime;

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  2 );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		MiscAnimSetupHelper(  hMenuBars ,  2 );
		hMenuBars->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::MenuBarsFlyinCompletionCb ) );
	}

	GoHandle hRightSide( m_guiRightSide );
	if ( hRightSide.IsValid() )
	{
		MiscAnimSetupHelper(  hRightSide ,  2 );
	}

	GoHandle hLeftSide( m_guiLeftSide );
	if ( hLeftSide.IsValid() )
	{
		MiscAnimSetupHelper(  hLeftSide ,  2 );
	}

	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  2 );
	}

	PlayFrontendSound( "frontend_menu_enter" );
}

void UIFrontend::GuiSinglePlayer()
{
	AppModule::AutoFreezeTime freezeTime;
	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  5 );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		MiscAnimSetupHelper(  hMenuBars ,  5 );
		hMenuBars->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::MenuBarsSinglePlayerCompletionCb ) );
	}

	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  5 );
	}
}


void UIFrontend::GuiStartGame()
{
	AppModule::AutoFreezeTime freezeTime;

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  8 );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		MiscAnimSetupHelper(  hMenuBars ,  8 );
		hMenuBars->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::MenuBarsStartNewGameCompletionCb ) );
	}

	GoHandle hHeroMenu( m_guiHeroMenu );
	if ( hHeroMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hHeroMenu ,  3 );
	}

	GoHandle hRightSide( m_guiRightSide );
	if ( hRightSide.IsValid() )
	{
		MiscAnimSetupHelper(  hRightSide ,  4 );
	}

	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  3 );
	}
}


void UIFrontend::GuiBackToMain()
{
	AppModule::AutoFreezeTime freezeTime;

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  7 );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		MiscAnimSetupHelper(  hMenuBars ,  6 );
		hMenuBars->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::MenuBarsFlyinCompletionCb ) );
	}

	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  2 );
	}
}


void UIFrontend::GuiBackToSinglePlayer()
{
	AppModule::AutoFreezeTime freezeTime;

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  6 );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		MiscAnimSetupHelper(  hMenuBars ,  7 );
		hMenuBars->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::MenuBarsSinglePlayerCompletionCb ) );
	}

	GoHandle hHeroMenu( m_guiHeroMenu );
	if ( hHeroMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hHeroMenu ,  4 );
	}

	GoHandle hRightSide( m_guiRightSide );
	if ( hRightSide.IsValid() )
	{
		MiscAnimSetupHelper(  hRightSide ,  3 );
	}


	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  4 );
	}
}


void UIFrontend::GuiBackToHeroSelect()
{
	AppModule::AutoFreezeTime freezeTime;

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  11 );
	}

	GoHandle hHeroMenu( m_guiHeroMenu );
	if ( hHeroMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hHeroMenu ,  3 );
	}

	GoHandle hRightSide( m_guiRightSide );
	if ( hRightSide.IsValid() )
	{
		MiscAnimSetupHelper(  hRightSide ,  4 );
	}


	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  7 );
	}

	GoHandle hLoadMap( m_guiLoadMap );
	if ( hLoadMap.IsValid() )
	{
		MiscAnimSetupHelper(  hLoadMap ,  1 );
		hLoadMap->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::LoadMapPreviousCompletionCb ) );
	}
}


void UIFrontend::GuiLoadMap()
{
	AppModule::AutoFreezeTime freezeTime;	

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  15 );
	}

	GoHandle hHeroMenu( m_guiHeroMenu );
	if ( hHeroMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hHeroMenu ,  4 );
	}

	GoHandle hRightSide( m_guiRightSide );
	if ( hRightSide.IsValid() )
	{
		MiscAnimSetupHelper(  hRightSide ,  3 );
	}


	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  6 );
	}

	GoHandle hLoadMap( m_guiLoadMap );
	if ( hLoadMap.IsValid() )
	{
		MiscAnimSetupHelper(  hLoadMap ,  2 );
		hLoadMap->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::LoadMapCompletionCb ) );
	}
}

void UIFrontend::GuiLoadGame()
{
	AppModule::AutoFreezeTime freezeTime;

	ConstructSaveList();

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  16 );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		MiscAnimSetupHelper(  hMenuBars ,  8 );
	}

	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  8 );
	}

	GoHandle hLoadMap( m_guiLoadMap );
	if ( hLoadMap.IsValid() )
	{
		MiscAnimSetupHelper(  hLoadMap ,  2 );
		hLoadMap->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::LoadGameCompletionCb ) );
	}
}


void UIFrontend::GuiBackToSPFromLG()
{
	AppModule::AutoFreezeTime freezeTime;

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  10 );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		MiscAnimSetupHelper(  hMenuBars ,  7 );
		hMenuBars->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::MenuBarsSinglePlayerCompletionCb ) );
	}

	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  9 );
	}

	GoHandle hLoadMap( m_guiLoadMap );
	if ( hLoadMap.IsValid() )
	{
		MiscAnimSetupHelper(  hLoadMap ,  3 );
	}
}


void UIFrontend::GuiMultiplayer()
{
	AppModule::AutoFreezeTime freezeTime;

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  12 );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		MiscAnimSetupHelper(  hMenuBars ,  9 );
		hMenuBars->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::MenuBarsMultiPlayerCompletionCb ) );
	}

	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  5 );
	}
}


void UIFrontend::GuiBackToMainFromMP()
{
	AppModule::AutoFreezeTime freezeTime;

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  14 );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		MiscAnimSetupHelper(  hMenuBars ,  10 );
		hMenuBars->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::MenuBarsMPToMainCompletionCb ) );
	}

	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  2 );
	}

	m_bProviderInit = false;
}


void UIFrontend::GuiProviderFromMP()
{
	AppModule::AutoFreezeTime freezeTime;

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  12 );
		hMainMenu->GetAspect()->GetAspectHandle()->UpdateAnimationStateMachine( 10.0f );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		MiscAnimSetupHelper(  hMenuBars ,  9 );
		hMenuBars->GetAspect()->GetAspectHandle()->UpdateAnimationStateMachine( 10.0f );
	}

	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  5 );
		hBackButton->GetAspect()->GetAspectHandle()->UpdateAnimationStateMachine( 10.0f );
	}

	GoHandle hRightSide( m_guiRightSide );
	if ( hRightSide.IsValid() )
	{
		MiscAnimSetupHelper(  hRightSide ,  2 );
		hRightSide->GetAspect()->GetAspectHandle()->UpdateAnimationStateMachine( 10.0f );
	}

	GoHandle hLeftSide( m_guiLeftSide );
	if ( hLeftSide.IsValid() )
	{
		MiscAnimSetupHelper(  hLeftSide ,  2 );
		hLeftSide->GetAspect()->GetAspectHandle()->UpdateAnimationStateMachine( 10.0f );
	}

	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:multiplayer_provider", true );
	MenuBarsMultiPlayerCompletionCb( 0 );
}


void UIFrontend::GuiLoadMapToDifficulty()
{
	AppModule::AutoFreezeTime freezeTime;

	PlayFrontendSound( "frontend_menu_roll" );

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  17 );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		MiscAnimSetupHelper(  hMenuBars ,  11 );
		hMenuBars->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::MapLoadToDifficultyCompletionCb ) );

		gpstring sTexture = "b_gui_fe_m_mn_3d_text-menubars3";
		hMenuBars->GetAspect()->SetCurrentAspectTexture( 11, sTexture.c_str() );
		hMenuBars->GetAspect()->SetCurrentAspectTexture( 13, sTexture.c_str() );
		hMenuBars->GetAspect()->SetCurrentAspectTexture( 15, sTexture.c_str() );
	}

	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  9 );
	}

	if ( gWorldState.GetCurrentState() == WS_SP_MAP_SELECT )
	{
		GoHandle hLoadMap( m_guiLoadMap );
		if ( hLoadMap.IsValid() )
		{
			MiscAnimSetupHelper(  hLoadMap ,  3 );
		}
	}
}


void UIFrontend::GuiDifficultyToLoadMap()
{
	AppModule::AutoFreezeTime freezeTime;

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  18 );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		MiscAnimSetupHelper(  hMenuBars ,  12 );
		hMenuBars->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::DifficultyToMapLoadCompletionCb ) );
	}

	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  8 );
	}

	if ( gWorldState.GetCurrentState() == WS_SP_MAP_SELECT || gWorldState.GetPreviousState() == WS_SP_MAP_SELECT )
	{
		GoHandle hLoadMap( m_guiLoadMap );
		if ( hLoadMap.IsValid() )
		{
			MiscAnimSetupHelper(  hLoadMap ,  2 );
		}
	}
}


void UIFrontend::GuiHeroSelectToDifficulty()
{
	AppModule::AutoFreezeTime freezeTime;

	PlayFrontendSound( "frontend_menu_roll" );

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  19 );
	}

	GoHandle hHeroMenu( m_guiHeroMenu );
	if ( hHeroMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hHeroMenu ,  4 );
	}

	GoHandle hRightSide( m_guiRightSide );
	if ( hRightSide.IsValid() )
	{
		MiscAnimSetupHelper(  hRightSide ,  3 );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		MiscAnimSetupHelper(  hMenuBars ,  11 );
		hMenuBars->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::MapLoadToDifficultyCompletionCb ) );

		gpstring sTexture = "b_gui_fe_m_mn_3d_text-menubars3";
		hMenuBars->GetAspect()->SetCurrentAspectTexture( 11, sTexture.c_str() );
		hMenuBars->GetAspect()->SetCurrentAspectTexture( 13, sTexture.c_str() );
		hMenuBars->GetAspect()->SetCurrentAspectTexture( 15, sTexture.c_str() );
	}

	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  4 );
	}
}


void UIFrontend::GuiDifficultyToHeroSelect()
{
	AppModule::AutoFreezeTime freezeTime;

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  20 );
	}

	GoHandle hHeroMenu( m_guiHeroMenu );
	if ( hHeroMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hHeroMenu ,  3 );
	}

	GoHandle hRightSide( m_guiRightSide );
	if ( hRightSide.IsValid() )
	{
		MiscAnimSetupHelper(  hRightSide ,  4 );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		MiscAnimSetupHelper(  hMenuBars ,  12 );
		hMenuBars->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::DifficultyToHeroSelectCompletionCb ) );
	}

	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  3 );
	}
}


void UIFrontend::GuiLogoIn()
{
	AppModule::AutoFreezeTime freezeTime;

	gUIShell.ShowInterface( "fade_screen" );

	m_logoSample = gSoundManager.PlayStream( "s_m_frontend_logo_flyin", GPGSound::STRM_MUSIC, false );

	GoHandle hLogo( m_guiLogo );
	if ( hLogo.IsValid() )
	{
		MiscAnimSetupHelper( hLogo, 0 );
		hLogo->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::LogoInCompletionCb ) );
	}

	PlayFrontendSound( "frontend_logo_flyin" );
}


void UIFrontend::GuiLogoOut()
{
	if ( !m_bLogoOut )
	{
		m_bLogoOut = true;
		AppModule::AutoFreezeTime freezeTime;

		GoHandle hLogo( m_guiLogo );
		if ( hLogo.IsValid() )
		{
			MiscAnimSetupHelper( hLogo, 1 );
			hLogo->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::LogoOutCompletionCb ) );
		}	

		gUIShell.HideInterface( "fade_screen" );

		PlayFrontendSound( "frontend_logo_flyout" );

		gSoundManager.StopStream( m_logoSample );
	}	
}


void UIFrontend::InitGui( bool bFromMulti )
{
	if( !Server::DoesSingletonExist() )
	{
		gWorld.InitServer( false );
	}

	if( m_bGuiInitialized )
	{
		return;
	}

	gWorldFx.SetAllowUndefinedNodeGUID( true );

	AppModule::AutoFreezeTime freezeTime;

	gUI.GetGameGUI().GetMessenger()->RegisterCallback( makeFunctor( *this, &UIFrontend::GameGUICallback ) );

	gUIShell.MarkInterfaceForActivation( "ui:interfaces:cursors", true );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:tool_tip", true );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:frontend_help", true );	
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:main_menu", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:single_player", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:load_game", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:character_select", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:map_chooser", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:select_name_dialog", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:multiplayer_provider", bFromMulti );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:difficulty_menu", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:about_dialog", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:continue_dialog", false );	
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:save_delete_confirm", false );	
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:fade_screen", false );	

	gUIShell.PlaceInterfaceOnTop( "options_game"     );
	gUIShell.PlaceInterfaceOnTop( "options_video"    );
	gUIShell.PlaceInterfaceOnTop( "options_audio"    );
	gUIShell.PlaceInterfaceOnTop( "options_input"    );
	gUIShell.PlaceInterfaceOnTop( "options_bindings" );
	gUIShell.PlaceInterfaceOnTop( "options_item_filters" );

	gUIShell.MarkInterfaceForActivation( "ui:interfaces:frontend:frontend_dialog", false );
	gUIShell.PlaceInterfaceOnTop( "frontend_dialog" );

	TextureColl::iterator i;
	for ( i = m_preloadTextures.begin(); i != m_preloadTextures.end(); ++i )
	{
		gUIShell.GetRenderer().DestroyTexture( *i );
	}
	m_preloadTextures.clear();


	UIWindow * pWindow = gUIShell.FindUIWindow( "listener" );
	if ( pWindow )
	{
		m_char_rect.x1 = pWindow->GetRect().left;
		m_char_rect.y1 = pWindow->GetRect().top;
		m_char_rect.x2 = pWindow->GetRect().right;
		m_char_rect.x2 = pWindow->GetRect().bottom;
	}

	FastFuelHandle hTextures( "ui:config:preload_textures:preload_textures" );
	if ( hTextures.IsValid() )
	{
		FastFuelFindHandle hFind( hTextures );
		if ( hFind.IsValid() && hFind.FindFirstValue( "*" ) )
		{
			gpstring sTexture;
			while ( hFind.GetNextValue( sTexture ) )
			{
				gpstring sFile;
				gNamingKey.BuildIMGLocation( sTexture, sFile );
				m_preloadTextures.push_back( gUIShell.GetRenderer().CreateTexture( sFile, false, 0, TEX_STATIC ) );
			}
		}
	}

	GoCloneReq backdropReq( "ui_backdrop" );
	backdropReq.m_OmniGo = true;
	m_guiBackdrop = gGoDb.CloneLocalGo( backdropReq );

	GoHandle hBackdrop( m_guiBackdrop );
	vector_3 bonePos;
	Quat	 boneRot;

	gGoDb.CommitAllRequests();

	if ( hBackdrop->GetAspect()->GetAspectHandle()->GetBoneOrientation( "farmboyloc", bonePos, boneRot ) )
	{
		m_vCharacterPos = bonePos;
	}

	if ( hBackdrop->GetAspect()->GetAspectHandle()->GetBoneOrientation( "faceboyloc", bonePos, boneRot ) )
	{
		m_vCharacterFacePos = bonePos;
	}

	GoCloneReq mainmenuReq( "ui_mainmenu" );
	mainmenuReq.m_OmniGo = true;
	m_guiMainMenu = gGoDb.CloneLocalGo( mainmenuReq );

	GoCloneReq heromenuReq( "ui_heromenu" );
	heromenuReq.m_OmniGo = true;
	m_guiHeroMenu = gGoDb.CloneLocalGo( heromenuReq );

	GoCloneReq leftsideReq( "ui_leftside" );
	leftsideReq.m_OmniGo = true;
	m_guiLeftSide = gGoDb.CloneLocalGo( leftsideReq );

	GoCloneReq menubarsReq( "ui_menubars" );
	menubarsReq.m_OmniGo = true;
	m_guiMenuBars = gGoDb.CloneLocalGo( menubarsReq );

	GoCloneReq rightsideReq( "ui_rightside" );
	rightsideReq.m_OmniGo = true;
	m_guiRightSide = gGoDb.CloneLocalGo( rightsideReq );

	GoCloneReq backbuttonReq( "ui_backbutton" );
	backbuttonReq.m_OmniGo = true;
	m_guiBackButton = gGoDb.CloneLocalGo( backbuttonReq );

	GoCloneReq loadmapReq( "ui_loadmap" );
	loadmapReq.m_OmniGo = true;
	m_guiLoadMap = gGoDb.CloneLocalGo( loadmapReq );

	GoCloneReq logoReq( "ui_logo" );
	logoReq.m_OmniGo = true;
	m_guiLogo = gGoDb.CloneLocalGo( logoReq );

	gGoDb.CommitAllRequests();

	GoHandle hMainMenu( m_guiMainMenu );
	if ( hMainMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hMainMenu ,  1 );
		//hRightPillar->GetAspect()->GetAspectHandle()->RegisterAnimationCompletionCallback( makeFunctor( *this, &UIFrontend::RightPillarAnimCallback ) );
	}

	GoHandle hHeroMenu( m_guiHeroMenu );
	if ( hHeroMenu.IsValid() )
	{
		MiscAnimSetupHelper(  hHeroMenu ,  1 );
	}

	GoHandle hMenuBars( m_guiMenuBars );
	if ( hMenuBars.IsValid() )
	{
		gpstring sTexture = "b_gui_fe_m_mn_3d_text-menubars2";
		hMenuBars->GetAspect()->SetCurrentAspectTexture( 11, sTexture.c_str() );
		hMenuBars->GetAspect()->SetCurrentAspectTexture( 13, sTexture.c_str() );
		hMenuBars->GetAspect()->SetCurrentAspectTexture( 15, sTexture.c_str() );
		MiscAnimSetupHelper(  hMenuBars ,  1 );
	}

	GoHandle hRightSide( m_guiRightSide );
	if ( hRightSide.IsValid() )
	{
		MiscAnimSetupHelper(  hRightSide ,  1 );
	}

	GoHandle hLeftSide( m_guiLeftSide );
	if ( hLeftSide.IsValid() )
	{
		MiscAnimSetupHelper(  hLeftSide ,  1 );
	}

	GoHandle hBackButton( m_guiBackButton );
	if ( hBackButton.IsValid() )
	{
		MiscAnimSetupHelper(  hBackButton ,  1 );
	}

	if ( m_pCharacterSelect )
	{
		Delete( m_pCharacterSelect );
	}

	if ( UIMultiplayer::DoesSingletonExist() )
	{
		gUIMultiplayer.DeinitCharacterSelect();
	}
	m_pCharacterSelect = new UICharacterSelect;
	m_pCharacterSelect->AddAllCharacters();

	gGoDb.CommitAllRequests();

	m_bProviderInit = false;

	FastFuelHandle hSaveColors( "config:global_settings:save_game_settings" );
	if ( hSaveColors.IsValid() )
	{
		hSaveColors.Get( "quick_save_color", m_dwQuickColor );
		hSaveColors.Get( "auto_save_color", m_dwAutoColor );
	}

	if ( gWorldState.GetPreviousState() != WS_LOGO )
	{
		InitGuiLights();
	}

	m_bGuiInitialized = true;	
}


void UIFrontend::MenuBarsFlyinCompletionCb( unsigned int /* param */ )
{
	gUI.GetGameGUI().ShowInterface( "main_menu" );
	UITextBox * pText = (UITextBox *)gUIShell.FindUIWindow( "legal_info" );
	if ( pText && !m_bVersionSet )
	{
		m_bVersionSet = true;
		gpwstring sVer;
		sVer.assignf( gpwtranslate( $MSG$ "Version - %S" ), SysInfo::GetExeFileVersionText( gpversion::MODE_AUTO_SHORT ).c_str() );
		pText->SetLineText( pText->GetNumElements(), sVer );
		pText->SetVisible( true );

		UITextBox * pBox = (UITextBox *)gUIShell.FindUIWindow( "frontend_help", "frontend_help" );
		if ( pBox )
		{
			pBox->SetText( L"" );
		}
	}
}

void UIFrontend::MenuBarsSinglePlayerCompletionCb( unsigned int /* param */ )
{
	m_bDrawCharSelect = false;
	gUI.GetGameGUI().ShowInterface( "single_player" );
}


void UIFrontend::MenuBarsStartNewGameCompletionCb( unsigned int /* param */ )
{
	gUI.GetGameGUI().ShowInterface( "character_select" );
}

void UIFrontend::LoadMapCompletionCb( unsigned int /* param */ )
{
	gUI.GetGameGUI().ShowInterface( "map_chooser" );
	if ( !gTankMgr.HasAnyMaps() )
	{
		ShowDialog( gpwtranslate( $MSG$ "There are no maps currently available.  Please check your installation." ) );
	}
}

void UIFrontend::LoadMapPreviousCompletionCb( unsigned int /* param */ )
{
	gUI.GetGameGUI().ShowInterface( "character_select" );
}

void UIFrontend::LoadGameCompletionCb( unsigned int /* param */ )
{
	gUI.GetGameGUI().ShowInterface( "load_game" );
	
	LoadGameSelect();
}


void UIFrontend::MenuBarsMultiPlayerCompletionCb( unsigned int /* param */ )
{
	gUI.GetGameGUI().ShowInterface( "multiplayer_provider" );
}


void UIFrontend::MenuBarsMPToMainCompletionCb( unsigned int /* param */ )
{
	gUI.GetGameGUI().ShowInterface( "main_menu" );
	UITextBox * pText = (UITextBox *)gUIShell.FindUIWindow( "legal_info" );
	if ( pText && !m_bVersionSet )
	{
		m_bVersionSet = true;	
		gpwstring sVer;
		sVer.assignf( gpwtranslate( $MSG$ "Version - %S" ), SysInfo::GetExeFileVersionText( gpversion::MODE_AUTO_SHORT ).c_str() );
		pText->SetLineText( 2, sVer );
	}
}


void UIFrontend::MapLoadToDifficultyCompletionCb( unsigned int /* param */ )
{
	m_bDrawCharSelect = false;
	gUIShell.ShowInterface( "difficulty_menu" );
}


void UIFrontend::DifficultyToMapLoadCompletionCb( unsigned int /* param */ )
{
	gpstring sTexture = "b_gui_fe_m_mn_3d_text-menubars2";
	GoHandle hMenuBars( m_guiMenuBars );
	hMenuBars->GetAspect()->SetCurrentAspectTexture( 11, sTexture.c_str() );
	hMenuBars->GetAspect()->SetCurrentAspectTexture( 13, sTexture.c_str() );
	hMenuBars->GetAspect()->SetCurrentAspectTexture( 15, sTexture.c_str() );
	gUIShell.ShowInterface( "map_chooser" );
}


void UIFrontend::DifficultyToHeroSelectCompletionCb( unsigned int /* param */ )
{
	gpstring sTexture = "b_gui_fe_m_mn_3d_text-menubars2";
	GoHandle hMenuBars( m_guiMenuBars );
	hMenuBars->GetAspect()->SetCurrentAspectTexture( 11, sTexture.c_str() );
	hMenuBars->GetAspect()->SetCurrentAspectTexture( 13, sTexture.c_str() );
	hMenuBars->GetAspect()->SetCurrentAspectTexture( 15, sTexture.c_str() );
	gUIShell.ShowInterface( "character_select" );
}


void UIFrontend::LogoOutCompletionCb( unsigned int /* param */ )
{
	gWorldStateRequest( WS_MAIN_MENU );	
}


void UIFrontend::LogoInCompletionCb( unsigned int /* param */ )
{
	UIWindow * pWindow = gUIShell.FindUIWindow( "fade_panel", "fade_screen" );
	if ( pWindow )
	{
		pWindow->ReceiveMessage( UIMessage( MSG_STARTANIMATION ) );
	}
}


void UIFrontend::InitTexSwapMap()
{
	FastFuelHandle hMap( "ui:config:art_mapping" );
	if ( hMap.IsValid() )
	{
		FastFuelHandleColl hlArtMapping;
		hMap.ListChildren( hlArtMapping );
		FastFuelHandleColl::iterator i;
		for ( i = hlArtMapping.begin(); i != hlArtMapping.end(); ++i )
		{
			GuiTexEventMap eventMap;

			FastFuelHandle hDown = (*i).GetChildNamed( "mousedown" );
			if ( hDown.IsValid() )
			{
				GuiTexIndexMap indexMap;
				FastFuelFindHandle hFind( hDown );
				if ( hFind.IsValid() && hFind.FindFirstKeyAndValue() )
				{
					gpstring sKey;
					gpstring sValue;
					while ( hFind.GetNextKeyAndValue( sKey, sValue ) )
					{
						int index = 0;
						stringtool::Get( sKey, index );
						indexMap.insert( GuiTexIndexPair( index, sValue ) );
					}
				}

				eventMap.insert( GuiTexEventPair( "mousedown", indexMap ) );
			}

			FastFuelHandle hOver = (*i).GetChildNamed( "mouseover" );
			if ( hOver.IsValid() )
			{
				GuiTexIndexMap indexMap;
				FastFuelFindHandle hFind( hOver );
				if ( hFind.FindFirstKeyAndValue() )
				{
					gpstring sKey;
					gpstring sValue;
					while ( hFind.GetNextKeyAndValue( sKey, sValue ) )
					{
						int index = 0;
						stringtool::Get( sKey, index );
						indexMap.insert( GuiTexIndexPair( index, sValue ) );
					}
				}

				eventMap.insert( GuiTexEventPair( "mouseover", indexMap ) );
			}

			FastFuelHandle hOut = (*i).GetChildNamed( "mouseout" );
			if ( hOut.IsValid() )
			{
				GuiTexIndexMap indexMap;
				FastFuelFindHandle hFind( hOut );
				if ( hFind.FindFirstKeyAndValue() )
				{
					gpstring sKey;
					gpstring sValue;
					while ( hFind.GetNextKeyAndValue( sKey, sValue ) )
					{
						int index = 0;
						stringtool::Get( sKey, index );
						indexMap.insert( GuiTexIndexPair( index, sValue ) );
					}
				}

				eventMap.insert( GuiTexEventPair( "mouseout", indexMap ) );
			}

			m_guiTexSwapMap.insert( GuiTexSwapPair( (*i).GetName(), eventMap ) );
		}
	}
}


void UIFrontend::GuiTextureSwap( gpstring sWindow, int windowIndex, gpstring sEvent )
{
	GuiTexSwapMap::iterator iFound = m_guiTexSwapMap.find( sWindow );
	if ( iFound != m_guiTexSwapMap.end() )
	{
		GuiTexEventMap::iterator iEventFound = (*iFound).second.find( sEvent );
		if ( iEventFound != (*iFound).second.end() )
		{
			GuiTexIndexMap::iterator iIndex;
			for ( iIndex = (*iEventFound).second.begin(); iIndex != (*iEventFound).second.end(); ++iIndex )
			{
				Goid object = GOID_INVALID;
				switch( windowIndex )
				{
				case 1:
					object = m_guiBackButton;
					break;
				case 2:
					object = m_guiMenuBars;
					break;
				case 3:
					object = m_guiHeroMenu;
					break;
				}

				GoHandle hObject( object );
				if ( hObject.IsValid() )
				{
					gpstring sTexture = "b_gui_fe_m_mn_3d_";
					sTexture += (*iIndex).second;
					hObject->GetAspect()->SetCurrentAspectTexture( (*iIndex).first, sTexture.c_str() );
				}
			}
		}
	}
}


void UIFrontend::PlayFrontendSound( gpstring const & sEvent )
{
	GoHandle hGui( m_guiBackdrop );
	if ( hGui.IsValid() )
	{
		hGui->PlayVoiceSound( sEvent, false );
	}
}


void UIFrontend::ShowDialog( gpwstring const & message )
{
	UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "fe_dialog_text_box", "frontend_dialog" );
	if ( pTextBox )
	{		
		pTextBox->SetText( message );
	}

	gUIShell.ShowInterface( "frontend_dialog" );
}


void UIFrontend::StartFrontendMusic()
{
	if ( m_FrontendMusic != GPGSound::INVALID_SOUND_ID && gSoundManager.IsStreamPlaying( m_FrontendMusic ) )
	{
		return;	
	}

	FastFuelHandle hFrontendMusic( "ui:config:frontend_music:frontend_music" );
	if ( hFrontendMusic )
	{
		gpstring sSample;
		hFrontendMusic.Get( "sample", sSample );		
		
		m_FrontendMusic = gSoundManager.PlayStream( sSample, GPGSound::STRM_MUSIC, true );
	}
}


void UIFrontend::StopFrontendMusic()
{
	gSoundManager.StopStream( m_FrontendMusic );
	m_FrontendMusic = GPGSound::INVALID_SOUND_ID;
	gSoundManager.Update( 0 );
}