/*****************************************************************************************
**
**								RapiSurface
**
**		See .h file for details.
**
*****************************************************************************************/

#include "precomp_rapi.h"

#include "GpStatsDefs.h"
#include "RapiOwner.h"
#include "RapiImage.h"


// Construction of algorithmic textures
RapiSurface::RapiSurface(
						Rapi*				pOwner,
						const gpstring&	filename,
						bool				bMipmapping,
						int					rStage,
						TEX_OPT				rOptimize,
						bool				bAlpha,
						bool				bDither,
						int					texDetail,
						bool				bFullLoad,
						gpstring&			errorSink )
						: m_pDevice( pOwner )
						, m_bValid( false )
						, m_pSurface( NULL )
						, m_pOriginalSurface( NULL )
						, m_Image( NULL )
						, m_Pathname( filename )
						, m_bMipmapping( bMipmapping && !pOwner->GetNoTextures() )
						, m_rStage( rStage )
						, m_texOpt( rOptimize )
						, m_refCount( 0 )
						, m_loadRefCount( bFullLoad ? 1 : 0 )
						, m_Width( 0 )
						, m_Height( 0 )
						, m_Color( 0x00000000 )
						, m_bAlpha( bAlpha )
						, m_bDither( bDither )
						, m_bMatchPrimary( false )
						, m_bRenderTarget( false )
						, m_bActive( false )
						, m_saturation( 1.0f )
						, m_texDetail( texDetail )
#						if GP_ENABLE_STATS
						, m_OwningSystem( SP_NONE )
#						endif // GP_ENABLE_STATS
{
	// Setup this texture
	if( bFullLoad && !RestoreSurface( &errorSink ) )
	{
		// Ack!
		errorSink.appendf( "Texture Creation failed!  %s\n", filename.c_str() );
	}
}

// Construction of algorithmic texture
RapiSurface::RapiSurface(
						Rapi*			pOwner,
						RapiMemImage*	takeImage,
						bool			bMipmapping,
						int				rStage,
						TEX_OPT			rOptimize,
						bool			bAlpha,
						bool			bDither,
						int				texDetail,
						gpstring&		errorSink )
						: m_pDevice( pOwner )
						, m_bValid( false )
						, m_pSurface( NULL )
						, m_pOriginalSurface( NULL )
						, m_Image( takeImage )
						, m_Pathname( "" )
						, m_bMipmapping( bMipmapping && !pOwner->GetNoTextures() )
						, m_rStage( rStage )
						, m_texOpt( rOptimize )
						, m_refCount( 0 )
						, m_loadRefCount( 1 )
						, m_Width( takeImage->GetWidth() )
						, m_Height( takeImage->GetHeight() )
						, m_Color( 0x00000000 )
						, m_bAlpha( bAlpha )
						, m_bDither( bDither )
						, m_bMatchPrimary( false )
						, m_bRenderTarget( false )
						, m_bActive( false )
						, m_saturation( 1.0f )
						, m_texDetail( texDetail )
#						if GP_ENABLE_STATS
						, m_OwningSystem( SP_NONE )
#						endif // GP_ENABLE_STATS
{
	// Make sure we have a valid image
	if( m_Image == NULL )
	{
		return;
	}

	if( !RestoreSurface( &errorSink ) )
	{
		// Ack!
		errorSink.append( "Algorithmic texture creation failed!\n" );
	}
}

// Construction of blank texture
RapiSurface::RapiSurface(
						Rapi*				pOwner,
						int					width,
						int					height,
						DWORD				color,
						TEX_OPT				rOptimize,
						bool				bAlpha,
						bool				bDither,
						bool				bMatchPrimary,
						bool				bRenderTarget,
						gpstring&			errorSink )
						: m_pDevice( pOwner )
						, m_bValid( false )
						, m_pSurface( NULL )
						, m_pOriginalSurface( NULL )
						, m_Image( NULL )
						, m_Pathname( "" )
						, m_bMipmapping( false )
						, m_rStage( 0 )
						, m_texOpt( rOptimize )
						, m_refCount( 0 )
						, m_loadRefCount( 1 )
						, m_Width( width )
						, m_Height( height )
						, m_Color( color )
						, m_bAlpha( bAlpha )
						, m_bDither( bDither )
						, m_bMatchPrimary( bMatchPrimary )
						, m_bRenderTarget( bRenderTarget )
						, m_bActive( false )
						, m_saturation( 1.0f )
						, m_texDetail( 0 )
