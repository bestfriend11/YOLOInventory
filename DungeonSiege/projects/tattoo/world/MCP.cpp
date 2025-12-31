#include "Precomp_World.h"

#include "MCP.h"
#include "AIquery.h"
#include "Go.h"
#include "GoAspect.h"
#include "GoCore.h"
#include "GoBody.h"
#include "GoFollower.h"
#include "GoInventory.h"
#include "GoMind.h"
#include "Nema_Aspect.h"
#include "Nema_Blender.h"
#include "RapiPrimitive.h"
#include "ReportSys.h"
#include "services.h"
#include "Siege_Node.h"
#include "SkritSupport.h"
#include "space_2.h"
#include "world.h"
#include "WorldTime.h"
#include "filter_1.h"
#include <algorithm>


using namespace MCP;

//**********************************************************************************

////////////////////////////////////////////////////////////////////////////////////
// enum eOrientMode support

static const char* s_OrientModeStrings[] =
{
	"undefined",
	"lock_to_heading",
	"fixed_direction",
	"fixed_position",
	"track_object",
	"lock_to_heading_reverse",
	"fixed_position_reverse",
	"track_object_reverse",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_OrientModeStrings ) == ORIENTMODE_COUNT );

static stringtool::EnumStringConverter s_OrientModeConverter( s_OrientModeStrings, ORIENTMODE_BEGIN, ORIENTMODE_END, ORIENTMODE_UNDEFINED);

const char* ToString( eOrientMode om )
	{  return ( s_OrientModeConverter.ToString( om ) );  }
bool FromString( const char* str, eOrientMode& om )
	{  return ( s_OrientModeConverter.FromString( str, rcast <int&> ( om ) ) );  }

FUBI_EXPORT_ENUM(eOrientMode, ORIENTMODE_BEGIN , ORIENTMODE_COUNT );
FUBI_REPLACE_NAME("MCPeOrientMode",eOrientMode);
FUBI_DECLARE_ENUM_TRAITS( MCP::eOrientMode, ORIENTMODE_UNDEFINED );

////////////////////////////////////////////////////////////////////////////////////
// enum eReqFlags support

static const char* s_ReqFlagStrings[] =
{
	"reqflag_default",
	"reqflag_nomove",
	"reqflag_noturn",
	"reqflag_nomoveturn",
	"reqflag_facereverse",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_ReqFlagStrings ) == REQFLAG_COUNT );

static stringtool::EnumStringConverter s_ReqFlagConverter( s_ReqFlagStrings, REQFLAG_BEGIN, REQFLAG_END, REQFLAG_DEFAULT);

const char* ToString( eReqFlag f )
	{  return ( s_ReqFlagConverter.ToString( f ) );  }
bool FromString( const char* str, eReqFlag& f )
	{  return ( s_ReqFlagConverter.FromString( str, rcast <int&> ( f ) ) );  }

FUBI_EXPORT_ENUM(eReqFlag, REQFLAG_BEGIN , REQFLAG_END );
FUBI_REPLACE_NAME("MCPeReqFlag",eReqFlag);

////////////////////////////////////////////////////////////////////////////////////
// enum eRID support

static const char* s_RIDStrings[] =
{
	"rid_undefined",
	"rid_internal",
	"rid_lockmove",
	"rid_synch",
	"rid_clip",
	"rid_refresh",
	"rid_dummysaved",
	"rid_override",
	"rid_blockid",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_RIDStrings ) == RID_COUNT );

static stringtool::EnumStringConverter s_RIDConverter( s_RIDStrings, RID_BEGIN, RID_END, RID_BLOCKID);

const char* ToString( eRID e )
	{  return ( s_RIDConverter.ToString( e ) );  }
bool FromString( const char* str, eRID& e )
	{  return ( s_RIDConverter.FromString( str, rcast <int&> ( e ) ) );  }

FUBI_EXPORT_ENUM(eRID, RID_BEGIN , RID_COUNT );
FUBI_REPLACE_NAME("MCPeRID",eRID);
FUBI_DECLARE_CAST_TRAITS( eRID, DWORD );

//**********************************************************************************
//**********************************************************************************

////////////////////////////////////////////////////////////////////////////////
//	class Manager implementation


Manager::Manager()
{
	m_LeadingTime = 0;
	m_LaggingTime = 0;
	m_DeltaTime = 0;

	// Initialize the report contexts
	GPDEV_ONLY( GetMCPContext() );
	GPDEV_ONLY( GetMCPMessageContext() );
	GPDEV_ONLY( GetMCPCrowdContext() );
	GPDEV_ONLY( GetMCPInterceptContext() );
	GPDEV_ONLY( GetMCPSequencerContext() );
	GPDEV_ONLY( GetMCPNetLogContext() );
	GPDEV_ONLY( GetAISkritContext() );
	GPDEV_ONLY( GetAIMoveContext() );
	GPDEV_ONLY( GetGamePlayContext() );
	GPDEV_ONLY( GetJEDIContext() );
	
	gpdevreportf( &gMCPNetLogContext,("** MCP INITIALIZED ***\n"));
}


