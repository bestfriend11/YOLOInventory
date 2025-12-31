/*************************************************************************************
**
**									RapiTexture
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

#include "RapiImage.h"
#include "fuel.h"
#include "stringtool.h"

// Need access to the naming to expand textures within a TSD definition
#include "namingkey.h"


// Construction of algorithmic textures
RapiTexture::RapiTexture(
						Rapi*			pOwner,
						const gpstring&	filename,
						bool			bMipmapping,
						int				rStage,
						TEX_OPT			rOptimize,
						bool			bFullLoad,
						gpstring&		errorSink )
						: m_pDevice( pOwner )
						, m_Pathname( filename )
						, m_refCount( 0 )
						, m_loadRefCount( bFullLoad ? 1 : 0 )
						, m_bEnvironment( false )
						, m_bSaturated( false )
						, m_numPasses( 1 )
{
	gpassert( FileSys::DoesResourceFileExist( m_Pathname ) );

	InitSurfaceDescription( errorSink );

	for( unsigned int p = 0; p < m_numPasses; ++p )
	{
		TextureSurfaceDescription& surfaceDescription	= m_pSurfaceDescription[p];
		for( int i = 0; i < surfaceDescription.m_numLayers; ++i )
		{
			for( int o = 0; o < surfaceDescription.m_SurfaceLayers[i].m_numFrames; ++o )
			{
				if( !surfaceDescription.m_SurfaceLayers[i].m_textureNames[o].length() )
				{
					surfaceDescription.m_SurfaceLayers[i].m_textureSurface.push_back( 0 );
				}
				else if( surfaceDescription.m_SurfaceLayers[i].m_textureNames[o].same_no_case( "environment" ) )
				{
					surfaceDescription.m_SurfaceLayers[i].m_textureSurface.push_back( ENVIRONMENT );
					m_bEnvironment	= true;
				}
				else
				{
					surfaceDescription.m_SurfaceLayers[i].m_textureSurface.push_back( m_pDevice->CreateSurface( surfaceDescription.m_SurfaceLayers[i].m_textureNames[o], bMipmapping, rStage + i, rOptimize, IsAlphaFormat(), surfaceDescription.m_dither, surfaceDescription.m_texDetail, bFullLoad, errorSink ) );
				}
			}
		}
	}

#	if !GP_RETAIL
	if ( gRapiOwner.IsTracingTextures() )
	{
		gpgenericf(( "(0x%08X) texture creation: %s\n", this, m_Pathname.c_str() ));
	}
#	endif // !GP_RETAIL
}

// Construction of algorithmic texture
RapiTexture::RapiTexture(
						Rapi*			pOwner,
						const gpstring&	texture_name,
						RapiMemImage*	pTakeImage,
						bool			bMipmapping,
						int				rStage,
						TEX_OPT			rOptimize,
						bool			bHasAlpha,
						bool			bDither,
						gpstring&		errorSink )
						: m_pDevice( pOwner )
						, m_Pathname( "<algo> " + texture_name )
						, m_refCount( 0 )
						, m_loadRefCount( 1 )
						, m_bEnvironment( false )
						, m_bSaturated( false )
						, m_numPasses( 1 )
{
	m_pSurfaceDescription	= new TextureSurfaceDescription[ 1 ];
	m_pbActive				= new bool[ 1 ];

	// Set the alpha flag
	m_pSurfaceDescription[0].m_hasAlpha	= bHasAlpha;

	// Set the dither flag
	m_pSurfaceDescription[0].m_dither	= bDither;

	TextureSurfaceInfo nInfo;
	nInfo.m_textureSurface.push_back( m_pDevice->CreateSurface( pTakeImage, bMipmapping, rStage, rOptimize, bHasAlpha, bDither, 0, errorSink ) );
	m_pSurfaceDescription[0].m_SurfaceLayers.push_back( nInfo );

#	if !GP_RETAIL
	if ( gRapiOwner.IsTracingTextures() )
	{
		gpgenericf(( "(0x%08X) algo texture creation: %s\n", this, m_Pathname.c_str() ));
	}
#	endif // !GP_RETAIL
}

// Construction of empty texture
RapiTexture::RapiTexture(
						Rapi*				pOwner,
						int					width,
						int					height,
						DWORD				color,
						TEX_OPT				rOptimize,
						bool				bHasAlpha,
						bool				bDither,
						bool				bMatchPrimary,
						bool				bRenderTarget,
						gpstring&			errorSink )
						: m_pDevice( pOwner )
						, m_Pathname( "<empty>" )
						, m_refCount( 0 )
						, m_loadRefCount( 1 )
						, m_bEnvironment( false )
						, m_bSaturated( false )
						, m_numPasses( 1 )
{
	m_pSurfaceDescription	= new TextureSurfaceDescription[ 1 ];
	m_pbActive				= new bool[ 1 ];

	// Set the alpha flag
	m_pSurfaceDescription[0].m_hasAlpha	= bHasAlpha;

	// Set the dither flag
	m_pSurfaceDescription[0].m_dither	= bDither;

	TextureSurfaceInfo nInfo;
	nInfo.m_textureSurface.push_back( m_pDevice->CreateSurface( width, height, color, rOptimize, bHasAlpha, bDither, bMatchPrimary, bRenderTarget, errorSink ) );
	m_pSurfaceDescription[0].m_SurfaceLayers.push_back( nInfo );

#	if !GP_RETAIL
	if ( gRapiOwner.IsTracingTextures() )
	{
		gpgenericf(( "(0x%08X) empty texture creation\n", this ));
	}
#	endif // !GP_RETAIL
}

// Implicit destruction of texture
RapiTexture::~RapiTexture()
{
#	if !GP_RETAIL
	if ( gRapiOwner.IsTracingTextures() )
	{
		gpgenericf(( "(0x%08X) texture destruction: '%s'\n", this, m_Pathname.c_str() ));
	}
#	endif // !GP_RETAIL

	Destroy();
}

// Cleanup all information for this texture
void RapiTexture::Destroy()
{
	for( unsigned int p = 0; p < m_numPasses; ++p )
	{
		TextureSurfaceDescription& surfaceDescription	= m_pSurfaceDescription[p];
		for( int i = 0; i < surfaceDescription.m_numLayers; ++i )
		{
			for( int o = 0; o < surfaceDescription.m_SurfaceLayers[i].m_numFrames; ++o )
			{
				if( surfaceDescription.m_SurfaceLayers[i].m_textureSurface[o] &&
					surfaceDescription.m_SurfaceLayers[i].m_textureSurface[o] != ENVIRONMENT )
				{
					m_pDevice->DestroySurface( surfaceDescription.m_SurfaceLayers[i].m_textureSurface[o] );
				}
			}
		}
	}

	// Destroy our surface descriptions
	delete[] m_pSurfaceDescription;
	delete[] m_pbActive;
}

// Get the width of the first surface
unsigned int RapiTexture::GetWidth()
{
	TextureSurfaceDescription& surfaceDescription	= m_pSurfaceDescription[0];
	if( surfaceDescription.m_numLayers && surfaceDescription.m_SurfaceLayers[0].m_numFrames )
	{
		if( surfaceDescription.m_SurfaceLayers[0].m_textureSurface[0] == ENVIRONMENT )
		{
			if( m_pDevice->m_Surfaces[ m_pDevice->GetEnvironmentTexture() ] )
			{
				return m_pDevice->m_Surfaces[ m_pDevice->GetEnvironmentTexture() ]->GetWidth();
			}
		}
		else if( m_pDevice->m_Surfaces[ surfaceDescription.m_SurfaceLayers[0].m_textureSurface[0] ] )
		{
			return m_pDevice->m_Surfaces[ surfaceDescription.m_SurfaceLayers[0].m_textureSurface[0] ]->GetWidth();
		}
	}

	return 0;
}

// Get the height of the first surface
unsigned int RapiTexture::GetHeight()
{
	TextureSurfaceDescription& surfaceDescription	= m_pSurfaceDescription[0];
	if( surfaceDescription.m_numLayers && surfaceDescription.m_SurfaceLayers[0].m_numFrames )
	{
		if( surfaceDescription.m_SurfaceLayers[0].m_textureSurface[0] == ENVIRONMENT )
		{
			if( m_pDevice->m_Surfaces[ m_pDevice->GetEnvironmentTexture() ] )
			{
				return m_pDevice->m_Surfaces[ m_pDevice->GetEnvironmentTexture() ]->GetHeight();
			}
		}
		else if( m_pDevice->m_Surfaces[ surfaceDescription.m_SurfaceLayers[0].m_textureSurface[0] ] )
		{
			return m_pDevice->m_Surfaces[ surfaceDescription.m_SurfaceLayers[0].m_textureSurface[0] ]->GetHeight();
		}
	}

	return 0;
}

// No from-disk texture can be empty or start with '<'
bool RapiTexture::IsFromDisk()
{
	return ( !m_Pathname.empty() && (m_Pathname[ 0 ] != '<') );
}

// Read in the surface description information
void RapiTexture::InitSurfaceDescription( gpstring& errorSink )
{
	// Open the fuel block to the requested information
	FastFuelHandle hSurface;
	if( gDefaultRapi.IsMultiTexBlend() )
	{
		hSurface = FastFuelHandle( FilePathToFuelAddress( m_Pathname ) );

#if !GP_RETAIL
		if( hSurface.IsValid() )
		{
			if( hSurface.GetInt( "numlayers", 1 ) > 1 )
			{
				gpstring simpleName	= m_Pathname;
				stringtool::RemoveExtension( simpleName );
				FastFuelHandle testSurface( FilePathToFuelAddress( simpleName + "-simple" ) );
				if( !testSurface.IsValid() )
				{
					errorSink.appendf( "Texture [%s] has multiple layers but no -simple version has been specified\n", m_Pathname.c_str() );
				}
			}
		}
#endif
	}
	else
	{
		gpstring simpleName	= m_Pathname;
		stringtool::RemoveExtension( simpleName );
		hSurface = FastFuelHandle( FilePathToFuelAddress( simpleName + "-simple" ) );
		if( !hSurface.IsValid() )
		{
			hSurface = FastFuelHandle( FilePathToFuelAddress( m_Pathname ) );
		}
	}

	if( hSurface.IsValid() )
	{
		// First see if we have multiple passes to deal with
		hSurface.Get( "numpasses",			m_numPasses );
		m_pSurfaceDescription	= new TextureSurfaceDescription[ m_numPasses ];
		m_pbActive				= new bool[ m_numPasses ];

		for( unsigned int p = 0; p < m_numPasses; ++p )
		{
			// Get a reference to the descriptor we're filling for easy access
			TextureSurfaceDescription& surfaceDescription	= m_pSurfaceDescription[p];

			// Build the proper address for this pass
			gpstring passName	= hSurface.GetAddress();
			passName.appendf( ":pass%d", p+1 );
			FastFuelHandle hPass( passName );

			// If this address is not valid, fall back to the base address
			if( !hPass.IsValid() )
			{
				hPass = hSurface;
			}

			// This should be the information we are searching for
			hPass.Get( "numlayers",			surfaceDescription.m_numLayers );
			hPass.Get( "timesyncanimation",	surfaceDescription.m_syncTimer );
			hPass.Get( "hasalpha",			surfaceDescription.m_hasAlpha );
			hPass.Get( "alphaformat",		surfaceDescription.m_alphaFormat );
			hPass.Get( "alphatest",			surfaceDescription.m_alphaTest );
			hPass.Get( "noalphasort",		surfaceDescription.m_noAlphaSort );
			hPass.Get( "dither",			surfaceDescription.m_dither );
			hPass.Get( "startanimation",	surfaceDescription.m_bAnimationState );
			hPass.Get( "texturedetail",		surfaceDescription.m_texDetail );
			hPass.Get( "multiinstance",		surfaceDescription.m_multiInstance );

			// Get the blending information for this pass
			gpstring blendinfo;
			if( hPass.Get( "srcblend", blendinfo ) )
			{
				surfaceDescription.m_srcBlend	= GetEnumeratedBlendValueFromString( blendinfo );
			}
			if( hPass.Get( "destblend", blendinfo ) )
			{
				surfaceDescription.m_destBlend	= GetEnumeratedBlendValueFromString( blendinfo );
			}

			// Make sure that if we have alpha, we use an alpha based format
			if( surfaceDescription.m_hasAlpha || surfaceDescription.m_alphaTest )
			{
				surfaceDescription.m_alphaFormat	= true;
			}

			gpassert( surfaceDescription.m_numLayers > 0 );

			if( surfaceDescription.m_numLayers > MAX_TEXTURE_LAYERS )
			{
				errorSink.appendf( "Texture [%s] has too many layers\n", m_Pathname.c_str() );
				surfaceDescription.m_numLayers = MAX_TEXTURE_LAYERS;
			}

			if( surfaceDescription.m_numLayers > (int)m_pDevice->m_d3dDevDesc.MaxSimultaneousTextures )
			{
				surfaceDescription.m_numLayers = (int)m_pDevice->m_d3dDevDesc.MaxSimultaneousTextures;
			}

			surfaceDescription.m_SurfaceLayers.reserve( surfaceDescription.m_numLayers );

			char buf[ 256 ];
			for( int layer = 0; layer < surfaceDescription.m_numLayers; ++layer )
			{
				TextureSurfaceInfo nInfo;

				sprintf( buf, "layer%dnumframes", layer+1 );
				hPass.Get( buf, nInfo.m_numFrames );
				gpassert( nInfo.m_numFrames > 0 );

				if( nInfo.m_numFrames > MAX_TEXTURE_FRAMES )
				{
					errorSink.appendf( "Texture [%s] has too many frames on layer %d\n", m_Pathname.c_str(), layer );
					nInfo.m_numFrames = MAX_TEXTURE_FRAMES;
				}

				nInfo.m_textureNames.reserve( nInfo.m_numFrames );
				nInfo.m_textureSurface.reserve( nInfo.m_numFrames );

				for( int frame = 0; frame < nInfo.m_numFrames; ++frame )
				{
					sprintf( buf, "layer%dtexture%d", layer+1, frame+1 );
					gpstring texturecontentname;

					if( !hPass.HasKey( buf ) )
					{
						errorSink.appendf( "Texture [%s] is missing a a texture for layer %d frame %d\n", m_Pathname.c_str(), layer, frame );
					}
					hPass.Get( buf, texturecontentname );

					if( !texturecontentname.same_no_case( "environment" ) &&
						NamingKey::DoesSingletonExist() )
					{
						// Convert the content name to a path name using the naming key
						gpstring fullname;
						if( !gNamingKey.BuildIMGLocation( texturecontentname, fullname ) )
						{
							// Is this a legacy texture?
							errorSink.appendf( "Legacy texture [%s] in TSD [%s]\n", texturecontentname.c_str(), m_Pathname.c_str() );
							fullname = texturecontentname;
						}

						nInfo.m_textureNames.push_back( fullname );
					}
					else
					{
						nInfo.m_textureNames.push_back( texturecontentname );
					}
				}

				sprintf( buf, "layer%dsecondsperframe", layer+1 );
				hPass.Get( buf, nInfo.m_secondsPerFrame );

				sprintf( buf, "layer%dushiftpersecond", layer+1 );
				hPass.Get( buf, nInfo.m_ushiftPerSecond );

				sprintf( buf, "layer%dvshiftpersecond", layer+1 );
				hPass.Get( buf, nInfo.m_vshiftPerSecond );

				sprintf( buf, "layer%drotationpersecond", layer+1 );
				hPass.Get( buf, nInfo.m_radrotPerSecond );

				sprintf( buf, "layer%drotationucenter", layer+1 );
				hPass.Get( buf, nInfo.m_uRotCenter );

				sprintf( buf, "layer%drotationvcenter", layer+1 );
				hPass.Get( buf, nInfo.m_vRotCenter );

				/*******************************************************************/
				/*				OPTIONAL PARAMETERS								   */
				/*******************************************************************/

				bool wrap = false;

				sprintf( buf, "layer%duwrap", layer+1 );
				if( hPass.Get( buf, wrap ) )
				{
					nInfo.m_uWrap = wrap ? 1 : 0;
				}

				sprintf( buf, "layer%dvwrap", layer+1 );
				if( hPass.Get( buf, wrap ) )
				{
					nInfo.m_vWrap = wrap ? 1 : 0;
				}

				gpstring textureinfo;

				sprintf( buf, "layer%dcolorop", layer+1 );
				if( hPass.Get( buf, textureinfo ) )
				{
					nInfo.m_colorOp = GetEnumeratedOpValueFromString( textureinfo );
				}

				sprintf( buf, "layer%dcolorarg1", layer+1 );
				if( hPass.Get( buf, textureinfo ) )
				{
					nInfo.m_colorArg1 = GetEnumeratedArgValueFromString( textureinfo );
				}

				sprintf( buf, "layer%dcolorarg2", layer+1 );
				if( hPass.Get( buf, textureinfo ) )
				{
					nInfo.m_colorArg2 = GetEnumeratedArgValueFromString( textureinfo );
				}

				sprintf( buf, "layer%dalphaop", layer+1 );
				if( hPass.Get( buf, textureinfo ) )
				{
					nInfo.m_alphaOp = GetEnumeratedOpValueFromString( textureinfo );
				}

				sprintf( buf, "layer%dalphaarg1", layer+1 );
				if( hPass.Get( buf, textureinfo ) )
				{
					nInfo.m_alphaArg1 = GetEnumeratedArgValueFromString( textureinfo );
				}

				sprintf( buf, "layer%dalphaarg2", layer+1 );
				if( hPass.Get( buf, textureinfo ) )
				{
					nInfo.m_alphaArg2 = GetEnumeratedArgValueFromString( textureinfo );
				}

				surfaceDescription.m_SurfaceLayers.push_back( nInfo );
			}
		}
	}
	else
	{
		// This texture has no descriptor, so just put the name of the texture into the first slot
		// This should eventually be taken out, because we should have a TSD for EVERY texture, not just
		// animated ones.
		m_pSurfaceDescription	= new TextureSurfaceDescription[ 1 ];
		m_pbActive				= new bool[ 1 ];
		TextureSurfaceInfo nInfo;
		nInfo.m_textureNames.push_back( m_Pathname );
		m_pSurfaceDescription[0].m_SurfaceLayers.push_back( nInfo );
	}
}

