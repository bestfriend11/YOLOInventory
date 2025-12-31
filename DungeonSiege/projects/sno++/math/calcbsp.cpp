/***********************************************************************
**
**						Siege BSP support
**
***********************************************************************/

// Container includes
#include <vector>

// Math library includes
#include "vector_3.h"
#include "triangle_3.h"
#include "space_3.h"

// Class definition include
#include "calcbsp.h"

#include "assert.h"


// Construct a BSPTree object and generate the tree itself from the lists
// of data.
BSPTree::BSPTree( const vector_3 *vertices, const unsigned int numvertices,
				  const unsigned short *indices, const unsigned short numtriangles,
				  const unsigned int maxprimitives, const unsigned int maxdepth )
				  : m_Error( false )
				  , m_MaxPrimitives( maxprimitives )
				  , m_MaxDepth( maxdepth )
				  , m_numTriangles( 0 )
{
	// Setup the TriNorm list
	InitTriNorm( vertices, numvertices, indices, numtriangles );

	// Initialize the tree by creating the root node
	m_BSPRoot					= new BSPNode;
	m_BSPRoot->m_IsLeaf			= false;
	m_BSPRoot->m_LeftChild		= NULL;
	m_BSPRoot->m_RightChild		= NULL;

	// Initialize the triangles for the entire tree
	m_BSPRoot->m_Triangles		= new unsigned short[ numtriangles ];
	for( unsigned short i = 0; i < numtriangles; ++i )
	{
		m_BSPRoot->m_Triangles[i]	= i;
	}
	m_BSPRoot->m_NumTriangles	= numtriangles;

	// Initialize the bounding volume of the entire tree
	GetBounds( m_BSPRoot->m_Triangles, m_BSPRoot->m_NumTriangles, 
			   m_BSPRoot->m_MinBound, m_BSPRoot->m_MaxBound );
	
	// Generate the tree
	GenerateTreeNode( m_BSPRoot, 0, GetAxisOfDivision( m_BSPRoot ) );
}

BSPTree::BSPTree( const TriNorm* triNorms, const unsigned short numtrinorms,
				  const unsigned int maxprimitives, const unsigned int maxdepth, bool bGenerateTree )
				  : m_Error( false )
				  , m_MaxPrimitives( maxprimitives )
				  , m_MaxDepth( maxdepth )
{
	// Create the new TriNorm list
	m_Triangles		= new TriNorm[ numtrinorms ];
	m_numTriangles	= numtrinorms;

	// Copy the lists
	memcpy( m_Triangles, triNorms, sizeof( TriNorm ) * numtrinorms );

	// Initialize the tree by creating the root node
	m_BSPRoot					= new BSPNode;
	memset( m_BSPRoot, 0, sizeof( BSPNode ) );

	m_BSPRoot->m_IsLeaf			= false;
	m_BSPRoot->m_LeftChild		= NULL;
	m_BSPRoot->m_RightChild		= NULL;

	if( bGenerateTree )
	{
		// Initialize the triangles for the entire tree
		m_BSPRoot->m_Triangles		= new unsigned short[ numtrinorms ];
		for( unsigned short i = 0; i < numtrinorms; ++i )
		{
			m_BSPRoot->m_Triangles[i]	= i;
		}
		m_BSPRoot->m_NumTriangles	= numtrinorms;

		// Initialize the bounding volume of the entire tree
		GetBounds( m_BSPRoot->m_Triangles, m_BSPRoot->m_NumTriangles, 
				   m_BSPRoot->m_MinBound, m_BSPRoot->m_MaxBound );
		
		// Generate the tree
		GenerateTreeNode( m_BSPRoot, 0, GetAxisOfDivision( m_BSPRoot ) );
	}
}

// Destroy the tree
BSPTree::~BSPTree()
{
	// Destroy the TriNorms
	if( m_Triangles )
	{
		delete[] m_Triangles;
	}

	// Destroy the tree
	if( m_BSPRoot )
	{
		DestroyNode( m_BSPRoot );
		delete m_BSPRoot;
	}
}