#						if GP_ENABLE_STATS
						, m_OwningSystem( SP_NONE )
#						endif // GP_ENABLE_STATS
{
	if( !RestoreSurface( &errorSink ) )
	{
		// Ack!
		errorSink.append( "Blank texture creation failed!\n" );
	}
}

// Destruction
RapiSurface::~RapiSurface()
{
	Destroy();

	delete ( m_Image );
}

// Cleanup all information for this texture
void RapiSurface::Destroy()
{
#	if GP_ENABLE_STATS
	if ( m_pSurface != NULL )
	{
		GPSTATS_SAMPLE( AddDxReleaseSurface( m_OwningSystem, m_Width, m_Height, m_pDevice->GetBitsPerPixel() / 8, m_bMipmapping ) );
	}
#	endif // GP_ENABLE_STATS

	// Remove system memory for tracking
	if( m_pSurface )
	{
		m_pDevice->m_TexManagerSystemMemory	-= (unsigned int )((float)(m_Width * m_Height * (m_pDevice->GetBitsPerPixel() / 8)) * 1.333333333f);

		// Make sure this only happens on primary
		if( m_pDevice->m_primaryThreadId != ::GetCurrentThreadId() )
		{
			gperror( "Cannot destroy DX surface on separate thread!" );
		}
	}

	m_bValid = false;
	RELEASE( m_pSurface );
	m_pOriginalSurface = NULL;
}

// Invalidate the texture
void RapiSurface::InvalidateSurface()
{
	Destroy();
}

// Restore this texture
bool RapiSurface::RestoreSurface( gpstring* pErrorSink )
{
	kerneltool::Critical::Lock autoLock( m_pDevice->m_RestoreCritical );

	if( !m_loadRefCount )
	{
		return true;
	}

	// Make sure everything is all cleaned up first
	if( IsOne( m_saturation ) )
	{
		Destroy();
	}
	else
	{
		RevertSaturation( false );
	}

	// Reader goes here
	RapiImageReader* reader = NULL;

	// See if this texture is algorithmic or file
	if ( m_Image != NULL )
	{
		// Assume algorithmic
		reader = RapiImageReader::CreateReader( m_Pathname.empty() ? "<algorithmic>" : m_Pathname.c_str(), *m_Image, m_bMipmapping, false );
	}
	else if( !m_Pathname.empty() )
	{
		// Assume file based
		if ( !m_pDevice->GetNoTextures() )
		{
			reader = RapiImageReader::CreateReader( m_Pathname, m_bMipmapping, false );
			if( !reader )
			{
				if( pErrorSink )
				{
					pErrorSink->appendf( "A reader for texture %s could not be created.  Perhaps file does not exist?\n", m_Pathname.c_str() );
				}
				return false;
			}
		}
		else
		{
			// Special - fill with color
			reader = new RapiColorFiller( 4, 4, MAKEDWORDCOLOR( vector_3( Random( 1.0f ), Random( 1.0f ), Random( 1.0f ) ) ) );
		}
	}

	bool success = false;

#if !GP_RETAIL

	// Read in surface
	if ( reader != NULL )
	{
		m_Width		= reader->GetWidth();
		m_Height	= reader->GetHeight();
	}

	if( !IsPower2( m_Width ) || !IsPower2( m_Height ) )
	{
		if( pErrorSink )
		{
			pErrorSink->append( "Texture is not a power of 2!\n" );
		}
		if( reader != NULL )
		{
			delete reader;
		}
		return false;
	}

	if( ((m_Width < 4) || (m_Height < 4)) || ((m_Width > 256) || (m_Height > 256)) )
	{
		if( pErrorSink )
		{
			pErrorSink->append( "Texture does not meet min(4)/max(256) dimension requirements!\n" );
		}
		if( reader != NULL )
		{
			delete reader;
		}
		return false;
	}

	if( ((m_Width > m_Height) && ((m_Width / m_Height) > 8)) ||
		((m_Height > m_Width) && ((m_Height / m_Width) > 8)) )
	{
		if( pErrorSink )
		{
			pErrorSink->append( "Texture does not meet maximum dimension ratio requirements of 1/8 pixels!\n" );
		}
		if( reader != NULL )
		{
			delete reader;
		}
		return false;
	}

#endif

	// Read in surface
	if ( reader != NULL )
	{
		success = SetupSurface( reader );
		delete ( reader );
	}
	else
	{
		success = SetupSurface( NULL );
	}

	if( success )
	{
		m_bValid = true;
	}
	return true;
}

