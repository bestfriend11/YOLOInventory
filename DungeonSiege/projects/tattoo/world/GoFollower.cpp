#include "Precomp_World.h"
#include "MCP.h"
#include "MCP_Request.h"
#include "GoFollower.h"
#include "GoAspect.h"
#include "GoBody.h"
#include "GoInventory.h"
#include "nema_aspect.h"
#include "world.h"
#include "WorldTime.h"
#include "ReportSys.h"
#include "Trigger_Sys.h"
#include "NetFuBi.h"
#include "FuBiBitPacker.h"
#include "TattooGame.h"
#if !GP_RETAIL
#include "GoMind.h"
#include "Job.h"
#endif

DECLARE_GO_COMPONENT( GoFollower, "follower" );

////////////////////////////////////////////////////////////////////////////////////

FUBI_DECLARE_SELF_TRAITS( PlanSegment );
FUBI_DECLARE_SELF_TRAITS( trigger::Truid );
FUBI_DECLARE_SELF_TRAITS( trigger::BoundaryCrossing );

bool PlanSegment::Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_Mask",        m_Mask        );
	persist.Xfer( "m_Flags",       m_Flags       );
	persist.Xfer( "m_ReqBlock",	   m_ReqBlock    );
	persist.Xfer( "m_WayTime",     m_WayTime     );
	persist.Xfer( "m_BegTime",     m_BegTime     );
	persist.Xfer( "m_EndTime",     m_EndTime     );
	persist.Xfer( "m_EndVelocity", m_EndVelocity );
	
	if (m_Mask & MCP::HAS_POSITION_FLAG)
	{
		persist.Xfer( "m_WayPos",  m_WayPos	    );
		persist.Xfer( "m_BegPos",  m_BegPos   	);
		persist.Xfer( "m_EndPos",  m_EndPos	    );
	}

	if (m_Mask & MCP::HAS_CHORE_FLAG)
	{
		persist.Xfer( "m_Chore",	m_Chore		);
		persist.Xfer( "m_Stance",	m_Stance	);
		persist.Xfer( "m_SubAnim",	m_SubAnim	);
		persist.Xfer( "m_Range",	m_Range		);
	}

	if (m_Mask & MCP::HAS_ORIENT_FLAG)
	{
		persist.Xfer( "m_Mode",		m_Mode		);
		persist.Xfer( "m_Target",	m_Target	);
		persist.Xfer( "m_TargPos",	m_TargPos	);
		persist.Xfer( "m_TargRot",	m_TargRot	);		
	}

	if (m_Mask & MCP::HAS_SIGNAL_FLAG)
	{
		persist.Xfer   ( "m_Trigger",     m_Trigger     );
		persist.Xfer   ( "m_CrossType",   m_CrossType   );
		persist.XferHex( "m_HitCondID",	  m_HitCondID   );
		persist.Xfer   ( "m_SequenceNum", m_SequenceNum );
	}

	//$$$$$$$$$$$$$ add remaining data fields
	return ( true );
}

////////////////////////////////////////////////////////////////////////////////////
PlanSegment::PlanSegment( double curr_time, MCP::eRID rb, const SiegePos& wpos )
	: m_EndVelocity(0)
	, m_Mode(MCP::ORIENTMODE_UNDEFINED)
	, m_TargPos(SiegePos::INVALID)
	, m_Target(GOID_INVALID)
	, m_TargRot(SiegeRot::INVALID)
	, m_Chore(CHORE_DEFAULT)
	, m_Stance(AS_DEFAULT)
	, m_SubAnim(0)
	, m_Range(0)
	, m_Flags(0)
	, m_Trigger(trigger::Truid())
	, m_CrossType(trigger::BCROSS_ENTER_AT_TIME)
	, m_HitCondID(0)
	, m_SequenceNum(0)
{

	m_Mask = MCP::HAS_POSITION_FLAG;
	m_WayPos = wpos;
	m_ReqBlock = rb;
	m_BegTime = m_EndTime = m_WayTime = curr_time;
	m_BegPos = m_EndPos = wpos.pos;
	;

}


/////////////////////////////////////////////////////////////
// PlanSegment DebugDraw

#if !GP_RETAIL

void PlanSegment::DebugDraw( DWORD colour, const vector_3& offset, bool forcelabel ) const
{

	const vector_3 East		(0.15f, 0.00f, 0.00f);
	const vector_3 North	(0.00f, 0.00f, 0.15f);
	const vector_3 Up		(0.00f, 0.15f, 0.00f);

	static SiegePos lblpos = SiegePos::INVALID;
	static gpstring lblstr;

	vector_3 Pos = m_WayPos.pos;

	// Raise WayPoint a little
	Pos.SetY( Pos.y + 0.1f );

	gSiegeEngine.GetNodeSpaceLines().DrawLine( Pos - East,  Pos + East,  colour, m_WayPos.node );
	gSiegeEngine.GetNodeSpaceLines().DrawLine( Pos - North, Pos + North, colour, m_WayPos.node );

	const vector_3 p0 = m_BegPos; // (0.3f * Pos.pos) + (0.7f * (*seg).m_BegPos);
	const vector_3 p1 = Pos;
	const vector_3 p2 = m_EndPos; // (0.3f * Pos.pos) + (0.7f * (*seg).m_EndPos);

	vector_3 p(p0);
	vector_3 q(vector_3::ZERO);

	float t = 1.0f/16;
	for (int j = 0; j < 16; ++j, t+=1.0f/16) {
		// formula straight outta the book, will optimize --biddle
		q = ((1-t)*(1-t) * p0) + ((2*(1-t)*t) * p1 ) + ((t*t) * p2);
		gSiegeEngine.GetNodeSpaceLines().DrawLine( p + offset , q + offset, colour, m_WayPos.node );
		p = q;
	}

	gSiegeEngine.GetNodeSpaceLines().DrawLine( p0 , p0 + offset, colour, m_WayPos.node );
	gSiegeEngine.GetNodeSpaceLines().DrawLine( p2 , p2 + offset, colour, m_WayPos.node );

	if (gWorldOptions.GetLabelMCP())
	{
	
		if (m_WayTime < FLOAT_INFINITE)
		{
			if (lblpos == m_WayPos)
			{
				// Stack the labels
				lblstr.appendf("\n%8.3f",m_WayTime);
			}
			else
			{
				if (lblpos != SiegePos::INVALID)
				{
					// Draw the previous label before we start a new one
					lblpos.pos -= vector_3(0,0.3f,0);
					gWorld.DrawDebugScreenLabelColor( lblpos, lblstr.c_str(), 0xffffffff);
				}
				// Start a new stack of labels
				lblpos = m_WayPos;
				lblstr.assignf("%8.3f", m_WayTime);
			}
			unsigned char red,grn,blu;
			if (HasOrient())
			{
				lblstr.assignf("\n%8.3f", m_WayTime);
				red = 0x7f;
				grn = 0x7f;
				blu = 0xff;
				siege::SiegeLabel::AppendColorChangeEscapeSequence(lblstr,red,grn,blu);
			}
			else if (HasSignal())
			{
				lblstr.assignf("\n%8.3f", m_WayTime);
				red = 0xA0;
				grn = 0x80;
				blu = 0xf0;
				siege::SiegeLabel::AppendColorChangeEscapeSequence(lblstr,red,grn,blu);
			}
			else if (HasChore())
			{
				lblstr.assignf("\n%8.3f", m_WayTime);
				if (m_Chore == CHORE_WALK)
				{
					red = 0x7f;
					grn = 0xff;
					blu = 0x7f;
				}
				else
				{
					red = 0xff;
					grn = 0x7f;
					blu = 0x7f;
				}
				siege::SiegeLabel::AppendColorChangeEscapeSequence(lblstr,red,grn,blu);
			}
		}

		if ((forcelabel) && (lblpos != SiegePos::INVALID))
		{
			// Draw the last label
			lblpos.pos += vector_3(0,0.1f,0);
			gWorld.DrawDebugScreenLabelColor( lblpos, lblstr.c_str(), 0xffffffff);
		}

	}

	// Raise WayPoint a little more
	Pos.SetY( Pos.y + 0.1f );

	if (HasChore())
	{
		gSiegeEngine.GetNodeSpaceLines().DrawLine( Pos + North, Pos + East, 0xff2020ff, m_WayPos.node );
		gSiegeEngine.GetNodeSpaceLines().DrawLine( Pos - North, Pos + East, 0xff2020ff, m_WayPos.node );
		gSiegeEngine.GetNodeSpaceLines().DrawLine( Pos - North, Pos - East, 0xff2020ff, m_WayPos.node );
		gSiegeEngine.GetNodeSpaceLines().DrawLine( Pos + North, Pos - East, 0xff2020ff, m_WayPos.node );
	}



}

#endif // !GP_RETAIL

/////////////////////////////////////////////////////////////
// GoFollower constructor
GoFollower::GoFollower( Go* parent )
	: Inherited( parent )
{

	m_SynchronizedTime          = 0.0;
	m_CurrentStartTime          = 0.0;
	m_CurrentPosition           = SiegePos::INVALID;
	m_CurrentOrientation        = SiegeRot::INVALID;
	m_CurrentTangent            = vector_3::ZERO;
	m_CurrentVelocity           = 0;
	m_HasReachedOrientationGoal = false;
	m_OrientationMode           = MCP::ORIENTMODE_UNDEFINED;
	m_OrientationGoal           = SiegeRot::INVALID;
	m_OrientationTrackPos       = SiegePos::INVALID;
	m_OrientationTrackObj       = GOID_INVALID;
	m_LockedMovementGoid		= GOID_INVALID;
	m_SegmentQueue.clear();

}

/////////////////////////////////////////////////////////////
// GoFollower constructor
GoFollower::GoFollower( const GoFollower& source, Go* parent )
	: Inherited( source, parent )
{
	m_SynchronizedTime			= source.m_SynchronizedTime;
	m_CurrentStartTime			= source.m_CurrentStartTime;
	m_CurrentPosition			= source.m_CurrentPosition;
	m_CurrentOrientation		= source.m_CurrentOrientation;
	m_CurrentTangent            = source.m_CurrentTangent;
	m_CurrentVelocity           = source.m_CurrentVelocity;
	m_HasReachedOrientationGoal = source.m_HasReachedOrientationGoal;
	m_OrientationMode			= source.m_OrientationMode;
	m_OrientationGoal           = source.m_OrientationGoal;
	m_OrientationTrackPos       = source.m_OrientationTrackPos;
	m_OrientationTrackObj       = source.m_OrientationTrackObj;
	m_LockedMovementGoid		= source.m_LockedMovementGoid;
	m_SegmentQueue              = source.m_SegmentQueue;

}

/////////////////////////////////////////////////////////////
GoFollower::~GoFollower(void)
{
}

/////////////////////////////////////////////////////////////
GoComponent* GoFollower::Clone( Go* newParent )
{
	return ( new GoFollower( *this, newParent ) );
}

/////////////////////////////////////////////////////////////

