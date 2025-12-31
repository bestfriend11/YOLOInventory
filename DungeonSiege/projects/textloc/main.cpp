/*=====================================================================

	Name	:	TextLoc
	Date	:	Jan, 2001
	Author	:	Adam Swensen

	Purpose	:	Packs game strings from gas into a DLL, extracts
				localized strings from DLL into gas.

---------------------------------------------------------------------*/

#include "gpcore.h"

#include "fuel.h"
#include "fueldb.h"
#include "gpmem.h"
#include "lochelp.h"
#include "filesys.h"
#include "pathfilemgr.h"
#include "masterfilemgr.h"
#include "filesysutils.h"
#include "stringtool.h"
#include "reportsys.h"

#include <shellapi.h>
#include <iostream>

const char * RC_HEADER = "#include <windows.h>\n"
						 "LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US\n"
						 "#pragma code_page(1252)\n\n";

const char * DLL_NAME = "Text.DLL";

using namespace std;
using namespace FileSys;

bool bQuiet = true;

// Pack dll
struct TextEntry
{
	gpstring english;
	gpstring foreign;
};
typedef std::map< UINT16, TextEntry > TextMap;
TextMap LocText;

void InsertLocString( gpstring english, gpstring foreign )
{
	UINT16 crc = GetCRC16( 0, foreign.c_str(), foreign.size() );

	// warn for bad embedded chars
	size_t pos = foreign.find_first_of( "\n\r\v\t" );
	if ( pos != gpstring::npos )
	{
		gperrorf(( "An illegal escape code (%s) was found in the string : \"%s\"",
				stringtool::EscapeCharactersToTokens( gpstring( 1, foreign[ pos ] ) ).c_str(),
				stringtool::EscapeCharactersToTokens( foreign ).c_str() ));
	}

	// check and warn about suspicious looking names
	if ( stringtool::ContainsDevText( foreign ) )
	{
		gperrorf(( "Found suspicious looking dev text in the string: \"%s\"", english.c_str() ));
	}

	// add string
	bool bDupeCRC = false, bDupeString = false;
	TextMap::iterator i = LocText.find( crc );
	while ( i != LocText.end() )
	{
		if ( foreign.same_with_case( (*i).second.foreign ) )
		{
			// duplicate string
			bDupeString = true;
			break;
		}
		i = LocText.find( ++crc );
		bDupeCRC = true;
	}
	if ( !bDupeString )
	{
		if ( bDupeCRC )
		{
			//gpwarningf(( "Found Duplicate CRC : \"%s\"", english.c_str() ));
		}
		TextEntry entry;
		entry.english = english;
		entry.foreign = foreign;
		LocText.insert( TextMap::value_type( crc, entry ) );
	}
}


