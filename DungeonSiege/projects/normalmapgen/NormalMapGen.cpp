/***************************************************************************************
**
**							Normal Map Generator
**
**		Generates a normal map from a height map and writes out the result.
**
**	Author:	James Loe
**	Date:	04/03/00
**
***************************************************************************************/

#include "gpcore.h"
#include "gpstring.h"

#include "vector_3.h"

#include "FileFormat_PSD.h"


void HeightVectorToRGBA( const float& theHeight, const vector_3& inVector, DWORD& outColor )
{
	const unsigned int red   = FTOL( min_t( 255.0f, ( ( inVector.x + 1.0f ) * 127.5f  ) ) );
	const unsigned int green = FTOL( min_t( 255.0f, ( ( inVector.y + 1.0f ) * 127.5f ) ) );
	const unsigned int blue  = FTOL( min_t( 255.0f, ( ( inVector.z + 1.0f ) * 127.5f ) ) );
	const unsigned int alpha = FTOL( min_t( 255.0f, theHeight * 255.0f ) );

	outColor = ( ( alpha << 24 ) | ( red << 16 ) | ( green << 8 ) | ( blue << 0 ) );
}

float GetHeight( Channels &chan, const PSDINFOHEADER &header, float u, float v )
{
	if ( u > 1.0f ){ u = 1.0f; }
	if ( u < 0.0f ){ u = 0.0f; }
	if ( v > 1.0f ){ v = 1.0f; }
	if ( v < 0.0f ){ v = 0.0f; }

	float x = max_t( 0.0f, ( u * (header.lColumns-1)  ) );
	float y = max_t( 0.0f, ( v * (header.lRows-1)  ) );

	return (float)( chan.pColor[3][ FTOL( (y * header.lColumns) + x ) ] );
}

// Assumes surface is 32 bit
void GenerateHeightMap( Channels &chan, const PSDINFOHEADER &header )
{
	// To generate each normal, use 3 adjacent alpha values
	// to make a triangle in model space.  Then use the plane
	// equation of the triangle to calculate the normal
	float oneTexelWidth = 1.0f / (header.lColumns-1);
	float oneTexelHeight = 1.0f / (header.lRows-1);

	float theHeight0	= 0.0f;
	float theHeight1	= 0.0f;
	float theHeight2	= 0.0f;
	float theU			= 0.0f;
	float theV			= 0.0f;

	unsigned int index	= 0;
	for ( unsigned int j = 0; j < header.lRows; ++j  )
	{
		theU	= 0.0f;
		for ( unsigned int i = 0; i < header.lColumns; ++i, ++index )
		{
			// Get the height by reading the alpha component at the corresponding pixel
			theHeight0 = GetHeight( chan, header, theU, theV );
			theHeight1 = GetHeight( chan, header, theU + oneTexelWidth, theV );
			theHeight2 = GetHeight( chan, header, theU, theV + oneTexelHeight );

			// Generate 3D coordinates for the normal math
			vector_3 thePoint0( theU,					theHeight0,	theV					);
			vector_3 thePoint1( theU + oneTexelWidth,	theHeight1, theV					);
			vector_3 thePoint2( theU,					theHeight2, theV + oneTexelHeight	);

			// Generate the Normal
			vector_3 theNormal = CrossProduct( (thePoint1 - thePoint0), (thePoint2 - thePoint0) );
			theNormal.Normalize();
			theNormal.y = ( theHeight0 / 255.0f );
			theNormal *= 0.5f;

			// Convert the normal and height information to a color that we can use
			unsigned long color;
			HeightVectorToRGBA( theHeight0, theNormal, color );

			// Set the new color
			chan.pColor[0][index]	= color & 0x000000FF;
			chan.pColor[1][index]	= (color & 0x0000FF00) >> 8;
			chan.pColor[2][index]	= (color & 0x00FF0000) >> 16;
			chan.pColor[3][index]	= (color & 0xFF000000) >> 24;

			theU	+= oneTexelWidth;
		}
		theV	+= oneTexelHeight;
	}
}

void BigEndianToLittleEndian16( short *sValue )
{
	BYTE one	= (BYTE)(*sValue);
	BYTE two	= (BYTE)(*sValue>>8);

	*sValue		= (short)(two | (one<<8));		
}

void BigEndianToLittleEndian32( long *sValue )
{
	BYTE one	= (BYTE)(*sValue);
	BYTE two	= (BYTE)(*sValue>>8);
	BYTE three	= (BYTE)(*sValue>>16);
	BYTE four	= (BYTE)(*sValue>>24);

	*sValue		= four | (three<<8) | (two<<16) | (one<<24);
}

