/*==================================================================================================

	MCP_Request -- MCP support code

	purpose:	Provides support for managing requests made to the MCP by the AI

	author:

  (c)opyright Gas Powered Games, 2000

--------------------------------------------------------------------------------------------------*/

#include "Precomp_World.h"
#include "MCP_Defs.h"
#include "MCP.h"
#include "MCP_Request.h"

#include "GoCore.h"
#include "GoAspect.h"
#include "GoBody.h"
#include "GoInventory.h"
#include "GoPhysics.h"
#include "GoAttack.h"
#include "Nema_Aspect.h"
#include "Nema_Blender.h"
#include "skritsupport.h"
#include "world.h"
#include "WorldTime.h"

using namespace MCP;
using namespace siege;

//**********************************************************************************
//**********************************************************************************

///////////////////////////////////////////////////////////////////////
// enum eRequest support
// 

static const char* s_RequestStrings[] =
{
	"pl_none",
	"pl_approach",
	"pl_follow",
	"pl_intercept",
	"pl_avoid",
	"pl_face",
	"pl_die",
	"pl_attack_object_melee",
	"pl_attack_object_ranged",
	"pl_attack_position_melee",
	"pl_attack_position_ranged",
	"pl_cast",
	"pl_cast_on_object",
	"pl_cast_on_position",
	"pl_fidget",
	"pl_open",
	"pl_close",
	"pl_wait_for_contact",
	"pl_playanim",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_RequestStrings ) == PL_COUNT );

FUBI_EXPORT_ENUM( eRequest, PL_BEGIN, PL_COUNT );
FUBI_REPLACE_NAME("MCPeRequest",eRequest);

static stringtool::EnumStringConverter s_PLConverter( s_RequestStrings, PL_BEGIN, PL_END );

bool FromString( const char* str, MCP::eRequest & x )
{
	return ( s_PLConverter.FromString( str, rcast <int&> ( x ) ) );
}

const char* ToString( MCP::eRequest x )
{
	return ( s_PLConverter.ToString( x ) );
}

////////////////////////////////////////////////////////////////////////////////////
// enum eReqRetCode support

static const char* s_ReqRetCodeStrings[] =
{
	"request_ok",
	"request_ok_in_range",
	"request_ok_inside_range",
	"request_ok_beyond_range",
	"request_ok_crowded",
	"request_failed",
	"request_failed_bad_type",
	"request_failed_bad_owner",
	"request_failed_bad_target",
	"request_failed_missing_follower",
	"request_failed_no_path",
	"request_failed_bad_path_begin",
	"request_failed_bad_path_end",
	"request_failed_zero_velocity",
	"request_failed_overcrowded",
	"request_failed_path_leaves_world"
};

COMPILER_ASSERT( ELEMENT_COUNT( s_ReqRetCodeStrings ) == REQRETCODE_COUNT );

FUBI_EXPORT_ENUM(eReqRetCode, REQRETCODE_BEGIN, REQRETCODE_COUNT);
FUBI_REPLACE_NAME("MCPeReqRetCode",eReqRetCode);

static stringtool::EnumStringConverter s_ReqRetCodeConverter( s_ReqRetCodeStrings, REQRETCODE_BEGIN, REQRETCODE_END, REQUEST_OK);

const char* ToString( eReqRetCode e )
	{  return ( s_ReqRetCodeConverter.ToString( e ) );  }
bool FromString( const char* str, eReqRetCode& e )
	{  return ( s_ReqRetCodeConverter.FromString( str, rcast <int&> ( e ) ) );  }


///////////////////////////////////////////////////////////////////////
bool RequestOk( eReqRetCode code )
{
	switch( code )
	{
	case REQUEST_OK:
	case REQUEST_OK_IN_RANGE:
	case REQUEST_OK_INSIDE_RANGE:
	case REQUEST_OK_BEYOND_RANGE:
	case REQUEST_OK_CROWDED:
		return true;
	};

	return false;
}


///////////////////////////////////////////////////////////////////////
bool RequestFailed( eReqRetCode code )
{
	switch( code )
	{
	case REQUEST_OK:
	case REQUEST_OK_IN_RANGE:
	case REQUEST_OK_INSIDE_RANGE:
	case REQUEST_OK_BEYOND_RANGE:
	case REQUEST_OK_CROWDED:
		return false;
	};

	return true ;
}

///////////////////////////////////////////////////////////////////////
bool RequestBeyondRange( eReqRetCode code )
{
	switch( code )
	{
	case REQUEST_OK:
	case REQUEST_OK_IN_RANGE:
	case REQUEST_OK_INSIDE_RANGE:
	case REQUEST_OK_CROWDED:
		return false;
	};

	return true ;
}


///////////////////////////////////////////////////////////////////////
void DumpRequestInfo( Goid ownerGoid, eRequest type, eReqRetCode ret )
{
#if !GP_RETAIL
	GoHandle owner( ownerGoid );
	if( owner && owner->IsHuded() )
	{
		gpgenericf(( "[%2.4f, 0x%08x] - MCP request '%s' returned '%s' for '%s', goid:0x%08x\n",
					gWorldTime.GetTime(),
					gWorldTime.GetSimCount(),
					::ToString( type ),
					::ToString( ret ),
					owner->GetTemplateName(),
					owner->GetGoid()
					));
	}
#endif
}

//////////////////////////////////////////////////////////////////////////////
// Targets that are Positions
eReqRetCode Manager::FindPathTo( Goid sourceGoid, const SiegePos& reqPos, float range, bool reverse )
{

	GoHandle sourceGo(sourceGoid);
	if (!sourceGo.IsValid())
	{
		gpwarning("Attempted FindPathTo with invalid sourceGoid");
		return REQUEST_FAILED_BAD_OWNER;
	}

	Plan* srcPlan = FindPlan( sourceGoid ) ;

	const SiegePos b = srcPlan->GetFinalDestination();

	if (!gSiegeEngine.IsNodeInAnyFrustum(b.node))
	{
		gpwarning("Attempted FindPathTo with invalid start position");
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}

	SiegePos targetPos(reqPos);
	if( !gSiegeEngine.IsNodeInAnyFrustum(targetPos.node) || !gSiegeEngine.AdjustPointToTerrain(targetPos) )
	{
		gpwarning("Attempted FindPathTo with invalid targetPos");
		return REQUEST_FAILED_BAD_PATH_END;
	}

	vector_3 delta = gSiegeEngine.GetDifferenceVector(b,targetPos);

	eReqRetCode ret = REQUEST_FAILED;

	float diffsquared = Length2(delta);
	float minrange = range*0.8f;
	float minrangesquared = minrange*minrange;
	float maxrangesquared = range*range;
	if (!IsPositive(diffsquared-maxrangesquared,MINIMUM_DISTANCE_TOLERANCE_SQUARED))
	{

		if (diffsquared<=minrangesquared)
		{
			ret = REQUEST_OK_INSIDE_RANGE;
		}
		else
		{
			ret = REQUEST_OK_IN_RANGE;
		}
	} 

	{
		// $$$ Need to determine if we have to insert a CHORE_WALK
		ret = srcPlan->AppendMoveHelper(targetPos, range, FLOAT_MAX, reverse, GOID_INVALID);
	}

	return ret;
}


////////////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::FindPathToCrowd( Plan* srcPlan, Plan* tgtPlan, float range, bool reverse )
{
	// The target (this plan) is in a melee crowd with at least one other Go 
	// Need to append a path to the srcPlan that will allow it to get into range without
	// interfering with any existing combatants
	
	GoHandle sourceGo(srcPlan->GetOwner());

	if (!sourceGo.IsValid())
	{
		gpwarning("Attempted FindPathTo with invalid sourceGoid");
		return REQUEST_FAILED_BAD_OWNER;
	}

	const SiegePos b = srcPlan->GetFinalDestination();
	const SiegePos e = tgtPlan->GetFinalDestination();

	if (!gSiegeEngine.IsNodeInAnyFrustum(b.node))
	{
		gpwarning("Attempted FindPathTo with invalid start position");
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}

	if( !gSiegeEngine.IsNodeInAnyFrustum(e.node) ) {
		gpwarning("Attempted FindPathTo with invalid targetPos");
		return REQUEST_FAILED_BAD_PATH_END;
	}

	eReqRetCode ret = REQUEST_FAILED;

	{
		ret = srcPlan->AppendMoveToCrowd( tgtPlan, range, reverse );
	}


	return ret;
}

//////////////////////////////////////////////////////////////////////////////
float ApproximateFiringElevation(Plan* shootPlan, const SiegePos& targPos)
{
	GoHandle theShooter(shootPlan->GetOwner());
	
	double shoottime = shootPlan->GetAppendTime();
	SiegePos shootPos = shootPlan->GetPosition(shoottime,true,false);

	float ele = 0;

	if (   gSiegeEngine.IsNodeInAnyFrustum(shootPos.node)
		&& gSiegeEngine.IsNodeInAnyFrustum(targPos.node)
		&& theShooter->GetInventory()->IsRangedWeaponEquipped()) 
	{

		// Offset the launch position by the height of the head
		vector_3 bone_position(DoNotInitialize);
		Quat dq(DoNotInitialize);

		int head = -1;
		if (!theShooter->GetAspect()->GetAspectHandle()->GetBoneIndex("Bip01_Head",head))
		{
			const BoneTranslator& bt = theShooter->GetBody()->GetBoneTranslator();
			// Get the bone translator and extract the head bone
			bt.Translate( BoneTranslator::BODY_ANTERIOR,  head );
		}
		
		if ((head != -1) && theShooter->GetAspect()->GetAspectHandle()->GetIndexedBoneOrientation(head,bone_position,dq))
		{
			shootPos.pos.y += bone_position.y;
		}

		Go* theWeapon;
		if (theShooter->GetInventory()->GetAnimStance() == AS_BOW_AND_ARROW)
		{
			theWeapon = theShooter->GetInventory()->GetEquipped(ES_SHIELD_HAND);
		}
		else
		{
			theWeapon = theShooter->GetInventory()->GetEquipped(ES_WEAPON_HAND);
		}

		if (theWeapon) 
		{
			float v = theWeapon->GetPhysics()->GetVelocity();

			vector_3 diff = gSiegeEngine.GetDifferenceVector(shootPos,targPos);
			float ang = (float)atan2(diff.y,sqrtf((diff.x*diff.x)+(diff.z*diff.z)));

			Goid ammo = theWeapon->GetAttack()->GetAmmoCloneSource();

			ang += theWeapon->GetAttack()->ComputeAimingAngle( ammo , shootPos, targPos, v );

			ele = ang * (1/PI);
		}

	}

	return ele;

}

