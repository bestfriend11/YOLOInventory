/*************************************************************************************
**
**									Rapi
**
**		This is a test class for the purposes of planning out the different elements
**		and implementation methods of a high quality 3D abstraction layer.
**
**		Date:	05/26/99
**		Author: James Loe
**
*************************************************************************************/



#include "precomp_rapi.h"

#include "GpStatsDefs.h"
#include "RapiAppModule.h"
#include "RapiOwner.h"
#include "RapiPrimitive.h"
#include "RapiImage.h"
#include "RapiMouse.h"

// save .dsp's the trouble of adding these to lib line
#pragma comment ( lib, "d3d8" )
#pragma comment ( lib, "dxguid" )

HRESULT D3DMath_MatrixInvert( D3DMATRIX& q, D3DMATRIX& a )
{
    if( fabs(a._44 - 1.0f) > .001f)
        return E_INVALIDARG;
    if( fabs(a._14) > .001f || fabs(a._24) > .001f || fabs(a._34) > .001f )
        return E_INVALIDARG;

    FLOAT fDetInv = INVERSEF( a._11 * ( a._22 * a._33 - a._23 * a._32 ) -
                              a._12 * ( a._21 * a._33 - a._23 * a._31 ) +
                              a._13 * ( a._21 * a._32 - a._22 * a._31 ) );

    q._11 =  fDetInv * ( a._22 * a._33 - a._23 * a._32 );
    q._12 = -fDetInv * ( a._12 * a._33 - a._13 * a._32 );
    q._13 =  fDetInv * ( a._12 * a._23 - a._13 * a._22 );
    q._14 = 0.0f;

    q._21 = -fDetInv * ( a._21 * a._33 - a._23 * a._31 );
    q._22 =  fDetInv * ( a._11 * a._33 - a._13 * a._31 );
    q._23 = -fDetInv * ( a._11 * a._23 - a._13 * a._21 );
    q._24 = 0.0f;

    q._31 =  fDetInv * ( a._21 * a._32 - a._22 * a._31 );
    q._32 = -fDetInv * ( a._11 * a._32 - a._12 * a._31 );
    q._33 =  fDetInv * ( a._11 * a._22 - a._12 * a._21 );
    q._34 = 0.0f;

    q._41 = -( a._41 * q._11 + a._42 * q._21 + a._43 * q._31 );
    q._42 = -( a._41 * q._12 + a._42 * q._22 + a._43 * q._32 );
    q._43 = -( a._41 * q._13 + a._42 * q._23 + a._43 * q._33 );
    q._44 = 1.0f;

    return S_OK;
}

VOID D3DMath_MatrixMultiply( D3DMATRIX& out, D3DMATRIX& l, D3DMATRIX& r )
{
	gpassert( (&out != &l) && (&out != &r) );

	out._11 = (l._11 * r._11) + (l._12 * r._21) + (l._13 * r._31) + (l._14 * r._41);
	out._12 = (l._11 * r._12) + (l._12 * r._22) + (l._13 * r._32) + (l._14 * r._42);
	out._13 = (l._11 * r._13) + (l._12 * r._23) + (l._13 * r._33) + (l._14 * r._43);
	out._14 = (l._11 * r._14) + (l._12 * r._24) + (l._13 * r._34) + (l._14 * r._44);

	out._21 = (l._21 * r._11) + (l._22 * r._21) + (l._23 * r._31) + (l._24 * r._41);
	out._22 = (l._21 * r._12) + (l._22 * r._22) + (l._23 * r._32) + (l._24 * r._42);
	out._23 = (l._21 * r._13) + (l._22 * r._23) + (l._23 * r._33) + (l._24 * r._43);
	out._24 = (l._21 * r._14) + (l._22 * r._24) + (l._23 * r._34) + (l._24 * r._44);

	out._31 = (l._31 * r._11) + (l._32 * r._21) + (l._33 * r._31) + (l._34 * r._41);
	out._32 = (l._31 * r._12) + (l._32 * r._22) + (l._33 * r._32) + (l._34 * r._42);
	out._33 = (l._31 * r._13) + (l._32 * r._23) + (l._33 * r._33) + (l._34 * r._43);
	out._34 = (l._31 * r._14) + (l._32 * r._24) + (l._33 * r._34) + (l._34 * r._44);

	out._41 = (l._41 * r._11) + (l._42 * r._21) + (l._43 * r._31) + (l._44 * r._41);
	out._42 = (l._41 * r._12) + (l._42 * r._22) + (l._43 * r._32) + (l._44 * r._42);
	out._43 = (l._41 * r._13) + (l._42 * r._23) + (l._43 * r._33) + (l._44 * r._43);
	out._44 = (l._41 * r._14) + (l._42 * r._24) + (l._43 * r._34) + (l._44 * r._44);
}

// Convert a render target format into a display format (remove any alpha)
D3DFORMAT GetDisplayFormat( D3DFORMAT renderTargetFormat )
{
	switch( renderTargetFormat )
	{
		case D3DFMT_A8R8G8B8:
		{
			return D3DFMT_X8R8G8B8;
			break;
		}

		case D3DFMT_A1R5G5B5:
		{
			return D3DFMT_X1R5G5B5;
			break;
		}

		case D3DFMT_A4R4G4B4:
		{
			return D3DFMT_X4R4G4B4;
			break;
		}

		default:
		{
			return renderTargetFormat;
			break;
		}
	};
}

// Constructor
Rapi::Rapi()
	: m_hwnd( 0 )
	, m_pD3D( NULL )
	, m_pD3DDevice( NULL )
	, m_pBackBuffer( NULL )
	, m_bManualTextureProjection( true )
	, m_bManualFrustumClipping( false )
	, m_bOnlySquareTextures( false )
	, m_b16bitOnly( false )
	, m_bForceTexRestore( false )
	, m_bColorControl( false )
	, m_bMultiTexBlend( true )
	, m_bTextureStateReset( false )
	, m_bModulateOnly( false )
	, m_bNoAdd( false )
	, m_bShadowRenderTarget( false )
	, m_pZBuffer( NULL )
	, m_pVBuffer( NULL )
	, m_VBufferPosition( 0 )
	, m_bVBufferLocked( false )
	, m_pLVBuffer( NULL )
	, m_LVBufferPosition( 0 )
	, m_bLVBufferLocked( false )
	, m_pTVBuffer( NULL )
	, m_TVBufferPosition( 0 )
	, m_bTVBufferLocked( false )
	, m_pIndexBuffer( NULL )
	, m_IndexBufferPosition( 0 )
	, m_bIndexBufferLocked( false )
	, m_bufferFlags( 0 )
	, m_bufferMem( D3DPOOL_DEFAULT )
	, m_Width( 0 )
	, m_Height( 0 )
	, m_BPP( 0 )
	, m_shadowTexSize( 64 )
	, m_mipFilter( D3DTEXF_LINEAR )
	, m_bTrilinearCapable( true )
	, m_bInScene( false )
	, m_bFlipBltOnBegin( false )
	, m_bPaused( false )
	, m_bFullScreen( true )
	, m_bWireframe( false )
	, m_bIgnoreVertexColor( false )
	, m_bBuffersFrames( false )
	, m_bSceneFade( true )
	, m_bVSync( true )
	, m_bFlip( true )
	, m_bFastFPU( true )
	, m_ambience( 0xFFFFFFFF )
	, m_frameRate( 0.0f )
	, m_ClearColor( 0 )
	, m_Gamma( 1.0f, 1.0f, 1.0f )
	, m_SystemGamma( 2.5f )
	, m_Fade( 0.0f )
	, m_TextureSwitches( 0 )
	, m_NumActiveTextures( 0 )
	, m_TrianglesDrawn( 0 )
	, m_VerticesTransformed( 0 )
	, m_TextureMemory( 0 )
	, m_TexSystemMemory( 0 )
	, m_TexManagerSystemMemory( 0 )
	, m_secondTime( 0.0 )
	, m_bAlphaTest( false )
	, m_srcBlend( D3DBLEND_ONE )
	, m_dstBlend( D3DBLEND_ZERO )
	, m_SurfaceExpirationTime( 5.0f )
	, m_sysDetail( 10 )
	, m_fogType( RAPI_FOGTYPE_NONE )
	, m_bFogIsActive( false )
	, m_fogColor( 0xFF000000 )
	, m_fogDensity( 1.0f )
	, m_clipNearDist( 0.1f )	
	, m_clipFarDist( 75.0f )
	, m_fogNearDist( 0.1f )
	, m_fogFarDist( 1.0f )	
	, m_clipToFogRatio( 1.0f )
	, m_bUseWBuffer( false )
	, m_EnvironmentTexture( 0 )
	, m_bActiveTextureLog( false )
	, m_Saturation( 1.0f )
	, m_FrameCount( 0 )
	, m_primaryPixelFormat( (ePixelFormat)0 )
	, m_primaryD3DFormat( (D3DFORMAT)0 )
	, m_depthD3DFormat( (D3DFORMAT)0 )
	, m_stencil( 0 )
	, m_opaqueTexPixelFormat( (ePixelFormat)0 )
	, m_opaqueTexD3DFormat( (D3DFORMAT)0 )
	, m_alphaTexPixelFormat( (ePixelFormat)0 )
	, m_alphaTexD3DFormat( (D3DFORMAT)0 )
	, m_bOnGDIBuffer( true )
	, m_bSoftwareTransform( true )
	, m_bTripleBuffering( false )
	, m_bAlphaBlendEnabled( true )
	, m_bNoTextures( false )
	, m_bNoRender( false )
	, m_bAsyncCursor( false )
	, m_bEdit( false )
	, m_bSimdSupport( false )
{
	memset( m_ActiveTex, 0, sizeof( RapiTexture* ) * NUM_TEXTURES );
	memset( m_Lights, 0, sizeof( RapiLight* ) * LIGHT_CACHE );

	m_MatrixStack.reserve( MATRIXSTACKSIZE );

	m_Textures.SetIsZeroIndexValid( true );
	m_Surfaces.SetIsZeroIndexValid( true );

	m_Textures.Dereference( 0 ) = NULL;
	m_Surfaces.Dereference( 0 )	= NULL;

	if( RapiMouse::DoesSingletonExist() )
	{
		m_pRapiMouse	= NULL;
	}
	else
	{
		m_pRapiMouse	= new RapiMouse( this );
	}

	m_primaryThreadId	= ::GetCurrentThreadId();

	// Detect SIMD support
	m_bSimdSupport		= SysInfo::IsSIMDSupported();
}

// Destructor
Rapi::~Rapi()
{
	// SPECIAL NOTE ABOUT THIS DESTRUCTOR
	// Yes, there is a reason that I am not using an iterative method
	// to destroy these lists.  The destructors for fonts and lights
	// automatically remove themselves from the given lists, so
	// if we used an iterative method it would constantly invalidate our
	// iterator.  What follows seems to me to be the best solution to this
	// problem, although if anyone has any better ideas let me know.

	// Make sure that all lights are cleared out
	for( unsigned int i = 0; i < LIGHT_CACHE; ++i ){
		if( m_Lights[ i ] ){
			delete m_Lights[ i ];
			m_Lights[ i ] = NULL;
		}
	}

	// Make sure that all fonts are cleared out
	while( !m_Fonts.empty() )
	{
		delete (*m_Fonts.begin());
	}

	// Release any textures from their holds in the active texture
	for( i = 0; i < NUM_TEXTURES; ++i )
	{
		if( m_pD3DDevice )
		{
			TRYDX( m_pD3DDevice->SetTexture( i, NULL ) );
		}
	}

	// Make sure that all textures are cleared out
	// This should be the last step before releasing DirectX
	for( TextureBucketVector::iterator t = m_Textures.begin(); t != m_Textures.end(); ++t )
	{
		gpassertf( (*t)->GetReferenceCount() == 0, ("Texture [%s] is still referenced [%d] times!", (*t)->GetPathname().c_str(), (*t)->GetReferenceCount() ) );

		delete (*t);
		(*t) = NULL;
	}
	m_Textures.clear();

	for( SurfaceExpirationMap::iterator s = m_SurfaceExpirationMap.begin(); s != m_SurfaceExpirationMap.end(); ++s )
	{
		RapiSurface* pSurface	= m_Surfaces[ (*s).first ];

		// Subtract this tex from our system memory counter
		if( pSurface->GetTextureBits() != NULL )
		{
			m_TexSystemMemory -= pSurface->GetTextureBits()->GetSizeInBytes();
		}

		// Remove this texture from the mapping
		m_SurfaceMap.erase( pSurface->GetPathname() );

		// Delete the texture
		delete pSurface;
		m_Surfaces.Free( (*s).first, NULL );
	}
	m_SurfaceExpirationMap.clear();

#if !GP_RETAIL

	gpassert( m_Surfaces.empty() );

#endif

	// Do the full release
	DestroyComponents();

	// Destroy the mouse
	if( m_pRapiMouse )
	{
		delete m_pRapiMouse;
		m_pRapiMouse	= NULL;
	}
}

// Destroy components of DirectX
void Rapi::DestroyComponents()
{
	if( m_pRapiMouse )
	{
		m_pRapiMouse->Disable();
	}

	if( m_pBackBuffer )
	{
		RELEASE( m_pBackBuffer );
	}

	if( m_pZBuffer )
	{
		RELEASE( m_pZBuffer );
	}

	if( m_pVBuffer )
	{
		RELEASE( m_pVBuffer );
	}

	if( m_pLVBuffer )
	{
		RELEASE( m_pLVBuffer );
	}

	if( m_pTVBuffer )
	{
		RELEASE( m_pTVBuffer );
	}

	if( m_pIndexBuffer )
	{
		RELEASE( m_pIndexBuffer );
	}

	if( m_pD3DDevice )
	{
		RELEASE( m_pD3DDevice );
	}
}

// Get D3D object
LPDIRECT3D8 Rapi::GetObject()
{
	return m_pD3D;
}

// Initializes needed members
HRESULT Rapi::Init( AdapterDesc &ddobj, HWND hwnd )
{
	// Make sure we are clean
	DestroyComponents();

	// Initialize the pointer to my owner window
    m_hwnd			= hwnd;

	// Create new Direct3D8 interface
	m_pD3D = gRapiOwner.GetDirect3D();
	if( !m_pD3D )
	{
		// Failure?
	}

	// Copy our device description
	m_deviceDesc	= ddobj;

	HRESULT hResult;

 	// Get this card's identifying information
	if( FAILED( hResult	= m_pD3D->GetAdapterIdentifier( m_deviceDesc.adapterNum, D3DENUM_NO_WHQL_LEVEL, &m_deviceId ) ) )
	{
		gperrorboxf(( $MSG$ "Video initialization failure!\n\nFunction failure: %s\nDirectX error code: 0x%08X\n%s\n",
					  "GetAdapterIdentifier", hResult, MakeDxErrorMessage( hResult ).c_str() ));
		return E_FAIL;
	}

	if( m_deviceId.DriverVersion.QuadPart == 0 )
	{
		// No driver version was specified, so we need to get the product version from the driver
		DWORD dwHandle	= 0;
		DWORD infoSize	= GetFileVersionInfoSize( m_deviceId.Driver, &dwHandle );
		if( infoSize )
		{
			BYTE* pVersionInfo	= new BYTE[ infoSize ];
			if( GetFileVersionInfo( m_deviceId.Driver, dwHandle, infoSize, (void*)pVersionInfo ) )
			{
				// Query for the product version
				VS_FIXEDFILEINFO* pFileInfo	= NULL;
				unsigned int length			= 0;
				if( VerQueryValue( (void*)pVersionInfo, TEXT("\\"), (void**)&pFileInfo, &length ) )
				{
					m_deviceId.DriverVersion.LowPart	= pFileInfo->dwProductVersionLS;
					m_deviceId.DriverVersion.HighPart	= pFileInfo->dwProductVersionMS;
				}
			}
			delete[] pVersionInfo;
		}
	}

	// Clear display mode listing
	m_DisplayList.clear();

	// $$  Enumerate display modes


	return S_OK;
}

