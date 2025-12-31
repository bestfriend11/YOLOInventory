//////////////////////////////////////////////////////////////////////////////
//
// File     :  RapiImage.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Rapi.h"
#include "RapiImage.h"

#include "FileSysUtils.h"
#include "FileSysXfer.h"

// $ undef these to disable features
#define ENABLE_DITHERING 1
#define ENABLE_ROUNDUP   1

//////////////////////////////////////////////////////////////////////////////
// documentation

/*
	VERY IMPORTANT:

	dungeon siege by convention follows a texture coordinate system where
	0,0 is lower left and 1,1 is upper right. this is upside down for textures
	so we have to flip all images coming in...unless they are already flipped,
	which is true for bmp's with height > 0 and raw format.
*/

//////////////////////////////////////////////////////////////////////////////
// misc implementations

ePixelFormat GetPixelFormat( D3DFORMAT format )
{
	ePixelFormat pixel = PIXEL_UNKNOWN;

	switch( format )
	{
		case D3DFMT_X8R8G8B8:
		case D3DFMT_A8R8G8B8:
		{
			pixel = PIXEL_ARGB_8888;
			break;
		}

		case D3DFMT_R8G8B8:
		{
			pixel = PIXEL_RGB_888;
			break;
		}

		case D3DFMT_R5G6B5:
		{
			pixel = PIXEL_RGB_565;
			break;
		}

		case D3DFMT_X1R5G5B5:
		case D3DFMT_A1R5G5B5:
		{
			pixel = PIXEL_ARGB_1555;
			break;
		}

		case D3DFMT_X4R4G4B4:
		case D3DFMT_A4R4G4B4:
		{
			pixel = PIXEL_ARGB_4444;
			break;
		}

		default:
		{
			gperror( "Unrecognized pixel format!" );
			break;
		}
	}

	return ( pixel );
}

int GetSizeBytes( ePixelFormat e )
{
	if ( e == PIXEL_ARGB_8888 )
	{
		return ( 4 );
	}
	else if ( e == PIXEL_RGB_888 )
	{
		return ( 3 );
	}
	else if ( e != PIXEL_UNKNOWN )
	{
		return ( 2 );
	}
	else
	{
		gpassert( 0 );	// $ should never get here
		return ( 0 );
	}
}

const char* GetExtension( eImageFormat format )
{
	static const char* s_Extensions[] =
	{
		"raw", "bmp", "png", "jpg", "psd",
	};

	COMPILER_ASSERT( ELEMENT_COUNT( s_Extensions ) == IMAGE_COUNT );

	gpassert( (format >= IMAGE_BEGIN) && (format < IMAGE_END) );
	return ( s_Extensions[ format - IMAGE_BEGIN ] );
}

void BoxFilter(       DWORD* dst, int dstWidth, int dstHeight, int dstStride,
				const DWORD* src, int srcWidth, int srcHeight, int srcStride )
{
	gpassert( IsPointerValid( dst ) && IsPointerValid( src ) );
	gpassert( (dstWidth > 0) && (dstHeight > 0) && (dstStride >= dstWidth) );
	gpassert( (srcWidth > 0) && (srcHeight > 0) && (srcStride >= srcWidth) );

	if ( (srcWidth == (dstWidth * 2)) && (srcHeight == (dstHeight * 2)) )
	{
		// $ special 2x2 power of 2 optimized algo. note that this function
		//   will filter on top of itself just fine

		for ( int y = dstHeight ; y ; --y )
		{
			DWORD* out = dst, * end = out + dstWidth;
			const DWORD* row0 = src;
			const DWORD* row1 = (const DWORD*)((const BYTE*)row0 + srcStride);

			while ( out != end )
			{
				DWORD b0 = *row0++, g0 = b0 >> 8, r0 = g0 >> 8, a0 = r0 >> 8;
				DWORD b1 = *row0++, g1 = b1 >> 8, r1 = g1 >> 8, a1 = r1 >> 8;
				DWORD b2 = *row1++, g2 = b2 >> 8, r2 = g2 >> 8, a2 = r2 >> 8;
				DWORD b3 = *row1++, g3 = b3 >> 8, r3 = g3 >> 8, a3 = r3 >> 8;

				DWORD a = (BYTE)(((BYTE)a0 + (BYTE)a1 + (BYTE)a2 + (BYTE)a3) >> 2) << 24;
				DWORD r = (BYTE)(((BYTE)r0 + (BYTE)r1 + (BYTE)r2 + (BYTE)r3) >> 2) << 16;
				DWORD g = (BYTE)(((BYTE)g0 + (BYTE)g1 + (BYTE)g2 + (BYTE)g3) >> 2) << 8;
				DWORD b = (BYTE)(((BYTE)b0 + (BYTE)b1 + (BYTE)b2 + (BYTE)b3) >> 2);

				*out++ = a | r | g | b;
			}

			dst = (DWORD*)((BYTE*)dst + dstStride);
			src = (const DWORD*)((const BYTE*)src + (2 * srcStride));
		}
	}
	else
	{
		gpassert( dst != src );

		gpassert( srcHeight >= dstHeight );
		gpassert( srcWidth >= dstWidth );

		// Calculate the steps
		float ystep			= (float)srcHeight / (float)dstHeight;
		float xstep			= (float)srcWidth / (float)dstWidth;

		// Setup our runners
		int sStride			= srcStride >> 2;
		float imageh		= 0.0f;

		// This is a smaller surface, so we need to blend
		DWORD red			= 0;
		DWORD green			= 0;
		DWORD blue			= 0;
		DWORD alpha			= 0;
		DWORD count			= 0;

		DWORD* pDst			= dst;

		for( int i = 0; i < dstHeight; ++i, imageh += ystep )
		{
			DWORD dimageh	= FTOL( imageh );
			DWORD nhstep	= FTOL( imageh + ystep );
			float imagew	= 0.0f;
			for( int o = 0; o < dstWidth; ++o, imagew += xstep, ++pDst )
			{
				// Reset our accumulators
				red = green = blue = alpha = count = 0;
				DWORD dimagew	= FTOL( imagew );
				DWORD nwstep	= FTOL( imagew + xstep );

				// Run the pixel block for our box filter
				for( DWORD yrun = dimageh; yrun < nhstep; ++yrun )
				{
					DWORD pixelindex = yrun * sStride;
					for( DWORD xrun = dimagew; xrun < nwstep; ++xrun )
					{
						// Get the pixel
						DWORD pixel	= src[ pixelindex + xrun ];

						// Add the components;
						alpha	+= (pixel >> 24) & 0xFF;
						red		+= (pixel >> 16) & 0xFF;
						green	+= (pixel >> 8)  & 0xFF;
						blue	+= pixel & 0xFF;
						count	++;
					}
				}

				// Divide the color components
				alpha	/= count;
				red		/= count;
				green	/= count;
				blue	/= count;

				*pDst	= ( (alpha<<24) | (red<<16) | (green<<8) | (blue) );
			}
		}
	}
}

