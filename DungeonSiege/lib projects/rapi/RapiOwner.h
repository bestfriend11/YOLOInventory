#pragma once
#ifndef _RAPI_OWNER_
#define _RAPI_OWNER_
/*************************************************************************************
**
**									RapiOwner
**
**		This is a test class for the purposes of planning out the different elements
**		and implementation methods of a high quality 3D abstraction layer.
**
**		Date:	05/26/99
**		Author: James Loe
**
*************************************************************************************/


// Forward declaration to placate the compiler through these includes
class Rapi;
class RapiFont;
class RapiOwner;
class RapiTexture;
class RapiSurface;
class RapiMemImage;
enum  ePixelFormat;

/*************************************************/
// Yeah, I know what you're thinking.  Why the hell did
// James put these includes into the .h file?  Well,
// the reason for this is so that the client can include
// one .h file and get all needed includes for the layer.

/* STANDARD INCLUDES NEEDED FOR RAPI SUPPORT */
#define D3D_OVERLOADS

#pragma warning ( push, 1 )		// ignore any warnings or changes that win32 has
#include <d3d8.h>
#pragma warning ( pop )			// back to normal

#include <vector>
#include <list>
#include <map>
#include <set>
#include "gpcoll.h"

#include "bucketvector.h"
#include "vector_3.h"
#include "matrix_3x3.h"

/* END STANDARD INCLUDES NEEDED FOR RAPI SUPPORT */
/*************************************************/

/**************** MACROS **********************/

// Macro for the release of devices
#define RELEASE( p ) ( (p) ? ((p)->Release(), p = NULL) : NULL )

/**********************************************/

/**************** DEFINES *********************/

#define LIGHT_CACHE		128						// Maximum number of lights that the system can handle
#define TEX_CACHE		1024					// Maximum number of textures that the system can handle
#define NUM_TEXTURES	8						// Number of simultaneous multitexture to attempt
#define NO_TEXTURE		-1						// Used to tell me that you don't want a texture
#define NUM_MIPLEVELS	8						// How many mipmap levels do you want me to generate
#define MATRIXSTACKSIZE 32						// Number of matrices to reserve on the stack
#define SIZE_VBUFFER	16384					// Number of vertices in the main vertex buffer
#define SIZE_TVBUFFER	8192					// Number of vertices in the transformed vertex buffer
#define MANUALZBIAS		0.000025f				// Scalar amount for manual z biasing (added to nearplane)
#define LOWVIDMEM		10000000				// Indicates amount of memory considered low for video
#define LOWSYSMEM		66000000				// Indicates amount of memory considered low for the system
#define DP_TYPE			_D3DPRIMITIVETYPE		// Primitive drawing types
#define TEX_OP			_D3DTEXTUREOP			// Texture operations (modulate, add, etc.)
#define TEX_AD			_D3DTEXTUREADDRESS		// Texture addressing operations (wrap, clamp)
#define TEX_MGF			_D3DTEXTUREFILTERTYPE	// Texture mag filtering (point, linear, anisotropic)
#define TEX_MNF			_D3DTEXTUREFILTERTYPE	// Texture min filtering (point, linear, anisotropic)
#define TEX_MPF			_D3DTEXTUREFILTERTYPE	// Texture mip filtering (point, linear, anisotropic)
#define TEX_BLD			_D3DBLEND				// Blending operations
#define LTYPE			_D3DLIGHTTYPE			// Type of light
#define LCOLOR			_D3DCOLORVALUE			// Color of light

#define LVERTEX			(D3DFVF_TEX1 | D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEXCOORDSIZE2(0) )
#define MVERTEX			(D3DFVF_TEX2 | D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE2(1) )

// We can disable the 'slimming down' of vertices --biddle
#ifdef RAPI_USES_FULL_D3DVERTEX
#define SVERTEX			D3DFVF_VERTEX	// radeon test D3DFVF_LVERTEX
#define TVERTEX			D3DFVF_TLVERTEX
#define GVERTEX			D3DFVF_LVERTEX
#else
#define SVERTEX			(D3DFVF_TEX1 | D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEXCOORDSIZE2(0) )
#define TVERTEX			(D3DFVF_TEX1 | D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEXCOORDSIZE2(0) )
#define GVERTEX			(D3DFVF_XYZ	 | D3DFVF_DIFFUSE)
#endif


