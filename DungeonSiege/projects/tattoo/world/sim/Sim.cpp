//
//	Sim.cpp
//

#include "precomp_world.h"
#include "AppModule.h"
#include "Aiquery.h"
#include "BoneTranslator.h"
#include "Gps_manager.h"
#include "ContentDb.h"
#include "Matrix_3x3.h"
#include "Nema_AspectMgr.h"
#include "Go.h"
#include "GoAspect.h"
#include "GoAttack.h"
#include "GoBody.h"
#include "GoCore.h"
#include "GoMind.h"
#include "GoShmo.h"
#include "GoDb.h"
#include "GoInventory.h"
#include "GoPhysics.h"
#include "Oriented_Bounding_Box_3.h"
#include "PhysCalc.h"
#include "Player.h"
#include "Rules.h"
#include "Rules.h"
#include "Server.h"
#include "Siege_Database_Guid.h"
#include "Siege_Engine.h"
#include "Siege_Node.h"
#include "Siege_logical_node.h"
#include "Siege_Pos.h"
#include "siege_options.h"
#include "Sim.h"
#include "SimObj.h"
#include "Space_3.h"
#include "Vector_3.h"
#include "World.h"
#include "WorldFx.h"
#include "WorldMessage.h"
#include "WorldOptions.h"
#include "WorldSound.h"
#include "gosupport.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "FuBiTraitsimpl.h"


FUBI_REPLACE_NAME( "Sim", Physics );


Sim :: Sim( void )
{
	m_Elapsed			= 0;
	m_dt				= 0;
	m_NextID			= ID_INVALID;
	m_LastID			= ID_INVALID;
	m_collision_count	= 0;
	m_at_rest_count		= 0;
	m_SniperRange		= 0.0f;

#	if !GP_RETAIL
	m_bDebugState		=	false;
#	endif // !GP_RETAIL

	// Read in the timeout values defined in formulas.gas

	FastFuelHandle		hPhysicsBlock( "world:global:formula:physics_constants" );
	if( !hPhysicsBlock.IsValid() )
	{
		gpfatal("Cannot find world:global:physics_constants in gas tree!");
	}

	m_Party_Attach_Life	 = SimObj::PARTY_ATTACH_LIFE;
	m_Enemy_Attach_Life	 = SimObj::ENEMY_ATTACH_LIFE;

	hPhysicsBlock.Get( "party_attach_life",	m_Party_Attach_Life );
	hPhysicsBlock.Get( "enemy_attach_life",	m_Enemy_Attach_Life );


	m_RNG.SetSeed( RandomDword() );

	for( DWORD i = (m_RNG.RandomDword() & 0xFF); i > 0; --i )
	{
		m_RNG.RandomDword();
	}
}


Sim :: ~Sim( void )
{
	Shutdown();
}

void
Sim :: Shutdown( void )
{
	// delete
	stdx::for_each( m_Objects,				stdx::delete_by_ptr() );
	stdx::for_each( m_Explosions,			stdx::delete_by_ptr() );
	stdx::for_each( m_SimDmgs,				stdx::delete_by_ptr() );

	stdx::for_each( m_InactiveObjects,		stdx::delete_by_ptr() );
	stdx::for_each( m_InactiveExplosions,	stdx::delete_by_ptr() );
	stdx::for_each( m_InactiveSimDmgs,		stdx::delete_by_ptr() );

	stdx::for_each( m_ObjInsertionQueue,	stdx::delete_by_ptr() );
	stdx::for_each( m_ExpInsertionQueue,	stdx::delete_by_ptr() );
	stdx::for_each( m_SimDmgInsQueue,		stdx::delete_by_ptr() );

	// clear
	m_Objects			.clear();
	m_Explosions		.clear();
	m_SimDmgs			.clear();

	m_InactiveObjects	.clear();
	m_InactiveExplosions.clear();
	m_InactiveSimDmgs	.clear();

	m_ObjDeletionQueue	.clear();
	m_ExpDeletionQueue	.clear();
	m_DmgDeletionQueue	.clear();

	m_ObjInsertionQueue	.clear();
	m_ExpInsertionQueue	.clear();
	m_SimDmgInsQueue	.clear();

	m_ObjUnloadQueue	.clear();
	m_ObjLoadQueue		.clear();

	m_ExpUnloadQueue	.clear();
	m_ExpLoadQueue		.clear();
			
	m_SimDmgUnloadQueue	.clear();
	m_SimDmgLoadQueue	.clear();
}


bool
Sim :: IsEmpty( void ) const
{
	return (   m_Objects				.empty()
			&& m_Explosions				.empty()
			&& m_SimDmgs				.empty()
			&& m_InactiveObjects		.empty()
			&& m_InactiveExplosions		.empty()
			&& m_InactiveSimDmgs		.empty()
			&& m_ObjDeletionQueue		.empty()
			&& m_ExpDeletionQueue		.empty()
			&& m_DmgDeletionQueue		.empty()
			&& m_ObjInsertionQueue		.empty()
			&& m_ExpInsertionQueue		.empty()
			&& m_SimDmgInsQueue			.empty()
			&& m_ObjUnloadQueue			.empty()
			&& m_ObjLoadQueue			.empty()
			&& m_ExpUnloadQueue			.empty()
			&& m_ExpLoadQueue			.empty()
			&& m_SimDmgUnloadQueue		.empty()
			&& m_SimDmgLoadQueue		.empty()
			);
}


bool
Sim :: Xfer( FuBi::PersistContext &persist )
{
	persist.XferList	( "m_Objects",					m_Objects				);
	persist.XferList	( "m_Explosions",				m_Explosions			);
	persist.XferList	( "m_SimDmgs",					m_SimDmgs				);

	persist.XferList	( "m_InactiveObjects",			m_InactiveObjects		);
	persist.XferList	( "m_InactiveExplosions",		m_InactiveExplosions	);
	persist.XferList	( "m_InactiveSimDmgs",			m_InactiveSimDmgs		);

	persist.XferList	( "m_ObjDeletionQueue",			m_ObjDeletionQueue		);
	persist.XferList	( "m_ExpDeletionQueue",			m_ExpDeletionQueue		);
	persist.XferList	( "m_DmgDeletionQueue",			m_DmgDeletionQueue		);

	persist.XferList	( "m_ObjInsertionQueue",		m_ObjInsertionQueue		);
	persist.XferList	( "m_ExpInsertionQueue",		m_ExpInsertionQueue		);
	persist.XferList    ( "m_SimDmgInsQueue",			m_SimDmgInsQueue        );

	persist.XferList	( "m_ObjUnloadQueue",			m_ObjUnloadQueue		);
	persist.XferList	( "m_ObjLoadQueue",				m_ObjLoadQueue			);

	persist.XferList	( "m_ExpUnloadQueue",			m_ExpUnloadQueue		);
	persist.XferList	( "m_ExpLoadQueue",				m_ExpLoadQueue			);

	persist.XferList	( "m_SimDmgUnloadQueue",		m_SimDmgUnloadQueue		);
	persist.XferList	( "m_SimDmgLoadQueue",			m_SimDmgLoadQueue		);

	persist.Xfer		( "m_Party_Attach_Life",		m_Party_Attach_Life		);
	persist.Xfer		( "m_Enemy_Attach_Life",		m_Enemy_Attach_Life		);

	persist.Xfer		( "m_Elapsed",					m_Elapsed				);
	persist.Xfer		( "m_dt",						m_dt					);

	persist.Xfer		( "m_NextID",					m_NextID				);
	persist.Xfer		( "m_LastID",					m_LastID				);

	persist.Xfer		( "m_sSimInfo",					m_sSimInfo				);
	persist.Xfer		( "m_collision_count",			m_collision_count		);
	persist.Xfer		( "m_at_rest_count",			m_at_rest_count			);
	persist.Xfer		( "m_SniperRange",				m_SniperRange			);

	return true;
}


void
Sim :: SpawnLocalPhysicsObject( const gpstring &sName, const SiegePos &initial_position,
								const vector_3 &linear_acceleration, const vector_3 &linear_velocity,
								const vector_3 &angular_acceleration, const vector_3 &angular_velocity,
								const GoidColl &ignore_list )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	GoCloneReq cloneReq( sName );
	cloneReq.m_Parent = GOID_INVALID;
	cloneReq.SetStartingPos( initial_position );

	GoHandle hNewGo( gGoDb.CloneLocalGo( cloneReq ) );

	gpassert( !hNewGo->HasActor() );

	SimObj *pNewObject = NULL;

	if( hNewGo && !hNewGo->HasActor() && hNewGo->HasPhysics() && CreateObject( &pNewObject, hNewGo->GetGoid(), GOID_INVALID, initial_position, m_RNG.RandomDword() ) )
	{
		nema::HAspect hasp = hNewGo->GetAspect()->GetAspectHandle();
		if( hasp->IsPurged( 2 ) )
		{
			hasp->Reconstitute( 1, true );
		}
		if( !hasp->LockPrimaryBone() )
		{
			gperrorf(("Sim :: SpawnLocalPhysicsObject - could not LockPrimaryBone for [%s] after reconstituting.\n", hNewGo->GetTemplateName()) );
		}

		pNewObject->m_AngularAcceleration	= angular_acceleration;
		pNewObject->m_Acceleration			= linear_acceleration;

		pNewObject->m_AngularVelocity		= angular_velocity;
		pNewObject->m_Velocity				= linear_velocity;

		pNewObject->m_Ignorables			= ignore_list;
		pNewObject->m_bPassThrough			= hNewGo->GetPhysics()->GetPassThrough();

		AddObject( pNewObject );
	}
}


bool
Sim :: SFireProjectile( SimID &pid, Go* ammo, Go* weapon, Goid intended_target, const GoidColl &ignorables, const SiegePos &firing_position,
					  const SiegePos &target_position, float velocity, float x_error, float y_error )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	CHECK_SERVER_ONLY;

	gpassert( ammo != NULL );
	gpassert( weapon != NULL );
	gpassert( !ammo->HasActor() );

	vector_3 velocity_vector;

	if( !ammo->HasActor() && ammo->HasPhysics() )
	{
		const float gravity = ammo->GetPhysics()->GetGravity();
		const vector_3 position( gSiegeEngine.GetDifferenceVector( firing_position, target_position ) );

		if( !!m_SniperRange && weapon->HasParent() && weapon->GetParent()->IsAnyHumanPartyMember() )
		{
			const float delta_velocity = velocity * 0.15f;
			while( !Physcalc::IsInRange( position, velocity, gravity )  )
			{
				velocity += delta_velocity;
			}
			x_error = y_error = 0;
		}

		if( Physcalc::CalculateFiringVector( position, velocity, gravity, x_error, y_error, velocity_vector ) )
		{
			weapon->GetAttack()->RCLaunchAmmo( ammo, intended_target, firing_position, velocity_vector, RandomDword() );

			// Set what game objects to ignore on server only - no collisions occur with GOs on client
			SetIgnorables( m_LastID,ignorables );
			SetVelocity( m_LastID,	velocity_vector );

			pid = m_LastID;

			return true;
		}
	}
	return false;
}


void
Sim :: FireProjectile( Goid ammo_id, Goid weapon_id, const SiegePos &firing_position, const vector_3 &velocity_vector, DWORD seed )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	SimObj *pNewObject = NULL;
	GoHandle hAmmo( ammo_id );
	gpassert( !hAmmo->HasActor() );

	if( !hAmmo->HasActor() && hAmmo->HasPhysics() && CreateObject( &pNewObject, ammo_id, weapon_id, firing_position, seed ) )
	{
		GoHandle( ammo_id )->GetPlacement()->ForceSetPosition( firing_position );

		pNewObject->m_Velocity		= velocity_vector;
		pNewObject->m_bPassThrough	= hAmmo->GetPhysics()->GetPassThrough();

		m_LastID = AddObject( pNewObject );
	}
}


bool
Sim :: SExplodeGo( Goid Object, float magnitude, const vector_3 &initial_velocity )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	CHECK_SERVER_ONLY;

	GoHandle hObject( Object );

	if( !(hObject && hObject->HasPhysics()) )
	{
		return false;
	}

	GoPhysics	*pPhysicsObject = hObject->GetPhysics();

	if( pPhysicsObject->GetIsBreakable() && !pPhysicsObject->GetHasExploded() )
	{
		// Get rid of any attached objects so they don't fall to the ground
		SimObjColl::iterator iObject = m_Objects.begin(), end = m_Objects.end();
		for(; iObject != end; ++iObject )
		{
			if( (*iObject)->m_AttachedID == Object )
			{
				RemoveObject( (*iObject)->m_ID );
			}
		}

		// Explode the object
		RCExplodeGo( Object, magnitude, initial_velocity, m_RNG.RandomDword() );

		// If there are any objects touching this one make them start to simulate
		TriggerDependents( Object, ID_INVALID );

		return true;
	}
	return false;
}


void
Sim :: RCExplodeGo( Goid Object, float magnitude, const vector_3 &initial_velocity, DWORD seed )
{
	// route this function through the go so we can take advantage of the
	// automatic rebroadcasting.

	GoHandle hObject( Object );

	if( hObject && hObject->HasPhysics() )
	{
		hObject->RCExplodeGo( magnitude, initial_velocity, seed );
	}
}


