#pragma once
#ifndef _SIEGE_BSP_
#define _SIEGE_BSP_

/***********************************************************************
**
**						Siege BSP support
**
**	This file defines support functionality for binary space partitioning
**	within the Siege engine.
**
**  I chose a rather simple method of BSP for this implementation, as
**  outlined by Kelvin Sung and Peter Shirley in GG3.  This method
**  consists of a recursive even subdivision of axis-aligned bounding
**  volumes, with the classification of individual polygons lying solely
**  in the leaf nodes.  This method is very efficient when using a BSP
**  tree to facilitate quick ray casting tests, which is the primary
**  goal of this structure within the Siege engine.
**
**  Author:			James Loe
**  Date:			09/23/99
**
**
***********************************************************************/

#include <vector>

// Enumerated type defines axis of division
enum DIVAXIS
{
	DIV_XAXIS_HALF				= 0,	// Divide along the X-axis half
	DIV_XAXIS_QUARTER_BOTTOM	= 1,	// Divide along the X-axis quarter bottom
	DIV_XAXIS_QUARTER_TOP		= 2,	// Divide along the X-axis quarter top

	DIV_YAXIS_HALF				= 3,	// Divide along the Y-axis half
	DIV_YAXIS_QUARTER_BOTTOM	= 4,	// Divide along the Y-axis quarter bottom
	DIV_YAXIS_QUARTER_TOP		= 5,	// Divide along the Y-axis quarter top

	DIV_ZAXIS_HALF				= 6,	// Divide along the Z-axis half
	DIV_ZAXIS_QUARTER_BOTTOM	= 7,	// Divide along the Z-axis quarter bottom
	DIV_ZAXIS_QUARTER_TOP		= 8,	// Divide along the Z-axis quarter top

	DIV_ALIGN					= 0x7FFFFFFF,	// DWORD align this enumerated type for speed
};

// Definition of a triangle/normal pair
struct TriNorm
{
	vector_3		m_Vertices[3];		// List of triangle vertices
	vector_3		m_Normal;			// Face normal of this triangle
};

// Definition of a BSP node
struct BSPNode
{
	vector_3		m_MinBound;			// Minimum bound of node
	vector_3		m_MaxBound;			// Maximum bound of node

	bool			m_IsLeaf;			// Is this node a leaf node?
	unsigned short*	m_Triangles;		// List of triangles in this node (index into main list)
	unsigned short	m_NumTriangles;		// Number of triangles in this node

	BSPNode*		m_LeftChild;		// Pointer to left child node
	BSPNode*		m_RightChild;		// Pointer to right child node
};

// Definition of a ray trace pair
struct IntersectTestNode
{
	vector_3		origin;				// Ray origin for this iteration of tracing
	BSPNode*		node;				// Node to trace into
};

// Class definition of a BSP tree
class BSPTree
{
public:

	// The constructor handles the generation of the tree
	BSPTree( const vector_3* vertices, const unsigned int numvertices,
			 const unsigned short* indices, const unsigned short numtriangles,
			 const unsigned int maxprimitives = 8, const unsigned int maxdepth = 16 );
	BSPTree( const TriNorm* triNorms, const unsigned short numtrinorms,
			 const unsigned int maxprimitives = 8, const unsigned int maxdepth = 16,
			 bool bGenerateTree = true );

	~BSPTree();

	// Trace a ray into the tree
	bool		RayIntersectTree( const vector_3& ray_orig, const vector_3& ray_dir,
								  float& ray_t, vector_3& facenormal );

	// Find the triangles enclosed by given axis-aligned volume
	void		BoxIntersectTree( const vector_3& minBox, const vector_3& maxBox, 
								  std::vector< unsigned short >& triangleIndices );

	// Determine if any part of a triangle intersects a bounding volume
	bool		TriIntersectsBox( const vector_3& minBound, const vector_3& maxBound, const unsigned short tri );

	// See if an error has occured
	bool		HasError()					{ return m_Error; }

	// Get the root of the tree /* BE CAREFUL WITH IT! */
	BSPNode*	GetRoot()					{ return m_BSPRoot; }

	// Get the triangles that compose this tree
	TriNorm*	GetTriangles()				{ return m_Triangles; }
	int			GetNumTriangles()			{ return m_numTriangles; }

private:

	// Recursive tree generation
	void		GenerateTreeNode( BSPNode* current_node, unsigned int current_depth,
								  DIVAXIS current_div );

	// Get the minimum and maximum bounds of a set of vertices
	void		GetBounds( const unsigned short* indices, const unsigned short numtriangles,
						   vector_3& minbound, vector_3& maxbound );

	// Decide what axis this box should be split down next
	DIVAXIS		GetAxisOfDivision( const BSPNode* node );

	// Test an axis for possible splitting
	int			TestAxisOfDivision( const BSPNode* node, const DIVAXIS div );

	// Initialize the triangle list from the raw data
	void		InitTriNorm( const vector_3* vertices, const unsigned int numvertices,
							 const unsigned short* indices, const unsigned short numtriangles );

	// Setup the boxes that define the children of a node
	void		SetupChildBoxes( const BSPNode* current_node, const DIVAXIS div, vector_3& maxBound, vector_3& minBound );

	// Destroy a node and all its children
	void		DestroyNode( BSPNode* node );

	// The actual tree information
	BSPNode*	m_BSPRoot;

	// The geometric information
	TriNorm*	m_Triangles;
	int			m_numTriangles;

	// Balance/complexity information for this tree
	int			m_MaxPrimitives;
	int			m_MaxDepth;

	// Signifies whether or not an error has occured in the BSP
	bool		m_Error;

};
	

#endif
