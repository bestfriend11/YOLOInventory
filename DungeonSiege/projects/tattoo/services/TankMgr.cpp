//////////////////////////////////////////////////////////////////////////////
//
// File     :  TankMgr.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Services.h"
#include "TankMgr.h"

#include "AppModule.h"
#include "Config.h"
#include "ContentDb.h"
#include "FileSysUtils.h"
#include "Fuel.h"
#include "FuBiPersistImpl.h"
#include "FuelDb.h"
#include "Gps_Manager.h"
#include "MasterFileMgr.h"
#include "PathFileMgr.h"
#include "Services.h"
#include "StdHelp.h"
#include "StringTool.h"
#include "TankFileMgr.h"
#include "TattooVersion.h"

// $ special include for all our tools to be able to use
#include "WorldConfig.inc"

//////////////////////////////////////////////////////////////////////////////
// helper functions

gpversion GetDungeonSiegeVersion( void )
{
	static bool s_Init = false;
	static gpversion s_Ver;

	if ( !s_Init )
	{
		s_Init = true;

		s_Ver = SysInfo::GetExeFileVersion();
		if ( s_Ver.IsZero() )
		{
			// fall back to .h file
			// $ THIS IS A SPECIAL CASE, DO NOT COPY PASTE THIS CODE ELSEWHERE! -sb
			::FromString( DS1_VERSION_TEXT, s_Ver );
		}
	}

	return ( s_Ver );
}

gpstring GetDungeonSiegeVersionText( gpversion::eMode mode )
{
	return ( ::ToString( GetDungeonSiegeVersion(), mode ) );
}

// string = 'config/dev' or (retail) 'config/dev/retail', key = 0xFE

const char* GetDungeonSiegeSpecialPath( void )
{
	static bool s_Init = false;
	static char s_Str[] = "\x9D\x91\x90\x98\x97\x99\xD1\x9A\x9B\x88" GPRETAIL_ONLY( "\xD1\x8C\x9B\x8A\x9F\x97\x92" );

	if ( !s_Init )
	{
		s_Init = true;

		for ( char* i = ARRAY_END( s_Str ) - 1 ; i != s_Str ; --i )
		{
			i[ -1 ] = (char)((unsigned char)(i[ -1 ]) ^ (unsigned char)0xFE);
		}
	}

	return ( s_Str );
}

bool HasDungeonSiegeSpecialSkipCdCheckFile( void )
{
	gpstring spec;
	spec = GetDungeonSiegeSpecialPath();
	spec += '/';
	spec += '*';

	// iterate through these files and check their hashes vs. the hashed name
	// "special_gpg_file_skip_cd_check.xxx".
	FileSys::AutoFindHandle find( spec, FileSys::FINDFILTER_FILES );
	gpstring filename;
	while ( find.GetNext( filename ) )
	{
		// hash it
		::TeaEncrypt( filename, 0x0DB6B126, 0x55FB3848, 0xB9ECAC26, 0xE04832BF );
		UINT32 hash = ::GetAdler32( filename.c_str(), filename.length() + 1 )
					+ ::GetCRC32  ( 0x1728F133, filename.c_str(), filename.length() + 1 );
#		if GP_RETAIL
		if ( hash == 0x5508cd63 )
#		else
		if ( hash == 0x3d201f62 )
#		endif
		{
			// found!
			return ( true );
		}
	}

	// not found!
	return ( false );
}

bool HasDungeonSiegeSpecialEnableDsrMpFile( void )
{
	// iterate through these files and check their hashes vs. the hashed name
	// "special_gpg_file_enable_dsr_multiplayer.xxx".
	FileSys::AutoFindHandle find( gpstring( GetDungeonSiegeSpecialPath() ) + "/*", FileSys::FINDFILTER_FILES );
	gpstring filename;
	while ( find.GetNext( filename ) )
	{
		// hash it
		UINT32 hash = ::GetCRC32( 0x8948FEEB, filename.c_str(), filename.length() + 1 )
					+ ::GetAdler32( filename.c_str(), filename.length() + 1 );
		if ( hash == 0x944277bc )
		{
			// found!
			return ( true );
		}
	}

	// not found!
	return ( false );
}

