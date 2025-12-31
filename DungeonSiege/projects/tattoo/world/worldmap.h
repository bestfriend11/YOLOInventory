/*============================================================================

  WorldMap

  author:	Bartosz Kijanka, Scott Bilas

  date:		Nov. 17, 1999

  (C)opyright Gas Powered Games 1999

----------------------------------------------------------------------------*/
#ifndef __WORLDMAP_H
#define __WORLDMAP_H
#pragma once

//////////////////////////////////////////////////////////////////////////////

#include "GoDefs.h"
#include "GpColl.h"
#include "CameraPosition.h"
#include "Siege_Transition.h"

//////////////////////////////////////////////////////////////////////////////
// class WorldMap declaration

class WorldMap : public Singleton <WorldMap>
{
public:

// Types.

	typedef siege::database_guid database_guid;

	struct StartingPos
	{
		SiegePos       spos;
		CameraPosition cpos;
	};

	typedef stdx::linear_map <unsigned int, StartingPos> SPosMap;

	struct WorldStartLevel
	{
		gpstring sWorld;
		float	 requiredLevel;
	};

	typedef std::vector< WorldStartLevel > WorldStartLevels;

	struct StartingGroup
	{
		int		  id;
		SPosMap   positions;
		bool      bDefault;
		gpstring  sGroup;
		gpwstring sDescription;		// pretranslated
		gpwstring sScreenName;		// pretranslated
		bool	  bEnabled;
		
		WorldStartLevels startLevels;

		StartingGroup( void )
		{
			id = 0;
			bDefault = false;
			bEnabled = true;
		}
	};

	typedef stdx::fast_vector <StartingGroup> SGroupColl;

	struct WorldLocation
	{
		gpstring  sName;
		gpwstring sScreenName;		// pretranslated
	};

	typedef stdx::linear_map <unsigned int, WorldLocation> WorldLocationMap;

// Setup.

	// ctor/dtor
	WorldMap( void );
   ~WorldMap( void );

	// main setup

	bool SSet( const char * name, const char * world = NULL, DWORD machineId = RPC_TO_ALL );
	FUBI_EXPORT FuBiCookie RCSet( const char * name, const char * world, DWORD machineId );
	FUBI_MEMBER_SET( RCSet, +SERVER );

	bool Init              ( const char* name, const char* world = NULL,				// initializes map - sets its base fuel dir and loads starting positions
							 bool force = false, bool forLoadingSavedGame = false );
	void InitLoaders       ( void );													// initializes the loaders - meant for editor use only
	bool Shutdown          ( bool fullShutdown = true );								// unloads the current map and clears all indexes
	bool IsInitialized     ( void ) const							{  return ( m_bInitialized );  }
	bool CheckIndexesLoaded( void );

	// persistence
	bool		Xfer( FuBi::PersistContext& persist );
	void		SSyncOnMachine( DWORD machineId );
FEX	FuBiCookie	RCSyncOnMachineHelper( DWORD machineId, const_mem_ptr syncBlock );
	void		XferForSync( FuBi::PersistContext& persist );

	// reloading
	bool ReloadContentIndexes( void );							// clean out and reload content-related indexes only (game objects)

	// editor specific functions
#	if !GP_RETAIL
	void OnEditorAddedRegionToMap( RegionId id, const gpstring& regionName );
	void OnEditorAddedNodeToMap  ( RegionId id, database_guid nodeGuid );
	void SeekAndDestroyDataRot   ( void );
#	endif // !GP_RETAIL

// Query.

	// map query
	const gpstring&  GetMapName       ( void ) const				{  gpassert( IsInitialized() );  return ( m_MapName );  }
	const gpwstring& GetMapScreenName ( void ) const;
	bool             GetShowIntro     ( void ) const				{  gpassert( IsInitialized() );  return ( m_ShowIntro );  }
	gpstring         MakeMapDirAddress( void ) const;
	FastFuelHandle   MakeMapDirFuel   ( void ) const;
	gpstring         MakeMapDefAddress( void ) const;
	FastFuelHandle   MakeMapDefFuel   ( void ) const;

