#pragma once
/*==================================================================================================


	MCP -- Master Control Program (Super Collider)

	purpose:	responsible for path collision detection, path maintainance
				path adjustments etc... of all non-static objects


	notes:
				Each GO (GOID) that we need to track is mapped to a PLAN structure that is in turn broken
				down into SECTIONS. 
				
				A PLAN is a connected sequence of SECTIONS, while a SECTION is an area of the 2-D plane that
				a GO occupies during a time interval. Another way to look at a SECTION is to consider it a bounding
				volume (in both space and time) that encloses a GO as it moves across the landscape

				A PLAN is analagous (well sort of...) to the J0B QUEUE structure in the AI.
				
				The expected behaviour of a GO is defined by the contents of its JOB QUEUE.
				The expected position of a GO is defined by the contents of its PLAN.

				A single JOB in the JOB QUEUE may result in several connected SECTIONs in the PLAN.

				Requests are made to the MCP to verify any changes to the PLAN whenever the JOB QUEUE is to be
				modified. This will allow the MCP to ensure that conflicts between PLANS are detected
				and handled in an appropriate manner before the JOB QUEUE is revised.



	author:	biddle

  (c)opyright Gas Powered Games, 2000

--------------------------------------------------------------------------------------------------*/
#ifndef __MCP_H
#define __MCP_H

#include "mcp_defs.h"
#include "mcp_request.h"
#include "godefs.h"
#include "space_2.h"
#include "siege_engine.h"
#include "choreographer.h"
#include "trigger_sys.h"
#include <map>

#define VALID_TIME(t) ( (t) == DBL_MAX || (t) == FLOAT_MAX || (t)<1000000 )

namespace MCP {

const float MINIMUM_DISTANCE_TOLERANCE         = 0.10f;
const float MINIMUM_DISTANCE_TOLERANCE_SQUARED = 0.01f;

enum eOrientMode
{
	SET_BEGIN_ENUM( ORIENTMODE_, 0 ),

	ORIENTMODE_UNDEFINED,
	ORIENTMODE_LOCK_TO_HEADING,
	ORIENTMODE_FIXED_DIRECTION,
	ORIENTMODE_FIXED_POSITION,
	ORIENTMODE_TRACK_OBJECT,
	ORIENTMODE_LOCK_TO_HEADING_REVERSE,
	ORIENTMODE_FIXED_POSITION_REVERSE,
	ORIENTMODE_TRACK_OBJECT_REVERSE,

	SET_END_ENUM( ORIENTMODE_ )
};

enum eReqFlag
{
	SET_BEGIN_ENUM( REQFLAG_, 0 ),

	REQFLAG_DEFAULT			= 0,
	REQFLAG_NOMOVE			= 1,
	REQFLAG_NOTURN			= 2,
	REQFLAG_NOMOVETURN		= 3,
	REQFLAG_FACEREVERSE		= 4,

	SET_END_ENUM( REQFLAG_ )
};

enum eRID
{
	SET_BEGIN_ENUM( RID_, -8 ),

	RID_UNDEFINED		= -8,
	RID_INTERNAL		= -7,	// UNUSED!
	RID_LOCKMOVE		= -6,
	RID_SYNC			= -5,
	RID_CLIP			= -4,
	RID_REFRESH			= -3,
	RID_DUMMYSAVED		= -2,
	RID_OVERRIDE		= -1,
	RID_BLOCKID			= 0,

	SET_END_ENUM( RID_ )
};

enum eLNodeStatus 
{
	LNODE_OK,
	LNODE_UNWALKABLE,
	LNODE_MISSING,
	LNODE_NO_TERRAIN
};

enum eDependancyType 
{
	SET_BEGIN_ENUM( DEPTYPE_, 0 ),
	DEPTYPE_PLAIN,
	DEPTYPE_MELEE_ENEMY,
	DEPTYPE_MELEE_FRIEND,
	DEPTYPE_PROJECTILE,
	SET_END_ENUM( DEPTYPE_ )
};


/*===========================================================================
	Struct	MCP::WayPoint
---------------------------------------------------------------------------*/

typedef std::list<SiegePos>				WayPointList;
typedef WayPointList::iterator			WayPointIter;
typedef WayPointList::const_iterator	WayPointConstIter;

/*===========================================================================
	Struct		: MCP::TimeNode
---------------------------------------------------------------------------*/

const DWORD HAS_POSITION_FLAG				= 1<<0;
const DWORD HAS_CHORE_FLAG					= 1<<1;
const DWORD HAS_ORIENT_FLAG					= 1<<2;
const DWORD HAS_SIGNAL_FLAG					= 1<<3;
const DWORD HAS_SEGMENT_PREPPED				= 1<<4;
const DWORD HAS_SEGMENT_BEGINPOS_PREPPED	= 1<<5;
const DWORD HAS_TRANSMITTED					= 1<<6;
const DWORD HAS_COMPLETED					= 1<<7;
const DWORD HAS_LOCKED_IN					= 1<<8;
const DWORD HAS_LOCKED_MOVEMENT_GO			= 1<<9;

struct TimeNode {

