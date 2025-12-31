//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoPhysics.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the physics component for Go's.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOPHYSICS_H
#define __GOPHYSICS_H

//////////////////////////////////////////////////////////////////////////////

#include "Go.h"
#include "Sim.h"

//////////////////////////////////////////////////////////////////////////////
// class GoPhysics declaration

class GoPhysics : public GoComponent
{
public:
	SET_INHERITED( GoPhysics, GoComponent );

// Types.

	struct DamageParticulate
	{
		Goid m_CloneSource;
		int  m_Count;

		DamageParticulate( Goid cloneSource = GOID_INVALID, int count = 0 )
			: m_CloneSource( cloneSource ), m_Count( count )  {  }

		bool Xfer( FuBi::PersistContext& persist );
	};

	typedef std::vector <DamageParticulate> ParticulateColl;

	enum eOptions
	{
		// $ note: options tagged as (local) are not part of the public schema

		OPTION_NONE						=      0,

		OPTION_EXPLODE_IF_HIT_GO		= 1 << 1,		// true if this object explodes if it collides with a go
		OPTION_EXPLODE_IF_HIT_TERRAIN	= 1 << 2,		// true if this object explodes if it collides with terrain
		OPTION_EXPLODE_IF_EXPIRED		= 1 << 3,		// true if this object should explode when it times out
		OPTION_EXPLODE_WHEN_KILLED		= 1 << 4,		// true if this object explodes on impact
		OPTION_IS_BREAKABLE				= 1 << 5,		// (local) true if this object can be broken - implicit
		OPTION_HAS_EXPLODED				= 1 << 6,		// (local) flag for marking something as exploded during a single frame evaluation
		OPTION_PASS_THROUGH				= 1 << 7,		// true if collision with other gos causes damage but doesn't influence trajectory
		OPTION_BONE_BOX_COLLISION		= 1 << 8,		// true if should check smaller bone bboxes for more accurate collision like with the dragon
	};

// Setup.

	// ctor/dtor
	GoPhysics( Go* parent );
	GoPhysics( const GoPhysics& source, Go* parent );
	virtual ~GoPhysics( void );

// Interface.