FUBI_DECLARE_CAST_TRAITS( MCP::eRID, DWORD );
FUBI_DECLARE_ENUM_TRAITS( MCP::eOrientMode, MCP::ORIENTMODE_UNDEFINED );

bool GoFollower::Xfer ( FuBi::PersistContext& persist )
{
	if ( persist.IsFullXfer() )
	{
		if (persist.IsRestoring())
		{
			m_SignalsPending.clear();
			m_SegmentQueue.clear();
		}
		persist.Xfer     ( "m_OrientationMode",           m_OrientationMode           );
		persist.Xfer     ( "m_OrientationGoal",           m_OrientationGoal           );
		persist.Xfer     ( "m_OrientationTrackPos",       m_OrientationTrackPos       );
		persist.Xfer     ( "m_OrientationTrackObj",       m_OrientationTrackObj       );
		persist.Xfer     ( "m_HasReachedOrientationGoal", m_HasReachedOrientationGoal );
		persist.XferDeque( "m_SegmentQueue",              m_SegmentQueue              );
		persist.Xfer     ( "m_CurrentPosition",           m_CurrentPosition           );
		persist.Xfer     ( "m_CurrentTangent",            m_CurrentTangent            );
		persist.Xfer     ( "m_CurrentOrientation",        m_CurrentOrientation        );
		persist.Xfer     ( "m_CurrentVelocity",           m_CurrentVelocity           );
		persist.Xfer     ( "m_SynchronizedTime",          m_SynchronizedTime          );
		persist.Xfer     ( "m_CurrentStartTime",          m_CurrentStartTime          );
		persist.Xfer     ( "m_LockedMovementGoid",        m_LockedMovementGoid        );

		persist.XferSet  ( FuBi::XFER_HEX, "m_SignalsPending", m_SignalsPending       );

	}

	return ( true );
}

/////////////////////////////////////////////////////////////
bool GoFollower::ForceUpdatePositionOrientation( void )
{

	m_CurrentPosition    = GetGo()->GetPlacement()->GetPosition();
	m_CurrentOrientation = m_OrientationGoal = SiegeRot(GetGo()->GetPlacement()->GetOrientation(),m_CurrentPosition.node);
	m_HasReachedOrientationGoal = true;

	return ( true );
}

/////////////////////////////////////////////////////////////
void GoFollower::HandleMessage( const WorldMessage& msg )
{
	gpassert( !msg.IsCC() );

	if ( msg.GetEvent() == WE_ENTERED_WORLD )
	{
		gTriggerSys.UpdateTriggersWhenGoEntersWorld( GetGo(),msg.GetData1() );
	}	
	else if ( msg.GetEvent() == WE_TERRAIN_TRANSITION_DONE )
	{
		// We have just been subjected to a node transition
		gTriggerSys.UpdateTriggersAfterTerrainMoves( GetGo() );
	}
}

#define MCP_TOTAL_PARANOIA 0
#if MCP_TOTAL_PARANOIA
#define CHECK_VALID_SEGTIMES_1(c)											\
	gpassertf(!IsNegative((float)(PreciseSubract((c).m_WayTime, (c).m_BegTime)),0.001f),	\
		("Cseg BT: %g WT: %g WT-BT: %g\n",									\
		 (c).m_BegTime,(c).m_WayTime,PreciseSubract((c).m_WayTime, (c).m_BegTime)			\
		));																	\
	gpassertf(!IsNegative((float)(PreciseSubract((c).m_EndTime, (c).m_WayTime)),0.001f),	\
		("CSeg WT: %g ET: %g ET-WT: %g\n",									\
		(c).m_WayTime,(c).m_EndTime,PreciseSubract((c).m_EndTime, (c).m_WayTime)			\
		));


#define CHECK_VALID_SEGTIMES_2(c,n)											\
	gpassertf(!IsNegative((float)(PreciseSubract((c).m_WayTime, (c).m_BegTime)),0.001f),	\
		("Cseg BT: %g WT: %g WT-BT: %g\n",									\
		(c).m_BegTime,(c).m_WayTime,PreciseSubract((c).m_WayTime, (c).m_BegTime)			\
		));																	\
	gpassertf(!IsNegative((float)(PreciseSubract((c).m_EndTime, (c).m_WayTime)),0.001f),	\
		("CSeg WT: %g ET: %g ET-WT: %g\n"									\
		"NSeg BT: %g WT: %g WT-BT: %g\n",									\
		(c).m_WayTime,(c).m_EndTime,PreciseSubract((c).m_EndTime, (c).m_WayTime),			\
		(n).m_BegTime,(n).m_WayTime,PreciseSubract((n).m_WayTime, (n).m_BegTime)			\
		));																	\
	gpassertf(!(n).IsPrepared() || IsZero((float)(PreciseSubract((c).m_EndTime, (n).m_BegTime)),0.001f),	\
		("CSeg ET: %g NSeg BT: %g NBT-CET: %g\n",							\
		(c).m_EndTime,(n).m_BegTime,PreciseSubract((n).m_BegTime, (c).m_EndTime)			\
		));
#endif


//------------------------------------------------------------
// PreparePlanSegment is a helper function used to for JIT calculation
// of the segment begin and endpos

bool PrepareNextSegment(PlanSegment &curr_seg, PlanSegment &next_seg)
{
	bool ret = false;

	const SiegePos& curr_wayp = curr_seg.m_WayPos;
	const SiegePos& next_wayp = next_seg.m_WayPos;

	if (gSiegeEngine.IsNodeInAnyFrustum(curr_wayp.node))
	{
		// We can't prepare anything yet if the current node hasn't been loaded

		if (curr_seg.m_Mask & MCP::HAS_SEGMENT_PREPPED)
		{
			ret = true;
		}

		else
		{
			if (!(curr_seg.m_Mask & MCP::HAS_SEGMENT_BEGINPOS_PREPPED))
			{
				curr_seg.m_BegPos = curr_seg.m_WayPos.pos;
				curr_seg.m_Mask |= MCP::HAS_SEGMENT_BEGINPOS_PREPPED;
			}

			if (!curr_seg.HasPosition())
			{
				curr_seg.m_Mask |= MCP::HAS_SEGMENT_PREPPED|MCP::HAS_SEGMENT_BEGINPOS_PREPPED;
				curr_seg.m_EndVelocity = 0;
				next_seg.m_EndVelocity = 0;
			}
			else
			{
				if (gSiegeEngine.IsNodeInAnyFrustum(next_wayp.node))
				{

					vector_3 diff = gSiegeEngine.GetDifferenceVector(curr_wayp,next_wayp);

					// Ideally we want to calculate a line integral on the spline that 
					// the interval defines and use that for the velocity calc
					// For the time being, I am using staight line dist between 
					// the begin point and the endpoint
					// We store the 'end' velocity for the current segment. The 'begin'
					// velocity is simply the 'end' velocity of the previous segment

					float tdelta = (float)( PreciseSubtract(next_seg.m_WayTime, curr_seg.m_WayTime));
					if (IsPositive(tdelta))
					{
						curr_seg.m_EndVelocity = Length(diff)/tdelta;
					}
					else
					{
						curr_seg.m_EndVelocity = 0;
					}
					next_seg.m_EndVelocity = 0;

					curr_seg.m_EndPos = curr_wayp.pos + (0.5f*diff);

					if (next_wayp.node == curr_wayp.node)
					{
						next_seg.m_BegPos = curr_seg.m_EndPos;
					}
					else
					{
						// The next pos is in a different node altogether
						diff = gSiegeEngine.GetDifferenceVector(next_wayp,curr_wayp);

						next_seg.m_BegPos = next_wayp.pos + (0.5f*diff);

					}

					curr_seg.m_Mask |= MCP::HAS_SEGMENT_PREPPED;
					next_seg.m_Mask |= MCP::HAS_SEGMENT_BEGINPOS_PREPPED;

					ret = true;
				}

			}
		}
	}

#if MCP_TOTAL_PARANOIA
	if (ret)
	{
		CHECK_VALID_SEGTIMES_2(curr_seg,next_seg);
	}
#endif

	// We return true only the current segment has both begin and end point prepped
	return ret;
}

//------------------------------------------------------------
// PrepareLastSegment is a helper function used to for JIT calculation
// of the LAST segment begin and endpos.

bool PrepareLastSegment(PlanSegment &last_seg)
{

	bool ret = false;
	if (gSiegeEngine.IsNodeInAnyFrustum(last_seg.m_WayPos.node))
	{
		// We can't prepare anything yet if the current node hasn't been loaded

		if (!(last_seg.IsPrepared()))
		{
			if (!(last_seg.IsBeginPosPrepared()))
			{
				last_seg.m_BegPos = last_seg.m_WayPos.pos;
				last_seg.m_Mask |= MCP::HAS_SEGMENT_BEGINPOS_PREPPED;
			}

			last_seg.m_EndPos = last_seg.m_WayPos.pos;
			last_seg.m_Mask |= MCP::HAS_SEGMENT_PREPPED;

			last_seg.m_EndVelocity = 0;

		}

		ret = true;
	}

#if MCP_TOTAL_PARANOIA
	if (ret)
	{
		CHECK_VALID_SEGTIMES_1(last_seg);
	}
#endif
	
	return ret;

}

/////////////////////////////////////////////////////////////
void GoFollower::ClipAllQueues( const double &clip_time, const SiegePos &clip_pos )
{

	// This routine will clip all the queues that the follower is about to use
	// It will ensure that the last position in the segment queue is the clip seg

	// clip Segment Queue

	PlanSegment clip_seg(clip_time,MCP::RID_CLIP,clip_pos);

	{
		PlanSegQIter beg = m_SegmentQueue.begin();
		PlanSegQIter end = m_SegmentQueue.end();
		PlanSegQIter i;

		double wt;
		for (i = beg;  i != end; ++i)
		{
			// We keep the last segment after a clip, it defines the end of the
			// interval that we want to include. 
			wt = (*i).m_WayTime;

			if ( IsPositive((float)(PreciseSubtract(wt,clip_time)),0.001f) )
			{
				break;
			}

		}

#if GP_DEBUG
		for (PlanSegQIter j = i;  j != end; ++j)
		{
			gpassertf(!(*j).HasSignal(),("MCP CLIPPER ERROR: [%s g:%08x s:%08x] Clipped a plan segment with a signal!",GetGo()->GetTemplateName(),GetGo()->GetGoid(),GetGo()->GetScid()));
		}
#endif

		if (i != end)
		{
			m_SegmentQueue.erase(i,end);

			// Add the segment that was used to define the clip
			if (!m_SegmentQueue.empty())
			{
				PlanSegment& last_seg = m_SegmentQueue.back();

				/* Over-zealous
				gpassertf( clip_seg.m_WayTime>=last_seg.m_WayTime , 
					("GoFollower::ClipAllQueues() [%s g:%08x s:%08x]\n\tNEW clip_seg.m_WayTime < OLD back().m_WayTime (%f<%f))",
					GetGo()->GetTemplateName(),GetGo()->GetGoid(),GetGo()->GetScid(),
					clip_seg.m_WayTime,last_seg.m_WayTime));
				*/

				if (IsEqual(last_seg.m_WayPos,clip_seg.m_WayPos,0.001f))
				{
					last_seg.m_EndTime = last_seg.m_WayTime;
					clip_seg.m_BegTime = clip_seg.m_WayTime;
				}
				else
				{
					double  et = PreciseMultiply( PreciseAdd(last_seg.m_WayTime,clip_seg.m_WayTime),  0.5 );
					last_seg.m_EndTime = et;
					clip_seg.m_BegTime = et;
				}

				// Make sure that the last segment before the clip is marked as 
				// "not prepped" so that is gets prepped again with the new clip_seg info
				// We don't reset the begin pos prepped flag, we want to keep the original
				// begin pos of the segment intact
				last_seg.m_Mask &= ~(MCP::HAS_SEGMENT_PREPPED);
			}
		}
		else
		{
			if (!m_SegmentQueue.empty())
			{
				m_SegmentQueue.push_back(m_SegmentQueue.back());
				m_SegmentQueue.back().m_EndTime = clip_seg.m_WayTime;
				clip_seg.m_BegTime = clip_seg.m_WayTime;
				m_SegmentQueue.back().m_Mask &= ~(MCP::HAS_SEGMENT_PREPPED);
			}
		}

		m_SegmentQueue.push_back(clip_seg);

	}

}