// Restore this texture by going through it owned surfaces and restoring them
bool RapiTexture::Load()
{
	for( unsigned int p = 0; p < m_numPasses; ++p )
	{
		TextureSurfaceDescription& surfaceDescription	= m_pSurfaceDescription[p];
		for( int i = 0; i < surfaceDescription.m_numLayers; ++i )
		{
			for( std::vector< unsigned int >::iterator t = surfaceDescription.m_SurfaceLayers[i].m_textureSurface.begin();
				 t != surfaceDescription.m_SurfaceLayers[i].m_textureSurface.end(); ++t )
			{
				if( (*t) != ENVIRONMENT )
				{
					m_pDevice->LoadSurface( (*t) );
				}
			}
		}
	}

	return true;
}

// Release this texture by going through its owned surfaces and releasing them
bool RapiTexture::Unload()
{
	for( unsigned int p = 0; p < m_numPasses; ++p )
	{
		TextureSurfaceDescription& surfaceDescription	= m_pSurfaceDescription[p];
		for( int i = 0; i < surfaceDescription.m_numLayers; ++i )
		{
			for( std::vector< unsigned int >::iterator t = surfaceDescription.m_SurfaceLayers[i].m_textureSurface.begin();
				 t != surfaceDescription.m_SurfaceLayers[i].m_textureSurface.end(); ++t )
			{
				if( (*t) != ENVIRONMENT )
				{
					m_pDevice->UnloadSurface( (*t) );
				}
			}
		}
	}

	return true;
}

