#pragma once
#ifndef _SIEGE_MESH_IO_
#define _SIEGE_MESH_IO_

/* ========================================================================
   Header File: siege_mesh_io.h
   Declares: 
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include <string>

/* ========================================================================
   Forward Declarations
   ======================================================================== */

namespace siege
{
	class SiegeMesh;			// from siege_mesh.h
	class database_guid;		// from siege_database_guid.h
};

/* ************************************************************************
   Class: SiegeMeshIO
   Description: Handles i/o of shared meshes
   ************************************************************************ */

namespace siege
{
	struct SiegeMeshHeader
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

	class SiegeMeshIO
	{
	public:
		
		// Read in a SiegeMesh
		static bool Read( SiegeMesh &This, database_guid const &GUID, bool bVerbose = true );

	};

}

#endif