	TimeNode() 
	{
		Init();
		m_WayPos = SiegePos::INVALID;
	}

	explicit TimeNode(const SiegePos& p)
	{
		Init();
		m_WayPos = p;
	}

	bool Xfer( FuBi::PersistContext& persist );
	void XferForSync(FuBi::BitPacker &packer, double &start_time );
	void Crack(int count, gpstring &buff) const;

	void SetTime(double t)
	{
		m_Time = t;
	}

	void SetReqBlock(eRID rb)
	{
		m_ReqBlock = rb;
	}

	void SetPosition(float dist, const vector_3& delta,const SiegePos& pos)
	{
		m_Dist = dist;
		m_Delta = delta;
		m_WayPos = pos;
		m_Mask |= HAS_POSITION_FLAG;
	}

	void SetChore(eAnimChore ch,eAnimStance st ,DWORD sa , float rg, DWORD fl )
	{
		m_Chore = ch;
		m_Stance = st;
		m_SubAnim = sa;
		m_Range = rg;
		m_Flags = fl;
		m_Mask |= HAS_CHORE_FLAG;
	}

	void SetOrient(eOrientMode md)
	{
		m_Mode = md;
		m_Target = 0;
		m_TargPos = SiegePos::INVALID;
		m_TargRot = SiegeRot::INVALID;
		m_Mask |= HAS_ORIENT_FLAG;
	}

	void SetOrient(eOrientMode md, Goid tg)
	{
		m_Mode = md;
		m_Target = tg;
		m_TargPos = SiegePos::INVALID;
		m_TargRot = SiegeRot::INVALID;
		m_Mask |= HAS_ORIENT_FLAG;
	}

	void SetOrient(eOrientMode md, const SiegePos& tp)
	{
		m_Mode = md;
		m_Target = GOID_INVALID;
		m_TargPos = tp;
		m_TargRot = SiegeRot::INVALID;
		m_Mask |= HAS_ORIENT_FLAG;
	}

	void SetOrient(eOrientMode md, const SiegeRot& tr)
	{
		m_Mode = md;
		m_Target = GOID_INVALID;
		m_TargPos = SiegePos::INVALID;
		m_TargRot = tr;
		m_Mask |= HAS_ORIENT_FLAG;
	}

	void SetSignal(trigger::Truid trigger, trigger::eBoundaryCrossType crosstype, WORD condid, WORD seqnum)
	{
		m_Trigger = trigger;
		m_CrossType = crosstype;
		m_CondID = condid;
		m_SeqNum = seqnum; 
		m_Mask |= HAS_SIGNAL_FLAG;
	}

	void SetTransmitted()
	{
		m_Mask |= HAS_TRANSMITTED;
	}

	void SetCompleted()
	{
		m_Mask |= HAS_COMPLETED;
	}

	void SetLockedIn()
	{
		m_Mask |= HAS_LOCKED_IN;
	}

	void SetLockedMovementGo(Goid tg)
	{
		m_LockedMoveGo = tg;
		m_Mask |= HAS_LOCKED_MOVEMENT_GO;
	}

	bool HasPosition(void) const			{ return (m_Mask & HAS_POSITION_FLAG) != 0; }
	bool HasOrient(void) const				{ return (m_Mask & HAS_ORIENT_FLAG) != 0; }
	bool HasChore(void) const				{ return (m_Mask & HAS_CHORE_FLAG) != 0; }
	bool HasSignal(void) const				{ return (m_Mask & HAS_SIGNAL_FLAG) != 0; }
	bool HasTransmitted(void) const			{ return (m_Mask & HAS_TRANSMITTED) != 0; }
	bool HasCompleted(void) const			{ return (m_Mask & HAS_COMPLETED) != 0; }
	bool HasLockedIn(void) const			{ return (m_Mask & HAS_LOCKED_IN) != 0; }
	bool HasLockedMovementGo(void) const	{ return (m_Mask & HAS_LOCKED_MOVEMENT_GO) != 0; }

	WORD		m_Mask;

	double		m_Time;	

	// Request
	eRID		m_ReqBlock;
	