void
Sim :: ExplodeGo( Goid Object, float magnitude, const vector_3 &initial_velocity, DWORD seed )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	GoHandle hObject( Object );
	gpassert( hObject && hObject->HasPhysics() );

	if( !hObject->HasPhysics() )
	{
		return;
	}

	// Sync random
	m_RNG.SetSeed( seed );

	GoPhysics *pPhysicsObject = hObject->GetPhysics();

	// setup
	const bool bGoIsGory	= !pPhysicsObject->GetCanDismember();
	const bool bGoreAllowed	= gWorldOptions.GetAllowDismemberment();

	bool bRemoveAspect = false;

	if( !bGoreAllowed && bGoIsGory )
	{
		const bool bGoHasDieAnim= (hObject->HasMind() && hObject->GetMind()->UnderstandsJob( JAT_DIE ));

		if( !bGoHasDieAnim )
		{
			// just fade the go out since it isn't allowed to explode and doesn't have a die anim
			GoFader::FadeAspect( Object );
		}
		return;
	}
	else
	{
		bRemoveAspect = true;
	}

	// Prevent explosion feedback
	pPhysicsObject->SetHasExploded( true );

	// Must be in memory to get bounding volume (#10434)
	hObject->PrepareToDrawNow();

	// Find out where in the volume of the object that is getting exploded where it makes sense to position
	// the particulates by using the bounding volume
	vector_3	explosion_center( DoNotInitialize );
	vector_3	half_diag		( DoNotInitialize );
	matrix_3x3	orient			( DoNotInitialize );

	hObject->GetAspect()->GetNodeSpaceOrientedBoundingVolume( orient, explosion_center, half_diag );
	SiegePos explosion_position( explosion_center, hObject->GetPlacement()->GetPosition().node );

	// halve the half_diag for our max offset
	float max_gib_dist = fabsf( half_diag.y ) * 0.90f;

	// Play break sound effect if defined
	gpstring sBreakSound = pPhysicsObject->GetBreakSound();
	if( !sBreakSound.empty() )
	{
		gWorldSound.PlaySample( sBreakSound, explosion_position );
	}

	// Start the break effect script if defined
	gpstring sBreakEffect = pPhysicsObject->GetBreakEffect();
	if( !sBreakEffect.empty() )
	{
		def_tracker tracker = WorldFx::MakeTracker();
		tracker->AddStaticTarget( explosion_position, vector_3::UP, 1.0f );
		tracker->AddStaticTarget( explosion_position, vector_3::UP, 1.0f );
		gWorldFx.RunScript( sBreakEffect, tracker, "", GOID_INVALID, WE_EXPLODED );
	}

	// Make sure it is ok to dismember this object
	if( gWorldOptions.GetAllowDismemberment() || ( !gWorldOptions.GetAllowDismemberment() && pPhysicsObject->GetCanDismember() ) )
	{
		// First handle the spawing of the damage particulates if there are any defined for the game object
		const GoPhysics::ParticulateColl& ParticleColl = pPhysicsObject->GetDamageParticulates();

		// Optionally superchunk!
		for ( int i = gWorldOptions.GetGibMultiplier() ; i > 0 ; --i )
		{
			float spawn_rate = gWorldFx.GetDetailLevel();

			// when spawn is greater the 1 then we will spawn a frag starting at 1 to ensure that we spawn at least 1
			float spawn = 1.0f;

			// Iterate through the list of particulats and spawn them in the appropriate position within the box
			GoPhysics::ParticulateColl::const_iterator iParticle = ParticleColl.begin(), iend = ParticleColl.end();
			for(; iParticle != iend; ++iParticle )
			{
				for( int i = (*iParticle).m_Count; i != 0; --i )
				{
					spawn += spawn_rate;
					if( spawn > 1.0f )
					{
						spawn -= 1.0f;

						// Initial position is based on the center of the exploding object
						SiegePos initial_position( explosion_position );

						// Offset initial position with some random displacement inside objects bounding volume
						vector_3 center_offset(	m_RNG.Random( -max_gib_dist, max_gib_dist ),
												m_RNG.Random( -max_gib_dist, max_gib_dist ),
												m_RNG.Random( -max_gib_dist, max_gib_dist ) );

						// if we happen to get a zero length vector, then offset the value a bit.
						if( fabs(center_offset.Length()) < 0.05f )
						{
							center_offset.y = 0.05f;
						}

						// make sure that our offset is non-zero
						gpassert( center_offset.Length() != 0 );

						if( !gWorldFx.AddPositions( initial_position, center_offset, false, NULL ) )
						{
							continue;
						}

						// Create a velocity vector based on center offset direction with the specified magnitude
						vector_3 velocity_vector( initial_velocity + ( magnitude * Normalize(center_offset)) );

						// give it random starting orientation
						Quat quat;
						quat.RotateX( m_RNG.Random( 0.0f, PI * 2 ) );
						quat.RotateY( m_RNG.Random( 0.0f, PI * 2 ) );
						quat.RotateZ( m_RNG.Random( 0.0f, PI * 2 ) );

						// Break particulates, frags, gibs, giblets, whatever are created locally and destroyed locally
						GoCloneReq cloneReq( iParticle->m_CloneSource );
						cloneReq.SetStartingPos( initial_position );
						cloneReq.SetStartingOrient( quat );


						GoHandle hNewGo( gGoDb.CloneLocalGo( cloneReq ) );

						if( hNewGo && hNewGo->HasPhysics() )
						{
							// Setup the ignore list to ignore collisions with itself
							GoidColl ignorables;

							SimObj *pNew_object = NULL;

							if( hNewGo->HasPhysics() && CreateObject( &pNew_object, hNewGo->GetGoid(), GOID_INVALID, initial_position, m_RNG.RandomDword() ) )
							{
								pNew_object->m_Velocity		= velocity_vector;
								pNew_object->m_Ignorables	= ignorables;
								pNew_object->m_Orientation	= quat.BuildMatrix();
								pNew_object->m_Duration	   += m_RNG.Random( -pNew_object->m_Duration * 0.2f, pNew_object->m_Duration * 0.2f );

								const float am = hNewGo->GetPhysics()->GetAngularMagnitude();
								const vector_3 ang_vel( m_RNG.Random( -am, am ), m_RNG.Random( -am, am ), m_RNG.Random( -am, am ) );
								pNew_object->m_AngularVelocity = ang_vel;

								pNew_object->m_bPassThrough	= hNewGo->GetPhysics()->GetPassThrough();

								// As soon as the object is added it starts simulating
								AddObject( pNew_object );
							}
						}
					}
				}
			}
		}
	}


	// have to defer changing visibilty until after we use the positional information
	if( bRemoveAspect )
	{
		// just make the go invisible here on the the client machine since we know it's not to be seen
		hObject->GetAspect()->SetIsVisible( false );

		// make sure not to delete the go in multiplayer since the clients are free to change dismemberment settings on the fly
		if( gServer.IsLocal() && (!::IsMultiPlayer() || !bGoIsGory) )
		{
			float radius = 1.0f;
			if( hObject && hObject->HasAttack() )
			{
				radius = hObject->GetAttack()->GetAreaDamageRadius();
			}

			// don't allow for the owner of an explosion to last for longer than 10 seconds.
			const float duration = min( 10.0f, radius / ( magnitude + FLOAT_TOLERANCE ) );

			WorldMessage ( WE_REQ_DELETE, GOID_INVALID, Object, MakeInt(GOID_INVALID) ).SendDelayed( duration );
		}
	}
}


bool
Sim :: ExplodeGoWithDamage( Goid Object, Goid Owner, Goid OwnerWeapon )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	CHECK_SERVER_ONLY;

	GoHandle hObject( Object );

	GoPhysics *pPhysics = hObject->QueryPhysics();

	if( !(hObject && pPhysics ) || (pPhysics && pPhysics->GetHasExploded()) )
	{
		return false;
	}

	const float explosion_magnitude = pPhysics->GetExplosionMagnitude();
	SExplodeGo( Object, explosion_magnitude, vector_3::UP );

	// If this is an explosive object that was launched from a weapon then use
	// the damage that is defined by the projectile launcher weapon instead of
	// the projectile itself
	Go *pParent = NULL;
	GoAttack *pAttack = NULL;

	if( hObject->GetParent( pParent ) )
	{
		if( pParent->HasAttack() )
		{
			const eAttackClass ac = pParent->GetAttack()->GetAttackClass();
			if( (ac == AC_BOW) || (ac == AC_MINIGUN) || (ac == AC_COMBAT_MAGIC) || (ac == AC_NATURE_MAGIC) )
			{
				pAttack = pParent->GetAttack();
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		if( hObject->HasAttack() )
		{
			pAttack = hObject->GetAttack();
		}
		else
		{
			return false;
		}
	}

	const float explosion_radius = pAttack->GetAreaDamageRadius();
	if( explosion_radius <= 0 )
	{
		return false;
	}

	const float damage = m_RNG.Random( pAttack->GetDamageMin(), pAttack->GetDamageMax() );

	SimID explosion = CreateExplosion( hObject->GetPlacement()->GetPosition(), explosion_radius, explosion_magnitude, damage, Owner, OwnerWeapon );

	if( !pPhysics->GetDamageAll() )
	{
		SetIgnorePartyMembers( explosion );
	}

	return true;
}


SimID
Sim :: CreateExplosion( const SiegePos &position, float radius, float magnitude, float damage, Goid Owner, Goid OwnerWeapon )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	CHECK_SERVER_ONLY;

	gpassert( m_Explosions.size() < 500 );

	SimExp *pNewExplosion = new SimExp( position, radius, magnitude, damage, Owner, OwnerWeapon );
	pNewExplosion->m_ID = ++m_NextID;

	GoCloneReq cloneReq( gContentDb.GetDefaultPointTemplate() );
	cloneReq.SetStartingPos( position );

	GoHandle hNewGo( gGoDb.CloneLocalGo( cloneReq ) );

	if( !hNewGo )
	{
		gperror(( "Sim :: CreateExplosion unable to create communicator go 'point' for explosion\n"));
	}
	else
	{
		pNewExplosion->m_UnloadSignaler = hNewGo->GetGoid();
	}

	m_ExpInsertionQueue.push_back( pNewExplosion );

	return pNewExplosion->m_ID;
}


SimID
Sim :: CreateDamageVolume ( const SiegePos &start_pos, const SiegePos &end_pos, float start_radius, float end_radius,
							float damage, float duration, Goid Owner, Goid OwnerWeapon )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	CHECK_SERVER_ONLY;

	gpassert( m_SimDmgs.size() < 500 );
	gpassert( start_pos.pos.Length() < 500 );
	gpassert( end_pos.pos.Length() < 500 );
	gpassert( start_radius < 500 );
	gpassert( end_radius < 500 );

	if( start_pos.node == siege::UNDEFINED_GUID )
	{
		gperror( "Invalid start position supplied for Sim::CreateDamageVolume" );
		return ID_INVALID;
	}

	if( end_pos.node == siege::UNDEFINED_GUID )
	{
		gperror( "Invalid end position supplied for Sim::CreateDamageVolume" );
		return ID_INVALID;
	}

	SimDmg * pSimDmg = new SimDmg( start_pos, end_pos, start_radius, end_radius, damage, duration, Owner, OwnerWeapon );

	GoCloneReq cloneReq( gContentDb.GetDefaultPointTemplate() );
	cloneReq.SetStartingPos( start_pos );

	GoHandle hNewGo( gGoDb.CloneLocalGo( cloneReq ) );

	pSimDmg->m_ID = ++m_NextID;

	pSimDmg->m_UnloadSignaler = hNewGo->GetGoid();

	m_SimDmgInsQueue.push_back( pSimDmg );

	return pSimDmg->m_ID;
}


SimID
Sim :: CreateDamageVolume ( Goid source, Goid target, float length, float start_radius, float end_radius, float damage, float duration,
							Goid Owner, Goid OwnerWeapon )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	CHECK_SERVER_ONLY;

	gpassert( m_SimDmgs.size() < 500 );
	gpassert( length < 500 );
	gpassert( start_radius < 500 );
	gpassert( end_radius < 500 );

	SimDmg * pSimDmg = new SimDmg( source, target, length, start_radius, end_radius, damage, duration, Owner, OwnerWeapon );

	pSimDmg->m_UnloadSignaler = source;

	pSimDmg->m_ID = ++m_NextID;

	m_SimDmgInsQueue.push_back( pSimDmg );

	return pSimDmg->m_ID;
}


void
Sim :: StartDropAnimation ( Goid item, const SiegePos &dropPos, const Quat &dropQuat )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	GoHandle hItem( item );
	GoPhysics *pPhysics = hItem->QueryPhysics();

	gpassert( !hItem->HasActor() );

	if( hItem && !hItem->HasActor() && pPhysics )
	{
		SimObj *pNewObject = NULL;

		SiegePos spawnPos( dropPos );
		spawnPos.pos.y += 1.0f;

		// Make sure this object doesn't appear on the ground for a sec and cause a terrain collision
		hItem->GetPlacement()->ForceSetPlacement( spawnPos, dropQuat );

		if( CreateObject( &pNewObject, item, GOID_INVALID, spawnPos, RandomDword() ) )
		{
			const float velocity= pPhysics->GetTossVelocity();
			const float gravity	= pPhysics->GetGravity();

			const float t = Physcalc::CalculateTimeToImpact( gSiegeEngine.GetDifferenceVector( spawnPos, dropPos ), velocity, 0.5f*PI, gravity );

			const float temp = (PI2 / (t + FLOAT_TOLERANCE));

			float x_spin = floorf(pPhysics->GetTossSpin().x) * temp;
			float y_spin = floorf(pPhysics->GetTossSpin().y) * temp;
			float z_spin = floorf(pPhysics->GetTossSpin().z) * temp;

			if( RandomFloat() > 0.5f )
			{
				x_spin *= -1.0f;
			}

			if( RandomFloat() > 0.5f )
			{
				y_spin *= -1.0f;
			}

			if( RandomFloat() > 0.5f )
			{
				z_spin *= -1.0f;
			}

			const vector_3 spin_velocity( x_spin, y_spin, z_spin );
			const vector_3 toss_velocity( velocity * vector_3::UP );

			pNewObject->m_Velocity				= toss_velocity;
			pNewObject->m_AngularVelocity		= spin_velocity;

			pNewObject->m_bCollideWithGOs		= false;
			pNewObject->m_bShouldDestroy		= false;
			pNewObject->m_bUseTrajectoryToOrient= false;

			pNewObject->m_InitialOrientation	= dropQuat.BuildMatrix();

			pNewObject->m_DropPos				= dropPos;
			pNewObject->m_DropQuat				= dropQuat;
			pNewObject->m_bIsDropSim			= true;

			m_LastID = AddObject( pNewObject );

			hItem->PlayVoiceSound( "put_down_toss" );

			if ( pPhysics->GetSimDuration() == 0.0f )
			{
				hItem->PlayVoiceSound( "put_down" );
			}
		}
	}
}


bool
Sim :: StartSimulation( Goid object, SimID &pid )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	GoHandle hObject( object );

	gpassert( !hObject->HasActor() );

	SimObj *pNew_object = NULL;

	if( hObject && !hObject->HasActor() && hObject->HasPhysics() && CreateObject( &pNew_object, object, GOID_INVALID, hObject->GetPlacement()->GetPosition(), m_RNG.RandomDword() ) )
	{
		pNew_object->m_Orientation		= hObject->GetPlacement()->GetOrientation().BuildMatrix();

		GoidColl ignorables;

		pNew_object->m_Ignorables		= ignorables;
		pNew_object->m_bShouldDestroy	= false;

		pNew_object->m_bPassThrough		= hObject->GetPhysics()->GetPassThrough();

		pid = AddObject( pNew_object );
		return true;
	}
	return false;
}


void
Sim :: StopAllSimulations( void )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	SimObjColl::iterator iObject = m_Objects.begin();

	for(; iObject != m_Objects.end(); ++iObject )
	{
		RemoveObject( (*iObject)->m_ID );
	}

	SimDmgColl::iterator iSimDmg = m_SimDmgs.begin();
	for(; iSimDmg != m_SimDmgs.end(); ++iSimDmg )
	{
		(*iSimDmg)->m_duration = 0;
	}
}


