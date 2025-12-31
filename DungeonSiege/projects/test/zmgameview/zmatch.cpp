#include "pch.h"
#include "ZMatch.h"

#include "Misc.h"

#include <stdio.h>


LPCSTR pszKey_PlayerName  = "Player Name";
LPCSTR pszKey_PlayerLevel = "Level";
LPCSTR pszKey_Frags       = "Frags";
LPCSTR pszKey_ServerRID   = "Server Row ID";

       
HRESULT
ZoneMatch::StartConnection(
        LPCSTR pszMasterServer,
        const GUID &guid )
{
    HRESULT hr=S_OK;
    HRESULT hr2=S_OK;
    IUnknown* punk=NULL;

    hr = CoCreateInstance(
                CLSID_ZoneMatchBrowser,
                NULL,               // punkOuter
                CLSCTX_INPROC,
                IID_IUnknown,
                (PVOID*) &punk);
    if( FAILED( hr ) )
    {
        ErrorMessage( "CoCreateInstance", hr );
        return hr;
    }
      
    hr  = punk->QueryInterface(IID_IZoneMatchBrowser, (PVOID*)&m_pBrowser );
    hr2 = punk->QueryInterface(IID_IZoneAsync,        (PVOID*)&m_pAsync );

    // Don't need this anymore.
    punk->Release();

    if( FAILED( hr ) || FAILED( hr2 ) )
    {
        ErrorMessage( "QI Failure", hr );
        return hr;
    }

    m_pszMasterServer = strdup( pszMasterServer );
    if( NULL == m_pszMasterServer )
    {
        printf("Memory Allocation failure\n");
        return hr;
    }

    // Load the connection parameters.
    //
    hr = m_pBrowser->InitZMBrowser( m_pszMasterServer,
                                    MASTER_SERVER_PORT,
                                    guid,
                                    NULL,   // Refresh callback not implemented
                                    NULL ); // in this version of ZoneMatch.
    if( FAILED( hr ) )
    {
        ErrorMessage( "Failed to Init Browser", hr );
        return hr;
    }

    hr = m_pAsync->Start( NULL );
    if( S_OK != hr )
    {
        if( hr != ZT_E_PENDING )
        {
            ErrorMessage( "IZoneAsync::Start", hr );
            return hr;
        }
    }
    return S_OK;
}

bool
ZoneMatch::FinishConnection(
        HRESULT *phr )
{
    IZoneAsync::STATUS Status;
    LPSTR pszTitle=NULL;
    CHAR szMsg[128];
    HRESULT hr=S_OK;

    m_pAsync->GetStatus( &Status );

    // If it hasn't started up yet, then return and come back on
    // the next timer tick.
    //
    if( IZoneAsync::STATUS_STARTING == Status )
        return false;

    if( IZoneAsync::STATUS_RUNNING != Status )
    {
        hr = m_pAsync->GetLastError();
        ::ErrorMessage( "Connect Failed", hr );

        switch( hr )
        {
        case ZT_E_FAILED_TO_CONNECT:
            ::printf( "No MasterServer @ '%s'", m_pszMasterServer );
            break;

        case ZT_E_NO_QUERYSERVER:
            ::printf( szMsg, "No QueryServer for your game\n"
                             "on the MasterServer @ '%s'", m_pszMasterServer );
            break;
        }
    }
    *phr = hr;
    return true;
}

HRESULT
ZoneMatch::GetHeaders(
        IN  IZoneMatchBrowser::TABLE table,
        OUT LPZONEMATCHROW* ppHeaders )
{
    IZoneMatchTable *pTable=NULL;
    HRESULT hr=S_OK;

    pTable = GetTable( table );
    
    hr = pTable->GetAllHeaders( ppHeaders );
    if( FAILED( hr ) )
    {
        ::ErrorMessage( "IZoneMatchTable::GetAllHeaders", hr );
        return hr;
    }

    return hr;
}


HRESULT
ZoneMatch::StartView(
        IN  IZoneMatchBrowser::TABLE table,
        IN  LPZONEMATCHROW pColumnFilter,
        IN  LPCSTR pszRowFilter,
        IN  IZoneMatchTableView::LPFN_ZMTABLE_DATA pfnUpdateCB,
        IN  PVOID pContext,
        OUT IZoneMatchTableView** ppView )
{
    IZoneMatchTable *pTable=NULL;
    IZoneMatchTableView *pView=NULL;
    HRESULT hr=S_OK;

    pTable = GetTable( table );
    hr = pTable->CreateView( NULL, IID_IZoneMatchTableView,
                             pColumnFilter,
                             pszRowFilter,
                             NULL, TRUE,
                             (PVOID*) &pView );
    if( FAILED( hr ) )
    {
        ::ErrorMessage( "IZoneMatchTable::CreateView", hr );
        return hr;
    }
                              
    hr = pView->StartData( TRUE, pfnUpdateCB, pContext );
    if( FAILED( hr ) )
    {
        ::ErrorMessage( "IZoneMatchTableView::StartData", hr );
        pView->Release();
        return hr;
    }

    *ppView = pView;
    return hr;
}

HRESULT
ZoneMatch::ViewPlayersOnServer(
        DWORD dwServerId,
        IN  IZoneMatchTableView::LPFN_ZMTABLE_DATA pfnUpdateCB,
        IN  PVOID pContext )
{
    IZoneMatchBrowser::TABLE table;
    CHAR szRowFilter[128];
    LPZONEMATCHROW pColumnFilter=NULL;
    HRESULT hr=S_OK;

    if( NULL != m_pViewPlayers )
    {
        m_pViewPlayers->Release();
        m_pViewPlayers = NULL;
    }

    table = IZoneMatchBrowser::TABLE_PLAYER;
    wsprintf( szRowFilter, "%s=%d", pszKey_ServerRID, dwServerId );

    hr = GetHeaders( table, &pColumnFilter );
    if( FAILED( hr ) )
        return hr;

    hr = StartView( table,
                    pColumnFilter,
                    szRowFilter,
                    pfnUpdateCB,
                    pContext,
                    &m_pViewPlayers );
    if( FAILED( hr ) )
        return hr;

    return hr;
}


IZoneMatchTable*
ZoneMatch::GetTable(
        IZoneMatchBrowser::TABLE table )
{
    IZoneMatchTable** ppTable=NULL;
    IZoneMatchTable* pTable=NULL;
    HRESULT hr=S_OK;

    switch( table )
    {
    case IZoneMatchBrowser::TABLE_SERVER:
        ppTable = &m_pTableServer;
        break;

    case IZoneMatchBrowser::TABLE_PLAYER:
        ppTable = &m_pTablePlayer;
        break;

    default:
        printf( "Illegal table value\n" );
        return NULL;
    }

    if( NULL != *ppTable )
        return *ppTable;

    hr = m_pBrowser->CreateTable( NULL,
                                  IID_IZoneMatchTable,
                                  table,
                                  (PVOID*)&pTable );
    if( FAILED( hr ) )
    {
        ::ErrorMessage( "IZoneMatchBrowser::CreateTable", hr );
        return NULL;
    }

    *ppTable = pTable;
    return pTable;
}