	// region query
	const gpstring&  GetRegionName       ( RegionId id ) const;
FEX	const gpstring&  GetRegionNameForNode( database_guid guid ) const;
	bool             ContainsRegion      ( RegionId id ) const;
	RegionId         GetRegionByNode     ( database_guid guid ) const;
	gpstring         MakeRegionDirAddress( RegionId id ) const;
	FastFuelHandle   MakeRegionDirFuel   ( RegionId id ) const;
	gpstring         MakeRegionDefAddress( RegionId id ) const;
	FastFuelHandle   MakeRegionDefFuel   ( RegionId id ) const;
	int              GetAllRegionsDirFuel( FastFuelHandleColl& coll ) const;
	int              GetAllRegionsDefFuel( FastFuelHandleColl& coll ) const;

	// quest query
	gpstring         MakeQuestsAddress	( void ) const;
	gpstring         MakeChaptersAddress( void ) const;
	FastFuelHandle   MakeQuestsFuel		( void ) const;
	FastFuelHandle   MakeChaptersFuel	( void ) const;
	int              GetAllQuestsFuel	( FastFuelHandleColl& coll ) const;
	int				 GetAllChaptersFuel	( FastFuelHandleColl& coll ) const;

	// conversation query
	gpstring         MakeConversationAddress( RegionId id, const char* convoName ) const;
	FastFuelHandle   MakeConversationFuel   ( RegionId id, const char* convoName ) const;

	// bookmark query
	gpstring         MakeBookmarksAddress( void ) const;
	FastFuelHandle   MakeBookmarksFuel   ( void ) const;
	bool             LoadBookmark        ( const gpstring& name, SiegePos& pos, CameraPosition& cameraPos ) const;
	bool             LoadBookmark        ( FastFuelHandle hBookmark, SiegePos& pos, CameraPosition& cameraPos ) const;
#	if !GP_RETAIL
	bool             DumpBookmarks       ( ReportSys::ContextRef ctx = NULL ) const;
	bool             DumpBookmarks       ( const char* pattern, ReportSys::ContextRef ctx = NULL ) const;
#	endif // !GP_RETAIL

	// node query
	bool             ContainsNode   ( database_guid guid ) const;
	gpstring         MakeNodeAddress( database_guid guid ) const;
	FastFuelHandle   MakeNodeFuel   ( database_guid guid ) const;

	// object query
	bool             ContainsScid  ( Scid scid ) const;
	FastFuelHandle   MakeObjectFuel( Scid scid, RegionId regionId, const SiegeGuid& forNodeGuid = siege::UNDEFINED_GUID, bool errorOnFail = false ) const;

// Multiplayer worlds.

	void            GetMpWorlds   ( MpWorldColl& coll );
	bool            HasMpWorld    ( const char* mpWorldName );
	bool            SetMpWorld    ( const gpstring& mpWorldName, bool forLoadingSavedGame = false );
FEX	const gpstring& GetMpWorldName( void ) const				{  return ( m_MpWorldName );  }

	bool SSetMpWorld( const gpstring& mpWorldName, DWORD machineId = RPC_TO_ALL );
	FUBI_EXPORT FuBiCookie RCSetMpWorld( const gpstring& mpWorldName, DWORD machineId );
	FUBI_MEMBER_SET( RCSetMpWorld, +SERVER );

// Starting positions and bookmarks.

	// player assignment of positions
	void SAssignStartingPositions( void );
	void SVerifyValidPlayerStartingPositions( void );
	float GetRequiredStartingPositionLevel( const gpstring& startPosition ) const;
	DWORD GetNumStartingPositionsAssigned()						{  return m_PositionsAssigned; }

	// starting position query
	bool GetDefaultStartingGroup   ( gpstring& groupName ) const;
	bool GetDefaultStartingPosition( StartingPos& startingPos, int id ) const;
	bool GetRandomStartingPosition ( const char* groupName, SiegePos& pos, CameraPosition& cameraPos ) const;

	// direct access
	const SGroupColl& GetStartGroups( void ) const				{  return ( m_SGroupColl );  }	;
FEX void  SEnableStartGroup			( int id, bool bEnabled );
FEX	void  RCEnableStartGroup		( int id, bool bEnabled );
FEX	void  SSetDefaultStartGroup		( int id, bool bSet		);
FEX	void  RCSetDefaultStartGroup	( int id, bool bSet		);

// World location names

	unsigned int GetWorldLocationId( const gpstring& name ) const;
	const gpwstring& GetWorldLocationScreenName( unsigned int id ) const;

// Streaming API.

