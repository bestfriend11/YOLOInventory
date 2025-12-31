#pragma once
/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: Trigger_sys.h

	Author(s)	: Rick Saenz, Adam Swensen, biddle

	Purpose		:

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#ifndef _TRIGGER_SYS_H_
#define _TRIGGER_SYS_H_

#include "GoDefs.h"
#include <vector>
#include <map>

#include "trigger_defs.h"
#include "trigger_groups.h"
#include "WorldMessage.h"

#if !GP_RETAIL
	#include "WorldTime.h"
#endif

//#include "siege_defs.h"

//////////////////////////////////////////////////////////////////////////////

namespace trigger {  // begin namespace trigger


/////////////////////////////////////////////////////////////////////////////
//	Trigger storage

class Storage
{
public:

	typedef stdx::fast_vector< Trigger* >	TriggerColl;
	typedef TriggerColl::const_iterator		TriggerCollConstIter;
	typedef TriggerColl::iterator			TriggerCollIter;


	Storage( WORD InitialTrigNum = 0, Go* owner = NULL );
	Storage( const Storage& other, WORD InitialTrigNum, Go* newOwner = NULL );
   ~Storage();

	void	CommitCreation();		// Assign Truids to all existing triggers

	void	Add_Trigger( Trigger *pTrigger );
	TriggerColl::iterator Remove_Trigger( Trigger *pTrigger );

	UINT32	Size()										{ return m_Triggers.size(); }
	bool	Get( UINT32 i, trigger::Trigger **pTrigger );

	void	HandleMessage( const WorldMessage& msg );
	void	Update( const double& CurrentTime );

	bool	CommitDeleteRequests();

	bool	Load( FastFuelHandle fuel, bool IsTemplateStorage );
	void	Save( FuelHandle fuel ) const;

#	if !GP_RETAIL
	bool	ValidateTriggers(const gpstring& errheader) const;
#	endif

	bool	Xfer( FuBi::PersistContext& persist );
	bool	XferPost();

	void	SetOwner( Go* go );
	void	RefreshOwner( );

	bool	GetMustCreateOnClients() const;
	bool	CanSelfDestruct() const						{ return m_CanSelfDestruct; }

	bool	GetCanExpire() const;

	TriggerCollConstIter Begin() const					{ return m_Triggers.begin(); }
	TriggerCollConstIter End() const					{ return m_Triggers.end(); }

	// Client synchronization
		void	XferForSync( FuBi::BitPacker& packer );
static	void	SkipTriggerXfer( FuBi::BitPacker& packer );

	// Return the number of triggers that will exist on both the client & server
	DWORD	NumClientTriggers();

#	if !GP_RETAIL
	void	Draw();
#	endif

private:

	TriggerColl m_Triggers;
	Go*			m_Owner;
	WORD		m_TrigNumAssigned;
	int			m_UpdateNesting;
	bool		m_CanSelfDestruct;

	// disable this
	Storage& operator = ( const Storage& );
};


//////////////////////////////////////////////////////////////////////////////
//	Parameter storage

//	This param struct is used for defining the parameters that define a action or condition
struct Params
{
	Params()         : bWhenFalse( false ), Delay( 0 ), CountdownTimeout( 0 ), SubGroup(0), ParamID( 0 )  {}
	Params(WORD pid) : bWhenFalse( false ), Delay( 0 ), CountdownTimeout( 0 ), SubGroup(0), ParamID( pid )  {}

	bool	Xfer( FuBi::PersistContext& persist );

	void	resize( UINT32 c ) { ints.resize(c);floats.resize(c);strings.resize(c); }

	void	Clear() { ints.clear();floats.clear();strings.clear();}

	WORD	ParamID;		// Used to identify parameters when there are more than 
							// one set in use by given trigger. This allows me to 
							// distinguish different spatial conditions
							//   --biddle

	// Storage
	std::vector< INT32 >		ints;
	std::vector< float >		floats;
	gpstring_coll				strings;

	// Action option
	bool	bWhenFalse;

	// Action delay
	double	CountdownTimeout;	// count down once triggered
	float	Delay;

	int		SubGroup;		// internal trigger action/condition grouping 
							// -- replace with or derive from the 'ParamID'? --biddle

