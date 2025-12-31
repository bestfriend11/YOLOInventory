/*******************************************************************************************
**
**									SiegeDecal
**
**		See .h file for details
**
*******************************************************************************************/

#include "precomp_siege.h"

// Core includes
#include "gpcore.h"
#include "namingkey.h"

// Renderer includes
#include "RapiOwner.h"

// Siege includes
#include "siege_viewfrustum.h"
#include "siege_engine.h"
#include "siege_loader.h"
#include "siege_decal.h"

// Core includes
#include "space_3.h"
#include "triangle_3.h"


using namespace siege;


// Takes a position and uses the face normal at that position to setup a
// standard default projection.  Uses horizontal and vertical measurements
// (in meters) to setup the initial scaling.
SiegeDecal::SiegeDecal( const SiegePos& pos, const vector_3& normal,
						const float horizMeters, const float vertMeters,
						const gpstring& decal_tex, bool bMipMapping,
						bool bLight, bool bPerspCorrect )
{
	// Generate a random GUID for this decal
	m_Guid				= gSiegeDecalDatabase.GenerateRandomDecalGUID();

	// Set the detail to max
	m_LevelOfDetail		= 1.0f;

	// Build the input parameters
	m_Origin			= pos;
	m_Origin.pos		+= normal;
	m_Orientation		= MatrixFromDirection( -normal );

	gpassert( IsOne( normal.Length() ) );
	m_Angle				= atanf( (horizMeters * 0.5f) / 1.1f ) * 2.0f;
	m_Aspect			= horizMeters / vertMeters;

	m_NearPlane			= 0.1f;
	m_FarPlane			= 1.1f;

	gpassert( IsEqual( GetHorizontalMeters(), horizMeters ) );
	gpassert( IsEqual( GetVerticalMeters(), vertMeters ) );

	// Build the decal texture
	m_DecalTexture		= gDefaultRapi.CreateTexture( decal_tex, bMipMapping, 0, TEX_LOCKED );
	m_DecalLoadOrder	= LOADORDERID_INVALID;

	// Defaults
	m_bActive			= true;
	m_Alpha				= 255;
	m_bLight			= bLight;
	m_bPerspCorrect		= bPerspCorrect;

	// Build the decal geometry
	BuildDecal();
}

// Detailed construction.  Takes a projection origin and direction,
// as well as a horizontal and vertical projection size to setup scale,
// and both the near plane and far plane to build a proper frustum.
SiegeDecal::SiegeDecal( const SiegePos& origin, const matrix_3x3& orient,
						const float horizMeters, const float vertMeters,
						const float nearPlane, const float farPlane,
						const gpstring& decal_tex, bool bMipMapping,
						bool bLight, bool bPerspCorrect )
{
	// Generate a random GUID for this decal
	m_Guid				= gSiegeDecalDatabase.GenerateRandomDecalGUID();

	// Set the detail to max
	m_LevelOfDetail		= 1.0f;

	// Save off the input parameters
	m_Origin			= origin;
	m_Orientation		= orient;
	m_Angle				= atanf( (horizMeters * 0.5f) / farPlane ) * 2.0f;
	m_Aspect			= horizMeters / vertMeters;
	m_NearPlane			= nearPlane;
	m_FarPlane			= farPlane;

	gpassert( IsEqual( GetHorizontalMeters(), horizMeters ) );
	gpassert( IsEqual( GetVerticalMeters(), vertMeters ) );

	// Build the decal texture
	m_DecalTexture		= gDefaultRapi.CreateTexture( decal_tex, bMipMapping, 0, TEX_LOCKED );
	m_DecalLoadOrder	= LOADORDERID_INVALID;

	// Defaults
	m_bActive			= true;
	m_Alpha				= 255;
	m_bLight			= bLight;
	m_bPerspCorrect		= bPerspCorrect;

	// Buid the decal geometry
	BuildDecal();
}

