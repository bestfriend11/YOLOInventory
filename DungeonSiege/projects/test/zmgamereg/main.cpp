#include <windows.h>
#include <stdio.h>
#include <time.h>

#include <initguid.h>
#include <ZoneAsync.h>
#include <ZoneMatch.h>

#define Check(hr,str) if(FAILED(hr)){ printf("%s. hr=%x\n",str,hr); exit(0); }

void usage()
{
    printf( "zonemastersample [host]\n" );
    ExitProcess(1);
}


BOOL WINAPI cbEnumIPAddrs(
        IN LPCSTR pszGameAddr,
        IN LPVOID pContext )
{
    printf( "\tEnum'ing Game Addresses -> %s\n", pszGameAddr );
    return TRUE;
}


BOOL WINAPI cbEnumPlayers(
        IN DWORD dwPlayerID,
        IN LPSTR pszPlayerName,
        IN LPVOID pContext )
{
    //printf( "\t%d\t%s\n", dwPlayerID, pszPlayerName );
    return TRUE;
}

BOOL WINAPI cbEnumItemValues(
        IN DWORD dwItemID,
        IN LPSTR pszKey,
        IN LPSTR pszValue,
        IN LPVOID pContext )
{
    //printf( "Item #%d\t%s = %s\n", dwItemID, pszKey, pszValue );
    return TRUE;
}


GUID  guidSalmonSpawningMadness = {
            0xd5e25e3e, 0xad83, 0x4e66,
        { 0xad, 0xee, 0x4b, 0x41, 0xe9, 0xaa, 0x82, 0xd } };

#define MAX_PLAYERS 16

class CReg
{
public:
    CReg();
    HRESULT Setup( WORD wQueryPort );
    void ReleaseAll();

    HRESULT LoadAttributes();
    HRESULT Register( LPSTR pszHostName, HANDLE hEvent );
    HRESULT GetConnection();

    HRESULT CreateAPlayer();
    HRESULT DeleteAPlayer();
    HRESULT ChangeAttribute();
    HRESULT ChangeSomething();
    DWORD GetCountOfPlayers();

private:
    DWORD AllocateId();
    DWORD GetRandomId();
    DWORD FreeRandomId();

private:
    IZoneAsync* m_pZMAsync;
    IZoneMatchApplication*  m_pZMApp;
    IZoneMatchRegistration* m_pZMReg;
    bool m_bPlayersOn[ MAX_PLAYERS ];
    DWORD m_cPlayers;
};


CReg::CReg()
{
    int i;

    m_pZMAsync = NULL;
    m_pZMApp = NULL;
    m_pZMReg= NULL;
    m_cPlayers=0;
    for(i=0; i<MAX_PLAYERS; i++)
        m_bPlayersOn[ i ] = false;
}

void
Usage()
{
    printf( "GameView MasterServer [ GamePort# ]\n" );
    exit(0);
}


void _cdecl
main(int argc,char **argv)
{
    HANDLE hEvent=NULL;
    //char szHostBuf[64];
    LPSTR pszServerName=NULL;
    CReg Reg;
    WORD wQueryPort;
    HRESULT hr=S_OK;
    
    if( argc < 2 )
        Usage();
    pszServerName = argv[1];

    if( argc > 2 )
        wQueryPort = atoi( argv[2] );
    if( 0 == wQueryPort )
        Usage();

    CoInitialize(NULL);

    Reg.Setup( wQueryPort );
   
    Reg.LoadAttributes();

    hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    Reg.Register( pszServerName, hEvent );

    printf( "Waiting for connection.....\n" );
    WaitForSingleObject( hEvent, INFINITE );

    srand( (UINT)time( NULL ) );
    if( ! FAILED( Reg.GetConnection() ) )
    {
        for(;;)
        {
            Sleep( 1000*(3 + rand() % 10) );
            Reg.ChangeSomething();
        }
    }

    Reg.ReleaseAll();

    CoUninitialize();
}