	// Position
	float		m_Dist;		// linear distance to next waypos
	vector_3	m_Delta;	// 3D delta next waypos
	SiegePos	m_WayPos;

	// Chore
	eAnimChore	m_Chore;
	eAnimStance	m_Stance;
	DWORD		m_SubAnim;
	float		m_Range;
	DWORD		m_Flags;

	// Orient
	eOrientMode m_Mode;
	Goid        m_Target;
	SiegePos    m_TargPos;
	SiegeRot    m_TargRot;

	// Signal (notify trigger that we have hit a boundary defined in condition)
	trigger::Truid              m_Trigger;
	trigger::eBoundaryCrossType m_CrossType; 
	WORD                        m_CondID; 
	WORD                        m_SeqNum; 

	// Locked movement go (when following something)
	Goid		m_LockedMoveGo;

	void DebugDraw(double t, DWORD color) const;

	// Predicate for sorting
	struct LessThan
	{
		bool operator () ( const TimeNode& l, const TimeNode& r )
		{
			return (l.m_Time < r.m_Time);
		}
	};

	// Function object for applying time changes
	struct TimeAdjust 
	{
		TimeAdjust(double s,double t) : m_scale(s),m_origin(t) {};
		void operator() (TimeNode& e) 
		{
			e.m_Time = PreciseAdd( PreciseMultiply( PreciseSubtract(e.m_Time, m_origin), m_scale), m_origin);
		}
	private:
		double m_scale;
		double m_origin;
	};

#if !GP_RETAIL
	void DebugDraw( Goid owner, DWORD colour, float width , const SiegePos& pos, const SiegeRot& rot ) const; 
	void Dump();
#endif

private:
	void Init()
	{
		m_Time = DBL_MAX;
		m_Mask = 0;
		m_ReqBlock = RID_UNDEFINED;
		m_Dist = 0;
		m_Delta = vector_3::ZERO;
		m_Chore = CHORE_INVALID;
		m_Stance = AS_DEFAULT;
		m_SubAnim = 0;
		m_Range = 0.0f;
		m_Flags = REQFLAG_DEFAULT;
		m_Mode = ORIENTMODE_UNDEFINED;
		m_Target = 0;
		m_TargPos = SiegePos::INVALID;
		m_TargRot = SiegeRot::INVALID;
		m_Trigger = trigger::Truid();
		m_CrossType = trigger::BCROSS_ENTER_AT_TIME;
		m_CondID = 0;
		m_SeqNum = 0;	
		m_LockedMoveGo = GOID_INVALID;
	};
};

typedef std::list<TimeNode>				TimeNodeList;
typedef TimeNodeList::iterator			TimeNodeIter;
typedef TimeNodeList::const_iterator	TimeNodeConstIter;

/*===========================================================================
	Struct		: MCP::Sequencer
---------------------------------------------------------------------------*/

class Sequencer 
{
public:
	
	Sequencer();

	bool Xfer( FuBi::PersistContext& persist );

	bool Initialize(eRID init_rb, double init_time, const SiegePos& init_pos, const SiegeRot& init_rot);

	bool Empty(void) const	{ return m_NodeList.empty(); }

	TimeNodeIter FindNodeAtOrBeforeTime(double t);
	TimeNodeIter FindNodeAtOrAfterTime(double t);

	void AdjustTimeline(TimeNodeIter sn, double scale, double shift);

	bool AppendWayPoints(eRID rb, double bt, WayPointList& wpl, const SiegePos& dest, float v,float ra, float et, siege::eLogicalNodeFlags p);
	bool AppendWayPointsWithArrivalTime(eRID rb, double bt, WayPointList& wpl, const SiegePos& dest, float v,float ra, double arrivet, siege::eLogicalNodeFlags p);
	bool AppendChore(eRID rb,double bt,eAnimChore ch,eAnimStance st, DWORD sa, DWORD fl, float ra);
	bool AppendOrient(eRID rb ,double bt,eOrientMode md );
	bool AppendOrient(eRID rb ,double bt,eOrientMode md, SiegePos p );
	bool AppendOrient(eRID rb ,double bt,eOrientMode md, SiegeRot r );
	bool AppendOrient(eRID rb ,double bt,eOrientMode md, Goid g );

	bool AppendTeleport(eRID rb ,const SiegePos& tgtPos, const SiegeRot& tgtRot, bool flushAll = false );

	bool AppendLockedMovementGo(eRID rb ,double bt, Goid g );
	
	bool GetLastChoreAndOrient(TimeNode& tn);
	bool PopLastChoreAndOrient(TimeNode& tn);
	bool SetLastChoreAndOrient(const TimeNode& tn);