// Load construction.  Loads information about this decal from a block
// of data, usually a mapping into a file.
SiegeDecal::SiegeDecal( const char*& pData )
{
	// Pull off the version info
	unsigned int version	= *((unsigned int *)pData);
	pData	+= sizeof( unsigned int );

	gpassert( version == DECAL_VERSION );

	if( version >= 2 )
	{
		// Read in the GUID for this decal
		m_Guid		= *((database_guid*)pData);
		pData		+= sizeof( database_guid );

		// Read in level of detail
		if( version >= 3 )
		{
			m_LevelOfDetail	= *((float *)pData);
			pData	+= sizeof( float );
		}
		else
		{
			m_LevelOfDetail = 1.0f;
		}

		// Read in active state
		m_bActive	= *((bool *)pData);
		pData		+= sizeof( bool );

		// Read in alpha
		m_Alpha		= *((BYTE *)pData);
		pData		+= sizeof( BYTE );
	}
	else
	{
		// Generate a new GUID
		m_Guid		= gSiegeDecalDatabase.GenerateRandomDecalGUID();

		// Defaults
		m_bActive	= true;
		m_Alpha		= 255;
	}

	// Read in the projection information
	memcpy( &m_Origin.pos, pData, sizeof( vector_3 ) );
	pData	+= sizeof( vector_3 );

	memcpy( &m_Origin.node, pData, sizeof( database_guid ) );
	pData	+= sizeof( database_guid );

	memcpy( &m_Orientation, pData, sizeof( matrix_3x3 ) );
	pData	+= sizeof( matrix_3x3 );

	m_Angle		= *((float *)pData);
	pData	+= sizeof( float );

	m_Aspect	= *((float *)pData);
	pData	+= sizeof( float );

	m_NearPlane	= *((float *)pData);
	pData	+= sizeof( float );

	m_FarPlane	= *((float *)pData);
	pData	+= sizeof( float );

	// Read in the texture information
	gpstring decalTex	= pData;
	pData	+= strlen( pData ) + 1;

	if( version >= 2 )
	{
		// Build location
		gpstring strPath;
		if ( !gNamingKey.BuildContentLocation( decalTex.c_str(), strPath ) )
		{
			gperrorf(( "Illegal decal name '%s' - cannot be located through the naming key!\n", decalTex.c_str() ));
		}

		// Add on texture extension
		strPath	+= ".%img%";

		// Create the texture
		m_DecalTexture		= gDefaultRapi.CreateTexture( strPath, true, 0, TEX_LOCKED, false );
	}
	else
	{
		// Create the texture
		m_DecalTexture		= gDefaultRapi.CreateTexture( decalTex, true, 0, TEX_LOCKED );
	}
	m_DecalLoadOrder		= LOADORDERID_INVALID;

	// Read in geometry
	unsigned int numDecalSets	= *((unsigned int *)pData);
	pData	+= sizeof( unsigned int );

	for( unsigned int i = 0; i < numDecalSets; ++i )
	{
		// Read in the node guid
		database_guid node	= *((database_guid *)pData);
		pData	+= sizeof( database_guid );

		RenderInfo info;
		info.node					= node;
		DecalNodeRenderSet& nSet	= info.renderSet;

		nSet.m_numVertices	= *((unsigned int *)pData);
		pData	+= sizeof( unsigned int );

		nSet.m_pVertices			= new sVertex[ nSet.m_numVertices ];
		nSet.m_pPerspCorrect		= new float[ nSet.m_numVertices ];
		nSet.m_pLightInterp			= new LightInterpolant[ nSet.m_numVertices ];
		nSet.m_bRequestLighting		= true;

		for( unsigned int o = 0; o < nSet.m_numVertices; ++o )
		{
			DecalVertex vert		= *((DecalVertex *)pData);
			pData					+= sizeof( DecalVertex );

			// Build our render time vertices
			sVertex& pVert			= nSet.m_pVertices[o];

			pVert.x					= vert.x;
			pVert.y					= vert.y;
			pVert.z					= vert.z;
			pVert.uv.u				= vert.u;
			pVert.uv.v				= vert.v;
			
			nSet.m_pPerspCorrect[o]	= vert.wl;

			LightInterpolant& lInt	= nSet.m_pLightInterp[o];
			lInt.m_InterpAmount1	= min_t( 1.0f, max_t( 0.0f, vert.li.m_InterpAmount1 ) );
			lInt.m_InterpAmount2	= min_t( 1.0f, max_t( 0.0f, vert.li.m_InterpAmount2 ) );
			lInt.m_InterpAmount3	= min_t( 1.0f, max_t( 0.0f, vert.li.m_InterpAmount3 ) );

			lInt.m_LightIndex1		= vert.li.m_LightIndex1;
			lInt.m_LightIndex2		= vert.li.m_LightIndex2;
			lInt.m_LightIndex3		= vert.li.m_LightIndex3;
		}

		m_DecalRenderColl.push_back( info );
	}

	// Defaults
	m_bLight			= true;
	m_bPerspCorrect		= false;
}

// Destruction
SiegeDecal::~SiegeDecal()
{
	// Release texture
	if( m_DecalTexture )
	{
		// Cancel outstanding orders in the manager
		if( (m_DecalLoadOrder != LOADORDERID_INVALID) && LoadMgr::DoesSingletonExist() )
		{
			// Cancel any outstanding orders and unload the texture
			if( !gSiegeLoadMgr.Cancel( m_DecalLoadOrder ) )
			{
				gDefaultRapi.UnloadTexture( m_DecalTexture );
			}
			m_DecalLoadOrder	= LOADORDERID_INVALID;
		}

		gDefaultRapi.DestroyTexture( m_DecalTexture );
	}

	// Destroy the decal
	DestroyDecal();
}

// Submit the proper loads for upgrading this decal to fully loaded
void SiegeDecal::UpgradeLoad()
{
	// Create texture load orders
	m_DecalLoadOrder = gSiegeLoadMgr.LoadById( LOADER_TEXTURE_TYPE, m_DecalTexture, true );
}

