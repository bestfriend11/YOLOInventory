/**************************************************************************

Filename		: UIMultiplayer.cpp
Class			: UIMultiplayer
Description		: The class for the multiplayer interface.
Creation Date	: 06/14/00

**************************************************************************/


#include "Precomp_Game.h"
#include "UIMultiPlayer.h"

#include "AppModule.h"
#include "Config.h"
#include "Fuel.h"
#include "filesysutils.h"
#include "InputBinder.h"
#include "NetFuBi.h"
#include "Player.h"
#include "Server.h"
#include "Rules.h"
#include "UI_EditBox.h"
#include "UI_Messenger.h"
#include "UI_Mouse_Listener.h"
#include "UICharacterSelect.h"
#include "World.h"
#include "WorldState.h"
#include "WorldOptions.h"
#include "worldtime.h"
#include "UI_DialogBox.h"
#include "UI_ListReport.h"
#include "UI_ChatBox.h"
#include "UI_Button.h"
#include "UI_Emotes.h"
#include "UI_Line.h"
#include "UI_PopupMenu.h"
#include "UI_TextBox.h"
#include "UI_ComboBox.h"
#include "UI_Tab.h"
#include "UI_Textureman.h"
#include "victory.h"
#include "UIGameConsole.h"
#include "UIFrontend.h"
#include "UIZoneMatch.h"
#include "GameAuditor.h"
#include "WorldMap.h"
#include "TattooGame.h"
#include "TankMgr.h"
#include "GoActor.h"
#include "GoAspect.h"
#include "GoBody.h"
#include "GoDb.h"
#include "RapiImage.h"
#include "stringtool.h"
#include "LocHelp.h"
#include "WorldMessage.h"
#include "gpglobal.h"
#include "gps_manager.h"
#include "GamePersist.h"


///////////////////////////////////////////////////////////////
//	UI helpers

#define UIFIND( name, type )		UI##type * p##type = (UI##type *)gUIShell.FindUIWindow( gpstringf name ); if ( p##type )
#define UIFINDI( name, type, ui )	UI##type * p##type = (UI##type *)gUIShell.FindUIWindow( gpstringf name , ui ); if ( p##type )

#define SETVISIBLE( name, flag )	{UIFIND( name, Window ){ pWindow->SetVisible( flag ); }}
#define SETENABLED( name, flag )	{UIFIND( name, Window ){ pWindow->SetEnabled( flag ); }}
#define SETALPHAENABLED( name, flag )	{UIFIND( name, Window ){ pWindow->SetEnabled( flag ); pWindow->SetAlpha( flag ? 1.0f : 0.6f ); }}

#define SHOW( name )				SETVISIBLE( name, true )
#define HIDE( name )				SETVISIBLE( name, false )
#define ENABLE( name )				SETENABLED( name, true )
#define DISABLE( name )				SETENABLED( name, false )

#define SHOWUI( name )				gUIShell.ShowInterface( name )
#define HIDEUI( name )				gUIShell.HideInterface( name )

#define VALIDUI( ui )				if ( !ui ){  gperror( "Attempted to find an invalid window." );  return;  }


///////////////////////////////////////////////////////////////
//	Popup menu commands

enum eMenuList
{
	MENU_SEPERATOR,

	MENU_PLAYER_EJECT,
	MENU_PLAYER_BAN,
};


#define MENU_COMMAND( menu, command )		menu->AddElement( m_UIMMultiplayerMenuList[(int)command], command )


///////////////////////////////////////////////////////////////
//	Game Settings

enum eGameSettings
{
	GS_IP_ADDRESS,
	GS_PASSWORD,
	GS_MAP,
	GS_WORLD,
	GS_PLAYER_LIMIT,
	GS_DIFFICULTY,
	GS_TIME_LIMIT,
	GS_DROP_ON_DEATH,
	GS_TEAM_PLAY,
	GS_PVP,
	GS_SELECT_LOCATIONS,
	GS_ALLOW_NEW_CHARACTERS,
	GS_ALLOW_PAUSING,
	GS_JIP,
};

#define HOST_CHANGE_SETTING_MSG_COLOR 0xFF00FF00
#define STAGING_AREA_STATUS_MSG_COLOR 0xFF00FF00

#define SETTING_FMT		L"%s:  %s"


UIMultiplayer::UIMultiplayer()
	: UILayer()
	, m_bIsVisible					( false	)
	, m_bIsInitialized				( false )
	, m_bIsActivated				( false )
	, m_OnNextUpdate				( NU_NONE )
	, m_pCharacterSelect			( NULL )
	, m_bRequestedLeaveGame			( false )
	, m_CurrentCharacterSlot		( 0 )
	, m_ActivePlayerSlot			( 1 )
	, m_SetSelectedCharacter		( 0 )
	, m_FindSessionTimeout			( 0.0 )
	, m_SessionTimeout				( 10.0 )
	, m_LanLastUpdateTime			( 0.0 )
	, m_JoinSessionLocalId			( 0 )
	, m_bShowTeams					( true )
	, m_SelectedCharacter			( -1 )
	, m_fEatEnter					( false )
	, m_bPlayersDirty				( false )
	, m_bPlayersPanelVisible		( false )
	, m_currentPlayers				( 0 )
	, m_MultiplayerDialog			( MP_DIALOG_INVALID )
	, m_MultiplayerEditDialog		( MP_EDIT_DIALOG_INVALID )
	, m_TimeToShowPendingMessage	( 0.0 )
	, m_bShowDialog					( false )
	, m_bActivateEndGameScreen		( false )
{
	m_pInputBinder = new InputBinder( "multiplayer" );
	gAppModule.RegisterBinder( m_pInputBinder, 200 );

	m_pUIZoneMatch = new UIZoneMatch;	

	m_PlayerColors.resize( Server::MAX_PLAYERS );
	FastFuelHandle hPlayerColors( "ui:config:multiplayer:player_colors" );
	if ( hPlayerColors.IsValid() )
	{		
		FastFuelFindHandle findColors( hPlayerColors );
		if ( findColors.IsValid() && findColors.FindFirstKeyAndValue() )
		{
			DWORD color;
			gpstring key, value;
			int player = 0;
			while ( findColors.GetNextKeyAndValue( key, value ) )
			{
				stringtool::Get( value, color );
				m_PlayerColors[ player++ ] = color;
				if ( player == Server::MAX_PLAYERS )
				{
					break;
				}
			}
		}
	}
	
	FastFuelHandle hZoneMatchSettings( "ui:config:multiplayer:zonematch" );
	if ( hZoneMatchSettings.IsValid() )
	{
		FastFuelHandle hGames = hZoneMatchSettings.GetChildNamed( "games" );
		if ( hGames.IsValid() )
		{
			hGames.GetDouble("m_SessionTimeout",m_SessionTimeout);

			// make sure we have a good value
			if( m_SessionTimeout <= 1.0 )
			{
				gperrorf( ("Bad value: %lf for ui:config:multiplayer:zonematch:games:m_sessionTimeout ", m_SessionTimeout) );
				m_SessionTimeout = 10.0;
			}
		}
	}	
}


UIMultiplayer::~UIMultiplayer()
{
	Delete( m_pCharacterSelect );
	Delete( m_pInputBinder );
	Delete( m_pCharacterSelect );
	Delete( m_pUIZoneMatch );

	for ( int i = 1; i <= STAGING_PLAYER_SLOTS; ++i )
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_sign", i ), "staging_area_players" );
		if ( pWindow )
		{
			pWindow->SetTextureIndex( 0 );
		}
	}

	for ( i = 1; i <= STAGING_CHARACTER_SLOTS; ++i )
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "character%d_portrait", i ), "staging_area_select_character" );
		if ( pWindow )
		{
			pWindow->SetTextureIndex( 0 );
		}
	}

	for ( i = 1; i <= STAGING_CHARACTER_SLOTS; ++i )
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "player%dteam", i ), "end_game_summary" );
		if ( pWindow )
		{
			pWindow->SetTextureIndex( 0 );
		}
	}	

	m_MpCharacters.clear();
	{for ( std::vector<DWORD>::iterator i = m_CharacterPortraits.begin(); i != m_CharacterPortraits.end(); ++i )
	{
		gDefaultRapi.DestroyTexture( *i );
	}}
	m_CharacterPortraits.clear();
}


void UIMultiplayer::SetIsVisible( bool flag )
{ 
	m_bIsVisible = flag;
	m_pInputBinder->SetIsActive( flag );
	
	if ( !m_bIsVisible )
	{
		HideInterfaces();
	}
}


InputBinder & UIMultiplayer::GetInputBinder()
{
	gpassert( m_pInputBinder );
	return( *m_pInputBinder );
}