void GoFollower::SSendPackedUpdateToFollowers( FuBi::BitPacker& packer,  DWORD frustums )
{
	CHECK_SERVER_ONLY;

	FuBi::AddressColl addresses;
	
	gGoDb.FillAddressesForFrustum( (FrustumId)frustums, addresses);
//	gGoDb.FillAddressesForFrustum(GetGo()->GetWorldFrustumMembership(),addresses);

	FuBi::AddressColl::iterator i;
	FuBi::AddressColl::iterator ibegin = addresses.begin();
	FuBi::AddressColl::iterator iend = addresses.end();
	
	for ( i = ibegin ; (i != iend) && (*i != NULL) ; ++i )
	{
		FUBI_SET_RPC_PEEKADDRESS( (*i) );
		RCSendPackedUpdateToFollowers( packer.GetSavedBits() );
	}

}

void GoFollower::RCSendPackedUpdateToFollowers( const_mem_ptr ptr )
{
	FUBI_RPC_THIS_CALL_PEEKADDRESS( RCSendPackedUpdateToFollowers );

	FuBi::BitPacker packer( ptr );
	SendPackedUpdateToFollowers( packer );
}

void GoFollower::SendPackedUpdateToFollowers( FuBi::BitPacker& packer )
{

	MCP::TimeNode tn;

	DWORD num_packed_nodes = 0;

	packer.XferCount(num_packed_nodes);

#if !GP_RETAIL
	gpstring buff;
	buff.assignf("MCP-IN [%s:g:%08x]",
		GetGo()->GetTemplateName(),
		GetGo()->GetGoid()
		);
#endif

	for (DWORD i=0;i<num_packed_nodes;++i)
	{
		tn.XferForSync(packer,m_CurrentStartTime);

#if !GP_RETAIL
		tn.Crack(i,buff);
#endif

		DecodeNode(tn);
	}

#if !GP_RETAIL
	buff.appendf("\n");
	gpdevreportf( &gMCPNetLogContext, (buff.c_str()));
#endif
}

/////////////////////////////////////////////////////////////
void GoFollower::DecodeNode(const MCP::TimeNode& tn)
{

	if (tn.HasLockedMovementGo())
	{
		SetLockedMovementGoid(tn.m_LockedMoveGo);
	}

	if (tn.HasPosition())
	{

		PlanSegment seg(tn.m_Time,tn.m_ReqBlock,tn.m_WayPos);
		
		if (!m_SegmentQueue.empty())
		{

			PlanSegment& last_seg = m_SegmentQueue.back();

#if MCP_TOTAL_PARANOIA
			gpassertf( !IsNegative((float)(PreciseSubtract(seg.m_WayTime,last_seg.m_WayTime)),0.01f) , 
				("GoFollower::DecodeNode() [%s g:%08x s:%08x]\n\tNEW seg.m_WayTime < OLD back().m_WayTime (%f<%f))",
				GetGo()->GetTemplateName(),GetGo()->GetGoid(),GetGo()->GetScid(),
				seg.m_WayTime,
				last_seg.m_WayTime));
#endif

			if (seg.m_WayTime<last_seg.m_WayTime)
			{
				seg.m_WayTime = seg.m_BegTime = seg.m_EndTime = last_seg.m_WayTime;
			}

			if (IsEqual(last_seg.m_WayPos,seg.m_WayPos,0.001f))
			{
				last_seg.m_EndTime = last_seg.m_WayTime;
				seg.m_BegTime = seg.m_WayTime;
			}
			else
			{
				double  et = PreciseMultiply( PreciseAdd(last_seg.m_WayTime,seg.m_WayTime), 0.5);
				last_seg.m_EndTime = et;
				seg.m_BegTime = et;
			}

			// This new path segment modifies the endpos of segment at the end of the queue
			// The back seg is no longer in a prepped state
			last_seg.m_Mask &= (~MCP::HAS_SEGMENT_PREPPED);
		}

		m_SegmentQueue.push_back(seg);

	}

	if (tn.HasOrient())
	{
		if ( m_SegmentQueue.empty() )
		{
			// We need to push a new segment onto the queue
			PlanSegment seg(PreciseAdd(m_CurrentStartTime,tn.m_Time),tn.m_ReqBlock,m_CurrentPosition);
			m_SegmentQueue.push_back(seg);
		}
		else
		{
			if (!IsZero((float)(PreciseSubtract(tn.m_Time,m_SegmentQueue.back().m_WayTime))))
			{
				// Add another segment
				m_SegmentQueue.push_back(m_SegmentQueue.back());
				m_SegmentQueue.back().ClearMask();
				m_SegmentQueue.back().Collapse();
				m_SegmentQueue.back().SetTime(tn.m_Time);
			}
		}

		switch (tn.m_Mode)
		{
		case MCP::ORIENTMODE_FIXED_DIRECTION : 
			{
				m_SegmentQueue.back().SetOrient(tn.m_Mode,tn.m_TargRot);
				break;
			}
		case MCP::ORIENTMODE_FIXED_POSITION : 
		case MCP::ORIENTMODE_FIXED_POSITION_REVERSE : 
			{
				m_SegmentQueue.back().SetOrient(tn.m_Mode,tn.m_TargPos);
				break;
			}
		case MCP::ORIENTMODE_TRACK_OBJECT : 
		case MCP::ORIENTMODE_TRACK_OBJECT_REVERSE : 
		case MCP::ORIENTMODE_UNDEFINED :
			{
				m_SegmentQueue.back().SetOrient(tn.m_Mode,tn.m_Target);
				break;
			}
		case MCP::ORIENTMODE_LOCK_TO_HEADING :
		case MCP::ORIENTMODE_LOCK_TO_HEADING_REVERSE :
			{
				m_SegmentQueue.back().SetOrient(tn.m_Mode);
				break;
			}
		default:
			{
				gperrorf( ("GoFollower::DecodeNode() Can't handle the orientmode = %d \n",tn.m_Mode) );
				break;
			}
		}

	}

	if (tn.HasChore())
	{
		if ( m_SegmentQueue.empty() )
		{
			// We need to push a new segment onto the queue
			PlanSegment seg(tn.m_Time,tn.m_ReqBlock,m_CurrentPosition);
			m_SegmentQueue.push_back(seg);
		} 
		else if (!IsZero((float)(PreciseSubtract(tn.m_Time,m_SegmentQueue.back().m_WayTime))))
		{
			// Add another segment
			m_SegmentQueue.push_back(m_SegmentQueue.back());
			m_SegmentQueue.back().ClearMask();
			m_SegmentQueue.back().Collapse();
			m_SegmentQueue.back().SetTime(tn.m_Time);
		}
		m_SegmentQueue.back().SetChore(tn.m_Chore,tn.m_Stance,tn.m_SubAnim,0.0f,tn.m_Flags);
	}

	if (tn.HasSignal())
	{
		if ( m_SegmentQueue.empty() )
		{
			// We need to push a new segment onto the queue
			PlanSegment seg(tn.m_Time,tn.m_ReqBlock,m_CurrentPosition);
			m_SegmentQueue.push_back(seg);
		}
		else if ( m_SegmentQueue.back().HasSignal() || (!IsZero((float)(PreciseSubtract(tn.m_Time,m_SegmentQueue.back().m_WayTime)))))
		{
			// Add another segment
			m_SegmentQueue.push_back(m_SegmentQueue.back());
			m_SegmentQueue.back().ClearMask();
			m_SegmentQueue.back().Collapse();
			m_SegmentQueue.back().SetTime(tn.m_Time);
		}
		m_SegmentQueue.back().SetSignal(tn.m_Trigger,tn.m_CrossType,tn.m_CondID,tn.m_SeqNum);

	}

#if MCP_TOTAL_PARANOIA
	if (!m_SegmentQueue.empty())
	{
		for (DWORD tt = 1; tt<m_SegmentQueue.size();++tt)
		{
			CHECK_VALID_SEGTIMES_2(m_SegmentQueue[tt-1],m_SegmentQueue[tt]);
		}
		CHECK_VALID_SEGTIMES_1(m_SegmentQueue[tt-1]);
	}
#endif

	return;
}

void GoFollower::SSendPackedPositionUpdateToFollowers( FuBi::BitPacker& packer,  DWORD frustums )
{
	CHECK_SERVER_ONLY;

	FuBi::AddressColl addresses;
	
	gGoDb.FillAddressesForFrustum( (FrustumId)frustums, addresses);

	FuBi::AddressColl::iterator i;
	FuBi::AddressColl::iterator ibegin = addresses.begin();
	FuBi::AddressColl::iterator iend = addresses.end();
	
	for ( i = ibegin ; (i != iend) && (*i != NULL) ; ++i )
	{
		FUBI_SET_RPC_PEEKADDRESS( (*i) );
		RCSendPackedPositionUpdateToFollowers( packer.GetSavedBits() );
	}

}

void GoFollower::RCSendPackedPositionUpdateToFollowers( const_mem_ptr ptr )
{
	FUBI_RPC_THIS_CALL_PEEKADDRESS( RCSendPackedPositionUpdateToFollowers );

	FuBi::BitPacker packer( ptr );
	SendPackedPositionUpdateToFollowers( packer );
}

void GoFollower::SendPackedPositionUpdateToFollowers( FuBi::BitPacker& packer )
{
	m_SegmentQueue.clear();
	
	packer.XferFloat(m_CurrentPosition.pos.x);
	packer.XferFloat(m_CurrentPosition.pos.y);
	packer.XferFloat(m_CurrentPosition.pos.z);
	packer.XferRaw(m_CurrentPosition.node);

	GetGo()->GetPlacement()->ForceSetPosition(m_CurrentPosition);
}