// Cancel loads and unload texture to go back into downgraded mode
void SiegeDecal::DowngradeLoad()
{
	// Release texture
	if( m_DecalTexture )
	{
		// Cancel outstanding orders in the manager
		if( (m_DecalLoadOrder != LOADORDERID_INVALID) && LoadMgr::DoesSingletonExist() )
		{
			// Cancel any outstanding orders and unload the texture
			if( !gSiegeLoadMgr.Cancel( m_DecalLoadOrder ) )
			{
				gDefaultRapi.UnloadTexture( m_DecalTexture );
			}
			m_DecalLoadOrder	= LOADORDERID_INVALID;
		}
	}
}

// Draw the decal.  Since a decal exists in only a single node, but may
// cross into several others, this must be called after all solid node
// geometry has been rendered.  Also, the render matrix (with z-bias) is
// expected to be setup for rendering from the owner node with a call to
// BuildCombinedTransformationMatrix().  Only the portions of the decal
// that exist in the passed node will be rendered.
unsigned int SiegeDecal::Render( const database_guid node )
{
	if( !m_bActive || IsPositive( m_LevelOfDetail - gSiegeEngine.GetLevelOfDetail() ) )
	{
		return 0;
	}

	for( DecalRenderColl::iterator i = m_DecalRenderColl.begin(); i != m_DecalRenderColl.end(); ++i )
	{
		if( (*i).node == node )
		{
			DecalNodeRenderSet& nrs	= (*i).renderSet;
			gDefaultRapi.SetTexture( 0, m_DecalTexture );

			// Do proper lighting management
			if( nrs.m_bRequestLighting )
			{
				SiegeNode* pNode	= gSiegeEngine.IsNodeValid( node );
				if( pNode )
				{
					UpdateLighting( node, pNode->GetLightingData() );
				}
			}
			else if( ((ARGB*)&nrs.m_pVertices[0].color)->a != m_Alpha )
			{
				FillDWORDAlphaStrided( (void*)&nrs.m_pVertices[0].color, sizeof( sVertex ), nrs.m_numVertices, m_Alpha );
			}

			// Get a pointer to the main vertex buffer
			unsigned int	startIndex;
			sVertex*		pVerts;
			if( gDefaultRapi.LockBufferedVertices( nrs.m_numVertices, startIndex, pVerts ) )
			{
				// Fill in the main vertex buffer
				memcpy( pVerts, nrs.m_pVertices, nrs.m_numVertices * sizeof( sVertex ) );
				gDefaultRapi.UnlockBufferedVertices();

				if( gDefaultRapi.IsManualTextureProjection() )
				{
					if( m_bPerspCorrect )
					{
						// Disable fogging
						bool bFog	= gDefaultRapi.SetFogActivatedState( false );

						tVertex* ptVerts;

						// Transform the vertices
						unsigned int tIndex = gDefaultRapi.TransformAndLockVertices( startIndex, nrs.m_numVertices, ptVerts );
						if( tIndex != 0xFFFFFFFF )
						{
							for( unsigned int iv = 0; iv < nrs.m_numVertices; ++iv )
							{
								ptVerts[iv].rhw		*= nrs.m_pPerspCorrect[iv];
							}
							gDefaultRapi.UnlockBufferedTVertices();

							gDefaultRapi.DrawBufferedTPrimitive( D3DPT_TRIANGLELIST, tIndex, nrs.m_numVertices );
						}

						// Reset fogging
						gDefaultRapi.SetFogActivatedState( bFog );
					}
					else
					{
						// Draw
						gDefaultRapi.DrawBufferedPrimitive( D3DPT_TRIANGLELIST, startIndex, nrs.m_numVertices );
					}
				}
				else
				{
					gDefaultRapi.SetTextureProjectionMatrix( m_ProjectionMatrix );

					// Draw
					gDefaultRapi.DrawBufferedPrimitive( D3DPT_TRIANGLELIST, startIndex, nrs.m_numVertices );
				}
			}

			// return triangle count
			return nrs.m_numVertices / 3;
		}
	}

	return 0;
}

// Update the lighting for this node on this decal
void SiegeDecal::UpdateLighting( const database_guid node, const DWORD* lightingInfo )
{
	// Look for the node
	for( DecalRenderColl::iterator i = m_DecalRenderColl.begin(); i != m_DecalRenderColl.end(); ++i )
	{
		if( (*i).node == node )
		{
			DecalNodeRenderSet& nrs	= (*i).renderSet;
			if( IsPositive( m_LevelOfDetail - gSiegeEngine.GetLevelOfDetail() ) )
			{
				nrs.m_bRequestLighting = true;
				continue;
			}

			if( m_bLight )
			{
				for( unsigned int iv = 0; iv < nrs.m_numVertices; ++iv )
				{
					LightInterpolant& li		= nrs.m_pLightInterp[ iv ];
					gpassert( IsEqual( (li.m_InterpAmount1 + li.m_InterpAmount2 + li.m_InterpAmount3), 1.0f, 0.01f ) );

					if( IsOne( li.m_InterpAmount1 ) )
					{
						nrs.m_pVertices[iv].color	= SetDWORDAlpha( lightingInfo[ li.m_LightIndex1 ], m_Alpha );
					}
					else if( IsOne( li.m_InterpAmount2 ) )
					{
						nrs.m_pVertices[iv].color	= SetDWORDAlpha( lightingInfo[ li.m_LightIndex2 ], m_Alpha );
					}
					else if( IsOne( li.m_InterpAmount3 ) )
					{
						nrs.m_pVertices[iv].color	= SetDWORDAlpha( lightingInfo[ li.m_LightIndex3 ], m_Alpha );
					}
					else
					{
						DWORD color	= fScaleDWORDColor( lightingInfo[ li.m_LightIndex1 ], li.m_InterpAmount1 );
						fScaleAndAddDWORDColor( color, lightingInfo[ li.m_LightIndex2 ], li.m_InterpAmount2 );
						fScaleAndAddDWORDColor( color, lightingInfo[ li.m_LightIndex3 ], li.m_InterpAmount3 );
						nrs.m_pVertices[iv].color	= SetDWORDAlpha( color, m_Alpha );
					}
				}
			}
			else
			{
				FillDWORDColorStrided( (void*)&nrs.m_pVertices[0].color, sizeof( sVertex ), nrs.m_numVertices, SetDWORDAlpha( 0xFFFFFFFF, m_Alpha ) );
			}

			// Reset lighting request
			nrs.m_bRequestLighting	= false;
			return;
		}
	}
}