// Enables this device
bool Rapi::Enable( bool bFullScreen, int fsScreenWidth, int fsScreenHeight, int fsBPP, bool bVsync )
{
	HRESULT			hr;
	m_bFullScreen	= bFullScreen;
	m_bVSync		= bVsync;
	m_BPP			= fsBPP;

	// Init window rect stuff
	m_Width			= fsScreenWidth;
	m_Height		= fsScreenHeight;

	// Find a good display mode
	bool bModeFound	= false;
	for( unsigned int i = 0; i < m_deviceDesc.numDisplayModes; ++i )
	{
		D3DDISPLAYMODE& displayMode	= m_deviceDesc.displayModes[i];
		if( displayMode.Width	== m_Width  &&
			displayMode.Height	== m_Height &&
			GetSizeBytes( GetPixelFormat( displayMode.Format ) ) == (int)(m_BPP/8) &&
			SUCCEEDED( m_pD3D->CheckDeviceType( m_deviceDesc.adapterNum, 
												D3DDEVTYPE_HAL, 
												GetDisplayFormat( displayMode.Format ), 
												displayMode.Format, 
												!bFullScreen ) ) )
		{
			// Indicate that we have found a valid display format, in our bit depth and
			// using a HAL device and windowed settings.
			bModeFound	= true;

			// Okay, so we have a match, but there may be multiples to we need to find the one
			// that has the best refresh
			if( m_deviceDesc.activeMode.Width			 != displayMode.Width  ||
				m_deviceDesc.activeMode.Height			 != displayMode.Height )
			{
				// Width and height match (only hits this branch on first valid loop)
				m_deviceDesc.activeMode	= displayMode;
			}
			else if( m_deviceDesc.activeMode.Format		 == displayMode.Format &&
					 m_deviceDesc.activeMode.RefreshRate <  displayMode.RefreshRate )
			{
				// Format matches, and refresh rate is better
				m_deviceDesc.activeMode	= displayMode;
			}
			else
			{
				// There are 4 basic render target formats that can be used, and of the 4 there
				// is a preferable one for each bit depth.  This code makes sure we are using the
				// one we prefer.
				if( m_BPP								== 32			   &&
					m_deviceDesc.activeMode.Format		== D3DFMT_A8R8G8B8 &&
					displayMode.Format					== D3DFMT_X8R8G8B8 )
				{
					m_deviceDesc.activeMode				= displayMode;
				}
				else if( m_BPP							== 16			   &&
						 m_deviceDesc.activeMode.Format	== D3DFMT_X1R5G5B5 &&
						 displayMode.Format				== D3DFMT_R5G6B5 )
				{
					m_deviceDesc.activeMode				= displayMode;
				}
			}
		}
	}

	if( !bModeFound )
	{
		gperrorboxf(( $MSG$ "Video initialization failure!\n\nNo valid display modes are available.  This could mean "
							"that your video card does not support hardware rendering, or does not support the windowed "
							"method that you have chosen.  It could also mean that you have chosen a bit-depth that is "
							"unsupported by your video hardware.\n" ));
		return false;
	}

	// Get the pixel format
	m_primaryD3DFormat		= m_deviceDesc.activeMode.Format;
	m_primaryPixelFormat	= GetPixelFormat( m_primaryD3DFormat );

	// Find a good depth buffer format
	for( i = 0; i < ELEMENT_COUNT( PreferredDepthFormats ); ++i )
	{
		// Check the preferred format with the device
		if( SUCCEEDED( m_pD3D->CheckDeviceFormat( m_deviceDesc.adapterNum,
												  D3DDEVTYPE_HAL,
												  GetDisplayFormat( m_primaryD3DFormat ),
												  D3DUSAGE_DEPTHSTENCIL,
												  D3DRTYPE_SURFACE,
												  PreferredDepthFormats[ i ] ) ) &&
			// Verify that the depth format is compatible
			SUCCEEDED( m_pD3D->CheckDepthStencilMatch( m_deviceDesc.adapterNum,
													   D3DDEVTYPE_HAL,
													   GetDisplayFormat( m_primaryD3DFormat ),
													   m_primaryD3DFormat,
													   PreferredDepthFormats[ i ] ) ) )
		{
			// Save the format
			m_depthD3DFormat		= PreferredDepthFormats[ i ];

			switch( m_depthD3DFormat )
			{
				case D3DFMT_D24S8:
				{
					m_stencil		= 8;
					break;
				}

				case D3DFMT_D24X4S4:
				{
					m_stencil		= 4;
					break;
				}

				case D3DFMT_D15S1:
				{
					m_stencil		= 1;
					break;
				}

				default:
				{
					m_stencil		= 0;
					break;
				}
			};
		}
	}

	// Find a good opaque texture format
	for( i = 0; i < ELEMENT_COUNT( PreferredOpaqueTexFormats ); ++i )
	{
		// Check the preferred format with the device
		if( SUCCEEDED( m_pD3D->CheckDeviceFormat( m_deviceDesc.adapterNum,
												  D3DDEVTYPE_HAL,
												  GetDisplayFormat( m_primaryD3DFormat ),
												  0,
												  D3DRTYPE_TEXTURE,
												  PreferredOpaqueTexFormats[ i ] ) ) )
		{
			// Save the format
			m_opaqueTexD3DFormat	= PreferredOpaqueTexFormats[ i ];
			m_opaqueTexPixelFormat	= GetPixelFormat( m_opaqueTexD3DFormat );
			break;
		}
	}

	// Find a good alpha texture format
	for( i = 0; i < ELEMENT_COUNT( PreferredAlphaTexFormats ); ++i )
	{
		// Check the preferred format with the device
		if( SUCCEEDED( m_pD3D->CheckDeviceFormat( m_deviceDesc.adapterNum,
												  D3DDEVTYPE_HAL,
												  GetDisplayFormat( m_primaryD3DFormat ),
												  0,
												  D3DRTYPE_TEXTURE,
												  PreferredAlphaTexFormats[ i ] ) ) )
		{
			// Save the format
			m_alphaTexD3DFormat		= PreferredAlphaTexFormats[ i ];
			m_alphaTexPixelFormat	= GetPixelFormat( m_alphaTexD3DFormat );
			break;
		}
	}

	// $$$$$
	// Right now textures are not all marked properly with regards to the alpha flag.  This means that
	// many of the textures that looked right in the DX7 work don't look right now because there is
	// no alpha in the texture format.  To fix this, I am using the alpha texture format as the opaque
	// texture format until we can go back through and fix this properly.
	m_opaqueTexD3DFormat		= m_alphaTexD3DFormat;
	m_opaqueTexPixelFormat		= m_alphaTexPixelFormat;

	// Setup our original vertex and index buffer flags
	m_bufferFlags				= D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
	m_bufferMem					= D3DPOOL_DEFAULT;

	// See if we need to run in software processing mode due to lack of hardware transform
	D3DCAPS8 caps;
	m_pD3D->GetDeviceCaps( m_deviceDesc.adapterNum, D3DDEVTYPE_HAL, &caps );
	if( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT )
	{
		m_bSoftwareTransform	= false;
	}
	else
	{
		m_bufferFlags			|= D3DUSAGE_SOFTWAREPROCESSING;
		m_bufferMem				= D3DPOOL_SYSTEMMEM;
	}

	// Fill in the creation parameters with the information about our chosen format and other settings
	ZeroMemory( &m_deviceParams, sizeof( D3DPRESENT_PARAMETERS ) );
	m_deviceParams.BackBufferWidth					= m_deviceDesc.activeMode.Width;
	m_deviceParams.BackBufferHeight					= m_deviceDesc.activeMode.Height;
	m_deviceParams.BackBufferFormat					= m_deviceDesc.activeMode.Format;
	m_deviceParams.BackBufferCount					= 1;
	m_deviceParams.MultiSampleType					= D3DMULTISAMPLE_NONE;
	m_deviceParams.SwapEffect						= (bFullScreen && m_bFlip) ? D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_COPY;
	m_deviceParams.hDeviceWindow					= m_hwnd;
	m_deviceParams.Windowed							= !bFullScreen;
	m_deviceParams.EnableAutoDepthStencil			= TRUE;
	m_deviceParams.AutoDepthStencilFormat			= m_depthD3DFormat;
	m_deviceParams.Flags							= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	m_deviceParams.FullScreen_RefreshRateInHz		= D3DPRESENT_RATE_DEFAULT;
	m_deviceParams.FullScreen_PresentationInterval	= D3DPRESENT_INTERVAL_DEFAULT;
	
	// Create the device
	TRYDX( m_pD3D->CreateDevice( m_deviceDesc.adapterNum, D3DDEVTYPE_HAL, m_hwnd,
								 m_bSoftwareTransform ? D3DCREATE_SOFTWARE_VERTEXPROCESSING : D3DCREATE_HARDWARE_VERTEXPROCESSING,
								 &m_deviceParams, &m_pD3DDevice ) );

	// Indicate software vertex processing
	if( m_bSoftwareTransform )
	{
		// This call may fail, so not TryDx.  It is okay if it fails.
		m_pD3DDevice->SetRenderState( D3DRS_SOFTWAREVERTEXPROCESSING, TRUE );
	}

	// Grab a pointer to the back buffer through the primary
	if( FAILED( hr = m_pD3DDevice->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer ) ) )
	{
		gperrorboxf(( $MSG$ "Video initialization failure!\n\nFunction failure: %s\nDirectX error code: 0x%08X\n%s\n",
					  "GetBackBuffer", hr, MakeDxErrorMessage( hr ).c_str() ));
		return false;
	}

	// Grab a pointer to the zbuffer through the primary
	if( FAILED( hr = m_pD3DDevice->GetDepthStencilSurface( &m_pZBuffer ) ) )
	{
		gperrorboxf(( $MSG$ "Video initialization failure!\n\nFunction failure: %s\nDirectX error code: 0x%08X\n%s\n",
					  "GetDepthStencilSurface", hr, MakeDxErrorMessage( hr ).c_str() ));
		return false;
	}

	// Get texture memory
	m_vidMem = m_pD3DDevice->GetAvailableTextureMem();

	// Get the capabilities of this D3D device so we can check them for support
	// of various options as we cruise through the rest of the setup
	ZeroMemory( &m_d3dDevDesc, sizeof( D3DCAPS8 ) );
	TRYDX( m_pD3DDevice->GetDeviceCaps( &m_d3dDevDesc ) );

	// Finally, set the viewport for the newly created device
	if( !SetViewport( 1.0f, 1.0f ) )
	{
		gperrorbox( $MSG$ "D3D Initialization Failure: Dungeon Siege could not setup the viewport." );
		return false;
	}

	if( (m_d3dDevDesc.RasterCaps & D3DPRASTERCAPS_FOGVERTEX) )
	{
		m_fogType					= RAPI_FOGTYPE_VERTEX;
	}
	else if( (m_d3dDevDesc.RasterCaps & D3DPRASTERCAPS_WFOG) )
	{
		m_fogType					= RAPI_FOGTYPE_WFOG;
	}
	else if( (m_d3dDevDesc.RasterCaps & D3DPRASTERCAPS_ZFOG) )
	{
		m_fogType					= RAPI_FOGTYPE_ZFOG;
	}
	else
	{
		gpwarning( "This video card supports no valid fog modes, fog will be disabled" );
	}

	if( !m_bSoftwareTransform &&
	   ((m_d3dDevDesc.TextureCaps & D3DPTEXTURECAPS_PROJECTED)	&&
		(m_d3dDevDesc.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)))
	{
		m_bManualTextureProjection	= false;
	}
	else
	{
		m_bufferFlags				|= D3DUSAGE_SOFTWAREPROCESSING;
		m_bufferMem					= D3DPOOL_SYSTEMMEM;
	}

	if( m_d3dDevDesc.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY )
	{
		m_bOnlySquareTextures		= true;
	}

	if( m_d3dDevDesc.TextureOpCaps & D3DTEXOPCAPS_ADD )
	{
		m_bNoAdd					= false;
	}
	else
	{
		m_bNoAdd					= true;
	}

	// Check for a good guard band
	if( ((fabsf( m_d3dDevDesc.GuardBandLeft  ) < (float)m_Width) || (fabsf( m_d3dDevDesc.GuardBandTop    ) < (float)m_Height)) ||
		((fabsf( m_d3dDevDesc.GuardBandRight ) < (float)m_Width) || (fabsf( m_d3dDevDesc.GuardBandBottom ) < (float)m_Height)) )
	{
		// Need manual frustum clipping
		m_bManualFrustumClipping	= true;
	}

	if( FAILED( hr = m_pD3DDevice->CreateVertexBuffer( sizeof( sVertex ) * SIZE_VBUFFER,
													   m_bufferFlags, SVERTEX, m_bufferMem, &m_pVBuffer ) ) )
	{
		gperrorboxf(( $MSG$ "Video initialization failure!\n\nFunction failure: %s\nDirectX error code: 0x%08X\n%s\n",
					  "CreateVertexBuffer - VBuffer", hr, MakeDxErrorMessage( hr ).c_str() ));
		return false;
	}

	if( FAILED( hr = m_pD3DDevice->CreateVertexBuffer( sizeof( tVertex ) * SIZE_TVBUFFER,
													   m_bManualTextureProjection ? (m_bufferFlags & ~D3DUSAGE_WRITEONLY) : m_bufferFlags,
													   TVERTEX, m_bufferMem, &m_pTVBuffer ) ) )
	{
		gperrorboxf(( $MSG$ "Video initialization failure!\n\nFunction failure: %s\nDirectX error code: 0x%08X\n%s\n",
					  "CreateVertexBuffer - TVBuffer", hr, MakeDxErrorMessage( hr ).c_str() ));
		return false;
	}

	if( FAILED( hr = m_pD3DDevice->CreateVertexBuffer( sizeof( lVertex ) * SIZE_VBUFFER,
													   m_bufferFlags, LVERTEX, m_bufferMem, &m_pLVBuffer ) ) )
	{
		gperrorboxf(( $MSG$ "Video initialization failure!\n\nFunction failure: %s\nDirectX error code: 0x%08X\n%s\n",
					  "CreateVertexBuffer - LVBuffer", hr, MakeDxErrorMessage( hr ).c_str() ));
		return false;
	}

	if( FAILED( hr = m_pD3DDevice->CreateIndexBuffer( SIZE_VBUFFER * 2,
													  m_bufferFlags, D3DFMT_INDEX16, m_bufferMem, &m_pIndexBuffer ) ) )
	{
		gperrorboxf(( $MSG$ "Video initialization failure!\n\nFunction failure: %s\nDirectX error code: 0x%08X\n%s\n",
					  "CreateIndexBuffer", hr, MakeDxErrorMessage( hr ).c_str() ));
		return false;
	}

	// Setup a reasonable set of default settings
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZENABLE,				D3DZB_TRUE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_FILLMODE,			D3DFILL_SOLID ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_SHADEMODE,			D3DSHADE_GOURAUD ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		TRUE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE,		FALSE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND,			D3DBLEND_ONE )); 
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND,			D3DBLEND_ZERO )); 
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_CULLMODE,			D3DCULL_CW ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZFUNC,				D3DCMP_LESSEQUAL ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ALPHAREF,			0 ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ALPHAFUNC,			D3DCMP_ALWAYS ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_DITHERENABLE,		m_BPP < 32 ? TRUE : FALSE )); 
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE,	FALSE )); 
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE,			FALSE )); 
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_SPECULARENABLE,		FALSE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_EDGEANTIALIAS,		FALSE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZBIAS,				0 ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_STENCILENABLE,		FALSE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_TEXTUREFACTOR,		0 )); 
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_WRAP0,				0 ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_WRAP1,				0 ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_WRAP2,				0 ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_WRAP3,				0 ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_LIGHTING,			FALSE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_COLORVERTEX,			FALSE )); 
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_LOCALVIEWER,			TRUE ));  
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_NORMALIZENORMALS,	FALSE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_VERTEXBLEND,			D3DVBF_DISABLE ));

	// Figure out a good default mip filter
	if( m_d3dDevDesc.TextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR )
	{
		m_bTrilinearCapable	= true;
	}
	else
	{
		// Set to point because there is no other option
		m_mipFilter			= D3DTEXF_POINT;
		m_bTrilinearCapable	= false;
	}

	// Go through the stages that the hardware says it can handle and setup some defaults
	for( i = 0; i < m_d3dDevDesc.MaxTextureBlendStages; ++i )
	{
		TRYDX (m_pD3DDevice->SetTextureStageState( i, D3DTSS_MAGFILTER,	D3DTEXF_LINEAR ));
		TRYDX (m_pD3DDevice->SetTextureStageState( i, D3DTSS_MINFILTER,	D3DTEXF_LINEAR ));
		TRYDX (m_pD3DDevice->SetTextureStageState( i, D3DTSS_MIPFILTER,	m_mipFilter ));
	}

	// Clear active textures
	memset( m_ActiveTex, 0, sizeof( RapiTexture* ) * NUM_TEXTURES );

	for( i = 0; i < NUM_TEXTURES; ++i )
	{
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_COLOROP,	(DWORD*)&m_TextureStageStates[i].colorOp	) );
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_ALPHAOP,	(DWORD*)&m_TextureStageStates[i].alphaOp	) );

		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_COLORARG1, (DWORD*)&m_TextureStageStates[i].colorArg1	) );
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_COLORARG2, (DWORD*)&m_TextureStageStates[i].colorArg2	) );
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_ALPHAARG1, (DWORD*)&m_TextureStageStates[i].alphaArg1	) );
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_ALPHAARG2, (DWORD*)&m_TextureStageStates[i].alphaArg2	) );

		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_ADDRESSU,	(DWORD*)&m_TextureStageStates[i].addressU	) );
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_ADDRESSV,	(DWORD*)&m_TextureStageStates[i].addressV	) );

		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_MAGFILTER,	(DWORD*)&m_TextureStageStates[i].magFilter	) );
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_MINFILTER,	(DWORD*)&m_TextureStageStates[i].minFilter	) );
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_MIPFILTER,	(DWORD*)&m_TextureStageStates[i].mipFilter	) );
	}

	m_bAlphaTest = false;
	TRYDX( m_pD3DDevice->GetRenderState( D3DRS_SRCBLEND,		(DWORD*)&m_srcBlend ) );
	TRYDX( m_pD3DDevice->GetRenderState( D3DRS_DESTBLEND,		(DWORD*)&m_dstBlend ) );

	DWORD alphaTest;
	TRYDX( m_pD3DDevice->GetRenderState( D3DRS_ALPHATESTENABLE, (DWORD*)&alphaTest ) );
	m_bAlphaTest = alphaTest == TRUE ? true : false;

	// Set the current sys id
	SetSystemDetailIdentifier();

	// Set the proper fog information
	SetFogModeFormula( D3DFOG_LINEAR );

	SetFogDensity( m_fogDensity );
	SetFogNearDist( m_fogNearDist );
	SetFogFarDist( m_fogFarDist );
	SetFogColor( m_fogColor );

	// Reset the gamma table
	BuildGammaTable();

	// Create RapiMouse
	if( m_pRapiMouse )
	{
		m_pRapiMouse->Enable();
	}

	return true;
}

// Disables this device
void Rapi::Disable( )
{
	// Release any textures from their holds in the active texture
	for( unsigned int i = 0; i < NUM_TEXTURES; ++i )
	{
		TRYDX (m_pD3DDevice->SetTexture( i, NULL ));
	}

	// Destroy everything
	DestroyComponents();
}

// Resets the device with the set parameters
bool Rapi::ResetDevice()
{
	// Release all of the vertex buffers, index buffer, and render targets
	{
		// Lock the surface collection critical
		kerneltool::Critical::Lock autoLock( m_SurfaceCritical );
		kerneltool::RwCritical::ReadLock vectLock( m_Surfaces.GetCritical() );

		// Go through surfaces
		for( SurfaceBucketVector::iterator i = m_Surfaces.begin(); i != m_Surfaces.end(); ++i )
		{
			if( (*i)->GetRenderTarget() )
			{
				(*i)->Destroy();
			}
		}
	}

	// Reset Any Fonts
	FontList::iterator iFont;
	for ( iFont = m_Fonts.begin(); iFont != m_Fonts.end(); ++iFont )
	{
		(*iFont)->ResetSurfaces();
	}

	// Release buffers
	RELEASE( m_pBackBuffer );
	RELEASE( m_pZBuffer );
	RELEASE( m_pVBuffer );
	RELEASE( m_pLVBuffer );
	RELEASE( m_pTVBuffer );
	RELEASE( m_pIndexBuffer );

	// Reset the device
	TRYDX( m_pD3DDevice->Reset( &m_deviceParams ) );

	// Grab a pointer to the back buffer through the primary
	HRESULT hr;
	if( FAILED( hr = m_pD3DDevice->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer ) ) )
	{
		gperrorboxf(( $MSG$ "Video initialization failure!\n\nFunction failure: %s\nDirectX error code: 0x%08X\n%s\n",
					  "GetBackBuffer", hr, MakeDxErrorMessage( hr ).c_str() ));
		return false;
	}

	// Grab a pointer to the zbuffer through the primary
	if( FAILED( hr = m_pD3DDevice->GetDepthStencilSurface( &m_pZBuffer ) ) )
	{
		gperrorboxf(( $MSG$ "Video initialization failure!\n\nFunction failure: %s\nDirectX error code: 0x%08X\n%s\n",
					  "GetDepthStencilSurface", hr, MakeDxErrorMessage( hr ).c_str() ));
		return false;
	}

	// Fonts and render targets will create themselves again when needed.  For now we just
	// need to recreate our vertex and index buffers
	if( FAILED( m_pD3DDevice->CreateVertexBuffer( sizeof( sVertex ) * SIZE_VBUFFER,
												  m_bufferFlags, SVERTEX, m_bufferMem, &m_pVBuffer ) ) )
	{
		gperror( "Could not create main vertex buffer in Rapi::RestoreSurfaces()" );
		return false;
	}
	m_VBufferPosition	= 0;

	if( FAILED( m_pD3DDevice->CreateVertexBuffer( sizeof( lVertex ) * SIZE_VBUFFER,
												  m_bufferFlags, LVERTEX, m_bufferMem, &m_pLVBuffer ) ) )
	{
		gperror( "Could not create lit vertex buffer in Rapi::RestoreSurfaces()" );
		return false;
	}
	m_LVBufferPosition	= 0;

	if( FAILED( m_pD3DDevice->CreateVertexBuffer( sizeof( tVertex ) * SIZE_TVBUFFER,
												  m_bManualTextureProjection ? (m_bufferFlags & ~D3DUSAGE_WRITEONLY) : m_bufferFlags,
												  TVERTEX, m_bufferMem, &m_pTVBuffer ) ) )
	{
		gpdebugger( "Could not create transformed vertex buffer in Rapi::Enable()\n" );
		return false;
	}
	m_TVBufferPosition	= 0;

	if( FAILED( m_pD3DDevice->CreateIndexBuffer( SIZE_VBUFFER * 2,
												 m_bufferFlags, D3DFMT_INDEX16, m_bufferMem, &m_pIndexBuffer ) ) )
	{
		gpdebugger( "Could not create index buffer in Rapi::Enable()\n" );
		return false;
	}
	m_IndexBufferPosition	= 0;

	// Setup a reasonable set of default settings
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZENABLE,				D3DZB_TRUE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_FILLMODE,			D3DFILL_SOLID ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_SHADEMODE,			D3DSHADE_GOURAUD ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		TRUE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE,		FALSE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND,			D3DBLEND_ONE )); 
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND,			D3DBLEND_ZERO )); 
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_CULLMODE,			D3DCULL_CW ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZFUNC,				D3DCMP_LESSEQUAL ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ALPHAREF,			0 ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ALPHAFUNC,			D3DCMP_ALWAYS ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_DITHERENABLE,		m_BPP < 32 ? TRUE : FALSE )); 
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE,	FALSE )); 
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE,			FALSE )); 
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_SPECULARENABLE,		FALSE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_EDGEANTIALIAS,		FALSE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZBIAS,				0 ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_STENCILENABLE,		FALSE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_TEXTUREFACTOR,		0 )); 
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_WRAP0,				0 ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_WRAP1,				0 ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_WRAP2,				0 ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_WRAP3,				0 ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_LIGHTING,			FALSE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_COLORVERTEX,			FALSE )); 
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_LOCALVIEWER,			TRUE ));  
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_NORMALIZENORMALS,	FALSE ));
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_VERTEXBLEND,			D3DVBF_DISABLE ));

	// Go through the stages that the hardware says it can handle and setup some defaults
	for( unsigned int i = 0; i < m_d3dDevDesc.MaxTextureBlendStages; ++i )
	{
		TRYDX (m_pD3DDevice->SetTextureStageState( i, D3DTSS_MAGFILTER,	D3DTEXF_LINEAR ));
		TRYDX (m_pD3DDevice->SetTextureStageState( i, D3DTSS_MINFILTER,	D3DTEXF_LINEAR ));
		TRYDX (m_pD3DDevice->SetTextureStageState( i, D3DTSS_MIPFILTER,	m_mipFilter ));
	}

	// Clear active textures
	memset( m_ActiveTex, 0, sizeof( RapiTexture* ) * NUM_TEXTURES );

	for( i = 0; i < NUM_TEXTURES; ++i )
	{
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_COLOROP,	(DWORD*)&m_TextureStageStates[i].colorOp	) );
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_ALPHAOP,	(DWORD*)&m_TextureStageStates[i].alphaOp	) );

		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_COLORARG1, (DWORD*)&m_TextureStageStates[i].colorArg1	) );
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_COLORARG2, (DWORD*)&m_TextureStageStates[i].colorArg2	) );
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_ALPHAARG1, (DWORD*)&m_TextureStageStates[i].alphaArg1	) );
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_ALPHAARG2, (DWORD*)&m_TextureStageStates[i].alphaArg2	) );

		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_ADDRESSU,	(DWORD*)&m_TextureStageStates[i].addressU	) );
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_ADDRESSV,	(DWORD*)&m_TextureStageStates[i].addressV	) );

		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_MAGFILTER,	(DWORD*)&m_TextureStageStates[i].magFilter	) );
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_MINFILTER,	(DWORD*)&m_TextureStageStates[i].minFilter	) );
		TRYDX( m_pD3DDevice->GetTextureStageState( i, D3DTSS_MIPFILTER,	(DWORD*)&m_TextureStageStates[i].mipFilter	) );
	}

	m_bAlphaTest = false;
	TRYDX( m_pD3DDevice->GetRenderState( D3DRS_SRCBLEND,		(DWORD*)&m_srcBlend ) );
	TRYDX( m_pD3DDevice->GetRenderState( D3DRS_DESTBLEND,		(DWORD*)&m_dstBlend ) );

	DWORD alphaTest;
	TRYDX( m_pD3DDevice->GetRenderState( D3DRS_ALPHATESTENABLE, (DWORD*)&alphaTest ) );
	m_bAlphaTest = alphaTest == TRUE ? true : false;

	// Set the proper fog information
	SetFogModeFormula( D3DFOG_LINEAR );

	SetFogDensity( m_fogDensity );
	SetFogNearDist( m_fogNearDist );
	SetFogFarDist( m_fogFarDist );
	SetFogColor( m_fogColor );

	// Reset the gamma table
	BuildGammaTable();

	return true;
}

