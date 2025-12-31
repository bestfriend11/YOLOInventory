//////////////////////////////////////////////////////////////////////////////
//
// File     :  DSVideoConfig.cpp
// Author(s):  Chad Queen, James Loe
//
// Purpose	: Defines the entry point for the application.
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ignoredwarnings.h"
#include "DSVideoConfig.h"
#include "resource.h"
#include <vector>

#define D3D_OVERLOADS

#pragma warning ( push, 1 )		// ignore any warnings or changes that win32 has
#include <d3d.h>
#pragma warning ( pop )			// back to normal

#include "gpcore.h"
#include "DllBinder.h"
#include "vector_3.h"
#include "stringtool.h"
#include "filesys.h"
#include "filesysutils.h"
#include "pathfilemgr.h"
#include "masterfilemgr.h"
#include "fuel.h"
#include "fueldb.h"
#include "namingkey.h"
#include "rapiowner.h"
#include "config.h"
#include <shlobj.h>
#include <locale.h>

// Forward declarations of enumeration functions
HRESULT __stdcall ModeEnumCallback( DDSURFACEDESC2* pddsd, void* pParentInfo );
HRESULT __stdcall DeviceEnumCallback( TCHAR* strDesc, TCHAR* strName, D3DDEVICEDESC7* pDesc, void* pParentInfo );
BOOL	__stdcall DriverEnumCallback( GUID* pGUID, TCHAR* strDesc, TCHAR* strName, VOID* devicelist, HMONITOR );

// Device object description
struct MYDXDEVICEDESC
{
	GUID							deviceguid;
	GUID							driverguid;
	char							driverdesc[256];
	gpstring						hardwarename;
	bool							belowminspec;
	bool							only16bit;
	bool							trilinear_filt;
	bool							complex_shadows;
	std::vector< DisplayMode >		displaymodes;
	DDSURFACEDESC2					activemode;
};

// Driver object description
struct MYDXOBJDESC
{
	GUID							guid;
	char							driverdesc[256];
	gpstring						hardwarename;
	bool							belowminspec;
	bool							only16bit;
	bool							trilinear_filt;
	bool							complex_shadows;
	DWORD							vidMem;
	std::vector< DDSURFACEDESC2 >	displaymodes;
	std::vector< MYDXDEVICEDESC >*	pDescList;
};

std::vector< MYDXDEVICEDESC >	g_DescList;
MYDXDEVICEDESC* g_pCurrentDriver	= NULL;
SysDetail g_16bitSystemDetail;
SysDetail g_32bitSystemDetail;
Config* pConfig						= NULL;

class GamePath
{
public:
	SET_NO_INHERITED( GamePath );

	GamePath( void )
		{  }

	bool Init( const wchar_t* path, const char* keyName = NULL, GamePath* base = NULL, bool createIfNotThere = false )
		{
			gpassert( m_Path.empty() );

			if ( (keyName != NULL) && (*keyName != '\0') )
			{
				if ( Init( gConfig.GetWString( keyName ), NULL, base, createIfNotThere ) )
				{
					return ( true );
				}
			}

			if ( (path != NULL) && (*path != '\0') )
			{
				if ( base != NULL )
				{
					if ( *base )
					{
						Init( base->GetPath() + path, NULL, NULL, createIfNotThere );
					}
				}
				else
				{
					gpassert( FileSys::IsAbsolute( path ) );

					if ( FileSys::DoesPathExist( path ) || (createIfNotThere && ::CreateDirectory( ::ToAnsi( path ), NULL )) )
					{
						m_Path = path;
						stringtool::AppendTrailingBackslash( m_Path );
						FileSys::CleanupPath( m_Path );
					}
				}
			}

			return ( !m_Path.empty() );
		}
	bool Init( const char* path, const char* keyName = NULL, GamePath* base = NULL, bool createIfNotThere = false )
		{  return ( Init( ::ToUnicode( path ), keyName, base, createIfNotThere ) );  }

	gpwstring GetPath ( bool trailingSlash = true ) const
		{
			gpassert( !m_Path.empty() );

			gpwstring path = m_Path;
			if ( !trailingSlash )
			{
				stringtool::RemoveTrailingBackslash( path );
			}
			return ( path );
		}
	gpstring  GetPathA( bool trailingSlash = true ) const
		{  return ( ::ToAnsi( GetPath( trailingSlash ) ) );  }

	bool AddSubdir( gpwstring& paths, const wchar_t* subdirName )
		{
			bool added = false;

			if ( !m_Path.empty() )
			{
				gpwstring subdir = GetPath() + subdirName;
				if ( FileSys::DoesPathExist( subdir ) )
				{
					if ( !paths.empty() )
					{
						paths += ';';
					}
					paths += subdir;
					added = true;
				}
			}

			return ( added );
		}

	operator bool ( void ) const
		{  return ( !m_Path.empty() );  }

private:
	gpwstring m_Path;
};

GamePath m_InstallPath;
GamePath m_UserPath;

class ShFolderDll : public DllBinder
{
public:
	SET_INHERITED( ShFolderDll, DllBinder );

