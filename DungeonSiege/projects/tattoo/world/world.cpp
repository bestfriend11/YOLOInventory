//////////////////////////////////////////////////////////////////////////////
//
// File     :  World.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"

#include "AiAction.h"
#include "AiQuery.h"
#include "AppModule.h"
#include "CameraAgent.h"
#include "Contentdb.h"
#include "DebugProbe.h"
#include "Rules.h"
#include "Filter_1.h"
#include "FuBiPersist.h"
#include "FuBiPersistImpl.h"
#include "FuBiTraits.h"
#include "FuelDb.h"
#include "GameAuditor.h"
#include "Go.h"
#include "GoDb.h"
#include "GoRpc.h"
#include "GoSupport.h"
#include "Gps_manager.h"
#include "Trigger_Sys.h"
#include "GpProfiler.h"
#include "MCP.h"
#include "MessageDispatch.h"
#include "Mood.h"
#include "NetFuBi.h"
#include "NetLog.h"
#include "Player.h"
#include "Sim.h"
#include "rapiprimitive.h"
#include "RatioStack.h"
#include "Rules.h"
#include "ReportSys.h"
#include "server.h"
#include "Services.h"
#include "Siege_Camera.h"
#include "Siege_Engine.h"
#include "StringTool.h"
#include "TimeOfDay.h"
#include "TimeMgr.h"
#include "Victory.h"
#include "Weather.h"
#include "World.h"
#include "WorldFx.h"
#include "WorldMap.h"
#include "WorldOptions.h"
#include "WorldState.h"
#include "WorldTerrain.h"
#include "WorldTime.h"
#include "WorldPathfinder.h"
#include "WorldSound.h"


//////////////////////////////////////////////////////////////////////////////
// class WorldTime implementation


const float TIME_BROADCAST_PERIOD		= 1.0f;
const float MAX_DISCIPLINE_PER_SEC		= 0.5f; //variation around average latency between client and server
const float FORCE_TIME_THREASHOLD		= 2.0f;


WorldTime :: WorldTime()
{
	Init();
	m_pTimeDrift = new AverageAccumulator( 6 );
}


WorldTime::~WorldTime()
{
	Delete( m_pTimeDrift );
}


void WorldTime::Init()
{
	m_SimCount = 1;
	ClearTime();
}


void WorldTime::ClearTime()
{
	m_Time                  = 0.0;
	m_SecondsElapsed		= 0;
	m_TimeBroadcastTime		= 0;
}


bool WorldTime :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_Time",           m_Time           );
	persist.Xfer( "m_SecondsElapsed", m_SecondsElapsed );
	persist.Xfer( "m_SimCount",       m_SimCount       );

	return ( true );
}


void WorldTime :: Update( float SecondsElapsedSinceLastSim )
{
	gpassert( SecondsElapsedSinceLastSim >= 0 );
	m_Time = PreciseAdd( m_Time, SecondsElapsedSinceLastSim );
	m_SecondsElapsed = SecondsElapsedSinceLastSim;
	++m_SimCount;

	if( gWorld.IsMultiPlayer() )
	{
		if( gServer.IsLocal() )
		{
			if( m_TimeBroadcastTime <= m_Time )
			{
				RCSetServerTime( m_Time );
				m_TimeBroadcastTime = PreciseAdd( m_Time, TIME_BROADCAST_PERIOD);
			}
		}
		
	}
}


void WorldTime :: RCSetServerTime( double time )
{
	FUBI_RPC_CALL( RCSetServerTime, RPC_TO_ALL );

	gpnetf(( "RCSetServerTime - time = %2.3f\n", time ));

	if( gServer.IsRemote() )
	{
		gpnetf(("TIMELOGB drift=%3.3f,servertime=%3.3f,clienttime=%3.3f\n",time - m_Time,time,m_Time));

		if( fabs( PreciseSubtract(GetTime(), time) ) > FORCE_TIME_THREASHOLD )
		{
			ForceTime( time );
		}
		else
		{
			m_pTimeDrift->AddSample( float( PreciseSubtract(time, m_Time) ) );

			float drift = m_pTimeDrift->GetAverage();
			float adjustment = 0;

			if( drift > 0 )
			{
				adjustment = min( MAX_DISCIPLINE_PER_SEC , drift );
				
			}
			else if( drift < 0 )
			{
				adjustment = max( -MAX_DISCIPLINE_PER_SEC , drift );
				
			}

			m_Time = PreciseAdd( m_Time, adjustment/3.0f);

			gpnetf(("TIMELOGA time=%3.3f,adjustment=%3.3f,average=%3.3f\n",m_Time,adjustment,drift));
			
		}
	}
}


float WorldTime :: GetLocalRelativeServerTimeDrift()
{
	return m_pTimeDrift->GetAverage();
}

void WorldTime :: ForceTime( double time )
{
	gpgenericf(( "WorldTime :: ForceTime - time set with a %2.4f world time delta\n", time - m_Time ));
	m_Time = time;
}


//////////////////////////////////////////////////////////////////////////////
// class ViewportChangerTimeTarget declaration and implementation

class ViewportChangerTimeTarget : public TimeTarget
{
public:
	SET_INHERITED( ViewportChangerTimeTarget, TimeTarget );

	ViewportChangerTimeTarget( float destWidth, float destHeight, float destFovDegrees )
	{
		m_CurrentTime = 0;

		gWorld.GetWorldViewport( m_StartWidth, m_StartHeight, m_StartFov );

		m_EndWidth  = destWidth;
		m_EndHeight = destHeight;
		m_EndFov    = destFovDegrees;
	}

	virtual void OnUpdate( float deltaTime )
	{
		m_CurrentTime += deltaTime;
		float t = m_CurrentTime / GetDuration();
		t *= FilterSmoothStep( 0.0f, 1.0f, t );
		float width  = (float)(m_StartWidth  + ((m_EndWidth  - m_StartWidth)  * t));
		float height = (float)(m_StartHeight + ((m_EndHeight - m_StartHeight) * t));
		float fov    = (float)(m_StartFov    + ((m_EndFov    - m_StartFov)    * t));
		gWorld.SetWorldViewport( width, height, fov, 0 );
	}

	virtual void OnLastUpdate( float /*deltaTime*/ )
	{
		gWorld.SetWorldViewport( m_EndWidth, m_EndHeight, m_EndFov, 0 );
	}

#	if GP_DEBUG
	virtual gpstring OnGetDebugType( void ) const
		{  return ( "ViewportChangerTimeTarget" );  }
	virtual gpstring OnDebugGetInfo( void ) const
		{  return ( gpstringf( "current time = %f, start height = %f, end height = %f",
										m_CurrentTime, m_StartHeight, m_EndHeight ) );  }
#	endif // GP_DEBUG

private:
	float m_CurrentTime;
	float m_StartWidth, m_EndWidth;
	float m_StartHeight, m_EndHeight;
	float m_StartFov, m_EndFov;

	SET_NO_COPYING( ViewportChangerTimeTarget );
};


//////////////////////////////////////////////////////////////////////////////
// class World implementation

World :: World( bool editing )
{
	// init data members
	m_ViewportWidth    = 1.0;
	m_ViewportHeight   = 1.0;
	m_ViewportFov      = gSiegeEngine.GetCamera().GetFieldOfView();
	m_NormalFov        = m_ViewportFov;

	// clear out data-constructed subsystems
	m_pFlamethrower       = NULL;
	m_pSim                = NULL;
	m_pTimeOfDay          = NULL;
	m_pCameraAgent        = NULL;
	m_pRules              = NULL;
	m_pWeather            = NULL;
	m_pMood               = NULL;
	m_pWorldTerrain       = NULL;
	m_pServer			  = NULL;

	// build permanent subsystems
	m_pOptions         = new WorldOptions;
	m_pMessageDispatch = new MessageDispatch;
	m_pGoRpcMgr        = editing ? NULL : new GoRpcMgr;
	m_pNetLog          = editing ? NULL : new NetLog;
	m_pTimeMgr         = new TimeMgr;
	m_pDrawTimeMgr     = new DrawTimeMgr;
	m_pTime            = new WorldTime;
	m_pAIAction        = new AIAction;
	m_pAIQuery         = new AIQuery;
	m_pWorldPathFinder = new WorldPathFinder;
	m_pTriggerSys      = new trigger::TriggerSys;
	m_pContentDb       = new ContentDb;
	m_pMCP             = new MCP::Manager;
	m_pWorldSound      = new WorldSound;
	m_pGameAuditor	   = new GameAuditor;
	m_pVictory		   = new Victory;
#	if !GP_RETAIL
	m_pDebugProbe      = new DebugProbe;
#	endif // !GP_RETAIL
}


