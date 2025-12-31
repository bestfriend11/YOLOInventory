#pragma once
/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: SimObj.h

	Author(s)	: Rick Saenz

	Purpose		: Defines a simulation object for Sim.

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#ifndef _SIMOBJ_H_
#define _SIMOBJ_H_


class	Sim;


#include "Sim.h"


struct SimObj
{
	enum	{	ENEMY_ATTACH_LIFE = 45 };
	enum	{	PARTY_ATTACH_LIFE = 5 };

	SimObj( void ) {}

	SimObj(	Goid owner_goguid,							// Owner of this object GOID_INVALID means no owner
			Goid object_goguid,							// The game object to modify to show this simulation

			float Gravity,								// Gravitational acceleration constant
			float mass,									// Mass in kilograms
			float restitution,							// Coefficient of restitution
			float friction,								// Coefficient of friction
			float deflection_angle,						// Radian tolerance before sticking ( 0 is perpendicular, 1 is parallel )

			const SiegePos		&position,				// Initial position
			const matrix_3x3	&orientation,			// Initial orientation
			const vector_3		&velocity,				// Initial center of mass velocity
			const vector_3		&acceleration,			// Initial center of mass acceleration

			bool bUseTrajectoryToOrient = false,		// Should this object orient itself to the direction it is traveling?
			bool bExplodeIfHitGo		= false,		// Object will explode if it collides with a go
			bool bExplodeIfHitTerrain	= false,		// Object will explode if it collides with a go
			bool bExplodeOnExpiration	= false,		// Object will explode when it expires if it doesn't explode some way else
			bool bCollideWithGOs		= false,		// Should this object collide with other game objects? false = terrain only
			float timeout = Sim::DEFAULT_SIM_TIMEOUT )	// Amount of time before destroying this simulation and game object

		:	m_OwnerID					( owner_goguid )
		,	m_ObjectID					( object_goguid )

		,	m_Gravity					( Gravity )
		,	m_Mass						( mass )
		,	m_Restitution				( restitution )
		,	m_Friction					( friction )
		,	m_DeflectionAngle			( deflection_angle )

		,	m_Position					( position )
		,	m_Velocity					( velocity )
		,	m_Acceleration				( acceleration )
		,	m_InitialOrientation		( orientation )

		,	m_bUseTrajectoryToOrient	( bUseTrajectoryToOrient )
		,	m_bExplodeIfHitGo			( bExplodeIfHitGo )
		,	m_bExplodeIfHitTerrain		( bExplodeIfHitTerrain )
		,	m_bExplodeOnExpiration		( bExplodeOnExpiration )
		,	m_bCollideWithGOs			( bCollideWithGOs )

		,	m_bPassThrough				( false )
		,	m_bShouldDestroy			( true  )
		,	m_bObjectAtRest				( false )
		,	m_bInCollision				( false )
		,	m_bInDeletion				( false	)
		,	m_bCanDoDamage				( true	)
		,	m_AttachLife				( ENEMY_ATTACH_LIFE )
		,	m_AttachedID				( GOID_INVALID )
		,	m_bAttached					( false )

		,	m_IntendedTarget			( GOID_INVALID )

		,	m_Duration					( timeout )
		,	m_Elapsed					( 0 )
		,	m_bIsDropSim				( false )
		,	m_bPlayDropSound			( true )
		,	m_Attacker					( GOID_INVALID )
	{}

	bool	Xfer( FuBi::PersistContext &persist );



	SimID				m_ID;						// Simulation ID for this object
	Goid				m_OwnerID;					// What game object owns this object
	Goid				m_AttachedID;				// If attached, game object id of
	Goid				m_ObjectID;					// associated game object to draw
	Goid				m_IntendedTarget;			// Intended target that this object was shot at to hit

	Goid				m_Attacker;					// who the attacker for this object is

	// Attachment
	bool				m_bAttached;				// True if attached 
	float				m_AttachLife;				// Time in seconds before ending sim
	vector_3			m_AttachOffset;				// Positional
	matrix_3x3			m_AttachOrientation;		// Positional

	// Positional
	SiegePos			m_Position;					// Current position
	vector_3			m_Velocity;					// Current velocity
	vector_3			m_Acceleration;				// Current acceleration
	matrix_3x3			m_InitialOrientation;		// Initial orientation
	matrix_3x3			m_Orientation;				// Current orientation
	vector_3			m_Direction;				// Current direction
	vector_3			m_AngularPosition;			// Angular position
	vector_3			m_AngularVelocity;			// momentum
	vector_3			m_AngularAcceleration;		// torque