// Update the projection matrix for this decal
void SiegeDecal::UpdateProjection()
{
	SiegeNode* pNode	= gSiegeEngine.IsNodeValid( m_Origin.node );
	if( pNode )
	{
		// Build a render set for this node and add it to our collector
		gDefaultRapi.SetLightProjection( pNode->NodeToWorldSpace( m_Origin.pos ),
										 pNode->GetCurrentOrientation() * m_Orientation,
										 m_Angle, m_Aspect, m_NearPlane, m_FarPlane );

		gDefaultRapi.BuildTextureProjectionMatrix( m_ProjectionMatrix, false );
	}
}

// Accessor for the original horizontal meter construction size
float SiegeDecal::GetHorizontalMeters()
{
	return( (tanf( m_Angle * 0.5f ) * m_FarPlane) * 2.0f );
}

// Accessor for the original vertical meter construction size
float SiegeDecal::GetVerticalMeters()
{
	return( GetHorizontalMeters() / m_Aspect );
}

// Hit test with this decal.  Ray is assumed to be in world space.
bool SiegeDecal::HitTest( const vector_3& ray_orig, const vector_3& ray_dir )
{
	for( DecalRenderColl::iterator i = m_DecalRenderColl.begin(); i != m_DecalRenderColl.end(); ++i )
	{
		// Get the node for this portion of the decal
		SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( (*i).node );
		SiegeNode& node			= handle.RequestObject( gSiegeEngine.NodeCache() );

		// Convert the ray into node local space so we can do the hit
		vector_3 node_orig		= node.WorldToNodeSpace( ray_orig );
		vector_3 node_dir		= node.GetTransposeOrientation() * ray_dir;

		// Go through all of the triangles and hit test
		for( unsigned int v = 0; v < (*i).renderSet.m_numVertices; v += 3 )
		{
			if( math::RayIntersectsTriangle( node_orig, node_dir,
											 *((vector_3*)&(*i).renderSet.m_pVertices[v]),
											 *((vector_3*)&(*i).renderSet.m_pVertices[v+1]),
											 *((vector_3*)&(*i).renderSet.m_pVertices[v+2]) ) )
			{
				return true;
			}
		}
	}

	return false;
}

// Destroys the data allocated by this decal
void SiegeDecal::DestroyDecal()
{
	// Clean up render map
	for( DecalRenderColl::iterator i = m_DecalRenderColl.begin(); i != m_DecalRenderColl.end(); ++i )
	{
		delete[] (*i).renderSet.m_pVertices;
		delete[] (*i).renderSet.m_pPerspCorrect;
		delete[] (*i).renderSet.m_pLightInterp;
	}
	m_DecalRenderColl.clear();
}