void ParseFuelForText( FuelHandle hFuel, int recurse = 0 )
{
	// $ early bailout on special dirs (optimization, makes loc tool run lots faster)
	if ( hFuel->IsDirectory() )
	{
		if ( same_no_case( hFuel->GetName(), "terrain_nodes" ) )
		{
			return;
		}
		if ( same_no_case( hFuel->GetName(), "editor" ) )
		{
			return;
		}
	}

	if ( (recurse < 4) && !bQuiet )
	{
		std::cout << hFuel->GetAddress() << endl;
	}

	if ( !hFuel->IsDirectory() && !::same_no_case( hFuel->GetName(), "test_" ) )
	{
		if ( hFuel->FindFirstKeyAndValue() )
		{
			gpstring key, value, adjusted;
			if ( gpstring( "skill*" ).same_no_case( hFuel->GetName() ) )
			{
				eFuelValueProperty fvp;
				while ( (fvp = hFuel->GetNextKeyAndValue( key, value )) != FVP_NONE )
				{
					if ( key.same_no_case( "name" )
						|| key.same_no_case( "class" ) )
					{
						if ( !(fvp & FVP_QUOTED_STRING) )
						{
							gperrorf(( "No quotes around %s entry (value = '%s') found at '%s'\n", key.c_str(), value.c_str(), hFuel->GetAddress().c_str() ));
						}

						if ( value.size() > 4000 )
						{
							gperrorf(( "Found %s entry larger than 4000 characters (value = '%s') at '%s'\n", key.c_str(), value.c_str(), hFuel->GetAddress().c_str() ));
						}

						InsertLocString( value, value );
					}
				}
			}
			else
			{
				bool bUserEdTooltips = false;
				if ( hFuel->GetParent() && gpstring( "user_education_tooltips" ).same_no_case( hFuel->GetParent()->GetName() ) )
				{
					bUserEdTooltips = true;
				}

				eFuelValueProperty fvp;
				while ( (fvp = hFuel->GetNextKeyAndValue( key, value )) != FVP_NONE )
				{
					if ( key.same_no_case( "screen_name" ) )
					{
						if ( !(fvp & FVP_QUOTED_STRING) )
						{
							gperrorf(( "No quotes around %s entry (value = '%s') found at '%s'\n", key.c_str(), value.c_str(), hFuel->GetAddress().c_str() ));
						}

						if ( value.size() > 4000 )
						{
							gperrorf(( "Found %s entry larger than 4000 characters (value = '%s') at '%s'\n", key.c_str(), value.c_str(), hFuel->GetAddress().c_str() ));
						}

						if ( hFuel->GetParent() && gpstring( hFuel->GetParent()->GetName() ).same_no_case( "modifiers" ) )
						{
							adjusted = "<modifier>";
							adjusted += value;
						}
						else 
						{
							adjusted = value;
						}

						InsertLocString( value, adjusted );
					}
					else if ( key.same_no_case( "screen_text" ) || key.same_no_case( "button_1_text" ) || key.same_no_case( "button_2_text" ) || key.same_no_case( "speaker" ) )
					{
						if ( !(fvp & FVP_QUOTED_STRING) )
						{
							gperrorf(( "No quotes around %s entry (value = '%s') found at '%s'\n", key.c_str(), value.c_str(), hFuel->GetAddress().c_str() ));
						}						

						if ( value.size() > 4000 )
						{
							gperrorf(( "Found %s entry larger than 4000 characters (value = '%s') at '%s'\n", key.c_str(), value.c_str(), hFuel->GetAddress().c_str() ));
						}

						InsertLocString( value, value );
					}
					else if ( key.same_no_case( "description" ) || key.same_no_case( "caster_description" ) || key.same_no_case( "caster_state_name" ) )
					{
						if ( fvp & FVP_QUOTED_STRING )
						{
							if ( value.size() > 4000 )
							{
								gperrorf(( "Found %s entry larger than 4000 characters (value = '%s') at '%s'\n", key.c_str(), value.c_str(), hFuel->GetAddress().c_str() ));
							}

							InsertLocString( value, value );
						}
					}
					else if ( bUserEdTooltips && (fvp & FVP_QUOTED_STRING) )
					{
						if ( value.size() > 4000 )
						{
							gperrorf(( "Found user_education_tooltips entry larger than 4000 characters (value = '%s') at '%s'\n", value.c_str(), hFuel->GetAddress().c_str() ));
						}
						InsertLocString( value, value );
					}
					else if ( key.same_no_case( "screen_class" ) )
					{
						if ( !(fvp & FVP_QUOTED_STRING) )
						{
							gperrorf(( "No quotes around %s entry (value = '%s') found at '%s'\n", key.c_str(), value.c_str(), hFuel->GetAddress().c_str() ));
						}						

						if ( value.size() > 4000 )
						{
							gperrorf(( "Found %s entry larger than 4000 characters (value = '%s') at '%s'\n", key.c_str(), value.c_str(), hFuel->GetAddress().c_str() ));
						}

						InsertLocString( value, value );
					}
				}
			}
		}
	}
	else
	{
		// see if we're a dev-only map and bail if so
		FuelHandle child = hFuel->GetChildBlock( "map" );
		if ( child && same_no_case( child->GetType(), "map" ) && child->GetBool( "dev_only", false ) )
		{
			return;
		}
	}

	FuelHandleList fhlFuel = hFuel->ListChildBlocks();
	for ( FuelHandleList::iterator i = fhlFuel.begin(); i != fhlFuel.end(); ++i )
	{
		ParseFuelForText( *i, recurse + 1 );
		if ( (*i)->IsDirectory() )
		{
			(*i).Unload();
		}
	}
}


