/*============================================================================

  WorldMap

  (C)opyright Gas Powered Games 1999

----------------------------------------------------------------------------*/

#include "precomp_world.h"

#include "CameraAgent.h"
#include "Config.h"
#include "ContentDb.h"
#include "FuBiSchema.h"
#include "FuBiPersistImpl.h"
#include "FuBiTraits.h"
#include "GoShmo.h"
#include "NamingKey.h"
#include "NetFuBi.h"
#include "RatioStack.h"
#include "ReportSys.h"
#include "Services.h"
#include "Server.h"
#include "Siege_Camera.h"
#include "Siege_Loader.h"
#include "Siege_Mesh_Door.h"
#include "Siege_Node.h"
#include "Siege_Node_Io.h"
#include "siege_hotpoint_database.h"
#include "siege_mouse_shadow.h"
#include "TankMgr.h"
#include "TimeOfDay.h"
#include "Victory.h"
#include "World.h"
#include "WorldMap.h"
#include "WorldState.h"


using namespace siege;


//////////////////////////////////////////////////////////////////////////////
// class WorldLoader declaration and implementation

class WorldLoader : public siege::Loader
{
public:
	SET_INHERITED( WorldLoader, siege::Loader );

	WorldLoader( void )									{  }
	virtual ~WorldLoader( void )						{  }

	struct LoadOrder
	{
		FOURCC    m_OrderType;
		SiegeGuid m_NodeGuid;
		DWORD     m_TexId;
		Scid      m_Scid;
		RegionId  m_RegionId;
		bool      m_FadeIn;

		LoadOrder( FOURCC orderType, SiegeGuid guid )
			: m_OrderType( orderType ), m_NodeGuid( guid )  {  }
		LoadOrder( FOURCC orderType, DWORD id )
			: m_OrderType( orderType ), m_TexId( id )  {  }
		LoadOrder( FOURCC orderType, Scid scid, RegionId regionId, SiegeGuid guid, bool fadeIn )
			: m_OrderType( orderType ), m_Scid( scid ), m_RegionId( regionId ), m_NodeGuid( guid ), m_FadeIn( fadeIn )  {  }
	};

	virtual void* OnCreateLoadOrder( FOURCC type, const gpstring& /*name*/, DWORD id, void* extra )
	{
		SiegeGuid guid( id );

		if ( type == siege::LOADER_SIEGE_NODE_TYPE )
		{
			// preload the node
			if ( !gWorldMap.PreloadNode( guid ) )
			{
				// failure
				return NULL;
			}
		}
		else if ( type == siege::LOADER_TEXTURE_TYPE )
		{
			return ( new LoadOrder( type, id ) );
		}
		else if ( type == LOADER_LODFI_TYPE )
		{
			LodfiLoaderInfo* info = (LodfiLoaderInfo*)extra;
			gpassert( info != NULL );
			return ( new LoadOrder( type, (Scid)id, info->m_RegionId, info->m_NodeGuid, info->m_FadeIn ) );
		}
		else
		{
			gpassert( type == LOADER_GO_TYPE );
		}

		return ( new LoadOrder( type, guid ) );
	}

	virtual void OnDestroyLoadOrder( void* data )
	{
		delete( (LoadOrder*)( data ) );
	}

	virtual eCommitCode OnCommitLoadOrder( void* data )
	{
		LoadOrder* order = (LoadOrder*)data;

		if ( order->m_OrderType == siege::LOADER_SIEGE_NODE_TYPE )
		{
			return ( gWorldMap.LoadNode( order->m_NodeGuid )
					 ? COMMIT_SUCCESS : COMMIT_FAILURE );
		}
		else if ( order->m_OrderType == siege::LOADER_TEXTURE_TYPE )
		{
			return ( gDefaultRapi.LoadTexture( order->m_TexId )
					 ? COMMIT_SUCCESS : COMMIT_FAILURE );
		}
		else if ( order->m_OrderType == LOADER_LODFI_TYPE )
		{
			return ( (gGoDb.LoadLocalGo( order->m_Scid, order->m_RegionId, order->m_NodeGuid, true, order->m_FadeIn ) != GOID_INVALID)
					 ? COMMIT_SUCCESS : COMMIT_FAILURE );
		}
		else
		{
			gpassert( order->m_OrderType == LOADER_GO_TYPE );
			return ( gWorldMap.LoadNodeContent( order->m_NodeGuid )
					 ? COMMIT_SUCCESS : COMMIT_FAILURE );
		}
	}

#	if !GP_RETAIL
	virtual gpstring OnStringizeLoadOrder( const void* data )
	{
		LoadOrder* order = (LoadOrder*)data;

		if ( order->m_OrderType == siege::LOADER_SIEGE_NODE_TYPE )
		{
			return ( order->m_NodeGuid.ToString() );
		}
		else if ( order->m_OrderType == siege::LOADER_TEXTURE_TYPE )
		{
			return ( FileSys::GetFileNameOnly( gDefaultRapi.GetTexturePathname( order->m_TexId ) ) );
		}
		else if ( order->m_OrderType == LOADER_LODFI_TYPE )
		{
			FastFuelHandle fuel( gWorldMap.MakeObjectFuel( order->m_Scid, order->m_RegionId ) );
			gpstring name;
			if ( fuel )
			{
				name.assignf( "%s (0x%08X)", fuel.GetType(), order->m_Scid );
			}
			else
			{
				name.assignf( "0x%08X", order->m_Scid );
			}
			return ( name );
		}
		else
		{
			return ( order->m_NodeGuid.ToString() );
		}
	}
#	endif // !GP_RETAIL

private:
	SET_NO_COPYING( WorldLoader );
};

//////////////////////////////////////////////////////////////////////////////
// class WorldMap implementation

WorldMap :: WorldMap( void )
{
	m_ClampingRegionGUID  = REGIONID_INVALID;
	m_WorldFrustumRadius  = 0;
	m_WorldFrustumHeight  = 0;
	m_WorldFrustumDepth	  = 0;
	m_WorldInterestRadius = 0;
	m_IndexesLoaded       = false;
	m_ShowIntro           = false;
	m_bInitialized		  = false;
	m_PositionsAssigned	  = 0;

	FastFuelHandle hEngine( "config:engine_settings" );
	if( hEngine.IsValid() )
	{
		// install some siege stuff
		gSiegeLoadMgr.InstallLoader( siege::LOADER_SIEGE_NODE_TYPE, new WorldLoader, hEngine.GetFloat( "streamer_node_time" ) );
		gSiegeLoadMgr.InstallLoader( siege::LOADER_TEXTURE_TYPE,	new WorldLoader, hEngine.GetFloat( "streamer_texture_time" ) );
		gSiegeLoadMgr.InstallLoader( LOADER_GO_TYPE,				new WorldLoader, hEngine.GetFloat( "streamer_go_time" ) );
		gSiegeLoadMgr.InstallLoader( LOADER_LODFI_TYPE,				new WorldLoader, hEngine.GetFloat( "streamer_lodfi_time" ) );

		// set the overflow time
		gSiegeLoadMgr.SetOverflowTime( hEngine.GetFloat( "streamer_overflow_time" ) );

		// set the constants used to determine load spreading.
			
		gSiegeLoadMgr.SetGoLoadCost(		hEngine.GetInt(		"go_load_cost",			100 ) );
		gSiegeLoadMgr.SetLODFIGoLoadCost(	hEngine.GetInt(		"lodfi_go_load_cost",	21 ) );
		gSiegeLoadMgr.SetSiegeNodeLoadCost( hEngine.GetInt(		"siege_node_load_cost", 33 ) );
		gSiegeLoadMgr.SetTextureLoadCost(	hEngine.GetInt(		"texture_load_cost",	7 ) );
		gSiegeLoadMgr.SetBaseLoadAmount(	hEngine.GetInt(		"base_load_amount",		100 ) );
		gSiegeLoadMgr.SetGrowLoadAmount(	hEngine.GetFloat(	"grow_load_amount",		0.00002f ) );
	}
	else
	{
		// install some siege stuff
		gSiegeLoadMgr.InstallLoader( siege::LOADER_SIEGE_NODE_TYPE,	new WorldLoader );
		gSiegeLoadMgr.InstallLoader( siege::LOADER_TEXTURE_TYPE,	new WorldLoader );
		gSiegeLoadMgr.InstallLoader( LOADER_GO_TYPE,				new WorldLoader );
		gSiegeLoadMgr.InstallLoader( LOADER_LODFI_TYPE,				new WorldLoader );
	}
	gSiegeEngine.RegisterCollectDoorsCb( makeFunctor( *this, &WorldMap::collectDoors ) );
}


WorldMap :: ~WorldMap( void )
{
	Shutdown();

	// uninstall the loaders
	gSiegeLoadMgr.UninstallLoader( siege::LOADER_SIEGE_NODE_TYPE );
	gSiegeLoadMgr.UninstallLoader( siege::LOADER_TEXTURE_TYPE );
	gSiegeLoadMgr.UninstallLoader( LOADER_GO_TYPE );
	gSiegeLoadMgr.UninstallLoader( LOADER_LODFI_TYPE );
	gSiegeEngine.RegisterCollectDoorsCb( NULL );
}


bool WorldMap :: SSet( const char * sMapName, const char * world, DWORD machineId )
{
	CHECK_SERVER_ONLY;

	// clear out old starting positions when changing maps
	if ( !IsInitialized() || !GetMapName().same_no_case( sMapName ) )
	{
		Server :: PlayerColl::const_iterator iplayer = gServer.GetPlayers().begin();
		for ( ; iplayer != gServer.GetPlayers().end(); ++iplayer )
		{
			if ( !IsValid(*iplayer) || (*iplayer)->IsComputerPlayer() )
			{
				continue;
			}
			(*iplayer)->SetStartingGroup( gpstring() );
		}
	}

	RCSet( sMapName, world, machineId );

	if( IsInitialized() )
	{
		SAssignStartingPositions();
		SVerifyValidPlayerStartingPositions();
		if( gVictory.GetGameType() )
		{
			gVictory.SSetGameType( gVictory.GetGameType()->GetName() );
		}
		return true;
	}
	else
	{
		return false;
	}
}


FuBiCookie WorldMap :: RCSet( const char * name, const char * world, DWORD machineId )
{
	FUBI_RPC_CALL_RETRY( RCSet, machineId );

	gpgeneric( "Server :: RCSetMap\n" );

	// $$ make sure we send ths RPC out asap to avoid major UI lag...
	if( gServer.IsLocal() && gWorld.IsMultiPlayer() )
	{
		gNetFuBiSend.ForceUpdate();
	}

	// $$$ security

	if ( !Init( name, world ) )
	{
		// $$$$ check for map not exist and request a transfer from the server
		gperrorf(( "Failed to set map '%s - %s'.", name, (world && *world) ? world : "<default world>" ));
		return ( RPC_FAILURE_IGNORE );
	}

	WorldMessage( WE_MP_SET_MAP, GOID_INVALID, GOID_ANY ).Send();

	return RPC_SUCCESS;
}


bool WorldMap :: Init( const char* name, const char* world, bool force, bool forLoadingSavedGame )
{
	GPSTATS_SYSTEM( SP_STATIC_CONTENT );

	gpassert( name != NULL );

	// adjust
	if ( world == NULL )
	{
		world = "";
	}

	// $ early bailout if no change (unless forced)
	if ( m_MapName.same_no_case( name ) && m_MpWorldName.same_no_case( world ) && !force )
	{
		// make sure it's already been mounted
		gpassert( gTankMgr.IsMapSet( name ) );
		return ( true );
	}

	// $ mount the map's tank, early bailout if not available
	if ( !gTankMgr.SetMap( name ) )
	{
		// can't find map in resources!
		return ( false );
	}

	// shutdown first to clear out old map if any
	Shutdown( false );

	gpassert( !IsInitialized() );

	// build address
	gpstring mapAddress( "world:maps:" );
	mapAddress += name;

	// $ early bailout if invalid handle
	FastFuelHandle mapDir( mapAddress );
	if ( !mapDir )
	{
		gperror( "WorldMap::Init() - passed in invalid map fuel handle for base!\n" );
		return ( false );
	}

	// $ early bailout if not a directory
	if ( !mapDir.IsDirectory() )
	{
		gperrorf(( "WorldMap::Init() - map at '%s' must be a directory by convention\n", mapAddress.c_str() ));
		return ( false );
	}

	// $ early bailout if no map definition block
	FastFuelHandle mapFuel = mapDir.GetChildNamed( "map" );
	if ( !mapFuel )
	{
		gperrorf(( "WorldMap::Init() - no map definition block exists at '%s:map'!\n", mapAddress.c_str() ));
		return ( false );
	}

	// init local vars from map
	m_MapName             = mapDir.GetName();
	m_ShowIntro           = mapFuel.GetBool( "show_intro", false );
	m_WorldFrustumRadius  = mapFuel.GetFloat( "world_frustum_radius", 45.0 );
	m_WorldFrustumHeight  = mapFuel.GetFloat( "world_frustum_height", 100000.0f );
	m_WorldFrustumDepth	  = mapFuel.GetFloat( "world_frustum_depth",  m_WorldFrustumHeight );
	m_WorldInterestRadius = mapFuel.GetFloat( "world_interest_radius", 25.0 );

	// init time of day
	gpstring timeOfDayStr;
	if ( mapFuel.Get( "timeofday", timeOfDayStr ) )
	{
		TimeInfo ti;
		if ( sscanf( timeOfDayStr, "%dh%dm", &ti.m_Hour, &ti.m_Minute ) == 2 )
		{
			gTimeOfDay.SetTime( ti );
		}
		else
		{
			gperrorf(( "Invalid time of day string '%s' found at fuel address '%s'\n",
					   timeOfDayStr.c_str(), mapFuel.GetAddress().c_str() ));
		}
	}

	m_bInitialized = true;

	// precache the starting positions only
	bool success = loadStartingPositions();

	// init default world
	success = success && SetMpWorld( world, forLoadingSavedGame );

	if( success && !forLoadingSavedGame )
	{
		gVictory.LoadFromMap();
	}

	if( success )
	{
		gSiegeEngine.GetMouseShadow().Init();
	}

	if ( !success )
	{
		// failure, abort!
		Shutdown( false );
	}

	m_bInitialized = success;
	return ( success );
}


