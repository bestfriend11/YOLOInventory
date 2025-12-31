//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoPhysics.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoPhysics.h"

#include "Flamethrower_FuBi_Support.h"
#include "FuBiTraitsImpl.h"
#include "GoData.h"
#include "GoDb.h"
#include "GoSupport.h"
#include "Rules.h"
#include "WorldFx.h"

DECLARE_GO_COMPONENT( GoPhysics, "physics" );

//////////////////////////////////////////////////////////////////////////////
// class GoPhysics::DamageParticulate implementation

bool GoPhysics::DamageParticulate :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_CloneSource", m_CloneSource );
	persist.Xfer( "m_Count",       m_Count       );

	return ( true );
}

FUBI_DECLARE_SELF_TRAITS( GoPhysics::DamageParticulate );

//////////////////////////////////////////////////////////////////////////////
// class GoPhysics implementation

GoPhysics :: GoPhysics( Go* parent )
	: Inherited( parent )
{
	m_FireResistance			= 0;
	m_FireCharredCloneSource	= GOID_INVALID;
	m_Options					= OPTION_NONE;
	m_SimId						= Sim::ID_INVALID;
	m_FireScriptID				= 0;

	m_bCollisionOccured			= false;
	m_bCollisionGlanced			= false;
	m_CollisionGoid				= GOID_INVALID;

}

GoPhysics :: GoPhysics( const GoPhysics& source, Go* parent )
	: Inherited( source, parent ),
	  m_ParticulateColl( source.m_ParticulateColl )
{
	// copy normal info
	m_FireResistance			= source.m_FireResistance;
	m_FireCharredCloneSource	= source.m_FireCharredCloneSource;
	m_Options					= source.m_Options;

	// fx and sim not started on new object, don't copy
	m_SimId						= Sim::ID_INVALID;
	m_FireScriptID				= 0;

	m_bCollisionOccured			= source.m_bCollisionOccured;
	m_bCollisionGlanced			= source.m_bCollisionGlanced;
	m_CollisionGoid				= source.m_CollisionGoid;

	// lock our clone sources into memory
	ParticulateColl::const_iterator i, ibegin = m_ParticulateColl.begin(), iend = m_ParticulateColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		gGoDb.LockCloneSource( i->m_CloneSource, GetGo() );
	}
	gGoDb.LockCloneSource( m_FireCharredCloneSource, GetGo() );
}

GoPhysics :: ~GoPhysics( void )
{
	// this space intentionally left blank...
}

