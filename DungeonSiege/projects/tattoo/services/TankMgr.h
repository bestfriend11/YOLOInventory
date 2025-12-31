//////////////////////////////////////////////////////////////////////////////
//
// File     :  TankMgr.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a class to manage tank files. It knows about maps,
//             mods, search paths, and such, and can remount tanks on demand.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __TANKMGR_H
#define __TANKMGR_H

//////////////////////////////////////////////////////////////////////////////

#include "GoDefs.h"
#include "GpColl.h"

//////////////////////////////////////////////////////////////////////////////
// documentation

/*
	Convention:

	$required

		A file $required may exist in the root dir of each tank. This file is a
		simple list of names, one per line, that are required in order to run
		the tank. For simplicity, self-references are ignored.

		Usage for .dsmod files:

			When creating a .dsmod tank that is intended for a particular .dsmap
			file, put $required in it with the name of the .dsmap file. Then
			this .dsmod will not be available *except* when that particular
			.dsmap is chosen.

			If the $required file is empty, then that means this .dsmod is
			available to any .dsmap that wants it, but only if referenced by
			name from the .dsmap. In other words it's not choosable from the
			"choose mod" interface.

		Usage for .dsmap files:

			When creating a .dsmap tank that requires a particular .dsmod file
			to operate, put a $required in it with the name of that .dsmod file.
			Then that .dsmod will automatically be chosen when the map is used.
*/

//////////////////////////////////////////////////////////////////////////////
// forward declarations

class FuelHandle;

namespace FileSys
{
	class TankFileMgr;
	class MasterFileMgr;
	class MultiFileMgr;
	class PathFileMgr;
}

namespace FuBi
{
	class PersistContext;
}

//////////////////////////////////////////////////////////////////////////////
// helper functions

// this tries to query the version from the DS EXE, falling back to .h file
gpversion GetDungeonSiegeVersion( void );
gpstring  GetDungeonSiegeVersionText( gpversion::eMode mode );

// this is secret!
bool HasDungeonSiegeSpecialSkipCdCheckFile( void );
bool HasDungeonSiegeSpecialEnableDsrMpFile( void );
bool HasAnyDungeonSiegeSpecialFile        ( void );

//////////////////////////////////////////////////////////////////////////////
// enum eTankType declaration

enum eTankType
{
	SET_BEGIN_ENUM( TANK_, 0 ),

	TANK_NONE,
	TANK_RESOURCE,
	TANK_MAP,
	TANK_MOD,

	SET_END_ENUM( TANK_ ),
};

const char* ToString   ( eTankType e );
const char* ToPathVar  ( eTankType e );
const char* ToExtension( eTankType e );
bool        FromString ( const char* str, eTankType& e );

//////////////////////////////////////////////////////////////////////////////
// enum eModFlags declaration

enum eModFlags
{
	MOD_NONE						= 0,
	MOD_AFFECTS_MP_VERSION			= 1 << 0,	// includes files that affect the multiplayer version code
	MOD_REQUIRES_CONTENTDB_RELOAD	= 1 << 1,	// true if loading/unloading this mod requires a reload of the contentdb
	MOD_REQUIRES_NNK_RELOAD			= 1 << 2,	// true if loading/unloading this mod requires a reload of the naming key
	MOD_REQUIRES_WORLDFX_RELOAD		= 1 << 3,	// true if loading/unloading this mod requires a reload of flamethrower scripts
	MOD_INCLUDES_MAP_DATA			= 1 << 4,	// true if there is any map data in here, meaning do not load in SE!
};

eModFlags DetectModFlags( FileSys::TankFileMgr* fileMgr );

MAKE_ENUM_BIT_OPERATORS( eModFlags );

//////////////////////////////////////////////////////////////////////////////
// struct TankInfo declaration

struct TankInfo
{
	// persistent
	gpstring    m_FileName;			// filename of tank
	eTankType   m_TankType;			// type of this tank according to how it was loaded
	gpversion   m_Version;			// product version this tank was built with
	FOURCC      m_CreatorId;		// who created the tank (creation tool will choose, not user)
	GUID        m_GUID;				// true GUID assigned at creation time
	DWORD       m_IndexCRC32;		// CRC-32 of just the index (not including the header)
	DWORD       m_DataCRC32;		// CRC-32 of just the data
	SYSTEMTIME  m_UtcBuildTime;		// when the tank was constructed (stored in UTC)

	// loaded from tank only
	gpwstring   m_Title;			// title of tank (not stored!)
	eModFlags   m_ModFlags;			// only set if type = TANK_MOD
	MSQAVersion m_MinVersion;		// min version this tank requires

	TankInfo( void );

	void SaveToFuel    ( FuelHandle fuel );
	void LoadFromSystem( FileSys::TankFileMgr* fileMgr, eTankType type );
	bool LoadFromFuel  ( FuelHandle fuel );
	bool Xfer          ( FuBi::PersistContext& persist );
};

//////////////////////////////////////////////////////////////////////////////
// struct TankManifest declaration