/////////////////////////////////////////////////////////////
void GoFollower::SSendPackedClipToFollowers( FuBi::BitPacker& packer )
{
	CHECK_SERVER_ONLY;

	FuBi::AddressColl addresses;
	
	gGoDb.FillAddressesForFrustum(GetGo()->GetWorldFrustumMembership(),addresses);

	FuBi::AddressColl::iterator i;
	FuBi::AddressColl::iterator ibegin = addresses.begin();
	FuBi::AddressColl::iterator iend = addresses.end();
	
	for ( i = ibegin ; (i != iend) && (*i != NULL) ; ++i )
	{
		FUBI_SET_RPC_PEEKADDRESS( (*i) );
		RCSendPackedClipToFollowers( packer.GetSavedBits() );
	}

}

void GoFollower::RCSendPackedClipToFollowers( const_mem_ptr ptr )
{
	FUBI_RPC_THIS_CALL_PEEKADDRESS( RCSendPackedClipToFollowers );

	FuBi::BitPacker packer( ptr );
	SendPackedClipToFollowers( packer );
}

void GoFollower::SendPackedClipToFollowers( FuBi::BitPacker &packer )
{
	double clip_time;
	SiegePos clip_pos;

	packer.XferRaw(clip_time);
	packer.XferFloat(clip_pos.pos.x);
	packer.XferFloat(clip_pos.pos.y);
	packer.XferFloat(clip_pos.pos.z);
	packer.XferRaw(clip_pos.node);

	ClipAllQueues(clip_time,clip_pos);

#if !GP_RETAIL
	gpstring buff;	
	buff.assignf("MCPCLIP-IN [%s:g:%08x] %f [%f,%f,%f,%08x]\n",
		GetGo()->GetTemplateName(),
		GetGo()->GetGoid(),
		clip_time,
		clip_pos.pos.x,
		clip_pos.pos.y,
		clip_pos.pos.z,
		clip_pos.node
		);
	gpdevreportf( &gMCPNetLogContext, (buff.c_str()));
#endif

}

void GoFollower::UnpackRefresh( FuBi::BitPacker &inout_packer )
{
	m_SegmentQueue.clear();

	// Current Position

	inout_packer.XferRaw( m_CurrentStartTime );

	inout_packer.XferFloat( m_CurrentPosition.pos.x );
	inout_packer.XferFloat( m_CurrentPosition.pos.y );
	inout_packer.XferFloat( m_CurrentPosition.pos.z );
	inout_packer.XferRaw  ( m_CurrentPosition.node  );	

	PlanSegment seg(m_CurrentStartTime,MCP::RID_REFRESH,m_CurrentPosition);

	seg.m_Mask |= MCP::HAS_SEGMENT_PREPPED|MCP::HAS_SEGMENT_BEGINPOS_PREPPED;
	m_SegmentQueue.push_back(seg);

	seg.m_Mask &= ~MCP::HAS_SEGMENT_PREPPED;
	m_SegmentQueue.push_back(seg);

	// Current Rotation

	inout_packer.XferFloat( m_CurrentOrientation.rot.m_x    );
	inout_packer.XferFloat( m_CurrentOrientation.rot.m_y    );
	inout_packer.XferFloat( m_CurrentOrientation.rot.m_z    );
	inout_packer.XferFloat( m_CurrentOrientation.rot.m_w    );
	inout_packer.XferRaw  ( m_CurrentOrientation.node );	

	// Chore 
	eAnimChore  ac;
	inout_packer.XferRaw( ac,  FUBI_MAX_ENUM_BITS( CHORE_ ));

	DWORD val;
	inout_packer.XferRaw( val, FuBi::GetMaxEnumBits(0,AS_END-AS_BEGIN) );
	eAnimStance st = (eAnimStance)(val+AS_BEGIN);

	DWORD sa;
	inout_packer.XferCount( sa, 1 );

	DWORD fl;
	inout_packer.XferRaw  ( fl );

	bool special = (ac == CHORE_ATTACK) || (ac == CHORE_MAGIC);
	// Unpack the current offset from the flags and start the animation up 
	float elapsed = special ? 0 : DecodeChoreElapsedTime(fl);
	DWORD flags = special ? fl : DecodeChoreFlags(fl);
	nema::Aspect* aspect = GetGo()->GetAspect()->GetAspectPtr();
	aspect->SetNextChore(ac,st,sa,flags);
	aspect->RefreshAnimationStateMachine(elapsed);

	// Orient
	inout_packer.XferRaw( m_OrientationMode,  FUBI_MAX_ENUM_BITS( MCP::ORIENTMODE_ ) );

	if (( m_OrientationMode == MCP::ORIENTMODE_FIXED_POSITION ) || ( m_OrientationMode == MCP::ORIENTMODE_FIXED_POSITION_REVERSE ) )
	{
		inout_packer.XferFloat( m_OrientationTrackPos.pos.x );
		inout_packer.XferFloat( m_OrientationTrackPos.pos.y );
		inout_packer.XferFloat( m_OrientationTrackPos.pos.z );
		inout_packer.XferRaw  ( m_OrientationTrackPos.node  );	
	}
	else if (( m_OrientationMode == MCP::ORIENTMODE_FIXED_DIRECTION ) || ( m_OrientationMode == MCP::ORIENTMODE_FIXED_POSITION_REVERSE ) ) 
	{
		inout_packer.XferFloat( m_OrientationGoal.rot.m_x    );
		inout_packer.XferFloat( m_OrientationGoal.rot.m_y    );
		inout_packer.XferFloat( m_OrientationGoal.rot.m_z    );
		inout_packer.XferFloat( m_OrientationGoal.rot.m_w    );
		inout_packer.XferRaw  ( m_OrientationGoal.node );	
	}
	else if (( m_OrientationMode == MCP::ORIENTMODE_TRACK_OBJECT ) || ( m_OrientationMode == MCP::ORIENTMODE_TRACK_OBJECT_REVERSE ) ) 
	{
		inout_packer.XferRaw( m_OrientationTrackObj );	
	}
	
	m_HasReachedOrientationGoal = false;

	inout_packer.XferRaw( m_LockedMovementGoid );	
}


