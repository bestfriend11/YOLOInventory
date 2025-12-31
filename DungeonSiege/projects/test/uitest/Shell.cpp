/**************************************************************************

Filename		: Shell.cpp

Description		: Main Test Application Shell

Creation Date	: 6/30/99

**************************************************************************/


// Includes
#include "Precomp_UITest.h"

#include "execution_environment.h"
#include "PathFileMgr.h"
#include "Fuel.h"
#include "FuelDB.h"
#include "IMG.h"
#include "IMG_Default_File_Formats.h"
#include "Rapi.h"
#include "RapiOwner.h"
#include "Shell.h"
#include "tank.h"
#include "ui_shell.h"
#include "ui_textureman.h"

#include <string>


Shell :: Shell( HINSTANCE inst)
{
	gpassert( inst != NULL );

	m_hWnd		= NULL;
	m_hInstance = inst;
	m_Rapi      = NULL;
}

Shell :: ~Shell( void )
{
	// this space intentionally left blank...
}

bool Shell :: Init( int cmdShow )
{
	// don't call twice
	gpassert( m_hWnd == NULL );

// User configuration.

	// prefs
	m_ExecutionEnv.TransferAndOwn( new execution_environment( "UITest", "" ) );

	// home dir
	gpstring home = "P:\\TATTOO_ASSETS\\CDimage";
	m_ExecutionEnv->GetVariableString( "home", home );

	// window/render specs
	int width = 640, height = 480, bpp = 32;
	bool fullscreen = !GP_DEBUG, vsync = true;
	m_ExecutionEnv->GetVariableBool ( "Fullscreen",   fullscreen );
	m_ExecutionEnv->GetVariableInt32( "Width",        width      );
	m_ExecutionEnv->GetVariableInt32( "Height",       height     );
	m_ExecutionEnv->GetVariableInt32( "BitsPerPixel", bpp        );
	m_ExecutionEnv->GetVariableBool ( "VSync",        vsync      );

	// device specs
	gpstring driver;
	m_ExecutionEnv->GetRegistryString( 1, "DisplaySettings", "DriverDescription", driver );

// Create window.

	// register window class
	WNDCLASSEX wndClass;
	wndClass.cbSize			= sizeof( WNDCLASSEX );
	wndClass.style			= NULL;
	wndClass.lpfnWndProc	= WndProc;
	wndClass.cbClsExtra		= 0;
	wndClass.cbWndExtra		= 0;
	wndClass.hInstance		= m_hInstance;
	wndClass.hIcon			= NULL;
	wndClass.hCursor		= ::LoadCursor( NULL, IDC_ARROW );
	wndClass.hbrBackground	= (HBRUSH)::GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName	= NULL;
	wndClass.lpszClassName	= "UITest";
	wndClass.hIconSm		= NULL;
	::RegisterClassEx( &wndClass );

	// create window
	m_hWnd = ::CreateWindowEx( 0, "UITest", "UI Test App",
							   WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
							   CW_USEDEFAULT, SW_SHOW,
							   width, height, NULL, NULL,
							   m_hInstance, 0 );

	// Display window
	::ShowWindow( m_hWnd, cmdShow );
	::UpdateWindow( m_hWnd );

// Build services.

	// file system
	FileSys::PathFileMgr* pathMgr = new FileSys::PathFileMgr;
	pathMgr->SetRoot( home );
	pathMgr->SetMasterFileMgr();
	m_FileMgr.TransferAndOwn( pathMgr );
	Tank::GSetRootDirectory( home );

	// Rapi
	m_RapiOwner.TransferAndOwn( new RapiOwner );
	if ( FAILED( m_RapiOwner->Initialize( driver ) ) )
	{
		gperrora( "Could not initialize the RapiOwner. Initialization cannot continue." );
		::DestroyWindow( m_hWnd );
		return ( false );
	}
	driver = m_RapiOwner->GetActiveDeviceDescription();
	m_ExecutionEnv->SetRegistryString( 1, "DisplaySettings", "DriverDescription", driver );

	// Set up the default rasterizer
	RECT rect;
	GetClientRect( m_hWnd, &rect );
	m_RapiOwner->PrepareToDraw( m_hWnd, fullscreen, width, height, bpp );
	m_Rapi = m_RapiOwner->GetDevice( 0 );

	// Initialize Fuel/GAS files
	m_FuelDB.TransferAndOwn( new FuelDB );
	m_FuelDB->SetIsReadModeRelaxed( true );
	m_FuelDB->Init( home.get_std_string() );

	// Initialize IMG stuff
	m_IMG.TransferAndOwn( new IMG );
	m_IMG_Default_File_Formats.TransferAndOwn( new IMG_Default_File_Formats( *m_IMG ) );

	// Build shell
	m_UIShell.TransferAndOwn( new UIShell( *m_Rapi, *m_IMG, rect.right, rect.bottom ) );
	m_UIShell->GetTextureman()->SetInvalidTexture( "UI\\Graphics\\not_found.bmp" );

	// show cursor
	FuelHandle cursor( "UI:interfaces:cursors:generic_cursors" );
	m_UIShell->ActivateInterface( cursor );

// Done.

	return ( true );
}

int Shell :: Main( void )
{
	// Main Message Loop
	MSG msg;
    for ( ; ; )
    {
		if ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
		{
			if ( !::GetMessage( &msg, NULL, 0, 0 ) )
			{
				return ( msg.wParam );
			}
			::TranslateMessage( &msg );
			::DispatchMessage ( &msg );
		}
		else
		{
			Draw();
		}
    }
    return ( msg.wParam );
}

void Shell :: Draw()
{
	// Begin a frame
	m_Rapi->Begin3D();

	m_UIShell->Draw();

	// End a frame
	m_Rapi->End3D();
}

LRESULT CALLBACK Shell :: WndProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam )
{
	LRESULT rc = TRUE;

	switch( Msg )
	{
		case WM_LBUTTONUP:
			if ( ::DoesUIShellExist() )
			{
				g_UIShell.Input_OnMouseLeftButtonUp();
				rc = FALSE;
			}
		break;

		case WM_LBUTTONDOWN:
			if ( ::DoesUIShellExist() )
			{
				g_UIShell.Input_OnMouseLeftButtonDown();
				rc = FALSE;
			}
		break;

		case WM_MOUSEMOVE:
			if ( ::DoesUIShellExist() )
			{
				g_UIShell.Update( 0, LOWORD(lParam), HIWORD(lParam) );
				rc = FALSE;
			}
		break;

		case WM_DESTROY:
			PostQuitMessage( 0 );
			rc = FALSE;
		break;
	}

	if ( rc )
	{
		rc = ::DefWindowProc( hWnd, Msg, wParam, lParam );
	}

	return ( rc );
}