// Pauses drawing for this device
void Rapi::Pause( bool bPause )
{
	gRapiMouse.PauseMouse( bPause );

	if( bPause )
	{
		m_bPaused = true;

		if( m_bFullScreen )
		{
			// Synchronize calls to Pause()
			kerneltool::Critical::Lock locker( m_pauseCritical );
			if( !m_bOnGDIBuffer )
			{
				m_pD3DDevice->Present( NULL, NULL, NULL, NULL );
				m_bOnGDIBuffer	= !m_bOnGDIBuffer;
			}
		}
	}
	else
	{
		m_bPaused = false;
	}
}

// Invalidates all textures that are currently created.  Used when windowed apps resize.
void Rapi::InvalidateSurfaces()
{
	// Release any textures from their holds in the active texture
	for( unsigned int i = 0; i < NUM_TEXTURES; ++i )
	{
		TRYDX( m_pD3DDevice->SetTexture( i, NULL ) );
		m_ActiveTex[i]	= NULL;
	}

	// Lock the surface collection critical
	kerneltool::Critical::Lock autoLock( m_SurfaceCritical );
	kerneltool::RwCritical::ReadLock vectLock( m_Surfaces.GetCritical() );

	for( SurfaceBucketVector::iterator s = m_Surfaces.begin(); s != m_Surfaces.end(); ++s )
	{
		(*s)->InvalidateSurface();
	}
}

// Restore all of the textures that were available before the InvalidateTextures call
void Rapi::RestoreSurfacesAndLights()
{
	{
		// Lock the surface collection critical
		kerneltool::Critical::Lock autoLock( m_SurfaceCritical );
		kerneltool::RwCritical::ReadLock vectLock( m_Surfaces.GetCritical() );

		// Restore all previously available textures
		for( SurfaceBucketVector::iterator s = m_Surfaces.begin(); s != m_Surfaces.end(); ++s )
		{
			// Setup this texture surface again
			(*s)->RestoreSurface();
		}
	}

	// Restore all previously available lights
	for( unsigned int i = 0; i < LIGHT_CACHE; ++i )
	{
		if( m_Lights[i] )
		{
			// Setup this light again
			TRYDX (m_pD3DDevice->SetLight( i, &m_Lights[i]->GetLight() ));

			// Re-enable any lights that were enabled before
			if( m_Lights[i]->IsEnabled() )
			{
				EnableLight( i, true );
			}
		}
	}

	// Reset Any Fonts
	FontList::iterator iFont;
	for ( iFont = m_Fonts.begin(); iFont != m_Fonts.end(); ++iFont )
	{
		(*iFont)->ResetSurfaces();
	}
}

// Begin a frame
bool Rapi::Begin3D( bool bClear )
{
	GPSTATS_GROUP( GROUP_RENDER_MISC );

	gpassertm( !m_bInScene, "Someone called Rapi::Begin3D() without calling Rapi::End3D()!" );

	// Get counter for frame rate calculation
	m_startTime	= ::GetSystemSeconds();

	// Copy the scene to primary if requested
	if( m_bFlipBltOnBegin )
	{
		if( !CopySceneToPrimary() )
		{
			// Restore surfaces if copy failed
			if( !RestoreMainSurfaces() )
			{
				return false;
			}
		}
	}
	else
	{
		// Make sure our surfaces are around
		if( !RestoreMainSurfaces() )
		{
			return false;
		}
	}

	// Check the swap effect
	// Note that this change is handled only once per frame, during the beginning of the scene.  This
	// is done to prevent thrashing, so only the last change takes effect and only if it is different
	// than the effect already in place.
	D3DSWAPEFFECT swapEffect		= (m_bFullScreen && m_bFlip) ? D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_COPY;
	if( swapEffect != m_deviceParams.SwapEffect )
	{
		// Set the new swap effect
		m_deviceParams.SwapEffect	= swapEffect;

		// Reset the device
		ResetDevice();
	}

	// Begin the scene
	if( FAILED( m_pD3DDevice->BeginScene() ) )
	{
		return false;
	}

	m_bInScene = true;

	if( bClear )
	{
		// Clear the buffers
		DWORD dwFlags	= D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
		if( m_stencil )
		{
			dwFlags		|= D3DCLEAR_STENCIL;
		}

		D3DRECT rect;
		rect.x1	= 0;
		rect.y1 = 0;
		rect.x2	= m_Width;
		rect.y2 = m_Height;

		TRYDX (m_pD3DDevice->Clear( 1, &rect, dwFlags, m_ClearColor, 1.0f, 0 ));
	}

	// Reset the vertex buffer indices
	m_VBufferPosition		= SIZE_VBUFFER;
	m_LVBufferPosition		= SIZE_VBUFFER;
	m_TVBufferPosition		= SIZE_TVBUFFER;
	m_IndexBufferPosition	= SIZE_VBUFFER;

	// Reset frame information
	m_TextureSwitches		= 0;
	m_NumActiveTextures		= 0;
	m_TrianglesDrawn		= 0;
	m_VerticesTransformed	= 0;
	m_TextureMemory			= 0;

#if !GP_RETAIL

	{
		// Lock the surface collection critical
		kerneltool::Critical::Lock autoLock( m_SurfaceCritical );
		kerneltool::RwCritical::ReadLock vectLock( m_Surfaces.GetCritical() );

		// Accumulate active texture information
		unsigned int bytesPerPixel	= m_BPP / 8;
		for( SurfaceBucketVector::iterator s = m_Surfaces.begin(); s != m_Surfaces.end(); ++s )
		{
			if( (*s)->GetActive() )
			{
				// Get this texture's info and reset it
				m_TextureMemory +=	(*s)->GetWidth() * (*s)->GetHeight() * bytesPerPixel;
				m_NumActiveTextures++;
				(*s)->SetActive( false );
			}
		}
	}

#endif

	{
		// Lock the surface collection critical
		kerneltool::Critical::Lock autoLock( m_TextureCritical );
		kerneltool::RwCritical::ReadLock vectLock( m_Textures.GetCritical() );

		for( TextureBucketVector::iterator t = m_Textures.begin(); t != m_Textures.end(); ++t )
		{
			if( (*t)->GetActive() )
			{
				(*t)->SetActive( false );
			}
		}
	}

	return true;
}

// End a frame
bool Rapi::End3D( bool bCopyToPrimary, bool bCopyImmediately )
{
	GPSTATS_GROUP( GROUP_RENDER_MISC );

	gpassertm( m_bInScene, "Someone called Rapi::End3D() without calling Rapi::Begin3D()!" );

	if( (m_Fade != 0.0f) && m_bSceneFade )
	{
		// Draw the fade rect
		RP_DrawFilledRect( *this, 0, 0, m_Width, m_Height, MAKEDWORDCOLORA( vector_3::ZERO, m_Fade ) );

		// Always copy the scene immediately if we are fading
		bCopyImmediately = true;
	}

	// Indicate that we are outside of the frame
	m_bInScene			= false;

	// Synchronize calls to Pause()
	kerneltool::Critical::Lock locker( m_pauseCritical );

	// End the Scene
	if( FAILED( m_pD3DDevice->EndScene() ) )
	{
		return false;
	}

	// Increment frame counter
	++m_FrameCount;

	// Indicate that we should blt/flip on the next Begin3D()
	if( bCopyToPrimary && bCopyImmediately )
	{
		if( !CopySceneToPrimary() )
		{
			// Restore surfaces if copy failed
			RestoreMainSurfaces();
		}
		m_bFlipBltOnBegin	= false;
	}
	else
	{
		m_bFlipBltOnBegin	= bCopyToPrimary;

		// Make sure our surfaces are around
		RestoreMainSurfaces();
	}

	// Calculate frame rate
	float elapsedFrameTime	= (float)(::GetSystemSeconds() - m_startTime);
	m_frameRate = 1.0f / elapsedFrameTime;

	// Lock the surface collection critical
	kerneltool::Critical::Lock autoLock( m_SurfaceCritical );

	// Update surface expiration
	for( SurfaceExpirationMap::iterator i = m_SurfaceExpirationMap.begin(); i != m_SurfaceExpirationMap.end(); )
	{
		// Add elapsed time
		(*i).second	+= elapsedFrameTime;

		if( (*i).second > m_SurfaceExpirationTime )
		{
			RapiSurface* pSurface	= m_Surfaces[ (*i).first ];

			// Subtract this tex from our system memory counter
			if( pSurface->GetTextureBits() != NULL )
			{
				m_TexSystemMemory -= pSurface->GetTextureBits()->GetSizeInBytes();
			}

			// Remove this texture from the mapping
			m_SurfaceMap.erase( pSurface->GetPathname() );

			// Delete the texture
			delete pSurface;
			m_Surfaces.Free( (*i).first, NULL );

			// Remove from expiration list
			i = m_SurfaceExpirationMap.erase( i );
		}
		else
		{
			++i;
		}
	}

	{
		// Lock the surface collection critical
		kerneltool::Critical::Lock autoLock( m_SurfaceCritical );
		kerneltool::RwCritical::ReadLock vectLock( m_Surfaces.GetCritical() );

		for( SurfaceBucketVector::iterator s = m_Surfaces.begin(); s != m_Surfaces.end(); ++s )
		{
			if( (*s)->GetRenderTarget() && !(*s)->GetSurface() )
			{
				(*s)->RestoreSurface();
			}
		}
	}

	return true;
}

// Blt/Flip the current frame onto the primary
bool Rapi::CopySceneToPrimary()
{
	if( !m_bPaused && RestoreMainSurfaces() )
	{
		// Hold the mouse critical during the entire copy/mouse update process in order
		// to ensure that no secondary updates occur before the primary surface has a chance
		// to be updated
		kerneltool::Critical::Lock mouseLock( gRapiMouse.GetMouseCritical() );

		// Update cursor to the back buffer and clear the primary copy back flag
		gRapiMouse.UpdateCursor( false );

		// Present the scene
		m_pD3DDevice->Present( NULL, NULL, NULL, NULL );
		m_bOnGDIBuffer	= !m_bOnGDIBuffer;
		return true;
	}

	return false;
}

// Clear the backbuffer
void Rapi::ClearBackBuffer( RECT *pRect, DWORD color )
{
	switch( m_primaryPixelFormat )
	{
		case PIXEL_ARGB_8888:
		{
			break;
		}

		case PIXEL_RGB_565:
		{
			color	= (DWORD)MAKE565FROM8888( color );
			break;
		}

		case PIXEL_ARGB_1555:
		{
			color	= (DWORD)MAKE1555FROM8888( color );
			break;
		}

		case PIXEL_ARGB_4444:
		{
			color	= (DWORD)MAKE4444FROM8888( color );
			break;
		}

		default:
		{
			gperror( "Unrecognized primary buffer pixel format, cannot clear!" );
		}
	};
	
	m_pD3DDevice->Clear( pRect ? 1 : 0, (D3DRECT*)pRect, D3DCLEAR_TARGET, color, 1.0f, 0 );
}

// Clear front and all back buffers to black
void Rapi::ClearAllBuffers()
{
	DWORD oldColor = GetClearColor();
	bool oldFog = GetFogActivatedState();
	SetClearColor( 0 );
	SetFogActivatedState( false );

	int bufferCount = m_bTripleBuffering ? 3 : 2;
	while ( bufferCount-- > 0 )
	{
		Begin3D();
		End3D( true, true );
	}

	SetClearColor( oldColor );
	SetFogActivatedState( oldFog );
}

void Rapi::SetSystemDetailList( const SystemDetailList& sysDetails )
{
	// Store off this list of detail identifiers
	m_sysDetailList	= sysDetails;

	// Setup the current detail level
	SetSystemDetailIdentifier();
}

void Rapi::SetSystemDetailIdentifier()
{
	// Go through the available detail list and find a suitable match
	unsigned int current_mem_diff	= UINT_MAX;
	unsigned int current_avail_mem	= min_t( m_availTexMem, SysInfo::GetTotalPhysicalMemory() / 2 );
	for( SystemDetailList::const_iterator d = m_sysDetailList.begin(); d != m_sysDetailList.end(); ++d )
	{
		// Make sure that our bits per pixel matches
		if( (*d).bpp == m_BPP )
		{
			// Find the closest memory match among the contestants
			unsigned int mem_diff	= (current_avail_mem > (*d).memory) ? (current_avail_mem - (*d).memory) : ((*d).memory - current_avail_mem);
			if( mem_diff < current_mem_diff )
			{
				current_mem_diff	= mem_diff;
				m_sysDetail			= (*d).detail;
				m_shadowTexSize		= (*d).shadowTexSize;
				m_validModes		= (*d).valid_modes;
			}
		}
	}
}

// Display mode information
const DisplayList Rapi::GetDisplayModeList()
{
	// Create our return list
	DisplayList displayList;

	// Go through the enumerated display listing
	for( DisplayList::iterator i = m_DisplayList.begin(); i != m_DisplayList.end(); ++i )
	{
		for( ValidModeList::iterator v = m_validModes.begin(); v != m_validModes.end(); ++v )
		{
			if( ((*i).m_Width == (*v).m_Width) && ((*i).m_Height == (*v).m_Height) )
			{
				if( m_b16bitOnly && ((*i).m_BPP != 16) )
				{
					continue;
				}
				displayList.push_back( (*i) );
			}
		}
	}

	return displayList;
}

void Rapi::GetClosestDisplayMode( const int mWidth, const int mHeight, const int mBPP,
								  int& rWidth, int& rHeight, int& rBPP )
{
	int closest_match	= INT_MAX;

	// Go through the enumerated display listing and find the best match
	for( DisplayList::iterator i = m_DisplayList.begin(); i != m_DisplayList.end(); ++i )
	{
		int current_match	= abs( (*i).m_Width - mWidth ) + abs( (*i).m_Height - mHeight ) + abs( (*i).m_BPP - mBPP );
		if( current_match < closest_match )
		{
			closest_match	= current_match;
			rWidth			= (*i).m_Width;
			rHeight			= (*i).m_Height;
			rBPP			= (*i).m_BPP;
		}
	}
}

bool Rapi::BitDepthAvailable( const int bpp )
{
	for( DisplayList::iterator i = m_DisplayList.begin(); i != m_DisplayList.end(); ++i )
	{
		if( (*i).m_BPP == bpp )
		{
			return true;
		}
	}

	return false;
}

void Rapi::SetMipFilter( TEX_MPF filter )
{
	if( filter == D3DTEXF_LINEAR && m_bTrilinearCapable )
	{
		m_mipFilter = filter;
	}
	else
	{
		m_mipFilter = D3DTEXF_POINT;
	}
}

void Rapi::SetFlipAllowed( bool bFlip )
{
	// Save the state
	m_bFlip = bFlip;
}

void Rapi::SetGameElapsedTime( double time )
{
	m_elapsedTime	= time;
	m_secondTime	+= time;
}

// Set wireframe mode
void Rapi::SetWireframe( bool bWire )
{
	m_bWireframe	= bWire;
	if( bWire ){
		TRYDX (m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME ));
	}
	else{
		TRYDX (m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID ));
	}
}

// Set the gamma
void Rapi::SetGamma( const vector_3& gamma )
{
	// Set our gamma
	gpassert( (gamma.x >= 0.0f - FLOAT_TOLERANCE) && (gamma.x <= 2.0f + FLOAT_TOLERANCE) );
	gpassert( (gamma.y >= 0.0f - FLOAT_TOLERANCE) && (gamma.y <= 2.0f + FLOAT_TOLERANCE) );
	gpassert( (gamma.z >= 0.0f - FLOAT_TOLERANCE) && (gamma.z <= 2.0f + FLOAT_TOLERANCE) );

	m_Gamma = gamma;

	// Build the gamma table
	BuildGammaTable();
}

void Rapi::SetGamma( const float gamma )
{
	// Set our gamma
	gpassert( (gamma >= 0.0f - FLOAT_TOLERANCE) && (gamma <= 2.0f + FLOAT_TOLERANCE) );
	m_Gamma = vector_3( gamma, gamma, gamma );

	// Build the gamma table
	BuildGammaTable();
}

void Rapi::SetSystemGamma( const float gamma )
{
	// Set the system gamma
	m_SystemGamma	= gamma;

	// Build the gamma table
	BuildGammaTable();
}

void Rapi::BuildGammaTable()
{
	if( !m_bColorControl )
	{
		return;
	}

	// Invert the gamma (because 0.0f should be dark and 2.0f should be light);
	vector_3 gamma( DoNotInitialize );
	float correctedSysGamma	= 2.5f / m_SystemGamma;

	gamma.x	= (2.0f - m_Gamma.x) / correctedSysGamma;
	gamma.y = (2.0f - m_Gamma.y) / correctedSysGamma;
	gamma.z = (2.0f - m_Gamma.z) / correctedSysGamma;

	// Fill in the ramp table for gamma correction.
	D3DGAMMARAMP ramptable;
	memset( &ramptable, 0, sizeof( D3DGAMMARAMP ) );

	for( int i = 0; i < 256; ++i )
	{
		ramptable.red[i]	= (WORD)(pow((double)i / 255.0, gamma.x) * 65535.0);
		ramptable.green[i]	= (WORD)(pow((double)i / 255.0, gamma.y) * 65535.0);
		ramptable.blue[i]	= (WORD)(pow((double)i / 255.0, gamma.z) * 65535.0);
	} 

	m_pD3DDevice->SetGammaRamp( D3DSGR_CALIBRATE, &ramptable );
}

// Set the maximum Z
void Rapi::SetMaximumZ( float z )
{
	D3DVIEWPORT8 viewPort;
	TRYDX( m_pD3DDevice->GetViewport( &viewPort ) );
	if( IsEqual( z, viewPort.MaxZ ) && IsZero( viewPort.MinZ ) )
	{
		return;
	}

	viewPort.MinZ		= 0.0f;
	viewPort.MaxZ		= z;

	TRYDX( m_pD3DDevice->SetViewport( &viewPort ) );
}

// Set the minimum Z
void Rapi::SetMinimumZ( float z )
{
	D3DVIEWPORT8 viewPort;
	TRYDX( m_pD3DDevice->GetViewport( &viewPort ) );
	if( IsEqual( z, viewPort.MinZ ) && IsOne( viewPort.MaxZ ) )
	{
		return;
	}

	if( IsOne( z ) )
	{
		z	= 0.9990f;
	}
	viewPort.MinZ		= z;
	viewPort.MaxZ		= 1.0f;

	TRYDX( m_pD3DDevice->SetViewport( &viewPort ) );
}

// Set the texture factor based off of the given vector
void Rapi::SetActiveLightFactor( const vector_3& light )
{
	// Calculate the factor
	const unsigned int red		= FTOL( min_t( 255.0f, ( ( light.x + 1.0f ) * 127.5f ) ) );
	const unsigned int green	= FTOL( min_t( 255.0f, ( ( light.y + 1.0f ) * 127.5f ) ) );
	const unsigned int blue		= FTOL( min_t( 255.0f, ( ( light.z + 1.0f ) * 127.5f ) ) );

	DWORD color					= ( red << 16 ) | ( green << 8 ) | ( blue << 0 );

	// Set the factor
	TRYDX (m_pD3DDevice->SetRenderState( D3DRS_TEXTUREFACTOR, color ));
}

// Enable/Disable alpha blending.  Returns old alpha blend state.
bool Rapi::EnableAlphaBlending( bool bBlend )
{
	bool retState			= m_bAlphaBlendEnabled;
	m_bAlphaBlendEnabled	= bBlend;

	if( bBlend )
	{
		m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	}
	else
	{
		m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	}

	return retState;
}

// Enable/Disable async cursor
void Rapi::SetAsyncCursor( bool bCursor )
{
	// Save new cursor state
	m_bAsyncCursor	= bCursor;

	// Create RapiMouse
	if( m_pRapiMouse )
	{
		if( m_bAsyncCursor && !m_pRapiMouse->IsMousePaused() )
		{
			m_pRapiMouse->PauseMouse( true, true );
		}
		else if( !m_bAsyncCursor && m_pRapiMouse->IsMousePaused() )
		{
			m_pRapiMouse->PauseMouse( false );
		}
	}
}

// Explicitly destroy a texture and all of its resources
bool Rapi::DestroyTexture( const unsigned int tex )
{
	// Lock the texture collection critical
	kerneltool::Critical::Lock autoLock( m_TextureCritical );

	// Check validity of request
	if( !m_Textures.IsValid( tex ) )
	{
		gperror( "Invalid texture has been destroyed!" );
		return false;
	}

	RapiTexture* pTexture	= m_Textures[ tex ];
	if( pTexture )
	{
		gpassert( pTexture->GetReferenceCount() );

		pTexture->DecrementReferenceCount();

		if( pTexture->GetReferenceCount() == 0 )
		{
			// Remove this texture from the mapping
			TextureMap::iterator t = m_TextureMap.find( pTexture->GetPathname() );
			if( t != m_TextureMap.end() )
			{
				// Only erase the first element we find, as this texture may be multiinstance
				m_TextureMap.erase( t );
			}

			// Delete the texture
			for( unsigned int i = 0; i < NUM_TEXTURES; ++i )
			{
				if( m_ActiveTex[ i ] == pTexture )
				{
					TRYDX( m_pD3DDevice->SetTexture( i, NULL ) );
					m_ActiveTex[ i ] = NULL;
				}
			}

			delete pTexture;

			// Free the spot
			m_Textures.Free( tex, NULL );
			return true;
		}
	}

	return false;
}

