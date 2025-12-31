/*************************************************************************************
**
**									RapiStaticObject
**
**		This is a test class for the purposes of planning out the different elements
**		and implementation methods of a high quality 3D abstraction layer.
**
**		Date:	05/27/99
**		Author: James Loe
**
*************************************************************************************/

#include "precomp_rapi.h"
#include "gpcore.h"

#if !defined(_RAPI_OWNER_)
#include "RapiOwner.h"
#endif

// Constructor
RapiStaticObject::RapiStaticObject(		
							Rapi *pDevice,									// My owner object
							sVertex* pVertices,			int numVertices,	// Vertex info
							unsigned short* pIndices,	int numIndices,		// Index info
							unsigned int* pTexTriCount,						// Num tri's per tex
							D3DTexList &texlist )							// Texture info
						: m_pDevice( pDevice )
						, m_numVertices( numVertices )
						, m_numTriangles( numIndices / 3 )
						, m_texlist( texlist )
						, m_pVertices( 0 )
{
	if( !m_pDevice )
	{
		return;
	}

	// Organize triangles
	OrganizeInformation( pVertices, pIndices, pTexTriCount );
}

RapiStaticObject::RapiStaticObject(		
							Rapi* pDevice,									// My owner object
							sVertex* pVertices,								// Vertex info
							int numVertices, int numTriangles,				// Number info
							TexStageList& stageList )						// Stage info
						: m_pDevice( pDevice )
						, m_numVertices( numVertices )
						, m_numTriangles( numTriangles )
						, m_pVertices( pVertices )
						, m_TexStageList( stageList )
{
	if( !m_pDevice )
	{
		return;
	}

	// Build up a default texture list
	StaticObjectTex tex;
	tex.textureId	= 0;
	tex.alpha		= false;
	m_texlist.resize( m_TexStageList.size(), tex );
}

// Destructor
RapiStaticObject::~RapiStaticObject()
{
	Destroy();
}


// Replace existing texture list with new list
void RapiStaticObject::UpdateTexlist( const D3DTexList &texlist )
{
	gpassert( m_texlist.size() == texlist.size() );
	m_texlist = texlist;
}

// Draw object
int RapiStaticObject::Render( const DWORD* pLightingInfo )
{
	int numTriangles	= 0;

	// Cycle through the stages
	for( TexStageListIter i = m_TexStageList.begin(); i != m_TexStageList.end(); ++i )
	{
		if( !m_texlist[ (*i).tIndex ].alpha )
		{
			// Fill in the lighting
			void* pVertColors	= (void*)&m_pVertices[ (*i).startIndex ].color;

			if( pLightingInfo )
			{
				gpassert( (*i).numLIndices == 0 );

				// Fill in the colors
				FillDWORDColorStrided( pVertColors, sizeof(sVertex), pLightingInfo + (*i).startIndex, (*i).numVerts );
			}
			else
			{
				// Fill in white
				FillDWORDColorStrided( pVertColors, sizeof(sVertex), (*i).numVerts, 0xFFFFFFFF );
			}

			// Set texture
			m_pDevice->SetTexture( 0, m_texlist[ (*i).tIndex ].textureId );

			// Get a pointer to the main vertex buffer
			unsigned int	startIndex;
			if( m_pDevice->CopyBufferedVertices( (*i).numVerts, startIndex, m_pVertices + (*i).startIndex ) )
			{
				// Draw
				m_pDevice->DrawBufferedIndexedPrimitive( D3DPT_TRIANGLELIST, startIndex, (*i).numVerts, (*i).pVIndices, (*i).numVIndices );
			}
		}
	}

	return numTriangles;
}

// Draw object
int RapiStaticObject::RenderAlpha( const DWORD* pLightingInfo, bool bNoSort )
{
	int numTriangles	= 0;

	// Cycle through the stages
	for( TexStageListIter i = m_TexStageList.begin(); i != m_TexStageList.end(); ++i )
	{
		if( m_texlist[ (*i).tIndex ].alpha && (bNoSort == m_texlist[ (*i).tIndex ].noalphasort) )
		{
			// Fill in the lighting
			void* pVertColors	= (void*)&m_pVertices[ (*i).startIndex ].color;

			if( pLightingInfo )
			{
				gpassert( (*i).numLIndices == 0 );

				// Fill in the colors
				FillDWORDColorStrided( pVertColors, sizeof(sVertex), pLightingInfo + (*i).startIndex, (*i).numVerts );
			}
			else
			{
				// Fill in white
				FillDWORDColorStrided( pVertColors, sizeof(sVertex), (*i).numVerts, 0xFFFFFFFF );
			}

			// Set texture
			m_pDevice->SetTexture( 0, m_texlist[ (*i).tIndex ].textureId );

			// Get a pointer to the main vertex buffer
			unsigned int	startIndex;
			if( m_pDevice->CopyBufferedVertices( (*i).numVerts, startIndex, m_pVertices + (*i).startIndex ) )
			{
				// Draw
				m_pDevice->DrawBufferedIndexedPrimitive( D3DPT_TRIANGLELIST, startIndex, (*i).numVerts, (*i).pVIndices, (*i).numVIndices );
			}
		}
	}

	return numTriangles;
}

