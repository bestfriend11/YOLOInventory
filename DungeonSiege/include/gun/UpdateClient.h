/**************************************************************
* UpdateClient.h: Games AutoUpdate Component
*
* Chris N. Haddan
*
* (C) 2001 Microsoft Corporation
*
***************************************************************/
#pragma once

#ifndef __UPDATECLIENTH__
#define __UPDATECLIENTH__
#include "windows.h"
#include "objbase.h"
#include "INITGUID.H"

interface __declspec(
		uuid( "72f336af-af4c-490d-95a2-3638dd236a61" ) ) IUpdateClient;

#define IID_IUpdateClient  __uuidof( IUpdateClient )


interface __declspec(
		uuid( "F1F1AE09-1B6F-48f2-8F42-0F43B7E3E99F" ) ) IUpdateClient2;

#define IID_IUpdateClient2  __uuidof( IUpdateClient2 )


interface __declspec(
		uuid( "E157EA66-F20C-432d-9D7E-09A9DF91270D" ) ) ISetupEngineResult;

#define IID_ISetupEngineResult  __uuidof( ISetupEngineResult )



enum {	UC_E_INVALIDPARAM,		
		UC_E_WINSOCKINITFAILED,		
		UC_E_CONNECTFAILED,		
		UC_E_NOTCONNECTED, 
		UC_E_SENDFAILED,		
		UC_E_OUTOFMEMORY,			
		UC_E_REQUEST_FAILED,	
		UC_E_INVALIDDATARECEIVED,
		UC_E_ENUMCANCELLED,		
		UC_E_ALREADYCONNECTED,		
		UC_E_NOPACKAGESELECTED, 
		UC_E_PACKAGENOTPREPARED,	
		UC_E_DOWNLOADFAILED,	
		UC_E_CANTCREATETHREAD,		
		UC_E_ENUMERATIONFAILED,
		UC_E_INITIALIZESETUPFAILED,
		UC_E_UPDATING_EXIT_NOW,
		UC_E_CLIENT_UPDATE_AVAILABLE,
		UC_E_EXECUTE_SELF_UPDATE_FAILED,
		UC_E_INVALIDCONNECTIONSETTINGS,
		UC_E_NOTIMPLEMENTED,
		UC_E_SERVERACCESSDENIED,
		UC_E_ADMINRIGHTSREQUIRED
};

enum {	UCSTATE_NONE,			
		UCSTATE_PREPARED,			
		UCSTATE_INSTALLED,		
		UCSTATE_PREPARATIONCANCELLED, 
		UCSTATE_INSTALLATIONCANCELLED, 
		UCSTATE_PREPARATIONFAILED,
		UCSTATE_INSTALLATIONFAILED};

enum {UCMODE_NONE, UCMODE_PREPARING, UCMODE_INSTALLING};

enum {DLSTATE_DOWNLOADING, DLSTATE_DOWNLOADSUCCESS, DLSTATE_DOWNLOADFAILED, DLSTATE_DOWNLOADCANCELLED};


typedef int (WINAPI * ALERTWINPROC)(HWND,LPSTR,LPSTR,UINT);

typedef struct tagPACKAGESTATUS {
	DWORD  dwState;
	DWORD  dwMode;
	DWORD  dwPhase;
	TCHAR  *pszAction;
	TCHAR  *pszTarget;
	TCHAR  *pszTempFolder;
	DWORD  dwCurrentBytesDownloaded;
	DWORD  dwTotalBytesToDownload;
	DWORD  dwEstimatedTimeRemaining;
	double dTransferRateBitsPerSecond;
	DWORD  dwOperationsComplete;
	DWORD  dwTotalOperations;
	DWORD  dwDownloadState;
	PVOID  pSetupEngine;
} PACKAGESTATUS, *LPPACKAGESTATUS;

typedef struct tagPACKAGEDESC {
	DWORD dwPackageId;
	TCHAR *pszGameGuid;
	TCHAR *pszGameVersion;
	TCHAR *pszPackageName;
	TCHAR *pszPackageVersion;
	TCHAR *pszLanguage;
	TCHAR *pszGameParam;
	DWORD dwReserved1;
	TCHAR *pszReserved2;
	DWORD dwReserved3;
	bool  bPackageInstalled; 
	DWORD dwPackageStatus;
} PACKAGEDESC, *LPPACKAGEDESC;

typedef struct tagFILEINFO {
	TCHAR *pszFileName;
	TCHAR *pszFilePath;
	DWORD dwSize;
	DWORD dwCrc;
} FILEINFO, *LPFILEINFO;

typedef struct tagCONNECTIONSETTINGS {
	TCHAR *pszServer;
	TCHAR *pszProxy;
	DWORD dwServerPort;
	DWORD dwProxyPort;
} CONNECTIONSETTINGS, *LPCONNECTIONSETTINGS;


typedef bool (*ENUMPACKAGESCALLBACK)(LPPACKAGEDESC pPackageInfo, PVOID pContext);
typedef bool (*ENUMFILESCALLBACK)(LPFILEINFO pFileInfo, PVOID pContext);
typedef bool (*PACKAGESTATUSCALLBACK)(LPPACKAGESTATUS pStatus, PVOID pContext);
typedef int (WINAPI * ALERTWINPROC)(HWND,LPSTR,LPSTR,UINT);

