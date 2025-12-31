//
//	Flamethrower_interpreter_commands
//
//	Herein contains recognized interpreter commands
//

#include "precomp_flamethrower.h"
#include "gpcore.h"

#include <string>
#include <map>
#include <vector>

#include "flamethrower_effect_base.h"
#include "flamethrower_interpreter_commands.h"
#include "flamethrower_interpreter.h"
#include "flamethrower_tracker.h"

#include "Go.h"
#include "GoSupport.h"

#include "gps_manager.h"

#include "siege_database_guid.h"
#include "siege_pos.h"
#include "gomind.h"
#include "server.h"	// for getting the party
#include "player.h"
#include "worldtime.h"



#define DEFINE_PARAM_CONVERTERS( CMD )	\
	COMPILER_ASSERT( ELEMENT_COUNT( s_##CMD##ParamStrings ) == FC_##CMD##::PARAM_COUNT );	\
	static stringtool::EnumStringConverter s_##CMD##ParamConverter( s_##CMD##ParamStrings, FC_##CMD##::PARAM_BEGIN, FC_##CMD##::PARAM_END );	\
	const char* ToString( FC_##CMD##::e##CMD##Param b ) {  return ( s_##CMD##ParamConverter.ToString( b ) );  }	\
	bool FromString( const char* str, FC_##CMD##::e##CMD##Param & b )	{  return ( s_##CMD##ParamConverter.FromString( str, rcast <int&> ( b ) ) );  } \
	bool FC_##CMD##::GetParamToken( const char * sParam, UINT32 & token ){ FC_##CMD##::e##CMD##Param Token; if( FromString( sParam, Token ) ) \
	{ token = Token; return true; } return false; }

#undef ERROR_IF
#define ERROR_IF( EXPRESSION, ERROR_MSG ) \
	if( EXPRESSION ) { gperrorf(("SiegeFx: error - script \"%s\" %s\n", pScript->GetName().c_str(), ERROR_MSG )); \
	pScript->Stop(); return false; }
#undef ERROR
#define ERROR( ERROR_MSG ) \
	gperrorf(("SiegeFx: error - script \"%s\" %s\n", pScript->GetName().c_str(), ERROR_MSG )); pScript->Stop(); return false;

#undef RETURN
#define RETURN( VAL ) \
	Flamethrower_params * new_return_params = &*pScript->GetStack().push_back(); new_return_params->Add( VAL ); return true;



bool
FC_Call::Execute( Flamethrower_script *pScript, Flamethrower_params & params )
{
	ERROR_IF( (params.Count() < 1), "call requires at least one parameter" );
	ERROR_IF( (params.GetType(0) != Flamethrower_params::PT_STRING), "call param 1 must ba a string" );

	TattooTracker *pTracker = pScript->GetDefaultTargets();

	def_tracker Tracker( pTracker->Duplicate() );
	
	gWorldFx.GetRNG().SetSeed( pScript->GetSeed() );
	pScript->SetSeed( gWorldFx.GetRNG().RandomDword() );

	UINT32 script_id = (UINT32)gFlamethrower_interpreter.RunScript( params._gpstring(0), Tracker, params, 1, pScript->GetOwner(), pScript->GetSeed(), WE_CONSTRUCTED, false );

	RETURN( script_id );
}


bool
FC_Camerashake::Execute( Flamethrower_script *pScript, Flamethrower_params & params )
{
	ERROR_IF( ( params.Count() != 2 ), "camerashake requires 2 parameters" );
	ERROR_IF( ( params.GetType( 0 ) != Flamethrower_params::PT_STRING ), "camerashake param 1 must be a string" );
	ERROR_IF( ( params.GetType( 1 ) != Flamethrower_params::PT_STRING ), "camerashake param 2 must be a string" );

	StartCameraFx( params._gpstring( 0 ), params._gpstring( 1 ) );

	return true;
}


bool
FC_Exit::Execute( Flamethrower_script *pScript, Flamethrower_params & params )
{
	UNREFERENCED_PARAMETER( params );

	pScript->Stop();
	return false;
}


bool
FC_Frandrange::Execute( Flamethrower_script *pScript, Flamethrower_params & params )
{
	ERROR_IF( (params.Count() != 2 ), "frandrange requires 2 float parameters" );

	// cast and complain
	ERROR_IF( (!params.SetType(0, Flamethrower_params::PT_FLOAT)), "frandrange param 1 must be a float" );
	ERROR_IF( (!params.SetType(1, Flamethrower_params::PT_FLOAT)), "frandrange param 1 must be a float" );


	gWorldFx.GetRNG().SetSeed( pScript->GetSeed() );
	pScript->SetSeed( gWorldFx.GetRNG().RandomDword() );

	float number = gWorldFx.GetRNG().Random( params._float( 0 ), params._float( 1 ) );

	RETURN( number );
}


static const char * s_GetParamStrings[] =
{
	"collision",
	"target_position",
	"id",
	"target",
	"source",
	"point",
	"direction",
};

DEFINE_PARAM_CONVERTERS( Get );

bool
FC_Get::Execute( Flamethrower_script *pScript, Flamethrower_params & params )
{
	ERROR_IF( (params.GetType(0) != Flamethrower_params::PT_COMMAND_PARAM), "get param 1 must be a command param" );
	ERROR_IF( (params.Count() < 3), "get doesn't have enough parameters" );
	ERROR_IF( (params.Count() > 4), "get specifying too many parameters" );

	if( params._cparam(0) == (UINT32)GP_COLLISION )
	{
		ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_COMMAND_PARAM), "get param 2 must be a command param" );

		switch( (eGetParam)params._cparam(1) )
		{
			case GP_ID:
			{
				ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_UINT32), "get collision id must be a uint" );

				Goid target_guid = GOID_INVALID;
				gWorldFx.GetCollisionTargetID( (SFxEID)params._cparam(2), target_guid );

				RETURN( (UINT32)target_guid );
			}

			case GP_TARGET:
			{
				ERROR_IF( (params.Count() < 4), "get collision target is missing params" );
				ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_UINT32), "get collision target must be a uint" );
				ERROR_IF( (params.GetType(3) != Flamethrower_params::PT_COMMAND_PARAM ), "get collision target param 2 must be a command param" );

				TattooTracker *pTracker = pScript->GetDefaultTargets();

				Flamethrower_target *pOldTarget = NULL;
				TattooTracker::TARGET_INDEX ti = (params._cparam(3) == (UINT32)GP_TARGET)?TattooTracker::TARGET:TattooTracker::SOURCE;

				if( !pTracker->GetTarget( ti, &pOldTarget ) )
				{
					pScript->Stop();
					return false;
				}

				Goid collided_target;

				if( gWorldFx.GetCollisionTargetID( (SFxEID)params._uint32(2), collided_target ) )
				{
					if( MakeGoid( pOldTarget->GetID() ) == collided_target )
					{
						return true;
					}
					else
					{
						Flamethrower_target *pNewTarget = NULL;

						if( pOldTarget->Duplicate( &pNewTarget ) )
						{
							pNewTarget->SetID( MakeInt( collided_target ) );
							
							pTracker->SetTarget( pNewTarget, ti );
							return true;
						}
						else
						{
							pScript->Stop();
							return false;
						}
					}
				}
				// unable to get the collision target, end this script
				pScript->Stop();
				return false;
			}

			case GP_POINT:
			{
				ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_UINT32), "get collision point must be a uint" );

				SiegePos	ContactPoint, NormalPoint;

				gWorldFx.GetCollisionPoint( (SFxEID)params._uint32(2), ContactPoint );
				gWorldFx.GetCollisionNormal( (SFxEID)params._uint32(2), NormalPoint );

				if( gSiegeEngine.IsNodeValid( ContactPoint.node ) )
				{
					Flamethrower_target *pTarget = Flamethrower_target::CreateByType( Flamethrower_target::STATIC, ContactPoint );
					pTarget->SetDirection( NormalPoint.pos );
					pTarget->SetBoundingRadius( 1.25f );

					def_tracker pTracker( WorldFx::MakeTracker() );

					int number = pScript->AddTracker( pTracker );

					pTracker->RemoveAllTargets();
					pTracker->AddTarget( pTarget );

					Flamethrower_params * return_value = &*pScript->GetStack().push_back();
					return_value->Add( number );
					return_value->SetType( 0, Flamethrower_params::PT_TRACKER );

					return true;
				}

				// the contact point is not valid so stop the script
				pScript->Stop();
				return false;
			}

			case GP_DIRECTION:
			{
				ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_UINT32), "get collision direction must be a uint" );

				vector_3	Direction;
				SiegePos	NormalPoint;

				gWorldFx.GetCollisionNormal( (SFxEID)params._uint32(2), NormalPoint );

				if( 0.25f <= (NormalPoint.pos.x + NormalPoint.pos.y + NormalPoint.pos.z) )
				{
					Direction = NormalPoint.pos;
				}
				else
				{
					gWorldFx.GetCollisionDirection( (SFxEID)params._uint32(2), Direction );
					Direction = -Direction;
				}

				Flamethrower_params * return_value = &*pScript->GetStack().push_back();
				return_value->Add( Direction );
				return true;
			}

			default:
			{
				ERROR( "get collision param 1 is unknown" );
			}
		}
	}
	else if( params._cparam(0) == (UINT32)GP_TARGET_POSITION )
	{
		ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "sfx get target_position param 1 must be a uint" );
		ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_COMMAND_PARAM), "sfx get target_position param 2 must be a command param" );

		SpecialEffect *pEffect = gWorldFx.FindEffect( (SFxEID)params._uint32(1) );

		if( pEffect )
		{	
			SiegePos position;

			TattooTracker::TARGET_INDEX ti = ( params._cparam(2) == (UINT32)GP_TARGET )?TattooTracker::TARGET:TattooTracker::SOURCE;
			
			position = pEffect->GetTargetPosition( ti );

			RETURN( position );
		}
	}

	ERROR( "get param 1 is invalid" );
}


