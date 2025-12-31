#pragma once
#include "pch.h"

#include <ZoneAsync.h>
#include <ZoneMatch.h>

class ZoneMatch
{
public:
    inline ZoneMatch();
    inline ~ZoneMatch();

    HRESULT StartConnection(
                        IN LPCSTR pszMasterServer,
                        IN const GUID &guid );
    
    bool FinishConnection( OUT HRESULT *phr );
    
    HRESULT GetHeaders( IN  IZoneMatchBrowser::TABLE table,
                        OUT LPZONEMATCHROW* ppHeaders );
    
    HRESULT StartView( IN  IZoneMatchBrowser::TABLE table,
                       IN  LPZONEMATCHROW pColumnFilter,
                       IN  LPCSTR pszRowFilter,
                       IN  IZoneMatchTableView::LPFN_ZMTABLE_DATA pfnUpdateCB,
                       IN  PVOID pContext,
                       OUT IZoneMatchTableView** ppView );
    
    HRESULT ViewPlayersOnServer(
                       IN  DWORD dwServerId,
                       IN  IZoneMatchTableView::LPFN_ZMTABLE_DATA pfnUpdateCB,
                       IN  PVOID pContext );

private:
    IZoneMatchTable* GetTable( IZoneMatchBrowser::TABLE table );

private:
    IZoneMatchBrowser* m_pBrowser;
    IZoneAsync* m_pAsync;
    IZoneMatchTable* m_pTableServer;
    IZoneMatchTable* m_pTablePlayer;

    IZoneMatchTableView* m_pViewPlayers;

    LPSTR m_pszMasterServer;
};

inline
ZoneMatch::ZoneMatch()
{
    m_pBrowser = NULL;
    m_pAsync = NULL;
    m_pTableServer = NULL;
    m_pTablePlayer = NULL;
    m_pViewPlayers = NULL;

    m_pszMasterServer = NULL;
}

inline
ZoneMatch::~ZoneMatch()
{
    if( NULL != m_pszMasterServer )
        free( m_pszMasterServer );

    if( NULL != m_pViewPlayers )
        m_pViewPlayers->Release();

    if( NULL != m_pTableServer )
        m_pTableServer->Release();

    if( NULL != m_pTablePlayer )
        m_pTablePlayer->Release();

    if( NULL != m_pAsync )
        m_pAsync->Release();

    if( NULL != m_pBrowser )
        m_pBrowser->Release();
}