	gpstring Doc;			// for siege editor action/condition descriptions

	// Condition state
	bool	bWaitingForMessage;

private:
	

};

// Helper class for outlining the format of a Condition or an Action for SiegeEditor
class Parameter
{
public:

	class format
	{
	public:

		format() :	isInt( false ),
					isFloat( false ),
					isString( false ),
					isRequired( false ),
					isGoid( false),
					isBool( false ),
					bSaveAsHex( false ),
					b_default( false ),
					f_min( 0 ), f_max( 0 ), f_default( 0 ),
					i_min( 0 ), i_max( 0 ), i_default( 0 )	{}

		void clear()
		{
			isInt = isFloat = isString = isRequired = isGoid = isBool = bSaveAsHex = b_default = false;
			f_min = f_max = f_default = 0.0f;
			i_min = i_max = i_default = 0;
			sValues.clear();
		};

		gpstring		sName;

		bool			isRequired;
		bool			bSaveAsHex;

		// types
		bool			isInt;
		bool			isFloat;
		bool			isString;

		// sub-types
		bool			isGoid;
		bool			isBool;

		bool			b_default;
		float			f_min, f_max, f_default;
		int				i_min, i_max, i_default;
		gpstring		s_default;

		gpstring_coll	sValues;
	};

	UINT32	Size()							{ return m_args.size(); }
	bool	Get( UINT32 i, format **p )		{	if ( i < m_args.size() )
												{
													*p = &m_args[i];
													return true;
												}
												return false;	}

	void	Add( const format &p )			{ m_args.push_back( p ); }

private:

	std::vector< format >	m_args;
};

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
// Condition class moved to trigger_conditions.h

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
// Action class moved to trigger_actions.h

//////////////////////////////////////////////////////////////////////////////
//	Message-handling Trigger definitions
// 
// Notes:
//  The actual Conditions imposed on the trigger define the 'boundary' 

struct MessageHeader
{
	eWorldEvent	m_Event;
	Goid		m_SendFrom;
	Goid		m_SendTo;
	DWORD		m_Data1;

	MessageHeader()
		: m_Event   (WE_INVALID)
		, m_SendFrom(GOID_INVALID)
		, m_SendTo  (GOID_INVALID)
		, m_Data1   ()
	{}
	
	MessageHeader(eWorldEvent e,Goid f, Goid t, DWORD d)
		: m_Event   (e)
		, m_SendFrom(f)
		, m_SendTo  (t)
		, m_Data1   (d)
	{}

	MessageHeader(const WorldMessage& m)
		: m_Event   (m.GetEvent())
		, m_SendFrom(m.GetSendFrom())
		, m_SendTo  (m.GetSendTo())
		, m_Data1   (m.GetData1())
	{}

	bool Xfer( FuBi::PersistContext& persist );

};

//////////////////////////////////////////////////////////////////////////////
//	Spatial Trigger definitions
// 
// Notes:
//  The actual Conditions imposed on the trigger define the 'boundary' 

struct BoundaryCrossing
{
	double				m_Time;
	Truid				m_TriggerID;
	DWORD				m_Data;			// Sometimes it's a GO, sometimes it's a database_guid (depends on m_CrossType)
	eBoundaryCrossType	m_CrossType;
	WORD				m_ConditionID;
	WORD				m_SequenceNum;

	BoundaryCrossing() {};

	BoundaryCrossing(const double& t, Truid i, DWORD m, eBoundaryCrossType e, WORD c, WORD s) 
		: m_Time(t)
		, m_TriggerID(i)
		, m_Data(m)
		, m_CrossType(e)
		, m_ConditionID(c)
		, m_SequenceNum(s)
	{
		// verify incoming boundary information.
#if !GP_RETAIL
		if( m_Time > PreciseAdd(gWorldTime.GetTime(), 20.0 ))
		{
			GoHandle hMovingGo( MakeGoid(m_Data) );

			gperrorf(("TRIGGERSYS: Adding Boundary crossing for trigger [g:0x%08x] that will not expire for %lf seconds [%s g:%08x s:%08x]\n",
				MakeInt(m_TriggerID.GetOwnGoid()),
				PreciseSubtract(m_Time, gWorldTime.GetTime()),
				(hMovingGo) ? hMovingGo->GetTemplateName() : "<UNKNOWN>",
				(MakeGoid(m_Data)) ? MakeInt(MakeGoid(m_Data)) : -1,
				(hMovingGo) ? MakeInt(hMovingGo->GetScid()) : -1
				));
		}

#endif
	};

