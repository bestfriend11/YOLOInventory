//
//	Flamethrower_interpreter_macros.h
//
//	Herin contains all of the macros used in the Flamethrower
//	interpreter
//
#include "precomp_flamethrower.h"
#include "gpcore.h"

#include "Flamethrower_interpreter_macros.h"
#include "Flamethrower_tracker.h"

#include "stringtool.h"

#include "siege_pos.h"
#include "Go.h"
#include "GoInventory.h"
#include "GoAttack.h"



#undef ERROR
#define ERROR( ERROR_MSG )	\
	gperrorf(("SiegeFx: error - script \"%s\" %s\n", pScript->GetName().c_str(), ERROR_MSG )); pScript->Stop();


bool
FM_nop::Execute( Flamethrower_script *pScript )
{
	UNREFERENCED_PARAMETER( pScript );

	return false;
}


bool
FM_pop::Execute( Flamethrower_script *pScript )
{
	Flamethrower_script::def_stack & stack = pScript->GetStack();

	if( !stack.empty() )
	{
		Flamethrower_script::def_stack::iterator it = stack.end();
		--it;

		pScript->GetReturnValue().Clear();
		pScript->GetReturnValue() = *it;

		stack.erase( it );

		return true;
	}
	else
	{
		ERROR( "#POP does not operate on an empty stack" );
	}

	return false;
}


bool
FM_peek::Execute( Flamethrower_script *pScript )
{
	Flamethrower_script::def_stack & stack = pScript->GetStack();

	if( !stack.empty() )
	{
		Flamethrower_script::def_stack::iterator it = stack.end();
		--it;

		pScript->GetReturnValue().Clear();
		pScript->GetReturnValue() = *it;

		return true;
	}
	else
	{
		ERROR( "#PEEK does not operate on an empty stack" );
	}


	return false;
}


bool
FM_true::Execute( Flamethrower_script *pScript )
{
	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( 1.0f );

	return true;
}


bool
FM_false::Execute( Flamethrower_script *pScript )
{
	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( 0.0f );

	return true;
}


bool
FM_null_target::Execute( Flamethrower_script *pScript )
{
	def_tracker null_tracker = WorldFx::MakeTracker();

	Flamethrower_params & rv = pScript->GetReturnValue();

	rv.Clear();
	rv.Add( pScript->AddTracker( null_tracker ) );
	rv.SetType( 0, Flamethrower_params::PT_TRACKER );

	return true;
}


bool
FM_source::Execute( Flamethrower_script *pScript )
{
	Flamethrower_target *pOriginal = NULL, *pDuplicate = NULL;
	Flamethrower_params & rv = pScript->GetReturnValue();

	if( pScript->GetDefaultTargets()->GetTarget( TattooTracker::SOURCE, &pOriginal ) )
	{
		pOriginal->Duplicate( &pDuplicate );
		if( pDuplicate != NULL )
		{
			def_tracker new_tracker = WorldFx::MakeTracker();
			new_tracker->AddTarget( pDuplicate );

			rv.Clear();
			rv.SetType( rv.Add( pScript->AddTracker( new_tracker ) ), Flamethrower_params::PT_TRACKER );

			return true;
		}
	}

	pScript->Stop();

	rv.Clear();
	rv.SetType( rv.Add( -1 ), Flamethrower_params::PT_TRACKER );

	return true;
}


bool
FM_target::Execute( Flamethrower_script *pScript )
{
	Flamethrower_target *pOriginal = NULL, *pDuplicate = NULL;
	Flamethrower_params & rv = pScript->GetReturnValue();

	if( pScript->GetDefaultTargets()->GetTarget( TattooTracker::TARGET, &pOriginal ) )
	{
		pOriginal->Duplicate( &pDuplicate );
		if( pDuplicate != NULL )
		{
			def_tracker new_tracker = WorldFx::MakeTracker();
			new_tracker->AddTarget( pDuplicate );

			rv.Clear();
			rv.SetType( rv.Add( pScript->AddTracker( new_tracker ) ), Flamethrower_params::PT_TRACKER );

			return true;
		}
	}

	pScript->Stop();

	rv.Clear();
	rv.SetType( rv.Add( -1 ), Flamethrower_params::PT_TRACKER );

	return true;
}


