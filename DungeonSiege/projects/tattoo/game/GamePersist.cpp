//////////////////////////////////////////////////////////////////////////////
//
// File     :  GamePersist.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Game.h"
#include "GamePersist.h"

#include "CameraAgent.h"
#include "Config.h"
#include "ContentDb.h"
#include "FileSysXfer.h"
#include "FuBi.h"
#include "FuBiPersistBinary.h"
#include "FuBiPersistImpl.h"
#include "FuelDb.h"
#include "GameState.h"
#include "GoActor.h"
#include "GoCore.h"
#include "GoData.h"
#include "GoDb.h"
#include "GoSupport.h"
#include "LocHelp.h"
#include "MasterFileMgr.h"
#include "NamingKey.h"
#include "PathFileMgr.h"
#include "PContentDb.h"
#include "Player.h"
#include "RapiImage.h"
#include "RatioStack.h"
#include "Server.h"
#include "Siege_Loader.h"
#include "SkritEngine.h"
#include "StdHelp.h"
#include "TankBuilder.h"
#include "TankFileMgr.h"
#include "TankMgr.h"
#include "TattooGame.h"
#include "Ui_ChatBox.h"
#include "UiCamera.h"
#include "UiGameConsole.h"
#include "UiIntro.h"
#include "UiPartyManager.h"
#include "World.h"
#include "WorldFx.h"
#include "WorldMap.h"
#include "WorldMessage.h"
#include "WorldOptions.h"
#include "WorldState.h"
#include "WorldTerrain.h"
#include "WorldTime.h"

//////////////////////////////////////////////////////////////////////////////
// class FuelSaver declaration

class FuelSaver
{
public:
	SET_NO_INHERITED( FuelSaver );

	FuelSaver( const char* name, const wchar_t* saveFileName, Tank::Builder* tankBuilder )
	{
		// take tankbuilder
		m_TankBuilder = tankBuilder;

		// figure out options
		FuelDB::eOptions options = FuelDB::OPTION_WRITE | FuelDB::OPTION_NO_WRITE_TYPES;
		if ( tankBuilder != NULL )
		{
			options |= FuelDB::OPTION_MEM_ONLY;
		}

		// figure out names
		gpstring dbName = gpstring( name ) + "_save";
		gpstring address = "::" + dbName + ":root";
		m_GasName = gpstring( name ) + ".gas";

		// build text db
		m_Db = gFuelSys.AddTextDb( dbName );
		gpstring localSaveFileName( ::ToAnsi( saveFileName ) );
		m_Db->Init( options, localSaveFileName, localSaveFileName );

		// create base .gas file
		m_FuelBase = FuelHandle( address )->CreateChildBlock( name, m_GasName );

		// build the writers
		m_Writer = new FuBi::FuelWriter( m_FuelBase );
		m_Persist = new FuBi::PersistContext( m_Writer, false );
	}

	bool Commit( void )
	{
		bool success = true;

		if ( m_TankBuilder != NULL )
		{
			// build where it goes
			Tank::Builder::FileSpec fileSpec( m_GasName, TankConstants::DATAFORMAT_ZLIB );
			fileSpec.m_MinCompressionRatio = 0;

			// save the gas direct to tank
			if ( m_TankBuilder->BeginFile( fileSpec ) )
			{
				Tank::Writer writer( *m_TankBuilder );
				if ( !m_FuelBase->Write( writer ) )
				{
					success = false;
				}
				if ( !m_TankBuilder->EndFile() )
				{
					success = false;
				}
			}
			else
			{
				success = false;
			}
		}
		else
		{
			// save it direct
			success = m_Db->Save();
		}

		return ( success );
	}

   ~FuelSaver( void )
	{
		// clean up db
		gFuelSys.Remove( m_Db );

		// free objects
		delete ( m_Persist );
		delete ( m_Writer );
	}

	FuelHandle GetFuelBase( void ) const			{  gpassert( m_FuelBase );  return ( m_FuelBase );  }

	FuBi::PersistContext& GetPersist( void )		{  gpassert( m_Persist != NULL );  return ( *m_Persist );  }

private:
	TextFuelDB*              m_Db;
	gpstring                 m_GasName;
	FuelHandle               m_FuelBase;
	my FuBi::FuelWriter*     m_Writer;
	my FuBi::PersistContext* m_Persist;
	Tank::Builder*           m_TankBuilder;

	SET_NO_COPYING( FuelSaver );
};

//////////////////////////////////////////////////////////////////////////////
// struct SaveInfo implementation

SaveInfo :: SaveInfo( void )
{
	init();
}

SaveInfo :: SaveInfo( const SaveInfo& other )
{
	init();
	*this = other;
}

SaveInfo :: SaveInfo( const gpwstring& fileName, bool isQuickSave, bool isAutoSave )
{
	init();
	LoadFromSystem( fileName, isQuickSave, isAutoSave );
}

SaveInfo :: ~SaveInfo( void )
{
	delete ( m_FuBiManifest );
	delete ( m_TankManifest );
	delete ( m_Thumbnail );
}

void SaveInfo :: SaveToFuel( FuelHandle fuel, bool forHistory ) const
{
	if ( !forHistory )
	{
		fuel->Add( "player_name",     m_PlayerName      );
		fuel->Add( "hero_name",       m_HeroName        );
		fuel->Add( "map_screen_name", m_MapScreenName   );
		fuel->Add( "map_name",        m_MapName         );
		fuel->Add( "time_elapsed",    m_ElapsedTime     );
		fuel->Add( "single_player",   m_IsSinglePlayer  );
	}

	fuel->Add( "file_name",            m_FileName        );
	fuel->Add( "is_quick_save",        m_IsQuickSave     );
	fuel->Add( "is_auto_save",         m_IsAutoSave      );
	fuel->Add( "is_retail_content",    m_IsRetailContent );
	fuel->Add( "screen_name",          m_ScreenName      );
	fuel->Add( "save_time_text",       m_SaveTimeText    );
	fuel->Add( "time_elapsed_text",    m_ElapsedTimeText );
	fuel->Add( "product_version",      ToString( m_ProductVersion, gpversion::MODE_COMPLETE ) );
	fuel->Add( "product_compile_mode", m_ProductCompileMode, FVP_QUOTED_STRING );
	fuel->Add( "save_time",            ToString( m_SaveTime ) );
	fuel->Add( "teleport",             m_TeleportText );
	fuel->Add( "region",               m_RegionText   );
	fuel->Add( "save_duration",        m_SaveDuration );

	if ( m_FuBiManifest != NULL )
	{
		m_FuBiManifest->SaveToFuel( "fubi_manifest", fuel );
	}

	if ( m_TankManifest != NULL )
	{
		m_TankManifest->SaveToFuel( "tank_manifest", fuel );
	}

#	if GP_ENABLE_DEV_FEATURES

	fuel->Add( "warning_count",   ReportSys::GetTotalWarningCount           () );
	fuel->Add( "error_count",     ReportSys::GetTotalErrorCount             () );
	fuel->Add( "seh_count",       ::GlobalGetUnfilteredHandledExceptionCount() );
	fuel->Add( "computer_name",   SysInfo::GetComputerName                  () );
	fuel->Add( "user_name",       SysInfo::GetUserName                      () );
	fuel->Add( "system_memory",   SysInfo::GetTotalPhysicalMemory           () );
	fuel->Add( "cpu_speed",       SysInfo::GetCPUSpeedInHertz               () );
	fuel->Add( "is_win9x",        SysInfo::IsWin9x                          () );

	// think you can ignore those asserts and get away with it, hmm??? muahahaha...
	FuelHandle ignoredAsserts = fuel->CreateChildBlock( "ignored_asserts" );
	{
		gpstring str;

		int totalIgnoreAlways = 0;
		for ( AssertInstance* i = AssertInstance::ms_Root ; i != NULL ; i = i->m_Next )
		{
			if ( i->m_IgnoreCount < 0 )
			{
				++totalIgnoreAlways;
			}

			if ( i->m_File && *i->m_File )
			{
				str.assignf( "file %s, line %d, fail count %d", i->m_File, i->m_Line, i->m_FailCount );
			}
			else
			{
				str.assignf( "<location unknown>, fail count %d", i->m_FailCount );
			}
			ignoredAsserts->Add( "*", str );
		}

		ignoredAsserts->Add( "total_failure_count", AssertData::ms_FailedCount );
		ignoredAsserts->Add( "ignore_always_count", totalIgnoreAlways );
	}

	// $$$ include lots of other debug info in here!!!!
	// $$$ include psapi stuff like memory usage!!!!
	// $$$ if stats running, then do a quick dump of current stage and all logs into a stats file

#	endif // GP_ENABLE_DEV_FEATURES
}