	ShFolderDll( void )
		: Inherited( "SHFOLDER.DLL" )
	{
		AddProc( &SHGetFolderPathA, "SHGetFolderPathA" );
		AddProc( &SHGetFolderPathW, "SHGetFolderPathW", true );

		Load( false );
	}

	DllProc5 <HRESULT,						// SHGetFolderPathA(
			HWND,								// hwndOwner
			int,								// nFolder
			HANDLE,								// hToken
			DWORD,								// dwFlags
			LPSTR>								// pszPath
		SHGetFolderPathA;

	DllProc5 <HRESULT,						// SHGetFolderPathW(
			HWND,								// hwndOwner
			int,								// nFolder
			HANDLE,								// hToken
			DWORD,								// dwFlags
			LPWSTR>								// pszPath
		SHGetFolderPathW;

	SET_NO_COPYING( ShFolderDll );
};

HINSTANCE g_hInstance;


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	// Crazy fixola for loc
	char szCodePage[8];
	sprintf( szCodePage, ".%d", ::GetACP() );
	setlocale( LC_CTYPE, szCodePage );

	g_hInstance = hInstance;

	// dev paths?
	bool devPaths = false;

	// optionally set/clear dev paths for testing purposes (this only works if
	// the ini file is in the exe dir, but that's a dev setup anyway so cool)
	IniFile iniFile;
	if ( iniFile.LoadFile( "DungeonSiege" ) )
	{
		// assume that if there is an ini file in this dir then it's dev paths
		devPaths = true;

		// see if there's an override
		gpstring setting;
		if ( iniFile.ReadKeyValue( "dev_paths", setting ) )
		{
			::FromString( setting, devPaths );
		}
	}

	// get some vars
	ShFolderDll shfolder;
	char userPathA[ MAX_PATH ];
	gpwstring userPath;

	// if our shfolder version is too low to handle our query or we don't know
	// what it is, just force dev paths
	if ( shfolder && SUCCEEDED( shfolder.SHGetFolderPathA( NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, userPathA ) ) )
	{
		// try for unicode paths if available
		wchar_t userPathW[ MAX_PATH ];
		if ( shfolder.SHGetFolderPathW && SUCCEEDED( shfolder.SHGetFolderPathW( NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, userPathW ) ) )
		{
			userPath   = userPathW;
		}
		else
		{
			userPath   = ::ToUnicode( userPathA );
		}
	}
	else
	{
		gperror( "Unable to access SHGetFolderPath() - your SHFOLDER.DLL needs updating!\n" );
		devPaths = true;
	}

	// install path is where the EXE is located
	m_InstallPath.Init( FileSys::GetModuleDirectory() );

	// based on dev paths option, choose our base paths
	if ( devPaths )
	{
		m_UserPath   = m_InstallPath;
	}
	else
	{
		m_UserPath  .Init( userPath   + L"\\Dungeon Siege", NULL, NULL, true );
	}

	// File system init
	FileSys::MasterFileMgr masterFileMgr;

	FileSys::PathFileMgr fileMgr;
	fileMgr.SetRoot( m_InstallPath.GetPathA() );

	masterFileMgr.AddFileMgr( &fileMgr );

	// Fuel system init
	FuelSys fuel;
	FuelDB* pfDB	= fuel.AddTextDb( "default" );
	pfDB->Init( FuelDB::OPTION_JIT_READ, m_InstallPath.GetPathA() );

	// build player path driver
	gpstring playerPath = m_UserPath.GetPathA();
	std::auto_ptr <FileSys::PathFileMgr> pathFileMgr( new FileSys::PathFileMgr );
	if ( pathFileMgr->SetRoot( playerPath ) )
	{
		// install it
		masterFileMgr.AddDriver( pathFileMgr.release(), true, "player" );

		// build player fuel db
		TextFuelDB* playerDb = fuel.AddTextDb( "player" );
		playerDb->Init(   FuelDB::OPTION_READ
					    | FuelDB::OPTION_WRITE
					    | FuelDB::OPTION_JIT_READ
					    | FuelDB::OPTION_NO_WRITE_LIQUID
					    | FuelDB::OPTION_NO_WRITE_TYPES,
					    "player://",
						playerPath );
	}
	else
	{
		gperror( "Could not create player prefs!" );
	}

	pConfig	= new Config( COMPANY_NAME, APP_NAME );
	pConfig->SetIniFileName( m_UserPath.GetPathA() + "DungeonSiege" );
	pConfig->SetOutRegistryIni();

	DialogBox( hInstance, MAKEINTRESOURCE(IDD_DIALOG_VIDEO_CONFIG), NULL, DialogVideoConfigProc );
    
	return 0;
}


// The Window Procedure for the main window
LRESULT CALLBACK WindProc( HWND hWnd, UINT Msg, WPARAM  wParam,
						   LPARAM  lParam )
{
	switch ( Msg )
	{
	case WM_CREATE:		
		break;
	}
	return DefWindowProc( hWnd, Msg, wParam, lParam );
}


