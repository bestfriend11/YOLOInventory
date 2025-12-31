/******************************************************************************************
**
**								SiegeNode
**
**		See .h file for details
**
******************************************************************************************/

#include "precomp_siege.h"
#include "gpcore.h"
#include "gpprofiler.h"
#include "space_3.h"

#include "siege_cache.h"
#include "siege_cache_handle.h"
#include "siege_engine.h"
#include "siege_loader.h"
#include "siege_node.h"
#include "siege_mesh.h"
#include "siege_logical_mesh.h"
#include "siege_logical_node.h"

#include "namingkey.h"

using namespace siege;

//========================================================================

SiegeNode::SiegeNode(void)
	: m_MeshGUID(UNDEFINED_GUID)
#if !GP_RETAIL
	, m_meshChecksum( 0 )
#endif
	, m_pLogicalNodes( NULL )
	, m_numLogicalNodes( 0 )
	, m_pMeshHandle( 0 )
	, m_bVisible( true )
	, m_bVisibleThisFrame( false )
	, m_bLit( false )
	, m_LastVisited( UINT_MAX )
	, m_APVisited( UINT_MAX )
	, m_sphereRadius( 0.0f )
	, m_pStaticVertexColors( 0 )
	, m_pVertexColors( 0 )
	, m_numVertices( 0 )
	, m_AmbientColor( 0xFFFFFFFF )
	, m_ActorAmbientColor( 0xFFFFFFFF )
	, m_ObjectAmbientColor( 0xFFFFFFFF )
	, m_CameraDistance( 0.0f )
	, m_Alpha( 255 )
	, m_CameraAlpha( 255 )
	, m_RegionID( -1 )
	, m_Section( -1 )
	, m_Level( -1 )
	, m_Object( -1 )
	, m_FrustumOwned( 0 )
	, m_SpaceFrustum( 0 )
	, m_LoadedOwned( 0 )
	, m_InterestOwned( 0 )
	, m_UpgradedOwned( 0 )
	, m_bLoadedPhysical( false )
	, m_bValidSpace( false )
	, m_bLoadedLastUpdate( false )
	, m_bUpdateHotpoints( false )
	, m_bAutoStitch( false )
	, m_bLightLocked( false )
	, m_bHasAlphaStages( false )
	, m_bNoAlphaSortStages( false )
	, m_EnvironmentMap( 0 )
	, m_sTextureSetAbbr("")
	, m_sTextureSetVersion("")
	, m_bOccludesLight( true )
	, m_bOccludesCamera( true )
	, m_bBoundsCamera( true )
	, m_bCameraFade( false )
	, m_bAccumulateRequested( false )
	, m_bLightingChanged( true )
	, m_fileHandle( 0 )
{
}

SiegeNode::~SiegeNode(void)
{
	gSiegeEngine.NotifyNodeWorldMembershipChanged( *this, 0x00000000, true );

	if( m_pVertexColors )
	{
		delete[] m_pVertexColors;
	}

	if( m_pStaticVertexColors )
	{
		delete[] m_pStaticVertexColors;
	}

	DWORD index = 0;
	for( NodeTexColl::const_iterator i = m_TextureListing.begin(); i != m_TextureListing.end(); ++i, ++index )
	{
		// Cancel outstanding orders in the manager
		if( !m_TextureLoadListing.empty() && LoadMgr::DoesSingletonExist() )
		{
			if( !gSiegeLoadMgr.Cancel( m_TextureLoadListing[ index ] ) )
			{
				gSiegeEngine.Renderer().UnloadTexture( (*i).textureId );
			}
		}

		gSiegeEngine.Renderer().DestroyTexture( (*i).textureId );
	}
	m_TextureLoadListing.clear();

	if( m_EnvironmentMap )
	{
		gSiegeEngine.Renderer().DestroyTexture( m_EnvironmentMap );
	}

	for( SiegeLightList::const_iterator l = m_LoadedStaticLights.begin(); l != m_LoadedStaticLights.end(); ++l )
	{
		if( (*l).m_pLightEffect )
		{
			delete[] (*l).m_pLightEffect;
		}
	}

	for( l = m_StaticLights.begin(); l != m_StaticLights.end(); ++l )
	{
		if( (*l).m_pLightEffect )
		{
			delete[] (*l).m_pLightEffect;
		}

		gSiegeEngine.LightDatabase().UnRegisterLight( m_Guid, (*l).m_Guid );
	}
	for( l = m_DynamicLights.begin(); l != m_DynamicLights.end(); ++l )
	{
		if( (*l).m_pLightEffect )
		{
			delete[] (*l).m_pLightEffect;
		}

		gSiegeEngine.LightDatabase().UnRegisterLight( m_Guid, (*l).m_Guid );
	}

	for( DecalIndexList::iterator di = m_DecalList.begin(); di != m_DecalList.end(); ++di )
	{
		gSiegeDecalDatabase.DestroyDecal( (*di), false );
	}

	if( m_pMeshHandle )
	{
		// Synchro threads
		kerneltool::Critical::Lock autoLock( gSiegeEngine.GetMeshCritical() );

		// See if we are the last one holding a reference to this mesh
		if( m_pMeshHandle->GetReferenceCountOfObject() == 1 )
		{
			// Get rid of our handle so that the reference count is zero
			delete m_pMeshHandle;

			// Remove the slot from our cache
			gSiegeEngine.MeshCache().RemoveSlotFromCache( m_MeshGUID );
		}
		else
		{
			delete m_pMeshHandle;			
		}
	}

	if( m_pLogicalNodes )
	{
		for( unsigned int i = 0; i < m_numLogicalNodes; ++i )
		{
			delete m_pLogicalNodes[i];
		}

		delete[] m_pLogicalNodes;
	}

	for( SiegeNodeDoorIter d = m_DoorList.begin(); d != m_DoorList.end(); ++d )
	{
		delete (*d);
	}

	gSiegeEngine.NodeDatabase().ReleaseLogicalFile( m_Guid, m_fileHandle );
}

#if !GP_RETAIL

// Verify that the stored checksum matches the assigned mesh
bool SiegeNode::VerifyChecksum()
{
	// Get the mesh
	const SiegeMesh& mesh	= m_pMeshHandle->RequestObject( gSiegeEngine.MeshCache() );

	if( mesh.GetMeshChecksum() != m_meshChecksum )
	{
		return false;
	}
	return true;
}

#endif

