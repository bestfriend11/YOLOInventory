#include "precomp_world.h"
/*==================================================================================================

	MCP_Sequencer -- MCP support code

	purpose:	Provides support for synchronizing PLAN information 

	author:

  (c)opyright Gas Powered Games, 2001

--------------------------------------------------------------------------------------------------*/

#include "MCP.h"
#include "MCP_Request.h"

#include "GoBody.h"
#include "GoFollower.h"
#include "GoInventory.h"
#include <algorithm>

#include "RapiPrimitive.h"
#include "World.h"
#include "ReportSys.h"

#include "Trigger_Defs.h"
#include "Trigger_Sys.h"
#include "WorldTime.h"

// Need to be able to detetmine the current state of the aspect so that 
// I can refresh stale followers properly. To do this I need to peek into the 
// nema_aspect & nema_blender

#include "GoAspect.h"
#include "nema_aspect.h"
#include "nema_blender.h"

#include "FubiBitPacker.h"

using namespace MCP;


FUBI_DECLARE_CAST_TRAITS( MCP::eRID, DWORD );
FUBI_DECLARE_ENUM_TRAITS( MCP::eOrientMode, ORIENTMODE_UNDEFINED );
FUBI_DECLARE_SELF_TRAITS( Sequencer );
FUBI_DECLARE_SELF_TRAITS( trigger::Truid );

//-------------------------------------------------------------------------
FUBI_DECLARE_SELF_TRAITS( TimeNode );
bool TimeNode::Xfer( FuBi::PersistContext& persist )
{
	persist.XferHex ( "m_Mask"		,m_Mask			);
	persist.Xfer    ( "m_Time"		,m_Time			);
	persist.Xfer    ( "m_ReqBlock"	,m_ReqBlock		);

	if (m_Mask & HAS_POSITION_FLAG)
	{
		persist.Xfer   ( "m_Dist"	,		m_Dist		);
		persist.Xfer   ( "m_Delta"	,		m_Delta		);	
		persist.Xfer   ( "m_WayPos"	,		m_WayPos	);
	}
	if (m_Mask & HAS_CHORE_FLAG)
	{
		persist.Xfer   ( "m_Chore"	,		m_Chore		);
		persist.Xfer   ( "m_Stance"	,		m_Stance	);
		persist.Xfer   ( "m_SubAnim",		m_SubAnim	);
		persist.Xfer   ( "m_Range"	,		m_Range		);
		persist.XferHex( "m_Flags"	,		m_Flags		);
	}
	if (m_Mask & HAS_ORIENT_FLAG)
	{
		persist.Xfer   ( "m_Mode"	,		m_Mode		);

		gpassertf(( (m_Mode >= ORIENTMODE_BEGIN) && (m_Mode < ORIENTMODE_END) ),
			("MCP SEQUENCER ERROR: TimeNode::Xfer() has invalid orient mode %08x [%s]",
			m_Mode,
			stringtool::MakeFourCcString( m_Mode ).c_str()
			));

		persist.Xfer   ( "m_Target"	,		m_Target	);
		persist.Xfer   ( "m_TargPos",		m_TargPos	);
		persist.Xfer   ( "m_TargRot",		m_TargRot	);		
	}
	if (m_Mask & HAS_SIGNAL_FLAG)
	{
		persist.Xfer   ( "m_Trigger",		m_Trigger   );
		persist.Xfer   ( "m_CrossType",		m_CrossType );
		persist.XferHex( "m_CondID",		m_CondID    );
		persist.Xfer   ( "m_SeqNum",		m_SeqNum    );
	}
	if (m_Mask & HAS_LOCKED_MOVEMENT_GO)
	{
		persist.Xfer   ( "m_LockedMoveGo",	m_Trigger   );
	}
	
	return ( true );
}

//-------------------------------------------------------------------------
void TimeNode::XferForSync(FuBi::BitPacker &inout_packer, double &inout_start_time )
{

	#define SYNCH_REFRESH_INTERVAL 15.0
	#define RELATIVE_TIME_BITS 14
	#define RELATIVE_TIME_MAXVAL ((1<<RELATIVE_TIME_BITS)-1)
	
	if (inout_packer.IsSaving())
	{
		double relative_time = PreciseSubtract(m_Time, inout_start_time);

		DWORD crunched = (int)ceil( relative_time * ( ((double)RELATIVE_TIME_MAXVAL)/SYNCH_REFRESH_INTERVAL) );
		bool send_synch = crunched >= RELATIVE_TIME_MAXVAL;
		inout_packer.XferBit(send_synch);
		if (send_synch)
		{
			inout_start_time = m_Time;
			inout_packer.XferRaw( inout_start_time );
		}
		else
		{
			inout_packer.XferRaw( crunched , RELATIVE_TIME_BITS );
		}
	}
	else
	{
		bool send_synch = false;
		inout_packer.XferBit(send_synch);
		if (send_synch)
		{
			inout_packer.XferRaw( inout_start_time );
			m_Time = inout_start_time;
		}
		else
		{
			DWORD crunched;
			inout_packer.XferRaw( crunched , RELATIVE_TIME_BITS );
			float relative_time = (float)(crunched * ( SYNCH_REFRESH_INTERVAL/RELATIVE_TIME_MAXVAL ));
			m_Time = PreciseAdd(inout_start_time, relative_time);
		}
	}
	
	inout_packer.XferRaw( m_Mask );

	inout_packer.XferRaw( m_ReqBlock );

	if (HasLockedMovementGo())
	{
		inout_packer.XferRaw( m_LockedMoveGo );	
	} 
	
	if ( HasPosition() )
	{
		inout_packer.XferFloat( m_WayPos.pos.x );
		inout_packer.XferFloat( m_WayPos.pos.y );
		inout_packer.XferFloat( m_WayPos.pos.z );
		inout_packer.XferRaw  ( m_WayPos.node  );	
	}

	if ( HasChore() )
	{
		inout_packer.XferRaw( m_Chore,  FUBI_MAX_ENUM_BITS( CHORE_ ));
		
		DWORD val;
		if (inout_packer.IsSaving())
		{
			val = m_Stance-AS_BEGIN;
		}
		inout_packer.XferRaw( val, FuBi::GetMaxEnumBits(0,AS_END-AS_BEGIN) );
		if (inout_packer.IsRestoring())
		{
			m_Stance = (eAnimStance)(val+AS_BEGIN);
		}

		inout_packer.XferCount( m_SubAnim, 1 );
		inout_packer.XferRaw  ( m_Flags );
	}

	if ( HasOrient() )
	{		
		inout_packer.XferRaw( m_Mode,  FUBI_MAX_ENUM_BITS( ORIENTMODE_ ) );

		if (( m_Mode == ORIENTMODE_FIXED_POSITION ) || ( m_Mode == ORIENTMODE_FIXED_POSITION_REVERSE ) )
		{
			inout_packer.XferFloat( m_TargPos.pos.x );
			inout_packer.XferFloat( m_TargPos.pos.y );
			inout_packer.XferFloat( m_TargPos.pos.z );
			inout_packer.XferRaw  ( m_TargPos.node  );	
		}
		else if (( m_Mode == ORIENTMODE_FIXED_DIRECTION ) || ( m_Mode == ORIENTMODE_FIXED_POSITION_REVERSE ) ) 
		{
			inout_packer.XferFloat( m_TargRot.rot.m_x    );
			inout_packer.XferFloat( m_TargRot.rot.m_y    );
			inout_packer.XferFloat( m_TargRot.rot.m_z    );
			inout_packer.XferFloat( m_TargRot.rot.m_w    );
			inout_packer.XferRaw  ( m_TargRot.node );	
		}
		else if (( m_Mode == ORIENTMODE_TRACK_OBJECT ) || ( m_Mode == ORIENTMODE_TRACK_OBJECT_REVERSE ) ) 
		{
			inout_packer.XferRaw( m_Target );	
		}
	}
	if ( HasSignal() )
	{

		inout_packer.XferRaw( m_Trigger );	
		inout_packer.XferRaw( m_CrossType, FUBI_MAX_ENUM_BITS( trigger::BCROSS_ ) );	
		inout_packer.XferRaw( m_CondID );	
		inout_packer.XferRaw( m_SeqNum );	
	}

};


//-------------------------------------------------------------------------
void TimeNode::Crack(int count, gpstring &buff ) const
{

	gpstring reqBlockStr;
	if ( (m_ReqBlock >= MCP::RID_BEGIN) && (m_ReqBlock < MCP::RID_END) )
	{
		reqBlockStr = ::ToString( (MCP::eRID)m_ReqBlock );
	}
	else
	{
		reqBlockStr.assignf( "%d", m_ReqBlock );
	}

	buff.appendf( "TimeNode (#%d) %8.5f reqBlock:%s ", count, m_Time, reqBlockStr.c_str());

	if (HasLockedMovementGo())
	{
		buff.appendf( "LockedMoveGo:%08x ", m_LockedMoveGo);
	} 
	
	if ( HasPosition() )
	{
		buff.appendf( "Pos:[%8.5f,%8.5f,%8.5f,%08x] ",
			m_WayPos.pos.x, m_WayPos.pos.y, m_WayPos.pos.z,m_WayPos.node );
	}

	if ( HasChore() )
	{
		// PACKED_PARAMETERS_MASK is defined in MCP_REQUEST.CPP
		const DWORD PACKED_PARAMETERS_MASK = 1<<3;
		if ( (m_Flags & PACKED_PARAMETERS_MASK) != 0 )
		{
			buff.appendf( "CHORE %s Stance:%s SubAnim:%d Flags:0x%08X (Dur:%f Elev:%f) ",
				::ToString( m_Chore ),
				::ToString( m_Stance ),
				m_SubAnim,
				m_Flags,
				DecodeAttackLoopDuration( m_Flags ),
				DecodeAttackElevation( m_Flags )
				);
		}
		else
		{
		buff.appendf( "CHORE %s Stance:%s SubAnim:%d Flags:0x%08X ",
				::ToString( m_Chore ),
				::ToString( m_Stance ),
				m_SubAnim,
				m_Flags
				);
		}
	}

	if ( HasOrient() )
	{		
		if (( m_Mode == ORIENTMODE_FIXED_POSITION ) || ( m_Mode == ORIENTMODE_FIXED_POSITION_REVERSE ) )
		{
			buff.appendf( "ORIENT [%s] TargPos:[%8.5f,%8.5f,%8.5f,%08x] ",
				::ToString(m_Mode), m_TargPos.pos.x, m_TargPos.pos.y, m_TargPos.pos.z,m_TargPos.node );
		}
		else if (( m_Mode == ORIENTMODE_FIXED_DIRECTION ) || ( m_Mode == ORIENTMODE_FIXED_POSITION_REVERSE ) ) 
		{
			buff.appendf( "ORIENT [%s] TargRot:[%8.5f,%8.5f,%8.5f,%08x] ",
				::ToString(m_Mode), m_TargRot.rot.m_x, m_TargRot.rot.m_y, m_TargRot.rot.m_z, m_TargRot.rot.m_w,m_TargRot.node );
		}
		else if (( m_Mode == ORIENTMODE_TRACK_OBJECT ) || ( m_Mode == ORIENTMODE_TRACK_OBJECT_REVERSE ) ) 
		{
			buff.appendf( "ORIENT [%s] Target:[%08x] ",
				::ToString(m_Mode), m_Target);
		}
	}

	if ( HasSignal() )
	{
		buff.appendf( "SIGNAL Truid:%08X:%04X:%04X seqNum:%d cross:%s",
			m_Trigger.GetOwnGoid(),
			m_Trigger.GetTrigNum(),
			m_CondID,
			m_SeqNum,
			::ToString(m_CrossType) );
	}

};

