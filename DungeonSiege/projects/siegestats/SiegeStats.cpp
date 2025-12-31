/**********************************************************************************************
**
**										SiegeTex
**
**		Tracks texture usage within the SiegeEngine.
**
**		Author:		James Loe
**		Date:		11/03/00
**
***********************************************************************************************/
#include "ignoredwarnings.h"

#include <vector>
#include <set>
#include <functional>

#include "gpcore.h"
#include "vector_3.h"
#include "stringtool.h"
#include "filesys.h"
#include "filesysutils.h"
#include "pathfilemgr.h"
#include "fuel.h"
#include "fueldb.h"
#include "namingkey.h"
#include "rapiimage.h"

using namespace FileSys;

struct NodeRef
{
	gpstring	nodeName;
	bool		bGeneric;
	int			refCount;
	int			polyCount;

	std::set< gpstring, istring_less > texSets;
};

struct TexRef
{
	gpstring	texName;
	int			refCount;
};

struct storeVertex
{
	float			x, y, z;
	float			nx, ny, nz;
	DWORD			color;
	float			u,v;
};

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

int GetSNOPolyCount( const gpstring& name )
{
	FileSys::AutoFileHandle hFile;
	hFile.Open( name );

	AutoMemHandle hMem;
	hMem.Map( hFile );

	if( hMem.IsMapped() )
	{
		// Read header
		return ((SiegeMeshHeader*)hMem.GetData())->m_numTriangles;
	}

	return 0;
}

std::list< gpstring > GetSNOTextures( const gpstring& name )
{
	std::list< gpstring > texList;
	std::list< gpstring > tempTexList;

	FileSys::AutoFileHandle hFile;
	hFile.Open( name );

	AutoMemHandle hMem;
	hMem.Map( hFile );

	if( hMem.IsMapped() )
	{
		// Read header
		const char* pData	= (const char*)hMem.GetData();
		SiegeMeshHeader Header;
		memcpy( &Header, pData, sizeof( SiegeMeshHeader ) );
		pData += sizeof( SiegeMeshHeader );

		// Step past the checksum
		if( Header.m_majorVersion > 6 ||
			(Header.m_majorVersion == 6 && Header.m_minorVersion >= 2) )
		{
			pData		+= sizeof( DWORD );
		}

		// Read in the doors
		for( unsigned int d = 0; d < Header.m_numDoors; ++d )
		{
			pData += sizeof( unsigned int );
			pData += sizeof( vector_3 );
			pData += sizeof( vector_3 );
			pData += sizeof( vector_3 );
			pData += sizeof( vector_3 );

			int numVerts = *((int *)pData);
			pData += sizeof( int );

			for( ; numVerts > 0; --numVerts )
			{
				pData += sizeof( int );
			}
		}

		// Read in the spots
		for( unsigned int s = 0; s < Header.m_numSpots; ++s )
		{
			pData += sizeof( vector_3 );
			pData += sizeof( vector_3 );
			pData += sizeof( vector_3 );
			pData += sizeof( vector_3 );
			pData += strlen( pData ) + 1;
		}

		for( unsigned int v = 0; v < Header.m_numVertices; ++v )
		{
			pData	+= sizeof( storeVertex );
		}
		
		// Write out the stages
		for( unsigned int t	= 0; t < Header.m_numStages; ++t )
		{
			// Pull the name from the file
			const char * pname = pData;
			int namelen = strlen( pData );
			pData += namelen + 1;

			// Put the string on our list of textures
			tempTexList.push_back( gpstring( pname, namelen ) );

			pData					+= sizeof( unsigned int );
			pData					+= sizeof( unsigned int );

			int numVIndices			= *((unsigned int *)pData);
			pData					+= sizeof( unsigned int );
			pData					+= sizeof( WORD ) * numVIndices;
		}
	}

	for( std::list< gpstring >::iterator t = tempTexList.begin(); t != tempTexList.end(); ++t )
	{
		gpstring fullTexName;
		if( gNamingKey.BuildIMGLocation( (*t), fullTexName ) )
		{
			// Open the fuel block to the requested information
			FuelHandle hSurface( FilePathToFuelAddress( fullTexName ) );
			if( hSurface.IsValid() )
			{
				// This should be the information we are searching for
				int numLayers	= 1;
				hSurface->Get( "numlayers", numLayers );
				gpassert( numLayers );

				for( int layer = 0; layer < numLayers; ++layer )
				{
					char buf[256];
					sprintf( buf, "layer%dnumframes", layer+1 );

					int numFrames	= 1;
					hSurface->Get( buf, numFrames );
					gpassert( numFrames );

					for( int frame = 0; frame < numFrames; ++frame )
					{
						sprintf( buf, "layer%dtexture%d", layer+1, frame+1 );
						gpstring texturecontentname;

						hSurface->Get( buf, texturecontentname );

						if( !texturecontentname.same_no_case( "environment" ) )
						{
							texList.push_back( texturecontentname );
						}
					}
				}
			}
			else
			{
				// This texture has no descriptor
				texList.push_back( (*t) );
			}
		}
		else
		{
			// This texture fails naming key
			texList.push_back( (*t) );
		}
	}

	return texList;
}