// Cleanup
void RapiStaticObject::Destroy( )
{
	for( TexStageListIter i = m_TexStageList.begin(); i != m_TexStageList.end(); ++i )
	{
		if( (*i).pLIndices )
		{
			delete[] (*i).pLIndices;
		}

		delete[] (*i).pVIndices;
	}
	m_TexStageList.clear();

	delete[] m_pVertices;
}

// Organize texture/triangle stages for maximum efficiency
void RapiStaticObject::OrganizeInformation( sVertex* pVerts, unsigned short* pIndices, unsigned int* pTexTriCount )
{
	std::list< sBuildTexStage > texStageList;

	// Build the lists
	unsigned int texCount		= 0;
	unsigned int indexOffset	= 0;
	unsigned int totalVertCount	= 0;
	for( D3DTexList::iterator i = m_texlist.begin(); i != m_texlist.end(); ++i, ++texCount )
	{
		// Build the stage
		sBuildTexStage	newStage;

		// Fill in data we can get quickly
		newStage.tIndex		= texCount;

		std::set< unsigned short > UniqueVerts;
		for( unsigned int n = 0; n < (pTexTriCount[ texCount ] * 3); ++n )
		{
			UniqueVerts.insert( pIndices[ indexOffset + n ] );
			newStage.vIndex.push_back( pIndices[ indexOffset + n ] );
		}
		indexOffset	+= n;

		unsigned short nIndex = 0;
		for( std::set< unsigned short >::iterator v = UniqueVerts.begin(); v != UniqueVerts.end(); ++v, ++nIndex )
		{
			newStage.verts.push_back( pVerts[ (*v) ] );
			newStage.lIndex.push_back( (*v) );

			for( std::vector< WORD >::iterator x = newStage.vIndex.begin(); x != newStage.vIndex.end(); ++x )
			{
				if( (*x) == (*v) )
				{
					(*x) = nIndex;
				}
			}
		}

		newStage.numVerts	= newStage.verts.size();
		totalVertCount		+= newStage.numVerts;

		texStageList.push_back( newStage );
	}

	// Build vertices
	m_pVertices	= new sVertex[ totalVertCount ];
	memset( m_pVertices, 0, sizeof( sVertex ) * totalVertCount );

	// Get a pointer to our vertex listing that we can fill up
	sVertex *plVertices = m_pVertices;

	unsigned int currentIndex	= 0;
	std::list< sBuildTexStage >::iterator o;
	for( o = texStageList.begin(); o != texStageList.end(); ++o )
	{
		// Create a new sTexStage for this stage
		sTexStage newTexStage;

		// Set known info
		newTexStage.tIndex		= (*o).tIndex;
		newTexStage.startIndex	= currentIndex;
		newTexStage.numVerts	= (*o).numVerts;

		unsigned int index		= 0;

		// Fill in the indices for the lighting info
		newTexStage.numLIndices	= (*o).lIndex.size();
		newTexStage.pLIndices	= new DWORD[ newTexStage.numLIndices ];
		for( std::vector< DWORD >::iterator d = (*o).lIndex.begin(); d != (*o).lIndex.end(); ++d, ++index )
		{
			newTexStage.pLIndices[ index ]	= (*d);
		}

		// Fill in the vertex indices
		newTexStage.numVIndices	= (*o).vIndex.size();
		newTexStage.pVIndices	= new WORD[ newTexStage.numVIndices ];
		index	= 0;
		for( std::vector< WORD >::iterator w = (*o).vIndex.begin(); w != (*o).vIndex.end(); ++w, ++index )
		{
			newTexStage.pVIndices[ index ]	= (*w);
		}
		
		// Fill in the vertices
		for( std::vector< sVertex >::iterator v = (*o).verts.begin(); v != (*o).verts.end(); ++v, ++plVertices, ++currentIndex )
		{
			(*plVertices)	= (*v);
		}

		// Put the new stage on our main list
		m_TexStageList.push_back( newTexStage );
	}
}
