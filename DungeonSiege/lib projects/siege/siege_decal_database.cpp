/************************************************************************************
**
**								SiegeDecalDatabase
**
**		See .h file for details
**
**
************************************************************************************/

#include "precomp_siege.h"
#include "gpcore.h"
#include "gpprofiler.h"

// Rapi includes
#include "RapiOwner.h"

// Siege includes
#include "siege_engine.h"
#include "siege_decal.h"
#include "siege_decal_database.h"

// Core includes
#include "space_3.h"

using namespace siege;


// Construction
SiegeDecalDatabase::SiegeDecalDatabase()
{

}

// Destruction
SiegeDecalDatabase::~SiegeDecalDatabase()
{
	// Clean up the collection
	for( DecalCollection::iterator i = m_DecalCollection.begin(); i != m_DecalCollection.end(); ++i )
	{
		delete (*i);
	}

	m_DecalCollection.clear();
	m_DecalMap.clear();
	m_DecalGuidMap.clear();
}

// Takes a position and uses the face normal at that position to setup a
// standard default projection.  Uses horizontal and vertical measurements
// (in meters) to setup the initial scaling.
unsigned int SiegeDecalDatabase::CreateDecal( const SiegePos& pos, const vector_3& normal,
											  const float horizMeters, const float vertMeters,
											  const gpstring& decal_tex, bool bMipMapping,
											  bool bLight, bool bPerspCorrect )
{
	// Create the new decal
	SiegeDecal* pDecal	= new SiegeDecal( pos, normal, horizMeters, vertMeters, decal_tex, bMipMapping, bLight, bPerspCorrect );

	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetDecalCritical() );

	// Insert new decal
	unsigned int decal_index = InsertDecal( pDecal );

	// Tell the owner node about this decal
	SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( pos.node );
	SiegeNode& node			= handle.RequestObject( gSiegeEngine.NodeCache() );

	// Put the decal reference into its owner node
	node.m_DecalList.push_back( decal_index );

	return decal_index;
}

// Detailed construction.  Takes a projection origin and direction,
// as well as a horizontal and vertical projection size to setup scale,
// and both the near plane and far plane to build a proper frustum.
unsigned int SiegeDecalDatabase::CreateDecal( const SiegePos& origin, const matrix_3x3& orient,
											  const float horizMeters, const float vertMeters,
											  const float nearPlane, const float farPlane,
											  const gpstring& decal_tex, bool bMipMapping,
											  bool bLight, bool bPerspCorrect )
{
	// Create the new decal
	SiegeDecal* pDecal	= new SiegeDecal( origin, orient, horizMeters, vertMeters, nearPlane, farPlane, decal_tex, bMipMapping, bLight, bPerspCorrect );

	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetDecalCritical() );

	// Insert new decal
	unsigned int decal_index = InsertDecal( pDecal );

	// Tell the owner node about this decal
	SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( origin.node );
	SiegeNode& node			= handle.RequestObject( gSiegeEngine.NodeCache() );

	// Put the decal reference into its owner node
	node.m_DecalList.push_back( decal_index );

	return decal_index;
}

// Load a decal
unsigned int SiegeDecalDatabase::LoadDecal( const char*& pData )
{
	// Create the new decal
	SiegeDecal* pDecal	= new SiegeDecal( pData );

	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetDecalCritical() );

	// Insert new decal
	return InsertDecal( pDecal, false );
}

void SiegeDecalDatabase::UpgradeLoadDecal( unsigned int decal_index )
{
	if( m_DecalCollection.IsValid( decal_index ) )
	{
		m_DecalCollection[ decal_index ]->UpgradeLoad();
	}
}

void SiegeDecalDatabase::DowngradeLoadDecal( unsigned int decal_index )
{
	if( m_DecalCollection.IsValid( decal_index ) )
	{
		m_DecalCollection[ decal_index ]->DowngradeLoad();
	}
}

