#pragma once

#ifndef _POSTPROCESS_H_
#define _POSTPROCESS_H_

/*********************************************************************************
**
**								PostProcess
**
**		Functional responsibility to reorganize triangle data for SNO meshes
**		as well as calculate redundant pathfinding information.
**
**	Author:		James Loe
**	Date:		July 6, 2000
**
*********************************************************************************/


const unsigned int SNOD_MAGIC			= 0x444F4E53;
const unsigned int SNOD_VERSION			= 7;
const unsigned int SNOD_MINOR_VERSION	= 0;

class BSPTree;


// UV storage
struct UV
{
	float u;
	float v;
};

class InMesh
{
public:

	// Construction and destruction
	InMesh();
	~InMesh();

	// Read in mesh from data pointer
	bool Read( char* pData );

	// Door storage
	struct PPDoorStorage
	{
		unsigned int	door_id;
		vector_3		center;
		vector_3		x_axis;
		vector_3		y_axis;
		vector_3		z_axis;
		int				num_verts;
		int*			pverts;
	};

	// Spot storage
	struct PPSpotStorage
	{
		vector_3		x_axis;
		vector_3		y_axis;
		vector_3		z_axis;
		vector_3		center;
		char*			name;
	};

	// Index collection storage
	struct VERTINDEXCOLLECTION
	{
		UINT32 VertexIndex;
		UINT32 TextureVertexIndex;
		UINT32 ColorIndex;
		UINT32 NormalIndex;
	};


	// Storage
	std::vector< PPDoorStorage >	m_DoorStorage;
	std::vector< PPSpotStorage >	m_SpotStorage;

	// Info
	vector_3						m_minBBox;
	vector_3						m_maxBBox;
	vector_3						m_CentroidOffset;

	unsigned int					m_ArraySize;
	unsigned int					m_IndexedSize;

	vector_3*						m_pVertices;
	vector_3*						m_pNormals;
	vector_3*						m_pColorVerts;
	UV*								m_pTextureVerts;
	unsigned short*					m_pIndices;
	bool							m_bTile;

	std::list< unsigned short >		m_FloorFaces;
	std::list< unsigned short >		m_WaterFaces;

	unsigned int					m_NumberOfTriangles;
	unsigned int					m_NumberOfTextureMaps;
	unsigned int					m_NumberOfMaterials;

	unsigned int*					m_pTexturedTriCount;
	unsigned int*					m_pFlatShadedTriCount;

	vector_3*						m_pMaterialDiffuseColour;

	std::vector< std::string >		m_Textures;
	std::list<std::string>			m_infoStrings;
};


class OutMesh
{
public:

	// Construction and destruction
	OutMesh();
	~OutMesh();

	// Build from an in mesh
	bool Build( InMesh& inMesh );

	// Write out the new mesh
	bool Write( InMesh& inMesh, FILE* pFile, DWORD& checksum );

	// Untransformed vertex with color and texture
	struct storeVertex
	{
		float x, y, z;
		float nx, ny, nz;
		DWORD color;
		UV	  uv;
	};

	struct sBuildTexStage
	{
		unsigned int tIndex;				// Which texture is active in this stage
		std::vector< storeVertex > verts;	// List of vertices for this stage
		std::vector< WORD > vIndex;			// Vertex indices for this stage
		std::vector< DWORD > lIndex;		// Lighting indices back into the main vertex array
		unsigned int numVerts;				// How many verts in list (size() is CPU intensive)
	};

	struct sTexStage
	{
		unsigned int tIndex;				// Which texture is active in this stage
		unsigned int startIndex;			// Where to start the indices in the VBuffer
		unsigned int numVerts;				// Number of vertices
		WORD*		 pVIndices;				// Pointer to the indices for this stage
		unsigned int numVIndices;			// Number of indices in this stage
		DWORD*		 pLIndices;				// Pointer to the lighting indices
		unsigned int numLIndices;			// Number of lighting indices
	};

	struct OutMeshHeader
	{
		// File identification
		unsigned int	m_id;
		unsigned int	m_majorVersion;
		unsigned int	m_minorVersion;

		// Door and spot information
		unsigned int	m_numDoors;
		unsigned int	m_numSpots;

		// Mesh information
		unsigned int	m_numVertices;
		unsigned int	m_numTriangles;

		// Stage information
		unsigned int	m_numStages;

		// Spatial information
		vector_3		m_minBBox;
		vector_3		m_maxBBox;
		vector_3		m_centroidOffset;

		// Whether or not this mesh requires wrapping
		bool			m_bTile;

		// Reserved information for possible future use
		unsigned int	m_reserved0;
		unsigned int	m_reserved1;
		unsigned int	m_reserved2;
	};

