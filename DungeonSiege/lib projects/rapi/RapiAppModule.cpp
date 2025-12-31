//////////////////////////////////////////////////////////////////////////////
//
// File     :  RapiAppModule.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Rapi.h"
#include "RapiAppModule.h"

#include "Config.h"
#include "FileSysUtils.h"
#include "RapiImage.h"
#include "RapiMouse.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////
// class RapiAppModule implementation

RapiAppModule :: RapiAppModule( eOptions options )
	: Inherited( options )
{
	// construct rapi device owner
	m_RapiOwner = new RapiOwner;

	// register our events
	RegisterEventCb( makeFunctor( *this, &RapiAppModule::RapiInitWindow   ), PRI_INIT_WINDOW   );
	RegisterEventCb( makeFunctor( *this, &RapiAppModule::RapiInitServices ), PRI_INIT_SERVICES );
	RegisterEventCb( makeFunctor( *this, &RapiAppModule::RapiShutdown     ), PRI_SHUTDOWN+10   );
}

RapiAppModule :: ~RapiAppModule( void )
{
	// this space intentionally left blank...
}

bool RapiAppModule :: ScreenShot( bool frontBuffer, const wchar_t* fileName, gpwstring* outFileName )
{
	UNREFERENCED_PARAMETER( frontBuffer );

	// $$$ switch this function to use PNG's instead

// Take the shot.

	// take the screen shot
	std::auto_ptr <RapiMemImage> image( gDefaultRapi.CreateSubsectionImage() );

// Build the filename.

	// build filename if empty
	gpwstring localFileName;
	if ( (fileName == NULL) || (*fileName == '\0') )
	{
		localFileName = L"s_s_";
	}
	else
	{
		localFileName = fileName;
	}

	// add path to name if not already there
	if ( localFileName.find_first_of( L"\\/:" ) == gpwstring::npos )
	{
		// get base path
		gpwstring dir = m_ScreenShotDir;
		if ( dir.empty() )
		{
			dir = gConfig.GetWString( "screen_shot_dir", FileSys::GetLocalWritableDirW() );
		}
		stringtool::AppendTrailingBackslash( dir );
		localFileName.insert( 0, dir );
	}

#	if GP_RETAIL

	// split off the path
	int pathLen = FileSys::GetPathEnd( localFileName ) - localFileName.c_str();
	gpwstring localPath = localFileName.left( pathLen );
	localFileName.erase( 0, pathLen );

	// generate unique filename
	localFileName = FileSys::MakeUniqueFileName( localFileName, L"bmp", localPath );

#	else // GP_RETAIL

	// build unique filename using timestamp info
	if ( !IsCapturingMovie() )
	{
		SYSTEMTIME time;
		::GetLocalTime( &time );
		localFileName.appendf( L"%S_%04d_%02d_%02d_%02d_%02d_%02d.bmp",
								SysInfo::GetComputerName(),
								time.wYear,
								time.wMonth,
								time.wDay,
								time.wHour,
								time.wMinute,
								time.wSecond );
	}

#	endif // GP_RETAIL

// Save it out.

	// create directory if it doesn't exist
	bool success = FileSys::CreateFullFileNameDirectory( localFileName );
	if ( success )
	{
		// write the image out to disk
#		if !GP_RETAIL
		localFileName.to_lower();
#		endif // !GP_RETAIL
		success = ::WriteImage( ::ToAnsi( localFileName ), IMAGE_BMP, *image, true, false, true );
	}

	// status
	gpgenericf(( "%s screen shot: %S\n", success ? "Took" : "Failed to take", localFileName.c_str() ));

	// update
	if ( outFileName != NULL )
	{
		*outFileName = localFileName;
	}

	// done
	return ( success );
}

bool RapiAppModule :: CopyScreenShotToClipboard( void )
{
	std::auto_ptr <RapiMemImage> image( gDefaultRapi.CreateSubsectionImage( false ) );
	if ( image.get() != NULL )
	{
		image->CopyToClipboard( true );
	}
	return ( image.get() != NULL );
}