// Loading
void SiegeNode::Load( const char *pData )
{
	GPPROFILERSAMPLE( "SiegeNode::Load()", SP_LOAD );

	// Get the file version
	unsigned int version = *((unsigned int *)pData);
	pData += sizeof( unsigned int );

	if( version < 5 )
	{
		gpwarning( "Old logical node detected.  Possible corruption." );
		return;
	}

	// Step past the checksum
	if( version < 10 )
	{
		pData += (sizeof( unsigned int ) * 2) + (sizeof( double ) * 3);
	}
	else
	{
#if		!GP_RETAIL
		m_meshChecksum	= *((DWORD*)pData);
#endif
		pData			+= sizeof( DWORD );
	}

	// Read in min and max bounds
	m_minBox	= *((vector_3*)pData);
	pData		+= sizeof( vector_3 );
	m_maxBox	= *((vector_3*)pData);
	pData		+= sizeof( vector_3 );

	// Get the number of logical nodes that we need to get from this file
	unsigned int num_lnodes = *((unsigned int *)pData);
	pData += sizeof( unsigned int );

	{
		// Synchro threads
		kerneltool::Critical::Lock autoLock( gSiegeEngine.GetLogicalCritical() );

		// Create logical node arrays
		m_numLogicalNodes	= num_lnodes;
		m_pLogicalNodes		= new SiegeLogicalNode*[ num_lnodes ];
		memset( m_pLogicalNodes, 0, sizeof( SiegeLogicalNode* ) * num_lnodes );

		// Iterate over each of the logical nodes, reading them in one at a time
		for( unsigned int l = 0; l < num_lnodes; ++l )
		{
			SiegeLogicalNode* pNode = new SiegeLogicalNode;
			pNode->Load( this, pData );

			gpassert( pNode->GetID() < num_lnodes && pNode->GetID() >= 0 );
			gpassert( pNode->GetID() == l );
			m_pLogicalNodes[ pNode->GetID() ] = pNode;
		}
	}

	// Read in the number of vertices
	m_numVertices = *((unsigned int *)pData);
	pData += sizeof( unsigned int );

	// Read in the number of lights that is written out here
	unsigned int numLights = *((unsigned int*)pData);
	pData += sizeof( unsigned int );

	{
		// Synchro threads
		kerneltool::Critical::Lock autoLock( gSiegeEngine.GetLightCritical() );

		// Go through and read in the lights
		for( unsigned int i = 0; i < numLights; ++i )
		{
			SiegeNodeLight light;

			// Read in the guid that identifies this light
			light.m_Guid	= database_guid( *((unsigned int *)pData) );
			pData += sizeof( unsigned int );

			// Read in the position
			memcpy( &light.m_Position, pData, sizeof( vector_3 ) );
			pData += sizeof( vector_3 );

			// Read in the direction
			memcpy( &light.m_Direction, pData, sizeof( vector_3 ) );
			pData += sizeof( vector_3 );

			// Read in the light descriptor
			if( version == 5 )
			{
				memcpy( &light.m_Descriptor, pData, 32 );
				light.m_Descriptor.m_bAffectsActors		= true;
				light.m_Descriptor.m_bAffectsItems		= true;
				light.m_Descriptor.m_bAffectsTerrain	= true;
				light.m_Descriptor.m_bOnTimer			= false;
				pData += 32;
			}
			else
			{
				memcpy( &light.m_Descriptor, pData, sizeof( LightDescriptor ) );
				pData += sizeof( LightDescriptor );

				if( version == 6 )
				{
					light.m_Descriptor.m_bOnTimer		= false;
				}
			}

			// We know the size of the intensity map must be the number of verts
			if( light.m_Descriptor.m_bAffectsTerrain )
			{
				light.m_pLightEffect	= new float[ m_numVertices ];
				memcpy( light.m_pLightEffect, pData, sizeof( float ) * m_numVertices );
				pData += sizeof( float ) * m_numVertices;
			}
			else
			{
				light.m_pLightEffect	= NULL;
			}

			// With the light read in, we need to put it in the correct list
			if( light.m_Descriptor.m_Subtype != LS_STATIC )
			{
				gperror( "Non-static light stored in .LNC file!" );
			}

			InsertLightInList( light, m_LoadedStaticLights );
		}
	}

	if( version >= 8 )
	{
		// Synchro threads
		kerneltool::Critical::Lock autoLock( gSiegeEngine.GetHotpointCritical() );

		// Read in hotpoints
		unsigned int numHotpoints = *((unsigned int *)pData);
		pData += sizeof( unsigned int );

		for( unsigned int i = 0; i < numHotpoints; ++i )
		{
			// Get the id
			unsigned int id	= *((unsigned int *)pData);
			pData += sizeof( unsigned int );

			// Get the direction
			vector_3 direction( DoNotInitialize );
			memcpy( &direction, pData, sizeof( vector_3 ) );
			pData += sizeof( vector_3 );

			// Add this hotpoint to our mapping
			m_HotpointMap.insert( std::pair< unsigned int, vector_3 >( id, direction ) );
		}
	}

	if( version >= 9 )
	{
		// Read in decals
		unsigned int numDecals = *((unsigned int *)pData);
		pData += sizeof( unsigned int );

		for( unsigned int i = 0; i < numDecals; ++i )
		{
			// Load this decal
			unsigned int decal_index = gSiegeDecalDatabase.LoadDecal( pData );

			// Tell the node about it
			m_DecalList.push_back( decal_index );
		}
	}
}

// Submit texture load requests
void SiegeNode::UpgradeLoad()
{
	gpassert( m_bLoadedPhysical );

	for( NodeTexColl::iterator i = m_TextureListing.begin(); i != m_TextureListing.end(); ++i )
	{
		// Create texture load orders
		m_TextureLoadListing.push_back( gSiegeLoadMgr.LoadById( LOADER_TEXTURE_TYPE, (*i).textureId, true ) );
	}

	for( DecalIndexList::iterator d = m_DecalList.begin(); d != m_DecalList.end(); ++d )
	{
		// Upgrade decals
		gSiegeDecalDatabase.UpgradeLoadDecal( (*d) );
	}
}

// Submit texture load requests
void SiegeNode::DowngradeLoad()
{
	gpassert( m_bLoadedPhysical );

	DWORD index	= 0;
	for( NodeTexColl::iterator i = m_TextureListing.begin(); i != m_TextureListing.end(); ++i, ++index )
	{
		// Cancel texture load orders
		if( !m_TextureLoadListing.empty() && LoadMgr::DoesSingletonExist() )
		{
			// Unload the texture
			if( !gSiegeLoadMgr.Cancel( m_TextureLoadListing[ index ] ) )
			{
				gDefaultRapi.UnloadTexture( (*i).textureId );
			}
		}
	}
	m_TextureLoadListing.clear();

	for( DecalIndexList::iterator d = m_DecalList.begin(); d != m_DecalList.end(); ++d )
	{
		// Downgrade decals
		gSiegeDecalDatabase.DowngradeLoadDecal( (*d) );
	}
}

