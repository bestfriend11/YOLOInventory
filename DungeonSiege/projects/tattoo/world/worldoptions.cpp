#include "Precomp_World.h"

#include "WorldOptions.h"

#include "AppModule.h"
#include "Config.h"
#include "FuBi.h"
#include "FuBiPersistImpl.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "GoSupport.h"
#include "Server.h"
#include "Flamethrower_interpreter.h"


using namespace FuBi;




WorldOptions::WorldOptions()
{
	m_bSound                         = true;
	m_bPlayerCollisions              = true;
	m_bWireframe                     = false;
	m_bObstacleSteering              = true;
	m_bShowAll                       = false;
	m_bShowObjects                   = true;
	m_bShowMpUsage                   = false;
	m_bObjectCostEnabled             = false;
	m_ObjectDetailLevel              = 1.0f;
	m_bIndicateItems                 = false;
	m_bShowGizmos                    = false;
	m_bShowPathsMCP                  = false;
	m_bShowMCP                       = false;
	m_fLagMCP                        = 0.0f;
	m_bLabelMCP                      = false;
	m_bOptimizeMCP                   = true;
	m_bHudMCP                        = false;
	m_bGoalMCP                       = false;
	m_bBlockMCP                      = false;
	m_bDebugMCP                      = (GP_DEBUG==1);
	m_bTestMCP                       = false;
	m_bCrowdingMCP                   = false;
	m_bShowTriggerSys				 = false;
	m_bShowTriggerSysHits			 = false;
	m_bEliminateWaypoints			 = false;
	m_bFixedWorldFrustum             = false;
	m_bDebugToolTips                 = false;
	m_bSkipContent                   = false;
	m_bForceMpContent                = false;
	m_bAlwaysGib                     = false;
	m_GibMultiplier                  = 1;
	m_bAiQueryBoxes                  = false;
	m_bAllowAutoSave                 = true;
	m_bAllowAutoSaveParty			 = true;
	m_bForceRetailContent			 = GP_RETAIL || gConfig.GetBool( "force_retail_content", false );
	m_DebugProbeObject               = GOID_INVALID;
	m_DebugHUDObject                 = GOID_INVALID;
	m_DebugHUDObject                 = GOID_INVALID;
	m_DebugHudGroup                  = DHG_NONE;
	m_Difficulty                     = 0.5f;
	m_bGoreGood						 = false;
#if !DISABLE_GORE
	m_bGoreGood						 = true;
#endif
	m_bTuningLoad					 = false;
	m_bDisableHighlight				 = false;
	m_bIsBloodEnabled				 = true;
	m_bShareExpEnabled				 = true;
	m_bShareGoldEnabled				 = true;
	m_bIsBloodRed					 = true;	
	m_bCommandCastEnabled			 = true;
	m_bSelectionRings				 = true;

	////////////////////////////////////////
	//	AI

	m_ShowSpatialQueries              = false;
	m_FindSpotRetryLimit              = 15;
	m_VisibilityCacheDirtyDistance    = 0.25;
	m_PartySpeed                      = 4.0f;
	m_PartyTether                     = 4.0f;
	m_FormationSpotVerticalConstraint = 2.0f;
	m_DropItemsDistanceMin            = 0;
	m_DropItemsDistanceMax            = 0.3f;
	m_InventorySpewVerticalConstraint = 1.5f;
	m_SpotWalkMaskRadius			  = 0.25f;
	m_DefaultSensorScanPeriod		  = 0.25f;
	m_DefaultFollowerSpacing		  = 1.5;
	m_DefaultPackFollowerSpacing	  = 2.5;
	m_MeleeAttackerCountLimit		  = 8;

#if !GP_RETAIL
	m_AICaching						  = true;
#endif

	SetDebugHudOptions( DHO_ALL, true );
	//ToggleDebugHudOptions( DHO_LABELS );

#	if !GP_RETAIL
	gConfig.Read( "noscids", m_bSkipContent, false );
	gConfig.Read( "mpscids", m_bForceMpContent, false );
#	endif // !GP_RETAIL

	gConfig.Read( "save_as_text", m_bSaveAsText, !GP_RETAIL );
	gConfig.Read( "save_as_tank", m_bSaveAsTank, !GP_DEBUG );

	// Find out if dismemberment is ok
	// $ dismemberment disable for esrb
#if !DISABLE_GORE
	FastFuelHandle	hLocaleBlock( "config:global_settings:locale" );
	if( hLocaleBlock.IsValid() )
	{
		hLocaleBlock.Get( "allow_dismemberment", m_bGoreGood );
	}
	else
	{
		m_bGoreGood = false;
	}
#endif

	FastFuelHandle	hConfig( "config:global_settings:special" );
	if( hConfig.IsValid() )
	{
		hConfig.Get( "allow_dismemberment", m_bGoreGood );
	}
	else
	{
		m_bGoreGood = false;
	}

}


WorldOptions::~WorldOptions()
{
}


