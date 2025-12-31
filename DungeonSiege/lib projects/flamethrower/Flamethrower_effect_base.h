#pragma once
#ifndef _FLAMETHROWER_EFFECT_BASE_H_
#define _FLAMETHROWER_EFFECT_BASE_H_
//
//	Flamethrower_effect_base.h
//	Copyright 1999, Gas Powered Games
//
//


class	Flamethrower;
class	TattooTracker;
class	SpecialEffect;


#include <string>
#include <vector>

#include "vector_3.h"
#include "matrix_3x3.h"

#include "Flamethrower.h"
#include "Flamethrower_tracker.h"
#include "Flamethrower_interpreter.h"

#include "siege_pos.h"
#include "siege_database_guid.h"


//
//	Base class for all special effects
//

#define PARAM(offset) (17+offset)

typedef	stdx::fast_vector< sVertex >	VertColl;
typedef stdx::fast_vector< vector_3 >	V3Coll;
typedef stdx::fast_vector< SiegePos >	PosColl;



class SpecialEffect
{
	friend class Flamethrower;

public:

	// --- Existence

	SpecialEffect();
	virtual ~SpecialEffect();

	void	operator = ( SpecialEffect &dup );

	// --- Persistence
	void	PrepareForSave			( void );
	void	PrepareForSaveChildren	( void );


	bool	Xfer			( FuBi::PersistContext& persist );
	bool	ShouldPersist	( void ) const;


	// --- Implementation

	void	Initialize	( UINT32 seed );
	void	Update		( const float SecondsElapsed, const float UnPausedSecondsElapsed );
	bool	Draw		( void );
	bool	ShouldDraw	( void ) const;


	// --- Utility

	void	AttachEffect( SFxEID id );
	void	RemoveEffect( SFxEID id );

	void	AddTarget	( Flamethrower_target *pTarget );
	void	NotifyScript( void );


	// --- Accessors
	inline bool						GetInterestOnly			( void ) const							{ return m_bInterestOnly; }
	inline void						SetInterestOnly			( bool val )							{ m_bInterestOnly |= val; }

	inline virtual bool				GetCullPosition			( SiegePos & CullPosition );
			virtual	float			GetBoundingRadius		( void )								{ return m_BoundingRadius; }

	inline bool						GetMarkedForDraw		( void ) const							{ return m_bMarkedForDraw; }
	inline void						SetMarkedForDraw		( void )								{ m_bMarkedForDraw = true; }

	inline virtual const SiegePos&	GetPosition				( void )								{ return m_Pos; }
	inline void						SetPosition				( const SiegePos &pos )					{ gpassert( pos.node.IsValid() ); m_Pos = pos; }

	inline const vector_3&			GetDirection			( void ) const							{ return m_Dir; }
	inline void						SetDirection			( const vector_3 &dir )					{ m_Dir = dir; }

	inline const matrix_3x3&		GetOrientation			( void ) const							{ return m_Orient; }
	inline void						SetOrientation			( const matrix_3x3 &orient)				{ m_Orient = orient; }

	const matrix_3x3&				GetTargetOrientation	( TattooTracker::TARGET_INDEX n = TattooTracker::SOURCE );
	const SiegePos&					GetTargetPosition		( TattooTracker::TARGET_INDEX n = TattooTracker::TARGET );

	inline float					GetDuration				( void ) const							{ return m_Duration; }
	inline void						SetDuration				( float dur )							{ m_Duration = dur; }

	inline SFxEID					GetID					( void ) const							{ return m_ID; }
	inline void						SetID					( SFxEID id )							{ m_ID = id; }

	inline const gpstring&			GetName					( void ) const							{ return m_sName; }
	inline void						SetName					( const gpstring &sName )				{ m_sName = sName; }

	inline EffectParams&			GetParams				( void )								{ return m_Params; }
	inline void						SetParams				( const EffectParams &Params )			{ m_Params = Params; }