bool SaveInfo :: LoadFromFuel( FuelHandle fuel, bool getThumbnail, bool wantSp, bool wantMp, bool forHistory )
{
	bool success = false;
	if ( fuel )
	{
		// make sure we're getting the mode we're interested in (note that we
		// may be interested in BOTH single and multiplayer infos)
		m_IsSinglePlayer = fuel->GetBool( "single_player" );
		if (   ( m_IsSinglePlayer && wantSp)
			|| (!m_IsSinglePlayer && wantMp)
			|| (!wantSp && !wantMp) )
		{
			if ( !forHistory )
			{
				fuel->Get( "player_name",     m_PlayerName    );
				fuel->Get( "hero_name",       m_HeroName      );
				fuel->Get( "map_screen_name", m_MapScreenName );
				fuel->Get( "map_name",        m_MapName       );
				fuel->Get( "time_elapsed",    m_ElapsedTime   );
			}

			m_ProductCompileMode = fuel->GetString( "product_compile_mode" );
			m_TeleportText       = fuel->GetString( "teleport"             );
			m_RegionText         = fuel->GetString( "region"               );

			if ( forHistory )
			{
				fuel->Get( "file_name",         m_FileName        );
				fuel->Get( "is_quick_save",     m_IsQuickSave     );
				fuel->Get( "is_auto_save",      m_IsAutoSave      );
				fuel->Get( "is_retail_content", m_IsRetailContent );
				fuel->Get( "screen_name",       m_ScreenName      );
				fuel->Get( "save_time_text",    m_SaveTimeText    );
				fuel->Get( "time_elapsed_text", m_ElapsedTimeText );
				fuel->Get( "save_duration",     m_SaveDuration    );
			}

			gpstring temp;
			fuel->Get( "product_version", temp );
			::FromString( temp, m_ProductVersion );
			fuel->Get( "save_time", temp );
			::FromString( temp, m_SaveTime );

			if ( getThumbnail && (m_Thumbnail == NULL) )
			{
				m_Thumbnail = RapiMemImage::CreateMemImage( "load://thumb.bmp", false, true );
			}

			Delete ( m_FuBiManifest );
			if ( fuel->HasChildBlock( "fubi_manifest" ) )
			{
				m_FuBiManifest = new FuBi::Manifest;
				m_FuBiManifest->LoadFromFuel( "fubi_manifest", fuel );
			}

			Delete ( m_TankManifest );
			if ( fuel->HasChildBlock( "tank_manifest" ) )
			{
				m_TankManifest = new TankManifest;
				m_TankManifest->LoadFromFuel( "tank_manifest", fuel );
			}

			// cool, it all worked
			success = true;
		}
	}
	return ( success );
}

void SaveInfo :: LoadFromSystem( const gpwstring& fileName, bool isQuickSave, bool isAutoSave )
{
	m_FileName           = fileName;
	m_IsQuickSave        = isQuickSave;
	m_IsAutoSave         = isAutoSave;
	m_IsRetailContent    = GP_RETAIL || gWorldOptions.GetForceRetailContent();
	m_ProductVersion     = SysInfo::GetExeFileVersion();
	m_ProductCompileMode = COMPILE_MODE_TEXT;

	SYSTEMTIME systemTime;
	::GetSystemTime( &systemTime );
	::SystemTimeToFileTime( &systemTime, &m_SaveTime );

	m_IsSinglePlayer = ::IsSinglePlayer();

	Go* hero = gServer.GetScreenHero();
	if ( hero != NULL )
	{
		m_TeleportText.assignf( "%s:n:%0.2f,%0.2f,%0.2f,%s",
				gWorldMap.GetMapName().c_str(),
				hero->GetPlacement()->GetPosition().pos.x,
				hero->GetPlacement()->GetPosition().pos.y,
				hero->GetPlacement()->GetPosition().pos.z,
				hero->GetPlacement()->GetPosition().node.ToString().c_str() );

		m_RegionText = gWorldMap.GetRegionNameForNode( hero->GetPlacement()->GetPosition().node );
	}

	m_ElapsedTime   = gWorldTime.GetTime();
	m_PlayerName    = gServer.GetScreenPlayer()->GetName();
	m_HeroName      = gServer.GetScreenPlayer()->GetHeroName();
	m_MapScreenName = gWorldMap.GetMapScreenName();
	m_MapName       = gWorldMap.GetMapName();

	if ( m_FuBiManifest == NULL )
	{
		m_FuBiManifest = new FuBi::Manifest;
	}
	m_FuBiManifest->LoadFromSystem();

	if ( m_TankManifest == NULL )
	{
		m_TankManifest = new TankManifest;
	}
	m_TankManifest->LoadFromSystem();
}

void SaveInfo :: ConvertForScreen( void )
{
	// build and adjust screen name if necessary
	if ( m_IsQuickSave )
	{
		m_ScreenName = gpwtranslate( $MSG$ "[Quick-Save]" );
	}
	else if ( m_IsAutoSave )
	{
		m_ScreenName = gpwtranslate( $MSG$ "[Auto-Save]" );
	}
	else
	{
		m_ScreenName = FileSys::GetFileNameOnly( m_FileName );
	}

	// convert time to local zone
	FILETIME localFileTime;
	::FileTimeToLocalFileTime( &m_SaveTime, &localFileTime );
	SYSTEMTIME localTime;
	::FileTimeToSystemTime( &localFileTime, &localTime );

	// format the date all nice
	char buffer[ 100 ];
	::GetDateFormat(
			LocMgr::DoesSingletonExist() ? gLocMgr.GetLocale() : LOCALE_USER_DEFAULT,
			0,
			&localTime,
			"ddd MMM dd yy",
			buffer,
			ELEMENT_COUNT( buffer ) );
	m_SaveTimeText = ::ToUnicode( buffer );
	m_SaveTimeText += L" - ";

	// do the time too
	::GetTimeFormat(
			LocMgr::DoesSingletonExist() ? gLocMgr.GetLocale() : LOCALE_USER_DEFAULT,
			0,
			&localTime,
			"hh':'mm':'ss tt",
			buffer,
			ELEMENT_COUNT( buffer ) );
	m_SaveTimeText += ::ToUnicode( buffer );

	// convert elapsed time
	float seconds = (float)m_ElapsedTime;
	int hours = 0, minutes = 0;
	::ConvertTime( hours, minutes, seconds );
	m_ElapsedTimeText.assignf( gpwtranslate( $MSG$ "%d:%02d:%02d" ), hours, minutes, (int)seconds );
}

DWORD SaveInfo :: CreateThumbnailTexture( void )
{
	// bail early if nothing to do
	if ( m_Thumbnail == NULL )
	{
		return ( 0 );
	}

	// make it a square power of two texture so video cards like it, note that
	// alpha fill with zero so extended area does not render
	int side = max( MakePower2Up( m_Thumbnail->GetWidth() ), MakePower2Up( m_Thumbnail->GetHeight() ) );
	RapiMemImage* thumbnail = new RapiMemImage( side, side, 0 );

	// copy bits in
	::CopyPixels( thumbnail->GetBits( thumbnail->GetHeight() - m_Thumbnail->GetHeight() ),
				  PIXEL_ARGB_8888, m_Thumbnail->GetWidth(), m_Thumbnail->GetHeight(), thumbnail->GetStride(),
				  m_Thumbnail->GetBits(),
				  PIXEL_ARGB_8888, m_Thumbnail->GetWidth(), m_Thumbnail->GetHeight(), m_Thumbnail->GetStride(),
				  false, false );

	// build name
	gpstring name;
	name.assignf( "Thumbnail: %S", m_FileName.c_str() );

	// build texture
	return ( gDefaultRapi.CreateAlgorithmicTexture( name, thumbnail, false, 0, TEX_LOCKED, false, false ) );
}

SaveInfo& SaveInfo :: operator = ( const SaveInfo& other )
{
	if ( this == &other )
	{
		return ( *this );
	}

	Delete( m_FuBiManifest );
	Delete( m_TankManifest );
	Delete( m_Thumbnail );

	m_FileName           = other.m_FileName;
	m_IsQuickSave        = other.m_IsQuickSave;
	m_IsAutoSave         = other.m_IsAutoSave;
	m_IsRetailContent    = other.m_IsRetailContent;
	m_ScreenName         = other.m_ScreenName;
	m_SaveTimeText       = other.m_SaveTimeText;
	m_ElapsedTimeText    = other.m_ElapsedTimeText;
	m_ProductVersion     = other.m_ProductVersion;
	m_ProductCompileMode = other.m_ProductCompileMode;
	m_SaveTime           = other.m_SaveTime;
	m_IsSinglePlayer     = other.m_IsSinglePlayer;
	m_TeleportText       = other.m_TeleportText;
	m_RegionText         = other.m_RegionText;
	m_ElapsedTime        = other.m_ElapsedTime;
	m_SaveDuration       = other.m_SaveDuration;
	m_PlayerName         = other.m_PlayerName;
	m_HeroName           = other.m_HeroName;
	m_MapScreenName      = other.m_MapScreenName;
	m_MapName            = other.m_MapName;

	if ( other.m_FuBiManifest != NULL )
	{
		m_FuBiManifest = new FuBi::Manifest( *other.m_FuBiManifest );
	}
	if ( other.m_TankManifest != NULL )
	{
		m_TankManifest = new TankManifest( *other.m_TankManifest );
	}
	if ( other.m_Thumbnail != NULL )
	{
		m_Thumbnail = new RapiMemImage( *other.m_Thumbnail );;
	}

	return ( *this );
}

void SaveInfo :: init( void )
{
	::ZeroObject( m_SaveTime );

	m_IsQuickSave     = false;
	m_IsAutoSave      = false;
	m_IsRetailContent = false;
	m_IsSinglePlayer  = true;
	m_ElapsedTime     = 0;
	m_SaveDuration    = 0;
	m_FuBiManifest    = NULL;
	m_TankManifest    = NULL;
	m_Thumbnail       = NULL;
}

//////////////////////////////////////////////////////////////////////////////
// struct CharacterInfo implementation

CharacterInfo :: ~CharacterInfo( void )
{
	Delete( m_Portrait );
}

//////////////////////////////////////////////////////////////////////////////
// class GameXfer implementation

GameXfer :: GameXfer( eOptions options )
	: m_Options( options )
{
	m_StoppedLoadingThread = false;

	SetOptions( OPTION_TEXT, gWorldOptions.GetSaveAsText() );
	SetOptions( OPTION_TANK, gWorldOptions.GetSaveAsTank() );
}

gpwstring GameXfer :: MakeSaveGameFileName( const wchar_t* name, eOptions options, bool wildcard )
{
	gpwstring fileName;

	// give it a name if none
	if ( (name == NULL) || (*name == L'\0') )
	{
		if ( options & OPTION_QUICK_SAVE )
		{
			fileName = L"Quick-Save";
		}
		else if ( options & OPTION_AUTO_SAVE )
		{
			fileName = L"Auto-Save";
		}
		else
		{
			fileName = L"Default";
		}
	}
	else
	{
		fileName = name;
	}

	// add path info if none
	if ( !FileSys::HasPathInfo( fileName ) )
	{
		fileName = gConfig.ResolvePathVars( L"%save_path%/" + fileName );
	}

	// extension if none
	fileName = AddSaveGameExtension( fileName, options, wildcard );

	// done
	return ( fileName );
}