void WorldMap :: InitLoaders( void )
{
	GPSTATS_SYSTEM( SP_STATIC_CONTENT );

	gpassert( Services::IsEditor() );

	gSiegeEngine.GetMouseShadow().Init();
	loadNodeMeshInfoDb();
}


bool WorldMap :: Shutdown( bool fullShutdown )
{
	GPSTATS_SYSTEM( SP_STATIC_CONTENT );

	// $ early bailout if nothing to do
	if ( !IsInitialized() )
	{
		return ( true );
	}

	gSiegeEngine.GetMouseShadow().Shutdown();

	// clear out system gunk
	if ( CameraAgent::DoesSingletonExist() )
	{
		gCameraAgent.ClearPosition();
	}
	if ( SiegeEngine::DoesSingletonExist() )
	{
		gSiegeEngine.Shutdown( fullShutdown );
	}
	if ( fullShutdown && TankMgr::DoesSingletonExist() )
	{
		gTankMgr.ClearMap();
	}

	// clear out member gunk
	m_MapName.clear();
	m_ShowIntro = false;
	m_ClampingRegionGUID = REGIONID_INVALID;
	m_WorldFrustumRadius = 0;
	m_WorldInterestRadius = 0;

	// clear out db gunk
	m_IndexesLoaded = false;
	m_MpWorldName.clear();
	m_SGroupColl.destroy();
	m_RegionIdToNameDb.destroy();
	m_NodeGuidToStitchInfoDb.destroy();
	m_NodeGuidToStitchCalcDb.destroy();
	m_NodeGuidToNodeInfoDb.destroy();
	m_ScidColl.destroy();
	m_WorldLocations.clear();

	// clear out dev gunk
#	if !GP_RETAIL
	m_ScidDb.clear();
	m_RottingScidDb.clear();
#	endif // !GP_RETAIL

	m_bInitialized = false;

	return ( true );
}

bool WorldMap :: CheckIndexesLoaded( void )
{
	if ( m_IndexesLoaded )
	{
		return ( true );
	}
	m_IndexesLoaded = true;

	RatioSample sample( "load_map_indexes" );

	// ok load 'em all up
	bool success = true;
	if ( success )
	{
		RatioSample sample( "", 0, 0.05 );
		success = loadCamera();
	}
	if ( success )
	{
		RatioSample sample( "", 0, 0.80 );
		success = loadDatabases();
	}
	if ( success )
	{
		RatioSample sample( "", 0, 0.10 );
		success = loadStitchDb();
	}
	if ( success )
	{
		RatioSample sample( "", 0, 0.05 );
		success = loadHotpoints() && loadWorldLocations();
	}

	if ( success )
	{
		gpassert( !m_RegionIdToNameDb.empty() );

		// check starting positions
		for ( SGroupColl::iterator i = m_SGroupColl.begin() ; i != m_SGroupColl.end() ; )
		{
			for ( SPosMap::iterator j = i->positions.begin() ; j != i->positions.end() ; )
			{
				if ( !ContainsNode( j->second.spos.node ) )
				{
					if ( !IsStreamingRestricted() )
					{
						gperrorf(( "Content error: map '%s' start group '%s', starting position id %d refers to unknown siege node %s. Removed\n",
								   GetMapName().c_str(), i->sGroup.c_str(), j->first, j->second.spos.node.ToString().c_str() ));
					}
					j = i->positions.erase( j );
				}
				else
				{
					++j;
				}
			}

			if ( i->positions.empty() )
			{
				i = m_SGroupColl.erase( i );
			}
			else
			{
				++i;
			}
		}

		// output some stats
		gpgenericf(( "Loaded map databases:\n"
					 "\tRegions       : %d\n"
					 "\tStitch Entries: %d\n"
					 "\tNodes         : %d (%d avg per region)\n"
					 "\tScids         : %d (%d avg per region)\n"
				   , m_RegionIdToNameDb.size()
				   , m_NodeGuidToStitchInfoDb.size()
				   , m_NodeGuidToNodeInfoDb.size()
				   , m_NodeGuidToNodeInfoDb.size() / m_RegionIdToNameDb.size()
				   , m_ScidColl.size()
				   , m_ScidColl.size() / m_RegionIdToNameDb.size()
				   ));
	}
	else
	{
		// failure, abort!
		Shutdown( false );
		success = false;
	}

	// done
	return ( success );
}

bool WorldMap :: Xfer( FuBi::PersistContext& persist )
{
	// persist name
	gpstring mapName = m_MapName;
	persist.Xfer( "m_MapName", mapName );
	gpstring mpWorldName = m_MpWorldName;
	persist.Xfer( "m_MpWorldName", mpWorldName );

	// reinit with new map, force it to init
	if ( persist.IsRestoring() )
	{
		bool success = true;

		{
			RatioSample ratioSample( "map_init", 0, 0.04 );
			success = Init( mapName, mpWorldName, false, true );
		}

		if ( success )
		{
			RatioSample ratioSample( "" );
			bool needsLoad = m_IndexesLoaded;
			success = CheckIndexesLoaded();
			if ( success && needsLoad )
			{
				success = loadHotpoints();
			}
		}

		if ( !success )
		{
			persist.SetFatalError();
			gpreportf( persist.GetReportContext(),
					( "Map '%s' in world '%s' cannot be initialized!\n\tPerhaps .dsmap file missing/out of date?\n",
					  mapName.c_str(), mpWorldName.empty() ? "<default>" : mpWorldName.c_str() ));
			return ( false );
		}
	}

	// persist volatile parameters (these can change on the fly)
	persist.Xfer( "m_WorldFrustumRadius", m_WorldFrustumRadius );
	persist.Xfer( "m_WorldInterestRadius", m_WorldInterestRadius );

	// done
	return ( true );
}


void WorldMap :: SSyncOnMachine( DWORD machineId )
{
	if( IsInitialized() )
	{
		SSet( GetMapName(), GetMpWorldName(), machineId );

		// $$$ jip
		gVictory.RCSetGameType( gVictory.GetGameType()->GetName(), machineId );
		gVictory.SSetTimeLimit( gVictory.GetTimeLimit(), machineId );

		FuBi::SerialBinaryWriter writer;
		FuBi::PersistContext persist( &writer );
		XferForSync( persist );	
		RCSyncOnMachineHelper( machineId, writer.GetBuffer() );
	}
}


FuBiCookie WorldMap :: RCSyncOnMachineHelper( DWORD machineId, const_mem_ptr syncBlock )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSyncOnMachineHelper, machineId );

	FuBi::SerialBinaryReader reader( syncBlock );
	FuBi::PersistContext persist( &reader );
	XferForSync( persist );
	
	return ( RPC_SUCCESS );
}


void WorldMap :: XferForSync( FuBi::PersistContext& persist )
{
	// update this as more stuff needs to be synced			
	SGroupColl::iterator i;
	for ( i = m_SGroupColl.begin(); i != m_SGroupColl.end(); ++i )
	{			
		persist.Xfer( gpstringf( "%d", (*i).id ), (*i).bEnabled );	
	}
}

bool WorldMap :: ReloadContentIndexes( void )
{
	GPSTATS_SYSTEM( SP_STATIC_CONTENT );

	CheckIndexesLoaded();

	// clear old fun
	m_RegionIdToNameDb.destroy();
	m_NodeGuidToStitchInfoDb.destroy();
	m_NodeGuidToNodeInfoDb.destroy();
	m_ScidColl.destroy();
	GPDEV_ONLY( m_ScidDb.clear() );
	GPDEV_ONLY( m_RottingScidDb.clear() );

	// refill the db's
	return ( loadDatabases() && loadStitchDb() );
}

#if !GP_RETAIL

void WorldMap :: OnEditorAddedRegionToMap( RegionId id, const gpstring& regionName )
{
	// do not call this if there are nodes in there!
	gpassert( id != REGIONID_INVALID );
	gpassert( m_NodeGuidToNodeInfoDb.empty() );

	std::pair <RegionIdToNameDb::iterator, bool>
			rc = m_RegionIdToNameDb.insert( std::make_pair( id, regionName ) );
	if ( !rc.second )
	{
		gperrorf(( "Region id %d already defined as '%s', new region named '%s' cannot be added!\n",
				   id, rc.first->second.c_str(), regionName.c_str() ));
	}
}

void WorldMap :: OnEditorAddedNodeToMap( RegionId id, database_guid nodeGuid )
{
	gpassert( id != REGIONID_INVALID );
	std::pair <NodeGuidToNodeInfoDb::iterator, bool>
			rc = m_NodeGuidToNodeInfoDb.insert( std::make_pair( nodeGuid, NodeInfo() ) );
	if ( rc.second )
	{
		// find region
		RegionIdToNameDb::iterator found = m_RegionIdToNameDb.find( id );
		gpassert( found != m_RegionIdToNameDb.end() );

		// set using index
		NodeInfo& info = rc.first->second;
		info.m_RegionIndex = (WORD)(found - m_RegionIdToNameDb.begin());
	}
	else
	{
		gperrorf(( "Node 0x%08x already defined!\n", nodeGuid.GetValue() ));
	}
}

void WorldMap :: SeekAndDestroyDataRot()
{
	// note that this only works when clamping the region, should be edit mode
	gpassert( ::IsWorldEditMode() );
	gpassert( IsStreamingRestricted() );

	// $ this function will loop through all rotting content and delete the gas
	//   block for each rotting go (a go detected to be in a node that is not
	//   in the map). remember to save the fuel if you want those to "take".

	ScidMdb::iterator i, ibegin = m_RottingScidDb.begin(), iend = m_RottingScidDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		gpassert( !ContainsNode( i->second.m_NodeGuid ) );
		gpwarningf(( "Deleting rotten Go, scid=0x%08X, address=%s...\n", i->first, i->second.m_FuelAddress.c_str() ));

		// may have already been deleted, check for existence
		FuelHandle rottenGo( i->second.m_FuelAddress );
		if ( rottenGo )
		{
			rottenGo.Delete();
		}
	}

	// no more
	m_RottingScidDb.clear();
}

#endif // !GP_RETAIL

const gpwstring& WorldMap :: GetMapScreenName( void ) const
{
	return ( gTankMgr.GetMapInfo().m_ScreenName );
}

gpstring WorldMap :: MakeMapDirAddress( void ) const
{
	return ( "world:maps:" + GetMapName() );
}

FastFuelHandle WorldMap :: MakeMapDirFuel( void ) const
{
	return ( FastFuelHandle( MakeMapDirAddress() ) );
}

gpstring WorldMap :: MakeMapDefAddress( void ) const
{
	return ( MakeMapDirAddress() + ":map" );
}

FastFuelHandle WorldMap :: MakeMapDefFuel( void ) const
{
	return ( FastFuelHandle( MakeMapDefAddress() ) );
}

const gpstring& WorldMap :: GetRegionName( RegionId id ) const
{
	ccast <WorldMap*> ( this )->CheckIndexesLoaded();
	gpassert( id != REGIONID_INVALID );

	RegionIdToNameDb::const_iterator found = m_RegionIdToNameDb.find( id );
	return ( (found != m_RegionIdToNameDb.end()) ? found->second : gpstring::EMPTY );
}

const gpstring& WorldMap :: GetRegionNameForNode( database_guid guid ) const
{
	return ( GetRegionName( GetRegionByNode( guid ) ) );
}

bool WorldMap :: ContainsRegion( RegionId id ) const
{
	ccast <WorldMap*> ( this )->CheckIndexesLoaded();
	gpassert( id != REGIONID_INVALID );

	return ( m_RegionIdToNameDb.find( id ) != m_RegionIdToNameDb.end() );
}

RegionId WorldMap :: GetRegionByNode( database_guid guid ) const
{
	ccast <WorldMap*> ( this )->CheckIndexesLoaded();

	NodeGuidToNodeInfoDb::const_iterator found = m_NodeGuidToNodeInfoDb.find( guid );
	if ( found != m_NodeGuidToNodeInfoDb.end() )
	{
		return ( m_RegionIdToNameDb.at( found->second.m_RegionIndex ).first );
	}
	else
	{
		return ( REGIONID_INVALID );
	}
}

gpstring WorldMap :: MakeRegionDirAddress( RegionId id ) const
{
	gpassert( id != REGIONID_INVALID );

	const gpstring& regionName = GetRegionName( id );
	if ( !regionName.empty() )
	{
		return ( MakeMapDirAddress() + ":regions:" + regionName );
	}
	else
	{
		return ( gpstring::EMPTY );
	}
}

FastFuelHandle WorldMap :: MakeRegionDirFuel( RegionId id ) const
{
	gpassert( id != REGIONID_INVALID );
	return ( FastFuelHandle( MakeRegionDirAddress( id ) ) );
}

gpstring WorldMap :: MakeRegionDefAddress( RegionId id ) const
{
	gpassert( id != REGIONID_INVALID );
	return ( MakeRegionDirAddress( id ) + ":region" );
}

FastFuelHandle WorldMap :: MakeRegionDefFuel( RegionId id ) const
{
	gpassert( id != REGIONID_INVALID );
	return ( FastFuelHandle( MakeRegionDefAddress( id ) ) );
}

int WorldMap :: GetAllRegionsDirFuel( FastFuelHandleColl& coll ) const
{
	// get regions
	int size = GetAllRegionsDefFuel( coll );

	// now convert each to its parent dir
	for ( FastFuelHandleColl::iterator i = coll.end() - size ; i != coll.end() ; )
	{
		FastFuelHandle parent = i->GetParent();
		if ( parent.IsDirectory() )
		{
			// convert to parent and advance
			*i = parent;
			 ++i;
		}
		else
		{
			gperrorf(( "Region '%s' at address '%s' must be a directory by convention",
					   parent.GetName(), parent.GetAddress().c_str() ));

			// nuke it
			i = coll.erase( i );
			--size;
		}
	}

	return ( size );
}

