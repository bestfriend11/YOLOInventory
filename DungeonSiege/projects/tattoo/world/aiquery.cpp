//////////////////////////////////////////////////////////////////////////////

#include "precomp_world.h"

#include "aiquery.h"
#include "enchantment.h"
#include "formulahelper.h"
#include "go.h"
#include "Go.h"
#include "GoActor.h"
#include "GoAttack.h"
#include "GoBody.h"
#include "GoCore.h"
#include "GoDb.h"
#include "GoInventory.h"
#include "GoMagic.h"
#include "GoMind.h"
#include "GoAspect.h"
#include "Job.h"
#include "player.h"
#include "ReportSys.h"
#include "WorldMap.h"
#include "WorldTime.h"
#include "WorldOptions.h"
#include "WorldTerrain.h"
#include "server.h"
#include "siege_engine.h"
#include "siege_node.h"


using namespace siege;


// this is the constant used to determine whether or not sphere occupancy
// queries are close enough to be glommed together.
static const float SPHERE_QUERY_CLOSE_ENOUGH_TO_GLOM_RATIO = 0.05f;		// 5%
static const float SPHERE_QUERY_EXPIRATION_TIME = 1.0f;					// 1 seconds

#if AIQUERY_UBER_PARANOIA
void gpbitchslap_aiq( const char* msg, const Go* go )
{
	gperrorf(( "TELL SCOTT: AIQuery paranoia check failure!!!\n"
			   "\n"
			   "Message: %s\n"
			   "Go     : %s\n"
			   "Goid   : 0x%08X\n"
			   "Scid   : 0x%08X\n"
			   "Node   : %s\n",
			   msg,
			   go->GetTemplateName(),
			   go->GetGoid(),
			   go->GetScid(),
			   go->HasPlacement() ? go->GetPlacement()->GetPosition().node.ToString().c_str() : "<NULL!!>" ));
}
#else // AIQUERY_UBER_PARANOIA
#define gpbitchslap_aiq( x, y )
#endif // AIQUERY_UBER_PARANOIA

//////////////////////////////////////////////////////////////////////////////
// helper methods

inline bool AIQueryInterestFilter( const Go* go )
{
	gpassert( go != NULL );

	// gotta have placement - shouldn't get here without it, but check anyway
	if ( !go->HasPlacement() )
	{
		gpbitchslap_aiq( "Somehow we're filtering a Go without placement!", go );
		return ( false );
	}

	// no parties allowed
	if ( go->HasParty() )
	{
		return ( false );
	}

	// not global, ai can't see it
	if ( !go->IsGlobalGo() )
	{
		return ( false );
	}

	// in inventory, basically invisible too
	if ( go->IsInsideInventory() )
	{
		return ( false );
	}

	// filter out ammo, no reason to include it in ai queries
	if ( go->IsAmmo() )
	{
		return ( false );
	}

	// must be ok then
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// struct QTBundle declaration and implementation

struct QtBundle
{
	float NormQtVal;
	Go * pGO;
};

typedef stdx::fast_vector< QtBundle > QtBundleColl;

inline bool operator < ( const QtBundle& l, const QtBundle& r )  {  return( l.NormQtVal < r.NormQtVal );  }

//////////////////////////////////////////////////////////////////////////////
// struct AIQuery::Occupant implementation

AIQuery::Occupant :: Occupant( Go* go )
{
	gpassert( go != NULL );
	gpassert( IsValid( go->GetGoid() ) );
	m_Go = go;
	m_LastDistance2 = FLOAT_MAX;
}

bool AIQuery::Occupant :: HasMoved( bool& isValid ) const
{
	// special: somehow this Go may not have a placement. i have no idea why
	// this can happen but check for it anyway and bail out.
	isValid = m_Go->HasPlacement();
	if ( !isValid )
	{
		gpbitchslap_aiq( "Somehow we have a Go without a placement in the cache!", m_Go );
		return ( false );
	}

	// $ optz: use a memcmp because it's much faster (the default != op does a
	//         slow IsEqual() test which is not needed here).

	return ( ::memcmp( &m_LastPos, &m_Go->GetPlacement()->GetPosition(), sizeof( m_LastPos ) ) != 0 );
}

bool AIQuery::Occupant :: Resolve( const vector_3& center, bool force ) const
{
	bool isValid = true;
	if ( force || HasMoved( isValid ) )
	{
		const GoPlacement* place = m_Go->GetPlacement();
		const SiegePos& pos = place->GetPosition();

		m_LastDistance2 = center.Distance2( place->GetWorldPosition() );
		m_LastPos = pos;
	}
	return ( isValid );
}

//////////////////////////////////////////////////////////////////////////////
// struct AIQuery::MetaQuery::Walker implementation

AIQuery::MetaQuery::Walker :: Walker( MetaQuery* metaQuery, SiegeGuidColl& out )
	: m_Center( metaQuery->GetWorldPos() ), m_Output( out )
{
	m_Radius = metaQuery->m_Radius;
	m_VisitCount = RandomDword();
}

void AIQuery::MetaQuery::Walker :: Walk( SiegeNode* node )
{
	// $ early abort if no node
	if ( node == NULL )
	{
		return;
	}

	// set it as visited so no one else does work on it
	node->SetAPVisited( m_VisitCount );

	// add to list
	m_Output.push_back( node->GetGUID() );

	// go through neighbors
	SiegeNodeDoorConstIter i, ibegin = node->GetDoors().begin(), iend = node->GetDoors().end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// get a valid neighbor
		SiegeGuid nextGuid = (*i)->GetNeighbour();
		SiegeNode* next = gSiegeEngine.IsNodeValid( nextGuid );
		if ( (next == NULL) || !next->IsOwnedByAnyFrustum() )
		{
			// we're on the edge of the world - only insert once. we need to
			// track this node so if it enters the world we can re-walk, but
			// it has no other purpose.
			if ( !stdx::has_any( m_Output, nextGuid ) )
			{
				m_Output.push_back( nextGuid );
			}
			continue;
		}

		// only process if we haven't touched this one
		if ( next->GetAPVisited() != m_VisitCount )
		{
			// see if this node is in our requested sphere and recurse
			if ( ::SphereIntersectsBox( m_Center, m_Radius, next->GetWorldSpaceMinimumBounds(), next->GetWorldSpaceMaximumBounds() ) )
			{
				Walk( next );
			}
			else
			{
				// set it as visited so no one else does work on it
				next->SetAPVisited( m_VisitCount );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// struct AIQuery::MetaQuery implementation

AIQuery::MetaQuery :: MetaQuery( void )
{
	m_Radius            = 0;
	m_MasterFilter      = OF_MATCH_ANY;
	m_SoonestExpireTime = 0;
	m_WorldPosDirty     = true;
	m_LastQuerySim      = 0;
}

#if AIQUERY_UBER_PARANOIA

void AIQuery::MetaQuery :: UnregisterAllOccupants( void )
{
	OccupantColl::const_iterator i, ibegin = m_Occupants.begin(), iend = m_Occupants.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		GoMetaQueryDb::iterator found = gAIQuery.FindGoInCache( i->m_Go, this );
		if ( found != gAIQuery.m_GoMetaQueryDb.end() )
		{
			gAIQuery.m_GoMetaQueryDb.erase( found );
		}
		else
		{
			gpbitchslap_aiq( "Occupant exists in meta-query but not in paranoia cache!", i->m_Go );
		}
	}
}

#endif // AIQUERY_UBER_PARANOIA

void AIQuery::MetaQuery :: AddOccupant( Go* go )
{
	gpassert( go != NULL );
	gpassert( !stdx::has_any( m_Occupants, go ) );

	// $ must do it this way instead of just asking if go is in any world
	//   frustum because we may not have assigned go's bitfield yet, but have
	//   assigned node's bitfield (rare transition case).
	gpassert( go->GetPlacement()->GetIsNodeInAnyWorldFrustum() );

#	if AIQUERY_UBER_PARANOIA
	std::pair <GoMetaQueryDb::const_iterator, GoMetaQueryDb::const_iterator>
			rc = gAIQuery.m_GoMetaQueryDb.equal_range( go );
	for ( ; rc.first != rc.second ; ++rc.first )
	{
		if ( rc.first->second == this )
		{
			gpbitchslap_aiq( "Occupant being added to an existing meta-query", go );
		}
	}
#	endif // AIQUERY_UBER_PARANOIA

	if ( gAIQuery.Filter( go, NULL, m_MasterFilter, true ) )
	{
		Occupant occupant( go );
		if ( occupant.Resolve( GetWorldPos() ) )
		{
			m_Occupants.insert( occupant );

#			if AIQUERY_UBER_PARANOIA
			gAIQuery.m_GoMetaQueryDb.insert( std::make_pair( go, this ) );
#			endif // AIQUERY_UBER_PARANOIA

#			if !GP_RETAIL
			if ( gWorldOptions.GetAiQueryBoxes() )
			{
				gWorld.DrawDebugLine( go->GetPlacement()->GetPosition(), SiegePos( m_NodePos, m_Node ), COLOR_WHITE, gpstring::EMPTY );
				gWorld.DrawDebugCircle( go->GetPlacement()->GetPosition(), 0.5f, COLOR_WHITE, gpstring::EMPTY );
			}
#			endif // !GP_RETAIL
		}
	}

	// dirty the query
	m_LastQuerySim = 0;
}


void AIQuery::MetaQuery :: AddOccupantIfNew( Go* go )
{
	if ( !stdx::has_any( m_Occupants, go ) )
	{
		AddOccupant( go );
	}
#	if AIQUERY_UBER_PARANOIA
	else if ( !gAIQuery.HasGoInCache( go, this ) )
	{
		gpbitchslap_aiq( "Occupant registered in meta-query but not in paranoia cache!", go );
	}
#	endif // AIQUERY_UBER_PARANOIA
}


void AIQuery::MetaQuery :: RemoveOccupant( Go* go )
{
	gpassert( go != NULL );

	OccupantColl::iterator found = stdx::find( m_Occupants, go );
	if ( found != m_Occupants.end() )
	{
		m_Occupants.erase( found );
		gpassert( !stdx::has_any( m_Occupants, go ) );

#		if AIQUERY_UBER_PARANOIA
		GoMetaQueryDb::iterator found = gAIQuery.FindGoInCache( go, this );
		if ( found != gAIQuery.m_GoMetaQueryDb.end() )
		{
			gAIQuery.m_GoMetaQueryDb.erase( found );
		}
		else
		{
			gpbitchslap_aiq( "Occupant being removed from meta-query but not registered in paranoia cache!", go );
		}
#		endif // AIQUERY_UBER_PARANOIA

#		if !GP_RETAIL
		if ( gWorldOptions.GetAiQueryBoxes() )
		{
			gWorld.DrawDebugLine( go->GetPlacement()->GetPosition(), SiegePos( m_NodePos, m_Node ), COLOR_DARK_GREY, gpstring::EMPTY );
			gWorld.DrawDebugCircle( go->GetPlacement()->GetPosition(), 0.5f, COLOR_DARK_GREY, gpstring::EMPTY );
		}
#		endif // !GP_RETAIL
	}
#	if AIQUERY_UBER_PARANOIA
	else if ( gAIQuery.HasGoInCache( go, this ) )
	{
		gpbitchslap_aiq( "Occupant does not exist in meta-query but is registered in paranoia cache!", go );
	}
#	endif // AIQUERY_UBER_PARANOIA
}


vector_3& AIQuery::MetaQuery :: GetWorldPos( void ) const
{
	if ( m_WorldPosDirty )
	{
		m_WorldPosDirty = false;
		m_WorldPos = SiegePos( m_NodePos, m_Node ).WorldPos();
	}
	return ( m_WorldPos );
}


void AIQuery::MetaQuery :: Glom( AIQuery::Query* query )
{
	gpassert( query != NULL );

	// filter changed?
	bool requery = false;
	if ( (query->m_Filter & OF_ACTORS) && !(m_MasterFilter & OF_ACTORS) )
	{
		m_MasterFilter |= OF_ACTORS;
		requery = true;
	}
	if ( (query->m_Filter & OF_ITEMS) && !(m_MasterFilter & OF_ITEMS) )
	{
		m_MasterFilter |= OF_ITEMS;
		requery = true;
	}

	// do rewalk
	if ( m_Radius < query->m_Radius )
	{
		m_Radius = query->m_Radius;
		Rewalk( !requery );
	}

	// do requery
	if ( requery )
	{
		Requery();
	}
}


void AIQuery::MetaQuery :: UnGlom( AIQuery::Query* query )
{
	gpassert( query != NULL );

	// if that was the last query, then we're going to be deleted anyway, no sense unglomming
	if ( m_Queries.size() == 1 )
	{
		return;
	}

	// get some detect vars
	float newRadius = 0;
	eOccupantFilter newFilter = OF_MATCH_ANY;

	// check queries
	QueryColl::const_iterator i, ibegin = m_Queries.begin(), iend = m_Queries.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( &*i != query )
		{
			// check new radius
			maximize( newRadius, i->m_Radius );

			// check flags
			if ( i->m_Filter & OF_ACTORS )
			{
				newFilter |= OF_ACTORS;
			}
			if ( i->m_Filter & OF_ITEMS )
			{
				newFilter |= OF_ITEMS;
			}
		}
	}

	// check for new filter
	bool requery = newFilter != m_MasterFilter;

	// do rewalk
	if ( m_Radius > newRadius )
	{
		m_Radius = newRadius;
		Rewalk( !requery );
	}

	// do requery
	if ( requery )
	{
		Requery();
	}
}


int AIQuery::MetaQuery :: Query(
		AIQuery::Query* query, const Go* sourceObject, const GoidColl* ignoreColl,
		int earlyOutSize, GopColl* out )
{
	// check for query dirty - we want to dirty every frame because go's may
	// move within nodes, but for multiple queries within a frame, we can cache
	// the results of the last query
	unsigned int simCount = gWorldTime.GetSimCount();
	if ( m_LastQuerySim != simCount )
	{
		const vector_3& worldPos = GetWorldPos();

		// the changed-coll should be done locally, but don't chance overflowing
		// the size. base this on max possible size, assuming all occupants have
		// changed position.
		size_t allocSize = m_Occupants.size() * sizeof( OccupantColl::iterator );
		if ( allocSize >= 4096 )
		{
			// $ IMPORTANT: if you change this code, update the 'else' below too!

			typedef stdx::fast_vector <OccupantColl::iterator> IterColl;
			IterColl changed;

			// verify and resolve them in case they moved
			OccupantColl::iterator i, ibegin = m_Occupants.begin(), iend = m_Occupants.end();
			for ( i = ibegin ; i != iend ; )
			{
				gpassert( IsValid( i->m_Go->GetGoid() ) );
				bool isValid;
				if ( i->HasMoved( isValid ) )
				{
					changed.push_back( i );
					++i;
				}
				else if ( !isValid )
				{
					// somehow went invalid, just kill it
					i = m_Occupants.erase( i );
				}
				else
				{
					++i;
				}
			}

			// remove and re-insert changed occupants based on distance from center
			IterColl::iterator j, jbegin = changed.begin(), jend = changed.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				Occupant occupant( **j );
				gpverify( occupant.Resolve( worldPos, true ) );
				m_Occupants.erase( *j );
				m_Occupants.insert( occupant );
			}
		}
		else
		{
			// $ IMPORTANT: if you change this code, update the 'if' above too!

			// allocate enough space to hold 'em all if necessary
			OccupantColl::iterator* changed = (OccupantColl::iterator*)::_alloca( allocSize );
			OccupantColl::iterator* changedEnd = changed;

			// verify and resolve them in case they moved
			OccupantColl::iterator i, ibegin = m_Occupants.begin(), iend = m_Occupants.end();
			for ( i = ibegin ; i != iend ; )
			{
				gpassert( IsValid( i->m_Go->GetGoid() ) );
				bool isValid;
				if ( i->HasMoved( isValid ) )
				{
					*changedEnd++ = i;
					++i;
				}
				else if ( !isValid )
				{
					// somehow went invalid, just kill it
					i = m_Occupants.erase( i );
				}
				else
				{
					++i;
				}
			}

			// remove and re-insert changed occupants based on distance from center
			OccupantColl::iterator* j, * jbegin = changed, * jend = changedEnd;
			for ( j = jbegin ; j != jend ; ++j )
			{
				// recalc dist
				gpverify( (*j)->Resolve( worldPos, true ) );

				// does it need to move?
				bool moveIt = false;
				if ( *j != m_Occupants.begin() )
				{
					OccupantColl::iterator prev = *j;
					--prev;
					if ( **j < *prev )
					{
						moveIt = true;
					}
				}
				if ( !moveIt && (*j != m_Occupants.end()) )
				{
					OccupantColl::iterator next = *j;
					++next;
					if ( !(**j < *next) )
					{
						moveIt = true;
					}
				}

				// move if necessary
				if ( moveIt )
				{
					Occupant occupant( **j );
					m_Occupants.erase( *j );
					m_Occupants.insert( occupant );
				}
			}
		}

		// update to current
		m_LastQuerySim = simCount;
	}

	// get some vars
	int found = 0;
	float r2 = Square( query->m_Radius );

	// ok now go out from the center, until we hit the early out or radius
	OccupantColl::const_iterator i, ibegin = m_Occupants.begin(), iend = m_Occupants.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Go* go = i->m_Go;
		float localr2 = go->HasMind() ? Square( query->m_Radius + go->GetMind()->GetPersonalSpaceRange() ) : r2;

		// abort if we've gone too far
		if ( localr2 < i->m_LastDistance2 )
		{
			// everything is sorted by distance, so all remaining entries are
			// also too far, and we can abort here
			break;
		}

		// check against source which we always ignore
		if ( sourceObject == go )
		{
			continue;
		}

		// check against the optional ignore coll
		if ( (ignoreColl != NULL) && stdx::has_any( *ignoreColl, go->GetGoid() ) )
		{
			continue;
		}

		// check filter
		if ( (query->m_Filter != m_MasterFilter) && !gAIQuery.Filter( go, sourceObject, query->m_Filter ) )
		{
			continue;
		}

		// found one
		if ( out != NULL )
		{
			out->push_back( go );
		}

		// early out says we should abort?
		if ( (++found >= earlyOutSize) && (earlyOutSize > 0) )
		{
			break;
		}
	}

	// done
	return ( found );
}