BOOL CALLBACK DialogVideoConfigProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
	case WM_INITDIALOG:
		{
			Init( hDlg );
		}
		return TRUE;
	case WM_COMMAND:
		switch ( LOWORD( wParam ) )
		{
		case IDOK:
			{
				SaveConfiguration( hDlg );
				EndDialog( hDlg, 0 );
			}
			break;
		case IDCANCEL:
			{
				EndDialog( hDlg, 0 );
			}
			break;
		case IDC_BUTTON_TEST:
			{
				TestConfiguration( hDlg );
			}
			break;
		case IDC_COMBO_DRIVER:
			{
				if ( HIWORD( wParam ) == CBN_SELCHANGE )
				{					
					gpwstring sNewDriver;
					GetDriver( hDlg, sNewDriver );
					for( std::vector< MYDXDEVICEDESC >::iterator i = g_DescList.begin(); i != g_DescList.end(); ++i )
					{
						gpwstring sBuffer;
						char szBuffer[256] = "";

						if( (*i).deviceguid == IID_IDirect3DHALDevice )
						{							
							sBuffer.assignf( L"%s - %s", ToUnicode( (*i).driverdesc ).c_str(), GetResourceString( IDS_STRING_HARDWARE ).c_str() );							
						}
						else if( (*i).deviceguid == IID_IDirect3DTnLHalDevice )
						{							
							sBuffer.assignf( L"%s - %s", ToUnicode( (*i).driverdesc ).c_str(),GetResourceString( IDS_STRING_HARDWARE_TNL ).c_str() );							
						}
						else
						{							
							sBuffer.assignf( L"%s - %s", ToUnicode( (*i).driverdesc ).c_str(), GetResourceString( IDS_STRING_UNKNOWN ).c_str() );							
						}

						if( sBuffer.same_no_case( sNewDriver ) )
						{
							g_pCurrentDriver = &(*i);
							break;
						}
					}

					if( i != g_DescList.end() )
					{
						NewDriverSelected( hDlg );
					}
				}
			}
			break;
		case IDC_COMBO_RESOLUTION:
			{
				if ( HIWORD( wParam ) == CBN_SELCHANGE )
				{
					// Notify of resolution change
					NewResolutionSelected( hDlg );
				}
			}
			break;
		case IDC_SHADOWCOMBO:
			{
				if( HIWORD( wParam ) == CBN_SELCHANGE )
				{
					// Notify of shadow change
					NewShadowSelected( hDlg );
				}
			}
			break;
		case IDC_FILTERCOMBO:
			{
				if( HIWORD( wParam ) == CBN_SELCHANGE )
				{
					// Notify of shadow change
					NewFilterSelected( hDlg );
				}
			}
			break;
		}

		return TRUE;	
	}

	return FALSE;
}


// Dialog Initialization
void Init( HWND hDlg )
{
    // Enumerate drivers, devices, and modes
	DirectDrawEnumerateEx( DriverEnumCallback, &g_DescList,
                           DDENUM_ATTACHEDSECONDARYDEVICES |
                           DDENUM_DETACHEDSECONDARYDEVICES |
                           DDENUM_NONDISPLAYDEVICES );

	// Shut the app down if we don't have any hardware acceleration
	if( g_DescList.empty() )
	{
/*
		gpfatal( $MSG$ "Unable to enumerate any DirectDraw devices installed on "
					   "this system.\n\n[Note: this error can also occur if "
					   "another application has exclusive control of the "
					   "display. For example, NetMeeting's desktop sharing "
					   "feature may be enabled.]" );
*/
	}

	// $$$ look up saved device if there is one
	gpstring driverdesc	= gConfig.GetString( "driver_description" );
	gpwstring sDriverDesc = ToUnicode( driverdesc );
	TranslateDriverDescToUnicode( sDriverDesc );
	gpwstring sSelectDriver;	

	for( std::vector< MYDXDEVICEDESC >::iterator i = g_DescList.begin(); i != g_DescList.end(); ++i )
	{
		gpwstring sBuffer;		

		if( (*i).deviceguid == IID_IDirect3DHALDevice )
		{			
			sBuffer.assignf( L"%s - %s", ToUnicode( (*i).driverdesc ).c_str(), GetResourceString( IDS_STRING_HARDWARE ).c_str() );							
		}
		else if( (*i).deviceguid == IID_IDirect3DTnLHalDevice )
		{			
			sBuffer.assignf( L"%s - %s", ToUnicode( (*i).driverdesc ).c_str(), GetResourceString( IDS_STRING_HARDWARE_TNL ).c_str() );							
		}
		else
		{			
			sBuffer.assignf( L"%s - %s", ToUnicode( (*i).driverdesc ).c_str(), GetResourceString( IDS_STRING_UNKNOWN ).c_str() );							
		}

		if( sDriverDesc.same_no_case( sBuffer ) || i == g_DescList.begin() )
		{
			g_pCurrentDriver	= &(*i);
			sSelectDriver = sBuffer;
		}

		AddDriver( hDlg, ToAnsi( sBuffer ) );
	}

	// Shadow options	
	AddShadowOption( hDlg, ToAnsi( GetResourceString( IDS_STRING_COMPLEX_SHADOWS ) ) );
	AddShadowOption( hDlg, ToAnsi( GetResourceString( IDS_STRING_SIMPLE_SHADOWS ) ) );
	AddShadowOption( hDlg, ToAnsi( GetResourceString( IDS_STRING_NO_SHADOWS ) ) ); 
	AddShadowOption( hDlg, ToAnsi( GetResourceString( IDS_STRING_PARTY_SHADOWS ) ) ); 

	// Filter options
	AddFilterOption( hDlg, ToAnsi( GetResourceString( IDS_STRING_BILINEAR_FILTERING ) ) );
	AddFilterOption( hDlg, ToAnsi( GetResourceString( IDS_STRING_TRILINEAR_FILTERING ) ) );

	// Select the driver
	SelectDriver( hDlg, sSelectDriver );
}

