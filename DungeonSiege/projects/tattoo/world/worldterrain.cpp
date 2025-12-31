//////////////////////////////////////////////////////////////////////////////

#include "precomp_world.h"
#include "WorldTerrain.h"

#include "AIQuery.h"
#include "AppModule.h"
#include "CameraAgent.h"
#include "FileSys.h"
#include "FileSysXfer.h"
#include "FuBiBitPacker.h"
#include "FuBiPersist.h"
#include "FubiPersistBinary.h"
#include "Fuel.h"
#include "GoAspect.h"
#include "GoCore.h"
#include "GoDb.h"
#include "GoSupport.h"
#include "MCP.h"
#include "nema_aspect.h"
#include "netfubi.h"
#include "netpipe.h"
#include "Oriented_Bounding_Box_3.h"
#include "Player.h"
#include "RatioStack.h"
#include "Server.h"
#include "Siege_Engine.h"
#include "Siege_Frustum.h"
#include "Siege_Loader.h"
#include "Siege_Logical_Mesh.h"
#include "Siege_Logical_Node.h"
#include "Siege_Mesh.h"
#include "Siege_Mesh_Door.h"
#include "Siege_Node.h"
#include "Siege_Options.h"
#include "StdHelp.h"
#include "TimeOfDay.h"
#include "WorldMap.h"
#include "WorldMessage.h"
#include "WorldState.h"
#include "WorldTime.h"


using namespace siege;


void TTMessages :: Init()
{
	ZeroObject( *this );

	m_BeginEvent	= WE_INVALID;
	m_BeginSendTo	= SCID_INVALID;

	m_EndEvent		= WE_INVALID;
	m_EndSendTo		= SCID_INVALID;
}

TTMessages & MakeTTMessages()
{
	static TTMessages temp;
	temp.Init();
	return temp;
}


struct BlockRequestInfo
{
	oriented_bounding_box_3	objectObb;
	bool					bStatus;
};




//////////////////////////////////////////////////////////////////////////////
// class WorldFrustum declaration

class WorldFrustum : public siege::SiegeFrustum
{
public:
	SET_INHERITED( WorldFrustum, siege::SiegeFrustum );

	WorldFrustum( unsigned int idBitfield )
		: Inherited( idBitfield )
	{
		m_PlayerId = PLAYERID_INVALID;
	}

	virtual ~WorldFrustum( void )
	{
		// this space intentionally left blank...
	}

	virtual bool Xfer( FuBi::PersistContext& persist )
	{
		bool rc = Inherited::Xfer( persist );
		persist.Xfer( "m_PlayerId", m_PlayerId );
		return ( rc );
	}

	void SetPlayerId( PlayerId playerId )
	{
		gpassert( m_PlayerId == PLAYERID_INVALID );
		m_PlayerId = playerId;
	}

	PlayerId GetPlayerId( void ) const
	{
		gpassert( m_PlayerId != PLAYERID_INVALID );
		return ( m_PlayerId );
	}

	static Inherited* FactoryProc( unsigned int idBitfield )
	{
		return ( new WorldFrustum( idBitfield ) );
	}

private:
	PlayerId m_PlayerId;

	SET_NO_COPYING( WorldFrustum );
};

//////////////////////////////////////////////////////////////////////////////

WorldTerrain :: WorldTerrain()
{
	// Create our map
	m_pMap = new WorldMap;

	// Register callback
	gSiegeEngine.RegisterFrustumFactoryCb( makeFunctor( WorldFrustum::FactoryProc ) );

	// Register transition complete callback
	gSiegeEngine.RegisterTransitionCompleteCb( makeFunctor( *this, &WorldTerrain :: NodeTransitionComplete ) );
}


WorldTerrain :: ~WorldTerrain()
{
	// Unregister callback
	gSiegeEngine.RegisterFrustumFactoryCb( NULL );

	// Delete owned objects
	Delete( m_pMap );
}