bool
FM_source_kb::Execute( Flamethrower_script *pScript )
{
	Flamethrower_target *pOriginal = NULL, *pDuplicate = NULL;
	Flamethrower_params & rv = pScript->GetReturnValue();

	if( pScript->GetDefaultTargets()->GetTarget( TattooTracker::SOURCE, &pOriginal ) )
	{
		pOriginal->Duplicate( &pDuplicate );
		if( pDuplicate != NULL )
		{
			if( pDuplicate->HasPositionOffsetName( "@kill_bone" ) )
			{
				pDuplicate->SetPositionOffsetName( "@kill_bone" );
			}

			def_tracker new_tracker = WorldFx::MakeTracker();
			new_tracker->AddTarget( pDuplicate );

			rv.Clear();
			rv.SetType( rv.Add( pScript->AddTracker( new_tracker ) ), Flamethrower_params::PT_TRACKER );

			return true;
		}
	}

	pScript->Stop();

	rv.Clear();
	rv.SetType( rv.Add( -1 ), Flamethrower_params::PT_TRACKER );

	return true;
}


bool
FM_target_kb::Execute( Flamethrower_script *pScript )
{
	Flamethrower_target *pOriginal = NULL, *pDuplicate = NULL;
	Flamethrower_params & rv = pScript->GetReturnValue();

	if( pScript->GetDefaultTargets()->GetTarget( TattooTracker::TARGET, &pOriginal ) )
	{
		pOriginal->Duplicate( &pDuplicate );
		if( pDuplicate != NULL )
		{
			if( pDuplicate->HasPositionOffsetName( "@kill_bone" ) )
			{
				pDuplicate->SetPositionOffsetName( "@kill_bone" );
			}

			def_tracker new_tracker = WorldFx::MakeTracker();
			new_tracker->AddTarget( pDuplicate );

			rv.Clear();
			rv.SetType( rv.Add( pScript->AddTracker( new_tracker ) ), Flamethrower_params::PT_TRACKER );

			return true;
		}
	}

	pScript->Stop();

	rv.Clear();
	rv.SetType( rv.Add( -1 ), Flamethrower_params::PT_TRACKER );

	return true;
}


bool
FM_default_timeout::Execute( Flamethrower_script *pScript )
{
	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( 15.0f );

	return true;
}


bool
FM_no_collision::Execute( Flamethrower_script *pScript )
{
	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( (float)Flamethrower_script::NO_COLLISION );

	return true;
}


bool
FM_object_collision::Execute( Flamethrower_script *pScript )
{
	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( (float)Flamethrower_script::TARGET_COLLISION );

	return true;
}


bool
FM_terrain_collision::Execute( Flamethrower_script *pScript )
{
	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( (float)Flamethrower_script::TERRAIN_COLLISION );

	return true;
}


bool
FM_source_position::Execute( Flamethrower_script *pScript )
{
	TattooTracker *pTracker = pScript->GetDefaultTargets();

	// if no targets, a legal scenario, then end the script
	if( !pTracker->AtLeastOneTarget() )
	{
		pScript->Stop();
		return false;
	}

	SiegePos	position;
	pTracker->GetPlacement( &position, NULL, false, TattooTracker::SOURCE );

	// always return a position even if fails to retrieve one - sound will handle invalid position
	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( position );

	return true;
}


bool
FM_target_position::Execute( Flamethrower_script *pScript )
{
	TattooTracker *pTracker = pScript->GetDefaultTargets();

	// if no targets, a legal scenario, then end the script
	if( !pTracker->AtLeastOneTarget() )
	{
		pScript->Stop();
		return false;
	}

	SiegePos	position;

	pTracker->GetPlacement( &position, NULL, false, TattooTracker::TARGET );

	// always return a position even if fails to retrieve one - sound will handle invalid position
	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( position );

	return true;
}


bool
FM_camera_position::Execute( Flamethrower_script *pScript )
{
	// $ This doesn't seem right - find out if it is correct to not set the node
	SiegePos position;
	position.pos = gWorldFx.GetCameraPosition();

	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( position );

	return true;
}


bool
FM_target_goid::Execute( Flamethrower_script *pScript )
{
	TattooTracker *pTracker = pScript->GetDefaultTargets();

	Goid goid = pTracker->GetID( TattooTracker::TARGET );

	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( (UINT32)goid );

	return true;
}


bool
FM_source_goid::Execute( Flamethrower_script *pScript )
{
	TattooTracker *pTracker = pScript->GetDefaultTargets();

	Goid goid = pTracker->GetID( TattooTracker::SOURCE );

	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( (UINT32)goid );

	return true;
}


bool
FM_owner_goid::Execute( Flamethrower_script *pScript )
{
	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( (UINT32)pScript->GetOwner() );

	return true;
}