//////////////////////////////////////////////////////////////////////////////
float ApproximateFiringDeviation(Plan* shootPlan, const SiegePos& targPos)
{

	UNREFERENCED_PARAMETER(shootPlan);
	UNREFERENCED_PARAMETER(targPos);

	float deviation = 0;

// In this version of DS we don't allow the archers to twist in place
/*
	GoHandle theShooter(shootPlan->GetOwner());
	
	double apptime = shootPlan->GetAppendTime();
	SiegePos shootPos = shootPlan->GetPosition(apptime);

	if (   gSiegeEngine.IsNodeInAnyFrustum(shootPos.node)
		&& gSiegeEngine.IsNodeInAnyFrustum(targPos.node)) 
	{
		// What if this is a moving critter? 
		Quat shootRot = shootPlan->GetOrientation(apptime).rot;

		// Determine difference between the  angle we expect to be facing and the angle to the targetPos
		vector_3 a = gSiegeEngine.GetDifferenceVector(shootPos,targPos);
		vector_3 b( DoNotInitialize );
		shootRot.RotateXAxis( b );

		// cos(deviation) = (A.B) / (|A| * |B|)  where B is a unit vector

		float d = (a.x*a.x)+(a.z*a.z);

		if (!IsZero(d))
		{
			float dot = (a.x*b.x)+(a.z*b.z);
			float theta = acosf(dot/sqrtf(d)); 
			deviation = theta * (1/PI) - 0.5f;
		}
		else
		{
			deviation = 0;
		}
	}
*/
	return deviation;

}

//////////////////////////////////////////////////////////////////////////////
// Targets that are Objects
eReqRetCode Manager::FindPathTo( Goid sourceGoid, Goid targetGoid, float range, bool reverse, eDependancyType dtype )
{
	////////////////////////////////////////
	//	validate params

	GoHandle sourceGo( sourceGoid );
	if (!sourceGo.IsValid())
	{
		return REQUEST_FAILED_BAD_OWNER;
	}

	GoHandle targetGo(targetGoid);
	if (!targetGo.IsValid())
	{
		return REQUEST_FAILED_BAD_TARGET;
	}
	
	Plan* srcPlan = FindPlan( sourceGoid ) ;
	const SiegePos b = srcPlan->GetFinalDestination();

	if (!gSiegeEngine.IsNodeInAnyFrustum(b.node))
	{
		gpwarning("Attempted FindPathTo with invalid start position");
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}

	Plan* tgtPlan = FindPlan( targetGoid, false ) ;

	eReqRetCode ret	= REQUEST_FAILED_BAD_PATH_END;

	typedef stdx::linear_map<float,SiegePos>  SortedUsePointsColl;
		
	SortedUsePointsColl sorted_use_points;

	GoPlacement::UsePointColl usep;

	const SiegePos& e = targetGo->GetPlacement()->GetPosition();

	if (!gSiegeEngine.IsNodeInAnyFrustum(e.node))
	{
		gpwarningf(("[%s:0x%08x] Attempted FindPathTo with target [%s:0x%08x] that will not remain in the world",
			sourceGo->GetTemplateName(),
			sourceGoid,
			targetGo->GetTemplateName(),
			targetGoid));

		ret	= REQUEST_FAILED_BAD_PATH_END;
	}
	else 
	{
		if (targetGo->GetPlacement()->GetUsePoints(usep))
		{
			GoPlacement::UsePointColl::iterator pbeg = usep.begin();
			GoPlacement::UsePointColl::iterator pend = usep.end();
			GoPlacement::UsePointColl::iterator pit;

			siege::eLogicalNodeFlags walkPerms = sourceGo->GetBody()->GetTerrainMovementPermissions();

			for (pit = pbeg; pit != pend; ++pit)
			{
				SiegePos upoint(*pit);
				if (gSiegeEngine.IsPositionAllowed(upoint,walkPerms,5.0f))
				{
					float ll;
#if !GP_RETAIL					
					ll = Length2(gSiegeEngine.GetDifferenceVector(e,upoint));
					if (!IsZero(range) && (ll > (9*range*range)))
					{
						gpwarningf(("POSSIBLE CONTENT ERROR: USE_POINT attached to [%s g:%08x s:%08x] is %f away\n"
									"\twhich is more than 3x the request range of %f!\n"
									"\t...this is NOT a problem if it's on an elevator or teleport platform that has moved away\n",
							targetGo->GetTemplateName(),
							targetGoid,
							targetGo->GetScid(),
							sqrtf(ll),
							range));
					}
#endif
					ll = Length2(gSiegeEngine.GetDifferenceVector(b,upoint));
					sorted_use_points.unsorted_push_back(std::make_pair(ll,upoint));
				}
				else
				{
					gpwarningf(("POSSIBLE CONTENT ERROR: USE_POINT attached to [%s g:%08x s:%08x] is unreachable by [%s g:%08x s:%08x]\n",
						targetGo->GetTemplateName(),
						targetGoid,
						targetGo->GetScid(),
						sourceGo->GetTemplateName(),
						sourceGoid,
						sourceGo->GetScid()));
				}
			}
			
			sorted_use_points.sort();

			SortedUsePointsColl::iterator beg = sorted_use_points.begin();
			SortedUsePointsColl::iterator end = sorted_use_points.end();
			SortedUsePointsColl::iterator i;

			ret	= REQUEST_FAILED;

			// Assume that we want to get 'very close' to a USE_POINT unless we are wielding a projectile weapon
			float MIN_RANGE_TO_USE_POINT = (DEPTYPE_PROJECTILE == dtype) ? range : 0.01f;

			for ( i = beg ; i != end && RequestFailed(ret); ++i )
			{

				const SiegePos& e = (*i).second;

				if (!gSiegeEngine.IsNodeInAnyFrustum(e.node))
				{
					gpwarning("Attempted FindPathTo with invalid targetGoid");
					ret	= REQUEST_FAILED_BAD_PATH_END;
				}
				else 
				{
					// Are there any other dependants heading for this use point?
					if (!tgtPlan || !srcPlan->CheckForOccupied(sourceGo.Get(),e))
					{
						vector_3 delta = gSiegeEngine.GetDifferenceVector(b,e);

						float diffsquared = Length2(delta);

						if (!IsPositive(diffsquared,MINIMUM_DISTANCE_TOLERANCE_SQUARED))
						{
							ret = REQUEST_OK_IN_RANGE;
						}
						else
						{
							ret = srcPlan->AppendMoveHelper( e, MIN_RANGE_TO_USE_POINT, FLOAT_MAX, reverse,targetGoid );
						}

						if (RequestOk(ret))
						{
							if (tgtPlan)
							{
								Plan::DependancyInfo diInt(sourceGoid,
														srcPlan->GetFinalDestinationTime(),
														tgtPlan->GetFinalDestination(),
														srcPlan->GetFinalDestination(),
														dtype);
								tgtPlan->CreateDependancyLink(diInt);
							}
							return ret;
						}
					}
				}
			}
		}
	
		// We couldn't use a use point

		vector_3 delta = gSiegeEngine.GetDifferenceVector(b,e);
			
		// $$ TODO: Can I get away with not checking for blocker AGAIN?
		bool crowdcontrol = false;
		if (tgtPlan)
		{
			float w = (srcPlan->GetPathWidth()+tgtPlan->GetPathWidth()) * 0.8f;
			float deltalen2 = delta.Length2();

			crowdcontrol = (tgtPlan->IsInMeleeCrowd() || 
							((tgtPlan->CheckForBlockers(e,sourceGoid,false,false,false,true) || ((w*w) > deltalen2))
							  &&
							 ((dtype == DEPTYPE_MELEE_ENEMY) || (dtype == DEPTYPE_MELEE_FRIEND) )) );

		}

		ret	= REQUEST_FAILED;

		if (crowdcontrol)
		{
			ret = srcPlan->AppendMoveToCrowd( tgtPlan, range, reverse );
		}
		else
		{
			float diffsquared = Length2(delta);
			float minrange = range*0.8f;
			float minrangesquared = minrange*minrange;
			float maxrangesquared = range*range;
			if (!IsPositive(diffsquared-maxrangesquared,MINIMUM_DISTANCE_TOLERANCE_SQUARED))
			{

				bool conflict_detected = tgtPlan && srcPlan->CheckForBlockers(b,targetGoid,true,false,false,false);

				if (conflict_detected)
				{
					ret = srcPlan->AppendMoveToCrowd( tgtPlan, range, reverse );
				}
				else
				{
					if (diffsquared<=minrangesquared)
					{
						ret = REQUEST_OK_INSIDE_RANGE;
					}
					else
					{
						ret = REQUEST_OK_IN_RANGE;
					}
				}
			}
			else
			{
				ret = srcPlan->AppendMoveHelper(e, range, FLOAT_MAX, reverse,targetGoid);
			}

			if (RequestOk(ret))
			{
				if (tgtPlan)
				{
					Plan::DependancyInfo diInt(sourceGoid,
											srcPlan->GetFinalDestinationTime(),
											tgtPlan->GetFinalDestination(),
											srcPlan->GetFinalDestination(),
											dtype);
					tgtPlan->CreateDependancyLink(diInt);
				}
				return ret;
			}
		}
	}

	return ret;
}