	// Attributes
	bool				m_bUseTrajectoryToOrient;	// Always point the direction traveling
	bool				m_bCollideWithGOs;			// Should this object ignore other GOs
	bool				m_bExplodeIfHitGo;			// Should this object explode if it collides with a go?
	bool				m_bExplodeIfHitTerrain;		// Should this object explode if it collides with the terrain?
	bool				m_bExplodeOnExpiration;		// Should this object expldoe when it expires?
	bool				m_bPassThrough;				// Pass through other go's - damage still occurs
	bool				m_bShouldDestroy;			// Should m_ObjectID be destroyed
	bool				m_bObjectAtRest;			// Has this object stopped moving?
	bool				m_bInCollision;				// Is this object currently in collision?
	bool				m_bInDeletion;				// Is this object getting deleted?
	bool				m_bCanDoDamage;				// Is set to true until a collision occurs
	GoidColl			m_Ignorables;				// Game objects we don't collide with
	float				m_Gravity;					// Gravitational acceleration constant
	float				m_Mass;						// Mass in kg
	float				m_Restitution;				// Coefficient of restitution
	float				m_Friction;					// Coefficient of friction
	float				m_DeflectionAngle;			// Angle of deflection
	float				m_Duration;					// Max duration of this simulation 
	float				m_Elapsed;					// Elapsed time

	bool				m_bIsDropSim;				// True if this object is being simulated to show that it is being removed from inventory
	Quat				m_DropQuat;					// Desired orientation once it has been dropped
	SiegePos			m_DropPos;					// The position where this object must be at after animation starts from being dropped
	bool				m_bPlayDropSound;			// Should a drop sound be played for this item?
};



struct SimExp
{	
	SimExp( void )
	{
		m_ID			=	Sim::ID_INVALID;
		m_Elapsed		=	0;
		m_Radius		=	0;
		m_MaxRadius		=	0;
		m_Magnitude		=	0;
		m_Damage		=	0;
		m_Duration		=	0;

		m_bDamageAll	=	false;

		m_Owner			=	GOID_INVALID;
		m_UnloadSignaler=	GOID_INVALID;
	}

	SimExp( const SiegePos &position, float radius, float magnitude, float damage, Goid owner, Goid owner_weapon )
		: m_Position( position )
		, m_MaxRadius( radius )
		, m_Magnitude( magnitude )
		, m_Damage( damage )
		, m_Owner( owner )
		, m_OwnerWeapon( owner_weapon )
		, m_bDamageAll( false )
		, m_Duration( radius / magnitude )
		, m_Elapsed( 0 )
		, m_Radius( 0 )
	{}

	bool		Xfer( FuBi::PersistContext &persist );


	SimID		m_ID;

	float		m_Elapsed;
	float		m_Duration;

	float		m_Radius;

	// const values
	SiegePos	m_Position;
	float		m_MaxRadius;
	float		m_Magnitude;
	float		m_Damage;

	bool		m_bDamageAll;

	GoidColl	m_Ignorables;

	Goid		m_Owner;			// Owner of the explosion - must be an actor or GOID_INVALID
	Goid		m_OwnerWeapon;		// Weapon the owner used to cause the explosion, spell or melee weapon

	Goid		m_UnloadSignaler;
};



struct SimDmg
{
	// Default constructor
	SimDmg( void )
	{
		m_ID				= Sim::ID_INVALID;
		m_source			= GOID_INVALID;
		m_target			= GOID_INVALID;

		m_length			= 0;
		m_start_radius		= 0;
		m_end_radius		= 0;

		m_damage			= 0;

		m_duration			= 0;
		m_elapsed			= 0;

		m_Owner				= GOID_INVALID;
		m_OwnerWeapon		= GOID_INVALID;

		m_bStatic			= false;
	}

	// Static targets
	SimDmg( const SiegePos &start_pos, const SiegePos &end_pos, float start_radius, float end_radius, float damage, float duration,
			Goid Owner, Goid OwnerWeapon )
		: m_start_pos	( start_pos )
		, m_end_pos		( end_pos )
		, m_start_radius( start_radius )
		, m_end_radius	( end_radius )
		, m_Owner		( Owner )
		, m_OwnerWeapon	( OwnerWeapon )
	{
		m_damage		= damage;
		m_duration		= duration;
		m_elapsed		= 0;
		
		m_bStatic		= true;
	}


	// Moving targets
	SimDmg( Goid source, Goid target, float length, float start_radius, float end_radius, float damage, float duration, Goid Owner, Goid OwnerWeapon )
		: m_source		( source )
		, m_target		( target )
		, m_Owner		( Owner )
		, m_OwnerWeapon	( OwnerWeapon )
	{
		m_length		= length;
		m_start_radius	= start_radius;
		m_end_radius	= end_radius;
		m_damage		= damage;
		m_duration		= duration;
		m_elapsed		= 0;

		m_bStatic		= false;
	}

	bool		Xfer( FuBi::PersistContext &persist );


	// Access
	bool		GetStartPosition	( SiegePos & pos );
	bool		GetEndPosition		( SiegePos & pos );

	// Storage

	SimID		m_ID;

	Goid		m_source;
	Goid		m_target;
	SiegePos	m_start_pos;
	SiegePos	m_end_pos;

	float		m_length;

	float		m_start_radius;
	float		m_end_radius;

	float		m_damage;

	float		m_duration;
	float		m_elapsed;

	bool		m_bStatic;

	GoidColl	m_Ignorables;

	Goid		m_Owner;
	Goid		m_OwnerWeapon;

	Goid		m_UnloadSignaler;
};


#endif // _SIMOBJ_H_
