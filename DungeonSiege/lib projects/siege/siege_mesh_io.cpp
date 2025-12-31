/* ========================================================================
   Source File: siege_mesh_io.cpp
   Derived From: spacenode_io.cpp, shared_mesh_io.cpp
   Description: Implements siege_mesh_io
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "precomp_siege.h"

#include "FileSys.h"

#include "siege_database_guid.h"
#include "siege_database.h"

#include "siege_engine.h"

#include "siege_mesh.h"
#include "siege_mesh_io.h"
#include "siege_mesh_door.h"

#include "matrix_3x3.h"

using namespace siege;

/* ========================================================================
   Types
   ======================================================================== */

static UINT32 const HeaderMagicValue = 0x444f4e53; // "SNOD"

bool SiegeMeshIO::Read( SiegeMesh& This, database_guid const &GUID, bool bVerbose )
{
	bool ReadSuccessful = false;

	FileSys::AutoFileHandle hFile = gSiegeEngine.MeshDatabase().ReadFromLatestVersion( GUID, bVerbose );
	FileSys::AutoMemHandle hMem;

	if( hMem.Map( hFile ) )
	{
		// Page all data into memory
		hMem.Touch();

		// Read header
		const char* pData	= (const char*)hMem.GetData();
		SiegeMeshHeader Header;
		memcpy( &Header, pData, sizeof( SiegeMeshHeader ) );
		pData += sizeof( SiegeMeshHeader );

		gpassertm( Header.m_id == HeaderMagicValue, "SNO file has unknown internal type!" );
		gpassert( Header.m_majorVersion >= 6 );

		if( Header.m_majorVersion > 6 ||
			(Header.m_majorVersion == 6 && Header.m_minorVersion >= 2) )
		{
#if !GP_RETAIL

			DWORD checksum	= *((DWORD *)pData);
			This.SetMeshChecksum( checksum );

#endif
			pData	+= sizeof( DWORD );
		}

		ReadSuccessful = This.Loader( pData, Header );
	}
	else
	{
		const char* fileName = gSiegeEngine.MeshDatabase().FindFileName( GUID );
		if ( fileName == NULL )
		{
			fileName = "<filename not found>";
		}
		gpwatsonf(( "Unable to open file '%s' (mesh guid: %s)\n\n"
					"Check your nodes_misc.gas file for a typo. Cannot continue, the program will now exit.",
					fileName, GUID.ToString().c_str() ));
	}

	return(ReadSuccessful);
}