// Set the mesh guid
void SiegeNode::SetMeshGUID( const database_guid& g, bool bFullLoad )
{
	// Set the GUID
	m_MeshGUID	= g;

	{
		// Synchro threads
		kerneltool::Critical::Lock autoLock( gSiegeEngine.GetMeshCritical() );

		// Setup the mesh handle
		if( m_pMeshHandle )
		{
			delete m_pMeshHandle;

			DWORD index = 0;
			for( NodeTexColl::const_iterator i = m_TextureListing.begin(); i != m_TextureListing.end(); ++i, ++index )
			{
				// Cancel outstanding orders in the manager
				if( !m_TextureLoadListing.empty() && LoadMgr::DoesSingletonExist() )
				{
					if( !gSiegeLoadMgr.Cancel( m_TextureLoadListing[ index ] ) )
					{
						gSiegeEngine.Renderer().UnloadTexture( (*i).textureId );
					}
				}

				gSiegeEngine.Renderer().DestroyTexture( (*i).textureId );
			}
			m_TextureListing.clear();
			m_TextureLoadListing.clear();
		}

		m_pMeshHandle	= new SiegeMeshHandle( gSiegeEngine.MeshCache().UseObject( g ) );
	}

	// Get the mesh
	const SiegeMesh& mesh	= m_pMeshHandle->RequestObject( gSiegeEngine.MeshCache() );

#if	!GP_RETAIL
	if( (mesh.GetMeshChecksum() != m_meshChecksum) && gSiegeEngine.GetOptions().ErrorReporting() )
	{
		gpfatalf(( "Checksum for node %s does not match owned mesh %s.  Please recalc the map.", m_Guid.ToString().c_str(), g.ToString().c_str() ));
	}
/*
	if( mesh.GetMeshChecksum() != m_meshChecksum ) )
	{
		if( gSiegeEngine.GetOptions().ErrorReporting() )
		{
			gpfatalf(( "Checksum for node %s does not match owned mesh %s.  Please recalc the map.", m_Guid.ToString().c_str(), g.ToString().c_str() ));
		}

		// The following is correctional code to keep other client applications, such as editors, from crashing
		// when a mesh no longer matches it's saved LNC data.

		// Remove lights
		for( SiegeLightList::const_iterator l = m_LoadedStaticLights.begin(); l != m_LoadedStaticLights.end(); ++l )
		{
			if( (*l).m_pLightEffect )
			{
				delete[] (*l).m_pLightEffect;
			}
		}
		m_LoadedStaticLights.clear();

		// Remove decals
		for( DecalIndexList::iterator di = m_DecalList.begin(); di != m_DecalList.end(); ++di )
		{
			gSiegeDecalDatabase.DestroyDecal( (*di), false );
		}
	}
*/
#endif

	// Fixup internal pointers to SiegeLogicalMesh structures from their SiegeLogicalNode counterparts
	for( unsigned int l = 0; l < m_numLogicalNodes; ++l )
	{
		gpassert( mesh.GetLogicalMeshes()[ l ].GetID() == m_pLogicalNodes[ l ]->GetID() );
		m_pLogicalNodes[ l ]->SetLogicalMesh( &mesh.GetLogicalMeshes()[ l ] );
	}

	// Build the texture list
	m_bHasAlphaStages		= false;
	m_bNoAlphaSortStages	= false;
	std::vector< gpstring >::const_iterator i = mesh.GetTextureStrings().begin();
	for( ; i != mesh.GetTextureStrings().end(); ++i )
	{
		gpstring filename;
		gpstring localized(*i);
		localized.to_lower();

		unsigned int pos = localized.find("xxx");
		if( pos != localized.npos )
		{
			// This is a generic texture, we need to localize it to the correct texture set and version
			localized.replace( pos, 3, m_sTextureSetAbbr );
			localized	+= m_sTextureSetVersion;
		}
		
		// Try to load the texture using the naming key to locate it
		if( !gNamingKey.BuildIMGLocation( localized.c_str(), filename ) )
		{
			gperrorf(( "TEXTURE ERROR: SiegeNode loader could not build naming key location for bitmap [%s]", localized.c_str() ));
			gNamingKey.BuildIMGLocation( "b_i_glb_placeholder", filename );
		}

		StaticObjectTex tex;
		tex.textureId	= gSiegeEngine.Renderer().CreateTexture( filename, true, 0, TEX_LOCKED, false );
		tex.alpha		= gSiegeEngine.Renderer().IsTextureAlpha( tex.textureId );
		tex.noalphasort	= gSiegeEngine.Renderer().IsTextureNoAlphaSort( tex.textureId );
		m_TextureListing.push_back( tex );

		if( bFullLoad )
		{
			// Create texture load orders
			m_TextureLoadListing.push_back( gSiegeLoadMgr.LoadById( LOADER_TEXTURE_TYPE, tex.textureId, true ) );
		}

		if( tex.alpha )
		{
			m_bHasAlphaStages		= true;
		}
		if( tex.noalphasort )
		{
			m_bNoAlphaSortStages	= true;
		}
	}

	// Set the sphere radius
	m_sphereRadius	= mesh.BBoxHalfDiag().Length();
}

// Find a texture with the given name in our listing
int SiegeNode::FindTextureIndex( const gpstring& texName )
{
	// Get the mesh
	const SiegeMesh& mesh	= m_pMeshHandle->RequestObject( gSiegeEngine.MeshCache() );
	int index				= 0;

	std::vector< gpstring >::const_iterator i = mesh.GetTextureStrings().begin();
	for( ; i != mesh.GetTextureStrings().end(); ++i, ++index )
	{
		// See if this is our texture
		if( (*i).same_no_case( texName ) )
		{
			// Return the texture index
			return index;
		}
	}

	// Return failure
	return( -1 );
}

// Replace the given texture with a new one
void SiegeNode::ReplaceTexture( const int tex_index, const gpstring& newTexName, bool bFullLoad )
{
	if( tex_index >= (int)m_TextureListing.size() )
	{
		gperrorf(( "Invalid texture index %d on node %s requested for replacement!", tex_index, m_Guid.ToString().c_str() ));
		return;
	}

	// Create the new texture
	gpstring filename;
	gpstring localized( newTexName );
	localized.to_lower();

	unsigned int pos = localized.find("xxx");

	if( pos != localized.npos )
	{
		// This is a generic texture, we need to localize it to the correct texture set and version
		localized.replace( pos, 3, m_sTextureSetAbbr );
		localized += m_sTextureSetVersion;
		gNamingKey.BuildIMGLocation( localized.c_str(), filename );
	}
	else
	{
		// Try to load the texture using the naming key to locate it
		if( !gNamingKey.BuildIMGLocation( localized.c_str(), filename ) )
		{
			// This is a legacy texture
			gperrorf(( "The texture %s could not be loaded because it does not match the naming key!", newTexName.c_str() ));
		}
	}

	// Create the new texture
	StaticObjectTex tex;
	tex.textureId	= gSiegeEngine.Renderer().CreateTexture( filename, true, 0, TEX_LOCKED, bFullLoad );
	tex.alpha		= gSiegeEngine.Renderer().IsTextureAlpha( tex.textureId );
	tex.noalphasort	= gSiegeEngine.Renderer().IsTextureNoAlphaSort( tex.textureId );
	gpassert( tex.textureId );

	// Cancel outstanding orders in the manager
	if( !m_TextureLoadListing.empty() && LoadMgr::DoesSingletonExist() )
	{
		if( !gSiegeLoadMgr.Cancel( m_TextureLoadListing[ tex_index ] ) )
		{
			gSiegeEngine.Renderer().UnloadTexture( m_TextureListing[ tex_index ].textureId );
			m_TextureLoadListing[ tex_index ]	= LOADORDERID_INVALID;
		}
	}

	if( !bFullLoad && IsUpgradedByAnyFrustum() )
	{
		// Create texture load order
		m_TextureLoadListing[ tex_index ]	= gSiegeLoadMgr.LoadById( LOADER_TEXTURE_TYPE, tex.textureId, true );
	}

	// Release the old texture that we are replacing
	gSiegeEngine.Renderer().DestroyTexture( m_TextureListing[ tex_index ].textureId );

	// Set our replacement
	m_TextureListing[ tex_index ]	= tex;

	// Set alpha indicator if we need to
	if( tex.alpha )
	{
		m_bHasAlphaStages		= true;
	}
	if( tex.noalphasort )
	{
		m_bNoAlphaSortStages	= true;
	}
}

