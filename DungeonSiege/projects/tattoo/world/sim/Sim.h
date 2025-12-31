#pragma once
/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: Sim.h

	Author(s)	: Rick Saenz

	Purpose		: Physics simulation

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#ifndef _SIM_H_
#define _SIM_H_

class	Sim;
struct	SimObj;
struct	SimExp;
struct	SimDmg;


#include "vector_3.h"
#include "space_3.h"

#include "siege_database_guid.h"
#include "siege_engine.h"
#include "siege_pos.h"


#include <vector>



// $todo: make these actually be different types
typedef	UINT32	SimID;




class	Sim : public Singleton <Sim>
{
public:
	// --- Enums & Types
	enum { ID_INVALID			= 0		};
	enum { DEFAULT_SIM_TIMEOUT	= 60	};

	struct	Explosion;
	typedef	std::list< Explosion >	ExplosionColl;
	typedef	std::list< SimID	 >	SimIDColl;
	typedef std::list< SimObj*	 >	SimObjColl;
	typedef	std::list< SimExp*	 >	SimExpColl;
	typedef std::list< SimDmg*	 >	SimDmgColl;



	// --- Existence
	Sim();
	~Sim();

	void	Shutdown				( void );
	bool	IsEmpty					( void ) const;

	// --- Persistence
	bool	Xfer					( FuBi::PersistContext &persist );



	// --- Creation

	// Creates and starts simulation on specified object
	void	SpawnLocalPhysicsObject	( const gpstring &sName, const SiegePos &initial_position,
									const vector_3 &linear_acceleration, const vector_3 &linear_velocity, 
									const vector_3 &angular_acceleration, const vector_3 &angular_velocity,
									const GoidColl &ignore_list );

	// Fire projectile returns true if hittable with given velocity - 'weapon' must be an actor or a weapon that is parented to an actor
	bool	SFireProjectile			( SimID &pid, Go* ammo, Go* weapon, Goid intended_target, const GoidColl &ignorables, const SiegePos &firing_position, 
									const SiegePos &target_position, float velocity, float x_error = 0, float y_error = 0 );
	void	FireProjectile			( Goid ammo_id, Goid weapon_id, const SiegePos &firing_position, const vector_3 &velocity_vector, DWORD seed );



	// --- Explosions

	// Breaks an object apart only - does not do any damage whatsovever
FEX	bool	SExplodeGo				( Goid Object, float magnitude, const vector_3 &initial_velocity = vector_3::UP );
	void	RCExplodeGo				( Goid Object, float magnitude, const vector_3 &initial_velocity, DWORD seed );
	void	ExplodeGo				( Goid Object, float magnitude, const vector_3 &initial_velocity, DWORD seed );

	// Calls ExplodeGo and Create explosion with parameters supplied by go
FEX bool	ExplodeGoWithDamage		( Goid Object, Goid Owner, Goid OwnerWeapon );

	// Creates an explosion at the specified point and damages any game objects in the vicinity
FEX SimID	CreateExplosion			( const SiegePos &position, float radius, float magnitude, float damage, Goid Owner, Goid OwnerWeapon );



	// --- Damage Volumes

	// Create a damage volume using two static points
FEX	SimID	CreateDamageVolume		( const SiegePos &start_pos, const SiegePos &end_pos, float start_radius, float end_radius,
									float damage, float duration, Goid Owner, Goid OwnerWeapon );

	// Create a damage volume using two targets and deriving the positions from their positions, a length of 0 will make sure it is perfect length 
FEX SimID	CreateDamageVolume		( Goid source, Goid target, float length, float start_radius, float end_radius, float damage, float duration,
									Goid Owner, Goid OwnerWeapon );


	// --- Animated item dropping

FEX	void	StartDropAnimation		( Goid item, const SiegePos &dropPos, const Quat &dropQuat );



	// --- Simulation control

	// Creates a simulation from the information stored in the specified GOs physics component block
	bool	StartSimulation			( Goid object, SimID &pid );

	// Stops all currently active simulations
	void	StopAllSimulations		( void );

	// Removes a simulated object and destroys it's visual representation
	void	RemoveObject			( SimID id );

	// Starts simulation for objects that touch each other
	void	TriggerDependents		( Goid object, SimID source_object, UINT32 recursion = 1 );