// Trace a ray into the tree
bool BSPTree::RayIntersectTree( const vector_3& ray_orig, const vector_3& ray_dir,
							    float& ray_t, vector_3& facenormal )
{
	// In order to avoid recursive function calls, I implemented this function
	// using an explicit stack.
	std::vector< IntersectTestNode > nodeStack;
	nodeStack.reserve( m_MaxDepth );

	// Initialize t factor
	ray_t	= RealMaximum;

	// First, push the root onto the stack
	assert( m_BSPRoot );
	assert( m_BSPRoot->m_NumTriangles );

	IntersectTestNode	root;
	root.origin			= ray_orig;
	root.node			= m_BSPRoot;
	nodeStack.push_back( root );

	// Run the intersection testing while there is something to test
	while( !nodeStack.empty() )
	{
		// Get the next node to test
		IntersectTestNode nNode		= nodeStack.back();
		BSPNode* pNode				= nNode.node;
		nodeStack.pop_back();

		// Only test nodes that actually have triangle data will get inserted into the list
		vector_3 coord;
		if( RayIntersectsBox( pNode->m_MinBound, pNode->m_MaxBound, nNode.origin, ray_dir, coord ) )
		{
			if( pNode->m_IsLeaf )
			{
				// Do triangle hit testing with this node
				float TCoord, UCoord, VCoord;
				for( unsigned short i = 0; i < pNode->m_NumTriangles; ++i )
				{
					TriNorm& tri	= m_Triangles[ pNode->m_Triangles[ i ] ];
					if( math::RayIntersectsTriangle( ray_orig, ray_dir, 
													 tri.m_Vertices[0], 
													 tri.m_Vertices[1],
													 tri.m_Vertices[2], 
													 TCoord, UCoord, VCoord ) )
					{
						if( (TCoord < ray_t) && (TCoord >= 0.0) )
						{
							ray_t		= TCoord;
							facenormal	= tri.m_Normal;
						}
					}
				}
			}
			else
			{
				// Only nodes with children will ever get here
				IntersectTestNode	nNode;
				nNode.origin		= coord;

				// Check right child
				if( pNode->m_RightChild->m_NumTriangles )
				{
					nNode.node	= pNode->m_RightChild;
					nodeStack.push_back( nNode );
				}
				// Check left child
				if( pNode->m_LeftChild->m_NumTriangles )
				{
					nNode.node	= pNode->m_LeftChild;
					nodeStack.push_back( nNode );
				}
			}
		}
	}

	if( ray_t != RealMaximum )
	{
		return true;
	}
	return false;
}

// Find the triangles enclosed by given axis-aligned volume
void BSPTree::BoxIntersectTree( const vector_3& minBox, const vector_3& maxBox, 
								std::vector< unsigned short >& triangleIndices )
{
	// In order to avoid recursive function calls, I implemented this function
	// using an explicit stack.
	std::vector< BSPNode* > nodeStack;
	nodeStack.reserve( m_MaxDepth );

	// First, push the root onto the stack
	assert( m_BSPRoot );
	assert( m_BSPRoot->m_NumTriangles );

	nodeStack.push_back( m_BSPRoot );

	// Run the intersection testing while there is something to test
	while( !nodeStack.empty() )
	{
		// Get the next node to test
		BSPNode* pNode		= nodeStack.back();
		nodeStack.pop_back();

		// Only test nodes that actually have triangle data will get inserted into the list
		vector_3 coord;
		if( BoxIntersectsBox( pNode->m_MinBound, pNode->m_MaxBound, minBox, maxBox ) )
		{
			if( pNode->m_IsLeaf )
			{
				// Push all of this leaf's triangles on the list
				triangleIndices.insert( triangleIndices.end(), pNode->m_Triangles, &pNode->m_Triangles[pNode->m_NumTriangles] );
			}
			else
			{
				// Only nodes with children will ever get here

				// Check right child
				if( pNode->m_RightChild->m_NumTriangles )
				{
					nodeStack.push_back( pNode->m_RightChild );
				}
				// Check left child
				if( pNode->m_LeftChild->m_NumTriangles )
				{
					nodeStack.push_back( pNode->m_LeftChild );
				}
			}
		}
	}
}

