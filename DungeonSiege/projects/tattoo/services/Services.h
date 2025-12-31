#pragma once

#include "fubidefs.h"


class gpprofiler;
class GAS;
class Rapi;
class RapiFont;
class FuelSys;
class NamingKey;
class SkritBotMgr;
class TankMgr;


namespace siege {
	class SiegeEngine;
	class Console;
}

namespace nema {
	class AspectStorage;
	class PRSKeysStorage;
}

namespace FuBi {
	class SysExports;
}

namespace Skrit {
	class Engine;
}

namespace GPGSound {
	class SoundManager;
}


/*=======================================================================================

  Services

  purpose:	This class is a collection of the libraries the Game and World require.

  author:	Bartosz Kijanka

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
class Services : public Singleton <Services>
{
public:
	Services( void );
   ~Services( void );

	enum
	{
		APP_DUNGEON_SIEGE = 'DS',
		APP_SIEGE_EDITOR  = 'SE',
		APP_ANIM_VIEWER   = 'AVue',
		APP_SIEGE_MAX	  = 'SMax',
	};

	static void   SetAppType( FOURCC app )		{  msAppType = app;  }
	static FOURCC GetAppType( void )			{  return ( msAppType );  }
	static bool   IsDungeonSiege( void )		{  return ( msAppType == APP_DUNGEON_SIEGE );  }
	static bool   IsEditor( void )				{  return ( msAppType == APP_SIEGE_EDITOR  );  }
	static bool   IsAnimViewer( void )			{  return ( msAppType == APP_ANIM_VIEWER  );  }
	static bool   IsSiegeMax( void )			{  return ( msAppType == APP_SIEGE_MAX  );  }

	bool   InitServices( FOURCC app );
	bool   Reinit( void );
	void   Shutdown( bool fullShutdown );
	bool   Xfer( FuBi::PersistContext& persist );
	void   EnableLogAll( bool enable = true );
	bool   IsLogAllEnabled( void ) const;

	static FuBi::SysExports* CreateFuBi( void );
	static Skrit::Engine*    CreateSkrit( void );

#	if !GP_RETAIL
	static void DumpHelp( void );
#	endif
	static void CheckSkrit( const gpstring& skritFileName );

	RapiFont &						GetConsoleFont();
	FUBI_EXPORT siege::Console & 	GetOutputConsole();

#	if !GP_RETAIL
	FUBI_EXPORT siege::Console & 	GetFooterConsole();
	FUBI_EXPORT siege::Console & 	GetWatchConsole();
#	endif // !GP_RETAIL

private:

	//----- services created here
	std::auto_ptr< FuBi::SysExports				> mFuBiSysExports;				// $ Singleton
	std::auto_ptr< TankMgr						> mTankMgr;						// $ Singleton
	std::auto_ptr< FuelSys						> mFuelSys;
	std::auto_ptr< siege::SiegeEngine           > mRenderer;
	std::auto_ptr< siege::Console				> mOutputConsole;
#	if !GP_RETAIL
	std::auto_ptr< ReportSys::Sink				> mOutputConsoleSink;
#	endif // !GP_RETAIL
	std::auto_ptr< ReportSys::Sink				> mOutputFileSink;
#	if !GP_RETAIL
	std::auto_ptr< siege::Console				> mFooterConsole;
	std::auto_ptr< siege::Console				> mWatchConsole;
	std::auto_ptr< siege::Console				> mTempStatsConsole;
#	endif // !GP_RETAIL
	std::auto_ptr< GPGSound::SoundManager		> mSoundManager;
	std::auto_ptr< Skrit::Engine				> mSkritEngine;					// $ Singleton
#	if GP_ENABLE_SKRITBOT
	std::auto_ptr< SkritBotMgr					> mSkritBotMgr;
#	endif // GP_ENABLE_SKRITBOT
	std::auto_ptr< NamingKey					> mNamingKey;


	// TODO-- revisit this to decide if it would be better to
	// create a 'nema uber-class' that groups all the nema data together
	// similar to what the soul_handle did --biddle

	std::auto_ptr< nema::AspectStorage			> mNemaAspectStorage;			// bid - Also a singleton
	std::auto_ptr< nema::PRSKeysStorage			> mNemaKeyMgr;

	//----- windows related
	std::auto_ptr< RapiFont			            > mConsoleFont;

	static FOURCC msAppType;

	FUBI_SINGLETON_CLASS( Services, "Basic game services." );
	SET_NO_COPYING( Services );
};

#define gServices Services::GetSingleton()
#define gFooterConsole gServices.GetFooterConsole()
#define gWatchConsole gServices.GetWatchConsole()


/*=======================================================================================



  purpose:

  author:

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
//
// this is just a little timer thingy...
//
class ScopeTimer
{
public:
	ScopeTimer( Rapi* renderer, siege::Console* console );
	ScopeTimer( gpstring const & scope_name );
	~ScopeTimer();

private:
	static siege::Console* mOutputConsole;
	static Rapi*           mRenderer;

	bool mIgnoreDestruction;

#	if !GP_RETAIL
	double      mBeginTime;
	gpstring    mName;
	unsigned int mLastLinePrintedTo;
#	endif // !GP_RETAIL

#	if GP_DEBUG
	unsigned int mBeginNumAllocs;
	unsigned int mBeginSizeAllocs;
	unsigned int mBeginSerialAllocID;
#	endif // GP_DEBUG
};


// This is our Window Callback
LRESULT CALLBACK Tattoo_MessageCallback( HWND hWnd, UINT Message, WPARAM WParam, LPARAM LParam );
