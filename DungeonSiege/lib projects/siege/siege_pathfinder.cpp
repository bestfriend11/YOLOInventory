/**********************************************************************************************
**
**									SiegePathfinder
**
**		see .h file for complete details
**
**********************************************************************************************/

#include "precomp_siege.h"
#include <queue>
#include <functional>

#include "gpcore.h"

#include "vector_3.h"
#include "space_3.h"

#include "siege_engine.h"
#include "siege_node.h"
#include "siege_logical_node.h"
#include "siege_logical_mesh.h"
#include "siege_pathfinder.h"

#include "RapiPrimitive.h"

using namespace siege;

#define MAX_NODEPATH_SIZE		100
#define MAX_LEAFPATH_SIZE		2000

#define MAX_VALID_NODE_SIZE		3
#define MAX_VALID_LEAF_SIZE		50

const vector_3 mod( 0.01f, 0.01f, 0.01f );


// Construction
SiegePathfinder::SiegePathfinder()
{

}

// Destruction
SiegePathfinder::~SiegePathfinder()
{

}

// Find a path
bool SiegePathfinder::FindPath( SiegeLogicalNode* pStartNode, SiegePos& startPos,
							    SiegeLogicalNode* pEndNode, SiegePos& endPos, 
								eLogicalNodeFlags flags, const float bufferRange,
								std::list< SiegePos >& wayPoints, LeafBoxColl* pWayBoxes )
{
	// Set bounds high
	m_maxNodepathSize	= MAX_NODEPATH_SIZE;
	m_maxLeafpathSize	= MAX_LEAFPATH_SIZE;

	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetLogicalCritical() );
		
	FindLogicalNodePath( pStartNode, pEndNode, endPos, flags, bufferRange );
	if( !m_PathList.size() )
	{
		return false;
	}

	OptimizeNodePath();

	// Go through the nodes and set them active for search
	for( LogicalNodeColl::iterator i = m_PathList.begin(); i != m_PathList.end(); ++i )
	{
		(*i)->SetActivePath( true );
	}

	// Go through the neighboring logical nodes of my path and if any neighbors are bordered by 2 or more
	// logicalnodes in the existing path, add them to the extra path list.
	for( i = m_PathList.begin(); i != m_PathList.end(); ++i )
	{
		LNODECONNECT* pConnections	= (*i)->GetNodeConnectionInfo();
		for( unsigned int c = 0; c < (*i)->GetNumNodeConnections(); ++c )
		{
			SiegeNode* pNode	= gSiegeEngine.IsNodeValid( pConnections[ c ].m_farSiegeNode );
			if( pNode )
			{
				SiegeLogicalNode* pLogicalNode	= pNode->GetLogicalNodes()[ pConnections[ c ].m_pCollection->m_farid ];

				if( !pLogicalNode->GetActivePath() && pLogicalNode->AreFlagsAllowable( flags ) )
				{
					pLogicalNode->SetActivePath( true );
					m_ExtraPathList.push_back( pLogicalNode );
				}
			}
		}
	}

	wayPoints = FindLogicalLeafPath( pStartNode, startPos, pEndNode, endPos, bufferRange, pWayBoxes );

	// Go through the nodes and disable them for search
	for( i = m_PathList.begin(); i != m_PathList.end(); ++i )
	{
		(*i)->SetActivePath( false );
		(*i)->m_searchLeafMap.clear();
	}
	for( i = m_ExtraPathList.begin(); i != m_ExtraPathList.end(); ++i )
	{
		(*i)->SetActivePath( false );
		(*i)->m_searchLeafMap.clear();
	}

#if 0

	// Normally, I would return the path to the caller at this point.  For testing purposes, I will
	// continue and draw the path.
	for( i = m_PathList.begin(); i != m_PathList.end(); ++i )
	{
		gSiegeEngine.Renderer().SetWorldMatrix( (*i)->GetSiegeNode()->GetCurrentOrientation(), (*i)->GetSiegeNode()->GetCurrentCenter() );
		RP_DrawBox( gSiegeEngine.Renderer(), (*i)->GetLogicalMesh()->GetMinimumBounds(), (*i)->GetLogicalMesh()->GetMaximumBounds(), 0xFF00FFFF );
	}

	std::vector< vector_3 > CenterList;
	for( std::list< SiegePos >::iterator j = wayPoints.begin(); j != wayPoints.end(); ++j )
	{
		// Get the SiegeNode that we need for space conversion
		SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( (*j).node ));
		SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );

		CenterList.push_back( node.NodeToWorldSpace( (*j).pos ) + vector_3( 0.0f, 0.5f, 0.0f ) );
	}

	if( CenterList.size() )
	{
		unsigned int numlines = CenterList.size()-1;
		for( unsigned int l = 0; l < numlines; ++l )
		{
			if( l == 0 || l == (numlines-1) )
			{
				gSiegeEngine.GetWorldSpaceLines().DrawLine(	CenterList[ l ], CenterList[ l+1 ], 0xffff00ff );
			}
			else
			{
				gSiegeEngine.GetWorldSpaceLines().DrawLine( CenterList[ l ], CenterList[ l+1 ], 0xffffffff );
			}
		}
	}