void ConvertPSD(	const void *pBuffer, FILE* to, UINT32 width, UINT32 height, 
					UINT32& outSize, const PSDINFOHEADER& psdInfo ) 
{
    // We'll use this variable to get the length of variable sized chunks
	// within our psd.
	long	lLength;
	UINT8	*pBits = (UINT8 *)pBuffer;
	outSize = width * height * 4;

	// Make sure we have bits to utilize
	gpassert( pBits != 0 );
	
	// First, we want to pass by the Color mode information
	lLength	=	*(const long*)pBits;
	fwrite( pBits, sizeof( long ), 1, to );
	pBits	+=	sizeof( long );
	BigEndianToLittleEndian32( &lLength );
	fwrite( pBits, lLength, 1, to );
	pBits	+=	lLength;

	// Now pass by the Image Resource information
	lLength	=	*(const long*)pBits;
	fwrite( pBits, sizeof( long ), 1, to );
	pBits	+=	sizeof( long );
	BigEndianToLittleEndian32( &lLength );
	fwrite( pBits, lLength, 1, to );
	pBits	+=	lLength;

	// Now pass by the Layer and Mask information
	lLength	=	*(const long*)pBits;
	fwrite( pBits, sizeof( long ), 1, to );
	pBits	+=	sizeof( long );
	BigEndianToLittleEndian32( &lLength );
	fwrite( pBits, lLength, 1, to );
	pBits	+=	lLength;

	// Now lets extract the compression information
	// Note: 0 = Raw, 1 = RLE compression
	short write_comp = 0;
	fwrite( &write_comp, sizeof( short ), 1, to );

	short sCompression = *(const short *)pBits;
	pBits	+=	sizeof( short );
	BigEndianToLittleEndian16( &sCompression );

	// Now we have the bits in a planar order ( RRR GGG BBB AAA, etc. )
	// Lets build our pixel map
	
	int chn[4]		=	{2,1,0,3};
	int PixelCount	=	width * height;

	Channels chan;
	memset( &chan, 0, sizeof( Channels ) );
				
	// Load in RLE compressed image data
	if( sCompression == 1 )
	{
		pBits += (height) * psdInfo.sChannels * 2;

		for ( int c = 0; c < 4; c++ )
		{			
			int pn = 0;
			int channel = chn[c];

			// Allocate holder array
			chan.pColor[ channel ] = new unsigned char[ PixelCount ];

			if( channel >= psdInfo.sChannels )
			{
				memset( chan.pColor[ channel ], 255, PixelCount );
			}
			else
			{
				int count = 0; 
				while( count < PixelCount )
				{
					int length	= *pBits;
					pBits		++;
					if( length < 128 )
					{
						length				++;
						count				+= length;

						memcpy( &chan.pColor[ channel ][ pn ], pBits, length );

						pBits				+= length;
						pn					+= length;
					}
					else if( length > 128 )
					{
						length				^= 0x0FF;
						length				+= 2;
						count				+= length;

						memset( &chan.pColor[ channel ][ pn ], *pBits, length );

						pBits				++;
						pn					+= length;
					}
				}
			}			
		}
	}
	
	// This loads in non-RLE compressed PSD's	
	else
	{
		// There are 4 channels that we have to deal with, so we go through them in order
		for( int c = 0; c < 4; ++c )
		{
			int channel	= chn[c];

			chan.pColor[ channel ] = new unsigned char[ PixelCount ];
			if( channel < psdInfo.sChannels )
			{
				memcpy( chan.pColor[ channel ], pBits, PixelCount );
				pBits	+= PixelCount;
			}
			else
			{
				memset( chan.pColor[ channel ], 255, PixelCount );
			}
		}
	}

	GenerateHeightMap( chan, psdInfo );

	for( int c = 0; c < 4; ++c )
	{
		if( chn[c] < psdInfo.sChannels )
		{
			fwrite( chan.pColor[ chn[c] ], PixelCount, 1, to );
		}
	}

	delete[] chan.pColor[0];
	delete[] chan.pColor[1];
	delete[] chan.pColor[2];
	delete[] chan.pColor[3];
}


int main(int argc, char* argv[])
{
	gpassert( argc == 2 );

	// Get the file from the passed in arguments
	gpstring input_file	= argv[1];

	// Split up the path
	char suffix[8];
	_splitpath( input_file, NULL, NULL, NULL, suffix );
	if( !strnicmp( suffix, ".psd", 4 ) )
	{
		// Open the file
		FILE* from				= fopen( input_file, "rb" );

		// Open a file to write to
		gpstring output_file	= input_file.left( input_file.get_std_string().length() - 4 );
		output_file				+= "_nmap.psd";
		FILE* to				= fopen( output_file, "wb" );

		// Read the header
		PSDINFOHEADER header;
		fread( &header, sizeof( PSDINFOHEADER ), 1, from );
		
		// Write the header
		fwrite( &header, sizeof( PSDINFOHEADER ), 1, to );

		// Convert the header
		BigEndianToLittleEndian32( &header.lSignature );
		BigEndianToLittleEndian16( &header.sVersion );
		BigEndianToLittleEndian16( &header.sChannels );
		BigEndianToLittleEndian32( &header.lRows );
		BigEndianToLittleEndian32( &header.lColumns );
		BigEndianToLittleEndian16( &header.sDepth );
		BigEndianToLittleEndian16( &header.sMode );

		// The buffer of pixels
		unsigned int rgbDataSize	= header.lColumns * header.lRows * 4;
		char* data	= new char[ rgbDataSize ];
		fread( data, rgbDataSize, 1, from );

		// Convert the PSD to a normal map
		ConvertPSD( data, to, header.lColumns, header.lRows, rgbDataSize, header );

		// Close the files
		fclose( to );
		fclose( from );
	}
	else
	{
		printf( "NormalMapGen can only convert PSD's!" );
	}

	return 0;
}