int WorldMap :: GetAllRegionsDefFuel( FastFuelHandleColl& coll ) const
{
	// get map fuel
	FastFuelHandle mapFuel( MakeMapDirFuel() );
	gpassert( mapFuel );

	// get vars
	int oldSize = coll.size();
	FastFuelHandle regionFuel( mapFuel.GetChildNamed( "regions" ) );

	// get children if there are regions
	if ( regionFuel )
	{
		regionFuel.ListChildrenTyped( coll, "region", 2 );
	}

	// done
	return ( coll.size() - oldSize );
}

gpstring WorldMap :: MakeQuestsAddress( void ) const
{
	return ( MakeMapDirAddress() + ":quests:quests" );
}

gpstring WorldMap :: MakeChaptersAddress( void ) const
{
	return ( MakeMapDirAddress() + ":quests:chapters" );
}

FastFuelHandle WorldMap :: MakeQuestsFuel( void ) const
{
	return ( FastFuelHandle( MakeQuestsAddress() ) );
}

FastFuelHandle WorldMap :: MakeChaptersFuel( void ) const
{
	return ( FastFuelHandle( MakeChaptersAddress() ) );
}

int WorldMap :: GetAllQuestsFuel( FastFuelHandleColl& coll ) const
{
	int oldSize = coll.size();

	FastFuelHandle quests( MakeQuestsFuel() );
	if ( quests )
	{
		quests.ListChildren( coll );
	}

	return ( coll.size() - oldSize );
}

int	WorldMap :: GetAllChaptersFuel( FastFuelHandleColl& coll ) const
{
	int oldSize = coll.size();
	
	FastFuelHandle chapters( MakeChaptersFuel() );
	if ( chapters )
	{
		chapters.ListChildren( coll );
	}

	return ( coll.size() - oldSize );
}

gpstring WorldMap :: MakeConversationAddress( RegionId id, const char* convoName ) const
{
	gpassert( id != REGIONID_INVALID );
	return ( MakeRegionDirAddress( id ) + ":conversations:" + convoName );
}

FastFuelHandle WorldMap :: MakeConversationFuel( RegionId id, const char* convoName ) const
{
	gpassert( id != REGIONID_INVALID );
	return ( FastFuelHandle( MakeConversationAddress( id, convoName ) ) );
}

gpstring WorldMap :: MakeBookmarksAddress( void ) const
{
	return ( MakeMapDirAddress() + ":info:bookmarks" );
}

FastFuelHandle WorldMap :: MakeBookmarksFuel( void ) const
{
	return ( FastFuelHandle( MakeBookmarksAddress() ) );
}

bool WorldMap :: LoadBookmark( const gpstring & name, SiegePos & pos, CameraPosition & cameraPos ) const
{
	bool bSuccess = false;
	pos = SiegePos();

	if ( name.empty() || ((name.length() > 2) && (name[ 1 ] == ':')) )
	{
		bool bOnlyNodeGiven = false;

		if ( name.empty() || (tolower( name[ 0 ] ) == 's') )
		{
			// load scid if 0x format
			if ( !name.empty() && (name[ 2 ] == '0') && (tolower( name[ 3 ] ) == 'x') )
			{
#				if !GP_RETAIL

				ccast <WorldMap*> ( this )->CheckIndexesLoaded();

				// build scid
				Scid scid = SCID_INVALID;
				FromString( name.c_str() + 2, scid );

				// look up scid
				ScidDb::const_iterator found = m_ScidDb.find( scid );
				if ( found != m_ScidDb.end() )
				{
					pos.node = found->second.m_NodeGuid;
					bOnlyNodeGiven = true;
					bSuccess = true;
				}
				else
				{
					gpwarningf(( "Scid not found! Bookmark request = '%s'\n", name.c_str() ));
				}

#				endif // !GP_RETAIL
			}
			else
			{
				// load starting position - default to 1 if none given
				int numpos = 0;
				if ( !name.empty() )
				{
					FromString( name.c_str() + 2, numpos );
				}
				else
				{
					numpos = 1;
				}

				// go there
				StartingPos SPos;
				if ( GetDefaultStartingPosition( SPos, numpos ) )
				{
					pos = SPos.spos;
					cameraPos = SPos.cpos;
					bSuccess = true;
				}
				else
				{
					gpwarningf(( "Starting position not found! Bookmark request = '%s'\n", name.c_str() ));
				}
			}
		}
		else if ( tolower( name[ 0 ] ) == 'n' )
		{
			// parse node location
			if ( (name[ 2 ] == '0') && (tolower( name[ 3 ] ) == 'x') )
			{
				bSuccess = pos.node.FromString( name.c_str() + 2 );
			}
			else
			{
				bSuccess = ::FromString( name.c_str() + 2, pos );
			}

			if ( bSuccess )
			{
				bOnlyNodeGiven = true;
			}
			else
			{
				gpwarning( "That's not a node guid!\n" );
			}
		}
		else
		{
			gpwarning( "Unrecognized bookmark command.\n" );
		}

		if ( bSuccess && bOnlyNodeGiven )
		{
			// load node
			if ( ContainsNode( pos.node ) )
			{
				cameraPos.targetPos = pos;
				cameraPos.cameraPos = pos;

				// build a generic camera position
				float d						 = 10.0f * cosf( RadiansFrom( 45 ) );
				cameraPos.cameraPos.pos.x	+= d * sinf( 0.0f );
				cameraPos.cameraPos.pos.y	+= 10.0f * sinf( RadiansFrom( 45 ) );
				cameraPos.cameraPos.pos.z	+= d * cosf( 0.0f );
			}
			else
			{
				gpwarningf(( "Sorry, that node isn't in the current map! Bookmark request = '%s'\n", name.c_str() ));
				bSuccess = false;
			}
		}
	}
	else
	{
		FastFuelHandle hBookmarks( MakeBookmarksFuel() );
		if( hBookmarks )
		{
			FastFuelHandle hBookmark = hBookmarks.GetChildNamed( name );
			if( hBookmark )
			{
				if( LoadBookmark( hBookmark, pos, cameraPos ) )
				{
					bSuccess = true;
				}
				else
				{
					gpwarning( "Loading of bookmark failed.\n" );
				}
			}
			else
			{
				gpwarning( "Bookmark not found. Check name spelling.\n" );
			}
		}
	}
	return ( bSuccess );
}

bool WorldMap :: LoadBookmark( FastFuelHandle hBookmark, SiegePos& pos, CameraPosition& cameraPos ) const
{
	Quat orient;

	/////////////////////////////////////////////
	//	load position

	gpstring sPos;

	if ( !hBookmark.Get( "position", sPos ) )
	{
		gperror( "Failed to load bookmark position.\n" );
		return false;
	}

	::FromString( sPos, pos );


	/////////////////////////////////////////////
	//	validate position

	gpstring address;

	if ( !ContainsNode( pos.node ) )
	{
		// failed validation... node doestn't exist...
		return false;
	}


	/////////////////////////////////////////////
	//	load camera position

	FastFuelHandle camBlock = hBookmark.GetChildNamed( "camera" );

	if ( camBlock.IsValid() )
	{
		CameraEulerPosition eulerPos;
		eulerPos.targetPos = pos;
		camBlock.Get( "orbit", eulerPos.orbitAngle );
		camBlock.Get( "azimuth", eulerPos.azimuthAngle );
		camBlock.Get( "distance", eulerPos.distance );

		// Build the camera position
		eulerPos.MakeCameraPosition( cameraPos );
	}
	else
	{
		// Setup the new position
		cameraPos.targetPos			= pos;
		cameraPos.cameraPos			= pos;

		// Build the camera position using the angles
		float d						= 10.0f * cosf( RadiansFrom( 80 ) );
		cameraPos.cameraPos.pos.x	+= d * sinf( 0.0f );
		cameraPos.cameraPos.pos.y	+= 10.0f * sinf( RadiansFrom( 80 ) );
		cameraPos.cameraPos.pos.z	+= d * cosf( 0.0f );
	}

	return true;
}

#if !GP_RETAIL

bool WorldMap :: DumpBookmarks( ReportSys::ContextRef ctx ) const
{
	return ( DumpBookmarks( "", ctx ) );
}

bool WorldMap :: DumpBookmarks( const char* pattern, ReportSys::ContextRef ctx ) const
{
	if ( (pattern == NULL) || (*pattern == '\0') )
	{
		pattern = "*";
	}

	FastFuelHandle hbookmarkFuel( MakeBookmarksFuel() );

	FastFuelHandleColl bookmarks;

	if ( !hbookmarkFuel || !hbookmarkFuel.ListChildrenTyped( bookmarks, "bookmark" ) )
	{
		gpwarning( "Hmm, couldn't find me any bookmarks for this map...\n" );
		return ( false );
	}

	// dump objects into sink
	ReportSys::TextTableSink sink;
	ReportSys::LocalContext localCtx( &sink, false );
	localCtx.SetSchema( new ReportSys::Schema( 3, "valid", "name", "description" ), true );

	FastFuelHandleColl::iterator i;
	for ( i = bookmarks.begin(); i != bookmarks.end() ; ++i )
	{
		if ( stringtool::IsDosWildcardMatch( pattern, (*i).GetName() ) )
		{
			ReportSys::AutoReport autoReport( localCtx );

			// validity
			SiegePos pos;
			CameraPosition cameraPos;
			bool valid = LoadBookmark( (*i), pos, cameraPos );
			localCtx.OutputField( valid ? "OK" : "INVALID" );

			// name
			localCtx.OutputField( (*i).GetName() );

			// node
			localCtx.OutputFieldObj( (*i).GetString( "description" ) );
		}
	}

	// out header
	ctx->Output( "\n" );
	int width = sink.CalcDividerWidth();
	if ( width != 0 )
	{
		ctx->OutputRepeatWidth( width, "-==- " );
		ctx->Output( "\n\nBookmarks:\n\n" );

		// out table
		sink.Sort( 1 );
		sink.OutputReport( ctx );
	}
	else
	{
		gpwarning( "No bookmarks matching that pattern exist for this map...\n" );
		return ( false );
	}

	return ( true );
}

#endif // !GP_RETAIL

bool WorldMap :: ContainsNode( database_guid guid ) const
{
	ccast <WorldMap*> ( this )->CheckIndexesLoaded();

	if ( IsStreamingRestricted() )
	{
		return ( GetRegionByNode( guid ) == m_ClampingRegionGUID );
	}
	else
	{
		return ( m_NodeGuidToNodeInfoDb.find( guid ) != m_NodeGuidToNodeInfoDb.end() );
	}
}

gpstring WorldMap :: MakeNodeAddress( database_guid guid ) const
{
	ccast <WorldMap*> ( this )->CheckIndexesLoaded();

	gpstring address;

	NodeGuidToNodeInfoDb::const_iterator found = m_NodeGuidToNodeInfoDb.find( guid );
	if ( found != m_NodeGuidToNodeInfoDb.end() )
	{
		address = MakeMapDirAddress();
		address += ":regions:";
		address += m_RegionIdToNameDb.at( found->second.m_RegionIndex ).second;
		address += ":terrain_nodes:siege_node_list:";
		address.appendf( "0x%08X", guid.GetValue() );
	}

	return ( address );
}

FastFuelHandle WorldMap :: MakeNodeFuel( database_guid guid ) const
{
	return ( FastFuelHandle( MakeNodeAddress( guid ) ) );
}

bool WorldMap :: ContainsScid( Scid scid ) const
{
#	if GP_RETAIL
	return ( ::IsValid( scid, false ) );
#	else // GP_RETAIL

	if ( scid == SCID_INVALID )
	{
		return ( false );
	}
	
	ccast <WorldMap*> ( this )->CheckIndexesLoaded();
	if ( m_ScidDb.find( scid ) != m_ScidDb.end() )
	{
		return ( true );
	}

	// special test if we're the editor, to permit cross-region scid linking.
	// 'cause SE only loads one region at a time...
	if ( Services::IsEditor() )
	{
		// ok iterate over all our objects and see if any matches
		RegionIdToNameDb::const_iterator i, ibegin = m_RegionIdToNameDb.begin(), iend = m_RegionIdToNameDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gpstring address;
			address.assignf( "%s:objects:0x%08X", MakeRegionDirAddress( i->first ).c_str(), scid );
			FastFuelHandle fuel( address );
			if ( fuel )
			{
				// found one!
				return ( true );
			}
		}
	}

	return ( false );
#	endif // GP_RETAIL
}

FastFuelHandle WorldMap :: MakeObjectFuel( Scid scid, RegionId regionId, const SiegeGuid& forNodeGuid, bool errorOnFail ) const
{
	FastFuelHandle fuel( MakeRegionDirAddress( regionId ) + ":objects" );
	if ( fuel )
	{
		if ( !m_MpWorldName.empty() )
		{
			FastFuelHandle world = fuel.GetChildNamed( m_MpWorldName );
			if ( world )
			{
				fuel = world;
			}
		}

		fuel = fuel.GetChildNamed( gpstringf( "0x%08X", scid ) );
	}

#	if !GP_RETAIL
	if ( errorOnFail && !fuel )
	{
		gpstring regionName = GetRegionName( regionId );
		if ( regionName.empty() )
		{
			regionName = "<REGION NOT FOUND!>";
		}

		gpstring worldName = m_MpWorldName;
		if ( !worldName.empty() )
		{
			worldName += ':';
		}

		gperrorf(( "CONTENT ERROR: this will probably result in a game object "
				   "failing to be added to the world. Most likely cause is an "
				   "out of sync gas file or perhaps a messed up scid index. "
				   "Anyway bottom line is I can't resolve the fuel address! "
				   "Here's some more info:\n"
				   "\n"
				   "Scid         = 0x%08x\n"
				   "Region ID    = 0x%08X\n"
				   "Region name  = %s\n"
				   "Node GUID    = %s\n"
				   "Fuel address = %s:regions:%s:objects:%s0x%08X\n"
				   "\n"
				   "To debug this issue, see why there isn't anything at that "
				   "particular fuel address. In the meantime I'm just going to "
				   "skip loading this game object, 'cause I can't find it!",
				   scid,
				   regionId,
				   regionName.c_str(),
				   forNodeGuid.ToString().c_str(),
				   MakeMapDirAddress().c_str(), regionName.c_str(),
				   worldName.c_str(),
				   scid ));
	}
#	endif // !GP_RETAIL

	return ( fuel );
}

