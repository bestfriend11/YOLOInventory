/*************************************************************************************
**
**									RapiOwner
**
**		This is a test class for the purposes of planning out the different elements
**		and implementation methods of a high quality 3D abstraction layer.
**
**		Date:	05/26/99
**		Author: James Loe
**
*************************************************************************************/

#include "precomp_rapi.h"

#include "RapiImage.h"
#include "RapiOwner.h"
#include "DrWatson.h"
#include "FileSysUtils.h"


// Safe message box hooks
static void RapiSafeMessageBoxPreHook( bool /*forError*/ )
{
	if ( RapiOwner::DoesSingletonExist() )
	{
		gRapiOwner.Pause( true );
	}
}	
static void RapiSafeMessageBoxPostHook( bool /*forError*/ )
{
	if ( RapiOwner::DoesSingletonExist() )
	{
		gRapiOwner.Pause( false );
	}
}	

// Callback to get a dxdiag dump
static void DxDiagCallback( DrWatson::Info& info )
{
	if ( !(info.m_InfoFlags & DrWatson::INFO_FULL) )
	{
		return;
	}

	// $ note that this function does not alloc any heap memory.
	//   be sure to keep it that way.

	PROCESS_INFORMATION	piInfo;
	STARTUPINFO			siInfo;

	ZeroObject( piInfo );
	ZeroObject( siInfo );
	siInfo.cb = sizeof( siInfo );

	char fileName[ MAX_PATH ];
	strcpy( fileName, DrWatson::GetOutputDirectory() );
	strcat( fileName, "dxdiag.txt" );

	char command[ MAX_PATH ];
	strcpy( command, "dxdiag.exe " );
	strcat( command, fileName );

	if ( CreateProcess( NULL, command, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, NULL, NULL, &siInfo, &piInfo ) )
	{
		WaitForSingleObject( piInfo.hProcess, 60000 );

		CloseHandle( piInfo.hProcess );
		CloseHandle( piInfo.hThread );
	}

	if ( FileSys::File().OpenExisting( fileName ) )
	{
		DrWatson::RegisterFileForReport( fileName );
	}
}

// Constructor
RapiOwner::RapiOwner()
	: m_hWnd( 0 )
	, m_bFullScreen( 1 )
	, m_nPauseCount( 0 )
	, m_pDefaultDevice( 0 )
{
#	if !GP_RETAIL
	m_bTraceTextures = false;
#	endif // !GP_RETAIL

	memset( &m_ActiveDesc, 0, sizeof( AdapterDesc ) );

	ReportSys::RegisterPreMessageBoxHook( RapiSafeMessageBoxPreHook );
	ReportSys::RegisterPostMessageBoxHook( RapiSafeMessageBoxPostHook );

	DrWatson::RegisterInfoCallback( makeFunctor( DxDiagCallback ) );

	RapiImageReader::RegisterFileTypeAliases();
}

// Destructor
RapiOwner::~RapiOwner()
{
	RapiImageReader::UnregisterFileTypeAliases();

	ReportSys::UnregisterMessageBoxHook( RapiSafeMessageBoxPreHook );
	ReportSys::UnregisterMessageBoxHook( RapiSafeMessageBoxPostHook );

	for( RapiIter i = m_Devices.begin(); i != m_Devices.end(); ++i )
	{
		delete (*i).second;
	}

	for( DescListIter d = m_Desclist.begin(); d != m_Desclist.end(); ++d )
	{
		delete[] (*d).displayModes;
	}

	// Release the Direct3D handle
	RELEASE( m_pD3D );
}