bool WorldOptions::Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_bSound",                          m_bSound                          );
	persist.Xfer( "m_bPlayerCollisions",               m_bPlayerCollisions               );
	persist.Xfer( "m_bWireframe",                      m_bWireframe                      );
	persist.Xfer( "m_bObstacleSteering",               m_bObstacleSteering               );
	persist.Xfer( "m_bShowAll",                        m_bShowAll                        );
	persist.Xfer( "m_bShowObjects",                    m_bShowObjects                    );
	persist.Xfer( "m_bShowMpUsage",                    m_bShowMpUsage                    );
	persist.Xfer( "m_bObjectCostEnabled",              m_bObjectCostEnabled              );	
	persist.Xfer( "m_bIndicateItems",                  m_bIndicateItems                  );
	persist.Xfer( "m_bShowGizmos",                     m_bShowGizmos                     );
	persist.Xfer( "m_bShowPathsMCP",                   m_bShowPathsMCP                   );
	persist.Xfer( "m_bShowMCP",                        m_bShowMCP                        );
	persist.Xfer( "m_fLagMCP",                         m_fLagMCP                         );
	persist.Xfer( "m_bLabelMCP",                       m_bLabelMCP                       );
	persist.Xfer( "m_bOptimizeMCP",                    m_bOptimizeMCP                    );
	persist.Xfer( "m_bHudMCP",                         m_bHudMCP                         );
	persist.Xfer( "m_bGoalMCP",                        m_bGoalMCP                        );
	persist.Xfer( "m_bBlockMCP",                       m_bBlockMCP                       );
	persist.Xfer( "m_bDebugMCP",                       m_bDebugMCP                       );
	persist.Xfer( "m_bTestMCP",                        m_bTestMCP						 );
	persist.Xfer( "m_bCrowdingMCP",                    m_bCrowdingMCP					 );
	persist.Xfer( "m_bShowTriggerSys",                 m_bShowTriggerSys				 );
	persist.Xfer( "m_bShowTriggerSysHits",             m_bShowTriggerSysHits			 );
	persist.Xfer( "m_bEliminateWaypoints",			   m_bEliminateWaypoints			 );
	persist.Xfer( "m_bFixedWorldFrustum",              m_bFixedWorldFrustum              );
	persist.Xfer( "m_bDebugToolTips",                  m_bDebugToolTips                  );
	persist.Xfer( "m_bSkipContent",                    m_bSkipContent                    );
	persist.Xfer( "m_bForceMpContent",                 m_bForceMpContent                 );
	persist.Xfer( "m_bAlwaysGib",                      m_bAlwaysGib                      );
	persist.Xfer( "m_GibMultiplier",                   m_GibMultiplier                   );
	persist.Xfer( "m_bAiQueryBoxes",                   m_bAiQueryBoxes                   );
	persist.Xfer( "m_bForceRetailContent",			   m_bForceRetailContent			 );
	persist.Xfer( "m_DebugProbeObject",                m_DebugProbeObject                );
	persist.Xfer( "m_Difficulty",                      m_Difficulty                      );
	persist.Xfer( "m_bGoreGood",                       m_bGoreGood                       );
	persist.Xfer( "m_bShareExpEnabled",                m_bShareExpEnabled                );
	persist.Xfer( "m_bShareGoldEnabled",               m_bShareGoldEnabled               );
	persist.Xfer( "m_bTuningLoad",                     m_bTuningLoad                     );	
	persist.Xfer( "m_bCommandCastEnabled",			   m_bCommandCastEnabled			 );
	persist.Xfer( "m_bSelectionRings",				   m_bSelectionRings				 );

	persist.Xfer( "m_FindSpotRetryLimit",              m_FindSpotRetryLimit              );
	persist.Xfer( "m_VisibilityCacheDirtyDistance",    m_VisibilityCacheDirtyDistance    );
	persist.Xfer( "m_PartySpeed",                      m_PartySpeed                      );
	persist.Xfer( "m_PartyTether",                     m_PartyTether                     );
	persist.Xfer( "m_FormationSpotVerticalConstraint", m_FormationSpotVerticalConstraint );
	persist.Xfer( "m_DropItemsDistanceMin",            m_DropItemsDistanceMin            );
	persist.Xfer( "m_DropItemsDistanceMax",            m_DropItemsDistanceMax            );
	persist.Xfer( "m_InventorySpewVerticalConstraint", m_InventorySpewVerticalConstraint );
	persist.Xfer( "m_SpotWalkMaskRadius",			   m_SpotWalkMaskRadius );
	persist.Xfer( "m_DefaultSensorScanPeriod",		   m_DefaultSensorScanPeriod		 );
	persist.Xfer( "m_DefaultFollowerSpacing",		   m_DefaultFollowerSpacing			 );
	persist.Xfer( "m_DefaultPackFollowerSpacing",	   m_DefaultPackFollowerSpacing		 );
	persist.Xfer( "m_MeleeAttackerCountLimit",		   m_MeleeAttackerCountLimit		 );

	return ( true );
}