Manager::~Manager()
{
	PlanConstIter pbeg = m_Plans.begin();
	PlanConstIter pend = m_Plans.end();
	PlanConstIter pit;

	for ( pit = pbeg; pit != pend ; ++pit )
	{
		delete (*pit).second;
	}
}

////////////////////////////////////////////////////////////////////////////////////
void Manager::Reset()
{
	m_LeadingTime = 0;
	m_LaggingTime = 0;
	m_DeltaTime = 0;

	PlanConstIter beg = m_Plans.begin();
	PlanConstIter end = m_Plans.end();
	PlanConstIter it;

	for ( it = beg; it != end ; ++it )
	{
		delete (*it).second;
	}

	m_Plans.clear();

	m_RecentlyChangedNodes.clear();

}

////////////////////////////////////////////////////////////////////////////////////

FUBI_DECLARE_XFER_TRAITS( Plan* )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		if ( persist.IsRestoring() )
		{
			obj = new Plan;
		}

		return ( obj->Xfer( persist ) );
	}
};


bool Manager::Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer      ( "m_LaggingTime",                m_LaggingTime          );
	persist.Xfer      ( "m_LeadingTime",                m_LeadingTime          );
	persist.Xfer      ( "m_DeltaTime",                  m_DeltaTime            );
	persist.XferMap   ( "m_Plans",                      m_Plans                );
	persist.XferVector( "m_RecentlyChangedNodes",       m_RecentlyChangedNodes );
	
	return ( true );
}


///////////////////////////////////////////////////////////////////////////////////
Plan* Manager::FindPlan( const Goid goid, bool create )
{

	if( goid == GOID_INVALID )	return NULL;

	// Find the plan associated with this GO
	PlanIter planit = m_Plans.find( goid );
	Plan* goplan;

	if (planit != m_Plans.end())
	{
		goplan = (*planit).second;
	}
	else if (create)
	{
		// Create a new plan
		goplan = new Plan( goid,m_LeadingTime );
		gpverify( m_Plans.insert( std::make_pair(goid,goplan) ).second == true );
	}
	else
	{
		goplan = NULL;
	}

	return goplan;
}
////////////////////////////////////////////////////////////////////////////////////
void Manager::Update( )
{
	GPSTATS_GROUP( GROUP_PATH_FIND );

	double wt = gWorldTime.GetTime();

	m_DeltaTime = (float)(PreciseSubtract(wt, m_LaggingTime));
	m_LaggingTime = wt;
	m_LeadingTime = PreciseAdd(wt,gWorldOptions.GetLagMCP());

	// Check the to see if any plans have been invalidated by a new node block
	VerifyBlockedNodes();
/********
	WorldMessage msg( WE_MCP_DEPENDANCY_BROKEN, GOID_INVALID, GOID_INVALID, (DWORD)RID_OVERRIDE );

	for( PlanIter pit = m_Plans.begin(); pit != m_Plans.end(); ++pit )
	{
		Goid busted;
		if ((*pit).second->FetchDelayedDependancyToBreak(busted))
		{
			gpdevreportf( &gMCPMessageContext,("BUSTING DEPENDANCY: '%s:%s' send msg to '%s:%s'\n",
				(GetGo((*pit).first)?GetGo((*pit).first)->GetTemplateName():"<UNKNOWN>"),
				(GetGo((*pit).first)?::ToString((*pit).first).c_str():""),
				(GetGo(busted)?GetGo(busted)->GetTemplateName():"<UNKNOWN>"),
				(GetGo(busted)?::ToString(busted).c_str():"")
				));

			msg.SetSendFrom((*pit).first);
			msg.SetSendTo(busted);
			msg.SendDelayed( 0.0f );
		}
	}
*********/
}

////////////////////////////////////////////////////////////////////////////////////
void Manager::NotifyNodeStateChange(const siege::database_guid& node)
{
	m_RecentlyChangedNodes.push_back(node);
}

////////////////////////////////////////////////////////////////////////////////////
void Manager::VerifyBlockedNodes(void)
{
	if (!m_RecentlyChangedNodes.empty()) 
	{
		PlanConstIter pbeg = m_Plans.begin();
		PlanConstIter pend = m_Plans.end();
		PlanConstIter pit;

		for ( pit = pbeg; pit != pend ; ++pit )
		{
			// Is this a moving object?
			if ((*pit).second->IsMoving())
			{
				stdx::fast_vector<siege::database_guid>::const_iterator nbeg = m_RecentlyChangedNodes.begin();
				stdx::fast_vector<siege::database_guid>::const_iterator nend = m_RecentlyChangedNodes.end();
				stdx::fast_vector<siege::database_guid>::const_iterator nit;
				for (nit = nbeg ;nit != nend; ++nit)
				{
					if ((*pit).second->UsesChangedNode(*nit))
					{
						// Send a WE_MCP_NODE_BLOCKED to indicate to the AI/GoMind that we can no longer complete the 
						// request due to a blocking siege node
						WorldMessage msg1( WE_MCP_NODE_BLOCKED, GOID_INVALID, (*pit).second->GetOwner() , (DWORD)RID_OVERRIDE );
						msg1.SendDelayed( 0.0f );
						
						// Send and MCP_INVALIDATED and/or MCP_DEPENDANCY_CHANGED
						// Flag it as an override request so that we can sure that it gets received
						
						WorldMessage msg0( WE_MCP_DEPENDANCY_BROKEN, GOID_INVALID, (*pit).second->GetOwner() , (DWORD)RID_OVERRIDE );
						msg0.SendDelayed( 0.001f );

/*****
						// Make sure there is no delayed dependancy to break anymore
						Goid dummy;
						(*pit).second->FetchDelayedDependancyToBreak(dummy);
******/

						break;
					}
				}
			}

		}
		m_RecentlyChangedNodes.clear();
	}
}