//-------------------------------------------------------------------------
Sequencer::Sequencer()
	: m_LastTimeSent(DBL_MAX)
	, m_LastChoreTransmitted(CHORE_NONE)
	, m_LastStanceTransmitted(AS_DEFAULT)
	, m_LastSubAnimTransmitted(0)
	, m_LastFlagsTransmitted(0)
	, m_LockedMovementGo(GOID_INVALID)
	, m_WayPosNodeTransmitted(SiegePos::INVALID)
{
};

//-------------------------------------------------------------------------
bool Sequencer::Xfer( FuBi::PersistContext& persist )
{
	persist.XferList( "m_NodeList",					m_NodeList					);
	persist.Xfer    ( "m_LastTimeSent",				m_LastTimeSent				);
	persist.Xfer    ( "m_LastChoreTransmitted",		m_LastChoreTransmitted		);
	persist.Xfer    ( "m_LastStanceTransmitted",	m_LastStanceTransmitted		);
	persist.Xfer    ( "m_LastSubAnimTransmitted",	m_LastSubAnimTransmitted	);
	persist.Xfer    ( "m_LastFlagsTransmitted",		m_LastFlagsTransmitted		);
	persist.Xfer	( "m_LockedMovementGo",			m_LockedMovementGo			);
	persist.Xfer	( "m_WayPosNodeTransmitted",	m_WayPosNodeTransmitted		);
	return true;
}

//-------------------------------------------------------------------------
bool Sequencer::Initialize(eRID init_rb,double init_time, const SiegePos& init_pos, const SiegeRot& init_rot)
{

	TimeNode newnode;
	
	newnode.SetReqBlock(init_rb);
	newnode.SetTime(init_time);
	newnode.SetOrient(ORIENTMODE_UNDEFINED,init_rot);
	newnode.SetPosition(0,vector_3::ZERO,init_pos);

	m_NodeList.clear();
	m_NodeList.push_back(newnode);

	m_LastTimeSent = DBL_MAX;
	m_LockedMovementGo = GOID_INVALID;

	return true;
}

//-------------------------------------------------------------------------
TimeNodeIter Sequencer::FindNodeAtOrBeforeTime(double t)
{

	if (!m_NodeList.empty())
	{

		TimeNodeIter beg = m_NodeList.begin();
		TimeNodeIter it = m_NodeList.end();

		do {
			--it;
			if ((*it).m_Time <= t)
			{
				return (it);
			}
		} while ( it != beg );
	}

	return m_NodeList.end();
}

//-------------------------------------------------------------------------
TimeNodeIter Sequencer::FindNodeAtOrAfterTime(double t)
{
	TimeNodeIter it = m_NodeList.begin();
	TimeNodeIter end = m_NodeList.end();

	for ( ; it != end; ++it ) 
	{
		double tm = (*it).m_Time;
		
		if ( tm >= t )
		{
			return (it);
		}
	}
	return m_NodeList.end();
}

//-------------------------------------------------------------------------
void Sequencer::AdjustTimeline(TimeNodeIter sn, double scale, double shift)
{
	std::for_each(sn,m_NodeList.end(),TimeNode::TimeAdjust(scale,shift));
}

//-------------------------------------------------------------------------
bool Sequencer::IsPositionConstant(void) const
{

	if (m_NodeList.empty()) return true;

	TimeNodeConstIter beg = m_NodeList.begin();
	TimeNodeConstIter it = m_NodeList.end();
	
	--it;

	if ((*it).HasTransmitted())
	{
		return true;
	}

	// Search for a change in position
	for (;it != beg; --it ) 
	{
		if ((*it).m_WayPos != (*beg).m_WayPos)
		{
			return false;
		}
	}
	return true;
}

//-------------------------------------------------------------------------
SiegePos Sequencer::GetFinalPosition(void) const
{
	if (m_NodeList.empty()) return SiegePos::INVALID;

	return  m_NodeList.back().m_WayPos;
	
}

////////////////////////////////////////////////////////////////////////////////////
bool Sequencer::GetPosition(double& t, bool no_interp, bool skip_locked, SiegePos& retval)
{

	if (m_NodeList.empty()) 
	{
		return false;
	}

	if (skip_locked)
	{
		bool dummy1,dummy2;
		TimeNodeIter lntk = FindLastNodeToKeep(t,dummy1,dummy2,false);	
		if (lntk == m_NodeList.end())
		{
			retval = SiegePos::INVALID;
			return false;
		}
		else
		{
			t = (*lntk).m_Time;
			retval = (*lntk).m_WayPos;
			return true;
		}
	}
	
	TimeNodeConstIter beg = m_NodeList.begin();
	TimeNodeConstIter end = m_NodeList.end();
	TimeNodeConstIter mid_node = end;

	bool found = false;
	do {
		--mid_node;
		if ((*mid_node).m_Time <= t)
		{
			found = true;
		}
	} while ( !found && (mid_node != beg) );

	if ( !found ) 
	{
		retval = SiegePos::INVALID;
		return false;
	} 
	else
	{
		TimeNodeConstIter rhs_node = mid_node;

		++rhs_node;

		if ( no_interp || (rhs_node == m_NodeList.end()) )
		{
			retval = (*mid_node).m_WayPos;
		}
		else
		{

			TimeNodeConstIter lhs_node;

			double mt = (*mid_node).m_Time;
			double rt = (*rhs_node).m_Time;

			float i0;
			float i1 = (float)(PreciseSubtract(rt,mt))*0.5f;

			float normalized_delta = (float)(PreciseSubtract(t,mt))/i1;

			float tau;

			if (normalized_delta <= 1.0f)
			{
				lhs_node = mid_node;
				if (lhs_node!=beg)
				{
					--lhs_node;
				}

				double lt = (*lhs_node).m_Time;
				
				i0 = (float)(PreciseSubtract(mt, lt)) * 0.5f;
				tau = (float)(i0 + (i1 * normalized_delta)) / (i0+i1);

			} 
			else
			{
				// We are more than halfway to the next interval
				// so advance all the iterators

				lhs_node = mid_node;
				mid_node = rhs_node;
				++rhs_node;

				mt = rt;
				if (rhs_node != end) 
				{
					rt = (*rhs_node).m_Time;
				}

				i0 = (float)(PreciseSubtract(rt, mt)) * 0.5f;
				tau = (i1 * (normalized_delta-1)) / (i0+i1);

			}

			vector_3 p0 = (mid_node==beg) ? vector_3::ZERO :
							gSiegeEngine.GetDifferenceVector((*mid_node).m_WayPos,(*lhs_node).m_WayPos);
			vector_3 p1 = (rhs_node==end) ? vector_3::ZERO :
							gSiegeEngine.GetDifferenceVector((*mid_node).m_WayPos,(*rhs_node).m_WayPos);

			p0 *= 0.5f * (1-tau);
			p1 *= 0.5f * (tau);

			retval = (*mid_node).m_WayPos;
			retval.pos += p0 + ((p1-p0) * tau);

		}
	}

	return true;
}

//-------------------------------------------------------------------------
bool Sequencer::GetNextValidPositionTime(double &t, SiegePos& retval, eLogicalNodeFlags perms) const
{
	if (m_NodeList.empty()) 
	{
		return false;
	}

	TimeNodeConstIter beg = m_NodeList.begin();
	TimeNodeConstIter end = m_NodeList.end();
	TimeNodeConstIter mid_node = end;

	bool found = false;
	do {
		--mid_node;
		if ((*mid_node).m_Time <= t)
		{
			found = true;
		}
	} while ( !found && (mid_node != beg) );

	if ( !found ) 
	{
		retval = GetFinalPosition();
	} 
	else
	{

		TimeNodeConstIter rhs_node = mid_node;

		++rhs_node;

		if ( rhs_node == m_NodeList.end() )
		{
			retval = (*mid_node).m_WayPos;
		}
		else
		{
			TimeNodeConstIter lhs_node;

			double mt = (*mid_node).m_Time;
			double rt = (*rhs_node).m_Time;

			float i0;
			float i1 = (float)(PreciseSubtract(rt,mt))*0.5f;

			float normalized_delta = (float)(PreciseSubtract(t,mt))/i1;

			float tau;

			if (normalized_delta <= 1.0f)
			{
				lhs_node = mid_node;
				if (lhs_node!=beg)
				{
					--lhs_node;
				}

				double lt = (*lhs_node).m_Time;
				
				i0 = (float)(PreciseSubtract(mt, lt)) * 0.5f;
				tau = (float)(i0 + (i1 * normalized_delta)) / (i0+i1);

			} 
			else
			{
				// We are more than halfway to the next interval
				// so advance all the iterators

				lhs_node = mid_node;
				mid_node = rhs_node;
				++rhs_node;

				mt = rt;
				if (rhs_node != end) 
				{
					rt = (*rhs_node).m_Time;
				}

				i0 = (float)(PreciseSubtract(rt, mt)) * 0.5f;
				tau = (i1 * (normalized_delta-1)) / (i0+i1);

			}

			vector_3 p0 = (mid_node==beg) ? vector_3::ZERO :
							gSiegeEngine.GetDifferenceVector((*mid_node).m_WayPos,(*lhs_node).m_WayPos);
			vector_3 p1 = (rhs_node==end) ? vector_3::ZERO :
							gSiegeEngine.GetDifferenceVector((*mid_node).m_WayPos,(*rhs_node).m_WayPos);

			p0 *= 0.5f * (1-tau);
			p1 *= 0.5f * (tau);

			retval = (*mid_node).m_WayPos;
			retval.pos += p0 + ((p1-p0) * tau);

			if (!gSiegeEngine.IsPositionAllowed(retval,perms,0.5f))
			{
				// We can't stop in the middle, just move to the next waypoint
				// $$ Should I do anything fancy like searching for the first valid 
				// position in the interval?
				t = (*mid_node).m_Time;
				retval.pos = (*mid_node).m_WayPos.pos;
			}

		}
	}

	return true;
}

//-------------------------------------------------------------------------
eAnimChore Sequencer::GetLastChore(void) const 
{

	if (m_NodeList.empty()) return m_LastChoreTransmitted;

	TimeNodeConstIter it = m_NodeList.end();
	do {
		--it;
		if ((*it).HasChore())
		{
			return (*it).m_Chore;
		}
	} while (it != m_NodeList.begin());

	return m_LastChoreTransmitted;

}

//-------------------------------------------------------------------------
bool Sequencer::GetLastChoreAndStance(eAnimChore& slc, eAnimStance& sls) const
{

	if (m_NodeList.empty()) return false;

	TimeNodeConstIter it = m_NodeList.end();
	do {
		--it;
		if ((*it).HasChore())
		{
			slc = (*it).m_Chore;
			sls = (*it).m_Stance;
			return true;
		}
	} while (it != m_NodeList.begin());

	slc = m_LastChoreTransmitted;
	sls = m_LastStanceTransmitted;

	return true;

}

//-------------------------------------------------------------------------
bool Sequencer::UsesNode(const siege::database_guid query_node) const 
{

	TimeNodeConstIter it = m_NodeList.begin();
	TimeNodeConstIter end = m_NodeList.end();
	
	for (;it!=end;++it)
	{
		if ((*it).m_WayPos.node == query_node)
		{
			return true;
		}
	}
	return false;
}


//-------------------------------------------------------------------------
bool Sequencer::CheckForCompletion(double t) 
{
	if (!m_NodeList.empty())
	{
		TimeNode& ln = m_NodeList.back();
		if (!ln.HasCompleted())
		{
			if (ln.m_Time <= t)
			{
				ln.SetCompleted();
				return true;
			}
		}
	}
	return false;
}

