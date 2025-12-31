//////////////////////////////////////////////////////////////////////////////
//
// File     :  World.h
// Author(s):  Bartosz Kijanka
//
// Summary  :  This is the main entry point for the Tattoo World. Everything
//             that comprises the "game world" can be found through here.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __WORLD_H
#define __WORLD_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"
#include "Matrix_3x3.h"
#include "gps_types.h"
#include "GoDefs.h"
#include "Siege_Pos.h"

//////////////////////////////////////////////////////////////////////////////
// forward declarations

class AIAction;
class AIQuery;
class CameraAgent;
class Conditions;
class DebugProbe;
class DrawTimeMgr;
class GameAuditor;
class GoRpcMgr;
class Job;
class MessageDispatch;
class Mood;
class NetLog;
class Player;
class Rules;
class Services;
class Server;
class Sim;
class SoundManager;
class TimeMgr;
class TimeOfDay;
class Victory;
class Weather;
class WorldFx;
class WorldOptions;
class WorldPathFinder;
class WorldSound;
class WorldTerrain;
class WorldTime;

struct vector_3;

namespace trigger
{
	class TriggerSys;
}

namespace MCP
{
	class Manager;
}

//////////////////////////////////////////////////////////////////////////////
// class World declaration

class World : public Singleton <World>
{
	struct DebugBox;

public:
	SET_NO_INHERITED( World );

// Types.

	typedef CBFunctor1< Goid >			InventoryUpdateCb;	
	typedef CBFunctor1< Goid >			TestEquipCb;
	typedef CBFunctor2< Goid, Goid >	DialogueCallbackCb;
	typedef CBFunctor2< Goid, Goid >	PurchaseGuiCb;
	typedef CBFunctor2< Goid, Goid >	StashGuiCb;
	typedef CBFunctor1< bool & >		RolloverHighlightCb;
	typedef CBFunctor0					PartyCb;
	typedef CBFunctor1< Goid >			CastCb;

// Creation.

	World( bool editing = false );

   ~World();

	// basic initialization
	bool Init();
	bool InitServer( bool multiPlayerMode );

	// shut down parts or full
	bool Shutdown( eShutdown shutdownType, bool restart );

	// persistence
	bool Xfer( FuBi::PersistContext& persist );

	// which mode are we in?
FEX bool IsSinglePlayer() const;
FEX bool IsMultiPlayer() const;

// Events.

	// called by app when we lose focus - reset all UI-dependent states!!
	void OnAppActivate( bool activate );

// Viewport.

	// parameters for world viewport
FEX void SetWorldViewport    ( float width, float height, float fovDegrees, float duration );
FEX void RestoreWorldViewport( float duration );

	// set viewport to world viewport or default
	void SetViewportWorld          ( void );
	void SetViewportDefault        ( void );
	void GetWorldViewport          ( float& width, float& height, float& fovDegrees );
	bool ViewportDiffersFromDefault( void );

// Other.

	// callback for updating UI inventory
	void       RegisterInventoryUpdateCallback	( InventoryUpdateCb callback );	
	void       UpdateInventoryGUICallback		( Goid clientId );

	// callback for testing equip requirements to see if something should be dropped
	void       RegisterTestEquipCallback		( TestEquipCb callback );	
	void       TestEquipCallback				( Goid clientId );

	// callback for updating UI dialogue
	void       RegisterDialogueCallback( DialogueCallbackCb callback );
	void       RSDialogueCallback      ( Goid talkerID, Goid member );
FEX FuBiCookie RCDialogueCallback      ( DWORD MachineID, Goid talkerID, Goid member );
	void       DialogueCallback        ( Goid talkerID, Goid member );

	// callback for store purchasing 	
	void	   RegisterPurchaseGuiCallback	( PurchaseGuiCb callback );
	void	   PurchaseGuiCallback			( Goid client, Goid item );	
	
	// callback for stash item extraction
	void	   RegisterStashGuiCallback		( StashGuiCb callback );
	void	   StashGuiCallback				( Goid client, Goid item );

	// callback for corpse highlighting
	void	   RegisterRolloverHighlightCallback( RolloverHighlightCb callback );
	void	   RolloverHighlightCallback( bool & bHighlight );

	// callback for party refreshing
	void	   RegisterPartyCallback( PartyCb callback );
	void	   PartyCallback( void );

	// callback for gui updating for things like reload delay.
	void	   RegisterCastCallback( CastCb callback );
	void	   CastCallback( Goid spell );

	void Update( float secondsElapsed, float actualDeltaTime );

	void Draw( float secondsElapsed );

	DWORD PlaySample( const gpstring& sSound,
					float Pitch = 1.0f,
					bool fLoop = false );

// Debug methods.