#endif

	// Clear logical node list
	m_PathList.clear();
	m_ExtraPathList.clear();

	return( wayPoints.size() > 1 ? true : false );
}


// Simple path query
float SiegePathfinder::IsPathValid( SiegeLogicalNode* pStartNode, SiegePos& startPos,
								    SiegeLogicalNode* pEndNode, SiegePos& endPos,
									eLogicalNodeFlags flags )
{
	// Set bounds low
	m_maxNodepathSize	= MAX_VALID_NODE_SIZE;
	m_maxLeafpathSize	= MAX_VALID_LEAF_SIZE;

	// Find a path
	std::list< SiegePos > wayPoints;
	FindPath( pStartNode, startPos, pEndNode, endPos, flags, 0.0f, wayPoints, NULL );

	if( wayPoints.size() > 1 )
	{
		float totalDist	= 0.0f;
		for( std::list< SiegePos >::iterator i = wayPoints.begin(); i != wayPoints.end(); ++i )
		{
			std::list< SiegePos >::iterator j = i;
			++j;
			if( j != wayPoints.end() )
			{
				totalDist	+= Length( (*j).WorldPos() - (*i).WorldPos() );
			}
		}
		return totalDist;
	}
	else
	{
		return -1.0f;
	}
}


// Finds the path of LogicalNodes needed to go between two separated LogicalNodes
void SiegePathfinder::FindLogicalNodePath( SiegeLogicalNode* pStartNode, SiegeLogicalNode* pEndNode, SiegePos& finalPos, eLogicalNodeFlags flags, const float bufferRange )
{
	// Did the node path find complete successfully or did it fail?
	bool bPathSuccess				= false;
	DWORD closeNode					= 0xFFFFFFFF;

	// Visit counter
	int currentNodeCount			= 0;
	SiegeNode* pSiegeNode			= NULL;

	// Setup the starting node and push it onto our Open list
	gpassert( m_SearchNodeColl.empty() );
	LNODESEARCH* pSearchNode		= &*m_SearchNodeColl.push_back();

	pSearchNode->m_pLogicalNode		= pStartNode;
	pSearchNode->m_CostToStart		= 0.0f;
	pSearchNode->m_CostToFinish		= CalculateNodeCost( pStartNode, finalPos );
	pSearchNode->m_CombinedCost		= pSearchNode->m_CostToStart + pSearchNode->m_CostToFinish;
	pSearchNode->m_Parent			= 0xFFFFFFFF;
	pSearchNode->m_bInOpen			= true;
	pStartNode->SetPathSearchIndex( 0 );

	gpassert( m_OpenSearchColl.empty() );
	m_OpenSearchColl.push_back( 0 );
	for( ; ; )
	{
		// Get the node from the top of the stack
		float bestCombinedCost					= FLOAT_MAX;
		OpenIndexColl::iterator bestSearchNode	= m_OpenSearchColl.end();
		for( OpenIndexColl::iterator sli = m_OpenSearchColl.begin(); sli != m_OpenSearchColl.end(); ++sli )
		{
			pSearchNode	= &m_SearchNodeColl[ (*sli) ];
			if( pSearchNode->m_bInOpen && (pSearchNode->m_CombinedCost < bestCombinedCost) )
			{
				bestSearchNode		= sli;
				bestCombinedCost	= pSearchNode->m_CombinedCost;
			}
		}

		if( bestSearchNode == m_OpenSearchColl.end() )
		{
			break;
		}

		// Get the information about the best search node
		DWORD searchNodeIndex		= (*bestSearchNode);
		pSearchNode					= &m_SearchNodeColl[ searchNodeIndex ];

		// Get the node from the top of the stack
		SiegeLogicalNode* pNode		= pSearchNode->m_pLogicalNode;

		// See if we have found our goal
		if( pNode == pEndNode )
		{
			// Build the path
			m_PathList.push_back( pSearchNode->m_pLogicalNode );

			LNODESEARCH* pBuildNode = pSearchNode;
			while( pBuildNode->m_Parent != -1 )
			{
				pBuildNode	= &m_SearchNodeColl[ pBuildNode->m_Parent ];
				m_PathList.push_back( pBuildNode->m_pLogicalNode );
			}

			bPathSuccess = true;
			break;
		}

		// Go through the connected neighbors of this LogicalNode
		LNODECONNECT* pConnectionInfo	= pNode->GetNodeConnectionInfo();
		for( unsigned int i = 0; i < pNode->GetNumNodeConnections(); ++i )
		{
			// Get this connection
			LNODECONNECT* pNodalConnection		= &pConnectionInfo[ i ];

			// Get the node for this connection
			if( pNodalConnection->m_farSiegeNode == pNode->GetSiegeNode()->GetGUID() )
			{
				pSiegeNode						= pNode->GetSiegeNode();
			}
			else if( !pSiegeNode || (pSiegeNode->GetGUID() != pNodalConnection->m_farSiegeNode) )
			{
				pSiegeNode						= gSiegeEngine.IsNodeValid( pNodalConnection->m_farSiegeNode );
			}

			if( pSiegeNode && pSiegeNode->HasValidSpace() )
			{
				SiegeLogicalNode* pNeighbor		= pSiegeNode->GetLogicalNodes()[ pNodalConnection->m_pCollection->m_farid ];
				gpassert( pNeighbor->GetID()	== pNodalConnection->m_pCollection->m_farid );

				// See if this neighbor matches search criteria
				if( pNeighbor->AreFlagsAllowable( flags ) )
				{
					// Calculate new cost to this node
					LNODESEARCH* pCurrentNode	= NULL;
					float newcostToStart		= pSearchNode->m_CostToStart + CalculateNodeCost( pNeighbor, pNode );

					// Get the search node for this worldnode
					DWORD searchIndex			= pNeighbor->GetPathSearchIndex();
					if( searchIndex == -1 )
					{
						searchIndex						= m_SearchNodeColl.size();
						pCurrentNode					= &*m_SearchNodeColl.push_back();
						pSearchNode						= &m_SearchNodeColl[ searchNodeIndex ];
						pCurrentNode->m_pLogicalNode	= pNeighbor;
						pCurrentNode->m_CostToFinish	= CalculateNodeCost( pNeighbor, finalPos );
						pNeighbor->SetPathSearchIndex( searchIndex );

						if( (pCurrentNode->m_CostToFinish <= bufferRange) &&
							( (closeNode == -1) || (pCurrentNode->m_CostToFinish < m_SearchNodeColl[ closeNode ].m_CostToFinish) ) )
						{
							closeNode				= searchIndex;
						}

						// Put this node onto the open list
						m_OpenSearchColl.push_back( searchIndex );
					}
					else
					{
						pCurrentNode				= &m_SearchNodeColl[ searchIndex ];
						if( newcostToStart >= pCurrentNode->m_CostToStart )
						{
							continue;
						}
					}

					pCurrentNode->m_CostToStart		= newcostToStart;
					pCurrentNode->m_CombinedCost	= pCurrentNode->m_CostToStart + pCurrentNode->m_CostToFinish;
					pCurrentNode->m_Parent			= searchNodeIndex;
					pCurrentNode->m_bInOpen			= true;
				}
			}
		}

		// Tell node that it is no longer on the open list
		pSearchNode->m_bInOpen	= false;

		if( ++currentNodeCount > m_maxNodepathSize )
		{
			break;
		}
	}

	if( !bPathSuccess )
	{
		if( closeNode != -1 )
		{
			// Build the path
			LNODESEARCH* pBuildNode	= &m_SearchNodeColl[ closeNode ];
			m_PathList.push_back( pBuildNode->m_pLogicalNode );

			while( pBuildNode->m_Parent != -1 )
			{
				pBuildNode	= &m_SearchNodeColl[ pBuildNode->m_Parent ];
				m_PathList.push_back( pBuildNode->m_pLogicalNode );
			}

			// Indicate success
			bPathSuccess = true;
		}
		else
		{
			m_PathList.clear();
		}
	}

	// Reset the search indices
	for( OpenIndexColl::iterator sli = m_OpenSearchColl.begin(); sli != m_OpenSearchColl.end(); ++sli )
	{
		m_SearchNodeColl[ (*sli) ].m_pLogicalNode->SetPathSearchIndex( 0xFFFFFFFF );
	}

	// Clear collections
	m_SearchNodeColl.clear();
	m_OpenSearchColl.clear();
}

