//////////////////////////////////////////////////////////////////////////////
//
// File     :  WorldConfig.inc
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Config.h"
#include "Services.h"

//////////////////////////////////////////////////////////////////////////////

static gpstring GetOverridePath( const char* var, bool& pathOnly )
{
	pathOnly = false;

	gpstring path = gConfig.GetString( var );
	if ( path[ 0 ] == '!' )
	{
		path.erase( 0, 1 );
		pathOnly = true;
	}
	return ( path );
}

static void InitializeWorldConfigPaths( FOURCC appCode, int attempt = 0 )
{
// Detect DungeonSiege path.

	// detect from installer key in registry
	{
		Registry registry( "Microsoft", "Microsoft Games", false );
		gpwstring installPath = registry.GetWString( "DungeonSiege/1.0/EXE Path" );
		if ( !installPath.empty() )
		{
			gConfig.SetPathVar( "ds_install_path", installPath, Config::PVM_REQUIRED );
		}
	}

	// first attempt we try to use the exe dir. if that fails, then fall back
	// to what's in the registry.
	if ( (appCode == Services::APP_DUNGEON_SIEGE) && (attempt == 0) )
	{
		// we're DS, that's just the exe dir then!
		gConfig.SetPathVar( "ds_exe_path", "%exe_path%" );
	}
	else
	{
		gConfig.SetPathVar( "ds_exe_path", "%ds_install_path%", Config::PVM_REQUIRED );
	}

// Detect DSTK path.

	// detect from installer key in registry
	{
		Registry registry( "Gas Powered Games", "DSTK", false );
		gpwstring installPath = registry.GetWString( "1.0/Install Directory" );
		if ( !installPath.empty() )
		{
			gConfig.SetPathVar( "dstk_path", installPath, Config::PVM_REQUIRED );
		}
	}

// Setup HOME path.

	// default to exe dir
	gConfig.SetPathVar( "home", "%exe_path%" );

	// try user-set one
	if ( !gConfig.SetPathVar( "home", (const char*)NULL, Config::PVM_USE_CONFIG | Config::PVM_REQUIRED | Config::PVM_ERROR_ON_FAIL ) )
	{
		// no? ok try using the DS exe dir for our home (dev-mode uses exe dir)
		if ( !gConfig.IsDevMode() && gConfig.IsPathVarSet( "ds_exe_path" ) )
		{
			gConfig.SetPathVar( "home", "%ds_exe_path%" );
		}
	}

// Setup tank paths.

	struct Local { static void AddTankPaths( const char* var, const char* subDir, bool create )
	{
		// clear old value
		gConfig.DeletePathVar( var );

		// try outta config
		bool pathOnly = false;
		gpstring path = GetOverridePath( var, pathOnly );
		if ( !path.empty() )
		{
			if ( !gConfig.AddPathVar( var, path, Config::PVM_REQUIRED | Config::PVM_ERROR_ON_FAIL ) )
			{
				pathOnly = false;
			}
		}
		if ( !pathOnly )
		{
			// try subdir off user at higher priority
			gConfig.AddPathVar( var, gpstring( "%user_path%/" ) + subDir, create ? Config::PVM_CREATE : Config::PVM_REQUIRED );

			// try subdir off home
			if ( !gConfig.AddPathVar( var, gpstring( "%home%/" ) + subDir, Config::PVM_REQUIRED ) )
			{
				// ok just use the home dir then
				gConfig.AddPathVar( var, "%home%", Config::PVM_REQUIRED );
			}
		}
	} };

	Local::AddTankPaths( "res_paths", "Resources", false );
	Local::AddTankPaths( "map_paths", "Maps", appCode == Services::APP_SIEGE_EDITOR );
	Local::AddTankPaths( "mod_paths", "Mods", appCode == Services::APP_SIEGE_EDITOR );

// Setup OUT and BITS paths.

	// $ these are only needed in dev builds

#	if !GP_RETAIL

	// set OUT dir
	if ( !gConfig.SetPathVar( "out", (const char*)NULL, Config::PVM_USE_CONFIG | Config::PVM_WRITABLE | Config::PVM_ABSOLUTE_PATH | Config::PVM_ERROR_ON_FAIL ) )
	{
		// detect from installer key in registry
		gpwstring outPath;
		{
			Registry registry( "Gas Powered Games", "DSTK", true );
			outPath = registry.GetWString( "1.0/Paths/Out" );
			if ( !outPath.empty() )
			{
				gConfig.SetPathVar( "out", outPath, Config::PVM_CREATE | Config::PVM_ABSOLUTE_PATH | Config::PVM_ERROR_ON_FAIL );
			}
		}

		// still not set?
		if ( outPath.empty() )
		{
			if ( gConfig.IsDevMode() )
			{
				// LD's are expecting to work directly in the home dir
				gConfig.SetPathVar( "out", "%home%", Config::PVM_CREATE | Config::PVM_ABSOLUTE_PATH | Config::PVM_ERROR_ON_FAIL );
			}
			else
			{
				// default to path off user dir
				gConfig.SetPathVar( "out", "%user_path%/Bits", Config::PVM_CREATE | Config::PVM_ABSOLUTE_PATH | Config::PVM_ERROR_ON_FAIL );
			}
		}
	}

	// set BITS dirs, using a prefix of ! will prevent mounting of default bits dirs
	gConfig.DeletePathVar( "bits" );
	bool bitsOnly = false;
	gpstring bitsPath = GetOverridePath( "bits", bitsOnly );
	if ( !bitsPath.empty() )
	{
		gConfig.AddPathVar( "bits", bitsPath, Config::PVM_REQUIRED | Config::PVM_ERROR_ON_FAIL );
	}
	if ( !bitsOnly )
	{
		gConfig.AddPathVar( "bits", "%out%", Config::PVM_REQUIRED );
		gConfig.AddPathVar( "bits", "%home%", Config::PVM_REQUIRED );
	}

#	endif // !GP_RETAIL
}

//////////////////////////////////////////////////////////////////////////////