	// world frustum parameters
	float GetWorldFrustumRadius ( void ) const					{  return ( m_WorldFrustumRadius ); }
	void  SetWorldFrustumRadius ( float radius)					{  m_WorldFrustumRadius = radius; }
	float GetWorldFrustumHeight ( void ) const					{  return ( m_WorldFrustumHeight );  }
	void  SetWorldFrustumHeight ( float height )				{  m_WorldFrustumHeight = height;  }
	float GetWorldFrustumDepth  ( void ) const					{  return ( m_WorldFrustumDepth );  }
	void  SetWorldFrustumDepth	( float depth )					{  m_WorldFrustumDepth = depth;  }
	float GetWorldInterestRadius( void ) const					{  return ( m_WorldInterestRadius ); }
	void  SetWorldInterestRadius( float radius )				{  m_WorldInterestRadius = radius; }

	// restrict streaming to only this region (generally needed only for the editor)
	void RestrictStreamingToRegion( RegionId id )				{  m_ClampingRegionGUID = id;  }
	void UnrestrictStreaming      ( void )						{  RestrictStreamingToRegion( REGIONID_INVALID );  }
	bool IsStreamingRestricted    ( void ) const				{  return ( m_ClampingRegionGUID != REGIONID_INVALID );  }

	// creates a slot for the given node and loads in all GAS information.
	bool PreloadNode( database_guid nodeGuid );

	// loads LNC (logical) information, SNO (mesh) information, and texture,
	// and performs realtime regional boundary connections.
	bool LoadNode( database_guid nodeGuid );

	// adds requests to the streamer to load go's for this node
	bool LoadNodeContent( database_guid nodeGuid, bool localOnly = false, bool loadNow = false );

	// for multiplayer slim loading - causes a node to add or remove its lodfi content
	bool LoadLodfiContent( database_guid nodeGuid, bool load );

	// for multiplayer slim loading - causes a remove its local-plain content
	bool UnloadLocalContent( database_guid nodeGuid );

	// loads all the scids for all nodes that have them
	void LoadAllNodeContent( void );

	// deletes and requests loads for lodfi scids depending on detail or slim loading settings
	void SyncNodeLodfiContent( database_guid nodeGuid );

// Static content API.

	// scid index modifications - only meant to be used by GoDb!
#	if !GP_RETAIL
	void ValidateMapScidIndexes( void ) const;
	bool SetScidIndex          ( Scid scid, const gpstring& fuel );
	void RemoveScidIndex       ( Scid scid );
	int  ClearRegionScidIndexes( const gpstring& addrPrefix );
#	endif // !GP_RETAIL

	enum eLoadSpec
	{
		LOAD_DEFAULT         =      0,
		LOAD_LODFI_ONLY      = 1 << 0,
		LOAD_LOCAL_ONLY      = 1 << 1,
		LOAD_FOR_DETAIL_SYNC = 1 << 2,
		LOAD_FOR_SLIM_LOAD   = 1 << 3,
		LOAD_FULL            = 1 << 4,
		LOAD_IMMEDIATE       = 1 << 5,
	};

private:

// Private methods.

	// loader helpers
	bool loadCamera           ( void );
	bool loadDatabases        ( void );
	bool loadStitchDb         ( void );
	bool loadStartingPositions( void );
	bool loadHotpoints        ( void );
	bool loadWorldLocations   ( void );

	// loader helper helpers
	bool loadNodeInfoDb  ( RegionId regionId );
	bool loadContentIndex( RegionId regionId );
	bool loadNodeMeshInfoDb	  ( void );

	// loader helper helper helper
	bool loadNodeMeshInfo	  ( const gpstring & guidString, const gpstring & filename, const gpstring & address, bool bHasNodeMeshIndex = false );

	// collect the doors for the given node
	void collectDoors( database_guid nodeGuid, siege::SiegeTransition::NodeDoorColl& doorColl );

	// stitch node info using the stitch db
	void autoStitchNode( siege::SiegeNode& node );

	// node content loader
	bool loadNodeContent( database_guid nodeGuid, eLoadSpec spec );

// Types.

	// holds info to stitch two nodes together
	struct StitchInfo
	{
		unsigned int	MeDoor;
		database_guid	NeighborGUID;
		unsigned int	NeighborDoor;
	};

	// holds actual calcs for stitch data
	struct LeafStitch
	{
		unsigned short	NearLeafId;
		unsigned short	FarLeafId;
	};
	struct FarStitch
	{
		unsigned char	FarLogicalId;
		stdx::fast_vector< LeafStitch > LeafConnections;
	};
	struct LogicalStitch
	{
		unsigned char	NearLogicalId;
		stdx::fast_vector< FarStitch > FarConnections;
	};
	struct StitchCalc
	{
		database_guid	NeighborGUID;
		stdx::fast_vector< LogicalStitch > NeighborConnections;
	};