std::list< SiegePos > SiegePathfinder::FindLogicalLeafPath( SiegeLogicalNode* pStartNode, SiegePos& startPos,
														    SiegeLogicalNode* pFinalNode, SiegePos& finalPos,
															const float bufferRange, LeafBoxColl* pWayBoxes )
{
	bool bPathSuccess					= false;
	int currentLeafCount				= 0;
	int pClosestSearchLeafIndex			= -1;
	std::pair< LMESHSEARCHLEAFMAP::iterator, bool > sLeafMapPair;

	// Setup the path list
	std::list< SiegePos >			PathList;

	if( !m_PathList.front()->GetLogicalMesh() ||
		!m_PathList.back()->GetLogicalMesh() )
	{
		return PathList;
	}

	// Get the starting leaf and put it on our lists
	LMESHLEAFINFO* pStartLeaf			= &pStartNode->GetLogicalMesh()->GetLeafConnectionInfo()[ GetLeafAtPosition( pStartNode, startPos.pos ) ];

	// Get the final position
	LMESHLEAFINFO* pFinalLeaf			= &pFinalNode->GetLogicalMesh()->GetLeafConnectionInfo()[ GetLeafAtPosition( pFinalNode, finalPos.pos ) ];

	// Setup the starting leaf and push it onto our Open list
	gpassert( m_SearchLeafColl.empty() );

	LMESHSEARCHLEAF* pStartSearch		= &*m_SearchLeafColl.push_back();
	pStartSearch->pLogicalNode			= pStartNode;
	pStartSearch->pLeafInfo				= pStartLeaf;
	pStartSearch->costToStart			= Length( pStartLeaf->center - startPos.pos );
	pStartSearch->costToFinish			= Length( gSiegeEngine.GetDifferenceVector( SiegePos( pStartLeaf->center, startPos.node ), finalPos ) );
	pStartSearch->combinedCost			= pStartSearch->costToStart + pStartSearch->costToFinish;

	pStartSearch->parent				= 0xFFFFFFFF;
	pStartSearch->bInOpen				= true;

	gpassert( m_OpenSearchColl.empty() );
	m_OpenSearchColl.push_back( 0 );
	DWORD searchLeafCollSize			= 1;
	LMESHSEARCHLEAF* pSearchLeaf		= NULL;

	while( !m_OpenSearchColl.empty() )
	{
		// Get the node from the top of the stack
		float bestCombinedCost					= FLOAT_MAX;
		OpenIndexColl::iterator bestSearchLeaf	= m_OpenSearchColl.end();
		for( OpenIndexColl::iterator sli = m_OpenSearchColl.begin(); sli != m_OpenSearchColl.end(); ++sli )
		{
			pSearchLeaf	= &m_SearchLeafColl[ (*sli) ];
			if( pSearchLeaf->combinedCost < bestCombinedCost )
			{
				bestSearchLeaf		= sli;
				bestCombinedCost	= pSearchLeaf->combinedCost;
			}
		}

		DWORD searchLeafIndex		= (*bestSearchLeaf);
		m_OpenSearchColl.erase( bestSearchLeaf );

		// Get the information about the best search leaf
		pSearchLeaf					= &m_SearchLeafColl[ searchLeafIndex ];

		// See if we have found our goal
		if( pSearchLeaf->pLeafInfo == pFinalLeaf && pSearchLeaf->pLogicalNode == pFinalNode )
		{
			if( pFinalNode->IsLeafBlocked( pFinalLeaf->id ) )
			{
				if( pSearchLeaf->costToFinish > bufferRange  )
				{
					break;
				}
			}

			// Build the path
			ConstructPath( pSearchLeaf, PathList, pWayBoxes, startPos, finalPos );

			// Indicate success
			bPathSuccess = true;
			break;
		}

		unsigned int j;

		// Go through the connected local neighbors of this leaf
		SiegeLogicalNode* pLogicalNode	= pSearchLeaf->pLogicalNode;
		vector_3 finalOffset( DoNotInitialize );
		bool bFinalOffsetValid			= false;

		for( j = 0; j < pSearchLeaf->pLeafInfo->numLocalConnections; ++j )
		{
			// Get the neighbor leaf
			LMESHLEAFINFO *pNeighbor		= pSearchLeaf->pLeafInfo->localConnections[ j ];
			
			if( pLogicalNode->IsLeafBlocked( pNeighbor->id ) )
			{
				if( !((pNeighbor == pFinalLeaf) && (pLogicalNode == pFinalNode)) )
				{
					continue;
				}
			}
			
			// Calculate new cost to this leaf
			float newcostToStart			= pSearchLeaf->costToStart + Length( pNeighbor->center - pSearchLeaf->pLeafInfo->center );

			// Find the search leaf
			sLeafMapPair					= pLogicalNode->m_searchLeafMap.find_insert( pNeighbor );

			if( !sLeafMapPair.second )
			{
				LMESHSEARCHLEAF* pnSearch	= &m_SearchLeafColl[ (*sLeafMapPair.first).second ];
				if( pnSearch->costToStart > newcostToStart )
				{
					pnSearch->costToStart	= newcostToStart;
					pnSearch->parent		= searchLeafIndex;
					pnSearch->combinedCost	= pnSearch->costToStart + pnSearch->costToFinish;

					// Add to open list if it is not there
					if( !pnSearch->bInOpen )
					{
						m_OpenSearchColl.push_back( (*sLeafMapPair.first).second );
						pnSearch->bInOpen	= true;
					}
				}
			}
			else
			{
				if( !bFinalOffsetValid )
				{
					finalOffset				= -gSiegeEngine.GetDifferenceVector( SiegePos( vector_3::ZERO, pLogicalNode->GetSiegeNode()->GetGUID() ), finalPos );
					bFinalOffsetValid		= true;
				}

				LMESHSEARCHLEAF* pnSearch	= &*m_SearchLeafColl.push_back();
				pSearchLeaf					= &m_SearchLeafColl[ searchLeafIndex ];

				// Fill in new search leaf information
				pnSearch->pLogicalNode		= pLogicalNode;
				pnSearch->pLeafInfo			= pNeighbor;
				pnSearch->costToFinish		= Length( finalOffset + pNeighbor->center );
				pnSearch->costToStart		= newcostToStart;
				pnSearch->combinedCost		= pnSearch->costToStart + pnSearch->costToFinish;
				pnSearch->parent			= searchLeafIndex;

				if( pnSearch->costToFinish < bufferRange )
				{
					if( (pClosestSearchLeafIndex == -1) ||
						(pnSearch->costToFinish < m_SearchLeafColl[ pClosestSearchLeafIndex ].costToFinish) )
					{
						pClosestSearchLeafIndex	= searchLeafCollSize;
					}
				}

				// Put new search leaf into leafmapping
				pnSearch->bInOpen			= true;

				pLogicalNode->m_searchLeafMap.unsorted_insert( sLeafMapPair.first, std::pair< LMESHLEAFINFO*, DWORD >( pNeighbor, searchLeafCollSize ) );
				m_OpenSearchColl.push_back( searchLeafCollSize );
				++searchLeafCollSize;
			}
		}

		// Setup search offset for space comparisons
		SiegePos searchPos( pSearchLeaf->pLeafInfo->center, pLogicalNode->GetSiegeNode()->GetGUID() );
		vector_3 searchOffset( DoNotInitialize );
		bool bSearchOffsetValid	= false;
		SiegeNode* pNode		= NULL;

		// Go through the connected nodal neighbors of this leaf
		for( j = 0; j < pLogicalNode->GetNumNodeConnections(); ++j )
		{
			// Get this connection
			LNODECONNECT* pNodalConnection	= &pLogicalNode->GetNodeConnectionInfo()[ j ];

			// Get the node for this connection
			if( pNodalConnection->m_farSiegeNode == searchPos.node )
			{
				pNode						= pLogicalNode->GetSiegeNode();
			}
			else if( !pNode || (pNode->GetGUID() != pNodalConnection->m_farSiegeNode) )
			{
				pNode						= gSiegeEngine.IsNodeValid( pNodalConnection->m_farSiegeNode );
			}
			if( !pNode )
			{
				continue;
			}

			SiegeLogicalNode* pFarLogicalNode	= pNode->GetLogicalNodes()[ pNodalConnection->m_pCollection->m_farid ];
			gpassert( pFarLogicalNode != pLogicalNode );

			if( pFarLogicalNode->GetActivePath() )
			{
				// Reset our space notification
				bFinalOffsetValid			= false;
				bSearchOffsetValid			= false;

				for( unsigned int n = 0; n < pNodalConnection->m_pCollection->m_numNodalLeafConnections; ++n )
				{
					if( pNodalConnection->m_pCollection->m_pNodalLeafConnections[n].local_id == pSearchLeaf->pLeafInfo->id )
					{
						LMESHLEAFINFO* pNeighbor		= &pFarLogicalNode->GetLogicalMesh()->GetLeafConnectionInfo()[ pNodalConnection->m_pCollection->m_pNodalLeafConnections[n].far_id ];

						// See if we have calculated our searchOffset yet
						if( !bSearchOffsetValid )
						{
							searchOffset		= -gSiegeEngine.GetDifferenceVector( SiegePos( vector_3::ZERO, pNode->GetGUID() ), searchPos );
							bSearchOffsetValid	= true;
						}

						// Check to see if this leaf is blocked
						if( pFarLogicalNode->IsLeafBlocked( pNeighbor->id ) )
						{
							if( !((pNeighbor == pFinalLeaf) && (pLogicalNode == pFinalNode)) )
							{
								continue;
							}
						}

						gpassert( pNeighbor->id == pNodalConnection->m_pCollection->m_pNodalLeafConnections[n].far_id );
						gpassert( pFarLogicalNode->GetSiegeNode()->GetGUID() == pNode->GetGUID() );

						float newcostToStart			= pSearchLeaf->costToStart + Length( searchOffset + pNeighbor->center );

						// Find the search leaf
						sLeafMapPair					= pFarLogicalNode->m_searchLeafMap.find_insert( pNeighbor );

						if( !sLeafMapPair.second )
						{
							LMESHSEARCHLEAF* pnSearch	= &m_SearchLeafColl[ (*sLeafMapPair.first).second ];
							if( pnSearch->costToStart > newcostToStart )
							{
								pnSearch->costToStart	= newcostToStart;
								pnSearch->parent		= searchLeafIndex;
								pnSearch->combinedCost	= pnSearch->costToStart + pnSearch->costToFinish;

								// Add to open list if it is not there
								if( !pnSearch->bInOpen )
								{
									m_OpenSearchColl.push_back( (*sLeafMapPair.first).second );
									pnSearch->bInOpen	= true;
								}
							}
						}
						else
						{
							if( !bFinalOffsetValid )
							{
								finalOffset				= -gSiegeEngine.GetDifferenceVector( SiegePos( vector_3::ZERO, pNode->GetGUID() ), finalPos );
								bFinalOffsetValid		= true;
							}

							LMESHSEARCHLEAF* pnSearch	= &*m_SearchLeafColl.push_back();
							pSearchLeaf					= &m_SearchLeafColl[ searchLeafIndex ];

							// Fill in new search leaf information
							pnSearch->pLogicalNode		= pFarLogicalNode;
							pnSearch->pLeafInfo			= pNeighbor;
							pnSearch->costToFinish		= Length( finalOffset + pNeighbor->center );
							pnSearch->costToStart		= newcostToStart;
							pnSearch->combinedCost		= pnSearch->costToStart + pnSearch->costToFinish;
							pnSearch->parent			= searchLeafIndex;

							if( pnSearch->costToFinish < bufferRange )
							{
								if( (pClosestSearchLeafIndex == -1) ||
									(pnSearch->costToFinish < m_SearchLeafColl[ pClosestSearchLeafIndex ].costToFinish) )
								{
									pClosestSearchLeafIndex	= searchLeafCollSize;
								}
							}

							// Put new search leaf into leafmapping
							pnSearch->bInOpen			= true;

							pFarLogicalNode->m_searchLeafMap.unsorted_insert( sLeafMapPair.first, std::pair< LMESHLEAFINFO*, DWORD >( pNeighbor, searchLeafCollSize ) );
							m_OpenSearchColl.push_back( searchLeafCollSize );
							++searchLeafCollSize;
						}
					}
				}
			}
		}

		// Put this node on our closed list
		pSearchLeaf->bInOpen	= false;
		if( ++currentLeafCount > m_maxLeafpathSize )
		{
			break;
		}
	}

	if( !bPathSuccess )
	{
		if( pClosestSearchLeafIndex != -1 )
		{
			// Build the path
			SiegePos closestFinalPos( m_SearchLeafColl[ pClosestSearchLeafIndex ].pLeafInfo->center,
									  m_SearchLeafColl[ pClosestSearchLeafIndex ].pLogicalNode->GetSiegeNode()->GetGUID() );
			ConstructPath( &m_SearchLeafColl[ pClosestSearchLeafIndex ], PathList, pWayBoxes, startPos, closestFinalPos );

			// Indicate success
			bPathSuccess = true;
		}
		else
		{
			PathList.clear();
		}
	}

	// Clear collections
	m_SearchLeafColl.clear();
	m_OpenSearchColl.clear();

	// Return the path to the client
	return PathList;
}

