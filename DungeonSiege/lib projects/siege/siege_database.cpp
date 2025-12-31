/* ========================================================================
   Source File: siege_database.cpp
   Description: Implements siege::database
   ======================================================================== */

#include "precomp_siege.h"
#include "gpcore.h"
#include "FileSys.h"
#include "namingkey.h"

#include "siege_database_guid.h"
#include "siege_database.h"

using namespace siege;

MeshDatabase::MeshDatabase() 
{
}

MeshDatabase::~MeshDatabase()
{
}

FileSys::FileHandle MeshDatabase::ReadFromLatestVersion( const database_guid& meshGUID, bool bVerbose )
{
	gpassertm( meshGUID.IsValid(), "Can't read in an undefined GUID object" );

	// Find the mesh in the mapping
	MeshFileMap::iterator i = m_MeshFileMap.find( meshGUID );

	FileSys::FileHandle hFile;
	if( i != m_MeshFileMap.end() )
	{
		gpstring filename;
		if( gNamingKey.BuildSNOLocation( (*i).second, filename ) )
		{
			if( !hFile.Open( filename.c_str() ) && bVerbose && gSiegeEngine.GetOptions().ErrorReporting() )
			{
				gperrorf( ("Could not open file: \"%s\" in SiegeDatabase for mesh guid: '%s'!", filename.c_str(), meshGUID.ToString().c_str() ) );
			}
		}
	}
	else if( bVerbose && gSiegeEngine.GetOptions().ErrorReporting() )
	{
		gperrorf(( "Handle to unknown mesh [%s] was requested!", meshGUID.ToString().c_str() ));
	}

	return hFile;
}

// Insert a new entry into the mapping
void MeshDatabase::InsertMeshMapping( const database_guid& meshGUID, const gpstring& fileName )
{
	m_MeshFileMap.insert( std::pair< database_guid, gpstring >( meshGUID, fileName ) );
}

// Remove an entry from the mapping
void MeshDatabase::RemoveMeshMapping( const database_guid& meshGUID )
{
	m_MeshFileMap.erase( meshGUID );
}

// Find the name of a mesh file
const char* MeshDatabase::FindFileName( const database_guid& meshGUID )
{
	// Find the mesh in the mapping
	MeshFileMap::iterator i = m_MeshFileMap.find( meshGUID );
	if( i != m_MeshFileMap.end() )
	{
		return (*i).second.c_str();
	}

	return NULL;
}


// Construction
NodeDatabase::NodeDatabase()
{
}

// Destruction
NodeDatabase::~NodeDatabase()
{
	gpassert( m_offsetMap.empty() );
	gpassert( m_fileColl.empty() );
}

// Return whether or not a given node is valid
bool NodeDatabase::IsNodeValid( const database_guid& nodeGUID )
{
#if !GP_RETAIL

	// Find the node
	LogicalOffsetMap::iterator i = m_offsetMap.find( nodeGUID );
	if( i != m_offsetMap.end() )
	{
		return (*i).second.valid;
	}

	return false;

#else

	return true;

#endif
}

// Get a mem handle, if possible, to the given node
FileSys::FileHandle NodeDatabase::ReadFromLatestVersion( const database_guid& nodeGUID, bool bVerbose, DWORD& offset, DWORD& size )
{
	FileSys::FileHandle hFile;

	// Find the node
	LogicalOffsetMap::iterator i = m_offsetMap.find( nodeGUID );
	if( i != m_offsetMap.end() )
	{
		if( (*i).second.valid )
		{
			// Fill in the file offsets for this node
			offset	= (*i).second.offset;
			size	= (*i).second.size;
			hFile	= (*i).second.fileHandle;
		}

#if !GP_RETAIL

		else if( bVerbose && gSiegeEngine.GetOptions().ErrorReporting() )
		{
			gperrorf(( "Handle to unloaded node %s requested!", nodeGUID.ToString().c_str() ));
		}

#endif

	}
	else if( bVerbose && gSiegeEngine.GetOptions().ErrorReporting() )
	{
		gperrorf(( "Handle to unknown node %s requested!", nodeGUID.ToString().c_str() ));
	}

	return hFile;
}

