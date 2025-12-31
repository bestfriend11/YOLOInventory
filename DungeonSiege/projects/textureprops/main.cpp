/*
	Name:		TextureProps
	Date:		7/25/2000
	Author:		Adam Swensen

	Desc:		TSD property conversion from Fuel into an external spreadsheet.
*/


#include <windows.h>
#include <commctrl.h>
#include <wtypes.h>
#include <stdio.h>

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
#include "config.h"
#include "winx.h"
//#include "FileFormat_PSD.h"

#include "resource.h"


using namespace std;
using namespace FileSys;


// TSD / PSD defs
#define TSD_HEADER_TEXTURE		"<Texture>"
#define TSD_HEADER_FILENAME		"<TextureFilename>"
#define TSD_HEADER_DIMENSIONS	"# Dimensions"

typedef std::list< gpstring > StringColl;

struct TSD
{
	gpstring		Texturename;
	gpstring		Filename;
	StringColl		TsdValues;
};

typedef std::vector< TSD > TSDColl;

TSDColl			TsdEntries;

#	pragma pack ( push, 1 )

struct PSDINFOHEADER
{
	FOURCC m_Signature;			// must be '8BPS'
	WORD   m_Version;			// must be 1
	BYTE   m_Reserved[ 6 ];		// ignore (but zero-fill)
	WORD   m_Channels;			// number of channels in the image, including alphas (from 1-24)
	DWORD  m_Height;			// height of image in pixels (from 1-30000)
	DWORD  m_Width;				// width of image in pixels (from 1-30000)
	WORD   m_BitsPerChannel;	// number of bits per channel (1, 8, or 16)
	WORD   m_Mode;				// color mode of file (from 0-9)

	PSDINFOHEADER()
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

// Forward decl
BOOL CALLBACK TextureDialogProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
void EnableDialogControls( BOOL bState );
void GetPSDInfo( const char *fname, PSDINFOHEADER & psdInfo );
gpstring GetOnlyFilename( const char * pStr );
gpstring GetFuelPathFromFilename( gpstring & filename );
bool FindPSDFiles( AutoFindHandle & hFindRoot );
void ExportTableFile();
void ImportTableFile();
void LoadSettings( HWND hDlg );
void SaveSettings( HWND hDlg );
gpstring GetOnlyPath( const char * pStr );



// Global decl
HWND		hDlg;
HWND		hwndProgressBar;


int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow )
{
    MSG			msg;
	Config		MyConfig( COMPANY_NAME, "TextureProps" );

	InitCommonControls();

	hDlg = CreateDialog( hInstance, MAKEINTRESOURCE( IDD_TEXTUREPROPS ), NULL, TextureDialogProc );
	gpassertm( hDlg, "Failed to create application dialog." );
	ShowWindow( hDlg, iCmdShow );
 
    while( GetMessage( &msg, NULL, 0, 0 ) ) 
	{
		TranslateMessage( &msg );
		DispatchMessage ( &msg );		
    }

    return( msg.wParam );
}


BOOL CALLBACK TextureDialogProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_INITDIALOG:
			LoadSettings( hDlg );
			break;
		case WM_COMMAND:
			switch( LOWORD( wParam ) )
			{
				case IDC_EXPORT:
					ExportTableFile();
					break;
				case IDC_IMPORT:
					ImportTableFile();
					break;
				case IDC_OPENEXCEL:
					ShellExecute( hDlg, "open", "excel", winx::GetDlgItemText( hDlg, IDC_FILENAME ), GetOnlyPath( winx::GetDlgItemText( hDlg, IDC_FILENAME ) ), SW_SHOW );
					break;
				case IDC_CLOSE:
					SendMessage( hDlg, WM_CLOSE, 0, 0 );
					break;
			}
			break;
		case WM_CLOSE:
		case WM_DESTROY:
			SaveSettings( hDlg );
			PostQuitMessage( 0 );
			break;
	}

	return( FALSE );
}


