/*=======================================================================================

  AIQ - "AI Queries"

  purpose	:	A collection of helper methods the AI uses to ask basic questions about the
				environment.

  author	:	Bartosz Kijanka, Scott Bilas

  (C)opyright Gas Powered Games 1999-2001

---------------------------------------------------------------------------------------*/
#ifndef __AIQUERY_H
#define __AIQUERY_H
#pragma once

//////////////////////////////////////////////////////////////////////////////

#include "godefs.h"
#include "siege_database_guid.h"

#include <set>

typedef std::pair< Go*, vector_3 >			GOVectorPair;
typedef stdx::fast_vector< GOVectorPair >	GOVectorColl;
typedef std::pair< Go*, SiegePos >			GONodePosPair;
typedef stdx::fast_vector< GONodePosPair >	GONodePosList;

#define AIQUERY_UBER_PARANOIA (!GP_RETAIL)

#if AIQUERY_UBER_PARANOIA
void gpbitchslap_aiq( const char* msg, const Go* go );
#else // AIQUERY_UBER_PARANOIA
#define gpbitchslap_aiq( msg, go )
#endif // AIQUERY_UBER_PARANOIA

//////////////////////////////////////////////////////////////////////////////
// enum eJobSpecificType declaration

enum eOccupantFilter
{
	// $ note: "me" in docs for filter bits is the 'sourceObject' passed in to
	//         the GetOccupantsInSphere test

	// pick one or more things to filter on
	OF_NONE					=	    0,				// no filter, choose any
	OF_SCREEN_CONTROLLED	= 1 <<  0,				// go's controlled by the screen player
	OF_HUMAN_CONTROLLED		= 1 <<  1,				// human controlled go's
	OF_COMPUTER_CONTROLLED	= 1 <<  2,				// computer controlled go's
	OF_ACTORS				= 1 <<  3,				// actors
	OF_ALIVE				= 1 <<  4,				// objects where IsAlive() is true
	OF_ALIVE_OR_GHOST		= 1 <<  5,				// objects where IsGhost() || IsAlive() is true
	OF_COLLIDABLES			= 1 <<  6,				// objects where GetAspect()->GetIsCollidable() is true
	OF_BREAKABLES			= 1 <<  7,				// objects where IsBreakable() is true
	OF_FRIENDS				= 1 <<  8,				// my friends
	OF_ENEMIES				= 1 <<  9,				// my enemies
	OF_PARTY_MEMBERS		= 1 << 10,				// my party members, or generic player's party if i'm null
	OF_CAN_BURN				= 1 << 11,				// objects that can burn (checks go->IsFireImmune)
	OF_ITEMS				= 1 << 12,				// items	
	OF_CAN_MOVE_SELF		= 1 << 13,				// objects that can move about on their own


	// if a go matches these, it will be culled out of list
	OF_CULL_PARTY_MEMBERS	= 1 << 16,				// cull my party members from list
	OF_CULL_SAME_MEMBERSHIP	= 1 << 17,				// cull objects with the same membership as me
	OF_CULL_ACTORS			= 1 << 18,				// cull any actors
	OF_CULL_GHOSTS			= 1 << 19,				// cull any ghosts
	OF_CULL_INVINCIBLE		= 1 << 20,				// cull any invincible

	// and this if you want it to match all of the filter flags (default is to match all)
	OF_MATCH_ANY			= 1 << 31,

	// special mask to strip off selection bits for filter matches (used internally)
	OF_MASK_ALL				= 0xFFFFFFFF & ~OF_MATCH_ANY,

	// mask used to strip off selection and ownership bits (used internally)
	OF_MASK_OWNER			= 0xFFFFFFFF & ~(OF_HUMAN_CONTROLLED | OF_COMPUTER_CONTROLLED | OF_SCREEN_CONTROLLED),

	// mask used to strip off selection and traits bits (used internally)
	OF_MASK_TRAITS			= OF_MASK_ALL & ~(OF_HUMAN_CONTROLLED | OF_COMPUTER_CONTROLLED | OF_SCREEN_CONTROLLED | OF_ACTORS | OF_ALIVE | OF_ALIVE_OR_GHOST),
};

MAKE_ENUM_BIT_OPERATORS( eOccupantFilter );

//////////////////////////////////////////////////////////////////////////////
// class AIQuery declaration

