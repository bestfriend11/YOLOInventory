/*************************************************************************
**
**						Logical Node Creator
**
**		Implementation of the LogicalNodeCreator.  See .h file for
**		details.
**
**		Author:	James Loe
**
*************************************************************************/

// PCH
#include "PrecompEditor.h"
#include "malloc.h"

// Math includes
#include "vector_3.h"
#include "space_3.h"

// Renderer includes
#include "RapiOwner.h"
#include "RapiPrimitive.h"

// Siege includes
#include "siege_bsp.h"

// Class includes
#include "LogicalNodeCreator.h"

// Namespace
using namespace siege;

const float	CONNECTION_TOLERANCE	= 0.01f;
const float CONNECTION_PROBLEM		= CONNECTION_TOLERANCE / 10.0f;

// Global modification vector
const vector_3 mod( 0.01f, 0.01f, 0.01f );


// Construction
LogicalNodeCreator::LogicalNodeCreator()
{
}

// Destruction
LogicalNodeCreator::~LogicalNodeCreator()
{
	// Clean up the list
	ClearLogicalList();
}

// Clears and cleans up the current list of logical nodes
void LogicalNodeCreator::ClearLogicalList()
{
	// Clear the main list
	m_LogicalNodes.clear();
	m_BadNodeMap.clear();
}

// Write the logical node information to a file
void LogicalNodeCreator::WriteLogicalInformation( siege::SiegeEngine& engine, const siege::database_guid nodeGuid, HANDLE file )
{
	// Get this node's information
	LogicalNodeMap::iterator i	= m_LogicalNodes.find( nodeGuid );

	SiegeNodeHandle handle( engine.NodeCache().UseObject( nodeGuid ) );
	SiegeNode& node = handle.RequestObject(engine.NodeCache());

	SiegeMesh& mesh = node.GetMeshHandle().RequestObject( engine.MeshCache() );

	if( i != m_LogicalNodes.end() )
	{
		// First, we write the version number
		unsigned int version	= LVERSION;		
		unsigned long temp_bytes_written = 0;
		::WriteFile( file, &version, sizeof( unsigned int ), &temp_bytes_written, 0 );

		// Write out the checksum
		DWORD checksum	= mesh.GetMeshChecksum();		
		::WriteFile( file, &checksum, sizeof( DWORD ), &temp_bytes_written, 0 );

		// Write out bounding information
		vector_3 minBounds	= node.GetMinimumBounds();		
		::WriteFile( file, &minBounds, sizeof( vector_3 ), &temp_bytes_written, 0 );

		vector_3 maxBounds	= node.GetMaximumBounds();		
		::WriteFile( file, &maxBounds, sizeof( vector_3 ), &temp_bytes_written, 0 );

		// Write the number of logical nodes we are about to write to file
		unsigned int num_lnodes	= (*i).second.size();		
		::WriteFile( file, &num_lnodes, sizeof( unsigned int ), &temp_bytes_written, 0 );

		// Write all of the logical nodes out to file
		for( LogicalNodeList::iterator l = (*i).second.begin(); l != (*i).second.end(); ++l )
		{
			// Write information about this logical node
			unsigned char hellocompiler	= (*l)->GetID();			
			::WriteFile( file, &hellocompiler, sizeof( unsigned char ), &temp_bytes_written, 0 );

			unsigned int iloveyoucompiler	= (*l)->GetFlags();			
			::WriteFile( file, &iloveyoucompiler, sizeof( unsigned int ), &temp_bytes_written, 0 );

			unsigned short numNodeConnections = 0;
			for( unsigned int n = 0; n < (*l)->GetNumNodeConnections(); ++n )
			{
				if( (*l)->GetNodeConnectionInfo()[ n ].m_farSiegeNode != node.GetGUID() )
				{
					numNodeConnections++;
				}
			}
			
			::WriteFile( file, &numNodeConnections, sizeof( unsigned short ), &temp_bytes_written, 0 );

			for( n = 0; n < (*l)->GetNumNodeConnections(); ++n )
			{
				// Get a pointer to our connection
				LNODECONNECT* pConnection	= &(*l)->GetNodeConnectionInfo()[ n ];

				if( pConnection->m_farSiegeNode != node.GetGUID() )
				{
					// Siege node that owns this object					
					::WriteFile( file, &pConnection->m_farSiegeNode, sizeof( database_guid ), &temp_bytes_written, 0 );

					// Unique identifier of node (corresponds to logical mesh id)					
					::WriteFile( file, &pConnection->m_pCollection->m_farid, sizeof( unsigned char ), &temp_bytes_written, 0 );

					// List of leaf connections
					::WriteFile( file, &pConnection->m_pCollection->m_numNodalLeafConnections, sizeof( unsigned int ), &temp_bytes_written, 0 );
					::WriteFile( file, pConnection->m_pCollection->m_pNodalLeafConnections, sizeof( NODALLEAFCONNECT ) * pConnection->m_pCollection->m_numNodalLeafConnections, &temp_bytes_written, 0 );										
				}
			}
		}
	}
}