bool HasAnyDungeonSiegeSpecialFile( void )
{
	return ( HasDungeonSiegeSpecialSkipCdCheckFile() || HasDungeonSiegeSpecialEnableDsrMpFile() );
}

//////////////////////////////////////////////////////////////////////////////
// enum eTankType implementation

static const char* s_TankTypeStrings[] =
{
	"tank_none",
	"tank_resource",
	"tank_map",
	"tank_mod",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_TankTypeStrings ) == TANK_COUNT );

static stringtool::EnumStringConverter s_TankTypeConverter( s_TankTypeStrings, TANK_BEGIN, TANK_END );

const char* ToString( eTankType e )
	{  return ( s_TankTypeConverter.ToString( e ) );  }

bool FromString( const char* str, eTankType& e )
	{  return ( s_TankTypeConverter.FromString( str, rcast <int&> ( e ) ) );  }

static const char* s_TankTypePathVars[] =
{
	"",
	"%res_paths%",
	"%map_paths%",
	"%mod_paths%",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_TankTypePathVars ) == TANK_COUNT );

static stringtool::EnumStringConverter s_PathVarConverter( s_TankTypePathVars, TANK_BEGIN, TANK_END );

const char* ToPathVar( eTankType e )
	{  return ( s_PathVarConverter.ToString( e ) );  }

static const char* s_TankTypeExtensions[] =
{
	"",
	".dsres",
	".dsmap",
	".dsmod",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_TankTypeExtensions ) == TANK_COUNT );

static stringtool::EnumStringConverter s_ExtensionConverter( s_TankTypeExtensions, TANK_BEGIN, TANK_END );

const char* ToExtension( eTankType e )
	{  return ( s_ExtensionConverter.ToString( e ) );  }

FUBI_DECLARE_ENUM_TRAITS( eTankType, TANK_NONE );

//////////////////////////////////////////////////////////////////////////////
// enum eModFlags implementation

eModFlags DetectModFlags( FileSys::TankFileMgr* fileMgr )
{
	eModFlags flags = MOD_NONE;

	if ( fileMgr != NULL )
	{
		gpassert( fileMgr->IsTankOpen() );
		gpassert( fileMgr->IsEnabled() );

		// $$$ IMPORTANT: always flush certain fuel blocks, like the sounddb and
		//  other similar things that don't directly map onto any system in the
		//  game, yet are candidates for overriding from a map.

		// first get reload flags
		{
			for ( const char** i = ContentDb::SYNC_DIRS ; *i != NULL ; ++i )
			{
				if ( FileSys::DoesResourcePathExist( *i, fileMgr ) )
				{
					flags |= MOD_REQUIRES_CONTENTDB_RELOAD;
					break;
				}
			}
		}

		// if any of these are set, then it's a cause for mp sync violation
		if ( flags & MOD_REQUIRES_CONTENTDB_RELOAD )
		{
			flags |= MOD_AFFECTS_MP_VERSION;
		}

		// does it contain a naming key?
		FileSys::FindHandle fh = fileMgr->Find( "art/maps/*.nnk" );
		if ( fh )
		{
			if ( fileMgr->HasNext( fh ) )
			{
				flags |= MOD_REQUIRES_NNK_RELOAD;
			}
			fileMgr->Close( fh );
			fh = fh.Null;
		}

		// does it contain map info?
		fh = fileMgr->Find( "world/maps/*", FileSys::FINDFILTER_DIRECTORIES );
		if ( fh  )
		{
			if ( fileMgr->HasNext( fh ) )
			{
				flags |= MOD_INCLUDES_MAP_DATA;
			}
			fileMgr->Close( fh );
			fh = fh.Null;
		}
	}

	return ( flags );
}

//////////////////////////////////////////////////////////////////////////////
// struct TankInfo implementation

TankInfo :: TankInfo( void )
{
	m_TankType   = TANK_NONE;
	m_CreatorId  = 0;
	m_IndexCRC32 = 0;
	m_DataCRC32  = 0;

	::ZeroObject( m_GUID );
	::ZeroObject( m_UtcBuildTime );
}

void TankInfo :: SaveToFuel( FuelHandle fuel )
{
	FuBi::FuelWriter writer( fuel );
	FuBi::PersistContext persist( &writer );
	Xfer( persist );
}

