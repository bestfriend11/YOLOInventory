/*==================================================================================================

	MCP_Plan -- MCP support code

	purpose:	Provides support for maintaining PLAN information 

	author:

  (c)opyright Gas Powered Games, 2001

--------------------------------------------------------------------------------------------------*/

#include "Precomp_World.h"
#include "filter_1.h"
#include "GoAspect.h"
#include "GoBody.h"
#include "GoFollower.h"
#include "GoInventory.h"
#include "GoMind.h"
#include "MCP.h"
#include "Nema_Blender.h"
#include "ReportSys.h"
#include "Siege_Logical_Node.h"
#include "World.h"
#include "WorldMap.h"
#include "WorldOptions.h"
#include "WorldPathFinder.h"
#include "WorldTime.h"

using namespace MCP;
using namespace siege;

////////////////////////////////////////////////////////////////////////////////////
// enum eDEPTYPE support

static const char* s_DEPTYPEStrings[] =
{
	"deptype_plain",
	"deptype_melee_enemy",
	"deptype_melee_friend",
	"deptype_projectile",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_DEPTYPEStrings ) == DEPTYPE_COUNT );

static stringtool::EnumStringConverter s_DEPTYPEConverter( s_DEPTYPEStrings, DEPTYPE_BEGIN, DEPTYPE_END, DEPTYPE_PLAIN);

const char* ToString( eDependancyType e )
	{  return ( s_DEPTYPEConverter.ToString( e ) );  }
bool FromString( const char* str, eDependancyType& e )
	{  return ( s_DEPTYPEConverter.FromString( str, rcast <int&> ( e ) ) );  }

FUBI_EXPORT_ENUM(eDependancyType, DEPTYPE_BEGIN , DEPTYPE_COUNT );
FUBI_DECLARE_ENUM_TRAITS( eDependancyType, DEPTYPE_PLAIN );

// Keep a table of the last chores for each type of request
// This
static const eAnimChore PlanChoreLookup[] =
{
	CHORE_NONE,		//	"pl_none",
	CHORE_FIDGET,	//	"pl_approach",
	CHORE_FIDGET,	//	"pl_follow",
	CHORE_FIDGET,	//	"pl_intercept",
	CHORE_FIDGET,	//	"pl_avoid",
	CHORE_FIDGET,	//	"pl_face",
	CHORE_DIE,		//	"pl_die",
	CHORE_ATTACK,	//	"pl_attack_object_melee",
	CHORE_ATTACK,	//	"pl_attack_object_ranged",
	CHORE_ATTACK,	//	"pl_attack_position_melee",
	CHORE_ATTACK,	//	"pl_attack_position_ranged",
	CHORE_MAGIC,	//	"pl_cast",
	CHORE_MAGIC,	//	"pl_cast_on_object",
	CHORE_MAGIC,	//	"pl_cast_on_position",
	CHORE_FIDGET,	//	"pl_fidget",
	CHORE_FIDGET,	//	"pl_equip",
	CHORE_OPEN,		//	"pl_open",
	CHORE_CLOSE,	//	"pl_close",
	CHORE_NONE,		//	"pl_wait_for_contact",
	CHORE_MISC,		//	"pl_playanim",
};

////////////////////////////////////////////////////////////////////////////////////
Plan::Plan(Goid goid, double init_time)
	: m_Owner(goid)
	, m_AppendTime(init_time)
	, m_ClientSynchTime(0)
	, m_IsLockedOn(false)
	, m_CurrentReqBlock(0)
	, m_LastRequestRID(RID_DUMMYSAVED)
	, m_LastRequestType(PL_NONE)
	, m_LastRequestDuration(0)
	, m_LastRequestTarget(0)
	, m_LastRequestSubAnim(0)
	, m_LastRequestRange(0)
	, m_LastRequestFlags(0)
	, m_PendingRequestRID(RID_DUMMYSAVED)
	, m_PendingRequestType(PL_NONE)
	, m_PendingRequestDuration(0)
	, m_PendingRequestTarget(0)
	, m_PendingRequestSubAnim(0)
	, m_PendingRequestRange(0)
	, m_PendingRequestFlags(0)
	, m_DepParent(GOID_INVALID)
	, m_DelayedDepToBreak(GOID_INVALID)

{

	GoHandle go(goid);
	gpassert(go.IsValid());

	const SiegePos&	init_pos = go->GetPlacement()->GetPosition();

	const float DEFAULT_PATH_WIDTH = 0.5f;

	m_PathWidth = DEFAULT_PATH_WIDTH * go->GetAspect()->GetSelectionIndicatorScale();

	// This will cause a new follower to be created if one doesn't exist
	if (go->HasFollower())
	{
		SiegePos adj_pos(init_pos);
		
		if (go->HasMind() && (go->GetMind()->MayMove() || go->HasAttack()) )
		{
			eLogicalNodeFlags perms = go->GetBody()->GetTerrainMovementPermissions();

			bool pos_ok;

			if (!(perms & LF_HOVER))
			{
				pos_ok = gSiegeEngine.AdjustPointToTerrain(adj_pos,perms);

				if (pos_ok)
				{
					vector_3 dv = gSiegeEngine.GetDifferenceVector(adj_pos,init_pos);
					if (fabs(dv.y)>0.5f )
					{
						gpwarningf((
							"CONTENT WARNING: Could not initialize [%s goid:0x%08x scid:0x%08x] with position [%s].\n"
							"The requested position was more than 0.5 meter (%f) above/below the ground.\n"
							"The object has been adjusted vertically to [%s]\n"
							"It's in the [%s] region\n"
							"If it's not suppose to be on the ground, set the LF_HOVER flag",
							go ? go->GetTemplateName() : "<invalid GO>",
							go ? MakeInt(go->GetGoid()) : 0,
							go ? MakeInt(go->GetScid()) : 0,
							::ToString(init_pos).c_str(),
							dv.y,
							::ToString(adj_pos).c_str(),
							gWorldMap.GetRegionNameForNode(init_pos.node).c_str()
							));
					}
				}
				else
				{
					gpwarningf((
						"MISPLACED CONTENT: Could not initialize [%s g:0x%08x s:0x%08x] with position [%s].\n"
						"Either it cannot stand at this location on the ground or the postion is outside the world entirely.\n"
						"You can expect to see error messages regarding this object if it is attacked or moved.\n"
						"It's in the [%s] region",
						go ? go->GetTemplateName() : "<invalid GO>",
						go ? MakeInt(go->GetGoid()) : 0,
						go ? MakeInt(go->GetScid()) : 0,
						::ToString(init_pos).c_str(),
						gWorldMap.GetRegionNameForNode(init_pos.node).c_str()
						));
				}
			}
		}
		else
		{
			gpassertf(gSiegeEngine.IsNodeInAnyFrustum(init_pos.node), ("MCP_PLAN: Attempting to initialize a plan for [%s g:0x%08x s:0x%08x] with position [%s] that is outside the world\n",
				go ? go->GetTemplateName() : "<invalid GO>",
				go ? MakeInt(go->GetGoid()) : 0,
				go ? MakeInt(go->GetScid()) : 0,
				::ToString(init_pos).c_str()));
		}

		m_Follower = go->GetFollower();
		
		SiegeRot init_rot(m_Follower->GetCurrentOrientationRelativeToNode(adj_pos.node),adj_pos.node);

		//$$$$ Initialize the sequencer 
		m_Sequencer.Initialize((eRID)m_CurrentReqBlock,init_time,adj_pos,init_rot);
		m_AppendTime = init_time;
	}
	else
	{
		// This is an static object without a follower, like a barrel
		m_Follower = NULL;
	}


}

////////////////////////////////////////////////////////////////////////////////////
Plan::~Plan()
{
	BreakAllDependancyLinks(true);
	RemoveAllBlockingLinks();
}

////////////////////////////////////////////////////////////////////////////////////
FUBI_DECLARE_XFER_TRAITS( GoFollower* )
{
	static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj )
	{
		UNREFERENCED_PARAMETER(xfer);
		
		// xfer
		Goid goid = GOID_INVALID;
		if ( persist.IsSaving() && (obj != NULL) )
		{
			goid = obj->GetGoid();
		}
		bool rc = persist.Xfer( key, goid );

		// reset pointer
		if ( persist.IsRestoring() )
		{
			GoHandle go( goid );
			if ( go && go->HasFollower() )
			{
				obj = go->GetFollower();
			}
			else
			{
				obj = NULL;
			}
		}

		// done
		return ( rc );
	}
};

FUBI_DECLARE_CAST_TRAITS( eRID, DWORD );
FUBI_DECLARE_ENUM_TRAITS( eRequest, PL_NONE );
FUBI_DECLARE_SELF_TRAITS( Sequencer );
FUBI_DECLARE_SELF_TRAITS( Plan );

bool Plan::Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer     ( "m_Owner",                  m_Owner                  );
	persist.Xfer     ( "m_AppendTime",             m_AppendTime             );
	persist.Xfer     ( "m_ClientSynchTime",        m_ClientSynchTime        );
	persist.Xfer     ( "m_PathWidth",              m_PathWidth              );
	persist.Xfer     ( "m_IsLockedOn",             m_IsLockedOn             );
	persist.XferHex  ( "m_CurrentReqBlock",        m_CurrentReqBlock        );
	persist.XferHex  ( "m_LastRequestRID",	       m_LastRequestRID         );
	persist.Xfer     ( "m_LastRequestType",        m_LastRequestType        );
	persist.Xfer     ( "m_LastRequestDuration",    m_LastRequestDuration    );
	persist.Xfer     ( "m_LastRequestTarget",      m_LastRequestTarget      );
	persist.Xfer     ( "m_LastRequestSubAnim",     m_LastRequestSubAnim     );
	persist.Xfer     ( "m_LastRequestRange",       m_LastRequestRange       );
	persist.XferHex  ( "m_LastRequestFlags",       m_LastRequestFlags       );
	persist.XferHex  ( "m_PendingRequestRID",      m_PendingRequestRID      );
	persist.Xfer     ( "m_PendingRequestType",     m_PendingRequestType     );
	persist.Xfer     ( "m_PendingRequestDuration", m_PendingRequestDuration );
	persist.Xfer     ( "m_PendingRequestTarget",   m_PendingRequestTarget   );
	persist.Xfer     ( "m_PendingRequestSubAnim",  m_PendingRequestSubAnim  );
	persist.Xfer     ( "m_PendingRequestRange",    m_PendingRequestRange    );
	persist.XferHex  ( "m_PendingRequestFlags",    m_PendingRequestFlags    );
	persist.Xfer     ( "m_Follower",               m_Follower               );
	persist.Xfer     ( "m_DepParent",              m_DepParent              );
	persist.Xfer     ( "m_DelayedDepToBreak",      m_DelayedDepToBreak      );
	persist.XferList ( "m_DepChildren",            m_DepChildren            );
	persist.Xfer	 ( "m_Sequencer",              m_Sequencer              );
	return ( true );
}

////////////////////////////////////////////////////////////////////////////////////
SiegePos Plan::GetPosition(double& t, bool no_interp,bool skip_locked)
{


	SiegePos pos;

	if (!m_Sequencer.GetPosition(t,no_interp,skip_locked,pos))
	{
		if (m_Follower)
		{
			pos = m_Follower->GetCurrentPosition();
		}
		else
		{
			// This is a static object (like a barrel)
			GoHandle go(m_Owner);
			pos = go->GetPlacement()->GetPosition();
		}
	}

	return pos;
}

////////////////////////////////////////////////////////////////////////////////////
SiegePos Plan::GetFinalDestination() const
{
	if (m_Follower)
	{
		if (m_Sequencer.Empty() || (m_Sequencer.GetLockedMovementGo() != GOID_INVALID) ) 
		{
			return m_Follower->GetCurrentPosition();
		}
		else
		{
			return m_Sequencer.GetFinalPosition();
		}
	}
	else
	{
		// This is a static object (like a barrel)
		GoHandle go(m_Owner);
		return go->GetPlacement()->GetPosition();
	}
}

////////////////////////////////////////////////////////////////////////////////////
double Plan::GetFinalDestinationTime() const
{

	if (m_Follower)
	{
		if (m_Sequencer.Empty() || (m_Sequencer.GetLockedMovementGo() != GOID_INVALID))
		{
			return DBL_MAX;
		}
		else
		{
			return m_Sequencer.GetFinalTime();
		}
	}
	else
	{
		// This is a static object (like a barrel)
		return DBL_MAX;
	}

}

////////////////////////////////////////////////////////////////////////////////////
bool Plan::IsMoving() const
{
	return (!(m_Sequencer.IsPositionConstant()));
}

////////////////////////////////////////////////////////////////////////////////////
eReqRetCode Plan::EstimateInterceptPosition(
									double& contact_time,
									SiegePos& contact_pos,
									const SiegePos& src_pos,
									double initial_time,
									float velocity,
									float look_ahead,
									float range,
									float cushion
									)
{

	if (!m_Follower)
	{
		// This is a static object (like a barrel)
		GoHandle go(m_Owner);
		contact_time = DBL_MAX;
		contact_pos = go->GetPlacement()->GetPosition();
		return REQUEST_OK;
	}

	if (m_Sequencer.Empty())
	{
		contact_time = DBL_MAX;
		contact_pos = m_Follower->GetCurrentPosition();
		return REQUEST_OK;
	}
	eReqRetCode ret = m_Sequencer.EstimateInterceptPosition(
									contact_time,
									contact_pos,
									src_pos,
									initial_time,
									velocity,
									look_ahead,
									range,
									cushion
									);

#if !GP_RETAIL
	if (ret == REQUEST_FAILED_PATH_LEAVES_WORLD)
	{
		GoHandle owngo(m_Owner);
		gpwarningf(("WARNING:Attempted FindInterceptPath with [%s:0x%08x] which is on a path that will exit the world\n",
			owngo->GetTemplateName(),
			m_Owner
			));
	}
#endif
	return ret;

}

////////////////////////////////////////////////////////////////////////////////////
Plan::DependancyInfo::DependancyInfo(Goid own, double time, const SiegePos& required, const SiegePos& reserved, eDependancyType dtype)
	: m_Owner(own)
	, m_Time(time)
	, m_RequiredPosition(required)
{
	vector_3 v = gSiegeEngine.GetDifferenceVector(required,reserved);
	m_ReservedPosition = reserved;
	m_RequiredDistance = Length(v);

	m_Type = dtype;

};

