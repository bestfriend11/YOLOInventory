#pragma once
#ifndef _WORLDFX_H_
#define	_WORLDFX_H_
//
//	WorldFx.h
//	Copyright 1999, Gas Powered Games
//
//
//

class	Flamethrower_target;
class	Flamethrower;
class	TattooTracker;			// Helper class for tracking Tattoo GOs

class	Rapi;
class	Siege;


#include <vector>

#include "vector_3.h"

#include "Flamethrower.h"
#include "Flamethrower_interpreter.h"
#include "Flamethrower_tracker.h"

#include "siege_database_guid.h"
#include "siege_engine.h"
#include "siege_pos.h"

#include "WorldOptions.h"


#include "bonetranslator.h"



/*=======================================================================================

  WorldFx

  purpose:

  author:	Rick Saenz

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/

class WorldFx : public Singleton < WorldFx >
{
public:


	// Existence
	WorldFx												( const char *sScriptFuelAddress );
	~WorldFx											( void );

	void				Shutdown						( void );
	bool				IsEmpty							( void ) const;

	void				PrepareForSave					( void );
	bool				Xfer							( FuBi::PersistContext& persist );

	// Implementation
	//
	// External (calls made from Tattoo)
	//

	void				SetLightRadiusScaleFactor		( float factor );
	float				GetLightRadiusScaleFactor		( void );

#	if !GP_RETAIL
	void				SetShowVolumes					( bool setting )	{ m_bShowVolumes = setting; }
	bool				GetShowVolumes					( void ) const		{ return m_bShowVolumes; }
#	endif
	// Scripted effect control
	void				RebuildScriptVocabulary			( void );

	bool				DestroyScript					( const char *sName );


	SFxSID				SRunScript						( const char *sName, def_tracker targets, const char *sParams, Goid Owner, eWorldEvent type, bool server = true, bool xfer = false );
	SFxSID				SRunMpScript					( const char *sName, def_tracker targets, const char *sParams, Goid Owner, eWorldEvent type, bool server = true )  {  return ( SRunScript( sName, targets, sParams, Owner, type, server, true ) );  }
	SFxSID				RunScript						( const char *sName, def_tracker targets, const char *sParams, Goid Owner, eWorldEvent type )  {  return ( SRunScript( sName, targets, sParams, Owner, type, false, false ) );  }
	SFxSID				RunMpScript						( const char *sName, def_tracker targets, const char *sParams, Goid Owner, eWorldEvent type )  {  return ( SRunScript( sName, targets, sParams, Owner, type, false, true ) );  }

	SFxSID				SRunScript						( const char *sName, Goid target, Goid source ,const char *sParams, Goid Owner, eWorldEvent type, bool server, bool xfer );
FEX SFxSID				SRunScript						( const char *sName, Goid target, Goid source ,const char *sParams, Goid Owner, eWorldEvent type )  {  return ( SRunScript( sName, target, source, sParams, Owner, type, true, false ) );  }
FEX SFxSID				SRunMpScript					( const char *sName, Goid target, Goid source ,const char *sParams, Goid Owner, eWorldEvent type )  {  return ( SRunScript( sName, target, source, sParams, Owner, type, true, true ) );  }
FEX	SFxSID				RunScript						( const char *sName, Goid target, Goid source, const char *sParams, Goid Owner, eWorldEvent type )  {  return ( SRunScript( sName, target, source, sParams, Owner, type, false, false ) );  }


	bool				SRunScriptSpecial				( const char *sName, const char *sParams, Goid Owner, eWorldEvent type, const SiegePos &target_pos,
														float radius, float x_offset, float y_offset, float z_offset, bool server );

FEX	bool				SRunScriptSpecial				( const char *sName, const char *sParams, Goid Owner, eWorldEvent type, const SiegePos &target_pos,
														float radius, float x_offset, float y_offset, float z_offset )
														{  return ( SRunScriptSpecial( sName, sParams, Owner, type, target_pos, radius, x_offset, y_offset, z_offset, true ) );  }

FEX	bool				RunScriptSpecial				( const char *sName, const char *sParams, Goid Owner, eWorldEvent type, const SiegePos &target_pos,
														float radius, float x_offset, float y_offset, float z_offset )
														{  return ( SRunScriptSpecial( sName, sParams, Owner, type, target_pos, radius, x_offset, y_offset, z_offset, false ) );  }

FEX void				SStopScript						( SFxSID id );
FEX void				SStopScript						( Goid id, const char *sName );
FEX	void				StopScript						( SFxSID id );
FEX	void				StopScript						( Goid id, const char *sName );

	void				FinishScript					( SFxSID id );
FEX	void				FUBI_RENAME						( FinishScript )( SFxSID id )				{  FinishScript( id );  }
						FUBI_MEMBER_DOC					( FinishScript, "id", "Tell the given SiegeFx script to finish its effect (it can take it's time)" );


	// This function replaces all of the targets in a currently running script with the ones specified
	void				ReplaceScriptTargetIDs			( SFxSID script_id, Goid target, Goid source );

	// This function returns what Go the script should be registered with as it may have to create the go sometimes
	Goid				GetRegistrationGoid				( SFxSID id ) const;

	// stop all currently executing scripts
	void				StopAllScripts					( void );

	// For Unloading
	void				UnloadScript					( SFxSID id );

	void				LoadScript						( SFxSID id );

	// get all of the effects for a given script
	void				GetEffectsForScript				( SFxSID id, SEVector & effectColl );

	// set the callback for an individual script
	// The validator is the go that the script is run on that can be verified as being in the game still so the blind
	// callback doesn't leap before it looks to see if it can jump there.
	bool				SetScriptCallback				( SFxSID id, const CBFunctor1< SFxSID > callback );
	bool				ClearScriptCallback				( SFxSID id );

	// external entrypoint for sending script signals
	void				PostSignal						( SFxSID id, const gpstring &strSignal );

	// true if this script cares about signals
	bool				ScriptUsesSignals				( SFxSID id )						{ return ( ((DWORD)id & 0x00000100) == 256); }

	// collision ignore information
	void				AddFriendly						( SFxEID effect_id, Goid id );

	// Tells an effect to get into it's finishing state
	void				SetFinishing					( SFxEID effect_id );

	// Singular effect control

	SFxEID				CreateEffect					( const char *sName, const char *sParams, def_tracker targets, Goid owner_id, SFxSID script_id, UINT32 seed );

	// Creates an uninitialized effect - used for restoring
	SpecialEffect*		CreateEffect					( const char *sName );

	void				DestroyEffect					( SFxEID id );
	void				DestroyAllEffects				( void );

	void				UnloadEffect					( SFxEID id );
	void				LoadEffect						( SFxEID id );

	SpecialEffect*		FindEffect						( SFxEID id );

	bool				GetDirection					( SFxEID id, vector_3 &dir );
	bool				SetDirection					( SFxEID id, const vector_3 &dir );

	bool				GetOrientation					( SFxEID id, matrix_3x3 &orientation );
	bool				SetOrientation					( SFxEID id, const matrix_3x3 &orientation );

	bool				GetPosition						( SFxEID id, SiegePos &pos );
	bool				SetPosition						( SFxEID id, const SiegePos &pos );

	bool				OffsetPosition					( SFxEID id, vector_3 &offset, matrix_3x3 &basis );
	bool				OffsetTargetPosition			( SFxEID id, const TattooTracker::TARGET_INDEX index,
														const vector_3 &offset );

	bool				MakeTargetsStatic				( SFxEID id, bool snap_to_ground = false );
	def_tracker			MakeTrackerTargetsStatic		( def_tracker old_targets, bool snap_to_ground );
	void				ConvertTargetToStatic			( Flamethrower_target *pOldTarget, Flamethrower_target *&pNewTarget, bool snap_to_ground );

	void				SetInterestOnly					( SFxEID id, bool val );

	bool				GetBoundingRadius				( SFxEID id, float &radius );
						
	void				AddEffectTarget					( SFxEID id, SFxEID target_id );
	void				AddTarget						( SFxEID id, Flamethrower_target *pTarget );
	void				RemoveAllTargets				( SFxEID id );
	bool				AttachEffect					( SFxEID parent_id, SFxEID child_id );
						
	void				StartEffect						( SFxEID id );
	void				StartAllEffects					( void );
						
	void				StopEffect						( SFxEID id );
	void				StopAllEffects					( void );
						
	void				SetTargets						( SFxEID id, def_tracker targets );
						
	bool				GetTargets						( SFxEID id, TattooTracker **pTracker );
						
	void				Update							( float SecondsElapsed, float UnpausedSecondsElapsed );
						
	void				CollectEffectsToDraw			( siege::WorldEffectColl &collection, const siege::SiegeNode &node );

	unsigned int		GetGlobalTexture				( UINT32 texture_id ) const					{ return IFlamethrower->GetGlobalTexture( texture_id ); }
	UINT16*				GetGlobalDrawIndexArray			( void ) const								{ return IFlamethrower->GetGlobalDrawIndexArray(); }

	// Accessors
	Rapi&				GetRenderer						( void ) const								{ return gSiegeEngine.Renderer(); }
#	if !GP_RETAIL
	gpstring			GetActiveEffects				( void ) const								{ return IFlamethrower->GetActiveEffects(); }
	gpstring			GetActiveScripts				( void ) const								{ return IFlamethrower->GetActiveScripts(); }
	gpstring			GetAvailableEffects				( void ) const								{ return IFlamethrower->GetAvailableEffects(); }
	gpstring			GetAvailableScripts				( void ) const								{ return IFlamethrower->GetAvailableScripts(); }
#	endif // !GP_RETAIL

	void				PostCollision					( CollisionInfo *pTakeCollision );
						
	bool				EffectHasCollided				( SFxEID id, CollisionInfo &info );
	bool				GetCollisionPoint				( SFxEID id, SiegePos &point );
	bool				GetCollisionNormal				( SFxEID id, SiegePos &normal );
	bool				GetCollisionDirection			( SFxEID id, vector_3 &dir );
	bool				GetCollisionTargetID			( SFxEID id, Goid &target_id );
						
	void				Message							( const char *szEvent, Goid from, Goid to, DWORD data );
						
	static def_tracker	MakeTracker						( void );
	static def_tracker	MakeTracker						( Goid target, Goid source );


	// Random number service

	RandomGeneratorNR&	GetRNG							( void )									{ return m_RandomNumberGenerator; }


	//
	// Internal	( Calls made from Flamethrower )
	//

	bool				IsServerLocal					( void );
						
	void				UnRegisterScriptsFromGO			( Flamethrower_script &Script );
						
	bool				GetCullState					( void ) const;
						
	bool				IsEffectVisible					( SpecialEffect * pEffect );
	bool				IsTargetVisible					( SpecialEffect * pEffect, Flamethrower_target * pTarget, bool & bCulledByInventory );

	bool				IsInWorldFrustum				( const siege::database_guid & node );
	bool				IsNodeVisible					( const siege::database_guid & node );

	bool				IsScriptOwnerVisible			( const SFxSID& script_id );
						
						
	void				SaveRenderingState				( void ) const;
	void				RestoreRenderingState			( void ) const;

	const vector_3&		GetCameraPosition				( void ) const								{ return m_CamPos; }
	const vector_3&		GetCameraDirection				( void ) const								{ return m_CamDir; }
	const matrix_3x3&	GetCameraOrientation			( void ) const								{ return m_CamOrient; }

	void				SetCameraOrientation			( const matrix_3x3 &orient )				{ m_CamOrient = orient; }
						
	float				GetGroundHeight					( const SiegePos &position );
						
	bool				RayCast							( const SiegePos &start_pos, const SiegePos &end_pos, vector_3 &normal, SiegePos &point );
						
	bool				UpdateNodePosition				( SiegePos &position );
	bool				AdjustPointToTerrain			( SiegePos &position );
						
	bool				CheckForCollision				( bool bIgnoreObjects, 
														const SiegePos &start_position,
														const SiegePos &end_position,
														float Width, float Height,
														const GoidColl & ignore_list,
														Goid &ContactGUID,
														SiegePos &ContactPoint,
														SiegePos &ContactNormal, Goid Owner );
						
	bool				CreateLightSource				( siege::database_guid &light_id, const SiegePos &position, const vector_3 &color,
															float inner_radius, float outer_radius, bool bCastShadow );
						
	bool				PositionLightSource				( const siege::database_guid &light_id, const SiegePos &position, unsigned long color);
						
	void				DestroyLightSource				( const siege::database_guid &light_id );
						
	void				SpawnLocalPhysicsObject			( const char *sName, const SiegePos &initial_position,
														  const vector_3 &linear_acceleration, const vector_3 &linear_velocity,
														  const vector_3 &angular_acceleration, const vector_3 &angular_velocity,
														  const GoidColl & ignore_list );
						
	Goid				CreateEffectObject				( Goid owner, const char *sName, const SiegePos &initial_position );
						
	void				SetEffectObjectPosition			( Goid model, const SiegePos &position );
	void				SetEffectObjectOrientation		( Goid model, const matrix_3x3 &orient );
						
	void				RotateEffectObject				( Goid model, const vector_3 &angles );
	void				DestroyEffectObject				( Goid model );
						
	float				GetEffectObjectScale			( Goid model );
	void				SetEffectObjectScale			( Goid model, float scale_factor );
						
	bool				CreateDecal						( unsigned int &decal_id, const SiegePos &pos, const matrix_3x3 &orient, float hMeters,
															float vMeters, float nearPlane, float farPlane,	const char *sTexture );
						
	void				DestroyDecal					( unsigned int decal_id, bool remove_texture );
						
	bool				SetDecalAlpha					( unsigned int decal_id, float alpha );
						
	void				GetWorldWindVector				( vector_3 &vector ) const;
	void				GetNodeWindVector				( const siege::database_guid &node, vector_3 &vector ) const;
						
	void				GetTargetNodeGUID				( siege::database_guid &guid ) const;

	const matrix_3x3&	GetNodeOrientation				( const SiegePos &position );
	const vector_3&		GetNodeCenter					( const SiegePos &position );
	void				GetNodeOrientAndCenter			( const SiegePos &position, matrix_3x3 &orient, vector_3 &center );
	void				GetNodeOrientAndCenter			( const siege::database_guid node, matrix_3x3 &orient, vector_3 &center );

	DWORD				PlaySample						( const char *sName, const SiegePos &position, bool loop, float min_dist, float max_dist );
	DWORD				PlaySample						( const char *sName, const SiegePos &position, bool loop );
	DWORD				PlaySample						( const char *sName, bool loop );
	void				StopSample						( DWORD id );
	float				GetSampleLengthInSeconds		( const char *sName );
	void				SetSampleStopCallback			( DWORD id, Flamethrower_script *pScript );
							
						
	bool				FindScript						( SFxSID id, Flamethrower_script **pScript );
						
	void				ApplyDamage						( Goid Attacker, Goid Victim, float damage_min, float damage_max, bool bIgnite, float t, bool bCalcFireResist );
						
	void				DamageWithinBox					( const SiegePos &position, const matrix_3x3 &orient, const vector_3 &half_diag,
															float damage_min, float damage_max, bool bIgnite, float t, bool bCalcFireResist, const GoidColl &ignore, Goid Owner );
						
	bool				AddTargetsWithinSphere			( const SiegePos &position, float radius, const GoidColl &ignore, Goid Owner, UINT32 &AddMax, def_tracker &targets );
						
						
	bool				RegisterEffectScript			( Goid object, SFxSID id, eWorldEvent type, bool xfer, const Flamethrower_params* originalParams );
						
	SFxSID				GetLastScriptID					( void )								{ return m_LastScriptID; }

	static WorldFx&		GetSingleton					( void )								{ return ( Singleton <WorldFx>::GetSingleton() );  }


	float				GetDetailLevel					( void )								{ return gWorldOptions.GetObjectDetailLevel(); }
	bool				IsBloodRed						( void )								{ return IFlamethrower->GetInterpreter().IsBloodRed(); }
	bool				IsBloodEnabled					( void )								{ return IFlamethrower->GetInterpreter().IsBloodEnabled(); }


	// Generic positional helper functions

	// false upon entering invalid node
	bool				AddPositions					( SiegePos &pos, const vector_3 &add, bool bFast, bool * pbNodeChange );
						
	void				ReOrientVector					( const siege::database_guid &old_node, const siege::database_guid &new_node, vector_3 &vector );
	void				GetDifferenceOrient				( const siege::database_guid &old_node, const siege::database_guid &new_node, matrix_3x3 &difference );

	// Persistent variable storage
FEX void				ClearVariables					( Goid owner );

FEX	UINT32				AddVariable						( SiegePos &position, Goid owner );
FEX	SiegePos&			GetVariable						( UINT32 index, Goid owner );
FEX	void				SetVariable						( UINT32 index, SiegePos &position, Goid owner );


	bool				ValidTargets					( const def_tracker& targets, const char *sName );

	EffectParams&		GetDefaultEffectParams			( const char* sName );
#	if !GP_RETAIL
	void				DoUpdateSanityCheck				( void );		
	void				DoDrawSanityCheck				( void );		
#	endif

	// for drawing special effects in the front end where the node will always be UNDEFINED_GUID
	void				SetAllowUndefinedNodeGUID		( bool bSet )							{ m_bAllowUndefinedNodeGUID = bSet; }
	bool				GetAllowUndefinedNodeGUID		( void )								{ return m_bAllowUndefinedNodeGUID; }


protected:

	float				m_SSLC;

	Flamethrower		*IFlamethrower;

	matrix_3x3			m_CamOrient;
	vector_3			m_CamPos;
	vector_3			m_CamDir;

	SFxSID				m_LastScriptID;

	RandomGeneratorNR	m_RandomNumberGenerator;
	DWORD				m_RandomSeed;

#	if !GP_RETAIL
	bool				m_bShowVolumes;
	UINT32				m_EffectsDrawn;
	UINT32				m_EffectsDrawnLimit;
	UINT32				m_EffectsUpdated;
	UINT32				m_EffectsUpdatedLimit;
#endif

	typedef stdx::fast_vector< SiegePos >	SPosVector;
	typedef std::map< Goid, SPosVector >	SPosMap;

	SPosMap				m_SiegePosVars;

	SiegePos			m_NullPosition;
	bool				m_bAllowUndefinedNodeGUID;


	FUBI_SINGLETON_CLASS( WorldFx, "Interface to Siege Engine effects (SiegeFx)." );
	SET_NO_COPYING( WorldFx );
};

#define gWorldFx WorldFx::GetSingleton()





/*=======================================================================================

  Tattoo specific target types defined for use with Flamethrower

  GoTarget	=	DEFAULT : Standard target type for any go - can move and have bones
  GoEmitter	=	EMITTER : Target can move but has no bones
  GoStatic	=	STATIC	: Target can't move and has no bones.

---------------------------------------------------------------------------------------*/
class GoTargetBase : public Flamethrower_target
{
public:
	GoTargetBase( eTargetType type ) : Flamethrower_target( type ) {}