gpwstring GameXfer :: AddSaveGameExtension( const wchar_t* name, eOptions options, bool wildcard )
{
	gpassert( (name != NULL) && (*name != L'\0') );
	gpwstring fileName = name;

	// add extension if none
	if ( !FileSys::HasExtension( fileName ) )
	{
		if ( options & OPTION_WORLD_SAVE )
		{
			if ( wildcard && (options & OPTION_PARTY_SAVE) )
			{
				fileName += L".ds*";
			}
			else if ( options & OPTION_SINGLEPLAYER )
			{
				if ( wildcard && (options & OPTION_MULTIPLAYER) )
				{
					fileName += L".ds*save*";
				}
				else if ( options & OPTION_QUICK_SAVE )
				{
					fileName += L".dsqsave";
				}
				else if ( options & OPTION_AUTO_SAVE )
				{
					fileName += L".dsasave";
				}
				else
				{
					fileName += L".dssave";
				}
			}
			else
			{
				if ( options & OPTION_QUICK_SAVE )
				{
					fileName += L".dsqsavem";
				}
				else if ( options & OPTION_AUTO_SAVE )
				{
					fileName += L".dsasavem";
				}
				else
				{
					fileName += L".dssavem";
				}
			}
		}
		else if ( options & OPTION_PARTY_SAVE )
		{
			fileName += L".dsparty";
		}
	}

	// done
	return ( fileName );
}

GameXfer::eOptions GameXfer :: MakeSaveOptionsAndName( const wchar_t*& name, bool quickSave, bool autoSave )
{
	eOptions options = OPTION_WORLD_SAVE | OPTION_PARTY_SAVE;

	// multiplayer?
	if ( ::IsMultiPlayer() )
	{
		options |= OPTION_MULTIPLAYER;
	}
	else
	{
		options |= OPTION_SINGLEPLAYER;
	}

	// specific file
	if ( quickSave )
	{
		options |= OPTION_QUICK_SAVE;
		if ( (name == NULL) || (*name == L'\0') )
		{
			name = L"Quick-Save";
		}
	}
	else if ( autoSave )
	{
		options |= OPTION_AUTO_SAVE;
		if ( (name == NULL) || (*name == L'\0') )
		{
			name = L"Auto-Save";
		}
	}

	// default/missing filename
	if ( (name == NULL) || (*name == L'\0') )
	{
		name = L"Default";
	}

	return ( options );
}

bool GameXfer :: DoesSaveGameExist( const wchar_t* name, eOptions options )
{
	gpwstring fileName = MakeSaveGameFileName( name, options );
	return ( FileSys::DoesFileExist( fileName ) || FileSys::DoesPathExist( fileName ) );
}

bool GameXfer :: HasValidSaveGameExtension( const wchar_t* name )
{
	if ( name == NULL )
	{
		return ( false );
	}

	const wchar_t* ext = FileSys::GetExtension( name, true );
	if ( ext == NULL )
	{
		return ( false );
	}

	return (   ::same_no_case( ext, (const wchar_t*)L".dsqsave"  )
			|| ::same_no_case( ext, (const wchar_t*)L".dsasave"  )
			|| ::same_no_case( ext, (const wchar_t*)L".dssave"   )
			|| ::same_no_case( ext, (const wchar_t*)L".dsqsavem" )
			|| ::same_no_case( ext, (const wchar_t*)L".dsasavem" )
			|| ::same_no_case( ext, (const wchar_t*)L".dssavem"  )
			|| ::same_no_case( ext, (const wchar_t*)L".dsparty"  ) );
}

bool GameXfer :: XferCommon( FuBi::PersistContext& persist )
{
	// xfer the world
	gWorld.Xfer( persist );
	if ( persist.IsInFatalError() )
	{
		return ( false );
	}

	// xfer the player
	persist.EnterBlock( "ScreenPlayer" );
	gServer.GetScreenPlayer()->Xfer( persist );
	persist.LeaveBlock();

	// xfer the game engine
	persist.EnterBlock( "TattooGame" );
	gTattooGame.Xfer( persist );
	persist.LeaveBlock();

	// load-only...
	if ( persist.IsRestoring() )
	{
		// finish up skrit engine now that the game is up but before UI runs
		gSkritEngine.CommitLoad();

		// notify fresh go's we're (pretty much) all done
		WorldMessage( WE_POST_RESTORE_GAME, GOID_ANY ).Send();
	}

	// xfer the UI camera
	persist.EnterBlock( "UICamera" );
	gUICamera.Xfer( persist );
	persist.LeaveBlock();

	// xfer the UI
	persist.EnterBlock( "UIGame" );
	gUIGame.Xfer( persist );
	persist.LeaveBlock();

	// update engine
	gSiegeEngine.UpdateFrustumOwnership();

	// guess everything went ok
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// class GameSaver implementation

// $$$$$ implement a "bad bit" for the saver that will get set on the first
//       error that occurs, saving out the ::GetLastError(). check this on
//       occasion to abort the save and then report the error via nice gui
//       dialog...

#if GP_ENABLE_DEV_FEATURES
SaveInfoColl GameSaver::ms_SaveHistory;
#endif // GP_ENABLE_DEV_FEATURES

GameSaver :: GameSaver( eOptions options )
	: Inherited( options )
{
	m_StartTime = 0;
	m_TankBuilder = NULL;
}

GameSaver :: ~GameSaver( void )
{
	AbortSave();
}

bool GameSaver :: BeginSave( const wchar_t* saveName )
{
	bool success = true;

	m_StartTime = ::GetSystemSeconds();

	// $$$ MP: verify that all clients are in sync before saving

	// make name and path, and get an ansi version for use locally
	m_SaveFileName = MakeSaveGameFileName( saveName, GetOptions() );
	gpstring saveFileName = ::ToAnsi( m_SaveFileName );

	// remove old save game dir if it exists. this is necessary to clean out
	// a dir when we're converting it to a tank. don't do in retail just because
	// i'm paranoid and that case should not come up there.
#	if !GP_RETAIL
	if ( TestOptions( OPTION_TANK ) )
	{
		bool exists;
		if ( !DeleteSave( saveName, &exists, TestOptions( OPTION_TANK ) ) && exists )
		{
			return ( false );
		}
	}
#	endif // !GP_RETAIL

	// now start the save
	if ( TestOptions( OPTION_TANK ) )
	{
		// construct a tank builder
		m_TankBuilder = new Tank::Builder;

		// get options
		Tank::Builder::eOptions tankOptions =
			  Tank::Builder::OPTION_PE_FORMAT
			| Tank::Builder::OPTION_USE_TEMP_FILE;

		// if it's quick/autosave or party-only save, then backup the old one else overwrite
		if ( TestOptions( OPTION_QUICK_SAVE | OPTION_AUTO_SAVE ) || !TestOptions( OPTION_WORLD_SAVE ) )
		{
			tankOptions |= Tank::Builder::OPTION_BACKUP_OLD;
		}

		// set options on how to build
		m_TankBuilder->SetOptions( tankOptions );

		// set its header
		m_TankBuilder->SetTankProductVersion( SysInfo::GetExeFileVersion() );
		m_TankBuilder->SetTankMinimumVersion( SysInfo::GetExeFileVersion() );
		m_TankBuilder->AddTankFlags( Tank::TANKFLAG_ALLOW_MULTIPLAYER_XFER );
		m_TankBuilder->SetCreatorId( CREATOR_GPG );
		m_TankBuilder->SetTitleText( saveName );
		m_TankBuilder->SetAuthorText( gServer.GetScreenPlayer()->GetName() );

		// create it
		success = m_TankBuilder->CreateTankAlways( m_SaveFileName );
	}
	else
	{
		// remove old save game dir if it exists
		bool exists;
		if ( !DeleteSave( saveName, &exists ) && exists )
		{
			return ( false );
		}

		// create subdir
		success = !!::CreateDirectory( saveFileName, NULL );

		// create subdir for subdir
		if ( success && TestOptions( OPTION_TEXT ) && TestOptions( OPTION_WORLD_SAVE ) )
		{
			success = !!::CreateDirectory( saveFileName + "\\debug", NULL );
		}
	}

	// handle failure
	if ( !success )
	{
		// reset on failure
		reset();
	}

	// done
	return ( success );
}

bool GameSaver :: FullSave( void )
{
	// $ note the bitwise AND to avoid short-circuiting

	bool success = true;

	gGameState.StartProgress( false );

	{
		RatioSample ratioSample( "save_game" );

		{
			RatioSample ratioSample( "save_world", 0, 0.67 );
			success &= SaveWorld();
		}
		{
			RatioSample ratioSample( "save_party", 0, 0.01 );
			success &= SaveParty();
		}
		{
			RatioSample ratioSample( "save_portraits", 0, 0.01 );
			success &= SavePortraits();
		}
		{
			RatioSample ratioSample( "save_thumbnail", 0, 0.26 );
			success &= SaveThumbnail();
		}
		{
			RatioSample ratioSample( "save_commit" );
			success &= CommitSave();
		}
	}

	gGameState.StopProgress();

	return ( success );
}

bool GameSaver :: FullPartySave( int slot )
{
	// $ note the bitwise AND to avoid short-circuiting

	return ( SaveParty( slot ) & SavePortraits( slot ) & CommitSave() );
}

bool GameSaver :: CommitSave( void )
{
	bool success = true;

	// put in a header
	{

		// build writer
		FuelSaver fuelSaver( "info", m_SaveFileName, m_TankBuilder );
		FuelHandle fuel = fuelSaver.GetFuelBase();

		// build and save state
		SaveInfo saveInfo( m_SaveFileName, TestOptions( OPTION_QUICK_SAVE ), TestOptions( OPTION_AUTO_SAVE ) );
		saveInfo.ConvertForScreen();
		saveInfo.m_SaveDuration = PreciseSubtract( ::GetSystemSeconds(), m_StartTime );
		saveInfo.SaveToFuel( fuel );

#		if GP_ENABLE_DEV_FEATURES
		if ( ::IsSinglePlayer() )
		{
			// save current history
			if ( !ms_SaveHistory.empty() )
			{
				FuelHandle history = fuel->CreateChildBlock( "history" );
				SaveInfoColl::reverse_iterator i, ibegin = ms_SaveHistory.rbegin(), iend = ms_SaveHistory.rend();
				for ( i = ibegin ; i != iend ; ++i )
				{
					FuelHandle child = history->CreateChildBlock( "*" );
					(*i)->SaveToFuel( child, true );
				}
			}

			// store new entry in history
			AddSaveHistory( saveInfo );
		}
#		endif // GP_ENABLE_DEV_FEATURES

		// commit
		if ( !fuelSaver.Commit() )
		{
			success = false;
		}
	}

	// commit the tank if any
	if ( (m_TankBuilder != NULL) && !m_TankBuilder->CommitTank() )
	{
		success = false;
	}

	// delete failed save
	if ( !success )
	{
		DeleteSave( m_SaveFileName );
	}

	// clear it out
	reset();

	// done
	return ( success );
}

bool GameSaver :: AbortSave( void )
{
	bool success = true;

	// abort
	if ( m_TankBuilder != NULL )
	{
		m_TankBuilder->AbortTank();
	}

	// clear it out
	reset();

	// done
	return ( success );
}

bool GameSaver :: SaveParty( int slot )
{
	gpassert( IsSaving() );
	gpassert( TestOptions( OPTION_PARTY_SAVE ) );

	// find the party $ early bailout if none
	Go* party = gServer.GetScreenParty();
	if ( !party )
	{
		return ( false );
	}

	// build our saver
	FuelSaver fuelSaver( "party", m_SaveFileName, m_TankBuilder );
	FuBi::PersistContext& persist = fuelSaver.GetPersist();

	// get some vars
	int heroIndex = 0;
	GoHandle hero( gServer.GetScreenPlayer()->GetHero() );

	// for each child...
	persist.EnterColl( "members" );
	GopColl::iterator i, ibegin = party->GetChildBegin(), iend = party->GetChildEnd();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// skip?
		if ( (slot != -1) && (slot != (int)(i - ibegin)) )
		{
			continue;
		}

		// advance the pointer
		persist.AdvanceCollIter();

		// enter a non-named block so that it's easier to import
		persist.EnterBlock( "member" );

		// template
		gpstring templateName = (*i)->GetTemplateName();
		persist.Xfer( "template_name", templateName );

		// character
		(*i)->XferCharacter( persist );

		// hero?
		if ( (*i == hero) || (slot != -1) )
		{
			heroIndex = i - ibegin;
		}

		persist.LeaveBlock();
	}
	persist.LeaveColl();

	// calc gold
	int gold = 0;
	if ( hero )
	{
		gold = hero->GetInventory()->GetGold();
	}

	// xfer party fun
	persist.EnterColl( "party" );
	persist.Xfer( "hero_index", heroIndex );
	persist.Xfer( "gold", gold );
	persist.LeaveColl();
	
	// xfer the character's player stash	
	if ( gServer.GetScreenStash() )
	{
		persist.EnterBlock( "stash" );	
		gpstring sTemplate = gServer.GetScreenStash()->GetTemplateName() ;
		persist.Xfer( "template_name", sTemplate );
		gServer.GetScreenStash()->GetInventory()->XferCharacter( persist );
		persist.LeaveBlock();
	}	

	// commit
	return ( fuelSaver.Commit() );
}