void
Sim :: RemoveObject( SimID id )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	SimObjColl::iterator iSimObj;

	if( FindObject( m_Objects, id, iSimObj ) )
	{
		if( (*iSimObj)->m_bInDeletion == false )
		{
			(*iSimObj)->m_bInDeletion = true;
			m_ObjDeletionQueue.push_back( id );
		}
	}
}


void
Sim :: TriggerDependents( Goid object, SimID source_object, UINT32 recursion )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	CHECK_SERVER_ONLY;

	GoHandle hObject( object );

	if( hObject && !hObject->HasActor() && hObject->HasPhysics() )
	{
		vector_3	source_c( DoNotInitialize );
		vector_3	source_hd( DoNotInitialize );
		matrix_3x3	source_o( DoNotInitialize );

		hObject->GetAspect()->GetWorldSpaceOrientedBoundingVolume( source_o, source_c, source_hd );
		oriented_bounding_box_3	source_box( source_c, source_hd, source_o );

		// What needs to get written is a replacement for the lame GetOccupantsOfBox but, this will work for now
		GopColl sphere_occupants;
		SiegePos center_pos( hObject->GetPlacement()->GetPosition() );
		hObject->GetAspect()->GetNodeSpaceCenter( center_pos.pos );
		gAIQuery.GetOccupantsInSphere(
				center_pos,
				Length( source_hd ),
				hObject,
				NULL,
				0,
				OF_BREAKABLES | OF_CULL_ACTORS,
				&sphere_occupants,
				false );

		GopColl::iterator iSO = sphere_occupants.begin();

		for( ; iSO != sphere_occupants.end(); ++iSO )
		{
			vector_3	dep_c( DoNotInitialize );
			vector_3	dep_hd( DoNotInitialize );
			matrix_3x3	dep_o( DoNotInitialize );

			// make sure to not add an object that blocks a path
			if( (*iSO)->GetAspect()->GetDoesBlockPath() )
			{
				continue;
			}

			(*iSO)->GetAspect()->GetWorldSpaceOrientedBoundingVolume( dep_o, dep_c, dep_hd );

			dep_hd *= 0.95f; // Use a 5% size collision tolerance

			oriented_bounding_box_3 dep_box( dep_c, dep_hd, dep_o );

			if( source_box.CheckForContact( dep_box ) && (*iSO)->HasPhysics() )
			{
				// Check to see if the source object should break any dependent objects
				if( hObject->GetPhysics()->GetBreakDependents() )
				{
					SExplodeGo( (*iSO)->GetGoid(), (*iSO)->GetPhysics()->GetExplosionMagnitude() );
				}
				else
				{
					SiegePos new_pos( (*iSO)->GetPlacement()->GetPosition() );
					SiegePos old_pos( new_pos );
					gSiegeEngine.AdjustPointToTerrain( new_pos );

					// Cause other objects to simulate only if they are above the ground by 12 inches or more
					if( (old_pos.pos.y - new_pos.pos.y) > 0.3048f )
					{
						SimID dep_object = 0;

						StartSimulation( (*iSO)->GetGoid(), dep_object );

						if( recursion > 0 )
						{
							--recursion;
							TriggerDependents( (*iSO)->GetGoid(), dep_object, recursion );
						}

						// Based on the direction that the source object was moving apply friction
						// damped source velocity to the dependant object
						float dep_friction = 0;
						if( GetFriction( dep_object, dep_friction ) )
						{
							float friction = 0;
							if( hObject->HasPhysics() )
							{
								friction = hObject->GetPhysics()->GetFriction();
							}
							float friciton_damp = dep_friction * friction;
							vector_3 velocity;
							GetVelocity( source_object, velocity );
							velocity *= friciton_damp;
							SetVelocity( dep_object, velocity );
						}
					}
				}
			}
		}
	}
}


SimID
Sim :: GetSimID( Goid object )
{
	SimObj *pSimulation = NULL;

	if( FindObject( object, &pSimulation ) )
	{
		return pSimulation->m_ID;
	}
	return ID_INVALID;
}


bool
Sim :: CheckForTerrainCollision( SiegePos &ContactPoint, SiegePos &ContactNormal, const SiegePos &start_position, const SiegePos &end_position )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	if( !gSiegeEngine.IsNodeValid( start_position.node ) && !gSiegeEngine.IsNodeValid( end_position.node ) )
	{
		return false;
	}

	// check to see if we are really close, if we are, then is a high potential for numerical inaccuaracies using tracelines
	// so if our vectors are really close do an adjust point to terrain and check for collision manually....
	vector_3 diff( DoNotInitialize );
	diff = gSiegeEngine.GetDifferenceVector( start_position, end_position );
	if( diff.IsZero( 0.0001f ) )
	{
		SiegePos start(start_position);
		vector_3 normal;

		gSiegeEngine.AdjustPointToTerrain(start,  siege::LF_IS_FLOOR, 1, &normal);

		// if we are on the terrain, set the contact point to the adjusted point
		if( gSiegeEngine.GetDifferenceVector( start_position, start ).IsZero( 0.0002f ) )
		{
			ContactPoint = start;
			ContactNormal.pos = normal;
			ContactNormal.node = ContactPoint.node;
		}
	}
	else if( gSiegeEngine.HitTestGlobalTerrain( start_position, end_position, ContactPoint, ContactNormal.pos, (siege::LF_IS_WALL | siege::LF_IS_FLOOR) ) )
	{
#		if !GP_RETAIL
		// For Debugging purposes draw a sphere where the explosion is at
		if( m_bDebugState )
		{
			static bool red_dash = true;
			red_dash = (red_dash)?false:true;
			DWORD color = (red_dash)? 0xFFFF0000 : 0xFFFFFF00;

			gWorld.DrawDebugLine( start_position, end_position, color, ContactPoint.node.ToString().c_str(), 10.0f );
		}
#		endif // !GP_RETAIL

		ContactNormal.node	= ContactPoint.node;
		return true;
	}

#	if !GP_RETAIL
	// For Debugging purposes draw a sphere where the explosion is at
	if( m_bDebugState )
	{
		static bool green_dash = true;
		green_dash = (green_dash)?false:true;
		DWORD color = (green_dash)? 0xFF00FF00 : 0xFF0000FF;

		gWorld.DrawDebugLine( start_position, end_position, color, "", 8.0f );
	}
#	endif // !GP_RETAIL

	return false;
}


bool
Sim :: CheckForObjectCollision( SiegePos &ContactPoint, SiegePos &ContactNormal, Goid & ContactGoid, const GoidColl &ignore_list,
				const SiegePos &start_position,	const SiegePos &end_position, float Width, float Height, Goid Owner )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	if( !gSiegeEngine.IsNodeValid( start_position.node ) && !gSiegeEngine.IsNodeValid( end_position.node ) )
	{
		return false;
	}

	// There wasn't a terrain collision, check if there is one with any nearby game objects

	// Calculate RAY_DIR in the start_position's space

	const vector_3 ray_dir = gSiegeEngine.GetDifferenceVector( start_position, end_position );

	gpassertm( ray_dir.Length() < 2000, ("Object Collision Box is too large") );

	if( ray_dir.IsZero() )
	{
		return false;
	}

	const float ray_length	= ray_dir.Length();
	const float half_ray_length = ray_length * 0.5f;

	// Figure out the box surrounding the projectile's volume as it moves from start to end
	const vector_3	projectile_half_diag( Width * 0.5f, Height * 0.5f, half_ray_length );
	const float     projectile_half_diag_length = Length(projectile_half_diag);

	// For collision purposes, put the projectile halfway to its destination
	SiegePos projectile_pos = start_position;
	projectile_pos.pos += ray_dir * 0.5;

		// Create the projectile's obb IN WORLD SPACE
	matrix_3x3	projectile_orient	= MatrixFromDirection( ray_dir );

	siege::SiegeNodeHandle start_handle	= gSiegeEngine.NodeCache().UseObject( start_position.node );
	const siege::SiegeNode& start_node	= start_handle.RequestObject( gSiegeEngine.NodeCache() );
	oriented_bounding_box_3 projectile_box( projectile_pos.WorldPos(), projectile_half_diag, start_node.GetCurrentOrientation() * projectile_orient );

#	if !GP_RETAIL
	if( gSiegeEngine.GetOptions().IsCollisionBoxes() )
	{
		// Using the current camera target node lets us draw our 'worldspace' box in the 'localspace'
		// of the node that currrently has the origin
		SiegePos draw_pos( projectile_box.GetCenter(), gSiegeEngine.NodeWalker().TargetNodeGUID() );
 		gWorld.DrawDebugBox( draw_pos,projectile_box.GetOrientation(),projectile_box.GetHalfDiagonal(), 0xFFFF0000, 0.5f, true );
	}
#	endif // !GP_RETAIL


	gpassertm( projectile_half_diag_length < 495, ("Object Collision Box is too large, this number is from the sanity check in AiQuery") );

	// Get the objects that make up a possible collision set

	// This is set of object within asphere centered on the MIDPOINT of the projectile movement
	// with DIAMETER equal to the distance the projectile wants to move (add on 1.0f to compensate
	// for other objects moving too)

	GoHandle hOwner( Owner );
	GopColl collidables;
	gAIQuery.GetOccupantsInSphere(
			projectile_pos,
			max_t( projectile_half_diag_length, 2.0f ),
			hOwner,
			&ignore_list,
			0,
			OF_CULL_SAME_MEMBERSHIP | OF_ALIVE | OF_COLLIDABLES,
			&collidables,
			false /*no cache*/ );

	if (collidables.empty())
	{
		return false;
	}

	for( GopColl::iterator iCollidable = collidables.begin() ; iCollidable != collidables.end(); ++iCollidable )
	{
		Go * hTarget = *iCollidable;

		// Get the oriented bounding box of this object
		oriented_bounding_box_3 target_box( DoNotInitialize );

		// make sure the nema aspect of the go has been animated at least once
		if( hTarget && !hTarget->HasAspect() )
		{
			continue;
		}

		hTarget->GetAspect()->GetWorldSpaceOrientedBoundingVolume(	target_box.GetOrientation(),
																	target_box.GetCenter(),
																	target_box.GetHalfDiagonal() );
#		if !GP_RETAIL
		if( gSiegeEngine.GetOptions().IsCollisionBoxes() )
		{
			// Using the current camera target node trick for worldspace boxes again...
			SiegePos collidable_pos( target_box.GetCenter(), gSiegeEngine.NodeWalker().TargetNodeGUID() );
			gWorld.DrawDebugBox( collidable_pos, target_box.GetOrientation(), target_box.GetHalfDiagonal(), 0xFF8F009F, 0.5f, true );
		}
#		endif // !GP_RETAIL


		// coarse collision box check
		if( projectile_box.CheckForContact( target_box ) )
		{
#		if !GP_RETAIL
			if( m_bDebugState && hOwner && hTarget )
			{
				gpstring sShooter( (hOwner) ? hOwner->GetTemplateName() : "INVALID_GO" );
				gpstring sTarget ( (hTarget) ? hTarget->GetTemplateName() : "INVALID_GO" );

				gpgenericf(("%s shot %s\n     TARGET: center = [%2.3f,%2.3f,%2.3f] half_diag = [%2.3f,%2.3f,%2.3f]\n PROJECTILE: center = [%2.3f,%2.3f,%2.3f] half_diag = [%2.3f,%2.3f,%2.3f]\n",
							sShooter.c_str(),
							sTarget.c_str(),

							target_box.GetCenter().x,
							target_box.GetCenter().y,
							target_box.GetCenter().z,

							target_box.GetHalfDiagonal().x,
							target_box.GetHalfDiagonal().y,
							target_box.GetHalfDiagonal().z,

							projectile_box.GetCenter().x,
							projectile_box.GetCenter().y,
							projectile_box.GetCenter().z,

							projectile_box.GetHalfDiagonal().x,
							projectile_box.GetHalfDiagonal().y,
							projectile_box.GetHalfDiagonal().z
						));
			}
#		endif

			bool bContact = false;
			bool bFineCollision = false;

			// fine collision box check
			if( hTarget->HasPhysics() && hTarget->GetPhysics()->GetBoneBoxCollision() )
			{
				bFineCollision = true;
				DWORD num_bones = hTarget->GetAspect()->GetAspectHandle()->GetNumBones();
				for( DWORD bone_index = 0; bone_index != num_bones; ++bone_index )
				{
					hTarget->GetAspect()->GetWorldSpaceOrientedBoneBoundingVolume( target_box.GetOrientation(), target_box.GetCenter(), target_box.GetHalfDiagonal(), bone_index );

#					if !GP_RETAIL
					if( gSiegeEngine.GetOptions().IsCollisionBoxes() )
					{
						// Using the current camera target node trick for worldspace boxes again...
						SiegePos collidable_pos( target_box.GetCenter(), gSiegeEngine.NodeWalker().TargetNodeGUID() );
						gWorld.DrawDebugBox( collidable_pos, target_box.GetOrientation(), target_box.GetHalfDiagonal(), 0xFFFFFFFF, 0.5f, true );
					}
#					endif // !GP_RETAIL

					// coarse collision box check
					if( projectile_box.CheckForContact( target_box ) )
					{
#						if !GP_RETAIL
						if( gSiegeEngine.GetOptions().IsCollisionBoxes() )
						{
							// Using the current camera target node trick for worldspace boxes again...
							SiegePos collidable_pos( target_box.GetCenter(), gSiegeEngine.NodeWalker().TargetNodeGUID() );
							gWorld.DrawDebugBox( collidable_pos, target_box.GetOrientation(), target_box.GetHalfDiagonal(), 0xFF8F0000, 0.5f, true );
						}
#						endif // !GP_RETAIL
						bContact = true;
						break;
					}
				}
			}
			else
			{
				bContact = true;
			}

			if( bContact )
			{
				ContactGoid	= (*iCollidable)->GetGoid();

				gpassert( ContactGoid != GOID_INVALID );

				// Calculate the normal of intersection with a sphere
				// Need the normal in the space of the contact point
				SiegePos ContactCenter( target_box.GetCenter(), gSiegeEngine.NodeWalker().TargetNodeGUID() );
				vector_3 cnorm( gSiegeEngine.GetDifferenceVector(ContactCenter,start_position) );

				if( bFineCollision )
				{
					ContactNormal = SiegePos( Normalize(cnorm), ContactCenter.node );

					if( !target_box.RayIntersectsOrientedBox( start_node.NodeToWorldSpace( start_position.pos ),
																  start_node.GetCurrentOrientation() * ray_dir,
																  &ContactPoint.pos ) )
					{
						ContactPoint = end_position;
					}
					else
					{
						ContactPoint.FromWorldPos( ContactPoint.pos, end_position.node );
					}
				}
				else
				{
					if( hTarget->HasBody() && hTarget->GetBody()->GetBoneTranslator().GetPosition( hTarget, BoneTranslator::KILL_BONE, ContactPoint ) )
					{
						ContactNormal	= SiegePos( Normalize(cnorm), ContactCenter.node );
					}
					else
					{
						// Find a good approximation of a contact point
						if( !target_box.RayIntersectsOrientedBox( start_node.NodeToWorldSpace( start_position.pos ),
																	  start_node.GetCurrentOrientation() * ray_dir,
																	  &ContactPoint.pos ) )
						{
							ContactPoint = hTarget->GetPlacement()->GetPosition();
						}
						else
						{
							ContactPoint.FromWorldPos( ContactPoint.pos, hTarget->GetPlacement()->GetPosition().node );
						}

						ContactNormal	= SiegePos( Normalize( -ray_dir ), ContactPoint.node );
					}
				}
				return true;
			}
		}
	}
	return false;
}


