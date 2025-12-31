//-----------------------------------------------------------------------------
//    ZoneMatch.h
//
//    Copyright (c) Microsoft Corp. 1999-2000. All rights reserved.
//
//    Content: Interface for the Zone's game server directory
//
//-----------------------------------------------------------------------------

#pragma once

#include "ZoneTechErr.h"

/////////////////////////////////////////////////////////////////////////////
//  ZoneMatch Browser API.
/////////////////////////////////////////////////////////////////////////////

//
// notes:
//
// All character strings used throughout the API are zero-terminated
// String lengths up to 127 bytes are supported ( 128 including zero-terminator )
//
// All enumeration callbacks continue the enumeration if TRUE is returned.
//
//
// All port numbers are in host byte order.
//

class __declspec(
        uuid( "9BE1D721-77ED-11d3-A3CF-00C04F5F1241" ) ) CZoneMatchBrowser;

class __declspec(
        uuid( "42BA50DA-3424-11d3-BA6C-00C04F8EF92F" ) ) CZoneMatchApplication;

interface __declspec(
        uuid( "9BE1D722-77ED-11d3-A3CF-00C04F5F1241" ) ) IZoneMatchBrowser;

interface __declspec(
        uuid( "9BE1D723-77ED-11d3-A3CF-00C04F5F1241" ) ) IZoneMatchTable;

interface __declspec(
        uuid( "D809231D-5D93-4299-9D7F-C2F871BDADA3" ) ) IZoneMatchTableView;

interface __declspec(
        uuid( "9BE1D725-77ED-11d3-A3CF-00C04F5F1241" ) ) IZoneRowManager;

interface __declspec(
        uuid( "42BA50DB-3424-11d3-BA6C-00C04F8EF92F" ) ) IZoneMatchApplication;

interface __declspec(
        uuid( "42BA50DC-3424-11d3-BA6C-00C04F8EF92F" ) ) IZoneMatchRegistration;


// 
// Also allow the old name style.
//
#define CLSID_ZoneMatchBrowser      __uuidof( CZoneMatchBrowser )
#define CLSID_ZoneMatchApplication  __uuidof( CZoneMatchApplication )
#define IID_IZoneMatchBrowser       __uuidof( IZoneMatchBrowser )
#define IID_IZoneMatchTable         __uuidof( IZoneMatchTable )
#define IID_IZoneMatchTableView     __uuidof( IZoneMatchTableView )
#define IID_IZoneRowManager         __uuidof( IZoneRowManager )
#define IID_IZoneMatchApplication   __uuidof( IZoneMatchApplication )
#define IID_IZoneMatchRegistration  __uuidof( IZoneMatchRegistration )


#define MAX_ROWFILTER_LENGTH    4096

typedef class tagZoneMatchRow
{
private:
    // Use IZoneRowManager ONLY, to make rows!
    //
    tagZoneMatchRow();
    tagZoneMatchRow( const tagZoneMatchRow & );

public:
    inline DWORD Count() const
                { return cStrings; };
    inline LPCSTR Strings( DWORD i ) const
                { return apszStrings[i]; };

private:
        DWORD  cStrings;
        LPCSTR apszStrings[1];
} const *LPZONEMATCHROW;


//
// This interface is used to create, dup and Free ZONEMASTERROWS.
// It is found with IZoneMatchTable and IZoneMatchTableView.
//
interface IZoneRowManager : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE AllocateRow(
                IN DWORD cValues,
                IN LPCSTR const * apszValues,
                OUT LPZONEMATCHROW* ppRow ) = 0;

    virtual HRESULT STDMETHODCALLTYPE FreeRow(
                IN LPZONEMATCHROW pRow ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DupRow(
                IN LPZONEMATCHROW pRow,
                OUT LPZONEMATCHROW* ppRow ) = 0;
};


//
// This interface is used with IZoneAsync
//
interface IZoneMatchBrowser : public IUnknown
{
    typedef void ( WINAPI FN_ZM_REFRESH ) (
                IN PVOID pReserved,
                IN PVOID pContext );
    typedef FN_ZM_REFRESH *LPFN_ZM_REFRESH;