/**********************************************/

/**************** ENUMERATIONS ****************/

enum TEX_OPT
{
	TEX_DYNAMIC = 0,	// For frequently changing textures, like procedural stuff	(SLOWEST)
	TEX_STATIC	= 1,	// For textures that change, but not very often				(FAST)
	TEX_LOCKED	= 2,	// For textures that NEVER change (will do auto compress)	(FASTEST)
};

static D3DFORMAT PreferredDepthFormats[] =
{
	D3DFMT_D24X8,
	D3DFMT_D24S8,
	D3DFMT_D32,
	D3DFMT_D16,
	D3DFMT_D15S1,
	D3DFMT_D24X4S4,
};

static D3DFORMAT PreferredOpaqueTexFormats[] =
{
	D3DFMT_X8R8G8B8,
	D3DFMT_A8R8G8B8,
/*
	D3DFMT_R5G6B5,
	D3DFMT_X1R5G5B5,
	D3DFMT_A1R5G5B5,
	D3DFMT_X4R4G4B4,
	D3DFMT_A4R4G4B4,
*/
};

static D3DFORMAT PreferredAlphaTexFormats[] =
{
	D3DFMT_A8R8G8B8,
/*
	D3DFMT_A4R4G4B4,
	D3DFMT_A1R5G5B5,
	D3DFMT_A8R3G3B2,
*/
};

/**********************************************/

/**************** STRUCTURES ******************/

// ARGB struct
struct ARGB
{
	BYTE b;
	BYTE g;
	BYTE r;
	BYTE a;
};

// Texture addressing structure
struct UV
{
	float u, v;							// Tex coords
};

// Untransformed vertex with normal, color, and texture info
struct lVertex
{
	float x, y, z;						// Position
	float nx, ny, nz;					// Normal
	DWORD color;						// Color   // colour not allowed in basic (non-fvf) primitive -- biddle
	UV	  uv;							// Tex coordinate set

	bool operator==(const lVertex& l)
	{
		return( !memcmp( &l, this, sizeof( lVertex ) ) );
	}
};


// Untransformed vertex with color and texture
struct sVertex
{
	float x, y, z;
#ifdef RAPI_USES_FULL_D3DVERTEX
	DWORD reserved;
	DWORD color;
	DWORD speccolor;
#else
	DWORD color;
#endif
	UV	  uv;

	bool operator==(const sVertex& s)
	{
		return( !memcmp( &s, this, sizeof( sVertex ) ) );
	}
};


// Untransformed vertex with color and 2 texture sets
struct mVertex
{
	float x, y, z;
	DWORD color;
	UV	  uv[2];

	bool operator==(const mVertex& m)
	{
		return( !memcmp( &m, this, sizeof( mVertex ) ) );
	}
};

// Transformed standard vertex structure
struct tVertex
{
	float x, y, z;
	float rhw;
	DWORD color;
#ifdef RAPI_USES_FULL_D3DVERTEX
	DWORD speccolor;
#endif
	UV	  uv;

	bool operator==(const tVertex& t)
	{
		return( !memcmp( &t, this, sizeof( tVertex ) ) );
	}
};

// Untransformed generic vertex
struct gVertex
{
	float x, y, z;
#ifdef RAPI_USES_FULL_D3DVERTEX
	float nx, ny, nz;
#endif
	DWORD color;

	bool operator==(const gVertex& g)
	{
		return( !memcmp( &g, this, sizeof( gVertex ) ) );
	}
};

// Triangle description structure
struct sTriangle
{
	unsigned int vIndex[3];				// Vertex indices into list
	unsigned int tIndex;				// Texture index into list
};

// Texture stage description structure
struct sTexStage
{
	unsigned int tIndex;				// Which texture is active in this stage
	unsigned int startIndex;			// Where to start the indices in the VBuffer
	unsigned int numVerts;				// Number of vertices
	WORD*		 pVIndices;				// Pointer to the indices for this stage
	unsigned int numVIndices;			// Number of indices in this stage
	DWORD*		 pLIndices;				// Pointer to the lighting indices
	unsigned int numLIndices;			// Number of lighting indices
};