void PackTextDLL( gpstring fuelPath )
{
	// Initialize file manager for fuel
	FileSys::MasterFileMgr * pMasterFileMgr	= new MasterFileMgr();
	FileSys::PathFileMgr * pPathFileMgr		= new PathFileMgr( fuelPath );
	pMasterFileMgr->AddFileMgr( pPathFileMgr, true );

	// Create our fuel database
	FuelSys * pFuelSys = new FuelSys;
	FuelDB * pFuelDB = pFuelSys->AddTextDb( "default" );
	pFuelDB->Init( FuelDB::OPTION_JIT_READ | FuelDB::OPTION_READ | FuelDB::OPTION_NO_READ_DEV_BLOCKS, fuelPath );

	FuelHandle fhRoot( "root" );
	if ( !fhRoot )
	{
		gperror(( "The fuel path '%s' was invalid.", fuelPath ));
		delete pFuelSys;
		delete pMasterFileMgr;
		return;
	}

	// Find all localizable text
	ParseFuelForText( fhRoot );

	delete pFuelSys;
	delete pMasterFileMgr;

	// Make RC file
	FILE *rcFile = fopen( "temp.rc", "w" );
	if ( !rcFile )
	{
		gperror( "Unable to open file \"temp.rc\" for writing\n" );
		return;
	}
	fputs( RC_HEADER, rcFile );

	// Make string table
	fputs( "STRINGTABLE\n"
		   "{\n", rcFile );
	{for( TextMap::iterator i = LocText.begin(); i != LocText.end(); ++i )
	{
		fprintf( rcFile, "\t%d, \"%s\"\n", i->first, i->second.foreign.c_str() );
	}}
	fputs( "}\n\n", rcFile );

	// Make English lookup table (so LocStudio won't notice)
	fputs( "1 RCDATA\n"
		   "{\n", rcFile );
	{for( TextMap::iterator i = LocText.begin(); i != LocText.end(); ++i )
	{
		fprintf( rcFile, "\t%d, \"%s\\0\",\n", i->first, i->second.english.c_str() );
	}}
	fputs( "\t0\n"
		   "}\n\n", rcFile );

	// Done
	fclose( rcFile );

	// Make struct
	SHELLEXECUTEINFO info;
	ZeroObject( info );
	info.cbSize = sizeof( info );
	info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
	info.lpVerb = "open";
	info.lpDirectory = ".";

	// Make RES struct
	info.lpFile = "rc";
	info.lpParameters = "temp.rc";

	// Make RES file
	if ( !ShellExecuteEx( &info ) )
	{
		gperror( "Could not execute RC\n" );
		return;
	}
	WaitForSingleObject( info.hProcess, INFINITE );
	CloseHandle( info.hProcess );

	// Make LINK struct
	gpstring params;
	info.lpFile = "link";
	info.lpParameters = params.assignf( "temp.res /NOLOGO /FIXED /DLL /OUT:%s /NOENTRY /MACHINE:IX86", DLL_NAME );

	// Make DLL
	if ( !ShellExecuteEx( &info ) )
	{
		gperror( "Could not execute LINK\n" );
		return;
	}
	WaitForSingleObject( info.hProcess, INFINITE );
	CloseHandle( info.hProcess );
}