// Read the logical node information from a file
bool LogicalNodeCreator::ReadLogicalInformation( siege::SiegeEngine& engine, const siege::database_guid nodeGuid )
{
	// Build a pair and attach it to our map
	LogicalNodeList nList;

	SiegeNodeHandle handle( engine.NodeCache().UseObject( nodeGuid ) );
	SiegeNode& node = handle.RequestObject(engine.NodeCache());

	for( unsigned int i = 0; i < node.GetNumLogicalNodes(); ++i )
	{
		nList.push_back( node.GetLogicalNodes()[ i ] );
	}

	std::pair< database_guid, LogicalNodeList > nPair( nodeGuid, nList );
	m_LogicalNodes.insert( nPair );

	return true;
}

// Attach a node to the logical tree
void LogicalNodeCreator::AttachSiegeNode( SiegeEngine& engine, const database_guid nodeGuid )
{
	// Find the node
	std::map< siege::database_guid, LogicalNodeList >::iterator i = m_LogicalNodes.find( nodeGuid );
	
	if( i == m_LogicalNodes.end() )
	{
		// Build the logical nodes from their logical mesh counterparts
		SiegeNodeHandle handle( engine.NodeCache().UseObject( nodeGuid ) );
		SiegeNode& node = handle.RequestObject(engine.NodeCache());
		
		if( node.GetLogicalNodes() )
		{
			// Logical nodes exist, but are not in our list.  We will assume at this point
			// that the logical nodes here are bad, and recalc them
			for( unsigned int l = 0; l < node.GetNumLogicalNodes(); ++l )
			{
				delete node.GetLogicalNodes()[ l ];
			}

			delete[] node.GetLogicalNodes();

			node.SetNumLogicalNodes( 0 );
			node.SetLogicalNodes( NULL );
		}

		gpassert( node.GetLogicalNodes() == NULL );

		// Get current node boundary mesh
		SiegeMesh& bmesh = node.GetMeshHandle().RequestObject(engine.MeshCache());

		// Make sure bounding information is setup correctly
		node.SetMinimumBounds( bmesh.GetMinimumCorner() );
		node.SetMaximumBounds( bmesh.GetMaximumCorner() );
		node.SetNumLitVertices( bmesh.GetNumberOfVertices() );

		LogicalNodeList logicalList;

		SiegeLogicalNode** pLogicalNodes	= new SiegeLogicalNode*[ bmesh.GetNumLogicalMeshes() ];
		node.SetNumLogicalNodes( bmesh.GetNumLogicalMeshes() );
		node.SetLogicalNodes( pLogicalNodes );

		for( unsigned int l = 0; l < bmesh.GetNumLogicalMeshes(); ++l )
		{
			pLogicalNodes[ l ]	= new SiegeLogicalNode;
			logicalList.push_back( pLogicalNodes[ l ] );

			// Build the logical node
			gpassert( bmesh.GetLogicalMeshes()[ l ].GetID() == l );
			pLogicalNodes[ l ]->SetID( bmesh.GetLogicalMeshes()[ l ].GetID() );
			pLogicalNodes[ l ]->SetSiegeNode( &node );
			pLogicalNodes[ l ]->SetLogicalMesh( &bmesh.GetLogicalMeshes()[ l ] );
			pLogicalNodes[ l ]->SetFlags( bmesh.GetLogicalMeshes()[ l ].GetFlags() );
			
			DWORD flags = pLogicalNodes[ l ]->GetFlags();
			if( flags & 0x00000001 )
			{
				flags &= ~0x00000001;
				flags |= LF_HUMAN_PLAYER | LF_COMPUTER_PLAYER | LF_DIRT;
			}

			pLogicalNodes[ l ]->SetFlags( flags );
		}

		// Put these new logical nodes into our map
		std::pair< database_guid, LogicalNodeList > nPair( nodeGuid, logicalList );
		m_LogicalNodes.insert( nPair );

		// As a final step, calculate the nodal connection information
		CalculateNodalConnections( engine, nodeGuid );
	}
}