//-------------------------------------------------------------------------
bool Sequencer::AppendWayPoints(eRID rb, double start_t, WayPointList& wpl, const SiegePos& dest, float v,float range, float max_t, siege::eLogicalNodeFlags perms)
{

	bool modifybacknode;
	TimeNode& ln = m_NodeList.back();
	double last_t = ln.m_Time;

	bool transmitted = ln.HasTransmitted() || ln.HasLockedIn();
	if  ( transmitted || (start_t > last_t) )
	{
		// We are appending more nodes with a 'gap' between the current end and the new WPL
		last_t = start_t;
		modifybacknode = false;
	}
	else
	{
		// We need to modify the last node so that there are no gaps in the time line
		// We test to see if the points are the same, they won't be in an instantaneous teleport situation
		modifybacknode = wpl.front() == ln.m_WayPos;
	}

	WayPointIter beg = wpl.begin();
	WayPointIter end = wpl.end();
	WayPointIter cp = beg;
	WayPointIter np = cp;

	float inv_v = IsZero(v) ? 0.0f : 1.0f/v;

	vector_3 currpoint = gSiegeEngine.GetDifferenceVector(dest,(*cp));

	TimeNode tn;

	// All new nodes created will have the same request block
	tn.SetReqBlock(rb);

	while ( (cp != end) && ((PreciseSubtract(last_t, start_t)) <= max_t) )
	{
		tn.SetTime(last_t);

		vector_3 delta(DoNotInitialize);
		float dist = 0.0f;

		while ( IsZero(dist) )
		{
			++np;
			if (np==end)
			{
				break;
			}
			else
			{
				delta = gSiegeEngine.GetDifferenceVector((*cp),(*np));
				dist = delta.Length();
			}
		}

		if (np == end) 
		{
			if (modifybacknode)
			{
				modifybacknode = false;
				m_NodeList.back().SetPosition(0,vector_3::ZERO,(*cp));
			}
			else
			{
				tn.SetPosition(0,vector_3::ZERO,(*cp));
				m_NodeList.push_back(tn);
			}
			cp = end;
		}
		else
		{
			vector_3 nextpoint = gSiegeEngine.GetDifferenceVector(dest,(*np));
			
			float tau;

			if ( DirectedSegmentPenetratesSphereAtOrigin(currpoint,nextpoint,range,tau) )
			{
				// Create an new point at tau position, if tau is an illegal move, then
				// sample three intermediate position to see if we can find a location

				float interp_dist = dist * tau;
				
				vector_3 interp_delta = delta * tau;

				// interpolate an FORWARD point and a REVERSE point that is relative
				// to far side of the interval

				vector_3 deltaREV = gSiegeEngine.GetDifferenceVector((*np),(*cp));
				vector_3 interpREV_delta = deltaREV * (1.0f-tau);

				SiegePos interp = (*cp);
				interp.pos += interp_delta;
				
				SiegePos interpREV = (*np);
				interpREV.pos += interpREV_delta;

				const float onefourthremainder = ((1.0f-tau)*0.25f);
				
				vector_3 delta_step = delta * onefourthremainder;
				float dist_step = dist * onefourthremainder;

				vector_3 deltaREV_step = -interpREV_delta * onefourthremainder;

				for (int loop = 0; loop<4 ; ++loop)
				{

					if (gSiegeEngine.IsPositionAllowed(interp,perms,0.5f))
					{
						break;
					}
					else 
					{
						if (gSiegeEngine.IsPositionAllowed(interpREV,perms,0.5f))
						{
							// gpwarning("Good news, we just avoided a hideous pop! -- biddle");
							interp = interpREV;
							break;
						}
					}

					if (loop == 3)
					{
						interp = (*np);
						interp_dist = dist;
						interp_delta = delta;
					}
					else
					{
						interp_dist += dist_step;

						interp_delta += delta_step;
						interp = (*cp);
						interp.pos += interp_delta;

						interpREV_delta += deltaREV_step;
						interpREV = (*np);
						interpREV.pos += interpREV_delta;
					}
				}

				if (modifybacknode)
				{
					modifybacknode = false;
					m_NodeList.back().SetPosition(interp_dist,interp_delta,(*cp));
				}
				else
				{
					tn.SetPosition(interp_dist,interp_delta,(*cp));
					m_NodeList.push_back(tn);
				}

				last_t = PreciseAdd( last_t, interp_dist * inv_v );

				tn.SetTime(last_t);
				tn.SetPosition(0,vector_3::ZERO,interp);
				m_NodeList.push_back(tn);

				cp = end;
			}
			else
			{

				if (modifybacknode)
				{
					modifybacknode = false;
					m_NodeList.back().SetPosition(dist,delta,(*cp));
				}
				else
				{
					tn.SetPosition(dist,delta,(*cp));
					m_NodeList.push_back(tn);
				}

				last_t = PreciseAdd( last_t, dist * inv_v );
				currpoint = nextpoint;

				cp = np;
			}

		}

	}

	return ((PreciseSubtract(last_t, start_t)) <= max_t);

}

//-------------------------------------------------------------------------
bool Sequencer::AppendWayPointsWithArrivalTime(eRID rb, double start_t, WayPointList& wpl, const SiegePos& dest, float v,float range, double arrive_time, siege::eLogicalNodeFlags perms)
{

	bool modifybacknode;
	
	TimeNode& ln = m_NodeList.back();
	double last_t = ln.m_Time;

	bool transmitted = ln.HasTransmitted() || ln.HasLockedIn();
	if  ( transmitted || (start_t > last_t) )
	{
		// We are appending more nodes with a 'gap' between the current end and the new WPL
		last_t = start_t;
		modifybacknode = false;
	}
	else
	{
		// We need to modify the last node so that there are no gaps in the time line
		// We test to see if the points are the same, they won't be in an instantaneous teleport situation
		modifybacknode = wpl.front() == ln.m_WayPos;
	}

	WayPointIter beg = wpl.begin();
	WayPointIter end = wpl.end();
	WayPointIter cp = beg;
	WayPointIter np = cp;

	float inv_v = IsZero(v) ? 0.0f : 1.0f/v;

	vector_3 currpoint = gSiegeEngine.GetDifferenceVector(dest,(*cp));

	TimeNode tn;

	// All new nodes created will have the same request block
	tn.SetReqBlock(rb);

	while ( (cp != end) )
	{
		tn.SetTime(last_t);

		vector_3 delta(DoNotInitialize);
		float dist = 0.0f;

		while ( IsZero(dist) )
		{
			++np;
			if (np==end)
			{
				break;
			}
			else
			{
				delta = gSiegeEngine.GetDifferenceVector((*cp),(*np));
				dist = delta.Length();
			}
		}

		if (np == end) 
		{
			if (modifybacknode)
			{
				modifybacknode = false;
				m_NodeList.back().SetPosition(0,vector_3::ZERO,(*cp));
			}
			else
			{
				tn.SetPosition(0,vector_3::ZERO,(*cp));
				m_NodeList.push_back(tn);
			}
			cp = end;
		}
		else
		{
			vector_3 nextpoint = gSiegeEngine.GetDifferenceVector(dest,(*np));
			
			float tau;

			if ( DirectedSegmentPenetratesSphereAtOrigin(currpoint,nextpoint,range,tau) )
			{
				// Create an new point at tau position, if tau is an illegal move, then
				// sample three intermediate position to see if we can find a location
				float interp_dist = dist * tau;
				vector_3 interp_delta = delta * tau;

				// interpolate an FORWARD point and a REVERSE point that is relative
				// to far side of the interval

				vector_3 deltaREV = gSiegeEngine.GetDifferenceVector((*np),dest);

				SiegePos interp = (*cp);
				SiegePos interpREV = (*np);

				interp.pos += interp_delta;
				interpREV.pos += deltaREV * (1.0f-tau);

				const float onefourthremainder = ((1.0f-tau)*0.25f);
				
				vector_3 delta_step = delta * onefourthremainder;
				float dist_step = dist * onefourthremainder;

				vector_3 deltaREV_step = -deltaREV * onefourthremainder;

				for (int loop = 0; loop<4 ; ++loop)
				{

					if (gSiegeEngine.IsPositionAllowed(interp,perms,0.5f))
					{
						break;
					}
					else if (gSiegeEngine.IsPositionAllowed(interpREV,perms,0.5f))
					{
						//gpwarning("Good news, we just avoided a hideous pop! -- biddle");
						interp = interpREV;
						break;
					}

					if (loop == 3)
					{
						interp = (*np);
						interp_dist = dist;
						interp_delta = delta;
					}
					else
					{
						interp.pos += delta_step;
						interp_delta += delta_step;
						interp_dist += dist_step;
						interpREV.pos += deltaREV_step;
					}
				}

				if (modifybacknode)
				{
					modifybacknode = false;
					m_NodeList.back().SetPosition(interp_dist,interp_delta,(*cp));
				}
				else
				{
					tn.SetPosition(interp_dist,interp_delta,(*cp));
					m_NodeList.push_back(tn);
				}

				last_t = PreciseAdd( last_t, interp_dist * inv_v );

				tn.SetTime(last_t);
				tn.SetPosition(0,vector_3::ZERO,interp);
				m_NodeList.push_back(tn);

				cp = end;
			}
			else
			{

				if (modifybacknode)
				{
					modifybacknode = false;
					m_NodeList.back().SetPosition(dist,delta,(*cp));
				}
				else
				{
					tn.SetPosition(dist,delta,(*cp));
					m_NodeList.push_back(tn);
				}

				last_t = PreciseAdd( last_t, dist * inv_v );
				currpoint = nextpoint;

				cp = np;
			}

		}

	}

	double fdt = m_NodeList.back().m_Time;
	
	// scale all the nodes if round off is off by 1/20 of second or more
	if (!IsEqual((float)(PreciseSubtract(fdt,arrive_time)),0.05f))
	{
		// Adjust all of the new nodes so that we arrive at the correct time

		TimeNodeIter first_to_adj = m_NodeList.begin();
		++first_to_adj;

		while ( (first_to_adj != m_NodeList.end()) 
			&& ((*first_to_adj).HasTransmitted() || (*first_to_adj).HasLockedIn()) )
		{
			++first_to_adj;
		}


		if (first_to_adj!=m_NodeList.end()) 
		{
			TimeNodeIter last_unch = first_to_adj;
			--last_unch;
			double unch_time = (*last_unch).m_Time;

			float denom = (float)(PreciseSubtract(fdt,unch_time));

			// ignore differences of less than 1/20 of a second
			if (IsZero(denom,0.05f))
			{
				m_NodeList.back().m_Time = arrive_time;
			}
			else
			{
				double timescale = PreciseSubtract(arrive_time,unch_time)/denom;
				gpdevreportf( &gMCPInterceptContext,("AppendWayPointsWithArrivalTime: scale %ff\n",timescale));
				AdjustTimeline(first_to_adj, timescale, unch_time);
			}
		}

	}
	else
	{
		m_NodeList.back().m_Time = arrive_time;
	}

	double nfdt = m_NodeList.back().m_Time;

	return IsZero((float) PreciseSubtract(arrive_time,nfdt),0.001f);

}

//-------------------------------------------------------------------------
bool Sequencer::AppendChore(eRID rb,double start_t,eAnimChore ch,eAnimStance stance, DWORD subanim, DWORD flags, float range)
{

	TimeNodeIter it = m_NodeList.end();
	if (it != m_NodeList.begin()) --it;

	TimeNode& ln = m_NodeList.back();
	double last_t = ln.m_Time;
	bool transmitted = m_NodeList.back().HasTransmitted();
	gpassert(start_t>=last_t);
	if (transmitted || (start_t > last_t)) 
	{
		// We are appending more nodes with a 'gap' between the current end and this new chore
		TimeNode tn;
		tn.SetReqBlock(rb);
		tn.SetTime(start_t);
		tn.SetChore( ch, stance , subanim , range, flags );
		tn.SetPosition(0,vector_3::ZERO,m_NodeList.back().m_WayPos);
		m_NodeList.push_back(tn);
	}
	else
	{
		// We need to modify the last node so that there are no gaps in the time line
		TimeNode& tn = m_NodeList.back();
		tn.SetReqBlock(rb);
		tn.SetChore( ch, stance , subanim , range, flags );
	}

	return true;
}