	// Gets the associated Simulation id for specified game object if one exists - returns Sim::ID_INVALID if it doesn't exist
FEX	SimID	GetSimID				( Goid object );

	// For sniping ability
inline float	GetSniperRange		( void ) const				{ return m_SniperRange; }
		void	SetSniperRange		( float range )				{ m_SniperRange = min_t( 90.0f, range ); }

	// --- Collisions

	// Call through to check for a collision and get resultant information
	bool	CheckForTerrainCollision( SiegePos &ContactPoint, SiegePos &ContactNormal, const SiegePos &start_position, const SiegePos &end_position );

	bool	CheckForObjectCollision	( SiegePos &contact_point, SiegePos &contact_normal, Goid & contact_id,	const GoidColl &ignore_list,
									const SiegePos &start_pos, const SiegePos &end_pos, float width, float height, Goid Owner = GOID_INVALID );

	// For collision syncronization
	void	RCSetCollision			( Goid object_id, Goid contact_object, bool bGlance );
	void	SetCollision			( Goid object_id, Goid contact_object, bool bGlance );


	// --- Loading / Unloading

	void	PauseSimulation			( Goid Object );
	void	ResumeSimulation		( Goid Object );
	void	RemoveSimulation		( Goid Object );



	// --- Update all objects, explosions, and damage volumes

	void	Update					( float SecondsSinceLastCall );



	// --- SimObj Accessor MACRO definintions

#define ACCESSOR_PAIR_DECL( NAME, GET_TYPE, SET_TYPE )		\
	bool Get##NAME( SimID id, GET_TYPE val );				\
	bool Set##NAME( SimID id, const SET_TYPE val );	
#define ACCESSOR_PAIR_IMPL( NAME, GET_TYPE, SET_TYPE )		\
	bool Sim :: Get##NAME( SimID id, GET_TYPE val )			\
	{	SimObjColl::iterator obj;							\
		if( FindObject( m_Objects, id, obj ) ) {			\
			val = (*obj)->m_##NAME;							\
			return true;									\
		}													\
		return false;										\
	}														\
	bool Sim :: Set##NAME( SimID id, const SET_TYPE val )	\
	{	SimObjColl::iterator obj;							\
		if( FindObject( m_Objects, id, obj ) ) {			\
			(*obj)->m_##NAME = val;							\
			return true;									\
		}													\
		return false;										\
	}

#define ACCESSOR_PAIR_BOOL_DECL( NAME )						\
	FEX bool Get##NAME( SimID id );							\
	FEX bool Set##NAME( SimID id, bool state );	
#define ACCESSOR_PAIR_BOOL_IMPL( NAME )						\
	bool Sim :: Get##NAME( SimID id )						\
	{	SimObjColl::iterator obj;							\
		if( FindObject( m_Objects, id, obj ) ) {			\
			return (*obj)->m_b##NAME;						\
		}													\
		return false;										\
	}														\
	bool Sim :: Set##NAME( SimID id, bool state )			\
	{	SimObjColl::iterator obj;							\
		if( FindObject( m_Objects, id, obj ) ) {			\
			(*obj)->m_b##NAME = state;						\
			return true;									\
		}													\
		return false;										\
	}



	// --- SimObj Accessors