	// simple options
	void SetOptions   ( eOptions options, bool set = true )		{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void ClearOptions ( eOptions options )						{  SetOptions( options, false );  }
	void ToggleOptions( eOptions options )						{  m_Options = (eOptions)(m_Options ^ options);  }
	bool TestOptions  ( eOptions options ) const				{  return ( (m_Options & options) != 0 );  }

// Fire.

	// fire query
	bool            GetIsFireImmune		( void ) const			{  return ( m_FireResistance >= 1.0f );  }
	float           GetFireResistance	( void ) const			{  return ( m_FireResistance );  }
	float			GetFireBurnThreshold( void ) const;			// amount of fire damage that can be taken without catching on fire
	const gpstring& GetFireEffect		( void ) const;			// SiegeFx script to call that makes sense for this object when on fire
	const gpstring& GetFireEffectParams ( void ) const;

	// fire control
	bool GetIsOnFire( void ) const;
	void SetIsOnFire( bool burn = true, Goid owner = GOID_INVALID );

// Breakables.

	// breakables
	bool                   GetIsBreakable       ( void ) const	{  return ( TestOptions( OPTION_IS_BREAKABLE ) );  }
	const gpstring&        GetBreakEffect       ( void ) const;
	const gpstring&        GetBreakSound        ( void ) const;
	const ParticulateColl& GetDamageParticulates( void ) const	{  return ( m_ParticulateColl    );  }

	// explosion
	bool            GetExplodeIfHitGo	  ( void ) const 					{  return ( TestOptions( OPTION_EXPLODE_IF_HIT_GO ) );  }
	bool            GetExplodeIfHitTerrain( void ) const 					{  return ( TestOptions( OPTION_EXPLODE_IF_HIT_TERRAIN ) );  }
	bool            GetExplodeWhenKilled  ( void ) const					{  return ( TestOptions( OPTION_EXPLODE_WHEN_KILLED ) ); }
	bool            GetExplodeOnExpiration( void ) const					{  return ( TestOptions( OPTION_EXPLODE_IF_EXPIRED ) ); }
FEX float           GetExplosionMagnitude ( void ) const;
	bool            GetHasExploded		  ( void ) const					{  return ( TestOptions( OPTION_HAS_EXPLODED ) );  }
	void            SetHasExploded		  ( bool set = true );
	bool            GetPassThrough		  ( void ) const					{  return ( TestOptions( OPTION_PASS_THROUGH ) ); }
	bool            GetBoneBoxCollision	  ( void ) const					{  return ( TestOptions( OPTION_BONE_BOX_COLLISION ) ); }
	bool            GetCanDismember		  ( void ) const;
	float           GetGibMinDamage		  ( void ) const;
	float           GetGibThreshold		  ( void ) const;
	float           GetAngularMagnitude	  ( void ) const;
	bool            GetDamageAll		  ( void ) const;
	bool            GetBreakDependents	  ( void ) const;
	bool            GetOrientToTrajectory ( void ) const;
	float           GetSimDuration		  ( void ) const;
	float           GetImpactAlertDistance( void ) const;
	float           GetTossVelocity		  ( void ) const;
	const vector_3& GetTossSpin           ( void ) const;

// Sim.

	// physics initial constants query
	float           GetDeflectionAngle  ( void ) const;			// radian angle to deflect objects by - 0 is perpendicular (always sticks), 1 is parallel
	float           GetElasticity       ( void ) const;			// coefficient of elasticity (restitution)
	float           GetFriction         ( void ) const;			// coefficient of friction
	float           GetMass             ( void ) const;			// mass of object in kg
FEX	float           GetVelocity         ( void ) const;			// ammo initial velocity
FEX	bool            GetRandomizeVelocity( void ) const;			// flag for randomizing velocity settings as +/- values
FEX	const vector_3& GetAngularVelocity  ( void ) const;			// ammo initial angular velocity
FEX	float           GetGravity          ( void ) const;			// ammo gravity

	// physics simulation
	bool IsSimulating   ( void ) const							{  return ( m_SimId != Sim::ID_INVALID );  }
	void SetSimulationId( SimID simId );
	void StopSimulating ( void );

	// Collision syncronization
	bool GetCollisionOccured( void ) const						{ return m_bCollisionOccured; }
	void SetCollisionOccured( bool b )							{ m_bCollisionOccured = b; }

	bool GetCollisionGlanced( void ) const						{ return m_bCollisionGlanced; }
	void SetCollisionGlanced( bool b )							{ m_bCollisionGlanced = b; }
	
	Goid GetCollisionGoid( void ) const							{ return m_CollisionGoid; }
	void SetCollisionGoid( Goid goid )							{ m_CollisionGoid = goid; }
	
	
	// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newParent );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );

	// event handlers
	virtual void Shutdown     ( void );
	virtual void HandleMessage( const WorldMessage& msg );

private:

// Spec.

	// fire
	float m_FireResistance;						// percent of calculated damage that gets through to this object 0-1
	Goid  m_FireCharredCloneSource;				// go to clone for my charred form - note that new cloned go receives my scid

	// breakables
	ParticulateColl m_ParticulateColl;			// particulates to spawn when broken

// State.

	eOptions m_Options;							// spec/state options for this object
	SimID    m_SimId;							// current physics simulation running on this
	SFxSID   m_FireScriptID;					// SiegeFx script id of fire script that is burning if there is one

	bool m_bCollisionOccured;					// Sim communication for server to client collision syncronization
	bool m_bCollisionGlanced;
	Goid m_CollisionGoid;


	static DWORD      GoPhysicsToNet( GoPhysics* x );
	static GoPhysics* NetToGoPhysics( DWORD d, FuBiCookie* cookie );

	SET_NO_COPYING( GoPhysics );
	FUBI_RPC_CLASS( GoPhysics, GoPhysicsToNet, NetToGoPhysics, "" );
};

MAKE_ENUM_BIT_OPERATORS( GoPhysics::eOptions );

//////////////////////////////////////////////////////////////////////////////

#endif  // __GOPHYSICS_H

//////////////////////////////////////////////////////////////////////////////