//-------------------------------------------------------------------------
bool Sequencer::AppendOrient(eRID rb ,double start_t,eOrientMode md )
{
	double last_t = m_NodeList.back().m_Time;
	bool transmitted = m_NodeList.back().HasTransmitted();
	gpassert(start_t>=last_t);
	if (transmitted || (start_t > last_t)) 
	{									
		TimeNode tn;					
		tn.SetReqBlock(rb);				
		tn.SetTime(start_t);			
		tn.SetOrient( md );				
		tn.SetPosition(0,vector_3::ZERO,m_NodeList.back().m_WayPos);
		m_NodeList.push_back(tn);		
	}									
	else								
	{
		TimeNode& tn = m_NodeList.back();
		tn.SetReqBlock(rb);
		tn.SetOrient( md );
	}
	
	return true;
}

//-------------------------------------------------------------------------
bool Sequencer::AppendOrient(eRID rb ,double start_t,eOrientMode md, Goid targ)
{
	double last_t = m_NodeList.back().m_Time;
	bool transmitted = m_NodeList.back().HasTransmitted();
	gpassert(start_t>=last_t);
	if (transmitted || (start_t > last_t)) 
	{									
		TimeNode tn;					
		tn.SetReqBlock(rb);				
		tn.SetTime(start_t);			
		tn.SetOrient( md, targ );				
		tn.SetPosition(0,vector_3::ZERO,m_NodeList.back().m_WayPos);
		m_NodeList.push_back(tn);		
	}									
	else								
	{
		TimeNode& tn = m_NodeList.back();
		tn.SetReqBlock(rb);
		tn.SetOrient( md, targ );
	}

	return true;
}

//-------------------------------------------------------------------------
bool Sequencer::AppendOrient(eRID rb ,double start_t,eOrientMode md, SiegePos tpos)
{
	double last_t = m_NodeList.back().m_Time;
	bool transmitted = m_NodeList.back().HasTransmitted();
	gpassert(start_t>=last_t);
	if (transmitted || (start_t > last_t)) 
	{									
		TimeNode tn;
		tn.SetReqBlock(rb);				
		tn.SetTime(start_t);			
		tn.SetOrient( md ,tpos );				
		tn.SetPosition(0,vector_3::ZERO,m_NodeList.back().m_WayPos);
		m_NodeList.push_back(tn);		
	}									
	else								
	{
		TimeNode& tn = m_NodeList.back();
		tn.SetReqBlock(rb);
		tn.SetOrient( md, tpos );
	}

	return true;
}

//-------------------------------------------------------------------------
bool Sequencer::AppendOrient(eRID rb ,double start_t,eOrientMode md, SiegeRot trot)
{
	double last_t = m_NodeList.back().m_Time;
	bool transmitted = m_NodeList.back().HasTransmitted();
	gpassert(start_t>=last_t);
	if (transmitted || (start_t > last_t)) 
	{									
		TimeNode tn;					
		tn.SetReqBlock(rb);				
		tn.SetTime(start_t);			
		tn.SetOrient( md, trot );				
		tn.SetPosition(0,vector_3::ZERO,m_NodeList.back().m_WayPos);
		m_NodeList.push_back(tn);		
	}									
	else								
	{
		TimeNode& tn = m_NodeList.back();
		tn.SetReqBlock(rb);
		tn.SetOrient( md, trot );
	}

	return true;
}

//-------------------------------------------------------------------------
bool Sequencer::AppendTeleport(eRID rb ,const SiegePos& tpos, const SiegeRot& trot, bool flushAll)
{
	TimeNode& ln = m_NodeList.back();
	double last_t = max_t(ln.m_Time,gMCP.GetLeadingTime());

	TimeNode tn;		

	if( flushAll )
	{
		double	earlyClipTime	= 0.0;
		bool	mustLock		= false;
		bool	invalidChore	= false;

		FindLastNodeToKeep(earlyClipTime, mustLock, invalidChore, false);
		if( mustLock )
		{
			gperrorf( ("APPENDTELEPORT: Trying to teleport with locked MCP nodes!!!\n"
				"Please move the teleporter to a safe haven away from triggers and elevators and other undesireables") );
		}

		m_NodeList.clear();
	}
	else
	{
		if (ln.HasTransmitted()) 
		{
			ln.m_Mask &= ~HAS_TRANSMITTED;
		}

		ln.SetTime(last_t);			
		ln.m_Dist = -1.0f;
		ln.m_Delta = vector_3::ZERO;
	}

	tn.SetReqBlock(rb);				
	tn.SetTime(last_t);			
	tn.SetPosition( 0, vector_3::ZERO, tpos);				
	tn.SetOrient( ORIENTMODE_FIXED_DIRECTION, trot );				
	m_NodeList.push_back(tn);		
	
	return true;
}

//-------------------------------------------------------------------------
bool Sequencer::AppendLockedMovementGo(eRID rb, double start_t, Goid tg)
{
	double last_t = m_NodeList.back().m_Time;
	bool transmitted = m_NodeList.back().HasTransmitted();
	gpassert(start_t>=last_t);
	if (transmitted || (start_t > last_t)) 
	{									
		TimeNode tn;					
		tn.SetReqBlock(rb);				
		tn.SetTime(start_t);			
		tn.SetLockedMovementGo( tg );				
		tn.SetPosition(0,vector_3::ZERO,m_NodeList.back().m_WayPos);
		tn.SetLockedIn();
		m_NodeList.push_back(tn);		
	}									
	else								
	{
		TimeNode& tn = m_NodeList.back();
		tn.SetReqBlock(rb);
		tn.SetLockedMovementGo( tg );
		tn.SetLockedIn();
	}

	m_LockedMovementGo = tg;


	return true;
}
//-------------------------------------------------------------------------
bool Sequencer::PopLastChoreAndOrient(TimeNode& tn)
{

	if (m_NodeList.empty())
	{
		return false;
	}

	// Pop the last chore and orient if we haven't been transmitted yet

	TimeNode& last = m_NodeList.back();

	if (last.HasTransmitted())
	{
		return false;
	}

	tn.m_Mask		= (WORD)(last.m_Mask & (HAS_ORIENT_FLAG|HAS_CHORE_FLAG));
	tn.m_Time		= last.m_Time;
	tn.m_ReqBlock	= last.m_ReqBlock;
	
	if (last.HasChore())
	{
		tn.m_Chore   = last.m_Chore; 
		tn.m_Stance  = last.m_Stance;
		tn.m_SubAnim = last.m_SubAnim;
		tn.m_Range   = last.m_Range;  
		tn.m_Flags   = last.m_Flags;
	}
	if (last.HasOrient())
	{
		tn.m_Mode    = last.m_Mode;
		tn.m_Target  = last.m_Target;
		tn.m_TargPos = last.m_TargPos;
		tn.m_TargRot = last.m_TargRot;
	}

	last.m_Mask &= ~(HAS_ORIENT_FLAG|HAS_CHORE_FLAG);

	return true;
}

//-------------------------------------------------------------------------
bool Sequencer::GetLastChoreAndOrient(TimeNode& tn)
{
	if (m_NodeList.empty())
	{
		return false;
	}

	TimeNode& last = m_NodeList.back();

	tn.m_Mask		= (WORD)(last.m_Mask & (HAS_ORIENT_FLAG|HAS_CHORE_FLAG));
	tn.m_Time		= last.m_Time;
	tn.m_ReqBlock	= last.m_ReqBlock;
	
	if (last.HasChore())
	{
		tn.m_Chore   = last.m_Chore; 
		tn.m_Stance  = last.m_Stance;
		tn.m_SubAnim = last.m_SubAnim;
		tn.m_Range   = last.m_Range;  
		tn.m_Flags   = last.m_Flags;
	}
	if (last.HasOrient())
	{
		tn.m_Mode    = last.m_Mode;
		tn.m_Target  = last.m_Target;
		tn.m_TargPos = last.m_TargPos;
		tn.m_TargRot = last.m_TargRot;
	}

	return true;
}

//-------------------------------------------------------------------------
bool Sequencer::SetLastChoreAndOrient(const TimeNode& tn)
{
	if (m_NodeList.empty())
	{
		return false;
	}

	TimeNode& last = m_NodeList.back();

	last.m_Mask	|= (tn.m_Mask & (HAS_ORIENT_FLAG|HAS_CHORE_FLAG));
	last.m_ReqBlock	= tn.m_ReqBlock;

	if (tn.HasChore())
	{
		last.m_Chore   = tn.m_Chore;
		last.m_Stance  = tn.m_Stance;
		last.m_SubAnim = tn.m_SubAnim;
		last.m_Range   = tn.m_Range;
		last.m_Flags   = tn.m_Flags;
	}
	if (tn.HasOrient())
	{
		last.m_Mode    = tn.m_Mode;
		last.m_Target  = tn.m_Target;
		last.m_TargPos = tn.m_TargPos;
		last.m_TargRot = tn.m_TargRot;
	}

	return true;
}

