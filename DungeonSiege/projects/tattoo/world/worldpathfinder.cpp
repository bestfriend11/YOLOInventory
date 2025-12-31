/*************************************************************************************
**
**								WorldPathFinder
**
**		This class is responsible for navigating through the LogicalNode collection
**		and returning paths throught the terrain.
**
*************************************************************************************/

#include "precomp_world.h"

#include "gpcore.h"
#include "gpprofiler.h"

#include "vector_3.h"
#include "space_3.h"

#include "siege_engine.h"
#include "siege_pathfinder.h"
#include "worldpathfinder.h"

#include "RapiPrimitive.h"

using namespace siege;

const vector_3 mod( 0.02f, 0.05f, 0.02f );

// Construction
WorldPathFinder::WorldPathFinder()
{
}

// Destruction
WorldPathFinder::~WorldPathFinder()
{
}

// Find a path
bool WorldPathFinder::FindPath( SiegeLogicalNode* pStartNode, SiegePos& startPos,
							    SiegeLogicalNode* pEndNode, SiegePos& endPos, 
								eLogicalNodeFlags flags, const float bufferRange,
								std::list< SiegePos >& wayPoints )
{
	CHECK_SERVER_ONLY;

	GPSTATS_GROUP( GROUP_PATH_FIND );
	GPPROFILERSAMPLE( "WorldPathFinder::FindPath", SP_AI_MIND | SP_AI );

	gpassert( m_LeafBoxColl.empty() );
	if( !gSiegePathfinder.FindPath( pStartNode, startPos, pEndNode, endPos, flags, bufferRange, wayPoints, &m_LeafBoxColl ) ||
		wayPoints.empty() )
	{
		gpassert( m_LeafBoxColl.empty() );
		return false;
	}

	if( gWorldOptions.GetOptimizeMCP() )
	{
		OptimizePath( wayPoints, flags );

		// Check to make sure that our post optimized path is still valid
		if( gSiegeEngine.GetDifferenceVector( endPos, wayPoints.back() ).Length() > (bufferRange + 0.001f) )
		{
			wayPoints.clear();
		}
	}
	m_LeafBoxColl.clear();

	return( wayPoints.size() > 1 ? true : false );
}