	typedef		std::vector< sTexStage >	TexStageList;
	typedef		TexStageList::iterator		TexStageListIter;



	// Vertex information
	int				m_numVertices;

	// Triangle information
	int				m_numTriangles;

	// Organized tex/tri list.
	TexStageList	m_TexStageList;

	// Vertex buffer
	storeVertex*	m_pVertices;
};

class PathAndBSP
{
public:

	// Construction and Destruction
	PathAndBSP();
	~PathAndBSP();

	// Build the path and BSP information from the mesh
	bool Build( InMesh& inMesh );

	// Write out path and BSP information to a file
	bool Write( FILE* pFile, DWORD& checksum );

	struct LEAFINFO
	{
		// Identifier
		unsigned short				id;

		// Bounding information
		vector_3					minBox;
		vector_3					maxBox;
		vector_3					center;

		// Triangle information (used to calc connections)
		std::list< unsigned short >	triangles;

		// Connection information for local leaves
		std::list< unsigned short >	localConnections;
	};

	struct NLEAFCONNECT
	{
		// Local leaf identifier
		unsigned short				local_id;

		// Far leaf identifier
		unsigned short				far_id;
	};

	struct LBUILDCOLLECT
	{
		// Unique identifier of node (corresponds to logical mesh id)
		unsigned char				m_farid;

		// List of leaf connections
		std::list< NLEAFCONNECT >	m_nodalLeafConnections;
	};

	struct LBUILDNODE
	{
		// Unique identifier
		unsigned char				id;

		// Bounding box of this logical node
		vector_3					minBox;
		vector_3					maxBox;

		// BSP'd geometrical information
		BSPTree*					pBSPTree;

		// Bit flags that define node attributes
		unsigned int				flags;

		// Leaf connections
		std::list< LEAFINFO >		leafConnectionInfo;

		// Local (but still nodal) connections
		std::list< LBUILDCOLLECT >	nodalConnectionInfo;
	};

	struct NODETRICOLL
	{
		vector_3					minBox;
		vector_3					maxBox;
		plane_3						triPlane;

		std::vector< unsigned short >	triangles;
	};

	typedef std::list< LBUILDNODE >								LogicalNodeList;
	typedef std::vector< std::list< BSPNode* > >				LogicalGroup;

private:

	// Build logical groups from the given geometry and index list
	LogicalGroup BuildLogicalGrouping( std::list< unsigned short >& indexList, InMesh& inMesh, BSPTree*& pTree );

	// Build's a leaf listing for the given node
	void BuildLeafList( LBUILDNODE& nNode );

	// Calculate the closest point, inside the box, that lies within the triangles of the
	// leaf.  Returns false if it cannot find any.
	int  FindClosestPointInTriangles( LBUILDNODE& nNode, NODETRICOLL& nTriColl, LEAFINFO& nLeaf );

	// Subdivides a leaf node and creates smaller leaves for use in the logical nodes
	void SubdivideLeaf( NODETRICOLL& nTriColl, LBUILDNODE& nNode, unsigned short& current_id );

	// Calculate the local connections of the given leaf
	void CalculateLocalConnections( LBUILDNODE& nNode, LEAFINFO& nLeaf );

	// Do these two leaves intersect?
	bool LogicalLeavesIntersect( LEAFINFO& leaf1, LBUILDNODE& node1, LEAFINFO& leaf2, LBUILDNODE& node2 );

	// Connect two leafs together
	void ConnectLeafs( LEAFINFO& leaf1, LEAFINFO& leaf2 );

	// Calculate the internodal connections
	void CalculateLogicalNodeConnections( LBUILDNODE* pnNode, LBUILDNODE* pfNode );

	// Connect two logical nodes together
	void ConnectLogicalNodes( LBUILDNODE* pnNode, LBUILDCOLLECT* &pnNodeConnect, LBUILDNODE* pfNode, LBUILDCOLLECT* &pfNodeConnect );

	// Connect two nodal leafs
	void ConnectNodalLeafs( LBUILDCOLLECT* pnNodeConnect, unsigned short nID, LBUILDCOLLECT* pfNodeConnect, unsigned short fID );

	// Recursive function for writing a BSP tree to a file
	void WriteBSPNodeToFile( FILE* pFile, BSPNode* pNode, DWORD& checksum );


	// Storage of our logical nodes
	LogicalNodeList m_logicalList;

};




class PostProcess
{
public:

	// Construction and Destruction
	PostProcess();
	~PostProcess();

	// The only accessible function from the outside, it's all you need
	bool OrgAndCalcInfo( const char* filename );

private:

	// Mesh being read in
	InMesh		m_inMesh;

	// Mesh being stored out
	OutMesh		m_outMesh;

	// Path and BSP information
	PathAndBSP	m_pathAndBSP;
};


#endif