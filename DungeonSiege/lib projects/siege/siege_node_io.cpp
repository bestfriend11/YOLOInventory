/* ========================================================================
   Source File: spacenode_io.cpp
   Derived From: shared_mesh_io.cpp
   Description: Implements SpaceNode_io
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "precomp_siege.h"

#include "vector_3.h"

#include "siege_database_guid.h"
#include "siege_database.h"

#include "siege_engine.h"

#include "siege_node.h"
#include "siege_node_io.h"

using namespace siege;


/* ========================================================================
   Types
   ======================================================================== */

bool SiegeNodeIO::Read( SiegeNode& This, database_guid const &GUID, bool bVerbose )
{
	if( GUID == UNDEFINED_GUID )
	{
		return true;
	}

	bool ReadSuccessful = false;

	// Open the file if it exists
	DWORD offset = 0, size = 0;
	FileSys::FileHandle hFile = gSiegeEngine.NodeDatabase().ReadFromLatestVersion( GUID, bVerbose, offset, size );
	if( hFile.IsOpen() )
	{
		// Map this node's part of the file
		FileSys::MemHandle hMem;
		if( hMem.Map( hFile, offset, size ) )
		{
			// Page all data into memory
			hMem.Touch();

			// Set the guid for the node as a precaution
			This.SetGUID( GUID );

			// Load the node info
			This.Load( (const char*)hMem.GetData() );

			// Close the mapping and indicate success
			hMem.Close();
			ReadSuccessful = true;
		}
		else
		{
			gpwatson( "Corrupted LNC file!  This is really REALLY bad!  Go get an older version from SourceSafe and hope for the best!" );
		}
	}

	return(ReadSuccessful);
}