void UIMultiplayer::GameGUICallback( gpstring const & message, UIWindow & ui_window )
{
	if ( !IsVisible() )
	{
		return;
	}

	if ( message.same_no_case( "show_multiplayer_main" ) )
	{
		ShowMultiplayerMain();
	}
	else if ( message.same_no_case( "internet_choose_ip" ) )
	{
		UIListbox *lb = (UIListbox *)gUIShell.FindUIWindow( "recent_ip_listbox" );
		if ( lb )
		{
			UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "ip_edit_box" );
			if ( eb )
			{
				gpwstring ipAddress = lb->GetSelectedText();

				UINT32 dash = ipAddress.find_last_of( L"-" );
				if ( dash != gpwstring::npos )
				{
					ipAddress = ipAddress.mid( dash + 2, ipAddress.size() - dash - 2 );
				}

				eb->SetText( ipAddress );
			}
		}
	}
	else if ( message.same_no_case( "internet_remove_ip" ) )
	{
		UIListbox *lb = (UIListbox *)gUIShell.FindUIWindow( "recent_ip_listbox" );
		if ( lb )
		{
			gpwstring listAddress = lb->GetSelectedText();
			if ( listAddress.empty() )
			{
				return;
			}

			lb->RemoveElement( lb->GetSelectedTag() );

			for ( IpColl::iterator i = m_IpHistory.begin(); i != m_IpHistory.end(); ++i )
			{
				gpwstring thisAddress;

				if ( !i->second.empty() )
				{
					thisAddress.assignf( L"%s - %s", i->second.c_str(), ToUnicode( i->first ).c_str() );
				}
				else
				{
					thisAddress = ToUnicode( i->first );
				}

				if ( thisAddress.same_no_case( listAddress ) )
				{
					m_IpHistory.erase( i );
					break;
				}
			}

			SaveIpAddressHistory();
		}
	}
	else if ( message.same_no_case( "internet_connect" ) )
	{
		if ( GetNextUpdate() == NU_JOIN_GAME )
		{
			return;
		}

		// BUG 12205/6 we want to make sure players have their safe disks when we are going to join a game
		// this way we don't have people joining games and holding everyone up while they are looking for disks!
		if ( !gUIFrontend.ShowCDCheckDialog() )
		{
			return;
		}

		gpwstring ipAddress;
		UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "ip_edit_box" );
		if ( eb )
		{
			ipAddress = eb->GetText();

			// check that the IP was entered in a valid format
			bool validIp = true;
			UINT pos = 0, endPos = ipAddress.size() - 1, numCount = 0, dotCount = 0;
			for ( ; pos <= endPos; ++pos )
			{
				if ( ipAddress[ pos ] != '.' )
				{
					++numCount;
					if ( numCount > 3 )
					{
						validIp = false;
						break;
					}
				}
				else
				{
					if ( numCount > 0 )
					{
						++dotCount;
						numCount = 0;
						if ( dotCount > 3 )
						{
							validIp = false;
							break;
						}
					}
					else
					{
						validIp = false;
						break;
					}

					if ( pos == endPos )
					{
						validIp = false;
					}
				}
			}

			if ( !validIp )
			{
				ShowMessageBox( gpwtranslate( $MSG$ "Enter a valid IP address." ) );
				
				eb->GiveFocus();

				eb = (UIEditBox *)gUIShell.FindUIWindow( "internet_player_name_edit_box" );
				if ( eb )
				{
					eb->SetAllowInput( false );
				}
				return;
			}
		}

		eb = (UIEditBox *)gUIShell.FindUIWindow( "internet_player_name_edit_box" );
		if ( eb )
		{
			m_PlayerName = eb->GetText();
			gUIFrontend.IsNameValid( m_PlayerName );
		}
		if ( m_PlayerName.empty() )
		{
			ShowMessageBox( gpwtranslate( $MSG$ "Enter a player name." ), MP_DIALOG_ENTER_PLAYER_NAME );
			if ( eb )
			{
				eb->GiveFocus();
			}
			return;
		}

		if ( !JoinGame( ipAddress ) )
		{
			return;
		}
	}
	else if ( message.same_no_case( "internet_host" ) )
	{
		// BUG 12205/6 we want to make sure players have their safe disks when we are going to start a game
		// this way we don't have people starting games that they won't be able to actually start....
		if ( !gUIFrontend.ShowCDCheckDialog() )
		{
			return;
		}

		UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "internet_player_name_edit_box" );
		if ( eb )
		{
			m_PlayerName = eb->GetText();
		}
		if ( m_PlayerName.empty() )
		{
			ShowMessageBox( gpwtranslate( $MSG$ "Enter a player name." ), MP_DIALOG_ENTER_PLAYER_NAME );
			if ( eb )
			{
				eb->GiveFocus();
			}
			return;
		}

		ShowHostGameInterface();
	}
	else if ( message.same_no_case( "join_lan_game" ) )
	{
		if ( GetNextUpdate() == NU_JOIN_GAME )
		{
			return;
		}

		// BUG 12205/6 we want to make sure players have their safe disks when we are going to join a game
		// this way we don't have people joining games and holding everyone up while they are looking for disks!
		if ( !gUIFrontend.ShowCDCheckDialog() )
		{
			return;
		}

		UIListReport *lr = (UIListReport *)gUIShell.FindUIWindow( "lan_games_list" );
		if ( !lr || lr->GetSelectedElementText().empty() )
		{
			return;
		}
		
		NetPipeSessionHandle session = gNetPipe.GetSessionByLocalId( lr->GetSelectedElementTag() );
		if( !session )
		{
			return;
		}

		if( !session->IsExecutableCompatible() )
		{
			gpwstring info( gpwtranslate( $MSG$ "Unable to join. The selected game is an incompatible version." ) );

			bool bUseLargeBox = false;
#if !GP_RETAIL

			bUseLargeBox = true;
			gpstring incompat( "\n=-=-=-=-=-=-=-=-= Debug Info Follows =-=-=-=-=-=-=-=-=\n" );
			NetPipeSession::Info localInfo;
			GetIncompatabilityString( session->GetInfo(), localInfo, incompat );
			info.append( ToUnicode( incompat ) );
#endif			
			if ( bUseLargeBox )
			{
				ShowLargeMessageBox( info );
			}
			else
			{
				ShowMessageBox( info );
			}

			return;
		}

		TankManifest manifest;
		gTankMgr.GetTankManifest( manifest );
		bool usingTankFiles = false;
		for( TankManifest::iterator i = manifest.begin(); i != manifest.end(); ++i )
		{
			if( (*i).m_TankType != TANK_NONE )
			{
				usingTankFiles = true;
				break;
			}
		}

		if( !usingTankFiles )
		{
			// if we're not running on tanks, don't attempt to connect with session of incompatible content
			// however, if you're running WITH tanks, attempt to connect only to be denied by server with info as to
			// which tanks are needed...
			if ( !session->IsContentCompatible() )
			{
				gpwstring info( gpwtranslate( $MSG$ "Unable to join. The selected game has incompatible content." ) );

				bool bUseLargeBox = false;
#if !GP_RETAIL
				gpstring incompat( "\n=-=-=-=-=-=-=-=-= Debug Info Follows =-=-=-=-=-=-=-=-=\n" );
				NetPipeSession::Info localInfo;
				GetIncompatabilityString( session->GetInfo(), localInfo, incompat );
				info.append( ToUnicode( incompat ) );
				bUseLargeBox = true;
#endif				
				if ( bUseLargeBox )
				{
					ShowLargeMessageBox( info, MP_DIALOG_OK );
				}
				else
				{
					ShowMessageBox( info );
				}
				return;
			}
		}

		UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "lan_player_name_edit_box" );
		if ( eb )
		{
			m_PlayerName = eb->GetText();
			gUIFrontend.IsNameValid( m_PlayerName );
		}

		if ( m_PlayerName.empty() )
		{
			ShowMessageBox( gpwtranslate( $MSG$ "Enter a player name." ), MP_DIALOG_ENTER_PLAYER_NAME );
			return;
		}

		if ( session->GetHasPassword() )
		{
			gUIShell.ShowInterface( "enter_password" );

			UIEditBox *pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "enter_password_edit_box" );
			if ( pEditBox )
			{
				pEditBox->SetText( L"" );
			}

			m_JoinSessionLocalId = session->GetLocalId();
		}
		else
		{
			Connect( session, L"" );
		}
	}
	else if ( message.same_no_case( "host_lan_game" ) )
	{

		// BUG 12205/6 we want to make sure players have their safe disks when we are going to start a game
		// this way we don't have people starting games that they won't be able to actually start....
		if ( !gUIFrontend.ShowCDCheckDialog() )
		{
			return;
		}

		UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "lan_player_name_edit_box" );
		if ( eb )
		{
			m_PlayerName = eb->GetText();
			gUIFrontend.IsNameValid( m_PlayerName );
		}
		if ( m_PlayerName.empty() )
		{
			ShowMessageBox( gpwtranslate( $MSG$ "Enter a player name." ), MP_DIALOG_ENTER_PLAYER_NAME );
			return;
		}

		ShowHostGameInterface();
	}
	else if ( message.same_no_case( "host_game_check_password" ) )
	{
		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "game_password_edit_box", "host_game" );
		if ( editBox )
		{
			editBox->SetEnabled( true );
			editBox->SetText( L"" );
			editBox->SetAllowInput( true );
			editBox = (UIEditBox *)gUIShell.FindUIWindow( "game_name_edit_box", "host_game" );
			if ( editBox )
			{
				editBox->SetAllowInput( false );
			}
		}
	}
	else if ( message.same_no_case( "host_game_uncheck_password" ) )
	{
		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "game_password_edit_box", "host_game" );
		if ( editBox )
		{
			editBox->SetAllowInput( false );
			editBox->SetEnabled( false );
			editBox->SetText( L"" );
		}
	}
	else if ( message.same_no_case( "multiplayer_host_game" ) )
	{
		// Create the game session
		gpwstring sessionName;
		gpwstring sessionPword;

		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "game_name_edit_box", "host_game" );
		if ( editBox )
		{
			sessionName = editBox->GetText();
			if ( !gUIFrontend.IsNameValid( sessionName ) )
			{
				ShowMessageBox( gpwtranslate( $MSG$ "Please enter a valid game name." ) );
				return;
			}
		}

		editBox = (UIEditBox *)gUIShell.FindUIWindow( "game_password_edit_box", "host_game" );
		if ( editBox )
		{
			sessionPword = editBox->GetText();
		}
	
		// remove multiple spaces in name
		for ( UINT i = 1; i < sessionName.size(); )
		{
			if ((sessionName[i] == L' ') && (sessionName[i - 1] == L' '))
			{
				sessionName = sessionName.left( i ) + sessionName.right( sessionName.size() - i - 1 );
			}
			else
			{
				++i;
			}
		}

		if ( sessionName.empty() )
		{
			return;
		}

		UICheckbox * pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_password_game", "host_game" );
		if ( pCheckbox && pCheckbox->GetState() && sessionPword.empty() )
		{
			ShowMessageBox( gpwtranslate( $MSG$ "Enter a password." ) );
			return;
		}

		gUIShell.HideInterface( "host_game" );

		if ( (GetMultiplayerMode() == MM_GUN) )
		{
			if ( !gUIZoneMatch.HostGame( sessionName ) )
			{
				return;
			}

			gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_PASSWORD_PROTECTED, !sessionPword.empty() );
		}

		// before we host, must tell netpipe what resources are required

		SetNetPipeContentResources();

		// host

		if ( !gNetPipe.Host( sessionName, sessionPword, Server::MAX_PLAYERS ) )
		{
			gpassertm( 0, "Could not create game session.\n" );

			if ( GetMultiplayerMode() == MM_GUN )
			{
				gUIZoneMatch.OnExitStagingArea();
			}
			else
			{
				ShowMultiplayerMain();
        		ShowMessageBox( gpwtranslate( $MSG$ "Can't connect. Possible problem negotiating with the proxy. Please see your network administrator." ) );
			}
			return;
		}

		gpwstring defaultGameName;
		defaultGameName.assignf( gpwtranslate( $MSG$ "%s's Game" ), m_PlayerName.c_str() );
		if ( defaultGameName.size() > 20 )
		{
			defaultGameName = defaultGameName.left( 20 );
		}

		if ( sessionName.same_no_case( defaultGameName ) )
		{
			sessionName.clear();
		}

		FuelHandle prefs = gTattooGame.GetPrefsConfigFuel( true );
		if ( prefs.IsValid() )
		{
			FuelHandle hSettings = prefs->GetChildBlock( "multiplayer", true );
			if ( hSettings.IsValid() )
			{
				hSettings->Set( "host_game_name", sessionName );
			}
			prefs->GetDB()->SaveChanges();
		}

		gpstring savePassword;
		TeaEncrypt( savePassword, sessionPword );
		gConfig.Set( "multiplayer\\gamepassword", savePassword );

		// Enter Staging Area
		gWorldStateRequest( WS_MP_STAGING_AREA_SERVER );
		gWorldState.Update();
	}
	else if ( message.same_no_case( "cancel_host_game" ) )
	{
		gUIShell.HideInterface( "host_game" );
	}
	else if ( message.same_no_case( "enter_password_ok" ) )
	{
		gpwstring passWord;

		UIEditBox *pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "enter_password_edit_box" );
		if ( pEditBox )
		{
			passWord = pEditBox->GetText();
		}
		
		if ( passWord.empty() )
		{
			return;
		}

		NetPipeSessionHandle session = gNetPipe.GetSessionByLocalId( m_JoinSessionLocalId );
		if( session )
		{
			Connect( session, passWord );
		}

		gUIShell.HideInterface( "enter_password" );
	}
	else if ( message.same_no_case( "enter_password_cancel" ) )
	{
		gUIShell.HideInterface( "enter_password" );
	}
	else if ( message.same_no_case( "mp_dialog_ok" ) )
	{
		eMultiplayerDialog dialog = m_MultiplayerDialog;

		HideMessageBox();

		m_DialogInput = DIALOG_INPUT_OK;

		switch ( dialog )
		{
			case MP_DIALOG_OK:
			{
				break;
			}

			case MP_DIALOG_OK_ENUM_SESSIONS:
			{
				gNetPipe.RequestSessionEnumeration();
				break;
			}

			case MP_DIALOG_CANCEL_FIND:
			{
				gNetPipe.CancelConnect();
				OnNextUpdate( NU_NONE );
				break;
			}

			case MP_DIALOG_CANCEL_CONNECT:
			{
				if ( !m_fEatEnter && ui_window.GetVisible() && ui_window.IsEnabled() )
				{
					// cancel anything we might be doing
					// gNetPipe.StopSessionEnumeration();
					gNetPipe.CancelConnect();
					gNetPipe.RequestSessionEnumeration();
					OnNextUpdate( NU_NONE );
				}
				break;
			}

			case MP_DIALOG_ENTER_PLAYER_NAME:
				{ 
				if ( gWorldState.GetCurrentState() == WS_MP_INTERNET_GAME )
				{
					UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "internet_player_name_edit_box" );
					if ( pEditBox )
					{
						pEditBox->GiveFocus();
					}
				}
				else if ( gWorldState.GetCurrentState() == WS_MP_LAN_GAME )
				{
					UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "lan_player_name_edit_box" );
					if ( pEditBox )
					{
						pEditBox->GiveFocus();
					}
				}
				break;
			}

			case MP_DIALOG_CANCEL_ZONE_CONNECT:
			{
				gUIZoneMatch.OnZoneCancelConnect();
				break;
			}

			case MP_DIALOG_CANCEL_ZONE_LOGIN:
			{
				gUIZoneMatch.OnZoneCancelLogin();
				break;
			}
		}
	}
	else if ( message.same_no_case( "mp_dialog_yes" ) )
	{
		eMultiplayerDialog dialog = m_MultiplayerDialog;

		HideMessageBox();

		m_DialogInput = DIALOG_INPUT_YES;

		switch ( dialog )
		{
			case MP_DIALOG_LEAVE_STAGING_AREA:
			{
				LeaveStagingArea();
				break;
			}

			case MP_DIALOG_YESNO_DELETE_CHARACTER:
			{
				if ( (m_SetSelectedCharacter < 0) || (m_SetSelectedCharacter >= (int)m_MpCharacters.size()) )
				{
					return;
				}

				if ( gTattooGame.DeleteSaveGame( m_MpCharacters[ m_SetSelectedCharacter ]->m_FileName ) )
				{
					m_MpCharacters.clear();
					gTattooGame.GetMpCharacterSaveInfos( m_MpCharacters );

					if ( m_SetSelectedCharacter >= (int)m_MpCharacters.size() )
					{
						m_SetSelectedCharacter = m_MpCharacters.size() - 1;
					}

					{for ( std::vector<DWORD>::iterator i = m_CharacterPortraits.begin(); i != m_CharacterPortraits.end(); ++i )
					{
						gDefaultRapi.DestroyTexture( *i );
					}}
					m_CharacterPortraits.clear();

					{for ( CharacterInfoColl::iterator i = m_MpCharacters.begin(); i != m_MpCharacters.end(); ++i )
					{
						DWORD portrait = gDefaultRapi.CreateAlgorithmicTexture( gpstringf( "portrait%d", m_CharacterPortraits.size() ).c_str(), (*i)->m_Portrait, false, 0, TEX_LOCKED, false, true );
						m_CharacterPortraits.push_back( portrait );
						(*i)->m_Portrait = NULL;
					}}

					StagingAreaShowMpCharacters();

					UISlider * pSlider = (UISlider *)gUIShell.FindUIWindow( "scroll_characters", "staging_area_select_character" );
					if ( pSlider )
					{
						pSlider->SetValue( m_SelectedCharacter - STAGING_CHARACTER_SLOTS );
					}
				}

				break;
			}

			case MP_DIALOG_YESNO_IMPORT_CHARACTER:
			{
				int saveIndex = UIListbox::INVALID_TAG;
				gpwstring characterName;
				int characterIndex = -1;

				UIListbox *pListbox = (UIListbox *)gUIShell.FindUIWindow( "listbox_import_save_games" );
				if ( pListbox )
				{
					saveIndex = pListbox->GetSelectedTag();
				}

				pListbox = (UIListbox *)gUIShell.FindUIWindow( "listbox_import_character" );
				if ( pListbox )
				{
					characterName = pListbox->GetSelectedText();
					characterIndex = pListbox->GetSelectedTag();
				}

				// delete duplicate character

				for ( CharacterInfoColl::iterator i = m_MpCharacters.begin(); i != m_MpCharacters.end(); ++i )
				{
					if ( (*i)->m_ScreenName.same_no_case( characterName ) )
					{
						if ( gTattooGame.DeleteSaveGame( (*i)->m_FileName ) )
						{
							m_MpCharacters.clear();
							gTattooGame.GetMpCharacterSaveInfos( m_MpCharacters );

							if ( m_SetSelectedCharacter >= (int)m_MpCharacters.size() )
							{
								m_SetSelectedCharacter = m_MpCharacters.size() - 1;
							}

							{for ( std::vector<DWORD>::iterator i = m_CharacterPortraits.begin(); i != m_CharacterPortraits.end(); ++i )
							{
								gDefaultRapi.DestroyTexture( *i );
							}}

							m_CharacterPortraits.clear();

							{for ( CharacterInfoColl::iterator i = m_MpCharacters.begin(); i != m_MpCharacters.end(); ++i )
							{
								DWORD portrait = gDefaultRapi.CreateAlgorithmicTexture( gpstringf( "portrait%d", m_CharacterPortraits.size() ).c_str(), (*i)->m_Portrait, false, 0, TEX_LOCKED, false, true );
								m_CharacterPortraits.push_back( portrait );
								(*i)->m_Portrait = NULL;
							}}
						}
						break;
					}
				}

				// import character

				if ( ( saveIndex != UIListbox::INVALID_TAG ) && ( characterIndex != UIListbox::INVALID_TAG ) )
				{
					// free old character
					if ( gServer.GetLocalHumanPlayer()->GetHero() != GOID_INVALID )
					{
						gGoDb.RSMarkGoAndChildrenForDeletion( gServer.GetLocalHumanPlayer()->GetHero() );
					}

					if ( gTattooGame.ImportCharacterFromSave( m_saveInfoColl[ saveIndex ]->m_FileName, characterIndex ) )
					{
						m_SelectedCharacter = -1;
						m_CharacterName = characterName;

						float uberLevel = 0.0f;
						CharacterInfoColl characterInfo;
						gTattooGame.GetSavedCharacterInfos( characterInfo, m_saveInfoColl[ saveIndex ]->m_FileName, false );
						for ( CharacterInfoColl::iterator i = characterInfo.begin(); i != characterInfo.end(); ++i )
						{
							if ( (*i)->m_Index == characterIndex )
							{
								CharacterInfo::SkillDb::iterator skill = (*i)->m_Skills.find( "uber" );
								if ( skill != (*i)->m_Skills.end() )
								{
									uberLevel = skill->second.m_Level;
								}
								break;
							}
						}

						gServer.GetLocalHumanPlayer()->RSSetHeroName( m_CharacterName );
						gServer.GetLocalHumanPlayer()->RSSetHeroUberLevel( uberLevel );

						StagingAreaEnableSelections( true );

						gUIShell.HideInterface( "staging_area_import_character" );
						gUIShell.ShowInterface( "staging_area_players" );
					}
					else
					{
						ShowMessageBox( gpwtranslate( $MSG$ "The character you selected was not loaded." ) );
					}
				}

				break;
			}

			case MP_DIALOG_YESNO_CREATE_CHARACTER:
			{
				gpwstring characterName;

				UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_char_name_edit_box" );
				if ( editBox )
				{
					if ( !editBox->GetText().empty() )
					{
						gpwstring errorMsg;

						gpwstring sName = editBox->GetText();
						if ( !gUIFrontend.IsNameValid( sName ) || !TattooGame::IsValidSaveScreenName( sName, &errorMsg ) )
						{
							ShowMessageBox( errorMsg );
							return;
						}

						characterName = sName;
					}
					else
					{
						return;
					}
				}

				if ( characterName.empty() )
				{
					return;
				}

				// delete duplicate character
				for ( CharacterInfoColl::iterator i = m_MpCharacters.begin(); i != m_MpCharacters.end(); ++i )
				{
					if ( (*i)->m_ScreenName.same_no_case( characterName ) )
					{
						if ( gTattooGame.DeleteSaveGame( (*i)->m_FileName ) )
						{
							m_MpCharacters.clear();
							gTattooGame.GetMpCharacterSaveInfos( m_MpCharacters );

							if ( m_SetSelectedCharacter >= (int)m_MpCharacters.size() )
							{
								m_SetSelectedCharacter = m_MpCharacters.size() - 1;
							}

							{for ( std::vector<DWORD>::iterator i = m_CharacterPortraits.begin(); i != m_CharacterPortraits.end(); ++i )
							{
								gDefaultRapi.DestroyTexture( *i );
							}}

							m_CharacterPortraits.clear();

							{for ( CharacterInfoColl::iterator i = m_MpCharacters.begin(); i != m_MpCharacters.end(); ++i )
							{
								DWORD portrait = gDefaultRapi.CreateAlgorithmicTexture( gpstringf( "portrait%d", m_CharacterPortraits.size() ).c_str(), (*i)->m_Portrait, false, 0, TEX_LOCKED, false, true );
								m_CharacterPortraits.push_back( portrait );
								(*i)->m_Portrait = NULL;
							}}
						}
						break;
					}
				}

				// create new character
				if ( gServer.GetLocalHumanPlayer()->GetHero() != GOID_INVALID )
				{
					gGoDb.RSMarkGoAndChildrenForDeletion( gServer.GetLocalHumanPlayer()->GetHero() );
				}

				m_SelectedCharacter = -1;
				m_CharacterName = characterName;

				gServer.GetLocalHumanPlayer()->RSSetHeroName( m_CharacterName );
				gServer.GetLocalHumanPlayer()->RSSetHeroUberLevel( 0.0f );

				m_pCharacterSelect->AcceptCharacterSelection();
				
				StagingAreaEnableSelections( true );

				gUIShell.HideInterface( "staging_area_create_character" );
				gUIShell.ShowInterface( "staging_area_players" );

				UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_area_chat_editbox" );
				if ( pEditBox )
				{
					pEditBox->SetPermanentFocus( true );
					pEditBox->SetAllowInput( true );
				}

				break;
			}

			case MP_DIALOG_YESNO_CHANGE_WORLD:
			{
				GameGUICallback( "staging_accept_map_settings", ui_window );
				break;
			}			
		}
	}
	else if ( message.same_no_case( "mp_dialog_no" ) )
	{
		HideMessageBox();

		m_DialogInput = DIALOG_INPUT_NO;
	}

	///////////////////////////////////////////////////////////////
	// Staging Area

	else if ( message.same_no_case( "staging_cancel_option" ) )
	{
		HideStagingAreaInterfaces();

		StagingAreaEnableSelections( true );

		gUIShell.ShowInterface( "staging_area_players" );
	}
	else if ( message.same_no_case( "staging_map_settings" ) )
	{
		StagingAreaEnableSelections( false );

		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "staging_map_list" );
		if ( listBox )
		{
			MapInfoColl::iterator i, ibegin = m_MapInfoColl.begin(), iend = m_MapInfoColl.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( i->m_InternalName.same_no_case( gWorldMap.GetMapName() ) )
				{
					listBox->SelectElement( std::distance( ibegin, i ) );

					UITextBox *pText = (UITextBox *)gUIShell.FindUIWindow( "map_description_text" );
					if ( pText )
					{
						pText->SetText( i->m_Description );
					}

					UIListbox *listBoxWorlds = (UIListbox *)gUIShell.FindUIWindow( "listbox_map_world" );
					if ( listBoxWorlds )
					{
						listBoxWorlds->RemoveAllElements();

						UIComboBox *comboWorlds = (UIComboBox *)gUIShell.FindUIWindow( "combo_map_world" );
						if ( comboWorlds )
						{
							if ( !i->m_MpWorlds.empty() )
							{
								comboWorlds->SetEnabled( true );

								for ( MpWorldColl::iterator iWorld = i->m_MpWorlds.begin(); iWorld != i->m_MpWorlds.end(); ++iWorld )
								{
									listBoxWorlds->AddElement( iWorld->m_ScreenName );

									if ( iWorld->m_Name.same_no_case( gWorldMap.GetMpWorldName() ) )
									{
										comboWorlds->SetText( iWorld->m_ScreenName );										
									}
								}
							}
							else
							{
								comboWorlds->SetEnabled( false );
								comboWorlds->SetText( gpwtranslate( $MSG$ "None" ) );
							}
						}
					}

					break;
				}
			}
		}

		HideStagingAreaInterfaces();
		gUIShell.ShowInterface( "staging_area_map_settings" );
	}
	else if ( message.same_no_case( "staging_show_map_info" ) )
	{
		UIComboBox *comboWorlds = (UIComboBox *)gUIShell.FindUIWindow( "combo_map_world" );
		if ( comboWorlds )
		{
			comboWorlds->SetText( L"" );
		}

		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "staging_map_list" );
		if ( listBox )
		{
			if ( !listBox->GetSelectedText().empty() )
			{
				UITextBox *pText = (UITextBox *)gUIShell.FindUIWindow( "map_description_text" );
				if ( pText )
				{
					pText->SetText( m_MapInfoColl[ listBox->GetSelectedTag() ].m_Description );
				}

				UIListbox *listBoxWorlds = (UIListbox *)gUIShell.FindUIWindow( "listbox_map_world" );
				if ( listBoxWorlds )
				{
					listBoxWorlds->RemoveAllElements();

					UIComboBox *comboWorlds = (UIComboBox *)gUIShell.FindUIWindow( "combo_map_world" );
					if ( comboWorlds )
					{
						if ( !m_MapInfoColl[ listBox->GetSelectedTag() ].m_MpWorlds.empty() )
						{
							comboWorlds->SetEnabled( true );

							MpWorldColl::iterator iWorld = m_MapInfoColl[ listBox->GetSelectedTag() ].m_MpWorlds.begin();
							for ( ; iWorld != m_MapInfoColl[ listBox->GetSelectedTag() ].m_MpWorlds.end(); ++iWorld )
							{
								listBoxWorlds->AddElement( iWorld->m_ScreenName );

								if ( iWorld->m_Name.same_no_case( m_MapInfoColl[ listBox->GetSelectedTag() ].m_MpWorlds.GetDefaultMpWorldName() ) )
								{
									comboWorlds->SetText( iWorld->m_ScreenName );
								}
							}
						}
						else
						{
							comboWorlds->SetEnabled( false );
							comboWorlds->SetText( gpwtranslate( $MSG$ "None" ) );
						}
					}
				}
			}
		}
	}
	else if ( message.same_no_case( "show_world_description" ) )
	{
		UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "staging_map_list" );
		if ( !pListbox || pListbox->GetSelectedText().empty() )
		{
			return;
		}

		int selectedMap = pListbox->GetSelectedTag();

		pListbox = (UIListbox *)gUIShell.FindUIWindow( "listbox_map_world" );
		if ( pListbox )
		{
			UITextBox * pHelp = (UITextBox *)gUIShell.FindUIWindow( "multiplayer_help", "multiplayer_help" );
			MpWorldColl::iterator iWorld = m_MapInfoColl[ selectedMap ].m_MpWorlds.begin();
			for ( ; iWorld != m_MapInfoColl[ selectedMap ].m_MpWorlds.end(); ++iWorld )
			{
				if ( iWorld->m_ScreenName.same_no_case( pListbox->GetSelectedText() ) )
				{
					pHelp->SetText( iWorld->m_Description );
					break;
				}
			}
		}
	}
	else if ( message.same_no_case( "clear_world_description" ) )
	{
		UITextBox * pHelp = (UITextBox *)gUIShell.FindUIWindow( "multiplayer_help", "multiplayer_help" );
		if ( pHelp )
		{
			pHelp->SetText( L"" );
		}
	}
	else if ( message.same_no_case( "staging_accept_map_settings" ) )
	{
		if ( gServer.IsLocal() )
		{
			UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "staging_map_list" );
			if ( listBox )
			{
				if ( !listBox->GetSelectedText().empty() )
				{
					const gpstring& mapName = m_MapInfoColl[ listBox->GetSelectedTag() ].m_InternalName;

					gpstring worldName;

					UIComboBox *comboWorlds = (UIComboBox *)gUIShell.FindUIWindow( "combo_map_world" );
					if ( comboWorlds && comboWorlds->IsEnabled() )
					{
						MpWorldColl::iterator iWorld = m_MapInfoColl[ listBox->GetSelectedTag() ].m_MpWorlds.begin();
						for ( ; iWorld != m_MapInfoColl[ listBox->GetSelectedTag() ].m_MpWorlds.end(); ++iWorld )
						{
							if ( iWorld->m_ScreenName.same_no_case( comboWorlds->GetText() ) )
							{
								if ( !IsWorldLevelValidForCharacters( iWorld->m_RequiredLevel ) && (m_DialogInput != DIALOG_INPUT_YES) )
								{
									ShowMessageBox( gpwtranslate( $MSG$ "Some of the characters in the game are below the world level you've selected. Do you want to proceed?" ), MP_DIALOG_YESNO_CHANGE_WORLD );
									return;
								}
								worldName = iWorld->m_Name;
								break;
							}
						}
					}

					bool changedMap = !mapName.same_no_case( gWorldMap.GetMapName() );
					bool changedMapWorld = !worldName.same_no_case( gWorldMap.GetMpWorldName() ) &&
										   !(changedMap && worldName.same_no_case( m_MapInfoColl[ listBox->GetSelectedTag() ].m_MpWorlds.GetDefaultMpWorldName() ));

					if ( !gWorldMap.SSet( mapName, worldName, RPC_TO_ALL ) )
					{
						ShowMessageBox( gpwtranslate( $MSG$ "The selected map could not be loaded." ) );
						return;
					}

					if ( changedMap )
					{
						RSDisplayMessage( gpwstringf( gpwtranslate( $MSG$ "Host changed the map to \"%s\"." ), gWorldMap.GetMapScreenName().c_str() ), HOST_CHANGE_SETTING_MSG_COLOR );
						
						if ( gUIZoneMatch.IsHosting() )
						{
							gUIZoneMatch.HostUpdateGameSetting( ZONE_GAME_MAP, gWorldMap.GetMapScreenName() );
						}
					}

					if ( changedMapWorld )
					{
						RSDisplayMessage( gpwstringf( gpwtranslate( $MSG$ "Host changed the World Difficulty to %s." ), comboWorlds->GetText().c_str() ), HOST_CHANGE_SETTING_MSG_COLOR );
					}

					if ( gUIZoneMatch.IsHosting() )
					{
						gUIZoneMatch.HostUpdateGameSetting( ZONE_GAME_WORLD_LEVEL, comboWorlds->GetText() );
						gUIZoneMatch.UpdateHostWorldFlags( comboWorlds->GetText() );
					}
				}
			}

			StagingAreaCheckEnableStart();

			StagingAreaEnableSelections( true );

			gUIShell.HideInterface( "staging_area_map_settings" );
			gUIShell.ShowInterface( "staging_area_players" );
		}
	}
	else if ( message.same_no_case( "staging_game_settings" ) )
	{
		StagingAreaEnableSelections( false );

		ShowStagingAreaGameSettings();

		HideStagingAreaInterfaces();
		gUIShell.ShowInterface( "staging_area_game_settings" );
	}
	else if ( message.same_no_case( "staging_accept_game_settings" ) )
	{
		if ( gServer.IsLocal() )
		{
			UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_player_limit" );
			if ( pCombo )
			{
				for ( unsigned int players = 2; players <= Server::MAX_PLAYERS; ++players )
				{
					if ( pCombo->GetText().same_no_case( gpwstringf( gpwtranslate( $MSG$ "%d Players" ), players ), players ) )
					{
						if ( players < (unsigned int)Player::GetHumanPlayerCount() )
						{
							ShowMessageBox( gpwstringf( gpwtranslate( $MSG$ "You must remove players from the game before setting the limit to %d Players." ), players ) );
							return;
						}

						NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

						if( session && ( players != session->GetNumMachinesMax() ) )
						{
							gServer.SSetMaxPlayers( players );

							if ( gUIZoneMatch.IsHosting() )
							{
								gUIZoneMatch.HostUpdateGameSetting( ZONE_GAME_MAX_PLAYERS, gpwstringf( L"%d", gServer.GetMaxPlayers() ) );
							}

							RSDisplayMessage( gpwstringf( gpwtranslate( $MSG$ "Host set the player limit to %d players." ), players ), HOST_CHANGE_SETTING_MSG_COLOR );
						}
						break;
					}
				}

			}

			UIListbox *pListbox = (UIListbox *)gUIShell.FindUIWindow( "listbox_game_type" );
			if ( pListbox )
			{
				gpstring gameType = gVictory.GetGameTypeName( pListbox->GetSelectedTag() );

				if ( !gameType.empty() && !gameType.same_no_case( gVictory.GetGameType()->GetName() ) )
				{
					gVictory.SSetGameType( gameType );
				}
			}
			
			UICheckbox * pPvpCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_enable_pvp" );
			UICheckbox * pTeamCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_enable_teamplay" );
			if ( pPvpCheckbox && pTeamCheckbox )
			{	
				gpstring gameType;
				if ( !pPvpCheckbox->GetState() && !pTeamCheckbox->GetState() )
				{
					gameType = "coop_no_teams";
				}
				else if ( !pPvpCheckbox->GetState() && pTeamCheckbox->GetState() )
				{
					gameType = "coop_teams";
				}
				else if ( pPvpCheckbox->GetState() && !pTeamCheckbox->GetState() )
				{
					gameType = "pvp_no_teams";
				}
				else if ( pPvpCheckbox->GetState() && pTeamCheckbox->GetState() )
				{
					gameType = "pvp_teams";
				}

				if ( !gameType.empty() && !gameType.same_no_case( gVictory.GetGameType()->GetName() ) )
				{
					bool wasPvP = gVictory.GetGameType()->GetName().same_no_case( "pvp_no_teams" ) || gVictory.GetGameType()->GetName().same_no_case( "pvp_teams" );
					bool isPvP = gameType.same_no_case( "pvp_no_teams" ) || gameType.same_no_case( "pvp_teams" );

					if ( isPvP != wasPvP )
					{
						if ( isPvP )
						{
							RSDisplayMessage( gpwtranslate( $MSG$ "Host has enabled Player-vs-Player combat." ), HOST_CHANGE_SETTING_MSG_COLOR );
						}
						else
						{
							RSDisplayMessage( gpwtranslate( $MSG$ "Host has disabled Player-vs-Player combat." ), HOST_CHANGE_SETTING_MSG_COLOR );
						}
					}

					bool wasTeams = gVictory.GetGameType()->GetName().same_no_case( "coop_teams" ) || gVictory.GetGameType()->GetName().same_no_case( "pvp_teams" );
					bool isTeams = gameType.same_no_case( "coop_teams" ) || gameType.same_no_case( "pvp_teams" );

					if ( isTeams != wasTeams )
					{
						if ( isTeams )
						{
							RSDisplayMessage( gpwtranslate( $MSG$ "Host has enabled team play." ), HOST_CHANGE_SETTING_MSG_COLOR );
						}
						else
						{
							RSDisplayMessage( gpwtranslate( $MSG$ "Host has disabled team play." ), HOST_CHANGE_SETTING_MSG_COLOR );
						}
					}

					if ( gUIZoneMatch.IsHosting() )
					{
						gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_PVP_ENABLED, isPvP );
						gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_TEAMS_ENABLED, isTeams );
					}

					gVictory.SSetGameType( gameType );
				}
			}

			pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_difficulty" );
			if ( pCombo )
			{
				if ( pCombo->GetText().same_no_case( gpwtranslate( $MSG$ "Easy" ) ) )
				{
					if ( gWorldOptions.GetDifficulty() != DIFFICULTY_EASY )
					{
						gWorldOptions.RCSetDifficulty( DIFFICULTY_EASY );

						RSDisplayMessage( gpwtranslate( $MSG$ "Host set the game difficulty to Easy." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}
				}
				else if ( pCombo->GetText().same_no_case( gpwtranslate( $MSG$ "Normal" ) ) )
				{
					if ( gWorldOptions.GetDifficulty() != DIFFICULTY_MEDIUM )
					{
						gWorldOptions.RCSetDifficulty( DIFFICULTY_MEDIUM );

						RSDisplayMessage( gpwtranslate( $MSG$ "Host set the game difficulty to Normal." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}
				}
				else if ( pCombo->GetText().same_no_case( gpwtranslate( $MSG$ "Hard" ) ) )
				{
					if ( gWorldOptions.GetDifficulty() != DIFFICULTY_HARD )
					{
						gWorldOptions.RCSetDifficulty( DIFFICULTY_HARD );

						RSDisplayMessage( gpwtranslate( $MSG$ "Host set the game difficulty to Hard." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}
				}

				if ( gUIZoneMatch.IsHosting() )
				{
					gUIZoneMatch.HostUpdateGameSetting( ZONE_GAME_DIFFICULTY, gpwstringf( L"%d", (int)gWorldOptions.GetDifficulty() ) );
				}
			}

			pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_time_limit" );
			if ( pCombo )
			{
				if ( pCombo->GetSelectedTag() != -1 ) 
				{
					float timeLimit = 60.0f * pCombo->GetSelectedTag();
					if ( timeLimit != gVictory.GetTimeLimit() )
					{
						SSetTimeLimit( timeLimit );

						RSDisplayMessage( gpwstringf( gpwtranslate( $MSG$ "Host set the time limit to %s." ), pCombo->GetText().c_str() ), HOST_CHANGE_SETTING_MSG_COLOR );

						if ( gUIZoneMatch.IsHosting() )
						{
							gUIZoneMatch.HostUpdateGameSetting( ZONE_GAME_TIME_LIMIT, gpwstringf( L"%d", (int)(gVictory.GetTimeLimit() / 60) ) );
						}

						gServer.SetSettingsDirty();
					}					
				}	
				else 
				{
					if ( gVictory.GetTimeLimit() != -1 )
					{
						SSetTimeLimit( -1 );

						RSDisplayMessage( gpwstringf( gpwtranslate( $MSG$ "Host set the time limit to %s." ), pCombo->GetText().c_str() ), HOST_CHANGE_SETTING_MSG_COLOR );

						if ( gUIZoneMatch.IsHosting() )
						{
							gUIZoneMatch.HostUpdateGameSetting( ZONE_GAME_TIME_LIMIT, gpwstringf( L"%d", (int)(gVictory.GetTimeLimit() / 60) ) );
						}

						gServer.SetSettingsDirty();
					}
				}
			}

			pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_characters_drop" );
			if ( pCombo )
			{
				if ( pCombo->GetText().same_no_case( gpwtranslate( $MSG$ "Nothing" ) ) )
				{
					if ( gServer.GetDropInvOption() != DIO_NOTHING )
					{
						gServer.SSetDropInvOption( DIO_NOTHING );

						RSDisplayMessage( gpwtranslate( $MSG$ "Host set \"Drop on Death\" to Nothing." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}
				}
				else if ( pCombo->GetText().same_no_case( gpwtranslate( $MSG$ "Equipped Items" ) ) )
				{
					if ( gServer.GetDropInvOption() != DIO_EQUIPPED )
					{
						gServer.SSetDropInvOption( DIO_EQUIPPED );

						RSDisplayMessage( gpwtranslate( $MSG$ "Host set \"Drop on Death\" to Equipped Items only." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}
				}
				else if ( pCombo->GetText().same_no_case( gpwtranslate( $MSG$ "Inventory" ) ) )
				{
					if ( gServer.GetDropInvOption() != DIO_INVENTORY )
					{
						gServer.SSetDropInvOption( DIO_INVENTORY );

						RSDisplayMessage( gpwtranslate( $MSG$ "Host set \"Drop on Death\" to Inventory only." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}
				}
				else if ( pCombo->GetText().same_no_case( gpwtranslate( $MSG$ "All" ) ) )
				{
					if ( gServer.GetDropInvOption() != DIO_ALL )
					{
						gServer.SSetDropInvOption( DIO_ALL );

						RSDisplayMessage( gpwtranslate( $MSG$ "Host set \"Drop on Death\" to All." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}
				}

				if ( gUIZoneMatch.IsHosting() )
				{
					gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_DROP_EQUIPPED_ON_DEATH, (gServer.GetDropInvOption() == DIO_EQUIPPED) || (gServer.GetDropInvOption() == DIO_ALL) );
					gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_DROP_INVENTORY_ON_DEATH, (gServer.GetDropInvOption() == DIO_INVENTORY) || (gServer.GetDropInvOption() == DIO_ALL) );
					gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_DROP_ANYTHING_ON_DEATH, ( gServer.GetDropInvOption() == DIO_ALL || gServer.GetDropInvOption() == DIO_INVENTORY || gServer.GetDropInvOption() == DIO_EQUIPPED) );
				}
			}

			pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_character_respawn" );
			if ( pCombo )
			{
				if ( pCombo->GetText().same_no_case( gpwtranslate( $MSG$ "Respawn at Corpse" ) ) )
				{
					if ( !gServer.GetAllowRespawn() )
					{
						gServer.SSetAllowRespawn( true );
					}
				}
				else if ( pCombo->GetText().same_no_case( gpwtranslate( $MSG$ "No Respawn" ) ) )
				{
					if ( gServer.GetAllowRespawn() )
					{
						gServer.SSetAllowRespawn( false );
					}
				}
			}

			UICheckbox *pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_allow_start_selection" );
			if ( pCheckbox )
			{
				if ( gServer.GetAllowStartLocationSelection() != pCheckbox->GetState() )
				{
					gServer.SSetAllowStartLocationSelection( pCheckbox->GetState() );

					if ( gServer.GetAllowStartLocationSelection() )
					{
						RSDisplayMessage( gpwtranslate( $MSG$ "Host has allowed the selection of Start Locations." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}
					else
					{
						RSDisplayMessage( gpwtranslate( $MSG$ "Host has prevented the selection of Start Locations." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}

					if ( gUIZoneMatch.IsHosting() )
					{
						gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_ALLOW_START_SELCTION, gServer.GetAllowStartLocationSelection() );
					}
				}
			}

			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_allow_new_characters" );
			if ( pCheckbox )
			{
				if ( gServer.GetAllowNewCharactersOnly() != pCheckbox->GetState() )
				{
					gServer.SSetAllowNewCharactersOnly( pCheckbox->GetState() );

					if ( gServer.GetAllowNewCharactersOnly() )
					{
						RSDisplayMessage( gpwtranslate( $MSG$ "Host has restricted all players to new characters." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}
					else
					{
						RSDisplayMessage( gpwtranslate( $MSG$ "Host has allowed saved characters to be used." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}

					if ( gUIZoneMatch.IsHosting() )
					{
						gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_NEW_CHARACTERS_ONLY, gServer.GetAllowNewCharactersOnly() );
					}
				}
			}

			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_allow_player_pausing" );
			if ( pCheckbox )
			{
				if ( gServer.GetAllowPausing() != pCheckbox->GetState() )
				{
					gServer.SSetAllowPausing( pCheckbox->GetState() );

					if ( !gServer.GetAllowPausing() )
					{
						RSDisplayMessage( gpwtranslate( $MSG$ "Host has restricted all players from pausing the game." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}
					else
					{
						RSDisplayMessage( gpwtranslate( $MSG$ "Host has allowed all players to be able to pause the game." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}

					if ( gUIZoneMatch.IsHosting() )
					{
						gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_ALLOW_PAUSING, gServer.GetAllowPausing() );
					}
				}
			}

			pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_allow_player_joining" );
			if ( pCheckbox )
			{
				if ( gServer.GetAllowJIP() != pCheckbox->GetState() )
				{
					gServer.SSetAllowJIP( pCheckbox->GetState() );

					if ( gServer.GetAllowJIP() )
					{
						RSDisplayMessage( gpwtranslate( $MSG$ "Host has allowed new players to join the game while in progress." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}
					else
					{
						RSDisplayMessage( gpwtranslate( $MSG$ "Host has prevented new players from joining the game while in progress." ), HOST_CHANGE_SETTING_MSG_COLOR );
					}

					if ( gUIZoneMatch.IsHosting() )
					{
						gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_ALLOW_JIP, gServer.GetAllowJIP() );
					}
				}
			}
		}

		StagingAreaEnableSelections( true );

		gUIShell.HideInterface( "staging_area_game_settings" );
		gUIShell.ShowInterface( "staging_area_players" );
	}
	else if ( message.same_no_case( "staging_change_password" ) )
	{
		gUIShell.ShowInterface( "staging_area_change_password" );

		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "change_game_password_edit_box", "staging_area_change_password" );
		if ( editBox )
		{
			editBox->SetText( L"" );
		}
	}
	else if ( message.same_no_case( "change_password_ok" ) )
	{
		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "change_game_password_edit_box", "staging_area_change_password" );
		if ( editBox )
		{
			gServer.SSetGamePassword( editBox->GetText() );

			// inform zoneMatch of our password change if they care :)
			if( IsGunMM() )
			{
				gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_PASSWORD_PROTECTED, !editBox->GetText().empty() );
			}
			ShowMessageBox( gpwtranslate( $MSG$ "The game password has been changed." ) );
		}
		gUIShell.HideInterface( "staging_area_change_password" );
	}
	else if ( message.same_no_case( "change_password_cancel" ) )
	{
		gUIShell.HideInterface( "staging_area_change_password" );
	}
	else if ( message.same_no_case( "staging_select_character" ) )
	{
		HideStagingAreaInterfaces();

		StagingAreaEnableSelections( false );

		if ( m_MpCharacters.empty() || gServer.GetAllowNewCharactersOnly() )
		{
			gUIShell.ShowInterface( "staging_area_create_character" );

			EnableCharacterAppearanceSelection();

			UIWindow * pWindow = gUIShell.FindUIWindow( "staging_import_character", "staging_area_create_character" );
			if ( pWindow )
			{
				pWindow->SetVisible( !gServer.GetAllowNewCharactersOnly() && m_MpCharacters.empty() );
			}

			UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_char_name_edit_box" );
			if ( editBox )
			{
				editBox->SetAllowInput( true );
				editBox->SetText( m_CharacterName );
			}

			UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_area_chat_editbox" );
			if ( pEditBox )
			{
				pEditBox->SetPermanentFocus( false );
				pEditBox->SetAllowInput( false );
			}
		}
		else
		{
			m_SetSelectedCharacter = m_SelectedCharacter;

			StagingAreaShowMpCharacters();

			UISlider * pSlider = (UISlider *)gUIShell.FindUIWindow( "scroll_characters", "staging_area_select_character" );
			if ( pSlider )
			{
				pSlider->SetValue( m_SelectedCharacter - STAGING_CHARACTER_SLOTS );
			}

			gUIShell.ShowInterface( "staging_area_select_character" );
		}
	}
	else if ( message.same_no_case( "slider_scroll_characters" ) )
	{
		StagingAreaShowMpCharacters( ((UISlider *)(&ui_window))->GetValue() );
	}
	else if ( message.same_no_case( "scroll_characters_up" ) )
	{
		if ( ui_window.GetParentWindow() && (ui_window.GetParentWindow()->GetType() == UI_TYPE_SLIDER) )
		{
			UISlider *pSlider = (UISlider *)ui_window.GetParentWindow();
			pSlider->SetValue( pSlider->GetValue() - 1 );
		}
	}
	else if ( message.same_no_case( "scroll_characters_down" ) )
	{
		if ( ui_window.GetParentWindow() && (ui_window.GetParentWindow()->GetType() == UI_TYPE_SLIDER) )
		{
			UISlider *pSlider = (UISlider *)ui_window.GetParentWindow();
			pSlider->SetValue( pSlider->GetValue() + 1 );
		}
	}
	else if ( message.same_no_case( "on_select_character" ) )
	{
		UISlider * pSlider = (UISlider *)gUIShell.FindUIWindow( "scroll_characters", "staging_area_select_character" );
		if ( pSlider )
		{
			HIDE(( "character%d_sel", m_SetSelectedCharacter - pSlider->GetValue() + 1 ));

			m_SetSelectedCharacter = pSlider->GetValue() + ui_window.GetTag();
			if ( m_SetSelectedCharacter >= (int)m_MpCharacters.size() )
			{
				m_SetSelectedCharacter = m_MpCharacters.size() - 1;
			}

			SHOW(( "character%d_sel", m_SetSelectedCharacter - pSlider->GetValue() + 1 ));
		}
	}
	else if ( message.same_no_case( "staging_select_character_ok" ) )
	{
		if ( (m_SetSelectedCharacter < 0) || (m_SetSelectedCharacter >= (int)m_MpCharacters.size()) )
		{
			ShowMessageBox( gpwtranslate( $MSG$ "You have not selected a character." ) );
			return;
		}
	
		int selectCharacterCancel = m_SelectedCharacter;
		m_SelectedCharacter = m_SetSelectedCharacter;

		// check character skill level
		if ( !DoesCharacterMeetWorldLevel( GetSelectedCharacterUberLevel() ) )
		{
			ShowMessageBox( gpwtranslate( $MSG$ "This character is below the level required for the chosen World." ) );
			m_SelectedCharacter = selectCharacterCancel;
			return;
		}

		// free old character
		if ( gServer.GetLocalHumanPlayer()->GetHero() != GOID_INVALID )
		{
			gGoDb.RSMarkGoAndChildrenForDeletion( gServer.GetLocalHumanPlayer()->GetHero() );
		}

		if ( gTattooGame.ImportCharacterFromSave( m_MpCharacters[ m_SetSelectedCharacter ]->m_FileName, m_MpCharacters[ m_SetSelectedCharacter ]->m_Index ) )
		{
			m_CharacterName = m_MpCharacters[ m_SelectedCharacter ]->m_ScreenName;
			gServer.GetLocalHumanPlayer()->RSSetHeroName( m_CharacterName );
			gServer.GetLocalHumanPlayer()->RSSetHeroUberLevel( GetSelectedCharacterUberLevel() );
		}
		else
		{
			ShowMessageBox( gpwtranslate( $MSG$ "Unable to load the selected character." ) );
			m_SelectedCharacter = selectCharacterCancel;
			return;
		}

		StagingAreaEnableSelections( true );

		gUIShell.HideInterface( "staging_area_select_character" );
		gUIShell.ShowInterface( "staging_area_players" );
	}
	else if ( message.same_no_case( "staging_select_character_cancel" ) )
	{
		StagingAreaEnableSelections( true );

		gUIShell.HideInterface( "staging_area_select_character" );
		gUIShell.ShowInterface( "staging_area_players" );
	}
	else if ( message.same_no_case( "staging_delete_character" ) )
	{
		ShowMessageBox( gpwtranslate( $MSG$ "Are you sure you want to delete this character?" ), MP_DIALOG_YESNO_DELETE_CHARACTER );
	}
	else if ( message.same_no_case( "staging_create_character" ) )
	{
		gUIShell.HideInterface( "staging_area_select_character" );
		gUIShell.ShowInterface( "staging_area_create_character" );

		EnableCharacterAppearanceSelection();

		UIWindow * pWindow = gUIShell.FindUIWindow( "staging_import_character", "staging_area_create_character" );
		if ( pWindow )
		{
			pWindow->SetVisible( !gServer.GetAllowNewCharactersOnly() && m_MpCharacters.empty() );
		}

		gpassertm( m_pCharacterSelect, "Character selection UI not initialized." );

		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_char_name_edit_box" );
		if ( editBox )
		{
			editBox->SetAllowInput( true );
			editBox->SetText( L"" );
		}

		UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_area_chat_editbox" );
		if ( pEditBox )
		{
			pEditBox->SetPermanentFocus( false );
			pEditBox->SetAllowInput( false );
		}

	}
	else if ( message.same_no_case( "next_character" ) )
	{
		m_pCharacterSelect->SetNextActiveCharacter();
		EnableCharacterAppearanceSelection();
		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
	else if ( message.same_no_case( "previous_character" ) )
	{
		m_pCharacterSelect->SetPreviousActiveCharacter();
		EnableCharacterAppearanceSelection();
		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
	else if ( message.same_no_case( "char_next_pants" ) )
	{
		m_pCharacterSelect->SelectNextPantsTexture();
		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
	else if ( message.same_no_case( "char_prev_pants" ) )
	{
		m_pCharacterSelect->SelectPreviousPantsTexture();
		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
	else if ( message.same_no_case( "char_next_shirt" ) )
	{
		m_pCharacterSelect->SelectNextShirtTexture();
		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
	else if ( message.same_no_case( "char_prev_shirt" ) )
	{
		m_pCharacterSelect->SelectPreviousShirtTexture();
		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
	else if ( message.same_no_case( "char_prev_face" ) )
	{
		m_pCharacterSelect->SelectPreviousFaceTexture();
		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
	else if ( message.same_no_case( "char_next_face" ) )
	{
		m_pCharacterSelect->SelectNextFaceTexture();
		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
	else if ( message.same_no_case( "char_prev_hair" ) )
	{
		m_pCharacterSelect->SelectPreviousHairTexture();
		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
	else if ( message.same_no_case( "char_next_hair" ) )
	{
		m_pCharacterSelect->SelectNextHairTexture();
		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
	else if ( message.same_no_case( "char_next_head" ) )
	{
		m_pCharacterSelect->SelectNextHeadTexture();
		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
	else if ( message.same_no_case( "char_prev_head" ) )
	{
		m_pCharacterSelect->SelectPreviousHeadTexture();
		gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	}
	else if ( message.same_no_case( "listener_mouse_move" ) )
	{
		m_pCharacterSelect->SetRotationX( -((float)(((UIMouseListener *)&ui_window)->GetDeltaX())/100.0f) );
	}
	else if ( message.same_no_case( "staging_create_character_ok" ) )
	{
		gpwstring characterName;

		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_char_name_edit_box" );
		if ( editBox )
		{
			if ( !editBox->GetText().empty() )
			{
				gpwstring errorMsg;

				gpwstring sName = editBox->GetText();
				if ( !gUIFrontend.IsNameValid( sName ) || !TattooGame::IsValidSaveScreenName( sName, &errorMsg ) )
				{
					ShowMessageBox( errorMsg );
					return;
				}

				characterName = sName;
			}
			else
			{
				return;
			}
		}

		if ( characterName.empty() )
		{
			return;
		}

		bool bAllowCreateCharacter = true;

		if ( gWorldMap.IsInitialized() && !gWorldMap.GetMpWorldName().empty() )
		{
			MpWorldColl mpWorlds;
			gWorldMap.GetMpWorlds( mpWorlds );

			for ( MpWorldColl::iterator i = mpWorlds.begin(); i != mpWorlds.end(); ++i )
			{
				if ( i->m_Name.same_no_case( gWorldMap.GetMpWorldName() ) )
				{
					if ( i->m_RequiredLevel > 0 )
					{
						bAllowCreateCharacter = false;
					}
					break;
				}
			}
		}

		if ( !bAllowCreateCharacter )
		{
			ShowMessageBox( gpwtranslate( $MSG$ "The chosen World does not allow new characters." ) );
			return;
		}

		for ( CharacterInfoColl::iterator i = m_MpCharacters.begin(); i != m_MpCharacters.end(); ++i )
		{
			if ( (*i)->m_ScreenName.same_no_case( characterName ) )
			{
				ShowMessageBox( gpwtranslate( $MSG$ "Creating this character will overwrite an existing one of the same name. Are you sure?" ), MP_DIALOG_YESNO_CREATE_CHARACTER );
				return;
			}
		}

		// free old character
		if ( gServer.GetLocalHumanPlayer()->GetHero() != GOID_INVALID )
		{
			gGoDb.RSMarkGoAndChildrenForDeletion( gServer.GetLocalHumanPlayer()->GetHero() );
		}

		m_CharacterName = characterName;

		gServer.GetLocalHumanPlayer()->RSSetHeroName( m_CharacterName );
		gServer.GetLocalHumanPlayer()->RSSetHeroUberLevel( 0.0f );

		m_pCharacterSelect->AcceptCharacterSelection();
		
		m_SelectedCharacter = -1;

		StagingAreaEnableSelections( true );

		gUIShell.ShowInterface( "staging_area_players" );
		gUIShell.HideInterface( "staging_area_create_character" );

		UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_area_chat_editbox" );
		if ( pEditBox )
		{
			pEditBox->SetPermanentFocus( true );
			pEditBox->SetAllowInput( true );
		}
	}
	else if ( message.same_no_case( "staging_create_character_cancel" ) )
	{
		m_pCharacterSelect->CancelCharacterSelection();

		gUIShell.HideInterface( "staging_area_create_character" );

		if ( m_MpCharacters.empty() || gServer.GetAllowNewCharactersOnly() )
		{
			StagingAreaEnableSelections( true );

			gUIShell.ShowInterface( "staging_area_players" );
		}
		else
		{
			gUIShell.ShowInterface( "staging_area_select_character" );
		}

		UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_area_chat_editbox" );
		if ( pEditBox )
		{
			pEditBox->SetPermanentFocus( true );
			pEditBox->SetAllowInput( true );
		}
	}
	else if ( message.same_no_case( "staging_import_character" ) )
	{
		gUIShell.HideInterface( "staging_area_select_character" );
		gUIShell.HideInterface( "staging_area_create_character" );
		gUIShell.ShowInterface( "staging_area_import_character" );

		UIListbox *pListbox = (UIListbox *)gUIShell.FindUIWindow( "listbox_import_character" );
		if ( pListbox )
		{
			pListbox->RemoveAllElements();
		}

		pListbox = (UIListbox *)gUIShell.FindUIWindow( "listbox_import_save_games" );
		if ( pListbox )
		{
			// get save infos
			gTattooGame.GetCampaignSaveGameInfos( m_saveInfoColl, true );

			// re-sort based on screen name
			m_saveInfoColl.sort( SaveInfo::LessByScreenName() );

			// add each
			pListbox->RemoveAllElements();

			UIComboBox * pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_import_save_games" );

			SaveInfoColl::iterator i;
			int listIndex = 0;
			for( i = m_saveInfoColl.begin(); i != m_saveInfoColl.end(); ++i, ++listIndex )
			{
				pListbox->AddElement( (*i)->m_ScreenName, listIndex );

				if ( pCombo && pCombo->GetText().empty() )
				{
					pCombo->SetText( (*i)->m_ScreenName );
					pListbox->SelectElement( listIndex );
				}			
			}			

			int tag = pListbox->GetSelectedTag();

			pListbox = (UIListbox *)gUIShell.FindUIWindow( "listbox_import_character" );
			if ( pListbox && (tag != -1) )
			{
				pListbox->RemoveAllElements();

				CharacterInfoColl characterInfo;
				gTattooGame.GetSavedCharacterInfos( characterInfo, m_saveInfoColl[ tag ]->m_FileName, false );

				CharacterInfoColl::iterator i;
				for ( i = characterInfo.begin(); i != characterInfo.end(); ++i )
				{
					pListbox->AddElement( (*i)->m_ScreenName, (*i)->m_Index );
				}
			}
		}
	}
	else if ( message.same_no_case( "select_import_save_game" ) )
	{
		UIListbox *pListbox = (UIListbox *)gUIShell.FindUIWindow( "listbox_import_save_games" );
		if ( pListbox )
		{
			int tag = pListbox->GetSelectedTag();

			pListbox = (UIListbox *)gUIShell.FindUIWindow( "listbox_import_character" );
			if ( pListbox && (tag != -1) )
			{
				pListbox->RemoveAllElements();

				CharacterInfoColl characterInfo;
				gTattooGame.GetSavedCharacterInfos( characterInfo, m_saveInfoColl[ tag ]->m_FileName, false );

				CharacterInfoColl::iterator i;
				for ( i = characterInfo.begin(); i != characterInfo.end(); ++i )
				{
					pListbox->AddElement( (*i)->m_ScreenName, (*i)->m_Index );
				}
			}
		}
	}
	else if ( message.same_no_case( "staging_accept_import_character" ) )
	{
		int saveIndex = UIListbox::INVALID_TAG;
		gpwstring characterName;
		int characterIndex = -1;

		UIListbox *pListbox = (UIListbox *)gUIShell.FindUIWindow( "listbox_import_save_games" );
		if ( pListbox )
		{
			saveIndex = pListbox->GetSelectedTag();
		}

		pListbox = (UIListbox *)gUIShell.FindUIWindow( "listbox_import_character" );
		if ( pListbox )
		{
			characterName = pListbox->GetSelectedText();
			characterIndex = pListbox->GetSelectedTag();
		}

		if ( ( saveIndex != UIListbox::INVALID_TAG ) && ( characterIndex != UIListbox::INVALID_TAG ) )
		{
			for ( CharacterInfoColl::iterator i = m_MpCharacters.begin(); i != m_MpCharacters.end(); ++i )
			{
				if ( (*i)->m_ScreenName.same_no_case( characterName ) )
				{
					ShowMessageBox( gpwtranslate( $MSG$ "A saved multiplayer character already exists with this name. Importing this character will overwrite the existing one. Are you sure?" ), MP_DIALOG_YESNO_IMPORT_CHARACTER );
					return;
				}
			}

			// free old character
			if ( gServer.GetLocalHumanPlayer()->GetHero() != GOID_INVALID )
			{
				gGoDb.RSMarkGoAndChildrenForDeletion( gServer.GetLocalHumanPlayer()->GetHero() );
			}

			if ( gTattooGame.ImportCharacterFromSave( m_saveInfoColl[ saveIndex ]->m_FileName, characterIndex ) )
			{
				m_CharacterName = characterName;

				float uberLevel = 0.0f;
				CharacterInfoColl characterInfo;
				gTattooGame.GetSavedCharacterInfos( characterInfo, m_saveInfoColl[ saveIndex ]->m_FileName, false );
				for ( CharacterInfoColl::iterator i = characterInfo.begin(); i != characterInfo.end(); ++i )
				{
					if ( (*i)->m_Index == characterIndex )
					{
						CharacterInfo::SkillDb::iterator skill = (*i)->m_Skills.find( "uber" );
						if ( skill != (*i)->m_Skills.end() )
						{
							uberLevel = skill->second.m_Level;
						}
						break;
					}
				}

				gServer.GetLocalHumanPlayer()->RSSetHeroName( m_CharacterName );
				gServer.GetLocalHumanPlayer()->RSSetHeroUberLevel( uberLevel );

				m_SelectedCharacter = -1;

				StagingAreaEnableSelections( true );

				gUIShell.HideInterface( "staging_area_import_character" );
				gUIShell.ShowInterface( "staging_area_players" );
			}
			else
			{
				ShowMessageBox( gpwtranslate( $MSG$ "The character you selected was not loaded." ) );
			}
		}
		else
		{
			ShowMessageBox( gpwtranslate( $MSG$ "Please select a character first." ) );
		}
	}
	else if ( message.same_no_case( "staging_cancel_import_character" ) )
	{
		gUIShell.HideInterface( "staging_area_import_character" );
		if ( m_MpCharacters.empty() )
		{
			UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_char_name_edit_box" );
			if ( editBox )
			{
				editBox->SetAllowInput( true );
				editBox->SetText( L"" );
			}

			UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_area_chat_editbox" );
			if ( pEditBox )
			{
				pEditBox->SetPermanentFocus( false );
				pEditBox->SetAllowInput( false );
			}

			gUIShell.ShowInterface( "staging_area_create_character" );
		}
		else
		{
			gUIShell.ShowInterface( "staging_area_select_character" );
		}
	}
	else if ( message.same_no_case( "show_player_options_popup" ) )
	{
		if ( gServer.IsLocal() )
		{
			UIPopupMenu *pMenu = (UIPopupMenu *)gUIShell.FindUIWindow( "player_options_popup_menu" );
			if ( pMenu )
			{
				pMenu->RemoveAllElements();

				PlayerId playerId = MakePlayerId( ui_window.GetTag() );

				PlayerId localPlayerId = PLAYERID_INVALID;
				if ( gServer.GetLocalHumanPlayer() )
				{
					localPlayerId = gServer.GetLocalHumanPlayer()->GetId();
				}

				if ( (playerId != PLAYERID_INVALID) &&
					 (playerId != localPlayerId) )
				{
					MENU_COMMAND( pMenu, MENU_PLAYER_EJECT );
					MENU_COMMAND( pMenu, MENU_PLAYER_BAN );

					pMenu->SetTag( ui_window.GetTag() );
					pMenu->ShowAtCursor( UIPopupMenu::ALIGN_DOWN_RIGHT );
				}
			}
		}
	}
	else if ( message.same_no_case( "player_popup_menu_select" ) )
	{
		if ( gServer.IsLocal() )
		{
			UIPopupMenu *pMenu = (UIPopupMenu *)gUIShell.FindUIWindow( "player_options_popup_menu" );
			if ( pMenu )
			{
				PlayerId playerId = MakePlayerId( pMenu->GetTag() );

				switch ( pMenu->GetSelectedElement() )
				{
					case MENU_PLAYER_EJECT:
					{
						SKickPlayer( playerId );
						break;
					}

					case MENU_PLAYER_BAN:
					{
						SBanPlayer( playerId );
						break;
					}
				}
			}
		}
	}
	else if ( message.same_no_case( "change_start_location" ) )
	{
		UIComboBox *pCombo = (UIComboBox *)&ui_window;
		const WorldMap::SGroupColl &startGroups = gWorldMap.GetStartGroups();
		for ( WorldMap::SGroupColl::const_iterator i = startGroups.begin(); i != startGroups.end(); ++i )
		{
			if ( i->sScreenName.same_no_case( pCombo->GetText() ) )
			{
				if( MakePlayerId( pCombo->GetTag() ) != PLAYERID_INVALID )
				{
					Player * pPlayer = gServer.GetPlayer( MakePlayerId( pCombo->GetTag() ) );
					if ( (pPlayer && gServer.IsLocal()) ||
						 (gServer.GetAllowStartLocationSelection() && pPlayer && (pPlayer == gServer.GetLocalHumanPlayer())) )
					{
						if ( !pPlayer->GetStartingGroup().same_no_case( i->sGroup ) &&
							 (pPlayer->GetHeroUberLevel() >= gWorldMap.GetRequiredStartingPositionLevel( i->sGroup )) )
						{
							pPlayer->RSSetStartingGroup( i->sGroup );
						}
					}
				}
				break;
			}		
		}

		UITextBox * pWindow = (UITextBox *)gUIShell.FindUIWindow( "multiplayer_help", "multiplayer_help" );
		if ( pWindow )
		{
			pWindow->SetText( L"" );
		}
	}
	else if ( message.same_no_case( "show_town_description" ) )
	{
		UIListbox * pList = (UIListbox *)&ui_window;

		if ( pList && pList->GetParentWindow() )
		{
			Player * pPlayer = gServer.GetPlayer( MakePlayerId( pList->GetParentWindow()->GetTag() ) );
			if ( pPlayer )
			{
				ShowStartDescription( pPlayer, pList->GetSelectedText() );
			}
		}
	}
	else if ( message.same_no_case( "clear_town_description" ) )
	{
		UITextBox * pWindow = (UITextBox *)gUIShell.FindUIWindow( "multiplayer_help", "multiplayer_help" );
		if ( pWindow )
		{
			pWindow->SetText( L"" );
		}
	}	
	else if ( message.same_no_case( "select_next_team" ) )
	{
		gServer.RSSelectNextTeam( gServer.GetLocalHumanPlayer()->GetId() );
	}
	else if ( message.same_no_case( "select_previous_team" ) )
	{
		gServer.RSSelectNextTeam( gServer.GetLocalHumanPlayer()->GetId(), true );
	}
	else if ( message.same_no_case( "staging_chat_send" ) )
	{
		UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "staging_area_chat_editbox" );
		if ( eb )
		{
			ProcessChatMessage(eb, "staging_area");
		}
	}
	else if ( message.same_no_case( "staging_check_ready" ) )
	{
		if( gServer.GetLocalHumanPlayer() )
		{			
			if ( gServer.GetLocalHumanPlayer()->GetHeroName().empty() || gServer.GetLocalHumanPlayer()->GetHeroCloneSourceTemplate().empty() )
			{
				ShowMessageBox( gpwtranslate( $MSG$ "Please select a character first." ) );

				UICheckbox * pCheck = (UICheckbox *)gUIShell.FindUIWindow( "staging_checkbox_ready" );
				if ( pCheck )
				{
					pCheck->SetState( false );
				}

				return;
			}

			float requiredLevel = 0.0f;

			if ( CanCharacterJoinGame( requiredLevel ) )
			{
				UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( gpstringf( "button_select_char%d", m_ActivePlayerSlot ) );
				if ( pButton )
				{
					pButton->SetVisible( false );
				}

				UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( gpstringf( "combo_start_locations%d", m_ActivePlayerSlot ) );
				if ( pCombo )
				{
					pCombo->SetVisible( false );
				}
				UIWindow *pWindow = gUIShell.FindUIWindow( gpstringf( "start_location%d_bg", m_ActivePlayerSlot ) );
				if ( pWindow )
				{
					pWindow->SetVisible( false );
				}

				pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_sign", m_ActivePlayerSlot ), "staging_area_players" );
				if ( pWindow )
				{
					pWindow->SetEnabled( false );
				}

				if ( !gServer.GetLocalHumanPlayer()->IsReadyToPlay() )
				{
					gpwstring messageText;
					messageText.assignf( gpwtranslate( $MSG$ "%s is ready to begin" ), gServer.GetLocalHumanPlayer()->GetName().c_str() );

					if ( m_TimeToShowPendingMessage > 0 )
					{
						m_PendingMessageText = messageText;
					}
					else
					{
						RSDisplayMessage( messageText, STAGING_AREA_STATUS_MSG_COLOR );
						m_TimeToShowPendingMessage = 2.0f;
					}

					gServer.GetLocalHumanPlayer()->RSSetReadyToPlay( true );
				}
			}
			else
			{
				gpwstring sText;
				sText.assignf( gpwtranslate( $MSG$ "Your character needs to be at least level %0.0f to play in this world." ), requiredLevel );
				ShowMessageBox( sText );

				UICheckbox * pCheck = (UICheckbox *)gUIShell.FindUIWindow( "staging_checkbox_ready" );
				if ( pCheck )
				{
					pCheck->SetState( false );
				}
			}
		}
	}
	else if ( message.same_no_case( "staging_uncheck_ready" ) )
	{
		if( gServer.GetLocalHumanPlayer() )
		{
			UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( gpstringf( "button_select_char%d", m_ActivePlayerSlot ) );
			if ( pButton )
			{
				pButton->SetVisible( true );
			}

			UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( gpstringf( "combo_start_locations%d", m_ActivePlayerSlot ) );
			if ( pCombo )
			{
				pCombo->SetVisible( true );
			}
			UIWindow *pWindow = gUIShell.FindUIWindow( gpstringf( "start_location%d_bg", m_ActivePlayerSlot ) );
			if ( pWindow )
			{
				pWindow->SetVisible( true );
			}

			pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_sign", m_ActivePlayerSlot ), "staging_area_players" );
			if ( pWindow )
			{
				pWindow->SetEnabled( true );
			}

			if ( gServer.GetLocalHumanPlayer()->IsReadyToPlay() )
			{
				gpwstring messageText;
				messageText.assignf( gpwtranslate( $MSG$ "%s is not ready to begin" ), gServer.GetLocalHumanPlayer()->GetName().c_str() );

				if ( m_TimeToShowPendingMessage > 0 )
				{
					m_PendingMessageText = messageText;
				}
				else
				{
					RSDisplayMessage( messageText, STAGING_AREA_STATUS_MSG_COLOR );
					m_TimeToShowPendingMessage = 2.0f;
				}

				gServer.GetLocalHumanPlayer()->RSSetReadyToPlay( false );
			}
		}
	}
	else if ( message.same_no_case( "staging_leave_game" ) )
	{
		if( gServer.GetLocalHumanPlayer() )
		{
			RSLeaveStagingArea( gServer.GetLocalHumanPlayer()->GetId() );
		}
	}
	else if ( message.same_no_case( "staging_start_game" ) )
	{
		if ( gServer.IsLocal() )
		{
			if ( !gWorldMap.IsInitialized() )
			{
				ShowMessageBox( gpwtranslate( $MSG$ "You must choose a map before you can start the game." ) );
				return;
			}

			if ( gServer.GetLocalHumanPlayer() && gServer.GetLocalHumanPlayer()->GetHeroName().empty() )
			{
				ShowMessageBox( gpwtranslate( $MSG$ "You must select a character before you can start the game." ) );
				return;
			}

			float requiredLevel = 0.0f;
			bool bAllowCharacter = CanCharacterJoinGame( requiredLevel );
			if ( !bAllowCharacter )
			{
				gpwstring sText;
				sText.assignf( gpwtranslate( $MSG$ "Your character needs to be at least level %0.0f to play in this world." ), requiredLevel );
				ShowMessageBox( sText );
				return;
			}

			bool ready = true;
			Server::PlayerColl::const_iterator i;
			for ( i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
			{				
				if ( IsValid( *i ) && !(*i)->IsComputerPlayer() && !(*i)->IsReadyToPlay() )
				{
					ready = false;
					RCDisplayMessage( gpwtranslate( $MSG$ "The host is ready to begin. Please ensure you have clicked \"I'm Ready to Begin\"." ), STAGING_AREA_STATUS_MSG_COLOR, (*i)->GetMachineId() );
				}
			}

			if ( !ready )
			{
				ShowMessageBox( gpwtranslate( $MSG$ "All players must click \"I'm Ready to Begin\" before you can start the game." ) );
				return;
			}

			if( gWorldMap.GetNumStartingPositionsAssigned() < gServer.GetNumHumanPlayers() )
			{
#if !GP_RETAIL
				gperror( "Not enough starting positions in this map.  Can't load map." );
#endif
				return;
			}

			// Head into game
			SStartNetworkGame();
		}
		else
		{
			gpgeneric( "You are lowly client. You don't have the power to start the game.\n" );
		}
	}
	else if ( message.same_no_case( "staging_area_refresh_listbox_players" ) )
	{
		UpdatePlayerListBox();
	}
	else if ( message.same_no_case( "rename_selected_character" ) )
	{
		ShowMultiplayerEditDialog( gpwtranslate( $MSG$ "Rename character to:" ), MP_EDIT_DIALOG_RENAME );			
	}
	else if ( message.same_no_case( "copy_selected_character" ) )
	{
		ShowMultiplayerEditDialog( gpwtranslate( $MSG$ "Enter name for copied character:" ), MP_EDIT_DIALOG_COPY );		
	}
	else if ( message.same_no_case( "mp_edit_dialog_ok" ) )
	{
		UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "edit_box_mp_dialog_text", "mp_edit_dialog" );
		switch ( m_MultiplayerEditDialog )
		{
			case MP_EDIT_DIALOG_RENAME:
			{
				CopyOrRenameCharacter( pEditBox->GetText(), true );								
			}
			break;

			case MP_EDIT_DIALOG_COPY:
			{				
				CopyOrRenameCharacter( pEditBox->GetText(), false );					
			}
			break;
		}
	}
	else if ( message.same_no_case( "mp_edit_dialog_cancel" ) )
	{		
		HideMultiplayerEditDialog();
	}
}


void UIMultiplayer::EndGameGameGUICallback( gpstring const & message, UIWindow & /*ui_window*/ )
{
	if ( message.same_no_case( "end_game_view_kills" ) )
	{
		gpstring screenType = "kills";
		ShowEndGameRankings( screenType );
	}
	else if ( message.same_no_case( "end_game_view_experience" ) )
	{
		gpstring screenType = "experience";
		ShowEndGameRankings( screenType );
	}
	else if ( message.same_no_case( "end_game_view_loot" ) )
	{
		gpstring screenType = "loot" ;
		ShowEndGameRankings( screenType );
	}
	else if ( message.same_no_case( "end_game_view_timeline" ) )
	{
		gUIShell.HideInterface( "end_game_summary_stats" );
		gUIShell.ShowInterface( "end_game_summary_timeline" );
		gUIShell.ShowGroup("end_game_player_only", true);
		gUIShell.ShowGroup("end_game_class", false);
		
		UIRadioButton *radioButton = (UIRadioButton*)gUIShell.FindUIWindow( "details_radio1" );
		if ( radioButton )
		{
			radioButton->SetCheck( true );
		}
	}
	else if ( message.same_no_case( "end_game_view_stats" ) )
	{
		gUIShell.ShowInterface( "end_game_summary_stats" );
		gUIShell.HideInterface( "end_game_summary_timeline" );
		gUIShell.ShowGroup("end_game_player_only", false);
		gUIShell.ShowGroup("end_game_class", true);

	}
	else if ( message.same_no_case( "end_game_details_change" ) )
	{
		int playerSlot = 1;
		for ( Server::PlayerColl::const_iterator player = gServer.GetPlayers().begin(); player != gServer.GetPlayers().end(); ++player )
		{
			if ( IsValid( *player ) && !(*player)->IsComputerPlayer() && !(*player)->IsMarkedForDeletion() )
			{
				RefreshEndGameDetails( playerSlot, (*player)->GetId() );
			}
			else
			{
				UILine * pUILine = (UILine *) gUIShell.FindUIWindow( gpstringf( "timeline_player%d", playerSlot ), "end_game_summary_timeline" );
				if( pUILine )
				{
					pUILine->SetVisible( false );
				}
			}
			++playerSlot;
		}
	}
	else if ( message.same_no_case( "end_game_chat_send" ) )
	{
		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "end_game_chat_editbox" );
		if ( editBox )
		{
			ProcessChatMessage( editBox, "end_game_summary" );
		}
	}
	else if ( message.same_no_case( "end_game_staging_return" ) )
	{
		// $$$ temporarily disabling return to staging area

		/*
		gUIShell.GetMessenger()->UnRegisterCallback( makeFunctor( *this, &UIMultiplayer::EndGameGameGUICallback ) );

		if ( gServer.IsLocal() )
		{
			gWorldStateRequest( WS_MP_STAGING_AREA_SERVER );
			RCEndGameActivateStagingReturn();
		}
		else
		{
			gWorldStateRequest( WS_MP_STAGING_AREA_CLIENT );
		}
		*/
	}
	else if ( message.same_no_case( "end_game_close" ) )
	{
		gUIShell.GetMessenger()->UnRegisterCallback( makeFunctor( *this, &UIMultiplayer::EndGameGameGUICallback ) );

		if ( GetMultiplayerMode() == MM_GUN )
		{
			gWorldStateRequest( WS_MP_MATCH_MAKER );
		}
		else
		{
			HideInterfaces();
			SetIsVisible( false );

			gWorldStateRequest( WS_MAIN_MENU );
		}
	}
	else if ( message.same_no_case( "show_end_game_close_helptext" ) )
	{
		UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "multiplayer_help", "multiplayer_help" );
		if ( pTextBox )
		{
			if ( GetMultiplayerMode() == MM_GUN )
			{
				pTextBox->SetText( gpwtranslate( $MSG$ "Close the Game Summary screen and return to ZoneMatch" ) );
			}
			else
			{
				pTextBox->SetText( gpwtranslate( $MSG$ "Close the Game Summary screen and return to the Main Menu" ) );
			}
		}
	}
	else if ( message.same_no_case( "hide_end_game_close_helptext" ) )
	{
		UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "multiplayer_help", "multiplayer_help" );
		if ( pTextBox )
		{
			pTextBox->SetText( L"" );
		}
	}
	else if ( message.same_no_case( "end_game_summary_refresh_listbox_players" ) )
	{
		UpdatePlayerListBox();
	}
}


void UIMultiplayer::ActivateInterfaces()
{
	if ( m_bIsActivated )
	{
		return;
	}

	m_bIsActivated = true;
	m_bIsInitialized = false;

	// Activate multiplayer interfaces
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:background",                    false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:internet_game_menu",            false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:lan_game_menu",                 false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:enter_password",                false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:host_game",                     false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:staging_area",                  false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:staging_area_players",          false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:staging_area_select_team",      false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:staging_area_map_settings",     false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:staging_area_game_settings",    false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:staging_area_change_password",  false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:staging_area_select_character", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:staging_area_create_character", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:staging_area_import_character", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:multiplayer_help",              false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:mp_edit_dialog",                false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:mp_dialog",                     false );	
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:message_box_large",             false );

	// Force immediate interface activation
	gUIShell.Update( 0.0f, gAppModule.GetCursorX(), gAppModule.GetCursorY() );

	gUIShell.GetMessenger()->RegisterCallback( makeFunctor( *this, &UIMultiplayer::GameGUICallback ) );

	m_UIMMultiplayerMenuList.clear();
	m_UIMMultiplayerMenuList.push_back( L"-" );
	m_UIMMultiplayerMenuList.push_back( gpwtranslate( $MSG$ "Eject Player" ) );
	m_UIMMultiplayerMenuList.push_back( gpwtranslate( $MSG$ "Eject and Ban Player" ) );
}


void UIMultiplayer::DeactivateInterfaces()
{
	m_bIsActivated = false;
	m_bIsInitialized = false;
	m_bShowDialog = false;

	SetIsVisible( false );
	HideInterfaces();

	for ( int i = 1; i <= STAGING_PLAYER_SLOTS; ++i )
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_sign", i ), "staging_area_players" );
		if ( pWindow )
		{
			pWindow->SetTextureIndex( 0 );
		}
	}

	for ( i = 1; i <= STAGING_CHARACTER_SLOTS; ++i )
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "character%d_portrait", i ), "staging_area_select_character" );
		if ( pWindow )
		{
			pWindow->SetTextureIndex( 0 );
		}
	}

	for ( i = 1; i <= STAGING_CHARACTER_SLOTS; ++i )
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "player%dteam", i ), "end_game_summary" );
		if ( pWindow )
		{
			pWindow->SetTextureIndex( 0 );
		}
	}	

	gUIShell.MarkInterfaceForDeactivation( "background"                    );
	gUIShell.MarkInterfaceForDeactivation( "enter_password"                );
	gUIShell.MarkInterfaceForDeactivation( "host_game"                     );
	gUIShell.MarkInterfaceForDeactivation( "internet_game_menu"            );
	gUIShell.MarkInterfaceForDeactivation( "lan_game_menu"                 );
	gUIShell.MarkInterfaceForDeactivation( "mp_edit_dialog"				   );
	gUIShell.MarkInterfaceForDeactivation( "mp_dialog"                     );	
	gUIShell.MarkInterfaceForDeactivation( "multiplayer_help"              );
	gUIShell.MarkInterfaceForDeactivation( "staging_area"                  );
	gUIShell.MarkInterfaceForDeactivation( "staging_area_create_character" );
	gUIShell.MarkInterfaceForDeactivation( "staging_area_game_settings"    );
	gUIShell.MarkInterfaceForDeactivation( "staging_area_change_password"  );
	gUIShell.MarkInterfaceForDeactivation( "staging_area_import_character" );
	gUIShell.MarkInterfaceForDeactivation( "staging_area_map_settings"     );
	gUIShell.MarkInterfaceForDeactivation( "staging_area_players"          );
	gUIShell.MarkInterfaceForDeactivation( "staging_area_select_character" );
	gUIShell.MarkInterfaceForDeactivation( "staging_area_select_team"      );
	gUIShell.MarkInterfaceForDeactivation( "end_game_summary"              );
	gUIShell.MarkInterfaceForDeactivation( "end_game_summary_stats"        );
	gUIShell.MarkInterfaceForDeactivation( "end_game_summary_timeline"     );
	gUIShell.MarkInterfaceForDeactivation( "message_box_large"			   );	

	gUIShell.GetMessenger()->UnRegisterCallback( makeFunctor( *this, &UIMultiplayer::GameGUICallback ) );

	m_MapInfoColl.clear();
	m_saveInfoColl.clear();

	gUIZoneMatch.Deactivate();
}


void UIMultiplayer::HideInterfaces()
{
	gUIShell.HideInterface( "background"                    );
	gUIShell.HideInterface( "enter_password"                );
	gUIShell.HideInterface( "host_game"                     );
	gUIShell.HideInterface( "internet_game_menu"            );
	gUIShell.HideInterface( "lan_game_menu"                 );
	gUIShell.HideInterface( "multiplayer_help"              );
	gUIShell.HideInterface( "staging_area"                  );
	gUIShell.HideInterface( "staging_area_create_character" );
	gUIShell.HideInterface( "staging_area_game_settings"    );
	gUIShell.HideInterface( "staging_area_change_password"  );
	gUIShell.HideInterface( "staging_area_import_character" );
	gUIShell.HideInterface( "staging_area_map_settings"     );
	gUIShell.HideInterface( "staging_area_players"          );
	gUIShell.HideInterface( "staging_area_select_character" );
	gUIShell.HideInterface( "staging_area_select_team"      );
	gUIShell.HideInterface( "end_game_summary"              );
	gUIShell.HideInterface( "end_game_summary_stats"        );
	gUIShell.HideInterface( "end_game_summary_timeline"     );
	gUIShell.HideInterface( "zonematch"                     );
	gUIShell.HideInterface( "zonematch_panel"               );	
	gUIShell.HideInterface( "zonematch_account_settings"    );
	gUIShell.HideInterface( "zonematch_chat"                );
	gUIShell.HideInterface( "zonematch_create_chatroom"     );
	gUIShell.HideInterface( "zonematch_game_details"        );
	gUIShell.HideInterface( "zonematch_games"               );
	gUIShell.HideInterface( "zonematch_hostgame"            );
	gUIShell.HideInterface( "zonematch_news"                );
	gUIShell.HideInterface( "zonematch_signin"              );
//	gUIShell.HideInterface( "mp_dialog" );

	if ( IsVisible() )
	{
//		if ( gWorldState.GetCurrentState() != WS_MP_MATCH_MAKER )
		{
			gUIShell.ShowInterface( "background" );
		}
		gUIShell.ShowInterface( "multiplayer_help" );
	}
}


void UIMultiplayer::HideStagingAreaInterfaces()
{
	gUIShell.HideInterface( "staging_area_create_character" );
	gUIShell.HideInterface( "staging_area_game_settings"    );
	gUIShell.HideInterface( "staging_area_change_password"  );
	gUIShell.HideInterface( "staging_area_import_character" );
	gUIShell.HideInterface( "staging_area_map_settings"     );
	gUIShell.HideInterface( "staging_area_players"          );
	gUIShell.HideInterface( "staging_area_select_character" );
}


void UIMultiplayer::InitInterfaceElements()
{
	if( !m_bIsInitialized )
	{
		// Lan

		UIListReport *listreport = (UIListReport *)gUIShell.FindUIWindow( "lan_games_list" );
		if( listreport ) 
		{			
			listreport->InsertColumn( gpwtranslate( $MSG$ "Game" ),  200 );
			listreport->InsertColumn( gpwtranslate( $MSG$ "Players" ), 75 );
			listreport->InsertColumn( gpwtranslate( $MSG$ "Ping" ), 75 );
			listreport->InsertColumn( gpwtranslate( $MSG$ "In Progress" ), 100 );
		}

		// Staging Area

		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "listbox_player_limit" );
		if ( listBox )
		{
			listBox->RemoveAllElements();
			for ( int i = 2; i <= Server::MAX_PLAYERS; ++i )
			{
				listBox->AddElement( gpwstringf( gpwtranslate( $MSG$ "%d Players" ), i ), i );
			}
		}

		listBox = (UIListbox *)gUIShell.FindUIWindow( "listbox_difficulty" );
		if ( listBox )
		{
			listBox->RemoveAllElements();
			listBox->AddElement( gpwtranslate( $MSG$ "Easy" ) );
			listBox->AddElement( gpwtranslate( $MSG$ "Normal" ) );
			listBox->AddElement( gpwtranslate( $MSG$ "Hard" ) );
		}

		listBox = (UIListbox *)gUIShell.FindUIWindow( "listbox_time_limit" );
		if ( listBox )
		{
			listBox->RemoveAllElements();
			listBox->AddElement( gpwtranslate( $MSG$ "None" ), 0 );			

			FastFuelHandle hTimerSettings( "config:multiplayer:game_timer" );
			if ( hTimerSettings )
			{
				FastFuelFindHandle hFind( hTimerSettings );
				if ( hFind.FindFirstValue( "*" ) )
				{
					int time = 0;
					while ( hFind.GetNextValue( time ) )
					{
						listBox->AddElement( ConstructTimeString( time ), time );						
					}
				}
			}
		}

		listBox = (UIListbox *)gUIShell.FindUIWindow( "listbox_characters_drop" );
		if ( listBox )
		{
			listBox->RemoveAllElements();
			listBox->AddElement( gpwtranslate( $MSG$ "Nothing" ) );
			listBox->AddElement( gpwtranslate( $MSG$ "Equipped Items" ) );
			listBox->AddElement( gpwtranslate( $MSG$ "Inventory" ) );
			listBox->AddElement( gpwtranslate( $MSG$ "All" ) );
		}

		listBox = (UIListbox *)gUIShell.FindUIWindow( "listbox_character_respawn" );
		if ( listBox )
		{
			listBox->RemoveAllElements();			
			listBox->AddElement( gpwtranslate( $MSG$ "Respawn at Corpse" ) );
			listBox->AddElement( gpwtranslate( $MSG$ "No Respawn" ) );
		}
	}
}


void UIMultiplayer::Update( double secondsElapsed )
{
	m_fEatEnter = false;
	
	m_DialogInput = DIALOG_INPUT_NONE;

	if ( m_bShowDialog )
	{
		if ( gUIZoneMatch.IsDialogPending() || gUIShell.IsInterfaceVisible( "zonematch_dialog" ) )
		{
			// make sure to still update ZM so we can close the dialoge we are waiting for!
			if( !gUI.UpdateShell( secondsElapsed, gAppModule.GetCursorX(), gAppModule.GetCursorY() ) )
			{
				return;
			}

			if ( m_MultiplayerMode == MM_GUN )
			{
				m_pUIZoneMatch->Update( secondsElapsed );
			}

			return;
		}

		gUIShell.ShowInterface( "mp_dialog" );
		m_bShowDialog = false;
	}

	if( !gUI.UpdateShell( secondsElapsed, gAppModule.GetCursorX(), gAppModule.GetCursorY() ) )
	{
		return;
	}

	if ( m_bActivateEndGameScreen )
	{
		ShowEndGameScreen();
		m_bActivateEndGameScreen = false;
	}

	if ( !m_bIsInitialized )
	{
		InitInterfaceElements();
		m_bIsInitialized = true;
	}

	if ( m_MultiplayerMode == MM_GUN )
	{
		m_pUIZoneMatch->Update( secondsElapsed );
	}

	if ( gUIShell.IsInterfaceVisible( "staging_area_create_character" ) )
	{

		UIText *text = (UIText *)gUIShell.FindUIWindow( "create_new_character_warning" );
		if( text )
		{
			gpassert( Server::DoesSingletonExist() != NULL );
			if( gServer.GetAllowNewCharactersOnly() )
			{
				text->SetVisible( true );
			}
			else
			{
				text->SetVisible( false );
			}
		}

		m_pCharacterSelect->Update( secondsElapsed );

		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_char_name_edit_box" );
		if ( editBox )
		{
			gpwstring sName = editBox->GetText();
			SETENABLED(( "staging_create_character_ok" ), gUIFrontend.IsNameValid( sName ) );
		}
	}

	switch ( gWorldState.GetCurrentState() )
	{
		case WS_MP_LAN_GAME:
		{
			if ( GetGlobalSeconds() - m_LanLastUpdateTime > 0.5 )
			{
				gNetPipe.SetIsSessionCollDirty( true );
				m_LanLastUpdateTime = GetGlobalSeconds();
			}

			if( gNetPipe.IsSessionCollDirty() )
			{
				DisplayLanGamesList();
				gNetPipe.SetIsSessionCollDirty( false );
			}

			// $$ failsafe - make sure we're always enumerating sessions - this will early out if we're already enumerating
			gNetPipe.RequestSessionEnumeration();

			break;
		}

		case WS_MP_STAGING_AREA_CLIENT:
		case WS_MP_STAGING_AREA_SERVER:
		{
			gNetPipe.StopSessionEnumeration();

			if ( m_pCharacterSelect && !m_pCharacterSelect->IsInitialized() )
			{
				m_pCharacterSelect->Initialize();
			}

			// display any pending messages
			if ( m_TimeToShowPendingMessage > 0 )
			{
				m_TimeToShowPendingMessage -= secondsElapsed;
				if ( (m_TimeToShowPendingMessage < 0) && !m_PendingMessageText.empty() )
				{
					RSDisplayMessage( m_PendingMessageText, STAGING_AREA_STATUS_MSG_COLOR );
					m_PendingMessageText.clear();
				}
			}

			if ( gServer.GetSettingsDirty() )
			{
				// Set players as not ready when settings are changed
				if ( gServer.IsLocal() )
				{
					for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
					{
						if ( IsValid( *i ) && !(*i)->IsComputerPlayer() && !(*i)->IsMarkedForDeletion() && (*i != gServer.GetLocalHumanPlayer()) )
						{
							(*i)->RSSetReadyToPlay( false );
						}
					}
				}
				else
				{
					UICheckbox * pCheck = (UICheckbox *)gUIShell.FindUIWindow( "staging_checkbox_ready" );
					if ( pCheck )
					{
						pCheck->SetState( false );
					}
				}

				if ( gServer.GetAllowNewCharactersOnly() )
				{
					if ( gServer.GetLocalHumanPlayer() &&
						 IsCharacterImported() )
					{
						m_SelectedCharacter = -1;

						gServer.GetLocalHumanPlayer()->RSSetHeroCloneSourceTemplate( "" );
						gServer.GetLocalHumanPlayer()->RSSetStashCloneSourceTemplate( "" );
						gServer.GetLocalHumanPlayer()->RSSetHeroName( L"" );
						gServer.GetLocalHumanPlayer()->RSSetHeroUberLevel( 0.0f );

						if ( !gServer.IsLocal() )
						{
							ShowMessageBox( gpwtranslate( $MSG$ "The host has chosen to only allow new characters. You must create a new character before you can join this game." ) );

							UICheckbox * pCheck = (UICheckbox *)gUIShell.FindUIWindow( "staging_checkbox_ready" );
							if ( pCheck )
							{
								pCheck->SetState( false );
							}
						}
					}

					if ( gUIShell.IsInterfaceVisible( "staging_area_import_character" ) || gUIShell.IsInterfaceVisible( "staging_area_select_character" ) )
					{
						StagingAreaEnableSelections( true );

						gUIShell.HideInterface( "staging_area_import_character" );
						gUIShell.HideInterface( "staging_area_select_character" );
						gUIShell.ShowInterface( "staging_area_players" );
					}
				}

				ShowGameSettingsList();
				StagingAreaUpdatePlayerList();
			}

			// Update pings at said interval
			static double StagingUpdateTime = 0;
			StagingUpdateTime += secondsElapsed;
			if ( StagingUpdateTime > 2 )
			{
				StagingUpdateTime = 0;
				StagingAreaUpdatePings();
			}

			// Update any players with dirty flags
			int PlayerNumber = 1;
			for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
			{
				if ( IsValid( *i ) && !(*i)->IsComputerPlayer() && !(*i)->IsMarkedForDeletion() )
				{
					if ( (*i)->GetIsDirty() )
					{
						StagingAreaUpdatePlayer( PlayerNumber, *i );
					}

					if ( IsInGame( (*i)->GetWorldState() ) )
					{
						UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_ingame", PlayerNumber ) );
						if ( pWindow )
						{
							pWindow->SetHasTexture( (gWorldTime.GetRealTime() - (int)gWorldTime.GetRealTime()) > 0.5f );
						}
					}

					++PlayerNumber;
				}
			}

			if ( gWorldMap.IsInitialized() && !m_SelectedMap.same_no_case( gWorldMap.GetMapName() ) )
			{
				m_SelectedMap = gWorldMap.GetMapName();
				if( IsGunMM() && gUIZoneMatch.IsHosting() )
				{
					gUIZoneMatch.SetHostWorldFlags();
				}
			}

			if ( m_bPlayersDirty )
			{
//				StagingAreaUpdatePlayerList();			
				// update our player chat list
				//$$$ now done on the combo up UpdatePlayerListBox();
				
				m_bPlayersDirty = false;
			}
			break;
		}
	}

	switch ( m_OnNextUpdate )
	{
		case NU_FIND_GAME:
		{
			m_FindSessionTimeout += secondsElapsed;
			if ( m_FindSessionTimeout > m_SessionTimeout ) 
			{
				ShowMessageBox( gpwtranslate( $MSG$ "No games were found at the specified address." ) );
				OnNextUpdate( NU_NONE );
			}
			else
			{
				if ( !gNetPipe.GetSessions().empty() )
				{
					HideMessageBox();

					if ( gNetPipe.GetSessions().front()->GetHasPassword() )
					{
						gUIShell.ShowInterface( "enter_password" );

						UIEditBox *pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "enter_password_edit_box" );
						if ( pEditBox )
						{
							pEditBox->SetText( L"" );
							pEditBox->SetAllowInput( true );
						}

						m_JoinSessionLocalId = gNetPipe.GetFrontSession()->GetLocalId();

						OnNextUpdate( NU_NONE );
					}
					else
					{
						NetPipeSessionHandle session = gNetPipe.GetFrontSession();
						Connect( session, L"" );
					}
				}
				else
				{
					gpwstring message = gpwtranslate( $MSG$ "Looking for game..." );
					message.appendf( L" (%d)", (int) (m_SessionTimeout - m_FindSessionTimeout) );
					ShowMessageBox( message, MP_DIALOG_CANCEL_FIND );
				}
			}


			break;
		}
		// enable when bart gets back and we ahve an alternative to FindSessionAtIp
		//case NU_CONNECTING:
		//{
			// check to make sure we still have a valid game
			// if we don't then early out! if not, wait some more for an answer.
			//HRESULT hr = gNetPipe.FindSessionAtIp( ToUnicode(m_JoiningIP) );		
			//if ( FAILED(hr) )
			//{
			//	HideMessageBox();
			//	ShowMessageBox( gpwtranslate( $MSG$ "Game is no longer active." ) );
			//	gNetPipe.CancelConnect();
			//	OnNextUpdate( NU_NONE );
			//}
		//	break;
		//}
		
		case NU_JOIN_GAME:
		{
			NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

			if( !session )
			{
				ShowMessageBox( gpwtranslate( $MSG$ "No games were found at the specified address." ) );
				OnNextUpdate( NU_NONE );
			}
			else if ( CompleteJoinGame() )
			{
				if ( m_OnNextUpdate == NU_JOIN_GAME )
				{
					OnNextUpdate( NU_NONE );
				}
			}
			break;
		}

		case NU_SHOW_MULTIPLAYER_END_GAME:
		{
			ShowEndGameScreen();
			
			OnNextUpdate( NU_NONE );
			break;
		}
	}	
}


void UIMultiplayer::Draw( double /*seconds_elapsed*/ )
{
	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZENABLE, FALSE );

	// Draw the user interface
	gUIShell.Draw();	

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZENABLE, TRUE );

	if ( gUIShell.IsInterfaceVisible( "staging_area_create_character" ) )
	{
		m_pCharacterSelect->Draw();
	}

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZENABLE, FALSE );

	gUIShell.DrawItems();

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZENABLE, TRUE );

	if ( (gWorldState.GetCurrentState() == WS_GAME_ENDED ) &&
		 gUIShell.IsInterfaceVisible( "end_game_summary_timeline" ) )
	{
		DrawEndGameTimeline();
	}
}


void UIMultiplayer::HandleBroadcast( WorldMessage & msg )
{
	if( ( msg.GetEvent() == WE_PLAYER_DATA_CHANGED ) || 
		( msg.GetEvent() == WE_MP_PLAYER_CREATED ) || 
		( msg.GetEvent() == WE_MP_PLAYER_DESTROYED ) )
	{
		if ( gUIZoneMatch.IsHosting() )
		{
			gUIZoneMatch.HostRefreshPlayerList();
		}

		if ( IsInStagingArea( gWorldState.GetCurrentState() ) )
		{
			StagingAreaUpdatePlayerList();
		}

		if ( gServer.IsLocal() && (IsInStagingArea( gWorldState.GetCurrentState() ) || IsEndGame( gWorldState.GetCurrentState() )) )
		{
			Player * player = gServer.GetPlayer( MakePlayerId( msg.GetData1() ) );
			if ( player && (player != gServer.GetLocalHumanPlayer()) )
			{
				for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
				{
					if ( IsValid( *i ) && !(*i)->IsComputerPlayer() && (*i != player) )
					{
						if ( msg.GetEvent() == WE_MP_PLAYER_CREATED )
						{
							RCDisplayMessage( gpwstringf( gpwtranslate( $MSG$ "%s has entered the game." ), player->GetName().c_str() ), STAGING_AREA_STATUS_MSG_COLOR, (*i)->GetMachineId() );
						}
						else if ( msg.GetEvent() == WE_MP_PLAYER_DESTROYED )
						{
							RCDisplayMessage( gpwstringf( gpwtranslate( $MSG$ "%s has left the game." ), player->GetName().c_str() ), STAGING_AREA_STATUS_MSG_COLOR, (*i)->GetMachineId() );
						}
					}
				}
			}
		}

		m_bPlayersDirty = true;
	}

	if( msg.GetEvent() == WE_MP_SESSION_CREATED )
	{
		gpassert( gServer.IsLocal() );
		gpassert( gServer.GetLocalHumanPlayer() == NULL );

		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
		gpassert( session );

		// if the server has connected, ask it for a player
		if( session )
		{
//			gpassert( (gWorldState.GetCurrentState() == WS_MP_LAN_GAME) ||
//					  (gWorldState.GetCurrentState() == WS_MP_INTERNET_GAME) ||
//					  (gWorldState.GetCurrentState() == WS_MP_MATCH_MAKER)	);

			// create server's local human player
			gpwstring playerName = (GetMultiplayerMode() == MM_GUN) ? gUIZoneMatch.GetPlayerName() : m_PlayerName;
			gServer.RSCreatePlayer( playerName.c_str(), PC_HUMAN );
		}
	}
	else if( msg.GetEvent() == WE_MP_SESSION_CONNECTED )
	{
//			gpassert( (gWorldState.GetCurrentState() == WS_MP_LAN_GAME) ||
//					  (gWorldState.GetCurrentState() == WS_MP_INTERNET_GAME) ||
//					  (gWorldState.GetCurrentState() == WS_MP_MATCH_MAKER) );
		eNetPipeEvent hr = (eNetPipeEvent)msg.GetData1();
		HideMessageBox();

		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

		if ( session )
		{
			// ask server for player
			gpwstring playerName = (GetMultiplayerMode() == MM_GUN) ? gUIZoneMatch.GetPlayerName() : m_PlayerName;
			gServer.RSCreatePlayer( playerName.c_str(), PC_HUMAN );

			OnNextUpdate( NU_JOIN_GAME );
		}
		else
		{
			ShowFailedToJoinMessage( hr );
			OnNextUpdate( NU_NONE );
		}
	}
	else if( msg.GetEvent() == WE_MP_SESSION_TERMINATED )
	{
		if ( !m_bRequestedLeaveGame )
		{
			m_bWasDisconnected = true;
			m_DisconnectInfo = eNetPipeEvent( msg.GetData1() );
			ShowDisconnectedMessage();
		}
	}
	else if( msg.GetEvent() == WE_MP_PLAYER_CREATED )
	{
		Player * player = gServer.GetPlayer( MakePlayerId( msg.GetData1() ) );
		if( gServer.IsLocal() && player->IsComputerPlayer() )
		{
			// special case... this player is always created locally and before we're really in the staging area
			return;
		}

		if ( player->IsLocalHumanPlayer() )
		{
			player->RSSetIsOnZone( ::IsOnZone() );

			LoadPlayerSettings();

			// players playing on server always start ready
			if ( gServer.IsLocal() )
			{
				player->RSSetReadyToPlay( true );
			}

			// set the uber level for characters imported/selected/created for ZM game
			if( IsGunMM() )
			{
				gUIZoneMatch.UpdateHostUberLevelFlags();
			}
		}
		else if ( gServer.IsLocal() )
		{
			if ( !gServer.GetAllowStartLocationSelection() )
			{
				player->RSSetStartingGroup( gServer.GetLocalHumanPlayer()->GetStartingGroup() );
			}

			// make sure to sync joining player's pause state
			if( gTattooGame.IsPaused() )
			{
				gTattooGame.SPause( true );
			}
		}

	}
	else if ( msg.GetEvent() ==	 WE_MP_PLAYER_SET_CHARACTER_LEVEL )
	{
		if ( IsInStagingArea( gWorldState.GetCurrentState() ) )
		{
			Player * player = gServer.GetPlayer( MakePlayerId( msg.GetData1() ) );
			RefreshStartingGroups( player );
		}

		// set the uber level for characters imported/selected/created for ZM game
		if( IsGunMM() )
		{
			gUIZoneMatch.UpdateHostUberLevelFlags();
		}
	}
	else if ( msg.GetEvent() == WE_MP_SET_MAP )
	{
		// create list of game types for the map
		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "listbox_game_type" );
		if ( listBox )
		{
			listBox->RemoveAllElements();

			const Victory::GameTypeColl &gameTypes = gVictory.GetMapGameTypes();

			Victory::GameTypeColl::const_iterator i = gameTypes.begin();
			for ( ; i != gameTypes.end(); ++i )
			{
				listBox->AddElement( (*i)->GetScreenName(), (*i)->GetId() );
			}
		}

		if ( IsInStagingArea( gWorldState.GetCurrentState() ) )
		{
			LoadPlayerStartLocationPref();
			RefreshStartingGroups();
		}

		ShowGameSettingsList( true );
	}
	else if ( msg.GetEvent() == WE_MP_SET_MAP_WORLD )
	{
		float requiredLevel = 0;

		if ( gWorldMap.IsInitialized() && !gWorldMap.GetMpWorldName().empty() )
		{
			MpWorldColl mpWorlds;
			gWorldMap.GetMpWorlds( mpWorlds );

			for ( MpWorldColl::iterator i = mpWorlds.begin(); i != mpWorlds.end(); ++i )
			{
				if ( i->m_Name.same_no_case( gWorldMap.GetMpWorldName() ) )
				{
					requiredLevel = i->m_RequiredLevel;
					break;
				}
			}
		}

		SETENABLED(( "staging_character_create" ), requiredLevel == 0 );

		StagingAreaUpdatePlayerList();

		// check that selected character can enter this world
		if ( gServer.GetLocalHumanPlayer() )
		{
			if ( !DoesCharacterMeetWorldLevel( gServer.GetLocalHumanPlayer()->GetHeroUberLevel() ) && gServer.GetLocalHumanPlayer()->IsReadyToPlay() )
			{
				if ( !gServer.IsLocal() )
				{
					UICheckbox * pCheck = (UICheckbox *)gUIShell.FindUIWindow( "staging_checkbox_ready" );
					if ( pCheck )
					{
						pCheck->SetState( false );
					}
				}
			}
		}

		if ( IsInStagingArea( gWorldState.GetCurrentState() ) )
		{
			// update the start location listing
			RefreshStartingGroups();
		}
	}
	else if ( msg.GetEvent() == WE_MP_SET_GAME_TYPE )
	{
		StagingAreaShowTeamColumn( gVictory.GetTeamMode() != Victory::NO_TEAMS );

		ShowGameSettingsList( true );
	}
	else if( msg.GetEvent() == WE_MP_FAILED_CONNECT )
	{
		eNetPipeEvent hr = (eNetPipeEvent)msg.GetData1();
		ShowFailedToJoinMessage( hr );
		OnNextUpdate( NU_NONE );
	}
}


void UIMultiplayer::ShowMessageBox( gpwstring message, eMultiplayerDialog dialog, gpwstring messageButton )
{
	if ( !m_bIsActivated )
	{
		return;
	}

	UIButton * pOK  = (UIButton *)gUIShell.FindUIWindow( "mp_dialog_button_ok", "mp_dialog" );
	VALIDUI( pOK );

	int bottomText = pOK->GetRect().top - 12;

	switch ( dialog )
	{
		case MP_DIALOG_OK_ENUM_SESSIONS:
		case MP_DIALOG_OK:
		case MP_DIALOG_CANCEL_FIND:
		case MP_DIALOG_CANCEL_CONNECT:
		case MP_DIALOG_CANCEL_ZONE_CONNECT:
		case MP_DIALOG_CANCEL_ZONE_LOGIN:
		case MP_DIALOG_ENTER_PLAYER_NAME:
		case MP_DIALOG_ZONE_CONNECT_FAILED:
		case MP_DIALOG_DISCONNECTED_FROM_GAME:
		{
			SHOW(( "mp_dialog_button_ok" ));
			HIDE(( "mp_dialog_button_yes" ));
			HIDE(( "mp_dialog_button_no" ));
			break;
		}

		case MP_DIALOG_YESNO:
		case MP_DIALOG_LEAVE_STAGING_AREA:
		case MP_DIALOG_YESNO_IMPORT_CHARACTER:
		case MP_DIALOG_YESNO_DELETE_CHARACTER:
		case MP_DIALOG_YESNO_CREATE_CHARACTER:
		case MP_DIALOG_YESNO_CHANGE_WORLD:
		{
			HIDE(( "mp_dialog_button_ok" ));
			SHOW(( "mp_dialog_button_yes" ));
			SHOW(( "mp_dialog_button_no" ));
			break;
		}

		default:
		{
			bottomText = pOK->GetRect().bottom;

			HIDE(( "mp_dialog_button_ok" ));
			HIDE(( "mp_dialog_button_yes" ));
			HIDE(( "mp_dialog_button_no" ));
			break;
		}
	}

	UIText * pButtonText = (UIText *)gUIShell.FindUIWindow( "mp_dialog_button_ok_text" );
	if ( pButtonText )
	{
		if ( (dialog == MP_DIALOG_CANCEL_CONNECT) ||
			 (dialog == MP_DIALOG_CANCEL_FIND) ||
			 (dialog == MP_DIALOG_CANCEL_ZONE_CONNECT) ||
			 (dialog == MP_DIALOG_CANCEL_ZONE_LOGIN) )
		{
			messageButton = gpwtranslate( $MSG$ "Cancel" );
		}
		else if ( messageButton.empty() )
		{
			messageButton = gpwtranslate( $MSG$ "OK" );
		}

		pButtonText->SetText( messageButton );
	}

	UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "mp_dialog_text_box" );
	if ( pTextBox )
	{
		pTextBox->SetRect( pTextBox->GetRect().left, pTextBox->GetRect().right, pTextBox->GetRect().top, bottomText );
		pTextBox->SetText( message );
		//pTextBox->SetToolTipText( message );
	}

	UIText * pText = (UIText *)gUIShell.FindUIWindow( "mp_dialog_text" );
	if ( pText )
	{
		pText->SetText( L"" );
	}
	pText = (UIText *)gUIShell.FindUIWindow( "mp_dialog_text2" );
	if ( pText )
	{
		pText->SetText( L"" );
	}

	m_fEatEnter = true;
	m_DialogInput = DIALOG_INPUT_NONE;
	m_MultiplayerDialog = dialog;
	m_bShowDialog = true;
}


void UIMultiplayer::ShowLargeMessageBox( gpwstring message, eMultiplayerDialog dialog, gpwstring messageButton )
{
	if ( !m_bIsActivated )
	{
		return;
	}

	gUIShell.ShowInterface( "message_box_large" );

	UITextBox * pBox = (UITextBox *)gUIShell.FindUIWindow( "text_box_message", "message_box_large" );
	if ( pBox )
	{
		pBox->SetText( message );
	}

	UIText * pButtonText = (UIText *)gUIShell.FindUIWindow( "text_message_okay", "message_box_large" );	
	if ( pButtonText && !messageButton.empty() )
	{
		pButtonText->SetText( messageButton );
	}
	else
	{		
		pButtonText->SetText( gpwtranslate( $MSG$ "OK" ) );
	}

	m_MultiplayerDialog = dialog;
}


void UIMultiplayer::HideMessageBox()
{
	m_bShowDialog = false;
	m_MultiplayerDialog = MP_DIALOG_INVALID;
	gUIShell.HideInterface( "mp_dialog" );
	gUIShell.HideInterface( "message_box_large" );
}


void UIMultiplayer::SetNetPipeContentResources()
{
	TankManifest manifest;
	gTankMgr.GetTankManifest( manifest );

	gNetPipe.ClearLocalContentResourceColl();
	for( TankManifest::iterator i = manifest.begin(); i != manifest.end(); ++i )
	{
		gNetPipe.AddLocalContentResource( (*i).m_GUID, (*i).m_FileName );
	}
}


bool UIMultiplayer::JoinLanGame( const char* playerName, const char* password, bool errorOnFail )
{
	gpwstring sessionName;
	sessionName.assignf( gpwtranslate( $MSG$ "%s's Game" ), ::ToUnicode( playerName ).c_str() );

	NetPipeSessionHandle session = gNetPipe.GetSession( sessionName );
	if( !session )
	{
		return( false );
	}

	if( gNetPipe.HasOpenSession() )
	{
		gpwarning( "UIMultiplayer::JoinLanGame - already have an open session.  Request ignored.\n" );
		return true;
	}

	SetNetPipeContentResources();

	eNetPipeEvent hr = gNetPipe.ConnectTo( session, ::ToUnicode( password ) );

	if( hr == NE_OK )
	{
		ShowMessageBox( gpwtranslate( $MSG$ "Connecting to game..." ), MP_DIALOG_CANCEL_CONNECT, gpwtranslate( $MSG$ "Cancel" ) );
	}
	else if ( errorOnFail )
	{
		ShowFailedToJoinMessage( hr );
	}

	OnNextUpdate( NU_NONE );
	return ( SUCCEEDED(hr) );
}


FuBiCookie UIMultiplayer::RSDisplayImportCharacter( const gpstring & templateName, DWORD machineId )
{
	FUBI_RPC_THIS_CALL_RETRY( RSDisplayImportCharacter, RPC_TO_SERVER );

	GoCloneReq cloneReq;
	cloneReq.m_OmniGo = true;
	cloneReq.m_AllClients = true;
	Goid importChar = gGoDb.SCloneGo( cloneReq, templateName );

	return ( RCDisplayImportCharacter( importChar, machineId ) );
}

FuBiCookie UIMultiplayer::RCDisplayImportCharacter( Goid importChar, DWORD machineId )
{
	FUBI_RPC_THIS_CALL_RETRY( RCDisplayImportCharacter, machineId );

	m_pCharacterSelect->SetDisplayActor( importChar );

	return RPC_SUCCESS;
}


void UIMultiplayer::SStartNetworkGame()
{
	CHECK_SERVER_ONLY;

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	if( session )
	{
		session->SetIsLocked( true );

		if ( gUIZoneMatch.IsHosting() )
		{
			gUIZoneMatch.Update( 0 );  // force a zone refresh before starting load
		}

		SaveGameSettings();

		RCStartNetworkGame();
	}
}


FuBiCookie UIMultiplayer::RCStartNetworkGame()
{
	FUBI_RPC_THIS_CALL_RETRY( RCStartNetworkGame, RPC_TO_ALL );

	// BUG 12205/6 we can't have the check here, because for this dialogue, when you click ok, it goes to the 
	// previous menu.  so if we don't have the cd in, it will take us to a previous menu, that might have
	// some options already set, like the i am ready to begin button for example....
	// this is now checked when you start or host a game.  this way, spawning functionality, if we deside to
	// implement it will not be comprimized.
	//if ( !gUIFrontend.ShowCDCheckDialog() )
	//{
	//	return RPC_SUCCESS;
	//}

	gUIFrontend.PlayFrontendSound( "frontend_big_button" );

	gWorldStateRequest( WS_LOAD_MAP );

	if ( GetMultiplayerMode() == MM_GUN )
	{
		gUIZoneMatch.OnBeginGame();
	}

	return RPC_SUCCESS;
}


void UIMultiplayer::DisplayLanGamesList()
{
	//
	// update session list only if it is dirty... also keep periodically asking for session enumeration
	//

	gpassert( gNetPipe.IsSessionCollDirty() );

	UIListReport *listreport = (UIListReport *)gUIShell.FindUIWindow( "lan_games_list" );
	if ( !listreport )
	{
		return;
	}

	int selectedGame = listreport->GetSelectedElementTag();

	listreport->RemoveAllElements();

	int nRow = 0;
	
	kerneltool::Critical::Lock lock( gNetPipe.GetSessionCritical() );
	
	NetPipeSessionHandleColl sessions = gNetPipe.GetSessions();

	for( NetPipeSessionHandleColl::iterator i = sessions.begin(); i != sessions.end(); ++i, ++nRow )
	{
		DWORD color = 0xffffffff;

		if( !(*i)->IsExecutableCompatible() )
		{
			color = 0xffff0000;
		}
		else if( !(*i)->IsContentCompatible() )
		{
			color = 0xffffff00;
		}

		listreport->AddElement( (*i)->GetSessionName().c_str(), color, (*i)->GetLocalId() );

		gpwstring occupancy;
		occupancy.assignf( L"%d/%d", (*i)->GetNumMachinesPresent(), (*i)->GetNumMachinesMax() );
		listreport->SetElementText( occupancy, 1, nRow );

		gpwstring latency;
		latency.assignf( L"%dms", (int)(1000.0f * (*i)->GetLatency()) );
		listreport->SetElementText( latency, 2, nRow );

		listreport->SetElementText( ::IsInGame( (eWorldState)(*i)->GetInfo().m_WorldState ) ? gpwtranslate( $MSG$ "Yes" ) : gpwtranslate( $MSG$ "No" ), 3, nRow );
	}

	listreport->SortColumn( listreport->GetActiveColumn() );

	UI_LISTREPORT_ELEMENT * pElement = listreport->GetElementByTag( selectedGame );
	if ( pElement )
	{
		listreport->SetSelectedElement( pElement->ID );
	}

	UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( "button_join", "lan_game_menu" );
	if ( pButton )
	{
		pButton->SetEnabled( listreport->GetNumElements() > 0 );
	}
}


HRESULT UIMultiplayer::Connect( NetPipeSessionHandle & session, gpwstring const & password )
{
	gpassert( session );

	if( gNetPipe.HasOpenSession() )
	{
		gpwarning( "UIMultiplayer::Connect - already have an open session.  Request ignored.\n" );
		return S_OK;
	}

	SetNetPipeContentResources();

	eNetPipeEvent hr = gNetPipe.ConnectTo( session, password );

	if( hr == NE_OK )
	{
		ShowMessageBox( gpwtranslate( $MSG$ "Connecting to game..." ), MP_DIALOG_CANCEL_CONNECT, gpwtranslate( $MSG$ "Cancel" ) );
	}
	else
	{
		ShowFailedToJoinMessage( hr );
	}

	// put us into a state where we detect if the game has gone bad, and exits early.
	// enable when bart gets back
	// OnNextUpdate( NU_CONNECTING );

	OnNextUpdate( NU_NONE );

	return hr;
}


bool UIMultiplayer::JoinGame( gpwstring ipAddress )
{
	if( gNetPipe.HasOpenSession() )
	{
		gpwarning( "UIMultiplayer::JoinGame - already have an open session.  Request ignored.\n" );
		return true;
	}

	HRESULT hr = gNetPipe.FindSessionAtIp( ipAddress );
		
	if ( FAILED(hr) )
	{
		switch (hr)
		{
			case DPNERR_INVALIDDEVICEADDRESS:
        		ShowMessageBox( gpwtranslate( $MSG$ "Can't connect. Possible problem negotiating with the proxy. Please see your network administrator." ) );
				break;
				
			case DPNERR_INVALIDHOSTADDRESS:
				ShowMessageBox( gpwtranslate( $MSG$ "Address entered is invalid. Enter a valid IP address or machine name." ) );
				break;

			case DPNERR_ADDRESSING:
				ShowMessageBox( gpwtranslate( $MSG$ "Host machine name could not be resolved to valid IP address." ) );
				break;
		}

		return false;
	}

	ShowMessageBox( gpwtranslate( $MSG$ "Looking for game..." ), MP_DIALOG_CANCEL_FIND );

	m_FindSessionTimeout = 0.0;

	m_JoiningIP = ToAnsi( ipAddress );

	OnNextUpdate( NU_FIND_GAME );

	return true;
}


bool UIMultiplayer::CompleteJoinGame()
{
	if ( gServer.IsLocal() )
	{
		return ( false );
	}

	// Save ip address history
	if ( (m_MultiplayerMode == MM_NET) && (!m_JoiningIP.empty()) )
	{
		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

		gpassert( session );

		for ( IpColl::iterator i = m_IpHistory.begin(); i != m_IpHistory.end(); ++i )
		{
			if ( i->first.same_no_case( m_JoiningIP ) )
			{
				m_IpHistory.erase( i );
				break;
			}
		}

		m_IpHistory.insert( m_IpHistory.begin(), IpPair( m_JoiningIP, session->GetSessionName() ) );

		SaveIpAddressHistory();

		m_JoiningIP.clear();
	}

	if ( m_MultiplayerMode == MM_GUN )
	{
		gUIZoneMatch.SetCurrentUserStatus( Gun2::INGAME );
	}

	gpassert( gServer.GetLocalHumanPlayer() == NULL );

	// Enter Staging Area
	gWorldStateRequest( WS_MP_STAGING_AREA_CLIENT );
	gWorldState.Update();

	return ( true );
}


void UIMultiplayer::ShowFailedToJoinMessage( eNetPipeEvent event )
{
	// make sure nothing is on the screen when we get the error.
	HideMessageBox();

	// display the error message.
	if ( event == NE_FAILED_CONNECT_HOST_REJECTED )
	{
		ShowMessageBox( gpwtranslate( $MSG$ "The host rejected your join request." ), MP_DIALOG_OK_ENUM_SESSIONS );
	}
	else if ( event == NE_FAILED_CONNECT_INVALID_PASSWORD )
	{
		ShowMessageBox( gpwtranslate( $MSG$ "The password you entered was invalid." ), MP_DIALOG_OK_ENUM_SESSIONS );
	}
	else if ( event == NE_FAILED_CONNECT_SESSION_FULL )
	{
		ShowMessageBox( gpwtranslate( $MSG$ "The game is full." ), MP_DIALOG_OK_ENUM_SESSIONS );
	}
	else if( event == NE_FAILED_CONNECT_INCOMPATIBLE_CONTENT )
	{
		gpwstring msg( gpwtranslate( $MSG$ "This game is using a mod that you currently do not have installed. Resource files that are missing or different from the host:\n" ) );		

		NetPipe::AutoContentResourceColl & resources = gNetPipe.GetServerRequiredContentColl();
		for( NetPipe::AutoContentResourceColl::iterator i = resources.begin(); i != resources.end(); ++i )
		{
			msg.append( ToUnicode( (*i)->m_Filename.c_str() ));
			msg.append( ToUnicode( "\n" ));
		}
		
		ShowLargeMessageBox( msg, MP_DIALOG_OK_ENUM_SESSIONS );
	}
	else
	{
		ShowMessageBox( gpwtranslate( $MSG$ "Could not join the game." ), MP_DIALOG_OK_ENUM_SESSIONS );
//		GPDEV_ONLY( TRYDX( hr ) );
	}
}


void UIMultiplayer::StagingAreaClearPlayerSlot( int playerNumber )
{
	UIWindow *pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_bg", playerNumber ) );
	if ( pWindow )
	{
		pWindow->SetTag( MakeInt( PLAYERID_INVALID ) );
	}

	pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_ping", playerNumber ) );
	if ( pWindow )
	{
		pWindow->SetVisible( false );
	}

	UIText *pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "player%d_text", playerNumber ) );
	if ( pText ) 
	{
		pText->SetText( L"" );
	}

	pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_ready", playerNumber ) );
	if ( pWindow ) 
	{
		pWindow->SetVisible( false );
	}
	pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_ingame", playerNumber ) );
	if ( pWindow ) 
	{
		pWindow->SetVisible( false );
	}
	pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_zone", playerNumber ) );
	if ( pWindow ) 
	{
		pWindow->SetVisible( false );
	}

	pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_bg", playerNumber ) );
	if ( pWindow )
	{
		pWindow->SetVisible( false );
	}
	pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_sign", playerNumber ) );
	if ( pWindow )
	{
		pWindow->SetVisible( false );
	}

	pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "start_location%d_text", playerNumber ) );
	if ( pText )
	{
		pText->SetText( L"" );
	}

	UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( gpstringf( "combo_start_locations%d", playerNumber ) );
	if ( pCombo )
	{
		pCombo->SetTag( MakeInt( PLAYERID_INVALID ) );
		pCombo->SetVisible( false );
	}
	pWindow = gUIShell.FindUIWindow( gpstringf( "start_location%d_bg", playerNumber ) );
	if ( pWindow )
	{
		pWindow->SetVisible( false );
	}

	UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( gpstringf( "button_select_char%d", playerNumber ) );
	if ( pButton )
	{
		pButton->SetVisible( false );
	}

	pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "character%d_text", playerNumber ) );
	if ( pText ) 
	{
		pText->SetText( L"" );
	}
}


void UIMultiplayer::StagingAreaUpdatePlayerList()
{
	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

	if ( !IsVisible() || !session )
	{
		return;
	}

	UIWindow *pActivePlayerBox = gUIShell.FindUIWindow( "active_player_box" );
	if ( pActivePlayerBox )
	{
		pActivePlayerBox->SetVisible( false );
	}

	m_ActivePlayerSlot = 0;

	int playerNum = 0;

	{for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
	{
		Player * player = *i;

		if ( IsValid( player ) && !player->IsComputerPlayer() )
		{
			++playerNum;

			if ( gServer.GetLocalHumanPlayer() == player )
			{
				m_ActivePlayerSlot = playerNum;
			}

			UIWindow *pPlayerBg = gUIShell.FindUIWindow( gpstringf( "player%d_bg", playerNum ) );
			if ( pPlayerBg )
			{
				pPlayerBg->SetTag( MakeInt( player->GetId() ) );
				pPlayerBg->SetVisible( true );
			}

			UIWindow *pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_ping", playerNum ) );
			if ( pWindow )
			{
				pWindow->SetVisible( true );
			}

			UIText *pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "player%d_text", playerNum ) );
			if ( pText ) 
			{
				pText->SetText( player->GetName() );
			}

			pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_ready", playerNum ) );
			if ( pWindow ) 
			{
				if ( !IsInGame( player->GetWorldState() ) )
				{
					pWindow->SetVisible( player->IsReadyToPlay() );
				}
				else
				{
					pWindow->SetVisible( false );
				}
			}

			pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_ingame", playerNum ) );
			if ( pWindow ) 
			{
				pWindow->SetVisible( IsInGame( player->GetWorldState() ) );
			}

			pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_zone", playerNum ) );
			if ( pWindow ) 
			{
				pWindow->SetVisible( player->IsOnZone() );
			}

			if ( m_bShowTeams )
			{
				pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_sign", playerNum ) );
				if ( pWindow )
				{
					pWindow->SetVisible( true );

					Team * team = gServer.GetTeam( player->GetId() );
					if ( team && (team->m_TextureId != 0) )
					{
						pWindow->SetTextureIndex( team->m_TextureId );
						pWindow->SetHasTexture( true );
					}
					else
					{
						pWindow->SetTextureIndex( 0 );
						pWindow->SetHasTexture( false );
					}

					((UIButton*)pWindow)->EnableButton();
				}
			}			

			gpwstring startGroup;
			const WorldMap::SGroupColl &startGroups = gWorldMap.GetStartGroups();
			for ( WorldMap::SGroupColl::const_iterator iGroup = startGroups.begin(); iGroup != startGroups.end(); ++iGroup )
			{
				if ( iGroup->sGroup.same_no_case( player->GetStartingGroup() ) )
				{
					startGroup = iGroup->sScreenName;										
					break;
				}
			}

			pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "start_location%d_text", playerNum ) );
			if ( pText )
			{
				pText->SetText( startGroup );
			}

			gpwstring heroName;
			if ( !(*i)->GetHeroName().empty() )
			{
				heroName.assignf( gpwtranslate( $MSG$ "%s - Level %d" ), (*i)->GetHeroName().c_str(), (int)(*i)->GetHeroUberLevel() );
			}
			pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "character%d_text", playerNum ) );
			if ( pText )
			{
				pText->SetText( heroName );
				if ( DoesCharacterMeetWorldLevel( (*i)->GetHeroUberLevel() ) )
				{
					pText->SetColor( 0xFFFFFFFF );
				}
				else
				{
					pText->SetColor( 0xFFFF0000 );
				}
			}

			if ( gServer.GetLocalHumanPlayer() == player )
			{
				bool allowSelections = !player->IsReadyToPlay() || gServer.IsLocal();

				if ( pActivePlayerBox && pPlayerBg )
				{
					pActivePlayerBox->SetVisible( true );
					pActivePlayerBox->SetRect( pActivePlayerBox->GetRect().left, pActivePlayerBox->GetRect().right, pPlayerBg->GetRect().top - 2, pPlayerBg->GetRect().bottom + 2 );
				}

				if ( m_bShowTeams )
				{
					UIWindow *pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_sign", playerNum ) );
					if ( pWindow )
					{
						pWindow->SetEnabled( allowSelections );
					}
				}

				UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( gpstringf( "button_select_char%d", playerNum ) );
				if ( pButton )
				{
					pButton->SetVisible( allowSelections );
				}

				pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "button_select_char%d_text", playerNum ) );
				if ( pText ) 
				{
					if ( heroName.empty() )
					{
						pText->SetText( gpwtranslate( $MSG$ "Select Character..." ) );
					}
					else
					{
						pText->SetText( heroName );
						if ( DoesCharacterMeetWorldLevel( (*i)->GetHeroUberLevel() ) )
						{
							pText->SetColor( 0xFFFFFFFF );
						}
						else
						{
							pText->SetColor( 0xFFFF0000 );
						}
					}
				}

				UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( gpstringf( "combo_start_locations%d", playerNum ) );
				if ( pCombo )
				{
					long tag = MakeInt( (*i)->GetId() );
					pCombo->SetTag( tag );
					pCombo->SetText( startGroup );
					if ( gServer.GetAllowStartLocationSelection() || gServer.IsLocal() )
					{
						pCombo->SetVisible( allowSelections );
					}
					else
					{
						pCombo->SetVisible( false );
					}

					UIWindow *pWindow = gUIShell.FindUIWindow( gpstringf( "start_location%d_bg", playerNum ) );
					if ( pWindow )
					{
						pWindow->SetVisible( pCombo->GetVisible() );
					}

					RefreshStartingGroups();
				}
			}
			else
			{
				UIWindow *pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_sign", playerNum ) );
				if ( pWindow )
				{
					pWindow->SetEnabled( false );
				}

				UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( gpstringf( "button_select_char%d", playerNum ) );
				if ( pButton )
				{
					pButton->SetVisible( false );
				}

				UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( gpstringf( "combo_start_locations%d", playerNum ) );
				if ( pCombo )
				{
					pCombo->SetTag( MakeInt( (*i)->GetId() ) );
					pCombo->SetText( startGroup );

					if ( !gServer.GetAllowStartLocationSelection() && gServer.IsLocal() )
					{
						pCombo->SetVisible( true );
					}
					else
					{
						pCombo->SetVisible( false );
					}

					UIWindow *pWindow = gUIShell.FindUIWindow( gpstringf( "start_location%d_bg", playerNum ) );
					if ( pWindow )
					{
						pWindow->SetVisible( pCombo->GetVisible() );
					}
				}
			}
		}
	}}

	{for ( unsigned int i = playerNum + 1; i <= STAGING_PLAYER_SLOTS; ++i )
	{
		bool bgVisible = true;

		gpassert( session );

		if( !session || ( i > session->GetNumMachinesMax() ) )
		{
			bgVisible = false;
		}

		UIWindow *pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_bg", i ) );
		if ( pWindow )
		{
			pWindow->SetVisible( bgVisible );
		}
		if ( m_bShowTeams )
		{
			pWindow = gUIShell.FindUIWindow( gpstringf( "player_team_sep%d", i ) );
			if ( pWindow )
			{
				pWindow->SetVisible( bgVisible );
			}
		}
		pWindow = gUIShell.FindUIWindow( gpstringf( "team_start_sep%d", i ) );
		if ( pWindow )
		{
			pWindow->SetVisible( bgVisible );
		}
		pWindow = gUIShell.FindUIWindow( gpstringf( "start_char_sep%d", i ) );
		if ( pWindow )
		{
			pWindow->SetVisible( bgVisible );
		}

		StagingAreaClearPlayerSlot( i );
	}}

	StagingAreaUpdatePings();
}