// Set the active texture for this stage
void Rapi::SetTexture( const unsigned int stage, const unsigned int tex )
{
	if( !m_Textures.IsValid( tex ) )
	{
		gperror( "Invalid texture has been requested!" );
		return;
	}

	RapiTexture* pTexture	= m_Textures[ tex ];

	// Handle active texture logging
	if( m_bActiveTextureLog && pTexture )
	{
		m_ActiveTextureLog.insert( pTexture->GetPathname() );
	}

	// Go through available textures and setup stages
	if( m_ActiveTex[ stage ] == pTexture )
	{
		if( pTexture )
		{
			pTexture->SetSurfaceInformation( stage );
		}
		return;
	}
	m_ActiveTex[ stage ] = pTexture;

	if( pTexture )
	{
		pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, stage );
	}
	else
	{
		TRYDX (m_pD3DDevice->SetTexture( stage, NULL ));
		m_bNoRender	= false;
	}

#if !GP_RETAIL

	// Add one to our texture switches, so we can see how much performance is lost
	m_TextureSwitches++;

#endif
}

// See if given texture has alpha
bool Rapi::IsTextureAlpha( const unsigned int tex )
{
	if( m_Textures.IsValid( tex ) && m_Textures[ tex ] )
	{
		return m_Textures[ tex ]->HasAlpha();
	}

	return false;
}

// See if given texture does not need alpha sorting
bool Rapi::IsTextureNoAlphaSort( const unsigned int tex )
{
	if( m_Textures.IsValid( tex ) && m_Textures[ tex ] )
	{
		return m_Textures[ tex ]->IsNoAlphaSort();
	}

	return false;
}

// See if given texture has environment
bool Rapi::IsTextureEnvironment( const unsigned int tex )
{
	if( m_Textures.IsValid( tex ) && m_Textures[ tex ] )
	{
		return m_Textures[ tex ]->HasEnvironment();
	}

	return false;
}

// See if given texture was loaded from disk
bool Rapi::IsTextureFromDisk( const unsigned int tex )
{
	if( m_Textures.IsValid( tex ) && m_Textures[ tex ] )
	{
		return m_Textures[ tex ]->IsFromDisk();
	}

	return false;
}

// Set the named texture's animation state
void Rapi::SetTextureAnimationState( unsigned int tex, bool bState )
{
	// Lock the texture collection critical
	kerneltool::Critical::Lock autoLock( m_TextureCritical );

	if( m_Textures.IsValid( tex ) && m_Textures[ tex ] )
	{
		m_Textures[ tex ]->SetAnimationState( bState );
	}
}

// Set the named texture's animation speed
void Rapi::SetTextureAnimationSpeed( unsigned int tex, float speed )
{
	// Lock the texture collection critical
	kerneltool::Critical::Lock autoLock( m_TextureCritical );

	if( m_Textures.IsValid( tex ) && m_Textures[ tex ] )
	{
		m_Textures[ tex ]->SetAnimationSpeed( speed );
	}
}

// Get the named texture's animation state
bool Rapi::GetTextureAnimationState( unsigned int tex )
{
	if( m_Textures.IsValid( tex ) && m_Textures[ tex ] )
	{
		return( m_Textures[ tex ]->GetAnimationState() );
	}

	return false;
}

// Get the named texture's animation speed
float Rapi::GetTextureAnimationSpeed( unsigned int tex )
{
	if( m_Textures.IsValid( tex ) && m_Textures[ tex ] )
	{
		return( m_Textures[ tex ]->GetAnimationSpeed() );
	}

	return 0.0f;
}

// Create a texture for this device
const unsigned int Rapi::CreateTexture(	const		gpstring& texture_file,
										bool		bMipmapping,
										int			rStage,
										TEX_OPT		rOptimize,
										bool		bFullLoad,
										gpstring*	pErrorSink )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	gpstring errorSink;
	{
		// Lock the texture collection critical
		kerneltool::Critical::Lock autoLock( m_TextureCritical );

		// See if this file already exists in our database
		TextureMap::iterator i = m_TextureMap.find( texture_file );
		if( (i != m_TextureMap.end()) && (!m_Textures[ (*i).second ]->IsMultiInstance()) )
		{
			m_Textures[ (*i).second ]->IncrementReferenceCount();
			if( bFullLoad )
			{
				m_Textures[ (*i).second ]->IncrementLoadReferenceCount();
			}
			return (*i).second;
		}
	}

	// Create a new texture
	RapiTexture* pTex	= new RapiTexture( this, texture_file, bMipmapping, rStage, rOptimize, bFullLoad, errorSink );

	// Put this texture into our global list
	unsigned int texture_slot	= m_Textures.Alloc( pTex );

	if( texture_slot )
	{
		// Lock the texture collection critical
		kerneltool::Critical::Lock autoLock( m_TextureCritical );

		// Insert texture into slot and mapping
		m_TextureMap.insert( std::pair< gpstring, unsigned int >( texture_file, texture_slot ) );

		// Increase ref count
		pTex->IncrementReferenceCount();

		// Handle saturation if needed
		if( m_SaturatedTextureSet.find( texture_file ) != m_SaturatedTextureSet.end() )
		{
			pTex->SetSaturation( m_Saturation );
		}
	}
	else
	{
		delete pTex;
	}

	if( !errorSink.empty() && !m_bEdit )
	{
		if( pErrorSink )
		{
			*pErrorSink	= errorSink;
		}
		else
		{
			gperror( errorSink );
		}
	}

	// Send the texture pointer back to the client
	return texture_slot;
}

// Restore (or upgrade) a texture that had no (or lost) surface information
bool Rapi::LoadTexture( const unsigned int tex )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	// Check validity of request
	if( !m_Textures.IsValid( tex ) )
	{
		gperror( "Invalid texture has been loaded!" );
		return false;
	}

	// Tell this texture to load
	RapiTexture* pTexture	= m_Textures[ tex ];
	if( pTexture )
	{
		bool bLoad		= false;
		{
			// Lock the texture collection critical
			kerneltool::Critical::Lock autoLock( m_TextureCritical );

			if( pTexture->GetLoadReferenceCount() == 0 )
			{
				bLoad	= true;
			}
			pTexture->IncrementLoadReferenceCount();
		}

		if( bLoad )
		{
			// Load the texture
			pTexture->Load();
		}

		return true;
	}

	return false;
}

// Release (or downgrade) a texture that has full surface information
bool Rapi::UnloadTexture( const unsigned int tex )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	// Check validity of request
	if( !m_Textures.IsValid( tex ) )
	{
		gperror( "Invalid texture has been unloaded!" );
		return false;
	}

	// Tell this texture to load
	RapiTexture* pTexture	= m_Textures[ tex ];
	if( pTexture )
	{
		// Lock the texture collection critical
		kerneltool::Critical::Lock autoLock( m_TextureCritical );

		gpassert(pTexture->GetLoadReferenceCount());

		pTexture->DecrementLoadReferenceCount();
		if( pTexture->GetLoadReferenceCount() == 0 )
		{
			pTexture->Unload();
		}

		return true;
	}

	return false;
}

// Create a texture for this device from a image instead of a file
const unsigned int Rapi::CreateAlgorithmicTexture(	gpstring const	&texture_name,
													RapiMemImage	*pTakeImage,
													bool			bMipmapping,
													int				rStage,
													TEX_OPT			rOptimize,
													bool			bHasAlpha,
													bool			bDither )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	gpstring errorSink;

	// Build the texture
	RapiTexture* pTex	= new RapiTexture( this, texture_name, pTakeImage, bMipmapping, rStage, rOptimize, bHasAlpha, bDither, errorSink );

	// Put this texture into our global list
	unsigned int texture_slot	= m_Textures.Alloc( pTex );

	if( texture_slot )
	{
		// Increase ref count
		pTex->IncrementReferenceCount();
	}
	else
	{
		delete pTex;
	}

	if( !errorSink.empty() && !m_bEdit )
	{
		gperror( errorSink );
	}

	// Send the texture pointer back to the client
	return texture_slot;
}

// Creates a texture from a region in the primary buffer
const unsigned int Rapi::CreateSubsectionTexture(	const RECT	*pRect,
													bool		bMipmapping,
													int			rStage,		
													TEX_OPT		rOptimize )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	gpstring errorSink;
	
	RapiMemImage * pImage = CreateSubsectionImage( pRect );

	// Build the texture
	RapiTexture* pTex	= new RapiTexture( this, "subsection", pImage, bMipmapping, rStage, rOptimize, false, false, errorSink );

	// Put this texture into our global list
	unsigned int texture_slot	= m_Textures.Alloc( pTex );;

	if( texture_slot )
	{
		// Increase ref count
		pTex->IncrementReferenceCount();
	}
	else
	{
		delete pTex;
	}

	if( !errorSink.empty() && !m_bEdit )
	{
		gperror( errorSink );
	}

	// Send the texture pointer back to the client
	return texture_slot;
}

// Creates a blank texture filled with the given color
const unsigned int Rapi::CreateBlankTexture( int		width,
											 int		height,
											 DWORD		color,
											 TEX_OPT	rOptimize,
											 bool		bHasAlpha,
											 bool		bMatchPrimary,
											 bool		bRenderTarget )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	gpstring errorSink;

	// Build the texture
	RapiTexture* pTex		= new RapiTexture( this, width, height, color, rOptimize, bHasAlpha, false, bMatchPrimary, bRenderTarget, errorSink );

	// Put this texture into our global list
	unsigned int texture_slot	= m_Textures.Alloc( pTex );

	if( texture_slot )
	{
		// Increase ref count
		pTex->IncrementReferenceCount();
	}
	else
	{
		delete pTex;
	}

	if( !errorSink.empty() && !m_bEdit )
	{
		gperror( errorSink );
	}

	// Send the texture pointer back to the client
	return texture_slot;
}

// Creates a texture for shadow use
const unsigned int Rapi::CreateShadowTexture()
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );
	return CreateBlankTexture( m_shadowTexSize, m_shadowTexSize, 0xFFFFFFFF, TEX_DYNAMIC, false, true, m_bShadowRenderTarget );
}

// Creates an image from a region in the primary buffer - You must maintain and delete the pointer
RapiMemImage * Rapi::CreateSubsectionImage(	const RECT 	*pRect )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	RapiMemImage * pImage = new RapiMemImage;
	CopySubsectionToImage( pImage, pRect );
	return ( pImage );
}

// Creates a texture from a region in the primary buffer - You must maintain and delete the pointer yourself
void Rapi::CopySubsectionToImage(	RapiMemImage	*pImage,
									const RECT		*pRect )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	// Create a reader
	std::auto_ptr <RapiImageReader> reader( RapiImageReader::CreateReader( "<backbuffer>", m_pBackBuffer, false, true, pRect ) );
	if ( reader.get() != NULL )
	{
		// Copy the surface to it (flip so we have 0,0 as lower left as is "standard" in this game)
		reader->GetNextSurface( *pImage, true );
	}
}

// Blits a region in the primary buffer to the given texture
void Rapi::CopySubsectionToTexture(	const unsigned int	texture,
									const RECT			*pRect )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	// Check validity of request
	if( !m_Textures.IsValid( texture ) )
	{
		return;
	}

	gpassert( m_Surfaces.IsValid( m_Textures[ texture ]->m_pSurfaceDescription[0].m_SurfaceLayers[0].m_textureSurface[0] ) );
	RapiSurface* pSurface	= m_Surfaces[ m_Textures[ texture ]->m_pSurfaceDescription[0].m_SurfaceLayers[0].m_textureSurface[0] ];
	if( pSurface->GetSurface() )
	{
		LPDIRECT3DSURFACE8 pD3DSurface;
		if( SUCCEEDED( pSurface->GetSurface()->GetSurfaceLevel( 0, &pD3DSurface ) ) )
		{
			POINT dstPoint = { 0, 0 };

			// Copy the primary image to the sub surface
			if( FAILED( m_pD3DDevice->CopyRects( m_pBackBuffer, pRect, 1, pD3DSurface, &dstPoint ) ) )
			{
				// Lock both surfaces
				D3DLOCKED_RECT srcLockedRect;
				if( FAILED( m_pBackBuffer->LockRect( &srcLockedRect, pRect, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK ) ) )
				{
					return;
				}

				D3DLOCKED_RECT dstLockedRect;
				if( FAILED( pD3DSurface->LockRect( &dstLockedRect, NULL, D3DLOCK_NOSYSLOCK ) ) )
				{
					m_pBackBuffer->UnlockRect();
					return;
				}

				CopyPixels( dstLockedRect.pBits, m_primaryPixelFormat, pRect->right - pRect->left, pRect->bottom - pRect->top, dstLockedRect.Pitch,
							srcLockedRect.pBits, pSurface->m_bAlpha ? m_alphaTexPixelFormat : m_opaqueTexPixelFormat, pSurface->GetWidth(), pSurface->GetHeight(), srcLockedRect.Pitch,
							false, false );

				pD3DSurface->UnlockRect();
				m_pBackBuffer->UnlockRect();
			}

			// Release our surface
			RELEASE( pD3DSurface );
		}
	}
}

// Get the primary surface of a texture
LPDIRECT3DTEXTURE8 Rapi::GetTexSurface( const unsigned int tex )
{
	if( m_Textures.IsValid( tex ) && m_Textures[ tex ] )
	{
		return m_Surfaces[ m_Textures[ tex ]->m_pSurfaceDescription[0].m_SurfaceLayers[0].m_textureSurface[0] ]->GetSurface();
	}

	return NULL;
}

// Verify that a texture exists in Rapi
bool Rapi::TextureExists( const gpstring& texture_file )
{
	// Lock the texture collection critical
	kerneltool::Critical::Lock autoLock( m_TextureCritical );

	return ( m_TextureMap.find( texture_file ) != m_TextureMap.end() );
}

// Add a reference to the given texture
bool Rapi::AddTextureReference( const unsigned int tex )
{
	// Lock the texture collection critical
	kerneltool::Critical::Lock autoLock( m_TextureCritical );

	if( m_Textures.IsValid( tex ) && m_Textures[ tex ] )
	{
		m_Textures[ tex ]->IncrementReferenceCount();
		return true;
	}

	return false;
}

// Get the current reference count
unsigned int Rapi::GetTextureReferenceCount( const unsigned int tex )
{
	// Lock the texture collection critical
	kerneltool::Critical::Lock autoLock( m_TextureCritical );

	if( m_Textures.IsValid( tex ) && m_Textures[ tex ] )
	{
		return m_Textures[ tex ]->GetReferenceCount();
	}

	return 0;
}

// Get the current reference count
unsigned int Rapi::GetLoadTextureReferenceCount( const unsigned int tex )
{
	// Lock the texture collection critical
	kerneltool::Critical::Lock autoLock( m_TextureCritical );

	if( m_Textures.IsValid( tex ) && m_Textures[ tex ] )
	{
		return m_Textures[ tex ]->GetLoadReferenceCount();
	}

	return 0;
}

// Makes sure that all surfaces are properly restored and that gamma is properly set.
void Rapi::RestoreSurfaces()
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	// Make sure we have the proper interfaces
	if( !m_pD3D )
	{
		return;
	}

	// Restore primary surfaces
	RestoreMainSurfaces( true );

	// Set the gamma level
	BuildGammaTable();
}

// Reloads all loaded textures
void Rapi::ReloadSurfaces()
{
	// Lock the surface collection critical
	kerneltool::Critical::Lock autoLock( m_SurfaceCritical );
	kerneltool::RwCritical::ReadLock vectLock( m_Surfaces.GetCritical() );

	// Iterate through the texture listing
	for( SurfaceBucketVector::iterator i = m_Surfaces.begin(); i != m_Surfaces.end(); ++i )
	{
#if !GP_RETAIL
		gpstring errorSink;
		(*i)->RestoreSurface( &errorSink );

		if( !errorSink.empty() && !m_bEdit )
		{
			errorSink.appendf( "RestoreSurface() failed for %s", (*i)->GetPathname().c_str() );
			gperror( errorSink );
		}
#else
		(*i)->RestoreSurface();
#endif
	}
}

// Active texture logging
void Rapi::SetActiveTextureLogging( bool bLogging )
{
	// Clear our log if we're turning it on
	if( bLogging )
	{
		m_ActiveTextureLog.clear();
	}

	// Save logging state
	m_bActiveTextureLog = bLogging;
}

// Change the saturation level for a collection of textures
void Rapi::ChangeSaturationForTextures( const TextureSet& texSet, float saturation )
{
	// Lock the texture collection critical
	kerneltool::Critical::Lock autoLock( m_TextureCritical );

	// Save saturation level
	m_Saturation	 = saturation;

	for( TextureSet::const_iterator i = texSet.begin(); i != texSet.end(); ++i )
	{
		// See if this file already exists in our database
		std::pair< TextureMap::iterator, TextureMap::iterator > tm = m_TextureMap.equal_range( (*i) );
		for( ; tm.first != tm.second ; ++tm.first )
		{
			// Tell the texture to change its saturation level
			m_Textures[ (*tm.first).second ]->SetSaturation( saturation );
		}
	}

	// Save this set for future creations
	m_SaturatedTextureSet	= texSet;
}

// Revert any textures that have had their saturation level changed
void Rapi::RevertSaturatedTextures()
{
	// Lock the surface collection critical
	kerneltool::Critical::Lock autoLock( m_TextureCritical );
	kerneltool::RwCritical::ReadLock vectLock( m_Textures.GetCritical() );

	// Reset saturation level
	m_Saturation	 = 1.0f;

	// Go through all textures looking for those that have been modified and change them back
	for( TextureBucketVector::iterator t = m_Textures.begin(); t != m_Textures.end(); ++t )
	{
		if( (*t)->IsSaturated() )
		{
			(*t)->RevertSaturation();
		}
	}

	// Clear our saturated set now that we are back to normal
	m_SaturatedTextureSet.clear();

	// Make sure all surfaces are properly restored
	RestoreSurfaces();
}

// Creation of a file based surface
const unsigned int Rapi::CreateSurface(	const gpstring		surface_file,
										bool				bMipmapping,
										int					rStage,
										TEX_OPT				rOptimize,
										bool				bAlpha,
										bool				bDither,
										int					texDetail,
										bool				bFullLoad,
										gpstring&			errorSink )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	RapiSurface* pSurf	= NULL;

	{
		// Lock the surface collection critical
		kerneltool::Critical::Lock autoLock( m_SurfaceCritical );

		// See if this file already exists in our database
		SurfaceMap::iterator i = m_SurfaceMap.find( surface_file );
		if( i != m_SurfaceMap.end() )
		{
			pSurf	= m_Surfaces[ (*i).second ];

			// Erase surface from the expiration map
			if( !pSurf->GetReferenceCount() )
			{
				m_SurfaceExpirationMap.erase( (*i).second );
			}

			// Setup proper ref counts
			pSurf->IncrementReferenceCount();
			if( bFullLoad )
			{
				pSurf->IncrementLoadReferenceCount();
			}
			return (*i).second;
		}
	}

	// Create a new texture
	pSurf	= new RapiSurface( this, surface_file, bMipmapping, rStage, rOptimize, bAlpha, bDither, texDetail, bFullLoad, errorSink );

	// Put this texture into our global list
	unsigned int surface_slot	= m_Surfaces.Alloc( pSurf );
	if( surface_slot )
	{
		// Lock the surface collection critical
		kerneltool::Critical::Lock autoLock( m_SurfaceCritical );

		// Insert texture into slot and mapping
		m_SurfaceMap.insert( std::pair< gpstring, unsigned int >( surface_file, surface_slot ) );

		// Increase ref count
		pSurf->IncrementReferenceCount();
	}
	else
	{
		delete pSurf;
	}

	// Send the texture pointer back to the client
	return surface_slot;
}

// Creation of an algorithmic surface
const unsigned int Rapi::CreateSurface(	RapiMemImage		*pTakeImage,
										bool				bMipmapping,
										int					rStage,
										TEX_OPT				rOptimize,
										bool				bAlpha,
										bool				bDither,
										int					texDetail,
										gpstring&			errorSink )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	if( pTakeImage != NULL )
	{
		// We have a valid image
		RapiSurface* pSurf	= new RapiSurface( this, pTakeImage, bMipmapping, rStage, rOptimize, bAlpha, bDither, texDetail, errorSink );

		// Debugging to make sure everything is valid
		gpassert( pSurf );
		gpassert( pSurf->GetSurface() );

		// Put this texture into our global list
		unsigned int surface_slot	= m_Surfaces.Alloc( pSurf );
		if( surface_slot )
		{
			// Increase ref count
			pSurf->IncrementReferenceCount();
		}
		else
		{
			delete pSurf;
		}

		// Add this tex to our system memory counter
		m_TexSystemMemory += pTakeImage->GetSizeInBytes();

		// Send the texture pointer back to the client
		return surface_slot;
	}

	return 0;
}

// Creation of a blank surface
const unsigned int Rapi::CreateSurface(	int					width,
										int					height,
										DWORD				color,	
										TEX_OPT				rOptimize,
										bool				bAlpha, 
										bool				bDither,
										bool				bMatchPrimary,
										bool				bRenderTarget,
										gpstring&			errorSink )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	// We have a valid image
	RapiSurface* pSurf	= new RapiSurface( this, width, height, color, rOptimize, bAlpha, bDither, bMatchPrimary, bRenderTarget, errorSink );

	// Debugging to make sure everything is valid
	gpassert( pSurf );
	gpassert( pSurf->GetSurface() );

	// Put this texture into our global list
	unsigned int surface_slot	= m_Surfaces.Alloc( pSurf );
	if( surface_slot )
	{
		// Increase ref count
		pSurf->IncrementReferenceCount();
	}
	else
	{
		delete pSurf;
	}

	// Send the texture pointer back to the client
	return surface_slot;
}