// Recursively generate the tree
void BSPTree::GenerateTreeNode( BSPNode* current_node, unsigned int current_depth, 
							    DIVAXIS current_div )
{
	assert( current_node );

	// Determine if this node needs to be split
	if( (current_node->m_NumTriangles > m_MaxPrimitives) && 
		((int)current_depth <= m_MaxDepth) )
	{
		// Prepare for classification
		current_node->m_LeftChild					= new BSPNode;
		current_node->m_RightChild					= new BSPNode;

		current_node->m_LeftChild->m_IsLeaf			= false;
		current_node->m_LeftChild->m_MinBound		= current_node->m_MinBound;
		current_node->m_LeftChild->m_MaxBound		= current_node->m_MaxBound;
		current_node->m_LeftChild->m_LeftChild		= NULL;
		current_node->m_LeftChild->m_RightChild		= NULL;

		*(current_node->m_RightChild)				= *(current_node->m_LeftChild);

		// Do axis subdivision
		SetupChildBoxes( current_node, current_div, current_node->m_LeftChild->m_MaxBound, current_node->m_RightChild->m_MinBound );

		// Iterate through triangles and classify them
		std::vector< unsigned short > lefttris;
		std::vector< unsigned short > righttris;
		for( unsigned short i = 0; i < current_node->m_NumTriangles; ++i )
		{
			if( TriIntersectsBox( current_node->m_LeftChild->m_MinBound,
								  current_node->m_LeftChild->m_MaxBound,
								  current_node->m_Triangles[i] ) )
			{
				// This triangle is in the left child
				lefttris.push_back( current_node->m_Triangles[i] );
			}
			else if( TriIntersectsBox( current_node->m_RightChild->m_MinBound,
								  current_node->m_RightChild->m_MaxBound,
								  current_node->m_Triangles[i] ) )
			{
				// This triangle is in the right child
				righttris.push_back( current_node->m_Triangles[i] );
			}
		}

		// Check to make sure that we actually seperated some triangles
		if( current_node->m_NumTriangles == lefttris.size() ||
			current_node->m_NumTriangles == righttris.size() )
		{
			delete current_node->m_LeftChild;
			current_node->m_LeftChild	= NULL;
			delete current_node->m_RightChild;
			current_node->m_RightChild	= NULL;

			// No division actually occured
			current_node->m_IsLeaf	= true;
		}
		else
		{
			// Setup the triangles for the left child
			current_node->m_LeftChild->m_NumTriangles	= (unsigned short)lefttris.size();
			if( current_node->m_LeftChild->m_NumTriangles )
			{
				// Build list
				current_node->m_LeftChild->m_Triangles		= new unsigned short[ lefttris.size() ];
				for( i = 0; i < current_node->m_LeftChild->m_NumTriangles; ++i )
				{
					current_node->m_LeftChild->m_Triangles[i]	= lefttris[i];
				}

				// Get accurate boundary
				GetBounds( current_node->m_LeftChild->m_Triangles,
						   current_node->m_LeftChild->m_NumTriangles,
						   current_node->m_LeftChild->m_MinBound,
						   current_node->m_LeftChild->m_MaxBound );
			}
			else
			{
				current_node->m_LeftChild->m_Triangles	= NULL;
			}

			// Setup the triangles for the right child
			current_node->m_RightChild->m_NumTriangles	= (unsigned short)righttris.size();
			if( current_node->m_RightChild->m_NumTriangles )
			{
				// Build list
				current_node->m_RightChild->m_Triangles		= new unsigned short[ righttris.size() ];
				for( i = 0; i < current_node->m_RightChild->m_NumTriangles; ++i )
				{
					current_node->m_RightChild->m_Triangles[i]	= righttris[i];
				}
				
				// Get accurate boundary
				GetBounds( current_node->m_RightChild->m_Triangles,
						   current_node->m_RightChild->m_NumTriangles,
						   current_node->m_RightChild->m_MinBound,
						   current_node->m_RightChild->m_MaxBound );
				
			}
			else
			{
				current_node->m_RightChild->m_Triangles	= NULL;
			}

			// Recurse
			GenerateTreeNode( current_node->m_LeftChild, current_depth+1, 
							  GetAxisOfDivision( current_node->m_LeftChild ) );
			GenerateTreeNode( current_node->m_RightChild, current_depth+1, 
							  GetAxisOfDivision( current_node->m_RightChild ) );
		}
	}
	else
	{
		// We are at the end of the line, so we have a leaf
		current_node->m_IsLeaf	= true;
	}
}

