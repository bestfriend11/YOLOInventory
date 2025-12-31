/*********************************************************************************
**
**							SiegeLogicalMesh
**
**		Author:		James Loe
**		Date:		07/12/00
**
**********************************************************************************/

#include "precomp_siege.h"
#include "siege_logical_mesh.h"

using namespace siege;


// Construction
SiegeLogicalMesh::SiegeLogicalMesh()
	: m_id( 0 )
	, m_pBSPTree( NULL )
	, m_flags( 0 )
	, m_numLeafConnections( 0 )
	, m_pLeafConnectionInfo( NULL )
{
}

// Destruction
SiegeLogicalMesh::~SiegeLogicalMesh()
{
	delete		m_pBSPTree;

	for( unsigned int i = 0; i < m_numLeafConnections; ++i )
	{
		delete[] m_pLeafConnectionInfo[ i ].pTriangles;
		delete[] m_pLeafConnectionInfo[ i ].localConnections;
	}
	delete[]	m_pLeafConnectionInfo;

	for( i = 0; i < m_numNodalConnections; ++i )
	{
		delete[] m_pNodalConnectionInfo[ i ].m_pNodalLeafConnections;
	}
	delete[]	m_pNodalConnectionInfo;
}

// Trace a ray into this logical node
bool SiegeLogicalMesh::HitTest( const vector_3& ray_orig, const vector_3& ray_dir,
								float& ray_t, vector_3& facenormal )
{
	if( m_pBSPTree )
	{
		if( IsZero( ray_dir.x ) && IsZero( ray_dir.z ) )
		{
			return m_pBSPTree->YRayIntersectTree( ray_orig, ray_dir.y, ray_t, facenormal );
		}
		else
		{
			return m_pBSPTree->RayIntersectTree( ray_orig, ray_dir, ray_t, facenormal );
		}
	}

	return false;
}

bool SiegeLogicalMesh::HitTestTri( const vector_3& ray_orig, const vector_3& ray_dir,
								   float& ray_t, TriNorm& triangle )
{
	if( m_pBSPTree )
	{
		return m_pBSPTree->RayIntersectTreeTri( ray_orig, ray_dir, ray_t, triangle );
	}

	return false;
}