////////////////////////////////////////////////////////////////////////////////////
void Manager::AppendTriggerSignals(Goid owner, const trigger::SignalSet& sigset)
{
	Plan* p = FindPlan(owner);

	if (p)
	{
		p->AppendTriggerSignals(sigset);
	}
}

////////////////////////////////////////////////////////////////////////////////////
void Manager::UpdateGo( const Goid goid )
{
	if ( !GetGo(goid)->HasFollower() )
	{
		// This is a static object (like a barrel)
		return;
	}

	////////////////////////////////////////
	//	update plan
	PlanIter planit = m_Plans.find(goid);
	if( planit != m_Plans.end() )
	{
		Plan* plan = (*planit).second;

		int crb = (int)plan->GetCurrentReqBlock();
		gpassert (crb >= 0 || (crb&0xffff0000) == 0xfeed0000);

		plan->CheckAllDependancyLinks(true);
		plan->SSendPackedUpdateToFollowers(m_LeadingTime);

		crb = (int)plan->GetCurrentReqBlock();
		gpassert (crb >= 0 || (crb&0xffff0000) == 0xfeed0000);
	}

}

////////////////////////////////////////////////////////////////////////////////////
void Manager::RemoveGo( const Goid goid )
{
	PlanIter planit = m_Plans.find(goid);
	if (planit != m_Plans.end())
	{
		delete (*planit).second;
		m_Plans.erase(planit);
	}
}

////////////////////////////////////////////////////////////////////////////////////
void Manager::ResetGo(const Goid goid)
{
	PlanIter planit = m_Plans.find(goid);
	if (planit != m_Plans.end())
	{
		delete (*planit).second;
		(*planit).second = new Plan(goid,m_LeadingTime);
	}
}

////////////////////////////////////////////////////////////////////////////////////
void Manager::TeleportGo(const Goid goid, const SiegePos& pos, const SiegeRot& rot, bool flushAll)
{
	PlanIter planit = m_Plans.find(goid);
	if (planit != m_Plans.end())
	{
//		delete (*planit).second;
//		(*planit).second = new Plan(goid,m_LeadingTime);
		(*planit).second->AppendTeleport( pos, rot, flushAll);
	}
}

////////////////////////////////////////////////////////////////////////////////////
bool Manager::Flush( const Goid goid, float delay )
{
	GPPROFILERSAMPLE( "Manager :: Flush - MCP", SP_AI );

	bool ret = false;

	PlanIter planit = m_Plans.find(goid);

	if (planit != m_Plans.end())
	{
		double t = PreciseAdd(m_LeadingTime,delay);
		(*planit).second->FlushAfterTime(t);
		ret = (t == PreciseAdd(m_LeadingTime,delay));
	}

	return ret;
}

bool Manager::BeginTeleport( Goid owner )
{
	return Flush( owner );
}

#if !GP_RETAIL
////////////////////////////////////////////////////////////////////////////////////
void Manager::DebugDraw()
{

	// We need to be within a Begin/End render!
	bool fogstate = gDefaultRapi.SetFogActivatedState( false );

	DWORD zstate;
	gDefaultRapi.GetDevice()->GetRenderState( D3DRS_ZENABLE, &zstate );
	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZENABLE, FALSE );

	PlanConstIter beg = m_Plans.begin();
	PlanConstIter end = m_Plans.end();
	PlanConstIter it;

	for ( it = beg; it != end ; ++it )
	{
		(*it).second->DebugDraw(COLOR_CHERRY,m_LaggingTime);
	}

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZENABLE, zstate );
	gDefaultRapi.SetFogActivatedState( fogstate );

}


////////////////////////////////////////////////////////////////////////////////////

void Manager :: Dump( const Goid go, ReportSys::Context * ctx )
{
	GoHandle actor( go );
	if( !actor )
	{
		return;
	}

	ctx->OutputF( "Plan dump for goid 0x%08x, '%s'\n", go, actor->GetTemplateName() );
	ctx->Output ( "=============================================\n" );

	PlanDb::iterator i = m_Plans.find( go );
	if( i != m_Plans.end() )
	{
		(*i).second->Dump( ctx );
	}
}

#endif // !GP_RETAIL

