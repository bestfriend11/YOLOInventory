/*=====================================================================

	Name	:	ContentTune
	Date	:	Jan, 2001
	Author	:	Adam Swensen

	Purpose	:	Provides an easy way to create and adjust leaf
				template values (actors, items, etc) on a mass scale
				in the game.

---------------------------------------------------------------------*/


#include <windows.h>
#include <commctrl.h>
#include <wtypes.h>
#include <stdio.h>

#include "gpcore.h"
#include "fuel.h"
#include "fueldb.h"
#include "gpmem.h"
#include "filesys.h"
#include "pathfilemgr.h"
#include "masterfilemgr.h"
#include "filesysutils.h"
#include "stringtool.h"
#include "config.h"
#include "winx.h"

#include "resource.h"


using namespace std;
using namespace FileSys;


// Types
typedef std::pair< gpstring, gpstring > StringPair; // <key, value> pair
typedef std::vector< StringPair > ContentColl;

struct ContentTemplate
{
	ContentColl			m_Values;
	gpstring			m_Specializes;
	gpstring			m_GasFilename;
};
typedef std::map< gpstring, ContentTemplate*, istring_less > TemplateMap;

typedef std::vector< gpstring > StringColl;
typedef std::pair< gpstring, StringColl > Row;
typedef std::vector< Row > RowColl;


// Functions
BOOL CALLBACK AppDialogProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );

void EnableDialogControls( BOOL bState );

gpstring GetOnlyFilename( const char * pStr );
gpstring GetOnlyPath( const char * pStr );
gpstring GetFuelPathFromFilename( gpstring & filename );

void LoadTemplateValuesAtHandle( ContentTemplate *pTemplate, FuelHandle fhTemplate, gpstring currentPath = "" );
void LoadTemplatesAtHandle( FuelHandle fhContent );
void ClearTemplates();

void ExportTableFile();
void ImportTableFile();

void LoadSettings( HWND hDlg );
void SaveSettings( HWND hDlg );


// Globals
HWND		hDlg;
HWND		hwndProgressBar;

ContentColl	GasFileFilterList; // GasFilename, TableFilename

StringColl GasFileFilter;

TemplateMap gTemplates;

StringColl Specialized;

// Constants
static const char *TableDelimeter = "\t";

#define UGLYHO 0


int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow )
{
    MSG			msg;
	Config		MyConfig( COMPANY_NAME, "ContentTune" );

	InitCommonControls();

	hDlg = CreateDialog( hInstance, MAKEINTRESOURCE( IDD_CONTENTTUNE ), NULL, AppDialogProc );
	gpassertm( hDlg, "Failed to create application dialog." );
	ShowWindow( hDlg, iCmdShow );
 
    while( GetMessage( &msg, NULL, 0, 0 ) ) 
	{
		TranslateMessage( &msg );
		DispatchMessage ( &msg );		
    }

    return( msg.wParam );
}


BOOL CALLBACK AppDialogProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_INITDIALOG:
		{
			LoadSettings( hDlg );
			break;
		}

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case IDC_EXPORT:
				{
					ExportTableFile();
					break;
				}
				case IDC_IMPORT:
				{
					ImportTableFile();
					break;
				}
				case IDC_OPENEXCEL:
				{
					ShellExecute( hDlg, "open", "excel", winx::GetDlgItemText( hDlg, IDC_FILENAME ), GetOnlyPath( winx::GetDlgItemText( hDlg, IDC_FILENAME ) ), SW_SHOW );
					break;
				}
				case IDC_CLOSE:
				{
					SendMessage( hDlg, WM_CLOSE, 0, 0 );
					break;
				}
				case IDC_COMBOGASFILES:
				{
					if ( HIWORD(wParam) == CBN_SELCHANGE )
					{
						gpstring gasFileFilter = winx::GetDlgItemText( hDlg, IDC_COMBOGASFILES );
						int selIndex = SendMessage( (HWND)lParam, CB_GETCURSEL, 0, 0 );
						//int textLen = SendMessage( (HWND)lParam, CB_GETLBTEXTLEN, selIndex, 0 );
						char buffer[1024];
						SendMessage( (HWND)lParam, CB_GETLBTEXT, selIndex, (LPARAM)buffer );
						gasFileFilter = buffer;
						for ( ContentColl::iterator i = GasFileFilterList.begin(); i != GasFileFilterList.end(); ++i )
						{
							if ( gasFileFilter.same_no_case( (*i).first ) )
							{
								SetDlgItemText( hDlg, IDC_FILENAME, (*i).second.c_str() );
							}
						}
					}
					break;
				}
			}
			break;
		}

		case WM_CLOSE:
		case WM_DESTROY:
		{
			ClearTemplates();
			SaveSettings( hDlg );
			PostQuitMessage( 0 );
			break;
		}
	}

	return( FALSE );
}