HRESULT
CReg::Setup(
        WORD wQueryPort )
{
    IZoneMatchApplication::APPDESCRIPTION* pAppDesc=NULL;
    HRESULT hr=S_OK;

    //
    // Create the object.
    //
    hr = CoCreateInstance( __uuidof( CZoneMatchApplication ),
                           NULL, CLSCTX_INPROC_SERVER,
                           __uuidof( IZoneMatchApplication ),
                           (LPVOID*)&m_pZMApp );
    Check(hr, "failed to create IZoneMatchApplication.");

    //
    // This is just a test of EnumApplicationAddrs.
    //
    printf( "EnumApplicationAddrs\n" );
    hr = m_pZMApp->EnumApplicationAddrs( cbEnumIPAddrs, (LPVOID)1 );
    Check(hr, "EnumApplicationAddrs failed.");

    //
    // Initialize the Application object.
    //
    pAppDesc = new IZoneMatchApplication::APPDESCRIPTION;
    if( NULL == pAppDesc )
    {
        printf("Memory Allocation Failure\n");
        exit(0);
    }

    CopyMemory( &pAppDesc->guidApplication,
                &guidSalmonSpawningMadness, sizeof(GUID) );
    pAppDesc->pszProductName = "Salmon Spawning Maddness";
    pAppDesc->pszModName = "Standard";
    pAppDesc->pszVersion = "1.0";
    CoCreateGuid( &(pAppDesc->guidInstanceID) );
    pAppDesc->wAppServerPort = 12345;       // Cannot default this value
    pAppDesc->pszServerName = "Bear Creek";
    pAppDesc->dwMaxPlayers = 100;
    pAppDesc->bPasswordProtected = FALSE;

    hr = m_pZMApp->InitApplication( pAppDesc,
                                    NULL,  // Defaults to first local addr
                                    wQueryPort );
    Check(hr, "InitApplication Failed.");
    
    
    return hr;
}


HRESULT
CReg::LoadAttributes()
{
   // CHAR szNameBuf[64];
    int i, count;
    HRESULT hr=S_OK;

    hr = m_pZMApp->SetItemValue( SERVER_ITEM_ID, "Map Type", "Lake Sammamish" );
    Check(hr, "SetItemValue failed: 0x%x");

    count = 1+rand()%3;
    for(i=0; i < count; i++ )
    {
        hr = CreateAPlayer();
    }

    return hr;
}

LPSTR g_apszNames[] = { "Squirt", "Babble", "Freddi", "Skipper", "Floppy" };
#define NUM_NAMES (sizeof( g_apszNames) / sizeof( g_apszNames[0]) ) )

LPSTR g_apszPlayerAttrNames[] = { "Frags", "Level" };
#define NUM_PLAYERATTRS (sizeof( g_apszPlayerAttrNames)   \
                                    / sizeof( g_apszPlayerAttrNames[0] ) )

DWORD
CReg::AllocateId()
{
    DWORD dwId;

    dwId = rand() % MAX_PLAYERS;
    while( m_bPlayersOn[ dwId ] )
    {
        dwId += 1;
        dwId %= MAX_PLAYERS;
    }
    m_bPlayersOn[ dwId ] = true;
    m_cPlayers += 1;
    
    return 1+dwId;
}

DWORD
CReg::GetRandomId()
{
    DWORD dwId;

    // Pick an existing player Id at random.
    //
    dwId = rand() % MAX_PLAYERS;
    while( ! m_bPlayersOn[ dwId ] )
    {
        dwId += 1;
        dwId %= MAX_PLAYERS;
    }
    
    return 1+dwId;
}


DWORD
CReg::FreeRandomId()
{
    DWORD dwId;

    // Pick an existing player Id at random.
    //
    dwId = rand() % MAX_PLAYERS;
    while( ! m_bPlayersOn[ dwId ] )
    {
        dwId += 1;
        dwId %= MAX_PLAYERS;
    }
    m_bPlayersOn[ dwId ] = false;
    m_cPlayers -= 1;

    return 1+dwId;
}

DWORD
CReg::GetCountOfPlayers()
{
    return m_cPlayers;
}


HRESULT
CReg::CreateAPlayer()
{
    DWORD dwId, i;
    LPSTR pszAttr=NULL;
    LPSTR pszPlayerName=NULL;
    HRESULT hr=S_OK;

    if( m_cPlayers >= MAX_PLAYERS )
        return S_OK;

    dwId = AllocateId();

    pszPlayerName = g_apszNames[ rand() % 5 ];

    printf("Add Player #%d '%s'\n", dwId, pszPlayerName );
    hr = m_pZMApp->AddPlayer( dwId, pszPlayerName );
    Check(hr, "AddPlayer failed." ); 

    for(i=0; i<NUM_PLAYERATTRS; i++)
    {
        pszAttr = g_apszPlayerAttrNames[i];
        hr = m_pZMApp->SetItemValue( dwId, pszAttr, "0" );
        Check(hr, "SetItemValue (on player) failed." );
    }

    return S_OK;
}