void CopyPixels( void* dst, ePixelFormat dstFormat, const void* src, ePixelFormat srcFormat,
				 int srcWidth, int srcHeight, int dstWidth, int dstHeight, int startY, bool allowDither, float saturation = 1.0f )
{
#	if !ENABLE_DITHERING
	UNREFERENCED_PARAMETER( allowDither );
	UNREFERENCED_PARAMETER( startY );
#	endif // !ENABLE_DITHERING

	// special: if formats are the same, easy
	if ( ((srcWidth == dstWidth && srcHeight == dstHeight) && dstFormat == srcFormat) && IsOne( saturation ) )
	{
		::memcpy( dst, src, srcWidth * srcHeight * GetSizeBytes( dstFormat ) );
	}
	else
	{
		// $$ this code can totally be sped up using mmx...

		// this is a 4x4 matrix use for fixed order dithering (got the
		// algorithm from peter freese)
#		if ENABLE_DITHERING
		static const unsigned int s_Dither[ 4 ][ 4 ] =
		{
			{  0,  8,  2, 10 },
			{ 12,  4, 14,  6 },
			{  3, 11,  1,  9 },
			{ 15,  7, 13,  5 },
		};
#		endif // ENABLE_DITHERING

		switch( srcFormat )
		{
			case PIXEL_ARGB_8888:
			{
				gpassert( srcWidth <= dstWidth && srcHeight <= dstHeight );
				const DWORD* i	= (const DWORD*)src;
				int wdup		= dstWidth / srcWidth;
				int hdup		= (dstHeight / srcHeight) - 1;

				switch( dstFormat )
				{
					case PIXEL_ARGB_4444:
					{
						WORD* o			= (WORD*)dst;

#						if ENABLE_DITHERING
						if ( allowDither )
						{
							for ( int y = startY ; y != (startY + dstHeight) ; ++y )
							{
								for ( int x = 0 ; x != dstWidth ; )
								{
									// get colors and alpha
									DWORD a = (*i >> 16) & 0xF000;
									DWORD b = ScaleDWORDColorSaturation( *i++, saturation ), g = b >> 8, r = g >> 8;

									// 4 bits of error precision
									DWORD er = (r << 1) & 0x0F;
									DWORD eg = (g << 1) & 0x0F;
									DWORD eb = (b << 1) & 0x0F;

									// convert to 4444
									r = (BYTE)r >> 4;
									g = (BYTE)g >> 4;
									b = (BYTE)b >> 4;

									// dither if not pure color
									if ( r < 0xF )  r += (er > s_Dither[y & 3][x & 3]);
									if ( g < 0xF )  g += (eg > s_Dither[y & 3][x & 3]);		// $$$ james said that green must be toned down on a dither - add this
									if ( b < 0xF )  b += (eb > s_Dither[y & 3][x & 3]);

									// write
									for ( int xr = 0; xr != wdup; ++xr, ++x )
									{
										*o++ = (WORD)(a | (r << 8) | (g << 4) | b);
									}
								}

								for ( int yr = 0; yr != hdup; ++yr, ++y )
								{
									memcpy( o, o - dstWidth, sizeof( WORD ) * dstWidth );
									o += dstWidth;
								}
							}
						}
						else
#						endif // ENABLE_DITHERING
						{
							for ( int y = 0 ; y != dstHeight ; ++y )
							{
								for ( int x = 0 ; x != dstWidth ; )
								{

#							if ENABLE_ROUNDUP

									// get colors and alpha
									DWORD a = (*i >> 16) & 0xF000;
									DWORD s = ScaleDWORDColorSaturation( *i++, saturation );
									DWORD b = s, g = b >> 8, r = g >> 8;

									// round up to avoid loss of lowest bit of color info
									if ( (BYTE)r >= 0xE8 )  r = 0xFF;
									if ( (BYTE)g >= 0xE8 )  g = 0xFF;
									if ( (BYTE)b >= 0xE8 )  b = 0xFF;

									// convert to 4444
									r = (BYTE)r >> 4;
									g = (BYTE)g >> 4;
									b = (BYTE)b >> 4;

									// write
									for ( int xr = 0; xr != wdup; ++xr, ++x )
									{
										*o++ = (WORD)(a | (r << 8) | (g << 4) | b);
									}

#							else // ENABLE_ROUNDUP

									for ( int xr = 0; xr != wdup; ++xr, ++x )
									{
										*o++ = MAKE4444FROM8888( s );
									}

#							endif // ENABLE_ROUNDUP

								}

								for ( int yr = 0; yr != hdup; ++yr, ++y )
								{
									memcpy( o, o - dstWidth, sizeof( WORD ) * dstWidth );
									o += dstWidth;
								}
							}
						}
						break;
					}

					case PIXEL_ARGB_1555:
					{
						WORD* o = (WORD*)dst;

#						if ENABLE_DITHERING
						if ( allowDither )
						{
							for ( int y = startY ; y != (startY + dstHeight) ; ++y )
							{
								for ( int x = 0 ; x != dstWidth ; )
								{
									// get colors and alpha
									DWORD a = (*i >> 16) & 0x8000;
									DWORD b = ScaleDWORDColorSaturation( *i++, saturation ), g = b >> 8, r = g >> 8;

									// 4 bits of error precision
									DWORD er = (r << 1) & 0x0F;
									DWORD eg = (g << 1) & 0x0F;
									DWORD eb = (b << 1) & 0x0F;

									// convert to 1555
									r = (BYTE)r >> 3;
									g = (BYTE)g >> 3;
									b = (BYTE)b >> 3;

									// dither if not pure color
									if ( r < 0x1F )  r += (er > s_Dither[y & 3][x & 3]);
									if ( g < 0x1F )  g += (eg > s_Dither[y & 3][x & 3]);		// $$$ james said that green must be toned down on a dither - add this
									if ( b < 0x1F )  b += (eb > s_Dither[y & 3][x & 3]);

									// write
									for ( int xr = 0; xr != wdup; ++xr, ++x )
									{
										*o++ = (WORD)(a | (r << 10) | (g << 5) | b);
									}
								}

								for ( int yr = 0; yr != hdup; ++yr, ++y )
								{
									memcpy( o, o - dstWidth, sizeof( WORD ) * dstWidth );
									o += dstWidth;
								}
							}
						}
						else
#						endif // ENABLE_DITHERING
						{
							for ( int y = 0 ; y != dstHeight ; ++y )
							{
								for ( int x = 0 ; x != dstWidth ; )
								{

#							if ENABLE_ROUNDUP

									// get colors and alpha
									DWORD a = (*i >> 16) & 0x8000;
									DWORD s = ScaleDWORDColorSaturation( *i++, saturation );
									DWORD b = s, g = b >> 8, r = g >> 8;

									// round up to avoid loss of lowest bit of color info
									if ( (BYTE)r >= 0xF4 )  r = 0xFF;
									if ( (BYTE)g >= 0xF4 )  g = 0xFF;
									if ( (BYTE)b >= 0xF4 )  b = 0xFF;

									// convert to 1555
									r = (BYTE)r >> 3;
									g = (BYTE)g >> 3;
									b = (BYTE)b >> 3;

									// write
									for ( int xr = 0; xr != wdup; ++xr, ++x )
									{
										*o++ = (WORD)(a | (r << 10) | (g << 5) | b);
									}

#							else // ENABLE_ROUNDUP

									// write
									for ( int xr = 0; xr != wdup; ++xr, ++x )
									{
										*o++ = MAKE1555FROM8888( s );
									}

#							endif // ENABLE_ROUNDUP

								}

								for ( int yr = 0; yr != hdup; ++yr, ++y )
								{
									memcpy( o, o - dstWidth, sizeof( WORD ) * dstWidth );
									o += dstWidth;
								}
							}
						}
						break;
					}

					case PIXEL_RGB_565:
					{
						WORD* o = (WORD*)dst;

#						if ENABLE_DITHERING
						if ( allowDither )
						{
							for ( int y = startY ; y != (startY + dstHeight) ; ++y )
							{
								for ( int x = 0 ; x != dstWidth ; )
								{
									// get colors and alpha
									DWORD b = ScaleDWORDColorSaturation( *i++, saturation ), g = b >> 8, r = g >> 8;

									// 4 bits of error precision
									DWORD er = (r << 1) & 0x0F;
									DWORD eg = (g << 1) & 0x0F;
									DWORD eb = (b << 1) & 0x0F;

									// convert to 565
									r = (BYTE)r >> 3;
									g = (BYTE)g >> 2;
									b = (BYTE)b >> 3;

									// dither if not pure color
									if ( r < 0x1F )  r += (er > s_Dither[y & 3][x & 3]);
									if ( g < 0x3F )  g += (eg > s_Dither[y & 3][x & 3]);		// $$$ james said that green must be toned down on a dither - add this
									if ( b < 0x1F )  b += (eb > s_Dither[y & 3][x & 3]);

									// write
									for ( int xr = 0; xr != wdup; ++xr, ++x )
									{
										*o++ = (WORD)((r << 11) | (g << 5) | b);
									}
								}

								for ( int yr = 0; yr != hdup; ++yr, ++y )
								{
									memcpy( o, o - dstWidth, sizeof( WORD ) * dstWidth );
									o += dstWidth;
								}
							}
						}
						else
#						endif // ENABLE_DITHERING
						{
							for ( int y = 0 ; y != dstHeight ; ++y )
							{
								for ( int x = 0 ; x != dstWidth ; )
								{

#							if ENABLE_ROUNDUP

									// get colors and alpha
									DWORD s = ScaleDWORDColorSaturation( *i++, saturation );
									DWORD b = s, g = b >> 8, r = g >> 8;

									// round up to avoid loss of lowest bit of color info
									if ( (BYTE)r >= 0xF4 )  r = 0xFF;
									if ( (BYTE)g >= 0xF4 )  g = 0xFF;
									if ( (BYTE)b >= 0xF4 )  b = 0xFF;

									// convert to 565
									r = (BYTE)r >> 3;
									g = (BYTE)g >> 2;
									b = (BYTE)b >> 3;

									// write
									for ( int xr = 0; xr != wdup; ++xr, ++x )
									{
										*o++ = (WORD)((r << 11) | (g << 5) | b);
									}

#							else // ENABLE_ROUNDUP

									for ( int xr = 0; xr != wdup; ++xr, ++x )
									{
										*o++ = MAKE565FROM8888( s );
									}

#							endif // ENABLE_ROUNDUP

								}

								for ( int yr = 0; yr != hdup; ++yr, ++y )
								{
									memcpy( o, o - dstWidth, sizeof( WORD ) * dstWidth );
									o += dstWidth;
								}
							}
						}
						break;
					}

					case PIXEL_ARGB_8888:
					{
						DWORD* o = (DWORD*)dst;
						for ( int y = 0 ; y != dstHeight ; ++y )
						{
							for ( int x = 0 ; x != dstWidth ; ++i )
							{
								DWORD s = ScaleDWORDColorSaturation( *i, saturation );
								for ( int xr = 0; xr != wdup; ++xr, ++x )
								{
									*o++ = s;
								}
							}

							for ( int yr = 0; yr != hdup; ++yr, ++y )
							{
								memcpy( o, o - dstWidth, sizeof( DWORD ) * dstWidth );
								o += dstWidth;
							}
						}
						break;
					}

					case PIXEL_RGB_888:
					{
						BYTE* o = (BYTE*)dst;
						for ( int y = 0 ; y != dstHeight ; ++y )
						{
							for ( int x = 0 ; x != dstWidth ; ++i )
							{
								DWORD s = ScaleDWORDColorSaturation( *i, saturation );
								for ( int xr = 0; xr != wdup; ++xr, ++x )
								{
									*o++ = (BYTE)s;
									*o++ = (BYTE)(s >> 8);
									*o++ = (BYTE)(s >> 16);
								}
							}

							for ( int yr = 0; yr != hdup; ++yr, ++y )
							{
								memcpy( o, o - (dstWidth*3), 3 * dstWidth );
								o += (dstWidth*3);
							}
						}
						break;
					}

					default:
					{
						gpassertm( 0, "Unsupported pixel conversion!" );
					}
				}
				break;
			}

			case PIXEL_ARGB_1555:
			{
				gpassert( srcWidth == dstWidth && srcHeight == dstHeight );
				const WORD* i = (const WORD*)src;
				const WORD* iend = i + (srcWidth * srcHeight);

				switch( dstFormat )
				{
					case PIXEL_ARGB_8888:
					{
						DWORD* o = (DWORD*)dst;

						for ( ; i != iend ; ++i, ++o )
						{
							*o = MAKE8888FROM1555( *i );
						}
						break;
					}

					case PIXEL_ARGB_4444:
					{
						WORD* o = (WORD*)dst;

						for ( ; i != iend ; ++i, ++o )
						{
							*o = MAKE4444FROM1555( *i );
						}
						break;
					}

					case PIXEL_RGB_565:
					{
						WORD* o = (WORD*)dst;

						for ( ; i != iend ; ++i, ++o )
						{
							*o = MAKE565FROM1555( *i );
						}
						break;
					}

					default:
					{
						gpassertm( 0, "Unsupported pixel conversion!" );
					}
				}
				break;
			}

			case PIXEL_ARGB_4444:
			{
				gpassert( srcWidth == dstWidth && srcHeight == dstHeight );
				const WORD* i = (const WORD*)src;
				const WORD* iend = i + (srcWidth * srcHeight);

				switch( dstFormat )
				{
					case PIXEL_ARGB_8888:
					{
						DWORD* o = (DWORD*)dst;

						for ( ; i != iend ; ++i, ++o )
						{
							*o = MAKE8888FROM4444( *i );
						}
						break;
					}

					case PIXEL_ARGB_1555:
					{
						WORD* o = (WORD*)dst;

						for ( ; i != iend ; ++i, ++o )
						{
							*o = MAKE1555FROM4444( *i );
						}
						break;
					}

					case PIXEL_RGB_565:
					{
						WORD* o = (WORD*)dst;

						for ( ; i != iend ; ++i, ++o )
						{
							*o = MAKE565FROM4444( *i );
						}
						break;
					}

					default:
					{
						gpassertm( 0, "Unsupported pixel conversion!" );
					}
				}
				break;
			}

			case PIXEL_RGB_565:
			{
				gpassert( srcWidth == dstWidth && srcHeight == dstHeight );
				const WORD* i = (const WORD*)src;
				const WORD* iend = i + (srcWidth * srcHeight);

				switch( dstFormat )
				{
					case PIXEL_ARGB_8888:
					{
						DWORD* o = (DWORD*)dst;

						for ( ; i != iend ; ++i, ++o )
						{
							*o = MAKE8888FROM565( *i );
						}
						break;
					}

					case PIXEL_ARGB_4444:
					{
						WORD* o = (WORD*)dst;

						for ( ; i != iend ; ++i, ++o )
						{
							*o = MAKE4444FROM565( *i );
						}
						break;
					}

					case PIXEL_ARGB_1555:
					{
						WORD* o = (WORD*)dst;

						for ( ; i != iend ; ++i, ++o )
						{
							*o = MAKE1555FROM565( *i );
						}
						break;
					}

					default:
					{
						gpassertm( 0, "Unsupported pixel conversion!" );
					}
				}
				break;
			}

			default:
			{
				gpassertm( 0, "Unsupported pixel conversion!" );
			}
		}
	}
}