static const char * s_ModeParamStrings[] =
{
	"pause_immune",
	"pause_affected",
};

DEFINE_PARAM_CONVERTERS( Mode );

bool
FC_Mode::Execute( Flamethrower_script *pScript, Flamethrower_params & params )
{
	ERROR_IF( (params.GetType(0) != Flamethrower_params::PT_COMMAND_PARAM), "mode parameter unknown" );

	pScript->SetPauseImmune( ( params._cparam(0) == (UINT32)MP_PAUSE_IMMUNE ) );

	return true;
}


bool
FC_Pause::Execute( Flamethrower_script *pScript, Flamethrower_params & params )
{
	ERROR_IF( (params.Count() < 1), "pause is missing a parameter" );
	ERROR_IF( !params.SetType( 0, Flamethrower_params::PT_FLOAT ), "pause param 1 must be a float" );

	// initialize if needed
	if( params.Count() == 1 )
	{
		params.Add( params._float(0) );
	}

	// check if it's time to reset and exit
	if( params._float(1) <= 0 )
	{
		float time = params._float(0);
		params.Clear();
		params.Add( time );

		return true;
	}

	params._float(1) -= gFlamethrower.GetInterpreter().GetSecondsElapsed();

	return false;
}


bool
FC_Randrange::Execute( Flamethrower_script *pScript, Flamethrower_params & params )
{
	ERROR_IF( (params.Count() < 2), "randrange missing parameters" );
	
	Flamethrower_params::PARAM_TYPE pt0 = params.GetType(0);
	Flamethrower_params::PARAM_TYPE pt1 = params.GetType(1);

	ERROR_IF( !((pt0 == Flamethrower_params::PT_INT) || (pt0 == Flamethrower_params::PT_UINT32) ), "randrange param 1 must be an int or uint" );
	ERROR_IF( !((pt1 == Flamethrower_params::PT_INT) || (pt1 == Flamethrower_params::PT_UINT32) ), "randrange param 2 must be an int or uint" );

	gWorldFx.GetRNG().SetSeed( pScript->GetSeed() );
	pScript->SetSeed( gWorldFx.GetRNG().RandomDword() );

	float min = 0, max = 0;

	if( pt0 == Flamethrower_params::PT_INT )
	{
		min = (float)params._int(0);
	}
	else if( pt0 == Flamethrower_params::PT_UINT32 )
	{
		min = (float)params._uint32(0);
	}

	if( pt1 == Flamethrower_params::PT_INT )
	{
		max = (float)params._int(1);
	}
	else if( pt1 == Flamethrower_params::PT_UINT32 )
	{
		max = (float)params._uint32(1);
	}

	int number = (int)gWorldFx.GetRNG().Random( min, max );
	
	RETURN( number );
}