void
Sim :: RCSetCollision( Goid object_id, Goid contact_object, bool bGlance )
{
	// route this function through the go so we can take advantage of the
	// automatic rebroadcasting.

	GoHandle hObject( object_id );

	if( hObject && hObject->HasPhysics() )
	{
		hObject->RCSetCollision( contact_object, bGlance );
	}
}


void
Sim :: SetCollision( Goid object_id, Goid contact_object, bool bGlance )
{
	GoHandle hObject( object_id );

	if( hObject && hObject->HasPhysics() )
	{
		GoPhysics *pPhysics = hObject->GetPhysics();

		pPhysics->SetCollisionOccured( true );
		pPhysics->SetCollisionGlanced( bGlance );
		pPhysics->SetCollisionGoid( contact_object );
	}
}


void
Sim :: PauseSimulation( Goid Object )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	// Find any objects or explosions that are owned by the Object and make a request to unload them

	// Unload objects
	// Check the active updated objects
	SimObjColl::iterator iSimObj = m_Objects.begin(), iSimObj_end = m_Objects.end();
	for(; iSimObj != iSimObj_end; ++iSimObj )
	{
		if( (*iSimObj)->m_ObjectID == Object )
		{
			m_ObjUnloadQueue.push_back( (*iSimObj)->m_ID );
		}
	}
	// Check insertion queue
	iSimObj = m_ObjInsertionQueue.begin(), iSimObj_end = m_ObjInsertionQueue.end();
	for(; iSimObj != iSimObj_end; ++iSimObj )
	{
		if( (*iSimObj)->m_ObjectID == Object )
		{
			m_ObjUnloadQueue.push_back( (*iSimObj)->m_ID );
		}
	}


	// Unload explosions
	// Check active updated explosions
	SimExpColl::iterator iSimExp = m_Explosions.begin(), iSimExp_end = m_Explosions.end();
	for(; iSimExp != iSimExp_end; ++iSimExp )
	{
		if( (*iSimExp)->m_UnloadSignaler == Object )
		{
			m_ExpUnloadQueue.push_back( (*iSimExp)->m_ID );
		}
	}
	// Check insertion queue
	iSimExp = m_ExpInsertionQueue.begin(), iSimExp_end = m_ExpInsertionQueue.end();
	for(; iSimExp != iSimExp_end; ++iSimExp )
	{
		if( (*iSimExp)->m_UnloadSignaler == Object )
		{
			m_ExpUnloadQueue.push_back( (*iSimExp)->m_ID );
		}
	}


	// Unload damage volumes
	// Check active damage volumes
	SimDmgColl::iterator iSimDmg = m_SimDmgs.begin(), iSimDmgEnd = m_SimDmgs.end();
	for(; iSimDmg != iSimDmgEnd; ++iSimDmg )
	{
		if( (*iSimDmg)->m_UnloadSignaler == Object )
		{
			m_SimDmgUnloadQueue.push_back( (*iSimDmg)->m_ID );
		}
	}
	// Check to see if there are any that are waiting to be inserted and need to be stopped
	iSimDmg = m_SimDmgInsQueue.begin(), iSimDmgEnd = m_SimDmgInsQueue.end();
	for(; iSimDmg != iSimDmgEnd; ++iSimDmg )
	{
		if( (*iSimDmg)->m_UnloadSignaler == Object )
		{
			m_SimDmgUnloadQueue.push_back( (*iSimDmg)->m_ID );
		}
	}
}


void
Sim :: ResumeSimulation( Goid Object )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	// Look up any objects in the unload queues that belong to Object and request to load them

	// Request load of SimObjs
	SimObjColl::iterator iSimObj = m_InactiveObjects.begin(), iSimObj_end = m_InactiveObjects.end();
	for(; iSimObj != iSimObj_end; ++iSimObj )
	{
		if( (*iSimObj)->m_ObjectID == Object )
		{
			m_ObjLoadQueue.push_back( (*iSimObj)->m_ID );
		}
	}

	// Request load of SimExps
	SimExpColl::iterator iSimExp = m_InactiveExplosions.begin(), iSimExp_end = m_InactiveExplosions.end();
	for(; iSimExp != iSimExp_end; ++iSimExp )
	{
		if( (*iSimExp)->m_Owner == Object )
		{
			m_ExpLoadQueue.push_back( (*iSimExp)->m_ID );
		}
	}

	// Request load of DamageVols
	SimDmgColl::iterator iSimDmg = m_InactiveSimDmgs.begin(), iSimDmgEnd = m_InactiveSimDmgs.end();
	for(; iSimDmg != iSimDmgEnd; ++iSimDmg )
	{
		if( (*iSimDmg)->m_Owner == Object )
		{
			m_SimDmgLoadQueue.push_back( (*iSimDmg)->m_ID );
		}
	}
}


void
Sim :: RemoveSimulation( Goid Object )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	// Request deletion of SimObjs
	SimObjColl::iterator iSimObj = m_InactiveObjects.begin(), iSimObj_end = m_InactiveObjects.end();
	for(; iSimObj != iSimObj_end; ++iSimObj )
	{
		if( (*iSimObj)->m_ObjectID == Object )
		{
			m_ObjDeletionQueue.push_back( (*iSimObj)->m_ID );
		}
	}

	// Request deletion of SimExps
	SimExpColl::iterator iSimExp = m_InactiveExplosions.begin(), iSimExp_end = m_InactiveExplosions.end();
	for(; iSimExp != iSimExp_end; ++iSimExp )
	{
		if( (*iSimExp)->m_Owner == Object )
		{
			m_ExpDeletionQueue.push_back( (*iSimExp)->m_ID );
		}
	}

	// Request deletion of SimDmgs
	SimDmgColl::iterator iSimDmg = m_InactiveSimDmgs.begin(), iSimDmgEnd = m_InactiveSimDmgs.end();
	for(; iSimDmg != iSimDmgEnd; ++iSimDmg )
	{
		if( (*iSimDmg)->m_Owner == Object )
		{
			m_DmgDeletionQueue.push_back( (*iSimDmg)->m_ID );
		}
	}
}


void
Sim :: Update( float SecondsSinceLastCall )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	// early out if paused
	if( SecondsSinceLastCall == 0 )
	{
		return;
	}

	// set time
	m_dt		= SecondsSinceLastCall;
	m_Elapsed	+= m_dt;

	// delete/remove expired or requested objects
	DestroyExpiredObjects();

	// move and collide all simulation objects
	UpdateAllObjects();

	// explosions and damage volumes are not seen by the client so only the server does it
	if( gServer.IsLocal() )
	{
		UpdateExplosions();
		UpdateDamageVolumes();
	}

	// insert new objects
	InsertNewObjects();

	// unload
	ProcessPauseRequests();

	// load
	ProcessResumeRequests();
}





// Reference style accessors
ACCESSOR_PAIR_IMPL( OwnerID,			Goid&,			Goid		);
ACCESSOR_PAIR_IMPL( AttachedID,			Goid&,			Goid		);
ACCESSOR_PAIR_IMPL( ObjectID,			Goid&,			Goid		);
ACCESSOR_PAIR_IMPL( AttachLife,			float&,			float		);
ACCESSOR_PAIR_IMPL( AttachOffset,		vector_3&,		vector_3&	);
ACCESSOR_PAIR_IMPL( AttachOrientation,	matrix_3x3&,	matrix_3x3&	);
ACCESSOR_PAIR_IMPL( Position,			SiegePos&,		SiegePos&	);
ACCESSOR_PAIR_IMPL( Velocity,			vector_3&,		vector_3&	);
ACCESSOR_PAIR_IMPL( Acceleration,		vector_3&,		vector_3&	);
ACCESSOR_PAIR_IMPL( Orientation,		matrix_3x3&,	matrix_3x3&	);
ACCESSOR_PAIR_IMPL( Direction,			vector_3&,		vector_3&	);
ACCESSOR_PAIR_IMPL( AngularPosition,	vector_3&,		vector_3&	);
ACCESSOR_PAIR_IMPL( AngularVelocity,	vector_3&,		vector_3&	);
ACCESSOR_PAIR_IMPL( AngularAcceleration,vector_3&,		vector_3&	);
ACCESSOR_PAIR_IMPL( Ignorables,			GoidColl&,		GoidColl&	);
ACCESSOR_PAIR_IMPL( Gravity,			float&,			float		);
ACCESSOR_PAIR_IMPL( Mass,				float&,			float		);
ACCESSOR_PAIR_IMPL( Restitution,		float&,			float		);
ACCESSOR_PAIR_IMPL( Friction,			float&,			float		);
ACCESSOR_PAIR_IMPL( DeflectionAngle,	float&,			float		);
ACCESSOR_PAIR_IMPL( Duration,			float&,			float		);
ACCESSOR_PAIR_IMPL( Elapsed,			float&,			float		);
ACCESSOR_PAIR_IMPL( IntendedTarget,		Goid&,			Goid		);

// Boolean accessors
ACCESSOR_PAIR_BOOL_IMPL( Attached				);
ACCESSOR_PAIR_BOOL_IMPL( UseTrajectoryToOrient	);
ACCESSOR_PAIR_BOOL_IMPL( ExplodeIfHitGo			);
ACCESSOR_PAIR_BOOL_IMPL( ExplodeIfHitTerrain	);
ACCESSOR_PAIR_BOOL_IMPL( CollideWithGOs			);
ACCESSOR_PAIR_BOOL_IMPL( ObjectAtRest			);



bool
Sim :: SetIgnorePartyMembers( SimID id )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	// Find the explosion
	SimExpColl::iterator i = m_ExpInsertionQueue.begin(), i_end = m_ExpInsertionQueue.end();
	for(; i != i_end; ++i )
	{
		if( (*i)->m_ID == id )
		{
			break;
		}
	}

	if( i == i_end )
	{
		for(i = m_Explosions.begin(), i_end = m_Explosions.end(); i != i_end; ++i )
		{
			if( (*i)->m_ID == id )
			{
				break;
			}
		}
		if( i == i_end )
		{
			return false;
		}
	}

	// Get the explosion owners party and add to it's list of ignorables
	GoHandle hOwner( (*i)->m_Owner );
	Go *pLeaf = NULL;

	if( hOwner && hOwner->GetParent( pLeaf ) )
	{
		bool bHasParty = true;
		while( !pLeaf->HasParty() )
		{
			if( !pLeaf->GetParent( pLeaf ) )
			{
				bHasParty = false;
				break;
			}
		}

		if( bHasParty )
		{
			GopColl party = pLeaf->GetChildren();
			party.Translate( (*i)->m_Ignorables, GOID_INVALID );
		}
	}

	return true;
}


bool
Sim :: SetDamageAll( SimID id, bool state )
{
	GPSTATS_GROUP( GROUP_PHYSICS );

	// Find the explosion
	SimExpColl::iterator i = m_ExpInsertionQueue.begin(), i_end = m_ExpInsertionQueue.end();
	for(; i != i_end; ++i )
	{
		if( (*i)->m_ID == id )
		{
			break;
		}
	}

	if( i == i_end )
	{
		for(i = m_Explosions.begin(), i_end = m_Explosions.end(); i != i_end; ++i )
		{
			if( (*i)->m_ID == id )
			{
				break;
			}
		}
		if( i == i_end )
		{
			return false;
		}
	}

	(*i)->m_bDamageAll = state;

	return true;
}


#if !GP_RETAIL

gpstring&
Sim :: GetSimInfo( void )
{
	m_sSimInfo.clear();
	m_sSimInfo.appendf( "Simulation objects  = %d\n", m_Objects.size() );
	m_sSimInfo.appendf( "Objects in collison = %d\n", m_collision_count );
	m_sSimInfo.appendf( "Objects at rest     = %d\n", m_at_rest_count );

	return m_sSimInfo;
}


void
Sim :: SetDebugState( const bool state )
{
	m_bDebugState = state;

	gWorldFx.SetShowVolumes( state );
}


bool
Sim :: GetDebugState( void ) const
{
	return m_bDebugState;
}

#endif	// !GP_RETAIL



//////////
/////
///
//  - Begin Privately Declared Section -
///
/////
//////////

void
Sim :: UpdateAllObjects( void )
{
	// clear out debug counters
	m_collision_count = 0;
	m_at_rest_count = 0;

	// update each simulation object
	SimObjColl::iterator iObject = m_Objects.begin(), obj_end = m_Objects.end();
	for( ; iObject != obj_end; ++iObject )
	{
		SimObj &object = **iObject;

		if( UpdateObjectStatus( object ) )
		{
			UpdateObjectMotion( object, m_dt );
		}
	}
}


