#pragma once
//
//	Flamethrower.h
//	Copyright 1999, Gas Powered Games
//
//

#ifndef _FLAMETHROWER_H_
#define	_FLAMETHROWER_H_


class	Flamethrower_interpreter;
class	Flamethrower_script;
class	Flamethrower_effect_factory;
class	TattooTracker;
class	Flamethrower_target;
class	Flamethrower;
class	SpecialEffect;


#include <string>
#include <vector>
#include <map>

#include "Flamethrower_types.h"
#include "Flamethrower_tracker.h"

#include "BlindCallback.h"
#include "vector_3.h"
#include "matrix_3x3.h"

#include "siege_engine.h"
#include "siege_pos.h"
#include "siege_walker.h"
#include "siege_database_guid.h"


class CollisionInfo;







class EffectParams
{
public:

	enum { INVALID_PARAM = 0xFFFFFFFF };

	enum PARAM_TYPE
	{
		PT_INVALID,
		PT_BOOL,
		PT_UINT32,
		PT_FLOAT,
		PT_VECTOR_3,
		PT_GPSTRING
	};

	struct effect_param
	{
		effect_param		( void )
			: m_bOverride	( false )
			, m_Type		( PT_INVALID )
			, m_bool		( false )
			, m_UINT32		( 0 )
			, m_float		( 0 ) {}

		effect_param		( const char* sName, bool value )
			: m_sParamName	( sName )
			, m_bOverride	( false	)
			, m_Type		( PT_BOOL )
			, m_bool		( value ) {}

		effect_param		( const char* sName, UINT32 value )
			: m_sParamName	( sName )
			, m_bOverride	( false	)
			, m_Type		( PT_UINT32 )
			, m_UINT32		( value ) {}

		effect_param		( const char* sName, float value )
			: m_sParamName	( sName )
			, m_bOverride	( false	)
			, m_Type		( PT_FLOAT )
			, m_float		( value ) {}

		effect_param		( const char* sName, vector_3 value )
			: m_sParamName	( sName )
			, m_bOverride	( false	)
			, m_Type		( PT_VECTOR_3 )
			, m_vector_3	( value ) {}

		effect_param		( const char* sName, const char *value )
			: m_sParamName	( sName )
			, m_bOverride	( false	)
			, m_Type		( PT_GPSTRING )
			, m_gpstring	( value ) {}

		gpstring	m_sParamName;	// name of parameter
		bool		m_bOverride;	// true if default has been overidden

		PARAM_TYPE	m_Type;

		// storage - $ should figure out if I can use union here
		bool		m_bool;
		UINT32		m_UINT32;
		float		m_float;
		vector_3	m_vector_3;
		gpstring	m_gpstring;
	};


	typedef	std::vector < effect_param >	paramColl;


	// Set the name of the effect
	void			SetEffectName	( const char* sEffectName )		{ m_sEffectName = sEffectName; }
	const char*		GetEffectName	( void )						{ return ( m_sEffectName.c_str() ); }

	// Storage - returns an index
	UINT32			SetBool			( const char* sParamName, bool value );
	UINT32			SetUINT32		( const char* sParamName, UINT32 value );
	UINT32			SetFloat		( const char* sParamName, float value );
	UINT32			SetVector3		( const char* sParamName, float value_x, float value_y, float value_z );
	UINT32			SetGPString		( const char* sParamName, const char* value );

	// Override defaults from string - returns false if syntax is incorrect or unrecognized param specified
	bool			ParseAndStore	( const char* sParams );


	// Retrieval (INVALID_PARAM if not found)
	UINT32			GetIndex		( const char* sParamName );
	bool			GetParam		( UINT32 index, effect_param **ppParam );

	// Returns true if not default param
	bool			GetBool			( UINT32 index, bool &value		);
	bool			GetUINT32		( UINT32 index, UINT32 &value	);
	bool			GetFloat		( UINT32 index, float &value	);
	bool			GetVector3		( UINT32 index, vector_3 &value );
	bool			GetGPString		( UINT32 index, gpstring &value );

	// Get rid of the used memory
	void			Clear			( void );

private:
	gpstring		m_sEffectName;
	paramColl		m_Params;
};




class Flamethrower : public Singleton <Flamethrower>
{
public:

	enum	{ MAX_POINT_INDEX = 600 };


	// Existence
	Flamethrower								( void );
	~Flamethrower								( void );

	void		Initialize						( const char *sScriptFuelAddress );
	void		Shutdown						( bool bFullShutdown = false );
	bool		IsEmpty							( void ) const;

	// Persistence
	void		PrepareForSave					( void );

