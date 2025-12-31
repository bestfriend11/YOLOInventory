// SNOConvert.cpp : Defines the entry point for the console application.
//

#include "stdio.h"
#include "assert.h"
#include <vector>

#include "vector_3.h"

#define UINT32		unsigned int
#include "spacenode_io_header.h"

// Shut the fucking linker up
bool _gpassert( bool & ignore_always, const char * expression, char * file, unsigned int line, const char * message )
{
	return true;
}

// Magic fun
static UINT32 const HeaderMagicValue = 0x444f4e53; // "SNOD"

// Forward declaration for key generation function
void GenerateKey( std::vector<vector_3>& meshverts, std::vector<int>& verts, void** keyinfo, int& sizekeyinfo );

// Forward declarations for conversion functions
void ConvertVersion4( siege::SpaceNodeIO_header& header, FILE* pInputfile, FILE* pOutputfile );
void ConvertVersion5( siege::SpaceNodeIO_header& header, FILE* pInputfile, FILE* pOutputfile );

struct doorinfo
{
	UINT32	door_id;
	float	center[3];
	float	x_axis[3];
	float	y_axis[3];
	float	z_axis[3];
	int		numVerts;
	std::vector< int > verts;
};

// Main loop
int main(int argc, char* argv[])
{
	if( argc != 3 ){
		printf( "Correct syntax is:  snoconvert inputfile.sno outputfile.sno\n" );
		return 0;
	}

	FILE* pInputfile	= fopen( argv[1], "rb" );
	if( !pInputfile ){
		// Failed to open the input file
		printf( "Failed to open the input file, cannot continue.\n" );
		return 0;
	}

	if( !stricmp( argv[1], argv[2] ) ){
		printf( "Input and output file cannot be the same!\n" );
		return 0;
	}

	FILE* pOutputfile	= fopen( argv[2], "wb" );
	if( !pOutputfile ){
		// Failed to open the output file
		printf( "Failed to open the output file, cannot continue.\n" );
		fclose( pInputfile );
		return 0;
	}

	// Read in the header
	siege::SpaceNodeIO_header Header;
	fread( &Header, sizeof( siege::SpaceNodeIO_header ), 1, pInputfile );

	// Make sure our magic number is right
	if( Header.MagicValue != HeaderMagicValue ){
		// Bad file
		printf( "This SNO file is either invalid or corrupt!  Cannot continue.\n" );
		fclose( pInputfile );
		fclose( pOutputfile );
		return 0;
	}

	// Convert version
	switch( Header.Version ){

		case 1:
			printf( "This SNO file is too old to be converted.  Please re-export this node from MAX.\n" );
			break;

		case 2:
			printf( "This SNO file is too old to be converted.  Please re-export this node from MAX.\n" );
			break;

		case 3:
			printf( "This SNO file is too old to be converted.  Please re-export this node from MAX.\n" );
			break;

		case 4:
			ConvertVersion4( Header, pInputfile, pOutputfile );
			break;

		case 5:
			ConvertVersion5( Header, pInputfile, pOutputfile );
			break;
	}

	// Close and cleanup
	fclose( pInputfile );
	fclose( pOutputfile );

	return 0;
}