int main(int argc, char* argv[])
{
	RapiImageReader::RegisterFileTypeAliases();

	if( argc < 3 )
	{
		printf( "USAGE:\n        siegestats [cdimage dir] [output.txt]\n(i.e.)  siegestats c:\\tattoo_assets\\cdimage c:\\tattoo_assets\\bad_textures.txt\n\n" );
	}
	else
	{
		gpstring home_dir		= argv[1];
		gpstring output_file	= argv[2];

		bool g_bCleanup			= false;
		if( argc == 4 )
		{
			gpstring cleanup	= argv[3];
			if( cleanup.same_no_case( "cleanup" ) )
			{
				g_bCleanup		= true;
			}
		}

		stringtool::AppendTrailingBackslash( home_dir );

		if( !DoesPathExist( home_dir ) )
		{
			printf( "That cdimage directory is invalid!\n" );
			return 0;
		}
		if( !IsValidFileName( GetFileNameOnly( output_file ) ) )
		{
			printf( "That is not a valid output filename!\n" );
			return 0;
		}

		// File system init
		PathFileMgr fileMgr;
		fileMgr.SetRoot( home_dir );
		fileMgr.SetMasterFileMgr();

		// Fuel system init
		FuelSys fuel;
		BinFuel::SetDictionaryPath( "" );
		TextFuelDB* pfDB			= fuel.AddTextDb( "default" );
		pfDB->Init( FuelDB::OPTION_READ | FuelDB::OPTION_JIT_READ |
			FuelDB::OPTION_WRITE | FuelDB::OPTION_SAVE_EMPTY_BLOCKS, "", home_dir );

		BinaryFuelDB * pfbDB	= fuel.AddBinaryDb( "default_binary" );
		pfbDB->Init( FuelDB::OPTION_READ | FuelDB::OPTION_JIT_READ, "", home_dir );

		// Namingkey init
		NamingKey nKey( "art\\NamingKey.NNK" );

		// create the full set of paths required to make this file
		CreateFullFileNameDirectory( output_file );
		File output;
		output.CreateAlways( output_file );

		// Our first order of business is to go through all of the available nodes_misc files and build a list
		// of SNO's that the game knows about.
		std::map< int, NodeRef >								snoMap;
		std::map< int, NodeRef >::iterator						n;
		std::list< gpstring >									noNamingKeyList;
		std::list< gpstring >									noFileList;
		std::list< gpstring >									badGenericList;
		std::list< gpstring >									genericNodesList;
		std::list< gpstring >									duplicateGuidList;
		std::list< gpstring >									nomiscentryNodesList;
		std::list< gpstring >									scidIndexMismatchList;
		std::list< gpstring >									scidMissingList;
		std::set< gpstring, istring_less >						noFileTextures;
		std::set< gpstring, istring_less >						noNamingKeyTextures;
		std::set< int >											badGuids;
		std::map< gpstring, TexRef, istring_less >				texMap;
		std::map< gpstring, TexRef, istring_less >::iterator	tm;
		gpstring												outputStr;

		// Load the generic node naming key
		FuelHandle genNameKey( "world:global:siege_nodes:generic:generic_node_naming_key" );
		if( genNameKey->FindFirstKeyAndValue() )
		{
			gpstring key, value;
			while( genNameKey->GetNextKeyAndValue( key, value ) )
			{
				genericNodesList.push_back( key );
			}
		}

		// Create a handle to our misc folder
		printf( "SNO verification in progress.\n" );
		{
			FastFuelHandle miscHandle( "world:global:siege_nodes" );
			FastFuelHandleColl hNodes;
			miscHandle.ListChildrenTyped( hNodes, "siege_nodes", -1 );

			for( FastFuelHandleColl::iterator i = hNodes.begin(); i != hNodes.end(); ++i )
			{
				FastFuelHandleColl node_blocks;
				(*i).ListChildren( node_blocks );
				for( FastFuelHandleColl::const_iterator s = node_blocks.begin(); s != node_blocks.end(); ++s )
				{
					gpstring fName	= (*s).GetString( "filename" );
					if( !fName.empty() )
					{
						gpstring snoName;
						if( nKey.BuildSNOLocation( fName, snoName ) )
						{
							if( DoesResourceFileExist( snoName ) )
							{
								NodeRef ref;
								ref.nodeName	= snoName;

								// If this is a generic mesh, we need to test and make sure that it has a copy of itself
								// for each generic set
								unsigned int pos = fName.find( "xxx" );
								if( pos != fName.npos )
								{
									ref.bGeneric	= true;
								}
								else
								{
									ref.bGeneric	= false;
								}

								ref.refCount	= 0;

								int guid = (*s).GetInt( "guid" );
								std::map< int, NodeRef >::iterator n = snoMap.find( guid );
								if( n != snoMap.end() )
								{
									duplicateGuidList.push_back( snoName + " GUID ALREADY USED BY " + (*n).second.nodeName );
								}
								else
								{
									snoMap.insert( std::pair< int, NodeRef >( guid, ref ) );
								}
							}
							else
							{
								noFileList.push_back( snoName );
							}
						}
						else
						{
							// Bad sno name
							noNamingKeyList.push_back( fName );
						}
					}
					else
					{
						// Check for generics
						fName	= (*s).GetString( "genericname" );
						if( !fName.empty() )
						{
							// If this is a generic mesh, we need to test and make sure that it has a copy of itself
							// for each generic set
							unsigned int pos = fName.find( "xxx" );
							if( pos != fName.npos )
							{
								gpstring snoName;
								if( nKey.BuildSNOLocation( fName, snoName ) )
								{
									if( DoesResourceFileExist( snoName ) )
									{
										NodeRef ref;
										ref.nodeName	= snoName;
										ref.bGeneric	= true;
										ref.refCount	= 0;

										int guid = (*s).GetInt( "guid" );
										std::map< int, NodeRef >::iterator n = snoMap.find( guid );
										if( n != snoMap.end() )
										{
											duplicateGuidList.push_back( snoName + " GUID ALREADY USED BY " + (*n).second.nodeName );
										}
										else
										{
											snoMap.insert( std::pair< int, NodeRef >( guid, ref ) );
										}
									}
									else
									{
										noFileList.push_back( snoName );
									}
								}
								else
								{
									noNamingKeyList.push_back( fName );
								}
							}
							else
							{
								badGenericList.push_back( fName );
							}
						}
					}
				}
			}

			miscHandle.Unload();

			RecursiveFinder finder( "art\\terrain\\*.sno" );

			gpstring snodeName;
			while( finder.GetNext( snodeName, true ) )
			{
				for( n = snoMap.begin(); n != snoMap.end(); ++n )
				{
					if( (*n).second.nodeName.same_no_case( snodeName ) )
					{
						break;	
					}
				}
				if( n == snoMap.end() )
				{
					// Couldn't find SNO
					nomiscentryNodesList.push_back( snodeName );

					if( g_bCleanup )
					{
						// Delete it
						DeleteFile( home_dir + snodeName );
					}
				}
			}
		}

		outputStr	= "\nMap statistics                       [NODES]  [POLYS]\n";
		output.WriteText( outputStr, outputStr.length() );

		// Build handle to maps
		printf( "Map verification in progress.\n" );
		{
			for( n = snoMap.begin(); n != snoMap.end(); ++n )
			{
				(*n).second.polyCount = GetSNOPolyCount( (*n).second.nodeName );
			}

			FastFuelHandle mapHandle( "world:maps" );
			FastFuelHandleColl hMaps;
			mapHandle.ListChildren( hMaps );

			for( FastFuelHandleColl::iterator m = hMaps.begin(); m != hMaps.end(); ++m )
			{
				FuelHandle mHand( gpstring( (*m).GetAddress() ) + ":map" );
				outputStr	= "\n    ";
				if( !mHand )
				{
					continue;
				}

				gpstring mapName( mHand->GetString( "name" ) );
				outputStr	+= mapName + ":\n";
				output.WriteText( outputStr, outputStr.length() );

				int totalnodeCount	= 0;
				int totalpolyCount	= 0;

				FastFuelHandle regionHandle( gpstring( (*m).GetAddress() ) + ":regions" );
				if( regionHandle.IsValid() )
				{
					FastFuelHandleColl hRegions;
					regionHandle.ListChildren( hRegions );

					for( FastFuelHandleColl::iterator r = hRegions.begin(); r != hRegions.end(); ++r )
					{
						// Do node evaluation
						int nodeCount = 0;
						int polyCount = 0;

						FastFuelHandle nodeHandle( gpstring( (*r).GetAddress() ) + ":terrain_nodes:siege_node_list" );
						if( nodeHandle.IsValid() )
						{
							FastFuelHandleColl hNodes;
							nodeHandle.ListChildren( hNodes );
							for( FastFuelHandleColl::iterator i = hNodes.begin(); i != hNodes.end(); ++i )
							{
								int guid	= (*i).GetInt( "mesh_guid" );

								n = snoMap.find( guid );
								if( n != snoMap.end() )
								{
									(*n).second.refCount++;
									if( (*n).second.bGeneric )
									{
										(*n).second.texSets.insert( (*i).GetString( "texsetabbr" ) );
									}
									
									polyCount	+= (*n).second.polyCount;
									nodeCount++;
								}
								else
								{
									badGuids.insert( guid );
								}
							}
						}

						outputStr	= "        ";
						outputStr	+= (*r).GetName();
						stringtool::PadBackToLength( outputStr, ' ', 32 );
						output.WriteF( "%s = %8d | %8d\n", outputStr.c_str(), nodeCount, polyCount );

						totalnodeCount	+= nodeCount;
						totalpolyCount	+= polyCount;

						// Load context index map for this region
						std::map< DWORD, DWORD > indexMapping;
						FastFuelHandle indexHandle( gpstring( (*r).GetAddress() ) + ":index:streamer_node_content_index" );

						if( indexHandle.IsValid() )
						{
							FastFuelFindHandle fh( indexHandle );
							gpstring key, value;
							DWORD node, scid;
							fh.FindFirstKeyAndValue();

							while ( fh.GetNextKeyAndValue( key, value ) )
							{
								stringtool::Get( key, node );
								stringtool::Get( value, scid );

								indexMapping.insert( std::pair< DWORD, DWORD >( scid, node ) );							
							}

							// Iterate through objects for this region
							FastFuelHandle objectHandle( gpstring( (*r).GetAddress() ) + ":objects" );
							if( objectHandle.IsValid() )
							{
								FastFuelHandleColl hObjects;
								objectHandle.ListChildren( hObjects );
								for( FastFuelHandleColl::iterator i = hObjects.begin(); i != hObjects.end(); ++i )
								{
									stringtool::Get( (*i).GetName(), scid );
									std::map< DWORD, DWORD >::iterator iIndex = indexMapping.find( scid );
									if( iIndex != indexMapping.end() )
									{
										// Check to make sure node in placement block matches node in index
										FastFuelHandle placementHandle( gpstring( (*i).GetAddress() ) + ":placement" );
										if( placementHandle.IsValid() && placementHandle.HasKey( "position" ) )
										{
											SiegePosData data;
											placementHandle.Get( "position", data );
											
											if( (*iIndex).second != data.node.GetValue() )
											{
												// Build error string
												gpstring objectError;
												objectError.assignf( "SCID: 0x%08x  NAME: %-30s  MAP: %-30s  REGION: %-20s  POSNODE: 0x%08x  INDEXNODE: 0x%08x", scid, (*i).GetType(), mapName.c_str(), (*r).GetName(), data.node.GetValue(), (*iIndex).second );
												scidIndexMismatchList.push_back( objectError );
											}
										}
									}
									else
									{
										// Build error string
										gpstring objectError;
										objectError.assignf( "SCID: 0x%08x  NAME: %-30s  MAP: %-30s  REGION: %-20s", scid, (*i).GetType(), mapName.c_str(), (*r).GetName() );
										scidMissingList.push_back( objectError );
									}
								}

								objectHandle.Unload();
							}
						}

						(*r).Unload();
					}
				}

				outputStr	= "        ----------------------------------------------------\n";
				output.WriteText( outputStr, outputStr.length() );

				outputStr	= "        Total";
				stringtool::PadBackToLength( outputStr, ' ', 32 );
				output.WriteF( "%s = %8d | %8d\n", outputStr.c_str(), totalnodeCount, totalpolyCount );

				(*m).Unload();
			}

			mapHandle.Unload();
		}

		outputStr	= "\n----------------------------------------------------------------------------------------------------\n";
		output.WriteText( outputStr, outputStr.length() );

		outputStr	= "SNO reference counts:                                                                           [REFS]\n";
		output.WriteText( outputStr, outputStr.length() );

		DWORD usedNodeSpace		= 0;
		DWORD unusedNodeSpace	= 0;

		std::multimap< int, NodeRef, std::greater<int> >	orderedSnoMap;
		for( n = snoMap.begin(); n != snoMap.end(); ++n )
		{
			// Get the size of this file
			FileHandle node_file( (*n).second.nodeName );
			DWORD fileSize	= node_file.GetSize();
			node_file.Close();

			if( (*n).second.refCount == 0 )
			{
				unusedNodeSpace	+= fileSize;

				// Delete file if cleanup is requested
				if( g_bCleanup )
				{
					DeleteFile( home_dir + (*n).second.nodeName );

					gpstring tnodeName	= (*n).second.nodeName;
					stringtool::RemoveExtension( tnodeName );
					gpstring nodeName;
					stringtool::GetBackDelimitedString( tnodeName, '\\', nodeName );

					FuelHandle miscHandle( "world:global:siege_nodes" );
					FuelHandleList hNodes;
					miscHandle->ListChildBlocks( -1, "siege_nodes", NULL, hNodes );

					for( FuelHandleList::iterator i = hNodes.begin(); i != hNodes.end(); ++i )
					{
						FuelHandleList node_blocks;
						(*i)->ListChildBlocks( -1, NULL, NULL, node_blocks );
						for( FuelHandleList::iterator s = node_blocks.begin(); s != node_blocks.end(); ++s )
						{
							gpstring fName	= (*s)->GetString( "filename" );
							if( fName.same_no_case( nodeName ) )
							{
								(*s).Delete();
							}
						}
					}
				}
			}
			else
			{
				usedNodeSpace	+= fileSize;
			}

			orderedSnoMap.insert( std::pair< int, NodeRef >( (*n).second.refCount, (*n).second ) );
		}

		if( g_bCleanup )
		{
			pfDB->SaveChanges();
		}

		for( std::multimap< int, NodeRef, std::greater<int> >::iterator mmn = orderedSnoMap.begin(); mmn != orderedSnoMap.end(); ++mmn )
		{
			stringtool::RemoveFrontDelimitedString( (*mmn).second.nodeName, '\\', outputStr );
			stringtool::RemoveFrontDelimitedString( outputStr, '\\', outputStr );

			outputStr	= "    " + outputStr;
			stringtool::PadBackToLength( outputStr, ' ', 90 );
			output.WriteF( "%s = %8d\n", outputStr.c_str(), (*mmn).second.refCount );
		}

		outputStr	= "\n    Total used node space";
		stringtool::PadBackToLength( outputStr, ' ', 90 );
		output.WriteF( "%s = %u\n", outputStr.c_str(), usedNodeSpace );

		outputStr	= "    Total unused node space";
		stringtool::PadBackToLength( outputStr, ' ', 90 );
		output.WriteF( "%s = %u\n", outputStr.c_str(), unusedNodeSpace );

		outputStr	= "\n-----------------------------------------------------------------------------------------------------\n";
		output.WriteText( outputStr, outputStr.length() );

		// Notify the user of our progress
		printf( "Texture verification in progress.\n" );
		{
			gpstring					tName;

			for( n = snoMap.begin(); n != snoMap.end(); ++n )
			{
				if( !g_bCleanup || (*n).second.refCount )
				{
					std::list< gpstring > texList	= GetSNOTextures( (*n).second.nodeName );
					if( (*n).second.bGeneric )
					{
						// Generic SNO, so we need to check each of it's textures against the generic listing
						for( std::list< gpstring >::iterator t = texList.begin(); t != texList.end(); ++t )
						{
							unsigned int pos	= (*t).find( "xxx" );
							if( pos != (*t).npos )
							{
								// Switch between whole list and just the referenced list if cleanup mode is on.  Duplicated
								// code here, should probably be moved into a function, but this is a tool and I'm too lazy.
								if( g_bCleanup )
								{
									for(std::set< gpstring, istring_less >::iterator g = (*n).second.texSets.begin(); g != (*n).second.texSets.end(); ++g )
									{
										gpstring localized( (*t) );
										localized.replace( pos, 3, (*g) );

										if( nKey.BuildIMGLocation( localized, tName ) )
										{
											if( !DoesResourceFileExist( tName ) )
											{
												noFileTextures.insert( tName );
											}
											else
											{
												tm	= texMap.find( tName );
												if( tm != texMap.end() )
												{
													(*tm).second.refCount++;
												}
												else
												{
													TexRef ref;
													ref.texName		= tName;
													ref.refCount	= 1;
													texMap.insert( std::pair< gpstring, TexRef >( tName, ref ) );
												}
											}
										}
										else
										{
											noNamingKeyTextures.insert( localized );
										}
									}
								}
								else
								{
									for(std::list< gpstring >::iterator g = genericNodesList.begin(); g != genericNodesList.end(); ++g )
									{
										gpstring localized( (*t) );
										localized.replace( pos, 3, (*g) );

										if( nKey.BuildIMGLocation( localized, tName ) )
										{
											if( !DoesResourceFileExist( tName ) )
											{
												noFileTextures.insert( tName );
											}
											else
											{
												tm	= texMap.find( tName );
												if( tm != texMap.end() )
												{
													(*tm).second.refCount++;
												}
												else
												{
													TexRef ref;
													ref.texName		= tName;
													ref.refCount	= 1;
													texMap.insert( std::pair< gpstring, TexRef >( tName, ref ) );
												}
											}
										}
										else
										{
											noNamingKeyTextures.insert( localized );
										}
									}
								}
							}
							else
							{
								if( nKey.BuildIMGLocation( (*t), tName ) )
								{
									if( !DoesResourceFileExist( tName ) )
									{
										noFileTextures.insert( tName );
									}
									else
									{
										tm	= texMap.find( tName );
										if( tm != texMap.end() )
										{
											(*tm).second.refCount++;
										}
										else
										{
											TexRef ref;
											ref.texName		= tName;
											ref.refCount	= 1;
											texMap.insert( std::pair< gpstring, TexRef >( tName, ref ) );
										}
									}
								}
								else
								{
									noNamingKeyTextures.insert( (*t) );
								}
							}
						}
					}
					else
					{
						// Regular SNO
						for( std::list< gpstring >::iterator t = texList.begin(); t != texList.end(); ++t )
						{
							if( nKey.BuildIMGLocation( (*t), tName ) )
							{
								if( !DoesResourceFileExist( tName ) )
								{
									noFileTextures.insert( tName );
								}
								else
								{
									tm	= texMap.find( tName );
									if( tm != texMap.end() )
									{
										(*tm).second.refCount++;
									}
									else
									{
										TexRef ref;
										ref.texName		= tName;
										ref.refCount	= 1;
										texMap.insert( std::pair< gpstring, TexRef >( tName, ref ) );
									}
								}
							}
							else
							{
								noNamingKeyTextures.insert( (*t) );
							}
						}
					}
				}
			}
		}

		outputStr	= "Texture reference counts:                                                                       [REFS]\n";
		output.WriteText( outputStr, outputStr.length() );

		// Go through all of the textures on the disk and check them against the referenced list
		RecursiveFinder finder( "art\\bitmaps\\terrain\\*.psd" );

		std::multimap< int, TexRef, std::greater<int> > orderedTexMap;

		DWORD usedTexSpace		= 0;
		DWORD unusedTexSpace	= 0;
		gpstring texName;
		while( finder.GetNext( texName, true ) )
		{
			// Get the size of this file
			FileHandle tex_file( texName );
			DWORD fileSize	= tex_file.GetSize();
			tex_file.Close();

			stringtool::ReplaceExtension( texName, ".%img%" );

			tm = texMap.find( texName );
			if( tm != texMap.end() )
			{
				orderedTexMap.insert( std::pair< int, TexRef >( (*tm).second.refCount, (*tm).second ) );
				usedTexSpace	+= fileSize;
			}
			else
			{
				TexRef ref;
				ref.texName		= texName;
				ref.refCount	= 0;
				orderedTexMap.insert( std::pair< int, TexRef >( 0, ref ) );
				unusedTexSpace	+= fileSize;

				// Delete file if cleanup is requested
				if( g_bCleanup )
				{
					stringtool::ReplaceExtension( texName, ".psd" );
					DeleteFile( home_dir + texName );
					stringtool::ReplaceExtension( texName, ".gas" );
					DeleteFile( home_dir + texName );
				}
			}
		}

		for( std::multimap< int, TexRef, std::greater<int> >::iterator mtm = orderedTexMap.begin(); mtm != orderedTexMap.end(); ++mtm )
		{
			stringtool::RemoveFrontDelimitedString( (*mtm).second.texName, '\\', outputStr );
			stringtool::RemoveFrontDelimitedString( outputStr, '\\', outputStr );
			stringtool::RemoveFrontDelimitedString( outputStr, '\\', outputStr );
			stringtool::RemoveExtension( outputStr );

			outputStr	= "    " + outputStr;
			stringtool::PadBackToLength( outputStr, ' ', 90 );
			output.WriteF( "%s = %8d\n", outputStr.c_str(), (*mtm).second.refCount );
		}
		outputStr	= "\n    Total used texture space";
		stringtool::PadBackToLength( outputStr, ' ', 90 );
		output.WriteF( "%s = %u\n", outputStr.c_str(), usedTexSpace );

		outputStr	= "    Total unused texture space";
		stringtool::PadBackToLength( outputStr, ' ', 90 );
		output.WriteF( "%s = %u\n", outputStr.c_str(), unusedTexSpace );


		outputStr	= "\n-----------------------------------------------------------------------------------------------------\n";
		output.WriteText( outputStr, outputStr.length() );

		printf( "Writing error log information to output file.\n" );

		// Output text into the output file designated this block of errors
		outputStr	= "SNO's who failed naming key check:\n";		
		output.WriteText( outputStr, outputStr.length() );

		for( std::list< gpstring >::iterator b = noNamingKeyList.begin(); b != noNamingKeyList.end(); ++b )
		{
			// Output text into the output file
			outputStr	= (*b) + "\n";
			output.WriteText( outputStr , outputStr.length() );
		}

		// Output text into the output file designated this block of errors
		outputStr	= "\nSNO's who do not have a file:\n";
		output.WriteText( outputStr, outputStr.length() );

		for( b = noFileList.begin(); b != noFileList.end(); ++b )
		{
			// Output text into the output file
			outputStr	= (*b) + "\n";
			output.WriteText( outputStr , outputStr.length() );
		}

		// Output text into the output file designated this block of errors
		outputStr	= "\nSNO's who are generic but are not setup correctly:\n";
		output.WriteText( outputStr, outputStr.length() );

		for( b = badGenericList.begin(); b != badGenericList.end(); ++b )
		{
			// Output text into the output file
			outputStr	= (*b) + "\n";
			output.WriteText( outputStr , outputStr.length() );
		}

		// Output text into the output file designated this block of errors
		outputStr	= "\nSNO's that have duplicate mesh guids:\n";
		output.WriteText( outputStr, outputStr.length() );

		for( b = duplicateGuidList.begin(); b != duplicateGuidList.end(); ++b )
		{
			// Output text into the output file
			outputStr	= (*b) + "\n";
			output.WriteText( outputStr , outputStr.length() );
		}

		// Output text into the output file designated this block of errors
		outputStr	= "\nMesh guids used in a map but not in a misc file:\n";
		output.WriteText( outputStr, outputStr.length() );

		for( std::set< int >::iterator bg = badGuids.begin(); bg != badGuids.end(); ++bg )
		{
			// Output text into the output file
			output.WriteF( "0x%x\n", (*bg) );
		}

		// Output text into the output file designated this block of errors
		outputStr	= "\nSNO files that have no entry in a misc file:\n";
		output.WriteText( outputStr, outputStr.length() );

		for( b = nomiscentryNodesList.begin(); b != nomiscentryNodesList.end(); ++b )
		{
			// Output text into the output file
			outputStr	= (*b) + "\n";
			output.WriteText( outputStr , outputStr.length() );
		}

		// Output text into the output file designated this block of errors
		outputStr	= "\nTextures that failed naming key check:\n";
		output.WriteText( outputStr, outputStr.length() );

		for( std::set< gpstring, istring_less >::iterator ts = noNamingKeyTextures.begin(); ts != noNamingKeyTextures.end(); ++ts )
		{
			// Output text into the output file
			stringtool::RemoveFrontDelimitedString( (*ts), '\\', outputStr );
			stringtool::RemoveFrontDelimitedString( outputStr, '\\', outputStr );
			stringtool::RemoveFrontDelimitedString( outputStr, '\\', outputStr );
			stringtool::RemoveExtension( outputStr );

			outputStr	+= "\n";

			output.WriteText( outputStr , outputStr.length() );
		}

		// Output text into the output file designated this block of errors
		outputStr	= "\nTextures that have no file:\n";
		output.WriteText( outputStr, outputStr.length() );

		for( ts = noFileTextures.begin(); ts != noFileTextures.end(); ++ts )
		{
			// Output text into the output file
			stringtool::RemoveFrontDelimitedString( (*ts), '\\', outputStr );
			stringtool::RemoveFrontDelimitedString( outputStr, '\\', outputStr );
			stringtool::RemoveFrontDelimitedString( outputStr, '\\', outputStr );
			stringtool::RemoveExtension( outputStr );

			outputStr	+= "\n";

			output.WriteText( outputStr , outputStr.length() );
		}

		// Output text into the output file designated this block of errors
		outputStr	= "\nObjects whose positional node does not match their index node:\n";
		output.WriteText( outputStr, outputStr.length() );

		for( b = scidIndexMismatchList.begin(); b != scidIndexMismatchList.end(); ++b )
		{
			// Output text into the output file
			outputStr	= (*b) + "\n";
			output.WriteText( outputStr , outputStr.length() );
		}

		// Output text into the output file designated this block of errors
		outputStr	= "\nObjects which are in the world but not in their corresponding content index:\n";
		output.WriteText( outputStr, outputStr.length() );

		for( b = scidMissingList.begin(); b != scidMissingList.end(); ++b )
		{
			// Output text into the output file
			outputStr	= (*b) + "\n";
			output.WriteText( outputStr , outputStr.length() );
		}


		// Close the output file
		output.Close();

		printf( "Complete!\n" );
	}

	return 0;
}