void TankInfo :: LoadFromSystem( FileSys::TankFileMgr* fileMgr, eTankType type )
{
	if ( fileMgr != NULL )
	{
		const Tank::TankFile* file = &fileMgr->GetTankFile();
		const Tank::RawIndex::Header& header = file->GetHeader();
		gpassert( type != TANK_NONE );

		m_FileName     = FileSys::GetFileName( file->GetFileName() );
		m_TankType     = type;
		m_Version      = ::ToGpVersion( header.m_ProductVersion );
		m_CreatorId    = header.m_CreatorId;
		m_GUID         = header.m_GUID;
		m_IndexCRC32   = header.m_IndexCRC32;
		m_DataCRC32    = header.m_DataCRC32;
		m_UtcBuildTime = header.m_UtcBuildTime;
		m_Title        = header.m_TitleText;
		m_ModFlags     = DetectModFlags( fileMgr );
		m_MinVersion   = header.m_MinimumVersion;
	}
	else
	{
		gpassert( type == TANK_NONE );

		m_FileName = gConfig.ResolvePathVars( "%home%" );
		m_Title    = L"<Path>";
	}
}

bool TankInfo :: LoadFromFuel( FuelHandle fuel )
{
	FuBi::FuelReader reader( fuel );
	FuBi::PersistContext persist( &reader );
	return ( Xfer( persist ) );
}

bool TankInfo :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer      ( "file_name",      m_FileName     );
	persist.Xfer      ( "tank_type",      m_TankType     );
	persist.Xfer      ( "version",        m_Version      );
	persist.XferFourCc( "creator_id",     m_CreatorId    );
	persist.XferQuoted( "guid",           m_GUID         );
	persist.XferHex   ( "index_crc32",    m_IndexCRC32   );
	persist.XferHex   ( "data_crc32",     m_DataCRC32    );
	persist.Xfer      ( "utc_build_time", m_UtcBuildTime );

	return ( true );
}

FUBI_DECLARE_SELF_TRAITS( TankInfo );

//////////////////////////////////////////////////////////////////////////////
// struct TankManifest implementation

void TankManifest :: SaveToFuel( const char* key, FuelHandle fuel )
{
	FuBi::FuelWriter writer( fuel );
	FuBi::PersistContext persist( &writer );
	Xfer( key, persist );
}

void TankManifest :: LoadFromSystem( void )
{
	clear();
	gTankMgr.GetTankManifest( *this );
}

void TankManifest :: LoadFromSystem( FileSys::TankFileMgr* fileMgr, eTankType type )
{
	TankInfo& info = *push_back();
	info.LoadFromSystem( fileMgr, type );
}

bool TankManifest :: LoadFromFuel( const char* key, FuelHandle fuel )
{
	FuBi::FuelReader reader( fuel );
	FuBi::PersistContext persist( &reader );
	return ( Xfer( key, persist ) );
}

bool TankManifest :: Xfer( const char* key, FuBi::PersistContext& persist )
{
	return ( persist.XferVector( key, *this ) );
}

//////////////////////////////////////////////////////////////////////////////
// class TankMgr implementation

TankMgr :: TankMgr( void )
{
	// build owned systems
	m_MasterFileMgr = NULL;
#	if !GP_RETAIL
	m_MultiFileMgr = NULL;
#	endif // !GP_RETAIL

	// clear other vars
	deinit();

	// the editor needs virtual paging disabled, otherwise goofy MFC code
	// chokes on the exceptions.
	if ( Services::IsEditor() )
	{
		TankFileMgr::SetDefaultOptions( TankFileMgr::OPTION_NO_PAGING );
	}
}

TankMgr :: ~TankMgr( void )
{
	deinit();
}

static void StopAllSoundsNow( void )
{
	if ( GPGSound::SoundManager::DoesSingletonExist() )
	{
		gSoundManager.StopAllSounds( true, true );
	}
}