struct TankManifest : stdx::fast_vector <TankInfo>
{
	void SaveToFuel    ( const char* key, FuelHandle fuel );
	void LoadFromSystem( void );
	void LoadFromSystem( FileSys::TankFileMgr* fileMgr, eTankType type );
	bool LoadFromFuel  ( const char* key, FuelHandle fuel );
	bool Xfer          ( const char* key, FuBi::PersistContext& persist );
};

//////////////////////////////////////////////////////////////////////////////
// class TankMgr declaration

class TankMgr : public Singleton <TankMgr>
{
public:
	SET_NO_INHERITED( TankMgr );

	typedef FileSys::MasterFileMgr MasterFileMgr;
	typedef FileSys::MultiFileMgr  MultiFileMgr ;
	typedef FileSys::TankFileMgr   TankFileMgr  ;
	typedef FileSys::PathFileMgr   PathFileMgr  ;

// Setup.

	// ctor/dtor
	TankMgr( void );
   ~TankMgr( void );

	// setup
	bool Init( void );

	// query the masterfilemgr that we own
	MasterFileMgr* GetMasterFileMgr( void )				{  return ( m_MasterFileMgr );  }
#	if GP_RETAIL
	MasterFileMgr* GetGlobalFileMgr( void )				{  return ( GetMasterFileMgr() );  }
#	else // GP_RETAIL
	MultiFileMgr * GetMultiFileMgr ( void )				{  return ( m_MultiFileMgr );  }
	MultiFileMgr * GetGlobalFileMgr( void )				{  return ( GetMultiFileMgr() );  }
#	endif // GP_RETAIL

// Commands.

	// query
	void      GetTankManifest    ( TankManifest& manifest );
	gpversion GetNewestMinVersion( void );
	bool      HasAnyTanks        ( void ) const;

	// map info query
	int  GetMapInfos  ( const char* fuelRoot, MapInfoColl& coll );
	int  GetMapInfos  ( MapInfoColl& coll );					// query all available map tanks and fill out the mapinfo structure for each
	bool GetMapInfo   ( const char* fuelRoot, const char* mapName, MapInfo& info );
	bool GetMapInfo   ( const char* mapName, MapInfo& info );	// get map info for a particular map
	void ResetMapInfos( void );									// clear the map info cache (only needed when new tanks loaded)

	// current map
	bool     SetMap    ( const gpstring& mapName );				// mount a specific tank's map set
	bool     HasMap    ( const char* mapName )	;				// see if this map exists in the set
	bool     HasAnyMaps( void );								// see if any maps exist
	bool     IsMapSet  ( const char* mapName ) const;			// see if this map is currently mounted
	bool     IsMapSet  ( void ) const;							// see if any map is currently mounted
	void     ClearMap  ( void );								// unmount any existing map tank
	MapInfo& GetMapInfo( void ) const;							// get info on current map

private:

	typedef std::set <gpstring, istring_less> StringSet;

	struct TankEntry
	{
		eTankType    m_Type;			// type of this tank
		eModFlags    m_ModFlags;		// if mod type, any flags
		MapInfoColl  m_MapInfoColl;		// info about the maps contained here
		TankFileMgr* m_FileMgr;			// may be NULL if for bits only
		bool         m_HasRequiredFile;	// true if the $required file exists
		StringSet    m_RequiredTanks;	// list of tanks required for this tank to be used

		TankEntry( void )
		{
			m_Type            = TANK_NONE;
			m_ModFlags        = MOD_NONE;
			m_FileMgr         = NULL;
			m_HasRequiredFile = false;
		}

		bool operator == ( eTankType type ) const
		{
			return ( m_Type == type );
		}

		bool operator == ( const TankFileMgr* tankFileMgr ) const
		{
			return ( m_FileMgr == tankFileMgr );
		}

		bool operator == ( const GUID& guid ) const;
	};

	typedef std::list <TankEntry> TankFileMgrColl;

	bool init     ( int attempt = 0 );			// returns true if core resources are available
	void deinit   ( void );
	void findTanks( eTankType type, bool enable = false );
	void findBits ( void );
	void findMaps ( const char* fuelRoot, TankEntry* tankEntry, MapInfoColl* coll );
	void findMaps ( void );
	bool hasTank  ( const GUID& guid ) const;

	// owned systems
	my MasterFileMgr* m_MasterFileMgr;			// master filemgr
#	if !GP_RETAIL
	my MultiFileMgr*  m_MultiFileMgr;			// dev-only filemgr
#	endif // !GP_RETAIL

	// resource state
	bool            m_MapsScanned;				// remember if we've scanned the maps or not
	TankFileMgrColl m_Tanks;					// currently mounted/resolved resource tanks
	MapInfo*        m_CurrentMapInfo;			// the currently selected map
	TankEntry*      m_CurrentMapTank;			// the currently mounted map tank

	SET_NO_COPYING( TankMgr );
};

#define gTankMgr Singleton <TankMgr>::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

#endif  // __TANKMGR_H

//////////////////////////////////////////////////////////////////////////////