void WorldOptions::SSyncOnMachine( DWORD machineId )
{
	FuBi::SerialBinaryWriter writer;
	FuBi::PersistContext persist( &writer );
	XferForSync( persist );	
	RCSyncOnMachineHelper( machineId, writer.GetBuffer() );
}

FuBiCookie WorldOptions :: RCSyncOnMachineHelper( DWORD machineId, const_mem_ptr syncBlock )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSyncOnMachineHelper, machineId );

	FuBi::SerialBinaryReader reader( syncBlock );
	FuBi::PersistContext persist( &reader );
	XferForSync( persist );
	
	return ( RPC_SUCCESS );
}


void WorldOptions :: XferForSync( FuBi::PersistContext& persist )
{
	// update this as more stuff needs to be synced

	persist.Xfer( "", m_Difficulty );		

	if( persist.IsRestoring() )
	{
		float speed = 1.0f;
		persist.Xfer( "", speed );
		SetGameSpeed( speed );
	}
	else
	{		
		float speed = GetGameSpeed();
		persist.Xfer( "", speed );	
	}
}


void WorldOptions::RSSetGameSpeed( float x )
{
	FUBI_RPC_CALL( RSSetGameSpeed, RPC_TO_SERVER );

	RCSetGameSpeed( x );
}


void WorldOptions::RCSetGameSpeed( float x )
{
	FUBI_RPC_CALL( RCSetGameSpeed, RPC_TO_ALL );

	SetGameSpeed( x, true );
}


void WorldOptions::SetGameSpeed( float x, bool printToScreen )
{
	// multiplayer only allows a 1.0 speed
#	if GP_RETAIL
	if ( ::IsMultiPlayer() )
	{
		x = 1.0f;
	}
#	endif // GP_RETAIL

	if ( (x >= 0) && !::IsEqual( x, gAppModule.GetTimeMultiplier() ) )
	{
		gAppModule.SetTimeMultiplier( x );
		if ( printToScreen )
		{
#			if GP_RETAIL
			gpscreenf(( $MSG$ "Set game speed to %1.1f\n", x ));
#			else // GP_RETAIL
			gpscreenf(( "Set game speed to %2.3f\n", x ));
#			endif // GP_RETAIL
		}
	}
}

float WorldOptions::GetGameSpeed( void )
{
	return ( gAppModule.GetTimeMultiplier() );
}


void WorldOptions::SetObjectDetailLevel( const float level )
{
	gpassert( ( level >= 0 ) && (level <= 1) );

	// Changed?
	if ( !::IsEqual( level, m_ObjectDetailLevel ) )
	{
		// Save the object detail level
		m_ObjectDetailLevel = level;

		// Notify Siege of the new level
		gSiegeEngine.SetLevelOfDetail( m_ObjectDetailLevel );

		// Notify GoDb of the new level if it's there
		if ( GoDb::DoesSingletonExist() )
		{
			gGoDb.UpdateLevelOfDetail();
		}
	}
}

void WorldOptions::SSetDifficulty( const eDifficulty difficulty )
{
	if ( ::IsServerLocal() )
	{
		RCSetDifficulty( difficulty );
	}
}


FuBiCookie WorldOptions::RCSetDifficulty( const eDifficulty difficulty )
{
	FUBI_RPC_CALL_RETRY( RCSetDifficulty, RPC_TO_ALL );

	SetDifficulty( difficulty );

	gServer.SetSettingsDirty();

	return RPC_SUCCESS;
}


void WorldOptions::SetDifficulty( const eDifficulty difficulty )
{
	if ( difficulty == DIFFICULTY_EASY )
	{
		m_Difficulty = 0.16665f;  // 0.3333 / 2 ( Center of range )
	}
	else if ( difficulty == DIFFICULTY_MEDIUM )
	{
		m_Difficulty = 0.5f; // ( Center of range )
	}
	else if ( difficulty == DIFFICULTY_HARD )
	{
		m_Difficulty = 0.83325f; // 0.6666 / 2 ( Center of range )
	}
}


eDifficulty WorldOptions::GetDifficulty() const
{
	if ( m_Difficulty <= 0.3333f )
	{
		return DIFFICULTY_EASY;
	}
	else if ( m_Difficulty <= 0.6666f )
	{
		return DIFFICULTY_MEDIUM;
	}
	else
	{
		return DIFFICULTY_HARD;
	}
}

void WorldOptions :: SetIsBloodRed( bool bBloodRed )
{
	m_bIsBloodRed = bBloodRed;
	gFlamethrower_interpreter.SetIsBloodRed( bBloodRed );
}

void WorldOptions :: SetIsBloodEnabled( bool bBloodEnabled )
{
	m_bIsBloodEnabled = bBloodEnabled;
	gFlamethrower_interpreter.SetIsBloodEnabled( bBloodEnabled );
}