	//	bool Get/Set NAME ( SimID id, TYPE value )
	//						NAME				GET_TYPE		SET_TYPE
	ACCESSOR_PAIR_DECL	( OwnerID,				Goid&,			Goid		);
	ACCESSOR_PAIR_DECL	( ObjectID,				Goid&,			Goid		);
	ACCESSOR_PAIR_DECL	( AttachedID,			Goid&,			Goid		);
	ACCESSOR_PAIR_DECL	( AttachLife,			float&,			float		);
	ACCESSOR_PAIR_DECL	( AttachOffset,			vector_3&,		vector_3&	);
	ACCESSOR_PAIR_DECL	( AttachOrientation,	matrix_3x3&,	matrix_3x3&	);
	ACCESSOR_PAIR_DECL	( Position,				SiegePos&,		SiegePos&	);
	ACCESSOR_PAIR_DECL	( Velocity,				vector_3&,		vector_3&	);
	ACCESSOR_PAIR_DECL	( Acceleration,			vector_3&,		vector_3&	);
	ACCESSOR_PAIR_DECL	( Orientation,			matrix_3x3&,	matrix_3x3&	);
	ACCESSOR_PAIR_DECL	( Direction,			vector_3&,		vector_3&	);
	ACCESSOR_PAIR_DECL	( AngularPosition,		vector_3&,		vector_3&	);
	ACCESSOR_PAIR_DECL	( AngularVelocity,		vector_3&,		vector_3&	);
	ACCESSOR_PAIR_DECL	( AngularAcceleration,	vector_3&,		vector_3&	);
	ACCESSOR_PAIR_DECL	( Ignorables,			GoidColl&,		GoidColl&	);
	ACCESSOR_PAIR_DECL	( Gravity,				float&,			float		);
	ACCESSOR_PAIR_DECL	( Mass,					float&,			float		);
	ACCESSOR_PAIR_DECL	( Restitution,			float&,			float		);
	ACCESSOR_PAIR_DECL	( Friction,				float&,			float		);
	ACCESSOR_PAIR_DECL	( DeflectionAngle,		float&,			float		);
	ACCESSOR_PAIR_DECL	( Duration,				float&,			float		);
	ACCESSOR_PAIR_DECL	( Elapsed,				float&,			float		);
	ACCESSOR_PAIR_DECL	( IntendedTarget,		Goid&,			Goid		);

	//	bool Get/Set NAME ( SimID id )
	//							NAME
	ACCESSOR_PAIR_BOOL_DECL	( Attached				);
	ACCESSOR_PAIR_BOOL_DECL	( UseTrajectoryToOrient	);
	ACCESSOR_PAIR_BOOL_DECL	( ExplodeIfHitGo		);
	ACCESSOR_PAIR_BOOL_DECL	( ExplodeIfHitTerrain	);
	ACCESSOR_PAIR_BOOL_DECL	( CollideWithGOs		);
	ACCESSOR_PAIR_BOOL_DECL	( ObjectAtRest			);



	// --- Explosion accessors

	// Sets an explosion to ignore party members - true if found
FEX	bool	SetIgnorePartyMembers	( SimID id );

	// Makes the explosion damage all without using the filter - true if found
FEX bool	SetDamageAll			( SimID id, bool state );



	// --- Debug stuff

#	if !GP_RETAIL
	// Returns a string of information
	gpstring&	GetSimInfo			( void );
FEX	void		SetDebugState		( const bool state );
FEX bool		GetDebugState		( void ) const;
#	endif // !GP_RETAIL




private:

	// --- Update support

	void		UpdateAllObjects			( void );
	void		UpdateExplosions			( void );
	void		UpdateDamageVolumes			( void );
	bool		UpdateObjectStatus			( SimObj & object );
	bool		UpdateObjectMotion			( SimObj & object, float t );
	void		UpdateNodePosition			( SimObj & object, SiegePos & last_position, SiegePos & new_position, vector_3 & new_acceleration, vector_3 & new_velocity, matrix_3x3 & initial_orient, bool bDoesDamage );
	void		UpdateObjectOrientation		( SimObj & object, float t, SiegePos & new_position, vector_3 & new_acceleration, vector_3 & new_velocity, vector_3 & new_direction, matrix_3x3 & new_orientation );

	void		ProcessTerrainCollision		( SimObj & object, float t, SiegePos & contact_position_terrain, SiegePos & contact_normal_terrain, vector_3 & new_angular_acceleration, vector_3 & new_angular_velocity, vector_3 & new_acceleration, vector_3 & new_velocity, SiegePos & new_position );
	void		ProcessGoCollision			( SimObj & object, float t, SiegePos & contact_position_go, SiegePos & contact_normal_go, vector_3 & new_angular_acceleration, vector_3 & new_angular_velocity, vector_3 & new_acceleration, vector_3 & new_velocity, Goid contact_id );
	void		SyncronizeCollisions		( SimObj & object );
	void		PositionGameObject			( SimObj & object );

	void		InsertNewObjects			( void );
	void		DestroyExpiredObjects		( void );
	void		ProcessPauseRequests		( void );
	void		ProcessResumeRequests		( void );


	// --- Creation

	bool		CreateObject				( SimObj **pSimObj, Goid object, Goid parent, const SiegePos &start_pos, DWORD seed );
	SimID		AddObject					( SimObj *pSimObj );


	// --- Collision related functions