eReqRetCode Manager::FindInterceptPath( Goid sourceGoid, Goid targetGoid, float lookahead, float maxtime,	float range, float cushion, bool synch_at_end, bool reverse, eDependancyType dtype)
{

	// We want to find a path for the source that will cause it to meet up with the target

	eReqRetCode ret = REQUEST_FAILED_NO_PATH;

	GoHandle sourceGo(sourceGoid);
	if (!sourceGo.IsValid() || !sourceGo->IsInAnyWorldFrustum()) 
	{
		return REQUEST_FAILED_BAD_OWNER;
	}

	GoHandle targetGo(targetGoid);
	if (!targetGo.IsValid() || !targetGo->IsInAnyWorldFrustum()) 
	{
		return REQUEST_FAILED_BAD_TARGET;
	}
	
	Plan* srcPlan = FindPlan( sourceGoid ) ;
	Plan* tgtPlan = FindPlan( targetGoid ) ;

	float src_avg_velocity = sourceGo->GetBody()->GetAvgMoveVelocity();

	SiegePos intercept_pos;

	if (IsZero(src_avg_velocity))
	{

		SiegePos initial_pos  = sourceGo->GetPlacement()->GetPosition();
		intercept_pos = targetGo->GetPlacement()->GetPosition();
		vector_3 delta = gSiegeEngine.GetDifferenceVector(initial_pos,intercept_pos);

		float diffsquared = Length2(delta);
		float minrange = range*0.8f;
		float minrangesquared = minrange*minrange;
		float maxrangesquared = range*range;
		if (!IsPositive(diffsquared-maxrangesquared,MINIMUM_DISTANCE_TOLERANCE_SQUARED))
		{
			// Create a dependancy link to the target's intercept position at the intercept time
			Plan::DependancyInfo diInt(sourceGoid,srcPlan->GetFinalDestinationTime(),intercept_pos,srcPlan->GetFinalDestination(),dtype);
			tgtPlan->CreateDependancyLink(diInt);

			if (diffsquared<=minrangesquared)
			{
				return REQUEST_OK_INSIDE_RANGE;
			}
			else
			{
				return REQUEST_OK_IN_RANGE;
			}
		}
		else
		{
			gpwarningf(("WARNING: Attempting to find an INTERCEPT path for [%s:0x%08x] but the is AvgMoveVelocity is ZERO. Please check the template\n",GetGo(sourceGoid)->GetTemplateName(),sourceGoid));
			src_avg_velocity = sourceGo->GetAspect()->GetAspectHandle()->GetBlender()->GetScalarVelocity(CHORE_WALK, AS_PLAIN, 0);

			if (!IsZero(src_avg_velocity))
			{
				gpwarningf(("         Using the velocity %f from [CHORE_WALK, AS_PLAIN, 0] as as substitute\n",src_avg_velocity));	
			} 
			else
			{
				return( REQUEST_FAILED_ZERO_VELOCITY );
			}
		}
	}
	
	// Does the target have use points?
	if (targetGo->GetPlacement()->HasUsePoints())
	{
		ret = FindPathTo(sourceGoid,targetGoid,range,reverse, dtype);
		if (RequestOk(ret))
		{
			// Create a dependancy link to the target's intercept position at the intercept time
			if (tgtPlan)
			{
				Plan::DependancyInfo diInt(sourceGoid,
										srcPlan->GetFinalDestinationTime(),
										tgtPlan->GetFinalDestination(),
										srcPlan->GetFinalDestination(),
										dtype);
				tgtPlan->CreateDependancyLink(diInt);
			}
			return ret;
		}
	}

	float tgtTravelTime = (float)(PreciseSubtract(tgtPlan->GetFinalDestinationTime(),GetLeadingTime()));

	// $$ TODO: Can I get away with not checking for blocker AGAIN?
	bool crowdcontrol = false;
	if (tgtPlan && !IsPositive(tgtTravelTime,1.0f))
	{

		const SiegePos s = srcPlan->GetFinalDestination();
		const SiegePos e = tgtPlan->GetFinalDestination();
		if (gSiegeEngine.IsNodeInAnyFrustum(s.node) && gSiegeEngine.IsNodeInAnyFrustum(e.node))
		{
			float w = srcPlan->GetPathWidth()+tgtPlan->GetPathWidth();
			float deltalen2 = gSiegeEngine.GetDifferenceVector(s,e).Length2();

			crowdcontrol = ((	tgtPlan->IsInMeleeCrowd()
							 || (tgtPlan->CheckForBlockers(e,sourceGoid,false,false,false,true))
							 || ((w*w) > deltalen2)
					   	     || (srcPlan->CheckForBlockers(s,targetGoid,false,false,false,false))
							 )
							  &&
							((dtype == DEPTYPE_MELEE_ENEMY) || (dtype == DEPTYPE_MELEE_FRIEND) ));
		}
	}

	
	float tgt_avg_velocity = targetGo->GetBody()->GetAvgMoveVelocity();
	if (IsZero(tgt_avg_velocity))
	{
		// Don't complain if the target has no velocity
		tgt_avg_velocity = targetGo->GetAspect()->GetAspectHandle()->GetBlender()->GetScalarVelocity(CHORE_WALK, AS_PLAIN, 0);
	}

	bool collisionallowed = !crowdcontrol &&
							(srcPlan->NumDependants() == 1)
							// && srcPlan->AllowCollisionPath()
							&& srcPlan->IsOnInterceptPath(tgtPlan);						 

	if (collisionallowed)
	{
		// Is there a collision path we can derive by peeking into the plans?

		ret = tgtPlan->AppendCollisionMove( srcPlan, src_avg_velocity, range, tgt_avg_velocity, reverse );

		if (RequestOk(ret))
		{

			Plan::DependancyInfo diSrc, diTgt;
			if (   srcPlan->BuildDependancyInfo(diSrc,targetGoid,tgtPlan->GetFinalDestination(),dtype)
				&& tgtPlan->BuildDependancyInfo(diTgt,sourceGoid,srcPlan->GetFinalDestination(),dtype)) 
			{

				if (ret == REQUEST_OK_BEYOND_RANGE)
				{
					if (gWorldOptions.GetDebugMCP())
					{
						gpwarningf((
							"FIDGET/FIGHT INCONSISTENCY: Converting a \"REQUEST_OK_BEYOND_RANGE\" while mutually dependant to a \"REQUEST_OK\"."
							"This should eliminate a potential fidget/fight situation."
							));
					}
					ret = REQUEST_OK;
				}
				
				// Do not send any messages about the source's dependancy info
				// changing to the target! 
				// We know it already depended on source (earlier we tested to be sure it's on
				// an intercept path) so all we need to do is update the 
				// position the target expects the source to be at.
				srcPlan->CreateDependancyLink(diSrc);

				// Let the target know that the source now depends on it
				tgtPlan->CreateDependancyLink(diTgt);

				if (((dtype == DEPTYPE_MELEE_ENEMY) || (dtype == DEPTYPE_MELEE_FRIEND) ))
				{
					tgtPlan->CheckForBlockers(srcPlan->GetFinalDestination(),sourceGoid,false,false,false,true);
					srcPlan->CheckForBlockers(tgtPlan->GetFinalDestination(),targetGoid,false,false,false,true);
				}

				WorldMessage msg1( WE_MCP_MUTUAL_DEPENDANCY, srcPlan->GetOwner(), tgtPlan->GetOwner() , tgtPlan->GetCurrentReqBlock() );
				msg1.SendDelayed( 0.001f );
				
				WorldMessage msg2( WE_MCP_MUTUAL_DEPENDANCY, tgtPlan->GetOwner(), srcPlan->GetOwner() , srcPlan->GetCurrentReqBlock() );
				msg2.SendDelayed( 0.001f );

#if !GP_RETAIL
				srcPlan->DebugValidate();
				tgtPlan->DebugValidate();
#endif
				return ret;
			}
		}
	}
	
	else if (crowdcontrol)
	{
		// Not a lot of room to manuever if we are in a melee situation
		intercept_pos = tgtPlan->GetFinalDestination();
		ret = srcPlan->AppendMoveToCrowd( tgtPlan, range, reverse );
		if ( RequestOk(ret) )
		{
			return ret;
		}
	}

	else if (!tgtPlan->IsMoving())
	{
		// Easy to intercept a stationary target
		SiegePos initial_pos  = srcPlan->GetFinalDestination();
		intercept_pos = tgtPlan->GetFinalDestination();

		vector_3 delta = gSiegeEngine.GetDifferenceVector(initial_pos,intercept_pos);
		float diffsquared = Length2(delta);
		float minrange = range*0.8f;
		float minrangesquared = minrange*minrange;
		float maxrangesquared = range*range;

		if (!IsPositive(diffsquared-maxrangesquared,MINIMUM_DISTANCE_TOLERANCE_SQUARED))
		{
			if (diffsquared<=minrangesquared)
			{
				ret = REQUEST_OK_INSIDE_RANGE;
			}
			else
			{
				ret = REQUEST_OK_IN_RANGE;
			}
		}
		else
		{
			ret = srcPlan->AppendMoveHelper(intercept_pos, range, FLOAT_MAX, reverse, targetGoid);
		}
		if ( RequestOk(ret) )
		{
			// Create a dependancy link to the target's intercept position at the intercept time
			Plan::DependancyInfo diInt(sourceGoid,srcPlan->GetFinalDestinationTime(),intercept_pos,srcPlan->GetFinalDestination(),dtype);
			tgtPlan->CreateDependancyLink(diInt);

			if (((dtype == DEPTYPE_MELEE_ENEMY) || (dtype == DEPTYPE_MELEE_FRIEND) ))
			{
				tgtPlan->CheckForBlockers(srcPlan->GetFinalDestination(),sourceGoid,false,false,false,true);
			}

			return ret;
		}

	}
	
	// We need to do a full intercept calc
	{

		SiegePos initial_pos;
		double initial_time;

		// The path has been flushed
		initial_pos  = srcPlan->GetFinalDestination();
		initial_time = srcPlan->GetFinalDestinationTime();
		if (initial_time == DBL_MAX)
		{
			initial_time = max_t(srcPlan->GetAppendTime(),GetLeadingTime());
		}
		else
		{
			initial_time = max_t(initial_time,GetLeadingTime());
		}

		if (!gSiegeEngine.IsNodeInAnyFrustum(initial_pos.node))
		{
			gpwarning("Attempted FindInterceptPath with invalid initial position");
			return REQUEST_FAILED_BAD_PATH_BEGIN;
		}

		double intercept_time;
		
		eReqRetCode iret = tgtPlan->EstimateInterceptPosition(intercept_time,
																intercept_pos,
																initial_pos,
																initial_time,
																src_avg_velocity,
																lookahead+maxtime,
																range,
																cushion
																);

		if (RequestFailed(iret)) 
		{
			return REQUEST_FAILED_BAD_PATH_END;
		}

		bool lookahead_failed = (iret == REQUEST_OK_BEYOND_RANGE);

		vector_3 delta = gSiegeEngine.GetDifferenceVector(initial_pos,intercept_pos);

		float diffsquared = Length2(delta);
		float minrange = range*0.8f;
		float minrangesquared = minrange*minrange;
		float maxrangesquared = range*range;

		if (!IsPositive(diffsquared-maxrangesquared,MINIMUM_DISTANCE_TOLERANCE_SQUARED))
		{
			if (diffsquared<=minrangesquared)
			{
				ret = REQUEST_OK_INSIDE_RANGE;
			}
			else
			{
				ret = REQUEST_OK_IN_RANGE;
			}
		}
		else
		{
			ret = srcPlan->AppendMoveHelper(intercept_pos, range, maxtime, reverse, targetGoid);
		}

		// According to the plan just generated, will we arrive
		// at the intercept position 'early'? (not within a 1/4 second)
		if ((ret == REQUEST_OK_IN_RANGE) || (ret == REQUEST_OK_INSIDE_RANGE))
		{
			if (synch_at_end)
			{
				double apptime = srcPlan->GetAppendTime();
				if ((intercept_time != DBL_MAX) 
					&& IsPositive((float)(PreciseSubtract(intercept_time,PreciseAdd(apptime,0.25f)))))
				{
					ret = REQUEST_OK;
				}
			}

		}
		else
		{
			if (lookahead_failed)
			{
				ret = REQUEST_OK_BEYOND_RANGE;
			}
		}

		// Create a dependancy link to the target's intercept position at the intercept time
		Plan::DependancyInfo diInt(sourceGoid,intercept_time,intercept_pos,srcPlan->GetFinalDestination(),dtype);
		tgtPlan->CreateDependancyLink(diInt);

		if ( ret == REQUEST_OK_CROWDED )
		{
			gpdevreportf( &gMCPCrowdContext,("MCP FIP returned REQ_OK_CROWDED for [%s:0x%08x]\n",
				sourceGo->GetTemplateName(),
				MakeInt(sourceGo->GetGoid())
				));
		}
	}

	return ret;

}