void RapiTexture::IncrementReferenceCount()
{
	++m_refCount;

#	if !GP_RETAIL
	if ( gRapiOwner.IsTracingTextures() )
	{
		gpgenericf(( "(0x%08X) ++refcount %s -> %d\n", this, m_Pathname.c_str(), m_refCount ));
	}
#	endif // !GP_RETAIL
}

void RapiTexture::DecrementReferenceCount()
{
	--m_refCount;

#	if !GP_RETAIL
	if ( gRapiOwner.IsTracingTextures() )
	{
		gpgenericf(( "(0x%08X) --refcount %s -> %d\n", this, m_Pathname.c_str(), m_refCount ));
	}
#	endif // !GP_RETAIL
}

void RapiTexture::IncrementLoadReferenceCount()
{
	++m_loadRefCount;

#	if !GP_RETAIL
	if ( gRapiOwner.IsTracingTextures() )
	{
		gpgenericf(( "(0x%08X) ++loadrefcount %s -> %d\n", this, m_Pathname.c_str(), m_loadRefCount ));
	}
#	endif // !GP_RETAIL
}

void RapiTexture::DecrementLoadReferenceCount()
{
	--m_loadRefCount;

#	if !GP_RETAIL
	if ( gRapiOwner.IsTracingTextures() )
	{
		gpgenericf(( "(0x%08X) --loadrefcount %s -> %d\n", this, m_Pathname.c_str(), m_loadRefCount ));
	}
#	endif // !GP_RETAIL
}