void
Sim :: UpdateExplosions( void )
{
	// Insert any explosions waiting to be processed
	if( !m_ExpInsertionQueue.empty() )
	{
		SimExpColl::iterator iSimExp = m_ExpInsertionQueue.begin(), iExpEnd = m_ExpInsertionQueue.end();
		for(; iSimExp != iExpEnd; ++iSimExp )
		{
			m_Explosions.push_back( *iSimExp );
		}
		m_ExpInsertionQueue.clear();
	}

	// Start processing actual explosions
	SimExpColl::iterator iExp = m_Explosions.begin();

	while( iExp != m_Explosions.end() )
	{
		(*iExp)->m_Elapsed	+= m_dt;
		(*iExp)->m_Radius	+= m_dt * (*iExp)->m_Magnitude;

		const float radius		= (*iExp)->m_Radius;
		const float max_radius	= (*iExp)->m_MaxRadius;

		if( radius > max_radius )
		{
			GoHandle go( (*iExp)->m_UnloadSignaler );
			if ( go && same_no_case( go->GetTemplateName(), "point" ) )
			{
				gGoDb.StartExpiration( go );
			}

			delete *iExp;
			iExp = m_Explosions.erase( iExp );
			continue;
		}

		const SiegePos exp_position = (*iExp)->m_Position;

#		if !GP_RETAIL
		// For Debugging purposes draw a sphere where the explosion is at
		if( m_bDebugState )
		{
			gWorld.DrawDebugSphere( exp_position, radius, 0xFFFF0000, "exp vol cur");
			gWorld.DrawDebugSphere( exp_position, max_radius, 0xFFFFFFFF, "exp vol lim");
		}
#		endif // !GP_RETAIL

		// Then handle damaging/exploding any neighboring game objects if the object actually exploded
		GopColl occupants;

		if( (*iExp)->m_bDamageAll )
		{
			gAIQuery.GetOccupantsInSphere(
					exp_position,
					radius,
					NULL,
					NULL,
					0,
					OF_BREAKABLES | OF_ACTORS | OF_MATCH_ANY,	// actors or breakables are ok
					&occupants );
		}
		else
		{
			gAIQuery.GetOccupantsInSphere(
					exp_position,
					radius,
					GoHandle( (*iExp)->m_Owner ),
					&(*iExp)->m_Ignorables,
					0,
					OF_CULL_SAME_MEMBERSHIP | OF_CULL_PARTY_MEMBERS | OF_CULL_GHOSTS | OF_CULL_INVINCIBLE | OF_BREAKABLES | OF_ACTORS | OF_MATCH_ANY,	// actors or breakables are ok
					&occupants );
		}


		GopColl::iterator ipGo = occupants.begin(), occ_end = occupants.end();
		for(; ipGo != occ_end; ++ipGo )
		{
			eLifeState life_state = (*ipGo)->GetLifeState();
			if( (life_state != LS_IGNORE) || (life_state != LS_GONE) )
			{
				// damage the go
				gRules.DamageGoVolume( (*ipGo)->GetGoid(), (*iExp)->m_Owner, (*iExp)->m_OwnerWeapon, (*iExp)->m_Damage, m_dt, true );
			}
		}
		++iExp;
	}
}


void
Sim :: UpdateDamageVolumes( void )
{
	// Insert any damage volumes waiting to be processed
	if( !m_SimDmgInsQueue.empty() )
	{
		SimDmgColl::iterator iSimDmg = m_SimDmgInsQueue.begin(), iSimDmgEnd = m_SimDmgInsQueue.end();
		for(; iSimDmg != iSimDmgEnd; ++iSimDmg )
		{
			m_SimDmgs.push_back( *iSimDmg );
		}
		m_SimDmgInsQueue.clear();
	}

	// Process damage volumes
	SimDmgColl::iterator iSimDmg = m_SimDmgs.begin();

	while( iSimDmg != m_SimDmgs.end() )
	{
		// Remove this damage volume if time has expired
		if( (*iSimDmg)->m_elapsed >= (*iSimDmg)->m_duration )
		{
			GoHandle go( (*iSimDmg)->m_UnloadSignaler );
			if ( go && same_no_case( go->GetTemplateName(), "point" ) )
			{
				gGoDb.StartExpiration( go );
			}

			delete *iSimDmg;
			iSimDmg = m_SimDmgs.erase( iSimDmg );
			continue;
		}

		// Pre-dereference
		SimDmg &damage_vol = **iSimDmg;
		SiegePos start_position, end_position;

		// get rid of this damage volume if position query fails
		if( !damage_vol.GetStartPosition( start_position ) || !damage_vol.GetEndPosition( end_position ) )
		{
			(*iSimDmg)->m_elapsed = (*iSimDmg)->m_duration = 0;
			++iSimDmg;
			continue;
		}

		// Calculate cone direction
		vector_3 cone_direction = gSiegeEngine.GetDifferenceVector( start_position, end_position );

		// Determine origin
		SiegePos origin( start_position );

		if( !gWorldFx.AddPositions( origin, (0.5f * cone_direction), false, NULL ) )
		{
			continue;
		}

		// Put the direction in the space of the starting positions node
		siege::SiegeNode* pStartNode = gSiegeEngine.IsNodeInAnyFrustum( start_position.node );
		if( pStartNode )
		{
			cone_direction = pStartNode->GetCurrentOrientation() * cone_direction;
		}
		else
		{
			++iSimDmg;
			continue;
		}

		// Determine cone length and normalize direction
		float cone_length = 0;
		if( !cone_direction.IsZero() )
		{
			cone_length = Length( cone_direction );
		}

		// Determine radii
		float test_radius = (damage_vol.m_start_radius > damage_vol.m_end_radius) ? damage_vol.m_start_radius : damage_vol.m_end_radius;
		if( !cone_direction.IsZero() )
		{
			test_radius += (0.5f * cone_length);
		}

		// Setup the start and end positons in world space
		const vector_3 sp	= pStartNode->NodeToWorldSpace( start_position.pos );
		const vector_3 ep	= end_position.WorldPos();
		const vector_3 a	= ep - sp;

#		if !GP_RETAIL
		if( m_bDebugState )
		{
			// Draw centerline
			gSiegeEngine.GetWorldSpaceLines().DrawLine( sp, ep, 0xFFA0A0FF );

			// Draw start & and circles
			matrix_3x3 orient( MatrixFromDirection( Normalize(cone_direction) ) );
			float theta = 0, sin_theta = 0, cos_theta = 0;
			vector_3 last_sp, last_ep;

			while( theta < PI2 )
			{
				SINCOSF( theta, sin_theta, cos_theta );

				const float sx = damage_vol.m_start_radius * cos_theta;
				const float sy = damage_vol.m_start_radius * sin_theta;
				const float ex = damage_vol.m_end_radius * cos_theta;
				const float ey = damage_vol.m_end_radius * sin_theta;

				vector_3 cur_sp( (sx * orient.GetColumn0_T()) + (sy * orient.GetColumn1_T()) );
				vector_3 cur_ep( (ex * orient.GetColumn0_T()) + (ey * orient.GetColumn1_T()) );

				if( theta > 0 )
				{
					// Draw the circles
					gSiegeEngine.GetWorldSpaceLines().DrawLine( sp + cur_sp, sp + last_sp, 0xFF0000FF );
					gSiegeEngine.GetWorldSpaceLines().DrawLine( ep + cur_ep, ep + last_ep, 0xFF0000FF );

					// Draw connecting lines
					gSiegeEngine.GetWorldSpaceLines().DrawLine( sp + cur_sp, ep + cur_ep, 0xFF6060FF );
				}

				theta += PI2/24.0f;

				last_sp = cur_sp;
				last_ep = cur_ep;
			}

			gWorld.DrawDebugSphere( origin, test_radius, 0xFF00FF00, "" );
		}
#		endif


		const float m = (damage_vol.m_end_radius - damage_vol.m_start_radius) / cone_length;

		// Radius and origin of sphere have been determined so get the occupants
		GopColl occupants;
		gAIQuery.GetOccupantsInSphere(
				origin,
				test_radius,
				GoHandle( damage_vol.m_Owner ),
				&(*iSimDmg)->m_Ignorables,
				0,
				OF_CULL_SAME_MEMBERSHIP | OF_CULL_PARTY_MEMBERS | OF_CULL_GHOSTS | OF_CULL_INVINCIBLE | OF_BREAKABLES | OF_ACTORS | OF_MATCH_ANY,	// actors or breakables are ok
				&occupants );

		// Get target go and make sure it is in the occupant list
		Go* pTargetGo	= GetGo( damage_vol.m_target );
		if( pTargetGo )
		{
			GopColl::iterator iOcc = occupants.begin(), occ_end = occupants.end();
			for(; iOcc != occ_end; ++iOcc )
			{
				if( (*iOcc) == pTargetGo )
				{
					break;
				}
			}
			if( iOcc == occ_end )
			{
				occupants.push_back( pTargetGo );
			}
		}

		// Process occupants reported from test radius
		GopColl::iterator iOcc = occupants.begin(), occ_end = occupants.end();
		for(; iOcc != occ_end; ++iOcc )
		{
			SiegePos occ_pos = (*iOcc)->GetPlacement()->GetPosition();
			const vector_3 b = occ_pos.WorldPos() - sp;

			const float ab	 = a.DotProduct( b );

			// Reject any qualified position beyond the ends of the cone
			if( (*iOcc) != pTargetGo && ((ab < 0) || (ab>cone_length*cone_length)) )
			{
				continue;
			}

			vector_3 projection = pStartNode->GetTransposeOrientation() * ( a * ( ab / a.DotProduct( a )) );
			SiegePos proj_pos( start_position );

			if( !gWorldFx.AddPositions( proj_pos, projection, false, NULL ) )
			{
				continue;
			}

			vector_3 ray_dir( occ_pos.WorldPos() - proj_pos.WorldPos() );
			const float ray_len = Length( ray_dir );

			const float i_radius = m * Length( projection ) + damage_vol.m_start_radius;

			if( (*iOcc) == pTargetGo || ray_len <= i_radius )
			{
				eLifeState life_state = (*iOcc)->GetLifeState();
				if( (life_state != LS_IGNORE) || (life_state != LS_GONE) )
				{
#					if !GP_RETAIL
					if( m_bDebugState )
					{
						gSiegeEngine.GetWorldSpaceLines().DrawLine( proj_pos.WorldPos(), occ_pos.WorldPos(), 0xFFFF2020 );
					}
#					endif

					// damage the go
					gRules.DamageGoVolume( (*iOcc)->GetGoid(), (*iSimDmg)->m_Owner, (*iSimDmg)->m_OwnerWeapon, (*iSimDmg)->m_damage, m_dt );
				}
			}
		}

		// Update volume elapsed
		(*iSimDmg)->m_elapsed += m_dt;

		++iSimDmg;
	}
}


bool
Sim :: UpdateObjectStatus( SimObj & object )
{
	// skip objects that are marked to be deleted
	if( object.m_bInDeletion )
	{
		return false;
	}

	// if a go that is being controlled by a sim becomes invalid stop the sim for that go
	// isvalid(checks against null) will test if the m_go is null, but we also need to make sure that it has not
	// been marked for deletion, because we can't access stuff if it has been marked for
	// deletion.
	GoHandle hObjectGo( object.m_ObjectID );
	if( !hObjectGo )
	{
		RemoveObject( object.m_ID );
		return false;
	}

	if( hObjectGo->IsMarkedForDeletion() == true )
	{
		RemoveObject( object.m_ID );
		return false;
	}

   	// increment elapsed time for simulation
	object.m_Elapsed = object.m_Elapsed + m_dt;

	// if this object has expired set it up for deletion
	if( object.m_Elapsed >= object.m_Duration )
	{
		if( object.m_bExplodeOnExpiration )
		{
			// resolve who the owner of this object is as an Actor
			GoHandle	hWeapon( object.m_OwnerID );
			Goid Attacker( GOID_INVALID );
			if( hWeapon && hWeapon->HasParent() )
			{
				Attacker = hWeapon->GetParent()->GetGoid();
			}

			// only the server is authorized to dole out damage - clients will be informed from server for visual
			if( gServer.IsLocal() )
			{
				ExplodeGoWithDamage( object.m_ObjectID, Attacker, object.m_OwnerID );
			}
		}

		// make sure that if the simulation duration for a drop simulated object has expired to force it to be in the
		// proper position that it is supposed to be at so it isn't lost because of unforseen circumstances
		if( object.m_bIsDropSim && !object.m_bObjectAtRest )
		{
			object.m_bObjectAtRest = true;

			// Stop the object from continuing to move so that it can go through the "at rest" procedure
			object.m_Velocity			= vector_3::ZERO;
			object.m_Acceleration		= vector_3::ZERO;
			object.m_AngularAcceleration= vector_3::ZERO;
			object.m_AngularVelocity	= vector_3::ZERO;
			object.m_Position			= object.m_DropPos;
			object.m_Orientation		= object.m_DropQuat.BuildMatrix();

			PositionGameObject( object );
		}
		else
		{
			m_ObjDeletionQueue.push_back( object.m_ID );
			return false;
		}
	}

	// If we are attached to something and our life has expired then setup the destruction
	if( (object.m_AttachedID != GOID_INVALID) && object.m_bAttached )
	{
		// Override Sim duration
		object.m_Duration	= object.m_AttachLife + m_dt;

		object.m_AttachLife	= object.m_AttachLife - m_dt;

		if( object.m_AttachLife <= 0 )
		{
			RemoveObject( object.m_ID );
			return false;
		}
	}

	// If we are stuck in something then don't update this object
	if( !object.m_bIsDropSim && (object.m_bObjectAtRest || object.m_bAttached) )
	{
		++m_at_rest_count;
		return false;
	}

	// make sure this go isn't simulating outside the visible world
	GoHandle hGo( object.m_ObjectID );

	// the simulation should never try and control the movement of an actor
	gpassert( !hGo || !hGo->HasActor() );

	if( hGo && !hGo->IsInAnyWorldFrustum() )
	{
		RemoveObject( object.m_ID );
		return false;
	}

	return true;
}


