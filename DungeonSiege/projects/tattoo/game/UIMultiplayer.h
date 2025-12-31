/**************************************************************************

Filename		: UIMultiplayer.cpp
Class			: UIMultiplayer
Description		: The class for the multiplayer interface.
Creation Date	: 06/14/00

**************************************************************************/

#pragma once
#ifndef _UIMULTIPLAYER_H_
#define _UIMULTIPLAYER_H_

#include "ui.h"
#include "GoDefs.h"
#include "GamePersist.h"


// Forward Declarations
class InputBinder;
class UIShell;
class UIWindow;
class UICharacterSelect;
class UIZoneMatch;
class NetPipeSession;
class NetPipeSessionHandle;


enum eNetPipeEvent;


enum eMultiplayerMode
{
	MM_NONE = 0,
	MM_GUN,
	MM_NET,
	MM_LAN,
};

enum eMultiplayerDialog
{
	MP_DIALOG_INVALID = 0,

	MP_DIALOG_OK,
	MP_DIALOG_YESNO,
	MP_DIALOG_OK_ENUM_SESSIONS,
	MP_DIALOG_YESNO_DELETE_CHARACTER,
	MP_DIALOG_YESNO_IMPORT_CHARACTER,
	MP_DIALOG_YESNO_CREATE_CHARACTER,
	MP_DIALOG_YESNO_CHANGE_WORLD,
	MP_DIALOG_CANCEL_FIND,
	MP_DIALOG_CANCEL_CONNECT,
	MP_DIALOG_NO_BUTTON,
	MP_DIALOG_LEAVE_STAGING_AREA,
	MP_DIALOG_CANCEL_ZONE_CONNECT,
	MP_DIALOG_CANCEL_ZONE_LOGIN,
	MP_DIALOG_ZONE_CONNECT_FAILED,
	MP_DIALOG_AUTOUPDATE_CONNECT,
	MP_DIALOG_ENTER_PLAYER_NAME,
	MP_DIALOG_DISCONNECTED_FROM_GAME,
};

enum eMultiplayerEditDialog
{
	MP_EDIT_DIALOG_INVALID, 

	MP_EDIT_DIALOG_RENAME,
	MP_EDIT_DIALOG_COPY,
};


class UIMultiplayer : public UILayer, public Singleton< UIMultiplayer >
{
public:

	UIMultiplayer();
	~UIMultiplayer();

	////////////////////////////////////////
	//  Updating / Rendering

	void Update( double seconds_elapsed );
	void Draw( double seconds_elapsed );	

	void HandleBroadcast( WorldMessage & msg );

	bool IsVisible()							{  return( m_bIsVisible );  }
	void SetIsVisible( bool flag );

	void PublishInterfaceToInputBinder() {}
	void BindInputToPublishedInterface() {}
	InputBinder & GetInputBinder();

	virtual void OnAppActivate( bool /*activate*/ ) {}
	
	void GameGUICallback( gpstring const & message, UIWindow & ui_window );

	void EndGameGameGUICallback( gpstring const & message, UIWindow & ui_window );

	////////////////////////////////////////
	//	General calls

	void InitInterfaceElements();
	void ActivateInterfaces();
	void DeactivateInterfaces();
	void HideInterfaces();

	// generic message box dialog
	eMultiplayerDialog GetActiveMessageBox()	{  return m_MultiplayerDialog;  }
	void ShowMessageBox( gpwstring message, eMultiplayerDialog dialog = MP_DIALOG_OK, gpwstring messageButton = gpwstring::EMPTY );
	void ShowLargeMessageBox( gpwstring message, eMultiplayerDialog dialog = MP_DIALOG_OK, gpwstring messageButton = gpwstring::EMPTY );
	void HideMessageBox();
	bool IsMessageBoxPending()					{  return m_bShowDialog;  }

	// returns control to UIFrontend
	void ShowMultiplayerMain();

	////////////////////////////////////////
	//	Game Selection

	void ShowLanGamesInterface();
	void ShowInternetGamesInterface();
	void ShowMatchMakerInterface();
	void ShowHostGameInterface();

	// display a list of available sessions
	void DisplayLanGamesList();

	HRESULT Connect( NetPipeSessionHandle & session, gpwstring const & password );

	bool IsValidGame( gpwstring ipAddress );
	bool JoinGame( gpwstring ipAddress );
	bool CompleteJoinGame();

	void ShowFailedToJoinMessage( eNetPipeEvent event );

	////////////////////////////////////////
	//	Staging Area

	void ShowStagingAreaClient();
	void ShowStagingAreaServer();
	void ShowStagingAreaGameSettings();
	void ShowGameSettingsList( bool clearList = false );

FEX	FuBiCookie RSLeaveStagingArea( PlayerId playerId );
FEX FuBiCookie RCLeaveStagingArea( DWORD machineId );

