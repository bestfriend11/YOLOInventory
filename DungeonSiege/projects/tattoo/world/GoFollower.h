//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoFollower.h
// Author(s):  biddle
//
// Summary  :  Contains info that allows a Go to 'follow' an MCP plan
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//
//  $Notes:: This was built using GoStore as an example
//
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOFOLLOWER_H
#define __GOFOLLOWER_H

//////////////////////////////////////////////////////////////////////////////

#include "Go.h"
#include "MCP.h"
#include "Trigger_Sys.h"

// Forward Declarations

/*===========================================================================
	Struct		: RotReq
---------------------------------------------------------------------------*/
/* thm
struct RotReq {

	RotReq(DWORD reqid = 0, double init_time = 0, const SiegeRot& rot = SiegeRot::INVALID)
		: m_RID(reqid)
		, m_StartTime(init_time)
		, m_Rotation(rot)
	{};

	bool Xfer( FuBi::PersistContext& persist );

	DWORD		m_RID;
	double      m_StartTime;
	SiegeRot	m_Rotation;
};

typedef std::deque<RotReq>				RotationQueue;
typedef RotationQueue::iterator			RotationQIter;
typedef RotationQueue::const_iterator	RotationQConstIter;
*/

/*===========================================================================
	Class		: PlanSegment
---------------------------------------------------------------------------*/
struct PlanSegment
{

	// Default constructor
	PlanSegment() 
		: m_Mask(0)
		, m_WayPos(SiegePos::INVALID)
		, m_BegPos(vector_3::ZERO)
		, m_EndPos(vector_3::ZERO)
		, m_ReqBlock(MCP::RID_UNDEFINED)
		, m_WayTime(0)
		, m_BegTime(0)
		, m_EndTime(0)
		, m_EndVelocity(0)
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
	{};

	// Plan Segment constructed from time&pos sent by MCP
	PlanSegment( double curr_time, MCP::eRID rb, const SiegePos& pos );

	void SetTime(double nt) { m_WayTime = m_BegTime = m_EndTime = nt; }

	void ClearMask()		{ m_Mask = 0; }

	void SetChore(eAnimChore ch,eAnimStance st ,DWORD sa , float rg, DWORD fl )
	{
		m_Chore = ch;
		m_Stance = st;
		m_SubAnim = sa;
		m_Range = rg;
		m_Flags = fl;
		m_Mask |= MCP::HAS_CHORE_FLAG;
	};

	void SetOrient(MCP::eOrientMode md)
	{
		m_Mode = md;
		m_Target = GOID_INVALID;
		m_TargPos = SiegePos::INVALID;
		m_TargRot = SiegeRot::INVALID;
		m_Mask |= MCP::HAS_ORIENT_FLAG;
	};

	void SetOrient(MCP::eOrientMode md, Goid tg)
	{
		m_Mode = md;
		m_Target = tg;
		m_TargPos = SiegePos::INVALID;
		m_TargRot = SiegeRot::INVALID;
		m_Mask |= MCP::HAS_ORIENT_FLAG;
	};

	void SetOrient(MCP::eOrientMode md, const SiegePos& tp)
	{
		m_Mode = md;
		m_Target = GOID_INVALID;
		m_TargPos = tp;
		m_TargRot = SiegeRot::INVALID;
		m_Mask |= MCP::HAS_ORIENT_FLAG;
	};

	void SetOrient(MCP::eOrientMode md, const SiegeRot& tr)
	{
		m_Mode = md;
		m_Target = GOID_INVALID;
		m_TargPos = SiegePos::INVALID;
		m_TargRot = tr;
		m_Mask |= MCP::HAS_ORIENT_FLAG;
	};

	void SetSignal(trigger::Truid t, trigger::eBoundaryCrossType e, WORD c, WORD s)
	{
		m_Trigger = t;
		m_CrossType = e;
		m_HitCondID = c;
		m_SequenceNum = s;
		m_Mask |= MCP::HAS_SIGNAL_FLAG;
	};

	void Collapse()
	{
		m_BegTime = m_EndTime = m_WayTime;
		m_BegPos = m_EndPos = vector_3::ZERO;
		m_EndVelocity = 0.0f;
	};

	DWORD	m_Mask;	

	MCP::eRID m_ReqBlock;
	SiegePos m_WayPos;		// The waypoint that controls this segment
	double m_WayTime;		// Time at the WAY point of the segment