bool GameSaver :: SavePortraits( int slot )
{
	gpassert( IsSaving() );
	gpassert( TestOptions( OPTION_PARTY_SAVE | OPTION_WORLD_SAVE ) );

	// find the party $ early bailout if none
	Go* party = gServer.GetScreenParty();
	if ( !party )
	{
		return ( false );
	}

	bool success = true;

	GopColl::iterator i, ibegin = party->GetChildBegin(), iend = party->GetChildEnd();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// skip?
		if ( (slot != -1) && (slot != (int)(i - ibegin)) )
		{
			continue;
		}

		// portrait
		DWORD portraitTex = (*i)->GetActor()->GetPortraitTexture();
		if ( portraitTex )
		{
			// get index
			int index = (slot == -1) ? (i - ibegin) : 0;

			// build filename (same as index into party)
			gpstring portraitFileName = "portrait-" + ToString( index ) + ".bmp";

			// build writer
			std::auto_ptr <FileSys::StreamWriter> writer( createWriter( portraitFileName ) );

			// build reader
			std::auto_ptr <RapiImageReader> reader( RapiImageReader::CreateReader( gDefaultRapiPtr, portraitTex, false ) );

			// write it out and commit
			if ( !::WriteImage( *writer, IMAGE_BMP, reader.get(), true, false ) )
			{
				success = false;
			}
			if ( !commitWriter( writer.release() ) )
			{
				success = false;
			}
		}
	}

	return ( success );
}

bool GameSaver :: SaveWorld( void )
{
	gpassert( IsSaving() );
	gpassert( TestOptions( OPTION_WORLD_SAVE ) );

#	if !GP_RETAIL
	if ( ::GlobalGetUnfilteredHandledExceptionCount() != 0 )
	{
		gperrorf((
				"Hello. You are saving a game after %d SEH errors have "
				"occurred (exceptions caused and caught by bad skrit code, "
				"usually). This is bad. The save game you are about to make "
				"should be considered unstable and should not be used to "
				"generate bug reports. Once the SEH errors are fixed (those "
				"should definitely go in RAID) the world will be safe for "
				"saving again. Thank you. -SB",
				::GlobalGetUnfilteredHandledExceptionCount() ));
	}
#	endif // !GP_RETAIL

// Prep game for save.

	{
		// prevent this simple stuff from cluttering our progress bar
		RatioStack::Locker locker;

		// notify all go's we're saving...
		WorldMessage( WE_PRE_SAVE_GAME, GOID_ANY ).Send();

		// force shutdown of world streaming while we save
		m_StoppedLoadingThread = gSiegeEngine.IsLoadingThreadRunning();
		gSiegeEngine.StopLoadingThread();

		// force all nodes to load and flush pending nodes
		gWorldTerrain.ForceWorldLoad();

		// commit any pending requests
		gGoDb.CommitAllRequests();

		// clear out any targets and scripts pending deletion
		gWorldFx.PrepareForSave();

		// lock it out
		gGoDb.SetLockedForSaveLoad();
	}

// Xfer binary.

	bool success = true;

	{
		RatioSample ratioSample( "save_world_binary", 0, TestOptions( OPTION_TEXT ) ? 0.50 : 0 );

		// build writers
		std::auto_ptr <FileSys::StreamWriter> dataWriter( createWriter( "world.xdat" ) );
		FuBi::TreeBinaryWriter xferWriter( *dataWriter );
		FuBi::PersistContext persist( &xferWriter );

		{
			RatioSample ratioSample( "save_world_data", 0, 0.80 );

			// xfer data
			xfer( persist );

			// commit data
			if ( !commitWriter( dataWriter.release() ) )
			{
				success = false;
			}
		}

		{
			RatioSample ratioSample( "save_world_index" );

			// write index
			std::auto_ptr <FileSys::StreamWriter> indexWriter( createWriter( "world.xidx" ) );
			if ( !xferWriter.WriteIndex( *indexWriter ) )
			{
				success = false;
			}

			// commit index
			if ( !commitWriter( indexWriter.release() ) )
			{
				success = false;
			}
		}
	}

	// safe to unlock now
	gGoDb.SetLockedForSaveLoad( false );

// Xfer text.

	// $ this unfortunately requires a second xfer, as we can't xfer to two
	//   destinations simultaneously (as was possible with fuel, which built
	//   the text db no matter what).

	if ( TestOptions( OPTION_TEXT ) )
	{
		RatioSample ratioSample( "save_world_text" );

		// $$$ do xml writer instead

		// build writers
		std::auto_ptr <FileSys::StreamWriter> dataWriter( createWriter( "debug\\world.gas" ) );
		FuBi::GasWriter xferWriter( *dataWriter );
		FuBi::PersistContext persist( &xferWriter );

		// xfer
		xfer( persist );

		// commit data
		if ( !commitWriter( dataWriter.release() ) )
		{
			success = false;
		}
	}

// Finish.

	// notify all go's we're done...
	WorldMessage( WE_POST_SAVE_GAME, GOID_ANY ).Send();

	// done
	return ( success );
}

