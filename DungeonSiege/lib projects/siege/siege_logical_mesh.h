#pragma once

#ifndef _SIEGE_LOGICAL_MESH_H_
#define _SIEGE_LOGICAL_MESH_H_

/*********************************************************************************
**
**							SiegeLogicalMesh
**
**		This class defines the storage and functionality of a logical mesh, which
**		interacts with a logical node to provide total logical control to
**		clients.
**
**		Author:		James Loe
**		Date:		07/12/00
**
**********************************************************************************/


class BSPTree;
struct BSPNode;
struct TriNorm;


namespace siege
{
	// Forward dec
	struct LMESHLEAFINFO;
	struct LCCOLLECTION;
	class SiegeLogicalNode;

	// Leaf search structure, used for pathfinding
	struct LMESHSEARCHLEAF
	{
		// Identifier of this leaf
		SiegeLogicalNode*			pLogicalNode;
		LMESHLEAFINFO*				pLeafInfo;

		// Search information
		float						costToStart;
		float						costToFinish;
		float						combinedCost;

		DWORD						parent;
		bool						bInOpen;
	};

	// Leaf information
	struct LMESHLEAFINFO
	{
		// Identifier
		unsigned short				id;

		// Bounding information
		vector_3					minBox;
		vector_3					maxBox;
		vector_3					center;

		// Triangle information (used to calc connections)
		unsigned short				numTriangles;
		unsigned short*				pTriangles;

		// Connection information for local leaves
		unsigned int				numLocalConnections;
		LMESHLEAFINFO**				localConnections;
	};

	class SiegeLogicalMesh
	{
		friend class SiegeMesh;

	public:

		// Destruction
		~SiegeLogicalMesh();

		// Trace a ray into this logical mesh
		bool HitTest( const vector_3& ray_orig, const vector_3& ray_dir,
					  float& ray_t, vector_3& facenormal );

		bool HitTestTri( const vector_3& ray_orig, const vector_3& ray_dir,
						 float& ray_t, TriNorm& triangle );

		// Get ID
		unsigned char	GetID()					{ return m_id; }

		// Get flags
		unsigned int	GetFlags()				{ return m_flags; }

		// Get the BSPTree for this logical mesh
		BSPTree*		GetBSPTree()			{ return m_pBSPTree; }

		// Get bounding information
		const vector_3&	GetMinimumBounds()		{ return m_minBox; }
		const vector_3&	GetMaximumBounds()		{ return m_maxBox; }
		const vector_3	GetHalfDiag()			{ return (m_maxBox - m_minBox) * 0.5f; }
		const vector_3	GetCenter()				{ return m_minBox + ((m_maxBox - m_minBox) * 0.5f); }

		// Get leaf connection info
		unsigned int	GetNumLeafConnections()	{ return m_numLeafConnections; }
		LMESHLEAFINFO*	GetLeafConnectionInfo()	{ return m_pLeafConnectionInfo; }

		// Get node connection info
		unsigned int	GetNumNodalConnections(){ return m_numNodalConnections; }
		LCCOLLECTION*	GetNodalConnectionInfo(){ return m_pNodalConnectionInfo; }

	private:

		// Construction
		SiegeLogicalMesh();

		// Load from file
		bool Load( const char* &pData, const SiegeMeshHeader& header );

		// Recursive function for reading BSP information from a file
		void ReadBSPNodeFromFile( const char* &pData, BSPNode* pNode );


		// Unique identifier
		unsigned char				m_id;

		// Bounding box of this logical node
		vector_3					m_minBox;
		vector_3					m_maxBox;

		// BSP'd geometrical information
		BSPTree*					m_pBSPTree;

		// Bit flags that define node attributes
		unsigned int				m_flags;

		// Leaf connections
		unsigned int				m_numLeafConnections;
		LMESHLEAFINFO*				m_pLeafConnectionInfo;

		// Nodal connections
		unsigned int				m_numNodalConnections;
		LCCOLLECTION*				m_pNodalConnectionInfo;
	};
}

#endif