//-------------------------------------------------------------------------
bool Sequencer::CheckForTriggerCollisions(Go* mover, TimeNodeIter curr)
{
	// Scan from the curr node to the next, looking for positions/times that we 
	// intersect triggers. If we find an intesection, add a new timenode at thay position

	if (curr == m_NodeList.end() || (m_LockedMovementGo != GOID_INVALID) )
	{
		return false;
	}

	TimeNodeIter next = curr;
	++next;
	
	if (next == m_NodeList.end())
	{
		return false;
	}

	bool ret = false;

	if (
		!IsZero((*curr).m_Dist,0.001f)					// Are we moving at this timenode?
		&& !(*curr).HasSignal() && !(*next).HasSignal()	// Have we already checked this timenode?
		)
	{
		gpassert(!(*curr).HasTransmitted());

		bool teleporting = IsNegative((*curr).m_Dist);

		if (teleporting)
		{
			(*curr).m_Dist = 0;
		}

		trigger::SignalSet hitlist;

		// expected_ratio, hittrigger, entering, condid
		if (gTriggerSys.CollectTriggerCollisions( mover,
			(*curr).m_WayPos,
			(*curr).m_Time, 
			(*next).m_WayPos,
			(*next).m_Time,
			hitlist ))
		{

			siege::eLogicalNodeFlags walkPerms = mover->GetBody()->GetTerrainMovementPermissions();

			trigger::SignalIter end = hitlist.end();
			trigger::SignalIter it = hitlist.begin();

			double curr_time = (*curr).m_Time;
			vector_3 init_delta = (*curr).m_Delta;
			vector_3 curr_delta = init_delta;
				
			SiegePos ip = (*curr).m_WayPos;

			for ( ; it != end; ++it )
			{
				// process the hit list
				// squeeze all hits forward if the character
				// can't stand at a trigger intersection point (not walkable)
				double new_time = max_t( curr_time , (*it).m_Time );

				if (new_time == curr_time)
				{		
					TimeNode tn;					
					tn.SetReqBlock((*curr).m_ReqBlock);				
					tn.SetTime((*curr).m_Time);			
					tn.SetPosition((*curr).m_Dist,(*curr).m_Delta,(*curr).m_WayPos);
					tn.SetSignal((*it).m_TriggerID,(*it).m_CrossType,(*it).m_ConditionID,(*it).m_SequenceNum);			
					
					(*curr).m_Dist = 0;
					(*curr).m_Delta = vector_3::ZERO; 

					curr = m_NodeList.insert(next,tn);		
				}
				else
				{	
					curr_time = new_time;

					double time_diff = PreciseSubtract((*next).m_Time, (*curr).m_Time);
					if( IsZero( (float)time_diff ) )
					{
						gperror( "time_diff is zero! If you are in the debugger DO NOT IGNORE THIS ERROR report it to jess or mike" );
					}

					float curr_ratio = (float)((PreciseSubtract(new_time,(*curr).m_Time))/time_diff);					

					if (curr_ratio<0.01f)
					{
						curr_ratio = 0.0f;
					}
					else if( curr_ratio >= 0.01f && curr_ratio <= 0.99f)
					{
						// Stretch the intersection every so slightly past the boundary
						// so that we don't insert a waypoint that is right on the boundary
						curr_ratio += (1.0f-curr_ratio)*0.1f;

						if (curr_ratio > 0.99f)
						{
							//The streched waypoint is on the bloody boundary!
							curr_ratio = 1.0f;
						}
						else
						{
							// Skim off a little bit of time
							curr_time = PreciseAdd( (*curr).m_Time, (curr_ratio*time_diff) );
							ip = (*curr).m_WayPos;
							ip.pos += (*curr).m_Delta * curr_ratio;

							float ratio_inc = (1.0f-curr_ratio)/4.0f;

							// Allow 4 attempts to find a valid position for the trigger intersection
							while (!gSiegeEngine.IsPositionAllowed(ip,walkPerms,0.5f))
							{
								curr_ratio += ratio_inc;
								
								if (curr_ratio > 0.99f)
								{
									//The next valid position is the bloody waypoint!
									curr_ratio = 1.0f;
									break;
								}

								curr_time = PreciseAdd( (*curr).m_Time, (curr_ratio*time_diff) );
								ip = (*curr).m_WayPos;
								ip.pos += (*curr).m_Delta * curr_ratio;

							}
						}
					}
					else //if (curr_ratio>0.99f)
					{
						//The damn waypoint is on the bloody boundary!
						curr_ratio = 1.0f;
					}


					if (curr_ratio == 1.0f)
					{		
						TimeNode tn;					
						tn.SetReqBlock((*curr).m_ReqBlock);				
						tn.SetTime((*next).m_Time);			
						tn.SetPosition(0,vector_3::ZERO,(*next).m_WayPos);
						tn.SetSignal((*it).m_TriggerID,(*it).m_CrossType,(*it).m_ConditionID,(*it).m_SequenceNum);								
						curr = m_NodeList.insert(next,tn);		
					}
					else if (curr_ratio == 0.0f)
					{		
						TimeNode tn;					
						tn.SetReqBlock((*curr).m_ReqBlock);				
						tn.SetTime((*curr).m_Time);			
						tn.SetPosition((*curr).m_Dist,(*curr).m_Delta,(*curr).m_WayPos);
						tn.SetSignal((*it).m_TriggerID,(*it).m_CrossType,(*it).m_ConditionID,(*it).m_SequenceNum);			
						
						(*curr).m_Dist = 0;
						(*curr).m_Delta = vector_3::ZERO;

						curr = m_NodeList.insert(next,tn);		
					}
					else
					{
						vector_3 diff = gSiegeEngine.GetDifferenceVector((*curr).m_WayPos,ip);
						(*curr).m_Dist = diff.Length();
						(*curr).m_Delta = diff;

						diff = gSiegeEngine.GetDifferenceVector(ip,(*next).m_WayPos);
						
						TimeNode tn;					
						tn.SetReqBlock((*curr).m_ReqBlock);				
						tn.SetTime(curr_time);			
						tn.SetPosition(diff.Length(),diff,ip);
						tn.SetSignal((*it).m_TriggerID,(*it).m_CrossType,(*it).m_ConditionID,(*it).m_SequenceNum);			

						curr = m_NodeList.insert(next,tn);		
					}
				}
			}
			Validate( mover, m_NodeList.back().m_Time );
		}		
	}

	return ret;

}

//-------------------------------------------------------------------------
void Sequencer::AppendTriggerSignals(const trigger::SignalSet& sigset)
{
	gpassert(!m_NodeList.empty());
	if (m_NodeList.empty())
	{
		return;
	}

	Validate( NULL, m_NodeList.back().m_Time );

	trigger::SignalConstIter sit = sigset.begin();

	// we need to add this signal at the last node that we have to keep
	// this will insert signals such as bcross_force_enter onto the next
	// possible waypoint!
	double earliestClipTime = gWorldTime.GetTime();
	bool mustLock;
	bool lastChoreInvalid;
	TimeNodeIter lastNodeKeep = FindLastNodeToKeep(earliestClipTime, mustLock, lastChoreInvalid, true );

	// we are at the last node in the list, we are keeping them all!
	if (lastNodeKeep == m_NodeList.end() )
	{
		--lastNodeKeep;
	}
	gpassert( (m_NodeList.size() == 1 )|| (lastNodeKeep != m_NodeList.begin()) );

	bool must_lock = false;

	// The node that follows the keeper becomes the insertion point if we have to
	// add any new signals in
	
	TimeNodeIter firstNodeToDrop = lastNodeKeep;
	++firstNodeToDrop;

	TimeNode tn;

	tn.SetTime((*lastNodeKeep).m_Time);			
	tn.SetPosition(0,vector_3::ZERO,(*lastNodeKeep).m_WayPos);
	tn.SetReqBlock((*lastNodeKeep).m_ReqBlock);				
	
	for (; sit != sigset.end(); ++sit)
	{
		must_lock |= trigger::IsForcedCrossing((*sit).m_CrossType);

		if ((*lastNodeKeep).HasSignal() || (*lastNodeKeep).HasTransmitted())
		{

			tn.SetSignal((*sit).m_TriggerID,(*sit).m_CrossType,(*sit).m_ConditionID,(*sit).m_SequenceNum);

			lastNodeKeep = m_NodeList.insert(firstNodeToDrop, tn);
		}
		else
		{
			// It is safe to just tack it onto the last node that we are going to keep
			(*lastNodeKeep).SetSignal((*sit).m_TriggerID,(*sit).m_CrossType,(*sit).m_ConditionID,(*sit).m_SequenceNum);
		}
		
	}
	
	if (must_lock)
	{
		// Lock ALL the nodes leading up to the forced crossing
		TimeNodeIter i = m_NodeList.begin();
		for (;i!=firstNodeToDrop; ++i)
		{
			(*i).SetLockedIn();
		}
	}

	Validate( NULL, m_NodeList.back().m_Time );
}

//-------------------------------------------------------------------------
bool Sequencer::CollectTimeNodesToSend( Go* mover, double forward_time, std::vector<TimeNode> &out_ToSend )
{

	if ( m_NodeList.empty() ) return false;

	TimeNodeIter beg = m_NodeList.begin();
	TimeNodeIter end = m_NodeList.end();

	// See if we have transmitted the second-to-last node already
	bool SecondToLastTransmitted = false;

	TimeNodeIter second_to_last = end;
	--second_to_last;

	if (second_to_last != beg) 
	{
		--second_to_last;
		SecondToLastTransmitted = (*second_to_last).HasTransmitted();
	}
	else
	{
		SecondToLastTransmitted = true;
	}

	TimeNodeIter left;
	TimeNodeIter mid;
	TimeNodeIter right;
	TimeNodeIter next;

	left = beg;

	// Keep track of whether or not we need to lock the next
	// node because it is used in a delayed signal
	bool lock_next = false;
	siege::database_guid locked_node = siege::UNDEFINED_GUID;

	#define MAINTAIN_FLAGS(n)									\
		n.SetTransmitted();										\
		if (n.HasChore())										\
		{														\
			m_LastChoreTransmitted = n.m_Chore;					\
			m_LastStanceTransmitted = n.m_Stance;				\
			m_LastSubAnimTransmitted = n.m_SubAnim;				\
			m_LastFlagsTransmitted = n.m_Flags;					\
		}														\
		if ( n.HasPosition() &&									\
			(m_WayPosNodeTransmitted != n.m_WayPos) )			\
		{														\
			m_WayPosNodeTransmitted = n.m_WayPos;				\
		}														\
		if (lock_next)											\
		{														\
			lock_next = (locked_node == n.m_WayPos.node);		\
			n.SetLockedIn();									\
		}														\
		lock_next |= n.HasSignal()								\
			 && trigger::IsDelayedCrossing(n.m_CrossType);		\
		if (lock_next)											\
		{														\
			locked_node = n.m_WayPos.node;						\
		}														\
		m_LastTimeSent = n.m_Time;								\
		out_ToSend.push_back(n);									


	// Make sure we have sent the first three nodes

	if ((left != end) && !(*left).HasTransmitted() )
	{
		CheckForTriggerCollisions(mover,left);
		MAINTAIN_FLAGS((*left));
	}

	mid = left;
	if (mid != end) ++mid;
	if ((mid != end) && !(*mid).HasTransmitted() )
	{
		CheckForTriggerCollisions(mover,mid);
		MAINTAIN_FLAGS((*mid));
	}

	right = mid;				
	if (right != end) ++right;	
	if (right != end)
	{
		if (!(*right).HasTransmitted() )
		{
			CheckForTriggerCollisions(mover,right);
			MAINTAIN_FLAGS((*right));
		}
		else
		{
			lock_next = (*right).HasSignal() && trigger::IsDelayedCrossing((*right).m_CrossType);
			locked_node = (*right).m_WayPos.node;
		}
	}
	else
	{
		lock_next = true;
	}


	// Peek ahead to the time of the next node in the list we need to send, 
	// While that time is greater than the forward_time
	// advance the front and send the 'next node to send'

	if (right == end) return true;

	// Send the next waypoint if we are 1/4 of the way there
	bool send_next = false;
	if (!lock_next)
	{
		double dift = PreciseSubtract((*right).m_Time,(*mid).m_Time);
		if (dift<0.01)
		{
			send_next = true;
		}
		else
		{
			double cutt = PreciseAdd((*mid).m_Time, PreciseMultiply(0.25,dift));
			send_next = forward_time > cutt;
		}
	}

	TimeNodeIter it = right;
	++it;

	while ( (it != end) && (lock_next || send_next) )
	{
		// Advance to the next interval
		if (!(*it).HasTransmitted())
		{
			CheckForTriggerCollisions(mover,it);
			MAINTAIN_FLAGS((*it));
		}

		if (!lock_next)
		{
			double dift = PreciseSubtract((*it).m_Time,(*right).m_Time);
			if (dift<0.01)
			{
				send_next = true;
			}
			else
			{
				double cutt = PreciseAdd((*right).m_Time, PreciseMultiply(0.25,dift));
				send_next = forward_time > cutt;
			}
		}

		left = mid;
		mid = right;
		right = it;

		++it;

	}

	// Remove everything up to the left node
	m_NodeList.erase(beg,left);

#if !GP_RETAIL
	int post_size = m_NodeList.size();
	if ( post_size >= 2 )
	{
		int d = std::distance(m_NodeList.begin(),right);
		gpassert(d==2);
	}
#endif
	
	// Return true if the second-to-last node was just transmitted
	return !SecondToLastTransmitted && (*second_to_last).HasTransmitted();
}