#if !GP_RETAIL
////////////////////////////////////////////////////////////////////////////////////
void Plan::DependancyInfo::Dump(const char* msg,int count)
{
	gpdevreportf( &gMCPCrowdContext,("%2d: [%s] [%s] %s\n",
		count,
		::ToString(m_RequiredPosition).c_str(),
		::ToString(m_ReservedPosition).c_str(),
		msg
		));
}
////////////////////////////////////////////////////////////////////////////////////
void Plan::DependancyInfo::DebugDraw(DWORD color) const
{
	SiegePos wp = m_RequiredPosition;
	wp.pos.y +=1.0f;
	
	color = (0x00ffffff & color) | 0x60000000;
	
	gWorld.DrawDebugWedge( wp, m_RequiredDistance, m_ReservedPosition, PI/(16*m_RequiredDistance) , color, true);
}
#endif

////////////////////////////////////////////////////////////////////////////////////
void Plan::CreateDependancyLink(DependancyInfo& dl)
{

	gpassert(dl.m_Owner != m_Owner);

	Plan* kidplan = gMCPManager.FindPlan(dl.m_Owner,false);
	if (kidplan)
	{

		bool create_dep = true;

		// Make sure the kid is no longer linked to a stale parent
		if (kidplan->m_DepParent == m_Owner)
		{
			DependantIter it = m_DepChildren.begin();
			while (((*it).m_Owner != kidplan->m_Owner) && (it != m_DepChildren.end())) ++it;
			gpassert (it != m_DepChildren.end());
			if (it != m_DepChildren.end())
			{
				DependancyInfo& stale = (*it);
				if (
					// IsEqual((float)dl.m_Time,(float)stale.m_Time,0.001f) &&
					IsEqual(dl.m_ReservedPosition,stale.m_ReservedPosition,0.01f) &&
					IsEqual(dl.m_RequiredPosition,stale.m_RequiredPosition,0.01f))
				{
					stale.m_Time = dl.m_Time;
					create_dep = false;
				}
				else
				{
					kidplan->BreakDependancyLinkToParent();
				}
			}

		}
		else
		{
			kidplan->BreakDependancyLinkToParent();
		}


		if (create_dep)
		{

			// Set the kid's dependancy on this new parent

			kidplan->m_DepParent = m_Owner;
			m_DepChildren.push_back(dl);

#if !GP_RETAIL
			gpdevreportf( &gMCPMessageContext,("WE_MCP_DEPENDANCY_CREATED '%s:%s' depends on '%s:%s' to be %s @ %8.3f\n",
				(GetGo(dl.m_Owner)?GetGo(dl.m_Owner)->GetTemplateName():"<UNKNOWN>"),
				(GetGo(dl.m_Owner)?::ToString(dl.m_Owner).c_str():""),
				(GetGo(m_Owner)?GetGo(m_Owner)->GetTemplateName():"<UNKNOWN>"),
				(GetGo(m_Owner)?::ToString(m_Owner).c_str():""),
				::ToString(dl.m_RequiredPosition).c_str(),
				(dl.m_Time == DBL_MAX) ? -1 : dl.m_Time
				));

			gpdevreportf( &gMCPCrowdContext,(" Number of dependancies %d\n",m_DepChildren.size()));
			gpdevreportf( &gMCPCrowdContext,("     ANG toP  toN Msep rez -> req\n"));

			DependantIter it = m_DepChildren.begin();
			int count=0;
			while (it != m_DepChildren.end()) 
			{
				gpstring tstr;
				tstr.assignf("[%s:0x%08x]", GetGo((*it).m_Owner) ? GetGo((*it).m_Owner)->GetTemplateName() : "<UNKNOWN>",(*it).m_Owner);
				(*it).Dump(tstr.c_str(),count++);
				++it;
			}
#endif
		}

	}

}

////////////////////////////////////////////////////////////////////////////////////
void Plan::BreakDependancyLinkToParent(void)
{
	if (m_DepParent != GOID_INVALID)
	{
		Plan* parentplan = gMCPManager.FindPlan(m_DepParent,false);

		if (parentplan)
		{
			DependantIter it = parentplan->m_DepChildren.begin();
			while (((*it).m_Owner != m_Owner) && (it != parentplan->m_DepChildren.end())) ++it;
			gpassert (it != parentplan->m_DepChildren.end());
			if (it != parentplan->m_DepChildren.end())
			{

#if !GP_RETAIL
				{
					gpdevreportf( &gMCPMessageContext,("WE_MCP_DEPENDANCY_REMOVED '%s:%s' depended on '%s:%s'\n",
						(GetGo(m_Owner)?GetGo(m_Owner)->GetTemplateName():"<UNKNOWN>"),
						(GetGo(m_Owner)?::ToString(m_Owner).c_str():""),
						(GetGo(parentplan->m_Owner)?GetGo(parentplan->m_Owner)->GetTemplateName():"<UNKNOWN>"),
						(GetGo(parentplan->m_Owner)?::ToString(parentplan->m_Owner).c_str():"")));
				}
#endif
	
				parentplan->m_DepChildren.erase(it);
			}
			gpassert(m_DepParent == parentplan->m_Owner);
		}

		m_DepParent = GOID_INVALID;
	}

}

////////////////////////////////////////////////////////////////////////////////////
void Plan::BreakDependancyLinkToChild(Goid kidgoid,bool sendmsg)
{
	Plan* kidplan = gMCPManager.FindPlan(kidgoid,false);
	if (kidplan)
	{
		DependantIter it = m_DepChildren.begin();
		while (((*it).m_Owner != kidgoid) && (it != m_DepChildren.end())) ++it;
		gpassert (it !=  m_DepChildren.end());
		if (it != m_DepChildren.end() )
		{
			{
				gpdevreportf( &gMCPMessageContext,("WE_MCP_DEPENDANCY_BROKEN '%s:%s' depended on '%s:%s'\n",
					(GetGo(kidgoid)?GetGo(kidgoid)->GetTemplateName():"<UNKNOWN>"),
					(GetGo(kidgoid)?::ToString(kidgoid).c_str():""),
					(GetGo(m_Owner)?GetGo(m_Owner)->GetTemplateName():"<UNKNOWN>"),
					(GetGo(m_Owner)?::ToString(m_Owner).c_str():"")));
			}
			if (sendmsg)
			{
/********
				// We only send the first dep break we see
				if (m_DelayedDepToBreak == GOID_INVALID)
				{
					m_DelayedDepToBreak = kidgoid;
				}
********/
				WorldMessage msg( WE_MCP_DEPENDANCY_BROKEN, m_Owner, kidgoid, (DWORD)RID_OVERRIDE );
				msg.SendDelayed( 0.05f );
			}
			m_DepChildren.erase(it);
		}
		gpassert (kidplan->m_DepParent ==  m_Owner);
		kidplan->m_DepParent = GOID_INVALID;

	}

}

////////////////////////////////////////////////////////////////////////////////////
void Plan::BreakAllDependancyLinks(bool sendmsg)
{
	DependantIter it;

	BreakDependancyLinkToParent();

	for (it = m_DepChildren.begin() ; it != m_DepChildren.end() ;)
	{
		Goid deadgoid = (*it).m_Owner;
		++it;
		{
			gpdevreportf( &gMCPMessageContext,("ALL DEPENDANCIES INVALIDATED '%s:%s' can no longer depend on '%s:%s' to be %s @ %12.3f\n",
				(GetGo((*it).m_Owner)?GetGo((*it).m_Owner)->GetTemplateName():"<UNKNOWN>"),
				(GetGo((*it).m_Owner)?::ToString((*it).m_Owner).c_str():""),
				(GetGo(m_Owner)?GetGo(m_Owner)->GetTemplateName():"<UNKNOWN>"),
				(GetGo(m_Owner)?::ToString(m_Owner).c_str():""),
				::ToString((*it).m_RequiredPosition).c_str(),
				(*it).m_Time
				));
		}
		BreakDependancyLinkToChild(deadgoid, sendmsg);
	}

}

///////////////////////////////////////////////////////////////////////////////////
bool Plan::CheckAllDependancyLinks(bool sendmsg)
{

	UNREFERENCED_PARAMETER(sendmsg);

#if GP_DEBUG
	if (m_DepParent!=GOID_INVALID)
	{
		Plan* parentplan = gMCPManager.FindPlan(m_DepParent,false);
		if( parentplan )
		{
			DependantIter it = parentplan->m_DepChildren.begin();
			while (((*it).m_Owner != m_Owner) && (it != parentplan->m_DepChildren.end()) ) ++it;
			gpassert (it !=  parentplan->m_DepChildren.end());
		}
	}
	{for (DependantIter it = m_DepChildren.begin() ; it != m_DepChildren.end() ; ++it)
	{
		Plan* kidplan = gMCPManager.FindPlan((*it).m_Owner,false);
		gpassert( kidplan && (kidplan->m_DepParent ==  m_Owner) );
	}}
#endif

	const double leadtime = gMCP.GetLeadingTime();

	for (DependantIter it = m_DepChildren.begin() ; it != m_DepChildren.end() ; )
	{
		double check_time = max_t(leadtime,(*it).m_Time);
		
		if (!m_Sequencer.VerifyPositionAtTime((*it).m_RequiredPosition,check_time))
		{
			Goid deadgoid = (*it).m_Owner;
			{
				gpdevreportf( &gMCPMessageContext,("DEPENDANCY INVALIDATED '%s:%s' can no longer depend on '%s:%s' to be %s @ %12.3f\n",
					(GetGo(deadgoid)?GetGo(deadgoid)->GetTemplateName():"<UNKNOWN>"),
					(GetGo(deadgoid)?::ToString(deadgoid).c_str():""),
					(GetGo(m_Owner)?GetGo(m_Owner)->GetTemplateName():"<UNKNOWN>"),
					(GetGo(m_Owner)?::ToString(m_Owner).c_str():""),
					::ToString((*it).m_RequiredPosition).c_str(),
					((*it).m_Time == DBL_MAX) ? -1 : (*it).m_Time
					));
			}
			++it;
			BreakDependancyLinkToChild(deadgoid,sendmsg);
		}
		else
		{
			++it;
		}
	}

#if GP_DEBUG
	if (m_DepParent!=GOID_INVALID)
	{
		Plan* parentplan = gMCPManager.FindPlan(m_DepParent,false);
		if( parentplan )
		{
			DependantIter it = parentplan->m_DepChildren.begin();
			while (((*it).m_Owner != m_Owner) && (it != parentplan->m_DepChildren.end()) ) ++it;
			gpassert (it !=  parentplan->m_DepChildren.end());
		}
	}
	{for (DependantIter it = m_DepChildren.begin() ; it != m_DepChildren.end() ; ++it)
	{
		Plan* kidplan = gMCPManager.FindPlan((*it).m_Owner,false);
		gpassert ( kidplan && (kidplan->m_DepParent ==  m_Owner) );
	}}
#endif

	return true;
}


