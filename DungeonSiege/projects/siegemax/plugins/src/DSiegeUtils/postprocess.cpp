/*******************************************************************************
**
**							PostProcess
**
*******************************************************************************/

#include <gpcore.h>

#include "postprocess.h"
//#include <io.h>
#include <set>
#include "tristrip\nvtristrip.h"
#include "SNOExp.h"

#include "plane_3.h"
#include "space_3.h"
#include "triangle_3.h"
#include "calcbsp.h"

#define HUMANOID_WALKABLE			1<<0
#define IS_WALL						1<<29
#define IS_FLOOR					1<<30
#define IS_WATER					1<<31
#define SUBDIVSIZE_INMETERS			1.0f
const float	CONNECTION_TOLERANCE	= 0.0001f;

//#define Y_DIMENSION_DIVIDE

// constant vector
const vector_3 mod( 0.01f, 0.01f, 0.01f );

// Macro for the creation of a DWORD color value from a vector of floats
#define MAKEDWORDCOLOR( v ) ( 0xFF000000 | ( FTOL( v.x * 255.0f ) << 16 ) | ( FTOL( v.y * 255.0f ) << 8 ) | FTOL( v.z * 255.0f ) )


//////////////////////////////////////////////////////////////////////////////
// CRC32 checksum

// copied from zlib's crc32.c

static const DWORD s_CRC32Table[ 256 ] =
{
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
	0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
	0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
	0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
	0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
	0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
	0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
	0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
	0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
	0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
	0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
	0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
	0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
	0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
	0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
	0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
	0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
	0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
	0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
	0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
	0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
	0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
	0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
	0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
	0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
	0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
	0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
	0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
	0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
	0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
	0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
	0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
	0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
	0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
	0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
	0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
	0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
	0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
	0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
	0x2d02ef8dL,
};

#define DO1( buf )  seed = s_CRC32Table[ ((int)seed ^ (*buf++)) & 0xff ] ^ (seed >> 8)
#define DO2( buf )  DO1( buf );  DO1( buf )
#define DO4( buf )  DO2( buf );  DO2( buf )
#define DO8( buf )  DO4( buf );  DO4( buf )

DWORD GetCRC32( DWORD seed, const void* buf, UINT sizeInBytes )
{
	// $ wacky DO stuff is to unroll the loop

	if ( buf == NULL )
	{
		return ( 0 );
	}

	const UINT8* buf8 = (const UINT8*)buf;
	seed = seed ^ 0xffffffffL;

	while ( sizeInBytes >= 8 )
	{
		DO8( buf8 );
		sizeInBytes -= 8;
	}
	if ( sizeInBytes ) do
	{
		DO1( buf8 );
	}
	while ( --sizeInBytes );

	return ( seed ^ 0xffffffffL );
}

#undef DO1
#undef DO2
#undef DO4
#undef DO8

// adds more info to CRC
inline void AddCRC32( DWORD& seed, const void* buf, UINT sizeInBytes )
	{  seed = GetCRC32( seed, buf, sizeInBytes );  }

// adds more info to CRC
template <typename T>
inline void AddCRC32( DWORD& seed, const T& obj )
	{  AddCRC32( seed, &obj, sizeof( T ) );  }


// Construction
PostProcess::PostProcess()
{
}

// Destruction
PostProcess::~PostProcess()
{
}

// The only external function
bool PostProcess::OrgAndCalcInfo( FileSys::File& expFile, const std::vector<char>& pData )
{
	// Read in the mesh information
	m_inMesh.Read( &pData[0] );

	// Setup checksum
	DWORD checksum	= 0;

	// Build the outbound mesh from the in mesh
	m_outMesh.Build( m_inMesh );

	// Write out the new mesh
	m_outMesh.Write( m_inMesh, expFile, checksum );

	// Calculate the pathing and BSP information to go along with this mesh
	m_pathAndBSP.Build( m_inMesh );

	// Write out that info along with existing mesh information
	m_pathAndBSP.Write( expFile, checksum );

	// Reset write pointer and fill in proper checksum
	expFile.SeekBegin( sizeof( OutMesh::OutMeshHeader ) );
	expFile.Write(&checksum, sizeof( DWORD ));

/*	bool  Write( LPCVOID buffer, int size, DWORD& written )
				 {  return ( GP_WriteFile( *this, buffer, size, &written ) != FALSE );  }
	bool  Write( LPCVOID buffer, int size )
				 {  DWORD dummy;  return ( Write( buffer, size, dummy ) );  }
*/
//	fwrite( &checksum, sizeof( DWORD ), 1, expFile );

	return true;
}


// Construction
InMesh::InMesh()
{
}

// Destruction
InMesh::~InMesh()
{
}