// Destroy a decal
void SiegeDecalDatabase::DestroyDecal( unsigned int decal_index, bool bRemoveIndex )
{
	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetDecalCritical() );

	// Free this decal from our collection
	if( m_DecalCollection.IsValid( decal_index ) )
	{
		SiegeDecal* pDecal	= m_DecalCollection[ decal_index ];

		if( bRemoveIndex )
		{
			// Tell the owner node about this decal
			SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( pDecal->GetDecalOrigin().node );
			SiegeNode& node			= handle.RequestObject( gSiegeEngine.NodeCache() );

			for( DecalIndexList::iterator d = node.m_DecalList.begin(); d != node.m_DecalList.end(); ++d )
			{
				if( (*d) == decal_index )
				{
					node.m_DecalList.erase( d );
					break;
				}
			}
		}

		// Cleanup our GUID mapping
		m_DecalGuidMap.erase( pDecal->GetGUID() );

		delete pDecal;
		m_DecalCollection.Free( decal_index );

		// Go through the mapping and clean out any references to this decal
		for( DecalMap::iterator i = m_DecalMap.begin(); i != m_DecalMap.end(); )
		{
			for( DecalIndexList::iterator l = (*i).second.begin(); l != (*i).second.end(); ++l )
			{
				if( (*l) == decal_index )
				{
					(*i).second.erase( l );
					break;
				}
			}

			if( (*i).second.empty() )
			{
				i = m_DecalMap.erase( i );
			}
			else
			{
				++i;
			}
		}
	}
}

// Render the decals for this node
unsigned int SiegeDecalDatabase::RenderNodeDecals( const SiegeNode& node )
{
	GPPROFILERSAMPLE( "SiegeDecalDatabase::RenderNodeDecals()", SP_DRAW | SP_DRAW_TERRAIN );

	if( !gSiegeEngine.GetOptions().DrawDecals() )
	{
		return 0;
	}

	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetDecalCritical() );

	// Find this node
	DecalMap::iterator i = m_DecalMap.find( node.GetGUID() );
	if( i != m_DecalMap.end() )
	{
		unsigned int tri_count	= 0;

		// The assumption is made that space is already setup properly for nodal drawing.
		gDefaultRapi.SetObserverZbiasProjectionActive();

		bool bAlphaBlend = gDefaultRapi.EnableAlphaBlending( true );
		gDefaultRapi.SetTextureStageState(	0,
											D3DTOP_MODULATE,
											D3DTOP_MODULATE,
											D3DTADDRESS_CLAMP,
											D3DTADDRESS_CLAMP,
											D3DTEXF_LINEAR,
											D3DTEXF_LINEAR,
											D3DTEXF_LINEAR,
											D3DBLEND_SRCALPHA,
											D3DBLEND_INVSRCALPHA,
											true );

		gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
		if( !gDefaultRapi.IsManualTextureProjection() )
		{
			gDefaultRapi.SetSingleStageState( 0, D3DTSS_TEXCOORDINDEX,			D3DTSS_TCI_CAMERASPACEPOSITION );
			gDefaultRapi.SetSingleStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS,	D3DTTFF_PROJECTED | D3DTTFF_COUNT3 );
		}

		// Draw each decal that exists in this node
		for( DecalIndexList::iterator l = (*i).second.begin(); l != (*i).second.end(); ++l )
		{
			SiegeDecal* pDecal	=  m_DecalCollection[ (*l) ];

			if( node.GetNodeAlpha() < pDecal->GetAlpha() )
			{
				// Save off existing alpha
				BYTE alpha	= pDecal->GetAlpha();
				pDecal->SetAlpha( (BYTE)node.GetNodeAlpha() );

				// Render decal
				tri_count += pDecal->Render( node.GetGUID() );

				// Restore alpha
				pDecal->SetAlpha( alpha );
			}
			else
			{
				// Render decal
				tri_count += pDecal->Render( node.GetGUID() );
			}
		}

		if( !gDefaultRapi.IsManualTextureProjection() )
		{
			gDefaultRapi.SetSingleStageState( 0, D3DTSS_TEXCOORDINDEX,			0 );
			gDefaultRapi.SetSingleStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS,	D3DTTFF_DISABLE );
		}
		gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
		gDefaultRapi.EnableAlphaBlending( bAlphaBlend );

		gDefaultRapi.ResetObserverZbiasProjection();
		return tri_count;
	}

	return 0;
}

