#pragma once
/*=======================================================================================

  WorldOptions

  purpose:	Track world-specific options.

  author:	Bartosz Kijanka

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
#ifndef __OPTIONS_H
#define __OPTIONS_H




class Options;


#include "fubidefs.h"
#include "GoDefs.h"

// $$$$$ NON-RETAIL ONLY FOR MOST OF THESE!!! -sb


enum eDebugHudGroup
{
	DHG_NONE,
	DHG_ACTORS,
	DHG_ITEMS,
	DHG_COMMANDS,
	DHG_MISC,
	DHG_ALL
};


enum eDebugHudOptions
{
	DHO_NONE			= 0,

	DHO_AI_SENSORS		= 1 << 0,
	DHO_AI_PATHFINDER	= 1 << 1,
	DHO_AI				= DHO_AI_SENSORS | DHO_AI_PATHFINDER,

	DHO_MESSAGES		= 1 << 2,

	DHO_LABELS			= 1 << 3,

	DHO_ERRORS			= 1 << 4,

	DHO_ALL				= DHO_AI | DHO_MESSAGES | DHO_LABELS,

	DHO_DEPTH			= 1 << 5
};


enum eDifficulty
{
	DIFFICULTY_EASY		= 0,
	DIFFICULTY_MEDIUM,
	DIFFICULTY_HARD,

	DIFFICULTY_SIZE
};


// Use this to turn off the gore.
#define DISABLE_GORE 0


class WorldOptions : public Singleton <WorldOptions>
{

public:

	WorldOptions();
	~WorldOptions();

	bool Xfer( FuBi::PersistContext& persist );
	void SSyncOnMachine( DWORD machineId );
FEX	FuBiCookie RCSyncOnMachineHelper( DWORD machineId, const_mem_ptr syncBlock );
	void XferForSync( FuBi::PersistContext& persist );

FEX void  RSSetGameSpeed( float x );
FEX void  RCSetGameSpeed( float x );
FEX void  SetGameSpeed( float x, bool printToScreen );
FEX void  SetGameSpeed( float x )										{ SetGameSpeed( x, false ); }
FEX float GetGameSpeed( void );

FEX	bool GetSound() const												{ return( m_bSound ); }
	void SetSound( const bool flag )									{ m_bSound = flag; }

	bool GetPlayerCollisions() const									{ return( m_bPlayerCollisions ); }
	void SetPlayerCollisions( const bool flag )							{ m_bPlayerCollisions = flag; }

	bool GetWireframe() const											{ return( m_bWireframe ); }
	void SetWireframe( const bool flag )								{ m_bWireframe = flag; }

	bool GetObstacleSteering() const									{ return( m_bObstacleSteering ); }
	void SetObstacleSteering( const bool flag )							{ m_bObstacleSteering = flag; }

	Goid GetDebugProbeObject() const									{ return( m_DebugProbeObject ); }
	void SetDebugProbeObject( Goid id )									{ m_DebugProbeObject = id; }

	Goid GetDebugHUDObject() const										{ return( m_DebugHUDObject ); }
	void SetDebugHUDObject( Goid id )									{ m_DebugHUDObject = id; }

	eDebugHudGroup GetDebugHudGroup() const								{ return( m_DebugHudGroup ); }
	void SetDebugHudGroup( const eDebugHudGroup Level )					{ m_DebugHudGroup = Level; }

FEX void SetDebugHudOptions   ( eDebugHudOptions options, bool set )	{  m_DebugHudOptions = (eDebugHudOptions)(set ? (m_DebugHudOptions | options) : (m_DebugHudOptions & ~options));  }
FEX void ClearDebugHudOptions ( eDebugHudOptions options )				{  SetDebugHudOptions( options, false );  }
FEX void ToggleDebugHudOptions( eDebugHudOptions options )				{  m_DebugHudOptions = (eDebugHudOptions)(m_DebugHudOptions ^ options);  }
FEX bool TestDebugHudOptions  ( eDebugHudOptions options ) const		{  return ( (m_DebugHudOptions & options) != 0 );  }
FEX bool TestDebugHudOptionsEq( eDebugHudOptions options ) const		{  return ( (m_DebugHudOptions & options) == options );  }
	
	eDebugHudOptions GetDebugHudOptions() const							{ return( m_DebugHudOptions ); }

	bool GetShowAll() const												{ return( m_bShowAll ); }
	void SetShowAll( const bool Flag )									{ m_bShowAll = Flag; }

	bool GetShowObjects() const											{ return( m_bShowObjects ); }
	void SetShowObjects( const bool Flag )								{ m_bShowObjects = Flag; }

	bool GetShowMpUsage() const											{ return( m_bShowMpUsage ); }
	void SetShowMpUsage( const bool Flag )								{ m_bShowMpUsage = Flag; }

FEX	float GetObjectDetailLevel() const									{ return( m_ObjectDetailLevel ); }
	void  SetObjectDetailLevel( const float level );

	bool GetShowGizmos() const											{ return( m_bShowGizmos ); }
	void SetShowGizmos( const bool Flag )								{ m_bShowGizmos = Flag; }

FEX bool GetShowPathsMCP() const										{ return( m_bShowPathsMCP  ); }
FEX void SetShowPathsMCP( const bool Flag )								{ m_bShowPathsMCP  = Flag; }

FEX bool GetShowMCP() const												{ return( m_bShowMCP ); }
FEX void SetShowMCP( const bool Flag )									{ m_bShowMCP = Flag; }

FEX float GetLagMCP() const												{ return( m_fLagMCP ); }
FEX void  SetLagMCP( const float val )									{ m_fLagMCP = val; }

FEX bool GetLabelMCP() const											{ return( m_bLabelMCP ); }
FEX void SetLabelMCP( const bool Flag )									{ m_bLabelMCP = Flag; }

FEX bool GetOptimizeMCP() const											{ return( m_bOptimizeMCP ); }
FEX void SetOptimizeMCP( const bool Flag )								{ m_bOptimizeMCP = Flag; }

FEX bool GetHudMCP() const												{ return( m_bHudMCP ); }
FEX void SetHudMCP( const bool Flag )									{ m_bHudMCP = Flag; }

FEX bool GetGoalMCP() const												{ return( m_bGoalMCP ); }
FEX void SetGoalMCP( const bool Flag )									{ m_bGoalMCP = Flag; }

FEX bool GetBlockMCP() const											{ return( m_bBlockMCP ); }
FEX void SetBlockMCP( const bool Flag )									{ m_bBlockMCP = Flag; }

FEX bool GetDebugMCP() const											{ return( m_bDebugMCP ); }
FEX void SetDebugMCP( const bool Flag )									{ m_bDebugMCP = Flag; }

FEX bool GetTestMCP() const												{ return( m_bTestMCP ); }
FEX void SetTestMCP( const bool Flag )									{ m_bTestMCP = Flag; }

FEX bool GetCrowdingMCP() const											{ return( m_bCrowdingMCP ); }
FEX void SetCrowdingMCP( const bool Flag )								{ m_bCrowdingMCP = Flag; }

FEX bool GetShowTriggerSys() const										{ return( m_bShowTriggerSys ); }
FEX void SetShowTriggerSys( const bool Flag )							{ m_bShowTriggerSys = Flag; }

FEX bool GetShowTriggerSysHits() const									{ return( m_bShowTriggerSysHits ); }
FEX void SetShowTriggerSysHits( const bool Flag )						{ m_bShowTriggerSysHits = Flag; }

	bool GetEliminateWaypoints() const									{ return( m_bEliminateWaypoints ); }
	void SetEliminateWaypoints( const bool Flag )						{ m_bEliminateWaypoints = Flag; }

	bool GetIndicateItems() const										{ return m_bIndicateItems; }
	void SetIndicateItems( const bool Flag )							{ m_bIndicateItems = Flag; }

	bool GetFixedWorldFrustum() const									{ return m_bFixedWorldFrustum; }
	void SetFixedWorldFrustum( const bool Flag )						{ m_bFixedWorldFrustum = Flag; }

	bool GetDebugToolTips() const										{ return m_bDebugToolTips; }
	void SetDebugToolTips( const bool Flag )							{ m_bDebugToolTips = Flag; }

	bool GetSkipContent() const											{ return m_bSkipContent; }
	void SetSkipContent( const bool Flag )								{ m_bSkipContent = Flag; }

	bool GetForceMpContent() const										{ return m_bForceMpContent; }
	void SetForceMpContent( const bool Flag )							{ m_bForceMpContent = Flag; }

	// $ FEX's removed from these to comply with ESRB + MS decision -sb
	bool GetAlwaysGib() const											{ return m_bAlwaysGib; }
	void SetAlwaysGib( const bool Flag )								{ m_bAlwaysGib = Flag; }

	// $ FEX's removed from these to comply with ESRB + MS decision -sb
	int  GetGibMultiplier() const										{ return m_GibMultiplier; }
	void SetGibMultiplier( const int Count )							{ m_GibMultiplier = Count; }

	bool GetAiQueryBoxes() const										{ return m_bAiQueryBoxes; }
	void SetAiQueryBoxes( const bool Flag )								{ m_bAiQueryBoxes = Flag; }

	bool GetSaveAsText() const											{ return m_bSaveAsText; }
	void SetSaveAsText( const bool Flag )								{ m_bSaveAsText = Flag; }

	bool GetSaveAsTank() const											{ return m_bSaveAsTank; }
	void SetSaveAsTank( const bool Flag )								{ m_bSaveAsTank = Flag; }

	bool GetAllowAutoSave() const										{ return m_bAllowAutoSave; }
	void SetAllowAutoSave( const bool Flag )							{ m_bAllowAutoSave = Flag; }

	bool GetAllowAutoSaveParty() const									{ return m_bAllowAutoSaveParty; }
	void SetAllowAutoSaveParty( const bool Flag )						{ m_bAllowAutoSaveParty = Flag; }

	bool GetForceRetailContent() const									{ return m_bForceRetailContent; }
	void SetForceRetailContent( const bool Flag )						{ m_bForceRetailContent = Flag; }

	void		SSetDifficulty( const eDifficulty difficulty );
FEX FuBiCookie	RCSetDifficulty( const eDifficulty difficulty );	FUBI_MEMBER_SET( RCSetDifficulty, +SERVER );
	void		SetDifficulty( const eDifficulty difficulty );
	void		SetDifficultyFloat( const float difficulty )			{ m_Difficulty = difficulty; }
FEX	eDifficulty GetDifficulty() const;								
FEX float		GetDifficultyFloat() const								{ return m_Difficulty; }

#if !GP_RETAIL
	bool GetAICaching()													{ return m_AICaching; }
FEX void SetAICaching( bool value )										{ m_AICaching = value; }
#endif

	bool GetAllowDismemberment( void ) const							{ return m_bGoreGood; }
	void SetAllowDismemberment( bool state )							{ m_bGoreGood = state; }

	bool IsBloodEnabled( void ) const									{ return m_bIsBloodEnabled; }
	void SetIsBloodEnabled( bool bBloodEnabled );

	bool IsBloodRed( void ) const										{ return m_bIsBloodRed; }
	void SetIsBloodRed( bool bBloodRed );


	bool GetTuningLoad( void ) const									{ return m_bTuningLoad; }
	void SetTuningLoad( bool bSet )										{ m_bTuningLoad = bSet; }

	bool GetDisableHighlight() const									{ return m_bDisableHighlight; }
	void SetDisableHighlight( bool bSet )								{ m_bDisableHighlight = bSet; }

	bool GetCommandCastEnabled()										{ return m_bCommandCastEnabled; }
	void SetCommandCastEnabled( bool bSet )								{ m_bCommandCastEnabled = bSet; }

	bool GetShowObjectCost()											{ return m_bObjectCostEnabled; }
	void SetShowObjectCost( bool bSet )									{ m_bObjectCostEnabled = bSet; }

	bool GetSelectionRings() const										{ return m_bSelectionRings; }
	void SetSelectionRings( bool bSet )									{ m_bSelectionRings = bSet; }

	// is exp gharing among friends in SP and MP enabled?
FEX	bool GetShareExpEnabled() const										{ return m_bShareExpEnabled; }
	void SetShareExpEnabled( bool bSet )								{ m_bShareExpEnabled = bSet; }

	// is gold gharing among friends in MP enabled?
FEX	bool GetShareGoldEnabled() const									{ return m_bShareGoldEnabled; }
	void SetShareGoldEnabled( bool bSet )								{ m_bShareGoldEnabled = bSet; }

	////////////////////////////////////////
	//	AI related

FEX	bool GetShowSpatialQueries()										{ return( m_ShowSpatialQueries ); }
FEX void SetShowSpatialQueries( bool flag )								{ m_ShowSpatialQueries = flag; }

	DWORD GetFindSpotRetryLimit()										{ return m_FindSpotRetryLimit; }
	float GetVisibilityCacheDirtyDistance()								{ return m_VisibilityCacheDirtyDistance; }
	float GetPartySpeed()												{ return m_PartySpeed; }
	float GetPartyTether()												{ return m_PartyTether; }
	float GetFormationSpotVerticalConstraint()							{ return m_FormationSpotVerticalConstraint; }
	float GetDropItemsDistanceMin()										{ return m_DropItemsDistanceMin; }
	float GetDropItemsDistanceMax()										{ return m_DropItemsDistanceMax; }
	float GetInventorySpewVerticalConstraint()							{ return m_InventorySpewVerticalConstraint; }
	float GetSpotWalkMaskRadius()										{ return m_SpotWalkMaskRadius; }
	float GetDefaultSensorScanPeriod()									{ return m_DefaultSensorScanPeriod; }
	float GetDefaultFollowerSpacing()									{ return m_DefaultFollowerSpacing; }
	float GetDefaultPackFollowerSpacing()								{ return m_DefaultPackFollowerSpacing; }
FEX int GetMeleeAttackerCountLimit()									{ return m_MeleeAttackerCountLimit; }


private:

	bool				m_bSound;
	bool				m_bPlayerCollisions;
	bool				m_bWireframe;
	bool				m_bObstacleSteering;
	bool				m_bShowAll;
	bool				m_bShowObjects;
	bool				m_bShowMpUsage;
	float				m_ObjectDetailLevel;
	bool				m_bIndicateItems;
	bool				m_bShowGizmos;
	bool				m_bShowPathsMCP;
	bool				m_bShowMCP;
	float				m_fLagMCP;
	bool				m_bLabelMCP;
	bool				m_bOptimizeMCP;
	bool				m_bHudMCP;
	bool				m_bGoalMCP;
	bool				m_bBlockMCP;
	bool				m_bDebugMCP;
	bool				m_bTestMCP;
	bool				m_bCrowdingMCP;
	bool				m_bShowTriggerSys;
	bool				m_bShowTriggerSysHits;
	bool				m_bEliminateWaypoints;
	bool				m_bFixedWorldFrustum;
	bool				m_bDebugToolTips;
	bool				m_bSkipContent;
	bool				m_bForceMpContent;
	bool				m_bAlwaysGib;
	int					m_GibMultiplier;
	bool				m_bAiQueryBoxes;
	bool				m_bSaveAsText;
	bool				m_bSaveAsTank;
	bool				m_bAllowAutoSave;
	bool				m_bAllowAutoSaveParty;
	bool				m_bForceRetailContent;
	Goid				m_DebugProbeObject;
	Goid				m_DebugHUDObject;
	eDebugHudGroup		m_DebugHudGroup;
	eDebugHudOptions	m_DebugHudOptions;
	float				m_Difficulty;
	bool				m_bGoreGood;
	bool				m_bTuningLoad;
	bool				m_bDisableHighlight;
	bool				m_bIsBloodEnabled;
	bool				m_bIsBloodRed;
	bool				m_bCommandCastEnabled;
	bool				m_bObjectCostEnabled;
	bool				m_bSelectionRings;
	bool				m_bShareExpEnabled;
	bool				m_bShareGoldEnabled;

	////////////////////////////////////////
	//	AI 

	bool				m_ShowSpatialQueries;
	DWORD				m_FindSpotRetryLimit;
	float				m_VisibilityCacheDirtyDistance;
	float				m_PartySpeed;
	float				m_PartyTether;
	float				m_FormationSpotVerticalConstraint;
	float				m_DropItemsDistanceMin;
	float				m_DropItemsDistanceMax;
	float				m_InventorySpewVerticalConstraint;
	float				m_SpotWalkMaskRadius;
	float				m_DefaultSensorScanPeriod;
	float				m_DefaultFollowerSpacing;
	float				m_DefaultPackFollowerSpacing;
	int					m_MeleeAttackerCountLimit;

#if !GP_RETAIL
	bool				m_AICaching;
#endif

	FUBI_SINGLETON_CLASS( WorldOptions, "This object is responsible for game initiation." );
};


#define gWorldOptions WorldOptions::GetSingleton()


#endif