bool
Sim :: UpdateObjectMotion( SimObj & object, float t )
{
	if( object.m_bObjectAtRest )
	{
		return false;
	}

	vector_3	object_gravity			( 0.0f, -object.m_Gravity, 0.0f	);

	SiegePos	last_position			( object.m_Position );

	// read & calculate new kinematic quantites from last
	vector_3	new_acceleration		( object.m_Acceleration );
	vector_3	new_angular_acceleration( object.m_AngularAcceleration );

	vector_3	new_velocity			( (object.m_Acceleration + object_gravity ) * t + object.m_Velocity );
	vector_3	new_angular_velocity	( object.m_AngularAcceleration * t + object.m_AngularVelocity );

	SiegePos	new_position			( (object.m_Acceleration + object_gravity ) * (t*t) * 0.5f + object.m_Velocity * t + object.m_Position.pos, object.m_Position.node );
	vector_3	new_angular_position	( object.m_AngularAcceleration * (t*t) * 0.5f + object.m_AngularVelocity * t + object.m_AngularPosition );

	vector_3	new_direction;
	matrix_3x3	new_orientation;


	// adjust for the possiblity of moving into a new node
	UpdateNodePosition( object, last_position, new_position, new_acceleration, new_velocity, object.m_InitialOrientation, object.m_bCanDoDamage );

	// set direction
	UpdateObjectOrientation( object, t, new_position, new_acceleration, new_velocity, new_direction, new_orientation );

	// check for collision
	const vector_3	half_diag( GetHalfDiag( object.m_ObjectID ) );

	gpassertm( half_diag.Length() < 495, ("Object is too large") );

	const float	width	= 2.0f * half_diag.x;
	const float	height	= 2.0f * half_diag.y;

	Goid		contact_id = GOID_INVALID;
	SiegePos	contact_position_go, contact_normal_go;
	SiegePos	contact_position_terrain, contact_normal_terrain;

	// check to see if there was a possible collision with the terrain
	bool bTerrainCollision	= CheckForTerrainCollision( contact_position_terrain, contact_normal_terrain, last_position, new_position );
	bool bGoCollision		= false;

	if( object.m_bCollideWithGOs && !object.m_bIsDropSim )
	{
		bGoCollision = CheckForObjectCollision( contact_position_go, contact_normal_go, contact_id, object.m_Ignorables, last_position, new_position, width, height, object.m_Attacker );
		if( bGoCollision )
		{
			gpassert( contact_id != GOID_INVALID );
		}
	}

	// collision occured with both the terrain and a go so determine which one is closer to the starting position
	if( bTerrainCollision && bGoCollision )
	{
		const float tdist2 = Length2( gSiegeEngine.GetDifferenceVector( contact_position_terrain, last_position ));
		const float gdist2 = Length2( gSiegeEngine.GetDifferenceVector( contact_position_go, last_position ));

		bTerrainCollision = ( tdist2 < gdist2 );
	}

	// process collision
	if( bTerrainCollision )
	{
		// make sure that projectiles that stick into the ground have the correct orientation
		if( (object.m_DeflectionAngle == 0) && (new_position.node != contact_position_terrain.node) )
		{
			matrix_3x3	new_orient,	old_orient;
			vector_3	new_trans,	old_trans;

			GetNodeInfo( new_position.node, old_orient, old_trans );
			GetNodeInfo( contact_position_terrain.node,	 new_orient, new_trans );

			matrix_3x3 orient_difference= Transpose( new_orient * Transpose( old_orient ) );

			new_acceleration			= orient_difference * new_acceleration;
			new_velocity				= orient_difference * new_velocity;
			object.m_InitialOrientation	= orient_difference * object.m_InitialOrientation;

			UpdateObjectOrientation( object, t, contact_position_terrain, new_acceleration, new_velocity, new_direction, new_orientation );
		}

		// Process terrain collision
		ProcessTerrainCollision( object, t, contact_position_terrain, contact_normal_terrain, new_angular_acceleration, new_angular_velocity, new_acceleration, new_velocity, new_position );
	}
	else if( bGoCollision )
	{
		ProcessGoCollision( object, t, contact_position_go, contact_normal_go, new_angular_acceleration, new_angular_velocity, new_acceleration, new_velocity, contact_id );
	}
	else
	{
		object.m_bInCollision = false;
	}


	// syncronize collisions according to the server
	if( !gServer.IsLocal() )
	{
		SyncronizeCollisions( object );
	}

	// override based on object state
	if( object.m_bObjectAtRest && object.m_bIsDropSim )
	{
		new_position	= object.m_DropPos;
		new_orientation = object.m_DropQuat.BuildMatrix();
	}

	// write new kinematic quantities
	object.m_Acceleration		 = new_acceleration;
	object.m_AngularAcceleration = new_angular_acceleration;

	object.m_Velocity			 = new_velocity;
	object.m_AngularVelocity	 = new_angular_velocity;

	object.m_Position			 = new_position;
	object.m_AngularPosition	 = new_angular_position;

	object.m_Direction			 = new_direction;
	object.m_Orientation		 = new_orientation;


	// position the game object
	PositionGameObject( object );

	return ( bTerrainCollision );
}


void
Sim :: UpdateNodePosition( SimObj & object, SiegePos & last_position, SiegePos & new_position, vector_3 & new_acceleration, vector_3 & new_velocity, matrix_3x3 & initial_orient, bool bDoesDamage )
{
	// adjust vectors upon entering new node
	SiegePos	adjusted_pos( new_position );

	siege::SiegeNode *pNode = gSiegeEngine.IsNodeValid( adjusted_pos.node );

	// early out if not in a valid node anymore
	if( pNode == NULL )
	{
		RemoveObject( object.m_ID );
		return;
	}

	gSiegeEngine.AdjustPointToTerrainPriority( pNode, adjusted_pos, siege::LF_IS_FLOOR, siege::LF_IS_WALL | siege::LF_IS_WATER, 3, !bDoesDamage );

	if( !(new_position.node == adjusted_pos.node) )
	{
		new_position.pos	= new_position.WorldPos();
		new_position.FromWorldPos( new_position.pos, adjusted_pos.node );

		matrix_3x3	new_orient,	old_orient;
		vector_3	new_trans,	old_trans;

		GetNodeInfo( last_position.node, old_orient, old_trans );
		GetNodeInfo( new_position.node,	 new_orient, new_trans );

		matrix_3x3 orient_difference= Transpose( new_orient * Transpose( old_orient ) );

		new_acceleration = orient_difference * new_acceleration;
		new_velocity	 = orient_difference * new_velocity;
		initial_orient	 = orient_difference * initial_orient;
	}
}


void
Sim :: UpdateObjectOrientation( SimObj & object, float t, SiegePos & new_position, vector_3 & new_acceleration, vector_3 & new_velocity, vector_3 &new_direction, matrix_3x3 & new_orientation )
{
	// determine direction by integrating with a smaller timestep
	float last_dT = t * 0.5f;

	vector_3	object_gravity( 0, -object.m_Gravity, 0 );

	vector_3	vel( (new_acceleration + object_gravity ) * last_dT + new_velocity );
	vector_3	pos( (new_acceleration + object_gravity ) * (last_dT*last_dT) * 0.5f + new_velocity*last_dT + new_position.pos );

	SiegePos	t_pos( pos, new_position.node );
	new_direction = gSiegeEngine.GetDifferenceVector( t_pos, new_position );

	// create orientation using trajectory if specified to do so
	matrix_3x3 trajectory_orientation;

	if( object.m_bUseTrajectoryToOrient )
	{
		if( new_direction.IsZero() )
		{
 			trajectory_orientation = matrix_3x3::IDENTITY;
		}
		else
		{
			vector_3 z_axis( -Normalize( new_direction ) );
			vector_3 y_axis( vector_3::UP );
			vector_3 x_axis( CrossProduct( y_axis, z_axis ) );

			if( x_axis.IsZero() )
			{
				x_axis = vector_3::NORTH;
			}
			else
			{
				x_axis = Normalize( x_axis );
			}

			y_axis = CrossProduct( z_axis, x_axis );

			// linear orientation
			trajectory_orientation = MatrixColumns( x_axis, y_axis, z_axis );
		}
	}

	// angular orientation
	matrix_3x3	angular_orientation((XRotationColumns( object.m_AngularPosition.x) *
									YRotationColumns( object.m_AngularPosition.y)) *
									ZRotationColumns( object.m_AngularPosition.z) );

	if( object.m_bUseTrajectoryToOrient )
	{
		new_orientation = trajectory_orientation * angular_orientation;
	}
	else
	{
		new_orientation = object.m_InitialOrientation * angular_orientation;
	}
}


void
Sim :: ProcessTerrainCollision( SimObj & object, float t, SiegePos & contact_position_terrain, SiegePos & contact_normal_terrain, vector_3 & new_angular_acceleration, vector_3 & new_angular_velocity, vector_3 & new_acceleration, vector_3 & new_velocity, SiegePos & new_position )
{
	++m_collision_count;

	if( object.m_bIsDropSim && object.m_bPlayDropSound )
	{
		// stop this thing from rotating anymore
		// Clear kinematic quantities
		new_angular_acceleration= vector_3::ZERO;
		new_angular_velocity	= vector_3::ZERO;

		object.m_bPlayDropSound = false;

		GoHandle hObject( object.m_ObjectID );
		hObject->PlayVoiceSound( "put_down" );
	}

	const vector_3 i( Normalize( new_velocity ) );
	const float s = InnerProduct( contact_normal_terrain.pos, -i );

	bool bGlance = true;
	bool bCanStick = (0 == object.m_DeflectionAngle);

	if( (Length( new_velocity ) < 1.25f) || bCanStick )
	{
		bGlance = false;
	}
	else if( object.m_bUseTrajectoryToOrient )
	{
		bGlance = ( s < object.m_DeflectionAngle );
	}

	// Don't bothter doing much more if the object should just explode
	if( object.m_bExplodeIfHitTerrain  )
	{
		if( GoHandle( object.m_ObjectID )->IsInAnyWorldFrustum() )
		{
			WorldMessage ( WE_COLLIDED, object.m_OwnerID, object.m_ObjectID, MakeInt(GOID_INVALID) ).Send();

			if( gServer.IsLocal() )
			{
				ExplodeGoWithDamage( object.m_ObjectID, object.m_Attacker, object.m_OwnerID );
			}
		}
		return;
	}

	// calculate reflection
	vector_3	p		=	i - contact_normal_terrain.pos * -s;
	vector_3	q		=	contact_normal_terrain.pos * -s;
	vector_3	rebound =	(p - q);

	bool bCanStopMoving = true;

	// if this is a "wall" then make sure that the object keeps moving after new velocity vector is calculated
	if( DotProduct( vector_3::UP, contact_normal_terrain.pos ) < 0.875f )
	{
		new_velocity	=	object.m_Restitution * rebound * Length( new_velocity );
		bCanStopMoving	=	false;

		object.m_bInCollision = true;
	}

	if( (bGlance || !bCanStopMoving) && !bCanStick )
	{
		object.m_bCanDoDamage = false;

		new_velocity		=  object.m_Restitution * rebound * Length( new_velocity );
		new_velocity		+= vector_3( 0, -object.m_Gravity, 0 ) * t;

		new_angular_velocity*= Normalize( rebound );

		if( gServer.IsLocal() )
		{
			DamageObjectFromCollision( object, new_velocity );
		}

		// Resolve contact
		if( bCanStopMoving && ( Length( new_velocity ) < 1.25f ) && object.m_bShouldDestroy )
		{
			object.m_bObjectAtRest = true;
			GoHandle hObject( object.m_ObjectID );
			if( hObject && hObject->HasAspect() )
			{
				hObject->GetAspect()->SetIsCollidable( false );
			}
		}

		const float mag = Length( object.m_AngularVelocity );
		const vector_3 spin( CrossProduct( object.m_AngularVelocity, rebound ));

		object.m_AngularVelocity = mag * spin;

		// Readjust before forcing into new node
		if( contact_position_terrain.node != new_position.node )
		{
			siege::cache<siege::SiegeNode> &node_cache = gSiegeEngine.NodeCache();

			siege::SiegeNodeHandle	Handle	( node_cache.UseObject( new_position.node ) );
			siege::SiegeNode const	&OldNode= Handle.RequestObject( node_cache );
			Handle = node_cache.UseObject	( contact_position_terrain.node );
			siege::SiegeNode const	&NewNode= Handle.RequestObject( node_cache );

			matrix_3x3 new_orient = NewNode.GetCurrentOrientation();
			matrix_3x3 old_orient = OldNode.GetCurrentOrientation();

			matrix_3x3 orient_difference( Transpose( new_orient * Transpose( old_orient ) ) );

			new_acceleration	= orient_difference * new_acceleration;
			new_velocity		= orient_difference * new_velocity;
			object.m_InitialOrientation = ( orient_difference * object.m_InitialOrientation );
		}

		new_position.pos	= contact_position_terrain.pos + (contact_normal_terrain.pos * 0.001f);
		new_position.node	= contact_position_terrain.node;

		// Let the rest of the code know that a glance occured if this object is owned by someone
		if( (object.m_OwnerID != GOID_INVALID ) && GoHandle( object.m_ObjectID )->IsInAnyWorldFrustum() )
		{
			WorldMessage ( WE_GLANCED, object.m_OwnerID, object.m_ObjectID, MakeInt(GOID_INVALID) ).Send();

			// Signal a near miss if appropriate
			SendNearMiss( object );
		}
	}
	else
	{
		object.m_bCanDoDamage	= false;

		// Stick
		new_acceleration		= vector_3::ZERO;
		new_velocity			= vector_3::ZERO;
		new_angular_acceleration= vector_3::ZERO;
		new_angular_velocity	= vector_3::ZERO;
		new_position			= contact_position_terrain;
		object.m_bObjectAtRest	= true;

		if( GoHandle( object.m_ObjectID )->IsInAnyWorldFrustum() )
		{
			// Let the rest of the code know that a collision occured
			WorldMessage ( WE_COLLIDED, object.m_OwnerID, object.m_ObjectID, MakeInt(GOID_INVALID) ).Send();

			// Signal a near miss if appropriate
			SendNearMiss( object );
		}
	}
}