void AIQuery::MetaQuery :: Rewalk( bool queryOccupants )
{
	GPPROFILERSAMPLE( "AIQuery::MetaQuery :: Rewalk", SP_AI | SP_AI_QUERY );

	// get root node - may be null if we're out of the world
	SiegeNode* rootNode = gSiegeEngine.IsNodeValid( m_Node );
	if ( rootNode == NULL )
	{
		return;
	}

	// get new nodes
	SiegeGuidColl newColl;
	Walker( this, newColl ).Walk( rootNode );
	stdx::sort( newColl );

	// add any new ones that showed up
	SiegeGuidColl::const_iterator ni = newColl.begin(), ne = newColl.end();
	SiegeGuidColl::const_iterator oi = m_InterestNodes.begin(), oe = m_InterestNodes.end();
	for ( ; ; )
	{
		// compare the query bases
		int compare;
		if ( oi == oe )
		{
			if ( ni == ne )
			{
				break;
			}
			compare = 1;
		}
		else if ( ni == ne )
		{
			compare = -1;
		}
		else if ( *oi == *ni )
		{
			compare = 0;
		}
		else
		{
			compare = (*oi < *ni) ? -1 : 1;
		}

		// compare = 0: exists in both queries, no change to membership
		// compare < 0: exists in old but not in new
		// compare > 0: exists in new but not in old

		if ( compare == 0 )
		{
			// no change, just advance
			++oi;
			++ni;
		}
		else if ( compare < 0 )
		{
			// remove from old
			RemoveInterestNode( *oi );

			// advance
			++oi;
		}
		else
		{
			// add to new
			AddInterestNode( *ni, queryOccupants );

			// advance
			++ni;
		}
	}

	// take new stuff
	stdx::swap( newColl, m_InterestNodes );

#	if !GP_RETAIL
	if ( gWorldOptions.GetAiQueryBoxes() )
	{
		SiegePos pos( m_NodePos, m_Node );
		gWorld.DrawDebugPoint( pos, 0.5, COLOR_ORANGE, "Rewalked!" );
	}
#	endif // !GP_RETAIL
}


void AIQuery::MetaQuery :: Requery( void )
{
	GPPROFILERSAMPLE( "AIQuery::MetaQuery :: Requery", SP_AI | SP_AI_QUERY );

	// unregister occupants
#	if AIQUERY_UBER_PARANOIA
	UnregisterAllOccupants();
#	endif // AIQUERY_UBER_PARANOIA

	// nuke the existing occupants
	m_Occupants.clear();

	// requery for them
	SiegeGuidColl::const_iterator i, ibegin = m_InterestNodes.begin(), iend = m_InterestNodes.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		AddOccupants( *i );
	}

#	if !GP_RETAIL
	if ( gWorldOptions.GetAiQueryBoxes() )
	{
		SiegePos pos( m_NodePos, m_Node );
		pos.pos.y += 0.2f;
		gWorld.DrawDebugPoint( pos, 0.5, COLOR_ORANGE, "Requeried!" );
	}
#	endif // !GP_RETAIL
}


void AIQuery::MetaQuery :: AddInterestNode( SiegeGuid guid, bool addOccupants )
{
	// $ note: this does not mess with the local interest node coll

	// insert and make sure not already there
	gpverify( gAIQuery.m_InterestNodeDb.insert( InterestNode( guid, this ) ).second );

	// optionally add occupants too
	if ( addOccupants )
	{
		AddOccupants( guid );
	}
}

void AIQuery::MetaQuery :: RemoveInterestNode( SiegeGuid guid )
{
	// $ note: this does not mess with the local interest node coll, but it
	//   does remove affected occupants.

	// remove from global registry
	InterestNodeDb::iterator found = gAIQuery.m_InterestNodeDb.find( InterestNode( guid, this ) );
	gpassert( found != gAIQuery.m_InterestNodeDb.end() );
	gAIQuery.m_InterestNodeDb.erase( found );

	// now remove any occupants in that node
	GoDb::SiegeGoDb::iterator i, ibegin, iend;
	if ( gGoDb.GetNodeOccupantsIters( guid, ibegin, iend ) )
	{
		for ( i = ibegin ; i != iend ; ++i )
		{
			// use the filter to avoid the lsearch
			if ( AIQueryInterestFilter( i->second ) )
			{
				RemoveOccupant( i->second );
			}
		}
	}
}

void AIQuery::MetaQuery :: AddOccupants( SiegeGuid guid )
{
	// look up occupants if we have a valid frustum
	if ( gSiegeEngine.IsNodeInAnyFrustum( guid ) )
	{
		GoDb::SiegeGoDb::iterator i, ibegin, iend;
		if ( gGoDb.GetNodeOccupantsIters( guid, ibegin, iend ) )
		{
			for ( i = ibegin ; i != iend ; ++i )
			{
				Go* go = i->second;

				// only bother if in world (out of world can happen on rare
				// setup occasions, but we'll be picking it up in just a sec)
				if ( go->IsInAnyWorldFrustum() )
				{
					// only add what we're interested in
					if ( AIQueryInterestFilter( go ) )
					{
						AddOccupant( go );
					}
				}
			}
		}
	}
}


void AIQuery::MetaQuery :: RecalcTimeout( void )
{
	m_SoonestExpireTime = PreciseAdd( gWorldTime.GetTime(), SPHERE_QUERY_EXPIRATION_TIME );

	for ( QueryColl::iterator i = m_Queries.begin() ; i != m_Queries.end() ; ++i )
	{
		minimize( m_SoonestExpireTime, i->m_ExpireTime );
	}
}


//////////////////////////////////////////////////////////////////////////////
// class AIQuery implementation


AIQuery :: AIQuery( void )
{
	m_UseQueryCache = true;
	m_LastTime = 0;
}


AIQuery :: ~AIQuery( void )
{
	Shutdown();
}


void AIQuery :: Shutdown( void )
{
	CHECK_PRIMARY_THREAD_ONLY;

	m_MetaQueryDb.clear();
	m_InterestNodeDb.clear();
	m_LastTime = 0;

#	if AIQUERY_UBER_PARANOIA
	m_GoMetaQueryDb.clear();
#	endif // AIQUERY_UBER_PARANOIA
}


void AIQuery :: Update( float /*secondsElapsed*/ )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: Update", SP_AI | SP_AI_QUERY );
	CHECK_PRIMARY_THREAD_ONLY;

	double time = gWorldTime.GetTime();
	if ( fabs( PreciseSubtract(time, m_LastTime) ) > 5.0f )
	{
		// ok there has been a mass delta of some kind, probably due to a server
		// force-updating time. if it goes backwards we may end up with queries
		// that never expire! so with large deltas just flush the cache.
		Shutdown();
	}
	m_LastTime = time;

	// check for expirations
	MetaQueryDb::iterator i = m_MetaQueryDb.begin();
	while ( i != m_MetaQueryDb.end() )
	{
		MetaQuery* metaQuery = &i->second;

#		if GP_DEBUG

		// check for leftovers in the cache
		{
			OccupantColl::const_iterator j, jbegin = metaQuery->m_Occupants.begin(), jend = metaQuery->m_Occupants.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				SiegeGuid node = j->m_Go->GetPlacement()->GetPosition().node;
				gpassertm( stdx::has_any( metaQuery->m_InterestNodes, node ), "TELL SCOTT: A Go in the AIQ cache is filed under the wrong node!" );
			}
		}

		// check for strange timing bugs
		{
			gpassert( (PreciseSubtract(metaQuery->m_SoonestExpireTime, time)) < 30.0f );

			QueryColl::iterator j, jbegin = metaQuery->m_Queries.begin(), jend = metaQuery->m_Queries.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				gpassert( (PreciseSubtract(j->m_ExpireTime, time)) < 30.0f );
			}
		}

#		endif // GP_DEBUG

		// any expirations?
		bool removed = false;
		if ( metaQuery->m_SoonestExpireTime <= time )
		{
			// find which one(s)
			QueryColl::iterator j = metaQuery->m_Queries.begin();
			while ( j != metaQuery->m_Queries.end() )
			{
				Query* query = &*j;

				// expiring?
				if ( query->m_ExpireTime <= time )
				{
					// de-glom and remove!
					metaQuery->UnGlom( query );
					j = metaQuery->m_Queries.erase( j );
					removed = true;
				}
				else
				{
					++j;
				}
			}
		}

		// any remaining? if not, kill the metaquery
		if ( metaQuery->m_Queries.empty() )
		{
			// nuke it
			i = EraseMetaQuery( i );
		}
		else
		{
#			if !GP_RETAIL
			if ( gWorldOptions.GetAiQueryBoxes() )
			{
				// $$$ render each aiq according to stats - color it different if not used and have expiry on text
				// $$$ render little x on each go that was "found" in the query

			// First render the cached boxes.

				SiegeGuidColl::const_iterator j, jbegin = metaQuery->m_InterestNodes.begin(), jend = metaQuery->m_InterestNodes.end();
				for ( j = jbegin ; j != jend ; ++j )
				{
					SiegeNode* node = gSiegeEngine.IsNodeValid( *j );
					if ( node == NULL )
					{
						continue;
					}

					DWORD color = MAKEDWORDCOLOR( vector_3( 0.5f, 0, 0 ) );

					const vector_3& b0 = node->GetWorldSpaceMinimumBounds();
					const vector_3& b1 = node->GetWorldSpaceMaximumBounds();

					vector_3 p0( b1.x - 0.1f, b1.y - 0.1f, b0.z + 0.1f );
					vector_3 p1( b0.x + 0.1f, b1.y - 0.1f, b0.z + 0.1f );
					vector_3 p2( b0.x + 0.1f, b0.y + 0.1f, b0.z + 0.1f );
					vector_3 p3( b1.x - 0.1f, b0.y + 0.1f, b0.z + 0.1f );
					vector_3 p4( b1.x - 0.1f, b1.y - 0.1f, b1.z - 0.1f );
					vector_3 p5( b0.x + 0.1f, b1.y - 0.1f, b1.z - 0.1f );
					vector_3 p6( b0.x + 0.1f, b0.y + 0.1f, b1.z - 0.1f );
					vector_3 p7( b1.x - 0.1f, b0.y + 0.1f, b1.z - 0.1f );

					gSiegeEngine.GetWorldSpaceLines().DrawLine( p0, p1, color );
					gSiegeEngine.GetWorldSpaceLines().DrawLine( p2, p1, color );
					gSiegeEngine.GetWorldSpaceLines().DrawLine( p2, p3, color );
					gSiegeEngine.GetWorldSpaceLines().DrawLine( p0, p3, color );
					gSiegeEngine.GetWorldSpaceLines().DrawLine( p4, p5, color );
					gSiegeEngine.GetWorldSpaceLines().DrawLine( p6, p5, color );
					gSiegeEngine.GetWorldSpaceLines().DrawLine( p6, p7, color );
					gSiegeEngine.GetWorldSpaceLines().DrawLine( p4, p7, color );
					gSiegeEngine.GetWorldSpaceLines().DrawLine( p0, p4, color );
					gSiegeEngine.GetWorldSpaceLines().DrawLine( p1, p5, color );
					gSiegeEngine.GetWorldSpaceLines().DrawLine( p2, p6, color );
					gSiegeEngine.GetWorldSpaceLines().DrawLine( p3, p7, color );
				}

			// Next render query info.

				gWorld.DrawDebugPoint( SiegePos( metaQuery->m_NodePos, metaQuery->m_Node ), 0.3f, COLOR_GREEN,
						gpstringf( "Queries: %d, Filter: 0x%08X\n"
											"Occupants: %d, Interest Nodes: %d\n"
											"Expire in %.2f",
											metaQuery->m_Queries.size(), metaQuery->m_MasterFilter,
											metaQuery->m_Occupants.size(), metaQuery->m_InterestNodes.size(),
											PreciseSubtract(metaQuery->m_SoonestExpireTime, gWorldTime.GetTime()) ) );

				QueryColl::iterator k, kbegin = metaQuery->m_Queries.begin(), kend = metaQuery->m_Queries.end();
				for ( k = kbegin ; k != kend ; ++k )
				{
					gWorld.DrawDebugTriangle( SiegePos( k->m_NodePos, metaQuery->m_Node ), 0.1f, COLOR_ORANGE, gpstring::EMPTY );
				}
			}
#			endif // !GP_RETAIL

			// recalc expiry?
			if ( removed )
			{
				metaQuery->RecalcTimeout();
			}

			// advance
			++i;
		}
	}

	// reset global temps
	m_TempGopColl1.clear();
	m_TempGopColl2.clear();
	m_TempGopColl3.clear();
}


void AIQuery :: OnSpaceChanged( SiegeGuid guid )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	CHECK_PRIMARY_THREAD_ONLY;

	std::pair <MetaQueryDb::iterator, MetaQueryDb::iterator> rc = m_MetaQueryDb.equal_range( guid );
	for ( ; rc.first != rc.second ; ++rc.first )
	{
		rc.first->second.m_WorldPosDirty = true;
	}
}


void AIQuery :: OnNodeEnterWorld( const SiegeGuid& guid )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: OnNodeEnterWorld", SP_AI | SP_AI_QUERY );
	CHECK_PRIMARY_THREAD_ONLY;

	// find metaqueries
	InterestNodeDb::iterator i = m_InterestNodeDb.lower_bound( InterestNode( guid, 0 ) );
	if ( (i != m_InterestNodeDb.end()) && (i->m_InterestNode == guid) )
	{
		typedef stdx::fast_vector <MetaQuery*> MetaQueryColl;
		MetaQueryColl coll;
		coll.reserve( 50 );

		// copy local for processing - rewalking will cause interest coll to change!
		do
		{
			coll.push_back( i->m_Query );
			++i;
		}
		while ( (i != m_InterestNodeDb.end()) && (i->m_InterestNode == guid) );

		// process them
		MetaQueryColl::iterator i, ibegin = coll.begin(), iend = coll.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			(*i)->Rewalk();
		}
	}
}


void AIQuery :: OnNodeLeaveWorld( const SiegeGuid& guid )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: OnNodeEnterWorld", SP_AI | SP_AI_QUERY );
	CHECK_PRIMARY_THREAD_ONLY;

	// find metaqueries
	std::pair <MetaQueryDb::iterator, MetaQueryDb::iterator> rc
			= m_MetaQueryDb.equal_range( guid );

	// let's nuke them all. if their root node is outside the world then we
	// do not need them, plus it will cause some problems if in-world nodes are
	// updating out-of-world metaqueries.
	while ( rc.first != rc.second )
	{
		rc.first = EraseMetaQuery( rc.first );
	}
}