bool TankMgr :: Init( void )
{
	if ( !init( 0 ) && !init( 1 ) )
	{
#		if GP_RETAIL
		gperrorboxf(( $MSG$ "Some resource files haven't been copied to the "
							"hard disk (or I can't find them), which prevents "
							"%s from running.\n"
							"\n"
							"It's possible that some files have been "
							"accidentally deleted or corrupted, or a patch "
							"procedure has failed. Please reinstall from the "
							"CD to resolve this issue, or contact Microsoft "
							"technical support for further help.",
							::ToAnsi( gAppModule.GetAppName() ).c_str() ));
#		else // GP_RETAIL
		{
			ReportSys::AutoReport autoReport( &gErrorContext );
			gperror( "Unable to find core resources - did you set up your "
					 "HOME directory properly? Check these parameters to see "
					 "if anything is incorrect. Note that if any errors have "
					 "already come up regarding your paths, then those must "
					 "be fixed first, otherwise parts of this table may be "
					 "incorrect.\n\n" );
			gConfig.DumpPathVars( &gErrorContext );
		}
#		endif // GP_RETAIL

		// failure
		return ( false );
	}

	// success
	return ( true );
}

bool TankMgr :: init( int attempt )
{
// Setup.

	// clean out old stuff
	deinit();

	// setup default config vars
	InitializeWorldConfigPaths( Services::GetAppType(), attempt );

// Initialize FileSys.

	// build owned systems
	m_MasterFileMgr = new MasterFileMgr;
#	if !GP_RETAIL
	m_MultiFileMgr = new MultiFileMgr;
	m_MasterFileMgr->AddFileMgr( m_MultiFileMgr );
#	endif // !GP_RETAIL

	// when the file system gets rebuilt, the index we can't have any
	// sounds on the other thread still playing, so nuke them when that
	// happens. and the rebuild can happen at many places, so it's
	// easier to just handle it all with a simple callback.
	FileSys::SetRebuildCb( makeFunctor( StopAllSoundsNow ) );

	// find all the tanks
	findTanks( TANK_RESOURCE, true );
	findTanks( TANK_MOD );

	// after everything is done, update the index now to fill it (do it now
	// before journaling kicks in)
	m_MasterFileMgr->CheckIndexDirty();

	// build our dev-only file mgrs
	findBits();

// Check and report.

	// enable logging
#	if !GP_RETAIL
	if ( gConfig.GetBool( "file_journaling", false ) )
	{
		m_MasterFileMgr->SetOptions( MasterFileMgr::OPTION_LOG_OPENS );
	}
#	endif // !GP_RETAIL

	bool hasCoreResources = false;

	// this file contains paths we must search for to test for existence of our
	// core tank files and resources. if they don't exist, the system paths are
	// set up wrong and we'll crash anyway.
	FileSys::AutoFileHandle startupCatalog( "config/startup_verify.gpg" );
	if ( startupCatalog )
	{
		hasCoreResources = true;

		gpstring fileName;
		while ( startupCatalog.ReadLine( fileName ) )
		{
			stringtool::RemoveBorderingWhiteSpace( fileName );

			// skip empties and comments
			if ( !fileName.empty() && !((fileName[ 0 ] == '/') && (fileName[ 1 ] == '/')) )
			{
				FileSys::CleanupPath( fileName );
				if ( !FileSys::DoesResourcePathExist( fileName ) && !FileSys::DoesResourceFileExist( fileName ) )
				{
					hasCoreResources = false;
					break;
				}
			}

			fileName.clear();
		}
	}

	// success/failure determined by whether or not our core files are there
	return ( hasCoreResources );
}

void TankMgr :: deinit( void )
{
	// clear tank cache
	m_Tanks.clear();

	// clear map state
	m_MapsScanned    = false;
	m_CurrentMapInfo = NULL;
	m_CurrentMapTank = NULL;

	// delete owners
	Delete( m_MasterFileMgr );
#	if !GP_RETAIL
	Delete( m_MultiFileMgr  );
#	endif // !GP_RETAIL
}

void TankMgr :: GetTankManifest( TankManifest& manifest )
{
	bool hadTanks = false;

	TankFileMgrColl::const_iterator i, ibegin = m_Tanks.begin(), iend = m_Tanks.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( (i->m_FileMgr != NULL) && i->m_FileMgr->IsEnabled() )
		{
			manifest.LoadFromSystem( i->m_FileMgr, i->m_Type );
			hadTanks = true;
		}
	}

	// special case if no tanks at all - we're running from bits
	if ( !hadTanks )
	{
		manifest.LoadFromSystem( NULL, TANK_NONE );
	}
}