void TranslateDriverDescToUnicode( gpwstring & sDriverDesc )
{	
	unsigned int pos = sDriverDesc.find( L"Hardware TnL" );	

	if ( pos != gpwstring::npos )
	{
		gpwstring sStart = sDriverDesc.substr( 0, pos );		
		sDriverDesc.assignf( L"%s%s", sStart.c_str(), GetResourceString( IDS_STRING_HARDWARE_TNL ).c_str() );
	}
	else
	{
		pos = sDriverDesc.find( L"Hardware" );
		if ( pos != gpwstring::npos )
		{
			gpwstring sStart = sDriverDesc.substr( 0, pos );					
			sDriverDesc.assignf( L"%s%s", sStart.c_str(), GetResourceString( IDS_STRING_HARDWARE ).c_str() );
		}
		else
		{
			pos = sDriverDesc.find( L"TnL" );
			if ( pos != gpwstring::npos )
			{
				gpwstring sStart = sDriverDesc.substr( 0, pos );					
				sDriverDesc.assignf( L"%s%s", sStart.c_str(), GetResourceString( IDS_STRING_UNKNOWN ).c_str() );
			}
		}
	}
}


void TranslateDriverDescToAnsi( gpwstring & sDriverDesc )
{		
	unsigned int pos = sDriverDesc.find( GetResourceString( IDS_STRING_HARDWARE_TNL )  );	

	if ( pos != gpwstring::npos )
	{
		gpwstring sStart = sDriverDesc.substr( 0, pos );		
		sDriverDesc.assignf( L"%s%s", sStart.c_str(), L"Hardware TnL" );
	}
	else
	{		
		pos = sDriverDesc.find( GetResourceString( IDS_STRING_HARDWARE ) );
		if ( pos != gpwstring::npos )
		{
			gpwstring sStart = sDriverDesc.substr( 0, pos );		
			sDriverDesc.assignf( L"%s%s", sStart.c_str(), L"Hardware" );
		}
		else
		{			
			pos = sDriverDesc.find( GetResourceString( IDS_STRING_UNKNOWN ) );
			if ( pos != gpwstring::npos )
			{
				gpwstring sStart = sDriverDesc.substr( 0, pos );		
				sDriverDesc.assignf( L"%s%s", sStart.c_str(), L"TnL" );
			}
		}
	}
}

gpwstring GetResourceString( unsigned int id )
{
	char szBuffer[512] = "";
	LoadString( g_hInstance, id, szBuffer, sizeof( szBuffer ) );
	return ToUnicode( szBuffer );
}


// Query
void GetDriver( HWND hDlg, gpwstring & sDriver )
{	
	char szDriver[512] = "";
	GetDlgItemText( hDlg, IDC_COMBO_DRIVER, szDriver, MAX_PATH );
	sDriver = ToUnicode( szDriver );
}


void GetResolution( HWND hDlg, char * szResolution )
{
	GetDlgItemText( hDlg, IDC_COMBO_RESOLUTION, szResolution, MAX_PATH );	
}


void GetShadow( HWND hDlg, char * szShadow )
{
	GetDlgItemText( hDlg, IDC_SHADOWCOMBO, szShadow, MAX_PATH );	
}


void GetFilter( HWND hDlg, char * szFilter )
{
	GetDlgItemText( hDlg, IDC_FILTERCOMBO, szFilter, MAX_PATH );	
}


void AddDriver( HWND hDlg, const char * szDriver )
{
	SendMessage( GetDlgItem( hDlg, IDC_COMBO_DRIVER ), CB_ADDSTRING, 0, (LPARAM)szDriver );
}


void AddResolution( HWND hDlg, const char * szResolution )
{
	SendMessage( GetDlgItem( hDlg, IDC_COMBO_RESOLUTION ), CB_ADDSTRING, 0, (LPARAM)szResolution );
}


void AddShadowOption( HWND hDlg, const char * szShadow )
{
	SendMessage( GetDlgItem( hDlg, IDC_SHADOWCOMBO ), CB_ADDSTRING, 0, (LPARAM)szShadow );
}


void AddFilterOption( HWND hDlg, const char * szFilter )
{
	SendMessage( GetDlgItem( hDlg, IDC_FILTERCOMBO ), CB_ADDSTRING, 0, (LPARAM)szFilter );
}


void ClearDrivers( HWND hDlg )
{
	SendMessage( GetDlgItem( hDlg, IDC_COMBO_DRIVER ), CB_RESETCONTENT, 0, 0 );
}


void ClearResolutions( HWND hDlg )
{
	SendMessage( GetDlgItem( hDlg, IDC_COMBO_RESOLUTION ), CB_RESETCONTENT, 0, 0 );
}