	bool		Xfer							( FuBi::PersistContext& persist );


	// Implementation

	// Scaling factor for lights
	void		SetLightRadiusScaleFactor		( float factor )					{ m_LightRadiusScaleFactor = factor; }
	float		GetLightRadiusScaleFactor		( void )							{ return m_LightRadiusScaleFactor; }


	// Script interface

	bool		DestroyScript					( const char *sScriptName );

	void		RebuildScriptVocabulary			( void );

	SFxSID		RunScript						( const char *sName, def_tracker targets, const char *sParams, Goid ScriptOwner, eWorldEvent type, bool server, bool xfer );

	Goid		GetRegistrationGoid				( SFxSID id );

	void		FinishScript					( SFxSID id );

	void		SStopScript						( SFxSID id );
	
	void		SStopScript						( Goid id, const char *sName );

	void		StopScript						( SFxSID id );

	void		StopScript						( Goid id, const char *sName );

	void		StopAllScripts					( void );

	void		UnloadScript					( SFxSID id );

	void		LoadScript						( SFxSID id );

	bool		SetScriptCallback				( SFxSID id, const CBFunctor1< SFxSID > callback );

	void		PostSignal						( SFxSID id, const gpstring &sSignal );

	bool		FindScript						( SFxSID id, Flamethrower_script **pScript );


	Flamethrower_interpreter
				&GetInterpreter					( void )							{ return *m_pIInterpreter; }

	// Effect interface
	SFxEID		CreateEffect					( const char *sName, const char *sParam, def_tracker targets, Goid owner_id, SFxSID script_id, UINT32 seed );

	SpecialEffect*
				CreateEffect					( const char *sName );

	void		AddTarget						( SFxEID id, Flamethrower_target *pTarget );
	void		AddEffectTarget					( SFxEID id, SFxEID target_id );
	void		RemoveAllTargets				( SFxEID id );

	bool		GetTargets						( SFxEID id, TattooTracker **pTracker );
	void		SetTargets						( SFxEID id, def_tracker targets );

	void		AddFriendly						( SFxEID effect_id, Goid goid_id );

	void		StartEffect						( SFxEID id );
	void		StartAllEffects					( void );

	void		SetFinishing					( SFxEID effect_id );

	void		StopEffect						( SFxEID id );
	void		StopAllEffects					( void );

	void		DestroyEffect					( SFxEID id );
	void		DestroyAllEffects				( void );

	void		UnloadEffect					( SFxEID id );
	void		LoadEffect						( SFxEID id );

	SpecialEffect*
				FindEffect						( SFxEID id );

	bool		GetPosition						( SFxEID id, SiegePos &pos );
	bool		SetPosition						( SFxEID id, const SiegePos &pos );

	bool		GetOrientation					( SFxEID id, matrix_3x3 &orientation );
	bool		SetOrientation					( SFxEID id, const matrix_3x3 &orientation );

	bool		GetDirection					( SFxEID id, vector_3 &dir );
	bool		SetDirection					( SFxEID id, const vector_3 &dir );

	bool		OffsetPosition					( SFxEID id, vector_3 &offset, matrix_3x3 &basis );
	bool		OffsetTargetPosition			( SFxEID id, const TattooTracker::TARGET_INDEX n, const vector_3 &offset );

	bool		MakeTargetsStatic				( SFxEID id, bool snap_to_ground = false );
	def_tracker	MakeTrackerTargetsStatic		( def_tracker old_targets, bool snap_to_ground );
	bool		ConvertTargetToStatic			( Flamethrower_target *pOldTarget, Flamethrower_target *&pNewTarget, bool snap_to_ground );


	bool		GetBoundingRadius				( SFxEID id, float &radius );

	bool		AttachEffect					( SFxEID parent_id, SFxEID child_id );

	void		Update							( float SecondsElapsed, float UnpausedSecondsElapsed );

	void		InsertForCollection				( SpecialEffect *effect, const SiegePos & pos );

	void		CollectEffectsToDraw			( siege::WorldEffectColl &collection, const siege::SiegeNode &node );

	unsigned int
				GetGlobalTexture				( UINT32 texture_id );

	UINT16*		GetGlobalDrawIndexArray			( void )							{ return &m_GlobalDrawIndexArray[ 0 ]; }



	// --- Collision utility

	void		PostCollision					( CollisionInfo *pCollision );
	bool		FindCollision					( SFxEID id, CollisionInfo **pCollision );
	void		AddCollision					( CollisionInfo *pCollision )		{ m_CollisionRoster.push_back( pCollision ); }