	// drawing debug primitives
FEX void DrawDebugLinkedTerrainPoints	( const SiegePos& PosA, const SiegePos& PosB, DWORD const Color, const gpstring& sName );
FEX void DrawDebugLine					( const SiegePos& PosA, const SiegePos& PosB, DWORD const Color, const gpstring& sName );
FEX	void DrawDebugLine					( const SiegePos& PosA, const SiegePos& PosB, DWORD const Color, const gpstring& sName, float duration );
FEX void DrawDebugDashedLine			( const SiegePos& PosA, const SiegePos& PosB, DWORD const Color, const gpstring& sName );
FEX void DrawDebugDirectedLine			( const SiegePos& PosA, const SiegePos& PosB, DWORD const Color, const gpstring& sName );
FEX void DrawDebugTriangle				( const SiegePos& Center, float Radius, DWORD const Color, const gpstring& sName );
FEX void DrawDebugPulsePolygon			( SiegePos const& Center, float MinRadius, float MaxRadius, float PulsePeriod, unsigned int Sides, DWORD const Color, gpstring const& sName );
FEX void DrawDebugCircle				( SiegePos const& Center, float Radius, DWORD const Color, gpstring const& sName );
FEX void DrawDebugWedge                 ( const SiegePos& Center, float Radius, vector_3 const& Direction, float ArcLength, DWORD Color, bool filled);
FEX void DrawDebugWedge                 ( const SiegePos& Center, float Radius, SiegePos const& Direction, float ArcLength, DWORD Color, bool filled);
FEX void DrawDebugSphere				( const SiegePos& Center, float Radius, DWORD const Color, const gpstring& sName );
FEX void DrawDebugSphereOffsetAngle		( const SiegePos& Center, float Radius, float OffsetAngle, DWORD const Color, const gpstring& sName );
FEX void DrawDebugSphereConeSection		( const SiegePos& Position, const vector_3& Direction, float Angle, float Radius, DWORD const Color, const gpstring& sName );
FEX void DrawDebugPoint					( const SiegePos& Center, float Radius, DWORD const Color, const gpstring& sName );
FEX void DrawDebugPoint					( SiegePos const& Center, float Radius, DWORD const Color, float height, gpstring const& sName, DWORD labelColor );
FEX void DrawDebugScreenLabel			( SiegePos const& Pos, gpstring const & name) { DrawDebugScreenLabelColor( Pos, name, 0xFFFFFFFF); }
FEX void DrawDebugScreenLabelColor		( SiegePos const& Pos, gpstring const & name, DWORD color);
FEX void DrawDebugWorldLabelScroll		( SiegePos const& Pos, gpstring const & name, DWORD color, float duration, bool scroll);
FEX void DrawDebugArc                   ( const SiegePos& PosA, const SiegePos& PosB, DWORD const color, const gpstring& sName=gpstring::EMPTY, bool drawticks=false );
FEX	void DrawDebugBoxStack				( const SiegePos& pos, const float size, DWORD color, float duration, const matrix_3x3& orient = matrix_3x3::IDENTITY );

#	if !GP_RETAIL
/*
	void DebugDrawPrimitive( eDDP primitive, Go * from				, Go * to			 , DWORD primColor, const gpstring & name, DWORD nameColor );
	void DebugDrawPrimitive( eDDP primitive, SiegePos const & from	, Go * to			 , DWORD primColor, const gpstring & name, DWORD nameColor );
	void DebugDrawPrimitive( eDDP primitive, Go * from				, SiegePos const & to, DWORD primColor, const gpstring & name, DWORD nameColor );
*/	
	void GetPositionString( const SiegePos& Position, gpstring& output );

	void DrawDebugBox( const SiegePos &box_center, const matrix_3x3 &orient,
						const vector_3 &half_diag, const DWORD color, const float existence_duration,
						const bool world_space = false, const bool draw_origin = true, const gpstring& text=gpstring::EMPTY );

#	endif // !GP_RETAIL

private:

	struct DebugBox
	{
		DebugBox(	const SiegePos &box_center, const matrix_3x3 &orient, const vector_3 &half_diag,
					const float origin_scale, const DWORD color, const float existence_duration, const gpstring& text )
					: m_center( box_center )
					, m_orient( orient )
					, m_half_diag( half_diag )
					, m_origin_scale( origin_scale )
					, m_duration( existence_duration )
					, m_color( color )
					, m_elapsed( 0 )
					, m_text( text )
		{}

		SiegePos	m_center;
		matrix_3x3	m_orient;
		vector_3	m_half_diag;
		DWORD		m_color;
		float		m_origin_scale;
		float		m_duration;
		float		m_elapsed;
		gpstring	m_text;
	};

	struct DebugLine
	{
		DebugLine( const SiegePos & start, const SiegePos & end, DWORD color, float duration, const gpstring & sText )
			: m_start		( start )
			, m_end			( end )
			, m_color		( color )
			, m_duration	( duration )
			, m_text		( sText )
			, m_elapsed		( 0 )
		{}