void AIQuery :: OnNodeTransition( const SiegeGuid& guid )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: OnNodeTransition", SP_AI | SP_AI_QUERY );
	CHECK_PRIMARY_THREAD_ONLY;

	// first dirty the elevator
	OnNodeInvalidate( guid );

	// get elevator node
	SiegeNode* node = gSiegeEngine.IsNodeValid( guid );
	if ( node != NULL )
	{
		// dirty all neighbors too
		SiegeNodeDoorConstIter i, ibegin = node->GetDoors().begin(), iend = node->GetDoors().end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			OnNodeInvalidate( (*i)->GetNeighbour() );
		}
	}
}


void AIQuery :: OnNodeInvalidate( const SiegeGuid& guid )
{
	typedef std::set <MetaQuery*> MetaQuerySet;
	MetaQuerySet metaQueries;

	// find any metaqueries that were interested in this node
	{
		InterestNodeDb::iterator i = m_InterestNodeDb.lower_bound( InterestNode( guid, 0 ) );
		for ( ; (i != m_InterestNodeDb.end()) && (i->m_InterestNode == guid) ; ++i )
		{
			metaQueries.insert( i->m_Query );
		}
	}

	// nuke 'em
	{
		MetaQuerySet::iterator i, ibegin = metaQueries.begin(), iend = metaQueries.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			MetaQueryDb::iterator j = m_MetaQueryDb.lower_bound( (*i)->m_Node );
			for ( ; j != m_MetaQueryDb.end() ; ++j )
			{
				if ( &j->second == *i )
				{
					EraseMetaQuery( j );
					break;
				}
			}
		}
	}
}


void AIQuery :: OnOccupantChanged( Go* occupant, SiegeGuid oldNode, SiegeGuid newNode, bool force )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	CHECK_PRIMARY_THREAD_ONLY;

	gpassert( occupant != NULL );
	gpassert( occupant->IsGlobalGo() );
	gpassert( force || (oldNode != newNode) );

	// $ early bailout if we aren't interested in including this in queries
	bool addToCache = AIQueryInterestFilter( occupant );
	if ( !addToCache && !force )
	{
		return;
	}

	// is this new node valid? if not, clear it so it doesn't go into the coll
	if ( (oldNode != newNode) && !gSiegeEngine.IsNodeInAnyFrustum( newNode ) )
	{
		newNode = siege::UNDEFINED_GUID;
	}

	// find metaqueries
	InterestNodeDb::iterator end = m_InterestNodeDb.end();
	InterestNodeDb::iterator oi  = m_InterestNodeDb.lower_bound( InterestNode( oldNode, 0 ) );
	InterestNodeDb::iterator ni  = m_InterestNodeDb.lower_bound( InterestNode( newNode, 0 ) );

	// update both simultaneously - they are sorted by query node, so skip over
	// any query nodes that they have in common, and only process those that
	// are different in either query group
	for ( ; ; )
	{
		// compare the query bases
		int compare;
		if ( (oi == end) || (oi->m_InterestNode != oldNode) )
		{
			if ( (ni == end) || (ni->m_InterestNode != newNode) )
			{
				break;
			}
			compare = 1;
		}
		else if ( (ni == end) || (ni->m_InterestNode != newNode) )
		{
			compare = -1;
		}
		else if ( oi->m_Query->m_Node == ni->m_Query->m_Node )
		{
			if ( oi->m_Query == ni->m_Query )
			{
				compare = 0;
			}
			else
			{
				compare = (oi->m_Query < ni->m_Query) ? -1 : 1;
			}
		}
		else
		{
			compare = (oi->m_Query->m_Node < ni->m_Query->m_Node) ? -1 : 1;
		}

		// compare = 0: exists in both queries, no change to membership
		// compare < 0: exists in old but not in new
		// compare > 0: exists in new but not in old

		if ( compare == 0 )
		{
			// forcing it?
			if ( force )
			{
				if ( addToCache )
				{
					// make sure it's in there
					ni->m_Query->AddOccupantIfNew( occupant );
				}
				else
				{
					// no node change, but we want it gone
					oi->m_Query->RemoveOccupant( occupant );
				}
			}

			// no change, just advance
			++oi;
			++ni;
		}
		else if ( compare < 0 )
		{
			// remove from old
			oi->m_Query->RemoveOccupant( occupant );

			// advance
			++oi;
		}
		else
		{
			// add to new
			if ( addToCache )
			{
				if ( force )
				{
					// forcing means ok new one
					ni->m_Query->AddOccupantIfNew( occupant );
				}
				else
				{
					// keep the assert if we're not forcing, maybe something odd going on
					ni->m_Query->AddOccupant( occupant );
				}
			}

			// advance
			++ni;
		}
	}
}


static int GetPartyMembersInNodes( Player* player, int region, int section, int level, int object, int earlyOutSize, GopColl* out )
{
	int found = 0;
	gpassert( player != NULL );

	Go* party = player->GetParty();
	if ( party != NULL )
	{
		GopColl::const_iterator i, ibegin = party->GetChildBegin(), iend = party->GetChildEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			SiegeNode* pNode = gSiegeEngine.IsNodeValid( (*i)->GetPlacement()->GetPosition().node );
			if ( pNode != NULL )
			{
				if (   (pNode->GetRegionID() == region )
					&& ((section == -1) || (pNode->GetSection() == section))
					&& ((level   == -1) || (pNode->GetLevel()   == level  ))
					&& ((object  == -1) || (pNode->GetObject()  == object )) )
				{
					if ( out != NULL )
					{
						out->push_back( *i );
					}
					if ( (++found >= earlyOutSize) && (earlyOutSize > 0) )
					{
						break;
					}
				}
			}
		}
	}

	return ( found );
}


bool AIQuery :: GetScreenPartyMembersInNodes( int region, int section, int level, int object, GopColl& out )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetScreenPartyMembersInNodes( region, section, level, object, 0, &out ) > 0 );
}


bool AIQuery :: AreScreenPartyMembersInNodes( int region, int section, int level, int object )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetScreenPartyMembersInNodes( region, section, level, object, 1, NULL ) > 0 );
}


int AIQuery :: GetScreenPartyMembersInNodes( int region, int section, int level, int object, int earlyOutSize, GopColl* out )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetPartyMembersInNodes( gServer.GetScreenPlayer(), region, section, level, object, earlyOutSize, out ) );
}


bool AIQuery :: GetHumanPartyMembersInNodes( int region, int section, int level, int object, GopColl& out )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetHumanPartyMembersInNodes( region, section, level, object, 0, &out ) > 0 );
}


bool AIQuery :: AreHumanPartyMembersInNodes( int region, int section, int level, int object )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetHumanPartyMembersInNodes( region, section, level, object, 1, NULL ) > 0 );
}


int AIQuery :: GetHumanPartyMembersInNodes( int region, int section, int level, int object, int earlyOutSize, GopColl* out )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	int found = 0;

	for ( Server::PlayerColl::const_iterator iPlayer = gServer.GetPlayers().begin(); iPlayer != gServer.GetPlayers().end(); ++iPlayer )
	{
		if ( IsValid( *iPlayer ) && !(*iPlayer)->IsComputerPlayer() )
		{
			int localFound = GetPartyMembersInNodes( *iPlayer, region, section, level, object, earlyOutSize, out );
			found += localFound;

			if ( earlyOutSize != 0 )
			{
				earlyOutSize -= localFound;
				if ( earlyOutSize <= 0 )
				{
					break;
				}
			}
		}
	}

	return ( found );
}


bool AIQuery :: Get( Go * reference, eQueryTrait qt, GopColl const & in, GopColl & out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( Get( reference, &qt, 1, in, out ) );
}


bool AIQuery :: Get( Go * reference, QtColl const & qtc, GopColl const & in, GopColl & out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( Get( reference, &*qtc.begin(), qtc.size(), in, out ) );
}


bool AIQuery :: Get( Go * reference, eQueryTrait const * qt, unsigned int qtCount, GopColl const & in, GopColl & out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetFirstN( reference, qt, qtCount, UINT_MAX, in, out ) );
}


bool AIQuery :: GetFirstN( Go * reference, eQueryTrait qt, unsigned int N, GopColl const & in, GopColl & out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetFirstN( reference, &qt, 1, N, in, out ) );
}


bool AIQuery :: GetFirstN( Go * reference, QtColl const & qtc, unsigned int N, GopColl const & in, GopColl & out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetFirstN( reference, &*qtc.begin(), qtc.size(), N, in, out ) );
}


bool AIQuery :: GetFirstN( Go * reference, eQueryTrait const * qt, unsigned int qtCount, unsigned int N, GopColl const & in, GopColl & out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	unsigned int SizeBefore = out.size();

	GopColl::const_iterator i;
	for ( i = in.begin() ; (i != in.end()) && (N > 0) ; ++i )
	{
		if ( Is( reference, (*i), qt, qtCount ) )
		{
			out.push_back( *i );
			--N;
		}
	}
	return ( SizeBefore < out.size() );
}


bool AIQuery :: GetMaxN( Go * reference, eQueryTrait qt, unsigned int N, GopColl const & In, GopColl & Out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetMaxN( reference, &qt, 1, N, In, Out ) );
}


bool AIQuery :: GetMaxN( Go * reference, QtColl const & qtc, unsigned int N, GopColl const & In, GopColl & Out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetMaxN( reference, &*qtc.begin(), qtc.size(), N, In, Out ) );
}

static bool GetN( QtBundleColl& TempQtBundleColl, Go * reference, eQueryTrait const * qt, unsigned int qtCount, unsigned int N, GopColl const & In )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	if( N == 0 )
	{
		gpassertm( 0, "Passed in suspicious value.  Check usage." );
		return false;
	}

	TempQtBundleColl.reserve( In.size() );

	GopColl::const_iterator i;

	float NormQtVal = 0.0;

	for ( i = In.begin(); i != In.end(); ++i )
	{
		if ( gAIQuery.Is( reference, (*i), qt, qtCount ) )
		{
			QtBundle& NewEntry = *TempQtBundleColl.push_back();
			NewEntry.NormQtVal = NormQtVal;
			NewEntry.pGO = *i;
		}
	}

	TempQtBundleColl.sort();

	return ( !TempQtBundleColl.empty() );
}

bool AIQuery :: GetMaxN( Go * reference, eQueryTrait const * qt, unsigned int qtCount, unsigned int N, GopColl const & In, GopColl & Out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	unsigned int OutSizeBefore = Out.size();

	QtBundleColl TempQtBundleColl;
	if ( GetN( TempQtBundleColl, reference, qt, qtCount, N, In ) )
	{
		QtBundleColl::reverse_iterator ib;
		for( ib = TempQtBundleColl.rbegin(); ib != TempQtBundleColl.rend() && N != 0; ++ib, --N )
		{
			Out.push_back( (*ib).pGO );
		}
	}

	return( OutSizeBefore < Out.size() );
}


Go * AIQuery :: GetMax( Go * reference, eQueryTrait Adj, GopColl const & In ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	GopColl temp;
	if( GetMax( reference, Adj, In, temp ) )
	{
		return temp.front();
	}
	else
	{
		return NULL;
	}
}


bool AIQuery :: GetMax( Go * reference, eQueryTrait Adj, GopColl const & In, GopColl & Out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetMaxN( reference, Adj, 1, In, Out ) );
}


bool AIQuery :: GetMax( Go * reference, QtColl const & qtc, GopColl const & In, GopColl & Out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetMaxN( reference, qtc, 1, In, Out ) );
}


bool AIQuery :: GetMinN( Go * reference, eQueryTrait qt, unsigned int N, GopColl const & In, GopColl & Out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetMinN( reference, &qt, 1, N, In, Out ) );
}


bool AIQuery :: GetMinN( Go * reference, QtColl const & qtc, unsigned int N, GopColl const & In, GopColl & Out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetMinN( reference, &*qtc.begin(), qtc.size(), N, In, Out ) );
}


bool AIQuery :: GetMinN( Go * reference, eQueryTrait const * qt, unsigned int qtCount, unsigned int N, GopColl const & In, GopColl & Out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	unsigned int OutSizeBefore = Out.size();

	QtBundleColl TempQtBundleColl;
	if ( GetN( TempQtBundleColl, reference, qt, qtCount, N, In ) )
	{
		QtBundleColl::iterator ib;
		for( ib = TempQtBundleColl.begin(); ib != TempQtBundleColl.end() && N != 0; ++ib, --N )
		{
			Out.push_back( (*ib).pGO );
		}
	}

	return( OutSizeBefore < Out.size() );
}


Go * AIQuery :: GetMin( Go * reference, eQueryTrait Adj, GopColl const & In ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	GopColl temp;
	if( GetMin( reference, Adj, In, temp ) )
	{
		return temp.front();
	}
	else
	{
		return NULL;
	}
}


bool AIQuery :: GetMin( Go * reference, eQueryTrait Adj, GopColl const & In, GopColl & Out ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return GetMinN( reference, Adj, 1, In, Out );
}


int AIQuery :: GetCount( Go * reference, QtColl const & qtc, GopColl const & in ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	int count = 0;

	GopColl::const_iterator i;

	for( i = in.begin(); i != in.end(); ++i )
	{
		if ( Is( reference, (*i), qtc ) )
		{
			++count;
		}
	}

	return count;
}


int AIQuery :: GetCount( Go * reference, eQueryTrait qt, GopColl const & in )  const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	int count = 0;

	GopColl::const_iterator i;

	for( i = in.begin(); i != in.end(); ++i )
	{
		if ( Is( reference, (*i), qt ) )
		{
			++count;
		}
	}

	return count;
}


bool AIQuery :: Is( Go const * reference, Go const * go, eQueryTrait qt ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	float value;
	return Is( reference, go, qt, value );
}


