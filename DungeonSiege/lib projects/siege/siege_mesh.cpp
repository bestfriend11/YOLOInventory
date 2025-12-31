#include "precomp_siege.h"

// Std includes
#include <vector>

// System includes
#include "gpprofiler.h"
#include "StdHelp.h"

// Math includes
#include "triangle_3.h"
#include "quat.h"

// Engine includes
#include "siege_cache.h"
#include "siege_cache_handle.h"
#include "siege_database_guid.h"
#include "siege_database.h"

#include "siege_mesh.h"
#include "siege_mesh_door.h"

#include "siege_logical_mesh.h"

#include "siege_engine.h"
#include "siege_bsp.h"

// Renderer includes
#include "RapiOwner.h"
#include "RapiPrimitive.h"


using namespace siege;


SiegeMesh::SiegeMesh(void)
	: m_minBBox( FLOAT_MAX )
	, m_maxBBox( FLOAT_MAX )
	, m_centroidOffset( vector_3::ZERO )
	, m_bTile( false )
	, m_pNormals( 0 )
	, m_numDoors( 0 )
	, m_numStages( 0 )
	, m_pRenderObject( 0 )
	, m_numLogicalMeshes( 0 )
	, m_pLogicalMeshes( 0 )
#if !GP_RETAIL
	, m_pColors( 0 )
	, m_numSpots( 0 )
	, m_checksum( 0 )
#endif
{
}

SiegeMesh::~SiegeMesh(void)
{
	// Cleanup
	delete[]	m_pNormals;
	delete		m_pRenderObject;
	delete[]	m_pLogicalMeshes;

	stdx::for_each( m_DoorList, stdx::delete_by_ptr() );

#if !GP_RETAIL
	delete[]	m_pColors;
	stdx::for_each( m_Spots, stdx::delete_by_ptr() );
#endif
}

int SiegeMesh::Render( int flag, const NodeTexColl& texList, DWORD* litColorArray, const DWORD alpha ) const
{
	GPPROFILERSAMPLE( "SiegeMesh::Render", SP_DRAW | SP_DRAW_TERRAIN );

	// Fill in the global mesh alpha if needed
	if( litColorArray && ((ARGB*)litColorArray)->a != alpha )
	{
		FillDWORDAlpha( litColorArray, m_pRenderObject->GetNumVertices(), (BYTE)alpha );
	}
 
	// Draw the object
	gDefaultRapi.SetTextureStageState(	0,
										D3DTOP_MODULATE,
										alpha < 255 ? D3DTOP_MODULATE : D3DTOP_SELECTARG1,
										m_bTile ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP,
										m_bTile ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DBLEND_SRCALPHA,
										D3DBLEND_INVSRCALPHA,
										false );

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ALPHAREF, 0x00000000 );

	m_pRenderObject->UpdateTexlist( texList );
	m_pRenderObject->Render( litColorArray );