bool WorldTerrain :: Xfer( FuBi::PersistContext& persist )
{
	// persist map
	{
		RatioSample ratioSample( "load_map", 0, 0.56 );

		persist.EnterBlock( "m_pMap" );
		if ( !m_pMap->Xfer( persist ) )
		{
			return ( false );
		}
		persist.LeaveBlock();
	}

	// temporarily disable content loading so we can just load terrain
	bool oldSkipContent = gWorldOptions.GetSkipContent();
	gWorldOptions.SetSkipContent( true );

	// persist siege engine
	persist.EnterBlock( "SiegeEngine" );
	gSiegeEngine.Xfer( persist );
	persist.LeaveBlock();

	// force it in (lock out nested ratio if we're saving)
	if ( persist.IsRestoring() )
	{
		RatioSample ratioSample( "" );
		ForceWorldLoad( true, true );
	}

	// back to normal
	gWorldOptions.SetSkipContent( oldSkipContent );

	return ( true );
}


void WorldTerrain :: SSyncOnMachine( DWORD machineId )
{
	SSyncTransitionsOnMachine( machineId );
	SSyncGlobalFadesOnMachine( machineId );
	SSyncNodalTexStatesOnMachine( machineId );
	SSyncNodalFlagStatesOnMachine( machineId );
}


FuBiCookie WorldTerrain :: SRegisterWorldFrustum( FrustumId& frustumId, Player* player )
{
	CHECK_SERVER_ONLY;

	gpassert( player != NULL );
	gpassert( player->GetController() == PC_HUMAN );

	// Get an ID
	frustumId = MakeFrustumId( siege::SiegeFrustum::GetNextFreeIdBitfield() );
	if ( frustumId == 0 )
	{
		gperror( "Too many frustums, can't create any more!!!\n" );
		return ( RPC_FAILURE_IGNORE );
	}

	// Register the frustum and fill it
	RCRegisterWorldFrustumOnMachine( RPC_TO_LOCAL, player->GetId(), frustumId );

	// Tell remote player to create their own frustum
	if ( player->IsRemote() )
	{
		RCRegisterWorldFrustumOnMachine( player->GetMachineId(), player->GetId(), frustumId );
	}

	// Success
	return ( RPC_SUCCESS );
}


FuBiCookie WorldTerrain :: RCRegisterWorldFrustumOnMachine( unsigned int machineId, PlayerId playerId, FrustumId frustumId )
{
	FUBI_RPC_CALL_RETRY( RCRegisterWorldFrustumOnMachine, machineId );

	Player* player = gServer.GetPlayer( playerId );
	gpassert( player != NULL );

	// Out status
	gpgenericf(( "WorldTerrain :: RCRegisterWorldFrustumOnMachine( machine 0x%08X, playerid %d - '%S', frustum %d )\n",
				 machineId, playerId, player->GetName().c_str(), frustumId ));

	return ( RegisterWorldFrustum( playerId, frustumId ) ? RPC_SUCCESS : RPC_FAILURE_IGNORE );
}

bool WorldTerrain :: RegisterWorldFrustum( PlayerId playerId, FrustumId frustumId )
{
	Player* player = gServer.GetPlayer( playerId );
	gpassert( player != NULL );

	// Add to collection
	frustumId = MakeFrustumId( gSiegeEngine.CreateFrustum( MakeInt( frustumId ) ) );
	WorldFrustum* frustum = scast <WorldFrustum*> ( gSiegeEngine.GetFrustum( MakeInt( frustumId ) ) );

	// Set it up
	frustum->SetDimensions( m_pMap->GetWorldFrustumRadius(), m_pMap->GetWorldFrustumDepth(), m_pMap->GetWorldFrustumHeight() );
	frustum->SetInterestRadius( m_pMap->GetWorldInterestRadius() );
	frustum->SetPlayerId( playerId );

	// Tell siege this is the render frustum if this is the screen player
	if ( player->IsScreenPlayer() && (gSiegeEngine.GetRenderFrustum() == 0) )
	{
		gSiegeEngine.SetRenderFrustum( MakeInt( frustumId ) );
	}

	// If the player is remote, setup for slim load
	if ( player->IsRemote() )
	{
		frustum->SetFullLoad( false );
		frustum->SetDiscoverySubmission( false );
	}

	// Success
	return ( RPC_SUCCESS );
}


// Performs actual destruction
void WorldTerrain :: DestroyWorldFrustum( FrustumId frustumId )
{
	WorldFrustum* frustum = scast <WorldFrustum*> ( gSiegeEngine.GetFrustum( MakeInt( frustumId ) ) );
	if ( frustum == NULL )
	{
		return;
	}

	// Out status
#	if GP_DEBUG
	Player* player = gServer.GetPlayer( frustum->GetPlayerId() );
	gpassert( player != NULL );
	gpgenericf(( "WorldTerrain :: DestroyWorldFrustum( player %d - '%S', frustum %d )\n",
				 frustum->GetPlayerId(), player->GetName().c_str(), frustumId ));
#	endif // GP_DEBUG

	// Destroy!
	gSiegeEngine.DestroyFrustum( MakeInt( frustumId ) );
}