void SelectDriver( HWND hDlg, gpwstring & sDriver )
{
	gpstring sFind = ToAnsi( sDriver.c_str() );
	int sel = SendMessage( GetDlgItem( hDlg, IDC_COMBO_DRIVER ), CB_FINDSTRINGEXACT, 0, (LPARAM)sFind.c_str() );
	if ( sel != CB_ERR )
	{
		SendMessage( GetDlgItem( hDlg, IDC_COMBO_DRIVER ), CB_SETCURSEL, sel, 0 );
		NewDriverSelected( hDlg );
	}
}


void SelectResolution( HWND hDlg, const char * szResolution )
{
	int sel = SendMessage( GetDlgItem( hDlg, IDC_COMBO_RESOLUTION ), CB_FINDSTRINGEXACT, 0, (LPARAM)szResolution );
	if ( sel != CB_ERR )
	{
		SendMessage( GetDlgItem( hDlg, IDC_COMBO_RESOLUTION ), CB_SETCURSEL, sel, 0 );
		NewResolutionSelected( hDlg );
	}
}


void SelectShadow( HWND hDlg, const char * szShadow )
{
	int sel = SendMessage( GetDlgItem( hDlg, IDC_SHADOWCOMBO ), CB_FINDSTRINGEXACT, 0, (LPARAM)szShadow );
	if ( sel != CB_ERR )
	{
		SendMessage( GetDlgItem( hDlg, IDC_SHADOWCOMBO ), CB_SETCURSEL, sel, 0 );
		NewShadowSelected( hDlg );
	}
}


void SelectFilter( HWND hDlg, const char * szFilter )
{
	int sel = SendMessage( GetDlgItem( hDlg, IDC_FILTERCOMBO ), CB_FINDSTRINGEXACT, 0, (LPARAM)szFilter );
	if ( sel != CB_ERR )
	{
		SendMessage( GetDlgItem( hDlg, IDC_FILTERCOMBO ), CB_SETCURSEL, sel, 0 );
		NewFilterSelected( hDlg );
	}
}


// Events
void NewDriverSelected( HWND hDlg )
{
	gpwstring sDriver;
	GetDriver( hDlg, sDriver );

	SendMessage( GetDlgItem( hDlg, IDC_HARDWARENAME ), WM_SETTEXT, 0, (LPARAM)g_pCurrentDriver->hardwarename.c_str() );

	// Save the driver setting
	TranslateDriverDescToAnsi( sDriver );

	gpstring newDriver = ToAnsi( sDriver );
	gConfig.Set( "driver_description", newDriver );

	if( g_pCurrentDriver->belowminspec )
	{
		MessageBoxW( NULL, GetResourceString( IDS_STRING_BELOW_MIN_SPEC_WARN ).c_str(), GetResourceString( IDS_STRING_HARDWARE_WARNING ).c_str(), MB_OK );
	}

	// Clear existing resolutions
	ClearResolutions( hDlg );

	// Add the resolutions for this driver
	char fallbackDriver[256];
	bool bResolutionSet	= false;
	for( std::vector< DisplayMode >::iterator i = g_pCurrentDriver->displaymodes.begin(); i != g_pCurrentDriver->displaymodes.end(); ++i )
	{
		char buf[ 256 ];
		sprintf( buf, "%dx%dx%d", (*i).m_Width, (*i).m_Height, (*i).m_BPP );
		AddResolution( hDlg, buf );

		// Make sure something gets selected
		if( i == g_pCurrentDriver->displaymodes.begin() )
		{
			strcpy( fallbackDriver, buf );
		}

		// Look for the saved res
		if( ( gConfig.HasKey( "width" ) && gConfig.GetInt( "width" ) == (*i).m_Width ) &&
			( gConfig.HasKey( "height" ) && gConfig.GetInt( "height" ) == (*i).m_Height ) &&
			( gConfig.HasKey( "bpp" ) && gConfig.GetInt( "bpp" ) == (*i).m_BPP ) )
		{
			bResolutionSet	= true;
			SelectResolution( hDlg, buf );
		}
	}

	if( !bResolutionSet )
	{
		SelectResolution( hDlg, fallbackDriver );
	}

	FuelHandle prefs;

	// build path
	FuelHandle base( "::player" );
	if ( base )
	{
		prefs = base->GetChildBlock( "prefs" );
		if ( !prefs )
		{
			prefs = base->CreateChildBlock( "prefs", "prefs.gas" );
		}
	}

	if( prefs->HasKey( "video_shadows" ) )
	{
		gpstring shadows = prefs->GetString( "video_shadows" );
		if( shadows.same_no_case( "complex" ) )
		{
			SelectShadow( hDlg, ToAnsi( GetResourceString( IDS_STRING_COMPLEX_SHADOWS ) ) );
		}
		else if( shadows.same_no_case( "simple" ) )
		{
			SelectShadow( hDlg, ToAnsi( GetResourceString( IDS_STRING_SIMPLE_SHADOWS ) ) );
		}
		else if( shadows.same_no_case( "complex_party" ) )
		{
			SelectShadow( hDlg, ToAnsi( GetResourceString( IDS_STRING_PARTY_SHADOWS ) ) );
		}
		else
		{
			SelectShadow( hDlg, ToAnsi( GetResourceString( IDS_STRING_NO_SHADOWS ) ) );
		}
	}
	else
	{
		if( g_pCurrentDriver->complex_shadows )
		{
			SelectShadow( hDlg, ToAnsi( GetResourceString( IDS_STRING_PARTY_SHADOWS ) ) );
		}
		else
		{
			SelectShadow( hDlg, ToAnsi( GetResourceString( IDS_STRING_SIMPLE_SHADOWS ) ) );
		}
	}

	if( prefs->HasKey( "texture_filtering" ) )
	{
		if( prefs->GetString( "texture_filtering" ).same_no_case( "trilinear" ) )
		{
			SelectFilter( hDlg, ToAnsi( GetResourceString( IDS_STRING_TRILINEAR_FILTERING ) ) );
		}
		else
		{
			SelectFilter( hDlg, ToAnsi( GetResourceString( IDS_STRING_BILINEAR_FILTERING ) ) );
		}
	}
	else
	{
		if( g_pCurrentDriver->trilinear_filt )
		{
			SelectFilter( hDlg, ToAnsi( GetResourceString( IDS_STRING_TRILINEAR_FILTERING ) ) );
		}
		else
		{
			SelectFilter( hDlg, ToAnsi( GetResourceString( IDS_STRING_BILINEAR_FILTERING ) ) );
		}
	}
}