void WorldMap :: GetMpWorlds( MpWorldColl& coll )
{
	// init the worlds
	FastFuelHandle worldsFuel( MakeMapDefAddress() + ":worlds" );
	if ( worldsFuel )
	{
		coll.Load( worldsFuel );
	}
}

bool WorldMap :: HasMpWorld( const char* mpWorldName )
{
	// empty name means choose default (lowest required level)
	if ( *mpWorldName == '\0' )
	{
		return ( true );
	}

	// check to see if it's there
	MpWorldColl coll;
	GetMpWorlds( coll );
	MpWorldColl::const_iterator i, ibegin = coll.begin(), iend = coll.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->m_Name.same_no_case( mpWorldName ) )
		{
			return ( true );
		}
	}

	// guess not
	return ( false );
}

bool WorldMap :: SetMpWorld( const gpstring& mpWorldName, bool forLoadingSavedGame )
{
	gpassert( IsInitialized() );
	gpassert( !m_IndexesLoaded );

	// none specified? get the default and set that instead
	if ( mpWorldName.empty() )
	{
		MpWorldColl coll;
		GetMpWorlds( coll );
		m_MpWorldName = coll.GetDefaultMpWorldName();
		return ( true );
	}

	// make sure it exists!
	if ( !HasMpWorld( mpWorldName ) )
	{
		gperrorf(( "MP World '%s' does not exist in map '%s'!\n", mpWorldName.c_str(), GetMapName().c_str() ));
		return ( false );
	}

	// set it
	m_MpWorldName = mpWorldName;
	if ( !forLoadingSavedGame )
	{
		WorldMessage( WE_MP_SET_MAP_WORLD, GOID_INVALID, GOID_ANY ).Send();
	}

	return ( true );
}

bool WorldMap :: SSetMpWorld( const gpstring& mpWorldName, DWORD machineId )
{
	CHECK_SERVER_ONLY;

	if ( !SetMpWorld( mpWorldName ) )
	{
		return ( false );
	}

	RCSetMpWorld( mpWorldName, machineId );
	return ( true );
}

FuBiCookie WorldMap :: RCSetMpWorld( const gpstring& mpWorldName, DWORD machineId )
{
	FUBI_RPC_CALL_RETRY( RCSetMpWorld, machineId );
	return ( SetMpWorld( mpWorldName ) ? RPC_SUCCESS : RPC_FAILURE_IGNORE );
}

void WorldMap :: SAssignStartingPositions()
{
	CHECK_SERVER_ONLY;

	if( !IsInitialized() )
	{
		return;
	}

	gpgeneric( "WorldMap::AssignStartingPositions - assigning.\n" );

	// see if we wanted to teleport, gather info
	bool hasTeleport = false;
	SiegePos teleportPos;
	CameraPosition cameraPos;
	gpstring teleport;
	if ( ::IsSinglePlayer() )
	{
		teleport = gConfig.GetString( "teleport" );
		if ( !teleport.empty() )
		{
			hasTeleport = LoadBookmark( teleport, teleportPos, cameraPos );
		}
	}

	m_PositionsAssigned = 0;

	// assign starting positions
	SGroupColl::iterator i;
	for ( i = m_SGroupColl.begin(); i != m_SGroupColl.end(); ++i )
	{
		SPosMap::iterator ipos = (*i).positions.begin();
		Server::PlayerColl::const_iterator iplayer = gServer.GetPlayers().begin();

		if( m_PositionsAssigned == gServer.GetNumHumanPlayers() )
		{
			break;
		}

		for ( ; iplayer != gServer.GetPlayers().end() && ipos != (*i).positions.end() ; ++iplayer )
		{
			if ( !IsValid(*iplayer) || (*iplayer)->IsComputerPlayer() )
			{
				continue;
			}

			StartingPos pos;

			if ( hasTeleport )
			{
				pos.spos = teleportPos;
				pos.cpos = cameraPos;

				gpgenericf(( "Assigned teleport position %s to player %S, playerid %d\n",
							 teleport.c_str(),
							 (*iplayer)->GetName().c_str(),
							 (*iplayer)->GetId() ));
			}
			else if ( (*iplayer)->GetStartingGroup().same_no_case( (*i).sGroup ) || ((*iplayer)->GetStartingGroup().empty() && (*i).bDefault ) )
			{
				pos = ipos->second;

				gpgenericf(( "Assigned starting position id %d group: %s to player %S, playerid %d\n",
							 (*ipos).first,
							 (*i).sGroup.c_str(),
							 (*iplayer)->GetName().c_str(),
							 (*iplayer)->GetId() ));
			}
			else
			{
				continue;
			}

			(*iplayer)->RCSetStartingGroup( (*i).sGroup );
			(*iplayer)->SSetStartingPosition( pos.spos, pos.cpos );
			++m_PositionsAssigned;
			++ipos;
		}
	}

	if( ( gServer.GetNumHumanPlayers() > 0 ) && ( m_PositionsAssigned != gServer.GetNumHumanPlayers() ) )
	{
		if ( !IsStreamingRestricted() )
		{
			gperror( "Server::AssignStartingPositions - there are not enough starting positions on this map to support current number of players.\n" );
		}
		return;
	}
}


void WorldMap :: SVerifyValidPlayerStartingPositions()
{
	Server::PlayerColl::const_iterator i;
	for ( i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
	{
		if ( !IsValid(*i) || (*i)->IsComputerPlayer() )
		{
			continue;
		}

		if ( (*i)->GetHeroUberLevel() < GetRequiredStartingPositionLevel( (*i)->GetStartingGroup() ) )
		{
			gpstring sDefault;
			GetDefaultStartingGroup( sDefault );
			(*i)->RSSetStartingGroup( sDefault );
		}
	}
}

float WorldMap :: GetRequiredStartingPositionLevel( const gpstring & startPosition ) const
{
	SGroupColl::const_iterator i;
	for ( i = m_SGroupColl.begin(); i != m_SGroupColl.end(); ++i )
	{
		if ( startPosition.same_no_case( (*i).sGroup ) )
		{
			WorldMap::WorldStartLevels::const_iterator iLevel;
			for ( iLevel = (*i).startLevels.begin(); iLevel != (*i).startLevels.end(); ++iLevel )
			{
				if ( (*iLevel).sWorld.same_no_case( GetMpWorldName() ) )
				{
					return (*iLevel).requiredLevel;
				}
			}
		}
	}

	return 0.0f;
}

bool WorldMap :: GetDefaultStartingGroup( gpstring& groupName ) const
{
	SGroupColl::const_iterator i;
	for ( i = m_SGroupColl.begin(); i != m_SGroupColl.end(); ++i )
	{
		if ( ((*i).bDefault && (*i).bEnabled) || (m_SGroupColl.size() == 1) )
		{
			groupName = (*i).sGroup;
			return true;
		}
	}

	return false;
}

bool WorldMap :: GetDefaultStartingPosition( StartingPos& startingPos, int id ) const
{
	SGroupColl::const_iterator i;
	for ( i = m_SGroupColl.begin(); i != m_SGroupColl.end(); ++i )
	{
		if ( (*i).bDefault || (m_SGroupColl.size() == 1) )
		{
			SPosMap::const_iterator iPos = (*i).positions.find( id );
			if ( iPos != (*i).positions.end() )
			{
				startingPos = (*iPos).second;
				return true;
			}
		}
	}

	return false;
}

bool WorldMap :: GetRandomStartingPosition( const char* groupName, SiegePos& pos, CameraPosition& cameraPos ) const
{
	SGroupColl::const_iterator i;
	for ( i = m_SGroupColl.begin(); i != m_SGroupColl.end(); ++i )
	{
		if ( (*i).sGroup.same_no_case( groupName ) && (*i).bEnabled )
		{
			SPosMap::const_iterator iPos;
			int id = Random( 1, (int)(*i).positions.size()-1 );
			iPos = (*i).positions.find( id );
			if ( iPos != (*i).positions.end() )
			{
				pos = (*iPos).second.spos;
				cameraPos = (*iPos).second.cpos;
				return true;
			}
			else
			{
				gperrorf(( "There are %d starting positions for group %s, but id %d does not exist. This content needs to be added.",
							(*i).positions.size(), groupName, id ));
				return false;
			}
		}
	}

	return false;
}

void WorldMap :: SEnableStartGroup( int id, bool bEnabled )
{
	CHECK_SERVER_ONLY;
		
	RCEnableStartGroup( id, bEnabled );
}

void WorldMap :: RCEnableStartGroup( int id, bool bEnabled )
{
	FUBI_RPC_THIS_CALL( RCEnableStartGroup, RPC_TO_ALL );

	SGroupColl::iterator i;
	for ( i = m_SGroupColl.begin(); i != m_SGroupColl.end(); ++i )
	{		
		if ( (*i).id == id )
		{
			(*i).bEnabled = bEnabled;
			return;
		}
	}	
}

void WorldMap :: SSetDefaultStartGroup( int id, bool bSet )
{
	CHECK_SERVER_ONLY;

	RCSetDefaultStartGroup( id, bSet );
}


void WorldMap :: RCSetDefaultStartGroup( int id, bool bSet )
{
	FUBI_RPC_THIS_CALL( RCSetDefaultStartGroup, RPC_TO_ALL );

	SGroupColl::iterator i;
	for ( i = m_SGroupColl.begin(); i != m_SGroupColl.end(); ++i )
	{		
		if ( (*i).id == id )
		{
			(*i).bDefault = bSet;
			return;
		}
	}
}


unsigned int WorldMap :: GetWorldLocationId( const gpstring& name ) const
{
	WorldLocationMap::const_iterator i;
	for ( i = m_WorldLocations.begin(); i != m_WorldLocations.end(); ++i )
	{
		if ( i->second.sName.same_no_case( name ) )
		{
			return ( i->first );
		}
	}
	return ( 0 );
}

const gpwstring& WorldMap :: GetWorldLocationScreenName( unsigned int id ) const
{
	WorldLocationMap::const_iterator findLocation = m_WorldLocations.find( id );
	if ( findLocation != m_WorldLocations.end() )
	{
		return ( findLocation->second.sScreenName );
	}
	return gpwstring::EMPTY;
}

bool WorldMap :: PreloadNode( database_guid nodeGuid )
{
	GPSTATS_SYSTEM( SP_LOAD_TERRAIN );
	CheckIndexesLoaded();

	// Make sure we are really dealing with a node that is not already loaded
	if ( gSiegeEngine.DoesNodeExist( nodeGuid ) )
	{
		return false;
	}

	// Look up node info
	NodeGuidToNodeInfoDb::const_iterator found = m_NodeGuidToNodeInfoDb.find( nodeGuid );
	if ( found == m_NodeGuidToNodeInfoDb.end() )
	{
		gperrorf(( "You're asking to load a SiegeNode (guid=%s) that isn't part of the map!", nodeGuid.ToString().c_str() ));
		return false;
	}
	const NodeInfo& nodeInfo = found->second;

	// Look up region info
	const std::pair <RegionId, gpstring> & regionInfo = m_RegionIdToNameDb.at( nodeInfo.m_RegionIndex );

	// Get a handle to the node
	SiegeNode* pNode			= NULL;
	{
		// Hold the node critical while I build the handle
		kerneltool::Critical::Lock nodeLock( gSiegeEngine.GetNodeCreateCritical() );

		SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( nodeGuid );
		pNode					= &handle.CreateObject( gSiegeEngine.NodeCache() );
	}

	// Add location path
	gpstring path( MakeMapDirAddress() );
	path += ":regions:";
	path += regionInfo.second;
	path = FuelAddressToFilePath( path );
	gpassert( !path.empty() );
	path += ( "terrain_nodes\\siege_nodes.lnc" );

	// Set the file in the node
	pNode->SetFileHandle( gSiegeEngine.NodeDatabase().ReferenceLogicalFile( path, nodeGuid ) );

	// Set the GUID
	gpassert( !pNode->IsLoadedPhysical() );
	pNode->SetGUID( nodeGuid );

	// Find the node's gas block
	FastFuelHandle hNode( MakeNodeFuel( nodeGuid ) );
	if ( !hNode.IsValid() )
	{
		gpfatalf(( "Failed to load siege node %s because we couldn't create a FastFuelHandle to it.\n", nodeGuid.ToString().c_str() ));
		return false;
	}

	// Set fading info etc
	pNode->SetRegionID( (int)regionInfo.first );
	pNode->SetSection( hNode.GetInt( "nodesection", -1 ) );
	pNode->SetLevel( hNode.GetInt( "nodelevel", -1 ) );
	pNode->SetObject( hNode.GetInt( "nodeobject", -1 ) );

	// Get light information
	pNode->SetOccludesLight( hNode.GetBool( "occludes_light", true ) );

	// Get camera information
	pNode->SetOccludesCamera( hNode.GetBool( "occludes_camera", true ) );
	pNode->SetBoundsCamera( hNode.GetBool( "bounds_camera", true ) );
	pNode->SetCameraFade( hNode.GetBool( "camera_fade", false ) );
	gSiegeEngine.GetNodeFlags( pNode );

	// Get the region block
	const FastFuelHandle hRegionDefinition = hNode.GetParent();

	float ambient	= 0.0f;
	DWORD color		= 0xFFFFFFFF;
	if ( hRegionDefinition.Get( "ambient_intensity", ambient ) )
	{
		if ( hRegionDefinition.Get( "ambient_color", color ) )
		{
			pNode->SetAmbientColor( fScaleDWORDColor( color, ambient ) );
		}
		else
		{
			pNode->SetAmbientColor( fScaleDWORDColor( 0xFFFFFFFF, ambient ) );
		}
	}
	if ( hRegionDefinition.Get( "object_ambient_intensity", ambient ) )
	{
		if ( hRegionDefinition.Get( "object_ambient_color", color ) )
		{
			pNode->SetObjectAmbientColor( fScaleDWORDColor( color, ambient ) );
		}
		else
		{
			pNode->SetObjectAmbientColor( fScaleDWORDColor( 0xFFFFFFFF, ambient ) );
		}
	}

	// Get the actor intensity if it exists, otherwise just leave the ambient value as the object ambience
	hRegionDefinition.Get( "actor_ambient_intensity", ambient );
	if ( hRegionDefinition.Get( "actor_ambient_color", color ) )
	{
		pNode->SetActorAmbientColor( fScaleDWORDColor( color, ambient ) );
	}
	else
	{
		pNode->SetActorAmbientColor( fScaleDWORDColor( 0xFFFFFFFF, ambient ) );
	}

	if( !gSiegeEngine.RebuildDoors( nodeGuid ) )
	{
		// Load door information
		FastFuelHandleColl doors;
		hNode.ListChildrenNamed( doors, "door*" );
		for( FastFuelHandleColl::iterator iDoor = doors.begin(); iDoor != doors.end(); ++iDoor )
		{
			// Add this door, sans orientation and positional information
			pNode->AddDoor( (*iDoor).GetInt( "id" ), database_guid( (*iDoor).GetInt( "farguid" ) ), (*iDoor).GetInt( "fardoor" ), false );
		}

		// Load for regional door information
		if ( !IsStreamingRestricted() )
		{
			// Get all the nodes that request a regional connection with me (may not be any)
			std::pair <NodeGuidToStitchInfoDb::const_iterator, NodeGuidToStitchInfoDb::const_iterator>
					rc = m_NodeGuidToStitchInfoDb.equal_range( nodeGuid );

			// See if we need to manually connect or if the proper stitch info exists in our db
			bool bManualConnect	= true;
			if( (rc.first != rc.second) &&
				(m_NodeGuidToStitchCalcDb.find( nodeGuid ) != m_NodeGuidToStitchCalcDb.end()) )
			{
				bManualConnect = false;
				pNode->SetRequestAutoStitch( true );
			}

			for ( ; rc.first != rc.second; ++rc.first )
			{
				const StitchInfo& info = rc.first->second;
				gpassert( rc.first->first == nodeGuid );

				// Add this door, sans orientation and positional information
				pNode->AddDoor( info.MeDoor, info.NeighborGUID, info.NeighborDoor, bManualConnect );
			}
		}
	}
	else
	{
		pNode->RequestHotpointUpdate( true );
		if( m_NodeGuidToStitchCalcDb.find( nodeGuid ) != m_NodeGuidToStitchCalcDb.end() )
		{
			pNode->SetRequestAutoStitch( true );
		}
	}

	return true;
}

bool WorldMap :: LoadNode( database_guid nodeGuid )
{
	GPSTATS_SYSTEM( SP_LOAD_TERRAIN );
	CheckIndexesLoaded();

	gpassert( nodeGuid.IsValid() )

	// Get the node - make sure it exists
	SiegeNode* pNode = NULL;
	{
		// Hold criticals to protect against node destruction/creation during lookup
		kerneltool::Critical::Lock destroyLock( gSiegeEngine.GetNodeDestroyCritical() );
		kerneltool::Critical::Lock nodeLock( gSiegeEngine.GetNodeCreateCritical() );

		pNode = gSiegeEngine.NodeCache().GetValidObjectInCache( nodeGuid );
	}

	if( pNode == NULL )
	{
		return ( false );
	}
	SiegeNode& node	= *pNode;

	// Make sure we haven't already done this
	if( node.IsLoadedPhysical() )
	{
		return true;
	}

	// Load the node's LNC data
	SiegeNodeIO::Read( node, nodeGuid, true );
	FastFuelHandle hNode( MakeNodeFuel( nodeGuid ) );
	if_not_gpassert( hNode.IsValid() )
	{
		return false;
	}

	// Get the region block
	const FastFuelHandle hRegionDefinition = hNode.GetParent();

	// Load the environment map texture for this node if there is one
	gpstring envmapname, fullenvmapname;
	if( hRegionDefinition.Get( "environment_map", envmapname ) &&
		gNamingKey.BuildIMGLocation( envmapname.c_str(), fullenvmapname ) )
	{
		unsigned int envMap = gDefaultRapi.CreateTexture( fullenvmapname, true, 1, TEX_LOCKED );
		if( envMap )
		{
			node.SetEnvironmentMap( envMap );
		}
		else
		{
			gDefaultRapi.AddTextureReference( gSiegeEngine.GetEnvironmentTexture() );
			node.SetEnvironmentMap( gSiegeEngine.GetEnvironmentTexture() );
		}
	}
	else
	{
		gDefaultRapi.AddTextureReference( gSiegeEngine.GetEnvironmentTexture() );
		node.SetEnvironmentMap( gSiegeEngine.GetEnvironmentTexture() );
	}

	// Grab the transition critical
	gSiegeEngine.GetTransitionCritical().Enter();

	// Load node mesh
	if( node.GetMeshGUID() == UNDEFINED_GUID )
	{
		// Get the mesh GUID
		database_guid MeshGUID( hNode.GetInt( "mesh_guid" ) );

		// Get the texture set (if there is one)
		node.SetTextureSetAbbr( hNode.GetString( "texsetabbr" ) );
		node.SetTextureSetVersion( hNode.GetString( "texsetver" ) );

		// Set the mesh
		node.SetMeshGUID( MeshGUID, false );
	}

	// Get the mesh from the node
	const SiegeMesh& bmesh = node.GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );

	// Go through each door of the node and set its proper orientation and position
	const SiegeMesh::SiegeMeshDoorList& mesh_doors	= bmesh.GetDoors();
	for( SiegeMesh::SiegeMeshDoorList::const_iterator d = mesh_doors.begin(); d != mesh_doors.end(); ++d )
	{
		// Get the door in the node
		SiegeNodeDoor* pDoor	= node.GetDoorByID( (*d)->GetID() );
		if( pDoor )
		{
			// Set door information
			pDoor->SetCenter( (*d)->GetCenter() );
			pDoor->SetOrient( (*d)->GetOrientation() );
		}
	}

	// Auto stitch
	if( node.IsRequestAutoStitch() )
	{
		// Auto stitch the node
		autoStitchNode( node );
		node.SetRequestAutoStitch( false );
	}

	// Process disconnects
	{
		// Hold criticals to protect against node destruction/creation during connection
		kerneltool::Critical::Lock destroyLock( gSiegeEngine.GetNodeDestroyCritical() );
		kerneltool::Critical::Lock nodeLock( gSiegeEngine.GetNodeCreateCritical() );

		gSiegeEngine.ProcessDisconnects( nodeGuid );
	}

	// Go through existing doors and look for connections
	const SiegeNodeDoorList& dl	= node.GetDoors();
	for( SiegeNodeDoorConstIter nd = dl.begin(); nd != dl.end(); ++nd )
	{
		// See if this door needs a manual connection
		if( (*nd)->GetConnection() )
		{
			// Hold the node destruction critical
			kerneltool::Critical::Lock destroyLock( gSiegeEngine.GetNodeDestroyCritical() );

			// See if this node has been properly loaded
			SiegeNode* pFarNode	= NULL;
			{
				// Hold the node create critical
				kerneltool::Critical::Lock nodeLock( gSiegeEngine.GetNodeCreateCritical() );

				if( (*nd)->GetNeighbour().IsValid() )
				{
					// Attempt to get the node
					pFarNode = gSiegeEngine.NodeCache().GetValidObjectInCache( (*nd)->GetNeighbour() );
					if( pFarNode && !pFarNode->IsLoadedPhysical() )
					{
						pFarNode = NULL;
					}
				}
			}
			if( pFarNode )
			{
				// Make a node connection request
				gSiegeEngine.ConnectSiegeNodes( node.GetGUID(), pFarNode->GetGUID() );
			}
		}
	}

	// Leave the transition critical
	gSiegeEngine.GetTransitionCritical().Leave();

	// Process any texture changes
	gSiegeEngine.ProcessNodalTextureStates( node, false );

	// Set states
	node.SetIsLoadedPhysical( true );
	node.RequestLightingAccum( true );

	// Success
	return true;
}