bool
FC_Set::Execute( Flamethrower_script *pScript, Flamethrower_params & params )
{
	ERROR_IF( (params.Count() < 2), "set is missing parameters" );
	ERROR_IF( (params.GetType(0) != Flamethrower_params::PT_VARIABLE), "set parameter 1 must start with a $" );

	Flamethrower_params new_params;

	for( UINT32 n = 0, end = params.Count()-1; n != end; ++n )
	{
		new_params.Add( n );
		new_params.Replace( n, n+1, params );
	}

	pScript->GetSymbols().insert( std::make_pair( params._variable(0), new_params ) );

	return true;
}


static const char * s_SfxParamStrings[] =
{
	"all",
	"target",
	"source",

	"create",
	"destroy",

	"start",
	"finish",
	"stop",

	"freeze_targets",
	"snap_to_ground",
	"rat",
	"attach",
	"attach_point",
	"position",
	"position_at",
	"offset",
	"offset_bone",
	"direction",

	"friendly",
	"party",
};

DEFINE_PARAM_CONVERTERS( Sfx );

bool
FC_Sfx::Execute( Flamethrower_script *pScript, Flamethrower_params & params )
{
	ERROR_IF( (params.GetType(0) != Flamethrower_params::PT_COMMAND_PARAM), "sfx param 1 must be an sfx parameter" );

	switch( params._cparam(0) )
	{
		case SP_CREATE:
		{
			ERROR_IF( (params.Count() < 3), "sfx create is missing a parameter" );
			ERROR_IF( (params.Count() > 4), "sfx create has too many parameters" );
			ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_TRACKER), "sfx create param 3 must be a tracker" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_STRING), "sfx create param 2 must be a string" );

			if( params._tracker(2) == -1 )
			{
				// target is gone - just exit silently
				pScript->Stop();
				return false;
			}

			// click over the random seed
			gWorldFx.GetRNG().SetSeed( pScript->GetSeed() );
			pScript->SetSeed( gWorldFx.GetRNG().RandomDword() );

			SFxEID return_value = 0;
			def_tracker pTracker( pScript->ReleaseTracker( params._tracker(2) ) );

			gpstring & effect_params = ( params.Count() == 4 ) ? params._sfxparam(3) : gpstring::EMPTY;
			return_value = gWorldFx.CreateEffect( params._gpstring(1), effect_params, pTracker, pScript->GetOwner(), pScript->GetID(), pScript->GetSeed() );

			ERROR_IF( (SFxEID_INVALID == return_value), "sfx cannot create effect" );

			gWorldFx.SetInterestOnly( return_value, pTracker->HasInterestOnlyTarget() );

			pScript->GetEffectLog().push_back( return_value );

			RETURN( (UINT32)return_value );
		}

		case SP_START:
		{
			ERROR_IF( (params.Count() < 2), "sfx start missing parameter" );

			if( params.GetType(1) == Flamethrower_params::PT_UINT32 )
			{
				gWorldFx.StartEffect( (SFxEID)params._uint32(1) );
				return true;
			}
			else if( params.GetType(1) == Flamethrower_params::PT_COMMAND_PARAM )
			{
				if( params._cparam(1) == SP_ALL )
				{
					gWorldFx.StartAllEffects();
					return true;
				}
			}

			ERROR( "sfx start is using an invalid effect id type" );
		}

		case SP_TARGET:
		{
			ERROR_IF( (params.Count() < 3), "sfx target is missing parameters" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "sfx target param 1 must be a uint" );

			Flamethrower_params::PARAM_TYPE pt = params.GetType(2);

			ERROR_IF( !((pt == Flamethrower_params::PT_UINT32)||(pt == Flamethrower_params::PT_COMMAND_PARAM)), "sfx target param 2 is an invalid type" );

			if( pt == Flamethrower_params::PT_COMMAND_PARAM )
			{
				TattooTracker::TARGET_INDEX ti = TattooTracker::TARGET;

				if( params._cparam(2) == SP_SOURCE )
				{
					ti = TattooTracker::SOURCE;
				}

				Flamethrower_target *pTarget = NULL, *pNewTarget = NULL;

				if( !pScript->GetDefaultTargets()->GetTarget( ti, &pTarget ) )
				{
					pScript->Stop();
					return false;
				}

				if( pTarget->Duplicate( &pNewTarget ) )
				{
					gWorldFx.AddTarget( (SFxEID)params._uint32(1), pNewTarget );
					return true;
				}
				pScript->Stop();
			}

			gWorldFx.AddEffectTarget( (SFxEID)params._uint32(1), (SFxEID)params._uint32(2) );
			return true;
		}

		case SP_FREEZE_TARGETS:
		{
			ERROR_IF( (params.Count() < 2), "sfx freeze_targets missing params" );

			if( params.GetType(1) == Flamethrower_params::PT_COMMAND_PARAM )
			{
				if( params._cparam(1) == SP_ALL )
				{
					def_tracker def_targets = pScript->ReleaseDefaultTargets();
					def_tracker new_targets = gWorldFx.MakeTrackerTargetsStatic( def_targets, false );

					pScript->SetDefaultTargets( new_targets );
				}
			}
			else
			{
				ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "sfx freeze targets param 1 must be a uint" );
				ERROR_IF( !gWorldFx.MakeTargetsStatic( (SFxEID)params._uint32(1) ), "sfx freeze targets param cannot freeze targets" );
			}
			return true;
		}

		case SP_SNAP_TO_GROUND:
		{
			ERROR_IF( (params.Count() < 2), "sfx snap_to_ground missing params" );

			if( params.GetType(1) == Flamethrower_params::PT_COMMAND_PARAM )
			{
				if( params._cparam(1) == SP_ALL )
				{
					def_tracker def_targets = pScript->ReleaseDefaultTargets();
					def_tracker new_targets = gWorldFx.MakeTrackerTargetsStatic( def_targets, true );

					pScript->SetDefaultTargets( new_targets );
				}
			}
			else
			{
				ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "sfx snap_to_ground 1 must be a uint" );
				ERROR_IF( !gWorldFx.MakeTargetsStatic( (SFxEID)params._uint32(1), true ), "sfx snap_to_ground cannot make target static" );
			}
			return true;
		}

		case SP_RAT:
		{
			ERROR_IF( (params.Count() < 2), "sfx rat is missing parameters" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "sfx rat param 1 must be a uint" );

			gWorldFx.RemoveAllTargets( (SFxEID)params._uint32(1) );
			return true;
		}

		case SP_FINISH:
		{
			ERROR_IF( (params.Count() < 2), "sfx finish is missing parameters" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "sfx finish param 1 must be a uint" );

			gWorldFx.SetFinishing( (SFxEID)params._uint32(1) );
			return true;
		}

		case SP_STOP:
		{
			ERROR_IF( (params.Count() < 2), "sfx stop is missing a param" );

			if( (params.GetType(1) == Flamethrower_params::PT_COMMAND_PARAM) && ( params._cparam(1) == SP_ALL ) )
			{
				gWorldFx.StopAllEffects();
				return true;
			}
			else if( params.GetType(1) == Flamethrower_params::PT_UINT32 )
			{
				gWorldFx.StopEffect( (SFxEID)params._uint32(1) );
				return true;
			}

			ERROR( "sfx stop is specifying invalid param types" );
		}

		case SP_DESTROY:
		{
			ERROR_IF( (params.Count() < 2), "sfx destroy is missing a param" );

			if( (params.GetType(1) == Flamethrower_params::PT_COMMAND_PARAM) && ( params._cparam(1) == SP_ALL ) )
			{
				gWorldFx.DestroyAllEffects();
				return true;
			}
			else if( params.GetType(1) == Flamethrower_params::PT_UINT32 )
			{
				gWorldFx.DestroyEffect( (SFxEID)params._uint32(1) );
				return true;
			}

			ERROR( "sfx destroy is specifying invalid param types" );
		}

		case SP_ATTACH:
		{
			ERROR_IF( (params.Count() < 3), "sfx attach is missing parameters" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "sfx attach parameter 1 must be a uint" );
			ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_UINT32), "sfx attach parameter 2 must be a uint" );

			gWorldFx.AttachEffect( (SFxEID)params._uint32(1), (SFxEID)params._uint32(2) );
			return true;
		}

		case SP_ATTACH_POINT:
		{
			ERROR_IF( (params.Count() < 2), "sfx attach_point is missing parameters" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "sfx attach_point param 1 must be a uint" );
			ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_STRING), "sfx attach_point param 2 must be a string" );
			ERROR_IF( (params.GetType(3) != Flamethrower_params::PT_COMMAND_PARAM), "sfx attach_point param 3 must be a command param" );

			TattooTracker *pTracker = NULL;

			if( gWorldFx.GetTargets( (SFxEID)params._uint32(1), &pTracker ) )
			{
				TattooTracker::TARGET_INDEX ti = (params._cparam(3) == SP_TARGET)?TattooTracker::TARGET:TattooTracker::SOURCE;

				pTracker->SetPositionOffsetName( params._gpstring(2), ti );
				return true;
			}

			// targets were deleted so end this script
			pScript->Stop();
			return false;
		}

		case SP_POSITION:
		{
			ERROR_IF( (params.Count() < 3), "sfx position is missing parameters" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "sfx position param 1 must be a uint" );
			ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_VECTOR), "sfx position param 2 must be a vector" );

			SiegePos position( params._vector_3(2), siege::UNDEFINED_GUID );

			gWorldFx.SetPosition( (SFxEID)params._uint32(1), position );
			return true;
		}

		case SP_POSITION_AT:
		{
			ERROR_IF( (params.Count() < 4), "sfx position_at is missing parameters" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "sfx position_at param 1 must be a uint" );
			ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_STRING), "sfx position_at param 2 must be a string" );
			ERROR_IF( (params.GetType(3) != Flamethrower_params::PT_COMMAND_PARAM), "sfx position_at param 3 must be a command param" );
			
			TattooTracker *pTracker = pScript->GetDefaultTargets();
			Flamethrower_target *pTarget = NULL;
	
			TattooTracker::TARGET_INDEX ti = (params._cparam(3) == SP_SOURCE)?TattooTracker::SOURCE:TattooTracker::TARGET;

			pTracker->GetTarget( ti, &pTarget );

			// Target could have been deleted before this code executes so check to make sure
			if( pTarget )
			{
				SiegePos	target_position;
				if( pTarget->GetPlacement( &target_position, NULL, false, params._gpstring(2) ) )
				{
					gWorldFx.SetPosition( (SFxEID)params._uint32(1), target_position );
					return true;
				}
			}

			// No target exists so the script cannot continue
			pScript->Stop();
			return false;
		}

		case SP_OFFSET:
		{
			ERROR_IF( (params.Count() < 4), "sfx offset is missing parameters" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "sfx offset param 1 must be a uint" );
			ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_VECTOR), "sfx offset param 2 must be a vector" );
			ERROR_IF( (params.GetType(3) != Flamethrower_params::PT_COMMAND_PARAM), "sfx offset param 3 must be a command param" );

			TattooTracker *pTracker = pScript->GetDefaultTargets();

			TattooTracker::TARGET_INDEX ti = (params._cparam(3) == SP_SOURCE)?TattooTracker::SOURCE:TattooTracker::TARGET;

			matrix_3x3	basis;
			pTracker->GetPlacement( NULL, &basis, false, ti );

			gWorldFx.OffsetPosition( (SFxEID)params._uint32(1), params._vector_3(2), basis );
			return true;
		}

		case SP_OFFSET_BONE:
		{
			ERROR_IF( (params.Count() < 4), "sfx offset_bone is missing parameters" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "sfx offset_bone param 1 must be a uint" );
			ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_VECTOR), "sfx offset_bone param 2 must be a vector" );
			ERROR_IF( (params.GetType(3) != Flamethrower_params::PT_COMMAND_PARAM), "sfx offset_bone param 3 must be a command param" );

			TattooTracker::TARGET_INDEX ti = (params._cparam(3) == SP_SOURCE)?TattooTracker::SOURCE:TattooTracker::TARGET;

			gWorldFx.OffsetTargetPosition( (SFxEID)params._uint32(1), ti, params._vector_3(2) );
			return true;
		}

		case SP_DIRECTION:
		{
			ERROR_IF( (params.Count() < 3), "sfx direction is missing parameters" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "sfx direction param 1 must be a uint" );
			ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_VECTOR), "sfx direction param 2 must be a vector" );

			gWorldFx.SetDirection( (SFxEID)params._uint32(1), params._vector_3(2) );
			return true;
		}

		case SP_FRIENDLY:
		{
			ERROR_IF( (params.Count() < 3), "sfx friendly is missing parameters" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_COMMAND_PARAM), "sfx friendly param 1 must be a command param" );
			ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_UINT32), "sfx friendly param 2 must be a uint" );

			TattooTracker *pTargets = pScript->GetDefaultTargets();

			TattooTracker::TARGET_INDEX ti = (params._cparam(1) == SP_TARGET)?TattooTracker::TARGET:TattooTracker::SOURCE; 

			SFxEID effect_id = (SFxEID)params._uint32(2);

			gWorldFx.AddFriendly( effect_id, pTargets->GetID( ti ) );

			if( params._cparam(1) == SP_PARTY )
			{
				Go* hParty = gServer.GetScreenParty();
				if( hParty == NULL )
				{
					return true;
				}

				GoHandle hSource( pTargets->GetID( TattooTracker::SOURCE ) );
				if( !hSource.IsValid() )
				{
					pScript->Stop();
					return false;
				}

				GopColl::const_iterator iGo = hParty->GetChildBegin(), party_end = hParty->GetChildEnd();
				for( ; iGo != party_end; ++iGo )
				{
					if( hSource->HasMind() && hSource->GetMind()->IsFriend( *iGo ) )
					{
						gWorldFx.AddFriendly( effect_id, (*iGo)->GetGoid() );
					}
				}
			}
			return true;
		}
	}

	ERROR( "sfx unknown commapnd parameter" );
}