World :: ~World()
{
	Shutdown( SHUTDOWN_FULL, false );

	// kill permanent subsystems
	Delete( m_pOptions             );
	Delete( m_pGoRpcMgr            );
	Delete( m_pNetLog              );
	Delete( m_pTimeMgr             );
	Delete( m_pDrawTimeMgr         );
	Delete( m_pTime                );
	Delete( m_pAIAction            );
	Delete( m_pAIQuery             );
	Delete( m_pWorldPathFinder     );
	Delete( m_pTriggerSys          );
	Delete( m_pMessageDispatch     );
	Delete( m_pContentDb           );
	Delete( m_pMCP                 );
	Delete( m_pWorldSound		   );
	Delete( m_pGameAuditor		   );
	Delete( m_pVictory			   );
#	if !GP_RETAIL
	Delete( m_pDebugProbe		   );
#	endif // !GP_RETAIL
}


bool World :: Init()
{
	bool success = true;

// Subsystem initialization construction.

	if ( m_pFlamethrower == NULL )
	{
		ScopeTimer timer( "init Flamethrower" );
		m_pFlamethrower = new WorldFx( "world:global:effects" );
	}

	if ( m_pSim == NULL )
	{
		m_pSim = new Sim;
	}

	if ( m_pTimeOfDay == NULL )
	{
		m_pTimeOfDay = new TimeOfDay( "world:global:moods:timeofday" );
	}

	if ( m_pCameraAgent == NULL )
	{
		m_pCameraAgent = new CameraAgent;
	}

	if ( m_pRules == NULL )
	{
		m_pRules = new Rules;
	}

	if ( m_pWeather == NULL )
	{
		m_pWeather = new Weather;
	}

	if ( m_pMood )
	{
		delete m_pMood;
	}
	m_pMood = new Mood;

	if ( m_pWorldTerrain == NULL )
	{
		m_pWorldTerrain = new WorldTerrain;
	}

// External initialization.

	gServices.Reinit();

// Subsystem data initialization.

	if ( !m_pContentDb->IsInitialized() )
	{
		ScopeTimer timer( "init ContentDb" );
		if ( !m_pContentDb->Init() )
		{
			success = false;
		}
	}

	return ( success );
}


bool World :: InitServer( bool multiPlayer )
{
	Delete( m_pServer );

	std::auto_ptr <Server> newServer( new Server( multiPlayer ) );
	newServer->Init( multiPlayer );
	m_pServer = newServer.release();

	return( true );
}


bool World :: Shutdown( eShutdown shutdownType, bool restart )
{
	// shut down the loader so nothing more is committed. this is needed during
	// shutdown of the godb, where siege and nema will clean out any outstanding
	// requests and properly handle ref counting.
	gSiegeEngine.StopLoadingThread( siege::STOP_LOADER_THREAD_ONLY );

	// shut down network so we don't try to RPC to anyone...
	if( NetPipe::DoesSingletonExist() && (shutdownType == SHUTDOWN_FOR_NEW_GAME) )
	{
		gServer.ShutdownTransport( true );	// full shutdown
	}

	m_pTriggerSys->Shutdown();

	// shut down godb always
	if ( GoDb::DoesSingletonExist() )
	{
		gGoDb.Shutdown();
	}

	// shut down flamethrower
	if( WorldFx::DoesSingletonExist() )
	{
		gWorldFx.Shutdown();
	}

	// and sim
	if( Sim::DoesSingletonExist() )
	{
		gSim.Shutdown();
	}

	// reset the MCP
	if( MCP::Manager::DoesSingletonExist() )
	{
		gMCP.Reset();
	}

	// clean out these objects always
	m_pMessageDispatch->Shutdown();
	m_pAIQuery->Shutdown();
	m_pGameAuditor->Reset();
	m_pVictory->Init();

	m_pTriggerSys->Shutdown();

	// kill some more stuff
	if (   (shutdownType == SHUTDOWN_FULL)
		|| (shutdownType == SHUTDOWN_FOR_RELOAD_GAME)
		|| (shutdownType == SHUTDOWN_FOR_NEW_MAP)
		|| (shutdownType == SHUTDOWN_FOR_NEW_GAME) )
	{
		if ( m_pServer != NULL )
		{
			m_pServer->Shutdown( shutdownType );
		}

		if ( shutdownType != SHUTDOWN_FOR_NEW_MAP )
		{
			Delete( m_pServer );
		}

		// unload the map? only if we're not loading a new game
		if ( WorldMap::DoesSingletonExist() && (shutdownType != SHUTDOWN_FOR_RELOAD_GAME) )
		{
			gWorldMap.Shutdown();
		}

		// Shutdown the mood
		if ( m_pMood != NULL )
		{
			if ( ReportSys::IsFatalOccurring() )
			{
				Delete( m_pMood );
			}
			else
			{
				m_pMood->Shutdown();
			}
		}
	}

	// kill everything maybe
	if (   (shutdownType == SHUTDOWN_FULL)
		|| (shutdownType == SHUTDOWN_FOR_RELOAD_GAME) )
	{
		// kill data-constructed subsystems
		if ( shutdownType == SHUTDOWN_FULL )
		{
			Delete( m_pFlamethrower );
		}
		Delete( m_pSim         );
		Delete( m_pTimeOfDay   );
		Delete( m_pCameraAgent );
		Delete( m_pRules       );
		Delete( m_pWeather     );
		Delete( m_pMood        );

		// kill volatile subsystems
		if ( shutdownType == SHUTDOWN_FULL )
		{
			Delete( m_pWorldTerrain );
		}

		// kill contentdb
		if ( shutdownType == SHUTDOWN_FULL )
		{
			gContentDb.Shutdown();
		}

		// kill services
		gServices.Shutdown( true );

		// unload all gas
		if ( shutdownType == SHUTDOWN_FULL )
		{
			FuelHandle globalFuel( "root" );
			gpassert( globalFuel );
			globalFuel.Unload();
		}
	}

	// now that we're fully shut down, clear out any orders that are in there
	gSiegeEngine.StopLoadingThread( siege::STOP_LOADER_AND_CANCEL );

	// validate some other things
#	if !GP_RETAIL
	if ( GoDb::DoesSingletonExist() && (gGoDb.GetTotalGoCount() != 0) )
	{
		gperrorf(( "SERIOUS ERROR: the World was shut down yet there are still %d Go's remaining!\n", gGoDb.GetTotalGoCount() ));
	}
	if ( Sim::DoesSingletonExist() && !gSim.IsEmpty() )
	{
		ReportSys::AutoReport autoReport( &gErrorContext );
		gperror( "SERIOUS ERROR: the GoDb was shut down yet there are still simulations running!\n\n" );
		FuBi::ReportWriter writer( &gErrorContext );
		FuBi::PersistContext persist( &writer );
		try
		{
			gSim.Xfer( persist );
		}
		catch ( ... )  {  }
	}
	if ( WorldFx::DoesSingletonExist() && !gWorldFx.IsEmpty() )
	{
		ReportSys::AutoReport autoReport( &gErrorContext );
		gperror( "SERIOUS ERROR: the World was shut down yet there are still effects running!\n" );
		FuBi::ReportWriter writer( &gErrorContext );
		FuBi::PersistContext persist( &writer );
		try
		{
			gWorldFx.Xfer( persist );
		}
		catch ( ... )  {  }
	}
#	endif // !GP_RETAIL

	// restart?
	return ( restart ? Init() : true );
}


FUBI_DECLARE_SELF_TRAITS( WorldOptions                );
FUBI_DECLARE_SELF_TRAITS( WorldTime                   );
FUBI_DECLARE_SELF_TRAITS( MessageDispatch             );
FUBI_DECLARE_SELF_TRAITS( MCP::Manager                );
FUBI_DECLARE_SELF_TRAITS( CameraAgent                 );
FUBI_DECLARE_SELF_TRAITS( WorldFx					  );
FUBI_DECLARE_SELF_TRAITS( Sim						  );
FUBI_DECLARE_SELF_TRAITS( TimeOfDay					  );
FUBI_DECLARE_SELF_TRAITS( Mood						  );
FUBI_DECLARE_SELF_TRAITS( trigger::TriggerSys		  );
FUBI_DECLARE_SELF_TRAITS( GameAuditor				  );
FUBI_DECLARE_SELF_TRAITS( Victory					  );


