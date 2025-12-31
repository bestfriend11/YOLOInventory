/*=====================================================================

	Name	:	UIZoneMatch
	Date	:	Mar, 2001
	Author	:	Adam Swensen

---------------------------------------------------------------------*/

#include "Precomp_game.h"
#include "UIZoneMatch.h"

#include "appmodule.h"
#include "config.h"
#include "fubitraits.h"
#include "lochelp.h"
#include "uifrontend.h"
#include "uimultiplayer.h"
#include "ui_shell.h"
#include "ui_chatbox.h"
#include "ui_checkbox.h"
#include "ui_dialogbox.h"
#include "ui_listreport.h"
#include "ui_editbox.h"
#include "ui_emotes.h"
#include "ui_popupmenu.h"
#include "ui_tab.h"
#include "ui_text.h"
#include "ui_textbox.h"
#include "worldstate.h"
#include "inputbinder.h"
#include "server.h"
#include "player.h"
#include "netpipe.h"
#include "stringtool.h"
#include "tattoogame.h"
#include "victory.h"
#include "worldoptions.h"
#include "worldmap.h"
#include "worldtime.h"

#include "UIZoneMatchFilter.cpp"


#include <winsock.h>
#include <UpdateClient.h>

using namespace Gun2;

const int ZONE_PING_INTERVAL = 25;


bool IsOnZone()
{
	return ( (gUIMultiplayer.GetMultiplayerMode() == MM_GUN) &&
			 gUIZoneMatch.IsSignedIn() );
}

bool IsGunMM()
{
	return ( gUIMultiplayer.GetMultiplayerMode() == MM_GUN );
}

///////////////////////////////////////////////////////////////

const GUID ZONE_GAME_GUID =	{ 0x77e2d9c2, 0x504e, 0x459f, 0x84, 0x16, 0x8, 0x48, 0x13, 0xb, 0xbe, 0x1e };


///////////////////////////////////////////////////////////////

static const char * s_ZoneGameSetting[] =
{
	"Map",
	"World",
	"Difficulty",
	"Time",
	"TimeL",
	"MaxP",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_ZoneGameSetting ) == ZONE_COUNT );

static stringtool::EnumStringConverter s_ZoneConverter( s_ZoneGameSetting, ZONE_BEGIN, ZONE_END );

bool FromString( const char* str, eZoneGameSetting & gs )
{
	return ( s_ZoneConverter.FromString( str, rcast <int&> ( gs ) ) );
}

const char* ToString( eZoneGameSetting gs )
{
	return ( s_ZoneConverter.ToString( gs ) );
}


///////////////////////////////////////////////////////////////

#define MM_REQUESTED_LOGOUT			(VOID *)0x10
#define MM_LOGOUT_NEW_LOGIN			(VOID *)0x11
#define MM_LOGIN_JOIN_CHANNEL		(VOID *)0x12
#define MM_LEAVE_JOIN_CHANNEL		(VOID *)0x13
#define MM_LEAVE_HOST_CHANNEL		(VOID *)0x14
#define MM_REQUEST_WHERE_IS			(VOID *)0x15
#define MM_REFRESH_GAME_DETAILS		(VOID *)0x16
#define MM_REFRESH_GAME_AND_JOIN	(VOID *)0x17


///////////////////////////////////////////////////////////////
// Game Browser Views

#define GAMEVIEW_SORT_NAME					101
#define GAMEVIEW_SORT_MAP					102
#define GAMEVIEW_SORT_WORLD					103
#define GAMEVIEW_SORT_PLAYERS				104
#define GAMEVIEW_SORT_TIME					105
#define GAMEVIEW_SORT_FILTERED				110
#define GAMEVIEW_SEARCH_NAME				111
#define GAMEVIEW_SEARCH_MAP					112
#define GAMEVIEW_SEARCH_WORLD				113
#define GAMEVIEW_SEARCH_PLAYERS				114
//#define GAMEVIEW_SEARCH_TIME				115

#define GAMEVIEW_SORT_NAME_FRIENDSONLY		201
#define GAMEVIEW_SORT_MAP_FRIENDSONLY		202
#define GAMEVIEW_SORT_WORLD_FRIENDSONLY		203
#define GAMEVIEW_SORT_PLAYERS_FRIENDSONLY	204
#define GAMEVIEW_SORT_TIME_FRIENDSONLY		205
#define GAMEVIEW_SORT_FILTERED_FRIENDSONLY	210

#define GAMEVIEW_GAME_DETAILS				301
#define GAMEVIEW_GAME_PLAYERS				501


enum eGameListColumns
{
	LIST_COLUMNS_GAME,
	LIST_COLUMNS_PLAYERS,
	LIST_COLUMNS_MAP,
	LIST_COLUMNS_DIFFICULTY,
	LIST_COLUMNS_GAME_TIME,
	LIST_COLUMNS_PING_TIME
};


///////////////////////////////////////////////////////////////
// UI icons

enum eUserIcons
{
	ICON_USER_ACTIVE,
	ICON_USER_CHAT,
	ICON_USER_INGAME,
	ICON_USER_AWAY,
	ICON_USER_DND,
	ICON_USER_OFFLINE,
};

enum eChatIcons
{
	ICON_CHAT_SQUELCH,
	ICON_CHAT_IGNORE,
	ICON_CHAT_OWNER,
};

enum eMessageIcons
{
	ICON_MESSAGE_USER,
	ICON_MESSAGE_ALERT,
};

enum eGamesListIcons
{
	ICON_PASSWORD,

	ICON_PING5,
	ICON_PING4,
	ICON_PING3,
	ICON_PING2,
	ICON_PING1,
};

///////////////////////////////////////////////////////////////
// Popup menu commands

enum eMenuList
{
	MENU_SEPERATOR,

	MENU_FRIENDS_PRIVATE_CHAT,
	MENU_FRIENDS_INVITE_CHAT,
	MENU_FRIENDS_WHERE_IS,
	MENU_FRIENDS_IGNORE,
	MENU_FRIENDS_REMOVE,
	MENU_FRIENDS_ADD,
	MENU_FRIENDS_DELETE_MSG,

	MENU_CHAT_PRIVATE_CHAT,
	MENU_CHAT_ADD_TO_FRIENDS,
	MENU_CHAT_IGNORE_USER,
	MENU_CHAT_UNIGNORE_USER,
	MENU_CHAT_SQUELCH_USER,
	MENU_CHAT_UNSQUELCH_USER,
	MENU_CHAT_KICK_USER,

	MENU_MSGING_VIEW,
	MENU_MSGING_DELETE,
	MENU_MSGING_ADD_TO_FRIENDS,
	MENU_MSGING_IGNORE_USER,
	MENU_MSGING_UNIGNORE_USER,
	MENU_MSGING_ACCEPT_REQUEST,
	MENU_MSGING_DENY_REQUEST,
	MENU_MSGING_PRIVATE_CHAT,
	MENU_MSGING_INVITE_CHAT,

	MENU_FILTER_GAMES_SEARCH,
	MENU_FILTER_GAMES_WITH_FRIENDS,
	MENU_FILTER_GAMES_ALL,
};

#define MENU_COMMAND( menu, command )		menu->AddElement( m_MenuList[(int)command], command )


///////////////////////////////////////////////////////////////
// GunCreateInstance

const char Dll_CLASS_OBJECT_FUNC[] = "DllGetClassObject";

typedef HRESULT (STDAPICALLTYPE * DllEntryPoint)(REFCLSID, REFIID, LPVOID *); 

HRESULT GunCreateInstance( HINSTANCE hLib, REFCLSID rclsid, REFIID riid, LPVOID * ppv )
{
    HRESULT hr = E_FAIL;
    
    IClassFactory *pClassFact = NULL;

	// find the DllGetClassObject entry point
    DllEntryPoint pfDllEntryPoint = NULL;

	(FARPROC &)pfDllEntryPoint = GetProcAddress( hLib, Dll_CLASS_OBJECT_FUNC );
	if ( pfDllEntryPoint == NULL )
	{
        return GetLastError();
	}

    hr = pfDllEntryPoint( rclsid, IID_IClassFactory, (LPVOID *)&pClassFact );

    if ( FAILED( hr ) )
    {
        return hr;
    }

    // ok, try to call CreateInstance on the class factory.
    hr = pClassFact->CreateInstance(NULL, riid, ppv);

    if ( FAILED( hr ) )
    {
        return hr;
    }


	if ( pClassFact )
	{
		pClassFact->Release();
	}

    return hr;
}

///////////////////////////////////////////////////////////////
// AutoUpdate
#define SELFUPDATER_MUTEX_NAME				"RAT:AUTOUPDATE:SELFUPDATER"


int WINAPI AutoUpdateDialog( HWND /*hWnd*/, LPSTR pszText, LPSTR /*pszTitle*/, UINT /*uType*/ )
{
	gUIZoneMatch.ShowDialog( UIZoneMatch::DIALOG_OK, ToUnicode( pszText ) );
	return ( IDOK );
}

IUpdateClient2 *CreateUpdateClientObject()
{
	IUpdateClient2 *pUpdateClient = NULL;

	if ( gUIZoneMatch.GetAutoUpdateLib() == NULL )
	{
		return NULL;
	}

    HRESULT hr = GunCreateInstance(	gUIZoneMatch.GetAutoUpdateLib(), CLSID_UpdateClient,          
							IID_IUpdateClient2, (PVOID *)&pUpdateClient );

    if ( FAILED( hr ) )
    {
	    gperrorf(( "AutoUpdate : GunCreateInstance Failed. Result = %x\n", hr ));
        return false;
    }

	/*
	HRESULT hr = CoCreateInstance ( CLSID_UpdateClient,
									NULL,
									CLSCTX_INPROC_SERVER,
									IID_IUpdateClient,
									(PVOID *)&pUpdateClient );
	*/

	if ( SUCCEEDED( hr ) )
	{
		pUpdateClient->SetAppWnd( gAppModule.GetMainWnd() );
		pUpdateClient->SetAppAlertCallback( AutoUpdateDialog );
		return pUpdateClient;
	}

	return NULL;
}

// callback is called once for each patch available
bool PackageCallback( LPPACKAGEDESC lppd, PVOID )
{
	gUIZoneMatch.SetGameUpdatePackage( lppd->dwPackageId );
	return true;
}

// download status callback
bool StatusCallback( LPPACKAGESTATUS pStatus, PVOID )
{
	if ( pStatus->dwTotalBytesToDownload != 0 )
	{
		int maxWidth = 0;

		UIWindow *pProgress = gUIShell.FindUIWindow( "progress_bg", "zonematch_download_status" );
		if ( pProgress )
		{
			maxWidth = pProgress->GetRect().Width() - 6;
		}

		pProgress = gUIShell.FindUIWindow( "progress_bar", "zonematch_download_status" );
		if ( pProgress )
		{
			pProgress->SetVisible( true );
			pProgress->GetRect().right = pProgress->GetRect().left + (int)(((float)pStatus->dwCurrentBytesDownloaded / pStatus->dwTotalBytesToDownload) * maxWidth);
		}

		UIText *pText = (UIText *)gUIShell.FindUIWindow( "status_text", "zonematch_download_status" );
		if ( pText )
		{
			if ( gUIZoneMatch.GetAutoUpdateStatus() == UPDATE_DOWNLOADCLIENT )
			{
				pText->SetText( gpwtranslate( $MSG$ "Downloading AutoUpdate patch..." ) );
			}
			else if ( gUIZoneMatch.GetAutoUpdateStatus() == UPDATE_DOWNLOADGAME )
			{
				pText->SetText( gpwstringf( gpwtranslate( $MSG$ "Downloading game update... (%s)" ), ToUnicode( pStatus->pszTarget ).c_str() ) );
			}
		}
	}

	if ( gUIZoneMatch.GetCancelDownload() )
	{
		return false;
	}
	else
	{
		return true;
	}
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// GUN debugging

struct GunErrors
{   
    char *szError;
    HRESULT hrError;
};
const struct GunErrors aGunErrors[] =
{
    {   "ZT_E_VERSION_NOT_AVAIL",          ZT_E_VERSION_NOT_AVAIL},
    {   "ZT_E_BUFFER_TOO_SMALL",           ZT_E_BUFFER_TOO_SMALL},
    {   "ZT_E_INTERNAL",                   ZT_E_INTERNAL},
    {   "ZT_E_INVALID_STATUS",             ZT_E_INVALID_STATUS},
    {   "ZT_E_STR_TOO_LONG",               ZT_E_STR_TOO_LONG},
    {   "ZT_E_PENDING",                    ZT_E_PENDING},
    {   "ZT_E_ATTR_NOT_FOUND",             ZT_E_ATTR_NOT_FOUND},
    {   "ZT_E_NOT_INITED",                 ZT_E_NOT_INITED},
    {   "ZT_E_ALREADY_INITED",             ZT_E_ALREADY_INITED},
    {   "ZT_E_WRONG_THREAD",               ZT_E_WRONG_THREAD},
    {   "ZT_E_INVALID_CHARACTER",          ZT_E_INVALID_CHARACTER},
    {   "ZT_E_READONLY",                   ZT_E_READONLY},
    {   "ZT_E_MISSING_OBJECT",             ZT_E_MISSING_OBJECT},
    {   "ZT_E_INVALID_OP",                 ZT_E_INVALID_OP},
    {   "ZT_E_TIMEOUT",                    ZT_E_TIMEOUT},
    {   "ZT_E_NOT_LOGGED_IN",              ZT_E_NOT_LOGGED_IN},
    {   "ZT_E_INVALID_USER",               ZT_E_INVALID_USER},
    {   "ZT_E_INVALID_ARG",                ZT_E_INVALID_ARG},
    {   "ZT_E_ALREADY_EXISTS",             ZT_E_ALREADY_EXISTS},
    {   "ZT_E_CANNOT_INIT_WINSOCK",        ZT_E_CANNOT_INIT_WINSOCK},
    {   "ZT_E_FAILED_TO_CONNECT",          ZT_E_FAILED_TO_CONNECT},
    {   "ZT_E_CONNECTION_CLOSED",          ZT_E_CONNECTION_CLOSED},
    {   "ZT_E_SEND_FAILED",                ZT_E_SEND_FAILED},
    {   "ZT_E_NO_ADAPTER",                 ZT_E_NO_ADAPTER},
    {   "ZT_E_NOT_CONNECTED",              ZT_E_NOT_CONNECTED},
    {   "ZT_E_WINSOCK_ERROR",              ZT_E_WINSOCK_ERROR},
    {   "ZT_E_RECEIVE_FAILED",             ZT_E_RECEIVE_FAILED},
    {   "ZT_E_WAITMAX_EXCEEDED",           ZT_E_WAITMAX_EXCEEDED},
    {   "ZT_E_AUTH_INVALID_STATUS",        ZT_E_AUTH_INVALID_STATUS},
    {   "ZT_E_AUTH_PENDING",               ZT_E_AUTH_PENDING},
    {   "ZT_E_AUTH_TIMEOUT",               ZT_E_AUTH_TIMEOUT},
    {   "ZT_E_AUTH_INVALID_PACKET",        ZT_E_AUTH_INVALID_PACKET},
    {   "ZT_E_AUTH_USER_CANCEL",           ZT_E_AUTH_USER_CANCEL},
    {   "ZT_E_AUTH_DENIED",                ZT_E_AUTH_DENIED},
    {   "ZT_E_AUTH_INVALID_TICKET",        ZT_E_AUTH_INVALID_TICKET},
    {   "ZT_E_AUTH_NO_CACHE",              ZT_E_AUTH_NO_CACHE},
    {   "ZT_E_AUTH_NO_LOGIN_INFO",         ZT_E_AUTH_NO_LOGIN_INFO},
    {   "ZT_E_MATCH_NO_KEY",               ZT_E_MATCH_NO_KEY},
    {   "ZT_E_MATCH_NO_PLAYER",            ZT_E_MATCH_NO_PLAYER},
    {   "ZT_E_MATCH_PLAYER_EXISTS",        ZT_E_MATCH_PLAYER_EXISTS},
    {   "ZT_E_MATCH_RONLY_ATTR",           ZT_E_MATCH_RONLY_ATTR},
    {   "ZT_E_PROTOCOL_ERROR",             ZT_E_PROTOCOL_ERROR},
    {   "ZT_E_BAD_PROTOCOL_VER",           ZT_E_BAD_PROTOCOL_VER},
    {   "ZT_E_NO_QUERYSERVER",             ZT_E_NO_QUERYSERVER},
    {   "ZT_E_ALREADY_RUNNING",            ZT_E_ALREADY_RUNNING},
    {   "ZT_E_CLOSED",                     ZT_E_CLOSED},
    {   "ZT_E_INVALID_COLUMN",             ZT_E_INVALID_COLUMN},
    {   "ZT_E_DISCONNECTED",               ZT_E_DISCONNECTED},
    {   "ZT_E_NOT_READY",                  ZT_E_NOT_READY},
    {   "ZT_E_USERNAME_ALREADY_TAKEN",     ZT_E_USERNAME_ALREADY_TAKEN},
    {   "ZT_E_INVALID_USERNAME_LENGTH",    ZT_E_INVALID_USERNAME_LENGTH},
    {   "ZT_E_INVALID_USERNAME_OBSCENE",   ZT_E_INVALID_USERNAME_OBSCENE},
    {   "ZT_E_INVALID_PASSWORD",           ZT_E_INVALID_PASSWORD},
    {   "ZT_E_INVALID_USERID",             ZT_E_INVALID_USERID},
    {   "ZT_E_ACCOUNT_LOCKED",             ZT_E_ACCOUNT_LOCKED},
    {   "ZT_E_MAC_BANNED",                 ZT_E_MAC_BANNED},
    {   "ZT_E_AUTHENTICATION_FAILURE",     ZT_E_AUTHENTICATION_FAILURE},
    {   "ZT_E_CHANGED_TOO_RECENTLY",       ZT_E_CHANGED_TOO_RECENTLY},
    {   "ZT_E_XML_SYNTAX_ERROR",           ZT_E_XML_SYNTAX_ERROR},
    {   "ZT_E_TOO_MANY_ATTRIBUTES",        ZT_E_TOO_MANY_ATTRIBUTES},
	{	"ZT_E_ATTRIBUTE_NOT_SET",		   ZT_E_ATTRIBUTE_NOT_SET},
	{	"ZT_E_REQUEST_DENIED",			   ZT_E_REQUEST_DENIED},
	{	"ZT_E_NOT_REQUESTED",			   ZT_E_NOT_REQUESTED},
    {   "ZT_E_MISMATCHED_XML_TAGS",        ZT_E_MISMATCHED_XML_TAGS},
    {   "ZT_E_FAILED_LIST_ADD",            ZT_E_FAILED_LIST_ADD},
    {   "ZT_E_NOT_IN_LIST",                ZT_E_NOT_IN_LIST},
    {   "ZT_E_ALREADY_IN_LIST",            ZT_E_ALREADY_IN_LIST},
    {   "ZT_E_INVALID_ROOM",               ZT_E_INVALID_ROOM},
    {   "ZT_E_ALREADY_IN_ROOM",            ZT_E_ALREADY_IN_A_ROOM},
    {   "ZT_E_ALREADY_LOGGED_IN",          ZT_E_ALREADY_LOGGED_IN},
	{	"ZT_E_NOT_INVITED",				   ZT_E_NOT_INVITED},
	{	"ZT_E_ROOM_LIMIT",				   ZT_E_ROOM_LIMIT},
    {   "E_FAIL",                          E_FAIL},    
    {   "E_INVALIDARG",                    E_INVALIDARG},
    {   "E_NOINTERFACE",                   E_NOINTERFACE},
    {   "E_NOTIMPL",                       E_NOTIMPL},
    {   "E_PENDING",                       E_PENDING}, 
    {   "E_UNEXPECTED",                    E_UNEXPECTED},  
    {   "S_OK",                            S_OK},
    {   "WAIT_TIMEOUT",                    WAIT_TIMEOUT},
    {   "HOST NOT FOUND",                  HRESULT_FROM_WIN32(WSAHOST_NOT_FOUND)}
};
const int c_cGunErrors = sizeof(aGunErrors) / sizeof(struct GunErrors);

const char * GetHRString( HRESULT hr )
{
    for ( int i = 0; i < c_cGunErrors; i++ )
    {
	    if ( hr == aGunErrors[i].hrError )
		{
            return aGunErrors[i].szError;
		}
    }

	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		hr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL );

	return (const char *)lpMsgBuf;
}

// GUN debugging
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////


UIZoneMatch :: UIZoneMatch()
	: m_pGunConnection( NULL )
	, m_pGunAccount   ( NULL )
	, m_pGunNews      ( NULL )
	, m_pGunChat      ( NULL )
	, m_pGunFriends   ( NULL )
	, m_pGunHost      ( NULL )
	, m_pGunBrowser   ( NULL )
	, m_hGunLib		  ( NULL )
	, m_hAutoUpdateLib( NULL )
	, m_pUpdateClient ( NULL )
	, m_bLoginAutomatically		( true )
	, m_bSignedIn				( false )
	, m_bRequestedDisconnect	( false )
	, m_bIsActivated			( false )
	, m_bInitializedUI			( false )
	, m_bHostOfChannel			( false )
	, m_bRefreshGamesList		( false )
	, m_bRefreshChatRoomList	( false )
	, m_bHostingGame			( false )
	, m_bRequestGameListCreation( false )
	, m_bCancelDownload			( false )
	, m_bShowDialog				( false )
	, m_ActiveFilters			( 0 )
	, m_ActivePairFilters		( 0 )
	, m_GameFlags				( 0 )
	, m_LastRefreshedGameTime	( 0 )
	, m_CurrentGameRowsPerPage	( 20 )
	, m_AutoUpdateStatus		( UPDATE_NONE )
	, m_GameViewType			( SHOW_ALL_GAMES )
	, m_bSizingChatArea			( false )
	, m_pSelectedGamesTab		( NULL )
	, m_ServerStartTime			( 0 )
	, m_LocalStartTime			( 0 )
	, m_bScrollingGamesList		( false )
	, m_GamesScrollStartValue	( 0 )
	, m_bAttemptReconnect		( false )
	, m_bFromInGame				( false )
	, m_inGameMessageOptions	( ZONE_MESSAGE_OPTION_EVERYONE )
	// enable when GUN supports 
	//, m_LanguageFilterFlag		( 0 )
{
	m_pInputBinder = new InputBinder( "zonematch" );
	gAppModule.RegisterBinder( m_pInputBinder, 49 );
	m_pInputBinder->SetIsActive( false );

	m_pInputBinder->PublishVoidInterface( "zonematch_enter", makeFunctor( *this, &UIZoneMatch :: InputPressedEnter	) );
	m_pInputBinder->BindInputToPublishedInterface( KeyInput( Keys::KEY_RETURN,Keys::Q_KEY_DOWN ), "zonematch_enter"     );

	m_pInputBinder->PublishVoidInterface( "zonematch_escape", makeFunctor( *this, &UIZoneMatch :: InputPressedEscape	) );
	m_pInputBinder->BindInputToPublishedInterface( KeyInput( Keys::KEY_ESCAPE,Keys::Q_KEY_DOWN ), "zonematch_escape"     );

	FastFuelHandle hZoneMatch( "config:zonematch" );
	if ( hZoneMatch.IsValid() )
	{
		// retrieve default server settings
		hZoneMatch.Get( "zone_server", m_ServerName );
		hZoneMatch.Get( "zone_server_port", m_ServerPort );

		hZoneMatch.Get( "news_server", m_NewsServerName );
		hZoneMatch.Get( "news_server_file", m_NewsServerFile );
		hZoneMatch.Get( "news_server_port", m_NewsServerPort );

		hZoneMatch.Get( "autoupdate_server", m_AutoUpdateServer );
		hZoneMatch.Get( "autoupdate_proxy", m_AutoUpdateProxy );

		// retrieve registry settings
		if ( !gConfig.Read( "multiplayer\\gun_server", m_ServerName, m_ServerName ) )
		{
			gConfig.Set( "multiplayer\\gun_server", m_ServerName );
		}
		if ( !gConfig.Read( "multiplayer\\gun_server_port", m_ServerPort, m_ServerPort ) )
		{
			gConfig.Set( "multiplayer\\gun_server_port", m_ServerPort );
		}

		if ( !gConfig.Read( "multiplayer\\news_server", m_NewsServerName, m_NewsServerName ) )
		{
			gConfig.Set( "multiplayer\\news_server", m_NewsServerName );
		}
		if ( !gConfig.Read( "multiplayer\\news_server_file", m_NewsServerFile, m_NewsServerFile ) )
		{
			gConfig.Set( "multiplayer\\news_server_file", m_NewsServerFile );
		}
		if ( !gConfig.Read( "multiplayer\\news_server_port", m_NewsServerPort, m_NewsServerPort ) )
		{
			gConfig.Set( "multiplayer\\news_server_port", m_NewsServerPort );
		}

		if ( !gConfig.Read( "multiplayer\\autoupdate_server", m_AutoUpdateServer, m_AutoUpdateServer ) )
		{
			gConfig.Set( "multiplayer\\autoupdate_server", m_AutoUpdateServer );
		}
		if ( !gConfig.Read( "multiplayer\\autoupdate_proxy", m_AutoUpdateProxy, m_AutoUpdateProxy ) )
		{
			gConfig.Set( "multiplayer\\autoupdate_proxy", m_AutoUpdateProxy );
		}
	}

	m_CurrentRoomId = 0;
	m_DefaultJoinChannel = gpwtranslate( $MSG$ "DungeonSiege" );

	m_TimeToRefreshGamesList	= 0.0f;
	m_TimeToRefreshChatRoomList = 0.0f;
	m_TimeToRefreshLatencies	= 0.0f;

	m_ChatAreaSize = CHAT_AREA_MEDIUM;

	m_ConnectionStatus = GUN_DISCONNECTED;

	// Set available filters
	m_Filters.push_back( ZONE_GAME_IN_PROGRESS	);
	m_Filters.push_back( ZONE_GAME_PVP_ENABLED	);
	m_Filters.push_back( ZONE_GAME_PASSWORD_PROTECTED	);
	m_Filters.push_back( ZONE_GAME_HAS_UBER_CHARACTERS_10	);
	m_Filters.push_back( ZONE_GAME_HAS_UBER_CHARACTERS_25	);
	m_Filters.push_back( ZONE_GAME_HAS_UBER_CHARACTERS_50	);
	m_Filters.push_back( ZONE_GAME_HAS_UBER_CHARACTERS_100	);
	m_Filters.push_back( ZONE_GAME_HAS_UBER_CHARACTERS_MAX	);
	m_Filters.push_back( ZONE_GAME_FULL	); 
	m_Filters.push_back( ZONE_GAME_TEAMS_ENABLED );
	// doesn't fit into the gun filter list. m_Filters.push_back( ZONE_GAME_NEW_CHARACTERS_ONLY );
	m_Filters.push_back( ZONE_GAME_DROP_ANYTHING_ON_DEATH );
	// doesn't fit into the gun filter list.  m_Filters.push_back( ZONE_GAME_DROP_EQUIPPED_ON_DEATH );
	// doesn't fit into the gun filter list.  m_Filters.push_back( ZONE_GAME_DROP_INVENTORY_ON_DEATH );
	m_Filters.push_back( ZONE_GAME_WORLD_REGULAR );
	m_Filters.push_back( ZONE_GAME_WORLD_VETERAN );
	m_Filters.push_back( ZONE_GAME_WORLD_ELITE );

	m_ExclusiveFilters.push_back( ZONE_GAME_HAS_UBER_CHARACTERS );
	//m_ExclusiveFilters.push_back( ZONE_GAME_WORLD );

	// filter for our language on or off only.
	// scott's fix m_Filters.push_back( ZONE_GAME_LANGUAGE	<< (eZoneGameFlag) ( ZONE_GAME_LANGUAGE << LocMgr::GetDefaultLanguage() ) );
	// enable when GUN supports 
	// m_Filters.push_back( (eZoneGameFlag) m_LanguageFilterFlag  );
	}


UIZoneMatch::~UIZoneMatch()
{
	if ( m_hGunLib )
	{
		FreeLibrary( m_hGunLib );
	}

	if ( m_hAutoUpdateLib )
	{
		FreeLibrary( m_hAutoUpdateLib );
	}

	Delete( m_pInputBinder );
}