// Detach a node from the logical tree
void LogicalNodeCreator::DetachSiegeNode( const database_guid nodeGuid )
{
	// First, get rid of this node's entry in the map
	LogicalNodeMap::iterator i = m_LogicalNodes.find( nodeGuid );
	if( i != m_LogicalNodes.end() )
	{
		m_LogicalNodes.erase( i );
	}

	// The go through all of the other nodes and their logical nodes and remove any connection info
	for( i = m_LogicalNodes.begin(); i != m_LogicalNodes.end(); ++i )
	{
		// And then go through all of their logical nodes
		for( LogicalNodeList::iterator l = (*i).second.begin(); l != (*i).second.end(); ++l )
		{
			for( int n = 0; n < (*l)->GetNumNodeConnections(); ++n )
			{
				if( (*l)->GetNodeConnectionInfo()[ n ].m_farSiegeNode == nodeGuid )
				{
					// Remove this block from the list
					LNODECONNECT* pConnections		= (*l)->GetNodeConnectionInfo();

					delete[]	pConnections[ n ].m_pCollection->m_pNodalLeafConnections;
					delete		pConnections[ n ].m_pCollection;

					LNODECONNECT* pNewConnections	= new LNODECONNECT[ (*l)->GetNumNodeConnections() - 1 ];
					memcpy( pNewConnections, pConnections, sizeof( LNODECONNECT ) * n );
					memcpy( &pNewConnections[n], &pConnections[n+1], sizeof( LNODECONNECT ) * ((*l)->GetNumNodeConnections() - n - 1) );

					delete[] pConnections;
					(*l)->SetNodeConnectionInfo( pNewConnections );

					unsigned short compilerismyfriend	= (*l)->GetNumNodeConnections();
					compilerismyfriend					-= 1;
					(*l)->SetNumNodeConnections( compilerismyfriend );
					--n;
				}
			}
		}
	}
}

// Calculate connection information
void LogicalNodeCreator::CalculateNodalConnections( SiegeEngine& engine, const database_guid nodeGuid )
{
	std::set< SiegeLogicalNode* > possibleConnections;

	// Get the node so we can look through the doors
	SiegeNodeHandle handle( engine.NodeCache().UseObject( nodeGuid ) );
	SiegeNode& node = handle.RequestObject(engine.NodeCache());						

	// Go through the doors
	const SiegeNodeDoorList& doors	= node.GetDoors();
	for( SiegeNodeDoorConstIter d = doors.begin(); d != doors.end(); ++d )
	{
		if( (*d)->GetNeighbour() == UNDEFINED_GUID )
		{
			continue;
		}

		// Get the logical nodes attached to this neighbor
		LogicalNodeMap::iterator m	= m_LogicalNodes.find( (*d)->GetNeighbour() );
		if( m != m_LogicalNodes.end() )
		{
			for( LogicalNodeList::iterator n = (*m).second.begin(); n != (*m).second.end(); ++n )
			{
				possibleConnections.insert( (*n) );
			}
		}
	}

	// Get the logical nodes for this node to use for comparison with our possibles
	LogicalNodeMap::iterator m	= m_LogicalNodes.find( nodeGuid );
	if( m != m_LogicalNodes.end() )
	{
		// Go through all the local logical nodes
		for( LogicalNodeList::iterator n = (*m).second.begin(); n != (*m).second.end(); ++n )
		{
			// Go through all available possible connections and see if connection needs to be made
			for( std::set< SiegeLogicalNode* >::iterator i = possibleConnections.begin(); i != possibleConnections.end(); ++i )
			{
				if( !LogicalNodesAreConnected( (*n), (*i) ) )
				{
					CalculateLogicalNodeConnections( engine, (*n), (*i) );
				}
			}
		}
	}
}

// Draws the logical nodes of the given node
void LogicalNodeCreator::DrawLogicalNodes( SiegeEngine& engine, const database_guid nodeGuid )
{
	UNREFERENCED_PARAMETER( engine );
	UNREFERENCED_PARAMETER( nodeGuid );
}

