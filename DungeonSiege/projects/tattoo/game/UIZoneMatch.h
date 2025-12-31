/*=====================================================================

	Name	:	UIZoneMatch
	Date	:	Mar, 2001
	Author	:	Adam Swensen

	Purpose	:	Handles ZoneMatch multiplayer interfaces

---------------------------------------------------------------------*/

#pragma once
#ifndef __UIZONEMATCH_H
#define __UIZONEMATCH_H

#include "GoDefs.h"

#include <GUNQueue.h>
#include <GunApi.h>
#include "zping.h"

class InputBinder;
class UIPopupMenu;
class UISlider;

interface IUpdateClient2;


///////////////////////////////////////////////////////////////

namespace Gun2
{
	enum eUserStatus
	{
		LOGGEDOUT = 0,
		LOGGEDIN = 1,
		AFK = 2,
		DND = 4,
		INGAME = 8,
		INCHAT = 16,
		INSTAGING = 32,
		FRIENDS_ONLY = 64,
	};
}


enum eZoneGameSetting
{
	SET_BEGIN_ENUM( ZONE_, 0 ),
	
	ZONE_GAME_MAP,
	ZONE_GAME_WORLD_LEVEL,
	ZONE_GAME_DIFFICULTY,
	ZONE_GAME_TIME,
	ZONE_GAME_TIME_LIMIT,
	ZONE_GAME_MAX_PLAYERS,

	SET_END_ENUM( ZONE_ ),	
};

enum eZoneGameFlag
{
	ZONE_GAME_FLAG_DISABLED					= -2,
	ZONE_GAME_NO_FLAG						= 0,
	ZONE_GAME_LOADING						= 1 << 0,
	ZONE_GAME_IN_PROGRESS					= 1 << 1,
	ZONE_GAME_PVP_ENABLED					= 1 << 2,
	ZONE_GAME_TEAMS_ENABLED					= 1 << 3,
	ZONE_GAME_NEW_CHARACTERS_ONLY			= 1 << 4,
	ZONE_GAME_DROP_ANYTHING_ON_DEATH		= 1 << 5,
	ZONE_GAME_DROP_EQUIPPED_ON_DEATH		= 1 << 6,
	ZONE_GAME_DROP_INVENTORY_ON_DEATH		= 1 << 7,
	ZONE_GAME_DROP_INVENTORY				= ZONE_GAME_DROP_ANYTHING_ON_DEATH|ZONE_GAME_DROP_EQUIPPED_ON_DEATH|ZONE_GAME_DROP_INVENTORY_ON_DEATH,
	ZONE_GAME_PASSWORD_PROTECTED			= 1 << 8,
	ZONE_GAME_HAS_UBER_CHARACTERS_10		= 1 << 9,
	ZONE_GAME_HAS_UBER_CHARACTERS_25		= 1 << 10,
	ZONE_GAME_HAS_UBER_CHARACTERS_50		= 1 << 11,
	ZONE_GAME_HAS_UBER_CHARACTERS_100		= 1 << 12,
	ZONE_GAME_HAS_UBER_CHARACTERS_MAX		= 1 << 13,
	ZONE_GAME_HAS_UBER_CHARACTERS			= ZONE_GAME_HAS_UBER_CHARACTERS_10|ZONE_GAME_HAS_UBER_CHARACTERS_25|ZONE_GAME_HAS_UBER_CHARACTERS_50|ZONE_GAME_HAS_UBER_CHARACTERS_100|ZONE_GAME_HAS_UBER_CHARACTERS_MAX,
	ZONE_GAME_WORLD_REGULAR					= 1 << 14,
	ZONE_GAME_WORLD_VETERAN					= 1 << 15,
	ZONE_GAME_WORLD_ELITE					= 1 << 16,
	ZONE_GAME_WORLD							= ZONE_GAME_WORLD_REGULAR|ZONE_GAME_WORLD_VETERAN|ZONE_GAME_WORLD_ELITE,