// Returns the enumerated op value of the D3D argument passed in string form.
int	RapiTexture::GetEnumeratedOpValueFromString( gpstring& value )
{
	if( value.same_no_case( "DISABLE" ) ){
		return D3DTOP_DISABLE;
	}
	if( value.same_no_case( "ARG1" ) ){
		return D3DTOP_SELECTARG1;
	}
	if( value.same_no_case( "ARG2" ) ){
		return D3DTOP_SELECTARG2;
	}
	if( value.same_no_case( "MODULATE" ) ){
		return D3DTOP_MODULATE;
	}
	if( value.same_no_case( "MODULATE2X" ) ){
		if( m_pDevice->IsModulateOnly() )
		{
			return D3DTOP_MODULATE;
		}
		else
		{
			return D3DTOP_MODULATE2X;
		}
	}
	if( value.same_no_case( "MODULATE4X" ) ){
		if( m_pDevice->IsModulateOnly() )
		{
			return D3DTOP_MODULATE;
		}
		else
		{
			return D3DTOP_MODULATE4X;
		}
	}
	if( value.same_no_case( "ADD" ) ){
		return D3DTOP_ADD;
	}
	if( value.same_no_case( "ADDSIGNED" ) ){
		return D3DTOP_ADDSIGNED;
	}
	if( value.same_no_case( "ADDSIGNED2X" ) ){
		return D3DTOP_ADDSIGNED2X;
	}
	if( value.same_no_case( "SUBTRACT" ) ){
		return D3DTOP_SUBTRACT;
	}
	if( value.same_no_case( "ADDSMOOTH" ) ){
		return D3DTOP_ADDSMOOTH;
	}
	if( value.same_no_case( "BLENDDIFFUSEALPHA" ) ){
		return D3DTOP_BLENDDIFFUSEALPHA;
	}
	if( value.same_no_case( "BLENDTEXTUREALPHA" ) ){
		return D3DTOP_BLENDTEXTUREALPHA;
	}
	if( value.same_no_case( "BLENDCURRENTALPHA" ) ){
		return D3DTOP_BLENDCURRENTALPHA;
	}
	if( value.same_no_case( "BLENDTEXTUREALPHAPM" ) ){
		return D3DTOP_BLENDTEXTUREALPHAPM;
	}
	if( value.same_no_case( "DOTPRODUCT3" ) ){
		return D3DTOP_DOTPRODUCT3;
	}
	if( value.same_no_case( "PREMODULATE" ) ){
		return D3DTOP_PREMODULATE;
	}
	if( value.same_no_case( "MODULATEALPHA_ADDCOLOR" ) ){
		return D3DTOP_MODULATEALPHA_ADDCOLOR;
	}
	if( value.same_no_case( "MODULATECOLOR_ADDALPHA" ) ){
		return D3DTOP_MODULATECOLOR_ADDALPHA;
	}
	if( value.same_no_case( "MODULATEINVALPHA_ADDCOLOR" ) ){
		return D3DTOP_MODULATEINVALPHA_ADDCOLOR;
	}
	if( value.same_no_case( "MODULATEINVCOLOR_ADDALPHA" ) ){
		return D3DTOP_MODULATEINVCOLOR_ADDALPHA;
	}

	gpassertf( 0, ("Could not identify an op value from a TSD description = %s", m_Pathname.c_str()) );
	return -1;
}