// Get the file size of an LNO for the given guid
unsigned int LogicalNodeCreator::GetSizeOfLNO( siege::database_guid const nodeGuid )
{
	// Find the node
	LogicalNodeMap::iterator i = m_LogicalNodes.find( nodeGuid );

	if( i != m_LogicalNodes.end() )
	{
		unsigned int LNOSize	= 0;

		// First, we write the version number
		LNOSize					+= sizeof( unsigned int );

		// Checksum information
		LNOSize					+= sizeof( DWORD );

		// Bounding information
		LNOSize					+= sizeof( vector_3 );
		LNOSize					+= sizeof( vector_3 );

		// Write the number of logical nodes we are about to write to file
		LNOSize					+= sizeof( unsigned int );

		// Write all of the logical nodes out to file
		for( LogicalNodeList::iterator l = (*i).second.begin(); l != (*i).second.end(); ++l )
		{
			// Write information about this logical node
			LNOSize				+= sizeof( unsigned char );
			LNOSize				+= sizeof( unsigned int );
			LNOSize				+= sizeof( unsigned short );

			for( unsigned int n = 0; n < (*l)->GetNumNodeConnections(); ++n )
			{
				if( (*l)->GetNodeConnectionInfo()[ n ].m_farSiegeNode != nodeGuid )
				{
					// Siege node that owns this object
					LNOSize			+= sizeof( database_guid );

					// Unique identifier of node (corresponds to logical mesh id)
					LNOSize			+= sizeof( unsigned char );

					// List of leaf connections
					LNOSize			+= sizeof( unsigned int );
					LNOSize			+= sizeof( NODALLEAFCONNECT ) * (*l)->GetNodeConnectionInfo()[ n ].m_pCollection->m_numNodalLeafConnections;
				}
			}
		}

		return LNOSize;
	}
	else
	{
		gpassertm( 0, "Requested an LNO size from the creator which was not in the mapping!" );
	}

	return 0;
}