	ZONE_GAME_FULL							= 1 << 17,
	ZONE_GAME_ALLOW_JIP						= 1 << 18,
	ZONE_GAME_ALLOW_START_SELCTION			= 1 << 19,
	ZONE_GAME_ALLOW_PAUSING					= 1 << 20,


	ZONE_GAME_LAST_FLAG						= 1 << 21,

	ZONE_GAME_LANGUAGE						= ZONE_GAME_LAST_FLAG << 1,
	// language flags programmaticaly generated after 
	// the last flag.  there will be lots of them! :)
};

enum eAutoUpdate
{
	UPDATE_NONE,
	UPDATE_CHECKCLIENT,
	UPDATE_CHECKGAME,
	UPDATE_DOWNLOADCLIENT,
	UPDATE_DOWNLOADGAME,
	UPDATE_UPDATINGCLIENT,
};

enum eInGameMessageOption
{
	ZONE_MESSAGE_OPTION_EVERYONE,
	ZONE_MESSAGE_OPTION_FRIENDS,
	ZONE_MESSAGE_OPTION_NOONE,
};

///////////////////////////////////////////////////////////////
bool IsOnZone();
bool IsGunMM();

using namespace Gun2;
class UIZoneMatch : public Gun2::IGUNQueueRead, public IGunHostEnum, public Singleton< UIZoneMatch >
{
public:

	UIZoneMatch();
	~UIZoneMatch();

	bool Init();
	bool IsInitialized()			{  return ( m_pGunConnection != NULL );  }

	void Activate();
	void Deactivate();

	void LoadZoneSettings();
	void SaveZoneSettings();

	void Update( double secondsElapsed );
	void UpdateInGame( double secondsElapsed );
	void InactiveUpdate();

	// Connection
	bool Connect();
	void Disconnect();

	bool IsSignedIn()								{  return m_bSignedIn;  }

	void OnZoneCancelConnect();
	void OnZoneCancelLogin();

	ULONGLONG GetLocalTimeInSeconds();
	
	ULONGLONG ToGunTime( ULONGLONG localTime );
	ULONGLONG ToLocalTime( DWORD serverTime );

	// GUN callbacks
	STDMETHODIMP Read( Gun2::GUNQueueMessage *pMsg );

	STDMETHODIMP OnEnumPlayer( DWORD dwItemId, LPCWSTR pwszPlayerName );
	STDMETHODIMP OnEnumItemValue( LPCSTR pszKey, LPCWSTR pwszValue );

	// UI
	void GameGUICallback( gpstring const & message, UIWindow & ui_window );

	void ShowMatchMakerInterface( bool bReactivate = false );

	void ShowConnectedInterface( bool bReactivate = false );
	void ShowDisconnectedInterface();

	void ShowNewsTab();
	void ShowGamesTab();
	void ShowChatTab();

	void ShowSignInDialog();
	void ShowCreateAccountDialog();

	// this will clear up all requests and messages 
	// so we start a fresh with a new login.
	void ClearPendingRequests();

	void EnableInterfaceOnLogin();
	void DisableInterfaceOnLogout();

	void EnableButton( const char * buttonName, bool bState );
	void EnableRadioButton( const char * radioButton, bool bState, const char* sInterface = NULL );
	void EnableTextBox( const char * textbox, bool bState, const char* sInterface = NULL );

	void UpdateCharacterLevelRadioButtons( bool forceDefaultCheck = false );

	enum eChatAreaSize
	{
		CHAT_AREA_NONE = 520,
		CHAT_AREA_SMALL = 490,
		CHAT_AREA_MEDIUM = 350,
		CHAT_AREA_LARGE = 270,
	};

	void ResizeChatArea( eChatAreaSize size );

	void HideDialogs();

	void RefreshFriendsList( bool fullRefresh = false );

	void SetNewMessageState( gpwstring userName, bool bActive = true );
	void UpdateFriendsNewMessages();