// Actually does creation
bool RapiSurface::SetupSurface( RapiImageReader* reader )
{
	// Skip some surfaces (reduce textures) if necessary
	if ( m_bMipmapping && reader )
	{
		int numRedux = max_t( 0, m_texDetail - m_pDevice->GetSystemDetailIdentifier() );
		while ( (numRedux--) &&
				((reader->GetWidth() > 8) &&
				(reader->GetHeight() > 8)) )
		{
			reader->SkipNextSurface();
		}
	}

	// Read in surface dimensions, because they might have changed if the detail was low
	if ( reader != NULL )
	{
		m_Width		= reader->GetWidth();
		m_Height	= reader->GetHeight();
	}

	// Correct width and height for square texture only video hardware
	if( m_pDevice->IsOnlySquareTextures() )
	{
		// Take our dimensions
		m_Width		= max_t( m_Width, m_Height );
		m_Height	= m_Width;
	}

	unsigned int numMipLevels	= m_bMipmapping ? max_t( 0, min_t( GetShift( min_t( m_Width, m_Height ) ), NUM_MIPLEVELS ) ) : 1;

	// Save the original surface
	m_pOriginalSurface	= m_pSurface;

	// Create the texture
	HRESULT hr;
	if( FAILED( hr = m_pDevice->GetDevice()->CreateTexture( min_t( m_Width, m_pDevice->GetDeviceDesc().MaxTextureWidth ),
															min_t( m_Height, m_pDevice->GetDeviceDesc().MaxTextureHeight ),
															numMipLevels, m_bRenderTarget ? D3DUSAGE_RENDERTARGET : 0,
															m_bMatchPrimary ? m_pDevice->GetPrimaryD3DFormat() : (m_bAlpha ? m_pDevice->GetAlphaTexD3DFormat() : m_pDevice->GetOpaqueTexD3DFormat()),
															m_bRenderTarget ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &m_pSurface ) ) )
	{
		RELEASE( m_pSurface );
		RELEASE( m_pOriginalSurface );
		return false;
	}

	if( IsOne( m_saturation ) )
	{
		m_pOriginalSurface	= m_pSurface;
	}

#	if GP_ENABLE_STATS
	m_OwningSystem = gpstats::GetCurrentSystem();
	GPSTATS_SAMPLE( AddDxCreateSurface( m_OwningSystem, m_Width, m_Height, m_pDevice->GetBitsPerPixel() / 8, m_bMipmapping ) );
#	endif // GP_ENABLE_STATS

	if( reader )
	{
		// Generate levels (1 if no mip, NUM_MIPLEVELS if mip )
		D3DLOCKED_RECT lockedRect;
		for( unsigned int tex_level = 0; tex_level < numMipLevels; ++tex_level )
		{
			D3DSURFACE_DESC surfaceDesc;
			m_pSurface->GetLevelDesc( tex_level, &surfaceDesc );
			
			if( SUCCEEDED( m_pSurface->LockRect( tex_level, &lockedRect, NULL, 0 ) ) )
			{
				// Copy the next surface
				reader->GetNextSurface( lockedRect.pBits, PIXEL_ARGB_8888, lockedRect.Pitch, false, m_bDither, true, surfaceDesc.Width, surfaceDesc.Height );

				// Unlock the surface now that we are done with it
				m_pSurface->UnlockRect( tex_level );


#if !GP_RETAIL
				// Add the proper amount of memory used by this surface to our global counter
				m_pDevice->m_TexManagerSystemMemory	+= surfaceDesc.Size;
#endif
			}
		}
	}
	else
	{
		// Flood fill the texture with the requested clear color??

#if !GP_RETAIL
		// Remove system memory for tracking
		if( m_pSurface )
		{
			for( unsigned int tex_level = 0; tex_level < numMipLevels; ++tex_level )
			{
				D3DSURFACE_DESC surfaceDesc;
				if( SUCCEEDED( m_pSurface->GetLevelDesc( tex_level, &surfaceDesc ) ) )
				{
					// Add the proper amount of memory used by this surface to our global counter
					m_pDevice->m_TexManagerSystemMemory	+= surfaceDesc.Size;
				}
			}
		}
#endif

	}

	// Success
	return true;
}