/////////////////////////////////////////////////////////////
// GoFollower Update
void GoFollower::Update(float /*game_delta_time*/)
{
	GPSTATS_GROUP( GROUP_PATH_FOLLOW );

	// $$$ jip
	// this component should never update
	if( GetGo()->IsOmni() )
	{
		return;
	}
	
	gpassert( GetGo()->IsInActiveWorldFrustum() );

	double wt;
	
	if( IsServerLocal() )
	{
		// Tell the MCP to process this Go
		gMCP.UpdateGo(GetGoid());
		wt = gMCP.GetLaggingTime();
	}
	else
	{
		wt = gWorldTime.GetTime();
	}

	
	float delta_time = (float)(PreciseSubtract(wt,m_SynchronizedTime));
	m_SynchronizedTime = wt;

	if (!IsPositive(delta_time))
	{
		return;
	}

	if (!m_SegmentQueue.empty())
	{
		while(!m_SegmentQueue.empty())
		{

			if (!m_SegmentQueue[0].IsPrepared())
			{
				if (m_SegmentQueue.size() == 1)
				{
					PrepareLastSegment(m_SegmentQueue[0]);
				}
				else
				{
					PrepareNextSegment(m_SegmentQueue[0],m_SegmentQueue[1]);
				}
			}

#if MCP_TOTAL_PARANOIA
			if (m_SegmentQueue[0].IsPrepared())  
			{
				if ((m_SegmentQueue.size() == 1))
				{
					CHECK_VALID_SEGTIMES_1(m_SegmentQueue[0]);
				}
				else
				{
					CHECK_VALID_SEGTIMES_2(m_SegmentQueue[0],m_SegmentQueue[1]);
				}
			}
#endif
			const PlanSegment& CurrSeg = m_SegmentQueue[0];

			// tau stores the normalized displacement along this segment.
			float tau = 0.0f;


			gpassert( (CurrSeg.m_Chore != CHORE_DIE) || (CurrSeg.m_Mode == MCP::ORIENTMODE_UNDEFINED) );

			GoAspect* aspect = GetGo()->GetAspect();

			nema::HAspect haspect = aspect->GetAspectHandle();


			if (!CurrSeg.IsPrepared() || CurrSeg.m_WayTime <= m_SynchronizedTime)
			{

				haspect->SetCurrReqBlock(CurrSeg.m_ReqBlock);

				if ((CurrSeg.m_Mask & MCP::HAS_ORIENT_FLAG))
				{
					//+++++++++++++++++++++
					// Set the orientation
					m_HasReachedOrientationGoal = false;
					m_OrientationMode = CurrSeg.m_Mode;

					if ((m_OrientationMode == MCP::ORIENTMODE_TRACK_OBJECT) || (m_OrientationMode == MCP::ORIENTMODE_TRACK_OBJECT_REVERSE))
					{
						m_OrientationTrackObj = CurrSeg.m_Target;
					}
					else if ((m_OrientationMode == MCP::ORIENTMODE_FIXED_POSITION) || (m_OrientationMode == MCP::ORIENTMODE_FIXED_POSITION_REVERSE))
					{
						m_OrientationTrackPos = CurrSeg.m_TargPos;
					}
					else if (m_OrientationMode == MCP::ORIENTMODE_FIXED_DIRECTION)
					{
						m_OrientationGoal =  CurrSeg.m_TargRot;
						gpassert(!(m_OrientationGoal == SiegeRot::INVALID));
					}
					else if (m_OrientationMode == MCP::ORIENTMODE_UNDEFINED)
					{
						if (m_CurrentOrientation == SiegeRot::INVALID)
						{
							m_CurrentOrientation = SiegeRot(GetGo()->GetPlacement()->GetOrientation(),GetGo()->GetPlacement()->GetPosition().node);
						}
						m_OrientationTrackObj = GOID_INVALID;
						m_OrientationGoal = m_CurrentOrientation;
						m_HasReachedOrientationGoal = true;
					}

					m_SegmentQueue[0].m_Mask &= ~MCP::HAS_ORIENT_FLAG;
				}
				
				if (CurrSeg.m_Mask & MCP::HAS_CHORE_FLAG)
				{
					//+++++++++++++++++++++
					// Set the chore

					if ( IsServerLocal() )
					{
						WorldMessage msg( WE_MCP_CHORE_CHANGING, GOID_INVALID, GetGoid(), (DWORD)CurrSeg.m_ReqBlock );
						msg.SendDelayed( 0 );
					}

#if !GP_RETAIL
					if (gWorldOptions.GetHudMCP())
					{
						SiegePos p = CurrSeg.m_WayPos;
						p.pos.y += 2.0f*GetGo()->GetAspect()->GetBoundingSphereRadius();
						
						gpstring txt = ToString(CurrSeg.m_Chore);
						txt.append(":");
						txt.append(ToString(CurrSeg.m_Stance));
						txt.append(":");
						txt.appendf("%d",CurrSeg.m_SubAnim);

						gWorld.DrawDebugWorldLabelScroll( p, txt,0xffff00ff,3,true);
					}
#endif // !GP_RETAIL

					haspect->SetNextChore(
						CurrSeg.m_Chore,
						CurrSeg.m_Stance,
						CurrSeg.m_SubAnim,
						CurrSeg.m_Flags
						);

					m_SegmentQueue[0].m_Mask &= ~MCP::HAS_CHORE_FLAG;
				}

				if (CurrSeg.m_Mask & MCP::HAS_SIGNAL_FLAG)
				{
					//+++++++++++++++++++++
					// Send a signal the trigger indicating that we are
					// just about to enter or exit it's boundary
					trigger::BoundaryCrossing bc(CurrSeg.m_WayTime,CurrSeg.m_Trigger,MakeInt(GetGo()->GetGoid()),CurrSeg.m_CrossType,CurrSeg.m_HitCondID,CurrSeg.m_SequenceNum);

					if ((CurrSeg.m_CrossType != trigger::BCROSS_ENTER_ON_NODE_CHANGE) &&
						(CurrSeg.m_CrossType != trigger::BCROSS_LEAVE_ON_NODE_CHANGE))
					{
						gTriggerSys.SignalTrigger(bc);

						gpdevreportf( &gMCPSignalContext,("At time %8.3f, [%s:0x%08x] %s the boundary of trigger [0x%08x]\n",
							CurrSeg.m_WayTime,
							GetGo()->GetTemplateName(),	GetGo()->GetGoid(),
							trigger::IsEnteringAtCrossing(CurrSeg.m_CrossType)?"ENTERS":"LEAVES",
							CurrSeg.m_Trigger));

					}
					else
					{
						// We are looking for a node change. We'll send the signal as soon as the current
						// position node matches the next waypos node. Since we are running on the clients
						// we will send the signal 'just in time'.					
						if (m_SegmentQueue.size()>1)
						{
							// HACKAHACK to stuff data in
							if (CurrSeg.IsPrepared())
							{
								bc.m_Data = m_SegmentQueue[1].m_WayPos.node.GetValue();
								bc.m_Time = m_SegmentQueue[1].m_WayTime;
								m_SignalsPending.insert(bc);
							}
							else
							{
								// The current segment wasn't prepped, so we 
								// are jumping straight to the next, so the trigger must be true
								gTriggerSys.SignalTrigger(bc);
							}
						}
						else
						{
							gperrorf(("ERROR GoFollower: the segment queue is empty, unable to create delayed signal!"));
							//bid
							// Maybe we want to simply signal the trigger anyways?
						}
					}
					m_SegmentQueue[0].m_Mask &= ~MCP::HAS_SIGNAL_FLAG;

				}		
			}

			if (CurrSeg.m_Mask & MCP::HAS_POSITION_FLAG)
			{
				//+++++++++++++++++++++
				// Set the position

				// Only move to the calculated position if the waypos we are using is
				// in a valid node ON THIS CLIENT

				if (!CurrSeg.IsPrepared())
				{
						gpdevreportf( &gMCPNetLogContext,(
							"GoFollower::Update() [%s:g:%08x] using unprepped segment, tangent is not valid\n",
							GetGo()->GetTemplateName(),
							GetGo()->GetGoid()
							));
						if (m_SegmentQueue.size()>1)
						{
							if (gSiegeEngine.IsNodeInAnyFrustum(CurrSeg.m_WayPos.node) &&
								gSiegeEngine.IsNodeInAnyFrustum(m_SegmentQueue[1].m_WayPos.node))
							{
								m_CurrentTangent = gSiegeEngine.GetDifferenceVector(CurrSeg.m_WayPos,m_SegmentQueue[1].m_WayPos);
							}
						}
						else if ((gSiegeEngine.IsNodeInAnyFrustum(m_CurrentPosition.node)) &&
								 (gSiegeEngine.IsNodeInAnyFrustum(CurrSeg.m_WayPos.node)))							
						{
								m_CurrentTangent = gSiegeEngine.GetDifferenceVector(m_CurrentPosition,CurrSeg.m_WayPos);
						}
						m_CurrentPosition = CurrSeg.m_WayPos;
						m_CurrentVelocity = 0.0f;
						
				}				
				else 
				{
					if ((m_SynchronizedTime) < CurrSeg.m_BegTime)
					{
						gpdevreportf( &gMCPNetLogContext,(
							"GoFollower::Update() [%s:g:%08x] has current path segment begin > current time (%f > %f)\n",
							GetGo()->GetTemplateName(),
							GetGo()->GetGoid(),
							CurrSeg.m_BegTime,
							m_SynchronizedTime
							));
						m_CurrentPosition.node = CurrSeg.m_WayPos.node;
						m_CurrentPosition.pos = CurrSeg.m_BegPos;
						m_CurrentTangent = CurrSeg.m_WayPos.pos-CurrSeg.m_BegPos;
					}
					else
					{
						float CurrSegDuration = (float)(PreciseSubtract((CurrSeg.m_EndTime), (CurrSeg.m_BegTime)));

						if (!IsPositive(CurrSegDuration))
						{
							m_CurrentPosition.node = CurrSeg.m_WayPos.node;
							m_CurrentPosition.pos = CurrSeg.m_BegPos;
							m_CurrentTangent = CurrSeg.m_WayPos.pos-CurrSeg.m_BegPos;
							m_CurrentVelocity = 0.0f;
						}
						else
						{

							float CurrSegTime = (float)(PreciseSubtract(m_SynchronizedTime, (CurrSeg.m_BegTime)));

							if (CurrSegTime>CurrSegDuration)
							{
								/*********************
								if (m_SegmentQueue.size() == 1)
								{
									gpdevreportf( &gMCPSignalContext,(
										"GoFollower::Update() [%s:g:%08x] has moved past the last segment! (end_t:%f sync_t:%f)\n",
										GetGo()->GetTemplateName(),
										GetGo()->GetGoid(),
										CurrSeg.m_EndTime,
										m_SynchronizedTime
										));
								}
								*********************/
								m_CurrentPosition.node = CurrSeg.m_WayPos.node;
								m_CurrentPosition.pos = CurrSeg.m_EndPos;
								m_CurrentTangent = CurrSeg.m_EndPos-CurrSeg.m_WayPos.pos;
								m_CurrentVelocity = CurrSeg.m_EndVelocity;
							}
							else
							{

								tau = CurrSegTime/CurrSegDuration;

								m_CurrentPosition.node = CurrSeg.m_WayPos.node;
						
								if (IsZero(CurrSegTime))
								{
									m_CurrentPosition.node = CurrSeg.m_WayPos.node;
									m_CurrentPosition.pos = CurrSeg.m_BegPos;
									m_CurrentTangent = CurrSeg.m_WayPos.pos-CurrSeg.m_BegPos;
									// Leave the velocity set to whatever it was before this segment
								}
								else if (IsEqual(CurrSegTime,CurrSegDuration))
								{
									m_CurrentPosition.node = CurrSeg.m_WayPos.node;
									m_CurrentPosition.pos = CurrSeg.m_EndPos;
									m_CurrentTangent = CurrSeg.m_EndPos-CurrSeg.m_WayPos.pos;
									m_CurrentVelocity = CurrSeg.m_EndVelocity;
								}
								else
								{
									tau = CurrSegTime/CurrSegDuration;

									const vector_3& p0 = CurrSeg.m_BegPos;
									const vector_3& p1 = CurrSeg.m_WayPos.pos;
									const vector_3& p2 = CurrSeg.m_EndPos;

									vector_3 t0 = ((1-tau) * p0) + (tau * p1);
									vector_3 t1 = ((1-tau) * p1) + (tau * p2);
									vector_3 s0 = t1-t0;
									m_CurrentPosition.pos = t0 + tau*s0;
									if (!IsZero(s0,0.001f))
									{
										m_CurrentTangent = s0;
									}

									if (tau > 0.5f)
									{
										if (m_SegmentQueue.size()>1) 
										{
											m_CurrentVelocity = CurrSeg.m_EndVelocity;
										}
									}
									else
									{
										if (m_CurrentVelocity == 0.0) 
										{
											m_CurrentVelocity = CurrSeg.m_EndVelocity;
										}
									}
								}
							}
						}
					}
				}
			}

			if (!m_SignalsPending.empty())
			{
				// This is a bit of a pain, but we need to check to
				// see if updating to the new position will put us
				// in a new node that a signal is waiting for. UGH!

				SiegePos probe = m_CurrentPosition;	
				gSiegeEngine.AdjustPointToTerrain(probe);

				trigger::SignalIter psig;
				
				for (psig = m_SignalsPending.begin(); psig != m_SignalsPending.end();)
				{
					// HACKAHACKA comparison using 32bit value stuffed into structure
					if ((probe.node.GetValue() == (*psig).m_Data) || ((*psig).m_Time <= m_SynchronizedTime) )
					{
						// Restore the data value to the follower owner
						(*psig).m_Data = MakeInt(GetGo()->GetGoid());
						(*psig).m_Time = m_SynchronizedTime;

						gTriggerSys.SignalTrigger((*psig));

						gpdevreportf( &gMCPSignalContext,("At time %8.3f, [%s:0x%08x] %s (on time!) a node indicated by trigger[0x%08x]\n",
							m_SynchronizedTime,
							GetGo()->GetTemplateName(),	GetGo()->GetGoid(),
							trigger::IsEnteringAtCrossing((*psig).m_CrossType)?"ENTERS":"LEAVES",
							(*psig).m_TriggerID));

						psig = m_SignalsPending.erase(psig);
					}
					else
					{
						 ++psig;
					}
				}
			}

			// Decide if we should keep this segment or move on to the next one.
			bool keeper = false;

			if (CurrSeg.IsPrepared())
			{
				keeper = m_SegmentQueue[0].m_EndTime>=m_SynchronizedTime;
			}
			else
			{
				if (m_SegmentQueue.size()>1)
				{
					keeper = PreciseMultiply((PreciseAdd(m_SegmentQueue[0].m_WayTime, m_SegmentQueue[1].m_WayTime)), 0.5f) > m_SynchronizedTime;
					gpdevreportf( &gMCPNetLogContext,(
						"GoFollower::Update() [%s:g:%08x] Deleting first unprepped segment! (st:%f wt:%f)\n",
						GetGo()->GetTemplateName(),
						GetGo()->GetGoid(),
						m_SegmentQueue[0].m_WayTime,
						m_SynchronizedTime
						));
				}
				else 
				{
					// Even though it's not prepared, it's the only thing we have!
					keeper = true; // m_SegmentQueue[0].m_WayTime > m_SynchronizedTime;
					gpdevreportf( &gMCPNetLogContext,(
						"GoFollower::Update() [%s:g:%08x] Keeping the last unprepped segment! (st:%f wt:%f)\n",
						GetGo()->GetTemplateName(),
						GetGo()->GetGoid(),
						m_SegmentQueue[0].m_WayTime,
						m_SynchronizedTime
						));
				}
			}

			if (keeper)
			{
				// We will remain in this segment
				break;
			}

			m_SegmentQueue.pop_front();
			
		}
	
	}
		
	if (m_CurrentPosition.node == siege::UNDEFINED_GUID)
	{
		Quat current_orient(DoNotInitialize);

		if (m_LockedMovementGoid == GOID_INVALID)
		{
			m_CurrentPosition = GetGo()->GetPlacement()->GetPosition();
			current_orient = GetGo()->GetPlacement()->GetOrientation();
		}
		else
		{
			// This is an attached object, like the GRS torso, it get its position from 
			// whatever it is locked on to.
			if (IsValid(m_LockedMovementGoid))
			{
				m_CurrentPosition = ::GetGo(m_LockedMovementGoid)->GetPlacement()->GetPosition();
				current_orient = ::GetGo(m_LockedMovementGoid)->GetPlacement()->GetOrientation();
			}
			else
			{
				m_CurrentPosition = GetGo()->GetPlacement()->GetPosition();
				current_orient =GetGo()->GetPlacement()->GetOrientation();
			}
		}

		if (current_orient == Quat::ZERO)
		{
			m_CurrentOrientation = SiegeRot(Quat::IDENTITY,m_CurrentPosition.node);
		}
		else
		{
			m_CurrentOrientation = SiegeRot(current_orient,m_CurrentPosition.node);
		}

		if (m_OrientationGoal.rot == Quat::ZERO)
		{
			m_OrientationGoal = m_CurrentOrientation;
			m_HasReachedOrientationGoal = true;
		}
		
		m_CurrentVelocity = 0.0f;
	}
	else
	{
		if (m_LockedMovementGoid == GOID_INVALID)
		{
 			if ( gSiegeEngine.IsNodeInAnyFrustum(m_CurrentPosition.node) || IsServerLocal() )
 			{
 				GetGo()->GetPlacement()->ForceSetPosition(m_CurrentPosition);
 			}
 			else
 			{
 				gTattooGame.TeleportPlayer(GetGo()->GetPlayerId(), m_CurrentPosition, false);
 			}
		}
		else
		{
			// This object is locked to another so (like the GRS torso) so it get its position/orientation from 
			// whatever it is locked on to.
			if (IsValid(m_LockedMovementGoid))
			{
				GoPlacement* movingplace = ::GetGo(m_LockedMovementGoid)->GetPlacement();
				GoPlacement* lockedplace = GetGo()->GetPlacement();
				m_CurrentPosition  = movingplace->GetPosition();
				if (!lockedplace->DoesInheritPlacement())
				{
					lockedplace->ForceSetPosition(m_CurrentPosition);
				}
			}
		}
	}

#if !GP_RETAIL
	if( gWorldOptions.GetDebugMCP() && IsServerLocal() && (m_LockedMovementGoid == GOID_INVALID))
	{
		MCP::Plan * plan = gMCPManager.FindPlan( GetGoid(), false );
		if( plan )
		{
			double fdt = plan->GetFinalDestinationTime();
			if (fdt == DBL_MAX || fdt < m_SynchronizedTime)
			{
				const SiegePos& cp = m_CurrentPosition;
				const SiegePos& fd = plan->GetFinalDestination();
				if (!IsEqual(cp,fd,0.01f))
				{
				gperrorf(( 
					"KNOWN ISSUE: At time %f [%s:0x%08x] is not where MCP thinks it is. "
					"You can ignore this error, though it may result in an "
					"endless melee situation if one of the combatants isn't in the correct position."
					"Please log this occurrence in raid ONLY if you can easily reproduce this situation."						
					"\n\tACTUAL   [%12.6f,%12.6f,%12.6f,%s]"
					"\n\tEXPECTED [%12.6f,%12.6f,%12.6f,%s]\n" 
					"\nteleport n:%f,%f,%f,%s"
					"\nteleport n:%f,%f,%f,%s",
					m_SynchronizedTime,
					GetGo()->GetTemplateName(),	GetGo()->GetGoid(),
					cp.pos.x,cp.pos.y,cp.pos.z,cp.node.ToString().c_str(),
					fd.pos.x,fd.pos.y,fd.pos.z,fd.node.ToString().c_str(),
					cp.pos.x,cp.pos.y,cp.pos.z,cp.node.ToString().c_str(),
					fd.pos.x,fd.pos.y,fd.pos.z,fd.node.ToString().c_str()
					) );
				}
			}
		}
	}
#endif

	bool valid_current_node = gSiegeEngine.IsNodeInAnyFrustum(m_CurrentPosition.node)!=NULL;

	if ( valid_current_node && (m_OrientationMode != MCP::ORIENTMODE_UNDEFINED))
	{
		// We may need to update the orientation (after a node change)
		if (m_CurrentOrientation.node != m_CurrentPosition.node )
		{
			if ( gSiegeEngine.IsNodeInAnyFrustum(m_CurrentPosition.node) )
			{
				if ( gSiegeEngine.IsNodeInAnyFrustum(m_CurrentOrientation.node) )
				{
					m_CurrentOrientation.rot = GetCurrentOrientationRelativeToNode(m_CurrentPosition.node);
				}
				else
				{
					gpassertf( (m_CurrentOrientation.node == siege::UNDEFINED_GUID) || !IsServerLocal(),
						("ERROR: GoFollower::Update() CurrentOrientation not in any frustum"));
					valid_current_node = false;
				}
			}
			m_CurrentOrientation.node = m_CurrentPosition.node;
		}
	}

	if (valid_current_node) 
	{

	  //$$$$$$
	  //$$$$$$ Orientation maintenance section
	  //$$$$$$

		// For most orientation mode, we do modify the GoPlacement orientation
		bool orient_changed = true;

		// Allow any rotation amount as the default
		float ANGULAR_DELTA = PI/delta_time;

		// Now we have to update the orientation
		switch (m_OrientationMode)
		{
			case MCP::ORIENTMODE_LOCK_TO_HEADING:
			case MCP::ORIENTMODE_LOCK_TO_HEADING_REVERSE:
			{
				if (!IsZero(m_CurrentTangent.x,0.001f) || !IsZero(m_CurrentTangent.z,0.001f))
				{
					if (m_OrientationMode == MCP::ORIENTMODE_LOCK_TO_HEADING_REVERSE)
					{
						QuatFromXZComponents(-m_CurrentTangent.x,-m_CurrentTangent.z,m_OrientationGoal.rot);
					}
					else
					{
						QuatFromXZComponents(m_CurrentTangent.x,m_CurrentTangent.z,m_OrientationGoal.rot);
					}

					m_OrientationGoal.node = m_CurrentPosition.node;

					Quat tempq;
					m_HasReachedOrientationGoal = SlerpTowards(tempq,m_CurrentOrientation.rot,m_OrientationGoal.rot,ANGULAR_DELTA);
					m_CurrentOrientation = SiegeRot(tempq,m_CurrentPosition.node);

				}
				else
				{
					if ( m_OrientationGoal.node == siege::UNDEFINED_GUID )
					{
						m_OrientationGoal = m_CurrentOrientation;
					}
					m_HasReachedOrientationGoal = true;
				}

				m_HasReachedOrientationGoal = true;
				break;

			}

			case MCP::ORIENTMODE_FIXED_DIRECTION:
			{

				gpassert(!(m_OrientationGoal == SiegeRot::INVALID));

				Quat tempq;
				if (gSiegeEngine.IsNodeInAnyFrustum(m_OrientationGoal.node))
				{
					if (m_OrientationGoal.node != m_CurrentPosition.node )
					{
						Quat targq = NodeRotationDifferenceQuat( m_OrientationGoal.node , m_CurrentPosition.node ) * m_OrientationGoal.rot;
						m_HasReachedOrientationGoal = SlerpTowards(tempq,m_CurrentOrientation.rot,targq,ANGULAR_DELTA);
					}
					else
					{
						m_HasReachedOrientationGoal = SlerpTowards(tempq,m_CurrentOrientation.rot,m_OrientationGoal.rot,ANGULAR_DELTA);
					}
					m_CurrentOrientation = SiegeRot(tempq,m_CurrentPosition.node);
				}

				break;
			}

			case MCP::ORIENTMODE_FIXED_POSITION:
			case MCP::ORIENTMODE_FIXED_POSITION_REVERSE:
			{

				if ( gSiegeEngine.IsNodeInAnyFrustum(m_OrientationTrackPos.node) )
				{
					vector_3 diff = gSiegeEngine.GetDifferenceVector(m_CurrentPosition,m_OrientationTrackPos);
					if (m_OrientationMode == MCP::ORIENTMODE_FIXED_POSITION_REVERSE)
					{
						diff.x = -diff.x;
						diff.z = -diff.z;
					}

					if (!IsZero(diff.x) || !IsZero(diff.z))
					{
						QuatFromXZComponents(diff.x,diff.z,m_OrientationGoal.rot);
					}
					else
					{
						m_OrientationGoal.rot = Quat::IDENTITY;
					}
					m_OrientationGoal.node = m_CurrentOrientation.node;
					Quat tempq;
					m_HasReachedOrientationGoal = SlerpTowards(tempq,m_CurrentOrientation.rot,m_OrientationGoal.rot,ANGULAR_DELTA);
					m_CurrentOrientation = SiegeRot(tempq,m_CurrentPosition.node);
				}
				break;
			}

			case MCP::ORIENTMODE_TRACK_OBJECT:
			case MCP::ORIENTMODE_TRACK_OBJECT_REVERSE:
			{

				// If we are tracking object, then let the turning velocity dictate
				// how fast we can rotate to face
				ANGULAR_DELTA = GetGo()->GetBody()->GetAngularTurningVelocity();

				GoHandle tobj( m_OrientationTrackObj );

				if( tobj.IsValid() )
				{
					SiegePos tpos = tobj->GetPlacement()->GetPosition();

					if (gSiegeEngine.IsNodeInAnyFrustum(tpos.node)) {

						vector_3 diff = gSiegeEngine.GetDifferenceVector(m_CurrentPosition,tpos);
						if (m_OrientationMode == MCP::ORIENTMODE_TRACK_OBJECT_REVERSE)
						{
							diff.x = -diff.x;
							diff.z = -diff.z;
						}

						if (!IsZero(diff.x) || !IsZero(diff.z))
						{
							QuatFromXZComponents(diff.x,diff.z,m_OrientationGoal.rot);
						}
						else
						{
							m_OrientationGoal.rot = Quat::IDENTITY;
						}

						m_OrientationGoal.node = m_CurrentPosition.node;
						Quat tempq;
						bool reachedgoal = SlerpTowards(tempq,m_CurrentOrientation.rot,m_OrientationGoal.rot,ANGULAR_DELTA);
						m_CurrentOrientation = SiegeRot(tempq,m_CurrentPosition.node);

						if( IsServerLocal() )
						{
							if (reachedgoal && !m_HasReachedOrientationGoal )
							{
								WorldMessage( WE_MCP_FACING_LOCKEDON, GOID_INVALID, GetGoid(), (DWORD)MCP::RID_OVERRIDE ).SendDelayed(0);
								m_HasReachedOrientationGoal = true;
							}
							else if (!reachedgoal && m_HasReachedOrientationGoal )
							{
								WorldMessage( WE_MCP_FACING_UNLOCKED, GOID_INVALID, GetGoid(), (DWORD)MCP::RID_OVERRIDE ).SendDelayed(0);
								m_HasReachedOrientationGoal = false;
							}
						}

						orient_changed = true;

					}
					else
					{
						// $$$ The object we are trying to track has left the world !
						m_OrientationMode = MCP::ORIENTMODE_UNDEFINED;
						m_OrientationTrackObj = GOID_INVALID;
						m_OrientationGoal = m_CurrentOrientation;
						if( IsServerLocal() && m_HasReachedOrientationGoal  )
						{
							WorldMessage( WE_MCP_FACING_UNLOCKED, GOID_INVALID, GetGoid(), (DWORD)MCP::RID_OVERRIDE ).SendDelayed(0);
						}
						m_HasReachedOrientationGoal = true;
					}

				}
				else
				{
					if( IsServerLocal() )
					{
						// The object we were tracking is gone
						WorldMessage( WE_MCP_INVALIDATED, GOID_INVALID, GetGoid(), (DWORD)MCP::RID_OVERRIDE ).SendDelayed(0);
					}
					m_HasReachedOrientationGoal = true;
					m_OrientationGoal = m_CurrentOrientation;
					m_OrientationMode = MCP::ORIENTMODE_UNDEFINED;
				}

				break;

			}

			case MCP::ORIENTMODE_UNDEFINED:
			{
				// Assume that we AREN'T changing the orientation
				orient_changed = false;

				const Quat& current_orient = GetGo()->GetPlacement()->GetOrientation();
				if (current_orient == Quat::ZERO)
				{
					m_CurrentOrientation = SiegeRot(Quat::IDENTITY,m_CurrentPosition.node);
					orient_changed = true;
				}
				else
				{
					m_CurrentOrientation = SiegeRot(current_orient,m_CurrentPosition.node);
				}
				m_OrientationGoal = m_CurrentOrientation;
				m_HasReachedOrientationGoal = true;
				break;
			}

			default:
			{
				gperrorf(("GoFollower::Update() [%s:g:%08x] with unknown orientation mode [%08x]",
					GetGo()->GetTemplateName(),
					GetGo()->GetGoid(),
					m_OrientationMode
					));
			}

		}

		gpassert(!(m_CurrentOrientation == SiegeRot::INVALID) && (m_CurrentOrientation.rot != Quat::ZERO));

		if (orient_changed)
		{
			GetGo()->GetPlacement()->SetOrientation(GetCurrentOrientation());
		}

	}

// new assert for the running and fighting gom!
#if !GP_RETAIL
	GoMind * mind = GetGo()->GetMind();
	if (mind)
	{
		Job * action = mind->GetFrontJob( JQ_ACTION );
		if( action )
		{
			eJobAbstractType currentBaseJat = GetBaseJat( action->GetJobAbstractType() );
			if( (currentBaseJat >= JAT_ATTACK_OBJECT && currentBaseJat <= JAT_ATTACK_POSITION_RANGED) 
				|| currentBaseJat == JAT_CAST || currentBaseJat == JAT_CAST_POSITION )
			{
				MCP::Plan * plan = gMCPManager.FindPlan( GetGoid(), false );
				eAnimChore lastchore = plan ? plan->GetLastChore() : CHORE_INVALID;

				if ( lastchore == CHORE_WALK )
				{
					gperrorf(( 
						"RUNNING FIGHT?: At time %f, [%s:0x%08x] thinks its attacking, "
						"last chore is [%s] (not [CHORE_ATTACK])."
						"You can safely ignore this error, though it _MAY_ result in an "
						"actor that will appear to be paused or frozen while in combat."
						"Please watch carefully when play resumes for a \"running fighter\"",
						m_SynchronizedTime,
						GetGo()->GetTemplateName(),
						GetGo()->GetGoid(),
						::ToString(lastchore)
						));
				}


			}
		}
	}
	
#endif	
	
	// $$ commented out because these tests are no longer needed. mike wants to
	// keep them around so he can turn them back on in case he needs them again.
	// -sb
#if 0 && !GP_RETAIL
	if( gWorldOptions.GetDebugMCP() && IsServerLocal() )
	{
		if (!IsZero(GetCurrentVelocity()) && (GetGo()->GetAspect()->GetAspectHandle()->GetNextChore() != CHORE_WALK))
		{
			gperrorf(( 
				"KNOWN ISSUE: At time %f, [%s:0x%08x] has velocity %8.5f, but it's next chore is [%s] and not [chore_walk]. "
				"You can ignore this error, though it may result in an "
				"actor that will appear to be skating to it's destination."
				"Please log this occurrence in raid ONLY if you can easily reproduce this situation.",
				m_SynchronizedTime,
				GetGo()->GetTemplateName(),
				GetGo()->GetGoid(),
				GetCurrentVelocity(),
				::ToString(GetGo()->GetAspect()->GetAspectHandle()->GetNextChore())
				));
		}

		// Does our mind think we are attacking while our chore is fidget?
		else
		{
			GoMind * mind = GetGo()->GetMind();
			if (mind)
			{
				Job * action = mind->GetFrontJob( JQ_ACTION );
				if( action )
				{
					eJobAbstractType currentBaseJat = GetBaseJat( action->GetJobAbstractType() );
					if( currentBaseJat == JAT_ATTACK_OBJECT )
					{
						if (GetGo()->GetAspect()->GetAspectHandle()->GetNextChore() == CHORE_FIDGET)
						{
							MCP::Plan * plan = gMCPManager.FindPlan( GetGoid(), false );
							eAnimChore lastchore = plan ? plan->GetLastChore() : CHORE_INVALID;
							if( lastchore == CHORE_ATTACK)
							{
/*								gperrorf(( 
									"KNOWN ISSUE: At time %f, [%s:0x%08x] thinks its attacking. "
									"While the last chore is [CHORE_ATTACK] the next chore is [CHORE_FIDGET]."
									"You can ignore this error, though it may result in an "
									"actor that will appear to be paused MOMENTARILY in combat.",
									m_SynchronizedTime,
									GetGo()->GetTemplateName(),
									GetGo()->GetGoid()
									));
*/							}
							else
							{
								gperrorf(( 
									"FIDGET FIGHT?: At time %f, [%s:0x%08x] thinks its attacking, "
									"but it's next chore is [CHORE_FIDGET] and the last chore is [%s] (not [CHORE_ATTACK)."
									"You can safely ignore this error, though it _MAY_ result in an "
									"actor that will appear to be paused or frozen while in combat."
									"Please watch carefully when play resumes for a \"fidgeting fighter\"",
									m_SynchronizedTime,
									GetGo()->GetTemplateName(),
									GetGo()->GetGoid(),
									::ToString(lastchore)
									));
							}
						}
					}
				}
			}
		}				
	}
#endif
}