	friend bool operator== ( const BoundaryCrossing& x, const BoundaryCrossing& y )
	{
		return ((x.m_TriggerID	 == y.m_TriggerID)   &&
				(x.m_Data		 == y.m_Data)        &&
				(x.m_CrossType	 == y.m_CrossType)   &&
				(x.m_ConditionID == y.m_ConditionID) &&
				(x.m_SequenceNum == y.m_SequenceNum) 
				);
	}

	friend bool operator< ( const BoundaryCrossing& x, const BoundaryCrossing& y )
	{ 
		return (x.m_Time != y.m_Time) ? (x.m_Time < y.m_Time) : (x.m_SequenceNum < y.m_SequenceNum);
	}

	bool Xfer( FuBi::PersistContext& persist );

};

typedef	stdx::fast_vector<MessageHeader>				MessageList;

typedef std::multiset<BoundaryCrossing>					SignalSet;
typedef SignalSet::iterator								SignalIter;
typedef SignalSet::const_iterator						SignalConstIter;

// convert this to a std::set !
typedef std::map< Truid, Trigger* >						TriggerDb;
typedef TriggerDb::iterator								TriggerIter;

// Map Goids inside a Trigger boundary to the time that they entered
typedef stdx::linear_map< Goid, double >				GoOccupantDb;
typedef GoOccupantDb::iterator							GoOccupantIter;
typedef GoOccupantDb::const_iterator					GoOccupantConstIter;

typedef stdx::linear_map< Goid, int >					GoOccupantMap;
typedef GoOccupantMap::iterator							GoOccupantMapIter;
typedef GoOccupantMap::const_iterator					GoOccupantMapConstIter;

typedef std::map<gpstring,GroupLink*, istring_less>		GroupDb;
typedef GroupDb::iterator								GroupIter;
typedef GroupDb::const_iterator							GroupConstIter;

typedef stdx::linear_map< WORD, Condition* >			ConditionDb;
typedef ConditionDb::iterator							ConditionIter;
typedef ConditionDb::const_iterator						ConditionConstIter;

typedef stdx::linear_map< WORD, std::pair<Params,Action*> >	ActionDb;
typedef ActionDb::iterator									ActionIter;
typedef ActionDb::const_iterator							ActionConstIter;

typedef std::pair<bool,bool>							SubGroupInfo;
typedef stdx::linear_map< int, SubGroupInfo >			SubGroupStateDb;
typedef SubGroupStateDb::iterator						SubGroupIter;

struct pcondition_less
{
	bool operator () ( const Condition* x, const Condition* y ) const
	{ 
		return LessThanCondition(x,y);
	}
};

typedef stdx::linear_set< Condition*, pcondition_less > ConditionSet;
typedef ConditionSet::iterator							ConditionSetIter;
	

/////////////////////////////////////////////////////////////////////////////
//	TrigBits (micro-xfer-storage)
/*
struct TrigBits
{
	union
	{
		union
		{
			struct
			{
				WORD m_PlayerTracking;				// bits used for tracking per-player info
				WORD m_Dummy1;
			};

			struct
			{
				unsigned m_Dummy2 : 16;

				bool m_CurrentActiveState  : 1;		// is this trigger active?
			};
		};

		DWORD m_Bits;
	};
};
*/
typedef DWORD TrigBits;
#define CURRENT_ACTIVE_STATE_TRIGBIT	(1 << 0)
#define ONESHOT_HAS_TRIGGERED_TRIGBIT	(1 << 1)
#define MARKED_FOR_DELETION_TRIGBIT		(1 << 2)	// added during fix of bug #10392

#define SUBGROUP0_IS_TRUE_TRIGBIT		(1 << 8)	// added during fix of bug #10933
#define SUBGROUP1_IS_TRUE_TRIGBIT		(1 << 9)	
#define SUBGROUP2_IS_TRUE_TRIGBIT		(1 <<10)	
#define SUBGROUP3_IS_TRUE_TRIGBIT		(1 <<11)	
#define SUBGROUP4_IS_TRUE_TRIGBIT		(1 <<12)	
#define SUBGROUP5_IS_TRUE_TRIGBIT		(1 <<13)	
#define SUBGROUP6_IS_TRUE_TRIGBIT		(1 <<14)	
#define SUBGROUP7_IS_TRUE_TRIGBIT		(1 <<15)	

//////////////////////////////////////////////////////////////////////////////
// Trigger interface

class Trigger
{
	friend class TriggerSys;

public:

	Trigger();
	explicit Trigger(const Trigger& source);

	~Trigger();

	void	Shutdown();

	const Truid&	GetTruid( void ) const					{ return m_Truid; }
	void			SetTruid( const Truid& t )				{ m_Truid = t; }

	bool	Xfer( FuBi::PersistContext& persist );

	bool	PreserveInTrigBits();
	bool	RestoreFromTrigBits();
	
	void	Update( const double& CurrentTime );
	void	HandleMessage( const WorldMessage& msg );

	bool	AddCondition(WORD i, Condition* c);
	bool	AddAction(WORD i, Action* a, const Params& p );	// Actions still use separate params

	bool	RemoveCondition(WORD id);
	bool	RemoveAction(WORD id);
	void	LabelActions();

	bool	EvaluateConditions( int group );

	// $ This function will execute all its actions without evaluating the conditions.
	// $ It is intended for SE use only.
	// $$ DISABLED!!! --biddle (talked to chad he's ok with it not doing anything right now)
	bool	ExecuteActions( float SecondsSinceLastCall );
	
	bool	RemoveStaleExpectedSignals();

#	if !GP_RETAIL
	void	Draw();
	bool	Validate(gpstring& errheader);
#	endif

	void	Reset( const double& CurrentTime );

	void	IncrementExpectedSignals(const BoundaryCrossing& bc);
	void	DecrementExpectedSignals(const BoundaryCrossing& bc, const double& current_time);
	int		GetNumExpectedSignals(WORD condid);
		
	void	SetOwner( Go* go );
	void	RefreshOwner();

	void	UpdateTriggerFlags();

	// Accessors
	const gpstring&	GetGroupName()							{ return m_GroupName; }
	void	        SetGroupName(const gpstring& og)		{ m_GroupName = og; }

	void	SetInitiallyActive( bool set )					{ m_bInitiallyActiveState = set; }
	bool	GetInitiallyActive()							{ return m_bInitiallyActiveState; }

	void	SetRequestedActive( bool set )					{ m_bRequestedActiveState = set; }
	bool	GetRequestedActive()							{ return m_bRequestedActiveState; }

	bool	GetCurrentActive() const						{ return m_bCurrentActiveState;  }

	void	SetMarkedForDeletion(bool set);
	bool	GetMarkedForDeletion()							{ return m_bMarkedForDeletion;  }

	double	GetResetTimeout() const							{ return m_ResetTimeout;  }

	bool	GetIsSingle() const								{ return m_bIsSingle;  }
	void	SetIsSingle( bool set )							{ m_bIsSingle = set;  }

	bool	GetIsMulti() const								{ return m_bIsMulti;  }
	void	SetIsMulti( bool set )							{ m_bIsMulti = set;  }

	bool	GetIsSpatial() const							{ return m_bSpatialTrigger;  }
	bool	GetIsMessageHandler() const						{ return m_bMessageTrigger;  }
	bool	GetIsPureMessageHandler() const					{ return m_bPureMessageTrigger; }
	bool	GetIsPartySpecific() const						{ return m_bIsPartySpecific; }
	bool	GetIsGroupWatcher() const						{ return m_bIsGroupWatcher; }

	bool	GetHasDelayedActions() const					{ return !m_bAllActionsExecuted; }

	bool	GetIsSubGroupSpatial(int sg) const;
	bool	GetIsSubGroupMessageHandler(int sg) const;
	bool	GetIsSubGroupPureMessageHandler(int sg) const;

	bool	GetIsTooComplicated() const;		// Trigger is too complicated to be persisted using trigbits
	bool	GetMustPreserveState() const;		// Trigger state must be preserved in trigbit DB
	
	Goid	GetOwnerGoid() const;
	Go		*GetOwner() const								{ return m_pOwnerGO; }