void
Sim :: ProcessGoCollision( SimObj & object, float t, SiegePos & contact_position_go, SiegePos & contact_normal_go, vector_3 & new_angular_acceleration, vector_3 & new_angular_velocity, vector_3 & new_acceleration, vector_3 & new_velocity, Goid contact_id )
{
	if( object.m_bInCollision )
	{
		return;
	}

	object.m_bInCollision = true;

	// Collisions with game objects are only processed on the server
	++m_collision_count;

	const vector_3 i( Normalize( new_velocity ) );
	const float s = InnerProduct( contact_normal_go.pos, -i );

	GoHandle contact_target( contact_id );

	if( contact_target && !contact_target->IsMarkedForDeletion() )
	{
		bool bGlance = true;
		const bool bPassThrough = object.m_bPassThrough;

		if( !bPassThrough && object.m_bUseTrajectoryToOrient )
		{
			// if the angle is set to 0 then the projectile should always stick
			if( 0 == object.m_DeflectionAngle )
			{
				bGlance = false;
			}
			else
			{
				bGlance = (s < object.m_DeflectionAngle );
			}
		}

		// Don't bothter doing much more if the object should just explode
		if( object.m_bExplodeIfHitGo )
 		{
			GoHandle hObject( contact_id );

			if( gServer.IsLocal() && GoHandle( object.m_ObjectID )->IsInAnyWorldFrustum() )
			{
				WorldMessage( WE_COLLIDED, GOID_INVALID, object.m_ObjectID, MakeInt( contact_id ) ).Send();
				ExplodeGoWithDamage( object.m_ObjectID, object.m_Attacker, object.m_OwnerID );
			}
			return;
		}

		if( bGlance || bPassThrough )
		{
			if( !bPassThrough )
			{
				// The angle this simulation object hit the game object was too great
				// so we will just glance off of it
				vector_3	p		=	i - contact_normal_go.pos * -s;
				vector_3	q		=	contact_normal_go.pos * -s;
				vector_3	rebound =	object.m_Restitution * (p - q);
				new_velocity		=	rebound * ( Length(new_velocity) );
				new_velocity		+=	vector_3( 0, -object.m_Gravity, 0 ) * t;
			}

			if( gServer.IsLocal() )
			{
				DamageObjectFromCollision( object, new_velocity );
			}

			// Let the rest of the code know that a glance occured
			if( object.m_OwnerID != GOID_INVALID )
			{
				GoHandle hObject( contact_id );

				if( GoHandle( object.m_ObjectID )->IsInAnyWorldFrustum() && object.m_bCanDoDamage && !hObject->IsMarkedForDeletion() )
				{
					object.m_bCanDoDamage = false;

					// Let the rest of the world know that we hit our target so damage and other stuff can happen
					WorldMessage( WE_GLANCED, object.m_OwnerID, object.m_ObjectID, MakeInt(GOID_INVALID) ).Send();
					WorldMessage( WE_COLLIDED, object.m_OwnerID, object.m_ObjectID, MakeInt(contact_id) ).Send();

					// Save collision information in the simulation object for the go that has collided so that the client
					// can syncronize to it if it hasn't already done it.
					if( gServer.IsLocal() && !hObject->IsMarkedForDeletion())
					{
						RCSetCollision( object.m_ObjectID, contact_id, true );
					}
				}
			}

			// Signal a near miss if appropriate
			SendNearMiss( object );
		}
		else
		{
			// The deflection angle couldn't save this go so puncture it

			// Clear kinematic quantities
			new_acceleration		= vector_3::ZERO;
			new_velocity			= vector_3::ZERO;
			new_angular_acceleration= vector_3::ZERO;
			new_angular_velocity	= vector_3::ZERO;

			// Set relevant attach information
			object.m_AttachOrientation	= object.m_Orientation;
			object.m_AttachedID			= contact_id;
			object.m_Position			= contact_position_go;

			GoHandle hObject( contact_id );

			if( GoHandle( object.m_ObjectID )->IsInAnyWorldFrustum() && object.m_bCanDoDamage && !hObject->IsMarkedForDeletion() )
			{
				object.m_bCanDoDamage = false;

				// Let the rest of the world know that we hit our target so damage and other stuff can happen
				WorldMessage( WE_COLLIDED, object.m_OwnerID, object.m_ObjectID, MakeInt(contact_id) ).Send();

				// Save collision information in the simulation object for the go that has collided so that the client
				// can syncronize to it if it hasn't already done it.
				if( !hObject->IsMarkedForDeletion() )
				{
					if( gServer.IsLocal() )
					{
						RCSetCollision( object.m_ObjectID, contact_id, false );
					}

					// Signal a near miss if appropriate
					if( contact_id != object.m_IntendedTarget )
					{
						SendNearMiss( object );
					}

					// Attach the projectile to the go it collided with
					AttachToGameObject( contact_position_go, contact_id, object.m_ObjectID, object.m_AttachedID, object.m_OwnerID );

					if( gServer.IsLocal() )
					{
						DamageObjectFromCollision( object, new_velocity );
					}
				}
			}
		}
	}
}


void
Sim :: SyncronizeCollisions( SimObj & object )
{
	GoHandle hObject( object.m_ObjectID );

	GoPhysics *pPhysics = hObject->QueryPhysics();

	if( pPhysics && pPhysics->GetCollisionOccured() )
	{
		SimID id = GetSimID( object.m_ObjectID );

		SimObjColl::iterator iSimObj;

		if( FindObject( m_Objects, id, iSimObj ) )
		{
			if( GoHandle( (*iSimObj)->m_ObjectID )->IsInAnyWorldFrustum() )
			{
				if( pPhysics->GetCollisionGlanced() )
				{
					WorldMessage( WE_GLANCED, (*iSimObj)->m_OwnerID, (*iSimObj)->m_ObjectID, MakeInt(GOID_INVALID) ).Send();
				}

				WorldMessage( WE_COLLIDED, (*iSimObj)->m_OwnerID, (*iSimObj)->m_ObjectID, MakeInt(pPhysics->GetCollisionGoid()) ).Send();
			}

			// Reset the collision
			pPhysics->SetCollisionOccured	( false );
			pPhysics->SetCollisionGlanced	( false );
			pPhysics->SetCollisionGoid		( GOID_INVALID );
		}
	}
}


void
Sim :: PositionGameObject( SimObj & object )
{
	GoHandle hObject( object.m_ObjectID );

	if( hObject && !object.m_bAttached )
	{
		hObject->GetPlacement()->ForceSetPlacement( object.m_Position, object.m_Orientation );
	}

#	if !GP_RETAIL
	if( gSiegeEngine.GetOptions().IsCollisionBoxes() )
	{
		const vector_3	half_diag( GetHalfDiag( object.m_ObjectID ) );
		gWorld.DrawDebugBox( object.m_Position, object.m_Orientation, half_diag, 0xFF00FF00, m_dt );
	}
#	endif // !GP_RETAIL
}


void
Sim :: InsertNewObjects( void )
{
	if( !m_ObjInsertionQueue.empty() )
	{
		SimObjColl::iterator iSimObj = m_ObjInsertionQueue.begin(), sim_end = m_ObjInsertionQueue.end();

		// Each time AddObject is called it places those objects in a queue
		// to get inserted at the beginning of Sims update
		for(; iSimObj != sim_end; ++iSimObj )
		{
			m_Objects.push_back( *iSimObj );
		}
		m_ObjInsertionQueue.clear();
	}
}


void
Sim :: DestroyExpiredObjects( void )
{
	SimObjColl::iterator iSimObj;

	// Iterate through the deletion queue and remove/erase/delete/destroy the
	// listed objects.

	SimIDColl::iterator iID = m_ObjDeletionQueue.begin(), i_end = m_ObjDeletionQueue.end();
	for(; iID != i_end; ++iID )
	{
		SimObjColl *pCollectionToEraseFrom = &m_Objects;
		// Lookup the SimObj object from the queue based on it's id
		// so that we know that it is still valid - it should always be valid
		if( FindObject( m_Objects, *iID, iSimObj, true, &pCollectionToEraseFrom ) )
		{
			// Not all objects that end up getting temporarily controlled by
			// Sim should be destroyed - this flag is set for the
			// ones that should:
			if( (*iSimObj)->m_bShouldDestroy )
			{
				GoHandle hObject( (*iSimObj)->m_ObjectID );
				if ( hObject && (hObject->IsLocalGo() || ::IsServerLocal()) )
				{
					if( hObject->HasActor() && gWorld.IsMultiPlayer() )
					{
						hObject->GetAspect()->RCFadeOut();
					}
					else
					{
						bool bFadeOut	= true;
						if( (*iSimObj)->m_bAttached && !GetGo( (*iSimObj)->m_AttachedID ) )
						{
							// parent has been deleted, we don't want to fade in this case
							bFadeOut	= false;
						}
						gGoDb.SMarkForDeletion( hObject->GetGoid(), true, bFadeOut );
					}
				}
			}
			// Delete the physics simulation object - the SimObj
			delete *iSimObj;
			pCollectionToEraseFrom->erase( iSimObj );
		}
		else if( FindObject( m_InactiveObjects, *iID, iSimObj, false ) )
		{
			// Remove the object itself
			delete *iSimObj;
			m_InactiveObjects.erase( iSimObj );
		}
	}
	m_ObjDeletionQueue.clear();


	// Delete any explosions that have been in an inactive state
	SimExpColl::iterator iSimExp;
	iID = m_ExpDeletionQueue.begin(); i_end = m_ExpDeletionQueue.end();
	for( ; iID != i_end; ++iID )
	{
		if( FindExplosion( m_InactiveExplosions, *iID, iSimExp, false ) )
		{
			delete *iSimExp;
			m_InactiveExplosions.erase( iSimExp );
		}
	}

	// Delete requested damage volumes
	SimDmgColl::iterator iSimDmg;
	iID = m_DmgDeletionQueue.begin(); i_end = m_DmgDeletionQueue.end();
	for( ; iID != i_end; ++iID )
	{
		if( FindDamageVolume( m_InactiveSimDmgs, *iID, iSimDmg, false ) )
		{
			delete *iSimDmg;
			m_InactiveSimDmgs.erase( iSimDmg );
		}
	}
}


void
Sim :: ProcessResumeRequests( void )
{
	// Process loading the objects and their associated collision information
	SimIDColl::iterator iSimObj = m_ObjLoadQueue.begin(), iSimObj_end = m_ObjLoadQueue.end();
	for(; iSimObj != iSimObj_end; ++iSimObj )
	{
		// Move the actual object over to the active list
		SimObjColl::iterator iSimObject;
		if( FindObject( m_InactiveObjects, *iSimObj, iSimObject, false ) )
		{
			m_Objects.push_back( *iSimObject );
			m_InactiveObjects.erase( iSimObject );
		}
	}
	m_ObjLoadQueue.clear();


	// Process the loading of the exploosions
	SimIDColl::iterator iSimExp = m_ExpLoadQueue.begin(), iSimExp_end = m_ExpLoadQueue.end();
	for(; iSimExp != iSimExp_end; ++iSimExp )
	{
		SimExpColl::iterator iExplosion;
		if( FindExplosion( m_InactiveExplosions, *iSimExp, iExplosion, false ) )
		{
			m_Explosions.push_back( *iExplosion );
			m_InactiveExplosions.erase( iExplosion );
		}
	}
	m_ExpLoadQueue.clear();


	// Process the loading of the damage volumes
	SimIDColl::iterator iSimDmg = m_SimDmgLoadQueue.begin(), iSimDmg_end = m_SimDmgLoadQueue.end();
	for(; iSimDmg != iSimDmg_end; ++iSimDmg )
	{
		SimDmgColl::iterator iSimDamageVol;
		if( FindDamageVolume( m_InactiveSimDmgs, *iSimDmg, iSimDamageVol ) )
		{
			m_SimDmgs.push_back( *iSimDamageVol );
			m_InactiveSimDmgs.erase( iSimDamageVol );
		}
	}
	m_SimDmgLoadQueue.clear();
}


void
Sim :: ProcessPauseRequests( void )
{
	// Process the objects and any collision information they might have
	SimIDColl::iterator iSimObj = m_ObjUnloadQueue.begin(), iSimObj_end = m_ObjUnloadQueue.end();
	for(; iSimObj != iSimObj_end; ++iSimObj )
	{
		// Move the actual object over to the inactive list
		SimObjColl::iterator iSimObject;
		
		SimObjColl *pCollectionToEraseFrom = &m_Objects;

		if( FindObject( m_Objects, *iSimObj, iSimObject, true, &pCollectionToEraseFrom ) )
		{
			m_InactiveObjects.push_back( *iSimObject );
			pCollectionToEraseFrom->erase( iSimObject );
		}
	}
	m_ObjUnloadQueue.clear();


	// Process the explosions
	SimIDColl::iterator iSimExp = m_ExpUnloadQueue.begin(), iSimExp_end = m_ExpUnloadQueue.end();
	for(; iSimExp != iSimExp_end; ++iSimExp )
	{
		SimExpColl::iterator iExplosion;
		if( FindExplosion( m_Explosions, *iSimExp, iExplosion ) )
		{
			m_InactiveExplosions.push_back( *iExplosion );
			m_Explosions.erase( iExplosion );
		}
	}
	m_ExpUnloadQueue.clear();


	// Process the damage volumes
	SimIDColl::iterator iSimDmg = m_SimDmgUnloadQueue.begin(), iSimDmg_end = m_SimDmgUnloadQueue.end();
	for(; iSimDmg != iSimDmg_end; ++iSimDmg )
	{
		SimDmgColl::iterator iSimDamageVol;
		if( FindDamageVolume( m_SimDmgs, *iSimDmg, iSimDamageVol ) )
		{
			m_InactiveSimDmgs.push_back( *iSimDamageVol );
			m_SimDmgs.erase( iSimDamageVol );
		}
	}
	m_SimDmgUnloadQueue.clear();
}


SimID
Sim :: AddObject( SimObj *pSimObj )
{
	pSimObj->m_ID = ++m_NextID;

	GoHandle hObject( pSimObj->m_ObjectID );

	if( hObject->HasAspect() && hObject->GetAspect()->GetDoesBlockPath() )
	{
		gperrorf(( "Sim::AddObject - [%s] is a path blocking go. No path blocking go should be moved\n", hObject->GetTemplateName() ));
		return ID_INVALID;
	}

	gpassert( !hObject->HasActor() );

	if( hObject && !hObject->HasActor() )
	{
		hObject->GetPhysics()->SetSimulationId( pSimObj->m_ID );
	}

	m_ObjInsertionQueue.push_back( pSimObj );

	return pSimObj->m_ID;
}


bool
Sim :: CreateObject( SimObj **pSimObj, Goid object, Goid parent, const SiegePos &start_pos, DWORD seed )
{
	GoHandle hObject( object );

	gpassert( !hObject->HasActor() );

	if( hObject && !hObject->HasActor() && hObject->HasPhysics() )
	{
		m_RNG.SetSeed( seed );

		GoPhysics *physics = hObject->GetPhysics();

		SimObj	*pNewObject = new SimObj(	parent,								// Goid owner
											object,								// Goid object
											physics->GetGravity(),				// constant of gravitational acceleration
											physics->GetMass(),					// Mass
											physics->GetElasticity(),			// Restitution
											physics->GetFriction(),				// Friction
											physics->GetDeflectionAngle(),		// Deflection angle
											start_pos,							// position
											matrix_3x3::IDENTITY,				// initial orientation
											vector_3::ZERO,						// velocity
											vector_3::ZERO,						// acceleration
											physics->GetOrientToTrajectory(),	// True if arrow or bolt, false if rock
											physics->GetExplodeIfHitGo(),		// True if projectile should break on contact with a go
											physics->GetExplodeIfHitTerrain(),	// True if projectile should break on contact with terrain
											physics->GetExplodeOnExpiration(),	// True if projectile should explode when it timesout
											true,								// We are interested in hitting other game objects
											physics->GetSimDuration()			// timeout time in seconds
											);

		float vx = physics->GetAngularVelocity().x;
		float vy = physics->GetAngularVelocity().y;
		float vz = physics->GetAngularVelocity().z;

		if( physics->GetRandomizeVelocity() )
		{
			vx = m_RNG.Random( -vx, vx );
			vy = m_RNG.Random( -vy, vy );
			vz = m_RNG.Random( -vz, vz );
		}

		const vector_3 angular_velocity( vx, vy, vz );

		pNewObject->m_AngularVelocity = angular_velocity;
		*pSimObj = pNewObject;

		// resolve who the actual attacker is for this object
		GoHandle	hWeapon( parent );

		if( hWeapon )
		{
			if( hWeapon->HasActor() )
			{
				pNewObject->m_Attacker = hWeapon->GetGoid();
			}
			else if( hWeapon->HasParent() )
			{
				pNewObject->m_Attacker = hWeapon->GetParent()->GetGoid();
			}
		}
		return true;
	}
	return false;
}