	void		DamageObjectFromCollision	( SimObj & object, const vector_3 &velocity );
	void		SendNearMiss				( SimObj & object );
	void		AttachToGameObject			( SiegePos contact_point, Goid contact_id, Goid object_id, Goid parent_id, Goid owner_id );
	int			GetClosestBoneIndex			( Go *pTarget, Go*pParent, nema::HAspect &pParent_aspect, float parent_render_scale, const SiegePos &contact_point,
											  const vector_3 test_position, bool &bSingleBone, vector_3 &bone_position, Quat &bone_rotation );


	// --- Utility

	bool		FindObject					( SimObjColl &coll, SimID id, SimObjColl::iterator &object, bool bBoth = true, SimObjColl **pCollToEraseFrom = NULL );
	bool		FindObject					( Goid object_id, SimObj **object );
	bool		FindExplosion				( SimExpColl &coll, SimID id, SimExpColl::iterator &explosion, bool bBoth = true );
	bool		FindDamageVolume			( SimDmgColl &coll, SimID id, SimDmgColl::iterator &dvolume, bool bBoth = true );
	void		GetNodeInfo					( const siege::database_guid &node, matrix_3x3 &orient, vector_3 &center );
	vector_3	GetHalfDiag					( Goid id );



public:

	// --- Helper class declarations

	struct Explosion
	{
		Explosion( Goid Object, float magnitude, float delay, const vector_3 &initial_velocity )
			: m_Object( Object )
			, m_magnitude( magnitude )
			, m_delay( delay )
			, m_initial_velocity( initial_velocity )
			, m_spawned( false ) {}

		bool		Xfer( FuBi::PersistContext &persist );

		Goid		m_Object;
		float		m_magnitude;
		float		m_delay;
		vector_3	m_initial_velocity;
		bool		m_spawned;
	};


private:

	// --- Variable Stoarge

	// Active Storage
	SimObjColl					m_Objects;					// Simulated objects
	SimExpColl					m_Explosions;				// Simulated explosions
	SimDmgColl					m_SimDmgs;					// Active damage volumes
															
	// Inactive storage										
	SimObjColl					m_InactiveObjects;			// Unloaded SimObjs
	SimExpColl					m_InactiveExplosions;		// Unloaded SimExps
	SimDmgColl					m_InactiveSimDmgs;			// Unloaded Damage volumes
															
															
	// Deletion/Insertion queues							
	SimIDColl					m_ObjDeletionQueue;			// Queue of objects that need to be removed
	SimIDColl					m_ExpDeletionQueue;			// Queue of explosions that need to be removed
	SimIDColl					m_DmgDeletionQueue;			// Queue of damage volumes that need to be removed
															
	SimObjColl					m_ObjInsertionQueue;		// Queue of objects that need to be inserted
	SimExpColl					m_ExpInsertionQueue;		// Queue of explosions that need to be inserted
	SimDmgColl					m_SimDmgInsQueue;			// Queue of damage volumes that need to be inserted
															
	// Loading/Unload queues								
	SimIDColl					m_ObjUnloadQueue;			// Queue of objects that need to be unloaded
	SimIDColl					m_ObjLoadQueue;				// Queue of objects that need to be loaded
															
	SimIDColl					m_ExpUnloadQueue;			// Queue of explosions that need to be unloaded
	SimIDColl					m_ExpLoadQueue;				// Queue of explosions that need to be loaded
															
	SimIDColl					m_SimDmgUnloadQueue;		// Queue of damage volumes that need to be unloaded
	SimIDColl					m_SimDmgLoadQueue;			// Queue of damage volumes that need to be loaded

	float						m_Party_Attach_Life;		// Once attached this is the timeout value before being deleted
	float						m_Enemy_Attach_Life;		// 

	float						m_Elapsed;
	float						m_dt;

	float						m_SniperRange;				// Simulation range minimum

	SimID						m_NextID;
	SimID						m_LastID;

	gpstring					m_sSimInfo;
	int							m_collision_count;
	int							m_at_rest_count;

	RandomGeneratorNR			m_RNG;
	
#	if !GP_RETAIL
	bool						m_bDebugState;
#	endif // !GP_RETAIL

	FUBI_SINGLETON_CLASS( Sim, "Dungeon Siege physics simulator." );
	SET_NO_COPYING( Sim );
};

#define gSim Sim::GetSingleton()

#endif // _SIM_H_