/////////////////////////////////////////////////////////////
// GoFollower GetCurrentPosition
const SiegePos& GoFollower::GetCurrentPosition(void) const
{
	if (!this)
	{
		gpfatal("CATASTROPHE! PathFollower is NULL");
	}

	return m_CurrentPosition;
}

/////////////////////////////////////////////////////////////
// GoFollower GetCurrentOrientation
const Quat& GoFollower::GetCurrentOrientation(void) const
{
	return m_CurrentOrientation.rot;
}

/////////////////////////////////////////////////////////////
// GoFollower GetCurrentOrientationSiegeRot
const SiegeRot& GoFollower::GetCurrentOrientationSiegeRot(void) const
{
	return m_CurrentOrientation;
}

/////////////////////////////////////////////////////////////
// GoFollower GetCurrentOrientationRelativeToNode
Quat GoFollower::GetCurrentOrientationRelativeToNode(const siege::database_guid& relNode) const
{

	if (m_CurrentOrientation.node != relNode)
	{
		return NodeRotationDifferenceQuat( m_CurrentOrientation.node , relNode ) * m_CurrentOrientation.rot;
	}

	return m_CurrentOrientation.rot;
}


/////////////////////////////////////////////////////////////
// GoFollower GetCurrentTangentVector
vector_3 GoFollower::GetCurrentTangentVector(void)
{
	return m_CurrentTangent;
}