void UIMultiplayer::StagingAreaUpdatePlayer( int playerNumber, Player * pPlayer )
{
	UIWindow *pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_ready", playerNumber ) );
	if ( pWindow ) 
	{
		if ( !IsInGame( pPlayer->GetWorldState() ) )
		{
			pWindow->SetVisible( pPlayer->IsReadyToPlay() );
		}
		else
		{
			pWindow->SetVisible( false );
		}
	}

	pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_ingame", playerNumber ) );
	if ( pWindow ) 
	{
		pWindow->SetVisible( IsInGame( pPlayer->GetWorldState() ) );
	}

	pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_zone", playerNumber ) );
	if ( pWindow ) 
	{
		pWindow->SetVisible( pPlayer->IsOnZone() );
	}

	if ( m_bShowTeams )
	{
		pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_sign", playerNumber ) );
		if ( pWindow )
		{
			pWindow->SetVisible( true );

			Team * team = gServer.GetTeam( pPlayer->GetId() );
			if ( team && (team->m_TextureId != 0) )
			{
				pWindow->SetTextureIndex( team->m_TextureId );
				pWindow->SetHasTexture( true );
			}
			else
			{
				pWindow->SetTextureIndex( 0 );
				pWindow->SetHasTexture( false );
			}
		}
	}

	gpwstring hero;
	if ( !pPlayer->GetHeroName().empty() )
	{
		hero.assignf( gpwtranslate( $MSG$ "%s - Level %d" ), pPlayer->GetHeroName().c_str(), (int)pPlayer->GetHeroUberLevel() );
	}

	if ( !hero.empty() )
	{
		UIText * pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "button_select_char%d_text", playerNumber ) );
		if ( pText )
		{
			pText->SetText( hero );
			if ( DoesCharacterMeetWorldLevel( pPlayer->GetHeroUberLevel() ) )
			{
				pText->SetColor( 0xFFFFFFFF );
			}
			else
			{
				pText->SetColor( 0xFFFF0000 );
			}
		}
	}

	UIText * pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "character%d_text", playerNumber ) );
	if ( pText )
	{
		pText->SetText( hero );
		if ( DoesCharacterMeetWorldLevel( pPlayer->GetHeroUberLevel() ) )
		{
			pText->SetColor( 0xFFFFFFFF );
		}
		else
		{
			pText->SetColor( 0xFFFF0000 );
		}
	}

	const WorldMap::SGroupColl &startGroups = gWorldMap.GetStartGroups();
	for ( WorldMap::SGroupColl::const_iterator i = startGroups.begin(); i != startGroups.end(); ++i )
	{
		if ( i->sGroup.same_no_case( pPlayer->GetStartingGroup() ) )
		{
			UIText *pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "start_location%d_text", playerNumber ) );
			if ( pText )
			{
				pText->SetText( i->sScreenName );
			}

			UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( gpstringf( "combo_start_locations%d", playerNumber ) );
			if ( pCombo )
			{
				pCombo->SetText( i->sScreenName );
			}
			break;
		}
	}

	if ( gServer.IsLocal() )
	{
		StagingAreaCheckEnableStart();
	}
}