bool Rapi::LoadSurface( const unsigned int surface )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	// Check validity of request
	if( !m_Surfaces.IsValid( surface ) )
	{
		gperror( "Invalid surface has been loaded!" );
		return false;
	}

	RapiSurface* pSurf	= m_Surfaces[ surface ];
	if( pSurf )
	{
		bool bLoad		= false;
		{
			// Lock the texture collection critical
			kerneltool::Critical::Lock autoLock( m_SurfaceCritical );

			if( m_SurfaceExpirationMap.find( surface ) != m_SurfaceExpirationMap.end() )
			{
				gperror( "Cannot load surface that is waiting to expire!" );
			}

			if( pSurf->GetLoadReferenceCount() == 0 )
			{
				bLoad	= true;
			}
			pSurf->IncrementLoadReferenceCount();
		}

		if( bLoad )
		{
#if !GP_RETAIL
			gpstring errorSink;
			bool bRestored = pSurf->RestoreSurface( &errorSink );

			// Check for error
			if( !errorSink.empty() && !m_bEdit )
			{
				errorSink.appendf( "RestoreSurface() failed for %s", pSurf->GetPathname().c_str() );
				gperror( errorSink );
			}

			return bRestored;
#else
			return pSurf->RestoreSurface();
#endif
		}
	}

	return false;
}

bool Rapi::UnloadSurface( const unsigned int surface )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	// Check validity of request
	if( !m_Surfaces.IsValid( surface ) )
	{
		gperror( "Invalid texture has been unloaded!" );
		return false;
	}

	RapiSurface* pSurf	= m_Surfaces[ surface ];
	if( pSurf )
	{
		// Lock the texture collection critical
		kerneltool::Critical::Lock autoLock( m_SurfaceCritical );

		if( m_SurfaceExpirationMap.find( surface ) != m_SurfaceExpirationMap.end() )
		{
			gperror( "Cannot unload surface that is waiting to expire!" );
		}
		
		gpassert( pSurf->GetLoadReferenceCount() );
		pSurf->DecrementLoadReferenceCount();
		if( pSurf->GetLoadReferenceCount() == 0 )
		{
			pSurf->Destroy();
		}

		return true;
	}

	return false;
}

// Destruction of a surface
void Rapi::DestroySurface( unsigned int surface )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	// Lock the surface collection critical
	kerneltool::Critical::Lock autoLock( m_SurfaceCritical );

	// It doesn't hurt to be extra careful here, because we are not really causing any
	// performance hitches since this function is not called all that often.
	if( !m_Surfaces.IsValid( surface ) )
	{
		gperror( "Invalid surface has been destroyed!" );
		return;
	}
	
	RapiSurface* pSurface	= m_Surfaces[ surface ];
	if( pSurface )
	{
		gpassert( pSurface->GetReferenceCount() );
		pSurface->DecrementReferenceCount();

		if( pSurface->GetReferenceCount() == 0 )
		{
			if( !pSurface->GetPathname().empty() )
			{
				// Add this surface to our expiration mapping
				m_SurfaceExpirationMap.insert( std::pair< unsigned int, float >( surface, 0.0f ) );
			}
			else
			{
				// Subtract this tex from our system memory counter
				if( pSurface->GetTextureBits() != NULL )
				{
					m_TexSystemMemory -= pSurface->GetTextureBits()->GetSizeInBytes();
				}

				// Remove this texture from the mapping
				m_SurfaceMap.erase( pSurface->GetPathname() );

				// Delete the texture
				delete pSurface;
				m_Surfaces.Free( surface, NULL );
			}
		}
	}
}

// Restore main surfaces
bool Rapi::RestoreMainSurfaces( bool bInfiniteWait )
{
	GPSTATS_GROUP( GROUP_LOAD_TEXTURE );

	// The most common case is that the coop level is good to go.  Optimize for this case.
	if( m_pD3DDevice->TestCooperativeLevel() == D3D_OK )
	{
		return true;
	}

	// Wait for the application to return to its proper mode
	DWORD spinCount	= 20;
	while( m_pD3DDevice->TestCooperativeLevel() == D3DERR_DEVICELOST && spinCount )
	{
		Sleep( 5 );
		if( !bInfiniteWait )
		{
			--spinCount;
		}
	}
	if( m_pD3DDevice->TestCooperativeLevel() == D3DERR_DEVICELOST )
	{
		return false;
	}

	if( m_pD3DDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET )
	{
		// Reset the device
		return ResetDevice();
	}

	return true;
}

// Create a new light
const unsigned int Rapi::CreateLight()
{
	// Create a new light object
	RapiLight* pLight	= new RapiLight();

	// Debuggin info
	gpassert( pLight );

	// Put our new light into the global list
	unsigned int light_slot	= FindEmptyLightSlot();
	gpassert( light_slot >= 0 && light_slot < LIGHT_CACHE );
	m_Lights[ light_slot ]	= pLight;

	// Insert the light into D3D
	TRYDX (m_pD3DDevice->SetLight( light_slot, &pLight->GetLight() ));

	// Return the handle to our loyal clients
	return light_slot;
}

// Destroy indicated light and remove it from the list
void Rapi::DestroyLight( const unsigned int light )
{
	// Make sure some dummy isn't telling me to do something stupid
	gpassert( (light >= 0) && (light < LIGHT_CACHE) );
	gpassert( m_Lights[ light ] );

	// Make sure I don't do anything stupid, and clean up the light
	if( (light >= 0) && (light < LIGHT_CACHE) ){
		if( m_Lights[ light ] ){
			delete m_Lights[ light ];
			m_Lights[ light ] = NULL;
		}
	}
}

// Set the properties of a light
void Rapi::SetLight(	unsigned int	light,		// light id
						LTYPE			lType,		// Type of light
						LCOLOR			&lColor,	// Color of light
						vector_3		&vPos,		// World position of light
						vector_3		&vDir,		// Direction the light is pointing
						float			fRange,		// Range beyond which the light has no effect
						float			fLinearA,	// Linear attenuation value
						float			fConstA,	// Constant attenuation value
						float			fQuadA,		// Quadratic attenuation value
						float			fInnerAngle,// Inner cone angle (only used for spotlights)
						float			fOuterAngle	// Outer cone angle (only used for spotlights)
					)
{
	// Make sure we aren't doing anything bad
	if( (light >= 0) && (light < LIGHT_CACHE) )
	{
		if( m_Lights[ light ] )
		{
			// Set the lights parameters
			m_Lights[ light ]->SetLight( lType, lColor, vPos, vDir, fRange, fLinearA, fConstA, fQuadA, fInnerAngle, fOuterAngle );

			// Insert the light into D3D
			TRYDX (m_pD3DDevice->SetLight( light, &m_Lights[ light ]->GetLight() ));
		}
	}
}

// Enable or disable a light.
void Rapi::EnableLight( const unsigned int light, bool bEnable )
{
	if( (light >= 0) && (light < LIGHT_CACHE) )
	{
		if( m_Lights[ light ] )
		{
			// Enable the light
			TRYDX (m_pD3DDevice->LightEnable( light, bEnable ));

			// Tell the light about it
			m_Lights[ light ]->SetEnabled( bEnable );
		}
	}
}
/*
// Add a new light to the scene
RapiLight* Rapi::CreateLight(	LTYPE		lType,		// Type of light
								LCOLOR		&lColor,	// Color of light
								vector_3	&vPos,		// World position of light
								vector_3	&vDir,		// Direction the light is pointing
								float		fRange,		// Range beyond which the light has no effect
								float		fLinearA,	// Linear attenuation value
								float		fConstA,	// Constant attenuation value
								float		fQuadA,		// Quadratic attenuation value
								float		fInnerAngle,// Inner cone angle (only used for spotlights)
								float		fOuterAngle // Outer cone angle (only used for spotlights)
							)
{
	// Create new light
//	RapiLight* pLight	= new RapiLight( GetObject(), m_pD3DDevice, this );

	// Insert light into our list
//	m_Lights.insert( pLight );

	// Setup light with the given defaults
//	pLight->SetLight( lType, lColor, vPos, vDir, fRange, fLinearA, fConstA, fQuadA, fInnerAngle, fOuterAngle );

	// We are done here
//	return pLight;
	return NULL;
}
/*
// Remove a light from the scene
void Rapi::DestroyLight(	RapiLight* pLight )
{
	LightIter i = m_Lights.find( pLight );
	m_Lights.erase( i );
	delete pLight;
	pLight	= NULL;
}
*/
// Set ambient light
void Rapi::SetAmbientLight( DWORD color )
{
	m_ambience	= color;
	TRYDX (m_pD3DDevice->SetRenderState(  D3DRS_AMBIENT,  m_ambience ));
}

// Ignore vertex coloring information in vertices
void Rapi::IgnoreVertexColor( bool bIgnore )
{
	m_bIgnoreVertexColor	= bIgnore;
}

// Set/Reset the fog, returns the previous state
bool Rapi::SetFogActivatedState( const bool bNewStateRequest )
{
	bool prevstate	= m_bFogIsActive;

	// Check to see if we need to initialize the fog distances
	if( m_fogNearDist < 0.0f || m_fogFarDist < 0.0f )
	{
		SetFogNearDist( m_clipNearDist );
		SetFogFarDist( m_clipFarDist );
	}

	bool enablefog	= bNewStateRequest;
	if( ( m_fogType == RAPI_FOGTYPE_NONE ) || ( m_fogFarDist <= m_fogNearDist ) )
	{
		enablefog	= false;
	}

	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, enablefog ) );
	
	m_bFogIsActive	= bNewStateRequest;
	return prevstate;
}

bool Rapi::IsFogEnabled()
{
	return m_fogType != RAPI_FOGTYPE_NONE;
}

void Rapi::EnableRangeBasedFog( const bool bEnable )
{
	// We are not set up to to use range based fog
	UNREFERENCED_PARAMETER( bEnable );
	return;
/*
	if (!IsRangeFogAvailable()) return;

	if( bEnable == true ) {
		TRYDX (m_pD3DDevice->SetRenderState( D3DRS_RANGEFOGENABLE, TRUE ));
	} else {
		TRYDX (m_pD3DDevice->SetRenderState( D3DRS_RANGEFOGENABLE, FALSE));
	}
*/
}

bool Rapi::IsRangeBasedFogEnabled()
{
	// We are not set up to to use range based fog
	return false;
/*
	DWORD enabled = 0;
	TRYDX (m_pD3DDevice->GetRenderState( D3DRS_RANGEFOGENABLE, (LPDWORD)(&enabled) ));

	if( enabled == 1 ) {
		return true;
	}
	return false;
*/
}

bool Rapi::IsWFogEnabled()
{
	return (m_fogType == RAPI_FOGTYPE_WFOG);
}

bool Rapi::IsZFogEnabled()
{
	return (m_fogType == RAPI_FOGTYPE_ZFOG);
}

bool Rapi::IsVertFogEnabled()
{
	return (m_fogType == RAPI_FOGTYPE_VERTEX);
}

bool Rapi::IsRangeFogAvailable()
{
	// We are not set up to to use range based fog
	return false;
/*
	return (
		(m_d3dDevDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGRANGE) != 0
	);
*/
}

bool Rapi::IsTableFogAvailable()
{
	return( (m_d3dDevDesc.RasterCaps & D3DPRASTERCAPS_FOGTABLE) != 0 );
}

// no alpha for fog so only use lower 24 bits
void Rapi::SetFogColor( const DWORD color )
{
	// Save the fog color
	m_fogColor	= color | 0xFF000000;

	// Set the fog color
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_FOGCOLOR, m_fogColor ) );
}

void Rapi::SetFogDensity( const float density )
{
	// Save the fog density
	m_fogDensity	= density;

	// Set the fog color
	TRYDX( m_pD3DDevice->SetRenderState( D3DRS_FOGDENSITY, *((DWORD*)&density) ) );
}

void Rapi::SetFogModeFormula( const D3DFOGMODE mode )
{
	// Set the fog mode based off of our current fog type
	if( m_fogType == RAPI_FOGTYPE_VERTEX )
	{
		TRYDX (m_pD3DDevice->SetRenderState( D3DRS_FOGVERTEXMODE, mode ));
	}
	else
	{
		TRYDX (m_pD3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, mode ));
	}
}

DWORD Rapi::GetFogModeFormula( )
{
	DWORD mode;

	if( m_fogType == RAPI_FOGTYPE_VERTEX )
	{
		TRYDX (m_pD3DDevice->GetRenderState( D3DRS_FOGVERTEXMODE, &mode ));
	}
	else
	{
		TRYDX (m_pD3DDevice->GetRenderState( D3DRS_FOGTABLEMODE, &mode ));
	}

	return mode;
}

void Rapi::SetFogNearDist( const float &dist )
{
	m_fogNearDist = max( 0.0f, dist );
	if( m_fogFarDist < m_fogNearDist )
	{
		gperror( "Fog far distance cannot be less than fog near distance!" );
		return;
	}

	if( m_fogType == RAPI_FOGTYPE_ZFOG )
	{
		// Need to re-normalize the near fog distance
		float normdist = Minimum( 1.0f, m_fogNearDist/GetClipFarDistAdjustedForFog() );
		if( !IsNegative( normdist ) )
		{
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_FOGSTART, *((DWORD*)&normdist) ) );
		}
	}
	else if( !IsNegative( m_fogNearDist ) )
	{
		TRYDX( m_pD3DDevice->SetRenderState( D3DRS_FOGSTART, *((DWORD*)&m_fogNearDist) ) );
	}
}

float Rapi::GetFogNearDist( )
{
	return m_fogNearDist;
}

float Rapi::GetClipSpaceFogNearDist( )
{
	return Minimum( 1.0f, m_fogNearDist/GetClipFarDistAdjustedForFog() );
}

void Rapi::SetFogFarDist( const float &dist )
{
	m_fogFarDist	= max( 0.1f, dist );
	if( m_fogFarDist < m_fogNearDist )
	{
		gperror( "Fog far distance cannot be less than fog near distance!" );
		return;
	}

	if( m_fogType == RAPI_FOGTYPE_ZFOG )
	{
		// Need to re-normalize the near fog distance
		float normdist;
		normdist = Minimum( 1.0f, m_fogNearDist/GetClipFarDistAdjustedForFog() );
		if( !IsNegative( normdist ) )
		{
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_FOGSTART, *((DWORD*)&normdist) ) );
		}
		normdist = 1.0f;
		TRYDX( m_pD3DDevice->SetRenderState( D3DRS_FOGEND, *((DWORD*)&normdist) ) );
	}
	else if( !IsNegative( m_fogFarDist ) )
	{
		TRYDX( m_pD3DDevice->SetRenderState( D3DRS_FOGEND, *((DWORD*)&m_fogFarDist) ) );
	}
}

void Rapi::SetFogDistances( const float near_dist, const float far_dist )
{
	if( far_dist <= near_dist )
	{
		gperror( "Fog far distance cannot be less than or equal to fog near distance!" );
		return;
	}

	// Save the new fog distances
	m_fogNearDist	= max( 0.0f, near_dist );
	m_fogFarDist	= max( 0.1f, far_dist );

	// Call the set function
	SetFogNearDist( m_fogNearDist );
	SetFogFarDist( m_fogFarDist );
}

float Rapi::GetFogFarDist( )
{
	return m_fogFarDist;
}

float Rapi::GetClipSpaceFogFarDist( )
{
	return 1.0f;
}

// Return a far clip that takes into account whether of not there is
// fog limiting the distance we can see
float Rapi::GetClipFarDistAdjustedForFog( )
{
	if( m_bFogIsActive )
	{ 
		if( !IsZero( m_fogFarDist ) && !IsZero( m_clipToFogRatio ) )
		{
			return( max( m_clipNearDist + 0.1f, m_fogFarDist * m_clipToFogRatio ) ); 
		}
		else
		{
			return m_clipFarDist;
		}
	}
	else
	{
		return m_clipFarDist;
	}
}

// Clipping planes
void Rapi::SetClipFarDist( const float &dist )
{
	if( dist < m_clipNearDist )
	{
		gpwarningf(( "The current Near Clip distance [%f] exceeds the requested Far Clip distance [%f], ignoring change", m_clipNearDist, dist ));
		return;
	}

	m_clipFarDist = dist;

	float normdist;
	
	if( m_fogType == RAPI_FOGTYPE_ZFOG )
	{
		normdist = Minimum( 1.0f, m_fogNearDist/GetClipFarDistAdjustedForFog() );
		if( !IsNegative( normdist ) )
		{
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_FOGSTART, *((DWORD*)&normdist) ));
		}
		normdist = 1.0f;
		TRYDX( m_pD3DDevice->SetRenderState( D3DRS_FOGEND, *((DWORD*)&normdist) ));
	}
}

void Rapi::SetClipNearDist( const float &dist )
{
	if( dist > m_clipFarDist )
	{
		gpwarningf(( "The requested Near Clip distance [%f] exceeds the current Far Clip distance [%f], ignoring change", dist, m_clipFarDist ));
		return;
	}

	m_clipNearDist = dist;
}



// Add a new font to the system
RapiFont* Rapi::CreateFont( HFONT font, bool forceBitmap )
{
	// Create new font
	HDC hdc	= GetDC( m_hwnd );
	RapiFont* pFont	= new RapiFont( this, hdc, font, forceBitmap );
	ReleaseDC( m_hwnd, hdc );

	// Insert font into our list
	m_Fonts.insert( pFont );

	// We are done here
	return pFont;
}
RapiFont* Rapi::CreateFont( unsigned int texture,
							gpstring sFont,
							UINT32 characterHeight,
							UINT32 startRange,
							UINT32 endRange )
{
	RapiFont* pFont = new RapiFont( this, texture, characterHeight, startRange, endRange, sFont );

	// Insert font into our list
	m_Fonts.insert( pFont );

	// We are done here
	return pFont;
}

/*
// Remove a font from the system
void Rapi::DestroyFont( RapiFont* pFont )
{
	FontIter i = m_Fonts.find( pFont );
	m_Fonts.erase( i );
	delete pFont;
	pFont	= NULL;
}
*/
// Remove this font from our internal listing
void Rapi::RemoveFromFontList( RapiFont* pFont )
{
	gpassert( pFont );
	if( pFont ){
		FontIter i = m_Fonts.find( pFont );
		m_Fonts.erase( i );
	}
}

// Remove this light from our internal listing
void Rapi::RemoveFromLightList( RapiLight* pLight )
{
	UNREFERENCED_PARAMETER( pLight );
//	gpassert( pLight );
//	if( pLight ){
//		LightIter i = m_Lights.find( pLight );
//		m_Lights.erase( i );
//	}
}

// Go through the global light list and find an empty slot
inline DWORD Rapi::FindEmptyLightSlot()
{
	for( unsigned int i = 0; i < LIGHT_CACHE; ++i ){
		if( !m_Lights[ i ] ){
			// We found an empty slot
			return i;
		}
	}

	gpassertm( 0, "There are no empty light slots!  If you see this, go talk to James immediately!" );
	return 0;
}

// Viewport modification
bool Rapi::SetViewport( float norm_width, float norm_height )
{
	gpassert( norm_width <= 1.0f && norm_width >= 0.0f );
	gpassert( norm_height <= 1.0f && norm_height >= 0.0f );

	m_viewWidth				= FTOL( (float)m_Width * norm_width );
	m_viewHeight			= FTOL( (float)m_Height * norm_height );

	m_viewWidthOffset		= (m_Width - m_viewWidth) / 2;
	m_viewHeightOffset		= (m_Height - m_viewHeight) / 2;

	D3DVIEWPORT8 vp = { m_viewWidthOffset, m_viewHeightOffset, m_viewWidth, m_viewHeight, 0.0f, 1.0f };
	if( FAILED( m_pD3DDevice->SetViewport( &vp ) ) )
	{
		gpdebugger( "Could not set the viewport for the current device!\n" );
		return false;
	}

	return true;
}

// Viewport query
bool Rapi::GetViewport( float& norm_width, float& norm_height )
{
	D3DVIEWPORT8 vp;
	if( FAILED( m_pD3DDevice->GetViewport( &vp ) ) )
	{
		gpdebugger( "Could not get the viewport for the current device!\n" );
		return false;
	}

	norm_width  = (float)vp.Width  / m_Width;
	norm_height = (float)vp.Height / m_Height;

	return true;
}

// Band rendering - necessary to avoid fog
void Rapi::DrawViewportBands( DWORD color )
{
	// easy out
	UINT oldViewHeight = m_viewHeight;
	UINT oldViewWidth = m_viewWidth;
	if ( (m_Height == oldViewHeight) && (m_Width == oldViewWidth) )
	{
		return;
	}

	// temporarily set viewport to full screen to avoid clipping out bands
	float oldWidth, oldHeight;
	GetViewport( oldWidth, oldHeight );
	SetViewport( 1.0, 1.0 );

	// draw top & bottom
	if ( m_Height != oldViewHeight )
	{
		UINT border = (m_Height - oldViewHeight) / 2;
		RP_DrawFilledRect( *this, 0, 0, m_Width, border, color );
		RP_DrawFilledRect( *this, 0, m_Height - border, m_Width, m_Height, color );
	}

	// draw left & right
	if ( m_Width != oldViewWidth )
	{
		UINT border = (m_Width - oldViewWidth) / 2;
		RP_DrawFilledRect( *this, 0, 0, border, m_Height, color );
		RP_DrawFilledRect( *this, m_Width - border, 0, m_Width, m_Height, color );
	}

	// restore viewport
	SetViewport( oldWidth, oldHeight );
}