gpversion TankMgr :: GetNewestMinVersion( void )
{
	gpversion version;

	TankFileMgrColl::const_iterator i, ibegin = m_Tanks.begin(), iend = m_Tanks.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( (i->m_FileMgr != NULL) && i->m_FileMgr->IsEnabled() )
		{
			gpversion v = i->m_FileMgr->GetTankMinimumVersion();
			if ( version < v )
			{
				version = v;
			}
		}
	}

	return ( version );
}

bool TankMgr :: HasAnyTanks( void ) const
{
	TankFileMgrColl::const_iterator i, ibegin = m_Tanks.begin(), iend = m_Tanks.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->m_FileMgr != NULL )
		{
			return ( true );
		}
	}
	return ( false );
}

int TankMgr :: GetMapInfos( const char* fuelRoot, MapInfoColl& coll )
{
	coll.clear();

	findMaps( fuelRoot, NULL, &coll );

	return ( coll.size() );
}

int TankMgr :: GetMapInfos( MapInfoColl& coll )
{
	findMaps();

	coll.clear();

	TankFileMgrColl::const_iterator i, ibegin = m_Tanks.begin(), iend = m_Tanks.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		coll.insert( coll.end(), i->m_MapInfoColl.begin(), i->m_MapInfoColl.end() );
	}

	return ( coll.size() );
}

bool TankMgr :: GetMapInfo( const char* fuelRoot, const char* mapName, MapInfo& info )
{
	MapInfoColl coll;
	GetMapInfos( fuelRoot, coll );

	for ( MapInfoColl::iterator i = coll.begin() ; i != coll.end() ; ++i )
	{
		if ( i->m_InternalName.same_no_case( mapName ) )
		{
			info = *i;
			return ( true );
		}
	}

	return ( false );
}

bool TankMgr :: GetMapInfo( const char* mapName, MapInfo& info )
{
	MapInfoColl coll;
	GetMapInfos( coll );

	for ( MapInfoColl::iterator i = coll.begin() ; i != coll.end() ; ++i )
	{
		if ( i->m_InternalName.same_no_case( mapName ) )
		{
			info = *i;
			return ( true );
		}
	}

	return ( false );
}

void TankMgr :: ResetMapInfos( void )
{
	if ( !m_MapsScanned )
	{
		return;
	}

	ClearMap();

	for ( TankFileMgrColl::iterator i = m_Tanks.begin() ; i != m_Tanks.end() ; )
	{
		if ( (i->m_Type == TANK_MAP) || !i->m_MapInfoColl.empty() )
		{
			if ( i->m_FileMgr != NULL )
			{
				GetGlobalFileMgr()->RemoveFileMgr( i->m_FileMgr );
				Delete( i->m_FileMgr );
			}
			i = m_Tanks.erase( i );
		}
		else
		{
			++i;
		}
	}

	m_MapsScanned = false;
}

bool TankMgr :: SetMap( const gpstring& mapName )
{
	// $ early bailout if already mounted
	if ( IsMapSet( mapName ) )
	{
		return ( true );
	}

	// unmount whatever was there before
	ClearMap();

	// make sure cache filled
	findMaps();

	// find it in the cache
	TankFileMgrColl::iterator i, ibegin = m_Tanks.begin(), iend = m_Tanks.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		MapInfoColl::iterator j, jbegin = i->m_MapInfoColl.begin(), jend = i->m_MapInfoColl.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			if ( mapName.same_no_case( j->m_InternalName ) )
			{
				m_CurrentMapInfo = &*j;
				m_CurrentMapTank = &*i;
				goto done;
			}
		}
	}
done:

	// mount the tank if there is one (if cdimage there won't be)
	if ( (m_CurrentMapInfo != NULL) && (m_CurrentMapTank->m_FileMgr != NULL) )
	{
		// enable as appropriate - note that this automatically checks the
		// index upon reenabling the filemgr (tankfilemgr does it inside Enable)
		if ( !GetGlobalFileMgr()->EnableFileMgr( m_CurrentMapTank->m_FileMgr ) )
		{
			ClearMap();
			return ( false );
		}

		// $$$$ DEAL WITH $required FILE!!!!
	}

	// successful if valid iter
	return ( m_CurrentMapInfo != NULL );
}

