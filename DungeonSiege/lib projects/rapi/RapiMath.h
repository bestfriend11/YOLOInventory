#pragma once
#ifndef _RAPI_MATH_
#define _RAPI_MATH_

/**********************************************************************************************
**
**										RapiMath
**
**		This is a collection of inline functions that will help a Rapi client
**		perform neccesary pixel and lighting operations.  They have been optimized to
**		provide the best performance possible.
**
**		Author:	James Loe
**
**		Copyright (c) 1999-2001,   Gas Powered Games
**
**********************************************************************************************/



// Creation of a DWORD color value from a vector of floats
inline DWORD MAKEDWORDCOLOR( const vector_3& v )
{
	return ( 0xFF000000 | ( FTOL( v.x * 255.0f ) << 16 ) | ( FTOL( v.y * 255.0f ) << 8 ) | FTOL( v.z * 255.0f ) );
}

// Creation of a DWORD color value from a vector of floats
inline DWORD MAKEDWORDCOLOR( float r, float g, float b )
{
	return ( 0xFF000000 | ( FTOL( r * 255.0f ) << 16 ) | ( FTOL( g * 255.0f ) << 8 ) | FTOL( b * 255.0f ) );
}

// Creation of a DWORD color value from a vector of floats
inline DWORD MAKEDWORDCOLORA( const vector_3& v, float a )
{
	return ( ( FTOL( a * 255.0f ) << 24 ) | ( FTOL( v.x * 255.0f ) << 16 ) | ( FTOL( v.y * 255.0f ) << 8 ) | FTOL( v.z * 255.0f ) );
}

// Creation of a DWORD color value from floats
inline DWORD MAKEDWORDCOLORA( float r, float g, float b, float a )
{
	return ( ( FTOL( a * 255.0f ) << 24 ) | ( FTOL( r * 255.0f ) << 16 ) | ( FTOL( g * 255.0f ) << 8 ) | FTOL( b * 255.0f ) );
}

// Retrieve alpha component from a DWORD color
inline float GETDWORDCOLORA( DWORD d )
{
	return ( (float)(BYTE)(d >> 24) * (1.0f / 255.0f) );
}

// Retrieve red component from a DWORD color
inline float GETDWORDCOLORR( DWORD d )
{
	return ( (float)(BYTE)(d >> 16) * (1.0f / 255.0f) );
}

// Retrieve green component from a DWORD color
inline float GETDWORDCOLORG( DWORD d )
{
	return ( (float)(BYTE)(d >>  8) * (1.0f / 255.0f) );
}

// Retrieve blue component from a DWORD color
inline float GETDWORDCOLORB( DWORD d )
{
	return ( (float)(BYTE)(d      ) * (1.0f / 255.0f) );
}

// Build a vector to represent the color of a DWORD
inline vector_3 GETDWORDCOLORV( DWORD d )
{
	return ( vector_3(GETDWORDCOLORR(d),GETDWORDCOLORG(d),GETDWORDCOLORB(d)));
}

// Take a 32 bit color and turn it into a 1555 format 16 bit color
inline WORD MAKE1555FROM8888( DWORD a )
{
	return (WORD)( ((a&0x000000F8)>>3) | ((a&0x0000F800)>>6) | ((a&0x00F80000)>>9) | ((a&0x80000000)>>16) );
}

// Take a 32 bit color and turn it into a 565 format 16 bit color
inline WORD MAKE565FROM8888( DWORD a )
{
	return (WORD)( ((a&0x000000F8)>>3) | ((a&0x0000FC00)>>5) | ((a&0x00F80000)>>8) );
}

// Take a 32 bit color and turn it into a 4444 format 16 bit color
inline WORD MAKE4444FROM8888( DWORD a )
{
	return (WORD)( ((a&0x000000F0)>>4) | ((a&0x0000F000)>>8) | ((a&0x00F00000)>>12) | ((a&0xF0000000)>>16) );
}