// Uses the current decal parameters to build the render information
void SiegeDecal::BuildDecal()
{
	// Destroy existing information about this decal
	DestroyDecal();

	// Build information needed to construct this decal
	SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( m_Origin.node );
	SiegeNode& node			= handle.RequestObject( gSiegeEngine.NodeCache() );

	GeometryCollector		collector;
	vector_3				FrustumPoint[8];
	collector.m_frustPos	= node.NodeToWorldSpace( m_Origin.pos );
	collector.m_frustOrient	= node.GetCurrentOrientation() * m_Orientation;

	float Zdist				= m_NearPlane;
	float Xdist				= Zdist * tanf( m_Angle * 0.5f );
	float Ydist				= Xdist  / m_Aspect;

	FrustumPoint[0]			= collector.m_frustPos + (collector.m_frustOrient * vector_3(-Xdist, Ydist,Zdist));
	FrustumPoint[1]			= collector.m_frustPos + (collector.m_frustOrient * vector_3( Xdist, Ydist,Zdist));
	FrustumPoint[2]			= collector.m_frustPos + (collector.m_frustOrient * vector_3( Xdist,-Ydist,Zdist));
	FrustumPoint[3]			= collector.m_frustPos + (collector.m_frustOrient * vector_3(-Xdist,-Ydist,Zdist));

	float fn				= m_FarPlane / m_NearPlane;
	Xdist					= Xdist * fn;
	Ydist					= Ydist * fn;
	Zdist					= Zdist * fn;

	FrustumPoint[4]			= collector.m_frustPos + (collector.m_frustOrient * vector_3(-Xdist, Ydist,Zdist));
	FrustumPoint[5]			= collector.m_frustPos + (collector.m_frustOrient * vector_3( Xdist, Ydist,Zdist));
	FrustumPoint[6]			= collector.m_frustPos + (collector.m_frustOrient * vector_3( Xdist,-Ydist,Zdist));
	FrustumPoint[7]			= collector.m_frustPos + (collector.m_frustOrient * vector_3(-Xdist,-Ydist,Zdist));

	collector.m_wminBox = vector_3( FLOAT_MAX );
	collector.m_wmaxBox = vector_3( FLOAT_MIN );
	for( unsigned int y = 0; y < 8; ++y )
	{
		collector.m_wminBox.x	= min_t( collector.m_wminBox.x, FrustumPoint[y].x );
		collector.m_wminBox.y	= min_t( collector.m_wminBox.y, FrustumPoint[y].y );
		collector.m_wminBox.z	= min_t( collector.m_wminBox.z, FrustumPoint[y].z );

		collector.m_wmaxBox.x	= max_t( collector.m_wmaxBox.x, FrustumPoint[y].x );
		collector.m_wmaxBox.y	= max_t( collector.m_wmaxBox.y, FrustumPoint[y].y );
		collector.m_wmaxBox.z	= max_t( collector.m_wmaxBox.z, FrustumPoint[y].z );
	}

	// Calculate the plane equation for the near plane
	collector.m_nearPlaneNormal		= collector.m_frustOrient.GetColumn2_T();
	collector.m_nearPlanePoint		= FrustumPoint[0];

	collector.m_farPlaneNormal		= -collector.m_frustOrient.GetColumn2_T();
	collector.m_farPlanePoint		= FrustumPoint[4];

	CalcFaceNormal( collector.m_leftPlaneNormal, FrustumPoint[3], FrustumPoint[0], FrustumPoint[7] );
	collector.m_leftPlanePoint		= FrustumPoint[0];

	CalcFaceNormal( collector.m_rightPlaneNormal, FrustumPoint[1], FrustumPoint[2], FrustumPoint[5] );
	collector.m_rightPlanePoint		= FrustumPoint[1];

	CalcFaceNormal( collector.m_topPlaneNormal, FrustumPoint[0], FrustumPoint[1], FrustumPoint[4] );
	collector.m_topPlanePoint		= FrustumPoint[0];

	CalcFaceNormal( collector.m_bottomPlaneNormal, FrustumPoint[2], FrustumPoint[3], FrustumPoint[6] );
	collector.m_bottomPlanePoint	= FrustumPoint[3];

	gSiegeEngine.NodeTraversalCallback( node.GetGUID(),
										makeFunctor( *this, &SiegeDecal::GeometryCallBack ),
										3, &collector );
}