// Saturation
void RapiSurface::SetSaturation( float saturation )
{
	// Save current saturation
	m_saturation = saturation;

	// Will revert back to original before applying new saturation
	if( !IsOne( m_saturation ) )
	{
		RestoreSurface();
	}
	else
	{
		RevertSaturation( false );
	}
}

// Revert saturation
void RapiSurface::RevertSaturation( bool bReset )
{
	// Reset saturation
	if( bReset )
	{
		m_saturation = 1.0f;
	}

	// Make sure they differ
	if( m_pSurface != m_pOriginalSurface )
	{
		// Release and revert the surface to the original
		RELEASE( m_pSurface );
		m_pSurface = m_pOriginalSurface;
		
		// If surface is NULL, then we started in saturated mode and must
		// reload the original
		if( !m_pSurface )
		{
			RestoreSurface();
		}
	};
}




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// this code no longer necessary - it's handled in RapiImage.*, but keeping it
// around for posterity. remove at will.


#if 0

// Copy 32 bit surface
void RapiSurface::CopySurface32( DDSURFACEDESC2 &ddsd )
{
	if( m_Height == ddsd.dwHeight && m_Width == ddsd.dwWidth )
	{
		memcpy( ddsd.lpSurface, m_Image->BitsPointer(), sizeof( DWORD ) * m_Height * m_Width );
		return;
	}

	// Calculate the steps
	unsigned int ystep	= m_Height/ddsd.dwHeight;
	unsigned int xstep	= m_Width/ddsd.dwWidth;
	unsigned int numsh	= GetShift( xstep * ystep );

	// Get our surface pointer
	DWORD *pDst	= (DWORD*)ddsd.lpSurface;

	// Get our image pointer
	UINT32 *pImg = m_Image->BitsPointer();

	// Setup our runners
	unsigned int imageh = 0;

	// This is a smaller surface, so we need to blend
	unsigned int red	= 0;
	unsigned int green	= 0;
	unsigned int blue	= 0;
	unsigned int alpha	= 0;

	for( unsigned int i = 0; i < ddsd.dwHeight; ++i, imageh += ystep )
	{
		unsigned int imagew	= 0;
		for( unsigned int o = 0; o < ddsd.dwWidth; ++o, imagew += xstep, ++pDst )
		{
			// Reset our accumulators
			red = green = blue = alpha = 0;

			// Run the pixel block for our box filter
			for( unsigned int yrun = imageh; yrun < (imageh+ystep); ++yrun )
			{
				unsigned int pixelindex = yrun * m_Width;
				for( unsigned int xrun = imagew; xrun < (imagew+xstep); ++xrun )
				{
					// Get the pixel
					DWORD pixel	= pImg[ pixelindex + xrun ];

					// Add the components;
					alpha	+= (pixel >> 24) & 0xFF;
					red		+= (pixel >> 16) & 0xFF;
					green	+= (pixel >> 8)  & 0xFF;
					blue	+= pixel & 0xFF;
				}
			}

			// Divide the color components
			alpha	>>= numsh;
			red		>>= numsh;
			green	>>= numsh;
			blue	>>= numsh;

			*pDst	= ( (alpha<<24) | (red<<16) | (green<<8) | (blue) );
		}
	}
}

// Copy 16 bit surface
void RapiSurface::CopySurface16( DDSURFACEDESC2 &ddsd )
{
	// Calculate the steps
	unsigned int ystep	= m_Height/ddsd.dwHeight;
	unsigned int xstep	= m_Width/ddsd.dwWidth;
	unsigned int numsh	= GetShift( xstep * ystep );

	// Get our surface pointer
	WORD *pDst	= (WORD*)ddsd.lpSurface;

	// Get our image pointer
	UINT32 *pImg = m_Image->BitsPointer();

	// Setup our runners
	unsigned int imageh = 0;

	// This is a smaller surface, so we need to blend
	unsigned int red	= 0;
	unsigned int green	= 0;
	unsigned int blue	= 0;
	unsigned int alpha	= 0;

	for( unsigned int i = 0; i < ddsd.dwHeight; ++i, imageh += ystep )
	{
		unsigned int imagew	= 0;
		for( unsigned int o = 0; o < ddsd.dwWidth; ++o, imagew += xstep, ++pDst )
		{
			// Reset our accumulators
			red = green = blue = alpha = 0;

			// Run the pixel block for our box filter
			for( unsigned int yrun = imageh; yrun < (imageh+ystep); ++yrun )
			{
				unsigned int pixelindex = yrun * m_Width;
				for( unsigned int xrun = imagew; xrun < (imagew+xstep); ++xrun )
				{
					// Get the pixel
					DWORD pixel	= pImg[ pixelindex + xrun ];

					// Add the components;
					alpha	+= (pixel >> 24) & 0xFF;
					red		+= (pixel >> 16) & 0xFF;
					green	+= (pixel >> 8)  & 0xFF;
					blue	+= pixel & 0xFF;
				}
			}

			// Divide the color components
			alpha	>>= numsh;
			red		>>= numsh;
			green	>>= numsh;
			blue	>>= numsh;

			DWORD pixel = ( (alpha<<24) | (red<<16) | (green<<8) | (blue) );
			*pDst	= m_bAlpha ? MAKE4444FROM8888( pixel ) : MAKE1555FROM8888( pixel );
		}
	}
}