bool World :: Xfer( FuBi::PersistContext& persist )
{

// Persist terrain and map first (many dependents).

	// persist terrain
	{
		RatioSample ratioSample( "", 0, persist.IsSaving() ? 0.014 : 0.58 );
		persist.EnterBlock( "m_pWorldTerrain" );
		if ( !m_pWorldTerrain->Xfer( persist ) )
		{
			return ( false );
		}
		persist.LeaveBlock();
	}

// Persist local world.

	persist.EnterBlock( "World" );

	persist.Xfer( "m_ViewportWidth",		m_ViewportWidth		);
	persist.Xfer( "m_ViewportHeight",		m_ViewportHeight	);
	persist.Xfer( "m_ViewportFov",			m_ViewportFov		);
	persist.Xfer( "m_NormalFov",			m_NormalFov			);
	persist.Xfer( "m_pOptions",             *m_pOptions			);
	persist.Xfer( "m_pTime",                *m_pTime			);
	persist.Xfer( "m_pMessageDispatch",     *m_pMessageDispatch	);
	persist.Xfer( "m_pTimeOfDay",           *m_pTimeOfDay		);
	persist.Xfer( "m_pCameraAgent",         *m_pCameraAgent		);
	persist.Xfer( "m_pMood",                *m_pMood			);
	persist.Xfer( "m_pTriggerSys",			*m_pTriggerSys		);
	persist.Xfer( "m_pGameAuditor",			*m_pGameAuditor		);
	persist.Xfer( "m_pVictory",				*m_pVictory			);

	persist.LeaveBlock();

// Persist major systems.

	// $ important: godb must be persisted after services. gobody requires that
	//   the aspects are set up before it can xfer.

	// persist services
	{
		RatioSample ratioSample( "", 0, persist.IsSaving() ? 0.21 : 0.04 );
		persist.EnterBlock( "Services" );
		gServices.Xfer( persist );
		persist.LeaveBlock();
	}

	// persist godb
	{
		RatioSample ratioSample( "" );
		persist.EnterBlock( "GoDb" );
		gGoDb.Xfer( persist );
		persist.LeaveBlock();
	}

	// persist systems that require go's to be xfer'd first
	persist.Xfer( "m_pMCP",          *m_pMCP          );
	persist.Xfer( "m_pFlamethrower", *m_pFlamethrower );
	persist.Xfer( "m_pSim",          *m_pSim          );

	return ( true );
}


bool World :: IsSinglePlayer() const
{
	return ( !Server::DoesSingletonExist() || gServer.IsSinglePlayer() );
}


bool World :: IsMultiPlayer() const
{
	return( Server::DoesSingletonExist() && gServer.IsMultiPlayer() );
}


void World :: OnAppActivate( bool activate )
{
	// $$$ fill this in - reset the camera, tell server, etc...
	if( activate )
	{
		// Force update the shadows to handle lost surfaces
		gSiegeEngine.NodeWalker().ForceShadowGeneration();
	}
}

void World :: SetWorldViewport( float width, float height, float fovDegrees, float duration )
{
	float fov = DegreesToRadians( fovDegrees );

	if (   IsZero( duration )
		|| (   IsEqual( width,  m_ViewportWidth )
			&& IsEqual( height, m_ViewportHeight )
			&& IsEqual( fov,    m_ViewportFov )) )
	{
		m_ViewportWidth  = width;
		m_ViewportHeight = height;
		m_ViewportFov    = fov;
	}
	else
	{
		gTimeMgr.AddDurationTarget( new ViewportChangerTimeTarget( width, height, fovDegrees ), duration, true );
	}
}


void World :: RestoreWorldViewport( float duration )
{
	SetWorldViewport( 1, 1, RadiansToDegrees( m_NormalFov ), duration );
}


void World :: GetWorldViewport( float& width, float& height, float& fovDegrees )
{
	width      = m_ViewportWidth;
	height     = m_ViewportHeight;
	fovDegrees = RadiansToDegrees( m_ViewportFov );
}


void World :: SetViewportWorld( void )
{
	// Get the camera
	siege::SiegeCamera& camera = gSiegeEngine.GetCamera();

	// Change the viewport
	camera.SetViewportNormalizedWidth( m_ViewportWidth );
	camera.SetViewportNormalizedHeight( m_ViewportHeight );
	camera.SetFieldOfView( m_ViewportFov );

	// Update the camera
	camera.UpdateCamera();
}


void World :: SetViewportDefault( void )
{
	// Get the camera
	siege::SiegeCamera& camera = gSiegeEngine.GetCamera();

	// Change the viewport
	camera.SetViewportNormalizedWidth( 1.0f );
	camera.SetViewportNormalizedHeight( 1.0f );
	camera.SetFieldOfView( m_NormalFov );

	// Update the camera
	camera.UpdateCamera();
}


bool World :: ViewportDiffersFromDefault( void )
{
	if ( !IsEqual( m_ViewportWidth,  1.0f) ||
		 !IsEqual( m_ViewportHeight, 1.0f) ||
		 !IsEqual( m_ViewportFov, m_NormalFov ) )
	{
		return ( true );
	}

	return ( false );
}


void World :: RegisterInventoryUpdateCallback( InventoryUpdateCb callback )
{
	m_InventoryUpdateCallback = callback;
}


void World :: UpdateInventoryGUICallback( Goid clientId )
{
	if ( m_InventoryUpdateCallback )
	{
		m_InventoryUpdateCallback( clientId );
	}
}


void World :: RegisterTestEquipCallback( TestEquipCb callback )
{
	m_TestEquipCallback = callback;	
}


void World :: TestEquipCallback( Goid clientId )
{
	if ( m_TestEquipCallback )
	{
		m_TestEquipCallback( clientId );
	}
}


void World :: RegisterDialogueCallback( DialogueCallbackCb callback )
{
	m_DialogueCallback = callback;
}


void World :: RSDialogueCallback( Goid talkerID, Goid member )
{
	GoHandle hClient( talkerID );
	if( hClient.IsValid() )
	{
		RCDialogueCallback( hClient->GetPlayer()->GetMachineId(), talkerID, member );
	}
}


FuBiCookie World :: RCDialogueCallback( DWORD MachineID, Goid talkerID, Goid member )
{
	FUBI_RPC_CALL_RETRY( RCDialogueCallback, MachineID );
	DialogueCallback( talkerID, member );
	return RPC_SUCCESS;
}


void World :: DialogueCallback( Goid talkerID, Goid member )
{
	if ( m_DialogueCallback )
	{
		m_DialogueCallback( talkerID, member );
	}
}


void World :: RegisterPurchaseGuiCallback( PurchaseGuiCb callback )
{ 
	m_PurchaseGuiCb = callback; 
}	


void World :: PurchaseGuiCallback( Goid client, Goid item )
{ 
	if ( m_PurchaseGuiCb ) 
	{ 
		m_PurchaseGuiCb( client, item ); 
	} 
}


void World :: RegisterStashGuiCallback( StashGuiCb callback )
{
	m_StashGuiCb = callback;
}


void World :: StashGuiCallback( Goid client, Goid item )
{
	if ( m_StashGuiCb )
	{
		m_StashGuiCb( client, item );
	}
}

void World :: RegisterRolloverHighlightCallback( RolloverHighlightCb callback )
{
	m_RolloverHighlightCb = callback;
}


void World :: RolloverHighlightCallback( bool & bHighlight )
{
	if ( m_RolloverHighlightCb )
	{
		m_RolloverHighlightCb( bHighlight );
	}
}

void World :: RegisterPartyCallback( PartyCb callback )
{
	m_PartyUpdateCb = callback;
}


void World :: PartyCallback( void )
{	
	if ( m_PartyUpdateCb )
	{
		m_PartyUpdateCb();
	}
}

void World :: RegisterCastCallback( CastCb callback )
{
	m_CastCb = callback;	
}

void World :: CastCallback( Goid spell )
{
	if ( m_CastCb )
	{
		m_CastCb( spell );
	}
}