	// Beg/End are the midpoints of the segments joining waypoints

	double   m_BegTime;		// Time at the BEG of the segment (End of prev segment)
	vector_3 m_BegPos;		// offset from WayPos at beginning of segment
	
	double   m_EndTime;		// Time at the END of the segment (Beginning of next segment)
	vector_3 m_EndPos;		//  offset from WayPos at end of segment

	float m_EndVelocity;	// The velocity required to traverse the second half of the interval
	
	// Orient At Way Time/Pos
	Goid        m_Target;
	SiegePos    m_TargPos;
	SiegeRot    m_TargRot;
	MCP::eOrientMode m_Mode;

	// Chore At Way Time/Pos
	eAnimChore	m_Chore;
	eAnimStance	m_Stance;
	DWORD		m_SubAnim;
	float		m_Range;
	DWORD		m_Flags;

	// Signal (to trigger)
	trigger::Truid              m_Trigger;
	trigger::eBoundaryCrossType m_CrossType;
	WORD                        m_HitCondID;
	WORD                        m_SequenceNum;

	// Predicate for sorting
	struct LessThan
	{
		bool operator () ( const PlanSegment& l, const PlanSegment& r )
		{
			return (l.m_BegTime < r.m_BegTime);
		}
	};

	bool Xfer( FuBi::PersistContext& persist );

	bool IsPrepared() const				{ return (m_Mask & MCP::HAS_SEGMENT_PREPPED)!=0;}
	bool IsBeginPosPrepared() const		{ return (m_Mask & MCP::HAS_SEGMENT_BEGINPOS_PREPPED)!=0;}

	bool HasPosition(void) const		{ return (m_Mask & MCP::HAS_POSITION_FLAG) != 0; }
	bool HasOrient(void) const			{ return (m_Mask & MCP::HAS_ORIENT_FLAG) != 0; }
	bool HasChore(void) const			{ return (m_Mask & MCP::HAS_CHORE_FLAG) != 0; }
	bool HasSignal(void) const			{ return (m_Mask & MCP::HAS_SIGNAL_FLAG) != 0; }


#	if !GP_RETAIL
	void DebugDraw( DWORD colour, const vector_3& offset ,bool forcelabel = false) const;
#	endif // !GP_RETAIL
};

typedef std::deque<PlanSegment>			PlanSegQueue;
typedef PlanSegQueue::iterator			PlanSegQIter;
typedef PlanSegQueue::const_iterator	PlanSegQConstIter;

//////////////////////////////////////////////////////////////////////////////
// class GoFollower declaration

class GoFollower : public GoComponent
{
public:
	SET_INHERITED( GoFollower, GoComponent );

// Setup.

	// ctor/dtor
	GoFollower( Go* parent );
	GoFollower( const GoFollower& GoFollower, Go* parent );
	virtual ~GoFollower( void );

// Interface.

	bool Update(double synchronized_time);

	const SiegePos& GetCurrentPosition() const ;
	const Quat&		GetCurrentOrientation() const;
	const SiegeRot&	GetCurrentOrientationSiegeRot() const;
		  Quat		GetCurrentOrientationRelativeToNode(const siege::database_guid& n) const;

		  vector_3	GetCurrentTangentVector();
	      vector_3	GetCurrentNormalVector();

FEX float GetCurrentVelocity() const;
FEX	MCP::eOrientMode GetCurrentOrientMode() const		{ return m_OrientationMode; }	
FEX	bool IsOrientModeDefined() const					{ return ( m_OrientationMode != MCP::ORIENTMODE_UNDEFINED); }	
FEX	Goid GetCurrentOrientTarget() const					{ if ((m_OrientationMode == MCP::ORIENTMODE_TRACK_OBJECT) ||
															  (m_OrientationMode == MCP::ORIENTMODE_TRACK_OBJECT_REVERSE))
															{
																return  m_OrientationTrackObj;
															}
															else
															{
																return GOID_INVALID;
															}
														}

FEX	SiegePos GetCurrentOrientTargPos() const			{ if ((m_OrientationMode == MCP::ORIENTMODE_FIXED_POSITION) ||
															  (m_OrientationMode == MCP::ORIENTMODE_FIXED_POSITION_REVERSE))
															{
																return  m_OrientationTrackPos;
															}
															else
															{
																return SiegePos::INVALID;
															}
														}

FEX	SiegeRot GetCurrentOrientationGoal() const			{ if ((m_OrientationMode == MCP::ORIENTMODE_FIXED_DIRECTION))														  
															{
																return  m_OrientationGoal;
															}
															else
															{
																return SiegeRot::INVALID;
															}
														}