//-------------------------------------------------------------------------
void Sequencer::PackRefresh( FuBi::BitPacker &inout_packer, GoFollower *follower, double ClientSynchTime )
{

	inout_packer.XferRaw( ClientSynchTime );

	// Current Position
	SiegePos cp;
	
	if (m_LockedMovementGo != GOID_INVALID)
	{
		Go* lockgo = GetGo(m_LockedMovementGo);
		if ( lockgo && lockgo->HasFollower() )
		{
			lockgo->GetFollower()->GetPositionAtTime( DBL_MAX, cp );
		}
		else
		{
			follower->GetPositionAtTime( DBL_MAX, cp );
		}
	}
	else
	{
		follower->GetPositionAtTime( DBL_MAX, cp );
	}

	inout_packer.XferFloat( cp.pos.x );
	inout_packer.XferFloat( cp.pos.y );
	inout_packer.XferFloat( cp.pos.z );
	inout_packer.XferRaw  ( cp.node  );	

	// Current Rotation

	SiegeRot sr;
	
	follower->GetOrientationAtTime( DBL_MAX, sr);
	if (cp.node != sr.node)
	{
		// Make sure that the rotation is relative to the same node as the position
		sr.rot = NodeRotationDifferenceQuat( sr.node , cp.node ) * sr.rot;
		sr.node = cp.node;
	}

	
	inout_packer.XferFloat( sr.rot.m_x  );
	inout_packer.XferFloat( sr.rot.m_y  );
	inout_packer.XferFloat( sr.rot.m_z  );
	inout_packer.XferFloat( sr.rot.m_w  );
	inout_packer.XferRaw  ( sr.node );	

	// Chore 

	eAnimChore  ac;
	eAnimStance as;
	DWORD		sa;
	DWORD		fl;

	// The follower only has a couple of way points, so by sending a
	// a huge value we are sure to get the last entry

	follower->GetChoreParamsAtTime(DBL_MAX,ac,as,sa,fl);

	m_LastChoreTransmitted = ac;
	m_LastStanceTransmitted = as;
	m_LastSubAnimTransmitted = sa;
	m_LastFlagsTransmitted = fl;
	
	nema::Aspect* pAsp = follower->GetGo()->GetAspect()->GetAspectPtr();

	if (pAsp->HasBlender())
	{
		float dctw = pAsp->GetBlender()->DurationOfCurrentTimeWarp();
		float elapsed = IsZero(dctw) ? 0.0f : ((pAsp->GetBlender()->TimeOfCurrentTimeWarp()+gWorldOptions.GetLagMCP())/ dctw);
		elapsed = min_t(elapsed,1.0f);
		if ((ac != CHORE_ATTACK) && (ac != CHORE_MAGIC))
		{
			// Attack&Magic chores have special flags so we leave them alone
			// They will synch up properly on the very next cycle
			EncodeChoreParameters( elapsed, fl);
		}
	}

	inout_packer.XferRaw( ac,  FUBI_MAX_ENUM_BITS( CHORE_ ));
	
	DWORD val;
	val = as-AS_BEGIN;
	inout_packer.XferRaw( val, FuBi::GetMaxEnumBits(0,AS_END-AS_BEGIN) );

	inout_packer.XferCount( sa, 1 );
	inout_packer.XferRaw  ( fl );

	// Orient

	eOrientMode om = follower->GetCurrentOrientMode();

	inout_packer.XferRaw( om,  FUBI_MAX_ENUM_BITS( ORIENTMODE_ ) );

	if (( om == ORIENTMODE_FIXED_POSITION ) || ( om == ORIENTMODE_FIXED_POSITION_REVERSE ) )
	{
		SiegePos tp = follower->GetCurrentOrientTargPos();

		inout_packer.XferFloat( tp.pos.x );
		inout_packer.XferFloat( tp.pos.y );
		inout_packer.XferFloat( tp.pos.z );
		inout_packer.XferRaw  ( tp.node  );	
	}
	else if (( om == ORIENTMODE_FIXED_DIRECTION ) || ( om == ORIENTMODE_FIXED_POSITION_REVERSE ) ) 
	{
		SiegeRot tr = follower->GetCurrentOrientationGoal();

		inout_packer.XferFloat( tr.rot.m_x    );
		inout_packer.XferFloat( tr.rot.m_y    );
		inout_packer.XferFloat( tr.rot.m_z    );
		inout_packer.XferFloat( tr.rot.m_w    );
		inout_packer.XferRaw  ( tr.node );	
	}
	else if (( om == ORIENTMODE_TRACK_OBJECT ) || ( om == ORIENTMODE_TRACK_OBJECT_REVERSE ) ) 
	{
		Goid tg = follower->GetCurrentOrientTarget();
		inout_packer.XferRaw( tg );	
	}

	inout_packer.XferRaw( m_LockedMovementGo );	

};


//-------------------------------------------------------------------------
eReqRetCode Sequencer::EstimateInterceptPosition(
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

	if (Empty() || (m_LockedMovementGo != GOID_INVALID) )
	{
		contact_pos = SiegePos::INVALID;
		contact_time = DBL_MAX;
		return REQUEST_FAILED;
	}

	double travel_time = DBL_MAX;

	double max_arrival_time = PreciseAdd(initial_time, (max_t(0.0f,look_ahead)+max_t(0.0f,cushion)));
	
	TimeNodeIter end = m_NodeList.end();

	TimeNodeIter first = FindNodeAtOrAfterTime(initial_time);
	TimeNodeIter last = FindNodeAtOrAfterTime(max_arrival_time);

	if (last == end)
	{ 
		--last; // We know the list isn't empty, the lookahead may exceed the end-of-list
	}

	TimeNodeIter searchit;
	TimeNodeIter contactit = m_NodeList.end();

	const float inv_vel = 1.0f/velocity;
	
	if (first == end)
	{	
		const SiegePos& targ_nextp = (*last).m_WayPos;
		
		float dist = gSiegeEngine.GetDifferenceVector(src_pos,targ_nextp).Length();
		contactit = last;
		travel_time = (float)( PreciseAdd(initial_time, (dist * inv_vel)) );
	}
	else
	{

		for (searchit = first ; searchit != end ; ++searchit)
		{
			
			const SiegePos& targ_nextp = (*searchit).m_WayPos;

			if (!gSiegeEngine.IsNodeInAnyFrustum(targ_nextp.node))
			{
				if (contactit != end)
				{
					contact_pos = (*contactit).m_WayPos;
					contact_time = (*contactit).m_Time;
					return REQUEST_OK_BEYOND_RANGE;
				}
				else
				{
					contact_pos = SiegePos::INVALID;
					contact_time = DBL_MAX;
					return REQUEST_FAILED_PATH_LEAVES_WORLD;
				}
			}
			else
			{
				// Applying a fudge factor to addressing swinging at the air 
				// reported in bug #13352 
				static float ANTI_WIFFING_FUDGE_FACTOR = 0.6f;

				float adjusted_separation = ANTI_WIFFING_FUDGE_FACTOR * gSiegeEngine.GetDifferenceVector(src_pos,targ_nextp).Length();

				float dist = max_t(0.0f,(adjusted_separation-range));

				double intercept_time = PreciseAdd( initial_time, (dist * inv_vel) );

				float diff = (float)(PreciseSubtract( intercept_time, (*searchit).m_Time));

				if ((intercept_time <= max_arrival_time) &&
					!IsPositive(diff,0.01f))
				{
					contactit = searchit;
					travel_time = intercept_time;
					break;
				}
			}

		}
	}

	if (contactit != end)
	{
		contact_pos = (*contactit).m_WayPos;
		contact_time = (*contactit).m_Time;
	}
	else
	{
		contact_pos = (*last).m_WayPos;
		contact_time = (*last).m_Time;
	}

	if (contact_time <= max_arrival_time)
	{
		return REQUEST_OK;
	}
	else
	{
		return REQUEST_OK_BEYOND_RANGE;
	}

}

//-------------------------------------------------------------------------
double Sequencer::GetEarliestClipOrTransmittedTime(const double& inout_ClipTime)
{
	// return the time of the last locked-in or transmitted node
	double probetime = inout_ClipTime;
	bool dummy1,dummy2;
	FindLastNodeToKeep(probetime,dummy1,dummy2,true);
	return probetime;
}


//-------------------------------------------------------------------------
double Sequencer::GetEarliestClipTime(const double& inout_ClipTime)
{
	// return the time of the last locked-in node
	double probetime = inout_ClipTime;
	bool dummy1,dummy2;
	FindLastNodeToKeep(probetime,dummy1,dummy2,false);
	return probetime;
}

//-------------------------------------------------------------------------
TimeNodeIter Sequencer::FindLastNodeToKeep(double& inout_ClipTime, bool &out_MustLock, bool &out_LastChoreInvalid, bool notTransmitted)
{
	// This is a utility function I added to clean up the ClipAfterTime function
	// We need to located the first node that is 'safe' to clip AFTER the time 
	// A node isn't considered clippable if is alrady locked or if its
	// part of a delayed boundary crossing.
	// Node that are very close (in time) to a locked node are also locked
	
	double keep_t = inout_ClipTime;

	out_MustLock = false;
	out_LastChoreInvalid = false;

	if (!m_NodeList.empty())
	{

		TimeNodeIter beg = m_NodeList.begin();
		TimeNodeIter end = m_NodeList.end();
		TimeNodeIter keep = end;

		// Find the last node that is either a) less than clip time, or b) locked or c) has a signal

		--keep;
		do {
			if ((*keep).HasLockedIn())
			{
				keep_t = (*keep).m_Time;
				break;
			}
			if ((*keep).HasSignal() && (*keep).HasTransmitted())
			{
				out_MustLock = true;
				keep_t = (*keep).m_Time;
				break;
			}
			if (!IsNegative((float)(PreciseSubtract(keep_t,(*keep).m_Time))))
			{
				break;
			}
			if ((*keep).HasChore() && (*keep).HasTransmitted() )
			{
				// We aren't keeping the node that transmitted this chore anymore
				out_LastChoreInvalid = true;
			}
			if ((*keep).HasTransmitted() && notTransmitted )
			{
				keep_t = (*keep).m_Time;
				break;
			}

			--keep;
		} while ( keep != beg );

		if (((*keep).m_Time) > keep_t)
		{
			keep_t = (*keep).m_Time;
		}
		
		bool check_adjacent = true;

		// If we have to, lock each TimeNode that defines the end of a delayed crossing that starts at the keeper
		// ...and make sure that all nodes that are within 1/100th second of the keeper are kept too.
		while ((keep != end) && check_adjacent)
		{
			if ((*keep).HasSignal() && (*keep).HasTransmitted() && trigger::IsDelayedCrossing((*keep).m_CrossType))
			{
				keep_t = (*keep).m_Time;
				siege::database_guid previous_siegenode = (*keep).m_WayPos.node;
				++keep;

				// Search until we find a change to a different siegenode
				while (keep != end) 
				{
					keep_t = (*keep).m_Time;
					if (previous_siegenode != (*keep).m_WayPos.node)
					{
						// We have changed nodes
						break;
					}
					++keep;
				}
				check_adjacent = true;
			}

			// Now 'KEEP' all TimeNodes that are within 1/100th second of the current keeper
			if ((keep != end) && check_adjacent)
			{
				check_adjacent = false;
				TimeNodeIter clip = keep;
				++clip;
				while ( (clip != end) && IsZero((float)(PreciseSubtract(keep_t,(*clip).m_Time)),0.01f))
				{
					keep = clip;
					keep_t = (*keep).m_Time;
					++clip;
				}
			}

			// ...and repeat the test for delayed crossings until they are all accounted for
		}

		inout_ClipTime = keep_t;
		return keep;
	}
	return m_NodeList.end();
}


