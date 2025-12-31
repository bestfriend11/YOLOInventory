#pragma once
/*************************************************************************************
**
**								WorldPathFinder
**
**		This class is responsible for navigating through the LogicalNode collection
**		and returning paths throught the terrain.
**
**		Author:		James Loe
**		Date:		12/8/99
**
*************************************************************************************/
#ifndef _WORLD_PATHFINDER_
#define _WORLD_PATHFINDER_


struct SiegePos;
namespace siege
{
	class	SiegeLogicalNode;
};

#include "siege_pathfinder.h"

class WorldPathFinder : public Singleton <WorldPathFinder>
{
public:

	// Construction and destruction
	WorldPathFinder();
	~WorldPathFinder();

	// Find a path
	bool FindPath( siege::SiegeLogicalNode* pStartNode, SiegePos& startPos,
				   siege::SiegeLogicalNode* pEndNode, SiegePos& endPos,
				   siege::eLogicalNodeFlags flags, const float bufferRange,
				   std::list< SiegePos >& wayPoints );

private:

	// List of boxes
	siege::LeafBoxColl		m_LeafBoxColl;

	// Optimize the path
	void OptimizePath( std::list< SiegePos >& pathList, siege::eLogicalNodeFlags flags );

	// Verify an adjusted position by comparing it to the original
	bool VerifyAdjustedPosition( const SiegePos& orig, const SiegePos& adjusted, siege::eLogicalNodeFlags flags, bool bCheckBlocked = true );

};

#define gWorldPathFinder WorldPathFinder::GetSingleton()

#endif