// Device object description
struct AdapterDesc
{
	unsigned int			adapterNum;
	char					driverDesc[256];
	D3DDISPLAYMODE*			displayModes;
	unsigned int			numDisplayModes;
	D3DDISPLAYMODE			activeMode;
};

// Triangle vertex buffer combination description
struct TriVBDescription
{
	sTexStage*				pStage;
	LPDIRECT3DVERTEXBUFFER8	pBuffer;
	unsigned int*			pLightingInfo;
	D3DMATRIX				matrix;
	bool					bTile;
};

// Struct that identifies a display mode
struct DisplayMode
{
	int						m_Width;
	int						m_Height;
	int						m_BPP;
};

// Struct that identifies a valid system mode
struct ValidSysMode
{
	int						m_Width;
	int						m_Height;
	int						m_MaxBackBuffers;
};

// Struct that holds information about a given detail level
struct SysDetail
{
	// Memory and bits per pixel
	unsigned int				memory;
	unsigned int				bpp;

	// Detail level assigned to this detail level
	int							detail;

	// Shadow texture size
	int							shadowTexSize;

	// List of valid display modes
	std::vector< ValidSysMode >	valid_modes;
};

// Texture stage state information
struct TextureStageState
{
	TEX_OP	colorOp;	// Texturing operation
	TEX_OP  alphaOp;	// Alpha operation
	TEX_AD	addressU;	// Address mode in U direction
	TEX_AD	addressV;	// Address mode in V direction
	TEX_MGF	magFilter;	// Expansion filter
	TEX_MNF minFilter;	// Shrinking filter
	TEX_MPF mipFilter;	// Mipmap filter
	DWORD	colorArg1;	// Color argument
	DWORD	colorArg2;	// Color argument
	DWORD	alphaArg1;	// Alpha argument
	DWORD	alphaArg2;	// Alpha argument
};

// Static object texture info
struct StaticObjectTex
{
	DWORD	textureId;
	bool	alpha;
	bool	noalphasort;
};


/**************************************************/

/***************** TYPEDEFS ***********************/

typedef			std::map< int, Rapi* >									RapiMap;
typedef			RapiMap::iterator										RapiIter;
typedef			std::vector< AdapterDesc >								DescList;
typedef			DescList::iterator										DescListIter;
typedef			LPDIRECT3DVERTEXBUFFER8									VBuffer;
typedef			std::map< gpstring, unsigned int, istring_less >		SurfaceMap;
typedef			std::multimap< gpstring, unsigned int, istring_less >	TextureMap;
typedef			std::set< gpstring, istring_less >						TextureSet;
typedef			std::set< RapiFont* >									FontList;
typedef			FontList::iterator										FontIter;
typedef			std::vector< D3DMATRIX >								MatrixStack;
typedef			std::list< SysDetail >									SystemDetailList;
typedef			UnsafeBucketVector< RapiTexture* >						TextureBucketVector;
typedef			UnsafeBucketVector< RapiSurface* >						SurfaceBucketVector;
typedef			std::vector< DisplayMode >								DisplayList;
typedef			std::vector< ValidSysMode >								ValidModeList;
typedef			stdx::linear_map< unsigned int, float >					SurfaceExpirationMap;
typedef			UnsafeBucketVector< RapiMemImage* >						MemImageBucketVector;

/*************************************************/

/**************** RAPI INCLUDES ******************/

#include "RapiSurface.h"
#include "RapiTexture.h"
#include "RapiLight.h"
#include "RapiFont.h"
#include "RapiStaticObject.h"
#include "Rapi.h"
#include "RapiMath.h"

/**************************************************/

/************** IMPLEMENTATION ********************/

/*
**	Defines the RapiOwner class.
**
**	RapiOwner is a top level class responsible for managing all 3D
**  calls in the game
*/
class RapiOwner : public Singleton <RapiOwner>
{

public:

	// Constructor and destructor are public
	RapiOwner();
	~RapiOwner();

	// Get the Direct3D handle
	LPDIRECT3D8		GetDirect3D()						{ return m_pD3D; }