	void LeaveStagingArea();

	void HideStagingAreaInterfaces();

	// chat
FEX	void ProcessChatMessage( UIEditBox *eb, const char* sInterface, bool sendToAllAsIs = false );
FEX void ProcessChatMessage( gpwstring chatText, const char* sInterface, bool sendToAllAsIs = false );
FEX void RSChat( gpwstring const & sChatText );
FEX void RSChatPlayer( gpwstring const & sChatText, Player * player );
FEX void RCChatPlayer( gpwstring const & sChatText, DWORD machineId );
FEX void RCChat( gpwstring const & sChatText );
	FUBI_MEMBER_SET( RCChat, +SERVER );
FEX void Chat( gpwstring const & sChatText );
	

FEX void RSDisplayMessage( gpwstring const & message, DWORD color );
FEX void RCDisplayMessage( gpwstring const & message, DWORD color, DWORD machineId = RPC_TO_ALL );
	FUBI_MEMBER_SET( RCDisplayMessage, +SERVER );
	void DisplayMessage( gpwstring const & message, DWORD color );

	// map selection
	void SetMapStrings();
	gpwstring GetMapScreenName( gpstring devName );

	// character selection
	void EnableCharacterAppearanceSelection();

	// ui update calls
	void StagingAreaClearPlayerSlot( int playerNumber );

	void StagingAreaUpdatePlayerList();
	bool UpdatePlayerListBox();

	void StagingAreaUpdatePlayer( int playerNumber, Player * pPlayer );
	void StagingAreaUpdatePings();
	void StagingAreaShowTeamColumn( bool flag );
	void StagingAreaShowMpCharacters( int charIndex = 0 );
	void StagingAreaEnableSelections( bool enable );

	void SavePlayerSettings();
	void LoadPlayerSettings();
	void LoadPlayerStartLocationPref();

	void SaveGameSettings();
	void LoadGameSettings();

	void SaveIpAddressHistory();
	void LoadIpAddressHistory();

	// check if conditions are met to start the game
	void StagingAreaCheckEnableStart();

	// check that WS_MP_SESSION_LOST was requested
	void SetRequestedLeaveGame( bool flag )		{  m_bRequestedLeaveGame = flag;  }
	bool RequestedLeaveGame()					{  return m_bRequestedLeaveGame;  }

	bool GetWasDisconnectedFromGame()			{  return m_bWasDisconnected;  }
	void ShowDisconnectedMessage();

	void ShowMultiplayerEditDialog( gpwstring sText, eMultiplayerEditDialog type );
	void HideMultiplayerEditDialog( void );
	void CopyOrRenameCharacter( gpwstring sName, bool bRename );
	bool CanCopyOrRenameCharacter( gpwstring & sName );
	bool IsCharacterNameUnique( const gpwstring & sName );
	void RefreshCharacterScreen();

	////////////////////////////////////////
	//	In-game

	void ShowJIPInterface();
	void HideJIPInterface();
	void UpdateJIPInterface();

	void ShowPlayersPanel();
	void ShowPlayerCharactersPanel();
	void HidePlayersPanel();
	void UpdatePlayersPanel();
	bool IsPlayersPanelVisible()				{  return m_bPlayersPanelVisible;  }

	void ShowPlayerListing( const char * interfaceName );

	void ActivateEndGameScreen();
	void ShowEndGameRankings( gpstring &endGameType );
	void RefreshEndGameDetails( int playerSlot, PlayerId playerId );
	void ShowEndGameScreen();
	void DrawEndGameTimeline();
	void EndGameDisableChat();

FEX	FuBiCookie RCEndGameActivateStagingReturn();
	FUBI_MEMBER_SET( RCEndGameActivateStagingReturn, +SERVER );

	////////////////////////////////////////
	//	Misc

FEX	void SKickPlayer( PlayerId playerId );
FEX	void SBanPlayer ( PlayerId playerId );

	bool CheckForTCPIP();

	gpwstring ConstructTimeString( int totalMinutes );

	void DeinitCharacterSelect();

	void ShowStartDescription( Player * player, const gpwstring & sStart );

	bool IsCharacterImported();

	float GetSelectedCharacterUberLevel();

	bool CanCharacterJoinGame( float & requiredLevel );

	bool DoesCharacterMeetWorldLevel( float charLevel );

	bool IsWorldLevelValidForCharacters( float worldLevel );

	bool IsStartingGroupEnabled( gpstring sGroup );

	void RefreshStartingGroups( Player * player = NULL );	

	gpwstring GetIpFromAddress( const gpwstring & wsAddress );

	////////////////////////////////////////
	//  Specific Settings