// Returns the enumerated arg value of the D3D argument passed in string form.
int	RapiTexture::GetEnumeratedArgValueFromString( gpstring& value )
{
	if( same_no_case( value.c_str(), "TEXTURE" ) ){
		return D3DTA_TEXTURE;
	}
	if( same_no_case( value.c_str(), "DIFFUSE" ) ){
		return D3DTA_DIFFUSE;
	}
	if( same_no_case( value.c_str(), "CURRENT" ) ){
		return D3DTA_CURRENT;
	}
	if( same_no_case( value.c_str(), "TFACTOR" ) ){
		return D3DTA_TFACTOR;
	}

	gpassertf( 0, ("Could not identify an arg value from a TSD description = %s", m_Pathname.c_str()) );
	return -1;
}

// Returns the enumerated blend value of the D3D argument pass in string form.
int RapiTexture::GetEnumeratedBlendValueFromString( gpstring& value )
{
	if( same_no_case( value.c_str(), "ZERO" ) ){
		return D3DBLEND_ZERO;
	}
	if( same_no_case( value.c_str(), "ONE" ) ){
		return D3DBLEND_ONE;
	}
	if( same_no_case( value.c_str(), "SRCCOLOR" ) ){
		return D3DBLEND_SRCCOLOR;
	}
	if( same_no_case( value.c_str(), "INVSRCCOLOR" ) ){
		return D3DBLEND_INVSRCCOLOR;
	}
	if( same_no_case( value.c_str(), "SRCALPHA" ) ){
		return D3DBLEND_SRCALPHA;
	}
	if( same_no_case( value.c_str(), "INVSRCALPHA" ) ){
		return D3DBLEND_INVSRCALPHA;
	}
	if( same_no_case( value.c_str(), "DESTALPHA" ) ){
		return D3DBLEND_DESTALPHA;
	}
	if( same_no_case( value.c_str(), "INVDESTALPHA" ) ){
		return D3DBLEND_INVDESTALPHA;
	}
	if( same_no_case( value.c_str(), "DESTCOLOR" ) ){
		return D3DBLEND_DESTCOLOR;
	}
	if( same_no_case( value.c_str(), "INVDESTCOLOR" ) ){
		return D3DBLEND_INVDESTCOLOR;
	}
	if( same_no_case( value.c_str(), "SRCALPHASAT" ) ){
		return D3DBLEND_SRCALPHASAT;
	}
	if( same_no_case( value.c_str(), "BOTHSRCALPHA" ) ){
		return D3DBLEND_BOTHSRCALPHA;
	}
	if( same_no_case( value.c_str(), "BOTHINVSRCALPHA" ) ){
		return D3DBLEND_BOTHINVSRCALPHA;
	}


	gpassertf( 0, ("Could not identify an arg value from a TSD description = %s", m_Pathname.c_str()) );
	return -1;
}