void UIMultiplayer::StagingAreaUpdatePings()
{
	int playerNum = 0;

	for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
	{
		if ( IsValid( *i ) && !(*i)->IsComputerPlayer())
		{
			++playerNum;
			UIWindow *pWindow = gUIShell.FindUIWindow( gpstring().appendf( "player%d_ping", playerNum ) );
			if ( pWindow )
			{
				// $$$ c/s
				//if( IsValid(*i) && ( (*i)->IsRemote() || gServer.IsLocal() ) )
				if( IsValid(*i) && ( (*i)->IsRemote() && gServer.IsLocal() ) )
				{
					if ( (*i)->GetInstantaneousLatency() < 0.1 )
					{
						pWindow->LoadTexture( "b_gui_fe_m_mp_staging_ping_5" );
					}
					else if ( (*i)->GetInstantaneousLatency() < 0.25 )
					{
						pWindow->LoadTexture( "b_gui_fe_m_mp_staging_ping_4" );
					}
					else if ( (*i)->GetInstantaneousLatency() < 0.5 )
					{
						pWindow->LoadTexture( "b_gui_fe_m_mp_staging_ping_3" );
					}
					else if ( (*i)->GetInstantaneousLatency() < 0.75 )
					{
						pWindow->LoadTexture( "b_gui_fe_m_mp_staging_ping_2" );
					}
					else
					{
						pWindow->LoadTexture( "b_gui_fe_m_mp_staging_ping_1" );
					}
				}
				else
				{
					pWindow->SetVisible(false);
				}
			}
		}
	}
}