bool AIQuery :: Is( Go const * reference, Go const * go, eQueryTrait qt, float & value) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: Is", SP_AI | SP_AI_QUERY );

	if( !go->IsInActiveWorldFrustum() )
	{
		// AI shouldn't see or query objects that are in an inactive frustum...
		return( false );
	}

	switch( qt )
	{
		case QT_ANY:
		{
			return true;
			break;
		}

		case QT_LIFE:
		{
			if( go->HasActor() && go->HasAspect() )
			{
				value = go->GetAspect()->GetCurrentLife();
				return value > 0.0f;
			}
			else
			{
				return false;
			}
			break;
		}

		case QT_LIFE_LOW:
		{
			return go->HasActor() && ( go->HasMind() ? go->GetMind()->IsLifeLow() : false );
			break;
		}

		case QT_LIFE_HIGH:
		{
			return go->HasActor() && ( go->HasMind() ? go->GetMind()->IsLifeHigh() : false );
			break;
		}

		case QT_MANA_LOW:
		{
			return go->HasActor() && ( go->HasMind() ? go->GetMind()->IsManaLow() : false );
			break;
		}

		case QT_MANA_HIGH:
		{
			return go->HasActor() && ( go->HasMind() ? go->GetMind()->IsManaHigh() : false );
			break;
		}

		case QT_MANA_HEALING:
		{
			if( go->IsSpell() )
			{
				if( go->GetMagic()->GetUsageContextFlags() & UC_MANA_GIVING )
				{
					value = go->GetMagic()->GetRequiredCastLevel();
					return true;
				}
			}
			else if( go->IsPotion() )
			{
				float Amount = GetAlterationSum( go, ALTER_MANA );
				value = max( 0.0f, Amount );
				return Amount > 0;
			}
			else
			{
				value = 0.0f;
				return false;
			}
			break;
		}

		case QT_MANA_DAMAGING:
		{
			if( go->IsSpell() || go->IsPotion() )
			{
				if( go->GetMagic()->GetUsageContextFlags() & UC_OFFENSIVE )
				{
					value = go->GetMagic()->GetRequiredCastLevel();
					return true;
				}
			}
			else
			{
				value = 0.0f;
				return false;
			}
			break;
		}

		case QT_REANIMATING:
		{
			if( go->IsSpell() )
			{
				if( go->GetMagic()->GetUsageContextFlags() & UC_REANIMATING )
				{
					return true;
				}
			}
			return false;
			break;
		}

		case QT_LIFE_HEALING:
		{
			if( go->IsSpell() )
			{
				if( go->GetMagic()->GetUsageContextFlags() & UC_LIFE_GIVING )
				{
					value = go->GetMagic()->GetRequiredCastLevel();
					return true;
				}
			}
			else if( go->IsPotion() )
			{
				float Amount = GetAlterationSum( go, ALTER_LIFE );
				value = max( 0.0f, Amount );
				return Amount > 0;
			}
			else
			{
				value = 0.0f;
				return false;
			}
			break;
		}
		case QT_LIFE_DAMAGING:
		{
			if( ( go->IsSpell() || go->IsPotion() ) && (go->GetMagic()->GetUsageContextFlags() & UC_OFFENSIVE) )
			{
				//value = go->GetMagic()->GetRequiredCastLevel();
				value = go->GetMagic()->EvaluateAttackDamageModifierMax( reference, go );
				return true;
			}
			else if( go->IsWeapon() )
			{
				float weaponDamage = 0;

				if( go->HasAttack() )
				{
					weaponDamage = ( (go->GetAttack()->GetDamageMin() + go->GetAttack()->GetDamageMax() ) * 0.5f );
				}

				float Amount = GetAlterationSum( go, ALTER_LIFE );

				if( Amount < 0.0 )
				{
					Amount = AbsoluteValue( Amount );
					value = Amount + weaponDamage;
					return true;
				}
				else
				{
					value = weaponDamage;
					return value > 0;
				}
			}
			else
			{
				value = 0;
				return false;
			}
			break;
		}
		case QT_WEAPON:
		{
			return( go->IsWeapon() );
			break;
		}
		case QT_MELEE_WEAPON:
		{
			return( go->IsMeleeWeapon() );
			break;
		}
		case QT_RANGED_WEAPON:
		{
			return( go->IsRangedWeapon() );
			break;
		}
		case QT_ACTOR:
		{
			return( go->IsActor() );
			break;
		}
		case QT_ITEM:
		{
			return( go->IsItem() );
			break;
		}
		case QT_SPELL:
		{
			return( go->IsSpell() );
			break;
		}
		case QT_POTION:
		{
			return go->IsPotion();
			break;
		}
		case QT_ARMOR:
		{
			return ( go->IsArmor() );
		}
		case QT_ARMOR_WEARABLE:
		{
			return ( go->IsArmor() && !go->IsShield() );
		}
		case QT_SHIELD:
		{
			return ( go->IsShield() );
		}
		case QT_MELEE_WEAPON_SELECTED:
		{
			return go->HasInventory() ? go->GetInventory()->IsMeleeWeaponSelected() : false;
			break;
		}
		case QT_RANGED_WEAPON_SELECTED:
		{
			return go->HasInventory() ? go->GetInventory()->IsRangedWeaponSelected() : false;
			break;
		}
		case QT_ONE_SHOT_SPELL:
		{
			gpassert( go->HasMagic() );
			return go->HasMagic() && go->GetMagic()->GetIsOneShot();
			break;
		}
		case QT_MULTIPLE_SHOT_SPELL:
		{
			gpassert( go->HasMagic() );
			return go->HasMagic() && !go->GetMagic()->GetIsOneShot();
			break;
		}

		case QT_COMMAND_CAST_SPELL:
		{
			gpassert( go->HasMagic() );
			return go->HasMagic() && go->GetMagic()->GetIsCommandCast();
			break;
		}

		case QT_AUTO_CAST_SPELL:
		{
			gpassert( go->HasMagic() );
			return go->HasMagic() && !go->GetMagic()->GetIsCommandCast();
			break;
		}

		case QT_FIGHTING:
		{
			if( go->HasActor() && go->HasMind() )
			{
				Job * job = go->GetMind()->GetFrontJob( JQ_ACTION );
				if( job )
				{
					return( (job->GetTraits() & JT_OFFENSIVE) != 0 );
				}
			}
			return false;
			break;
		}
		case QT_BUSY:
		{
			return go->HasActor() && ( go->HasMind() ? go->GetMind()->AmBusy() : false );
			break;
		}
		case QT_IDLE:
		{
			return go->HasActor() && ( go->HasMind() ? !go->GetMind()->AmBusy() : true );
			break;
		}
		case QT_ALIVE_CONSCIOUS:
		{
			return go->HasActor() && ( go->GetAspect()->GetLifeState() == LS_ALIVE_CONSCIOUS );
			break;
		}
		case QT_ALIVE_UNCONSCIOUS:
		{
			return go->HasActor() && ( go->GetAspect()->GetLifeState() == LS_ALIVE_UNCONSCIOUS );
			break;
		}
		case QT_ALIVE:
		{
			return go->HasActor() && IsAlive( go->GetLifeState() );
			break;
		}
		case QT_DEAD:
		{
			return go->HasActor() && !IsAlive( go->GetAspect()->GetLifeState() );
			break;
		}
		case QT_GHOST:
		{
			return go->HasActor() && ( go->GetAspect()->GetLifeState() == LS_GHOST );
		}
		case QT_GOOD:
		{
			return go->HasActor() ? go->GetActor()->GetAlignment() == AA_GOOD : false;
			break;
		}
		case QT_EVIL:
		{
			return go->HasActor() ? go->GetActor()->GetAlignment() == AA_EVIL : false;
			break;
		}
		case QT_NEUTRAL:
		{
			return go->HasActor() ? go->GetActor()->GetAlignment() == AA_NEUTRAL : false;
			break;
		}
		case QT_FRIEND:
		{
			gpassert( reference->HasMind() );
			return( reference->GetMind()->IsFriend( go ) );
			break;
		}

		case QT_ENEMY:
		{
			gpassert( reference->HasMind() );
			return( reference->GetMind()->IsEnemy( go ) );
			break;
		}

		case QT_SURVIVAL_FACTOR:
		{
			value = CalcSurvivalFactor( go );
			return true;
			break;
		}
		case QT_OFFENSE_FACTOR:
		{
			value = CalcOffenseFactor( go );
			return true;
			break;
		}
		case QT_CASTABLE:
		{
			if( go->IsSpell() )
			{
				if( go->HasParent() )
				{
					Go * caster = go->GetParent()->GetParent();
					if( caster && caster->IsActor() )
					{
						bool manaMet	= go->GetMagic()->EvaluateManaCost( caster, NULL ) <= caster->GetAspect()->GetCurrentMana();
						bool levelMet	= go->GetMagic()->GetRequiredCastLevel() <= caster->GetActor()->GetSkillLevel( go->GetMagic()->GetSkillClass() );
						return manaMet && levelMet;
					}
				}
			}
			return( false );
			break;
		}
		case QT_ATTACKABLE:
		{
			gpassert( reference->HasMind() );

			if( !reference->GetMind()->GetMayAttack() || ( go->HasMind() && !go->GetMind()->GetMayBeAttacked() ) )
			{
				return( false );
			}
			return( true );
			break;
		}
		case QT_SELECTED:
		{
			return( go->IsSelected() );
			break;
		}
		case QT_HAS_LOS:
		{
			if( reference )
			{
				return IsLosClear( reference, go );
			}
			else
			{
				return false;
			}
			break;
		}

		case QT_PACK:
		{
			return( go->HasInventory() && go->GetInventory()->IsPackOnly() );
			break;
		}
		case QT_NONPACK:
		{
			return( go->HasInventory() && !go->GetInventory()->IsPackOnly() );
			break;
		}
		default:
		{
			gperrorf(( "AIQuery :: Is - unsupported query %s - please report to Bart\n", ToString( qt ) ));
			break;
		}
	};
	return false;
}


bool AIQuery :: Is( Go const * reference, Go const * go, QtColl const & qtColl ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( Is( reference, go, &*qtColl.begin(), qtColl.size() ) );
}


bool AIQuery :: Is( Go const * reference, Go const * go, eQueryTrait const * qt, unsigned int qtCount ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	for( ; qtCount > 0; ++qt, --qtCount )
	{
		if( !Is( reference, go, *qt) )
		{
			return false;
		}
	}
	return true;
}


bool AIQuery :: Filter( const Go * go, const Go * source, eOccupantFilter filter, bool isUnionFilter ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: Filter", SP_AI | SP_AI_QUERY );

	// make sure it didn't get by our filter
	gpassert( AIQueryInterestFilter( go ) );

	// early outs
	if ( go == NULL )
	{
		return ( false );
	}
	else if ( (filter & OF_MASK_ALL) == OF_NONE )
	{
		return ( true );
	}

// Test culling first.

#	define CULL( WHICH, TEST )										\
	if ( filter & WHICH )											\
	{																\
		if ( TEST )  return ( false );								\
	}

	CULL( OF_CULL_PARTY_MEMBERS  , go->IsInSameParty( source ) )
	CULL( OF_CULL_SAME_MEMBERSHIP, (source != NULL) && source->GetCommon()->GetMembership().ContainsAny( go->GetCommon()->GetMembership() )
								   && (!source->HasMind() || source->GetMind()->IsFriend( go )) )
	CULL( OF_CULL_ACTORS         , go->HasActor() )
	CULL( OF_CULL_GHOSTS         , go->IsGhost() )
	CULL( OF_CULL_INVINCIBLE	 , go->HasAspect() && go->GetAspect()->GetIsInvincible() )

#	undef CULL

// Test filters next.

	bool matched = false;

#	define FILTER( WHICH, TEST )									\
	if ( filter & WHICH )											\
	{																\
		if ( TEST )													\
		{															\
			matched = true;											\
			if ( filter & OF_MATCH_ANY )  return ( true );			\
		}															\
		else														\
		{															\
			if ( !(filter & OF_MATCH_ANY) )  return ( false );		\
		}															\
	}

	FILTER( OF_SCREEN_CONTROLLED  , go->IsScreenPlayerOwned() )
	FILTER( OF_HUMAN_CONTROLLED   , go->GetPlayer()->GetController() == PC_HUMAN )
	FILTER( OF_COMPUTER_CONTROLLED, go->GetPlayer()->GetController() == PC_COMPUTER )
	FILTER( OF_ACTORS             , go->IsActor() )
	FILTER( OF_ALIVE              , IsAlive( go->GetLifeState() ) )
	FILTER( OF_ALIVE_OR_GHOST	  , go->IsGhost() || IsAlive( go->GetLifeState() ) )
	FILTER( OF_COLLIDABLES        , go->GetAspect()->GetIsCollidable() )
	FILTER( OF_BREAKABLES         , go->IsBreakable() )
	FILTER( OF_FRIENDS            , (source != NULL) && source->HasMind() && source->GetMind()->IsFriend( go ) )
	FILTER( OF_ENEMIES            , (source != NULL) && source->HasMind() && source->GetMind()->IsEnemy( go ) )
	FILTER( OF_CAN_BURN           , IsAlive( go->GetLifeState() ) && !go->IsFireImmune() )
	FILTER( OF_ITEMS              , go->IsItem() )
	FILTER( OF_CAN_MOVE_SELF	  , go->HasFollower() )

#	undef FILTER

	if ( filter & OF_PARTY_MEMBERS )
	{
		bool success = false;

		if ( (source != NULL) && !(filter & (OF_HUMAN_CONTROLLED | OF_COMPUTER_CONTROLLED | OF_SCREEN_CONTROLLED)) )
		{
			success = go->IsInSameParty( source );
		}
		else
		{
			Go* party = go->GetOwningParty();
			if ( party != NULL )
			{
				if ( (filter & OF_SCREEN_CONTROLLED) && (party == gServer.GetScreenParty()) )
				{
					success = true;
				}

				if ( !success && (filter & OF_HUMAN_CONTROLLED) )
				{
					const Server::PlayerColl& players = gServer.GetPlayers();
					Server::PlayerColl::const_iterator i, ibegin = players.begin(), iend = players.end();
					for ( i = ibegin ; i != iend ; ++i )
					{
						if ( IsValid(*i) && !(*i)->IsComputerPlayer() )
						{
							const GopColl& parties = (*i)->GetPartyColl();
							GopColl::const_iterator j, jbegin = parties.begin(), jend = parties.end();
							for ( j = jbegin ; j != jend ; ++j )
							{
								if ( *j == go )
								{
									success = true;
									break;
								}
							}
							if ( success )
							{
								break;
							}
						}
					}
				}

				if ( !success && (filter & OF_COMPUTER_CONTROLLED) )
				{
					const GopColl& parties = gServer.GetComputerPlayer()->GetPartyColl();
					GopColl::const_iterator i, ibegin = parties.begin(), iend = parties.end();
					for ( i = ibegin ; i != iend ; ++i )
					{
						if ( party == *i )
						{
							success = true;
							break;
						}
					}
				}
			}
		}

		if ( success )
		{
			matched = true;
			if ( filter & OF_MATCH_ANY )  return ( true );
		}
		else
		{
			if ( !(filter & OF_MATCH_ANY) )  return ( false );
		}
	}

	// may have OF_MATCH_ANY set but not hit anything
	if ( !matched && !isUnionFilter && (filter & OF_MATCH_ANY) )
	{
		return ( false );
	}

	// guess it's a hit (possibly was cull-only query)
	return ( true );
}


float AIQuery :: GetTraitValue( Go const * reference, Go const * go, eQueryTrait qt ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	float value = 0.0f;
	Is( reference, go, qt, value );
	return value;
}


bool AIQuery :: IsInRange( SiegePos const & PosA, SiegePos const & PosB, float const Range )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetDistanceSquared( PosA, PosB ) <= (Range*Range) );
}


bool AIQuery :: IsInRange2D( SiegePos const & PosA, SiegePos const & PosB, float const Range )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return Get2DDistance( PosA, PosB ) <= Range;
}


bool AIQuery :: IsInRange( const Go* a, const Go* b, float range )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetDistance( a, b ) <= range );
}


bool AIQuery :: IsInRange2D( const Go* a, const Go* b, float range )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( Get2DDistance( a, b ) <= range );
}


bool AIQuery :: GetOccupantsInSphere( const SiegePos& center, float radius, GopColl& out )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetOccupantsInSphere( center, radius, NULL, NULL, 0, OF_NONE, &out ) > 0 );
}


bool AIQuery :: DoesSphereHaveOccupants( const SiegePos& center, float radius )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetOccupantsInSphere( center, radius, NULL, NULL, 1, OF_NONE, NULL ) > 0 );
}


bool AIQuery :: DoesSphereHaveHumanControlledOccupants( const SiegePos& center, float radius )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( GetOccupantsInSphere( center, radius, NULL, NULL, 1, OF_HUMAN_CONTROLLED, NULL ) > 0 );
}


int AIQuery :: GetOccupantsInSphere(
		const SiegePos& center,
		float           radius,
		const Go*       sourceObject,
		const GoidColl* ignoreColl,
		int             earlyOutSize,
		eOccupantFilter filter,
		GopColl*        out,
		bool            cache )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: GetOccupantsInSphere", SP_AI | SP_AI_QUERY );
	CHECK_PRIMARY_THREAD_ONLY;

	gpassertm( radius < 500, "Sanity check on GetOccupantsInSphere distance!!" );

//	uncomment me for log-profiling purposes -sb
//
//	gpgenericf(( "pos = %s, %f, %f, %f, r = %f, ignore = %s, earlyout = %d, filter = %x\n",
//				 center.node.ToString().c_str(), center.pos.x, center.pos.y, center.pos.z, radius, sourceObject ? "y" : "n", earlyOutSize, filter ));

	// early out on bad query (this is ok)
	SiegeNode* node = gSiegeEngine.IsNodeValid( center.node );
	if ( node == NULL )
	{
		return ( false );
	}

#	if !GP_RETAIL

	// check to see if the center is (a) valid and (b) not offset beyond itself.
	// aiquery centers must be within the node otherwise weird things will
	// happen...
	SiegePos localCenter( center );
	bool okCenter = false;
	if ( gSiegeEngine.AdjustPointToTerrain( localCenter, LF_ALL ) )
	{
		if ( center.node == localCenter.node )
		{
			okCenter = true;
		}
		else
		{
/*			gperrorf(( "Convention-violating AIQuery::GetOccupantsInSphere() call!\n"
					   "\n"
					   "SiegePos passed in for center (%g,%g,%g,%s) is not contained "
					   "within a node!! This will cause unexpected behavior from "
					   "this function (though not a crash, just may not get what "
					   "you want...)\n"
					   "\n"
					   "Please report this heinous error with full call stack.\n",
					   center.pos.x, center.pos.y, center.pos.z, center.node.ToString().c_str() ));*/
		}
	}

	// draw the query
	if ( gWorldOptions.GetAiQueryBoxes() )
	{
		gWorld.DrawDebugSphere(
				center, radius,
				okCenter ? COLOR_YELLOW : COLOR_RED,
				okCenter ? gpstring::EMPTY : "<ILLEGAL>" );
	}