// Performs an instantaneous complete world load
void WorldTerrain :: ForceWorldLoad( bool forceThreadStart, bool queuedLoad )
{
	RatioSample ratioSample( "load_map_data" );

	// Shut down the loading thread
	bool wasLoading = gSiegeEngine.IsLoadingThreadRunning();
	gSiegeEngine.StopLoadingThread();

	// Queued?
	bool wasQueued = gSiegeLoadMgr.IsQueuedMode();
	if ( queuedLoad )
	{
		gSiegeLoadMgr.SetQueuedMode( true );
		gSiegeLoadMgr.SetQueuedCommitType( siege::LOADER_SIEGE_NODE_TYPE );
	}

	// Make sure the map is ready
	gWorldMap.CheckIndexesLoaded();

	// Force load all frustums
	{
		RatioSample ratioSample( "prefetch_nodes", 0, 0.10 );
		gSiegeEngine.UpdateFrustums( 0.0f, true );
	}

	// Get the active frustum and set the current target node
	SiegeFrustum* pFrustum = gSiegeEngine.GetFrustum( gSiegeEngine.GetRenderFrustum() );
	if( pFrustum )
	{
		gSiegeEngine.NodeWalker().SetTargetNodeGUID( pFrustum->GetPosition().node );
	}

	// If queued mode, commit now
	if ( queuedLoad )
	{
		{
			RatioSample ratioSample( "", 0, 0.60 );
			gSiegeLoadMgr.Commit( LOADER_GO_TYPE, true );
		}
		{
			RatioSample ratioSample( "", 0, 0.25 );
			gSiegeLoadMgr.Commit( LOADER_TEXTURE_TYPE, true );
		}
		{
			gSiegeLoadMgr.CommitAll( true );
		}
		gSiegeLoadMgr.SetQueuedMode( wasQueued );
	}

	// Start loading thread if it was going before
	if ( wasLoading || forceThreadStart )
	{
		gSiegeEngine.StartLoadingThread();
	}
}


// Initiate a nodal transition (elevators, platforms, any type of moving node)
void WorldTerrain :: SRequestNodeTransition( SiegeId targetNode, const DWORD targetDoor,
										   SiegeId connectNode, const DWORD connectDoor,
										   const float duration, const eAxisHint ahint, Goid owner,
										   bool bConnect, bool bForceComplete, TTMessages * msg )
{
	CHECK_SERVER_ONLY;

	if( ((unsigned int)targetNode == 0) ||
		((unsigned int)connectNode == 0) )
	{
		gperrorf(( "Node transition was requested with NULL node!  Did you forget to enter a property?  SCID: 0x%08x", GetScid( owner ) ));
		return;
	}

	double startTime	= PreciseAdd(gWorldTime.GetTime(), gServer.GetMaxPlayerLatency());

	siege::database_guid targetGuid( (unsigned int)targetNode );
	gMCPManager.NotifyNodeStateChange(targetGuid);

	RCRequestNodeTransition( targetNode, targetDoor, connectNode, connectDoor,
							 startTime, PreciseAdd(startTime, duration), ahint, owner,
							 bConnect, bForceComplete, msg );
}