// Copy 32 bit hand made surface
void RapiSurface::CopyHandMadeSurface32( DDSURFACEDESC2 &ddsd, unsigned int surface )
{
	// The first thing we need to do is find the image pointer for the source data
	UINT32 *pImg		= m_Image->BitsPointer();

	// Add the base amount
	pImg	+= (m_Image->Width() * (m_Image->Height() / 3)) - m_Image->Width();

	if( surface > 0 )
	{
		// Add in the addition size
		unsigned int current_width	= m_Image->Width();
		for( unsigned int i = 1; i < surface; ++i )
		{
			// Divide the current width counter by 2
			current_width >>= 1;

			// Add that size to the pointer
			pImg += current_width;
		}
	}
	else
	{
		// Add the amount for the first level
		pImg += (m_Image->Width() * ((m_Image->Height() / 3) * 2));
	}

	// Get our surface pointer
	DWORD *pDst	= (DWORD*)ddsd.lpSurface;
	pDst		+= (ddsd.dwWidth * ddsd.dwHeight) - ddsd.dwWidth;

	// Now we copy the surface information
	for( unsigned int i = 0; i < ddsd.dwHeight; ++i )
	{
		// Copy the pixels
		memcpy( pDst, pImg, ddsd.dwWidth * sizeof( DWORD ) );

		// Increment the pointers
		pDst	-= ddsd.dwWidth;
		pImg	-= m_Image->Width();
	}
}

// Copy 16 bit hand made surface
void RapiSurface::CopyHandMadeSurface16( DDSURFACEDESC2 &ddsd, unsigned int surface )
{
	// The first thing we need to do is find the image pointer for the source data
	UINT32 *pImg		= m_Image->BitsPointer();

	// Add the base amount
	pImg	+= (m_Image->Width() * (m_Image->Height() / 3)) - m_Image->Width();

	if( surface > 0 )
	{
		// Add in the addition size
		unsigned int current_width	= m_Image->Width();
		for( unsigned int i = 1; i < surface; ++i )
		{
			// Divide the current width counter by 2
			current_width >>= 1;

			// Add that size to the pointer
			pImg += current_width;
		}
	}
	else
	{
		// Add the amount for the first level
		pImg += (m_Image->Width() * ((m_Image->Height() / 3) * 2));
	}

	// Get our surface pointer
	WORD *pDst	= (WORD*)ddsd.lpSurface;
	pDst		+= (ddsd.dwWidth * ddsd.dwHeight) - ddsd.dwWidth;

	// Now we copy the surface information
	if( m_bAlpha )
	{
		for( unsigned int i = 0; i < ddsd.dwHeight; ++i )
		{
			for( unsigned int o = 0; o < ddsd.dwWidth; ++o )
			{
				pDst[ o ]	= MAKE4444FROM8888( pImg[ o ] );
			}

			pDst	-= ddsd.dwWidth;
			pImg	-= m_Image->Width();
		}
	}
	else
	{
		for( unsigned int i = 0; i < ddsd.dwHeight; ++i )
		{
			for( unsigned int o = 0; o < ddsd.dwWidth; ++o )
			{
				pDst[ o ]	= MAKE1555FROM8888( pImg[ o ] );
			}

			pDst	-= ddsd.dwWidth;
			pImg	-= m_Image->Width();
		}
	}
}

#endif // 0