// Extract in mesh information from an INODE
bool InMesh::Read( const char *pData )
{
	
/*
	// Get the trimesh 
	// Get the SNODATA

	TimeValue t(0);
	Object *o =thisnode->EvalWorldState(t).obj;
	TriObject *triobj = (TriObject *)o->ConvertToType(t, triObjectClassID);

	if (!triobj) 
	{
		return false
	}

	Mesh& trimesh = triobj->GetMesh();
*/

	unsigned int Index	= 0;
	unsigned int Pos	= 0;
	float *pElements	= NULL;

	SiegeNodeIO_header Header;
	memcpy( &Header, pData, sizeof( SiegeNodeIO_header ) );
	pData	+= sizeof( SiegeNodeIO_header );


	// Read in the doors
	for( Index = 0; Index < Header.NumDoors; ++Index)
	{
		PPDoorStorage	nDoor;
		nDoor.door_id = *((UINT32*)pData);
		pData += sizeof( UINT32 );

		nDoor.center = *(vector_3*)pData;
		pData += sizeof( vector_3 );

		nDoor.x_axis = *(vector_3*)pData;
		pData += sizeof( vector_3 );

		nDoor.y_axis = *(vector_3*)pData;
		pData += sizeof( vector_3 );

		nDoor.z_axis = *(vector_3*)pData;
		pData += sizeof( vector_3 );

		std::vector<int> verts;
		std::vector<float> key;

		nDoor.num_verts = *((int *)pData);
		pData += sizeof( int );

		nDoor.pverts	= new int[ nDoor.num_verts ];
		for( unsigned int i = 0; i < nDoor.num_verts; ++i )
		{
			nDoor.pverts[i] = *((int *)pData);
			pData += sizeof( int );
		}

		m_DoorStorage.push_back( nDoor );
	}

	// Does this SNO have any spots?
	if( Header.Version > 5 || ((Header.Version == 5) && (Header.MinorVersion > 4) ))
	{
		for( Index = 0; Index < Header.NumSpots; ++Index)
		{
			PPSpotStorage	nSpot;

			nSpot.x_axis = *(vector_3*)pData;
			pData += sizeof( vector_3 );

			nSpot.y_axis = *(vector_3*)pData;
			pData += sizeof( vector_3 );

			nSpot.z_axis = *(vector_3*)pData;
			pData += sizeof( vector_3 );

			nSpot.center = *(vector_3*)pData;
			pData += sizeof( vector_3 );

			nSpot.name	= new char[ strlen( pData ) + 1 ];
			strcpy( nSpot.name, pData );
			pData += strlen( nSpot.name ) + 1;

			m_SpotStorage.push_back( nSpot );
		}
	}

	// Reset the bounding volume
	m_minBBox = vector_3( RealMaximum, RealMaximum, RealMaximum );
	m_maxBBox = vector_3( RealMinimum, RealMinimum, RealMinimum );

	// Read in the vertices
	vector_3* tempVertices = (vector_3*)pData;
	pData += (sizeof( vector_3 ) * Header.NumVertices);

	// Calculate the bounding volume and centroid
	for( Index = 0; Index < Header.NumVertices; ++Index )
	{
		vector_3& Vertex = tempVertices[ Index ];

		m_CentroidOffset += Vertex;

		m_minBBox.x = min( Vertex.x, m_minBBox.x );
		m_minBBox.y = min( Vertex.y, m_minBBox.y );
		m_minBBox.z = min( Vertex.z, m_minBBox.z );

		m_maxBBox.x = max( Vertex.x, m_maxBBox.x );
		m_maxBBox.y = max( Vertex.y, m_maxBBox.y );
		m_maxBBox.z = max( Vertex.z, m_maxBBox.z );
	}
	
	m_CentroidOffset /= Header.NumVertices;
	
	// Read in the UV coordinates
	UV* tempTVerts = (UV*)pData;
	pData += (Header.NumTextureVerts * sizeof( UV ));

	// See if we need to tile or clamp this node
	for( Index = 0; Index < Header.NumTextureVerts; ++Index )
	{
		UV& uv = tempTVerts[ Index ];

		if( (uv.u > 1.0f) || (uv.u < 0.0f) )
		{
			m_bTile = true;
			break;
		}

		if( (uv.v > 1.0f) || (uv.v < 0.0f) )
		{
			m_bTile = true;
			break;
		}
	}

	// Read in the vertex color information
	vector_3* tempColourVerts = (vector_3*)pData;
	pData += (Header.NumColourVerts * sizeof( vector_3 ));

	vector_3* tempNormals = (vector_3*)pData;
	pData += (Header.NumNormals * sizeof( vector_3 ));
	
	// Actually the array size is the number of wedges, not 3*NumFaces
	m_ArraySize		= Header.NumFaces*3;
	
	// Allocate the memory for the GL arrays
	m_pVertices		= new vector_3[ m_ArraySize ];
	m_pNormals		= new vector_3[ m_ArraySize ];
	m_pColorVerts	= new vector_3[ m_ArraySize ];
	m_pTextureVerts	= new UV[ m_ArraySize ];
	m_pIndices  	= new UINT16[ m_ArraySize ];

	VERTINDEXCOLLECTION* pLookup	= new VERTINDEXCOLLECTION[ m_ArraySize ];

	m_IndexedSize = 0;

	VERTINDEXCOLLECTION* pIndexCollection = (VERTINDEXCOLLECTION*)pData;
	pData += (m_ArraySize * sizeof( VERTINDEXCOLLECTION ));

	for( Index = 0; Index < m_ArraySize; ++Index)
	{
		VERTINDEXCOLLECTION& info	= pIndexCollection[ Index ];

#if !GP_RETAIL

		if( Header.NumTextureVerts > 0 )
		{
			gpassert( info.TextureVertexIndex <= Header.NumTextureVerts );
		}

		if( Header.NumColourVerts <= 0 )
		{
			gpassert( info.ColorIndex <= Header.NumColourVerts );
		}

		if( Header.NumNormals <= 0 )
		{
			gpassert( info.NormalIndex <= Header.NumNormals );
		}

		gpassert( info.VertexIndex <= Header.NumVertices );
		
#endif

		for( Pos = 0; Pos < m_IndexedSize; ++Pos )
		{
			VERTINDEXCOLLECTION& testIndex	= pLookup[ Pos ];

			if( *((LONGLONG*)&testIndex)			== *((LONGLONG*)&info) &&
				*((LONGLONG*)&testIndex.ColorIndex) == *((LONGLONG*)&info.ColorIndex) )
			{
				break;
			}
		}

		if( Pos < m_IndexedSize )
		{
			m_pIndices[ Index ] = Pos;
			continue;
		}

		pLookup[ m_IndexedSize ]			= info;
	
		m_pIndices[ Index ]					= m_IndexedSize;

		m_pVertices[ m_IndexedSize ]		= tempVertices[ info.VertexIndex ];

		if( Header.NumNormals <= 0 )
		{
			m_pNormals[ m_IndexedSize ]		= vector_3( 0.0f, 0.0f, 0.0f );
		}
		else
		{
			m_pNormals[ m_IndexedSize ]		= tempNormals[ info.NormalIndex ]; 
		}

		if( Header.NumColourVerts <= 0 )
		{
			m_pColorVerts[ m_IndexedSize ]	= vector_3( 0.0f, 0.0f, 0.0f );
		}
		else
		{
			m_pColorVerts[ m_IndexedSize ]	= tempColourVerts[ info.ColorIndex ];
		}

		if( Header.NumTextureVerts <= 0 )
		{
			UV uv;
			uv.u = 0.0f;
			uv.v = 0.0f;

			m_pTextureVerts[ m_IndexedSize ]= uv;
		}
		else
		{
			m_pTextureVerts[ m_IndexedSize ]= tempTVerts[ info.TextureVertexIndex ];
		}

		// Increment the indexed size
		m_IndexedSize++;
	}

	for( std::vector< PPDoorStorage >::iterator di = m_DoorStorage.begin(); di != m_DoorStorage.end(); ++di )
	{
		for( unsigned int v = 0; v < (*di).num_verts; ++v )
		{
			for( int m = 0; m < m_IndexedSize; ++m )
			{
				if( pLookup[m].VertexIndex == (*di).pverts[v] )
				{
					(*di).pverts[v]	= m;
					break;
				}
			}
		}
	}

	delete[] pLookup;

	m_ArraySize = m_IndexedSize;

	// Load in the floor faces if there are any
	for( Index = 0; Index < Header.NumFloorFaces; ++Index)
	{
		m_FloorFaces.push_back( *((unsigned short*)pData) );
		pData += sizeof( unsigned short );
	}

	// Load in the water faces if there are any
	for( Index = 0; Index < Header.NumWaterFaces; ++Index)
	{
		m_WaterFaces.push_back( *((unsigned short*)pData) );
		pData += sizeof( unsigned short );
	}

	// ********************************************************
	m_NumberOfTriangles = Header.NumFaces;

	// This read will be bogus after I have converted to version 3
	int mat_count = *((int *)pData);
	pData += sizeof( int );

	bool NEW_VERSION = false;

	if (Header.NumMaterials == mat_count-1)
	{	// Check for an old format
		m_NumberOfTextureMaps	= Header.NumMaterials;
		m_NumberOfMaterials		= mat_count;
	}
	else
	{
		m_NumberOfTextureMaps	= Header.NumTextureMaps;
		m_NumberOfMaterials		= Header.NumMaterials;
		NEW_VERSION = true;
	}

	if( !NEW_VERSION )
	{
		// Can't go on
		gpassert( !"VERY OLD SNO file being used.  BAD!" );
		return false;
	}
	
	m_pFlatShadedTriCount	= new UINT32[ m_NumberOfMaterials-m_NumberOfTextureMaps ];
	memcpy( m_pFlatShadedTriCount, pData, (sizeof( UINT32 ) * (m_NumberOfMaterials-m_NumberOfTextureMaps)) );
	pData += (sizeof( UINT32 ) * (m_NumberOfMaterials-m_NumberOfTextureMaps));

	m_pTexturedTriCount		= new UINT32[ m_NumberOfTextureMaps ];
	memcpy( m_pTexturedTriCount, pData, (sizeof( UINT32 ) * m_NumberOfTextureMaps) );
	pData += (sizeof( UINT32 ) * m_NumberOfTextureMaps);

	m_pMaterialDiffuseColour = new vector_3[ m_NumberOfMaterials ];
	memcpy( m_pMaterialDiffuseColour, pData, (sizeof( vector_3 ) * m_NumberOfMaterials) );
	pData += (sizeof( vector_3 ) * m_NumberOfMaterials);

	gpassert( Header.Version > 3 );
	std::vector< unsigned int > texlist;
	for( int i = 0; i < m_NumberOfTextureMaps; i++ )
	{
		// Pull the name from the file
		const char * pname = pData;
		int namelen = strlen( pData );
		pData += namelen+1;

		// Remove any file extension
		int j = namelen;
		while( j >= 0 )
		{
			if( pname[j] == '.' )
			{
				namelen = j;
				break;
			}
			j--;
		}

		// Put the string on our list of textures
		m_Textures.push_back( gpstring( pname, namelen ) );

		// Push index onto list.  This list is used by the RapiStaticObject for organization
		// purposes, so we just need to put some dummy value in it.
		texlist.push_back( i );
	}


	if ((Header.Version >= 5) || ((Header.Version == 4) && (Header.MinorVersion > 1)))  {

		int marker = *((int *)pData);
		pData += sizeof( int );

		if (marker == 0x4f464e49) {		// try to match 'INFO' in the stream

			int count = *((int *)pData);
			pData += sizeof( int );

			for (int i = 0; i < count; i++ )
			{
				const char *pbuff = pData;
				int bufflen = strlen( pData );
				pData += bufflen+1;

				m_infoStrings.push_back(pbuff);
			}
		}

	} else {

		// This is a really OLD sno file
		gpstring tmp = "\n\nSNOFILE=";// + "NeedFileNameHere" ;
		m_infoStrings.push_back(tmp);
		char buf[64];
		sprintf (buf,"VERSION=%d.%d",Header.Version,Header.MinorVersion);
		m_infoStrings.push_back(buf);
		m_infoStrings.push_back("\n\nWARNING: This is an old SNO file, you should have it re-exported\n\n");

	}

	return true;
}


// Construction
OutMesh::OutMesh()
{
}

// Destruction
OutMesh::~OutMesh()
{
}