void RapiAppModule :: StartCaptureMovie( const char* fileName, float fps, float secondsFromNow, float durationSeconds )
{
	m_MovieFilename  = fileName;
	m_MovieFrame     = 0;
	m_MovieBeginTime = GetAbsoluteTime() + secondsFromNow;
	m_MovieEndTime   = m_MovieBeginTime + durationSeconds;

	if ( !m_MovieFilename.empty() )
	{
		SetOptions( OPTION_FIXED_FRAME_RATE );
		SetFixedFrameRate( fps );
	}
}

bool RapiAppModule :: FrameUpdateCursor( int* pCursorX, int* pCursorY, bool bFullUpdate )
{
	bool bSuccess = Inherited::FrameUpdateCursor( pCursorX, pCursorY, bFullUpdate );
	if ( bFullUpdate )
	{
		gRapiMouse.SetCursorPosition( GetCursorX(), GetCursorY() );
	}
	return bSuccess;
}

bool RapiAppModule :: OnScreenChange( void )
{
	RebuildRapi();

	// recenter mouse etc
	ResetUserInputs();

	return ( true );
}

void RapiAppModule :: OnFailSafeShutdown( void )
{
	// $$$ SetCooperativeLevel( hwnd, DDSCL_NORMAL )
}

void RapiAppModule :: OnRestoreSurfaces( void )
{
	gDefaultRapi.RestoreSurfaces();
}

bool RapiAppModule :: OnGetFullScreen( bool& full )
{
	// $$$ check to see if full screen is ok
	UNREFERENCED_PARAMETER( full );

	return ( true );
}

bool RapiAppModule :: OnGetGameSize( GSize& size )
{
	// $$$ check to see if the requested game size is ok
	UNREFERENCED_PARAMETER( size );

	return ( true );
}

bool RapiAppModule :: OnGetGameSizeDirty( void )
{
	if ( gDefaultRapiPtr == NULL )
	{
		return ( true );
	}

	// Only pay attention to BPP and VSync if in fullscreen, where they actually matter
	if( gDefaultRapi.IsFullscreen() )
	{
		if (   (gDefaultRapi.GetBitsPerPixel() != GetConfigBpp())
			|| (gDefaultRapi.IsVSync()         != GetConfigVSync()) )
		{
			return ( true );
		}
	}

	return ( false );
}

bool RapiAppModule :: OnPaint( PAINTSTRUCT& ps )
{
	// only bother to paint if we're not active (we'd be painting anyway in that case)
	if (   !IsAppActive()
		&& (ps.rcPaint.left < ps.rcPaint.right )
		&& (ps.rcPaint.top  < ps.rcPaint.bottom) )
	{
		// tell rapi to repaint itself(ves)
		m_RapiOwner->OnWmPaint( ps.rcPaint, ps.rcPaint );
	}
	return ( true );
}

bool RapiAppModule :: OnEndFrame( void )
{
	if ( IsCapturingMovie() )
	{
		if ( GetAbsoluteTime() <= m_MovieEndTime )
		{
			if ( GetAbsoluteTime() >= m_MovieBeginTime )
			{
				// on first frame, force time diff in case we lost time getting here
				if ( m_MovieFrame == 0 )
				{
					m_MovieEndTime = GetAbsoluteTime() + m_MovieEndTime - m_MovieBeginTime;
				}

				// there's grass on the field, take the shot
				ScreenShot( false, gpwstringf( L"%S-%05d", m_MovieFilename.c_str(), m_MovieFrame++ ) );
			}
		}
		else
		{
			StopCaptureMovie();
		}
	}

	return ( true );
}