bool
FM_invalid_goid::Execute( Flamethrower_script *pScript )
{
	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( (UINT32)GOID_INVALID );

	return true;
}


bool
FM_weapon_hand_name::Execute( Flamethrower_script *pScript )
{
	if( pScript->GetOwner() != GOID_INVALID )
	{
		GoHandle hOwner( pScript->GetOwner() );
		if( hOwner && hOwner->HasActor() && hOwner->HasInventory() )
		{
			Go *pGo = hOwner->GetInventory()->GetEquipped( ES_WEAPON_HAND );

			if( pGo )
			{
				pScript->GetReturnValue().Clear();
				pScript->GetReturnValue().Add( pGo->GetTemplateName() );

				return true;
			}
			else
			{
				pGo = hOwner->GetInventory()->GetEquipped( ES_SHIELD_HAND );

				if( pGo )
				{
					pScript->GetReturnValue().Clear();
					pScript->GetReturnValue().Add( pGo->GetTemplateName() );

					return true;
				}
			}
		}
	}
	return false;
}


bool
FM_shield_hand_name::Execute( Flamethrower_script *pScript )
{
	if( pScript->GetOwner() != GOID_INVALID )
	{
		GoHandle hOwner( pScript->GetOwner() );
		if( hOwner && hOwner->HasActor() && hOwner->HasInventory() )
		{
			Go *pGo = hOwner->GetInventory()->GetEquipped( ES_SHIELD_HAND );

			if( pGo )
			{
				pScript->GetReturnValue().Clear();
				pScript->GetReturnValue().Add( pGo->GetTemplateName() );

				return true;
			}
			else
			{
				pGo = hOwner->GetInventory()->GetEquipped( ES_WEAPON_HAND );

				if( pGo )
				{
					pScript->GetReturnValue().Clear();
					pScript->GetReturnValue().Add( pGo->GetTemplateName() );

					return true;
				}
			}
		}
	}
	return false;
}


bool
FM_caster_name::Execute( Flamethrower_script *pScript )
{
	if( pScript->GetOwner() != GOID_INVALID )
	{
		GoHandle hOwner( pScript->GetOwner() );

		if( hOwner )
		{
			pScript->GetReturnValue().Clear();
			pScript->GetReturnValue().Add( hOwner->GetTemplateName() );

			return true;
		}
	}
	return false;
}


bool
FM_attack_class::Execute( Flamethrower_script *pScript )
{
	if( pScript->GetOwner() != GOID_INVALID )
	{
		GoHandle hOwner( pScript->GetOwner() );
		if( hOwner && hOwner->HasActor() && hOwner->HasInventory() )
		{
			Go *pGo = hOwner->GetInventory()->GetEquipped( ES_WEAPON_HAND );

			if( pGo && pGo->HasAttack() )
			{
				eAttackClass ac = pGo->GetAttack()->GetAttackClass();

				pScript->GetReturnValue().Clear();
				pScript->GetReturnValue().Add( (float)ac );

				return true;
			}
			else
			{
				pGo = hOwner->GetInventory()->GetEquipped( ES_SHIELD_HAND );

				if( pGo && pGo->HasAttack() )
				{
					eAttackClass ac = pGo->GetAttack()->GetAttackClass();

					pScript->GetReturnValue().Clear();
					pScript->GetReturnValue().Add( (float)ac );

					return true;
				}
			}
		}
	}
	return false;
}


bool
FM_weapon_type_dagger::Execute( Flamethrower_script *pScript )
{
	if( pScript->GetOwner() != GOID_INVALID )
	{
		GoHandle hTarget( pScript->GetOwner() );

		if( hTarget && hTarget->HasAttack() )
		{
			eAttackClass ac = hTarget->GetAttack()->GetAttackClass();

			if( ac == AC_DAGGER )
			{
				pScript->GetReturnValue().Clear();
				pScript->GetReturnValue().Add( 1.0f );

				return true;
			}
		}
	}

	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( 0.0f );

	return true;
}


bool
FM_weapon_type_axe::Execute( Flamethrower_script *pScript )
{
	if( pScript->GetOwner() != GOID_INVALID )
	{
		GoHandle hTarget( pScript->GetOwner() );

		if( hTarget && hTarget->HasAttack() )
		{
			eAttackClass ac = hTarget->GetAttack()->GetAttackClass();

			if( ac == AC_AXE )
			{
				pScript->GetReturnValue().Clear();
				pScript->GetReturnValue().Add( 1.0f );

				return true;
			}
		}
	}

	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( 0.0f );

	return true;
}