#	endif // !GP_RETAIL

	// how many we found goes in here
	int found = 0;

	// special optimization: if (screen) party members only, we can just compare
	// world pos directly, much faster and less complex than query cache system
	if ( !(filter & OF_MATCH_ANY) && ((filter & OF_MASK_TRAITS) == OF_PARTY_MEMBERS) )
	{
		// got a party to use?
		Go* sourceParty = NULL;
		if ( (sourceObject != NULL) && ((sourceParty = sourceObject->GetOwningParty()) != NULL) )
		{
			// use source's party
			found += GetPartyMembersInSphere( center, radius, sourceParty, ignoreColl, earlyOutSize, filter, out );
		}
		else
		{
			// screen party members
			if ( filter & OF_SCREEN_CONTROLLED )
			{
				found += GetPartyMembersInSphere( center, radius, gServer.GetScreenParty(), ignoreColl, earlyOutSize, filter, out );
				if ( earlyOutSize > 0 )
				{
					earlyOutSize -= found;
					if ( earlyOutSize == 0 )
					{
						return ( found );
					}
				}
			}

			// human party members
			if ( filter & OF_HUMAN_CONTROLLED )
			{
				const Server::PlayerColl& players = gServer.GetPlayers();
				Server::PlayerColl::const_iterator i, ibegin = players.begin(), iend = players.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					if ( IsValid(*i) && !(*i)->IsComputerPlayer() )
					{
						const GopColl& parties = (*i)->GetPartyColl();
						GopColl::const_iterator j, jbegin = parties.begin(), jend = parties.end();
						for ( j = jbegin ; j != jend ; ++j )
						{
							found += GetPartyMembersInSphere( center, radius, *j, ignoreColl, earlyOutSize, filter, out );
							if ( earlyOutSize > 0 )
							{
								earlyOutSize -= found;
								if ( earlyOutSize == 0 )
								{
									return ( found );
								}
							}
						}
					}
				}
			}

			// computer party members
			if ( filter & OF_COMPUTER_CONTROLLED )
			{
				const GopColl& parties = gServer.GetComputerPlayer()->GetPartyColl();
				GopColl::const_iterator i, ibegin = parties.begin(), iend = parties.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					found += GetPartyMembersInSphere( center, radius, *i, ignoreColl, earlyOutSize, filter, out );
					if ( earlyOutSize > 0 )
					{
						earlyOutSize -= found;
						if ( earlyOutSize == 0 )
						{
							return ( found );
						}
					}
				}
			}
		}
	}
	else if ( !cache || !m_UseQueryCache )
	{
		// get the worldspace position of the center
		vector_3 query_center = node->NodeToWorldSpace( center.pos );

		// make a new list to traverse
		SiegeNodeColl scoll;
		scoll.push_back( node );

		// make a visit counter to keep track of who we've seen and who we haven't
		unsigned int visitCount = RandomDword();
		node->SetAPVisited( visitCount );

		// color for rendering
#		if !GP_RETAIL
		float intensity = 1.0f;
#		endif // !GP_RETAIL

		// find all connecting nodes
		GoDb::SiegeGoDb::iterator j, jbegin, jend;
		while ( !scoll.empty() )
		{
			// get the node
			SiegeNode* node = scoll.back();
			scoll.pop_back();

			// render boxes
#			if !GP_RETAIL
			if ( gWorldOptions.GetAiQueryBoxes() )
			{
				DWORD color = MAKEDWORDCOLOR( vector_3( intensity, intensity, 0 ) );
				intensity -= .05f;
				if ( intensity < 0 )
				{
					intensity = 1.0f;
				}

				const vector_3& b0 = node->GetWorldSpaceMinimumBounds();
				const vector_3& b1 = node->GetWorldSpaceMaximumBounds();

				vector_3 p0( b1.x, b1.y, b0.z );
				vector_3 p1( b0.x, b1.y, b0.z );
				vector_3 p2( b0.x, b0.y, b0.z );
				vector_3 p3( b1.x, b0.y, b0.z );
				vector_3 p4( b1.x, b1.y, b1.z );
				vector_3 p5( b0.x, b1.y, b1.z );
				vector_3 p6( b0.x, b0.y, b1.z );
				vector_3 p7( b1.x, b0.y, b1.z );

				gSiegeEngine.GetWorldSpaceLines().DrawLine( p0, p1, color );
				gSiegeEngine.GetWorldSpaceLines().DrawLine( p2, p1, color );
				gSiegeEngine.GetWorldSpaceLines().DrawLine( p2, p3, color );
				gSiegeEngine.GetWorldSpaceLines().DrawLine( p0, p3, color );
				gSiegeEngine.GetWorldSpaceLines().DrawLine( p4, p5, color );
				gSiegeEngine.GetWorldSpaceLines().DrawLine( p6, p5, color );
				gSiegeEngine.GetWorldSpaceLines().DrawLine( p6, p7, color );
				gSiegeEngine.GetWorldSpaceLines().DrawLine( p4, p7, color );
				gSiegeEngine.GetWorldSpaceLines().DrawLine( p0, p4, color );
				gSiegeEngine.GetWorldSpaceLines().DrawLine( p1, p5, color );
				gSiegeEngine.GetWorldSpaceLines().DrawLine( p2, p6, color );
				gSiegeEngine.GetWorldSpaceLines().DrawLine( p3, p7, color );
			}
#			endif // !GP_RETAIL

			// find occupants of this node
			if ( gGoDb.GetNodeOccupantsIters( node->GetGUID(), jbegin, jend ) )
			{
				// go through all occupants of this node
				for ( j = jbegin ; j != jend ; ++j )
				{
					Go* go = j->second;

					/////////////////////////////////////////////
					//	check early-out conditions

					if ( sourceObject == go )
					{
						continue;
					}

					if ( (ignoreColl != NULL) && stdx::has_any( *ignoreColl, go->GetGoid() ) )
					{
						continue;
					}

					if ( !AIQueryInterestFilter( go ) )
					{
						continue;
					}

					if ( !Filter( go, sourceObject, filter ) )
					{
						continue;
					}

					/////////////////////////////////////////////
					// spatial tests

					// adjust for personal space to handle issues of small creatures
					// fighting large creatures

					// test distance

					vector_3 bsphere_center(DoNotInitialize);
					float sum_radius;

					// $$ Do we ALWAYS want to use the 3D position of the bsphere center?
					// $$ ...or do we want to use the projection of the sphere onto the ground?
					// $$ --biddle

					if (go->HasAspect())
					{
						vector_3 bsphere_hdiag(DoNotInitialize);
						float bsphere_radius;

						go->GetAspect()->GetWorldSpaceBoundingVolume (bsphere_center, bsphere_hdiag );
						bsphere_radius = bsphere_hdiag.Length();
						sum_radius = radius+bsphere_radius;
					}
					else
					{
						sum_radius = go->HasMind() ? ( radius + go->GetMind()->GetPersonalSpaceRange() ) : ( radius+0.5f );
						bsphere_center = go->GetPlacement()->GetWorldPosition();
					}

					gpassert( go != NULL );
					gpassert( go == j->second );

					if ( query_center.Distance2( bsphere_center ) < Square(sum_radius) )
					{
						if ( out != NULL )
						{
							out->push_back( go );
						}
						if ( (++found >= earlyOutSize) && (earlyOutSize > 0) )
						{
							// $ early abort
							return ( found );
						}
					}
				}
			}

			// go through neighbors
			SiegeNodeDoorConstIter i, ibegin = node->GetDoors().begin(), iend = node->GetDoors().end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				// get node
				SiegeNode* node = gSiegeEngine.IsNodeValid( (*i)->GetNeighbour() );
				if( node == NULL )
				{
					continue;
				}

				// check existence and whether or not we've already touched it
				if( !node->IsOwnedByAnyFrustum() || (node->GetAPVisited() == visitCount) )
				{
					continue;
				}

				// set it as visited so no one else does work on it
				node->SetAPVisited( visitCount );

				// see if this node is in our requested sphere
				if ( SphereIntersectsBox( query_center, radius, node->GetWorldSpaceMinimumBounds(), node->GetWorldSpaceMaximumBounds() ) )
				{
					scoll.push_back( node );
				}
			}
		}
	}
	else
	{
		// look for cached query
		MetaQuery* metaQuery = NULL;
		Query* query = NULL;
		if ( !m_MetaQueryDb.empty() )
		{
			// get a threshold
			float glomDistance2 = Square( SPHERE_QUERY_CLOSE_ENOUGH_TO_GLOM_RATIO * radius );

			// attempt to find a cached metaquery
			std::pair <MetaQueryDb::iterator, MetaQueryDb::iterator>
					rc = m_MetaQueryDb.equal_range( center.node );
			for ( ; rc.first != rc.second ; ++rc.first )
			{
				MetaQuery* localMetaQuery = &rc.first->second;

				// make sure we're in the range of the metaquery
				if ( localMetaQuery->CanGlom( center.pos, glomDistance2 ) )
				{
					// ok now check for a query match
					QueryColl::iterator i, ibegin = localMetaQuery->m_Queries.begin(), iend = localMetaQuery->m_Queries.end();
					for ( i = ibegin ; i != iend ; ++i )
					{
						if ( i->IsMatch( center.pos, radius, filter ) )
						{
							// cool!
							query = &*i;
							break;
						}
					}

					// found what we're looking for - abort
					metaQuery = localMetaQuery;
					break;
				}
			}
		}

#		if !GP_RETAIL
		gpstring aiqNotes;
#		endif // !GP_RETAIL

		// need to create new metaquery?
		if ( metaQuery == NULL )
		{
			// build new one
			metaQuery = &m_MetaQueryDb.insert( std::make_pair( center.node, MetaQuery() ) )->second;
			metaQuery->m_Node    = center.node;
			metaQuery->m_NodePos = center.pos;

#			if !GP_RETAIL
			if ( gWorldOptions.GetAiQueryBoxes() )
			{
				aiqNotes += "New MetaQuery added\n";
			}
#			endif // !GP_RETAIL
		}

		// need to add new query?
		if ( query == NULL )
		{
			// add it
			query = &*metaQuery->m_Queries.insert( metaQuery->m_Queries.end() );
			query->m_NodePos    = center.pos;
			query->m_Radius     = radius;
			query->m_Filter     = filter;

			// glom it in
			metaQuery->Glom( query );

#			if !GP_RETAIL
			if ( gWorldOptions.GetAiQueryBoxes() )
			{
				aiqNotes += "New Query glommed\n";
			}
#			endif // !GP_RETAIL
		}

		// update expiration time
		query->m_ExpireTime = PreciseAdd( gWorldTime.GetTime(), SPHERE_QUERY_EXPIRATION_TIME);
		metaQuery->RecalcTimeout();

		// ok now perform the query
		found = metaQuery->Query( query, sourceObject, ignoreColl, earlyOutSize, out );

#		if !GP_RETAIL
		if ( gWorldOptions.GetAiQueryBoxes() )
		{
			gWorld.DrawDebugSphere( center, radius, COLOR_GREEN, aiqNotes );
		}
#		endif // !GP_RETAIL
	}

	return ( found );
}


int AIQuery :: GetPartyMembersInSphere(
		const SiegePos& center,
		float           radius,
		const Go*       party,
		const GoidColl* ignoreColl,
		int             earlyOutSize,
		eOccupantFilter filter,
		GopColl*        out )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: GetPartyMembersInSphere", SP_AI | SP_AI_QUERY );
	CHECK_PRIMARY_THREAD_ONLY;

	int found = 0;

	// $$$ This code has the same inaccuracies that I found in the GetOccupantsOfSphere
	// $$$ I chose NOT to modify it use the distance to the center of the bounding spheres
	// $$$ for fear of breaking the expected behaviour - biddle

	// iterate over party children and test each's distance
	if ( (party != NULL) && !party->GetChildren().empty() )
	{
		// get comparison vars
		const SiegeNode* pNode	= gSiegeEngine.IsNodeValid( center.node );
		DWORD frusta			= pNode->GetFrustumOwnedBitfield();
		if ( frusta && pNode )
		{
			vector_3 c = pNode->NodeToWorldSpace( center.pos );
			float r2 = Square( radius + 0.5f );

			// party members are actors, no need to filter that
			filter &= NOT( OF_ACTORS );

			// ignore controlling bits - we'll be called by someone who has already figured it out
			filter &= OF_MASK_OWNER;

			// iterate
			GopColl::const_iterator i, ibegin = party->GetChildBegin(), iend = party->GetChildEnd();
			for ( i = ibegin ; i != iend ; ++i )
			{
				// test ignores first
				if ( *i == party )
				{
					continue;
				}
				if ( (ignoreColl != NULL) && stdx::has_any( *ignoreColl, (*i)->GetGoid() ) )
				{
					continue;
				}

				// additional filtering required?
				if ( (filter != OF_PARTY_MEMBERS) && (filter != OF_NONE) )
				{
					// see if it hits
					if ( !Filter( *i, party, filter & NOT( OF_PARTY_MEMBERS ) ) )
					{
						continue;
					}
				}

				// check to make sure that this object shares a frustum with the node
				if ( !(MakeInt( (*i)->GetWorldFrustumMembership() ) & frusta) )
				{
					continue;
				}

				// adjust for personal space to handle issues of small creatures
				// fighting large creatures
				float localr2 = (*i)->HasMind() ? Square( radius + (*i)->GetMind()->GetPersonalSpaceRange() ) : r2;

				// test distance
				if ( c.Distance2( (*i)->GetPlacement()->GetWorldPosition() ) < localr2 )
				{
					if ( out != NULL )
					{
						out->push_back( *i );
					}
					if ( (++found >= earlyOutSize) && (earlyOutSize > 0) )
					{
						break;
					}
				}
			}
		}
	}

	return ( found );
}


vector_3 AIQuery :: GetGeometricCenter( GoidColl const & List )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	vector_3 WorldPosition = vector_3::ZERO;

	if( !List.empty() )
	{
		GoidColl::const_iterator i;

		for( i=List.begin(); i!=List.end(); ++i )
		{
			WorldPosition += GoHandle( *i )->GetPlacement()->GetWorldPosition();
		}

		WorldPosition /= float(List.size());
	}

	return( WorldPosition );
}


vector_3 AIQuery :: GetGeometricCenterWithDeltas( GopColl const & List, GOVectorColl & PositionList )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	GOVectorColl TempColl;

	vector_3 Center = GetGeometricCenter( List, TempColl );

	GOVectorColl::iterator i;
	for( i = TempColl.begin(); i != TempColl.end(); ++i )
	{
		vector_3 Delta = (*i).second - Center;
		GOVectorColl::value_type NewEntry( (*i).first, Delta );
		PositionList.push_back( NewEntry );
	}

	return Center;
}


vector_3 AIQuery :: GetGeometricCenter( GopColl const & List, GOVectorColl & PositionList )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	GoHandle hGO;

	vector_3 WorldPosition;

	if( !List.empty() )
	{
		PositionList.clear();

		vector_3 GOWorldPosition;

		GopColl::const_iterator i;

		for( i=List.begin(); i!=List.end(); ++i ) {

			GOWorldPosition = (*i)->GetPlacement()->GetWorldPosition();
			WorldPosition	+= GOWorldPosition;

			GOVectorColl::value_type NewEntry( *i, GOWorldPosition );
			PositionList.push_back( NewEntry );
		}

		WorldPosition /= float( List.size() );
	}
	else {
		gpassertm( 0, "Something could be wrong.  Passed in empty go list" );
	}

	return( WorldPosition );
}


bool AIQuery :: GetGeometricCenter( GopColl const & List, vector_3 & Center )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	GoHandle hGO;

	if( !List.empty() ) {

		Center = vector_3( 0,0,0 );

		GopColl::const_iterator i;

		for( i = List.begin(); i != List.end(); ++i )
		{
			Center += (*i)->GetPlacement()->GetWorldPosition();
		}

		Center = Center / float( List.size() );

		return true;
	}

	return false;
}


float AIQuery :: GetDistanceSquared( SiegePos const & start, SiegePos const & end )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	if( !gSiegeEngine.IsNodeInAnyFrustum( start.node ) )
	{
		gpwarningf(("AIQuery::GetDistanceSquared() Begin pos is not in the world\n"));
		return( FLOAT_MAX );
	}
	else if (!gSiegeEngine.IsNodeInAnyFrustum( end.node ) )
	{
		gpwarningf(("AIQuery::GetDistanceSquared() End pos is not in the world\n"));
		return( FLOAT_MAX );
	}

	vector_3 diff = gSiegeEngine.GetDifferenceVector( start, end );
	return( Length2( diff ) );
}