// Initiate a nodal transition (elevators, platforms, any type of moving node)
FuBiCookie WorldTerrain :: RCRequestNodeTransition( SiegeId targetNode, const DWORD targetDoor,
												  SiegeId connectNode, const DWORD connectDoor,
												  const double startTime, const double endTime,
												  const eAxisHint ahint, Goid owner,
												  bool bConnect, bool bForceComplete, TTMessages * msg )
{
	FUBI_RPC_CALL_RETRY( RCRequestNodeTransition, RPC_TO_ALL );

	// Request the node transition from the engine
	siege::database_guid targetGuid( (unsigned int)targetNode );
	siege::database_guid connectGuid( (unsigned int)connectNode );

	gSiegeEngine.NodeTransition( targetGuid, targetDoor, connectGuid, connectDoor,
								 gWorldTime.GetTime(), startTime, endTime,
								 ahint, MakeInt(owner), bConnect, bForceComplete );

	// now make sure begin and end transition messages get sent...

	if( msg && ( msg->m_BeginEvent != WE_INVALID ) )
	{	
		WorldMessage msgBegin( msg->m_BeginEvent, GOID_INVALID, msg->m_BeginSendTo );
		msgBegin.SendDelayed( float( PreciseSubtract(startTime, gWorldTime.GetTime()) ) + msg->m_BeginDelay, MD_FROM_ELEVATOR );
	}

	if( msg && ( msg->m_EndEvent != WE_INVALID ) )
	{
		WorldMessage msgEnd( msg->m_EndEvent, GOID_INVALID, msg->m_EndSendTo );
		msgEnd.SendDelayed( float( PreciseSubtract(endTime, gWorldTime.GetTime()) ) + msg->m_EndDelay, MD_FROM_ELEVATOR );
	}

	return RPC_SUCCESS;
}


// Request the connection of two nodes
void WorldTerrain :: SRequestNodeConnection( SiegeId targetNode, const DWORD targetDoor,
										   SiegeId connectNode, const DWORD connectDoor,
										   bool bConnect, bool bForceComplete )
{
	CHECK_SERVER_ONLY;

	RCRequestNodeConnection( targetNode, targetDoor, connectNode, connectDoor, bConnect, bForceComplete );
}


// Request the connection of two nodes
FuBiCookie WorldTerrain :: RCRequestNodeConnection( SiegeId targetNode, const DWORD targetDoor,
												  SiegeId connectNode, const DWORD connectDoor,
												  bool bConnect, bool bForceComplete )
{
	FUBI_RPC_CALL_RETRY( RCRequestNodeConnection, RPC_TO_ALL );

	siege::database_guid targetGuid( (unsigned int)targetNode );
	siege::database_guid connectGuid( (unsigned int)connectNode );

	gSiegeEngine.NodeConnection( targetGuid, targetDoor, connectGuid, connectDoor, bConnect, bForceComplete );

	return RPC_SUCCESS;
}


// Query path connection information
bool WorldTerrain :: IsNodalPathingConnected( SiegeId targetNode, SiegeId connectNode )
{
	CHECK_SERVER_ONLY;

	siege::database_guid targetGuid( (unsigned int)targetNode );
	siege::database_guid connectGuid( (unsigned int)connectNode );

	return gSiegeEngine.IsNodalPathingConnected( targetGuid, connectGuid );
}

#if !GP_RETAIL
static std::set< Goid > blockSet;
#endif

// Request the opening of a door
void WorldTerrain :: RequestObjectUnblock( Goid object )
{
#	if !GP_RETAIL
	if( blockSet.find( object ) == blockSet.end() )
	{
		gperror( "Cannot unblock object that was never blocked!  DO NOT IGNORE, TELL JAMES!" );
		return;
	}
	else
	{
		blockSet.erase( object );
	}
#	endif

	// Find logical leaves that were affected by this door being closed, and unmark them
	GoHandle hObject( object );
	if ( hObject && hObject->HasAspect() && hObject->GetPlacement()->GetPosition().node.IsValidSlow() )
	{
#		if !GP_RETAIL
		if( hObject->GetAspect()->GetAspectPtr()->IsPurged( 2 ) )
		{
			gperror( "Cannot unblock purged aspect!  DO NOT IGNORE, TELL JAMES!" );
			return;
		}
#		endif

		// Get the oriented bounding box
		vector_3	collidable_center( DoNotInitialize );
		matrix_3x3	collidable_orient( DoNotInitialize );
		vector_3	collidable_hdiag ( DoNotInitialize );

 		hObject->GetAspect()->GetWorldSpaceOrientedBoundingVolume( collidable_orient, collidable_center, collidable_hdiag );
		collidable_hdiag.x	= max_t( collidable_hdiag.x, 0.1f );
		collidable_hdiag.z	= max_t( collidable_hdiag.z, 0.1f );

		// Modify the half diagonal
		collidable_hdiag	+= Transpose( collidable_orient ) * 
							   ((collidable_orient * collidable_hdiag).Normalize_T() * vector_3( 0.01f, 0.11f, 0.01f ));

		// Build our info structure for callback
		BlockRequestInfo blockInfo;
		blockInfo.objectObb	= oriented_bounding_box_3( collidable_center, collidable_hdiag, collidable_orient );
		blockInfo.bStatus	= false;

#		if !GP_RETAIL
		if ( gSiegeEngine.GetOptions().IsCollisionBoxes() )
		{
			gWorld.DrawDebugBox( SiegePos( collidable_center, hObject->GetPlacement()->GetPosition().node ),
								 collidable_orient, collidable_hdiag, 0xFFFF00FF, 5.0f, true );
		}
#		endif // !GP_RETAIL

		gSiegeEngine.NodeTraversalCallback( hObject->GetPlacement()->GetPosition().node,
											makeFunctor( *this, &WorldTerrain :: BlockRequestCallBack ),
											2,
											&blockInfo );
	}
}