	bool GetLastChoreAndStance(eAnimChore& slc, eAnimStance& sls) const;

	bool IsPositionConstant(void) const;

	double GetFinalTime(void) const { return m_NodeList.empty() ? DBL_MAX : m_NodeList.back().m_Time; }
	SiegePos GetFinalPosition(void) const;
	TimeNode GetFinalNode(void)		{ return m_NodeList.empty() ? TimeNode() : m_NodeList.back(); }

	bool GetPosition(double& t, bool no_interp, bool skip_locked, SiegePos& retval);
	bool GetNextValidPositionTime(double& t, SiegePos& retval, siege::eLogicalNodeFlags perms) const;

	eAnimChore GetLastChore(void) const;
	eRID GetLastReqBlock(void) const			{ return m_NodeList.empty() ? RID_UNDEFINED : m_NodeList.back().m_ReqBlock; }

	bool IsAllTransmitted(void) const			{ return m_NodeList.empty() ? true : m_NodeList.back().HasTransmitted();	}
	bool CheckForCompletion(double t);
	
	bool UsesNode(const siege::database_guid node) const;

	eReqRetCode EstimateInterceptPosition(
									double& contact_time,
									SiegePos& contact_pos,
									const SiegePos& src_pos,
									double initial_time,
									float velocity,
									float look_ahead,
									float range,
									float cushion
									);


	bool CheckForTriggerCollisions( Go* mover, TimeNodeIter tni);
	void AppendTriggerSignals(const trigger::SignalSet& sigset);

	// Compressed transmission stuff

	bool CollectTimeNodesToSend( Go* mover, double forward_time, std::vector<TimeNode> &out_ToSend );
	void PackRefresh( FuBi::BitPacker &inout_packer, GoFollower *follower, double ClientSynchTime );

	double GetLastTimeSent(void)		{ return m_LastTimeSent; }

	// TimeNode Clipping 
	double GetEarliestClipTime(const double& inout_ClipTime);
	double GetEarliestClipOrTransmittedTime(const double& inout_ClipTime);
	TimeNodeIter FindLastNodeToKeep(double& inout_ClipTime, bool &out_MustLock, bool &out_LastChoreInvalid, bool notTransmitted);
	bool ClipToTime(double& inout_AdjustedClipTime,siege::eLogicalNodeFlags perms, Go* owngo);
	bool UpdateAfterClipIsSent(double clip_time);

	bool VerifyPositionAtTime(const SiegePos& p,double t);

	// Set this plan up so that it tracks the position of another
	void SetLockedMovementGo(Goid targ)		{ m_LockedMovementGo = targ; }
	Goid GetLockedMovementGo() const		{ return m_LockedMovementGo; }

	const SiegePos& GetWayPosNodeTransmitted() const { return m_WayPosNodeTransmitted; }

#if !GP_RETAIL	
	void Validate(Go* own,double appendtime) const;
	void DebugDraw(double currtime,float draw_width) const;
	void Dump( ReportSys::ContextRef ctx ) const;
#else
	void Validate(Go* own,double appendtime) const {};
#endif

private:
	TimeNodeList	m_NodeList;
	double			m_LastTimeSent;
	eAnimChore		m_LastChoreTransmitted;
	eAnimStance		m_LastStanceTransmitted;
	DWORD			m_LastSubAnimTransmitted;
	DWORD			m_LastFlagsTransmitted;
	Goid			m_LockedMovementGo;
	SiegePos		m_WayPosNodeTransmitted;
};

/*===========================================================================
	Class		: Plan

	Notes:
		
---------------------------------------------------------------------------*/

class Plan
{

public:

	struct DependancyInfo
	{

		DependancyInfo() {};

		DependancyInfo(
			Goid own, 
			double time, 
			const SiegePos& required, 
			const SiegePos& reserved, 
			eDependancyType type
			);

		bool Xfer( FuBi::PersistContext& persist );

		Goid			m_Owner;
		double			m_Time;
		SiegePos		m_RequiredPosition;	
		float			m_RequiredDistance;
		SiegePos		m_ReservedPosition;
		eDependancyType	m_Type;					// Friend/Foe? Near/Far?

		#if !GP_RETAIL
		void Dump(const char* msg, int c);
		void DebugDraw(DWORD color) const;
		#endif

	};

	typedef std::list<DependancyInfo>		DependantDB;
	typedef DependantDB::iterator			DependantIter;
	typedef DependantDB::const_iterator		DependantConstIter;