    virtual HRESULT STDMETHODCALLTYPE InitZMBrowser (
                IN LPCSTR pszMaster,
                IN unsigned short wPort,
                IN const GUID& guidApplication,
                IN LPFN_ZM_REFRESH pfnRefresh,
                IN PVOID pRefreshContext ) = 0;
    
    enum TABLE {
                TABLE_NONE,
                TABLE_SERVER,
                TABLE_PLAYER
    };
    virtual HRESULT STDMETHODCALLTYPE CreateTable( 
                IN LPUNKNOWN pUnkOuter,
                IN REFIID riid,
                IN TABLE table,
                OUT LPVOID* ppTableInf) = 0;

    virtual HRESULT STDMETHODCALLTYPE Refresh( ) = 0;
};


interface IZoneMatchTable : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetAllHeaders(
                OUT LPZONEMATCHROW* ppRow ) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateView(
                IN LPUNKNOWN pUnkOuter,
                IN REFIID     riid,
                IN LPZONEMATCHROW pColumnFilter,
                IN LPCSTR     pszRowFilter,
                IN LPCSTR     pszSortAttribute,
                IN BOOL       bSortAscending,
                OUT PVOID*    pView ) = 0;
};


interface IZoneMatchTableView : public IUnknown
{
    enum UPDATE_STATUS { ROW_UPDATE, ROW_DELETED,
                         ROW_STALE,  ROW_END };

    typedef void (WINAPI FN_ZMTABLE_DATA) (
                IN DWORD iRowID,
                IN UPDATE_STATUS   status,
                IN LPZONEMATCHROW pRow,
                IN PVOID pContext );
    typedef FN_ZMTABLE_DATA *LPFN_ZMTABLE_DATA;

    virtual HRESULT STDMETHODCALLTYPE SetUpdateCursorRange(
                IN DWORD dwStartingIndex,
                IN DWORD dwUpdateRange )=0;

    virtual HRESULT STDMETHODCALLTYPE StartData(
                IN BOOL bUpdate,
                IN LPFN_ZMTABLE_DATA pfnData,
                IN PVOID pContext )=0;

    virtual HRESULT STDMETHODCALLTYPE CloseData()=0;

    virtual HRESULT STDMETHODCALLTYPE GetViewSize(
                OUT PDWORD pdwSize )=0;
};


/////////////////////////////////////////////////////////////////////////////
//  ZoneMatch Registration API.
/////////////////////////////////////////////////////////////////////////////

// Port on the Master Server in the data center.
// Game Servers connect here to register.
// Game Clients connect here to browse.
//
#define MASTER_SERVER_NAME  "match.zone.com"
#define MASTER_SERVER_PORT  28805

// Port on the Game Server Application that the Query Server
// connects to and pings for registration data.
//
#define APP_QUERY_PORT      27999

//
// notes:
//
// All character strings used throughout the API are zero-terminated
// String lengths up to 127 bytes are supported ( 128 including zero-terminator )
//
// All enumeration callbacks continue the enumeration if TRUE is returned.
//
//
// All port numbers are in host byte order
//


interface IZoneMatchApplication : public IUnknown
{
    typedef BOOL ( WINAPI *LPFN_ZM_ENUM_IPADDRS )(
                        IN LPCSTR pszIpAddr,
                        IN LPVOID pContext );

    virtual HRESULT STDMETHODCALLTYPE EnumApplicationAddrs(
                        IN LPFN_ZM_ENUM_IPADDRS,
                        IN LPVOID pContext ) = 0;


    struct APPDESCRIPTION {
                GUID guidApplication;
                LPCSTR pszProductName;
                LPCSTR pszModName;
                LPCSTR pszVersion;

                GUID guidInstanceID;
                unsigned short wAppServerPort;
                LPCSTR pszServerName;
                DWORD dwMaxPlayers;
                BOOL bPasswordProtected;
    };
    typedef APPDESCRIPTION const* LPCAPPDESCRIPTION;
 