    void SetLockedMovementGoid(Goid targ)				{ m_LockedMovementGoid = targ; }
	Goid GetLockedMovementGoid() const					{ return m_LockedMovementGoid; }

	eAnimChore GetChoreAtTime(double t);
	void GetChoreParamsAtTime(double t, eAnimChore &ac, eAnimStance &as, DWORD &sa, DWORD &fl);

	void GetOrientationAtTime(double t, SiegeRot& cr);
	void GetPositionAtTime(double t, SiegePos& cp);


#	if !GP_RETAIL
	void DebugDraw(void);
	void Dump(void);
#	endif // !GP_RETAIL

	// Networking

// Packed synchronization
	void SSendPackedUpdateToFollowers( FuBi::BitPacker &packer, DWORD frustums );
FEX	void RCSendPackedUpdateToFollowers( const_mem_ptr ptr );
	void SendPackedUpdateToFollowers( FuBi::BitPacker &packer );
	void DecodeNode(const MCP::TimeNode& tn);

	void SSendPackedPositionUpdateToFollowers( FuBi::BitPacker &packer, DWORD frustums );
FEX	void RCSendPackedPositionUpdateToFollowers( const_mem_ptr ptr );
	void SendPackedPositionUpdateToFollowers( FuBi::BitPacker &packer );

	void SSendPackedClipToFollowers( FuBi::BitPacker &packer );
FEX	void RCSendPackedClipToFollowers( const_mem_ptr ptr );
	void SendPackedClipToFollowers( FuBi::BitPacker &packer );

	void UnpackRefresh( FuBi::BitPacker &packer );

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone( Go* newParent );
	virtual bool         Xfer ( FuBi::PersistContext& persist );

	// event handlers
	virtual void HandleMessage  ( const WorldMessage& msg );
	virtual void Update         ( float deltaTime );
	virtual void DetectWantsBits( void )  {  SetWantsUpdates();  }

	void ClipAllQueues( const double& cliptime, const SiegePos& pos );

	bool ForceUpdatePositionOrientation ( void );

	static DWORD		GoFollowerToNet( GoFollower* x );
	static GoFollower*	NetToGoFollower( DWORD d, FuBiCookie* cookie );

private:

	MCP::eOrientMode	m_OrientationMode;

	SiegeRot			m_OrientationGoal;
	SiegePos	 		m_OrientationTrackPos;
	Goid				m_OrientationTrackObj;

	bool				m_HasReachedOrientationGoal;

	PlanSegQueue		m_SegmentQueue;

	SiegePos			m_CurrentPosition;
	vector_3			m_CurrentTangent;
	SiegeRot			m_CurrentOrientation;

	float				m_CurrentVelocity;

	double				m_SynchronizedTime;
	double				m_CurrentStartTime;

	Goid				m_LockedMovementGoid;

	trigger::SignalSet	m_SignalsPending;

	SET_NO_COPYING( GoFollower );
	FUBI_RPC_CLASS( GoFollower, GoFollowerToNet, NetToGoFollower, "" );

};


struct PackedUpdateToFollowersXfer
{
	PackedUpdateToFollowersXfer( void )
	{
		::ZeroObject( *this );
	}
	void Xfer( FuBi::BitPacker& packer );

#	if !GP_RETAIL
	void RpcToString( gpstring& appendHere ) const;
#	endif

	std::vector<MCP::TimeNode> m_TimeNodes;
};


struct PackedPositionUpdateToFollowersXfer
{
	PackedPositionUpdateToFollowersXfer( void )
	{
		::ZeroObject( *this );
	}
	void Xfer( FuBi::BitPacker& packer );

#	if !GP_RETAIL
	void RpcToString( gpstring& appendHere ) const;
#	endif

	SiegePos m_UpdatePosition;
};

struct PackedClipToFollowersXfer
{
	PackedClipToFollowersXfer( void )
	{
		::ZeroObject( *this );
	}
	void Xfer( FuBi::BitPacker& packer );

#	if !GP_RETAIL
	void RpcToString( gpstring& appendHere ) const;
#	endif

	double m_ClipTime;
	SiegePos m_ClipPos;
};




//////////////////////////////////////////////////////////////////////////////

#endif  // __GOFOLLOWER_H