void NewResolutionSelected( HWND hDlg )
{
	char szResolution[256];
	GetResolution( hDlg, szResolution );

	// Parse new resolution string
	unsigned int width, height, bpp;
	sscanf( szResolution, "%dx%dx%d", &width, &height, &bpp );

	// Save to config
	gConfig.Set( "width", width );
	gConfig.Set( "height", height );
	gConfig.Set( "bpp", bpp );
}


void NewShadowSelected( HWND hDlg )
{
	char szShadow[256];
	GetShadow( hDlg, szShadow );
	gpwstring sShadow = ToUnicode( szShadow );

	FuelHandle prefs( "::player:prefs" );
	if( sShadow.same_no_case( GetResourceString( IDS_STRING_COMPLEX_SHADOWS ) ) )
	{
		prefs->Set( "video_shadows", "complex" );
	}
	else if( sShadow.same_no_case( GetResourceString( IDS_STRING_SIMPLE_SHADOWS ) ) )
	{
		prefs->Set( "video_shadows", "simple" );
	}
	else if( sShadow.same_no_case( GetResourceString( IDS_STRING_PARTY_SHADOWS ) ) )
	{
		prefs->Set( "video_shadows", "complex_party" );
	}
	else
	{
		prefs->Set( "video_shadows", "none" );
	}
}


void NewFilterSelected( HWND hDlg )
{
	char szFilter[256];
	GetFilter( hDlg, szFilter );
	gpwstring sFilter = ToUnicode( szFilter );

	FuelHandle prefs( "::player:prefs" );
	if( sFilter.same_no_case( GetResourceString( IDS_STRING_TRILINEAR_FILTERING ) ) )
	{
		prefs->Set( "texture_filtering", "trilinear" );
	}
	else
	{
		prefs->Set( "texture_filtering", "bilinear" );
	}
}


// Configuration
void SaveConfiguration( HWND hDlg )
{
	// Delete config in order to write out values
	delete pConfig;

	FuelHandle prefs( "::player:prefs" );
	prefs->GetDB()->SaveChanges();
}

void TestConfiguration( HWND hDlg )
{
}

// Callback function for enumerating display modes.
HRESULT __stdcall ModeEnumCallback( DDSURFACEDESC2* pDesc, VOID* param )
{
    // Add the mode to the list
    MYDXOBJDESC* pObjDesc = (MYDXOBJDESC*)param;
    pObjDesc->displaymodes.push_back(*pDesc);

	return DDENUMRET_OK;
}