void LoadSettings( HWND hDlg )
{
	gpstring sTemp;

	if( gConfig.ReadKeyValue( "HomePath", sTemp ) )
	{
		SetDlgItemText( hDlg, IDC_HOMEPATH, sTemp.c_str() );
	}
	else
	{
		SetDlgItemText( hDlg, IDC_HOMEPATH, "p:\\tattoo_assets\\cdimage" );
	}
	
	if( gConfig.ReadKeyValue( "DefaultFilename", sTemp ) )
	{
		SetDlgItemText( hDlg, IDC_FILENAME, sTemp.c_str() );
	}
	else
	{
		SetDlgItemText( hDlg, IDC_FILENAME, "c:\\temp\\tsdtable.csv" );
	}
	
	if( gConfig.ReadKeyValue( "FuelRoot", sTemp ) )
	{
		SetDlgItemText( hDlg, IDC_FUELROOT, sTemp.c_str() );
	}
	else
	{
		SetDlgItemText( hDlg, IDC_FUELROOT, "art:bitmaps" );
	}

	if( gConfig.ReadKeyValue( "TsdVals", sTemp ) )
	{
		SetDlgItemText( hDlg, IDC_TSDVALS, sTemp.c_str() );
	}
	else
	{
		SetDlgItemText( hDlg, IDC_TSDVALS, "hasalpha, texturedetail" );
	}
	
	if( gConfig.ReadKeyValue( "ExportDims", sTemp ) )
	{
		if( sTemp.compare_no_case( "true" ) == 0 )
		{
			HWND hwnd = GetDlgItem( hDlg, IDC_TEXTUREDIMS );
			SendMessage( hwnd, BM_SETCHECK, BST_CHECKED, 0 );
		}
	}
}


