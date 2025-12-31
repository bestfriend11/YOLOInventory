//////////////////////////////////////////////////////////////////////////////
//
// File     :  RapiAppModule.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains an AppModule derivative for Rapi.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __RAPIAPPMODULE_H
#define __RAPIAPPMODULE_H

//////////////////////////////////////////////////////////////////////////////

#include "AppModule.h"

//////////////////////////////////////////////////////////////////////////////
// forward declarations

class RapiOwner;

//////////////////////////////////////////////////////////////////////////////
// class RapiAppModule declaration

class RapiAppModule : public AppModule, public Singleton <RapiAppModule>
{
public:
	SET_INHERITED( RapiAppModule, AppModule );

	static ThisType& GetSingleton( void )  {  return ( Singleton <ThisType>::GetSingleton() );  }

	RapiAppModule( eOptions options = OPTION_NONE );
	virtual ~RapiAppModule( void );

	// screen shots
	virtual bool ScreenShot               ( bool frontBuffer, const wchar_t* fileName = NULL, gpwstring* outFileName = NULL );
FEX			bool ScreenShot               ( const wchar_t* fileName = NULL )	{  return ( ScreenShot( false, fileName ) );  }
FEX			bool ScreenShot               ( const char* fileName = NULL )		{  return ( ScreenShot( false, fileName ? ::ToUnicode( fileName ) : NULL ) );  }
			bool ScreenShotDefault        ( void )								{  return ( ScreenShot( false, NULL ) );  }
FEX			bool CopyScreenShotToClipboard( void );
			void SetScreenShotDir         ( const gpwstring& dir )				{  m_ScreenShotDir = dir;  }

	// movie making
FEX	void StartCaptureMovie( const char* fileName, float fps, float secondsFromNow, float durationSeconds );
FEX	void StopCaptureMovie ( void )			{  ClearOptions( OPTION_FIXED_FRAME_RATE );  }
FEX	bool IsCapturingMovie ( void ) const	{  return ( TestOptions( OPTION_FIXED_FRAME_RATE ) && !m_MovieFilename.empty() );  }

	// mouse state
	virtual bool FrameUpdateCursor( int* pCursorX, int* pCursorY, bool bFullUpdate = true );

protected:

	// special execution
	virtual bool OnScreenChange( void );

	// special pipeline
	virtual void OnFailSafeShutdown( void );
	virtual void OnRestoreSurfaces ( void );

	// init callbacks
	virtual bool OnGetFullScreen   ( bool& full );
	virtual bool OnGetGameSize     ( GSize& size );
	virtual bool OnGetGameSizeDirty( void );

	// message callbacks
	virtual bool OnPaint          ( PAINTSTRUCT& ps );
	virtual bool OnEndFrame       ( void );
	virtual bool OnClearScreen    ( void );
	virtual void OnResetUserInputs( bool keyboard, bool mouse );
	virtual bool OnMessage        ( HWND hwnd, UINT msg, WPARAM wparam,	LPARAM lparam, LRESULT& returnValue );

private:

	bool GetConfigVSync( void ) const;
	int  GetConfigBpp  ( void ) const;
	bool GetConfigFlip ( void ) const;

	// pipeline
	bool RapiInitWindow  ( void );
	bool RapiInitServices( void );
	bool RapiShutdown    ( void );

	// utility
	bool RebuildRapi( void );

	// spec
	gpwstring m_ScreenShotDir;

	// owned objects
	my RapiOwner* m_RapiOwner;

	// movie making
	gpstring m_MovieFilename;
	int      m_MovieFrame;
	double   m_MovieBeginTime;
	double   m_MovieEndTime;

	FUBI_CLASS_INHERIT( RapiAppModule, Inherited );
	FUBI_SINGLETON_CLASS( RapiAppModule, "Rapi-enabled version of AppModule." );
	SET_NO_COPYING( RapiAppModule );
};

#define gRapiAppModule RapiAppModule::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

#endif  // __RAPIAPPMODULE_H

//////////////////////////////////////////////////////////////////////////////