	void	SetDelay( float initdelay )						{ m_InitialDelay = initdelay; }
	float	GetDelay() const								{ return m_InitialDelay; }

	void	SetIsOneShot( bool set )						{ m_bIsOneShot = set; }
	bool	IsOneShot()	const								{ return m_bIsOneShot; }
	bool	GetOneShotHasTriggered() const					{ return m_bOneShotHasTriggered; }

	void	SetNoTrigBits( bool set )						{ m_bNoTrigBits = set; }
	bool	IsNoTrigBits() const							{ return m_bNoTrigBits; }

	void	SetCanSelfDestruct( bool set )					{ m_bCanSelfDestruct = set; }
	bool	CanSelfDestruct() const							{ return m_bCanSelfDestruct; }

	bool	GetWaitsForConstruction() const					{  return m_bWaitsForConstruction;  }
	void	SetWaitsForConstruction( bool set )				{  m_bWaitsForConstruction = set;  }

	void	SetIsFlipFlop( bool set )						{ m_bIsFlipFlop = set; }
	bool	IsFlipFlop() const								{ return m_bIsFlipFlop; }

	//		You can no longer set the state of 'm_bClientTrigger' directly --biddle
	bool	IsClientTrigger() const							{ return m_bClientTrigger; }

	void	SetResetDuration( float set )					{ m_ResetDuration = set; }
	float	GetResetDuration() const						{ return m_ResetDuration; }

	bool	HasSubGroup(DWORD sg) const						{ return m_SubGroupStates.find(sg) != m_SubGroupStates.end(); }
	void	AddSubGroup(DWORD sg)							{ if (!HasSubGroup(sg)) m_SubGroupStates.unsorted_push_back(std::make_pair(sg,std::make_pair(false,false))); }

	// Modification

	// True if index is found, provides references for write access to params
	bool	GetConditionInfo( WORD id,  gpstring &sName, Parameter& pFormat, Params &pParams );
	bool	GetActionInfo( WORD id,  gpstring &sName, Parameter const * &pFormat, Params const * &pParams );

	const ActionDb&	GetActions() const						{ return m_Actions;		}
	const ConditionDb&	GetConditions() const 				{ return m_Conditions;	}

	bool	SetConditionInfo( WORD id , Params &pParams );
	void	ConvertConditionsToOneShot();

	// True if we need to create this trigger on clients too, false if server-only is ok (this is a conservative test)
	bool	GetMustCreateOnClients() const;

	// Parameter storage for parameter passing from action to condition

	// Return an ever increasing ID value
	WORD	GetNextParamID(void)							{ return ++m_LastParamIDAssigned; }

	// Groups
	bool	AddWatchedByCondition(Condition* watchcond);
	bool	RemoveWatchedByCondition(Condition* watchcond);

	// Spatial Collisions & Signal Messaging 

	void	OnActivation( void );
	void	OnDeactivation( void );

	void	NewGoEntering(const double& in_Time, const Go* in_Newbie, DWORD in_NewFrustum, SignalSet& inout_hitlist);
	void	StaleGoLeaving(const double& in_Time, const Go* in_Stale, SignalSet& inout_hitlist);
	void	DestroyedGoRemoval(Goid deadgoid);

	void	TeleportingGo(const double& in_Time, const Go* in_Jumper, const SiegePos& in_JumpPos, SignalSet& inout_hitlist);

	void	GoJoinedWithParty(const double& in_Time, const Go* in_Buddy, SignalSet& inout_hitlist);
	void	GoRemovedFromParty(const double& in_Time, const Go* in_Estranged, SignalSet& inout_hitlist);

	void	EnterWatchersOfTrigger( Goid in_Newbie );
	void	LeaveWatchersOfTrigger( Goid in_Stale );

#if !GP_RETAIL
	int		NumInsideAfterAllPendingCrossings(Goid in_MoverGoid, WORD in_CondID, int in_StartInside,bool in_UniqueTest, bool in_GroupWatcher) const;
#else
	int		NumInsideAfterAllPendingCrossings(Goid in_MoverGoid, WORD in_CondID, int in_StartInside) const;
#endif