bool GameSaver :: SaveThumbnail( void )
{
	gpassert( IsSaving() );
	gpassert( TestOptions( OPTION_WORLD_SAVE ) );

	// get prefs
	FastFuelHandle prefs( "config:global_settings:save_game_settings" );
	if ( !prefs )
	{
		return ( false );
	}
	int thumbWidth = prefs.GetInt( "thumbnail_width", 80 );
	int thumbHeight = prefs.GetInt( "thumbnail_height", 60 );

	std::auto_ptr <RapiMemImage> image;
	{
		RatioSample ratioSample( "render_shot", 0, 0.90 );

		// render a clean image
		gTattooGame.RenderFrame( false );

		// get image
		stdx::make_auto_ptr( image, gDefaultRapi.CreateSubsectionImage( false ) );
		if ( (image.get() == NULL) || (image->GetSizeInPixels() == 0) )
		{
			return ( false );
		}

		// restore old image
		gTattooGame.RenderFrame( true );
	}

	bool success = true;

	// downsample
	RapiMemImage thumb( thumbWidth, thumbHeight );
	{
		RatioSample ratioSample( "downsample_shot", 0, 0.08 );
		::BoxFilter( thumb.GetBits(), thumb.GetWidth(), thumb.GetHeight(), thumb.GetStride(),
					 image->GetBits(), image->GetWidth(), image->GetHeight(), image->GetStride() );
	}

	// build writer
	std::auto_ptr <FileSys::StreamWriter> writer( createWriter( "thumb.bmp" ) );

	// write it out and commit
	if ( !writer.get() || !::WriteImage( *writer, IMAGE_BMP, thumb, true, false, true ) )
	{
		success = false;
	}
	if ( !commitWriter( writer.release() ) )
	{
		success = false;
	}

	// now re-render that back buffer with gui so we don't get weird stutter
	gTattooGame.RenderFrame( true );

	return ( success );
}

bool GameSaver :: DeleteSave( const wchar_t* saveName, bool* saveFileExists, bool dirOnly )
{
	gpstring filename;
	bool success = false, exists = false;

	// attempt to delete a tank file if it exists
	filename = ::ToAnsi( MakeSaveGameFileName( saveName, GetOptions() | OPTION_TANK ) );
	if ( FileSys::DoesFileExist( filename ) )
	{
		// if we're only interested in removing dirs, then bail out here. the
		// tank system will take care of deleting that file.
		if ( dirOnly )
		{
			return ( true );
		}

		exists = true;

		// this may fail if it's read-only or if the file doesn't exist. that's
		// ok, don't want to be deleting read-only save games!!
		if ( ::DeleteFile( filename ) )
		{
			success = true;
		}
	}

	// attempt to remove the save game subfolder if it exists
	filename = ::ToAnsi( MakeSaveGameFileName( saveName, GetOptions() & NOT( OPTION_TANK ) ) );
	if ( FileSys::DoesPathExist( filename ) )
	{
		exists = true;
		if ( FileSys::RemoveDirectory( filename, 1 ) )		// recurse to remove 'debug' subdir if it's there
		{
			success = true;
		}
	}

	// save exist if necessary
	if ( saveFileExists != NULL )
	{
		*saveFileExists = exists;
	}

	// done
	return ( success );
}

#if GP_ENABLE_DEV_FEATURES

void GameSaver :: LoadSaveHistory( FuelHandle fuel )
{
	ClearSaveHistory();

	if ( ::IsSinglePlayer() && fuel )
	{
		FuelHandle history = fuel->GetChildBlock( "history" );
		if ( history )
		{
			FuelHandleList children = history->ListChildBlocks();
			FuelHandleList::reverse_iterator i, ibegin = children.rbegin(), iend = children.rend();
			for ( i = ibegin ; i != iend ; ++i )
			{
				(*ms_SaveHistory.push_back( new SaveInfo ))->LoadFromFuel( *i, false, false, false, true );
			}
		}
	}
}

#endif // GP_ENABLE_DEV_FEATURES

void GameSaver :: reset( void )
{
	m_SaveFileName.clear();
	Delete( m_TankBuilder );

	// safe to unlock now (do it just in case we aborted an earlier op)
	if ( GoDb::DoesSingletonExist() && gGoDb.IsLockedForSaveLoad() )
	{
		gGoDb.SetLockedForSaveLoad( false );
	}

	// restart loader if needed
	if ( m_StoppedLoadingThread )
	{
		gSiegeEngine.StartLoadingThread();
	}
}

void GameSaver :: xfer( FuBi::PersistContext& persist )
{
	// xfer skrit engine
	{
		RatioSample ratioSample( "save_skrit_engine", 0, 0.17 );

		persist.EnterBlock( "SkritEngine" );
		gSkritEngine.Xfer( persist );
		persist.LeaveBlock();
	}

	// xfer common stuff
	{
		RatioSample ratioSample( "" );
		XferCommon( persist );
	}

	// xfer party gridboxes
	persist.EnterColl( "GridBoxes" );
	Go* party = gServer.GetScreenParty();
	if ( party != NULL )
	{
		GopColl::iterator i, ibegin = party->GetChildBegin(), iend = party->GetChildEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( (*i)->HasInventory() )
			{
				persist.AdvanceCollIter();

				// goid
				Goid goid = (*i)->GetGoid();
				persist.Xfer( "goid", goid );

				// character
				(*i)->GetInventory()->XferGridbox( persist );
			}
		}
	}

	// xfer the player's stash
	Player* player = gServer.GetScreenPlayer();
	if ( player != NULL )
	{
		GoHandle hStash( player->GetStash() );
		if ( hStash && hStash->HasStash() && hStash->HasInventory() )
		{
			persist.AdvanceCollIter();

			// goid
			Goid goid = hStash->GetGoid();
			persist.Xfer( "goid", goid );

			// character
			hStash->GetInventory()->XferGridbox( persist );			
		}
	}

	persist.LeaveColl();

	// xfer whatever is left
	eWorldState worldState = gWorldState.GetCurrentState();
	persist.EnterBlock( "misc" );
	persist.Xfer( "world_state", worldState );
	persist.LeaveBlock();
}

FileSys::StreamWriter* GameSaver :: createWriter( const char* fileName )
{
	FileSys::StreamWriter* writer = NULL;

	// build data writer
	if ( m_TankBuilder != NULL )
	{
		// get a filespec
		Tank::Builder::FileSpec fileSpec( fileName, TankConstants::DATAFORMAT_ZLIB );
		fileSpec.m_MinCompressionRatio = 0;

		// start the file
		if ( m_TankBuilder->BeginFile( fileSpec ) )
		{
			writer = new Tank::Writer( *m_TankBuilder );
		}
	}
	else
	{
		// create the file
		gpstring localFileName = ::ToAnsi( m_SaveFileName ) + "\\" + fileName;
		FileSys::File localFile;
		if ( localFile.CreateAlways( localFileName, FileSys::File::ACCESS_WRITE_ONLY, FileSys::File::SHARE_READ_ONLY ) )
		{
			writer = new FileSys::FileWriter( localFile.Release(), true );
		}
	}

	// got me a writer!
	return ( writer );
}

