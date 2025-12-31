/*
	Name:	TSDtool
	Date:	07/16/2000

	Desc:	Converts texture detail information into Fuel.

	Usage:
			tsdtool homedir <read, write> filename

			homedir:
			the search path (p:\tattoo_assets\cdimage)

			write: 
			retrieves a listing of all PSD textures in the homedir and converts them into a
			comma-delimitted table.

			read: 
			converts data from the tsd table file back into gas files, one for each texture.

			filename: 
			the table file with tsd information that will be exported/imported.


			table file format:
			texturename, hasalpha, texturedetail, texturefilename

			Fuel format:
			[t:tsd, n:texture_name] {
				hasalpha = false;
				layer1texture1 = texture_name;
				texturedetail = 0;
			}

*/

#include "gpcore.h"

#include "fuel.h"
#include "fueldb.h"
#include "tank.h"
#include "gpmem.h"
#include "filesys.h"
#include "pathfilemgr.h"
#include "masterfilemgr.h"
#include "filesysutils.h"
#include "stringtool.h"

#include <iostream>
#include <wtypes.h>
#include <stdio.h>


#define TABLE_HEADER	"Texturename, HasAlpha, TextureDetail, TextureFilename\n"


using namespace std;
using namespace FileSys;


struct TSDinfo
{
	gpstring		sFilename;
	gpstring		sTexturename;
	bool			bHasAlpha;
	int				texturedetail;
};


// Return a filename without a path or extension
gpstring GetOnlyFilename( const char * pStr )
{
	gpstring sTemp;

	for ( int i = 0; i < strlen( pStr ); ++i ) {
		if ( pStr[i] == '\\' ) {
			sTemp.clear();
		}
		else if ( pStr[i] == '.' ) {
			return sTemp;
		}
		else
		{
			if ( sTemp.size() == 0 ) {
				sTemp = pStr[i];
			}
			else {
				sTemp += pStr[i];
			}
		}
	}
	return sTemp;
}


gpstring GetFuelPathFromFilename( gpstring & filename )
{
	gpstring sTemp;
	const char * pStr = filename.c_str();
	int i, EndOfPath;

	EndOfPath = filename.find_last_of( "\\" );

	for( i = 0; i < EndOfPath; ++i ) {
		if( pStr[i] == '\\' ) {
			sTemp += ':';
		}
		else {
			sTemp += pStr[i];
		}
	}
	return sTemp;
}


// Recurse the root for PSD files.
bool FindPSDFiles( AutoFindHandle & hFindRoot, std::vector< TSDinfo > & TSDtable )
{
	TSDinfo * pTSD;

	FindData TempFindData;

	AutoFindHandle hFindDirectories;

	hFindDirectories.Find( hFindRoot, "*.psd", FINDFILTER_FILES );
	if( hFindDirectories ) {
		while( hFindDirectories.GetNext( TempFindData, true ) ) {
			pTSD = new TSDinfo;
			pTSD->sFilename = gpstring( TempFindData.m_Name.c_str() );
			pTSD->sTexturename = GetOnlyFilename( pTSD->sFilename.c_str() );
			pTSD->bHasAlpha = false;
			pTSD->texturedetail = 0;
			TSDtable.push_back( *pTSD );
		}
	}

	hFindDirectories.Find( hFindRoot, "*", FINDFILTER_DIRECTORIES );
	if( hFindDirectories ) {
		gpstring sDirName;
		while( hFindDirectories.GetNext( sDirName, false ) ) {
			AutoFindHandle hRecurseDir( hFindRoot, ( sDirName + "\\*").c_str() );
			if( hRecurseDir ) {
				FindPSDFiles( hRecurseDir, TSDtable );
			}
		}
	}
	return( true );
}