void SaveSettings( HWND hDlg )
{
	gpstring sTemp;

	sTemp = winx::GetDlgItemText( hDlg, IDC_HOMEPATH );
	gConfig.SetKeyValue( "HomePath", sTemp );

	sTemp = winx::GetDlgItemText( hDlg, IDC_FILENAME );
	gConfig.SetKeyValue( "DefaultFilename", sTemp );
	
	sTemp = winx::GetDlgItemText( hDlg, IDC_FUELROOT );
	gConfig.SetKeyValue( "FuelRoot", sTemp );

	sTemp = winx::GetDlgItemText( hDlg, IDC_TSDVALS );
	gConfig.SetKeyValue( "TsdVals", sTemp );

	HWND hwnd = GetDlgItem( hDlg, IDC_TEXTUREDIMS );
	if ( SendMessage( hwnd, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
	{
		gConfig.SetKeyValue( "ExportDims", "true" );
	}
	else
	{
		gConfig.SetKeyValue( "ExportDims", "false" );
	}
}


void EnableDialogControls( BOOL bState )
{
	HWND hCtl;

	hCtl = GetDlgItem( hDlg, IDC_EXPORT );
	EnableWindow( hCtl, bState );
	hCtl = GetDlgItem( hDlg, IDC_IMPORT );
	EnableWindow( hCtl, bState );
	hCtl = GetDlgItem( hDlg, IDC_CLOSE );
	EnableWindow( hCtl, bState );
}


gpstring GetOnlyPath( const char * pStr )
{
	gpstring sTemp = pStr;
	int index = sTemp.find_last_of( "\\" );

	if ( index != gpstring::npos )
	{
		sTemp = sTemp.left( index );
	}

	return( sTemp );
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


// Read header from a PSD
void GetPSDInfo( const char *fname, PSDINFOHEADER & psdInfo )
{
	FILE *pFile = fopen( fname, "rb" );
	fread( (void *) &psdInfo, sizeof( PSDINFOHEADER ), 1, pFile );
	fclose( pFile );

	psdInfo.ByteSwap();
}


// Return a filename without a path or extension
gpstring GetOnlyFilename( const char * pStr )
{
	gpstring sTemp;

	for ( int i = 0; i < strlen( pStr ); ++i ) 
	{
		if ( pStr[i] == '\\' ) 
		{
			sTemp.clear();
		}
		else if ( pStr[i] == '.' ) 
		{
			return sTemp;
		}
		else
		{
			if ( sTemp.size() == 0 )
			{
				sTemp = pStr[i];
			}
			else
			{
				sTemp += pStr[i];
			}
		}
	}

	return( sTemp );
}


gpstring GetFuelPathFromFilename( gpstring & filename )
{
	gpstring sTemp;
	const char * pStr = filename.c_str();
	int i, EndOfPath;

	EndOfPath = filename.find_last_of( "\\" );

	for( i = 0; i < EndOfPath; ++i ) 
	{
		if( pStr[i] == '\\' ) 
		{
			sTemp += ':';
		}
		else 
		{
			sTemp += pStr[i];
		}
	}

	return( sTemp );
}


// Recurse the root for PSD files.
bool FindPSDFiles( AutoFindHandle & hFindRoot )
{
	FindData TempFindData;
	AutoFindHandle hFindDirectories;
	TSD tsd;

	hFindDirectories.Find( hFindRoot, "*.psd", FINDFILTER_FILES );
	if( hFindDirectories ) 
	{
		while( hFindDirectories.GetNext( TempFindData, true ) ) 
		{
			tsd.Filename = gpstring( TempFindData.m_Name.c_str() );
			tsd.Texturename = GetOnlyFilename( TempFindData.m_Name.c_str() );
			TsdEntries.push_back( tsd );
		}
	}

	hFindDirectories.Find( hFindRoot, "*", FINDFILTER_DIRECTORIES );
	if( hFindDirectories ) 
	{
		gpstring sDirName;
		while( hFindDirectories.GetNext( sDirName, false ) ) 
		{
			AutoFindHandle hRecurseDir( hFindRoot, ( sDirName + "\\*").c_str() );
			if( hRecurseDir ) 
			{
				FindPSDFiles( hRecurseDir );
			}
		}
	}

	return( true );
}


void ImportTableFile()
{
	StringColl	TsdHeader;

	// Get dialog settings
	gpstring sHomeDir = winx::GetDlgItemText( hDlg, IDC_HOMEPATH );
	gpstring sFilename = winx::GetDlgItemText( hDlg, IDC_FILENAME );
	gpstring sFuelRoot = winx::GetDlgItemText( hDlg, IDC_FUELROOT );
	stringtool::AppendTrailingBackslash( sHomeDir );

	EnableDialogControls( FALSE );
	hwndProgressBar = GetDlgItem( hDlg, IDC_PROGRESS );

	// Initialize file manager for fuel
	FileSys::MasterFileMgr * pMasterFileMgr	= new MasterFileMgr();
	FileSys::PathFileMgr * pPathFileMgr		= new PathFileMgr( sHomeDir );
	pMasterFileMgr->AddFileMgr( pPathFileMgr, true );

	// Create our fuel database
	FuelSys * pFuelSys = new FuelSys;
	TextFuelDB * pFuelDB = pFuelSys->AddTextDb( "default" );
	pFuelDB->Init( FuelDB::OPTION_JIT_READ | FuelDB::OPTION_WRITE, sHomeDir, sHomeDir );

	// Read TSD entries from the table file
	SetDlgItemText( hDlg, IDC_STATUS, "Reading table file..." );

	FILE * pFile = fopen( sFilename.c_str(), "rb" );
	if( !pFile )
	{
		MessageBox( NULL, "The file you specified could not be opened.", "Error", MB_OK | MB_ICONEXCLAMATION );

		delete pFuelSys;
		delete pMasterFileMgr;

		SetDlgItemText( hDlg, IDC_STATUS, "Ready" );
		SendMessage( hwndProgressBar, PBM_SETPOS, (WPARAM) 0, 0 );
		EnableDialogControls( TRUE );
		return;
	}

	TSD tsd;
	gpstring sBuf;
	char cBuf;
	int nField = 0;	// Which comma-delimitted field is being read?
	std::vector< gpstring * > TsdFields;
	int nValueCount = 0;

	TsdHeader.clear();
	TsdEntries.clear();

	while( !feof( pFile ) )
	{
		fread( &cBuf, 1, 1, pFile );

		if( ( cBuf == '\n' ) || ( cBuf == '\r' ) || ( cBuf == ',' ) )
		{
			if( sBuf.left( 1 ).same_no_case( "<" ) )
			{
				if( TsdFields.size() <= nField )
				{
					TsdFields.resize( nField + 1, &sBuf );
				}

				if( sBuf.same_no_case( TSD_HEADER_TEXTURE ) )
				{
					TsdFields[nField] = &tsd.Texturename;
				}
				else if( sBuf.same_no_case( TSD_HEADER_FILENAME ) )
				{
					TsdFields[nField] = &tsd.Filename;
				}
				else 
				{
					if( TsdFields[nField] == &sBuf )
					{
						TsdHeader.push_back( sBuf.right( sBuf.size() - 1 ).left( sBuf.size() - 2 ) );
						tsd.TsdValues.resize( nValueCount + 1 );
						TsdFields[nField] = &tsd.TsdValues.back();
						++nValueCount;
					}
				}
			}
			else if( !sBuf.left( 1 ).same_no_case( "#" ) )
			{
				if( ( nField < TsdFields.size() ) && ( sBuf.size() > 0 ) )
				{
					sBuf.to_lower();
					*TsdFields[nField] = sBuf;
				}
			}

			sBuf.clear();
			++nField;
		}
		else if( cBuf != ' ' )
		{
			sBuf += cBuf;
		}

		if( ( cBuf == '\n' ) || ( cBuf == '\r' ) )
		{
			if( ( tsd.Filename.size() > 0 ) )
			{
				TsdEntries.push_back( tsd );
			}

			tsd.Filename.clear();
			tsd.Texturename.clear();
			for( StringColl::iterator i = tsd.TsdValues.begin(); i != tsd.TsdValues.end(); ++i )
			{
				(*i).clear();
			}

			sBuf.clear();
			nField = 0;
		}
	}
	
	fclose( pFile );

	// Modify TSD entries in fuel
	gpstring sKey, sValue;
	SetDlgItemText( hDlg, IDC_STATUS, "Updating tsd entries in Fuel..." );
    SendMessage( hwndProgressBar, PBM_SETRANGE, 0, MAKELPARAM( 0, TsdEntries.size() + 1 ) ); 
    SendMessage( hwndProgressBar, PBM_SETSTEP, (WPARAM) 1, 0 ); 

	TSDColl::iterator idx;
	for( idx = TsdEntries.begin(); idx != TsdEntries.end(); ++idx )
	{
		FuelHandle fhRootTSD( GetFuelPathFromFilename( (*idx).Filename.right( (*idx).Filename.size() - sHomeDir.size() ) ) );	

		if( fhRootTSD.IsValid() ) 
		{
			FuelHandle fhTSD = fhRootTSD->GetChildBlock( (*idx).Texturename );
			if( !fhTSD.IsValid() )
			{
				// Only make tsd entries if there are settings to add
				bool bFoundValues = false;
				StringColl::iterator j = (*idx).TsdValues.begin();

				for( StringColl::iterator i = TsdHeader.begin(); i != TsdHeader.end(); ++i, ++j )
				{
					if( ( (*i).size() > 0 ) && ( (*j).size() > 0 ) )
					{
						bFoundValues = true;
						break;
					}
				}
				if( bFoundValues )
				{
					fhTSD = fhRootTSD->CreateChildBlock(  (*idx).Texturename, (*idx).Texturename + ".gas" );
					fhTSD->SetType( "tsd" );
				}
			}
			if( fhTSD.IsValid() )
			{
				fhTSD->Set( "layer1texture1", (*idx).Texturename );

				StringColl::iterator j = (*idx).TsdValues.begin();
				
				for( StringColl::iterator i = TsdHeader.begin(); i != TsdHeader.end(); ++i, ++j )
				{
					if( (*i).size() > 0 )
					{
						if( (*j).size() > 0 )
						{
							fhTSD->Set( (*i), (*j) );
						}
						else
						{
							fhTSD->Remove( (*i) );
						}
					}
				}
				
				// Check to see if this entry is empty, and can be deleted
				int nKeyCount = 0;
				if ( fhTSD->FindFirstKeyAndValue() )
				{
					while( fhTSD->GetNextKeyAndValue( sKey, sValue ) )
					{
						++nKeyCount;
					}
				}
				if( (nKeyCount == 1) && sKey.same_no_case( "layer1texture1" ) )
				{
					fhTSD->GetParentBlock()->DestroyChildBlock( fhTSD );
					continue;
				}
			}

			// Convert all tsd values into type formats
			if ( fhTSD )
			{
				gpstring value;

				int numLayers = 0;
				if ( fhTSD->Get( "numlayers", numLayers ) )
				{
					fhTSD->Set( "numlayers", gpstringf( "%d", numLayers ), FVP_NORMAL_INT );
				}

				if ( fhTSD->Get( "timesyncanimation", value ) )
				{
					fhTSD->Set( "timesyncanimation", value, FVP_BOOL );
				}
				if ( fhTSD->Get( "hasalpha", value ) )
				{
					fhTSD->Set( "hasalpha", value, FVP_BOOL );
				}
				if ( fhTSD->Get( "alphaformat", value ) )
				{
					fhTSD->Set( "alphaformat", value, FVP_BOOL );
				}
				if ( fhTSD->Get( "dither", value ) )
				{
					fhTSD->Set( "dither", value, FVP_BOOL );
				}
				if ( fhTSD->Get( "startanimation", value ) )
				{
					fhTSD->Set( "startanimation", value, FVP_BOOL );
				}
				if ( fhTSD->Get( "texturedetail", value ) )
				{
					fhTSD->Set( "texturedetail", value, FVP_NORMAL_INT );
				}
				if ( fhTSD->Get( "multiinstance", value ) )
				{
					fhTSD->Set( "multiinstance", value, FVP_BOOL );
				}

				char key[256];
				for ( int layer = 0; layer < numLayers; ++layer )
				{
					int numFrames = 0;
					sprintf( key, "layer%dnumframes", layer + 1 );
					if ( !fhTSD->Get( key, numFrames ) )
					{
						continue;
					}
					fhTSD->Set( key, gpstringf( "%d", numFrames ), FVP_NORMAL_INT );

					for ( int frame = 0; frame < numFrames; ++frame )
					{
						sprintf( key, "layer%dtexture%d", layer + 1, frame + 1 );
						if ( fhTSD->Get( key, value ) )
						{
							fhTSD->Set( key, value, FVP_STRING );
						}
					}

					sprintf( key, "layer%dsecondsperframe", layer+1 );
					if ( fhTSD->Get( key, value ) )
					{
						fhTSD->Set( key, value, FVP_FLOAT );
					}

					sprintf( key, "layer%dushiftpersecond", layer+1 );
					if ( fhTSD->Get( key, value ) )
					{
						fhTSD->Set( key, value, FVP_FLOAT );
					}

					sprintf( key, "layer%dvshiftpersecond", layer+1 );
					if ( fhTSD->Get( key, value ) )
					{
						fhTSD->Set( key, value, FVP_FLOAT );
					}

					sprintf( key, "layer%drotationpersecond", layer+1 );
					if ( fhTSD->Get( key, value ) )
					{
						fhTSD->Set( key, value, FVP_FLOAT );
					}

					sprintf( key, "layer%drotationucenter", layer+1 );
					if ( fhTSD->Get( key, value ) )
					{
						fhTSD->Set( key, value, FVP_FLOAT );
					}

					sprintf( key, "layer%drotationvcenter", layer+1 );
					if ( fhTSD->Get( key, value ) )
					{
						fhTSD->Set( key, value, FVP_FLOAT );
					}

					sprintf( key, "layer%duwrap", layer+1 );
					if ( fhTSD->Get( key, value ) )
					{
						fhTSD->Set( key, value, FVP_BOOL );
					}

					sprintf( key, "layer%dvwrap", layer+1 );
					if ( fhTSD->Get( key, value ) )
					{
						fhTSD->Set( key, value, FVP_BOOL );
					}

					sprintf( key, "layer%dcolorop", layer+1 );
					if ( fhTSD->Get( key, value ) )
					{
						fhTSD->Set( key, value, FVP_STRING );
					}

					sprintf( key, "layer%dcolorarg1", layer+1 );
					if ( fhTSD->Get( key, value ) )
					{
						fhTSD->Set( key, value, FVP_STRING );
					}

					sprintf( key, "layer%dcolorarg2", layer+1 );
					if ( fhTSD->Get( key, value ) )
					{
						fhTSD->Set( key, value, FVP_STRING );
					}

					sprintf( key, "layer%dalphaop", layer+1 );
					if ( fhTSD->Get( key, value ) )
					{
						fhTSD->Set( key, value, FVP_STRING );
					}

					sprintf( key, "layer%dalphaarg1", layer+1 );
					if ( fhTSD->Get( key, value ) )
					{
						fhTSD->Set( key, value, FVP_STRING );
					}

					sprintf( key, "layer%dalphaarg2", layer+1 );
					if ( fhTSD->Get( key, value ) )
					{
						fhTSD->Set( key, value, FVP_STRING );
					}
				}
			}
		}
		SendMessage( hwndProgressBar, PBM_STEPIT, 0, 0 ); 
	}

	// Write fuel
	SetDlgItemText( hDlg, IDC_STATUS, "Writing Fuel..." );
	SendMessage( hwndProgressBar, PBM_SETPOS, (WPARAM) 0, 0 ); 
	pFuelDB->SaveChanges();

	delete pFuelSys;
	delete pMasterFileMgr;

	SetDlgItemText( hDlg, IDC_STATUS, "Ready" );
	SendMessage( hwndProgressBar, PBM_SETPOS, (WPARAM) 0, 0 );
	EnableDialogControls( TRUE );
}


void ExportTableFile()
{
	// Get dialog settings
	bool bExportDims = ( SendMessage( GetDlgItem( hDlg, IDC_TEXTUREDIMS ), BM_GETCHECK, 0, 0 ) == BST_CHECKED );
	gpstring sFilename = winx::GetDlgItemText( hDlg, IDC_FILENAME );
	gpstring sFuelRoot = winx::GetDlgItemText( hDlg, IDC_FUELROOT );
	gpstring sHomeDir = winx::GetDlgItemText( hDlg, IDC_HOMEPATH );
	stringtool::AppendTrailingBackslash( sHomeDir );
	
	StringColl	TsdHeader;
	gpstring sTsdVal;
	gpstring sTsdVals = winx::GetDlgItemText( hDlg, IDC_TSDVALS );
	for( int iTsdVals = 0; iTsdVals < stringtool::GetNumDelimitedStrings( sTsdVals, ',' ); ++iTsdVals )
	{
		stringtool::GetDelimitedValue( sTsdVals, ',', iTsdVals, sTsdVal );
		TsdHeader.push_back( sTsdVal );
	}

	// Refresh interface
	EnableDialogControls( FALSE );
	hwndProgressBar = GetDlgItem( hDlg, IDC_PROGRESS );


	// Initialize file manager for fuel
	FileSys::MasterFileMgr * pMasterFileMgr	= new MasterFileMgr();
	FileSys::PathFileMgr * pPathFileMgr		= new PathFileMgr( sHomeDir );
	pMasterFileMgr->AddFileMgr( pPathFileMgr, true );

	// Create our fuel database
	FuelSys * pFuelSys = new FuelSys;
	TextFuelDB * pFuelDB = pFuelSys->AddTextDb( "default" );
	pFuelDB->Init( FuelDB::OPTION_JIT_READ | FuelDB::OPTION_READ, sHomeDir );

	// Use FileSys to find all the PSD files in the HomeDir
	gpstring sRoot( sHomeDir.c_str() );
	stringtool::ReplaceAllChars( sFuelRoot, ':', '\\' );
	sRoot.append( sFuelRoot );
	stringtool::ReplaceAllChars( sFuelRoot, '\\', ':' );
	sRoot.append( "\\*" );
	AutoFindHandle hFindRoot( sRoot.c_str(), FINDFILTER_DIRECTORIES );
	
	TsdEntries.clear();	
	SetDlgItemText( hDlg, IDC_STATUS, "Finding *.PSD..." );
	FindPSDFiles( hFindRoot );

	hFindRoot.Close();

	// Search fuel for existing TSD values
	SetDlgItemText( hDlg, IDC_STATUS, "Searching Fuel for tsd entries..." );
	FuelHandle fhBitmaps( sFuelRoot );	
	FuelHandleList fhlBitmaps = fhBitmaps->ListChildBlocks( -1 );
	FuelHandleList::iterator i;

	int TSDcount = 0;
    SendMessage( hwndProgressBar, PBM_SETRANGE, 0, MAKELPARAM( 0, fhlBitmaps.size() + 1 ) ); 
    SendMessage( hwndProgressBar, PBM_SETSTEP, (WPARAM) 1, 0 ); 
	for( i = fhlBitmaps.begin(); i != fhlBitmaps.end(); ++i ) 
	{
		if( strcmp( (*i)->GetType(), "tsd" ) == 0 )
		{
			gpstring gastexturename = GetOnlyFilename( (*i)->GetOriginPath() );

			TSDColl::iterator idx;
			for( idx = TsdEntries.begin(); idx != TsdEntries.end(); ++idx )
			{
				if( gastexturename.same_no_case( (*idx).Texturename ) )
				{
					(*idx).TsdValues.clear();

					StringColl::iterator k;
					for( k = TsdHeader.begin(); k != TsdHeader.end(); ++k )
					{
						(*idx).TsdValues.push_back( (*i)->GetString( (*k) ) );
					}
					++TSDcount;
				}
			}
		}
		SendMessage( hwndProgressBar, PBM_STEPIT, 0, 0 );
	}

	// Write the table file
	gpstring LastFuelPath;

	SetDlgItemText( hDlg, IDC_STATUS, "Writing table file..." );

	FILE * pFile = fopen( sFilename.c_str(), "wb" );
	if( !pFile )
	{
		MessageBox( NULL, "The file you specified could not be opened.", "Error", MB_OK | MB_ICONEXCLAMATION );

		delete pFuelSys;
		delete pMasterFileMgr;

		SetDlgItemText( hDlg, IDC_STATUS, "Ready" );
		SendMessage( hwndProgressBar, PBM_SETPOS, (WPARAM) 0, 0 );
		EnableDialogControls( TRUE );
		return;
	}

    SendMessage( hwndProgressBar, PBM_SETRANGE, 0, MAKELPARAM( 0, TsdEntries.size() + 1 ) ); 
    SendMessage( hwndProgressBar, PBM_SETSTEP, (WPARAM) 1, 0 );
    SendMessage( hwndProgressBar, PBM_SETPOS, (WPARAM) 0, 0 );

	// Write table header
	fwrite( TSD_HEADER_TEXTURE, strlen( TSD_HEADER_TEXTURE ), 1, pFile );
	fwrite( ",", 1, 1, pFile );
	if( bExportDims )
	{
		fwrite( TSD_HEADER_DIMENSIONS, strlen( TSD_HEADER_DIMENSIONS ), 1, pFile );
		fwrite( ",", 1, 1, pFile );
	}
	StringColl::iterator iStr;
	for( iStr = TsdHeader.begin(); iStr != TsdHeader.end(); ++iStr )
	{
		fwrite( "<", 1, 1, pFile );
		fwrite( (*iStr), (*iStr).size(), 1, pFile );
		fwrite( ">,", 2, 1, pFile );
	}
	fwrite( TSD_HEADER_FILENAME, strlen( TSD_HEADER_FILENAME ), 1, pFile );
	fwrite( "\n", 1, 1, pFile );

	TSDColl::iterator idx;
	for( idx = TsdEntries.begin(); idx != TsdEntries.end(); ++idx )
	{
		if( LastFuelPath.compare_no_case( GetFuelPathFromFilename( (*idx).Filename.right( (*idx).Filename.size() - sHomeDir.size() ) ) ) != 0 )
		{
			LastFuelPath = GetFuelPathFromFilename( (*idx).Filename.right( (*idx).Filename.size() - sHomeDir.size() ) );

			fwrite( "\n\n", 2, 1, pFile );
			fwrite( "# ", 2, 1, pFile );
			fwrite( LastFuelPath.c_str(), LastFuelPath.size(), 1, pFile );
			fwrite( "\n", 1, 1, pFile );
		}

		fwrite( (*idx).Texturename.c_str(), (*idx).Texturename.size(), 1, pFile );
		fwrite( ",", 1, 1, pFile );

		if( bExportDims )
		{
			PSDINFOHEADER psdInfo;
			char buf[16];

			GetPSDInfo( (*idx).Filename.c_str(), psdInfo );			
			_ltoa( psdInfo.m_Width, buf, 10 );
			fwrite( buf, strlen( buf ), 1, pFile );
			fwrite( " x ", 3, 1, pFile );
			_ltoa( psdInfo.m_Height, buf, 10 );
			fwrite( buf, strlen( buf ), 1, pFile );
			fwrite( ",", 1, 1, pFile );
		}
		
		(*idx).TsdValues.resize( TsdHeader.size() );
		for( iStr = (*idx).TsdValues.begin(); iStr != (*idx).TsdValues.end(); ++iStr )
		{
			fwrite( (*iStr), (*iStr).size(), 1, pFile );
			fwrite( ",", 1, 1, pFile );
		}

		fwrite( (*idx).Filename, (*idx).Filename.size(), 1, pFile );
		fwrite( "\n", 1, 1, pFile );

		SendMessage( hwndProgressBar, PBM_STEPIT, 0, 0 );
	}

	fclose( pFile );
	
	delete pFuelSys;
	delete pMasterFileMgr;

	SetDlgItemText( hDlg, IDC_STATUS, "Ready" );
	SendMessage( hwndProgressBar, PBM_SETPOS, (WPARAM) 0, 0 );
	EnableDialogControls( TRUE );
}

