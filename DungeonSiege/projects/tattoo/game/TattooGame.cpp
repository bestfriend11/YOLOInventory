//////////////////////////////////////////////////////////////////////////////
//
// File     :  TattooGame.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Game.h"


#include "AtlX.h"
#include "AIAction.h"
#include "CameraAgent.h"
#include "Config.h"
#include "ContentDb.h"
#include "DebugProbe.h"
#include "DllBinder.h"
#include "DrWatson.h"
#include "FileSysUtils.h"
#include "FileSysXfer.h"
#include "FuBi.h"
#include "FuBiPersistImpl.h"
#include "GameMovie.h"
#include "GamePersist.h"
#include "GameState.h"
#include "GoAspect.h"
#include "GoCore.h"
#include "GoDb.h"
#include "GoFollower.h"
#include "GoMind.h"
#include "GoParty.h"
#include "GpConsole.h"
#include "GpStats.h"
#include "Gps_manager.h"
#include "ImeUi.h"
#include "InputBinder.h"
#include "LocHelp.h"
#include "MasterFileMgr.h"
#include "MessageDispatch.h"
#include "Mood.h"
#include "NeMa_Aspect.h"
#include "NeMa_AspectMgr.h"
#include "NetFubi.h"
#include "NetLog.h"
#include "PathFileMgr.h"
#include "PContentDb.h"
#include "Player.h"
#include "Quantify.h"
#include "RapiImage.h"
#include "RapiMouse.h"
#include "ReportSys.h"
#include "Server.h"
#include "Services.h"
#include "Siege_Console.h"
#include "Siege_Frustum.h"
#include "Siege_Loader.h"
#include "Siege_Mouse_Shadow.h"
#include "Sim.h"
#include "SkritBotMgr.h"
#include "SkritEngine.h"
#include "SkritObject.h"
#include "SkritSupport.h"
#include "TattooGame.h"
#include "TimeMgr.h"
#include "TimeOfDay.h"
#include "TankMgr.h"
#include "TattooVersion.h"
#include "Ui_ChatBox.h"
#include "Ui.h"
#include "UiCamera.h"
#include "UiFrontEnd.h"
#include "UiIntro.h"
#include "UiOutro.h"
#include "UiMultiplayer.h"
#include "UiStoreManager.h"
#include "UIZoneMatch.h"
#include "UiOptions.h"
#include "UiGame.h"
#include "UiGameConsole.h"
#include "UiUserConsole.h"
#include "UiMenuManager.h"
#include "UiPartyManager.h"
#include "Weather.h"
#include "World.h"
#include "WorldFx.h"
#include "WorldOptions.h"
#include "WorldMap.h"
#include "WorldSound.h"
#include "WorldTerrain.h"
#include "WorldTime.h"
#include "MCP.h"
#include "FuelDB.h"
#include "siege_compass.h"
#include "siege_options.h"
#include "..\Exe\Res\Resource.h"

#include <zmouse.h>

//////////////////////////////////////////////////////////////////////////////

FUBI_REPLACE_NAME( "TattooGame", Game );

//////////////////////////////////////////////////////////////////////////////
// class EbuEulaDll declaration and implementation

class EbuEulaDll : public DllBinder
{
public:
	SET_INHERITED( EbuEulaDll, DllBinder );

	EbuEulaDll( const gpstring& path )
		: Inherited( path + "EBUEULA.DLL" )
	{
		AddProc( &EBUEula, "EBUEula" );
	}

	CdeclProc4 <BOOL,				// EBUEula(
			LPCTSTR,					// lpRegKeyLocation
			LPCTSTR,					// lpEULAFileName
			LPCSTR,						// lpWarrantyFileName
			BOOL>						// fCheckForFirstRun )
		EBUEula;
};

//////////////////////////////////////////////////////////////////////////////
// class PooDll declaration and implementation

#include "cdadecl.h"
#include "ltdll.h"

class PooDll : public DllBinder
{
public:
	SET_INHERITED( PooDll, DllBinder );

	PooDll( const gpstring& path )
		: Inherited( path + "LTDLL.DLL" )
	{
		AddProc( &LTDLL_Initialise  , "LTDLL_Initialise"   );
		AddProc( &LTDLL_Authenticate, "LTDLL_Authenticate" );
	}

	CdeclProc1 <CdaBOOL,			// LTDLL_Initialise(
			CdaUINT32>					// VersionNumber
		LTDLL_Initialise;

	CdeclProc2 <CdaBOOL,			// LTDLL_Authenticate(
			const CdaTCHAR*,			// pSourceDirectory
			CdaUINT32*>					// pReasonCode
		LTDLL_Authenticate;
};

//////////////////////////////////////////////////////////////////////////////
// class AutoHideGui declaration and implementation

class AutoHideGui : public Singleton <AutoHideGui>
{
public:
	AutoHideGui( bool doit = true )
	{
		// take command flag
		m_Hiding = doit;

		// get old vis settings
		m_OldCursorVisible = gTattooGame.TestOptions( TattooGame::GOPTION_CLEAN_RENDER );
		m_OldGuiVisible = gUI.GetGameGUI().GetVisible();
		m_OldCompassVisible = gSiegeEngine.GetCompass().GetVisible();

		// hide stuff if requested
		if ( m_Hiding )
		{
			// always hide cursor
			gTattooGame.SetOptions( TattooGame::GOPTION_CLEAN_RENDER, true );

			// extra stuff for ingame
			if ( ::IsInGame( gWorldState.GetCurrentState() ) )
			{
				// do this to get rid of paper doll aspect
				gUIGame.GetUIPartyManager()->CloseAllCharacterMenus();

				// turn off vis
				gUI.GetGameGUI().SetVisible( false );
				gSiegeEngine.GetCompass().SetVisible( false );
			}
		}
	}

   ~AutoHideGui( void )
	{
		// restore old vis if was disabled
		if ( m_Hiding )
		{
			gTattooGame.SetOptions( TattooGame::GOPTION_CLEAN_RENDER, m_OldCursorVisible );
			gUI.GetGameGUI().SetVisible( m_OldGuiVisible );
			gSiegeEngine.GetCompass().SetVisible( m_OldCompassVisible );
		}
	}

private:
	bool m_Hiding;
	bool m_OldCursorVisible;
	bool m_OldGuiVisible;
	bool m_OldCompassVisible;
};

//////////////////////////////////////////////////////////////////////////////
// class GameConsoleSink declaration and implementation

class GameConsoleSink : public ReportSys::StreamSink
{
public:
	SET_INHERITED( GameConsoleSink, ReportSys::StreamSink );

	// ctor/dtor
	GameConsoleSink( void )
		: Inherited( "GameConsole" )
	{
		SetType( "GameConsole" );
	}

	// overrides
	virtual bool OutputString( const ReportSys::Context* context, const char* text, int len,
							   bool startLine, bool endLine )
	{
		CHECK_PRIMARY_THREAD_ONLY;

		if ( startLine )
		{
			m_Buffer.assign( GetIndentSpaces( context ), L' ' );
		}

		m_Buffer.append( ::ToUnicode( text, len ) );

		if ( endLine && UIShell::DoesSingletonExist() )
		{
			// send to game console
			UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box" );
			if ( pChat )
			{
				pChat->SubmitText( m_Buffer, pChat->GetFontColor() );
			}
		}

		return ( true );
	}

private:
	gpwstring m_Buffer;

	SET_NO_COPYING( GameConsoleSink );
};

//////////////////////////////////////////////////////////////////////////////
// class DirectRenderSink declaration

class DirectRenderSink : public ReportSys::StreamSink
{
public:
	SET_INHERITED( DirectRenderSink, ReportSys::StreamSink );

	// ctor/dtor
	DirectRenderSink( RapiFont* font, int startX, int startY, DWORD color = COLOR_WHITE )
		: Inherited( "DirectRenderSink" )
	{
		SetType( "DirectRenderSink" );

		gpassert( font != NULL );

		m_Font   = font;
		m_StartX = startX;
		m_StartY = startY;
		m_IterY  = startY;
		m_Color  = color;

		int dummy;
		m_Font->CalculateStringSize( "W", dummy, m_Height );
	}

	// overrides
	virtual bool OutputString( const ReportSys::Context* context, const char* text, int len,
							   bool startLine, bool endLine )
	{
		if ( startLine )
		{
			m_Buffer.assign( GetIndentSpaces( context ), L' ' );
		}

		m_Buffer.append( text, len );

		if ( endLine )
		{
			m_Font->Print( m_StartX, m_IterY, m_Buffer, context->IsColorDefault() ? m_Color : context->GetColor() );
			m_IterY += m_Height;
		}

		return ( true );
	}

private:
	RapiFont* m_Font;
	int       m_StartX, m_StartY, m_IterY, m_Height;
	DWORD     m_Color;
	gpstring  m_Buffer;

	SET_NO_COPYING( DirectRenderSink );
};

//////////////////////////////////////////////////////////////////////////////
// class gpc_mman declaration and implementation

#if !GP_RETAIL

#if GPMEM_DBG_NEW
gpmem::eSortType s_MemSortType = gpmem::SORT_BYSIZE;
#endif // GP_DEBUG