SiegeNodeDoor* SiegeNode::GetDoorByID( const DWORD id )
{
	for( SiegeNodeDoorConstIter d = m_DoorList.begin(); d != m_DoorList.end(); ++d )
	{
		if( (*d)->GetID() == id )
		{
			return (*d);
		}
	}

	return NULL;
}

// Add a door to the request queue
void SiegeNode::AddDoor( const DWORD init_id, const vector_3& init_center, const matrix_3x3& init_orient,
						 const database_guid& init_neighbour, const DWORD init_neighbour_id, bool bConnect )
{
	// Add this door to the node
	m_DoorList.push_back( new SiegeNodeDoor( init_id, init_center, init_orient, init_neighbour, init_neighbour_id, bConnect ) );
}

void SiegeNode::AddDoor( const DWORD init_id, const database_guid& init_neighbour, const DWORD init_neighbour_id, bool bConnect )
{
	// Add this door to the node
	m_DoorList.push_back( new SiegeNodeDoor( init_id, init_neighbour, init_neighbour_id, bConnect ) );
}

// Add a remove door request
void SiegeNode::RemoveDoor( const DWORD id )
{
	// Remove this door from the node
	for( SiegeNodeDoorIter d = m_DoorList.begin(); d != m_DoorList.end(); ++d )
	{
		if( (*d)->GetID() == id )
		{
			// Remove this door
			delete (*d);
			m_DoorList.erase( d );
			break;
		}
	}
}

bool SiegeNode::HitTest( const vector_3& RayPos, const vector_3& RayDir, float& RayT, vector_3& FaceNormal, const eLogicalNodeFlags& flags ) const
{
	// Always assume that information passed to this function is in Node space.  If it is not, then
	// we are going to have problems.

	// Initialize our T
	RayT = FLOAT_MAX;

	// Go through all of the logical nodes and only collide with those logical nodes
	// that are marked as humanoid walkable.  Return the hit that is closest.
	float		tempT = 0.0f;
	vector_3	tempFaceNormal( DoNotInitialize );
	
	for( unsigned int i = 0; i < m_numLogicalNodes; ++i )
	{
		SiegeLogicalNode* pLogicalNode = m_pLogicalNodes[i];

		if( pLogicalNode->GetFlags() & flags )
		{
			// Hit test with this logical node
			if( pLogicalNode->HitTest( RayPos, RayDir, tempT, tempFaceNormal ) )
			{
				// Make sure that the new T is less than the existing one
				if( tempT < RayT )
				{
					// Set the variables to pass back to the user
					RayT		= tempT;
					FaceNormal	= tempFaceNormal;
				}
			}
		}
	}

	if( RayT != FLOAT_MAX )
	{
		return true;
	}
	return false;
}

bool SiegeNode::HitTestTri( const vector_3& RayPos, const vector_3& RayDir, float& RayT, TriNorm& Triangle, const eLogicalNodeFlags& flags ) const
{
	// Always assume that information passed to this function is in Node space.  If it is not, then
	// we are going to have problems.

	// Initialize our T
	RayT = FLOAT_MAX;

	// Go through all of the logical nodes and only collide with those logical nodes
	// that are marked as humanoid walkable.  Return the hit that is closest.
	float		tempT = 0.0f;
	TriNorm		tempTri;
	
	for( unsigned int i = 0; i < m_numLogicalNodes; ++i )
	{
		SiegeLogicalNode* pLogicalNode = m_pLogicalNodes[i];

		if( pLogicalNode->GetFlags() & flags )
		{
			// Hit test with this logical node
			if( pLogicalNode->HitTestTri( RayPos, RayDir, tempT, tempTri ) )
			{
				// Make sure that the new T is less than the existing one
				if( tempT < RayT )
				{
					// Set the variables to pass back to the user
					RayT		= tempT;
					Triangle	= tempTri;
				}
			}
		}
	}

	if( RayT != FLOAT_MAX )
	{
		return true;
	}
	return false;
}

bool SiegeNode::HitTestFlags( const vector_3& RayPos, const vector_3& RayDir, float& RayT, eLogicalNodeFlags& flags ) const
{
	// Always assume that information passed to this function is in Node space.  If it is not, then
	// we are going to have problems.

	// Initialize our T
	RayT = FLOAT_MAX;

	// Go through all of the logical nodes and only collide with those logical nodes
	// that are marked as humanoid walkable.  Return the hit that is closest.
	float				tempT = 0.0f;
	vector_3			tempFaceNormal( DoNotInitialize );
	eLogicalNodeFlags	tempFlags	= flags;
	
	for( unsigned int i = 0; i < m_numLogicalNodes; ++i )
	{
		SiegeLogicalNode* pLogicalNode = m_pLogicalNodes[i];

		if( pLogicalNode->GetFlags() & tempFlags )
		{
			// Hit test with this logical node
			if( pLogicalNode->HitTest( RayPos, RayDir, tempT, tempFaceNormal ) )
			{
				// Make sure that the new T is less than the existing one
				if( tempT < RayT )
				{
					// Set the variables to pass back to the user
					RayT		= tempT;
					flags		= (eLogicalNodeFlags)pLogicalNode->GetFlags();
				}
			}
		}
	}

	if( RayT != FLOAT_MAX )
	{
		return true;
	}
	return false;
}

bool SiegeNode::IsPointInNode( const vector_3& position ) const
{
	// Return whether or not the point is in the node's axis aligned bounding box
	if( position.x < m_minBox.x ||
		position.z < m_minBox.z )
	{
		return false;
	}

	if( position.x > m_maxBox.x ||
		position.z > m_maxBox.z )
	{
		return false;
	}

	return true;
}

bool SiegeNode::IsWorldPointInNode( const vector_3& position ) const
{
	// Return whether or not the point is in the node's axis aligned bounding box
	if( position.x < m_wminBox.x ||
		position.z < m_wminBox.z )
	{
		return false;
	}

	if( position.x > m_wmaxBox.x ||
		position.z > m_wmaxBox.z )
	{
		return false;
	}

	return true;
}

// Set the frustum ownership of this node
void SiegeNode::SetFrustumOwnership( DWORD frustId, bool bOwned )
{
	unsigned int oldOwned = m_FrustumOwned;

	gpassert( ::IsPower2( frustId ) );
	if( bOwned )
	{
		m_FrustumOwned |= frustId;
	}
	else
	{
		m_FrustumOwned &= ~frustId;
	}

	if ( oldOwned != m_FrustumOwned )
	{
		gSiegeEngine.NotifyNodeWorldMembershipChanged( *this, oldOwned, false );
	}
}

void SiegeNode::SetLoadedOwnership( DWORD frustId, bool bLoaded )
{
	gpassert( ::IsPower2( frustId ) );
	if( bLoaded )
	{
		m_LoadedOwned |= frustId;
	}
	else
	{
		m_LoadedOwned &= ~frustId;
	}
}