	void CreateMessageList( );

	void OnLeaveChatRoom( bool bHideChatArea = true );

	void GetGamesListHeaders();

	void CreateGamesList( bool forceRefresh = false );

	void UpdateGameColumns();

	void ShowGameDetails();

	bool HostGame( gpwstring & gameName );

	void OnEnterStagingArea();
	void OnExitStagingArea();

	void OnBeginGame();
	void OnEndGame();

	void OnInGameEnableJIP();
	void OnInGameDisableJIP();

	enum eDialogs
	{
		DIALOG_OK,
		DIALOG_ACCEPT_ADD_FRIEND,
		DIALOG_IGNORE_FRIEND,
		DIALOG_ADD_FRIEND,
		DIALOG_CHAT_SIGN_IN,
		DIALOG_CHAT_CHANGE_TOPIC,
		DIALOG_INVITE_CHAT,
		DIALOG_FIND_GAME_NAME,
		DIALOG_JOIN_PASSWORD_CHATROOM,
		DIALOG_HOST_GAME,
		DIALOG_PLAYER_NAME_JOIN_GAME,
		DIALOG_PLAYER_NAME_HOST_GAME,
		DIALOG_INSTALL_CLIENT_UPDATE,
		DIALOG_INSTALL_GAME_UPDATE,
		DIALOG_INSTALL_GAME_EXIT_NOW,
		DIALOG_CLIENT_UPDATE_SUCCESS,
	};

	void ShowDialog( eDialogs dialogType, gpwstring text = L"" );
	bool IsDialogPending()								{  return m_bShowDialog;  }

	void ShowInputDialog( eDialogs dialogType, gpwstring title, gpwstring text );

	// Chat

	void ShowPrivateChatWindow( gpwstring userName, bool chatEnabled = true );
	void HidePrivateChatWindow();
	void RequestChatRefresh();
	bool IsUserInChat( gpwstring userName );

	// In game

	void HostUpdateGameSetting( eZoneGameSetting setting, const gpwstring & value );
	void HostUpdateGameFlag( eZoneGameFlag flag, bool value );

	void HostUpdateGameSettings();

	void HostRefreshPlayerList();

	// AutoUpdate
	HMODULE GetAutoUpdateLib()							{  return m_hAutoUpdateLib;  }

	bool IsClientUpdateAvailable();
	bool CheckForClientUpdate();
	bool DownloadClientUpdate();
	void DownloadClientUpdateComplete();

	void SetGameUpdatePackage( DWORD packageId )		{  m_GameUpdatePackageId = packageId;  }

	bool CheckForGameUpdate();
	bool DownloadGameUpdate();
	void DownloadGameUpdateComplete();
	bool InstallGameUpdate();

	bool GetCancelDownload()							{  return m_bCancelDownload;  }

	eAutoUpdate GetAutoUpdateStatus()					{  return m_AutoUpdateStatus;  }

	// Properties
	gpwstring GetPlayerName()							{  return m_PlayerName;  }

	void SetCurrentUserStatus( DWORD status );
	eUserStatus GetCurrentUserStatus(  )				{	return m_UserStatus;	}
	void UpdateCurrentUserStatus();

	bool IsHosting()									{  return m_bHostingGame;  }

	// Input
	bool InputPressedEnter();
	bool InputPressedEscape();

	typedef std::map< gpwstring, DWORD, istring_less > FriendsMap;
	FriendsMap	GetFriendsNames()						{	return m_Friends;	}

	bool IsFriendOnline( DWORD name );
	bool IsUserFriend( gpwstring userName );
	void RemoveFriend( gpwstring userName );
	bool SendMessageToFriend( gpwstring userName, gpwstring messageText );

	void UpdateHostUberLevelFlags();
	void UpdateHostWorldFlags( gpwstring mapName );
	void SetHostWorldFlags();