void* CopyPixels( void* dst, ePixelFormat dstFormat, int dstWidth, int dstHeight, int dstStride,
				  const void* src, ePixelFormat srcFormat, int srcWidth, int srcHeight, int srcStride,
				  bool flip, bool allowDither, float saturation )
{
	gpassert( (dstWidth > 0) && (dstHeight > 0) );
	gpassert( (srcWidth > 0) && (srcHeight > 0) );
	gpassert( (dstStride == 0) || (dstStride >= dstWidth) );
	gpassert( (srcStride == 0) || (srcStride >= srcWidth) );
	gpassert( IsPointerValid( dst ) && IsPointerValid( src ) );

	// autodetect strides - note that dx requires dword alignment regardless of format
	int dstSizeBytes = GetSizeBytes( dstFormat );
	if ( dstStride == 0 )
	{
		dstStride = GetDwordAlignUp( dstWidth * dstSizeBytes );
	}
	int srcSizeBytes = GetSizeBytes( srcFormat );
	if ( srcStride == 0 )
	{
		srcStride = GetDwordAlignUp( srcWidth * srcSizeBytes );
	}

	// special: if strides are the same on both ends, blast copy
	if ( ((dstWidth * dstSizeBytes) == dstStride) && ((srcWidth * srcSizeBytes) == srcStride) && !flip )
	{
		CopyPixels( dst, dstFormat, src, srcFormat, srcWidth, srcHeight, dstWidth, dstHeight, 0, allowDither, saturation );
	}
	else
	{
		// $ must loop over lines

		if ( flip )
		{
			BYTE* idst			= (BYTE*)dst + (dstStride * (dstHeight - 1));	// out iter
			const BYTE* isrc	= (const BYTE*)src;								// in iter
			int hdup			= (dstHeight / srcHeight) - 1;

			for ( int y = 0 ; y != dstHeight ; ++y )
			{
				CopyPixels( idst, dstFormat, isrc, srcFormat, srcWidth, 1, dstWidth, 1, y, allowDither, saturation );

				for ( int yr = 0; yr != hdup; ++yr, ++y )
				{
					idst -= dstStride;
					memcpy( idst, idst + dstStride, dstSizeBytes * dstWidth );
				}

				idst -= dstStride;
				isrc += srcStride;
			}
		}
		else
		{
			BYTE* idst			= (BYTE*)dst;						// out iter
			const BYTE* isrc	= (const BYTE*)src;					// in iter
			int hdup			= (dstHeight / srcHeight) - 1;

			for ( int y = 0 ; y != dstHeight ; ++y )
			{
				CopyPixels( idst, dstFormat, isrc, srcFormat, srcWidth, 1, dstWidth, 1, y, allowDither, saturation );

				for ( int yr = 0; yr != hdup; ++yr, ++y )
				{
					idst += dstStride;
					memcpy( idst, idst - dstStride, dstSizeBytes * dstWidth );
				}

				idst += dstStride;
				isrc += srcStride;
			}
		}
	}

	// done
	return ( (BYTE*)dst + (dstStride * dstHeight) );
}

// $ writers are at the bottom of the file (so can pick up the readers)

//////////////////////////////////////////////////////////////////////////////
// class RapiMemImage implementation

RapiMemImage* RapiMemImage :: CreateMemImage( const char* fileName, bool wantsMips, bool screenShot )
{
	RapiMemImage* image = NULL;

	std::auto_ptr <RapiImageReader> reader( RapiImageReader::CreateReader( fileName, wantsMips, screenShot ) );
	if ( reader.get() != NULL )
	{
		image = new RapiMemImage( reader->GetWidth(), reader->GetHeight() );
		reader->GetNextSurface( image->GetBits(), PIXEL_ARGB_8888, 0, false, true, 1.0f );
	}

	return ( image );
}

bool RapiMemImage :: Init( const char* fileName )
{
	std::auto_ptr <RapiImageReader> reader( RapiImageReader::CreateReader( fileName, false, true ) );
	if ( reader.get() )
	{
		reader->GetNextSurface( *this, false );
	}

	return ( reader.get() != NULL );
}

void RapiMemImage :: CopyToClipboard( bool flip ) const
{
	// alloc buffer
	int stride = m_Width * 3;
	HGLOBAL hglobal = ::GlobalAlloc( GMEM_MOVEABLE, sizeof( BITMAPINFOHEADER ) + (stride * m_Height) );
	gpassert( hglobal != NULL );
	BITMAPINFOHEADER* header = (BITMAPINFOHEADER*)::GlobalLock( hglobal );
	gpassert( header != NULL );

	// setup header
	::ZeroObject( *header );
	header->biSize        = sizeof( *header );
	header->biWidth       = m_Width;
	header->biHeight      = m_Height;
	header->biPlanes      = 1;
	header->biCompression = BI_RGB;
	header->biBitCount    = 24;

	// copy bits, be sure to flip the flip, because it's a bmp, which is flipped, jeez...
	::CopyPixels( header + 1, PIXEL_RGB_888, m_Width, m_Height, stride,
				  m_Bitmap.begin(), PIXEL_ARGB_8888, m_Width, m_Height, 0,
				  !flip, true );

	// move it to the clipboard
	::GlobalUnlock( hglobal );
	::OpenClipboard( NULL );
	::EmptyClipboard();
	::SetClipboardData( CF_DIB, hglobal );
	::CloseClipboard();
}