	inline bool						IsLooping				( void ) const							{ return m_bLoop; }
	inline bool						IsFinished				( void ) const							{ return m_bFinished; }
	inline bool						IsFinishing				( void ) const							{ return m_bFinishing; }
	inline bool						HasTargets				( void ) const							{ return m_Targets->AtLeastOneTarget(); }
	inline virtual bool				UpdatesDuringDelay		( void ) const							{ return false; }

	inline void						EndThisEffect			( void )								{ m_bFinished = true; }

	inline SFxEID					GetParentEffectID		( void ) const							{ return m_ParentEffectID; }
	inline void						SetParentEffectID		( SFxEID id )							{ m_ParentEffectID = id; }
	inline bool						IsChild					( void ) const							{ return m_bIsChild; }
	inline void						GetChildren				( SFxEIDColl &list )					{ list = m_Children; }
	inline SFxEIDColl &				GetChildren				( void )								{ return m_Children; }
	inline bool						HasChildren				( void ) const							{ return !m_Children.empty(); }

	void							OwnTargets				( def_tracker &t );
	inline const def_tracker&		GetTargets				( void ) const							{ return m_Targets; }
	inline void						GetTargets				( TattooTracker **pT )					{ *pT = m_Targets.get(); }
	inline def_tracker				ReleaseTargets			( void )								{ return m_Targets; }

	inline void						RemoveAllTargets		( void )								{ m_Targets->RemoveAllTargets(); }

	inline void						SetOwnerID				( Goid id )								{ m_OwnerID = id; }
	const Goid						GetOwnerID				( void )								{ return m_OwnerID; }

	inline void						SetParentScriptID		( SFxSID id )							{ m_ParentScriptID = id; }
	inline SFxSID					GetParentScriptID		( void ) const							{ return m_ParentScriptID; }

	inline void						AddFriendly				( Goid id )								{ m_Friendlys.push_back( id ); }
	GoidColl&						GetFriendly				( void )								{ return m_Friendlys; }

	inline void						SetIsVisible			( bool bVisible )						{ m_bVisible = bVisible; }
	inline bool						GetIsVisible			( void ) const							{ return m_bVisible; }

	inline bool						GetMustUpdate			( void ) const							{ return m_bMustUpdate; }
	inline bool						GetHasMoved				( void ) const							{ return m_bHasMoved; }
	inline bool						GetUpdated				( void ) const							{ return m_bUpdated; }

	inline void						SetNodeAlpha			( float alpha )							{ m_NodeAlpha = alpha; }

	inline float					GetMinFPS				( void ) const							{ return m_MinFPS; }

	// Parameter registration
	void							RegisterDefaultParams	( EffectParams &params );

	GPDEV_ONLY( void CheckValid( void ) const; )

protected:

	// --- Utility

	void	PostCollision( Goid target_id, const SiegePos &pos, const SiegePos &norm, const vector_3 &dir );
	void	SetDrawMatrix( const siege::database_guid &guid );


	virtual bool	OnXfer( FuBi::PersistContext &persist ) = 0;

	// --- Initialization
	virtual void	RegisterDerivedParams( EffectParams &params ) = 0;

	virtual void	InitializeDerived( void ) = 0;
	virtual void	InitializeTextures( void );

	virtual bool	ReInitialize( void )			{ return true; }	// When an effect is loaded handle any special case stuff here
	virtual bool	UnInitialize( void )			{ return true; }	// Wnen an effect is unloaded handle any special case stuff here

	virtual UINT32	InitializeTexture( void )		{ return 0; }


	// --- Update

	virtual void	UpdateDerived( void ) = 0;
	virtual void	DrawSelf( void ) = 0;
	virtual	void	Track( void );
	virtual	void	Put( void );


	// --- Misc

	virtual void	SetFinishing( void )			{ m_bFinishing = true; }



	// --- Data members

	gpstring						m_sName;				// Name of the effect
	EffectParams					m_Params;				// Params for this effect  $ shouldn't have to xfer this
	SFxEIDColl						m_Children;				// Other effects attached to this one
	GoidColl						m_Friendlys;			// Gos to avoid collisions with
									