// Do these two leaves intersect?
bool LogicalNodeCreator::LogicalLeavesIntersect( siege::SiegeEngine& engine, LMESHLEAFINFO& leaf1, SiegeLogicalNode* node1, LMESHLEAFINFO& leaf2, SiegeLogicalNode* node2 )
{
	UNREFERENCED_PARAMETER( engine );
	gpassert( node1->GetSiegeNode() != node2->GetSiegeNode() );

	bool bConnect		= false;

	// Nodal connection
	vector_3 minBox1( DoNotInitialize );
	vector_3 maxBox1( DoNotInitialize );
	node1->GetSiegeNode()->NodeToWorldBox( leaf1.minBox, leaf1.maxBox, minBox1, maxBox1 );

	vector_3 minBox2( DoNotInitialize );
	vector_3 maxBox2( DoNotInitialize );
	node2->GetSiegeNode()->NodeToWorldBox( leaf2.minBox, leaf2.maxBox, minBox2, maxBox2 );

	if( BoxIntersectsBox( minBox1-mod, maxBox1+mod, minBox2-mod, maxBox2+mod ) )
	{
		vector_3* pLeaf1worldTri = (vector_3*)_alloca( sizeof( vector_3 ) * (leaf1.numTriangles*3) );
		vector_3* pLeaf2worldTri = (vector_3*)_alloca( sizeof( vector_3 ) * (leaf2.numTriangles*3) );

		// Get the world space triangle information
		for( unsigned int t = 0; t < leaf1.numTriangles; ++t )
		{
			pLeaf1worldTri[(t*3)]	= node1->GetSiegeNode()->NodeToWorldSpace( node1->GetLogicalMesh()->GetBSPTree()->GetTriangles()[ leaf1.pTriangles[t] ].m_Vertices[0] );
			pLeaf1worldTri[(t*3)+1]	= node1->GetSiegeNode()->NodeToWorldSpace( node1->GetLogicalMesh()->GetBSPTree()->GetTriangles()[ leaf1.pTriangles[t] ].m_Vertices[1] );
			pLeaf1worldTri[(t*3)+2]	= node1->GetSiegeNode()->NodeToWorldSpace( node1->GetLogicalMesh()->GetBSPTree()->GetTriangles()[ leaf1.pTriangles[t] ].m_Vertices[2] );
		}

		for( t = 0; t < leaf2.numTriangles; ++t )
		{
			pLeaf2worldTri[(t*3)]	= node2->GetSiegeNode()->NodeToWorldSpace( node2->GetLogicalMesh()->GetBSPTree()->GetTriangles()[ leaf2.pTriangles[t] ].m_Vertices[0] );
			pLeaf2worldTri[(t*3)+1]	= node2->GetSiegeNode()->NodeToWorldSpace( node2->GetLogicalMesh()->GetBSPTree()->GetTriangles()[ leaf2.pTriangles[t] ].m_Vertices[1] );
			pLeaf2worldTri[(t*3)+2]	= node2->GetSiegeNode()->NodeToWorldSpace( node2->GetLogicalMesh()->GetBSPTree()->GetTriangles()[ leaf2.pTriangles[t] ].m_Vertices[2] );
		}

		for( unsigned int i = 0; i < (leaf1.numTriangles*3); ++i )
		{
			for( unsigned int o = 0; o < (leaf2.numTriangles*3); ++o )
			{
				vector_3 testVect = pLeaf1worldTri[i] - pLeaf2worldTri[o];

				if( testVect.IsZero( CONNECTION_TOLERANCE ) )
				{
					bConnect = true;
					break;
				}
				else
				{
					if( testVect.IsZero( CONNECTION_PROBLEM ) )
					{
						BadNodeMap::iterator n = m_BadNodeMap.find( node1->GetSiegeNode()->GetGUID() );
						if( n != m_BadNodeMap.end() )
						{
							// Put this new node in the set
							(*n).second.insert( node2->GetSiegeNode()->GetGUID() );
						}
						else
						{
							// Put this node into the mapping
							std::set< siege::database_guid > nSet;
							nSet.insert( node2->GetSiegeNode()->GetGUID() );
							m_BadNodeMap.insert( std::make_pair( node1->GetSiegeNode()->GetGUID(), nSet ) );
						}

						n = m_BadNodeMap.find( node2->GetSiegeNode()->GetGUID() );
						if( n != m_BadNodeMap.end() )
						{
							// Put this new node in the set
							(*n).second.insert( node1->GetSiegeNode()->GetGUID() );
						}
						else
						{
							// Put this node into the mapping
							std::set< siege::database_guid > nSet;
							nSet.insert( node1->GetSiegeNode()->GetGUID() );
							m_BadNodeMap.insert( std::make_pair( node2->GetSiegeNode()->GetGUID(), nSet ) );
						}
					}
				}
			}
			if( bConnect )
			{
				break;
			}
		}
	}

	if( !bConnect )
	{
		return false;
	}

	vector_3 nminBox1( DoNotInitialize );
	vector_3 nmaxBox1( DoNotInitialize );
	node1->GetSiegeNode()->NodeToWorldBox( node1->GetLogicalMesh()->GetMinimumBounds(), node1->GetLogicalMesh()->GetMaximumBounds(), nminBox1, nmaxBox1 );

	vector_3 nminBox2( DoNotInitialize );
	vector_3 nmaxBox2( DoNotInitialize );
	node2->GetSiegeNode()->NodeToWorldBox( node2->GetLogicalMesh()->GetMinimumBounds(), node2->GetLogicalMesh()->GetMaximumBounds(), nminBox2, nmaxBox2 );

	float max_y			= max( nmaxBox1.y, nmaxBox2.y ) + 0.1f;

	vector_3 worldSpace1Center	= node1->GetSiegeNode()->NodeToWorldSpace( leaf1.center );
	vector_3 worldSpace2Center	= node2->GetSiegeNode()->NodeToWorldSpace( leaf2.center );

	float	ray_t	= FLOAT_MAX;
	vector_3 face_normal( DoNotInitialize );

	// Now we need to do poly testing.  Whee fun!
	vector_3 testVector	= worldSpace2Center - worldSpace1Center;
	for( unsigned int i = 1; i < 8; ++i )
	{
		vector_3 testPoint1		= worldSpace1Center + (testVector * ((0.125f * (float)i) - 0.01f));
		vector_3 testPoint2		= worldSpace1Center + (testVector * ((0.125f * (float)i) + 0.01f));
		testPoint1.y = testPoint2.y = max_y;

		if( !node1->HitTest( node1->GetSiegeNode()->WorldToNodeSpace( testPoint1 ), vector_3( 0.0f, -1.0f, 0.0f ), ray_t, face_normal ) &&
			!node2->HitTest( node2->GetSiegeNode()->WorldToNodeSpace( testPoint2 ), vector_3( 0.0f, -1.0f, 0.0f ), ray_t, face_normal ) )
		{
			return false;
		}
	}

	return true;
}