///////////////////////////////////////////////////////////////////////////////////
bool Plan::CheckDependancyPosition(Goid child,const SiegePos& p, float range) const
{
	{for (DependantConstIter it = m_DepChildren.begin() ; it != m_DepChildren.end() ; ++it)
		if ((*it).m_Owner == child) 
		{
			return !IsNegative(range-(*it).m_RequiredDistance,0.01f) &&
					IsEqual(p,(*it).m_ReservedPosition,0.01f) &&
				    IsEqual(GetFinalDestination(),(*it).m_RequiredPosition,0.01f);
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////
bool Plan::CheckDependancyNode(Goid child, database_guid n) const
{
	{for (DependantConstIter it = m_DepChildren.begin() ; it != m_DepChildren.end() ; ++it)
		if ((*it).m_Owner == child) 
		{
			return  ( n == (*it).m_RequiredPosition.node);
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////
int Plan::NumMeleeDependants(Goid excludeGoid)
{
	int count = 0;

	for (DependantConstIter it = m_DepChildren.begin() ; it != m_DepChildren.end() ; ++it)
	{
		if ((((*it).m_Type == DEPTYPE_MELEE_FRIEND) || ((*it).m_Type == DEPTYPE_MELEE_ENEMY)) 
			&& ((*it).m_Owner != excludeGoid)) 
		{
			++count;
		}
	}
	return count;
}

////////////////////////////////////////////////////////////////////////////////////
int Plan::NumBlockersInCrowd(const Goid excludeGoid) const
{
	bool incrowd = 
		m_Blockers.end() != 
		std::find_if(m_Blockers.begin(),m_Blockers.end(),BlockingInfo::PointsTo(excludeGoid));

	DWORD sz = m_Blockers.size();
	if (incrowd) 
	{
		sz--;
	}

	return sz;
}

////////////////////////////////////////////////////////////////////////////////////
// Fill in the dependancy info based on the current end-of-plan state
bool Plan::BuildDependancyInfo(DependancyInfo &di,Goid own,const SiegePos& reserve, eDependancyType dtype)
{
	di.m_Owner = own;
	di.m_RequiredPosition = GetFinalDestination();
	di.m_Time = GetFinalDestinationTime();
	di.m_ReservedPosition = reserve;

	vector_3 v = gSiegeEngine.GetDifferenceVector(di.m_RequiredPosition,reserve);

	di.m_RequiredDistance = Length(v);

	di.m_Type = dtype;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////
bool Plan::CreateBlockingLink(Plan* farplan)
{

	gpassert(this != farplan);
	
	// Do we have already have a link to the target?
	RemoveBlockingLink(farplan);

	BlockerIter testnit = std::find_if(m_Blockers.begin(),m_Blockers.end(),BlockingInfo::PointsTo(farplan));
	BlockerIter testfit = std::find_if(farplan->m_Blockers.begin(),farplan->m_Blockers.end(),BlockingInfo::PointsTo(this));

	gpassert(testnit == m_Blockers.end());
	gpassert(testfit == farplan->m_Blockers.end());

	BlockingInfo* nearbi = new BlockingInfo();
	BlockingInfo* farbi = new BlockingInfo();

	nearbi->m_FarInfo = farbi;
	farbi->m_FarInfo = nearbi;

	nearbi->m_Plan = this;
	farbi->m_Plan = farplan;

	nearbi->m_Position = GetFinalDestination();
	farbi->m_Position = farplan->GetFinalDestination();

	vector_3 nv = gSiegeEngine.GetDifferenceVector(nearbi->m_Position,farbi->m_Position);
	nearbi->m_AngleFromLocalZAxis = atan2f(nv.x,nv.z);
	KeepAngleInInterval(nearbi->m_AngleFromLocalZAxis);
	nearbi->m_BlockingAngle = atan2f(farplan->m_PathWidth,nv.Length());
	KeepAngleInInterval(nearbi->m_BlockingAngle);

	std::pair<BlockerIter,bool> insret = m_Blockers.insert(nearbi);
	gpassert(insret.second);

	vector_3 fv = gSiegeEngine.GetDifferenceVector(farbi->m_Position,nearbi->m_Position);
	farbi->m_AngleFromLocalZAxis = atan2f(fv.x,fv.z);
	KeepAngleInInterval(farbi->m_AngleFromLocalZAxis);
	farbi->m_BlockingAngle = atan2f(m_PathWidth,fv.Length());
	KeepAngleInInterval(farbi->m_BlockingAngle);

	insret = farplan->m_Blockers.insert(farbi);
	gpassert(insret.second);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////
bool Plan::RemoveBlockingLink(Plan* farplan)
{

	bool ret = false;

	BlockerIter nit = std::find_if(m_Blockers.begin(),m_Blockers.end(),BlockingInfo::PointsTo(farplan));

	if (nit != m_Blockers.end()) 
	{
		BlockerIter fit = std::find_if(farplan->m_Blockers.begin(),farplan->m_Blockers.end(),BlockingInfo::PointsTo(*nit));
		if ( fit != farplan->m_Blockers.end() )
		{

			gpassert((*nit)->m_Plan == farplan);
			gpassert((*fit)->m_Plan == this);

			delete (*nit);
			delete (*fit);

			m_Blockers.erase(nit);
			farplan->m_Blockers.erase(fit);

			ret = true;
		}
		else
		{
			if (gWorldOptions.GetDebugMCP())
			{
				gperror("MCP ERROR::Attempted to removes broken blocking link")
			}
		}
		
		nit = std::find_if(m_Blockers.begin(),m_Blockers.end(),BlockingInfo::PointsTo(farplan));
		gpassert(nit == m_Blockers.end());
	}


	return ret;
	
}

////////////////////////////////////////////////////////////////////////////////////
bool Plan::RemoveAllBlockingLinks()
{
	BlockerIter nit = m_Blockers.begin();
	BlockerIter nend = m_Blockers.end();

	while (nit != nend)
	{
		BlockingInfo* nbi = (*nit);
		BlockingInfo* fbi = nbi->m_FarInfo;

		BlockerIter fit = std::find_if(fbi->m_Plan->m_Blockers.begin(),fbi->m_Plan->m_Blockers.end(),BlockingInfo::PointsTo(nbi));
		BlockerIter fit2 = std::find_if(fbi->m_Plan->m_Blockers.begin(),fbi->m_Plan->m_Blockers.end(),BlockingInfo::PointsTo(this));
		gpassert(fit2 == fit);
		gpassert(fbi->m_Plan->m_Blockers.end() != fit);

		nit = m_Blockers.erase(nit);
		fbi->m_Plan->m_Blockers.erase(fit);

		BlockerIter stalenit = std::find_if(m_Blockers.begin(),m_Blockers.end(),BlockingInfo::PointsTo(fbi));
		gpassert(stalenit == m_Blockers.end());
		fit = std::find_if(fbi->m_Plan->m_Blockers.begin(),fbi->m_Plan->m_Blockers.end(),BlockingInfo::PointsTo(nbi));
		gpassert(fit == fbi->m_Plan->m_Blockers.end());

		nbi->m_FarInfo = NULL;
		fbi->m_FarInfo = NULL;
	
		delete nbi;
		delete fbi;
	}


	return true;
}

#if !GP_RETAIL
////////////////////////////////////////////////////////////////////////////////////
void Plan::BlockingInfo::DebugDraw(DWORD color) const
{
	SiegePos wp = m_Position;
	wp.pos.y +=1.0f;
	
	color = (0x00ffffff & color) | 0x60000000;
	
	gWorld.DrawDebugWedge( wp, 1.0f, m_FarInfo->m_Position, 2.0f*m_BlockingAngle , color, true);
}
#endif

////////////////////////////////////////////////////////////////////////////////////
bool Plan::UsesChangedNode(const database_guid query_node) const
{

	// Does the plan cross into or out of the node
	
	// Are we in the node?
	if (m_Follower)
	{
		if (m_Follower->GetCurrentPosition().node == query_node)
		{
			return true;
		}
	}
	
	//Do we depend on something to be in the node?
	if (m_DepParent)
	{
		Plan* parentplan = gMCPManager.FindPlan(m_DepParent,false);
		if ( parentplan && parentplan->CheckDependancyNode(m_Owner,query_node) )
		{
			return true;
		}
	}

	// Will we move into the node?
	return m_Sequencer.UsesNode(query_node);

}

////////////////////////////////////////////////////////////////////////////////////
void Plan::AppendTriggerSignals(const trigger::SignalSet& sigset)
{
	m_Sequencer.AppendTriggerSignals(sigset);
}

////////////////////////////////////////////////////////////////////////////////////
bool Plan::FlushAfterTime( double &request_cutoff_time )
{

	m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);

	// The actual cut off time may be later than the request, due to packets already tranmitted
	request_cutoff_time = max_t(request_cutoff_time,gMCP.GetLeadingTime());

	if (request_cutoff_time == m_AppendTime)
	{
		// We are already flushed this point in time
		// Are we duplicating the flush calls too often?
		return true;
	}

	// Always advance the request block, to invalidate any outstanding section completes
//	++m_CurrentReqBlock;
//	m_CurrentReqBlock &= 0x7fff;

	GoHandle owngo(m_Owner);
	eLogicalNodeFlags perms = owngo->GetBody()->GetTerrainMovementPermissions();

	if ( m_Sequencer.ClipToTime(request_cutoff_time,perms, owngo.Get()) )
	{
		m_Sequencer.Validate(owngo,request_cutoff_time);
		SSendPackedClipToFollowers(request_cutoff_time,m_Sequencer.GetFinalPosition());
		m_Sequencer.Validate(owngo,request_cutoff_time);
		m_AppendTime = request_cutoff_time;
	}
	else
	{
		m_AppendTime = request_cutoff_time;
		m_Sequencer.Validate(owngo,m_AppendTime);
	}
	

	return true;

}

////////////////////////////////////////////////////////////////////////////////////
void Plan::AppendChore( eAnimChore ch, eAnimStance stance, DWORD subanim, DWORD flags, float range )
{

	gpassert(m_AppendTime < FLOAT_MAX);

	m_AppendTime = max_t(m_AppendTime,gMCPManager.GetLeadingTime());
	m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);
		
	//$$$$$$$$$
	// Stance is SUPPOSED to be sent is as a parameter
	stance = AS_DEFAULT;
	if ( stance == AS_DEFAULT )
	{
		// current equipment tells us which stance to use
		GoInventory* inv = GetGo(m_Owner)->QueryInventory();
		if ( inv != NULL )
		{
			stance = inv->GetAnimStance();
		}

		// may still be default
		if ( stance == AS_DEFAULT )
		{
			stance = AS_PLAIN;
		}
	}

	gpdevreportf( &gMCPContext,("%8.3f [%8.3f:0x%08x] Appending Chore [%s:%s] for %s:%s\n",
		gMCP.GetLaggingTime(),
		m_AppendTime,
		m_CurrentReqBlock,
		::ToString(ch),
		::ToString(stance),
		GetGo(m_Owner)->GetTemplateName(),
		::ToString(m_Owner).c_str()
		));

	m_Sequencer.AppendChore((eRID)m_CurrentReqBlock,m_AppendTime,ch,stance,subanim,flags,range);
	m_AppendTime = m_Sequencer.GetFinalTime();

	m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);
		
}

//////////////////////////////////////////////////////////////////////////////
void Plan::AppendFaceTarget( Goid Targ, bool reversed, bool no_turn )
{

	m_AppendTime = max_t(m_AppendTime,gMCPManager.GetLeadingTime());
	m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);

	gpdevreportf( &gMCPContext,("%8.3f [%8.3f:0x%08x] Appending Orient to Object [%s:%s:%s] for %s:%s\n",
		gMCP.GetLaggingTime(),
		m_AppendTime,
		m_CurrentReqBlock,
		::ToString(reversed?ORIENTMODE_TRACK_OBJECT_REVERSE:ORIENTMODE_TRACK_OBJECT),
		GetGo(Targ)->GetTemplateName(),
		::ToString(Targ).c_str(),
		GetGo(m_Owner)->GetTemplateName(),
		::ToString(m_Owner).c_str()
		));

	if (no_turn)
	{
		// We know that we want to face something, we just don't know 
		// how to do it. The actual rotation will be handled on the client-side
		// in the animation skrit
		m_Sequencer.AppendOrient((eRID)m_CurrentReqBlock,
			m_AppendTime,
			ORIENTMODE_UNDEFINED,
			Targ);
	}
	else
	{
		m_Sequencer.AppendOrient((eRID)m_CurrentReqBlock,
			m_AppendTime,
			reversed ? ORIENTMODE_TRACK_OBJECT_REVERSE : ORIENTMODE_TRACK_OBJECT,
			Targ);
	}

	m_AppendTime = m_Sequencer.GetFinalTime();
}

//////////////////////////////////////////////////////////////////////////////
void Plan::AppendFacePosition( const SiegePos& tpos, bool reversed )
{

	m_AppendTime = max_t(m_AppendTime,gMCPManager.GetLeadingTime());

	gpdevreportf( &gMCPContext,("%8.3f [%8.3f:0x%08x] Appending Orient to Position [%s] for %s:%s\n",
		gMCP.GetLaggingTime(),
		m_AppendTime,
		m_CurrentReqBlock,
		::ToString(reversed?ORIENTMODE_FIXED_POSITION_REVERSE:ORIENTMODE_FIXED_POSITION),
		GetGo(m_Owner)->GetTemplateName(),
		::ToString(m_Owner).c_str()
		));

	m_Sequencer.AppendOrient((eRID)m_CurrentReqBlock,m_AppendTime,reversed?ORIENTMODE_FIXED_POSITION_REVERSE:ORIENTMODE_FIXED_POSITION, tpos);
	m_AppendTime = m_Sequencer.GetFinalTime();

	m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);
}

//////////////////////////////////////////////////////////////////////////////
void Plan::AppendFaceRotation( const SiegeRot& rot )
{
	m_AppendTime = max_t(m_AppendTime,gMCPManager.GetLeadingTime());
	m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);

	gpdevreportf( &gMCPContext,("%8.3f [%8.3f:0x%08x] Appending Orient to Rotation [%s] for %s:%s\n",
		gMCP.GetLaggingTime(),
		m_AppendTime,
		m_CurrentReqBlock,
		::ToString(ORIENTMODE_FIXED_DIRECTION),
		GetGo(m_Owner)->GetTemplateName(),
		::ToString(m_Owner).c_str()
		));

	m_Sequencer.AppendOrient((eRID)m_CurrentReqBlock,m_AppendTime,ORIENTMODE_FIXED_DIRECTION,rot);
	m_AppendTime = m_Sequencer.GetFinalTime();
}

//////////////////////////////////////////////////////////////////////////////
void Plan::AppendFaceHeading( bool reversed )
{

	m_AppendTime = max_t(m_AppendTime,gMCPManager.GetLeadingTime());

	gpdevreportf( &gMCPContext,("%8.3f [%8.3f:0x%08x] Appending Orient to Heading [%s] for %s:%s\n",
		gMCP.GetLaggingTime(),
		m_AppendTime,
		m_CurrentReqBlock,
		::ToString(reversed?ORIENTMODE_LOCK_TO_HEADING_REVERSE:ORIENTMODE_LOCK_TO_HEADING),
		GetGo(m_Owner)->GetTemplateName(),
		::ToString(m_Owner).c_str()
		));

	m_Sequencer.AppendOrient((eRID)m_CurrentReqBlock,m_AppendTime,reversed?ORIENTMODE_LOCK_TO_HEADING_REVERSE:ORIENTMODE_LOCK_TO_HEADING);
	m_AppendTime = m_Sequencer.GetFinalTime();
	m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);
}

//////////////////////////////////////////////////////////////////////////////
void Plan::AppendFaceUndefined( )
{

	m_AppendTime = max_t(m_AppendTime,gMCPManager.GetLeadingTime());

	gpdevreportf( &gMCPContext,("%8.3f [%8.3f:0x%08x] Appending Orient [ORIENTMODE_UNDEFINED] for %s:%s\n",
		gMCP.GetLaggingTime(),
		m_AppendTime,
		m_CurrentReqBlock,
		GetGo(m_Owner)->GetTemplateName(),
		::ToString(m_Owner).c_str()
		));

	m_Sequencer.AppendOrient((eRID)m_CurrentReqBlock,m_AppendTime,ORIENTMODE_UNDEFINED,GOID_INVALID);
	m_AppendTime = m_Sequencer.GetFinalTime();
}

//-------------------------------------------------------------------------
void Plan::AppendLockMovementToGo(Goid targ)
{
	m_Sequencer.AppendLockedMovementGo((eRID)m_CurrentReqBlock,m_AppendTime,targ);
}

//////////////////////////////////////////////////////////////////////////////
void Plan::GetLastChoreAndStance(eAnimChore& slc,eAnimStance& sls) const
{
	m_Sequencer.GetLastChoreAndStance(slc,sls);
}


//////////////////////////////////////////////////////////////////////////////
void Plan::SavePendingRequest(eRequest type, float duration, Goid target, DWORD subanim, float range, DWORD flags)
{

	m_PendingRequestRID      = (eRID)m_CurrentReqBlock;

	++m_CurrentReqBlock;
	m_CurrentReqBlock &= 0x7fff;

	m_PendingRequestType     = type;
	m_PendingRequestDuration = duration;
	m_PendingRequestTarget   = target;
	m_PendingRequestSubAnim  = subanim;
	m_PendingRequestRange    = range;
	m_PendingRequestFlags    = flags;

	if (gWorldOptions.GetDebugMCP() )
	{
		gpdevreportf( &gMCPMessageContext,("MCP: [%s:%s] [%s:%08x] Increment ReqBlock from [%04x] to [%04x]\n",
			GetGo(m_Owner)->GetTemplateName(),
			::ToString(m_Owner).c_str(),
			ToString(m_PendingRequestType),
			subanim,
			m_PendingRequestRID,
			m_CurrentReqBlock)
 		);
	}

}

//////////////////////////////////////////////////////////////////////////////
void Plan::CommitPendingRequest() 
{
	m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);

	m_PendingRequestRID   = (eRID)m_CurrentReqBlock;
	m_LastRequestRID      = m_PendingRequestRID;      
	m_LastRequestType     = m_PendingRequestType;     
	m_LastRequestDuration = m_PendingRequestDuration;
	m_LastRequestTarget   = m_PendingRequestTarget;   
	m_LastRequestSubAnim  = m_PendingRequestSubAnim;  
	m_LastRequestRange    = m_PendingRequestRange;   
	m_LastRequestFlags    = m_PendingRequestFlags;    
	
}