interface IUpdateClient : public IUnknown
{
		virtual HRESULT ConfigureConnectionSettings (TCHAR *pszProxy, DWORD dwProxyPort, TCHAR *pszServer, DWORD dwServerPort) = 0;
		virtual HRESULT EnumeratePackages (LPPACKAGEDESC pPackageDesc, ENUMPACKAGESCALLBACK lpfnCallback, PVOID pContext) = 0;
		virtual HRESULT EnumerateFilesInPackage (LPPACKAGEDESC pPackageDesc, ENUMFILESCALLBACK lpfnCallback, PVOID pContext) = 0;
		virtual HRESULT SelectPackage (LPPACKAGEDESC pPackageDesc) = 0;
		virtual HRESULT PreparePackage(PACKAGESTATUSCALLBACK lpfnCallback, PVOID pContext, HANDLE hWaitEvent) = 0;
		virtual HRESULT PreparePackageFromTempFolder (TCHAR *pszTempFolder) = 0;
		virtual HRESULT ExecutePackageInstaller(PACKAGESTATUSCALLBACK lpfnCallback, PVOID pContext, HANDLE hWaitEvent) = 0;
		virtual HRESULT GetPackageStatus (LPPACKAGESTATUS *pPackageStatus) = 0;
		virtual HRESULT RemovePackageFiles() = 0;
		virtual HRESULT DownloadFiles (DWORD dwPackageId, TCHAR *pszDestPath) = 0;
		virtual HRESULT CancelInstall() = 0;
		virtual HRESULT ShutdownSetupEngine() = 0;
		virtual HRESULT SetAppWnd (HWND) = 0;
		virtual	void SetLastHResult (HRESULT hResult) = 0;
		virtual	HRESULT GetLastHResult (void ) = 0;
		virtual HRESULT SetAppAlertCallback (ALERTWINPROC pCallback) = 0;
		virtual HRESULT LaunchAutoUpdate (DWORD dwPackageId, TCHAR *szRelaunchExe, TCHAR *szRelaunchParams) = 0;
		virtual HRESULT LaunchAutoUpdateToInstallDownloadedPackage (TCHAR *szTempFolder, TCHAR *szRelaunchExe, TCHAR *szRelaunchParams) = 0;
		virtual HRESULT DownloadAndInstallClientUpdate (DWORD dwPackageId, TCHAR *szRelaunchExe, TCHAR *szRelaunchParams, PACKAGESTATUSCALLBACK lpfnCallback, PVOID pContext, HANDLE hWaitEvent) = 0;
		virtual HRESULT IsClientUpdateAvailable (DWORD *pdwPackageId) = 0;
		virtual HRESULT GetServerVersion (TCHAR *szVersion) = 0;
		virtual LPPACKAGESTATUS GetPackageStatusPtr (void) = 0; 
};


// new interface to support fully async operation
interface IUpdateClient2 : public IUnknown
{
		virtual HRESULT ConfigureConnectionSettings (TCHAR *pszProxy, DWORD dwProxyPort, TCHAR *pszServer, DWORD dwServerPort, DWORD dwLangId) = 0;
		virtual HRESULT EnumeratePackages (LPPACKAGEDESC pPackageDesc, ENUMPACKAGESCALLBACK lpfnCallback, PVOID pContext, HANDLE hWaitEvent) = 0;
		virtual HRESULT EnumerateFilesInPackage (LPPACKAGEDESC pPackageDesc, ENUMFILESCALLBACK lpfnCallback, PVOID pContext) = 0;
		virtual HRESULT SelectPackage (LPPACKAGEDESC pPackageDesc) = 0;
		virtual HRESULT PreparePackage(PACKAGESTATUSCALLBACK lpfnCallback, PVOID pContext, HANDLE hWaitEvent) = 0;
		virtual HRESULT PreparePackageFromTempFolder (TCHAR *pszTempFolder) = 0;
		virtual HRESULT ExecutePackageInstaller(PACKAGESTATUSCALLBACK lpfnCallback, PVOID pContext, HANDLE hWaitEvent) = 0;
		virtual HRESULT GetPackageStatus (LPPACKAGESTATUS *pPackageStatus) = 0;
		virtual HRESULT RemovePackageFiles() = 0;
		virtual HRESULT DownloadFiles (DWORD dwPackageId, TCHAR *pszDestPath) = 0;
		virtual HRESULT CancelInstall() = 0;
		virtual HRESULT ShutdownSetupEngine() = 0;
		virtual HRESULT SetAppWnd (HWND) = 0;
		virtual	void SetLastHResult (HRESULT hResult) = 0;
		virtual	HRESULT GetLastHResult (void ) = 0;
		virtual HRESULT SetAppAlertCallback (ALERTWINPROC pCallback) = 0;
		virtual HRESULT LaunchAutoUpdate (DWORD dwPackageId, TCHAR *szRelaunchExe, TCHAR *szRelaunchParams) = 0;
		virtual HRESULT LaunchAutoUpdateToInstallDownloadedPackage (TCHAR *szTempFolder, TCHAR *szRelaunchExe, TCHAR *szRelaunchParams) = 0;
		virtual HRESULT DownloadAndInstallClientUpdate (DWORD dwPackageId, TCHAR *szRelaunchExe, TCHAR *szRelaunchParams, PACKAGESTATUSCALLBACK lpfnCallback, PVOID pContext, HANDLE hWaitEvent) = 0;
		virtual HRESULT IsClientUpdateAvailable (DWORD *pdwPackageId, HANDLE hWaitEvent) = 0;
		virtual HRESULT GetServerVersion (TCHAR *szVersion) = 0;
		virtual LPPACKAGESTATUS GetPackageStatusPtr (void) = 0; 
};

interface ISetupEngineResult : public IUnknown
{
		virtual HRESULT GetEngineResultCode (DWORD *pdwResultCode) = 0;
};

class __declspec(uuid( "3696D5F3-2050-44a1-BE3E-35AA7C5BC2DB" ) ) CUpdateClient;

#define CLSID_UpdateClient __uuidof (CUpdateClient)

#endif