		SiegePos	m_start;
		SiegePos	m_end;
		DWORD		m_color;
		float		m_duration;
		float		m_elapsed;
		gpstring	m_text;
	};


#	if !GP_RETAIL
	void	DrawDebugBoxes( float delta_t );
	void	DrawDebugLines( float delta_t );
#	endif // !GP_RETAIL

// Data members.

	// misc
	InventoryUpdateCb				m_InventoryUpdateCallback;
	DialogueCallbackCb				m_DialogueCallback;
	TestEquipCb						m_TestEquipCallback;
	PurchaseGuiCb					m_PurchaseGuiCb;
	StashGuiCb						m_StashGuiCb;
	RolloverHighlightCb				m_RolloverHighlightCb;
	PartyCb							m_PartyUpdateCb;
	CastCb							m_CastCb;

	// viewport
	float							m_ViewportWidth;
	float							m_ViewportHeight;
	float							m_ViewportFov;
	float							m_NormalFov;

	// debugging
	stdx::fast_vector< DebugBox >	m_DebugBoxes;
	stdx::fast_vector< DebugLine>	m_DebugLines;

// Permanent subsystems.

	my WorldOptions					* m_pOptions;
	my GoRpcMgr						* m_pGoRpcMgr;
	my NetLog                       * m_pNetLog;
	my TimeMgr						* m_pTimeMgr;
	my DrawTimeMgr					* m_pDrawTimeMgr;
	my WorldTime					* m_pTime;
	my AIAction						* m_pAIAction;
	my AIQuery						* m_pAIQuery;
	my WorldPathFinder				* m_pWorldPathFinder;
	my trigger::TriggerSys			* m_pTriggerSys;
	my MessageDispatch				* m_pMessageDispatch;
	my ContentDb					* m_pContentDb;
#	if !GP_RETAIL
	my DebugProbe					* m_pDebugProbe;
#	endif // !GP_RETAIL
	my MCP::Manager					* m_pMCP;
	my WorldSound					* m_pWorldSound;
	my GameAuditor					* m_pGameAuditor;
	my Victory						* m_pVictory;

// Data-constructed subsystems.

	my WorldFx					 	* m_pFlamethrower;
	my Sim					   		* m_pSim;
	my TimeOfDay			   		* m_pTimeOfDay;
	my CameraAgent			   		* m_pCameraAgent;
	my Rules				   		* m_pRules;
	my Weather				   		* m_pWeather;
	my Mood					   		* m_pMood;
	my WorldTerrain			   		* m_pWorldTerrain;

// Volatile subsystems.

	// $ m_pPlay is the single-player or multi-player coordination object. it
	//   gets recreated each time you start single or multiplayer game.

	my Server						* m_pServer;

	FUBI_SINGLETON_CLASS( World, "The set of systems representing the game's world." );
	SET_NO_COPYING( World );
};

#define gWorld World::GetSingleton()




//////////////////////////////////////////////////////////////////////////////
// report streams

#if !GP_RETAIL

ReportSys::Context& GetMCPContext( void );
#define gMCPContext GetMCPContext()

ReportSys::Context& GetMCPMessageContext( void );
#define gMCPMessageContext GetMCPMessageContext()

ReportSys::Context& GetMCPCrowdContext( void );
#define gMCPCrowdContext GetMCPCrowdContext()

ReportSys::Context& GetMCPInterceptContext( void );
#define gMCPInterceptContext GetMCPInterceptContext()

ReportSys::Context& GetMCPNetLogContext( void );
#define gMCPNetLogContext GetMCPNetLogContext()

ReportSys::Context& GetMCPSequencerContext( void );
#define gMCPSequencerContext GetMCPSequencerContext()

ReportSys::Context& GetMCPSignalContext( void );
#define gMCPSignalContext GetMCPSignalContext()

ReportSys::Context& GetAISkritContext( void );
#define gGetAISkritContext GetAISkritContext()

ReportSys::Context& GetAIMoveContext( void );
#define gGetAIMoveContext GetAIMoveContext()

ReportSys::Context& GetGamePlayContext( void );
#define gGetGamePlayContext GetGamePlayContext()
FEX ReportSys::Context * GamePlayContext( void );
FEX ReportSys::Context * PerfLogContext( void );

ReportSys::Context& GetJEDIContext( void );
#define gGetJEDIContext GetJEDIContext()

ReportSys::Context& GetTriggerSysContext( void );
#define gTriggerSysContext GetTriggerSysContext()

#endif // !GP_RETAIL report streams

// $ make sure to put the DEV_MACRO_REPORT stuff outside of !GP_RETAIL code.
//   they will auto-compile out in retail modes but must be outside !GP_RETAIL
//   to avoid causing problems in other code.

#define gpgameplay( msg )	DEV_MACRO_REPORT( &gGetGamePlayContext, msg )
#define gpgameplayf( msg )	DEV_MACRO_REPORTF( &gGetGamePlayContext, msg )

//////////////////////////////////////////////////////////////////////////////


#endif  // __WORLD_H

//////////////////////////////////////////////////////////////////////////////