    virtual HRESULT STDMETHODCALLTYPE InitApplication(
                        IN LPCAPPDESCRIPTION pAppDesc,
                        IN LPCSTR pszAppQueryAddr,      // Defaults if == NULL
                        IN unsigned short wAppQueryPort // Defaults if = 0
                                                ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Close() = 0;

    enum QUERY_LEVEL {
                QL_NONE          =   0,
                QL_PING          =  10,
                QL_REGISTRATION  =  20,
                QL_GAME_VALUES   =  30,
                QL_PLAYERS       =  40,
                QL_PLAYER_VALUES =  50,
                QL_ALL           = 100
    };

    virtual HRESULT STDMETHODCALLTYPE SetQueryLevel(
                        IN  QUERY_LEVEL QueryLevel ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetQueryLevel(
                        OUT QUERY_LEVEL* pQueryLevel ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetMatchEncryptionKey(
                        IN LPBYTE pBuf,
                        IN DWORD cbBuf ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetGameEncryptionKey(
                        IN LPBYTE pBuf,
                        IN DWORD cbBuf ) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateRegistration(
                        IN IUnknown* pUnkOuter,
                        IN REFIID riid,
                        OUT void** ppInf ) = 0;
    
    // Managing Players.

    virtual HRESULT STDMETHODCALLTYPE AddPlayer(
                        IN DWORD dwItemID,
                        IN LPCSTR pszPlayerName ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetPlayer(
                        IN DWORD dwItemID,
                        OUT LPSTR pszPlayerName,
                        IN DWORD cbPlayerName ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DeletePlayer(
                        IN DWORD dwItemID ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetNumPlayers(
                        OUT DWORD* pdwNumPlayers ) = 0;

    typedef BOOL ( WINAPI *LPFN_ENUM_PLAYERS )(
                        IN DWORD dwItemID,
                        IN LPCSTR pszPlayerName,
                        IN LPVOID pContext );

    virtual HRESULT STDMETHODCALLTYPE EnumPlayers(
                        IN LPFN_ENUM_PLAYERS pfn,
                        IN LPVOID pContext ) = 0;

    // Managing Values on Servers and Players

    virtual HRESULT STDMETHODCALLTYPE SetItemValue(
                        IN DWORD dw,
                        IN LPCSTR pszKey,
                        IN LPCSTR pszValue ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetItemValue(
                        IN DWORD dw,
                        IN LPCSTR pszKey,
                        OUT LPSTR pszValue,
                        IN DWORD cbValue ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DeleteItemValue(
                        IN DWORD dw,
                        IN LPCSTR pszKey ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetNumItemValues(
                        IN DWORD dwItemID,
                        OUT DWORD* pdwNumValues ) = 0;

    typedef BOOL ( WINAPI *LPFN_ENUM_ITEM_VALUES )(
                        IN DWORD dwItemID,
                        IN LPCSTR pszKey,
                        IN LPCSTR pszValue,
                        IN LPVOID pContext );

    virtual HRESULT STDMETHODCALLTYPE EnumItemValues(
                        IN DWORD dwItemID,
                        IN LPFN_ENUM_ITEM_VALUES pfn,
                        IN LPVOID pContext ) = 0;
};

#define SERVER_ITEM_ID  0
#define ILLEGAL_ITEM_ID 0xFFFFFFFF

#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

//
// This interface is used with IZoneAsync.
//
interface IZoneMatchRegistration : public IUnknown
{
    
    virtual HRESULT STDMETHODCALLTYPE InitRegistration(
                        IN LPCSTR pszMasterAddr,        // Defaults if == NULL
                        IN unsigned short wMasterPort   // defaults if == 0
                                                ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetRegistration(
                        OUT LPSTR pszQueryAddr,
                        IN  DWORD cbQueryAddr,
                        OUT unsigned short* pQueryPort,
                        OUT time_t* pConnectTime ) = 0;
};