#if !GP_RETAIL

	if( flag & FLAG_DRAW_FLOORS )
	{
		// Draw the floors for Snowcake.
		// Cause he keeps bothering me about it.  And I want him to shut up.
		gDefaultRapi.SetTextureStageState(	0,
											D3DTOP_SELECTARG2,
											D3DTOP_SELECTARG2,
											D3DTADDRESS_WRAP,
											D3DTADDRESS_WRAP,
											D3DTEXF_LINEAR,
											D3DTEXF_LINEAR,
											D3DTEXF_LINEAR,
											D3DBLEND_SRCALPHA,
											D3DBLEND_INVSRCALPHA,
											false );

		sVertex floortris[3];
		memset( floortris, 0, sizeof( sVertex ) * 3 );

		for( unsigned int lm = 0; lm < m_numLogicalMeshes; ++lm )
		{
			if( m_pLogicalMeshes[ lm ].GetFlags() & LF_IS_FLOOR )
			{
				for( int t = 0; t < m_pLogicalMeshes[ lm ].GetBSPTree()->GetNumTriangles(); ++t )
				{
					memcpy( &floortris[0], &m_pLogicalMeshes[ lm ].GetBSPTree()->GetTriangles()[ t ].m_Vertices[0], sizeof( vector_3 ) );
					memcpy( &floortris[1], &m_pLogicalMeshes[ lm ].GetBSPTree()->GetTriangles()[ t ].m_Vertices[1], sizeof( vector_3 ) );
					memcpy( &floortris[2], &m_pLogicalMeshes[ lm ].GetBSPTree()->GetTriangles()[ t ].m_Vertices[2], sizeof( vector_3 ) );
					floortris[0].color = floortris[1].color = floortris[2].color = 0xFFFFFFFF;

					gDefaultRapi.DrawPrimitive( D3DPT_TRIANGLELIST, floortris, 3, SVERTEX, NULL, 0 );
				}
			}
		}
	}

	if( flag & FLAG_DRAW_WATER )
	{
		// Draw the floors for Snowcake.
		// Cause he keeps bothering me about it.  And I want him to shut up.
		gDefaultRapi.SetTextureStageState(	0,
											D3DTOP_SELECTARG2,
											D3DTOP_SELECTARG2,
											D3DTADDRESS_WRAP,
											D3DTADDRESS_WRAP,
											D3DTEXF_LINEAR,
											D3DTEXF_LINEAR,
											D3DTEXF_LINEAR,
											D3DBLEND_SRCALPHA,
											D3DBLEND_INVSRCALPHA,
											false );

		sVertex watertris[3];
		memset( watertris, 0, sizeof( sVertex ) * 3 );

		for( unsigned int lm = 0; lm < m_numLogicalMeshes; ++lm )
		{
			if( m_pLogicalMeshes[ lm ].GetFlags() & LF_IS_WATER )
			{
				for( int t = 0; t < m_pLogicalMeshes[ lm ].GetBSPTree()->GetNumTriangles(); ++t )
				{
					memcpy( &watertris[0], &m_pLogicalMeshes[ lm ].GetBSPTree()->GetTriangles()[ t ].m_Vertices[0], sizeof( vector_3 ) );
					memcpy( &watertris[1], &m_pLogicalMeshes[ lm ].GetBSPTree()->GetTriangles()[ t ].m_Vertices[1], sizeof( vector_3 ) );
					memcpy( &watertris[2], &m_pLogicalMeshes[ lm ].GetBSPTree()->GetTriangles()[ t ].m_Vertices[2], sizeof( vector_3 ) );
					watertris[0].color = watertris[1].color = watertris[2].color = 0xFFFFFFFF;

					gDefaultRapi.DrawPrimitive( D3DPT_TRIANGLELIST, watertris, 3, SVERTEX, NULL, 0 );
				}
			}
		}
	}

	if( flag & FLAG_DRAW_NORMALS )
	{
		gDefaultRapi.SetTextureStageState(	0,
											D3DTOP_DISABLE,
											D3DTOP_SELECTARG2,
											D3DTADDRESS_WRAP,
											D3DTADDRESS_WRAP,
											D3DTEXF_LINEAR,
											D3DTEXF_LINEAR,
											D3DTEXF_LINEAR,
											D3DBLEND_SRCALPHA,
											D3DBLEND_INVSRCALPHA,
											false );

		sVertex norm[2];
		for( int i = 0; i < m_pRenderObject->GetNumVertices(); ++i )
		{
			norm[0] = norm[1]	= m_pRenderObject->GetVertices()[ i ];

			norm[1].x			+= m_pNormals[ i ].x;
			norm[1].y			+= m_pNormals[ i ].y;
			norm[1].z			+= m_pNormals[ i ].z;

			gDefaultRapi.DrawPrimitive( D3DPT_LINELIST, norm, 2, SVERTEX, NULL, 0 );
		}
	}

	if( (flag & FLAG_DRAW_ORIGIN) )
	{
		RP_DrawOrigin( gDefaultRapi, 2.0f );
	}

#endif

	return 0;
}

int SiegeMesh::RenderAlpha( const NodeTexColl& texList, DWORD* litColorArray, bool noalphasort ) const
{
	GPPROFILERSAMPLE( "SiegeMesh::RenderAlpha", SP_DRAW | SP_DRAW_TERRAIN );

	// Draw the object
	gDefaultRapi.SetTextureStageState(	0,
										D3DTOP_MODULATE,
										D3DTOP_SELECTARG1,
										m_bTile ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP,
										m_bTile ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DBLEND_SRCALPHA,
										D3DBLEND_INVSRCALPHA,
										false );

	m_pRenderObject->UpdateTexlist( texList );
	m_pRenderObject->RenderAlpha( litColorArray, noalphasort );
	return 0;
}