void SiegeNode::SetInterestOwnership( DWORD frustId, bool bInterest )
{
	gpassert( ::IsPower2( frustId ) );
	if( bInterest )
	{
		m_InterestOwned |= frustId;
	}
	else
	{
		m_InterestOwned &= ~frustId;
	}
}

void SiegeNode::SetUpgradedOwnership( DWORD frustId, bool bUpgraded )
{
	unsigned int oldOwned = m_UpgradedOwned;

	gpassert( ::IsPower2( frustId ) );
	if( bUpgraded )
	{
		m_UpgradedOwned |= frustId;
	}
	else
	{
		m_UpgradedOwned &= ~frustId;
	}

	if ( oldOwned != m_UpgradedOwned )
	{
		gSiegeEngine.NotifyNodeUpgradedMembershipChanged( *this, oldOwned );
	}
}

// Find the light in the passed list that has the corresponding guid
SiegeLightListConstIter SiegeNode::FindLightInList( const database_guid guid, const SiegeLightList& list )
{
	for( SiegeLightListConstIter i = list.begin(); i != list.end(); ++i )
	{
		if( (*i).m_Guid == guid )
		{
			return i;
		}
	}

	return list.end();
}

bool SiegeNode::ComputeLighting( SiegeMesh const& mesh )
{
	GPPROFILERSAMPLE( "SiegeNode::ComputeLighting()", SP_DRAW | SP_DRAW_CALC_TERRAIN );

	// Make sure there isn't a problem
	if( m_numVertices != mesh.GetNumberOfVertices() )
	{
		gperrorf(( "Siege node : '%s'\n  Node does not have a valid LNO, and must be recalculated.  Thank you for playing Dungeon Siege.  Please put your keyboard in the upright and locked position before exiting.\n", m_Guid.ToString().c_str()  ));
		return false;
	}

	// Create buffer and copy static data in
	if( !m_pVertexColors )
	{
		m_pVertexColors	= new DWORD[ m_numVertices ];
	}

	bool lightingChanged	= false;
	if( m_pStaticVertexColors && m_bLightingChanged )
	{
		memcpy( m_pVertexColors, m_pStaticVertexColors, sizeof( DWORD ) * m_numVertices );
		lightingChanged		= true;
	}

	// Reset lighting changed
	m_bLightingChanged		= false;

	// Go through all dynamic sources
	for( SiegeLightList::const_iterator l = m_DynamicLights.begin(); l != m_DynamicLights.end(); ++l )
	{
		const LightDescriptor& lDesc	= (*l).m_Descriptor;
		if( lDesc.m_bActive && (*l).m_pLightEffect )
		{
			const float* pLightEffect	= (*l).m_pLightEffect;

			if( IsPositive( lDesc.m_Intensity ) )
			{
				// Calculate the individual colors for this light
				for( unsigned int i = 0; i < m_numVertices; ++i, ++pLightEffect )
				{
					// Check for skip value
					if( (*pLightEffect) > (1.0f + FLOAT_TOLERANCE) )
					{
						DWORD skip		= FTOL( (*pLightEffect) );
						i				+= skip;
						if( i >= m_numVertices )
						{
							break;
						}
						pLightEffect	+= skip;
					}

					fScaleAndAddDWORDColor( ((DWORD*)m_pVertexColors)[ i ], lDesc.m_Color, min_t( 1.0f, lDesc.m_Intensity * (*pLightEffect) ) );
				}
			}
			else if( IsNegative( lDesc.m_Intensity ) )
			{
				const float intensity	= FABSF( lDesc.m_Intensity );

				// Calculate the individual colors for this light
				for( unsigned int i = 0; i < m_numVertices; ++i, ++pLightEffect )
				{
					// Check for skip value
					if( (*pLightEffect) > (1.0f + FLOAT_TOLERANCE) )
					{
						DWORD skip		= FTOL( (*pLightEffect) );
						i				+= skip;
						if( i >= m_numVertices )
						{
							break;
						}
						pLightEffect	+= skip;
					}

					fScaleAndSubDWORDColor( ((DWORD*)m_pVertexColors)[ i ], lDesc.m_Color, min_t( 1.0f, intensity * (*pLightEffect) ) );
				}
			}

			m_bLightingChanged	= true;
			lightingChanged		= true;
		}
	}

	return lightingChanged;
}

// Accumulate the static lights into the static buffer
void SiegeNode::AccumulateStaticLighting()
{
	GPPROFILERSAMPLE( "SiegeNode::AccumulateStaticLighting()", SP_DRAW | SP_DRAW_CALC_TERRAIN );

	// Get the mesh
	SiegeMesh& mesh = m_pMeshHandle->RequestObject( gSiegeEngine.MeshCache() );

	if( mesh.GetNumberOfVertices() != m_numVertices )
	{
		gpwarning( "Could not accumulate lighting because vert counts differ!" );
		return;
	}

	// Update our static lights if some new ones got loaded
	if( !m_LoadedStaticLights.empty() )
	{
		// Synchro threads
		kerneltool::Critical::Lock autoLock( gSiegeEngine.GetLightCritical() );

		for( SiegeLightList::iterator l = m_LoadedStaticLights.begin(); l != m_LoadedStaticLights.end(); ++l )
		{
			// Insert light into static list
			InsertLightInList( (*l), m_StaticLights );

			// Register this light with the database
			gSiegeEngine.LightDatabase().RegisterLight( m_Guid, (*l).m_Guid, (*l) );
		}

		// Manually call destructor to free memory
		m_LoadedStaticLights.clear();
	}

	// If a static buffer already exists, delete it
	if( !m_pStaticVertexColors )
	{
		// Create a new buffer
		m_pStaticVertexColors	= new DWORD[ m_numVertices ];
	}

	// Insert the ambient values into our array of color
	FillDWORDColorStrided( m_pStaticVertexColors, sizeof( DWORD ), m_numVertices, m_AmbientColor );

	// Go through all static sources
	for( SiegeLightList::const_iterator l = m_StaticLights.begin(); l != m_StaticLights.end(); ++l )
	{
		const LightDescriptor& lDesc	= (*l).m_Descriptor;
		if( lDesc.m_bActive && (*l).m_pLightEffect )
		{
			const float* pLightEffect	= (*l).m_pLightEffect;

			if( IsPositive( lDesc.m_Intensity ) )
			{
				// Calculate the individual colors for this light
				for( unsigned int i = 0; i < m_numVertices; ++i, ++pLightEffect )
				{
					// Check for skip value
					if( (*pLightEffect) > (1.0f + FLOAT_TOLERANCE) )
					{
						DWORD skip		= FTOL( (*pLightEffect) );
						i				+= skip;
						if( i >= m_numVertices )
						{
							break;
						}
						pLightEffect	+= skip;
					}

					fScaleAndAddDWORDColor( ((DWORD*)m_pStaticVertexColors)[ i ], lDesc.m_Color, min_t( 1.0f, lDesc.m_Intensity * (*pLightEffect) ) );
				}
			}
			else if( IsNegative( lDesc.m_Intensity ) )
			{
				const float intensity	= FABSF( lDesc.m_Intensity );

				// Calculate the individual colors for this light
				for( unsigned int i = 0; i < m_numVertices; ++i, ++pLightEffect )
				{
					// Check for skip value
					if( (*pLightEffect) > (1.0f + FLOAT_TOLERANCE) )
					{
						DWORD skip		= FTOL( (*pLightEffect) );
						i				+= skip;
						if( i >= m_numVertices )
						{
							break;
						}
						pLightEffect	+= skip;
					}

					fScaleAndSubDWORDColor( ((DWORD*)m_pStaticVertexColors)[ i ], lDesc.m_Color, min_t( 1.0f, intensity * (*pLightEffect) ) );
				}
			}
		}
	}

	// Notify that lighting changed
	m_bLightingChanged = true;
}