void World :: Update( float secondsElapsed, float actualDeltaTime )
{
	GPPROFILERSAMPLE( "World :: Update", SP_MISC );

	if( IsInGame( gWorldState.GetCurrentState() ) && ( gWorldState.GetCurrentState() != WS_MP_INGAME_JIP ) )
	{
		m_pAIQuery->Update( secondsElapsed );

		if( gServer.IsLocal() )
		{
			m_pVictory->Update();
		}
	}

	if( m_pServer )
	{
		m_pServer->Update( actualDeltaTime );
	}
}


void World :: Draw( float secondsElapsed )
{
#	if !GP_RETAIL
	DrawDebugBoxes( secondsElapsed );
	DrawDebugLines( secondsElapsed );
#	endif // !GP_RETAIL
}


DWORD World :: PlaySample( gpstring const& Sound, float /*Pitch*/, bool fLoop )
{
	DWORD SampleGUID = GPGSound::INVALID_SOUND_ID;

	if( !Sound.empty() )
	{
		// play the sound
		SampleGUID = gSoundManager.PlaySample( Sound, fLoop );
	}
	return SampleGUID;
}


void World :: DrawDebugLinkedTerrainPoints( SiegePos const & PosA, SiegePos const & PosB, DWORD const Color, gpstring const & sName )
{
#	if !GP_RETAIL

	// $ early out if anything wrong
	if ( !gSiegeEngine.IsNodeValid( PosA.node ) || !gSiegeEngine.IsNodeValid( PosB.node ) )
	{
		return;
	}

	SiegePos A = PosA;
	SiegePos AU = A;
	AU.pos.y += 3.0;

	SiegePos B = PosB;
	SiegePos BU = B;
	BU.pos.y += 3.0;

	DrawDebugDirectedLine( A,  AU, Color, "" );
	DrawDebugDirectedLine( AU, BU, Color, "" );
	DrawDebugDirectedLine( BU, B,  Color, "" );

	if( !sName.empty() && gWorldOptions.TestDebugHudOptions( DHO_LABELS ) )
	{
		vector_3 ToMidPoint = gSiegeEngine.GetDifferenceVector( AU, BU ) * 0.5;
		AU.pos += ToMidPoint;
		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), AU.ScreenPos(), sName );
	}

#	endif // !GP_RETAIL
}


void World :: DrawDebugDashedLine( SiegePos const & PosA, SiegePos const & PosB, DWORD const Color, gpstring const & sName )
{
#	if !GP_RETAIL

	// $ early out if anything wrong
	if ( !gSiegeEngine.IsNodeValid( PosA.node ) || !gSiegeEngine.IsNodeValid( PosB.node ) )
	{
		return;
	}

	vector_3 a( PosA.WorldPos() );
	vector_3 b( PosB.WorldPos() );

	const vector_3 diff = b - a;
	if ( !diff.IsZeroLength() )
	{
		// divide it up (always have odd # segments)
		int segments = max( (int)floorf( diff.Length() ), 1 );
		if ( !(segments & 1) )
		{
			++segments;
		}

		// build offset params
		vector_3 offset = diff / (float)segments;
		vector_3 iter = a;
		bool on = true;

		// now draw dashes
		for ( int i = 0 ; i != segments ; ++i, iter += offset, on = !on )
		{
			if ( on )
			{
				gSiegeEngine.GetWorldSpaceLines().DrawLine( iter, iter + offset, Color );
			}
		}
	}

	if( !sName.empty() && gWorldOptions.TestDebugHudOptions( DHO_LABELS ) )
	{
		SiegePos pos( PosA );
		pos.pos += gSiegeEngine.GetDifferenceVector( PosA, PosB ) * 0.5;
		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), pos.ScreenPos(), sName );
	}

#	endif // !GP_RETAIL
}


void World :: DrawDebugLine( SiegePos const & PosA, SiegePos const & PosB, DWORD const Color, gpstring const & sName )
{
#	if !GP_RETAIL

	// $ early out if anything wrong
	if ( !gSiegeEngine.IsNodeValid( PosA.node ) || !gSiegeEngine.IsNodeValid( PosB.node ) )
	{
		return;
	}

	gSiegeEngine.GetWorldSpaceLines().DrawLine( PosA.WorldPos(), PosB.WorldPos(), Color );

	if( !sName.empty() && gWorldOptions.TestDebugHudOptions( DHO_LABELS ) )
	{
		SiegePos pos( PosA );
		pos.pos += gSiegeEngine.GetDifferenceVector( PosA, PosB ) * 0.5;
		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), pos.ScreenPos(), sName );
	}

#	endif // !GP_RETAIL
}


void World :: DrawDebugLine( const SiegePos& PosA, const SiegePos& PosB, DWORD const Color, const gpstring& sName, float duration )
{
#	if !GP_RETAIL

	// If the game is paused and duration is not zero, well, it can basically fuck off
	if( gAppModule.IsUserPaused() && !IsZero( duration ) )
	{
		duration	= 0.0f;
	}

	DebugLine new_line( PosA, PosB, Color, duration, sName );

	m_DebugLines.push_back( new_line );

#	endif // !GP_RETAIL
}


void World :: DrawDebugDirectedLine( SiegePos const & PosA, SiegePos const & PosB, DWORD const Color, gpstring const & sName )
{
#	if !GP_RETAIL

	// $ early out if anything wrong
	if ( !gSiegeEngine.IsNodeValid( PosA.node ) || !gSiegeEngine.IsNodeValid( PosB.node ) )
	{
		return;
	}

	vector_3 a( PosA.WorldPos() );
	vector_3 b( PosB.WorldPos() );

	// draw line connecting go's
	gSiegeEngine.GetWorldSpaceLines().DrawLine( a, b, Color );

	// get diff
	const vector_3 diff = b - a;

	// length nonzero?
	if ( !diff.IsZeroLength() )
	{
		// divide it up
		float length = diff.Length();
		int segments = max( (int)floorf( length ), 1 );
		length /= segments;

		// get time iter
		static const float ROTATION_PERIOD = 3.0f;
		static const float ARROW_LENGTH = 0.15f;
		static const float ARROW_WIDTH = ARROW_LENGTH * 0.5f;
		float arrowPos = float( Modulus( (float)gWorldTime.GetTime(), ROTATION_PERIOD ) / ROTATION_PERIOD );

		// build offset params
		vector_3 unit = diff.Normalize_T();
		vector_3 offset = unit * length;
		vector_3 iter = a + (offset * arrowPos);
		matrix_3x3 dir = MatrixFromDirection( unit );
		unit *= ARROW_LENGTH;

		// now draw ticks
		for ( int i = 0 ; i != segments ; ++i, iter += offset )
		{
			matrix_3x3 rot = AxisRotationColumns( dir.GetColumn2_T(), (arrowPos * (PI2 + (PI2 * 0.1f))) + (i * (PI2 * 0.1f)) );
			vector_3 tick = Normalize( rot * dir.GetColumn0_T() ) * ARROW_WIDTH;

			gSiegeEngine.GetWorldSpaceLines().DrawLine( iter, iter - unit + tick, Color );
			gSiegeEngine.GetWorldSpaceLines().DrawLine( iter, iter - unit - tick, Color );
		}
	}

	// draw a label
	if ( !sName.empty() && gWorldOptions.TestDebugHudOptions( DHO_LABELS ) )
	{
		SiegePos pos( PosA );
		pos.pos += gSiegeEngine.GetDifferenceVector( PosA, PosB ) * 0.5f;
		pos.pos.y += 0.1f;
		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), pos.ScreenPos(), sName );
	}

#	endif // !GP_RETAIL
}