bool RapiAppModule :: OnClearScreen( void )
{
	if ( gDefaultRapiPtr != NULL )
	{
		gRapiMouse.SetCursorImage( 0, 0, 0 );
		gDefaultRapi.ClearAllBuffers();
	}
	return ( true );
}

void RapiAppModule :: OnResetUserInputs( bool /*keyboard*/, bool mouse )
{
	if ( mouse && RapiMouse::DoesSingletonExist() )
	{
		gRapiMouse.SetCursorPosition( GetCursorX(), GetCursorY() );
	}
}

bool RapiAppModule :: OnMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& returnValue )
{
	UNREFERENCED_PARAMETER( hwnd );
	UNREFERENCED_PARAMETER( wparam );
	UNREFERENCED_PARAMETER( lparam );
	UNREFERENCED_PARAMETER( returnValue );

	switch( msg )
	{
		case( WM_DESTROY ):
		{
			// Shut down mouse
			gRapiMouse.Disable();
		}
	}

	return false;
}

bool RapiAppModule :: GetConfigVSync( void ) const
{
	return ( gConfig.GetBool( "vsync", true ) );
}

int RapiAppModule :: GetConfigBpp( void ) const
{
	return ( gConfig.GetInt( "bpp", 32 ) );
}

bool RapiAppModule :: GetConfigFlip( void ) const
{
	if ( gDefaultRapiPtr == NULL )
	{
		return ( !gConfig.GetBool( "bltonly", false ) );
	}
	else
	{
		return ( !gConfig.GetBool( "bltonly", !gDefaultRapiPtr->IsFlipAllowed() ) );
	}
}

bool RapiAppModule :: RapiInitWindow( void )
{
	// $$$ this section must change - configuration of rasterizer at game
	//     startup has to be absolutely bulletproof. this code is placeholder
	//     until polish time. -scott

	// initialize rapi using old device if any
	gpstring deviceDesc = gConfig.GetString( "driver_description" );
	TRYDX_SUCCEED( m_RapiOwner->Initialize( deviceDesc ) );

	// save back out the configuration
	deviceDesc = m_RapiOwner->GetActiveDeviceDescription();
	gConfig.Set( "driver_description", deviceDesc );

	// create directdraw
	return ( RebuildRapi() );
}

bool RapiAppModule :: RapiInitServices( void )
{
	// Set system level gamma correction
	gDefaultRapi.SetSystemGamma( gConfig.GetFloat( "sysgamma", 2.5f ) );

	// Set notextures option
	gDefaultRapi.SetNoTextures( gConfig.GetBool( "notextures", false ) );

	return ( true );
}

bool RapiAppModule :: RapiShutdown( void )
{
	// delete owned objects
	Delete( m_RapiOwner );

	// all ok i guess
	return ( true );
}

bool RapiAppModule :: RebuildRapi( void )
{
	// stop rapi
	m_RapiOwner->InvalidateTextures();
	m_RapiOwner->ForceResume();
	m_RapiOwner->StopDrawing();

	// rebuild rapi
	bool rc = m_RapiOwner->PrepareToDraw( GetMainWnd(),			// HWND hwnd
										  0,					// int requestedDeviceID = 0,
										  IsFullScreen(),		// bool bFullScreen = true,
										  GetGameWidth(),		// int fsScreenWidth = 640,
										  GetGameHeight(),		// int fsScreenHeight = 480,
										  GetConfigBpp(),		// int fsBPP = 16,
										  GetConfigVSync(),		// bool bVsync = true
										  TestOptions( OPTION_FAST_FTOL ),
										  GetConfigFlip() );

	if( !rc )
	{
		gpfatal( $MSG$ "Could not restore renderer, now exiting..." );
	}

	// restore rapi
	m_RapiOwner->RestoreTexturesAndLights();

	// reset to chop mode because directx futzes the mode in setup (noted in
	// dx docs for coop level stuff)
	if ( TestOptions( OPTION_FAST_FTOL ) )
	{
		SetFpuChopMode();
	}

	// done
	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