bool TankMgr :: HasMap( const char* mapName )
{
	gpassert( mapName != NULL );
	findMaps();

	// note that we are searching in priority order so the higher priority tanks
	// get precedence to have them be the map source.

	TankFileMgrColl::const_iterator i, ibegin = m_Tanks.begin(), iend = m_Tanks.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		MapInfoColl::const_iterator j, jbegin = i->m_MapInfoColl.begin(), jend = i->m_MapInfoColl.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			if ( j->m_InternalName.same_no_case( mapName ) )
			{
				return ( true );
			}
		}
	}

	return ( false );
}

bool TankMgr :: HasAnyMaps( void )
{
	MapInfoColl coll;
	return ( GetMapInfos( coll ) > 0 );
}

bool TankMgr :: IsMapSet( const char* mapName ) const
{
	return ( IsMapSet() && m_CurrentMapInfo->m_InternalName.same_no_case( mapName ) );
}

bool TankMgr :: IsMapSet( void ) const
{
	return ( m_CurrentMapInfo != NULL );
}

void TankMgr :: ClearMap( void )
{
	// only bother if something mounted
	if ( IsMapSet() )
	{
		if ( m_CurrentMapTank->m_FileMgr != NULL )
		{
			m_CurrentMapTank->m_FileMgr->Disable();
		}
		m_CurrentMapTank = NULL;
		m_CurrentMapInfo = NULL;

		// file system has changed, nuke the maps dir
		{
			FastFuelHandle maps( "world:maps" );
			if ( maps )
			{
				maps.Unload();
			}
		}
	}
}

MapInfo& TankMgr :: GetMapInfo( void ) const
{
	gpassert( IsMapSet() );
	return ( *m_CurrentMapInfo );
}