eReqRetCode Manager::FindPathAwayFrom( Goid /*sourceGoid*/, Goid /*targetGoid*/,	float /*range*/, bool /*reverse*/ )
{
	return REQUEST_FAILED;
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type )
{

	return MakeRequest( ownerGoid, type, (int)0 , REQFLAG_DEFAULT );
}


//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, eReqFlag flags )
{

	UNREFERENCED_PARAMETER(flags);

	return MakeRequest( ownerGoid, type, (int)0 , flags);
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, DWORD subanim )
{
	return MakeRequest( ownerGoid, type, subanim , REQFLAG_DEFAULT);
}


//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, DWORD subanim, eReqFlag flags )
{
	UNREFERENCED_PARAMETER(flags);

	eReqRetCode ret = REQUEST_OK;

	GoHandle ownerGo( ownerGoid );

	if (!ownerGo.IsValid()) 
	{
		ret = REQUEST_FAILED_BAD_OWNER;
		DumpRequestInfo( ownerGoid, type, ret );
		return ret;
	}

	if( !ownerGo->HasFollower() )
	{
		// The owner is brainless so we will have to send animate requests
		// directly instead of plan requests
		switch( type )
		{
		case PL_OPEN:
			{
				ownerGo->SetBucketDirty();
				ownerGo->GetBody()->SAnimate(AnimReq( CHORE_OPEN, 0, AS_DEFAULT, subanim, 0 ));
				ret = REQUEST_OK;
				break;
			}

		case PL_CLOSE:
			{
				ownerGo->SetBucketDirty();
				ownerGo->GetBody()->SAnimate(AnimReq( CHORE_CLOSE, 0, AS_DEFAULT, subanim, 0 ));
				ret = REQUEST_OK;
				break;
			}
		default:
			{
				ret = REQUEST_FAILED_BAD_TYPE;
				break;
			}
		}

		DumpRequestInfo( ownerGoid, type, ret );

		return ret;
	}


	Plan* srcPlan = FindPlan( ownerGoid ) ;
	srcPlan->SavePendingRequest(type,0,0,subanim,0,flags);
	srcPlan->BreakDependancyLinkToParent();

	switch( type )
	{
		case PL_FIDGET:
			{
				// Append a fidget
				srcPlan->AppendChore(CHORE_FIDGET, AS_DEFAULT,subanim,0,0);
			}
			ret = REQUEST_OK;
			break;

		case PL_CAST:
			{
				// Append an untargeted casting animation
				eAnimStance stance = ownerGo->GetInventory()->GetAnimStance();
				float duration = ownerGo->GetAspect()->GetAspectPtr()->GetBlender()->GetDuration( CHORE_MAGIC, stance, subanim);
				if (IsZero(duration))
				{
					// Don't allow a ZERO duration
					duration = 1;
				}
				DWORD flg = (DWORD)flags;
				EncodeAttackParameters( duration, 0, 0, flg);
				srcPlan->AppendFaceUndefined();
				srcPlan->AppendChore(CHORE_MAGIC, AS_DEFAULT,subanim,flg,0);
			}
			ret = REQUEST_OK;
			break;

		case PL_DIE:
			{
				// Append a death animation
				srcPlan->AppendFaceUndefined();
				srcPlan->AppendChore(CHORE_DIE, AS_DEFAULT,subanim,flags,0);
			}
			ret = REQUEST_OK;
			break;

		case PL_OPEN:
			{
				ownerGo->SetBucketDirty();
				srcPlan->AppendChore(CHORE_OPEN, AS_DEFAULT,subanim,0,0);
			}
			ret = REQUEST_OK;
			break;

		case PL_CLOSE:
			{
				ownerGo->SetBucketDirty();
				srcPlan->AppendChore(CHORE_CLOSE, AS_DEFAULT,subanim,0,0);
			}
			ret = REQUEST_OK;
			break;

		case PL_FACE:
			{
				srcPlan->AppendFaceUndefined();
			}
			ret = REQUEST_OK;
			break;

		default:
			{
			}
			ret = REQUEST_FAILED_BAD_TYPE;
			break;
	}

	if (RequestOk(ret))
	{
		srcPlan->CommitPendingRequest();
	}
	else
	{
		srcPlan->ClearPendingRequest();
	}

	DumpRequestInfo( ownerGoid, type, ret );

	return ret;
}