	struct BlockingInfo
	{
		Plan*			m_Plan;
		SiegePos		m_Position;
		BlockingInfo*	m_FarInfo;
		float			m_BlockingAngle;
		float			m_AngleFromLocalZAxis;

		~BlockingInfo() { gpassert(m_FarInfo == NULL); }

		bool Xfer( FuBi::PersistContext& persist );

		struct LessThan
		{
			bool operator () ( const BlockingInfo* l, const BlockingInfo* r ) const
			{
				gpassert((DWORD)l->m_Plan == (DWORD)r->m_Plan);
				gpassert((DWORD)l->m_FarInfo->m_Plan->GetOwner() != (DWORD)r->m_FarInfo->m_Plan->GetOwner());
			   	return (l->m_AngleFromLocalZAxis == r->m_AngleFromLocalZAxis) ? 
					  ((DWORD)l->m_FarInfo->m_Plan->GetOwner() < (DWORD)r->m_FarInfo->m_Plan->GetOwner()) 
					: (l->m_AngleFromLocalZAxis < r->m_AngleFromLocalZAxis);
			}
		};

		struct OwnedBy
		{
			OwnedBy(Goid g) : owngoid(g) {};

			OwnedBy(Plan* p)
			{
				owngoid = p->GetOwner();
			};

			bool operator () (const BlockingInfo* bi) const
			{
				return (bi->m_Plan->GetOwner() == owngoid);
			}
			Goid owngoid;
		};

		struct PointsTo
		{
			PointsTo(Goid g) : fargoid(g) {};
			PointsTo(Plan *p) : fargoid(p->GetOwner()) {};
			PointsTo(BlockingInfo* bi) : fargoid(bi->m_Plan->GetOwner()) {};

			bool operator () (const BlockingInfo* bi) const
			{
				return (bi->m_FarInfo->m_Plan->GetOwner() == fargoid);
			}
			Goid fargoid;
		};

		#if !GP_RETAIL
		void DebugDraw(DWORD color) const;
		#endif

	};

	typedef std::set<BlockingInfo*,BlockingInfo::LessThan>	BlockerDB;
	typedef BlockerDB::iterator								BlockerIter;
	typedef BlockerDB::const_iterator						BlockerConstIter;

	Plan( Goid owner, double init_time );
	Plan()  {  }
	~Plan();

	bool Xfer( FuBi::PersistContext& persist );

	Goid GetOwner()	const				{ return m_Owner;	}

	eRID GetCurrentReqBlock() const		{ return (eRID)m_CurrentReqBlock;	}

	bool FlushAfterTime( double &cutoff_time );
	
	void SSendPackedUpdateToFollowers(double forward_time);

	void SSendPackedClipToFollowers(double clip_time, SiegePos pos);

	bool NeedsRefresh();
	void BuildPackedRefresh( FuBi::BitPacker& packer );

	SiegePos GetPosition(double& t, bool no_interp, bool skip_locked);
	double GetEarliestClipOrTransmittedTime(const double& inout_ClipTime)		{ return m_Sequencer.GetEarliestClipOrTransmittedTime(inout_ClipTime);	}
	
	float GetPathWidth() const					{ return m_PathWidth; }

	eReqRetCode AppendMove( const SiegePos& tgtPos, float range, float max_time, bool reversed, Goid tg );
	eReqRetCode AppendMoveHelper( const SiegePos& tgtPos, float range, float max_time, bool reversed, Goid tg );
	eReqRetCode AppendMoveToCrowd( Plan* tgtPlan, float range, bool reversed );
	eReqRetCode AppendMoveWithDestinationTime( const SiegePos& tgtPos, float range, double dest_time, bool reversed, Goid tg );
	eReqRetCode AppendCollisionMove( Plan* lhs_plan, float lhs_avgvel, float lhs_range, float rhs_avgvel, bool reversed );
	
	eReqRetCode AppendTeleport( const SiegePos& tgtPos, const SiegeRot& tgtRot, bool flushAll = false );

	bool ChangeMoveDestination( double changetime, const SiegePos& tgtPos, float avgvel, siege::eLogicalNodeFlags perms, Goid tg );

	void AppendChore( eAnimChore ch, eAnimStance stance, DWORD subanim, DWORD flags, float range );

	void AppendFaceTarget( Goid Targ, bool reversed, bool no_turn );
	void AppendFacePosition( const SiegePos& tpos, bool reversed );
	void AppendFaceRotation( const SiegeRot& trot );
	void AppendFaceHeading( bool reversed );
	void AppendFaceUndefined( );

	void AppendLockMovementToGo(Goid targ);