// Do device enumeration and setup
HRESULT RapiOwner::Initialize( const char* driverdesc )
{
	// Create new Direct3D8 interface
	m_pD3D = Direct3DCreate8( D3D_SDK_VERSION );
	if( !m_pD3D )
	{
		// Failure?
	}

	// Enumerate our adapters
	unsigned int adapterCount	= m_pD3D->GetAdapterCount();
	for( unsigned int i = 0; i < adapterCount; ++i )
	{
		// Get this adapter's device capabilities (making sure it is HAL)
		D3DCAPS8 adapterCaps;
		if( SUCCEEDED( m_pD3D->GetDeviceCaps( i, D3DDEVTYPE_HAL, &adapterCaps ) ) )
		{
			// Get the identifying information
			D3DADAPTER_IDENTIFIER8 adapterId;
			if( SUCCEEDED( m_pD3D->GetAdapterIdentifier( i, D3DENUM_NO_WHQL_LEVEL, &adapterId ) ) )
			{
				// Build a description for this device
				AdapterDesc adapterDesc;
				adapterDesc.adapterNum		= i;
				strcpy( adapterDesc.driverDesc, adapterId.Description );

				// Enumerate the available display modes
				unsigned int modeCount		= m_pD3D->GetAdapterModeCount( i );
				adapterDesc.displayModes	= new D3DDISPLAYMODE[ modeCount ];
				adapterDesc.numDisplayModes	= modeCount;
				for( unsigned int m = 0; m < modeCount; ++m )
				{
					TRYDX( m_pD3D->EnumAdapterModes( i, m, &(adapterDesc.displayModes[ m ]) ) );
				}

				// Set an initial supported active mode
				adapterDesc.activeMode		= adapterDesc.displayModes[ 0 ];

				// Put this adapter on our list
				m_Desclist.push_back( adapterDesc );
			}
		}
	}

	// Shut the app down if we don't have any hardware acceleration
	if( m_Desclist.empty() )
	{
		gpfatal( $MSG$ "Unable to enumerate any DirectDraw devices installed on "
					   "this system.\n\n[Note: This error can occur if "
					   "another application has exclusive control of the "
					   "display. For example, NetMeeting's desktop sharing "
					   "feature may be enabled or you are trying to run this "
					   "app through a remote desktop in Windows XP or Terminal "
					   "Server. It can also occur if your hardware does not "
					   "support 3D acceleration or if your video driver does "
					   "not properly support DirectX 8.]" );
	}

#if !GP_RETAIL

	// Do device selection
	if( GetKeyState( VK_SHIFT ) & 0x8000 )
	{
		// Shift key is pressed, always bring up dialog
		m_ActiveDesc	= SelectDevice();
	}
	else
	{

#endif
		bool bMatch = false;
		if( driverdesc )
		{
			for( DescListIter i = m_Desclist.begin(); i != m_Desclist.end(); ++i )
			{
				if( !strcmp( (*i).driverDesc, driverdesc ) )
				{
					m_ActiveDesc = *i;
					m_sActiveDescription = (*i).driverDesc;
					bMatch = true;
					break;
				}
			}
		}
		if( !bMatch )
		{
			// No match, use primary
			m_ActiveDesc	= m_Desclist.front();
		}

#if !GP_RETAIL

	}

#endif

    return S_OK;
}

// Surface creation and the remainder of preparation for drawing
bool RapiOwner::PrepareToDraw( HWND hwnd, int requestedDeviceID, bool bFullScreen, int fsScreenWidth, int fsScreenHeight, int fsBPP, bool bVsync, bool bFastFPU, bool bFlip )
{
	// Initialize and make all devices we need active
	// Are we preparing for full or windowed drawing
	m_bFullScreen = bFullScreen;

	// If we are preparing to use a window, we must make sure that our bitdepth
	// is consistent with our desktop.
	if( !m_bFullScreen )
	{
		DEVMODE dm;
		memset( &dm, 0, sizeof( DEVMODE ) );
		dm.dmSize	= sizeof( DEVMODE );
		dm.dmDriverExtra = 0;

		if( !EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &dm ) )
		{
			gpwatson( "Could not retrieve desktop display settings!" );
			return false;
		}

		fsBPP = dm.dmBitsPerPel;
	}

	RapiIter i = m_Devices.find( requestedDeviceID );
	if( i != m_Devices.end() )
	{
		// Rapi object already exists
		if( (*i).second->Init( m_ActiveDesc, hwnd ) == S_OK )
		{
			return (*i).second->Enable( m_bFullScreen, fsScreenWidth, fsScreenHeight, fsBPP, bVsync );
		}
	}
	else
	{
		// Create new Rapi object
		Rapi* pRapi	= new Rapi();
		if( pRapi->Init( m_ActiveDesc, hwnd ) == S_OK )
		{
			pRapi->SetFastFPU( bFastFPU );
			pRapi->SetFlipAllowed( bFlip );
			if( pRapi->Enable( m_bFullScreen, fsScreenWidth, fsScreenHeight, fsBPP, bVsync ) )
			{
				m_Devices.insert( std::pair< int, Rapi* >( requestedDeviceID, pRapi ) );
				if( !m_pDefaultDevice )
				{
					m_pDefaultDevice = pRapi;
				}
				return true;
			}
			else
			{
				delete pRapi;
			}
		}
		else
		{
			delete pRapi;
		}
	}

	return false;
}

// Return requested device to client
Rapi* RapiOwner::GetDevice( int rDevice )
{
	RapiIter i = m_Devices.find( rDevice );
	if( i != m_Devices.end() )
	{
		return (*i).second;
	}
	else
	{
		return NULL;
	}
}