//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, eAnimChore ch, DWORD subanim, DWORD flags )
{

	UNREFERENCED_PARAMETER(flags);

	eReqRetCode ret = REQUEST_OK;

	GoHandle ownerGo(ownerGoid);

	if ( !ownerGo.IsValid() ) 
	{
		ret = REQUEST_FAILED_BAD_OWNER;
		DumpRequestInfo( ownerGoid, type, ret );
		return ret;
	}

	if ( !ownerGo->HasFollower() )
	{
		// The owner is brainless so we will have to send animate requests
		// directly instead of plan requests
		switch( type )
		{
			case PL_PLAYANIM:
				{
					// Append a specific animation
					// $$ What about setting the framerate and other 
					// $$ parameters for the animation?
					eAnimStance stance = AS_DEFAULT;
					ownerGo->SetBucketDirty();
					ownerGo->GetBody()->SAnimate(AnimReq( ch, 0, stance, subanim, flags ));
				}
				ret = REQUEST_OK;
				break;

			default:
				{
				ret = REQUEST_FAILED_BAD_TYPE;
				}
				break;
		}

		DumpRequestInfo( ownerGoid, type, ret );
		return ret;
	}

	Plan* srcPlan = FindPlan( ownerGoid ) ;
	srcPlan->SavePendingRequest(type,0,0,subanim,0,flags);
	srcPlan->BreakDependancyLinkToParent();

	switch( type )
	{
		case PL_PLAYANIM:
			{
				// Append a specific animation
				// $$ What about setting the framerate and other 
				// $$ parameters for the animation?
				eAnimStance stance = AS_DEFAULT;
				srcPlan->AppendChore(ch,stance,subanim,flags,0);
			}
			ret = REQUEST_OK;
			break;
		default:
			{
				ret = REQUEST_FAILED_BAD_TYPE;
			}
			break;
	}

	// This is not a valid type for this MakeRequest format
	if (RequestOk(ret))
	{
		srcPlan->CommitPendingRequest();
	}
	else
	{
		srcPlan->ClearPendingRequest();
	}

	DumpRequestInfo( ownerGoid, type, ret );
	return ret;
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, SiegePos const & targetPos )
{
	return MakeRequest( ownerGoid, type, targetPos, REQFLAG_DEFAULT );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, SiegePos const & targetPos, eReqFlag flags )
{

	UNREFERENCED_PARAMETER(flags);

	eReqRetCode ret = REQUEST_OK;

	GoHandle ownerGo(ownerGoid);

	if (!ownerGo.IsValid())
	{
		ret = REQUEST_FAILED_BAD_OWNER;
	}
	else if( targetPos == SiegePos::INVALID ) 
	{
		ret = REQUEST_FAILED_BAD_TARGET;
	}
	else if( !ownerGo->HasFollower() )
	{
		// The owner is brainless
		ret = REQUEST_FAILED_MISSING_FOLLOWER;
	}

	Plan* srcPlan = FindPlan( ownerGoid ) ;
	srcPlan->SavePendingRequest(type,0,0,0,0,flags);

	bool reverse = (flags & REQFLAG_FACEREVERSE)!=0;

	if( RequestOk( ret ) )
	{
		switch( type )
		{

		case PL_FACE:
			{
				srcPlan->AppendFacePosition( targetPos, reverse );			
			}
			ret = REQUEST_OK;
			break;

		default:
			{
			}
			ret = REQUEST_FAILED_BAD_TYPE;
			break;
		}
	}

	DumpRequestInfo( ownerGoid, type, ret );

	if (RequestOk(ret))
	{
		srcPlan->CommitPendingRequest();
	}
	else
	{
		srcPlan->ClearPendingRequest();
	}

	return ret;
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, const SiegeRot& targetRot )
{
	return MakeRequest( ownerGoid, type, targetRot, REQFLAG_DEFAULT );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, const SiegeRot& targetRot, eReqFlag flags )
{

	UNREFERENCED_PARAMETER(flags);

	eReqRetCode ret = REQUEST_OK;

	GoHandle ownerGo(ownerGoid);

	if (!ownerGo.IsValid())
	{
		ret = REQUEST_FAILED_BAD_OWNER;
	}
	else if( !ownerGo->HasFollower() )
	{
		// The owner is brainless
		ret = REQUEST_FAILED_MISSING_FOLLOWER;
	}

	Plan* srcPlan = FindPlan( ownerGoid ) ;
	srcPlan->SavePendingRequest(type,0,0,0,0,flags);

	if( RequestOk( ret ) )
	{
		switch( type )
		{

		case PL_FACE:
			{
				gpassert(targetRot.rot != Quat::INVALID);
				if (targetRot.rot != Quat::INVALID)
				{
					SiegeRot t = targetRot;
					bool reverse = (flags & REQFLAG_FACEREVERSE)!=0;
					if (reverse)
					{
						t.rot.RotateY(PI);
					}
					srcPlan->AppendFaceRotation(t);
				}
			}
			ret = REQUEST_OK;
			break;

		default:
			{
			}
			ret = REQUEST_FAILED_BAD_TYPE;
			break;
		}
	}

	DumpRequestInfo( ownerGoid, type, ret );

	if (RequestOk(ret))
	{
		srcPlan->CommitPendingRequest();
	}
	else
	{
		srcPlan->ClearPendingRequest();
	}

	return ret;

}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, DWORD subanim, const SiegePos& targetPos, float range)
{
	return MakeRequest( ownerGoid, type, -1, subanim, targetPos, range, REQFLAG_DEFAULT );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, DWORD subanim, const SiegePos& targetPos, float range, eReqFlag flags)
{
	return MakeRequest( ownerGoid, type, -1, subanim, targetPos, range, flags );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, const SiegePos& targetPos, float range )
{
	return MakeRequest( ownerGoid, type, -1, 0, targetPos, range, REQFLAG_DEFAULT );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, const SiegePos& targetPos, float range, eReqFlag flags )
{
	return MakeRequest( ownerGoid, type, -1, 0, targetPos, range, flags );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, float duration, const SiegePos& targetPos, float range )
{
	return MakeRequest( ownerGoid, type, duration, 0, targetPos, range, REQFLAG_DEFAULT );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, float duration, const SiegePos& targetPos, float range, eReqFlag flags)
{
	return MakeRequest( ownerGoid, type, duration, 0, targetPos, range, flags );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, float duration, DWORD subanim, const SiegePos& targetPos, float range )
{
	return MakeRequest( ownerGoid, type, duration, subanim, targetPos, range, REQFLAG_DEFAULT );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, float duration, DWORD subanim, const SiegePos& targetPos, float range, eReqFlag flags )
{

	UNREFERENCED_PARAMETER(flags);

	eReqRetCode ret = REQUEST_OK;

	GoHandle ownerGo(ownerGoid);

	if (!ownerGo.IsValid())
	{
		ret = REQUEST_FAILED_BAD_OWNER;
	}
	else if( targetPos == SiegePos::INVALID ) 
	{
		ret = REQUEST_FAILED_BAD_TARGET;
	}
	else if (!ownerGo->HasFollower())  
	{
		ret = REQUEST_FAILED_MISSING_FOLLOWER;
	}

	Plan* srcPlan = FindPlan( ownerGoid ) ;
	srcPlan->SavePendingRequest(type,duration,0,subanim,range,flags);
	srcPlan->BreakDependancyLinkToParent();

	bool reverse = (flags & REQFLAG_FACEREVERSE)!=0;

	if( RequestOk( ret ) )
	{
		switch( type )
		{
			case PL_APPROACH:
				{
					ret = FindPathTo( ownerGoid, targetPos, range, reverse );
					if (ret == REQUEST_OK_INSIDE_RANGE)
					{
						ret = REQUEST_OK_IN_RANGE;
					}
					if (ret < REQUEST_FAILED)
					{
#if !GP_RETAIL
						if (IsZero(range))
						{
							srcPlan->SetGoalType("EXPLICIT");
						}
						else
						{
							srcPlan->SetGoalType("SLACK");
						}
#endif
						srcPlan->AppendChore( CHORE_FIDGET,AS_DEFAULT,subanim,0,0 );
						//float len = gSiegeEngine.GetDifferenceVector(srcPlan->GetFinalDestination(),targetPos).Length();
						//gpassertm( !IsPositive((len - range),0.1f), "Supposed to be able to get there");
					}
				}
				break;

			case PL_ATTACK_POSITION_MELEE:
				{
					ret = FindPathTo( ownerGoid, targetPos, range, reverse );
					if (ret == REQUEST_OK_INSIDE_RANGE)
					{
						ret = REQUEST_OK_IN_RANGE;
					}
					if (ret < REQUEST_FAILED)
					{
#if !GP_RETAIL
						if (IsZero(range))
						{
							srcPlan->SetGoalType("EXPLICIT");
						}
						else
						{
							srcPlan->SetGoalType("SLACK");
						}
#endif
						bool reverse = (flags & REQFLAG_FACEREVERSE)!=0;
						srcPlan->AppendFacePosition( targetPos, reverse );
						// Append a melee attack animation
						DWORD flg=0;
						EncodeAttackParameters ( duration,0 , 0, flg ) ;
						srcPlan->AppendChore( CHORE_ATTACK,AS_DEFAULT,subanim,flg,range);
					}
				}
				break;

			case PL_ATTACK_POSITION_RANGED:
				{
					ret = FindPathTo( ownerGoid, targetPos, range, reverse);
					if (ret == REQUEST_OK_INSIDE_RANGE)
					{
						ret = REQUEST_OK_IN_RANGE;
					}
					if (ret < REQUEST_FAILED)
					{
#if !GP_RETAIL
						if (IsZero(range))
						{
							srcPlan->SetGoalType("EXPLICIT");
						}
						else
						{
							srcPlan->SetGoalType("SLACK");
						}
#endif
						float deviation;
						if (flags & REQFLAG_NOTURN)
						{
							deviation = ApproximateFiringDeviation(srcPlan, targetPos);
						}
						else
						{
							bool reverse = (flags & REQFLAG_FACEREVERSE)!=0;
							srcPlan->AppendFacePosition( targetPos, reverse );
							deviation = 0;
						}

						// Append a projectile attack animation
						float elev=ApproximateFiringElevation(srcPlan, targetPos);
						DWORD flg=0;
						EncodeAttackParameters( duration, elev, deviation, flg);
						srcPlan->AppendChore( CHORE_ATTACK,AS_DEFAULT,subanim,flg,range);
					}
				}
				break;

			case PL_CAST_ON_POSITION:
				{

					if ((flags & REQFLAG_NOMOVE) == 0) 
					{
						ret = FindPathTo( ownerGoid, targetPos, range, reverse );
					}
					if (ret == REQUEST_OK_INSIDE_RANGE)
					{
						ret = REQUEST_OK_IN_RANGE;
					}

					if (ret < REQUEST_FAILED)
					{

#if !GP_RETAIL
						if (IsZero(range))
						{
							srcPlan->SetGoalType("EXPLICIT");
						}
						else
						{
							srcPlan->SetGoalType("SLACK");
						}
#endif

						if ((flags & REQFLAG_NOTURN) == 0)
						{
							srcPlan->AppendFacePosition( targetPos, reverse );
						}

						// Append a projectile attack animation
						DWORD flg=0;
						// $$$ Magic is handled like melee attack, no elevation/deviation
						EncodeAttackParameters( duration, 0, 0, flg);
						srcPlan->AppendChore( CHORE_MAGIC,AS_DEFAULT,subanim,flg,range);
					}

				}
				break;

			default:
				{
					ret = REQUEST_FAILED_BAD_TYPE;
				}
				break;
		};
	}
	
	if (RequestOk(ret))
	{	
		srcPlan->CommitPendingRequest();
	}
	else
	{
		srcPlan->ClearPendingRequest();
	}

	DumpRequestInfo( ownerGoid, type, ret );

	// This is not a valid type for this MakeRequest
	return ret;

}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, Goid targetGoid)
{
	return MakeRequest( ownerGoid, type, targetGoid, REQFLAG_DEFAULT );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, Goid targetGoid, eReqFlag flags)
{

	UNREFERENCED_PARAMETER(flags);

	eReqRetCode ret = REQUEST_OK;

	GoHandle ownerGo(ownerGoid);
	GoHandle targetGo(targetGoid);

	if( !ownerGo.IsValid() )
	{
		ret = REQUEST_FAILED_BAD_OWNER;
	}
	else if( !targetGo.IsValid() )
	{
		ret = REQUEST_FAILED_BAD_TARGET;
	}
	else if( !ownerGo->HasFollower() )
	{
		ret = REQUEST_FAILED_MISSING_FOLLOWER;
	}

	Plan* srcPlan = FindPlan( ownerGoid ) ;
	srcPlan->SavePendingRequest(type,-1,0,0,0,flags);

	if( RequestOk( ret ) )
	{
		switch( type )
		{

		case PL_FOLLOW:
			{					
				ret = REQUEST_OK;
				if (targetGoid == GOID_INVALID)
				{
					srcPlan->AppendLockMovementToGo(GOID_INVALID);
				}
				else
				{
					GoHandle targetGo(targetGoid);
					if ( !targetGo->HasFollower() )
					{
						srcPlan->AppendLockMovementToGo(GOID_INVALID);
						ret = REQUEST_FAILED;
					}
					else
					{
						srcPlan->AppendLockMovementToGo(targetGoid);
					}
				}
			}
			break;

		case PL_FACE:
			{
			if ((flags & REQFLAG_NOTURN) == 0)
			{
				bool reverse = (flags & REQFLAG_FACEREVERSE)!=0;
				srcPlan->AppendFaceTarget( targetGoid, reverse, false );
			}
			else
			{
				// We know who we want to face, but we can't turn the actor ourselves
				// This is used for objects that resolve their facing direction on the client
				srcPlan->AppendFaceTarget( targetGoid, false, true );
			}
			ret = REQUEST_OK;
			break;
			}
		default:
			{
				ret = REQUEST_FAILED_BAD_TYPE;
			}
			break;
		}
	}

	if (RequestOk(ret))
	{	
		srcPlan->CommitPendingRequest();
	}
	else
	{
		srcPlan->ClearPendingRequest();
	}

	DumpRequestInfo( ownerGoid, type, ret );

	return ret;
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, Goid targetGoid, float lookahead, float maxtime, float range, eReqFlag flags )
{
	return MakeRequest( ownerGoid, type, -1, 0, targetGoid, lookahead, maxtime, range, flags );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, DWORD subanim, Goid targetGoid, float lookahead, float maxtime, float range, eReqFlag flags)
{
	return MakeRequest( ownerGoid, type, -1, subanim, targetGoid, lookahead, maxtime, range, flags );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, Goid targetGoid, float lookahead, float maxtime, float range )
{
	return MakeRequest( ownerGoid, type, -1, 0, targetGoid, lookahead, maxtime, range, REQFLAG_DEFAULT );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, DWORD subanim, Goid targetGoid, float lookahead, float maxtime, float range)
{
	return MakeRequest( ownerGoid, type, -1, subanim, targetGoid, lookahead, maxtime, range, REQFLAG_DEFAULT );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, float duration, Goid targetGoid, float lookahead, float maxtime, float range )
{
	return MakeRequest( ownerGoid, type, duration, 0, targetGoid, lookahead, maxtime, range, REQFLAG_DEFAULT );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, float duration, Goid targetGoid, float lookahead, float maxtime, float range, eReqFlag flags )
{
	return MakeRequest( ownerGoid, type, duration, 0, targetGoid, lookahead, maxtime, range, flags );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, float duration, DWORD subanim, Goid targetGoid, float lookahead, float maxtime, float range )
{
	return MakeRequest( ownerGoid, type, duration, subanim, targetGoid, lookahead, maxtime, range, REQFLAG_DEFAULT );
}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Manager::MakeRequest( Goid ownerGoid, eRequest type, float duration, DWORD subanim, Goid targetGoid, float lookahead, float maxtime, float range, eReqFlag flags )
{
	UNREFERENCED_PARAMETER(flags);

	eReqRetCode ret = REQUEST_OK;

	GoHandle ownerGo(ownerGoid);
	GoHandle targetGo(targetGoid);

	if( !ownerGo.IsValid() )
	{
		ret = REQUEST_FAILED_BAD_OWNER;
	}
	else if (!targetGo.IsValid() )
	{
		ret = REQUEST_FAILED_BAD_TARGET;
	}
	else if ( !ownerGo->HasFollower() )
	{
		ret = REQUEST_FAILED_MISSING_FOLLOWER;
	}

	Plan* srcPlan = FindPlan( ownerGoid ) ;
	srcPlan->SavePendingRequest(type,duration,targetGoid,subanim,range,flags);

	bool reverse = (flags & REQFLAG_FACEREVERSE)!=0;

	if( RequestOk( ret ) )
	{
		switch( type )
		{
			case PL_APPROACH:
				{
					if ( !targetGo->HasFollower() )
					{
						ret = FindPathTo( ownerGoid, targetGoid, range, reverse, DEPTYPE_PLAIN );
					}
					else
					{
						float cushion = duration*0.5f;
						ret = FindInterceptPath( ownerGoid, targetGoid, lookahead, maxtime, range, cushion, true, reverse, DEPTYPE_PLAIN );
					}
					if (ret < REQUEST_FAILED)
					{
#if !GP_RETAIL
						srcPlan->SetGoalType("ADJUSTABLE");
#endif
						srcPlan->AppendChore( CHORE_FIDGET,AS_DEFAULT,subanim,0,0 );
						if (ret == REQUEST_OK_INSIDE_RANGE)
						{
							ret = REQUEST_OK_IN_RANGE;
						}
					}
				}
				break;

			case PL_INTERCEPT:
				{
					GoHandle targetGo(targetGoid);

					if ( !targetGo->HasFollower())
					{
						ret = FindPathTo( ownerGoid, targetGoid, range, reverse, DEPTYPE_PLAIN );
					}
					else
					{
						float cushion = duration*0.5f;
						ret = FindInterceptPath( ownerGoid, targetGoid, lookahead, maxtime, range, cushion ,true ,reverse, DEPTYPE_PLAIN );
					}
					if (ret < REQUEST_FAILED)
					{
#if !GP_RETAIL
						srcPlan->SetGoalType("ADJUSTABLE");
#endif
						if ((flags & REQFLAG_NOTURN) == 0)
						{
							bool reverse = (flags & REQFLAG_FACEREVERSE)!=0;
							srcPlan->AppendFaceTarget( targetGoid, reverse, false );
						}
						else
						{
							// We know who we want to face, but we can't turn the actor ourselves
							// This is used for objects that resolve their facing direction on the client
							srcPlan->AppendFaceTarget( targetGoid, false, true );
						}
						if (ret == REQUEST_OK_INSIDE_RANGE)
						{
							ret = REQUEST_OK_IN_RANGE;
						}
					}
				}
				break;

			case PL_AVOID:
				{
					ret = FindPathAwayFrom( ownerGoid, targetGoid, range, reverse );
					// Stay beyond a specified distance
				}
				break;

			case PL_ATTACK_OBJECT_MELEE:
				{
					if ((flags & REQFLAG_NOMOVE) == 0) 
					{
						GoHandle targetGo(targetGoid);
						if ( !targetGo.IsValid() ) return REQUEST_FAILED_BAD_TARGET;

						if ( !targetGo->HasFollower() )
						{
							ret = FindPathTo( ownerGoid, targetGoid, range, reverse, DEPTYPE_MELEE_ENEMY );
						}
						else
						{
							float cushion = duration*0.5f;
							ret = FindInterceptPath( ownerGoid, targetGoid, lookahead, maxtime, range, cushion, true, reverse, DEPTYPE_MELEE_ENEMY );
						}
					}
					else
					{
						if ((flags & REQFLAG_NOTURN) == 0)
						{
							bool reverse = (flags & REQFLAG_FACEREVERSE)!=0;
							srcPlan->AppendFaceTarget( targetGoid, reverse, false );
						}
						else
						{
							// We know who we want to face, but we can't turn the actor ourselves
							// This is used for objects that resolve their facing direction on the client
							srcPlan->AppendFaceTarget( targetGoid, false, true );
						}
						ret = REQUEST_OK_IN_RANGE;
					}

					if (RequestOk(ret))
					{
#if !GP_RETAIL
						srcPlan->SetGoalType("ADJUSTABLE");
#endif
						// We need to save out the attack parameters in case the
						// target decides to attack us back and create a mutual dependancy
						// in that case we will need to build (or possibly re-build) the chore
						if (RequestBeyondRange(ret))
						{
							// Could we select a random fidget?
							// find the shortest fidget animation to insert onto the end of the walk.
							gpassert( ownerGo->GetAspect() && ownerGo->GetAspect()->GetAspectHandle() && ownerGo->GetAspect()->GetAspectHandle()->GetBlender() );
							
							eAnimStance stance = AS_DEFAULT;
							// current equipment tells us which stance to use
							GoInventory* inv = ownerGo->QueryInventory();
							if ( inv != NULL )
							{
								stance = inv->GetAnimStance();
							}
							// may still be default
							if ( stance == AS_DEFAULT )
							{
								stance = AS_PLAIN;
							}

							int minDurationAnimation = ownerGo->GetAspect()->GetAspectHandle()->GetBlender()->GetMinDurationAnim( CHORE_FIDGET, stance);
							srcPlan->AppendChore(CHORE_FIDGET, AS_DEFAULT, minDurationAnimation,0,0);

							#if 0 && !GP_RETAIL
								if(  ownerGo->GetAspect()->GetAspectHandle()->GetBlender()->GetDuration(CHORE_FIDGET, stance, minDurationAnimation) > 2 )
								{
									gpwarningf(( 
									"FIDGET FIGHT?: [%s(%s):0x%08x] could not quickly make it to the "
									"target of its attack.  The shortest fidget [CHORE_FIDGET] for stance: %s found to append "
									"to the end of the walk cycle is %g seconds long!!!! "
									"This is a _content problem_, the template needs to be updated "
									"to include at least one fidget shorter than 1 second long. "
									"Only ignore this error once, it will come up for different monsters",
									ownerGo->GetTemplateName(),
									ownerGo->GetAspect()->GetAspectHandle()->GetBlender()->GetKeys( CHORE_FIDGET, stance, minDurationAnimation )->GetDebugName(),
									ownerGoid,
									::ToString(stance),
									ownerGo->GetAspect()->GetAspectHandle()->GetBlender()->GetDuration(CHORE_FIDGET, stance, minDurationAnimation)
									));
								}
							#endif
							
						}
						else
						{
							bool reverse = (flags & REQFLAG_FACEREVERSE)!=0;
							DWORD flg=0;
							EncodeAttackParameters ( duration,0,0,flg );

							if ((flags & REQFLAG_NOTURN) == 0)
							{
								srcPlan->AppendFaceTarget( targetGoid, reverse, false );
							}
							else
							{
								// We know who we want to face, but we can't turn the actor ourselves
								// This is used for objects that resolve their facing direction on the client (like the GRS)
								srcPlan->AppendFaceTarget( targetGoid, false, true );
							}

							srcPlan->AppendChore( CHORE_ATTACK,AS_DEFAULT,subanim,flg,range);
							if (ret == REQUEST_OK_INSIDE_RANGE)
							{
								ret = REQUEST_OK_IN_RANGE;
							}
						}

					}

				}
				break;

			case PL_ATTACK_OBJECT_RANGED:
				{

					if ((flags & REQFLAG_NOMOVE) == 0) 
					{
						if ( !targetGo->HasFollower() )
						{
							ret = FindPathTo( ownerGoid, targetGoid, range, reverse, DEPTYPE_PROJECTILE );
						}
						else
						{
							float cushion = duration*0.5f;
							ret = FindInterceptPath( ownerGoid, targetGoid, lookahead, maxtime, range, cushion, false, reverse, DEPTYPE_PROJECTILE );
						}

					}
					if (ret < REQUEST_FAILED)
					{

#if !GP_RETAIL
						srcPlan->SetGoalType("ADJUSTABLE");
#endif
						
						Plan* tgtPlan = gMCP.FindPlan(targetGoid,false);

						SiegePos tgtPos;

						if (tgtPlan) 
						{
							double appendtime = srcPlan->GetAppendTime();
							tgtPos = tgtPlan->GetPosition(appendtime,true,false);
							GoHandle targetGo(targetGoid);
							if( targetGo->HasActor() )
							{
								targetGo->GetBody()->GetBoneTranslator().GetPosition( targetGo, BoneTranslator::KILL_BONE, tgtPos );
							}
						}
						else
						{
							GoHandle targ(targetGoid);
							// Get current pos
							tgtPos = targ->GetPlacement()->GetPosition();
						}

						float elev=ApproximateFiringElevation(srcPlan, tgtPos);

						float deviation;
						if (flags & REQFLAG_NOTURN)
						{
							deviation = ApproximateFiringDeviation(srcPlan, tgtPos);
						}
						else
						{
							if ((flags & REQFLAG_NOTURN) == 0)
							{
								bool reverse = (flags & REQFLAG_FACEREVERSE)!=0;
								srcPlan->AppendFaceTarget( targetGoid, reverse, false );
							}
							else
							{
								// We know who we want to face, but we can't turn the actor ourselves
								// This is used for objects that resolve their facing direction on the client
								srcPlan->AppendFaceTarget( targetGoid, false, true );
							}
							deviation = 0;
						}

						DWORD flg=0;
						EncodeAttackParameters ( duration, elev, deviation, flg );

						srcPlan->AppendChore( CHORE_ATTACK,AS_DEFAULT,subanim,flg,range);

						if (ret == REQUEST_OK_INSIDE_RANGE)
						{
							ret = REQUEST_OK_IN_RANGE;
						}
					}
				}
				break;

			case PL_CAST_ON_OBJECT:
				{
					// make sure that the target is valid.
					gpassert( targetGo.IsValid() );
					if ( !targetGo.IsValid() ) return REQUEST_FAILED_BAD_TARGET;

					if ((flags & REQFLAG_NOMOVE) == 0) 
					{
						if ( !targetGo->HasFollower() )
						{
							ret = FindPathTo( ownerGoid, targetGoid, range, reverse, DEPTYPE_PROJECTILE );
						}
						else
						{
							float cushion = duration*0.5f;

							// $$ If the range is small, we will want to set the 'avoidcrowd' parameter to TRUE
							ret = FindInterceptPath( ownerGoid, targetGoid, lookahead, maxtime, range, cushion, false, reverse, DEPTYPE_PROJECTILE );
						}
					}

					if (RequestOk(ret) )
					{
						// We need to save out the attack parameters in case the
						// target decides to attack us back and create a mutual dependancy
						// in that case we will need to build (or possibly re-build) the chore
						if (RequestBeyondRange(ret))
						{
							// Could we select a random fidget?
							// find the shortest fidget animation to insert onto the end of the walk.
							gpassert( ownerGo->GetAspect() && ownerGo->GetAspect()->GetAspectHandle() && ownerGo->GetAspect()->GetAspectHandle()->GetBlender() );

							eAnimStance stance = AS_DEFAULT;
							// current equipment tells us which stance to use
							GoInventory* inv = ownerGo->QueryInventory();
							if ( inv != NULL )
							{
								stance = inv->GetAnimStance();
							}
							// may still be default
							if ( stance == AS_DEFAULT )
							{
								stance = AS_PLAIN;
							}

							int minDurationAnimation = ownerGo->GetAspect()->GetAspectHandle()->GetBlender()->GetMinDurationAnim( CHORE_FIDGET, stance);
							srcPlan->AppendChore(CHORE_FIDGET, AS_DEFAULT, minDurationAnimation,0,0);

							#if 0 && !GP_RETAIL
								if(  ownerGo->GetAspect()->GetAspectHandle()->GetBlender()->GetDuration(CHORE_FIDGET, stance, minDurationAnimation) > 2 )
								{
									gpwarningf(( 
									"FIDGET FIGHT?: [%s(%s):0x%08x] could not quickly make it to the "
									"target of its attack.  The shortest fidget for [CHORE_FIDGET] stance: [%s] found to append "
									"to the end of the walk cycle is %g seconds long!!!! "
									"This is a _content problem_, the template needs to be updated "
									"to include at least one fidget shorter than 1 second long. "
									"Only ignore this error once, it will come up for different monsters",
									ownerGo->GetTemplateName(),
									ownerGo->GetAspect()->GetAspectHandle()->GetBlender()->GetKeys( CHORE_FIDGET, stance, minDurationAnimation )->GetDebugName(),
									ownerGoid,
									::ToString(stance),
									ownerGo->GetAspect()->GetAspectHandle()->GetBlender()->GetDuration(CHORE_FIDGET, stance, minDurationAnimation)
									));
								}
							#endif
						}
						else
						{
#if !GP_RETAIL
							srcPlan->SetGoalType("ADJUSTABLE");
#endif
							
							if ((flags & REQFLAG_NOTURN) == 0)
							{
								bool reverse = (flags & REQFLAG_FACEREVERSE)!=0;
								srcPlan->AppendFaceTarget( targetGoid, reverse, false);
							}
							else
							{
								// We know who we want to face, but we can't turn the actor ourselves
								// This is used for objects that resolve their facing direction on the client
								srcPlan->AppendFaceTarget( targetGoid, false, true );
							}

							// Append a projectile attack animation
							DWORD flg=0;
							// $$$ Magic is handled like melee attack, no elevation/deviation
							EncodeAttackParameters( duration, 0, 0, flg);
							srcPlan->AppendChore( CHORE_MAGIC,AS_DEFAULT,subanim,flg,range);
							if (ret == REQUEST_OK_INSIDE_RANGE)
							{
								ret = REQUEST_OK_IN_RANGE;
							}
						}
					}
					
					// make sure our last chore isn't a walk, we shouldn't ever be walking at the end.
					#if !GP_RETAIL
					if( srcPlan->GetLastChore() == CHORE_WALK )
					{
						GoHandle hGo( ownerGoid );
						gperrorf(( 
							"RUNNING FIGHT?: [%s:0x%08x] thinks its attacking, "
							"last chore is [%s] (not [CHORE_MAGIC, or CHORE_FIDGET])."
							"You can safely ignore this error, though it _MAY_ result in an "
							"actor that will appear to be running in place while in combat."
							"Please watch carefully when play resumes for a \"running fighter\"",
							hGo->GetTemplateName(),
							ownerGoid,
							::ToString(srcPlan->GetLastChore())
							));
					}
					#endif	

				}
				break;

			default:
				{
					ret = REQUEST_FAILED_BAD_TYPE;
				}
				break;
		};
	}

	if (RequestOk(ret))
	{
		srcPlan->CommitPendingRequest();
	}
	else
	{
		srcPlan->BreakDependancyLinkToParent();
		srcPlan->ClearPendingRequest();
	} 

	DumpRequestInfo( ownerGoid, type, ret );

	return ret;
}