void LoadSettings( HWND hDlg )
{
	gpstring value;

	if( gConfig.ReadKeyValue( "HomePath", value ) )
	{
		SetDlgItemText( hDlg, IDC_HOMEPATH, value.c_str() );
	}
	else
	{
		SetDlgItemText( hDlg, IDC_HOMEPATH, "p:\\tattoo_assets\\cdimage" );
	}
	
	if( gConfig.ReadKeyValue( "DefaultFilename", value ) )
	{
		SetDlgItemText( hDlg, IDC_FILENAME, value.c_str() );
	}
	else
	{
		SetDlgItemText( hDlg, IDC_FILENAME, "c:\\temp\\contentdb.txt" );
	}
	
	if( gConfig.ReadKeyValue( "FuelRoot", value ) )
	{
		SetDlgItemText( hDlg, IDC_FUELROOT, value.c_str() );
	}
	else
	{
		SetDlgItemText( hDlg, IDC_FUELROOT, "world:contentdb" );
	}

	if( gConfig.ReadKeyValue( "Specialized", value ) )
	{
		SetDlgItemText( hDlg, IDC_SPECIALIZED, value.c_str() );
	}
	else
	{
		SetDlgItemText( hDlg, IDC_SPECIALIZED, "" );
	}
	
	HWND hCtl = GetDlgItem( hDlg, IDC_COMBOGASFILES );
	SendMessage( hCtl, CB_RESETCONTENT, 0, 0 );
	GasFileFilterList.clear();
	for ( int i = 0; i < 16; ++i )
	{
		gpstring key( "GasFileFilter" );
		stringtool::Append( i, key );
		gpstring gasFileFilter;
		gConfig.ReadKeyValue( key, gasFileFilter );
		if ( gasFileFilter.size() == 0 )
		{
			break;
		}

		key = "GasFileFilterTableFile";
		stringtool::Append( i, key );
		gpstring tableFile;
		gConfig.ReadKeyValue( key, tableFile );

		GasFileFilterList.push_back( StringPair( gasFileFilter, tableFile ) );
		SendMessage( hCtl, CB_ADDSTRING, 0, (LPARAM)gasFileFilter.c_str() );
	}

	if( gConfig.ReadKeyValue( "GasFileFilter", value ) )
	{
		SetDlgItemText( hDlg, IDC_COMBOGASFILES, value.c_str() );
	}
	else
	{
		SetDlgItemText( hDlg, IDC_COMBOGASFILES, "" );
	}

	if( gConfig.ReadKeyValue( "FlipTable", value ) )
	{
		if( value.same_no_case( "true" ) )
		{
			HWND hwnd = GetDlgItem( hDlg, IDC_FLIPTABLE );
			SendMessage( hwnd, BM_SETCHECK, BST_CHECKED, 0 );
		}
	}
}