// See if two logical nodes are already connected
bool LogicalNodeCreator::LogicalNodesAreConnected( const SiegeLogicalNode* pNode1, const SiegeLogicalNode* pNode2 )
{
	for( unsigned int i = 0; i < pNode1->GetNumNodeConnections(); ++i )
	{
		if( pNode1->GetNodeConnectionInfo()[ i ].m_farSiegeNode			== pNode2->GetSiegeNode()->GetGUID() &&
			pNode1->GetNodeConnectionInfo()[ i ].m_pCollection->m_farid	== pNode2->GetID() )
		{
			return true;
		}
	}

	return false;
}

// Connect to leafs together
void LogicalNodeCreator::ConnectLeafs( LNODECONNECT* pNode1, unsigned short leafid1, LNODECONNECT* pNode2, unsigned short leafid2 )
{
	// Nodal connection because it it between to logical nodes
	bool IsAlreadyInList	= false;

	for( unsigned int o = 0; o < pNode1->m_pCollection->m_numNodalLeafConnections; ++o )
	{
		if( pNode1->m_pCollection->m_pNodalLeafConnections[ o ].local_id	== leafid1 &&
			pNode1->m_pCollection->m_pNodalLeafConnections[ o ].far_id		== leafid2 )
		{
			IsAlreadyInList = true;
			break;
		}
	}
	if( !IsAlreadyInList )
	{
		NODALLEAFCONNECT nConnect;
		nConnect.local_id	= leafid1;
		nConnect.far_id		= leafid2;

		NODALLEAFCONNECT* poldConnections	= pNode1->m_pCollection->m_pNodalLeafConnections;
		NODALLEAFCONNECT* pnewConnections	= new NODALLEAFCONNECT[ pNode1->m_pCollection->m_numNodalLeafConnections + 1 ];
		memcpy( pnewConnections, poldConnections, sizeof( NODALLEAFCONNECT ) * pNode1->m_pCollection->m_numNodalLeafConnections );
		delete[] poldConnections;

		pnewConnections[ pNode1->m_pCollection->m_numNodalLeafConnections ] = nConnect;
		pNode1->m_pCollection->m_numNodalLeafConnections++;
		pNode1->m_pCollection->m_pNodalLeafConnections	= pnewConnections;
	}

	IsAlreadyInList		= false;

	for( o = 0; o < pNode2->m_pCollection->m_numNodalLeafConnections; ++o )
	{
		if( pNode2->m_pCollection->m_pNodalLeafConnections[ o ].local_id	== leafid2 &&
			pNode2->m_pCollection->m_pNodalLeafConnections[ o ].far_id		== leafid1 )
		{
			IsAlreadyInList = true;
			break;
		}
	}
	if( !IsAlreadyInList )
	{
		NODALLEAFCONNECT nConnect;
		nConnect.local_id	= leafid2;
		nConnect.far_id		= leafid1;

		NODALLEAFCONNECT* poldConnections	= pNode2->m_pCollection->m_pNodalLeafConnections;
		NODALLEAFCONNECT* pnewConnections	= new NODALLEAFCONNECT[ pNode2->m_pCollection->m_numNodalLeafConnections + 1 ];
		memcpy( pnewConnections, poldConnections, sizeof( NODALLEAFCONNECT ) * pNode2->m_pCollection->m_numNodalLeafConnections );
		delete[] poldConnections;

		pnewConnections[ pNode2->m_pCollection->m_numNodalLeafConnections ] = nConnect;
		pNode2->m_pCollection->m_numNodalLeafConnections++;
		pNode2->m_pCollection->m_pNodalLeafConnections	= pnewConnections;
	}
}