class AIQuery : public Singleton <AIQuery>
{
public:

	//----- setup

	// ctor/dtor
	AIQuery( void );
   ~AIQuery( void );

	// options
	void SetQueryCacheEnabled( bool set = true )			{  m_UseQueryCache = set;  }
	bool GetQueryCacheEnabled( void ) const					{  return ( m_UseQueryCache );  }

	// events
	void Shutdown         ( void );
	void Update           ( float secondsElapsed );
	void OnSpaceChanged   ( SiegeGuid guid );
	void OnNodeEnterWorld ( const SiegeGuid& guid );
	void OnNodeLeaveWorld ( const SiegeGuid& guid );
	void OnNodeTransition ( const SiegeGuid& guid );
	void OnNodeInvalidate ( const SiegeGuid& guid );
	void OnOccupantChanged( Go* occupant, SiegeGuid oldNode, SiegeGuid newNode, bool force = false );

	// query
#	if AIQUERY_UBER_PARANOIA
	bool HasGoInCache( Go* go ) const						{  return ( m_GoMetaQueryDb.find( go ) != m_GoMetaQueryDb.end() );  }
#	endif // AIQUERY_UBER_PARANOIA

	//----- occupant queries for node inhabitance

FEX bool GetScreenPartyMembersInNodes( int region, int section, int level, int object, GopColl& out );
FEX bool AreScreenPartyMembersInNodes( int region, int section, int level, int object );
	int  GetScreenPartyMembersInNodes( int region, int section, int level, int object, int earlyOutSize, GopColl* out );
FEX bool GetHumanPartyMembersInNodes ( int region, int section, int level, int object, GopColl& out );
FEX bool AreHumanPartyMembersInNodes ( int region, int section, int level, int object );
	int  GetHumanPartyMembersInNodes ( int region, int section, int level, int object, int earlyOutSize, GopColl* out );

	//----- filtering lists of objects

FEX bool Get      ( Go * reference, eQueryTrait qt, GopColl const & in, GopColl & out ) const;
FEX bool Get      ( Go * reference, QtColl const & qtc, GopColl const & in, GopColl & out ) const;
	bool Get      ( Go * reference, eQueryTrait const * qt, unsigned int qtCount, GopColl const & in, GopColl & out ) const;

FEX bool GetFirstN( Go * reference, eQueryTrait qt, unsigned int N, GopColl const & in, GopColl & out ) const;
FEX bool GetFirstN( Go * reference, QtColl const & qtc, unsigned int N, GopColl const & in, GopColl & out ) const;
	bool GetFirstN( Go * reference, eQueryTrait const * qt, unsigned int qtCount, unsigned int N, GopColl const & in, GopColl & out ) const;

FEX bool GetMaxN( Go * reference, eQueryTrait qt, unsigned int N, GopColl const & in, GopColl & out ) const;
FEX bool GetMaxN( Go * reference, QtColl const & qtc, unsigned int N, GopColl const & in, GopColl & out ) const;
FEX bool GetMaxN( Go * reference, eQueryTrait const * qt, unsigned int qtCount, unsigned int N, GopColl const & in, GopColl & out ) const;

FEX Go * GetMax ( Go * reference, eQueryTrait qt, GopColl const & in ) const;
FEX bool GetMax ( Go * reference, eQueryTrait qt, GopColl const & in, GopColl & out ) const;
FEX bool GetMax ( Go * reference, QtColl const & qtc, GopColl const & in, GopColl & out ) const;

FEX bool GetMinN( Go * reference, eQueryTrait qt, unsigned int N, GopColl const & in, GopColl & out ) const;
FEX bool GetMinN( Go * reference, QtColl const & qtc, unsigned int N, GopColl const & in, GopColl & out ) const;
FEX bool GetMinN( Go * reference, eQueryTrait const * qt, unsigned int qtCount, unsigned int N, GopColl const & in, GopColl & out ) const;

FEX Go * GetMin ( Go * reference, eQueryTrait qt, GopColl const & in ) const;
FEX bool GetMin ( Go * reference, eQueryTrait qt, GopColl const & in, GopColl & out ) const;
FEX bool GetMin ( Go * reference, QtColl const & qtc, GopColl const & in, GopColl & out ) const;

FEX int GetCount( Go * reference, QtColl const & qtc, GopColl const & in ) const;
FEX int GetCount( Go * reference, eQueryTrait qt, GopColl const & in ) const;

FEX bool Is( Go const * reference, Go const * go, eQueryTrait qt ) const;
FEX bool Is( Go const * reference, Go const * go, eQueryTrait qt, float & pNormAdjVal ) const;
FEX bool Is( Go const * reference, Go const * go, QtColl const & qtColl ) const;
FEX bool Is( Go const * reference, Go const * go, eQueryTrait const * qt, unsigned int qtCount ) const;