void
Sim :: DamageObjectFromCollision( SimObj & object, const vector_3 &velocity )
{
	CHECK_SERVER_ONLY;

	if( !object.m_bIsDropSim )
	{
		float lv = Length( velocity );
		lv *= lv;
		const float contrived_damage = 0.115f * lv * lv * (1.0f - object.m_Restitution);

		gRules.DamageGo( object.m_ObjectID, object.m_OwnerID, GOID_INVALID, contrived_damage, false );
	}
}


void Sim :: SendNearMiss( SimObj & object )
{
	if( gServer.IsLocal() )
	{
		// Let the intended target know that the projectile is/was very close to hitting
		GoHandle hIntendedTarget( object.m_IntendedTarget );
		if( hIntendedTarget && hIntendedTarget->HasMind() && hIntendedTarget->IsInAnyWorldFrustum() )
		{
			const float distance = Length( gSiegeEngine.GetDifferenceVector( hIntendedTarget->GetPlacement()->GetPosition(), object.m_Position ) );

			if( distance <= ( hIntendedTarget->GetMind()->GetSightRange() * 0.5f ) )
			{
				WorldMessage ( WE_ALERT_PROJECTILE_NEAR_MISSED, GOID_INVALID, object.m_IntendedTarget, MakeInt( object.m_OwnerID ) ).Send();
			}
		}
	}
}


void
Sim :: AttachToGameObject( SiegePos contact_point, Goid contact_id, Goid object_id, Goid parent_id, Goid owner_id )
{
	GoHandle hTarget( contact_id );
	GoHandle hProjectile( object_id );
	GoHandle hParent( parent_id );

	if( hTarget && hProjectile && hParent )
	{
		// Parent of the projectile is now the target
		hProjectile->SetParent( hTarget );

		GoAspect& Parent_Aspect		= *hParent->GetAspect();
		GoAspect& Projectile_Aspect	= *hProjectile->GetAspect();

		nema::HAspect pParent_aspect	= Parent_Aspect.GetAspectHandle();
		nema::HAspect pProjectile_aspect= Projectile_Aspect.GetAspectHandle();

		const float parent_render_scale = Parent_Aspect.GetRenderScale();
		const float inverse_parent_render_scale = 1.0f/parent_render_scale;

		// Use primary bone of projectile
		int	nema_bone = 0;

		// Get the current position/rotation of the projectile's attach point
		vector_3	tip_position(DoNotInitialize);
		Quat		tip_rotation(DoNotInitialize);

		pProjectile_aspect->GetIndexedBoneOrientation( nema_bone, tip_position, tip_rotation );

		tip_position *= parent_render_scale;

 		pProjectile_aspect->UnlockPrimaryBone();

		if( contact_point.node != hProjectile->GetPlacement()->GetPosition().node )
		{
			contact_point.FromWorldPos( contact_point.WorldPos(), hProjectile->GetPlacement()->GetPosition().node );
		}
		tip_position =  contact_point.pos +
						hProjectile->GetPlacement()->GetOrientation().RotateVector_T( tip_position );

		tip_rotation = 	hProjectile->GetPlacement()->GetOrientation() * tip_rotation;

		// Get the current position/rotation of the bone that is closest to the tip of the arrow
		SiegePos tip_siegepos = hProjectile->GetPlacement()->GetPosition();
		tip_siegepos.pos = tip_position;

		const vector_3 tip_relative = gSiegeEngine.GetDifferenceVector( hParent->GetPlacement()->GetPosition(), tip_siegepos );

		vector_3	bone_position(vector_3::ZERO);
		Quat		bone_rotation(Quat::IDENTITY);
		bool		bSingleBone			= true;
		int			closest_bone_index	= 0;
		bool		bIsPurged			= pParent_aspect->IsPurged( 2 );

		if( !bIsPurged )
		{
			bSingleBone			= pParent_aspect->GetNumBones() <= 1;
			closest_bone_index	= GetClosestBoneIndex( hTarget, hParent, pParent_aspect, parent_render_scale,
													contact_point, tip_relative, bSingleBone, bone_position, bone_rotation );
		}

		// Get the relative difference between the two position/orientations in the BONE space
		vector_3	delta_position;
		Quat		delta_rotation;

		gSiegeEngine.GetDifferenceVectorOrientation( hParent->GetPlacement()->GetPosition().node, bone_position, bone_rotation,
													 hProjectile->GetPlacement()->GetPosition().node, tip_position, tip_rotation,
													 delta_position, delta_rotation );

		//*** Attach the projectile to the object we have hit offset by the delta
		if( !bSingleBone && Length2( delta_position ) > (0.275f * 0.275f) )
		{
			delta_position = Normalize( delta_position ) * -0.275f;
		}

		// Need to compensate for the scale of the parent in the difference delta --biddle
		delta_position *= inverse_parent_render_scale;

		hProjectile->GetPlacement()->CopyPosition( *hParent->GetPlacement() );
		if( !bIsPurged )
		{
			pParent_aspect->AttachReverseLinkedChild( pProjectile_aspect, pParent_aspect->GetBoneName( closest_bone_index ),
							pProjectile_aspect->GetBoneName( nema_bone ), delta_rotation, delta_position );
		}

		vector_3 kid_scale = pProjectile_aspect->GetCurrentScale();
		kid_scale *= inverse_parent_render_scale;
		pProjectile_aspect->SetCurrentScale( kid_scale );

		Projectile_Aspect.SetIsVisible( true );

		// Use the appropriate attach duration for party member / enemy

		bool stuck_in_party_member = false;

		if( owner_id != GOID_INVALID )
		{
			Go* pParty = gServer.GetScreenParty();
			if( pParty != NULL )
			{
				GopColl::const_iterator i, begin = pParty->GetChildBegin(), end = pParty->GetChildEnd();
				for ( i = begin ; i != end ; ++i )
				{
					if ( (*i)->GetGoid() == contact_id )
					{
						stuck_in_party_member = true;
						break;
					}
				}
			}
		}

		SimObj *pObject = NULL;

		if( FindObject( object_id, &pObject ) )
		{
			if( stuck_in_party_member )
			{
				pObject->m_AttachLife = m_Party_Attach_Life;
			}
			else
			{
				pObject->m_AttachLife = m_Enemy_Attach_Life;
			}

			// Attach it
			pObject->m_bAttached		= true;
			pObject->m_bCollideWithGOs	= false;
		}

		hProjectile->GetAspect()->SetIsCollidable( false );

		// Use position from attached go
		hProjectile->GetPlacement()->CopyPlacement( *hTarget->GetPlacement() );
	}
}


int
Sim :: GetClosestBoneIndex( Go *pTarget, Go*pParent, nema::HAspect &pParent_aspect, float parent_render_scale, const SiegePos &contact_point,
						   const vector_3 test_position, bool &bSingleBone, vector_3 &bone_position, Quat &bone_rotation )
{
	UNREFERENCED_PARAMETER( contact_point );
	int closest_bone_index = 0;

	if( pTarget->HasBody() )
	{
		const BoneTranslator &bt = pTarget->GetBody()->GetBoneTranslator();

		vector_3	temp_position(DoNotInitialize);
 		Quat		temp_rotation(DoNotInitialize);

		float min_dist = FLOAT_INFINITE;

		for( int i = 0; i < 3; ++i )
		{
			int nema_bone_index = 0;

			if( pTarget->HasBody() )
			{
				switch( i )
				{
				case 0:
					bt.Translate( BoneTranslator::KILL_BONE,	 nema_bone_index );
					break;
				case 1:
					bt.Translate( BoneTranslator::BODY_ANTERIOR, nema_bone_index );
					break;
				case 2:
					bt.Translate( BoneTranslator::BODY_MID,		 nema_bone_index );
					break;
				}
			}

			if( -1 == nema_bone_index )
			{
				nema_bone_index = 0;
			}

			pParent_aspect->GetIndexedBoneOrientation( nema_bone_index, temp_position, temp_rotation );

			// Need to compensate for the scale of the parent in the position of one of its bones -- biddle
			temp_position *= parent_render_scale;

			const float dist2 = Length2( test_position - temp_position );

			if( dist2 < min_dist )
			{
				min_dist = dist2;
				closest_bone_index = nema_bone_index;
				bone_position = temp_position;
				bone_rotation = temp_rotation;
			}

			if( (pParent_aspect->GetNumBones() <= 1) && ( 0 == nema_bone_index ) )
			{
				bSingleBone = true;
				break;
			}
			else
			{
				bSingleBone = false;
			}
		}
	}

	if( bSingleBone )
	{
		pParent_aspect->GetPrimaryBoneOrientation( bone_position, bone_rotation );
	}

	bone_position = pParent->GetPlacement()->GetPosition().pos +
					pParent->GetPlacement()->GetOrientation().RotateVector_T( bone_position );

	bone_rotation = pParent->GetPlacement()->GetOrientation() * bone_rotation;

	return closest_bone_index;
}


inline bool
Sim :: FindObject( SimObjColl &coll, SimID id, SimObjColl::iterator &object, bool bBoth, SimObjColl **pCollToEraseFrom )
{
	SimObjColl::iterator current = coll.begin(), end = coll.end();

	// First look through the active list of simulated objects
	for(; current != end; ++current )
	{
		if( (*current)->m_ID == id )
		{
			object = current;

			if( pCollToEraseFrom != NULL )
			{
				*pCollToEraseFrom = &coll;
			}
			return true;
		}
	}

	if( bBoth )
	{
		// If we didn't find it in the active list then it may have not got
		// a chance to get inserted and simulated yet so take a look in the
		// insertion list for it there
		current = m_ObjInsertionQueue.begin();
		end = m_ObjInsertionQueue.end();

		for(; current != end; ++current )
		{
			if( (*current)->m_ID == id )
			{
				object = current;

				if( pCollToEraseFrom != NULL )
				{
					*pCollToEraseFrom = &m_ObjInsertionQueue;
				}
				return true;
			}
		}
	}
	return false;
}


inline bool
Sim :: FindObject( Goid object_id, SimObj **object )
{
	SimObjColl::iterator current = m_Objects.begin(), end = m_Objects.end();

	// First look through the active list of simulated objects
	for(; current != end; ++current )
	{
		if( (*current)->m_ObjectID == object_id )
		{
			*object = *current;
			return true;
		}
	}

	// If we didn't find it in the active list then it may have not got
	// a chance to get inserted and simulated yet so take a look in the
	// insertion list for it there
	current = m_ObjInsertionQueue.begin();
	end = m_ObjInsertionQueue.end();

	for(; current != end; ++current )
	{
		if( (*current)->m_ObjectID == object_id )
		{
			*object = *current;
			return true;
		}
	}
	return false;
}


inline bool
Sim :: FindExplosion( SimExpColl &coll, SimID id, SimExpColl::iterator &explosion, bool bBoth )
{
	SimExpColl::iterator current = coll.begin(), end = coll.end();

	// First look through the active list of simulated objects
	for(; current != end; ++current )
	{
		if( (*current)->m_ID == id )
		{
			explosion = current;
			return true;
		}
	}

	if( bBoth )
	{
		// If we didn't find it in the active list then it may have not got
		// a chance to get inserted and simulated yet so take a look in the
		// insertion list for it there
		current = m_ExpInsertionQueue.begin(), end = m_ExpInsertionQueue.end();

		for(; current != end; ++current )
		{
			if( (*current)->m_ID == id )
			{
				explosion = current;
				return true;
			}
		}
	}
	return false;
}


inline bool
Sim :: FindDamageVolume( SimDmgColl &coll, SimID id, SimDmgColl::iterator &dvolume, bool bBoth )
{
	SimDmgColl::iterator current = coll.begin(), end = coll.end();

	// First look through the active list of simulated objects
	for(; current != end; ++current )
	{
		if( (*current)->m_ID == id )
		{
			dvolume = current;
			return true;
		}
	}

	if( bBoth )
	{
		// If we didn't find it in the active list then it may have not got
		// a chance to get inserted and simulated yet so take a look in the
		// insertion list for it there
		current = m_SimDmgInsQueue.begin(), end = m_SimDmgInsQueue.end();

		for(; current != end; ++current )
		{
			if( (*current)->m_ID == id )
			{
				dvolume = current;
				return true;
			}
		}
	}
	return false;
}


inline void
Sim :: GetNodeInfo( const siege::database_guid &node, matrix_3x3 &orient, vector_3 &center )
{
	siege::cache<siege::SiegeNode> &node_cache = gSiegeEngine.NodeCache();

	const siege::SiegeNodeHandle handle(node_cache.UseObject(node));
	const siege::SiegeNode& locate_node = handle.RequestObject( node_cache );

	center = locate_node.GetCurrentCenter();
	orient = locate_node.GetCurrentOrientation();
}


inline vector_3
Sim :: GetHalfDiag( Goid id )
{
	GoHandle hObject( id );
	vector_3 half_diag( DoNotInitialize );

	if( hObject )
	{
		hObject->GetAspect()->GetHalfDiagonal( half_diag );
	}
	return half_diag;
}


FUBI_DECLARE_SELF_TRAITS( SimObj );
FUBI_DECLARE_XFER_TRAITS( SimObj* )
{
	static bool XferDef( FuBi::PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		if ( persist.IsRestoring() )
		{
			obj = new SimObj;
		}

		return ( obj->Xfer( persist ) );
	}
};


FUBI_DECLARE_SELF_TRAITS( SimExp );
FUBI_DECLARE_XFER_TRAITS( SimExp* )
{
	static bool XferDef( FuBi::PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		if ( persist.IsRestoring() )
		{
			obj = new SimExp;
		}

		return ( obj->Xfer( persist ) );
	}
};


FUBI_DECLARE_SELF_TRAITS( SimDmg );
FUBI_DECLARE_XFER_TRAITS( SimDmg* )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		if ( persist.IsRestoring() )
		{
			obj = new SimDmg;
		}

		return ( obj->Xfer( persist ) );
	}
};


FUBI_DECLARE_SELF_TRAITS( Sim :: Explosion );

bool
Sim :: Explosion::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "m_Object",		m_Object	);
	persist.Xfer( "m_magnitude",	m_magnitude );
	persist.Xfer( "m_delay",		m_delay		);
	persist.Xfer( "m_spawned",		m_spawned	);

	return true;
}
