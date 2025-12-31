#pragma once
#ifndef _SIEGE_NODE_IO_
#define _SIEGE_NODE_IO_

/* ========================================================================
   Header File: siege_node_io.h
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
	class SiegeNode;		// from siege_mesh.h
	class database_guid;	// from siege_database_guid.h
};

/* ************************************************************************
   Class: SiegeMeshIO
   Description: Handles i/o of shared meshes
   ************************************************************************ */

namespace siege
{
	class SiegeNodeIO
	{

	public:
		
		static bool Read( SiegeNode& This, database_guid const &GUID, bool bVerbose = true );

	private:
	};
}

#endif