void Plan::ClearPendingRequest()
{
	m_CurrentReqBlock = (DWORD)m_PendingRequestRID;

	if (gWorldOptions.GetDebugMCP() )
	{
		gpdevreportf( &gMCPMessageContext,("MCP: [%s:%s] [%s:%08x] failed. Rollback ReqBlock to [%04x]\n",
			GetGo(m_Owner)->GetTemplateName(),
			::ToString(m_Owner).c_str(),
			ToString(m_PendingRequestType),
			m_PendingRequestSubAnim,
			m_CurrentReqBlock)
 		);
	}

}

///////////////////////////////////////////////////////////////////////
eLNodeStatus FetchLogicalNodesHelper(	SiegePos& pos,
								SiegeLogicalNode* &lno,
								siege::eLogicalNodeFlags perms)
{
	eLNodeStatus retcode = LNODE_OK;

	// Make sure the path point is on the terrain

	SiegePos orig_pos	= pos;
	if ( !gSiegeEngine.AdjustPointToTerrain(pos, perms) )
	{
		retcode = LNODE_NO_TERRAIN;
	}
	else
	{
		lno = gSiegeEngine.GetLogicalNodeAtPosition( pos, perms );

		if( FABSF( gSiegeEngine.GetDifferenceVector( orig_pos, pos ).y ) > 1.0f )
		{
			retcode = LNODE_UNWALKABLE;
		} 
		else if ( !lno )
		{
			retcode = gSiegeEngine.GetLogicalNodeAtPosition( pos, LF_ALL ) ? LNODE_UNWALKABLE : LNODE_MISSING;
		}
	}

	return retcode;
}

///////////////////////////////////////////////////////////////////////
bool Plan::CheckForBlockers(const SiegePos& destpos,
							Goid approachingGoid,
							bool skipApproaching,
							bool skipEnemies,
							bool skipDependants,
							bool skipDepsOfApproach
							)
{
	bool ret = false;

	if (!gSiegeEngine.IsNodeInAnyFrustum(destpos.node))
	{
		gpwarning("Attempted CheckForBlockers with a destpos that was not in the world");
		return false;
	}

	// Search blockers looking for anything close to the final destination point
	RemoveAllBlockingLinks();
	
	bool enemypass = false;

	float probepersonalspace = 0;
	float probeweaponrange = 0;

	Go* own = GetGo(m_Owner);
	Go* approachGo = GetGo(approachingGoid);

	GoMind* ownMind = 0;

	if (own && own->HasMind())
	{
		ownMind = own->GetMind();
	}
	else
	{
		return false;
	}

	probepersonalspace = ownMind->GetPersonalSpaceRange();
	probeweaponrange = ownMind->GetMeleeSeparationRange();

	const double cutoff_time = gMCP.GetLeadingTime();
	const double probe_time = max_t(GetFinalDestinationTime(),cutoff_time);
	// How long will it take the approaching goid to travel 2 meters?

	float avgvel;
	if (approachGo)
	{
		avgvel = approachGo->HasBody() ? approachGo->GetBody()->GetAvgMoveVelocity() : 0.0f;
	}
	else
	{
		avgvel = own->HasBody() ? own->GetBody()->GetAvgMoveVelocity() : 0.0f;
	}
	float time_tolerance = min_t(IsZero(avgvel) ? 0.5f : (2.0f / avgvel),0.75f);


	for (;;)
	{
		const GoidColl& blockers = (enemypass) ? ownMind->GetEnemiesVisible() : ownMind->GetFriendsVisible();

		GoidColl::const_iterator it,beg,end;
		beg = blockers.begin();
		end = blockers.end();

		for (it=beg;it!=end; ++it)
		{
			if (!skipApproaching || ((*it) != approachingGoid))
			{
				Plan* blockplan = gMCP.FindPlan((*it),false);
				if (blockplan)
				{
					// Are they within a half second of each other at their destinations?
					const double block_time = max_t(blockplan->GetFinalDestinationTime(),cutoff_time);
					if ( IsZero((float)(PreciseSubtract(block_time,probe_time)),time_tolerance) )
					{
						Go* bgo = GetGo(blockplan->m_Owner);
						GoAspect* basp = bgo->GetAspect();
						
						if (IsAlive((basp->GetLifeState())))
						{
							// Build a list of 'hotspots' we need to avoid
							const SiegePos& blockpos = blockplan->GetFinalDestination();

							if (gSiegeEngine.IsNodeInAnyFrustum(blockpos.node))
							{
								const vector_3& dv = gSiegeEngine.GetDifferenceVector(destpos,blockpos);
								float dist2 = dv.Length2();

								float sep_dist;		
								if (enemypass)
								{
									GoMind* bmind = bgo->GetMind();
									sep_dist = probepersonalspace + bmind->GetPersonalSpaceRange();
									sep_dist += min_t(bmind->GetMeleeSeparationRange(),probeweaponrange);
								}
								else
								{
									sep_dist = m_PathWidth + blockplan->m_PathWidth;
								}
							
								if (!IsPositive((dist2 - (sep_dist*sep_dist)),0.01f))
								{
									CreateBlockingLink(blockplan);
									// Report a conflict with everything OTHER than the approaching
									// and any thing that is ALREADY depending upon the 
									if (((*it) != approachingGoid))
									{
										if (   (!skipDependants || (blockplan->DependsUpon() != m_Owner))
											&& (!skipDepsOfApproach || (blockplan->DependsUpon() != approachingGoid))
											)
										{
											ret |= true;
										}
									}
								}
							}
						}
					}
				}
			}
		}

		if (skipEnemies || enemypass)
		{
			break;
		}
		enemypass = true;
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////
bool Plan::CheckForOccupied(Go *mover,const SiegePos& destpos) const
{

	if (!gSiegeEngine.IsNodeInAnyFrustum(destpos.node))
	{
		gpwarning("Attempted CheckForOccupied with a destpos that was not in the world");
		return false;
	}

	bool enemypass = false;

	if (mover->HasMind())
	{

		GoMind* mind = mover->GetMind();

		const double cutoff_time = gMCP.GetLeadingTime();
		const double probe_time = max_t(GetFinalDestinationTime(),cutoff_time);

		// How long will it take the mover to travel 3 meters?
		float avgvel = mover->HasBody() ? mover->GetBody()->GetAvgMoveVelocity() : 0.0f;
		float time_tolerance = min_t(IsZero(avgvel) ? 0.5f : (2.0f / avgvel),0.75f);

		for (;;)
		{
			const GoidColl& blockers = (enemypass) ? mind->GetEnemiesVisible() : mind->GetFriendsVisible();

			GoidColl::const_iterator it,beg,end;
			beg = blockers.begin();
			end = blockers.end();

			for (it=beg;it!=end; ++it)
			{
				Plan* blockplan = gMCP.FindPlan((*it),false);
				if (blockplan)
				{				
					// Are they within a half second of each other at their destinations?
					const double block_time = max_t(blockplan->GetFinalDestinationTime(),cutoff_time);
					if ( IsZero((float)(PreciseSubtract(block_time,probe_time)),time_tolerance) )
					{
						Go* bgo = GetGo(blockplan->m_Owner);
						GoAspect* basp = bgo->GetAspect();
						if (IsAlive((basp->GetLifeState())))
						{
							// What if the final destination isn't the 'target' destination?
							const SiegePos& blockpos = blockplan->GetFinalDestination();
							// Build a list of 'hotspots' we need to avoid

							if (gSiegeEngine.IsNodeInAnyFrustum(blockpos.node))
							{
								const vector_3& dv = gSiegeEngine.GetDifferenceVector(destpos,blockpos);
								float dist2 = dv.Length2();
							
								if (!IsPositive((dist2 - (m_PathWidth*m_PathWidth)),0.01f))
								{
									return true;
								}
							}
						}
					}
				}
			}

			if (enemypass)
			{
				break;
			}
			enemypass = true;
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////
eReqRetCode Plan::AppendMoveHelper( const SiegePos& dst, float range, float max_time, bool reverse, Goid targetGoid )
{
	// This helper routine takes care of popping/restoring any 'padding' fidget that may exist
	
	m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);

	TimeNode last_chore;
	bool popped;

	// $$$ Need to determine if we have to insert a CHORE_WALK
	eAnimChore lch; 
	eAnimStance lst;
	GetLastChoreAndStance(lch,lst);
	
	if (lch == CHORE_FIDGET)
	{
		popped = m_Sequencer.PopLastChoreAndOrient(last_chore);
		GetLastChoreAndStance(lch,lst);
	}
	else
	{
		popped = false;
	}
	
	bool sendchore = (lch != CHORE_WALK);

	if (!sendchore)
	{
		// Do we need to send a different walk stance?
		GoInventory* inv = GetGo(m_Owner)->QueryInventory();
		if ( inv == NULL )
		{
			sendchore = true;
		}
		else
		{
			sendchore = lst != inv->GetAnimStance();
		}
	}
	
	if (sendchore)
	{
		AppendChore( CHORE_WALK, AS_DEFAULT, 0, 0, 0 );
		AppendFaceHeading( reverse );
	}

	eReqRetCode ret = AppendMove( dst, range, max_time, reverse, targetGoid );
	
	if ( RequestFailed(ret) )
	{
		if (sendchore)
		{
			#if !GP_RETAIL
				if( GetLastChore() != CHORE_WALK )
				{
					gperrorf((" Attempting to PopLastChore from [%s goid:0x%08x scid:0x%08x] \n"
						"But the last chore is %s, not [chore_walk]",
						GetGo(m_Owner) ? GetGo(m_Owner)->GetTemplateName() : "<invalid GO>",
						GetGo(m_Owner) ? MakeInt(GetGo(m_Owner)->GetGoid()) : 0,
						GetGo(m_Owner) ? MakeInt(GetGo(m_Owner)->GetScid()) : 0,
						::ToString(lch)
					));
				}
			#endif

			// Remove the bogus walk chore that we just added
			TimeNode walk_chore;
			m_Sequencer.PopLastChoreAndOrient(walk_chore);
		}
		if (popped)
		{
			m_Sequencer.SetLastChoreAndOrient(last_chore);
		}
	}

	m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);

	return ret;
}


///////////////////////////////////////////////////////////////////////
eReqRetCode Plan::AppendMove( const SiegePos& E, float range, float max_time, bool reverse, Goid targetGoid )
{

	UNREFERENCED_PARAMETER(reverse);

	// Need to determine if a walk chore needs to be added here!
	// May also need to set the orientation (possibly reversed)

	Go* hMover = GetGo(m_Owner);

	eLogicalNodeFlags walkMaskPermissions = hMover->GetBody()->GetTerrainMovementPermissions();

	SiegeLogicalNode * pBeginLogicalNode, * pEndLogicalNode;

	SiegePos Begin = GetFinalDestination();
	SiegePos End = E;

	eLNodeStatus err = FetchLogicalNodesHelper( Begin,	pBeginLogicalNode, walkMaskPermissions );
	
	if (err == LNODE_UNWALKABLE)
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendPath using a BEGIN pos [%s] that you are not allow to walk to/from\n[%s::0x%08x] is stuck in an invalid location!",::ToString(Begin).c_str(),hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}
	else if (err == LNODE_NO_TERRAIN)
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendPath using a BEGIN pos [%s] that is not over terrain\n[%s::0x%08x] is stuck in an invalid location!",::ToString(Begin).c_str(),hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}
	else if (err == LNODE_MISSING)
	{
		gpwarning("Attempted an AppendPath, got a NULL pBeginLogicalNode.");
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}

	err = FetchLogicalNodesHelper( End, pEndLogicalNode, walkMaskPermissions );
	
	if ((err == LNODE_UNWALKABLE) && IsZero(range))
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendPath using an END pos [%s] that you are not allowed to walk to/from\n[%s::0x%08x] can't reach an invalid location!",::ToString(Begin).c_str(),hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_END;
	}
	else if (err == LNODE_NO_TERRAIN)
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendPath using an END pos [%s] that is not over terrain\n[%s::0x%08x] can't reach an invalid location!",::ToString(Begin).c_str(),hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_END;
	}
	else if (err == LNODE_MISSING)
	{
		gpwarning("Attempted an AppendPath, got a NULL pBeginLogicalNode.");
		return REQUEST_FAILED_BAD_PATH_END;
	}


	float v = hMover->GetBody()->GetAvgMoveVelocity();
	if (IsZero(v))
	{

		vector_3 delta = gSiegeEngine.GetDifferenceVector(Begin,End);

		float diffsquared = Length2(delta);
		float minrange = range*0.8f;
		float minrangesquared = minrange*minrange;
		float maxrangesquared = range*range;
		if (!IsPositive(diffsquared-maxrangesquared,MINIMUM_DISTANCE_TOLERANCE_SQUARED))
		{
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
			gpwarningf(("WARNING: Attempting to find a path for [%s:0x%08x] but the is AvgMoveVelocity is ZERO. Please check the template\n",hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
			v = hMover->GetAspect()->GetAspectHandle()->GetBlender()->GetScalarVelocity(CHORE_WALK, AS_PLAIN, 0);

			if (!IsZero(v))
			{
				gpwarningf(("         Using the velocity %f from [CHORE_WALK, AS_PLAIN, 0] as as substitute\n",v));	
			} 
			else
			{
				return( REQUEST_FAILED_ZERO_VELOCITY );
			}
		}
	}

	eReqRetCode ret = REQUEST_FAILED_NO_PATH;

	WayPointList wpl;
	bool bSuccess = gWorldPathFinder.FindPath( pBeginLogicalNode, Begin, pEndLogicalNode, End, walkMaskPermissions, range, wpl );

	if ( bSuccess && (wpl.size() > 0) )
	{

		m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);

		bool within_range = m_Sequencer.AppendWayPoints(
			(eRID)m_CurrentReqBlock,
			m_AppendTime,
			wpl,
			End,
			v,
			range,
			max_time,
			walkMaskPermissions);

		bool conflict_detected = CheckForBlockers(Begin,targetGoid,false,false,false,false);
		if (conflict_detected)
		{

			if ( gWorldOptions.GetCrowdingMCP() )
			{
				gWorld.DrawDebugBoxStack(GetFinalDestination(), 0.1f , 0xff8080ff, 3.0f);	
			}
			ret = REQUEST_OK_CROWDED;

		}
		else
		{
			if (within_range)
			{
				ret = REQUEST_OK;
			}
			else
			{
				ret = REQUEST_OK_BEYOND_RANGE;
			}
		}
		
		m_AppendTime = m_Sequencer.GetFinalTime();

		m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);

	}
	else
	{
	#if GP_DEBUG
		SiegePos label_pos( Begin );
		label_pos.pos.y	+= 2.0f;
		gpstring label( "FindPath() failed!" );
	#endif
	}

	return ret;

}

///////////////////////////////////////////////////////////////////////
eReqRetCode Plan::AppendMoveWithDestinationTime( const SiegePos& E, float range, double dest_time, bool reverse, Goid targetGoid )
{
	UNREFERENCED_PARAMETER(reverse);
	UNREFERENCED_PARAMETER(targetGoid);

	// Need to determine if a walk chore needs to be added here!
	// May also need to set the orientation (possibly reversed)

	Go* hMover = GetGo(m_Owner);

	m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);

	eLogicalNodeFlags walkMaskPermissions = hMover->GetBody()->GetTerrainMovementPermissions();

	SiegeLogicalNode * pBeginLogicalNode, * pEndLogicalNode;

	SiegePos Begin = GetFinalDestination();
	SiegePos End = E;

	eLNodeStatus err = FetchLogicalNodesHelper( Begin,	pBeginLogicalNode, walkMaskPermissions );
	
	if (err == LNODE_UNWALKABLE)
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendPath using a BEGIN pos [%s] that you are not allow to walk to/from\n[%s::0x%08x] is stuck in an invalid location!",::ToString(Begin).c_str(),hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}
	else if (err == LNODE_NO_TERRAIN)
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendPath using a BEGIN pos [%s] that is not over terrain\n[%s::0x%08x] is stuck in an invalid location!",::ToString(Begin).c_str(),hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}
	else if (err == LNODE_MISSING)
	{
		gpwarning("Attempted an AppendPath, got a NULL pBeginLogicalNode.");
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}

	err = FetchLogicalNodesHelper( End, pEndLogicalNode, walkMaskPermissions );
	
	if ((err == LNODE_UNWALKABLE) && IsZero(range))
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendPath using a END pos [%s] that you are not allow to walk to/from\n[%s::0x%08x] can't reach an invalid location!",::ToString(Begin).c_str(),hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_END;
	}
	else if (err == LNODE_NO_TERRAIN)
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendPath using a END pos [%s] that is not over terrain\n[%s::0x%08x] can't reach an invalid location!",::ToString(Begin).c_str(),hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_END;
	}
	else if (err == LNODE_MISSING)
	{
		gpwarning("Attempted an AppendPath, got a NULL pBeginLogicalNode.");
		return REQUEST_FAILED_BAD_PATH_END;
	}


	float v = hMover->GetBody()->GetAvgMoveVelocity();
	if (IsZero(v))
	{

		vector_3 delta = gSiegeEngine.GetDifferenceVector(Begin,End);

		float diffsquared = Length2(delta);
		float minrange = range*0.8f;
		float minrangesquared = minrange*minrange;
		float maxrangesquared = range*range;
		if (!IsPositive(diffsquared-maxrangesquared,MINIMUM_DISTANCE_TOLERANCE_SQUARED))
		{
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
			gpwarningf(("WARNING: Attempting to find a path for [%s:0x%08x] but the is AvgMoveVelocity is ZERO. Please check the template\n",hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
			v = hMover->GetAspect()->GetAspectHandle()->GetBlender()->GetScalarVelocity(CHORE_WALK, AS_PLAIN, 0);

			if (!IsZero(v))
			{
				gpwarningf(("         Using the velocity %f from [CHORE_WALK, AS_PLAIN, 0] as as substitute\n",v));	
			} 
			else
			{
				return( REQUEST_FAILED_ZERO_VELOCITY );
			}
		}
	}

	eReqRetCode ret = REQUEST_FAILED_NO_PATH;

	WayPointList wpl;
	bool bSuccess = gWorldPathFinder.FindPath( pBeginLogicalNode, Begin, pEndLogicalNode, End, walkMaskPermissions, range, wpl );

	if ( bSuccess && (wpl.size() > 0) )
	{

		m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);

		bool within_range = m_Sequencer.AppendWayPointsWithArrivalTime(
			(eRID)m_CurrentReqBlock,
			m_AppendTime,
			wpl,
			End,
			v,
			range,
			dest_time,
			walkMaskPermissions);

		m_AppendTime = m_Sequencer.GetFinalTime();

		m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);

		if (within_range)
		{
			ret = REQUEST_OK;
		}
		else
		{
			ret = REQUEST_OK_BEYOND_RANGE;
		}

	}
	else
	{
	#if GP_DEBUG
		SiegePos label_pos( Begin );
		label_pos.pos.y	+= 2.0f;
		gpstring label( "FindPath() failed!" );
	#endif
	}

	return ret;

}