////////////////////////////////////////////////////////////////////////////////////
// Reconstruct the dependancy if the src is still close enough to the target
bool Manager::ReconstructDependancy(Goid src, Goid tgt, float reqrange)
{
	bool ret = false;

	Plan* srcPlan = gMCP.FindPlan(src,false);
	Plan* tgtPlan = gMCP.FindPlan(tgt,false);
	 
	if (srcPlan && tgtPlan)
	{
		if (PreciseSubtract(tgtPlan->GetFinalDestinationTime(), GetLeadingTime()) < 0.5f)
		{

			SiegePos srcPos = srcPlan->GetFinalDestination();
			SiegePos tgtPos = tgtPlan->GetFinalDestination();
			
			if (gSiegeEngine.IsNodeInAnyFrustum(tgtPos.node) && gSiegeEngine.IsNodeInAnyFrustum(srcPos.node))
			{
				eRequest rt = srcPlan->GetLastRequestType();
				eAnimChore lch = srcPlan->GetLastChore();
				eDependancyType dtype;
			
				if (rt == PL_ATTACK_OBJECT_MELEE)
				{
					if (lch != CHORE_ATTACK)
					{
						return false;
					}
					dtype = DEPTYPE_MELEE_ENEMY;
				}
				else if ((rt == PL_ATTACK_OBJECT_RANGED) || (rt == PL_CAST_ON_OBJECT))
				{
					if ( (lch != CHORE_ATTACK) || (lch != CHORE_MAGIC) )
					{
						return false;
					}
					dtype = DEPTYPE_PROJECTILE;
				}
				else if (rt == PL_APPROACH)
				{
					// Should it be considered a friend... or not (does it REALLY matter)
					if (lch != CHORE_FIDGET)
					{
						return false;
					}
					dtype = DEPTYPE_MELEE_FRIEND;
				}
				else
				{
					if (lch != CHORE_FIDGET)
					{
						return false;
					}
					dtype = DEPTYPE_PLAIN;
				}

				float vl2 = gSiegeEngine.GetDifferenceVector(tgtPos,srcPos).Length2();
				if (!IsNegative(((reqrange*reqrange)-vl2),0.01f))
				{
					if (!srcPlan->CheckForBlockers(srcPos,tgt,false,false,true,false ))
					{
						Plan::DependancyInfo di;
						di.m_Owner = srcPlan->GetOwner();
						di.m_RequiredPosition = tgtPos;
						di.m_Time = srcPlan->GetFinalDestinationTime();
						di.m_ReservedPosition = srcPos;
						di.m_RequiredDistance = sqrtf(vl2);
						di.m_Type = dtype;
						tgtPlan->CreateDependancyLink(di);
						ret = true;
					}
				}
			}
		}
		else
		{

		}
	}
	return ret;
}