bool WorldMap :: LoadNodeContent( database_guid nodeGuid, bool localOnly, bool loadNow )
{
	eLoadSpec spec = LOAD_DEFAULT;
	if ( localOnly )
	{
		spec |= LOAD_LODFI_ONLY;
	}
	if ( loadNow )
	{
		spec |= LOAD_IMMEDIATE;
	}
	return ( loadNodeContent( nodeGuid, spec ) );
}

bool WorldMap :: LoadLodfiContent( database_guid nodeGuid, bool load )
{
	eLoadSpec spec = LOAD_LODFI_ONLY | LOAD_FOR_SLIM_LOAD;
	if ( load )
	{
		spec |= LOAD_FULL;
	}
	return ( loadNodeContent( nodeGuid, spec ) );
}

bool WorldMap :: UnloadLocalContent( database_guid nodeGuid )
{
	return ( loadNodeContent( nodeGuid, LOAD_LOCAL_ONLY | LOAD_FOR_SLIM_LOAD ) );
}

void WorldMap :: LoadAllNodeContent( void )
{
	CheckIndexesLoaded();

	NodeGuidToNodeInfoDb::iterator i, ibegin = m_NodeGuidToNodeInfoDb.begin(), iend = m_NodeGuidToNodeInfoDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->second.m_ScidCount != 0 )
		{
			RegionId regionId = m_RegionIdToNameDb.at( i->second.m_RegionIndex ).first;

			ScidColl::iterator j, jbegin = m_ScidColl.begin() + i->second.m_ScidIndex, jend = jbegin + i->second.m_ScidCount;
			for ( j = jbegin ; j != jend ; ++j )
			{
				gGoDb.SLoadGo( *j, regionId, i->first );
			}
		}
	}
}

void WorldMap :: SyncNodeLodfiContent( database_guid nodeGuid )
{
	loadNodeContent( nodeGuid, LOAD_LODFI_ONLY | LOAD_FOR_DETAIL_SYNC );
}

#if !GP_RETAIL

void WorldMap :: ValidateMapScidIndexes( void ) const
{
	ccast <WorldMap*> ( this )->CheckIndexesLoaded();

	ScidDb::const_iterator i, begin = m_ScidDb.begin(), end = m_ScidDb.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( !FastFuelHandle( i->second.m_FuelAddress ) )
		{
			gperrorf(( "Scid 0x%08X at '%s' does not exist!\n", i->first, i->second.m_FuelAddress.c_str() ));
		}
	}
}

bool WorldMap :: SetScidIndex( Scid scid, const gpstring& fuel )
{
	CheckIndexesLoaded();

	std::pair <ScidDb::iterator, bool> rc = m_ScidDb.insert( std::make_pair( scid, ScidInfo() ) );

	ScidInfo& scidInfo = rc.first->second;
	scidInfo.m_FuelAddress = fuel;

	return ( rc.second );
}

void WorldMap :: RemoveScidIndex( Scid scid )
{
	CheckIndexesLoaded();

	ScidDb::iterator found = m_ScidDb.find( scid );
	if ( found != m_ScidDb.end() )
	{
		m_ScidDb.erase( found );
	}
}

int WorldMap :: ClearRegionScidIndexes( const gpstring& addrPrefix )
{
	CheckIndexesLoaded();
	int found = 0;

	for ( ScidDb::iterator i = m_ScidDb.begin() ; i != m_ScidDb.end() ; )
	{
		if ( i->second.m_FuelAddress.same_no_case( addrPrefix, addrPrefix.length() ) )
		{
			++found;
			i = m_ScidDb.erase( i );
		}
		else
		{
			++i;
		}
	}

	return ( found );
}

#endif // !GP_RETAIL

bool WorldMap :: loadCamera( void )
{
	RatioSample sample( "map_load_camera" );

	// $ get the fuel containing the camera, early bailout if not there
	FastFuelHandle cameraFuel( MakeMapDefAddress() + ":camera" );
	if ( !cameraFuel )
	{
		gperror( "Error loading camera block." );
		return ( false );
	}

	CameraEulerPosition position;
	::ZeroObject( position );

	cameraFuel.Get( "distance", position.distance     );
	cameraFuel.Get( "azimuth",  position.azimuthAngle );
	cameraFuel.Get( "orbit",    position.orbitAngle   );

	float clip = 0.0f;
	if ( cameraFuel.Get( "nearclip", clip ) )
	{
		gSiegeEngine.Renderer().SetClipNearDist( clip );
	}
	if ( cameraFuel.Get( "farclip", clip ) )
	{
		gSiegeEngine.Renderer().SetClipFarDist( clip );
	}

	float temp = 0.0f;
	if ( cameraFuel.Get( "max_azimuth", temp ) )
	{
		gSiegeEngine.GetCamera().SetMaxAzimuth( temp );
	}
	if ( cameraFuel.Get( "max_distance", temp ) )
	{
		gSiegeEngine.GetCamera().SetMaxDistance( temp );
	}
	if ( cameraFuel.Get( "min_azimuth", temp ) )
	{
		gSiegeEngine.GetCamera().SetMinAzimuth( temp );
	}
	if ( cameraFuel.Get( "min_distance", temp ) )
	{
		gSiegeEngine.GetCamera().SetMinDistance( temp );
	}

	gpstring posText;
	if ( cameraFuel.Get( "position", posText ) && ::FromString( posText, position.targetPos.pos ) )
	{
		vector_3 cameraPos;

		// Build the camera position using the angles
		float d;
		SINCOSF( position.azimuthAngle, cameraPos.y, d );
		SINCOSF( position.orbitAngle, cameraPos.x, cameraPos.z );

		d			*= position.distance;
		cameraPos.x	*= d;
		cameraPos.y	*= position.distance;
		cameraPos.z	*= d;

		gSiegeEngine.GetCamera().SetCameraAndTargetPosition( cameraPos, position.targetPos.pos );
		gSiegeEngine.GetCamera().UpdateCamera();
	}
	else
	{
		gperror( "No position block or invalid position specified for camera!\n" );
	}

	return ( true );
}