// Take a 1555 format 16 bit color and turn it into a 32 bit color
inline DWORD MAKE8888FROM1555( WORD a )
{
	return ( ((a&0x8000)<<16) | (FTOL(((float)((a&0x7C00)>>10)/31.0f)*255.0f)<<16) | (FTOL(((float)((a&0x03E0)>>5)/31.0f)*255.0f)<<8) | (FTOL(((float)(a&0x001F)/31.0f)*255.0f)) );
}

// Taking a 565 format 16 bit color and turn it into a 32 bit color
inline DWORD MAKE8888FROM565( WORD a )
{
	return ( (FTOL(((float)((a&0xF800)>>11)/31.0f)*255.0f)<<16) | (FTOL(((float)((a&0x07E0)>>5)/63.0f)*255.0f)<<8) | (FTOL(((float)(a&0x001F)/31.0f)*255.0f)) );
}

// Taking a 4444 format 16 bit color and turn it into a 32 bit color
inline DWORD MAKE8888FROM4444( WORD a )
{
	return ( (FTOL(((float)((a&0xF000)>>12)/15.0f)*255.0f)<<24) | (FTOL(((float)((a&0x0F00)>>8)/15.0f)*255.0f)<<16) | (FTOL(((float)((a&0x00F0)>>4)/15.0f)*255.0f)<<8) | (FTOL(((float)(a&0x000F)/15.0f)*255.0f)) );
}

// Taking a 565 format to a 1555 format
inline WORD MAKE1555FROM565( WORD a )
{
	return (WORD)( 0x8000 | ((a >> 1) & 0x7FE0) | (a & 0x001F) );
}

// Taking a 1555 format to a 565 format
inline WORD MAKE565FROM1555( WORD a )
{
	return (WORD)( ((a & 0x7FE0) << 1) | (a & 0x001F) );
}

// Taking a 4444 format to a 1555 format
inline WORD MAKE1555FROM4444( WORD a )
{
	return (WORD)( (a & 0x8000) | ((a & 0x0F00) << 3) | ((a & 0x00F0) << 2) | ((a & 0x000F) << 1) );
}

// Taking a 1555 format to a 4444 format
inline WORD MAKE4444FROM1555( WORD a )
{
	return (WORD)( (a & 0x8000) | ((a >> 3) & 0x0F00) | ((a >> 2) & 0x00F0) | ((a >> 1) & 0x000F) );
}

// Taking a 4444 format to a 565 format
inline WORD MAKE565FROM4444( WORD a )
{
	return (WORD)( ((a & 0x0F00) << 4) | ((a & 0x00F0) << 3) | ((a & 0x000F) << 1) );
}

// Taking a 565 format to a 4444 format
inline WORD MAKE4444FROM565( WORD a )
{
	return (WORD)( 0xF000 | ((a >> 4) & 0x0F00) | ((a >> 3) & 0x00F0) | ((a >> 1) & 0x000F) );
}

// Taking a Win32 COLORREF to a 8888 format
inline DWORD MAKE8888FROMCOLORREF( COLORREF c )
{
	return ( 0xFF000000 | (GetRValue( c ) << 16) | (GetGValue( c ) << 8) | GetBValue( c ) );
}

// Taking a Win32 COLORREF to a 1555 format
inline WORD MAKE1555FROMCOLORREF( COLORREF c )
{
	// $ i'm lazy - sb
	return ( MAKE1555FROM8888( MAKE8888FROMCOLORREF( c ) ) );
}

// Taking a Win32 COLORREF to a 565 format
inline WORD MAKE565FROMCOLORREF( COLORREF c )
{
	// $ i'm lazy - sb
	return ( MAKE565FROM8888( MAKE8888FROMCOLORREF( c ) ) );
}

// Set the alpha component of a color DWORD
inline DWORD SetDWORDAlpha( DWORD src, DWORD alpha )
{
	return (src & 0x00FFFFFF) | (alpha << 24);
}

// Scale and set the alpha component of a color DWORD
inline DWORD ScaleDWORDAlpha( DWORD src, DWORD scale )
{
	DWORD r = scale * ((src >> 8) & 0x00FF0000) + 0x00800000;
	return( ((r + ((r >> 8) & 0x00FF0000)) & 0xFF000000) | (src & 0x00FFFFFF) );
}