bool SiegeMesh::Loader( const char* pData, SiegeMeshHeader const &Header )
{
	GPPROFILERSAMPLE( "SiegeMesh::Loader()", SP_LOAD );

	// Get the data I need
	Rapi& mRenderer		= gSiegeEngine.Renderer();

	m_numDoors			= Header.m_numDoors;
	m_numSpots			= Header.m_numSpots;

	m_numStages			= Header.m_numStages;

	m_minBBox			= Header.m_minBBox;
	m_maxBBox			= Header.m_maxBBox;
	m_centroidOffset	= Header.m_centroidOffset;

	m_bTile				= Header.m_bTile;

	// Read in the doors
	for( unsigned int d = 0; d < m_numDoors; ++d )
	{
		unsigned int door_id = *((UINT32*)pData);
		pData += sizeof( unsigned int );

		int		numVerts;

		vector_3* pcenter = (vector_3*)pData;
		pData += sizeof( vector_3 );

		vector_3* px_axis = (vector_3*)pData;
		pData += sizeof( vector_3 );

		vector_3* py_axis = (vector_3*)pData;
		pData += sizeof( vector_3 );

		vector_3* pz_axis = (vector_3*)pData;
		pData += sizeof( vector_3 );

		numVerts = *((int *)pData);
		pData += sizeof( int );

#if !GP_RETAIL

		std::vector< int > verts;
		for( ; numVerts > 0; --numVerts )
		{
			verts.push_back( *((int *)pData) );
			pData += sizeof( int );
		}

#else
		
		pData += sizeof( int ) * numVerts;

#endif

		matrix_3x3 orient;
		orient.SetColumns( *px_axis, *py_axis, *pz_axis );

#if !GP_RETAIL

		m_DoorList.push_back( new SiegeMeshDoor( door_id, *pcenter, orient, verts ) );

#else
		m_DoorList.push_back( new SiegeMeshDoor( door_id, *pcenter, orient ) );

#endif

	}

#if !GP_RETAIL

	// Read in the spots
	for( unsigned int s = 0; s < m_numSpots; ++s )
	{
		vector_3 x_axis = *(vector_3*)pData;
		pData += sizeof( vector_3 );

		vector_3 y_axis = *(vector_3*)pData;
		pData += sizeof( vector_3 );

		vector_3 z_axis = *(vector_3*)pData;
		pData += sizeof( vector_3 );

		vector_3 spot_center = *(vector_3*)pData;
		pData += sizeof( vector_3 );

		matrix_3x3 spot_orient;
		spot_orient.SetColumns( x_axis, y_axis, z_axis );
	
		gpstring spot_label = pData;
		pData += strlen( pData ) + 1;

		m_Spots.push_back( new Spot( spot_label, spot_orient, spot_center ) );
	}

	// Allocate space for colors
	m_pColors			= new DWORD[ Header.m_numVertices ];

#else

	for( unsigned int s = 0; s < m_numSpots; ++s )
	{
		pData += sizeof( vector_3 ) * 4;
		pData += strlen( pData ) + 1;
	}

#endif

	// Write out our list of vertices
	sVertex* pVertices	= new sVertex[ ((Header.m_numVertices + 7) & 0xFFFFFFF8) + 7 ];
	m_pNormals			= new vector_3[ Header.m_numVertices ];

	for( unsigned int v = 0; v < Header.m_numVertices; ++v )
	{
		storeVertex buildVertex;
		memcpy( &buildVertex, pData, sizeof( storeVertex ) );
		pData	+= sizeof( storeVertex );

		sVertex& nVertex	= pVertices[ v ];		
		nVertex.x			= buildVertex.x;
		nVertex.y			= buildVertex.y;
		nVertex.z			= buildVertex.z;
		nVertex.uv			= buildVertex.uv;

		vector_3& nNormal	= m_pNormals[ v ];
		nNormal.x			= buildVertex.nx;
		nNormal.y			= buildVertex.ny;
		nNormal.z			= buildVertex.nz;

#if !GP_RETAIL
		m_pColors[ v ]		= buildVertex.color;
#endif
	}
	
	// Write out the stages
	TexStageList stageList;
	for( unsigned int t	= 0; t < m_numStages; ++t )
	{
		// Pull the name from the file
		const char * pname = pData;
		int namelen = strlen( pData );
		pData += namelen + 1;

		// Put the string on our list of textures
		m_Textures.push_back( gpstring( pname, namelen ) );

		// Write out remaining stage information
		sTexStage nStage;

		nStage.tIndex			= t;

		nStage.startIndex		= *((unsigned int *)pData);
		pData					+= sizeof( unsigned int );

		nStage.numVerts			= *((unsigned int *)pData);
		pData					+= sizeof( unsigned int );

		nStage.numVIndices		= *((unsigned int *)pData);
		pData					+= sizeof( unsigned int );

		nStage.pVIndices		= new WORD[ nStage.numVIndices ];
		memcpy( nStage.pVIndices, pData, sizeof( WORD ) * nStage.numVIndices );
		pData					+= sizeof( WORD ) * nStage.numVIndices;

		nStage.numLIndices		= 0;
		nStage.pLIndices		= NULL;

		// Put the new stage onto our list
		stageList.push_back( nStage );
	}

	// Build our render object
	m_pRenderObject				= new RapiStaticObject( &mRenderer, pVertices, Header.m_numVertices, Header.m_numTriangles, stageList );

	// Read the number of logical nodes we are about to read from file
	m_numLogicalMeshes			= *((unsigned int *)pData);
	pData						+= sizeof( unsigned int );

	m_pLogicalMeshes			= new SiegeLogicalMesh[ m_numLogicalMeshes ];

	// Write all of the logical nodes out to file
	for( unsigned int l = 0; l < m_numLogicalMeshes; ++l )
	{
		m_pLogicalMeshes[ l ].Load( pData, Header );
		gpassert( m_pLogicalMeshes[ l ].GetID() == l );

		if( Header.m_majorVersion <= 6 && Header.m_minorVersion < 1 )
		{
			if( m_pLogicalMeshes[ l ].m_flags == LF_NONE )
			{
				m_pLogicalMeshes[ l ].m_flags = m_pLogicalMeshes[ l ].m_flags | LF_IS_WALL;
			}
			if( m_pLogicalMeshes[ l ].m_flags == LF_ALL )
			{
				m_pLogicalMeshes[ l ].m_flags = m_pLogicalMeshes[ l ].m_flags | LF_IS_FLOOR;
			}
		}
	}

	return true;
}