void SaveSettings( HWND hDlg )
{
	gpstring value;

	value = winx::GetDlgItemText( hDlg, IDC_HOMEPATH );
	gConfig.SetKeyValue( "HomePath", value );

	gpstring tableFile = winx::GetDlgItemText( hDlg, IDC_FILENAME );
	gConfig.SetKeyValue( "DefaultFilename", tableFile );
	
	value = winx::GetDlgItemText( hDlg, IDC_FUELROOT );
	gConfig.SetKeyValue( "FuelRoot", value );

	value = winx::GetDlgItemText( hDlg, IDC_SPECIALIZED );
	gConfig.SetKeyValue( "Specialized", value );

	gpstring gasFileFilter = winx::GetDlgItemText( hDlg, IDC_COMBOGASFILES );
	gConfig.SetKeyValue( "GasFileFilter", gasFileFilter );

	if ( gasFileFilter.size() > 0 )
	{
		int filterCount = 1;
		gConfig.SetKeyValue( "GasFileFilter0", gasFileFilter );
		gConfig.SetKeyValue( "GasFileFilterTableFile0", tableFile );
		for ( ContentColl::iterator i = GasFileFilterList.begin(); i != GasFileFilterList.end(); ++i )
		{
			if ( !gasFileFilter.same_no_case( (*i).first ) )
			{
				gpstring key( "GasFileFilter" );
				stringtool::Append( filterCount, key );
				gConfig.SetKeyValue( key, (*i).first );
				key = "GasFileFilterTableFile";
				stringtool::Append( filterCount, key );
				gConfig.SetKeyValue( key, (*i).second );
				++filterCount;
			}
		}
	}

	HWND hwnd = GetDlgItem( hDlg, IDC_FLIPTABLE );
	if ( SendMessage( hwnd, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
	{
		gConfig.SetKeyValue( "FlipTable", "true" );
	}
	else
	{
		gConfig.SetKeyValue( "FlipTable", "false" );
	}
}


void EnableDialogControls( BOOL bState )
{
	HWND hCtl;

	hCtl = GetDlgItem( hDlg, IDC_EXPORT );
	EnableWindow( hCtl, bState );
	hCtl = GetDlgItem( hDlg, IDC_IMPORT );
	EnableWindow( hCtl, bState );
	hCtl = GetDlgItem( hDlg, IDC_OPENEXCEL );
	EnableWindow( hCtl, bState );
	hCtl = GetDlgItem( hDlg, IDC_CLOSE );
	EnableWindow( hCtl, bState );	
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


void ImportTableFile()
{
	ClearTemplates();

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
	pFuelDB->Init( FuelDB::OPTION_JIT_READ | FuelDB::OPTION_WRITE | FuelDB::OPTION_SAVE_EMPTY_BLOCKS, sHomeDir, sHomeDir );

	// Read in the table file
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

	gpstring sBuf;
	char cBuf;
	int colNum = 0, rowNum = 0;
	int elementCount = 0;
	gpstring sGasFilename;
	RowColl Rows;
	RowColl::iterator currentRow = NULL;
	bool bGetTemplateNameRow = false;
	int TemplateNameCol = 0;
	StringColl templateNames;
	RowColl gasFiles; // gasFile, keepTemplates
	bool bFlippedTable = false;

#if UGLYHO
	bFlippedTable = true;
	StringColl gasFileList;
#endif

	while( !feof( pFile ) )
	{
		fread( &cBuf, 1, 1, pFile );

		if( ( cBuf == '\n' ) || ( cBuf == TableDelimeter[0] ) )
		{
#if !UGLYHO
			if( sBuf.left( 6 ).same_no_case( "[FILE=" ) )
			{
				// add template group
				if ( (sGasFilename.size() > 0) && (Rows.size() > 0) )
				{
					int colAt = 0;
					for ( StringColl::iterator i = templateNames.begin(); i != templateNames.end(); ++i, ++colAt )
					{
						ContentTemplate *pTemplate = new ContentTemplate;
						pTemplate->m_GasFilename = sGasFilename;
						gTemplates.insert( TemplateMap::value_type( (*i), pTemplate ) );

						for ( RowColl::iterator j = Rows.begin(); j != Rows.end(); ++j )
						{
							if ( colAt < (*j).second.size() )
							{
								gpstring &value = (*j).second[colAt];
								if ( value.size() > 0 )
								{
									if ( (*j).first.left(11).same_no_case( "specializes" ) )
									{
										pTemplate->m_Specializes = value;
									}
									for ( int k = 0; k < value.size(); ++k )
									{
										if ( value[k] == '\'' )
										{
											value.replace( k, 1, "\"" );
										}
									}
									pTemplate->m_Values.push_back( StringPair( (*j).first, value ) );
								}
							}
						}
					}
				}

				Rows.clear();
				templateNames.clear();

				// gas filename
				if ( sBuf.right( 2 ).same_no_case( "]*" ) )
				{
					sBuf = sBuf.left( sBuf.size() - 2 );
					bFlippedTable = true;
				}
				else
				{
					sBuf = sBuf.left( sBuf.size() - 1 );
					bFlippedTable = false;
				}
				sGasFilename = sBuf.right( sBuf.size() - 6 );
				sGasFilename = sGasFilename.right( sGasFilename.size() - sHomeDir.size() );
				gasFiles.push_back( Row( sGasFilename, StringColl() ) );
				rowNum = 0;
				TemplateNameCol = 0;
			}
			else
#endif
			{
				if ( !sBuf.empty() )
				{
					++elementCount;
				}
				if ( !bFlippedTable )
				{
					if ( colNum == 0 )
					{
						if ( !sBuf.empty() )
						{
							if ( sBuf.same_no_case( "#Template" ) )
							{
								bGetTemplateNameRow = true;
							}
							else
							{
								Rows.push_back( Row( sBuf, StringColl() ) );
								currentRow = &Rows.back();
							}
						}
					}
					else
					{
						if ( bGetTemplateNameRow )
						{
							if ( !sBuf.empty() )
							{
								templateNames.push_back( sBuf );
							}
						}
						else
						{
							if ( currentRow )
							{
								currentRow->second.push_back( sBuf );
							}
						}
					}
				}
				else
				{
					if ( rowNum == 0 )
					{
						if ( !sBuf.empty() )
						{
							if ( sBuf.same_no_case( "#Template" ) )
							{
								TemplateNameCol = colNum;
							}
#if UGLYHO
							else if ( sBuf.same_no_case( "#GasFile" ) )
							{
							}
#endif
							else
							{
								Rows.push_back( Row( sBuf, StringColl() ) );
							}
						}
					}
					else
					{
#if !UGLYHO
						if ( colNum == TemplateNameCol )
						{
							if ( !sBuf.empty() )
							{
								templateNames.push_back( sBuf );
							}
						}
#else
						if ( colNum == 0 )
						{
							if ( !sBuf.empty() )
							{
								if ( sBuf.right( 2 ).same_no_case( "]*" ) )
								{
									sBuf = sBuf.left( sBuf.size() - 2 );
								}
								else
								{
									sBuf = sBuf.left( sBuf.size() - 1 );
								}
								sGasFilename = sBuf.right( sBuf.size() - 1 );
								sGasFilename = sGasFilename.right( sGasFilename.size() - sHomeDir.size() );

								bool bFound = false;
								for ( StringColl::iterator i = gasFileList.begin(); i != gasFileList.end(); ++i )
								{
									if ( i->same_no_case( sGasFilename ) )
									{
										bFound = true;
										break;
									}
								}
								if ( !bFound )
								{
									gasFiles.push_back( Row( sGasFilename, StringColl() ) );
								}
								gasFileList.push_back( sGasFilename );
							}
						}
						else if ( colNum == 1 )
						{
							if ( !sBuf.empty() )
							{
								templateNames.push_back( sBuf );
							}
						}
#endif
						else
						{
#if UGLYHO
							if ( (colNum-2) < Rows.size() )
							{
								for ( int i = Rows[colNum-2].second.size(); i < (rowNum - 1); ++i )
								{
									Rows[colNum-2].second.push_back( gpstring() );
								}
								Rows[colNum-2].second.push_back( sBuf );
							}
#else
							if ( (colNum-1) < Rows.size() )
							{
								for ( int i = Rows[colNum-1].second.size(); i < (rowNum - 1); ++i )
								{
									Rows[colNum-1].second.push_back( gpstring() );
								}
								Rows[colNum-1].second.push_back( sBuf );
							}
#endif
						}
					}
				}
			}

			sBuf.clear();
			++colNum;
		}
		else
		{
			if ( (cBuf != '\"') && (cBuf != '\r') )
			{
				sBuf += cBuf;
			}
		}

		// at the end of the row
		if( cBuf == '\n' )
		{
			sBuf.clear();
			colNum = 0;
			if ( elementCount > 0 )
			{
				++rowNum;
			}
			elementCount = 0;
			currentRow = NULL;
			bGetTemplateNameRow = false;
		}
	}
#if UGLYHO
	if ( (sGasFilename.size() > 0) && (Rows.size() > 0) )
	{
		int colAt = 0;
		StringColl::iterator iGas = gasFileList.begin();
		for ( StringColl::iterator i = templateNames.begin(); i != templateNames.end(); ++i, ++colAt, ++iGas )
		{
			ContentTemplate *pTemplate = new ContentTemplate;
			pTemplate->m_GasFilename = *iGas;
			gTemplates.insert( TemplateMap::value_type( (*i), pTemplate ) );

			for ( RowColl::iterator j = Rows.begin(); j != Rows.end(); ++j )
			{
				if ( colAt < (*j).second.size() )
				{
					gpstring &value = (*j).second[colAt];
					if ( value.size() > 0 )
					{
						if ( (*j).first.left(11).same_no_case( "specializes" ) )
						{
							pTemplate->m_Specializes = value;
						}
						for ( int k = 0; k < value.size(); ++k )
						{
							if ( value[k] == '\'' )
							{
								value[k] = '\"';
							}
						}
						pTemplate->m_Values.push_back( StringPair( (*j).first, value ) );
					}
				}
			}
		}
	}
#else
	// add last template group
	if ( (sGasFilename.size() > 0) && (Rows.size() > 0) )
	{
		int colAt = 0;
		for ( StringColl::iterator i = templateNames.begin(); i != templateNames.end(); ++i, ++colAt )
		{
			ContentTemplate *pTemplate = new ContentTemplate;
			pTemplate->m_GasFilename = sGasFilename;
			gTemplates.insert( TemplateMap::value_type( (*i), pTemplate ) );

			for ( RowColl::iterator j = Rows.begin(); j != Rows.end(); ++j )
			{
				if ( colAt < (*j).second.size() )
				{
					gpstring &value = (*j).second[colAt];
					if ( value.size() > 0 )
					{
						if ( (*j).first.left(11).same_no_case( "specializes" ) )
						{
							pTemplate->m_Specializes = value;
						}
						for ( int k = 0; k < value.size(); ++k )
						{
							if ( value[k] == '\'' )
							{
								value.replace( k, 1, "\"" );
							}
						}
						pTemplate->m_Values.push_back( StringPair( (*j).first, value ) );
					}
				}
			}
		}
	}
#endif
	fclose( pFile );

	// update templates in fuel
	SetDlgItemText( hDlg, IDC_STATUS, "Updating templates in Fuel..." );
    SendMessage( hwndProgressBar, PBM_SETRANGE, 0, MAKELPARAM( 0, gTemplates.size() + 1 ) ); 
    SendMessage( hwndProgressBar, PBM_SETSTEP, (WPARAM) 1, 0 ); 

	{for ( TemplateMap::iterator i = gTemplates.begin(); i != gTemplates.end(); ++i )
	{
		FuelHandle fhRoot( GetFuelPathFromFilename( (*i).second->m_GasFilename ) );
		if ( !fhRoot )
		{
			gpassertm( fhRoot, gpstring( "The fuel path '" ) + GetFuelPathFromFilename( (*i).second->m_GasFilename ) + "' was not found." );
			continue;
		}

		// this is to catch any templates of the same name as a directory found in the same base fuel location
		if ( !fhRoot->IsDirectory() )
		{
			bool bFoundRootDirectory = false;
			gpstring sFuelPath = GetFuelPathFromFilename( (*i).second->m_GasFilename );
			int pos = sFuelPath.find_last_of( ":" );
			sFuelPath = sFuelPath.right( sFuelPath.size() - (pos+1) );

			fhRoot = fhRoot->GetParent();
			gpassert( fhRoot );

			FuelHandleList fhlFindRoot = fhRoot->ListChildBlocks();
			for ( FuelHandleList::iterator j = fhlFindRoot.begin(); j != fhlFindRoot.end(); ++j )
			{
				if ( sFuelPath.same_no_case( (*j)->GetName() ) && ( (*j)->IsDirectory() ) )
				{
					fhRoot = (*j);
					bFoundRootDirectory = true;
					break;
				}
			}

			gpassertm( bFoundRootDirectory, gpstring( "Could not find a directory path for : " ) + GetFuelPathFromFilename( (*i).second->m_GasFilename ) );
		}

		// destroy old template
		FuelHandle fhOldTemplate = fhRoot->GetChildBlock( (*i).first );
		if ( fhOldTemplate )
		{
			fhRoot->DestroyChildBlock( fhOldTemplate, false );
		}

		// protect this template and its specialize template from cleansing
		{for ( RowColl::iterator j = gasFiles.begin(); j != gasFiles.end(); ++j )
		{
			if ( (*j).first.same_no_case( (*i).second->m_GasFilename ) )
			{
				(*j).second.push_back( (*i).first );
				bool flag = false;
				for ( StringColl::iterator k = (*j).second.begin(); k != (*j).second.end(); ++k )
				{
					if ( (*k).same_no_case( (*i).second->m_Specializes ) )
					{
						flag = true;
						break;
					}
				}
				if ( !flag )
				{
					(*j).second.push_back( (*i).second->m_Specializes );
				}
			}
		}}

		// create new template
		gpstring sGasFilename = GetOnlyFilename( (*i).second->m_GasFilename ) + ".gas";
		gpassert( !sGasFilename.same_no_case( ".gas" ) );
		FuelHandle fhTemplate = fhRoot->CreateChildBlock( (*i).first, sGasFilename );
		gpassertm( fhTemplate, "Child block not created." );

		if ( fhTemplate )
		{
			fhTemplate->SetType( "template" );

			typedef std::map< gpstring, FuelHandle, istring_less > MultiBlockMap;
			MultiBlockMap MultiBlocks;

			for ( ContentColl::iterator j = (*i).second->m_Values.begin(); j != (*i).second->m_Values.end(); ++j )
			{
				FuelHandle fhKey = fhTemplate;

				gpstring key = (*j).first;
				gpstring value = (*j).second;
				bool bIsString = false;

				if ( key.right( 1 ).same_no_case( "$" ) )
				{
					key = key.left( key.size() - 1 );
					bIsString = true;
				}

				int pos = 0;
				int index = key.find( ":", 0 );
				while ( index != std::string::npos )
				{
					gpstring childBlock = key.mid( pos, index - pos );

					if ( childBlock.right( 1 ).same_no_case( "]" ) )
					{
						gpstring multiKey = key.left( index );
						MultiBlockMap::iterator findBlock = MultiBlocks.find( multiKey );
						if ( findBlock != MultiBlocks.end() )
						{
							fhKey = findBlock->second;
						}
						else
						{
							int pos = childBlock.find( "[" );
							childBlock = childBlock.left( pos );
							fhKey = fhKey->CreateChildBlock( childBlock );
							MultiBlocks.insert( MultiBlockMap::value_type( multiKey, fhKey ) );
						}
					}
					else
					{
						fhKey = fhKey->GetChildBlock( childBlock, true );
					}

					pos = index + 1;
					index = key.find_first_of( ":", pos );
				}
				key = key.mid( pos, key.size() - pos );

				if ( !key.empty() )
				{
					if ( bIsString )
					{
						fhKey->Add( key, value, FVP_QUOTED_STRING );
					}
					else
					{
						if ( key.same_no_case( "armor_style" ) )
						{
							value = gpstring().append( 3 - value.size(), '0' ) + value;
						}
						fhKey->Add( key, value );
					}
				}
			}
		}

		SendMessage( hwndProgressBar, PBM_STEPIT, 0, 0 ); 
	}}

	// destroy everything else that isn't protected
#if !UGLYHO
	{for( RowColl::iterator i = gasFiles.begin(); i != gasFiles.end(); ++i )
	{
		FuelHandle fhRoot( GetFuelPathFromFilename( (*i).first ) );
		if ( !fhRoot )
		{
			gpassertm( fhRoot, gpstring( "The fuel path '" ) + GetFuelPathFromFilename( (*i).first ) + "' was not found." );
			continue;
		}

		// this is to catch any templates of the same name as a directory found in the same base fuel location
		if ( !fhRoot->IsDirectory() )
		{
			bool bFoundRootDirectory = false;
			gpstring sFuelPath = GetFuelPathFromFilename( (*i).first );
			int pos = sFuelPath.find_last_of( ":" );
			sFuelPath = sFuelPath.right( sFuelPath.size() - (pos+1) );

			fhRoot = fhRoot->GetParent();
			gpassert( fhRoot );

			FuelHandleList fhlFindRoot = fhRoot->ListChildBlocks();
			for ( FuelHandleList::iterator j = fhlFindRoot.begin(); j != fhlFindRoot.end(); ++j )
			{
				if ( sFuelPath.same_no_case( (*j)->GetName() ) && ( (*j)->IsDirectory() ) )
				{
					fhRoot = (*j);
					bFoundRootDirectory = true;
					break;
				}
			}

			gpassertm( bFoundRootDirectory, gpstring( "Could not find a directory path for : " ) + GetFuelPathFromFilename( (*i).first ) );
		}

		FuelHandleList fhlTemplates = fhRoot->ListChildBlocks( 1 );
		{for( FuelHandleList::iterator j = fhlTemplates.begin(); j != fhlTemplates.end(); ++j ) 
		{
			if( strcmp( (*j)->GetType(), "template" ) == 0 )
			{
				if ( GetOnlyFilename( (*i).first ).same_no_case( GetOnlyFilename( (*j)->GetOriginPath() ) ) )
				{
					bool flag = false;
					for ( StringColl::iterator k = (*i).second.begin(); k != (*i).second.end(); ++k )
					{
						if ( (*k).same_no_case( (*j)->GetName() ) )
						{
							flag = true;
							break;
						}
					}
					if ( !flag )
					{
						fhRoot->DestroyChildBlock( (*j), false );
					}
				}
			}
		}}
	}}
#endif

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


void ClearTemplates()
{
	for( TemplateMap::iterator i = gTemplates.begin(); i != gTemplates.end(); ++i )
	{
		delete (*i).second;
	}

	gTemplates.clear();
}


void LoadTemplateValuesAtHandle( ContentTemplate *pTemplate, FuelHandle fhTemplate, gpstring currentPath )
{
	if ( fhTemplate->FindFirstKeyAndValue() )
	{
		gpstring key, value;

		eFuelValueProperty valueProp = fhTemplate->GetNextKeyAndValue( key, value );
		while ( valueProp )
		{
			key = currentPath + key;
			if ( valueProp & FVP_QUOTED_STRING )
			{
				key = key + "$";
			}
			for ( int k = 0; k < value.size(); ++k )
			{
				if ( value[k] == '\"' )
				{
					value.replace( k, 1, "\'" );
				}
				if ( value[k] == '\t' )
				{
					value.replace( k, 1, " " );
				}
			}
			pTemplate->m_Values.push_back( StringPair( key, value ) );
			valueProp = fhTemplate->GetNextKeyAndValue( key, value );
		}
	}
	else
	{
		pTemplate->m_Values.push_back( StringPair( currentPath, "-" ) );
	}

	FuelHandleList fhlValues = fhTemplate->ListChildBlocks( 1 );
	for ( FuelHandleList::iterator i = fhlValues.begin(); i != fhlValues.end(); ++i )
	{
		gpstring newPath = currentPath + (*i)->GetName();

		int blockIndex = 0;

		for ( ContentColl::iterator j = pTemplate->m_Values.begin(); j != pTemplate->m_Values.end(); ++j )
		{
			if ( j->first.find( newPath ) != gpstring::npos )
			{
				gpstring temp = j->first.right( j->first.size() - newPath.size() );
				if ( temp.find( ":" ) != gpstring::npos )
				{
					if ( temp.left( 1 ).same_no_case( ":" ) )
					{
						j->first = newPath + "[0]" + temp;
						if ( blockIndex == 0 )
						{
							blockIndex = 1;
						}
					}
					else if ( temp.left( 1 ).same_no_case( "[" ) )
					{
						int value = 0;
						int bracketPos = temp.find( "]" );
						if ( bracketPos == gpstring::npos )
						{
							continue;
						}
						stringtool::Get( temp.substr( 1, bracketPos-1 ), value );
						if ( blockIndex < value + 1 )
						{
							blockIndex = value + 1;
						}
					}
				}
			}
		}

		if ( blockIndex > 0 )
		{
			newPath.appendf( "[%i]", blockIndex );
		}

		LoadTemplateValuesAtHandle( pTemplate, *i, newPath + ":" );
	}
}


void LoadTemplatesAtHandle( FuelHandle fhContent )
{
	// recursively find all templates
	FuelHandleList fhlContent = fhContent->ListChildBlocks( 1 );
	for( FuelHandleList::iterator i = fhlContent.begin(); i != fhlContent.end(); ++i ) 
	{
		if( strcmp( (*i)->GetType(), "template" ) == 0 )
		{
			// check to see if it is in the gas file filter list
			if ( GasFileFilter.size() > 0 )
			{
				bool flag = false;
				for ( StringColl::iterator k = GasFileFilter.begin(); k != GasFileFilter.end(); ++k )
				{
					if ( GetOnlyFilename( (*k) ).same_no_case( GetOnlyFilename( (*i)->GetOriginPath() ) ) )
					{
						flag = true;
						break;
					}
				}
				if ( !flag )
				{
					continue;
				}				
			}

			gpstring sTemplateName = (*i)->GetName();
			TemplateMap::iterator temp = gTemplates.find( sTemplateName );
			if ( temp != gTemplates.end() )
			{
				gpassertm( 0, gpstring( "Found a duplicate template name : " ) + sTemplateName );
				continue;
			}


			ContentTemplate *pTemplate = new ContentTemplate;
			pTemplate->m_GasFilename = (*i)->GetOriginPath();

			(*i)->Get( "specializes", pTemplate->m_Specializes );
			// just check to see if this template matches the correct specialized case (if needed)
			if ( Specialized.size() > 0 )
			{
				bool flag = false;
				for ( StringColl::iterator k = Specialized.begin(); k != Specialized.end(); ++k )
				{
					if ( (*k).same_no_case( pTemplate->m_Specializes ) )
					{
						flag = true;
						break;
					}
				}
				if ( !flag )
				{
					delete pTemplate;
					continue;
				}
			}

			temp = gTemplates.find( pTemplate->m_Specializes );
			if ( temp != gTemplates.end() )
			{
				// this template is specialized by another template, so it's not a leaf
				delete temp->second;
				gTemplates.erase( temp );
			}

			LoadTemplateValuesAtHandle( pTemplate, *i );

			gTemplates.insert( TemplateMap::value_type( sTemplateName, pTemplate ) );
		}
		else
		{
			LoadTemplatesAtHandle( (*i) );
		}
	}
}


//////////////////////////////////////////////////////////
// sorting

struct SortBlock;
typedef std::map< gpstring, SortBlock*, istring_less > SortMap;

struct SortValue
{
	SortValue( gpstring val )	{  value = val;  count = 0;  highCount = 1; }

	gpstring value;

	int count;
	int highCount;
	gpstring lastTemplate;
};
typedef std::vector< SortValue > SortValueColl;

struct SortBlock
{
	SortBlock()		{  bBlank = false;  }
	~SortBlock();
	bool		bBlank;
	SortValueColl  values;
	SortMap		blocks;
};

SortBlock::~SortBlock()
{
	for ( SortMap::iterator i = blocks.begin(); i != blocks.end(); ++i )
	{
		delete i->second;
	}
}

void AddSortValue( gpstring key, SortBlock *pBlock, gpstring templateName )
{
	int pos = key.find( ":" );
	int lastPos	= 0;

	while ( pos != gpstring::npos )
	{
		gpstring blockName = key.mid( lastPos, pos - lastPos );

		SortMap::iterator findBlock = pBlock->blocks.find( blockName );
		if ( findBlock == pBlock->blocks.end() )
		{
			SortBlock *newBlock = new SortBlock;
			pBlock->blocks.insert( SortMap::value_type( blockName, newBlock ) );
			pBlock = newBlock;
		}
		else
		{
			pBlock = findBlock->second;
		}
		lastPos = pos + 1;
		pos = key.find( ":", lastPos );
	}

	gpstring value = key.mid( lastPos, key.size() - lastPos );
	if ( value.empty() && pBlock->values.empty() )
	{
		pBlock->bBlank = true;
	}
	else
	{
		bool bFound = false;
		for ( SortValueColl::iterator i = pBlock->values.begin(); i != pBlock->values.end(); ++i )
		{
			if ( i->value.same_no_case( value ) )
			{
				bFound = true;
				break;
			}
		}
		if ( !bFound )
		{
			pBlock->values.push_back( SortValue( value ) );
		}
		else
		{
			if ( i->lastTemplate.same_no_case( templateName ) )
			{
				++i->count;
				if ( i->count > i->highCount )
				{
					i->highCount = i->count;
				}
			}
			else
			{
				i->count = 1;
			}
		}
	}
}


void MakeSortColl( StringColl & sortColl, SortBlock *pBlock, gpstring path = "" )
{
	if ( pBlock->bBlank )
	{
		sortColl.push_back( path + ":" );
	}

	{for ( SortValueColl::iterator i = pBlock->values.begin(); i != pBlock->values.end(); ++i )
	{
		for ( int j = 0; j < i->highCount; ++j )
		{
			if ( path.empty() )
			{
				sortColl.push_back( i->value );
			}
			else
			{
				sortColl.push_back( path + ":" + i->value );
			}
		}
	}}

	{for ( SortMap::iterator i = pBlock->blocks.begin(); i != pBlock->blocks.end(); ++i )
	{
		if ( path.empty() )
		{
			MakeSortColl( sortColl, i->second, i->first );
		}
		else
		{
			MakeSortColl( sortColl, i->second, path + ":" + i->first );
		}
	}}
}

//////////////////////////////////////////////////////////

void ExportTableFile()
{
	// Get dialog settings
	gpstring sFilename = winx::GetDlgItemText( hDlg, IDC_FILENAME );
	gpstring sHomeDir = winx::GetDlgItemText( hDlg, IDC_HOMEPATH );
	gpstring sFuelRoot = winx::GetDlgItemText( hDlg, IDC_FUELROOT );
	stringtool::AppendTrailingBackslash( sHomeDir );
	gpstring value;
	gpstring values = winx::GetDlgItemText( hDlg, IDC_SPECIALIZED );
	{for( int i = 0; i < stringtool::GetNumDelimitedStrings( values, ',' ); ++i )
	{
		stringtool::GetDelimitedValue( values, ',', i, value );
		Specialized.push_back( value );
	}}
	GasFileFilter.clear();
	value = "";
	values = winx::GetDlgItemText( hDlg, IDC_COMBOGASFILES );
	{for( int i = 0; i < stringtool::GetNumDelimitedStrings( values, ',' ); ++i )
	{
		stringtool::GetDelimitedValue( values, ',', i, value );
		GasFileFilter.push_back( value );
	}}
	bool bFlipTable = false;
	HWND hwnd = GetDlgItem( hDlg, IDC_FLIPTABLE );
	if ( SendMessage( hwnd, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
	{
		bFlipTable = true;
	}
#if UGLYHO
	bFlipTable = true;
#endif

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

	// Search fuel for templates
	SetDlgItemText( hDlg, IDC_STATUS, "Finding templates in ContentDB..." );

	FuelHandle fhContent( sFuelRoot );
	ClearTemplates();
	LoadTemplatesAtHandle( fhContent );
	fhContent.Unload();

	// Write the table file
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

	StringColl gasFiles;
	{for ( TemplateMap::iterator i = gTemplates.begin(); i != gTemplates.end(); ++i )
	{
		bool bFound = false;
		for ( StringColl::iterator j = gasFiles.begin(); j != gasFiles.end(); ++j )
		{
			if ( (*j).same_no_case( (*i).second->m_GasFilename ) )
			{
				bFound = true;
				break;
			}
		}

		if ( !bFound )
		{
			gasFiles.push_back( (*i).second->m_GasFilename );
		}
	}}


#if UGLYHO
	// create a master list of all the keys from all gathered templates
	StringColl sharedKeys;
	SortBlock *Sorted = new SortBlock;
	{for ( TemplateMap::iterator i = gTemplates.begin(); i != gTemplates.end(); ++i )
	{
		for ( ContentColl::iterator j = (*i).second->m_Values.begin(); j != (*i).second->m_Values.end(); ++j )
		{
			bool bFound = false;
			for ( StringColl::iterator k = sharedKeys.begin(); k != sharedKeys.end(); ++k )
			{
				if ( j->first.same_no_case( *k ) )
				{
					bFound = true;
					break;
				}
			}
			if ( !bFound )
			{
				sharedKeys.push_back( j->first );
				AddSortValue( j->first, Sorted, i->first );
			}
		}
	}}
	sharedKeys.clear();
	Sorted->bBlank = false;
	MakeSortColl( sharedKeys, Sorted );
	delete Sorted;

	{for ( StringColl::iterator i = sharedKeys.begin(); i != sharedKeys.end(); ++i )
	{
		gpgenericf(( "%s\n", i->c_str() ));
	}}

	// write out header
	fprintf( pFile, "#GasFile%s#Template", TableDelimeter );
	{for ( StringColl::iterator i = sharedKeys.begin(); i != sharedKeys.end(); ++i )
	{
		fprintf( pFile, "%s%s", TableDelimeter, i->c_str() );
	}}
	fprintf( pFile, "\n" );
#endif

    SendMessage( hwndProgressBar, PBM_SETRANGE, 0, MAKELPARAM( 0, gasFiles.size() + 1 ) ); 
    SendMessage( hwndProgressBar, PBM_SETSTEP, (WPARAM) 1, 0 );
    SendMessage( hwndProgressBar, PBM_SETPOS, (WPARAM) 0, 0 );
	{for ( StringColl::iterator i = gasFiles.begin(); i != gasFiles.end(); ++i )
	{
		int columnCount = 0;
		RowColl Rows;
		Rows.push_back( Row( gpstring( "#Template" ), StringColl() ) );

#if UGLYHO
		{for ( TemplateMap::iterator j = gTemplates.begin(); j != gTemplates.end(); ++j )
		{
			if ( (*i).same_no_case( (*j).second->m_GasFilename ) )
			{
				// add the template's name to the header
				Rows[0].second.push_back( (*j).first );

				// add the template's values to RowColl
				for ( StringColl::iterator iKeys = sharedKeys.begin(); iKeys != sharedKeys.end(); ++iKeys )
				{
					gpstring key = *iKeys;
					gpstring value;
					bool bFound = false;
					for ( ContentColl::iterator k = (*j).second->m_Values.begin(); k != (*j).second->m_Values.end(); ++k )
					{
						if ( (*k).first.same_no_case( *iKeys ) )
						{
							value = (*k).second;
							bFound = true;
							break;
						}
					}
					if ( bFound )
					{
						(*j).second->m_Values.erase( k );
					}

					bool bFoundRow = false;
					
					for ( RowColl::iterator m = Rows.begin(); m != Rows.end(); ++m )
					{
						if ( (*m).first.same_no_case( key ) || 
							 gpstring( (*m).first ).append( "$" ).same_no_case( key ) ||
							 gpstring( key ).append( "$" ).same_no_case( (*m).first ) ) // keys match
						{
							if ( (*m).second.size() > columnCount )
							{
								// this column has already been used by me.  probably a duplicate key.
								continue;
							}
							else
							{
								// found the row, set the value
								int temp = columnCount - (*m).second.size();
								for ( int dummy = 0; dummy < temp; ++dummy )
								{
									(*m).second.push_back( gpstring() );
								}
								(*m).second.push_back( value );

								bFoundRow = true;
								break;
							}
						}
					}

					if ( !bFoundRow )
					{
						// add a new row for this key, and set the value
						StringColl temp;
						for ( int dummy = 0; dummy < columnCount; ++dummy )
						{
							temp.push_back( gpstring() );
						}
						temp.push_back( value );

						Rows.push_back( Row( key, temp ) );
					}
				}

				++columnCount;
			}
		}}
#else
		{for ( TemplateMap::iterator j = gTemplates.begin(); j != gTemplates.end(); ++j )
		{
			if ( (*i).same_no_case( (*j).second->m_GasFilename ) )
			{
				// just check to see if this template matches the correct specialized case (if needed)
				/*
				if ( Specialized.size() > 0 )
				{
					bool flag = false;
					for ( StringColl::iterator k = Specialized.begin(); k != Specialized.end(); ++k )
					{
						if ( (*k).same_no_case( (*j).second->m_Specializes ) )
						{
							flag = true;
							break;
						}
					}
					if ( !flag )
					{
						continue;
					}
				}
				*/

				// add the template's name to the header
				Rows[0].second.push_back( (*j).first );

				// add the template's values to RowColl
				for ( ContentColl::iterator k = (*j).second->m_Values.begin(); k != (*j).second->m_Values.end(); ++k )
				{
					bool bFoundRow = false;
					
					for ( RowColl::iterator m = Rows.begin(); m != Rows.end(); ++m )
					{
						if ( (*m).first.same_no_case( (*k).first ) || 
							 gpstring( (*m).first ).append( "$" ).same_no_case( (*k).first ) ||
							 gpstring( (*k).first ).append( "$" ).same_no_case( (*m).first ) ) // keys match
						{
							if ( (*m).second.size() > columnCount )
							{
								// this column has already been used by me.  probably a duplicate key.
								continue;
							}
							else
							{
								// found the row, set the value
								int temp = columnCount - (*m).second.size();
								for ( int dummy = 0; dummy < temp; ++dummy )
								{
									(*m).second.push_back( gpstring() );
								}
								(*m).second.push_back( (*k).second );

								bFoundRow = true;
								break;
							}
						}
					}

					if ( !bFoundRow )
					{
						// add a new row for this key, and set the value
						StringColl temp;
						for ( int dummy = 0; dummy < columnCount; ++dummy )
						{
							temp.push_back( gpstring() );
						}
						temp.push_back( (*k).second );

						Rows.push_back( Row( (*k).first, temp ) );
					}
				}

				++columnCount;
			}
		}}
#endif
		// write out this gas file block
		if ( Rows.size() > 1 )
		{
			if ( !bFlipTable )
			{
				fprintf( pFile, "[FILE=%s]\n", (*i).c_str() );
						
				{for ( RowColl::iterator j = Rows.begin(); j != Rows.end(); ++j )
				{
					fwrite( (*j).first.c_str(), (*j).first.size(), 1, pFile );
					fwrite( TableDelimeter, 1, 1, pFile );
					for ( StringColl::iterator k = (*j).second.begin(); k != (*j).second.end(); ++k )
					{
						fwrite( (*k).c_str(), (*k).size(), 1, pFile );				
						fwrite( TableDelimeter, 1, 1, pFile );
					}
					fwrite( "\n", 1, 1, pFile );
				}}
				fwrite( "\n\n", 2, 1, pFile );
			}
			else
			{
#if !UGLYHO
				fprintf( pFile, "[FILE=%s]*\n", (*i).c_str() );

				int colCount = 0;
				{for ( RowColl::iterator j = Rows.begin(); j != Rows.end(); ++j )
				{
					fwrite( (*j).first.c_str(), (*j).first.size(), 1, pFile );
					fwrite( TableDelimeter, 1, 1, pFile );

					if ( (*j).second.size() > colCount )
					{
						colCount = (*j).second.size();
					}
				}}
				fwrite( "\n", 1, 1, pFile );
				
				for ( int col = 0; col < colCount; ++col )
				{
					for ( RowColl::iterator j = Rows.begin(); j != Rows.end(); ++j )
					{
						if ( col < (*j).second.size() )
						{
							fwrite( (*j).second[ col ].c_str(), (*j).second[ col ].size(), 1, pFile );				
						}
						fwrite( TableDelimeter, 1, 1, pFile );
					}
					fwrite( "\n", 1, 1, pFile );
				}
				fwrite( "\n\n", 2, 1, pFile );
#else
				int colCount = 0;
				{for ( RowColl::iterator j = Rows.begin(); j != Rows.end(); ++j )
				{
					if ( (*j).second.size() > colCount )
					{
						colCount = (*j).second.size();
					}
				}}

				for ( int col = 0; col < colCount; ++col )
				{
					fprintf( pFile, "[FILE=%s]*%s", i->c_str(), TableDelimeter );
					for ( RowColl::iterator j = Rows.begin(); j != Rows.end(); ++j )
					{
						if ( col < (*j).second.size() )
						{
							fprintf( pFile, "%s", j->second[ col ].c_str() );
						}
						fprintf( pFile, TableDelimeter );
					}
					fprintf( pFile, "\n" );
				}
#endif
			}
		}
		SendMessage( hwndProgressBar, PBM_STEPIT, 0, 0 );
	}}

	fclose( pFile );

	delete pFuelSys;
	delete pMasterFileMgr;

	SetDlgItemText( hDlg, IDC_STATUS, "Ready" );
	SendMessage( hwndProgressBar, PBM_SETPOS, (WPARAM) 0, 0 );
	EnableDialogControls( TRUE );

	// Refresh dialog
	SaveSettings( hDlg );
	LoadSettings( hDlg );
}