// Version 3 upgrade function
void ConvertVersion4( siege::SpaceNodeIO_header& header, FILE* pInputfile, FILE* pOutputfile )
{
	// Write the header
	header.Version	= 5;
	fwrite( &header, sizeof( siege::SpaceNodeIO_header ), 1, pOutputfile );

	std::vector< doorinfo > doorlist;

	// Go through doors
	for(int unsigned Index = 0; Index < header.NumDoors; ++Index) {

		doorinfo	info;

		info.verts.clear();

		fread( &info.door_id, sizeof( info.door_id ), 1, pInputfile );

		fread( &info.center[0], sizeof( float ), 1, pInputfile );
		fread( &info.center[1], sizeof( float ), 1, pInputfile );
		fread( &info.center[2], sizeof( float ), 1, pInputfile );

		fread( &info.x_axis[0], sizeof( float ), 1, pInputfile );
		fread( &info.x_axis[1], sizeof( float ), 1, pInputfile );
		fread( &info.x_axis[2], sizeof( float ), 1, pInputfile );

		fread( &info.y_axis[0], sizeof( float ), 1, pInputfile );
		fread( &info.y_axis[1], sizeof( float ), 1, pInputfile );
		fread( &info.y_axis[2], sizeof( float ), 1, pInputfile );

		fread( &info.z_axis[0], sizeof( float ), 1, pInputfile );
		fread( &info.z_axis[1], sizeof( float ), 1, pInputfile );
		fread( &info.z_axis[2], sizeof( float ), 1, pInputfile );

		fread( &info.numVerts, sizeof( int ), 1, pInputfile );

		for( int numVerts = info.numVerts; numVerts > 0; --numVerts ){
			int vert;
			fread( &vert, sizeof( int ), 1, pInputfile );
			assert( vert >= 0 );
			info.verts.push_back( vert );
		}

		doorlist.push_back( info );
	}

	// Read in verts
	std::vector< vector_3 > meshverts;
	for( Index = 0; Index < header.NumVertices; ++Index)
	{
		vector_3 Vertex;
		fread( &Vertex(0), sizeof( float ), 1, pInputfile );
		fread( &Vertex(1), sizeof( float ), 1, pInputfile );
		fread( &Vertex(2), sizeof( float ), 1, pInputfile );
		meshverts.push_back( Vertex );
	}

	// Generate and write doors
	for( Index = 0; Index < header.NumDoors; ++Index)
	{
		doorinfo	info	= doorlist[ Index ];

		fwrite( &info.door_id, sizeof( info.door_id ), 1, pOutputfile );

		fwrite( &info.center[0], sizeof( float ), 1, pOutputfile );
		fwrite( &info.center[1], sizeof( float ), 1, pOutputfile );
		fwrite( &info.center[2], sizeof( float ), 1, pOutputfile );

		fwrite( &info.x_axis[0], sizeof( float ), 1, pOutputfile );
		fwrite( &info.x_axis[1], sizeof( float ), 1, pOutputfile );
		fwrite( &info.x_axis[2], sizeof( float ), 1, pOutputfile );

		fwrite( &info.y_axis[0], sizeof( float ), 1, pOutputfile );
		fwrite( &info.y_axis[1], sizeof( float ), 1, pOutputfile );
		fwrite( &info.y_axis[2], sizeof( float ), 1, pOutputfile );

		fwrite( &info.z_axis[0], sizeof( float ), 1, pOutputfile );
		fwrite( &info.z_axis[1], sizeof( float ), 1, pOutputfile );
		fwrite( &info.z_axis[2], sizeof( float ), 1, pOutputfile );

		fwrite( &info.numVerts, sizeof( int ), 1, pOutputfile );

		for( int numVerts = 0; numVerts < info.numVerts; ++numVerts ){
			int vert	= info.verts[ numVerts ];
			fwrite( &vert, sizeof( int ), 1, pOutputfile );
		}

		void*	keyinfo;
		int		sizekeyinfo;

		GenerateKey( meshverts, info.verts, &keyinfo, sizekeyinfo );

		fwrite( keyinfo, sizekeyinfo, 1, pOutputfile );
		free( keyinfo );
	}

	// Write verts
	for( Index = 0; Index < header.NumVertices; ++Index)
	{
		vector_3 Vertex	= meshverts[ Index ];
		fwrite( &Vertex(0), sizeof( float ), 1, pOutputfile );
		fwrite( &Vertex(1), sizeof( float ), 1, pOutputfile );
		fwrite( &Vertex(2), sizeof( float ), 1, pOutputfile );
	}

	// Read and write the rest of the data if there is any
	while( !feof( pInputfile ) )
	{
		char temp;
		if( fread( &temp, 1, 1, pInputfile ) ){
			fwrite( &temp, 1, 1, pOutputfile );
		}
	}
}

// Version 4 upgrade function
void ConvertVersion5( siege::SpaceNodeIO_header& header, FILE* pInputfile, FILE* pOutputfile )
{
	fwrite( &header, sizeof( siege::SpaceNodeIO_header ), 1, pOutputfile );

	// Read and write the rest of the data if there is any
	while( !feof( pInputfile ) )
	{
		char temp;
		if( fread( &temp, 1, 1, pInputfile ) ){
			fwrite( &temp, 1, 1, pOutputfile );
		}
	}
}

/*********************************************************
**		Door hashing functions
*********************************************************/

float RoundToNearestTenth( float num )
{
	int		ipart		= (int)num;
	float	fpart		= num-ipart;

	fpart				*= 10.0f;
	int		fipart		= (int)fpart;
	float	ffpart		= fpart-fipart;

	if( ffpart > 0.4999f ){
		fipart++;
	}

	float ret			= ipart + ((float)fipart/10.0f);
	return ret;
}

void GenerateKey( std::vector<vector_3>& meshverts, std::vector<int>& verts, void** keyinfo, int& sizekeyinfo )
{
	sizekeyinfo		= 4*verts.size();
	*keyinfo		= malloc( sizekeyinfo );

	float *pInfo	= (float*)*keyinfo;

	*pInfo++		= verts.size();

	// Calculate standard length
	float unitl		= Length( meshverts[ verts[ 1 ] ] - meshverts[ verts[ 0 ] ] );

	for( int i = 1; i < verts.size(); ++i, ++pInfo ){
		// Get this length
		float length	= Length( meshverts[ verts[ i ] ] - meshverts[ verts[ 0 ] ] );

		// Calculate our base relativity
		float relative	= (length/unitl);

		// Round it off
		*pInfo			= RoundToNearestTenth( relative );
	}
}