SiegeMeshDoor* SiegeMesh::GetDoorByIndex(const unsigned int index) const
{
	unsigned int curr_index = 0;
	for( SiegeMeshDoorConstIter d = m_DoorList.begin(); d != m_DoorList.end(); ++d, ++curr_index )
	{
		gpassert((*d));

		if( index == curr_index )
		{
			return (*d);
		}
	}

	return NULL;
}

SiegeMeshDoor* SiegeMesh::GetDoorByID( const unsigned int id) const
{
	// Go through all the doors and find the one we want
	for( SiegeMeshDoorConstIter d = m_DoorList.begin(); d != m_DoorList.end(); ++d )
	{
		gpassert((*d));

		if( id == (*d)->GetID() )
		{
			return (*d);
		}
	}

	return NULL;
}

#if !GP_RETAIL

const siege::Spot& SiegeMesh::GetSpotByIndex(const unsigned int id) const
{
	gpassertm( id > 0 && id < m_Spots.size(), "Invalid spot ID" );
	return *m_Spots[id];
}

const siege::Spot& SiegeMesh::GetSpotByLabel( const char* label ) const
{
	// Go through all the spots and find the one we want
	for( SpotArrayConstIter s = m_Spots.begin(); s != m_Spots.end(); ++s )
	{
		if( !strcmp( (*s)->m_label.c_str(), label ) )
		{
			return *(*s);
		}
	}

	gpwarningf( ("Spot label [%s] not found.  GetSpotByLabel()", label) );
	return *m_Spots[0];
}

bool SiegeMesh::GetSpotPositionOrientation( const char* spotlabel, vector_3 &position, matrix_3x3 &orientation ) const
{
	for( SpotArrayConstIter s = m_Spots.begin(); s != m_Spots.end(); ++s )
	{
		if( !stricmp( (*s)->m_label.c_str(), spotlabel) )
		{
			position	= (*s)->m_position;
			orientation = (*s)->m_orientation;
			return true;
		}
	}

	gpwarningf( ("Spot label [%s] not found.  GetSpotPositionOrientation()", spotlabel) );
	position	= vector_3::ZERO;
	orientation = matrix_3x3::IDENTITY;
	return false;
}

#endif