	// collections
	typedef stdx::linear_map      < RegionId, gpstring >								RegionIdToNameDb;
	typedef stdx::linear_multimap < database_guid, StitchInfo >							NodeGuidToStitchInfoDb;
	typedef stdx::linear_map      < database_guid, stdx::fast_vector< StitchCalc > >	NodeGuidToStitchCalcDb;
	typedef stdx::fast_vector     < Scid >												ScidColl;

#	if !GP_RETAIL
	// holds info about a scid
	struct ScidInfo
	{
		gpstring      m_FuelAddress;					// where to find this go
		database_guid m_NodeGuid;						// what node it's in
		int           m_TouchCount;						// num times been touched (used for debugging)

		ScidInfo( void )
		{
			m_TouchCount = 0;
		}
	};

	// dev collections
	typedef std::map <Scid, ScidInfo> ScidDb;
	typedef std::multimap <Scid, ScidInfo> ScidMdb;

	// collection for finding duplicate node mesh info.
	typedef std::map <siege::database_guid, FastFuelHandle> SiegeDupDb;	

#	endif // !GP_RETAIL

	// holds info about a node in the map
	struct NodeInfo
	{
		DWORD m_ScidIndex;								// starting index into m_ScidColl for
		WORD  m_ScidCount;								// number of scids in this node
		WORD  m_RegionIndex;							// index into m_RegionIdToNameDb for this node

		NodeInfo( void )
		{
			m_ScidIndex   = 0xFFFFFFFF;
			m_ScidCount   = 0;
			m_RegionIndex = 0xFFFF;
		}
	};
	COMPILER_ASSERT( sizeof( NodeInfo ) == 8 );			// guarantee i'm getting the size i want

	// collections
	typedef stdx::linear_map <database_guid, NodeInfo> NodeGuidToNodeInfoDb;

// Data.

	bool	   m_bInitialized;
	// map traits
	gpstring   m_MapName;								// this map's name
	gpstring   m_MpWorldName;							// multiplayer world difficulty determines the prefix to use for paths
	bool       m_ShowIntro;								// should we show the gpg/ms intro thingy when entering the map?
	RegionId   m_ClampingRegionGUID;					// clamp streaming to this region (set to invalid region id when normal gameplay)
	SGroupColl m_SGroupColl;							// db of starting groups
	DWORD	   m_PositionsAssigned;						// number of positions last assined by SAssignStartingPositions()
	WorldLocationMap m_WorldLocations;					// world location names

	// world frustum traits
	float m_WorldFrustumRadius;							// radius of the world frustum
	float m_WorldFrustumHeight;							// height of the world frustum
	float m_WorldFrustumDepth;							// depth of the world frustum
	float m_WorldInterestRadius;						// radius of the interest volume for world frustum

	// indexes
	bool                   m_IndexesLoaded;				// have indexes been loaded yet?
	RegionIdToNameDb       m_RegionIdToNameDb;			// map of region id's to their internal region names
	NodeGuidToStitchInfoDb m_NodeGuidToStitchInfoDb;	// map of node guid's to their stitch infoz (only nodes that are inter-regional are here)
	NodeGuidToStitchCalcDb m_NodeGuidToStitchCalcDb;	// map of node guid's to their stitch calcz
	NodeGuidToNodeInfoDb   m_NodeGuidToNodeInfoDb;		// map of node guid's to node infoz
	ScidColl               m_ScidColl;					// simple vector of scids, referenced by the node info db
#	if !GP_RETAIL
	ScidDb                 m_ScidDb;					// map of scids to their addresses
	ScidMdb                m_RottingScidDb;				// db of scids that are leftovers (somehow got orphaned)
	SiegeDupDb			   m_SiegeDupDb;				// db of guids to track duplicates (cleared after usage)
#	endif // !GP_RETAIL

	FUBI_SINGLETON_CLASS( WorldMap, "World map object." );
	SET_NO_COPYING( WorldMap );
};

MAKE_ENUM_BIT_OPERATORS( WorldMap::eLoadSpec );

#define gWorldMap WorldMap::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

#endif __WORLDMAP_H

//////////////////////////////////////////////////////////////////////////////