void World :: DrawDebugTriangle( SiegePos const& Center, float Radius, DWORD const Color, gpstring const& sName )
{
#	if !GP_RETAIL

	// $ early out if anything wrong
	if ( !gSiegeEngine.IsNodeValid( Center.node ) )
	{
		return;
	}

	// adjust the center up a bit

	SiegePos NewCenter = Center;

	// todo: probably shouldn't bias this much...
	NewCenter.pos.y += 0.05f;

	siege::nodespace_lines& Lines = gSiegeEngine.GetNodeSpaceLines();//gSiegeEngine.GetWorldSpaceLines();

	const float RotationPeriod = 15.0;

	const unsigned int Pieces = 3;

	float OrbitPosition = float( Modulus( (float)gWorldTime.GetTime(), RotationPeriod ) / RotationPeriod );

	float AngleIncrement = PI2/float(Pieces);


	//----- draw horizontal disc

	float AngleThisFrame = PI2 * OrbitPosition;

	vector_3 P1, P2;

	float HorizontalAngle = AngleThisFrame;

	{for( unsigned int i = 0; i < Pieces; ++i ) {

		float X1, Y1;
		SINCOSF( HorizontalAngle, Y1, X1 );
		X1	*= Radius;
		Y1	*= Radius;

		P1 = NewCenter.pos;

		P1.z += Y1;
		P1.x += X1;

		HorizontalAngle += AngleIncrement;

		float X2, Y2;
		SINCOSF( HorizontalAngle, Y2, X2 );
		X2	*= Radius;
		Y2	*= Radius;

		P2 = NewCenter.pos;

		P2.z += Y2;
		P2.x += X2;

		Lines.DrawLine( P1, P2, Color, NewCenter.node );
	}}


	// draw label on horizontal disc

	if( !sName.empty() && gWorldOptions.TestDebugHudOptions( DHO_LABELS ) )
	{
		SiegePos LabelPos;
		LabelPos.node = NewCenter.node;
		LabelPos.pos = P2;
		LabelPos.pos.y +=  0.25;

		// todo:label should take the color of the circle, for clarity

		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), sName );
	}

#	endif // !GP_RETAIL
}


void World :: DrawDebugPulsePolygon( SiegePos const& Center, float MinRadius, float MaxRadius, float PulsePeriod, unsigned int Sides, DWORD const Color, gpstring const& sName )
{
#	if !GP_RETAIL

	// $ early out if anything wrong
	if ( !gSiegeEngine.IsNodeValid( Center.node ) )
	{
		return;
	}

	// adjust the center up a bit

	SiegePos NewCenter = Center;

	// todo: probably shouldn't bias this much...
	NewCenter.pos.y += 0.05f;

	siege::nodespace_lines& Lines = gSiegeEngine.GetNodeSpaceLines();//gSiegeEngine.GetWorldSpaceLines();

	const unsigned int Pieces = Sides + 1;

	float Radius = MinRadius + ( (MaxRadius - MinRadius) * float( Modulus( (float)gWorldTime.GetTime(), PulsePeriod ) / PulsePeriod ) );

	//float OrbitPosition = float( Modulus( (float)gWorldTime.GetTime(), RotationPeriod ) / RotationPeriod );

	float AngleIncrement = PI2/float(Pieces);

	//----- draw horizontal disc

	//float AngleThisFrame = PI2 * OrbitPosition;

	vector_3 P1, P2;

	float HorizontalAngle = 0;

	{for( unsigned int i = 0; i < Pieces; ++i ) {

		float X1, Y1;
		SINCOSF( HorizontalAngle, Y1, X1 );
		X1	*= Radius;
		Y1	*= Radius;

		P1 = NewCenter.pos;

		P1.z += Y1;
		P1.x += X1;

		HorizontalAngle += AngleIncrement;

		float X2, Y2;
		SINCOSF( HorizontalAngle, Y2, X2 );
		X2	*= Radius;
		Y2	*= Radius;

		P2 = NewCenter.pos;

		P2.z += Y2;
		P2.x += X2;

		Lines.DrawLine( P1, P2, Color, NewCenter.node );
	}}

	// draw label on horizontal polygon

	if( !sName.empty() && gWorldOptions.TestDebugHudOptions( DHO_LABELS ) )
	{
		SiegePos LabelPos;
		LabelPos.node = NewCenter.node;
		LabelPos.pos = P2;
		LabelPos.pos.y +=  0.25;

		// todo:label should take the color of the circle, for clarity

		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), sName );
	}

#	endif // !GP_RETAIL
}


void World :: DrawDebugCircle( SiegePos const& Center, float const Radius, DWORD const Color, gpstring const& sName )
{
#	if !GP_RETAIL

	// $ early out if anything wrong
	if ( !gSiegeEngine.IsNodeValid( Center.node ) )
	{
		return;
	}

	// adjust the center up a bit

	SiegePos NewCenter = Center;

	// todo: probably shouldn't bias this much...
	NewCenter.pos.y += 0.05f;

	siege::nodespace_lines& Lines = gSiegeEngine.GetNodeSpaceLines();//gSiegeEngine.GetWorldSpaceLines();

	const float RotationPeriod = 15.0;

	const unsigned int Pieces = 32;

	float OrbitPosition;

	if( !sName.empty() )
	{
		OrbitPosition = float( Modulus( (float)gWorldTime.GetTime(), RotationPeriod ) / RotationPeriod );
	}
	else
	{
		OrbitPosition = 0;
	}

	float AngleIncrement = PI2/float(Pieces);


	//----- draw horizontal disc

	float AngleThisFrame = PI2 * OrbitPosition;

	vector_3 P1, P2;

	float HorizontalAngle = AngleThisFrame;

	{for( unsigned int i = 0; i < Pieces; ++i ) {

		float X1, Y1;
		SINCOSF( HorizontalAngle, Y1, X1 );
		X1	*= Radius;
		Y1	*= Radius;

		P1 = NewCenter.pos;

		P1.z += Y1;
		P1.x += X1;

		HorizontalAngle += AngleIncrement;

		float X2, Y2;
		SINCOSF( HorizontalAngle, Y2, X2 );
		X2	*= Radius;
		Y2	*= Radius;

		P2 = NewCenter.pos;

		P2.z += Y2;
		P2.x += X2;

		Lines.DrawLine( P1, P2, Color, NewCenter.node );
	}}


	// draw label on horizontal disc

	if( !sName.empty() && gWorldOptions.TestDebugHudOptions( DHO_LABELS ) )
	{
		SiegePos LabelPos;
		LabelPos.node = NewCenter.node;
		LabelPos.pos = P2;
		LabelPos.pos.y +=  0.25;

		// todo:label should take the color of the circle, for clarity

		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), sName );
	}

#	endif // !GP_RETAIL
}


void World :: DrawDebugWedge( const SiegePos& Center, float Radius, vector_3 const& Direction, float ArcLength, DWORD Color, bool filled)
{
#	if !GP_RETAIL
	
	if ( !gSiegeEngine.IsNodeValid( Center.node ) || IsZero(Radius) )
	{
		return;
	}

	float middleang;

	if ( IsZero(ArcLength) || IsPositive(ArcLength-PI2) || (Direction == vector_3::ZERO) )
	{
		// Draw a full circle
		const float RotationPeriod = 15.0;
		middleang = PI2 * float( Modulus( (float)gWorldTime.GetTime(), RotationPeriod ) / RotationPeriod );
		ArcLength = PI2;
	}
	else
	{
		// convert siegepos to vectors
		vector_3 p0 = Direction;

		p0.y = 0;

		if (IsZero(p0))
		{
			middleang = 0;
		}
		else
		{
			middleang = SignedAngleBetween(vector_3(1,0,0),p0);
		}

	}

	siege::nodespace_lines& nodespace = gSiegeEngine.GetNodeSpaceLines();

	nodespace.DrawCircle(Center.pos,Center.node,Radius,middleang,ArcLength,Color,filled);

#	endif // !GP_RETAIL
}


void World :: DrawDebugWedge( const SiegePos& Center, float Radius, SiegePos const& Direction, float ArcLength, DWORD Color, bool filled)
{
#	if !GP_RETAIL

	if ( !gSiegeEngine.IsNodeValid( Center.node ) || IsZero(Radius) )
	{
		return;
	}

	vector_3 dir;

	if ( IsZero(ArcLength) || IsPositive(ArcLength-PI2) || (Direction == SiegePos::INVALID) )
	{
		// Draw a full circle
		dir = vector_3::ZERO;
	}
	else
	{
		// convert siegepos to vectors
		dir = gSiegeEngine.GetDifferenceVector(Center,Direction);
	}

	DrawDebugWedge( Center, Radius, dir, ArcLength, Color, filled );

#	endif // !GP_RETAIL
}


void World :: DrawDebugSphere( const SiegePos& Center, float Radius, DWORD const Color, const gpstring& sName )
{
#	if !GP_RETAIL

	DrawDebugSphereOffsetAngle( Center, Radius, 0, Color, sName );

#	endif // !GP_RETAIL
}