	void	CollectTriggerCollisions( const Go*			in_Mover,
									  const SiegePos&	in_PosA,
									  const double&		in_TimeA,
									  const SiegePos&	in_PosB,
									  const double&		in_TimeB,
									  SignalSet&		out_HitList );

	void	CollectWatchersOfCollision(const Go*			in_MoveGoid,
									   const double&		in_HitTime,
									   bool					in_IsEntering,
									   eBoundaryCrossType	in_CrossType,
									   SignalSet&			inout_hitlist);

	bool	OnGoEntersBoundary(Goid goid, double time, WORD condID);
	bool	OnGoLeavesBoundary(Goid goid, double time, WORD condID);
	bool	IsInsideBoundary(Goid goid,WORD condID) const;

	void	NotifyWatchersOfCrossingError(const Goid		in_BadGoid,
										  const double&		in_BadTime);

	bool 	GetSubGroupBoundaryOccupants(int sg, const GoOccupantMap* &occs);
	bool	HasSubGroupBoundaryOccupants(int sg) const;

	bool 	GetSubGroupSatisfyingOccupants(int sg, const GoOccupantMap* &occs);
	bool	HasSubGroupSatisfyingOccupants(int sg) const;

	bool 	GetSubGroupSatisfyingMessages(int sg, const MessageList* &mess);
	bool	HasSubGroupSatisfyingMessages(int sg) const;
	bool	ClearSubGroupSatisfyingMessages(int sg);

	void	ClearUndelayedSubGroups();

	void	GetNonMessageConditions(gpstring& txt) const;

	/// Synchronization
	void	XferForSync( FuBi::BitPacker& packer );

	struct LessThan
	{
		bool operator () ( const Trigger* l, const Trigger* r )
		{
			return l && r && (l->GetTruid() < r->GetTruid());
		}
	};

private:

	void EnterTheWorld(bool forceactive);
	void LeaveTheWorld();
	
	bool		m_bIsRestoringFromTrigBits;	// Gets set to true while the trigger is being
											// restored so that we don't set off any triggers 
											// as I recover the current occupants

	Truid		m_Truid;

	bool		m_bInitiallyActiveState;
	bool		m_bRequestedActiveState;
	bool		m_bCurrentActiveState;

	bool		m_bMarkedForDeletion;

	double		m_ResetTimeout;				// Time of next reset (negative value --> infinite duration)
	float		m_ResetDuration;			// If not single shot how long of time to wait before resetting

	float		m_InitialDelay;				// Basic delay to apply to all triggers

	bool		m_bIsOneShot;				// If this is set trigger is a one time only trigger
	bool		m_bNoTrigBits;				// Disallow usage of trigbits on this guy
	bool		m_bOneShotHasTriggered;		// Indicates that a single shot has been trigger but is running delayed actions

	bool		m_bCanSelfDestruct;			// Can we self-destruct if all systems are go? Set to false if the trigger must hang around after it's blown its load.
	bool		m_bWaitsForConstruction;	// If the trigger has a condition that waits for a WE_CONSTRUCTED message

	bool		m_bSpatialTrigger;			// Trigger has at least boundary checking condition
	bool		m_bMessageTrigger;			// Trigger has at least one message receiving condition
	bool		m_bPureMessageTrigger;		// Trigger has only message receiving conditions
	bool		m_bUniqueEnterTrigger;		// Trigger has at least one ON_UNIQUE_ENTER boundary condition
	bool		m_bIsPartySpecific;			// Trigger has only PARTY_MEMBER specific boundary conditions
	bool		m_bIsGroupWatcher;			// Trigger has Group Watching boundary condition

	bool		m_bIsFlipFlop;				// If this is a flip flop trigger then disregard single shot settings
											// and reset after cycling through a single state change.
	bool		m_bClientTrigger;			// this trigger runs on all machines

	bool		m_bIsMulti;					// Trigger runs in multiplayer
	bool		m_bIsSingle;				// Trigger runs in single player
	
	gpstring	m_GroupName;
	
	// for delayed actions
	bool		m_bAllActionsExecuted;

	// Used to keep track of signals that are yet to be processed by triggers that have reset delays. 
	// Don't want to reset if a signal is due to arrive
	SignalSet	m_SignalsExpected;
	
	Go			*m_pOwnerGO;				// who this trigger belongs to