// Setup the boxes that define the children of a node
void BSPTree::SetupChildBoxes( const BSPNode* current_node, const DIVAXIS div, vector_3& maxBound, vector_3& minBound )
{
	float cutting_plane = 0.0f;

	switch( div )
	{
	case DIV_XAXIS_HALF:
		cutting_plane	= current_node->m_MinBound.x + ((current_node->m_MaxBound.x - current_node->m_MinBound.x)*0.5f);
		maxBound.x = cutting_plane;
		minBound.x = cutting_plane;
		break;

	case DIV_XAXIS_QUARTER_BOTTOM:
		cutting_plane	= current_node->m_MinBound.x + ((current_node->m_MaxBound.x - current_node->m_MinBound.x)*0.25f);
		maxBound.x = cutting_plane;
		minBound.x = cutting_plane;
		break;

	case DIV_XAXIS_QUARTER_TOP:
		cutting_plane	= current_node->m_MinBound.x + ((current_node->m_MaxBound.x - current_node->m_MinBound.x)*0.75f);
		maxBound.x = cutting_plane;
		minBound.x = cutting_plane;
		break;

	case DIV_YAXIS_HALF:
		cutting_plane	= current_node->m_MinBound.y + ((current_node->m_MaxBound.y - current_node->m_MinBound.y)*0.5f);
		maxBound.y = cutting_plane;
		minBound.y = cutting_plane;
		break;

	case DIV_YAXIS_QUARTER_BOTTOM:
		cutting_plane	= current_node->m_MinBound.y + ((current_node->m_MaxBound.y - current_node->m_MinBound.y)*0.25f);
		maxBound.y = cutting_plane;
		minBound.y = cutting_plane;
		break;

	case DIV_YAXIS_QUARTER_TOP:
		cutting_plane	= current_node->m_MinBound.y + ((current_node->m_MaxBound.y - current_node->m_MinBound.y)*0.75f);
		maxBound.y = cutting_plane;
		minBound.y = cutting_plane;
		break;

	case DIV_ZAXIS_HALF:
		cutting_plane	= current_node->m_MinBound.z + ((current_node->m_MaxBound.z - current_node->m_MinBound.z)*0.5f);
		maxBound.z = cutting_plane;
		minBound.z = cutting_plane;
		break;

	case DIV_ZAXIS_QUARTER_BOTTOM:
		cutting_plane	= current_node->m_MinBound.z + ((current_node->m_MaxBound.z - current_node->m_MinBound.z)*0.25f);
		maxBound.z = cutting_plane;
		minBound.z = cutting_plane;
		break;

	case DIV_ZAXIS_QUARTER_TOP:
		cutting_plane	= current_node->m_MinBound.z + ((current_node->m_MaxBound.z - current_node->m_MinBound.z)*0.75f);
		maxBound.z = cutting_plane;
		minBound.z = cutting_plane;
		break;
	}
}