// Setup this surface as the currently active surface
void RapiTexture::SetAsActiveSurface( double timeElapsed, double secondTime, unsigned int stage, unsigned int pass )
{
	// Check the pass info and get the right pass
	if( pass >= m_numPasses )
	{
		gperror( "Cannot request a pass if the texture does not have that information!" );
		return;
	}
	TextureSurfaceDescription& surfaceDescription	= m_pSurfaceDescription[ pass ];

	// Setup the layer info
	int layer	= (int)stage;

	for( int i = 0; (i < surfaceDescription.m_numLayers) && (stage < MAX_TEXTURE_LAYERS); ++i, ++stage )
	{
		TextureSurfaceInfo& surfInfo	= surfaceDescription.m_SurfaceLayers[i];

		// Advance the animation if neccesary
		if( (surfInfo.m_secondsPerFrame > 0.0f && !m_pbActive[ pass ]) && surfaceDescription.m_bAnimationState)
		{
			const float modifiedSecondsPerFrame	= surfInfo.m_secondsPerFrame * surfaceDescription.m_animationSpeed;
			if( !surfaceDescription.m_syncTimer )
			{
				surfInfo.m_currentFrameTime		+= timeElapsed;
				if( surfInfo.m_currentFrameTime	> modifiedSecondsPerFrame )
				{
					++surfInfo.m_currentFrame;
					if( surfInfo.m_currentFrame	== surfInfo.m_numFrames )
					{
						// Reset the frame
						surfInfo.m_currentFrame	= 0;
					}
					surfInfo.m_currentFrameTime	-= modifiedSecondsPerFrame;
				}
			}
			else
			{
				double sync_time	= secondTime - surfInfo.m_currentFrameTime;
				if( sync_time > modifiedSecondsPerFrame )
				{
					// Calculate how many frames we need to advance to catch up with the second timer
					unsigned int numFramesToAdvance	= FTOL( sync_time / modifiedSecondsPerFrame );

					// Advance the frames
					surfInfo.m_currentFrame		+= (numFramesToAdvance % surfInfo.m_numFrames);
					if( surfInfo.m_currentFrame	>= surfInfo.m_numFrames )
					{
						// Reset the frame
						surfInfo.m_currentFrame	= 0;
					}
					surfInfo.m_currentFrameTime	+= (numFramesToAdvance * modifiedSecondsPerFrame);
				}
			}
		}

		bool bEnvironment = (surfInfo.m_textureSurface[ surfInfo.m_currentFrame ] == ENVIRONMENT);

		if( bEnvironment )
		{
			// Setup the stage texture transform
			TRYDX (m_pDevice->GetDevice()->SetTextureStageState( stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 ) );

			// Build the matrix
			D3DMATRIX mat;
			memset( &mat, 0, sizeof( D3DMATRIX ) );
			mat._11	= 0.5f;
			mat._22 = -0.5f;
			mat._33 = 0.0f;
			mat._44 = 1.0f;
			mat._41 = 0.5f;
			mat._42 = -0.5f;

			TRYDX (m_pDevice->GetDevice()->SetTransform( D3DTRANSFORMSTATETYPE(D3DTS_TEXTURE0 + stage), &mat ) );

			// Setup the parameters to be passed for the transform
			TRYDX (m_pDevice->GetDevice()->SetTextureStageState( stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR ) );
		}

		if( (surfInfo.m_ushiftPerSecond || surfInfo.m_vshiftPerSecond || surfInfo.m_radrotPerSecond) && !bEnvironment )
		{
			// Increment our UV animation and rotation info containers
			if( !m_pbActive[ pass ] && surfaceDescription.m_bAnimationState )
			{
				const float modifiedUShift	= surfInfo.m_ushiftPerSecond * surfaceDescription.m_animationSpeed;
				const float modifiedVShift	= surfInfo.m_vshiftPerSecond * surfaceDescription.m_animationSpeed;
				const float modifiedRot		= surfInfo.m_radrotPerSecond * surfaceDescription.m_animationSpeed;

				if( !surfaceDescription.m_syncTimer )
				{
					surfInfo.m_currentUshift		+= timeElapsed * modifiedUShift;
					surfInfo.m_currentVshift		+= timeElapsed * modifiedVShift;
					surfInfo.m_currentRotation		+= timeElapsed * modifiedRot;

					if( surfInfo.m_currentUshift	>= 1.0 )
					{
						surfInfo.m_currentUshift	-= 1.0;
					}
					if( surfInfo.m_currentVshift	>= 1.0 )
					{
						surfInfo.m_currentVshift	-= 1.0;
					}
					if( surfInfo.m_currentRotation	>= RADIANS_360 )
					{
						surfInfo.m_currentRotation	-= RADIANS_360;
					}
				}
				else
				{
					double ushift					= secondTime * modifiedUShift;
					double vshift					= secondTime * modifiedVShift;
					double rot						= (secondTime * modifiedRot) / RADIANS_360;
					surfInfo.m_currentUshift		= ushift - FTOL( ushift );
					surfInfo.m_currentVshift		= vshift - FTOL( vshift );
					surfInfo.m_currentRotation		= (rot - FTOL( rot )) * RADIANS_360;
				}
			}

			// Setup the stage texture transform
			TRYDX (m_pDevice->GetDevice()->SetTextureStageState( stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 ) );

			// Build the matrix
			D3DMATRIX mat;
			memset( &mat, 0, sizeof( D3DMATRIX ) );
			mat._11	= 1.0f;
			mat._22	= 1.0f;
			mat._33	= 1.0f;
			mat._44	= 1.0f;

			if( !IsZero( (float)surfInfo.m_currentRotation ) )
			{
				float sinrad, cosrad;
				SINCOSF( (float)surfInfo.m_currentRotation, sinrad, cosrad );

				matrix_3x3 result;
				result.v00		= cosrad;
				result.v01		= -sinrad;
				result.v10		= sinrad;
				result.v11		= cosrad;

				// Offset rotate by concatenation
				if( surfInfo.m_uRotCenter != 0.0f || surfInfo.m_vRotCenter != 0.0f )
				{
					matrix_3x3 trans;

					// Translate and rotate
					trans.v20	= -surfInfo.m_uRotCenter;
					trans.v21	= -surfInfo.m_vRotCenter;

					result		= trans * result;

					// Translate back
					trans.v20	= -trans.v20;
					trans.v21	= -trans.v21;

					result		= result * trans;

					// Add shift
					trans.v20	= (float)surfInfo.m_currentUshift;
					trans.v21	= (float)surfInfo.m_currentVshift;

					result		= result * trans;
				}
				else
				{
					result.v20	= (float)surfInfo.m_currentUshift;
					result.v21	= (float)surfInfo.m_currentVshift;
				}

				mat._11	= result.v00;
				mat._12 = result.v01;
				mat._13 = result.v02;
				mat._21 = result.v10;
				mat._22 = result.v11;
				mat._23	= result.v12;
				mat._31	= result.v20;
				mat._32	= result.v21;
				mat._33	= result.v22;
			}
			else
			{
				mat._31	= (float)surfInfo.m_currentUshift;
				mat._32	= (float)surfInfo.m_currentVshift;
			}

			TRYDX (m_pDevice->GetDevice()->SetTransform( D3DTRANSFORMSTATETYPE(D3DTS_TEXTURE0 + stage), &mat ) );
			TRYDX (m_pDevice->GetDevice()->SetTextureStageState( stage, D3DTSS_TEXCOORDINDEX, 0 ) );
		}

		// See if need any texture transform for this surface
		if( !bEnvironment && !(surfInfo.m_ushiftPerSecond || surfInfo.m_vshiftPerSecond || surfInfo.m_radrotPerSecond) )
		{
			// Disable texture transformations
			TRYDX (m_pDevice->GetDevice()->SetTextureStageState( stage, D3DTSS_TEXCOORDINDEX, 0 ) );
			TRYDX (m_pDevice->GetDevice()->SetTextureStageState( stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE ) );
		}

		// Get the surface
		RapiSurface* pSurface	= bEnvironment ? m_pDevice->m_Surfaces[ m_pDevice->GetEnvironmentTexture() ] : m_pDevice->m_Surfaces[ surfInfo.m_textureSurface[ surfInfo.m_currentFrame ] ];
		if( pSurface )
		{
			gpassert( pSurface->GetStage() == (int)stage );

#if !GP_DEBUG

			if( !pSurface->GetSurface() )
			{
				m_pDevice->SetNoRender( true );
			}
			else
			{
				m_pDevice->SetNoRender( false );
			}

#endif

			// Set the current stage to this texture
			TRYDX (m_pDevice->GetDevice()->SetTexture( stage, pSurface->GetSurface() ) );

			// Update the cache
			m_pDevice->m_ActiveTex[ stage ]	= this;

			// Tell this texture that it was active this frame
			pSurface->SetActive( true );
		}
	}

	// Set the wrapping mode, ops and arguments
	SetSurfaceInformation( layer, pass );

	// Set the active status
	m_pbActive[ pass ]	= true;
}