// Request the closing of a door
void WorldTerrain :: RequestObjectBlock( Goid object )
{
#	if !GP_RETAIL
	if( blockSet.find( object ) != blockSet.end() )
	{
		gperror( "Cannot block twice on same object!  DO NOT IGNORE, TELL JAMES!" );
		return;
	}
	else
	{
		blockSet.insert( object );
	}
#	endif

	// Find logical leaves that were affected by this door being closed, and unmark them
	GoHandle hObject( object );
	if ( hObject && hObject->HasAspect() )
	{
#		if !GP_RETAIL
		if( hObject->GetAspect()->GetAspectPtr()->IsPurged( 2 ) )
		{
			gperror( "Cannot block purged aspect!  DO NOT IGNORE, TELL JAMES!" );
			return;
		}
#		endif

		// Get the oriented bounding box
		vector_3	collidable_center( DoNotInitialize );
		matrix_3x3	collidable_orient( DoNotInitialize );
		vector_3	collidable_hdiag ( DoNotInitialize );

 		hObject->GetAspect()->GetWorldSpaceOrientedBoundingVolume( collidable_orient, collidable_center, collidable_hdiag );
		collidable_hdiag.x	= max_t( collidable_hdiag.x, 0.1f );
		collidable_hdiag.z	= max_t( collidable_hdiag.z, 0.1f );

		// Modify the half diagonal
		collidable_hdiag	+= Transpose( collidable_orient ) * 
							   ((collidable_orient * collidable_hdiag).Normalize_T() * vector_3( 0.0f, 0.1f, 0.0f ));

		// Build our info structure for callback
		BlockRequestInfo blockInfo;
		blockInfo.objectObb	= oriented_bounding_box_3( collidable_center, collidable_hdiag, collidable_orient );
		blockInfo.bStatus	= true;

#		if !GP_RETAIL
		if ( gSiegeEngine.GetOptions().IsCollisionBoxes() )
		{
			gWorld.DrawDebugBox( SiegePos( collidable_center, hObject->GetPlacement()->GetPosition().node ),
								 collidable_orient, collidable_hdiag, 0xFFFF00FF, 5.0f, true );
		}
#		endif // !GP_RETAIL

		gSiegeEngine.NodeTraversalCallback( hObject->GetPlacement()->GetPosition().node,
											makeFunctor( *this, &WorldTerrain :: BlockRequestCallBack ),
											2,
											&blockInfo );
	}
}


// Find the local index of a nodal based texture
int WorldTerrain :: FindNodalTextureIndex( SiegeId node, const gpstring& texName )
{
	siege::database_guid nodeGuid( (unsigned int)node );

	// Get the node
	SiegeNode* pNode	= gSiegeEngine.IsNodeValid( nodeGuid );
	if( pNode )
	{
		return( pNode->FindTextureIndex( texName ) );
	}

	return -1;
}


// Request nodal texture state change
void WorldTerrain :: SSetNodalTextureAnimationState( SiegeId node, int tex_index, bool bState )
{
	CHECK_SERVER_ONLY;

	RCSetNodalTextureAnimationState( node, tex_index, bState );
}

// Request nodal texture state change
FuBiCookie WorldTerrain :: RCSetNodalTextureAnimationState( SiegeId node, int tex_index, bool bState )
{
	FUBI_RPC_CALL_RETRY( RCSetNodalTextureAnimationState, RPC_TO_ALL );

	if ( tex_index != 0xFFFFFFFF )
	{
		siege::database_guid nodeGuid( (unsigned int)node );

		// Tell the engine about it
		gSiegeEngine.SetNodalTextureAnimationState( nodeGuid, tex_index, bState );
	}

	return RPC_SUCCESS;
}