	eInGameMessageOption GetGameMessageOptions()				{	return m_inGameMessageOptions; };
	void SetGameMessageOptions( eInGameMessageOption option );
	void UpdateGameMessageOptions( const char* sInterface, const char* sGroup );


private:

	// GUN message processing
	void ProcessEvent        ( Gun2::CBaseEvent *pEvent );
	void ProcessConnectEvent ( Gun2::CBaseEvent *pEvent );
	void ProcessAccountEvent ( Gun2::CBaseEvent *pEvent );
	void ProcessChatEvent    ( Gun2::CBaseEvent *pEvent );
	void ProcessFriendsEvent ( Gun2::CBaseEvent *pEvent );

	void ProcessResult        ( Gun2::CBaseResult *pResult );
	void ProcessAccountsResult( Gun2::CBaseResult *pResult );
	void ProcessNewsResult    ( Gun2::CBaseResult *pResult );
	void ProcessChatResult    ( Gun2::CBaseResult *pResult );
	void ProcessFriendsResult ( Gun2::CBaseResult *pResult );
	void ProcessMatchResult   ( Gun2::CBaseResult *pResult );

	void UpdateLatencies( double secondsElapsed );

    HMODULE			m_hGunLib;

	// GUN interfaces
	IGunConnection	*m_pGunConnection;
	IGunAccount		*m_pGunAccount;
	IGunNews		*m_pGunNews;
	IGunChat		*m_pGunChat;
	IGunFriends		*m_pGunFriends;
	IGunHost		*m_pGunHost;
	IGunBrowser		*m_pGunBrowser;

	// GUN connection
	gpstring		m_ServerName;
	unsigned int	m_ServerPort;

	gpstring		m_NewsServerName;
	gpstring		m_NewsServerFile;
	unsigned int	m_NewsServerPort;

	enum eGunConnectionStatus
	{
		GUN_INVALID,
		GUN_CONNECTING,
		GUN_CONNECTED,
		GUN_FAILED_TO_CONNECT,
		GUN_DISCONNECTED,
	};

	float					m_ConnectionTimeout;
	eGunConnectionStatus	m_ConnectionStatus;

	DWORD			m_ServerStartTime;
	ULONGLONG		m_LocalStartTime;

	bool			m_bAttemptReconnect;
	bool			m_bFromInGame;

//	Accounts

	gpwstring		m_Username;
	gpwstring		m_Password;
	gpwstring		m_EnteredUsername;
	gpwstring		m_EnteredPassword;

//	Chat

	typedef std::map< int, DWORD > ChatModeMap;
	ChatModeMap		m_ChatModes;
	gpwstring		m_DefaultJoinChannel;
	gpwstring		m_RequestedChannel;
	int				m_RequestedChannelSuffix;
	gpwstring		m_RequestedChannelPassword;
	gpwstring		m_RequestedTopic;
	DWORD			m_RequestedChatMode;
	int				m_RequestedUserLimit;
	bool			m_bRefreshChatRoomList;
	float			m_TimeToRefreshChatRoomList;
	gpwstring		m_CurrentChannel;
	gpwstring		m_CurrentChannelPassword;
	DWORD			m_CurrentRoomId;
	bool			m_bHostOfChannel;
	gpwstring		m_CurrentHostOfChannel;

	typedef std::map< int, gpwstring > SquelchRequestMap;
	SquelchRequestMap	m_SquelchRequests;
	int					m_NextSquelchRequest;

	typedef std::vector< gpwstring > UserColl;

	UserColl		m_Squelches;
	UserColl		m_Ignored;

	bool			AddFriendToZoneMatch( gpwstring userName );
	void			IgnoreUser( gpwstring userName );
	void			UnignoreUser( gpwstring userName );
	bool			IsUserIgnored( gpwstring userName );

	void			SquelchUser( gpwstring userName );
	void			UnSquelchUser( gpwstring userName );
	bool			IsUserSquelched( gpwstring userName );