// Set this texture as the render target
bool RapiTexture::SetAsRenderTarget()
{
	// Get the surface
	RapiSurface* pSurface	= m_pDevice->m_Surfaces[ m_pSurfaceDescription[0].m_SurfaceLayers[0].m_textureSurface[0] ];
	if( pSurface && pSurface->GetRenderTarget() && pSurface->GetSurface() )
	{
		LPDIRECT3DSURFACE8 pTarget = NULL;
		if( SUCCEEDED( TRYDX( pSurface->GetSurface()->GetSurfaceLevel( 0, &pTarget ) ) ) )
		{
			// Set the render target to the new surface
			TRYDX( m_pDevice->GetDevice()->SetRenderTarget( pTarget, 0 ) );

			// Clear the new target
			TRYDX( m_pDevice->GetDevice()->Clear( 0, NULL, D3DCLEAR_TARGET, 0xFFFFFFFF, 0.0f, 0 ) );
			
			// Release the increased ref count
			RELEASE( pTarget );
		}

		// Indicate success
		return true;
	}

	// Failure
	return false;
}

// Set the wrapping mode, ops and arguments
void RapiTexture::SetSurfaceInformation( int layer, int pass )
{
	// Check the pass info and get the right pass
	if( pass >= (int)m_numPasses )
	{
		gperror( "Cannot request a pass if the texture does not have that information!" );
		return;
	}
	TextureSurfaceDescription& surfaceDescription	= m_pSurfaceDescription[ pass ];

	// Enable alpha testing if this surface is marked as an alpha surface
	if( (surfaceDescription.m_hasAlpha || surfaceDescription.m_alphaTest) && !m_pDevice->m_bAlphaTest )
	{
		TRYDX( m_pDevice->GetDevice()->SetRenderState( D3DRS_ALPHATESTENABLE,	TRUE ) );
		TRYDX( m_pDevice->GetDevice()->SetRenderState( D3DRS_ALPHAFUNC,			D3DCMP_GREATER ) );
		m_pDevice->m_bAlphaTest	= true;
	}

	for( int i = 0; (i < surfaceDescription.m_numLayers) && (layer < MAX_TEXTURE_LAYERS); ++i, ++layer )
	{
		TextureSurfaceInfo& surfInfo	= surfaceDescription.m_SurfaceLayers[i];
		TextureStageState& stageState	= m_pDevice->m_TextureStageStates[i];

		if( surfInfo.m_uWrap != -1 && ((stageState.addressU == D3DTADDRESS_WRAP) != surfInfo.m_uWrap) )
		{
			stageState.addressU	= surfInfo.m_uWrap ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
			TRYDX( m_pDevice->GetDevice()->SetTextureStageState( i, D3DTSS_ADDRESSU,	stageState.addressU ) );
		}
		if( surfInfo.m_vWrap != -1 && ((stageState.addressV == D3DTADDRESS_WRAP) != surfInfo.m_vWrap))
		{
			stageState.addressV	= surfInfo.m_vWrap ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
			TRYDX( m_pDevice->GetDevice()->SetTextureStageState( i, D3DTSS_ADDRESSV,	stageState.addressV ) );
		}

		if( m_pDevice->IsIgnoringVertexColor() && (i == 0) )
		{
			if( stageState.colorOp != D3DTOP_SELECTARG1 )
			{
				TRYDX( m_pDevice->GetDevice()->SetTextureStageState( i, D3DTSS_COLOROP,	D3DTOP_SELECTARG1 ) );
				stageState.colorOp	= D3DTOP_SELECTARG1;
			}
		}
		else if( surfInfo.m_colorOp != -1 && (int)stageState.colorOp != surfInfo.m_colorOp )
		{
			TRYDX( m_pDevice->GetDevice()->SetTextureStageState( i, D3DTSS_COLOROP,		D3DTEXTUREOP(surfInfo.m_colorOp) ) );
			stageState.colorOp	= D3DTEXTUREOP(surfInfo.m_colorOp);
		}

		if( surfInfo.m_alphaOp != -1 && (int)stageState.alphaOp != surfInfo.m_alphaOp )
		{
			TRYDX( m_pDevice->GetDevice()->SetTextureStageState( i, D3DTSS_ALPHAOP,		D3DTEXTUREOP(surfInfo.m_alphaOp) ) );
			stageState.alphaOp	= D3DTEXTUREOP(surfInfo.m_alphaOp);
		}

		if( surfInfo.m_colorArg1 != -1 && (int)stageState.colorArg1 != surfInfo.m_colorArg1 )
		{
			TRYDX( m_pDevice->GetDevice()->SetTextureStageState( i, D3DTSS_COLORARG1,	surfInfo.m_colorArg1 ) );
			stageState.colorArg1	= surfInfo.m_colorArg1;
		}
		if( surfInfo.m_colorArg2 != -1 && (int)stageState.colorArg2 != surfInfo.m_colorArg2)
		{
			TRYDX( m_pDevice->GetDevice()->SetTextureStageState( i, D3DTSS_COLORARG2,	surfInfo.m_colorArg2 ) );
			stageState.colorArg2	= surfInfo.m_colorArg2;
		}
		if( surfInfo.m_alphaArg1 != -1 && (int)stageState.alphaArg1 != surfInfo.m_alphaArg1 )
		{
			TRYDX( m_pDevice->GetDevice()->SetTextureStageState( i, D3DTSS_ALPHAARG1,	surfInfo.m_alphaArg1 ) );
			stageState.alphaArg1	= surfInfo.m_alphaArg1;
		}
		if( surfInfo.m_alphaArg2 != -1 && (int)stageState.alphaArg2 != surfInfo.m_alphaArg2 )
		{
			TRYDX( m_pDevice->GetDevice()->SetTextureStageState( i, D3DTSS_ALPHAARG2,	surfInfo.m_alphaArg2 ) );
			stageState.alphaArg2	= surfInfo.m_alphaArg2;
		}
	}
	if( layer < MAX_TEXTURE_LAYERS )
	{
		TextureStageState& stageState	= m_pDevice->m_TextureStageStates[layer];
		if( stageState.colorOp != D3DTOP_DISABLE )
		{
			TRYDX( m_pDevice->GetDevice()->SetTextureStageState( layer, D3DTSS_COLOROP,	D3DTOP_DISABLE ) );
			stageState.colorOp	= D3DTOP_DISABLE;
		}
		if( stageState.alphaOp != D3DTOP_DISABLE )
		{
			TRYDX( m_pDevice->GetDevice()->SetTextureStageState( layer, D3DTSS_ALPHAOP,	D3DTOP_DISABLE ) );
			stageState.alphaOp	= D3DTOP_DISABLE;
		}
	}

	// Set the blend modes
	if( surfaceDescription.m_srcBlend != -1 && (int)m_pDevice->m_srcBlend != surfaceDescription.m_srcBlend )
	{
		TRYDX( m_pDevice->GetDevice()->SetRenderState( D3DRS_SRCBLEND,		surfaceDescription.m_srcBlend ) );
		m_pDevice->m_srcBlend	= (D3DBLEND)surfaceDescription.m_srcBlend;
	}
	if( surfaceDescription.m_destBlend != -1 && (int)m_pDevice->m_dstBlend != surfaceDescription.m_destBlend )
	{
		TRYDX( m_pDevice->GetDevice()->SetRenderState( D3DRS_DESTBLEND,	surfaceDescription.m_destBlend ) );
		m_pDevice->m_dstBlend	= (D3DBLEND)surfaceDescription.m_destBlend;
	}
}