static const char * s_SignalParamStrings[] =
{
	"all_scripts",
};

DEFINE_PARAM_CONVERTERS( Signal );

bool
FC_Signal::Execute( Flamethrower_script *pScript, Flamethrower_params & params )
{
	ERROR_IF( (params.Count() < 2), "signal is missing parameters" );
	ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_STRING), "signal param 2 must be a string" );

	if( (params.GetType(0) == Flamethrower_params::PT_COMMAND_PARAM ) && params._cparam(0) == SP_ALL_SCRIPTS )
	{
		gFlamethrower_interpreter.PostSignal( 0, params._gpstring(1), true );
	}
	else
	{
		ERROR_IF( (params.GetType(0) != Flamethrower_params::PT_UINT32), "signal param 1 must be a uint" );

		gFlamethrower_interpreter.PostSignal( (SFxSID)params._uint32(0), params._gpstring(1) );
	}
	return true;
}


static const char * s_SoundParamStrings[] =
{
	"play",
	"loop",
	"at",
	"dist",
	"stop",
};

DEFINE_PARAM_CONVERTERS( Sound );

bool
FC_Sound::Execute( Flamethrower_script *pScript, Flamethrower_params & params )
{
	UINT32 param_count = params.Count();

	ERROR_IF( (param_count > 8 ), "sound has too many parameters" );
	ERROR_IF( (params.GetType(0) != Flamethrower_params::PT_COMMAND_PARAM), "sound parameter 1 must be command param" );

	if( params._cparam(0) == SP_PLAY )
	{
		ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_STRING), "sound parameter 2 must be a string" );
	
		if( param_count == 2 )
		{
			UINT32	return_value = (UINT32) gWorldFx.PlaySample( params._gpstring(1), false );
			float	sound_length = gWorldFx.GetSampleLengthInSeconds( params._gpstring(1) );

			// this is now replaced by a self managed list.	// gWorldFx.SetSampleStopCallback( return_value, pScript );
			// the call back was calling pScript->RemoveSoundID 
			
			// we are not looping, insert on the non-looping map
			pScript->GetSoundLog().insert( std::make_pair( PreciseAdd( sound_length, gWorldTime.GetTime()), return_value ) );

			RETURN( return_value );
		}

		if( param_count == 3 )
		{
			ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_COMMAND_PARAM), "sound parameter 3 must be a command param" );

			const bool bLoop = (params._cparam(2) == SP_LOOP );
			
			UINT32	return_value = (UINT32) gWorldFx.PlaySample( params._gpstring(1), bLoop );
			
			if( bLoop == true )
			{
				// if we are looping then insert on the looping vector
				pScript->GetLoopingSoundLog().push_back( return_value );
			}
			else
			{
				// if we are not looping then push back on the non-looping map
				float	sound_length = gWorldFx.GetSampleLengthInSeconds( params._gpstring(1) );
				pScript->GetSoundLog().insert( std::make_pair( PreciseAdd(sound_length, gWorldTime.GetTime()), return_value ) );
			}

			RETURN( return_value );
		}

		if( param_count >= 4 )
		{
			ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_COMMAND_PARAM), "sound parameter 3 must be a command param" );
			ERROR_IF( (params._cparam(2) != SP_AT), "sound parameter 3 is invalid" );

			ERROR_IF( (params.GetType(3) != Flamethrower_params::PT_POSITION), "sound param 4 must be a position" );

			UINT32 return_value = 0;

			if( params._SiegePos(3).node != siege::UNDEFINED_GUID )
			{
				bool bLoop = false;
				float min_dist = 0, max_dist = 0;
				bool bRange = false;

				if( param_count > 4 )
				{
					ERROR_IF( (params.GetType(4) != Flamethrower_params::PT_COMMAND_PARAM ), "sound parameter 5 must be a command param" );

					if( params._cparam(4) == SP_DIST )
					{
						// cast and complain
						ERROR_IF( (!params.SetType(5, Flamethrower_params::PT_FLOAT)), "sound min distance param is invalid" );
						ERROR_IF( (!params.SetType(6, Flamethrower_params::PT_FLOAT)), "sound max distance param is invalid" );

						min_dist = params._float(5);
						max_dist = params._float(6);

						bRange = true;

						if( param_count > 7 )
						{
							ERROR_IF( (params.GetType(7) != Flamethrower_params::PT_COMMAND_PARAM), "sound param 8 must be a commnad param" );
							ERROR_IF( (params._cparam(7) != SP_LOOP), "sound param 8 is invalid" );

							bLoop = true;
						}
					}
					else if ( params._cparam(4) == SP_LOOP )
					{
						bLoop = true;
					}
				}

				if( bRange )
				{
					return_value = (UINT32)gWorldFx.PlaySample( params._gpstring(1), params._SiegePos(3), bLoop, min_dist, max_dist );
				}
				else
				{
					return_value = (UINT32)gWorldFx.PlaySample( params._gpstring(1), params._SiegePos(3), bLoop );
				}

				if( bLoop == true )
				{
					// if we are looping then push back on the looping vector
					pScript->GetLoopingSoundLog().push_back( return_value );
				}
				else
				{
					// if we are not looping then insert on the non-looping map
					float	sound_length = gWorldFx.GetSampleLengthInSeconds( params._gpstring(1) );
					pScript->GetSoundLog().insert( std::make_pair( PreciseAdd(sound_length, gWorldTime.GetTime()), return_value ) );
				}
			}

			RETURN( return_value );
		}
	}
	else if( params._cparam(0) == SP_STOP )
	{
		ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "sound stop param 1 must be a uint" );

		UINT32 id = params._uint32(1);

		pScript->RemoveCompletionTestID( id );
		gWorldFx.StopSample( id );
		return true;
	}

	ERROR( "sound unrecognized sound command parameter" );
}