// Callback function for enumerating devices
HRESULT __stdcall DeviceEnumCallback( TCHAR* strDesc, TCHAR* strName, D3DDEVICEDESC7* pDesc, void* pParentInfo )
{
	UNREFERENCED_PARAMETER( strName );
	UNREFERENCED_PARAMETER( strDesc );

	// Get our object description pointer
	MYDXOBJDESC* pObjDesc	= (MYDXOBJDESC*)pParentInfo;

	if( !(pDesc->dwDevCaps & D3DDEVCAPS_HWRASTERIZATION) )
	{
		return D3DENUMRET_OK;
	}

	// Setup a new device
	MYDXDEVICEDESC devicedesc;
	memset( &devicedesc, 0, sizeof( MYDXDEVICEDESC ) );

    // Set up device info for this device
	devicedesc.deviceguid		= pDesc->deviceGUID;
	devicedesc.driverguid		= pObjDesc->guid;
	strcpy( devicedesc.driverdesc, pObjDesc->driverdesc );
	devicedesc.hardwarename		= pObjDesc->hardwarename;
	devicedesc.belowminspec		= pObjDesc->belowminspec;
	devicedesc.only16bit		= pObjDesc->only16bit;
	devicedesc.trilinear_filt	= pObjDesc->trilinear_filt;
	devicedesc.complex_shadows	= pObjDesc->complex_shadows;

    // Build list of supported modes for the device
    for( std::vector< DDSURFACEDESC2 >::const_iterator i = pObjDesc->displaymodes.begin();
		 i != pObjDesc->displaymodes.end(); ++i )
    {
        DDSURFACEDESC2 ddsdMode = (*i);
        DWORD dwRenderDepths    = pDesc->dwDeviceRenderBitDepth;
        DWORD dwDepth           = ddsdMode.ddpfPixelFormat.dwRGBBitCount;

        // Accept modes that are compatable with the device
        if( ( ( dwDepth == 32 ) && ( dwRenderDepths & DDBD_32 ) ) ||
            ( ( dwDepth == 24 ) && ( dwRenderDepths & DDBD_24 ) ) ||
            ( ( dwDepth == 16 ) && ( dwRenderDepths & DDBD_16 ) ) )
        {
			// Create a new display mode structure
			if( (ddsdMode.dwWidth >= 640 && ddsdMode.dwHeight >= 480) &&
				(ddsdMode.ddpfPixelFormat.dwRGBBitCount == 16 || ddsdMode.ddpfPixelFormat.dwRGBBitCount == 32) )
			{
				if( !pObjDesc->only16bit || (pObjDesc->only16bit && ddsdMode.ddpfPixelFormat.dwRGBBitCount == 16) )
				{
					DisplayMode nMode;
					nMode.m_Width		= ddsdMode.dwWidth;
					nMode.m_Height		= ddsdMode.dwHeight;
					nMode.m_BPP			= ddsdMode.ddpfPixelFormat.dwRGBBitCount;

					// Put the new mode on our listing
					devicedesc.displaymodes.push_back( nMode );
				}
			}
        }
    }

    // Bail if the device has no supported modes
    if( !devicedesc.displaymodes.size() )
	{
        return D3DENUMRET_OK;
	}

	// Put this device into our list
	pObjDesc->pDescList->push_back( devicedesc );

    return D3DENUMRET_OK;
}