///////////////////////////////////////////////////////////////////////
bool Plan::ProbeForValidPositionInInterval(Plan* prober,
										   float probeinitangle,
										   float range,
										   float minsep,
										   BlockingInfo* lhs,
										   BlockingInfo* rhs,
										   eLogicalNodeFlags walkMask,
										   bool left_to_right,
										   SiegePos &probe
										   ) const
{

	gpassert( range < 20.0f );

	GoMind* probemind = GetGo(prober->m_Owner)->GetMind();

	gpassert(probemind);
	if (!probemind)
	{
		return false;
	}

	const double cutoff_time = gMCP.GetLeadingTime();

	// I am calling it the anchor because the probe takes place around it
	// We don't know when prober will arrive at the probe destination, but
	// we will examine the crowding that will exists when the anchor it in position
	const SiegePos& anchor = GetFinalDestination();
	const double anchor_time = max_t(GetFinalDestinationTime(),cutoff_time);

	// How long will it take the prober to travel 3 meters?
	float avgvel = GetGo(prober->m_Owner)->HasBody() ? GetGo(prober->m_Owner)->GetBody()->GetAvgMoveVelocity() : 0.0f;
	float time_tolerance = min_t(IsZero(avgvel) ? 0.5f : (2.0f / avgvel),0.75f);

	float lhs_ang = (*lhs).m_AngleFromLocalZAxis + ((*lhs).m_BlockingAngle + minsep);
	KeepAngleInInterval(lhs_ang);

	float rhs_ang = (*rhs).m_AngleFromLocalZAxis - ((*rhs).m_BlockingAngle + minsep);
	KeepAngleInInterval(rhs_ang);

	float gap = rhs_ang-lhs_ang;
	KeepAngleInInterval(gap);

	minsep *= 0.5f;
	
	float probe_ang = left_to_right ? lhs_ang : rhs_ang;
	float delta_ang = ((left_to_right) ? minsep : -minsep);

	bool interval_open = false;
	bool search_completed = false;

	float probepersonalspace = probemind->GetPersonalSpaceRange();
	float probeweaponrange = 0;
	probeweaponrange = probemind->GetMeleeSeparationRange();

	float timeout = 1;


	if (probeinitangle != -FLOAT_INFINITE)
	{
		float s,c;

		SINCOSF( probeinitangle, s, c);

		probe.node = anchor.node;
		probe.pos.x = anchor.pos.x + s*range;
		probe.pos.y = anchor.pos.y;
		probe.pos.z = anchor.pos.z + c*range;
	}
	else
	{
		// Set up the initial probe position if we didn't get 
		// one from the caller
		float s,c;

		SINCOSF( probe_ang, s, c);

		probe.node = anchor.node;
		probe.pos.x = anchor.pos.x + s*range;
		probe.pos.y = anchor.pos.y;
		probe.pos.z = anchor.pos.z + c*range;

		probe_ang += delta_ang;
		KeepAngleInInterval(probe_ang);
		gap -= minsep;
	}

	// Verify that the probe position is indeed a valid place to stand
	while (!interval_open && !search_completed)
	{

		// How far can the floor deviate by? Probably want a slope value based
		// on the required distance...
		const float MINIMUM_FLOOR_DIFFERENCE = 0.5;

		if (gSiegeEngine.IsPositionAllowed(probe,walkMask,MINIMUM_FLOOR_DIFFERENCE))
		{
			
			interval_open = true;

			bool enemypass = false;

			while (interval_open)
			{
				const GoidColl& blockers = (enemypass) ? probemind->GetEnemiesVisible() : probemind->GetFriendsVisible();

				GoidColl::const_iterator it,beg,end;
				beg = blockers.begin();
				end = blockers.end();

				for (it=beg; (it!=end) && interval_open; ++it)
				{
					if (m_Owner != (*it))
					{
						Plan* blockplan = gMCP.FindPlan((*it),false);
						if (blockplan && (blockplan != prober))
						{
							// Are they within time_tolerance of each other at their destinations?
							const double block_time = max_t(blockplan->GetFinalDestinationTime(),cutoff_time);
							if ( IsZero((float)(PreciseSubtract(block_time,anchor_time)),time_tolerance) )
							{
								Go* bgo = GetGo(blockplan->m_Owner);
								GoAspect* basp = bgo->GetAspect();
								
								if (IsAlive((basp->GetLifeState())))
								{

									// What if the final destination isn't the 'target' destination?
									const SiegePos& blockpos = blockplan->GetFinalDestination();
									// Build a list of 'hotspots' we need to avoid
									if (gSiegeEngine.IsNodeInAnyFrustum(probe.node))
									{
										const vector_3& dv = gSiegeEngine.GetDifferenceVector(probe,blockpos);
										float dist2 = dv.Length2();

										float sep_dist;		
										if (enemypass)
										{
											GoMind* bmind = bgo->GetMind();
											sep_dist = probepersonalspace + bmind->GetPersonalSpaceRange();
											sep_dist += min_t(bmind->GetMeleeSeparationRange(),probeweaponrange);
										}
										else
										{
											sep_dist = m_PathWidth + blockplan->m_PathWidth;
										}

										sep_dist *= 0.85f;
										
										if (!IsPositive((dist2 - (sep_dist*sep_dist)),0.01f))
										{

											if ( gWorldOptions.GetCrowdingMCP() )
											{
												double probetime = gWorldTime.GetTime();
												SiegePos porig = prober->GetPosition(probetime,true,false);
												gWorld.DrawDebugArc( porig, probe, enemypass ? 0xffff8080 : 0xffffff80, "" );
												gWorld.DrawDebugBoxStack(probe, 0.025f , enemypass ? 0xffff8080 : 0xffffff80, timeout);	
												timeout += 0.1f;
											}
											interval_open = false;
										}
									}
								}
							}
						}	
					}
				}

				if (interval_open)
				{
					if ( gWorldOptions.GetCrowdingMCP() )
					{
						double probetime = gWorldTime.GetTime();
						SiegePos porig = prober->GetPosition(probetime,true,false);
						gWorld.DrawDebugArc( porig, probe, 0xff80ff80, "" );
						gWorld.DrawDebugBoxStack(probe, 0.025f , 0xff80ff80, timeout);	
						timeout += 0.1f;
					}
				}

				if (enemypass)
				{
					break;
				}
				enemypass = true;
			}

		}
		else
		{
			// Position is blocked by some object
			if (gWorldOptions.GetCrowdingMCP())
			{
				double probetime = gWorldTime.GetTime();
				SiegePos porig = prober->GetPosition(probetime,true,false);
				gWorld.DrawDebugArc( porig, probe, 0xffff8080, "" );
				gWorld.DrawDebugBoxStack(probe, 0.025f , 0xffff8080, timeout);	
				timeout += 0.1f;
			}
		}


		if (!interval_open)
		{
			if (gap < 0)
			{
				// We didn't find a spot in this interval...
				search_completed = true;
			}
			else
			{
				// Calculate a new probe position

				// Convert the probe angle back into a siege position and then
				float s,c;

				SINCOSF( probe_ang, s, c);

				probe.node = anchor.node;
				probe.pos.x = anchor.pos.x + s*range;
				probe.pos.y = anchor.pos.y;
				probe.pos.z = anchor.pos.z + c*range;

				probe_ang += delta_ang;
				KeepAngleInInterval(probe_ang);
				gap -= minsep;

			}
		}


	}

	return interval_open;
}

///////////////////////////////////////////////////////////////////////
bool Plan::FindPositionInCrowdAroundTarget(float range, Plan* tgtPlan, SiegePos& probe)
{

	bool valid_probe = false;

	Go* hMover = GetGo(m_Owner);
	eLogicalNodeFlags walkMaskPermissions = hMover->GetBody()->GetTerrainMovementPermissions();

	double probetime = m_AppendTime;
	probe = GetPosition(probetime,true,false);

	vector_3 v = gSiegeEngine.GetDifferenceVector(tgtPlan->GetFinalDestination(),probe);

	if (IsZero(v,0.1f))
	{
		probe = m_Follower->GetCurrentPosition();
		v = gSiegeEngine.GetDifferenceVector(tgtPlan->GetFinalDestination(),probe);
	}
	
	if (IsZero(v,0.1f))
	{
		v.x = Random(-1.0f,1.0f);
		v.y = 0;
		v.z = Random(-1.0f,1.0f);
	}


	float AngleFromLocalZAxis = atan2f(v.x,v.z);
	KeepAngleInInterval(AngleFromLocalZAxis);
	float BlockingAngle = atan2f(m_PathWidth,range);
	KeepAngleInInterval(BlockingAngle);

	if (gWorldOptions.GetDebugMCP() && IsZero(range))
	{
		gperrorf(("The incoming crowd member [%s:0x%08x] is right at the center of the crowd! Please log this occurrence in raid ONLY if you can easily reproduce this situation. It is safe to continue",hMover->GetTemplateName(),m_Owner));
	}

	BlockerIter beg = tgtPlan->m_Blockers.begin();
	BlockerIter end = tgtPlan->m_Blockers.end();
	BlockerIter it;

	if (beg == end)
	{
		//gpwarning(("No crowd to avoid!!!"));

		// How far can the floor deviate by? Probably want a slope value based
		// on the required distance...
		const float MINIMUM_FLOOR_DIFFERENCE = 0.5;

		const SiegePos& dest = tgtPlan->GetFinalDestination();
		float s,c;
		
		probe = dest;
		probe.pos += Normalize(v) * range;
		
		for (int loop = 1; loop < 16; ++loop)
		{

			if (gSiegeEngine.IsPositionAllowed(probe,walkMaskPermissions,MINIMUM_FLOOR_DIFFERENCE))
			{
				return true;
			}

			// alternate angle 1, -1, 2, -2.....
			AngleFromLocalZAxis += ((loop&1)?loop:-loop) * PI2/16.0f;
			SINCOSF( AngleFromLocalZAxis, s, c);
			probe.node = dest.node;
			probe.pos.x = dest.pos.x + s*range;
			probe.pos.y = dest.pos.y;
			probe.pos.z = dest.pos.z + c*range;

		}
		return false;
	}

	// Interval markers
	BlockerIter lhs, rhs;

	// Placeholders
	BlockerIter lhs_runner, rhs_runner;

	lhs = rhs = beg;

	if (AngleFromLocalZAxis < (*lhs)->m_AngleFromLocalZAxis)
	{
		lhs = end;
		--lhs;
	}
	else
	{
		rhs = lhs;
		++rhs;
		if ( rhs == end ) 
		{
			rhs = beg;
		}
		while (AngleFromLocalZAxis > (*rhs)->m_AngleFromLocalZAxis)
		{
			lhs = rhs;
			++rhs;
			if ( rhs == end ) 
			{
				rhs = beg;
				break;
			}
		}
	}

	// Have we found ourself in the dependancy list already?
	// If so then use the next/prev entry
	if ((*lhs)->m_FarInfo->m_Plan == this)
	{
		
		if (lhs == beg)
		{
			lhs = end;
		}
		--lhs;

	}
	else if ((*rhs)->m_FarInfo->m_Plan == this)
	{
		++rhs;
		if ( rhs == end ) 
		{
			rhs = beg;
		}
	}

	lhs_runner = lhs;
	rhs_runner = rhs;

	// Need to decide which direction probe in

	float lhs_diff = AngleFromLocalZAxis - (*lhs)->m_AngleFromLocalZAxis;
	KeepAngleInInterval(lhs_diff);

	float rhs_diff = (*rhs)->m_AngleFromLocalZAxis - AngleFromLocalZAxis;
	KeepAngleInInterval(rhs_diff);

	bool left_to_right = rhs_diff>=lhs_diff;

	float probeinitangle = AngleFromLocalZAxis;

	do
	{
		float sep_ang = (*rhs)->m_AngleFromLocalZAxis - (*lhs)->m_AngleFromLocalZAxis;
		KeepAngleInInterval(sep_ang);
		if (IsZero(sep_ang)) 
		{
			if (tgtPlan->m_Blockers.size() <= 2)
			{
				sep_ang = PI2;
			}
		}
		float gap = sep_ang - 
						((*lhs)->m_BlockingAngle +
						 (*rhs)->m_BlockingAngle +
						 (2.0f*BlockingAngle));

		if ( IsPositive(gap) )
		{
			valid_probe = tgtPlan->ProbeForValidPositionInInterval(
				this,
				probeinitangle,
				range,
				BlockingAngle,
				(*lhs),
				(*rhs),
				walkMaskPermissions,
				left_to_right,
				probe);
		}

		if (!valid_probe)
		{
			probeinitangle = -FLOAT_INFINITE;
			probe = SiegePos::INVALID;

			lhs_diff =  AngleFromLocalZAxis - (*lhs_runner)->m_AngleFromLocalZAxis;
			KeepAngleInInterval(lhs_diff);

			rhs_diff = (*rhs_runner)->m_AngleFromLocalZAxis - AngleFromLocalZAxis;
			KeepAngleInInterval(rhs_diff);

			bool search_left = rhs_diff>=lhs_diff;

			if (search_left)
			{
				rhs = lhs_runner;
				if (lhs_runner == beg)
				{
					lhs_runner = end;
				}
				--lhs_runner;
				lhs = lhs_runner;
				left_to_right = false;
			}
			else
			{
				lhs = rhs_runner;
				++rhs_runner;
				if (rhs_runner == end)
				{
					rhs_runner = beg;
				}
				rhs = rhs_runner;
				left_to_right = true;
			}
		}

	} while (!valid_probe && ((lhs_runner != lhs) || (rhs_runner != rhs)));

	return valid_probe;
}