// Connect two logical nodes together
void LogicalNodeCreator::ConnectLogicalNodes( SiegeLogicalNode* pnNode, SiegeLogicalNode* pfNode )
{
	// The nodes intersect, so we insert connection info into both of the nodes
	// if the connection specified has not already been calculated
	bool IsAlreadyInList	= false;

	for( unsigned int i = 0; i < pnNode->GetNumNodeConnections(); ++i )
	{
		if( pnNode->GetNodeConnectionInfo()[ i ].m_farSiegeNode			== pfNode->GetSiegeNode()->GetGUID() &&
			pnNode->GetNodeConnectionInfo()[ i ].m_pCollection->m_farid	== pfNode->GetID() )
		{
			IsAlreadyInList = true;
			break;
		}
	}
	if( !IsAlreadyInList )
	{
		LNODECONNECT* pOldConnections	= pnNode->GetNodeConnectionInfo();
		LNODECONNECT* pNewConnections	= new LNODECONNECT[ pnNode->GetNumNodeConnections() + 1 ];
		memcpy( pNewConnections, pOldConnections, sizeof( LNODECONNECT ) * pnNode->GetNumNodeConnections() );

		delete[] pOldConnections;

		pNewConnections[ pnNode->GetNumNodeConnections() ].m_farSiegeNode							= pfNode->GetSiegeNode()->GetGUID();
		pNewConnections[ pnNode->GetNumNodeConnections() ].m_pCollection							= new LCCOLLECTION;
		pNewConnections[ pnNode->GetNumNodeConnections() ].m_pCollection->m_farid					= pfNode->GetID();
		pNewConnections[ pnNode->GetNumNodeConnections() ].m_pCollection->m_numNodalLeafConnections	= 0;
		pNewConnections[ pnNode->GetNumNodeConnections() ].m_pCollection->m_pNodalLeafConnections	= NULL;

		unsigned short compilerismyfriend	= pnNode->GetNumNodeConnections();
		compilerismyfriend					+= 1;
		pnNode->SetNumNodeConnections( compilerismyfriend );
		pnNode->SetNodeConnectionInfo( pNewConnections );
	}

	// Do the other node the same way
	IsAlreadyInList	= false;
	for( i = 0; i < pfNode->GetNumNodeConnections(); ++i )
	{
		if( pfNode->GetNodeConnectionInfo()[ i ].m_farSiegeNode			== pnNode->GetSiegeNode()->GetGUID() &&
			pfNode->GetNodeConnectionInfo()[ i ].m_pCollection->m_farid	== pnNode->GetID() )
		{
			IsAlreadyInList = true;
			break;
		}
	}
	if( !IsAlreadyInList )
	{
		LNODECONNECT* pOldConnections	= pfNode->GetNodeConnectionInfo();
		LNODECONNECT* pNewConnections	= new LNODECONNECT[ pfNode->GetNumNodeConnections() + 1 ];
		memcpy( pNewConnections, pOldConnections, sizeof( LNODECONNECT ) * pfNode->GetNumNodeConnections() );

		delete[] pOldConnections;

		pNewConnections[ pfNode->GetNumNodeConnections() ].m_farSiegeNode							= pnNode->GetSiegeNode()->GetGUID();
		pNewConnections[ pfNode->GetNumNodeConnections() ].m_pCollection							= new LCCOLLECTION;
		pNewConnections[ pfNode->GetNumNodeConnections() ].m_pCollection->m_farid					= pnNode->GetID();
		pNewConnections[ pfNode->GetNumNodeConnections() ].m_pCollection->m_numNodalLeafConnections	= 0;
		pNewConnections[ pfNode->GetNumNodeConnections() ].m_pCollection->m_pNodalLeafConnections	= NULL;

		unsigned short compilerismyfriend	= pfNode->GetNumNodeConnections();
		compilerismyfriend					+= 1;
		pfNode->SetNumNodeConnections( compilerismyfriend );
		pfNode->SetNodeConnectionInfo( pNewConnections );
	}
}