// Calculate the cost between two WorldNodes
const float SiegePathfinder::CalculateNodeCost( SiegeLogicalNode* pStartNode, SiegePos& finalPos )
{
	// Get a test point from our start node
	SiegePos startPos;
	startPos.node	= pStartNode->GetSiegeNode()->GetGUID();
	startPos.pos	= pStartNode->GetLogicalMesh()->GetCenter();

	// Return the cost
	return( max_t( 0.0f, ApproxLength( gSiegeEngine.GetDifferenceVector( startPos, finalPos ) ) - ApproxLength( pStartNode->GetLogicalMesh()->GetHalfDiag() ) ) );
}

const float SiegePathfinder::CalculateNodeCost( SiegeLogicalNode* pStartNode, SiegeLogicalNode* pEndNode )
{
	// Get a test point from our start node
	SiegePos startPos;
	startPos.node	= pStartNode->GetSiegeNode()->GetGUID();
	startPos.pos	= pStartNode->GetLogicalMesh()->GetCenter();

	// Get a test point from our start node
	SiegePos endPos;
	endPos.node		= pEndNode->GetSiegeNode()->GetGUID();
	endPos.pos		= pEndNode->GetLogicalMesh()->GetCenter();

	// Return the cost
	return( max_t( 0.0f, ApproxLength( gSiegeEngine.GetDifferenceVector( startPos, endPos ) ) - ApproxLength( pEndNode->GetLogicalMesh()->GetHalfDiag() ) ) );
}