// Callback function for enumerating drivers.
BOOL __stdcall DriverEnumCallback( GUID* pGUID, TCHAR* strDesc, TCHAR* strName, VOID* devicelist, HMONITOR )
{
	UNREFERENCED_PARAMETER( strName );

	// Structures we need to do the enumeration
	MYDXOBJDESC		objdesc;
    LPDIRECTDRAW7	pDD;
    LPDIRECT3D7		pD3D;
    HRESULT			hr;

	// Clear our driver description
	memset( &objdesc, 0, sizeof( MYDXOBJDESC ) );
	objdesc.pDescList = (std::vector< MYDXDEVICEDESC >*)devicelist;

    // Use the GUID to create the DirectDraw object
    hr = DirectDrawCreateEx( pGUID, (VOID**)&pDD, IID_IDirectDraw7, NULL );
    if( FAILED(hr) )
    {
        return D3DENUMRET_OK;
    }

    // Create a D3D object, to enumerate the d3d devices
    hr = pDD->QueryInterface( IID_IDirect3D7, (VOID**)&pD3D );
    if( FAILED(hr) )
    {
        pDD->Release();
        return D3DENUMRET_OK;
    }

	// Copy driver information into our object structure
	if( pGUID )
	{
		objdesc.guid = *pGUID;
	}
	strcpy( objdesc.driverdesc, strDesc );

    // Enumerate the fullscreen display modes.
    pDD->EnumDisplayModes( 0, NULL, &objdesc, ModeEnumCallback );

	// Get available video memory
	DDSCAPS2 ddscaps2;
	ZeroMemory( &ddscaps2, sizeof( DDSCAPS2 ) );

	ddscaps2.dwCaps	= DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
	if( FAILED(pDD->GetAvailableVidMem( &ddscaps2, NULL, &objdesc.vidMem ) ) )
	{
		objdesc.vidMem	= 16777216;
	}

	unsigned int current_16bit_mem_diff	= UINT_MAX;
	unsigned int current_32bit_mem_diff	= UINT_MAX;
	FuelHandle detail( "system_detail" );
	gpassert( detail.IsValid() );

	fuel_block_list& detail_blocks = detail->GetChildBlocks();
	for( fuel_block_list::const_iterator i = detail_blocks.begin(); i != detail_blocks.end(); ++i )
	{
		// Make a new block
		SysDetail sys_det;

		// Get the memory and bpp of this block
		sscanf( (*i)->GetName(), "%dm%db", &sys_det.memory, &sys_det.bpp );

		// Multiply by one megabyte to convert from meg to bytes
		sys_det.memory	<<= 20;

		// Get the detail value
		(*i)->Get( "detail", sys_det.detail, false );

		// Get the resolution block
		FuelHandle hResolutions;
		(*i)->GetChildBlock( "resolutions", hResolutions );
		gpassert( hResolutions.IsValid() );

		fuel_block_list& resolution_blocks = hResolutions->GetChildBlocks();
		for( fuel_block_list::const_iterator r = resolution_blocks.begin(); r != resolution_blocks.end(); ++r )
		{
			ValidSysMode valid_mode;

			// Get the width and height of this resolution
			sscanf( (*r)->GetName(), "%dx%d", &valid_mode.m_Width, &valid_mode.m_Height );

			// Get the maximum allowable number of backbuffers for this resolution
			(*r)->Get( "max_back_buffers", valid_mode.m_MaxBackBuffers, false );
			sys_det.valid_modes.push_back( valid_mode );
		}

		// Find the closest memory match among the contestants
		unsigned int mem_diff	= (objdesc.vidMem > sys_det.memory) ? (objdesc.vidMem - sys_det.memory) : (sys_det.memory - objdesc.vidMem);
		if( sys_det.bpp == 16 && mem_diff < current_16bit_mem_diff )
		{
			current_16bit_mem_diff	= mem_diff;
			g_16bitSystemDetail		= sys_det;
		}
		else if( sys_det.bpp == 32 && mem_diff < current_32bit_mem_diff )
		{
			current_32bit_mem_diff	= mem_diff;
			g_32bitSystemDetail		= sys_det;
		}
	}

	// Get this card's identifying information
	DDDEVICEIDENTIFIER2	deviceId;
	if( FAILED( pDD->GetDeviceIdentifier( &deviceId, 0 ) ) )
	{
		return D3DENUMRET_OK;
	}

	FuelHandle video( "video_capabilities" );
	if( video.IsValid() )
	{
		FuelHandleList vendor_blocks;
		video->ListChildBlocks( -1, NULL, "vendor*", vendor_blocks );
		for( FuelHandleList::const_iterator o = vendor_blocks.begin(); o != vendor_blocks.end(); ++o )
		{
			FuelHandleList device_blocks;
			(*o)->ListChildBlocks( -1, NULL, "device*", device_blocks );
			for( FuelHandleList::const_iterator d = device_blocks.begin(); d != device_blocks.end(); ++d )
			{
				if( (DWORD)(*d)->GetInt( "vendorid" ) == deviceId.dwVendorId &&
					(DWORD)(*d)->GetInt( "deviceid" ) == deviceId.dwDeviceId )
				{
					objdesc.hardwarename	= (*o)->GetString( "vendor" );
					objdesc.hardwarename	+= " ";
					objdesc.hardwarename	+= (*d)->GetString( "name" );
					objdesc.belowminspec	= (*d)->GetBool( "below_min_spec", (*o)->GetBool( "below_min_spec" ) );
					objdesc.only16bit		= (*d)->GetBool( "only_16_bit", (*o)->GetBool( "only_16_bit" ) );
					objdesc.trilinear_filt	= (*d)->GetBool( "trilinear_filt", (*o)->GetBool( "trilinear_filt" ) );
					objdesc.complex_shadows	= !(*d)->GetBool( "no_complex_shadows", (*o)->GetBool( "no_complex_shadows" ) );
					break;
				}
			}

			if( d != device_blocks.end() )
			{
				break;
			}
		}

		if( o == vendor_blocks.end() )
		{
			objdesc.hardwarename	= "Unknown Video Hardware";
			objdesc.belowminspec	= false;
			objdesc.only16bit		= false;
			objdesc.trilinear_filt	= true;
			objdesc.complex_shadows	= true;
		}
	}

	// Go through enumerated modes and clean out ones we don't want
	for( std::vector< DDSURFACEDESC2 >::iterator mode = objdesc.displaymodes.begin(); mode != objdesc.displaymodes.end(); )
	{
		if( (*mode).ddpfPixelFormat.dwRGBBitCount == 16 )
		{
			// Check it against valid 16 bit formats
			for( std::vector< ValidSysMode >::iterator sys_mode = g_16bitSystemDetail.valid_modes.begin(); sys_mode != g_16bitSystemDetail.valid_modes.end(); ++sys_mode )
			{
				if( (*mode).dwWidth == (*sys_mode).m_Width &&
					(*mode).dwHeight == (*sys_mode).m_Height )
				{
					break;
				}
			}
			if( sys_mode == g_16bitSystemDetail.valid_modes.end() )
			{
				mode = objdesc.displaymodes.erase( mode );
			}
			else
			{
				++mode;
			}
		}
		else if( (*mode).ddpfPixelFormat.dwRGBBitCount == 32 )
		{
			// Check it against valid 32 bit formats
			for( std::vector< ValidSysMode >::iterator sys_mode = g_32bitSystemDetail.valid_modes.begin(); sys_mode != g_32bitSystemDetail.valid_modes.end(); ++sys_mode )
			{
				if( (*mode).dwWidth == (*sys_mode).m_Width &&
					(*mode).dwHeight == (*sys_mode).m_Height )
				{
					break;
				}
			}
			if( sys_mode == g_32bitSystemDetail.valid_modes.end() )
			{
				mode = objdesc.displaymodes.erase( mode );
			}
			else
			{
				++mode;
			}
		}
		else
		{
			mode = objdesc.displaymodes.erase( mode );
		}
	}

    // Now, enumerate all the 3D devices
    pD3D->EnumDevices( DeviceEnumCallback, &objdesc );

    // Clean up and return
    pD3D->Release();
    pDD->Release();

    return DDENUMRET_OK;
}