// Calculate the internodal connections
void LogicalNodeCreator::CalculateLogicalNodeConnections( siege::SiegeEngine& engine, SiegeLogicalNode* pnNode, SiegeLogicalNode* pfNode )
{
	bool bConnectNodes	= false;

	LNODECONNECT*	pnNodeConnect	= NULL;
	LNODECONNECT*	pfNodeConnect	= NULL;

	vector_3 minBox2( DoNotInitialize );
	vector_3 maxBox2( DoNotInitialize );
	pfNode->GetSiegeNode()->NodeToWorldBox( pfNode->GetLogicalMesh()->GetMinimumBounds(), pfNode->GetLogicalMesh()->GetMaximumBounds(),
											minBox2, maxBox2 );
	minBox2				-= mod;
	maxBox2				+= mod;

	// Go through all the leaves
	for( unsigned int i = 0; i < pnNode->GetLogicalMesh()->GetNumLeafConnections(); ++i )
	{
		vector_3 minBox1( DoNotInitialize );
		vector_3 maxBox1( DoNotInitialize );
		pnNode->GetSiegeNode()->NodeToWorldBox( pnNode->GetLogicalMesh()->GetLeafConnectionInfo()[ i ].minBox,
												pnNode->GetLogicalMesh()->GetLeafConnectionInfo()[ i ].maxBox,
												minBox1, maxBox1 );

		if( BoxIntersectsBox( minBox1-mod, maxBox1+mod, minBox2, maxBox2 ) )
		{
			for( unsigned int o = 0; o < pfNode->GetLogicalMesh()->GetNumLeafConnections(); ++o )
			{
				if( LogicalLeavesIntersect( engine,
											pnNode->GetLogicalMesh()->GetLeafConnectionInfo()[ i ], pnNode,
											pfNode->GetLogicalMesh()->GetLeafConnectionInfo()[ o ], pfNode ) )
				{
					if( !bConnectNodes )
					{
						bConnectNodes	= true;
						ConnectLogicalNodes( pnNode, pfNode );

						for( unsigned int n = 0; n < pnNode->GetNumNodeConnections(); ++n )
						{
							if( pnNode->GetNodeConnectionInfo()[ n ].m_farSiegeNode			== pfNode->GetSiegeNode()->GetGUID() &&
								pnNode->GetNodeConnectionInfo()[ n ].m_pCollection->m_farid	== pfNode->GetID() )
							{
								pnNodeConnect	= &pnNode->GetNodeConnectionInfo()[ n ];
							}
						}
						for( n = 0; n < pfNode->GetNumNodeConnections(); ++n )
						{
							if( pfNode->GetNodeConnectionInfo()[ n ].m_farSiegeNode			== pnNode->GetSiegeNode()->GetGUID() &&
								pfNode->GetNodeConnectionInfo()[ n ].m_pCollection->m_farid	== pnNode->GetID() )
							{
								pfNodeConnect	= &pfNode->GetNodeConnectionInfo()[ n ];
							}
						}
					}

					ConnectLeafs( pnNodeConnect, pnNode->GetLogicalMesh()->GetLeafConnectionInfo()[ i ].id, pfNodeConnect, pfNode->GetLogicalMesh()->GetLeafConnectionInfo()[ o ].id );
				}
			}
		}
	}
}

// Check to see if two checksums are the same
bool LogicalNodeCreator::ChecksumPasses( LNOChecksum& sum1, LNOChecksum& sum2 )
{
	// Check the number of verts and tris first
	if( sum1.numTris	!= sum2.numTris		||
		sum1.numVerts	!= sum2.numVerts )
	{
		return false;
	}

	if( fabs( sum1.xSum - sum2.xSum ) > 0.00001 )
	{
		return false;
	}

	if( fabs( sum1.ySum - sum2.ySum ) > 0.00001 )
	{
		return false;
	}

	if( fabs( sum2.zSum - sum2.zSum ) > 0.00001 )
	{
		return false;
	}

	return true;
}

// Calculate a checksum from a node
LNOChecksum LogicalNodeCreator::CalculateChecksum( siege::SiegeEngine& engine, const siege::database_guid& nodeGuid )
{
	// Setup a new checksum
	LNOChecksum	checksum;

	// Get the node and the mesh for the node
	SiegeNodeHandle handle( engine.NodeCache().UseObject( nodeGuid ) );
	SiegeNode& node = handle.RequestObject( engine.NodeCache() );						

	// Get current node boundary mesh
	SiegeMesh& bmesh = node.GetMeshHandle().RequestObject(engine.MeshCache());

	// Build the checksum
	checksum.numTris	= bmesh.GetNumberOfTriangles();
	checksum.numVerts	= bmesh.GetNumberOfVertices();

	checksum.xSum		= 0.0;
	checksum.ySum		= 0.0;
	checksum.zSum		= 0.0;

	for( unsigned int i = 0; i < checksum.numVerts; ++i )
	{
		checksum.xSum	+= bmesh.GetVertices()[ i ].x;
		checksum.ySum	+= bmesh.GetVertices()[ i ].y;
		checksum.zSum	+= bmesh.GetVertices()[ i ].z;
	}

	return checksum;
}