void UnpackTextDLL( gpstring dllName, gpstring fuelPath )
{
	HMODULE hDLL = LoadLibraryEx( dllName, NULL, LOAD_LIBRARY_AS_DATAFILE );
	if ( !hDLL )
	{
		gperrorf(( "The specified DLL '%s' could not be loaded, error is '%s'.", dllName.c_str(), stringtool::GetLastErrorText().c_str() ));
		return;
	}

	LocStringExtractor extractor;
	if ( !extractor.Init( hDLL ) )
	{
		gperrorf(( "Unable to extract strings from DLL '%s'.", dllName.c_str() ));
		return;
	}

	::FreeLibrary( hDLL );

	using LocStringExtractor::LangColl;
	const LangColl& LangText = extractor.GetStrings();
	if ( LangText.empty() )
	{
		gperrorf(( "No strings were loaded from the specified DLL '%s'.", dllName.c_str() ));
		return;
	}

	// Initialize file manager for fuel
	stringtool::AppendTrailingBackslash( fuelPath );
	FileSys::MasterFileMgr * pMasterFileMgr	= new MasterFileMgr();
	FileSys::PathFileMgr * pPathFileMgr		= new PathFileMgr( fuelPath );
	pMasterFileMgr->AddFileMgr( pPathFileMgr, true );

	// Clear out any old file
	::DeleteFile( fuelPath + "dscore-text.gas" );

	// Create our fuel database
	FuelSys * pFuelSys = new FuelSys;
	TextFuelDB * localDb = gFuelSys.AddTextDb( "localize" );
	localDb->Init( FuelDB::OPTION_WRITE | FuelDB::OPTION_JIT_READ, fuelPath, fuelPath );
	FuelHandle fhRoot( "::localize" );
	if ( !fhRoot )
	{
		gperror( "Fuel was not initialized." );
		return;
	}

	using LocStringExtractor::StringColl;
	const StringColl& StringText = LangText.begin()->second;
	LANGID langId = LangText.begin()->first;

	FuelHandle fhLanguage = fhRoot->CreateChildBlock( "text", "dscore-text.gas" );
	if ( !fhLanguage )
	{
		gperror( "Unable to create language fuel block!" );
		return;
	}
	fhLanguage->SetType( gpstringf( "0x%04X", langId ) );

	for ( StringColl::const_iterator i = StringText.begin(); i != StringText.end(); ++i )
	{
		gpstring entry = gpstringf( "0x%04X", (*i).first );
		FuelHandle fhEntry = fhLanguage->CreateChildBlock( entry );
		gpassert( fhEntry );
		
		fhEntry->Set( "from", "\"" + i->second.m_From + "\"" );
		fhEntry->Set( "to", i->second.m_To );
	}

	// Write fuel
	localDb->Save();

	delete pFuelSys;
	delete pMasterFileMgr;
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


int main( int argc, char * argv[] )
{
	if( argc < 2 )
	{
		std::cout << "Usage 1: Extract all text from Fuel into Text.DLL" << endl;
		std::cout << "  TextLoc <Fuel Path> /show" << endl << endl;
		std::cout << "Usage 2: Extract text from Text.DLL back into Fuel" << endl;
		std::cout << "  TextLoc <Text.DLL> /show" << endl << endl;
		return( 0 );
	}

	if ( (argc > 2) && gpstring( argv[2] ).same_no_case( "/show" ) )
	{
		bQuiet = false;
	}

	// No annoying asserts for errors
	ReportSys::EnableErrorAssert( false );
	gGlobalSink.AddSink( new ReportSys::Win32ConsoleSink, true );

	if ( gpstring( argv[1] ).right( 3 ).same_no_case( "dll" ) )
	{
		UnpackTextDLL( argv[1], FileSys::GetCurrentDirectory() );
	}
	else
	{
		PackTextDLL( argv[1] );
	}

	int errors = ReportSys::GetErrorCount();
	if ( errors > 0 )
	{
		gperrorf(( "\n** There were %d errors during run.\n", errors ));
	}

	return( 0 );
}