HRESULT
CReg::DeleteAPlayer()
{
    DWORD dwId;
    HRESULT hr=S_OK;

    if( m_cPlayers == 0 )
        return S_OK;

    dwId = FreeRandomId();
    
    printf("Del Player #%d\n", dwId );
    hr = m_pZMApp->DeletePlayer( dwId );
    Check(hr, "DeletePlayer failed." ); 

    return S_OK;
}

HRESULT
CReg::ChangeAttribute()
{
    DWORD dwId, dwCount;
    LPSTR pszAttr=NULL;
    CHAR szValue[128];
    HRESULT hr=S_OK;

    if( m_cPlayers == 0 )
        return S_OK;
    
    dwId = GetRandomId();

    pszAttr = g_apszPlayerAttrNames[ rand() % NUM_PLAYERATTRS ];

    hr = m_pZMApp->GetItemValue( dwId, pszAttr, szValue, sizeof(szValue) );    
    Check(hr, "GetItemValue failed." ); 
    dwCount = atoi( szValue );
    wsprintf( szValue, "%d", dwCount+1 );

    printf("Mod Player #%d '%s'='%s'\n", dwId, pszAttr, szValue );
    
    hr = m_pZMApp->SetItemValue( dwId, pszAttr, szValue );
    Check(hr, "SetItemValue failed." ); 
    
    return S_OK;
}

HRESULT
CReg::ChangeSomething()
{
    DWORD i;

    i = rand() % 5;

    switch( i )
    {
    case 0:
        DeleteAPlayer();
        break;

    case 1:
        CreateAPlayer();
        break;

    default:
        ChangeAttribute();
        break;
    }
    printf("Total Number of Players is %d\n", GetCountOfPlayers() );
    return S_OK;
}


HRESULT
CReg::Register(
        LPSTR pszHostName,
        HANDLE hEvent )
{
    IZoneAsync* pZMRegAsync=NULL;
    HRESULT hr=S_OK;

    hr = m_pZMApp->CreateRegistration( NULL,
                                     __uuidof( IZoneMatchRegistration ),
                                     (LPVOID*)&m_pZMReg );
    Check(hr, "CreateRegistration failed.");

    hr = m_pZMReg->InitRegistration( pszHostName, MASTER_SERVER_PORT );
    Check(hr, "InitRegistration Failed");

    hr = m_pZMReg->QueryInterface( __uuidof( IZoneAsync ),
                                   (PVOID*)&pZMRegAsync );
    Check(hr, "QI For IZoneAsync Failed");

    printf( "Start..." );
    hr = pZMRegAsync->Start( hEvent );
    if( hr != ZT_E_PENDING && hr != S_OK )
        Check(hr, "Start failed.");

    printf( "Start Completed\n" );
    m_pZMAsync = pZMRegAsync;

    return hr;
}

#define ZMABUFSIZE  64

HRESULT
CReg::GetConnection()
{
    HRESULT hr=S_OK;
    HRESULT hr2=S_OK;
    time_t ConnectTime=0;
    CHAR szMasterAddress[ ZMABUFSIZE ];
    WORD wMasterPort=0;

    printf("Calling GetRegistation\n");
    hr = m_pZMReg->GetRegistration( szMasterAddress, ZMABUFSIZE,
                                    &wMasterPort, &ConnectTime );

    if( FAILED( hr ) )
    {
        printf("GetRegistration Failed ");
        if( ZT_E_INVALID_STATUS == hr )
        {
            IZoneAsync::STATUS status;

            m_pZMAsync->GetStatus( &status );
            printf("Bad Status %x, ", status );
        }
        hr2 = m_pZMAsync->GetLastError();
        printf("hr=%x, hr2=%x\n", hr, hr2);
        return hr2;
    }

    printf("GetRegistration returned %s:%d at %x\n",
                            szMasterAddress, wMasterPort, ConnectTime );

    return S_OK;

}


void
CReg::ReleaseAll()
{
    m_pZMAsync->Release();
    m_pZMReg->Release();
    m_pZMApp->Release();
}