bool WorldTerrain :: GetNodalTextureAnimationState( SiegeId node, int tex_index )
{
	if ( tex_index != 0xFFFFFFFF )
	{
		siege::database_guid nodeGuid( (unsigned int)node );

		// Get info from engine
		return gSiegeEngine.GetNodalTextureAnimationState( nodeGuid, tex_index );
	}

	return false;
}


// Request nodal texture speed change
void WorldTerrain :: SSetNodalTextureAnimationSpeed( SiegeId node, int tex_index, float speed )
{
	CHECK_SERVER_ONLY;

	RCSetNodalTextureAnimationSpeed( node, tex_index, speed );
}

FuBiCookie WorldTerrain :: RCSetNodalTextureAnimationSpeed( SiegeId node, int tex_index, float speed )
{
	FUBI_RPC_CALL_RETRY( RCSetNodalTextureAnimationSpeed, RPC_TO_ALL );

	if ( tex_index != 0xFFFFFFFF )
	{
		siege::database_guid nodeGuid( (unsigned int)node );

		// Tell the engine about it
		gSiegeEngine.SetNodalTextureAnimationSpeed( nodeGuid, tex_index, speed );
	}

	return RPC_SUCCESS;
}


float WorldTerrain :: GetNodalTextureAnimationSpeed( SiegeId node, int tex_index )
{
	if ( tex_index != 0xFFFFFFFF )
	{
		siege::database_guid nodeGuid( (unsigned int)node );

		// Get info from engine
		return gSiegeEngine.GetNodalTextureAnimationSpeed( nodeGuid, tex_index );
	}

	return 0.0f;
}


// Request nodal texture replacement
void WorldTerrain :: SReplaceNodalTexture( SiegeId node, int tex_index, const gpstring& newTexName )
{
	CHECK_SERVER_ONLY;

	RCReplaceNodalTexture( node, tex_index, newTexName );
}

FuBiCookie WorldTerrain :: RCReplaceNodalTexture( SiegeId node, int tex_index, const gpstring& newTexName )
{
	FUBI_RPC_CALL_RETRY( RCReplaceNodalTexture, RPC_TO_ALL );

	if ( tex_index != 0xFFFFFFFF )
	{
		siege::database_guid nodeGuid( (unsigned int)node );

		// Tell the engine about it
		gSiegeEngine.ReplaceNodalTexture( nodeGuid, tex_index, newTexName );
	}

	return RPC_SUCCESS;
}


void WorldTerrain :: SNodeFade( const database_guid &nodeGuid, FADETYPE ft )
{
	RCNodeFade( nodeGuid.GetValue(), (UINT8)ft );
}


FuBiCookie WorldTerrain :: RCNodeFade( const UINT32 nodeGuid, UINT8 ft )
{
	FUBI_RPC_CALL_RETRY( RCNodeFade, RPC_TO_ALL );

	database_guid makeNodeGuid;
	makeNodeGuid.SetValue( nodeGuid );
	gSiegeEngine.NodeFade( makeNodeGuid, (FADETYPE)ft );

	return RPC_SUCCESS;
}


void WorldTerrain :: SGlobalNodeFade( int regionID, int section, int level, int object, FADETYPE ft )
{
	RCGlobalNodeFade( regionID, section, level, object, (UINT8)ft );
}


FuBiCookie WorldTerrain :: RCGlobalNodeFade( int regionID, int section, int level, int object, UINT8 ft )
{
	FUBI_RPC_CALL_RETRY( RCGlobalNodeFade, RPC_TO_ALL );

	gSiegeEngine.GlobalNodeFade( regionID, section, level, object, (FADETYPE)ft );

	return RPC_SUCCESS;
}


FuBiCookie WorldTerrain :: RCSyncGlobalNodeFade( DWORD machineId, int regionID, int section, int level, int object, UINT8 ft )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSyncGlobalNodeFade, machineId );

	gSiegeEngine.GlobalNodeFade( regionID, section, level, object, (FADETYPE)ft );

	return RPC_SUCCESS;
}

// Managed node flags
void WorldTerrain :: SSetNodeOccludesCamera( const database_guid nodeGuid, bool bOccludes )
{
	CHECK_SERVER_ONLY;

	RCSetNodeOccludesCamera( nodeGuid.GetValue(), bOccludes );
}