// Load from a file
bool SiegeLogicalMesh::Load( const char* &pData, const SiegeMeshHeader& header )
{
	// Get the identifier
	m_id		= *pData;
	pData		+= sizeof( unsigned char );

	// Get the bounding information
	m_minBox	= *((vector_3*)pData);
	pData		+= sizeof( vector_3 );

	m_maxBox	= *((vector_3*)pData);
	pData		+= sizeof( vector_3 );

	// Get the flags that describe this logical node
	m_flags		= *((unsigned int *)pData);
	pData		+= sizeof( unsigned int );

	// Get the leaf connection info
	unsigned int num_lconnections = *((unsigned int *)pData);
	pData		+= sizeof( unsigned int );

	// Create the leaf connection array
	m_numLeafConnections	= num_lconnections;
	m_pLeafConnectionInfo	= new LMESHLEAFINFO[ num_lconnections ];

	for( unsigned int lc = 0; lc < num_lconnections; ++lc )
	{
		// Get the leaf id
		unsigned short newid = *((unsigned short *)pData);
		pData += sizeof( unsigned short );

		// Get the correct pointer
		gpassert( newid < num_lconnections );
		LMESHLEAFINFO* pLeaf = &m_pLeafConnectionInfo[ newid ];
		pLeaf->id = newid;

		// Get the leaf bounding information
		memcpy( &pLeaf->minBox, pData, sizeof( vector_3 ) );
		pData += sizeof( vector_3 );

		memcpy( &pLeaf->maxBox, pData, sizeof( vector_3 ) );
		pData += sizeof( vector_3 );

		if( header.m_majorVersion > 6 ||
			(header.m_majorVersion == 6 && header.m_minorVersion >= 4) )
		{
			// Read center out directly
			memcpy( &pLeaf->center, pData, sizeof( vector_3 ) );
			pData += sizeof( vector_3 );
		}
		else
		{
			// Generate the center
			pLeaf->center	= pLeaf->minBox + ((pLeaf->maxBox - pLeaf->minBox) * 0.5f);
		}

		// Get the triangle index
		if( header.m_majorVersion > 6 ||
			(header.m_majorVersion == 6 && header.m_minorVersion >= 2) )
		{
			pLeaf->numTriangles	= *((unsigned short*)pData);
			pData	+= sizeof( unsigned short );
			gpassert( pLeaf->numTriangles );

			pLeaf->pTriangles	= new unsigned short[ pLeaf->numTriangles ];
			memcpy( pLeaf->pTriangles, pData, sizeof( unsigned short ) * pLeaf->numTriangles );
			pData	+= sizeof( unsigned short ) * pLeaf->numTriangles;
		}
		else
		{
			pLeaf->numTriangles		= 1;
			pLeaf->pTriangles		= new unsigned short;

			*(pLeaf->pTriangles)	= *((unsigned short *)pData);
			pData += sizeof( unsigned short );
		}

		// Get the local connections
		unsigned int num_localconnections = *((unsigned int *)pData);
		pData += sizeof( unsigned int );

		// Create the local connection array
		pLeaf->numLocalConnections	= num_localconnections;
		if( num_localconnections )
		{
			pLeaf->localConnections		= new LMESHLEAFINFO*[ num_localconnections ];

			for( unsigned int localc = 0; localc < num_localconnections; ++localc )
			{
				pLeaf->localConnections[ localc ] = &m_pLeafConnectionInfo[ *((unsigned short *)pData) ];
				pData += sizeof( unsigned short );
			}
		}
		else
		{
			pLeaf->localConnections		= NULL;
		}
	}

	m_numNodalConnections	= *((unsigned int *)pData);
	pData		+= sizeof( unsigned int );

	m_pNodalConnectionInfo	= new LCCOLLECTION[ m_numNodalConnections ];

	for( unsigned int nc = 0; nc < m_numNodalConnections; ++nc )
	{
		LCCOLLECTION* pCollection = &m_pNodalConnectionInfo[ nc ];

		// Unique identifier of node (corresponds to logical mesh id)
		pCollection->m_farid	= *pData;
		pData					+= sizeof( unsigned char );

		// Connections
		pCollection->m_numNodalLeafConnections	= *((unsigned int*)pData);
		pData					+= sizeof( unsigned int );

		pCollection->m_pNodalLeafConnections	= new NODALLEAFCONNECT[ pCollection->m_numNodalLeafConnections ];
		for( unsigned int nlc = 0; nlc < pCollection->m_numNodalLeafConnections; ++nlc )
		{
			pCollection->m_pNodalLeafConnections[ nlc ]	= *((NODALLEAFCONNECT*)pData);
			pData										+= sizeof( NODALLEAFCONNECT );
		}
	}

	// Read in the number of triangles
	unsigned int num_triangles = *((unsigned int *)pData);
	pData += sizeof( unsigned int );

	// Create a new empty BSPTree with just our triangle information
	m_pBSPTree	= new BSPTree( (TriNorm*)pData, (unsigned short)num_triangles, 0, 0, false );
	pData += (num_triangles * sizeof( TriNorm ));

	// Read in the tree
	ReadBSPNodeFromFile( pData, m_pBSPTree->GetRoot() );

	return true;
}

// Recursive function for reading BSP information from a file
void SiegeLogicalMesh::ReadBSPNodeFromFile( const char* &pData, BSPNode* pNode )
{
	// Write this node's bounding information to a file
	memcpy( &pNode->m_MinBound, pData, sizeof( vector_3 ) );
	pData += sizeof( vector_3 );

	memcpy( &pNode->m_MaxBound, pData, sizeof( vector_3 ) );
	pData += sizeof( vector_3 );

	// Write whether this is a leaf or not
	pNode->m_IsLeaf	= *((bool*)pData);
	pData += sizeof( bool );

	// Write triangle index information
	pNode->m_NumTriangles = *((unsigned short *)pData);
	pData += sizeof( unsigned short );

	// Create the triangle array
	pNode->m_Triangles = new unsigned short[ pNode->m_NumTriangles ];
	memcpy( pNode->m_Triangles, pData, (pNode->m_NumTriangles * sizeof( unsigned short )) );
	pData += (pNode->m_NumTriangles * sizeof( unsigned short ));

	// Write information describing the child structure of this node
	unsigned char children	= *pData;
	pData += sizeof( unsigned char );

	//Write left child
	if( children )
	{
		pNode->m_LeftChild	= new BSPNode;
		memset( pNode->m_LeftChild, 0, sizeof( BSPNode ) );
		pNode->m_RightChild = new BSPNode;
		memset( pNode->m_RightChild, 0, sizeof( BSPNode ) );

		ReadBSPNodeFromFile( pData, pNode->m_LeftChild );
		ReadBSPNodeFromFile( pData, pNode->m_RightChild );
	}
}