// Procedure for my device selection dialog
LRESULT CALLBACK DeviceDialogProc( HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
	int i;
	switch ( iMsg ) {

	case WM_COMMAND:
		if( HIWORD( wParam ) == LBN_DBLCLK ){
			// The user double clicked a device to use
			// I know the following loop is slow, but it is not speed sensitive
			for( i = 0; i < ((int)SendMessage( (HWND)lParam, LB_GETCOUNT, 0, 0 )); ++i ){
				if( SendMessage( (HWND)lParam, LB_GETSEL, i, 0 ) ){
					PostQuitMessage( i );
					break;
				}
			}
		}
		break;
	}
	return DefWindowProc( hDlg, iMsg, wParam, lParam );
}

// Select a device from available
AdapterDesc RapiOwner::SelectDevice()
{
	WNDCLASSEX  WndClass;
	WndClass.cbSize			= sizeof( WNDCLASSEX );
    WndClass.style			= NULL;
    WndClass.lpfnWndProc	= DeviceDialogProc;
    WndClass.cbClsExtra		= 4;
    WndClass.cbWndExtra		= 4;
    WndClass.hInstance		= NULL;
    WndClass.hIcon			= NULL;
	WndClass.hCursor		= NULL;
    WndClass.hbrBackground	= (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    WndClass.lpszMenuName	= "SelectDevice";
    WndClass.lpszClassName	= "SelectDevice";
    WndClass.hIconSm		= NULL;
	RegisterClassEx( &WndClass );

	HWND		hWndDialog;
	hWndDialog = CreateWindowEx( WS_EX_TOPMOST, "SelectDevice", "Select Device", WS_POPUP | WS_CAPTION,
								 CW_USEDEFAULT, CW_USEDEFAULT, 500, 280, m_hWnd,
								 NULL, NULL, NULL );

	ShowWindow( hWndDialog, 1 );
	UpdateWindow( hWndDialog );

	HWND		hWndListbox;
	hWndListbox = CreateWindowEx(	WS_EX_TOPMOST, "LISTBOX", "Devices",  WS_CHILD | WS_VISIBLE | LBS_NOTIFY,
									CW_USEDEFAULT, CW_USEDEFAULT, 500, 280, hWndDialog, (HMENU)1, NULL, NULL );

	for( DescListIter i = m_Desclist.begin(); i != m_Desclist.end(); ++i )
	{
		SendMessage( hWndListbox, LB_ADDSTRING, NULL, (LPARAM)(*i).driverDesc );
	}

	EnableWindow( hWndDialog, TRUE );

	MSG Message;
	for(;;)
	{
		if( PeekMessage( &Message, 0, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &Message );
			DispatchMessage( &Message );

			if( Message.message == WM_QUIT )
			{
				break;
			}
		}
		else{
			UpdateWindow( hWndDialog );
		}
	}

	char descBuffer[ 256 ];
	SendMessage( hWndListbox, LB_GETTEXT, Message.wParam, (LPARAM)descBuffer );
	m_sActiveDescription = descBuffer;

	DestroyWindow( hWndDialog );
	return m_Desclist[ (int)Message.wParam ];
}

// Disable all active devices
void RapiOwner::StopDrawing( int rDevice )
{
	// Find requested device and shut it down
	RapiIter i = m_Devices.find( rDevice );
	if( i != m_Devices.end() )
	{
		(*i).second->Disable();
	}
}

// Pause drawing
void RapiOwner::Pause( bool bPause )
{
	if ( bPause != IsPaused() )
	{
		// Iterate through all active devices and tell them to pause
		for( RapiIter i = m_Devices.begin(); i != m_Devices.end(); i++ )
		{
			(*i).second->Pause( bPause );
		}
	}

	gpassert( m_nPauseCount >= (bPause ? 0 : 1) );
	m_nPauseCount += bPause ? 1 : -1;
}

// Force a resume if not already resumed
void RapiOwner::ForceResume()
{
	if ( IsPaused() )
	{
		m_nPauseCount = 1;
		Pause( false );
	}
}

// Invalidates all textures that are currently created.  Used when windowed apps resize.
void RapiOwner::InvalidateTextures( int rDevice )
{
	// Find the requested device and invalidate all of its surfaces
	RapiIter i = m_Devices.find( rDevice );
	if( i != m_Devices.end() )
	{
		(*i).second->InvalidateSurfaces();
	}
}

// Restore all of the textures that were available before the InvalidateTextures call
void RapiOwner::RestoreTexturesAndLights( int rDevice )
{
	// Iterate through all active devices and tell them to kill their textures
	RapiIter i = m_Devices.find( rDevice );
	if( i != m_Devices.end() )
	{
		(*i).second->RestoreSurfacesAndLights();
	}
}

// Paint the back buffer(s) to the front buffer(s)
void RapiOwner::OnWmPaint( const RECT& dstRect, const RECT& srcRect )
{
	for( RapiIter i = m_Devices.begin(); i != m_Devices.end(); i++ )
	{
		(*i).second->OnWmPaint( dstRect, srcRect );
	}
}