void UIMultiplayer::StagingAreaShowTeamColumn( bool flag )
{
	m_bShowTeams = flag;

	int playerColRight = 0;
	if ( flag )
	{
		UIWindow *pWindow = gUIShell.FindUIWindow( "player_team_sep1" );
		if ( pWindow )
		{
			playerColRight = pWindow->GetRect().left;
		}
	}
	else
	{
		UIWindow *pWindow = gUIShell.FindUIWindow( "team_start_sep1" );
		if ( pWindow )
		{
			playerColRight = pWindow->GetRect().left;
		}
	}

	if ( playerColRight == 0 )
	{
		return;
	}

	UIWindow *pWindow = gUIShell.FindUIWindow( "team_header_text" );
	if ( pWindow )
	{
		pWindow->SetVisible( flag );
	}

	for ( UINT playerSlot = 1; playerSlot <= STAGING_PLAYER_SLOTS; ++playerSlot )
	{
		UIWindow *pWindow = gUIShell.FindUIWindow( gpstring().appendf( "team%d_sign", playerSlot ) );
		if ( pWindow )
		{
			pWindow->SetVisible( flag );
		}
		pWindow = gUIShell.FindUIWindow( gpstring().appendf( "player_team_sep%d", playerSlot ) );
		if ( pWindow )
		{
			pWindow->SetVisible( flag );
		}

		UIText *pText = (UIText *)gUIShell.FindUIWindow( gpstring().appendf( "player%d_text", playerSlot ) );
		if ( pText )
		{
			pText->SetRect( pText->GetRect().left, playerColRight - 4, pText->GetRect().top, pText->GetRect().bottom );
		}

		pWindow = gUIShell.FindUIWindow( gpstring().appendf( "player%d_zone", playerSlot ) );
		if ( pWindow )
		{
			pWindow->SetRect( playerColRight - 20, playerColRight - 4, pWindow->GetRect().top, pWindow->GetRect().bottom );
		}
	}

	StagingAreaUpdatePlayerList();
}


void UIMultiplayer::StagingAreaShowMpCharacters( int charIndex )
{
	// update slider
	UISlider * pSlider = (UISlider *)gUIShell.FindUIWindow( "scroll_characters", "staging_area_select_character" );
	if ( pSlider )
	{
		pSlider->SetMax( m_MpCharacters.size() - STAGING_CHARACTER_SLOTS );
		pSlider->SetMin( 0 );
		pSlider->SetValue( charIndex );
		pSlider->SetEnabled( pSlider->GetMax() != 0 );
		pSlider->SetAlpha( pSlider->IsEnabled() ? 1.0f : 0.6f );
		for ( UIWindowVec::iterator i = pSlider->GetChildren().begin(); i != pSlider->GetChildren().end(); ++i )
		{
			(*i)->SetAlpha( pSlider->IsEnabled() ? 1.0f : 0.6f );
		}
		charIndex = pSlider->GetValue();
	}

	int index = 0, slot = 1;
	for ( CharacterInfoColl::iterator i = m_MpCharacters.begin(); i != m_MpCharacters.end(); ++i, ++index )
	{
		if ( (index == charIndex) || (slot > 1) )
		{
			UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "character%d_bg", slot ), "staging_area_select_character" );
			if ( pWindow )
			{
				pWindow->SetVisible( true );
			}
			pWindow = gUIShell.FindUIWindow( gpstringf( "character%d_sel", slot ), "staging_area_select_character" );
			if ( pWindow )
			{
				pWindow->SetVisible( index == m_SetSelectedCharacter );
			}
			pWindow = gUIShell.FindUIWindow( gpstringf( "character%d_portrait", slot ), "staging_area_select_character" );
			if ( pWindow )
			{
				pWindow->SetVisible( true );
				if ( index < (int)m_CharacterPortraits.size() )
				{
					pWindow->SetTextureIndex( m_CharacterPortraits[ index ] );
					pWindow->SetHasTexture( true );
				}
			}
			UIText * pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "character%d_name", slot ), "staging_area_select_character" );
			if ( pText )
			{
				pText->SetVisible( true );
				pText->SetText( (*i)->m_ScreenName );
			}
			pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "character%d_class", slot ), "staging_area_select_character" );
			if ( pText )
			{
				pText->SetVisible( true );
				pText->SetText( (*i)->m_ActorClass );
			}
			pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "character%d_skills", slot ), "staging_area_select_character" );
			if ( pText )
			{
				int strength = 0, dexterity = 0, intelligence = 0;

				CharacterInfo::SkillDb::iterator skill = (*i)->m_Skills.find( "strength" );
				if ( skill != (*i)->m_Skills.end() )
				{
					strength = (int)skill->second.m_Level;
				}
				skill = (*i)->m_Skills.find( "dexterity" );
				if ( skill != (*i)->m_Skills.end() )
				{
					dexterity = (int)skill->second.m_Level;
				}
				skill = (*i)->m_Skills.find( "intelligence" );
				if ( skill != (*i)->m_Skills.end() )
				{
					intelligence = (int)skill->second.m_Level;
				}

				pText->SetText( gpwstringf( gpwtranslate( $MSG$ "Str:%d Dex:%d Int:%d" ), strength, dexterity, intelligence ) );
				pText->SetVisible( true );
			}
			pWindow = gUIShell.FindUIWindow( gpstringf( "character%d_icon_melee", slot ), "staging_area_select_character" );
			if ( pWindow )
			{
				pWindow->SetVisible( true );
			}
			pWindow = gUIShell.FindUIWindow( gpstringf( "character%d_icon_ranged", slot ), "staging_area_select_character" );
			if ( pWindow )
			{
				pWindow->SetVisible( true );
			}
			pWindow = gUIShell.FindUIWindow( gpstringf( "character%d_icon_nmagic", slot ), "staging_area_select_character" );
			if ( pWindow )
			{
				pWindow->SetVisible( true );
			}
			pWindow = gUIShell.FindUIWindow( gpstringf( "character%d_icon_cmagic", slot ), "staging_area_select_character" );
			if ( pWindow )
			{
				pWindow->SetVisible( true );
			}
			pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "character%d_melee", slot ), "staging_area_select_character" );
			if ( pText )
			{
				CharacterInfo::SkillDb::iterator skill = (*i)->m_Skills.find( "melee" );
				if ( skill != (*i)->m_Skills.end() )
				{
					pText->SetText( gpwstringf( L"%d", (int)skill->second.m_Level ) );
				}
				pText->SetVisible( true );
			}
			pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "character%d_ranged", slot ), "staging_area_select_character" );
			if ( pText )
			{
				CharacterInfo::SkillDb::iterator skill = (*i)->m_Skills.find( "ranged" );
				if ( skill != (*i)->m_Skills.end() )
				{
					pText->SetText( gpwstringf( L"%d", (int)skill->second.m_Level ) );
				}
				pText->SetVisible( true );
			}
			pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "character%d_nmagic", slot ), "staging_area_select_character" );
			if ( pText )
			{
				CharacterInfo::SkillDb::iterator skill = (*i)->m_Skills.find( "nature magic" );
				if ( skill != (*i)->m_Skills.end() )
				{
					pText->SetText( gpwstringf( L"%d", (int)skill->second.m_Level ) );
				}
				pText->SetVisible( true );
			}
			pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "character%d_cmagic", slot ), "staging_area_select_character" );
			if ( pText )
			{
				CharacterInfo::SkillDb::iterator skill = (*i)->m_Skills.find( "combat magic" );
				if ( skill != (*i)->m_Skills.end() )
				{
					pText->SetText( gpwstringf( L"%d", (int)skill->second.m_Level ) );
				}
				pText->SetVisible( true );
			}

			++slot;
		}

		if ( slot > STAGING_CHARACTER_SLOTS )
		{
			break;
		}
	}

	// clear remaining character slots
	{for ( int i = slot; i <= STAGING_CHARACTER_SLOTS; ++i )
	{
		HIDE(( "character%d_bg",          i ));
		HIDE(( "character%d_sel",         i ));
		HIDE(( "character%d_portrait",    i ));
		HIDE(( "character%d_name",        i ));
		HIDE(( "character%d_class",       i ));
		HIDE(( "character%d_skills",      i ));
		HIDE(( "character%d_icon_melee",  i ));
		HIDE(( "character%d_icon_ranged", i ));
		HIDE(( "character%d_icon_nmagic", i ));
		HIDE(( "character%d_icon_cmagic", i ));
		HIDE(( "character%d_melee",       i ));
		HIDE(( "character%d_ranged",      i ));
		HIDE(( "character%d_nmagic",      i ));
		HIDE(( "character%d_cmagic",      i ));
	}}
}


void UIMultiplayer::StagingAreaEnableSelections( bool enable )
{
	SETENABLED(( "button_map_settings" ), enable );
	SETENABLED(( "button_game_settings" ), enable );
	SETENABLED(( "staging_area_button_start_game" ), enable );
	SETENABLED(( "staging_area_button_leave_game" ), enable );
	SETENABLED(( "staging_checkbox_ready" ), enable );
}


void UIMultiplayer::SavePlayerSettings()
{
	Player * player = gServer.GetLocalHumanPlayer();
	if ( !player )
	{
		return;
	}

	FuelHandle prefs = gTattooGame.GetPrefsConfigFuel( true );
	if ( prefs.IsValid() )
	{
		FuelHandle hSettings = prefs->GetChildBlock( "multiplayer" );
		if ( !hSettings.IsValid() )
		{
			return;
		}

		// character
		hSettings->Set( "character_name", player->GetHeroName() );

		// starting location
		FuelHandle hStartGroups = hSettings->GetChildBlock( "start_locations", true );
		if ( hStartGroups.IsValid() )
		{
			hStartGroups->Set( gWorldMap.GetMapName(), player->GetStartingGroup() );
		}

		prefs->GetDB()->SaveChanges();
	}
}


void UIMultiplayer::LoadPlayerSettings()
{
	Player * player = gServer.GetLocalHumanPlayer();
	if ( !player )
	{
		return;
	}

	FuelHandle prefs = gTattooGame.GetPrefsConfigFuel();
	if ( prefs.IsValid() )
	{
		FuelHandle hSettings = prefs->GetChildBlock( "multiplayer" );
		if ( !hSettings.IsValid() )
		{
			return;
		}

		// character
		m_SelectedCharacter = -1;
		m_CharacterName.clear();
		if ( hSettings->Get( "character_name", m_CharacterName ) )
		{
			if ( !gServer.GetAllowNewCharactersOnly() && !m_CharacterName.empty() )
			{
				int index = 0;
				for ( CharacterInfoColl::iterator i = m_MpCharacters.begin(); i != m_MpCharacters.end(); ++i, ++index )
				{
					if ( (*i)->m_ScreenName.same_no_case( m_CharacterName ) )
					{
						if ( gTattooGame.ImportCharacterFromSave( (*i)->m_FileName, (*i)->m_Index ) )
						{
							m_SelectedCharacter = index;
							player->RSSetHeroName( m_CharacterName );
							player->RSSetHeroUberLevel( GetSelectedCharacterUberLevel() );
						}
						break;
					}
				}
			}
		}
		if ( m_SelectedCharacter == -1 )
		{
			m_CharacterName.clear();
		}
	}

	LoadPlayerStartLocationPref();
}


void UIMultiplayer::LoadPlayerStartLocationPref()
{
	Player * player = gServer.GetLocalHumanPlayer();
	if ( !player )
	{
		return;
	}

	FuelHandle prefs = gTattooGame.GetPrefsConfigFuel();
	if ( prefs.IsValid() )
	{
		FuelHandle hSettings = prefs->GetChildBlock( "multiplayer" );
		if ( !hSettings.IsValid() )
		{
			return;
		}

		// starting location
		if ( gServer.GetAllowStartLocationSelection() )
		{
			FuelHandle hStartGroups = hSettings->GetChildBlock( "start_locations" );
			if ( hStartGroups.IsValid() )
			{
				gpstring startGroup;
				if ( hStartGroups->Get( gWorldMap.GetMapName(), startGroup ) && 
					 IsStartingGroupEnabled( startGroup ) )
				{
					player->RSSetStartingGroup( startGroup );
				}
			}
		}
	}
}


void UIMultiplayer::SaveGameSettings()
{
	FuelHandle prefs = gTattooGame.GetPrefsConfigFuel( true );
	if ( prefs.IsValid() )
	{
		FuelHandle hSettings = prefs->GetChildBlock( "multiplayer", true );
		if ( !hSettings.IsValid() )
		{
			return;
		}
		hSettings = hSettings->GetChildBlock( "game_settings", true );
		if ( !hSettings.IsValid() )
		{
			return;
		}

		hSettings->Set( "map", gWorldMap.GetMapName() );
		hSettings->Set( "map_world", gWorldMap.GetMpWorldName() );
		hSettings->Set( "player_limit", gServer.GetMaxPlayers() );
		hSettings->Set( "difficulty", (int)gWorldOptions.GetDifficulty() );
		hSettings->Set( "time_limit", gVictory.GetTimeLimit() );
		hSettings->Set( "drop_on_death", (int)gServer.GetDropInvOption() );
		hSettings->Set( "game_type", gVictory.GetGameType()->GetName() );
		hSettings->Set( "allow_new_characters_only", gServer.GetAllowNewCharactersOnly() );
		hSettings->Set( "allow_start_selection", gServer.GetAllowStartLocationSelection() );
		hSettings->Set( "allow_player_pausing", gServer.GetAllowPausing() );
		hSettings->Set( "enable_jip", gServer.GetAllowJIP() );

		prefs->GetDB()->SaveChanges();
	}
}


void UIMultiplayer::LoadGameSettings()
{
	FuelHandle prefs = gTattooGame.GetPrefsConfigFuel();
	if ( prefs.IsValid() )
	{
		FuelHandle hSettings = prefs->GetChildBlock( "multiplayer:game_settings" );
		if ( !hSettings.IsValid() )
		{
			return;
		}

		if ( !gWorldMap.IsInitialized() )
		{
			gpstring mapName, mapWorld;
			hSettings->Get( "map", mapName );
			hSettings->Get( "map_world", mapWorld );
			if ( !mapName.empty() && gTankMgr.HasMap( mapName ) )
			{
				if ( mapWorld.empty() )
				{
					gWorldMap.SSet( mapName, NULL, RPC_TO_ALL );
				}
				else
				{
					gWorldMap.SSet( mapName, mapWorld, RPC_TO_ALL );
				}
			}
		}

		int maxPlayers;
		if ( hSettings->Get( "player_limit", maxPlayers ) )
		{
			gServer.SSetMaxPlayers( maxPlayers );
		}

		int difficulty;
		if ( hSettings->Get( "difficulty", difficulty ) )
		{
			gWorldOptions.RCSetDifficulty( (eDifficulty)difficulty );
		}

		float timeLimit;
		if ( hSettings->Get( "time_limit", timeLimit ) )
		{
			SSetTimeLimit( timeLimit );
		}

		int dropOnDeath;
		if ( hSettings->Get( "drop_on_death", dropOnDeath ) )
		{
			gServer.SSetDropInvOption( (eDropInvOption)dropOnDeath );
		}

		gpstring gameType;
		if ( hSettings->Get( "game_type", gameType ) )
		{
			gVictory.SSetGameType( gameType );
		}

		bool allowNewCharactersOnly;
		if ( hSettings->Get( "allow_new_characters_only", allowNewCharactersOnly ) )
		{
			gServer.SSetAllowNewCharactersOnly( allowNewCharactersOnly );
		}

		bool allowStartSelection;
		if ( hSettings->Get( "allow_start_selection", allowStartSelection ) )
		{
			gServer.SSetAllowStartLocationSelection( allowStartSelection );
		}

		bool allowPlayerPausing;
		if ( hSettings->Get( "allow_player_pausing", allowPlayerPausing ) )
		{
			gServer.SSetAllowPausing( allowPlayerPausing );
		}

		bool enableJip;
		if ( hSettings->Get( "enable_jip", enableJip ) )
		{
			gServer.SSetAllowJIP( enableJip );
		}
	}
}


void UIMultiplayer::SaveIpAddressHistory()
{
	FuelHandle prefs = gTattooGame.GetPrefsConfigFuel( true );
	if ( prefs.IsValid() )
	{
		FuelHandle hHistory = prefs->GetChildBlock( "multiplayer", true );
		if ( !hHistory.IsValid() )
		{
			return;
		}
		hHistory = hHistory->GetChildBlock( "internet_history", true );
		if ( !hHistory.IsValid() )
		{
			return;
		}

		hHistory->ClearKeysAndChildren();

		int entry = 0;

		for ( IpColl::iterator i = m_IpHistory.begin(); i != m_IpHistory.end(); ++i, ++entry )
		{
			hHistory->Set( gpstringf( "ip%d", entry ), i->first );
			if ( !i->second.empty() )
			{
				hHistory->Set( gpstringf( "name%d", entry ), i->second );
			}
		}

		prefs->GetDB()->SaveChanges();
	}
}