	bool Filter( const Go * go, const Go * source, eOccupantFilter filter, bool isUnionFilter = false ) const;

FEX float GetTraitValue( Go const * reference, Go const * go, eQueryTrait qt ) const;

	//----- spatial tests

FEX	bool IsInRange  ( SiegePos const & PosA, SiegePos const & PosB, float const Range );
	bool IsInRange2D( SiegePos const & PosA, SiegePos const & PosB, float const Range );

FEX	bool IsInRange  ( const Go* a, const Go* b, float range );
	bool IsInRange2D( const Go* a, const Go* b, float range );

FEX bool GetOccupantsInSphere                  ( const SiegePos& center, float radius, GopColl& out );
FEX bool DoesSphereHaveOccupants               ( const SiegePos& center, float radius );
FEX bool DoesSphereHaveHumanControlledOccupants( const SiegePos& center, float radius );

	int GetOccupantsInSphere(
			const SiegePos& center,				// center of query
			float           radius,				// radius of sphere, all objects within are filtered
			const Go*       sourceObject,		// optional source used with certain filters - also note that source is always culled from result set
			const GoidColl* ignoreColl,			// optional list of goids to ignore
			int             earlyOutSize,		// stop when you get to this size in filters (in increasing distance from center) - get all occupants if this is 0
			eOccupantFilter filter,				// only include objects that match this filter
			GopColl*        out,				// optional list of results to store occupants in
			bool            cache = true );		// cache the results of this query?

	// special version for party members only
	int GetPartyMembersInSphere(
			const SiegePos& center,
			float           radius,
			const Go*       party,
			const GoidColl* ignoreColl,
			int             earlyOutSize,
			eOccupantFilter filter,
			GopColl*        out );

	//----- geometry

	// returns world-space position
	vector_3 GetGeometricCenter( GoidColl const & List );

	// returns world-space position, and a list GO ptrs, and their respective offsets from the geometric center
	vector_3 GetGeometricCenterWithDeltas( GopColl const & List, GOVectorColl & PositionList );

	// returns world-space position, and a list GO ptrs, and their respective world-space positions
	vector_3 GetGeometricCenter( GopColl const & List, GOVectorColl & WorldPositionList );

	// returns world-space position
	bool GetGeometricCenter( GopColl const & List, vector_3 & Center );

	// distance between two points in space
	float    GetDistanceSquared ( SiegePos const & begin, SiegePos const & end );
FEX float    GetDistance        ( SiegePos const & begin, SiegePos const & end );
	float    Get2DDistance      ( SiegePos const & begin, SiegePos const & end );
	vector_3 GetDifferenceVector( SiegePos const & begin, SiegePos const & end );

	// distance between two go's
	float    GetDistance        ( const Go* a, const Go* b );
	float    Get2DDistance      ( const Go* a, const Go* b );
	vector_3 GetDifferenceVector( const Go* a, const Go* b );

	Go * GetClosest( Go const * origin, GopColl const & in );
	void SortByDistance( Go const * from, GopColl const & to, GopColl & out );

FEX bool GetTerrainPosition( SiegePos const & c, float r, SiegePos & pos );
FEX void GetTerrainPosition( SiegePos const & c, float r, float angle, SiegePos & out );
FEX bool GetTerrainPositionAtEnd( SiegePos const & begin, SiegePos const & end, float r, float minAngle, float maxAngle, SiegePos & out );
FEX bool GetTerrainPositionAtEnd( SiegePos const & begin, SiegePos const & end, float r, float alpha, SiegePos & out );

	//----- tactical

	float CalcOffenseFactor( Go const * actor ) const;
	float CalcSurvivalFactor( Go const * actor ) const;

FEX Go * CalcSecondTierTarget( Go const * attacker, Go const * target );

