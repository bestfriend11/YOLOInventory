#pragma once
/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: 	GoMind.h

	Author(s)	: 	Bartosz Kijanka

	Purpose		:	This class manages skrit "jobs."  There are two queues of
					jobs; the brain quque and the action queue.  By convention
					the brain queue contains only up to one active job, while
					the action queue can contain n jobs.  Only the front job
					in each queue gets updates and world events.


	In the process of normal operation, the Mind ( and other systems ) 
	will generate a number of world events which will be sent to the 
	owning Go.  From there, the message will trickle back down to the Mind and 
	to the front job in each queue; to the current brain and current action.


	---------------------------------------------------------------------


	AI related events, and the conditions which lead to their generation are as follows:


	Terms:
	------
		Actor 	= owner of the brain.
		Target 	= the actor that the owner is attacking or interacting with.

	WE_ALERT_ENEMY_SPOTTED

		Sent to Actor when a new enemy enters the sight range AND line-of-sight.
		The Goid of the enemy sent with the event in WorldMessage::Data1


	WE_ATTACKED_MELEE

		Sent to Target when Actor has swung a melee weapon and reached the 
		point where hit/miss might be calcualted.


	WE_ATTACKED_RANGED

		Sent to Target when Actor launches a projectile a ranged weapon at the Target.


	WE_ENEMY_ENTERED_INNER_COMFORT_ZONE	

		Sent to Actor when an enemy enters into the inner comfort zone.


	WE_ENEMY_ENTERED_OUTER_COMFORT_ZONE	

		Sent to Actor when an enemy enters into the outer comfort zone.


	WE_ENEMY_SPOTTED

		Sent to Actor when an enemy enters the sight range AND is within line-of-sight.


	WE_ENGAGED_FLED
	
		Sent to Actor when Target has decided to turn and run like hell.


	WE_ENGAGED_HIT_KILLED				

		Sent to Actor when Target was hit AND killed as a result.


	WE_ENGAGED_HIT_LIVED				

		Sent to Actor when Target was was hit BUT he lived.


	WE_ENGAGED_INVALID

		Sent to Actor when Target is no longer a valid Go.  Usually results from the system 
		simply deleting the Go for a number of reasons like unloading.  The Actor should 
		receive a number of other messgaes and have an opportunity to disengage the object 
		before this message is received.  This is a message of last result.  Performing any
		operations on a target which is known to be invalid will likely lead
		to an acces violation.


	WE_ENGAGED_KILLED					

		Sent to Actor when Target was killed by some one other than Actor.


	WE_ENGAGED_LOST						

		Sent to Actor when Target is no longer visible to Actor.  Visibility of Target
		is lost after Target is no longer visible for m_VisibilityMemoryDuration seconds.


	WE_ENGAGED_LOST_CONSCIOUSNESS

		Sent to Actor when Target has been knocked out cold.


	WE_ENGAGED_MISSED

		Sent to Actor when he has swung a melee weapon at the target and missed.


	WE_FRIEND_ENTERED_INNER_COMFORT_ZONE

		Sent to Actor when a friend enters the inner comfort zone.


	WE_FRIEND_ENTERED_OUTER_COMFORT_ZONE

		Sent to Actor when friend enters the outer comfort zone.


	WE_FRIEND_SPOTTED					

		Sent to Actor when a friend enters the sight range AND is within line-of-sight.

	WE_GAINED_CONSCIOUSNESS				
	
		Sent to Actor when he gains conscousness after being knocked out.


	WE_JOB_FAILED						

		NOT USED


	WE_JOB_FINISHED

		Sent to Actor each time a job finished.


	WE_JOB_REACHED_TRAVEL_DISTANCE

		Sent to Actor each time any job moves the m_JobTravelDistanceLimit distance away from the point
		at which the job was initialized/begun.


	WE_JOB_TIMER_DONE

		Sent to each JOB every time the job timer has been set and has finished counting down.


	WE_LIFE_RATIO_REACHED_HIGH

		Sent to Actor each time his life ratio crosses the m_ActorLifeRatioHighThreshold threshold.


	WE_LIFE_RATIO_REACHED_LOW			

		Sent to Actor each time his life ratio crosses the m_ActorLifeRatioLowThreshold threshold.


	WE_MANA_RATIO_REACHED_HIGH			

		Sent to Actor each time his life ratio crosses the m_ActorLifeRatioHighThreshold threshold.


	WE_MANA_RATIO_REACHED_LOW			
	
		Sent to Actor each time his mana ratio crosses the m_ActorLifeRatioLowThreshold threshold.


	WE_MIND_ACTION_Q_EMPTIED

		Sent to the Actor each time the action queue is flushed empty.


	WE_MIND_ACTION_Q_IDLED				
	
		Sent to the Actor after m_IdleTimerTrigger seconds of non-activity.


	WE_MIND_PROCESSING_NEW_JOB

		Sent to Actor each time a new action Job is processed for the first time.


	WE_REQ_JOB_FINISH

		Sent to the JOB when the system request that the job bring itself to a graceful end.  I.e. in a 
		melee attack we will want the attack job to finish a swing if it's in the middle of one.


	(C)opyright 2001 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#ifndef __GOMIND_H
#define __GOMIND_H


#include "Go.h"
#include "GoCore.h"
#include "GoDefs.h"
#include "aiaction.h" // $$

//----- AI-related constants

/////////////////////////////////////////////////////////////
// eJobAbstractType

/*
	This describes the job type as seen by the highest-level ai in the game.  When the player or AI
	choose to give commands to an object, they do it in terms of this type.  So, the high-level Xs and Os
	commands that the AI or player gives, they give as these job types.

	When an actor is loaded, the content must describe the binding of abstract job to the specific.
*/
enum eJobAbstractType
{
	SET_BEGIN_ENUM( JAT_, 0 ),

	JAT_INVALID,
	JAT_NONE,
	JAT_ATTACK_OBJECT,
	JAT_ATTACK_OBJECT_MELEE,
	JAT_ATTACK_OBJECT_RANGED,
	JAT_ATTACK_POSITION,
	JAT_ATTACK_POSITION_MELEE,
	JAT_ATTACK_POSITION_RANGED,
	JAT_BRAIN,
	JAT_CAST,
	JAT_CAST_POSITION,
	JAT_DIE,
	JAT_DO_SE_COMMAND,
	JAT_DRINK,
	JAT_DROP,
	JAT_EQUIP,
	JAT_FACE,
	JAT_FIDGET,
	JAT_FLEE_FROM_OBJECT,
	JAT_FOLLOW,
	JAT_GAIN_CONSCIOUSNESS,
	JAT_GET,
	JAT_GIVE,
	JAT_GUARD,
	JAT_LISTEN,
	JAT_MOVE,
	JAT_PATROL,
	JAT_PLAY_ANIM,
	JAT_STARTUP,
	JAT_STOP,
	JAT_TALK,
	JAT_UNCONSCIOUS,
	JAT_USE,