bool UIZoneMatch :: Init()
{
	if ( m_hGunLib )
	{
		return IsInitialized();
	}

	// init COM library
	GPCoInitialize();

	IGunConnection *pGunConnection = NULL;
	IGunAccount    *pGunAccount = NULL;
	IGunNews       *pGunNews = NULL;
	IGunChat       *pGunChat = NULL;
	IGunFriends    *pGunFriends = NULL;
	IGunHost       *pGunHost = NULL;
	IGunBrowser    *pGunBrowser = NULL;

    HRESULT hr = S_OK;

	// load GUN library
	m_hGunLib = LoadLibrary( "gundll.dll" );
	if ( m_hGunLib < (HINSTANCE)HINSTANCE_ERROR )
    {
		gperrorf(( "GUN : LoadLibrary Failed. Make sure GunDll.dll exists in your application path." ));
		return false;
    }
    
    hr = GunCreateInstance(	m_hGunLib, CLSID_GunObject,          
							IID_IGunConnection, (PVOID *)&pGunConnection );

//    hr = CoCreateInstance( CLSID_GunObject, NULL, CLSCTX_INPROC_SERVER,
//                           IID_IGunConnection, (PVOID *)&pGunConnection );

    if( FAILED( hr ) )
    {
	    gperrorf(( "GUN : CoCreateInstance Failed. Result = %x : %s\n", hr, GetHRString( hr ) ));
        return false;
    }

	// Query GUN interfaces
    hr = pGunConnection->QueryInterface( IID_IGunAccount, (PVOID *)&pGunAccount );
    if( FAILED( hr ) )
    {
        gperrorf(( "GUN : QueryInterface for Account Failed. Result = %x\n", hr ));
        return false;
    }
	gpgeneric( "GUN : QI for IGunAccount\n" );

    hr = pGunConnection->QueryInterface( IID_IGunNews, (PVOID *)&pGunNews );
    if( FAILED( hr ) )
    {
        gperrorf(( "GUN : QueryInterface for News Failed. Result = %x\n", hr ));
        return false;
    }
	gpgeneric( "GUN : QI for IGunNews\n" );

	hr = pGunConnection->QueryInterface( IID_IGunChat, (PVOID *)&pGunChat );
    if( FAILED( hr ) )
    {
        gperrorf(( "QueryInterface for Chat Failed. Result = %x\n", hr ));
        return false;
    }
	gpgeneric( "GUN : QI for IGunChat\n" );

	hr = pGunConnection->QueryInterface( IID_IGunFriends, (PVOID *)&pGunFriends );
    if( FAILED( hr ) )
    {
        gperrorf(( "QueryInterface for Friends Failed. Result = %x\n", hr ));
        return false;
    }
	gpgeneric( "GUN : QI for IGunFriends\n" );

	hr = pGunConnection->QueryInterface( IID_IGunHost, (PVOID *)&pGunHost );
    if( FAILED( hr ) )
    {
        gperrorf(( "QueryInterface for Host Failed. Result = %x\n", hr ));
        return false;
    }
	gpgeneric( "GUN : QI for IGunHost\n" );

	hr = pGunConnection->QueryInterface( IID_IGunBrowser, (PVOID *)&pGunBrowser );
    if( FAILED( hr ) )
    {
        gperrorf(( "QueryInterface for Browser Failed. Result = %x\n", hr ));
        return false;
    }
	gpgeneric( "GUN : QI for IGunBrowser\n" );

	// Initialize
    DWORD locale = LocMgr::GetDefaultLocale(); 

    hr = pGunConnection->Init( ZONE_GAME_GUID, ToUnicode( SysInfo::GetExeFileVersionText( gpversion::MODE_AUTO_LONG ) ).c_str(), locale, L"" /*CDKEY*/ );
	if ( FAILED( hr ) )
	{
		gperrorf(( "GUN : Init failed. Result = %x : %s\n", hr, GetHRString( hr ) ));
		return false;
	}

	m_pGunConnection = pGunConnection;
	m_pGunAccount = pGunAccount;
	m_pGunNews = pGunNews;
	m_pGunChat = pGunChat;
	m_pGunFriends = pGunFriends;
	m_pGunHost = pGunHost;
	m_pGunBrowser = pGunBrowser;

	// load AutoUpdate library
	m_hAutoUpdateLib = LoadLibrary( "updateclientdll.dll" );
	if ( m_hAutoUpdateLib == NULL )
	{
		m_hAutoUpdateLib = LoadLibrary( "updateclient.dll" );
	}
	if ( m_hAutoUpdateLib == NULL )
    {
		gperrorf(( "AutoUpdate : LoadLibrary Failed. Make sure AutoUpdate.dll exists in your application path." ));
		return false;
    }

	m_MenuList.push_back( L"-" );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Private Chat..." ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Invite into Chat Room" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "View Current Location..." ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Ignore User" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Remove from Friends List" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Add to Friends List" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Delete Message" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Private Chat..." ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Add to Friends List" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Ignore User" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Unignore User" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Mute User" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Unmute User" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Kick User" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "View Message..." ) ); // this is in the message pane
	m_MenuList.push_back( gpwtranslate( $MSG$ "Delete Message" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Add to Friends List" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Ignore User" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Unignore User" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Accept Friend Add Request" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Deny Friend Add Request" ) );	
	m_MenuList.push_back( gpwtranslate( $MSG$ "Private Chat" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Invite into Chat Room" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "Search by Game Name..." ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "View Only Games with Friends" ) );
	m_MenuList.push_back( gpwtranslate( $MSG$ "View All Games" ) );

    return true;
}


void UIZoneMatch :: Activate()
{
	LoadZoneSettings();

	if ( m_bIsActivated || m_bInitializedUI )
	{
		ShowMatchMakerInterface( true );
		return;
	}

	m_pInputBinder->SetIsActive( true );

	gUIShell.GetMessenger()->RegisterCallback( makeFunctor( *this, &UIZoneMatch::GameGUICallback ) );

	gUIShell.MarkInterfaceForDeactivation( "zonematch" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_panel" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_news" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_games" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_chat" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_game_details" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_create_chatroom" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_signin" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_create_account" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_account_settings" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_friends_settings" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_ignored_settings" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_change_password" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_private_chat" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_send_message" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_view_message" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_dialog" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_input_dialog" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_games_filter" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_games_popup_search" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_popup_menus" );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_news", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_games", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_chat", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_game_details", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_private_chat", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_panel", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_create_chatroom", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_signin", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_create_account", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_account_settings", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_friends_settings", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_ignored_settings", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_change_password", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_send_message", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_view_message", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_input_dialog", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_dialog", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_download_status", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_games_filter", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_games_popup_search", false );
	gUIShell.MarkInterfaceForActivation( "ui:interfaces:multiplayer:zonematch_popup_menus", true );

	m_bIsActivated = true;
	m_bInitializedUI = false;

	if ( (m_ConnectionStatus == GUN_DISCONNECTED) ||
		 (m_ConnectionStatus == GUN_INVALID) )
	{
		m_bSignedIn = false;
		Connect();
	}

	//$$pingm_ZonePing.StartupServer();
	//$$pingm_ZonePing.StartupClient( ZONE_PING_INTERVAL );
}


void UIZoneMatch :: Deactivate()
{
	m_pInputBinder->SetIsActive( false );

	gUIShell.GetMessenger()->UnRegisterCallback( makeFunctor( *this, &UIZoneMatch::GameGUICallback ) );

	gUIShell.MarkInterfaceForDeactivation( "zonematch" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_panel" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_news" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_games" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_chat" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_game_details" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_create_chatroom" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_signin" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_create_account" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_account_settings" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_friends_settings" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_ignored_settings" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_change_password" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_private_chat" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_send_message" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_view_message" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_dialog" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_input_dialog" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_download_status" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_games_filter" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_games_popup_search" );
	gUIShell.MarkInterfaceForDeactivation( "zonematch_popup_menus" );

//	m_ConnectionStatus = GUN_DISCONNECTED;

	m_bIsActivated = false;
	m_bInitializedUI = false;
	m_bShowDialog = false;

	//$$pingm_ZonePing.Shutdown();
}


void UIZoneMatch :: LoadZoneSettings()
{
	FuelHandle prefs = gTattooGame.GetPrefsConfigFuel();
	if ( prefs.IsValid() )
	{
		FuelHandle hSettings = prefs->GetChildBlock( "multiplayer:zone" );
		if ( !hSettings.IsValid() )
		{
			return;
		}

		m_Username = hSettings->GetWString( "username" );

		gpstring readPassword;
		gConfig.Read( "multiplayer\\zone_userpassword", readPassword );
		TeaDecrypt( m_Password, readPassword );

		m_bLoginAutomatically = hSettings->GetBool( "autosignin", true );

		// Game list settings
		m_GameViewType = (eGameViewType)hSettings->GetInt( "game_view_type", SHOW_ALL_GAMES );

		hSettings->Get( "gameslist_filters", m_ActiveFilters );
		hSettings->Get( "gameslist_filters_value", m_ActivePairFilters );
		
		/*
		m_GamesShowInProgress = hSettings->GetBool( "games_show_in_progress", true );
		m_GamesShowPvP = hSettings->GetBool( "games_show_pvp", true );
		m_GamesShowPasswordProtected = hSettings->GetBool( "games_show_password_protected", true );
		*/
	}
}


void UIZoneMatch :: SaveZoneSettings()
{
	FuelHandle prefs = gTattooGame.GetPrefsConfigFuel( true );
	if ( prefs.IsValid() )
	{
		FuelHandle hSettings = prefs->GetChildBlock( "multiplayer", true );
		if ( !hSettings.IsValid() )
		{
			return;
		}
		hSettings = hSettings->GetChildBlock( "zone", true );
		if ( !hSettings.IsValid() )
		{
			return;
		}

		hSettings->Set( "username", m_Username );

		gpstring savePassword;
		TeaEncrypt( savePassword, m_Password );
		gConfig.Set( "multiplayer\\zone_userpassword", savePassword );

		hSettings->Set( "autosignin", m_bLoginAutomatically );

		// Game list settings
		hSettings->Set( "game_view_type", (int)m_GameViewType );

		/*
		hSettings->Set( "games_show_in_progress", m_GamesShowInProgress );
		hSettings->Set( "games_show_pvp", m_GamesShowPvP );
		hSettings->Set( "games_show_password_protected", m_GamesShowPasswordProtected );
		*/
		hSettings->Set( "gameslist_filters", m_ActiveFilters );
		hSettings->Set( "gameslist_filters_value", m_ActivePairFilters );

		prefs->GetDB()->SaveChanges();
	}
}

void UIZoneMatch :: Update( double secondsElapsed )
{
	if( IsEndGame( gWorldState.GetCurrentState() ) )
	{
		m_pGunConnection->DispatchQueue( this );
	}

	if ( !IsInitialized() || !m_bIsActivated )
	{
		return;
	}

	if ( !m_bInitializedUI )
	{
		if ( gUIShell.FindInterface( "zonematch" ) )
		{
			// place common MP interfaces on top
			gUIShell.PlaceInterfaceOnTop( "enter_password" );
			gUIShell.PlaceInterfaceOnTop( "host_game" );
			gUIShell.PlaceInterfaceOnTop( "mp_dialog" );
//			gUIShell.PlaceInterfaceOnTop( "multiplayer_help" );
			gUIShell.HideInterface( "multiplayer_help" );

			ShowMatchMakerInterface();

			m_pFriendsMenu = (UIPopupMenu *)gUIShell.FindUIWindow( "friends_popup_menu" );
			m_pMessagingMenu = (UIPopupMenu *)gUIShell.FindUIWindow( "messaging_popup_menu" );
			m_pChatMenu = (UIPopupMenu *)gUIShell.FindUIWindow( "chat_popup_menu" );
			m_pChatOptionsMenu = (UIPopupMenu *)gUIShell.FindUIWindow( "chat_options_popup_menu" );
			m_pGamesFilterMenu = (UIPopupMenu *)gUIShell.FindUIWindow( "games_filter_popup_menu" );
			gpassert( m_pFriendsMenu );
			gpassert( m_pMessagingMenu );
			gpassert( m_pChatMenu );
			gpassert( m_pChatOptionsMenu );
			gpassert( m_pGamesFilterMenu );

			m_bInitializedUI = true;
		}
	}

	if ( m_bShowDialog )
	{
		if ( gUIMultiplayer.IsMessageBoxPending() || gUIShell.IsInterfaceVisible( "mp_dialog" ) )
		{
			return;
		}

		gUIShell.ShowInterface( "zonematch_dialog" );
		m_bShowDialog = false;
	}

	// Gun Connection Status
	if ( m_ConnectionStatus == GUN_CONNECTING )
	{
		m_ConnectionTimeout -= (float)secondsElapsed;

		if ( m_ConnectionTimeout > 0.0f )
		{
			gUIMultiplayer.ShowMessageBox( gpwstringf( gpwtranslate( $MSG$ "Connecting to ZoneMatch Server... (%d)" ), (int)m_ConnectionTimeout + 1 ), MP_DIALOG_CANCEL_ZONE_CONNECT );
		}
		else
		{
			m_ConnectionStatus = GUN_FAILED_TO_CONNECT;
		}
	}
	else if ( m_ConnectionStatus == GUN_FAILED_TO_CONNECT )
	{
		gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Could not connect to the ZoneMatch Server." ), MP_DIALOG_ZONE_CONNECT_FAILED );

		// must not be GUN_DISCONNECTED because that is reserved for Disconnect() call or actual event from GUN
		m_ConnectionStatus = GUN_INVALID;
	}

	// AutoUpdate
	if ( (m_AutoUpdateStatus != UPDATE_NONE) &&
		 (m_ConnectionStatus == GUN_CONNECTED) && 
		 (gUIMultiplayer.GetActiveMessageBox() != MP_DIALOG_CANCEL_ZONE_LOGIN) &&
		 (gUIMultiplayer.GetActiveMessageBox() != MP_DIALOG_ZONE_CONNECT_FAILED) )
	{
		switch ( m_AutoUpdateStatus )
		{
			case UPDATE_CHECKCLIENT:
			{
				if ( gUIMultiplayer.GetActiveMessageBox() != MP_DIALOG_AUTOUPDATE_CONNECT )
				{
					if ( IsClientUpdateAvailable() )
					{
						gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Checking for new game updates..." ), MP_DIALOG_AUTOUPDATE_CONNECT );
					}
					else
					{
						CheckForGameUpdate();
					}
				}
				else
				{
					CheckForClientUpdate();
				}
				break;
			}

			case UPDATE_DOWNLOADCLIENT:
			{
				if ( WAIT_OBJECT_0 == WaitForSingleObject( m_DownloadWaitHandle, 0 ) )
				{
					DownloadClientUpdateComplete();
				}
				break;
			}

			case UPDATE_UPDATINGCLIENT:
			{
				static double timeSinceCheck = 0.0f;
				
				timeSinceCheck -= secondsElapsed;

				if ( timeSinceCheck <= 0.0f )
				{
					HANDLE hSelfUpdaterMutex = OpenMutex( MUTEX_ALL_ACCESS, FALSE, SELFUPDATER_MUTEX_NAME );

					if ( hSelfUpdaterMutex == NULL )
					{
						gUIMultiplayer.HideMessageBox();

						ShowWindow( gAppModule.GetMainWnd(), SW_RESTORE );

						// Update of Client complete

						m_hAutoUpdateLib = LoadLibrary( "updateclientdll.dll" );
						if ( m_hAutoUpdateLib == NULL )
						{
							m_hAutoUpdateLib = LoadLibrary( "updateclient.dll" );
						}
						if ( m_hAutoUpdateLib == NULL )
						{
							gperrorf(( "AutoUpdate : LoadLibrary Failed. Make sure UpdateClient.dll exists in your application path." ));
							m_hAutoUpdateLib = NULL;
						}
						else
						{
							ShowDialog( DIALOG_CLIENT_UPDATE_SUCCESS, gpwtranslate( $MSG$ "Games Client update has been installed successfully." ) );
						}

						m_AutoUpdateStatus = UPDATE_NONE;
					}

					timeSinceCheck = 2.0f;

					CloseHandle( hSelfUpdaterMutex );
				}
				break;
			}

			case UPDATE_CHECKGAME:
			{
				if ( WAIT_OBJECT_0 == WaitForSingleObject( m_CheckGameUpdateAvailable, 0 ) )
				{
					if ( m_GameUpdatePackageId != 0 )
					{
						// there is an update available
						ShowDialog( DIALOG_INSTALL_GAME_UPDATE, gpwtranslate( $MSG$ "There is an update for Dungeon Siege available. Do you want to install this now?" ) );
					}

					CloseHandle( m_CheckGameUpdateAvailable );
					
					m_pUpdateClient->Release();
					m_pUpdateClient = NULL;
					
					m_AutoUpdateStatus = UPDATE_NONE;
				}
				break;
			}

			case UPDATE_DOWNLOADGAME:
			{
				if ( WAIT_OBJECT_0 == WaitForSingleObject( m_DownloadWaitHandle, 0 ) )
				{
					DownloadGameUpdateComplete();
				}
				break;
			}
		}
	}

	if ( gUIShell.IsInterfaceVisible( "zonematch_create_account" ) )
	{
		bool bEnable = true;

		UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "create_account_user_name_edit_box" );
		if ( eb )
		{
			if ( eb->GetText().size() < 2 )
			{
				bEnable = false;
			}
		}
		eb = (UIEditBox *)gUIShell.FindUIWindow( "create_account_password_edit_box" );
		if ( eb )
		{
			if ( eb->GetText().size() < 2 )
			{
				bEnable = false;
			}
		}
		eb = (UIEditBox *)gUIShell.FindUIWindow( "create_account_password2_edit_box" );
		if ( eb )
		{
			if ( eb->GetText().size() < 2 )
			{
				bEnable = false;
			}
		}
		EnableButton( "button_create_account_ok", bEnable );
	}
	else if ( gUIShell.IsInterfaceVisible( "zonematch_change_password" ) )
	{
		bool bEnable = true;

		UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "change_oldpassword_edit_box" );
		if ( eb )
		{
			if ( eb->GetText().size() < 2 )
			{
				bEnable = false;
			}
		}
		eb = (UIEditBox *)gUIShell.FindUIWindow( "change_newpassword_edit_box" );
		if ( eb )
		{
			if ( eb->GetText().size() < 2 )
			{
				bEnable = false;
			}
		}
		eb = (UIEditBox *)gUIShell.FindUIWindow( "change_newpassword2_edit_box" );
		if ( eb )
		{
			if ( eb->GetText().size() < 2 )
			{
				bEnable = false;
			}
		}
		EnableButton( "button_change_password_ok", bEnable );
	}
	else if ( gUIShell.IsInterfaceVisible( "zonematch_sign_in" ) )
	{
		bool bEnable = true;

		UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "user_name_edit_box" );
		if ( eb )
		{
			if ( eb->GetText().size() < 2 )
			{
				bEnable = false;
			}
		}
		eb = (UIEditBox *)gUIShell.FindUIWindow( "password_edit_box" );
		if ( eb )
		{
			if ( eb->GetText().size() < 2 )
			{
				bEnable = false;
			}
		}
		EnableButton( "signin_button_ok", bEnable );
	}

	if ( gWorldState.GetCurrentState() == WS_MP_MATCH_MAKER )
	{
		if ( m_TimeToRefreshLatencies > 0.0f )
		{
			m_TimeToRefreshLatencies -= (float)secondsElapsed;
			if ( m_TimeToRefreshLatencies < 0.0f )
			{
				m_TimeToRefreshLatencies = 0.0f;
			}
		}
		if ( (m_TimeToRefreshLatencies == 0.0f) && (m_ConnectionStatus == GUN_CONNECTED) )
		{
			// only refresh latencies every 20 sec.  it does a critical lock...
			UpdateLatencies( secondsElapsed );
			m_TimeToRefreshLatencies = (float) ZONE_PING_INTERVAL;
		}

		if ( m_TimeToRefreshGamesList > 0.0f )
		{
			m_TimeToRefreshGamesList -= (float)secondsElapsed;
			if ( m_TimeToRefreshGamesList < 0.0f )
			{
				m_TimeToRefreshGamesList = 0.0f;
			}
		}
		if ( m_bRefreshGamesList && (m_TimeToRefreshGamesList == 0.0f) && (m_ConnectionStatus == GUN_CONNECTED) )
		{
			CreateGamesList( true );
			m_TimeToRefreshGamesList = 0.0f;
			m_bRefreshGamesList = false;
		}

		if ( m_TimeToRefreshChatRoomList > 0.0f )
		{
			m_TimeToRefreshChatRoomList -= (float)secondsElapsed;
			if ( m_TimeToRefreshChatRoomList < 0.0f )
			{
				m_TimeToRefreshChatRoomList = 0.0f;
			}
		}
		if ( m_bRefreshChatRoomList && (m_TimeToRefreshChatRoomList == 0.0f) && m_bSignedIn )
		{
			HRESULT hr = m_pGunChat->GetListOfRooms( NULL );
			gpassertf( !FAILED( hr ), ( "GUN : GetListOfRooms failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

			m_TimeToRefreshChatRoomList = 5.0f;
			m_bRefreshChatRoomList = false;
		}

		if ( m_bScrollingGamesList )
		{
			if ( m_pGamesListSlider->GetValue() == m_HeldGamesScrollPos )
			{
				m_TimeHeldGamesScrollPos += secondsElapsed;
				if ( (m_TimeHeldGamesScrollPos > 0.5) &&
					 (m_GamesScrollStartValue != m_HeldGamesScrollPos) )
				{
					m_GamesScrollStartValue = m_HeldGamesScrollPos;
					m_TimeHeldGamesScrollPos = 0;

					CreateGamesList();
				}
			}
			else
			{
				m_HeldGamesScrollPos = m_pGamesListSlider->GetValue();
				m_TimeHeldGamesScrollPos = 0;
			}
		}
	}

	if ( m_bSizingChatArea )
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( "chat_window_visible_slider" );
		if ( pWindow )
		{
			int yPos = gUIShell.GetMouseY();
			if ( yPos > 420 )
				yPos = 420;
			if ( yPos < 150 )
				yPos = 150;

			pWindow->SetRect( pWindow->GetRect().left, pWindow->GetRect().right, yPos - 1, yPos + 2 );
		}
	}

	//  enable when GUN supports 
	//if( m_LanguageFilterFlag == 0 && m_ConnectionStatus == GUN_CONNECTED)
	//{
	//	// set our location settings
	//	// we only do this once per connection.
	//	m_LanguageFilterFlag  =  ( ZONE_GAME_LANGUAGE << (LocMgr::DoesSingletonExist() ? (DWORD)gLocMgr.GetLanguage() : (DWORD)GetUserDefaultLangID()) );
	//	HostUpdateGameFlag( (eZoneGameFlag) m_LanguageFilterFlag, true );
	//}

	m_pGunConnection->DispatchQueue( this );
}


void UIZoneMatch :: InactiveUpdate()
{
	if ( m_AutoUpdateStatus == UPDATE_UPDATINGCLIENT )
	{
		HANDLE hSelfUpdaterMutex = OpenMutex( MUTEX_ALL_ACCESS, FALSE, SELFUPDATER_MUTEX_NAME );

		if ( hSelfUpdaterMutex == NULL )
		{
			ShowWindow( gAppModule.GetMainWnd(), SW_RESTORE );
		}

		CloseHandle( hSelfUpdaterMutex );
	}
}


void UIZoneMatch :: UpdateInGame( double secondsElapsed )
{
	m_pGunConnection->DispatchQueue( this );

	if ( m_ConnectionStatus != GUN_CONNECTED )
	{
		return;
	}

	if ( gServer.IsLocal() && m_bHostingGame && gServer.GetAllowJIP() )
	{
		if ( gTattooGame.IsUserPaused() )
		{
			m_LastRefreshedGameTime += secondsElapsed;

			if ( m_LastRefreshedGameTime > 60 )
			{
				// if more than a minute of paused time is accumulated, refresh the game start time

				m_LastRefreshedGameTime = 0;

				m_GameStartTime += (DWORD)m_LastRefreshedGameTime;

				HostUpdateGameSetting( ZONE_GAME_TIME, gpwstringf( L"%d", m_GameStartTime ) );
			}
		}
	}
	
	if( IsInitialized() && m_GameFlags & ZONE_GAME_LOADING )
	{
		HostUpdateGameFlag( ZONE_GAME_LOADING, false );
		HostUpdateGameFlag( ZONE_GAME_IN_PROGRESS, true );
	}
}


///////////////////////////////////////////////////////////////
// Connection

bool UIZoneMatch :: Connect()
{
    HRESULT hr = m_pGunConnection->Connect( m_ServerName.c_str(), (unsigned short)m_ServerPort );

	if ( hr != E_PENDING )
	{
		m_ConnectionStatus = GUN_FAILED_TO_CONNECT;
		return false;
	}

	m_ConnectionTimeout = 15.0f;
	m_ConnectionStatus = GUN_CONNECTING;

	m_bRequestedDisconnect = false;

	return true;
}


void UIZoneMatch :: Disconnect()
{
	if ( m_ConnectionStatus != GUN_CONNECTED )
	{
		m_ConnectionStatus = GUN_INVALID;
		return;
	}

	HRESULT hr = m_pGunConnection->Disconnect();

	m_bRequestedDisconnect = true;

	m_pGunConnection->DispatchQueue( this );
		
	m_ConnectionStatus = GUN_DISCONNECTED;

	gpassertf( !FAILED( hr ), ( "GUN : Disconnect failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
}


void UIZoneMatch :: OnZoneCancelConnect()
{
	m_ConnectionStatus = GUN_FAILED_TO_CONNECT;
}


void UIZoneMatch :: OnZoneCancelLogin()
{
	HRESULT hr = m_pGunAccount->LogoutUser( MM_REQUESTED_LOGOUT );
	gpassertf( !FAILED( hr ), ( "GUN : LogoutUser failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
}

// this is the time in sec since jan 1 , 2000
ULONGLONG UIZoneMatch :: GetLocalTimeInSeconds()
{
	ENTER_FPU_PRECISEMODE;

	SYSTEMTIME baseSysTime;
	memset( &baseSysTime, 0, sizeof(SYSTEMTIME) );
	baseSysTime.wYear = 2000;
	baseSysTime.wMonth = 1;
	baseSysTime.wDay = 1;
	FILETIME baseFileTime;
	::SystemTimeToFileTime( &baseSysTime, &baseFileTime );
	ULONGLONG baseTime = (((ULONGLONG)baseFileTime.dwHighDateTime) << 32) + baseFileTime.dwLowDateTime;
	baseTime /= (ULONGLONG)10000000;

	FILETIME sysFileTime;
	::GetSystemTimeAsFileTime( &sysFileTime );
	ULONGLONG sysTime = (((ULONGLONG)sysFileTime.dwHighDateTime) << 32) + sysFileTime.dwLowDateTime;
	sysTime /= (ULONGLONG)10000000;

	ULONGLONG localTime = sysTime - baseTime;
	EXIT_FPU_PRECISEMODE;

	return localTime;
}

ULONGLONG UIZoneMatch :: ToGunTime( ULONGLONG localTime )
{
	ENTER_FPU_PRECISEMODE;
	ULONGLONG gunTime = (INT64)localTime  + ( (INT64)m_ServerStartTime - (INT64)m_LocalStartTime );
	EXIT_FPU_PRECISEMODE;

	return gunTime;
}

ULONGLONG UIZoneMatch :: ToLocalTime( DWORD serverTime )
{
	ENTER_FPU_PRECISEMODE;
	ULONGLONG localTime = (INT64)GetLocalTimeInSeconds() - ( (INT64)serverTime  - ( (INT64)m_ServerStartTime - (INT64)m_LocalStartTime ) );
	EXIT_FPU_PRECISEMODE;

	return localTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// GUN message handling

STDMETHODIMP UIZoneMatch :: Read( GUNQueueMessage *pgqm )
{
    CBaseMessage *pMsg = (CBaseMessage *)pgqm->pvBuffer;
    MessageType msgType = pMsg->GetMessageType();

    switch ( msgType )
    {
		case c_MsgTypeEvent:
		{
			ProcessEvent( (CBaseEvent *)pMsg );
			break;
		}

		case c_MsgTypeResult:
		{
			ProcessResult( (CBaseResult *)pMsg );
			break;
		}

		default:
		{
			gperrorf(( "GUN : Received inappropriate message 0x%x\n", pMsg->GetMessageId() ));
			break;
		}
    }

    return S_OK;
}


void UIZoneMatch :: ProcessEvent( CBaseEvent *pEvent )
{
    CategoryId msgCategory = pEvent->GetMessageCategory();

	switch ( msgCategory )
	{
		case c_CategoryConnect:
		{
			ProcessConnectEvent( pEvent );
			break;
		}

		case c_CategoryAccounts:
		{
			ProcessAccountEvent( pEvent );
			break;
		}

		case c_CategoryChat:
		{
			ProcessChatEvent( pEvent );
			break;
		}

		case c_CategoryFriends:
		{
			ProcessFriendsEvent( pEvent );
			break;
		}

		default:
		{
			gperrorf(( "GUN : Received an unprocessed event: ID#%x\n", pEvent->GetMessageId() ));
			break;
		}
	}
}


void UIZoneMatch :: ProcessConnectEvent( CBaseEvent *pEvent )
{
    switch ( pEvent->GetMessageId() )	
	{
		case c_msgConnectEvent:
		{
			((CConnectEvent *)pEvent)->GetTime( &m_ServerStartTime );

			m_LocalStartTime = GetLocalTimeInSeconds();

			m_ConnectionStatus = GUN_CONNECTED;

			ShowConnectedInterface();
			
			gpgeneric( "GUN : Received a ConnectEvent.\n" );
			break;
		}

		case c_msgDisconnectEvent:
		{
			m_ConnectionStatus = GUN_DISCONNECTED;

			if ( !m_bRequestedDisconnect )
			{
				// make sure we aren't IsOnZone()
				m_bSignedIn = false;

				// update our friend's list if we are in staging/game/endgame
				gUIMultiplayer.UpdatePlayerListBox();

				/*
				Deactivate();
				gUIMultiplayer.ShowMultiplayerMain();
				*/
				if ( IsInGame( gWorldState.GetCurrentState() ) )
				{
					if ( m_bHostingGame )
					{
						m_bAttemptReconnect = true;
					}
				}
				else
				{
					gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "You have been disconnected from the ZoneMatch Server." ) );

					if ( gWorldState.GetCurrentState() == WS_MP_MATCH_MAKER )
					{
						ShowDisconnectedInterface();
					}
				}
			}
			
			m_bRequestedDisconnect = false;

			gpgeneric( "GUN : Received a Disconnect Event.\n" );
			break;
		}

		default:
		{
			gpgenericf(( "GUN : Received an unhandled connect event: ID#%x\n", pEvent->GetMessageId() ));
			break;
		}
    }
}


void UIZoneMatch :: ProcessAccountEvent( CBaseEvent *pEvent )
{
    switch ( pEvent->GetMessageId() )	
	{
		case c_msgLoginDupEvent:
		{
			gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Another user has logged into this ZoneMatch account." ) );
			
			DisableInterfaceOnLogout();

			break;
		}

		default:
		{
			gpgenericf(( "GUN : Received an unhandled accoount event: ID#%x\n", pEvent->GetMessageId() ));
			break;
		}
	}
}


void UIZoneMatch :: ProcessChatEvent( CBaseEvent *pEvent )
{
	DWORD dwRoomId;
	LPCWSTR pwszUserName;
	LPCWSTR pwszChatText;

    switch ( pEvent->GetMessageId() )	
	{
		case c_msgRoomSpeakEvent:
		{
			CRoomSpeakEvent *pRoomSpeakEvent = (CRoomSpeakEvent *)pEvent;
			pRoomSpeakEvent->GetUserName( &pwszUserName );
			pRoomSpeakEvent->GetText( &pwszChatText );
			pRoomSpeakEvent->GetRoomId( &dwRoomId );

			if ( dwRoomId == m_CurrentRoomId )
			{
				UIChatBox *chatBox = (UIChatBox *)gUIShell.FindUIWindow( "match_chatbox" );
				if ( chatBox )
				{
					gpwstring chatLine;
					if( ! gUIEmotes.PrepareStringTextEmotes( pwszChatText, chatLine, pwszUserName /*playerName*/, gpwtranslate( $MSG$ "everyone" ) /*targetName*/, UI_TEXT_EMOTE_TYPE_GROUP ) )
					{
						chatLine.assignf( gpwtranslate( $MSG$ "%s: %s" ), pwszUserName, pwszChatText );
					}
					chatBox->SubmitText( chatLine, 0xFFFFFFFF );
				}
			}
			break;
		}

		case c_msgUserSpeakEvent:
		{
			CUserSpeakEvent *pUserSpeakEvent = (CUserSpeakEvent *)pEvent;
			pUserSpeakEvent->GetUserName( &pwszUserName );
			pUserSpeakEvent->GetText( &pwszChatText );

			gpwstring userName( pwszUserName );
			gpwstring messageText( pwszChatText );

			// if not a friends and we only want friends, break here!
			if( ! IsUserFriend(userName) && GetCurrentUserStatus() == FRIENDS_ONLY )
			{
				break;
			}

			if ( !userName.empty() && !messageText.empty() )
			{
				unsigned int chatInvite = messageText.find( L"#CHATINVITE=" );
				if ( chatInvite != gpwstring::npos )
				{
					UserMessage msg;
					msg.Id   = m_NextMessageId++;
					msg.From = userName;
					msg.Text = messageText.mid( chatInvite + 12, messageText.size() - chatInvite - 12 );
					msg.Type = USER_CHAT_INVITE;
					m_Messages.push_back( msg );

					CreateMessageList();

					UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_messages_list" );
					if ( listBox )
					{
						listBox->FlashElementIcon( msg.Id );
					}
				}
				else
				{
					if ( IsInGame( gWorldState.GetCurrentState() ) || IsInStagingArea( gWorldState.GetCurrentState() ) || IsEndGame( gWorldState.GetCurrentState() ) )
					{
						gpwstring text;
						if( ! gUIEmotes.PrepareStringTextEmotes( messageText, text, userName.c_str() /*playerName*/, gpwtranslate( $MSG$ "you" ) /*targetName*/, UI_TEXT_EMOTE_TYPE_INDIVIDUAL ) )
						{
							text.assignf( gpwtranslate( $MSG$ "%s tells you: %s" ), userName.c_str(), messageText.c_str() );
						}
						
						UIChatBox * pChat = NULL;
						if( IsInGame( gWorldState.GetCurrentState() ) )
						{
							pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box" );
						}
						else if( IsInStagingArea( gWorldState.GetCurrentState() ) )
						{
							pChat = (UIChatBox *)gUIShell.FindUIWindow( "staging_area_chatbox" );
						}	
						else if( IsEndGame( gWorldState.GetCurrentState() ) )
						{
							pChat = (UIChatBox *)gUIShell.FindUIWindow( "end_game_chatbox" );
						}
						 
						if ( pChat )
						{
							pChat->SubmitText( text, 0xFF00FFFF );
						}
					}

					ChatLogMap::iterator findLog = m_ChatLogs.find( userName );
					if ( findLog == m_ChatLogs.end() )
					{
						UserChatLog log;
						log.push_back( UserChatText( MSG_FROM, messageText ) );
						findLog = m_ChatLogs.insert( ChatLogMap::value_type( userName, log ) ).first;
					}
					else
					{
						findLog->second.push_back( UserChatText( MSG_FROM, messageText ) );
					}

					if ( &findLog->second == m_pActiveChatLog )
					{
						UIChatBox * pChatBox = (UIChatBox *)gUIShell.FindUIWindow( "private_chat_box", "zonematch_private_chat" );
						if ( pChatBox )
						{
							gpwstring text;
							if( ! gUIEmotes.PrepareStringTextEmotes( messageText, text, userName.c_str() /*playerName*/, gpwtranslate( $MSG$ "you" ) /*targetName*/, UI_TEXT_EMOTE_TYPE_INDIVIDUAL ) )
							{
								text.assignf( gpwtranslate( $MSG$ "%s: %s" ), userName.c_str(), messageText.c_str() );
							}

							pChatBox->SubmitText( text , 0xffffffff );
						}
					}
					else
					{
						if( IsUserFriend( userName ) )
						{
							// if we are a friend update friends list
							SetNewMessageState( userName );
							UpdateFriendsNewMessages();
						}
						else
						{
							// if we already have a message from this person, make the icon blink.
							for ( UserMessageColl::iterator existingMessage = m_Messages.begin(); existingMessage != m_Messages.end(); existingMessage++ )
							{
								if( same_no_case(existingMessage->From, userName)  )
								{
									UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_messages_list" );
									if ( listBox )
									{
										listBox->FlashElementIcon( existingMessage->Id );
									}

									break;
								}
							}

							if( existingMessage == m_Messages.end() )
							{
								// add us to the message list.
								UserMessage msg;
								msg.Id   = m_NextMessageId++;
								msg.From = userName;
								msg.Type = USER_MESSAGE;
								m_Messages.push_back( msg );

								CreateMessageList();

								UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_messages_list" );
								if ( listBox )
								{
									listBox->FlashElementIcon( msg.Id );
								}
							}
						}
					}
				}
			}
			
			break;
		}

		case c_msgRoomLeaveEvent:
		{
			CRoomEnterEvent *pRoomEnterEvent = (CRoomEnterEvent *)pEvent;
			pRoomEnterEvent->GetUserName( &pwszUserName );		
			pRoomEnterEvent->GetRoomId( &dwRoomId );

			if ( dwRoomId == m_CurrentRoomId )
			{
				/*
				if ( IsUserSquelched( pwszUserName ) )
				{
					UnSquelchUser( pwszUserName );
				}
				*/

				UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
				if ( listBox )
				{
					listBox->RemoveElement( pwszUserName );
				}

				// if the owner left, get the new owner
				if( m_CurrentHostOfChannel.same_no_case( gpwstring(pwszUserName) ) )
				{
					HRESULT hr = m_pGunChat->GetRoomOwner( m_CurrentRoomId, NULL );
					gpassertf( !FAILED( hr ), ( "GUN : GetRoomOwner failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
				}

				//$$$UPDATE2 UIChatBox *chatBox = (UIChatBox *)gUIShell.FindUIWindow( "match_chatbox" );
				//$$$UPDATE2 if ( chatBox )
				//$$$UPDATE2 {
				//$$$UPDATE2 	gpwstring chatLine;
				//$$$UPDATE2 	chatLine.appendf( gpwtranslate( $MSG$ "%s has left the Chat Room" ) , pwszUserName );
				//$$$UPDATE2 	chatBox->SubmitText( chatLine, 0xFF0FF0FF );
				//$$$UPDATE2 }

			}
			break;
		}
			
		case c_msgRoomEnterEvent:
		{
			CRoomEnterEvent *pRoomEnterEvent = (CRoomEnterEvent *)pEvent;
			pRoomEnterEvent->GetUserName( &pwszUserName );		
			pRoomEnterEvent->GetRoomId( &dwRoomId );

			if ( dwRoomId == m_CurrentRoomId )
			{
				UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
				if ( listBox )
				{
					DWORD userColor = (gpwstring( pwszUserName ).find( L"@" ) != gpwstring::npos) ? 0xff00ff00 : 0xffffffff;
					
					listBox->AddElement( pwszUserName, UIListbox::INVALID_TAG, userColor );

					if ( IsUserIgnored( pwszUserName ) )
					{
						listBox->SetElementIcon( pwszUserName, ICON_CHAT_IGNORE );
					}
					else if ( IsUserSquelched( pwszUserName ) )
					{
						listBox->SetElementIcon( pwszUserName, ICON_CHAT_SQUELCH );
					}
					else if ( m_CurrentHostOfChannel.same_no_case( pwszUserName ) )
					{
						listBox->SetElementIcon( pwszUserName, ICON_CHAT_OWNER );
					}
				}

				//$$$UPDATE2 UIChatBox *chatBox = (UIChatBox *)gUIShell.FindUIWindow( "match_chatbox" );
				//$$$UPDATE2 if ( chatBox )
				//$$$UPDATE2 {
				//$$$UPDATE2 	gpwstring chatLine;
				//$$$UPDATE2 	chatLine.appendf( gpwtranslate( $MSG$ "%s has entered the Chat Room" ) , pwszUserName );
				//$$$UPDATE2 	chatBox->SubmitText( chatLine, 0xFF0FF0FF );
				//$$$UPDATE2 }

			}
			break;
		}

		case c_msgKickedOutEvent:
		{
			LPCWSTR pwszReason;
			CKickedOutEvent *pKickedOutEvent = (CKickedOutEvent *)pEvent;
			pKickedOutEvent->GetRoomId( &dwRoomId );
			pKickedOutEvent->GetReason( &pwszReason );

			OnLeaveChatRoom();

			gpwstring kickMessage( gpwtranslate( $MSG$ "You have been kicked from the chat room." ) );
			if ( wcslen( pwszReason ) )
			{
				kickMessage.append( L" " );
				kickMessage.appendf( gpwtranslate( $MSG$ "Reason given: %s" ), pwszReason );
			}

			gUIMultiplayer.ShowMessageBox( kickMessage );

			break;
		}

		case c_msgReceiveOwnershipEvent:
		{
			CReceiveOwnershipEvent* pReceiveOwnershipEvent = (CReceiveOwnershipEvent *)pEvent;
			pReceiveOwnershipEvent->GetRoomId( &dwRoomId );

			if ( dwRoomId == m_CurrentRoomId )
			{
				m_bHostOfChannel = true;
			}
			break;
		}

		default:
		{
    		gpgenericf(( "GUN : Received unhandled chat event: ID#%x\n", pEvent->GetMessageId() ));
			break;
		}
	}
}


void UIZoneMatch :: ProcessFriendsEvent( CBaseEvent *pEvent )
{
	UserModeFlags mode;
	LPCWSTR pwszUserName;

    switch ( pEvent->GetMessageId() )	
	{
		case c_msgFriendStartEvent:
		{
			UserStatus *aFriendsStatus;
			DWORD dwCount;
			
			CFriendStartEvent *pStartEvent = (CFriendStartEvent *)pEvent;
			pStartEvent->GetCount( &dwCount );
			pStartEvent->GetAllFriendsStatus( &aFriendsStatus );

			m_Friends.clear();
			for ( DWORD i = 0; i < dwCount; ++i )
			{
				m_Friends.insert( FriendsMap::value_type( aFriendsStatus[ i ].pwszUserName, aFriendsStatus[ i ].mode ) );
			}

			RefreshFriendsList( true );
			break;
		}
	
		case c_msgFriendStatusEvent:
		{
			CFriendStatusEvent *pFriendStatusEvent = (CFriendStatusEvent *)pEvent;
			pFriendStatusEvent->GetUserName( &pwszUserName );
			pFriendStatusEvent->GetMode( &mode );

			FriendsMap::iterator findFriend = m_Friends.find( pwszUserName );
			if ( findFriend != m_Friends.end() )
			{
				findFriend->second = mode;
			}

			RefreshFriendsList();
			break;
		}

		case c_msgFriendRequestEvent:
		{
			CFriendRequestEvent *pFriendRequestEvent = (CFriendRequestEvent *)pEvent;
			pFriendRequestEvent->GetUserName( &pwszUserName );
			gpwstring userName( pwszUserName );

			if ( !userName.empty() )
			{
				bool dupeRequest = false;

				for ( UserMessageColl::iterator i = m_Messages.begin(); i != m_Messages.end(); ++i )
				{
					if ( (i->Type == USER_REQUEST_ADD) && (i->From.same_no_case( userName )) )
					{
						dupeRequest = true;
						break;
					}
				}
				if ( !dupeRequest )
				{
					UserMessage msg;
					msg.Id = m_NextMessageId++;
					msg.From = userName;
					msg.Type = USER_REQUEST_ADD;
					m_Messages.push_back( msg );

					CreateMessageList();

					UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_messages_list" );
					if ( listBox )
					{
						listBox->FlashElementIcon( msg.Id, 10.0f );
					}
				}
			}
	
			break;
		}

		case c_msgFriendAddCompleteEvent:
		{
			FRIEND_ADD_STATUS friendAddStatus;
			CFriendAddCompleteEvent *pFriendAddCompleteEvent = (CFriendAddCompleteEvent *)pEvent;
			pFriendAddCompleteEvent->GetStatus( &friendAddStatus );
			pFriendAddCompleteEvent->GetUserName( &pwszUserName );

			if ( friendAddStatus == ALLOWED )
			{
				if ( ! IsUserFriend( pwszUserName ) )
				{
					m_Friends.insert( FriendsMap::value_type( pwszUserName, ICON_USER_OFFLINE ) );
					RefreshFriendsList( true );
				}
				
				HRESULT hr = m_pGunFriends->WhereIs( pwszUserName, NULL );
				gpassertf( !FAILED( hr ), ( "GUN : WhereIs failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );				

				gUIMultiplayer.ShowMessageBox( gpwstring().appendf( gpwtranslate( $MSG$ "%s has been added to your Friends List." ), pwszUserName ) );
			}
			else if ( friendAddStatus == DENIED )
			{
				gUIMultiplayer.ShowMessageBox( gpwstring().appendf( gpwtranslate( $MSG$ "%s denied your request to be added to your Friends List." ), pwszUserName ) );
			}
			else
			{
				gUIMultiplayer.ShowMessageBox( gpwstring().appendf( gpwtranslate( $MSG$ "%s logged out before accepting the request to be added onto your Friends List." ), pwszUserName ) );
			}
			break;
		}

		default:
		{
			gpgenericf(( "Receieved unhandled friends event: ID#%x\n", pEvent->GetMessageId() ));
			break;
		}
	}
}


void UIZoneMatch :: ProcessResult( CBaseResult *pResult )
{
    CategoryId msgCategory = pResult->GetMessageCategory();

    switch ( msgCategory )
    {
		case c_CategoryAccounts:
		{
			ProcessAccountsResult( pResult );
			break;
		}

		case c_CategoryNews:
		{
			ProcessNewsResult( pResult );
			break;
		}

		case c_CategoryChat:
		{
			ProcessChatResult( pResult );
			break;
		}

		case c_CategoryFriends:
		{
			ProcessFriendsResult( pResult );
			break;
		}

		case c_CategoryMatch:
		{
			ProcessMatchResult( pResult );
			break;
		}

		default:
		{
			gperrorf(( "GUN : Received Result 0x%x in unprocessed category #0x%x\n", pResult->GetMessageId(), msgCategory ));
			break;
		}
    }
}


void UIZoneMatch :: ProcessAccountsResult( CBaseResult *pResult )
{
    HRESULT hr = S_OK;

	if ( FAILED( pResult->GetHRESULT( &hr ) ) )
	{
		gpassertm( 0, "Bad accounts response packet, can't read HRESULT\n" );
	}
	
    switch ( pResult->GetMessageId() )
    {
		case c_msgLoginResult:
		{
			if ( gUIMultiplayer.GetActiveMessageBox() == MP_DIALOG_CANCEL_ZONE_LOGIN )
			{
				gUIMultiplayer.HideMessageBox();
			}

			if ( hr == S_OK )
			{
				gUIShell.HideInterface( "zonematch_signin" );

				m_Username = m_EnteredUsername;
				m_Password = m_EnteredPassword;
				UICheckbox *checkBox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_auto_signin" );
				if ( checkBox )
				{
					m_bLoginAutomatically = checkBox->GetState();
				}

				SaveZoneSettings();

				ClearPendingRequests();

				EnableInterfaceOnLogin();
			}
			else if ( hr == ZT_E_AUTHENTICATION_FAILURE )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "The Username or Password you entered was invalid." ) );

				if ( gUIShell.IsInterfaceVisible( "zonematch_signin" ) )
				{
					UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "user_name_edit_box", "zonematch_signin" );
					if ( eb )
					{
						eb->SetAllowInput( false );
					}
					eb = (UIEditBox *)gUIShell.FindUIWindow( "password_edit_box", "zonematch_signin" );
					if ( eb )
					{
						eb->SetText( L"" );
						eb->GiveFocus();
					}
				}
			}
			else if ( hr == ZT_E_ALREADY_LOGGED_IN )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "This account is already logged in." ) );
			}
			else if ( hr == ZT_E_MAC_BANNED )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Your system has been banned from the server." ) );
			}
			else if ( hr == ZT_E_ACCOUNT_LOCKED )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "This account has been locked." ) );
			}
			else
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Was unable to connect to the ZoneMatch server with the specified Username." ) );
			}
			break;
		}

		case c_msgLogoutResult:
		{
			PVOID pContext;
			pResult->GetContext( &pContext );
			if ( pContext == MM_REQUESTED_LOGOUT )
			{
				gUIMultiplayer.HideMessageBox();
			}
			else if ( pContext == MM_LOGOUT_NEW_LOGIN )
			{
				gpassert( !m_EnteredUsername.empty() );
				gpassert( !m_EnteredPassword.empty() );

				hr = m_pGunAccount->LoginUser( m_EnteredUsername.c_str(), m_EnteredPassword.c_str(), NULL );
				gpassertf( !FAILED( hr ), ( "GUN : LoginUser failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
				
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Signing in to ZoneMatch Server..." ), MP_DIALOG_CANCEL_ZONE_LOGIN );
			}
			else
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "You have been logged out from your account." ) );
			}

			DisableInterfaceOnLogout();

			break;
		}

		case c_msgCreateAccountResult:
		{
			gUIMultiplayer.HideMessageBox();

			if ( hr == S_OK )
			{
				gUIShell.HideInterface( "zonematch_create_account" );

				// now login
				gpassert( !m_EnteredUsername.empty() );
				gpassert( !m_EnteredPassword.empty() );

				if ( m_bSignedIn )
				{
					gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Signing out of current account..." ), MP_DIALOG_NO_BUTTON );

					hr = m_pGunAccount->LogoutUser( MM_LOGOUT_NEW_LOGIN );
					gpassertf( !FAILED( hr ), ( "GUN : LogoutUser failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
					return;
				}

				HRESULT hr = m_pGunAccount->LoginUser( m_EnteredUsername.c_str(), m_EnteredPassword.c_str(), NULL );
				gpassertf( !FAILED( hr ), ( "GUN : LoginUser failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
				
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Signing in to ZoneMatch Server..." ), MP_DIALOG_CANCEL_ZONE_LOGIN );
			}
			else if ( hr == ZT_E_USERNAME_ALREADY_TAKEN )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "The User Name you chose has already been taken." ) );
			}
			else
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Unable to create the account." ) );
			}
			break;
		}

		case c_msgChangePasswordResult:
		{
			if ( hr == S_OK )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Your password was changed successfully." ) );

				m_Password = m_EnteredPassword;

				gpstring savePassword;
				TeaEncrypt( savePassword, m_Password );
				gConfig.Set( "multiplayer\\zone_userpassword", savePassword );
			}
			else
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Failed to change your password." ) );
			}
			break;
		}

		case c_msgGetAccountStatusResult:
		{
			break;
		}

		default:
		{
			gpgenericf(( "GUN : Undefined Login result messageId=0x%x. Hr=0x%x \n",
												pResult->GetMessageId(), hr ));
			break;
		}
    }
}