void UIMultiplayer::LoadIpAddressHistory()
{
	m_IpHistory.clear();

	FuelHandle prefs = gTattooGame.GetPrefsConfigFuel();
	if ( prefs.IsValid() )
	{
		FuelHandle hHistory = prefs->GetChildBlock( "multiplayer:internet_history" );
		if ( !hHistory.IsValid() )
		{
			return;
		}

		gpstring ipAddress;
		gpwstring hostName;
		int entry = 0;

		while ( hHistory->Get( gpstringf( "ip%d", entry ), ipAddress ) )
		{
			hostName.clear();
			hHistory->Get( gpstringf( "name%d", entry ), hostName );

			m_IpHistory.push_back( IpPair( ipAddress, hostName ) );

			++entry;
		}
	}
}


void UIMultiplayer::StagingAreaCheckEnableStart()
{
	/*
	bool enableStart = true;

	for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
	{
		if ( *i && !(*i)->IsComputerPlayer() && !(*i)->IsReadyToPlay() )
		{
			enableStart = false;
			break;
		}
	}

	if ( !gWorldMap.IsInitialized() )
	{
		enableStart = false;
	}

	if ( gServer.GetLocalHumanPlayer() && gServer.GetLocalHumanPlayer()->GetHeroName().empty() )
	{
		enableStart = false;
	}

	UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( "staging_area_button_start_game" );
	if( pButton )
	{
		pButton->SetEnabled( enableStart );
	}
	*/
}


void UIMultiplayer::ShowDisconnectedMessage()
{
	if ( m_DisconnectInfo == NE_MACHINE_DISCONNECTED_KICKED )
	{
		ShowMessageBox( gpwtranslate( $MSG$ "You have been kicked from the game." ), MP_DIALOG_DISCONNECTED_FROM_GAME );
	}
	else if ( m_DisconnectInfo == NE_MACHINE_DISCONNECTED_BANNED )
	{
		ShowMessageBox( gpwtranslate( $MSG$ "You have been banned from the game by the host." ), MP_DIALOG_DISCONNECTED_FROM_GAME );
	}
	else
	{
		ShowMessageBox( gpwtranslate( $MSG$ "You have been disconnected from the game." ), MP_DIALOG_DISCONNECTED_FROM_GAME );
	}
}


void UIMultiplayer::ShowMultiplayerEditDialog( gpwstring sText, eMultiplayerEditDialog type )
{
	gUIShell.ShowInterface( "mp_edit_dialog" );

	m_MultiplayerEditDialog = type;

	UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "mp_edit_dialog_text_box", "mp_edit_dialog" );
	if ( pTextBox )
	{
		pTextBox->SetText( sText );
	}
}


void UIMultiplayer::HideMultiplayerEditDialog( void )
{
	gUIShell.HideInterface( "mp_edit_dialog" );

	UIEditBox * pEdit = (UIEditBox *)gUIShell.FindUIWindow( "edit_box_mp_dialog_text" );
	if ( pEdit )
	{
		pEdit->SetText( L"" );
	}

	m_MultiplayerEditDialog = MP_EDIT_DIALOG_INVALID;
}


void UIMultiplayer::CopyOrRenameCharacter( gpwstring sName, bool bRename )
{
	if ( !CanCopyOrRenameCharacter( sName ) )
	{
		return;
	}

	// Rename the character here	
	if ( !CopyRenameCharacterFromSave( m_MpCharacters[m_SetSelectedCharacter]->m_FileName, sName ) )
	{		
		HideMultiplayerEditDialog();	

		if ( bRename )
		{
			ShowMessageBox( gpwtranslate( $MSG$ "Could not rename the character. Make sure you are trying to rename from a character tank." ) );
		}
		else		
		{
			ShowMessageBox( gpwtranslate( $MSG$ "Could not copy the character. Make sure you are trying to copy from a character tank." ) );
		}

		return;
	}

	if ( bRename )
	{
		if ( !gTattooGame.DeleteSaveGame( m_MpCharacters[ m_SetSelectedCharacter ]->m_FileName ) )
		{
			HideMultiplayerEditDialog();	
			ShowMessageBox( gpwtranslate( $MSG$ "Failed to delete old save game file." ) );

			return;
		}
	}
	
	RefreshCharacterScreen();

	HideMultiplayerEditDialog();
}


bool UIMultiplayer::CanCopyOrRenameCharacter( gpwstring & sName )
{
	if ( !gUIFrontend.IsNameValid( sName ) ) 
	{
		ShowMessageBox( gpwtranslate( $MSG$ "You cannot have a character with this name. Please enter a different name." ) );
		return false;
	}

	if ( !IsCharacterNameUnique( sName ) )
	{
		ShowMessageBox( gpwtranslate( $MSG$ "There is already a character with this name. Please enter a different name." ) );
		return false;
	}

	gpwstring errorMsg;
	if ( !TattooGame::IsValidSaveScreenName( sName, &errorMsg ) )
	{
		ShowMessageBox( errorMsg );
		return false;
	}

	// make sure we have a valid character index selected
	if ( (m_SetSelectedCharacter < 0) || (m_SetSelectedCharacter >= (int)m_MpCharacters.size()) )
	{
		return false;
	}

	return true;
}

bool UIMultiplayer::IsCharacterNameUnique( const gpwstring & sName )
{
	for ( CharacterInfoColl::iterator i = m_MpCharacters.begin(); i != m_MpCharacters.end(); ++i )
	{
		if ( (*i)->m_ScreenName.same_no_case( sName ) )
		{
			return false;
		}
	}

	return true;
}


void UIMultiplayer::RefreshCharacterScreen( void )
{
	// Refresh the ui so the user sees their newly copied character
	m_MpCharacters.clear();
	gTattooGame.GetMpCharacterSaveInfos( m_MpCharacters );
	m_MpCharacters.sort( CharacterInfo::GreaterByFileTime() );
		
	{for ( std::vector<DWORD>::iterator i = m_CharacterPortraits.begin(); i != m_CharacterPortraits.end(); ++i )
	{
		gDefaultRapi.DestroyTexture( *i );
	}}
	m_CharacterPortraits.clear();

	{for ( CharacterInfoColl::iterator i = m_MpCharacters.begin(); i != m_MpCharacters.end(); ++i )
	{
		DWORD portrait = gDefaultRapi.CreateAlgorithmicTexture( gpstringf( "portrait%d", m_CharacterPortraits.size() ).c_str(), (*i)->m_Portrait, false, 0, TEX_LOCKED, false, true );
		m_CharacterPortraits.push_back( portrait );
		(*i)->m_Portrait = NULL;
	}}

	StagingAreaShowMpCharacters();

	UISlider * pSlider = (UISlider *)gUIShell.FindUIWindow( "scroll_characters", "staging_area_select_character" );
	if ( pSlider )
	{
		pSlider->SetValue( m_SelectedCharacter - STAGING_CHARACTER_SLOTS );
	}		
}


bool MPMapHandleCompare( const FuelHandle& l, const FuelHandle& r )
{
	gpstring lstr = l->GetString( "name" );
	gpstring rstr = r->GetString( "name" );
	return ( lstr.compare_no_case( rstr ) < 0 );
}


void UIMultiplayer::SetMapStrings()
{
	UIListbox *listbox = (UIListbox *)gUIShell.FindUIWindow( "staging_map_list" );
	if ( !listbox )
	{
		return;
	}
	listbox->RemoveAllElements();

	// get maps
	gTankMgr.GetMapInfos( m_MapInfoColl );

	// re-sort based on map screen name
	m_MapInfoColl.sort( MapInfo::LessByScreenName() );

	// add to listbox
	MapInfoColl::iterator i, ibegin = m_MapInfoColl.begin(), iend = m_MapInfoColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( !i->m_IsSingleplayerOnly )
		{
			listbox->AddElement( i->m_ScreenName, std::distance( ibegin, i ) );
		}
	}
}


gpwstring UIMultiplayer::GetMapScreenName( gpstring devName )
{
	for ( MapInfoColl::iterator i = m_MapInfoColl.begin(); i != m_MapInfoColl.end(); ++i )
	{
		if ( i->m_InternalName.same_no_case( devName ) )
		{
			return ( i->m_ScreenName );
		}
	}
	return gpwstring();
}


void UIMultiplayer::EnableCharacterAppearanceSelection()
{
	SETALPHAENABLED(( "button_prev_head" ), m_pCharacterSelect->GetTextureCount( SkinSet::SKIN_HEADS ) > 1 );
	SETALPHAENABLED(( "button_next_head" ), m_pCharacterSelect->GetTextureCount( SkinSet::SKIN_HEADS ) > 1 );
	SETALPHAENABLED(( "button_prev_face" ), m_pCharacterSelect->GetTextureCount( SkinSet::SKIN_FACES ) > 1 );
	SETALPHAENABLED(( "button_next_face" ), m_pCharacterSelect->GetTextureCount( SkinSet::SKIN_FACES ) > 1 );
	SETALPHAENABLED(( "button_prev_hair" ), m_pCharacterSelect->GetTextureCount( SkinSet::SKIN_HAIR ) > 1 );
	SETALPHAENABLED(( "button_next_hair" ), m_pCharacterSelect->GetTextureCount( SkinSet::SKIN_HAIR ) > 1 );
	SETALPHAENABLED(( "button_prev_shirt" ), m_pCharacterSelect->GetTextureCount( SkinSet::SKIN_SHIRTS ) > 1 );
	SETALPHAENABLED(( "button_next_shirt" ), m_pCharacterSelect->GetTextureCount( SkinSet::SKIN_SHIRTS ) > 1 );
	SETALPHAENABLED(( "button_prev_pants" ), m_pCharacterSelect->GetTextureCount( SkinSet::SKIN_PANTS ) > 1 );
	SETALPHAENABLED(( "button_next_pants" ), m_pCharacterSelect->GetTextureCount( SkinSet::SKIN_PANTS ) > 1 );
}


gpwstring UIMultiplayer::ConstructTimeString( int totalMinutes )
{
	int hours = totalMinutes / 60;
	int minutes = totalMinutes % 60;

	gpwstring timeLimit;

	if ( hours == 1 )
	{
		timeLimit.appendf( gpwtranslate( $MSG$ "%d hour" ), hours );
	}
	else if ( hours > 1 )
	{
		timeLimit.appendf( gpwtranslate( $MSG$ "%d hours" ), hours );
	}

	if ( minutes == 1 )
	{
		if ( !timeLimit.empty() )
		{
			timeLimit.append( L" " );
		}
		timeLimit.appendf( gpwtranslate( $MSG$ "%d minute" ), minutes );
	}
	else if ( minutes > 1 )
	{
		if ( !timeLimit.empty() )
		{
			timeLimit.append( L" " );
		}
		timeLimit.appendf( gpwtranslate( $MSG$ "%d minutes" ), minutes );
	}

	if ( timeLimit.empty() )
	{
		timeLimit = gpwtranslate( $MSG$ "None" );
	}

	return ( timeLimit );
}


void UIMultiplayer::DeinitCharacterSelect()
{
	Delete( m_pCharacterSelect );
}


void UIMultiplayer::ShowStartDescription( Player * player, const gpwstring & sStart )
{
	UITextBox * pHelp = (UITextBox *)gUIShell.FindUIWindow( "multiplayer_help", "multiplayer_help" );
	const WorldMap::SGroupColl &startGroups = gWorldMap.GetStartGroups();
	for ( WorldMap::SGroupColl::const_iterator i = startGroups.begin(); i != startGroups.end(); ++i )
	{
		if ( sStart.same_no_case( (*i).sScreenName ) )
		{	
			float requiredLevel = 0.0f;			
			WorldMap::WorldStartLevels::const_iterator iLevel;
			for ( iLevel = (*i).startLevels.begin(); iLevel != (*i).startLevels.end(); ++iLevel )
			{				
				if ( (*iLevel).sWorld.same_no_case( gWorldMap.GetMpWorldName() ) )
				{
					requiredLevel	= (*iLevel).requiredLevel;
					break;
				}
			}		
	
			if ( requiredLevel != 0 )
			{				
				if ( player->GetHeroUberLevel() >= gWorldMap.GetRequiredStartingPositionLevel( (*i).sGroup ) )
				{
					gpwstring sHelp; 				
					sHelp.assignf( gpwtranslate( $MSG$ "%s\nCharacters must be level %0.0f or higher." ), (*i).sDescription.c_str(), requiredLevel );
					pHelp->SetText( sHelp );
				}
				else
				{
					gpwstring sHelp; 
					sHelp.assignf( gpwtranslate( $MSG$ "%s\n<c:0xffff0000>Characters must be level %0.0f or higher.</c>" ), (*i).sDescription.c_str(), requiredLevel );
					pHelp->SetText( sHelp );				
				}
			}
			else
			{
				pHelp->SetText( (*i).sDescription );				
			}
		}
	}
}


bool UIMultiplayer::IsCharacterImported()
{
	if ( !gServer.GetLocalHumanPlayer()->GetHeroCloneSourceTemplate().empty() &&
		  gServer.GetLocalHumanPlayer()->GetHeroCloneSourceTemplate().left( 1 ).same_no_case( ":" ) )
	{
		return true;
	}

	return false;
}


float UIMultiplayer::GetSelectedCharacterUberLevel()
{
	if ( (m_SelectedCharacter >= 0) && m_MpCharacters[ m_SelectedCharacter ] )
	{
		CharacterInfo::SkillDb::iterator skill = m_MpCharacters[ m_SelectedCharacter ]->m_Skills.find( "uber" );
		if ( skill != m_MpCharacters[ m_SelectedCharacter ]->m_Skills.end() )
		{
			return skill->second.m_Level;
		}
	}
	
	return 0.0f;
}


bool UIMultiplayer::CanCharacterJoinGame( float & requiredLevel )
{
	if ( gWorldMap.IsInitialized() && !gWorldMap.GetMpWorldName().empty() )
	{
		MpWorldColl mpWorlds;
		gWorldMap.GetMpWorlds( mpWorlds );

		for ( MpWorldColl::iterator i = mpWorlds.begin(); i != mpWorlds.end(); ++i )
		{
			if ( i->m_Name.same_no_case( gWorldMap.GetMpWorldName() ) )
			{
				requiredLevel = i->m_RequiredLevel;

				return ( gServer.GetLocalHumanPlayer()->GetHeroUberLevel() >= i->m_RequiredLevel );
			}
		}
	}

	return true;
}


bool UIMultiplayer::DoesCharacterMeetWorldLevel( float charLevel )
{
	if ( gWorldMap.IsInitialized() && !gWorldMap.GetMpWorldName().empty() )
	{
		MpWorldColl mpWorlds;
		gWorldMap.GetMpWorlds( mpWorlds );

		for ( MpWorldColl::iterator i = mpWorlds.begin(); i != mpWorlds.end(); ++i )
		{
			if ( i->m_Name.same_no_case( gWorldMap.GetMpWorldName() ) )
			{
				return ( charLevel >= i->m_RequiredLevel );
			}
		}
	}

	return true;
}


bool UIMultiplayer::IsWorldLevelValidForCharacters( float worldLevel )
{
	for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
	{
		if ( !IsValid(*i) || (*i)->IsComputerPlayer() )
		{
			continue;
		}

		if ( !(*i)->GetHeroCloneSourceTemplate().empty() && 
			 ((*i)->GetHeroUberLevel() < worldLevel) )
		{
			return false;
		}
	}
	return true;
}


bool UIMultiplayer::IsStartingGroupEnabled( gpstring sGroup )
{
	const WorldMap::SGroupColl &startGroups = gWorldMap.GetStartGroups();
	for ( WorldMap::SGroupColl::const_iterator i = startGroups.begin(); i != startGroups.end(); ++i )
	{
		if ( (*i).sGroup.same_no_case( sGroup ) )
		{
			return (*i).bEnabled;	
		}
	}

	return false;	
}


void UIMultiplayer::RefreshStartingGroups( Player * player )
{
	for ( int iPlayer = 1; iPlayer <= STAGING_PLAYER_SLOTS; ++iPlayer )
	{
		UIListbox *pListbox = (UIListbox *)gUIShell.FindUIWindow( gpstringf( "listbox_start_locations%d", iPlayer ) );
		if ( pListbox && pListbox->GetParentWindow() )
		{
			UIComboBox * pCombo = (UIComboBox *)pListbox->GetParentWindow();
			bool expandCombo = pCombo->GetExpanded();
			pCombo->SetExpanded( false );

			if ( MakePlayerId( pListbox->GetParentWindow()->GetTag() ) == PLAYERID_INVALID )
			{
				continue;
			}

			Player * pPlayer = gServer.GetPlayer( MakePlayerId( pListbox->GetParentWindow()->GetTag() ) );
			if ( !pPlayer || 
				((pPlayer != player) && (player != NULL)) )
			{
				continue;
			}

			pListbox->RemoveAllElements();

			const WorldMap::SGroupColl &startGroups = gWorldMap.GetStartGroups();
			for ( WorldMap::SGroupColl::const_iterator i = startGroups.begin(); i != startGroups.end(); ++i )
			{
				if ( i->bEnabled )
				{
					if ( pPlayer->GetHeroUberLevel() >= gWorldMap.GetRequiredStartingPositionLevel( i->sGroup ) )
					{
						pListbox->AddElement( i->sScreenName.c_str() );
					}
					else
					{
						pListbox->AddElement( i->sScreenName.c_str(), -1, 0xffff0000 );
					}
				}
			}

			pCombo->SetExpanded( expandCombo );
		}
	}
}


gpwstring UIMultiplayer::GetIpFromAddress( const gpwstring & wsAddress )
{
	UINT ipPos = wsAddress.find( L"hostname=" );
	if ( ipPos != gpwstring::npos )
	{
		ipPos += 9;
		UINT ipEndPos = wsAddress.find( L";", ipPos );
		if ( ipEndPos == gpwstring::npos )
		{
			ipEndPos = wsAddress.size();
		}
		return ( wsAddress.mid( ipPos, ipEndPos - ipPos ) );
	}
	return gpwstring::EMPTY;
}

// this is for sending messages from the staging area and end game screen.
void UIMultiplayer::ProcessChatMessage( UIEditBox *eb, const char* sInterface, bool sendToAllAsIs /*=false*/ )
{
	gpwstring chatText = eb->GetText();
	stringtool::RemoveBorderingWhiteSpace( chatText );
	if ( !chatText.empty() )
	{
		chatText = eb->GetText();
		eb->SetText( L"" );

		ProcessChatMessage( chatText, sInterface, sendToAllAsIs );
	}
}

// this is for sending messages from the staging area and end game screen.
void UIMultiplayer::ProcessChatMessage( gpwstring chatText, const char* sInterface, bool sendToAllAsIs /*=false*/  )
{
	// This is the from field to players in the game.  If we are in the staging area
	// we use the name because they might not have a character name yet.  When we
	// move into the other areas, we use the hero name...
	gpwstring sFrom;
	if ( gServer.GetLocalHumanPlayer() )
	{
		sFrom.assign( gServer.GetLocalHumanPlayer()->GetHeroName() );

		// use the hero name in game...  become the character :)
		if( IsInStagingArea( gWorldState.GetCurrentState() ) )
		{
			sFrom.assign( gServer.GetLocalHumanPlayer()->GetName() );
		}
	}

	UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_players", sInterface );
	if( pList )
	{
		gpwstring sTo;
		sTo.assign( pList->GetSelectedText() );
		stringtool::RemoveBorderingWhiteSpace( sTo );

		gpwstring message;

		if ( (eChatTag)pList->GetSelectedTag() == CHAT_ALL || sendToAllAsIs )
		{
			if( ! gUIEmotes.PrepareStringTextEmotes( chatText, message, sFrom.c_str() /*playerName*/, gpwtranslate( $MSG$ "Everyone" ) /*targetName*/, UI_TEXT_EMOTE_TYPE_GROUP ) )
			{
				if( sendToAllAsIs )
				{
					message.assign( chatText.c_str() );
				}
				else
				{
					message.assignf( gpwtranslate( $MSG$ "%s: %s" ), sFrom.c_str(), chatText.c_str() );
				}
			}

			// Send this chat to everyone, but yourself
			Server::PlayerColl players = gServer.GetPlayers();
			for ( Server::PlayerColl::iterator i = players.begin(); i != players.end(); ++i )
			{
				if ( IsValid(*i) && (!(*i)->IsLocalHumanPlayer()) && (!(*i)->IsComputerPlayer()) )
				{
					RSChatPlayer( message, (*i) );
				}
			}

			if( ! gUIEmotes.PrepareStringTextEmotes( chatText, message, sFrom.c_str() /*playerName*/, gpwtranslate( $MSG$ "Everyone" ) /*targetName*/, UI_TEXT_EMOTE_TYPE_GROUP ) )
			{
				if( sendToAllAsIs )
				{
					message.assign( chatText.c_str() );
				}
				else
				{
					message.assignf( gpwtranslate( $MSG$ "(ownerschat_color)You tell everyone: %s" ), chatText.c_str() );
				}
			}

			Chat( message );


		}
		else if ( (eChatTag)pList->GetSelectedTag() == CHAT_TEAM )
		{
			if( ! gUIEmotes.PrepareStringTextEmotes( chatText, message, sFrom.c_str() /*playerName*/, gpwtranslate( $MSG$ "your team" ) /*targetName*/, UI_TEXT_EMOTE_TYPE_INDIVIDUAL ) )
			{
				message.assignf( gpwtranslate( $MSG$ "%s tells your team: %s" ), sFrom.c_str(), chatText.c_str() );
			}
		
			// Only send this chat to your team
			Server::PlayerColl players = gServer.GetPlayers();
			for ( Server::PlayerColl::iterator i = players.begin(); i != players.end(); ++i )
			{
				if ( IsValid(*i) )
				{
					// test their.Team == our.Team
					if ( (!(*i)->IsLocalHumanPlayer()) && (gServer.GetTeam( (*i)->GetId() ) == gServer.GetTeam( gServer.GetLocalHumanPlayer()->GetId()) ) )
					{
						RSChatPlayer( message, (*i) );
					}
				}
			}
			if( ! gUIEmotes.PrepareStringTextEmotes( chatText, message, sFrom.c_str() /*playerName*/, gpwtranslate( $MSG$ "your team" ) /*targetName*/, UI_TEXT_EMOTE_TYPE_INDIVIDUAL ) )
			{
				message.assignf( gpwtranslate( $MSG$ "(ownerschat_color)You tell your team: %s" ), chatText.c_str() );
			}

			Chat( message );
		}
		else
		{
			if( ! gUIEmotes.PrepareStringTextEmotes( chatText, message, sFrom.c_str() /*playerName*/, sTo.c_str() /*targetName*/, UI_TEXT_EMOTE_TYPE_INDIVIDUAL ) )
			{
				message.assignf( gpwtranslate( $MSG$ "%s tells you: %s" ), sFrom.c_str(), chatText.c_str() );
			}
			// They must want to chat to a single player, let's try to find them
			Server::PlayerColl players = gServer.GetPlayers();
			for ( Server::PlayerColl::iterator i = players.begin(); i != players.end(); ++i )
			{
				if ( IsValid(*i) )
				{
					if ( (int)(*i)->GetId() == pList->GetSelectedTag() )
					{
						RSChatPlayer( message, (*i) );
						break;
					}
				}
			}
			
			// if we didn't find them in the player list, lets try to find them
			// in our friends list if we are on zoneMatch...  we want to see our friends too!
			if( pList->GetSelectedTag() <= (int)CHAT_FRIEND && IsOnZone() )
			{
				gUIZoneMatch.SendMessageToFriend( sTo, chatText );
			}
			
			if( ! gUIEmotes.PrepareStringTextEmotes( chatText, message, sFrom.c_str() /*playerName*/, sTo.c_str() /*targetName*/, UI_TEXT_EMOTE_TYPE_INDIVIDUAL ) )
			{
				message.assignf( gpwtranslate( $MSG$ "(ownerschat_color)You tell %s: %s" ), sTo.c_str(), chatText.c_str() );
			}

			Chat( message );
		}
	}
}


void UIMultiplayer::RSChat( gpwstring const & sChatText )
{
	FUBI_RPC_THIS_CALL( RSChat, RPC_TO_SERVER );

	RCChat( sChatText );
}


void UIMultiplayer :: RSChatPlayer( gpwstring const & sChatText, Player * player )
{
	FUBI_RPC_THIS_CALL( RSChatPlayer, RPC_TO_SERVER );

	RCChatPlayer( sChatText, player->GetMachineId() );
}


void UIMultiplayer :: RCChatPlayer( gpwstring const & sChatText, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCChatPlayer, machineId );

	Chat( sChatText );
}

void UIMultiplayer::RCChat( gpwstring const & sChatText )
{
	FUBI_RPC_THIS_CALL( RCChat, RPC_TO_ALL );

	Chat( sChatText );

}

void UIMultiplayer::Chat( gpwstring const & sChatText )
{
	gpwstring text;
	DWORD textColor = 0xFFFFFFFF;

	text.assign( sChatText );

	UIChatBox * chatBox = (UIChatBox *)gUIShell.FindUIWindow( "staging_area_chatbox" );
	if ( chatBox )
	{
		chatBox->SubmitText( text, textColor );
	}

	if ( gWorldState.GetCurrentState() == WS_GAME_ENDED )
	{
		chatBox = (UIChatBox *)gUIShell.FindUIWindow( "end_game_chatbox" );
		if ( chatBox )
		{
			chatBox->SubmitText( text, textColor );
		}
	}

	if ( IsInGame( gWorldState.GetCurrentState() ) )
	{
		chatBox = (UIChatBox *)gUIShell.FindUIWindow( "console_box" );

		GoHandle hMember( gUIPartyManager.GetSelectedCharacter() );
		if ( hMember.IsValid() )
		{
			hMember->PlayVoiceSound( "chat_talk" );
		}

		if ( chatBox )
		{
			chatBox->SubmitText( text, chatBox->GetFontColor() );
		}
	}
}


void UIMultiplayer::RSDisplayMessage( gpwstring const & message, DWORD color )
{
	FUBI_RPC_THIS_CALL( RSDisplayMessage, RPC_TO_SERVER );

	RCDisplayMessage( message, color );
}


void UIMultiplayer::RCDisplayMessage( gpwstring const & message, DWORD color, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCDisplayMessage, machineId );

	DisplayMessage( message, color );
}


void UIMultiplayer::DisplayMessage( gpwstring const & message, DWORD color )
{
	UIChatBox *chatBox = (UIChatBox *)gUIShell.FindUIWindow( "staging_area_chatbox" );
	if ( chatBox )
	{
		chatBox->SubmitText( message, color );
	}

	if ( gWorldState.GetCurrentState() == WS_GAME_ENDED )
	{
		chatBox = (UIChatBox *)gUIShell.FindUIWindow( "end_game_chatbox" );
		if ( chatBox )
		{
			chatBox->SubmitText( message, color );
		}
	}
}


//void UIMultiplayer::Chat( gpwstring const & message )
//{
//	UIChatBox *chatBox = (UIChatBox *)gUIShell.FindUIWindow( "staging_area_chatbox" );
//	if ( chatBox )
//	{
//		chatBox->SubmitText( message, chatBox->GetFontColor() );
//	}
//}


void UIMultiplayer::SKickPlayer( PlayerId playerId )
{
	CHECK_SERVER_ONLY;

	Player *kickPlayer = gServer.GetPlayer( playerId );
	if ( (kickPlayer == NULL) || (kickPlayer == gServer.GetLocalHumanPlayer()) )
	{
		return;
	}

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	if( session )
	{
		gNetPipe.DisconnectMachine( kickPlayer->GetMachineId(), NE_MACHINE_DISCONNECTED_KICKED  );
	}

	gServer.SMarkPlayerForDeletion( playerId );	
}


void UIMultiplayer::SBanPlayer( PlayerId playerId )
{
	CHECK_SERVER_ONLY;

	Player *kickPlayer = gServer.GetPlayer( playerId );
	if ( (kickPlayer == NULL) || (kickPlayer == gServer.GetLocalHumanPlayer()) )
	{
		return;
	}

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	if( session )
	{
		session->AddBanned( kickPlayer->GetMachineId() );
		gNetPipe.DisconnectMachine( kickPlayer->GetMachineId(), NE_MACHINE_DISCONNECTED_BANNED );
	}
	gServer.SMarkPlayerForDeletion( playerId );
}


void UIMultiplayer::SSetTimeLimit( float timelimit )
{
	CHECK_SERVER_ONLY;
	RCSetTimeLimit( timelimit );
}

FuBiCookie UIMultiplayer::RCSetTimeLimit( float timelimit )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSetTimeLimit, RPC_TO_ALL );

	gServer.SetSettingsDirty();

	gVictory.SetTimeLimit( timelimit );

	UIText *pText = (UIText *)gUIShell.FindUIWindow( "staging_time_limit_text" );
	if ( pText )
	{
		pText->SetText( ConstructTimeString( (int)gVictory.GetTimeLimit() / 60 ) );
	}

	return RPC_SUCCESS;
}


FuBiCookie UIMultiplayer::RSLeaveStagingArea( PlayerId playerId )
{
	FUBI_RPC_THIS_CALL_RETRY( RSLeaveStagingArea, RPC_TO_SERVER );

	Player * player = gServer.GetPlayer( playerId );
	if ( !player )
	{
		return RPC_FAILURE;
	}

	if ( player != gServer.GetLocalHumanPlayer() )
	{
		player->RSSetReadyToPlay( false );
	}

	RCLeaveStagingArea( player->GetMachineId() );

	return RPC_SUCCESS;
}


FuBiCookie UIMultiplayer::RCLeaveStagingArea( DWORD machineId )
{
	FUBI_RPC_THIS_CALL_RETRY( RCLeaveStagingArea, machineId );

	UICheckbox * pCheck = (UICheckbox *)gUIShell.FindUIWindow( "staging_checkbox_ready" );
	if ( pCheck )
	{
		if ( !gServer.GetLocalHumanPlayer()->IsReadyToPlay() && pCheck->GetState() )
		{
			pCheck->SetState( false );
		}
	}

	ShowMessageBox( gpwtranslate( $MSG$ "Are you sure you want to leave this game?" ), MP_DIALOG_LEAVE_STAGING_AREA );

	return RPC_SUCCESS;
}


void UIMultiplayer::LeaveStagingArea()
{
	Delete( m_pCharacterSelect );

	m_bRequestedLeaveGame = true;

	gNetPipe.CloseOpenSession( true );
}


void UIMultiplayer::ShowMultiplayerMain()
{
	m_MultiplayerMode = MM_NONE;

	SetIsVisible( false );

	gUIFrontend.SetIsVisible( true );
	gUIFrontend.InitGui( true );
	gUIFrontend.GuiProviderFromMP();
	
	if ( gWorldState.GetCurrentState() != WS_MP_PROVIDER_SELECT )
	{
		gWorldStateRequest( WS_MP_PROVIDER_SELECT );
	}
}


bool UIMultiplayer::CheckForTCPIP()
{
	// TCP is always the default
	return true;
}


void UIMultiplayer::ShowHostGameInterface()
{
	gUIShell.ShowInterface( "host_game" );

	gpwstring hostGameName;
	gpwstring hostGamePassword;

	FuelHandle prefs = gTattooGame.GetPrefsConfigFuel();
	if ( prefs.IsValid() )
	{
		FuelHandle hSettings = prefs->GetChildBlock( "multiplayer" );
		if ( hSettings.IsValid() )
		{
			hSettings->Get( "host_game_name", hostGameName );
		}

		gpstring readPassword;
		gConfig.Read( "multiplayer\\gamepassword", readPassword );
		TeaDecrypt( hostGamePassword, readPassword );
	}

	if ( hostGameName.empty() )
	{
		gpwstring playerName = (GetMultiplayerMode() == MM_GUN) ? gUIZoneMatch.GetPlayerName() : m_PlayerName;
		if ( !playerName.empty() )
		{
			hostGameName.assignf( gpwtranslate( $MSG$ "%s's Game" ), playerName.c_str() );
		}
	}

	UICheckbox *checkBox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_password_game" );
	if ( checkBox )
	{
		checkBox->SetState( !hostGamePassword.empty() );
	}

	UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "game_name_edit_box" );
	if ( editBox )
	{
		editBox->SetText( hostGameName );
		editBox->SetAllowInput( true );
	}

	editBox = (UIEditBox *)gUIShell.FindUIWindow( "game_password_edit_box" );
	if ( editBox )
	{
		editBox->SetText( hostGamePassword );
		editBox->SetAllowInput( false );
		editBox->SetEnabled( !hostGamePassword.empty() );
	}	
}