bool GameSaver :: commitWriter( FileSys::StreamWriter* writer )
{
	bool success = true;

	Delete( writer );

	if ( (m_TankBuilder != NULL) && !m_TankBuilder->EndFile() )
	{
		success = false;
	}

	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// class GameLoader implementation

GameLoader :: GameLoader( eOptions options )
	: Inherited( options )
{
	m_Db = NULL;
	m_FileMgr = NULL;
	m_LoadedWorldState = WS_SP_INGAME;

	::ZeroObject( m_TimeStamp );
}

GameLoader :: ~GameLoader( void )
{
	EndLoad();
}

bool GameLoader :: BeginLoad( const wchar_t* loadName )
{
	bool success = true;

	// save original name
	if ( loadName != NULL )
	{
		m_OriginalName = loadName;
	}

	// make name and path
	m_LoadFileName = GameSaver::MakeSaveGameFileName( loadName, GetOptions() );
	gpstring loadFileName = ::ToAnsi( m_LoadFileName );

	// try it as a tank first
	if ( FileSys::DoesFileExist( m_LoadFileName ) )
	{
		// do we want to verify data on open? only bother with party saves b/c
		// they are important to make MP work and they're little.
		bool verifyData = (GetOptions() & OPTION_PARTY_SAVE) && !(GetOptions() & OPTION_WORLD_SAVE);

		// build tank mgr
		FileSys::TankFileMgr* tankFileMgr = new FileSys::TankFileMgr;
		m_FileMgr = tankFileMgr;
		if ( tankFileMgr->OpenTank( loadFileName, true, verifyData ) )
		{
			m_TimeStamp = tankFileMgr->GetTankBuildTime();
		}
		else
		{
			success = false;
		}
	}
	else
	{
		// ok try it as a path
		success = FileSys::DoesPathExist( m_LoadFileName );
		if ( success )
		{
			FileSys::PathFileMgr* pathFileMgr = new FileSys::PathFileMgr( loadFileName );
			m_FileMgr = pathFileMgr;
			if ( pathFileMgr->IsReady() )
			{
				FileSys::FileFinder finder( loadFileName );
				if ( finder.Next() )
				{
					m_TimeStamp = finder.Get().ftCreationTime;
				}
			}
			else
			{
				success = false;
			}
		}
	}

	// mount file system and build fueldb if success
	if ( success )
	{
		// add it as a driver
		((FileSys::MasterFileMgr*)FileSys::IFileMgr::GetMasterFileMgr())->AddDriver( m_FileMgr, false, "load" );

		// build fuel db
		m_Db = gFuelSys.AddTextDb( "load" );
		m_Db->Init( FuelDB::OPTION_READ | FuelDB::OPTION_JIT_READ, "load://", "" );

		// gather save info
		if ( FillInfo( m_SaveInfo, false ) )
		{
			// do a quick check to make sure we have the map we want!
			if ( GetOptions() & OPTION_WORLD_SAVE )
			{
				if ( !gTankMgr.HasMap( m_SaveInfo.m_MapName ) )
				{
					success = false;
				}

				// $$$$$ do tank and dsdll manifest here - and if there is a mismatch then FAIL
			}
		}
		else
		{
			success = false;
		}
	}

	// kill on failure
	if ( !success )
	{
		EndLoad();
	}

	// done
	return ( success );
}

void GameLoader :: EndLoad( void )
{
	// clear it out
	reset();
}

bool GameLoader :: LoadWorld( void )
{
	// $$$ MP: do sync or whatever...

	gpassert( IsLoading() );
	gpassert( TestOptions( OPTION_WORLD_SAVE ) );

// Setup.

#	if GP_ENABLE_DEV_FEATURES
	{
		GameSaver::LoadSaveHistory( FuelHandle( "::load:info" ) );
		SaveInfo lastSave;
		lastSave.LoadFromFuel(
				FuelHandle( "::load:info" ),
				false,
				TestOptions( OPTION_SINGLEPLAYER ),
				TestOptions( OPTION_MULTIPLAYER ),
				true);
		GameSaver::AddSaveHistory( lastSave );
	}
#	endif // GP_ENABLE_DEV_FEATURES

	// open handles
	FileSys::AutoFileHandle indexFile, dataFile;
	FileSys::AutoMemHandle indexMem, dataMem;
	if (   !indexFile.Open( "load://world.xidx" ) || !indexMem.Map( indexFile )
		|| !dataFile .Open( "load://world.xdat" ) || !dataMem .Map( dataFile  ) )
	{
		return ( false );
	}

	// build reader
	FuBi::TreeBinaryReader reader;
	if ( !reader.Init( const_mem_ptr( dataMem .GetData(), dataMem .GetSize() ),
					   const_mem_ptr( indexMem.GetData(), indexMem.GetSize() ) ) )
	{
		return ( false );
	}
	FuBi::PersistContext persist( &reader, true, FuBi::XMODE_DEFAULT, &m_ErrorString );
	persist.SetVersion( m_SaveInfo.m_ProductVersion );

// Shut down.

	// force shutdown of world streaming while we load
	m_StoppedLoadingThread = gSiegeEngine.IsLoadingThreadRunning();
	gSiegeEngine.StopLoadingThread();

	// shut down the world
	gWorld.Shutdown( SHUTDOWN_FOR_RELOAD_GAME, false );
	gUIPartyManager.DeconstructUIParty();

// Restart.

	// $ important: persist skrit engine before the world, and before
	//   reinitializing the contentdb. contentdb uses skrits but they are
	//   effectively "const" and should not be persisted.

	// persist skrit engine
	{
		RatioSample ratioSample( "", 0, 0.023 );
		persist.EnterBlock( "SkritEngine" );
		gSkritEngine.Xfer( persist );
		persist.LeaveBlock();
	}

	// start up the world
	{
		RatioSample ratioSample( "init_world", 0, 0.019 );
		gWorld.Init();
	}

	// start up the server
	{
		RatioSample ratioSample( "init_server", 0, 0.019 );
		gWorld.InitServer( false );
	}

	gSiegeEngine.Shutdown();

	// lock it out
	gGoDb.SetLockedForSaveLoad();

// Xfer.

	// xfer common stuff
	{
		RatioSample ratioSample( "", 0, 0.60 );
		XferCommon( persist );
	}

	// finish off the godb (load lodfi etc.)
	{
		RatioSample ratioSample( "" );
		gGoDb.PostLoad();
	}

	// only continue if everything is ok
	if ( !persist.IsInFatalError() )
	{
		// load up portraits
		Go* party = gServer.GetScreenParty();
		if ( party != NULL )
		{
			GopColl::iterator i, ibegin = party->GetChildBegin(), iend = party->GetChildEnd();
			for ( i = ibegin ; i != iend ; ++i )
			{
				DWORD portraitTex = CreatePortraitTextureByIndex( ibegin - i );
				if ( portraitTex != 0 )
				{
					(*i)->GetActor()->SetPortraitTexture( portraitTex, false );
				}
			}
		}

		// rebuild ui
		gUI.GetUIGame().GetUIPartyManager()->ConstructUIParty();

		// xfer party gridboxes
		persist.EnterColl( "GridBoxes" );
		while ( persist.AdvanceCollIter() )
		{
			Goid goid;
			persist.Xfer( "goid", goid );
			GoHandle( goid )->GetInventory()->XferGridbox( persist );
		}
		persist.LeaveColl();

		// xfer whatever is left
		persist.EnterBlock( "misc" );
		persist.Xfer( "world_state", m_LoadedWorldState, WS_SP_INGAME );
		persist.LeaveBlock();

		// special: match sp/mp to current mode
		if ( ::IsMultiPlayer() && (m_LoadedWorldState == WS_SP_INGAME) )
		{
			m_LoadedWorldState = WS_MP_INGAME;
		}
		else if ( ::IsSinglePlayer() && (m_LoadedWorldState == WS_MP_INGAME) )
		{
			m_LoadedWorldState = WS_SP_INGAME;
		}

		// clean out any pending messages in the chat history that might have
		// been caused by shutting down the previous game.
		gUIGameConsole.ChatHistoryClear();

		// clean out any pending messages in the screen text history that might
		// be caused by shutting down the previous game.
		UIChatBox* gameConsole = (UIChatBox*)gUIShell.FindUIWindow( "console_box" );
		if ( gameConsole != NULL )
		{
			gameConsole->Clear();
		}

		// commit any pending commands to flush pending texture commits
		gSiegeLoadMgr.CommitAll();
	}

// Finish.

	// reset cursor to center now
	gTattooGame.ResetUserInputs();

	// print out error if any
	if ( persist.IsInFatalError() )
	{
		// sorry but we just really have to fatal here. there is nothing that
		// can be done.
		if ( persist.IsInWatsonError() )
		{
			gpwatsonf(( $MSG$ "Unable to load the game '%s', reasons:\n\n%s\n",
						::ToAnsi( m_LoadFileName ).c_str(), m_ErrorString.c_str() ));
		}
		else
		{
			gpfatalf(( $MSG$ "Unable to load the game '%s', reasons:\n\n%s\n",
					   ::ToAnsi( m_LoadFileName ).c_str(), m_ErrorString.c_str() ));
		}
	}
#	if !GP_RETAIL
	else if ( persist.HasSeriousErrors() )
	{
		gperror( "Serious errors occurred during load! Game is now officially unstable.\n" );
	}
#	endif // !GP_RETAIL

	// done
	return ( true );
}

bool GameLoader :: LoadWorldPartial( void )
{
	// open handles
	FileSys::AutoFileHandle indexFile, dataFile;
	FileSys::AutoMemHandle indexMem, dataMem;
	if (   !indexFile.Open( "load://world.xidx" ) || !indexMem.Map( indexFile )
		|| !dataFile .Open( "load://world.xdat" ) || !dataMem .Map( dataFile  ) )
	{
		return ( false );
	}

	// build reader
	FuBi::TreeBinaryReader reader;
	if ( !reader.Init( const_mem_ptr( dataMem .GetData(), dataMem .GetSize() ),
					   const_mem_ptr( indexMem.GetData(), indexMem.GetSize() ) ) )
	{
		return ( false );
	}
	FuBi::PersistContext persist( &reader );
	persist.SetVersion( m_SaveInfo.m_ProductVersion );

	// get some vars
	gpstring mapName, mpWorldName;
	SiegePos siegePos;

	// xfer in some stuff
	persist.EnterBlock( "m_pWorldTerrain" );
		persist.EnterBlock( "m_pMap" );
			persist.Xfer( "m_MapName", mapName );
			persist.Xfer( "m_MpWorldName", mpWorldName );
		persist.LeaveBlock();
		persist.EnterBlock( "SiegeEngine" );
			persist.EnterBlock( "m_pCamera" );
				persist.Xfer( "m_TargetPosition", siegePos );
			persist.LeaveBlock();
		persist.LeaveBlock();
	persist.LeaveBlock();

	// set the map
	gWorldMap.SSet( mapName, mpWorldName, RPC_TO_ALL );
	if ( !gWorldMap.IsInitialized() )
	{
		return ( false );
	}

	// get the bookmark
	CameraPosition cameraPos;
	gpstring teleport;
	teleport.assignf( "n:%0.2f,%0.2f,%0.2f,%s",
			siegePos.pos.x, siegePos.pos.y, siegePos.pos.z, siegePos.node.ToString().c_str() );
	if ( !gWorldMap.LoadBookmark( teleport, siegePos, cameraPos ) )
	{
		return ( false );
	}

	// assign it as starting position
	gServer.GetScreenPlayer()->SetStartingPosition( siegePos, cameraPos );

	// load the map now
	gWorldStateRequest( WS_LOAD_MAP );
	gWorldState.Update();

	// now let's get the retired scids
	typedef stdx::fast_vector <Scid> ScidColl;
	ScidColl retiredScids;
	persist.EnterBlock( "GoDb" );
		persist.XferVector( "m_RetiredScidColl", retiredScids );
	persist.LeaveBlock();

	// temporarily go to reloading state to pull in party (needed to avoid errors)	
	eWorldState oldState = gWorldState.GetCurrentState();

	gWorldStateRequest( WS_RELOADING );
	gWorldState.Update();

	// nuke 'em all
	{
		ScidColl::iterator i, ibegin = retiredScids.begin(), iend = retiredScids.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			Goid goid = gGoDb.FindGoidByScid( *i );
			if ( goid != GOID_INVALID )
			{
				gGoDb.SMarkForDeletion( goid, true, false );
			}
		}
		gGoDb.CommitAllRequests();
	}

	// kill it from ui
	gUIPartyManager.DeconstructUIParty();

	// nuke current party members (hero)
	Go* party = gServer.GetScreenParty();
	if ( party != NULL )
	{
		GopColl::iterator i, ibegin = party->GetChildBegin(), iend = party->GetChildEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gGoDb.SMarkGoAndChildrenForDeletion( (*i)->GetGoid() );
		}
		gGoDb.CommitAllRequests();
	}

	// load the party
	if ( !LoadParty( siegePos, true ) )
	{
		return ( false );
	}

	// tell ui to rebuild the party
	gUIPartyManager.ConstructUIParty();

	// switch back to old state
	if ( oldState == WS_LOADED_INTRO )
	{
		gUIIntro.SetAsActive( false );
		gWorldStateRequest( WS_WAIT_FOR_BEGIN );
	}
	else
	{
		gWorldStateRequest( oldState );
	}

	gWorldState.Update();

	// now update the camera to commit it into the tracking
	gUICamera.Update( 0 );

	// done
	return ( true );
}