void SiegeNode::GetBoundedTriCollection( const vector_3& minBox, const vector_3& maxBox, TriangleColl& retColl ) const
{
	// Reserve enough space in our return collection for everything we might insert
	retColl.reserve( m_numVertices );

	// Build and index collection
	BSPTree::TriangleIndexColl indexListing;
	indexListing.reserve( m_numVertices );

	for( unsigned int i = 0; i < m_numLogicalNodes; ++i )
	{
		SiegeLogicalMesh* pLMesh	= m_pLogicalNodes[i]->GetLogicalMesh();
		if( pLMesh )
		{
			// Get the triangle indices of the bounding area
			pLMesh->GetBSPTree()->BoxIntersectTree( minBox, maxBox, indexListing );

			// Add the new triangles to my main list
			TriNorm* pTriangles	= pLMesh->GetBSPTree()->GetTriangles();
			for( BSPTree::TriangleIndexColl::const_iterator o = indexListing.begin(); o != indexListing.end(); ++o )
			{
				retColl.insert( retColl.end(), pTriangles[(*o)].m_Vertices, &pTriangles[(*o)].m_Vertices[3] );
			}

			// Clear the index list
			indexListing.clear();
		}
	}
}

const vector_3&	SiegeNode::GetHotpointDirection( const DWORD id )
{
	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetHotpointCritical() );

	SiegeHotpointMap::iterator i = m_HotpointMap.find( id );
	if( i != m_HotpointMap.end() )
	{
		return (*i).second;
	}

	return vector_3::ZERO;
}

void SiegeNode::SetHotpointDirection( const DWORD id, const vector_3& dir )
{
	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetHotpointCritical() );

	SiegeHotpointMap::iterator i = m_HotpointMap.find( id );
	if( i != m_HotpointMap.end() )
	{
		(*i).second = dir;
	}
}

void SiegeNode::ClearHotpoints()
{
	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetHotpointCritical() );

	m_HotpointMap.clear();
}

void SiegeNode::InsertHotpoint( const vector_3& dir, const DWORD id )
{
	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetHotpointCritical() );

	SiegeHotpointMap::iterator i = m_HotpointMap.find( id );
	if( i == m_HotpointMap.end() )
	{
		m_HotpointMap.insert( std::pair< unsigned int, vector_3 >( id, dir ) );
	}
	else
	{
		gpassert( "Cannot insert hotpoint because one with this ID already exists!" );
	}
}


void SiegeNode::RemoveHotpoint( const DWORD id )
{
	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetHotpointCritical() );

	SiegeHotpointMap::iterator i = m_HotpointMap.find( id );
	if( i != m_HotpointMap.end() )
	{
		m_HotpointMap.erase( i );
	}
}

// Update the light's description info
void SiegeNode::UpdateLightDescriptor( database_guid guid, LightDescriptor* pDescriptor )
{
	SiegeLightList::iterator i = FindLightInList( guid, m_DynamicLights );
	if( i != m_DynamicLights.end() )
	{
		// Update the dynamic light
		if( (*i).m_Descriptor.m_Type		!= pDescriptor->m_Type			||
			(*i).m_Descriptor.m_InnerRadius	!= pDescriptor->m_InnerRadius	||
			(*i).m_Descriptor.m_OuterRadius	!= pDescriptor->m_OuterRadius )
		{
			// Update the descriptor
			(*i).m_Descriptor				= *pDescriptor;

			// Calc the light
			CalculateLight( guid, (*i) );
		}
		else
		{
			// Update the descriptor
			(*i).m_Descriptor				= *pDescriptor;
		}

		SiegeNodeLight light	= (*i);
		m_DynamicLights.erase( i );
		InsertLightInList( light, m_DynamicLights );
	}
	else
	{
		i = FindLightInList( guid, m_StaticLights );
		if( i != m_StaticLights.end() )
		{
			// Update the descriptor
			(*i).m_Descriptor				= *pDescriptor;

			SiegeNodeLight light	= (*i);
			m_StaticLights.erase( i );
			InsertLightInList( light, m_StaticLights );
		}
	}
}

// Insert the light into this node
void SiegeNode::InsertLight( const database_guid guid, const vector_3& position, const vector_3& direction, const LightDescriptor* pDescriptor )
{
	// Return if the light already exists
	if( pDescriptor->m_Subtype == LS_DYNAMIC )
	{
		if( FindLightInList( guid, m_DynamicLights ) != m_DynamicLights.end() )
		{
			return;
		}
	}
	else if( pDescriptor->m_Subtype == LS_STATIC )
	{
		if( FindLightInList( guid, m_StaticLights ) != m_StaticLights.end() )
		{
			return;
		}
	}

	// Do the calculations for the light
	SiegeNodeLight	light;
	light.m_Guid			= guid;
	light.m_Position		= position;
	light.m_Direction		= direction;
	light.m_Descriptor		= *pDescriptor;
	light.m_pLightEffect	= NULL;

	CalculateLight( guid, light );

	// Insert the new light
	if( pDescriptor->m_Subtype == LS_DYNAMIC )
	{
		InsertLightInList( light, m_DynamicLights );
	}
	else if( pDescriptor->m_Subtype == LS_STATIC )
	{
		InsertLightInList( light, m_StaticLights );
	}
}

// Insert the given light into the passed list, sorting by intensity
void SiegeNode::InsertLightInList( SiegeNodeLight& light, SiegeLightList& list )
{
	for( SiegeLightListIter i = list.begin(); i != list.end(); ++i )
	{
		if( light.m_Descriptor.m_Intensity > (*i).m_Descriptor.m_Intensity )
		{
			list.insert( i, light );
			return;
		}
	}

	// If we get here, then the light goes on the end
	list.push_back( light );
}

// Find the light in the passed list that has the corresponding guid
SiegeLightListIter SiegeNode::FindLightInList( const database_guid guid, SiegeLightList& list )
{
	for( SiegeLightListIter i = list.begin(); i != list.end(); ++i )
	{
		if( (*i).m_Guid == guid )
		{
			return i;
		}
	}

	return list.end();
}