	//----- helpers/util

FEX void GetLOSPoint            (	Go const * go, SiegePos & point ) const;
FEX void GetLOSPoint            (	Go const * go, vector_3 & point ) const;
	bool IsLosClear             (	SiegePos const & from, SiegePos const & to, bool hud ) const;
FEX bool IsLosClear             (	SiegePos const & from, SiegePos const & to ) const;
FEX bool IsLosClear             (	Go const * from, Go const * to ) const;
FEX bool IsLosClear             (	Go const * from, SiegePos const & to ) const;
FEX bool FindClearLosPoint      (	Go const * from, Go const * to, float maxDist, float maxAngle, SiegePos & out );
FEX bool FindClearLosPoint      (	Go const * from, SiegePos const & to, float maxDist, float maxAngle, SiegePos & out );

	bool FindClearLosPoint      (	SiegePos const & from,
									SiegePos const & to,
									float maxDist,
									float maxAngle,
									siege::eLogicalNodeFlags nodeFlags,
									float GroundHugHeight,
									bool hud,
									SiegePos & out );

	bool FindClearLosPointOnLine(	SiegePos const & from,
									SiegePos const & to,
									float maxDist,
									float Angle,
									siege::eLogicalNodeFlags nodeFlags,
									float GroundHugHeight,
									bool hud,
									SiegePos & out );

	bool FindClearLosPointR(		SiegePos const & from,
									SiegePos const & to,
									vector_3 const & unitDir,
									float minDist,
									float maxDist,
									float closestMatchDistance,
									DWORD recursionDepth,
									DWORD recursionLimit,
									siege::eLogicalNodeFlags nodeFlags,
									float GroundHugHeight,
									bool hud,
									SiegePos & out );

FEX void GetRandomOrientation( Quat & q );

FEX bool IsAreaWalkable( siege::eLogicalNodeFlags nodeFlags, SiegePos const & center, DWORD numSamples, float radius );

FEX bool FindSpotForDrop(	Go * dropper,
							Go * dropFor,
							Go * dropItem,
							float diagLength,
							float maxDegreesAngularDeviance,
							float verticalDeltaConstraint,
							SiegePos & pos,
							bool bDropSame );

////////////////////////////////////////////////////////////////////////////////
//	
FEX bool FindSpotRelativeToSource(	SiegePos const * source,
									SiegePos const * target,
									siege::eLogicalNodeFlags nodeFlags,
									float pointBufferRadius,
									bool awayFromTarget,
									float minDistance,
									float maxDistance,
									float minDegreesAngularDeviance,
									float maxDegreesAngularDeviance,
									float verticalDistanceConstraint,
									bool showHuds,
									SiegePos & pos,
									bool bDropSame,
									bool bProjectBothSides );

////////////////////////////////////////////////////////////////////////////////
//
FEX bool FindSpotRelativeToSource(	SiegePos const * source,
									SiegePos const * target,
									siege::eLogicalNodeFlags nodeFlags,
									float pointBufferRadius,
									bool awayFromTarget,
									float minDistance,
									float maxDistance,
									float minDegreesAngularDeviance,
									float maxDegreesAngularDeviance,
									float verticalDistanceConstraint,
									bool showHuds,
									SiegePos & pos,
									bool bDropSame );
		
////////////////////////////////////////////////////////////////////////////////
//	
FEX bool FindSpotRelativeToSource(	SiegePos const * source,
									float pointBufferRadius,
									float minDistance,
									float maxDistance,
									float verticalDistanceConstraint,
									bool showHuds,
									SiegePos & pos );

////////////////////////////////////////////////////////////////////////////////
//	
FEX bool FindSpotRelativeToSource(	Go * source,
									Go * target,
									bool awayFromTarget,
									float minDistance,
									float maxDistance,
									float minDegreesAngularDeviance,
									float maxDegreesAngularDeviance,
									float verticalDistanceConstraint,
									SiegePos & pos, 
									bool bDropSame,
									bool bProjectBothSides );

////////////////////////////////////////////////////////////////////////////////
//	
FEX bool FindSpotRelativeToSource(	Go * source,
									Go * target,
									bool awayFromTarget,
									float minDistance,
									float maxDistance,
									float minDegreesAngularDeviance,
									float maxDegreesAngularDeviance,
									float verticalDistanceConstraint,
									SiegePos & pos,
									bool bDropSame );


////////////////////////////////////////////////////////////////////////////////
//	
FEX bool FindSpotRelativeToSource(	Go * source,
									float minDistance,
									float maxDistance,
									float verticalDistanceConstraint,
									SiegePos & pos );