bool GameLoader :: LoadParty( const SiegePos& where, bool fullLoad )
{
	// import all the party members we had
	if ( !ImportAllCharacters() )
	{
		return ( false );
	}

	// add party members
	GoidColl partyMembers;
	{
		Go* party = gServer.GetScreenParty();
		if ( party == NULL )
		{
			return ( false );
		}

		for ( int i = 0, count = GetCharacterCount() ; i != count ; ++i )
		{
			GoCloneReq cloneReq;
			cloneReq.m_Parent = party->GetGoid();
			cloneReq.SetStartingPos( where );
			Goid goid = gGoDb.SCloneGo( cloneReq, gGoDb.MakeImportCharacterFuelAddress( gServer.GetScreenPlayer()->GetId(), i ) );
			if ( goid != GOID_INVALID )
			{
				partyMembers.push_back( goid );
			}
		}
	}

	// arrange them so not on top of each other
	PContentDb::PlaceObjectsInArray( where, partyMembers, 1.5f );

	// party stats
	if ( fullLoad )
	{
		int heroIndex = 0;
		GopColl partyMembers = gServer.GetScreenParty()->GetChildren();		

		// gather some info
		FuelHandle partyFuel( "::load:party:party" );
		if ( partyFuel )
		{
			// hero status
			heroIndex = partyFuel->GetInt( "hero_index" );

			// party gold
			int gold = partyFuel->GetInt( "gold" );
			GoHandle hero( gServer.GetScreenPlayer()->GetHero() );
			if ( hero )
			{
				hero->GetInventory()->SetGold( gold );
			}
		}

		// always set the hero
		if ( (heroIndex >= 0) && (heroIndex < (int)partyMembers.size()) )
		{
			gServer.GetScreenPlayer()->SetHero( partyMembers[ heroIndex ]->GetGoid() );
		}
	}

	// done
	return ( true );
}

bool GameLoader :: FillInfo( SaveInfo& info, bool getThumbnail )
{
	bool success = info.LoadFromFuel(
			FuelHandle( "::load:info" ),
			getThumbnail,
			TestOptions( OPTION_SINGLEPLAYER ),
			TestOptions( OPTION_MULTIPLAYER ) );
	if ( success )
	{
		info.ConvertForScreen();
	}
	return ( success );
}

int GameLoader :: GetSaveGameInfos( CharacterInfoColl* ccoll, SaveInfoColl* scoll, eOptions options, const char* fileSpec, bool forRead, bool getPics )
{
	int oldSize = ccoll ? ccoll->size() : (scoll ? scoll->size() : 0);

	if ( gAppModule.GetShiftKey() )
	{
		options |= OPTION_SINGLEPLAYER | OPTION_MULTIPLAYER;
	}

	FileSys::FileFinder finder( fileSpec );
	gpstring name;
	while ( finder.Next( name ) )
	{
		// ignore read-only files if we're writing
		if ( !forRead && (finder.Get().dwFileAttributes & FILE_ATTRIBUTE_READONLY) )
		{
			continue;
		}

		// make sure it has a valid extension (wildcards may pick up .dssave.bak etc.)
		gpwstring namew = ::ToUnicode( name );
		if ( !HasValidSaveGameExtension( namew ) )
		{
			continue;
		}

		// we just looking for file existence?
		if ( (scoll == NULL) && (ccoll == NULL) )
		{
			return ( 1 );
		}

		// add new entry
		std::auto_ptr <SaveInfo> info( new SaveInfo );

		// get filename
		info->m_FileName = namew;

		// adjust bits
		if ( options & OPTION_QUICK_SAVE )
		{
			info->m_IsQuickSave = true;
		}
		else if ( options & OPTION_AUTO_SAVE )
		{
			info->m_IsAutoSave = true;
		}

		// extract extra info and add if successful
		GameLoader loader( options );
		if ( loader.BeginLoad( info->m_FileName ) )
		{
			if ( ccoll != NULL )
			{
				loader.GetCharacterInfos( *ccoll, getPics );
			}
			if ( (scoll != NULL) && loader.FillInfo( *info, getPics ) )
			{
				scoll->push_back( info.release() );
			}
		}
	}

	if ( !ccoll && !scoll )
	{
		return 0;
	}

	return ( ccoll ? (ccoll->size() - oldSize) : (scoll->size() - oldSize) );
}

int GameLoader :: GetSaveGameInfos( CharacterInfoColl* ccoll, SaveInfoColl* scoll, eOptions options, bool forRead, bool getPics )
{
	if ( ccoll != NULL )
	{
		ccoll->clear();
	}
	if ( scoll != NULL )
	{
		scoll->clear();
	}

	if ( gAppModule.GetShiftKey() )
	{
		options |= OPTION_SINGLEPLAYER | OPTION_MULTIPLAYER;
	}

	eOptions workingOptions = options & NOT( OPTION_QUICK_SAVE | OPTION_AUTO_SAVE );
	gpstring fileSpec = ::ToAnsi( MakeSaveGameFileName( L"*", workingOptions, true ) );
	int size = GetSaveGameInfos( ccoll, scoll, workingOptions, fileSpec, forRead, getPics );
	if ( ((ccoll == NULL) && (scoll == NULL)) && (size > 1) )
	{
		return ( 1 );
	}

	if ( options & OPTION_QUICK_SAVE )
	{
		workingOptions = options & NOT( OPTION_AUTO_SAVE );
		fileSpec = ::ToAnsi( MakeSaveGameFileName( NULL, workingOptions, true ) );
		size += GetSaveGameInfos( ccoll, scoll, workingOptions, fileSpec, forRead, getPics );
		if ( ((ccoll == NULL) && (scoll == NULL)) && (size > 1) )
		{
			return ( 1 );
		}
	}

	if ( options & OPTION_AUTO_SAVE )
	{
		workingOptions = options & NOT( OPTION_QUICK_SAVE );
		fileSpec = ::ToAnsi( MakeSaveGameFileName( NULL, workingOptions, true ) );
		size += GetSaveGameInfos( ccoll, scoll, workingOptions, fileSpec, forRead, getPics );
		if ( ((ccoll == NULL) && (scoll == NULL)) && (size > 1) )
		{
			return ( 1 );
		}
	}

	return ( size );
}

int GameLoader :: GetSaveGameInfos( SaveInfoColl* scoll, eOptions options, const char* fileSpec, bool forRead, bool getPics )
{
	return ( GetSaveGameInfos( NULL, scoll, options, fileSpec, forRead, getPics ) );
}

int GameLoader :: GetSaveGameInfos( SaveInfoColl* scoll, eOptions options, bool forRead, bool getPics )
{
	return ( GetSaveGameInfos( NULL, scoll, options, forRead, getPics ) );
}

int GameLoader :: GetSaveGameInfos( CharacterInfoColl* ccoll, eOptions options, bool forRead, bool getPics )
{
	return ( GetSaveGameInfos( ccoll, NULL, options, forRead, getPics ) );
}

int GameLoader :: GetSaveGameInfos( eOptions options, bool forRead )
{
	return ( GetSaveGameInfos( NULL, NULL, options, forRead, false ) );
}

bool GameLoader :: GetCharacterInfos( CharacterInfoColl& coll, bool getPics )
{
	gpassert( IsLoading() );

	// get all char names
	FuelHandle party( "::load:party:members" );
	if ( party )
	{
		FuelHandleList members = party->ListChildBlocks();
		FuelHandleList::const_iterator i, ibegin = members.begin(), iend = members.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// get child
			FuelHandle child( (*i)->GetChildBlock( "member" ) );
			if ( child )
			{
				// first check for pack mules, and skip them
				if ( child->GetBool( "pack_only", false ) )
				{
					continue;
				}

				// add new one
				CharacterInfo& info = **coll.push_back( new CharacterInfo );

				// configure easy stuff
				info.m_ScreenName = child->GetWString( "screen_name" );
				info.m_ActorClass = child->GetWString( "actor_class" );
				info.m_FileName   = m_OriginalName;
				info.m_SaveTime   = m_TimeStamp;
				::FromString( (*i)->GetName(), info.m_Index );
				if ( getPics )
				{
					info.m_Portrait = CreatePortraitImageByIndex( info.m_Index );

					// there may not be a portrait if we're supposed to use the
					// actor icon instead...
					if ( info.m_Portrait == 0 )
					{
						AutoRefTemplate autoGodt( child->GetString( "template_name" ) );
						const GoDataTemplate* godt = autoGodt.GetDataTemplate();
						if ( godt != NULL )
						{
							const GoDataComponent* godc = godt->FindComponentByName( "actor" );
							if ( godc != NULL )
							{
								const FuBi::Record* record = godc->GetRecord();
								gpassert( record != NULL );
								gpstring icon;
								gpverify( record->Get( "portrait_icon", icon ) );

								gpstring texture;
								if (   gNamingKey.BuildIMGLocation( icon, texture )
									&& FileSys::DoesResourceFileExist( texture ) )
								{
									info.m_Portrait = RapiMemImage::CreateMemImage( texture, false, false );
								}
							}
						}
					}
				}

				// ok now extract skills
				FuelHandle skills( child->GetChildBlock( "skills" ) );
				if ( skills )
				{
					gpstring name;

					const SkillColl& baseSkills = gRules.GetBaseSkills();
					SkillColl::const_iterator j, jbegin = baseSkills.begin(), jend = baseSkills.end();
					for ( j = jbegin ; j != jend ; ++j )
					{
						// get name and new skill info entry
						name = j->GetName();
						CharacterInfo::SkillInfo& skill = info.m_Skills[ name ];
						skill.m_ScreenName = j->GetScreenName();

						// extract the level from the fuel block
						std::replace( name.begin_split(), name.end_split(), ' ', '_' );
						skill.m_Level = skills->GetFloat( name + "_level" );
					}
				}
			}
		}
	}

	// never really fails i guess
	return ( true );
}