static const char * s_WaitforParamStrings[] =
{
	"termination",
	"script",
	"sig",
	"collision",
};

DEFINE_PARAM_CONVERTERS( Waitfor );

bool
FC_Waitfor::Execute( Flamethrower_script *pScript, Flamethrower_params & params )
{
	ERROR_IF( (params.Count() < 1), "waitfor is missing parameters" );
	ERROR_IF( (params.GetType(0) != Flamethrower_params::PT_COMMAND_PARAM), "waitfor paramter 1 must be a command param" );

	switch( (eWaitforParam)params._cparam(0) )
	{
		case WP_TERMINATION:
		{
			return false;
		}

		case WP_SCRIPT:
		{
			ERROR_IF( (params.Count() < 3), "waitfor script is missing a timeout value for param 2" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "waitfor script param 1 is invalid must be a uint" );

			// cast and complain
			ERROR_IF( (!params.SetType(2, Flamethrower_params::PT_FLOAT)), "waitfor specifies invalid timeout value" );

			// initialize
			if( params.Count() == 3 )
			{
				params.Add( params._float(2) );
				return false;
			}

			// poll until true or timeout reached
			if( !gFlamethrower_interpreter.IsScriptRunning( (SFxSID)params._uint32(1) ) )
			{
				params._float(3) = params._float(2);
				RETURN( 1.0f );
			}
			else if( params._float(3) <= 0 )
			{
				RETURN( 0.0f );
			}

			params._float(3) -= gFlamethrower.GetInterpreter().GetSecondsElapsed();
			return false;
		}

		case WP_SIG:
		{
			ERROR_IF( (params.Count() < 3), "waitfor sig is missing a timeout value for param 2" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_STRING), "waitfor script param 1 is invalid must be a string" );

			// cast and complain
			ERROR_IF( (!params.SetType(2, Flamethrower_params::PT_FLOAT)), "waitfor specifies invalid timeout value" );

			// initialize
			if( params.Count() == 3 )
			{
				params.Add( params._float(2) );
				return false;
			}

			if( gFlamethrower_interpreter.CheckForSignal( pScript->GetID(), params._gpstring(1) ) )
			{
				RETURN( 1.0f );
			}

			if( params._float(3) <= 0 )
			{
				RETURN( 0.0f );
			}

			params._float(3) -= gFlamethrower.GetInterpreter().GetSecondsElapsed();
			return false;
		}

		case WP_COLLISION:
		{
			ERROR_IF( (params.Count() < 3), "waitfor collision is missing a timeout value for param 2" );
			ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "waitfor collision param 1 must be a uint" );

			// cast and complain
			ERROR_IF( (!params.SetType(2, Flamethrower_params::PT_FLOAT)), "waitfor specifies invalid timeout value" );

			// initialize
			if( params.Count() == 3 )
			{
				params.Add( params._float(2) );
				return false;
			}

			if( params._float(3) <= 0 )
			{
				RETURN( (float)Flamethrower_script::NO_COLLISION );
			}

			CollisionInfo	collision;

			if( gWorldFx.EffectHasCollided( (SFxEID)params._uint32(1), collision ) )
			{
				if( collision.IsTerrainCollision() )
				{
					RETURN( Flamethrower_script::TERRAIN_COLLISION );
				}
				RETURN( Flamethrower_script::TARGET_COLLISION );
			}

			params._float(3) -= gFlamethrower.GetInterpreter().GetSecondsElapsed();
			return false;
		}
	}
	ERROR( "waitfor parse error!" );
}