// Callback function for the collection of geometry used by this decal
bool SiegeDecal::GeometryCallBack( const SiegeNode& node, void* pData )
{
	if( !node.IsOwnedByAnyFrustum() )
	{
		return false;
	}

	// Get the triangle collection search structure
	GeometryCollector* pCollector	= (GeometryCollector*)pData;
	TriangleColl newTriColl;

	if( !BoxIntersectsBox( pCollector->m_wminBox, pCollector->m_wmaxBox,
						   node.GetWorldSpaceMinimumBounds(), node.GetWorldSpaceMaximumBounds() ) )
	{
		return false;
	}

	// Make the box nodespace for collision
	vector_3 minBox( DoNotInitialize );
	vector_3 maxBox( DoNotInitialize );
	node.WorldToNodeBox( pCollector->m_wminBox, pCollector->m_wmaxBox, minBox, maxBox );

	// Get the new triangle collection
	node.GetBoundedTriCollection( minBox, maxBox, newTriColl );
	if( newTriColl.empty() )
	{
		return false;
	}

	// Build a list of render index light interpolants as a precursor to clipping
	std::vector< LightInterpolant > newLiColl;
	BuildRenderInterpolants( node, newTriColl, newLiColl );

	plane_3 plane;
	vector_3 nodeSpaceNormal( DoNotInitialize );

	// Build a nodespace near clipping plane
	nodeSpaceNormal	= node.GetTransposeOrientation() * pCollector->m_nearPlaneNormal;
	plane.SetNormal( nodeSpaceNormal );
	plane.SetD( -nodeSpaceNormal.DotProduct( node.WorldToNodeSpace( pCollector->m_nearPlanePoint ) ) );

	// Clip our list of triangles with the near plane to eliminate negative Z artifacting
	ClipTrianglesWithInterpolation( plane, newTriColl, newLiColl );

	// Build a nodespace far clipping plane
	nodeSpaceNormal	= node.GetTransposeOrientation() * pCollector->m_farPlaneNormal;
	plane.SetNormal( nodeSpaceNormal );
	plane.SetD( -nodeSpaceNormal.DotProduct( node.WorldToNodeSpace( pCollector->m_farPlanePoint ) ) );

	// Clip our list of triangles with the far plane
	ClipTrianglesWithInterpolation( plane, newTriColl, newLiColl );

	// Build a nodespace left clipping plane
	nodeSpaceNormal	= node.GetTransposeOrientation() * pCollector->m_leftPlaneNormal;
	plane.SetNormal( nodeSpaceNormal );
	plane.SetD( -nodeSpaceNormal.DotProduct( node.WorldToNodeSpace( pCollector->m_leftPlanePoint ) ) );

	// Clip our list of triangles with the left plane
	ClipTrianglesWithInterpolation( plane, newTriColl, newLiColl );

	// Build a nodespace right clipping plane
	nodeSpaceNormal	= node.GetTransposeOrientation() * pCollector->m_rightPlaneNormal;
	plane.SetNormal( nodeSpaceNormal );
	plane.SetD( -nodeSpaceNormal.DotProduct( node.WorldToNodeSpace( pCollector->m_rightPlanePoint ) ) );

	// Clip our list of triangles with the right plane
	ClipTrianglesWithInterpolation( plane, newTriColl, newLiColl );

	// Build a nodespace top clipping plane
	nodeSpaceNormal	= node.GetTransposeOrientation() * pCollector->m_topPlaneNormal;
	plane.SetNormal( nodeSpaceNormal );
	plane.SetD( -nodeSpaceNormal.DotProduct( node.WorldToNodeSpace( pCollector->m_topPlanePoint ) ) );

	// Clip our list of triangles with the top plane
	ClipTrianglesWithInterpolation( plane, newTriColl, newLiColl );

	// Build a nodespace bottom clipping plane
	nodeSpaceNormal	= node.GetTransposeOrientation() * pCollector->m_bottomPlaneNormal;
	plane.SetNormal( nodeSpaceNormal );
	plane.SetD( -nodeSpaceNormal.DotProduct( node.WorldToNodeSpace( pCollector->m_bottomPlanePoint ) ) );

	// Clip our list of triangles with the bottom plane
	ClipTrianglesWithInterpolation( plane, newTriColl, newLiColl );

	if( newTriColl.empty() )
	{
		// return true to get my neighbors, but don't create a render set for me
		return true;
	}

	// Build a render set for this node and add it to our collector
	gDefaultRapi.SetLightProjection( node.WorldToNodeSpace( pCollector->m_frustPos ),
									 node.GetTransposeOrientation() * pCollector->m_frustOrient,
									 m_Angle, m_Aspect, m_NearPlane, m_FarPlane );

	D3DMATRIX mat;
	gDefaultRapi.BuildTextureProjectionMatrix( mat, false );

	// mat now contains our proper projection matrix, so we can proceed with the projection
	RenderInfo info;
	info.node				= node.GetGUID();
	DecalNodeRenderSet& nrs	= info.renderSet;
	nrs.m_numVertices		= newTriColl.size();
	nrs.m_pVertices			= new sVertex[ nrs.m_numVertices ];
	nrs.m_pPerspCorrect		= new float[ nrs.m_numVertices ];
	nrs.m_pLightInterp		= new LightInterpolant[ nrs.m_numVertices ];
	nrs.m_bRequestLighting	= true;

	unsigned int iv	= 0;
	for( TriangleColl::iterator v = newTriColl.begin(); v != newTriColl.end(); ++v, ++iv )
	{
		*((vector_3*)&nrs.m_pVertices[iv])	= (*v);
		vector_3 vSrc( (*v) );

		// Calculate w and store it in our z component for future use
		nrs.m_pPerspCorrect[iv]		= (vSrc.x*mat._14) + (vSrc.y*mat._24) + (vSrc.z*mat._34) + mat._44;
		float invw					= 1.0f / nrs.m_pPerspCorrect[iv];

		// Calculate the 2D texture coords
		nrs.m_pVertices[iv].uv.u	= ((vSrc.x*mat._11) + (vSrc.y*mat._21) + (vSrc.z*mat._31) + mat._41) * invw;
		nrs.m_pVertices[iv].uv.v	= ((vSrc.x*mat._12) + (vSrc.y*mat._22) + (vSrc.z*mat._32) + mat._42) * invw;
		nrs.m_pVertices[iv].color	= 0xFFFFFFFF;
	}

	gpassert( newLiColl.size() == nrs.m_numVertices );
	iv	= 0;
	for( std::vector< LightInterpolant >::iterator l = newLiColl.begin(); l != newLiColl.end(); ++l, ++iv )
	{
		nrs.m_pLightInterp[iv].m_LightIndex1	= (*l).m_LightIndex1;
		nrs.m_pLightInterp[iv].m_LightIndex2	= (*l).m_LightIndex2;
		nrs.m_pLightInterp[iv].m_LightIndex3	= (*l).m_LightIndex3;

		nrs.m_pLightInterp[iv].m_InterpAmount1	= min_t( 1.0f, max_t( 0.0f, (*l).m_InterpAmount1 ) );
		nrs.m_pLightInterp[iv].m_InterpAmount2	= min_t( 1.0f, max_t( 0.0f, (*l).m_InterpAmount2 ) );
		nrs.m_pLightInterp[iv].m_InterpAmount3	= min_t( 1.0f, max_t( 0.0f, (*l).m_InterpAmount3 ) );
	}

	m_DecalRenderColl.push_back( info );
	return true;
}