FuBiCookie WorldTerrain :: RCSetNodeOccludesCamera( const UINT32 nodeGuid, bool bOccludes )
{
	FUBI_RPC_CALL_RETRY( RCSetNodeOccludesCamera, RPC_TO_ALL );

	database_guid makeNodeGuid;
	makeNodeGuid.SetValue( nodeGuid );
	gSiegeEngine.SetNodeOccludesCamera( makeNodeGuid, bOccludes );

	return RPC_SUCCESS;
}


void WorldTerrain :: SSetNodeBoundsCamera( const database_guid nodeGuid, bool bBounds )
{
	CHECK_SERVER_ONLY;

	RCSetNodeBoundsCamera( nodeGuid.GetValue(), bBounds );
}


FuBiCookie WorldTerrain :: RCSetNodeBoundsCamera( const UINT32 nodeGuid, bool bBounds )
{
	FUBI_RPC_CALL_RETRY( RCSetNodeBoundsCamera, RPC_TO_ALL );

	database_guid makeNodeGuid;
	makeNodeGuid.SetValue( nodeGuid );
	gSiegeEngine.SetNodeBoundsCamera( makeNodeGuid, bBounds );

	return RPC_SUCCESS;
}


void WorldTerrain :: SSetNodeCameraFade( const database_guid nodeGuid, bool bFade )
{
	CHECK_SERVER_ONLY;

	RCSetNodeCameraFade( nodeGuid.GetValue(), bFade );
}


FuBiCookie WorldTerrain :: RCSetNodeCameraFade( const UINT32 nodeGuid, bool bFade )
{
	FUBI_RPC_CALL_RETRY( RCSetNodeCameraFade, RPC_TO_ALL );

	database_guid makeNodeGuid;
	makeNodeGuid.SetValue( nodeGuid );
	gSiegeEngine.SetNodeCameraFade( makeNodeGuid, bFade );

	return RPC_SUCCESS;
}


// Synchronize the nodal transitions between server and given machine
void WorldTerrain :: SSyncTransitionsOnMachine( DWORD machineId )
{
	CHECK_SERVER_ONLY;

	FuBi::BitPacker packer;
	gSiegeEngine.GetTransition().XferForSync( packer );

	RCSyncTransitionOnMachine( machineId, packer.GetSavedBits() );
}


FuBiCookie WorldTerrain :: RCSyncTransitionOnMachine( DWORD machineId, const_mem_ptr data )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSyncTransitionOnMachine, machineId );

	FuBi::BitPacker packer( data );
	gSiegeEngine.GetTransition().XferForSync( packer );

	return ( RPC_SUCCESS );
}


// Synchronize the global fades between server and given machine
void WorldTerrain :: SSyncGlobalFadesOnMachine( DWORD machineId )
{
	CHECK_SERVER_ONLY;

	for( siege::NodeFadeColl::const_iterator i = gSiegeEngine.GetGlobalFadeColl().begin(); i != gSiegeEngine.GetGlobalFadeColl().end(); ++i )
	{
		// Make RC call to this machine
		RCSyncGlobalNodeFade( machineId, (*i).m_regionID, (*i).m_section, (*i).m_level, (*i).m_object, (UINT8)(*i).m_fadeType );
	}
}

// Synchronize the nodal texture states between server and given machine
void WorldTerrain :: SSyncNodalTexStatesOnMachine( DWORD machineId )
{
	CHECK_SERVER_ONLY;

	FileSys::BufferWriter::Buffer data, index;
	FileSys::BufferWriter dataWriter( data ), indexWriter( index );

	FuBi::TreeBinaryWriter writer( dataWriter );
	FuBi::PersistContext persist( &writer );
	gSiegeEngine.XferNodeTexMap( persist );
	writer.WriteIndex( indexWriter );

	RCSyncNodalTexStatesOnMachine( machineId, const_mem_ptr( &*data.begin(), data.size() ), const_mem_ptr( &*index.begin(), index.size() ) );
}


FuBiCookie WorldTerrain :: RCSyncNodalTexStatesOnMachine( DWORD machineId, const_mem_ptr data, const_mem_ptr index )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSyncNodalTexStatesOnMachine, machineId );

	FuBi::TreeBinaryReader reader;
	reader.Init( data, index );
	FuBi::PersistContext persist( &reader );
	gSiegeEngine.XferNodeTexMap( persist );

	return ( RPC_SUCCESS );
}