//-------------------------------------------------------------------------
bool Sequencer::ClipToTime(double& inout_AdjustedClipTime,siege::eLogicalNodeFlags perms, Go* owngo)
{

#if !GP_RETAIL
	double requested_time = inout_AdjustedClipTime;
#endif

	if (m_NodeList.empty())
	{
		return false;
	}

	bool must_lock,invalidate_chore;
	TimeNodeIter keep = FindLastNodeToKeep(inout_AdjustedClipTime,must_lock,invalidate_chore,false);

	bool last_chore_is_not_valid = invalidate_chore;

	// Make sure that we mark the timenodes that we have to keep
	// if we are preserving nodes because they contain a signal
	if (must_lock || invalidate_chore)
	{
		TimeNodeIter it = m_NodeList.begin();
		while (it != keep)
		{
			if (must_lock)
			{
				(*it).SetLockedIn();
			}
			if (invalidate_chore)
			{
				if ((*it).HasChore() && (*it).HasTransmitted() )
				{
					m_LastChoreTransmitted = (*it).m_Chore;
					m_LastStanceTransmitted = (*it).m_Stance;
					m_LastSubAnimTransmitted = (*it).m_SubAnim,
					m_LastFlagsTransmitted = (*it).m_Flags,
					last_chore_is_not_valid = false;
				}
			}

			++it;
		}
		if (must_lock)
		{
			(*keep).SetLockedIn();
		}
		if (invalidate_chore)
		{
			if ((*keep).HasChore() && (*keep).HasTransmitted() )
			{
				m_LastChoreTransmitted = (*it).m_Chore;
				m_LastStanceTransmitted = (*it).m_Stance;
				m_LastSubAnimTransmitted = (*it).m_SubAnim,
				m_LastFlagsTransmitted = (*it).m_Flags,
				last_chore_is_not_valid = false;
			}
		}

	}

	if (last_chore_is_not_valid)
	{
		if (owngo->HasFollower())
		{
			owngo->GetFollower()->GetChoreParamsAtTime(
				DBL_MAX,
				m_LastChoreTransmitted,
				m_LastStanceTransmitted,
				m_LastSubAnimTransmitted,
				m_LastFlagsTransmitted
				);
		}
	}

	
	if (keep == m_NodeList.end())
	{
		gpdevreportf( &gTriggerSysContext,("TRIGGERSYS: MCP Sequencer LOCKED_QUEUE unable to flush anything for [%s g:%08x s:%08x]\n",
			(owngo) ? owngo->GetTemplateName() : "<UNKNOWN>",
			(owngo) ? MakeInt(owngo->GetGoid()) : -1,
			(owngo) ? MakeInt(owngo->GetScid()) : -1
			));

		// Need to keep the entire list
		TimeNodeIter it = m_NodeList.begin();
		do 
		{
			(*it).SetLockedIn();
			++it;
		} while (it != keep);
		--keep;
		inout_AdjustedClipTime = (*keep).m_Time;
		return false;
	}

	TimeNodeIter clip = keep;
	clip++;

	// We can interpolate only if the first unclippable is LESS than the required time
	bool interp = (PreciseAdd( (*keep).m_Time, 0.01 ))<inout_AdjustedClipTime;

	SiegePos clip_pos;
	bool clipped = false;

	if (interp)
	{
		if (clip != m_NodeList.end())
		{
			// Search for a valid position to clip to in the interval
			double increment = PreciseSubtract((*clip).m_Time, inout_AdjustedClipTime);

			if (increment>0.0)
			{
				DWORD steps = 4;
				increment = PreciseMultiply( increment, (1.0f/steps) );

				double clip_time = inout_AdjustedClipTime;
				
				for ( DWORD i = 0; (i < steps) && !clipped; ++i)
				{
					clipped = GetPosition(clip_time, false, false, clip_pos);
					if (clipped)
					{
						const float MAX_HEIGHT_DIFFERENCE = 0.5f;
						if (gSiegeEngine.IsPositionAllowed( clip_pos, perms, MAX_HEIGHT_DIFFERENCE ))
						{
							inout_AdjustedClipTime = clip_time;
						}
						else
						{
							clip_time = PreciseAdd( clip_time, increment );
							clipped = false;
						}
					}
				}
			}

			if (!clipped)
			{
				keep = clip;
				inout_AdjustedClipTime = (*keep).m_Time;
				++clip;
			}
		}
	}
	else
	{
#if !GP_RETAIL
		if  ((*keep).m_Time>=requested_time)
		{

			TimeNodeIter counter = keep;
			int count = 1;
			while ((*counter).m_Time >= requested_time)
			{
				if (counter == m_NodeList.begin())
				{
					break;
				}
				count++;
				--counter;
			}
			gpdevreportf( &gTriggerSysContext,("TRIGGERSYS: ClipToTime(%f) resulted in a cliptime of %f\n"
				"\tMCP Sequencer has %d LOCKED TIMENODES, unable to flush everything for [%s g:%08x s:%08x]\n",
				requested_time,
				inout_AdjustedClipTime,
				count,
				(owngo) ? owngo->GetTemplateName() : "<UNKNOWN>",
				(owngo) ? MakeInt(owngo->GetGoid()) : -1,
				(owngo) ? MakeInt(owngo->GetScid()) : -1
				));


			for (;;counter++) 
			{
				gpdevreportf( &gTriggerSysContext,(
					"\t\tKeeping TimeNode: %f [g:%08x:%04x:%04x] %d [%s] [%s]\n",
					(*counter).m_Time,
					(*counter).HasSignal() ? (*counter).m_Trigger.GetOwnGoid() : GOID_INVALID,
					(*counter).HasSignal() ? (*counter).m_Trigger.GetTrigNum() : -1,
					(*counter).HasSignal() ? (*counter).m_CondID : -1,
					(*counter).HasSignal() ? (*counter).m_SeqNum : -1,
					(*counter).HasSignal() ? ToString((*counter).m_CrossType) : "NO SIGNAL",
					(*counter).HasTransmitted() ? "TRANSMITTED" : "NOT TRANSMITTED"
					));
				if (counter == keep) 
				{
					break;
				}
			}
		}	
#endif

	}

	bool ret;

	// Are we removing something we have transmitted?
	if (clip != m_NodeList.end())
	{
		ret = (*clip).HasTransmitted();
		for (TimeNodeIter tt = clip;tt!=m_NodeList.end();)
		{
			if ((*tt).HasSignal())
			{
				gpdevreportf( &gTriggerSysContext,(
					"TRIGGERSYS: MCP Sequencer has CANCELLED A SIGNAL from [%s g:%08x s:%08x]\n"
					"\t\tCancelled Signal: %f [g:%08x:%04x:%04x] [%d] [%s] [%s]\n",
					(owngo) ? owngo->GetTemplateName() : "<UNKNOWN>",
					(owngo) ? MakeInt(owngo->GetGoid()) : -1,
					(owngo) ? MakeInt(owngo->GetScid()) : -1,
					(*tt).m_Time,
					(*tt).m_Trigger.GetOwnGoid(),
					(*tt).m_Trigger.GetTrigNum(),
					(*tt).m_CondID,
					(*tt).m_SeqNum,
					ToString((*tt).m_CrossType),
					(*tt).HasTransmitted() ? "TRANSMITTED" : "NOT TRANSMITTED"
					));
				
				trigger::BoundaryCrossing bc(	(*tt).m_Time,
												(*tt).m_Trigger,
												MakeInt(owngo->GetGoid()),
												(*tt).m_CrossType,
												(*tt).m_CondID,
												(*tt).m_SeqNum);
				gTriggerSys.CancelSignal(bc);
			}
			tt=m_NodeList.erase(tt);
		}
	}
	else
	{
		ret = false;
	}


	if (clipped)
	{
		vector_3 diff = gSiegeEngine.GetDifferenceVector((*keep).m_WayPos,clip_pos);

		(*keep).SetPosition(diff.Length(),diff,(*keep).m_WayPos);

		TimeNode tn;
		tn.SetReqBlock((*keep).m_ReqBlock);
		tn.SetTime(inout_AdjustedClipTime);
		tn.SetPosition(0,vector_3::ZERO,clip_pos);

		m_NodeList.push_back(tn);

	}
	return ret;

}

//-------------------------------------------------------------------------
bool Sequencer::UpdateAfterClipIsSent(double clip_time)
{
	m_LastChoreTransmitted = CHORE_NONE; 
	m_LastStanceTransmitted = AS_DEFAULT;
	m_LastSubAnimTransmitted = 0;
	m_LastFlagsTransmitted = 0;

	m_LastTimeSent = clip_time;

	return true;
}

//-------------------------------------------------------------------------
bool Sequencer::VerifyPositionAtTime(const SiegePos& p0,double t0)
{
	TimeNodeIter end = m_NodeList.end();
	TimeNodeIter it = FindNodeAtOrBeforeTime(t0);

	if (it == end)
	{
		return false;
	}

	const SiegePos& p1 = (*it).m_WayPos;
	const double& t1 = (*it).m_Time;

	if ((t0 == t1))
	{
		return IsEqual(p0,p1,0.001f);
	}
	else if (t1 < t0)
	{
		// We are at the end of the node list, check to see if the
		// last position is the one we are looking for
		++it;
		if (it == end)
		{
			return IsEqual(p0,p1,0.001f);
		}
	}
	return (false);
}

#if !GP_RETAIL	

//-------------------------------------------------------------------------
void Sequencer::Validate(Go* own,double appendtime) const
{
	if (m_NodeList.empty()) return;

	TimeNodeConstIter beg = m_NodeList.begin();
	TimeNodeConstIter end = m_NodeList.end();
	TimeNodeConstIter it;

	gpassertf(( (beg==end) || (appendtime >= m_NodeList.back().m_Time) ),
		("MCP SEQUENCER ERROR: [%s:0x%08x] AppendTime less than last waypoint time (ap_t:%f < lw_t:%f)",
		own ? own->GetTemplateName() : "UNKNOWN",
		own ? MakeInt(own->GetGoid()) : -1,
		appendtime,
		m_NodeList.back().m_Time
		));

	if( appendtime > gWorldTime.GetTime() + 5000 )
	{
		gperrorf(("MCP SEQUENCER ERROR: [%s:0x%08x] Current appendtime %f occurs %f seconds in the future",
			own ? own->GetTemplateName() : "UNKNOWN",
			own ? MakeInt(own->GetGoid()) : -1,
			appendtime,
			appendtime - gWorldTime.GetTime()
			));
	}

	double prevtime = 0;
	for ( it = beg; it != end ; ++it )
	{ 
		if ((*it).HasOrient())
		{
			gpassertf(( ((*it).m_Mode >= ORIENTMODE_BEGIN) && ((*it).m_Mode < ORIENTMODE_END) ),
				("MCP SEQUENCER ERROR: [%s:0x%08x] Waypoint #%d has invalid orient mode %d",
				own ? own->GetTemplateName() : "UNKNOWN",
				own ? MakeInt(own->GetGoid()) : -1,
				std::distance(beg,it),
				(*it).m_Mode
				));
		}
		gpassertf(((*it).m_WayPos.node.GetValue() != 0xcdcdcdcd),
			("MCP SEQUENCER ERROR: [%s:0x%08x] Waypoint #%d is mangled 0x%08x",
			own ? own->GetTemplateName() : "UNKNOWN",
			own ? MakeInt(own->GetGoid()) : -1,
			std::distance(beg,it),
			(*it).m_WayPos.node.GetValue()
			));
		gpassertf(!IsPositive((float)(prevtime-(*it).m_Time),0.001f),
			("MCP SEQUENCER ERROR: [%s:0x%08x] Time at waypoint #%d is not increasing %f > %f",
			own ? own->GetTemplateName() : "UNKNOWN",
			own ? MakeInt(own->GetGoid()) : -1,
			std::distance(beg,it),
			prevtime,
			(*it).m_Time
			));
		gpassertf(!IsPositive((float)((*it).m_Time - appendtime),0.001f),
			("MCP SEQUENCER ERROR: [%s:0x%08x] Time at waypoint #%d exceeds appendtime %f > %f",
			own ? own->GetTemplateName() : "UNKNOWN",
			own ? MakeInt(own->GetGoid()) : -1,
			std::distance(beg,it),
			(*it).m_Time,
			appendtime
			));
		prevtime = (*it).m_Time;
	}


}