// Update decal lighting for this node
void SiegeDecalDatabase::UpdateDecalLighting( const SiegeNode& node )
{
	GPPROFILERSAMPLE( "SiegeDecalDatabase::UpdateDecalLighting()", SP_DRAW | SP_DRAW_TERRAIN );

	if( !gSiegeEngine.GetOptions().DrawDecals() )
	{
		return;
	}

	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetDecalCritical() );

	// Find this node
	DecalMap::iterator i = m_DecalMap.find( node.GetGUID() );
	if( i != m_DecalMap.end() )
	{
		// Draw each decal that exists in this node
		for( DecalIndexList::iterator l = (*i).second.begin(); l != (*i).second.end(); ++l )
		{
			m_DecalCollection[ (*l) ]->UpdateLighting( node.GetGUID(), (DWORD*)node.m_pVertexColors );
		}
	}
}

// Update decal projection
void SiegeDecalDatabase::UpdateDecalProjection( const database_guid nodeGuid )
{
	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetDecalCritical() );

	// Find this node
	DecalMap::iterator i = m_DecalMap.find( nodeGuid );
	if( i != m_DecalMap.end() )
	{
		// Draw each decal that exists in this node
		for( DecalIndexList::iterator l = (*i).second.begin(); l != (*i).second.end(); ++l )
		{
			SiegeDecal* pDecal = m_DecalCollection[ (*l) ];
			if( pDecal->GetDecalOrigin().node == nodeGuid )
			{
				pDecal->UpdateProjection();
			}
		}
	}
}

// Get the pointer to the requested decal
SiegeDecal* SiegeDecalDatabase::GetDecalPointer( unsigned int decal_index )
{
	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetDecalCritical() );

	if( m_DecalCollection.IsValid( decal_index ) )
	{
		return m_DecalCollection[ decal_index ];
	}

	return NULL;
}

// Get the pointer to the requested decal
SiegeDecal* SiegeDecalDatabase::GetDecalPointer( database_guid decal_guid )
{
	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetDecalCritical() );

	DecalGuidMap::iterator i = m_DecalGuidMap.find( decal_guid );
	if( i != m_DecalGuidMap.end() )
	{
		return (*i).second;
	}

	return NULL;
}

// Decal selection by ray test
bool SiegeDecalDatabase::GetHitDecal( const vector_3& ray_orig, const vector_3& ray_dir, unsigned int& hit_decal )
{
	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetDecalCritical() );

	for( DecalCollection::iterator i = m_DecalCollection.begin(); i != m_DecalCollection.end(); ++i )
	{
		// Test each decal for the ray hit
		if( (*i)->HitTest( ray_orig, ray_dir ) )
		{
			hit_decal	= i.GetIndex();
			return true;
		}
	}

	return false;
}

// Generate a random GUID
database_guid SiegeDecalDatabase::GenerateRandomDecalGUID()
{
	// Synchro threads
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetDecalCritical() );

	// Seed the random number generator
	Randomize();

	// Generate the random guid and assure its uniqueness
	database_guid guid;
	do
	{
		guid = database_guid( RandomDword() );
	} while( m_DecalGuidMap.find( guid ) != m_DecalGuidMap.end() );

	// Return GUID
	return guid;
}

// Insert a new decal into the proper collections
unsigned int SiegeDecalDatabase::InsertDecal( SiegeDecal* pDecal, bool bUpdateLighting )
{
	// Add it to our collection
	unsigned int decal_index = m_DecalCollection.Alloc( pDecal );

	// Update our mapping with this decal's info
	for( DecalRenderColl::iterator i = pDecal->m_DecalRenderColl.begin(); i != pDecal->m_DecalRenderColl.end(); ++i )
	{
		DecalMap::iterator d = m_DecalMap.find( (*i).node );
		if( d != m_DecalMap.end() )
		{
			// We already have an entry for this decal
			(*d).second.push_back( decal_index );
		}
		else
		{
			// Create a new index list
			DecalIndexList list;
			list.push_back( decal_index );

			// Insert this node into our mapping
			m_DecalMap.insert( std::pair< database_guid, DecalIndexList >( (*i).node, list ) );
		}

		if( bUpdateLighting )
		{
			// Update the lighting of the decal for this node
			SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( (*i).node );
			SiegeNode& node			= handle.RequestObject( gSiegeEngine.NodeCache() );

			if( node.m_pVertexColors )
			{
				pDecal->UpdateLighting( node.GetGUID(), (DWORD*)node.m_pVertexColors );
			}
		}
	}

	// Add a lookup for this decal in our GUID mapping
	m_DecalGuidMap.insert( std::pair< database_guid, SiegeDecal* >( pDecal->GetGUID(), pDecal ) );

	return decal_index;
}