void UIMultiplayer::ShowMatchMakerInterface()
{
	if ( !CheckForTCPIP() )
	{
		return;
	}

	if ( !m_pUIZoneMatch->IsInitialized() )
	{
		if ( !m_pUIZoneMatch->Init() )
		{
			ShowMultiplayerMain();
			ShowMessageBox( gpwtranslate( $MSG$ "ZoneMatch could not be initialized. This could be due to an install error." ) );
			return;
		}
	}
	
	m_MultiplayerMode = MM_GUN;

	m_pUIZoneMatch->Activate();
}


void UIMultiplayer::ShowLanGamesInterface()
{
	if ( !CheckForTCPIP() )
	{
		return;
	}

	gpwstring playerName;
	FuelHandle prefs = gTattooGame.GetPrefsConfigFuel();
	if ( prefs.IsValid() )
	{
		FuelHandle hSettings = prefs->GetChildBlock( "multiplayer" );
		if ( hSettings.IsValid() )
		{
			hSettings->Get( "player_name", playerName );
		}
	}
	if ( !playerName.empty() )
	{
		m_PlayerName = playerName;
	}

	UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "lan_player_name_edit_box" );
	if ( eb )
	{
		eb->SetText( m_PlayerName );
	}

	HideInterfaces();
	gUIShell.ShowInterface( "lan_game_menu" );

	m_LanLastUpdateTime = 0.0;

	m_MultiplayerMode = MM_LAN;

	gNetPipe.RequestSessionEnumeration();
}


void UIMultiplayer::ShowInternetGamesInterface()
{
	if ( !CheckForTCPIP() )
	{
		return;
	}

	UIText *tb = (UIText *)gUIShell.FindUIWindow( "yourip_text" );
	if( tb )
	{
		gpwstring sIPAddress;
		sIPAddress.assignf( gpwtranslate( $MSG$ "Your IP address is: %s" ), ToUnicode( SysInfo::GetIPAddress() ).c_str() );
		tb->SetText( sIPAddress  );
	}

	gpwstring playerName;
	FuelHandle prefs = gTattooGame.GetPrefsConfigFuel();
	if ( prefs.IsValid() )
	{
		FuelHandle hSettings = prefs->GetChildBlock( "multiplayer" );
		if ( hSettings.IsValid() )
		{
			hSettings->Get( "player_name", playerName );
		}
	}
	if ( !playerName.empty() )
	{
		m_PlayerName = playerName;
	}

	// Get ip address history
	LoadIpAddressHistory();

	UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "ip_edit_box" );
	if ( eb )
	{
		eb->SetText( L"" );
	}

	UIListbox *lb = (UIListbox *)gUIShell.FindUIWindow( "recent_ip_listbox" );
	if ( lb )
	{
		lb->RemoveAllElements();

		int ipNum = 0;
		for ( IpColl::iterator i = m_IpHistory.begin(); i != m_IpHistory.end(); ++i, ++ipNum )
		{
			if ( !i->second.empty() )
			{
				lb->AddElement( gpwstringf( L"%s - %s", i->second.c_str(), ToUnicode( i->first ).c_str() ), ipNum );
			}
			else
			{
				lb->AddElement( ToUnicode( i->first ), ipNum );
			}
		}

		lb->SelectElement( 0 );
	}

	if ( eb && !m_IpHistory.empty() )
	{
		eb->SetText( ToUnicode( m_IpHistory[ 0 ].first ) );
	}

	eb = (UIEditBox *)gUIShell.FindUIWindow( "internet_player_name_edit_box" );
	if ( eb )
	{
		eb->SetText( m_PlayerName );
	}

	HideInterfaces();
	gUIShell.ShowInterface( "internet_game_menu" );

	m_MultiplayerMode = MM_NET;
}


////////////////////////////////////////////////////////////////////////////////
//	
void UIMultiplayer::ShowStagingAreaClient()
{
	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	gpassert( session );

	if ( GetMultiplayerMode() == MM_GUN )
	{
		gUIZoneMatch.OnEnterStagingArea();
	}

	HideInterfaces();
	gUIShell.ShowInterface( "staging_area" );
	gUIShell.ShowInterface( "staging_area_players" );
	m_currentPlayers = 0;

	if ( !m_PlayerName.empty() )
	{
		FuelHandle prefs = gTattooGame.GetPrefsConfigFuel( true );
		if ( prefs.IsValid() )
		{
			FuelHandle hSettings = prefs->GetChildBlock( "multiplayer", true );
			if ( hSettings.IsValid() )
			{
				hSettings->Set( "player_name", m_PlayerName );
			}
			prefs->GetDB()->SaveChanges();
		}
	}

	m_CharacterName.clear();

	// disable server-only controls
	HIDE(( "staging_area_button_start_game" ));
	HIDE(( "button_map_settings" ));
	HIDE(( "button_game_settings" ));
	StagingAreaEnableSelections( true );

	// initialize interface
	StagingAreaShowTeamColumn( gVictory.GetTeamMode() != Victory::NO_TEAMS );

	for ( int i = 1; i <= STAGING_PLAYER_SLOTS; ++i )
	{
		StagingAreaClearPlayerSlot( i );
	}

	UIEditBox *pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_area_chat_editbox" );
	if ( pEditBox )
	{
		pEditBox->SetPermanentFocus( true );
		pEditBox->SetAllowInput( true );
		pEditBox->SetText( L"" );
		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{
			UIWindow *pWindow = gUIShell.FindUIWindow( "staging_send_to", "staging_area" );
			if ( pWindow )
			{
				pEditBox->SetRect( pEditBox->GetRect().left, pWindow->GetRect().left - 30, pEditBox->GetRect().top, pEditBox->GetRect().bottom );
			}
		}
	}

	pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_char_name_edit_box" );
	if ( pEditBox )
	{
		pEditBox->SetText( L"" );
		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{
			UIWindow *pWindow = gUIShell.FindUIWindow( "name_background", "staging_area_select_character" );
			if ( pWindow )
			{
				pEditBox->SetRect( pEditBox->GetRect().left, pWindow->GetRect().right - 30, pEditBox->GetRect().top, pEditBox->GetRect().bottom );
			}
		}
	}

	UICheckbox *pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "staging_checkbox_ready" );
	if ( pCheckbox )
	{
		pCheckbox->SetVisible( true );
		pCheckbox->SetState( false );
	}

	UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "checkbox_ready_text" );
	if ( pTextBox )
	{
		if ( m_CharacterName.empty() )
		{
			pTextBox->SetTextColor( 0x80808080 );
		}
		else
		{
			pTextBox->SetTextColor( 0xFFFFFFFF );
		}
	}

	UIChatBox *pChatBox = (UIChatBox *)gUIShell.FindUIWindow( "staging_area_chatbox" );
	if ( pChatBox )
	{
		pChatBox->Clear();
	}

	SetMapStrings();

	// display current game settings
	ShowStagingAreaGameSettings();
	ShowGameSettingsList( true );

	// init
	m_bImportedChar = false;
	m_bRequestedLeaveGame = false;
	m_bWasDisconnected = false;
	m_SelectedMap.clear();
	m_PendingMessageText.clear();
	m_TimeToShowPendingMessage = 0.0f;

	// character selection
	m_MpCharacters.clear();
	gTattooGame.GetMpCharacterSaveInfos( m_MpCharacters );
	m_MpCharacters.sort( CharacterInfo::GreaterByFileTime() );

	{for ( std::vector<DWORD>::iterator i = m_CharacterPortraits.begin(); i != m_CharacterPortraits.end(); ++i )
	{
		gDefaultRapi.DestroyTexture( *i );
	}}
	m_CharacterPortraits.clear();

	{for ( CharacterInfoColl::iterator i = m_MpCharacters.begin(); i != m_MpCharacters.end(); ++i )
	{
		DWORD portrait = gDefaultRapi.CreateAlgorithmicTexture( gpstringf( "portrait%d", m_CharacterPortraits.size() ).c_str(), (*i)->m_Portrait, false, 0, TEX_LOCKED, false, true );
		m_CharacterPortraits.push_back( portrait );
		(*i)->m_Portrait = NULL;
	}}

	Delete( m_pCharacterSelect );
	m_pCharacterSelect = new UICharacterSelect( true );
	m_pCharacterSelect->AddAllCharacters();	

	StagingAreaUpdatePlayerList();
	UpdatePlayerListBox();
}


////////////////////////////////////////////////////////////////////////////////
//	
void UIMultiplayer::ShowStagingAreaServer()
{
	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	gpassert( session );

	if ( GetMultiplayerMode() == MM_GUN )
	{
		gUIZoneMatch.OnEnterStagingArea();
	}

	HideInterfaces();
	gUIShell.ShowInterface( "staging_area" );
	gUIShell.ShowInterface( "staging_area_players" );
	m_currentPlayers = 0;

	if ( !m_PlayerName.empty() )
	{
		FuelHandle prefs = gTattooGame.GetPrefsConfigFuel( true );
		if ( prefs.IsValid() )
		{
			FuelHandle hSettings = prefs->GetChildBlock( "multiplayer", true );
			if ( hSettings.IsValid() )
			{
				hSettings->Set( "player_name", m_PlayerName );
			}
			prefs->GetDB()->SaveChanges();
		}
	}

	// enable server-only controls
	SHOW(( "staging_area_button_start_game" ));
	SHOW(( "button_map_settings" ));
	SHOW(( "button_game_settings" ));
	StagingAreaEnableSelections( true );

	// initialize interface
	StagingAreaShowTeamColumn( gVictory.GetTeamMode() != Victory::NO_TEAMS );

	for ( int i = 1; i <= STAGING_PLAYER_SLOTS; ++i )
	{
		UIWindow *pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_ping", i ) );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}
	}

	UIEditBox *pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_area_chat_editbox" );
	if ( pEditBox )
	{
		pEditBox->SetPermanentFocus( true );
		pEditBox->SetAllowInput( true );
		pEditBox->SetText( L"" );
		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{
			UIWindow *pWindow = gUIShell.FindUIWindow( "staging_send_to", "staging_area" );
			if ( pWindow )
			{
				pEditBox->SetRect( pEditBox->GetRect().left, pWindow->GetRect().left - 30, pEditBox->GetRect().top, pEditBox->GetRect().bottom );
			}
		}
	}

	pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "staging_char_name_edit_box" );
	if ( pEditBox )
	{
		pEditBox->SetText( L"" );
		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{
			UIWindow *pWindow = gUIShell.FindUIWindow( "name_background", "staging_area_select_character" );
			if ( pWindow )
			{
				pEditBox->SetRect( pEditBox->GetRect().left, pWindow->GetRect().right - 30, pEditBox->GetRect().top, pEditBox->GetRect().bottom );
			}
		}
	}

	UICheckbox *pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "staging_checkbox_ready" );
	if ( pCheckbox )
	{
		pCheckbox->SetVisible( false );
	}

	UITextBox *pTextBox = (UITextBox *)gUIShell.FindUIWindow( "checkbox_ready_text" );
	if ( pTextBox )
	{
		if ( m_CharacterName.empty() )
		{
			pTextBox->SetTextColor( 0x80808080 );
		}
		else
		{
			pTextBox->SetTextColor( 0xFFFFFFFF );
		}
	}

	UIChatBox *pChatBox = (UIChatBox *)gUIShell.FindUIWindow( "staging_area_chatbox" );
	if ( pChatBox )
	{
		pChatBox->Clear();
	}

	SetMapStrings();

	// load saved settings
	
	// default map to the mpmap= parameter on the command line
	gpstring selectedMap = gConfig.GetString( "mpmap" );
	if ( !selectedMap.empty() && gTankMgr.HasMap( selectedMap ) )
	{
		gWorldMap.SSet( selectedMap, NULL /*$$$MPWORLD$$$*/, RPC_TO_ALL );
	}

	LoadGameSettings();

	if ( !gWorldMap.IsInitialized() )
	{
		// try and set map to the default
		FastFuelHandle hSettings( "config:multiplayer:settings" );
		if ( hSettings.IsValid() )
		{
			hSettings.Get( "default_multiplayer_map", selectedMap );
			if ( !selectedMap.empty() && gTankMgr.HasMap( selectedMap ) )
			{
				gWorldMap.SSet( selectedMap, NULL /*$$$MPWORLD$$$*/, RPC_TO_ALL );
			}
		}
	}
	
	if ( !gWorldMap.IsInitialized() )
	{
		// try and set first valid map
		MapInfoColl::iterator i, ibegin = m_MapInfoColl.begin(), iend = m_MapInfoColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( gWorldMap.SSet( i->m_InternalName, NULL, RPC_TO_ALL ) )
			{
				break;
			}
		}
	}

	if ( !gWorldMap.IsInitialized() )
	{
		LeaveStagingArea();
		ShowMessageBox( gpwtranslate( $MSG$ "Unable to host a game. No valid maps were found." ) );
		return;
	}

	if ( gServer.GetLocalHumanPlayer() )
	{
		LoadPlayerSettings();
	}

	// update settings

	if ( gUIZoneMatch.IsHosting() )
	{
		gUIZoneMatch.HostUpdateGameSettings();
	}

	// show game settings

	ShowStagingAreaGameSettings();
	ShowGameSettingsList( true );

	// character selection

	m_MpCharacters.clear();
	gTattooGame.GetMpCharacterSaveInfos( m_MpCharacters );
	m_MpCharacters.sort( CharacterInfo::GreaterByFileTime() );

	{for ( std::vector<DWORD>::iterator i = m_CharacterPortraits.begin(); i != m_CharacterPortraits.end(); ++i )
	{
		gDefaultRapi.DestroyTexture( *i );
	}}
	m_CharacterPortraits.clear();

	{for ( CharacterInfoColl::iterator i = m_MpCharacters.begin(); i != m_MpCharacters.end(); ++i )
	{
		DWORD portrait = gDefaultRapi.CreateAlgorithmicTexture( gpstringf( "portrait%d", m_CharacterPortraits.size() ).c_str(), (*i)->m_Portrait, false, 0, TEX_LOCKED, false, true );
		m_CharacterPortraits.push_back( portrait );
		(*i)->m_Portrait = NULL;
	}}

	Delete( m_pCharacterSelect );
	m_pCharacterSelect = new UICharacterSelect( true );
	m_pCharacterSelect->AddAllCharacters();	

	// init

	m_bImportedChar = false;
	m_SelectedMap.clear();
	m_bRequestedLeaveGame = false;
	m_bWasDisconnected = false;
	m_PendingMessageText.clear();
	m_TimeToShowPendingMessage = 0.0f;

	StagingAreaUpdatePlayerList();
	UpdatePlayerListBox();
}


void UIMultiplayer::ShowStagingAreaGameSettings()
{
	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	if ( !session )
	{
		return;
	}

	UIText *pText = (UIText *)gUIShell.FindUIWindow( "staging_time_limit_text" );
	if ( pText )
	{
		pText->SetText( ConstructTimeString( (int)gVictory.GetTimeLimit() / 60 ) );
	}

	unsigned int numPlayers = 0;
	{for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
	{
		if ( !IsValid(*i) || (*i)->IsComputerPlayer() )
		{
			continue;
		}

		numPlayers++;
	}}

	for ( unsigned int playerSlot = 1; playerSlot <= STAGING_PLAYER_SLOTS; ++playerSlot )
	{
		// start locations
		UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( gpstringf( "combo_start_locations%d", playerSlot ) );
		if ( pCombo )
		{
			if ( gServer.IsLocal() && !gServer.GetAllowStartLocationSelection() && playerSlot <= numPlayers )
			{
				pCombo->SetVisible( true );
			}
			else
			{
				if ( gServer.GetLocalHumanPlayer() &&
					 ((DWORD)pCombo->GetTag() == MakeInt( gServer.GetLocalHumanPlayer()->GetId() )) )
				{
					if ( gServer.IsLocal() || gServer.GetAllowStartLocationSelection() )
					{
						pCombo->SetVisible( true );
					}
					else
					{
						pCombo->SetVisible( false );
					}
				}
				else
				{
					pCombo->SetVisible( false );
				}
			}

			UIWindow *pWindow = gUIShell.FindUIWindow( gpstringf( "start_location%d_bg", playerSlot ) );
			if ( pWindow )
			{
				pWindow->SetVisible( pCombo->GetVisible() );
			}
		}

		// hide unused player slots
		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
		gpassert( session );
		bool showSlot = true;

		if ( (playerSlot > session->GetNumMachinesMax()) &&
			 (playerSlot > session->GetNumMachinesPresent()) )
		{
			showSlot = false;
		}

		UIWindow *pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_bg", playerSlot ) );
		if ( pWindow )
		{
			pWindow->SetVisible( showSlot );
		}
		if ( m_bShowTeams )
		{
			pWindow = gUIShell.FindUIWindow( gpstringf( "player_team_sep%d", playerSlot ) );
			if ( pWindow )
			{
				pWindow->SetVisible( showSlot );
			}
		}
		pWindow = gUIShell.FindUIWindow( gpstringf( "team_start_sep%d", playerSlot ) );
		if ( pWindow )
		{
			pWindow->SetVisible( showSlot );
		}
		pWindow = gUIShell.FindUIWindow( gpstringf( "start_char_sep%d", playerSlot ) );
		if ( pWindow )
		{
			pWindow->SetVisible( showSlot );
		}
	}

	//
	// Game Settings Interface

	UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_player_limit" );
	if ( pCombo )
	{
		pCombo->SetText( gpwstringf( gpwtranslate( $MSG$ "%d Players" ), gServer.GetMaxPlayers() ) );
		pCombo->SetEnabled( gServer.IsLocal() );
	}

	pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_game_type" );
	if ( pCombo )
	{
		if ( gWorldMap.IsInitialized() )
		{
			pCombo->SetText( gVictory.GetGameType()->GetScreenName() );
		}
		pCombo->SetEnabled( gServer.IsLocal() );
	}

	pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_difficulty" );
	if ( pCombo )
	{
		if ( gWorldOptions.GetDifficulty() == DIFFICULTY_EASY )
		{
			pCombo->SetText( gpwtranslate( $MSG$ "Easy" ) );
		}
		else if ( gWorldOptions.GetDifficulty() == DIFFICULTY_MEDIUM )
		{
			pCombo->SetText( gpwtranslate( $MSG$ "Normal" ) );
		}
		else if ( gWorldOptions.GetDifficulty() == DIFFICULTY_HARD )
		{
			pCombo->SetText( gpwtranslate( $MSG$ "Hard" ) );
		}
		pCombo->SetEnabled( gServer.IsLocal() );
	}

	pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_time_limit" );
	if ( pCombo )
	{
		pCombo->SetSelectedTag( (int)gVictory.GetTimeLimit() / 60 );
		pCombo->SetText( ConstructTimeString( pCombo->GetSelectedTag() ) );
		pCombo->SetEnabled( gServer.IsLocal() );
	}

	pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_characters_drop" );
	if ( pCombo )
	{
		if ( gServer.GetDropInvOption() == DIO_NOTHING )
		{
			pCombo->SetText( gpwtranslate( $MSG$ "Nothing" ) );
		}
		else if ( gServer.GetDropInvOption() == DIO_EQUIPPED )
		{
			pCombo->SetText( gpwtranslate( $MSG$ "Equipped Items" ) );
		}
		else if ( gServer.GetDropInvOption() == DIO_INVENTORY )
		{
			pCombo->SetText( gpwtranslate( $MSG$ "Inventory" ) );
		}
		else if ( gServer.GetDropInvOption() == DIO_ALL )
		{
			pCombo->SetText( gpwtranslate( $MSG$ "All" ) );
		}		
		pCombo->SetEnabled( gServer.IsLocal() );
	}

	pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_character_respawn" );
	if ( pCombo )
	{
		pCombo->SetText( gServer.GetAllowRespawn() ? gpwtranslate( $MSG$ "Respawn at Corpse" ) : gpwtranslate( $MSG$ "No Respawn" ) );
		pCombo->SetEnabled( gServer.IsLocal() );
	}

	UICheckbox * pPvpCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_enable_pvp" );
	UICheckbox * pTeamCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_enable_teamplay" );
	if ( pPvpCheckbox && pTeamCheckbox && gVictory.GetGameType() )
	{	
		gpstring gameType = gVictory.GetGameType()->GetName();

		if ( gameType.same_no_case( "coop_no_teams" ) )
		{
			pPvpCheckbox->SetState( false );
			pTeamCheckbox->SetState( false );
		}
		else if ( gameType.same_no_case( "coop_teams" ) )
		{
			pPvpCheckbox->SetState( false );
			pTeamCheckbox->SetState( true );
		}
		else if ( gameType.same_no_case( "pvp_no_teams" ) )
		{
			pPvpCheckbox->SetState( true );
			pTeamCheckbox->SetState( false );
		}
		else if ( gameType.same_no_case( "pvp_teams" ) )
		{
			pPvpCheckbox->SetState( true );
			pTeamCheckbox->SetState( true );
		}	
	}

	UICheckbox *pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_allow_start_selection" );
	if ( pCheckbox )
	{
		pCheckbox->SetState( gServer.GetAllowStartLocationSelection() );
		pCheckbox->SetEnabled( gServer.IsLocal() );
	}

	pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_allow_new_characters" );
	if ( pCheckbox )
	{
		pCheckbox->SetState( gServer.GetAllowNewCharactersOnly() );
		pCheckbox->SetEnabled( gServer.IsLocal() );
	}

	pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_allow_player_pausing" );
	if ( pCheckbox )
	{
		pCheckbox->SetState( gServer.GetAllowPausing() );
		pCheckbox->SetEnabled( gServer.IsLocal() );
	}

	pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_allow_player_joining" );
	if ( pCheckbox )
	{
		pCheckbox->SetState( gServer.GetAllowJIP() );
		pCheckbox->SetEnabled( gServer.IsLocal() );
	}

	ShowGameSettingsList();
}


void UIMultiplayer::ShowGameSettingsList( bool clearList )
{
	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	if ( !session )
	{
		return;
	}

	UIListbox * pListbox = NULL;
	
	if ( IsStagingArea( gWorldState.GetCurrentState() ) )
	{
		pListbox = (UIListbox *)gUIShell.FindUIWindow( "staging_game_settings_listbox" );
	}
	else if ( IsInGame( gWorldState.GetCurrentState() ) )
	{
		pListbox = (UIListbox *)gUIShell.FindUIWindow( "in_game_settings_listbox" );
		clearList = true;
	}

	if ( !pListbox )
	{
		return;
	}

	if ( (session->GetHasPassword() && (pListbox->GetTagElement( (int)GS_PASSWORD ) == NULL)) ||
		 (!session->GetHasPassword() && (pListbox->GetTagElement( (int)GS_PASSWORD ) != NULL)) )
	{
		clearList = true;
	}

	if ( clearList )
	{
		pListbox->RemoveAllElements();
	}

	if ( pListbox->GetNumElements() == 0 )
	{
		if ( !IsInGame( gWorldState.GetCurrentState() ) )
		{
			pListbox->AddElement( gpwstringf( gpwtranslate( $MSG$ "Game: %s" ), session->GetSessionName().c_str() ) );
		}

		if ( (GetMultiplayerMode() == MM_NET) || IsInGame( gWorldState.GetCurrentState() ) )
		{
			pListbox->AddElement( gpwtranslate( $MSG$ "Host IP Address" ), (int)GS_IP_ADDRESS );
		}

		if ( session->GetHasPassword() )
		{
			pListbox->AddElement( gpwtranslate( $MSG$ "Password" ), (int)GS_PASSWORD );
		}

		pListbox->AddElement( gpwtranslate( $MSG$ "Map" ), (int)GS_MAP );
		pListbox->AddElement( gpwtranslate( $MSG$ "World Difficulty" ), (int)GS_WORLD );
		pListbox->AddElement( gpwtranslate( $MSG$ "Player Limit" ), (int)GS_PLAYER_LIMIT );
		pListbox->AddElement( gpwtranslate( $MSG$ "Difficulty" ), (int)GS_DIFFICULTY );
		pListbox->AddElement( gpwtranslate( $MSG$ "Time Limit" ), (int)GS_TIME_LIMIT );
		pListbox->AddElement( gpwtranslate( $MSG$ "Drop on Death" ), (int)GS_DROP_ON_DEATH );
		pListbox->AddElement( gpwtranslate( $MSG$ "Team Play" ), (int)GS_TEAM_PLAY );
		pListbox->AddElement( gpwtranslate( $MSG$ "PvP" ), (int)GS_PVP );
		pListbox->AddElement( gpwtranslate( $MSG$ "Select Locations" ), (int)GS_SELECT_LOCATIONS );
		pListbox->AddElement( gpwtranslate( $MSG$ "New Characters Only" ), (int)GS_ALLOW_NEW_CHARACTERS );
		pListbox->AddElement( gpwtranslate( $MSG$ "Pausing" ), (int)GS_ALLOW_PAUSING );
		pListbox->AddElement( gpwtranslate( $MSG$ "Join in Progress" ), (int)GS_JIP );
	}

	// Refresh settings
	// BUG 12699 the if you are the host, then the host's ip is not defined, so we just use our ip.
	if( session->IsHost() )
	{
		pListbox->SetElementText( (int)GS_IP_ADDRESS, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "Host IP Address" ).c_str(), ToUnicode( SysInfo::GetIPAddress() ).c_str() ), 0xFFFFFFFF );	
	}
	else
	{
		pListbox->SetElementText( (int)GS_IP_ADDRESS, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "Host IP Address" ).c_str(), GetIpFromAddress( session->GetHostAddress() ).c_str() ), 0xFFFFFFFF );	
	}

	pListbox->SetElementText( (int)GS_PASSWORD, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "Password" ).c_str(), session->GetSessionPassword().c_str() ), 0xFFFFFFFF );

	if ( gWorldMap.IsInitialized() )
	{
		pListbox->SetElementText( GS_MAP, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "Map" ).c_str(), gWorldMap.GetMapScreenName().c_str() ), 0xFFFFFFFF );	

		if ( gWorldMap.GetMpWorldName().empty() )
		{
			pListbox->SetElementText( GS_WORLD, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "World" ).c_str(), gpwtranslate( $MSG$ "None" ).c_str() ), 0xFFFFFFFF );
		}
		else
		{
			MpWorldColl mpWorlds;
			gWorldMap.GetMpWorlds( mpWorlds );

			for ( MpWorldColl::iterator i = mpWorlds.begin(); i != mpWorlds.end(); ++i )
			{
				if ( i->m_Name.same_no_case( gWorldMap.GetMpWorldName() ) )
				{
					pListbox->SetElementText( GS_WORLD, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "World" ).c_str(), i->m_ScreenName.c_str() ), 0xFFFFFFFF );
					break;
				}
			}
		}
	}

	pListbox->SetElementText( GS_PLAYER_LIMIT, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "Player Limit" ).c_str(), gpwstringf( L"%d", gServer.GetMaxPlayers() ).c_str() ), 0xFFFFFFFF );

	if ( gWorldOptions.GetDifficulty() == DIFFICULTY_EASY )
	{
		pListbox->SetElementText( GS_DIFFICULTY, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "Difficulty" ).c_str(), gpwtranslate( $MSG$ "Easy" ).c_str() ), 0xFFFFFFFF );
	}
	else if ( gWorldOptions.GetDifficulty() == DIFFICULTY_MEDIUM )
	{
		pListbox->SetElementText( GS_DIFFICULTY, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "Difficulty" ).c_str(), gpwtranslate( $MSG$ "Normal" ).c_str() ), 0xFFFFFFFF );
	}
	else if ( gWorldOptions.GetDifficulty() == DIFFICULTY_HARD )
	{
		pListbox->SetElementText( GS_DIFFICULTY, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "Difficulty" ).c_str(), gpwtranslate( $MSG$ "Hard" ).c_str() ), 0xFFFFFFFF );
	}

	pListbox->SetElementText( GS_TIME_LIMIT, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "Time Limit" ).c_str(), ConstructTimeString( (int)gVictory.GetTimeLimit() / 60 ).c_str() ), 0xFFFFFFFF );

	// Player Drop
	{
		gpwstring sText;
		if ( gServer.GetDropInvOption() == DIO_NOTHING )
		{
			sText = gpwtranslate( $MSG$ "Nothing" );
		}
		else if ( gServer.GetDropInvOption() == DIO_EQUIPPED )
		{
			sText = gpwtranslate( $MSG$ "Equipped Items" );
		}
		else if ( gServer.GetDropInvOption() == DIO_INVENTORY )
		{
			sText = gpwtranslate( $MSG$ "Inventory" );
		}
		else if ( gServer.GetDropInvOption() == DIO_ALL )
		{
			sText = gpwtranslate( $MSG$ "All" );
		}	

		pListbox->SetElementText( GS_DROP_ON_DEATH, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "Drop on Death" ).c_str(), sText.c_str() ), 0xFFFFFFFF );
	}

	// Team Play / PvP
	if ( gVictory.GetGameType() )
	{
		gpwstring sTeamPlay;
		gpwstring sPvp;
		gpstring gameType = gVictory.GetGameType()->GetName();
		if ( gameType.same_no_case( "coop_no_teams" ) )
		{
			sTeamPlay = gpwtranslate( $MSG$ "No" );
			sPvp = gpwtranslate( $MSG$ "No" );
		}
		else if ( gameType.same_no_case( "coop_teams" ) )
		{
			sTeamPlay = gpwtranslate( $MSG$ "Yes" );
			sPvp = gpwtranslate( $MSG$ "No" );
		}
		else if ( gameType.same_no_case( "pvp_no_teams" ) )
		{
			sTeamPlay = gpwtranslate( $MSG$ "No" );
			sPvp = gpwtranslate( $MSG$ "Yes" );
		}
		else if ( gameType.same_no_case( "pvp_teams" ) )
		{
			sTeamPlay = gpwtranslate( $MSG$ "Yes" );
			sPvp = gpwtranslate( $MSG$ "Yes" );
		}

		pListbox->SetElementText( GS_TEAM_PLAY, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "Team Play" ).c_str(), sTeamPlay.c_str() ), 0xFFFFFFFF );
		pListbox->SetElementText( GS_PVP, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "PvP" ).c_str(), sPvp.c_str() ), 0xFFFFFFFF );
	}


	// Select Locations
	pListbox->SetElementText(	GS_SELECT_LOCATIONS, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "Select Locations" ).c_str(), 
								gServer.GetAllowStartLocationSelection() ? gpwtranslate( $MSG$ "Yes" ).c_str() : gpwtranslate( $MSG$ "No" ).c_str() ), 0xFFFFFFFF );
	pListbox->SetElementText(	GS_ALLOW_NEW_CHARACTERS, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "New Characters Only" ).c_str(), 
								gServer.GetAllowNewCharactersOnly() ? gpwtranslate( $MSG$ "Yes" ).c_str() : gpwtranslate( $MSG$ "No" ).c_str() ), 0xFFFFFFFF );
	pListbox->SetElementText(	GS_ALLOW_PAUSING, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "Pausing" ).c_str(), 
								gServer.GetAllowPausing() ? gpwtranslate( $MSG$ "Yes" ).c_str() : gpwtranslate( $MSG$ "No" ).c_str() ), 0xFFFFFFFF );
	pListbox->SetElementText(	GS_JIP, 0, gpwstringf( SETTING_FMT, gpwtranslate( $MSG$ "Join in Progress" ).c_str(), 
								gServer.GetAllowJIP() ? gpwtranslate( $MSG$ "Yes" ).c_str() : gpwtranslate( $MSG$ "No" ).c_str() ), 0xFFFFFFFF );
}


void UIMultiplayer::ShowJIPInterface()
{
	gUIShell.ShowInterface( "join_in_progress" );

	// Hide/show team column
	int playerNameRight = 0;

	if ( gVictory.GetTeamMode() == Victory::NO_TEAMS )
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( "header_start_location_text", "join_in_progress" );
		if ( pWindow )
		{
			playerNameRight = pWindow->GetRect().left;
		}
	}

	UIWindow * pWindow = gUIShell.FindUIWindow( "header_team_text", "join_in_progress" );
	if ( pWindow )
	{
		pWindow->SetVisible( gVictory.GetTeamMode() != Victory::NO_TEAMS );
		if ( gVictory.GetTeamMode() != Victory::NO_TEAMS )
		{
			playerNameRight = pWindow->GetRect().left;
		}
	}

	for ( int jipSlot = 1; jipSlot <= 7; ++jipSlot )
	{
		UIText * pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "player%d_text", jipSlot ), "join_in_progress" );
		if ( pText )
		{
			pText->SetRect( pText->GetRect().left, playerNameRight, pText->GetRect().top, pText->GetRect().bottom );
		}
	}

	UpdateJIPInterface();
}


void UIMultiplayer::HideJIPInterface()
{
	gUIShell.HideInterface( "join_in_progress" );

	for ( int jipSlot = 1; jipSlot <= 7; ++jipSlot )
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_sign", jipSlot ), "join_in_progress" );
		if ( pWindow )
		{
			pWindow->SetTextureIndex( 0 );
			pWindow->SetHasTexture( false );
		}
	}
}


