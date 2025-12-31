#pragma once
#ifndef _SIEGE_MESH_
#define _SIEGE_MESH_

#include "RapiOwner.h"
#include "siege_cache_handle.h"
#include "siege_bsp.h"

namespace siege
{
	class SiegeMeshIO;
	class SiegeMeshDoor;
	class database;
	class SiegeLogicalMesh;
	class SiegeLogicalNode;

	struct SiegeMeshHeader;
}


namespace siege
{

#if !GP_RETAIL

	// Spot class definition
	class Spot
	{

	public:

		Spot(const gpstring& l, const matrix_3x3& m, const vector_3& p) 
			: m_label(l)
			, m_orientation(m)
			, m_position(p)
		{
		};

		gpstring	m_label;
		matrix_3x3	m_orientation;
		vector_3	m_position;
	};

	typedef std::vector<my Spot*> SpotArray;
	typedef SpotArray::iterator SpotArrayIter;
	typedef SpotArray::const_iterator SpotArrayConstIter;

#endif

	class SiegeMesh
	{

	public:

		typedef std::list<my SiegeMeshDoor*>			SiegeMeshDoorList;
		typedef SiegeMeshDoorList::iterator				SiegeMeshDoorIter;
		typedef SiegeMeshDoorList::const_iterator		SiegeMeshDoorConstIter;
		typedef std::vector< gpstring >					TexStrings;

		enum
		{
			FLAG_DRAW_FLOORS	= 1<<1,
			FLAG_DRAW_NORMALS	= 1<<2,
			FLAG_DRAW_SPOTS		= 1<<3,
			FLAG_DRAW_ORIGIN	= 1<<4,
			FLAG_DRAW_WATER		= 1<<5,
		};

		struct VERTINDEXCOLLECTION
		{
			UINT32 VertexIndex;
			UINT32 TextureVertexIndex;
			UINT32 ColorIndex;
			UINT32 NormalIndex;
		};

		struct storeVertex
		{
			float x, y, z;
			float nx, ny, nz;
			DWORD color;
			UV	  uv;
		};

	private:

		// Don't let anyone else copy
		SiegeMesh(SiegeMesh const &);
		SiegeMesh &operator=(SiegeMesh const &);

		// List of doors
		SiegeMeshDoorList					m_DoorList;

		// Door and spot information
		unsigned int						m_numDoors;
		unsigned int						m_numSpots;

		// Normals
		vector_3*							m_pNormals;

		// Texture strings
		TexStrings							m_Textures;

		// Mesh information
		RapiStaticObject*					m_pRenderObject;

		// Stage information
		unsigned int						m_numStages;

		// Spatial information
		vector_3							m_minBBox;
		vector_3							m_maxBBox;
		vector_3							m_centroidOffset;

		// Is this mesh tiled or clamped?
		bool								m_bTile;

		// Logical meshes
		unsigned int						m_numLogicalMeshes;
		SiegeLogicalMesh*					m_pLogicalMeshes;

#if !GP_RETAIL

		// Colors
		DWORD*								m_pColors;

		// Array of spots on the terrain, used for placement of objects
		SpotArray							m_Spots;

		// Debugging & History information read in from the SNO file
		std::list< gpstring >				m_infoStrings;

		// Checksum
		DWORD								m_checksum;

#endif

	public:

		// IO
		typedef SiegeMeshIO io_handler;
		friend SiegeMeshIO;

		SiegeMesh(void);
		~SiegeMesh(void);

		// Render object
		RapiStaticObject*			GetRenderObject() const				{ return m_pRenderObject; }

		// Texture information
		const TexStrings&			GetTextureStrings() const			{ return m_Textures; }

		// Mesh Information
		inline unsigned int			GetNumberOfVertices(void) const		{ return m_pRenderObject->GetNumVertices(); }
		inline unsigned int			GetNumberOfTriangles(void) const	{ return m_pRenderObject->GetNumTriangles(); }

		// Get information from the mesh
		sVertex*					GetVertices()						{ return m_pRenderObject->GetVertices(); }

		// Get lighting information
		inline void					GetLightingInformation( int unsigned const VertexIndex, vector_3 **Vertex, vector_3 **Normal) const
									{
										*Vertex = (vector_3*)&m_pRenderObject->GetVertices()[ VertexIndex ];
										*Normal = &m_pNormals[ VertexIndex ];
									}

		// Get logical information
		unsigned int				GetNumLogicalMeshes() const			{ return m_numLogicalMeshes; }
		SiegeLogicalMesh*			GetLogicalMeshes() const			{ return m_pLogicalMeshes; }
		
		// Render the mesh
		int							Render( int flag, const NodeTexColl& texList, DWORD* litColorArray, const DWORD alpha ) const;
		int							RenderAlpha( const NodeTexColl& texList, DWORD* litColorArray, bool noalphasort = false ) const;

		// Load the mesh
		bool						Loader( const char* pData, SiegeMeshHeader const &Header );

		inline const vector_3&		Centroid() const					{ return m_centroidOffset; }

		const vector_3&				GetMinimumCorner() const			{ return m_minBBox; }
		const vector_3&				GetMaximumCorner() const			{ return m_maxBBox; }

		vector_3					BBoxCenter() const					{ return (m_maxBBox+m_minBBox)*0.5f; }
		vector_3					BBoxHalfDiag() const				{ return m_maxBBox - (m_maxBBox+m_minBBox)*0.5f; }

		// Doors
		const SiegeMeshDoorList&	GetDoors() const					{ return m_DoorList; }
		SiegeMeshDoor*				GetDoorByIndex( unsigned int id ) const;
		SiegeMeshDoor*				GetDoorByID( unsigned int id ) const;

#if !GP_RETAIL

		inline DWORD				GetColorInformation( int unsigned const VertexIndex ) const
									{
										return m_pColors[ VertexIndex ];
									}

		// Spots
		int							NumSpots() const					{ return m_numSpots; }
		const Spot&					GetSpotByIndex( unsigned int id ) const;
		const Spot&					GetSpotByLabel( const char* label ) const;
		bool						GetSpotPositionOrientation( const char* s, vector_3 &p, matrix_3x3 &o ) const;

		const std::list<gpstring>&	GetInfo() const						{ return m_infoStrings; }

		// Checksum
		DWORD						GetMeshChecksum()	const			{ return m_checksum; }
		void						SetMeshChecksum( DWORD checksum )	{ m_checksum = checksum; }

#endif
	};

	// TYPDEFS
	typedef cache_handle<SiegeMesh> SiegeMeshHandle;

}

#endif