bool
FM_weapon_type_blunt::Execute( Flamethrower_script *pScript )
{
	if( pScript->GetOwner() != GOID_INVALID )
	{
		GoHandle hTarget( pScript->GetOwner() );

		if( hTarget && hTarget->HasAttack() )
		{
			eAttackClass ac = hTarget->GetAttack()->GetAttackClass();

			if( (ac == AC_CLUB) || (ac == AC_HAMMER) || (ac == AC_MACE) )
			{
				pScript->GetReturnValue().Clear();
				pScript->GetReturnValue().Add( 1.0f );

				return true;
			}
		}
	}

	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( 0.0f );

	return true;
}


bool
FM_weapon_type_staff::Execute( Flamethrower_script *pScript )
{
	if( pScript->GetOwner() != GOID_INVALID )
	{
		GoHandle hTarget( pScript->GetOwner() );

		if( hTarget && hTarget->HasAttack() )
		{
			eAttackClass ac = hTarget->GetAttack()->GetAttackClass();

			if( ac == AC_STAFF )
			{
				pScript->GetReturnValue().Clear();
				pScript->GetReturnValue().Add( 1.0f );

				return true;
			}
		}
	}

	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( 0.0f );

	return true;
}


bool
FM_weapon_type_sword::Execute( Flamethrower_script *pScript )
{
	if( pScript->GetOwner() != GOID_INVALID )
	{
		GoHandle hTarget( pScript->GetOwner() );

		if( hTarget && hTarget->HasAttack() )
		{
			eAttackClass ac = hTarget->GetAttack()->GetAttackClass();

			if( ac == AC_SWORD )
			{
				pScript->GetReturnValue().Clear();
				pScript->GetReturnValue().Add( 1.0f );

				return true;
			}
		}
	}

	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( 0.0f );

	return true;
}


bool
FM_weapon_type_bow::Execute( Flamethrower_script *pScript )
{
	if( pScript->GetOwner() != GOID_INVALID )
	{
		GoHandle hTarget( pScript->GetOwner() );

		if( hTarget && hTarget->HasAttack() )
		{
			eAttackClass ac = hTarget->GetAttack()->GetAttackClass();

			if( ac == AC_BOW )
			{
				pScript->GetReturnValue().Clear();
				pScript->GetReturnValue().Add( 1.0f );

				return true;
			}
		}
	}

	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( 0.0f );

	return true;
}


bool
FM_weapon_type_minigun::Execute( Flamethrower_script *pScript )
{
	if( pScript->GetOwner() != GOID_INVALID )
	{
		GoHandle hTarget( pScript->GetOwner() );

		if( hTarget && hTarget->HasAttack() )
		{
			eAttackClass ac = hTarget->GetAttack()->GetAttackClass();

			if( ac == AC_MINIGUN )
			{
				pScript->GetReturnValue().Clear();
				pScript->GetReturnValue().Add( 1.0f );

				return true;
			}
		}
	}

	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( 0.0f );

	return true;
}


bool
FM_weapon_type_projectile::Execute( Flamethrower_script *pScript )
{
	if( pScript->GetOwner() != GOID_INVALID )
	{
		GoHandle hTarget( pScript->GetOwner() );

		if( hTarget && hTarget->HasAttack() )
		{
			eAttackClass ac = hTarget->GetAttack()->GetAttackClass();

			if( (ac == AC_ARROW) || (ac == AC_BOLT) )
			{
				pScript->GetReturnValue().Clear();
				pScript->GetReturnValue().Add( 1.0f );

				return true;
			}
		}
	}

	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( 0.0f );

	return true;
}


bool
FM_lodfi::Execute( Flamethrower_script *pScript )
{
	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( gWorldFx.GetDetailLevel() );

	return true;
}


bool
FM_red_blood::Execute( Flamethrower_script *pScript )
{
	if( gWorldFx.IsBloodRed() )
	{
		pScript->GetReturnValue().Clear();
		pScript->GetReturnValue().Add( 1.0f );

		return true;
	}

	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( 0.0f );

	return true;
}


bool
FM_no_blood::Execute( Flamethrower_script *pScript )
{
	if( !gWorldFx.IsBloodEnabled() )
	{
		pScript->GetReturnValue().Clear();
		pScript->GetReturnValue().Add( 1.0f );

		return true;
	}

	pScript->GetReturnValue().Clear();
	pScript->GetReturnValue().Add( 0.0f );

	return true;
}