	bool			m_bSizingChatArea;

//	Friends

	typedef std::map< int, gpwstring > FriendRequestMap;

	FriendRequestMap	m_FriendAddRequests;
	int					m_NextFriendAddRequest;

	gpwstring			m_AcceptAddUser;
	gpwstring			m_RequestIgnoreUser;

	bool				m_bRequestBeforeAdd;
	bool				m_bDisableFriends;
	//$$$bool			m_bDisableFriendsInGame;
	eInGameMessageOption m_inGameMessageOptions;
	eUserStatus			m_UserStatus;

	FriendsMap			m_Friends;

//	Messaging

	// Special messages
	enum eUserMessageType
	{
		USER_MESSAGE,
		USER_CHAT_INVITE,
		USER_REQUEST_ADD,
	};

	struct UserMessage
	{
		int Id;
		gpwstring Text;
		gpwstring From;
		eUserMessageType Type;
	};

	typedef std::vector< UserMessage > UserMessageColl;

	UserMessageColl	m_Messages;
	int				m_NextMessageId;
	gpwstring		m_SendUserMessage;

	// Private user chat
	enum ePrivateChatType
	{
		MSG_FROM,
		MSG_TO,
	};
	typedef std::pair< ePrivateChatType, gpwstring > UserChatText;	// fromText?, text
	typedef std::vector< UserChatText > UserChatLog;
	typedef std::map< gpwstring, UserChatLog, istring_less > ChatLogMap;

	ChatLogMap				m_ChatLogs;
	std::vector<gpwstring>	m_NewMessages;
	gpwstring				m_ActiveChatUser;
	UserChatLog *			m_pActiveChatLog;


//	Match Making
	
	struct GameInfo
	{
		GameInfo()	:	numPlayers( 0 )
					,	maxPlayers( 0 )
					,	timeLimit( 0 )
					,	gameTime( 0 )
					,	flags( 0 )	
					,	ipDWORD( 0 )
					,	latency( 0 )
					,	inProgress( false )	{}

		gpwstring	Name;
		gpstring	Version;
		gpstring	Locale;
		gpstring	ipAddress;
		gpstring	ipAddress2;
		int			numPlayers;
		int			maxPlayers;
		gpwstring	mapName;
		gpwstring	worldLevel;
		gpwstring	difficulty;
		int			timeLimit;
		DWORD		gameTime;
		DWORD		flags;
		DWORD		ipDWORD;
		DWORD		latency;
		bool		inProgress;
	};

	typedef std::map< int, GameInfo > GameInfoMap;	// RowId, GameInfo
	typedef std::vector< int > GameInfoSortVec;  // RowId's, in order from gun
	
	GameInfoMap		m_GamesList;
	GameInfoSortVec	m_GamesSorted;

	unsigned int	m_CurrentGameRowId;
	unsigned int	m_CurrentGamePage;
	unsigned int	m_CurrentRequestedGamePage;
	unsigned int	m_CurrentGameEndPage;
	unsigned int	m_CurrentGameRowsPerPage;			// max number of rows contained in each downloaded gun-page
	unsigned int	m_CurrentViewsPerPage;				// max number of rows visible in the current list divided by rows contained in a page
	unsigned int	m_CurrentRowIndex;					// the index into the current page for the row at the top of the list

	gpwstring		m_SelectedGameName;

	UISlider		*m_pGamesListSlider;

	typedef std::vector< gpstring > ViewHeaderColl;
	typedef std::map< unsigned int, ViewHeaderColl > ViewHeaderMap;
	unsigned int	m_CurrentViewId;
	ViewHeaderMap	m_ViewHeaders;

	bool			m_bRequestGameListCreation;

	float			m_TimeToRefreshGamesList;
	bool			m_bRefreshGamesList;

	enum eGameListSort
	{
		SORT_GAMENAME = 0,
		SORT_PLAYERS,
		SORT_MAP,
		SORT_WORLD,
		SORT_TIME,
	};

