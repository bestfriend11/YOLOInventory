//////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------
// File     :  GPGlobal.h
// Author(s):  <Original Author Unknown>, Scott Bilas
//
// Copyright © 1999 Gas Powered Games.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "GpGlobal.h"

#include "AppModule.h"
#include "AtlX.h"
#include "Config.h"
#include "DebugHelp.h"
#include "DrWatson.h"
#include "FileSysXfer.h"
#include "KernelTool.h"
#include "ReportSysDialogs.h"
#include "StdHelp.h"
#include "StringTool.h"
#include "gpMath.h"

#pragma warning ( push, 1 )		// ignore any warnings or changes that win32 has
#include <lmcons.h>
#include <mmsystem.h>
#include <eh.h>
#include <ddraw.h>
#include <d3d.h>
#include <dinput.h>
#include <dsound.h>
#include <dplay8.h>
#include <math.h>
#include <winsock.h>
#pragma warning ( pop )			// back to normal

#pragma comment( lib, "wsock32" )

#include "Res\GpCoreRes.rh"

//////////////////////////////////////////////////////////////////////////////
// COM

// Initializes COM and maintains status
bool GPCoInitialize()
{
	static bool bInitialized = false;

	if ( !bInitialized )
	{
		HRESULT hr = CoInitializeEx( 0, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY );
		if ( FAILED( hr ) )
		{
			gperrorf(( "Could not initialize COM! (Error is 0x%08X)", hr ));
			return ( false );
		}

		// Set our static so we don't initialize more than once
		bInitialized = true;
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// CRC-16 checksum

// adapted from CRC32 below

static UINT16 s_CRC16Table[] =
{
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
};

#define DO1( buf )  seed = (UINT16)(s_CRC16Table[ ((short)seed ^ (*buf++)) & 0xff ] ^ (seed >> 8))
#define DO2( buf )  DO1( buf );  DO1( buf )
#define DO4( buf )  DO2( buf );  DO2( buf )
#define DO8( buf )  DO4( buf );  DO4( buf )

UINT16 GetCRC16( UINT16 seed, const void* buf, UINT sizeInBytes )
{
	if ( buf == NULL )
	{
		return ( 0 );
	}

	const UINT8* buf8 = rcast <const UINT8*> ( buf );
	seed = (UINT16)(seed ^ 0xffff);

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

	return ( (UINT16)(seed & 0xffff) );
}

#undef DO1
#undef DO2
#undef DO4
#undef DO8

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

UINT32 GetCRC32( UINT32 seed, const void* buf, UINT sizeInBytes )
{
	// $ wacky DO stuff is to unroll the loop

	if ( buf == NULL )
	{
		return ( seed );
	}

	const UINT8* buf8 = rcast <const UINT8*> ( buf );
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

//////////////////////////////////////////////////////////////////////////////
// Adler32 checksum

// copied from zlib's adler32.c

#define BASE 65521L 	// largest prime smaller than 65536
#define NMAX 5552		// NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1

#define DO1( buf, i )  {  s1 += buf[ i ];  s2 += s1;  }
#define DO2( buf, i )  DO1( buf, i );  DO1( buf, i + 1 );
#define DO4( buf, i )  DO2( buf, i );  DO2( buf, i + 2 );
#define DO8( buf, i )  DO4( buf, i );  DO4( buf, i + 4 );
#define DO16( buf )    DO8( buf, 0 );  DO8( buf, 8 );

UINT32 GetAdler32( const void* buf, UINT sizeInBytes )
{
	if ( buf == NULL )
	{
		return ( 1 );
	}

	const UINT8* buf8 = rcast <const UINT8*> ( buf );
	int len = sizeInBytes;
	unsigned long s1 = 0, s2 = 0;
	int k;

	while ( len > 0 )
	{
		k = (len < NMAX) ? len : NMAX;
		len -= k;
		while ( k >= 16 )
		{
			DO16( buf8 );
			buf8 += 16;
			k -= 16;
		}
		if ( k != 0 ) do
		{
			s1 += *buf8++;
				s2 += s1;
		}
		while ( --k );

		s1 %= BASE;
		s2 %= BASE;
	}

	return ( ( s2 << 16 ) | s1 );
}

#undef BASE
#undef NMAX
#undef DO1
#undef DO2
#undef DO4
#undef DO8
#undef DO16

//////////////////////////////////////////////////////////////////////////////
/*
**      UUENCODE AND UUDECODE
**
**      (c) COPYRIGHT MIT 1995.
**      Please first read the full copyright statement in the file COPYRIGH.
**
** ACKNOWLEDGEMENT:
**      This code is taken from rpem distribution, and was originally
**      written by Mark Riordan.
**
** AUTHORS:
**      MR      Mark Riordan    riordanmr@clvax1.cl.msu.edu
**      AL      Ari Luotonen    luotonen@dxcern.cern.ch
**
** HISTORY:
**      Added as part of the WWW library and edited to conform
**      with the WWW project coding standards by:       AL  5 Aug 1993
**      Originally written by:                          MR 12 Aug 1990
**      Original header text:
** -------------------------------------------------------------
**  File containing routines to convert a buffer
**  of bytes to/from RFC 1113 printable encoding format.
**
**  This technique is similar to the familiar Unix uuencode
**  format in that it maps 6 binary bits to one ASCII
**  character (or more aptly, 3 binary bytes to 4 ASCII
**  characters).  However, RFC 1113 does not use the same
**  mapping to printable characters as uuencode.
**
**  Mark Riordan   12 August 1990 and 17 Feb 1991.
**  This code is hereby placed in the public domain.
** -------------------------------------------------------------
** $ Note: I also cleaned up the code a little -ScottB
** $$ FUTURE: Switch to base64 encoding (whatever is used in XML standard) -sb
** -------------------------------------------------------------
*/

static char SIX2PR[ 64 ] =
{
	'A','B','C','D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
	'a','b','c','d','e','f','g','h','i','j','k','l','m',
	'n','o','p','q','r','s','t','u','v','w','x','y','z',
	'0','1','2','3','4','5','6','7','8','9','+','/'
};

static unsigned char PR2SIX[ 256 ];

int GetUuMaxInSize( void )
{
	return ( 48 );
}

/*--- function GetUuEncodeSize --------------------------------------
 *
 *   Return the output buffer size necessary to encode the given
 *   input buffer size.
 *
 *    Entry    bytes   is the number of bytes to be encoded.
 *
 *    Exit     Returns the number of ASCII characters required to encode
 *             the input buffer.
 */
int GetUuEncodeSize( int bytes )
{
	return ( 1 + ((4 * bytes) / 3) );
}

/*--- function UuEncodeLine -----------------------------------------------
 *
 *   Encode a single line of binary data to a standard format that
 *   uses only printing ASCII characters (but takes up 33% more bytes).
 *
 *    Entry    in       points to a buffer of bytes.
 *             inBytes  is the number of bytes in that buffer.
 *                      This cannot be more than 48.
 *             out      points to an output buffer.  Be sure that this
 *                      can hold at least 1 + (4*inBytes)/3 characters.
 *
 *    Exit     out      contains the coded line.  The first 4*inBytes/3 bytes
 *                      contain printing ASCII characters representing
 *                      those binary bytes. This may include one or
 *                      two '=' characters used as padding at the end.
 *                      The last byte is a zero byte.
 *             Returns the number of ASCII characters in "out".
 */
int UuEncodeLine( const void* in, int inBytes, char* out )
{
	char* i = out;
	int count = inBytes;

	for ( const BYTE* bufin = rcast <const BYTE*> ( in ) ; count >= 3 ; count -= 3, bufin += 3 )
	{
		*i++ = SIX2PR[   bufin[ 0 ] >> 2 ];											// c1
		*i++ = SIX2PR[ ((bufin[ 0 ] << 4) & 060) | ((bufin[ 1 ] >> 4) & 017 ) ];	// c2
		*i++ = SIX2PR[ ((bufin[ 1 ] << 2) & 074) | ((bufin[ 2 ] >> 6) & 03  ) ];	// c3
		*i++ = SIX2PR[   bufin[ 2 ]       & 077 ];									// c4
	}

	// fill in the remainder
	if ( count == 2 )
	{
		// there are only 2 bytes left
		*i++ = SIX2PR[   bufin[ 0 ] >> 2 ];											// c1
		*i++ = SIX2PR[ ((bufin[ 0 ] << 4) & 060) | ((bufin[ 1 ] >> 4) & 017 ) ];	// c2
		*i++ = SIX2PR[ ((bufin[ 1 ] << 2) & 074) ];									// c3
		*i++ = '=';																	// c4
	}
	else if ( count == 1 )
	{
		// there is only 1 byte left
		*i++ = SIX2PR[   bufin[ 0 ] >> 2 ];											// c1
		*i++ = SIX2PR[ ((bufin[ 0 ] << 4) & 060) ];									// c2
		*i++ = '=';																	// c3
		*i++ = '=';																	// c4
	}

	// terminate, return size
	*i = '\0';
	return ( i - out );
}

void UuEncode( const void* in, int inBytes, gpstring& out )
{
	// alloc a temporary buffer for each uuencoded line
	char buffer[ 100 ];
	gpassert( ELEMENT_COUNT( buffer ) > GetUuEncodeSize( GetUuMaxInSize() ) );
	gpassert( in != out.begin() );

	// loop over lines writing each out
	const BYTE* iter = rcast <const BYTE*> ( in );
	while ( inBytes > 0 )
	{
		// encode and write a line
		int write = min( inBytes, GetUuMaxInSize() );
		out.append( buffer, UuEncodeLine( iter, write, buffer ) );
		out += '\n';

		// update pointers
		inBytes -= write;
		iter += write;
	}
}

/*--- function UuDecodeLine ------------------------------------------------
 *
 *  Decode an ASCII-encoded buffer back to its original binary form.
 *
 *    Entry    in          points to a uuencoded string.  It is
 *                         terminated by any character not in
 *                         the printable character table SIX2PR, but
 *                         leading whitespace is stripped.
 *             out         points to the output buffer; must be big
 *                         enough to hold the decoded string (generally
 *                         shorter than the encoded string) plus
 *                         as many as two extra bytes used during
 *                         the decoding process.
 *
 *    Exit     Returns the number of binary bytes decoded.
 *             out contains these bytes.
 */
int UuDecodeLine( const char* in, void* out, const char** next )
{
	static const int MAXVAL = 63;

	// if this is the first call, initialize the mapping table.
	static bool first = true;
	if ( first )
	{
		first = false;
		for ( int  i = 0 ; i < 256 ; ++i )
		{
			PR2SIX[ i ] = MAXVAL + 1;
		}
		for ( char j = 0 ; j < 64 ; ++j )
		{
			PR2SIX[ SIX2PR[ j ] ] = j;
		}
	}

	// strip leading whitespace
	while ( ::isspace( *in ) )
	{
		++in;
	}

	// figure out how many characters are in the input buffer.
	const char* bufin = in;
	while( PR2SIX[ *bufin ] <= MAXVAL )
	{
		++bufin;
	}
	int bytes = bufin - in;
	int decoded = ((bytes + 3) / 4) * 3;

	// fix up for malformed data
	if ( decoded > GetUuMaxInSize() )
	{
		bytes = (GetUuMaxInSize() * 4) / 3;
	}

	// decode
	bufin = in;
	for ( BYTE* bufout = rcast <BYTE*> ( out ) ; bytes > 0 ; bufin += 4, bytes -= 4 )
	{
		*bufout++ = (BYTE)((PR2SIX[ bufin[ 0 ] ] << 2) | (PR2SIX[ bufin[ 1 ] ] >> 4));
		*bufout++ = (BYTE)((PR2SIX[ bufin[ 1 ] ] << 4) | (PR2SIX[ bufin[ 2 ] ] >> 2));
		*bufout++ = (BYTE)((PR2SIX[ bufin[ 2 ] ] << 6) |  PR2SIX[ bufin[ 3 ] ]);
	}

	// cleanup count
	if ( bytes & 03 )
	{
		if ( PR2SIX[ bufin[ -2 ] ] > MAXVAL )
		{
			decoded -= 2;
		}
		else
		{
			decoded -= 1;
		}
	}

	// where did we stop?
	if ( next != NULL )
	{
		*next = bufin;
	}

	return ( decoded );
}

int UuDecode( const char* in, void* out, int outSize )
{
	gpassert( in != NULL );
	gpassert( outSize >= 0 );

	// alloc a temporary buffer for each uuencoded line
	char buffer[ 100 ];
	gpassert( ELEMENT_COUNT( buffer ) > GetUuMaxInSize() );

	// iterate until no more
	int totalDecoded = 0;
	while ( *in != '\0' )
	{
		int decoded = UuDecodeLine( in, buffer, &in );
		if ( (decoded == 0) && (*in != '\0') )
		{
			return ( false );
		}

		outSize -= decoded;
		if ( outSize < 0 )
		{
			return ( -1 );
		}

		totalDecoded += decoded;
		::memcpy( out, buffer, decoded );

		out = rcast <BYTE*> ( out ) + decoded;
	}

	return ( totalDecoded );
}

//////////////////////////////////////////////////////////////////////////////
// encrypt/decrypt

// algorithm adapted from TEA (Tiny Encryption Algorithm)
//
// by Wheeler and Needham at Cambridge
// [http://www.ftp.cl.cam.ac.uk/ftp/papers/djw-rmn/djw-rmn-tea.html]

// randomly generated numbers from http://www.fourmilab.ch/hotbits/. we're
// storing the decryption key with the EXE so don't expect this to be safe (stop
// laughing, it's really not meant to be safe, it's just for casual users).

static const DWORD ENCRYPT_DUMMY1[] =
{
	0x7A566DC1, 0x02B831CE, 0x56DC2B9B, 0x298DCD6E,
	0x924F6ADF, 0xAF5FFB44, 0x79D9635F,
};

static const DWORD ENCRYPT_REAL[] =
{
	/*k0*/ 0x6FF27765, 0xFFFFDB76,
	/*k1*/ 0x93978D01, 0xDE6122B6, 0x08873220,
	/*k2*/ 0xF07F3170, 0xC2A4AD38,
	/*k3*/ 0xDBF07F42,
};

static const DWORD ENCRYPT_DUMMY2[] =
{
	0x5999A8D8, 0x1787005F, 0xFE21CDE0, 0x95FA976D,
	0xEE1D4A38, 0x4E9B1067, 0x445FC71F,
};

static const DWORD ENCRYPT_DELTA = 0x9e3779b9;

void TeaEncrypt( void* data, size_t length )
{
	TeaEncrypt( data, length, ENCRYPT_REAL[ 0 ], ENCRYPT_REAL[ 2 ], ENCRYPT_REAL[ 5 ], ENCRYPT_REAL[ 7 ] );
}

void TeaEncrypt( void* data, size_t length, DWORD k0, DWORD k1, DWORD k2, DWORD k3 )
{
	DWORD* ddata = rcast <DWORD*> (data);
	gpassert( IsAligned( length, 8 ) );

	// encode in place two dwords at a time
	while ( length > 8 )
	{
		DWORD y   = ddata[ 0 ];
		DWORD z   = ddata[ 1 ];
		DWORD sum = 0;

		for ( int n = 32 ; n > 0 ; --n )
		{
			sum += ENCRYPT_DELTA;
			y += (z << 4) + k0 ^ z + sum ^ (z >> 5) + k1;
			z += (y << 4) + k2 ^ y + sum ^ (y >> 5) + k3;
		}

		ddata[ 0 ] = y;
		ddata[ 1 ] = z;

		ddata += 2;
		length -= 8;
	}
}

void TeaEncrypt( gpstring& str )
{
	TeaEncrypt( str, ENCRYPT_REAL[ 0 ], ENCRYPT_REAL[ 2 ], ENCRYPT_REAL[ 5 ], ENCRYPT_REAL[ 7 ] );
}

void TeaEncrypt( gpstring& str, DWORD k0, DWORD k1, DWORD k2, DWORD k3 )
{
	// bail if nothing there
	if ( str.empty() )
	{
		return;
	}

	// first zero-expand the string and its null term to be qword aligned
	str.resize( GetAlignUp( str.size() + 1, 8 ) - 1, 0 );

	// now encrypt in place
	TeaEncrypt( str.begin_split(), str.size() + 1, k0, k1, k2, k3 );

	// uuencode to a temporary buffer
	gpstring out;
	UuEncode( str.begin(), str.size() + 1, out );

	// take it!
	out.swap( str );
}

void TeaEncrypt( gpstring& out, const gpwstring& in )
{
	TeaEncrypt( out, in, ENCRYPT_REAL[ 0 ], ENCRYPT_REAL[ 2 ], ENCRYPT_REAL[ 5 ], ENCRYPT_REAL[ 7 ] );
}

void TeaEncrypt( gpstring& out, const gpwstring& in, DWORD k0, DWORD k1, DWORD k2, DWORD k3 )
{
	// first clear 'er
	out.clear();

	// bail if nothing there
	if ( in.empty() )
	{
		return;
	}

	// working buffer
	gpwstring str( in );

	// first zero-expand the string and its null term to be qword aligned
	str.resize( (GetAlignUp( (str.size() + 1) * 2, 8 ) / 2) - 1, 0 );

	// now encrypt in place
	TeaEncrypt( str.begin_split(), (str.size() + 1) * 2, k0, k1, k2, k3 );

	// uuencode to output buffer
	UuEncode( str.begin(), (str.size() + 1) * 2, out );
}

void TeaDecrypt( void* data, size_t length )
{
	TeaDecrypt( data, length, ENCRYPT_REAL[ 0 ], ENCRYPT_REAL[ 2 ], ENCRYPT_REAL[ 5 ], ENCRYPT_REAL[ 7 ] );
}

void TeaDecrypt( void* data, size_t length, DWORD k0, DWORD k1, DWORD k2, DWORD k3 )
{
	DWORD* ddata = rcast <DWORD*> (data);
	gpassert( IsAligned( length, 8 ) );

	// decode in place two dwords at a time
	while ( length > 8 )
	{
		DWORD y   = ddata[ 0 ];
		DWORD z   = ddata[ 1 ];
		DWORD sum = ENCRYPT_DELTA << 5;

		for ( int n = 32 ; n > 0 ; --n )
		{
			z -= (y << 4) + k2 ^ y + sum ^ (y >> 5) + k3;
			y -= (z << 4) + k0 ^ z + sum ^ (z >> 5) + k1;
			sum -= ENCRYPT_DELTA;
		}

		ddata[ 0 ] = y;
		ddata[ 1 ] = z;

		ddata += 2;
		length -= 8;
	}
}

void TeaDecrypt( gpstring& str )
{
	TeaDecrypt( str, ENCRYPT_REAL[ 0 ], ENCRYPT_REAL[ 2 ], ENCRYPT_REAL[ 5 ], ENCRYPT_REAL[ 7 ] );
}

void TeaDecrypt( gpstring& str, DWORD k0, DWORD k1, DWORD k2, DWORD k3 )
{
	// bail if nothing there
	if ( str.empty() )
	{
		return;
	}

	// first uudecode to a temporary buffer (it won't be longer than encoded str)
	gpstring out( str.size(), 0 );
	int size = UuDecode( str, out.begin_split(), out.size() );
	if_gpassert ( size > 0 )
	{
		// now decrypt in place
		TeaDecrypt( out.begin_split(), size, k0, k1, k2, k3 );

		// now take the result (zero terminated)
		out.resize( ::strlen( out.c_str() ) );
		out.swap( str );
	}
}

void TeaDecrypt( gpwstring& out, const gpstring& in )
{
	TeaDecrypt( out, in, ENCRYPT_REAL[ 0 ], ENCRYPT_REAL[ 2 ], ENCRYPT_REAL[ 5 ], ENCRYPT_REAL[ 7 ] );
}

void TeaDecrypt( gpwstring& out, const gpstring& in, DWORD k0, DWORD k1, DWORD k2, DWORD k3 )
{
	// first clear 'er
	out.clear();

	// bail if nothing there
	if ( in.empty() )
	{
		return;
	}

	// working buffer
	gpstring str( in );

	// uudecode to the output buffer (it won't be longer than encoded str)
	out.resize( (str.size() + 1) / 2, 0 );
	int size = UuDecode( str, out.begin_split(), out.size() * 2 );
	if_gpassert ( size > 0 )
	{
		// now decrypt in place
		TeaDecrypt( out.begin_split(), size, k0, k1, k2, k3 );

		// now clamp the result
		out.resize( ::wcslen( out.c_str() ) );
	}
}

//////////////////////////////////////////////////////////////////////////////

/*  Interesting MSDN article on Win9x paging:

	http://msdn.microsoft.com/isapi/msdnlib.idc?theURL=/library/techart/msdn_gameperf.htm

	-- DATA MEMORY PAGING --

	Problem: Performance slows down as data is accessed from the swapfile, even
	though the application has attempted to load all data into memory by walking
	through it and "touching" it.

	Cause: The explanation requires some understanding of how Windows decides
	when a block of memory should be discarded or paged out, i.e., moved from
	RAM to the swapfile.

	The Windows 95 memory manager includes a safeguard against processes that
	use a block of memory once and then forget about it. Consider, for example,
	a large bitmap used as desktop wallpaper. The user has been running a
	maximized application, so that the bitmap has long since been paged out.
	Then the user minimizes the application for a moment. Now the system needs
	to reload the wallpaper bitmap so it can paint it onto the screen. Without a
	safeguard, the bitmap would elbow aside a lot of other pages from RAM, even
	though they might contain code or data that will be needed again as soon as
	the application is restored. The bitmap is unlikely to be needed again for
	some time, but it is now hogging physical memory.

	The safety mechanism prevents this from happening in the following way. When
	the system loads in a page (a page being 4 kilobytes on x86 machines), it
	first leaves it in a state of limbo. While it is in this state, the page
	never acquires a "last accessed" timestamp, even if it is accessed again. It
	is promoted to full status only when 16 more pages (in Windows 95) have been
	loaded. But at this point it is marked as not having been recently used, so
	it is still a prime candidate for disposal the next time physical memory is
	needed. Only if the page is accessed after promotion to full memory status
	does Windows mark it as recently used, thus recognizing it as potentially
	useful and worth keeping in physical memory.

	In the case of the desktop wallpaper example, this means that the first page
	loaded is likely to be discarded after 16 more pages have been loaded. As
	each new page is loaded, another one is promoted from limbo and one is
	discarded. The result is that loading an image of any size requires the
	replacement of as little as 17 x 4 = 68K of physical memory. (Again, the
	numbers are valid for Windows 95.)

	Any attempt to force Windows to keep a large block of data in physical
	memory by simply walking through it page by page will fail, because the
	safety mechanism treats the data just like the bitmap in our example.

	Solution: In order to have Windows recognize a page of memory as being too
	important to discard, you must touch it again as soon as it has been
	promoted from limbo. (Touching it before it is promoted will not have the
	desired effect.) The following function will accomplish this:

	// Assume that pb and cb are both rounded to page boundaries
	// and that PageSize has been determined from GetSystemInformation().

	#define cbLag  (16 * PageSize)

	void MakeRegionPresent(volatile BYTE *pb, UINT cb)
	{
		UINT ib;

		Assert(((DWORD)pb & (PageSize - 1)) == 0);
		Assert(((DWORD)cb & (PageSize - 1)) == 0);

		for (ib = cbLag; ib < cb; ib += PageSize)
		{
			pb[ib - cbLag];
			pb[ib];
		}
	}
*/

void MemTouchPages( const void* base, int size, bool fromPageFile )
{
	const DWORD pageSize = SysInfo::GetSystemPageSize();
	volatile BYTE* p = (BYTE*)GetAlignDown( (DWORD)base, pageSize );

	// special handling for the above article only required on pagefile stuff
	//  going on with win9x systems.
	if ( fromPageFile && SysInfo::IsOsWin9x() )
	{
		const DWORD lag = 16 * pageSize;
		for ( DWORD i = 0 ; i < (DWORD)size ; i += pageSize )
		{
			p[ i ];
			if( (i + lag) < (DWORD)size )
			{
				p[ i + lag ];
			}
		}
	}
	else
	{
		for ( DWORD i = 0 ; i < (DWORD)size ; i += pageSize )
		{
			p[ i ];
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// Misc functions

#pragma comment ( lib, "winmm" )

/*
	Notes on GetSystemSeconds():

	We want to use QueryPerformanceCounter() because it's got by far the best
	time function wrt accuracy, plus it also does not have a 49-day life.
	However on certain Intel boards there is a bug that Windows reacts to badly,
	which results in an occasional 4.7 second jump in QPC(). Info on the Intel
	bug:

		Errata 20. ACPI Timer Errata

		Problem: The power management timer may return improper result when
		read. Although the timer value settles properly after incrementing,
		while incrementing there is a 3nS window every 69.8nS where the timer
		value is indeterminate (a 4.2% chance that the data will be incorrect
		when read). As a result, the ACPI free running count up timer
		specification is violated due to erroneous reads.

		Implication: System hangs due to the “inaccuracy” of the timer when used
		by software for time critical events and delays.

		Workaround: Read the register twice and compare.

		Status: This will not be fixed in the PIIX4 or PIIX4E.

	And here is info on the Microsoft bug (KB #Q274323):

		SYMPTOMS
		========

		The result returned from QueryPerformanceCounter() on a machine running
		Windows 2000 may unexpectedly leap forward from time to time. This leap
		may represent several seconds.

		CAUSE
		=====

		This problem occurs as a result of a known bug in the Intel chipset for
		processors with an Intel 82371 PCI to ISA bridge. The jump will occur
		when the operating system gets a series of unexpected results from the
		hardware. Most machines will only experience this under a heavy PCI bus
		load.

		RESOLUTION
		==========

		To work around this problem, a program can watch for an unexpected jump
		by comparing the change in time determined by successive calls to
		QueryPerformanceCounter() with the change in time determined by
		successive calls to GetTickCount(). If there is a significant jump based
		on QueryPerformanceCounter() but no similar increase based on
		GetTickCount(), then it can be assumed that the performance counter just
		rolled forward.

		STATUS
		======

		This behavior is by design. The operating system is simply dealing with
		the bug in the chipset. The performance counter rollover is necessary
		when the hardware becomes unresponsive.

	So GetSystemSeconds() does the workaround, using timeGetTime instead.
*/


double GetSystemSeconds( void )
{
	// just make this whole function fast. most of these operations need to be precise :)
	ENTER_FPU_PRECISEMODE;

	static bool          s_Initialized;
	static DWORD         s_TimerAbsoluteZero = ::timeGetTime();
	static double        s_TimerWrapAdjust;
	static double        s_LastTime;
	static bool          s_QpcAvailable;
	static LARGE_INTEGER s_QpcAbsoluteZero;
	static LARGE_INTEGER s_QpcFrequency;
	static double        s_QpcSecondsConversion;

	// get multipliers
	if ( !s_Initialized )
	{
		s_Initialized = true;

		if (   ::QueryPerformanceFrequency( &s_QpcFrequency   )
			&& ::QueryPerformanceCounter  ( &s_QpcAbsoluteZero ) )
		{
			s_QpcAvailable = true;
			s_QpcSecondsConversion = 1.0 / s_QpcFrequency.QuadPart;
		}
	}

	double seconds = 0;

	// query time
	LARGE_INTEGER counter;
	if ( s_QpcAvailable && ::QueryPerformanceCounter( &counter ) )
	{
		// large gap? it could be a buggy chipset jump
		seconds = scast <double> ( counter.QuadPart - s_QpcAbsoluteZero.QuadPart ) * s_QpcSecondsConversion;
		if ( (seconds - s_LastTime) > 1.0f )
		{
			// check for necessary correction via a trusted counter
			double trueSeconds = ((::timeGetTime() - s_TimerAbsoluteZero) * 0.001) + s_TimerWrapAdjust;
			if ( trueSeconds < s_LastTime )
			{
				// timeGetTime going back in time means that we've probably
				// wrapped. trust qpc() instead, but update this so it will be
				// accurate again.
				s_TimerWrapAdjust += seconds - trueSeconds;
			}
			else if ( fabs( seconds - trueSeconds ) > 0.1 )
			{
				// use this time and readjust qpc
				s_QpcAbsoluteZero.QuadPart += (LONGLONG)((seconds - trueSeconds) / s_QpcSecondsConversion);
				seconds = trueSeconds;
			}
		}
	}
	else
	{
		// use mm timer
		seconds = ((::timeGetTime() - s_TimerAbsoluteZero) * 0.001) + s_TimerWrapAdjust;
	}

	// done
	s_LastTime = seconds;
	EXIT_FPU_PRECISEMODE;
	return ( seconds );
}

static double s_GlobalSecondsAdjust = 0;

double GetGlobalSeconds( void )
{
	return ( PreciseAdd( ::GetSystemSeconds(), s_GlobalSecondsAdjust ) );
}

void SetGlobalSecondsAdjust( double adjust )
{
	s_GlobalSecondsAdjust = adjust;
}

double GetGlobalSecondsAdjust( void )
{
	return ( s_GlobalSecondsAdjust );
}

void ConvertTime( int& hours, int& minutes, float& seconds )
{
	minutes =  int( seconds / 60.0f );
	seconds -= (minutes * 60);
	hours   =  minutes / 60;
	minutes -= (hours * 60);
}

void AccurateSleep( float seconds )
{
	ENTER_FPU_PRECISEMODE;

	// Sleep() may choose to return immediately if no other threads want the
	// time, depending on priority settings and what is blocking in the world.
	// so using Sleep() for "waiting" is not going to always yield accurate
	// results. AccurateSleep() works a little better for this.

	double time = GetSystemSeconds();
	while ( (GetSystemSeconds() - time) < seconds )
	{
		::Sleep( 0 );
	}
	EXIT_FPU_PRECISEMODE;
}

//////////////////////////////////////////////////////////////////////////////
// Random number generator

// Found at http://www.math.keio.ac.jp/~matumoto/cokus.c:
//
// This is the ``Mersenne Twister'' random number generator MT19937, which
// generates pseudorandom integers uniformly distributed in 0..(2^32 - 1)
// starting from any odd seed in 0..(2^32 - 1).  This version is a recode
// by Shawn Cokus (Cokus@math.washington.edu) on March 8, 1998 of a version by
// Takuji Nishimura (who had suggestions from Topher Cooper and Marc Rieffel in
// July-August 1997).
//
// Effectiveness of the recoding (on Goedel2.math.washington.edu, a DEC Alpha
// running OSF/1) using GCC -O3 as a compiler: before recoding: 51.6 sec. to
// generate 300 million random numbers; after recoding: 24.0 sec. for the same
// (i.e., 46.5% of original time), so speed is now about 12.5 million random
// number generations per second on this machine.
//
// According to the URL <http://www.math.keio.ac.jp/~matumoto/emt.html>
// (and paraphrasing a bit in places), the Mersenne Twister is ``designed
// with consideration of the flaws of various existing generators,'' has
// a period of 2^19937 - 1, gives a sequence that is 623-dimensionally
// equidistributed, and ``has passed many stringent tests, including the
// die-hard test of G. Marsaglia and the load test of P. Hellekalek and
// S. Wegenkittl.''  It is efficient in memory usage (typically using 2506
// to 5012 bytes of static data, depending on data type sizes, and the code
// is quite short as well).  It generates random numbers in batches of 624
// at a time, so the caching and pipelining of modern systems is exploited.
// It is also divide- and mod-free.
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Library General Public License as published by
// the Free Software Foundation (either version 2 of the License or, at your
// option, any later version).  This library is distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY, without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
// the GNU Library General Public License for more details.  You should have
// received a copy of the GNU Library General Public License along with this
// library; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA 02111-1307, USA.
//
// The code as Shawn received it included the following notice:
//
//   Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.  When
//   you use this, send an e-mail to <matumoto@math.keio.ac.jp> with
//   an appropriate reference to your work.
//
// It would be nice to CC: <Cokus@math.washington.edu> when you write.
//
// Performance note: in optimized builds, RandomDwordMT() is about the same
// speed as plain old rand().
//

void RandomGeneratorMT :: SetSeed( DWORD seed )
{
	//
	// We initialize m_State[ 0 .. (N - 1) ] via the generator
	//
	//   x_new = (69069 * x_old) mod 2^32
	//
	// from Line 15 of Table 1, p. 106, Sec. 3.3.4 of Knuth's
	// _The Art of Computer Programming_, Volume 2, 3rd ed.
	//
	// Notes (SJC): I do not know what the initial state requirements
	// of the Mersenne Twister are, but it seems this seeding generator
	// could be better.  It achieves the maximum period for its modulus
	// (2^30) iff x_initial is odd (p. 20-21, Sec. 3.2.1.2, Knuth); if
	// x_initial can be even, you have sequences like 0, 0, 0, ...;
	// 2^31, 2^31, 2^31, ...; 2^30, 2^30, 2^30, ...; 2^29, 2^29 + 2^31,
	// 2^29, 2^29 + 2^31, ..., etc. so I force seed to be odd below.
	//
	// Even if x_initial is odd, if x_initial is 1 mod 4 then
	//
	//   the          lowest bit of x is always 1,
	//   the  next-to-lowest bit of x is always 0,
	//   the 2nd-from-lowest bit of x alternates      ... 0 1 0 1 0 1 0 1 ... ,
	//   the 3rd-from-lowest bit of x 4-cycles        ... 0 1 1 0 0 1 1 0 ... ,
	//   the 4th-from-lowest bit of x has the 8-cycle ... 0 0 0 1 1 1 1 0 ... ,
	//    ...
	//
	// and if x_initial is 3 mod 4 then
	//
	//   the          lowest bit of x is always 1,
	//   the  next-to-lowest bit of x is always 1,
	//   the 2nd-from-lowest bit of x alternates      ... 0 1 0 1 0 1 0 1 ... ,
	//   the 3rd-from-lowest bit of x 4-cycles        ... 0 0 1 1 0 0 1 1 ... ,
	//   the 4th-from-lowest bit of x has the 8-cycle ... 0 0 1 1 1 1 0 0 ... ,
	//    ...
	//
	// The generator's potency (min. s>=0 with (69069-1)^s = 0 mod 2^32) is
	// 16, which seems to be alright by p. 25, Sec. 3.2.1.3 of Knuth.  It
	// also does well in the dimension 2..5 spectral tests, but it could be
	// better in dimension 6 (Line 15, Table 1, p. 106, Sec. 3.3.4, Knuth).
	//
	// Note that the random number user does not see the values generated
	// here directly since reloadMT() will always munge them first, so maybe
	// none of all of this matters.  In fact, the seed values made here could
	// even be extra-special desirable if the Mersenne Twister theory says
	// so-- that's why the only change I made is to restrict to odd seeds.
	//

	DWORD x = (seed | 1U) & 0xFFFFFFFFU;
	DWORD* s = m_State;
	int j;

	for ( m_Left = 0, *s++ = x, j = N ; --j ; *s++ = (x *= 69069U) & 0xFFFFFFFFU )
	{
		// $ just iterate
	}
}

DWORD RandomGeneratorMT :: RandomDword( void )
{
	DWORD y;

	if ( --m_Left < 0 )
	{
		DWORD* p0 = m_State;
		DWORD* p2 = m_State + 2;
		DWORD* pM = m_State + M;
		DWORD s0, s1;
		int j;

		if ( m_Left < -1 )
		{
			SetSeed( 4357U );
		}

		m_Left = N - 1;
		m_Next = m_State + 1;

#		define hiBit(u)       ((u) & 0x80000000U)   // mask all but highest   bit of u
#		define loBit(u)       ((u) & 0x00000001U)   // mask all but lowest    bit of u
#		define loBits(u)      ((u) & 0x7FFFFFFFU)   // mask     the highest   bit of u
#		define mixBits(u, v)  (hiBit(u)|loBits(v))  // move hi bit of u to hi bit of v

		for ( s0 = m_State[ 0 ], s1 = m_State[ 1 ], j = N - M + 1 ; --j ; s0 = s1, s1 = *p2++ )
		{
			*p0++ = *pM++ ^ (mixBits( s0, s1 ) >> 1) ^ (loBit( s1 ) ? K : 0U);
		}

		for ( pM = m_State, j = M ; --j ; s0 = s1, s1 = *p2++ )
		{
			*p0++ = *pM++ ^ (mixBits( s0, s1 ) >> 1) ^ (loBit( s1 ) ? K : 0U);
		}

		s1 = m_State[ 0 ];
		*p0 = *pM ^ (mixBits( s0, s1 ) >> 1) ^ (loBit( s1 ) ? K : 0U);
		y = s1;

#		undef hiBit
#		undef loBit
#		undef loBits
#		undef mixBits
	}
	else
	{
		y  = *m_Next++;
	}

	y ^= (y >> 11);
	y ^= (y <<  7) & 0x9D2C5680U;
	y ^= (y << 15) & 0xEFC60000U;

	return ( y ^ (y >> 18) );
}

DECL_THREAD static BYTE s_RandomGeneratorData[ sizeof( RandomGeneratorMT ) + 1 ];

RandomGeneratorMT& GetGlobalRng( void )
{
	if ( *s_RandomGeneratorData == 0 )
	{
		// tag as used
		*s_RandomGeneratorData = 0xFF;

		// new it up
		((RandomGeneratorMT*)(s_RandomGeneratorData + 1))->RandomGeneratorMT::RandomGeneratorMT();
	}

	return ( *(RandomGeneratorMT*)(s_RandomGeneratorData + 1) );
}

DWORD RandomGeneratorNR :: RandomDword( void )
{
	return ( m_Random = GetByteSwap( (m_Random * 1664525L) + 1013904223L ) );
}

//////////////////////////////////////////////////////////////////////////////
// Special "at final exit" functions

// for reference, see DEFSECTS.INC and ONEXIT.C in the CRT source. i'm using
// the "XCM" section to call atexit() so it will execute right before all
// internal crt cleanup executes. this guarantees that all global c++ objects
// have been destroyed but still occurs before library shutdown so automatic
// mfc leak detection reporting happens _after_ all our "after final exit"
// functions have run.

static VoidVoidProc* s_FinalExitProcBegin = NULL;
static VoidVoidProc* s_FinalExitProcEnd   = NULL;
static bool s_IsInFinalExit = false;

void __cdecl AtFinalExitProc( void )
{
	// setup
	s_IsInFinalExit = true;

	// execute
	for ( VoidVoidProc* i = s_FinalExitProcBegin ; i != s_FinalExitProcEnd ; ++i )
	{
		if ( *i != NULL )
		{
			(**i)();
		}
	}

	// delete
	::free( s_FinalExitProcBegin );
}

void __cdecl InitFinalExitProc( void )
{
	::atexit( AtFinalExitProc );
}

#pragma data_seg ( ".CRT$XCM" )
static VoidVoidProc s_ExitProc = InitFinalExitProc;
#pragma data_seg ()

void AddAtFinalExitProc( VoidVoidProc proc )
{
	// add a slot
	int count = s_FinalExitProcEnd - s_FinalExitProcBegin;
	s_FinalExitProcBegin = (VoidVoidProc*)::realloc( s_FinalExitProcBegin, sizeof( VoidVoidProc ) * (count + 1) );
	s_FinalExitProcEnd = s_FinalExitProcBegin + count;

	// save it
	*(s_FinalExitProcEnd++) = proc;
}

bool IsInFinalExit( void )
{
	return ( s_IsInFinalExit );
}

//////////////////////////////////////////////////////////////////////////////
// General version query implementations

SeException :: SeException( const EXCEPTION_POINTERS& info, const DWORD* lastError )
	: m_Information( info )
	, m_LastError( lastError ? *lastError : 0 )
	, m_HasLastError( lastError != NULL )
{
	// this space intentionally left blank...
}

SeException :: SeException( void )
{
	::ZeroObject( m_Information );
	::ZeroObject( m_LocalRecord );
	::ZeroObject( m_LocalContext );
	m_LastError = 0;
	m_HasLastError = false;
}

void SeException :: CopyLocal( const EXCEPTION_POINTERS& info, const DWORD* lastError )
{
	m_Information  = info;
	m_LastError    = lastError ? *lastError : 0;
	m_HasLastError = lastError != NULL;

	CopyLocal();
}

void SeException :: CopyLocal( const EXCEPTION_POINTERS& info, DWORD lastError )
{
	CopyLocal( info, &lastError );
}

void SeException :: CopyLocal( void )
{
	if ( m_Information.ExceptionRecord != &m_LocalRecord )
	{
		m_LocalRecord  = *m_Information.ExceptionRecord;
		m_LocalContext = *m_Information.ContextRecord;

		m_Information.ExceptionRecord = &m_LocalRecord;
		m_Information.ContextRecord   = &m_LocalContext;
	}
}

void SeException :: AcquireStackTrace( HANDLE hthread )
{
	m_StackTrace.clear();

	if ( m_Information.ContextRecord != NULL )
	{
		gDbgSymbolEngine.StackWalk( ::GetCurrentProcess(), hthread, *m_Information.ContextRecord, m_StackTrace );
	}
}

void SeException :: Dump( ReportSys::ContextRef ctx, bool allThreads ) const
{
	// optionally pause
	kerneltool::Thread::Pauser pauser;
	if ( allThreads )
	{
		pauser.Pause();
	}

	// begin a big report
	ReportSys::AutoReport autoReport( ctx );

	// dump process infoz
	DumpProcessTraits( ctx );

	// dump main thread
	ctx->OutputF( "*** Report for thread %s ***\n\n", kerneltool::Thread::MakeCurrentThreadName().c_str() );
	if ( allThreads )
	{
		ctx->Indent();
	}
	DumpException( *m_Information.ExceptionRecord, ctx );
	ctx->OutputEol();
	DumpThreadTraits( ::GetCurrentThread(), HasLastError() ? &m_LastError : NULL, ctx );
	ctx->OutputEol();
	PrivateDumpContext( ::GetCurrentThread(), ::GetCurrentThreadId(), *m_Information.ContextRecord, &m_StackTrace, ctx );
	ctx->OutputEol();
	if ( allThreads )
	{
		ctx->Outdent();
	}

	// optionally do the rest
	if ( allThreads )
	{
		// get all threads minus current (not just the ones that we paused)
		kerneltool::Thread::TInfoDb db;
		kerneltool::Thread::GetThreads( db, false );

		// dump them
		CONTEXT regs;
		gpstring name;
		kerneltool::Thread::TInfoDb::const_iterator i, begin = db.begin(), end = db.end();
		for ( i = begin ; i != end ; ++i )
		{
			const kerneltool::Thread::TInfo& tinfo = i->second;

			name = tinfo.m_Name;
			if ( name.empty() )
			{
				name = kerneltool::Thread::MakeThreadNameById( i->first );
			}

			ctx->OutputF( "*** Report for thread %s ***\n\n", name.c_str() );
			ctx->Indent();

			regs.ContextFlags = CONTEXT_FULL;
			if ( (tinfo.m_Handle != NULL) && ::GetThreadContext( tinfo.m_Handle, &regs ) )
			{
				DumpThreadTraits( tinfo.m_Handle, NULL, ctx );
				ctx->OutputEol();
				DumpContext( tinfo.m_Handle, i->first, regs, ctx );
			}
			else
			{
				ctx->OutputF( "*** Unable to dump context ***\n"
							  "    (OpenThread() may have failed or this is a Win9x system ***\n", name.c_str() );
			}

			ctx->OutputEol();
			ctx->Outdent();
		}
	}

	// optinally clean up
	if ( allThreads )
	{
		pauser.Resume();
	}
}

void SeException :: Dump( bool allThreads ) const
{
	Dump( NULL, allThreads );
}

void SeException :: DumpException( const EXCEPTION_RECORD& xrec, ReportSys::ContextRef ctx )
{
	// exception code
	ctx->OutputF( "Exception code: %s (%s)", ExceptionCodeToString( xrec.ExceptionCode ),
					(xrec.ExceptionFlags & EXCEPTION_NONCONTINUABLE) ? "noncontinuable" : "continuable" );
	if ( xrec.ExceptionCode == EXCEPTION_ACCESS_VIOLATION )
	{
		// special handling
		ctx->OutputF( " - attempted to %s data at 0x%08X\n",
						xrec.ExceptionInformation[ 0 ] ? "write" : "read",
						xrec.ExceptionInformation[ 1 ] );
	}
	else
	{
		ctx->OutputEol();
	}
	ctx->OutputF( "Occurred at IP: 0x%08X\n", xrec.ExceptionAddress );
}

void SeException :: DumpContext( HANDLE hthread, DWORD threadId, const CONTEXT& regs, ReportSys::ContextRef ctx )
{
	PrivateDumpContext( hthread, threadId, regs, NULL, ctx );
}

void SeException :: DumpContext( HANDLE hthread, DWORD threadId, const CONTEXT& regs, const AddressColl& addresses, ReportSys::ContextRef ctx )
{
	PrivateDumpContext( hthread, threadId, regs, &addresses, ctx );
}

void SeException :: PrivateDumpContext( HANDLE hthread, DWORD threadId, const CONTEXT& regs, const AddressColl* addresses, ReportSys::ContextRef ctx )
{
	// cpu registers
	ctx->OutputF( "CPU registers:\n\n" );
	{
		ReportSys::AutoIndent autoIndent( ctx );
		ctx->OutputF( "eax=%08X cs=%04X eip=%08X eflags=%08X\n", regs.Eax, regs.SegCs, regs.Eip, regs.EFlags );
		ctx->OutputF( "ebx=%08X ss=%04X esp=%08X ebp   =%08X\n", regs.Ebx, regs.SegSs, regs.Esp, regs.Ebp    );
		ctx->OutputF( "ecx=%08X ds=%04X esi=%08X fs    =%04X\n", regs.Ecx, regs.SegDs, regs.Esi, regs.SegFs  );
		ctx->OutputF( "edx=%08X es=%04X edi=%08X gs    =%04X\n", regs.Edx, regs.SegEs, regs.Edi, regs.SegGs  );
	}

	// fpu registers
	const FLOATING_SAVE_AREA& fregs = regs.FloatSave;
	ctx->OutputF( "\nFPU registers:\n\n" );
	{
		ReportSys::AutoIndent autoIndent( ctx );
		ctx->OutputF( "st0=%10f st4=%10f ctrl=%04X\n", *(float*)(fregs.RegisterArea +  0), *(float*)(fregs.RegisterArea + 40), (WORD)fregs.ControlWord );
		ctx->OutputF( "st1=%10f st5=%10f stat=%04X\n", *(float*)(fregs.RegisterArea + 10), *(float*)(fregs.RegisterArea + 50), (WORD)fregs.StatusWord );
		ctx->OutputF( "st2=%10f st6=%10f tags=%04X\n", *(float*)(fregs.RegisterArea + 20), *(float*)(fregs.RegisterArea + 60), (WORD)fregs.TagWord );
		ctx->OutputF( "st3=%10f st7=%10f\n",           *(float*)(fregs.RegisterArea + 30), *(float*)(fregs.RegisterArea + 70) );
	}

	// stack trace
	ctx->OutputF( "\nStack trace:\n\n" );
	{
		ReportSys::AutoIndent autoIndent( ctx );
		if ( (addresses == NULL) || addresses->empty() )
		{
			gDbgSymbolEngine.StackTrace( ::GetCurrentProcess(), hthread, threadId, regs, ctx );
		}
		else
		{
			gDbgSymbolEngine.StackTrace( ::GetCurrentProcess(), hthread, threadId, *addresses, ctx );
		}
	}
}

void SeException :: DumpProcessTraits( ReportSys::ContextRef ctx )
{
	// begin a big report
	ReportSys::AutoReport autoReport( ctx );

	// see if we have our extra kernel functions
	bool kernelAvailable = gKernel32Dll.Load( false );
//	bool psapiAvailable = gPsApiDll.Load( false );	// $$ currently unused -sb

	// dump process
	ctx->OutputF( "*** Report for process 0x%08X\n\n", ::GetCurrentProcessId() );
	{
		ReportSys::AutoIndent autoIndent( ctx );

		const char* processBoosted = "<unknown>";
		BOOL boosted = false;
		if ( kernelAvailable && gKernel32Dll.GetProcessPriorityBoost( ::GetCurrentProcess(), &boosted ) )
		{
			processBoosted = boosted ? "yes" : "no";
		}

		ctx->OutputF( "Process command line    : %s\n"
					  "Process priority class  : %d\n"
					  "Process priority boosted: %s\n",
					  ::GetCommandLineA(),
					  ::GetPriorityClass( ::GetCurrentProcess() ),
					  processBoosted );

		// $$$ add more fun here from PSAPI...

		ctx->OutputEol();
	}
}

void SeException :: DumpThreadTraits( HANDLE hthread, const DWORD* lastError, ReportSys::ContextRef ctx )
{
	ctx->Output( "Thread traits:\n\n" );

	ReportSys::AutoIndent autoIndent( ctx );

	ctx->OutputF( "Thread priority        : %d\n"
				  "Last error             : 0x%08X (%s)\n",
				  ::GetThreadPriority( hthread ),
				  lastError ? *lastError : 0,
				  lastError ? stringtool::GetLastErrorText( *lastError ).c_str() : "<unknown>" );

	if ( gKernel32Dll.Load( false ) )
	{
		const char* threadBoosted = "<unknown>";
		BOOL boosted = false;
		if ( gKernel32Dll.GetThreadPriorityBoost( hthread, &boosted ) )
		{
			threadBoosted = boosted ? "yes" : "no";
		}

		char threadCreation  [ 100 ] = "<unknown>";
		char threadKernelTime[ 100 ] = "<unknown>";
		char threadUserTime  [ 100 ] = "<unknown>";
		FILETIME createTime, exitTime, kernelTime, userTime;
		if ( gKernel32Dll.GetThreadTimes( hthread, &createTime, &exitTime, &kernelTime, &userTime ) )
		{
			FILETIME localTime;
			SYSTEMTIME createLocalTime;
			if (   ::FileTimeToLocalFileTime( &createTime, &localTime )
				&& ::FileTimeToSystemTime( &localTime, &createLocalTime ) )
			{
				::_snprintf( threadCreation, ELEMENT_COUNT( threadCreation ),
						"%02d/%02d/%4d,%02d:%02d:%02d.%d",
						createLocalTime.wMonth, createLocalTime.wDay, createLocalTime.wYear,
						createLocalTime.wHour, createLocalTime.wMinute, createLocalTime.wSecond, createLocalTime.wMilliseconds );
			}

			int hours, minutes;
			float seconds;

			seconds = (float)((double)(*(INT64*)(&kernelTime)) / 10000.0);		// FILETIME units is .1 msec
			::ConvertTime( hours, minutes, seconds );
			::_snprintf( threadKernelTime, ELEMENT_COUNT( threadKernelTime ), "%02d:%02d:%02.3f", hours, minutes, seconds );

			seconds = (float)((double)(*(INT64*)(&userTime)) / 10000.0);
			::ConvertTime( hours, minutes, seconds );
			::_snprintf( threadUserTime, ELEMENT_COUNT( threadUserTime ), "%02d:%02d:%02.3f", hours, minutes, seconds );
		}

		ctx->OutputF( "Thread priority boosted: %s\n"
					  "Thread creation time   : %s\n"
					  "Thread kernel mode time: %s\n"
					  "Thread user mode time  : %s\n",
					  threadBoosted,
					  threadCreation,
					  threadKernelTime,
					  threadUserTime );
	}
	else
	{
		ctx->Output( "<No more info available, this must be Win9x system>\n" );
	}
}

struct Entry
{
	DWORD       m_Code;
	const char* m_Name;

	bool operator == ( DWORD code ) const
		{  return ( m_Code == code );  }
};

const char* ExceptionCodeToString( DWORD code )
{
	static const Entry s_Entries[] =
	{
		{  0x40010005                        , "control-c"                ,  },
		{  0x40010008                        , "control-break"            ,  },
		{  EXCEPTION_GUARD_PAGE              , "guard page violation"     ,  },
		{  EXCEPTION_BREAKPOINT              , "breakpoint"               ,  },
		{  EXCEPTION_SINGLE_STEP             , "single step"              ,  },
		{  EXCEPTION_DATATYPE_MISALIGNMENT   , "datatype misalignment"    ,  },
		{  EXCEPTION_ACCESS_VIOLATION        , "access violation"         ,  },
		{  EXCEPTION_IN_PAGE_ERROR           , "in page error"            ,  },
		{  EXCEPTION_INVALID_HANDLE          , "invalid handle"           ,  },
		{  STATUS_NO_MEMORY                  , "no memory"                ,  },
		{  EXCEPTION_ILLEGAL_INSTRUCTION     , "illegal instruction"      ,  },
		{  EXCEPTION_NONCONTINUABLE_EXCEPTION, "noncontinuable exception" ,  },
		{  EXCEPTION_INVALID_DISPOSITION     , "invalid disposition"      ,  },
		{  EXCEPTION_ARRAY_BOUNDS_EXCEEDED   , "array bounds exceeded"    ,  },
		{  EXCEPTION_FLT_DENORMAL_OPERAND    , "float denormal operand"   ,  },
		{  EXCEPTION_FLT_DIVIDE_BY_ZERO      , "float divide by zero"     ,  },
		{  EXCEPTION_FLT_INEXACT_RESULT      , "float inexact result"     ,  },
		{  EXCEPTION_FLT_INVALID_OPERATION   , "float invalid operation"  ,  },
		{  EXCEPTION_FLT_OVERFLOW            , "float overflow"           ,  },
		{  EXCEPTION_FLT_STACK_CHECK         , "float stack check"        ,  },
		{  EXCEPTION_FLT_UNDERFLOW           , "float underflow"          ,  },
		{  EXCEPTION_INT_DIVIDE_BY_ZERO      , "integer divide by zero"   ,  },
		{  EXCEPTION_INT_OVERFLOW            , "integer overflow"         ,  },
		{  EXCEPTION_PRIV_INSTRUCTION        , "privileged instruction"   ,  },
		{  EXCEPTION_STACK_OVERFLOW          , "stack overflow"           ,  },
		{  0xC0000135                        , "dll not found"            ,  },
		{  STATUS_CONTROL_C_EXIT             , "control-c exit"           ,  },
		{  0xC0000142                        , "dll initialization failed",  },
		{  STATUS_FLOAT_MULTIPLE_FAULTS      , "float multiple faults"    ,  },
		{  STATUS_FLOAT_MULTIPLE_TRAPS       , "float multiple traps"     ,  },
		{  0xC06D007E                        , "module not found"         ,  },
		{  0xC06D007E                        , "procedure not found"      ,  },
		{  0xE06D7363                        , "microsoft c++ exception"  ,  },
		{  EXCEPTION_GPG_FATAL               , "gas powered fatal"        ,  },
	};

	const Entry* found = std::find( s_Entries, ARRAY_END( s_Entries ), code );
	if ( found != ARRAY_END( s_Entries ) )
	{
		return ( found->m_Name );
	}
	else
	{
		return ( "<unknown>" );
	}
}

typedef std::vector <LPTOP_LEVEL_EXCEPTION_FILTER> GlobalFilterColl;
static GlobalFilterColl s_GlobalFilterColl;

static DECL_THREAD DWORD s_UnfilteredHandledCount;

void RegisterGlobalExceptionFilter( LPTOP_LEVEL_EXCEPTION_FILTER filter )
{
	s_GlobalFilterColl.push_back( filter );
}

void UnregisterGlobalExceptionFilter( LPTOP_LEVEL_EXCEPTION_FILTER filter )
{
	GlobalFilterColl::iterator found = stdx::find( s_GlobalFilterColl, filter );
	if ( found != s_GlobalFilterColl.end() )
	{
		s_GlobalFilterColl.erase( found );
	}
}

LONG CALLBACK GlobalExceptionFilter(
		EXCEPTION_POINTERS* xinfo,
		LONG fatalReturn,
		LONG defaultReturn,
		bool execDefaultHandler,
		SeException* xtrace )
{
	// see if it's handled
	GlobalFilterColl::const_iterator i, ibegin = s_GlobalFilterColl.begin(), iend = s_GlobalFilterColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		LONG rc = (*i)( xinfo );
		if ( rc != EXCEPTION_CONTINUE_SEARCH )
		{
			return ( rc );
		}
	}

	// exec default?
	if ( execDefaultHandler && (xinfo->ExceptionRecord->ExceptionCode != EXCEPTION_GPG_FATAL) )
	{
		LPTOP_LEVEL_EXCEPTION_FILTER filter = SetUnhandledExceptionFilter( NULL );
		SetUnhandledExceptionFilter( filter );
		if ( filter != NULL )
		{
			(*filter)( xinfo );
		}
	}

	// if we're handling it, do some special work
	LONG rc = (xinfo->ExceptionRecord->ExceptionCode == EXCEPTION_GPG_FATAL) ? fatalReturn : defaultReturn;
	if ( rc == EXCEPTION_EXECUTE_HANDLER )
	{
		// note the handling
		++s_UnfilteredHandledCount;

		// copy and trace
		if ( xtrace != NULL )
		{
			// $ have to acquire the stack from within the exception filter. if
			//   we try to do it from an '__except' clause then it will sometimes
			//   not work (stack gets truncated after an entry or two).
			xtrace->CopyLocal( *xinfo, ::GetLastError() );
			xtrace->AcquireStackTrace( ::GetCurrentThread() );
		}
	}

	// return default
	return ( rc );
}

const DWORD& GlobalGetUnfilteredHandledExceptionCount( void )
{
	return ( s_UnfilteredHandledCount );
}

//////////////////////////////////////////////////////////////////////////////
// class ExceptionPreDlg declaration

class ExceptionPreDlg : public atlx::DialogImpl <ExceptionPreDlg>
{
public:
	SET_INHERITED( ExceptionPreDlg, atlx::DialogImpl <ExceptionPreDlg> );

// Ctor/dtor.

	ExceptionPreDlg( const SeException& sex, const char* headerMsg )
		: m_AutoSafe( true ), m_Sex( sex )
	{
		m_Sex.CopyLocal();

		if ( headerMsg != NULL )
		{
			m_HeaderMsg = headerMsg;
		}
	}

	virtual ~ExceptionPreDlg( void )
		{  }

// Commands.

	int  DoModal( HWND parent = NULL );
	void SetIcon( LPCTSTR icon );

private:

// Message map.

	BEGIN_MSG_MAP( ThisType )
		MESSAGE_CRACKER( WM_INITDIALOG, OnInitDialog )
	END_MSG_MAP()

// Message handlers.

	BOOL OnInitDialog( HWND focusctl, LPARAM init, BOOL& handled );

// Private data.

	ReportSys::AutoSafeMessageBox  m_AutoSafe;
	kerneltool::Thread::AutoPauser m_AutoThreadPauser;
	SeException                    m_Sex;
	gpstring                       m_HeaderMsg;

	SET_NO_COPYING( ExceptionPreDlg );
};

// $ note: implementation is after ExceptionDlg's

//////////////////////////////////////////////////////////////////////////////
// class ExceptionDlg declaration

static const int   EXCEPTION_FONT_SIZE   = 9;
static const char  EXCEPTION_FONT_NAME[] = "Lucida Console";

class ExceptionDlg : atlx::DialogImpl <ExceptionDlg>
{
public:
	SET_INHERITED( ExceptionDlg, atlx::DialogImpl <ExceptionDlg> );

// Ctor/dtor.

	ExceptionDlg( const char* msg )		: m_Message( msg ? msg : "" )  {  }
	 ~ExceptionDlg( void )				{  }

// Commands.

	int DoModal( HWND parent = NULL );

private:

// Message map.

	typedef atlx::ResizableImpl <ThisType> Resizable;

	BEGIN_MSG_MAP( ThisType )
		CHAIN_MSG_MAP_MEMBER( m_Resizable )
		MESSAGE_CRACKER     ( WM_INITDIALOG, OnInitDialog )
		MESSAGE_CRACKER     ( WM_ERASEBKGND, OnEraseBkgnd )
		COMMAND_CODE_HANDLER( BN_CLICKED,    OnButton     )
	END_MSG_MAP()

// Message handlers.

	BOOL    OnInitDialog( HWND focusctl, LPARAM init, BOOL& handled );
	BOOL    OnEraseBkgnd( HDC hdc, BOOL& handled );
	LRESULT OnButton    ( WORD code, WORD id, HWND ctl, BOOL& handled );

// Private data.

	// spec
	const char* m_Message;			// message to put in text box (by caller)

	// controls
	atlx::Edit m_ExceptionText;		// edit box for exception info text
	Resizable  m_Resizable;			// resizer helper

	// dialog state
	winx::Font  m_EditFont;			// font to use for edit control
	winx::Brush m_Brush;			// brush for background painting

	SET_NO_COPYING( ExceptionDlg );
};

int ExceptionDlg :: DoModal( HWND parent )
{
	static const WORD dlgTemplateData[] =
	{
		#define GP_DIALOG_EXCEPTION 1

		#if GP_DEBUG
		#include "Res\GpCoreRes-Debug.dat"
		#elif GP_RELEASE
		#include "Res\GpCoreRes-Release.dat"
		#elif GP_RETAIL
		#include "Res\GpCoreRes-Retail.dat"
		#elif GP_PROFILING
		#include "Res\GpCoreRes-Profiling.dat"
		#endif

		#undef  GP_DIALOG_EXCEPTION
	};

	if ( parent == NULL )
	{
		parent = winx::GetFrontWindow();
	}

	return ( Inherited::DoModal( rcast <const DLGTEMPLATE*> ( dlgTemplateData ), parent ) );
}

BOOL ExceptionDlg :: OnInitDialog( HWND /*focusctl*/, LPARAM /*init*/, BOOL& /*handled*/ )
{

// Initialize controls.

	// grab control
	m_ExceptionText = GetDlgItem( IDC_EXCEPTION_TEXT );

	// set up resizer
	m_Resizable.FixCornerToParent( GetDlgItem( IDC_EXCEPTION_TEXT ), atlx::CORNER_BOTTOMRIGHT );
	m_Resizable.FixPosToParent   ( GetDlgItem( IDABORT            ), atlx::CORNER_BOTTOMRIGHT );
	m_Resizable.FixPosToParent   ( GetDlgItem( IDREPORT           ), atlx::CORNER_BOTTOMRIGHT );
	m_Resizable.FixPosToParent   ( GetDlgItem( IDCANCEL           ), atlx::CORNER_BOTTOMRIGHT );
	m_Resizable.FixPosToParent   ( GetDlgItem( IDRETRY            ), atlx::CORNER_BOTTOMRIGHT );
	m_Resizable.FixPosToParent   ( GetDlgItem( IDC_COPY_EXCEPTION ), atlx::CORNER_BOTTOMRIGHT );

// Initialize defaults.

	// set icons
	HICON errorIcon = ::LoadIcon( NULL, IDI_ERROR );
	winx::Static::SetIcon( GetDlgItem( IDC_THEICON ), errorIcon );
	SetIcon( errorIcon );

	// build brush
	m_Brush.CreateHatch( HS_FDIAGONAL, RGB( 255, 0, 0 ) );

// Initialize text.

	// build a font spec
	winx::LogFont logFont;
	logFont.SetHeightPoints( EXCEPTION_FONT_SIZE );
	logFont.SetName( EXCEPTION_FONT_NAME );
	logFont.SetPitchAndFamily( FIXED_PITCH | FF_MODERN );

	// create the font
	m_EditFont.Create( logFont );

	// set font for controls
	m_ExceptionText.SetWindowFont( m_EditFont, FALSE );

	// init control with text
	if ( m_Message != NULL )
	{
		// use local context to convert newlines properly (so i'm lazy)
		ReportSys::EditCtlSink editSink( m_ExceptionText );
		ReportSys::LocalContext context( &editSink, false );
		context.Output( m_Message );

		// clear any selection
		m_ExceptionText.SetSelNone();
	}

// Finish up.

	// fullscreen means the main window is always on top - exception must be too
	if ( AppModule::DoesSingletonExist() && gAppModule.IsFullScreen() && gAppModule.HasVisibleMainWnd() )
	{
		SetAlwaysOnTop();
	}

	// center window
	if ( AppModule::DoesSingletonExist() && gAppModule.HasVisibleMainWnd() )
	{
		GRect rect( GetWindowRect() );
		rect.CenterIn( gAppModule.GetGameRect() );
		SetWindowPos( NULL, rect.left, rect.top, -1, -1, SWP_NOSIZE );
	}
	else
	{
		CenterWindow();
	}

	// finish
	GetDlgItem( IDC_COPY_EXCEPTION ).SetFocus();	// choose ignore as focus
	SetForeground();								// set us as foreground
	SetActiveWindow();								// activate and move to top

	// done
	return ( FALSE );
}

BOOL ExceptionDlg :: OnEraseBkgnd( HDC hdc, BOOL& handled )
{
	if ( m_Brush.DoesExist() )
	{
		::SetBkColor( hdc, ::GetSysColor( COLOR_BTNFACE ) );
		::FillRect( hdc, GetClientRect(), m_Brush );
		return ( TRUE );
	}
	else
	{
		handled = FALSE;
		return ( FALSE );
	}
}

LRESULT ExceptionDlg :: OnButton( WORD /*code*/, WORD id, HWND /*ctl*/, BOOL& handled )
{
	switch ( id )
	{

	// Folded.

		case ( IDABORT  ):		// abort program (nice)
		case ( IDCANCEL ):		// abort program (nasty)
		case ( IDREPORT ):		// report to Watson
		case ( IDRETRY  ):		// break into debugger
		{
			EndDialog( id );
		}
		break;

		case ( IDC_COPY_EXCEPTION ):
		{
			if ( !m_ExceptionText.CopyTextToClipboard() )
			{
				gperrorboxf(( "Unable to copy text to clipboard!\n\nError = 0x%08X ('%s')\n",
							  ::GetLastError(), stringtool::GetLastErrorText().c_str() ));
			}
		}
		break;

	// Unknown.

		default:  handled = FALSE;
	}

	// done
	return ( 0 );
}

//////////////////////////////////////////////////////////////////////////////
// class ExceptionPreDlg implementation

int ExceptionPreDlg :: DoModal( HWND parent )
{
	static const WORD dlgTemplateData[] =
	{
		#define GP_DIALOG_EXCEPTION_PRE 1

		#if GP_DEBUG
		#include "Res\GpCoreRes-Debug.dat"
		#elif GP_RELEASE
		#include "Res\GpCoreRes-Release.dat"
		#elif GP_RETAIL
		#include "Res\GpCoreRes-Retail.dat"
		#elif GP_PROFILING
		#include "Res\GpCoreRes-Profiling.dat"
		#endif

		#undef  GP_DIALOG_EXCEPTION_PRE
	};

	if ( parent == NULL )
	{
		parent = winx::GetFrontWindow();
	}

	return ( Inherited::DoModal( rcast <const DLGTEMPLATE*> ( dlgTemplateData ), parent ) );
}

void ExceptionPreDlg :: SetIcon( LPCTSTR icon )
{
	HICON hicon = ::LoadIcon( NULL, icon );
	winx::Static::SetIcon( GetDlgItem( IDC_THEICON ), hicon );
	Inherited::SetIcon( hicon );
}

BOOL ExceptionPreDlg :: OnInitDialog( HWND /*focusctl*/, LPARAM /*init*/, BOOL& /*handled*/ )
{
	// preset icon
	SetIcon( IDI_QUESTION );

	// fullscreen means the main window is always on top - exception must be too
	if ( AppModule::DoesSingletonExist() && gAppModule.IsFullScreen() && gAppModule.HasVisibleMainWnd() )
	{
		SetAlwaysOnTop();
	}

	// center window
	if ( AppModule::DoesSingletonExist() && gAppModule.HasVisibleMainWnd() )
	{
		GRect rect( GetWindowRect() );
		rect.CenterIn( gAppModule.GetGameRect() );
		SetWindowPos( NULL, rect.left, rect.top, -1, -1, SWP_NOSIZE );
	}
	else
	{
		CenterWindow();
	}

	// finish
	SetForeground();		// set us as foreground
	SetActiveWindow();		// activate and move to top

	// show it now
	ShowWindow( SW_SHOW );
	UpdateWindow();

	// output goes here
	gpstring dialogText;

	// process callbacks to gather full info to a file
	gpstring fileName;
	DWORD writeError = ERROR_SUCCESS;
	{
		// build filename for crash report
		fileName = DrWatson::GetOutputDirectory()
				 + FileSys::GetFileNameOnly( FileSys::GetModuleFileName() )
				 + ".crash";
		std::auto_ptr <FileSys::Writer> writer;
		FileSys::File file;
#		if GP_RETAIL
		if ( file.CreateAlways( fileName ) )
#		else // GP_RETAIL
		if ( file.OpenAlways( fileName, FileSys::File::ACCESS_READ_WRITE ) )
#		endif // GP_RETAIL
		{
			// skip to end
			file.SeekEnd();

			// register it
			DrWatson::RegisterFileForReport( fileName );

			// make writer
			stdx::make_auto_ptr( writer, new FileSys::FileWriter( file ) );

			// output header
			ReportSys::LocalContext ctx( new ReportSys::FileSysStreamerSink( *writer ), true );
			ReportSys::FileSink::OutputGenericHeader( &ctx, "Crash Report", true );

			// output header message if any
			if ( !m_HeaderMsg.empty() )
			{
				writer->WriteText( m_HeaderMsg );
				writer->WriteText( "\n\n" );
			}
		}
		else
		{
			writeError = ::GetLastError();
		}

		// gather info
		DrWatson::Info info;
		info.m_InfoFlags |= DrWatson::INFO_FULL;
		info.m_Sex        = &m_Sex;
		info.m_Writer     = writer.get();
		DrWatson::ExecuteInfoCallbacks( info );
	}

	// process callbacks to gather basic info if needed
	if ( !DrWatson::TestOptions( DrWatson::OPTION_FORCE_ENABLE ) )
	{
		// make writer
		FileSys::StringWriter writer( dialogText );

		// out header
		if ( !m_HeaderMsg.empty() )
		{
			writer.WriteText( m_HeaderMsg );
			writer.WriteText( "\n" );
		}

		// gather info
		DrWatson::Info info;
		info.m_Sex    = &m_Sex;
		info.m_Writer = &writer;
		DrWatson::ExecuteInfoCallbacks( info );

		// output summary
		if ( writeError == ERROR_SUCCESS )
		{
			writer.WriteF( "*** Full report written to '%s'\n", fileName.c_str() );
		}
		else
		{
			writer.WriteF(
					"*** Failed to write full report to '%s', error is '%s' (0x%08X)\n",
					fileName.c_str(), stringtool::GetLastErrorText( writeError ).c_str(), writeError );
		}
	}

	// show finished
	SetIcon( IDI_EXCLAMATION );
	SetDlgItemText( IDC_THETEXT, "Done!" );
	::Sleep( 200 );

	// optionally do dialog if we're not forcing dw-only
	int result = IDREPORT;
	if ( !DrWatson::TestOptions( DrWatson::OPTION_FORCE_ENABLE ) )
	{
		result = ExceptionDlg( dialogText ).DoModal( *this );
	}
	EndDialog( result );

	return ( TRUE );
}

//////////////////////////////////////////////////////////////////////////////
// exception function implementations

int DoModalExceptionDlg( const SeException& sex, const char* headerMsg, HWND parent )
{
	return ( ExceptionPreDlg( sex, headerMsg ).DoModal( parent ) );
}

LONG WINAPI ExceptionDlgFilter( const EXCEPTION_POINTERS* xinfo, DWORD lastError, const char* headerMsg, HWND parent )
{
	// caused by DShow or something - harmless
	if ( xinfo->ExceptionRecord->ExceptionCode == STATUS_ILLEGAL_INSTRUCTION )
	{
		return ( EXCEPTION_CONTINUE_SEARCH );
	}

	// breakpoint probably caused by int 3 - let it percolate so can jit debug
#	if !GP_RETAIL
	if ( xinfo->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT )
	{
		return ( EXCEPTION_CONTINUE_SEARCH );
	}
#	endif // !GP_RETAIL

	// copy the context before it gets blown away
	SeException sex( *xinfo, &lastError );
	sex.CopyLocal();

	// we're in an exception
	ReportSys::SetFatalIsOccurring();

	// check for recursive crash
	static bool s_Recursion = false;
	if ( s_Recursion )
	{
		// go out to top level
		return ( EXCEPTION_CONTINUE_SEARCH );
	}

	s_Recursion = true;

	// do dialog
	int rc = ::DoModalExceptionDlg( sex, headerMsg, parent );
	bool breakIntoDebugger = false;
	switch ( rc )
	{
		case ( IDREPORT ):
		{
			if ( DrWatson::TestOptions( DrWatson::OPTION_ENABLE ) )
			{
				LONG rc = DrWatson::ExceptionFilter( xinfo );
				if ( rc == 1 )
				{
					breakIntoDebugger = true;
				}
			}
		}
		break;

		case ( IDCANCEL ):
		{
			// nasty abort
			::TerminateProcess( ::GetCurrentProcess(), sex.GetCode() );
		}
		break;

		case ( IDRETRY ):
		{
			breakIntoDebugger = true;
		}
		break;
	}

	s_Recursion = false;

	if ( breakIntoDebugger )
	{
		// break into debugger
		BREAKPOINT();

		// continue execution at the point of failure - just keep stepping
		// out of functions until you get to the crasher.
		return ( EXCEPTION_CONTINUE_EXECUTION );
	}
	else
	{
		// default to nice abort, for which caller should handle and then re-throw
		// as a fatal.
		return ( EXCEPTION_EXECUTE_HANDLER );
	}
}

//////////////////////////////////////////////////////////////////////////////
// struct RecurseLocker implementation

#if !GP_RETAIL

static kerneltool::Critical s_RecurseCritical;

RecurseLocker :: RecurseLocker( DWORD cookie, const char* name, RecurseTrackerDb& db, bool readOnly )
	: m_Name    ( name )
	, m_Db      ( db )
	, m_ReadOnly( readOnly )
	, m_Critical( cookie ? &s_RecurseCritical : NULL )
{
	kerneltool::Critical::OptionalLock locker( m_Critical );

	m_Tracker = m_Db.insert( std::make_pair( cookie, RecurseTracker() ) ).first;
	RecurseTracker& tracker = m_Tracker->second;

	if ( m_ReadOnly )
	{
		gpassert( tracker.m_ReadLock >= 0 );
		++tracker.m_ReadLock;
	}
	else
	{
		static bool s_IsDebugging = SysInfo::IsDebuggerPresent();
		if ( tracker.m_WriteLock )
		{
			// $$$ special: we want to catch this thing RIGHT NOW to
			//     see what the other thread is doing that it's got
			//     the handle outstanding. so just force a
			//     breakpoint immediately if we're in the debugger.
			//     that will catch it before this thread loses its
			//     time slice...
			if ( s_IsDebugging )
			{
				BREAKPOINT();
			}

			gperrorf(( "Very bad: recursion in '%s' detected! Attempting to %s "
					   "when a write lock is already in place!",
					   m_Name, m_ReadOnly ? "read" : "write" ));
		}
		if ( tracker.m_ReadLock != 0 )
		{
			if ( s_IsDebugging )
			{
				BREAKPOINT();
			}

			gperrorf(( "Very bad: recursion in '%s' detected! Attempting to "
					   "write when one or more read locks are already in place!",
					   m_Name ));
		}

		tracker.m_WriteLock = true;
	}
}

RecurseLocker :: ~RecurseLocker( void )
{
	RecurseTracker& tracker = m_Tracker->second;

	if ( m_ReadOnly )
	{
		--tracker.m_ReadLock;
		gpassert( tracker.m_ReadLock >= 0 );
	}
	else
	{
		gpassert( tracker.m_WriteLock );
		tracker.m_WriteLock = false;
	}

	if ( m_Tracker->first != 0 )
	{
		kerneltool::Critical::OptionalLock locker( m_Critical );

		if ( (tracker.m_ReadLock == 0) && !tracker.m_WriteLock )
		{
			m_Db.erase( m_Tracker );
		}
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct MSQAVersion implementation

MSQAVersion :: MSQAVersion( void )
{
	::ZeroObject( *this );
}

bool MSQAVersion :: IsZero( void ) const
{
	return ( *this == MSQAVersion() );
}

bool MSQAVersion :: operator == ( const MSQAVersion& other ) const
{
	return ( CompareObjects( *this, other ) == 0 );
}

// msqa version format  : YY(YY)?.MM.DD.NN (year, month, day, build # of day)

bool FromString( const char* textVersion, MSQAVersion& out )
{
	bool success = false;
	out = MSQAVersion();

	// break it down
	stdx::fast_vector <gpstring> strings;
	stringtool::Extractor( ".", textVersion ).GetStrings( strings );

	// here we optionally process the long form
	if ( strings.size() == 5 )
	{
		out.m_VersionExternal = (WORD) ::atoi( strings[ 0 ].c_str() );
		out.m_VersionInternal = (WORD) ::atoi( strings[ 1 ].c_str() + 1 );	// $ skip bracket
		strings.erase( strings.begin(), strings.begin() + 2 );
	}

	// parse the rest
	if ( strings.size() == 3 )
	{
		out.m_Year       = (WORD)(::atoi( strings[ 0 ].c_str() ));			// $ prefix with century
		out.m_Month      = (WORD) ::atoi( strings[ 1 ].c_str() );
		out.m_Day        = (WORD)(::atoi( strings[ 2 ].c_str() ) / 100);
		out.m_BuildOfDay = (WORD)(::atoi( strings[ 2 ].c_str() ) % 100);

		// if it was 2-digit, tack on millenium
		if ( strings[ 0 ].length() < 4 )
		{
			SYSTEMTIME systemTime;
			::GetLocalTime( &systemTime );
			out.m_Year = (WORD)(out.m_Year + ((systemTime.wYear / 100) * 100));
		}

		success = true;
	}

	return ( success );
}

gpstring ToString( const MSQAVersion& in, bool full )
{
	if ( full )
	{
		return ( gpstringf(
				"%02d.[%02d].%04d.%02d.%02d%02d",
				in.m_VersionExternal,
				in.m_VersionInternal,
				in.m_Year,
				in.m_Month,
				in.m_Day,
				in.m_BuildOfDay ) );
	}
	else
	{
		return ( gpstringf(
				"%02d.%02d.%02d%02d",
				in.m_Year,
				in.m_Month,
				in.m_Day,
				in.m_BuildOfDay ) );
	}
}

bool operator < ( const MSQAVersion& l, const MSQAVersion& r )
{
#	define COMPARE_LESS( x ) if      ( l.x < r.x )  return ( true  ); \
							 else if ( l.x > r.x )  return ( false );

	COMPARE_LESS( m_VersionExternal );
	COMPARE_LESS( m_VersionInternal );
	COMPARE_LESS( m_Year );
	COMPARE_LESS( m_Month );
	COMPARE_LESS( m_Day );
	COMPARE_LESS( m_BuildOfDay );

#	undef COMPARE_LESS

	return ( false );
}

//////////////////////////////////////////////////////////////////////////////
// struct gpversion implementation

gpversion :: gpversion( void )
{
	::ZeroObject( *this );
}

bool gpversion :: InitFromResources( const char* fileName )
{
	// preinit
	*this = gpversion();

	// get at version info
	FileSys::FileVersionInfo fver;
	if ( !fver.Init( fileName ) )
	{
		return ( false );
	}

	// try to get msqa now
	const char* msqa = fver.GetEntry( "MSQAVersion" );
	if ( msqa != NULL )
	{
		MSQAVersion mver;
		if ( ::FromString( msqa, mver ) )
		{
			*this = ::ToGpVersion( mver );
		}
	}

	// get easy stuff
	const VS_FIXEDFILEINFO& vsinfo = fver.GetFixedInfo();
	m_MajorVersion = HIWORD( vsinfo.dwFileVersionMS );
	m_MinorVersion = LOWORD( vsinfo.dwFileVersionMS );
	m_Revision     = HIWORD( vsinfo.dwFileVersionLS );
	m_BuildNumber  = LOWORD( vsinfo.dwFileVersionLS );

	// done
	return ( true );
}

bool gpversion :: IsZero( void ) const
{
	return ( *this == gpversion() );
}

bool gpversion :: IsMSQAOnly( void ) const
{
	return ( !IsZero() && (m_BuildNumber == 0) );
}

bool gpversion :: operator == ( const gpversion& other ) const
{
	return ( CompareObjects( *this, other ) == 0 );
}

bool gpversion :: operator == ( const MSQAVersion& other ) const
{
	return ( ToMSQAVersion( *this ) == other );
}

// normal   version format: "A.BB.C.DDDD" (major, minor, revision, build # to date)
// complete version format: add on "(msqa:AA.BB.CCDD)"

bool FromString( const char* textVersion, gpversion& out )
{
	// first try new format
	out = gpversion();
	int found = ::sscanf( textVersion, "%hd . %hd . %hd . %hd ( msqa : %hd . %hd . %2hd%2hd )",
			&out.m_MajorVersion,
			&out.m_MinorVersion,
			&out.m_Revision,
			&out.m_BuildNumber,
			&out.m_Year,
			&out.m_Month,
			&out.m_Day,
			&out.m_BuildOfDay );
	if ( (found == 8) || (found == 4) )
	{
		return ( true );
	}

	// ok try old format
	MSQAVersion mver;
	if ( ::FromString( textVersion, mver ) )
	{
		out = ::ToGpVersion( mver );
		return ( true );
	}

	// guess it failed
	return ( false );
}

gpstring ToString( const gpversion& in, gpversion::eMode mode )
{
	if ( mode == gpversion::MODE_AUTO_SHORT )
	{
		return ( in.IsMSQAOnly()
				 ? ::ToString( in, gpversion::MODE_MSQA_ONLY_SHORT )
				 : ::ToString( in, gpversion::MODE_SHORT ) );
	}
	else if ( mode == gpversion::MODE_AUTO_LONG )
	{
		return ( in.IsMSQAOnly()
				 ? ::ToString( in, gpversion::MODE_MSQA_ONLY_LONG )
				 : ::ToString( in, gpversion::MODE_LONG ) );
	}
	else if ( mode == gpversion::MODE_COMPLETE )
	{
		return ( gpstringf( "%d.%02d.%d.%04d (msqa:%02d.%02d.%02d%02d)",
				in.m_MajorVersion,
				in.m_MinorVersion,
				in.m_Revision,
				in.m_BuildNumber,
				in.m_Year,
				in.m_Month,
				in.m_Day,
				in.m_BuildOfDay ) );
	}
	else if ( mode == gpversion::MODE_MSQA_ONLY_SHORT )
	{
		return ( ::ToString( ::ToMSQAVersion( in ), false ) );
	}
	else if ( mode == gpversion::MODE_MSQA_ONLY_LONG )
	{
		return ( ::ToString( ::ToMSQAVersion( in ), true ) );
	}
	else
	{
		gpstring out;
		out.appendf( "%d", in.m_MajorVersion );
		out.appendf( (in.m_MinorVersion >= 10) || (in.m_MinorVersion == 0) ? ".%d" : ".%02d", in.m_MinorVersion );
		if ( (mode == gpversion::MODE_LONG) || (in.m_Revision > 0) )
		{
			out.appendf( ".%d", in.m_Revision );
		}
		if ( mode == gpversion::MODE_LONG )
		{
			out.appendf( ".%d", in.m_BuildNumber );
		}
		return ( out );
	}
}

gpstring ToString( const MSQAVersion& in, gpversion::eMode mode )
{
	return ( ::ToString( ::ToGpVersion( in ), mode ) );
}

MSQAVersion ToMSQAVersion( const gpversion& in )
{
	MSQAVersion out;
	out.m_VersionExternal = 0;
	out.m_VersionInternal = 5;
	out.m_Year            = in.m_Year;
	out.m_Month           = in.m_Month;
	out.m_Day             = in.m_Day;
	out.m_BuildOfDay      = in.m_BuildOfDay;

	return ( out );
}

gpversion ToGpVersion( const MSQAVersion& in )
{
	gpversion out;
	out.m_Year       = in.m_Year;
	out.m_Month      = in.m_Month;
	out.m_Day        = in.m_Day;
	out.m_BuildOfDay = in.m_BuildOfDay;
	return ( out );
}

bool operator < ( const gpversion& l, const gpversion& r )
{
	if ( l.IsMSQAOnly() || r.IsMSQAOnly() )
	{
		// use old fashioned comparison method
		return ( ::ToMSQAVersion( l ) < ::ToMSQAVersion( r ) );
	}
	else
	{
		// easy comparison, this is a serial ID so that's all we need
		return ( l.m_BuildNumber < r.m_BuildNumber );
	}
}

//////////////////////////////////////////////////////////////////////////////
// DirectX helper implementations

#pragma comment ( lib, "dxguid.lib" )

// [This code was adapted from the DX samples -Scott]

typedef HRESULT (WINAPI *DIRECTDRAWCREATE) ( GUID*, LPDIRECTDRAW*, IUnknown* );
typedef HRESULT (WINAPI *DIRECTINPUTCREATE)( HINSTANCE, DWORD, LPDIRECTINPUT*, IUnknown* );

DWORD GetDxVersion( void )
{
	// $$ this is fairly old sample code - update it with a dx8 version when
	//    find the time -SB.

	HRESULT                 hr                = DD_OK;
	HINSTANCE               DDHinst           = 0;
	HINSTANCE               DIHinst           = 0;
	LPDIRECTDRAW            pDDraw            = 0;
	LPDIRECTDRAW2           pDDraw2           = 0;
	DIRECTDRAWCREATE        DirectDrawCreate  = 0;
	DIRECTINPUTCREATE       DirectInputCreate = 0;
	OSVERSIONINFO           osVer;
	LPDIRECTDRAWSURFACE     pSurf             = 0;
	LPDIRECTDRAWSURFACE3    pSurf3            = 0;
	LPDIRECTDRAWSURFACE4    pSurf4            = 0;

	/*
	 * First get the windows platform
	 */

	osVer.dwOSVersionInfoSize = sizeof(osVer);
	if (!GetVersionEx(&osVer))
		return 0;

	if (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		/*
		 * NT is easy... NT 4.0 is DX2, 4.0 SP3 is DX3, 5.0 is DX5
		 * and no DX on earlier versions.
		 */
		if (osVer.dwMajorVersion < 4)
			return 0;

		if (osVer.dwMajorVersion == 4)
		{
			/*
			 * NT4 up to SP2 is DX2, and SP3 onwards is DX3, so we are at least DX2
			 */

			/*
			 * We're not supposed to be able to tell which SP we're on, so check for dinput
			 */
			DIHinst = LoadLibrary("DINPUT.DLL");
			if (DIHinst == 0)
			{
				/*
				 * No DInput... must be DX2 on NT 4 pre-SP3
				 */
				gpdebugger("Couldn't LoadLibrary DInput\n");
				return 2;
			}

			DirectInputCreate = (DIRECTINPUTCREATE)GetProcAddress(DIHinst, "DirectInputCreateA");
			FreeLibrary(DIHinst);

			if (DirectInputCreate == 0)
			{
				/*
				 * No DInput... must be pre-SP3 DX2
				 */
				gpdebugger("Couldn't GetProcAddress DInputCreate\n");
				return 2;
			}

			return 3;	//DX3 on NT4 SP3 or higher
		}
		/*
		 * Else it's NT5 or higher, and it's DX5a or higher:
		 */

		/*
		 * Drop through to Win9x tests for a test of DDraw (DX6 or higher)
		 */
	}

	/*
	 * Now we know we are in Windows 9x (or maybe 3.1), so anything's possible.
	 * First see if DDRAW.DLL even exists.
	 */
	DDHinst = LoadLibrary("DDRAW.DLL");
	if (DDHinst == 0)
	{
		FreeLibrary(DDHinst);
		return 0;
	}

	/*
	 *  See if we can create the DirectDraw object.
	 */
	DirectDrawCreate = (DIRECTDRAWCREATE)
		GetProcAddress(DDHinst, "DirectDrawCreate");
	if (DirectDrawCreate == 0)
	{
		FreeLibrary(DDHinst);
		gpdebugger("Couldn't LoadLibrary DDraw\n");
		return 0;
	}

	hr = DirectDrawCreate(NULL, &pDDraw, NULL);
	if (FAILED(hr))
	{
		TRYDX(hr);

		FreeLibrary(DDHinst);
		gpdebugger("Couldn't create DDraw\n");
		return 0;
	}

	/*
	 *  So DirectDraw exists.  We are at least DX1.
	 */

	/*
	 *  Let's see if IID_IDirectDraw2 exists.
	 */
	hr = pDDraw->QueryInterface(IID_IDirectDraw2, (LPVOID *) & pDDraw2);
	if (FAILED(hr))
	{
		/*
		 * No IDirectDraw2 exists... must be DX1
		 */
		pDDraw->Release();
		FreeLibrary(DDHinst);
		gpdebugger("Couldn't QI DDraw2\n");
		return 1;
	}
	/*
	 * IDirectDraw2 exists. We must be at least DX2
	 */
	pDDraw2->Release();

	/*
	 *  See if we can create the DirectInput object.
	 */
	DIHinst = LoadLibrary("DINPUT.DLL");
	if (DIHinst == 0)
	{
		/*
		 * No DInput... must be DX2
		 */
		gpdebugger("Couldn't LoadLibrary DInput\n");
		pDDraw->Release();
		FreeLibrary(DDHinst);
		return 2;
	}

	DirectInputCreate = (DIRECTINPUTCREATE )GetProcAddress(DIHinst, "DirectInputCreateA");
	FreeLibrary(DIHinst);

	if (DirectInputCreate == 0)
	{
		/*
		 * No DInput... must be DX2
		 */
		FreeLibrary(DDHinst);
		pDDraw->Release();
		gpdebugger("Couldn't GetProcAddress DInputCreate\n");
		return 2;
	}

	/*
	 * DirectInputCreate exists. That's enough to tell us that we are at least DX3
	 */

	/*
	 * Checks for 3a vs 3b?
	 */

	/*
	 * We can tell if DX5 is present by checking for the existence of IDirectDrawSurface3.
	 * First we need a surface to QI off of.
	 */
	DDSURFACEDESC desc;

	ZeroObject(desc);
	desc.dwSize = sizeof(desc);
	desc.dwFlags = DDSD_CAPS;
	desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	hr = pDDraw->SetCooperativeLevel(NULL, DDSCL_NORMAL);
	if (FAILED(hr))
	{
		TRYDX(hr);

		/*
		 * Failure. This means DDraw isn't properly installed.
		 */
		pDDraw->Release();
		FreeLibrary(DDHinst);
		gpdebugger("Couldn't Set coop level\n");
		return 0;
	}

	hr = pDDraw->CreateSurface(&desc, &pSurf, NULL);
	if (FAILED(hr))
	{
		TRYDX(hr);

		/*
		 * Failure. This means DDraw isn't properly installed.
		 */
		pDDraw->Release();
		FreeLibrary(DDHinst);
		gpdebugger("Couldn't CreateSurface\n");
		return 0;
	}

	/*
	 * Try for the IDirectDrawSurface3 interface. If it works, we're on DX5 at least
	 */
	if (FAILED(pSurf->QueryInterface(IID_IDirectDrawSurface3, (LPVOID *) & pSurf3)))
	{
		pDDraw->Release();
		FreeLibrary(DDHinst);

		return 3;
	}

	/*
	 * QI for IDirectDrawSurface3 succeeded. We must be at least DX5
	 */

	/*
	 * Try for the IDirectDrawSurface4 interface. If it works, we're on DX6 at least
	 */
	if (FAILED(pSurf->QueryInterface(IID_IDirectDrawSurface4, (LPVOID *) & pSurf4)))
	{
		pDDraw->Release();
		FreeLibrary(DDHinst);

		return 5;
	}

	/*
	 * QI for IDirectDrawSurface4 succeeded. We must be at least DX6
	 */

	pSurf->Release();
	pDDraw->Release();
	FreeLibrary(DDHinst);

	return 6;
}

const char* LookupDxErrorMessage( HRESULT code )
{
	switch ( code )
	{
#		define EE( x    )  case ( x ):  return ( #x );
#		define EM( x, m )  case ( x ):  return ( #x ": " m );

	// General COM/Win32 API errors.

		EM( S_OK                 , "The request completed successfully." )
		EM( E_POINTER            , "Invalid pointer." )
		EM( CLASS_E_NOAGGREGATION, "The object does not support aggregation." )
		EM( CO_E_NOTINITIALIZED  , "An attempt was made to invoke an interface member of a DirectX object created by CoCreateInstance() before it was initialized." )
		EM( E_ACCESSDENIED       , "Access is denied." )
		EM( E_FAIL               , "An undetermined error occurred inside a DirectX subsystem." )
		EM( E_INVALIDARG         , "An invalid parameter was passed to the returning function." )
		EM( E_NOINTERFACE        , "The requested COM interface is not available." )
		EM( E_NOTIMPL            , "The function called is not supported at this time." )
		EM( E_OUTOFMEMORY        , "The DirectX subsystem could not allocate sufficient memory to complete the caller's request." )
		EM( E_PENDING            , "Data is not yet available." )

	// DirectDraw errors.

		EM( DDERR_ALREADYINITIALIZED          , "This object is already initialized." )
		EM( DDERR_BLTFASTCANTCLIP             , "If a clipper object is attached to the source surface passed into a BltFast call." )
		EM( DDERR_CANNOTATTACHSURFACE         , "This surface can not be attached to the requested surface." )
		EM( DDERR_CANNOTDETACHSURFACE         , "This surface can not be detached from the requested surface." )
		EM( DDERR_CANTCREATEDC                , "Windows can not create any more DCs, or a DC was requested for a paltte-indexed surface when the surface had no palette AND the display mode was not palette-indexed (in this case DirectDraw cannot select a proper palette into the DC)." )
		EM( DDERR_CANTDUPLICATE               , "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created." )
		EM( DDERR_CANTLOCKSURFACE             , "Access to this surface is being refused because no driver exists which can supply a pointer to the surface. This is most likely to happen when attempting to lock the primary surface when no DCI provider is present. Will also happen on attempts to lock an optimized surface." )
		EM( DDERR_CANTPAGELOCK                , "The attempt to page lock a surface failed." )
		EM( DDERR_CANTPAGEUNLOCK              , "The attempt to page unlock a surface failed." )
		EM( DDERR_CLIPPERISUSINGHWND          , "An attempt was made to set a clip list for a clipper object that is already monitoring an HWND." )
		EM( DDERR_COLORKEYNOTSET              , "No src color key specified for this operation." )
		EM( DDERR_CURRENTLYNOTAVAIL           , "Support is currently not available." )
		EM( DDERR_D3DNOTINITIALIZED           , "D3D has not yet been initialized." )
		EM( DDERR_DCALREADYCREATED            , "A DC has already been returned for this surface. Only one DC can be retrieved per surface." )
		EM( DDERR_DDSCAPSCOMPLEXREQUIRED      , "The specified surface type requires specification of the COMPLEX flag." )
		EM( DDERR_DEVICEDOESNTOWNSURFACE      , "Surfaces created by one direct draw device cannot be used directly by another DirectDraw device." )
		EM( DDERR_DIRECTDRAWALREADYCREATED    , "A DirectDraw object representing this driver has already been created for this process." )
		EM( DDERR_EXCEPTION                   , "An exception was encountered while performing the requested operation." )
		EM( DDERR_EXCLUSIVEMODEALREADYSET     , "An attempt was made to set the cooperative level when it was already set to exclusive." )
		EM( DDERR_EXPIRED                     , "The data has expired and is therefore no longer valid." )
		EM( DDERR_HEIGHTALIGN                 , "Height of rectangle provided is not a multiple of reqd alignment." )
		EM( DDERR_HWNDALREADYSET              , "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created." )
		EM( DDERR_HWNDSUBCLASSED              , "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state." )
		EM( DDERR_IMPLICITLYCREATED           , "This surface can not be restored because it is an implicitly created surface." )
		EM( DDERR_INCOMPATIBLEPRIMARY         , "Unable to match primary surface creation request with existing primary surface." )
		EM( DDERR_INVALIDCAPS                 , "One or more of the caps bits passed to the callback are incorrect." )
		EM( DDERR_INVALIDCLIPLIST             , "DirectDraw does not support provided Cliplist." )
		EM( DDERR_INVALIDDIRECTDRAWGUID       , "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier." )
		EM( DDERR_INVALIDMODE                 , "DirectDraw does not support the requested mode." )
		EM( DDERR_INVALIDOBJECT               , "DirectDraw received a pointer that was an invalid DIRECTDRAW object." )
		EM( DDERR_INVALIDPIXELFORMAT          , "Pixel format was invalid as specified." )
		EM( DDERR_INVALIDPOSITION             , "Returned when the position of the overlay on the destionation is no longer legal for that destination." )
		EM( DDERR_INVALIDRECT                 , "Rectangle provided was invalid." )
		EM( DDERR_INVALIDSTREAM               , "The specified stream contains invalid data." )
		EM( DDERR_INVALIDSURFACETYPE          , "The requested action could not be performed because the surface was of the wrong type." )
		EM( DDERR_LOCKEDSURFACES              , "Operation could not be carried out because one or more surfaces are locked." )
		EM( DDERR_MOREDATA                    , "There is more data available than the specified buffer size could hold." )
		EM( DDERR_NEWMODE                     , "The mode test has switched to a new mode." )
		EM( DDERR_NO3D                        , "There is no 3D present." )
		EM( DDERR_NOALPHAHW                   , "Operation could not be carried out because there is no alpha accleration hardware present or available." )
		EM( DDERR_NOBLTHW                     , "No blter." )
		EM( DDERR_NOCLIPLIST                  , "No clip list is available." )
		EM( DDERR_NOCLIPPERATTACHED           , "No clipper object attached to surface object." )
		EM( DDERR_NOCOLORCONVHW               , "Operation could not be carried out because there is no color conversion hardware present or available." )
		EM( DDERR_NOCOLORKEY                  , "Surface doesn't currently have a color key." )
		EM( DDERR_NOCOLORKEYHW                , "Operation could not be carried out because there is no hardware support of the dest color key." )
		EM( DDERR_NOCOOPERATIVELEVELSET       , "Create function called without DirectDraw object method SetCooperativeLevel being called." )
		EM( DDERR_NODC                        , "No DC was ever created for this surface." )
		EM( DDERR_NODDROPSHW                  , "No DirectDraw ROP hardware." )
		EM( DDERR_NODIRECTDRAWHW              , "A hardware only DirectDraw object creation was attempted but the driver did not support any hardware." )
		EM( DDERR_NODIRECTDRAWSUPPORT         , "No DirectDraw support possible with current display driver." )
		EM( DDERR_NODRIVERSUPPORT             , "The driver does not enumerate display mode refresh rates." )
		EM( DDERR_NOEMULATION                 , "Software emulation not available." )
		EM( DDERR_NOEXCLUSIVEMODE             , "Operation requires the application to have exclusive mode but the application does not have exclusive mode." )
		EM( DDERR_NOFLIPHW                    , "Flipping visible surfaces is not supported." )
		EM( DDERR_NOFOCUSWINDOW               , "Attempt was made to create or set a device window without first setting the focus window." )
		EM( DDERR_NOGDI                       , "There is no GDI present." )
		EM( DDERR_NOHWND                      , "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND." )
		EM( DDERR_NOMIPMAPHW                  , "Operation could not be carried out because there is no mip-map texture mapping hardware present or available." )
		EM( DDERR_NOMIRRORHW                  , "Operation could not be carried out because there is no hardware present or available." )
		EM( DDERR_NOMONITORINFORMATION        , "The monitor does not have EDID data." )
		EM( DDERR_NONONLOCALVIDMEM            , "An attempt was made to allocate non-local video memory from a device that does not support non-local video memory." )
		EM( DDERR_NOOPTIMIZEHW                , "Device does not support optimized surfaces, therefore no video memory optimized surfaces." )
		EM( DDERR_NOOVERLAYDEST               , "Returned when GetOverlayPosition is called on a overlay that UpdateOverlay has never been called on to establish a destination." )
		EM( DDERR_NOOVERLAYHW                 , "Operation could not be carried out because there is no overlay hardware present or available." )
		EM( DDERR_NOPALETTEATTACHED           , "No palette object attached to this surface." )
		EM( DDERR_NOPALETTEHW                 , "No hardware support for 16 or 256 color palettes." )
		EM( DDERR_NORASTEROPHW                , "Operation could not be carried out because there is no appropriate raster op hardware present or available." )
		EM( DDERR_NOROTATIONHW                , "Operation could not be carried out because there is no rotation hardware present or available." )
		EM( DDERR_NOSTEREOHARDWARE            , "Operation could not be carried out because there is no stereo hardware present or available." )
		EM( DDERR_NOSTRETCHHW                 , "Operation could not be carried out because there is no hardware support for stretching." )
		EM( DDERR_NOSURFACELEFT               , "Operation could not be carried out because there is no hardware present which supports stereo surfaces." )
		EM( DDERR_NOT4BITCOLOR                , "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette." )
		EM( DDERR_NOT4BITCOLORINDEX           , "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette." )
		EM( DDERR_NOT8BITCOLOR                , "DirectDraw Surface is not in 8 bit color mode and the requested operation requires 8 bit color." )
		EM( DDERR_NOTAOVERLAYSURFACE          , "Returned when an overlay member is called for a non-overlay surface." )
		EM( DDERR_NOTEXTUREHW                 , "Operation could not be carried out because there is no texture mapping hardware present or available." )
		EM( DDERR_NOTFLIPPABLE                , "An attempt has been made to flip a surface that is not flippable." )
		EM( DDERR_NOTFOUND                    , "Requested item was not found." )
		EM( DDERR_NOTLOADED                   , "Surface is an optimized surface, but has not yet been allocated any memory." )
		EM( DDERR_NOTLOCKED                   , "Surface was not locked. An attempt to unlock a surface that was not locked at all, or by this process, has been attempted." )
		EM( DDERR_NOTONMIPMAPSUBLEVEL         , "Attempt was made to set a palette on a mipmap sublevel." )
		EM( DDERR_NOTPAGELOCKED               , "An attempt was made to page unlock a surface with no outstanding page locks." )
		EM( DDERR_NOTPALETTIZED               , "The surface being used is not a palette-based surface." )
		EM( DDERR_NOVSYNCHW                   , "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations." )
		EM( DDERR_NOZBUFFERHW                 , "Operation could not be carried out because there is no hardware support for zbuffer blting." )
		EM( DDERR_NOZOVERLAYHW                , "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays." )
		EM( DDERR_OUTOFCAPS                   , "The hardware needed for the requested operation has already been allocated." )
		EM( DDERR_OUTOFVIDEOMEMORY            , "DirectDraw does not have enough memory to perform the operation." )
		EM( DDERR_OVERLAPPINGRECTS            , "Operation could not be carried out because the source and destination rectangles are on the same surface and overlap each other." )
		EM( DDERR_OVERLAYCANTCLIP             , "Hardware does not support clipped overlays." )
		EM( DDERR_OVERLAYCOLORKEYONLYONEACTIVE, "Can only have ony color key active at one time for overlays." )
		EM( DDERR_OVERLAYNOTVISIBLE           , "Returned when GetOverlayPosition is called on a hidden overlay." )
		EM( DDERR_PALETTEBUSY                 , "Access to this palette is being refused because the palette is already locked by another thread." )
		EM( DDERR_PRIMARYSURFACEALREADYEXISTS , "This process already has created a primary surface." )
		EM( DDERR_REGIONTOOSMALL              , "Region passed to Clipper::GetClipList is too small." )
		EM( DDERR_SURFACEALREADYATTACHED      , "This surface is already attached to the surface it is being attached to." )
		EM( DDERR_SURFACEALREADYDEPENDENT     , "This surface is already a dependency of the surface it is being made a dependency of." )
		EM( DDERR_SURFACEBUSY                 , "Access to this surface is being refused because the surface is already locked by another thread." )
		EM( DDERR_SURFACEISOBSCURED           , "Access to Surface refused because Surface is obscured." )
		EM( DDERR_SURFACELOST                 , "Access to this surface is being refused because the surface is gone. The DIRECTDRAWSURFACE object representing this surface should have Restore called on it." )
		EM( DDERR_SURFACENOTATTACHED          , "The requested surface is not attached." )
		EM( DDERR_TESTFINISHED                , "The mode test has finished executing." )
		EM( DDERR_TOOBIGHEIGHT                , "Height requested by DirectDraw is too large." )
		EM( DDERR_TOOBIGSIZE                  , "Size requested by DirectDraw is too large --  The individual height and width are OK." )
		EM( DDERR_TOOBIGWIDTH                 , "Width requested by DirectDraw is too large." )
		EM( DDERR_UNSUPPORTEDFORMAT           , "Pixel format requested is unsupported by DirectDraw." )
		EM( DDERR_UNSUPPORTEDMASK             , "Bitmask in the pixel format requested is unsupported by DirectDraw." )
		EM( DDERR_UNSUPPORTEDMODE             , "The display is currently in an unsupported mode." )
		EM( DDERR_VERTICALBLANKINPROGRESS     , "Vertical blank is in progress." )
		EM( DDERR_VIDEONOTACTIVE              , "The video port is not active." )
		EM( DDERR_WASSTILLDRAWING             , "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete." )
		EM( DDERR_WRONGMODE                   , "This surface can not be restored because it was created in a different mode." )
		EM( DDERR_XALIGN                      , "Rectangle provided was not horizontally aligned on reqd. boundary." )

	// Direct3D errors.

		EE( D3DERR_BADMAJORVERSION            )
		EE( D3DERR_BADMINORVERSION            )
		EE( D3DERR_COLORKEYATTACHED           )
		EE( D3DERR_CONFLICTINGRENDERSTATE     )
		EE( D3DERR_CONFLICTINGTEXTUREFILTER   )
		EE( D3DERR_CONFLICTINGTEXTUREPALETTE  )
		EE( D3DERR_DEVICEAGGREGATED           )
		EE( D3DERR_EXECUTE_CLIPPED_FAILED     )
		EE( D3DERR_EXECUTE_CREATE_FAILED      )
		EE( D3DERR_EXECUTE_DESTROY_FAILED     )
		EE( D3DERR_EXECUTE_FAILED             )
		EE( D3DERR_EXECUTE_LOCK_FAILED        )
		EE( D3DERR_EXECUTE_LOCKED             )
		EE( D3DERR_EXECUTE_NOT_LOCKED         )
		EE( D3DERR_EXECUTE_UNLOCK_FAILED      )
		EE( D3DERR_INBEGIN                    )
		EE( D3DERR_INBEGINSTATEBLOCK          )
		EE( D3DERR_INITFAILED                 )
		EE( D3DERR_INVALID_DEVICE             )
		EE( D3DERR_INVALIDCURRENTVIEWPORT     )
		EE( D3DERR_INVALIDMATRIX              )
		EE( D3DERR_INVALIDPALETTE             )
		EE( D3DERR_INVALIDPRIMITIVETYPE       )
		EE( D3DERR_INVALIDRAMPTEXTURE         )
		EE( D3DERR_INVALIDSTATEBLOCK          )
		EE( D3DERR_INVALIDVERTEXFORMAT        )
		EE( D3DERR_INVALIDVERTEXTYPE          )
		EE( D3DERR_LIGHT_SET_FAILED           )
		EE( D3DERR_LIGHTHASVIEWPORT           )
		EE( D3DERR_LIGHTNOTINTHISVIEWPORT     )
		EE( D3DERR_MATERIAL_CREATE_FAILED     )
		EE( D3DERR_MATERIAL_DESTROY_FAILED    )
		EE( D3DERR_MATERIAL_GETDATA_FAILED    )
		EE( D3DERR_MATERIAL_SETDATA_FAILED    )
		EE( D3DERR_MATRIX_CREATE_FAILED       )
		EE( D3DERR_MATRIX_DESTROY_FAILED      )
		EE( D3DERR_MATRIX_GETDATA_FAILED      )
		EE( D3DERR_MATRIX_SETDATA_FAILED      )
		EE( D3DERR_NOCURRENTVIEWPORT          )
		EE( D3DERR_NOTINBEGIN                 )
		EE( D3DERR_NOTINBEGINSTATEBLOCK       )
		EE( D3DERR_NOVIEWPORTS                )
		EE( D3DERR_SCENE_BEGIN_FAILED         )
		EE( D3DERR_SCENE_END_FAILED           )
		EE( D3DERR_SCENE_IN_SCENE             )
		EE( D3DERR_SCENE_NOT_IN_SCENE         )
		EE( D3DERR_SETVIEWPORTDATA_FAILED     )
		EE( D3DERR_STENCILBUFFER_NOTPRESENT   )
		EE( D3DERR_SURFACENOTINVIDMEM         )
		EE( D3DERR_TEXTURE_BADSIZE            )
		EE( D3DERR_TEXTURE_CREATE_FAILED      )
		EE( D3DERR_TEXTURE_DESTROY_FAILED     )
		EE( D3DERR_TEXTURE_GETSURF_FAILED     )
		EE( D3DERR_TEXTURE_LOAD_FAILED        )
		EE( D3DERR_TEXTURE_LOCK_FAILED        )
		EE( D3DERR_TEXTURE_LOCKED             )
		EE( D3DERR_TEXTURE_NO_SUPPORT         )
		EE( D3DERR_TEXTURE_NOT_LOCKED         )
		EE( D3DERR_TEXTURE_SWAP_FAILED        )
		EE( D3DERR_TEXTURE_UNLOCK_FAILED      )
		EE( D3DERR_TOOMANYOPERATIONS          )
		EE( D3DERR_TOOMANYPRIMITIVES          )
		EE( D3DERR_TOOMANYVERTICES            )
		EE( D3DERR_UNSUPPORTEDALPHAARG        )
		EE( D3DERR_UNSUPPORTEDALPHAOPERATION  )
		EE( D3DERR_UNSUPPORTEDCOLORARG        )
		EE( D3DERR_UNSUPPORTEDCOLOROPERATION  )
		EE( D3DERR_UNSUPPORTEDFACTORVALUE     )
		EE( D3DERR_UNSUPPORTEDTEXTUREFILTER   )
		EE( D3DERR_VBUF_CREATE_FAILED         )
		EE( D3DERR_VERTEXBUFFERLOCKED         )
		EE( D3DERR_VERTEXBUFFEROPTIMIZED      )
		EE( D3DERR_VERTEXBUFFERUNLOCKFAILED   )
		EE( D3DERR_VIEWPORTDATANOTSET         )
		EE( D3DERR_VIEWPORTHASNODEVICE        )
		EE( D3DERR_WRONGTEXTUREFORMAT         )
		EE( D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY   )
		EE( D3DERR_ZBUFF_NEEDS_VIDEOMEMORY    )
		EE( D3DERR_ZBUFFER_NOTPRESENT         )

	// DirectSound errors.

		EM( DSERR_ALLOCATED                   , "The request failed because resources, such as a priority level, were already in use by another caller." )
		EM( DSERR_ALREADYINITIALIZED          , "The object is already initialized." )
		EM( DSERR_BADFORMAT                   , "The specified wave format is not supported." )
		EM( DSERR_BUFFERLOST                  , "The buffer memory has been lost and must be restored." )
		EM( DSERR_CONTROLUNAVAIL              , "The control (volume, pan, and so forth) requested by the caller is not available." )
		EM( DSERR_INVALIDCALL                 , "This function is not valid for the current state of this object." )
		EM( DSERR_NODRIVER                    , "No sound driver is available for use." )
		EM( DSERR_OTHERAPPHASPRIO             , "Another application has a higher priority level, preventing this call from succeeding" )
		EM( DSERR_PRIOLEVELNEEDED             , "The caller does not have the priority level required for the function to succeed." )
		EM( DSERR_UNINITIALIZED               , "The IDirectSound::Initialize method has not been called or has not been called successfully before other methods were called." )

	// DirectPlay 8 errors.

		EE( DPNERR_ABORTED						)
		EE( DPNERR_ADDRESSING					)
		EE( DPNERR_ALREADYCLOSING				)
		EE( DPNERR_ALREADYCONNECTED				)
		EE( DPNERR_ALREADYDISCONNECTING			)
		EE( DPNERR_ALREADYINITIALIZED			)
		EE( DPNERR_ALREADYREGISTERED			)
		EE( DPNERR_BUFFERTOOSMALL				)
		EE( DPNERR_CANNOTCANCEL					)
		EE( DPNERR_CANTCREATEGROUP				)
		EE( DPNERR_CANTCREATEPLAYER				)
		EE( DPNERR_CANTLAUNCHAPPLICATION		)
		EE( DPNERR_CONNECTING					)
		EE( DPNERR_CONNECTIONLOST				)
		EE( DPNERR_CONVERSION					)
		EE( DPNERR_DATATOOLARGE					)
		EE( DPNERR_DOESNOTEXIST					)
		EE( DPNERR_DUPLICATECOMMAND				)
		EE( DPNERR_ENDPOINTNOTRECEIVING			)
		EE( DPNERR_ENUMQUERYTOOLARGE			)
		EE( DPNERR_ENUMRESPONSETOOLARGE			)
		EE( DPNERR_EXCEPTION					)
//		EE( DPNERR_GENERIC						)	//
		EE( DPNERR_GROUPNOTEMPTY				)
		EE( DPNERR_HOSTING                  	)
		EE( DPNERR_HOSTREJECTEDCONNECTION		)
		EE( DPNERR_HOSTTERMINATEDSESSION		)
		EE( DPNERR_INCOMPLETEADDRESS			)
		EE( DPNERR_INVALIDADDRESSFORMAT			)
		EE( DPNERR_INVALIDAPPLICATION			)
		EE( DPNERR_INVALIDCOMMAND				)
		EE( DPNERR_INVALIDDEVICEADDRESS			)
		EE( DPNERR_INVALIDENDPOINT				)
		EE( DPNERR_INVALIDFLAGS					)
		EE( DPNERR_INVALIDGROUP			 		)
		EE( DPNERR_INVALIDHANDLE				)
		EE( DPNERR_INVALIDHOSTADDRESS			)
		EE( DPNERR_INVALIDINSTANCE				)
		EE( DPNERR_INVALIDINTERFACE				)
		EE( DPNERR_INVALIDOBJECT				)
//		EE( DPNERR_INVALIDPARAM					)
		EE( DPNERR_INVALIDPASSWORD				)
		EE( DPNERR_INVALIDPLAYER				)
//		EE( DPNERR_INVALIDPOINTER				)
		EE( DPNERR_INVALIDPRIORITY				)
		EE( DPNERR_INVALIDSTRING				)
		EE( DPNERR_INVALIDURL					)
		EE( DPNERR_INVALIDVERSION				)
		EE( DPNERR_NOCAPS						)
		EE( DPNERR_NOCONNECTION					)
		EE( DPNERR_NOHOSTPLAYER					)
//		EE( DPNERR_NOINTERFACE					)
		EE( DPNERR_NOMOREADDRESSCOMPONENTS		)
		EE( DPNERR_NORESPONSE					)
		EE( DPNERR_NOTALLOWED					)
		EE( DPNERR_NOTHOST						)
		EE( DPNERR_NOTREADY						)
		EE( DPNERR_NOTREGISTERED				)
//		EE( DPNERR_OUTOFMEMORY					)
		EE( DPNERR_PENDING						)
		EE( DPNERR_PLAYERALREADYINGROUP     	)
		EE( DPNERR_PLAYERLOST					)
		EE( DPNERR_PLAYERNOTINGROUP         	)
		EE( DPNERR_PLAYERNOTREACHABLE			)
		EE( DPNERR_SENDTOOLARGE					)
		EE( DPNERR_SESSIONFULL					)
		EE( DPNERR_TABLEFULL					)
		EE( DPNERR_TIMEDOUT						)
		EE( DPNERR_UNINITIALIZED				)
//		EE( DPNERR_UNSUPPORTED					)
		EE( DPNERR_USERCANCEL					)

#		undef EE
#		undef EM

	}	// switch

	return ( NULL );
}

void MakeDxErrorMessage( gpstring& out, HRESULT code, const char* file, int line, bool includeExtra )
{
	if ( includeExtra )
	{
		out.appendf( "DirectX call failure, error = 0x%08X ", code );
	}

	const char* msg = LookupDxErrorMessage( code );
	if ( msg != NULL )
	{
		out.appendf( "('%s')", msg );
	}
	else
	{
		//
		// Ok so we don't have a text version of what's wrong here. Break
		// down the error message so it's easier to figure out.
		//
		// From winerror.h:
		//
		//
		//  HRESULTs are 32 bit values layed out as follows:
		//
		//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
		//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
		//  +-+-+-+-+-+---------------------+-------------------------------+
		//  |S|R|C|N|r|    Facility         |               Code            |
		//  +-+-+-+-+-+---------------------+-------------------------------+
		//
		//  where
		//
		//      S - Severity - indicates success/fail
		//
		//          0 - Success
		//          1 - Fail (COERROR)
		//
		//      R - reserved portion of the facility code, corresponds to NT's
		//              second severity bit.
		//
		//      C - reserved portion of the facility code, corresponds to NT's
		//              C field.
		//
		//      N - reserved portion of the facility code. Used to indicate a
		//              mapped NT status value.
		//
		//      r - reserved portion of the facility code. Reserved for internal
		//              use. Used to indicate HRESULT values that are not status
		//              values, but are instead message ids for display strings.
		//
		//      Facility - is the facility code
		//
		//      Code - is the facility's status code
		//
		// #define MAKE_HRESULT(sev,fac,code) \
		//     ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )
		//
		// From ddraw.h: (same for d3d)
		//
		// #define _FACDD 0x876
		// #define MAKE_DDHRESULT( code ) MAKE_HRESULT( 1, _FACDD, code )
		//
		// From dsound.h:
		//
		// #define _FACDS  0x878
		// #define MAKE_DSHRESULT( code ) MAKE_HRESULT( 1, _FACDS, code )
		//
		// From dplay8.h:
		//
		// #define _DPN_FACILITY_CODE	0x015
		// #define _DPNHRESULT_BASE		0x8000
		// #define MAKE_DPNHRESULT( code )			MAKE_HRESULT( 1, _DPN_FACILITY_CODE, ( code + _DPNHRESULT_BASE ) )
		//

		gpstring facility;
		switch ( HRESULT_FACILITY( code ) )
		{
			case ( _FACDD ):				facility = "DirectDraw" ;  break;
			case ( _DPN_FACILITY_CODE ):	facility = "DirectPlay" ;  break;
			case ( _FACDS ):				facility = "DirectSound";  break;
			default:						facility.assignf( "Unknown (0x%X)", HRESULT_FACILITY( code ) );
		}

		out.appendf( "(unrecognized: severity='%s', facility='%s', code=0x%X)",
					 HRESULT_SEVERITY( code ) ? "Success" : "Fail (COERROR)",
					 facility.c_str(),
					 HRESULT_CODE( code ) );
	}

	// tack on fail/line info
	if ( file != NULL )
	{
		out.appendf( ", file=%s, line=%d", file, line );
	}
}

gpstring MakeDxErrorMessage( HRESULT code, const char* file, int line )
{
	gpstring out;
	MakeDxErrorMessage( out, code, file, line, false );
	return ( out );
}

const char* LookupDPlayMessage( DWORD messageId )
{
	switch ( messageId )
	{
#		define MSG( x )  case ( x ):  return ( #x );

		MSG( DPN_MSGID_ADD_PLAYER_TO_GROUP )
		MSG( DPN_MSGID_APPLICATION_DESC )
		MSG( DPN_MSGID_ASYNC_OP_COMPLETE )
		MSG( DPN_MSGID_CLIENT_INFO )
		MSG( DPN_MSGID_CONNECT_COMPLETE )
		MSG( DPN_MSGID_CREATE_GROUP )
		MSG( DPN_MSGID_CREATE_PLAYER )
		MSG( DPN_MSGID_DESTROY_GROUP )
		MSG( DPN_MSGID_DESTROY_PLAYER )
		MSG( DPN_MSGID_ENUM_HOSTS_QUERY )
		MSG( DPN_MSGID_ENUM_HOSTS_RESPONSE )
		MSG( DPN_MSGID_GROUP_INFO )
		MSG( DPN_MSGID_HOST_MIGRATE )
		MSG( DPN_MSGID_INDICATE_CONNECT )
		MSG( DPN_MSGID_INDICATED_CONNECT_ABORTED )
		MSG( DPN_MSGID_PEER_INFO )
		MSG( DPN_MSGID_RECEIVE )
		MSG( DPN_MSGID_REMOVE_PLAYER_FROM_GROUP )
		MSG( DPN_MSGID_RETURN_BUFFER )
		MSG( DPN_MSGID_SEND_COMPLETE )
		MSG( DPN_MSGID_SERVER_INFO )
		MSG( DPN_MSGID_TERMINATE_SESSION )

#		undef MSG
	}

	return ( NULL );
}

#if GP_RETAIL
HRESULT TryDx( HRESULT result, bool fatalOnFail )
#else
HRESULT TryDx( HRESULT result, const char* file, int line, bool fatalOnFail )
#endif
{
	if ( SUCCEEDED( result ) ||
		(result == DDERR_SURFACELOST ||
		 result == DDERR_SURFACEBUSY ||
		 result == E_OUTOFMEMORY) )
	{
		return ( result );
	}

	gpstring msg;

#	if GP_RETAIL

	// no error context in retail mode, so just bail out
	if ( !fatalOnFail )
	{
		return ( result );
	}
	MakeDxErrorMessage( msg, result );

#	else

	MakeDxErrorMessage( msg, result, file, line );
	ReportSys::SetNextLocation( file, line );

#	endif

	// have to dance here because of recursive assert problems and clearing
	// string buffers and all that. it's a pain to figure out, just trust me
	// that we need this stuff.
	ReportSys::ContextRef ctx;
	if ( fatalOnFail )
	{
		ReportSys::EnableFatalAsWatson();
		ctx = &gFatalContext;
	}
	else if ( AssertData::IsAssertOccurring() )
	{
		ctx = &gWarningContext;
	}
	else
	{
		ctx = &gErrorContext;
	}
	ReportSys::Output( ctx, msg );

	return ( result );
}

//////////////////////////////////////////////////////////////////////////////
// namespace SysInfo implementation

namespace SysInfo  {  // begin of namespace SysInfo

const char* GetComputerName( void )
{
	static bool sQueried = false;
	static char sComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];

	if ( !sQueried )
	{
		sQueried = true;
		DWORD size = ELEMENT_COUNT( sComputerName );
		if ( !::GetComputerName( sComputerName, &size ) )
		{
			::strcpy( sComputerName, "(Unknown Computer)" );
		}
	}

	return ( sComputerName );
}

const char* GetUserName( void )
{
	static bool sQueried = false;
	static char sUserName[ UNLEN + 1 ];

	if ( !sQueried )
	{
		sQueried = true;
		DWORD size = ELEMENT_COUNT( sUserName );
		if ( !::GetUserName( sUserName, &size ) )
		{
			::strcpy( sUserName, "(Unknown User)" );
		}
	}

	return ( sUserName );
}

const SYSTEM_INFO& GetSystemInfo( void )
{
	static bool sQueried = false;
	static SYSTEM_INFO sSystemInfo;

	if ( !sQueried )
	{
		sQueried = true;
		::GetSystemInfo( &sSystemInfo );
	}

	return ( sSystemInfo );
}

const OSVERSIONINFO& GetOsVersionInfo( void )
{
	static bool sQueried = false;
	static OSVERSIONINFO sOsVersionInfo;

	if ( !sQueried )
	{
		sQueried = true;

		sOsVersionInfo.dwOSVersionInfoSize = sizeof( sOsVersionInfo );
		::GetVersionEx( &sOsVersionInfo );
	}

	return ( sOsVersionInfo );
}

const DWORD& GetTotalPhysicalMemory( void )
{
	static bool sQueried = false;
	static DWORD sTotalPhysicalMemory;

	if ( !sQueried )
	{
		sQueried = true;

		MEMORYSTATUS memory;
		memory.dwLength = sizeof( memory );
		::GlobalMemoryStatus(&memory);
		sTotalPhysicalMemory = memory.dwTotalPhys;
	}

	return ( sTotalPhysicalMemory );
}

gpstring GetIPAddress( void )
{
	char				temp[256];
	char				szHostName[256];
	char				string[256];
	WSADATA				wsaData;
	struct hostent FAR *addr;
	char FAR * FAR *	ipa;

	// startup winsock, version 1.1
	WSAStartup( MAKEWORD(1,1), &wsaData	);

	DWORD dwHi, dwLo;
	dwHi = 0;
	dwLo = 0;

	// Get the TCP/IP name of this machine
	gethostname( szHostName, sizeof(szHostName) );
	sprintf( string, "hostname: %s\n", szHostName );

	// Get the TCP/IP address(es) of this machine
	addr = gethostbyname( szHostName ) ;

	// if there is at least one valid address
	if ( addr ) {
		ipa = addr->h_aliases;

		// enumerate through the address aliases (text names)
		while ( *ipa ) {
			sprintf( temp, "Alias: %s\n", (*ipa) );
			strcat( string, temp );
			ipa++;
		}

		string[0]=0;
		ipa=addr->h_addr_list;
		while (*ipa) {
			sprintf(temp, "%d.%d.%d.%d  ",
				(unsigned char)(*ipa)[0],
				(unsigned char)(*ipa)[1],
				(unsigned char)(*ipa)[2],
				(unsigned char)(*ipa)[3]);
			strcat(string, temp);
			ipa++;
		}
	}

	WSACleanup();

	// $$$ remove bordering w/s

	return ( string );
}

double GetCPUSpeedInHertz()
{
	LARGE_INTEGER start_time, end_time, perf_freq, rdtsc_start, rdtsc_end;

	// Query for the frequency of the performance timer
	QueryPerformanceFrequency( &perf_freq );

	// Query for current performance time
	QueryPerformanceCounter( &start_time );
	end_time		= start_time;

	// Read the RDTSC cpu timer
	rdtsc_start		= ReadRDTSCTimer();
	while( (end_time.QuadPart - start_time.QuadPart) < 1000 )
	{
		QueryPerformanceCounter( &end_time );
	}

	// Get the end time
	rdtsc_end		= ReadRDTSCTimer();

	// Calculate processor speed
	return( (double)(rdtsc_end.QuadPart - rdtsc_start.QuadPart) /
					((1.0 / (double)perf_freq.QuadPart) * (double)(end_time.QuadPart - start_time.QuadPart)) );
}

double GetCPUSeconds()
{
	__int64 cycles	= ReadRDTSCTimer().QuadPart;
	static double clockfreq = GetCPUSpeedInHertz();
	double result = cycles / clockfreq;

	return result;
}

bool IsSIMDSupported()
{
	// Detect SIMD support
	DWORD cpuFeatures	= 0;
	__asm
	{
		mov eax, 1
		cpuid
		mov cpuFeatures, edx
	}
	if( cpuFeatures & 0x02000000 )
	{
		return true;
	}

	return false;
}

BOOL AntiHackIsDebuggerPresent( void )
{
	// $ i pulled this little piece of code out of john robbins' second edition
	//   of his debugging book. someone can probably bypass this with a simple
	//   scanner but at least it's better than importing a function to do it,
	//   which is trivial to hack.

	BOOL bRet = TRUE ;
	__asm
	{
		// Get the Thread Information block (TIB).
		MOV       EAX , FS:[00000018H]
		// 0x30 bytes into the TIB is a pointer field that 
		// points to a structure related to debugging.
		MOV       EAX , DWORD PTR [EAX+030H]
		// The second WORD in that debugging structure is the 
		// is being debugged flag.
		MOVZX     EAX , BYTE PTR [EAX+002H]
		// Return the result.
		MOV       bRet , EAX 
	}
	return ( bRet ) ;
}

bool IsDebuggerPresent( void )
{
#	if !GP_RETAIL
	if ( Config::DoesSingletonExist() )
	{
		return ( gConfig.GetBool( "debugger_present", !!::IsDebuggerPresent() ) );
	}
#	endif // !GP_RETAIL

	return ( !!AntiHackIsDebuggerPresent() );
}

static gpversion s_Version;
static bool s_Initialized = false;

const gpversion& GetExeFileVersion( void )
{
	if ( !s_Initialized )
	{
		s_Initialized = s_Version.InitFromResources();
		if ( !s_Initialized )
		{
			s_Version = gpversion();
		}
	}
	return ( s_Version );
}

gpstring GetExeFileVersionText( gpversion::eMode mode )
{
	return ( ::ToString( GetExeFileVersion(), mode ) );
}

gpstring MakeExeSessionInfo( void )
{
	// get buffer
	gpstring info;

	// version info
	info.appendf( "%s version %s\n", COMPILE_MODE_TEXT, GetExeFileVersionText( gpversion::MODE_COMPLETE ).c_str() );

	// time/date
	SYSTEMTIME date;
	::GetLocalTime( &date );
	info.appendf( "execution time [yyyy,mm,dd,hh,mm,ss] = %d:%02d:%02d:%02d:%02d:%02d\n",
				date.wYear,
				date.wMonth,
				date.wDay,
				date.wHour,
				date.wMinute,
				date.wSecond
				);

	// build info
	info.appendf( "build time = [ %s, %s ]\n", __TIME__ , __DATE__ );

	// done
	return ( info );
}

}  // end of namespace SysInfo

//////////////////////////////////////////////////////////////////////////////