bool WorldMap :: loadDatabases( void )
{
	RatioSample ratioSample( "map_load_databases" );

	gpassert( m_RegionIdToNameDb.empty() );
	gpassert( m_NodeGuidToNodeInfoDb.empty() );
	gpassert( m_ScidColl.empty() );

	// get all the regions we can find
	FastFuelHandleColl regionDirs;
	{
		RatioSample ratioSample( "map_add_regions", 0, 0.14 );
		if ( !GetAllRegionsDirFuel( regionDirs ) )
		{
			gpfatalf(( "Failed to find any regions in map %s.\n", GetMapName().c_str() ));
			return ( false );
		}

		// add regions
		{
			FastFuelHandleColl::iterator i, ibegin = regionDirs.begin(), iend = regionDirs.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				FastFuelHandle regionDir = *i;
				RegionId regionId = REGIONID_INVALID;

				// this block must exist otherwise the query function above wouldn't have returned it
				FastFuelHandle regionDef = regionDir.GetChildTyped( "region" );
				gpassert( regionDef );

				// assign its guid
				if ( regionDef.Get( "guid", rcast <DWORD&> ( regionId ), false ) )
				{
					// try to insert it
					std::pair <RegionIdToNameDb::iterator, bool>
							rc = m_RegionIdToNameDb.insert( std::make_pair( regionId, regionDir.GetName() ) );
					if ( !rc.second )
					{
						gperrorf(( "Redefined region id 0x%08X, at '%s'\n", regionId, regionDir.GetAddress().c_str() ));
					}
				}
				else
				{
					gperrorf(( "Region '%s' does not have a guid (region ID).\n", regionDir.GetAddress().c_str() ));
				}
			}
		}
	}

	// load in any mesh to sno map information
	{
		RatioSample ratioSample( "map_load_map_mesh_info", 0, 0.2 );
		loadNodeMeshInfoDb();
	}

	// now load in node indexes (unsorted)
	{
		RatioSample ratioSample( "map_load_region_node_info", m_RegionIdToNameDb.size(), 0.30 );
		gpstring status;

		RegionIdToNameDb::iterator i, ibegin = m_RegionIdToNameDb.begin(), iend = m_RegionIdToNameDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// only load the content index if we're not clamped or this is the clamped region
			if ( !IsStreamingRestricted() || (m_ClampingRegionGUID == i->first) )
			{
				ratioSample.Advance( 1.0, status.assignf( "indexing nodes: %s", i->second.c_str() ) );
				loadNodeInfoDb( i->first );
			}
		}
	}

	// postprocess node indexes
	{
		// sort now that it's all in there
		{
			RatioSample ratioSample( "map_sort_nodes", 0, 0.03 );
			m_NodeGuidToNodeInfoDb.sort();
		}

		// extra debugging info
#		if !GP_RETAIL
		if ( !m_NodeGuidToNodeInfoDb.empty() )
		{
			// now seek and destroy dups
			NodeGuidToNodeInfoDb dups;
			{
				for ( NodeGuidToNodeInfoDb::iterator i = m_NodeGuidToNodeInfoDb.begin() + 1 ; i != m_NodeGuidToNodeInfoDb.end() ; )
				{
					if ( i->first == (i - 1)->first )
					{
						dups.unsorted_push_back( *i );
						i = m_NodeGuidToNodeInfoDb.erase( i );
					}
					else
					{
						++i;
					}
				}
			}

			// report
			{
				NodeGuidToNodeInfoDb::const_iterator i, ibegin = dups.begin(), iend = dups.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					gperrorf(( "Error in node index! Found duplicate node guid %s in regions '%s' and '%s'\n",
							   i->first.ToString().c_str(),
							   GetRegionName( GetRegionByNode( i->first ) ).c_str(),
							   m_RegionIdToNameDb.at( i->second.m_RegionIndex ).second.c_str() ));
				}
			}
		}
#		endif // !GP_RETAIL
	}

	// now load in content indexes
	{
		RatioSample ratioSample( "map_load_region_content_info", m_RegionIdToNameDb.size() );
		gpstring status;

		RegionIdToNameDb::iterator i, ibegin = m_RegionIdToNameDb.begin(), iend = m_RegionIdToNameDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// only load the content index if we're not clamped or this is the clamped region
			if ( !IsStreamingRestricted() || (m_ClampingRegionGUID == i->first) )
			{
				ratioSample.Advance( 1.0, status.assignf( "indexing scids: %s", i->second.c_str() ) );
				loadContentIndex( i->first );
			}
		}
	}

#	if !GP_RETAIL

	// if clamping to a region, search out all scids to find rotting/duplicate ones
	if ( IsStreamingRestricted() )
	{
		FastFuelHandle objectsFuel( MakeRegionDirAddress( m_ClampingRegionGUID ) + ":objects" );
		if ( objectsFuel )
		{
			if ( !m_MpWorldName.empty() )
			{
				FastFuelHandle worldFuel = objectsFuel.GetChildNamed( m_MpWorldName );
				if ( worldFuel )
				{
					objectsFuel = worldFuel;
				}
			}

			FastFuelHandleColl objectsColl;
			objectsFuel.ListChildren( objectsColl );

			FastFuelHandleColl::iterator j, jbegin = objectsColl.begin(), jend = objectsColl.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				FastFuelHandle fuel( *j );
				if ( fuel.IsDirectory() )
				{
					continue;
				}

				Scid scid = SCID_INVALID;
				bool success = false;

				// get scid
				if ( ::FromString( fuel.GetName(), scid ) )
				{
					ScidDb::iterator found = m_ScidDb.find( scid );
					if ( found != m_ScidDb.end() )
					{
						if (   fuel.GetAddress().same_no_case( found->second.m_FuelAddress )
							&& (++found->second.m_TouchCount == 1) )
						{
							success = true;
						}
						else
						{
							gperrorf(( "Data rot detected! An object (scid=0x%08X) exists at '%s' that has a duplicate Scid of an object at '%s'\n",
									   scid, fuel.GetAddress().c_str(), found->second.m_FuelAddress.c_str() ));
						}
					}
					else
					{
						gperrorf(( "Data rot detected! An object (scid=0x%08X) exists at '%s' but is not in the content index!\n",
								   scid, fuel.GetAddress().c_str() ));
					}
				}

				// store bad one
				if ( !success )
				{
					ScidInfo& scidInfo = m_RottingScidDb.insert( std::make_pair( scid, ScidInfo() ) )->second;
					scidInfo.m_FuelAddress = fuel.GetAddress();
				}
			}
		}
	}

#	endif // !GP_RETAIL

	// finally, clean out all the fuel we no longer need
	if ( !IsStreamingRestricted() )
	{
		RegionIdToNameDb::iterator i, ibegin = m_RegionIdToNameDb.begin(), iend = m_RegionIdToNameDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// only load the content index if we're not clamped or this is the clamped region
			FastFuelHandle fh( MakeRegionDirAddress( i->first ) + ":index" );
			if ( fh )
			{
				fh.Unload();
			}
		}
	}

	// done
	return ( true );
}

bool WorldMap :: loadStitchDb( void )
{
	RatioSample sample( "map_load_stitch" );

	gpassert( !m_RegionIdToNameDb.empty() );
	gpassert( m_NodeGuidToStitchInfoDb.empty() );

	// $ early bailout if no need for stitch info
	if ( IsStreamingRestricted() )
	{
		return ( true );
	}

	gpgeneric( "Loading map stitch index\n" );

	// $ get the fuel containing the stitch infoz, early bailout if not there
	FastFuelHandle stitchFuel( MakeMapDirAddress() + ":index:stitch_index" );
	if ( !stitchFuel )
	{
		// no stitch info is perfectly ok
		return ( true );
	}

	// get all stitch info entries
	FastFuelHandleColl stitchEntries;
	stitchFuel.ListChildrenTyped( stitchEntries, "stitch_index" );

	// iterate over all stitch indexes
	FastFuelHandleColl::iterator i, ibegin = stitchEntries.begin(), iend = stitchEntries.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// get the region id to which this stitch index belongs
		RegionId regionId = REGIONID_INVALID;
		if ( ::FromString( i->GetName(), regionId ) && ContainsRegion( regionId ) )
		{
			gpstring key, value;
			StitchInfo stitchInfo;
			unsigned int nMeGUID, nNeighborGUID;
			database_guid MeGUID;

			FastFuelFindHandle fh( *i );
			fh.FindFirstKeyAndValue();

			while ( fh.GetNextKeyAndValue( key, value ) )
			{
				if (   stringtool::GetDelimitedValue( value, ',', 0, nMeGUID )
					&& stringtool::GetDelimitedValue( value, ',', 1, stitchInfo.MeDoor )
					&& stringtool::GetDelimitedValue( value, ',', 2, nNeighborGUID )
					&& stringtool::GetDelimitedValue( value, ',', 3, stitchInfo.NeighborDoor ) )
				{
					MeGUID.SetValue( nMeGUID );
					stitchInfo.NeighborGUID.SetValue( nNeighborGUID );

#					if !GP_RETAIL

					// safety
					if ( !ContainsNode( MeGUID ) )
					{
						gperrorf(( "stitch_index error: entry with unknown node GUID found (Region = %s, FromGUID = %s)\n",
								   GetRegionName( regionId ).c_str(), MeGUID.ToString().c_str() ));
						continue;
					}
					if ( !ContainsNode( stitchInfo.NeighborGUID ) )
					{
						gperrorf(( "stitch_index error: entry with unknown node GUID found (Region = %s, NeighborGUID = %s\n",
								   GetRegionName( regionId ).c_str(), stitchInfo.NeighborGUID.ToString().c_str() ));
						continue;
					}

					// search for dup
					std::pair <NodeGuidToStitchInfoDb::iterator, NodeGuidToStitchInfoDb::iterator>
							rc = m_NodeGuidToStitchInfoDb.equal_range( MeGUID );
					for ( ; rc.first != rc.second ; ++rc.first )
					{
						if ( rc.first->second.MeDoor == stitchInfo.MeDoor )
						{
							gperrorf(( "stitch_index error: duplicate FromDoor entry (Region = %s, FromGUID = %s, FromDoor = %d)\n",
									   GetRegionName( regionId ).c_str(), MeGUID.ToString().c_str(), stitchInfo.MeDoor ));
							break;
						}
					}

#					endif

					// ok add it
					m_NodeGuidToStitchInfoDb.insert( std::make_pair( MeGUID, stitchInfo ) );
				}
				else
				{
					gperrorf(( "Unable to extract guid/door info from string '%s' in stitch entry at '%s'\n",
							   value.c_str(), i->GetAddress().c_str() ));
				}
			}
		}
		else
		{
			gperrorf(( "Invalid region ID '%s' in stitch entry at '%s'\n",
					   i->GetName(), i->GetAddress().c_str() ));
		}
	}

	// get all stitch calc entries
	stitchEntries.clear();
	stitchFuel.ListChildrenTyped( stitchEntries, "node_connect_info" );

	int guid = 0;

	// Reserve size in the database
	m_NodeGuidToStitchCalcDb.reserve( stitchEntries.size() );

	// iterate over all stitch indexes
	ibegin = stitchEntries.begin(), iend = stitchEntries.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// Get all node connection blocks
		FastFuelHandleColl nodeStitches;
		(*i).ListChildrenTyped( nodeStitches, "neighbor_connections" );

		// Collection of calc information
		stdx::fast_vector< StitchCalc > stitchCalcColl;
		stitchCalcColl.reserve( nodeStitches.size() );

		// Iterate over all neighbor connections
		for( FastFuelHandleColl::iterator o = nodeStitches.begin(); o != nodeStitches.end(); ++o )
		{
			StitchCalc* pStitchCalc	= &*stitchCalcColl.push_back();
			stringtool::Get( (*o).GetName(), guid );
			pStitchCalc->NeighborGUID	= database_guid( guid );

			gpstring key, value;
			unsigned int nearLogicalId	= 0;
			unsigned int nearLeafId		= 0;
			unsigned int farLogicalId	= 0;
			unsigned int farLeafId		= 0;
			FastFuelFindHandle fh( *o );
			fh.FindFirstKeyAndValue();

			while ( fh.GetNextKeyAndValue( key, value ) )
			{
				// Parse info
				sscanf( value.c_str(), "%d,%d - %d,%d", &nearLogicalId, &nearLeafId, &farLogicalId, &farLeafId );

				LogicalStitch* pLogicalStitch	= NULL;
				
				// See if we already have an entry for this near logical node
				for( stdx::fast_vector< LogicalStitch >::iterator ls = pStitchCalc->NeighborConnections.begin();
					 ls != pStitchCalc->NeighborConnections.end(); ++ls )
				{
					if( (*ls).NearLogicalId == (unsigned char)nearLogicalId )
					{
						pLogicalStitch	= &(*ls);
						break;
					}
				}
				if( !pLogicalStitch )
				{
					gpassert( ls == pStitchCalc->NeighborConnections.end() );
					pLogicalStitch	= &*pStitchCalc->NeighborConnections.push_back();
					pLogicalStitch->NearLogicalId	= (unsigned char)nearLogicalId;
				}

				FarStitch* pFarStitch	= NULL;

				// See if we already have an entry for this far logical node
				for( stdx::fast_vector< FarStitch >::iterator fs = pLogicalStitch->FarConnections.begin();
					 fs != pLogicalStitch->FarConnections.end(); ++fs )
				{
					if( (*fs).FarLogicalId == (unsigned char)farLogicalId )
					{
						pFarStitch	= &(*fs);
						break;
					}
				}
				if( !pFarStitch )
				{
					gpassert( fs == pLogicalStitch->FarConnections.end() );
					pFarStitch	= &*pLogicalStitch->FarConnections.push_back();
					pFarStitch->FarLogicalId	= (unsigned char)farLogicalId;
				}

				LeafStitch* pLeafStitch	= &*pFarStitch->LeafConnections.push_back();
				pLeafStitch->NearLeafId	= (unsigned short)nearLeafId;
				pLeafStitch->FarLeafId	= (unsigned short)farLeafId;
			}

			// Lock the reserved memory for these collections
			for( stdx::fast_vector< LogicalStitch >::iterator ls = pStitchCalc->NeighborConnections.begin();
				 ls != pStitchCalc->NeighborConnections.end(); ++ls )
			{
				for( stdx::fast_vector< FarStitch >::iterator fs = (*ls).FarConnections.begin();
					 fs != (*ls).FarConnections.end(); ++fs )
				{
					(*fs).LeafConnections.reserve_lock();
				}

				(*ls).FarConnections.reserve_lock();
			}
			pStitchCalc->NeighborConnections.reserve_lock();
		}

		// Put the collection of stitch data into our db
		stringtool::Get( (*i).GetName(), guid );
		m_NodeGuidToStitchCalcDb.insert( std::pair< database_guid, stdx::fast_vector< StitchCalc > >( database_guid( guid ), stitchCalcColl ) );
	}

	// don't need the fuel any more
	stitchFuel.GetParent().Unload();

	// success!
	return( true );
}