	SET_END_ENUM( JAT_ ),
};

FEX const char * ToString( eJobAbstractType Type );
FEX bool FromString( const char* str, eJobAbstractType & Type );

MAKE_ENUM_MATH_OPERATORS( eJobAbstractType );

FEX bool IsAttackJat( eJobAbstractType jat );
FEX bool IsLeafAttackJat( eJobAbstractType jat );
FEX bool IsCastJat( eJobAbstractType jat );
FEX eJobAbstractType GetBaseJat( eJobAbstractType in );

struct JobLookupEntry
{
	JobLookupEntry()
	{
		ZeroObject( *this );
	}

	eJobAbstractType				m_Jat;
	JobCloneSourceMap::iterator		m_pSlot;
};

/////////////////////////////////////////////////////////////
// eJobTrait

enum eJobTraits
{
	// static traits
	JT_NONE                    			=		0,

	JT_REQUIRE_GOAL         			= 1 <<  0,
	JT_REQUIRE_MODIFIER					= 1 <<  1,
	JT_REQUIRE_POSITION       			= 1 <<  2,
	JT_REQUIRE_SLOT						= 1 <<  3,
	
	JT_UNCONSCIOUS_OK					= 1 <<  4,
	JT_GHOST_OK							= 1 <<  5,
	JT_DEAD_OK							= 1 <<  6,

	JT_ENGAGED_UNCONSCIOUS_OK			= 1 <<  7,
	JT_ENGAGED_GHOST_OK					= 1 <<  8,
	JT_ENGAGED_DEAD_OK					= 1 <<  9,

	JT_ALLOW_REFLEXIVE_GOAL    			= 1 << 10,
	JT_ALLOW_REFLEXIVE_MODIFIER			= 1 << 11,
	JT_ALLOW_REFLEXIVE_POSITION			= 1 << 12,
										
	JT_AUTO_INTERRUPTABLE           	= 1 << 13,
	JT_HUMAN_INTERRUPTABLE				= 1 << 14,
	JT_OFFENSIVE               			= 1 << 15,
	JT_DEFENSIVE               			= 1 << 16,

	JT_DISALLOW_CIRCULAR_JOB_ASSIGNMENT	= 1 << 17,
	JT_WATCH_GOAL_MESSAGES				= 1 << 18,
	JT_MOVES_ACTOR						= 1 << 19,
	JT_USE_SPATIAL_SENSORS				= 1 << 20,

	JT_REPEATING               			= 1 << 21,

	JT_ALWAYS_CREATE_ICON				= 1 << 22,

	JT_HUMAN_ASSIGNED           		= 1 << 23,
	JT_REFLEX_ASSIGNED         			= 1 << 24,
	JT_PARTY_ASSIGNED         			= 1 << 25,
	JT_COMMAND_ASSIGNED        			= 1 << 26,

	JT_ASSIGNED_FRONT					= 1 << 27,
	JT_ASSIGNED_BACK					= 1 << 28,

	JT_IN_ATOMIC_STATE					= 1 << 29,
};

MAKE_ENUM_BIT_OPERATORS( eJobTraits );


/////////////////////////////////////////////////////////////
// eJobQ

enum eJobQ
{
	SET_BEGIN_ENUM( JQ_, 0 ),

	JQ_BRAIN,
	JQ_ACTION,

	SET_END_ENUM( JQ_ ),
};

const char* ToString( eJobQ value );
bool FromString( const char* str, eJobQ & value );


/////////////////////////////////////////////////////////////
// eQPlace

enum eQPlace
{
	SET_BEGIN_ENUM( QP_, 0 ),

	QP_INVALID,
	QP_NONE,

	QP_FRONT,
	QP_UNUSED1,
	QP_BACK,

	QP_CLEAR,

	SET_END_ENUM( QP_ ),
};

const char* ToString( eQPlace value );
bool FromString( const char* str, eQPlace & value );


/////////////////////////////////////////////////////////////
// eActorDisposition

enum eActorDisposition
{
	SET_BEGIN_ENUM( AD_, 0 ),

	AD_INVALID,
	AD_OFFENSIVE,
	AD_DEFENSIVE,

	SET_END_ENUM( AD_ ),
};

const char* ToString( eActorDisposition value );
bool FromString( const char* str, eActorDisposition & value );


////////////////////////////////////////////////////////////////////////////////
//	eWeaponPreference

enum eWeaponPreference
{
	SET_BEGIN_ENUM( WP_, 0 ),

	WP_INVALID,
	WP_NONE,
	WP_KARATE,
	WP_MELEE,
	WP_RANGED,
	WP_MAGIC,
	
	SET_END_ENUM( WP_ ),
};

const char* ToString( eWeaponPreference value );
bool FromString( const char* str, eWeaponPreference & value );


/////////////////////////////////////////////////////////////
// eMovementOrders

enum eMovementOrders
{
	SET_BEGIN_ENUM( MO_, 0 ),

	MO_INVALID,

	MO_FREE,
	MO_LIMITED,
	MO_HOLD,

	SET_END_ENUM( MO_ )
};

const char* ToString( eMovementOrders value );
bool FromString( const char* str, eMovementOrders & value );

FEX int MakeInt( eMovementOrders mo );

FEX eMovementOrders MakeMovementOrders( DWORD x );


/////////////////////////////////////////////////////////////
// eCombatOrders

enum eCombatOrders
{
	SET_BEGIN_ENUM( CO_, 0 ),

	CO_INVALID,

	CO_FREE,
	CO_LIMITED,
	CO_HOLD,

	SET_END_ENUM( CO_ )
};

const char* ToString( eCombatOrders value );
bool FromString( const char* str, eCombatOrders & value );


FEX int MakeInt( eCombatOrders co );

FEX eCombatOrders MakeCombatOrders( DWORD x );


/////////////////////////////////////////////////////////////
// eFocusOrders

enum eFocusOrders
{
	SET_BEGIN_ENUM( FO_, 0 ),