// Optimizes the node path by removing wasted step and loops
void SiegePathfinder::OptimizeNodePath()
{
	// To remove loops and wasted steps, we are going to see if any of the node in our path can
	// connect to a later node in our path, thereby skipping the nodes that lay between.

	// If we have a one or two node path, there is nothing to optimize, so just return
	if( m_PathList.size() < 3 )
	{
		return;
	}

	m_ExtraPathList.push_back( m_PathList.front() );

	LogicalNodeColl::iterator listIter	= m_PathList.begin();
	do
	{
		for( LogicalNodeColl::iterator i = listIter; i != m_PathList.end(); ++i )
		{
			if( m_ExtraPathList.back()->IsConnectedTo( (*i) ) )
			{
				listIter = i;
			}
		}

		// Add the new node
		m_ExtraPathList.push_back( (*listIter) );

	} while( (*listIter) != m_PathList.back() );

	// Set the new path
	m_PathList.swap( m_ExtraPathList );
	m_ExtraPathList.clear();
}

// Get a leaf from a position
unsigned short SiegePathfinder::GetLeafAtPosition( SiegeLogicalNode* pNode, const vector_3& pos )
{
	// Setup a modification vector to enlarge boxes to close tests don't fail
	vector_3 mod( 0.0f, 0.2f, 0.0f );

	SiegeLogicalMesh* pLogicalMesh	= pNode->GetLogicalMesh();

#if !GP_RETAIL

	// Make sure no one is pulling my leg
	if( !PointInBox( (pLogicalMesh->GetMinimumBounds()-mod),
					 (pLogicalMesh->GetMaximumBounds()+mod), pos ) )
	{
		gperrorf(( "Cannot find leaf in node because position is outside of node!\nThis error is usually caused by:\n    1) A movable actor has been placed above or below valid terrain.\n    2) The node 0x%08x using mesh 0x%08x has been improperly floored.\n    3) The logical node flags have been setup improperly and a resulting floor over floor situation has occurred.\n",
				   pNode->GetSiegeNode()->GetGUID().GetValue(),
				   pNode->GetSiegeNode()->GetMeshGUID().GetValue() ));
	}

#endif

	// This is the leaf we will return.  Because leaves can overlap in many cases, we
	// need to choose the one that best fits the given position, and not the first
	// one that we come across.  This is a simple distance test.
	unsigned short next	= 0xFFFF;
	float next_cost		= FLOAT_MAX;

	unsigned short hit	= 0xFFFF;
	float hit_cost		= FLOAT_MAX;

	for( unsigned int i = 0; i < pLogicalMesh->GetNumLeafConnections(); ++i )
	{
		// Get the leaf that we will be checking
		LMESHLEAFINFO* pLeaf	= &pLogicalMesh->GetLeafConnectionInfo()[ i ];

		// Get the cost of the position in this node
		float current_cost	= Length2( pLeaf->center - pos );

		if( current_cost < next_cost )
		{
			next		= pLeaf->id;
			next_cost	= current_cost;
		}

		// Check to see if our cost is less than the maximum squared difference across
		// a leaf box.  Since the largest size of the cube is 1 meter in each dimension, the
		// longest squared distance across the box is 3.0f.  It is not necessary that this
		// check be completely accurate since it is merely a means of reducing the work
		// in this function.
		if( current_cost <= 3.0f && current_cost < hit_cost )
		{
			for( unsigned int o = 0; o < pLeaf->numTriangles; ++o )
			{
				TriNorm* pTriangle	= &pLogicalMesh->GetBSPTree()->GetTriangles()[ pLeaf->pTriangles[ o ] ];
				if( math::RayIntersectsTriangle( vector_3( pos.x, pLeaf->minBox.y, pos.z ),	vector_3::UP,
												 pTriangle->m_Vertices[ 0 ],
												 pTriangle->m_Vertices[ 1 ],
												 pTriangle->m_Vertices[ 2 ] ) )
				{
					hit			= pLeaf->id;
					hit_cost	= current_cost;
					break;
				}
			}
		}
	}

	// Return the index of the matching leaf
	return( hit != 0xFFFF ? hit : next );
}