//-------------------------------------------------------------------------
void Sequencer::DebugDraw(double current_time, float draw_width) const
{

	UNREFERENCED_PARAMETER(current_time);

	if (m_NodeList.empty()) return;

	TimeNodeConstIter beg = m_NodeList.begin();
	TimeNodeConstIter end = m_NodeList.end();
	TimeNodeConstIter it;

	std::vector<vector_3> verts;
	std::vector<vector_3> tag0verts;
	std::vector<vector_3> tag1verts;
	std::vector<vector_3> tag2verts;
	std::vector<vector_3> tag3verts;
	std::vector<vector_3> tag4verts;
	std::vector<vector_3> tag5verts;
	std::vector<vector_3> tag6verts;
	std::vector<vector_3> tag7verts;
	std::vector<vector_3> tag8verts;
	std::vector<vector_3> tag9verts;

	SiegePos origin;
	for ( it = beg; it != end ; ++it )
	{
		origin = (*it).m_WayPos;
		if (gSiegeEngine.IsNodeInAnyFrustum(origin.node)) 
		{
			break;
		}
	}

	if (it == end)
	{
		return;
	}

	siege::SiegeNodeHandle handle = gSiegeEngine.NodeCache().UseObject( origin.node );
	siege::SiegeNode& origin_node = handle.RequestObject( gSiegeEngine.NodeCache() );

	gDefaultRapi.PushWorldMatrix();
	gDefaultRapi.SetWorldMatrix(origin_node.GetCurrentOrientation(),origin_node.GetCurrentCenter());
	gDefaultRapi.TranslateWorldMatrix(origin.pos);

	verts.push_back(vector_3::ZERO);
	RP_DrawPolymarker( gDefaultRapi, verts, draw_width, 0xff00FF88, 3, 0);
	verts.clear();

	vector_3 P1;

	gpstring lblstr;
	SiegePos lblpos(SiegePos::INVALID);
	DWORD lblcolor(0xffffffff);

	for ( it = beg; it != end ; ++it )
	{

		if (!gSiegeEngine.IsNodeInAnyFrustum((*it).m_WayPos.node)) {
			continue;
		}

		if (gWorldOptions.GetLabelMCP())
		{
			if ((*it).m_Time < FLOAT_INFINITE)
			{
				if (lblpos == (*it).m_WayPos)
				{
					// Stack the labels
					lblstr.appendf("\n%8.3f",(*it).m_Time);
				}
				else
				{
					if (lblpos != SiegePos::INVALID)
					{
						// Draw the previous label before we start a new one
						lblpos.pos += vector_3(0,0.1f,0);
						gWorld.DrawDebugScreenLabelColor( lblpos, lblstr.c_str(),lblcolor);
					}
					// Start a new stack of labels
					lblpos = (*it).m_WayPos;
					lblstr.assignf("%8.3f",(*it).m_Time);
				}

				if ((*it).HasTransmitted())
				{
					lblstr.append("*");
				}

				if ((*it).HasChore())
				{
					unsigned char red,grn,blu;
					switch ((*it).m_Chore)
					{
						case CHORE_WALK:
						{
							red = 0x7f; grn = 0xff; blu = 0x7f;
							break;
						}
						case CHORE_ATTACK:
						{
							red = 0xff; grn = 0x3f; blu = 0x3f;
							break;
						}
						case CHORE_FIDGET:
						{
							red = 0xff; grn = 0xff; blu = 0x0f;
							break;
						}
						case CHORE_DIE:
						{
							red = 0x90; grn = 0x90; blu = 0x90;
							break;
						}
						case CHORE_MAGIC:
						{
							red = 0x18; grn = 0x18; blu = 0xa0;
							break;
						}
						default:
						{
							red = 0xff; grn = 0x7f; blu = 0x7f;
							break;
						} 
					}
					
					siege::SiegeLabel::AppendColorChangeEscapeSequence(lblstr,red,grn,blu);
				}
				else if ((*it).HasSignal())
				{
					unsigned char red,grn,blu;
					red = 0xa0;
					grn = 0x80;
					blu = 0xf0;
					siege::SiegeLabel::AppendColorChangeEscapeSequence(lblstr,red,grn,blu);
				}
				else if ((*it).HasOrient())
				{
					unsigned char red,grn,blu;
					red = 0x7f;
					grn = 0x7f;
					blu = 0xff;
					siege::SiegeLabel::AppendColorChangeEscapeSequence(lblstr,red,grn,blu);
				}
			}
		}

		P1 = gSiegeEngine.GetDifferenceVector(origin,(*it).m_WayPos);

		if ((*it).HasChore()) 
		{
			switch((*it).m_Chore)
			{
				case CHORE_WALK:
				{
					tag1verts.push_back(P1);
					break;
				}
				case CHORE_ATTACK:
				{
					tag2verts.push_back(P1);
					break;
				}
				case CHORE_FIDGET:
				{
					tag3verts.push_back(P1);
					break;
				}
				case CHORE_DIE:
				{
					tag4verts.push_back(P1);
					break;
				}
				case CHORE_MAGIC:
				{
					tag5verts.push_back(P1);
					break;
				}
				default:
				{
					tag6verts.push_back(P1);
					break;
				} 
			}
		}
		else if ((*it).HasOrient()) 
		{
			tag7verts.push_back(P1);
		}
		else
		{
			tag0verts.push_back(P1);
		}

		if ((*it).HasSignal()) 
		{
			tag8verts.push_back(P1);
		}
		if ((*it).HasLockedIn()) 
		{
			tag9verts.push_back(P1);
		}
		
		verts.push_back(P1);

	}

	if (lblpos != SiegePos::INVALID)
	{
		// Draw the last label
		lblpos.pos += vector_3(0,0.1f,0);
		gWorld.DrawDebugScreenLabelColor( lblpos, lblstr.c_str(), lblcolor);
	}

	// White circles & blue lines
	RP_DrawPolymarker( gDefaultRapi, verts, draw_width, 0xffFFFFFF, 50, 0);
	if (verts.size() > 1)
	{
		RP_DrawPolyline( gDefaultRapi, verts, 0xff0000FF);
		RP_DrawPolymarker( gDefaultRapi, tag0verts, draw_width*1.1f, 0xffff8000, 3, 0);
		RP_DrawPolymarker( gDefaultRapi, tag1verts, draw_width*1.1f, 0xff7fff7f, 4, 0);
		RP_DrawPolymarker( gDefaultRapi, tag2verts, draw_width*1.1f, 0xffff3f3f, 4, 0);
		RP_DrawPolymarker( gDefaultRapi, tag3verts, draw_width*1.1f, 0xffffff0f, 4, 0);
		RP_DrawPolymarker( gDefaultRapi, tag4verts, draw_width*1.1f, 0xff909090, 4, 0);
		RP_DrawPolymarker( gDefaultRapi, tag5verts, draw_width*1.1f, 0xff1818a0, 4, 0);
		RP_DrawPolymarker( gDefaultRapi, tag6verts, draw_width*1.1f, 0xffff7f7f, 4, 0);
		RP_DrawPolymarker( gDefaultRapi, tag7verts, draw_width*1.1f, 0xffff0080, 5, 0);

		// Draw the signallers
		RP_DrawPolymarker( gDefaultRapi, tag8verts, draw_width*1.2f, 0xffa080f0, 5, 0);

		// Draw the locked in
		RP_DrawPolymarker( gDefaultRapi, tag9verts, draw_width*1.3f,  0xffff8000, 6, 0);
		RP_DrawPolymarker( gDefaultRapi, tag9verts, draw_width*1.35f, 0xffffffff, 6, 0);
		RP_DrawPolymarker( gDefaultRapi, tag9verts, draw_width*1.4f,  0xffff8000, 6, 0);
	}


	gDefaultRapi.PopWorldMatrix();

}

//-------------------------------------------------------------------------
void Sequencer::Dump( ReportSys::ContextRef ctx ) const
{

	int count = 0;

	for( TimeNodeConstIter it = m_NodeList.begin(); it != m_NodeList.end(); ++it )
	{

		if ((*it).HasTransmitted()) continue;

		if ((*it).m_Time != DBL_MAX)
		{
			ctx->OutputF( "Node [%d] Time: %8.3f",
						  count++,
						  (*it).m_Time
						  );
		}
		else
		{
			ctx->OutputF( "Node [%d] Time: <infinite>" , count++);
		}

		ctx->OutputF( "ReqBlock: [0x%08x] %s\n", (*it).m_ReqBlock, (*it).HasTransmitted() ? "SENT" : "UNSENT" );

		ctx->Indent();

		if ((*it).HasPosition() && (m_LockedMovementGo == GOID_INVALID) )
		{
			ctx->OutputF( "\nPos = [%12.8f,%12.8f,%12.8f,0x%08x]",
						  (*it).m_WayPos.pos.x,
						  (*it).m_WayPos.pos.y,
						  (*it).m_WayPos.pos.z,
						  (*it).m_WayPos.node
						 );
		}
		if ((*it).HasChore())
		{
			ctx->OutputF( "\nChore = %s, Stance = %d, SubAnim = %d, Flags = 0x%08x, Range = %5.3f",
							::ToString( (*it).m_Chore ),
							(*it).m_Stance,
							(*it).m_SubAnim,
							(*it).m_Flags,
							(*it).m_Range
							);
		}
		if ((*it).HasOrient())
		{
			ctx->OutputF( "\nOrient = [%s] ",ToString((*it).m_Mode));
			switch ((*it).m_Mode)
			{
				case ORIENTMODE_UNDEFINED:
				case ORIENTMODE_LOCK_TO_HEADING:
				case ORIENTMODE_LOCK_TO_HEADING_REVERSE:
				{
					break;
				}
				case ORIENTMODE_FIXED_DIRECTION:
				{
					ctx->OutputF( "TargRot [%12.8f,%12.8f,%12.8f,0x%08x]",
						  (*it).m_TargRot.rot.m_x,
						  (*it).m_TargRot.rot.m_y,
						  (*it).m_TargRot.rot.m_z,
						  (*it).m_TargRot.rot.m_w,
						  (*it).m_TargRot.node
						  );
					break;
				}
				case ORIENTMODE_FIXED_POSITION:
				case ORIENTMODE_FIXED_POSITION_REVERSE:
				{
					ctx->OutputF( "TargPos [%12.8f,%12.8f,%12.8f,0x%08x]",
						  (*it).m_TargPos.pos.x,
						  (*it).m_TargPos.pos.y,
						  (*it).m_TargPos.pos.z,
						  (*it).m_TargPos.node
						  );
					break;
				}
				case ORIENTMODE_TRACK_OBJECT:
				case ORIENTMODE_TRACK_OBJECT_REVERSE:
				{
					GoHandle targ((*it).m_Target);
					if (targ.IsValid())
					{
						ctx->OutputF( "Target [%s:0x%08x]",targ->GetTemplateName(),(*it).m_Target);
					}
					else
					{
						ctx->OutputF( "Target [<unknown go>:0x%08x]",(*it).m_Target);
					}
					break;
				}
			}
		}

		ctx->Output( "\n" );

		ctx->Outdent();
	}

}

#endif // !GP_RETAIL