// Fill the alpha component of a number of DWORD colors
inline void FillDWORDAlpha( DWORD* pSrc, DWORD numDwords, BYTE alpha )
{
	__asm
	{
		mov		al, byte ptr [alpha]	// Move our alpha component into AL
		mov		edx, pSrc				// Move our DWORD pointer into EBX
		mov		ecx, numDwords			// Move our counter into ECX

LOOPFDA:

		mov		byte ptr [edx+3], al	// Move alpha into destination
		add		edx, 4					// Increment pointer to next element

		dec		ecx						// Decrement counter
		jnz		LOOPFDA					// Continue looping
	}
}

// Fill the alpha component of a number of strided structures
inline void FillDWORDAlphaStrided( void* pSrc, DWORD strideLen, DWORD numElements, BYTE alpha )
{
	__asm
	{
		mov		al, byte ptr [alpha]	// Move our alpha component into AL
		mov		ebx, strideLen			// Move our stride length into EBX
		mov		edx, pSrc				// Move our DWORD pointer into EDX
		mov		ecx, numElements		// Move our counter into ECX

LOOPFDAS:

		mov		byte ptr [edx+3], al	// Move alpha into destination
		add		edx, ebx				// Increment pointer to next element

		dec		ecx						// Decrement counter
		jnz		LOOPFDAS				// Continue looping
	}
}

// Add two DWORD colors together with saturation
inline DWORD AddDWORDColor( DWORD src, DWORD color )
{
	DWORD first		= (color & 0x00FF00FF) + (src & 0x00FF00FF);
	DWORD second	= (color & 0x0000FF00) + (src & 0xFF00FF00);

	if( first & 0xFF000000 )
	{
		// Saturate red
		first = (first & 0x0000FFFF) | 0x00FF0000;
	}
	if( first & 0x0000FF00 )
	{
		// Saturate blue
		first = (first & 0x00FF0000) | 0x000000FF;
	}
	if( second & 0x00FF0000 )
	{
		// Saturate green
		second = (second & 0xFF000000) | 0x0000FF00;
	}

	return( first + second );
}

// Subtract two DWORD colors with saturation
inline DWORD SubDWORDColor( DWORD src, DWORD color )
{
	DWORD first		= (src & 0x00FF0000) - (color & 0x00FF0000);
	DWORD second	= (src & 0x0000FF00) - (color & 0x0000FF00);
	DWORD third		= (src & 0x000000FF) - (color & 0x000000FF);

	if( first & 0xFF00FFFF )
	{
		// Saturate red
		first = 0;
	}
	if( second & 0xFFFF00FF )
	{
		// Saturate blue
		second = 0;
	}
	if( third & 0xFFFFFF00 )
	{
		// Saturate green
		third = 0;
	}

	return( (first + second + third) | (src & 0xFF000000) );
}

// Fixed point color scaling
inline DWORD ScaleDWORDColor( DWORD src, DWORD scale )
{
	DWORD first		= scale * (src & 0x00FF00FF) + 0x00800080;
	DWORD second	= scale * ((src >> 8) & 0x000000FF) + 0x00000080;

	return( ((first + ((first >> 8) & 0x00FF00FF)) >> 8 & 0x00FF00FF) +
		    ((second + ((second >> 8) & 0x000000FF)) & 0x0000FF00) +
			(src & 0xFF000000) );
}

// Scale a DWORD color by a floating point number between 0.0f and 1.0f
inline DWORD fScaleDWORDColor( DWORD src, float const scale )
{
	gpassert( scale >= 0.0f && scale <= 1.0f );
	return ScaleDWORDColor( src, FTOL( scale * 255.0f ) );
}