void World :: DrawDebugSphereOffsetAngle( SiegePos const& Center, float const Radius, float const OffsetAngle, DWORD const Color, gpstring const& sName )
{
#	if !GP_RETAIL

	// $ early out if anything wrong
	if ( !gSiegeEngine.IsNodeValid( Center.node ) )
	{
		return;
	}

	// adjust the center up a bit

	SiegePos NewCenter = Center;

	// todo: probably shouldn't bias this much...
	NewCenter.pos.y += 0.05f;

	siege::nodespace_lines& Lines = gSiegeEngine.GetNodeSpaceLines();//gSiegeEngine.GetWorldSpaceLines();

	const float RotationPeriod = 15.0;
	unsigned int Pieces = 32;

	float OrbitPosition = float( Modulus( (float)gWorldTime.GetTime() + Radius, RotationPeriod ) / RotationPeriod );
	float AngleIncrement = PI2/float(Pieces);

	//----- draw horizontal disc

	float AngleThisFrame = (PI2 * OrbitPosition) + OffsetAngle;

	vector_3 P1, P2;

	float HorizontalAngle = AngleThisFrame;

	{for( unsigned int i = 0; i < Pieces; ++i ) {

		float X1, Y1;
		SINCOSF( HorizontalAngle, Y1, X1 );
		X1	*= Radius;
		Y1	*= Radius;

		P1 = NewCenter.pos;

		P1.z += Y1;
		P1.x += X1;

		HorizontalAngle += AngleIncrement;

		float X2, Y2;
		SINCOSF( HorizontalAngle, Y2, X2 );
		X2	*= Radius;
		Y2	*= Radius;

		P2 = NewCenter.pos;

		P2.z += Y2;
		P2.x += X2;

		Lines.DrawLine( P1, P2, Color, NewCenter.node );
	}}


	// draw label on horizontal disc

	if( !sName.empty() && gWorldOptions.TestDebugHudOptions( DHO_LABELS ) )
	{
		SiegePos LabelPos;
		LabelPos.node = NewCenter.node;
		LabelPos.pos = P2;
		LabelPos.pos.y +=  0.25;

		// todo:label should take the color of the circle, for clarity

		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), sName );
	}

	//----- draw vertical disc

	if( gWorldOptions.TestDebugHudOptions( DHO_DEPTH ) )
	{
		float VerticalAngle = 0.0;

		{for( unsigned int i = 0; i < Pieces; ++i ) {

			float North1, Up1;
			SINCOSF( VerticalAngle, Up1, North1 );
			North1	*= Radius;
			Up1		*= Radius;

			P1.x = 0.0;
			P1.y = Up1;
			P1.z = North1;

			VerticalAngle += AngleIncrement;

			float North2, Up2;
			SINCOSF( VerticalAngle, Up2, North2 );
			North2	*= Radius;
			Up2		*= Radius;

			P2.x = 0.0;
			P2.y = Up2;
			P2.z = North2;

			P1.RotateY( -AngleThisFrame );
			P2.RotateY( -AngleThisFrame );

			P1 += NewCenter.pos;
			P2 += NewCenter.pos;

			Lines.DrawLine( P1, P2, Color, NewCenter.node );
		}}
	}
#	endif // !GP_RETAIL
}

void World :: DrawDebugSphereConeSection( SiegePos const& Position, vector_3 const& Direction, float _Angle, float Radius, DWORD const Color, gpstring const& sName )
{
#	if !GP_RETAIL

	// $ early out if anything wrong
	if ( !gSiegeEngine.IsNodeValid( Position.node ) )
	{
		return;
	}

	float Angle = _Angle;

	// draw cone representation in default orientation ( facing NORTH )
	siege::nodespace_lines& Lines = gSiegeEngine.GetNodeSpaceLines();

	//----- build orientation matrix for later use

	vector_3 z_axis( Normalize( Direction ) );
	vector_3 y_axis( vector_3::UP );
	vector_3 x_axis( CrossProduct( y_axis, z_axis ) );

	if( IsZero( x_axis ) )
	{
		x_axis = vector_3::NORTH;
	}
	else
	{
		x_axis = Normalize( x_axis );
	}
	z_axis = CrossProduct( x_axis, y_axis );
	matrix_3x3 Orientataion = MatrixColumns( x_axis, y_axis, z_axis );


	//----- draw circle around North ( Z ) axis, to represent cylinder base

	const float RotationPeriod = 4.0;

	float TimeAngle = PI2 * float( Modulus( (float)gWorldTime.GetTime(), RotationPeriod ) / RotationPeriod );

	const unsigned int Pieces = 16;

	float AngleIncrement = PI2/float(Pieces);

	float NorthRotation = 0.0;

	vector_3 P1, P2;

	float ConeHeight, ConeBaseRadius;
	SINCOSF( Angle, ConeBaseRadius, ConeHeight );
	ConeBaseRadius	*= Radius;
	ConeHeight		*= Radius;

	{for( unsigned int i = 0; i < Pieces; ++i ) {

		float East1, Up1;
		SINCOSF( NorthRotation, Up1, East1 );
		Up1		*= ConeBaseRadius;
		East1	*= ConeBaseRadius;

		P1.x = East1;
		P1.y = Up1;
		P1.z = ConeHeight;

		NorthRotation += AngleIncrement;

		float East2, Up2;
		SINCOSF( NorthRotation, Up2, East2 );
		Up2		*= ConeBaseRadius;
		East2	*= ConeBaseRadius;

		P2.x = East2;
		P2.y = Up2;
		P2.z = ConeHeight;

		// rotate around North (Z) axis over time...
		P1.RotateZ( TimeAngle );
		P2.RotateZ( TimeAngle );

		P1 = Orientataion * P1;
		P2 = Orientataion * P2;

		P1 += Position.pos;
		P2 += Position.pos;

		// draw node-space lines
		Lines.DrawLine( P1, P2, Color, Position.node );
		Lines.DrawLine( P1, Position.pos, Color, Position.node );
	}}


	//----- draw cylinder cap

	{
		float SweepAngleDelta = float( (2.0*Angle)/7.0 );

		float SweepAngle = Angle;

		for( unsigned int i = 0 ; i < 7 ; ++i )
		{
			SINCOSF( SweepAngle, P1.y, P1.z );
			P1.y *= Radius;
			P1.z *= Radius;
			P1.x = 0;

			SweepAngle -= SweepAngleDelta;

			SINCOSF( SweepAngle, P2.y, P2.z );
			P2.y *= Radius;
			P2.z *= Radius;
			P2.x = 0;

			// rotate around North (Z) axis over time...
			P1.RotateZ( TimeAngle );
			P2.RotateZ( TimeAngle );

			P1 = Orientataion * P1;
			P2 = Orientataion * P2;

			P1 += Position.pos;
			P2 += Position.pos;

			Lines.DrawLine( P1, P2, Color, Position.node );
		}
	}


	// draw label

	if( !sName.empty() && gWorldOptions.TestDebugHudOptions( DHO_LABELS ) )
	{
		SiegePos LabelPos;
		LabelPos.node = Position.node;
		LabelPos.pos = P2;

		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), sName );
	}

#	endif // !GP_RETAIL
}


void World :: DrawDebugPoint( SiegePos const& Center, float Radius, DWORD const Color, gpstring const& sName )
{
#	if !GP_RETAIL

	DrawDebugPoint( Center, Radius, Color, 2.0, sName, DWORD(COLOR_WHITE) );

#	endif // !GP_RETAIL
}


void World :: DrawDebugScreenLabelColor( SiegePos const& Pos, gpstring const & name, DWORD color )
{
#	if !GP_RETAIL

	gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), Pos.ScreenPos(), name,0,true,color );

#	endif // !GP_RETAIL
}


void World :: DrawDebugWorldLabelScroll( SiegePos const& pos, gpstring const & name, DWORD color, float duration, bool scroll )
{
#	if !GP_RETAIL

	// Draw the label with a 0 value as a height will give us automatically scaled text
	gSiegeEngine.GetLabels().DrawLabel( &gServices.GetConsoleFont(), pos , name, duration, 0, true,color, scroll );

#	endif // !GP_RETAIL
}


