#pragma once
#ifndef _SIEGE_DATABASE_
#define _SIEGE_DATABASE_
/* ========================================================================
   Header File: siege_database.h
   Description: Declares siege::database
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include <string>
#include <map>
#include <vector>
#include "filesys.h"
#include "bucketvector.h"

namespace siege
{
	class database_guid;		// From siege_database_guid.h
}

/* ************************************************************************
   Class: database
   ************************************************************************ */
namespace siege
{
	class MeshDatabase
	{
	public:
		typedef std::map< siege::database_guid, gpstring > MeshFileMap;

		// Construction
		MeshDatabase();
		~MeshDatabase();

		// Get a file handle, if possible, to the given mesh
		FileSys::FileHandle ReadFromLatestVersion( const database_guid& meshGUID, bool bVerbose );

		// Insert a new entry into the mapping
		void InsertMeshMapping( const database_guid& meshGUID, const gpstring& fileName );

		// Remove an entry from the mapping
		void RemoveMeshMapping( const database_guid& meshGUID );

		// Find the name of a mesh file
		const char* FindFileName( const database_guid& meshGUID );

		// Get the mesh file mapping
		MeshFileMap& FileNameMap()								{ return m_MeshFileMap; }

	private:

		// Mesh file index
		MeshFileMap				m_MeshFileMap;
	};

	class NodeDatabase
	{
		struct LogicalOffset
		{
			FileSys::FileHandle fileHandle;
			DWORD offset;
			DWORD size;
			bool  valid;
		};

		typedef std::map< database_guid, LogicalOffset >		LogicalOffsetMap;
		typedef std::vector< database_guid >					LogicalNodeColl;

		struct LogicalFile
		{
			DWORD				refCount;
			gpstring			fileName;
			FileSys::FileHandle fileHandle;
			LogicalNodeColl		nodeColl;
		};

		typedef std::vector< LogicalFile >						LogicalFileColl;

	public:

		// Construction
		NodeDatabase();
		~NodeDatabase();

		// Return whether or not a given node is valid
		bool IsNodeValid( const database_guid& nodeGUID );

		// Get a mem handle, if possible, to the given node
		FileSys::FileHandle ReadFromLatestVersion( const database_guid& nodeGUID, bool bVerbose, DWORD& offset, DWORD& size );

		// Insert a new logical file into the mapping, if it isn't already there
		FileSys::FileHandle ReferenceLogicalFile( const gpstring& fileName, const database_guid& nodeGUID );

		// Release a reference from the GUID
		void ReleaseLogicalFile( const database_guid& nodeGUID, FileSys::FileHandle fileHandle );

		// Close/Open logical files.  For editor use only.
		void CloseLogicalFile( const gpstring& fileName );
		void OpenLogicalFile( const gpstring& fileName );

	private:

		// Node offset mapping
		LogicalOffsetMap		m_offsetMap;

		// Logical file mapping
		LogicalFileColl			m_fileColl;
	};
};

#endif