DWORD RapiMemImage :: CalcCrc32( bool includeAlpha ) const
{
	UINT32 crc = 0;

	if ( includeAlpha )
	{
		crc = ::GetCRC32( &*m_Bitmap.begin(), m_Bitmap.size() * sizeof( DWORD ) );
	}
	else
	{
		Bitmap::const_iterator i, ibegin = m_Bitmap.begin(), iend = m_Bitmap.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			::AddCRC32( crc, (const BYTE*)&*i, 3 );
		}
	}

	return ( crc );
}

void RapiMemImage :: FlipVertical( void )
{
	DWORD* ib = GetBits(), * jb = GetBits( m_Height - 1 );
	for ( ; ib < jb ; ib += m_Width, jb -= m_Width )
	{
		DWORD* ie = ib + m_Width;
		DWORD* je = jb + m_Width;

		while ( ib != ie )
		{
			std::swap( *(--ie), *(--je) );
		}
	}
}

void RapiMemImage :: FlipHorizontal( void )
{
	Bitmap::iterator i, ibegin = m_Bitmap.begin(), iend = m_Bitmap.end();
	for ( i = ibegin ; i != iend ; i += m_Width )
	{
		std::reverse( i, i + m_Width );
	}
}

void RapiMemImage :: RotateClockwise( void )
{
	gpassert( m_Width == m_Height );

	// $ this would be lots faster if i did it in-place

	Bitmap oldBitmap( m_Bitmap );

	const DWORD* ib = oldBitmap.begin(), * ie = oldBitmap.end();
	int offset = m_Width - 1;

	for ( ; ib != ie ; --offset )
	{
		const DWORD* il = ib + m_Width;
		DWORD* out = m_Bitmap.begin() + offset;

		for ( ; ib != il ; ++ib, out += m_Width )
		{
			*out = *ib;
		}
	}
}

