#include "pch.h"
#include "misc.h"

#include "ZMatch.h"
#include "TblMgr.h"

#include <stdio.h>
#include <stdlib.h>


class __declspec( uuid( "D5E25E3E-AD83-4e66-ADEE-4B41E9AA820D" ) ) CSalmon;

void __cdecl
main( unsigned int argc, char ** argv )
{
    ZoneMatch ZM;
    LPCSTR pszMasterServer = "BChapmanZ";
    CTableManager TableMgr;
    IZoneMatchTableView* pView=NULL;
    LPZONEMATCHROW pServerHeaders=NULL;
    CHAR szCmdBuf[64];
    DWORD dwServerId;
    HRESULT hr=S_OK;

    CoInitialize( NULL );

    hr = ZM.StartConnection( argv[1],
                             __uuidof( CSalmon ) );

    //
    // Wait for completion without blocking.
    //
    while( ! ZM.FinishConnection( &hr ) )
        Sleep( 200 );

    if( FAILED( hr ) )
        exit( 1 );
        
    hr = ZM.GetHeaders( IZoneMatchBrowser::TABLE_SERVER, &pServerHeaders );
    if( FAILED( hr ) )
        exit( 1 );

    hr = ZM.StartView( IZoneMatchBrowser::TABLE_SERVER,
                       pServerHeaders,
                       NULL,
                       TableMgr.ServerDataCB,
                       &TableMgr,
                       &pView );

    for(;;)
    {
        gets( szCmdBuf );
        switch( szCmdBuf[0] )
        {
        case 's':
            dwServerId = atoi( &szCmdBuf[1] );
            if( 0 == dwServerId )
                goto BadCommand;
            ZM.ViewPlayersOnServer( dwServerId,
                                    TableMgr.PlayerDataCB,
                                    &TableMgr );
            break;

        case 'q':
            break;

BadCommand:
        default:
            printf("Bad Command\n");
            break;
        }
    }
}

void
ErrorMessage(
        LPCSTR szFailed,
        HRESULT hr )
{
    PVOID pvMsg=NULL;
    char a_chBuf[64];
    DWORD cnt=0;
    HANDLE hModErr=NULL;

    hModErr = GetModuleHandle( "GunErr.dll" );

    if( NULL == hModErr )
    {
        cnt = ::FormatMessage( 
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                    FORMAT_MESSAGE_FROM_SYSTEM | 
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    hr,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                    (LPTSTR) &pvMsg,
                    0,
                    NULL );
    }
    else
    {
        cnt = ::FormatMessage( 
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                    FORMAT_MESSAGE_FROM_SYSTEM | 
                    FORMAT_MESSAGE_IGNORE_INSERTS |
                    FORMAT_MESSAGE_FROM_HMODULE,
                    hModErr,
                    hr,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                    (LPTSTR) &pvMsg,
                    0,
                    NULL );
    }

    if( 0 == cnt )
    {
        wsprintf( a_chBuf, "Error 0x%x", hr );
        pvMsg = a_chBuf;
    }

    ::printf( "%s: %s\n", (LPSTR)pvMsg, szFailed );

    if( 0 != cnt )
        ::LocalFree( pvMsg );

    if( IsDebuggerPresent() )
        ::DebugBreak();
}

LPCSTR
pszUPDATE_STATUS(
        IZoneMatchTableView::UPDATE_STATUS usRow )
{
    switch( usRow )
    {
    case IZoneMatchTableView::ROW_UPDATE:  return "Updated";
    case IZoneMatchTableView::ROW_DELETED: return "Deleted";
    case IZoneMatchTableView::ROW_STALE:   return "Stale";
    case IZoneMatchTableView::ROW_END:     return "End";
    default: return "Illegal Operation";
    }
}