	// Unique ID that gets incremented for each set of parameters created
	WORD		m_LastParamIDAssigned;

	// Each trigger can have it conditions & actions split up into multiple independant 'sub groups'
	// We need to maintain a list of these sub-groups as well as the 'triggered' state for each
	SubGroupStateDb	m_SubGroupStates;

	my ConditionDb	m_Conditions;
	ActionDb		m_Actions;

	ConditionSet	m_WatchedByConditions;	// Pointers to conditions (of other triggers)
											// that are watching this trigger
											// This is meant to be a fast way to derefence
											// links defined in the TriggerSys::GroupDB table

	// disable this
	Trigger& operator= ( const Trigger& );

//	SET_NO_COPYING( Trigger );
};


//////////////////////////////////////////////////////////////////////////////
//	used by TriggerSys

class ActionInfo
{
public:

	enum eCallType
	{
		SET_BEGIN_ENUM( CT_, 0 ),
		CT_INVALID,
		CT_NODE_FADE,
		CT_SNODE_FADE,
		CT_SGLOBAL_NODE_FADE,
		CT_MOOD_CHANGE,
		SET_END_ENUM( CT_ ),
	};

	ActionInfo()
		: m_CallType(CT_INVALID)
		, m_Region(0)
		, m_Section(0)
		, m_Level(0)
		, m_Object(0)
		, m_Time(0)
		, m_FT(FT_NONE)
	{};

	~ActionInfo(){};

	bool Xfer( FuBi::PersistContext& persist );

	ActionInfo(double t, eCallType c, DWORD r, DWORD s,DWORD l,DWORD o, FADETYPE f, const GoidColl& m)
		: m_Time(t)
		, m_CallType(c)
		, m_Region(r)
		, m_Section(s)
		, m_Level(l)
		, m_Object(o)
		, m_FT(f)
	{
		m_MoodName = "";
		m_Members = m;
	};

	ActionInfo(double t, eCallType c, const gpstring& n, const GoidColl& m)
		: m_Time(t)
		, m_CallType(c)
		, m_Region(0)
		, m_Section(0)
		, m_Level(0)
		, m_Object(0)
		, m_FT(FT_NONE)
	{
		m_MoodName = n;
		m_Members = m;
	};
	
	void XferForSync( FuBi::BitPacker &packer );
//	void ExecuteNodeFade( FrustumId fid );
	
	double		m_Time;
	eCallType	m_CallType;

	int			m_Region;
	int			m_Section;
	int			m_Level;
	int			m_Object;
	FADETYPE	m_FT;

	GoidColl	m_Members;

	gpstring	m_MoodName;

	SET_NO_COPYING( ActionInfo );
	SET_NO_INHERITED( ActionInfo );
};


//////////////////////////////////////////////////////////////////////////////
//	TriggerSys definition

class TriggerSys : public Singleton <TriggerSys>
{
public:

	TriggerSys();
	~TriggerSys();

	void Update();
	WORD NextSeqNum()								{ return (++m_CurrentSeqNum); }

	void ClearScidSpecifics( void )					{ m_TrigBitsDb.clear(); }
	void RetireScid( Scid scid );

	bool FetchTrigBits(const Scuid& sc, TrigBits& tb);
	bool StoreTrigBits(const Scuid& sc, const TrigBits& tb);

	void AddActionInfo( double time, ActionInfo * actionInfo );
	void ExecuteActionInfo( ActionInfo * actionInfo );

//	Condition, Action interfaces

	Parameter* GetActionFormat( const gpstring& sActionName );
//	bool       Add_Action( const gpstring &sActionName, const Params& parms, Trigger &trigger );
	
	Action*	   NewAction( const gpstring& sActionName );
	Condition* NewCondition( const gpstring& sConditionName );

	UINT32	GetActionCount() const					{ return m_ActionDefs.size(); }

	gpstring_coll &GetConditionNames()				{ return m_ConditionNames; }
	gpstring_coll &GetActionNames()					{ return m_ActionNames; }

	bool	ActionAllowsClientExecution( const gpstring& sActionName );
	
//  'Valid' trigger database

	void AddValidTrigger( Trigger* pTrigger );
	void RemoveValidTrigger( Trigger* pTrigger );
	bool IsValidTrigger( Trigger* pTrigger );

//  Spatial triggers