void UIMultiplayer::UpdateJIPInterface()
{
	int jipSlot = 1;

	{for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
	{
		if ( !IsValid(*i) || (*i)->IsComputerPlayer() )
		{
			continue;
		}
		
		if ( !IsInGame( (*i)->GetWorldState() ) )
		{
			UIWindow * pPlayerBg = gUIShell.FindUIWindow( gpstringf( "player%d_bg", jipSlot ), "join_in_progress" );
			if ( !pPlayerBg )
			{
				continue;
			}

			// resize interface for the number of slots shown

			UIWindow * pPlayersBg = gUIShell.FindUIWindow( "players_bg", "join_in_progress" );
			if ( pPlayersBg )
			{
				pPlayersBg->SetVisible( true );
				pPlayersBg->GetRect().bottom = pPlayerBg->GetRect().bottom + 2;
			}

			UIWindow * pBg = gUIShell.FindUIWindow( "background", "join_in_progress" );
			if ( pBg )
			{
				int bgHeight = pBg->GetRect().Height();

				pBg->GetRect().bottom = pPlayerBg->GetRect().bottom + 22;

				if ( bgHeight != pBg->GetRect().Height() )
				{
					GUInterface *pInterface = gUIShell.FindInterface( "join_in_progress" );
					if ( pInterface )
					{
						gUIShell.CenterInterface( *pInterface );
					}
				}
			}

			// show player slot
			
			pPlayerBg->SetVisible( true );
			pPlayerBg->SetTag( MakeInt( (*i)->GetId() ) );

			UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_progress", jipSlot ), "join_in_progress" );
			if ( pWindow )
			{
				if ( IsLoading( (*i)->GetWorldState() ) )
				{
					float loadProgress = min( (*i)->GetLoadProgress(), 1.0f );
					pWindow->SetRect( pWindow->GetRect().left, pWindow->GetRect().left + (int)(loadProgress * pPlayerBg->GetRect().Width()), pWindow->GetRect().top, pWindow->GetRect().bottom );
					pWindow->SetVisible( true );
				}
				else
				{
					pWindow->SetVisible( false );
				}
			}

			UIText * pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "player%d_text", jipSlot ), "join_in_progress" );
			if ( pText )
			{
				pText->SetVisible( true );
				pText->SetText( (*i)->GetName() );
			}

			if ( m_bShowTeams )
			{
				UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_sign", jipSlot ), "join_in_progress" );
				if ( pWindow )
				{
					pWindow->SetVisible( true );

					Team * team = gServer.GetTeam( (*i)->GetId() );
					if ( team && (team->m_TextureId != 0) )
					{
						pWindow->SetTextureIndex( team->m_TextureId );
						pWindow->SetHasTexture( true );
					}
					else
					{
						pWindow->SetTextureIndex( 0 );
						pWindow->SetHasTexture( false );
					}
				}
			}			

			pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "startlocation%d_text", jipSlot ), "join_in_progress" );
			if ( pText )
			{
				pText->SetVisible( true );

				gpwstring startGroup;
				const WorldMap::SGroupColl &startGroups = gWorldMap.GetStartGroups();
				for ( WorldMap::SGroupColl::const_iterator iGroup = startGroups.begin(); iGroup != startGroups.end(); ++iGroup )
				{
					if ( iGroup->sGroup.same_no_case( (*i)->GetStartingGroup() ) )
					{
						startGroup = iGroup->sScreenName;
						break;
					}
				}

				pText->SetText( startGroup );
			}

			pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "character%d_text", jipSlot ), "join_in_progress" );
			if ( pText )
			{
				pText->SetVisible( true );
				pText->SetText( (*i)->GetHeroName() );
			}

			++jipSlot;
		}
	}}

	if ( jipSlot == 1 )
	{
		// no slots are currently visible

		UIWindow * pPlayersBg = gUIShell.FindUIWindow( "players_bg", "join_in_progress" );
		if ( pPlayersBg )
		{
			pPlayersBg->SetVisible( false );
		}

		UIWindow * pBg = gUIShell.FindUIWindow( "background", "join_in_progress" );
		if ( pBg && pPlayersBg )
		{
			int bgHeight = pBg->GetRect().Height();

			pBg->GetRect().bottom = pPlayersBg->GetRect().top + 22;

			if ( bgHeight != pBg->GetRect().Height() )
			{
				GUInterface *pInterface = gUIShell.FindInterface( "join_in_progress" );
				if ( pInterface )
				{
					gUIShell.CenterInterface( *pInterface );
				}
			}
		}
	}

	// clear out remaining slots
	{for ( int i = jipSlot; i <= 7; ++i )
	{
		UIWindow * pSlot = gUIShell.FindUIWindow( gpstringf( "player%d_bg", i ), "join_in_progress" );
		if ( pSlot )
		{
			pSlot->SetVisible( false );
		}
		pSlot = gUIShell.FindUIWindow( gpstringf( "player%d_progress", i ), "join_in_progress" );
		if ( pSlot )
		{
			pSlot->SetVisible( false );
		}
		pSlot = gUIShell.FindUIWindow( gpstringf( "player%d_text", i ), "join_in_progress" );
		if ( pSlot )
		{
			pSlot->SetVisible( false );
		}
		pSlot = gUIShell.FindUIWindow( gpstringf( "team%d_sign", i ), "join_in_progress" );
		if ( pSlot )
		{
			pSlot->SetVisible( false );
		}
		pSlot = gUIShell.FindUIWindow( gpstringf( "character%d_text", i ), "join_in_progress" );
		if ( pSlot )
		{
			pSlot->SetVisible( false );
		}
		pSlot = gUIShell.FindUIWindow( gpstringf( "startlocation%d_text", i ), "join_in_progress" );
		if ( pSlot )
		{
			pSlot->SetVisible( false );
		}
	}}
}


void UIMultiplayer::ShowPlayersPanel()
{
	m_bPlayersPanelVisible = true;

	gUIShell.ShowInterface( "in_game_player_panel" );
	gUIShell.HideInterface( "in_game_player_panel_characters" );

	UpdatePlayersPanel();
}


void UIMultiplayer::ShowPlayerCharactersPanel()
{
	m_bPlayersPanelVisible = true;

	gUIShell.HideInterface( "in_game_player_panel" );
	gUIShell.ShowInterface( "in_game_player_panel_characters" );

	UpdatePlayersPanel();
}


void UIMultiplayer::HidePlayersPanel()
{
	m_bPlayersPanelVisible = false;

	gUIShell.HideInterface( "in_game_player_panel" );
	gUIShell.HideInterface( "in_game_player_panel_characters" );

	for ( int slot = 1; slot <= 8; ++slot )
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_sign", slot ), "in_game_player_panel" );
		if ( pWindow )
		{
			pWindow->SetTextureIndex( 0 );
			pWindow->SetHasTexture( false );
		}
	}
}


void UIMultiplayer::UpdatePlayersPanel()
{
	if ( gUIShell.IsInterfaceVisible( "in_game_player_panel_characters" ) )
	{


	}
	else // in_game_player_panel
	{
		int playerSlot = 1;

		{for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
		{
			if ( !IsValid(*i) || (*i)->IsComputerPlayer() )
			{
				continue;
			}

			UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_bg", playerSlot ), "in_game_player_panel" );
			if ( pWindow )
			{
				pWindow->SetVisible( true );
			}

			UIText * pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "player%d_text", playerSlot ), "in_game_player_panel" );
			if ( pText )
			{
				pText->SetVisible( true );
				pText->SetText( (*i)->GetName() );
			}

			if ( m_bShowTeams )
			{
				UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_sign", playerSlot ), "in_game_player_panel" );
				if ( pWindow )
				{
					pWindow->SetVisible( true );

					Team * team = gServer.GetTeam( (*i)->GetId() );
					if ( team )
					{
						pWindow->SetTextureIndex( team->m_TextureId );
						pWindow->SetHasTexture( true );
					}
					else
					{
						pWindow->SetTextureIndex( 0 );
						pWindow->SetHasTexture( false );
					}
				}
			}			

			++playerSlot;
		}}

		// hide unused slots
		{for ( int i = playerSlot; i <= 8; ++i )
		{
			UIWindow * pPlayerBg = gUIShell.FindUIWindow( gpstringf( "player%d_bg", i ), "in_game_player_panel" );
			if ( !pPlayerBg )
			{
				break;
			}

			pPlayerBg->SetVisible( false );

			if ( i == playerSlot )
			{
				// resize interface for the number of slots shown

				UIWindow * pPlayersBg = gUIShell.FindUIWindow( "players_bg", "in_game_player_panel" );
				if ( pPlayersBg )
				{
					pPlayersBg->GetRect().bottom = pPlayerBg->GetRect().top + 4;
				}

				UIWindow * pWindow = gUIShell.FindUIWindow( "button_close", "in_game_player_panel" );
				if ( pWindow )
				{
					pWindow->SetRect( pWindow->GetRect().left, pWindow->GetRect().right, pPlayersBg->GetRect().bottom + 10, pPlayersBg->GetRect().bottom + 10 + pWindow->GetRect().Height() );
				}

				UIWindow * pBg = gUIShell.FindUIWindow( "background", "in_game_player_panel" );
				if ( pBg )
				{
					int bgHeight = pBg->GetRect().Height();

					pBg->GetRect().bottom = pPlayerBg->GetRect().top + 42;

					if ( bgHeight != pBg->GetRect().Height() )
					{
						GUInterface *pInterface = gUIShell.FindInterface( "in_game_player_panel" );
						if ( pInterface )
						{
							gUIShell.CenterInterface( *pInterface );
						}
					}
				}
			}

			HIDE(( "player%d_text", i ));
			HIDE(( "player%d_options", i ));
			HIDE(( "team%d_sign", i ));
			HIDE(( "character%d_text", i ));
			HIDE(( "location%d_text", i ));
		}}
	}
}


void UIMultiplayer::ShowPlayerListing( const char * interfaceName )
{
	int playerSlot = 1;

	{for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
	{
		if ( !IsValid(*i) || (*i)->IsComputerPlayer() )
		{
			continue;
		}

		UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "player%d_bg", playerSlot ), interfaceName );
		if ( pWindow )
		{
			pWindow->SetVisible( true );
		}

		if ( m_bShowTeams )
		{
			UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "team%d_sign", playerSlot ), interfaceName );
			if ( pWindow )
			{
				pWindow->SetVisible( true );

				Team * team = gServer.GetTeam( (*i)->GetId() );
				if ( team )
				{
					pWindow->SetTextureIndex( team->m_TextureId );
					pWindow->SetHasTexture( true );
				}
				else
				{
					pWindow->SetTextureIndex( 0 );
					pWindow->SetHasTexture( false );
				}
			}
		}			

		UIText * pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "player%d_text", playerSlot ), interfaceName );
		if ( pText )
		{
			pText->SetVisible( true );
			pText->SetText( (*i)->GetName() );
		}

		++playerSlot;
	}}

	// hide unused slots
	{for ( int i = playerSlot; i <= 8; ++i )
	{
		HIDE(( "player%d_bg", i ));
		HIDE(( "team%d_sign", i ));
		HIDE(( "player%d_text", i ));
	}}
}
//
// endGameType determines which set of stats to display
// this string is set in the gametypes gas file
// 
void UIMultiplayer::ShowEndGameRankings( gpstring &endGameType )
{
	UIWindow * pWindow = gUIShell.FindUIWindow( "stats_bg", "end_game_summary_stats" );
	if ( !pWindow )
	{
		return;
	}

	gUIShell.ShowGroup("end_game_player_only", false);

	// force another entry on the histories
	gVictory.UpdateAllPlayerHistories( true );

	// if we didn't pass in a game type, display the first one!
	if( endGameType.empty() )
	{
		endGameType.assign( gVictory.GetGameType()->GetEndGameRankings().begin()->first );
	}

	Victory::EndRankingsColl::const_iterator endRankings;
	endRankings = gVictory.GetGameType()->GetEndGameRankings().find(endGameType);

	Victory::RankingsColl rankings = endRankings->second;


	UIText * pText;
	int playerLeft = 0;
	int playerSlot = 1;
	{for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
	{
		if ( !IsValid(*i) || (*i)->IsComputerPlayer() )
		{
			continue;
		}

		UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "player%dteam", playerSlot ), "end_game_summary" );
		if ( pWindow )
		{
			if ( m_bShowTeams )
			{
				playerLeft = pWindow->GetRect().right + 6;
				pWindow->SetVisible( true );

				Team * team = gServer.GetTeam( (*i)->GetId() );
				if ( team && (team->m_TextureId != 0) )
				{
					pWindow->SetTextureIndex( team->m_TextureId );
					pWindow->SetHasTexture( true );
				}
				else
				{
					pWindow->SetTextureIndex( 0 );
					pWindow->SetHasTexture( false );
				}
			}
			else
			{
				playerLeft = pWindow->GetRect().left;
				pWindow->SetVisible( false );
			}
		}

		pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "player%dname", playerSlot ), "end_game_summary" );
		if ( pText )
		{
			pText->SetRect( playerLeft, pText->GetRect().right, pText->GetRect().top, pText->GetRect().bottom );

			pText->SetVisible( true );
			pText->SetText( (*i)->GetHeroName() );
		}

		pText = (UIText *)gUIShell.FindUIWindow( gpstringf( "player%dclass", playerSlot ), "end_game_summary" );
		if ( pText )
		{
			gpstring playerClass;
			pText->SetVisible( true );

			gVictory.GetEndGameAuditor()->Get( MakeInt( (*i)->GetId() ), "playerclass", playerClass );
			pText->SetText( ToUnicode( playerClass ) );
		}

		pWindow = gUIShell.FindUIWindow( gpstringf( "player%dcolor", playerSlot ), "end_game_summary_timeline" );
		if ( pWindow )
		{
			pWindow->SetBackgroundColor( m_PlayerColors[ playerSlot - 1 ] );
		}

		++playerSlot;
	}}
	{for ( int i = playerSlot; i <= 8; ++i )
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "player%dteam", i ), "end_game_summary" );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}
		pWindow = gUIShell.FindUIWindow( gpstringf( "player%dclass", playerSlot ), "end_game_summary" );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}

		pWindow = gUIShell.FindUIWindow( gpstringf( "player%dname", i ), "end_game_summary" );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}

		pWindow = gUIShell.FindUIWindow( gpstringf( "player%dcolor", i ), "end_game_summary_timeline" );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}
	}}

	pText = (UIText *)gUIShell.FindUIWindow( "player_name_header", "end_game_summary" );
	if ( pText )
	{
		pText->SetRect( playerLeft, pText->GetRect().right, pText->GetRect().top, pText->GetRect().bottom );
	}

	UISlider * pSlider = (UISlider *)gUIShell.FindUIWindow( "scroll_stats", "end_game_summary_stats" );
	if ( pSlider )
	{
		if ( rankings.size() > 6 )
		{
			pSlider->SetVisible( true );
			pSlider->SetMax( rankings.size() - 6 );
			pSlider->SetValue( 0 );
		}
		else
		{
			pSlider->SetVisible( false );
			pSlider->SetMax( 0 );
		}
	}

	int colLeft = pWindow->GetRect().left + 8;
	int colWidth = (pWindow->GetRect().Width() - 16 - (rankings.size() - 1) * 8) / rankings.size();
	int column = 1;

	bool addToInterface = false;

	for ( Victory::RankingsColl::const_iterator rank = rankings.begin(); rank != rankings.end(); ++rank, ++column )
	{
		FastFuelHandle hStatHeader( "ui:interfaces:multiplayer:end_game_summary_stats:stat_header" );
		gpassert( hStatHeader.IsValid() );
		UIText * pStatHeader = (UIText *) gUIShell.FindUIWindow( gpstringf( "stat%d_header", column ), "end_game_summary_stats" );
		if( !pStatHeader )
		{
			pStatHeader = new UIText( hStatHeader, "end_game_summary_stats", NULL );
			addToInterface = true;
		}
		pStatHeader->SetName( gpstringf( "stat%d_header", column ) );
		pStatHeader->SetRect( colLeft, colLeft + colWidth, pStatHeader->GetRect().top, pStatHeader->GetRect().bottom );
		pStatHeader->SetText( gVictory.GetStatScreenName( *rank ) );
		pStatHeader->SetVisible( true );
		if( addToInterface == true )
		{
			gUIShell.AddWindowToInterface( pStatHeader, "end_game_summary_stats" );
			addToInterface = false;
		}


		FastFuelHandle hStatBg( "ui:interfaces:multiplayer:end_game_summary_stats:stat_bg" );
		gpassert( hStatBg.IsValid() );		
		UIWindow * pStatBg = gUIShell.FindUIWindow( gpstringf( "stat%d_bg", column ), "end_game_summary_stats" );
		if( !pStatBg )
		{
			pStatBg = new UIWindow( hStatBg, "end_game_summary_stats", NULL );
			addToInterface = true;
		}
		pStatBg->SetName( gpstringf( "stat%d_bg", column ) );
		pStatBg->SetRect( colLeft, colLeft + colWidth, pStatBg->GetRect().top, pStatBg->GetRect().bottom );
		pStatBg->SetAlpha( (column % 2 == 0) ? 0.2f : 0.3f );
		pStatBg->SetVisible( true );
		if( addToInterface == true )
		{
			gUIShell.AddWindowToInterface( pStatBg, "end_game_summary_stats" );
			addToInterface = false;
		}
		
		int totalStatValue = 0;
		int numPlayers = 0;
		// determine the sum total of the stat for the up and downy
		for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
		{
			if ( IsValid(*i) && !(*i)->IsComputerPlayer() )
			{
				totalStatValue = totalStatValue + gVictory.GetPlayerStat( *rank, (*i)->GetId() );
				numPlayers ++;
			}
		}
		gpassert( numPlayers > 0 );

		playerSlot = 1;
		for ( Server::PlayerColl::const_iterator player = gServer.GetPlayers().begin(); player != gServer.GetPlayers().end(); ++player )
		{
			if ( !IsValid(*player) || (*player)->IsComputerPlayer() )
			{
				UILine * pUILine = (UILine *) gUIShell.FindUIWindow( gpstringf( "timeline_player%d", playerSlot ), "end_game_summary_timeline" );
				if( pUILine )
				{
					pUILine->SetVisible( false );
				}
				continue;
			}
			///////////////////////////////////////////
			// summary statistics!

			FastFuelHandle hStat( gpstringf( "ui:interfaces:multiplayer:end_game_summary_stats:player%dstat", playerSlot ) );
			gpassert( hStat.IsValid() );
			UIText * pStat = (UIText *) gUIShell.FindUIWindow( gpstringf( "player%dstat%d", playerSlot, column ), "end_game_summary_stats" );
			if( !pStat )
			{
				pStat = new UIText( hStat, "end_game_summary_stats", NULL );
				addToInterface = true;
			}
			pStat->SetName( gpstringf( "player%dstat%d", playerSlot, column ) );
			pStat->SetRect( colLeft, colLeft + colWidth, pStat->GetRect().top, pStat->GetRect().bottom );

			int playerValue = gVictory.GetPlayerStat( *rank, (*player)->GetId() );
			gpwstring displayValue;
			if( playerValue > totalStatValue/numPlayers )
			{
				displayValue.assign( L"(playerrank_green)(playerrank_up) " );
			}
			else if( playerValue < totalStatValue/numPlayers )
			{
				displayValue.assign( L"(playerrank_red)(playerrank_down) " );
			}
			displayValue.appendf( L"%d", playerValue );
			gUIEmotes.PrepareString(displayValue); 
			pStat->SetText( displayValue );

			pStat->SetVisible( true );
			if( addToInterface == true )
			{
				gUIShell.AddWindowToInterface( pStat, "end_game_summary_stats" );
				addToInterface = false;
			}

			RefreshEndGameDetails( playerSlot, (*player)->GetId() );
			++playerSlot;
		}
		colLeft += colWidth + 8;
	}

	// make sure any old windows are not visible!
	while( gUIShell.FindUIWindow( gpstringf( "stat%d_header", column ), "end_game_summary_stats" ) )
	{
		UIWindow* pWindow = gUIShell.FindUIWindow( gpstringf( "stat%d_header", column ), "end_game_summary_stats" );
		if( pWindow )
		{
			pWindow->SetVisible( false );
		}

		pWindow		 = gUIShell.FindUIWindow( gpstringf( "stat%d_bg", column ), "end_game_summary_stats" );
		if( pWindow )
		{
			pWindow->SetVisible( false );
		}

		for ( unsigned int playerSlot = 0; playerSlot < gServer.GetMaxPlayers(); playerSlot++  )
		{
			pWindow		 = gUIShell.FindUIWindow( gpstringf( "player%dstat%d", playerSlot + 1, column ), "end_game_summary_stats" );
			if( pWindow )
			{
				pWindow->SetVisible( false );
			}
		}
		column ++;
	}

	for ( playerSlot = 0; playerSlot < (int) gServer.GetMaxPlayers(); playerSlot++  )
	{
		// check to see if the player is still around, if not, clear out their lines.
		if( ! gServer.GetPlayer( MakePlayerIdFromIndex( playerSlot ) ) )
		{
			column = 1;
			while( gUIShell.FindUIWindow( gpstringf( "player%dstat%d", playerSlot + 1, column ), "end_game_summary_stats" ) )
			{
				pWindow		 = gUIShell.FindUIWindow( gpstringf( "player%dstat%d", playerSlot + 1, column ), "end_game_summary_stats" );
				if( pWindow )
				{
					pWindow->SetVisible( false );
				}
				column++;
			}
		}

	}
}

void UIMultiplayer::RefreshEndGameDetails( int playerSlot, PlayerId playerId )
{
	///////////////////////////////////////////
	// line drawing statistics!
	UILine * pUILine = (UILine *) gUIShell.FindUIWindow( gpstringf( "timeline_player%d", playerSlot ), "end_game_summary_timeline" );
	if( pUILine )
	{
		pUILine->ClearAllLineValues();
		pUILine->SetColor( m_PlayerColors[ playerSlot - 2 ] );
		pUILine->SetVisible( true );

		float historyValue = -1;
		float previousHistoryValue = -1;
		double historyTime = 0.0;
		gpstring statName;
		int historySize = 0;

		UIRadioButton *pRadio = NULL;

		// loop over until the radio button doesn't exist
		for( int i = 1; i == 1 || pRadio ; i++ ) 
		{
			pRadio = (UIRadioButton *) gUIShell.FindUIWindow( gpstringf( "details_radio%d", i ), "end_game_summary_timeline" );
			if( pRadio && pRadio->GetCheck() )
			{
				UIText *pRadioText = (UIText *) gUIShell.FindUIWindow( gpstringf( "details_radio%d_text", i ), "end_game_summary_timeline" );
				if( pRadioText )
				{
					gpwstring screenName( pRadioText->GetText() );
					statName = gVictory.GetNameFromScreenName( screenName );
					historySize = gVictory.GetPlayerHistorySize( playerId, statName );

					for( int i = 0; i < historySize; ++i )
					{
						gVictory.GetPlayerHistoryValue( historyValue, historyTime, i, playerId, statName );
						// allow repeats on the first and last numbers.
						if( i == 0  || i == historySize - 1 || previousHistoryValue != historyValue )
						{
							pUILine->AddLineValue( historyTime, historyValue );
						}
						previousHistoryValue = historyValue;
					}
				}
				UIText *pText = (UIText *) gUIShell.FindUIWindow( gpstringf( "timeline_selected_title" ), "end_game_summary_timeline" );
				if( pText )
				{
					pText->SetText( pRadioText->GetText() );
				}
			}
		}
	}
}

void UIMultiplayer::ActivateEndGameScreen()
{
	m_bActivateEndGameScreen = true;

	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:background", true );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:multiplayer_help", true );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:end_game_summary", true );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:end_game_summary_stats", true );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:end_game_summary_timeline", false );

	SetIsVisible( true );
}


void UIMultiplayer::ShowEndGameScreen( )
{
	gUIShell.GetMessenger()->RegisterCallback( makeFunctor( *this, &UIMultiplayer::EndGameGameGUICallback ) );

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

	if ( !session && !IsOnZone() )
	{
		EndGameDisableChat();
	}

	UIText * pText = (UIText *)gUIShell.FindUIWindow( "game_time", "end_game_summary" );
	if ( pText )
	{
		pText->SetText( ConstructTimeString( (int)((gVictory.GetEndGameTime() + 30) / 60) ) );
	}

	pText = (UIText *)gUIShell.FindUIWindow( "end_game_close_text", "end_game_summary" );
	if ( pText )
	{
		if ( GetMultiplayerMode() == MM_GUN )
		{
			pText->SetText( gpwtranslate( $MSG$ "Return to ZoneMatch" ) );
		}
		else
		{
			pText->SetText( gpwtranslate( $MSG$ "Return to Main Menu" ) );
		}
	}

	UIEditBox *pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "end_game_chat_editbox" );
	if ( pEditBox )
	{
		pEditBox->SetPermanentFocus( true );
		pEditBox->SetAllowInput( true );
		pEditBox->SetText( L"" );
		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{
			UIWindow *pWindow = gUIShell.FindUIWindow( "end_game_send_to", "end_game_summary" );
			if ( pWindow )
			{
				pEditBox->SetRect( pEditBox->GetRect().left, pWindow->GetRect().left - 30, pEditBox->GetRect().top, pEditBox->GetRect().bottom );
			}
		}
	}
	// update our player chat list
	UpdatePlayerListBox();

	gpstring endGameType;
	ShowEndGameRankings( endGameType );
}

void UIMultiplayer::DrawEndGameTimeline()
{
	if ( Player::GetHumanPlayerCount() < 2 )
	{
		return;
	}

	UIWindow * pWindow = gUIShell.FindUIWindow( "timeline_background", "end_game_summary_timeline" );
	if ( pWindow == NULL )
	{
		return;
	}

	UIRect rect = pWindow->GetRect();

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_ALWAYS );

	gUIShell.GetRenderer().SetTextureStageState(	0,
													D3DTOP_DISABLE,
													D3DTOP_MODULATE,
													D3DTADDRESS_CLAMP,
													D3DTADDRESS_CLAMP,
													D3DTEXF_POINT,
													D3DTEXF_POINT,
													D3DTEXF_POINT,
													D3DBLEND_SRCALPHA,
													D3DBLEND_INVSRCALPHA,
													true );

	tVertex Verts[4];
	memset( Verts, 0, sizeof( tVertex ) * 4 );

	PlayerId playerIds[ Server::MAX_PLAYERS ];
	int playerIdx = 0;
	for ( Server::PlayerColl::const_iterator player = gServer.GetPlayers().begin(); player != gServer.GetPlayers().end(); ++player )
	{
		if ( !IsValid(*player) || (*player)->IsComputerPlayer() )
		{
			continue;
		}

		pWindow = gUIShell.FindUIWindow( gpstringf( "player%dcolor", playerIdx + 1 ), "end_game_summary_timeline" );
		if ( pWindow )
		{
			Verts[0].x			= pWindow->GetRect().right - 0.5f;
			Verts[0].y			= pWindow->GetRect().top - 0.5f;
			Verts[0].z			= 0.0f;
			Verts[0].rhw		= 1.0f;
			Verts[0].uv.u		= 0.0f;
			Verts[0].uv.v		= 0.0f;
			Verts[0].color		= m_PlayerColors[playerIdx];

			Verts[1].x			= pWindow->GetRect().right - 0.5f;
			Verts[1].y			= pWindow->GetRect().bottom - 0.5f;
			Verts[1].z			= 1.0f;
			Verts[1].rhw		= 1.0f;
			Verts[1].uv.u		= 0.0f;
			Verts[1].uv.v		= 1.0f;
			Verts[1].color		= m_PlayerColors[playerIdx];

			Verts[2].x			= rect.left - 0.5f;
			Verts[2].y			= rect.top + ((float)rect.Height() / Player::GetHumanPlayerCount()) * playerIdx - 0.5f;
			Verts[2].z			= 0.0f;
			Verts[2].rhw		= 1.0f;
			Verts[2].uv.u		= 1.0f;
			Verts[2].uv.v		= 0.0f;
			Verts[2].color		= m_PlayerColors[playerIdx];

			Verts[3].x			= rect.left - 0.5f;
			Verts[3].y			= rect.top + ((float)rect.Height() / Player::GetHumanPlayerCount()) * (playerIdx + 1) - 0.5f;
			Verts[3].z			= 0.0f;
			Verts[3].rhw		= 1.0f;
			Verts[3].uv.u		= 1.0f;
			Verts[3].uv.v		= 1.0f;
			Verts[3].color		= m_PlayerColors[playerIdx];

			gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, 0, 1 );
		}

		playerIds[ playerIdx++ ] = (*player)->GetId();
	}

	float valueL[ Server::MAX_PLAYERS ];
	float valueR[ Server::MAX_PLAYERS ];
	{for ( int i = 0; i < Player::GetHumanPlayerCount(); ++i )
	{
		valueL[i] = ((i>0)?valueL[i - 1]:0.0f) + (float)rect.Height() / Player::GetHumanPlayerCount();
	}}

	float time = 0.0f, timeStep = (float)gVictory.GetEndGameTime() / 32;
	float x = (float)rect.left, xStep = (float)rect.Width() / 32;

	for ( int i = 0; i < 32; ++i )
	{
		float totalR = 0.0f;
		for ( int player = 0; player < Player::GetHumanPlayerCount(); ++player )
		{
			valueR[player] = gVictory.GetPlayerExperienceTimelineValue( playerIds[player], time + timeStep );
			totalR += valueR[player];
		}

		float prevL = 0.0f, prevR = 0.0f;
		for ( player = 0; player < Player::GetHumanPlayerCount(); ++player )
		{
			if ( totalR > 0 )
			{
				valueR[player] = prevR + (valueR[player] / totalR) * rect.Height();
			}
			else
			{
				valueR[player] = prevR + (float)rect.Height() / Player::GetHumanPlayerCount();
			}

			Verts[0].x			= x - 0.5f;
			Verts[0].y			= (rect.top + prevL) - 0.5f;
			Verts[0].z			= 0.0f;
			Verts[0].rhw		= 1.0f;
			Verts[0].uv.u		= 0.0f;
			Verts[0].uv.v		= 0.0f;
			Verts[0].color		= m_PlayerColors[player];

			Verts[1].x			= x - 0.5f;
			Verts[1].y			= (rect.top + valueL[player]) - 0.5f;
			Verts[1].z			= 1.0f;
			Verts[1].rhw		= 1.0f;
			Verts[1].uv.u		= 0.0f;
			Verts[1].uv.v		= 1.0f;
			Verts[1].color		= m_PlayerColors[player];

			Verts[2].x			= (x + xStep) - 0.5f;
			Verts[2].y			= (rect.top + prevR) - 0.5f;
			Verts[2].z			= 0.0f;
			Verts[2].rhw		= 1.0f;
			Verts[2].uv.u		= 1.0f;
			Verts[2].uv.v		= 0.0f;
			Verts[2].color		= m_PlayerColors[player];

			Verts[3].x			= (x + xStep) - 0.5f;
			Verts[3].y			= (rect.top + valueR[player]) - 0.5f;
			Verts[3].z			= 0.0f;
			Verts[3].rhw		= 1.0f;
			Verts[3].uv.u		= 1.0f;
			Verts[3].uv.v		= 1.0f;
			Verts[3].color		= m_PlayerColors[player];

			gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, 0, 1 );

			prevL = valueL[player];
			prevR = valueR[player];
			valueL[player] = valueR[player];
		}
		
		x += xStep;
		time += timeStep;
	}

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_LESSEQUAL );
}

void UIMultiplayer::EndGameDisableChat()
{
	DISABLE(( "end_game_return_to_staging" ));

	UIText * pText = (UIText *)gUIShell.FindUIWindow( "chat_disabled_text", "end_game_summary" );
	if ( pText )
	{
		pText->SetVisible( true );
	}

	UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "end_game_chat_editbox", "end_game_summary" );
	if ( pEditBox )
	{
		pEditBox->SetVisible( false );
	}

	pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "combo_box_players", "end_game_summary" );
	if ( pEditBox )
	{
		pEditBox->SetVisible( false );
	}

	UIText * pTextBox = (UIText *)gUIShell.FindUIWindow( "end_game_send_to", "end_game_summary" );
	if ( pTextBox )
	{
		pTextBox->SetVisible( false );
	}
}

FuBiCookie UIMultiplayer::RCEndGameActivateStagingReturn()
{
	FUBI_RPC_THIS_CALL_RETRY( RCEndGameActivateStagingReturn, RPC_TO_ALL );

	ENABLE(( "end_game_return_to_staging" ));

	return ( RPC_SUCCESS );
}


bool UIMultiplayer::UpdatePlayerListBox( )
{
	if( UIGame::DoesSingletonExist() )
	{
		if( IsInStagingArea( gWorldState.GetCurrentState() ) || IsInGame( gWorldState.GetCurrentState() ) || IsEndGame( gWorldState.GetCurrentState() ))
		{
			gUIGame.UpdateGuiEditBox();
		}
	}
	else
	{
		gperror( "I can not find the game ui to send the message" );
		return false;
	}
	

	return true;
}