	bool GetPositionsAroundNewCenter( GopColl const & Objects, SiegePos NewCenter, GONodePosList & PosList );

	// result will contain goids found in B that are NOT present in A
	bool FindBNotInA( GoidColl & a, GoidColl & b, GoidColl & result );

	// result will contain goids found in B that ARE present in A
	bool FindBInA( GoidColl & a, GoidColl & b, GoidColl & result );

FEX float GetAlterationSum( Go const * client, eAlteration Alteration ) const;

FEX GopColl & GetTempGopColl1()				{ return m_TempGopColl1 ; }
FEX GopColl & GetTempGopColl2()				{ return m_TempGopColl2 ; }
FEX GopColl & GetTempGopColl3()				{ return m_TempGopColl3 ; }
FEX QtColl & GetTempQtColl1()				{ return m_TempQtColl1; }
FEX SiegePos & GetTempPos1()				{ return m_TempPos1; }
FEX vector_3 & GetTempVector1()				{ return m_TempVector1; }

	// Debug

#	if !GP_RETAIL
FEX void DumpActorStats( Go const * go, ReportSys::Context * ctx );
#	endif // !GP_RETAIL

private:

// Types.

	struct Query
	{
		vector_3        m_NodePos;				// node-space center of query
		float           m_Radius;				// radius of query
		eOccupantFilter m_Filter;				// filter query was made with
		double          m_ExpireTime;			// when in absolute time this query expires

		bool IsMatch( const vector_3& nodePos, float radius, eOccupantFilter filter ) const
			{  return ( (m_NodePos == nodePos) && ::IsEqual( m_Radius, radius ) && (m_Filter == filter) );  }
	};

	typedef stdx::fast_vector <Query> QueryColl;

	struct Occupant
	{
		Go*              m_Go;					// this is the occupant
		mutable float    m_LastDistance2;		// (cache) last time we calc'd distance, this is what we got (this is dist squared btw)
		mutable SiegePos m_LastPos;				// (cache) last time we calc'd distance, this was the go's position

		Occupant( void )  {  }
		Occupant( Go* go );
		bool HasMoved( bool& isValid ) const;
		bool Resolve( const vector_3& center, bool force = false ) const;

		bool operator == ( const Go* go ) const
			{  return ( m_Go == go );  }
		bool operator < ( const Occupant& other ) const
			{  return ( m_LastDistance2 < other.m_LastDistance2 );  }
	};

	struct OccupantLess
	{
		bool operator () ( const Occupant& l, const Occupant& r ) const
		{
			return ( l.m_LastDistance2 < r.m_LastDistance2 );
		}
	};

	typedef std::multiset <Occupant, OccupantLess> OccupantColl;
	typedef stdx::fast_vector <SiegeGuid> SiegeGuidColl;

	struct MetaQuery
	{
		// spec
		SiegeGuid        m_Node;				// node guid that this metaquery is for
		QueryColl        m_Queries;				// queries active in this metaquery
		vector_3         m_NodePos;				// node-space center of metaquery, generally the first query sets this
		float            m_Radius;				// overall radius of metaquery - the max radius of all queries
		eOccupantFilter  m_MasterFilter;		// this is the or'd equivalent of all subordinate queries - note that it's kept SIMPLE, things that cannot change state only like actors and items (we just want to build a selection set which is filtered later)

		// state
		OccupantColl     m_Occupants;			// filtered go's occupying nodes this metaquery is interested in, sorted by distance from the center
		SiegeGuidColl    m_InterestNodes;		// nodes we're interested in
		double           m_SoonestExpireTime;	// time of the soonest expiration for any of the queries

		// cache
		mutable vector_3 m_WorldPos;			// cached worldpos center of metaquery - updated when space changes for a node
		mutable bool     m_WorldPosDirty;		// true if worldpos is dirty and needs recalc'ing
		unsigned int     m_LastQuerySim;		// last sim id of when this query was updated (for sorting occupants)