void World :: DrawDebugPoint( SiegePos const& Center, float radius, DWORD const Color, float height, gpstring const& sName, DWORD labelColor )
{
#	if !GP_RETAIL

	// $ early out if anything wrong
	if ( !gSiegeEngine.IsNodeValid( Center.node ) )
	{
		return;
	}

	if( radius > 0.0 )
	{
		DrawDebugSphere( Center, radius, Color, "" );
	}

	vector_3 A = Center.pos;
	A.y += radius;

	vector_3 B = A;
	B.y += height;

	if( Color != 0 )
	{
		siege::nodespace_lines& Lines = gSiegeEngine.GetNodeSpaceLines();
		Lines.DrawLine( A, B, Color, Center.node );
	}

	SiegePos LabelPos = Center;
	LabelPos.pos = B;

	if ( !sName.empty() && gWorldOptions.TestDebugHudOptions( DHO_LABELS ) )
	{
		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), sName, 0, true, labelColor );
	}

#	endif // !GP_RETAIL
}


void World :: DrawDebugArc( const SiegePos& from, const SiegePos& to, const DWORD color, const gpstring& text, bool drawticks )
{
#if !GP_RETAIL

	if ( !gSiegeEngine.IsNodeValid( from.node ) || !gSiegeEngine.IsNodeValid( to.node ) )		
	{
		return;
	}

	vector_3 fromPos	= from.WorldPos();
	vector_3 toPos		= to.WorldPos();

	const vector_3 p0 = fromPos;
	const vector_3 p1 = ((fromPos + toPos) * 0.5) + ( vector_3::UP * 3.0f );
	const vector_3 p1b = ((fromPos + toPos) * 0.5) + ( vector_3::UP * 2.5f );
	const vector_3 p2 = toPos;

	vector_3 p(p0);
	vector_3 q(vector_3::ZERO);

	const int splineiterations  = 16;
	const float splineIncrement = 1/float(splineiterations);
	float t = splineIncrement;
	vector_3 splineMidPoint;

	for (int j = 0; j < splineiterations; ++j, t += splineIncrement )
	{
		// formula straight outta the book, will optimize --biddle
		q = ((1-t)*(1-t) * p0) + ((2*(1-t)*t) * p1 ) + ((t*t) * p2);
		gSiegeEngine.GetWorldSpaceLines().DrawLine( p, q, color );
		p = q;

		if( j == 7 )
		{
			splineMidPoint = q;
		}
	}
	// length nonzero?
	vector_3 diff = toPos-fromPos;
	
	if ( drawticks )
	{
		// divide it up
		float length = diff.Length();
		int segments = max( (int)floorf( length*0.6f ), 1 );
		float offset = 1.0f / segments;

		// get time iter
		static const float ROTATION_PERIOD = 3.0f;
		static const float ARROW_LENGTH = 0.15f;
		static const float ARROW_WIDTH = ARROW_LENGTH * 0.5f;
		float arrowPos = float( Modulus( (float)gWorldTime.GetTime(), ROTATION_PERIOD ) / ROTATION_PERIOD );

		float iter = offset * arrowPos;
		
		// now draw ticks

		for ( int i = 0 ; i != segments ; ++i, iter += offset )
		{
			float t = __min(1,iter);
			vector_3 q = ((1-t)*(1-t) * p0) + ((2*(1-t)*t) * p1 ) + ((t*t) * p2);	
			float s = t - ARROW_LENGTH;
			vector_3 p = ((1-s)*(1-s) * p0) + ((2*(1-s)*s) * p1 ) + ((s*s) * p2);	

			vector_3 diff = q-p;

			vector_3 unit = diff.Normalize_T();
			matrix_3x3 dir = MatrixFromDirection( unit );
			unit *= ARROW_LENGTH;
	
			matrix_3x3 rot = AxisRotationColumns( dir.GetColumn2_T(), (arrowPos * (PI2 + (PI2 * 0.1f))) + (i * (PI2 * 0.1f)) );
			vector_3 tick = Normalize( rot * dir.GetColumn0_T() ) * ARROW_WIDTH;

			gSiegeEngine.GetWorldSpaceLines().DrawLine( q, q - unit + tick, color );
			gSiegeEngine.GetWorldSpaceLines().DrawLine( q, q - unit - tick, color );

		}
	}

	if (!text.empty() && gWorldOptions.TestDebugHudOptions( DHO_LABELS ))
	{
		// draw label
		SiegePos labelPos;
		labelPos.FromWorldPos( p1, from.node );

		DrawDebugScreenLabel( labelPos, text );
		gSiegeEngine.GetWorldSpaceLines().DrawLine( splineMidPoint, p1b, color);
	}
#endif // !GP_RETAIL
}

////////////////////////////////////////////////////////////////////////////////////
void World :: DrawDebugBoxStack(const SiegePos& p, const float s, DWORD c, float d, const matrix_3x3& m)
{
#if !GP_RETAIL
	// Make sure we account for any game speedup
	d *= gAppModule.GetTimeMultiplier();

	SiegePos pv = p ;

	for (int i = 0; i < 10;i++)
	{
		DrawDebugBox( pv, m, vector_3(s,s,s), c, FilterSmoothStep(0.0f,10.0f,(float)i) * d, false, i==0);
		pv.pos.y += 2*s;
	}
#endif
}


#if !GP_RETAIL

void World :: GetPositionString( SiegePos const& Position, gpstring& output )
{
	output.appendf( "pos=[%9.2fN,%9.2fE,%9.2fU] node=%s",
			 Position.pos.GetZ(),
			 Position.pos.GetX(),
			 Position.pos.GetY(),
			 Position.node.ToString().c_str() );
}

#endif // !GP_RETAIL


#if !GP_RETAIL

void World :: DrawDebugBox( const SiegePos &box_center, const matrix_3x3 &orient,
					const vector_3 &half_diag, const DWORD color, const float existence_duration,
					const bool world_space, bool draw_origin, const gpstring& text )
{
	// $ early out if anything wrong
	if ( !gSiegeEngine.IsNodeValid( box_center.node ) )
	{
		return;
	}

	// If game is paused but duration is not zero, well, it can basically fuck off.
	float duration	= existence_duration;
	if( gAppModule.IsUserPaused() && !IsZero( existence_duration ) )
	{
		duration	= 0.0f;
	}

	SiegePos	box_center_pos( box_center );
	matrix_3x3	box_orientation( orient );

	if( world_space )
	{
		siege::cache<siege::SiegeNode> &node_cache = gSiegeEngine.NodeCache();
		const siege::SiegeNodeHandle hNode( node_cache.UseObject( box_center_pos.node ) );
		const siege::SiegeNode& node = hNode.RequestObject( node_cache );

		box_center_pos.pos = node.WorldToNodeSpace( box_center.pos );
		box_orientation = node.GetTransposeOrientation() * box_orientation;
	}	
	DebugBox new_box( box_center_pos, box_orientation, half_diag,
		              draw_origin ? 0.333f * Length(half_diag) : 0.0f,
					  color, duration, text );

	m_DebugBoxes.push_back( new_box );
}

#endif // !GP_RETAIL


#if !GP_RETAIL

void World :: DrawDebugLines( float delta_t )
{
	if( m_DebugLines.empty() )
	{
		return;
	}

	stdx::fast_vector< DebugLine >::iterator iLine = m_DebugLines.begin();

	while( iLine != m_DebugLines.end() )
	{
		const float alpha	= (float)(1.0f - (*iLine).m_elapsed / (*iLine).m_duration);
		const DWORD t_color	= ((*iLine).m_color & 0x00FFFFFF) | (((DWORD)(0xFF * alpha))<<24);

		DrawDebugLine( (*iLine).m_start, (*iLine).m_end, t_color, (*iLine).m_text );

		(*iLine).m_elapsed += delta_t;

		if( (*iLine).m_elapsed >= (*iLine).m_duration || IsZero( (*iLine).m_duration ) )
		{
			iLine = m_DebugLines.erase( iLine );
		}
		else
		{
			++iLine;
		}
	}
}

#endif // !GP_RETAIL


#if !GP_RETAIL