void Rapi::SetColorControl( bool colorCont )
{
	m_bColorControl = colorCont;
	if( m_bColorControl )
	{
		BuildGammaTable();
	}
}

// Set texture as render target
bool Rapi::SetTextureAsRenderTarget( unsigned int tex )
{
	gpassert( m_Textures.IsValid( tex ) );

	// Get the texture and set it as the current render target
	RapiTexture* pTexture	= m_Textures[ tex ];
	if( pTexture )
	{
		return( pTexture->SetAsRenderTarget() );
	}
	else
	{
		return false;
	}
}

// Set the render target back to the primary surface
void Rapi::ResetRenderTarget()
{
	// Get the old render target
	LPDIRECT3DSURFACE8 pTarget;
	m_pD3DDevice->GetRenderTarget( &pTarget );
	pTarget->Release();

	if( m_pBackBuffer && m_pBackBuffer != pTarget )
	{
		// Set the render target back
		TRYDX( m_pD3DDevice->SetRenderTarget( m_pBackBuffer, m_pZBuffer ) );
	}
/*
	// Clear the render target
	ClearBackBuffer( NULL, m_ClearColor );
*/
}

// Change the currently active projection
void Rapi::SetLightProjectionActive()
{
	// Set the new viewport
	D3DVIEWPORT8 viewPort;
	TRYDX( m_pD3DDevice->GetViewport( &viewPort ) );
	viewPort.X		= 0;
	viewPort.Y		= 0;
	viewPort.Width	= m_shadowTexSize;
	viewPort.Height	= m_shadowTexSize;

	TRYDX( m_pD3DDevice->SetViewport( &viewPort ) );

	// Set the transformations to the light
	TRYDX( m_pD3DDevice->SetTransform( D3DTS_PROJECTION,	&m_LightProj ) );
	TRYDX( m_pD3DDevice->SetTransform( D3DTS_VIEW,			&m_LightView ) );
}

// Change the currently active projection
void Rapi::SetObserverProjectionActive()
{
	// Set the new viewport
	D3DVIEWPORT8 viewPort;
	TRYDX( m_pD3DDevice->GetViewport( &viewPort ) );
	viewPort.X		= m_viewWidthOffset;
	viewPort.Y		= m_viewHeightOffset;
	viewPort.Width	= m_viewWidth;
	viewPort.Height	= m_viewHeight;

	TRYDX( m_pD3DDevice->SetViewport( &viewPort ) );

	// Set the transformations to observer
	TRYDX( m_pD3DDevice->SetTransform( D3DTS_PROJECTION,	&m_ObserverProj ) );
	TRYDX( m_pD3DDevice->SetTransform( D3DTS_VIEW,			&m_ObserverView ) );
}

void Rapi::SetObserverZbiasProjectionActive()
{
	TRYDX( m_pD3DDevice->SetTransform( D3DTS_PROJECTION,	&m_ObserverZbiasProj ) );
}

void Rapi::ResetObserverZbiasProjection()
{
	TRYDX( m_pD3DDevice->SetTransform( D3DTS_PROJECTION,	&m_ObserverProj ) );
}

// Manually transform the passed verts using the current transforms
unsigned int Rapi::TransformAndLockVertices( const unsigned int vBufferStart, const unsigned int numVerts, tVertex*& pVerts )
{
	if( m_bTVBufferLocked )
	{
		gperror( "Cannot transform and lock because transformed vertex buffer is already locked!" );
		return 0;
	}

	unsigned int tIndex		= 0;
	if( m_TVBufferPosition + numVerts < SIZE_TVBUFFER )
	{
		// Use the current position
		tIndex				= m_TVBufferPosition;
		m_TVBufferPosition	+= numVerts;
	}
	else if( numVerts <= SIZE_TVBUFFER )
	{
		// Reset the position and index
		m_TVBufferPosition	= numVerts;
	}
	else
	{
		gperrorf(( "Requested transform of %d vertices, which is larger than alloted buffer size!", numVerts ));
		return 0;
	}

	// Setup the source info
	m_pD3DDevice->SetPixelShader( NULL );
	m_pD3DDevice->SetVertexShader( SVERTEX );
	m_pD3DDevice->SetStreamSource( 0, m_pVBuffer, sizeof( sVertex ) );

	// Process the vertices
	TRYDX( m_pD3DDevice->ProcessVertices( vBufferStart, tIndex, numVerts, m_pTVBuffer, 0 ) );

	// Lock the buffer
	if( SUCCEEDED( TRYDX( m_pTVBuffer->Lock( tIndex * sizeof( tVertex ), numVerts * sizeof( tVertex ),
											 (BYTE**)&pVerts, D3DLOCK_NOSYSLOCK ) ) ) )
	{
		// Indicate that we have locked the vertex buffer
		m_bTVBufferLocked	= true;

		// Return the index
		return tIndex;
	}
	else
	{
		return 0xFFFFFFFF;
	}
}

// Get the 2D screen coordinates of a 3D world position
vector_3 Rapi::GetScreenPosition( const vector_3& worldPos )
{
	// Build the combined observer view and projection matrix
	D3DMATRIX combined;
	D3DMath_MatrixMultiply( combined, m_ObserverView, m_ObserverProj );

	// Calculate the half height and width for our screen conversion
	const float half_vw	= m_viewWidth * 0.5f;
	const float half_vh	= m_viewHeight * 0.5f;

	// Calculate the inverse w component
	float rhw	= INVERSEF( (worldPos.x*combined._14) + (worldPos.y*combined._24) + (worldPos.z*combined._34) + combined._44 );
	DEBUG_FLOAT_VALIDATE( rhw );

	// Project the vert into screen coordinates
	vector_3 vDest( DoNotInitialize );
	vDest.x		= m_viewWidthOffset  + ((1.0f + (((worldPos.x*combined._11) + (worldPos.y*combined._21) + (worldPos.z*combined._31) + combined._41) * rhw) ) * half_vw);
	vDest.y		= m_viewHeightOffset + ((1.0f - (((worldPos.x*combined._12) + (worldPos.y*combined._22) + (worldPos.z*combined._32) + combined._42) * rhw) ) * half_vh);
	vDest.z		= 0.0f;

	// Return our result
	return vDest;
}

// Transforms the given vector by the texture matrix
void Rapi::GetWorldTextureMatrix( D3DMATRIX& mat )
{
	// Get the texture and world transform
	D3DMATRIX texture, world;
	TRYDX( m_pD3DDevice->GetTransform( D3DTS_TEXTURE0, &texture ) );
	TRYDX( m_pD3DDevice->GetTransform( D3DTS_WORLD, &world ) );

	// Combine them and return
	D3DMath_MatrixMultiply( mat, world, texture );
}

// Get the current world*view matrix
void Rapi::GetWorldViewMatrix( D3DMATRIX& mat )
{
	// Get the current world matrix
	D3DMATRIX world;
	TRYDX( m_pD3DDevice->GetTransform( D3DTS_WORLD, &world ) );

	// Multiply it by the current view
	D3DMath_MatrixMultiply( mat, world, m_ObserverView );
}

// Get the current world matrix
void Rapi::GetWorldMatrix( D3DMATRIX& mat )
{
	TRYDX( m_pD3DDevice->GetTransform( D3DTS_WORLD, &mat ) );
}

// Get the current texture matrix
void Rapi::GetTextureMatrix( D3DMATRIX& mat )
{
	TRYDX( m_pD3DDevice->GetTransform( D3DTS_TEXTURE0, &mat ) );
}

// Build a view matrix.  Result is returned in mView.
void Rapi::BuildViewMatrix( const vector_3& vFrom, const vector_3& vAt, const vector_3& vWorldUp, D3DMATRIX& mView )
{
	ZeroMemory( &mView, sizeof( D3DMATRIX ) );

	// Get the z basis vector, which points straight ahead. This is the
	// difference from the eyepoint to the lookat point.
	vector_3 vView	= vAt - vFrom;
	if( vView.IsZero() )
	{
		vView.y	= -1.0f;
	}

	// Normalize the z basis vector
	vView.Normalize();

	// Get the dot product, and calculate the projection of the z basis
	// vector onto the up vector. The projection is the y basis vector.
	vector_3 vUp	= vWorldUp - (vWorldUp.DotProduct( vView ) * vView);

	if( vUp.IsZero() )
	{
		vUp.Set( 0.0f, 0.0f, 1.0f );
	}
	else
	{
		// Normalize the y basis vector
		vUp.Normalize();
	}

	// The x basis vector is found simply with the cross product of the y
	// and z basis vectors
	vector_3 vRight = CrossProduct( vView, vUp );

	// Start building the matrix. The first three rows contains the basis
	// vectors used to rotate the view to point at the lookat point
	mView._11 = vRight.x;    mView._12 = vUp.x;    mView._13 = vView.x;
	mView._21 = vRight.y;    mView._22 = vUp.y;    mView._23 = vView.y;
	mView._31 = vRight.z;    mView._32 = vUp.z;    mView._33 = vView.z;

	// Do the translation values (rotations are still about the eyepoint)
	mView._41 = -vFrom.DotProduct( vRight );
	mView._42 = -vFrom.DotProduct( vUp );
	mView._43 = -vFrom.DotProduct( vView );

	// Set the corner of the matrix
	mView._44 = 1.0f;
}

// Build a projection matrix.  Result is returned in mPersp.
void Rapi::BuildPerspectiveMatrix( const float fFOV, const float fAspect, const float fNearPlane, const float fFarPlane, D3DMATRIX& mPersp )
{
	ZeroMemory( &mPersp, sizeof( D3DMATRIX ) );

	// Get the half of the field of view (in width)
	float fh	= fFOV * 0.5f;

	// Calculate w by taking the cotangent of fh
	float w, s;
	SINCOSF( fh, s, w );
	w			/= s;

	// Calculate h by taking w and correcting for the aspect ratio
	float h		= fAspect * w;

	// Calculate the distance component
	float Q;
	if( fNearPlane == fFarPlane )
	{
		Q	= fNearPlane;
	}
	else
	{
		Q	= fFarPlane / ( fFarPlane - fNearPlane );
	}

	// Setup the matrix
	mPersp._11 = w;
	mPersp._22 = h;
	mPersp._33 = Q;
	mPersp._34 = 1.0f;
	mPersp._43 = (-Q*fNearPlane);
}

// Build a orthographic matrix.  Result is returned in mOrtho.
void Rapi::BuildOrthoMatrix( const float fWidth, const float fHeight, const float fNearPlane, const float fFarPlane, D3DMATRIX& mOrtho )
{
	ZeroMemory( &mOrtho, sizeof( D3DMATRIX ) );

	// Use the requested width and height to determine projections
	mOrtho._11 = 2.0f / fWidth;
	mOrtho._22 = 2.0f / fHeight;

	// Correct for Z dist
	mOrtho._33 = -1.0f / (fNearPlane - fFarPlane);
	mOrtho._43 = fNearPlane * -mOrtho._33;
	mOrtho._44 = 1.0f;
}

// Texture projection matrix
void Rapi::SetTextureProjectionMatrix()
{
	// Build a basic texture projection matrix
	D3DMATRIX mat;
	BuildTextureProjectionMatrix( mat );
	SetTextureProjectionMatrix( mat );
}

void Rapi::SetTextureProjectionMatrix( D3DMATRIX& mat )
{
	TRYDX( m_pD3DDevice->SetTransform( D3DTS_TEXTURE0, &mat ) );

	if( !m_bManualTextureProjection )
	{
		// If we are not in a manual projection situation, we need to add the inverse view matrix
		D3DMATRIX invV;
		D3DMath_MatrixInvert( invV, m_ObserverView );
		TRYDX( m_pD3DDevice->MultiplyTransform( D3DTS_TEXTURE0, &invV ) );
	}
}

void Rapi::BuildTextureProjectionMatrix( D3DMATRIX& mat, bool bYFlip )
{
	// Build a scale and bias matrix
	D3DMATRIX matTexScale;
	ZeroMemory( &matTexScale, sizeof( D3DMATRIX ) );

	matTexScale._11	= 0.5f;					// scale +/-1.0 to +/- 0.5
	matTexScale._22	= bYFlip ? -0.5f : 0.5f;// scale to +/-0.5, flip sense
	matTexScale._33	= 0.0f;
	matTexScale._41	= 0.5f; 				// translate +/-0.5 to 0.0-1.0
	matTexScale._42	= 0.5f; 				// translate +/-0.5 to 0.0-1.0
	matTexScale._43	= 1.0f; 				// don't modify z
	matTexScale._44	= 1.0f;

	// Build the following:
	// T = InvV * V_light * Perspective_light * TexScale
	D3DMATRIX tempMat;
	D3DMath_MatrixMultiply( tempMat, m_LightProj, matTexScale );
	D3DMath_MatrixMultiply( mat, m_LightView, tempMat );
}

bool Rapi::SetLightProjection( const vector_3& object_position, const vector_3& eye_position, const float object_radius,
							   float& angle, float& nearplane, float& farplane )
{
	vector_3 direction	= object_position - eye_position;
	float distance		= direction.Length();

	if( distance < object_radius )
	{
		return false;
	}

	angle		= 2.0f * ASINF( (object_radius / distance) );
	nearplane	= distance - object_radius;
	farplane	= distance + object_radius;

	BuildViewMatrix( eye_position, object_position, vector_3::UP, m_LightView );
	BuildPerspectiveMatrix( angle, 1.0f, nearplane, farplane + 15.0f, m_LightProj );

	return true;
}

void Rapi::SetLightProjection( const vector_3& origin, const matrix_3x3& orientation, const float angle,
							   const float aspect, const float nearplane, const float farplane )
{
	BuildViewMatrix( origin, origin + orientation.GetColumn2_T(), Normalize( orientation.GetColumn1_T() ), m_LightView );
	BuildPerspectiveMatrix( angle, aspect, nearplane, farplane, m_LightProj );
}

// Generate and set the projection matrix
void Rapi::SetPerspectiveMatrix( const float fFOV, const float fNearPlane, const float fFarPlane )
{
	// Store current field of view
	m_ObserverFOV	= fFOV;

	// Calculate the bias we need
	float fBias		= (fFarPlane - fNearPlane) * MANUALZBIAS;

	// Build a new projection matrix
	BuildPerspectiveMatrix( m_ObserverFOV, ((float)m_viewWidth/(float)m_viewHeight), fNearPlane, fFarPlane, m_ObserverProj );
	BuildPerspectiveMatrix( m_ObserverFOV, ((float)m_viewWidth/(float)m_viewHeight), fNearPlane + fBias, fFarPlane, m_ObserverZbiasProj );

	TRYDX( m_pD3DDevice->SetTransform( D3DTS_PROJECTION, &m_ObserverProj ) );
}

// Generate and set the projection matrix
void Rapi::SetOrthoMatrix( const float fMetersPerPixel, const float fNearPlane, const float fFarPlane )
{
	// Calculate the bias we need
	float fBias		= (fFarPlane - fNearPlane) * MANUALZBIAS;

	// Build a new projection matrix
	BuildOrthoMatrix( fMetersPerPixel * m_viewWidth, fMetersPerPixel * m_viewHeight, fNearPlane, fFarPlane, m_ObserverProj );
	BuildOrthoMatrix( fMetersPerPixel * m_viewWidth, fMetersPerPixel * m_viewHeight, fNearPlane + fBias, fFarPlane, m_ObserverZbiasProj );

	TRYDX( m_pD3DDevice->SetTransform( D3DTS_PROJECTION, &m_ObserverProj ) );
}

// Generate and set the view matrix
void Rapi::SetViewMatrix( const vector_3& vFrom, const vector_3& vAt, const vector_3& vWorldUp )
{
	// Build a new view matrix
	BuildViewMatrix( vFrom, vAt, vWorldUp, m_ObserverView );

	TRYDX( m_pD3DDevice->SetTransform( D3DTS_VIEW, &m_ObserverView ) );
}

// Create world matrix
void Rapi::SetWorldMatrix( const matrix_3x3& mMat, const vector_3& vTrans )
{
	// Take a 3x3 matrix and a position and combine them into a matrix that D3D will like
	D3DMATRIX mat;
	ZeroMemory( &mat, sizeof( D3DMATRIX ) );

	mat._11	= mMat.v00;
	mat._12 = mMat.v10;
	mat._13 = mMat.v20;

	mat._21 = mMat.v01;
	mat._22 = mMat.v11;
	mat._23 = mMat.v21;

	mat._31 = mMat.v02;
	mat._32 = mMat.v12;
	mat._33 = mMat.v22;

	mat._41 = vTrans.x;
	mat._42 = vTrans.y;
	mat._43 = vTrans.z;

	mat._44 = 1.0f;

	TRYDX( m_pD3DDevice->SetTransform( D3DTS_WORLD, &mat ) );
}

void Rapi::SetWorldMatrixTranslation( const vector_3& vTrans )
{
	D3DMATRIX mat;
	TRYDX( m_pD3DDevice->GetTransform( D3DTS_WORLD, &mat ) );

	mat._41 = vTrans.x;
	mat._42 = vTrans.y;
	mat._43 = vTrans.z;

	TRYDX( m_pD3DDevice->SetTransform( D3DTS_WORLD, &mat ) );

}

void Rapi::SetWorldMatrixRotation( const matrix_3x3& mMat )
{
	D3DMATRIX mat;
	TRYDX( m_pD3DDevice->GetTransform( D3DTS_WORLD, &mat ) );

	mat._11	= mMat.v00;
	mat._12 = mMat.v10;
	mat._13 = mMat.v20;

	mat._21 = mMat.v01;
	mat._22 = mMat.v11;
	mat._23 = mMat.v21;

	mat._31 = mMat.v02;
	mat._32 = mMat.v12;
	mat._33 = mMat.v22;

	TRYDX( m_pD3DDevice->SetTransform( D3DTS_WORLD, &mat ) );

}

void Rapi::TranslateWorldMatrix( const vector_3& vTrans )
{
	D3DMATRIX mat;
	ZeroMemory( &mat, sizeof( D3DMATRIX ) );

	mat._11	= 1.0f;
	mat._22 = 1.0f;
	mat._33 = 1.0f;
	mat._44 = 1.0f;

	mat._41 = vTrans.x;
	mat._42 = vTrans.y;
	mat._43 = vTrans.z;

	TRYDX( m_pD3DDevice->MultiplyTransform( D3DTS_WORLD, &mat ) );
}

void Rapi::RotateWorldMatrix( const matrix_3x3& mMat )
{
	D3DMATRIX mat;
	ZeroMemory( &mat, sizeof( D3DMATRIX ) );

	mat._11	= mMat.v00;
	mat._12 = mMat.v10;
	mat._13 = mMat.v20;

	mat._21 = mMat.v01;
	mat._22 = mMat.v11;
	mat._23 = mMat.v21;

	mat._31 = mMat.v02;
	mat._32 = mMat.v12;
	mat._33 = mMat.v22;

	mat._44 = 1.0f;

	TRYDX( m_pD3DDevice->MultiplyTransform( D3DTS_WORLD, &mat ) );
}

void Rapi::ScaleWorldMatrix( const vector_3& vScale )
{
	D3DMATRIX mat;
	ZeroMemory( &mat, sizeof( D3DMATRIX ) );

	mat._11	= vScale.x;
	mat._22 = vScale.y;
	mat._33 = vScale.z;
	mat._44 = 1.0f;

	TRYDX( m_pD3DDevice->MultiplyTransform( D3DTS_WORLD, &mat ) );
}

void Rapi::PushWorldMatrix( const matrix_3x3& mMat, const vector_3& vTrans )
{
	// Save off the current matrix
	D3DMATRIX mat;
	TRYDX( m_pD3DDevice->GetTransform( D3DTS_WORLD, &mat ) );

	m_MatrixStack.push_back( mat );

	// Take a 3x3 matrix and a position and combine them into a matrix that D3D will like
	ZeroMemory( &mat, sizeof( D3DMATRIX ) );

	mat._11	= mMat.v00;
	mat._12 = mMat.v10;
	mat._13 = mMat.v20;

	mat._21 = mMat.v01;
	mat._22 = mMat.v11;
	mat._23 = mMat.v21;

	mat._31 = mMat.v02;
	mat._32 = mMat.v12;
	mat._33 = mMat.v22;

	mat._41 = vTrans.x;
	mat._42 = vTrans.y;
	mat._43 = vTrans.z;

	mat._44 = 1.0f;

	TRYDX( m_pD3DDevice->SetTransform( D3DTS_WORLD, &mat ) );
}

void Rapi::PushWorldMatrix()
{
	// Save off the current matrix
	D3DMATRIX mat;
	TRYDX( m_pD3DDevice->GetTransform( D3DTS_WORLD, &mat ) );
	m_MatrixStack.push_back( mat );

	// Sanity check!
	gpassert( m_MatrixStack.size() <= MATRIXSTACKSIZE );
}

void Rapi::PopWorldMatrix()
{
	gpassert( m_MatrixStack.size() );

	// Restore the world matrix to the top member of the matrix stack
	TRYDX( m_pD3DDevice->SetTransform( D3DTS_WORLD, &m_MatrixStack.back() ) );

	// Pop the stack
	m_MatrixStack.pop_back();
}