int GameLoader :: GetCharacterCount( void )
{
	int count = 0;

	FuelHandle party( "::load:party:members" );
	if ( party )
	{
		count = party->GetChildCount();
	}

	return ( count );
}

FuelHandle GameLoader :: GetCharacterByIndex( int index )
{
	gpassert( IsLoading() );

	return ( FuelHandle( "::load:party:members:" + ToString( index ) + ":member" ) );
}

bool GameLoader :: ImportCharacterByIndex( int index, bool setAsPrimary )
{
	bool success = false;

	if ( index == -1 )
	{
		for ( int i = 0, count = GetCharacterCount() ; i != count ; ++i )
		{
			if ( ImportCharacterByIndex( i, i == 0 ) )
			{
				success = true;
			}
		}
	}
	else
	{
		FuelHandle character( GetCharacterByIndex( index ) );
		if ( character )
		{
			gpstring buffer;
			FileSys::StringWriter writer( buffer );

			Player* player = gServer.GetScreenPlayer();
			if ( (player != NULL) && character->Write( writer ) )
			{
				if ( setAsPrimary )
				{
					// import character
					player->RSImportCharacter( buffer, 0 );
					player->RSSetHeroCloneSourceTemplate( GoDb::MakeImportCharacterFuelAddress() );

					// import portrait
					// $$$ future: pass texture over network if we ever want to
					//     have multiplayer clients see each other's portraits, say
					//     in staging area chat.
					player->SetHeroPortrait( CreatePortraitTextureByIndex( index ), false );
				}
				else
				{
					gGoDb.ImportCharacter( player->GetId(), index, buffer );
				}

				success = true;
			}
		}

		// Let's xfer the stash items
		FuelHandle stashFuel( "::load:party:stash" );
		if ( stashFuel )
		{
			gpstring buffer;
			FileSys::StringWriter writer( buffer );

			Player* player = gServer.GetScreenPlayer();
			if ( (player != NULL) && stashFuel->Write( writer ) )
			{
				player->RSImportStash( buffer );
				player->RSSetStashCloneSourceTemplate( GoDb::MakeImportStashFuelAddress() );
			}
		}		
	}

	return ( success );
}

DWORD GameLoader :: CreatePortraitTextureByIndex( int index )
{
	DWORD portraitTex = 0;

	// build image
	RapiMemImage* portrait = CreatePortraitImageByIndex( index );
	if ( portrait != NULL )
	{
		// $ don't dither - it will degrade over time with repeated save/load
		portraitTex = gDefaultRapi.CreateAlgorithmicTexture( "portrait", portrait, false, 0, TEX_LOCKED, false, false );
	}

	// done
	return ( portraitTex );
}

RapiMemImage* GameLoader :: CreatePortraitImageByIndex( int index )
{
	gpassert( IsLoading() );

	// build filename (same as index into party)
	gpstring portraitFileName = "load://portrait-" + ToString( index ) + ".bmp";

	// build image
	return ( RapiMemImage::CreateMemImage( portraitFileName, false, false ) );
}

void GameLoader :: reset( void )
{
	m_OriginalName.clear();
	m_LoadFileName.clear();
	m_ErrorString .clear();

	// uninstall filemgr
	if ( m_FileMgr != NULL )
	{
		((FileSys::MasterFileMgr*)FileSys::IFileMgr::GetMasterFileMgr())->RemoveFileMgr( m_FileMgr );
		Delete( m_FileMgr );
	}

	// kill fuel if any
	if ( m_Db != NULL )
	{
		gFuelSys.Remove( m_Db );
		m_Db = NULL;
	}

	// safe to unlock now (do it just in case we aborted an earlier op)
	if ( GoDb::DoesSingletonExist() && gGoDb.IsLockedForSaveLoad() )
	{
		gGoDb.SetLockedForSaveLoad( false );
	}

	// reset other stuff
	m_LoadedWorldState = WS_SP_INGAME;

	// restart loader if needed
	if ( m_StoppedLoadingThread )
	{
		gSiegeEngine.StartLoadingThread();
	}
}

//////////////////////////////////////////////////////////////////////////////

bool CopyRenameCharacterFromSave( const wchar_t* saveFileName, const wchar_t* newScreenName )
{
	// Validate parameters	
	gpassert( FileSys::IsValidFileName( saveFileName ) );
	gpassert( FileSys::IsValidFileName( newScreenName ) );

	bool success = true;

	// copy the tank file if there is one
	gpwstring dspartyName;
	dspartyName.assignf( L"%s.dsparty", newScreenName );

	gpstring oldFilename = ::ToAnsi( GameSaver::MakeSaveGameFileName( saveFileName, GameXfer::OPTION_TANK ) );	
	gpstring newFilename = ::ToAnsi( GameSaver::MakeSaveGameFileName( dspartyName,  GameXfer::OPTION_TANK ) );	
	
	FileSys::TankFileMgr* tankFileMgr = NULL;	

	if ( FileSys::DoesFileExist( oldFilename ) )
	{		
		// build tank mgr
		tankFileMgr = new FileSys::TankFileMgr;
		if ( !tankFileMgr->OpenTank( oldFilename, true, true ) )
		{
			success = false;
		}
	}	
	else
	{
		success = false;
	}

	Tank::Builder * tankBuilder = NULL;
	if ( success )
	{
		// construct a tank builder
		tankBuilder = new Tank::Builder;
		
		// get options
		Tank::Builder::eOptions tankOptions =
			  Tank::Builder::OPTION_PE_FORMAT
			| Tank::Builder::OPTION_USE_TEMP_FILE;

		// set options on how to build
		tankBuilder->SetOptions( tankOptions );

		// set its header
		tankBuilder->SetTankProductVersion( SysInfo::GetExeFileVersion() );
		tankBuilder->SetTankMinimumVersion( SysInfo::GetExeFileVersion() );
		tankBuilder->AddTankFlags( Tank::TANKFLAG_ALLOW_MULTIPLAYER_XFER );
		tankBuilder->SetCreatorId( CREATOR_GPG );
		tankBuilder->SetTitleText( dspartyName );
		tankBuilder->SetAuthorText( gServer.GetScreenPlayer()->GetName() );

		// create it
		success = tankBuilder->CreateTankAlways( ::ToUnicode( newFilename ) );
	}		

	// mount file system and build fueldb if success
	if ( success )
	{
		// add it as a driver
		((FileSys::MasterFileMgr*)FileSys::IFileMgr::GetMasterFileMgr())->AddDriver( tankFileMgr, false, "copy" );

		// build fuel db
		TextFuelDB * textDb = gFuelSys.AddTextDb( "copy" );
		textDb->Init( FuelDB::OPTION_READ | FuelDB::OPTION_JIT_READ, "copy://", "" );
		
		// do the screen name replacement		
		FuelHandle party( "::copy:party:members" );		
		if ( party )
		{
			FuelHandleList members = party->ListChildBlocks();
			FuelHandleList::const_iterator i, ibegin = members.begin(), iend = members.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				// get child
				FuelHandle child( (*i)->GetChildBlock( "member" ) );
				if ( child )
				{					
					child->Set( "screen_name", ::ToAnsi( newScreenName ) );
				}
			}
		}	

		// do the file name replacement
		FuelHandle info( "::copy:info" );
		if ( info )
		{
			info->Set( "file_name", newFilename );			
			info->Set( "hero_name", ::ToAnsi( newScreenName ) );
		}

		// write new fuel information into data buffers
		gpstring infoBuffer, partyBuffer;
		FileSys::StringWriter infoWriter( infoBuffer ), partyWriter( partyBuffer );
		FuelHandle infoHandle( "::copy:info" ), partyHandle( "::copy:party" );
		infoHandle->Write( infoWriter );
		partyHandle->Write( partyWriter );

		// add our needed files into the new tank builder
		{
			Tank::Builder::FileSpec fileSpecInfo( "info.gas", TankConstants::DATAFORMAT_ZLIB );
			if ( !tankBuilder->AddFile( fileSpecInfo, const_mem_ptr( infoBuffer.c_str(), infoBuffer.length() ) ) )
			{
				success = false;
			}

			Tank::Builder::FileSpec fileSpecParty( "party.gas", TankConstants::DATAFORMAT_ZLIB );
			if ( !tankBuilder->AddFile( fileSpecParty, const_mem_ptr( partyBuffer.c_str(), partyBuffer.length() ) ) )
			{
				success = false;
			}

			// add the portrait as well								
			Tank::Builder::FileSpec fileSpec( "portrait-0.bmp", TankConstants::DATAFORMAT_ZLIB );
			fileSpec.m_MinCompressionRatio = 0;

			FileSys::AutoFileHandle portraitFile( "copy://portrait-0.bmp" );
			FileSys::AutoMemHandle portraitHandle;
			if ( !portraitHandle.Map( portraitFile ) )
			{
				success = false;
			}			
			else if ( !tankBuilder->AddFile( fileSpec, const_mem_ptr( portraitHandle.GetData(), portraitHandle.GetSize() ) ) )
			{
				success = false;
			}
		}	
		
		// all done!  let's commit our changes - only if we've been successful so far
		if ( success && !tankBuilder->CommitTank() )
		{ 
			success = false;
		}

		// Cleanup
		gFuelSys.Remove( textDb );		
	}

	// Cleanup the rest
	if ( tankFileMgr != NULL )
	{
		((FileSys::MasterFileMgr*)FileSys::IFileMgr::GetMasterFileMgr())->RemoveFileMgr( tankFileMgr );
		Delete( tankFileMgr );
	}

	if ( tankBuilder != NULL )
	{
		// Deleting the tank builder will abort the current tank if we were unsuccessful
		Delete( tankBuilder );
	}

	return success;
}