	FO_INVALID,
	FO_CLOSEST,
	FO_FARTHEST,
	FO_WEAKEST,
	FO_STRONGEST,

	SET_END_ENUM( FO_ )
};

const char* ToString( eFocusOrders value );
bool FromString( const char* str, eFocusOrders & value );


/////////////////////////////////////////////////////////////
// eRangeLimit

enum eRangeLimit
{
	SET_BEGIN_ENUM( RL_, 0 ),

    RL_INVALID,
	RL_NONE,
	RL_EFFECTIVE_ATTACK_RANGE,
	RL_EFFECTIVE_ATTACK_RESPONSE_RANGE,
	RL_ENGAGE_RANGE,
	RL_SIGHT_RANGE,

	SET_END_ENUM( RL_ )
};

const char* ToString( eRangeLimit value );
bool FromString( const char* str, eRangeLimit & value );


/////////////////////////////////////////////////////////////
//	eDoJobMode - mainly for skrit ai commands
enum eDoJobMode
{
	SET_BEGIN_ENUM( DJM_, 0 ),

	DJM_CV1,
	DJM_1V2,
	DJM_1VC,

	SET_END_ENUM( DJM_ )
};

const char* ToString( eDoJobMode value );
bool FromString( const char* str, eDoJobMode & value );


////////////////////////////////////////////////////////////////////////////////
//	JobResult

struct JobResult
{
	JobResult( Job * job );
	JobResult() { gpassertm( 0, "Invalid flow" ); }

	FUBI_VARIABLE_BYREF	( double,			m_, TimeFinished,			"Time when job was finished."				);
	FUBI_VARIABLE		( eJobResult,		m_, Result,					"Result at job finish."						);
	FUBI_VARIABLE		( DWORD,			m_, Count,					"Times this job was attempted"				);

	FUBI_VARIABLE		( eJobAbstractType, m_, Jat,		   	 		"Job Abstract Type" 						);
	FUBI_VARIABLE		( eJobTraits,		m_, Traits,					"Job Traits"								);
	FUBI_VARIABLE		( eActionOrigin,	m_, Origin,    	 			"Job assignment origin"   					);

	FUBI_VARIABLE		( Goid,             m_, GoalObject,   	 		"Job param - goal object"  					);
	FUBI_VARIABLE		( Goid,             m_, GoalModifier,   	 	"Job param - goal modifier"					);
	FUBI_VARIABLE_BYREF	( SiegePos,         m_, GoalPosition, 	 		"Job param - goal position"  				);
};

typedef stdx::fast_vector< JobResult * > JobResultColl;


//////////////////////////////////////////////////////////////////////////////
// general traits for jobs

typedef std::map< eJobAbstractType, eJobTraits > JobTraitMap;
const char * GetJobWaypointName( eJobAbstractType jat );


//////////////////////////////////////////////////////////////////////////////
// class GoMind declaration

typedef stdx::fast_vector< Job * > JobQueue;
typedef std::map< Goid, DWORD > EngagedMeMap;


class GoMind : public GoComponent
{

public:

	struct VisibilityInfo
	{
		VisibilityInfo()
		{
			ZeroObject( *this );
		};

		VisibilityInfo( Goid const object, SiegePos const & pos, bool visible )
		{
			ZeroObject( *this );
			m_Object              = object;
			m_LastPositionVisible = pos;
			m_Visible			  = visible;
		};

		Goid		m_Object;
		SiegePos	m_LastPositionVisible;
		bool		m_Visible				: 1;
		bool 		m_MarkedForDeletion		: 1;
		bool		m_BlocksPath			: 1;
		bool		m_Visited				: 1;
	};

	typedef std::multimap< float, VisibilityInfo * > DistanceVisibleMap;
	typedef std::multimap< Goid, VisibilityInfo * > GoidVisibleMap;

	SET_INHERITED( GoMind, GoComponent );

	enum eOptions
	{
		OPTION_NONE						=      0,

		OPTION_UPDATE_COMPUTER_BRAINS	= 1 << 0,
		OPTION_UPDATE_COMPUTER_ACTIONS	= 1 << 1,
		OPTION_UPDATE_COMPUTER			= OPTION_UPDATE_COMPUTER_BRAINS | OPTION_UPDATE_COMPUTER_ACTIONS,

		OPTION_UPDATE_HUMAN_BRAINS		= 1 << 2,
		OPTION_UPDATE_HUMAN_ACTIONS		= 1 << 3,
		OPTION_UPDATE_HUMAN				= OPTION_UPDATE_HUMAN_BRAINS | OPTION_UPDATE_HUMAN_ACTIONS,

		OPTION_DRAW_HUMAN_WAYPOINTS		= 1 << 4,

		OPTION_UPDATE_ALL				= OPTION_UPDATE_HUMAN | OPTION_UPDATE_COMPUTER,

		OPTION_UPDATE_BRAINS			= OPTION_UPDATE_COMPUTER_BRAINS | OPTION_UPDATE_HUMAN_BRAINS,
		OPTION_UPDATE_ACTIONS			= OPTION_UPDATE_COMPUTER_ACTIONS | OPTION_UPDATE_HUMAN_ACTIONS,
	};

// Setup.

	// ctor/dtor
	GoMind( Go* parent );
	GoMind( const GoMind& source, Go* parent );
	virtual ~GoMind( void );

	friend class DebugProbe;
	friend class Job;

// Overrides.
	
	virtual void HandleMessage( WorldMessage const & Message );
	virtual void HandleCcMessage( WorldMessage const & Message );
	virtual void Update( float timeDelta );
	virtual void Draw();
#	if !GP_RETAIL
	virtual void DrawDebugHud();
	void DrawDebugRange( float range, DWORD color, const char * label );
	void DrawDebugRangeOffsetAngle( float range, float offsetangle, DWORD color, const char * label );
#	endif // !GP_RETAIL

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newParent );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );
	virtual bool         XferCharacter( FuBi::PersistContext& persist );

	virtual bool CommitCreation();
	virtual bool CommitLoad();
	virtual void DetectWantsBits()									{ SetWantsUpdates(); SetWantsDraws(); }


// interface


	FEX void SetInitialCommand( Scid command )			{ gpassertm( command != SCID_INVALID, "Setting initial command to invalid Scid." ); m_InitialSECommand = command;  m_GiveInitialSECommand = true; }