	eMultiplayerMode GetMultiplayerMode()		{  return m_MultiplayerMode;  }

	gpwstring GetEnteredName()					{  return m_CharacterName;  }
	gpwstring GetMultiPlayerName()				{  return m_PlayerName;  }
	void SetMultiPlayerName( gpwstring name	)	{  m_PlayerName = name;  }

	////////////////////////////////////////

	enum eNextUpdate
	{
		NU_NONE = 0,
		NU_FIND_GAME,
		NU_CONNECTING,
		NU_JOIN_GAME,
		NU_SHOW_MULTIPLAYER_END_GAME,
	};

	void OnNextUpdate( eNextUpdate nu )			{  m_OnNextUpdate = nu;  }
	eNextUpdate GetNextUpdate()					{  return m_OnNextUpdate;  }

private:

	////////////////////////////////////////
	//  automation

	void SetNetPipeContentResources();
FEX	bool JoinLanGame( const char* playerName, const char* password, bool errorOnFail );
FEX	bool JoinLanGame( const char* playerName, const char* password )  {  return ( JoinLanGame( playerName, password, true ) );  }

	////////////////////////////////////////
	//  RPC Network Calls

FEX FuBiCookie RSDisplayImportCharacter( const gpstring & templateName, DWORD machineId );
FEX FuBiCookie RCDisplayImportCharacter( Goid importChar, DWORD machineId );
	FUBI_MEMBER_SET( RCDisplayImportCharacter, +SERVER );

	void SStartNetworkGame();
FEX FuBiCookie RCStartNetworkGame();
	FUBI_MEMBER_SET( RCStartNetworkGame, +SERVER );

	void SSetTimeLimit( float timelimit );
FEX FuBiCookie RCSetTimeLimit( float timelimit );

FEX FuBiCookie RSSetTeamSign( PlayerId player, const gpstring & teamSign );


	////////////////////////////////////////
	//	data

	enum
	{
		STAGING_PLAYER_SLOTS = 8,
		STAGING_CHARACTER_SLOTS = 4,
		MAX_IP_HISTORY = 10,
	};

	bool					m_bIsVisible;
	bool					m_bIsInitialized;
	bool					m_bIsActivated;

	bool					m_bRequestedLeaveGame;

	bool					m_bWasDisconnected;
	DWORD					m_DisconnectInfo;

	eNextUpdate				m_OnNextUpdate;

	InputBinder *			m_pInputBinder;			

	UICharacterSelect *		m_pCharacterSelect;
	
	gpwstring				m_CharacterName;

	int						m_CurrentCharacterSlot;
	
	int						m_ActivePlayerSlot;
	gpwstring				m_PlayerName;

	typedef std::pair< gpstring, gpwstring > IpPair;
	typedef std::vector< IpPair > IpColl;

	IpColl					m_IpHistory;
	gpstring				m_JoiningIP;
	gpwstring				m_JoinHostName;

	gpstring				m_SelectedMap;

	MapInfoColl				m_MapInfoColl;

	SaveInfoColl			m_saveInfoColl;

	eMultiplayerMode		m_MultiplayerMode;

	bool					m_bImportedChar; // $$ temp disable character options after import

	double					m_FindSessionTimeout;
	double					m_SessionTimeout;

	double					m_LanLastUpdateTime;

	DWORD					m_JoinSessionLocalId;

	bool					m_bShowTeams;

	CharacterInfoColl		m_MpCharacters;
	std::vector< DWORD >	m_CharacterPortraits;
	int						m_SelectedCharacter;
	int						m_SetSelectedCharacter;
	bool					m_fEatEnter;
	
	bool					m_bPlayersDirty;

	bool					m_bPlayersPanelVisible;

	std::vector< DWORD >	m_PlayerColors;

	int						m_currentPlayers;

	bool					m_bActivateEndGameScreen;

	enum eDialogInput
	{
		DIALOG_INPUT_NONE,
		DIALOG_INPUT_OK,
		DIALOG_INPUT_YES,
		DIALOG_INPUT_NO,
	};

	eMultiplayerDialog		m_MultiplayerDialog;
	eMultiplayerEditDialog	m_MultiplayerEditDialog;
	eDialogInput			m_DialogInput;
	bool					m_bShowDialog;

	double					m_TimeToShowPendingMessage;
	gpwstring				m_PendingMessageText;

	////////////////////////////////////////
	//	ZoneMatch interface

	UIZoneMatch				*m_pUIZoneMatch;

	std::vector< gpwstring > m_UIMMultiplayerMenuList;


	FUBI_SINGLETON_CLASS( UIMultiplayer, "This layer owns all the multiplayer GUIs." );

	SET_NO_COPYING( UIMultiplayer );
};

#define gUIMultiplayer UIMultiplayer::GetSingleton()


#endif