float GoPhysics :: GetFireBurnThreshold( void ) const
{
	static AutoConstQuery <float> s_Query( "fire_burn_threshold" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoPhysics :: GetFireEffect( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "fire_effect" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoPhysics :: GetFireEffectParams( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "fire_effect_params" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoPhysics :: GetIsOnFire( void ) const
{
	return ( m_FireScriptID != 0 );
}

void GoPhysics :: SetIsOnFire( bool burn, Goid owner )
{
	if ( GetIsOnFire() != burn )
	{
		// Start a fire script
		if ( burn )
		{
			// Prepare the targets
			def_tracker tracker = WorldFx::MakeTracker();
			tracker->AddGoTarget( GetGo()->GetGoid() );
			tracker->AddGoTarget( GetGo()->GetGoid() );

			// Run & store the fire script id
			m_FireScriptID = gWorldFx.RunScript( GetFireEffect(), tracker, GetFireEffectParams(), owner, WE_START_BURNING );
		}
		else
		{
			// Stop burning
			gWorldFx.FinishScript( m_FireScriptID );
			m_FireScriptID = 0;
		}
	}
}

void GoPhysics :: SetSimulationId( SimID simId )
{
	gpassert( simId != Sim::ID_INVALID );

	// stop the old simulation if there is one
	if ( m_SimId != simId )
	{
		StopSimulating();
		m_SimId = simId;
	}
}

void GoPhysics :: StopSimulating( void )
{
	if ( IsSimulating() )
	{
		gSim.RemoveObject( m_SimId );
		m_SimId = Sim::ID_INVALID;
	}
}

const gpstring& GoPhysics :: GetBreakEffect( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "break_effect" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoPhysics :: GetBreakSound( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "break_sound" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoPhysics :: GetExplosionMagnitude( void ) const
{
	static AutoConstQuery <float> s_Query( "explosion_magnitude" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

void GoPhysics :: SetHasExploded( bool set )
{
	if ( set && !TestOptions( OPTION_HAS_EXPLODED ) )
	{
		WorldMessage( WE_EXPLODED, GetGoid() ).Send();
	}
	SetOptions( OPTION_HAS_EXPLODED, set );
}

bool GoPhysics :: GetCanDismember( void ) const
{
	static AutoConstQuery <bool> s_Query( "gib_gore_good" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoPhysics :: GetGibMinDamage( void ) const
{
	static AutoConstQuery <float> s_Query( "gib_min_damage" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoPhysics :: GetGibThreshold( void ) const
{
	static AutoConstQuery <float> s_Query( "gib_threshold" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoPhysics :: GetDamageAll( void ) const
{
	static AutoConstQuery <bool> s_Query( "damage_all" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoPhysics :: GetBreakDependents( void ) const
{
	static AutoConstQuery <bool> s_Query( "break_dependents" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoPhysics :: GetOrientToTrajectory( void ) const
{
	static AutoConstQuery <bool> s_Query( "orient_to_trajectory" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoPhysics :: GetSimDuration( void ) const
{
	static AutoConstQuery <float> s_Query( "sim_duration" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoPhysics :: GetImpactAlertDistance( void ) const
{
	static AutoConstQuery <float> s_Query( "impact_alert_distance" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoPhysics :: GetTossVelocity( void ) const
{
	static AutoConstQuery <float> s_Query( "toss_velocity" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const vector_3& GoPhysics :: GetTossSpin( void ) const
{
	static AutoConstQuery <vector_3> s_Query( "toss_spin" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoPhysics :: GetAngularMagnitude( void ) const
{
	static AutoConstQuery <float> s_Query( "angular_magnitude" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoPhysics :: GetDeflectionAngle( void ) const
{
	static AutoConstQuery <float> s_Query( "deflection_angle" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoPhysics :: GetElasticity( void ) const
{
	static AutoConstQuery <float> s_Query( "elasticity" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoPhysics :: GetFriction( void ) const
{
	static AutoConstQuery <float> s_Query( "friction" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoPhysics :: GetMass( void ) const
{
	static AutoConstQuery <float> s_Query( "mass" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoPhysics :: GetVelocity( void ) const
{
	static AutoConstQuery <float> s_Query( "velocity" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoPhysics :: GetRandomizeVelocity( void ) const
{
	static AutoConstQuery <bool> s_Query( "randomize_velocity" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const vector_3& GoPhysics :: GetAngularVelocity( void ) const
{
	static AutoConstQuery <vector_3> s_Query( "angular_velocity" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoPhysics :: GetGravity( void ) const
{
	static AutoConstQuery <float> s_Query( "gravity" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

GoComponent* GoPhysics :: Clone( Go* newParent )
{
	return ( new GoPhysics( *this, newParent ) );
}

bool GoPhysics :: Xfer( FuBi::PersistContext& persist )
{

// Xfer ordinary things.

	// options
	persist.XferOption( "explode_if_hit_go",      *this, OPTION_EXPLODE_IF_HIT_GO      );
	persist.XferOption( "explode_if_hit_terrain", *this, OPTION_EXPLODE_IF_HIT_TERRAIN );
	persist.XferOption( "explode_when_expired",   *this, OPTION_EXPLODE_IF_EXPIRED     );
	persist.XferOption( "explode_when_killed",    *this, OPTION_EXPLODE_WHEN_KILLED    );
	persist.XferOption( "bone_box_collision",     *this, OPTION_BONE_BOX_COLLISION     );


	// local options
	if ( persist.IsFullXfer() )
	{
		// $ note that is_breakable cache is updated below

		persist.XferOption( "has_exploded", *this, OPTION_HAS_EXPLODED );
	}

	// the rest
	persist.Xfer( "fire_resistance", m_FireResistance );

// Xfer internals.

	// only build particulate list if this is initial creation
	if ( persist.IsRestoring() && !GetGo()->HasValidGoid() )
	{
		gpassert( m_ParticulateColl.empty() );

		// particulates list is internal
		FastFuelHandle particulates = GetData()->GetInternalField( "break_particulate" );

		if ( particulates )
		{
			FastFuelFindHandle fh( particulates );
			if( fh.FindFirstKeyAndValue() )
			{
				gpstring key, value;
				Goid frag;
				int count;

				while ( fh.GetNextKeyAndValue( key, value ) )
				{
					frag = gGoDb.FindCloneSource( key );
					if ( (frag != GOID_INVALID) && ::FromString( value, count ) && (count > 0) )
					{
						m_ParticulateColl.push_back( DamageParticulate( frag, count ) );
					}
				}
			}
		}
	}

	// special for charred go
	if ( persist.IsFullXfer() )
	{
		persist.Xfer      ( "m_FireCharredCloneSource", m_FireCharredCloneSource );
		persist.XferVector( "m_ParticulateColl",        m_ParticulateColl        );
	}
	else
	{
		if ( persist.IsRestoring() )
		{
			gpstring templateName;
			persist.Xfer( "fire_charred_template", templateName );
			if ( !templateName.empty() )
			{
				m_FireCharredCloneSource = gGoDb.FindCloneSource( templateName );
			}
			else
			{
				m_FireCharredCloneSource = GOID_INVALID;
			}
		}
	}

// Update cache.

	if ( persist.IsFullXfer() )
	{
		persist.Xfer( "m_bCollisionOccured",		m_bCollisionOccured		);
		persist.Xfer( "m_bCollisionGlanced",		m_bCollisionGlanced		);
		persist.Xfer( "m_CollisionGoid",			m_CollisionGoid			);
		persist.Xfer( "m_SimId",					m_SimId					);
		persist.Xfer( "m_FireScriptID",				m_FireScriptID			);
	}

	if ( persist.IsRestoring() )
	{
		// update break flag
		if ( !GetBreakEffect().empty() || !GetBreakSound().empty() || !m_ParticulateColl.empty() )
		{
			SetOptions( OPTION_IS_BREAKABLE );
		}
	}

	return ( true );
}

void GoPhysics :: Shutdown( void )
{
	// need to clean out any simulation running on us right now. otherwise if
	// we're the client, and a pile of deletions and creations comes in on one
	// frame, and one of the goids happens to get reused, and its old self was
	// simulating, then the new one gets moved! very bad. fixes #12148.
	StopSimulating();
}

void GoPhysics :: HandleMessage( const WorldMessage& msg )
{
	gpassert( !msg.IsCC() );

	switch( msg.GetEvent() )
	{
		case ( WE_START_SIMULATING ):
		{
			// clear any old simulation
			StopSimulating();

			// we don't want to simulate stuff all the time so wait for a world
			// message - for now this really only works well for objects that
			// need to be dropped and not respond from getting hit like a
			// breakable object.
			gSim.StartSimulation( GetGoid(), m_SimId );
		}
		break;

		case ( WE_START_BURNING ):
		{
			SetIsOnFire( true );
		}
		break;

		case ( WE_STOP_BURNING ):
		{
			SetIsOnFire( false );
		}
		break;

		case ( WE_COLLIDED ):
		{
			if( gServer.IsLocal() )
			{
				gRules.OnProjectileCollision( GetGo()->GetGoid(), MakeGoid(msg.GetData1()) );
			}
		}
		break;

		case ( WE_PICKED_UP ):
		{
			// clear any old simulation
			StopSimulating();
		}
		break;
	}
}

DWORD GoPhysics :: GoPhysicsToNet( GoPhysics* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoPhysics* GoPhysics :: NetToGoPhysics( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetPhysics() : NULL );
}

//////////////////////////////////////////////////////////////////////////////