	eAnimChore GetLastChore() const  { return m_Sequencer.GetLastChore(); }

	void GetLastChoreAndStance(eAnimChore& slc,eAnimStance& sls) const;
	
	double GetAppendTime() const			{ return m_AppendTime;             }
	void   SetAppendTime(const double at)	{ m_AppendTime = at;               }

	void SavePendingRequest( eRequest type,float duration,Goid target,DWORD subanim,float range,DWORD flags);
	void CommitPendingRequest();
	void ClearPendingRequest();
	bool HasSavedLastRequest()				{ return m_LastRequestRID != RID_DUMMYSAVED; }
	DWORD GetSavedLastRequest()				{ return m_LastRequestRID; }

	eRequest GetLastRequestType()			{ return m_LastRequestType; }

	bool AllowCollisionPath() const;

	bool IsOnInterceptPath(Plan* attackPlan) const ;
	bool IsInMeleeCrowd() const ;

	bool FindPositionInCrowdAroundTarget( float range, Plan* tgtPlan, SiegePos& probe);

	bool ProbeForValidPositionInInterval(  Plan* prober,
										   float probeinitangle,
		                                   float range,
										   float minsep,
										   BlockingInfo* lhs,
										   BlockingInfo* rhs,
										   siege::eLogicalNodeFlags walkMask,
										   bool left_to_right,
										   SiegePos &probe
										   ) const;

	eReqRetCode EstimateInterceptPosition(
									double& contact_time,
									SiegePos& contact_pos,
									const SiegePos& src_pos,
									double initial_time,
									float velocity,
									float look_ahead,
									float range,
									float cushion
									);

	SiegePos GetFinalDestination()	const;
	double GetFinalDestinationTime() const;
	bool IsMoving() const;

	// Maintain dependancies between this plan and others that are linked to it

	void CreateDependancyLink(DependancyInfo& depinfo);
	void BreakDependancyLinkToChild(Goid child,bool sendmsg);
	void BreakDependancyLinkToParent();
	void BreakAllDependancyLinks(bool sendmsg);
	bool CheckAllDependancyLinks(bool sendmsg);
	bool CheckDependancyPosition(Goid child,const SiegePos& p, float range) const;
	bool CheckDependancyNode(Goid child, siege::database_guid n) const;

	Goid DependsUpon() const	{ return m_DepParent;	}
	int	 NumDependants() const	{ return m_DepChildren.size(); }

	int	 NumMeleeDependants(Goid excludeGoid);

	bool BuildDependancyInfo(DependancyInfo &di, Goid own, const SiegePos& reserve, eDependancyType dtype);

	bool FetchDelayedDependancyToBreak(Goid& k)	{ 
											k = m_DelayedDepToBreak;
											m_DelayedDepToBreak = GOID_INVALID;
											return (k != GOID_INVALID);
										}	

	// Maintain blocking information between this plan and others that may interfere with it
	
	bool CheckForBlockers(
		const SiegePos& dest,
		Goid approachingGoid,
		bool skipApproaching, 
		bool skipEnemies,
		bool skipDependants,
		bool skipDepsOfApproach
		);

	bool CheckForOccupied(Go* mover,const SiegePos& destpos) const;

	bool CreateBlockingLink(Plan* targPlan);
	bool RemoveBlockingLink(Plan* targPlan);
	bool RemoveAllBlockingLinks();

	int NumBlockersInCrowd(const Goid excludeGoid) const;

	bool UsesChangedNode(const siege::database_guid node) const;

	void AppendTriggerSignals(const trigger::SignalSet& sigset);
	
	////////////////////////////////////////////////////////////////////////////////
	//	debug

#	if !GP_RETAIL
	void DebugDraw( DWORD colour, double current_time ) const;
	bool DebugValidate() const;
	void Dump( ReportSys::ContextRef ctx = NULL );
	void SetGoalType(const char *s)			{ m_GoalType = s; }
	const gpstring& GetGoalType() const		{ return m_GoalType; }
#	endif // !GP_RETAIL


private:

	void SSendResetToFollowers();

	Goid		m_Owner;

	Sequencer	m_Sequencer;

    double		m_AppendTime;

	double		m_ClientSynchTime;

	float		m_PathWidth;

	bool		m_IsLockedOn;

	TimeNode	m_LastNodeSent;


	// Need to track blocks of request so that we can filter out message
	// generated by events in stale blocks that survive a plan flush
	DWORD		m_CurrentReqBlock;