	void AddSpatialTrigger( Trigger* pTrigger );
	void RemoveSpatialTrigger( Trigger* pTrigger );
	bool IsSpatialTrigger( Trigger* pTrigger );

	bool SignalTrigger(const BoundaryCrossing& bc);
	
	void UpdateTriggersWhenGoEntersWorld( const Go*	in_Newbie, DWORD in_NewFrustum );
	void UpdateTriggersWhenGoLeavesWorld( const Go*	in_Stale );
	void UpdateTriggersWhenGoIsDestroyed( const Go* in_Dead );
	void UpdateTriggersAfterTerrainMoves( const Go* in_Jumper );
	void UpdateTriggersWhenGoJoinsParty ( const Go* in_Buddy );
	void UpdateTriggersWhenGoLeavesParty( const Go* in_Buddy );

	bool CollectTriggerCollisions( const Go*		in_Mover,
								   const SiegePos&	in_PosA,
								   const double&	in_TimeA,
								   const SiegePos&	in_PosB,
								   const double&	in_TimeB,
								   SignalSet&		out_HitList );

	bool ProcessSignals( double current_time);
	bool CancelSignal(const BoundaryCrossing& bc);

	// Groups
	void AddGroupWatcher( const gpstring &sGroupName, const Cuid& watching_condition  );
	void AddGroupMember( const gpstring &sGroupName, const Truid& member_trigger );

	void RemoveGroupWatcher( const gpstring &sGroupName, const Cuid& watching_condition  );
	void RemoveGroupMember( const gpstring &sGroupName, const Truid& member_trigger );

	bool GetGroupMembers(const gpstring &sGroupName, GroupLink::MemberSet* &members);

	// Trigger and condition lookup
	bool FetchTriggerByTruid(const Truid& truid, Trigger*& trig);
	bool FetchConditionByCuid(const Cuid& cuid,  Condition*& cond);

FEX void SDeactivateTrigger( Goid owngoid, WORD trignum );
	void DeactivateTrigger( Scid ownscid, Goid owngoid, WORD trignum );

FEX	bool SSendActionToPartyMembersRemainingInNode(SiegeId node, const gpstring& fadereq );
FEX	void RCSendActionToPartyMembersRemainingInNode(  const_mem_ptr ptr );
	void SendActionToPartyMembersRemainingInNode( FuBi::BitPacker& packer );

	bool Xfer(FuBi::PersistContext& persist);
	bool Shutdown(void);
	
#if !GP_RETAIL
	void GroupDrawHelper(const SiegePos gpos, const gpstring& gname, const gpstring& lbl );
#endif

	
private:
	
	// Keep a global sequence number for trigger signals
	WORD				m_CurrentSeqNum;

	kerneltool::Critical m_TrigSysCritical;

	typedef std::pair< const gpstring, Action* >					tActionDef_pair;
	typedef std::map< const gpstring, Action*, istring_less >		tActionDef_map;
	typedef tActionDef_map::iterator								tActionDef_map_i;
	typedef std::map <Scuid, TrigBits> TrigBitsDb;
	typedef std::multimap< double, ActionInfo * >					tActionInfo_multimap;

	void	New_Action_Def( const gpstring &sActionName, Action *pAction, bool bAllowClientExecution );

//	Conditions, Actions

	tActionDef_map		m_ActionDefs;
	gpstring_coll		m_ActionNames;
	gpstring_coll		m_ConditionNames;

//	Spatial triggers & Accumulated Signals
	TriggerDb			m_ValidTriggers;		// ALL triggers that are properly loaded & committed
	TriggerDb			m_SpatialTriggers;		// Spatial triggers (subset of committed triggers)
	
	SignalSet			m_Signals;				// Storage for incoming signals from the MCP

	GroupDb				m_Groups;

	TrigBitsDb			m_TrigBitsDb;			// trigbits are used to do a micro save game to reduce memory usage when triggers leave the world

	tActionInfo_multimap	m_ActionInfos;

	FUBI_SINGLETON_CLASS(trigger::TriggerSys, "TriggerSys");
};

#define gTriggerSys trigger::TriggerSys::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

}  // end namespace trigger


#endif  // _TRIGGER_SYS_H_