float AIQuery :: GetDistance( SiegePos const & Begin, SiegePos const & End )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	if( !gSiegeEngine.IsNodeInAnyFrustum( Begin.node ) )
	{
		gpwarningf(("AIQuery::GetDistance() Begin pos is not in the world\n"));
		return( FLOAT_MAX );
	}
	else if (!gSiegeEngine.IsNodeInAnyFrustum( End.node ) )
	{
		gpwarningf(("AIQuery::GetDistance() End pos is not in the world\n"));
		return( FLOAT_MAX );
	}

	vector_3 diff = gSiegeEngine.GetDifferenceVector( Begin, End );
	return( Length( diff ) );
}


float AIQuery :: Get2DDistance( SiegePos const & Begin, SiegePos const & End )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	if( !gSiegeEngine.IsNodeInAnyFrustum( Begin.node ) )
	{
		gpwarningf(("AIQuery::Get2DDistance() Begin pos is not in the world\n"));
		return( FLOAT_MAX );
	}
	else if (!gSiegeEngine.IsNodeInAnyFrustum( End.node ) )
	{
		gpwarningf(("AIQuery::Get2DDistance() End pos is not in the world\n"));
		return( FLOAT_MAX );
	}

	vector_3 diff = gSiegeEngine.GetDifferenceVector( Begin, End );
	diff.y = 0;
	return( Length( diff ) );
}


vector_3 AIQuery :: GetDifferenceVector( SiegePos const & Begin, SiegePos const & End )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	if( !gSiegeEngine.IsNodeInAnyFrustum( Begin.node ) )
	{
		gpwarningf(("AIQuery::GetDifferenceVector() Begin pos is not in the world\n"));
		return( vector_3( FLOAT_MAX ) );
	}
	else if (!gSiegeEngine.IsNodeInAnyFrustum( End.node ) )
	{
		gpwarningf(("AIQuery::GetDifferenceVector() End pos is not in the world\n"));
		return( vector_3( FLOAT_MAX ) );
	}

	return( gSiegeEngine.GetDifferenceVector( Begin, End ) );
}


Go * AIQuery :: GetClosest( Go const * origin, GopColl const & in )
{
	Go * closestGo = NULL;
	float closestDistance = FLT_MAX;

	for( GopColl::const_iterator i = in.begin(); i != in.end(); ++i )
	{
		float dist = GetDistance( origin, *i );
		if( dist < closestDistance )
		{
			closestDistance = dist;
			closestGo		= ccast<Go*>(origin);
		}
	}

	gpassert( closestGo );
	return closestGo;
}


void AIQuery :: SortByDistance( Go const * from, GopColl const & to, GopColl & out )
{
	// $$ opt later
	// $$ note: easiest way to opt this is just copy "to" into "out" and then
	//          post-sort "out" using a predicate that does GetDistance(). -sb

	typedef std::multimap<float, Go*> distMap;
	distMap dmap;
	for( GopColl::const_iterator it = to.begin(); it != to.end(); ++it )
	{
		dmap.insert( std::make_pair( GetDistance( from, *it ), *it ) );
	}

	for( distMap::iterator im = dmap.begin(); im != dmap.end(); ++im )
	{
		out.push_back( (*im).second );
	}
}


float AIQuery :: GetDistance( const Go* a, const Go* b )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	if ( !a || !b || !(MakeInt( a->GetWorldFrustumMembership() ) & MakeInt( b->GetWorldFrustumMembership() )) )
	{
		gpwarningf(( "AIQuery::GetDistance() both Go's for comparison do not share a frustum\n" ));
		return( FLOAT_MAX );
	}

	float personalSpace = a->HasMind() ? a->GetMind()->GetPersonalSpaceRange() : ( a->HasAspect() ? a->GetAspect()->GetBoundingSphereRadius() : 0 );
	personalSpace		+= b->HasMind() ? b->GetMind()->GetPersonalSpaceRange() : ( b->HasAspect() ? b->GetAspect()->GetBoundingSphereRadius() : 0 );

	return ( max( 0.0f, a->GetPlacement()->GetWorldPosition().Distance( b->GetPlacement()->GetWorldPosition() ) - personalSpace ) );
}


float AIQuery :: Get2DDistance( const Go* a, const Go* b )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	if ( !a || !b || !(MakeInt( a->GetWorldFrustumMembership() ) & MakeInt( b->GetWorldFrustumMembership() )) )
	{
		gpwarningf(( "AIQuery::Get2DDistance() both Go's for comparison do not share a frustum\n" ));
		return( FLOAT_MAX );
	}

	float personalSpace = a->HasMind() ? a->GetMind()->GetPersonalSpaceRange() : a->GetAspect()->GetBoundingSphereRadius();
	personalSpace		+= b->HasMind() ? b->GetMind()->GetPersonalSpaceRange() : b->GetAspect()->GetBoundingSphereRadius();

	vector_3 diff = a->GetPlacement()->GetWorldPosition() - b->GetPlacement()->GetWorldPosition();
	diff.y = 0;

	return ( max( 0.0f, diff.Length() - personalSpace ) );
}


vector_3 AIQuery :: GetDifferenceVector( const Go* a, const Go* b )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	if ( !a || !b || !(MakeInt( a->GetWorldFrustumMembership() ) & MakeInt( b->GetWorldFrustumMembership() )) )
	{
		gpwarningf(( "AIQuery::GetDifferenceVector() both Go's for comparison do not share a frustum\n" ));
		return( vector_3( FLOAT_MAX ) );
	}

	return ( b->GetPlacement()->GetWorldPosition() - a->GetPlacement()->GetWorldPosition() );
}


void AIQuery :: GetTerrainPosition( SiegePos const & Center, float Radius, float RadAngle, SiegePos & NewPosition )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	NewPosition.node = Center.node;
	NewPosition.pos = ( vector_3::NORTH * Radius );
	NewPosition.pos.RotateY( RadAngle );
}


bool AIQuery :: GetTerrainPosition( SiegePos const & Center, float Radius, SiegePos & NewPosition )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	GetTerrainPosition( Center, Radius, Random( PI2 ), NewPosition );
	return gSiegeEngine.AdjustPointToTerrain( NewPosition );
}


bool AIQuery :: GetTerrainPositionAtEnd( SiegePos const & begin, SiegePos const & end, float r, float minAngle, float maxAngle, SiegePos & out )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return( GetTerrainPositionAtEnd( begin, end, r, minAngle + Random( maxAngle - minAngle ), out ) );
}


bool AIQuery :: GetTerrainPositionAtEnd( SiegePos const & begin, SiegePos const & end, float r, float alpha, SiegePos & out )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	if( !gSiegeEngine.IsNodeInAnyFrustum( begin.node ) )
	{
		gpwarningf(("AIQuery::GetTerrainPositionAtEnd() Begin pos is not in the world\n"));
		return( false );
	}
	else if (!gSiegeEngine.IsNodeInAnyFrustum( end.node ) )
	{
		gpwarningf(("AIQuery::GetTerrainPositionAtEnd() End pos is not in the world\n"));
		return( false );
	}

	out			= end;
	out.pos		+= Normalize( gSiegeEngine.GetDifferenceVector( end, begin ) ) * r;
	out.pos.RotateY( alpha );
	return gSiegeEngine.AdjustPointToTerrain( out );
}


float AIQuery :: CalcOffenseFactor( Go const * actor ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	// todo: take reload and cast delays into account
	//		 do this with the Is() methods that return some value...
	gpassert( actor->HasActor() && actor->HasMind() && actor->HasAttack() );

	GopColl possibleWeapons;

	//	account for most powerful spell

	float damageFactor = actor->GetAspect()->GetCurrentLife();

	GopColl tempItemsA;
	GopColl tempItemsB;
	actor->GetMind()->GetAutoItems( QT_SPELL, tempItemsA );
	Get( NULL, QT_CASTABLE, tempItemsA, tempItemsB );

	Go * lifeSpell = GetMax( NULL, QT_MANA_DAMAGING, tempItemsB );
	Go * manaSpell = GetMax( NULL, QT_LIFE_DAMAGING, tempItemsB );

	if( lifeSpell )
	{
		float damage = GetTraitValue( NULL, lifeSpell, QT_LIFE_DAMAGING );
		if( lifeSpell->GetMagic()->GetCastReloadDelay() > 0.0f )
		{
			damage = damage / lifeSpell->GetMagic()->GetCastReloadDelay();
		}
		damageFactor += damage;
	}

	if( manaSpell )
	{
		float damage = GetTraitValue( NULL, manaSpell, QT_MANA_DAMAGING );
		if( manaSpell->GetMagic()->GetCastReloadDelay() > 0.0f )
		{
			damage = damage / manaSpell->GetMagic()->GetCastReloadDelay();
		}
		damageFactor += damage;
	}

	//	account for most powerful weapon

	actor->GetMind()->GetAutoItems( QT_WEAPON, tempItemsA );
	Go * maxWeapon = GetMax( NULL, QT_LIFE_DAMAGING, tempItemsA );

	if( maxWeapon )
	{
		float damage = GetTraitValue( NULL, maxWeapon, QT_LIFE_DAMAGING );
		if( maxWeapon->HasAttack() && maxWeapon->GetAttack()->GetReloadDelay() > 0.0f )
		{
			damage = damage / maxWeapon->GetAttack()->GetReloadDelay();
		}
		damageFactor += damage;
	}

	return damageFactor;
}


float AIQuery :: CalcSurvivalFactor( Go const * actor ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	gpassert( actor->HasActor() && actor->HasMind() && actor->HasAttack() );

	EngagedMeMap::const_iterator i;
	EngagedMeMap::const_iterator ibegin	= actor->GetMind()->GetEngagedMeMap().begin();
	EngagedMeMap::const_iterator iend	= actor->GetMind()->GetEngagedMeMap().end();

	float totalEnemyOffenseFactor = 0.0f;

	for( i = ibegin; i != iend; ++i )
	{
		GoHandle engaged( (*i).first );
		if( engaged && actor->GetMind()->IsEnemy( engaged.Get() ) )
		{
			totalEnemyOffenseFactor += CalcOffenseFactor( engaged.Get() );
		}
	}

	return( CalcOffenseFactor( actor ) - totalEnemyOffenseFactor );
}


Go * AIQuery :: CalcSecondTierTarget( Go const * attacker, Go const * target )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	gpassert( attacker );

	if( !target )
	{
		return NULL;
	}

	if( Random( 1.0f ) < attacker->GetMind()->ActorBalancedAttackPreference() )
	{
		if( !target->HasParent() || !target->GetParent()->HasParty() )
		{
			return const_cast<Go*>(target);
		}

		Go * party = target->GetParent();

		Go * minEngagedGo = NULL;
		int minEngagedCount = INT_MAX;

		for( GopColl::const_iterator i = party->GetChildBegin(); i != party->GetChildEnd(); ++i )
		{
			if( !IsAlive( (*i)->GetLifeState() ) ||
				( (*i)->GetMind()->GetEngagedMeAttackerCount() == 8 ) )
			{
				continue;
			}

			if( GetDistance( attacker, (*i) ) > ( attacker->GetMind()->GetSightRange() * 1.5f ) )
			{
				continue;
			}

			if( (*i)->GetMind()->GetEngagedMeAttackerCount() < minEngagedCount )
			{
				minEngagedCount = (*i)->GetMind()->GetEngagedMeAttackerCount();
				minEngagedGo = (*i);
			}
		}
		return minEngagedGo;
	}
	else
	{
		return const_cast<Go*>(target);
	}
}


void AIQuery :: GetLOSPoint( Go const * go, SiegePos & point ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	if( go->HasMind() && go->GetMind()->GetSightOriginHeight() > 0 )
	{
		point = go->GetPlacement()->GetPosition();
		point.pos.y += go->GetMind()->GetSightOriginHeight();
	}
	else if( !(go->HasBody() && go->GetBody()->GetBoneTranslator().GetPosition( go, BoneTranslator::BODY_ANTERIOR, point, vector_3::ZERO, false )) )
	{
		point = go->GetPlacement()->GetPosition();
		point.pos.y += 1.0;
	}
}


void AIQuery :: GetLOSPoint( Go const * go, vector_3 & point ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	if( go->HasMind() && go->GetMind()->GetSightOriginHeight() > 0 )
	{
		point = go->GetPlacement()->GetWorldPosition();
		point.y += go->GetMind()->GetSightOriginHeight();
	}
	else
	{
		SiegePos tempPos;
		if( go->HasBody() && go->GetBody()->GetBoneTranslator().GetPosition( go, BoneTranslator::BODY_ANTERIOR, tempPos, vector_3::ZERO, false ) )
		{
			point = tempPos.WorldPos();
		}
		else
		{
			point = go->GetPlacement()->GetWorldPosition();
			point.y += 1.0;
		}
	}
}


bool AIQuery :: IsLosClear( SiegePos const & _from, SiegePos const & _to, bool hud ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: IsLosClear", SP_AI | SP_AI_QUERY );

	SiegePos to( _to );
	to.pos.y += 0.01f;

	bool clear = !gSiegeEngine.CheckForGlobalTerrainObstruction( _from, to );

#if !GP_RETAIL
	if( hud )
	{
		gWorld.DrawDebugSphere( _from, 0.2f, clear ? COLOR_WHITE : COLOR_YELLOW, "" );
		gWorld.DrawDebugSphere( _to,	0.2f, clear ? COLOR_WHITE : COLOR_YELLOW, "" );
		gWorld.DrawDebugLine( _from, _to, clear ? COLOR_WHITE : COLOR_YELLOW, "" );
	}
#endif

	return clear;
}


bool AIQuery :: IsLosClear( SiegePos const & _from, SiegePos const & _to ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return IsLosClear( _from, _to, false );
}


bool AIQuery :: IsLosClear( Go const * _from, Go const * _to ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: IsLosClear", SP_AI | SP_AI_QUERY );

	if( !_from || !_to )
	{
		gpwarning( "AIQuery :: IsLosClear - called with NULL parameter.\n" );
		return false;
	}

	SiegePos fromPos;
	GetLOSPoint( _from, fromPos );

	SiegePos toPos;
	GetLOSPoint( _to, toPos );

	bool clear = !gSiegeEngine.CheckForGlobalTerrainObstruction( fromPos, toPos );

	if( _from->IsHuded() )
	{
		gWorld.DrawDebugSphere( fromPos, 0.2f, clear ? COLOR_WHITE : COLOR_YELLOW, "" );
		gWorld.DrawDebugSphere( toPos,	0.2f, clear ? COLOR_WHITE : COLOR_YELLOW, "" );
		gWorld.DrawDebugLine( fromPos, toPos, clear ? COLOR_WHITE : COLOR_YELLOW, "" );
	}

	return clear;
}


bool AIQuery :: IsLosClear( Go const * _from, SiegePos const & toPos ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: IsLosClear", SP_AI | SP_AI_QUERY );

	if( !_from )
	{
		gpwarning( "AIQuery :: IsLosClear - called with NULL parameter.\n" );
		return false;
	}

	SiegePos fromPos;
	GetLOSPoint( _from, fromPos );

	bool clear = !gSiegeEngine.CheckForGlobalTerrainObstruction( fromPos, toPos );

	if( _from->IsHuded() )
	{
		gWorld.DrawDebugSphere( fromPos, 0.2f, clear ? COLOR_WHITE : COLOR_YELLOW, "" );
		gWorld.DrawDebugSphere( toPos,	0.2f, clear ? COLOR_WHITE : COLOR_YELLOW, "" );
		gWorld.DrawDebugLine( fromPos, toPos, clear ? COLOR_WHITE : COLOR_YELLOW, "" );
	}

	return clear;
}


bool AIQuery :: FindClearLosPoint( Go const * from, Go const * to, float _maxDist, float maxAngle, SiegePos & out )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	if( !from || !to )
	{
		gpwarning( "AIQuery :: FindClearLosPoint - called with NULL parameter.\n" );
		return false;
	}

	vector_3 losPoint;
	GetLOSPoint( from, losPoint );
	losPoint -= from->GetPlacement()->GetWorldPosition();
	float hugHeight = AbsoluteValue( losPoint.y );

	SiegePos toPos;
	GetLOSPoint( to, toPos );

	return FindClearLosPoint(	from->GetPlacement()->GetPosition(),
								toPos,
								_maxDist,
								maxAngle,
								from->HasBody() ? from->GetBody()->GetTerrainMovementPermissions() : siege::LF_IS_FLOOR,
								hugHeight,
								from->IsHuded(),
								out );
}