FEX Scid GetInitialCommand()							{ return m_InitialSECommand; }
FEX bool GetBrainActive()								{ return m_BrainActive; }

	FrustumId GetFrustumOnEntry()						{ return m_FrustumOnEntry; }

	void DrawActionQueue();

	// global Mind configuration
	static void SetOptions   ( eOptions options, bool set = true )	{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	static void ClearOptions ( eOptions options )					{  SetOptions( options, false );  }
	static void ToggleOptions( eOptions options )					{  m_Options = (eOptions)(m_Options ^ options);  }
	static bool TestOptions  ( eOptions options )					{  return ( (m_Options & options) != 0 );  }
	static bool TestOptionsEq( eOptions options )					{  return ( (m_Options & options) == options );  }

	// instance-level Mind configuration
FEX void SetActorAutoFidgets( bool flag )							{  m_ActorAutoFidgets = flag; }
FEX bool GetActorAutoFidgets()										{  return ( m_ActorAutoFidgets ); }


	//============================================================================================
	//
	//	Job commands... these result in jobs being pushed onto an actors action queue
	//
	//--------------------------------------------------------------------------------------------


	//===============================================
	//	move
	//-----------------------------------------------

	bool MayMove();

FEX void RSMove( SiegePos const & Pos, eQPlace place, eActionOrigin origin );

	//===============================================
	//	stop
	//-----------------------------------------------
FEX void RSStop( eActionOrigin origin );

	//===============================================
	//	guard
	//-----------------------------------------------
FEX void RSGuard(	Go * target, eQPlace place, eActionOrigin origin  );

	//===============================================
	//	give/get/drop
	//-----------------------------------------------
FEX void RSGive( Go * target, Go * item, eQPlace place, eActionOrigin origin );
FEX void RSGet( Go * item, eQPlace place, eActionOrigin origin );
FEX void RSDrop( Go * item, SiegePos Pos, eQPlace QPlace, eActionOrigin origin );
FEX void RSDropGold( int gold, SiegePos Pos, eQPlace QPlace, eActionOrigin origin );

	//===============================================
	//	magic/item usage
	//-----------------------------------------------
FEX void RSUse( Go * item, eQPlace place, eActionOrigin origin  );

FEX void RSDrinkLifeHealingPotion( eActionOrigin jo );
FEX void RSDrinkManaHealingPotion( eActionOrigin jo );
	void SDrinkLifeOrManaPotion( eActionOrigin jo, bool bLifePotion );
	bool RetrieveNextPotion( Go ** pPotion, GopColl & potions );
FEX void RSCastLifeHealingSpell( eActionOrigin jo );

	////////////////////////////////////////
	//	alliance

FEX bool IsEnemy( const Go * target ) const;
FEX bool IsFriend( const Go * target ) const;

FEX	int  GetAllignedTeam() const			{ return( m_AllignedTeam ); }
FEX void SetAllignedTeam( int team )		{ m_AllignedTeam = team; }

	////////////////////////////////////////
	//	inventory

FEX bool GetAutoItems( GopColl & out ) const;
FEX bool GetAutoItems( eQueryTrait itemTrait, GopColl & out ) const;
FEX bool GetAutoItems( QtColl & qtc, GopColl & out ) const;

	////////////////////////////////////////
	//	party-related

FEX	DWORD GetRank()				{ return( m_Rank ); }
FEX	void SetRank( DWORD rank )	{ /*sanity*/gpassert( rank < 64 || rank == -1 ); m_Rank = rank; }

	////////////////////////////////////////////////////////////////////////////////
	//	spatial queries

FEX bool HaveOccupantsInSphere( float const r ) const;
FEX bool GetOccupantsInSphere( float const r, GopColl & output ) const;

FEX bool GetEnemiesInSphere( float const r, GopColl * output = NULL ) const;
FEX bool HasEnemiesInSphere( float const r ) const;
	
FEX bool GetFriendsInSphere( float const r, GopColl & output ) const;
FEX bool HasPartyMembersInSphere( float const r ) const;
FEX bool GetPartyMembersInSphere( float const r, GopColl * output = NULL ) const;

FEX bool IsLosClear( Go * target ) const;
FEX bool IsLosClear( SiegePos const & target ) const;
FEX bool IsInVisibilityCone( Go * target ) const;
FEX bool IsInRange( Go * target, float range ) const;
FEX bool IsIn2DRange( Go * target, float range ) const;

	////////////////////////////////////////////////////////////////////////////////
	//	weapon ranges

FEX float GetWeaponRange() const;

FEX bool IsInRange( Go * target, Go * weapon, eRangeLimit limit ) const;
	bool IsInEffectiveAttackRange( Go * target, Go * weapon, bool respondingToAttack ) const;

	bool IsInMeleeContactRange( Go * target, Go * weapon ) const;
FEX bool IsInSpellRange( Go * target, Go * spell ) const;

FEX bool IsInMeleeEngageRange(  Go * target ) const;
FEX bool IsInRangedEngageRange( Go * target ) const;
FEX bool IsInEngageRange( Go * target, Go * weapon ) const;

FEX bool GetPositionBehind( Go * target, SiegePos * pos ) const;

	// distance between the personal spaces of two game objects... if the object personal space volumes overlap, the distance clips to 0.0
FEX float GetDistance( Go * pTo ) const;
FEX float GetDistanceAtPlanEnd( Go * pGo ) const;
FEX float Get2DDistance( Go * pTo ) const;

FEX bool AmFacing( Go * target ) const;
FEX bool AmFacing( SiegePos const & target ) const;

FEX bool PickSafeSpotAwayFrom( Go * target, float distance, SiegePos & pos ) const;

	////////////////////////////////////////////////////////////////////////////////
	//	sensors
	
FEX void ResetSensors();

FEX void RequestProcessHitEngaged()							{ m_RequestProcessHitEngaged = true; }
	
		// send message to all actors engaged with me, EXCEPT the ignored one
	void MessageEngagedMe( WorldMessage	const & msg, Goid ignore = GOID_INVALID );
	void MessageVisible( WorldMessage const & msg );
FEX	void ReqResetSensorsSelfAndVisible();

	void RegisterEngagedMe( Goid engagedBy );
	void UnregisterEngagedMe( Goid engagedBy );

	void SetIsRidingElevator();
FEX bool IsRidingElevator() const;

	////////////////////////////////////////////////////////////////////////////////
	//	sensor queries

FEX bool IsInterruptable() const;

FEX float GetSensorScanPeriod()	const;

FEX Goid GetEngagedObject();
FEX	void RCSetClientCachedEngagedObject( Goid obj );

FEX bool AliveEnemiesVisible();
FEX bool AliveFriendsVisible();
FEX bool ItemsVisible() const;

	GoidColl const & GetEnemiesVisible() 					{ return m_EnemiesVisible; }
	GoidColl const & GetFriendsVisible()					{ return m_FriendsVisible; }
	GoidColl const & GetItemsVisible()						{ return m_ItemsVisible; }

FEX SiegePos & GetLastExecutedUserAssignedActionPosition();

FEX bool GetVisible( eQueryTrait trait, GopColl & out, float range ) const;
FEX bool GetVisible( QtColl & qtc, GopColl & out, float range ) const;
FEX Go * GetVisible( QtColl & qtc, float range ) const;
FEX Go * GetVisible( eQueryTrait trait, float range ) const;

FEX bool GetVisible( eQueryTrait trait, GopColl & out ) const	{ return GetVisible( trait, out, FLT_MAX ); }
FEX bool GetVisible( QtColl & qtc, GopColl & out ) const		{ return GetVisible( qtc, out, FLT_MAX ); }
FEX Go * GetVisible( QtColl & qtc ) const						{ return GetVisible( qtc, FLT_MAX ); }
FEX Go * GetVisible( eQueryTrait trait ) const					{ return GetVisible( trait, FLT_MAX ); }

FEX bool GetClosestVisible( eQueryTrait trait, GopColl & out ) const;
FEX bool GetClosestVisible( QtColl & qtc, GopColl & out ) const;
FEX Go * GetClosestVisible( QtColl & qtc ) const;
FEX Go * GetClosestVisible( eQueryTrait trait ) const;

FEX bool GetFarthestVisible( eQueryTrait trait, GopColl & out ) const;
FEX bool GetFarthestVisible( QtColl & qtc, GopColl & out ) const;
FEX Go * GetFarthestVisible( QtColl & qtc ) const;
FEX Go * GetFarthestVisible( eQueryTrait trait ) const;

	DistanceVisibleMap const & GetDistanceVisibleMap()		{ return m_DistanceVisibleMap; }
	GoidColl & GetActorsInInnerComfortZone()				{ return m_ActorsInInnerComfortZone; }
	GoidColl & GetActorsInOuterComfortZone()				{ return m_ActorsInOuterComfortZone; }
	EngagedMeMap const & GetEngagedMeMap() const			{ return m_EngagedMeMap; }

FEX bool IsLifeLow() const;
FEX bool IsLifeHigh() const;
FEX bool IsManaLow() const;
FEX bool IsManaHigh() const;

FEX Go * GetBestFocusEnemy() const;

FEX bool GetEngagedMe( GopColl & out ) const;
FEX bool GetEngagedMeEnemies( GopColl & out ) const;

FEX int GetEngagedMeAttackerCount() const;
FEX int GetEngagedMeMeleeAttackerCount() const;
FEX int GetEngagedMeRangedAttackerCount() const;

FEX float GetTimeElapsedSinceLastEnemySpotted() const;


	////////////////////////////////////////////////////////////////////////////////
	//	sensor properties

FEX float GetSightRange()              					  	const { return m_SightRange; }
FEX float GetSightOriginHeight()							const { return m_SightOriginHeight; }
FEX float GetVisibilityMemoryDuration()					  	const { return m_VisibilityMemoryDuration; }

FEX float GetJobTravelDistanceLimit()       				const { return m_JobTravelDistanceLimit; }
FEX float GetLimitedMovementRange()							const { return m_LimitedMovementRange; }

FEX float GetMeleeEngageRange()        					  	const { return m_MeleeEngageRange; }
FEX float GetRangedEngageRange()       					  	const;
	float GetMeleeSeparationRange()							const;
 
FEX float GetPersonalSpaceRange()      					  	const { return m_PersonalSpaceRange; }
	
FEX float GetInnerComfortZoneRange()   					  	const { return m_InnerComfortZoneRange; }
FEX float GetOuterComfortZoneRange()   					  	const { return m_OuterComfortZoneRange; }

	float GetSightFov() 									const { return m_SightFov; } // Field Of View half-angle in Radians
	float GetActorLifeRatioLowThreshold() 					const { return m_ActorLifeRatioLowThreshold; }
	float GetActorManaRatioLowThreshold() 					const { return m_ActorManaRatioLowThreshold; }
	float GetActorLifeRatioHighThreshold() 					const { return m_ActorLifeRatioHighThreshold; }
	float GetActorManaRatioHighThreshold() 					const { return m_ActorManaRatioHighThreshold; }

	////////////////////////////////////////////////////////////////////////////////
	//	job properties

FEX eMovementOrders   GetMovementOrders()    				const { return( m_MovementOrders    ); }
FEX eCombatOrders     GetCombatOrders()      				const { return( m_CombatOrders      ); }
FEX eFocusOrders      GetFocusOrders()       				const { return( m_FocusOrders       ); }
FEX eActorDisposition GetDispositionOrders() 				const { return( m_DispositionOrders ); }

FEX void RSSetMovementOrders( eMovementOrders Order );
FEX void RSSetCombatOrders( eCombatOrders Order );
FEX void RSSetFocusOrders( eFocusOrders Order );
FEX void RSSetDispositionOrders( eActorDisposition Order );

FEX void RCSetMovementOrders( eMovementOrders Order );
FEX void RCSetCombatOrders( eCombatOrders Order );
FEX void RCSetFocusOrders( eFocusOrders Order );
FEX void RCSetDispositionOrders( eActorDisposition Order );

FEX float GetFleeDistance()            					  	const { return m_FleeDistance; }
FEX int   GetFleeCount()               					  	const { return m_FleeCount; }
FEX float GetComfort();

	////////////////////////////////////////////////////////////////////////////////
	//	basic permissions

FEX	bool GetMayProcessAI()									const	{ return m_MayProcessAI; }
FEX	void SetMayProcessAI( bool flag )								{ m_MayProcessAI = flag; }

FEX	bool GetMayAttack()										const	{ return m_MayAttack && m_CombatOrders != CO_HOLD; }
FEX	void SetMayAttack( bool flag )									{ m_MayAttack = flag; }

FEX	bool GetMayBeAttacked()									const	{ return m_MayBeAttacked; }
FEX	void SetMayBeAttacked( bool flag)								{ m_MayBeAttacked = flag; }

	////////////////////////////////////////////////////////////////////////////////
	//	job permissions

	// short-hand methods for skrit
FEX bool MayAutoUseMeleeWeapon();			// does actor have active melee weapon AND the permission to auto use it
FEX bool MayAutoUseRangedWeapon();			// does actor have active ranged weapon AND the permission to auto use it
FEX bool MayAutoCastLifeDamagingSpell();	// does actor have active offensive spell AND the permission to auto use it
FEX bool MayAutoCastLifeHealingSpell();		// does actor have active offensive spell AND the permission to auto use it

	// general auto permissions
FEX bool ActorAutoDefendsOthers()           				const { return( m_ActorAutoDefendsOthers           			); }
FEX bool ActorAutoHealsOthersLife()         				const { return( m_ActorAutoHealsOthersLife         			); }
FEX bool ActorAutoHealsOthersMana()         				const { return( m_ActorAutoHealsOthersMana         			); }
FEX bool ActorAutoHealsSelfLife()           				const { return( m_ActorAutoHealsSelfLife           			); }
FEX bool ActorAutoHealsSelfMana()           				const { return( m_ActorAutoHealsSelfMana           			); }
FEX bool ActorAutoPicksUpItems()            				const { return( m_ActorAutoPicksUpItems            			); }
FEX bool ActorAutoXfersMana()               				const { return( m_ActorAutoXfersMana               			); }
FEX bool ActorAutoReanimatesFriends()						const { return( m_ActorAutoReanimatesFriends				); }

FEX bool ActorPrefersMelee()								const { return( m_WeaponPreference == WP_MELEE				); }
FEX bool ActorPrefersRanged()								const { return( m_WeaponPreference == WP_RANGED				); }
FEX bool ActorPrefersMagic()								const { return( m_WeaponPreference == WP_MAGIC				); }

FEX void SetActorWeaponPreference( eWeaponPreference value ) { m_WeaponPreference = value; }

FEX bool ActorAutoSwitchesToKarate() 						const { return( m_ActorAutoSwitchesToKarate 				); }
FEX bool ActorAutoSwitchesToMelee()  						const { return( m_ActorAutoSwitchesToMelee  				); }
FEX bool ActorAutoSwitchesToRanged() 						const { return( m_ActorAutoSwitchesToRanged 				); }
FEX bool ActorAutoSwitchesToMagic()  						const { return( m_ActorAutoSwitchesToMagic  				); }

FEX bool ActorAutoUsesStoredItems()							const { return( m_ActorAutoUsesStoredItems					); }

FEX float ActorBalancedAttackPreference()					const { return( m_ActorBalancedAttackPreference				); }

	// specific event reflex permissions
FEX bool OnEnemySpottedAlertFriends()       				const { return( m_OnEnemySpottedAlertFriends       			); }
FEX bool OnEnemySpottedAttack()             				const { return( m_OnEnemySpottedAttack             			); }
FEX bool OnEnemySpottedFlee()               				const { return( m_OnEnemySpottedFlee               			); }

FEX bool OnAlertProjectileNearMissedAttack()				const { return( m_OnAlertProjectileNearMissedAttack			); }
FEX bool OnAlertProjectileNearMissedFlee()					const { return( m_OnAlertProjectileNearMissedFlee			); }
FEX bool OnEnemyEnteredICZAttack()          				const { return( m_OnEnemyEnteredICZAttack          			); }
FEX bool OnEnemyEnteredICZFlee()            				const { return( m_OnEnemyEnteredICZFlee            			); }
FEX bool OnEnemyEnteredICZSwitchToMelee()            		const { return( m_OnEnemyEnteredICZSwitchToMelee  			); }
FEX bool OnEnemyEnteredOCZAttack()          				const { return( m_OnEnemyEnteredOCZAttack          			); }
FEX bool OnEnemyEnteredOCZFlee()            				const { return( m_OnEnemyEnteredOCZFlee            			); }
FEX bool OnEnemyEnteredWeaponEngageRangeAttack() 			const { return( m_OnEnemyEnteredWeaponEngageRangeAttack  	); }

FEX bool OnFriendEnteredICZAttack()         				const { return( m_OnFriendEnteredICZAttack         			); }
FEX bool OnFriendEnteredICZFlee()           				const { return( m_OnFriendEnteredICZFlee           			); }
FEX bool OnFriendEnteredOCZAttack()         				const { return( m_OnFriendEnteredOCZAttack         			); }
FEX bool OnFriendEnteredOCZFlee()           				const { return( m_OnFriendEnteredOCZFlee           			); }

FEX bool OnLifeRatioLowFlee()               				const { return( m_OnLifeRatioLowFlee               			); }
FEX bool OnManaRatioLowFlee()               				const { return( m_OnManaRatioLowFlee               			); }

FEX bool OnJobReachedTravelDistanceAbortAttack()  			const { return( m_OnJobReachedTravelDistanceAbortAttack		); }

FEX bool OnEngagedLostLoiter()              				const { return( m_OnEngagedLostLoiter              			); }
FEX bool OnEngagedLostReturnToJobOrigin()   				const { return( m_OnEngagedLostReturnToJobOrigin   			); }
FEX bool OnEngagedFleedAbortAttack()              			const { return( m_OnEngagedFledAbortAttack        			); }
FEX bool OnEngagedLostConsciousnessAbortAttack()			const { return( m_OnEngagedLostConsciousnessAbortAttack		); }

	
	//============================================================================================
	//
	//	job queue manipulation
	//
	//--------------------------------------------------------------------------------------------

FEX	bool UnderstandsJob( eJobAbstractType eType ) const;

FEX Job * SDoJob( JobReq const & req );
FEX void RSDoJob( JobReq const & req );
FEX void RSDoJobPacker( const_mem_ptr packola );
FEX bool MaySDoJob( eJobAbstractType jat );

	bool Validate();
	bool Validate( JobReq const & req );
	bool CheckActionCollision( JobReq const & req );
	void ExamineEngaged();

FEX	bool DoingJobOriginatingFrom( eJobQ q, Go * origin );



FEX Job * GetFrontJob( eJobQ Q ) const;
FEX eJobAbstractType GetFrontActionJat() const;
FEX bool IsCurrentActionHumanInterruptable() const;

	void Clear( JobQueue & q );
FEX void Clear( eJobQ q );
FEX void Clear();
FEX void ClearAndCommit( eJobQ q );

	JobQueue const & GetQ( eJobQ Q ) const;
	JobQueue & GetQ( eJobQ Q );

	void MarkAllForDeletion( eJobQ Q );

	bool AmBusy( eJobQ Q ) const;
FEX bool AmBusy() const										{ return AmBusy( JQ_ACTION ); }
FEX bool AmPatrolling() const;

	// $$ ai rename
	Job * GetNextJob( Job * pJob );

FEX bool Send( WorldMessage const & msg );
FEX bool SendDelayed( WorldMessage const & msg, float delay );

	// util

	bool CanOperateOn( Go * go, bool silentError = true ) const;
FEX bool CanOperateOn( Go * go )							{ return( CanOperateOn( go, true ) ); }

	Skrit::HAutoObject const * GetJobSkritCloneSource( eJobAbstractType jat );

	//==========================================
	//	debug
	//------------------------------------------

#	if !GP_RETAIL
FEX	void Dump( bool verbose, ReportSys::Context * ctx );
FEX	void Dump2( bool verbose, ReportSys::Context * ctx );
#	endif



private:

	typedef stdx::fast_vector< JobLookupEntry > JobLookupEntryColl;

	void PrivateHandleMessage( WorldMessage const & msg );

FEX	WorldMessage * GetHandlingMessage() { gpassert( m_pHandlingMessage ); return( m_pHandlingMessage ); }
 
	void DrawActionWaypoint( SiegePos const & from, SiegePos const & to, Job * action );

	void DeleteJobsNotActingInFrustum( FrustumId const frustumId );
	void OnFrustumMembershipChanged( FrustumId const oldMemberhip );

	void OnFrustumGainedActiveState();
	void OnFrustumLostActiveState();

	////////////////////////////////////////////////////////////////////////////////
	//	the Q

	bool HasJob( JobQueue const & q, Job * job );

	void Remove( Job * job, eJobQ Q );

	JobQueue::iterator FindInsertionPoint( JobQueue & q, eJobTraits traits, eQPlace place );

	void MoveMarkedJobsToDeletionQ( bool leavingWorld );
	void ProcessDeletionQueue();
	void ProcessRequestHitEngaged();

	void ClearTemps();

	void OnJobAdded( Job * job );
	void OnJobEnterAtomicState( Job * job, float duration );
	void PostJobAnyEvent( Job * job, WorldMessage const * msg = NULL );
	//void ProcessActiveAction( Job * job );

	////////////////////////////////////////////////////////////////////////////////
	//	sensors

	void ProcessSensors();

		void ProcessStatsSensors();
		void ProcessSpatialSensors();
		void ProcessCombatSensors();

	void FlushCaches();

	////////////////////////////////////////
	//	memory

	void UpdateJobResults();

	void RegisterActionResult( Job * job );

	JobResult * FindActionResult( JobReq const & req );
	JobResult * FindActionResult( Job * job );
FEX bool JobRecentlyFailedToPathTo( Go * target );

	////////////////////////////////////////
	//	visibility

	bool IsVisible( Goid object ) const;
	bool IsInVisibleCache( Go * object, bool & visible );

	// job existence

	bool Create( Job * & pJob, eJobAbstractType AType );

	void InsertJob( Job * job, JobReq const & req, bool clearAfterInsertionPoint );
	void InsertJob( Job * job, JobQueue & q, JobQueue::iterator insertionPoint, JobReq const & req, bool clearAfterInsertionPoint );

	// temp storage for owned code and skrit jobs

FEX GopColl & GetTempGopColl1()				{ return m_TempGopColl1 ; }
FEX GopColl & GetTempGopColl2()				{ return m_TempGopColl2 ; }
FEX GopColl & GetTempGopColl3()				{ return m_TempGopColl3 ; }
FEX QtColl & GetTempQtColl1()				{ return m_TempQtColl1; }
FEX SiegePos & GetTempPos1()				{ return m_TempPos1; }
FEX vector_3 & GetTempVector1()				{ return m_TempVector1; }


	////////////////////////////////////////////////////////////////////////////////
	//	state
	//

	static eOptions m_Options;

	////////////////////////////////////////
	//	critical flags

	bool m_ShuttingDown							: 1;
	bool m_BrainActive							: 1;
	bool m_FrustumMembershipChanged				: 1;
	bool m_GiveInitialSECommand					: 1;
	bool m_InQueueDeletionScope					: 1;
	bool m_InUpdateScope						: 1;
	bool m_ReqClear								: 1;
	bool m_ReqDoLastSECommand					: 1;
	bool m_ReqGiveStartupJob					: 1;
	bool m_ReqInitBrain							: 1;

	////////////////////////////////////////
	//	timers
	float				m_IdleTimer;
	float				m_IdleTimerTrigger;
	double				m_NextTimeAttackAllowed;
	double				m_NextTimeCastAllowed;
	double          	m_NextSensorScanTime;            

	////////////////////////////////////////
	//	misc
	Scid				m_InitialSECommand;
	Scid				m_LastSECommand;
	double				m_LastSECommandPathFailureTime;
	WorldMessage *		m_pHandlingMessage;
	DWORD 				m_HandleMessageEntryCount;
	DWORD 				m_Rank;
	bool				m_bEnemySpottedPlayed;
	int					m_AllignedTeam;

	////////////////////////////////////////
	//	jobs
 	JobQueue			m_Brains;
	JobQueue			m_Actions;
	JobQueue			m_Deletion;

	////////////////////////////////////////
	//	sensors
 	float           	m_ActorLifeRatioLast;            
	float           	m_ActorManaRatioLast;            
	DWORD           	m_LastActionProcessedId;         
	eJobAbstractType	m_LastCurrentBaseJat;            
	DWORD           	m_LastJobReachedTravelDistanceId;

	bool				m_RequestFlushSensors			: 1;
	bool         		m_RequestProcessCombatSensors 	: 1;
	bool         		m_RequestProcessHitEngaged    	: 1;
	bool				m_AliveFriendsVisibleDirty		: 1;
	bool				m_AliveFriendsVisible			: 1;
	bool				m_AliveEnemiesVisibleDirty		: 1;
	bool				m_AliveEnemiesVisible			: 1;

	EngagedMeMap 		m_EngagedMeMap;              
	GoidColl     		m_ActorsInInnerComfortZone;  
	GoidColl     		m_ActorsInOuterComfortZone;  
	GoidColl     		m_EnemiesVisible;            
	GoidColl     		m_FriendsVisible;            
	GoidColl     		m_ItemsVisible;
	GoidColl			m_PathBlockers;

	SiegePos			m_LastExecutedUserAssignedActionPosition;

	float				m_ActionResultMemoryLength;
	JobResultColl		m_ActionResultColl;

	double				m_LastElevatorRideTime;

	////////////////////////////////////////
	//	visibility

	DistanceVisibleMap 	m_DistanceVisibleMap;
	GoidVisibleMap		m_GoidVisibleMap;
	GoidSet 			m_VisibleSet;
	SiegePos			m_MyPositionLastVisibilityCheck;
	double				m_TimeEnemySpottedLast;

	////////////////////////////////////////
	//	temp storage

	Goid		m_ClientCachedEngagedObject;

	GopColl		m_TempGopColl1;
	GopColl		m_TempGopColl2;
	GopColl		m_TempGopColl3;
	QtColl		m_TempQtColl1;
	SiegePos	m_TempPos1;
	vector_3	m_TempVector1;
	FrustumId	m_FrustumOnEntry;

	typedef stdx::fast_vector< WorldMessage > WorldMessageColl;
	WorldMessageColl m_DelayedMessages;

	////////////////////////////////////////////////////////////////////////////////
	//	SPEC

	////////////////////////////////////////
	//	job permissions

	bool m_ActorAutoFidgets							: 1;

	bool m_MayProcessAI								: 1;
	
	bool m_MayAttack								: 1;
	bool m_MayBeAttacked							: 1;

	// auto behavior permissions
	bool m_ActorAutoDefendsOthers              		: 1;
	bool m_ActorAutoHealsOthersLife            		: 1;
	bool m_ActorAutoHealsOthersMana            		: 1;
	bool m_ActorAutoHealsSelfLife              		: 1;
	bool m_ActorAutoHealsSelfMana              		: 1;
	bool m_ActorAutoPicksUpItems               		: 1;
	bool m_ActorAutoXfersMana                  		: 1;
	bool m_ActorAutoReanimatesFriends				: 1;

	eWeaponPreference m_WeaponPreference;

	bool m_ActorAutoSwitchesToKarate 				: 1;
	bool m_ActorAutoSwitchesToMelee  				: 1;
	bool m_ActorAutoSwitchesToRanged 				: 1;
	bool m_ActorAutoSwitchesToMagic  				: 1;

	bool m_ActorAutoUsesStoredItems					: 1;

	float m_ActorBalancedAttackPreference;

	// specific event reflex permissions
	bool m_OnAlertProjectileNearMissedAttack		: 1;
	bool m_OnAlertProjectileNearMissedFlee			: 1;
	bool m_OnEnemyEnteredWeaponEngageRangeFlee		: 1;
	bool m_OnEnemyEnteredICZAttack			   		: 1;
	bool m_OnEnemyEnteredICZFlee			   		: 1;
	bool m_OnEnemyEnteredICZSwitchToMelee			: 1;
	bool m_OnEnemyEnteredOCZAttack			   		: 1;
	bool m_OnEnemyEnteredOCZFlee			   		: 1;
	bool m_OnEnemyEnteredWeaponEngageRangeAttack	: 1;
	bool m_OnEnemySpottedAlertFriends		   		: 1;
	bool m_OnEnemySpottedAttack				   		: 1;
	bool m_OnEnemySpottedFlee				   		: 1;
	bool m_OnEngagedFledAbortAttack		   			: 1;
	bool m_OnEngagedLostConsciousnessAbortAttack	: 1;
	bool m_OnEngagedLostLoiter				   		: 1;
	bool m_OnEngagedLostReturnToJobOrigin	   		: 1;
	bool m_OnFriendEnteredICZAttack			   		: 1;
	bool m_OnFriendEnteredICZFlee			   		: 1;
	bool m_OnFriendEnteredOCZAttack			   		: 1;
	bool m_OnFriendEnteredOCZFlee			   		: 1;
	bool m_OnJobReachedTravelDistanceAbortAttack	: 1;
	bool m_OnLifeRatioLowFlee				   		: 1;
	bool m_OnManaRatioLowFlee				   		: 1;

#if	!GP_RETAIL
	bool m_DebugWeKilledReceived					: 1;
	bool m_DebugWeLostConsciousnessReceived			: 1;
	bool m_DebugWeEngagedLostConsciousnessRecevied	: 1;
#	endif

	////////////////////////////////////////
	//	standing orders

	eActorDisposition	m_DispositionOrders;
	eMovementOrders		m_MovementOrders;
	eCombatOrders		m_CombatOrders;
	eFocusOrders		m_FocusOrders;

	////////////////////////////////////////
	//	job params

	float	m_FleeDistance;
	float	m_MinComfortDelayThreshold;
	float	m_MaxComfortDelayThreshold;
	int		m_FleeCount;

	////////////////////////////////////////
	//	sensor parameters

	float 		m_ActorLifeRatioHighThreshold;
	float 		m_ActorLifeRatioLowThreshold;
	float 		m_ActorManaRatioHighThreshold;
	float 		m_ActorManaRatioLowThreshold;
	float		m_ComRange;
	float		m_InnerComfortZoneRange;
	float		m_JobTravelDistanceLimit;				
	float		m_LimitedMovementRange;
	float		m_MeleeEngageRange;					// range at which to attack/engage an enemy with melee weapon
	float		m_OuterComfortZoneRange;
	float		m_PersonalSpaceRange;				// defines personal space volume which is used to influence how close we can get to an opponent
	float		m_RangedEngageRange;				// range at which to attack/engage an enemy with a shooting weapon
	float		m_SensorScanPeriod;
	float		m_SightFov;							// sight angle for character
	float		m_SightRange;						// sight range for fog of war
	float		m_SightOriginHeight;
	float		m_VisibilityMemoryDuration;			// for how long will ther actor remember the engaged object after it is no longer visible
	Membership	m_ComChannels;

	////////////////////////////////////////
	//	job definitions

	JobLookupEntryColl	m_JobLookupEntryColl;

	// RPC
	static DWORD   GoMindToNet( GoMind* x );
	static GoMind* NetToGoMind( DWORD d, FuBiCookie* cookie );

	SET_NO_COPYING( GoMind );
	FUBI_RPC_CLASS( GoMind, GoMindToNet, NetToGoMind, "" );
};


//////////////////////////////////////////////////////////////////////////////

#endif  // __GOMIND_H

//////////////////////////////////////////////////////////////////////////////