// Determine if any part of a triangle intersects a bounding volume
bool BSPTree::TriIntersectsBox( const vector_3& minBound, const vector_3& maxBound, const unsigned short tri )
{
	vector_3& point0	= m_Triangles[ tri ].m_Vertices[0];
	vector_3& point1	= m_Triangles[ tri ].m_Vertices[1];
	vector_3& point2	= m_Triangles[ tri ].m_Vertices[2];

	// Do trivial acceptance testing
	if( PointInBox( minBound, maxBound, point0 ) ){
		return true;
	}
	if( PointInBox( minBound, maxBound, point1 ) ){
		return true;
	}
	if( PointInBox( minBound, maxBound, point2 ) ){
		return true;
	}

	// Do trivial rejection testing
	if( (point0.x < minBound.x) && (point1.x < minBound.x) && (point2.x < minBound.x) ){
		return false;
	}
	if( (point0.x > maxBound.x) && (point1.x > maxBound.x) && (point2.x > maxBound.x) ){
		return false;
	}
	if( (point0.y < minBound.y) && (point1.y < minBound.y) && (point2.y < minBound.y) ){
		return false;
	}
	if( (point0.y > maxBound.y) && (point1.y > maxBound.y) && (point2.y > maxBound.y) ){
		return false;
	}
	if( (point0.z < minBound.z) && (point1.z < minBound.z) && (point2.z < minBound.z) ){
		return false;
	}
	if( (point0.z > maxBound.z) && (point1.z > maxBound.z) && (point2.z > maxBound.z) ){
		return false;
	}

	// Do edge test
	vector_3 coord;

	if( RayIntersectsBox( minBound, maxBound, point0, point1 - point0, coord ) ){
		return true;
	}
	if( RayIntersectsBox( minBound, maxBound, point1, point2 - point1, coord ) ){
		return true;
	}
	if( RayIntersectsBox( minBound, maxBound, point2, point0 - point2, coord ) ){
		return true;
	}

	real TCoord, TCoord2;	// For sign and magnitude testing
	real UCoord,VCoord;

	// Run a single vertical test to catch planar tris
	if( math::RayIntersectsTriangle( minBound, vector_3( 0.0f, (maxBound.y - minBound.y) + FLOAT_TOLERANCE, 0.0f ),
									 point0, point1, point2,
									 TCoord, UCoord, VCoord ) )
	{
		if( IsPositive( TCoord ) && (TCoord <= (1.0f + FLOAT_TOLERANCE)) )
		{
			return true;
		}
	}

	// Do box test to catch the corner poking through center of triangle
	if( math::RayIntersectsTriangle( minBound, vector_3( 1, 0, 0 ), 
									 point0, point1, point2, TCoord, UCoord, VCoord ) &&
		math::RayIntersectsTriangle( vector_3( maxBound.x, minBound.y, minBound.z ), vector_3( -1, 0, 0 ),
									 point0, point1, point2, TCoord2, UCoord, VCoord ) )
	{
		if( IsPositive( TCoord ) && IsPositive( TCoord2 ) )
		{
			// If the sign of both T coordinates is positive, then we hit the triangle
			return true;
		}
	}
	if( math::RayIntersectsTriangle( vector_3( minBound.x, maxBound.y, minBound.z ), vector_3( 1, 0, 0 ), 
									 point0, point1, point2, TCoord, UCoord, VCoord ) &&
		math::RayIntersectsTriangle( vector_3( maxBound.x, maxBound.y, minBound.z ), vector_3( -1, 0, 0 ),
									 point0, point1, point2, TCoord2, UCoord, VCoord ) )
	{
		if( IsPositive( TCoord ) && IsPositive( TCoord2 ) )
		{
			// If the sign of both T coordinates is positive, then we hit the triangle
			return true;
		}
	}
	if( math::RayIntersectsTriangle( vector_3( minBound.x, minBound.y, maxBound.z ), vector_3( 1, 0, 0 ), 
									 point0, point1, point2, TCoord, UCoord, VCoord ) &&
		math::RayIntersectsTriangle( vector_3( maxBound.x, minBound.y, maxBound.z ), vector_3( -1, 0, 0 ),
									 point0, point1, point2, TCoord2, UCoord, VCoord ) )
	{
		if( IsPositive( TCoord ) && IsPositive( TCoord2 ) )
		{
			// If the sign of both T coordinates is positive, then we hit the triangle
			return true;
		}
	}

	if( math::RayIntersectsTriangle( vector_3( minBound.x, maxBound.y, maxBound.z ), vector_3( 1, 0, 0 ), 
									 point0, point1, point2, TCoord, UCoord, VCoord ) &&
		math::RayIntersectsTriangle( maxBound, vector_3( -1, 0, 0 ),
									 point0, point1, point2, TCoord2, UCoord, VCoord ) )
	{
		if( IsPositive( TCoord ) && IsPositive( TCoord2 ) )
		{
			// If the sign of both T coordinates is positive, then we hit the triangle
			return true;
		}
	}

	// Failure
	return false;
}

// Get the bounds of a set of vertices
void BSPTree::GetBounds( const unsigned short* indices, const unsigned short numtriangles,
						 vector_3& minbound, vector_3& maxbound )
{
	assert( indices && numtriangles );

	// Initialize the bounds
	minbound = maxbound = m_Triangles[(*indices)].m_Vertices[0];

	// Calculate the bounds from the verts
	for( unsigned short i = 0; i < numtriangles; ++i, ++indices )
	{
		TriNorm& tri = m_Triangles[(*indices)];

		for( unsigned short o = 0; o < 3; ++o )
		{
			vector_3& vert = tri.m_Vertices[o];

			minbound.x = min( minbound.x, vert.x );
			minbound.y = min( minbound.y, vert.y );
			minbound.z = min( minbound.z, vert.z );

			maxbound.x = max( maxbound.x, vert.x );
			maxbound.y = max( maxbound.y, vert.y );
			maxbound.z = max( maxbound.z, vert.z );
		}
	}
}