bool WorldMap :: loadStartingPositions( void )
{
	gpassert( m_SGroupColl.empty() );

	// $ get the fuel containing the starting positions, early bailout if not there
	FastFuelHandle sposFuel( MakeMapDirAddress() + ":info:start_positions" );
	FastFuelHandleColl startGroups;
	if ( !sposFuel || !sposFuel.ListChildrenTyped( startGroups, "start_group" ) )
	{
		gperrorf(( "WorldMap::LoadStartingPositions - map '%s' at '%s' does not have starting positions defined.",
				   GetMapName().c_str(), MakeMapDirAddress().c_str() ));
		return ( IsStreamingRestricted() );
	}

	bool bLoadedDefault = false;

	FastFuelHandleColl::iterator j;
	for ( j = startGroups.begin(); j != startGroups.end(); ++j )
	{
		bool doit =    j->GetBool( "dev_only", false )
					&& (GP_RETAIL || gWorldOptions.GetForceRetailContent());
		if ( doit )
		{
			continue;
		}

		FastFuelHandleColl PosList;

		(*j).ListChildrenNamed( PosList, "start_position", 1 );

		FastFuelHandleColl::iterator i;

		StartingGroup SGroup;
		SGroup.bDefault = false;

		SGroup.sGroup = (*j).GetName();

		// Is this the default starting group?
		(*j).Get( "default", SGroup.bDefault );

		(*j).Get( "id", SGroup.id );

		(*j).Get( "screen_name", SGroup.sScreenName );
		SGroup.sScreenName = ReportSys::TranslateW( SGroup.sScreenName );

		(*j).Get( "description", SGroup.sDescription );
		SGroup.sDescription = ReportSys::TranslateW( SGroup.sDescription.c_str() );

		SGroup.bEnabled = true;
		(*j).Get( "enabled", SGroup.bEnabled );

		FastFuelHandle hWorlds = (*j).GetChildNamed( "world_levels" );
		if( hWorlds )
		{
			FastFuelHandleColl hlWorlds;
			FastFuelHandleColl::iterator iWorlds;
			hWorlds.ListChildren( hlWorlds );
			for ( iWorlds = hlWorlds.begin(); iWorlds != hlWorlds.end(); ++iWorlds )
			{
				WorldStartLevel wsl;			
				wsl.sWorld = (*iWorlds).GetName();
				(*iWorlds).Get( "required_level", wsl.requiredLevel );			
				SGroup.startLevels.push_back( wsl );
			}
		}

		for ( i = PosList.begin(); i != PosList.end(); ++i )
		{
			unsigned int id	= 0;

			StartingPos Pos;

			gpstring posText;
			if ( !(*i).Get( "position", posText ) || !::FromString( posText, Pos.spos ) )
			{
				gperrorf(( "WorldMap::LoadStartingPositions - failed to load starting position block at %s\n", (*i).GetAddress().c_str() ));
				return false;
			}

			// Retrieve Camera information
			FastFuelHandle hCamera = (*i).GetChildNamed( "camera" );
			if ( hCamera.IsValid() )
			{
				CameraEulerPosition eulerPos;
				hCamera.Get( "orbit", eulerPos.orbitAngle );
				hCamera.Get( "azimuth", eulerPos.azimuthAngle );
				hCamera.Get( "distance", eulerPos.distance );
				hCamera.Get( "position", posText );
				::FromString( posText, eulerPos.targetPos );
				eulerPos.MakeCameraPosition( Pos.cpos );
			}
			else
			{
				gpwarningf(( "WorldMap::LoadStartingPositions - failed to load starting position camera block at %s\n", (*i).GetAddress().c_str() ));
			}


			if ( !(*i).Get( "id", id, false ) )
			{
				return false;
			}

			if ( SGroup.bDefault )
			{
				bLoadedDefault = true;
			}

			SGroup.positions.insert( std::make_pair( id, Pos ) );
		}

		SGroupColl::iterator addGroup = m_SGroupColl.begin();
		for ( ; addGroup != m_SGroupColl.end(); ++addGroup )
		{
			if ( SGroup.id < addGroup->id )
			{
				break;
			}
		}
		m_SGroupColl.insert( addGroup, SGroup );

		gpgenericf(( "WorldMap::LoadStartingPositions - loaded %d starting positions for map in group: %s.\n", SGroup.positions.size(), SGroup.sGroup.c_str() ));
	}

	if ( m_SGroupColl.size() == 1 )
	{
		m_SGroupColl.front().bDefault = true;
		bLoadedDefault = true;
	}

	if ( !bLoadedDefault )
	{
		gperrorf(( "Content error: No default Starting Group Found. This must be fixed.\n" ));
	}

	return bLoadedDefault || ::IsWorldEditMode();
}

bool WorldMap :: loadHotpoints( void )
{
	RatioSample sample( "map_load_hotpoints" );

	// always clear db first
	gSiegeHotpointDatabase.Clear();

	// get the fuel containing the hotpoints
	FastFuelHandle hotpointsFuel( MakeMapDirAddress() + ":info:hotpoints" );
	if ( hotpointsFuel )
	{
		FastFuelHandleColl hotpoints;
		hotpointsFuel.ListChildrenTyped( hotpoints, "hotpoint" );

		FastFuelHandleColl::iterator i, ibegin = hotpoints.begin(), iend = hotpoints.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			FastFuelHandle hotpointFuel( *i );
			gSiegeHotpointDatabase.InsertHotpoint( ::ToUnicode( hotpointFuel.GetName() ), hotpointFuel.GetInt( "id", 0 ) );
		}
	}

	// done
	return ( true );
}

bool WorldMap :: loadWorldLocations( void )
{
	gpassert( m_WorldLocations.empty() );

	FastFuelHandle locationsFuel( MakeMapDirAddress() + ":info:world_locations" );
	if ( locationsFuel )
	{
		FastFuelHandleColl locations;
		locationsFuel.ListChildrenTyped( locations, "location" );

		FastFuelHandleColl::iterator i = locations.begin();
		for ( ; i != locations.end(); ++i )
		{
			unsigned int id;
			WorldLocation newLocation;
			
			FastFuelHandle locationFuel( *i );
			if ( !locationFuel.Get( "id", id ) )
			{
				gpwarningf(( "Found world location '%s', without an id.", locationFuel.GetName() ));
				continue;
			}

			newLocation.sName = locationFuel.GetName();
			locationFuel.Get( "screen_name", newLocation.sScreenName );
			newLocation.sScreenName = ReportSys::TranslateW( newLocation.sScreenName );

			m_WorldLocations.insert( WorldLocationMap::value_type( id, newLocation ) );
		}
	}

	return ( true );
}

bool WorldMap :: loadNodeInfoDb( RegionId regionId )
{
	// $ for performance, this db is filled unsorted, in the same order that
	//   they appear in the gas file. after all regions are loaded, then the
	//   entire set is sorted at once.

	// absolutely must have a node index
	FastFuelHandle nodeIndexFuel( MakeRegionDirAddress( regionId ) + ":index:streamer_node_index" );
	if ( !nodeIndexFuel )
	{
		gperrorf(( "Region '%s' does not contain a node index!\n", GetRegionName( regionId ).c_str() ));
		return ( false );
	}

	// find our region index
	RegionIdToNameDb::const_iterator found = m_RegionIdToNameDb.find( regionId );
	gpassert( found != m_RegionIdToNameDb.end() );
	int regionIndex = found - m_RegionIdToNameDb.begin();

	// get some vars
	unsigned int nodeGuid = 0;
	FastFuelFindHandle fh( nodeIndexFuel );

	// reserve that much space in advance to get a precise alloc
	m_NodeGuidToNodeInfoDb.reserve_extra( fh.GetKeyAndValueCount() );

	// add node index
	fh.FindFirstValue( "*" );
	while ( fh.GetNextValue( nodeGuid ) )
	{
		NodeGuidToNodeInfoDb::iterator rc = m_NodeGuidToNodeInfoDb.unsorted_push_back(
				std::make_pair( database_guid( nodeGuid ), NodeInfo() ) );
		rc->second.m_RegionIndex = (WORD)regionIndex;
	}

	// done
	return ( true );
}

bool WorldMap :: loadContentIndex( RegionId regionId )
{	
	gpassert( Services::IsEditor() || !m_NodeGuidToNodeInfoDb.empty() );

	// try to get at the index using our world name (this may fail if content not there)
	FastFuelHandle contentIndexFuel;
	gpstring nameInsert;
	if ( !m_MpWorldName.empty() )
	{
		contentIndexFuel = FastFuelHandle( MakeRegionDirAddress( regionId ) + ":index:" + m_MpWorldName + ":streamer_node_content_index" );
		if ( contentIndexFuel )
		{
			nameInsert = ":" + m_MpWorldName;
		}
	}

	// try again if that didn't work using no world
	if ( !contentIndexFuel )
	{
		contentIndexFuel = FastFuelHandle( MakeRegionDirAddress( regionId ) + ":index:streamer_node_content_index" );
	}

	// still it's ok if there's no content
	if ( !contentIndexFuel )
	{
		return ( true );
	}

	// build prefix out to <current_map>:regions:<current_region>:objects:<mp_world>:
#	if !GP_RETAIL
	gpstring prefix( MakeRegionDirAddress( regionId ) );
	prefix += ":objects" + nameInsert + ":";
#	endif // !GP_RETAIL

	// get some vars
	gpstring key;
	unsigned int nscid = 0;
	FastFuelFindHandle fh( contentIndexFuel );

	// reserve that much space in advance to get a precise alloc
	m_ScidColl.reserve_extra( fh.GetKeyAndValueCount() );

	// add content index
	fh.FindFirstKeyAndValue();
	while ( fh.GetNextKeyAndValue( key, nscid ) )
	{
		unsigned int nguid = 0;
		if (   ::FromString( key, nguid )
			&& ::IsInstance( MakeScid( nscid ), false ) )
		{
			// convert data
			database_guid guid( nguid );
			Scid scid = MakeScid( nscid );

			// build scid info for dev tracking
#			if !GP_RETAIL
			ScidInfo scidInfo;
			scidInfo.m_FuelAddress = prefix + gpstringf( "0x%08X", scid );
			scidInfo.m_NodeGuid = guid;
#			endif // !GP_RETAIL

			// first find the node
			NodeGuidToNodeInfoDb::iterator found = m_NodeGuidToNodeInfoDb.find( guid );
			if ( found != m_NodeGuidToNodeInfoDb.end() )
			{
				NodeInfo& nodeInfo = found->second;

				// add to dev db
#				if !GP_RETAIL
				std::pair <ScidDb::iterator, bool> rc = m_ScidDb.insert( std::make_pair( scid, scidInfo ) );
				if ( !rc.second )
				{
					gperrorf(( "Error: duplicate guid/scid %s/0x%08X found in index at fuel '%s'\n",
							   guid.ToString().c_str(), scid, scidInfo.m_FuelAddress.c_str() ));
					continue;
				}
#				endif // !GP_RETAIL

				// set up nodeinfo
				if ( nodeInfo.m_ScidIndex == 0xFFFFFFFF )
				{
					gpassert( nodeInfo.m_ScidCount == 0 );
					nodeInfo.m_ScidIndex = m_ScidColl.size();
				}

				// add new item
				++nodeInfo.m_ScidCount;
				m_ScidColl.push_back( scid );

				// now verify that the fuel entries are in order
				gpassert( (m_ScidColl.size() - nodeInfo.m_ScidIndex) == nodeInfo.m_ScidCount );
			}
#			if !GP_RETAIL
			else
			{
				m_RottingScidDb.insert( std::make_pair( scid, scidInfo ) );

				gperrorf(( "Data rot detected! Somehow this map has a Go "
						   "(scid=0x%08X) sitting at siege node %s (at %s) but "
						   "that node is not in the map!...\n",
						   scid, guid.ToString().c_str(), scidInfo.m_FuelAddress.c_str() ));
			}
#			endif // !GP_RETAIL
		}
		else
		{
			gperrorf(( "Unable to convert key/value pair '%s=0x%08X' into a node guid/scid pair at fuel '%s'\n",
					   key.c_str(), nscid, contentIndexFuel.GetName() ));
		}
	}

	return ( true );
}