void Rapi::BuildCombinedTransformationMatrix( bool bZbias )
{
	// Get the current world matrix
	D3DMATRIX mat;
	TRYDX( m_pD3DDevice->GetTransform( D3DTS_WORLD, &mat ) );

	// Build combined transform matrix for manual transformation
	D3DMATRIX combined;

	// Get the current zbias
	if( bZbias )
	{
		D3DMath_MatrixMultiply( combined, m_ObserverView, m_ObserverZbiasProj );
	}
	else
	{
		D3DMath_MatrixMultiply( combined, m_ObserverView, m_ObserverProj );
	}

	D3DMath_MatrixMultiply( m_ObserverComb, mat, combined );
}

bool Rapi::EnableStencilBuffer( unsigned int reference )
{
	// Return if we have no stencil buffer
	if( !m_stencil )
	{
		return false;
	}

	// Figure out how to enable the stencil buffer
    TRYDX (m_pD3DDevice->SetRenderState( D3DRS_STENCILENABLE, TRUE ));

    TRYDX (m_pD3DDevice->SetRenderState( D3DRS_STENCILFUNC,			D3DCMP_GREATER ));
    TRYDX (m_pD3DDevice->SetRenderState( D3DRS_STENCILZFAIL,		D3DSTENCILOP_KEEP ));
    TRYDX (m_pD3DDevice->SetRenderState( D3DRS_STENCILFAIL,			D3DSTENCILOP_KEEP ));

	if( m_stencil == 1 )
	{
		// One bit stencil
        TRYDX (m_pD3DDevice->SetRenderState( D3DRS_STENCILREF,		0x1 ));
        TRYDX (m_pD3DDevice->SetRenderState( D3DRS_STENCILMASK,		0x1 ));
        TRYDX (m_pD3DDevice->SetRenderState( D3DRS_STENCILWRITEMASK,0x1 ));
        TRYDX (m_pD3DDevice->SetRenderState( D3DRS_STENCILPASS,		D3DSTENCILOP_REPLACE ));
	}
	else
	{
		// Normal stencil
        TRYDX (m_pD3DDevice->SetRenderState( D3DRS_STENCILREF,		reference ));
        TRYDX (m_pD3DDevice->SetRenderState( D3DRS_STENCILMASK,		0xffffffff ));
        TRYDX (m_pD3DDevice->SetRenderState( D3DRS_STENCILWRITEMASK,0xffffffff ));
        TRYDX (m_pD3DDevice->SetRenderState( D3DRS_STENCILPASS,		D3DSTENCILOP_REPLACE ));
	}

	return true;
}

void Rapi::DisableStencilBuffer()
{
    TRYDX (m_pD3DDevice->SetRenderState( D3DRS_STENCILENABLE, FALSE ));
}

// Helper to determine the number of primitives from the number of verts for any given type
DWORD Rapi::GetNumPrimitives( DP_TYPE pType, DWORD numVertices )
{
	switch( pType )
	{
	case D3DPT_POINTLIST:
		return( numVertices );
		break;

	case D3DPT_LINELIST:
		return( numVertices / 2 );
		break;

	case D3DPT_LINESTRIP:
		return( numVertices - 1 );
		break;

	case D3DPT_TRIANGLELIST:
		return( numVertices / 3 );
		break;

	case D3DPT_TRIANGLESTRIP:
	case D3DPT_TRIANGLEFAN:
		return( numVertices - 2 );
		break;
	};

	return 0;
}

// Draw primitive routine
void Rapi::DrawPrimitive(	DP_TYPE			pType,			// Type of primitive to draw
							void*			pVertices,		// Vertices to draw
							unsigned int	numVertices,	// Number of vertices in the list
							unsigned int	vertType,		// Type of vertices
							unsigned int*	pTex,			// Textures to draw with.  Set to NULL if no texture is needed.
							unsigned int	numTex )
{
	gpassert( (numVertices > 0) );
	gpassertm( m_bInScene, "Primitive being drawn outside of 3D scene!" );

	// Do the texture setup
	if( pTex && numTex )
	{
		for( unsigned int i = 0; i < numTex; ++i )
		{
			SetTexture( i, pTex[i] );
		}
	}
	else
	{
		SetTexture( 0, NULL );
		SetTexture( 1, NULL );
	}

	if( !m_bNoRender )
	{
		unsigned int startIndex = 0;
		m_pD3DDevice->SetPixelShader( NULL );

		if( vertType == SVERTEX )
		{
			sVertex* pVerts = NULL;
			m_pD3DDevice->SetVertexShader( SVERTEX );
			m_pD3DDevice->SetStreamSource( 0, m_pVBuffer, sizeof( sVertex ) );
			if( LockBufferedVertices( numVertices, startIndex, pVerts ) )
			{
				memcpy( pVerts, pVertices, sizeof( sVertex ) * numVertices );
				UnlockBufferedVertices();
			}
		}
		if( vertType == TVERTEX )
		{
			tVertex* pVerts = NULL;
			m_pD3DDevice->SetVertexShader( TVERTEX );
			m_pD3DDevice->SetStreamSource( 0, m_pTVBuffer, sizeof( tVertex ) );
			if( LockBufferedTVertices( numVertices, startIndex, pVerts ) )
			{
				memcpy( pVerts, pVertices, sizeof( tVertex ) * numVertices );
				UnlockBufferedTVertices();
			}
		}

		// Draw the first pass
		TRYDX( m_pD3DDevice->DrawPrimitive( pType, startIndex, GetNumPrimitives( pType, numVertices ) ) );

		// Test to see if we need to draw more passes
		if( numTex == 1 )
		{
			RapiTexture* pTexture	= m_ActiveTex[ 0 ];
			if( pTexture && pTexture->IsMultiPass() )
			{
				// Disable the zbuffer
				TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		FALSE ));

				// Draw each pass
				for( unsigned int i = 1; i < pTexture->m_numPasses; ++i )
				{
					pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, i );
					TRYDX( m_pD3DDevice->DrawPrimitive( pType, startIndex, GetNumPrimitives( pType, numVertices ) ) );
				}

				// Reset the texture
				// This will, most times, be a waste of state changes.  Unfortunately, with the potential
				// mix of primitive drawing calls available to the user, there's no easy way to assure that we
				// are on the first texture pass unless we add a check to every draw call.  Since multipass is
				// so rare, it seems to me that doing the extra switches only on multipass stuff and relieving
				// the other primitive calls of having to check for it is a better way to go.
				pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, 0 );

				// Enable the zbuffer
				TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		TRUE ));
			}
		}
	}

#if !GP_RETAIL

	m_VerticesTransformed	+= numVertices;
	if( (pType == D3DPT_TRIANGLELIST || pType == D3DPT_TRIANGLESTRIP) || pType == D3DPT_TRIANGLEFAN )
	{
		m_TrianglesDrawn += numVertices/3;
	}

#endif
}

void Rapi::DrawIndexedPrimitive(	DP_TYPE			pType,		// Type of primitive to draw
									void*			pVertices,	// Vertices to use
									unsigned int	numVertices,// Number of vertices in the list
									unsigned int	vertType,	// Type of vertices
									unsigned short*	pIndices,	// Indices to draw with
									unsigned int	numIndices,	// Number of indices
									unsigned int*	pTex,		// Textures to use
									unsigned int	numTex )
{
	gpassert( numVertices > 0 );
	gpassert( numIndices > 0 );
	gpassertm( m_bInScene, "Primitive being drawn outside of 3D scene!" );

	// Do the texture setup
	if( pTex && numTex )
	{
		for( unsigned int i = 0; i < numTex; ++i )
		{
			SetTexture( i, pTex[i] );
		}
	}
	else
	{
		SetTexture( 0, NULL );
		SetTexture( 1, NULL );
	}
	if( !m_bNoRender )
	{
		unsigned int startIndex = 0;
		m_pD3DDevice->SetPixelShader( NULL );

		if( vertType == SVERTEX )
		{
			sVertex* pVerts = NULL;
			m_pD3DDevice->SetVertexShader( SVERTEX );
			if( LockBufferedVertices( numVertices, startIndex, pVerts ) )
			{
				memcpy( pVerts, pVertices, sizeof( sVertex ) * numVertices );
				UnlockBufferedVertices();
			}
			m_pD3DDevice->SetStreamSource( 0, m_pVBuffer, sizeof( sVertex ) );
		}
		if( vertType == TVERTEX )
		{
			tVertex* pVerts = NULL;
			m_pD3DDevice->SetVertexShader( TVERTEX );
			if( LockBufferedTVertices( numVertices, startIndex, pVerts ) )
			{
				memcpy( pVerts, pVertices, sizeof( tVertex ) * numVertices );
				UnlockBufferedTVertices();
			}
			m_pD3DDevice->SetStreamSource( 0, m_pTVBuffer, sizeof( tVertex ) );
		}

		unsigned int indexStart = 0;
		WORD* pBufIndices	= NULL;
		if( LockIndexBuffer( numIndices, indexStart, pBufIndices ) )
		{
			memcpy( pBufIndices, pIndices, sizeof( WORD ) * numIndices );
			UnlockIndexBuffer();
		}
		m_pD3DDevice->SetIndices( m_pIndexBuffer, startIndex );

		TRYDX( m_pD3DDevice->DrawIndexedPrimitive( pType, 0, numVertices, indexStart, GetNumPrimitives( pType, numIndices ) ) );

		// Test to see if we need to draw more passes
		if( numTex == 1 )
		{
			RapiTexture* pTexture	= m_ActiveTex[ 0 ];
			if( pTexture && pTexture->IsMultiPass() )
			{
				// Disable the zbuffer
				TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		FALSE ));

				// Draw each pass
				for( unsigned int i = 1; i < pTexture->m_numPasses; ++i )
				{
					pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, i );
					TRYDX( m_pD3DDevice->DrawIndexedPrimitive( pType, 0, numVertices, indexStart, GetNumPrimitives( pType, numIndices ) ) );
				}

				// Reset the texture
				// This will, most times, be a waste of state changes.  Unfortunately, with the potential
				// mix of primitive drawing calls available to the user, there's no easy way to assure that we
				// are on the first texture pass unless we add a check to every draw call.  Since multipass is
				// so rare, it seems to me that doing the extra switches only on multipass stuff and relieving
				// the other primitive calls of having to check for it is a better way to go.
				pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, 0 );

				// Enable the zbuffer
				TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		TRUE ));
			}
		}
	}

#if !GP_RETAIL

	m_VerticesTransformed	+= numVertices;
	if( (pType == D3DPT_TRIANGLELIST || pType == D3DPT_TRIANGLESTRIP) || pType == D3DPT_TRIANGLEFAN )
	{
		m_TrianglesDrawn += numVertices/3;
	}

#endif
}

bool Rapi::LockBufferedVertices( const unsigned int numVerts, unsigned int& startIndex, sVertex*& pVerts )
{
	if( m_bVBufferLocked )
	{
		gperror( "Cannot lock a vertex buffer that is already locked!" );
		return false;
	}

	if( m_VBufferPosition + numVerts < SIZE_VBUFFER )
	{
		// Lock the buffer
		if( SUCCEEDED( TRYDX( m_pVBuffer->Lock( m_VBufferPosition * sizeof( sVertex ), numVerts * sizeof( sVertex ),
												(BYTE**)&pVerts, D3DLOCK_NOOVERWRITE | D3DLOCK_NOSYSLOCK ) ) ) )
		{
			// Set the index and increment to the new position
			startIndex			= m_VBufferPosition;
			m_VBufferPosition	+= numVerts;
			m_bVBufferLocked	= true;

			return true;
		}
		else
		{
			return false;
		}
	}
	else if( numVerts <= SIZE_VBUFFER )
	{
		// Reset the position and index
		m_VBufferPosition		= numVerts;
		startIndex				= 0;

		// Lock the buffer
		if( SUCCEEDED( TRYDX( m_pVBuffer->Lock( 0, numVerts * sizeof( sVertex ),
												(BYTE**)&pVerts, D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK ) ) ) )
		{
			m_bVBufferLocked	= true;
			return true;
		}
		else
		{
			return false;
		}
	}

	gperrorf( ("The VBuffer is too small %d verts were requested but it only fits %d", numVerts, SIZE_VBUFFER) );
	return false;
}

void Rapi::UnlockBufferedVertices()
{
	if( !m_bVBufferLocked )
	{
		gperror( "Cannot unlock a vertex buffer that is not locked!" );
		return;
	}

	// Unlock the buffer
	if( SUCCEEDED( TRYDX( m_pVBuffer->Unlock() ) ) )
	{
		m_bVBufferLocked	= false;
	}
}

bool Rapi::CopyBufferedVertices( const unsigned int numVerts, unsigned int& startIndex, sVertex* pSrcVerts )
{
	if( m_bVBufferLocked )
	{
		gperror( "Cannot lock a vertex buffer that is already locked!" );
		return false;
	}

	// numVerts must be a multiple of the number of vertices required to fall onto
	// the 64-byte boundary, since we assume the client will copy the verts
	// 64-bytes at a time.
	int numsVerts	= (numVerts + 7) & 0xFFFFFFF8;
	sVertex* pVerts	= NULL;

	if( m_VBufferPosition + numsVerts < SIZE_VBUFFER )
	{
		// Lock the buffer
		if( SUCCEEDED( TRYDX( m_pVBuffer->Lock( m_VBufferPosition * sizeof( sVertex ), numsVerts * sizeof( sVertex ),
												(BYTE**)&pVerts, D3DLOCK_NOOVERWRITE | D3DLOCK_NOSYSLOCK ) ) ) )
		{
			// Set the index and increment to the new position
			startIndex			= m_VBufferPosition;
			m_VBufferPosition	+= numsVerts;
		}
		else
		{
			return false;
		}
	}
	else if( numsVerts <= SIZE_VBUFFER )
	{
		// Reset the position and index
		m_VBufferPosition		= numsVerts;
		startIndex				= 0;

		// Lock the buffer
		if( FAILED( TRYDX( m_pVBuffer->Lock( 0, numsVerts * sizeof( sVertex ),
											 (BYTE**)&pVerts, D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK ) ) ) )
		{
			return false;
		}
	}
	else
	{
		gperrorf( ("The VBuffer is too small %d verts were requested but it only fits %d", numVerts, SIZE_VBUFFER) );
		return false;
	}

	// Copy the verts
	if( m_bSimdSupport )
	{
		_asm
		{ 
			// Hold number of iterations of the loop (64 bytes per iteration)
			mov eax, numsVerts
			shr eax, 2
			mov ecx, numsVerts
			sub ecx, eax
			shr ecx, 1

			mov esi, pSrcVerts
			mov edi, pVerts 

		loop1:

			// Prefetch next loop, non-temporal 
			prefetchnta 64[ESI] 
			prefetchnta 96[ESI] 

			// Read in source data 
			movq mm1,  0[ESI]
			movq mm2,  8[ESI] 
			movq mm3, 16[ESI] 
			movq mm4, 24[ESI]
			movq mm5, 32[ESI] 
			movq mm6, 40[ESI] 
			movq mm7, 48[ESI] 
			movq mm0, 56[ESI] 

			// Non-temporal stores
			movntq  0[EDI], mm1 
			movntq  8[EDI], mm2 
			movntq 16[EDI], mm3 
			movntq 24[EDI], mm4 
			movntq 32[EDI], mm5 
			movntq 40[EDI], mm6 
			movntq 48[EDI], mm7 
			movntq 56[EDI], mm0 

			add esi, 64 
			add edi, 64 
			dec ecx 
			jnz loop1 

			emms
		}
	}
	else
	{
		memcpy( pVerts, pSrcVerts, sizeof( sVertex ) * numVerts );
	}

	// Unlock the buffer
	return( SUCCEEDED( TRYDX( m_pVBuffer->Unlock() ) ) );
}

void Rapi::DrawBufferedIndexedPrimitive(	DP_TYPE			pType,
											unsigned int	startIndex,
											unsigned int	numVerts,
											unsigned short*	pIndices,
											unsigned int	numIndices )
{
	gpassert( numVerts > 0 );
	gpassert( numIndices > 0 );
	gpassert( startIndex + numVerts < SIZE_VBUFFER );

	if( m_bVBufferLocked )
	{
		gperror( "Cannot render from locked vertex buffer!" );
		return;
	}

	// Draw primitive from the main vertex buffer
	if( !m_bNoRender )
	{
		m_pD3DDevice->SetPixelShader( NULL );
		m_pD3DDevice->SetVertexShader( SVERTEX );
		m_pD3DDevice->SetStreamSource( 0, m_pVBuffer, sizeof( sVertex ) );

		unsigned int indexStart = 0;
		WORD* pBufIndices	= NULL;
		if( LockIndexBuffer( numIndices, indexStart, pBufIndices ) )
		{
			memcpy( pBufIndices, pIndices, sizeof( WORD ) * numIndices );
			UnlockIndexBuffer();
		}
		m_pD3DDevice->SetIndices( m_pIndexBuffer, startIndex );

		TRYDX( m_pD3DDevice->DrawIndexedPrimitive( pType, 0, numVerts, indexStart, GetNumPrimitives( pType, numIndices ) ) );

		// Test to see if we need to draw more passes
		RapiTexture* pTexture	= m_ActiveTex[ 0 ];
		if( pTexture && pTexture->IsMultiPass() )
		{
			// Disable the zbuffer
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		FALSE ));

			// Draw each pass
			for( unsigned int i = 1; i < pTexture->m_numPasses; ++i )
			{
				pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, i );
				TRYDX( m_pD3DDevice->DrawIndexedPrimitive( pType, 0, numVerts, indexStart, GetNumPrimitives( pType, numIndices ) ) );
			}

			// Reset the texture
			// This will, most times, be a waste of state changes.  Unfortunately, with the potential
			// mix of primitive drawing calls available to the user, there's no easy way to assure that we
			// are on the first texture pass unless we add a check to every draw call.  Since multipass is
			// so rare, it seems to me that doing the extra switches only on multipass stuff and relieving
			// the other primitive calls of having to check for it is a better way to go.
			pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, 0 );

			// Enable the zbuffer
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		TRUE ));
		}
	}
}

void Rapi::DrawBufferedPrimitive(	DP_TYPE			pType,
									unsigned int	startVert,
									unsigned int	numVerts )
{
	gpassert( numVerts > 0 );
	gpassert( startVert + numVerts < SIZE_VBUFFER );

	if( m_bVBufferLocked )
	{
		gperror( "Cannot render from locked vertex buffer!" );
		return;
	}

	// Draw primitive from the main vertex buffer
	if( !m_bNoRender )
	{
		m_pD3DDevice->SetPixelShader( NULL );
		m_pD3DDevice->SetVertexShader( SVERTEX );
		m_pD3DDevice->SetStreamSource( 0, m_pVBuffer, sizeof( sVertex ) );

		TRYDX( m_pD3DDevice->DrawPrimitive( pType, startVert, GetNumPrimitives( pType, numVerts ) ) );

		// Test to see if we need to draw more passes
		RapiTexture* pTexture	= m_ActiveTex[ 0 ];
		if( pTexture && pTexture->IsMultiPass() )
		{
			// Disable the zbuffer
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		FALSE ));

			// Draw each pass
			for( unsigned int i = 1; i < pTexture->m_numPasses; ++i )
			{
				pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, i );
				TRYDX( m_pD3DDevice->DrawPrimitive( pType, startVert, GetNumPrimitives( pType, numVerts ) ) );
			}

			// Reset the texture
			// This will, most times, be a waste of state changes.  Unfortunately, with the potential
			// mix of primitive drawing calls available to the user, there's no easy way to assure that we
			// are on the first texture pass unless we add a check to every draw call.  Since multipass is
			// so rare, it seems to me that doing the extra switches only on multipass stuff and relieving
			// the other primitive calls of having to check for it is a better way to go.
			pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, 0 );

			// Enable the zbuffer
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		TRUE ));
		}
	}
}

bool Rapi::LockBufferedLVertices( const unsigned int numVerts, unsigned int& startIndex, lVertex*& pVerts )
{
	if( m_bLVBufferLocked )
	{
		gperror( "Cannot lock a vertex buffer that is already locked!" );
		return false;
	}

	if( m_LVBufferPosition + numVerts < SIZE_VBUFFER )
	{
		// Lock the buffer
		if( SUCCEEDED( TRYDX( m_pLVBuffer->Lock( m_LVBufferPosition * sizeof( lVertex ), numVerts * sizeof( lVertex ),
												 (BYTE**)&pVerts, D3DLOCK_NOOVERWRITE | D3DLOCK_NOSYSLOCK ) ) ) )
		{
			// Set the index and increment to the new position
			startIndex			= m_LVBufferPosition;
			m_LVBufferPosition	+= numVerts;
			m_bLVBufferLocked	= true;

			return true;
		}
		else
		{
			return false;
		}
	}
	else if( numVerts <= SIZE_VBUFFER )
	{
		// Reset the position and index
		m_LVBufferPosition		= numVerts;
		startIndex				= 0;

		// Lock the buffer
		if( SUCCEEDED( TRYDX( m_pLVBuffer->Lock( 0, numVerts * sizeof( lVertex ),
												 (BYTE**)&pVerts, D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK ) ) ) )
		{
			m_bLVBufferLocked	= true;
			return true;
		}
		else
		{
			return false;
		}
	}

	gperrorf( ("The MVBuffer is too small %d verts were requested but it only fits %d", numVerts, SIZE_VBUFFER) );
	return false;
}

void Rapi::UnlockBufferedLVertices()
{
	if( !m_bLVBufferLocked )
	{
		gperror( "Cannot unlock a vertex buffer that is not locked!" );
		return;
	}

	// Unlock the buffer
	if( SUCCEEDED( TRYDX( m_pLVBuffer->Unlock() ) ) )
	{
		m_bLVBufferLocked	= false;
	}
}