// Insert a new logical file into the mapping, if it isn't already there
FileSys::FileHandle NodeDatabase::ReferenceLogicalFile( const gpstring& fileName, const database_guid& nodeGUID )
{
	// Make sure we don't already have this logical file in our mapping
	for( LogicalFileColl::iterator i = m_fileColl.begin(); i != m_fileColl.end(); ++i )
	{
		if( fileName.same_no_case( (*i).fileName ) )
		{
			// Increment reference count
			(*i).refCount++;

			// Mark this node as being properly referenced
			LogicalOffsetMap::iterator o = m_offsetMap.find( nodeGUID );
			if( o != m_offsetMap.end() )
			{
				(*o).second.valid	= true;
			}

#if !GP_RETAIL

			else if( gSiegeEngine.GetOptions().ErrorReporting() )
			{
				gperrorf(( "File %s exists in logical file mapping but the node %s is not in it!", fileName.c_str(), nodeGUID.ToString().c_str() ));
			}

#endif

			// Return the file index
			return (*i).fileHandle;
		}
	}

	// We need to parse the logical table for this file
	FileSys::FileHandle hFile( fileName.c_str() );
	if( !hFile.IsOpen() && gSiegeEngine.GetOptions().ErrorReporting() )
	{
		gperrorf(( "File %s was requested, but it does not exist!", fileName.c_str() ));
		return 0;
	}

	// Build new file information structure
	LogicalFile newFile;
	newFile.refCount	= 1;
	newFile.fileName	= fileName;
	newFile.fileHandle	= hFile;

	unsigned int guidValue	= nodeGUID.GetValue();

	// Read the first dword of the file to get the size of the map
	unsigned int sizeOfMap = 0;
	hFile.Read( &sizeOfMap, sizeof( unsigned int ) );

	// Go through it
	for( unsigned int m = 0; m < sizeOfMap; ++m )
	{
		// Read the guid
		unsigned int guid = 0;
		hFile.Read( &guid, sizeof( unsigned int ) );

		LogicalOffset logOffset;
		logOffset.fileHandle	= hFile;
		hFile.Read( &logOffset.offset, sizeof( DWORD ) );
		hFile.Read( &logOffset.size, sizeof( DWORD ) );

		// Save the offset and size if this is the node we are looking for
		if( guidValue == guid )
		{
			logOffset.valid	= true;
		}
		else
		{
			logOffset.valid = false;
		}

		database_guid nGuid( guid );

		// Insert new table element
		m_offsetMap.insert( std::pair< database_guid, LogicalOffset >( nGuid, logOffset ) );
		newFile.nodeColl.push_back( nGuid );
	}

	// Put this offset table into our file mapping
	m_fileColl.push_back( newFile );

	// Return the file index
	return hFile;
}

// Release a reference from the GUID
void NodeDatabase::ReleaseLogicalFile( const database_guid& nodeGUID, FileSys::FileHandle fileHandle )
{
	if( !nodeGUID.IsValid() || fileHandle.IsNull() )
	{
		return;
	}

	// Find the node
	LogicalOffsetMap::iterator i = m_offsetMap.find( nodeGUID );
	if( i != m_offsetMap.end() )
	{
		// Set this node as not valid
		(*i).second.valid	= false;
	}

#if !GP_RETAIL

	else if( gSiegeEngine.GetOptions().ErrorReporting() )
	{
		gperrorf(( "Cannot release logical information for node %s when that node doesn't exist!", nodeGUID.ToString().c_str() ));
	}

#endif

	// Look up the file in our mapping
	for( LogicalFileColl::iterator f = m_fileColl.begin(); f != m_fileColl.end(); ++f )
	{
		if( fileHandle == (*f).fileHandle )
		{
			// Decrement reference count
			(*f).refCount--;

			if( (*f).refCount == 0 )
			{
				// Remove this table from our mapping from
				for( LogicalNodeColl::iterator n = (*f).nodeColl.begin(); n != (*f).nodeColl.end(); ++n )
				{
					m_offsetMap.erase( (*n) );
				}

				(*f).fileHandle.Close();
				m_fileColl.erase( f );
			}
			break;
		}
	}
}

// Close logical file
void NodeDatabase::CloseLogicalFile( const gpstring& fileName )
{
	for( LogicalFileColl::iterator i = m_fileColl.begin(); i != m_fileColl.end(); ++i )
	{
		if( fileName.same_no_case( (*i).fileName ) )
		{
			(*i).fileHandle.Close();
		}
	}
}

// Open logical file
void NodeDatabase::OpenLogicalFile( const gpstring& fileName )
{
	for( LogicalFileColl::iterator i = m_fileColl.begin(); i != m_fileColl.end(); ++i )
	{
		if( fileName.same_no_case( (*i).fileName ) )
		{
			(*i).fileHandle.Open( fileName.c_str() );

			for( LogicalNodeColl::iterator n = (*i).nodeColl.begin(); n != (*i).nodeColl.end(); ++n )
			{
				SiegeNode* pNode	= gSiegeEngine.IsNodeValid( (*n) );
				if( pNode )
				{
					pNode->SetFileHandle( (*i).fileHandle );
				}
			}
		}
	}
}