	// Needed for mutual collision path re-creation
	eRID		m_LastRequestRID;
	eRequest	m_LastRequestType;
	float		m_LastRequestDuration;
	Goid		m_LastRequestTarget;
	DWORD		m_LastRequestSubAnim;
	float		m_LastRequestRange;
	DWORD		m_LastRequestFlags;

	eRID		m_PendingRequestRID;
	eRequest	m_PendingRequestType;
	float		m_PendingRequestDuration;
	Goid		m_PendingRequestTarget;
	DWORD		m_PendingRequestSubAnim;
	float		m_PendingRequestRange;
	DWORD		m_PendingRequestFlags;

	GoFollower*	m_Follower;

	Goid		m_LockedMovementGo;	// Used if this plan has it's position locked to another

	// Set up links between plans for intercepting/following/guarding etc.

	Goid		m_DepParent;		// plan that we depend upon
	DependantDB	m_DepChildren;		// plans that depend upon us

	Goid		m_DelayedDepToBreak;	// Used to create a single delayed dependancy message
										// rather than send those messages out inline

	BlockerDB	m_Blockers;			// other plans that surround us

#if !GP_RETAIL
	gpstring	m_GoalType;
#endif


};

/*===========================================================================
	Class		: Manager

  	Notes:
		A static container for PLANS

---------------------------------------------------------------------------*/
typedef std::map<Goid,Plan*>			PlanDb;
typedef PlanDb::iterator				PlanIter;
typedef PlanDb::const_iterator			PlanConstIter;

class Manager : public Singleton <Manager>
{
public:

	Manager();
	~Manager();

	void Reset();

	bool Xfer( FuBi::PersistContext& persist );

	void RemoveGo( const Goid goid ); 
	void UpdateGo( const Goid goid ); 
	void ResetGo( const Goid goid );
	void TeleportGo( const Goid goid, const SiegePos& p, const SiegeRot& r, bool flushAll = false );

	FUBI_EXPORT bool Flush( Goid owner )					{ return Flush(owner,0); };
	FUBI_EXPORT bool Flush( Goid owner , float delay );
	FUBI_EXPORT bool BeginTeleport( Goid owner );

	/////
	// The implementation of the MakeRequest calls is in the mcp_request.cpp file

	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type);
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, eReqFlag flags);

	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, Goid targetGo );
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, Goid targetGo, eReqFlag flags );

	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, Goid targetGo, float look_ahead, float max_time, float range );
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, Goid targetGo, float look_ahead, float max_time, float range, eReqFlag flags );

	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, SiegePos const & targetPos );
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, SiegePos const & targetPos, eReqFlag flags );

	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, SiegePos const & targetPos, float range );
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, SiegePos const & targetPos, float range, eReqFlag flags );

	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, SiegeRot const & targetRot );
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, SiegeRot const & targetRot, eReqFlag flags );

	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, DWORD subanim);
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, DWORD subanim, eReqFlag flags);

	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, DWORD subanim, float range );
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, DWORD subanim, float range, eReqFlag flags );

	//  missing variation
//	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, DWORD subanim, Goid targetGo, eReqFlag flags );

	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, DWORD subanim, Goid targetGo, float look_ahead, float max_time, float range );
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, DWORD subanim, Goid targetGo, float look_ahead, float max_time, float range, eReqFlag flags );

	//  missing variation
//	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, DWORD subanim, SiegePos const & targetPos, eReqFlag flags );

	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, DWORD subanim, SiegePos const & targetPos, float range );
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, DWORD subanim, SiegePos const & targetPos, float range, eReqFlag flags );

	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, float duration, Goid targetGo, float look_ahead, float max_time, float range);
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, float duration, Goid targetGo, float look_ahead, float max_time, float range, eReqFlag flags);

	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, float duration, DWORD subanim, Goid targetGo, float look_ahead, float max_time, float range);
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, float duration, DWORD subanim, Goid targetGo, float look_ahead, float max_time, float range, eReqFlag flags);

	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, float duration, const SiegePos& targetPos, float range );
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, float duration, const SiegePos& targetPos, float range, eReqFlag flags );

	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, float duration, DWORD subanim, SiegePos const & targetPos, float range );
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, float duration, DWORD subanim, SiegePos const & targetPos, float range, eReqFlag flags );

	// This is a 'special' request that allows you call for specific animations with specific flag settings
	FUBI_EXPORT eReqRetCode MakeRequest( Goid owner, eRequest type, eAnimChore ch, DWORD subanim, DWORD flags );

	FUBI_EXPORT bool ReconstructDependancy(Goid src, Goid tgt, float reqrange);

	// Need this to be able to test to ensure that a request have been transmitted
	FUBI_EXPORT bool MessagesAreInSync( Goid owner, eRID reqblock, eWorldEvent evt ) ;

	FUBI_EXPORT bool GetLastChoreRequestedIsStillValid( Goid owner, eAnimChore ch, eAnimStance st );

	FUBI_EXPORT float GetLastRequestTimeRemaining( Goid owner );

	void Update(void);

	void NotifyNodeStateChange(const siege::database_guid& node);
	void VerifyBlockedNodes(void);

	// Need allow the trigger system to tell us that we've signalled triggers (because the node we are in has moved)
	void AppendTriggerSignals(Goid owner, const trigger::SignalSet& sigset);

	double GetLaggingTime()					{ return m_LaggingTime; }
	double GetLeadingTime()					{ return m_LeadingTime; }
	double GetLagDelay()					{ return PreciseSubtract(m_LeadingTime,m_LaggingTime); }

	Plan * FindPlan( const Goid goid, bool autoCreate = true );


	////////////////////////////////////////////////////////////////////////////////
	//	debug