void World :: DrawDebugBoxes( float delta_t )
{
	if( m_DebugBoxes.empty() ) {
		return;
	}

	stdx::fast_vector< DebugBox >::iterator iDebugBox = m_DebugBoxes.begin();

	Rapi &renderer = gSiegeEngine.Renderer();

/*	// Preserve the texture stage 0 COLOROP
	DWORD colorop;
	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_COLOROP, &colorop );
	if( colorop != D3DTOP_SELECTARG2 )
	{
		GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
	}
*/
	bool fogstate = renderer.SetFogActivatedState( false );

	renderer.PushWorldMatrix();


	while( iDebugBox != m_DebugBoxes.end() ) {

		if ( gSiegeEngine.IsNodeValid( (*iDebugBox).m_center.node ) )
		{
			matrix_3x3	space_orient;
			vector_3	space_center;

			siege::cache<siege::SiegeNode> &node_cache = gSiegeEngine.NodeCache();

			const siege::SiegeNodeHandle hNode( node_cache.UseObject( (*iDebugBox).m_center.node ) );
			const siege::SiegeNode& node = hNode.RequestObject( node_cache );

			space_center = node.GetCurrentCenter();
			space_orient = node.GetCurrentOrientation();

			renderer.SetWorldMatrix( space_orient, space_center );
			renderer.TranslateWorldMatrix( (*iDebugBox).m_center.pos );
			renderer.RotateWorldMatrix( (*iDebugBox).m_orient );

			float alpha = 1.0f;
			if( !IsZero( (*iDebugBox).m_duration ) )
			{
				alpha	= (float)(1.0f - (*iDebugBox).m_elapsed / (*iDebugBox).m_duration);
			}

			DWORD	t_color = (*iDebugBox).m_color & 0x00FFFFFF;
					t_color |= ((DWORD)(0xFF * alpha))<<24;

			if (!IsZero((*iDebugBox).m_origin_scale))
			{
				RP_DrawOrigin( renderer, (*iDebugBox).m_origin_scale );
			}
			RP_DrawBox( renderer, -(*iDebugBox).m_half_diag, (*iDebugBox).m_half_diag, t_color );

			(*iDebugBox).m_elapsed += delta_t;

			if( !(*iDebugBox).m_text.empty() && gWorldOptions.TestDebugHudOptions( DHO_LABELS ) )
			{
				SiegePos LabelPos = (*iDebugBox).m_center;
				LabelPos.pos.y -= (*iDebugBox).m_half_diag.y*0.3333f;
				gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), (*iDebugBox).m_text );
			}
		}

		if( (*iDebugBox).m_elapsed >= (*iDebugBox).m_duration || IsZero( (*iDebugBox).m_duration ) ) {
			iDebugBox = m_DebugBoxes.erase( iDebugBox );
		} else {
			++iDebugBox;
		}
	}

	renderer.PopWorldMatrix();

	renderer.SetFogActivatedState( fogstate );
}

#endif // !GP_RETAIL


//**********************************************************************************
// Report streams for debugging

#if !GP_RETAIL

static ReportSys::Context* gMCPContextPtr   = NULL;

ReportSys::Context& GetMCPContext( void )
{
	if ( gMCPContextPtr == NULL )
	{
		static ReportSys::Context s_MCPContext( &gGenericContext, "MCP" );
		gMCPContextPtr = &s_MCPContext;

		s_MCPContext.Enable( false );
	}
	return ( *gMCPContextPtr );
}

static ReportSys::Context* gMCPMessageContextPtr   = NULL;

ReportSys::Context& GetMCPMessageContext( void )
{
	if ( gMCPMessageContextPtr == NULL )
	{
		static ReportSys::Context s_MCPMessageContext( &gGenericContext, "MCPMessage" );
		gMCPMessageContextPtr = &s_MCPMessageContext;

		s_MCPMessageContext.Enable( false );
	}
	return ( *gMCPMessageContextPtr );
}

static ReportSys::Context* gMCPCrowdContextPtr   = NULL;

ReportSys::Context& GetMCPCrowdContext( void )
{
	if ( gMCPCrowdContextPtr == NULL )
	{
		static ReportSys::Context s_MCPCrowdContext( &gGenericContext, "MCPCrowd" );
		gMCPCrowdContextPtr = &s_MCPCrowdContext;

		s_MCPCrowdContext.Enable( false );
	}
	return ( *gMCPCrowdContextPtr );
}

static ReportSys::Context* gMCPInterceptContextPtr   = NULL;

ReportSys::Context& GetMCPInterceptContext( void )
{
	if ( gMCPInterceptContextPtr == NULL )
	{
		static ReportSys::Context s_MCPInterceptContext( &gGenericContext, "MCPIntercept" );
		gMCPInterceptContextPtr = &s_MCPInterceptContext;

		s_MCPInterceptContext.Enable( false );
	}
	return ( *gMCPInterceptContextPtr );
}

static ReportSys::Context* gMCPNetLogContextPtr   = NULL;

ReportSys::Context& GetMCPNetLogContext( void )
{
	if ( gMCPNetLogContextPtr == NULL )
	{
		static ReportSys::Context s_MCPNetLogContext( &gGenericContext, "MCPNetLog" );
		gMCPNetLogContextPtr = &s_MCPNetLogContext;

		s_MCPNetLogContext.Enable( false );
	}
	return ( *gMCPNetLogContextPtr );
}

static ReportSys::Context* gMCPSequencerContextPtr   = NULL;

ReportSys::Context& GetMCPSequencerContext( void )
{
	if ( gMCPSequencerContextPtr == NULL )
	{
		static ReportSys::Context s_MCPSequencerContext( &gGenericContext, "MCPSeq" );
		gMCPSequencerContextPtr = &s_MCPSequencerContext;

		s_MCPSequencerContext.Enable( false );
	}
	return ( *gMCPSequencerContextPtr );
}

static ReportSys::Context* gMCPSignalContextPtr   = NULL;

ReportSys::Context& GetMCPSignalContext( void )
{
	if ( gMCPSignalContextPtr == NULL )
	{
		static ReportSys::Context s_MCPSignalContext( &gGenericContext, "MCPSignal" );
		gMCPSignalContextPtr = &s_MCPSignalContext;

		s_MCPSignalContext.Enable( false );
	}
	return ( *gMCPSignalContextPtr );
}


static ReportSys::Context* gAISkritContextPtr   = NULL;

ReportSys::Context& GetAISkritContext( void )
{
	if ( gAISkritContextPtr == NULL )
	{
		static ReportSys::Context s_AISkritContext( &gGenericContext, "AISkrit" );
		gAISkritContextPtr = &s_AISkritContext;

		s_AISkritContext.Enable( false );
	}
	return ( *gAISkritContextPtr );
}

static ReportSys::Context* gAIMoveContextPtr   = NULL;

ReportSys::Context& GetAIMoveContext( void )
{
	if ( gAIMoveContextPtr == NULL )
	{
		static ReportSys::Context s_AIMoveContext( &gGenericContext, "AIMove" );
		gAIMoveContextPtr = &s_AIMoveContext;

		s_AIMoveContext.Enable( false );
	}
	return ( *gAIMoveContextPtr );
}

static ReportSys::Context* gGamePlayContextPtr   = NULL;

ReportSys::Context& GetGamePlayContext( void )
{
	if ( gGamePlayContextPtr == NULL )
	{
		static ReportSys::Context s_GamePlayContext( &gGenericContext, "GamePlay" );
		gGamePlayContextPtr = &s_GamePlayContext;
		s_GamePlayContext.Enable( false );

		gpstring computerName( SysInfo::GetComputerName() );
		computerName.to_lower();
		gpstring fileName;
		fileName.assignf( "gameplay-%s.log", computerName.c_str() );
		gGamePlayContextPtr->AddSink( new ReportSys::LogFileSink <> ( fileName, false, "GamePlay" ), true );
	}
	return ( *gGamePlayContextPtr );
}

ReportSys::Context * GamePlayContext( void )
{
	return( &GetGamePlayContext() );
}


ReportSys::Context * PerfLogContext( void )
{
	return( &gPerfLogContext );
}

static ReportSys::Context* gJEDIContextPtr   = NULL;

ReportSys::Context& GetJEDIContext( void )
{
	if ( gJEDIContextPtr == NULL )
	{
		static ReportSys::Context s_JEDIContext( &gGenericContext, "JEDI" );
		gJEDIContextPtr = &s_JEDIContext;

		s_JEDIContext.Enable( false );
	}
	return ( *gJEDIContextPtr );
}

static ReportSys::Context* gTriggerSysContextPtr   = NULL;

ReportSys::Context& GetTriggerSysContext( void )
{
	if ( gTriggerSysContextPtr == NULL )
	{
		static ReportSys::Context s_TriggerSysContext( &gGenericContext, "TriggerSys" );
		gTriggerSysContextPtr = &s_TriggerSysContext;

		s_TriggerSysContext.Enable( false );
	}
	return ( *gTriggerSysContextPtr );
}

#endif // !GP_RETAIL