void TankMgr :: findTanks( eTankType type, bool enable )
{
	// get some specs
	gpwstring pathVar       = gConfig.ResolvePathVars( ::ToUnicode( ::ToPathVar( type ) ), Config::PVM_APPEND );
	gpwstring fileSpec      = L"*" + ::ToUnicode( ToExtension( type ) );
	bool      ignoreVersion = gConfig.GetBool( "tank_ignore_version", false );
	bool      verifyData    = gConfig.GetBool( "verifydata", false );

	// find files from those specs
	std::list <gpwstring> foundFiles;
	FileSys::FindFilesFromSpec( pathVar, foundFiles, true, fileSpec );

	// track new tanks so we can mount them
	typedef std::list <TankEntry*> TankList;
	TankList foundTanks;

	// working mgr
	std::auto_ptr <TankFileMgr> tankFileMgr( new TankFileMgr );

	// now process each that we found
	std::list <gpwstring>::iterator i, ibegin = foundFiles.begin(), iend = foundFiles.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		gpstring fileName( ::ToAnsi( *i ) );

		// note: with many tanks, the index crc check will eat up much memory
		// that is not returned, so defer that until a tank is mounted for all
		// but the resources.
		if ( tankFileMgr->OpenTank( fileName, enable ) )
		{
			// make sure that we meet minimum version requirements
			if ( tankFileMgr->CanUseTank( ::GetDungeonSiegeVersion() ) || ignoreVersion )
			{
				// make sure it's not already there
				if ( !hasTank( tankFileMgr->GetTankGuid() ) )
				{
					// optionally verify
					if ( verifyData )
					{
						if ( tankFileMgr->VerifyAll() )
						{
							gpmessageboxf(( "Resource verification SUCCEEDED! Resource CRC's check out a-ok!\n\nFile = '%s'\n", fileName.c_str() ));
						}
						else
						{
							gperrorboxf(( "Resource verification FAILED! Resource CRC mismatch, file may be corrupt!\n\nFile = '%s'\n", fileName.c_str() ));
						}
					}

					// the editor cannot load mod tanks with map info or it will
					// get all confused!!
					eModFlags modFlags = DetectModFlags( tankFileMgr.get() );
					if ( Services::IsEditor() && (modFlags & MOD_INCLUDES_MAP_DATA) )
					{
						gpgenericf(( "Tank file '%s' includes map data, skipping...\n", fileName.c_str() ));
						continue;
					}

					// now figure out where in the collection it should
					// go (in sorted priority order)
					TankFileMgrColl::iterator j, jbegin = m_Tanks.begin(), jend = m_Tanks.end();
					for ( j = jbegin ; j != jend ; ++j )
					{
						// if it's newer
						if ( (j->m_FileMgr != NULL) && tankFileMgr->ShouldOverride( *j->m_FileMgr ) )
						{
							break;
						}
					}

					// insert and configure
					j = m_Tanks.insert( j, TankEntry() );
					j->m_Type = type;
					j->m_ModFlags = modFlags;
					j->m_FileMgr = tankFileMgr.release();

					// fill out required tank list
					FileSys::FileHandle requiredFile = tankFileMgr->Open( "$required" );
					if ( requiredFile )
					{
						j->m_HasRequiredFile = true;

						// read each entry in the file
						gpstring fileName;
						while ( tankFileMgr->ReadLine( requiredFile, fileName ) )
						{
							stringtool::RemoveBorderingWhiteSpace( fileName );
							FileSys::CleanupPath( fileName );
							fileName.to_lower();

							// skip empties and comments
							if ( !fileName.empty() && !((fileName[ 0 ] == '/') && (fileName[ 1 ] == '/')) )
							{
								j->m_RequiredTanks.insert( fileName );
							}
						}

						// was this on a dsres? well just turn it into a dsmod.
						// this is necessary because 1.0 did not know about
						// dsmods, so we need to release a couple maps pre-1.1
						// using dsres for the mod part of the content. putting
						// the $required in there will say "this is really a
						// dsmod file".
						if ( j->m_Type == TANK_RESOURCE )
						{
							// re-cast to mod and disable for now
							j->m_Type = TANK_MOD;
							j->m_ModFlags = DetectModFlags( tankFileMgr.get() );
							j->m_FileMgr->Enable( enable );
						}

						// release!
						tankFileMgr->Close( requiredFile );
					}

					// map tank? scan it
					if ( j->m_Type == TANK_MAP )
					{
						// install temporarily into master file mgr
						m_MasterFileMgr->AddDriver( j->m_FileMgr, false, "maps_query" );

						// find all maps in that tank
						findMaps( "maps_query://world/maps", &*j, NULL );

						// uninstall driver
						m_MasterFileMgr->RemoveFileMgr( j->m_FileMgr );
					}

					// now make sure enabling matches and copy the successful ptr
					j->m_FileMgr->Enable( enable );
					foundTanks.push_back( &*j );

					// give the autoptr a new filemgr
					tankFileMgr = stdx::make_auto_ptr( new TankFileMgr );
				}
			}
			else
			{
				gperrorf(( "Unable to open resource file due to version mismatch:\n"
						   "\n"
						   "\tTank filename                : %s\n"
						   "\tTank requires EXE version    : %s\n"
						   "\tCurrently running EXE version: %s\n",
						   fileName.c_str(),
						   ::ToString( tankFileMgr->GetTankMinimumVersion(), gpversion::MODE_AUTO_LONG ).c_str(),
						   ::ToString( ::GetDungeonSiegeVersion(), gpversion::MODE_AUTO_LONG ).c_str() ));
			}
		}
		else if ( ::GetLastError() != ERROR_BAD_FORMAT )
		{
			gperrorf(( "Unable to open tank '%s', error is '%s' (0x%08X)\n",
					   fileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
		}
	}

	// mount our tanks
	TankList::const_iterator j, jbegin = foundTanks.begin(), jend = foundTanks.end();
	for ( j = jbegin ; j != jend ; ++j )
	{
		TankFileMgr* tankFileMgr = (*j)->m_FileMgr;

		GetGlobalFileMgr()->AddFileMgr( tankFileMgr, true, tankFileMgr->GetTankPriorityForMaster() );
		gpgenericf(( "FileSys %s = '%s'\n", ::ToString( type ), tankFileMgr->GetTankFileName().c_str() ));
	}
}

void TankMgr :: findBits( void )
{
#	if !GP_RETAIL

	// get some specs
	gpwstring pathVar = gConfig.ResolvePathVars( L"%bits%", Config::PVM_APPEND );

	// find paths from those specs
	std::list <gpwstring> foundPaths;
	FileSys::FindPathsFromSpec( pathVar, foundPaths );

	// working mgr
	std::auto_ptr <PathFileMgr> pathFileMgr( new PathFileMgr );

	// build filemgrs and mount directly
	std::list <gpwstring>::const_iterator i, ibegin = foundPaths.begin(), iend = foundPaths.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		gpstring pathName( ::ToAnsi( *i ) );

		// try to use it
		if ( pathFileMgr->SetRoot( pathName ) )
		{
			// set options we need
			pathFileMgr->SetOptions( PathFileMgr::OPTION_COPY_NET_LOCAL );

			// mount it
			m_MultiFileMgr->AddFileMgr( pathFileMgr.release(), true );

			// give the autoptr a new filemgr
			pathFileMgr = stdx::make_auto_ptr( new PathFileMgr );
		}
	}

#	endif // !GP_RETAIL
}