// Construct the path
void SiegePathfinder::ConstructPath( LMESHSEARCHLEAF* pFinalSearchLeaf, std::list< SiegePos >& pathList,
									 LeafBoxColl* pBoxList, SiegePos& startPos, SiegePos& finalPos )
{
	pathList.push_back( finalPos );

	gpassert( pFinalSearchLeaf->pLogicalNode->GetSiegeNode()->GetGUID() == finalPos.node );
	SiegeNode* pNode	= pFinalSearchLeaf->pLogicalNode->GetSiegeNode();

	LeafBox* pBox			= NULL;
	if( pBoxList )
	{
		pBox				= &*(pBoxList->push_back());
		pNode->NodeToWorldBox( pFinalSearchLeaf->pLeafInfo->minBox, pFinalSearchLeaf->pLeafInfo->maxBox,
							   pBox->min_box, pBox->max_box );
	}

	// Build the path
	LMESHSEARCHLEAF* pBuildLeaf		= NULL;
	if( pFinalSearchLeaf->parent	!= -1 )
	{
		pBuildLeaf					= &m_SearchLeafColl[ pFinalSearchLeaf->parent ];
		while( pBuildLeaf->parent	!= -1 )
		{
			// Calculate and insert a new world space box into the box collection
			pNode					= pBuildLeaf->pLogicalNode->GetSiegeNode();
			if( pBoxList )
			{
				pBox					= &*(pBoxList->push_back());
				pNode->NodeToWorldBox( pBuildLeaf->pLeafInfo->minBox, pBuildLeaf->pLeafInfo->maxBox,
									   pBox->min_box, pBox->max_box );
			}

			pathList.push_back( SiegePos( pBuildLeaf->pLeafInfo->center, pNode->GetGUID() ) );

			// Continue with path building
			pBuildLeaf				= &m_SearchLeafColl[ pBuildLeaf->parent ];
		}
	}

	// Push box and position for the start onto the list
	if( pBoxList )
	{
		if( pBuildLeaf )
		{
			pNode			= pBuildLeaf->pLogicalNode->GetSiegeNode();
			gpassert( pNode->GetGUID() == startPos.node );
			pBox			= &*(pBoxList->push_back());
			pNode->NodeToWorldBox( pBuildLeaf->pLeafInfo->minBox, pBuildLeaf->pLeafInfo->maxBox,
								   pBox->min_box, pBox->max_box );
		}
		else
		{
			pBoxList->push_back( pBoxList->front() );
		}
	}

	pathList.push_back( startPos );
	pathList.reverse();
}