bool WorldMap :: loadNodeMeshInfoDb( void )
{
	bool bUseNodeMesh = false;
	FastFuelHandle hMap;
	if ( IsInitialized() )
	{
		hMap =  MakeMapDirFuel();
	}
	if ( hMap )
	{
		FastFuelHandle hMain = hMap.GetChildNamed( "map" );	
		if ( hMain )
		{
			hMain.Get( "use_node_mesh_index", bUseNodeMesh );
		}
	}
	
	if ( !bUseNodeMesh )
	{
		// Clear out any existing nodes
		gSiegeEngine.MeshDatabase().FileNameMap().clear();

		// Initialize Global Siege node database.
		FastFuelHandle siegeNodes( "world:global:siege_nodes" );
		FastFuelHandleColl siegeNodeList;

		if ( siegeNodes && siegeNodes.ListChildrenNamed( siegeNodeList, "mesh_file*", -1 ) )
		{
			gpstring guidString, filename;

			FastFuelHandleColl::iterator i, begin = siegeNodeList.begin(), end = siegeNodeList.end();
			for ( i = begin ; i != end ; ++i )
			{
				if ( (*i).Get( "guid", guidString, false )
					&& ((*i).Get( "filename", filename ) || (*i).Get( "genericname", filename, false )) )
				{
					loadNodeMeshInfo( guidString, filename, (*i).GetAddress() );
				}
			}
		}
		else
		{
			gpwarning( "No Siege node Guid-to-mesh assignments found!\n" );
		}

		//siegeNodes->GetDB()->SaveChanges();		// $ enable if necessary (for cleaning)		
	}
	else
	{
		FastFuelHandle hRegions = hMap.GetChildNamed( "regions" );
		FastFuelHandleColl hlRegions;
		FastFuelHandleColl::iterator iRegion;
		hRegions.ListChildren( hlRegions, 1 );
		for ( iRegion = hlRegions.begin(); iRegion != hlRegions.end(); ++iRegion )
		{
			gpstring sNodeMesh = (*iRegion).GetAddress();
			sNodeMesh.appendf( ":index:node_mesh_index" );
			FastFuelHandle hNodeMesh( sNodeMesh );
			if ( hNodeMesh )
			{
				FastFuelFindHandle hFind( hNodeMesh );
				if ( hFind.FindFirstKeyAndValue() )
				{
					gpstring guidString, filename, fullname;
					while ( hFind.GetNextKeyAndValue( guidString, filename ) )
					{	
						loadNodeMeshInfo( guidString, filename, hNodeMesh.GetAddress(), true );
					}
				}			
			}
		}
	}

#	if !GP_RETAIL
	m_SiegeDupDb.clear();
#	endif // !GP_RETAIL

	return ( true );
}

// loader helper helper helper
bool WorldMap :: loadNodeMeshInfo( const gpstring & guidString, const gpstring & filename, const gpstring & address, bool bHasNodeMeshIndex )
{
	gpstring fullname;

#	if !GP_RETAIL
	if ( !gNamingKey.BuildSNOLocation( filename, fullname ) )
	{
		gperrorf(( "Could not build .SNO location from '%s' at fuel '%s'\n",
				   filename.c_str(), address.c_str() ));
		return ( false );
	}
	if ( !FileSys::DoesResourceFileExist( fullname ) )
	{
		gperrorf(( "Error: could not find file '%s' for mesh guid '%s' found at gas '%s'!\n",
				   fullname.c_str(), guidString.c_str(), address.c_str() ));
		// (*i).Delete();		// $ enable if necessary
		return ( false );
	}
	std::pair <SiegeDupDb::iterator, bool> rcx =
			m_SiegeDupDb.insert( std::make_pair( siege::database_guid( guidString ), FastFuelHandle( address ) ) );
#	endif // !GP_RETAIL

	// insert entry into mesh database
	std::pair <siege::MeshDatabase::MeshFileMap::iterator, bool> rc =
			gSiegeEngine.MeshDatabase().FileNameMap().insert(
					std::make_pair( siege::database_guid( guidString ), filename ) );

	// check for duplicates
#	if !GP_RETAIL
	if ( !bHasNodeMeshIndex && !rc.second && !address.same_no_case( rcx.first->second.GetAddress() ) )
	{		
		gperrorf(( "Error: duplicate mesh guid '%s' detected, colliding fuel addresses:\n   %s\n   %s\n",
				   guidString.c_str(), address.c_str(), rcx.first->second.GetAddress().c_str() ));
	}
#	endif // !GP_RETAIL

	return ( true );
}

// Collect the doors for the given node
void WorldMap :: collectDoors( database_guid nodeGuid, siege::SiegeTransition::NodeDoorColl& doorColl )
{
	CheckIndexesLoaded();

	FastFuelHandle hNode( MakeNodeFuel( nodeGuid ) );
	if ( !hNode )
	{
		gpfatalf(( "Failed to load siege node 0x%08x because we couldn't create a FastFuelHandle to it.\n", nodeGuid.GetValue() ));
		return;
	}

	// Load door information
	FastFuelHandleColl doors;
	hNode.ListChildrenNamed( doors, "door*" );

	for( FastFuelHandleColl::iterator iDoor = doors.begin(); iDoor != doors.end(); ++iDoor )
	{
		SiegeTransition::NodeDoorInfo doorInfo;
		doorInfo.m_nearDoor	= (*iDoor).GetInt( "id" );
		doorInfo.m_farNode	= database_guid( (*iDoor).GetInt( "farguid" ) );
		doorInfo.m_farDoor	= (*iDoor).GetInt( "fardoor" );
		doorInfo.m_bConnect	= false;

		doorColl.push_back( doorInfo );
	}

	// Load for regional door information
	if ( !IsStreamingRestricted() )
	{
		// Get all the nodes that request a regional connection with me (may not be any)
		std::pair <NodeGuidToStitchInfoDb::const_iterator, NodeGuidToStitchInfoDb::const_iterator>
				rc = m_NodeGuidToStitchInfoDb.equal_range( nodeGuid );
		for ( ; rc.first != rc.second; ++rc.first )
		{
			const StitchInfo& info = rc.first->second;
			gpassert( rc.first->first == nodeGuid );

			SiegeTransition::NodeDoorInfo doorInfo;
			doorInfo.m_nearDoor	= info.MeDoor;
			doorInfo.m_farNode	= database_guid( info.NeighborGUID );
			doorInfo.m_farDoor	= info.NeighborDoor;
			doorInfo.m_bConnect	= true;

			doorColl.push_back( doorInfo );
		}
	}
}

// stitch node info using the stitch db
void WorldMap :: autoStitchNode( SiegeNode& node )
{
	CheckIndexesLoaded();

	// Find the entry for this node in the stitch calc db
	NodeGuidToStitchCalcDb::const_iterator found = m_NodeGuidToStitchCalcDb.find( node.GetGUID() );
	if_not_gpassert( found != m_NodeGuidToStitchCalcDb.end() )
	{
		return;
	}

	const stdx::fast_vector< StitchCalc >& stitchColl	= found->second;
	for( stdx::fast_vector< StitchCalc >::const_iterator i = stitchColl.begin(); i != stitchColl.end(); ++i )
	{
		// Synchro threads while we update the pointers
		kerneltool::Critical::Lock autoLock( gSiegeEngine.GetLogicalCritical() );

		for( stdx::fast_vector< LogicalStitch >::const_iterator o = (*i).NeighborConnections.begin(); o != (*i).NeighborConnections.end(); ++o )
		{
			SiegeLogicalNode* pLogicalNode		= node.GetLogicalNodes()[ (*o).NearLogicalId ];
			gpassert( pLogicalNode->GetID() == (*o).NearLogicalId );

			// Connect two logical nodes together
			LNODECONNECT* pNearConnections		= pLogicalNode->GetNodeConnectionInfo();
			unsigned short numNearConnections	= pLogicalNode->GetNumNodeConnections();
			LNODECONNECT* pNewNearConnections	= NULL;

			// Make the new lists and copy the old information into them
			unsigned short newNearConnections	= (unsigned short)(numNearConnections + (*o).FarConnections.size());
			pNewNearConnections					= new LNODECONNECT[ newNearConnections ];
			memcpy( pNewNearConnections, pNearConnections, sizeof( LNODECONNECT ) * numNearConnections );

			unsigned int index					= 0;
			for( unsigned int f = numNearConnections; f != newNearConnections; ++f, ++index )
			{
				// Add the new connections
				pNewNearConnections[ f ].m_farSiegeNode			= (*i).NeighborGUID;
				pNewNearConnections[ f ].m_pCollection			= new LCCOLLECTION;
				pNewNearConnections[ f ].m_pCollection->m_farid	= (*o).FarConnections[ index ].FarLogicalId;

				// Build new lists
				unsigned int numConnections		= (*o).FarConnections[ index ].LeafConnections.size();
				LNODECONNECT* pNearConnection	= NULL;

				pNearConnection												= &pNewNearConnections[ f ];
				pNearConnection->m_pCollection->m_numNodalLeafConnections	= numConnections;
				pNearConnection->m_pCollection->m_pNodalLeafConnections		= new NODALLEAFCONNECT[ numConnections ];
				memcpy( pNearConnection->m_pCollection->m_pNodalLeafConnections,
						&(*(*o).FarConnections[ index ].LeafConnections.begin()),
						sizeof( NODALLEAFCONNECT ) * numConnections );
			}

			// Set the new data in the Logical node
			pLogicalNode->SetNodeConnectionInfo( pNewNearConnections );
			pLogicalNode->SetNumNodeConnections( newNearConnections );
			delete[] pNearConnections;
		}
	}
}

bool WorldMap :: loadNodeContent( database_guid nodeGuid, eLoadSpec spec )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );
	CheckIndexesLoaded();

	if ( m_NodeGuidToNodeInfoDb.empty() && Services::IsEditor() )
	{
		// must be starting a new region, it's safe.
		return ( true );
	}

	// look up node info
	NodeGuidToNodeInfoDb::const_iterator found = m_NodeGuidToNodeInfoDb.find( nodeGuid );
	if ( found == m_NodeGuidToNodeInfoDb.end() )
	{
		return ( false );
	}
	const NodeInfo& nodeInfo = found->second;

	// get the region id
	RegionId regionId = m_RegionIdToNameDb.at( nodeInfo.m_RegionIndex ).first;

	// access scids
	if ( nodeInfo.m_ScidCount != 0 )
	{
		ScidColl::iterator i, ibegin = m_ScidColl.begin() + nodeInfo.m_ScidIndex, iend = ibegin + nodeInfo.m_ScidCount;
		for ( i = ibegin ; i != iend ; ++i )
		{
			Scid scid = *i;

			// check for whether or not we're supposed to construct this
			ContentDb::eCreateType createType = gContentDb.GetCreateTypeForScid( scid, regionId );
			if ( createType == ContentDb::CREATE_GLOBAL )
			{
				if ( !(spec & (LOAD_LODFI_ONLY | LOAD_LOCAL_ONLY)) && ::IsServerLocal() )
				{
					gGoDb.SLoadGo( scid, regionId, nodeGuid );
				}
			}
			else if ( createType == ContentDb::CREATE_LOCAL_PLAIN )
			{
				if ( !(spec & LOAD_LODFI_ONLY) )
				{
					if ( spec & LOAD_FOR_SLIM_LOAD )
					{
						// get existing go
						Goid goid = gGoDb.FindGoidByScid( scid );
						if ( goid != GOID_INVALID )
						{
							GoHandle go( goid );
							if_gpassert( go )
							{
								// see note at bottom about #12255, same deal here.
								if ( !go->IsCommittingCreation() )
								{
									gGoDb.SMarkForDeletion( goid, false, false );
								}
							}
						}
					}
					else
					{
						gGoDb.LoadLocalGo( scid, regionId, nodeGuid, false, false );
					}
				}
			}
			else if ( !(spec & LOAD_LOCAL_ONLY) )
			{
				enum eLoadType
				{
					LOAD_SYNC,
					LOAD_ASYNC,
					LOAD_SKIP,
					LOAD_DELETE,
				};

				// default is skip - never request lodfi loading during normal
				// walking, wait until the membership is determined first so we
				// can avoid loading lodfi objects for non-screen player
				// frustums in multiplayer.
				eLoadType type = LOAD_SKIP;

				// detect how to load this lodfi type
				if ( createType == ContentDb::CREATE_LOCAL_LODFI )
				{
					if ( spec & LOAD_FOR_DETAIL_SYNC )
					{
						type = LOAD_ASYNC;
					}
					else if ( spec & LOAD_FOR_SLIM_LOAD )
					{
						if ( spec & LOAD_FULL )
						{
							type = LOAD_ASYNC;
						}
						else
						{
							type = LOAD_DELETE;
						}
					}
					else if ( spec & LOAD_IMMEDIATE )
					{
						type = LOAD_SYNC;
					}
				}
				else if ( spec & (LOAD_FOR_DETAIL_SYNC | LOAD_FOR_SLIM_LOAD) )
				{
					type = LOAD_DELETE;
				}

				// special case: if we're showing an NIS we're probably going to
				// be popping the camera around, so don't do any async loading
				// here, and don't do any delayed (via skipping) loading either,
				// but instead force it in now with no fading.
				bool permitFadeIn = true;
				if ( (type == LOAD_ASYNC) || ((type == LOAD_SKIP) && (createType != ContentDb::CREATE_NONE)) )
				{
					eWorldState ws = gWorldState.GetCurrentState();
					if ( (ws == WS_SP_NIS) || ::IsLoading( ws ) )
					{
						permitFadeIn = false;
					}
				}

				// get existing go
				Goid goid = gGoDb.FindGoidByScid( scid );

				// ok do what we said
				if ( type == LOAD_SYNC )
				{
					// load 'er right now, assuming it's not already there
					if ( goid == GOID_INVALID )
					{
						gGoDb.LoadLocalGo( scid, regionId, nodeGuid, true, false );
					}
				}
				if ( type == LOAD_ASYNC )
				{
					// request that this gets loaded whenever, assuming it's
					// not already there
					if ( goid == GOID_INVALID )
					{
						static gpstring s_Lodfi( "lodfi" );
						gSiegeLoadMgr.Load( LOADER_LODFI_TYPE, s_Lodfi, (DWORD)scid, false, LodfiLoaderInfo( regionId, nodeGuid, permitFadeIn ) );
					}
				}
				else if ( type == LOAD_DELETE )
				{
					// delete this go if it exists
					if ( goid != GOID_INVALID )
					{
						GoHandle go( goid );
						if_gpassert( go && go->IsLodfi() )
						{
							// note: only do the fade-out if the player changed
							// the object detail settings just now. also, don't do
							// it if it's still in the middle of committing
							// creation. that can happen if the primary thread is
							// trying to nuke a node's lodfi objects while the
							// loader is still working on creating one. it's ok to
							// ignore deletion because when the go tries to commit
							// it will detect the deleted node and then delete it
							// anyway (see #12255).
							if ( !go->IsCommittingCreation() )
							{
								gGoDb.SMarkForDeletion( goid, false, (spec & LOAD_FOR_DETAIL_SYNC) != 0 );
							}
						}
					}
				}
			}
		}
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