	// Do device enumeration and setup
	HRESULT			Initialize( const char *driverdesc = NULL );

	// Surface creation and the remainder of preparation for drawing
	bool			PrepareToDraw( HWND hwnd, 
								   int requestedDeviceID = 0,
								   bool bFullScreen = true,
								   int fsScreenWidth = 640,
								   int fsScreenHeight = 480,
								   int fsBPP = 16,
								   bool bVsync = true,
								   bool bFastFPU = true,
								   bool bFlip = true );

	// Return requested device
	Rapi*			GetDevice( int rDevice = 0 );

	// Return requested device, assert if invalid
	Rapi&			GetValidDevice( int rDevice = 0 )	{ Rapi* rapi = GetDevice( rDevice );
														  gpassert( rapi != NULL );
														  return( *rapi ); }

	// Get the default (first) device
	Rapi*			GetDefaultDevice()					{ return m_pDefaultDevice; }

	// Select a device from list.  This will either let the user do it, or it
	// will use a previously selected device.
	AdapterDesc		SelectDevice();

	// Get the description of active device
	const gpstring&	GetActiveDeviceDescription()		{  return m_sActiveDescription;  }

	// Disable all active devices
	void			StopDrawing( int rDevice = 0 );

	// Pausing and unpausing the drawing
	void			Pause( bool bPause );

	// Force a resume if not already resumed
	void			ForceResume();

	// Invalidates all textures that are currently created.  Used when windowed apps resize.
	void			InvalidateTextures( int rDevice = 0 );

	// Restore all of the textures that were available before the InvalidateTextures call
	void			RestoreTexturesAndLights( int rDevice = 0 );

	// Get the number of devices currently active
	int				GetNumActiveDevices()	{ return m_Devices.size(); }

	// Paint the back buffer(s) to the front buffer(s)
	void			OnWmPaint( const RECT& dstRect, const RECT& srcRect );

	// Return our state
	bool			IsFullScreen()			{ return m_bFullScreen; }
	bool			IsPaused()				{ return m_nPauseCount != 0; }

#	if !GP_RETAIL
	bool			IsTracingTextures()		{ return m_bTraceTextures; }
#	endif // !GP_RETAIL

private:

	// Handle to the parent window
	HWND			m_hWnd;

	// Direct3D handle
	LPDIRECT3D8		m_pD3D;

	// State variable to track screen
	bool			m_bFullScreen;

	// Track the state of the devices
	int				m_nPauseCount;

	// Maintains a list of our available and valid display descriptors
	DescList		m_Desclist;
	AdapterDesc		m_ActiveDesc;
	gpstring		m_sActiveDescription;

	// Maintains a list of our active display devices
	RapiMap			m_Devices;

	// Default device
	Rapi*			m_pDefaultDevice;

#	if !GP_RETAIL
	bool			m_bTraceTextures;
#	endif // !GP_RETAIL
};

#define gRapiOwner      RapiOwner::GetSingleton()
#define gDefaultRapi    (*gRapiOwner.GetDefaultDevice())
#define gDefaultRapiPtr gRapiOwner.GetDefaultDevice()

#if GP_DEBUG

	#define ONCE_PER_FRAME														\
	{																			\
		static unsigned int LastFrameExecuted;									\
		static std::set< void* > PointerMapping;								\
		if( LastFrameExecuted != gDefaultRapi.GetFrameCount() )					\
		{																		\
			PointerMapping.clear();												\
		}																		\
		else if( PointerMapping.find( (void*)this ) != PointerMapping.end() )	\
		{																		\
			gperror( "Cannot call here more than once per frame!" );			\
		}																		\
		PointerMapping.insert( (void*)this );									\
		LastFrameExecuted = gDefaultRapi.GetFrameCount();						\
	}

#else

	#define ONCE_PER_FRAME

#endif

class RapiPause
{
public:
	RapiPause( void )
	{
		if ( RapiOwner::DoesSingletonExist() )
		{
			gRapiOwner.Pause( true );
		}
	}

   ~RapiPause( void )
	{
		if ( RapiOwner::DoesSingletonExist() )
		{
			gRapiOwner.Pause( false );
		}
	}
};

#endif