// Decide what axis this box should be split down next
DIVAXIS BSPTree::GetAxisOfDivision( const BSPNode* node )
{
	// Prime split axis is the one that gives us the most even seperation of geometry
	// In order to facilitate this, we test all three axes, then choose the one that
	// is closest to our desired result.
	DIVAXIS next_div	= DIV_XAXIS_HALF;
	int div_diff		= TestAxisOfDivision( node, DIV_XAXIS_HALF );

	int test_diff		= TestAxisOfDivision( node, DIV_XAXIS_QUARTER_BOTTOM );
	if( test_diff < div_diff )
	{
		div_diff = test_diff;
		next_div = DIV_XAXIS_QUARTER_BOTTOM;
	}

	test_diff			= TestAxisOfDivision( node, DIV_XAXIS_QUARTER_TOP );
	if( test_diff < div_diff )
	{
		div_diff = test_diff;
		next_div = DIV_XAXIS_QUARTER_TOP;
	}

	test_diff			= TestAxisOfDivision( node, DIV_YAXIS_HALF );
	if( test_diff < div_diff )
	{
		div_diff = test_diff;
		next_div = DIV_YAXIS_HALF;
	}

	test_diff			= TestAxisOfDivision( node, DIV_YAXIS_QUARTER_BOTTOM );
	if( test_diff < div_diff )
	{
		div_diff = test_diff;
		next_div = DIV_YAXIS_QUARTER_BOTTOM;
	}

	test_diff			= TestAxisOfDivision( node, DIV_YAXIS_QUARTER_TOP );
	if( test_diff < div_diff )
	{
		div_diff = test_diff;
		next_div = DIV_YAXIS_QUARTER_TOP;
	}

	test_diff			= TestAxisOfDivision( node, DIV_ZAXIS_HALF );
	if( test_diff < div_diff )
	{
		div_diff = test_diff;
		next_div = DIV_ZAXIS_HALF;
	}

	test_diff			= TestAxisOfDivision( node, DIV_ZAXIS_QUARTER_BOTTOM );
	if( test_diff < div_diff )
	{
		div_diff = test_diff;
		next_div = DIV_ZAXIS_QUARTER_BOTTOM;
	}

	test_diff			= TestAxisOfDivision( node, DIV_ZAXIS_QUARTER_TOP );
	if( test_diff < div_diff )
	{
		div_diff = test_diff;
		next_div = DIV_ZAXIS_QUARTER_TOP;
	}

	return next_div;
}

// Test an axis for possible split
int BSPTree::TestAxisOfDivision( const BSPNode* node, const DIVAXIS div )
{
	// Bounds
	vector_3 rminBound	= node->m_MinBound;
	vector_3 lmaxBound	= node->m_MaxBound;

	// Do axis subdivision
	SetupChildBoxes( node, div, lmaxBound, rminBound );

	// Iterate through triangles and classify them
	std::vector< unsigned int > lefttris;
	std::vector< unsigned int > righttris;
	for( int i = 0; i < node->m_NumTriangles; ++i )
	{
		if( TriIntersectsBox( node->m_MinBound,
							  lmaxBound,
							  node->m_Triangles[i] ) )
		{
			// This triangle is in the left child
			lefttris.push_back( node->m_Triangles[i] );
		}
		else if( TriIntersectsBox( rminBound,
								   node->m_MaxBound,
								   node->m_Triangles[i] ) )
		{
			// This triangle is in the right child
			righttris.push_back( node->m_Triangles[i] );
		}
	}

	return ( abs( lefttris.size() - righttris.size() ) );
}

// Init the triangle listing
void BSPTree::InitTriNorm( const vector_3* vertices, const unsigned int numvertices,
						   const unsigned short* indices, const unsigned short numtriangles )
{
	assert( vertices && numvertices );
	assert( indices  && numtriangles );

	// Initialize the triangle list
	m_Triangles		= new TriNorm[ numtriangles ];
	m_numTriangles	= numtriangles;

	// Fill in each triangle
	for( unsigned short i = 0; i < numtriangles; ++i )
	{
		m_Triangles[i].m_Vertices[0]	= vertices[ *(indices++) ];
		m_Triangles[i].m_Vertices[1]	= vertices[ *(indices++) ];
		m_Triangles[i].m_Vertices[2]	= vertices[ *(indices++) ];

		vector_3 PreNormal				= CrossProduct( m_Triangles[i].m_Vertices[1] -
													    m_Triangles[i].m_Vertices[0],
													    m_Triangles[i].m_Vertices[2] -
													    m_Triangles[i].m_Vertices[0] );
		if( IsZero( PreNormal ) )
		{
			m_Error						= true;
			m_Triangles[i].m_Normal		= vector_3( 0, 0, -1 );
		}
		else
		{
			m_Triangles[i].m_Normal		= Normalize( PreNormal );
		}
	}
}

// Destroy a node and all of its children
void BSPTree::DestroyNode( BSPNode* node )
{
	assert( node );

	if( node )
	{
		if( node->m_LeftChild )
		{
			DestroyNode( node->m_LeftChild );
			delete node->m_LeftChild;
		}
		if( node->m_RightChild )
		{
			DestroyNode( node->m_RightChild );
			delete node->m_RightChild;
		}

		if( node->m_Triangles )
		{
			delete[] node->m_Triangles;
		}
	}
}