bool AIQuery :: FindClearLosPoint( Go const * from, SiegePos const & _to, float _maxDist, float maxAngle, SiegePos & out )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	SiegePos to( _to );
	to.pos.y += 0.1f;

	vector_3 losPoint;
	GetLOSPoint( from, losPoint );
	losPoint -= from->GetPlacement()->GetWorldPosition();
	float hugHeight = AbsoluteValue( losPoint.y );

	return FindClearLosPoint(	from->GetPlacement()->GetPosition(),
								to,
								_maxDist,
								maxAngle,
								from->HasBody() ? from->GetBody()->GetTerrainMovementPermissions() : siege::LF_IS_FLOOR,
								hugHeight,
								from->IsHuded(),
								out );
}


bool AIQuery :: FindClearLosPoint(	SiegePos const & from,
									SiegePos const & to,
									float _maxDist,
									float maxAngle,
									siege::eLogicalNodeFlags nodeFlags,
									float GroundHugHeight,
									bool hud,
									SiegePos & out )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	if( !gSiegeEngine.IsNodeInAnyFrustum( from.node ) )
	{
		gpwarningf(("AIQuery::FindClearLosPoint() From pos is not in the world\n"));
		return( false );
	}
	else if (!gSiegeEngine.IsNodeInAnyFrustum( to.node ) )
	{
		gpwarningf(("AIQuery::FindClearLosPoint() To pos is not in the world\n"));
		return( false );
	}

	float maxDist = min( _maxDist, Length( gSiegeEngine.GetDifferenceVector( from, to ) ) );

	if( Random( 100 ) > 50 )
	{
		maxAngle = -maxAngle;
	}

	if( FindClearLosPointOnLine( from, to, maxDist, maxAngle, nodeFlags, GroundHugHeight, hud, out ) )
	{
		return true;
	}
	else if( FindClearLosPointOnLine( from, to, maxDist, -maxAngle, nodeFlags, GroundHugHeight, hud, out ) )
	{
		return true;
	}
	else
	{
		return FindClearLosPointOnLine( from, to, maxDist, 0, nodeFlags, GroundHugHeight, hud, out );
	}
}


bool AIQuery :: FindClearLosPointOnLine(	SiegePos const & from,
											SiegePos const & to,
											float maxDist,
											float Angle,
											siege::eLogicalNodeFlags nodeFlags,
											float GroundHugHeight,
											bool hud,
											SiegePos & out )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: FindClearLosPointOnLine", SP_AI | SP_AI_QUERY );

	if( !gSiegeEngine.IsNodeInAnyFrustum( from.node ) )
	{
		gpwarningf(("AIQuery::FindClearLosPointOnLine() From pos is not in the world\n"));
		return( false );
	}
	else if (!gSiegeEngine.IsNodeInAnyFrustum( to.node ) )
	{
		gpwarningf(("AIQuery::FindClearLosPointOnLine() To pos is not in the world\n"));
		return( false );
	}

	vector_3 unitDir = Normalize( gSiegeEngine.GetDifferenceVector( from, to ) );
	unitDir.RotateY( Angle );

	return FindClearLosPointR(	from,
								to,
								unitDir,
								0,
								maxDist,
								maxDist+1,
								0,
								8,
								nodeFlags,
								GroundHugHeight,
								hud,
								out );
}


bool AIQuery :: FindClearLosPointR(	SiegePos const & from,
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
									SiegePos & out
									)
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: FindClearLosPointR", SP_AI | SP_AI_QUERY );

	gpassert( minDist < maxDist );

	if( recursionDepth == recursionLimit )
	{
		return (out.node != UNDEFINED_GUID) && IsAreaWalkable( nodeFlags, out, 3, 0.5 );
	}

	float closestDist = closestMatchDistance;

	// examine visibility at min
	bool failedAdjustment	= false;
	bool failedWalkable		= false;
	bool beginVisible		= false;

	SiegePos begin( from );
	begin.pos += unitDir * minDist;
	SiegePos adjustedBegin( begin );

	if( gSiegeEngine.AdjustPointToTerrain( adjustedBegin, nodeFlags, 2 ) )
	{
		if( GroundHugHeight > 0 )
		{
			SiegePos losBegin( adjustedBegin );
			losBegin.pos.y += GroundHugHeight;

			// check visibility
			if( IsLosClear( losBegin, to, hud ) )
			{
				if( IsAreaWalkable( nodeFlags, adjustedBegin, 3, 0.5 ) )
				{
					if( minDist < closestMatchDistance )
					{
						out = adjustedBegin;
						closestDist = minDist;
					}
					beginVisible = true;
				}
				else
				{
					failedWalkable = true;
				}
			}
		}
		else
		{
			// check visibility
			if( IsLosClear( begin, to, hud ) )
			{
				if( minDist < closestMatchDistance )
				{
					out = begin;
					closestDist = minDist;
				}
				beginVisible = true;
			}
		}
	}
	else
	{
		failedAdjustment = true;
	}

#if !GP_RETAIL
	if( hud )
	{
		if( failedAdjustment )
		{
			gWorld.DrawDebugSphere( begin, 0.21f, COLOR_RED, "" );
		}
		if( failedWalkable )
		{
			gWorld.DrawDebugSphere( adjustedBegin, 0.15f, COLOR_ORANGE, "" );
		}
		else
		{
			gWorld.DrawDebugSphere( adjustedBegin, 0.15f, COLOR_GREEN, "" );
		}
	}
#endif

	// examine visibility at max

	bool endVisible	= false;

	failedAdjustment	= false;
	failedWalkable		= false;

	SiegePos end( from );
	end.pos += unitDir * maxDist;
	SiegePos adjustedEnd( end );

	if( gSiegeEngine.AdjustPointToTerrain( adjustedEnd, nodeFlags, 2 ) )
	{
		if( GroundHugHeight > 0 )
		{
			SiegePos losEnd( adjustedEnd );
			losEnd.pos.y += GroundHugHeight;
			// check visibility
			if( IsLosClear( losEnd, to, hud ) )
			{
				if( IsAreaWalkable( nodeFlags, adjustedEnd, 3, 0.5 ) )
				{
					if( maxDist < closestMatchDistance )
					{
						out = adjustedEnd;
						closestDist = maxDist;
					}
					endVisible = true;
				}
				else
				{
					failedWalkable = true;
				}
			}
		}
		else
		{
			// check visibility
			if( IsLosClear( end, to, hud ) )
			{
				if( maxDist < closestMatchDistance )
				{
					out = end;
					closestDist = maxDist;
				}
				endVisible = true;
			}
		}
	}
	else
	{
		failedAdjustment = true;
	}

#if !GP_RETAIL
	if( hud )
	{
		if( failedAdjustment )
		{
			gWorld.DrawDebugSphere( end, 0.21f, COLOR_RED, "" );
		}
		if( failedWalkable )
		{
			gWorld.DrawDebugSphere( adjustedEnd, 0.15f, COLOR_ORANGE, "" );
		}
		else
		{
			gWorld.DrawDebugSphere( adjustedEnd, 0.15f, COLOR_GREEN, "" );
		}
	}
#endif

	// recurse

	float minmaxhalfdelta = (maxDist-minDist) * 0.5f;

	if( beginVisible )
	{
		return true;
	}
	else if( endVisible )
	{
		if( ( minmaxhalfdelta > 0.5f ) )
		{
			FindClearLosPointR(	from,
								to,
								unitDir,
								maxDist - minmaxhalfdelta,
								maxDist,
								closestDist,
								++recursionDepth,
								recursionLimit,
								nodeFlags,
								GroundHugHeight,
								hud,
								out );
		}
		return true;
	}
	else if( minDist+1 < maxDist-1 )
	{
		return FindClearLosPointR(	from,
									to,
									unitDir,
									minDist+1,
									maxDist-1,
									closestDist,
									++recursionDepth,
									recursionLimit,
									nodeFlags,
									GroundHugHeight,
									hud,
									out );
	}
	else
	{
		return false;
	}
}


void AIQuery :: GetRandomOrientation( Quat & q )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );

	q = Quat::IDENTITY;
	q.RotateY( -PI + Random( PI2 ) );
}


bool AIQuery :: IsAreaWalkable( siege::eLogicalNodeFlags nodeFlags, SiegePos const & center, DWORD numSamples, float radius )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: IsAreaWalkable", SP_AI | SP_AI_QUERY );

	float angle = 0;
	SiegePos tempPos;
	for( DWORD i = 0; i != numSamples; ++i )
	{
		tempPos = center;
		vector_3 dir( vector_3::NORTH*radius );
		dir.RotateY( angle );
		tempPos.pos += dir;
/*
		if( !gSiegeEngine.AdjustPointToTerrain( tempPos ) )
		{
#if !GP_RETAIL
			if( gWorldOptions.GetShowSpatialQueries() )
			{
				gWorld.DrawDebugPoint( tempPos, 0, COLOR_LIGHT_RED, "" );
			}
#endif
			return( false );
		}
*/
		if( !gSiegeEngine.IsPositionAllowed( tempPos, nodeFlags, 1.0f ) )
		{
#			if !GP_RETAIL
			if( gWorldOptions.GetShowSpatialQueries() )
			{
				gWorld.DrawDebugPoint( tempPos, 0, COLOR_RED, "" );
			}
#			endif
			return( false );
		}
		else
		{
/*
		if( gSiegeEngine.IsPositionBlocked( node, tempPos ) )
		{
#			if !GP_RETAIL
			if( gWorldOptions.GetShowSpatialQueries() )
			{
				gWorld.DrawDebugPoint( tempPos, 0, COLOR_RED, "" );
			}
#			endif
			return( false );
		}
*/

#			if !GP_RETAIL
			if( gWorldOptions.GetShowSpatialQueries() )
			{
				gWorld.DrawDebugPoint( tempPos, 0, COLOR_GREEN, "" );
			}
#			endif

			angle += (PI2)/float( numSamples );
		}
	}

#	if !GP_RETAIL
	if( gWorldOptions.GetShowSpatialQueries() )
	{
		gWorld.DrawDebugPoint( center, 0, COLOR_GREEN, "" );
	}
#	endif
	return( true );
}


bool AIQuery :: FindSpotRelativeToSource(	SiegePos const * source,
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
											bool bProjectBothSides ) // true if we should project the probability wedge on both sides
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: FindSpotRelativeToSource", SP_AI | SP_AI_QUERY );

	gpassert( source );
	gpassert( minDistance <= maxDistance );
	gpassert( minDegreesAngularDeviance <= maxDegreesAngularDeviance );
	gpassert( verticalDistanceConstraint > 0 );
	gpassert( maxDistance < 45 );	// $ sanity
	if( target )
	{
		gpassert( maxDegreesAngularDeviance < 180 );
	}

	if( !gSiegeEngine.IsNodeInAnyFrustum( source->node ) )
	{
		return false;
	}

	int retryCount = gWorldOptions.GetFindSpotRetryLimit();

	vector_3 direction;
	if( target && gSiegeEngine.IsNodeInAnyFrustum( target->node ) )
	{
		direction = GetDifferenceVector( *source, *target );
		direction.y = 0;
		direction *= awayFromTarget ? -1.0f : 1.0f;
		if( IsZero( direction ) )
		{
			direction = vector_3::NORTH;
		}
		direction.Normalize();
	}
	else
	{
		direction = vector_3::NORTH;
	}

#if !GP_RETAIL
	if( source && target && showHuds )
	{
		vector_3 debugDir = direction;
		float dir = minDegreesAngularDeviance + ( maxDegreesAngularDeviance - minDegreesAngularDeviance );
		debugDir.RotateY( dir * DEGREES_TO_RADIANS );

		gWorld.DrawDebugWedge( *source,
								maxDistance,
								debugDir,
								(maxDegreesAngularDeviance - minDegreesAngularDeviance) * DEGREES_TO_RADIANS,
								0x80ffffff,
								true );

		// only draw the second wedge if we have the symetry option turned on!
		if( bProjectBothSides )
		{
			debugDir.RotateY( -2.0f * dir * DEGREES_TO_RADIANS );

			gWorld.DrawDebugWedge( *source,
									maxDistance,
									debugDir,
									(maxDegreesAngularDeviance - minDegreesAngularDeviance) * DEGREES_TO_RADIANS,
									0x80ffffff,
									true );

		}
	}
#endif

	SiegePos testPos;

	bool bFirstTry = true;
	while( retryCount )
	{
		if( IsZero( direction ) )
		{
			direction = vector_3::NORTH;
		}

		float deviance;

		if( target )
		{
			deviance = minDegreesAngularDeviance + Random( maxDegreesAngularDeviance - minDegreesAngularDeviance );			
			if ( !bDropSame )
			{
				if( bProjectBothSides && Random( 100 ) < 50 )
				{
					deviance = 360.0f - deviance;
				}
			}
		}
		else
		{
			deviance = Random( 360.0f );
		}

		vector_3 offset = direction * ( minDistance + Random( maxDistance - minDistance ) );

		if ( !bFirstTry || !target || !bDropSame )
		{
			offset.RotateY( deviance * DEGREES_TO_RADIANS );			
		}

		bFirstTry = false;

		testPos = *source;
		testPos.pos += offset;

		if( gSiegeEngine.AdjustPointToTerrain( testPos, nodeFlags ) )
		{
			if( gSiegeEngine.IsNodeInAnyFrustum( testPos.node ) )
			{
				vector_3 diff = gSiegeEngine.GetDifferenceVector( *source, testPos );
				if( AbsoluteValue( diff.y ) < verticalDistanceConstraint )
				{
					if( IsAreaWalkable( nodeFlags, testPos, 4, pointBufferRadius ) )
					{
						pos = testPos;
						return( true );
					}					
				}
			}
		}
		--retryCount;
	}
	return( false );
}


bool AIQuery :: FindSpotRelativeToSource(	SiegePos const * source,
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
											bool bDropSame )
											
{

	return	FindSpotRelativeToSource(	source,
										target,
										nodeFlags,
										pointBufferRadius,
										awayFromTarget,
										minDistance,
										maxDistance,
										minDegreesAngularDeviance,
										maxDegreesAngularDeviance,
										verticalDistanceConstraint,
										showHuds,
										pos,
										bDropSame,
										true ); // true if we should project the probability wedge on both sides
}


bool AIQuery :: FindSpotRelativeToSource(	SiegePos const * source,
											float pointBufferRadius,
											float minDistance,
											float maxDistance,
											float verticalDistanceConstraint,
											bool showHuds,
											SiegePos & pos )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return FindSpotRelativeToSource(	source,
										NULL,
										siege::LF_HUMAN_PLAYER,
										pointBufferRadius,
										false,
										minDistance,
										maxDistance,
										0,
										0,
										verticalDistanceConstraint,
										showHuds,
										pos,
										false,
										true ); // bProjectBothSides
}


bool AIQuery :: FindSpotForDrop(	Go * dropper,
									Go * dropFor,
									Go * /*dropItem*/,
									float diagLength,									
									float maxDegreesAngularDeviance,
									float verticalDistanceConstraint,
									SiegePos & pos,
									bool bDropSame )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	gpassert( dropper && dropFor );
	return( FindSpotRelativeToSource(	dropper,
										dropFor,
										false,
										diagLength + gWorldOptions.GetDropItemsDistanceMin(),
										diagLength + gWorldOptions.GetDropItemsDistanceMax(),
										0,
										maxDegreesAngularDeviance,
										verticalDistanceConstraint,
										pos,
										bDropSame,
										true ) ); // bProjectBothSides
}

bool AIQuery :: FindSpotRelativeToSource(	Go * source,
											Go * target,
											bool awayFromTarget,
											float minDistance,
											float maxDistance,
											float minDegreesAngularDeviance,
											float maxDegreesAngularDeviance,
											float verticalDistanceConstraint,
											SiegePos & pos, 
											bool bDropSame )
{
	return FindSpotRelativeToSource(	source,
										target,
										awayFromTarget,
										minDistance,
										maxDistance,
										minDegreesAngularDeviance,
										maxDegreesAngularDeviance,
										verticalDistanceConstraint,
										pos, 
										bDropSame,
										true ); // bProjectBothSides
}

bool AIQuery :: FindSpotRelativeToSource(	Go * source,
											Go * target,
											bool awayFromTarget,
											float minDistance,
											float maxDistance,
											float minDegreesAngularDeviance,
											float maxDegreesAngularDeviance,
											float verticalDistanceConstraint,
											SiegePos & pos, 
											bool bDropSame,
											bool bProjectBothSides )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	gpassert( source );
#if !GP_RETAIL
	if( target )
	{
		gpassert( ( MakeInt( source->GetWorldFrustumMembership() ) & MakeInt( target->GetWorldFrustumMembership() ) ) != 0 );
		gpassert( maxDegreesAngularDeviance <= 180 );
	}