// Construct the path
void WorldPathFinder::OptimizePath( std::list< SiegePos >& pathList, siege::eLogicalNodeFlags flags )
{
	GPSTATS_GROUP( GROUP_PATH_FIND );
	GPPROFILERSAMPLE( "WorldPathFinder::OptimizePath", SP_AI_MIND | SP_AI );

	std::list< SiegePos >			newpathList;
	stdx::fast_vector< SiegePos >	intermedList;
	vector_3 coord( DoNotInitialize );
	vector_3 ray_dir( DoNotInitialize );

	// Reserve size in our collections
	intermedList.reserve( pathList.size() );

	// Build the path
	vector_3 currentPos				= pathList.front().WorldPos();
	SiegePos previousPos			= pathList.front();

	DWORD boxStartIndex				= 1;
	DWORD boxIndex					= 2;

	// Get the beginning of the list and iterate to the second element (start point cannot be optimized)
	std::list< SiegePos >::iterator i = pathList.begin();
	newpathList.push_back( (*i) );
	++i;

	bool bNewPosition = false;
	for( ; i != pathList.end(); ++i, ++boxIndex )
	{
		bNewPosition = false;

		// Get new position and direction
		ray_dir	= (*i).WorldPos() - currentPos;

		// Check ray intersections with current box window
		for( LeafBoxColl::iterator b = (m_LeafBoxColl.begin() + boxStartIndex); b != (m_LeafBoxColl.begin() + boxIndex); ++b )
		{
			if( !RayIntersectsBox( (*b).min_box - mod, (*b).max_box + mod, currentPos, ray_dir, coord ) )
			{
				bNewPosition = true;
				break;
			}
		}

		if( bNewPosition )
		{
			vector_3 previousWorld	= previousPos.WorldPos();
			if( gWorldOptions.GetEliminateWaypoints() )
			{
				ray_dir				= vector_3::ZERO;
			}
			else
			{
				ray_dir				= previousWorld - currentPos;
			}

			// Match up the intermediate listing
			for( stdx::fast_vector< SiegePos >::iterator p = intermedList.begin(); p != intermedList.end(); ++p )
			{
				SiegePos nPos;

				// Check for zero direction, and use current position if necessary
				if( !ray_dir.IsZero() )
				{
					vector_3 closest	= ClosestPointOnLine( currentPos, ray_dir, (*p).WorldPos() );
					nPos.FromWorldPos( closest, (*p).node );
				}
				else
				{
					nPos.FromWorldPos( currentPos, (*p).node );
				}

				// Adjust point and verify location
				if( gSiegeEngine.AdjustPointToTerrain( nPos, flags ) && VerifyAdjustedPosition( (*p), nPos, flags ) )
				{
					newpathList.push_back( nPos );
				}
				else
				{
					nPos = (*p);
					if( gSiegeEngine.AdjustPointToTerrain( nPos, flags ) && VerifyAdjustedPosition( (*p), nPos, flags ) )
					{
						newpathList.push_back( nPos );
					}
				}
			}

			// Put new position on the list
			currentPos	= previousWorld;

			// Reset our start index
			boxIndex--;
			boxStartIndex = boxIndex;

			// Clear the intermediate listing
			intermedList.clear();

			// Put information about this waypoint into our two lists
			intermedList.push_back( (*i) );
		}
		else if( !gWorldOptions.GetEliminateWaypoints() )
		{
			intermedList.push_back( (*i) );
		}

		// Continue with path building
		previousPos		= (*i);
	}

	if( gWorldOptions.GetEliminateWaypoints() )
	{
		ray_dir				= vector_3::ZERO;
	}
	else
	{
		ray_dir				= pathList.back().WorldPos() - currentPos;
	}

	// Match up the intermediate listing
	for( stdx::fast_vector< SiegePos >::iterator p = intermedList.begin(); p != intermedList.end(); ++p )
	{
		SiegePos nPos;

		// Check for zero direction, and use current position if necessary
		if( !ray_dir.IsZero() )
		{
			vector_3 closest	= ClosestPointOnLine( currentPos, ray_dir, (*p).WorldPos() );
			nPos.FromWorldPos( closest, (*p).node );
		}
		else
		{
			nPos.FromWorldPos( currentPos, (*p).node );
		}

		// Adjust point and verify location
		if( gSiegeEngine.AdjustPointToTerrain( nPos, flags ) && VerifyAdjustedPosition( (*p), nPos, flags ) )
		{
			newpathList.push_back( nPos );
		}
		else
		{
			nPos = (*p);
			if( gSiegeEngine.AdjustPointToTerrain( nPos, flags ) && VerifyAdjustedPosition( (*p), nPos, flags ) )
			{
				newpathList.push_back( nPos );
			}
		}
	}

	if( boxStartIndex != pathList.size() )
	{
		SiegePos nPos	= pathList.back();
		if( gSiegeEngine.AdjustPointToTerrain( nPos, flags ) && VerifyAdjustedPosition( pathList.back(), nPos, flags, false ) )
		{
			newpathList.push_back( nPos );
		}
	}

	pathList = newpathList;
}

// Verify an adjusted position by comparing it to the original
bool WorldPathFinder::VerifyAdjustedPosition( const SiegePos& orig, const SiegePos& adjusted, siege::eLogicalNodeFlags flags, bool bCheckBlocked )
{
	// Check the y modification
	if( FABSF( gSiegeEngine.GetDifferenceVector( orig, adjusted ).y ) > 1.0f )
	{
		// Too much modification
		return false;
	}

	if( bCheckBlocked && gSiegeEngine.IsPositionBlocked( gSiegeEngine.GetLogicalNodeAtPosition( adjusted, flags ), adjusted ) )
	{
		// Blocked position
		return false;
	}

	return true;
}