class gpc_mman : public GpConsole_command
{
public:
	gpc_mman();
   ~gpc_mman(){}
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()				{ return( __FILE__ ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 2 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_mman::gpc_mman()
{
	gpstring help;
	std::vector< gpstring > usage;
	help = "mman - toggle real-time allocation info\n";
	help += "mman <size|count|wastesize|wasteratio> - change sort order of output\n";
	help += "mman <debug|normal> - set safety level";

	usage.push_back( "mman size" );
	usage.push_back( "mman count" );
	usage.push_back( "mman wastesize" );
	usage.push_back( "mman wasteratio" );
	usage.push_back( "mman debug" );
	usage.push_back( "mman normal" );

	// register the command
	gGpConsole.RegisterCommand( *this, "mman", help, usage );
}

bool gpc_mman::Execute( GpConsole_command_arguments & args, gpstring & /*output*/ )
{
	bool processed = false;

	if( args.Size() > 1 )
	{
		if( args.GetAsString( 1 ).same_no_case( "size" ) ) {
#			if GPMEM_DBG_NEW
			s_MemSortType = gpmem::SORT_BYSIZE;
#			endif
			processed = true;
		}
		else if( args.GetAsString( 1 ).same_no_case( "count" ) ) {
#			if GPMEM_DBG_NEW
			s_MemSortType = gpmem::SORT_BYCOUNT;
#			endif
			processed = true;
		}
		else if( args.GetAsString( 1 ).same_no_case( "wastesize" ) ) {
#			if GPMEM_DBG_NEW
			s_MemSortType = gpmem::SORT_BYWASTESIZE;
#			endif
			processed = true;
		}
		else if( args.GetAsString( 1 ).same_no_case( "wasteratio" ) ) {
#			if GPMEM_DBG_NEW
			s_MemSortType = gpmem::SORT_BYWASTERATIO;
#			endif
			processed = true;
		}
		else if( args.GetAsString( 1 ).same_no_case( "debug" ) ) {
#			if GPMEM_DBG_NEW
			gpmem::SetDebugMax();
#			endif
			processed = true;
		}
		else if( args.GetAsString( 1 ).same_no_case( "normal" ) ) {
#			if GPMEM_DBG_NEW
			gpmem::SetDebugNormal();
#			endif
			processed = true;
		}
	}
	else
	{
		gTattooGame.ToggleOptions( TattooGame::GOPTION_TRACE_MEMORY );
	}

	return( processed );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class gpc_gpstats declaration and implementation

#if !GP_RETAIL && GP_ENABLE_STATS

class gpc_gpstats : public GpConsole_command
{
public:
	gpc_gpstats();
   ~gpc_gpstats(){}
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()				{ return( __FILE__ ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 2 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_gpstats::gpc_gpstats()
{
	gpstring help;
	std::vector< gpstring > usage;
	help = "gpstats - toggle real-time general stats\n";

	// register the command
	gGpConsole.RegisterCommand( *this, "gpstats", help, usage );
}

bool gpc_gpstats::Execute( GpConsole_command_arguments & args, gpstring & /*output*/ )
{
	bool processed = false;

	if( args.Size() > 1 )
	{
	}
	else
	{
		gTattooGame.ToggleOptions( TattooGame::GOPTION_TRACE_STATS );
		gGpStatsMgr.Enable( gTattooGame.TestOptions( TattooGame::GOPTION_TRACE_STATS ) );
	}

	return( processed );
}

#endif // !GP_RETAIL && GP_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////
// class gpc_file declaration and implementation

#if !GP_RETAIL

class gpc_file : public GpConsole_command
{
public:
	gpc_file( void );

	UINT32       GetFlags       ( void )		{  return ( 0 );  }
	gpstring     GetLocation    ( void )		{  return ( __FILE__ );  }
	unsigned int GetMinArguments( void )		{  return ( 1 );  }
	unsigned int GetMaxArguments( void )		{  return ( 3 );  }

	bool Execute( GpConsole_command_arguments& args, gpstring& output );

private:
	FileSys::CmdProcessor m_Processor;
};

gpc_file :: gpc_file( void )
{
	gpstring help;
	std::vector< gpstring > usage;

	help += "file <command> - for querying the file system\n\n";
	usage.push_back( "file" );

	help += "file dir [pattern] - list current directory (wildcards ok)\n";
	usage.push_back( "file dir" );

	help += "file cd <dir>      - enter directory\n";
	usage.push_back( "file cd" );

	gGpConsole.RegisterCommand( *this, "file", help, usage );
}

bool gpc_file :: Execute( GpConsole_command_arguments& args, gpstring& output )
{
	// one argument
	if ( args.Size() <= 1 )
	{
		return ( true );
	}

	gpstring cmd = args.GetAsString( 1 );
	if ( cmd.same_no_case( "dir" ) )
	{
		if ( args.Size() == 3 )
		{
			// do dir
			if ( !m_Processor.Dir( args.GetAsString( 2 ) ) )
			{
				output += "error: invalid pattern\n";
			}
		}
		else if ( args.Size() != 2 )
		{
			output += "error: wrong number of args\n";
		}
	}
	else if ( cmd.same_no_case( "cd" ) )
	{
		if ( args.Size() == 3 )
		{
			if ( !m_Processor.Cd( args.GetAsString( 2 ) ) )
			{
				output += "error: invalid path\n";
			}
		}
		else if ( args.Size() != 2 )
		{
			output += "error: wrong number of args\n";
		}
	}

	return ( true );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class gpc_log declaration and implementation

#if !GP_RETAIL

class gpc_log : public GpConsole_command
{
public:
	gpc_log( void );

	UINT32       GetFlags       ( void )		{  return ( 0 );  }
	gpstring     GetLocation    ( void )		{  return ( __FILE__ );  }
	unsigned int GetMinArguments( void )		{  return ( 2 );  }
	unsigned int GetMaxArguments( void )		{  return ( 2 );  }

	bool Execute( GpConsole_command_arguments& args, gpstring& output );
};

gpc_log :: gpc_log( void )
{
	gpstring help;
	std::vector< gpstring > usage;

	help += "log <command> - for modifying the log system\n\n";
	usage.push_back( "log" );

	help += "log all - toggle logging of all output to log file\n";
	usage.push_back( "log all" );

	gGpConsole.RegisterCommand( *this, "log", help, usage );
}

bool gpc_log :: Execute( GpConsole_command_arguments& args, gpstring& output )
{
	// one argument
	if ( args.Size() <= 1 )
	{
		return ( true );
	}

	gpstring cmd = args.GetAsString( 1 );
	if ( cmd.same_no_case( "all" ) )
	{
		bool enable = !gServices.IsLogAllEnabled();
		output.appendf( "%s all output to log file\n", enable ? "Enabling" : "Disabling" );
		gServices.EnableLogAll( enable );
	}

	return ( true );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class gpc_reload declaration and implementation

#if !GP_RETAIL

class gpc_reload : public GpConsole_command
{
public:
	gpc_reload( void );

	UINT32       GetFlags       ( void )		{  return ( 0 );  }
	gpstring     GetLocation    ( void )		{  return ( __FILE__ );  }
	unsigned int GetMinArguments( void )		{  return ( 2 );  }
	unsigned int GetMaxArguments( void )		{  return ( 2 );  }

	bool Execute( GpConsole_command_arguments& args, gpstring& output );
};

gpc_reload :: gpc_reload( void )
{
	gpstring help;
	std::vector< gpstring > usage;

	help += "reload <command> - for reloading various parts of the world\n\n";
	usage.push_back( "reload" );

	help += "reload gos - reload just the game objects\n";
	usage.push_back( "reload gos" );

	help += "reload contentdb - reload the contentdb, includes templates etc.\n";
	usage.push_back( "reload contentdb" );

	help += "reload world - reload the entire world\n";
	usage.push_back( "reload world" );

	help += "reload full - reload EVERYTHING\n";
	usage.push_back( "reload full" );

	help += "reload skritbots - reload the skrit bots\n";
	usage.push_back( "reload skritbots" );

	help += "reload moods - reload the moods\n";
	usage.push_back( "reload moods" );

	gGpConsole.RegisterCommand( *this, "reload", help, usage );
}

bool gpc_reload :: Execute( GpConsole_command_arguments& args, gpstring& output )
{
	// one argument
	if ( args.Size() <= 1 )
	{
		return ( true );
	}

	gpstring cmd = args.GetAsString( 1 );
	if ( cmd.same_no_case( "gos" ) )
	{
		output += "Reloading objects...\n";
		gTattooGame.ReloadObjects();
	}
	else if ( cmd.same_no_case( "contentdb" ) )
	{
		output += "Reloading the ContentDb...\n";
		gTattooGame.ReloadContentDb();
	}
	else if ( cmd.same_no_case( "world" ) )
	{
		output += "Reloading the world...\n";
		gTattooGame.ReloadWorld( false );
	}
	else if ( cmd.same_no_case( "full" ) )
	{
		output += "Reloading EVERYTHING...\n";
		gTattooGame.ReloadWorld( true );
	}
	else if ( cmd.same_no_case( "skritbots" ) )
	{
		output += "Reloading all Skritbots...\n";
		gSkritBotMgr.Reload();
	}
	else if ( cmd.same_no_case( "moods" ) )
	{
		output += "Reloading all moods...\n";
		gMood.InitMoods( true );
	}

	return ( true );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class gpc_streamer declaration and implementation

#if !GP_RETAIL

class gpc_streamer : public GpConsole_command
{
public:
	gpc_streamer( void );

	UINT32       GetFlags       ( void )		{  return ( 0 );  }
	gpstring     GetLocation    ( void )		{  return ( __FILE__ );  }
	unsigned int GetMinArguments( void )		{  return ( 2 );  }
	unsigned int GetMaxArguments( void )		{  return ( 2 );  }

	bool Execute( GpConsole_command_arguments& args, gpstring& output );
};

gpc_streamer :: gpc_streamer( void )
{
	gpstring help;
	std::vector< gpstring > usage;

	help += "streamer <command> - for modifying the world streamer\n\n";
	usage.push_back( "streamer" );

	help += "streamer on - enable the world streamer\n";
	usage.push_back( "streamer on" );

	help += "streamer off - disable the world streamer\n";
	usage.push_back( "streamer off" );

	help += "streamer forceload - force a world load right now\n";
	usage.push_back( "streamer forceload" );

	gGpConsole.RegisterCommand( *this, "streamer", help, usage );
}

bool gpc_streamer :: Execute( GpConsole_command_arguments& args, gpstring& output )
{
	// one argument
	if ( args.Size() <= 1 )
	{
		return ( true );
	}

	gpstring cmd = args.GetAsString( 1 );
	if ( cmd.same_no_case( "on" ) )
	{
		output += "Enabling the world streamer\n";
		gSiegeEngine.StartLoadingThread();
	}
	else if ( cmd.same_no_case( "off" ) )
	{
		output += "Disabling the world streamer\n";
		gSiegeEngine.StopLoadingThread();
	}
	else if ( cmd.same_no_case( "forceload" ) )
	{
		gWorldTerrain.ForceWorldLoad();
	}

	return ( true );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class gpc_fps declaration and implementation

#if !GP_RETAIL

class gpc_fps : public GpConsole_command
{
public:
	gpc_fps( void );

	UINT32       GetFlags       ( void )		{  return ( 0 );  }
	gpstring     GetLocation    ( void )		{  return ( __FILE__ );  }
	unsigned int GetMinArguments( void )		{  return ( 2 );  }
	unsigned int GetMaxArguments( void )		{  return ( 3 );  }

	bool Execute( GpConsole_command_arguments& args, gpstring& output );
};

gpc_fps :: gpc_fps( void )
{
	gpstring help;
	std::vector< gpstring > usage;

	usage.push_back( "fps sim" );
	usage.push_back( "fps sim off" );

	gGpConsole.RegisterCommand( *this, "fps", help, usage );
}

bool gpc_fps :: Execute( GpConsole_command_arguments& args, gpstring& /*output*/ )
{
	// one argument
	if ( args.Size() <= 1 )
	{
		return ( true );
	}

	gpstring cmd = args.GetAsString( 1 );
	if ( cmd.same_no_case( "sim" ) )
	{
		if ( args.Size() <= 2 )
		{
			if ( ::IsPositive( gTattooGame.GetSimFrameRate() ) )
			{
				gpgenericf(( "Sim frame rate is %g Hz\n", gTattooGame.GetSimFrameRate() ));
			}
			else
			{
				gpgenericf(( "Sim frame rate is infinite\n" ));
			}
		}
		else
		{
			gpstring subcmd = args.GetAsString( 2 );
			float fps = 0;
			if ( subcmd.same_no_case( "off" ) )
			{
				gTattooGame.SetSimFrameRate( 0 );
			}
			else if ( ::FromString( subcmd, fps ) && (fps >= 0) )
			{
				gTattooGame.SetSimFrameRate( fps );
			}
		}
	}
	else if ( cmd.same_no_case( "fixed" ) )
	{
		if ( args.Size() <= 2 )
		{
			if ( ::IsPositive( gTattooGame.GetFixedFrameRate() ) )
			{
				gpgenericf(( "Fixed frame rate is %g Hz\n", gTattooGame.GetFixedFrameRate() ));
			}
			else
			{
				gpgenericf(( "Fixed frame rate is infinite\n" ));
			}
		}
		else
		{
			gpstring subcmd = args.GetAsString( 2 );
			float fps = 0;
			if ( subcmd.same_no_case( "off" ) )
			{
				gTattooGame.SetFixedFrameRate( 0 );
			}
			else if ( ::FromString( subcmd, fps ) && (fps >= 0) )
			{
				gTattooGame.SetFixedFrameRate( fps );
			}
		}
	}
	else if ( cmd.same_no_case( "throttle" ) )
	{
		if ( args.Size() <= 2 )
		{
			if ( ::IsPositive( gTattooGame.GetThrottleFrameRate() ) )
			{
				gpgenericf(( "Throttle frame rate is %g Hz\n", gTattooGame.GetThrottleFrameRate() ));
			}
			else
			{
				gpgenericf(( "Throttle frame rate is infinite\n" ));
			}
		}
		else
		{
			gpstring subcmd = args.GetAsString( 2 );
			float fps = 0;
			if ( subcmd.same_no_case( "off" ) )
			{
				gTattooGame.SetThrottleFrameRate( 75 );
			}
			else if ( ::FromString( subcmd, fps ) && (fps > 0) )
			{
				gTattooGame.SetThrottleFrameRate( fps );
			}
		}
	}
	else if ( cmd.same_no_case( "inactive" ) )
	{
		if ( args.Size() <= 2 )
		{
			if ( ::IsPositive( gTattooGame.GetInactiveUpdateRate() ) )
			{
				gpgenericf(( "Inactive update rate is %g Hz\n", gTattooGame.GetInactiveUpdateRate() ));
			}
			else
			{
				gpgenericf(( "Inactive update rate is infinite\n" ));
			}
		}
		else
		{
			gpstring subcmd = args.GetAsString( 2 );
			float fps = 0;
			if ( subcmd.same_no_case( "off" ) )
			{
				gTattooGame.SetInactiveUpdateRate( 0 );
			}
			else if ( ::FromString( subcmd, fps ) && (fps >= 0) )
			{
				gTattooGame.SetInactiveUpdateRate( fps );
			}
		}
	}

	return ( true );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// random helper implementations

bool ShouldAutoPauseGame( void )
{
	return (
		   ::IsMultiPlayer()
		&& WorldState::DoesSingletonExist() && ::IsInGame( gWorldState.GetCurrentState() )
		&& (!Singleton <NetFuBiSend>::DoesSingletonExist() || gNetFuBiSend.IsForceUpdateEnabled())
		&& !gFuBiSysExports.IsSending()
		&& IsPrimaryThread() );
}

void MPPauseGameOnMessageBoxCb( bool forError )
{
	static bool s_Recursive = false;

	if ( ShouldAutoPauseGame() && !s_Recursive && forError )
	{
		s_Recursive = true;
//$$$	this causes too damn many problems with deadlocks RAAARRRRR fix it later -sb
//		gTattooGame.RSUserPause( true );
		s_Recursive = false;
	}
}

//////////////////////////////////////////////////////////////////////////////
// class PieMaker implementation

// Casualty of the DX8 conversion, will have to revisit
//#if GP_ENABLE_STATS
#if 0

class PieMaker
{
public:
	PieMaker( GSize size )
	{
		m_Size   = size;
		m_Runner = -0.25f;
		m_Surf   = NULL;
		m_Dc     = NULL;
	}

   ~PieMaker( void )
	{
		if ( m_Surf && m_Dc )
		{
			m_Surf->ReleaseDC( m_Dc );
		}
	}

	bool Init( unsigned int tex )
	{
		m_Surf = gDefaultRapi.GetTexSurface( tex );
		if ( (m_Surf == NULL) || !SUCCEEDED( m_Surf->GetDC( &m_Dc ) ) )
		{
			return ( false );
		}

		::SelectObject( m_Dc, ::GetStockObject( BLACK_PEN ) );

		return ( true );
	}

	void Add( float ratio, COLORREF color )
	{
		if ( ratio >= (1.0f / 360.0f) )
		{
			int x = m_Size.cx / 2, y = m_Size.cy / 2;

			HBRUSH brush = ::CreateSolidBrush( color );
			::BeginPath			( m_Dc );
			::SelectObject		( m_Dc, brush );
			::MoveToEx			( m_Dc, x, y, NULL );
			::AngleArc			( m_Dc, x, y, x - 1, m_Runner * 360.0f, ratio * 360.0f );
			::LineTo			( m_Dc, x, y );
			::EndPath			( m_Dc );
			::StrokeAndFillPath	( m_Dc );
			::SelectObject		( m_Dc, ::GetStockObject( BLACK_BRUSH ) );
			::DeleteObject		( brush );
			m_Runner += ratio;
		}
	}

	void Finish( COLORREF color )
	{
		Add( 0.75f - m_Runner, color );
	}

private:
	GSize  m_Size;
	float  m_Runner;
	LPDDS7 m_Surf;
	HDC    m_Dc;

	SET_NO_COPYING( PieMaker );
};

#endif // GP_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////
// class HistoryMaker declaration

// Casualty of the DX8 conversion, will have to revisit
//#if GP_ENABLE_STATS
#if 0

class HistoryMaker
{
public:
	HistoryMaker( GSize size, int& texIndex )
		: m_TexIndex( texIndex )
	{
		m_Size   = size;
		m_Runner = 0;
		m_Surf   = NULL;

		::ZeroObject( m_Ddsd );
		m_Ddsd.dwSize = sizeof( m_Ddsd );
	}

   ~HistoryMaker( void )
	{
		if ( m_Surf && m_Ddsd.lpSurface )
		{
			m_Surf->Unlock( NULL );

			if ( ++m_TexIndex >= m_Size.cx )
			{
				m_TexIndex = 0;
			}
		}
	}

	bool Init( unsigned int tex )
	{
		m_Surf = gDefaultRapi.GetTexSurface( tex );
		if ( m_Surf == NULL || !SUCCEEDED( m_Surf->Lock( NULL, &m_Ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_NOSYSLOCK | DDLOCK_WRITEONLY, NULL ) ) )
		{
			return ( false );
		}

		return ( true );
	}

	void Add( float ratio, COLORREF color )
	{
		if ( ratio >= (1.0f / m_Size.cy) )
		{
			// go from floor of old to ceil of new
			int y0 = (int)floor( m_Runner * m_Size.cy );
			clamp_min_max( y0, 0, m_Size.cy );
			int y1 = y0 + (int)ceil( ratio * m_Size.cy );
			clamp_min_max( y1, y0, m_Size.cy );
			m_Runner += ratio;

			// get bounds
			ePixelFormat format = ::GetPixelFormat( &m_Ddsd.ddpfPixelFormat );
			void* start = (BYTE*)m_Ddsd.lpSurface + (y0 * m_Ddsd.lPitch) + (m_TexIndex * ::GetSizeBytes( format ));
			void* end = (BYTE*)start + ((y1 - y0) * m_Ddsd.lPitch);

			// draw line
			if ( format == PIXEL_ARGB_8888 )
			{
				DWORD c = MAKE8888FROMCOLORREF( color );
				for ( BYTE* i = (BYTE*)start ; i != end ; i += m_Ddsd.lPitch )
				{
					*((DWORD*)i) = c;
				}
			}
			else if ( format == PIXEL_ARGB_1555 )
			{
				WORD c = MAKE1555FROMCOLORREF( color );
				for ( BYTE* i = (BYTE*)start ; i != end ; i += m_Ddsd.lPitch )
				{
					*((WORD*)i) = c;
				}
			}
		}
	}

	void Finish( float fullRatio, COLORREF color )
	{
		Add( fullRatio - m_Runner, color );
	}

	void Finish( COLORREF color )
	{
		Add( 1.0f - m_Runner, color );
	}

private:
	int&           m_TexIndex;
	GSize          m_Size;
	float          m_Runner;
	DDSURFACEDESC2 m_Ddsd;
	LPDDS7         m_Surf;

	SET_NO_COPYING( HistoryMaker );
};

#endif // GP_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////
// class TattooGame implementation

static void GameInfoCallback( DrWatson::Info& info )
{
	// early out if can't print
	if ( info.m_Writer == NULL )
	{
		return;
	}

	// header
	info.m_Writer->WriteText( "[Game]\n\n" );

	// time info
	info.m_Writer->WriteF(
			"    time: system = %f, global = %f",
			::GetSystemSeconds(),
			::GetGlobalSeconds() );
	if ( WorldTime::DoesSingletonExist() )
	{
		info.m_Writer->WriteF(
				", world = %f, delta = %f, sim = %d",
				gWorldTime.GetTime(),
				gWorldTime.GetSecondsElapsed(),
				gWorldTime.GetSimCount() );
	}
	info.m_Writer->WriteText( "\n\n" );

	// module dump
	if ( FuBi::SysExports::DoesSingletonExist() )
	{
		// header
		info.m_Writer->WriteText( "[Modules]\n\n" );

		const FuBi::ModuleColl& modules = gFuBiSysExports.GetModuleColl();
		for ( int i = 0, count = modules.size() ; i < count ; ++i )
		{
			info.m_Writer->WriteF(
					"    name = '%s', crc = 0x%08X, extra = 0x%08X\n",
					modules[ i ].m_Name.c_str(),
					modules[ i ].m_ActualCrc,
					modules[ i ].m_HeaderCrc );
		}
		info.m_Writer->WriteText( "\n" );
	}
}

TattooGame :: TattooGame( void )
{
	// basic options
#	if GP_DEBUG
	SetOptions( OPTION_WINDOWED_DEF );
#	else // GP_DEBUG
	SetOptions( OPTION_FULLSCREEN_DEF );
#	endif // GP_DEBUG

	// activate special options
	SetOptions( OPTION_GAME_STYLE_WND | OPTION_LOCALIZE | OPTION_FAST_FTOL | OPTION_ENABLE_MINFPS );
	ClearOptions( OPTION_PERSIST_SIZE_WND );

	// local options
	m_GOptions = GOPTION_NONE;

	// dev options
#	if !GP_RETAIL
	SetOptions( GOPTION_SCREENSHOT_STAMP );
	SetOptions( GOPTION_TRACE_CPUSTATS_TABLE | GOPTION_TRACE_CPUSTATS_GRAPH | GOPTION_TRACE_CPUSTATS_HISTORY );
#	endif // !GP_RETAIL

	// configure config
	GetConfigInit().SetProductName( DS1_APP_NAME );
	GetConfigInit().SetMyDocuments();
	GetConfigInit().m_IniFileName  = DS1_INI_NAME;
	GetConfigInit().m_RegistryName = DS1_INI_NAME;
	GetConfigInit().m_LogSubDir    = "Logs";

	// startup the game at 8x6 always
	SetStartupRect( GRect( 0, 0, 800, 600 ) );

	// keep a minimum frame rate going to avoid trigger issues
	SetMinFrameRate( 6.0f );

	// init members
	m_InPauseRequestScope = false;
	m_RenderSkipCount     = 0;
	m_IsSkippingTime      = false;
	m_IsSoundFaded        = false;
	m_CopyFullMovieFrame  = false;
	m_SimFrameRate        = 0;
	m_SimFramePending     = 0;

	// preinit owned objects
#	if !GP_RETAIL
	m_GpProfiler			= NULL;
#	endif // !GP_RETAIL
	m_GpConsole				= NULL;
	m_Services				= NULL;
	m_GameState				= NULL;
	m_GameMovie				= NULL;
	m_World					= NULL;
	m_UI					= NULL;
	m_GameConsoleSink		= NULL;

#	if GP_ENABLE_STATS
	m_CpuStatsGraphTex		= 0;
	m_CpuStatsPHistoryTex	= 0;
	m_CpuStatsUHistoryTex	= 0;
#	endif // GP_ENABLE_STATS

#	if !GP_RETAIL
#	if GP_ENABLE_STATS
	m_GpcGpStats			= NULL;
#	endif // GP_ENABLE_STATS
	m_GpcMman				= NULL;
	m_GpcFile				= NULL;
	m_GpcLog				= NULL;
	m_GpcReload				= NULL;
	m_GpcStreamer			= NULL;
	m_GpcFps				= NULL;
#	endif // !GP_RETAIL

	m_MPSleep				= 0;

	ReportSys::RegisterPreMessageBoxHook( MPPauseGameOnMessageBoxCb );
	DrWatson::RegisterInfoCallback( makeFunctor( GameInfoCallback ) );
}

TattooGame :: ~TattooGame( void )
{
	// destruction of objects happens in shutdown routine
	ReportSys::UnregisterMessageBoxHook( MPPauseGameOnMessageBoxCb );
}

FUBI_DECLARE_SELF_TRAITS( GameState );

bool TattooGame :: Xfer( FuBi::PersistContext& persist )
{
	if ( !Inherited::Xfer( persist ) )
	{
		return ( false );
	}

	persist.Xfer( "m_SimFrameRate", m_SimFrameRate );
	persist.Xfer( "m_GameState",    *m_GameState   );

	bool gameWasPaused = gUIMenuManager.GetGameWasPaused();
	persist.Xfer( "gameWasPaused", gameWasPaused );

	if ( persist.IsRestoring() )
	{
		UserPause( gameWasPaused );
		gUIMenuManager.GuiPause( gameWasPaused );
	}

	return ( true );
}

void TattooGame :: OnOptionsChanged( void )
{
#	if !GP_RETAIL
	if ( Services::DoesSingletonExist() )
	{
		gServices.GetOutputConsole().SetBottomUp( !TestOptions( GOPTION_TRACE_MEMORY | GOPTION_TRACE_PROFILER | GOPTION_TRACE_STATS ) );
	}
#	endif // !GP_RETAIL
}

bool TattooGame :: SkipTime( float seconds )
{
	// save and disable
	bool oldLog = TestOptions( OPTION_ENABLE_LOG_FPS );
	ClearOptions( OPTION_ENABLE_LOG_FPS );

	// verify and lock
	gpassert( !IsSkippingTime() );
	m_IsSkippingTime = true;
	bool oldFreezeDetect = TestOptions( OPTION_FREEZE_TIME_DETECT );
	ClearOptions( OPTION_FREEZE_TIME_DETECT );

	// advance time
	::SetGlobalSecondsAdjust( ::GetGlobalSecondsAdjust() + seconds );

	// skip next frame
	SetRenderSkip( 1 );

	// do it now
	bool rc = OnUpdate();

	// done, unlock
	SetOptions( OPTION_FREEZE_TIME_DETECT, oldFreezeDetect );
	m_IsSkippingTime = false;

	// save and disable
	SetOptions( OPTION_ENABLE_LOG_FPS, oldLog );

	return ( rc );
}

void TattooGame :: RenderFrame( bool includeGui )
{
	AutoHideGui autoHider( !includeGui );

	// Make sure camera orientation is up to date
	gSiegeEngine.GetCamera().BuildOrientations();

	// Update the frame
	UpdateFrame( 0, 0, false, false );
}

void TattooGame :: ExitGame( void )
{
	gWorldStateRequest( WS_DEINIT );
	RequestQuit( true );
}

bool TattooGame :: CheckCdInDrive( void )
{
	// special: existence of this file means bypass copy protection check
	if ( HasDungeonSiegeSpecialSkipCdCheckFile() )
	{
		// ...unless we really want to check it for sure
		if ( !gConfig.GetBool( "force_cdcheck", false ) )
		{
			return ( true );
		}
	}

	// simply test to see if cd is in drive. in case there are multiple cd
	// drives or a changer, be sure to always check the most recently successful
	// disc drive first (from config), or first in sequence if none in prefs.
	// once the disc is found, verify it's cool with safedisc lt.

	// get all available drives
	DWORD numBytes = ::GetLogicalDriveStrings( 0, NULL );
	char* buffer = (char*)::_alloca( numBytes );
	::GetLogicalDriveStrings( numBytes, buffer );

	// iterate over them and extract drives
	typedef std::list <gpstring> StringList;
	StringList drives;
	{
		for ( char* i = buffer ; *i != '\0' ; i += ::strlen( i ) + 1 )
		{
			drives.push_back( i );
		}
	}

	// look for previously successful one
	gpstring lastGoodCdrom = gConfig.GetString( "last_good_cdrom" );
	for ( StringList::iterator i = drives.begin() ; i != drives.end() ; ++i )
	{
		if ( lastGoodCdrom.same_no_case( *i ) )
		{
			drives.splice( drives.begin(), drives, i );
			break;
		}
	}

	// now iterate through our cdrom's until we find it
	for ( i = drives.begin() ; i != drives.end() ; ++i )
	{
		if ( ::GetDriveType( *i ) == DRIVE_CDROM )
		{
			char volumeName[ 50 ];
			if ( !::GetVolumeInformation( *i, volumeName, ELEMENT_COUNT( volumeName ), NULL, NULL, NULL, NULL, 0 ) )
			{
				continue;
			}

			// match? accept a safedisc-labelled "key disc" too for testing
			if ( !::same_no_case( volumeName, "DSIEGE1" ) )
			{
				continue;
			}

			// first try to attach to safedisc poo
			PooDll dll( *i );
			if ( !dll.Load( false ) )
			{
				// must not be installed right...dll missing or something
				continue;
			}

			// ok now try to init poo
			if ( !dll.LTDLL_Initialise( LTDLL_VERSIONNUMBER ) )
			{
				// bad version or something, kill it again
				continue;
			}

			// now see if it's our disco
			DWORD reason = 0;
			if ( !dll.LTDLL_Authenticate( *i, &reason ) )
			{
				continue;
			}

			// remember it for next time
			gConfig.Set( "last_good_cdrom", *i );

			// success!
			return ( true );
		}
	}

	// must be user error eh
	return ( false );
}

bool TattooGame :: LoadCameraSettings( const char* camBlock )
{
	// Set the correct camera mode as requested on the command line
	gpstring camera_mode	= gConfig.GetString( "camera_mode", "power" );
	if ( camera_mode.same_no_case( "follow" ) )
	{
		gUICamera.SetFollowCam( true );
	}
	else if ( camera_mode.same_no_case( "power" ) )
	{
		gUICamera.SetFollowCam( false );
	}

	FastFuelHandle hCamera( camBlock );
	if ( hCamera.IsValid() )
	{
		// Set the tracking position
		gUICamera.SetXScreenTrack( hCamera.GetFloat( "camera_x_screen_track" ) );
		gUICamera.SetYScreenTrack( hCamera.GetFloat( "camera_y_screen_track" ) );

		// Set the historic affect
		gCameraAgent.SetHistoricalAffect( hCamera.GetFloat( "camera_historic_affect", 0.5f ) );

		// Set affector times
		gCameraAgent.SetCameraAffectorTime( hCamera.GetFloat( "camera_affector_time" ) );
		gCameraAgent.SetTargetAffectorTime( hCamera.GetFloat( "target_affector_time" ) );

		// Set the maximum offset clamp
		gCameraAgent.SetCameraOffsetMaximum( hCamera.GetFloat( "camera_offset_maximum" ) );

		// Orbit degrees per second
		gUICamera.SetOrbitDegreesPerSecond( RadiansFrom( hCamera.GetFloat( "orbit_degrees_per_second", 60.0f ) ) );

		// Azimuth degrees per second
		gUICamera.SetAzimuthDegreesPerSecond( RadiansFrom( hCamera.GetFloat( "azimuth_degrees_per_second", 30.0f ) ) );
		gUICamera.SetReturnDegreesPerSecond( RadiansFrom( hCamera.GetFloat( "return_degrees_per_second", 30.0f ) ) );

		// Zoom meters per second
		gUICamera.SetZoomMetersPerSecond( hCamera.GetFloat( "zoom_meters_per_second", 8.0f ) );

		// Meters until retrack
		gUICamera.SetMetersUntilTrackChange( hCamera.GetFloat( "meters_until_retrack", 3.0f ) );

		// Zoom delta percentage
		gUICamera.SetZoomDelta( hCamera.GetFloat( "zoom_delta_percentage", 0.1f ) );

		// Tracking height offset
		gUICamera.SetTrackHeightOffset( hCamera.GetFloat( "tracking_height_offset", 0.0f ) );

		// Camera avoidance buffer time
		gUICamera.SetCameraAvoidanceTime( hCamera.GetFloat( "camera_avoid_time", 0.0f ) );

		// Camera tilt buffer time
		gUICamera.SetCameraTiltTime( hCamera.GetFloat( "camera_tilt_time", 0.0f ) );

		// Camera tilt return buffer time
		gUICamera.SetCameraTiltReturnTime( hCamera.GetFloat( "camera_tilt_return_time", 0.0f ) );

		// Camera leash length for hold mode
		gUICamera.SetHoldLeashDistance( hCamera.GetFloat( "camera_hold_leash_dist", 5.0f ) );

		// Screen buffer edges
		gUIGame.SetHorizontalScreenEdgeBuffer( hCamera.GetFloat( "horizontal_cursor_buffer", 0.98f ) );
		gUIGame.SetVerticalScreenEdgeBuffer( hCamera.GetFloat( "vertical_cursor_buffer", 0.98f ) );

		return true;
	}

	return false;
}

bool TattooGame :: ScreenShot( bool frontBuffer, const wchar_t* fileName, gpwstring* outFileName )
{
	// build local out filename
	gpwstring localOutFileName;
	if ( outFileName == NULL )
	{
		outFileName = &localOutFileName;
	}

	// build filename if empty
	gpwstring localFileName;
	if ( (fileName == NULL) || (*fileName == L'\0') )
	{
#		if GP_RETAIL
		localFileName = gpwtranslate( $MSG$ "Dungeon Siege Screen - " );
#		else
		localFileName = L"s_s_";
#		endif
		fileName = localFileName;
	}

	// do it
	bool rc = Inherited::ScreenShot( frontBuffer, fileName, outFileName );
	if ( rc )
	{
		if ( !IsCapturingMovie() )
		{
			gpscreenf(( $MSG$ "Screenshot successfully taken to '%S'\n", FileSys::GetFileName( *outFileName ) ));
		}
	}
	else
	{
		gpscreenf(( $MSG$ "Failed to take screenshot into '%S'\n", FileSys::GetFileName( *outFileName ) ));
	}

	return ( rc );
}

bool TattooGame :: ScreenShotDefaultNoGui( void )
{
	RenderFrame( false );
	return ( ScreenShotDefault() );
}

bool TattooGame :: CopyScreenShotToClipboardNoGui( void )
{
	RenderFrame( false );
	return ( CopyScreenShotToClipboard() );
}

bool TattooGame :: SaveGame( const wchar_t* saveName )
{
	bool rc = PrivateSaveGame( saveName, false, false );
	if ( rc )
	{
		gpscreenf(( $MSG$ "Game '%S' saved successfully\n", saveName ));
	}
	else
	{
		gpscreenf(( $MSG$ "Game '%S' failed to save\n", saveName ));
	}

	return ( rc );
}

bool TattooGame :: QuickSaveGame( void )
{
	bool rc = PrivateSaveGame( NULL, true, false );
	if ( rc )
	{
		gpscreen( $MSG$ "Quick Save completed\n" );
	}
	else
	{
		gpscreen( $MSG$ "Quick Save failed\n" );
	}

	return ( rc );
}

bool TattooGame :: AutoSaveGame( void )
{
	if ( gWorldOptions.GetAllowAutoSave() )
	{
		gUIMenuManager.AutoSave();
	}

	return ( true);
}

bool TattooGame :: AutoSaveGameNow( void )
{
	bool rc = PrivateSaveGame( NULL, false, true );
	if ( rc )
	{
		gpscreen( $MSG$ "Auto Save completed\n" );
	}
	else
	{
		gpscreen( $MSG$ "Auto Save failed\n" );
	}

	return ( rc );
}

bool TattooGame :: SaveParty( const wchar_t* saveName, bool outputStatus )
{
	GPSTATS_ONLY( gpstats::SetCurrentStage( STAGE_SAVE_GAME ) );

	GameSaver saver( GameSaver::OPTION_PARTY_SAVE );
	bool rc = saver.BeginSave( saveName );
	if ( rc )
	{
		gpwstring fullSaveName = FileSys::GetFileName( saver.GetSaveName() );

		rc = saver.FullPartySave();
		if ( rc )
		{
			FuelHandle prefs = gTattooGame.GetPrefsConfigFuel( true );
			if ( prefs.IsValid() )
			{
				FuelHandle hSettings = prefs->GetChildBlock( "player", true );
				if ( hSettings.IsValid() )
				{
					hSettings->Set( "last_party_save", fullSaveName );
				}
				prefs->GetDB()->SaveChanges();
			}
		}
	}

	GPSTATS_ONLY( gpstats::SetCurrentStage( STAGE_IN_GAME ) );

	if ( outputStatus )
	{
		if ( rc )
		{
			gpscreen( $MSG$ "Party Save completed\n" );
		}
		else
		{
			gpscreen( $MSG$ "Party Save failed\n" );
		}
	}

	return ( rc );
}

bool TattooGame :: SavePartyMember( int slot )
{
	GPSTATS_ONLY( gpstats::SetCurrentStage( STAGE_SAVE_GAME ) );

	// get our character
	Go* party = gServer.GetScreenParty();
	gpassert( (slot >= 0) && (slot < (int)party->GetChildren().size()) );
	Go* member = party->GetChildren()[ slot ];

	// save it
	GameSaver saver( GameSaver::OPTION_PARTY_SAVE );
	bool rc = saver.BeginSave( member->GetCommon()->GetScreenName() );
	if ( rc )
	{
		saver.FullPartySave( slot );
	}

	GPSTATS_ONLY( gpstats::SetCurrentStage( STAGE_IN_GAME ) );

	return ( rc );
}

bool TattooGame :: AutoSaveParty( void )
{
	bool success = true;

	if ( ::IsMultiPlayer() && Server::DoesSingletonExist() && gWorldOptions.GetAllowAutoSaveParty() && gServer.HasScreenParty() )
	{
		gpgeneric( "Auto-saving multiplayer party...\n" );

		int count = (int)gServer.GetScreenParty()->GetChildren().size();
		for ( int i = 0 ; i != count ; ++i )
		{
			if ( !SavePartyMember( i ) )
			{
				success = false;
			}
		}
	}

	return ( success );
}

bool TattooGame :: CheckAutoSaveParty( bool ignoreMinTime )
{
	bool saved = false;

	if ( ::IsMultiPlayer() && Server::DoesSingletonExist() )
	{
		Player* screenPlayer = gServer.GetScreenPlayer();
		if ( (screenPlayer != NULL) && screenPlayer->CheckPartyDirty( ignoreMinTime ) )
		{
			saved = AutoSaveParty();
		}
	}

	return ( saved );
}

bool TattooGame :: LoadGame( const wchar_t* loadName )
{
	return ( PrivateLoadGame( loadName, false, false ) );
}

bool TattooGame :: LoadNewestGame( void )
{
	SaveInfoColl coll;
	if ( GetCampaignSaveGameInfos( coll, true, false ) == 0 )
	{
		return ( false );
	}

	coll.sort( SaveInfo::GreaterByFileTime() );
	const SaveInfo& info = *coll[ 0 ];
	return ( PrivateLoadGame( info.m_FileName, info.m_IsQuickSave, info.m_IsAutoSave ) );
}

bool TattooGame :: LoadRecentGame( void )
{
	return ( PrivateLoadGame( GetLastSaveGame(), false, false ) );
}

bool TattooGame :: QuickLoadGame( void )
{
	return ( PrivateLoadGame( NULL, true, false ) );
}

bool TattooGame :: AutoLoadGame( void )
{
	return ( PrivateLoadGame( NULL, false, true ) );
}

bool TattooGame :: LoadGamePartial( const wchar_t* loadName )
{
	GameXfer::eOptions options = GameXfer::OPTION_WORLD_SAVE;
	if (   ::same_no_case( loadName, (const wchar_t*) L"[quick]" )
		|| ::same_no_case( loadName, (const wchar_t*) L"quick-save" )
		|| ::same_no_case( loadName, (const wchar_t*) L"[quick-save]" ) )
	{
		options |= GameXfer::OPTION_QUICK_SAVE;
		loadName = NULL;
	}
	else if (   ::same_no_case( loadName, (const wchar_t*) L"[auto]" )
			 || ::same_no_case( loadName, (const wchar_t*) L"auto-save" )
			 || ::same_no_case( loadName, (const wchar_t*) L"[auto-save]" ) )
	{
		options |= GameXfer::OPTION_AUTO_SAVE;
		loadName = NULL;
	}

	GameLoader loader( options );
	bool success = loader.BeginLoad( loadName );
	if ( success )
	{
		success = loader.LoadWorldPartial();
		ResetUserInputs();
	}
	else
	{
		gpfatalf(( "Sorry, can't partial load '%S', error is '%s'\n", loadName, stringtool::GetLastErrorText().c_str() ));
	}

	return ( success );
}

bool TattooGame :: HasRecentSaveGame( void )
{
	gpwstring defname = GetLastSaveGame();
	const wchar_t* name = defname;
	GameLoader::eOptions options = GameLoader::MakeSaveOptionsAndName( name, false, false );
	return ( GameLoader::DoesSaveGameExist( name, options ) );
}

bool TattooGame :: HasQuickSaveGame( void ) const
{
	const wchar_t* name = NULL;
	GameLoader::eOptions options = GameLoader::MakeSaveOptionsAndName( name, true, false );
	return ( GameLoader::DoesSaveGameExist( name, options ) );
}

bool TattooGame :: HasAutoSaveGame( void ) const
{
	const wchar_t* name = NULL;
	GameLoader::eOptions options = GameLoader::MakeSaveOptionsAndName( name, false, true );
	return ( GameLoader::DoesSaveGameExist( name, options ) );
}

bool TattooGame :: DeleteSaveGame( const wchar_t* saveName )
{
	GameSaver saver( GameSaver::OPTION_WORLD_SAVE | GameSaver::OPTION_PARTY_SAVE );
	return ( saver.DeleteSave( saveName ) );
}

bool TattooGame :: ImportCharacterFromSave( const wchar_t* saveName, int index )
{
	bool success = false;

	GameLoader loader( GameLoader::OPTION_PARTY_SAVE );
	if ( loader.BeginLoad( saveName ) )
	{
		success = loader.ImportCharacterByIndex( index );
	}

	return ( success );
}

void TattooGame :: RSPrepareTeleportPlayer( PlayerId playerId, const char* where )
{
	FUBI_RPC_CALL( RSPrepareTeleportPlayer, RPC_TO_SERVER );
	
	RCPrepareTeleportPlayer( playerId, where );

	if( gServer.GetLocalHumanPlayer()->GetId() == playerId )
	{
		PrepareTeleportPlayer( playerId, where );
	}
	
}

void TattooGame :: RCPrepareTeleportPlayer( PlayerId playerId, const char* where )
{
	FUBI_RPC_TAG();

	Player* player = gServer.GetPlayer( playerId );
	if ( player != NULL )
	{
		FUBI_RPC_CALL( RCPrepareTeleportPlayer, player->GetMachineId() );

		PrepareTeleportPlayer( playerId, where );
	}
}

void TattooGame :: PrepareTeleportPlayer( PlayerId playerId, const char* where )
{
	SiegePos pos;
	CameraPosition cpos;

	if ( gWorldMap.LoadBookmark( where, pos, cpos ) )
	{
		// this is for the server
		GoHandle playerHandle( gServer.GetPlayer( playerId )->GetHero () );
		gpassert( playerHandle );

		gUICamera.SetTeleportPosition( cpos, pos );		

		if( IsServerLocal() )
		{
			if( ! gMCPManager.BeginTeleport( playerHandle->GetGoid() ) )
			{
				gperror( "HEY! You aren't suppose to put teleporters near triggers! Stuff will break. BAD BAD BAD" );
			}
		}
	}
	else
	{
		gperrorf(( "Illegal teleport destination: '%s'\n" ));
	}
}

void TattooGame :: STeleportPlayer( PlayerId playerId, const char* where, bool teleportSelectedItems )
{
	SiegePos pos;
	CameraPosition cpos;
	if ( gWorldMap.LoadBookmark( where, pos, cpos ) )
	{
		TeleportPlayer( playerId, pos, cpos, teleportSelectedItems );
	}
	else
	{
		gperrorf(( "Illegal teleport destination: '%s'\n" ));
	}
}

void TattooGame :: TeleportPlayer( PlayerId playerId, const SiegePos& pos, bool teleportSelectedItems )
{
	CameraPosition cpos;
	// we are teleporting to a defined teleport location!
	// so we can use the camera
	if( pos == gUICamera.GetTeleportPositionId() )
	{
		// everything a-ok!
		cpos = gUICamera.GetTeleportPosition();
	}
	else
	{
		// this shouldn't happen in regular game play...  but i want to handle it, just incase paul
		// descides to play in debug at .01 fps running through town!
		gperror( "Hey you are teleporting out of the frustum not using an official teleporter! We will try and set a nice camera for you" );
		cpos.cameraPos = pos;
		cpos.targetPos = pos;
		cpos.targetPos.pos.z += 0.1f; // put the camera right above the player for now.
	}

	TeleportPlayer( playerId, pos, cpos, teleportSelectedItems );
}

void TattooGame :: TeleportPlayer( PlayerId playerId, const SiegePos& pos, const CameraPosition& cpos, bool teleportSelectedItems )
{
	// get the player
	Player* player = gServer.GetPlayer( playerId );
	if ( player == NULL )
	{
		gperror( "Can't teleport an invalid player!" ); 
		return;
	}

	if ( player->IsScreenPlayer() )
	{
		// black screen during transition
		gDefaultRapi.ClearAllBuffers();
		gUICamera.SetTeleportPosition( cpos, pos );		
	}

	// create temporary frustum
	FrustumId tempFrustum = MakeFrustumId( siege::SiegeFrustum::GetReservedIdBitfield() );
	gWorldTerrain.RegisterWorldFrustum( playerId, tempFrustum );
	siege::SiegeFrustum* frustum = gSiegeEngine.GetFrustum( MakeInt( tempFrustum ) );
	if ( frustum == NULL )
	{
		gperror( "Can't create temporary frustum! Already in use?" );
		return;
	}
	frustum->SetPosition( pos );

	// force that section of the world to load before we show it
	gWorldTerrain.ForceWorldLoad();

	// get our hero go
	GoHandle hero( gServer.GetPlayer( playerId )->GetHero() );

	// move all other selected things there before the hero goes and moves the
	// frustum. $$ for now, limit this extra stuff to single player because the
	// server doesn't know what the clients have selected, etc.
	GoidColl goids;
	goids.push_back( hero->GetOwningParty()->GetGoid() );

	// make sure to unselect all of our selected items.
	// if we are suppose to teleport them, then add them to the teleport list.
	if ( ::IsSinglePlayer() && teleportSelectedItems )
	{
		GopColl::const_iterator i, ibegin = gGoDb.GetSelectionBegin(), iend = gGoDb.GetSelectionEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			GoHandle go( *i );

			if( go->HasMind() && !go->HasParty() )
			{
				go->GetMind()->ClearAndCommit( JQ_ACTION );
			}

			gGoDb.PauseEffectScripts( *i );

			if ( *i != hero.Get() )
			{
				gpassert( *i != hero->GetParent() );
				goids.push_back( (*i)->GetGoid() );
			}

			GopColl::iterator j, jbegin = (*i)->GetChildBegin(), jend = (*i)->GetChildEnd();
			for ( j = jbegin ; j != jend ; ++j )
			{
				gGoDb.PauseEffectScripts( *j );
			}
		}

		// arrange our objects in new teleport location
		PContentDb::PlaceObjectsInArray( pos, goids, 1.5f );
	}

	// make sure we don't have anything selected anymore (it might not be comming with us!)
	gGoDb.DeselectAll( );

	// flush script pauses
	gWorldFx.Update( 0, 0 );

	// now we can move the hero to our teleport location 
	// we need to set the placement so the frustum will take
	// don't worry, we will clear the mcp later, and in the prepare-teleport
	// function, we prepared the mcp for it's impending doom!
	hero->GetPlacement()->ForceSetPosition( pos );
	hero->GetOwningParty()->GetPlacement()->ForceSetPosition( pos );
	
	// now put the hero go into the goid collection (so it is managed with them)
	goids.push_back( hero->GetGoid() );

	// tell the UICamera to go there
	if ( player->IsScreenPlayer() )
	{
		gUICamera.Teleport();
	}

	// force the frustum change to "take"
	gWorldTerrain.ForceWorldLoad();
	if ( player->IsScreenPlayer() )
	{
		gSiegeEngine.GetMouseShadow().Update( gAppModule.GetNormalizedCursorX(), gAppModule.GetNormalizedCursorY() );
		gUIGame.SetCameraTrackModeOn();
	}
	
	GoidColl::const_iterator i, ibegin = goids.begin(), iend = goids.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		GoHandle go( *i );

		if ( ::IsServerLocal() && go->HasMind() && !go->HasParty() )
		{
			go->GetMind()->SDoJob( MakeJobReq( JAT_STOP, JQ_ACTION, QP_CLEAR, AO_USER ) );
		}

		if ( go->HasFollower() )
		{
			go->GetPlacement()->SetOrientation( Quat::IDENTITY );
			go->GetFollower()->ForceUpdatePositionOrientation();
		}

		if ( ::IsServerLocal() )
		{
			// now we need to see if there is any extra 'bad' stuff
			// if there is assert, because we aren't supppose to teleport
			// with stuff in the mcp queue.
			// then we move the player to the requested location
			gMCPManager.TeleportGo(
					*i,
					go->GetPlacement()->GetPosition(),
					SiegeRot( Quat::IDENTITY, go->GetPlacement()->GetPosition().node ),
					true );
		}

		gGoDb.ResumeEffectScripts( GoHandle( *i ) );

		GopColl::iterator j, jbegin = go->GetChildBegin(), jend = go->GetChildEnd();
		for ( j = jbegin ; j != jend ; ++j )
		{
			gGoDb.ResumeEffectScripts( *j );
		}
	}

	// delete temp frustum, we're done with it now
	gWorldTerrain.DestroyWorldFrustum( tempFrustum );
}

#if !GP_RETAIL

bool TattooGame :: ReloadWorld( bool fullRestart )
{
	// remember old settings
	gpstring mapName = gWorldMap.GetMapName();

// Shut down.

	// ui: shutdown
	gUIPartyManager.DeconstructUIParty();

	// shutdown the world and restart it
	bool success = gWorld.Shutdown( fullRestart ? SHUTDOWN_FULL : SHUTDOWN_FOR_RELOAD_MAP, true );
	gUICamera.Shutdown();

// Reload.

	// reinit single play object
	gWorld.InitServer( false );

	// switch to that
	gWorldMap.Init( mapName, gWorldMap.GetMpWorldName() );

	// now force a reload
	gWorldTerrain.ForceWorldLoad();
	gUIPartyManager.ConstructUIParty();

	// Reload the moods
	gMood.InitMoods( true );

	// return success of initialization
	return ( success );
}

#endif // !GP_RETAIL

gpwstring TattooGame :: GetLastSaveGame( void )
{
	FuelHandle prefs = GetPrefsConfigFuel();
	if ( prefs.IsValid() )
	{
		FuelHandle hSettings = prefs->GetChildBlock( "player" );
		if ( hSettings.IsValid() )
		{
			return ( hSettings->GetWString( "last_save_game" ) );
		}
	}
	return gpwstring::EMPTY;
}

gpwstring TattooGame :: GetLastPartySave( void )
{
	FuelHandle prefs = GetPrefsConfigFuel();
	if ( prefs.IsValid() )
	{
		FuelHandle hSettings = prefs->GetChildBlock( "player" );
		if ( hSettings.IsValid() )
		{
			return ( hSettings->GetWString( "last_party_save" ) );
		}
	}
	return gpwstring::EMPTY;
}

int TattooGame :: GetCampaignSaveGameInfos( SaveInfoColl& coll, bool forRead, bool getThumbs )
{
	return ( GameLoader::GetSaveGameInfos(
				&coll,
				GameLoader::OPTION_WORLD_SAVE | GameLoader::OPTION_SINGLEPLAYER | GameLoader::OPTION_QUICK_SAVE | GameLoader::OPTION_AUTO_SAVE,
				forRead,
				getThumbs ) );
}

bool TattooGame :: HasCampaignSaveGameInfos( bool forRead )
{
	return ( GameLoader::GetSaveGameInfos(
				GameLoader::OPTION_WORLD_SAVE | GameLoader::OPTION_SINGLEPLAYER | GameLoader::OPTION_QUICK_SAVE | GameLoader::OPTION_AUTO_SAVE,
				forRead ) > 0 );
}

bool TattooGame :: HasCampaignSaveGame( const wchar_t* saveName )
{
	return ( GameLoader::DoesSaveGameExist( saveName, GameLoader::OPTION_WORLD_SAVE | GameLoader::OPTION_SINGLEPLAYER ) );
}

int TattooGame :: GetMultiplayerSaveGameInfos( SaveInfoColl& coll, bool forRead, bool getThumbs )
{
	return ( GameLoader::GetSaveGameInfos(
				&coll,
				GameLoader::OPTION_WORLD_SAVE | GameLoader::OPTION_MULTIPLAYER | GameLoader::OPTION_QUICK_SAVE | GameLoader::OPTION_AUTO_SAVE,
				forRead,
				getThumbs ) );
}

bool TattooGame :: HasMultiplayerSaveGameInfos( bool forRead )
{
	return ( GameLoader::GetSaveGameInfos(
				GameLoader::OPTION_WORLD_SAVE | GameLoader::OPTION_MULTIPLAYER | GameLoader::OPTION_QUICK_SAVE | GameLoader::OPTION_AUTO_SAVE,
				forRead ) > 0 );
}

bool TattooGame :: HasMultiplayerSaveGame( const wchar_t* saveName )
{
	return ( GameLoader::DoesSaveGameExist( saveName, GameLoader::OPTION_WORLD_SAVE | GameLoader::OPTION_MULTIPLAYER ) );
}

int TattooGame :: GetPartySaveInfos( SaveInfoColl& coll, bool forRead, bool includeSaveGames, bool getPortraits )
{
	GameLoader::eOptions options = GameLoader::OPTION_PARTY_SAVE;
	if ( includeSaveGames )
	{
		options |= GameLoader::OPTION_WORLD_SAVE;
	}
	return ( GameLoader::GetSaveGameInfos( &coll, options, forRead, getPortraits ) );
}

bool TattooGame :: HasPartySaveInfos( bool forRead, bool includeSaveGames )
{
	GameLoader::eOptions options = GameLoader::OPTION_PARTY_SAVE;
	if ( includeSaveGames )
	{
		options |= GameLoader::OPTION_WORLD_SAVE;
	}
	return ( GameLoader::GetSaveGameInfos( options, forRead ) > 0 );
}

bool TattooGame :: HasPartySave( const wchar_t* saveName, bool includeSaveGames )
{
	return (   GameLoader::DoesSaveGameExist( saveName, GameLoader::OPTION_PARTY_SAVE )
			|| (includeSaveGames && GameLoader::DoesSaveGameExist( saveName, GameLoader::OPTION_WORLD_SAVE )) );
}

int TattooGame :: GetMpCharacterSaveInfos( CharacterInfoColl& coll, bool getPortraits )
{
	return ( GameLoader::GetSaveGameInfos( &coll, GameLoader::OPTION_PARTY_SAVE, false, getPortraits ) );
}

int TattooGame :: GetSavedCharacterInfos( CharacterInfoColl& coll, const gpwstring& saveName, bool getPortraits )
{
	int oldSize = coll.size();

	// query loader
	GameLoader loader;
	if ( loader.BeginLoad( saveName ) )
	{
		loader.GetCharacterInfos( coll, getPortraits );
	}

	// done
	return ( coll.size() - oldSize );
}

bool TattooGame :: IsValidSaveScreenName( const wchar_t* saveName, gpwstring* errorMsg )
{
	bool valid = false;

	if ( FileSys::IsValidFileName( saveName ) )
	{
		if ( ::wcschr( saveName, L'.' ) == NULL )
		{
			SaveInfo info;
			info.m_IsQuickSave = true;
			info.ConvertForScreen();

			if ( !info.m_ScreenName.same_no_case( saveName ) )
			{
				info = SaveInfo();
				info.m_IsAutoSave = true;
				info.ConvertForScreen();

				if ( !info.m_ScreenName.same_no_case( saveName ) )
				{
					valid = true;
				}
			}
		}
	}

	if ( !valid && (errorMsg != NULL) )
	{
		*errorMsg = gpwtranslate( $MSG$ "Sorry, that name is invalid or reserved for use by the system. Please choose another." );
	}

	return ( valid );
}

void TattooGame :: LoadKeyConfig( const wchar_t* configName )
{
	FuelHandle keys( GetKeyConfigFuel( configName ) );
	if ( keys )
	{
		InputBinder* binder = InputBinder::FindInputBinder( "game" );
		if ( binder != NULL )
		{
			binder->ClearBindings();
			binder->BindInputsToPublishedInterfaces( keys );
		}
	}
}

void TattooGame :: SaveKeyConfig( const wchar_t* configName, bool autoSave )
{
	FuelHandle keys = GetKeyConfigFuel( configName, true );
	if ( keys )
	{
		// store key bindings
		InputBinder* binder = InputBinder::FindInputBinder( "game" );
		if ( binder != NULL )
		{
			binder->SaveBindings( keys );
		}

		// save fuel
		if ( autoSave )
		{
			keys->GetDB()->SaveChanges();
		}
	}
}

void TattooGame :: DeleteKeyConfig( const wchar_t* configName, bool autoSave )
{
	FuelHandle keys = GetKeyConfigFuel( configName );
	if ( keys )
	{
		TextFuelDB* fuelDb = keys->GetDB();
		keys->GetParent()->DestroyChildBlock( keys );

		if ( autoSave )
		{
			fuelDb->SaveChanges();
		}
	}
}

FuelHandle TattooGame :: GetKeyConfigFuel( const wchar_t* configName, bool forSaving )
{
	// check for default
	if ( configName == NULL )
	{
		configName = L"current";
	}

	// build path
	FuelHandle base( "::player:keys" );
	FuelHandle keys;
	if ( base )
	{
		gpwstring name;

		// try to find it
		FuelHandleList configs = base->ListChildBlocks( 1, NULL, "bindings" );
		FuelHandleList::const_iterator i, ibegin = configs.begin(), iend = configs.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( (*i)->Get( "name", name ) && name.same_no_case( configName ) )
			{
				keys = *i;
				break;
			}
		}

		// create?
		if ( forSaving )
		{
			if ( keys )
			{
				// clear existing
				keys->ClearKeysAndChildren();
			}
			else
			{
				// create new block
				keys = base->CreateChildBlock( "bindings", ::ToAnsi( configName ) + ".gas" );
			}

			// name it
			name = configName;
			keys->Set( "name", name );
		}
	}

	// done
	return ( keys );
}

void TattooGame :: LoadPrefsConfig()
{
	// Set the default filtering first, this can
	// change from video card to video card, and rapi
	// picks the best choice, so we'll use that as our "default"
	// for the options screen
	gUIOptions.SetDefaultFiltering( gDefaultRapi.GetMipFilter() == D3DTEXF_LINEAR ? "trilinear" : "bilinear" );
	gUIOptions.SetDefaultShadows();

	FuelHandle prefs = GetPrefsConfigFuel();
	if ( prefs.IsValid() )
	{
		// Game Options
		{	bool value;
		if ( prefs->Get( "show_tooltips", value ) )
		{
			gUIShell.SetToolTipsVisible( value );
		}}

		{	bool value;
		if ( prefs->Get( "show_framerate", value ) )
		{
			if ( value != gSiegeEngine.GetOptions().ShowFPSEnabled() )
			{
				gSiegeEngine.GetOptions().ToggleFPS();
			}
		}}

		{	float value;
		if ( prefs->Get( "text_scroll", value ) )
		{
			gUIMenuManager.SetScrollText( value );
		}}

		{	int value;
		if ( prefs->Get( "max_text", value ) )
		{
			gUIMenuManager.SetMaxText( value );
		}}

		{	gpstring value;
		if ( prefs->Get( "blood", value ) )
		{
			if ( value.same_no_case( "red" ) )
			{
				gWorldOptions.SetIsBloodRed( true );
				gWorldOptions.SetIsBloodEnabled( true );
			}
			else if ( value.same_no_case( "green" ) )
			{
				gWorldOptions.SetIsBloodRed( false );
				gWorldOptions.SetIsBloodEnabled( true );
			}
			else if ( value.same_no_case( "disabled" ) )
			{
				gWorldOptions.SetIsBloodRed( true );
				gWorldOptions.SetIsBloodEnabled( false );
			}
		}}

#if !DISABLE_GORE
		{	bool value;
		if ( prefs->Get( "dismemberment", value ) )
		{
			gWorldOptions.SetAllowDismemberment( value );
		}}
#endif

		{	bool value;
		if ( prefs->Get( "priority_boost", value ) )
		{
			gAppModule.SetOptions( OPTION_BOOST_APP_PRIORITY, value );
		}}

		{	bool value;
		if ( prefs->Get( "enable_sell_all_dialog", value ) )
		{
			gUIStoreManager.SetEnableSellAllDialog( value );
		}}

		// Video Options
		{	float value;
		if ( prefs->Get( "video_gamma", value ) )
		{
			gUIShell.GetRenderer().SetGamma( value );
		}}

		{	float value;
		if ( prefs->Get( "object_detail_level", value ) )
		{
			// $$$$ should have a default object detail level that we autodetect
			//  based on cpu, memory and video card. for example, 64 megs of
			//  ram == detail level of 0. 128 megs + 500 mhz == detail level of 0.3.

			gWorldOptions.SetObjectDetailLevel( value );
		}}

		{	gpstring value;
		if ( prefs->Get( "video_shadows", value ) )
		{
			bool bForceSimple = false;
			if ( !gDefaultRapi.IsMultiTexBlend() )
			{
				bForceSimple = true;
			}

			if ( value.same_no_case( "complex" ) && !bForceSimple )
			{
				if ( !gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
				{
					gSiegeEngine.GetOptions().ToggleStaticShadows();
				}
				if ( !gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
				{
					gSiegeEngine.GetOptions().ToggleDynamicShadows();
				}
				if ( !gSiegeEngine.GetOptions().UseExpensiveShadows() )
				{
					gSiegeEngine.GetOptions().ToggleExpensiveShadows();
				}
				if ( gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
				{
					gSiegeEngine.GetOptions().ToggleExpensivePartyShadows();
				}
			}
			else if ( value.same_no_case( "complex_party" ) && !bForceSimple )
			{
				if ( !gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
				{
					gSiegeEngine.GetOptions().ToggleStaticShadows();
				}
				if ( !gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
				{
					gSiegeEngine.GetOptions().ToggleDynamicShadows();
				}
				if ( gSiegeEngine.GetOptions().UseExpensiveShadows() )
				{
					gSiegeEngine.GetOptions().ToggleExpensiveShadows();
				}
				if ( !gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
				{
					gSiegeEngine.GetOptions().ToggleExpensivePartyShadows();
				}
			}
			else if ( value.same_no_case( "simple" ) || (bForceSimple && value.same_no_case( "complex" ))
					  || (bForceSimple && value.same_no_case( "complex_party" )) )
			{
				if ( !gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
				{
					gSiegeEngine.GetOptions().ToggleStaticShadows();
				}
				if ( !gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
				{
					gSiegeEngine.GetOptions().ToggleDynamicShadows();
				}
				if ( gSiegeEngine.GetOptions().UseExpensiveShadows() )
				{
					gSiegeEngine.GetOptions().ToggleExpensiveShadows();
				}
				if ( gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
				{
					gSiegeEngine.GetOptions().ToggleExpensivePartyShadows();
				}
			}
			else
			{
				if ( gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
				{
					gSiegeEngine.GetOptions().ToggleStaticShadows();
				}
				if ( gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
				{
					gSiegeEngine.GetOptions().ToggleDynamicShadows();
				}
				if ( gSiegeEngine.GetOptions().UseExpensiveShadows() )
				{
					gSiegeEngine.GetOptions().ToggleExpensiveShadows();
				}
				if ( gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
				{
					gSiegeEngine.GetOptions().ToggleExpensivePartyShadows();
				}
			}

			// Update shadows
			gGoDb.UpdateShadows();
		}}

		{	gpstring value;
		if ( prefs->Get( "texture_filtering", value ) )
		{
			if ( value.same_no_case( "bilinear" ) )
			{
				gUIShell.GetRenderer().SetMipFilter( D3DTEXF_POINT );
			}
			else
			{
				gUIShell.GetRenderer().SetMipFilter( D3DTEXF_LINEAR );
			}
		}}


		{	float value;
		if ( prefs->Get( "object_detail", value ) )
		{
			gWorldOptions.SetObjectDetailLevel( value );
		}}


		// Input Options
		{	bool value;
		if ( prefs->Get( "camera_inverse_x", value ) )
		{
			gUIGame.SetInvertCameraXAxis( value );
		}}

		{	bool value;
		if ( prefs->Get( "camera_inverse_y", value ) )
		{
			gUIGame.SetInvertCameraYAxis( value );
		}}

		{	bool value;
		if ( prefs->Get( "lock_camera_x_axis", value ) )
		{
			gUICamera.SetIgnoreOrbit( value );
		}}

		{	bool value;
		if ( prefs->Get( "lock_camera_y_axis", value ) )
		{
			gUICamera.SetIgnoreAzimuth( value );
		}}

		{	bool value;
		if ( prefs->Get( "screen_edge_tracking", value ) )
		{
			gUIGame.SetCameraScreenEdgeRotation( value );
		}}

		{	float value;
		if ( prefs->Get( "mouse_sensitivity", value ) )
		{
			gAppModule.SetMouseScaleX( value );
			gAppModule.SetMouseScaleY( value );
		}}

		{	float value;
		if ( prefs->Get( "orbit_degrees_per_second", value ) )
		{
			gUICamera.SetOrbitDegreesPerSecond( value );
		}}

		{	float value;
		if ( prefs->Get( "azimuth_degrees_per_second", value ) )
		{
			gUICamera.SetAzimuthDegreesPerSecond( value );
		}}

		{	float value;
		if ( prefs->Get( "zoom_meters_per_second", value ) )
		{
			gUICamera.SetZoomMetersPerSecond( value );
		}}
	}

	// Sound Options
	FastFuelHandle sound_defaults( "config:options:audio" );

	{	bool value;
	if ( (prefs.IsValid() && prefs->Get( "sound_enabled", value )) ||
		 (sound_defaults.IsValid() && sound_defaults.Get( "sound_enabled", value )) )
	{
		if( value && !gConfig.GetBool( "nosound", false ) )
		{
			gWorldSound.Enable( value, false );
			gWorldOptions.SetSound( value );
		}
	}}

	{	bool value;
	if ( (prefs.IsValid() && prefs->Get( "eax_enabled", value )) ||
		 (sound_defaults.IsValid() && sound_defaults.Get( "eax_enabled", value )) )
	{
		gWorldSound.Enable( gSoundManager.IsEnabled(), value );
	}}

	{	int value;
	if ( (prefs.IsValid() && prefs->Get( "sound_master_volume", value )) ||
		 (sound_defaults.IsValid() && sound_defaults.Get( "master_volume", value )) )
	{
		gSoundManager.SetMasterVolume( (DWORD)value );
	}}

	{	int value;
	if ( (prefs.IsValid() && prefs->Get( "sound_music_volume", value )) ||
		 (sound_defaults.IsValid() && sound_defaults.Get( "music_volume", value )) )
	{
		gSoundManager.SetStreamVolume( GPGSound::STRM_MUSIC, (DWORD)value );
	}}

	{	int value;
	if ( (prefs.IsValid() && prefs->Get( "sound_sfx_volume", value )) ||
		 (sound_defaults.IsValid() && sound_defaults.Get( "sfx_volume", value )) )
	{
		gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_LOW, (DWORD)value );
		gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_NORM, (DWORD)value );
		gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_HIGH, (DWORD)value );
	}}

	{	int value;
	if ( (prefs.IsValid() && prefs->Get( "sound_ambient_volume", value )) ||
		 (sound_defaults.IsValid() && sound_defaults.Get( "ambient_volume", value )) )
	{
		gSoundManager.SetStreamVolume( GPGSound::STRM_AMBIENT, (DWORD)value );
		gSoundManager.SetSampleVolume( GPGSound::SND_EFFECT_AMBIENT, (DWORD)value );
	}}

	{	int value;
	if ( (prefs.IsValid() && prefs->Get( "sound_voice_volume", value )) ||
		 (sound_defaults.IsValid() && sound_defaults.Get( "voice_volume", value )) )
	{
		gSoundManager.SetStreamVolume( GPGSound::STRM_VOICE, (DWORD)value );
	}}
}

void TattooGame :: SavePrefsConfig( bool autoSave )
{
	FuelHandle prefs = GetPrefsConfigFuel( true );
	if ( prefs )
	{
		// Game Options
		prefs->Set( "show_tooltips", gUIShell.GetToolTipsVisible() );

		prefs->Set( "show_framerate", gSiegeEngine.GetOptions().ShowFPSEnabled() );

		prefs->Set( "text_scroll", gUIMenuManager.GetScrollText() );

		prefs->Set( "max_text", gUIMenuManager.GetMaxText() );

		// Blood
		{
			if ( gWorldOptions.IsBloodRed() && gWorldOptions.IsBloodEnabled() )
			{
				prefs->Set( "blood", "red" );
			}
			else if ( !gWorldOptions.IsBloodRed() && gWorldOptions.IsBloodEnabled() )
			{
				prefs->Set( "blood", "green" );
			}
			else
			{
				prefs->Set( "blood", "disabled" );
			}		
		}

		// $ dismemberment disable for esrb
#if !DISABLE_GORE
		prefs->Set( "dismemberment", gWorldOptions.GetAllowDismemberment() );
#endif

		prefs->Set( "enable_sell_all_dialog", gUIStoreManager.GetEnableSellAllDialog() );

		// Video Options
		prefs->Set( "video_gamma", gUIShell.GetRenderer().GetGamma().x );

		prefs->Set( "object_detail_level", gWorldOptions.GetObjectDetailLevel() );		

		prefs->Set( "priority_boost", gAppModule.TestOptions( OPTION_BOOST_APP_PRIORITY ) );

		if ( gSiegeEngine.GetOptions().UseExpensiveShadows() )
		{
			prefs->Set( "video_shadows", "complex" );
		}
		else if( gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
		{
			prefs->Set( "video_shadows", "complex_party" );
		}
		else if ( gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() || gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
		{
			prefs->Set( "video_shadows", "simple" );
		}
		else
		{
			prefs->Set( "video_shadows", "none" );
		}

		if ( gUIShell.GetRenderer().GetMipFilter() == D3DTEXF_POINT )
		{
			prefs->Set( "texture_filtering", "bilinear" );
		}
		else
		{
			prefs->Set( "texture_filtering", "trilinear" );
		}

		// Input Options
		prefs->Set( "camera_inverse_x", gUIGame.GetInvertCameraXAxis() );

		prefs->Set( "camera_inverse_y", gUIGame.GetInvertCameraYAxis() );

		prefs->Set( "lock_camera_x_axis", gUICamera.GetIgnoreOrbit() );

		prefs->Set( "lock_camera_y_axis", gUICamera.GetIgnoreAzimuth() );

		prefs->Set( "screen_edge_tracking", gUIGame.IsCameraScreenEdgeRotation() );

		prefs->Set( "mouse_sensitivity", gAppModule.GetMouseScaleX() );

		prefs->Set( "orbit_degrees_per_second", gUICamera.GetOrbitDegreesPerSecond() );

		prefs->Set( "azimuth_degrees_per_second", gUICamera.GetAzimuthDegreesPerSecond() );

		prefs->Set( "zoom_meters_per_second", gUICamera.GetZoomMetersPerSecond() );

		// Sound Options
		prefs->Set( "sound_enabled", gSoundManager.IsEnabled() );

		prefs->Set( "eax_enabled", gSoundManager.IsEAXEnabled() );

		prefs->Set( "sound_master_volume", gSoundManager.GetMasterVolume() );

		prefs->Set( "sound_music_volume", gSoundManager.GetStreamVolume( GPGSound::STRM_MUSIC ) );

		prefs->Set( "sound_sfx_volume", gSoundManager.GetSampleVolume( GPGSound::SND_EFFECT_NORM ) );

		prefs->Set( "sound_ambient_volume", gSoundManager.GetStreamVolume( GPGSound::STRM_AMBIENT ) );

		prefs->Set( "sound_voice_volume", gSoundManager.GetStreamVolume( GPGSound::STRM_VOICE ));

		// save fuel
		if ( autoSave )
		{
			prefs->GetDB()->SaveChanges();
		}
	}
}

void TattooGame :: DeletePrefsConfig( bool autoSave )
{
	FuelHandle prefs = GetPrefsConfigFuel( true );
	if ( prefs )
	{
		TextFuelDB* fuelDb = prefs->GetDB();
		prefs->GetParent()->DestroyChildBlock( prefs );

		if ( autoSave )
		{
			fuelDb->SaveChanges();
		}
	}
}

FuelHandle TattooGame :: GetPrefsConfigFuel( bool forSaving )
{
	FuelHandle prefs;

	// build path
	FuelHandle base( "::player" );
	if ( base )
	{
		prefs = base->GetChildBlock( "prefs" );
		if ( forSaving )
		{
			if ( !prefs.IsValid() )
			{
				prefs = base->CreateChildBlock( "prefs", "prefs.gas" );
			}
		}
	}

	// done
	return ( prefs );
}

void TattooGame :: SPause( bool pause )
{
	CHECK_SERVER_ONLY;

	RSPauseAllClients( pause, PLAYERID_COMPUTER );
}

void TattooGame :: RSUserPause( bool pause )
{
	Player* player = gServer.GetScreenPlayer();
	if( player )
	{
		RSUserPause( pause, player->GetId() );
		gUIMenuManager.GuiPause( pause );
	}
	else
	{
		gpwarning( "TattooGame :: RSUserPause - called when no screen player present." );
	}
}

void TattooGame :: RSUserPause( bool pause, PlayerId requestingPlayer )
{
	FUBI_RPC_CALL( RSUserPause, RPC_TO_SERVER );

	if ( gServer.GetAllowPausing() || gServer.IsLocal() )
	{
		if( IsUserPaused() != pause )
		{
			RSPauseAllClients( pause, requestingPlayer );
		}
	}
}

void TattooGame :: SetMPSendFrequency( int Hz )
{
	if( NetPipe::DoesSingletonExist() && FuBi::NetSendTransport::DoesSingletonExist() )
	{
		gNetFuBiSend.SetSendDelay( 1.0f/float( Hz ) );
	}
}

bool TattooGame :: PrivateSaveGame( const wchar_t* saveName, bool quick, bool aut )
{
	GPSTATS_ONLY( gpstats::SetCurrentStage( STAGE_SAVE_GAME ) );

	std::auto_ptr <AutoFreezeTime> autoFreezer;
	if ( ::IsSinglePlayer() )
	{
		autoFreezer = stdx::make_auto_ptr( new AutoFreezeTime );
	}

	GameSaver::eOptions options = GameSaver::MakeSaveOptionsAndName( saveName, quick, aut );
	GameSaver saver( options );
	bool rc = saver.BeginSave( saveName );
	if ( rc )
	{
		gpwstring fullSaveName = FileSys::GetFileName( saver.GetSaveName() );

		rc = saver.FullSave();
		if ( rc )
		{
			FuelHandle prefs = GetPrefsConfigFuel( true );
			if ( prefs.IsValid() )
			{
				FuelHandle hSettings = prefs->GetChildBlock( "player", true );
				if ( hSettings.IsValid() )
				{
					hSettings->Set( "last_save_game", fullSaveName );
				}
				prefs->GetDB()->SaveChanges();
			}
		}
	}

	GPSTATS_ONLY( gpstats::SetCurrentStage( STAGE_IN_GAME ) );

	return ( rc );
}

bool TattooGame :: PrivateLoadGame( const wchar_t* loadName, bool quick, bool aut )
{
	if ( !gUIFrontend.ShowCDCheckDialog() )
	{
		return ( false );
	}

	GameLoader::eOptions options = GameLoader::MakeSaveOptionsAndName( loadName, quick, aut );
	m_LastSaveGame = GameLoader::AddSaveGameExtension( loadName, options );
	if ( GameLoader::DoesSaveGameExist( m_LastSaveGame, options ) )
	{
		gWorldStateRequest( WS_LOADING_SAVE_GAME );
		return ( true );
	}
	else
	{
		m_LastSaveGame.clear();
		return ( false );
	}
}

bool TattooGame :: OnInitCore( void )
{
	if ( !Inherited::OnInitCore() )
	{
		return ( false );
	}

	// are we supposed to do something special?
	if ( !CheckTaskAssigned() )
	{
		return ( false );
	}

	// now register the INI and prefs.gas so they are included in reports
	DrWatson::RegisterFileForReport( gConfig.GetIniFileName() );
	DrWatson::RegisterFileForReport( gConfig.ResolvePathVars( "%user_path%/prefs.gas" ) );

	// now that we've got an instance, set up resource-based infos
	SetAppIcon( IDI_APP );
	SetAppName( gpwtranslate( $MSG$ "Dungeon Siege" ) );

	// add in some useful details
#	if !GP_RETAIL
	SetAppTitleExtra( gpwstringf( L" - %S / %S", SysInfo::GetExeFileVersionText( gpversion::MODE_COMPLETE ).c_str(), COMPILE_MODE_TEXT ) );
#	endif // !GP_RETAIL

	// only write to ini file
	gConfig.SetOutRegistryIni();

	// set up dev-only bindings
#	if !GP_RETAIL
	if ( gConfig.GetBool( "force_retail_content", false ) )
	{
		InputBinder::SetAllowDevOnly( false );
	}
#	endif // !GP_RETAIL

	// build required paths
	gConfig.SetPathVar( "keys_path", "%user_path%/Keys", Config::PVM_CREATE | Config::PVM_FATAL_ON_FAIL );
	gConfig.SetPathVar( "save_path", "%user_path%/Save", Config::PVM_CREATE | Config::PVM_FATAL_ON_FAIL );
	if ( !gConfig.SetPathVar( "shots_path", "%user_path%/Screen Shots", Config::PVM_CREATE ) )
	{
		gConfig.SetPathVar( "save_path", "%user_path%" );
	}

	// set screen shots dir
	SetScreenShotDir( gConfig.ResolvePathVars( L"%shots_path%" ) );

	// check eula
	if ( !FirstRunEula() )
	{
		return ( false );
	}

	// got enough free space?
	if ( !CheckDiskSpace() )
	{
		return ( false );
	}

	// profiler
#	if !GP_RETAIL
	m_GpProfiler = new gpprofiler;
#	endif // !GP_RETAIL

	// done
	return ( true );
}

bool TattooGame :: OnInitServices( void )
{
	if ( !Inherited::OnInitServices() )
	{
		return ( false );
	}

// Low-level core components.

	// user input console
	GPDEV_ONLY( m_GpConsole = new GpConsole );

	// set the inactive update rate (to give time to the soundmgr while alt-tabbed away)
	SetInactiveUpdateRate( 5 );

	// set the sim fixed update rate
	SetSimFrameRate( gConfig.GetFloat( "simfps", 0 ) );

	// set how much we sleep between frames when in multiplayer
	SetMPSleep( gConfig.GetInt( "mpsleep", 0 ) );

	// set product id
	FileSys::SetProductId( DS1_FOURCC_ID );

// Major subsystems.

	// system services
	m_Services = new Services;
	if ( !m_Services->InitServices( Services::APP_DUNGEON_SIEGE ) )
	{
		return ( false );
	}

	// set special modes... marketing, press, etc.
#	if !GP_RETAIL
	FastFuelHandle hMarketing( "config:options:game" );
	if( hMarketing )
	{
		if( hMarketing.GetBool( "marketing", false ) )
		{
			SetOptions( GOPTION_MARKETING_DEMO );
			gGpConsole.SetAutoComplete( false );
			gGpConsole.SetShowHelp( false );
		}
	}
#	endif // !GP_RETAIL

	// set default fps visibility
	bool showFps;
	if ( gConfig.Read( "fps", showFps ) )
	{
		gSiegeEngine.GetOptions().SetShowFPS( showFps );
	}

	// movie player
	m_GameMovie = new GameMovie;

	// game world
	m_World = new World;

	// before init is called, setup default settings
	FastFuelHandle hDefaults( "config:default_settings" );

	if( hDefaults.IsValid() )
	{
		// List all children blocks
		FastFuelHandleColl cpu_blocks;
		hDefaults.ListChildren( cpu_blocks );

		// Get the speed of the host CPU
		int cpu_speed = FTOL( ceil( SysInfo::GetCPUSpeedInHertz() / 1000000.0 ) / 10.0 ) * 10;

		// Iterate to find if the current CPU speed matches given speed blocks
		for( FastFuelHandleColl::iterator d = cpu_blocks.begin(); d != cpu_blocks.end(); ++d )
		{
			// Scan the range from the block name
			int lower_range = 0, upper_range = 0;
			sscanf( (*d).GetName(), "%dmhz-%dmhz", &lower_range, &upper_range );

			// Compare with our known speed
			if( (cpu_speed >= lower_range) && (cpu_speed <= upper_range) )
			{
				// Default object detail
				gWorldOptions.SetObjectDetailLevel( (*d).GetFloat( "lodfi" ) );

				// Default shadows
				if( gSiegeEngine.GetOptions().IsExpensiveShadows() && (*d).GetBool( "no_complex_shadows" ) )
				{
					gSiegeEngine.GetOptions().ToggleExpensiveShadows();
				}

				// Default filtering
				if( gDefaultRapi.GetMipFilter() == D3DTEXF_LINEAR && !(*d).GetBool( "trilinear_filt" ) )
				{
					gDefaultRapi.SetMipFilter( D3DTEXF_POINT );
				}
			}
		}
	}

	// init world
	if ( !m_World->Init() )
	{
		return ( false );
	}

	// world state
	m_GameState = new GameState;


// Minor subsystems.

	// create the user interface
	{
		ScopeTimer timer( "init GUI" );
		m_UI = new UI;
	}

	// init user feedback message sinks
	m_GameConsoleSink = new GameConsoleSink;
	gScreenContext.AddSink( m_GameConsoleSink, false );

	// load camera settings
	if ( !LoadCameraSettings( "config:camera_settings" ) )
	{
		gperror( "Could not load camera settings!  Using defaults..." );
	}

// Preferences.

	// bind our keys
	GetGlobalBinder().PublishVoidInterface(
			"take_screenshot",
			makeFunctor( *this, &TattooGame::ScreenShotDefault ) );
	GetGlobalBinder().PublishVoidInterface(
			"copy_screenshot",
			makeFunctor( *this, &TattooGame::CopyScreenShotToClipboard ) );
	GetGlobalBinder().PublishVoidInterface(
			"take_screenshot_no_gui",
			makeFunctor( *this, &TattooGame::ScreenShotDefaultNoGui ) );
	GetGlobalBinder().PublishVoidInterface(
			"copy_screenshot_no_gui",
			makeFunctor( *this, &TattooGame::CopyScreenShotToClipboardNoGui ) );

#	if GP_ENABLE_PROFILING
	GetGlobalBinder().PublishVoidInterface(
			"profile_1",
			makeFunctor( gQuantifyApi, &QuantifyApi::Profile1 ) );
	GetGlobalBinder().PublishVoidInterface(
			"profile_10",
			makeFunctor( gQuantifyApi, &QuantifyApi::Profile10 ) );
	GetGlobalBinder().PublishVoidInterface(
			"profile_50",
			makeFunctor( gQuantifyApi, &QuantifyApi::Profile50 ) );
	GetGlobalBinder().PublishVoidInterface(
			"profile_100",
			makeFunctor( gQuantifyApi, &QuantifyApi::Profile100 ) );
#	endif // GP_ENABLE_PROFILING

	// bind user keys
	GetGlobalBinder().BindDefaultInputsToPublishedInterfaces();
	GetGlobalBinder().SetIsActive( true );

	// build player path driver
	gpstring playerPath = gConfig.ResolvePathVars( "%user_path%" );
	std::auto_ptr <FileSys::PathFileMgr> pathFileMgr( new FileSys::PathFileMgr );
	if ( pathFileMgr->SetRoot( playerPath ) )
	{
		// install it
		FileSys::MasterFileMgr::GetMasterFileMgr()->AddDriver( pathFileMgr.release(), true, "player" );

		// build player fuel db
		TextFuelDB* playerDb = gFuelSys.AddTextDb( "player" );
		playerDb->Init(   FuelDB::OPTION_READ
					    | FuelDB::OPTION_WRITE
					    | FuelDB::OPTION_JIT_READ
					    | FuelDB::OPTION_NO_WRITE_LIQUID
					    | FuelDB::OPTION_NO_WRITE_TYPES,
					    "player://",
						playerPath );

		if( playerDb->CriticalErrorEncountered() )
		{
			playerDb->Unload();
			gpmessageboxf(( $MSG$ "Encountered critical error while reading "
								  "player prefs file. Please repair, replace or "
								  "delete the corrupt file:\n\n%s",
								  playerPath + "prefs.gas" ));
		}
	}
	else
	{
		gperror( "Could not create player prefs!" );
	}

	// load current bindings
	LoadKeyConfig();

	// load user preferences
	gUIOptions.SetDefaults();
	LoadPrefsConfig();

// Startup settings.

	// load saved game if requested
	if ( !gWorldState.IsStateChangePending() )
	{
		gpstring requestedLoad = gConfig.GetString( "load_game" );
		if ( !requestedLoad.empty() )
		{
			if ( requestedLoad.same_no_case( "[newest]" ) )
			{
				LoadNewestGame();
			}
			else if ( requestedLoad.same_no_case( "[recent]" ) )
			{
				LoadRecentGame();
			}
			else if (   requestedLoad.same_no_case( "[quick]" )
					 || requestedLoad.same_no_case( "quick-save" )
					 || requestedLoad.same_no_case( "[quick-save]" ) )
			{
				QuickLoadGame();
			}
			else if (   requestedLoad.same_no_case( "[auto]" )
					 || requestedLoad.same_no_case( "auto-save" )
					 || requestedLoad.same_no_case( "[auto-save]" ) )
			{
				AutoLoadGame();
			}
			else
			{
				LoadGame( ::ToUnicode( requestedLoad ) );
			}
		}
	}

	// load saved game partially if requested
	if ( !gWorldState.IsStateChangePending() )
	{
		gpstring requestedLoad = gConfig.GetString( "load_game_partial" );
		if ( !requestedLoad.empty() )
		{
			LoadGamePartial( ::ToUnicode( requestedLoad ) );
		}
	}

	// load default map if requested
	if ( !gWorldState.IsStateChangePending() )
	{
		gpstring requestedMap = gConfig.GetString( "map" );
		if ( !requestedMap.empty() )
		{
			// hero specified for import?
			gpstring import = gConfig.GetString( "import" );
			if ( !import.empty() )
			{
				ImportCharacterFromSave( ::ToUnicode( import ), 0 );
			}

			// load map, be sure to check for successful load
			gWorldMap.SSet( requestedMap, gConfig.GetString( "world" ), RPC_TO_ALL );
			if ( gWorldMap.IsInitialized() )
			{
				if ( gTattooGame.CheckCdInDrive() )
				{
					gWorldStateRequest( WS_LOAD_MAP );
				}
				else
				{
					gperrorbox( "Command-line 'map' option given, but game disc is not in CD drive." );

					// give them one more shot
					if ( gTattooGame.CheckCdInDrive() )
					{
						gWorldStateRequest( WS_LOAD_MAP );
					}
				}
			}
		}
	}

	// are we going straight to zonematch?
	if ( !gWorldState.IsStateChangePending() )
	{
		if ( gConfig.GetBool( "zonematch" ) )
		{
			gWorldStateRequest( WS_MP_MATCH_MAKER );
		}
	}

	// if nothing to do, go straight into intro
	if ( !gWorldState.IsStateChangePending() && !::IsInGame( gWorldState.GetCurrentState() ) )
	{
		gWorldStateRequest( WS_INTRO );
	}

	// setup skrit bots
#	if GP_ENABLE_SKRITBOT
	gpstring skritbots = gConfig.GetString( "skritbot" );
	if ( !skritbots.empty() )
	{
		stringtool::Extractor extractor( ",", skritbots );
		gpstring skrit;
		while ( extractor.GetNextString( skrit ) )
		{
			gSkritBotMgr.AddSkritBot( skrit );
		}
	}
#	endif // GP_ENABLE_SKRITBOT

	// run autoexec.skrit
	{
		Skrit::GetReq getReq( "autoexec" );
		getReq.m_IgnoreFileErrors = true;
		Skrit::HAutoObject startup( getReq );
		if ( startup && startup->HasFunctions() )
		{
			ScopeTimer timer( "calling autoexec.skrit" );
			startup->Call( 0 );
		}
	}

	// get rid of the console
	gServices.GetOutputConsole().SetVisibility( false );

	// register render callback with siege
	gSiegeEngine.RegisterPostOpaqueRenderCb( makeFunctor( *this, &TattooGame::PostOpaqueRender ) );

// Finish.

	// $$$ make this an auto-registration thing
#	if !GP_RETAIL
#	if GP_ENABLE_STATS
	m_GpcGpStats  = new gpc_gpstats;
#	endif // GP_ENABLE_STATS
	m_GpcMman     = new gpc_mman;
	m_GpcFile     = new gpc_file;
	m_GpcLog      = new gpc_log;
	m_GpcReload   = new gpc_reload;
	m_GpcStreamer = new gpc_streamer;
	m_GpcFps      = new gpc_fps;
#	endif // !GP_RETAIL

	// done
	return ( true );
}

bool TattooGame :: OnShutdown( void )
{
	Inherited::OnShutdown();

	// $ general rule: shut down in reverse order of construction
	//                 (be sure to note exceptions here)

	// exception : shut down network so we don't try to RPC to anyone while we're destroying other objects...
	if( NetPipe::DoesSingletonExist() )
	{
		gServer.ShutdownTransport( true );	// full shutdown
	}

	// shut down the loader so we don't have new requests being filled while
	// we shut down (and let the world shutdown deal with the order cancelling)
	if ( siege::SiegeEngine::DoesSingletonExist() )
	{
		gSiegeEngine.StopLoadingThread( siege::STOP_LOADER_THREAD_ONLY );
	}

	// save prefs
	if ( !ReportSys::IsFatalOccurring() )
	{
		SavePrefsConfig();
	}

	// stats gathering
#	if GP_ENABLE_STATS
#	if !GP_RETAIL
	Delete( m_GpcGpStats		);
#	endif // !GP_RETAIL
	if ( RapiOwner::DoesSingletonExist() && (gDefaultRapiPtr != NULL) )
	{
		if ( m_CpuStatsGraphTex )
		{
			gDefaultRapi.DestroyTexture( m_CpuStatsGraphTex );
		}
		if ( m_CpuStatsPHistoryTex )
		{
			gDefaultRapi.DestroyTexture( m_CpuStatsPHistoryTex );
		}
		if ( m_CpuStatsUHistoryTex )
		{
			gDefaultRapi.DestroyTexture( m_CpuStatsUHistoryTex );
		}
	}
#	endif // GP_ENABLE_STATS

#	if !GP_RETAIL
	// dump perfmon stats
	if ( !m_PendingOrderBuffer.empty() )
	{
		// build context if none
		ReportSys::LogFileSink <> * sinkPerfmon = new ReportSys::LogFileSink <> ( "perfmon.log" );
		sinkPerfmon->DisablePrefix();
		ReportSys::LocalContext ctx( sinkPerfmon, true, "Perfmon Log", "Log" );
		ReportSys::AutoReport autoReport( &ctx );
	
		// print out the header.
		ctx.OutputF( "FRAME_DELTA, LOADER_GO_TYPE, LOADER_GO_PRELOAD_TYPE, LOADER_LODFI_TYPE,siege::LOADER_SIEGE_NODE_TYPE, siege::LOADER_TEXTURE_TYPE \n" );
	
		StringBuffer::iterator iPending, iPendingBegin = m_PendingOrderBuffer.begin(), iPendingEnd = m_PendingOrderBuffer.end();
	
		// output deltas and pending orders
		for ( iPending = iPendingBegin; iPending!=iPendingEnd; ++iPending )
		{
			ctx.OutputF( "%s\n", (*iPending).c_str() );
		}
		ctx.OutputEol();
	}
#	endif

	// other
#	if !GP_RETAIL
	Delete( m_GpcMman			);
	Delete( m_GpcFile			);
	Delete( m_GpcLog			);
	Delete( m_GpcReload			);
	Delete( m_GpcStreamer		);
	Delete( m_GpcFps			);
#	endif // !GP_RETAIL

	// services

	// delete in order of high-level to low-level
	Delete( m_UI				);
	Delete( m_World				);
	Delete( m_GameConsoleSink	);
	Delete( m_GameState			);
	Delete( m_GameMovie			);
	Delete( m_Services			);
#	if !GP_RETAIL
	Delete( m_GpConsole			);
#	endif // !GP_RETAIL

	// core
#	if !GP_RETAIL
	Delete( m_GpProfiler		);
#	endif // !GP_RETAIL

	// done
	return ( true );
}

void TattooGame :: OnAppActivate( bool activate )
{
	Inherited::OnAppActivate( activate );

	if ( !AssertData::IsAssertOccurring() )
	{
		CheckAutoSaveParty( true );

		CheckSoundFading();

		if ( World::DoesSingletonExist() )
		{
			gWorld.OnAppActivate( activate );
		}
		if ( UI::DoesSingletonExist() )
		{
			gUI.OnAppActivate( activate );
		}

		m_CopyFullMovieFrame	= true;
	}
}

void TattooGame :: OnUserPause( bool pause )
{
	Inherited::OnUserPause( pause );

	CheckAutoSaveParty( true );

	CheckSoundFading();

	if( m_InPauseRequestScope )
	{
		return;
	}

	if( ShouldAutoPauseGame() )
	{
		if( pause )
		{
			RSPauseAllClients( pause );
			gNetFuBiSend.ForceUpdate();
		}
		else if( !pause && gServer.IsLocal() )
		{
			RSPauseAllClients( pause );
			gNetFuBiSend.ForceUpdate();
		}
	}
}

bool TattooGame :: OnRequestAppClose( void )
{
	CheckAutoSaveParty( true );

	return ( gUI.RequestQuitGame() );
}

bool TattooGame :: OnScreenChange( void )
{
	// remember old streaming state then disable it - no streaming while rebuild rapi!
	bool oldStreaming = false;
	if ( siege::SiegeEngine::DoesSingletonExist() )
	{
		oldStreaming = gSiegeEngine.IsLoadingThreadRunning();
		gSiegeEngine.StopLoadingThread();
	}

	// change the res
	bool rc = Inherited::OnScreenChange();

	// restore
	if ( oldStreaming )
	{
		gSiegeEngine.StartLoadingThread();
	}

	// tell ui about it
	if ( UIShell::DoesSingletonExist() )
	{
		gUIShell.Resize( GetGameWidth(), GetGameHeight() );
	}
	if ( siege::SiegeEngine::DoesSingletonExist() )
	{
		gSiegeEngine.GetCompass().SetCompassRadius( gSiegeEngine.GetCompass().GetCompassRadius() );
	}
	if ( UIMenuManager::DoesSingletonExist() )
	{
		gUIMenuManager.HandleMenuResize();
	}
	if ( UIPartyManager::DoesSingletonExist() )
	{
		gUIPartyManager.OnScreenSize();
	}

	return ( rc );
}

bool TattooGame :: OnCheckFocus( void )
{
	// $$$ SUCCEEDED(lpDD4->TestCooperativeLevel())
	return ( true );
}

bool TattooGame :: OnPreTranslateMessage( MSG& msg )
{
	if ( !IsFullScreen() && atlx::Dialog::ProcessModelessDialogMsg( msg ) )
	{
		return ( true );
	}

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && ImeUi_IgnoreHotKey( &msg ) )
	{
	   return ( true );
	}

	return ( Inherited::OnPreTranslateMessage( msg ) );
}

bool TattooGame :: OnMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& returnValue )
{
	bool handled = false;

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		returnValue = ImeUi_ProcessMessage( hwnd, msg, wparam, lparam, &handled );
	}

	return ( handled || Inherited::OnMessage( hwnd, msg, wparam, lparam, returnValue ) );
}

#if !GP_RETAIL

namespace Test
{
	static int s_TestAssert = -1;
	static int s_TestCrash  = -1;
	static int s_TestFatal  = -1;
	static int s_TestWatson = -1;

	FUBI_EXPORT void MainAssert( void )
	{
		s_TestAssert = 20;
	}

	FUBI_EXPORT void MainCrash( void )
	{
		s_TestCrash = 20;
	}

	FUBI_EXPORT void MainFatal( void )
	{
		s_TestFatal = 20;
	}

	FUBI_EXPORT void MainWatson( void )
	{
		s_TestWatson = 20;
	}
}

#endif // !GP_RETAIL

bool TattooGame :: OnInactiveUpdate( void )
{
	if ( !Inherited::OnInactiveUpdate() )
	{
		return ( false );
	}

	// Call the inactive update for the sound manager
	gSoundManager.InactiveUpdate();

	if( gWorldState.GetCurrentState() == WS_MP_MATCH_MAKER )
	{
		gUIZoneMatch.InactiveUpdate();
	}

	return true;
}

bool TattooGame :: OnFrame( float deltaTime, float actualDeltaTime )
{
	if ( !Inherited::OnFrame( deltaTime, actualDeltaTime ) )
	{
		return ( false );
	}

	return ( UpdateFrame( deltaTime, actualDeltaTime ) );
}

bool TattooGame :: OnSysFrame( float deltaTime, float actualDeltaTime )
{
	if ( !Inherited::OnSysFrame( deltaTime, actualDeltaTime ) )
	{
		return ( false );
	}

	// $ update non-paused systems

#	if !GP_RETAIL
	if ( gWatchConsole.IsVisible() )
	{
		gWatchConsole.Clear();
	}
	if ( gFooterConsole.IsVisible() )
	{
		gFooterConsole.Clear();
	}
#	endif // !GP_RETAIL

	gServer.UpdateNetworkIO();

	if ( gWorldState.IsStateChangePending() && ::IsInGame( gWorldState.GetCurrentState() ) )
	{
		CheckAutoSaveParty( true );
	}

	gWorldState.Update();

	gUI.Update( deltaTime );

#	if GP_ENABLE_SKRITBOT
	gSkritBotMgr.SysUpdate( deltaTime );
#	endif // GP_ENABLE_SKRITBOT

	// Make sure camera orientation is up to date
	gSiegeEngine.GetCamera().BuildOrientations();

	if ( Server::DoesSingletonExist() )
	{
		// Get the desired 3D listening position for sound
		vector_3 listenerPos( DoNotInitialize );

		Go* party = gServer.GetScreenParty();
		if ( party != NULL && gWorldState.GetCurrentState() != WS_SP_NIS )
		{
			// Use the screen party's position as our target
			listenerPos		= party->GetPlacement()->GetWorldPosition();
			listenerPos.y	+= gUICamera.GetTrackHeightOffset();
		}
		else
		{
			listenerPos		= gSiegeEngine.GetCamera().GetCameraPosition();
		}

		gSoundManager.Update( actualDeltaTime,
							  listenerPos,
							  gSiegeEngine.GetCamera().GetMatrixOrientation().GetColumn2_T(),
							  gSiegeEngine.GetCamera().GetMatrixOrientation().GetColumn1_T() );

		// Update the world sound manager to keep positions up to date
		gWorldSound.Update( IsUserPaused() || !IsAppActive() );
	}

	if ( GoDb::DoesSingletonExist() )
	{
		gGoDb.SysUpdate( deltaTime );
	}

	gTimeMgr.GetSysTimecaster().OnUpdate( deltaTime );
	
	gAspectStorage.CommitRequests();

	return ( true );
}

bool TattooGame :: OnBeginFrame( void )
{
	if ( !Inherited::OnBeginFrame() )
	{
		return ( false );
	}

#	if !GP_RETAIL
	if ( TestOptions( GOPTION_TRACE_PROFILER ) )
	{
		m_GpProfiler->Begin();
	}
#	endif // !GP_RETAIL

	return ( true );
}

bool TattooGame :: OnEndFrame( void )
{
	if ( !Inherited::OnEndFrame() )
	{
		return ( false );
	}

	if ( TestOptions( GOPTION_TRACE_PROFILER ) )
	{
		GPSTATS_GROUP( GROUP_IDLE );

#		if !GP_RETAIL
		m_LastProfileOutput.clear();

		ReportSys::LocalContext localCont;
		ReportSys::StringSink tempSink( m_LastProfileOutput );
		localCont.AddSink( &tempSink, false );

		m_GpProfiler->End( &localCont );
#		endif // !GP_RETAIL
	}

	return ( true );
}


#if !GP_RETAIL
	#define	 ADD_PENDING_ORDER_STAT( type )	\
		i = stats.find( type );\
		if( i != iEnd )\
		{\
			str.appendf( ", %d", i->second.m_PendingOrderCount );\
		}\
		else\
		{\
			str.appendf( ", 0");\
		}
#endif

bool TattooGame :: UpdateFrame( float deltaTime, float actualDeltaTime, bool copyToPrimary, bool doUpdate )
{
	// $ this function is meant to update pausable systems

// Update game logic.

	bool gameIsVisible = ::IsInGame( gWorldState.GetCurrentState() ) && ((gWorldState.GetPendingState() == WS_INVALID ) || ::IsInGame(gWorldState.GetPendingState()) );
//	bool gameIsStarting = ::IsGameStarting( gWorldState.GetCurrentState() );

	if ( doUpdate )
	{
		GPPROFILERSAMPLE( "main - (update and draw ) untimed", SP_MISC );

#		if GP_ENABLE_SKRITBOT
		gSkritBotMgr.Update( deltaTime );
#		endif // GP_ENABLE_SKRITBOT

		// Update movie if necessary
		if( gWorldState.GetCurrentState() == WS_INTRO || gWorldState.GetCurrentState() == WS_SP_OUTRO ||
			gWorldState.GetCurrentState() == WS_CREDITS )
		{
			if( gDefaultRapiPtr )
			{
				gRapiMouse.SetCursorImage( 0, 0, 0 );

				bool bFlipAllowed	= gDefaultRapi.IsFlipAllowed();
				gDefaultRapi.SetFlipAllowed( false );
				if( gDefaultRapi.Begin3D( false ) )
				{
					if( gGameMovie.Update( gDefaultRapiPtr, m_CopyFullMovieFrame ) )
					{
						if ( gWorldState.GetCurrentState() == WS_INTRO )
						{
							gWorldStateRequest( WS_LOGO );
						}
						else if ( gWorldState.GetCurrentState() == WS_CREDITS )
						{
							gWorldStateRequest( WS_MAIN_MENU );
						}
						else if ( gWorldState.GetCurrentState() == WS_SP_OUTRO )
						{
							if ( gUIOutro.IsMonologuePlaying() )
							{
								gUIOutro.HideMonologue();
							}
							else if ( gUIOutro.AreCreditsRolling() )
							{
								gUIOutro.CreditsFinish();
							}
						}
						else if ( gWorldState.GetCurrentState() == WS_LOADED_INTRO )
						{
							gUIIntro.HideMonologue();
						}
					}
					else
					{
						// Indicate continuing render skip while movie is playing
						++m_RenderSkipCount;
					}

					gDefaultRapi.End3D( true, true );
				}
				gDefaultRapi.SetFlipAllowed( bFlipAllowed );
			}
		}

		{
			GPPROFILERSAMPLE( "main - update game logic", SP_MISC );

			if ( gameIsVisible )
			{
				// update other systems
				if ( !IsPaused() || IsSingleStep() )
				{
					bool infiniteSim = !::IsPositive( m_SimFrameRate );

					// calculate period of sim frame
					float period = infiniteSim ? deltaTime : (1.0f / m_SimFrameRate);

					// add in last frame's worth of time
					m_SimFramePending += deltaTime;

					// loop until we have whittled it down to less than a sim
					bool entered = false;
					while ( infiniteSim || (m_SimFramePending > period) )
					{
						entered = true;

						// do one sim for critical systems, at the fixed frame rate
						gWorldTime		.Update( period );
						gMessageDispatch.Update();
						gMCP			.Update();
						gWorld			.Update( period, actualDeltaTime );
						gSim			.Update( period );

						// subtract from available sim time
						m_SimFramePending -= period;

						// if infinite sim, we do it exactly once per frame
						if ( infiniteSim )
						{
							break;
						}
					}

					// make sure we advance at least one sim
					if ( !entered )
					{
						gWorldTime.Update( 0 );
					}

					// update non-sim systems with actual delta
					gWeather.Update( period );
					gSiegeEngine.SetDeltaFrameTime( deltaTime );
				}
				else
				{
					gWorldTime.Update( 0 );

					// godb does not receive update, but it must clear out any
					// pending requests
					if ( gGoDb.CommitAllRequests() )
					{
						// if anything entered the world, give it a little time
						// so it can go to its first anim frame
						gGoDb.Update( 0, actualDeltaTime );
					}
					else
					{
						// godb had no requests to update - spin worldfx directly
						gWorldFx.Update( 0, actualDeltaTime );
					}

					gSiegeEngine.SetDeltaFrameTime( 0 );

					gSim.Update( 0.0f );
					gWeather.Update( 0.0f );
				}

				// Tell the engine about the actual time delta
				gSiegeEngine.SetAbsoluteDeltaFrameTime( actualDeltaTime );

				// Update the camera
				gCameraAgent.Update( gWorldTime.GetSecondsElapsed() );

				// Update the mood system
				gMood.Update( gWorldTime.GetSecondsElapsed() );

/*				// Set timed light to noon shading
				if ( gWorldState.GetCurrentState() == WS_MEGA_MAP )
				{
					gSiegeEngine.LightDatabase().SetTimedLightColor( gTimeOfDay.GetTimeLightColor( 12, 0 ) );
				}
*/
				// Update the time of day lighting
				gTimeOfDay.Update( gWorldTime.GetSecondsElapsed() );

				// Maybe autosave
				CheckAutoSaveParty();

				// update Fuel - mainly for cache control
				if( ( gWorldState.GetCurrentState() != WS_SP_NIS )
					&& ( !siege::LoadMgr::DoesSingletonExist() || ( siege::LoadMgr::DoesSingletonExist() && !gSiegeLoadMgr.AreOrdersPending() ) ) )
				{
					gFuelSys.Update();
				}
			}
			else
			{
				// world time always nees to get time because WorldState transitions may depend on it -bk
				gWorldTime.Update( deltaTime );
				gWorld.Update( deltaTime, actualDeltaTime );
			}
		}

		// update time manager
		{
			GPPROFILERSAMPLE( "main - update time manager", SP_MISC );
			gTimeMgr.OnUpdate( deltaTime );
		}

		// update probe output
#		if !GP_RETAIL
		if( gDebugProbe.IsProbing() )
		{
			gpstring sProbe;
			gDebugProbe.GetProbeOutput( sProbe );

			gOutputConsole.Clear();
			gOutputConsole.Print( sProbe );
		}
#		endif // !GP_RETAIL

		#if !GP_RETAIL
		// add pending order information to the perfmon!
		// we only add this info when we can log time delta info, this
		// is cleared when we are in pause or skip time mode...
		// $ when this is moved, make sure to take out the includes added
		// on 8/7/02!
		if ( TestOptionsEq( OPTION_PERFMON_LOG | OPTION_ENABLE_LOG_FPS ) )
		{
			gpstring str;
			if ( gSiegeLoadMgr.IsLoadingThreadRunning() )
			{
				siege::LoadMgr::TypeStatDb stats;
				if ( gSiegeLoadMgr.GetTypeStats( stats ) )
				{
					gpstring type;
									
					str.assignf( "%f", actualDeltaTime );
					siege::LoadMgr::TypeStatDb::const_iterator i, iEnd = stats.end();

					ADD_PENDING_ORDER_STAT( LOADER_GO_TYPE );
					ADD_PENDING_ORDER_STAT( LOADER_GO_PRELOAD_TYPE );
					ADD_PENDING_ORDER_STAT( LOADER_LODFI_TYPE );
					ADD_PENDING_ORDER_STAT( siege::LOADER_SIEGE_NODE_TYPE );
					ADD_PENDING_ORDER_STAT( siege::LOADER_TEXTURE_TYPE );

					AddPendingOrderItem( str );
				}
				else // no loading stats
				{
					str.assignf( "%f\t\t", actualDeltaTime );
					AddPendingOrderItem( str );
				}			
			}
			else // we aren't loading at all
			{
				str.assignf( "%f\t\t", actualDeltaTime );
				AddPendingOrderItem( str );
			}		
		}
		#endif

	}
	else
	{
		// should always tick the frame counter or so many systems will complain...
		gWorldTime.Update( 0 );
	}

// Draw.

	// initial test
	bool should3dRender = (m_RenderSkipCount == 0) && IsAppActive() && (!gGameState.IsProgressBarUp() || AutoHideGui::DoesSingletonExist());
	if ( should3dRender )
	{
		// begin render
		if ( !gSiegeEngine.BeginRender() )
		{
			// failed for whatever reason, just skip it for now
			should3dRender = false;
		}

		// if the world viewport is not the same as the default, set it
		if ( gWorld.ViewportDiffersFromDefault() )
		{
			gWorld.SetViewportWorld();
		}
	}

	// Update the camera
	gSiegeEngine.GetCamera().UpdateCamera();

	// Store the history
	gSiegeEngine.GetCamera().StoreHistory();

	// ok now see if we should draw for real this time
	if ( should3dRender )
	{
		// store off the camera orientation for the labels. this is necessary
		// to prevent single frame snapping of labels when the target node
		// changes.
		matrix_3x3 camOrient = gSiegeEngine.GetCamera().GetMatrixOrientation();

		// update camera for interested subsystems
		gWorldFx.SetCameraOrientation( camOrient );
		gWeather.SetCameraOrientation( camOrient );

		// special - leave this between begin/end scene code
#		if ( !GP_RETAIL )
		{
			// $ special test code to make sure that aux thread crashes etc. work

			if ( (Test::s_TestAssert >= 0) && (Test::s_TestAssert-- == 0) )
			{
				gpassertm( 0, "Asserting inside main thread" );
			}

			if ( (Test::s_TestCrash >= 0) && (Test::s_TestCrash-- == 0) )
			{
				volatile int x = 0, y = 0;
				volatile int z = x / y;
				UNREFERENCED_PARAMETER( z );
			}

			if ( (Test::s_TestFatal >= 0) && (Test::s_TestFatal-- == 0) )
			{
				gpfatal( "Fatal forced inside main thread" );
			}

			if ( (Test::s_TestWatson >= 0) && (Test::s_TestWatson-- == 0) )
			{
				gpwatson( "Watson forced inside main thread" );
			}
		}
#		endif // !GP_RETAIL

		bool bFog	= gDefaultRapi.GetFogActivatedState();

		if( gWorldState.GetCurrentState() == WS_MEGA_MAP )
		{
			bFog = gDefaultRapi.SetFogActivatedState( false );

			// Update the mouse shadow
			gSiegeEngine.GetMouseShadow().Update( (float)-gAppModule.GetNormalizedCursorX(),
												  (float) gAppModule.GetNormalizedCursorY() );

			// Draw the minimap nodes
			GPDEV_ONLY( GoPlacement::SetIsRendering( true ) );
			gSiegeEngine.NodeWalker().RenderVisibleMiniNodes( 0, gAppModule.GetGameWidth(), 0, gAppModule.GetGameHeight() );
			GPDEV_ONLY( GoPlacement::SetIsRendering( false ) );

			// Tell the GoDb to update its mirror of this
			gGoDb.UpdateMouseShadow();

			// now draw the world
			if( gameIsVisible )
			{
				gSiegeEngine.GetWorldSpaceLines().Reset();
				gSiegeEngine.GetScreenSpaceLines().Render();
				gSiegeEngine.GetNodeSpaceLines().Reset();
				gSiegeEngine.GetLabels().Reset();
			}

			// reset the viewport to default settings
			if ( gWorld.ViewportDiffersFromDefault() )
			{
				gWorld.SetViewportDefault();
			}

			// render GUI - this must happen after terrain rendering because GUI
			// elements may have alpha blending
			gUI.Draw( deltaTime );

			// update time-based screen fx
			gDrawTimeMgr.OnUpdate( deltaTime );

			// Update the compass last
			gSiegeEngine.GetCompass().Update( camOrient );
		}
		else
		{
			// now draw the world
			if ( gameIsVisible )
			{
				{
					// $$ fix - if we support 16x9 (or whatever) mode in the
					//    game full time, then this code will need to be updated
					//    so that the normalized coordinates going into the
					//    siege mouse shadow are normalized to the viewport,
					//    not to the full window size. -sb

					//GPPROFILERSAMPLE( "main - update mouse shadow" );
					gSiegeEngine.GetMouseShadow().Update( (float)-gAppModule.GetNormalizedCursorX(),
														  (float) gAppModule.GetNormalizedCursorY() );
				}

				{
					//GPPROFILERSAMPLE( "main - render visible nodes" );
					GPDEV_ONLY( GoPlacement::SetIsRendering( true ) );
					gSiegeEngine.NodeWalker().RenderVisibleNodes();
					GPDEV_ONLY( GoPlacement::SetIsRendering( false ) );
				}

				{
					// Tell the GoDb to update its mirror of this
					// RenderVisibleNodes pushes hits during RenderVisibleNodes(), so let's update
					// the godb with the new information.
					gGoDb.UpdateMouseShadow();
				}

				{
					//GPPROFILERSAMPLE( "main - render world" );
					GPSTATS_GROUP( GROUP_RENDER_MISC );
					gWorld.Draw( deltaTime );
				}

				{
					//GPPROFILERSAMPLE( "main - render world space lines" );
					GPSTATS_GROUP( GROUP_RENDER_MISC );
					gSiegeEngine.GetWorldSpaceLines().Render();
				}

				{
					//GPPROFILERSAMPLE( "main - render node space lines" );
					GPSTATS_GROUP( GROUP_RENDER_MISC );
					gSiegeEngine.GetNodeSpaceLines().Render();
				}

				// draw weather
				{
					gWeather.Draw();
				}

#				if !GP_RETAIL
				// draw the MCP info (no Z-BUFFER)
				if ( gWorldOptions.GetShowMCP() )
				{
					gMCP.DebugDraw();
				}
#				endif // !GP_RETAIL

				// Anti-fog zone
				{
					bFog = gDefaultRapi.SetFogActivatedState( false );

					// draw the NIS bands (if any)
					gDefaultRapi.DrawViewportBands();

					// reset the viewport to default settings
					if ( gWorld.ViewportDiffersFromDefault() )
					{
						gWorld.SetViewportDefault();
					}

					{
						//GPPROFILERSAMPLE( "main - render screen space lines" );
						GPSTATS_GROUP( GROUP_RENDER_MISC );
						gSiegeEngine.GetScreenSpaceLines().Render();
					}

					{
						//GPPROFILERSAMPLE( "main - render labels" );
						GPSTATS_GROUP( GROUP_RENDER_MISC );
						gSiegeEngine.GetLabels().Render( camOrient );
					}

					// render GUI - this must happen after terrain rendering because GUI
					// elements may have alpha blending
					{
						gUI.Draw( deltaTime );
					}

					// update time-based screen fx
					{
						GPPROFILERSAMPLE( "main - update draw time manager", SP_MISC );
						gDrawTimeMgr.OnUpdate( deltaTime );
					}

					// Update the compass last
					gSiegeEngine.GetCompass().Update( camOrient );
				}
			}
			else
			{
				bFog = gDefaultRapi.SetFogActivatedState( false );

				if ( gWorldState.GetCurrentState() != WS_WAIT_FOR_BEGIN )
				{
					gUI.Draw( deltaTime );
				}
			}

			// Note:  Fog is disabled at this point in the loop.
		}

		// let the skritbots draw
#		if GP_ENABLE_SKRITBOT
		gSkritBotMgr.Draw();
#		endif // GP_ENABLE_SKRITBOT

		if ( !TestOptions( GOPTION_CLEAN_RENDER ) )
		{
			// draw all the debug consoles
			DrawDebugConsoles();

			// Draw cursors.  Actually doesn't really draw, just finds currently
			// active cursor.
			gUIShell.DrawCursor();

			// Set cursor image and position
			if ( gUIShell.GetActiveCursor() && gUIShell.GetActiveCursor()->GetVisible() &&
				 gWorldState.GetCurrentState() != WS_SP_NIS &&
				 gWorldState.GetCurrentState() != WS_SP_OUTRO &&
				 gWorldState.GetCurrentState() != WS_CREDITS &&
				 gWorldState.GetCurrentState() != WS_LOADED_INTRO )
			{
				gRapiMouse.SetCursorImage( gUIShell.GetActiveCursor()->GetTextureIndex(),
										   (int)gUIShell.GetActiveCursor()->GetHotspotX(),
										   (int)gUIShell.GetActiveCursor()->GetHotspotY() );
			}
			else
			{
				gRapiMouse.SetCursorImage( 0, 0, 0 );
			}

		}

		// end render - no more drawing after this point
		gSiegeEngine.EndRender( copyToPrimary, !copyToPrimary );

		// Set fog back to its original state
		gDefaultRapi.SetFogActivatedState( bFog );
	}
	else if ( m_RenderSkipCount != 0 )
	{
		--m_RenderSkipCount;
		gpassert( m_RenderSkipCount >= 0 );
	}

	if( gameIsVisible )
	{
		// Update the Siege frustums
		bool terrainFullyLoaded = gSiegeEngine.UpdateFrustums( deltaTime ) && !gSiegeLoadMgr.AreOrdersPending();
		if( gServer.IsLocal() && terrainFullyLoaded && ( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP ) )
		{
			// send directly to gamestate so we don't flood existing objects...
			WorldMessage msg( WE_MP_TERRAIN_FULLY_LOADED, GOID_INVALID, GOID_ANY );
			gGameState.HandleBroadcast( msg );
		}

		// Check for order overflow
		if( gSiegeLoadMgr.IsOverflowing() )
		{
			float current_time	= (float)::GetSystemSeconds();
			while( gSiegeLoadMgr.IsOverflowing() && ((float)::GetSystemSeconds() - current_time) < 0.015f )
			{
				Sleep( 1 );
			}
		}

		if( gServer.IsMultiPlayer() )
		{
			Sleep( GetMPSleep() );
		}
	}

	// done
	return ( true );
}

void TattooGame :: PostOpaqueRender()
{
	// selection indicators (rings around go's)
	if ( !TestOptions( GOPTION_CLEAN_RENDER ) )
	{
		gUIGame.DrawSelectionIndicators();
	}
}

void TattooGame :: RSPauseAllClients( bool pause )
{
	Player* player = gServer.GetScreenPlayer();

	RSPauseAllClients( pause, player ? player->GetId() : PLAYERID_INVALID );
}

void TattooGame :: RSPauseAllClients( bool pause, PlayerId requestingPlayer )
{
	FUBI_RPC_CALL( RSPauseAllClients, RPC_TO_SERVER );

	m_InPauseRequestScope = true;

	RCUserPause( pause, gWorldTime.GetTime(), requestingPlayer );

	if ( Singleton <NetFuBiSend>::DoesSingletonExist() )
	{
		gNetFuBiSend.ForceUpdate();
	}

	m_InPauseRequestScope = false;
}

void TattooGame :: RCUserPause( bool pause, double time, PlayerId requestingPlayer )
{
	FUBI_RPC_CALL( RCUserPause, RPC_TO_ALL );

	if( IsUserPaused() != pause )
	{
		m_InPauseRequestScope = true;

		UserPause( pause );

		m_InPauseRequestScope = false;

		if ( ::IsMultiPlayer() )
		{
			if( !pause )
			{
				gWorldTime.ForceTime( time );
			}

			if ( !gServer.GetScreenPlayer() || (gServer.GetScreenPlayer() && (gServer.GetScreenPlayer()->GetId() != requestingPlayer)) )
			{
				Player* player = gServer.GetPlayer( requestingPlayer );
				if ( player != NULL && player->GetId() != PLAYERID_COMPUTER )
				{
					if ( pause )
					{
						gpscreenf(( $MSG$ "%S paused the game\n", player->GetName().c_str() ));
					}
					else
					{
						gpscreenf(( $MSG$ "%S resumed the game\n", player->GetName().c_str() ));
					}
				}
			}
		}
	}
}

void TattooGame :: DrawDebugConsoles( void )
{
//	GPPROFILERSAMPLE( "main - render consoles" );
	GPSTATS_GROUP( GROUP_IDLE );

	#if !GP_RETAIL

	if ( gOutputConsole.IsVisible() )
	{
		gOutputConsole.Render( gSiegeEngine.Renderer() );
	}

	if ( gWatchConsole.IsVisible() )
	{
		gWatchConsole.Render( gSiegeEngine.Renderer() );
	}

	if ( gFooterConsole.IsVisible() )
	{
		UpdateFooter();
		gFooterConsole.Render( gSiegeEngine.Renderer() );
	}
	else
	{
		if ( gSiegeEngine.GetOptions().ShowFPSEnabled() )
		{
			gSiegeEngine.PrintFrameRate( GetFilteredFrameRate(), GetTimeMultiplier(), gServices.GetConsoleFont() );

			gpstring str;
			str.assignf( "WTime: %.3f RTime: %.3f TimeOfDay: %dh%dm ", gWorldTime.GetTime(), gWorldTime.GetRealTime(), gTimeOfDay.GetHour(), gTimeOfDay.GetMinute() );

			if ( gSiegeLoadMgr.IsLoadingThreadRunning() )
			{
				str.append( "Pending orders: " );

				siege::LoadMgr::TypeStatDb stats;
				if ( gSiegeLoadMgr.GetTypeStats( stats ) )
				{
					gpstring type;

					siege::LoadMgr::TypeStatDb::const_iterator i, ibegin = stats.begin(), iend = stats.end();
					for ( i = ibegin ; i != iend ; ++i )
					{
						if ( i->first == siege::LOADER_SIEGE_NODE_TYPE )
						{
							type = "Node";
						}
						else if ( i->first == LOADER_GO_TYPE )
						{
							type = "Go";
						}
						else
						{
							type = stringtool::MakeFourCcString( i->first );
						}

						if ( i != ibegin )
						{
							str += ", ";
						}
						str.appendf( "%s = %d", type.c_str(), i->second.m_PendingOrderCount );
					}
				}
				else
				{
					str += "<none>";
				}

			}

			gServices.GetConsoleFont().Print( 0, 30, str );
		}
		if ( gSiegeEngine.GetOptions().ShowPolyStatsEnabled() )
		{
			gSiegeEngine.PrintPolyStats( gServices.GetConsoleFont() );
		}

// Casualty of the DX8 conversion, will have to revisit
//#		if GP_ENABLE_STATS
#		if 0
		else if ( TestOptions( GOPTION_TRACE_CPUSTATS ) )
		{
			struct GraphEntry
			{
				const char* m_ColorName;
				const char* m_GroupName;
				COLORREF    m_Color;
				DWORD       m_Groups[ 11 ];

				float CalcScaledRatio( const GpGroupStats& stats, float scale ) const
				{
					float sum = 0;
					for ( int i = 0 ; m_Groups[ i ] != 0 ; ++i )
					{
						sum += stats.m_Time[ m_Groups[ i ] ];
					}
					return ( sum / scale );
				}

				float CalcAvgRatio( const GpGroupStats& stats ) const
				{
					return ( CalcScaledRatio( stats, stats.m_TotalTime ) );
				}
			};

			static const GraphEntry s_GraphSpecs[] =
			{
				{  "Blue",     "Models",   RGB( 49,    0, 244 ), GROUP_RENDER_MODELS, GROUP_LIGHT_MODELS, GROUP_DEFORM_MODELS, GROUP_ANIMATE_MODELS, 0,  },
				{  "LtBlue",   "Terrain",  RGB( 191, 222, 255 ), GROUP_RENDER_TERRAIN, GROUP_LIGHT_TERRAIN, 0,  },
				{  "Teal",     "UI",       RGB(   0, 128, 128 ), GROUP_RENDER_UI, GROUP_UPDATE_UI, 0,  },
				{  "Red",      "Effects",  RGB( 255,   0,   0 ), GROUP_RENDER_FX, GROUP_UPDATE_FX, 0,  },
				{  "DkGreen",  "Physics",  RGB(  50, 169,  37 ), GROUP_PHYSICS, 0  },
				{  "LtGreen",  "GODB",     RGB( 191, 239, 186 ), GROUP_GODB, 0,  },
				{  "LtOrange", "AI",       RGB( 255, 200, 149 ), GROUP_AI_QUERY, GROUP_AI_MISC, GROUP_PATH_FIND, GROUP_PATH_FOLLOW, 0,  },
				{  "Cyan",     "Loading",  RGB(  79, 242, 255 ), GROUP_LOAD_GOS, GROUP_LOAD_TERRAIN, GROUP_LOAD_TEXTURE, 0,  },
				{  "Yellow",   "Net",      RGB( 247, 249, 155 ), GROUP_NET, 0,  },
				{  "DkBlue",   "Skrit",    RGB(   0,   0, 128 ), GROUP_SKRIT, 0,  },
				{  "DkRed",    "Fuel",     RGB( 128,   0,   0 ), GROUP_FUEL, 0,  },
				{  "Orange",   "Triggers", RGB( 255, 128,   0 ), GROUP_TRIGGERS, 0,  },
				{  "DkPurple", "3DMisc",   RGB( 128,   0, 128 ), GROUP_RENDER_MISC, 0,  },
			};
			static const COLORREF s_DefaultColor = RGB( 100, 100, 100 );

			GpGroupStats avgStats;
			float avgFrameTime = 1.0f;

			if ( TestOptions( GOPTION_TRACE_CPUSTATS_TABLE ) )
			{
				ReportSys::LocalContext localCtx( new DirectRenderSink( &gServices.GetConsoleFont(), 0, 80 ), true );
				avgFrameTime = gGpStatsMgr.FillCpuStats( &avgStats, &localCtx );

				if ( TestOptions( GOPTION_TRACE_CPUSTATS_GRAPH | GOPTION_TRACE_CPUSTATS_HISTORY ) )
				{
					localCtx.OutputEol();

					const int ENTRIES_PER_ROW = 3;
					for ( int i = 0 ; i != ELEMENT_COUNT( s_GraphSpecs ) ; ++i )
					{
						if ( (i % ENTRIES_PER_ROW) != 0 )
						{
							localCtx.OutputF( ", " );
						}
						localCtx.OutputF( "%s: %s", s_GraphSpecs[ i ].m_ColorName, s_GraphSpecs[ i ].m_GroupName );
						if ( (i % ENTRIES_PER_ROW) == (ENTRIES_PER_ROW - 1) )
						{
							localCtx.OutputEol();
						}
					}

					localCtx.OutputEol();
				}
			}
			else if ( TestOptions( GOPTION_TRACE_CPUSTATS_GRAPH | GOPTION_TRACE_CPUSTATS_HISTORY ) )
			{
				avgFrameTime = gGpStatsMgr.FillCpuStats( &avgStats );
			}

			static const GSize GRAPH_SIZE( 64, 64 ), HISTORY_SIZE( 256, 64 );
			static const float HISTORY_AUTO_ADJUST_RATIO = 0.35f;

			GRect graphRect( GPoint( 0, 0 ), GRAPH_SIZE );
			graphRect.Offset( 5, GetGameHeight() - 45 - GRAPH_SIZE.cy );
			GRect historyRect( GPoint( 0, 0 ), HISTORY_SIZE );
			historyRect.Offset( graphRect.right + 5, GetGameHeight() - 45 - HISTORY_SIZE.cy );

			if ( TestOptions( GOPTION_TRACE_CPUSTATS_GRAPH ) )
			{
				// create texture if not already
				if ( !m_CpuStatsGraphTex )
				{
					// pre-fill with solid white so we can detect when not
					// written for alpha adjust (color keying not supported)
					m_CpuStatsGraphTex = gDefaultRapi.CreateBlankTexture(
							GRAPH_SIZE.cx, GRAPH_SIZE.cy, 0x00FFFFFF, TEX_DYNAMIC, false, false, false );
				}

				// only update this every 1/5 sec
				static double s_Time = 0;
				if ( (::GetSystemSeconds() - s_Time) > 0.2 )
				{
					s_Time = ::GetSystemSeconds();

					bool success = false;
					{
						PieMaker pie( GRAPH_SIZE );
						success = pie.Init( m_CpuStatsGraphTex );
						if ( success )
						{
							for ( int i = 0 ; i != ELEMENT_COUNT( s_GraphSpecs ) ; ++i )
							{
								pie.Add( s_GraphSpecs[ i ].CalcAvgRatio( avgStats ), s_GraphSpecs[ i ].m_Color );
							}

							pie.Finish( s_DefaultColor );
						}
					}

					// now post-fill the written areas with solid alpha
					if ( success )
					{
						DDSURFACEDESC2 ddsd;
						::ZeroObject( ddsd );
						ddsd.dwSize = sizeof( ddsd );

						LPDDS7 surf = gDefaultRapi.GetTexSurface( m_CpuStatsGraphTex );
						if ( (surf != NULL) && SUCCEEDED( surf->Lock( NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_NOSYSLOCK, NULL ) ) )
						{
							// get bounds
							ePixelFormat format = ::GetPixelFormat( &ddsd.ddpfPixelFormat );
							int texelBytes = ::GetSizeBytes( format );

							// make non-white areas opaque and white areas transparent
							for ( int y = 0 ; y != (int)ddsd.dwHeight ; ++y )
							{
								BYTE* bits = (BYTE*)ddsd.lpSurface + (ddsd.lPitch * y);
								BYTE* end = bits + (texelBytes * ddsd.dwWidth);

								if ( format == PIXEL_ARGB_8888 )
								{
									for ( DWORD* i = (DWORD*)bits ; i != (DWORD*)end ; ++i )
									{
										if ( (*i & 0x00FFFFFF) == 0x00FFFFFF )
										{
											*i = 0x00FFFFFF;
										}
										else
										{
											*i |= 0xFF000000;
										}
									}
								}
								else if ( format == PIXEL_ARGB_1555 )
								{
									for ( WORD* i = (WORD*)bits ; i != (WORD*)end ; ++i )
									{
										if ( (*i & 0x7FFF) == 0x7FFF )
										{
											*i = 0x7FFF;
										}
										else
										{
											*i |= 0x8000;
										}
									}
								}
							}

							surf->Unlock( NULL );
						}
					}
				}

				// render them
				gDefaultRapi.SetTextureStageState(
						0,
						D3DTOP_MODULATE,
						D3DTOP_MODULATE,
						D3DTADDRESS_CLAMP,
						D3DTADDRESS_CLAMP,
						D3DTEXF_POINT,
						D3DTEXF_POINT,
						D3DTEXF_POINT,
						D3DBLEND_SRCALPHA,
						D3DBLEND_INVSRCALPHA,
						true );
				tVertex verts[ 4 ];
				::ZeroObject( verts );
				verts[ 0 ].rhw		= 1;
				verts[ 1 ].rhw		= 1;
				verts[ 2 ].rhw		= 1;
				verts[ 3 ].rhw		= 1;

				verts[ 0 ].x		= graphRect.left - 0.5f;
				verts[ 0 ].y		= graphRect.top - 0.5f;
				verts[ 0 ].uv.u		= 0;
				verts[ 0 ].uv.v		= 1;
				verts[ 0 ].color	= 0xD0FFFFFF;

				verts[ 1 ].x		= graphRect.left - 0.5f;
				verts[ 1 ].y		= graphRect.bottom - 0.5f;
				verts[ 1 ].uv.u		= 0;
				verts[ 1 ].uv.v		= 0;
				verts[ 1 ].color	= 0xD0FFFFFF;

				verts[ 2 ].x		= graphRect.right - 0.5f;
				verts[ 2 ].y		= graphRect.top - 0.5f;
				verts[ 2 ].uv.u		= 1;
				verts[ 2 ].uv.v		= 1;
				verts[ 2 ].color	= 0xD0FFFFFF;

				verts[ 3 ].x		= graphRect.right - 0.5f;
				verts[ 3 ].y		= graphRect.bottom - 0.5f;
				verts[ 3 ].uv.u		= 1;
				verts[ 3 ].uv.v		= 0;
				verts[ 3 ].color	= 0xD0FFFFFF;

				gDefaultRapi.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 4, TVERTEX, &m_CpuStatsGraphTex, 1 );
			}

			// fill the history
			if ( TestOptions( GOPTION_TRACE_CPUSTATS_HISTORY ) )
			{
				// create textures if not already
				if ( !m_CpuStatsPHistoryTex )
				{
					m_CpuStatsPHistoryTex = gDefaultRapi.CreateBlankTexture(
							HISTORY_SIZE.cx, HISTORY_SIZE.cy, COLOR_BLACK, TEX_DYNAMIC, false, false, false );
				}
				if ( !m_CpuStatsUHistoryTex )
				{
					m_CpuStatsUHistoryTex = gDefaultRapi.CreateBlankTexture(
							HISTORY_SIZE.cx, HISTORY_SIZE.cy, COLOR_BLACK, TEX_DYNAMIC, false, false, false );
				}

				struct TrackEntry
				{
					unsigned int m_Tex;
					int m_TexIndex;
					int m_Scale;

					TrackEntry( int tex )
					{
						m_Tex = tex;
						m_TexIndex = 0;
						m_Scale = 100;		// start at 100 msec
					}
				};

				static TrackEntry s_Unpaused( m_CpuStatsUHistoryTex ), s_Paused( m_CpuStatsPHistoryTex );
				bool paused = IsPaused() || IsUserPaused();
				TrackEntry& track = paused ? s_Paused : s_Unpaused;

				// render to texture
				{
					HistoryMaker hist( HISTORY_SIZE, track.m_TexIndex );
					if ( hist.Init( track.m_Tex ) )
					{
						const GpStatsMgr::GroupStatColl& history = paused ? gGpStatsMgr.GetPausedHistory() : gGpStatsMgr.GetUnpausedHistory();
						const GpGroupStats& stats = history[ gGpStatsMgr.GetCurrentFrameIndex() ];

						for ( int i = 0 ; i != ELEMENT_COUNT( s_GraphSpecs ) ; ++i )
						{
							hist.Add(
									s_GraphSpecs[ i ].CalcScaledRatio( stats, track.m_Scale * 0.001f ),
									s_GraphSpecs[ i ].m_Color );
						}

						hist.Finish( stats.m_TotalTime / (track.m_Scale * 0.001f), s_DefaultColor );
						hist.Finish( 0 );
					}
				}

				// auto-adjust
				float scaledTotal = avgFrameTime / (track.m_Scale * 0.001f);
				if ( fabsf( 1.0f - scaledTotal ) > HISTORY_AUTO_ADJUST_RATIO )
				{
					bool rescaled = false;

					// down or up?
					if ( scaledTotal < 1.0f )
					{
						if ( track.m_Scale > 25 )
						{
							rescaled = true;
							track.m_Scale /= 2;
						}
					}
					else
					{
						if ( track.m_Scale < 5000 )
						{
							rescaled = true;
							track.m_Scale *= 2;
						}
					}

					if ( rescaled )
					{
						{
							HistoryMaker hist( HISTORY_SIZE, track.m_TexIndex );
							if ( hist.Init( track.m_Tex ) )
							{
								hist.Finish( RGB( 255, 0, 0 ) );
							}
						}
						{
							HistoryMaker hist( HISTORY_SIZE, track.m_TexIndex );
							if ( hist.Init( track.m_Tex ) )
							{
								hist.Finish( RGB( 255, 0, 0 ) );
							}
						}
					}
				}

				// render them
				gDefaultRapi.SetTextureStageState(
						0,
						D3DTOP_MODULATE,
						D3DTOP_SELECTARG2,
						D3DTADDRESS_WRAP,
						D3DTADDRESS_WRAP,
						D3DTEXF_POINT,
						D3DTEXF_POINT,
						D3DTEXF_POINT,
						D3DBLEND_SRCALPHA,
						D3DBLEND_INVSRCALPHA,
						false );
				tVertex verts[ 4 ];
				::ZeroObject( verts );
				float partition = s_Unpaused.m_TexIndex / (float)HISTORY_SIZE.cx;
				verts[ 0 ].rhw		= 1;
				verts[ 1 ].rhw		= 1;
				verts[ 2 ].rhw		= 1;
				verts[ 3 ].rhw		= 1;

				verts[ 0 ].x		= historyRect.left - 0.5f;
				verts[ 0 ].y		= historyRect.top - 0.5f;
				verts[ 0 ].uv.u		= partition - 1.0f;
				verts[ 0 ].uv.v		= 1;
				verts[ 0 ].color	= 0x20FFFFFF;

				verts[ 1 ].x		= historyRect.left - 0.5f;
				verts[ 1 ].y		= historyRect.bottom - 0.5f;
				verts[ 1 ].uv.u		= partition - 1.0f;
				verts[ 1 ].uv.v		= 0;
				verts[ 1 ].color	= 0x20FFFFFF;

				verts[ 2 ].x		= historyRect.right - 0.5f;
				verts[ 2 ].y		= historyRect.top - 0.5f;
				verts[ 2 ].uv.u		= partition;
				verts[ 2 ].uv.v		= 1;
				verts[ 2 ].color	= 0xD0FFFFFF;

				verts[ 3 ].x		= historyRect.right - 0.5f;
				verts[ 3 ].y		= historyRect.bottom - 0.5f;
				verts[ 3 ].uv.u		= partition;
				verts[ 3 ].uv.v		= 0;
				verts[ 3 ].color	= 0xD0FFFFFF;

				gDefaultRapi.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 4, TVERTEX, &m_CpuStatsUHistoryTex, 1 );
				gServices.GetConsoleFont().Print(
						historyRect.left + 2,
						historyRect.top + 2,
						gpstringf( "%dms", s_Unpaused.m_Scale ) );

				if ( paused )
				{
					gDefaultRapi.SetTextureStageState(
							0,
							D3DTOP_MODULATE,
							D3DTOP_SELECTARG2,
							D3DTADDRESS_WRAP,
							D3DTADDRESS_WRAP,
							D3DTEXF_POINT,
							D3DTEXF_POINT,
							D3DTEXF_POINT,
							D3DBLEND_SRCALPHA,
							D3DBLEND_INVSRCALPHA,
							false );

					historyRect.OffsetY( -HISTORY_SIZE.cy - 5 );
					partition = s_Paused.m_TexIndex / (float)HISTORY_SIZE.cx;

					verts[ 0 ].uv.u		= partition - 1.0f;
					verts[ 1 ].uv.u		= partition - 1.0f;
					verts[ 2 ].uv.u		= partition;
					verts[ 3 ].uv.u		= partition;

					verts[ 0 ].y		= historyRect.top - 0.5f;
					verts[ 1 ].y		= historyRect.bottom - 0.5f;
					verts[ 2 ].y		= historyRect.top - 0.5f;
					verts[ 3 ].y		= historyRect.bottom - 0.5f;

					gDefaultRapi.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 4, TVERTEX, &m_CpuStatsPHistoryTex, 1 );
					gServices.GetConsoleFont().Print(
							historyRect.left + 2,
							historyRect.top + 2,
							gpstringf( "%dms", s_Paused.m_Scale ) );
				}
			}
		}
#		endif // GP_ENABLE_STATS
	}

	// print mem manager stats
	if ( TestOptions( GOPTION_TRACE_MEMORY ) )
	{
		// $$$ replace this with a dump context

		gOutputConsole.Clear();
		gpstring output;

#		if GPMEM_DBG_NEW
		gpmem::GetAllocationsSummary( output, 20, s_MemSortType );
		gOutputConsole.Print( output );
		output.erase();
#		endif

		gpmem::GetStatusSummary( output );
		gOutputConsole.Print( output );
	}
	// or print profiler stats for last sim pass
	else if ( TestOptions( GOPTION_TRACE_PROFILER ) )
	{
		GPPROFILERSAMPLE( "main - output profiler samples", SP_MISC );
		gOutputConsole.Clear();
		gOutputConsole.Print( m_LastProfileOutput );
	}
	// or print global stats
	else if ( TestOptions( GOPTION_TRACE_STATS ) )
	{
#		if GP_ENABLE_STATS
		m_LastProfileOutput.clear();
		ReportSys::LocalContext localCtx;
		ReportSys::StringSink tempSink( m_LastProfileOutput );
		localCtx.AddSink( &tempSink, false );
		gGpStatsMgr.Report( localCtx );
		gOutputConsole.Clear();
		gOutputConsole.Print( m_LastProfileOutput );
#		endif // GP_ENABLE_STATS
	}

	#else // !GP_RETAIL

	if ( gSiegeEngine.GetOptions().ShowFPSEnabled() )
	{
		gSiegeEngine.PrintFrameRate( GetFilteredFrameRate(), GetTimeMultiplier(), gServices.GetConsoleFont() );
	}

	#endif // !GP_RETAIL
}

bool TattooGame :: FirstRunEula( void )
{
	bool allowGameToRun = false;
	gpstring installPath = gConfig.ResolvePathVars( "%exe_path%" );

	EbuEulaDll dll( installPath );
	if ( dll.Load( false ) )
	{
		gpwstring sEula		= gpwtranslate( $MSG$ "eula.rtf" );
		gpwstring sWarranty	= gpwtranslate( $MSG$ "warranty.rtf" );
		
		if ( dll.EBUEula(
				"Software\\Microsoft\\Microsoft Games\\Dungeon Siege\\Eula",
				installPath + ToAnsi( sEula ),
				installPath + ToAnsi( sWarranty ),
				TRUE ) )
		{
			allowGameToRun = true;
		}
	}
	else
	{
#		if GP_ENABLE_DEV_FEATURES
		allowGameToRun = true;
#		else // GP_ENABLE_DEV_FEATURES
		gperrorboxf(( $MSG$ "Unable to find/load EBUEULA.DLL. %S requires this file to continue.\n\n"
							"(Error is '%s')\n",
							GetAppName().c_str(),
							stringtool::GetLastErrorText().c_str() ));
#		endif // !GP_ENABLE_DEV_FEATURES
	}

	return ( allowGameToRun );
}

bool TattooGame :: CheckDiskSpace( void )
{
	// this function makes sure we have enough disk space to run. prevents
	// stupid people from trying to run DS on c:\ with 5 megs free or with their
	// virtual memory settings all borked.

	const DWORD MIN_VIRTUAL_MEMORY  = 200;		// available virtual memory (megs)
	const DWORD MIN_SAVE_GAME_SPACE = 15;		// free space in save game dir (megs)
	const DWORD MIN_REPORT_SPACE    = 50;		// free space for reports and logs and stuff (megs)

	// always provide an early-out
	if ( gConfig.GetBool( "nospacecheck", false ) )
	{
		return ( true );
	}

// Check virtual memory.

	// best way i could think to do this was just alloc it and see if it works
	{
		DWORD size = MIN_VIRTUAL_MEMORY * 1024 * 1024;
		BYTE* mem = (BYTE*)::VirtualAlloc( NULL, size, MEM_COMMIT, PAGE_READWRITE );
		if ( mem == NULL )
		{
			gperrorboxf(( $MSG$ "Help! Not enough free space is available in "
								"the swap file for Dungeon Siege to run "
								"properly (at least %d MB is required). Please "
								"increase your swap file settings and run "
								"Dungeon Siege again.\n",
								MIN_VIRTUAL_MEMORY ));
			return ( false );
		}
		else
		{
			// let 'er go
			::VirtualFree( mem, 0, MEM_DECOMMIT );
			::VirtualFree( mem, 0, MEM_RELEASE );
		}
	}

// Check space on save game drive.

	// not having enough space is not a reason to fail running completely.
	// just warn 'em.
	{
		gpstring savePath = gConfig.ResolvePathVars( "%save_path%" );

		ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
		if ( ::GetDiskFreeSpaceEx(
				savePath,
				&freeBytesAvailable,
				&totalNumberOfBytes,
				&totalNumberOfFreeBytes ) )
		{
			if ( freeBytesAvailable.QuadPart < (MIN_SAVE_GAME_SPACE * 1024 * 1024) )
			{
				gperrorboxf(( $MSG$ "Careful! There may not be enough free space "
									"available to save the game (saved games go "
									"to '%s'). Try to free up some space so "
									"you have enough room to save your games.\n",
									savePath.c_str() ));
			}
		}
	}

// Check report output dir.

#	if !GP_RETAIL
	{
		gpstring reportPath = FileSys::GetLocalWritableDir( false );

		ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
		if ( ::GetDiskFreeSpaceEx(
				reportPath,
				&freeBytesAvailable,
				&totalNumberOfBytes,
				&totalNumberOfFreeBytes ) )
		{
			if ( freeBytesAvailable.QuadPart < (MIN_REPORT_SPACE * 1024 * 1024) )
			{
				gperrorboxf(( "Careful! Disk space is low where temporary report "
							  "files and logs are stored (in '%s'). You might "
							  "want to free up some space before you have a "
							  "meltdown!\n",
							  reportPath.c_str() ));
			}
		}
	}
#	endif // !GP_RETAIL

	// all ok
	return ( true );
}

bool TattooGame :: CheckTaskAssigned( void )
{
	stringtool::CommandLineExtractor cle;
	CommandLine cl;
	bool notask = true;

	// scan for .dslog on command line
	stringtool::CommandLineExtractor::ConstIter i, ibegin = cle.Begin(), iend = cle.End();
	for ( i = ibegin ; i != iend ; ++i )
	{
		const char* ext = FileSys::GetExtension( *i );
		if ( (ext != NULL) && ::same_no_case( ext, "dslog" ) )
		{
			// suck out the data
			NetLog::Extract( *i );

			// false means don't continue running
			notask = false;
		}
	}

	// someone want to verify?
	if ( cl.GetBool( "verifydata", false ) )
	{
		DWORD headerCrc, actualCrc;
		if ( !FileSys::GetModuleChecksums( headerCrc, actualCrc ) || (headerCrc == 0) )
		{
			gperrorbox( "Can't verify EXE checksum, it's not set! Did you run the packer?\n" );
		}
		else if ( headerCrc == actualCrc )
		{
			gpmessagebox( "EXE verification SUCCEEDED, CRC checks out a-ok! Checking resources next...\n" );
		}
		else
		{
			gperrorboxf(( "EXE verification FAILED, file may be corrupt!\n\nFound checksum 0x%08X, but calculated 0x%08X!\n",
							headerCrc, actualCrc ));
			notask = false;
		}
	}

	// just want help?
#	if !GP_RETAIL
	if ( cl.GetBool( "dumphelp", false ) )
	{
		notask = false;
		Services::DumpHelp();
	}
#	endif // !GP_RETAIL

	// validate skrit?
	gpstring skritFileName = cl.GetString( "checkskrit" );
	if ( !skritFileName.empty() )
	{
		notask = false;
		Services::CheckSkrit( skritFileName );
	}

	// done
	return ( notask );
}

void TattooGame :: CheckSoundFading( void )
{
	bool shouldFade = IsUserPaused() || !IsAppActive();
	if ( m_IsSoundFaded != shouldFade )
	{
		m_IsSoundFaded = shouldFade;

		if( m_IsSoundFaded )
		{
			gSoundManager.FadePauseAllSounds( 2.0f );
		}
		else
		{
			gSoundManager.FadeResumeAllSounds( 0.3f );
		}
	}
}

#if !GP_RETAIL

void TattooGame :: UpdateFooter( void )
{
	gpassert( gFooterConsole.IsVisible() );

	ReportSys::LocalContext localCont;
	ReportSys::StringSink tempSink;
	localCont.AddSink( &tempSink, false );

	DumpPerformanceSummary( true, &localCont );

	gFooterConsole.Print( tempSink.GetBuffer() );
}

void TattooGame :: DumpPerformanceSummary( bool includeUI, ReportSys::Context * ctx )
{
	////////////////////////////////////////
	//	time

	ctx->OutputF( "[time]         WorldTime= %5.3f, Sim= %d, FPS=%2.2f, mcplag= %2.3f\n", 
					gWorldTime.GetTime(), 
					gWorldTime.GetSimCount(), 
					gAppModule.GetFilteredFrameRate(),
					gWorldOptions.GetLagMCP()	);

	if( gWorld.IsMultiPlayer() )
	{
		gpstring timeDrift;
		timeDrift.assignf( "[time]         clock drifts: worldtime= %2.3f", gWorldTime.GetLocalRelativeServerTimeDrift() );
	}

	////////////////////////////////////////
	//	misc mouse-hit related

	GoHandle hHitGo;
	if( includeUI && gSiegeEngine.GetMouseShadow().IsHit() )
	{
		hHitGo = MakeGoid(gSiegeEngine.GetMouseShadow().GetHit());

		if ( hHitGo )
		{
			ctx->OutputF( "[misc]         server %s\n", gServer.IsLocal() ? "LOCAL" : "REMOTE" );

			ctx->OutputF( "[hit go]       " );

			if ( hHitGo->GetCommon()->GetScreenName().empty() )
			{
				ctx->Output( "*" );
			}

			ctx->OutputF( "template=%s%s screenname=%S scid=",
						  hHitGo->GetTemplateName(),
						  (!hHitGo->HasGui() || hHitGo->GetGui()->GetVariation().empty()) ? gpstring::EMPTY.c_str() : gpstringf( ":%s", hHitGo->GetGui()->GetVariation().c_str() ).c_str(),
						  hHitGo->GetCommon()->GetScreenName().c_str() );

			if( hHitGo->GetScid() == SCID_SPAWNED )
			{
				ctx->OutputF( "<spawned>\n" );
			}
			else
			{
				ctx->OutputF( "0x%08x\n", hHitGo->GetScid() );
			}

			// extra traits, teleport

			const SiegePos& hitPos = hHitGo->GetPlacement()->GetPosition();

			ctx->OutputF( "               goid=0x%08x (%s) teleport %s:n:%0.2f,%0.2f,%0.2f,%s\n",
							hHitGo->GetGoid(),
							hHitGo->IsGlobalGo() ? "global" : "local",
							gWorldMap.GetMapName().c_str(),
							hitPos.pos.x, hitPos.pos.y, hitPos.pos.z,
							hitPos.node.ToString().c_str() );

			// gui helper stuff for lee

			if ( hHitGo->HasAspect() )
			{
				nema::HAspect aspect = hHitGo->GetAspect()->GetAspectHandle();
				ctx->OutputF( "               mesh=%s bitmap=%s value=%d\n",
								aspect->GetDebugName(),
								FileSys::GetFileNameOnly( gDefaultRapi.GetTexturePathname( aspect->GetTexture( 0 ) ) ).c_str(),
								hHitGo->GetAspect()->GetGoldValue() );
			}

			if ( hHitGo->HasGui() )
			{
				nema::HAspect aspect = hHitGo->GetAspect()->GetAspectHandle();
				ctx->OutputF( "               inv=%s active=%s\n",
								hHitGo->GetGui()->GetInventoryIcon().c_str(),
								hHitGo->GetGui()->GetActiveIcon().c_str() );
			}
		}
	}

	//----- mouse shadow information

	if( includeUI )
	{
		if ( !gSiegeEngine.GetMouseShadow().IsTerrainHit() )
		{
			ctx->Output( "[cursor]       hit terrain = FALSE\n");
		}
		else
		{
			// get infoz
			const SiegePos& hitPos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
			RegionId region = gWorldMap.GetRegionByNode( hitPos.node );
			gpstring regionName = gWorldMap.GetRegionName( region );

			// output
			ctx->OutputF( "[cursor]       hit pos = (r:%s), [teleport %s:n:%0.2f,%0.2f,%0.2f,%s]\n",
							 regionName.c_str(),
							 gWorldMap.GetMapName().c_str(),
							 hitPos.pos.x, hitPos.pos.y,hitPos.pos.z,
							 hitPos.node.ToString().c_str()  );
		}

		//----- mouse screen coordinance debug

		ctx->OutputF(	"               status(%s), screenpos abs(%3d,%3d), rel(%1.3f,%1.3f)\n",
						gAppModule.MakeMouseDebugState().c_str(),
						gAppModule.GetCursorX(),
						gAppModule.GetCursorY(),
						gAppModule.GetNormalizedCursorX(),
						gAppModule.GetNormalizedCursorY() );

		// range finder

		if( gSiegeEngine.GetMouseShadow().IsTerrainHit() && ( gGoDb.GetSelection().size() == 1 ) )
		{
			Go * from = *(gGoDb.GetSelectionBegin());
			if( from )
			{
				ctx->OutputF(	"[range finder] distance = %f meters\n",
								Length( gSiegeEngine.GetDifferenceVector( from->GetPlacement()->GetPosition(), gSiegeEngine.GetMouseShadow().GetFloorHitPos() ) ) );
			}
		}
	}

	// selection

	int TotalNodes			= gSiegeEngine.GetCulledNodeCount() + gSiegeEngine.GetRenderedNodeCount();

	float NodePercentage	= TotalNodes ? (100.0f * gSiegeEngine.GetRenderedNodeCount() / TotalNodes) : 0;

	ctx->OutputF( "[selection]    size= %3d, SNOs drawn: %3d/%3d [%3.0f%%], streamer: %s, mouse: %s\n",
					 gGoDb.GetSelection().size(),
					 gSiegeEngine.GetRenderedNodeCount(),
		 			 TotalNodes,
					 NodePercentage,
					 gSiegeEngine.IsLoadingThreadRunning() ? "running" : "stopped",
					 gRapiMouse.IsMousePaused() ? "stopped" : "running" );

	// message traffic summary
	ctx->OutputF( "[wmessage]     ctor= %d, sent= %d, delayed= %d, unloaded= %d, scidmsg= %d, scidloaded= %d\n",
					WorldMessage::GetConstructionCount(),
					gMessageDispatch.GetNumObjectsMessagedSinceLastUpdate(),
					gMessageDispatch.GetDelayedMessageDbSize(),
					gMessageDispatch.GetUnloadedMessageDbSize(),
					gMessageDispatch.GetScidMessageDbSize(),
					gMessageDispatch.GetScidLoadedListSize()	);

	WorldMessage::ClearConstructionCount();

	ctx->OutputF( "[godb]         size= %d (g:%d[d:%d,s:%d],l:%d,c:%d[u:%d],x:%d), upd= %d, rndr= %d, view= %d\n",
					 gGoDb.GetTotalGoCount(),
					 gGoDb.GetGlobalGoCount(), gGoDb.GetGlobalGoDirtyCount(), gGoDb.GetGlobalGoServerOnlyCount(),
					 gGoDb.GetLocalGoCount(),
					 gGoDb.GetCloneSrcGoCount(), gGoDb.GetCloneSrcTotalGoCount() - gGoDb.GetCloneSrcGoCount(),
					 gGoDb.GetExpiringGoCount(),
					 gGoDb.GetLastUpdateCount(), gGoDb.GetLastRenderCount(), gGoDb.GetLastViewCount() );

	ctx->OutputF( "[thrdlocks]    blckd= %d,longwait= %d,rendthrdblckd= %d,rendthrdfileops= %d \n",
		kerneltool::GetBlockedCriticalCount(),
		kerneltool::GetLongWaitCriticalCount(),
		kerneltool::GetRenderThreadFileOpBlockedCount(),
		kerneltool::GetRenderThreadFileOpCount());


	ctx->OutputF( "[ai]           job clone source cache size= %d\n", gAIAction.GetNumJobCloneSources() );

	if( gWorld.IsMultiPlayer() )
	{
		static double TimeToUpdate;
		static unsigned int sd, rd;
		if( TimeToUpdate < gWorldTime.GetTime() )
		{
			double TimeThen;
			sd = gNetPipe.GetBytesSentDelta( gWorldTime.GetTime(), TimeThen );
			sd = unsigned int( float(sd) / float( gWorldTime.GetTime() - TimeThen ) );

			rd = gNetPipe.GetBytesReceivedDelta( gWorldTime.GetTime(), TimeThen );
			rd = unsigned int( float(rd) / float( gWorldTime.GetTime() - TimeThen ) );

			TimeToUpdate = gWorldTime.GetTime() + 1.0;
		}

		ctx->OutputF( "[net]          sd= %d bytes, rd= %d bytes : st= %d bytes, rt= %d bytes\n",
						sd,
						rd,
						gNetPipe.GetTotalBytesSent(),
						gNetPipe.GetTotalBytesReceived() );
	}

	gFuelSys.Dump( ctx, false );

	gpstring mem_summary;
	gpmem::GetStatusSummary( mem_summary );

	ctx->Output( mem_summary.c_str() );
}


bool TattooGame :: PrivateReloadObjects( eReloadType type )
{
	// switch to reloading state temporarily
	eWorldState oldState = gWorldState.GetCurrentState();
	gWorldStateRequest( WS_RELOADING );
	gWorldState.Update();

	// temporarily shut down streamer
	gSiegeEngine.StopLoadingThread();

	// finish any pending requests
	gGoDb.CommitAllRequests();

	// possibly get specs on our party members
	Go* screenParty = gServer.GetScreenParty();
	gpassert( screenParty != NULL );
	typedef std::vector <std::pair <gpstring, GoCloneReq> > CloneColl;
	CloneColl cloneColl;

	// get a basis for xfer
	FuelHandle base( "::import:root" );
	base = base->CreateChildBlock( "temp", "temp" );

	// add party
	if ( type == RELOAD_CONTENTDB )
	{
		FuBi::FuelWriter writer( base );
		FuBi::PersistContext persist( &writer, false );

		GoCloneReq cloneReq;
		cloneReq.m_PlayerId = screenParty->GetPlayerId();
		cloneReq.SetStartingPos( screenParty->GetPlacement()->GetPosition() );
		cloneReq.SetStartingOrient( screenParty->GetPlacement()->GetOrientation() );
		cloneReq.m_AllClients = true;
		cloneColl.push_back( std::make_pair( gpstring( screenParty->GetTemplateName() ), cloneReq ) );

		// add children
		GopColl::const_iterator i,
								ibegin = screenParty->GetChildBegin(),
								iend   = screenParty->GetChildEnd  ();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// xfer
			persist.EnterBlock( ToString( cloneColl.size() ) );
			(*i)->XferCharacter( persist );
			persist.LeaveBlock();

			// copy clone req
			cloneReq.m_PlayerId = (*i)->GetPlayerId();
			cloneReq.SetStartingPos( (*i)->GetPlacement()->GetPosition() );
			cloneReq.SetStartingOrient( (*i)->GetPlacement()->GetOrientation() );
			cloneReq.m_AllClients = true;
			cloneColl.push_back( std::make_pair( gpstring( (*i)->GetTemplateName() ), cloneReq ) );
		}
	}

	// kill it from ui
	gUIPartyManager.DeconstructUIParty();

	// delete everybody except screen party and members and their posessions
	// (unless we're reloading the contentdb in which case we MUST rebuild ALL
	// the go's, even including clone sources).
	if ( type == RELOAD_CONTENTDB )
	{
		gGoDb.Shutdown();
	}
	else
	{
		GoidColl coll;
		gGoDb.GetAllGlobalGoids( coll );
		gGoDb.GetAllLocalGoids( coll );
		if ( type == RELOAD_CONTENTDB )
		{
			gGoDb.GetAllCloneSrcGoids( coll );
		}
		GoidColl::iterator i, ibegin = coll.begin(), iend = coll.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			GoHandle go( *i );
			if ( go->GetRoot() != screenParty )
			{
				gGoDb.SMarkForDeletion( *i, false );
			}
		}

		// force completion
		gGoDb.CommitAllRequests();

		// clean out icontent
		gGoDb.ClearScidSpecifics();
	}

	// optionally rebuild contentdb
	if ( type == RELOAD_CONTENTDB )
	{
		gContentDb.Shutdown();
		FastFuelHandle contentDb( "world:contentdb" );
		if ( contentDb )
		{
			contentDb.Unload();
		}
		FastFuelHandle soundDb( "world:global:sounds" );
		if ( soundDb )
		{
			soundDb.Unload();
		}
		gContentDb.Init();
	}

	// get some fuel
	FastFuelHandleColl regionFuelList;
	gWorldMap.GetAllRegionsDirFuel( regionFuelList );

	// unload indexes and objects from gas
	FastFuelHandleColl::iterator j, jbegin = regionFuelList.begin(), jend = regionFuelList.end();
	for ( j = jbegin ; j != jend ; ++j )
	{
		// unload indexes
		FastFuelHandle index( (*j).GetChildNamed( "index" ) );
		if ( index )
		{
			index.Unload();
		}

		// unload all object gas
		FastFuelHandle objects( (*j).GetChildNamed( "objects" ) );
		if ( objects )
		{
			objects.Unload();
		}
	}

	// reset content indexes
	gWorldMap.ReloadContentIndexes();

	// rebuild our party
	if ( !cloneColl.empty() )
	{
		FuBi::FuelReader reader( base );
		FuBi::PersistContext persist( &reader, false );

		// first rebuild party
		GoCloneReq& cloneReq = cloneColl.front().second;
		cloneReq.m_CloneSource = gGoDb.FindCloneSource( cloneColl.front().first );
		Goid partyGoid = gGoDb.SCloneGo( cloneReq );

		// give frustum to the party
		GoHandle party( partyGoid );
		party->GetParty()->TakeFrustum( gServer.GetScreenPlayer()->GetInitialFrustum() );

		// next rebuild party members
		CloneColl::iterator i, ibegin = cloneColl.begin(), iend = cloneColl.end();
		for ( i = ibegin, ++i ; i != iend ; ++i )
		{
			// construct
			GoCloneReq& cloneReq = i->second;
			cloneReq.m_CloneSource = gGoDb.FindCloneSource( i->first );
			cloneReq.m_Parent = partyGoid;
			Goid goid = gGoDb.SCloneGo( cloneReq );

			// xfer in
			persist.EnterBlock( ToString( i - ibegin ) );
			GoHandle( goid )->XferCharacter( persist );
			persist.LeaveBlock();
		}
	}

	// tell ui to rebuild the party
	gUIPartyManager.ConstructUIParty();

	// force load of all go's in our active set
	{
		siege::SiegeFrustum* frustum = gSiegeEngine.GetFrustum( gSiegeEngine.GetRenderFrustum() );
		siege::FrustumNodeColl::iterator i, ibegin, iend;
		ibegin = frustum->GetFrustumNodeColl().begin(),
		iend   = frustum->GetFrustumNodeColl().end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gWorldMap.LoadNodeContent( (*i)->GetGUID() );
			gWorldMap.LoadNodeContent( (*i)->GetGUID(), true, true );
		}
		ibegin = frustum->GetLoadedNodeColl().begin(),
		iend   = frustum->GetLoadedNodeColl().end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gWorldMap.LoadNodeContent( (*i)->GetGUID() );
			gWorldMap.LoadNodeContent( (*i)->GetGUID(), true, true );
		}
	}

	// advance a frame
	gWorldTime.Update( 0 );

	// commit all pending
	gGoDb.CommitAllRequests();
	gMessageDispatch.Update();

	// restart streamer
	gSiegeEngine.StartLoadingThread();

	// done with this fuel
	base.Delete();

	// switch back to old state
	gWorldStateRequest( oldState );
	gWorldState.Update();

	// done
	return ( true );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