// Destroy the light
void SiegeNode::DestroyLight( database_guid guid, bool bUnRegister )
{
	SiegeLightList::iterator i = FindLightInList( guid, m_DynamicLights );
	if( i != m_DynamicLights.end() )
	{
		if( (*i).m_pLightEffect )
		{
			delete[] (*i).m_pLightEffect;
		}

		m_DynamicLights.erase( i );

		// Unregister from the database
		if( bUnRegister )
		{
			gSiegeEngine.LightDatabase().UnRegisterLight( m_Guid, guid );
		}
	}
	else
	{
		i = FindLightInList( guid, m_StaticLights );
		if( i != m_StaticLights.end() )
		{
			if( (*i).m_pLightEffect )
			{
				delete[] (*i).m_pLightEffect;
			}

			m_StaticLights.erase( i );

			// Unregister from the database
			if( bUnRegister )
			{
				gSiegeEngine.LightDatabase().UnRegisterLight( m_Guid, guid );
			}
		}
	}
}

void SiegeNode::CalculateLight( const database_guid guid, SiegeNodeLight& light )
{
	// Delete old light data if we're not light locked
	if( light.m_pLightEffect && !m_bLightLocked )
	{
		delete[] light.m_pLightEffect;
		light.m_pLightEffect	= NULL;
	}

	// Return if this light doesn't affect terrain
	if( !light.m_Descriptor.m_bAffectsTerrain )
	{
		return;
	}

	// If we have no current light data, create it and clear it
	if( !light.m_pLightEffect )
	{
		light.m_pLightEffect	= new float[ m_numVertices ];
		memset( light.m_pLightEffect, 0, sizeof( float ) * m_numVertices );
	}

	// Return if this node is currently light locked
	if( m_bLightLocked )
	{
		return;
	}

	// Get the mesh
	SiegeMesh& mesh = m_pMeshHandle->RequestObject( gSiegeEngine.MeshCache() );

	// Get the possible occluding nodes if they exist
	SiegeNodeList occludeList;
	if( light.m_Descriptor.m_bOccludeGeometry )
	{
		GetPossibleOccludingNodes( guid, light, occludeList );
	}

	// Zero counters
	int zeroCount						= 0;
	int zeroIndex						= -1;

	// Calculate the lighting
	if( light.m_Descriptor.m_Type == LT_POINT )
	{
		// Calculate the squared falloff
		float SquaredOuterFalloffRadius	= light.m_Descriptor.m_OuterRadius * light.m_Descriptor.m_OuterRadius;
		float InvFalloff				= 1.0f / (light.m_Descriptor.m_OuterRadius - light.m_Descriptor.m_InnerRadius);

		for( unsigned int i = 0; i < m_numVertices; ++i )
		{
			// Get mesh information
			vector_3 *position, *normal;
			mesh.GetLightingInformation( i, &position, &normal );

			// Get this vertices' relative position and check for falloff
			vector_3 const PositionToLight	= light.m_Position - (*position);
			float SquaredDistanceToLight	= PositionToLight.Length2();
			if( SquaredDistanceToLight <= SquaredOuterFalloffRadius )
			{
				// Make sure no occlusion crap is going on
				if( light.m_Descriptor.m_bOccludeGeometry )
				{
					if( RayIsBlocked( *position, light.m_Position, occludeList ) )
					{
						// Vert is not affected by this light
						light.m_pLightEffect[i]	= 0.0f;
						++zeroCount;
						if( zeroIndex == -1 )
						{
							zeroIndex = i;
						}
						continue;
					}
				}

				// Get the real distance
				float DistanceToLight	= SquareRoot( SquaredDistanceToLight );

				// Calculate lighting info
				float Falloff			= 1.0f;
				if( DistanceToLight > light.m_Descriptor.m_InnerRadius )
				{
					Falloff -= ( (DistanceToLight - light.m_Descriptor.m_InnerRadius) * InvFalloff );
				}

				float Incidence			= 1.0f;
				if( !IsZero( PositionToLight ) )
				{
					Incidence			= InnerProduct( Normalize( PositionToLight ), *normal );
					Incidence			= Maximum(Incidence, 0.0f);
				}
				Falloff					*= Incidence;
				light.m_pLightEffect[i]	= Falloff;
				if( IsZero( Falloff ) )
				{
					// Setup zero counts and index
					++zeroCount;
					if( zeroIndex == -1 )
					{
						zeroIndex = i;
					}
				}
				else
				{
					// Check out zero count to go set the proper values
					if( zeroCount > 1 )
					{
						light.m_pLightEffect[ zeroIndex ]	= (float)zeroCount;
					}
					zeroIndex	= -1;
					zeroCount	= 0;
				}
			}
			else
			{
				// Setup zero counts and index
				++zeroCount;
				if( zeroIndex == -1 )
				{
					zeroIndex = i;
				}
			}
		}
	}
	else if( light.m_Descriptor.m_Type == LT_DIRECTIONAL )
	{
		// Calculate directional vectors
		vector_3 nvSun		= Normalize( light.m_Direction );
		vector_3 longSun	= nvSun * 500.0f;

		for( unsigned int i = 0; i < m_numVertices; ++i )
		{
			// Get mesh information
			vector_3 *position, *normal;
			mesh.GetLightingInformation( i, &position, &normal );

			// Make sure no occlusion crap is going on
			if( light.m_Descriptor.m_bOccludeGeometry )
			{
				if( RayIsBlocked( *position, (*position) + longSun, occludeList ) )
				{
					// Vert is not affected by this light
					light.m_pLightEffect[i]	= 0.0f;
					++zeroCount;
					if( zeroIndex == -1 )
					{
						zeroIndex = i;
					}
					continue;
				}
			}

			// Figure out the incidence
			float Incidence			= InnerProduct( nvSun, *normal );
			Incidence				= Maximum(Incidence, 0.0f);
			light.m_pLightEffect[i] = Incidence;
			if( IsZero( Incidence ) )
			{
				// Setup zero counts and index
				++zeroCount;
				if( zeroIndex == -1 )
				{
					zeroIndex = i;
				}
			}
			else
			{
				// Check out zero count to go set the proper values
				if( zeroCount > 1 )
				{
					light.m_pLightEffect[ zeroIndex ]	= (float)zeroCount;
				}
				zeroIndex	= -1;
				zeroCount	= 0;
			}
		}
	}
	else if( light.m_Descriptor.m_Type == LT_SPOT )
	{
		for( unsigned int i = 0; i < m_numVertices; ++i )
		{
			// Get mesh information
			vector_3 *position, *normal;
			mesh.GetLightingInformation( i, &position, &normal );

			// Find the closest point along the axis of the cone
			vector_3 closest_point			= ClosestPointOnLine( light.m_Position, light.m_Direction, (*position) );

			// Use the length to determine if point lies within cone
			float SquaredDistanceToLight	= Length2( (*position) - closest_point );
			float SquaredDistToFocus		= Length2( closest_point - light.m_Position );
			float OuterAngleTangent			= tanf( light.m_Descriptor.m_OuterRadius );

			if( SquaredDistanceToLight <= (SquaredDistToFocus * (OuterAngleTangent * OuterAngleTangent)) )
			{
				// Make sure no occlusion crap is going on
				if( light.m_Descriptor.m_bOccludeGeometry )
				{
					if( RayIsBlocked( *position, light.m_Position, occludeList ) )
					{
						// Vert is not affected by this light
						light.m_pLightEffect[i]	= 0.0f;
						++zeroCount;
						if( zeroIndex == -1 )
						{
							zeroIndex = i;
						}
						continue;
					}
				}

				// Get the real distance
				float DistanceToLight	= SquareRoot( SquaredDistanceToLight );

				float DistToFocus		= SquareRoot( SquaredDistToFocus );
				float OuterDistance		= DistToFocus * OuterAngleTangent;
				float InnerDistance		= DistToFocus * tanf( light.m_Descriptor.m_InnerRadius );

				// Calculate lighting info
				float Falloff			= 1.0f;
				if( DistanceToLight > InnerDistance )
				{
					Falloff -= ( (DistanceToLight - InnerDistance) / (OuterDistance - InnerDistance) );
				}

				float Incidence			= 1.0f;
				vector_3 PosToLight		= Normalize( light.m_Position - (*position) );
				if( !IsZero( PosToLight ) )
				{
					Incidence			= InnerProduct( PosToLight, *normal );
					Incidence			= Maximum(Incidence, 0.0f);
				}
				Falloff					*= Incidence;
				light.m_pLightEffect[i]	= Falloff;
				if( IsZero( Falloff ) )
				{
					// Setup zero counts and index
					++zeroCount;
					if( zeroIndex == -1 )
					{
						zeroIndex = i;
					}
				}
				else
				{
					// Check out zero count to go set the proper values
					if( zeroCount > 1 )
					{
						light.m_pLightEffect[ zeroIndex ]	= (float)zeroCount;
					}
					zeroIndex	= -1;
					zeroCount	= 0;
				}
			}
			else
			{
				// Setup zero counts and index
				++zeroCount;
				if( zeroIndex == -1 )
				{
					zeroIndex = i;
				}
			}
		}
	}

	// Set the values if zero's tail the end
	if( zeroCount > 1 )
	{
		light.m_pLightEffect[ zeroIndex ]	= (float)zeroCount;
	}
}