		// helper type
		struct Walker
		{
			const vector_3& m_Center;			// worldspace position of the center
			float           m_Radius;			// radius for node intersection checks
			unsigned int    m_VisitCount;		// visit counter to keep track of who we've seen and who we haven't
			SiegeGuidColl&  m_Output;			// output goes here

			// ctor
			Walker( MetaQuery* metaQuery, SiegeGuidColl& out );

			// helper methods
			void Walk( SiegeNode* node );
		};

		// ctor
		MetaQuery( void );
#		if AIQUERY_UBER_PARANOIA
	   ~MetaQuery( void )			{  UnregisterAllOccupants();  }
		void UnregisterAllOccupants( void );
#		endif // AIQUERY_UBER_PARANOIA

		// helper methods
		void      AddOccupant       ( Go* go );
		void      AddOccupantIfNew  ( Go* go );
		void      RemoveOccupant    ( Go* go );
		vector_3& GetWorldPos       ( void ) const;
		void      Glom              ( Query* query );
		void      UnGlom            ( Query* query );
		int       Query             ( Query* query, const Go* sourceObject, const GoidColl* ignoreColl, int earlyOutSize, GopColl* out );
		void      Rewalk            ( bool queryOccupants = true );
		void      Requery           ( void );
		void      AddInterestNode   ( SiegeGuid guid, bool addOccupants = true );
		void      RemoveInterestNode( SiegeGuid guid );
		void      AddOccupants      ( SiegeGuid guid );
		void      RecalcTimeout     ( void );

		// helper inlines
		bool CanGlom( const vector_3& pos, float distance2 ) const
			{  return ( m_NodePos.Distance2( pos ) <= distance2 );  }
	};

	friend MetaQuery;

	struct InterestNode
	{
		SiegeGuid  m_InterestNode;				// id of node that is interesting
		MetaQuery* m_Query;						// metaquery that is interested in it

		InterestNode( void )
		{
			m_Query = NULL;
		}

		InterestNode( SiegeGuid guid, MetaQuery* query )
			: m_InterestNode( guid )
		{
			m_Query = query;
		}
	};

	struct InterestNodeLess
	{
		bool operator () ( const InterestNode& l, const InterestNode& r ) const
		{
			if ( l.m_InterestNode < r.m_InterestNode )
			{
				return ( true );
			}
			if ( r.m_InterestNode < l.m_InterestNode )
			{
				return ( false );
			}
			if ( l.m_Query == NULL )
			{
				return ( true );
			}
			if ( r.m_Query == NULL )
			{
				return ( false );
			}
			if ( l.m_Query->m_Node < r.m_Query->m_Node )
			{
				return ( true );
			}
			if ( r.m_Query->m_Node < l.m_Query->m_Node )
			{
				return ( false );
			}
			return ( l.m_Query < r.m_Query );
		}
	};

	typedef std::multimap <SiegeGuid, MetaQuery> MetaQueryDb;
	typedef std::set <InterestNode, InterestNodeLess> InterestNodeDb;

	MetaQueryDb::iterator EraseMetaQuery( MetaQueryDb::iterator where );

// Data.

	MetaQueryDb    m_MetaQueryDb;			// metaqueries active, sorted by guid
	InterestNodeDb m_InterestNodeDb;		// nodes that are interesting to the metaqueries that have them in walk range
	bool           m_UseQueryCache;			// is the query cached enabled?
	double         m_LastTime;				// last time we got updated

	GopColl		m_TempGopColl1;
	GopColl		m_TempGopColl2;
	GopColl		m_TempGopColl3;
	QtColl		m_TempQtColl1;
	SiegePos	m_TempPos1;
	vector_3	m_TempVector1;

#	if AIQUERY_UBER_PARANOIA

	typedef std::multimap <Go*, MetaQuery*> GoMetaQueryDb;

	GoMetaQueryDb::iterator FindGoInCache( Go* go, MetaQuery* metaQuery );
	bool HasGoInCache( Go* go, MetaQuery* metaQuery );

	GoMetaQueryDb m_GoMetaQueryDb;

#	endif // AIQUERY_UBER_PARANOIA

	FUBI_SINGLETON_CLASS( AIQuery, "AI query services." );
	SET_NO_COPYING( AIQuery );
};


#define gAIQuery AIQuery::GetSingleton()


#endif