// Returns ( src + ( color * scale ) )
inline void ScaleAndAddDWORDColor( DWORD& src, const DWORD& color, DWORD scale )
{
	DWORD first		= scale * (color & 0x00FF00FF) + 0x00800080;
	DWORD second	= scale * ((color >> 8) & 0x000000FF) + 0x00000080;

	first			= ((first + ((first >> 8) & 0x00FF00FF)) >> 8 & 0x00FF00FF) + (src & 0x00FF00FF);
	second			= ((second + ((second >> 8) & 0x000000FF)) & 0x0000FF00) + (src & 0xFF00FF00);

	if( first & 0xFF000000 )
	{
		// Saturate red
		first = (first & 0x0000FFFF) | 0x00FF0000;
	}
	if( first & 0x0000FF00 )
	{
		// Saturate blue
		first = (first & 0x00FF0000) | 0x000000FF;
	}
	if( second & 0x00FF0000 )
	{
		// Saturate green
		second = (second & 0xFF000000) | 0x0000FF00;
	}

	src				= first + second;
}

// Scale a DWORD color by a floating point number between 0.0f and 1.0f and add to source
inline void fScaleAndAddDWORDColor( DWORD& src, const DWORD& color, float const scale )
{
	gpassert( scale >= -FLOAT_TOLERANCE && scale <= 1.0f+FLOAT_TOLERANCE );
	ScaleAndAddDWORDColor( src, color, FTOL( scale * 255.0f ) );
}

// Returns ( src - ( color * scale ) )
inline void ScaleAndSubDWORDColor( DWORD& src, const DWORD& color, DWORD scale )
{
	DWORD first		= scale * (color & 0x00FF00FF) + 0x00800080;
	DWORD second	= scale * ((color >> 8) & 0x000000FF) + 0x00000080;

	first			= ((first + ((first >> 8) & 0x00FF00FF)) >> 8 & 0x00FF00FF);
	DWORD third		= (src & 0x000000FF) - (first & 0x000000FF);
	first			= (src & 0x00FF0000) - (first & 0x00FF0000);
	second			= (src & 0x0000FF00) - ((second + ((second >> 8) & 0x000000FF)) & 0x0000FF00);

	if( first & 0xFF00FFFF )
	{
		// Saturate red
		first = 0;
	}
	if( second & 0xFFFF00FF )
	{
		// Saturate green
		second = 0;
	}
	if( third & 0xFFFFFF00 )
	{
		// Saturate blue
		third = 0;
	}

	src				= (first + second + third) | (src & 0xFF000000);
}

// Scale a DWORD color by a floating point number between 0.0f and 1.0f and add to source
inline void fScaleAndSubDWORDColor( DWORD& src, const DWORD& color, float const s )
{
	gpassert( s >= 0.0f && s <= 1.0f );
	ScaleAndSubDWORDColor( src, color, FTOL( s * 255.0f ) );
}

// Interpolate between two colors
inline DWORD InterpolateColor( DWORD firstColor, DWORD secondColor, float amount )
{
	gpassert( !IsNegative( amount ) && amount <= (1.0f+FLOAT_TOLERANCE) );

	const DWORD scale	= FTOL( amount * 255.0f );

	const DWORD first	= secondColor & 0x00FF00FF;
	const DWORD second	= secondColor & 0x0000FF00;

	return( (((((((firstColor & 0x00FF00FF) - first) * scale) >> 8) + first) & 0x00FF00FF) |
			((((((firstColor & 0x0000FF00) - second) * scale) >> 8)  + second) & 0x0000FF00)) |
			(firstColor & 0xFF000000) );
}

// Interpolate between two colors
inline DWORD InterpolateColor( DWORD firstColor, DWORD secondColor, DWORD scale )
{
	const DWORD first	= secondColor & 0x00FF00FF;
	const DWORD second	= secondColor & 0x0000FF00;

	return( (((((((firstColor & 0x00FF00FF) - first) * scale) >> 8) + first) & 0x00FF00FF) |
			((((((firstColor & 0x0000FF00) - second) * scale) >> 8)  + second) & 0x0000FF00)) |
			(firstColor & 0xFF000000) );
}