/////////////////////////////////////////////////////////////
// GoFollower GetCurrentNormalVector
vector_3 GoFollower::GetCurrentNormalVector(void)
{

	// $$ Calculate the 'real' normvec
	return vector_3(-m_CurrentTangent.z,0,m_CurrentTangent.x);

}


/////////////////////////////////////////////////////////////
// GoFollower GetCurrentVelocity
float GoFollower::GetCurrentVelocity() const
{ 
	DEBUG_FPU_STACK_VALIDATE(1);
	return IsPositive(m_CurrentVelocity,0.01f) ? m_CurrentVelocity: 0.0f; 
}

/////////////////////////////////////////////////////////////
// GoFollower GetChoreAtTime
eAnimChore GoFollower::GetChoreAtTime(double t)
{
	if( !IsServerLocal() )
	{
		return CHORE_INVALID;
	}
	else
	{
		if (m_SegmentQueue.empty()) 
		{
			// If the queue is empty, return the current chore
			return GetGo()->GetAspect()->GetAspectHandle()->GetCurrentChore();
		}
		
		PlanSegQIter beg = m_SegmentQueue.begin();
		PlanSegQIter it = m_SegmentQueue.end();

		do 
		{
			--it;
			if ((*it).HasChore() && ((*it).m_WayTime <= t))
			{
				return (*it).m_Chore;
			}
		}
		while (it != beg);

		return GetGo()->GetAspect()->GetAspectHandle()->GetCurrentChore();

	}
}

/////////////////////////////////////////////////////////////
// GoFollower GetPositionAtTime
void GoFollower::GetPositionAtTime(double t, SiegePos& cp)
{
	if( !IsServerLocal() )
	{
		cp = m_CurrentPosition;
	}
	else
	{
		if (!m_SegmentQueue.empty()) 
		{
			PlanSegQIter beg = m_SegmentQueue.begin();
			PlanSegQIter it = m_SegmentQueue.end();

			do 
			{
				--it;
				if ((*it).HasPosition() && ((*it).m_WayTime <= t))
				{
					cp = (*it).m_WayPos;
					return;
				}
			}
			while (it != beg);
		}
		cp = m_CurrentPosition;
	}
}