// Build from and existing inbound mesh
bool OutMesh::Build( InMesh& inMesh )
{
	storeVertex* pVerts = new storeVertex[ inMesh.m_ArraySize ];
	memset( pVerts, 0, sizeof( storeVertex ) * inMesh.m_ArraySize );

	for( int Index = 0; Index < inMesh.m_ArraySize; ++Index )
	{
		storeVertex *pVertex		= &pVerts[Index];

		*((vector_3*)pVertex)		= inMesh.m_pVertices[ Index ];
		*((vector_3*)&pVertex->nx)	= inMesh.m_pNormals[ Index ];
		pVertex->uv					= inMesh.m_pTextureVerts[ Index ];
		pVertex->color				= MAKEDWORDCOLOR( inMesh.m_pColorVerts[ Index ] );
	}

	m_numTriangles	= inMesh.m_NumberOfTriangles;
	std::list< sBuildTexStage > texStageList;

	// Build the lists
	unsigned int texCount		= 0;
	unsigned int indexOffset	= 0;
	unsigned int totalVertCount	= 0;
	for( texCount = 0; texCount < inMesh.m_NumberOfTextureMaps; ++texCount )
	{
		// Build the stage
		sBuildTexStage	newStage;

		// Fill in data we can get quickly
		newStage.tIndex		= texCount;

		std::set< unsigned short > UniqueVerts;
		unsigned int localVertIndex	= 0;
		for( unsigned int n = 0; n < (inMesh.m_pTexturedTriCount[ texCount ] * 3); ++n )
		{
			UniqueVerts.insert( inMesh.m_pIndices[ indexOffset + n ] );
			newStage.vIndex.push_back( inMesh.m_pIndices[ indexOffset + n ] );
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
	m_numVertices	= totalVertCount;

	m_pVertices		= new storeVertex[ totalVertCount ];
	memset( m_pVertices, 0, sizeof( storeVertex ) * totalVertCount );

	// Get a pointer to our vertex listing that we can fill up
	storeVertex *plVertices = m_pVertices;

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
		for( std::vector< storeVertex >::iterator v = (*o).verts.begin(); v != (*o).verts.end(); ++v, ++plVertices, ++currentIndex )
		{
			(*plVertices)	= (*v);
		}

		// Put the new stage on our main list
		m_TexStageList.push_back( newTexStage );
	}

	// Prepare to stripify the render geometry
	SetCacheSize( 16 );
	SetStitchStrips( false );
	SetMinStripSize( 0 );
	SetListsOnly( true );

	// Make a copy of the vertices for remapping
	storeVertex* tempVertices			= new storeVertex[ m_numVertices ];
	memcpy( tempVertices, m_pVertices, sizeof( storeVertex ) * m_numVertices );

	// Make a collection to hold our indices for post processing on the doors
	std::vector< unsigned short > indexCache;

	for( TexStageList::const_iterator t	= m_TexStageList.begin(); t != m_TexStageList.end(); ++t )
	{
		// Generate strip information for this stage, numGroups will always be 1
		PrimitiveGroup* pGroup			= NULL;
		WORD numGroups					= 0;
		GenerateStrips( (*t).pVIndices, (*t).numVIndices, &pGroup, &numGroups );

		// Remap indices
		PrimitiveGroup* pRemappedGroups	= NULL;
		int* pIndexCache				= NULL;
		RemapIndices( pGroup, numGroups, (*t).numVerts, &pRemappedGroups, pIndexCache );

		// Remap this section of vertices
		for( DWORD vertCount = 0; vertCount < (*t).numVerts; ++vertCount )
		{
			// Copy the vertex into it's new happy home
			m_pVertices[ pIndexCache[ vertCount ] + (*t).startIndex ]	= tempVertices[ vertCount + (*t).startIndex ];

			// Put this index onto the cache;
			indexCache.push_back( pIndexCache[ vertCount ] + (*t).startIndex );
		}

		// Copy the indices into the stage
		memcpy( (*t).pVIndices, pRemappedGroups[0].indices, sizeof( WORD ) * (*t).numVIndices );

		// Cleanup
		delete[] pIndexCache;
		delete[] pGroup;
		delete[] pRemappedGroups;
	}
	delete[] tempVertices;

	// Remap door vert indices
	for( std::vector< InMesh::PPDoorStorage >::const_iterator ds = inMesh.m_DoorStorage.begin(); ds != inMesh.m_DoorStorage.end(); ++ds )
	{
		for( unsigned int yo = 0; yo < (*ds).num_verts; ++yo )
		{
			// Find the new index in the stage for local remapping
			for( t = m_TexStageList.begin(); t != m_TexStageList.end(); ++t )
			{
				bool bFoundVert	= false;
				for( unsigned int whee = 0; whee < (*t).numLIndices; ++whee )
				{
					if( (*ds).pverts[ yo ] == (*t).pLIndices[ whee ] )
					{
						(*ds).pverts[ yo ] = indexCache[ (*t).startIndex + whee ];
						bFoundVert	= true;
						break;
					}
				}

				if( bFoundVert )	{ break; }
			}
		}
	}

	delete[] pVerts;

	return true;
}

// Write out the new mesh
bool OutMesh::Write( InMesh& inMesh, FileSys::File& expFile, DWORD& checksum )
{
	// Create a new header to be written out for this SNO
	OutMeshHeader	nHeader;
	memset( &nHeader, 0, sizeof( OutMeshHeader ) );

	nHeader.m_id				= SNOD_MAGIC;
	nHeader.m_majorVersion		= SNOD_VERSION;
	nHeader.m_minorVersion		= SNOD_MINOR_VERSION;

	nHeader.m_numDoors			= inMesh.m_DoorStorage.size();
	nHeader.m_numSpots			= inMesh.m_SpotStorage.size();

	nHeader.m_numVertices		= m_numVertices;
	nHeader.m_numTriangles		= m_numTriangles;

	nHeader.m_numStages			= m_TexStageList.size();

	nHeader.m_minBBox			= inMesh.m_minBBox;
	nHeader.m_maxBBox			= inMesh.m_maxBBox;
	nHeader.m_centroidOffset	= inMesh.m_CentroidOffset;

	nHeader.m_bTile				= inMesh.m_bTile;

	// Write out our header of goodness
	expFile.Write( &nHeader, sizeof( OutMeshHeader ) );

	// Write checksum
	expFile.Write( &checksum, sizeof( DWORD ) );

	// First we write out the doors
	for( std::vector< InMesh::PPDoorStorage >::const_iterator d = inMesh.m_DoorStorage.begin(); d != inMesh.m_DoorStorage.end(); ++d )
	{
		// Write out each door
		expFile.Write( &(*d).door_id, sizeof( unsigned int ) );
		AddCRC32( checksum, (*d).door_id );
		expFile.Write( &(*d).center, sizeof( vector_3 ) );
		AddCRC32( checksum, (*d).center );
		expFile.Write( &(*d).x_axis, sizeof( vector_3 ) );
		AddCRC32( checksum, (*d).x_axis );
		expFile.Write( &(*d).y_axis, sizeof( vector_3 ) );
		AddCRC32( checksum, (*d).y_axis );
		expFile.Write( &(*d).z_axis, sizeof( vector_3 ) );
		AddCRC32( checksum, (*d).z_axis );
		expFile.Write( &(*d).num_verts, sizeof( int ) );
		AddCRC32( checksum, (*d).num_verts );
		expFile.Write( (*d).pverts, sizeof( int ) * (*d).num_verts );
		AddCRC32( checksum, (*d).pverts, sizeof( int ) * (*d).num_verts );
	}

	// Write out the spots
	for( std::vector< InMesh::PPSpotStorage >::const_iterator s = inMesh.m_SpotStorage.begin(); s != inMesh.m_SpotStorage.end(); ++s )
	{
		// Write out each spot
		expFile.Write( &(*s).x_axis, sizeof( vector_3 ) );
		AddCRC32( checksum, (*s).x_axis );
		expFile.Write( &(*s).y_axis, sizeof( vector_3 ) );
		AddCRC32( checksum, (*s).y_axis );
		expFile.Write( &(*s).z_axis, sizeof( vector_3 ) );
		AddCRC32( checksum, (*s).z_axis );
		expFile.Write( &(*s).center, sizeof( vector_3 ) );
		AddCRC32( checksum, (*s).center );
		expFile.Write( (*s).name, strlen( (*s).name ) + 1 );
		AddCRC32( checksum, (*s).name, strlen( (*s).name ) + 1 );
	}

	// Write out our list of vertices
	expFile.Write( m_pVertices, sizeof( storeVertex ) * m_numVertices );
	AddCRC32( checksum, m_pVertices, sizeof( storeVertex ) * m_numVertices );

	// Write out the stages
	for( TexStageList::const_iterator t	= m_TexStageList.begin(); t != m_TexStageList.end(); ++t )
	{
		// Instead of writing out of texture index here, we write out the actual texture name
		expFile.Write( inMesh.m_Textures[ (*t).tIndex ].c_str(), inMesh.m_Textures[ (*t).tIndex ].size() + 1 );
		AddCRC32( checksum, inMesh.m_Textures[ (*t).tIndex ].c_str(), inMesh.m_Textures[ (*t).tIndex ].size() + 1 );

		// Write out remaining stage information
		expFile.Write( &(*t).startIndex , sizeof( unsigned int ) );
		AddCRC32( checksum, (*t).startIndex );
		expFile.Write( &(*t).numVerts, sizeof( unsigned int ));
		AddCRC32( checksum, (*t).numVerts );
		expFile.Write( &(*t).numVIndices, sizeof( unsigned int ));
		AddCRC32( checksum, (*t).numVIndices );
		expFile.Write( (*t).pVIndices, sizeof( WORD ) * (*t).numVIndices );
		AddCRC32( checksum, (*t).pVIndices, sizeof( WORD ) * (*t).numVIndices );
	}

	// Success
	return true;
}


// Construction
PathAndBSP::PathAndBSP()
{
}

// Destruction
PathAndBSP::~PathAndBSP()
{
}

// Build the path and BSP information from the mesh
bool PathAndBSP::Build( InMesh& inMesh )
{
	// The first step in building logical information is to break up the mesh into seperated
	// lists that denote the logical node seperation.  Any isolated section of the mesh should
	// be it's own logical node, such as floor sections and wall sections.
	// Build a BSPTree out of the floor faces for this boundary mesh
	BSPTree* pTree	= NULL;
	LogicalGroup logicalGroups = BuildLogicalGrouping( inMesh.m_FloorFaces, inMesh, pTree );

	// Now every group of BSP leaves in the list is a new logical node
	unsigned char logicalNodeId	= 0;
	for( LogicalGroup::iterator n = logicalGroups.begin(); n != logicalGroups.end(); ++n )
	{
		// Setup initial logical node information
		LBUILDNODE	nNode;
		nNode.id				= logicalNodeId;
		logicalNodeId			+= 1;

		// Get the geometric information and setup the unique BSP for this node
		std::vector< unsigned int > trinormIndices;
		for( std::list< BSPNode* >::iterator b = (*n).begin(); b != (*n).end(); ++b )
		{
			for( unsigned int index = 0; index < (*b)->m_NumTriangles; ++index )
			{
				trinormIndices.push_back( (*b)->m_Triangles[ index ] );
			}
		}

		TriNorm* pnTriNorms	= new TriNorm[ trinormIndices.size() ];
		for( unsigned int index = 0; index < (trinormIndices.size()); ++index )
		{
			pnTriNorms[ index ] = pTree->GetTriangles()[ trinormIndices[ index ] ];
		}

		nNode.pBSPTree	= new BSPTree( pnTriNorms, (const unsigned short)trinormIndices.size() );

		// Cleanup
		delete[] pnTriNorms;

		// Setup the remainder of the information
		nNode.minBox	= nNode.pBSPTree->GetRoot()->m_MinBound;
		nNode.maxBox	= nNode.pBSPTree->GetRoot()->m_MaxBound;

		// Since we know that this is floor, we can say that this is humanoid walkable
		nNode.flags		= HUMANOID_WALKABLE | IS_FLOOR;

		// Make the leaf list and setup all local connections
		BuildLeafList( nNode );

		// Put this logical node on our pending list
		for( std::list< LBUILDNODE >::iterator bni = m_logicalList.begin(); bni != m_logicalList.end(); ++bni )
		{
			CalculateLogicalNodeConnections( &nNode, &(*bni) );
		}

		m_logicalList.push_back( nNode );
	}

	delete pTree;

	// Get the water groups
	logicalGroups	= BuildLogicalGrouping( inMesh.m_WaterFaces, inMesh, pTree );

	// Now every group of BSP leaves in the list is a new logical node
	for( n = logicalGroups.begin(); n != logicalGroups.end(); ++n )
	{
		// Setup initial logical node information
		LBUILDNODE	nNode;
		nNode.id				= logicalNodeId;
		logicalNodeId			+= 1;

		// Get the geometric information and setup the unique BSP for this node
		std::vector< unsigned int > trinormIndices;
		for( std::list< BSPNode* >::iterator b = (*n).begin(); b != (*n).end(); ++b )
		{
			for( unsigned int index = 0; index < (*b)->m_NumTriangles; ++index )
			{
				trinormIndices.push_back( (*b)->m_Triangles[ index ] );
			}
		}

		TriNorm* pnTriNorms	= new TriNorm[ trinormIndices.size() ];
		for( unsigned int index = 0; index < (trinormIndices.size()); ++index )
		{
			pnTriNorms[ index ] = pTree->GetTriangles()[ trinormIndices[ index ] ];
		}

		nNode.pBSPTree	= new BSPTree( pnTriNorms, (const unsigned short)trinormIndices.size() );

		// Cleanup
		delete[] pnTriNorms;

		// Setup the remainder of the information
		nNode.minBox	= nNode.pBSPTree->GetRoot()->m_MinBound;
		nNode.maxBox	= nNode.pBSPTree->GetRoot()->m_MaxBound;

		// Since we know that this is floor, we can say that this is humanoid walkable
		nNode.flags		= IS_WATER;

		// Make the leaf list and setup all local connections
		BuildLeafList( nNode );

		// Put this logical node on our pending list
		for( std::list< LBUILDNODE >::iterator bni = m_logicalList.begin(); bni != m_logicalList.end(); ++bni )
		{
			CalculateLogicalNodeConnections( &nNode, &(*bni) );
		}

		m_logicalList.push_back( nNode );
	}

	delete pTree;

	// Now we need to pull out the non-floor information and make a seperate logical
	// node with that information
	std::vector< UINT16 > indexList;
	for( unsigned int w = 0; w < inMesh.m_NumberOfTriangles; ++w )
	{
		// Make sure that this triangle isn't in the floor
		bool InFloor	= false;
		for( std::list< unsigned short >::iterator ff = inMesh.m_FloorFaces.begin(); ff != inMesh.m_FloorFaces.end(); ++ff )
		{
			if( w == (*ff) ){
				InFloor	= true;
				break;
			}
		}
		if( InFloor ){
			continue;
		}

		// Make sure that this triangle isn't in the water
		bool InWater	= false;
		for( ff = inMesh.m_WaterFaces.begin(); ff != inMesh.m_WaterFaces.end(); ++ff )
		{
			if( w == (*ff) ){
				InWater	= true;
				break;
			}
		}
		if( InWater ){
			continue;
		}

		// Since this isn't in the floor, add it
		int index	= w*3;
		indexList.push_back( inMesh.m_pIndices[ index ] );
		indexList.push_back( inMesh.m_pIndices[ index+1 ] );
		indexList.push_back( inMesh.m_pIndices[ index+2 ] );
	}

	// Create the final logical node
	if( !indexList.empty() )
	{
		LBUILDNODE	tnNode;
		tnNode.id			= logicalNodeId;

		tnNode.pBSPTree		= new BSPTree( inMesh.m_pVertices, (const unsigned short)inMesh.m_ArraySize, (unsigned short*)indexList.begin(), (const unsigned short)(indexList.size()/3) );
		tnNode.minBox		= tnNode.pBSPTree->GetRoot()->m_MinBound;
		tnNode.maxBox		= tnNode.pBSPTree->GetRoot()->m_MaxBound;
		tnNode.flags		= IS_WALL;

		// Make the leaf list and setup local connections
		BuildLeafList( tnNode );

		// Put this logical node on our pending list
		for( std::list< LBUILDNODE >::iterator bni = m_logicalList.begin(); bni != m_logicalList.end(); ++bni )
		{
			CalculateLogicalNodeConnections( &tnNode, &(*bni) );
		}

		m_logicalList.push_back( tnNode );
	}

	// Cleanup the main BSPTree
	return true;
}

// Write out path and BSP information to a file
bool PathAndBSP::Write( FileSys::File& expFile, DWORD& checksum )
{
	// Write the number of logical nodes we are about to write to file
	unsigned int num_lnodes	= m_logicalList.size();
	expFile.Write( &num_lnodes, sizeof( unsigned int ) );

	AddCRC32( checksum, num_lnodes );

	// Write all of the logical nodes out to file
	for( LogicalNodeList::iterator l = m_logicalList.begin(); l != m_logicalList.end(); ++l )
	{
		// Write out the node identifier
		expFile.Write( &(*l).id, sizeof( unsigned char ));
		AddCRC32( checksum, (*l).id );

		// Write out the bounding information
		expFile.Write( &(*l).minBox, sizeof( vector_3 ) );
		AddCRC32( checksum, (*l).minBox );
		expFile.Write( &(*l).maxBox, sizeof( vector_3 ) );
		AddCRC32( checksum, (*l).maxBox );

		// Write out the flags
		expFile.Write( &(*l).flags, sizeof( unsigned int ) );
		AddCRC32( checksum, (*l).flags );

		// Write out leaf connection info
		unsigned int num_lconnections	= (*l).leafConnectionInfo.size();
		expFile.Write( &num_lconnections, sizeof( unsigned int ) );
		AddCRC32( checksum, num_lconnections );

		for( std::list< LEAFINFO >::iterator lc = (*l).leafConnectionInfo.begin(); lc != (*l).leafConnectionInfo.end(); ++lc )
		{
			// Write out the leaf id
			expFile.Write( &(*lc).id, sizeof( unsigned short ) );
			AddCRC32( checksum, (*lc).id );

			// Write out the bounding information for this leaf
			expFile.Write( &(*lc).minBox, sizeof( vector_3 ) );
			AddCRC32( checksum, (*lc).minBox );
			expFile.Write( &(*lc).maxBox, sizeof( vector_3 ) );
			AddCRC32( checksum, (*lc).maxBox );
			expFile.Write( &(*lc).center, sizeof( vector_3 ) );
			AddCRC32( checksum, (*lc).center );

			// Write out the triangle information for this leaf
			unsigned short numtriangles	= (*lc).triangles.size();
			expFile.Write( &numtriangles, sizeof( unsigned short ));
			AddCRC32( checksum, numtriangles );

			for( std::list< unsigned short >::iterator t = (*lc).triangles.begin(); t != (*lc).triangles.end(); ++t )
			{
				expFile.Write( &(*t), sizeof( unsigned short ) );
				AddCRC32( checksum, (*t) );
			}

			// Write out local connections
			unsigned int num_localconnections	= (*lc).localConnections.size();
			expFile.Write( &num_localconnections, sizeof( unsigned int ) );
			AddCRC32( checksum, num_localconnections );

			for( std::list< unsigned short >::iterator localc = (*lc).localConnections.begin(); localc != (*lc).localConnections.end(); ++localc )
			{
				expFile.Write( &(*localc),sizeof( unsigned short ) );
				AddCRC32( checksum, (*localc) );
			}
		}

		unsigned int num_bconnections	= (*l).nodalConnectionInfo.size();
		expFile.Write( &num_bconnections, sizeof( unsigned int ) );
		AddCRC32( checksum, num_bconnections );

		for( std::list< LBUILDCOLLECT >::iterator bc = (*l).nodalConnectionInfo.begin(); bc != (*l).nodalConnectionInfo.end(); ++bc )
		{
			// Unique identifier of node (corresponds to logical mesh id)
			expFile.Write( &(*bc).m_farid, sizeof( unsigned char ));
			AddCRC32( checksum, (*bc).m_farid );

			// Connections
			unsigned int numNodalLeafConnections	= (*bc).m_nodalLeafConnections.size();
			expFile.Write( &numNodalLeafConnections, sizeof( unsigned int ));
			AddCRC32( checksum, numNodalLeafConnections );

			for( std::list< NLEAFCONNECT >::iterator nli = (*bc).m_nodalLeafConnections.begin(); nli != (*bc).m_nodalLeafConnections.end(); ++nli )
			{
				expFile.Write( &(*nli), sizeof( NLEAFCONNECT ) );
				AddCRC32( checksum, (*nli) );
			}
		}

		// Write out the number of triangles
		unsigned int num_triangles	= (*l).pBSPTree->GetRoot()->m_NumTriangles;
		expFile.Write( &num_triangles, sizeof( unsigned int ) );
		AddCRC32( checksum, num_triangles );

		// Write out the BSPTree's triangle information
		expFile.Write( (*l).pBSPTree->GetTriangles(), sizeof( TriNorm ) * num_triangles );
		AddCRC32( checksum, (*l).pBSPTree->GetTriangles(), sizeof( TriNorm ) * num_triangles );

		// Finally, we must write out the recursive BSP structure
		WriteBSPNodeToFile( expFile, (*l).pBSPTree->GetRoot(), checksum );
	}

	return true;
}

// Build logical groups from the given geometry and index list
PathAndBSP::LogicalGroup PathAndBSP::BuildLogicalGrouping( std::list< unsigned short >& indexList, InMesh& inMesh, BSPTree*& pTree )
{
	pTree = NULL;
	if( indexList.size() )
	{
		unsigned short* indices		= new unsigned short[ indexList.size() * 3 ];
		unsigned short* pFloor		= indices;
		for( std::list< unsigned short >::iterator ff = indexList.begin(); ff != indexList.end(); ++ff )
		{
			int index	= ((*ff)*3);
			(*pFloor++)	= inMesh.m_pIndices[ index ];
			(*pFloor++)	= inMesh.m_pIndices[ index+1 ];
			(*pFloor++)	= inMesh.m_pIndices[ index+2 ];
		}
		pTree	= new BSPTree( inMesh.m_pVertices, (const unsigned short)inMesh.m_ArraySize, indices, (const unsigned short)indexList.size(), 1, 64 );
		delete[] indices;
	}

	// Build the list of floor leaves
	std::list< BSPNode* >  Leaves;
	if( pTree )
	{
		BSPNode* pRoot	= pTree->GetRoot();

		std::list< BSPNode* > activeList;
		activeList.push_back( pRoot );

		while( !activeList.empty() )
		{
			BSPNode* pCurrentNode	= activeList.back();
			activeList.pop_back();

			if( pCurrentNode->m_IsLeaf )
			{
				Leaves.push_back( pCurrentNode );
			}
			else
			{
				if( pCurrentNode->m_LeftChild->m_NumTriangles )
				{
					activeList.push_back( pCurrentNode->m_LeftChild );
				}
				if( pCurrentNode->m_RightChild->m_NumTriangles )
				{
					activeList.push_back( pCurrentNode->m_RightChild );
				}
			}
		}
	}

	// Organize the floor leaves into connected groups
	LogicalGroup logicalGroups;

	for( std::list< BSPNode* >::iterator i = Leaves.begin(); i != Leaves.end(); ++i )
	{
		bool bInserted	= false;
		for( LogicalGroup::iterator o = logicalGroups.begin(); o != logicalGroups.end(); ++o )
		{
			for( std::list< BSPNode* >::iterator j = (*o).begin(); j != (*o).end(); ++j )
			{
				if( BoxIntersectsBox( (*j)->m_MinBound-mod, (*j)->m_MaxBound+mod, (*i)->m_MinBound-mod, (*i)->m_MaxBound+mod ) )
				{
					(*o).push_back( (*i) );
					bInserted	= true;
					break;
				}
			}

			if( bInserted )
			{
				break;
			}
		}

		if( !bInserted )
		{
			std::list< BSPNode* > newList;
			newList.push_back( (*i) );
			logicalGroups.push_back( newList );
		}
	}

	// This results in a very seperated tree which must be collapsed
	for( LogicalGroup::iterator l = logicalGroups.begin(); l != logicalGroups.end();)
	{
		bool bMerged	= false;
		for( LogicalGroup::iterator lc = logicalGroups.begin(); lc != logicalGroups.end(); ++lc )
		{
			if( l == lc )
			{
				continue;
			}

			// Compare the leaves of this group with the leaves of the compare group
			for( std::list< BSPNode* >::iterator j = (*l).begin(); j != (*l).end(); ++j )
			{
				for( std::list< BSPNode* >::iterator k = (*lc).begin(); k != (*lc).end(); ++k )
				{
					if( BoxIntersectsBox( (*j)->m_MinBound-mod, (*j)->m_MaxBound+mod, (*k)->m_MinBound-mod, (*k)->m_MaxBound+mod ) )
					{
						// Merge the groups
						(*l).merge( (*lc) );
						bMerged			= true;
						break;
					}
				}

				if( bMerged )
				{
					break;
				}
			}

			if( bMerged )
			{
				logicalGroups.erase( lc );
				break;
			}
		}

		if( bMerged )
		{
			l = logicalGroups.begin();
		}
		else
		{
			++l;
		}
	}

	return logicalGroups;
}

// Build's a leaf listing for the given node
void PathAndBSP::BuildLeafList( LBUILDNODE& nNode )
{
	std::list< NODETRICOLL >	nodeTriList;

	// Get the triangles from the BSPTree
	TriNorm* pTriangles	= nNode.pBSPTree->GetTriangles();

	// Go through all of the triangles for this logical node
	for( int i = 0; i < nNode.pBSPTree->GetNumTriangles(); ++i )
	{
		bool bFound	= false;
		for( std::list< NODETRICOLL >::iterator n = nodeTriList.begin(); n != nodeTriList.end(); ++n )
		{
			if( (IsOnPlane( (*n).triPlane, pTriangles[i].m_Vertices[0], 0.001f ) &&
				 IsOnPlane( (*n).triPlane, pTriangles[i].m_Vertices[1], 0.001f ) &&
				 IsOnPlane( (*n).triPlane, pTriangles[i].m_Vertices[2], 0.001f )) )
			{
				(*n).triangles.push_back( (unsigned short)i );

				for( unsigned int v = 0; v < 3; ++v )
				{
					// Get the vert
					vector_3& vert	= pTriangles[i].m_Vertices[v];

					// Get minimum boundaries
					(*n).minBox.x	= min( vert.x, (*n).minBox.x );
					(*n).minBox.y	= min( vert.y, (*n).minBox.y );
					(*n).minBox.z	= min( vert.z, (*n).minBox.z );

					// Get maximum boundaries
					(*n).maxBox.x	= max( vert.x, (*n).maxBox.x );
					(*n).maxBox.y	= max( vert.y, (*n).maxBox.y );
					(*n).maxBox.z	= max( vert.z, (*n).maxBox.z );
				}

				bFound	= true;
				break;
			}
		}

		if( !bFound )
		{
			// Build a fitted axis-aligned bounding box around this triangle
			vector_3 minBounds( FLOAT_MAX, FLOAT_MAX, FLOAT_MAX );
			vector_3 maxBounds( FLOAT_MIN, FLOAT_MIN, FLOAT_MIN );

			for( unsigned int v = 0; v < 3; ++v )
			{
				// Get the vert
				vector_3& vert	= pTriangles[i].m_Vertices[v];

				// Get minimum boundaries
				minBounds.x		= min( vert.x, minBounds.x );
				minBounds.y		= min( vert.y, minBounds.y );
				minBounds.z		= min( vert.z, minBounds.z );

				// Get maximum boundaries
				maxBounds.x		= max( vert.x, maxBounds.x );
				maxBounds.y		= max( vert.y, maxBounds.y );
				maxBounds.z		= max( vert.z, maxBounds.z );
			}

			NODETRICOLL nTriColl;
			nTriColl.minBox		= minBounds;
			nTriColl.maxBox		= maxBounds;
			nTriColl.triPlane	= plane_3( pTriangles[ i ].m_Normal,
										   -pTriangles[ i ].m_Normal.DotProduct( pTriangles[ i ].m_Vertices[0] ) );

			nTriColl.triangles.push_back( (unsigned short)i );

			nodeTriList.push_back( nTriColl );
		}
	}

	// Now go through each collected set of coplaner polygons and break them into connected sets.
	std::list< NODETRICOLL > newnodeTriList;
	std::set< unsigned short > visitedTris;
	for( std::list< NODETRICOLL >::iterator n = nodeTriList.begin(); n != nodeTriList.end(); ++n )
	{
		for( std::vector< unsigned short >::iterator t = (*n).triangles.begin(); t != (*n).triangles.end(); ++t )
		{
			if( visitedTris.find( (*t) ) != visitedTris.end() )
			{
				continue;
			}

			// Create a new group
			NODETRICOLL newTriColl;
			newTriColl.triPlane	= (*n).triPlane;
			newTriColl.minBox	= vector_3( FLOAT_MAX, FLOAT_MAX, FLOAT_MAX );
			newTriColl.maxBox	= vector_3( FLOAT_MIN, FLOAT_MIN, FLOAT_MIN );
			newTriColl.triangles.push_back( (*t) );
			visitedTris.insert( (*t) );

			for( unsigned int newTris = 0; newTris < newTriColl.triangles.size(); ++newTris )
			{
				TriNorm* pTriangle	= &pTriangles[ newTriColl.triangles[ newTris ] ];
				for( unsigned int v = 0; v < 3; ++v )
				{
					// Get the vert
					vector_3& vert	= pTriangle->m_Vertices[v];

					// Get minimum boundaries
					newTriColl.minBox.x	= min( vert.x, newTriColl.minBox.x );
					newTriColl.minBox.y	= min( vert.y, newTriColl.minBox.y );
					newTriColl.minBox.z	= min( vert.z, newTriColl.minBox.z );

					// Get maximum boundaries
					newTriColl.maxBox.x	= max( vert.x, newTriColl.maxBox.x );
					newTriColl.maxBox.y	= max( vert.y, newTriColl.maxBox.y );
					newTriColl.maxBox.z	= max( vert.z, newTriColl.maxBox.z );
				}

				for( std::vector< unsigned short >::iterator it = (*n).triangles.begin(); it != (*n).triangles.end(); ++it )
				{
					if( visitedTris.find( (*it) ) != visitedTris.end() )
					{
						continue;
					}

					bool bDone = false;

					// Compare this triangles verts with the current test triangle
					TriNorm* pTestTri	= &pTriangles[ (*it) ];
					for( unsigned int dt = 0; dt < 3; ++dt )
					{
						for( unsigned int dit = 0; dit < 3; ++dit )
						{
							if( pTriangle->m_Vertices[dt].IsEqual( pTestTri->m_Vertices[dit] ) )
							{
								newTriColl.triangles.push_back( (*it) );
								visitedTris.insert( (*it) );
								bDone = true;
								break;
							}
						}
						if( bDone )
						{
							break;
						}
					}
				}
			}

			newnodeTriList.push_back( newTriColl );
		}
	}

	unsigned short current_id	= 0;

	// Subdivide each unique node piece
	for( n = newnodeTriList.begin(); n != newnodeTriList.end(); ++n )
	{
		SubdivideLeaf( (*n), nNode, current_id );
	}
}

// Calculate the closest point, inside the box, that lies within the triangles of the
// leaf.  Returns false if it cannot find any.
int PathAndBSP::FindClosestPointInTriangles( LBUILDNODE& nNode, NODETRICOLL& nTriColl, LEAFINFO& nLeaf )
{
	vector_3 boxCenter	= nLeaf.minBox + ((nLeaf.maxBox - nLeaf.minBox) * 0.5f);

	vector_3 planePoint	= ClosestPointOnPlane( nTriColl.triPlane, boxCenter );
	if( PointInBox( nLeaf.minBox-mod, nLeaf.maxBox+mod, planePoint ) )
	{
		nLeaf.center	= planePoint;
	}
	else
	{
		// Determine the angle of incidence between the ray and the plane
		real const Incidence = DotProduct( nTriColl.triPlane.GetNormal(), vector_3( 0.0f, nLeaf.maxBox.y - nLeaf.minBox.y, 0.0f ) );
		if( !IsZero( Incidence ) )
		{
			// The ray interesects the plane
			// Compute the intersection distance
			real T = -(DotProduct( nTriColl.triPlane.GetNormal(), vector_3( boxCenter.x, nLeaf.minBox.y, boxCenter.z )) +
					   nTriColl.triPlane.GetD()) / Incidence;

			planePoint	= vector_3( boxCenter.x, nLeaf.minBox.y, boxCenter.z ) +
						  (T * vector_3( 0.0f, nLeaf.maxBox.y - nLeaf.minBox.y, 0.0f ));
			if( PointInBox( nLeaf.minBox-mod, nLeaf.maxBox+mod, planePoint ) )
			{
				nLeaf.center	= planePoint;
			}
			else
			{
				return -1;
			}
		}
		else
		{
			// Should never get here (Closest point should have caught it), but we fail anyways
			return -1;
		}
	}

	if( PointInBox( nLeaf.minBox-mod, nLeaf.maxBox+mod, nLeaf.center ) )
	{
		vector_3 castCenter	= nLeaf.center - (Normalize( nTriColl.triPlane.GetNormal() ) * 0.01f);

		// Go through all of the triangles that make up this leaf and make sure that the center lies inside
		// the boundary of one of them, which should be true for any valid pathing position
		for( std::list< unsigned short >::iterator nt = nLeaf.triangles.begin(); nt != nLeaf.triangles.end(); ++nt )
		{
			if( math::RayIntersectsTriangle( castCenter, nTriColl.triPlane.GetNormal(),
									   nNode.pBSPTree->GetTriangles()[ (*nt) ].m_Vertices[0],
									   nNode.pBSPTree->GetTriangles()[ (*nt) ].m_Vertices[1],
									   nNode.pBSPTree->GetTriangles()[ (*nt) ].m_Vertices[2] ) ||
				math::RayIntersectsTriangle( castCenter, -nTriColl.triPlane.GetNormal(),
									   nNode.pBSPTree->GetTriangles()[ (*nt) ].m_Vertices[0],
									   nNode.pBSPTree->GetTriangles()[ (*nt) ].m_Vertices[1],
									   nNode.pBSPTree->GetTriangles()[ (*nt) ].m_Vertices[2] ) )
			{
				break;
			}
		}

		if( nt != nLeaf.triangles.end() )
		{
			return (*nt);
		}

		// The closest point on the plane that makes up this particular leaf box is not inside any
		// of the triangles that intersect with it.  The most common cause of this case is a thin,
		// flat section of polygons set at an angle in X,Z.  In order to handle this case, we must
		// try to find a point that lies with the box given for this leaf that is also within
		// one of the triangles.  To do this we will search the line segements of each triangle
		// for a closest point that is within the box.
		for( nt = nLeaf.triangles.begin(); nt != nLeaf.triangles.end(); ++nt )
		{
			TriNorm& triangle	= nNode.pBSPTree->GetTriangles()[ (*nt) ];
			vector_3 testPoint( DoNotInitialize );

			if( ClosestPointOnLineSegment( triangle.m_Vertices[0],
										   triangle.m_Vertices[1] - triangle.m_Vertices[0],
										   nLeaf.center, testPoint ) &&
				PointInBox( nLeaf.minBox, nLeaf.maxBox, testPoint ) )
			{
				vector_3 diff	= testPoint - nLeaf.center;
				nLeaf.center	+= diff + (diff.Normalize_T() * 0.01f);
				return (*nt);
			}
			if( ClosestPointOnLineSegment( triangle.m_Vertices[1],
										   triangle.m_Vertices[2] - triangle.m_Vertices[1],
										   nLeaf.center, testPoint ) &&
				PointInBox( nLeaf.minBox, nLeaf.maxBox, testPoint ) )
			{
				vector_3 diff	= testPoint - nLeaf.center;
				nLeaf.center	+= diff + (diff.Normalize_T() * 0.01f);
				return (*nt);
			}
			if( ClosestPointOnLineSegment( triangle.m_Vertices[2],
										   triangle.m_Vertices[0] - triangle.m_Vertices[2],
										   nLeaf.center, testPoint ) &&
				PointInBox( nLeaf.minBox, nLeaf.maxBox, testPoint ) )
			{
				vector_3 diff	= testPoint - nLeaf.center;
				nLeaf.center	+= diff + (diff.Normalize_T() * 0.01f);
				return (*nt);
			}
		}
	}

	return -1;
}

// Subdivides a leaf node and creates smaller leaves for use in the logical nodes
void PathAndBSP::SubdivideLeaf( NODETRICOLL& nTriColl, LBUILDNODE& nNode, unsigned short& current_id )
{
	// What we want to accomplish here is the breakup of the given BSPNode into bite-size chunks.
	// In order to accomplish this, we will subdivide the node until no divided portion has a 2
	// dimensional volume along the X,Z axis greater than .5 meters.
	const float xaxisdiff	= nTriColl.maxBox.x - nTriColl.minBox.x;
	const float zaxisdiff	= nTriColl.maxBox.z - nTriColl.minBox.z;

	unsigned int numxdivs	= max( 1, FTOL( xaxisdiff / SUBDIVSIZE_INMETERS ) );
	unsigned int numzdivs	= max( 1, FTOL( zaxisdiff / SUBDIVSIZE_INMETERS ) );

	const double xsubdiff	= xaxisdiff / (double)numxdivs;
	const double zsubdiff	= zaxisdiff / (double)numzdivs;

#ifdef Y_DIMENSION_DIVIDE

	const float yaxisdiff	= nTriColl.maxBox.y - nTriColl.minBox.y;
	unsigned int numydivs	= max( 1, FTOL( yaxisdiff / SUBDIVSIZE_INMETERS ) );
	const double ysubdiff	= yaxisdiff / (double)numydivs;

	for( unsigned int x = 0; x < numxdivs; ++x )
	{
		for( unsigned int y = 0; y < numydivs; ++y )
		{
			for( unsigned int z = 0; z < numzdivs; ++z )
			{
				LEAFINFO nInfo;
				nInfo.id			= current_id++;

				nInfo.minBox.x		= nTriColl.minBox.x + ((float)x * (float)xsubdiff);
				nInfo.minBox.y		= nTriColl.minBox.y + ((float)y * (float)ysubdiff);
				nInfo.minBox.z		= nTriColl.minBox.z + ((float)z * (float)zsubdiff);

				nInfo.maxBox.x		= nInfo.minBox.x + (float)xsubdiff;
				nInfo.maxBox.y		= nInfo.minBox.y + (float)ysubdiff;
				nInfo.maxBox.z		= nInfo.minBox.z + (float)zsubdiff;

#else

	for( unsigned int x = 0; x < numxdivs; ++x )
	{
		// No Y division
		{
			for( unsigned int z = 0; z < numzdivs; ++z )
			{
				LEAFINFO nInfo;
				nInfo.id			= current_id++;

				nInfo.minBox.x		= nTriColl.minBox.x + ((float)x * (float)xsubdiff);
				nInfo.minBox.y		= nTriColl.minBox.y;
				nInfo.minBox.z		= nTriColl.minBox.z + ((float)z * (float)zsubdiff);

				nInfo.maxBox.x		= nInfo.minBox.x + (float)xsubdiff;
				nInfo.maxBox.y		= nTriColl.maxBox.y;
				nInfo.maxBox.z		= nInfo.minBox.z + (float)zsubdiff;

#endif

				for( std::vector< unsigned short >::iterator t = nTriColl.triangles.begin(); t != nTriColl.triangles.end(); ++t )
				{
					if( nNode.pBSPTree->TriIntersectsBox( nInfo.minBox, nInfo.maxBox, (*t) ) )
					{
						nInfo.triangles.push_back( (*t) );
					}
				}

				if( !nInfo.triangles.empty() )
				{
					// Find the closest point on the plane that makes up this leaf
					int centerTriangle = FindClosestPointInTriangles( nNode, nTriColl, nInfo );
					if( centerTriangle != -1 )
					{
						// Cull the list of triangles for this leaf down to only those that are connected,
						// directly or indirectly, with the hit triangle.
						std::vector< unsigned short > triGroup;
						triGroup.push_back( centerTriangle );
						for( unsigned int groupIndex = 0; groupIndex < triGroup.size(); ++groupIndex )
						{
							TriNorm* pGroupTri = &nNode.pBSPTree->GetTriangles()[ triGroup[ groupIndex ] ];
							for( std::list< unsigned short >::iterator lt = nInfo.triangles.begin(); lt != nInfo.triangles.end(); ++lt )
							{
								for( std::vector< unsigned short >::iterator findtri = triGroup.begin(); findtri != triGroup.end(); ++findtri )
								{
									if( (*lt) == (*findtri) )
									{
										break;
									}
								}
								if( findtri != triGroup.end() )
								{
									continue;
								}

								bool bFoundTri	= false;
								TriNorm* pTestTri = &nNode.pBSPTree->GetTriangles()[ (*lt) ];
								for( unsigned int gtriindex = 0; gtriindex < 3; ++gtriindex )
								{
									for( unsigned int ttriindex = 0; ttriindex < 3; ++ttriindex )
									{
										if( pGroupTri->m_Vertices[ gtriindex ].IsEqual( pTestTri->m_Vertices[ ttriindex ] ) )
										{
											// Found a matching vertex
											triGroup.push_back( (*lt) );
											bFoundTri = true;
											break;
										}
									}
									if( bFoundTri )
									{
										break;
									}
								}
							}
						}

						for( std::list< unsigned short >::iterator lt = nInfo.triangles.begin(); lt != nInfo.triangles.end(); )
						{
							for( std::vector< unsigned short >::iterator groupTri = triGroup.begin(); groupTri != triGroup.end(); ++groupTri )
							{
								if( (*lt) == (*groupTri) )
								{
									break;
								}
							}
							if( groupTri == triGroup.end() )
							{
								lt = nInfo.triangles.erase( lt );
							}
							else
							{
								++lt;
							}
						}

						// Paranoia check
						nInfo.localConnections.clear();

						// Calculate all of the connections
						CalculateLocalConnections( nNode, nInfo );

						// We have a leaf node, so put it on our list of leaves
						nNode.leafConnectionInfo.push_back( nInfo );
					}
					else
					{
						--current_id;
					}
				}
				else
				{
					--current_id;
				}
			}
		}
	}
}

// Calculate the local connections of the given leaf
void PathAndBSP::CalculateLocalConnections( LBUILDNODE& nNode, LEAFINFO& nLeaf )
{
	// Go through all current logical leaves for this node
	std::list< LEAFINFO >::iterator i = nNode.leafConnectionInfo.begin();
	for( ; i != nNode.leafConnectionInfo.end(); ++i )
	{
		if( LogicalLeavesIntersect( nLeaf, nNode, (*i), nNode ) )
		{
			ConnectLeafs( nLeaf, (*i) );
		}
	}
}

// Do these two leaves intersect?
bool PathAndBSP::LogicalLeavesIntersect( LEAFINFO& leaf1, LBUILDNODE& node1, LEAFINFO& leaf2, LBUILDNODE& node2 )
{
	// Local connection
	if( BoxIntersectsBox( leaf1.minBox-mod, leaf1.maxBox+mod, leaf2.minBox-mod, leaf2.maxBox+mod ) )
	{
		for( std::list< unsigned short >::iterator t1 = leaf1.triangles.begin(); t1 != leaf1.triangles.end(); ++t1 )
		{
			for( unsigned int i = 0; i < 3; ++i )
			{
				for( std::list< unsigned short >::iterator t2 = leaf2.triangles.begin(); t2 != leaf2.triangles.end(); ++t2 )
				{
					for( unsigned int o = 0; o < 3; ++o )
					{
						vector_3 testVect = node1.pBSPTree->GetTriangles()[ (*t1) ].m_Vertices[i] - node2.pBSPTree->GetTriangles()[ (*t2) ].m_Vertices[o];
						if( testVect.IsZero( CONNECTION_TOLERANCE ) )
						{
							goto LLIEND;
						}
					}
				}
			}
		}
	}

	return false;

LLIEND:

	float	ray_t	= FLOAT_MAX;
	vector_3 face_normal( DoNotInitialize );
	float max_y		= max( node1.maxBox.y, node2.maxBox.y ) + 0.1f;

	// Now we need to do poly testing.  Whee fun!
	vector_3 testVector	= leaf2.center - leaf1.center;
	for( unsigned int i = 1; i < 16; ++i )
	{
		vector_3 testPoint1	= leaf1.center + ( testVector * ((0.0625f * (float)i) - 0.005f) );
		vector_3 testPoint2	= leaf1.center + ( testVector * ((0.0625f * (float)i) + 0.005f) );
		testPoint1.y = testPoint2.y = max_y;

		if( !node1.pBSPTree->RayIntersectTree( testPoint1, vector_3( 0.0f, -1.0f, 0.0f ), ray_t, face_normal ) &&
			!node2.pBSPTree->RayIntersectTree( testPoint2, vector_3( 0.0f, -1.0f, 0.0f ), ray_t, face_normal ) )
		{
			return false;
		}
	}

	return true;
}

// Connect two leafs together
void PathAndBSP::ConnectLeafs( LEAFINFO& leaf1, LEAFINFO& leaf2 )
{
	// Local connection because we are in the same logical node
	bool IsAlreadyInList	= false;
	std::list< unsigned short >::iterator i;

	for( i = leaf1.localConnections.begin(); i != leaf1.localConnections.end(); ++i )
	{
		if( (*i) == leaf2.id )
		{
			IsAlreadyInList = true;
			break;
		}
	}
	if( !IsAlreadyInList )
	{
		leaf1.localConnections.push_back( leaf2.id );
	}

	IsAlreadyInList		= false;
	for( i = leaf2.localConnections.begin(); i != leaf2.localConnections.end(); ++i )
	{
		if( (*i) == leaf1.id )
		{
			IsAlreadyInList = true;
			break;
		}
	}
	if( !IsAlreadyInList )
	{
		leaf2.localConnections.push_back( leaf1.id );
	}
}

// Calculate the internodal connections
void PathAndBSP::CalculateLogicalNodeConnections( LBUILDNODE* pnNode, LBUILDNODE* pfNode )
{
	bool bConnectNodes	= false;

	LBUILDCOLLECT*	pnNodeConnect	= NULL;
	LBUILDCOLLECT*	pfNodeConnect	= NULL;

	vector_3 minBox	= pfNode->minBox - mod;
	vector_3 maxBox	= pfNode->maxBox + mod;

	// Go through all the leaves
	for( std::list< LEAFINFO >::iterator i = pnNode->leafConnectionInfo.begin(); i != pnNode->leafConnectionInfo.end(); ++i )
	{
		if( BoxIntersectsBox( (*i).minBox-mod, (*i).maxBox+mod, minBox, maxBox ) )
		{
			for( std::list< LEAFINFO >::iterator o = pfNode->leafConnectionInfo.begin(); o != pfNode->leafConnectionInfo.end(); ++o )
			{
				if( LogicalLeavesIntersect( (*i), *pnNode, (*o), *pfNode ) )
				{
					if( !bConnectNodes )
					{
						bConnectNodes	= true;
						ConnectLogicalNodes( pnNode, pnNodeConnect, pfNode, pfNodeConnect );
					}

					ConnectNodalLeafs( pnNodeConnect, (*i).id, pfNodeConnect, (*o).id );
				}
			}
		}
	}
}

// Connect two logical nodes together
void PathAndBSP::ConnectLogicalNodes( LBUILDNODE* pnNode, LBUILDCOLLECT* &pnNodeConnect, LBUILDNODE* pfNode, LBUILDCOLLECT* &pfNodeConnect )
{
	// The nodes intersect, so we insert connection info into both of the nodes
	// if the connection specified has not already been calculated
	LBUILDCOLLECT nCollection;

	nCollection.m_farid	= pfNode->id;
	pnNode->nodalConnectionInfo.push_back( nCollection );
	pnNodeConnect	= &pnNode->nodalConnectionInfo.back();

	nCollection.m_farid	= pnNode->id;
	pfNode->nodalConnectionInfo.push_back( nCollection );
	pfNodeConnect	= &pfNode->nodalConnectionInfo.back();
}

// Connect two nodal leafs
void PathAndBSP::ConnectNodalLeafs( LBUILDCOLLECT* pnNodeConnect, unsigned short nID, LBUILDCOLLECT* pfNodeConnect, unsigned short fID )
{
	NLEAFCONNECT nConnection;

	nConnection.local_id	= nID;
	nConnection.far_id		= fID;
	pnNodeConnect->m_nodalLeafConnections.push_back( nConnection );

	nConnection.local_id	= fID;
	nConnection.far_id		= nID;
	pfNodeConnect->m_nodalLeafConnections.push_back( nConnection );
}

// Recursive function for writing a BSP tree to a file
void PathAndBSP::WriteBSPNodeToFile( FileSys::File& expFile, BSPNode* pNode, DWORD& checksum )
{
	// Write this node's bounding information to a file
	expFile.Write( &pNode->m_MinBound, sizeof( vector_3 ) );
	AddCRC32( checksum, pNode->m_MinBound );
	expFile.Write( &pNode->m_MaxBound, sizeof( vector_3 ) );
	AddCRC32( checksum, pNode->m_MaxBound );

	// Write whether this is a leaf or not
	expFile.Write( &pNode->m_IsLeaf, sizeof( bool ));
	AddCRC32( checksum, pNode->m_IsLeaf );

	// Write triangle index information
	expFile.Write( &pNode->m_NumTriangles, sizeof( unsigned short ));
	AddCRC32( checksum, pNode->m_NumTriangles );
	expFile.Write( pNode->m_Triangles, sizeof( unsigned short ) * pNode->m_NumTriangles );
	AddCRC32( checksum, pNode->m_Triangles, sizeof( unsigned short ) * pNode->m_NumTriangles );

	// Write information describing the child structure of this node
	unsigned char children	= 0;
	if( pNode->m_LeftChild && pNode->m_RightChild )
	{
		children	= 2;
	}
	expFile.Write( &children, sizeof( unsigned char ) );
	AddCRC32( checksum, children );

	//Write left child
	if( children )
	{
		WriteBSPNodeToFile( expFile, pNode->m_LeftChild, checksum );
		WriteBSPNodeToFile( expFile, pNode->m_RightChild, checksum );
	}
}
