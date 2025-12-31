#pragma once
#ifndef _RAPI_OBJECT_
#define _RAPI_OBJECT_
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



// Texture stage description structure used for building the static object
struct sBuildTexStage
{
	unsigned int tIndex;				// Which texture is active in this stage
	std::vector< sVertex > verts;		// List of vertices for this stage
	std::vector< WORD > vIndex;			// Vertex indices for this stage
	std::vector< DWORD > lIndex;		// Lighting indices back into the main vertex array
	unsigned int numVerts;				// How many verts in list (size() is CPU intensive)
};

typedef		std::vector< sTexStage >				TexStageList;
typedef		TexStageList::iterator					TexStageListIter;
typedef		stdx::fast_vector< StaticObjectTex >	D3DTexList;

/*
**	Defines the RapiStaticObject class.
**	
**	RapiStaticObject is a top level class responsible for managing all 3D
**  information for a given static object.  This is provided as a nice way to manage
**  vertex info and related stuff for an object in the world.  Note that it
**  is not wise to keep these objects around if they are not in use by the
**  game, as their vertex information could still be resident on the video card.
*/
class RapiStaticObject
{

public:

	// Constructor and destructor are public.  This class will mostly be used by the 
	// client through handles passed to them from the owner object.  Note that the
	// texIDlist is a list of ID's of textures that have already been created.  
	RapiStaticObject(		Rapi *pDevice,									// My owner object
							sVertex* pVertices,			int numVertices,	// Vertex info
							unsigned short* pIndices,	int numIndices,		// Index info
							unsigned int* pTexTriCount,						// Num tri's per tex
							D3DTexList &texlist );							// Texture info

	RapiStaticObject(		Rapi* pDevice,									// My owner object
							sVertex* pVertices,								// Vertex info
							int numVertices, int numTriangles,				// Number info
							TexStageList& stageList );						// Stage info

	~RapiStaticObject();

	// Lets the user change the renderer that this object uses.
	// This can be a dangerous function, so be very careful with it.
	void					SetRenderer( Rapi& renderer )	{ m_pDevice = &renderer; }

	// Returns the number of vertices in this static object
	const	int				GetNumVertices()		{ return m_numVertices; }

	// Returns the number of triangles in this static object
	const	int				GetNumTriangles()		{ return m_numTriangles; }

	// Returns a const pointer to the texture list.  Yucky, but it might be useful.
	const	D3DTexList&		GetTextureList()		{ return m_texlist; }

	// Returns the stage lists
	TexStageList&			GetStageList()			{ return m_TexStageList; }

	// Returns the vertices
	sVertex*				GetVertices()			{ return m_pVertices; }

	// Allows the client to supply a new texture list for use with this object.  I'm adding
	// this with the thought that we might need it in the future.
	void	UpdateTexlist( const D3DTexList &texlist );

	// The function that does all the fun stuff.  It will render it's geometry in an optimal
	// way, using the requested device's world, projection, and view matrices.
	// RETURNS:  The number of triangles that were rendered.
	int		Render( const DWORD* pLightingInfo = NULL );
	int		RenderAlpha( const DWORD* pLightingInfo = NULL, bool bNoSort = false );

protected:

	// Cleanup
	void	Destroy( );

	// Organize the texture/triangle stages for maximum efficiency
	void	OrganizeInformation( sVertex* pVerts, unsigned short* pIndices, unsigned int* pTexTriCount );

private:

	// Owner object
	Rapi*			m_pDevice;

	// Vertex information
	int				m_numVertices;

	// Triangle information
	int				m_numTriangles;

	// Organized tex/tri list.
	TexStageList	m_TexStageList;

	// Texture information
	D3DTexList		m_texlist;

	// Vertex buffer
	sVertex*		m_pVertices;
};

#endif