//////////////////////////////////////////////////////////////////////////////
bool Manager::MessagesAreInSync( Goid ownerGoid, eRID reqblock, eWorldEvent evt )
{

	// The messages are NOT 'in sync' if they are not in the current reqblock

	if (reqblock == RID_OVERRIDE)
	{
		return true;
	}
		
/*
	if (evt != WE_MCP_SECTION_COMPLETED)
	{
		return true;
	}
*/	
	// Make sure that the section completed is for this plan
	Plan* srcPlan = FindPlan( ownerGoid,false ) ;

	if ((evt == WE_ANIM_SFX) || (evt == WE_ANIM_OTHER))
	{
		// SFX & OTHER message don't send the Request Block
		// If this becomes a problem, I may have to stuff the ReqBlock in the 
		// upper 16 bits of the Data1 element or something
		return true;
	}

	if (!srcPlan || (reqblock == srcPlan->GetCurrentReqBlock()))
	{
		return true;
	}
#if !GP_RETAIL
	if (gWorldOptions.GetDebugMCP() )
	{
		gpdevreportf( &gMCPMessageContext,("MCP: Out-of-sync [%s:%s] [%s] submitted [%04x] current [%04x]\n",
			GetGo(ownerGoid)->GetTemplateName(),
			::ToString(ownerGoid).c_str(),
			::ToString(evt),
			reqblock,
			srcPlan->GetCurrentReqBlock())
 		);
	}
#endif

	return false;
}