#endif
	gpassert( minDistance <= maxDistance );
	gpassert( minDegreesAngularDeviance <= maxDegreesAngularDeviance );
	gpassert( verticalDistanceConstraint > 0 );
	gpassert( maxDistance < 45 );	// $ sanity

	float const personalSpace = source->HasMind() ? source->GetMind()->GetPersonalSpaceRange() : source->GetAspect()->GetBoundingSphereRadius();
	float _minDistance = minDistance + ( personalSpace * 0.5f );
	float _maxDistance = maxDistance + ( personalSpace * 0.5f );

	bool result = FindSpotRelativeToSource(	&source->GetPlacement()->GetPosition(),
											target ? &target->GetPlacement()->GetPosition() : NULL,
											source->HasBody() ? source->GetBody()->GetTerrainMovementPermissions() : siege::LF_HUMAN_PLAYER,
											gWorldOptions.GetSpotWalkMaskRadius(),
											awayFromTarget,
											_minDistance,
											_maxDistance,
											minDegreesAngularDeviance,
											maxDegreesAngularDeviance,
											verticalDistanceConstraint,
											source->IsHuded(),
											pos,
											bDropSame,
											bProjectBothSides );

	return( result );
}


bool AIQuery :: FindSpotRelativeToSource(	Go * source,
											float minDistance,
											float maxDistance,
											float verticalDistanceConstraint,
											SiegePos & pos )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return FindSpotRelativeToSource( 	source,
										NULL,
										false,
										minDistance,
										maxDistance,
										0,
										0,
										verticalDistanceConstraint,
										pos,
										false,
										true ); // bProjectBothSides
}


bool AIQuery :: GetPositionsAroundNewCenter( GopColl const & GOs, SiegePos NewCenter, GONodePosList & PosList )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery :: GetPositionsAroundNewCenter", SP_AI | SP_AI_QUERY );

	GOVectorColl WorldPositionList;
	WorldPositionList.reserve( GOs.size() );

	// handle single-member list
	if( GOs.size() == 1 )
	{
		GONodePosList::value_type NewEntry( GOs.front(), NewCenter );
		PosList.push_back( NewEntry );
		return( true );
	}

	// handle multiple-member list
	vector_3 GeometricCenter = GetGeometricCenter( GOs, WorldPositionList );

	GOVectorColl::iterator i;

	// Get the node so we can do orientation correction
	SiegeNode* pNode = gSiegeEngine.IsNodeValid( NewCenter.node );
	if( !pNode )
	{
		return( false );
	}

	for( i = WorldPositionList.begin(); i != WorldPositionList.end(); ++i )
	{

		vector_3 FromCenterDelta = pNode->GetTransposeOrientation() * ((*i).second - GeometricCenter);

		// check if this point is valid, if not try to find a new point
		SiegePos NewSiegePosition = NewCenter;

		bool bFoundGoodPoint = false;

		// todo[bk]: this isn't the best approach, but will work for the time being, make better later
		while( Length2( FromCenterDelta ) > 0.15f ) {
			// calc new SiegePos
			NewSiegePosition.node	= NewCenter.node;
			NewSiegePosition.pos	= NewCenter.pos + FromCenterDelta;

			if( gSiegeEngine.AdjustPointToTerrain( NewSiegePosition ) ) {
				bFoundGoodPoint = true;
				break;
			}
			else {
				FromCenterDelta = FromCenterDelta * 0.8f;
			}
		}

		if( bFoundGoodPoint ) {
			GONodePosList::value_type NewEntry( (*i).first, NewSiegePosition );
			PosList.push_back( NewEntry );
		}
	}
	return( true );
}


static bool FindIntersectionAB( GoidColl & a, GoidColl & b, GoidColl & result, bool isIn )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	GPPROFILERSAMPLE( "AIQuery::FindIntersectionAB", SP_AI | SP_AI_QUERY );

// Check early-outs.

	// no B at all, forget it
	if ( b.empty() )
	{
		return ( false );
	}

	// no A at all, choose all or nothing
	if ( a.empty() )
	{
		if ( isIn )
		{
			// nothing
			return ( false );
		}
		else
		{
			// all
			result.insert( result.end(), b.begin(), b.end() );
			return ( true );
		}
	}

// Do full (not)intersection test.

	size_t oldCount = result.size();

	// see how big
	size_t allocSize = b.size_bytes();

	// large allocs shouldn't use the faster method that requires the stack
	// so we don't have an overflow
	if ( allocSize >= 4096 )
	{
		// $ IMPORTANT: if you change this code, update the 'else' below too!

		// clear all B
		GoidColl::iterator i;
		for ( i = b.begin() ; i != b.end() ; ++i )
		{
			GoHandle go( *i );
			if ( go )
			{
				go->SetVisit( false );
			}
		}

		// set all A
		for ( i = a.begin() ; i != a.end() ; ++i )
		{
			GoHandle go( *i );
			if ( go )
			{
				go->SetVisit( true );
			}
		}

		// now see which B have been set to what we want
		for ( i = b.begin() ; i != b.end() ; ++i )
		{
			GoHandle go( *i );
			if ( go && (go->IsVisited() == isIn) )
			{
				result.push_back( *i );
			}
		}
	}
	else
	{
		// $ IMPORTANT: if you change this code, update the 'if' above too!

		// cached Go*'s from B go here
		Go** btemp = (Go**)::_alloca( allocSize );
		Go** biter = btemp;

		// clear all B and cache Go*'s
		GoidColl::iterator i;
		for ( i = b.begin() ; i != b.end() ; ++i )
		{
			GoHandle go( *i );
			if ( go )
			{
				go->SetVisit( false );
				*biter = go;
				++biter;
			}
		}

		// set all A
		for ( i = a.begin() ; i != a.end() ; ++i )
		{
			GoHandle go( *i );
			if ( go )
			{
				go->SetVisit( true );
			}
		}

		// now see which B have been set to what we want
		Go** bend = biter;
		for ( biter = btemp ; biter != bend ; ++biter )
		{
			if ( (*biter)->IsVisited() == isIn )
			{
				result.push_back( (*biter)->GetGoid() );
			}
		}
	}

	return ( result.size() > oldCount );
}


bool AIQuery :: FindBNotInA( GoidColl & a, GoidColl & b, GoidColl & result )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( FindIntersectionAB( a, b, result, false ) );
}


bool AIQuery :: FindBInA( GoidColl & a, GoidColl & b, GoidColl & result )
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	return ( FindIntersectionAB( a, b, result, true ) );
}


float AIQuery :: GetAlterationSum( Go const * pClient, eAlteration Alteration ) const
{
	GPSTATS_GROUP( GROUP_AI_QUERY );
	if( !pClient->HasMagic() || !pClient->GetMagic()->HasEnchantments() )
	{
		return 0;
	}

	Go *pParent = NULL;

	if( pClient->GetParent( pParent ) )
	{
		if( !pParent->HasActor() )
		{
			if( !pParent->GetParent( pParent ) )
			{
				return 0;
			}
		}
	}
	Goid client( pParent->GetGoid() );

	const EnchantmentStorage &EStorage = pClient->GetMagic()->GetEnchantmentStorage();
	const EnchantmentVector  &enchantments = EStorage.GetEnchantments();
	EnchantmentVectorCI i = enchantments.begin(), i_end = enchantments.end();

	float AlterationSum = 0.0f;

	for(; i != i_end; ++i )
	{
		if( (*i)->GetAlterationType() == Alteration )
		{
			float duration = 0;
			float frequency= 0;
			float value	   = 0;
			FormulaHelper::EvaluateFormula( (*i)->GetDurationString(), NULL, duration, client, client, pClient->GetGoid() );
			FormulaHelper::EvaluateFormula( (*i)->GetFrequencyString(), NULL, frequency, client, client, pClient->GetGoid() );
			FormulaHelper::EvaluateFormula( (*i)->GetValueString(), NULL, value, client, client, pClient->GetGoid() );

			if( ( duration > 0.0) && ( frequency > 0.0) )
			{
				// calculate sum effect of this particulat alteration
				AlterationSum += float( value * duration / frequency  );
			}
			else
			{
				AlterationSum += float( value );
			}
		}
	}

	return AlterationSum;
}


AIQuery::MetaQueryDb::iterator AIQuery :: EraseMetaQuery( AIQuery::MetaQueryDb::iterator where )
{
	MetaQuery* metaQuery = &where->second;

	// remove all nodes this metaquery was interested in
	SiegeGuidColl::iterator i, ibegin = metaQuery->m_InterestNodes.begin(), iend = metaQuery->m_InterestNodes.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// find it
		InterestNodeDb::iterator found = m_InterestNodeDb.find( InterestNode( *i, metaQuery ) );
		gpassert( found != m_InterestNodeDb.end() );
		m_InterestNodeDb.erase( found );
	}

	// finally, delete the metaquery
	return ( m_MetaQueryDb.erase( where ) );
}


//////////////////////////////////////////////////////////////////////////////
#if !GP_RETAIL

void AIQuery :: DumpActorStats( Go const * go, ReportSys::Context * ctx )
{
	gpstring sName			( ::ToAnsi( go->GetCommon()->GetScreenName() ) );
	gpstring sClass			( "not set" );
	gpstring sAlignment		( "not set" );
	gpstring sRace			( "not set" );
	gpstring sLifeState		( "not set" );
	gpstring sRegionName	( gWorldMap.GetRegionName( gWorldMap.GetRegionByNode( go->GetPlacement()->GetPosition().node ) ) );

	float	MaxLife	=	0;
	float	CurLife	=	0;
	float	LRU		=	0;
	float	LRP		=	0;

	float	MaxMana	=	0;
	float	CurMana	=	0;
	float	MRU		=	0;
	float	MRP		=	0;

	float	XP_value=	0;

	float	Int		=	0;
	float	Str		=	0;
	float	Dex		=	0;
	float	Melee	=	0;
	float	Ranged	=	0;
	float	NMagic	=	0;
	float	CMagic	=	0;

	float	Sight_fov	=	0;
	float	Sight_range =	0;

	float	MeleeEngage	=	0;
	float	RangedEngage=	0;

	float	Base_damage =	0;
	float	Base_defense=	0;

	GoAspect const * pAspect = go->GetAspect();

	MaxLife		=	pAspect->GetMaxLife();
	CurLife		=	pAspect->GetCurrentLife();
	LRU			=	pAspect->GetLifeRecoveryUnit();
	LRP			=	pAspect->GetLifeRecoveryPeriod();
	sLifeState	=	ToString( pAspect->GetLifeState() );

	if( go->HasActor() )
	{
		GoActor	const * pActor = go->GetActor();

		XP_value	=	pAspect->GetExperienceValue();
		sAlignment	=	ToString( pActor->GetAlignment() );
		sRace		=	pActor->GetRace();

		MaxMana		=	pAspect->GetMaxMana();
		CurMana		=	pAspect->GetCurrentMana();
		MRU			=	pAspect->GetManaRecoveryUnit();
		MRP			=	pAspect->GetManaRecoveryPeriod();

		sClass		=	::ToAnsi( pActor->GetClass() );

		Int			=	pActor->GetSkillLevel( "intelligence"	);
		Str			=	pActor->GetSkillLevel( "strength"		);
		Dex			=	pActor->GetSkillLevel( "dexterity"		);
		Melee		=	pActor->GetSkillLevel( "Melee"			);
		Ranged		=	pActor->GetSkillLevel( "Ranged"			);
		NMagic		=	pActor->GetSkillLevel( "Nature magic"	);
		CMagic		=	pActor->GetSkillLevel( "Combat magic"	);
	}

	gpstring	sStatsString;

	sStatsString.append( "Screen Name:    " ); sStatsString.append( sName );			 			sStatsString.append( "\n" );
	sStatsString.appendf("Templ. Name:    %s\n",		go->GetTemplateName() );
	sStatsString.appendf("scid:           0x%08x\n",	go->GetScid() );
	sStatsString.appendf("map region:     %s\n", sRegionName.c_str() );

	sStatsString.append( "Class:          " ); sStatsString.append( sClass );						sStatsString.append( "\n" );
	sStatsString.append( "Alignment:      " ); sStatsString.append( sAlignment );					sStatsString.append( "\n" );
	sStatsString.append( "Race:           " ); sStatsString.append( sRace );						sStatsString.append( "\n" );
	sStatsString.append( "LifeState:      " ); sStatsString.append( sLifeState );					sStatsString.append( "\n" );
	sStatsString.append( "XP value:       " ); stringtool::Append( XP_value, sStatsString );		sStatsString.append( "\n" );

	sStatsString.append( "Current Life:   " ); stringtool::Append( CurLife, sStatsString );		sStatsString.append( "\n" );
	sStatsString.append( "Max Life:       " ); stringtool::Append( MaxLife, sStatsString );		sStatsString.append( "\n" );
	sStatsString.append( "LRU:            " ); stringtool::Append( LRU, sStatsString );			sStatsString.append( "\n" );
	sStatsString.append( "LRP:            " ); stringtool::Append( LRP, sStatsString );			sStatsString.append( "\n" );

	sStatsString.append( "Current Mana:   " ); stringtool::Append( CurMana, sStatsString );		sStatsString.append( "\n" );
	sStatsString.append( "Max Mana:       " ); stringtool::Append( MaxMana, sStatsString );		sStatsString.append( "\n" );
	sStatsString.append( "MRU:            " ); stringtool::Append( MRU, sStatsString );			sStatsString.append( "\n" );
	sStatsString.append( "MRP:            " ); stringtool::Append( MRP, sStatsString );			sStatsString.append( "\n" );

	sStatsString.append( "Strength:       " ); stringtool::Append( Str, sStatsString );			sStatsString.append( "\n" );
	sStatsString.append( "Intelligence:   " ); stringtool::Append( Int, sStatsString );			sStatsString.append( "\n" );
	sStatsString.append( "Dexterity:      " ); stringtool::Append( Dex, sStatsString );			sStatsString.append( "\n" );
	sStatsString.append( "Melee:          " ); stringtool::Append( Melee, sStatsString );			sStatsString.append( "\n" );
	sStatsString.append( "Ranged:         " ); stringtool::Append( Ranged, sStatsString );		sStatsString.append( "\n" );
	sStatsString.append( "Nature Magic:   " ); stringtool::Append( NMagic, sStatsString );		sStatsString.append( "\n" );
	sStatsString.append( "Combat Magic:   " ); stringtool::Append( CMagic, sStatsString );		sStatsString.append( "\n" );

	sStatsString.append( "Sight Fov:      " ); stringtool::Append( Sight_fov, sStatsString );		sStatsString.append( "\n" );
	sStatsString.append( "Sight range:    " ); stringtool::Append( Sight_range, sStatsString );	sStatsString.append( "\n" );
	sStatsString.append( "Melee engage:   " ); stringtool::Append( MeleeEngage, sStatsString );	sStatsString.append( "\n" );
	sStatsString.append( "Ranged engage:  " ); stringtool::Append( RangedEngage, sStatsString );	sStatsString.append( "\n" );

	sStatsString.append( "Base damage:  " ); stringtool::Append( Base_damage, sStatsString );	sStatsString.append( "\n" );
	sStatsString.append( "Base defense: " ); stringtool::Append( Base_defense, sStatsString );	sStatsString.append( "\n" );
	sStatsString.append( "Total defense:" ); stringtool::Append( gRules.GetTotalDefense( go->GetGoid() ), sStatsString ); sStatsString.append( "\n" );

	if( pAspect->GetIsInvincible() )
	{
		sStatsString.append( "*INVINCIBLE*\n" );
	}

	ctx->Output( sStatsString.c_str() );
}

#endif // !GP_RETAIL

#if AIQUERY_UBER_PARANOIA

AIQuery::GoMetaQueryDb::iterator AIQuery :: FindGoInCache( Go* go, MetaQuery* metaQuery )
{
	std::pair <GoMetaQueryDb::iterator, GoMetaQueryDb::iterator>
			rc = m_GoMetaQueryDb.equal_range( go );
	for ( ; rc.first != rc.second ; ++rc.first )
	{
		if ( rc.first->second == metaQuery )
		{
			return ( rc.first );
		}
	}

	return ( m_GoMetaQueryDb.end() );
}

bool AIQuery :: HasGoInCache( Go* go, MetaQuery* metaQuery )
{
	return ( FindGoInCache( go, metaQuery ) != m_GoMetaQueryDb.end() );
}

#endif // AIQUERY_UBER_PARANOIA