// Build a list of render interpolants from basic geometry info
void SiegeDecal::BuildRenderInterpolants( const SiegeNode& node, TriangleColl& triList,
										  std::vector< LightInterpolant >& liList )
{
	// Get the mesh
	SiegeMesh& mesh			= node.GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );
	TexStageList& sList		= mesh.GetRenderObject()->GetStageList();
	sVertex* pVerts			= mesh.GetVertices();
	liList.resize( triList.size() );

	for( TexStageList::iterator i = sList.begin(); i != sList.end(); ++i )
	{
		for( unsigned int t = 0; t < triList.size(); t += 3 )
		{
			for( unsigned int v = 0; v < (*i).numVIndices; v += 3 )
			{
				unsigned int index1	= (*i).pVIndices[v]   + (*i).startIndex;
				unsigned int index2	= (*i).pVIndices[v+1] + (*i).startIndex;
				unsigned int index3	= (*i).pVIndices[v+2] + (*i).startIndex;

				unsigned int retIndex1, retIndex2, retIndex3;
				if( gSiegeEngine.GetTriangleIndexMatch( triList[t],   retIndex1, pVerts, index1, index2, index3 ) &&
					gSiegeEngine.GetTriangleIndexMatch( triList[t+1], retIndex2, pVerts, index1, index2, index3 ) &&
					gSiegeEngine.GetTriangleIndexMatch( triList[t+2], retIndex3, pVerts, index1, index2, index3 ) )
				{
					// After much pain and suffering, we have found a match
					LightInterpolant li;

					li.m_LightIndex1	= retIndex1 == index1 ? index1 : (retIndex1 == index2 ? index2 : (retIndex1 == index3 ? index3 : 0 ));
					li.m_LightIndex2	= retIndex2 == index1 ? index1 : (retIndex2 == index2 ? index2 : (retIndex2 == index3 ? index3 : 0 ));
					li.m_LightIndex3	= retIndex3 == index1 ? index1 : (retIndex3 == index2 ? index2 : (retIndex3 == index3 ? index3 : 0 ));

					li.m_InterpAmount1	= 1.0f;
					li.m_InterpAmount2	= 0.0f;
					li.m_InterpAmount3	= 0.0f;
					liList[t]			= li;

					li.m_InterpAmount1	= 0.0f;
					li.m_InterpAmount2	= 1.0f;
					li.m_InterpAmount3	= 0.0f;
					liList[t+1]			= li;

					li.m_InterpAmount1	= 0.0f;
					li.m_InterpAmount2	= 0.0f;
					li.m_InterpAmount3	= 1.0f;
					liList[t+2]			= li;

					break;
				}
			}
		}
	}

	gpassert( liList.size() == triList.size() );
}