int main( int argc, char * argv[] )
{
	if( argc != 4 )
	{
		std::cout << "Usage:" << endl;
		std::cout << "tsdtool homedir <read, write> filename.csv" << endl;
		return( 0 );
	}


	std::vector< TSDinfo > TSDtable;	// master TSD table

	gpstring sHomeDir( argv[1] );
	gpstring sOutputFile( argv[3] );

	if( sHomeDir.right( 1 ).compare_no_case( "\\" ) ) {
		sHomeDir.append( "\\" );
	}

	// Initialize FileSys, MasterFileMgr
	std::auto_ptr< FileSys::IFileMgr > mFileMgr;
	FileSys::MasterPathFileMgrBuilder builder;
	if( !builder.SetHomePath( sHomeDir ) ) {
		std::cout << "The location you specified was invalid: " << sHomeDir << endl;
		return( 0 );
	}

	gpstring pathMsg;
	mFileMgr = stdx::make_auto_ptr( builder.CreateMasterFileMgr( &pathMsg ) );

	// initialize old Tank stuff
	stringtool::AppendBackslash( sHomeDir.get_std_string() );
	Tank::GSetRootDirectory( sHomeDir.get_std_string() );

	// initialize fuel
	FuelDB * mFuelDB = new FuelDB;
	mFuelDB->SetIsReadModeRelaxed( true );
	mFuelDB->Init( sHomeDir );

	if( strcmp( argv[2], "write" ) == 0 )
	{
		// Use FileSys to find all the PSD files in the HomeDir
		gpstring sRoot = sHomeDir + "*";
		AutoFindHandle hFindRoot( sRoot.c_str(), FINDFILTER_DIRECTORIES );
		
		std::cout << "Finding *.PSD in " << sHomeDir << "...\n";
		FindPSDFiles( hFindRoot, TSDtable );
		std::cout << TSDtable.size() << " textures found\n\n";

		// Search fuel for existing TSD values
		std::cout << "Searching fuel for TSD entries...\n";
		FuelHandle fhBitmaps( "art:bitmaps" );	
		FuelHandleList fhlBitmaps = fhBitmaps->ListChildBlocks( -1 );
		FuelHandleList::iterator i;

		int TSDcount = 0;
		for( i = fhlBitmaps.begin(); i != fhlBitmaps.end(); ++i ) 
		{
			if( strcmp( (*i)->GetName(), "tsd" ) == 0 ) 
			{
				gpstring gastexturename = GetOnlyFilename( (*i)->GetOriginPath() );

				std::vector< TSDinfo >::iterator idx;
				for( idx = TSDtable.begin(); idx != TSDtable.end(); ++idx )
				{
					if( !gastexturename.compare_no_case( (*idx).sTexturename ) ) {
						(*idx).texturedetail = (*i)->GetInt( "texturedetail", 0 );
						(*idx).bHasAlpha = (*i)->GetBool( "HasAlpha", false );
						++TSDcount;
					}
				}
			}
		}
		std::cout << TSDcount << " TSD entries found\n\n";

		// Write the table file
		std::cout << "Writing table file " << sOutputFile << "...\n\n";

		FILE * pFile = fopen( sOutputFile.c_str(), "wb" );
		char buf[16];

		fwrite( TABLE_HEADER, strlen( TABLE_HEADER ), 1, pFile );

		std::vector< TSDinfo >::iterator idx;
		for( idx = TSDtable.begin(); idx != TSDtable.end(); ++idx ) {
			fwrite( (*idx).sTexturename.c_str(), (*idx).sTexturename.size(), 1, pFile );
			fwrite( ", ", 2, 1, pFile );
			if( (*idx).bHasAlpha ) {
				fwrite( "true", 4, 1, pFile );
			}
			else {
				fwrite( "false", 5, 1, pFile );
			}
			fwrite( ", ", 2, 1, pFile );
			_ltoa( (*idx).texturedetail, buf, 16 );
			fwrite( buf, strlen( buf ), 1, pFile );
			fwrite( ", ", 2, 1, pFile );
			fwrite( (*idx).sFilename.c_str(), (*idx).sFilename.size(), 1, pFile );
			fwrite( "\n", 1, 1, pFile );
		}

		fclose( pFile );
	}
	else if( strcmp( argv[2], "read" ) == 0 )
	{
		// Read TSD entries from the table file
		std::cout << "Reading table file " << sOutputFile << "...\n\n";

		FILE * pFile = fopen( sOutputFile.c_str(), "rb" );

		TSDinfo *pTSD;
		TSDinfo TempTSD;
		gpstring sTemp;
		char cTemp;
		int nField = 0;	// Which comma-delimitted field is being read?

//		table file format:
//		field0,      field1,   field2,        field3
//		texturename, hasalpha, texturedetail, texturefilename

		while( !feof( pFile ) )
		{
			fread( &cTemp, 1, 1, pFile );

			if( cTemp == '\n' ) {
				if( nField == 3 ) {
					if( TempTSD.sTexturename.compare_no_case( "texturename" ) ) {
						pTSD = new TSDinfo;

						pTSD->sTexturename = TempTSD.sTexturename;
						pTSD->bHasAlpha = TempTSD.bHasAlpha;
						pTSD->texturedetail = TempTSD.texturedetail;
						pTSD->sFilename = sTemp;

						TSDtable.push_back( *pTSD );
					}
				}

				TempTSD.sTexturename = "";
				TempTSD.bHasAlpha = false;
				TempTSD.texturedetail = 0;
				TempTSD.sFilename = "";

				nField = 0;
				sTemp = "";
			}
			else if( cTemp == ',' ) {
				if( nField == 0 ) {
					TempTSD.sTexturename = sTemp;
				}
				else if( nField == 1) {
					if( !sTemp.compare_no_case( "true" )) {
						TempTSD.bHasAlpha = true;
					}
					else {
						TempTSD.bHasAlpha = false;
					}
				}
				else if( nField == 2) {
					char *stopscan;
					TempTSD.texturedetail = strtol( sTemp.c_str(), &stopscan, 10 );
				}

				++nField;
				sTemp = "";
			}
			else if( cTemp != ' ' ) {
				sTemp += cTemp;
			}
		}
		
		fclose( pFile );

		// Modify TSD entries in fuel
		std::cout << "Updating TSD entries in Fuel...\n\n";

		FuelDB * mFuelDB = new FuelDB;
		mFuelDB->SetIsReadModeRelaxed( true );
		mFuelDB->Init( sHomeDir );


		std::vector< TSDinfo >::iterator idx;
		for( idx = TSDtable.begin(); idx != TSDtable.end(); ++idx )
		{
			FuelHandle fhRootTSD( GetFuelPathFromFilename( (*idx).sFilename.right( (*idx).sFilename.size() - sHomeDir.size() ) ) );	

			if( fhRootTSD.IsValid() ) 
			{
				FuelHandle fhTSD = fhRootTSD->GetChildBlock( (*idx).sTexturename );
				if( !fhTSD.IsValid() ) {
					fhTSD = fhRootTSD->CreateChildBlock(  (*idx).sTexturename, (*idx).sTexturename + ".gas" );
					fhTSD->SetType( "tsd" );
				}
				if ( fhTSD.IsValid() ) {
					fhTSD->Set( "texturedetail", (*idx).texturedetail );
					fhTSD->Set( "HasAlpha", (*idx).bHasAlpha );
					fhTSD->Set( "Layer1Texture1", (*idx).sTexturename );
				}
			}
		}

		// write fuel
		std::cout << "Writing to Fuel at " << sHomeDir << "...\n\n";
		mFuelDB->SetHomeWritePath( sHomeDir );
		mFuelDB->SaveChanges();
	}

	delete mFuelDB;
	return( 0 );
}