/////////////////////////////////////////////////////////////
// GoFollower GetOrientationAtTime
void GoFollower::GetOrientationAtTime(double t, SiegeRot& cr)
{
	// I am not going to bother to get the orientation 'perfect', just point
	// them the right way if they are walking

	if (  (m_SegmentQueue.size() > 1) &&
		  ( (m_OrientationMode == MCP::ORIENTMODE_LOCK_TO_HEADING) ||
		    (m_OrientationMode == MCP::ORIENTMODE_LOCK_TO_HEADING_REVERSE) ) )
	{
		PlanSegQIter beg = m_SegmentQueue.begin();
		PlanSegQIter it = m_SegmentQueue.end();

		do 
		{
			--it;
			if ((*it).HasPosition() && ((*it).m_WayTime <= t))
			{
				break;
			}
		}
		while (it != beg);

		PlanSegQIter next = it;
		++next;

		vector_3 currtan(DoNotInitialize);

		if (next == m_SegmentQueue.end())
		{
			next = it;
			--it;
			cr.node = (*next).m_WayPos.node;
			currtan = -gSiegeEngine.GetDifferenceVector((*next).m_WayPos,(*it).m_WayPos);
		}
		else
		{
			cr.node = (*it).m_WayPos.node;
			currtan = gSiegeEngine.GetDifferenceVector((*it).m_WayPos,(*next).m_WayPos);
		}

		if (m_OrientationMode == MCP::ORIENTMODE_LOCK_TO_HEADING)
		{
			QuatFromXZComponents(currtan.x,currtan.z,cr.rot);
		}
		else if (m_OrientationMode == MCP::ORIENTMODE_LOCK_TO_HEADING_REVERSE)
		{
			QuatFromXZComponents(-currtan.x,-currtan.z,cr.rot);
		}

		return;
	}

	// We didn't get the facing orientation, so we just use whatever it there
	cr = m_CurrentOrientation;
}

/////////////////////////////////////////////////////////////
// GoFollower GetChoreParamsAtTime
void GoFollower::GetChoreParamsAtTime(double t, eAnimChore& ac, eAnimStance& as, DWORD &sa, DWORD &fl)
{
	if( !IsServerLocal() )
	{
		ac = CHORE_INVALID;
		as = AS_DEFAULT;
		sa = 0;
		fl = 0;
	}
	else
	{
		if (!m_SegmentQueue.empty()) 
		{
			PlanSegQIter beg = m_SegmentQueue.begin();
			PlanSegQIter it = m_SegmentQueue.end();

			do 
			{
				--it;
				if ((*it).HasChore() && ((*it).m_WayTime <= t))
				{
					ac = (*it).m_Chore;
					as = (*it).m_Stance;
					sa = (*it).m_SubAnim;
					fl = (*it).m_Flags;
					return;
				}
			}
			while (it != beg);
		}

		// If the queue has no chore, return the next chore in the nema_aspect

		nema::Aspect* pAsp = GetGo()->GetAspect()->GetAspectPtr();

		ac = pAsp->GetNextChore();
		as = pAsp->GetNextStance();
		sa = pAsp->GetNextSubAnim();
		fl = pAsp->GetNextFlags();

		return;

	}
}

/////////////////////////////////////////////////////////////
// GoFollower FollowerToNet
DWORD GoFollower::GoFollowerToNet( GoFollower* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

/////////////////////////////////////////////////////////////
// GoFollower NetToFollower
GoFollower* GoFollower::NetToGoFollower( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetFollower() : NULL );
}


#if !GP_RETAIL
void PackedUpdateToFollowersXfer::Xfer( FuBi::BitPacker& packer )
{
	MCP::TimeNode tn;
	
	DWORD num_packed_nodes = 0;

	packer.XferCount(num_packed_nodes);

	m_TimeNodes.resize(num_packed_nodes);

	double currenttime = 0.0; // $$ Track down 'correct' current time

	for (DWORD i=0;i<num_packed_nodes;++i)
	{
		tn.XferForSync(packer,currenttime);
		m_TimeNodes[i] = tn;
	}
};

void PackedUpdateToFollowersXfer::RpcToString( gpstring& appendHere ) const
{
	for (DWORD i = 0; i < m_TimeNodes.size();i++)
	{
		m_TimeNodes[i].Crack(i,appendHere);
	}
};

void PackedPositionUpdateToFollowersXfer::Xfer( FuBi::BitPacker& packer )
{
	packer.XferFloat(m_UpdatePosition.pos.x);
	packer.XferFloat(m_UpdatePosition.pos.y);
	packer.XferFloat(m_UpdatePosition.pos.z);
	packer.XferRaw(m_UpdatePosition.node);
};

void PackedPositionUpdateToFollowersXfer::RpcToString( gpstring& appendHere ) const
{
	appendHere.appendf(" Pos Update: [%8.3f,%8.3f,%8.3f,0x%08x]",
		m_UpdatePosition.pos.x,
		m_UpdatePosition.pos.y,
		m_UpdatePosition.pos.z,
		m_UpdatePosition.node);
};

void PackedClipToFollowersXfer::Xfer( FuBi::BitPacker& packer )
{
	packer.XferRaw(m_ClipTime);
	packer.XferFloat(m_ClipPos.pos.x);
	packer.XferFloat(m_ClipPos.pos.y);
	packer.XferFloat(m_ClipPos.pos.z);
	packer.XferRaw(m_ClipPos.node);
};

void PackedClipToFollowersXfer::RpcToString( gpstring& appendHere ) const
{
	appendHere.appendf(" Clip Time: %f Pos: [%8.3f,%8.3f,%8.3f,0x%08x]",
		m_ClipTime,
		m_ClipPos.pos.x,
		m_ClipPos.pos.y,
		m_ClipPos.pos.z,
		m_ClipPos.node);
};
#endif

#if !GP_RETAIL

/////////////////////////////////////////////////////////////
// GoFollower Dump
void GoFollower::Dump(void)
{
	//Threaded Dump of all segs/chores/orients
	PlanSegQIter beg = m_SegmentQueue.begin();
	PlanSegQIter end = m_SegmentQueue.end();
	PlanSegQIter it = beg;

	gpdevreportf( &gMCPContext,("[%8.3f] *follower* CST: %8.3f [%s] [%8.3f]\n",
		m_SynchronizedTime,
		m_CurrentStartTime,
		ToString(GetGo()->GetAspect()->GetAspectHandle()->GetNextChore()),
		m_CurrentVelocity));

	double prevtime = 0;

	double const ALL_NINES = 999999.999999f;
	
	int count = 0;

	while ( it != end )
	{
		gpdevreportf( &gMCPContext,("Node:%d %f\n",count++,(*it).m_WayTime));
		gMCPContext.Indent();
		if ((*it).HasPosition())
		{
			if ((int)(*it).m_ReqBlock < 0)
			{
				gpdevreportf( &gMCPContext,("Segment %s ",ToString((*it).m_ReqBlock)));
			}
			else
			{
				gpdevreportf( &gMCPContext,("Segment 0x%08x ",(*it).m_ReqBlock));
			}
			gpdevreportf( &gMCPContext,("%s "
										"EV %5.3f "
										"%8.3f [%8.3f,%8.3f,%8.3f, 0x%08x] "
										"%8.3f [%8.3f,%8.3f,%8.3f, 0x%08x] "
										"%8.3f [%8.3f,%8.3f,%8.3f, 0x%08x]\n",
										(*it).IsPrepared() ? "PREPPED" : "NO PREP",
										(*it).m_EndVelocity,					
										((*it).m_BegTime == DBL_MAX) ? ALL_NINES : (*it).m_BegTime,
										(*it).m_BegPos.x,
										(*it).m_BegPos.y,
										(*it).m_BegPos.z,
										(*it).m_WayPos.node,
										((*it).m_WayTime == DBL_MAX) ? ALL_NINES : (*it).m_WayTime,
										(*it).m_WayPos.pos.x,
										(*it).m_WayPos.pos.y,
										(*it).m_WayPos.pos.z,
										(*it).m_WayPos.node,
										((*it).m_EndTime == DBL_MAX) ? ALL_NINES : (*it).m_EndTime,
										(*it).m_EndPos.x,
										(*it).m_EndPos.y,
										(*it).m_EndPos.z,
										(*it).m_WayPos.node
						));


		}

		if ((*it).HasChore())
		{
			if ((int)(*it).m_ReqBlock < 0)
			{
				gpdevreportf( &gMCPContext,("Chore   %s ",ToString((*it).m_ReqBlock)));
			}
			else
			{
				gpdevreportf( &gMCPContext,("Chore   0x%08x ",(*it).m_ReqBlock));
			}
			gpdevreportf( &gMCPContext,("%f: %s %s %d 0x%08x %f\n",
								((*it).m_WayTime == DBL_MAX) ? ALL_NINES : (*it).m_WayTime,
								ToString((*it).m_Chore),
								ToString((*it).m_Stance),
								(*it).m_SubAnim,
								(*it).m_Flags,
								(*it).m_Range
						));
		}

		if ((*it).HasOrient())
		{
			if ((int)(*it).m_ReqBlock < 0)
			{
				gpdevreportf( &gMCPContext,("Orient  %s ",ToString((*it).m_ReqBlock)));
			}
			else
			{
				gpdevreportf( &gMCPContext,("Orient  0x%08x ",(*it).m_ReqBlock));
			}
			gpdevreportf( &gMCPContext,("%f: %s\n",
								((*it).m_WayTime == DBL_MAX) ? ALL_NINES : (*it).m_WayTime,
								ToString((*it).m_Mode)
						));
		}

		++it;
		prevtime = (it == end) ? DBL_MAX : ((*it).HasPosition()&&(*it).IsPrepared()) ? (*it).m_EndTime : (*it).m_WayTime  ;

		gMCPContext.Outdent();

	}

}

/////////////////////////////////////////////////////////////
// GoFollower DebugDraw
void GoFollower::DebugDraw(void)
{

	// Set the colour for all segments preceeding the current
	const DWORD COMPLETED_PATH_COLOUR = 0xffFFFF00;
	const DWORD FROZEN_PATH_COLOUR    = 0xffFF0000;

	vector_3 Up(0,0.15f,0);

	vector_3 vtan = 0.5f * m_CurrentTangent;
	vector_3 vnorm = vector_3(-vtan.z*0.5f ,0,vtan.x*0.5f);

	if (m_CurrentPosition.node.IsValidSlow()) {

		gSiegeEngine.GetNodeSpaceLines().DrawLine( m_CurrentPosition.pos + vnorm + Up , m_CurrentPosition.pos - vnorm + Up, 0xffFFFFFF, m_CurrentPosition.node );
		gSiegeEngine.GetNodeSpaceLines().DrawLine( m_CurrentPosition.pos + vtan  + Up , m_CurrentPosition.pos         + Up, 0xffFFFFFF, m_CurrentPosition.node );
	}

	PlanSegQIter beg = m_SegmentQueue.begin();
	PlanSegQIter end = m_SegmentQueue.end();
	PlanSegQIter it;
	PlanSegQIter nxt;

	for( it = beg,nxt=beg ; it!=end; it = nxt) {

		++nxt;
		if (it == beg)
		{
			(*it).DebugDraw(COMPLETED_PATH_COLOUR,Up,(nxt == end));
		}
		else
		{
			(*it).DebugDraw(FROZEN_PATH_COLOUR,Up,(nxt == end));
		}
		Up.y += 0.05f;
	}
}

#endif // !GP_RETAIL