#	if !GP_RETAIL
	void DebugDraw();
FEX	void Dump( Goid go, ReportSys::Context * ctx );
#	endif // !GP_RETAIL


private:

	////////////////////////////////////////
	//	methods

	eReqRetCode FindPathTo			( Goid sourceGoid, const SiegePos& targetPos,	float range, bool reverse );
	eReqRetCode FindPathTo			( Goid sourceGoid, Goid targetGoid,				float range, bool reverse, eDependancyType dtype );
	eReqRetCode FindFollowPath		( Goid sourceGoid, Goid targetGoid,				float range, bool reverse );
	eReqRetCode FindPathAwayFrom	( Goid sourceGoid, Goid targetGoid,				float range, bool reverse );
	eReqRetCode FindInterceptPath	( Goid sourceGoid, Goid targetGoid,				float look_ahead, float max_time, float range, float cushion, bool synch_at_end, bool reverse, eDependancyType dtype );

	eReqRetCode FindCollisionPath   ( Plan* srcPlan, Plan* tgtPlan,	float range, bool reverse );
	eReqRetCode FindPathToCrowd		( Plan* srcPlan, Plan* tgtPlan,	float range, bool reverse );

	////////////////////////////////////////
	//	data 

    double         m_LaggingTime;   
    double         m_LeadingTime;   
    float          m_DeltaTime;     
    PlanDb         m_Plans; 
	
	stdx::fast_vector<siege::database_guid>	m_RecentlyChangedNodes;

	FUBI_SINGLETON_CLASS(MCP::Manager, "MCP manager");
};

#define gMCPManager MCP::Manager::GetSingleton()

}	// End of MCP namespace


const char* ToString( MCP::eOrientMode om );
bool FromString( const char* str, MCP::eOrientMode& om );

const char* ToString( MCP::eRID e );
bool FromString( const char* str, MCP::eRID& e );


/*===========================================================================
 Helpers
---------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////
// NodeRotationDifference for quaternions
inline Quat NodeRotationDifferenceQuat( const siege::database_guid& NodeA, const siege::database_guid& NodeB )
{
	matrix_3x3 OrientA( DoNotInitialize );
	matrix_3x3 OrientB( DoNotInitialize );
	matrix_3x3 result ( DoNotInitialize );

	gSiegeEngine.GetNodeOrientation( NodeA, OrientA );
	gSiegeEngine.GetNodeOrientation( NodeB, OrientB );

	Multiply( result, Transpose(OrientB), OrientA );

	return result;
}

/////////////////////////////////////////////////////////////
// NodeRotationDifference 
inline matrix_3x3 NodeRotationDifferenceMatrix( const siege::database_guid& NodeA, const siege::database_guid& NodeB )
{
	matrix_3x3 OrientA( DoNotInitialize );
	matrix_3x3 OrientB( DoNotInitialize );
	matrix_3x3 result ( DoNotInitialize );

	gSiegeEngine.GetNodeOrientation( NodeA, OrientA );
	gSiegeEngine.GetNodeOrientation( NodeB, OrientB );

	Multiply( result, Transpose(OrientB), OrientA );

	return result;
}

///////////////////////////////////////////////////////////////////////////////////
inline bool IsEqual(const SiegePos& p0, const SiegePos& p1, float tolerance)
{
	bool ret = (p0.node == p1.node) && p0.pos.IsEqual( p1.pos, tolerance );
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////
inline void KeepAngleInInterval(float &angle) 
{
	angle = fmodf(angle,PI2);
	if (angle < 0)   angle += PI2;
}


#endif  // __MCP_H
