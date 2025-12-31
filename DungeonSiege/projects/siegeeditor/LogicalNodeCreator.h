#ifndef _LOGICAL_NODE_CREATOR_
#define _LOGICAL_NODE_CREATOR_

/**********************************************************************
**
**					Logical node calculation
**
**		This class is responsible for the calculation of logical nodes
**		and their connection information, as well as calculating BSP
**		trees for those nodes and writing them out to disk.
**
**		The structures used in the creator were specifically created
**		to be easily written and read from a file, and are admittedly
**		slow because any traversal of them requires an N time lookup.
**		Game side structures that match these should resolve my id's
**		into pointers upon load time, so that lookups and searches
**		are completely removed, and speed substantially increased.
**
**		Author:  James Loe
**		Date:	 November 2, 1999
**
**********************************************************************/

#include "siege_logical_node.h"
#include "siege_logical_mesh.h"


namespace siege
{
	class SiegeEngine;
};


// Version of Logical Nodes
#define LVERSION					10

// Checksum
struct LNOChecksum
{
	unsigned int	numTris;
	unsigned int	numVerts;
	double			xSum;
	double			ySum;
	double			zSum;
};

typedef std::map< siege::database_guid, std::set< siege::database_guid > >		BadNodeMap;

class LogicalNodeCreator
{
public:

	// Construction and destruction
	LogicalNodeCreator();
	~LogicalNodeCreator();

	// Clears the list of current logical nodes
	void ClearLogicalList();

	// Write the logical node information to a file
	void WriteLogicalInformation( siege::SiegeEngine& engine, const siege::database_guid nodeGuid, HANDLE file );

	// Read the logical node information from a file
	bool ReadLogicalInformation( siege::SiegeEngine& engine, const siege::database_guid nodeGuid );

	// Attach a node to the logical tree
	void AttachSiegeNode( siege::SiegeEngine& engine, const siege::database_guid nodeGuid );

	// Detach a node from the logical tree
	void DetachSiegeNode( const siege::database_guid nodeGuid );

	// Calculate connection information
	void CalculateNodalConnections( siege::SiegeEngine& engine, const siege::database_guid nodeGuid );

	// Draws the logical nodes of the given node
	void DrawLogicalNodes( siege::SiegeEngine& engine, const siege::database_guid nodeGuid );

	// Get the file size of an LNO for the given guid
	unsigned int GetSizeOfLNO( siege::database_guid const nodeGuid );

	// Get the bad mesh set
	const BadNodeMap&	GetBadNodeMap()				{ return m_BadNodeMap; }

private:

	typedef std::list< siege::SiegeLogicalNode* >				LogicalNodeList;
	typedef std::map< siege::database_guid, LogicalNodeList >	LogicalNodeMap;

	// List of current logical nodes in this region
	LogicalNodeMap		m_LogicalNodes;

	// Does the current node have any issues?
	BadNodeMap			m_BadNodeMap;


	// Do these two leaves intersect?
	bool LogicalLeavesIntersect( siege::SiegeEngine& engine, siege::LMESHLEAFINFO& leaf1, siege::SiegeLogicalNode* node1, siege::LMESHLEAFINFO& leaf2, siege::SiegeLogicalNode* node2 );

	// See if two logical nodes are already connected
	bool LogicalNodesAreConnected( const siege::SiegeLogicalNode* pNode1, const siege::SiegeLogicalNode* pNode2 );

	// Connect to leafs together
	void ConnectLeafs( siege::LNODECONNECT* pNode1, unsigned short leafid1, siege::LNODECONNECT* pNode2, unsigned short leafid2 );

	// Connect to logical nodes together
	void ConnectLogicalNodes( siege::SiegeLogicalNode* pnNode, siege::SiegeLogicalNode* pfNode );

	// Calculate the internodal connections
	void CalculateLogicalNodeConnections( siege::SiegeEngine& engine, siege::SiegeLogicalNode* pnNode, siege::SiegeLogicalNode* pfNode );

	// Check to see if two checksums are the same
	bool ChecksumPasses( LNOChecksum& sum1, LNOChecksum& sum2 );

	// Calculate a checksum from a node
	LNOChecksum CalculateChecksum( siege::SiegeEngine& engine, const siege::database_guid& nodeGuid );
};


#endif