	eGameListSort	m_GameListSort;
	bool			m_bGameListSortInverse;

	eGameListSort	m_GameListSearch;
	gpwstring		m_GameListSearchString;

	enum eGameViewType
	{
		SHOW_ALL_GAMES,
		SHOW_FRIENDS_GAMES,
		SHOW_FILTERED_GAMES,
	};

	eGameViewType	m_GameViewType;

	// Custom Filter Searching.
	void		HideFiltersInterface();
	void		ShowFiltersInterface();

	gpstring	CreateFiltersList();
	gpstring	CreateFiltersListCombos( gpstring filteredFlags, DWORD currentMultiplier, int position );
	gpstring	CreateFiltersListExclusive( DWORD currentMultiplier );

	void		SetDefaultFilterSelections( UIListbox *pList, const char* selectedOption, eZoneGameFlag flag );
	void		SetFilter( UIListbox *pList, eZoneGameFlag flag );

	std::vector<DWORD> m_Filters;
	std::vector<DWORD> m_ExclusiveFilters;
	DWORD			m_ActiveFilters;
	DWORD			m_ActivePairFilters;
	// enable when GUN supports DWORD			m_LanguageFilterFlag;

	gpwstring		m_CurrentGame;

	UIButton *		m_pSelectedGamesTab;

	gpwstring		m_PlayerName;

	bool			m_bScrollingGamesList;
	int				m_GamesScrollStartValue;
	double			m_TimeHeldGamesScrollPos;
	int				m_HeldGamesScrollPos;

//	Match Hosting

	bool			m_bHostingGame;

	DWORD			m_GameFlags;

	struct PlayerInfo
	{
		bool		bRefreshed;

		gpwstring	Name;
		gpwstring	Character;
		gpwstring	Team;
		gpwstring	Level;
	};
	typedef std::map< PlayerId, PlayerInfo > PlayerInfoMap;
	PlayerInfoMap	m_GamePlayers;

	double			m_LastRefreshedGameTime;

	ULONGLONG		m_GameStartTime;

//	AutoUpdate

	eAutoUpdate		m_AutoUpdateStatus;

	HMODULE			m_hAutoUpdateLib;
	IUpdateClient2* m_pUpdateClient;

	HANDLE			m_CheckClientUpdateAvailable;
	HANDLE			m_CheckGameUpdateAvailable;
	HANDLE			m_DownloadWaitHandle;

	DWORD			m_ClientUpdatePackageId;
	DWORD			m_GameUpdatePackageId;

	gpstring		m_AutoUpdateServer;
	gpstring		m_AutoUpdateProxy;

	bool			m_bCancelDownload;


	// Flags
	bool			m_bSignedIn;
	bool			m_bRequestedDisconnect;
	
	// Settings
	bool			m_bLoginAutomatically;

	// UI
	bool			m_bIsActivated;
	bool			m_bInitializedUI;
	eChatAreaSize	m_ChatAreaSize;

	UIPopupMenu		*m_pFriendsMenu;
	UIPopupMenu		*m_pMessagingMenu;
	UIPopupMenu		*m_pChatMenu;
	UIPopupMenu		*m_pChatOptionsMenu;
	UIPopupMenu		*m_pGamesFilterMenu;

	eDialogs		m_CurrentDialog;

	bool			m_bShowDialog;

	InputBinder		*m_pInputBinder;

	std::vector< gpwstring > m_MenuList;

	//$$pingCZonePing		m_ZonePing;
	float			m_TimeToRefreshLatencies;


	// update andy dynamic content
	void UpdateDynamicChatToolTips(); // update out chat and friends tool tips


	FUBI_SINGLETON_CLASS( UIZoneMatch, "ZoneMatch UI layer." );

	SET_NO_COPYING( UIZoneMatch );
};

#define gUIZoneMatch UIZoneMatch::GetSingleton()


#endif