void UIZoneMatch :: ProcessNewsResult( CBaseResult *pResult )
{
    HRESULT hr = S_OK;

	if ( FAILED( pResult->GetHRESULT( &hr ) ) )
	{
		gpassertm( 0, "Bad news response packet, can't read HRESULT\n" );
	}

	switch ( pResult->GetMessageId() )
	{
		case c_msgGetNewsResult:
		{
			if ( hr == S_OK )
			{
				LPCWSTR pwszNews;
				((CGetNewsResult *)pResult)->GetNews( &pwszNews );

				UIChatBox *chatBox = (UIChatBox *)gUIShell.FindUIWindow( "match_chatbox_news" );
				if ( chatBox )
				{
					chatBox->Clear();

					gpwstring sNews = pwszNews;

					UINT32 lastPos = 0;
					UINT32 linePos = sNews.find( L"\r\n" );
					while ( linePos != gpwstring::npos )
					{
						gpwstring newLine = sNews.mid( lastPos, linePos - lastPos );
						if ( newLine.empty() )
						{
							newLine = L" ";
						}
						chatBox->SubmitText( newLine, 0xFFFFFFFF );
						lastPos = linePos + 2;
						linePos = sNews.find( L"\r\n", lastPos );
					}
					chatBox->SubmitText( sNews.mid( lastPos, sNews.size() - lastPos + 1 ), 0xFFFFFFFF );
				}
			}
			break;
		}
	
		default:
		{
			gpgenericf(( "GUN : Undefined News result messageId=0x%x. Hr=0x%x \n",
												pResult->GetMessageId(), hr ));
			break;
		}
	}
}