// Set the state of the texture animation
void RapiTexture::SetAnimationState( bool bState )
{
	for( unsigned int p = 0; p < m_numPasses; ++p )
	{
		m_pSurfaceDescription[p].m_bAnimationState = bState;
	}
}

// Set the speed of the texture animation
void RapiTexture::SetAnimationSpeed( float speed )
{
	for( unsigned int p = 0; p < m_numPasses; ++p )
	{
		m_pSurfaceDescription[p].m_animationSpeed = speed;
	}
}

// Active status this frame
void RapiTexture::SetActive( bool active )
{
	for( unsigned int p = 0; p < m_numPasses; ++p )
	{
		m_pbActive[p] = active;
	}
}

// Saturation
void RapiTexture::SetSaturation( float saturation )
{
	// Set the proper state
	m_bSaturated	= true;

	// Tell each sub surface of each pass of this texture that it needs to change saturation
	for( unsigned int p = 0; p < m_numPasses; ++p )
	{
		TextureSurfaceDescription& surfaceDescription	= m_pSurfaceDescription[p];
		for( int i = 0; i < surfaceDescription.m_numLayers; ++i )
		{
			for( int o = 0; o < surfaceDescription.m_SurfaceLayers[i].m_numFrames; ++o )
			{
				RapiSurface* pSurface = m_pDevice->m_Surfaces[ surfaceDescription.m_SurfaceLayers[i].m_textureSurface[o] ];
				if( pSurface )
				{
					pSurface->SetSaturation( saturation );
				}
			}
		}
	}
}

// Return saturated texture back to normal
void RapiTexture::RevertSaturation()
{
	// Tell each sub surface of each pass of this texture that it needs to revert
	for( unsigned int p = 0; p < m_numPasses; ++p )
	{
		TextureSurfaceDescription& surfaceDescription	= m_pSurfaceDescription[p];
		for( int i = 0; i < surfaceDescription.m_numLayers; ++i )
		{
			for( int o = 0; o < surfaceDescription.m_SurfaceLayers[i].m_numFrames; ++o )
			{
				RapiSurface* pSurface = m_pDevice->m_Surfaces[ surfaceDescription.m_SurfaceLayers[i].m_textureSurface[o] ];
				if( pSurface )
				{
					pSurface->RevertSaturation( true );
				}
			}
		}
	}

	// Set the proper state
	m_bSaturated	= false;
}

