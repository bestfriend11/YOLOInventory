//////////////////////////////////////////////////////////////////////////////
//
// File     :  Main.c
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include <lzo1x.h>

#ifdef _DEBUG
#pragma comment ( lib, "lzod.lib" )
#else
#pragma comment ( lib, "lzo.lib" )
#endif

//////////////////////////////////////////////////////////////////////////////
// helper function implementations

__declspec ( dllexport ) int __stdcall Deflate( void* out, unsigned int outSize, const void* in, unsigned int inSize )
{
	unsigned char buffer[ LZO1X_999_MEM_COMPRESS ];

	if ( inSize == 0 )
	{
		return ( 0 );
	}

	// $ maxi-crunch. not fast.

	// first compress it
	lzo1x_999_compress_level(
			(const unsigned char*)in, inSize,
			(unsigned char*)out, &outSize,
			buffer, 0, 0, 0, 9 );

	// go back and optimize it further
#if 0 // $$$ can't get this to work yet, it access violates
	lzo1x_optimize(
			(unsigned char*)in, inSize,		// $ not const-correct
			(unsigned char*)out, &outSize,
			NULL );
#endif // 0

	return ( outSize );
}

//////////////////////////////////////////////////////////////////////////////
