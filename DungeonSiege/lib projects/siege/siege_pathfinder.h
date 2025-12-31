#pragma once
#ifndef _SIEGE_PATHFINDER_
#define _SIEGE_PATHFINDER_

/**********************************************************************************************
**
**									SiegePathfinder
**
**		This class is responsible for interfacing with the Siege preprocessed data
**		and generating generic, unoptimized path information.  No guarantees are made
**		about the paths that are returned by this class, other than they should be a
**		good starting point for generating optimal path information.
**
**		Author:		James Loe
**		Date:		Dec. 11, 2000
**
**********************************************************************************************/

#include "siege_logical_mesh.h"


namespace siege
{
	struct LeafBox
	{
		vector_3	min_box;
		vector_3	max_box;
	};

	typedef stdx::fast_vector< SiegeLogicalNode* >	LogicalNodeColl;
	typedef stdx::fast_vector< LNODESEARCH >		SearchNodeColl;
	typedef stdx::fast_vector< LMESHSEARCHLEAF >	SearchLeafColl;
	typedef stdx::fast_vector< DWORD >				OpenIndexColl;
	typedef stdx::fast_vector< LeafBox >			LeafBoxColl;

	class SiegePathfinder : public Singleton< SiegePathfinder >
	{
		friend class SiegeEngine;

	public:

		// Construction and destruction
		SiegePathfinder();
		~SiegePathfinder();

		// Find a path
		bool							FindPath( SiegeLogicalNode* pStartNode, SiegePos& startPos,
												  SiegeLogicalNode* pEndNode, SiegePos& endPos,
												  eLogicalNodeFlags flags, const float bufferRange,
												  std::list< SiegePos >& wayPoints, LeafBoxColl* pWayBoxes );

		// Simple path query
		float							IsPathValid( SiegeLogicalNode* pStartNode, SiegePos& startPos,
													 SiegeLogicalNode* pEndNode, SiegePos& endPos,
													 eLogicalNodeFlags flags );

	private:

		// Finds the path of LogicalNodes needed to go between two separated LogicalNodes
		void							FindLogicalNodePath( SiegeLogicalNode* pStartNode, SiegeLogicalNode* pEndNode,
															 SiegePos& finalPos, eLogicalNodeFlags flags, const float bufferRange );

		// Version of the leaf finder that uses the entire node list to constrain and A*/Djikstra search
		// algorithm.
		std::list< SiegePos >			FindLogicalLeafPath( SiegeLogicalNode* pStartNode, SiegePos& startPos,
															 SiegeLogicalNode* pFinalNode, SiegePos& finalPos,
															 const float bufferRange, LeafBoxColl* pWayBoxes );

		// Calculate the cost between two WorldNodes
		const float						CalculateNodeCost( SiegeLogicalNode* pStartNode, SiegePos& finalPos );
		const float						CalculateNodeCost( SiegeLogicalNode* pStartNode, SiegeLogicalNode* pEndNode );

		// Optimizes the node path by removing wasted step and loops
		void							OptimizeNodePath();

		// Get a leaf from a position
		unsigned short					GetLeafAtPosition( SiegeLogicalNode* pNode, const vector_3& pos );

		// Construct the path
		void							ConstructPath( LMESHSEARCHLEAF* pFinalSearchLeaf, std::list< SiegePos >& pathList,
													   LeafBoxColl* pBoxList, SiegePos& startPos, SiegePos& finalPos );


		// List of logical nodes to be used for a path
		LogicalNodeColl					m_PathList;
		LogicalNodeColl					m_ExtraPathList;

		// Unsorted list of search information
		SearchNodeColl					m_SearchNodeColl;
		SearchLeafColl					m_SearchLeafColl;

		// Sorted list of search leaf indices
		OpenIndexColl					m_OpenSearchColl;

		// Maximum boundaries
		int								m_maxNodepathSize;
		int								m_maxLeafpathSize;

	};
}

#define gSiegePathfinder siege::SiegePathfinder::GetSingleton()

#endif