	bool		EffectHasCollided				( SFxEID id, CollisionInfo &info );
	bool		GetCollisionPoint				( SFxEID id, SiegePos &point );
	bool		GetCollisionNormal				( SFxEID id, SiegePos &normal );
	bool		GetCollisionDirection			( SFxEID id, vector_3 &dir );
	bool		GetCollisionTargetID			( SFxEID id, Goid &target_id );

	
	// Effect parameters
	EffectParams&
				GetDefaultEffectParams			( const char *sName );


	// --- Debugging information

#	if !GP_RETAIL
	gpstring	GetActiveEffects				( void ) const;
	gpstring	GetActiveScripts				( void ) const;
	gpstring	GetAvailableEffects				( void ) const;
	gpstring	GetAvailableScripts				( void ) const;

	void		IncUpdatedEffects				( void )					{ ++m_UpdatedEffects; }
#	endif // !GP_RETAIL


private:

	void		StartEffectNow					( SFxEID id );
	void		StopEffectNow					( SFxEID id );
	void		DestroyEffectNow				( SFxEID id );
	void		DoEffectDestruction				( SEMap::iterator iParent, SEMap &list );
	void		UnloadEffectNow					( SFxEID id );
	void		LoadEffectNow					( SFxEID id );

	bool			m_bInitialized;			// Has Flamethrower been initialized yet?

	SEMap			m_DisabledEffects;		// unloaded
	SEMap			m_InitializedEffects;	// initialized
	SEMap			m_ActiveEffects;		// running
	def_EParamMap	m_DefaultEffectParams;	// All default registered effect parameters
	def_EffectMap	m_EffectMap;			// Active effects collected by node


	SFxEIDColl		m_StartStack;			// waiting to start
	SFxEIDColl		m_StopStack;			// waiting to stop
	SFxEIDColl		m_UnloadStack;			// waiting to get moved to the disabled list
	SFxEIDColl		m_LoadStack;			// waiting to get enabled again
	SFxEIDColl		m_DeathStack;			// waiting to die

	CollColl		m_CollisionRoster;		// Posted collisions

	float			m_LightRadiusScaleFactor;


	Flamethrower_effect_factory		*m_pIFactory;
	Flamethrower_interpreter		*m_pIInterpreter;

	std::vector <unsigned int>		m_GlobalTextureNames;


#if !GP_RETAIL
	UINT32			m_UpdatedEffects;		// This is incremented by any effect that doesn't early out of an update
#endif

	UINT16			m_GlobalDrawIndexArray[ 6 * MAX_POINT_INDEX ];
};

#define gFlamethrower Singleton <Flamethrower>::GetSingleton()


// $$: Todo move these somewhere else
inline void	ValidateOrder( float &min, float &max )
{
	if( min > max )
	{
		const float temp = max;
		max = min;
		min = temp;
	}
	else if( min == max )
	{
		max += FLOAT_TOLERANCE;
	}
}


inline void ValidateAngle( float &angle )
{
	if( (PI2 - angle) < 0 )
	{
		const float temp(( angle - PI2 ) / PI2);
		angle = PI2 * ( temp - floorf( temp ));
	}
}




class CollisionInfo
{
	FUBI_POD_CLASS( CollisionInfo );

public:

	CollisionInfo						( void );
	CollisionInfo						( SFxEID effect_id, Goid target_id, Goid owner_id, const SiegePos &pos, const SiegePos &norm, const vector_3 &dir );

	// Persistence
	bool		Xfer					( FuBi::PersistContext& persist );

	// Access
	SFxEID		GetID					( void ) const								{ return m_Effect_id; }
	Goid		GetTargetID				( void ) const								{ return m_Target_id; }
	Goid		GetOwnerID				( void ) const								{ return m_Owner_id; }
	bool		IsTerrainCollision		( void ) const								{ return m_bTerrainCollision; }
	SiegePos	&GetCollisionPoint		( void )									{ return m_Collision_point; }
	SiegePos	&GetCollisionNormal		( void )									{ return m_Collision_normal; }
	vector_3	&GetCollisionDirection	( void )									{ return m_Collision_direction; }

	float		LapseTimer				( float SSLC )								{ m_Elapsed -= SSLC; return m_Elapsed; }

	GPDEV_ONLY( void CheckValid( bool fullCheck = true ) const; )

private:

	float		m_Elapsed;
	SFxEID		m_Effect_id;
	Goid		m_Target_id;
	Goid		m_Owner_id;
	bool		m_bTerrainCollision;
	SiegePos	m_Collision_point;
	SiegePos	m_Collision_normal;
	vector_3	m_Collision_direction;
};


#endif	// _FLAMETHROWER_H_