//////////////////////////////////////////////////////////////////////////////
bool Manager::GetLastChoreRequestedIsStillValid( Goid owner, eAnimChore ch, eAnimStance st )
{
	Go* ownGo = GetGo(owner);
	Plan* ownPlan = gMCP.FindPlan(owner,false);

	bool stillvalid;

	if (ownGo && ownPlan)
	{
		if (ownGo->GetAspect()->GetAspectHandle()->BlenderIsIdle())
		{
			stillvalid = false;
		}
		else
		{
			eAnimChore lchore;
			eAnimStance lstance;
			ownPlan->GetLastChoreAndStance(lchore,lstance);
			stillvalid = (lchore == ch) && (lstance == st);
		}
	}
	else
	{
		stillvalid = false;
	}

	return stillvalid;
}

//////////////////////////////////////////////////////////////////////////////
float Manager::GetLastRequestTimeRemaining( Goid owner )
{
	// Have we almost completed the last request in the plan?

	Plan* ownPlan = FindPlan(owner,false);

	if (ownPlan)
	{
		double fdt = ownPlan->GetFinalDestinationTime();
		if ( ( fdt != DBL_MAX ) && ( fdt > m_LeadingTime ) )
		{
			return (float)(PreciseSubtract(fdt, m_LeadingTime));
		}
	}

	return 0.0f;
}

//////////////////////////////////////////////////////////////////////////////
const DWORD RESERVED_FLAG_BITS = 4;
const DWORD START_AT_ANIM_END_MASK = 1<<0;
const DWORD PLAY_IN_REVERSE_MASK   = 1<<1;
const DWORD PACKED_PARAMETERS_MASK = 1<<3;

const float MAX_ATTACK_LOOP_DURATION = 60.0f;
const DWORD ATTACK_LOOP_DURATION_BITS = 12;
const DWORD ATTACK_LOOP_DURATION_SHIFT = RESERVED_FLAG_BITS;
const DWORD ATTACK_LOOP_DURATION_MASK  = (1 << ATTACK_LOOP_DURATION_BITS) - 1;
const float ATTACK_LOOP_DURATION_SCALE = MAX_ATTACK_LOOP_DURATION/ATTACK_LOOP_DURATION_MASK;

const float MAX_ATTACK_ELEVATION = 1.0f;
const DWORD ATTACK_ELEVATION_BITS = 8;
const DWORD ATTACK_ELEVATION_SHIFT = ATTACK_LOOP_DURATION_BITS+ATTACK_LOOP_DURATION_SHIFT;
const DWORD ATTACK_ELEVATION_MASK  = (1 << ATTACK_ELEVATION_BITS) - 1;
const float ATTACK_ELEVATION_SCALE = MAX_ATTACK_ELEVATION/ATTACK_ELEVATION_MASK;

const float MAX_ATTACK_DEVIATION = 1.0f;
const DWORD ATTACK_DEVIATION_BITS = 6;
const DWORD ATTACK_DEVIATION_SHIFT = ATTACK_ELEVATION_BITS+ATTACK_ELEVATION_SHIFT;
const DWORD ATTACK_DEVIATION_MASK  = (1 << ATTACK_DEVIATION_BITS) - 1;
const float ATTACK_DEVIATION_SCALE = MAX_ATTACK_DEVIATION/ATTACK_DEVIATION_MASK;

const float MAX_CHORE_ELAPSED = 1.0f;
const DWORD CHORE_ELAPSED_BITS = 32-RESERVED_FLAG_BITS;
const DWORD CHORE_ELAPSED_SHIFT = RESERVED_FLAG_BITS;
const DWORD CHORE_ELAPSED_MASK  = (1 << CHORE_ELAPSED_BITS) - 1;
const float CHORE_ELAPSED_SCALE = MAX_CHORE_ELAPSED/CHORE_ELAPSED_MASK;

COMPILER_ASSERT( (RESERVED_FLAG_BITS+ATTACK_LOOP_DURATION_BITS+ATTACK_ELEVATION_BITS+ATTACK_DEVIATION_BITS) <= 32);
COMPILER_ASSERT( (RESERVED_FLAG_BITS+CHORE_ELAPSED_BITS) <= 32);

//////////////////////////////////////////////////////////////////////////////
inline float UnpackIt( const DWORD packed, DWORD shift, DWORD mask, float scale)
{
	return ((packed >> shift) & mask) * scale;
}

//////////////////////////////////////////////////////////////////////////////
inline DWORD PackIt( float val, DWORD shift, DWORD mask, float scale)
{
	return ((DWORD)(val / scale) & mask) << shift;
}

//////////////////////////////////////////////////////////////////////////////
float DecodeAttackLoopDuration( DWORD flags)
{
	bool hasparams = (flags & PACKED_PARAMETERS_MASK) != 0;

	float duration = 0.0;
	if (hasparams) 
	{
		duration = UnpackIt(flags,ATTACK_LOOP_DURATION_SHIFT,ATTACK_LOOP_DURATION_MASK,ATTACK_LOOP_DURATION_SCALE);
	}
	else
	{
		duration = 0.0;
	}

	return duration;
}

//////////////////////////////////////////////////////////////////////////////
float DecodeAttackElevation( DWORD flags )
{
	bool hasparams = (flags & PACKED_PARAMETERS_MASK)  != 0;

	float elevation;
	if (hasparams) 
	{
		elevation = UnpackIt(flags,ATTACK_ELEVATION_SHIFT,ATTACK_ELEVATION_MASK,ATTACK_ELEVATION_SCALE);
		elevation -= MAX_ATTACK_ELEVATION/2;	// Elevation is a signed value
	}
	else
	{
		elevation = 0.0;
	}

	return elevation;
}

//////////////////////////////////////////////////////////////////////////////
float DecodeAttackDeviation( DWORD flags )
{
	bool hasparams = (flags & PACKED_PARAMETERS_MASK)  != 0;

	float deviation;
	if (hasparams) 
	{
		deviation = UnpackIt(flags,ATTACK_DEVIATION_SHIFT,ATTACK_DEVIATION_MASK,ATTACK_DEVIATION_SCALE);
		deviation -= MAX_ATTACK_DEVIATION/2;	// Deviation is a signed value
	}
	else
	{
		deviation = 0.0;
	}

	return deviation;
}

//////////////////////////////////////////////////////////////////////////////
bool EncodeAttackParameters( float loopdur, float elevation, float deviation, DWORD& flags)
{

	bool ok = true;

	if (loopdur < 0.0)
	{
		ok = false;
		loopdur = 0;
	}
	else if ( loopdur > MAX_ATTACK_LOOP_DURATION )
	{
		ok = false;
		loopdur = MAX_ATTACK_LOOP_DURATION;
	}

	// Bias the signed elevation
	elevation += MAX_ATTACK_ELEVATION/2;
	if (elevation < 0.0)
	{
		ok = false;
		elevation = 0;
	}
	else if ( elevation > MAX_ATTACK_ELEVATION )
	{
		ok = false;
		elevation = MAX_ATTACK_ELEVATION;
	}

	// Bias the signed deviation
	deviation += MAX_ATTACK_DEVIATION/2;
	if (deviation < 0.0)
	{
		ok = false;
		deviation = 0;
	}
	else if ( deviation > MAX_ATTACK_DEVIATION )
	{
		ok = false;
		deviation = MAX_ATTACK_DEVIATION;
	}

	flags &= ~((ATTACK_LOOP_DURATION_MASK<<ATTACK_LOOP_DURATION_SHIFT)|
	           (ATTACK_ELEVATION_MASK<<ATTACK_ELEVATION_SHIFT)|
			   (ATTACK_DEVIATION_MASK<<ATTACK_DEVIATION_SHIFT)
			  );

	flags |= PACKED_PARAMETERS_MASK;

	flags |= PackIt(loopdur,ATTACK_LOOP_DURATION_SHIFT,ATTACK_LOOP_DURATION_MASK,ATTACK_LOOP_DURATION_SCALE);
	flags |= PackIt(elevation,ATTACK_ELEVATION_SHIFT,ATTACK_ELEVATION_MASK,ATTACK_ELEVATION_SCALE);
	flags |= PackIt(deviation,ATTACK_DEVIATION_SHIFT,ATTACK_DEVIATION_MASK,ATTACK_DEVIATION_SCALE);

	return ok;

}	

//////////////////////////////////////////////////////////////////////////////
float DecodeChoreElapsedTime( DWORD flags )
{
	bool hasparams = (flags & PACKED_PARAMETERS_MASK)  != 0;

	float elapsed = 0.0;
	if (hasparams) 
	{
		elapsed = UnpackIt(flags,CHORE_ELAPSED_SHIFT,CHORE_ELAPSED_MASK,CHORE_ELAPSED_SCALE);
	}

	return elapsed;
}

//////////////////////////////////////////////////////////////////////////////
DWORD DecodeChoreFlags( DWORD flags )
{

	return flags & (START_AT_ANIM_END_MASK | PLAY_IN_REVERSE_MASK);
}

//////////////////////////////////////////////////////////////////////////////
bool EncodeChoreParameters( float elapsed, DWORD& flags)
{
	bool ok = true;

	if (elapsed < 0.0)
	{
		ok = false;
		elapsed = 0;
	}
	else if ( elapsed > MAX_CHORE_ELAPSED )
	{
		ok = false;
		elapsed = MAX_CHORE_ELAPSED;
	}

	flags &= ~(CHORE_ELAPSED_MASK<<CHORE_ELAPSED_SHIFT);
	flags |= PACKED_PARAMETERS_MASK;
	flags |= PackIt(elapsed,CHORE_ELAPSED_SHIFT,CHORE_ELAPSED_MASK,CHORE_ELAPSED_SCALE);

	return ok;
}