void TankMgr :: findMaps( const char* fuelRoot, TankEntry* tankEntry, MapInfoColl* coll )
{
	// install new text fueldb just to read out the map gas files
	TextFuelDB* fuelDb = gFuelSys.AddTextDb( "maps_query" );
	fuelDb->Init( FuelDB::OPTION_READ | FuelDB::OPTION_JIT_READ, fuelRoot );

	// read them out
	FuelHandle mapBase( "::maps_query:root" );
	FuelHandleList maps = mapBase->ListChildBlocks();
	FuelHandleList::iterator i, ibegin = maps.begin(), iend = maps.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// fill it
		MapInfo info;
		info.m_InternalName = (*i)->GetName();

		// get at child
		FuelHandle map = (*i)->GetChildBlock( "map" );
		if ( map && ((coll != NULL) || !HasMap( info.m_InternalName )) )
		{
			// extract map traits
			info.m_ScreenName         = ReportSys::TranslateW( map->GetWString( "screen_name" ) );
			info.m_Description        = ReportSys::TranslateW( map->GetWString( "description" ) );
			info.m_IsSingleplayerOnly = map->GetBool( "singleplayer_only", false );
			info.m_IsMultiplayerOnly  = map->GetBool( "multiplayer_only", false );
			info.m_IsDevOnly          = map->GetBool( "dev_only", false );

			// default
			if ( info.m_ScreenName.empty() )
			{
				info.m_ScreenName = ::ToUnicode( info.m_InternalName );
			}

			// add tank info
			if ( (tankEntry != NULL) && (tankEntry->m_FileMgr != NULL) )
			{
				info.m_TankFileName = tankEntry->m_FileMgr->GetTankFileName();
				info.m_TankGuid     = tankEntry->m_FileMgr->GetTankGuid();
			}

			// add worlds
			FuelHandle worlds( map->GetChildBlock( "worlds" ) );
			if ( worlds )
			{
				info.m_MpWorlds.Load( worlds );
			}

			// ready to add to collection, make sure we have entry
			if ( (tankEntry == NULL) && (coll == NULL) )
			{
				tankEntry = &*m_Tanks.insert( m_Tanks.begin(), TankEntry() );
			}

			// add to collection
			if ( tankEntry != NULL )
			{
				tankEntry->m_MapInfoColl.push_back( info );
			}
			else if ( coll != NULL )
			{
				coll->push_back( info );
			}
		}
	}

	// uninstall fuel
	gFuelSys.Remove( fuelDb );
}

void TankMgr :: findMaps( void )
{
	// done?
	if ( m_MapsScanned )
	{
		return;
	}
	m_MapsScanned = true;

	// find the maps that are off the home dir
	if ( FileSys::DoesResourcePathExist( "world/maps" ) )
	{
		findMaps( "world/maps", NULL, NULL );
	}

	// find .dsmap files if we are the game (nothing else can open-maps-from-tank)
	if ( Services::IsDungeonSiege() )
	{
		findTanks( TANK_MAP );
	}
}

bool TankMgr :: hasTank( const GUID& guid ) const
{
	return ( stdx::has_any( m_Tanks, guid ) );
}

//////////////////////////////////////////////////////////////////////////////
// struct TankMgr::TankEntry implementation

bool TankMgr::TankEntry :: operator == ( const GUID& guid ) const
{
	return ( (m_FileMgr != NULL) && (m_FileMgr->GetTankGuid() == guid) );
}

//////////////////////////////////////////////////////////////////////////////