///////////////////////////////////////////////////////////////////////
eReqRetCode Plan::AppendMoveToCrowd( Plan* tgtPlan, float range, bool reverse )
{
	SiegePos Begin = GetFinalDestination();
/*
	// Is character already in valid crowd position?
	if ( m_DepParent == tgtPlan->m_Owner )
	{
		// Am I at the position I'm supposed to be at, according to the dependancy?
		if (tgtPlan->CheckDependancyPosition(m_Owner,Begin,range))
		{
			return REQUEST_OK_IN_RANGE;
		}
	}
*/
	Go* hMover = GetGo(m_Owner);
	SiegePos End = tgtPlan->GetFinalDestination();

	eLogicalNodeFlags walkMaskPermissions = hMover->GetBody()->GetTerrainMovementPermissions();

	SiegeLogicalNode * pBeginLogicalNode, * pEndLogicalNode;

	eLNodeStatus err = FetchLogicalNodesHelper( Begin,	pBeginLogicalNode, walkMaskPermissions );
	
	if (err == LNODE_UNWALKABLE)
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendPathToCrowd using a BEGIN pos [%s] that you are not allow to walk to/from\n[%s::0x%08x] is stuck in an invalid location!",::ToString(Begin).c_str(),hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}
	else if (err == LNODE_NO_TERRAIN)
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendPathToCrowd using a BEGIN pos [%s] that is not over terrain\n[%s::0x%08x] is stuck in an invalid location!",::ToString(Begin).c_str(),hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}
	else if (err == LNODE_MISSING)
	{
		gpwarning("Attempted an AppendPathToCrowd, got a NULL pBeginLogicalNode.");
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}

	if (tgtPlan->IsMoving() /*|| (tgtPlan->NumBlockersInCrowd(m_Owner) == 0)*/ )
	{

		// The target is still moving and hasn't reach the crowd location --yet!

		double initial_time = GetFinalDestinationTime();
		double leading_time = max_t(m_Sequencer.GetLastTimeSent(),gMCP.GetLeadingTime());

		if (initial_time > leading_time)
		{
			
			FlushAfterTime(leading_time);
			Begin = GetFinalDestination();

			eLNodeStatus err = FetchLogicalNodesHelper( Begin,	pBeginLogicalNode, walkMaskPermissions );
			if (err == LNODE_UNWALKABLE)
			{
				gpwarningf(("POSITION CORRUPTION: Attempted an AppendPathToCrowd to a moving target using a BEGIN pos [%s] that you are not allow to walk to/from\n[%s::0x%08x] is stuck in an invalid location!",::ToString(Begin).c_str(),hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
				return REQUEST_FAILED_BAD_PATH_BEGIN;
			}
			else if (err == LNODE_NO_TERRAIN)
			{
				gpwarningf(("POSITION CORRUPTION: Attempted an AppendPathToCrowd to a moving target using a BEGIN pos [%s] that is not over terrain\n[%s::0x%08x] is stuck in an invalid location!",::ToString(Begin).c_str(),hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
				return REQUEST_FAILED_BAD_PATH_BEGIN;
			}
			else if (err == LNODE_MISSING)
			{
				gpwarning("Attempted an AppendPathToCrowd to a moving target, got a NULL pBeginLogicalNode.");
				return REQUEST_FAILED_BAD_PATH_BEGIN;
			}
			initial_time = GetFinalDestinationTime();
		}

		if (gWorldOptions.GetHudMCP())
		{
			gWorld.DrawDebugBoxStack(Begin, 0.1f , 0xff80ff80, 2.0f);	
		}

		float avg_velocity = hMover->GetBody()->GetAvgMoveVelocity();

		SiegePos intercept_pos;
		double intercept_time;

		eReqRetCode ret = tgtPlan->EstimateInterceptPosition(
										intercept_time,
										intercept_pos,
										Begin,
										initial_time,
										avg_velocity,
										FLOAT_MAX,
										range,
										0
										);

		if (gWorldOptions.GetHudMCP())
		{
			gWorld.DrawDebugBoxStack(intercept_pos, 0.1f , 0xffff8080, 2.0f);	
		}

		if (ret == REQUEST_OK)
		{
//			if (!IsEqual(intercept_pos,End,0.1f))
			{
				if ((tgtPlan->NumBlockersInCrowd(m_Owner) != 0))
				{
					End = intercept_pos;
				}
				else
				{
					ret = AppendMoveHelper(intercept_pos, range, FLOAT_MAX, reverse, tgtPlan->m_Owner);

					if (RequestOk(ret))
					{
						// Create a dependancy link to the target's intercept position at the intercept time
						Plan::DependancyInfo diInt(m_Owner,intercept_time,intercept_pos,GetFinalDestination(),DEPTYPE_MELEE_ENEMY);
						tgtPlan->CreateDependancyLink(diInt);
					}
					return ret;
				}
			}
		}
	}
	
	err = FetchLogicalNodesHelper( End, pEndLogicalNode, walkMaskPermissions );
	
	if ((err == LNODE_UNWALKABLE) && IsZero(range))
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendPathToCrowd using a END pos [%s] that you are not allow to walk to/from\n[%s::0x%08x] is stuck in an invalid location!",::ToString(Begin).c_str(),hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_END;
	}
	else if (err == LNODE_NO_TERRAIN)
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendPathToCrowd using a END pos [%s] that is not over terrain\n[%s::0x%08x] is stuck in an invalid location!",::ToString(Begin).c_str(),hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_END;
	}
	else if (err == LNODE_MISSING)
	{
		gpwarning("Attempted an AppendPathToCrowd, got a NULL pBeginLogicalNode.");
		return REQUEST_FAILED_BAD_PATH_END;
	}

	float v = hMover->GetBody()->GetAvgMoveVelocity();

	if (IsZero(v))
	{
		gpwarningf(("WARNING: Attempting to find a path for [%s:0x%08x] but the is AvgMoveVelocity is ZERO. Please check the template\n",hMover->GetTemplateName(),MakeInt(hMover->GetGoid())));
		v = hMover->GetAspect()->GetAspectHandle()->GetBlender()->GetScalarVelocity(CHORE_WALK, AS_PLAIN, 0);

		if (!IsZero(v))
		{
			gpwarningf(("         Using the velocity %f from [CHORE_WALK, AS_PLAIN, 0] as as substitute\n",v));	
		} 
		else
		{
			return( REQUEST_FAILED_ZERO_VELOCITY );
		}
	}

	eReqRetCode ret = REQUEST_FAILED_NO_PATH;

	WayPointList wpl;

	bool bSuccess = gWorldPathFinder.FindPath( pBeginLogicalNode, Begin, pEndLogicalNode, End, walkMaskPermissions, 0, wpl );

	if ( bSuccess )
	{
		// We know we can get to the target, can we get to an open 
		// slot in the crowd around the target?


		SiegePos crowdpos;

		if (FindPositionInCrowdAroundTarget(range,tgtPlan,crowdpos))
		{
			gpdevreportf( &gMCPInterceptContext,("AP2C CROWD POSITION IS %s\n",::ToString(crowdpos).c_str()));

			ret = AppendMoveHelper(crowdpos, 0, FLOAT_MAX, reverse, tgtPlan->m_Owner);

			if (RequestOk(ret))
			{
				Plan::DependancyInfo diInt(m_Owner,tgtPlan->GetFinalDestinationTime(),End,crowdpos,DEPTYPE_MELEE_ENEMY);
				tgtPlan->CreateDependancyLink(diInt);
				tgtPlan->CheckForBlockers(crowdpos,m_Owner,false,false,false,true);
			}
			else
			{
				gpdevreportf( &gMCPCrowdContext,("MCP FPICAT failed (AppendMoveHelper returned [%s]) for [%s:0x%08x]\n",
					hMover->GetTemplateName(),
					MakeInt(hMover->GetGoid()),
					::ToString(ret)
					));
				ret = REQUEST_FAILED_OVERCROWDED;
			}
		}
		else
		{
			gpdevreportf( &gMCPCrowdContext,("MCP FPICAT failed (no valid position to move to!) for [%s:0x%08x]\n",
				hMover->GetTemplateName(),
				MakeInt(hMover->GetGoid())
				));
			ret = REQUEST_FAILED_OVERCROWDED;
		}

	}


	return ret;

}


////////////////////////////////////////////////////////////////////////////////////
void CalculateCollisionHelper(
	float total_dist,
	double early_start_time, float early_velocity, float early_range, float& early_total_time, 
	double later_start_time, float later_velocity, float later_range, float& later_total_time
	)
{

	if (total_dist < early_range) 
	{
		early_total_time = 0.0f;
		early_velocity = 0.0f;
	}
	else
	{
		float time_diff = (float)(PreciseSubtract(later_start_time, early_start_time));
		float time_xtra = (total_dist-early_range)/early_velocity;
		if (time_xtra < time_diff)
		{
			early_total_time = time_xtra;
			total_dist = early_range;
			early_velocity = 0.0f;
		}
		else
		{
			early_total_time = time_diff;
			total_dist -= time_diff*early_velocity;
		}
	}

	later_total_time = 0.0f;
	if (total_dist < later_range) 
	{
		later_velocity = 0.0f;
	}

	float combined_velocity = early_velocity+later_velocity;

	if (!IsZero(combined_velocity))
	{

		float max_range,min_range,min_velocity;
		if (early_range < later_range)
		{
			max_range = later_range;
			min_range = early_range;
			min_velocity = early_velocity;
		}
		else
		{
			max_range = early_range;
			min_range = later_range;
			min_velocity = later_velocity;
		}

		float dist_to_max = total_dist-max_range;
		float combined_time = !IsPositive(dist_to_max) ? 0 : dist_to_max/combined_velocity;

		float dist_to_min = !IsPositive(dist_to_max) ? total_dist-min_range : max_range-min_range;
		float extra_time = !IsPositive(dist_to_min) ? 0 : dist_to_min/min_velocity;

		if (!IsZero(early_velocity))
		{
			early_total_time += combined_time;
			if (early_range < later_range)
			{
				early_total_time += extra_time;
			}
		}
		if (!IsZero(later_velocity))
		{
			later_total_time += combined_time;
			if (early_range > later_range)
			{
				later_total_time += extra_time;
			}
		}

	}
}

////////////////////////////////////////////////////////////////////////////////////
eReqRetCode Plan::AppendCollisionMove( Plan* lhs_plan, float lhs_vel, float lhs_range, float rhs_vel, bool reverse )
{

	UNREFERENCED_PARAMETER(reverse);

	eReqRetCode ret = REQUEST_FAILED;
	
	Plan* rhs_plan = this;

	Go* lhsGO = GetGo(lhs_plan->m_Owner);
	Go* rhsGO = GetGo(m_Owner);

	eLogicalNodeFlags lhs_walkmask = lhsGO->GetBody()->GetTerrainMovementPermissions();
	eLogicalNodeFlags rhs_walkmask = rhsGO->GetBody()->GetTerrainMovementPermissions();

	double start_time = lhs_plan->m_Sequencer.GetEarliestClipTime(gMCP.GetLeadingTime());
	start_time = rhs_plan->m_Sequencer.GetEarliestClipTime(start_time);

	lhs_plan->m_Sequencer.Validate(GetGo(lhs_plan->GetOwner()),lhs_plan->GetAppendTime());
	rhs_plan->m_Sequencer.Validate(GetGo(rhs_plan->GetOwner()),rhs_plan->GetAppendTime());

	SiegePos lhs_pos = lhs_plan->GetPosition(start_time,true,false);
	SiegePos rhs_pos = rhs_plan->GetPosition(start_time,true,false);

	if (gWorldOptions.GetHudMCP())
	{
		gWorld.DrawDebugBoxStack(rhs_pos, 0.1f , 0xffff0000, 2.0f);	
	}

	SiegeLogicalNode * lhs_lnode, * rhs_lnode;

	eLNodeStatus err = FetchLogicalNodesHelper( lhs_pos, lhs_lnode, lhs_walkmask );
	
	if (err == LNODE_UNWALKABLE)
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendCollisionPath using a LHS pos [%s] that you are not allow to walk to/from\n[%s::0x%08x] is stuck in an invalid location!",::ToString(lhs_pos).c_str(),lhsGO->GetTemplateName(),MakeInt(lhsGO->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}
	else if (err == LNODE_NO_TERRAIN)
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendCollisionPath using a LHS pos [%s] that is not over terrain\n[%s::0x%08x] is stuck in an invalid location!",::ToString(lhs_pos).c_str(),lhsGO->GetTemplateName(),MakeInt(lhsGO->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}
	else if (err == LNODE_MISSING)
	{
		gpwarning("Attempted an AppendCollisionPath, got a NULL lhs_lnode.");
		return REQUEST_FAILED_BAD_PATH_BEGIN;
	}

	err = FetchLogicalNodesHelper( rhs_pos, rhs_lnode, rhs_walkmask );
	
	if (err == LNODE_UNWALKABLE)
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendCollisionPath using a RHS pos [%s] that you are not allow to walk to/from\n[%s::0x%08x] is stuck in an invalid location!",::ToString(rhs_pos).c_str(),lhsGO->GetTemplateName(),MakeInt(lhsGO->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_END;
	}
	else if (err == LNODE_NO_TERRAIN)
	{
		gpwarningf(("POSITION CORRUPTION: Attempted an AppendCollisionPath using a RHS pos [%s] that is not over terrain\n[%s::0x%08x] is stuck in an invalid location!",::ToString(rhs_pos).c_str(),lhsGO->GetTemplateName(),MakeInt(lhsGO->GetGoid())));
		return REQUEST_FAILED_BAD_PATH_END;
	}
	else if (err == LNODE_MISSING)
	{
		gpwarning("Attempted an AppendCollisionPath, got a NULL rhs_lnode.");
		return REQUEST_FAILED_BAD_PATH_END;
	}
	
	vector_3 delta = gSiegeEngine.GetDifferenceVector(lhs_pos,rhs_pos);

	float ini_dist = Length(delta);

	float rhs_range = rhs_plan->m_LastRequestRange;
	float min_range = min_t(lhs_range,rhs_range);

	if (!IsPositive(ini_dist-min_range,MINIMUM_DISTANCE_TOLERANCE))
	{
		ret = REQUEST_OK_IN_RANGE;
	} 
	else
	{
		// Has the approaching character within 1/4 second of arrival at a stationary target?
		double rhs_fdt = rhs_plan->GetFinalDestinationTime();
		if (   !lhs_plan->IsMoving() 
			&& (rhs_plan->GetLastChore() == CHORE_ATTACK) 
			&& !IsPositive((float)(PreciseSubtract(rhs_fdt,start_time)),0.25f)
			)
		{
			// Test and see if the dependancy distance is close enough?
			// The lhs should just sit tight and wait for the approaching character.
			lhs_plan->SetAppendTime(rhs_fdt);
			ret = REQUEST_OK_IN_RANGE;
		}
		else
		{
			WayPointList wpl;
			bool bSuccess = gWorldPathFinder.FindPath( lhs_lnode, lhs_pos, rhs_lnode, rhs_pos, lhs_walkmask|rhs_walkmask , 0, wpl );

			if (bSuccess)
			{
				// Measure the length of the path

				if (gWorldOptions.GetHudMCP())
				{
					matrix_3x3 minc = YRotationColumns(30.0f * RealPi/180.0f);
					matrix_3x3 m = matrix_3x3::IDENTITY;
					for (WayPointIter pit = wpl.begin(); pit != wpl.end(); ++pit)
					{	
						m = m*minc;
						gWorld.DrawDebugBoxStack((*pit), 0.05f , 0xffffff00, 2.0f, m);
					}
				}

				int wpl_size = wpl.size();

				if ( wpl_size < 3 )
				{
					ret = REQUEST_OK_IN_RANGE;
				}
				else
				{
					// There are at least two nodes in the list

					bool lhs_stuck = false;
					bool rhs_stuck = false;

					float tot_dist = 0;

					// Measure the distance that the LHS may travel towards the RHS 

					WayPointIter lhs_curr = wpl.begin();
					WayPointIter rhs_curr;
					
					WayPointIter lhs_next = lhs_curr;
					WayPointIter lhs_last;
					++lhs_next;

					bool same_perms = (lhs_walkmask == rhs_walkmask);

					std::vector<float> dists;

					while (lhs_next != wpl.end())
					{
						float dist = gSiegeEngine.GetDifferenceVector(*lhs_curr,*lhs_next).Length();

						if ( !IsZero(dist,0.001f) )
						{
							if ( !same_perms )
							{
								if ( !lhs_stuck )
								{
									lhs_stuck = !gSiegeEngine.GetLogicalNodeAtPosition( (*lhs_next), lhs_walkmask );
									if ( lhs_stuck )
									{
										// The LHS can advance no further
										lhs_last = lhs_curr;
									}
								}
								
								// Optimize a little. Test the RHS while we are measuring the LHS
								if (!rhs_stuck)
								{
									rhs_stuck = !gSiegeEngine.GetLogicalNodeAtPosition( (*lhs_curr), rhs_walkmask );
								}

							}
						
							tot_dist += dist;

							gpdevreportf( &gMCPInterceptContext,("\n%02d: %8.5f %8.3f",dists.size(),dist,tot_dist) );

							dists.push_back(dist);

							lhs_curr = lhs_next;
							++lhs_next;
						}
						else
						{
							gpdevreportf( &gMCPInterceptContext,("\nskipping %8.5f",dist) );

							lhs_next = wpl.erase(lhs_next);
						}

					}

					// If the walk permission for the two sides are NOT the same, 
					// AND we've found a case where the RHS got stuck, THEN we need to 
					// measure the distance that the RHS can travel

					if (!same_perms && rhs_stuck)
					{

						WayPointIter rhs_last = wpl.end();
						--rhs_last;

						rhs_stuck = false;
						while ( !rhs_stuck && (rhs_last != wpl.begin()) )
						{
							--rhs_last;
							rhs_stuck = !gSiegeEngine.GetLogicalNodeAtPosition( (*rhs_last), rhs_walkmask );
						}
					}

					if (lhs_stuck || rhs_stuck)
					{
						//$$
						//$$ Should I handle both sides getting stuck?
						//$$   GP_Unimplemented$$$();
						//$$
						ret = REQUEST_FAILED;
					}
	/*
					else if (lhs_stuck)
					{
						GetClosestWalkablePosition()
						if (lhs_range >= rhs_range)
						{
							// Modify LHS so the it moves to closest walkable location, 
							// RHS approaches to within range of new LHS destination
							GP_Unimplemented$$$();
						}
						else
						{
							// LHS can't reach the RHS, LHS is in a vulnerable position as RHS can hit
							// but LHS can't hit back
							// RHS approaches to within range of new LHS destination. LHS will eventually
							// get a DEPENDANCY broken message and retarget
							GP_Unimplemented$$$();
						}
					else if (rhs_stuck)
					{
						if (rhs_range >= lhs_range)
						{
							// RHS moves to closest walkable location, LHS approaches to within range
							GP_Unimplemented$$$();
						}
						else
						{
							// RHS can't reach the LHS, RHS is in a vulnerable position as LHS can hit
							// but RHS can't hit back. Do not make this move, 
							ret = REQUEST_FAILED_BAD_PATH_END;
						}
					}
	*/
					else
					{
						// Neither party is limited in their movement, both should approach a collision 
						// area

						bool lhs_has_longer_range = lhs_range > rhs_range;

						float range_diff = lhs_has_longer_range ? (lhs_range-rhs_range) :  (rhs_range-lhs_range);
						float travel_time_diff = range_diff / (lhs_has_longer_range ? rhs_vel : lhs_vel);

						float travel_long_dist = tot_dist - (lhs_has_longer_range ? lhs_range : rhs_range);

						float travel_lhs_dist;
						float travel_rhs_dist;

						if (travel_long_dist < 0.0f) 
						{

							if (lhs_has_longer_range)
							{
								travel_lhs_dist = 0;
								travel_rhs_dist = tot_dist-rhs_range;
							}
							else
							{
								travel_lhs_dist = tot_dist-lhs_range;
								travel_rhs_dist = 0;
							}
						}
						else
						{
							float travel_long_time = travel_long_dist / (lhs_vel + rhs_vel);
							float travel_short_time = travel_long_time + travel_time_diff; 
							travel_lhs_dist = lhs_vel * (lhs_has_longer_range ? travel_long_time   : travel_short_time);
							travel_rhs_dist = rhs_vel * (lhs_has_longer_range ? travel_short_time  : travel_long_time );
							gpassert( IsEqual(
									(travel_lhs_dist+travel_rhs_dist),
									tot_dist - (lhs_has_longer_range ? rhs_range : lhs_range),
									0.01f) );
							gpdevreportf( &gMCPInterceptContext,("\nDIST %8.5f DIFF %8.5f TOT %8.5f\nLHS [%s:%s] %8.5f in %8.5f %8.5fm/s\nRHS [%s:%s] %8.5f in %8.5f %8.5fm/s\n",
								travel_long_dist,
								(lhs_has_longer_range ? lhs_range : rhs_range),
								tot_dist,
								GetGo(lhs_plan->m_Owner)->GetTemplateName(),
								::ToString(lhs_plan->m_Owner).c_str(),
								travel_lhs_dist,
								(lhs_has_longer_range ? travel_long_time   : travel_short_time),
								travel_lhs_dist/(lhs_has_longer_range ? travel_long_time   : travel_short_time),
								GetGo(rhs_plan->m_Owner)->GetTemplateName(),
								::ToString(rhs_plan->m_Owner).c_str(),
								travel_rhs_dist,
								(lhs_has_longer_range ? travel_short_time   : travel_long_time),
								travel_rhs_dist/(lhs_has_longer_range ? travel_short_time   : travel_long_time)
								));
						}

						std::vector<float>::const_iterator di;
						int cc;
						float accum;
/* Check the LHS for accuracy

						lhs_curr = wpl.begin();
	
						di = dists.begin();

						float catch_dist = travel_lhs_dist;

						accum = 0;
						cc=0;
						while (lhs_curr != wpl.end())
						{

							gpdevreportf( &gMCPInterceptContext,("%02d %8.3f (%8.3f)",cc++,accum,tot_dist-accum) );
							float tmp = (accum+(*di));
							gpdevreportf( &gMCPInterceptContext,(" %8.3f (%8.3f)",tmp,tot_dist-accum) );
							
							if ( tmp >= catch_dist )
							{
								gpdevreportf( &gMCPInterceptContext,(" *** LHS [%s]\n",::ToString(*lhs_curr).c_str()) );
								break;
							}
							else
							{
								gpdevreportf( &gMCPInterceptContext,("         [%s]\n",::ToString(*lhs_curr).c_str()) );
								++lhs_curr;
								accum = tmp;
								++di;
							}
						}

						if (lhs_curr != wpl.end())
						{
							
							SiegePos lhs_expected = (*lhs_curr);
							++lhs_curr;

							gpdevreportf( &gMCPInterceptContext,("%02d %8.3f (%8.3f)",cc,0,0) );
							gpdevreportf( &gMCPInterceptContext,(" %8.3f (%8.3f)",0,0) );
							gpdevreportf( &gMCPInterceptContext,("         [%s]\n",::ToString(*lhs_curr).c_str()) );
							
							float ratio = (float)((catch_dist-accum)/(*di));

							gpdevreportf( &gMCPInterceptContext,("%8.5f-%8.5f/%8.5f (%8.5f/%8.5f) %8.5f\n", catch_dist,accum,(*di),(catch_dist-accum),(*di),ratio));

							gpassert(ratio>0);
							gpassert(ratio<=1.0);

							const vector_3& dv = gSiegeEngine.GetDifferenceVector(lhs_expected,(*lhs_curr));

							gpassert(IsEqual(dv.Length(),(*di),0.01f));
							
							lhs_expected.pos += (dv*ratio);

							SiegePos probe = lhs_expected;
							dv *= 0.25f * (1.0f-ratio);
							DWORD loop;
							for (loop = 0; (loop < 3) && !gSiegeEngine.IsPositionAllowed(lhs_expected,lhs_walkmask,0.5f); loop++)
							{
								probe.pos += dv;
								lhs_expected = probe;
							}
							if (loop == 3)
							{
								// We couldn't stop at an intermediate location, need to advance to the next LHS point
								GP_Unimplemented$$$();
								lhs_expected = (*lhs_curr);
							}

							gWorld.DrawDebugBoxStack(lhs_expected, matrix_3x3::IDENTITY, 0.1f , 0xff00ffff, 2.0f);
						}
*/

						rhs_curr = wpl.end();
						--rhs_curr;

						di = dists.end();
						--di;

						accum = 0;
						cc = 0;

						bool rhs_not_at_begin = rhs_curr != wpl.begin();

						while (rhs_not_at_begin)
						{
							gpdevreportf( &gMCPInterceptContext,("%02d %8.3f (%8.3f)",cc++,accum,tot_dist-accum) );
							float tmp = accum+(*di);
							gpdevreportf( &gMCPInterceptContext,(" %8.3f (%8.3f)",tmp,tot_dist-accum) );

							if ( tmp >= travel_rhs_dist )
							{
								gpdevreportf( &gMCPInterceptContext,(" *** RHS [%s]\n",::ToString(*rhs_curr).c_str()) );
								break;
							}
							else
							{
								gpdevreportf( &gMCPInterceptContext,("         [%s]\n",::ToString(*rhs_curr).c_str()) );
								accum = tmp;
								--rhs_curr;
								rhs_not_at_begin = (rhs_curr != wpl.begin());
								if (rhs_not_at_begin) 
								{
									--di;
								}
							}
						}

						if (rhs_not_at_begin)
						{
							SiegePos rhs_expected = (*rhs_curr);
							--rhs_curr;

							gpdevreportf( &gMCPInterceptContext,("%02d %8.3f (%8.3f)",cc,0,0) );
							gpdevreportf( &gMCPInterceptContext,(" %8.3f (%8.3f)",0,0) );
							gpdevreportf( &gMCPInterceptContext,("         [%s]\n",::ToString(*rhs_curr).c_str()) );

							float ratio = (float)(travel_rhs_dist-accum)/(*di);

							gpdevreportf( &gMCPInterceptContext,("%8.5f-%8.5f/%8.5f (%8.5f/%8.5f) %8.5f\n", travel_rhs_dist,accum,(*di),(travel_rhs_dist-accum),(*di),ratio));
							
							gpassert(ratio>=0);
							gpassert(ratio<=1.0);

							vector_3 dv = gSiegeEngine.GetDifferenceVector(rhs_expected,(*rhs_curr));

							gpassert(IsEqual(dv.Length(),(*di),0.01f));
							
							rhs_expected.pos += (dv*ratio);
							SiegePos probe = rhs_expected;
							dv *= 0.25f * (1.0f-ratio);
							DWORD loop;
							for (loop = 0; (loop < 3) && !gSiegeEngine.IsPositionAllowed(rhs_expected,rhs_walkmask,0.5f); loop++)
							{
								probe.pos += dv;
								rhs_expected = probe;
							}
							if (loop == 3)
							{
								// We couldn't stop at an intermediate location, need to advance to the next RHS point
								rhs_expected = (*rhs_curr);
							}
							
							if (gWorldOptions.GetHudMCP())
							{
								gWorld.DrawDebugBoxStack(rhs_expected, 0.1f , 0xff0000ff, 2.0f);
							}

							if (rhs_plan->ChangeMoveDestination(start_time,rhs_expected,rhs_vel,rhs_walkmask, lhs_plan->m_Owner))
							{
								rhs_fdt = PreciseAdd(rhs_plan->GetFinalDestinationTime(), (lhs_has_longer_range ? -travel_time_diff : travel_time_diff));
					
								if ( lhs_plan->m_Sequencer.ClipToTime(start_time,lhs_walkmask, lhsGO) )
								{
									lhs_plan->m_Sequencer.Validate(GetGo(m_Owner),start_time);
									lhs_plan->SSendPackedClipToFollowers(start_time,lhs_plan->m_Sequencer.GetFinalPosition());
									lhs_plan->m_AppendTime = start_time;
								}
								else
								{
									lhs_plan->m_AppendTime = start_time;
								}

								bool sendchore = false;
								if (lhs_plan->GetLastChore() != CHORE_WALK)
								{
									reverse = (lhs_plan->m_LastRequestFlags & REQFLAG_FACEREVERSE)!=0;
									lhs_plan->AppendChore( CHORE_WALK, AS_DEFAULT, 0, 0, 0);
									lhs_plan->AppendFaceHeading(reverse);
									sendchore = true;
								}

								if (IsNegative((float)(PreciseSubtract(start_time,rhs_fdt)),0.01f))
								{
									ret = lhs_plan->AppendMoveWithDestinationTime( 
										rhs_expected, lhs_has_longer_range ? rhs_range : lhs_range, rhs_fdt, reverse, m_Owner );
								}
								else
								{
									ret = lhs_plan->AppendMoveHelper(rhs_expected, lhs_range, FLOAT_MAX, reverse, m_Owner);
								}

								if (RequestFailed(ret))
								{
									if (sendchore)
									{
										#if !GP_RETAIL
											if( lhs_plan->GetLastChore() != CHORE_WALK )
											{
												gperrorf((" Attempting to PopLastChore from [%s goid:0x%08x scid:0x%08x] \n"
													"But the last chore is %s, not [chore_walk]",
													GetGo(m_Owner) ? GetGo(m_Owner)->GetTemplateName() : "<invalid GO>",
													GetGo(m_Owner) ? MakeInt(GetGo(m_Owner)->GetGoid()) : 0,
													GetGo(m_Owner) ? MakeInt(GetGo(m_Owner)->GetScid()) : 0,
													::ToString(lhs_plan->GetLastChore())
												));
											}
										#endif
										// Remove the bogus walk chore that we just added
										TimeNode walk_chore;
										lhs_plan->m_Sequencer.PopLastChoreAndOrient(walk_chore);
									}
								}

							}
							else
							{
								//$$ Should I handle being unable to move the destination?
								//$$   GP_Unimplemented$$$();
								ret = REQUEST_FAILED;
							}
							
						}

						else
						{
							SiegePos rhs_expected = rhs_plan->GetFinalDestination();
							ret = lhs_plan->AppendMoveHelper(rhs_expected, lhs_range, FLOAT_MAX, reverse, m_Owner);
						}

					}
				}
			}
		}
		
	}

	return ret;

}

//////////////////////////////////////////////////////////////////////////////
eReqRetCode Plan::AppendTeleport( const SiegePos& tgtPos, const SiegeRot& tgtRot, bool flushAll )
{
	if (m_Sequencer.AppendTeleport((eRID)m_CurrentReqBlock,tgtPos,tgtRot, flushAll))
	{
		return REQUEST_OK;
	}
	return REQUEST_FAILED;
}

//////////////////////////////////////////////////////////////////////////////
bool Plan::ChangeMoveDestination(double changetime, const SiegePos& newdest, float avgvel, eLogicalNodeFlags perms, Goid targetGoid )
{

	UNREFERENCED_PARAMETER(targetGoid);

	// Should we clip immediately?
	//double changetime = m_Sequencer.GetLastTimeSent();

	SiegePos Begin;

	if (!m_Sequencer.GetNextValidPositionTime(changetime,Begin,perms))
	{
		return false;
	}
	
	SiegePos End = newdest;

	SiegeLogicalNode * pBeginLogicalNode, * pEndLogicalNode;
	eLNodeStatus err = FetchLogicalNodesHelper( Begin,	pBeginLogicalNode, perms );
	
	if (err != LNODE_OK)
	{
		gpwarning("Attempted a ChangeMoveDestination, got error fetching pBeginLogicalNode");
		return 0;
	}

	err = FetchLogicalNodesHelper( End, pEndLogicalNode, perms );
	
	if (err != LNODE_OK)
	{
		gpwarning("Attempted a ChangeMoveDestination, got error fetching pEndLogicalNode");
		return 0;
	}

	WayPointList wpl;

	bool bSuccess = gWorldPathFinder.FindPath( pBeginLogicalNode, Begin, pEndLogicalNode, End, perms, 0, wpl );

	if ( bSuccess && (wpl.size() > 0) )
	{

		m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);
		if ( m_Sequencer.ClipToTime(changetime,perms,GetGo(m_Owner)) )
		{
			m_Sequencer.Validate(GetGo(m_Owner),changetime);
			SSendPackedClipToFollowers(changetime,m_Sequencer.GetFinalPosition());
			m_AppendTime = changetime;
		}
		else
		{
			m_AppendTime = changetime;
			if (GetLastChore() != CHORE_WALK)
			{
				bool reverse = (m_LastRequestFlags & REQFLAG_FACEREVERSE)!=0;
				AppendChore( CHORE_WALK, AS_DEFAULT, 0, 0, 0 );
				AppendFaceHeading( reverse );
			}
		}
		m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);

		if (m_Sequencer.AppendWayPoints(
				m_LastRequestRID,
				m_AppendTime, // m_Sequencer.GetFinalTime(),
				wpl,
				End,
				avgvel,
				0,
				FLOAT_MAX,
				perms))
		{

			m_AppendTime = m_Sequencer.GetFinalTime();
		
			m_Sequencer.Validate(GetGo(m_Owner),m_AppendTime);

			// Make sure that we do the same thing at the end of the sequence of nodes
			if (m_LastRequestRID != RID_DUMMYSAVED)
			{
				bool reverse = (m_LastRequestFlags & REQFLAG_FACEREVERSE)!=0;
				bool noturn = (m_LastRequestFlags & REQFLAG_NOTURN)!=0;
				DWORD flg=0;
				EncodeAttackParameters ( m_LastRequestDuration,0,0,flg );
				AppendFaceTarget( m_LastRequestTarget , reverse, noturn );
				gpassert(PlanChoreLookup[m_LastRequestType] != CHORE_NONE);
				AppendChore( PlanChoreLookup[m_LastRequestType] , AS_DEFAULT,m_LastRequestSubAnim,flg,m_LastRequestRange);
			}

			return true;
		}

	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////
bool Plan::AllowCollisionPath() const
{


	bool ret = false;	
	return ret;
}

//////////////////////////////////////////////////////////////////////////////
bool Plan::IsOnInterceptPath(Plan* attackPlan) const
{

	// Return true if only the Go on the attackPlan is heading for the owner of this plan

	return  (   attackPlan->IsMoving() 
		     && (   ( attackPlan->m_LastRequestType ==  PL_ATTACK_OBJECT_MELEE )
			    )
			 && (attackPlan->DependsUpon() == m_Owner)
			);

}

//////////////////////////////////////////////////////////////////////////////
bool Plan::IsInMeleeCrowd() const
{
	bool have_arrived = !IsPositive((float)(PreciseSubtract(GetFinalDestinationTime(),gMCP.GetLeadingTime())),1.0f);

	// I have arrived at my destination
	if (have_arrived)
	{
		if ( m_LastRequestType == PL_ATTACK_OBJECT_MELEE )
		{
			// I am melee attacking
			return true;
		}

		DependantConstIter beg = m_DepChildren.begin();
		DependantConstIter end = m_DepChildren.end();
		DependantConstIter it;

		for (it = beg ; it != end ; ++it)
		{
			Plan* depplan = gMCPManager.FindPlan((*it).m_Owner,false);
			if( depplan->m_LastRequestType == PL_ATTACK_OBJECT_MELEE)
			{
				// Someone is melee attacking me!
				return true;
			}
		}
	}
		
	return false;
}


#if !GP_RETAIL
////////////////////////////////////////////////////////////////////////////////////
void Plan::DebugDraw( DWORD colour , double current_time ) const
{
	UNREFERENCED_PARAMETER(colour);
	UNREFERENCED_PARAMETER(current_time);

	SiegePos pos;

	if (m_Follower)
	{
		pos = m_Follower->GetCurrentPosition();
	}
	else
	{
		Go* go = GetGo(m_Owner);
		pos = go->GetPlacement()->GetPosition();
	}

	if (gWorldOptions.GetGoalMCP())
	{
		const gpstring& gts = GetGoalType();

		DWORD gtc = (gts.same_no_case("ADJUSTABLE")) ? 0xff00ff00 : 0xffff0000;

		SiegePos gtp = pos;
		gtp.pos.y += 2.0f;

		gWorld.DrawDebugScreenLabelColor( gtp, gts, gtc);
	}

	SiegeNodeHandle handle = gSiegeEngine.NodeCache().UseObject( pos.node );
	SiegeNode& origin_node = handle.RequestObject( gSiegeEngine.NodeCache() );

	gDefaultRapi.PushWorldMatrix();
	gDefaultRapi.SetWorldMatrix(origin_node.GetCurrentOrientation(),origin_node.GetCurrentCenter());

	if (!m_DepChildren.empty())
	{

		DependantConstIter beg = m_DepChildren.begin();
		DependantConstIter end = m_DepChildren.end();
		DependantConstIter it;

		for (it = beg ; it != end ; ++it)
		{

			Plan* childplan = gMCPManager.FindPlan((*it).m_Owner,false);
			if( childplan )
			{
				SiegePos pSrc = (*it).m_RequiredPosition;
				if (childplan->m_Follower)
				{
					pSrc = childplan->m_Follower->GetCurrentPosition();
				}
				else
				{
					Go* kidgo = GetGo(childplan->m_Owner);
					pSrc = kidgo->GetPlacement()->GetPosition();
				}


				gWorld.DrawDebugLinkedTerrainPoints(pSrc,(*it).m_RequiredPosition,0xffffffff,"");
			}
			DWORD colour;
			switch ((*it).m_Type)
			{
				case DEPTYPE_MELEE_ENEMY:	colour = 0xFF800000;  break;
				case DEPTYPE_MELEE_FRIEND:	colour = 0xFF008000;  break;
				case DEPTYPE_PROJECTILE:	colour = 0xFF000080;  break;
				case DEPTYPE_PLAIN:			colour = 0xFF400040;  break;
				default:					colour = 0xFF404040;  break;
			}
			(*it).DebugDraw(colour);
		}
	}

	if (gWorldOptions.GetBlockMCP())
	{
		if (!m_Blockers.empty())
		{
			BlockerConstIter beg = m_Blockers.begin();
			BlockerConstIter end = m_Blockers.end();
			BlockerConstIter it;

			for (it = beg; it != end; ++it)
			{
				(*it)->DebugDraw(0xff8080ff);
			}
		}
	}

	m_Sequencer.DebugDraw(current_time,GetPathWidth()*0.5f);

	gDefaultRapi.PopWorldMatrix();
}

////////////////////////////////////////////////////////////////////////////////////
bool Plan::DebugValidate() const
{


//#define MCP_PLAN_PARANOIA
#ifdef MCP_PLAN_PARANOIA

	gpassertm(VALID_TIME(m_AppendTime),"The append time is way too large, if possible at possible please stop and let me inspect your system -- mike");

	if (!m_Sections.empty())
	{

		double et = m_Sections.back()->m_EndTime;
		const SiegePos& ep = m_Sections.back()->m_Positions.empty() ? SiegePos::INVALID : m_Sections.back()->m_Positions[m_Sections.back()->m_DestinationIndex].m_Pos ;
		const SiegePos& fd = GetFinalDestination();
		gpassert (ep == fd);

		gpassertf( m_EndTime == et ,
			("This an MCP PLAN validation problem with [%s::0x%08x], you can continue",
			GetGo(m_Owner) ? GetGo(m_Owner)->GetTemplateName() : "<invalid GO>",
			GetGo(m_Owner) ? MakeInt(GetGo(m_Owner)->GetGoid()) : 0
			));

	}

#endif


	return true;
}

////////////////////////////////////////////////////////////////////////////////////
void Plan::Dump( ReportSys::ContextRef ctx )
{

	ctx->OutputF("LastResetTimeSent = %8.3f,"
				);

	ctx->OutputF("AppendTime = %2.3f,"
				 "IsLockedOn = %d\n",
				 m_AppendTime,
				 m_IsLockedOn
				);

	m_Sequencer.Dump(ctx);
	
}

#endif // !GP_RETAIL

////////////////////////////////////////////////////////////////////////////////////

FUBI_DECLARE_SELF_TRAITS( Plan::DependancyInfo );

bool Plan::DependancyInfo::Xfer( FuBi::PersistContext& persist )
{
	persist.XferHex( "m_Owner",					m_Owner					);
	persist.Xfer   ( "m_Time",					m_Time					);
	persist.Xfer   ( "m_RequiredPosition",		m_RequiredPosition		);
	persist.Xfer   ( "m_RequiredDistance",		m_RequiredDistance		);
	persist.Xfer   ( "m_ReservedPosition",		m_ReservedPosition		);
	persist.Xfer   ( "m_Type",					m_Type					);

	return ( true );
}