	SFxEID							m_ParentEffectID;		// The id of the parent effect that this effect is a child of
	SFxSID							m_ParentScriptID;		// The parent script this effect belongs to
									
	SFxEID							m_ID;					// The id of this effect
	Goid							m_OwnerID;				// The game object that owns this effect
									
	bool							m_bAbsoluteCoords;		// Forces unit scale
	float							m_Scale;				// The final scale
	float							m_ScaleFactor;			// The factor of the scale
	float							m_BoundingRadius;		// Radius that this effect could be bound in
									
	float							m_NodeAlpha;			// The associated alpha for the node this effect is in
									
	DWORD							m_Seed;					// Random seed for this effect
									
	// State flags					
	bool							m_bFinished;			// True if effect is finished
	bool							m_bFinishing;			// True if effect is in the process of finishing
	bool							m_bLoop;				// ?
	bool							m_bIsChild;				// True if this effect is attached to another effect

	// Orientation mode
	UINT32							m_OrientMode;			// Orient to the world, go, bone of a go

	// Visual immunities
	bool							m_bWindAffected;		// True if wind does not have an effect
	bool							m_bPauseImmune;			// Flag for not using game time so an effect updates when paused
	bool							m_bUseFog;				// Effect will overbrighten with fog

	// Initialization flagging
	bool							m_bInitialized;			// For deferring initialization until the first run through the update

	// Texture information
	bool							m_bUseLoadedTexture;	// Flag for specifying the request of a loaded texture
	UINT32							m_GlobalTextureID;		// Index for alogrithmic texture created by Flamethrower
									
	gpstring						m_sTextureName;			// For loaded textures the name is stored for recreation reference
	unsigned int					m_TextureName;			// Handle for specified texture
	UINT32							m_TextureWidth;			// Texture width
	UINT32							m_TextureHeight;		// Texture height
									
	// Rotation storage				
	bool							m_bRotation;			// Flag to specify if this effect has had rotation speicified
	vector_3						m_RotationAngles;		// The specified angles of rotation for this effect
	matrix_3x3						m_RotationMatrix;		// Storage for rotation angles
									
	// Camera Orientation			
	matrix_3x3						m_Cam;					// Camera orientation
	vector_3						m_Vx, m_Vy, m_Vz;		// Camera orientation column vectors
									
	// Node orientation				
	matrix_3x3						m_NodeOrient;			// Node orientation
	vector_3						m_NodeCentroid;			// Node center
									
	// Global parameters			
	bool							m_bMarkedForDraw;		// Set when collecting effects to draw so that an effect will draw only once
	SiegePos						m_Pos;					// Position
	matrix_3x3						m_Orient;				// Orientation
	vector_3						m_Dir;					// Direction
									
	SiegePos						m_ReportedPos;			// Reported postion used by GetPosition for a const ref
	matrix_3x3						m_ReportedOrient;		// Reported orientation used by GetOrientation for a const ref
									
	vector_3						m_Offset;				// Positional offset
	float							m_TimeScalar;			// Factor for scaling time by
	float							m_Delay;				// Delay before starting effect
	float							m_Duration;				// Duration of the effect
	float							m_Elapsed;				// Elapsed time of the effect
	float							m_FrameSync;			// Seconds since last call or Frequency
									
	bool							m_bInterestOnly;		// If an effect was ever once targetting an interest only go then it is an interest only effect
	def_tracker						m_Targets;				// Target container
									
	bool							m_bFast;				// True if this effect should run fast - in other words allow offsets outside of a node
	bool							m_bVisible;				// True if this effect is visible
	bool							m_bMustUpdate;			// True if this effect should always update like if it one that does damage
	bool							m_bHasMoved;			// True if this effect has moved and should be updated
	bool							m_bUpdated;				// True if this effect was updated during the update loop
									
	float							m_MinFPS;				// Minimum framerate in frames per second for this effect
};


#endif	// _FLAMETHROWER_EFFECT_BASE_H_