void Rapi::DrawBufferedIndexedLPrimitive( DP_TYPE			pType,
										  unsigned int		startIndex,
										  unsigned int		numVerts,
										  unsigned short*	pIndices,
										  unsigned int		numIndices )
{
	gpassert( numVerts > 0 );
	gpassert( numIndices > 0 );
	gpassert( startIndex + numVerts < SIZE_VBUFFER );

	if( m_bLVBufferLocked )
	{
		gperror( "Cannot render from locked vertex buffer!" );
		return;
	}

	// Draw primitive from the main vertex buffer
	if( !m_bNoRender )
	{
		m_pD3DDevice->SetPixelShader( NULL );
		m_pD3DDevice->SetVertexShader( LVERTEX );
		m_pD3DDevice->SetStreamSource( 0, m_pLVBuffer, sizeof( lVertex ) );

		unsigned int indexStart = 0;
		WORD* pBufIndices	= NULL;
		if( LockIndexBuffer( numIndices, indexStart, pBufIndices ) )
		{
			memcpy( pBufIndices, pIndices, sizeof( WORD ) * numIndices );
			UnlockIndexBuffer();
		}
		m_pD3DDevice->SetIndices( m_pIndexBuffer, startIndex );

		TRYDX( m_pD3DDevice->DrawIndexedPrimitive( pType, 0, numVerts, indexStart, GetNumPrimitives( pType, numIndices ) ) );

		// Test to see if we need to draw more passes
		RapiTexture* pTexture	= m_ActiveTex[ 0 ];
		if( pTexture && pTexture->IsMultiPass() )
		{
			// Disable the zbuffer
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		FALSE ));

			// Draw each pass
			for( unsigned int i = 1; i < pTexture->m_numPasses; ++i )
			{
				pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, i );
				TRYDX( m_pD3DDevice->DrawIndexedPrimitive( pType, 0, numVerts, indexStart, GetNumPrimitives( pType, numIndices ) ) );
			}

			// Reset the texture
			// This will, most times, be a waste of state changes.  Unfortunately, with the potential
			// mix of primitive drawing calls available to the user, there's no easy way to assure that we
			// are on the first texture pass unless we add a check to every draw call.  Since multipass is
			// so rare, it seems to me that doing the extra switches only on multipass stuff and relieving
			// the other primitive calls of having to check for it is a better way to go.
			pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, 0 );

			// Enable the zbuffer
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		TRUE ));
		}
	}
}

void Rapi::DrawBufferedLPrimitive( DP_TYPE		pType,
								   unsigned int	startVert,
								   unsigned int	numVerts )
{
	gpassert( numVerts > 0 );
	gpassert( startVert + numVerts < SIZE_VBUFFER );

	if( m_bLVBufferLocked )
	{
		gperror( "Cannot render from locked vertex buffer!" );
		return;
	}

	// Draw primitive from the main vertex buffer
	if( !m_bNoRender )
	{
		m_pD3DDevice->SetPixelShader( NULL );
		m_pD3DDevice->SetVertexShader( LVERTEX );
		m_pD3DDevice->SetStreamSource( 0, m_pLVBuffer, sizeof( lVertex ) );

		TRYDX( m_pD3DDevice->DrawPrimitive( pType, startVert, GetNumPrimitives( pType, numVerts ) ) );

		// Test to see if we need to draw more passes
		RapiTexture* pTexture	= m_ActiveTex[ 0 ];
		if( pTexture && pTexture->IsMultiPass() )
		{
			// Disable the zbuffer
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		FALSE ));

			// Draw each pass
			for( unsigned int i = 1; i < pTexture->m_numPasses; ++i )
			{
				pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, i );
				TRYDX( m_pD3DDevice->DrawPrimitive( pType, startVert, GetNumPrimitives( pType, numVerts ) ) );
			}

			// Reset the texture
			// This will, most times, be a waste of state changes.  Unfortunately, with the potential
			// mix of primitive drawing calls available to the user, there's no easy way to assure that we
			// are on the first texture pass unless we add a check to every draw call.  Since multipass is
			// so rare, it seems to me that doing the extra switches only on multipass stuff and relieving
			// the other primitive calls of having to check for it is a better way to go.
			pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, 0 );

			// Enable the zbuffer
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		TRUE ));
		}
	}
}

bool Rapi::LockBufferedTVertices( const unsigned int numVerts, unsigned int& startIndex, tVertex*& pVerts )
{
	if( m_bTVBufferLocked )
	{
		gperror( "Cannot lock a vertex buffer that is already locked!" );
		return false;
	}

	if( m_TVBufferPosition + numVerts < SIZE_TVBUFFER )
	{
		// Lock the buffer
		if( SUCCEEDED( TRYDX( m_pTVBuffer->Lock( m_TVBufferPosition * sizeof( tVertex ), numVerts * sizeof( tVertex ),
												 (BYTE**)&pVerts, D3DLOCK_NOOVERWRITE | D3DLOCK_NOSYSLOCK ) ) ) )
		{
			// Set the index and increment to the new position
			startIndex			= m_TVBufferPosition;
			m_TVBufferPosition	+= numVerts;
			m_bTVBufferLocked	= true;

			return true;
		}
		else
		{
			return false;
		}
	}
	else if( numVerts <= SIZE_TVBUFFER )
	{
		// Reset the position and index
		m_TVBufferPosition		= numVerts;
		startIndex				= 0;

		// Lock the buffer
		if( SUCCEEDED( TRYDX( m_pTVBuffer->Lock( 0, numVerts * sizeof( tVertex ),
												 (BYTE**)&pVerts, D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK ) ) ) )
		{
			m_bTVBufferLocked	= true;
			return true;
		}
		else
		{
			return false;
		}
	}

	gperrorf( ("The TVBuffer is too small %d verts were requested but it only fits %d", numVerts, SIZE_VBUFFER) );
	return false;
}

void Rapi::UnlockBufferedTVertices()
{
	if( !m_bTVBufferLocked )
	{
		gperror( "Cannot unlock a vertex buffer that is not locked!" );
		return;
	}

	// Unlock the buffer
	if( SUCCEEDED( TRYDX( m_pTVBuffer->Unlock() ) ) )
	{
		m_bTVBufferLocked	= false;
	}
}

void Rapi::DrawBufferedIndexedTPrimitive( DP_TYPE			pType,
										  unsigned int		startIndex,
										  unsigned int		numVerts,
										  unsigned short*	pIndices,
										  unsigned int		numIndices )
{
	gpassert( numVerts > 0 );
	gpassert( numIndices > 0 );
	gpassert( startIndex + numVerts < SIZE_TVBUFFER );

	if( m_bTVBufferLocked )
	{
		gperror( "Cannot render from locked vertex buffer!" );
		return;
	}

	// Draw primitive from the main vertex buffer
	if( !m_bNoRender )
	{
		m_pD3DDevice->SetPixelShader( NULL );
		m_pD3DDevice->SetVertexShader( TVERTEX );
		m_pD3DDevice->SetStreamSource( 0, m_pTVBuffer, sizeof( tVertex ) );

		unsigned int indexStart = 0;
		WORD* pBufIndices	= NULL;
		if( LockIndexBuffer( numIndices, indexStart, pBufIndices ) )
		{
			memcpy( pBufIndices, pIndices, sizeof( WORD ) * numIndices );
			UnlockIndexBuffer();
		}
		m_pD3DDevice->SetIndices( m_pIndexBuffer, startIndex );

		TRYDX( m_pD3DDevice->DrawIndexedPrimitive( pType, 0, numVerts, indexStart, GetNumPrimitives( pType, numIndices ) ) );

		// Test to see if we need to draw more passes
		RapiTexture* pTexture	= m_ActiveTex[ 0 ];
		if( pTexture && pTexture->IsMultiPass() )
		{
			// Disable the zbuffer
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		FALSE ));

			// Draw each pass
			for( unsigned int i = 1; i < pTexture->m_numPasses; ++i )
			{
				pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, i );
				TRYDX( m_pD3DDevice->DrawIndexedPrimitive( pType, 0, numVerts, indexStart, GetNumPrimitives( pType, numIndices ) ) );
			}

			// Reset the texture
			// This will, most times, be a waste of state changes.  Unfortunately, with the potential
			// mix of primitive drawing calls available to the user, there's no easy way to assure that we
			// are on the first texture pass unless we add a check to every draw call.  Since multipass is
			// so rare, it seems to me that doing the extra switches only on multipass stuff and relieving
			// the other primitive calls of having to check for it is a better way to go.
			pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, 0 );

			// Enable the zbuffer
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		TRUE ));
		}
	}
}

void Rapi::DrawBufferedTPrimitive( DP_TYPE		pType,
								   unsigned int	startVert,
								   unsigned int	numVerts )
{
	gpassert( numVerts > 0 );
	gpassert( startVert + numVerts < SIZE_TVBUFFER );

	if( m_bTVBufferLocked )
	{
		gperror( "Cannot render from locked vertex buffer!" );
		return;
	}

	// Draw primitive from the main vertex buffer
	if( !m_bNoRender )
	{
		m_pD3DDevice->SetPixelShader( NULL );
		m_pD3DDevice->SetVertexShader( TVERTEX );
		m_pD3DDevice->SetStreamSource( 0, m_pTVBuffer, sizeof( tVertex ) );

		TRYDX( m_pD3DDevice->DrawPrimitive( pType, startVert, GetNumPrimitives( pType, numVerts ) ) );

		// Test to see if we need to draw more passes
		RapiTexture* pTexture	= m_ActiveTex[ 0 ];
		if( pTexture && pTexture->IsMultiPass() )
		{
			// Disable the zbuffer
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		FALSE ));

			// Draw each pass
			for( unsigned int i = 1; i < pTexture->m_numPasses; ++i )
			{
				pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, i );
				TRYDX( m_pD3DDevice->DrawPrimitive( pType, startVert, GetNumPrimitives( pType, numVerts ) ) );
			}

			// Reset the texture
			// This will, most times, be a waste of state changes.  Unfortunately, with the potential
			// mix of primitive drawing calls available to the user, there's no easy way to assure that we
			// are on the first texture pass unless we add a check to every draw call.  Since multipass is
			// so rare, it seems to me that doing the extra switches only on multipass stuff and relieving
			// the other primitive calls of having to check for it is a better way to go.
			pTexture->SetAsActiveSurface( m_elapsedTime, m_secondTime, 0, 0 );

			// Enable the zbuffer
			TRYDX( m_pD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE,		TRUE ));
		}
	}
}

// Index buffer
bool Rapi::LockIndexBuffer( const unsigned int numIndices, unsigned int& startIndex, WORD*& pIndices )
{
	if( m_IndexBufferPosition + numIndices < SIZE_VBUFFER )
	{
		// Lock the buffer
		if( SUCCEEDED( TRYDX( m_pIndexBuffer->Lock( m_IndexBufferPosition * 2, numIndices * 2,
													(BYTE**)&pIndices, D3DLOCK_NOOVERWRITE | D3DLOCK_NOSYSLOCK ) ) ) )
		{
			// Set the index and increment to the new position
			startIndex				= m_IndexBufferPosition;
			m_IndexBufferPosition	+= numIndices;
			return true;
		}
		else
		{
			return false;
		}
	}
	else if( numIndices <= SIZE_VBUFFER )
	{
		// Reset the position and index
		m_IndexBufferPosition	= numIndices;
		startIndex				= 0;

		// Lock the buffer
		if( SUCCEEDED( TRYDX( m_pIndexBuffer->Lock( 0, numIndices * 2,
													(BYTE**)&pIndices, D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK ) ) ) )
		{
			return true;
		}
		else
		{
			return false;
		}

	}

	gperrorf( ("The index buffer is too small %d indices were requested but it only fits %d", numIndices, SIZE_VBUFFER) );
	return false;
}

void Rapi::UnlockIndexBuffer()
{
	TRYDX( m_pIndexBuffer->Unlock() );
}

// Set a set of parameters for this stage in the pipe
void Rapi::SetTextureStageState(	int		stage,		// What stage to setup
									TEX_OP	colorop,	// Texturing operation
									TEX_OP  alphaop,	// Alpha operation
									TEX_AD	uaddress,	// Address mode in U direction
									TEX_AD	vaddress,	// Address mode in V direction
									TEX_MGF	magfilt,	// Expansion filter
									TEX_MNF minfilt,	// Shrinking filter
									TEX_MPF mipfilt,	// Mipmap filter
									TEX_BLD srcblend,	// Source blend
									TEX_BLD dstblend,	// Destination blend
									bool	alphatest )
{
	gpassert( (stage >= 0) );
	TextureStageState& m_stageState = m_TextureStageStates[ stage ];

	// Setup the user defined values first
	if( m_bIgnoreVertexColor && (stage == 0) )
	{
		if( m_stageState.colorOp != D3DTOP_SELECTARG1 )
		{
			TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_COLOROP,	D3DTOP_SELECTARG1 ));
			m_stageState.colorOp	= D3DTOP_SELECTARG1;
		}
	}
	else
	{
		if( m_stageState.colorOp != colorop )
		{
			if( m_bNoAdd && colorop == D3DTOP_ADD )
			{
				colorop				= D3DTOP_SELECTARG2;
			}
			TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_COLOROP,	colorop ));
			m_stageState.colorOp	= colorop;
		}
	}

	if( m_stageState.alphaOp != alphaop )
	{
		if( m_bNoAdd && alphaop == D3DTOP_ADD )
		{
			alphaop				= D3DTOP_SELECTARG2;
		}
		TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_ALPHAOP,		alphaop ));
		m_stageState.alphaOp	= alphaop;
	}
	if( m_stageState.addressU != uaddress )
	{
		TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_ADDRESSU,		uaddress ));
		m_stageState.addressU	= uaddress;
	}
	if( m_stageState.addressV != vaddress )
	{
		TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_ADDRESSV,		vaddress ));
		m_stageState.addressV	= vaddress;
	}
	if( m_stageState.magFilter != magfilt )
	{
		TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_MAGFILTER,		magfilt ));
		m_stageState.magFilter	= magfilt;
	}
	if( m_stageState.minFilter != minfilt )
	{
		TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_MINFILTER,		minfilt ));
		m_stageState.minFilter	= minfilt;
	}

	mipfilt	= min_t( mipfilt, m_mipFilter );
	if( m_stageState.mipFilter != mipfilt )
	{
		TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_MIPFILTER,		mipfilt ));
		m_stageState.mipFilter	= mipfilt;
	}

	// Setup a default set of standard stage values
	if( m_stageState.colorArg1 != D3DTA_TEXTURE )
	{
		TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_COLORARG1,		D3DTA_TEXTURE ));
		m_stageState.colorArg1	= D3DTA_TEXTURE;
	}
	if( m_stageState.colorArg2 != D3DTA_DIFFUSE )
	{
		TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_COLORARG2,		D3DTA_DIFFUSE ));
		m_stageState.colorArg2	= D3DTA_DIFFUSE;
	}
	if( m_stageState.alphaArg1 != D3DTA_TEXTURE )
	{
		TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_ALPHAARG1,		D3DTA_TEXTURE ));
		m_stageState.alphaArg1	= D3DTA_TEXTURE;
	}
	if( m_stageState.alphaArg2 != D3DTA_DIFFUSE )
	{
		TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_ALPHAARG2,		D3DTA_DIFFUSE ));
		m_stageState.alphaArg2	= D3DTA_DIFFUSE;
	}

	// Blending modes
	if( m_srcBlend != srcblend )
	{
		TRYDX (m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND,			srcblend ));
		m_srcBlend	= srcblend;
	}
	if( m_dstBlend != dstblend )
	{
		TRYDX (m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND,			dstblend ));
		m_dstBlend	= dstblend;
	}

	// Alpha testing
	if( alphatest && !m_bAlphaTest )
	{
		TRYDX (m_pD3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE,		TRUE ));
		TRYDX (m_pD3DDevice->SetRenderState( D3DRS_ALPHAFUNC,			D3DCMP_GREATER ));
		m_bAlphaTest	= true;
	}
	else if( !alphatest && m_bAlphaTest )
	{
		TRYDX (m_pD3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE,	FALSE ));
		m_bAlphaTest	= false;
	}
}

void Rapi::SetSingleStageState( int stage, D3DTEXTURESTAGESTATETYPE type, DWORD value )
{
	gpassert( (stage >= 0) );
	TextureStageState& m_stageState = m_TextureStageStates[ stage ];

	switch( type )
	{
	case D3DTSS_COLOROP:
		{
			if( m_bIgnoreVertexColor && (stage == 0) )
			{
				if( m_stageState.colorOp != D3DTOP_SELECTARG1 )
				{
					TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_COLOROP,	D3DTOP_SELECTARG1 ));
					m_stageState.colorOp	= D3DTOP_SELECTARG1;
				}
			}
			else
			{
				if( m_stageState.colorOp != (TEX_OP)value )
				{
					if( m_bNoAdd && value == (DWORD)D3DTOP_ADD )
					{
						value				= (DWORD)D3DTOP_SELECTARG2;
					}
					TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_COLOROP,	value ));
					m_stageState.colorOp	= (TEX_OP)value;
				}
			}
			break;
		}
	case D3DTSS_ALPHAOP:
		{
			if( m_stageState.alphaOp != (TEX_OP)value )
			{
				if( m_bNoAdd && value == (DWORD)D3DTOP_ADD )
				{
					value				= (DWORD)D3DTOP_SELECTARG2;
				}
				TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_ALPHAOP,		value ));
				m_stageState.alphaOp	= (TEX_OP)value;
			}
			break;
		}
	case D3DTSS_ADDRESSU:
		{
			if( m_stageState.addressU != (TEX_AD)value )
			{
				TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_ADDRESSU,		value ));
				m_stageState.addressU	= (TEX_AD)value;
			}
			break;
		}
	case D3DTSS_ADDRESSV:
		{
			if( m_stageState.addressV != (TEX_AD)value )
			{
				TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_ADDRESSV,		value ));
				m_stageState.addressV	= (TEX_AD)value;
			}
			break;
		}
	case D3DTSS_MAGFILTER:
		{
			if( m_stageState.magFilter != (TEX_MGF)value )
			{
				TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_MAGFILTER,		value ));
				m_stageState.magFilter	= (TEX_MGF)value;
			}
			break;
		}
	case D3DTSS_MINFILTER:
		{
			if( m_stageState.minFilter != (TEX_MNF)value )
			{
				TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_MINFILTER,		value ));
				m_stageState.minFilter	= (TEX_MNF)value;
			}
			break;
		}
	case D3DTSS_MIPFILTER:
		{
			value = min_t( value, (DWORD)m_mipFilter );
			if( m_stageState.mipFilter != (TEX_MPF)value )
			{
				TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_MIPFILTER,		value ));
				m_stageState.mipFilter	= (TEX_MPF)value;
			}
			break;
		}
	case D3DTSS_COLORARG1:
		{
			if( m_stageState.colorArg1 != value )
			{
				TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_COLORARG1,		value ));
				m_stageState.colorArg1	= value;
			}
			break;
		}
	case D3DTSS_COLORARG2:
		{
			if( m_stageState.colorArg2 != value )
			{
				TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_COLORARG2,		value ));
				m_stageState.colorArg2	= value;
			}
			break;
		}
	case D3DTSS_ALPHAARG1:
		{
			if( m_stageState.alphaArg1 != value )
			{
				TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_ALPHAARG1,		value ));
				m_stageState.alphaArg1	= value;
			}
			break;
		}
	case D3DTSS_ALPHAARG2:
		{
			if( m_stageState.alphaArg2 != value )
			{
				TRYDX (m_pD3DDevice->SetTextureStageState( stage, D3DTSS_ALPHAARG2,		value ));
				m_stageState.alphaArg2	= value;
			}
			break;
		}
	default:
		{
			m_pD3DDevice->SetTextureStageState( stage, type, value );
			break;
		}
	};
}

// Copy a texture from one Rapi object to another
void Rapi::CopyTextureFromRenderer( Rapi& renderer, unsigned int tIndex )
{
	if( tIndex == 0 )
	{
		return;
	}

	gpassert( renderer.m_Textures[ tIndex ] );
	if( renderer.m_Textures[ tIndex ] )
	{
		RapiTexture* pTex	= NULL;

		if( renderer.m_Textures[ tIndex ]->GetPathname().length() )
		{
			// Create the texture from a file
			gpstring errorSink;
			pTex	= new RapiTexture(	this, renderer.m_Textures[ tIndex ]->GetPathname(), true, 0, TEX_LOCKED, true, errorSink );

			// Debugging to make sure everything is valid
			gpassert( pTex );
			pTex->IncrementReferenceCount();

			// Put this texture into our global list
			if( m_Textures.IsValid( tIndex ) && m_Textures[ tIndex ] )
			{
				DestroyTexture( tIndex );
			}

			if( !m_Textures.AllocSpecific( pTex, tIndex ) )
			{
				delete[] pTex;
				gperror( "Could not properly copy texture from one renderer to another!" );
			}
		}
		else
		{
			gpwarning( "Cannot duplicate algorithmic textures!" );
		}
	}
}

// Paint the back buffer(s) to the front buffer(s)
void Rapi::OnWmPaint( const RECT& dstRect, const RECT& srcRect )
{
	// Don't call this if fullscreen
	gpassert( !m_bFullScreen );

	// Present the scene
	m_pD3DDevice->Present( &srcRect, &dstRect, NULL, NULL );
}