// Synchronize the nodal flag states between server and given machine
void WorldTerrain :: SSyncNodalFlagStatesOnMachine( DWORD machineId )
{
	CHECK_SERVER_ONLY;

	FileSys::BufferWriter::Buffer data, index;
	FileSys::BufferWriter dataWriter( data ), indexWriter( index );

	FuBi::TreeBinaryWriter writer( dataWriter );
	FuBi::PersistContext persist( &writer );
	gSiegeEngine.XferNodeFlagMap( persist );
	writer.WriteIndex( indexWriter );

	RCSyncNodalFlagStatesOnMachine( machineId, const_mem_ptr( &*data.begin(), data.size() ), const_mem_ptr( &*index.begin(), index.size() ) );
}


FuBiCookie WorldTerrain :: RCSyncNodalFlagStatesOnMachine( DWORD machineId, const_mem_ptr data, const_mem_ptr index )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSyncNodalFlagStatesOnMachine, machineId );

	FuBi::TreeBinaryReader reader;
	reader.Init( data, index );
	FuBi::PersistContext persist( &reader );
	gSiegeEngine.XferNodeFlagMap( persist );

	return ( RPC_SUCCESS );
}


void WorldTerrain :: NodeTransitionComplete( siege::database_guid targetNode, DWORD owner )
{
	if( gServer.IsLocal() )
	{
		// Send WorldMessage to owner telling it that we are done with the transition now
		WorldMessage msg( WE_TERRAIN_TRANSITION_DONE, MakeGoid(owner) );
		msg.Send();

		GoidColl occupants;
		gGoDb.GetNodeOccupants( targetNode, occupants );
		for( GoidColl::iterator i = occupants.begin(); i != occupants.end(); ++i )
		{
			msg.SetSendFrom( GOID_INVALID );
			msg.SetSendTo( (*i) );
			msg.Send();
		}

		// Tell AIQ to invalidate any queries in the vicinity
		gAIQuery.OnNodeTransition( targetNode );
	}
}

// Callback for door request traversal
bool WorldTerrain :: BlockRequestCallBack( const siege::SiegeNode& node, void* pAppDefined )
{
	BlockRequestInfo *pInfo	= (BlockRequestInfo*)pAppDefined;

	bool HasMadeChange = false;

	for ( unsigned int i = 0; i < node.GetNumLogicalNodes(); ++i )
	{
		oriented_bounding_box_3 logicalBox( node.NodeToWorldSpace( node.GetLogicalNodes()[ i ]->GetLogicalMesh()->GetCenter() ),
											node.GetLogicalNodes()[ i ]->GetLogicalMesh()->GetHalfDiag(),
											node.GetCurrentOrientation() );

		if ( pInfo->objectObb.CheckForContact( logicalBox ) )
		{
			// Synchro threads
			kerneltool::Critical::Lock autoLock( gSiegeEngine.GetLogicalCritical() );

			// Go through each leaf of this logical node and see if it collides
			unsigned int numLeaves		= node.GetLogicalNodes()[ i ]->GetLogicalMesh()->GetNumLeafConnections();
			LMESHLEAFINFO* pLeafInfo	= node.GetLogicalNodes()[ i ]->GetLogicalMesh()->GetLeafConnectionInfo();
			for ( unsigned int o = 0; o < numLeaves; ++o )
			{
				// Build oriented box for this leaf
				oriented_bounding_box_3 leafBox( node.NodeToWorldSpace( pLeafInfo[ o ].center ),
												 (pLeafInfo[ o ].maxBox - pLeafInfo[ o ].minBox) * 0.5f,
												 node.GetCurrentOrientation() );

				if ( pInfo->objectObb.CheckForContact( leafBox ) )
				{
					// Mark this leaf
					node.GetLogicalNodes()[ i ]->MarkLeafAsBlocked( pLeafInfo[ o ].id, pInfo->bStatus );
					HasMadeChange |= pInfo->bStatus;
				}
			}
		}
	}

	if (HasMadeChange)
	{
		gMCPManager.NotifyNodeStateChange(node.GetGUID());
	}

	return true;
}


//////////////////////////////////////////////////////////////////////////////