//	worldmsg "Event" from to data

bool
FC_Worldmsg::Execute( Flamethrower_script *pScript, Flamethrower_params & params )
{
	ERROR_IF( (params.Count() < 4), "worldmsg is missing parameters" );

	ERROR_IF( (params.GetType(0) != Flamethrower_params::PT_STRING), "worldmsg parameter 1 must be a string" );
	ERROR_IF( (params.GetType(1) != Flamethrower_params::PT_UINT32), "worldmsg parameter 2 must be a uint" );
	ERROR_IF( (params.GetType(2) != Flamethrower_params::PT_UINT32), "worldmsg parameter 3 must be a uint" );
	ERROR_IF( (params.GetType(3) != Flamethrower_params::PT_UINT32), "worldmsg parameter 4 must be a uint" );

	Goid	From	= MakeGoid( params._uint32(1) );
	Goid	To		= MakeGoid( params._uint32(2) );
	DWORD	Data	= params._uint32(3);

	// Don't send the message to go's outside the valid world frusti
	GoHandle hTo( To );

	if( hTo )
	{
		bool bOkToSend = true;

		if( !hTo->IsOmni() )
		{
			bOkToSend = gWorldFx.IsInWorldFrustum( hTo->GetPlacement()->GetPosition().node );
		}

		// Ok to send the actual world message
		if( bOkToSend )
		{
			gWorldFx.Message( params._gpstring(0).c_str(), From, To, Data );
		}
	}
	return true;
}