void RapiMemImage :: RotateCounterClockwise( void )
{
	gpassert( m_Width == m_Height );

	// $ this would be lots faster if i did it in-place

	Bitmap oldBitmap( m_Bitmap );

	DWORD* ib = m_Bitmap.begin(), * ie = m_Bitmap.end();
	int offset = m_Width - 1;

	for ( ; ib != ie ; --offset )
	{
		DWORD* il = ib + m_Width;
		const DWORD* in = m_Bitmap.begin() + offset;

		for ( ; ib != il ; ++ib, in += m_Width )
		{
			*ib = *in;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// class RapiRawReader declaration

/*
	ideas:  $$

	palettizing - look for unique colors and do up-to-256 index palette
	compression. also save 16-bit palette in addition to 32-bit palette, but
	the same compressed bits.
*/

class RapiRawReader : public RapiImageReader
{
public:
	SET_INHERITED( RapiRawReader, RapiImageReader );

// Header type.

	enum  {  HEADER_MAGIC = 'Rapi' /*FOURCC*/  };

	enum /*FOURCC*/ eFormat
	{
		FORMAT_UNKNOWN   = 0,
		FORMAT_ARGB_8888 = '8888',

		// future: possible dxt, etc...
	};

	enum eFlags
	{
		FLAG_NONE,
	};

#	pragma pack ( push, 1 )

	// $$$ tank builder: set a bit saying if it generated mipmaps or not, and which filter it used (in eFlags)

	struct RawHeader
	{
		FOURCC m_HeaderMagic;	// special magic number
		FOURCC m_Format;		// format of bits
		WORD   m_Flags;			// any special flags (for future expansion)
		WORD   m_SurfaceCount;	// total surfaces stored (for mip maps), always >= 1
		WORD   m_Width;			// width of surface 0
		WORD   m_Height;		// height of surface 0
	//  BYTE   m_Bits[];		// raw image data (format-dependent)

		RawHeader( void )
		{
			::ZeroObject( *this );
		}
	};

#	pragma pack ( pop )

// Setup.

	static RapiImageReader* CreateReader( const char* name, const_mem_ptr mem, bool wantsMips )
	{
		RapiRawReader* reader = NULL;

		if ( mem.size >= sizeof( RawHeader ) )
		{
			const RawHeader* rawHeader = (const RawHeader*)mem.mem;
			if ( rawHeader->m_HeaderMagic == HEADER_MAGIC )
			{
				reader = new RapiRawReader( mem );
				if ( !reader->Init( name, wantsMips ) )
				{
					Delete ( reader );
				}
			}
		}

		return ( reader );
	}

	RapiRawReader( const_mem_ptr mem )
		: m_Mem( mem )
	{
		m_Header  = (const RawHeader*)m_Mem.mem;
		m_Width   = m_Header->m_Width;
		m_Height  = m_Header->m_Height;
		m_Surface = 0;

		m_Mem.advance( sizeof( RawHeader ) );
		m_Iter = (const DWORD*)m_Mem.mem;
	}

	virtual ~RapiRawReader( void )
	{
		// this space intentionally left blank...
	}

	bool Init( const char* name, bool wantsMips )
	{
		// check vitals
		if (   (m_Header->m_Format != FORMAT_ARGB_8888)		// this is all we support at the moment
			|| (m_Header->m_Flags  != FLAG_NONE) )			// ditto
		{
			gperrorf(( "Unsupported RAW format, file = '%s'\n", name ));
			return ( false );
		}

		// warn about mips
		bool ok = true;
		if ( wantsMips )
		{
			if ( m_Header->m_SurfaceCount <= 1 )
			{
				gperrorf(( "Texture wants mipmaps but RAW file not built with any, file = '%s'\n", name ));
				ok = false;
			}

			if ( !::IsPower2( m_Width ) || !::IsPower2( m_Height ) )
			{
				gperrorf(( "Bitmap wants to be a texture but it is not a power of 2, file = '%s'\n", name ));
				ok = false;
			}
		}

		// done
		return ( ok );
	}

// Query.

	virtual int GetWidth( void ) const
	{
		return ( m_Width );
	}

	virtual int GetHeight( void ) const
	{
		return ( m_Height );
	}

	virtual bool GetNextSurface( void* out, ePixelFormat format, int stride, bool flip, bool allowDither, float saturation, int dstWidth = -1, int dstHeight = -1 )
	{
		// copy
		if ( out != NULL )
		{
			CopyPixels( out, format, dstWidth > 0 ? dstWidth : m_Width, dstHeight > 0 ? dstHeight : m_Height, stride,
						m_Iter, PIXEL_ARGB_8888, m_Width, m_Height, m_Width * 4,
						flip, allowDither, saturation );
		}

		// advance surface iter
		m_Iter += m_Width * m_Height;

		// adjust for next
		m_Width /= 2;
		m_Height /= 2;
		++m_Surface;

		// more?
		return ( m_Surface < m_Header->m_SurfaceCount );
	}

private:
	const RawHeader* m_Header;
	int              m_Width;
	int              m_Height;
	const_mem_iter   m_Mem;
	const DWORD*     m_Iter;
	int              m_Surface;

	SET_NO_COPYING( RapiRawReader );
};

//////////////////////////////////////////////////////////////////////////////
// class RapiDecodingReader declaration

// $ this special base class is meant to wrap up stuff like pixel format
//   conversion and mip creation. only use this when working with non-native
//   formats like psd, where we have to do lots of work anyway to decompress
//   and swizzle bits. here we create a memory buffer to hold the "canonical"
//   (argb_888) bits for later processing.
//
//   be sure to store images in this bitmap upside down! this reader is
//   expecting mips stored like so (after flipping):
//
//      +--------+.........
//      |        |        .
//      |   1    +---+    .
//      |        | 2 +-+  .
//      |        |   |3+-+.
//      +--------+---+-+-++
//      |                 |
//      |                 |
//      |                 |
//      |                 |
//      |        0        |
//      |                 |
//      |                 |
//      |                 |
//      +-----------------+
//
//   if there are no hand-made mipmaps then it will construct them on top of
//   the 0 surface. each successive request for a new surface will box filter
//   the surface down over itself.

class RapiDecodingReader : public RapiImageReader
{
public:
	SET_INHERITED( RapiDecodingReader, RapiImageReader );

	RapiDecodingReader( void )
	{
		m_Width       = 0;
		m_Height      = 0;
		m_Stride      = 0;
		m_Surface     = 0;
		m_Source      = NULL;
		m_Iter        = NULL;
		m_BuildMips   = false;
		m_ConstSource = false;
	}

	virtual ~RapiDecodingReader( void )
	{
		// this space intentionally left blank...
	}

	virtual bool GetNextSurface( void* out, ePixelFormat format, int stride, bool flip, bool allowDither, float saturation, int dstWidth = -1, int dstHeight = -1 )
	{
		// build next mip level if needed
		if ( m_BuildMips && (m_Surface != 0) )
		{
			// if const source, then surface 0 -> surface 1 special filter
			if ( m_ConstSource && (m_Surface == 1) )
			{
				// alloc space for surface 1
				m_Bitmap.resize( m_Width * m_Height );
				m_Source = m_Bitmap.begin();

				// filter surf 0 from const -> surf 1 local
				BoxFilter( m_Source, m_Width, m_Height, m_Width * 4,
						   m_Iter, m_Width * 2, m_Height * 2, m_Stride );

				// update vars
				m_Stride = m_Width * 4;
				m_Iter = m_Source;
			}
			else
			{
				// filtered downsample
				BoxFilter( m_Iter, m_Width    , m_Height    , m_Stride,
						   m_Iter, m_Width * 2, m_Height * 2, m_Stride );
			}
		}

		// copy out surface
		if ( out != NULL )
		{
			CopyPixels( out, format, dstWidth > 0 ? dstWidth : m_Width, dstHeight > 0 ? dstHeight : m_Height, stride,
						m_Iter, PIXEL_ARGB_8888, m_Width, m_Height, m_Stride,
						flip, allowDither, saturation );
		}

		// advance surface iter
		if ( !m_BuildMips )
		{
			if ( m_Surface == 0 )
			{
				m_Iter = m_Source;
			}
			else
			{
				m_Iter += ((m_Stride / 4) * (m_Height / 2)) + m_Width;
			}
		}

		// adjust for next
		m_Width /= 2;
		m_Height /= 2;
		++m_Surface;

		// done?
		return ( m_Width && m_Height );
	}

// API.

	virtual int GetWidth( void ) const
	{
		return ( m_Width );
	}

	virtual int GetHeight( void ) const
	{
		return ( m_Height );
	}

	bool Init( const char* name, bool wantsMips, bool screenShot, bool noMipsOk, bool hasAlpha, int width, int height, const DWORD* bits = NULL )
	{
		gpassert( name != NULL );
		gpassert( width > 0 );
		gpassert( height > 0 );

		bool ok = true;
		bool offset = false;

		m_Width = width;
		m_Height = height;
		m_Stride = width * 4;

		int count = m_Width * m_Height;
		DWORD fill = hasAlpha ? 0 : 0xFF000000;
		bool canMip = (m_Width > 1) && (m_Height > 1);

		// if we want mips, auto-detect if we have hand-made versions already
		if ( canMip && !screenShot )
		{
			ok = false;

			if ( ::IsPower2( width ) )
			{
				if ( ::IsPower2( height ) )
				{
					ok = true;

					if( wantsMips )
					{
						m_BuildMips = true;

						if ( !noMipsOk )
						{
							gpperff(( "PERFORMANCE WARNING: texture wants mipmaps, but does not have them built in, file = '%s'.\n", name ));
						}
					}
				}
				else
				{
					height -= height / 3;
					if ( ::IsPower2( height ) )
					{
						ok = true;
						offset = true;
						m_Height = height;
					}
				}
			}
		}

		if ( ok )
		{
			// set our iter
			if ( bits != NULL )
			{
				m_ConstSource = true;

				// this cast is ok - we won't be modifying the bits
				m_Source = (DWORD*)bits;
			}
			else
			{
				// alloc translation buffer
				m_Bitmap.resize( count, fill );
				m_Source = m_Bitmap.begin();
			}
			m_Iter = m_Source;

			// offset to surface 0 if there are extra mips
			if ( offset )
			{
				m_Iter += m_Width * (m_Height / 2);
			}
		}
		else
		{
			// only can fail if no power 2
			gperrorf(( "Bitmap wants to be a texture but it is not a power of 2, file = '%s'\n", name ));
		}

		// done
		return ( ok );
	}

	typedef std::vector <DWORD> Bitmap;

	Bitmap m_Bitmap;				// derived must fill this with data arranged same as raw (linearly)

private:
	int    m_Width;					// width of current surface in pixels
	int    m_Height;				// height of current surface in pixels
	int    m_Stride;				// stride across lines in bytes
	int    m_Surface;				// index of current surface
	DWORD* m_Source;				// pointer to start of surface
	DWORD* m_Iter;					// where we are in surface for copying out
	bool   m_BuildMips;				// must build mips as we go
	bool   m_ConstSource;			// source is read-only

	SET_NO_COPYING( RapiDecodingReader );
};

//////////////////////////////////////////////////////////////////////////////
// class RapiPsdReader declaration

class RapiPsdReader : public RapiDecodingReader
{
public:
	SET_INHERITED( RapiPsdReader, RapiDecodingReader );

// Header type.

	enum  {  HEADER_MAGIC = '8BPS' /*FOURCC*/  };

#	pragma pack ( push, 1 )

	// note: PSD's are all byte-swapped

	struct RawHeader
	{
		FOURCC m_Signature;			// must be '8BPS'
		WORD   m_Version;			// must be 1
		BYTE   m_Reserved[ 6 ];		// ignore (but zero-fill)
		WORD   m_Channels;			// number of channels in the image, including alphas (from 1-24)
		DWORD  m_Height;			// height of image in pixels (from 1-30000)
		DWORD  m_Width;				// width of image in pixels (from 1-30000)
		WORD   m_BitsPerChannel;	// number of bits per channel (1, 8, or 16)
		WORD   m_Mode;				// color mode of file (from 0-9)

		RawHeader( void )
		{
			::ZeroObject( *this );
		}

		void ByteSwap( void )
		{
			::ByteSwap( m_Signature      );
			::ByteSwap( m_Version        );
			::ByteSwap( m_Channels       );
			::ByteSwap( m_Height         );
			::ByteSwap( m_Width          );
			::ByteSwap( m_BitsPerChannel );
			::ByteSwap( m_Mode           );
		}
	};

#	pragma pack ( pop )

// Setup.

	static RapiImageReader* CreateReader( const char* name, const_mem_ptr mem, bool wantsMips, bool screenShot )
	{
		RapiPsdReader* reader = NULL;

		if ( mem.size >= sizeof( RawHeader ) + (sizeof( DWORD ) * 4) )	// $ include section sizes
		{
			const RawHeader* psdHeader = (const RawHeader*)mem.mem;
			if ( GetByteSwap( psdHeader->m_Signature ) == HEADER_MAGIC )
			{
				reader = new RapiPsdReader( mem );
				if ( !reader->Init( name, wantsMips, screenShot ) )
				{
					Delete ( reader );
				}
			}
		}

		return ( reader );
	}

	RapiPsdReader( const_mem_ptr mem )
		: m_Iter( mem )
	{
		m_Header = *(const RawHeader*)m_Iter.mem;
		m_Header.ByteSwap();

		m_Iter.advance( sizeof( RawHeader ) );
	}

	virtual ~RapiPsdReader( void )
	{
		// this space intentionally left blank...
	}

	bool Init( const char* name, bool wantsMips, bool screenShot )
	{

	// Early bailouts.

		// check vitals
		if (   (m_Header.m_Version        != 1)		// must be 1
			|| (m_Header.m_BitsPerChannel != 8)		// must be 8 bpp
			|| (m_Header.m_Mode           != 3) )	// must be RGB mode $$ future support indexed?
		{
			gperrorf(( "Unsupported PSD format, file = '%s'\n", name ));
			return ( false );
		}

		// tell base we're ready
		if ( !Inherited::Init( name, wantsMips, screenShot, false, m_Header.m_Channels == 4, m_Header.m_Width, m_Header.m_Height ) )
		{
			return ( false );
		}

	// Preprocess.

		// skip color, image resource, and layer/mask data
		m_Iter.advance( GetByteSwap( m_Iter.advance_dword() ) );
		m_Iter.advance( GetByteSwap( m_Iter.advance_dword() ) );
		m_Iter.advance( GetByteSwap( m_Iter.advance_dword() ) );

		// get compression info (0 = raw, 1 = RLE)
		WORD compression = GetByteSwap( m_Iter.advance_word() );

		// skip byte count words if any
		if ( compression == 1 )
		{
			m_Iter.advance( m_Header.m_Height * m_Header.m_Channels * 2 );
		}

	// Process bits.

		// for each channel, up to the 4 we support
		for ( int channel = 0 ; (channel < m_Header.m_Channels) && (channel < 4) ; ++channel )
		{
			// order: RRR GGG BBB AAA
			int shift = "\x10\x08\x00\x18"[ channel ];

			// load in RLE compressed image data
			if ( compression == 1 )
			{
				// this section for compressed PSD's

				for ( DWORD* end = m_Bitmap.end() ; end != m_Bitmap.begin() ; end -= m_Header.m_Width )
				{
					for ( DWORD* out = end - m_Header.m_Width ; out < end ; )
					{
						BYTE length = m_Iter.advance_byte();
						if ( length & 0x80 )
						{
							// run of repeated bytes
							DWORD value = m_Iter.advance_byte() << shift;
							for ( DWORD* runEnd = out + BYTE(-length) + 1 ; out < runEnd ; )
							{
								*out++ |= value;
							}
						}
						else
						{
							// run of unique bytes
							for ( DWORD* runEnd = out + length + 1 ; out != runEnd ; )
							{
								*out++ |= m_Iter.advance_byte() << shift;
							}
						}
					}
				}
			}
			else
			{
				// this section for non-compressed PSD's

				for ( DWORD* end = m_Bitmap.end() ; end != m_Bitmap.begin() ; end -= m_Header.m_Width )
				{
					for ( DWORD* out = end - m_Header.m_Width ; out < end ; )
					{
						*out++ |= m_Iter.advance_byte() << shift;
					}
				}
			}
		}

	// Finish.

		// done
		return ( true );
	}

private:
	RawHeader      m_Header;
	const_mem_iter m_Iter;

	SET_NO_COPYING( RapiPsdReader );
};

//////////////////////////////////////////////////////////////////////////////
// class RapiFlmReader declaration

// this stores images stacked on top of each other (vertical, linear in memory)

class RapiFlmReader : public RapiImageReader
{
public:
	SET_INHERITED( RapiFlmReader, RapiImageReader );

// Header type.

	enum  {  HEADER_MAGIC = 'Rand' /*FOURCC*/  };

#	pragma pack ( push, 1 )

	// note: FLM's are all byte-swapped
	struct RawHeader
	{
		FOURCC m_Signature;			// must be 'Rand'
		DWORD  m_NumFrames;			// number of frames in file
		WORD   m_Packing;			// packing method (should be 0)
		WORD   m_Reserved;			// reserved, should be 0
		WORD   m_Width;				// image width
		WORD   m_Height;			// image height
		WORD   m_Leading;			// horiz gap between frames
		WORD   m_FramesPerSec;		// frame rate
		BYTE   m_Spare[ 16 ];		// some spare data.

		RawHeader( void )
		{
			::ZeroObject( *this );
		}

		void ByteSwap( void )
		{
			::ByteSwap( m_Signature    );
			::ByteSwap( m_NumFrames    );
			::ByteSwap( m_Packing      );
			::ByteSwap( m_Reserved     );
			::ByteSwap( m_Width        );
			::ByteSwap( m_Height       );
			::ByteSwap( m_Leading      );
			::ByteSwap( m_FramesPerSec );
		}
	};

#	pragma pack ( pop )

// Setup.

	static RapiImageReader* CreateReader( const char* name, const_mem_ptr mem )
	{
		RapiFlmReader* reader = NULL;

		if ( mem.size >= sizeof( RawHeader ) )
		{
			const RawHeader* flmHeader = (const RawHeader*)((const BYTE*)mem.mem + mem.size - sizeof( RawHeader ));
			if ( GetByteSwap( flmHeader->m_Signature ) == HEADER_MAGIC )
			{
				reader = new RapiFlmReader( mem );
				if ( !reader->Init( name ) )
				{
					Delete ( reader );
				}
			}
		}

		return ( reader );
	}

	RapiFlmReader( const_mem_ptr mem )
		: m_Iter( mem )
	{
		m_Header = *(const RawHeader*)((const BYTE*)m_Iter.mem + m_Iter.size - sizeof( RawHeader ));
		m_Header.ByteSwap();
	}

	virtual ~RapiFlmReader( void )
	{
		// this space intentionally left blank...
	}

	virtual int GetWidth( void ) const
	{
		return ( m_Header.m_Width );
	}

	virtual int GetHeight( void ) const
	{
		return ( m_Header.m_Height );
	}

	virtual int GetFps( void ) const
	{
		return ( m_Header.m_FramesPerSec );
	}

	virtual bool GetNextSurface( void* out, ePixelFormat format, int stride, bool flip, bool allowDither, float saturation, int dstWidth = -1, int dstHeight = -1 )
	{
		// copy out surface
		if ( out != NULL )
		{
			CopyPixels( out, format, dstWidth > 0 ? dstWidth : m_Header.m_Width, dstHeight > 0 ? dstHeight : m_Header.m_Height, stride,
						m_BitmapIter, PIXEL_ARGB_8888, m_Header.m_Width, m_Header.m_Height, m_Header.m_Width * 4,
						flip, allowDither, saturation );
		}

		// adjust for next
		m_BitmapIter += m_Header.m_Width * m_Header.m_Height;

		// done?
		return ( m_BitmapIter < m_Bitmap.end() );
	}

	bool Init( const char* name )
	{

	// Early bailouts.

		// check vitals
		if ( m_Header.m_Packing != 0 )		// must be 0
		{
			gperrorf(( "Unsupported FLM format, file = '%s'\n", name ));
			return ( false );
		}

		// alloc memory
		m_Bitmap.resize( m_Header.m_Width * m_Header.m_Height * m_Header.m_NumFrames );
		m_BitmapIter = m_Bitmap.begin();

	// Process bits.

		DWORD* frameBegin = m_Bitmap.begin();
		DWORD* frameEnd   = frameBegin + (m_Header.m_Width * m_Header.m_Height);

		// for each frame
		for ( int frame = 0 ; frame < (int)m_Header.m_NumFrames ; ++frame )
		{
			// for each line
			for ( DWORD* end = frameEnd ; end != frameBegin ; end -= m_Header.m_Width )
			{
				for ( DWORD* out = end - m_Header.m_Width ; out < end ; )
				{
					// order: RRR GGG BBB AAA

					BYTE r = m_Iter.advance_byte();
					BYTE g = m_Iter.advance_byte();
					BYTE b = m_Iter.advance_byte();
					BYTE a = m_Iter.advance_byte();
					*out++ = (a << 24) | (r << 16) | (g << 8) | b;
				}
			}

			// skip leading bytes of next frame
			m_Iter.advance( m_Header.m_Leading * m_Header.m_Width * 4 );

			// advance to next frame
			frameBegin += m_Header.m_Width * m_Header.m_Height;
			frameEnd   += m_Header.m_Width * m_Header.m_Height;
		}

	// Finish.

		// done
		return ( true );
	}

private:
	typedef std::vector <DWORD> Bitmap;

	RawHeader      m_Header;
	const_mem_iter m_Iter;
	Bitmap         m_Bitmap;
	DWORD*         m_BitmapIter;

	SET_NO_COPYING( RapiFlmReader );
};

//////////////////////////////////////////////////////////////////////////////
// class RapiBmpReader declaration

class RapiBmpReader : public RapiDecodingReader
{
public:
	SET_INHERITED( RapiBmpReader, RapiDecodingReader );

// Header type.

	enum  {  HEADER_MAGIC = 'MB'  };

#	pragma pack ( push, 1 )

	struct RawHeader : public BITMAPFILEHEADER, public BITMAPINFOHEADER
	{
		RawHeader( void )
		{
			::ZeroObject( *this );
		}
	};

#	pragma pack ( pop )

// Setup.

	static RapiImageReader* CreateReader( const char* name, const_mem_ptr mem, bool wantsMips, bool screenShot )
	{
		RapiBmpReader* reader = NULL;

		if ( mem.size >= sizeof( RawHeader ) )
		{
			const RawHeader* bmpHeader = (const RawHeader*)mem.mem;
			if ( bmpHeader->bfType == HEADER_MAGIC )
			{
				reader = new RapiBmpReader( mem );
				if ( !reader->Init( name, wantsMips, screenShot ) )
				{
					Delete ( reader );
				}
			}
		}

		return ( reader );
	}

	RapiBmpReader( const_mem_ptr mem )
		: m_Iter( mem )
	{
		m_Header = (const RawHeader*)m_Iter.mem;
		m_Iter.advance( m_Header->bfOffBits );
	}

	virtual ~RapiBmpReader( void )
	{
		// this space intentionally left blank...
	}

	bool Init( const char* name, bool wantsMips, bool screenShot )
	{
	// Early bailouts.

		// check vitals
		if ( m_Header->biPlanes != 1 )											// must be 1
		{
			gperrorf(( "Unsupported BMP format, file = '%s'\n", name ));
			return ( false );
		}

		// check channels
		if ( (m_Header->biBitCount != 8) && (m_Header->biBitCount != 24) )		// 8 or 24 bpp only
		{
			gperrorf(( "Only 8 or 24 bit BMP formats supported, file = '%s'\n", name ));
			return ( false );
		}

		// tell base we're ready
		if ( !Inherited::Init( name, wantsMips, screenShot, false, false, m_Header->biWidth, labs( m_Header->biHeight ) ) )
		{
			return ( false );
		}

	// Process bits.

		// detect direction
		bool topDown = m_Header->biHeight < 0;

		// go!
		if ( m_Header->biBitCount == 8 )
		{
			const gpcolor* palette = (const gpcolor*)((const BYTE*)m_Header + sizeof( *m_Header ));

			if ( m_Header->biCompression == BI_RGB )
			{
				// unpack 8 bpp uncompressed

				if ( topDown )
				{
					for ( DWORD* end = m_Bitmap.end() ; end != m_Bitmap.begin() ; end -= m_Header->biWidth )
					{
						for ( DWORD* out = end - m_Header->biWidth ; out < end ; )
						{
							*out++ |= palette[ m_Iter.advance_byte() ];
						}
					}
				}
				else
				{
					for ( DWORD* out = m_Bitmap.begin(), * end = m_Bitmap.end() ; out < end ; )
					{
						*out++ |= palette[ m_Iter.advance_byte() ];
					}
				}
			}
			else
			{
				// unpack 8 bpp RLE-compressed

				gpassert( m_Header->biCompression == BI_RLE8 );
				gpassert( !topDown );

				for ( DWORD* out = m_Bitmap.begin(), * end = m_Bitmap.end() ; out < end ; )
				{
					BYTE code = m_Iter.advance_byte();
					if ( code == 0 )
					{
						// escape
						code = m_Iter.advance_byte();
						if ( code == 0 )
						{
							// end of line, skip the rest
							int offset = (out - m_Bitmap.begin()) % m_Header->biWidth;
							if ( offset > 0 )
							{
								out += m_Header->biWidth - offset;
							}
						}
						else if ( code == 1 )
						{
							// end of bitmap, abort
							break;
						}
						else if ( code == 2 )
						{
							// delta from current
							BYTE x = m_Iter.advance_byte();
							BYTE y = m_Iter.advance_byte();
							out += x + (y * m_Header->biWidth);
						}
						else
						{
							// run of unique bytes - must be word-aligned
							for ( DWORD* runEnd = out + code ; out < runEnd ; )
							{
								*out++ |= palette[ m_Iter.advance_byte() ];
							}

							// make sure we're word-aligned
							if ( !IsWordAligned( m_Iter.mem ) )
							{
								m_Iter.advance_byte();
							}
						}
					}
					else
					{
						// run of repeated bytes
						DWORD value = palette[ m_Iter.advance_byte() ];
						for ( DWORD* runEnd = out + code ; out < runEnd ; )
						{
							*out++ |= value;
						}
					}
				}
			}
		}
		else
		{
			// unpack 24 bpp uncompressed

			gpassert( m_Header->biBitCount == 24 );
			gpassert( m_Header->biCompression == BI_RGB );

			if ( topDown )
			{
				for ( DWORD* end = m_Bitmap.end() ; end != m_Bitmap.begin() ; end -= m_Header->biWidth )
				{
					for ( DWORD* out = end - m_Header->biWidth ; out < end ; )
					{
						BYTE b = m_Iter.advance_byte();
						BYTE g = m_Iter.advance_byte();
						BYTE r = m_Iter.advance_byte();
						*out++ |= (r << 16) | (g << 8) | b;
					}
				}
			}
			else
			{
				for ( DWORD* out = m_Bitmap.begin(), * end = m_Bitmap.end() ; out < end ; )
				{
					BYTE b = m_Iter.advance_byte();
					BYTE g = m_Iter.advance_byte();
					BYTE r = m_Iter.advance_byte();
					*out++ |= (r << 16) | (g << 8) | b;
				}
			}
		}

	// Finish.

		// done
		return ( true );
	}

private:
	const RawHeader* m_Header;
	const_mem_iter   m_Iter;

	SET_NO_COPYING( RapiBmpReader );
};

//////////////////////////////////////////////////////////////////////////////
// class RapiPngReader declaration

#	if 0

class RapiPngReader : public RapiDecodingReader
{
public:
	SET_INHERITED( RapiPngReader, RapiDecodingReader );

	static RapiImageReader* CreateReader( const char* name, const_mem_ptr mem, bool wantsMips )
	{
		RapiImageReader* reader = NULL;

		if ( (mem.size >= 8) && (png_sig_cmp( (const BYTE*)mem.mem, 0, 8 ) == 0) )
		{
			reader = new RapiPngReader( mem );
		}

		return ( reader );
	}

	RapiPngReader( const_mem_ptr mem );
	virtual ~RapiPngReader( void );

private:
	const_mem_ptr m_Mem;

	SET_NO_COPYING( RapiPngReader );
};

#	endif // 0

//////////////////////////////////////////////////////////////////////////////
// class RapiJpgReader declaration

#	if 0

class RapiJpgReader : public RapiDecodingReader
{
public:
	SET_INHERITED( RapiJpgReader, RapiDecodingReader );

	static RapiImageReader* CreateReader( const char* name, const_mem_ptr mem, bool wantsMips )
	{
		RapiImageReader* reader = NULL;

#		if 0
		/* We set up the normal JPEG error routines, then override error_exit. */
		cinfo.err = jpeg_std_error(&jerr.pub);
		jerr.pub.error_exit = my_error_exit;
		/* Establish the setjmp return context for my_error_exit to use. */
		if (setjmp(jerr.setjmp_buffer))
		{
			/* If we get here, the JPEG code has signaled an error.
			 * We need to clean up the JPEG object, close the input file, and return.
			 */
			jpeg_destroy_decompress(&cinfo);
			fclose(infile);
			return 0;
		}
		/* Now we can initialize the JPEG decompression object. */
		jpeg_create_decompress(&cinfo);

		/* Step 2: specify data source (eg, a file) */

		jpeg_stdio_src(&cinfo, infile);

		/* Step 3: read file parameters with jpeg_read_header() */

		if ( jpeg_read_header(&cinfo, TRUE) == JPEG_HEADER_OK )
		{
			reader = new RapiJpgReader( mem );
		}
#		endif // 0

		return ( reader );
	}

	RapiJpgReader( const_mem_ptr mem );
	virtual ~RapiJpgReader( void );

private:
	const_mem_ptr m_Mem;

	SET_NO_COPYING( RapiJpgReader );
};

#	endif // 0

//////////////////////////////////////////////////////////////////////////////
// class RapiImageReader implementation

RapiImageReader* RapiImageReader :: CreateReader( const char* fileName, bool wantsMips, bool screenShot )
{
	gpassert( fileName != NULL );

	FileSys::AutoFileHandle fileHandle;
	FileSys::AutoMemHandle  memHandle;

	RapiImageReader* reader = NULL;
	if ( fileHandle.Open( fileName ) && memHandle.Map( fileHandle ) )
	{
		reader = CreateReader( fileName, const_mem_ptr( memHandle.GetData(), memHandle.GetSize() ), wantsMips, screenShot );
		if ( reader != NULL )
		{
			reader->m_FileHandle = fileHandle.Release();
			reader->m_MemHandle  = memHandle.Release();
		}
	}

	return ( reader );
}

RapiImageReader* RapiImageReader :: CreateReader( const char* name, const_mem_ptr mem, bool wantsMips, bool screenShot )
{
	RapiImageReader* reader = NULL;

	// test for raw
	if ( (reader = RapiRawReader::CreateReader( name, mem, wantsMips )) != NULL )
	{
		return ( reader );
	}

	// test for psd
	if ( (reader = RapiPsdReader::CreateReader( name, mem, wantsMips, screenShot )) != NULL )
	{
		return ( reader );
	}

	// test for flm (gui only currently)
	if ( !wantsMips )
	{
		if ( (reader = RapiFlmReader::CreateReader( name, mem )) != NULL )
		{
			return ( reader );
		}
	}

	// test for bmp
	if ( (reader = RapiBmpReader::CreateReader( name, mem, wantsMips, screenShot )) != NULL )
	{
		return ( reader );
	}

#	if 0
	// test for png
	if ( (reader = RapiPngReader::CreateReader( name, mem, wantsMips )) != NULL )
	{
		return ( reader );
	}

	// test for jpg
	if ( (reader = RapiJpgReader::CreateReader( name, mem, wantsMips )) != NULL )
	{
		return ( reader );
	}
#	endif // 0

	// dunno what the hell it is
	::SetLastError( ERROR_BAD_FORMAT );

	// fail
	return ( NULL );
}

RapiImageReader* RapiImageReader :: CreateReader( const char* name, const RapiMemImage& image, bool wantsMips, bool screenShot )
{
	RapiDecodingReader* reader = new RapiDecodingReader;
	if ( !reader->Init( name, wantsMips, screenShot, true, true, image.GetWidth(), image.GetHeight(), image.GetBits() ) )
	{
		Delete ( reader );
	}

	return ( reader );
}

RapiImageReader* RapiImageReader :: CreateReader( const char* name, LPDIRECT3DSURFACE8 surface, bool wantsMips, bool screenShot, const RECT* rect )
{
	gpassert( surface != NULL );

	// $$ this should probably walk the attached surface list for later surfaces
	//    so we can extract the mip maps too for debugging purposes.

	RapiDecodingReader* reader = NULL;

	// Get the description of this surface
	D3DSURFACE_DESC surfaceDesc;
	TRYDX( surface->GetDesc( &surfaceDesc ) );

	// Lock the rectangle of bits we are interested in
	D3DLOCKED_RECT lockedRect;
	if( SUCCEEDED( surface->LockRect( &lockedRect, rect, D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY ) ) )
	{
		ePixelFormat format = GetPixelFormat( surfaceDesc.Format );
		if ( format != PIXEL_UNKNOWN )
		{
			int width  = rect ? (rect->right  - rect->left) : surfaceDesc.Width;
			int height = rect ? (rect->bottom - rect->top ) : surfaceDesc.Height;

			reader = new RapiDecodingReader;
			if ( reader->Init( name, wantsMips, screenShot, true, true, width, height ) )
			{
				CopyPixels( reader->m_Bitmap.begin(), PIXEL_ARGB_8888, width, height, 0,
							lockedRect.pBits, format, width, height, lockedRect.Pitch,
							false, true );
			}
			else
			{
				Delete ( reader );
			}
		}
		else
		{
			gpwarning( "Unsupported source pixel format!" );
		}

		// unlock the surface
		surface->UnlockRect();
	}

	return ( reader );
}

RapiImageReader* RapiImageReader :: CreateReader( Rapi* rapi, DWORD tex, bool wantsMips, const RECT* rect )
{
	gpassert( rapi != NULL );
	LPDIRECT3DTEXTURE8 texture = rapi->GetTexSurface( tex );
	if( texture )
	{
		LPDIRECT3DSURFACE8 surface = NULL;
		TRYDX( texture->GetSurfaceLevel( 0, &surface ) );
		surface->Release();
		return ( surface ? CreateReader( rapi->GetTexturePathname( tex ), surface, wantsMips, false, rect ) : NULL );
	}

	return NULL;
}

struct Alias
{
	const char* m_From;
	const char* m_To;
};

static const Alias s_Aliases[] =
{
	{  "img", "raw"  },
	{  "img", "psd"  },
	{  "img", "bmp"  },
//$$$	{  "img", "png"  },
//$$$	{  "img", "jpg"  },
};

void RapiImageReader :: RegisterFileTypeAliases( void )
{
	const Alias* i, * ibegin = s_Aliases, * iend = ARRAY_END( s_Aliases );
	for ( i = ibegin ; i != iend ; ++i )
	{
		FileSys::IFileMgr::AddFileTypeAlias( i->m_From, i->m_To );
	}
}

void RapiImageReader :: UnregisterFileTypeAliases( void )
{
	const Alias* i, * ibegin = s_Aliases, * iend = ARRAY_END( s_Aliases );
	for ( i = ibegin ; i != iend ; ++i )
	{
		FileSys::IFileMgr::RemoveFileTypeAlias( i->m_From, i->m_To );
	}
}

bool RapiImageReader :: GetNextSurface( RapiMemImage& image, bool flip )
{
	image.Init( GetWidth(), GetHeight() );
	return ( GetNextSurface( image.GetBits(), PIXEL_ARGB_8888, 0, flip, true, 1.0f ) );
}

int RapiImageReader :: GetAllSurfaces( ImageColl& coll, bool flip )
{
	int oldSize = coll.size();

	bool more = true;
	do
	{
		RapiMemImage* image = new RapiMemImage( GetWidth(), GetHeight() );
		more = GetNextSurface( *image, flip );
		coll.push_back( image );
	}
	while ( more );

	return ( coll.size() - oldSize );
}

//////////////////////////////////////////////////////////////////////////////
// misc implementations (writers)

bool WriteImage( FileSys::StreamWriter& writer, eImageFormat format, RapiImageReader* reader, bool flip, bool includeMips )
{
	gpassert( reader != NULL );

	// add any additional mips
	int surfaceCount = 1;
	if ( includeMips )
	{
		surfaceCount += max_t( 0, min_t( ::GetShift( reader->GetWidth () ),
										 ::GetShift( reader->GetHeight() ) ) );
	}

	// store it
	if ( format == IMAGE_RAW )
	{
		// set up header
		RapiRawReader::RawHeader rawHeader;
		rawHeader.m_HeaderMagic  = RapiRawReader::HEADER_MAGIC;
		rawHeader.m_Format       = RapiRawReader::FORMAT_ARGB_8888;
		rawHeader.m_Flags        = RapiRawReader::FLAG_NONE;
		rawHeader.m_SurfaceCount = (WORD)surfaceCount;
		rawHeader.m_Width        = (WORD)reader->GetWidth();
		rawHeader.m_Height       = (WORD)reader->GetHeight();

		// write it out
		if ( !writer.WriteStruct( rawHeader ) )  return ( false );

		// write out mips
		for ( int i = 0 ; i < rawHeader.m_SurfaceCount ; ++i )
		{
			RapiMemImage image;
			reader->GetNextSurface( image, flip );
			if ( flip )
			{
				for ( DWORD* end = image.GetBitsEnd() - image.GetWidth() ; end != image.GetBits() ; end -= image.GetWidth() )
				{
					if ( !writer.Write( end, image.GetWidth() * 4 ) )  return ( false );
				}
			}
			else
			{
				if ( !writer.Write( image.GetBits(), image.GetWidth() * image.GetHeight() * 4 ) )  return ( false );
			}
		}
	}
	else
	{
		// get output buffer size
		int width = reader->GetWidth(), height = reader->GetHeight();
		if ( includeMips && (surfaceCount > 1) )
		{
			if ( ::IsPower2( width ) && ::IsPower2( height ) )
			{
				height += height / 2;
			}
			else
			{
				gperror( "Mipmaps requested to be written, but source image is not a power of 2, request ignored\n" );
				includeMips = false;
			}
		}

		// output image
		if ( format == IMAGE_BMP )
		{
			// build buffer
			int stride = width * 3;
			std::vector <BYTE> image( stride * height );
			BYTE* out = image.begin();
			if ( flip && (surfaceCount > 1) )
			{
				out += stride * (reader->GetHeight() / 2);
			}

			// set up header
			RapiBmpReader::RawHeader rawHeader;
			rawHeader.bfType		= RapiBmpReader::HEADER_MAGIC;
			rawHeader.bfOffBits		= sizeof( rawHeader );
			rawHeader.bfSize		= sizeof( rawHeader ) + image.size();
			rawHeader.biSize		= sizeof( BITMAPINFOHEADER );
			rawHeader.biWidth		= width;
			rawHeader.biHeight		= height;
			rawHeader.biPlanes		= 1;
			rawHeader.biCompression	= BI_RGB;
			rawHeader.biBitCount	= 24;

			// write it out
			if ( !writer.WriteStruct( rawHeader ) )  return ( false );

			// build output image
			for ( int surface = 0 ; surface < surfaceCount ; ++surface )
			{
				reader->GetNextSurface( out, PIXEL_RGB_888, stride, !flip, true, 1.0f );

				if ( flip )
				{
					if ( surface == 0 )
					{
						out = image.begin();
					}
					else
					{
						out += (stride * reader->GetHeight()) + (reader->GetWidth() * 2 * 3);
					}
				}
				else
				{
					if ( surface == 0 )
					{
						out += stride * reader->GetHeight() * 2;
					}
					else
					{
						out += reader->GetWidth() * 2 * 3;
					}
				}
			}

			// write it out
			if ( !writer.Write( image.begin(), image.size() ) )  return ( false );
		}
		else
		{
			GP_Unimplemented$$$();
		}
	}

	return ( true );
}

bool WriteImage( FileSys::StreamWriter& writer, eImageFormat format, const RapiMemImage& image, bool flip, bool includeMips, bool screenShot )
{
	std::auto_ptr <RapiImageReader> reader( RapiImageReader::CreateReader( "<mem>", image, includeMips, screenShot ) );
	return ( WriteImage( writer, format, reader.get(), flip, includeMips ) );
}

bool WriteImage( const char* fileName, eImageFormat format, RapiImageReader* reader, bool flip, bool includeMips )
{
	gpassert( fileName != NULL );

	// test for missing extension
	gpstring localFileName;
	const char* ext = FileSys::GetExtension( fileName );
	if ( ext == NULL )
	{
		localFileName = fileName;
		localFileName += ".";
		localFileName += GetExtension( format );
		fileName = localFileName;
	}

	// write
	FileSys::File file;
	if ( file.CreateAlways( fileName, FileSys::File::ACCESS_WRITE_ONLY, FileSys::File::SHARE_READ_ONLY ) )
	{
		FileSys::FileWriter writer( file );
		return ( WriteImage( writer, format, reader, flip, includeMips ) );
	}
	else
	{
		return ( false );
	}
}

bool WriteImage( const char* fileName, eImageFormat format, const RapiMemImage& image, bool flip, bool includeMips, bool screenShot )
{
	std::auto_ptr <RapiImageReader> reader( RapiImageReader::CreateReader( "<mem>", image, includeMips, screenShot ) );
	return ( WriteImage( fileName, format, reader.get(), flip, includeMips ) );
}

//////////////////////////////////////////////////////////////////////////////
// class RapiColorFiller implementation

bool RapiColorFiller :: GetNextSurface( void* out, ePixelFormat format, int stride, bool /*flip*/, bool /*allowDither*/, float saturation, int dstWidth, int dstHeight )
{
	UNREFERENCED_PARAMETER( dstWidth );
	DWORD color	= ScaleDWORDColorSaturation( m_Color, saturation );
	switch ( format )
	{
		case ( PIXEL_ARGB_8888 ):
		{
			std::fill( (DWORD*)out, (DWORD*)((BYTE*)out + (stride * (dstHeight > 0 ? dstHeight : m_Height))), color );
		}
		break;

		case ( PIXEL_ARGB_4444 ):
		{
			std::fill( (WORD*)out, (WORD*)((BYTE*)out + (stride * (dstHeight > 0 ? dstHeight : m_Height))), MAKE4444FROM8888( color ) );
		}
		break;

		case ( PIXEL_ARGB_1555 ):
		{
			std::fill( (WORD*)out, (WORD*)((BYTE*)out + (stride * (dstHeight > 0 ? dstHeight : m_Height))), MAKE1555FROM8888( color ) );
		}
		break;
	}

	return ( false );
}

//////////////////////////////////////////////////////////////////////////////