	bool		IsValid					( bool* shouldDelete = NULL );
	bool		GetInterestOnly			( void );
	bool		GetIsInsideInventory	( void );

protected:
	GoTargetBase( void ) {}
};


class GoTarget : public GoTargetBase
{
public:
	GoTarget	( Goid GO, const char* sBoneName = NULL );
	GoTarget	( Goid GO, int boneIndex );
	GoTarget	( void ) {}
	~GoTarget	( void ) {}

	bool		OnXfer					( FuBi::PersistContext &persist );
	bool		ShouldPersist			( bool& hasGoid ) const;

	bool		OnDuplicate				( Flamethrower_target **pTarget );

	bool		GetPlacement			( SiegePos *pPos, matrix_3x3 *pOrient, UINT32 orient_mode = Flamethrower_target::ORIENT_GO, 
										const gpstring &sBoneName = gpstring::EMPTY );

	bool		GetDirection			( vector_3 &direction );
	bool		GetBoundingRadius		( float &radius );

	void		SetPositionOffsetName	( const gpstring &sName );
	int			GetNemaBoneIndex		( void ) const  {  return ( m_nema_bone_index );  }

private:

	int			m_nema_bone_index;

};


class GoEmitter : public GoTargetBase
{
public:
	GoEmitter	( Goid GO );
	GoEmitter	( void ) {}
	~GoEmitter	( void ) {}

	bool		OnDuplicate				( Flamethrower_target **pTarget );

	bool		GetPlacement			( SiegePos *pPos, matrix_3x3 *pOrient, UINT32 orient_mode = Flamethrower_target::ORIENT_GO, 
										const gpstring &sBoneName = gpstring::EMPTY );

	bool		GetDirection			( vector_3 &direction );

};


class GoStatic : public GoTargetBase
{
public:
	GoStatic	( const SiegePos &position );
	GoStatic	( void ) {}
	~GoStatic	( void );

	bool		OnDuplicate				( Flamethrower_target **pTarget );

	bool		GetPlacement			( SiegePos *pPos, matrix_3x3 *pOrient, UINT32 orient_mode = Flamethrower_target::ORIENT_GO, 
										const gpstring &sBoneName = gpstring::EMPTY );
};


#endif	//	_WORLDFX_H_