// Get the nodes that may occlude me
void SiegeNode::GetPossibleOccludingNodes( const database_guid guid, const SiegeNodeLight& light, SiegeNodeList& slist )
{
	// If this is a point or spot light source, then we just need to test with the nodes that
	// are affected by the light.  Since the LightDatabase knows this information already, we
	// can just snag it from there
	if( light.m_Descriptor.m_Type	== LT_POINT	||
		light.m_Descriptor.m_Type	== LT_SPOT	)
	{
		LightMap::const_iterator i = gSiegeEngine.LightDatabase().GetLightDatabaseMap().find( guid );
		if( i != gSiegeEngine.LightDatabase().GetLightDatabaseMap().end() )
		{
			// Go through list of nodes and put in return list, exluding myself
			SiegeNodeList::const_iterator n = (*i).second.m_Nodes.begin();
			for( ; n != (*i).second.m_Nodes.end(); ++n )
			{
				if( !((*n).GetGuid() == m_Guid) )
				{
					slist.push_back( (*n) );
				}
			}
		}
	}
	else if( light.m_Descriptor.m_Type	== LT_DIRECTIONAL )
	{
		// We can do some quick work here to help reduce the number of nodes that are required for
		// testing.

		// Start making a stack to use
		SiegeNodeList nList;

		// Get visit counter
		unsigned int visitcount	= RandomDword();
		SetAPVisited( visitcount );

		// Put red leader on the stack
		for( SiegeNodeDoorConstIter d = m_DoorList.begin(); d != m_DoorList.end(); ++d )
		{
			if( !gSiegeEngine.IsNodeValid( (*d)->GetNeighbour() ) )
			{
				continue;
			}

			SiegeNodeHandle far_handle( gSiegeEngine.NodeCache().UseObject( (*d)->GetNeighbour() ) );
			SiegeNode& nnode = far_handle.RequestObject( gSiegeEngine.NodeCache() );

			nnode.SetAPVisited( visitcount );
			nList.push_back( far_handle );
		}

		// Get the world position
		float cone_angle				= RadiansFrom( 5 );
		vector_3 HalfDiag				= (m_maxBox - m_minBox) * 0.5f;
		vector_3 WorldSpaceDirection	= m_CurrentOrientation * Normalize( light.m_Direction );
		vector_3 WorldSpacePosition		= (m_wminBox + HalfDiag) - 
										  (WorldSpaceDirection * (HalfDiag.Length() / tanf( cone_angle )));

		while( !nList.empty() )
		{
			SiegeNodeHandle far_handle	= nList.back();
			SiegeNode& far_node			= far_handle.RequestObject( gSiegeEngine.NodeCache() );
			nList.pop_back();

			if( far_node.IsOwnedByAnyFrustum() )
			{
				const vector_3& min_box	= far_node.GetWorldSpaceMinimumBounds();
				HalfDiag				= (far_node.GetWorldSpaceMaximumBounds() - min_box) * 0.5f;

				if( SphereIntersectsCone( WorldSpacePosition, WorldSpaceDirection, cone_angle,
										  min_box + HalfDiag, HalfDiag.Length() ) )
				{
					slist.push_back( far_handle );
				}

				for( SiegeNodeDoorConstIter d = far_node.GetDoors().begin(); d != far_node.GetDoors().end(); ++d )
				{
					if( !gSiegeEngine.IsNodeValid( (*d)->GetNeighbour() ) )
					{
						continue;
					}

					SiegeNodeHandle nhandle( gSiegeEngine.NodeCache().UseObject( (*d)->GetNeighbour() ) );
					SiegeNode& nnode = nhandle.RequestObject( gSiegeEngine.NodeCache() );

					if( nnode.GetAPVisited() != visitcount )
					{
						nnode.SetAPVisited( visitcount );
						nList.push_back( nhandle );
					}
				}
			}
		}
	}
}

// See if the area between two points is occluded by some geometry
bool SiegeNode::RayIsBlocked( const vector_3& position, const vector_3& point, const SiegeNodeList& slist )
{
	vector_3 sposition			= position + ( Normalize( point - position ) * 0.001f );
	float ray_t					= 0.0f;
	vector_3 face_normal;

	// See if I occlude with myself
	if( m_bOccludesLight )
	{
		if( HitTest( sposition, point - sposition, ray_t, face_normal, LF_IS_ANY ) )
		{
			if( ray_t < 1.0f - FLOAT_TOLERANCE )
			{
				return true;
			}
		}
	}

	// Get world space equivalents
	vector_3 WorldSpacePosition		= NodeToWorldSpace( sposition );
	vector_3 WorldSpacePoint		= NodeToWorldSpace( point );

	// Check my possible neighbors
	for( SiegeNodeList::const_iterator i = slist.begin(); i != slist.end(); ++i )
	{
		const SiegeNode& far_node	= (*i).RequestObject( gSiegeEngine.NodeCache() );
		if( far_node.GetOccludesLight() )
		{
			vector_3 NodeSpacePosition	= far_node.WorldToNodeSpace( WorldSpacePosition );
			vector_3 NodeSpacePoint		= far_node.WorldToNodeSpace( WorldSpacePoint );

			if( far_node.HitTest( NodeSpacePosition, NodeSpacePoint - NodeSpacePosition, ray_t, face_normal, LF_IS_ANY ) )
			{
				if( ray_t < 1.0f - FLOAT_TOLERANCE )
				{
					return true;
				}
			}
		}
	}

	return false;
}