// Clip a list of vectors to a plane, maintaining a proper index list with interpolants
void SiegeDecal::ClipTrianglesWithInterpolation( const plane_3& plane, TriangleColl& vectList,
												 std::vector< LightInterpolant >& liList )
{
	TriangleColl					newVectList;
	std::vector< LightInterpolant > newLiList;

	unsigned int numVects	= vectList.size();
	newVectList.reserve( numVects<<1 );
	newLiList.reserve( numVects<<1 );

	for( unsigned int i = 0; i < numVects; i += 3 )
	{
		const vector_3& vector0			= vectList[i];
		const vector_3& vector1			= vectList[i+1];
		const vector_3& vector2			= vectList[i+2];

		const LightInterpolant& light0	= liList[i];
		const LightInterpolant& light1	= liList[i+1];
		const LightInterpolant& light2	= liList[i+2];

		const float eval0				= plane.Evaluate( vector0 );
		const float eval1				= plane.Evaluate( vector1 );
		const float eval2				= plane.Evaluate( vector2 );

		bool bOutside0					= eval0 < FLOAT_TOLERANCE;
		bool bOutside1					= eval1 < FLOAT_TOLERANCE;
		bool bOutside2					= eval2 < FLOAT_TOLERANCE;

		// Check to see if entire triangle is outside
		if( bOutside0 && bOutside1 && bOutside2 )
		{
			continue;
		}

		// Check to see if entire triangle is inside
		if( !bOutside0 && !bOutside1 && !bOutside2 )
		{
			newVectList.push_back( vector0 );
			newVectList.push_back( vector1 );
			newVectList.push_back( vector2 );

			newLiList.push_back( light0 );
			newLiList.push_back( light1 );
			newLiList.push_back( light2 );
			continue;
		}

		// Clip the line segments as needed
		vector_3 intersection( DoNotInitialize );
		vector_3 oldintersection( DoNotInitialize );
		LightInterpolant oldinterpolant;
		if( bOutside0 != bOutside1 )
		{
			// Clip line segment 01
			if( ClipLineSegment( plane.GetNormal(), vector0, eval0, vector1, intersection ) )
			{
				LightInterpolant interpolant	= BuildLightInterpolant( vector0, light0, vector1, light1, intersection );
				if( !bOutside0 )
				{
					newVectList.push_back( vector0 );
					newVectList.push_back( intersection );

					newLiList.push_back( light0 );
					newLiList.push_back( interpolant );
				}
				else if( !bOutside1 )
				{
					newVectList.push_back( intersection );
					newVectList.push_back( vector1 );

					newLiList.push_back( interpolant );
					newLiList.push_back( light1 );
				}
				oldinterpolant	= interpolant;
				oldintersection = intersection;
			}
		}
		if( bOutside1 != bOutside2 )
		{
			// Clip line segment 12
			if( ClipLineSegment( plane.GetNormal(), vector1, eval1, vector2, intersection ) )
			{
				LightInterpolant interpolant	= BuildLightInterpolant( vector1, light1, vector2, light2, intersection );
				if( !bOutside1 )
				{
					if( bOutside0 )
					{
						newVectList.push_back( intersection );
						newLiList.push_back( interpolant );
					}
					else
					{
						newVectList.push_back( vector1 );
						newVectList.push_back( intersection );

						newLiList.push_back( light1 );
						newLiList.push_back( interpolant );
					}
				}
				else if( !bOutside2 )
				{
					if( !bOutside0 )
					{
						newVectList.push_back( vector2 );
						newVectList.push_back( vector2 );
						newVectList.push_back( oldintersection );
						newVectList.push_back( intersection );

						newLiList.push_back( light2 );
						newLiList.push_back( light2 );
						newLiList.push_back( oldinterpolant );
						newLiList.push_back( interpolant );
					}
					else
					{
						newVectList.push_back( intersection );
						newVectList.push_back( vector2 );

						newLiList.push_back( interpolant );
						newLiList.push_back( light2 );
					}
				}
				oldinterpolant	= interpolant;
				oldintersection = intersection;
			}
		}
		if( bOutside2 != bOutside0 )
		{
			// Clip line segment 20
			if( ClipLineSegment( plane.GetNormal(), vector2, eval2, vector0, intersection ) )
			{
				LightInterpolant interpolant	= BuildLightInterpolant( vector2, light2, vector0, light0, intersection );
				if( !bOutside2 )
				{
					if( !bOutside1 )
					{
						newVectList.push_back( vector2 );
						newVectList.push_back( vector2 );
						newVectList.push_back( intersection );
						newVectList.push_back( oldintersection );

						newLiList.push_back( light2 );
						newLiList.push_back( light2 );
						newLiList.push_back( interpolant );
						newLiList.push_back( oldinterpolant );
					}
					else
					{
						newVectList.push_back( intersection );
						newLiList.push_back( interpolant );
					}
				}
				else if( !bOutside0 )
				{
					if( !bOutside1 )
					{
						newVectList.push_back( vector0 );
						newVectList.push_back( vector0 );
						newVectList.push_back( oldintersection );
						newVectList.push_back( intersection );

						newLiList.push_back( light0 );
						newLiList.push_back( light0 );
						newLiList.push_back( oldinterpolant );
						newLiList.push_back( interpolant );
					}
					else
					{
						newVectList.push_back( intersection );
						newLiList.push_back( interpolant );
					}
				}
			}
		}
	}

	// Assign the new list
	vectList	= newVectList;
	liList		= newLiList;
}

// Build a proper light interpolant from the given information
LightInterpolant SiegeDecal::BuildLightInterpolant( const vector_3& vector1, const LightInterpolant& light1,
													const vector_3& vector2, const LightInterpolant& light2,
													const vector_3& intersection )
{
	// Build a new light interpolant based off of an intersection of two previous interpolants
	LightInterpolant newLi;
	gpassert( light1.m_LightIndex1 == light2.m_LightIndex1 );
	gpassert( light1.m_LightIndex2 == light2.m_LightIndex2 );
	gpassert( light1.m_LightIndex3 == light2.m_LightIndex3 );

	newLi.m_LightIndex1		= light1.m_LightIndex1;
	newLi.m_LightIndex2		= light1.m_LightIndex2;
	newLi.m_LightIndex3		= light1.m_LightIndex3;

	float totalPercentage	= Length( vector1 - intersection ) / Length( vector1 - vector2 );

	newLi.m_InterpAmount1	= (totalPercentage * light2.m_InterpAmount1) + ((1.0f - totalPercentage) * light1.m_InterpAmount1);
	newLi.m_InterpAmount2	= (totalPercentage * light2.m_InterpAmount2) + ((1.0f - totalPercentage) * light1.m_InterpAmount2);
	newLi.m_InterpAmount3	= (totalPercentage * light2.m_InterpAmount3) + ((1.0f - totalPercentage) * light1.m_InterpAmount3);

	// return our interpolant
	return newLi;
}