void UIZoneMatch :: ProcessChatResult( CBaseResult *pResult )
{
    HRESULT hr = S_OK;

	if ( FAILED( pResult->GetHRESULT( &hr ) ) )
	{
		gpassertm( 0, "Bad chat response packet, can't read HRESULT\n" );
	}

	if ( hr != S_OK )
	{
		gpgenericf(( "ProcessChatResult failed : %s\n", GetHRString( hr ) ));
	}

    switch ( pResult->GetMessageId() )
    {
		case c_msgCreateRoomResult:
		{
			gUIShell.HideInterface( "zonematch_create_chatroom" );

			if ( hr == S_OK )
			{
				ResizeChatArea( m_ChatAreaSize );

				m_bHostOfChannel = true;
				m_CurrentChannel = m_RequestedChannel;
				m_CurrentChannelPassword = m_RequestedChannelPassword;
				m_CurrentHostOfChannel.clear();

				m_Squelches.clear();

				((CCreateRoomResult *)pResult)->GetRoomId( &m_CurrentRoomId );

				UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "match_chat_edit_box" );
				if ( editBox )
				{
					editBox->SetEnabled( true );
				}

				UIText *text = (UIText *)gUIShell.FindUIWindow( "chat_title", "zonematch" );
				if ( text )
				{
					//gpwstring chatTitle = gpwstringf( gpwtranslate( $MSG$ "Chatting in <c:0x00ffff>%s</c>" ), m_CurrentChannel.c_str() );
					//text->SetToolTipText( chatTitle );

					if ( m_RequestedTopic.empty() )
					{
						text->SetText( gpwstring().appendf( L"%s", m_CurrentChannel.c_str() ) );
					}
					else
					{
						text->SetText( gpwstring().appendf( L"%s - \"%s\"", m_CurrentChannel.c_str(), m_RequestedTopic.c_str() ) );
					}
				}

				UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( "button_chat_leave" );
				if ( pButton )
				{
					pButton->SetVisible( true );
				}

				UITab *pTab = (UITab *)gUIShell.FindUIWindow( "tab_view_chat" );
				if ( pTab )
				{
					pTab->SetVisible( true );
					pTab->SetCheck( true );
				}

				UIListReport *listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_chatroom_list" );
				if ( listReport )
				{
					int tag = listReport->GetNumElements();

					m_ChatModes.insert( ChatModeMap::value_type( tag, m_RequestedChatMode ) );

					listReport->AddElement( m_CurrentChannel, tag );
					listReport->SetElementText( gpwstringf( L"1/%d", m_RequestedUserLimit ), 1, tag );
					listReport->SetElementText( m_RequestedTopic, 2, tag );
				}

				SetCurrentUserStatus( INCHAT );
				UpdateCurrentUserStatus( );

				hr = m_pGunChat->GetUsersInRoom( m_CurrentRoomId, NULL );
				gpassertf( !FAILED( hr ), ( "GUN : GetUsersInRoom failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

				hr = m_pGunChat->GetRoomOwner( m_CurrentRoomId, NULL );
				gpassertf( !FAILED( hr ), ( "GUN : GetRoomOwner failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

				// make sure to refresh our chat list so we have an uptodate list ;)
				RequestChatRefresh();
			}
			else
			{
				if ( !m_CurrentChannel.empty() )
				{
					ResizeChatArea( CHAT_AREA_NONE );
				}

				if ( hr == ZT_E_ALREADY_EXISTS )
				{
					gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "A chat room with this name already exists." ) );
				}
				else
				{
					gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "The chat room was not created." ) );
				}
			}
			break;
		}

		case c_msgJoinRoomResult:
		{
			if ( hr == S_OK )
			{
				ResizeChatArea( m_ChatAreaSize );

				m_bHostOfChannel = false;
				m_CurrentChannel = m_RequestedChannel;
				m_CurrentChannelPassword = m_RequestedChannelPassword;
				m_CurrentHostOfChannel.clear();

				gpwstring topic;
				UIListReport *listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_chatroom_list" );
				if ( listReport )
				{
					int row = listReport->GetElementRowByText( m_CurrentChannel );
					topic = listReport->GetElementText( 2, row );
				}

				((CJoinRoomResult *)pResult)->GetRoomId( &m_CurrentRoomId );

				UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "match_chat_edit_box" );
				if ( editBox )
				{
					editBox->SetEnabled( true );
				}

				UIText *text = (UIText *)gUIShell.FindUIWindow( "chat_title", "zonematch" );
				if ( text )
				{
					if ( topic.empty() )
					{
						text->SetText( gpwstring().appendf( L"%s", m_CurrentChannel.c_str() ) );
					}
					else
					{
						text->SetText( gpwstring().appendf( L"%s - \"%s\"", m_CurrentChannel.c_str(), topic.c_str() ) );
					}
				}

				UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( "button_chat_leave" );
				if ( pButton )
				{
					pButton->SetVisible( true );
				}

				UITab *pTab = (UITab *)gUIShell.FindUIWindow( "tab_view_chat" );
				if ( pTab )
				{
					pTab->SetVisible( true );
					pTab->SetCheck( true );
				}

				SetCurrentUserStatus( INCHAT );
				UpdateCurrentUserStatus( );

				hr = m_pGunChat->GetUsersInRoom( m_CurrentRoomId, NULL );
				gpassertf( !FAILED( hr ), ( "GUN : GetUsersInRoom failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

				hr = m_pGunChat->GetRoomOwner( m_CurrentRoomId, NULL );
				gpassertf( !FAILED( hr ), ( "GUN : GetRoomOwner failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

			}
			else
			{
				PVOID pContext;
				pResult->GetContext( &pContext );
				if ( pContext == MM_LOGIN_JOIN_CHANNEL )
				{
					if ( m_RequestedChannel.same_no_case( m_DefaultJoinChannel ) )
					{
						++m_RequestedChannelSuffix;
						if ( m_RequestedChannelSuffix < 10 )
						{
							gpwstring nextChannel;
							nextChannel.appendf( L"%s%d", m_RequestedChannel.c_str(), m_RequestedChannelSuffix );
							hr = m_pGunChat->JoinRoom( nextChannel, NULL, MM_LOGIN_JOIN_CHANNEL );
							gpassertf( !FAILED( hr ), ( "GUN : JoinRoom failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
						}
					}
				}
				else
				{
					if ( m_CurrentChannel.empty() )
					{
						ResizeChatArea( CHAT_AREA_NONE );
					}

					if ( hr == ZT_E_INVALID_PASSWORD )
					{
						gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "The password you entered was invalid." ) );
					}
					else if ( hr == ZT_E_ROOM_LIMIT )
					{
						gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "The chat room is full." ) );
					}
					else
					{
						gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Unable to join the selected chat room." ) );
					}
				}
			}
			break;
		}

		case c_msgLeaveRoomResult:
		{
			if ( hr == S_OK )
			{
				PVOID pContext;
				pResult->GetContext( &pContext );
				if ( pContext == MM_LEAVE_JOIN_CHANNEL )
				{
					OnLeaveChatRoom( false );

					hr = m_pGunChat->JoinRoom( m_RequestedChannel, m_RequestedChannelPassword.empty() ? NULL : m_RequestedChannelPassword.c_str(), NULL );
					gpassertf( !FAILED( hr ), ( "GUN : JoinRoom failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
				}
				else if ( pContext == MM_LEAVE_HOST_CHANNEL )
				{
					OnLeaveChatRoom( false );

					UIWindow thisLooksPrettyUgly;
					GameGUICallback( "zonematch_create_chatroom_ok", thisLooksPrettyUgly );
				}
				else
				{
					OnLeaveChatRoom();

					ResizeChatArea( CHAT_AREA_NONE );
				}

				// remove all squelches
				for ( UserColl::iterator i = m_Squelches.begin(); i != m_Squelches.end(); ++i )
				{
					UnSquelchUser( *i );
				}
			}
			break;
		}

		case c_msgLeaveAllRoomsResult:
		{
			gpassertm( 0, "not used." );
			break;
		}

		case c_msgSendToRoomResult:
		{
			break;
		}

		case c_msgSendToUserResult:
		{
			if ( hr == ZT_E_NOT_LOGGED_IN )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Your message was not sent because that user is not currently signed in or that user is not accepting messages from you." ) );
			}
			else if ( hr == ZT_E_INVALID_USER )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Your message was not sent because that is not a valid user." ) );
			}
			break;
		}

		case c_msgSquelchAddResult:
		{
			PVOID pContext = NULL;
			pResult->GetContext( &pContext );
			SquelchRequestMap::iterator findRequest = m_SquelchRequests.find( (int)pContext );
			if ( findRequest == m_SquelchRequests.end() )
			{
				break;
			}

			if ( hr == S_OK )
			{
				UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
				if ( listBox )
				{
					listBox->SetElementIcon( findRequest->second.c_str(), ICON_CHAT_SQUELCH );
				}
			}

			hr = m_pGunChat->GetSquelchList( NULL );
			gpassertf( !FAILED( hr ), ( "GUN : GetSquelchList failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

			break;
		}

		case c_msgSquelchDeleteResult:
		{
			PVOID pContext = NULL;
			pResult->GetContext( &pContext );
			SquelchRequestMap::iterator findRequest = m_SquelchRequests.find( (int)pContext );
			if ( findRequest == m_SquelchRequests.end() )
			{
				break;
			}

			if ( hr == S_OK )
			{
				UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
				if ( listBox )
				{
					if ( IsUserIgnored( findRequest->second ) )
					{
						listBox->SetElementIcon( findRequest->second.c_str(), ICON_CHAT_IGNORE );
					}
					else if ( m_CurrentHostOfChannel.same_no_case( findRequest->second ) )
					{
						listBox->SetElementIcon( findRequest->second, ICON_CHAT_OWNER );
					}
					else
					{
						listBox->SetElementIcon( findRequest->second.c_str(), UIListbox::NO_ICON );
					}
				}
			}

			hr = m_pGunChat->GetSquelchList( NULL );
			gpassertf( !FAILED( hr ), ( "GUN : GetSquelchList failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

			break;
		}

		case c_msgGetSquelchListResult:
		{
			if ( hr == S_OK )
			{
				LPCWSTR *userNames;
				DWORD userCount;

				CGetSquelchListResult *pGetSquelchListResult = (CGetSquelchListResult *)pResult;
				pGetSquelchListResult->GetCount( &userCount );
				pGetSquelchListResult->GetIgnored( &userNames );
			
				m_Squelches.clear();
				for ( DWORD i = 0; i < userCount; ++i )
				{
					m_Squelches.push_back( userNames[i] );
				}
			} 
			break;
		}

		case c_msgAssignOwnershipResult:
		{
			break;
		}

		case c_msgSetRoomModeResult:
		{
			break;
		}

		case c_msgSetRoomTopicResult:
		{
			break;
		}

		case c_msgInviteUserResult:
		{
			break;
		}

		case c_msgKickOutUserResult:
		{
			break;
		}

		case c_msgGetUsersInRoomResult:
		{
			if ( hr == S_OK )
			{
				LPCWSTR *userNames;
				DWORD userCount;

				CGetUsersInRoomResult *pGetUsersInRoomResult = (CGetUsersInRoomResult *)pResult;
				pGetUsersInRoomResult->GetCount( &userCount );
				pGetUsersInRoomResult->GetUsers( &userNames );

				UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
				if ( listBox )
				{
					listBox->RemoveAllElements();

					for ( DWORD i = 0; i < userCount; ++i )
					{
						DWORD userColor = (gpwstring( userNames[i] ).find( L"@" ) != gpwstring::npos) ? 0xff00ff00 : 0xffffffff;

						listBox->AddElement( userNames[i], i, userColor );
						if ( IsUserIgnored( userNames[i] ) )
						{
							listBox->SetElementIcon( i, ICON_CHAT_IGNORE );
						}
						else if ( m_CurrentHostOfChannel.same_no_case( userNames[i] ) )
						{
							listBox->SetElementIcon( i, ICON_CHAT_OWNER );
						}
					}
				}
			} 
			break;
		}

		case c_msgGetListOfRoomsResult:
		{
			if ( hr == S_OK )
			{
				DWORD roomCount = 0;
				DWORD roomTotal = 0;
				DWORD roomIndex = 0;
				ChatRoomAttrs *roomList;

				CGetListOfRoomsResult *pGetListOfRoomsResult = (CGetListOfRoomsResult *)pResult;
				pGetListOfRoomsResult->GetCount( &roomCount );
				pGetListOfRoomsResult->GetTotal( &roomTotal );
				pGetListOfRoomsResult->GetIndex( &roomIndex );
				pGetListOfRoomsResult->GetRoomList( &roomList );

				UIListReport *listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_chatroom_list" );
				if ( listReport )
				{
					if ( roomIndex == 0 )
					{
						listReport->RemoveAllElements();
						m_ChatModes.clear();
					}

					for ( DWORD i = 0; i < roomCount; i++ )
					{
						int roomPosition = i + roomIndex;
						m_ChatModes.insert( ChatModeMap::value_type( roomPosition, (IGunChat::ChatModeFlags)roomList[i].mode ) );

						listReport->AddElement( roomList[i].pwszName, roomPosition );

						gpwstring users;
						users.assignf( L"%d/%d", roomList[i].cUserCount, roomList[i].cUserLimit );
						listReport->SetElementText( users, 1, roomPosition );

						listReport->SetElementText( roomList[i].pwszTopic, 2, roomPosition );
						/*
						char szRoomMode[80];
						GetRoomModeText(szRoomMode, aRoomList[ci].mode, 80);
						printf( "Mode: %s  ", szRoomMode);
						*/
					}
				}
			}
			break;
		}

		case c_msgGetRoomOwnerResult:
		{
			LPCWSTR ownerUserName;
			CGetRoomOwnerResult* pGetRoomOwnerResult = (CGetRoomOwnerResult *)pResult;
			pGetRoomOwnerResult->GetUserName( &ownerUserName );
			m_CurrentHostOfChannel.assign( gpwstring(ownerUserName)  );

			// update the chat room icons for the owner of the room
			UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
			if ( listBox )
			{
				listBox->SetElementIcon( m_CurrentHostOfChannel, ICON_CHAT_OWNER );
			}
			
			break;
		}

		default:
		{
			gpgenericf(( "GUN : Undefined Chat result messageId=0x%x. Hr=0x%x \n",
												pResult->GetMessageId(), hr ));
			break;
		}
	}
	
	UpdateDynamicChatToolTips();
}


void UIZoneMatch :: ProcessFriendsResult( CBaseResult *pResult )
{
    HRESULT hr = S_OK;

	if ( FAILED( pResult->GetHRESULT( &hr ) ) )
	{
		gpassertm( 0, "Bad friends response packet, can't read HRESULT\n" );
	}

    switch ( pResult->GetMessageId() )
    {
		case c_msgFlagsResult:
		{
			DWORD dwFlags;
			IGunFriends::FLAG_WORD flagType;
			CFlagsResult *pFlagsResult = (CFlagsResult *)pResult;
			pFlagsResult->GetWhichFlag( (DWORD *)&flagType );
			pFlagsResult->GetFlags( &dwFlags );

			if ( hr == S_OK )
			{
				if ( flagType == IGunFriends::CONTROL )
				{
					m_bDisableFriends = (( dwFlags & IGunFriends::NO_FRIENDS ) > 0);
					m_bRequestBeforeAdd = (( dwFlags & IGunFriends::ASK_BEFORE_ACCEPT ) > 0);
				}
				else if ( flagType == IGunFriends::STATE )
				{
				}
			}

	        break;
		}	

		case c_msgWhereIsResult:
		{
			UserModeFlags mode;
			LPCWSTR pwszUserName;
			LPCWSTR pwszLocation;
			CWhereIsResult *pWhereIsResult = (CWhereIsResult *)pResult;
			pWhereIsResult->GetUserName( &pwszUserName );
			pWhereIsResult->GetMode( &mode );
			pWhereIsResult->GetLocation( &pwszLocation );

			FriendsMap::iterator findFriend = m_Friends.find( pwszUserName );
			if ( findFriend != m_Friends.end() )
			{
				findFriend->second = mode;
				RefreshFriendsList();
			}

			PVOID pContext;
			pResult->GetContext( &pContext );
			if ( pContext == MM_REQUEST_WHERE_IS )
			{
				gpwstring sLocation = pwszLocation;
				if ( sLocation.left( 4 ).same_no_case( L"GAME" ) )
				{
					gpwstring sGameName = sLocation.right( sLocation.size() - 5 );
					gUIMultiplayer.ShowMessageBox( gpwstring().appendf( gpwtranslate( $MSG$ "%s's current location:\nPlaying in game \"%s\"" ), pwszUserName, sGameName.c_str() ) );
//					gUIMultiplayer.ShowMessageBox( gpwstring().appendf( gpwtranslate( $MSG$ "%s's current location:\nPlaying in game <c:0x00ffff>%s</c>" ), pwszUserName, sGameName.c_str() ) );
				}
				else if ( sLocation.left( 4 ).same_no_case( L"CHAT" ) )
				{
					gpwstring sChatRoom = sLocation.right( sLocation.size() - 5 );
					gUIMultiplayer.ShowMessageBox( gpwstring().appendf( gpwtranslate( $MSG$ "%s's current location:\nIn chat room \"%s\"" ), pwszUserName, sChatRoom.c_str() ) );
//					gUIMultiplayer.ShowMessageBox( gpwstring().appendf( gpwtranslate( $MSG$ "%s's current location:\nIn chat room <c:0x00ffff>%s</c>" ), pwszUserName, sChatRoom.c_str() ) );
				}
				else if ( sLocation.left( 6 ).same_no_case( L"BROWSE" ) )
				{
					gUIMultiplayer.ShowMessageBox( gpwstring().appendf( gpwtranslate( $MSG$ "%s's current location:\nBrowsing" ), pwszUserName ) );
//					gUIMultiplayer.ShowMessageBox( gpwstring().appendf( gpwtranslate( $MSG$ "%s's current location:\n<c:0x00ffff>Browsing</c>" ), pwszUserName ) );
				}
				else
				{
					gUIMultiplayer.ShowMessageBox( gpwstring().appendf( gpwtranslate( $MSG$ "%s's current location is not available." ), pwszUserName ) );
				}
			}
	        break;
		}	

		case c_msgFriendAddResult:
		{
			PVOID pContext = NULL;
			pResult->GetContext( &pContext );
			FriendRequestMap::iterator findRequest = m_FriendAddRequests.find( (int)pContext );
			if ( findRequest == m_FriendAddRequests.end() )
			{
				break;
			}

			if ( hr == S_OK || hr == ZT_E_ALREADY_IN_LIST )
			{
				if ( ! IsUserFriend( findRequest->second ) )
				{
					m_Friends.insert( FriendsMap::value_type( findRequest->second, ICON_USER_OFFLINE ) );
					RefreshFriendsList( true );
				}
				else
				{
					gUIMultiplayer.ShowMessageBox( gpwstringf( gpwtranslate( $MSG$ "A user named \"%s\" is already in your friends list." ), findRequest->second.c_str() ) );
				}

				hr = m_pGunFriends->WhereIs( findRequest->second.c_str(), NULL );
				gpassertf( !FAILED( hr ), ( "GUN : WhereIs failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );				

				gUIMultiplayer.ShowMessageBox( gpwstringf( gpwtranslate( $MSG$ "%s has been added to your friends list." ), findRequest->second.c_str() ) );
			}
			else if ( hr == E_PENDING )
			{
				gUIMultiplayer.ShowMessageBox( gpwstringf( gpwtranslate( $MSG$ "You have requested to add %s to your Friends List." ), findRequest->second.c_str() ) );
			}
			else if ( hr == ZT_E_REQUEST_DENIED )
			{
				gUIMultiplayer.ShowMessageBox( gpwstringf( gpwtranslate( $MSG$ "%s is currently not accepting messages." ), findRequest->second.c_str() ));
			}
			else if ( hr == ZT_E_INVALID_USER )
			{
				gUIMultiplayer.ShowMessageBox( gpwstringf( gpwtranslate( $MSG$ "A user named \"%s\" does not exist." ), findRequest->second.c_str() ) );
			}
			else if ( hr == ZT_E_NOT_LOGGED_IN )
			{
				gUIMultiplayer.ShowMessageBox( gpwstringf( gpwtranslate( $MSG$ "The user \"%s\" is not currently signed in to approve your request." ), findRequest->second.c_str() ) );
			}
			else
			{
				gpassertf( 0, ("Hit unhandled AddFriend result: %s", GetHRString( hr )) );
			}
	        break;
		}	

		case c_msgFriendDeleteResult:
		{
			RefreshFriendsList( true );
	        break;
		}	

		case c_msgIgnoredAddResult:
		{
			RefreshFriendsList( true );
	        break;
		}	

		case c_msgIgnoredDeleteResult:
		{
			RefreshFriendsList( true );
	        break;
		}	

		case c_msgAcceptAddFriendResult:
		{
			RefreshFriendsList( true );
	        break;
		}

		case c_msgGetFriendsListResult:
		{
			m_Friends.clear();

			if ( hr == S_OK )
			{
				DWORD dwCount;
				UserStatus *aFriendsStatus;

				CGetFriendsListResult *pGetFriendsListResult = (CGetFriendsListResult *)pResult;
				pGetFriendsListResult->GetCount( &dwCount );
				pGetFriendsListResult->GetAllFriendsStatus( &aFriendsStatus );

				for ( DWORD i = 0; i < dwCount; ++i )
				{
					m_Friends.insert( FriendsMap::value_type( aFriendsStatus[ i ].pwszUserName, aFriendsStatus[ i ].mode ) );
				}
			}

			RefreshFriendsList( true );
	        break;
		}	

		case c_msgGetIgnoredListResult:
		{
			UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_ignore_list" );
			if ( !listBox )
			{
				return;
			}

			m_Ignored.clear();

			listBox->RemoveAllElements();

			if ( hr == S_OK )
			{
				DWORD dwCount;
				LPCWSTR *awszIgnored;
				CGetIgnoredListResult *pGetIgnoredListResult = (CGetIgnoredListResult *)pResult;
				pGetIgnoredListResult->GetCount( &dwCount );
				pGetIgnoredListResult->GetIgnored( &awszIgnored );

				for ( DWORD i = 0; i < dwCount; ++i )
				{
					m_Ignored.push_back( awszIgnored[i] );
					listBox->AddElement( awszIgnored[i] );
				}		

				RefreshFriendsList( true );
			}
	        break;
		}	

	    default:
		{
			gpgenericf(( "GUN : Undefined Friends result messageId=0x%x. Hr=0x%x \n",
												pResult->GetMessageId(), hr ));
			break;
		}
    }
}


void UIZoneMatch :: ProcessMatchResult( CBaseResult *pResult )
{
    HRESULT hr = S_OK;

	if ( FAILED( pResult->GetHRESULT( &hr ) ) )
	{
		gpassertm( 0, "Bad match response packet, can't read HRESULT\n" );
	}

	switch ( pResult->GetMessageId() )
    {
		case c_msgGetViewListResult:
		{
			/*
			DWORD viewCount;
			VIEW_DESCRIPTION *pViews;

			CGetViewListResult *pGetViewListRes = (CGetViewListResult *)pResult;
			pGetViewListRes->GetCount( &viewCount );
			pGetViewListRes->GetViewList( &pViews );

			gpgenericf(( "There are %d views.\n", viewCount ));
			if (viewCount > 0)
			  printf( "#\tDescription\n");
			  for (iViews=0; iViews < dwCount; iViews++)
				printf( "%d\t%ls\n", aViews[iViews].dwId, aViews[iViews].pwszName);
			*/
			break;
		}

		case c_msgGetHeaderRowResult:
		{
			if ( hr == S_OK )
			{
				PVOID pContext;
				pResult->GetContext( &pContext );

				LPGUNROW pRowHdrs;
				CGetHeaderRowResult *pGetHeaderRowResult = (CGetHeaderRowResult *)pResult;
				pGetHeaderRowResult->GetHeaders( &pRowHdrs );

				ViewHeaderMap::iterator findView = m_ViewHeaders.find( (unsigned int)pContext );
				if ( findView == m_ViewHeaders.end() )
				{
					findView = m_ViewHeaders.insert( ViewHeaderMap::value_type( (unsigned int)pContext, ViewHeaderColl() ) ).first;
				}
				findView->second.clear();

				for ( DWORD i = 0; i < pRowHdrs->Count(); ++i )
				{
					findView->second.push_back( ToAnsi( pRowHdrs->Strings( i ) ) );
				}

				if ( m_bRequestGameListCreation && ((DWORD)pContext == m_CurrentViewId) )
				{
					m_pGamesListSlider->SetValue( 0 );
					CreateGamesList( true );
				}
			}
			break;
		}

	    case c_msgGetPageResult:
		case c_msgGetRowPageResult:
		{
//			gpassertf( hr == S_OK, ( "GetRowPage Failed: %s", GetHRString( hr ) ) );

		    LPGUNROW *apRows;
			DWORD rowCount = 0;
			DWORD viewId, viewIndex, viewTotal;

			CGetPageResult *pGetPageResult = (CGetPageResult *)pResult;
			pGetPageResult->GetRows( &apRows );
			pGetPageResult->GetCount( &rowCount );
			pGetPageResult->GetViewId( &viewId );
			pGetPageResult->GetViewIndex( &viewIndex );
			pGetPageResult->GetViewTotal( &viewTotal );

			PVOID pContext;
			pResult->GetContext( &pContext );

			const bool bRefreshGameDetails = (pContext == MM_REFRESH_GAME_DETAILS) || (pContext == MM_REFRESH_GAME_AND_JOIN);
			const bool bFindGame = (viewId == GAMEVIEW_SEARCH_NAME);
			const bool bRefreshGamesList = !bRefreshGameDetails && !bFindGame;

			if ( bRefreshGamesList )
			{
				UIText * pText = (UIText *)gUIShell.FindUIWindow( "zonematch_games_count" );
				if ( pText )
				{
					if ( viewTotal > 1 )
					{
						pText->SetText( gpwstringf( gpwtranslate( $MSG$ "%d Games" ), viewTotal ) );
					}
					else
					{
						pText->SetText( L"" );
					}
				}
				DWORD currentPage = 0;
				if ( viewIndex > 0 )
				{
					currentPage = viewTotal / viewIndex;
				}
			}


			if ( bRefreshGameDetails && (viewId != GAMEVIEW_GAME_PLAYERS) )
			{
				if ( rowCount > 0 )
				{
					gUIShell.HideInterface( "zonematch_games" );
					gUIShell.ShowInterface( "zonematch_game_details" );
					ShowGameDetails();
				}
				else
				{
					gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "The selected game is no longer available." ) );
					ShowGamesTab();
					return;
				}

				if ( pContext == MM_REFRESH_GAME_AND_JOIN )
				{
					GameInfoMap::iterator findGame = m_GamesList.find( m_CurrentGameRowId );
					if ( (findGame != m_GamesList.end()) && !findGame->second.ipAddress.empty() )
					{
						if ( !gUIMultiplayer.JoinGame( ToUnicode( findGame->second.ipAddress ) ) )
						{
							// attempt join on local host ip
							gUIMultiplayer.HideMessageBox();
							gUIMultiplayer.JoinGame( ToUnicode( findGame->second.ipAddress2 ) );
						}

						return;
					}
				}
			}

			if ( hr != S_OK )
			{
				return;
			}

			UIListReport * listReport = NULL;

			if ( viewId == GAMEVIEW_GAME_PLAYERS )
			{
				listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_game_player_list" );
			}
			else
			{
				listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_games_list" );
			}

			if ( listReport == NULL )
			{
				return;
			}

			ViewHeaderMap::iterator findView = m_ViewHeaders.find( viewId );
			if ( findView == m_ViewHeaders.end() )
			{
				gpassertm( 0, "No header found for the specified view." );
				return;
			}
			
			ViewHeaderColl &viewHeader = findView->second;

			if ( bRefreshGamesList )
			{
				if ( (rowCount > 0) && (rowCount != m_CurrentGameRowsPerPage) && ((viewIndex + rowCount) < viewTotal) )
				{
					m_CurrentGameRowsPerPage = rowCount;
					m_CurrentViewsPerPage = (unsigned int)ceilf( (float)m_CurrentGameRowsPerPage / listReport->GetMaxActiveElements() );
				}

				if ( m_pGamesListSlider )
				{
					if ( m_CurrentGameRowsPerPage > 0 )
					{
						int scrollMax = (int)ceilf( (float)m_CurrentViewsPerPage * ((float)viewTotal / m_CurrentGameRowsPerPage) ) - 1;
						m_pGamesListSlider->SetRange( scrollMax );
					}
					else
					{
						m_pGamesListSlider->SetRange( 0 );
					}

					m_pGamesListSlider->CalculateSlider();
					m_pGamesListSlider->SetVisible( m_pGamesListSlider->GetMin() < m_pGamesListSlider->GetMax() );
				}

				unsigned int pageAt = m_pGamesListSlider->GetValue() / m_CurrentViewsPerPage;

				if ( pageAt != m_CurrentRequestedGamePage )
				{
					// slider was changed, cancel list creation
					return;
				}

				UIWindow * pText = gUIShell.FindUIWindow( "refreshing_games_text", "zonematch_games" );
				if ( pText )
				{
					pText->SetVisible( false );
				}
			}

			for ( DWORD iRow = 0; iRow < rowCount; ++iRow )
			{
				GameInfo gameInfo;
				PlayerInfo playerInfo;
				int rowId = 0;

				for ( DWORD iVal = 0; iVal < apRows[iRow]->Count(); ++iVal )
				{
					if ( iVal < viewHeader.size() )
					{
						// game listing

						if ( viewHeader[ iVal ].same_no_case( "Rid" ) )
						{
							stringtool::Get( apRows[ iRow ]->Strings( iVal ), rowId );
						}
						else if ( viewHeader[ iVal ].same_no_case( "GName" ) )
						{
							gameInfo.Name = apRows[ iRow ]->Strings( iVal );
						}
						else if ( viewHeader[ iVal ].same_no_case( "GameV" ) )
						{
							gameInfo.Version = ToAnsi( apRows[ iRow ]->Strings( iVal ) );
						}
						else if ( viewHeader[ iVal ].same_no_case( "Locale" ) )
						{
							gameInfo.Locale = ToAnsi( apRows[ iRow ]->Strings( iVal ) );
						}
						else if ( viewHeader[ iVal ].same_no_case( "IpAddr" ) )
						{
							gameInfo.ipAddress = ToAnsi( apRows[ iRow ]->Strings( iVal ) );
							gameInfo.ipDWORD = ntohl( inet_addr( gameInfo.ipAddress ) );
						}
						else if ( viewHeader[ iVal ].same_no_case( "Ip2" ) )
						{
							gameInfo.ipAddress2 = ToAnsi( apRows[ iRow ]->Strings( iVal ) );
						}
						else if ( viewHeader[ iVal ].same_no_case( "SFlags" ) )
						{
							stringtool::Get( apRows[ iRow ]->Strings( iVal ), gameInfo.flags );
						}
						else if ( viewHeader[ iVal ].same_no_case( "Flags" ) )
						{
							stringtool::Get( gpwstring( L"0x" ).append( apRows[ iRow ]->Strings( iVal ) ), gameInfo.flags );
						}
						else if ( viewHeader[ iVal ].same_no_case( "Map" ) )
						{
							gameInfo.mapName = apRows[ iRow ]->Strings( iVal );
						}
						else if ( viewHeader[ iVal ].same_no_case( "World" ) )
						{
							gameInfo.worldLevel = apRows[ iRow ]->Strings( iVal );
							if ( gameInfo.worldLevel.empty() )
							{
								gameInfo.worldLevel = gpwtranslate( $MSG$ "None" );
							}
						}
						else if ( viewHeader[ iVal ].same_no_case( "NumP" ) )
						{
							stringtool::Get( apRows[ iRow ]->Strings( iVal ), gameInfo.numPlayers );
						}
						else if ( viewHeader[ iVal ].same_no_case( "MaxP" ) )
						{
							stringtool::Get( apRows[ iRow ]->Strings( iVal ), gameInfo.maxPlayers );
						}
						else if ( viewHeader[ iVal ].same_no_case( "Difficulty" ) )
						{
							int difficulty = 0;
							stringtool::Get( apRows[ iRow ]->Strings( iVal ), difficulty );
							if ( (eDifficulty)difficulty == DIFFICULTY_EASY )
							{
								gameInfo.difficulty = gpwtranslate( $MSG$ "Easy" );
							}
							else if ( (eDifficulty)difficulty == DIFFICULTY_MEDIUM )
							{
								gameInfo.difficulty = gpwtranslate( $MSG$ "Normal" );
							}
							else if ( (eDifficulty)difficulty == DIFFICULTY_HARD )
							{
								gameInfo.difficulty = gpwtranslate( $MSG$ "Hard" );
							}
						}
						else if ( viewHeader[ iVal ].same_no_case( "Time" ) )
						{
							stringtool::Get( apRows[ iRow ]->Strings( iVal ), gameInfo.gameTime );
						}
						else if ( viewHeader[ iVal ].same_no_case( "TimeL" ) )
						{
							stringtool::Get( apRows[ iRow ]->Strings( iVal ), gameInfo.timeLimit );
						}
						else if ( viewHeader[ iVal ].same_no_case( "InGame" ) )
						{
							gpwstring inProgress = apRows[ iRow ]->Strings( iVal );
							gameInfo.inProgress = inProgress.same_no_case( L"y" );
						}

						// player listing

						else if ( viewHeader[ iVal ].same_no_case( "User" ) )
						{
							playerInfo.Name = apRows[ iRow ]->Strings( iVal );
						}
						else if ( viewHeader[ iVal ].same_no_case( "PTeam" ) )
						{
							playerInfo.Team = apRows[ iRow ]->Strings( iVal );
						}
						else if ( viewHeader[ iVal ].same_no_case( "PChar" ) )
						{
							playerInfo.Character = apRows[ iRow ]->Strings( iVal );
						}
						else if ( viewHeader[ iVal ].same_no_case( "PLev" ) )
						{
							playerInfo.Level = apRows[ iRow ]->Strings( iVal );
						}
					}
				}

				if ( viewId == GAMEVIEW_GAME_PLAYERS )
				{
					listReport->AddElement( playerInfo.Name, rowId );
					int col = 1;
					if ( listReport->GetColumnCount() == 4 )
					{
						listReport->SetElementText( playerInfo.Team, col++, iRow );
					}
					listReport->SetElementText( playerInfo.Character, col++, iRow );
					listReport->SetElementText( playerInfo.Level, col++, iRow );
				}
				else if ( viewId == GAMEVIEW_SEARCH_NAME )
				{
					hr = m_pGunBrowser->GetRowPage( m_CurrentViewId, rowId, 0, NULL, NULL );
					gpassertf( !FAILED( hr ), ( "GUN : GetRowPage failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
					return;
				}
				else
				{
					GameInfoMap::iterator findGame = m_GamesList.find( rowId );
					if ( findGame != m_GamesList.end() )
					{
						findGame->second = gameInfo;
					}
					else
					{
						m_GamesList.insert( GameInfoMap::value_type( rowId, gameInfo ) );
						m_GamesSorted.push_back( rowId );

						//$$pingm_ZonePing.Add( gameInfo.ipDWORD );
					}
				}

				if ( bRefreshGamesList )
				{
					/*
					listReport->AddElement( gameInfo.Name, gameInfo.Version.same_no_case( SysInfo::GetExeFileVersionText( gpversion::MODE_AUTO_LONG ) ) ? 0xFFFFFFFF : 0xFFFF0000, rowId );
					//listReport->SetElementIcon( rowId, ICON_PING1 );

					int listRow = listReport->GetNumElements() - 1;
					listReport->SetElementText( gameInfo.mapName, 1, listRow );
					listReport->SetElementText( gameInfo.worldLevel, 2, listRow );
					listReport->SetElementText( gpwstringf( L"%d/%d", gameInfo.numPlayers, gameInfo.maxPlayers ), 3, listRow );
					listReport->SetElementText( (gameInfo.flags & ZONE_GAME_IN_PROGRESS) != 0 ? gpwtranslate( $MSG$ "Yes" ) : gpwtranslate( $MSG$ "No" ), 4, listRow );

					if ( gameInfo.Name.same_no_case( m_SelectedGameName ) )
					{
						listReport->SetSelectedElement( listRow );
					}

					if ( listReport->GetNumElements() == listReport->GetMaxActiveElements() )
					{
						return;
					}
					*/
				}
			}

			if ( bRefreshGamesList )
			{
				// Grab enough pages to fill the entire view
				if ( (m_GamesSorted.size() < listReport->GetMaxActiveElements()) &&
					 (((int)m_CurrentGameEndPage + 1) <= (m_pGamesListSlider->GetMax() / (int)m_CurrentViewsPerPage)) )
				{
					++m_CurrentGameEndPage;
					hr = m_pGunBrowser->GetPage( m_CurrentViewId, m_CurrentGameEndPage, 0, NULL, NULL );
					gpassertf( !FAILED( hr ), ( "GUN : GetPage failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
				}
				else
				{
					CreateGamesList();
				}
			}
			else if ( bRefreshGameDetails )
			{
				if ( viewId != GAMEVIEW_GAME_PLAYERS )
				{
					ShowGameDetails();
				}
			}
			break;
		}

		default:
		{
			gpgenericf(( "GUN : Undefined Match result messageId=0x%x. Hr=0x%x \n",
												pResult->GetMessageId(), hr ));
			break;
		}
    }
}

void UIZoneMatch :: UpdateLatencies( double /*secondsElapsed*/ )
{
	/*
	UIListReport * pListReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_games_list" );
	if ( !pListReport )
	{
		return;
	}

	GameInfo * pInfo = NULL;
	for ( GameInfoMap::iterator i = m_GamesList.begin(); i != m_GamesList.end(); ++i )
	{
		pInfo = &i->second;
		if ( pInfo->ipDWORD )
		{
			pInfo->latency = 0;
			m_ZonePing.Lookup( pInfo->ipDWORD, &pInfo->latency );

			if ( (pInfo->latency == 0xFFFFFFFF) )
			{
				pListReport->SetElementText( gpwtranslate( $MSG$ "NA" ), (int)LIST_COLUMNS_PING_TIME, i->first );
			}
			else
			{
				pListReport->SetElementText( gpwstringf( L"%d", pInfo->latency ), (int)LIST_COLUMNS_PING_TIME, i->first );
			}

		}
	}
	*/
}

///////////////////////////////////////////////////////////////
// Callbacks

STDMETHODIMP UIZoneMatch :: OnEnumPlayer( IN DWORD dwItemId, IN LPCWSTR pwszPlayerName )
{
	UNREFERENCED_PARAMETER( dwItemId );
	UNREFERENCED_PARAMETER( pwszPlayerName );
	return S_OK;
}


STDMETHODIMP UIZoneMatch :: OnEnumItemValue( IN LPCSTR pszKey, IN LPCWSTR pwszValue )
{
	UNREFERENCED_PARAMETER( pszKey );
	UNREFERENCED_PARAMETER( pwszValue );
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// UI message handling

void UIZoneMatch :: GameGUICallback( gpstring const & message, UIWindow & ui_window )
{
	///////////////////////////////////////////////////////////////

	if ( message.same_no_case( "zonematch_show_news" ) )
	{
		ShowNewsTab();
	}
	else if ( message.same_no_case( "zonematch_show_games" ) )
	{
		ShowGamesTab();
	}
	else if ( message.same_no_case( "zonematch_show_chat" ) )
	{
		ShowChatTab();
	}

	///////////////////////////////////////////////////////////////
	// Dialogs

	else if ( message.same_no_case( "zonematch_dialog_cancel" ) )
	{
		HideDialogs();
	}
	else if ( message.same_no_case( "zonematch_dialog_yes" ) )
	{
		switch ( m_CurrentDialog )
		{
			case DIALOG_ACCEPT_ADD_FRIEND:
			{
				HRESULT hr = m_pGunFriends->AcceptAddFriend( true, m_AcceptAddUser.c_str(), NULL );
				gpassertf( !FAILED( hr ), ( "GUN : AcceptAddFriend failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

				CreateMessageList();
				break;
			}

			case DIALOG_IGNORE_FRIEND:
			{
				IgnoreUser( m_RequestIgnoreUser );
				break;
			}

			case DIALOG_INVITE_CHAT:
			{
				if ( m_CurrentChannel.same_no_case( m_RequestedChannel ) )
				{
					break;
				}

				HRESULT hr;
				if ( !m_CurrentChannel.empty() )
				{
					hr = m_pGunChat->LeaveRoom( m_CurrentRoomId, MM_LEAVE_JOIN_CHANNEL );
				}
				else
				{
					hr = m_pGunChat->JoinRoom( m_RequestedChannel.c_str(), m_RequestedChannelPassword.empty() ? NULL : m_RequestedChannelPassword.c_str(), NULL );
				}
				gpassertf( !FAILED( hr ), ( "GUN : JoinRoom failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
				break;
			}

			case DIALOG_CHAT_SIGN_IN:
			{
				ShowSignInDialog();
				break;
			}

			case DIALOG_INSTALL_CLIENT_UPDATE:
			{
				DownloadClientUpdate();
				break;
			}

			case DIALOG_INSTALL_GAME_UPDATE:
			{
				DownloadGameUpdate();
				break;
			}
		}

		gUIShell.HideInterface( "zonematch_dialog" );
	}
	else if ( message.same_no_case( "zonematch_dialog_no" ) )
	{
		if ( m_CurrentDialog == DIALOG_ACCEPT_ADD_FRIEND )
		{
			HRESULT hr = m_pGunFriends->AcceptAddFriend( false, m_AcceptAddUser.c_str(), NULL );
			gpassertf( !FAILED( hr ), ( "GUN : AcceptAddFriend failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

			CreateMessageList();
		}

		gUIShell.HideInterface( "zonematch_dialog" );
	}
	else if ( message.same_no_case( "zonematch_dialog_ok" ) )
	{
		if ( m_CurrentDialog == DIALOG_INSTALL_GAME_EXIT_NOW )
		{
			if ( InstallGameUpdate() )
			{
				Disconnect();
				Deactivate();
				gAppModule.RequestQuit();
			}
		}
		else if ( m_CurrentDialog == DIALOG_CLIENT_UPDATE_SUCCESS )
		{
			CheckForGameUpdate();
		}

		gUIShell.HideInterface( "zonematch_dialog" );
	}
	else if ( message.same_no_case( "zonematch_input_dialog_ok" ) )
	{
		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "input_dialog_edit_box" );
		if ( !editBox || editBox->GetText().empty() )
		{
			return;
		}

		if ( m_CurrentDialog == DIALOG_JOIN_PASSWORD_CHATROOM )
		{
			HRESULT hr;
			if ( !m_CurrentChannel.empty() )
			{
				m_RequestedChannelPassword = editBox->GetText();
				hr = m_pGunChat->LeaveRoom( m_CurrentRoomId, MM_LEAVE_JOIN_CHANNEL );
			}
			else
			{
				hr = m_pGunChat->JoinRoom( m_RequestedChannel, editBox->GetText().c_str(), NULL );
			}
			gpassertf( !FAILED( hr ), ( "GUN : JoinRoom failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
		}
		else if ( m_CurrentDialog == DIALOG_CHAT_CHANGE_TOPIC )
		{
			gpwstring topic = editBox->GetText();

			HRESULT hr = m_pGunChat->SetRoomTopic( m_CurrentRoomId, topic.c_str(), NULL );
			gpassertf( !FAILED( hr ), ( "GUN : SetRoomTopic failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

			UIText *text = (UIText *)gUIShell.FindUIWindow( "chat_title", "zonematch" );
			if ( text )
			{
				//gpwstring chatTitle = gpwstringf( gpwtranslate( $MSG$ "Chatting in <c:0x00ffff>%s</c>" ), m_CurrentChannel.c_str() );
				//text->SetToolTipText( chatTitle );

				if ( topic.empty() )
				{
					text->SetText( gpwstring().appendf( L"%s", m_CurrentChannel.c_str() ) );
				}
				else
				{
					text->SetText( gpwstring().appendf( L"%s - \"%s\"", m_CurrentChannel.c_str(), topic.c_str() ) );
				}
			}
		}
		else if ( m_CurrentDialog == DIALOG_HOST_GAME )
		{
			/*
			m_CurrentGame = editBox->GetText();

			gUIShell.HideInterface( "zonematch_input_dialog" );

			m_bHostingGame = true;

			m_GamePlayers.clear();

			HRESULT hr = m_pGunHost->CreateGame( m_CurrentGame.c_str() );
			gpassertf( !FAILED( hr ), ( "GUN : CreateGame failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
			if ( FAILED( hr ) )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "The game could not be created." ) );
				return;
			}

			hr = m_pGunHost->SetItemValue( 0, "IpAddr", ToUnicode( SysInfo::GetIPAddress() ) );
			gpassertf( !FAILED( hr ), ( "GUN : SetItemValue failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
			
			// Create the game session	
			if( !gNetPipe.Host( m_CurrentGame, L"", 8 ) )
			{
				gpassertm( 0, "Could not create game session.\n" );
				return;
			}

			// Create local player
			if ( m_bSignedIn )
			{
				m_PlayerName = m_Username;
			}

			gpassert( gServer.GetLocalHumanPlayer() == NULL );

			// Update user state
			SetCurrentUserStatus( INGAME );

			// Enter Staging Area
			gUIShell.HideInterface( "zonematch" );
			gUIMultiplayer.ShowStagingAreaServer();
			*/
		}
		else if ( m_CurrentDialog == DIALOG_PLAYER_NAME_JOIN_GAME )
		{
			gUIShell.HideInterface( "zonematch_input_dialog" );

			m_PlayerName = editBox->GetText();

			if ( !m_PlayerName.empty() )
			{
				FuelHandle prefs = gTattooGame.GetPrefsConfigFuel( true );
				if ( prefs.IsValid() )
				{
					FuelHandle hSettings = prefs->GetChildBlock( "multiplayer", true );
					if ( hSettings.IsValid() )
					{
						hSettings->Set( "zone_player_name", m_PlayerName );
					}
					prefs->GetDB()->SaveChanges();
				}
			}

			GameGUICallback( "zonematch_join_game", ui_window );
			return;
		}
		else if ( m_CurrentDialog == DIALOG_PLAYER_NAME_HOST_GAME )
		{
			gUIShell.HideInterface( "zonematch_input_dialog" );

			m_PlayerName = editBox->GetText();

			if ( !m_PlayerName.empty() )
			{
				FuelHandle prefs = gTattooGame.GetPrefsConfigFuel( true );
				if ( prefs.IsValid() )
				{
					FuelHandle hSettings = prefs->GetChildBlock( "multiplayer", true );
					if ( hSettings.IsValid() )
					{
						hSettings->Set( "zone_player_name", m_PlayerName );
					}
					prefs->GetDB()->SaveChanges();
				}
			}

			GameGUICallback( "zonematch_host_game", ui_window );
			return;
		}
		else if ( m_CurrentDialog == DIALOG_ADD_FRIEND )
		{
			if ( !editBox->GetText().same_no_case( m_Username ) )
			{
				AddFriendToZoneMatch( editBox->GetText() );
			}
		}

		gUIShell.HideInterface( "zonematch_input_dialog" );
	}
	else if ( message.same_no_case( "zonematch_cancel_download" ) )
	{
		m_bCancelDownload = true;
	}

	///////////////////////////////////////////////////////////////
	// Sign In

	else if ( message.same_no_case( "zonematch_connect" ) )
	{
		if ( (m_ConnectionStatus == GUN_DISCONNECTED) ||
			 (m_ConnectionStatus == GUN_INVALID) )
		{
			Connect();
		}
	}
	else if ( message.same_no_case( "zonematch_sign_in" ) )
	{
		ShowSignInDialog();
	}
	else if ( message.same_no_case( "zonematch_sign_in_fresh" ) )
	{
		gUIShell.HideInterface( "zonematch_create_account" );
		gUIShell.ShowInterface( "zonematch_signin" );

		UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "user_name_edit_box" );
		if ( eb )
		{
			stringtool::RemoveBorderingWhiteSpace( m_Username ); //BUG: 12818
			eb->SetText( m_Username );
			eb->SetAllowInput( true );
		}
		eb = (UIEditBox *)gUIShell.FindUIWindow( "password_edit_box" );
		if ( eb )
		{
			eb->SetText( m_Password );
			eb->SetAllowInput( false );
		}
		UICheckbox *checkBox = (UICheckbox *)gUIShell.FindUIWindow( "match_chat_edit_box" );
		if ( checkBox )
		{
			checkBox->SetState( m_bLoginAutomatically );
		}
	}
	else if ( message.same_no_case( "zonematch_sign_in_ok" ) )
	{
		gpwstring userName;
		gpwstring passWord;

		UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "user_name_edit_box" );
		if ( eb )
		{
			userName = eb->GetText();
		}
		eb = (UIEditBox *)gUIShell.FindUIWindow( "password_edit_box" );
		if ( eb )
		{
			passWord = eb->GetText();
		}

		if ( userName.empty() || passWord.empty() )
		{
			return;
		}

		stringtool::RemoveBorderingWhiteSpace( userName ); //BUG: 12818
		m_EnteredUsername = userName;
		m_EnteredPassword = passWord;

		if ( m_bSignedIn )
		{
			HRESULT hr = m_pGunAccount->LogoutUser( MM_LOGOUT_NEW_LOGIN );
			gpassertf( !FAILED( hr ), ( "GUN : LogoutUser failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
			return;
		}

		HRESULT hr = m_pGunAccount->LoginUser( userName.c_str(), passWord.c_str(), NULL );
		gpassertf( !FAILED( hr ), ( "GUN : LoginUser failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

		gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Signing in to ZoneMatch Server..." ), MP_DIALOG_CANCEL_ZONE_LOGIN );
	}
	else if ( message.same_no_case( "zonematch_create_account" ) )
	{
		ShowCreateAccountDialog();
	}
	else if ( message.same_no_case( "zonematch_create_account_ok" ) )
	{
		gpwstring userName;
		gpwstring password;
		gpwstring password2;

		UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "create_account_user_name_edit_box" );
		if ( eb )
		{
			userName = eb->GetText();
			stringtool::RemoveBorderingWhiteSpace( userName ); //BUG: 12818
		}
		eb = (UIEditBox *)gUIShell.FindUIWindow( "create_account_password_edit_box" );
		if ( eb )
		{
			password = eb->GetText();
		}
		eb = (UIEditBox *)gUIShell.FindUIWindow( "create_account_password2_edit_box" );
		if ( eb )
		{
			password2 = eb->GetText();
		}

		if ( !password.same_no_case( password2 ) )
		{
			gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "The passwords you entered do not match." ) );
			return;
		}

		if ( userName.empty() || password.empty() )
		{
			return;
		}

		m_EnteredUsername = userName;
		m_EnteredPassword = password;

		gpwstring question = L"What is the meaning of this?";
		
		gpstring answer;
		for ( int i = 0; i < 10; ++i )
		{
			answer.append( 1, (char)Random( (int)'A', (int)'z' ) );
		}

		HRESULT hr = m_pGunAccount->CreateAccount( userName.c_str(), password.c_str(), question.c_str(), ToUnicode( answer ).c_str(), NULL );
		gpassertf( !FAILED( hr ), ( "GUN : CreateAccount failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

		gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Attempting to create account with ZoneMatch Server..." ), MP_DIALOG_NO_BUTTON );
	}

	///////////////////////////////////////////////////////////////
	// Options

	else if ( message.same_no_case( "zonematch_show_options" ) )
	{
		gUIShell.ShowInterface( "zonematch_account_settings" );

		UICheckbox *checkBox = (UICheckbox *)gUIShell.FindUIWindow( "disable_friends_for_account" );
		if ( checkBox )
		{
			checkBox->SetState( m_bDisableFriends );
		}
		UpdateGameMessageOptions( "zonematch_friends_settings", "zm_options_game_zmmessage" );
		//$$$checkBox = (UICheckbox *)gUIShell.FindUIWindow( "disable_friends_in_game" );
		//$$$if ( checkBox )
		//$$${
			//$$$checkBox->SetState( m_bDisableFriendsInGame );
		//$$$}
		checkBox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_ask_before_add" );
		if ( checkBox )
		{
			checkBox->SetState( m_bRequestBeforeAdd );
		}

		HRESULT hr = m_pGunFriends->GetIgnoredList( NULL );
		gpassertf( !FAILED( hr ), ( "GUN : GetIgnoredList failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	}
	else if ( message.same_no_case( "zonematch_account_settings" ) )
	{
		gUIShell.HideInterface( "zonematch_friends_settings" );
		gUIShell.HideInterface( "zonematch_ignored_settings" );
		gUIShell.ShowInterface( "zonematch_account_settings" );
	}
	else if ( message.same_no_case( "zonematch_friends_settings" ) )
	{
		gUIShell.HideInterface( "zonematch_account_settings" );
		gUIShell.HideInterface( "zonematch_ignored_settings" );
		gUIShell.ShowInterface( "zonematch_friends_settings" );
	}
	else if ( message.same_no_case( "zonematch_ignored_settings" ) )
	{
		gUIShell.HideInterface( "zonematch_account_settings" );
		gUIShell.HideInterface( "zonematch_friends_settings" );
		gUIShell.ShowInterface( "zonematch_ignored_settings" );
	}
	else if ( message.same_no_case( "zonematch_settings_ok" ) )
	{
		gUIShell.HideInterface( "zonematch_account_settings" );
		gUIShell.HideInterface( "zonematch_friends_settings" );
		gUIShell.HideInterface( "zonematch_ignored_settings" );

		bool bChangeFriendsState = false;

		UICheckbox *checkBox = (UICheckbox *)gUIShell.FindUIWindow( "disable_friends_for_account" );
		if ( checkBox )
		{
			if ( checkBox->GetState() != m_bDisableFriends )
			{
				bChangeFriendsState = true;
			}
			m_bDisableFriends = checkBox->GetState();
		}
		
		UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_zmmessage", "zonematch_friends_settings" );
		if( pList )
		{
			if( (eInGameMessageOption)pList->GetSelectedTag() != GetGameMessageOptions() )
			{
				bChangeFriendsState = true;
			}
			SetGameMessageOptions( (eInGameMessageOption)pList->GetSelectedTag());
		}
		//$$$checkBox = (UICheckbox *)gUIShell.FindUIWindow( "disable_friends_in_game" );
		//$$$if ( checkBox )
		//$$${
			//$$$if ( checkBox->GetState() != m_bDisableFriendsInGame )
			//$$${
			//$$$	bChangeFriendsState = true;
			//$$$}
			//$$$m_bDisableFriendsInGame = checkBox->GetState();
			//$$$SetGameMessageOptions( m_bDisableFriendsInGame ? ZONE_MESSAGE_OPTION_NOONE : ZONE_MESSAGE_OPTION_EVERYONE );
		//$$$}
		checkBox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_ask_before_add" );
		if ( checkBox )
		{
			if ( checkBox->GetState() != m_bRequestBeforeAdd )
			{
				bChangeFriendsState = true;
			}
			m_bRequestBeforeAdd = checkBox->GetState();
		}

		if ( bChangeFriendsState )
		{
			DWORD dwFlags = 0;
			if ( m_bDisableFriends )
			{
				dwFlags |= IGunFriends::NO_FRIENDS;
			}
			if ( m_bRequestBeforeAdd )
			{
				dwFlags |= IGunFriends::ASK_BEFORE_ACCEPT;
			}
			HRESULT hr = m_pGunFriends->Flags( IGunFriends::SET, IGunFriends::CONTROL, dwFlags, NULL );
			gpassertf( !FAILED( hr ), ( "GUN : Flags failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

			dwFlags = 0;
			if ( !m_bDisableFriends )
			{
				dwFlags |= IGunFriends::NO_FRIENDS;
			}
			if ( !m_bRequestBeforeAdd )
			{
				dwFlags |= IGunFriends::ASK_BEFORE_ACCEPT;
			}
			hr = m_pGunFriends->Flags( IGunFriends::CLEAR, IGunFriends::CONTROL, dwFlags, NULL );
			gpassertf( !FAILED( hr ), ( "GUN : Flags failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
		}
	}
	else if ( message.same_no_case( "zonematch_change_password" ) )
	{
		gUIShell.HideInterface( "zonematch_account_settings" );

		gUIShell.ShowInterface( "zonematch_change_password" );

		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "change_oldpassword_edit_box" );
		if ( editBox )
		{
			editBox->SetText( L"" );
			editBox->SetAllowInput( true );
		}
		editBox = (UIEditBox *)gUIShell.FindUIWindow( "change_newpassword_edit_box" );
		if ( editBox )
		{
			editBox->SetText( L"" );
			editBox->SetAllowInput( false );
		}
		editBox = (UIEditBox *)gUIShell.FindUIWindow( "change_newpassword2_edit_box" );
		if ( editBox )
		{
			editBox->SetText( L"" );
			editBox->SetAllowInput( false );
		}
	}
	else if ( message.same_no_case( "zonematch_change_password_ok" ) )
	{
		gpwstring oldPassword;
		gpwstring newPassword;

		UIEditBox * giveEditTo = NULL;

		UIEditBox * editBox = (UIEditBox *)gUIShell.FindUIWindow( "change_newpassword_edit_box" );
		if ( editBox )
		{
			editBox->SetAllowInput( false );
			newPassword = editBox->GetText();
			if ( newPassword.empty() )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Please enter a new password." ) );
				giveEditTo = editBox;
			}
		}
		editBox = (UIEditBox *)gUIShell.FindUIWindow( "change_newpassword2_edit_box" );
		if ( editBox )
		{
			editBox->SetAllowInput( false );
			if ( !newPassword.empty() && !newPassword.same_no_case( editBox->GetText() ) )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "The passwords you entered do not match." ) );
				giveEditTo = editBox;
			}
		}
		editBox = (UIEditBox *)gUIShell.FindUIWindow( "change_oldpassword_edit_box" );
		if ( editBox )
		{
			editBox->SetAllowInput( false );
			oldPassword = editBox->GetText();
			if ( oldPassword.empty() )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "You must enter your current password." ) );
				giveEditTo = editBox;
			}
		}

		if ( giveEditTo != NULL )
		{
			giveEditTo->GiveFocus();
			return;
		}

		m_EnteredPassword = newPassword;

		HideDialogs();

		HRESULT hr = m_pGunAccount->ChangePassword( oldPassword.c_str(), newPassword.c_str(), NULL );
		gpassertf( !FAILED( hr ), ( "GUN : ChangePassword failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	}
	else if ( message.same_no_case( "zonematch_sign_off" ) )
	{
		gUIShell.HideInterface( "zonematch_account_settings" );

		gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Logging out of your account..." ), MP_DIALOG_NO_BUTTON );

		HRESULT hr = m_pGunAccount->LogoutUser( MM_REQUESTED_LOGOUT );
		gpassertf( !FAILED( hr ), ( "GUN : LogoutUser failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	}
	else if ( message.same_no_case( "zonematch_settings_remove_ignore" ) )
	{
		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_ignore_list" );
		if ( listBox && !listBox->GetSelectedText().empty() )
		{
			UnignoreUser( listBox->GetSelectedText() );

			listBox->RemoveElement( listBox->GetSelectedText() );
		}
	}
	else if ( message.same_no_case( "zonematch_settings_clearall_ignore" ) )
	{
		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_ignore_list" );
		if ( listBox )
		{
			for ( unsigned int i = 0; i < listBox->GetNumElements(); ++i )
			{
				UnignoreUser( listBox->GetElement(i)->lines.begin()->sText );
			}
			listBox->RemoveAllElements();
		}
	}

	///////////////////////////////////////////////////////////////
	// Chat

	else if ( message.same_no_case( "zonematch_join_chatroom" ) )
	{
		UIListReport *listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_chatroom_list" );
		if ( !listReport || listReport->GetSelectedElementText().empty() )
		{
			return;
		}

		ChatModeMap::iterator chatMode = m_ChatModes.find( listReport->GetSelectedElementTag() );
		if ( chatMode == m_ChatModes.end() )
		{
			return;
		}

		if ( m_CurrentChannel.same_no_case( listReport->GetSelectedElementText() ) )
		{
			return;
		}

		m_RequestedChannel = listReport->GetSelectedElementText();
		m_RequestedChannelPassword.clear();

		if ( chatMode->second & IGunChat::PASSWORD )
		{
			ShowInputDialog( DIALOG_JOIN_PASSWORD_CHATROOM, gpwtranslate( $MSG$ "JOIN CHAT ROOM" ), gpwtranslate( $MSG$ "Enter Password" ) );
			return;
		}

		HRESULT hr;
		if ( !m_CurrentChannel.empty() )
		{
			hr = m_pGunChat->LeaveRoom( m_CurrentRoomId, MM_LEAVE_JOIN_CHANNEL );
		}
		else
		{
			hr = m_pGunChat->JoinRoom( m_RequestedChannel, NULL, NULL );
		}
		gpassertf( !FAILED( hr ), ( "GUN : JoinRoom failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	}
	else if ( message.same_no_case( "zonematch_create_chatroom" ) )
	{
		gUIShell.ShowInterface( "zonematch_create_chatroom" );
		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "create_chatroom_name_edit_box" );
		if ( editBox )
		{
			editBox->SetText( L"" );
			editBox->SetAllowInput( true );
		}
		editBox = (UIEditBox *)gUIShell.FindUIWindow( "create_chatroom_userlimit_edit_box" );
		if ( editBox )
		{
			editBox->SetText( L"200" );
			editBox->SetAllowInput( false );
		}
		editBox = (UIEditBox *)gUIShell.FindUIWindow( "create_chatroom_topic_edit_box" );
		if ( editBox )
		{
			editBox->SetText( L"" );
			editBox->SetAllowInput( false );
		}
		editBox = (UIEditBox *)gUIShell.FindUIWindow( "create_chatroom_password_edit_box" );
		if ( editBox )
		{
			editBox->SetEnabled( false );
			editBox->SetText( L"" );
			editBox->SetAllowInput( false );
		}

		UICheckbox *checkBox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_hidden_room" );
		if ( checkBox )
		{
			checkBox->SetState( false );
		}
		checkBox = (UICheckbox *)gUIShell.FindUIWindow( "create_chatroom_checkbox_password" );
		if ( checkBox )
		{
			checkBox->SetState( false );
		}
	}
	else if ( message.same_no_case( "create_chatroom_check_password" ) )
	{
		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "create_chatroom_password_edit_box" );
		if ( editBox )
		{
			editBox->SetEnabled( true );
			editBox->SetText( L"" );
			editBox->SetAllowInput( true );
			editBox = (UIEditBox *)gUIShell.FindUIWindow( "create_chatroom_name_edit_box" );
			if ( editBox )
			{
				editBox->SetAllowInput( false );
			}
			editBox = (UIEditBox *)gUIShell.FindUIWindow( "create_chatroom_userlimit_edit_box" );
			if ( editBox )
			{
				editBox->SetAllowInput( false );
			}
			editBox = (UIEditBox *)gUIShell.FindUIWindow( "create_chatroom_topic_edit_box" );
			if ( editBox )
			{
				editBox->SetAllowInput( false );
			}
		}

		UICheckbox *checkBox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_hidden_room" );
		if ( checkBox )
		{
			checkBox->SetState( false );
		}
	}
	else if ( message.same_no_case( "create_chatroom_uncheck_password" ) )
	{
		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "create_chatroom_password_edit_box" );
		if ( editBox )
		{
			editBox->SetEnabled( false );
			editBox->SetText( L"" );
			editBox->SetAllowInput( false );
		}
	}
	else if ( message.same_no_case( "create_chatroom_check_hidden_room" ) )
	{
		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "create_chatroom_password_edit_box" );
		if ( editBox )
		{
			editBox->SetEnabled( false );
			editBox->SetText( L"" );
			editBox->SetAllowInput( false );
		}

		UICheckbox *checkBox = (UICheckbox *)gUIShell.FindUIWindow( "create_chatroom_checkbox_password" );
		if ( checkBox )
		{
			checkBox->SetState( false );
		}
	}
	else if ( message.same_no_case( "toggle_check_password_room" ) )
	{
		UICheckbox *checkBox = (UICheckbox *)gUIShell.FindUIWindow( "create_chatroom_checkbox_password" );
		if ( checkBox )
		{
			checkBox->SetState( !checkBox->GetState() );
		}
	}
	else if ( message.same_no_case( "toggle_check_hidden_room" ) )
	{
		UICheckbox *checkBox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_hidden_room" );
		if ( checkBox )
		{
			checkBox->SetState( !checkBox->GetState() );
		}
	}
	else if ( message.same_no_case( "zonematch_create_chatroom_ok" ) )
	{
		gpwstring chatRoom;
		gpwstring topic;
		gpwstring password;
		DWORD roomMode = IGunChat::USER;
		DWORD userLimit = 200;

		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "create_chatroom_name_edit_box" );
		if ( editBox )
		{
			chatRoom = editBox->GetText();
			if( !gUIFrontend.IsNameValid( chatRoom ) )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Chatroom name is not valid. Please try another name." ) );
				return;
			}
		}
		if ( chatRoom.empty() )
		{
			gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "You must enter a name for your chatroom." ) );
			return;
		}

		editBox = (UIEditBox *)gUIShell.FindUIWindow( "create_chatroom_userlimit_edit_box" );
		if ( editBox )
		{
			userLimit = wcstoul( editBox->GetText().c_str(), NULL, 10 );
			if ( (userLimit < 1) || (userLimit > 200) )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "The user limit must be a value between 1-200." ) );
				return;
			}
		}
		editBox = (UIEditBox *)gUIShell.FindUIWindow( "create_chatroom_topic_edit_box" );
		if ( editBox )
		{
			topic = editBox->GetText();
		}
		editBox = (UIEditBox *)gUIShell.FindUIWindow( "create_chatroom_password_edit_box" );
		if ( editBox )
		{
			password = editBox->GetText();
			stringtool::RemoveBorderingWhiteSpace( password );
		}

		UICheckbox *checkBox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_hidden_room" );
		if ( checkBox && checkBox->GetState() )
		{
			roomMode |= (DWORD)IGunChat::SECRET;		
		}
		checkBox = (UICheckbox *)gUIShell.FindUIWindow( "create_chatroom_checkbox_password" );
		if ( checkBox && checkBox->GetState() )
		{
			if ( password.empty() )
			{
				gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Enter a password." ) );
				return;
			}
			roomMode |= (DWORD)IGunChat::PASSWORD;
		}

		if ( !m_CurrentChannel.empty() )
		{
			HRESULT hr = m_pGunChat->LeaveRoom( m_CurrentRoomId, MM_LEAVE_HOST_CHANNEL );
			gpassertf( !FAILED( hr ), ( "GUN : LeaveRoom failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
			return;
		}

		m_RequestedChannel = chatRoom;
		m_RequestedChannelPassword = password;
		m_RequestedTopic = topic;
		m_RequestedChatMode = roomMode;
		m_RequestedUserLimit = userLimit;

		HRESULT hr = m_pGunChat->CreateRoom( chatRoom.c_str(), topic.c_str(), roomMode, userLimit, (roomMode & IGunChat::PASSWORD) ? password.c_str() : NULL, NULL );
		gpassertf( !FAILED( hr ), ( "GUN : CreateRoom failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	}
	else if ( message.same_no_case( "refresh_chatroom_list" ) )
	{
		HRESULT hr = m_pGunChat->GetListOfRooms( NULL );
		gpassertf( !FAILED( hr ), ( "GUN : GetListOfRooms failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	}
	else if ( message.same_no_case( "zonematch_chat_change_topic" ) )
	{
		if ( m_bHostOfChannel )
		{
			ShowInputDialog( DIALOG_CHAT_CHANGE_TOPIC, gpwtranslate( $MSG$ "Change Topic" ), gpwtranslate( $MSG$ "Enter a new topic for the chat room." ) );
		}
	}
	else if ( message.same_no_case( "zonematch_chat_send" ) )
	{
		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "match_chat_edit_box" );
		if ( editBox )
		{
			gpwstring chatText = editBox->GetText();
			stringtool::RemoveBorderingWhiteSpace( chatText );
			if ( !chatText.empty() )
			{
				HRESULT hr = m_pGunChat->SendToRoom( m_CurrentRoomId, chatText.c_str(), NULL );
				gpassertf( !FAILED( hr ), ( "GUN : SendToRoom failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
			}
			editBox->SetText( L"" );
		}
	}
	else if ( message.same_no_case( "start_chat_resize" ) )
	{
		m_bSizingChatArea = true;

		UIWindow * pWindow = gUIShell.FindUIWindow( "chat_window_visible_slider" );
		if ( pWindow )
		{
			pWindow->SetVisible( true );
		}
	}
	else if ( message.same_no_case( "stop_chat_resize" ) )
	{
		if ( m_bSizingChatArea )
		{
			m_bSizingChatArea = false;

			UIWindow * pWindow = gUIShell.FindUIWindow( "chat_window_visible_slider" );
			if ( pWindow )
			{
				pWindow->SetVisible( false );
			}
		}
	}
	else if ( message.same_no_case( "zonematch_chat_min" ) )
	{
		m_ChatAreaSize = CHAT_AREA_SMALL;
		ResizeChatArea( CHAT_AREA_SMALL );
	}
	else if ( message.same_no_case( "zonematch_chat_mid" ) )
	{
		m_ChatAreaSize = CHAT_AREA_MEDIUM;
		ResizeChatArea( CHAT_AREA_MEDIUM );
	}
	else if ( message.same_no_case( "zonematch_chat_max" ) )
	{
		m_ChatAreaSize = CHAT_AREA_LARGE;
		ResizeChatArea( CHAT_AREA_LARGE );
	}
	else if ( message.same_no_case( "zonematch_chat_leave" ) )
	{
		HRESULT hr = m_pGunChat->LeaveRoom( m_CurrentRoomId, NULL );
		gpassertf( !FAILED( hr ), ( "GUN : LeaveRoom failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	}
	else if ( message.same_no_case( "show_chat_popup_menu" ) )
	{
		if ( m_pChatMenu )
		{
			m_pChatMenu->RemoveAllElements();

			MENU_COMMAND( m_pChatMenu, MENU_CHAT_PRIVATE_CHAT );

			UIListbox *chatListBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
			if ( !chatListBox )
			{
				return;
			}

			if ( chatListBox->GetSelectedText().same_no_case( m_Username ) )
			{
				return;
			}

			bool bIsFriend = false;
			gpwstring friendUserName = chatListBox->GetSelectedText();
			UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_friends_list" );
			if ( listBox )
			{
				for ( unsigned int i = 0; i < listBox->GetNumElements(); ++i )
				{
					if ( listBox->GetElement( i ) && listBox->GetElement( i )->lines.begin()->sText.same_no_case( friendUserName ) )
					{
						bIsFriend = true;
						break;
					}
				}
			}
			//must not be a freind, and not be ignored to add to friends list
			if ( !bIsFriend && ! IsUserFriend(friendUserName) && !IsUserIgnored(friendUserName) )
			{
				MENU_COMMAND( m_pChatMenu, MENU_CHAT_ADD_TO_FRIENDS );
			}

			MENU_COMMAND( m_pChatMenu, MENU_SEPERATOR );

			if ( IsUserIgnored( chatListBox->GetSelectedText() ) )
			{
				MENU_COMMAND( m_pChatMenu, MENU_CHAT_UNIGNORE_USER );
			}
			else
			{
				MENU_COMMAND( m_pChatMenu, MENU_CHAT_IGNORE_USER );
			}

			// taking this out for now.
			// gun's implementation of squelching is not independant of ignore
			// so we should just ignore them if we don't want to hear them.
			//bool bSquelched = IsUserSquelched( chatListBox->GetSelectedText() );

			//if ( bSquelched )
			//{
			//	MENU_COMMAND( m_pChatMenu, MENU_CHAT_UNSQUELCH_USER );
			//}
			//else
			//{
			//	MENU_COMMAND( m_pChatMenu, MENU_CHAT_SQUELCH_USER );
			//}

			if ( m_bHostOfChannel )
			{
				MENU_COMMAND( m_pChatMenu, MENU_SEPERATOR );
				MENU_COMMAND( m_pChatMenu, MENU_CHAT_KICK_USER );
			}

			m_pChatMenu->ShowAtCursor();
		}
	}
	else if ( message.same_no_case( "chat_popup_menu_select" ) )
	{
		gpwstring userName;
		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
		if ( listBox )
		{
			userName = listBox->GetSelectedText();
		}
		if ( userName.empty() )
		{
			return;
		}

		switch ( m_pChatMenu->GetSelectedElement() )
		{
			case MENU_CHAT_PRIVATE_CHAT:
			{
				// update friends list
				SetNewMessageState( userName, false );
				UpdateFriendsNewMessages();

				ShowPrivateChatWindow( userName );

				break;
			}

			case MENU_CHAT_ADD_TO_FRIENDS:
			{
				AddFriendToZoneMatch( userName );
				break;
			}

			case MENU_CHAT_KICK_USER:
			{
				if ( userName.find( L"@" ) != gpwstring::npos )
				{
					gUIMultiplayer.ShowMessageBox( gpwstringf( gpwtranslate( $MSG$ "Do you really want to kick the all-seeing %s?" ), userName.c_str() ), MP_DIALOG_OK, gpwtranslate( $MSG$ "No" ) );
					return;
				}

				gpwstring reason;
				HRESULT hr = m_pGunChat->KickOutUser( m_CurrentRoomId, userName.c_str(), reason.c_str(), NULL );
				gpassertf( !FAILED( hr ), ( "GUN : KickOutUser failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
				break;
			}

			case MENU_CHAT_IGNORE_USER:
			{
				IgnoreUser( userName );
				break;
			}

			case MENU_CHAT_UNIGNORE_USER:
			{
				UnignoreUser( userName );
				break;
			}

			case MENU_CHAT_SQUELCH_USER:
			{
				SquelchUser( userName );
				break;
			}

			case MENU_CHAT_UNSQUELCH_USER:
			{
				UnSquelchUser( userName );
				break;
			}
		}
	}
	else if ( message.same_no_case( "zonematch_chat_private_chat" ) )
	{
		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
		if ( listBox && !listBox->GetSelectedText().empty() )
		{
			if ( !listBox->GetSelectedText().same_no_case( m_Username ) )
			{
				// update friends list
				SetNewMessageState( listBox->GetSelectedText(), false );
				UpdateFriendsNewMessages();

				ShowPrivateChatWindow( listBox->GetSelectedText() );
			}
		}
	}
	else if ( message.same_no_case( "zonematch_send_private_message" ) )
	{
		if ( !m_pActiveChatLog || m_ActiveChatUser.empty() )
		{
			return;
		}

		gpwstring messageText;

		UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "private_chat_edit_box" );
		if ( pEditBox )
		{
			messageText = pEditBox->GetText();
			pEditBox->SetText( L"" );
		}

		if ( messageText.empty() )
		{
			return;
		}

		m_pActiveChatLog->push_back( UserChatText( MSG_TO, messageText ) );

		UIChatBox * pChatBox = (UIChatBox *)gUIShell.FindUIWindow( "private_chat_box", "zonematch_private_chat" );
		if ( pChatBox )
		{
			gpwstring text;
			if( ! gUIEmotes.PrepareStringTextEmotes( messageText, text, m_Username.c_str()/*playerName*/,  m_ActiveChatUser.c_str()/*targetName*/, UI_TEXT_EMOTE_TYPE_INDIVIDUAL ) )
			{
				text.assignf( gpwtranslate( $MSG$ "%s: %s" ), m_Username.c_str(), messageText.c_str() );
			}
			pChatBox->SubmitText( text, 0xffffffff );
		}

		HRESULT hr = m_pGunChat->SendToUser( m_ActiveChatUser.c_str(), messageText.c_str(), NULL );
		gpassertf( !FAILED( hr ), ( "GUN : SendToUser failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	}
	else if ( message.same_no_case( "zonematch_close_private_chat" ) )
	{
		HidePrivateChatWindow();
	}

	///////////////////////////////////////////////////////////////
	// Friends

	else if ( message.same_no_case( "show_friends_popup_menu" ) )
	{
		bool userActive = true;
		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_friends_list" );
		if ( listBox && listBox->GetSelectedElement() )
		{
			if ( listBox->GetSelectedElement()->icon == ICON_USER_OFFLINE )
			{
				userActive = false;
			}
		}

		if ( m_pFriendsMenu )
		{
			m_pFriendsMenu->RemoveAllElements();

			if ( userActive )
			{
				MENU_COMMAND( m_pFriendsMenu, MENU_FRIENDS_PRIVATE_CHAT );

				if ( !m_CurrentChannel.empty() && !IsUserInChat( listBox->GetSelectedText() ) )
				{
					MENU_COMMAND( m_pFriendsMenu, MENU_FRIENDS_INVITE_CHAT );
				}

				MENU_COMMAND( m_pFriendsMenu, MENU_FRIENDS_WHERE_IS );
				MENU_COMMAND( m_pFriendsMenu, MENU_SEPERATOR );
			}

			MENU_COMMAND( m_pFriendsMenu, MENU_FRIENDS_IGNORE );

			if ( listBox->GetSelectedTag() < (int)m_Friends.size() )
			{
				MENU_COMMAND( m_pFriendsMenu, MENU_FRIENDS_REMOVE );
			}
			else
			{
				MENU_COMMAND( m_pFriendsMenu, MENU_FRIENDS_ADD );
				MENU_COMMAND( m_pFriendsMenu, MENU_FRIENDS_DELETE_MSG );
			}

			m_pFriendsMenu->ShowAtCursor();
		}
	}
	else if ( message.same_no_case( "friends_popup_menu_select" ) )
	{
		gpwstring userName;
		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_friends_list" );
		if ( !listBox || listBox->GetSelectedText().empty() )
		{
			return;
		}

		userName = listBox->GetSelectedText();

		switch ( m_pFriendsMenu->GetSelectedElement() )
		{
			case MENU_FRIENDS_PRIVATE_CHAT:
			{
				// update friends list
				SetNewMessageState( userName, false );
				UpdateFriendsNewMessages();

				ShowPrivateChatWindow( userName );

				break;
			}

			case MENU_FRIENDS_INVITE_CHAT:
			{
				if ( !m_CurrentChannel.empty() )
				{
					gpwstring inviteLine;
					inviteLine.assignf( L"#CHATINVITE=%s", m_CurrentChannel.c_str() );
					if ( !m_CurrentChannelPassword.empty() )
					{
						inviteLine.appendf( L"#PASSWORD=%s", m_CurrentChannelPassword.c_str() );
					}
					HRESULT hr = m_pGunChat->SendToUser( userName.c_str(), inviteLine.c_str(), NULL );
					gpassertf( !FAILED( hr ), ( "GUN : SendToUser failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
				}
				break;
			}

			case MENU_FRIENDS_WHERE_IS:
			{
				HRESULT hr = m_pGunFriends->WhereIs( userName.c_str(), MM_REQUEST_WHERE_IS );
				gpassertf( !FAILED( hr ), ( "GUN : WhereIs failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
				break;
			}

			case MENU_FRIENDS_IGNORE:
			{
				if ( listBox->GetSelectedTag() < (int)m_Friends.size() )
				{
					ShowDialog( DIALOG_IGNORE_FRIEND, gpwstringf( gpwtranslate( $MSG$ "This will remove %s from your friends list. You can edit your ignored list from the Options menu. Proceed?" ), userName.c_str() ) );
				}
				m_RequestIgnoreUser = userName;
				break;
			}

			case MENU_FRIENDS_REMOVE:
			{
				listBox->RemoveElement( userName );
				RemoveFriend( userName );

				break;
			}

			case MENU_FRIENDS_ADD:
			{
				AddFriendToZoneMatch( userName );
				break;
			}

			case MENU_FRIENDS_DELETE_MSG:
			{
				// update friends list
				SetNewMessageState( userName, false );
				RefreshFriendsList();
				break;
			}
		}
	}
	else if ( message.same_no_case( "show_messages_popup_menu" ) )
	{
		if ( m_pMessagingMenu )
		{
			m_pMessagingMenu->RemoveAllElements();

			MENU_COMMAND( m_pMessagingMenu, MENU_MSGING_VIEW );
			MENU_COMMAND( m_pMessagingMenu, MENU_MSGING_DELETE );

			UIListbox *messagesListBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_messages_list" );
			if ( !messagesListBox )
			{
				return;
			}

			bool bIsFriend = false;
			gpwstring friendUserName = messagesListBox->GetSelectedText();
			UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_friends_list" );
			if ( listBox )
			{
				for ( unsigned int i = 0; i < listBox->GetNumElements(); ++i )
				{
					if ( listBox->GetElement( i ) && listBox->GetElement( i )->lines.begin()->sText.same_no_case( friendUserName ) )
					{
						bIsFriend = true;
						break;
					}
				}
			}

			MENU_COMMAND( m_pMessagingMenu, MENU_SEPERATOR );
			MENU_COMMAND( m_pMessagingMenu, MENU_MSGING_PRIVATE_CHAT );

			if ( !m_CurrentChannel.empty() && !IsUserInChat( friendUserName ) )
			{
				MENU_COMMAND( m_pMessagingMenu, MENU_MSGING_INVITE_CHAT );
			}

			MENU_COMMAND( m_pMessagingMenu, MENU_SEPERATOR );

			if ( !bIsFriend && ! IsUserFriend(friendUserName) && !IsUserIgnored(friendUserName) )
			{	
				MENU_COMMAND( m_pMessagingMenu, MENU_MSGING_ADD_TO_FRIENDS );
			}

			if( IsUserIgnored( messagesListBox->GetSelectedText() ))
			{
				MENU_COMMAND( m_pMessagingMenu, MENU_MSGING_UNIGNORE_USER );
			}
			else
			{
				MENU_COMMAND( m_pMessagingMenu, MENU_MSGING_IGNORE_USER );
			}
			
			

			int id = messagesListBox->GetSelectedTag();
			for ( UserMessageColl::iterator i = m_Messages.begin(); i != m_Messages.end(); ++i )
			{
				if ( i->Id == id )
				{
					if ( i->Type == USER_REQUEST_ADD )
					{
						MENU_COMMAND( m_pMessagingMenu, MENU_SEPERATOR );
						MENU_COMMAND( m_pMessagingMenu, MENU_MSGING_ACCEPT_REQUEST );
						MENU_COMMAND( m_pMessagingMenu, MENU_MSGING_DENY_REQUEST );
						break;
					}
				}
			}

			m_pMessagingMenu->ShowAtCursor();
		}
	}
	else if ( message.same_no_case( "messaging_popup_menu_select" ) )
	{
		gpwstring userName;
		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_messages_list" );
		if ( !listBox )
		{
			return;
		}
		userName = listBox->GetSelectedText();

		switch ( m_pMessagingMenu->GetSelectedElement() )
		{
			case MENU_MSGING_VIEW:
			{
				int id = listBox->GetSelectedTag();
				for ( UserMessageColl::iterator i = m_Messages.begin(); i != m_Messages.end(); ++i )
				{
					if ( i->Id == id )
					{
						if ( i->Type == USER_MESSAGE )
						{
							ShowPrivateChatWindow( userName );
						}
						else if ( i->Type == USER_REQUEST_ADD )
						{
							ShowDialog( DIALOG_ACCEPT_ADD_FRIEND, gpwstring().appendf( gpwtranslate( $MSG$ "Do you want to allow %s to add you to his/her Friends List?" ), i->From.c_str() ) );

							m_AcceptAddUser = i->From;

							m_Messages.erase( i );
							CreateMessageList();
						}
						else if ( i->Type == USER_CHAT_INVITE )
						{
							unsigned int password = i->Text.find( L"#PASSWORD=" );
							if ( password != gpwstring::npos )
							{
								m_RequestedChannel = i->Text.left( password );
								m_RequestedChannelPassword = i->Text.mid( password + 10, i->Text.size() - password - 10 );
							}
							else
							{
								m_RequestedChannel = i->Text;
								m_RequestedChannelPassword.clear();
							}

							ShowDialog( DIALOG_INVITE_CHAT, gpwstring().appendf( gpwtranslate( $MSG$ "%s has invited you into chatroom \"%s\". Do you want to join?" ), i->From.c_str(), i->Text.c_str() ) );

							m_Messages.erase( i );
							CreateMessageList();
						}
						break;
					}
				}
				break;
			}

			case MENU_MSGING_PRIVATE_CHAT:
			{
				ShowPrivateChatWindow( userName );
				break;
			}

			case MENU_MSGING_INVITE_CHAT:
			{
				if ( !m_CurrentChannel.empty() )
				{
					gpwstring inviteLine;
					inviteLine.assignf( L"#CHATINVITE=%s", m_CurrentChannel.c_str() );
					if ( !m_CurrentChannelPassword.empty() )
					{
						inviteLine.appendf( L"#PASSWORD=%s", m_CurrentChannelPassword.c_str() );
					}
					HRESULT hr = m_pGunChat->SendToUser( userName.c_str(), inviteLine.c_str(), NULL );
					gpassertf( !FAILED( hr ), ( "GUN : SendToUser failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
				}
				break;
			}
			
			case MENU_MSGING_DELETE:
			{
				int id = listBox->GetSelectedTag();
				for ( UserMessageColl::iterator i = m_Messages.begin(); i != m_Messages.end(); ++i )
				{
					if ( i->Id == id )
					{
						// we are rejecting this person!  tell them to bugger off.
						if( i->Type == USER_REQUEST_ADD )
						{
							HRESULT hr = m_pGunFriends->AcceptAddFriend( false, i->From.c_str(), NULL );
							gpassertf( !FAILED( hr ), ( "GUN : AcceptAddFriend failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
						}
						m_Messages.erase( i );
						CreateMessageList();
						break;
					}
				}
				break;
			}

			case MENU_MSGING_ADD_TO_FRIENDS:
			{

				AddFriendToZoneMatch( userName );
				break;
			}

			case MENU_MSGING_IGNORE_USER:
			{
				IgnoreUser( userName );
				break;
			}

			case MENU_MSGING_UNIGNORE_USER:
			{
				UnignoreUser( userName );
				break;
			}

			case MENU_MSGING_ACCEPT_REQUEST:
			{
				int id = listBox->GetSelectedTag();
				for ( UserMessageColl::iterator i = m_Messages.begin(); i != m_Messages.end(); ++i )
				{
					if ( i->Id == id )
					{
						HRESULT hr = m_pGunFriends->AcceptAddFriend( true, i->From.c_str(), NULL );
						gpassertf( !FAILED( hr ), ( "GUN : AcceptAddFriend failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

						m_Messages.erase( i );
						CreateMessageList();
						break;
					}
				}
				break;
			}

			case MENU_MSGING_DENY_REQUEST:
			{
				int id = listBox->GetSelectedTag();
				for ( UserMessageColl::iterator i = m_Messages.begin(); i != m_Messages.end(); ++i )
				{
					if ( i->Id == id )
					{
						HRESULT hr = m_pGunFriends->AcceptAddFriend( false, i->From.c_str(), NULL );
						gpassertf( !FAILED( hr ), ( "GUN : AcceptAddFriend failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

						m_Messages.erase( i );
						CreateMessageList();
						break;
					}
				}
				break;
			}
		}
	}
	else if ( message.same_no_case( "zonematch_friends_private_chat" ) )
	{
		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_friends_list" );
		if ( listBox && !listBox->GetSelectedText().empty() )
		{
			gpwstring userName = listBox->GetSelectedText();
			bool userActive = (listBox->GetSelectedElement()->icon != ICON_USER_OFFLINE);

			bool foundActiveMessage = false;

			for ( std::vector< gpwstring >::iterator i = m_NewMessages.begin(); i != m_NewMessages.end(); ++i )
			{
				if ( userName.same_no_case( *i ) )
				{
					foundActiveMessage = true;
					break;
				}
			}

			if ( !userActive && !foundActiveMessage )
			{
				return;
			}

			// update friends list
			SetNewMessageState( userName, false );
			UpdateFriendsNewMessages();

			ShowPrivateChatWindow( userName, userActive );	
		}
	}
	else if ( message.same_no_case( "zonematch_friends_send_message" ) )
	{
		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_friends_list" );
		if ( listBox && !listBox->GetSelectedText().empty() )
		{
			if ( listBox->GetSelectedElement()->icon == ICON_USER_OFFLINE )
			{
				return;
			}

			gUIShell.ShowInterface( "zonematch_send_message" );

			UITextBox *pTextBox = (UITextBox *)gUIShell.FindUIWindow( "zonematch_send_message_text" );
			if ( pTextBox )
			{
				pTextBox->SetText( gpwstringf( gpwtranslate( $MSG$ "Send Message To %s" ), listBox->GetSelectedText().c_str() ) );
			}
			UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "send_message_edit_box" );
			if ( editBox )
			{
				editBox->SetText( L"" );
				editBox->SetAllowInput( true );
			}

			m_SendUserMessage = listBox->GetSelectedText();
		}
	}
	else if ( message.same_no_case( "show_selected_message" ) ||
			  message.same_no_case( "show_selected_message_enter" ) )
	{
		if ( message.same_no_case( "show_selected_message_enter" ) )
		{
			UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "match_chat_edit_box", "zonematch" );
			if ( pEditBox && pEditBox->GetVisible() )
			{
				return;
			}
		}

		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_messages_list" );
		if ( listBox )
		{
			int id = listBox->GetSelectedTag();

			listBox->FlashElementIcon( id, 0.0f );

			for ( UserMessageColl::iterator i = m_Messages.begin(); i != m_Messages.end(); ++i )
			{
				if ( i->Id == id )
				{
					if ( i->Type == USER_MESSAGE )
					{	
						ShowPrivateChatWindow( i->From );
					}
					else if ( i->Type == USER_REQUEST_ADD )
					{
						ShowDialog( DIALOG_ACCEPT_ADD_FRIEND, gpwstring().appendf( gpwtranslate( $MSG$ "Do you want to allow %s to add you to his/her Friends List?" ), i->From.c_str() ) );

						m_AcceptAddUser = i->From;

						m_Messages.erase( i );
						CreateMessageList();
					}
					else if ( i->Type == USER_CHAT_INVITE )
					{
						unsigned int password = i->Text.find( L"#PASSWORD=" );
						if ( password != gpwstring::npos )
						{
							m_RequestedChannel = i->Text.left( password );
							m_RequestedChannelPassword = i->Text.mid( password + 10, i->Text.size() - password - 10 );
						}
						else
						{
							m_RequestedChannel = i->Text;
							m_RequestedChannelPassword.clear();
						}

						ShowDialog( DIALOG_INVITE_CHAT, gpwstring().appendf( gpwtranslate( $MSG$ "%s has invited you into chatroom \"%s\". Do you want to join?" ), i->From.c_str(), i->Text.c_str() ) );

						m_Messages.erase( i );
						CreateMessageList();
					}
					break;
				}
			}
		}
	}
	else if ( message.same_no_case( "zonematch_send_message_reply" ) )
	{
		gpwstring messageText;
		UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "reply_message_edit_box" );
		if ( editBox )
		{
			messageText = editBox->GetText();
		}

		if ( messageText.empty() || m_SendUserMessage.empty() )
		{
			return;
		}

		HRESULT hr = m_pGunChat->SendToUser( m_SendUserMessage.c_str(), messageText.c_str(), NULL );
		gpassertf( !FAILED( hr ), ( "GUN : SendToUser failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

		m_SendUserMessage.clear();

		gUIShell.HideInterface( "zonematch_view_message" );
	}
	else if ( message.same_no_case( "zonematch_delete_message" ) )
	{
		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_messages_list" );
		if ( listBox )
		{
			int id = listBox->GetSelectedTag();
			for ( UserMessageColl::iterator i = m_Messages.begin(); i != m_Messages.end(); ++i )
			{
				if ( i->Id == id )
				{
					m_Messages.erase( i );

					CreateMessageList();
					break;
				}
			}
		}

		gUIShell.HideInterface( "zonematch_view_message" );
	}
	else if ( message.same_no_case( "zonematch_add_friend" ) )
	{
		ShowInputDialog( DIALOG_ADD_FRIEND, gpwtranslate( $MSG$ "Add Friend" ), gpwtranslate( $MSG$ "Enter the name of the user" ) );
	}

	///////////////////////////////////////////////////////////////
	// Match Making

	else if ( message.same_no_case( "zonematch_show_games_column_popup" ) )
	{
		UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( "button_games_column_popup" );
		if ( pButton )
		{
			if ( m_pGamesFilterMenu )
			{
				/*
				static bool bWasGamesFilterVisible = false;
				if ( bWasGamesFilterVisible )
				{
					bWasGamesFilterVisible = false;
					m_pGamesFilterMenu->SetVisible( false );
					return;
				}
				*/

				m_pGamesFilterMenu->RemoveAllElements();

				MENU_COMMAND( m_pGamesFilterMenu, MENU_FILTER_GAMES_SEARCH );
				MENU_COMMAND( m_pGamesFilterMenu, MENU_SEPERATOR );
				MENU_COMMAND( m_pGamesFilterMenu, MENU_FILTER_GAMES_WITH_FRIENDS );
				MENU_COMMAND( m_pGamesFilterMenu, MENU_SEPERATOR );
				MENU_COMMAND( m_pGamesFilterMenu, MENU_FILTER_GAMES_ALL );

				m_pGamesFilterMenu->Show( pButton->GetRect().right, pButton->GetRect().bottom, UIPopupMenu::ALIGN_DOWN_LEFT );
			}
		}
	}
	else if ( message.same_no_case( "games_filter_popup_menu_select" ) )
	{
		switch ( m_pGamesFilterMenu->GetSelectedElement() )
		{
			case MENU_FILTER_GAMES_SEARCH:
			{
				ShowInputDialog( DIALOG_FIND_GAME_NAME, gpwtranslate( $MSG$ "GAMES LIST" ), gpwtranslate( $MSG$ "Enter a game name to search for" ) );
				break;
			}

			case MENU_FILTER_GAMES_WITH_FRIENDS:
			{
				break;
			}

			case MENU_FILTER_GAMES_ALL:
			{
				break;
			}
		}
	}
	else if ( message.same_no_case( "zonematch_games_refresh" ) )
	{
		if ( m_TimeToRefreshGamesList == 0.0f )
		{
			m_GameListSearchString.clear();
			UpdateGameColumns();

			CreateGamesList( true );
			m_TimeToRefreshGamesList = 5.0f;
		}
	}
	else if ( message.same_no_case( "start_slider_scroll_games_list" ) )
	{
		m_bScrollingGamesList = true;
		m_GamesScrollStartValue = ((UISlider *)&ui_window)->GetValue();
		m_HeldGamesScrollPos = m_GamesScrollStartValue;
		m_TimeHeldGamesScrollPos = 0;
	}
	else if ( message.same_no_case( "end_slider_scroll_games_list" ) )
	{
		if ( m_bScrollingGamesList )
		{
			m_bScrollingGamesList = false;
			if ( m_GamesScrollStartValue != ((UISlider *)&ui_window)->GetValue() )
			{
				CreateGamesList();
			}
		}
	}
	else if ( message.same_no_case( "slider_scroll_games_list" ) )
	{
		if ( !m_bScrollingGamesList )
		{
			CreateGamesList();
		}
	}
	else if ( message.same_no_case( "games_list_key_press" ) )
	{
		int keyPress = ((UIListReport *)&ui_window)->GetLastKeyPressed();

		if ( keyPress == Keys::KEY_INT_PAGEDOWN )
		{
			m_pGamesListSlider->SetValue( m_pGamesListSlider->GetValue() + 1 );
		}
		else if ( keyPress == Keys::KEY_INT_PAGEUP )
		{
			m_pGamesListSlider->SetValue( m_pGamesListSlider->GetValue() - 1 );
		}
	}
	else if ( message.same_no_case( "zonematch_host_game" ) )
	{
		// BUG 12205/6 we want to make sure players have their safe disks when we are going to start a game
		// this way we don't have people starting games that they won't be able to actually start....
		if ( !gUIFrontend.ShowCDCheckDialog() )
		{
			return;
		}

		if ( !m_bSignedIn && m_PlayerName.empty() )
		{
			ShowInputDialog( DIALOG_PLAYER_NAME_HOST_GAME, gpwtranslate( $MSG$ "HOST GAME" ), gpwtranslate( $MSG$ "Enter Player Name" ) );

			gpwstring playerName;
			FuelHandle prefs = gTattooGame.GetPrefsConfigFuel();
			if ( prefs.IsValid() )
			{
				FuelHandle hSettings = prefs->GetChildBlock( "multiplayer" );
				if ( hSettings.IsValid() )
				{
					hSettings->Get( "zone_player_name", playerName );
				}
			}

			UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "input_dialog_edit_box" );
			if ( editBox )
			{
				editBox->SetText( playerName );
			}

			return;
		}

		if ( m_bSignedIn )
		{
			m_PlayerName = m_Username;
		}

		gUIMultiplayer.ShowHostGameInterface();

		UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "zonematch_help", "zonematch" );
		if ( pTextBox )
		{
			pTextBox->SetText( L"" );
		}
	}
	else if ( message.same_no_case( "zonematch_join_game" ) )
	{
		if ( gUIMultiplayer.GetNextUpdate() == UIMultiplayer::NU_JOIN_GAME )
		{
			return;
		}

		m_bHostingGame = false;

		static bool bInDetailsView = gUIShell.IsInterfaceVisible( "zonematch_game_details" );

		GameInfoMap::iterator findGame = m_GamesList.end();

		if ( bInDetailsView )
		{
			findGame = m_GamesList.find( m_CurrentGameRowId );
		}
		else
		{
			UIListReport *listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_games_list" );
			if ( listReport )
			{
				findGame = m_GamesList.find( listReport->GetSelectedElementTag() );
			}
		}
		if ( (findGame == m_GamesList.end()) || findGame->second.ipAddress.empty() )
		{
			return;
		}

		if ( !findGame->second.Version.same_no_case( SysInfo::GetExeFileVersionText( gpversion::MODE_AUTO_LONG ) ) )
		{
			gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Unable to join. The selected game is an incompatible version." ) );
			return;
		}
		
		// BUG 12205/6  we want to make sure that the player is able to join and play the game before allowing them
		// to pause the game for everyone.  This way if someone has to search the house for a cd, they aren't making everyone wait! 
		if ( !gUIFrontend.ShowCDCheckDialog() )
		{
			return;
		}
		
		if ( !m_bSignedIn && m_PlayerName.empty() )
		{
			ShowInputDialog( DIALOG_PLAYER_NAME_JOIN_GAME, gpwtranslate( $MSG$ "JOIN GAME" ), gpwtranslate( $MSG$ "Enter Player Name" ) );

			gpwstring playerName;
			FuelHandle prefs = gTattooGame.GetPrefsConfigFuel();
			if ( prefs.IsValid() )
			{
				FuelHandle hSettings = prefs->GetChildBlock( "multiplayer" );
				if ( hSettings.IsValid() )
				{
					hSettings->Get( "zone_player_name", playerName );
				}
			}

			UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "input_dialog_edit_box" );
			if ( editBox )
			{
				editBox->SetText( playerName );
			}

			return;
		}

		if ( m_bSignedIn )
		{
			m_PlayerName = m_Username;
		}

		if ( bInDetailsView )
		{
			HRESULT hr = m_pGunBrowser->GetRowPage( GAMEVIEW_GAME_DETAILS, m_CurrentGameRowId, 0, NULL, MM_REFRESH_GAME_AND_JOIN );
			gpassertf( !FAILED( hr ), ( "GUN : GetRowPage failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
		}
		else
		{
			if ( !gUIMultiplayer.JoinGame( ToUnicode( findGame->second.ipAddress ) ) )
			{
				// attempt join on local host ip
				gUIMultiplayer.HideMessageBox();
				if( !gUIMultiplayer.JoinGame( ToUnicode( findGame->second.ipAddress2 ) ) )
				{
					gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Could not find the game. Please refresh and try again." ), MP_DIALOG_OK );
				}
			}
		}
	}
	else if ( message.same_no_case( "zonematch_sort_games" ) )
	{
		eGameListSort sort = (eGameListSort)((UIListReport *)(&ui_window))->GetSortColumn();

		if ( m_GameListSort != sort )
		{
			m_GameListSort = sort;

			if ( m_GameListSort != m_GameListSearch )
			{
				m_GameListSearchString.clear();
			}

			UpdateGameColumns();

			m_bRefreshGamesList = true;

			UIListReport *listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_games_list" );
			if ( listReport )
			{
				if ( listReport->GetSelectedElementID() != UIListReport::INVALID_ROW )
				{
					m_SelectedGameName = listReport->GetSelectedElementText();
				}

				listReport->RemoveAllElements();
			}

			UIWindow * pText = gUIShell.FindUIWindow( "refreshing_games_text", "zonematch_games" );
			if ( pText )
			{
				pText->SetVisible( true );
			}
		}
	}
	else if ( message.same_no_case( "zonematch_show_game_details" ) )
	{
		UIListReport *listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_games_list" );
		if ( !listReport || (listReport->GetSelectedElementTag() == UIListReport::TAG_NONE) )
		{
			return;
		}

		m_CurrentGameRowId = listReport->GetSelectedElementTag();

		HRESULT hr = m_pGunBrowser->GetRowPage( GAMEVIEW_GAME_DETAILS, m_CurrentGameRowId, 0, NULL, MM_REFRESH_GAME_DETAILS );
		gpassertf( !FAILED( hr ), ( "GUN : GetRowPage failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	}
	else if ( message.same_no_case( "zonematch_game_details_refresh" ) )
	{
		if ( m_TimeToRefreshGamesList == 0.0f )
		{
			HRESULT hr = m_pGunBrowser->GetRowPage( GAMEVIEW_GAME_DETAILS, m_CurrentGameRowId, 0, NULL, MM_REFRESH_GAME_DETAILS );
			gpassertf( !FAILED( hr ), ( "GUN : GetRowPage failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

			m_TimeToRefreshGamesList = 5.0f;
		}
	}
	else if ( message.same_no_case( "zonematch_show_all_games" ) )
	{
		if ( m_GameViewType != SHOW_ALL_GAMES )
		{
			m_GameViewType = SHOW_ALL_GAMES;
			SaveZoneSettings();

			m_bRefreshGamesList = true;

			if ( m_pSelectedGamesTab )
			{
				m_pSelectedGamesTab->SetPressedState( false );
			}
			m_pSelectedGamesTab = (UIButton *)&ui_window;

			if_gpassert( m_pSelectedGamesTab )
			{
				m_pSelectedGamesTab->SetPressedState( true );
			}

			UIText * pText = (UIText *)gUIShell.FindUIWindow( "zonematch_button_show_all_games_text" );
			if ( pText )
			{
				pText->SetColor( 0xff95fffe );
			}
			pText = (UIText *)gUIShell.FindUIWindow( "zonematch_button_show_friends_games_text" );
			if ( pText )
			{
				pText->SetColor( 0xffffffff );
			}
			pText = (UIText *)gUIShell.FindUIWindow( "zonematch_button_show_filtered_games_text" );
			if ( pText )
			{
				pText->SetColor( 0xffffffff );
			}

			UpdateGameColumns();
		}
	}
	else if ( message.same_no_case( "zonematch_show_friends_games" ) )
	{
		if ( m_GameViewType != SHOW_FRIENDS_GAMES )
		{
			m_GameViewType = SHOW_FRIENDS_GAMES;
			SaveZoneSettings();

			m_bRefreshGamesList = true;

			if ( m_pSelectedGamesTab )
			{
				m_pSelectedGamesTab->SetPressedState( false );
			}
			m_pSelectedGamesTab = (UIButton *)&ui_window;

			if_gpassert( m_pSelectedGamesTab )
			{
				m_pSelectedGamesTab->SetPressedState( true );
			}

			UIText * pText = (UIText *)gUIShell.FindUIWindow( "zonematch_button_show_all_games_text" );
			if ( pText )
			{
				pText->SetColor( 0xffffffff );
			}
			pText = (UIText *)gUIShell.FindUIWindow( "zonematch_button_show_friends_games_text" );
			if ( pText )
			{
				pText->SetColor( 0xff95fffe );
			}
			pText = (UIText *)gUIShell.FindUIWindow( "zonematch_button_show_filtered_games_text" );
			if ( pText )
			{
				pText->SetColor( 0xffffffff );
			}

			UpdateGameColumns();
		}
	}
	else if ( message.same_no_case( "zonematch_show_filtered_games" ) )
	{
		if ( m_GameViewType != SHOW_FILTERED_GAMES )
		{
			m_GameViewType = SHOW_FILTERED_GAMES;
			SaveZoneSettings();

			m_bRefreshGamesList = true;

			if ( m_pSelectedGamesTab )
			{
				m_pSelectedGamesTab->SetPressedState( false );
			}
			m_pSelectedGamesTab = (UIButton *)&ui_window;

			if_gpassert( m_pSelectedGamesTab )
			{
				m_pSelectedGamesTab->SetPressedState( true );
			}

			UIText * pText = (UIText *)gUIShell.FindUIWindow( "zonematch_button_show_all_games_text" );
			if ( pText )
			{
				pText->SetColor( 0xffffffff );
			}
			pText = (UIText *)gUIShell.FindUIWindow( "zonematch_button_show_friends_games_text" );
			if ( pText )
			{
				pText->SetColor( 0xffffffff );
			}
			pText = (UIText *)gUIShell.FindUIWindow( "zonematch_button_show_filtered_games_text" );
			if ( pText )
			{
				pText->SetColor( 0xff95fffe );
			}

			UpdateGameColumns();
		}

		// Show filters interface
		ShowFiltersInterface();
	}
	else if ( message.same_no_case( "zonematch_games_apply_filters" ) )
	{
		HideFiltersInterface();
	}
	else if ( message.same_no_case( "zonematch_show_games_popup_search" ) )
	{
		UIListReport * pGamesList = (UIListReport *)gUIShell.FindUIWindow( "zonematch_games_list" );
		if ( !pGamesList )
		{
			return;
		}

		m_GameListSearch = (eGameListSort)ui_window.GetTag();

		gUIShell.ShowInterface( "zonematch_games_popup_search" );

		UIWindow * pBackground = gUIShell.FindUIWindow( "games_search_background" );
		if ( pBackground )
		{
			UIWindow * pCaption = gUIShell.FindUIWindow( "games_search_text" );
			if ( !pCaption )
			{
				return;
			}

			int leftEdge = ui_window.GetRect().right - pGamesList->GetColumnSize( m_GameListSearch ) - pCaption->GetRect().Width() + 4;
			if ( leftEdge < 0 )
				leftEdge = 0;

			pCaption->SetRect(	leftEdge + 2,
								leftEdge + 2 + pCaption->GetRect().Width(),
								ui_window.GetRect().bottom + 2,
								ui_window.GetRect().bottom + 18 );

			pBackground->SetRect(	leftEdge,
									ui_window.GetRect().right,
									ui_window.GetRect().bottom,
									ui_window.GetRect().bottom + 20 );

			UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "games_search_edit_box" );
			if ( pEditBox )
			{
				if ( m_GameListSearch == SORT_PLAYERS )
				{
					pEditBox->SetAllowedChars( L"1234567890-," );
					pEditBox->EnableIme( false ); // 13501 disable ime for player, enabled for all others.
				}
				else
				{
					pEditBox->SetAllowedChars( L"" );
					pEditBox->EnableIme( true ); // 13501 disable ime for player, enabled for all others.
				}

				pEditBox->SetRect( pCaption->GetRect().right + 4, pBackground->GetRect().right - 2, pBackground->GetRect().top + 2, pBackground->GetRect().bottom - 2 );

				if ( m_GameListSort == m_GameListSearch )
				{
					pEditBox->SetText( m_GameListSearchString );
				}
				else
				{
					pEditBox->SetText( L"" );
				}

				pEditBox->GiveFocus();
			}
		}
	}
	else if ( message.same_no_case( "zonematch_show_games_button_rollover" ) )
	{
		ui_window.LoadTexture( "b_gui_cmn_colheader-drop-hov" );
	}
	else if ( message.same_no_case( "zonematch_show_games_button_rolloff" ) )
	{
		if ( ((eGameListSort)ui_window.GetTag() == m_GameListSort) && !m_GameListSearchString.empty() )
		{
			ui_window.LoadTexture( "b_gui_cmn_colheader-drop-hot" );
		}
		else
		{
			ui_window.LoadTexture( "b_gui_cmn_colheader-drop-up" );
		}
	}
	else if ( message.same_no_case( "zonematch_games_search" ) )
	{
		UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "games_search_edit_box" );
		if ( pEditBox )
		{
			UIListReport * pGamesList = (UIListReport *)gUIShell.FindUIWindow( "zonematch_games_list" );
			if ( !pGamesList )
			{
				return;
			}

			m_GameListSearchString = pEditBox->GetText();

			if ( !m_GameListSearchString.empty() )
			{
				m_GameListSort = m_GameListSearch;
				pGamesList->SetSortColumn( m_GameListSearch );
			}

			/*
				if ( m_GameListSort == SORT_GAMENAME )
				{
					m_CurrentViewId = GAMEVIEW_SORT_NAME;

					gpwstring gameSearch = m_GameListSearchString;
					gameSearch.append( L"-ZZZZZZZZZZZZZZZZZZZZZZZZZ" );

					HRESULT hr = m_pGunBrowser->GetPage( GAMEVIEW_SEARCH_NAME, 0, 0, gameSearch.c_str(), NULL );
					gpassertf( !FAILED( hr ), ( "GUN : GetPage failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
				}
			*/

			UpdateGameColumns();

			m_bRefreshGamesList = true;

			if ( pGamesList->GetSelectedElementID() != UIListReport::INVALID_ROW )
			{
				m_SelectedGameName = pGamesList->GetSelectedElementText();
			}

			pGamesList->RemoveAllElements();

			UIWindow * pText = gUIShell.FindUIWindow( "refreshing_games_text", "zonematch_games" );
			if ( pText )
			{
				pText->SetVisible( true );
			}
		}

		gUIShell.HideInterface( "zonematch_games_popup_search" );
	}
	else if ( message.same_no_case( "zonematch_games_search_cancel" ) )
	{
		gUIShell.HideInterface( "zonematch_games_popup_search" );
	}

	///////////////////////////////////////////////////////////////

	else if ( message.same_no_case( "zonematch_exit" ) )
	{
		Disconnect();

		Deactivate();

		gUIMultiplayer.ShowMultiplayerMain();

		m_ConnectionStatus = GUN_DISCONNECTED;
	}
}

void UIZoneMatch :: ShowMatchMakerInterface( bool bReactivate )
{
	gUIMultiplayer.HideInterfaces();

	gUIShell.ShowInterface( "zonematch" );
	gUIShell.ShowInterface( "zonematch_panel" );

	ShowNewsTab();

	if ( m_ConnectionStatus == GUN_CONNECTED )
	{
		ShowConnectedInterface( bReactivate );
	}
	else
	{
		ShowDisconnectedInterface();
	}

	// Initialize interface elements
	UIListReport *listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_games_list" );
	if ( listReport ) 
	{
		listReport->RemoveAllColumns();

//		listReport->InsertColumn( L"" /*Ping*/, 20 );
//		listReport->SetAllowColumnSort( 0 /*Ping*/, false );

		UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_games_list_search_name" );
		listReport->InsertColumn( gpwtranslate( $MSG$ "GAME" ), 190, pButton );

		pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_games_list_search_players" );
		listReport->InsertColumn( gpwtranslate( $MSG$ "PLAYERS" ), 80, pButton );

		pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_games_list_search_map" );
		listReport->InsertColumn( gpwtranslate( $MSG$ "MAP" ), 120, pButton );

		pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_games_list_search_world" );
		listReport->InsertColumn( gpwtranslate( $MSG$ "DIFFICULTY" ), 110, pButton );

		listReport->InsertColumn( gpwtranslate( $MSG$ "GAME TIME" ), 60 );

		//$$pinglistReport->InsertColumn( gpwtranslate( $MSG$ "PING TIME" ), 40 );

//		listReport->SetAllowColumnSort( 2 /*Map*/, false );
//		listReport->SetAllowColumnSort( 3 /*World Difficulty*/, false );
//		listReport->SetAllowColumnSort( 4 /*Game Time*/, false );

		listReport->SetSortColumn( SORT_GAMENAME );

		listReport->AddIcon( "b_gui_fe_m_mp_zone_user_locked", ICON_PASSWORD );

		listReport->AddIcon( "b_gui_fe_m_mp_staging_ping_5", ICON_PING5 );
		listReport->AddIcon( "b_gui_fe_m_mp_staging_ping_4", ICON_PING4 );
		listReport->AddIcon( "b_gui_fe_m_mp_staging_ping_3", ICON_PING3 );
		listReport->AddIcon( "b_gui_fe_m_mp_staging_ping_2", ICON_PING2 );
		listReport->AddIcon( "b_gui_fe_m_mp_staging_ping_1", ICON_PING1 );
	}

	m_pGamesListSlider = (UISlider *)gUIShell.FindUIWindow( "slider_games_list" );
	gpassert( m_pGamesListSlider );

	listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_game_player_list" );
	if ( listReport )
	{
		listReport->RemoveAllColumns();
		listReport->InsertColumn( gpwtranslate( $MSG$ "PLAYER" ), 120 );
		listReport->InsertColumn( gpwtranslate( $MSG$ "TEAM" ), 60 );
		listReport->InsertColumn( gpwtranslate( $MSG$ "CHARACTER" ), 120 );
	}

	listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_chatroom_list" );
	if ( listReport )
	{
		listReport->RemoveAllColumns();
		listReport->InsertColumn( gpwtranslate( $MSG$ "CHANNEL" ), 250 );
		listReport->InsertColumn( gpwtranslate( $MSG$ "USERS" ), 75 );
		listReport->InsertColumn( gpwtranslate( $MSG$ "TOPIC" ), 250 );
	}

	UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_friends_list" );
	if ( listBox )
	{
		listBox->AddIcon( "b_gui_fe_m_mp_zone_user_active", ICON_USER_ACTIVE );
		listBox->AddIcon( "b_gui_fe_m_mp_zone_user_chat",	ICON_USER_CHAT );
		listBox->AddIcon( "b_gui_fe_m_mp_zone_user_ingame", ICON_USER_INGAME );
		listBox->AddIcon( "b_gui_fe_m_mp_zone_user_away",	ICON_USER_AWAY );
		listBox->AddIcon( "b_gui_fe_m_mp_zone_user_dnd",	ICON_USER_DND );
		listBox->AddIcon( "b_gui_fe_m_mp_zone_user_asleep", ICON_USER_OFFLINE );
	}
	listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_messages_list" );
	if ( listBox )
	{
		listBox->AddIcon( "b_gui_fe_m_mp_zone_message_user",  ICON_MESSAGE_USER );
		listBox->AddIcon( "b_gui_fe_m_mp_zone_message_alert", ICON_MESSAGE_ALERT );
	}
	listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
	if ( listBox )
	{
		listBox->AddIcon( "b_gui_fe_m_mp_zone_user_away",	ICON_CHAT_SQUELCH );
		listBox->AddIcon( "b_gui_fe_m_mp_zone_user_dnd",	ICON_CHAT_IGNORE );
		listBox->AddIcon( "b_gui_fe_m_mp_zone_user_opking",	ICON_CHAT_OWNER );
		
	}

	UIChatBox *chatBox = (UIChatBox *)gUIShell.FindUIWindow( "match_chatbox" );
	if ( chatBox )
	{
		chatBox->Clear();
	}

	UICheckbox *checkBox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_auto_signin" );
	if ( checkBox )
	{
		checkBox->SetState( m_bLoginAutomatically );
	}

	// get news
//	HRESULT hr = m_pGunNews->GetNews( m_NewsServerName.c_str(), m_NewsServerFile.c_str(), (unsigned short)m_NewsServerPort, NULL );

	gpstring newsServerFile = m_NewsServerFile;
	newsServerFile.appendf( "?lang=%d", LocMgr::GetDefaultLanguage() );
	HRESULT hr = m_pGunNews->GetNews( m_NewsServerName.c_str(), newsServerFile.c_str(), (unsigned short)m_NewsServerPort, NULL );

	gpassertf( !FAILED( hr ), ( "GUN : GetNews failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	chatBox = (UIChatBox *)gUIShell.FindUIWindow( "match_chatbox_news" );
	if ( chatBox )
	{
		chatBox->Clear();
	}

	// check for AutoUpdate client updates
	m_AutoUpdateStatus = UPDATE_CHECKCLIENT;
	//m_bCheckForUpdate = true;

	if ( gUIMultiplayer.GetWasDisconnectedFromGame() )
	{
		gUIMultiplayer.ShowDisconnectedMessage();
	}

	// set our location settings
	// we only do this once per connection.
	//$$$jt HostUpdateGameFlag( (eZoneGameFlag) ( ZONE_GAME_LANGUAGE << LocMgr::GetDefaultLanguage ), true );
}


void UIZoneMatch :: ShowNewsTab()
{
	gUIShell.HideInterface( "zonematch_game_details" );
	gUIShell.ShowInterface( "zonematch_news" );
	gUIShell.HideInterface( "zonematch_games" );
	gUIShell.HideInterface( "zonematch_chat" );
}


void UIZoneMatch :: ShowGamesTab()
{
	gUIShell.HideInterface( "zonematch_game_details" );
	gUIShell.HideInterface( "zonematch_news" );
	gUIShell.ShowInterface( "zonematch_games" );
	gUIShell.HideInterface( "zonematch_chat" );

	if ( m_TimeToRefreshGamesList == 0.0f )
	{
		CreateGamesList( true );
	}
}


void UIZoneMatch :: ShowChatTab()
{
	gUIShell.HideInterface( "zonematch_game_details" );
	gUIShell.HideInterface( "zonematch_news" );
	gUIShell.HideInterface( "zonematch_games" );
	gUIShell.ShowInterface( "zonematch_chat" );

	if ( !IsSignedIn() )
	{
		ShowDialog( DIALOG_CHAT_SIGN_IN, gpwtranslate( $MSG$ "You must be signed in to chat. Do you want to sign in now?" ) );
	}
	else
	{
		UIListReport *listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_chatroom_list" );
		if ( listReport )
		{
			if ( listReport->GetNumElements() == 0 )
			{
				HRESULT hr = m_pGunChat->GetListOfRooms( NULL );
				gpassertf( !FAILED( hr ), ( "GUN : GetListOfRooms failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
			}
		}
	}
}


void UIZoneMatch :: ShowSignInDialog()
{
	if ( m_Username.empty() )
	{
		// no previous user : go directly to create new account
		ShowCreateAccountDialog();
	}
	else
	{
		gUIShell.ShowInterface( "zonematch_signin" );

		UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "user_name_edit_box", "zonematch_signin" );
		if ( eb )
		{
			stringtool::RemoveBorderingWhiteSpace( m_Username ); //BUG: 12818
			eb->SetText( m_Username );
			eb->SetAllowInput( true );
		}
		eb = (UIEditBox *)gUIShell.FindUIWindow( "password_edit_box", "zonematch_signin" );
		if ( eb )
		{
			eb->SetText( m_bLoginAutomatically ? m_Password : L"" );
			eb->SetAllowInput( false );
		}
		UICheckbox *checkBox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_auto_signin", "zonematch_signin" );
		if ( checkBox )
		{
			checkBox->SetState( m_bLoginAutomatically );
		}
	}
}


void UIZoneMatch :: ShowCreateAccountDialog()
{
	gUIShell.HideInterface( "zonematch_signin" );
	gUIShell.HideInterface( "zonematch_account_settings" );
	gUIShell.ShowInterface( "zonematch_create_account" );

	UIEditBox *eb = (UIEditBox *)gUIShell.FindUIWindow( "create_account_user_name_edit_box" );
	if ( eb )
	{
		eb->SetText( L"" );
		eb->SetAllowInput( true );
	}
	eb = (UIEditBox *)gUIShell.FindUIWindow( "create_account_password_edit_box" );
	if ( eb )
	{
		eb->SetText( L"" );
		eb->SetAllowInput( false );
	}
	eb = (UIEditBox *)gUIShell.FindUIWindow( "create_account_password2_edit_box" );
	if ( eb )
	{
		eb->SetText( L"" );
		eb->SetAllowInput( false );
	}

	EnableButton( "button_create_account_ok", false );
}


void UIZoneMatch :: EnableButton( const char * buttonName, bool bState )
{
	UIWindow *pButton = gUIShell.FindUIWindow( buttonName );
	if ( pButton )
	{
		pButton->SetEnabled( bState );

		if ( bState )
		{
			pButton->SetAlpha( 1.0f );
		}
		else
		{
			pButton->SetAlpha( 0.6f );
		}
		/*
		for ( UIWindowVec::iterator i = pButton->GetChildren().begin(); i != pButton->GetChildren().end(); ++i )
		{
			if ( (*i)->GetType() == UI_TYPE_TEXT )
			{
				if ( bState )
				{
					((UIText *)(*i))->SetColor( 0xFFFFFFFF );
				}
				else
				{
					((UIText *)(*i))->SetColor( 0xFF808080 );
				}
			}
		}
		*/
	}
}

void UIZoneMatch :: EnableTextBox( const char * textbox, bool bState, const char * sInterface  )
{
	UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( textbox, sInterface );
	if ( pTextBox )
	{
		if( bState )
		{
			pTextBox->SetTextColor( RGB(255,255,255) );
		}
		else
		{
			pTextBox->SetTextColor( 0xFF808080 );
		}
	}

}


void UIZoneMatch :: EnableRadioButton( const char * radioButton, bool bState, const char * sInterface  )
{
	UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( radioButton, sInterface );
	if ( pRadio )
	{
		if( bState )
		{
			pRadio->SetEnabled( true );
			pRadio->SetAllowUserPress( true );
			pRadio->SetAlpha( 1.0f );
		}
		else
		{
			pRadio->SetEnabled( false );
			pRadio->SetAllowUserPress( false );
			pRadio->SetAlpha( 0.50f );
		}
	}

}



void UIZoneMatch :: ResizeChatArea( eChatAreaSize size )
{
	int offset = (int)size;

	// Resize Chat Area
	UIDialogBox * dialogBox = (UIDialogBox *)gUIShell.FindUIWindow( "backgound_chat" );
	if ( dialogBox )
	{
		bool chatVis = (size != CHAT_AREA_NONE);

		gUIShell.ShiftGroup( "zonematch", "zone_chat_header", 0, offset - dialogBox->GetRect().top );
		gUIShell.ShowGroup( "zone_chat_header", chatVis );

		dialogBox->SetVisible( chatVis );
		dialogBox->GetRect().top = offset;

		if ( (size == CHAT_AREA_SMALL) || (size == CHAT_AREA_NONE) )
		{
			UIChatBox * chatBox = (UIChatBox *)gUIShell.FindUIWindow( "match_chatbox" );
			if ( chatBox )
			{
				chatBox->SetVisible( false );
			}

			UIWindow * pWindow = gUIShell.FindUIWindow( "backgound_chatusers" );
			if ( pWindow )
			{
				pWindow->SetVisible( false );
				UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
				if ( pListbox )
				{
					pListbox->SetVisible( false );
				}
			}

			gUIShell.ShowGroup( "zone_chat_text", false );
		}
		else
		{
			UIChatBox * chatBox = (UIChatBox *)gUIShell.FindUIWindow( "match_chatbox" );
			if ( chatBox )
			{
				chatBox->SetVisible( true );
				chatBox->GetRect().top = dialogBox->GetRect().top + 25;
				chatBox->RefreshSlider();
			}

			UIWindow * pWindow = gUIShell.FindUIWindow( "backgound_chatusers" );
			if ( pWindow )
			{
				pWindow->SetVisible( true );
				pWindow->GetRect().top = offset + 25;
				UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
				if ( pListbox )
				{
					pListbox->SetVisible( true );
					pListbox->SetRect( pListbox->GetRect().left, pListbox->GetRect().right, offset + 25, pListbox->GetRect().bottom );
					pListbox->SetHeight( 16 );
					pListbox->ResizeSlider();
				}
			}

			gUIShell.ShowGroup( "zone_chat_text", true );
		}
	}

	// Resize tab areas
	dialogBox = (UIDialogBox *)gUIShell.FindUIWindow( "backgound_news_area" );
	if ( dialogBox )
	{
		dialogBox->GetRect().bottom = offset - 6;

		UIChatBox *chatBox = (UIChatBox *)gUIShell.FindUIWindow( "match_chatbox_news" );
		if ( chatBox )
		{
			chatBox->GetRect().bottom = dialogBox->GetRect().bottom - 3;
			chatBox->RefreshSlider();
		}
	}

	dialogBox = (UIDialogBox *)gUIShell.FindUIWindow( "backgound_games_area" );
	if ( dialogBox )
	{
		dialogBox->GetRect().bottom = offset - 6;

		UIWindow * pWindow = gUIShell.FindUIWindow( "background_gamelist_commands" );
		if ( pWindow )
		{
			UIListReport * listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_games_list" );
			if ( listReport )
			{
				listReport->GetRect().bottom = dialogBox->GetRect().bottom - 3;
				listReport->GetUsableRect().bottom = dialogBox->GetRect().bottom - 3;

				UISlider * pSlider = (UISlider *)gUIShell.FindUIWindow( "slider_games_list" );
				if ( pSlider )
				{
					pSlider->SetRect( pSlider->GetRect().left, pSlider->GetRect().right, listReport->GetUsableRect().top, listReport->GetUsableRect().bottom );
					pSlider->ResizeSliderButtons();
				}

				m_CurrentViewsPerPage = (unsigned int)ceilf( (float)m_CurrentGameRowsPerPage / listReport->GetMaxActiveElements() );
			}
		}
	}

	dialogBox = (UIDialogBox *)gUIShell.FindUIWindow( "backgound_game_details_area" );
	if ( dialogBox )
	{
		dialogBox->GetRect().bottom = offset - 6;

		UIWindow * pWindow = gUIShell.FindUIWindow( "backgound_player_list" );
		if ( pWindow )
		{
			pWindow->GetRect().bottom = offset - 8;

			UIListReport * listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_game_player_list" );
			if ( listReport )
			{
				listReport->GetRect().bottom = dialogBox->GetRect().bottom - 3;
				listReport->GetUsableRect().bottom = listReport->GetRect().bottom;

				UISlider * pSlider = listReport->GetChildSlider();
				if ( pSlider )
				{
					pSlider->SetRect( pSlider->GetRect().left, pSlider->GetRect().right, listReport->GetUsableRect().top, listReport->GetUsableRect().bottom );
					pSlider->ResizeSliderButtons();
				}

				listReport->ResizeSlider();
			}
		}

		UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "game_details_settings_listbox", "zonematch_game_details" );
		if ( pListbox )
		{
			pListbox->SetRect( pListbox->GetRect().left, pListbox->GetRect().right, pListbox->GetRect().top, offset - 10 );
			pListbox->SetHeight( 16 );
			pListbox->ResizeSlider();
		}
	}

	dialogBox = (UIDialogBox *)gUIShell.FindUIWindow( "backgound_chat_area" );
	if ( dialogBox )
	{
		dialogBox->GetRect().bottom = offset - 6;

		UIWindow * pWindow = gUIShell.FindUIWindow( "background_chatlist_commands" );
		if ( pWindow )
		{
			UIListReport * listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_chatroom_list" );
			if ( listReport )
			{
				listReport->GetRect().bottom = dialogBox->GetRect().bottom - 3;
				listReport->GetUsableRect().bottom = dialogBox->GetRect().bottom - 3;

				UISlider * pSlider = listReport->GetChildSlider();
				if ( pSlider )
				{
					pSlider->SetRect( pSlider->GetRect().left, pSlider->GetRect().right, listReport->GetUsableRect().top, listReport->GetUsableRect().bottom );
					pSlider->ResizeSliderButtons();
				}

				listReport->ResizeSlider();
			}
		}
	}

	// Update chat area
	{
		int top = 0, bottom = 0, rightEdge = 0;

		UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_chat_leave", "zonematch" );
		if ( pButton )
		{
			top = pButton->GetRect().top;
			bottom = pButton->GetRect().bottom;
			rightEdge = pButton->GetRect().left - 4;
		}

		UIButton * pMinBtn = (UIButton *)gUIShell.FindUIWindow( "button_chat_min", "zonematch" );
		UIButton * pMidBtn = (UIButton *)gUIShell.FindUIWindow( "button_chat_mid", "zonematch" );
		UIButton * pMaxBtn = (UIButton *)gUIShell.FindUIWindow( "button_chat_max", "zonematch" );

		// the chat window might not have been initalized when we are returning to the main menu.
		// we don't need to set the buttons invisible, if they aren't even there :)
		if( pMinBtn && pMidBtn && pMaxBtn )
		{
			if ( size == CHAT_AREA_NONE )
			{
				pMinBtn->SetVisible( false );
				pMidBtn->SetVisible( false );
				pMaxBtn->SetVisible( false );
			}
			else
			{
				pMinBtn->SetVisible( size != CHAT_AREA_SMALL );
				pMidBtn->SetVisible( size != CHAT_AREA_MEDIUM );
				pMaxBtn->SetVisible( size != CHAT_AREA_LARGE );
			}

			if ( size == CHAT_AREA_SMALL )
			{
				pMidBtn->SetRect( rightEdge - 32, rightEdge - 16, top, bottom );
				pMaxBtn->SetRect( rightEdge - 16, rightEdge, top, bottom );
			}
			else if ( size == CHAT_AREA_MEDIUM )
			{
				pMinBtn->SetRect( rightEdge - 32, rightEdge - 16, top, bottom );
				pMaxBtn->SetRect( rightEdge - 16, rightEdge, top, bottom );
			}
			else if ( size == CHAT_AREA_LARGE )
			{
				pMinBtn->SetRect( rightEdge - 32, rightEdge - 16, top, bottom );
				pMidBtn->SetRect( rightEdge - 16, rightEdge, top, bottom );
			}
		}
	}

	if ( gUIShell.IsInterfaceVisible( "zonematch_games" ) )
	{
		m_bRefreshGamesList = true;
	}
}


void UIZoneMatch :: ShowConnectedInterface( bool bReactivate )
{
	gUIMultiplayer.HideMessageBox();

	UITab * pTab = (UITab *)gUIShell.FindUIWindow( "tab_games", "zonematch_news" );
	if ( pTab )
	{
		pTab->SetEnabled( true );
	}
	pTab = (UITab *)gUIShell.FindUIWindow( "tab_chat", "zonematch_news" );
	if ( pTab )
	{
		pTab->SetEnabled( true );
	}

	EnableButton( "button_sign_in", true );
	EnableButton( "button_account_options", true );

	UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_connect", "zonematch_panel" );
	if ( pButton )
	{
		pButton->SetVisible( false );
	}

	// Init Games List
	{
		UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_button_show_all_games" );
		if ( pButton )
		{
			pButton->SetPressedState( false );
			if ( m_GameViewType == SHOW_ALL_GAMES )
			{
				m_pSelectedGamesTab = pButton;
			}
		}
		pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_button_show_filtered_games" );
		if ( pButton )
		{
			pButton->SetPressedState( false );
			if ( m_GameViewType == SHOW_FILTERED_GAMES )
			{
				m_pSelectedGamesTab = pButton;
			}
		}
		pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_button_show_friends_games" );
		if ( pButton )
		{
			pButton->SetPressedState( false );
			if ( m_GameViewType == SHOW_FRIENDS_GAMES )
			{
				m_pSelectedGamesTab = pButton;
			}
		}

		if_gpassert( m_pSelectedGamesTab )
		{
			m_pSelectedGamesTab->SetPressedState( true );
		}		

		UIText * pText = (UIText *)gUIShell.FindUIWindow( "zonematch_button_show_all_games_text" );
		if ( pText )
		{
			pText->SetColor( (m_GameViewType == SHOW_ALL_GAMES) ? 0xff95fffe : 0xffffffff );
		}
		pText = (UIText *)gUIShell.FindUIWindow( "zonematch_button_show_friends_games_text" );
		if ( pText )
		{
			pText->SetColor( (m_GameViewType == SHOW_FRIENDS_GAMES) ? 0xff95fffe : 0xffffffff );
		}
		pText = (UIText *)gUIShell.FindUIWindow( "zonematch_button_show_filtered_games_text" );
		if ( pText )
		{
			pText->SetColor( (m_GameViewType == SHOW_FILTERED_GAMES) ? 0xff95fffe : 0xffffffff );
		}
	}

	m_GamesList.clear();
	m_ViewHeaders.clear();
	m_CurrentViewId = GAMEVIEW_SORT_NAME;
	m_CurrentGamePage = 0;
	m_GameListSort = SORT_GAMENAME;
	m_bGameListSortInverse = false;
	m_GameListSearchString.clear();
//	m_bRequestGameListCreation = true;

	UpdateGameColumns();

	if ( !m_bSignedIn )
	{
		DisableInterfaceOnLogout();

		// attempt login
		if ( !bReactivate && m_bLoginAutomatically && !m_Username.empty() && !m_Password.empty() )
		{
			m_EnteredUsername = m_Username;
			m_EnteredPassword = m_Password;
			HRESULT hr = m_pGunAccount->LoginUser( m_Username.c_str(), m_Password.c_str(), NULL );
			gpassertf( !FAILED( hr ), ( "GUN : LoginUser failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

			gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Signing in to ZoneMatch Server..." ), MP_DIALOG_CANCEL_ZONE_LOGIN );
		}
	}
	else
	{
		EnableInterfaceOnLogin();

		// update all the messages that are comming in
		CreateMessageList( );
	}
}


void UIZoneMatch :: ShowDisconnectedInterface()
{
	HideDialogs();
	DisableInterfaceOnLogout();
	if ( gWorldState.GetCurrentState() == WS_MP_MATCH_MAKER )
	{
		ShowNewsTab();

		UITab *pTab = (UITab *)gUIShell.FindUIWindow( "tab_news" );
		if ( pTab )
		{
			pTab->SetCheck( true );
		}
	}

	UITab * pTab = (UITab *)gUIShell.FindUIWindow( "tab_games", "zonematch_news" );
	if ( pTab )
	{
		pTab->SetEnabled( false );
	}
	pTab = (UITab *)gUIShell.FindUIWindow( "tab_chat", "zonematch_news" );
	if ( pTab )
	{
		pTab->SetEnabled( false );
	}

	UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_connect", "zonematch_panel" );
	if ( pButton )
	{
		pButton->SetVisible( true );
	}
	pButton = (UIButton *)gUIShell.FindUIWindow( "button_sign_in", "zonematch_panel" );
	if ( pButton )
	{
		pButton->SetVisible( false );
	}

	EnableButton( "button_sign_in", false );
	EnableButton( "button_account_options", false );

	UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "match_chat_edit_box" );
	if ( editBox )
	{
		editBox->SetEnabled( false );
		editBox->SetVisible( false );
	}

	m_Messages.clear();
	m_ChatLogs.clear();
	CreateMessageList();
}

void UIZoneMatch :: ClearPendingRequests()
{
	///////////////////////////////////////
	// only do this if we are logging on 
	// we don't want to loose this info if we
	// are just moving in and out of games and staging
	// areas

	m_NextMessageId = 0;

	m_Messages.clear();
	m_ChatLogs.clear();

	m_bHostOfChannel = false;
	m_CurrentRoomId = 0;
	m_CurrentChannel.clear();

	m_NextFriendAddRequest = 1;
	m_FriendAddRequests.clear();

	m_NextSquelchRequest = 1;
	m_SquelchRequests.clear();

	m_Ignored.clear();
}

void UIZoneMatch :: EnableInterfaceOnLogin()
{
	SetCurrentUserStatus( LOGGEDIN );
	UpdateCurrentUserStatus( );
	m_bSignedIn = true;

	// Commands Area
	UIText * pText = (UIText *)gUIShell.FindUIWindow( "current_user_name_text" );
	if ( pText )
	{
		pText->SetText( m_Username );
	}

	gUIShell.ShowGroup( "panel_signedin", true );
	gUIShell.ShowGroup( "panel_signedout", false );

	// Chat
	ResizeChatArea( CHAT_AREA_NONE );

	UIChatBox *chatBox = (UIChatBox *)gUIShell.FindUIWindow( "match_chatbox" );
	if ( chatBox )
	{
		chatBox->Clear();
	}

	UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "match_chat_edit_box" );
	if ( editBox )
	{
		editBox->SetText( L"" );
	}

	UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_join_chatroom", "zonematch_chat" );
	if ( pButton )
	{
		pButton->SetEnabled( true );
	}
	pButton = (UIButton *)gUIShell.FindUIWindow( "button_create_chatroom", "zonematch_chat" );
	if ( pButton )
	{
		pButton->SetEnabled( true );
	}
	pButton = (UIButton *)gUIShell.FindUIWindow( "button_refresh_chatroom_list", "zonematch_chat" );
	if ( pButton )
	{
		pButton->SetEnabled( true );
	}

	// Friends
	pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_button_show_friends_games", "zonematch_games" );
	if ( pButton )
	{
		pButton->SetEnabled( true );
	}

	UIListbox * listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_messages_list" );
	if ( listBox )
	{
		listBox->RemoveAllElements();
	}
	listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_friends_list" );
	if ( listBox )
	{
		listBox->RemoveAllElements();
	}

	HRESULT hr = m_pGunFriends->GetFriendsList( NULL );
	gpassertf( !FAILED( hr ), ( "GUN : GetFriendsList failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	hr = m_pGunFriends->Flags( IGunFriends::GET, IGunFriends::CONTROL, NULL, NULL );
	gpassertf( !FAILED( hr ), ( "GUN : Flags failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	hr = m_pGunFriends->GetIgnoredList( NULL );
	gpassertf( !FAILED( hr ), ( "GUN : GetIgnoredList failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	// Make sure we refresh the chat list so it automatically come down when we login.
	RequestChatRefresh();
}


void UIZoneMatch :: DisableInterfaceOnLogout()
{
	m_PlayerName.clear();
	m_bSignedIn = false;

	// Commands Area
	UIText * pText = (UIText *)gUIShell.FindUIWindow( "current_user_name_text" );
	if ( pText )
	{
		pText->SetText( gpwtranslate( $MSG$ "Not Signed In" ) );
	}

	gUIShell.ShowGroup( "panel_signedin", false );
	gUIShell.ShowGroup( "panel_signedout", true );

	// Chat
	ResizeChatArea( CHAT_AREA_NONE );

	UIListReport *listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_chatroom_list" );
	if ( listReport )
	{
		listReport->RemoveAllElements();
	}

	UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_join_chatroom", "zonematch_chat" );
	if ( pButton )
	{
		pButton->SetEnabled( false );
	}
	pButton = (UIButton *)gUIShell.FindUIWindow( "button_create_chatroom", "zonematch_chat" );
	if ( pButton )
	{
		pButton->SetEnabled( false );
	}
	pButton = (UIButton *)gUIShell.FindUIWindow( "button_refresh_chatroom_list", "zonematch_chat" );
	if ( pButton )
	{
		pButton->SetEnabled( false );
	}

	// Games
	if ( m_GameViewType == SHOW_FRIENDS_GAMES )
	{
		m_GameViewType = SHOW_ALL_GAMES;

		pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_button_show_all_games" );
		if ( pButton )
		{
			pButton->SetPressedState( true );
			m_pSelectedGamesTab = pButton;
		}
		pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_button_show_friends_games" );
		if ( pButton )
		{
			pButton->SetPressedState( false );
		}
		UIText * pText = (UIText *)gUIShell.FindUIWindow( "zonematch_button_show_all_games_text" );
		if ( pText )
		{
			pText->SetColor( 0xff95fffe );
		}
		pText = (UIText *)gUIShell.FindUIWindow( "zonematch_button_show_friends_games_text" );
		if ( pText )
		{
			pText->SetColor( 0xffffffff );
		}

		m_bRequestGameListCreation = true;
	}

	pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_button_show_friends_games", "zonematch_games" );
	if ( pButton )
	{
		pButton->SetEnabled( false );
	}

	HidePrivateChatWindow();
}


void UIZoneMatch :: HideDialogs()
{
	gUIShell.HideInterface( "zonematch_signin" );
	gUIShell.HideInterface( "zonematch_create_account" );
	gUIShell.HideInterface( "zonematch_account_settings" );
	gUIShell.HideInterface( "zonematch_friends_settings" );
	gUIShell.HideInterface( "zonematch_ignored_settings" );
	gUIShell.HideInterface( "zonematch_change_password" );
	gUIShell.HideInterface( "zonematch_create_chatroom" );
	gUIShell.HideInterface( "zonematch_send_message" );
	gUIShell.HideInterface( "zonematch_view_message" );
	gUIShell.HideInterface( "zonematch_input_dialog" );
	gUIShell.HideInterface( "zonematch_games_filter" );
}

void UIZoneMatch :: RefreshFriendsList( bool /*fullRefresh*/ )
{

	// refresh our in game list that we use for chatting in game
	//$$$ now done on the combo up gUIMultiplayer.UpdatePlayerListBox( );

	UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_friends_list" );
	if ( !pListbox )
	{
		return;
	}

	gpwstring selectedFriend = pListbox->GetSelectedText();

	pListbox->RemoveAllElements();
	pListbox->SetHelpBoxName( gpstring().assign("zonematch_help") );
	gpwstring tooltipMessage;

	// Active users
	{	int tag = 0;
	for ( FriendsMap::iterator i = m_Friends.begin(); i != m_Friends.end(); ++i, ++tag )
	{
		if ( (i->second & LOGGEDIN) && ((i->second & ~LOGGEDIN) == 0) )
		{
			pListbox->AddElement( i->first, tag );
			pListbox->SetElementIcon( i->first, ICON_USER_ACTIVE );
			tooltipMessage.assignf( gpwtranslate( $MSG$ "%s is currently signed in to ZoneMatch"), i->first );
			pListbox->SetElementTagToolTip( tag, tooltipMessage );
		}
	}}

	// Staging area users
	{	int tag = 0;
	for ( FriendsMap::iterator i = m_Friends.begin(); i != m_Friends.end(); ++i, ++tag )
	{
		if ( i->second & INSTAGING )
		{
			if ( !pListbox->GetTagElement( tag ) )
			{
				pListbox->AddElement( i->first, tag );
				pListbox->SetElementIcon( i->first, ICON_USER_INGAME );
				tooltipMessage.assignf( gpwtranslate( $MSG$ "%s is currently in a staging area"), i->first );
				pListbox->SetElementTagToolTip( tag, tooltipMessage );
			}
		}
	}}

	// In-Chat users
	{	int tag = 0;
	for ( FriendsMap::iterator i = m_Friends.begin(); i != m_Friends.end(); ++i, ++tag )
	{
		if ( i->second & INCHAT )
		{
			if ( !pListbox->GetTagElement( tag ) )
			{
				pListbox->AddElement( i->first, tag );
				pListbox->SetElementIcon( i->first, ICON_USER_CHAT );
				tooltipMessage.assignf( gpwtranslate( $MSG$ "%s is currently chatting"), i->first );
				pListbox->SetElementTagToolTip( tag, tooltipMessage );
			}
		}
	}}

	// In-Game users
	{	int tag = 0;
	for ( FriendsMap::iterator i = m_Friends.begin(); i != m_Friends.end(); ++i, ++tag )
	{
		if ( i->second & INGAME )
		{
			if ( !pListbox->GetTagElement( tag ) )
			{
				pListbox->AddElement( i->first, tag );
				pListbox->SetElementIcon( i->first, ICON_USER_INGAME );
				tooltipMessage.assignf( gpwtranslate( $MSG$ "%s is currently in a game"), i->first );
				pListbox->SetElementTagToolTip( tag, tooltipMessage );
			}
		}
	}}

	// FRIENDS_ONLY users
	{	int tag = 0;
	for ( FriendsMap::iterator i = m_Friends.begin(); i != m_Friends.end(); ++i, ++tag )
	{
		if ( i->second & FRIENDS_ONLY )
		{
			if ( !pListbox->GetTagElement( tag ) )
			{
				pListbox->AddElement( i->first, tag );
				pListbox->SetElementIcon( i->first, ICON_USER_DND );
				tooltipMessage.assignf( gpwtranslate( $MSG$ "%s is currently in a game and only accepting messages from friends"), i->first );
				pListbox->SetElementTagToolTip( tag, tooltipMessage );
			}
		}
	}}

	// DND users
	{	int tag = 0;
	for ( FriendsMap::iterator i = m_Friends.begin(); i != m_Friends.end(); ++i, ++tag )
	{
		if ( i->second & DND )
		{
			if ( !pListbox->GetTagElement( tag ) )
			{
				pListbox->AddElement( i->first, tag );
				pListbox->SetElementIcon( i->first, ICON_USER_DND );
				tooltipMessage.assignf( gpwtranslate( $MSG$ "%s is currently in a game and is not accepting messages"), i->first );
				pListbox->SetElementTagToolTip( tag, tooltipMessage );
			}
		}
	}}

	// Anyone else
	{	int tag = 0;
	for ( FriendsMap::iterator i = m_Friends.begin(); i != m_Friends.end(); ++i, ++tag )
	{
		if ( !pListbox->GetTagElement( tag ) )
		{
			pListbox->AddElement( i->first, tag );
			pListbox->SetElementIcon( i->first, ICON_USER_OFFLINE );
			tooltipMessage.assignf( gpwtranslate( $MSG$ "%s is currently not signed in to ZoneMatch"), i->first );
			pListbox->SetElementTagToolTip( tag, tooltipMessage );
		}
	}}

	UpdateDynamicChatToolTips();

	UpdateFriendsNewMessages();

	pListbox->SelectElement( selectedFriend );

/*
	if ( i->second & DND )
	{
		pListbox->SetElementIcon( i->first, ICON_USER_DND );
	}
	else if ( i->second & INGAME )
	{
		pListbox->SetElementIcon( i->first, ICON_USER_INGAME );
	}
	else if ( i->second & INCHAT )
	{
		pListbox->SetElementIcon( i->first, ICON_USER_CHAT );
	}
	else if ( i->second & AFK )
	{
		pListbox->SetElementIcon( i->first, ICON_USER_AWAY );
	}
	else if ( i->second & LOGGEDIN )  
	{
		pListbox->SetElementIcon( i->first, ICON_USER_ACTIVE );
	}
	else // ( i->second == LOGGEDOUT )
	{
		pListbox->SetElementIcon( i->first, ICON_USER_OFFLINE );
	}
*/
}

void UIZoneMatch :: UpdateFriendsNewMessages()
{
	UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_friends_list" );
	if ( !pListbox )
	{
		return;
	}

	for ( UINT tag = 0; tag < pListbox->GetNumElements(); ++tag )
	{
		pListbox->FlashElementIcon( tag, 0.0f );
	}

	for ( std::vector< gpwstring >::iterator i = m_NewMessages.begin(); i != m_NewMessages.end(); ++i )
	{
		ListboxElement * pElement = pListbox->GetTextElement( *i );
		if ( pElement )
		{
			pListbox->FlashElementIcon( pElement->tag );
		}
	}
}

void UIZoneMatch :: SetNewMessageState( gpwstring userName, bool bActive )
{
	bool found = false;

	for ( std::vector< gpwstring >::iterator i = m_NewMessages.begin(); i != m_NewMessages.end(); ++i )
	{
		if ( userName.same_no_case( *i ) )
		{
			found = true;
			m_NewMessages.erase( i );
			break;
		}
	}

	if ( bActive )
	{
		m_NewMessages.push_back( userName );
	}
}

void UIZoneMatch :: CreateMessageList( )
{
	UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_messages_list" );
	if ( listBox )
	{

		listBox->RemoveAllElements();

		for ( UserMessageColl::reverse_iterator i = m_Messages.rbegin(); i != m_Messages.rend(); ++i )
		{
			listBox->AddElement( i->From, i->Id );
			if ( i->Type == USER_MESSAGE )
			{
				listBox->SetElementIcon( i->Id, ICON_MESSAGE_USER );
			}
			else if ( i->Type == USER_REQUEST_ADD )
			{
				listBox->SetElementIcon( i->Id, ICON_MESSAGE_ALERT );
			}
			else if ( i->Type == USER_CHAT_INVITE )
			{
				listBox->SetElementIcon( i->Id, ICON_MESSAGE_ALERT );
			}
		}	
	
	}
}


void UIZoneMatch :: OnLeaveChatRoom( bool bHideChatArea )
{
	if( m_bHostOfChannel )
	{
		gpwstring newOwnerName;
		// get the user at the top of the list to be the new host
		UIListbox *chatListBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
		if( chatListBox )
		{
			for( unsigned int i = 1; i < chatListBox->GetNumElements(); i++)
			{
				newOwnerName = chatListBox->GetElementText( i );
				if( !newOwnerName.empty() )
				{
					break;
				}
			}
		}

		if( !newOwnerName.empty() )
		{
			HRESULT hr = m_pGunChat->AssignOwnership( m_CurrentRoomId, newOwnerName, NULL );
			gpassertf( !FAILED( hr ), ( "GUN : AssignOwnership failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
		}
	}

	UIText *text = (UIText *)gUIShell.FindUIWindow( "chat_title", "zonematch" );
	if ( text )
	{
		text->SetText( L"" );
	}
	UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( "button_chat_leave" );
	if ( pButton )
	{
		pButton->SetVisible( false );
	}

	EnableButton( "tab_view_chat", false );
	UITab *pTab = (UITab *)gUIShell.FindUIWindow( "tab_view_friends" );
	if ( pTab )
	{
		pTab->SetCheck( true );
	}
	pTab = (UITab *)gUIShell.FindUIWindow( "tab_view_chat" );
	if ( pTab )
	{
		pTab->SetVisible( false );
		pTab->SetCheck( false );
	}

	UIChatBox *chatBox = (UIChatBox *)gUIShell.FindUIWindow( "match_chatbox" );
	if ( chatBox )
	{
		chatBox->Clear();
	}

	m_bHostOfChannel = false;
	m_CurrentRoomId = 0;
	m_CurrentChannel.clear();
	m_CurrentChannelPassword.clear();

	if ( bHideChatArea )
	{
		ResizeChatArea( CHAT_AREA_NONE );
	}

	SetCurrentUserStatus( LOGGEDIN );
	UpdateCurrentUserStatus( );
}

void UIZoneMatch::GetGamesListHeaders()
{
	if ( !m_ViewHeaders.empty() )
	{
		return;
	}

	HRESULT hr = m_pGunBrowser->GetHeaderRowOfView( GAMEVIEW_SORT_NAME, (PVOID)GAMEVIEW_SORT_NAME );
	if ( hr == ZT_E_NOT_CONNECTED )
	{
		// if disconnected while in game, returning directly to ZoneMatch may occur before the disconnect message arrives.
		ShowDisconnectedInterface();
		return;
	}
		gpassertf( !FAILED( hr ), ( "GUN : GetHeaderRowOfView failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	hr = m_pGunBrowser->GetHeaderRowOfView( GAMEVIEW_SORT_MAP, (PVOID)GAMEVIEW_SORT_MAP );
		gpassertf( !FAILED( hr ), ( "GUN : GetHeaderRowOfView failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	hr = m_pGunBrowser->GetHeaderRowOfView( GAMEVIEW_SORT_WORLD, (PVOID)GAMEVIEW_SORT_WORLD );
		gpassertf( !FAILED( hr ), ( "GUN : GetHeaderRowOfView failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	hr = m_pGunBrowser->GetHeaderRowOfView( GAMEVIEW_SORT_PLAYERS, (PVOID)GAMEVIEW_SORT_PLAYERS );
		gpassertf( !FAILED( hr ), ( "GUN : GetHeaderRowOfView failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	hr = m_pGunBrowser->GetHeaderRowOfView( GAMEVIEW_SORT_TIME, (PVOID)GAMEVIEW_SORT_TIME );
		gpassertf( !FAILED( hr ), ( "GUN : GetHeaderRowOfView failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	hr = m_pGunBrowser->GetHeaderRowOfView( GAMEVIEW_SORT_FILTERED, (PVOID)GAMEVIEW_SORT_FILTERED );
		gpassertf( !FAILED( hr ), ( "GUN : GetHeaderRowOfView failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	hr = m_pGunBrowser->GetHeaderRowOfView( GAMEVIEW_SEARCH_NAME, (PVOID)GAMEVIEW_SEARCH_NAME );
		gpassertf( !FAILED( hr ), ( "GUN : GetHeaderRowOfView failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	hr = m_pGunBrowser->GetHeaderRowOfView( GAMEVIEW_SORT_NAME_FRIENDSONLY, (PVOID)GAMEVIEW_SORT_NAME_FRIENDSONLY );
		gpassertf( !FAILED( hr ), ( "GUN : GetHeaderRowOfView failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	hr = m_pGunBrowser->GetHeaderRowOfView( GAMEVIEW_SORT_MAP_FRIENDSONLY, (PVOID)GAMEVIEW_SORT_MAP_FRIENDSONLY );
		gpassertf( !FAILED( hr ), ( "GUN : GetHeaderRowOfView failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	hr = m_pGunBrowser->GetHeaderRowOfView( GAMEVIEW_SORT_WORLD_FRIENDSONLY, (PVOID)GAMEVIEW_SORT_WORLD_FRIENDSONLY );
		gpassertf( !FAILED( hr ), ( "GUN : GetHeaderRowOfView failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	hr = m_pGunBrowser->GetHeaderRowOfView( GAMEVIEW_SORT_PLAYERS_FRIENDSONLY, (PVOID)GAMEVIEW_SORT_PLAYERS_FRIENDSONLY );
		gpassertf( !FAILED( hr ), ( "GUN : GetHeaderRowOfView failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	hr = m_pGunBrowser->GetHeaderRowOfView( GAMEVIEW_SORT_TIME_FRIENDSONLY, (PVOID)GAMEVIEW_SORT_TIME_FRIENDSONLY );
		gpassertf( !FAILED( hr ), ( "GUN : GetHeaderRowOfView failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	hr = m_pGunBrowser->GetHeaderRowOfView( GAMEVIEW_GAME_DETAILS, (PVOID)GAMEVIEW_GAME_DETAILS );
	gpassertf( !FAILED( hr ), ( "GUN : GetHeaderRowOfView failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	hr = m_pGunBrowser->GetHeaderRowOfView( GAMEVIEW_GAME_PLAYERS, (PVOID)GAMEVIEW_GAME_PLAYERS );
	gpassertf( !FAILED( hr ), ( "GUN : GetHeaderRowOfView failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
}


void UIZoneMatch :: CreateGamesList( bool forceRefresh )
{
	GetGamesListHeaders();

	UIListReport *listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_games_list" );
	gpassert( listReport );

	if ( listReport->GetSelectedElementID() != UIListReport::INVALID_ROW )
	{
		m_SelectedGameName = listReport->GetSelectedElementText();
	}

	listReport->RemoveAllElements();

	UIWindow * pRefreshText = gUIShell.FindUIWindow( "refreshing_games_text", "zonematch_games" );
	gpassert( pRefreshText );

	unsigned int pageAt = m_pGamesListSlider->GetValue() / m_CurrentViewsPerPage;

	if ( !forceRefresh )
	{
		if ( pageAt == m_CurrentGamePage )
		{
			int pagePos = listReport->GetMaxActiveElements() * (m_pGamesListSlider->GetValue() % m_CurrentViewsPerPage);
			int pageEndPos = pagePos + listReport->GetMaxActiveElements();

			if ( pageEndPos > (int)m_GamesSorted.size() )
			{
				pagePos = m_GamesSorted.size() - listReport->GetMaxActiveElements();
				pagePos = (pagePos < 0) ? 0 : pagePos;
				pageEndPos = pagePos + listReport->GetMaxActiveElements();
				pageEndPos = (pageEndPos > (int)m_GamesSorted.size()) ? (int)m_GamesSorted.size() : pageEndPos;
			}

			for ( int pos = pagePos; pos < pageEndPos; ++pos )
			{
				GameInfoMap::iterator findGame = m_GamesList.find( m_GamesSorted[ pos ] );
				if ( findGame == m_GamesList.end() )
				{
					continue;
				}

				GameInfo &gameInfo = findGame->second;

				listReport->AddElement( L"", findGame->first );

				int listRow = listReport->GetNumElements() - 1;

				listReport->SetElementText( gameInfo.Name, (int)LIST_COLUMNS_GAME, listRow );

				if ( ! gameInfo.Version.same_no_case( SysInfo::GetExeFileVersionText( gpversion::MODE_AUTO_LONG ) ) )
				{
					listReport->SetElementColor( 0xffff0000, (int)LIST_COLUMNS_GAME, listReport->GetNumElements() - 1 );
				}

				if ( gameInfo.flags & ZONE_GAME_PASSWORD_PROTECTED )
				{
					listReport->SetElementIcon( findGame->first, ICON_PASSWORD );
				}

				listReport->SetElementText( gpwstringf( L"%d/%d", gameInfo.numPlayers, gameInfo.maxPlayers ), 1, listRow );
				
				listReport->SetElementText( gameInfo.mapName , (int)LIST_COLUMNS_MAP, listRow );
				listReport->SetElementText( gameInfo.worldLevel , (int)LIST_COLUMNS_DIFFICULTY, listRow );

				if ( gameInfo.flags & ZONE_GAME_LOADING )
				{
					listReport->SetElementText( gpwtranslate( $MSG$ "Loading" ), (int)LIST_COLUMNS_GAME_TIME, listRow );
				}
				else if ( gameInfo.flags & ZONE_GAME_IN_PROGRESS )
				{
					int totalGameMinutes = (int) ( ToLocalTime( gameInfo.gameTime) )/60;
					int hours = totalGameMinutes / 60;
					int minutes = totalGameMinutes % 60;

					listReport->SetElementText( gpwstringf( L"%.2d:%.2d", hours, minutes ), (int)LIST_COLUMNS_GAME_TIME, listRow );
				}
				else
				{
					listReport->SetElementText( gpwtranslate( $MSG$ "Forming" ), (int)LIST_COLUMNS_GAME_TIME, listRow );
				}

				//$$pingif ( gameInfo.ipDWORD )
				//$$ping{
					//$$pinggameInfo.latency = 0;
					//$$pingm_ZonePing.Lookup( gameInfo.ipDWORD, &gameInfo.latency );

					//$$pingif ( (gameInfo.latency == 0xFFFFFFFF) )
					//$$ping{
					//$$ping	listReport->SetElementText( gpwtranslate( $MSG$ "NA" ), (int)LIST_COLUMNS_PING_TIME, listRow );
					//$$ping}
					//$$pingelse
					//$$ping{
					//$$ping	listReport->SetElementText( gpwstringf( L"%d", gameInfo.latency ), (int)LIST_COLUMNS_PING_TIME, listRow );
					//$$ping}

				//$$ping}



				if ( gameInfo.Name.same_no_case( m_SelectedGameName ) )
				{
					listReport->SetSelectedElement( listRow );
				}

				if ( listReport->GetNumElements() == listReport->GetMaxActiveElements() )
				{
					break;
				}
			}

			pRefreshText->SetVisible( false );
		}
		else
		{
			//pRefreshText->SetVisible( true );

			m_bRefreshGamesList = true;
		}
	}
	else
	{
		pRefreshText->SetVisible( true );

		m_GamesList.clear();
		m_GamesSorted.clear();

		//$$pingm_ZonePing.RemoveAll();

		m_TimeToRefreshGamesList = 0.0f;
		m_bRequestGameListCreation = false;

		m_CurrentGamePage = pageAt;
		m_CurrentGameEndPage = pageAt;
		m_CurrentRequestedGamePage = pageAt;

		if ( m_GameViewType == SHOW_FILTERED_GAMES )
		{
			m_CurrentViewId = GAMEVIEW_SORT_FILTERED;

			gpstring filteredFlags;
			filteredFlags = CreateFiltersList( );

			HRESULT hr = m_pGunBrowser->GetPage( m_CurrentViewId, pageAt, 0, filteredFlags.empty() ? NULL : ToUnicode( filteredFlags ).c_str(), NULL );
			gpassertf( !FAILED( hr ), ( "GUN : GetPage failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
		}
		else if ( m_GameViewType == SHOW_FRIENDS_GAMES )
		{
			gpwstring searchString = m_GameListSearchString;

			if ( m_GameListSort == SORT_MAP )
			{
				m_CurrentViewId = GAMEVIEW_SORT_MAP_FRIENDSONLY;
				if ( !searchString.empty() )
				{
					searchString.appendf( L"-%sz", searchString.c_str() );
				}
			}
			else if ( m_GameListSort == SORT_WORLD )
			{
				m_CurrentViewId = GAMEVIEW_SORT_WORLD_FRIENDSONLY;
				if ( !searchString.empty() )
				{
					searchString.appendf( L"-%sz", searchString.c_str() );
				}
			}
			else if ( m_GameListSort == SORT_PLAYERS )
			{
				m_CurrentViewId = GAMEVIEW_SORT_PLAYERS_FRIENDSONLY;
			}
			else if ( m_GameListSort == SORT_TIME )
			{
				m_CurrentViewId = GAMEVIEW_SORT_TIME_FRIENDSONLY;
			}
			else // SORT_GAMENAME
			{
				m_CurrentViewId = GAMEVIEW_SORT_NAME_FRIENDSONLY;
				if ( !searchString.empty() )
				{
					searchString.appendf( L"-%sz", searchString.c_str() );
				}
			}

			HRESULT hr = m_pGunBrowser->GetPage( m_CurrentViewId, pageAt, 0, searchString.empty() ? NULL : searchString.c_str(), NULL );
			gpassertf( !FAILED( hr ), ( "GUN : GetPage failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
		}
		else
		{
			gpwstring searchString = m_GameListSearchString;

			if ( m_GameListSort == SORT_MAP )
			{
				m_CurrentViewId = GAMEVIEW_SORT_MAP;
				if ( !searchString.empty() )
				{
					searchString.appendf( L"-%sz", searchString.c_str() );
				}
			}
			else if ( m_GameListSort == SORT_WORLD )
			{
				m_CurrentViewId = GAMEVIEW_SORT_WORLD;
				if ( !searchString.empty() )
				{
					searchString.appendf( L"-%sz", searchString.c_str() );
				}
			}
			else if ( m_GameListSort == SORT_PLAYERS )
			{
				m_CurrentViewId = GAMEVIEW_SORT_PLAYERS;
			}
			else if ( m_GameListSort == SORT_TIME )
			{
				m_CurrentViewId = GAMEVIEW_SORT_TIME;
			}
			else // SORT_GAMENAME
			{
				m_CurrentViewId = GAMEVIEW_SORT_NAME;
				if ( !searchString.empty() )
				{
					searchString.appendf( L"-%sz", searchString.c_str() );
				}
			}

			HRESULT hr = m_pGunBrowser->GetPage( m_CurrentViewId, pageAt, 0, searchString.empty() ? NULL : searchString.c_str(), NULL );
			gpassertf( !FAILED( hr ), ( "GUN : GetPage failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
		}
	}
}


void UIZoneMatch :: UpdateGameColumns()
{
	UIListReport * pGamesList = (UIListReport *)gUIShell.FindUIWindow( "zonematch_games_list" );
	if ( !pGamesList )
	{
		return;
	}
	
	pGamesList->SetShowColumnButtons( (m_GameViewType != SHOW_FILTERED_GAMES) );

	pGamesList->SetAllowColumnSort( 0, (m_GameViewType != SHOW_FILTERED_GAMES) );
	pGamesList->SetAllowColumnSort( 1, (m_GameViewType != SHOW_FILTERED_GAMES) );
	pGamesList->SetAllowColumnSort( 2, (m_GameViewType != SHOW_FILTERED_GAMES) );
	pGamesList->SetAllowColumnSort( 3, (m_GameViewType != SHOW_FILTERED_GAMES) );
	pGamesList->SetAllowColumnSort( 4, (m_GameViewType != SHOW_FILTERED_GAMES) );

	pGamesList->SetSortColumn( m_GameListSort );

	gpwstring columnHeaderText[] =
	{
		gpwtranslate( $MSG$ "GAME" ),
		gpwtranslate( $MSG$ "PLAYERS" ),
		gpwtranslate( $MSG$ "MAP" ),
		gpwtranslate( $MSG$ "DIFFICULTY" ),
		gpwtranslate( $MSG$ "GAME TIME" ),
		//$$pinggpwtranslate( $MSG$ "PING TIME" )
	};

	for ( int col = SORT_GAMENAME; col < SORT_TIME; ++col )
	{
		if ( (m_GameViewType != SHOW_FILTERED_GAMES) && (m_GameListSort == col) && !m_GameListSearchString.empty() )
		{
			pGamesList->SetColumnHeaderText( SORT_GAMENAME + col, gpwstringf( L"%s - \"%s\"", columnHeaderText[ col ].c_str(), m_GameListSearchString.c_str() ), 0xff95fffe );
		}
		else
		{
			pGamesList->SetColumnHeaderText( SORT_GAMENAME + col, columnHeaderText[ col ] );
		}
	}

	UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_games_list_search_name", "zonematch_games" );
	if ( pButton )
	{
		if ( ((eGameListSort)pButton->GetTag() == m_GameListSort) && !m_GameListSearchString.empty() )
		{
			pButton->LoadTexture( "b_gui_cmn_colheader-drop-hot" );
		}
		else
		{
			pButton->LoadTexture( "b_gui_cmn_colheader-drop-up" );
		}
	}
	pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_games_list_search_players", "zonematch_games" );
	if ( pButton )
	{
		if ( ((eGameListSort)pButton->GetTag() == m_GameListSort) && !m_GameListSearchString.empty() )
		{
			pButton->LoadTexture( "b_gui_cmn_colheader-drop-hot" );
		}
		else
		{
			pButton->LoadTexture( "b_gui_cmn_colheader-drop-up" );
		}
	}
	pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_games_list_search_map", "zonematch_games" );
	if ( pButton )
	{
		if ( ((eGameListSort)pButton->GetTag() == m_GameListSort) && !m_GameListSearchString.empty() )
		{
			pButton->LoadTexture( "b_gui_cmn_colheader-drop-hot" );
		}
		else
		{
			pButton->LoadTexture( "b_gui_cmn_colheader-drop-up" );
		}
	}
	pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_games_list_search_world", "zonematch_games" );
	if ( pButton )
	{
		if ( ((eGameListSort)pButton->GetTag() == m_GameListSort) && !m_GameListSearchString.empty() )
		{
			pButton->LoadTexture( "b_gui_cmn_colheader-drop-hot" );
		}
		else
		{
			pButton->LoadTexture( "b_gui_cmn_colheader-drop-up" );
		}
	}
}


void UIZoneMatch :: ShowGameDetails()
{
	GameInfoMap::iterator findGame = m_GamesList.find( m_CurrentGameRowId );
	if ( (findGame == m_GamesList.end()) || findGame->second.ipAddress.empty() )
	{
		return;
	}
 
	GameInfo & gameInfo = findGame->second;

	const bool versionsMatch = gameInfo.Version.same_no_case( SysInfo::GetExeFileVersionText( gpversion::MODE_AUTO_LONG ) );

	// Display game information
	UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "view_game_name_text" );
	if ( pTextBox )
	{
		pTextBox->SetTextColor( versionsMatch ? 0xFFFFFFFF : 0xFFFF0000 );
		pTextBox->SetText( gameInfo.Name );
	}

	UIText * pText = (UIText *)gUIShell.FindUIWindow( "game_details_players" );
	if ( pText )
	{
		pText->SetText( gpwstringf( gpwtranslate( $MSG$ "%d/%d Players" ), gameInfo.numPlayers, gameInfo.maxPlayers ) );
	}

	UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_game_details_button_join" );
	if ( pButton )
	{
		//pButton->SetEnabled( versionsMatch );
	}

	UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "game_details_settings_listbox" );
	if ( pListbox )
	{
		bool allowJip				= (gameInfo.flags & ZONE_GAME_ALLOW_JIP) != 0;
		bool isPvP					= (gameInfo.flags & ZONE_GAME_PVP_ENABLED) != 0;
		bool isTeams				= (gameInfo.flags & ZONE_GAME_TEAMS_ENABLED) != 0;
		bool newCharactersOnly		= (gameInfo.flags & ZONE_GAME_NEW_CHARACTERS_ONLY) != 0;
		bool allowStartSelection	= (gameInfo.flags & ZONE_GAME_ALLOW_START_SELCTION) != 0;
		bool allowPausing			= (gameInfo.flags & ZONE_GAME_ALLOW_PAUSING) != 0;
		bool dropEquippedOnDeath	= (gameInfo.flags & ZONE_GAME_DROP_EQUIPPED_ON_DEATH) != 0;
		bool dropInventoryOnDeath	= (gameInfo.flags & ZONE_GAME_DROP_INVENTORY_ON_DEATH) != 0;
		bool passwordRequired		= (gameInfo.flags & ZONE_GAME_PASSWORD_PROTECTED) != 0;

		gpwstring dropOnDeath;
		if ( !dropEquippedOnDeath && !dropInventoryOnDeath )
		{
			dropOnDeath = gpwtranslate( $MSG$ "Nothing" );
		}
		else if ( dropEquippedOnDeath && !dropInventoryOnDeath )
		{
			dropOnDeath = gpwtranslate( $MSG$ "Equipped Items" );
		}
		else if ( !dropEquippedOnDeath && dropInventoryOnDeath )
		{
			dropOnDeath = gpwtranslate( $MSG$ "Inventory" );
		}
		else
		{
			dropOnDeath = gpwtranslate( $MSG$ "All" );
		}

		int tag = 0;
		pListbox->RemoveAllElements();

		if ( (gameInfo.flags & ZONE_GAME_IN_PROGRESS) != 0 )
		{
			int totalGameMinutes = (int) ( ToLocalTime( gameInfo.gameTime ) )/60; 

			pListbox->AddElement( gpwstringf( L"%s:  %s", gpwtranslate( $MSG$ "Time Elapsed" ).c_str(), gUIMultiplayer.ConstructTimeString( totalGameMinutes ).c_str() ), tag++ );
		}

		pListbox->AddElement( gpwstringf( L"%s:  %s", gpwtranslate( $MSG$ "Map" ).c_str(), gameInfo.mapName.c_str() ), tag++ );
		pListbox->AddElement( gpwstringf( L"%s:  %s", gpwtranslate( $MSG$ "World Difficulty" ).c_str(),  gameInfo.worldLevel.c_str() ), tag++ );
		pListbox->AddElement( gpwstringf( L"%s:  %s", gpwtranslate( $MSG$ "Difficulty" ).c_str(), ReportSys::TranslateW( gameInfo.difficulty ).c_str() ), tag++ );
		pListbox->AddElement( gpwstringf( L"%s:  %s", gpwtranslate( $MSG$ "Time Limit" ).c_str(), gUIMultiplayer.ConstructTimeString( gameInfo.timeLimit ).c_str() ), tag++ );
		pListbox->AddElement( gpwstringf( L"%s:  %s", gpwtranslate( $MSG$ "Drop on Death" ).c_str(), dropOnDeath.c_str() ), tag++ );
		pListbox->AddElement( gpwstringf( L"%s:  %s", gpwtranslate( $MSG$ "Team Play" ).c_str(), isTeams ? gpwtranslate( $MSG$ "Yes" ).c_str() : gpwtranslate( $MSG$ "No" ).c_str() ), tag++ );
		pListbox->AddElement( gpwstringf( L"%s:  %s", gpwtranslate( $MSG$ "PvP" ).c_str(), isPvP ? gpwtranslate( $MSG$ "Yes" ).c_str() : gpwtranslate( $MSG$ "No" ).c_str() ), tag++ );
		pListbox->AddElement( gpwstringf( L"%s:  %s", gpwtranslate( $MSG$ "Select Locations" ).c_str(), allowStartSelection ? gpwtranslate( $MSG$ "Yes" ).c_str() : gpwtranslate( $MSG$ "No" ).c_str() ), tag++ );
		pListbox->AddElement( gpwstringf( L"%s:  %s", gpwtranslate( $MSG$ "New Characters Only" ).c_str(), newCharactersOnly ? gpwtranslate( $MSG$ "Yes" ).c_str() : gpwtranslate( $MSG$ "No" ).c_str() ), tag++ );
		pListbox->AddElement( gpwstringf( L"%s:  %s", gpwtranslate( $MSG$ "Pausing" ).c_str(), allowPausing ? gpwtranslate( $MSG$ "Yes" ).c_str() : gpwtranslate( $MSG$ "No" ).c_str() ), tag++ );
		pListbox->AddElement( gpwstringf( L"%s:  %s", gpwtranslate( $MSG$ "Join In Progress Enabled" ).c_str(), allowJip ? gpwtranslate( $MSG$ "Yes" ).c_str() : gpwtranslate( $MSG$ "No" ).c_str() ), tag++ );
		pListbox->AddElement( gpwstringf( L"%s:  %s", gpwtranslate( $MSG$ "Password Required" ).c_str(), passwordRequired ? gpwtranslate( $MSG$ "Yes" ).c_str() : gpwtranslate( $MSG$ "No" ).c_str() ), tag++ );
		pListbox->DeselectElements();

		for ( int i = 0; i < tag; ++i )
		{
			pListbox->SetElementSelectable( i, false );
		}
	}

	// Get player listing

	UIListReport * listReport = (UIListReport *)gUIShell.FindUIWindow( "zonematch_game_player_list" );
	if ( listReport )
	{
		listReport->RemoveAllElements();
		listReport->RemoveAllColumns();
		listReport->InsertColumn( gpwtranslate( $MSG$ "PLAYER" ), 120 );
		if ( (gameInfo.flags & ZONE_GAME_TEAMS_ENABLED) != 0 )
		{
			listReport->InsertColumn( gpwtranslate( $MSG$ "TEAM" ), 60 );
		}
		listReport->InsertColumn( gpwtranslate( $MSG$ "CHARACTER" ), 120 );

		listReport->InsertColumn( gpwtranslate( $MSG$ "LEVEL" ), 60 );
	}

	HRESULT hr = m_pGunBrowser->GetPage( GAMEVIEW_GAME_PLAYERS, 0, 0, gpwstringf( L"%d", m_CurrentGameRowId ).c_str(), MM_REFRESH_GAME_DETAILS );
	gpassertf( !FAILED( hr ), ( "GUN : GetPage failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
}


bool UIZoneMatch :: HostGame( gpwstring & gameName )
{
	HRESULT hr = m_pGunHost->CreateGame( gameName.c_str() );
	gpassertf( !FAILED( hr ), ( "GUN : CreateGame failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	if ( FAILED( hr ) )
	{
		gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "The game could not be created." ) );
		return false;
	}

	// Send local ip
	gpwstring localIp = ToUnicode( SysInfo::GetIPAddress() );
	stringtool::RemoveBorderingWhiteSpace( localIp );
	hr = m_pGunHost->SetItemValue( 0, "Ip2", localIp.c_str() );
	gpassertf( !FAILED( hr ), ( "GUN : SetItemValue failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	// Send exe checksum
	hr = m_pGunHost->SetItemValue( 0, "Misc1", gpwstringf( L"%d", gAppModule.GetAppCrc32() ).c_str() );
	gpassertf( !FAILED( hr ), ( "GUN : SetItemValue failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	// Init
	m_CurrentGame = gameName;
	m_bHostingGame = true;
	m_GamePlayers.clear();
	m_GameFlags = 0;

	//$$pingm_ZonePing.StartupServer();

	return true;
}


void UIZoneMatch :: OnEnterStagingArea()
{
	// Update user state
	// if don't want messages, set us to do not disturb.  
	// this way we won't be able to send messages to people who don't want them
	UpdateCurrentUserStatus( );

	// Leave chat rooms
	if ( !m_CurrentChannel.empty() )
	{
		HRESULT hr = m_pGunChat->LeaveRoom( m_CurrentRoomId, NULL );
		gpassertf( !FAILED( hr ), ( "GUN : LeaveRoom failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
	}

	// Stop pinging games list
	//$$pingm_ZonePing.RemoveAll();
	//$$pingm_ZonePing.Shutdown();
}


void UIZoneMatch :: OnExitStagingArea()
{
	OnEndGame();

	m_TimeToRefreshGamesList = 3.0f;
	m_bRefreshGamesList = true;

	gUIShell.ShowInterface( "zonematch" );

	if ( m_ConnectionStatus == GUN_CONNECTED )
	{
		// Update user state
		SetCurrentUserStatus( LOGGEDIN );
		UpdateCurrentUserStatus( );

		ShowGamesTab();

	}
	else
	{
		ShowNewsTab();
	}

	//$$$jt check for old messages here
	// for display later
	m_bFromInGame = true;
}


void UIZoneMatch :: OnBeginGame()
{
	m_GameStartTime = ToGunTime( GetLocalTimeInSeconds() ); 

	m_LastRefreshedGameTime = 0.0;

	if ( m_bHostingGame )
	{
		// update our host flags to signify that we are in the game and are loading it up.
		HostUpdateGameFlag( ZONE_GAME_LOADING, true );
		HostUpdateGameFlag( ZONE_GAME_IN_PROGRESS, true );
		HostUpdateGameSetting( ZONE_GAME_TIME, gpwstringf( L"%d", m_GameStartTime ) );
	}

	//$$pingm_ZonePing.Shutdown();

	if ( !gServer.GetAllowJIP() )
	{
		OnInGameDisableJIP();
	}

	SetCurrentUserStatus( INGAME );
	UpdateCurrentUserStatus( );
}


void UIZoneMatch :: OnEndGame()
{
	if ( m_bHostingGame )
	{
		//$$pingm_ZonePing.Shutdown();

		HRESULT hr = m_pGunHost->EndGame();
		gpassertf( !FAILED( hr ), ( "GUN : EndGame failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
		m_bHostingGame = false;
	}

	//$$$jt check for old messages here
	// for display later
	m_bFromInGame = true;

	//$$pingm_ZonePing.StartupServer();
	//$$pingm_ZonePing.StartupClient( ZONE_PING_INTERVAL );
}


void UIZoneMatch :: OnInGameEnableJIP()
{
	if ( gServer.IsLocal() )
	{
		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
		if ( session )
		{
			m_bHostingGame = true;

			HRESULT hr = m_pGunHost->CreateGame( session->GetSessionName().c_str() );
			gpassertf( !FAILED( hr ), ( "GUN : CreateGame failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

			HostUpdateGameSettings();

			m_GamePlayers.clear();
			HostRefreshPlayerList();
		}
	}
}


void UIZoneMatch :: OnInGameDisableJIP()
{
	if ( gServer.IsLocal() )
	{
		HRESULT hr = m_pGunHost->EndGame();
		gpassertf( !FAILED( hr ), ( "GUN : EndGame failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

		m_bHostingGame = false;
	}
}


void UIZoneMatch :: ShowDialog( eDialogs dialogType, gpwstring text )
{
	m_CurrentDialog = dialogType;

	gUIShell.ShowInterface( "zonematch_dialog" );

	UITextBox *textBox = (UITextBox *)gUIShell.FindUIWindow( "zonematch_dialog_text_box" );
	if ( textBox )
	{
		textBox->SetText( text );
	}

	bool bYesNoDialog = true;

	if ( (dialogType == DIALOG_OK ) ||
		 (dialogType == DIALOG_INSTALL_GAME_EXIT_NOW) ||
		 (dialogType == DIALOG_CLIENT_UPDATE_SUCCESS) )
	{
		bYesNoDialog = false;
	}

	UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_button_yes", "zonematch_dialog" );
	if ( pButton )
	{
		pButton->SetVisible( bYesNoDialog );
	}
	pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_button_no", "zonematch_dialog" );
	if ( pButton )
	{
		pButton->SetVisible( bYesNoDialog );
	}
	pButton = (UIButton *)gUIShell.FindUIWindow( "zonematch_button_ok", "zonematch_dialog" );
	if ( pButton )
	{
		pButton->SetVisible( !bYesNoDialog );
	}

	m_bShowDialog = true;
}


void UIZoneMatch :: ShowInputDialog( eDialogs dialogType, gpwstring title, gpwstring text )
{
	UIEditBox *editBox = (UIEditBox *)gUIShell.FindUIWindow( "input_dialog_edit_box" );
	if ( editBox )
	{
		editBox->EnableIme( dialogType == DIALOG_JOIN_PASSWORD_CHATROOM ? false : true );		
		editBox->SetText( L"" );
		editBox->SetAllowInput( true );

		if ( dialogType == DIALOG_CHAT_CHANGE_TOPIC )
		{
			editBox->SetMaxStringSize( 256 );
		}
		else if ( dialogType == DIALOG_ADD_FRIEND )
		{
			editBox->SetMaxStringSize( 20 );
		}
		else
		{
			editBox->SetMaxStringSize( 32 );
		}

	}

	UIText *pText = (UIText *)gUIShell.FindUIWindow( "input_dialog_title" );
	if ( pText )
	{
		pText->SetText( title );
	}
	pText = (UIText *)gUIShell.FindUIWindow( "input_dialog_text" );
	if ( pText )
	{
		pText->SetText( text );
	}

	m_CurrentDialog = dialogType;

	gUIShell.ShowInterface( "zonematch_input_dialog" );
}


void UIZoneMatch :: ShowPrivateChatWindow( gpwstring userName, bool chatEnabled )
{
	gUIShell.ShowInterface( "zonematch_private_chat" );

	GUInterface * pInterface = gUIShell.FindInterface( "zonematch" );
	if ( pInterface )
	{
		pInterface->bEnabled = false;
	}
	pInterface = gUIShell.FindInterface( "zonematch_games" );
	if ( pInterface )
	{
		pInterface->bEnabled = false;
	}
	pInterface = gUIShell.FindInterface( "zonematch_game_details" );
	if ( pInterface )
	{
		pInterface->bEnabled = false;
	}
	pInterface = gUIShell.FindInterface( "zonematch_news" );
	if ( pInterface )
	{
		pInterface->bEnabled = false;
	}
	pInterface = gUIShell.FindInterface( "zonematch_chat" );
	if ( pInterface )
	{
		pInterface->bEnabled = false;
	}

	m_ActiveChatUser = userName;

	UIText * pText = (UIText *)gUIShell.FindUIWindow( "title", "zonematch_private_chat" );
	if ( pText )
	{
		pText->SetText( gpwstringf( gpwtranslate( $MSG$ "Chatting with %s" ), m_ActiveChatUser.c_str() ) );
	}

	UIChatBox * pChatBox = (UIChatBox *)gUIShell.FindUIWindow( "private_chat_box", "zonematch_private_chat" );
	if ( pChatBox )
	{
		pChatBox->Clear();

		ChatLogMap::iterator findChat = m_ChatLogs.find( m_ActiveChatUser );
		if ( findChat != m_ChatLogs.end() )
		{
			m_pActiveChatLog = &findChat->second;

			for ( UserChatLog::iterator iLog = findChat->second.begin(); iLog != findChat->second.end(); ++iLog )
			{
				if ( iLog->first == MSG_FROM )
				{
					gpwstring text;
					if( ! gUIEmotes.PrepareStringTextEmotes( iLog->second, text, findChat->first.c_str() /*playerName*/, gpwtranslate( $MSG$ "you" ) /*targetName*/, UI_TEXT_EMOTE_TYPE_INDIVIDUAL ) )
					{
						text.assignf( gpwtranslate( $MSG$ "%s: %s" ), findChat->first.c_str(), iLog->second.c_str() );
					}
					pChatBox->SubmitText( text, 0xffffffff );
				}
				else
				{
					gpwstring text;
					if( ! gUIEmotes.PrepareStringTextEmotes( iLog->second, text, m_Username.c_str() /*playerName*/, gpwtranslate( $MSG$ "you" ) /*targetName*/, UI_TEXT_EMOTE_TYPE_INDIVIDUAL ) )
					{
						text.assignf( gpwtranslate( $MSG$ "%s: %s" ), m_Username.c_str(), iLog->second.c_str() );
					}
					pChatBox->SubmitText( text, 0xffffffff );
				}
			}
		}
		else
		{
			m_pActiveChatLog = &m_ChatLogs.insert( ChatLogMap::value_type( m_ActiveChatUser, UserChatLog() ) ).first->second;
		}
	}

	UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "private_chat_edit_box" );
	if ( pEditBox )
	{
		pEditBox->SetEnabled( chatEnabled );
		if ( chatEnabled )
		{
			pEditBox->SetText( L"" );
		}
		else
		{
			pEditBox->SetText( gpwtranslate( $MSG$ "User is not currently active." ) );
		}
	}

	UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "private_chat_send" );
	if ( pButton )
	{
		pButton->SetVisible( chatEnabled );
	}
}


void UIZoneMatch :: HidePrivateChatWindow()
{
	m_ActiveChatUser.clear();
	m_pActiveChatLog = NULL;

	gUIShell.HideInterface( "zonematch_private_chat" );

	GUInterface * pInterface = gUIShell.FindInterface( "zonematch" );
	if ( pInterface )
	{
		pInterface->bEnabled = true;
	}
	pInterface = gUIShell.FindInterface( "zonematch_games" );
	if ( pInterface )
	{
		pInterface->bEnabled = true;
	}
	pInterface = gUIShell.FindInterface( "zonematch_game_details" );
	if ( pInterface )
	{
		pInterface->bEnabled = true;
	}
	pInterface = gUIShell.FindInterface( "zonematch_news" );
	if ( pInterface )
	{
		pInterface->bEnabled = true;
	}
	pInterface = gUIShell.FindInterface( "zonematch_chat" );
	if ( pInterface )
	{
		pInterface->bEnabled = true;
	}
}


void UIZoneMatch :: SetCurrentUserStatus( DWORD status )
{
	if ( m_ConnectionStatus != GUN_CONNECTED )
	{
		return;
	}

	m_UserStatus = (eUserStatus) status;

	gpwstring sLocation;

	if ( status == INGAME || status == DND || status == FRIENDS_ONLY )
	{
		sLocation.assignf( L"GAME=%s", m_CurrentGame.c_str() );
	}
	else if ( status == INCHAT )
	{
		sLocation.assignf( L"CHAT=%s", m_CurrentChannel.c_str() );
	}
	else if ( status == INSTAGING )
	{
		sLocation.assignf( L"STAGING AREA=%s", m_CurrentGame.c_str() );
	}
	else
	{
		sLocation = L"BROWSE";
	}

	// Update user state
	HRESULT hr = m_pGunFriends->SetLocation( status, sLocation.c_str(), NULL );
	gpassertf( !FAILED( hr ), ( "GUN : SetLocation failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
}

// used to set the message option. this will automatically set
// the correct user status.
void UIZoneMatch :: UpdateCurrentUserStatus(  )
{
	if ( m_inGameMessageOptions == ZONE_MESSAGE_OPTION_NOONE && ( IsInStagingArea(gWorldState.GetCurrentState()) || IsInGame(gWorldState.GetCurrentState()) ) )
	{
		SetCurrentUserStatus( DND );
	}
	else if ( m_inGameMessageOptions == ZONE_MESSAGE_OPTION_FRIENDS && ( IsInStagingArea(gWorldState.GetCurrentState()) || IsInGame(gWorldState.GetCurrentState()) ) )
	{
		SetCurrentUserStatus( FRIENDS_ONLY );
	}
	else if( IsInGame(gWorldState.GetCurrentState()) || IsInGame(gWorldState.GetPendingState()) )
	{
		SetCurrentUserStatus( INGAME );
	}
	else if ( IsInStagingArea(gWorldState.GetCurrentState()) )
	{
		SetCurrentUserStatus( INSTAGING );
	}
	else if( ! m_CurrentChannel.empty() )
	{
		SetCurrentUserStatus( INCHAT );
	}
	else
	{
		SetCurrentUserStatus( LOGGEDIN );
	}	

}

void UIZoneMatch :: HostUpdateGameSetting( eZoneGameSetting setting, const gpwstring & value )
{
	HRESULT hr = m_pGunHost->SetItemValue( 0, ToString( setting ), value.c_str() );
	gpassertf( !FAILED( hr ), ( "GUN : SetItemValue failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
}


void UIZoneMatch :: HostUpdateGameFlag( eZoneGameFlag flag, bool value )
{
	m_GameFlags &= ~flag;
	if ( value )
	{
		m_GameFlags |= flag;
	}

	// Update all flags
	gpwstring wsFlags;
	wsFlags.resize( 20 );
	_ultow( m_GameFlags, wsFlags.begin_split(), 16 );

	HRESULT hr = m_pGunHost->SetItemValue( 0, "Flags", wsFlags.c_str() );
	gpassertf( !FAILED( hr ), ( "GUN : SetItemValue failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	// Update sort flags
	DWORD sortFlags = (m_GameFlags & ZONE_GAME_IN_PROGRESS)
					| (m_GameFlags & ZONE_GAME_PVP_ENABLED)
					| (m_GameFlags & ZONE_GAME_PASSWORD_PROTECTED)
					| (m_GameFlags & ZONE_GAME_HAS_UBER_CHARACTERS_10)
					| (m_GameFlags & ZONE_GAME_HAS_UBER_CHARACTERS_25)
					| (m_GameFlags & ZONE_GAME_HAS_UBER_CHARACTERS_50)
					| (m_GameFlags & ZONE_GAME_HAS_UBER_CHARACTERS_100)
					| (m_GameFlags & ZONE_GAME_HAS_UBER_CHARACTERS_MAX)
					| (m_GameFlags & ZONE_GAME_FULL)
					| (m_GameFlags & ZONE_GAME_TEAMS_ENABLED)
					// doesn't fit into the gun filter list. | (m_GameFlags & ZONE_GAME_NEW_CHARACTERS_ONLY)
					| (m_GameFlags & ZONE_GAME_DROP_ANYTHING_ON_DEATH)
					// doesn't fit into the gun filter list. | (m_GameFlags & ZONE_GAME_DROP_EQUIPPED_ON_DEATH)
					// doesn't fit into the gun filter list. | (m_GameFlags & ZONE_GAME_DROP_INVENTORY_ON_DEATH)
					| (m_GameFlags & ZONE_GAME_WORLD_REGULAR)
					| (m_GameFlags & ZONE_GAME_WORLD_VETERAN)
					| (m_GameFlags & ZONE_GAME_WORLD_ELITE);

	_ultow( sortFlags, wsFlags.begin_split(), 10 );
	
#if !GP_RETAIL
	gpgenericf( ("Host flag set to: %d \n", sortFlags) );
#endif

	hr = m_pGunHost->SetItemValue( 0, "SFlags", wsFlags.c_str() );
	gpassertf( !FAILED( hr ), ( "GUN : SetItemValue failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
}


void UIZoneMatch :: HostUpdateGameSettings()
{
	gUIZoneMatch.HostUpdateGameSetting( ZONE_GAME_MAP, gWorldMap.GetMapScreenName() );
	gUIZoneMatch.HostUpdateGameSetting( ZONE_GAME_MAX_PLAYERS, gpwstringf( L"%d", gServer.GetMaxPlayers() ) );
	gUIZoneMatch.HostUpdateGameSetting( ZONE_GAME_DIFFICULTY, gpwstringf( L"%d", (int)gWorldOptions.GetDifficulty() ) );
	gUIZoneMatch.HostUpdateGameSetting( ZONE_GAME_TIME_LIMIT, gpwstringf( L"%d", (int)(gVictory.GetTimeLimit() / 60) ) );
	gUIZoneMatch.HostUpdateGameSetting( ZONE_GAME_TIME, gpwstringf( L"%d", m_GameStartTime ) );

	MpWorldColl worlds;
	gWorldMap.GetMpWorlds( worlds );
	for ( MpWorldColl::iterator i = worlds.begin(); i != worlds.end(); ++i )
	{
		if ( i->m_Name.same_no_case( gWorldMap.GetMpWorldName() ) )
		{
			gUIZoneMatch.HostUpdateGameSetting( ZONE_GAME_WORLD_LEVEL, i->m_ScreenName );
			UpdateHostWorldFlags( i->m_ScreenName );
			break;
		}
	}

	gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_IN_PROGRESS, IsInGame( gWorldState.GetCurrentState() ) );
	gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_ALLOW_JIP, gServer.GetAllowJIP() );
	gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_ALLOW_START_SELCTION, gServer.GetAllowStartLocationSelection() );
	gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_ALLOW_PAUSING, gServer.GetAllowPausing() );
	gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_NEW_CHARACTERS_ONLY, gServer.GetAllowNewCharactersOnly() );
	gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_PVP_ENABLED, gVictory.GetGameType()->GetName().same_no_case( "pvp_no_teams" ) || gVictory.GetGameType()->GetName().same_no_case( "pvp_teams" ) );
	gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_TEAMS_ENABLED, gVictory.GetGameType()->GetName().same_no_case( "coop_teams" ) || gVictory.GetGameType()->GetName().same_no_case( "pvp_teams" ) );
	gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_DROP_EQUIPPED_ON_DEATH, (gServer.GetDropInvOption() == DIO_EQUIPPED) || (gServer.GetDropInvOption() == DIO_ALL) );
	gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_DROP_INVENTORY_ON_DEATH, (gServer.GetDropInvOption() == DIO_INVENTORY) || (gServer.GetDropInvOption() == DIO_ALL) );
	gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_DROP_ANYTHING_ON_DEATH, (gServer.GetDropInvOption() == DIO_ALL || gServer.GetDropInvOption() == DIO_INVENTORY || gServer.GetDropInvOption() == DIO_EQUIPPED) );
}


void UIZoneMatch :: HostRefreshPlayerList()
{
	{for ( PlayerInfoMap::iterator i = m_GamePlayers.begin(); i != m_GamePlayers.end(); ++i )
	{
		i->second.bRefreshed = false;
	}}

	unsigned int playerCount = 0;

	{for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
	{
		Player * player = *i;

		if ( IsValid( player ) && !player->IsComputerPlayer() && !player->IsMarkedForDeletion() )
		{
			PlayerInfoMap::iterator findPlayer = m_GamePlayers.find( player->GetId() );
			if ( findPlayer == m_GamePlayers.end() )
			{
				m_pGunHost->AddPlayer( MakeInt( player->GetId() ), player->GetName().c_str() );

				PlayerInfo info;
				info.bRefreshed = true;
				info.Character = player->GetHeroName();
				info.Team = gpwstringf( L"%d", player->GetTeam() + 1 );
				info.Level = gpwstringf( L"%d", (int)player->GetHeroUberLevel() );

				m_GamePlayers.insert( PlayerInfoMap::value_type( player->GetId(), info ) );

				m_pGunHost->SetItemValue( MakeInt( player->GetId() ), "PChar", info.Character.c_str() );
				m_pGunHost->SetItemValue( MakeInt( player->GetId() ), "PTeam", info.Team.c_str() );
				m_pGunHost->SetItemValue( MakeInt( player->GetId() ), "PLev", info.Level.c_str() );
			}
			else
			{
				findPlayer->second.bRefreshed = true;
				findPlayer->second.Character = player->GetHeroName();
				findPlayer->second.Team = gpwstringf( L"%d", player->GetTeam() + 1 );
				findPlayer->second.Level = gpwstringf( L"%d", (int)player->GetHeroUberLevel() );
				m_pGunHost->SetItemValue( MakeInt( (*i)->GetId() ), "PChar", findPlayer->second.Character.c_str() );
				m_pGunHost->SetItemValue( MakeInt( (*i)->GetId() ), "PTeam", findPlayer->second.Team.c_str() );
				m_pGunHost->SetItemValue( MakeInt( (*i)->GetId() ), "PLev", findPlayer->second.Level.c_str() );
			}

			++playerCount;
		}
	}}

	{
		PlayerInfoMap::iterator i = m_GamePlayers.begin();
		while ( i != m_GamePlayers.end() )
		{
			if ( i->second.bRefreshed )
			{
				++i;
			}
			else
			{
				m_pGunHost->DeletePlayer( MakeInt( i->first ) );
				i = m_GamePlayers.erase( i );
			}
		}
	}

	HostUpdateGameFlag( ZONE_GAME_FULL, playerCount == gServer.GetMaxPlayers() );
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoUpdate

bool UIZoneMatch :: IsClientUpdateAvailable()
{
	gpassert( m_pUpdateClient == NULL );

	m_pUpdateClient = CreateUpdateClientObject();
	gpgeneric( "CreateUpdateClientObject : IsClientUpdateAvailable\n" );

	if ( m_pUpdateClient == NULL )
	{
		ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "Microsoft Games Update Object not Registered. Please re-install." ) );
		return false;
	}

	m_pUpdateClient->ConfigureConnectionSettings(
							(char *)m_AutoUpdateProxy.c_str(), 80, 
							(char *)m_AutoUpdateServer.c_str(), 80,
							LocMgr::GetDefaultLanguage() );

	m_ClientUpdatePackageId = 0;

	m_CheckClientUpdateAvailable = CreateEvent( NULL, FALSE, FALSE, NULL );

	if ( m_pUpdateClient->IsClientUpdateAvailable( &m_ClientUpdatePackageId, m_CheckClientUpdateAvailable ) != S_OK )
	{
		m_pUpdateClient->Release();
		m_pUpdateClient = NULL;
		return false;
	}

	// call succeeded, now wait for m_CheckClientUpdateAvailable
	return true;
}

bool UIZoneMatch :: CheckForClientUpdate()
{
	if ( WAIT_OBJECT_0 == WaitForSingleObject( m_CheckClientUpdateAvailable, 0 ) )
	{
		m_AutoUpdateStatus = UPDATE_NONE;

		HRESULT result = m_pUpdateClient->GetLastHResult();

		m_pUpdateClient->Release();
		m_pUpdateClient = NULL;

		CloseHandle( m_CheckClientUpdateAvailable );

		gUIMultiplayer.HideMessageBox();

		if ( result == UC_E_CLIENT_UPDATE_AVAILABLE )
		{
			ShowDialog( DIALOG_INSTALL_CLIENT_UPDATE, gpwtranslate( $MSG$ "A new version of the Game Update software is available. Do you want to install this now?" ) );
		}
		else
		{
			CheckForGameUpdate();
		}

		return true;
	}

	return false;
}


bool UIZoneMatch :: DownloadClientUpdate()
{	
	gpassert( m_pUpdateClient == NULL );

	m_pUpdateClient = CreateUpdateClientObject();
	gpgeneric( "CreateUpdateClientObject : DownloadClientUpdate\n" );

	if ( m_pUpdateClient == NULL )
	{
		ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "Microsoft Games Update Object not Registered. Please re-install." ) );
		return false;
	}

	m_pUpdateClient->ConfigureConnectionSettings(
	                    (char *)m_AutoUpdateProxy.c_str(), 80, 
						(char *)m_AutoUpdateServer.c_str(), 80,
						LocMgr::GetDefaultLanguage() );

	m_DownloadWaitHandle = CreateEvent( NULL, FALSE, FALSE, NULL );

	HRESULT result = m_pUpdateClient->DownloadAndInstallClientUpdate( m_ClientUpdatePackageId, NULL, NULL, StatusCallback, NULL, m_DownloadWaitHandle );
	if ( result != S_OK )
	{
		if ( result == UC_E_ADMINRIGHTSREQUIRED )
		{
			gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "You must be an administrator to install this patch." ) );
		}
		gpgeneric( "AutoUpdate : Could not spin up DownloadAndInstallClientUpdate Thread\n" );
		m_pUpdateClient->Release();
		m_pUpdateClient = NULL;
		return false;
	}

	m_AutoUpdateStatus = UPDATE_DOWNLOADCLIENT;
	m_bCancelDownload = false;

	gUIShell.ShowInterface( "zonematch_download_status" );
	
	UIWindow *pProgress = gUIShell.FindUIWindow( "progress_bar", "zonematch_download_status" );
	if ( pProgress )
	{
		pProgress->SetVisible( false );
	}

	UIText *pText = (UIText *)gUIShell.FindUIWindow( "status_text", "zonematch_download_status" );
	if ( pText )
	{
		pText->SetText( gpwtranslate( $MSG$ "Connecting to AutoUpdate server..." ) );
	}

	return true;
}


void UIZoneMatch :: DownloadClientUpdateComplete()
{
	gUIShell.HideInterface( "zonematch_download_status" );

	CloseHandle( m_DownloadWaitHandle );

	HRESULT hResult = m_pUpdateClient->GetLastHResult();

	m_pUpdateClient->Release();
	m_pUpdateClient = NULL;

	if ( hResult == UC_E_UPDATING_EXIT_NOW )
	{
		if ( m_hAutoUpdateLib )
		{
			BOOL freed = FreeLibrary( m_hAutoUpdateLib );
			gpassertm( freed, "UpdateClient.dll was not unloaded." );
			m_hAutoUpdateLib = NULL;
		}

		m_AutoUpdateStatus = UPDATE_UPDATINGCLIENT;
		gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "Please wait while the Game Update software is installed..." ), MP_DIALOG_NO_BUTTON );
		return;
	}
	else if ( !m_bCancelDownload )
	{
		if ( hResult == UC_E_ADMINRIGHTSREQUIRED )
		{
			gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "You must be an administrator to install this patch." ) );
		}
		else
		{
			ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "There was a problem downloading the AutoUpdate patch." ) );
		}
	}

	m_AutoUpdateStatus = UPDATE_NONE;
}


bool UIZoneMatch :: CheckForGameUpdate()
{
	gpassert( m_pUpdateClient == NULL );

	m_pUpdateClient = CreateUpdateClientObject();
	gpgeneric( "CreateUpdateClientObject : CheckForGameUpdate\n" );

	m_AutoUpdateStatus = UPDATE_NONE;

	if ( m_pUpdateClient == NULL )
	{
		ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "Microsoft Games Update Object not Registered. Please re-install." ) );
		return false;
	}

	m_pUpdateClient->ConfigureConnectionSettings(
	                    (char *)m_AutoUpdateProxy.c_str(), 80,
						(char *)m_AutoUpdateServer.c_str(), 80,
						LocMgr::GetDefaultLanguage() );

	// set up package search criteria
	PACKAGEDESC pd;

	ZeroMemory( &pd, sizeof(PACKAGEDESC) );

	pd.dwPackageId = 0;
	pd.pszPackageVersion = NULL;
	pd.pszPackageName = NULL;

	gpstring version;
	version = SysInfo::GetExeFileVersionText( gpversion::MODE_AUTO_LONG );
	pd.pszGameVersion = version.begin_split();

	gpstring gameLang;
	gameLang.assignf( "%d", LocMgr::GetDefaultLanguage() );
	pd.pszLanguage = gameLang.begin_split();

	gpstring gameGuid;
	stringtool::ToString( gameGuid, ZONE_GAME_GUID );
	pd.pszGameGuid = gameGuid.begin_split();

	gpstring autoupdateParam = gConfig.GetString( "multiplayer\\autoupdate_param", "" );
	pd.pszGameParam = autoupdateParam.empty() ? NULL : (char *)autoupdateParam.c_str();

	m_GameUpdatePackageId = 0;

	m_CheckGameUpdateAvailable = CreateEvent( NULL, FALSE, FALSE, NULL );

	// ask the UpdateServer to enumerate the packages
	if ( S_OK != m_pUpdateClient->EnumeratePackages( &pd, PackageCallback, NULL, m_CheckGameUpdateAvailable ) )
	{
		ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "There was a problem communicating with the AutoUpdate server." ) );
		m_pUpdateClient->Release();
		m_pUpdateClient = NULL;
		return false;
	}

	m_AutoUpdateStatus = UPDATE_CHECKGAME;

	return true;
}


bool UIZoneMatch :: DownloadGameUpdate()
{
	gpassert( m_pUpdateClient == NULL );

	m_pUpdateClient = CreateUpdateClientObject();
	gpgeneric( "CreateUpdateClientObject : DownloadGameUpdate\n" );

	if ( m_pUpdateClient == NULL )
	{
		ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "Microsoft Games Update Object not Registered. Please re-install." ) );
		return false;
	}

	m_pUpdateClient->ConfigureConnectionSettings(
	                    (char *)m_AutoUpdateProxy.c_str(), 80, 
						(char *)m_AutoUpdateServer.c_str(), 80,
						LocMgr::GetDefaultLanguage() );

	// build a package description based on the packageID
	PACKAGEDESC pd;
	ZeroMemory( &pd, sizeof(PACKAGEDESC) );
	pd.dwPackageId = m_GameUpdatePackageId;

	// Select the information into the UpdateClient.Dll
	if ( S_OK != m_pUpdateClient->SelectPackage( &pd ) )
	{
		free( pd.pszGameGuid );
		free( pd.pszPackageName );
		free( pd.pszPackageVersion );
		gperror( "AutoUpdate : PackageSelect Failed" );
		m_pUpdateClient->Release();
		m_pUpdateClient = NULL;
		return false;
	}

	// UpdateClient makes a copy of these strings so we can free them here.
	free( pd.pszGameGuid );
	free( pd.pszPackageName );
	free( pd.pszPackageVersion );
	pd.pszGameGuid = NULL;
	pd.pszPackageName = NULL;
	pd.pszPackageVersion = NULL;

	// The first step in installing a patch is preparing the Package. 
	// In this method, the setup engine is downloaded then invoked to see what
	// files are necessary for the patching process. Next the actual files are
	// downloaded to a temporary location.


	// Create an event which will get signalled by PreparePackage when it is finished.
	m_DownloadWaitHandle = CreateEvent( NULL, FALSE, FALSE, NULL );

	// Start Preparing the package. This call is asynchronous.  Note: To cancel the download
	// return false in the callback.
	HRESULT result = m_pUpdateClient->PreparePackage( StatusCallback, NULL, m_DownloadWaitHandle );
	if ( result != S_OK )
	{
		if ( result == UC_E_ADMINRIGHTSREQUIRED )
		{
			gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "You must be an administrator to install this patch." ) );
		}
		gpgeneric( "AutoUpdate : PreparePackage Failed\n" );
		m_pUpdateClient->Release();
		m_pUpdateClient = NULL;
		return false;
	}

	m_AutoUpdateStatus = UPDATE_DOWNLOADGAME;
	m_bCancelDownload = false;

	gUIShell.ShowInterface( "zonematch_download_status" );
	
	UIWindow *pProgress = gUIShell.FindUIWindow( "progress_bar", "zonematch_download_status" );
	if ( pProgress )
	{
		pProgress->SetVisible( false );
	}

	UIText *pText = (UIText *)gUIShell.FindUIWindow( "status_text", "zonematch_download_status" );
	if ( pText )
	{
		pText->SetText( gpwtranslate( $MSG$ "Connecting to AutoUpdate server..." ) );
	}

	return true;
}


void UIZoneMatch :: DownloadGameUpdateComplete()
{
	gUIShell.HideInterface( "zonematch_download_status" );

	CloseHandle( m_DownloadWaitHandle );

	m_AutoUpdateStatus = UPDATE_NONE;
	
	if ( UCSTATE_PREPARED != m_pUpdateClient->GetPackageStatusPtr()->dwState )
	{
		if ( !m_bCancelDownload )
		{
			gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "There was a problem downloading the necessary files to install this package." ) );
		}
		else
		{
			gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "You cancelled the patch download. The partial download has been saved and will be resumed the next time you attempt to download the patch." ) );
		}
		m_pUpdateClient->Release();
		m_pUpdateClient = NULL;
	}
	else if ( !m_bCancelDownload )
	{
		ShowDialog( DIALOG_INSTALL_GAME_EXIT_NOW, gpwtranslate( $MSG$ "The patch has been downloaded. The game will now exit to complete the installation." ) );
	}
}


bool UIZoneMatch :: InstallGameUpdate()
{
	TCHAR szFilename[512];
	GetModuleFileName( NULL, szFilename, sizeof(szFilename) );

	TCHAR szTempPath[512];
	lstrcpy( szTempPath, m_pUpdateClient->GetPackageStatusPtr()->pszTempFolder );

	HRESULT result = m_pUpdateClient->LaunchAutoUpdateToInstallDownloadedPackage( szTempPath, szFilename, "" );
	
	m_pUpdateClient->Release();
	m_pUpdateClient = NULL;

	if ( result == UC_E_ADMINRIGHTSREQUIRED )
	{
		gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "You must be an administrator to install this patch." ) );
		return false;
	}
	else if ( result != S_OK )
	{
		gUIMultiplayer.ShowMessageBox( gpwtranslate( $MSG$ "There was a problem launching the external auto updater." ) );
		return false;
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Input interfaces

bool UIZoneMatch :: InputPressedEnter()
{
	UIWindow tempBaddy;

	if ( gUIShell.IsInterfaceVisible( "zonematch_create_chatroom" ) )
	{
		GameGUICallback( "zonematch_create_chatroom_ok", tempBaddy );
		return true;
	}
	if ( gUIShell.IsInterfaceVisible( "zonematch_change_password" ) )
	{
		GameGUICallback( "zonematch_change_password_ok", tempBaddy );
		return true;
	}

	return false;
}


bool UIZoneMatch :: InputPressedEscape()
{
	if (   gUIShell.IsInterfaceVisible( "zonematch_signin" )
		|| gUIShell.IsInterfaceVisible( "zonematch_create_account" )
		|| gUIShell.IsInterfaceVisible( "zonematch_account_settings" )
		|| gUIShell.IsInterfaceVisible( "zonematch_friends_settings" )
		|| gUIShell.IsInterfaceVisible( "zonematch_ignored_settings" )
		|| gUIShell.IsInterfaceVisible( "zonematch_change_password" )
		|| gUIShell.IsInterfaceVisible( "zonematch_create_chatroom" )
		|| gUIShell.IsInterfaceVisible( "zonematch_send_message" )
		|| gUIShell.IsInterfaceVisible( "zonematch_view_message" )
		|| gUIShell.IsInterfaceVisible( "zonematch_input_dialog" ) )
	{
		HideDialogs();
		return true;
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Misc



void UIZoneMatch :: RemoveFriend( gpwstring userName )
{
	FriendsMap::iterator findFriend = m_Friends.find( userName );
	if ( findFriend != m_Friends.end() )
	{
		m_Friends.erase( findFriend );
	}

	HRESULT hr = m_pGunFriends->FriendDelete( userName.c_str(), NULL );
	gpassertf( !FAILED( hr ), ( "GUN : FriendDelete failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );
}


void UIZoneMatch :: IgnoreUser( gpwstring userName )
{
	if ( !IsUserIgnored( userName ) )
	{
		HRESULT hr = m_pGunFriends->IgnoredAdd( userName.c_str(), NULL );
		gpassertf( !FAILED( hr ), ( "GUN : IgnoredAdd failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

		m_Ignored.push_back( userName );

		// taking this out for now.
		// gun's implementation of squelching is not independant of ignore
		// so we should just ignore them if we don't want to hear them.

		// when a user is ignored, gun adds them to the squelch list...
		//SquelchUser( userName );

		if ( !m_CurrentChannel.empty() )
		{
			UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
			if ( listBox )
			{
				for ( unsigned int i = 0; i < listBox->GetNumElements(); ++i )
				{
					if ( IsUserIgnored( listBox->GetElement( i )->lines.begin()->sText ) )
					{
						listBox->GetElement( i )->icon = ICON_CHAT_IGNORE;
					}
				}
			}
		}
		RemoveFriend( userName );
	}
}

void UIZoneMatch :: UnignoreUser( gpwstring userName )
{
	HRESULT hr = m_pGunFriends->IgnoredDelete( userName.c_str(), NULL );
	gpassertf( !FAILED( hr ), ( "GUN : IgnoredDelete failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	for ( UserColl::iterator i = m_Ignored.begin(); i != m_Ignored.end(); ++i )
	{
		if ( i->same_no_case( userName ) )
		{
			m_Ignored.erase( i );
			break;
		}
	}

	// taking this out for now.
	// gun's implementation of squelching is not independant of ignore
	// so we should just ignore them if we don't want to hear them.
	// when a user is unignored, gun removes them from the squelch list
	// so we have to update it also.
	// UnSquelchUser( userName );

	if ( !m_CurrentChannel.empty() )
	{
		UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
		if ( listBox )
		{
			for ( unsigned int i = 0; i < listBox->GetNumElements(); ++i )
			{
				if ( IsUserSquelched( listBox->GetElement( i )->lines.begin()->sText ) )
				{
					listBox->GetElement( i )->icon = ICON_CHAT_SQUELCH;
				}
				else if ( m_CurrentHostOfChannel.same_no_case( listBox->GetElement( i )->lines.begin()->sText ) )
				{
					listBox->SetElementIcon( listBox->GetElement( i )->lines.begin()->sText, ICON_CHAT_OWNER );
				}
				else if ( !IsUserIgnored( listBox->GetElement( i )->lines.begin()->sText ) )
				{
					listBox->GetElement( i )->icon = UIListbox::NO_ICON;
				}
			}
		}
	}
}

bool UIZoneMatch :: IsUserIgnored( gpwstring userName )
{
	for ( UserColl::iterator i = m_Ignored.begin(); i != m_Ignored.end(); ++i )
	{
		if ( i->same_no_case( userName ) )
		{
			return true;
		}
	}
	return false;
}

void UIZoneMatch :: SquelchUser( gpwstring userName )
{
	HRESULT hr = m_pGunChat->SquelchAdd( userName.c_str(), (VOID *)m_NextSquelchRequest );
	gpassertf( !FAILED( hr ), ( "GUN : SquelchAdd failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	m_SquelchRequests.insert( SquelchRequestMap::value_type( m_NextSquelchRequest, userName ) );
	++m_NextSquelchRequest;
}

void UIZoneMatch :: UnSquelchUser( gpwstring userName )
{
	HRESULT hr = m_pGunChat->SquelchDelete( userName.c_str(), (VOID *)m_NextSquelchRequest );
	gpassertf( !FAILED( hr ), ( "GUN : SquelchDelete failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	m_SquelchRequests.insert( SquelchRequestMap::value_type( m_NextSquelchRequest, userName ) );
	++m_NextSquelchRequest;
}


bool UIZoneMatch :: IsUserSquelched( gpwstring userName )
{
	for ( UserColl::iterator i = m_Squelches.begin(); i != m_Squelches.end(); ++i )
	{
		if ( i->same_no_case( userName ) )
		{
			return true;
		}
	}
	return false;
}

bool UIZoneMatch :: IsUserFriend( gpwstring userName )
{
	FriendsMap::iterator findFriend = m_Friends.find( userName );
	return( findFriend != m_Friends.end() );
}
	
bool UIZoneMatch :: IsFriendOnline( DWORD userMode )
{
	if( userMode == LOGGEDIN || userMode == INGAME || userMode == INCHAT || userMode == DND || userMode == FRIENDS_ONLY )
	{
		return true;
	}

	return false;
}

bool  UIZoneMatch :: SendMessageToFriend( gpwstring userName, gpwstring messageText )
{
	FriendsMap::iterator findFriend = m_Friends.find( userName );
	
	if( gUIZoneMatch.IsFriendOnline( findFriend->second ) )
	{
		stringtool::RemoveBorderingWhiteSpace( userName );

		HRESULT hr = m_pGunChat->SendToUser( userName.c_str(), messageText.c_str(), NULL );
		gpassertf( !FAILED( hr ), ( "GUN : SendToUser failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

		return( hr == S_OK );
	}

	return false;
}


bool UIZoneMatch :: AddFriendToZoneMatch( gpwstring userName )
{
	// make sure we aren't currently ignoring this user.  if we are remove them
	// from our ignore list.
	for ( UserColl::iterator findIgnored = m_Ignored.begin(); findIgnored != m_Ignored.end(); ++findIgnored )
	{
		if ( findIgnored->same_no_case( userName ) )
		{
			m_Ignored.erase( findIgnored );
			HRESULT hr = m_pGunFriends->IgnoredDelete( userName.c_str(), NULL );
			gpassertf( !FAILED( hr ), ( "GUN : IgnoredDelete failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

			if ( ! IsUserFriend( userName ) )
			{
				m_Friends.insert( FriendsMap::value_type( userName, ICON_USER_OFFLINE ) );	

				// thsi will update their status
				HRESULT hr = m_pGunFriends->WhereIs( userName, NULL );
				gpassertf( !FAILED( hr ), ( "GUN : WhereIs failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );				
			}
			RefreshFriendsList( true );
			return (hr == 0) ;
		}
	}

	HRESULT hr = m_pGunFriends->FriendAdd( userName.c_str(), (VOID *)m_NextFriendAddRequest );
	gpassertf( !FAILED( hr ), ( "GUN : FriendAdd failed. Result = %x : %s\n", hr, GetHRString( hr ) ) );

	m_FriendAddRequests.insert( FriendRequestMap::value_type( m_NextFriendAddRequest, userName ) );
	++m_NextFriendAddRequest;

	return (hr == 0) ;
}

void UIZoneMatch :: RequestChatRefresh()
{
	// request a chat refresh.  but only do it if the
	// chat is active.
	GUInterface *pInterface = gUIShell.FindInterface( "zonematch_chat" );
	if ( pInterface->bEnabled == true )
	{
		m_bRefreshChatRoomList = true;
	}
}

void UIZoneMatch :: UpdateDynamicChatToolTips()
{
	UIListbox *chatListBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
	if( chatListBox )
	{
		chatListBox->SetHelpBoxName( gpstring().assign("zonematch_help") );
		for ( unsigned int i = 0; i < chatListBox->GetNumElements(); i++ )
		{
			gpwstring message;
			gpwstring userName;
			userName.assign( chatListBox->GetElementText( i ) );

			if( !userName.empty() && !userName.same_no_case(m_Username) )
			{
				message.assignf( userName );

				if( userName.same_no_case( m_CurrentHostOfChannel ) )
				{
					message.appendf( gpwtranslate( $MSG$ " is the operator of the chat room and") );
				}

				if( IsUserFriend( userName ) )
				{
					message.appendf( gpwtranslate( $MSG$ " is currently a friend") );
				}
				else
				{
					message.appendf( gpwtranslate( $MSG$ " is not currently a friend") );
				}

				if( IsUserSquelched( userName ) )
				{
					message.appendf( gpwtranslate( $MSG$ " and is muted by you") );
				}

				if( IsUserIgnored( userName ) )
				{
					message.appendf( gpwtranslate( $MSG$ " and is ignored by you") );
				}

				chatListBox->SetElementTextToolTip( userName, message );
			}
			else if( !userName.empty() )
			{
				// make sure to clear
				chatListBox->SetElementTextToolTip( userName, message );
			}
		}
	}
}

// used to set the message option. this will automatically set
// the correct user status.
void UIZoneMatch :: SetGameMessageOptions( eInGameMessageOption option )
{
	m_inGameMessageOptions = option;
	UpdateCurrentUserStatus();

	//$$$
	//$$$ m_bDisableFriendsInGame = ! (m_inGameMessageOptions == ZONE_MESSAGE_OPTION_EVERYONE);
}

void UIZoneMatch::UpdateGameMessageOptions( const char* sInterface, const char* sGroup )
{
	UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_zmmessage", sInterface );
	if( pList )
	{
		pList->RemoveAllElements();
		pList->AddElement( gpwtranslate( $MSG$ "Everyone" ),	(int)ZONE_MESSAGE_OPTION_EVERYONE );
		pList->AddElement( gpwtranslate( $MSG$ "Friends" ),		(int)ZONE_MESSAGE_OPTION_FRIENDS );
		pList->AddElement( gpwtranslate( $MSG$ "No one" ),		(int)ZONE_MESSAGE_OPTION_NOONE );
		
		pList->SelectElement( GetGameMessageOptions() );

		UIComboBox * pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_zmmessage", sInterface );
		if ( pCombo )
		{
			pCombo->SetText( pList->GetSelectedText() );
		}			

		//  show these items...
		gUIShell.ShowGroup(sGroup, true);
		//  but not the listbox :)
		pList->SetVisible(false);
	}
}

bool UIZoneMatch::IsUserInChat( gpwstring userName )
{
	// find the user in the chat room list, getTextElement returns NULL 
	// if the element doesn't exist
	UIListbox *listBox = (UIListbox *)gUIShell.FindUIWindow( "zonematch_chatusers_list" );
	if ( listBox )
	{
		return( listBox->GetTextElement( userName ) != NULL );
	}
	return false; 
}