// Fill a void structure pointer, using the stride offset, to fill DWORD colors.  This
// is especially useful when copying color information into vertex structures.
inline void FillDWORDColorStrided( void* pSrc, DWORD strideLen, const DWORD* pColors, DWORD numColors )
{
	__asm
	{
		mov		eax, pColors			// Move our color pointer into EAX
		mov		ebx, pSrc				// Move our void pointer into EBX
		mov		ecx, numColors			// Move our counter into ECX

LOOPFDCS:

		mov		edx, dword ptr [eax]
		add		eax, 4					// Increment pointer to next color

		mov		dword ptr [ebx], edx	// Move color into destination
		add		ebx, strideLen			// Increment pointer to next element

		dec		ecx						// Decrement the loop counter
		jnz		LOOPFDCS				// Continue looping
	}
}

inline void FillDWORDColorStrided( void* pSrc, DWORD strideLen, DWORD numElements, DWORD color )
{
	__asm
	{
		mov		eax, color				// Move our color into EAX
		mov		ebx, strideLen			// Move our stride length into EBX
		mov		ecx, numElements		// Move our counter into ECX
		mov		edx, pSrc				// Move our void pointer into EDX

LOOPFDCS:

		mov		dword ptr [edx], eax	// Move color into destination
		add		edx, ebx				// Increment pointer to next element

		dec		ecx						// Decrement the loop counter
		jnz		LOOPFDCS				// Continue looping
	}
}

inline DWORD ScaleDWORDColorSaturation( DWORD src, float satScale )
{
	// Separate out the color components
	BYTE r		= (BYTE)((src >> 16) & 0x000000FF);
	BYTE g		= (BYTE)((src >> 8)  & 0x000000FF);
	BYTE b		= (BYTE)((src)       & 0x000000FF);

	// Figure out the highest of the components
	BYTE max	= max_t( max_t( r, g ), b );

	// Check for grayscale
	if( IsZero( satScale ) )
	{
		return( (src & 0xFF000000) | (max << 16) | (max << 8) | max );
	}

	// Figure out the lowest of the components and then the delta between
	BYTE min	= min_t( min_t( r, g ), b );
	float delta	= (float)(max - min);

	// Check for white, cannot change the saturation of white
	if( !max )
	{
		return src;
	}
	else
	{
		satScale	*= (delta / (float)max);
	}

	// Calculate the hue as best as we can
	float hue;
	if( r == max )
	{
		hue		= (float)(g - b) / delta;
	}
	else if( g == max )
	{
		hue		= 2.0f + ((float)(b - r) / delta);
	}
	else
	{
		hue		= 4.0f + ((float)(r - g) / delta);
	}

	// Check for negative and correct
	if( hue < 0.0f )
	{
		hue		+= 6.0f;
	}

	// Calculate our new color parts using the given saturation
	int i_hue	= FTOL( floorf( hue ) );
	float f_hue	= hue - (float)i_hue;
	float v		= (float)max / 255.0f;
	float p		= v * (1.0f - satScale);
	float q		= v * (1.0f - (satScale * f_hue));
	float t		= v * (1.0f - (satScale * (1.0f - f_hue)));

	// Figure out what part of the color wheel we're on to generate new color
	switch( i_hue )
	{
		case( 0 ):
		{
			return( (src & 0xFF000000) | (MAKEDWORDCOLOR( v, t, p ) & 0x00FFFFFF) );
			break;
		}
		case( 1 ):
		{
			return( (src & 0xFF000000) | (MAKEDWORDCOLOR( q, v, p ) & 0x00FFFFFF) );
			break;
		}
		case( 2 ):
		{
			return( (src & 0xFF000000) | (MAKEDWORDCOLOR( p, v, t ) & 0x00FFFFFF) );
			break;
		}
		case( 3 ):
		{
			return( (src & 0xFF000000) | (MAKEDWORDCOLOR( p, q, v ) & 0x00FFFFFF) );
			break;
		}
		case( 4 ):
		{
			return( (src & 0xFF000000) | (MAKEDWORDCOLOR( t, p, v ) & 0x00FFFFFF) );
			break;
		}
		default:
		{
			return( (src & 0xFF000000) | (MAKEDWORDCOLOR( v, p, q ) & 0x00FFFFFF) );
			break;
		}
	}
}

#endif