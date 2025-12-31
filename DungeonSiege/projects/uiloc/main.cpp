/*=====================================================================

	Name	:	UILoc
	Date	:	Jan, 2001
	Author	:	Adam Swensen

	Purpose	:	(1) Packs ui strings from gas into a DLL
				(2) Unpacks	localized ui strings from DLL back into gas

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

const char * DLL_NAME = "UI.DLL";

#define TOOLTIP_ID_OFFSET 0xEA60

using namespace std;
using namespace FileSys;

// Pack dll
typedef std::map< UINT16, gpstring > UiMap;
UiMap UiText;

// Unpack dll
struct UiEntry
{
	UiEntry( gpstring s1, gpstring s2 )  { sInterface = s1; sWindow = s2; }
	gpstring sInterface;
	gpstring sWindow;
};
typedef std::map< UINT16, UiEntry > UiEntryMap;
UiEntryMap UiEntries;


void FindUiText( bool bClearUiMap = false )
{
	UINT16 newInterfaceId = 1;
	UINT16 newTooltipId = TOOLTIP_ID_OFFSET + 1;

	TextFuelDB * localDb = gFuelSys.AddTextDb( "localize" );
	localDb->Init( FuelDB::OPTION_WRITE | FuelDB::OPTION_JIT_READ, FileSys::GetCurrentDirectory(), FileSys::GetCurrentDirectory() );
	FuelHandle fhRoot( "::localize" );
	if ( !fhRoot )
	{
		gperror( "Fuel was not initialized." );
		return;
	}

	FuelHandle fhIdMap( "::localize:localization_ui_map" );
	if ( !fhIdMap.IsValid() )
	{
		fhIdMap = fhRoot->CreateChildBlock( "localization_ui_map", "localization_ui_map.gas" );		
		fhIdMap->CreateChildBlock( "interfaces" );
		fhIdMap->CreateChildBlock( "tooltips" );
	}
	else if ( bClearUiMap )
	{
		fhIdMap->ClearKeysAndChildren();
		fhIdMap->CreateChildBlock( "interfaces" );
		fhIdMap->CreateChildBlock( "tooltips" );
	}
	else
	{
		FuelHandle fhInterfaces = fhIdMap->GetChildBlock( "interfaces" );
		FuelHandleList fhlInterfaces = fhInterfaces->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = fhlInterfaces.begin(); i != fhlInterfaces.end(); ++i )
		{
			UINT16 id = (UINT16)atoi( (*i)->GetType() );
			if ( id >= newInterfaceId )
			{
				newInterfaceId = id + 1;
			}
		}
		FuelHandle fhTooltips = fhIdMap->GetChildBlock( "tooltips" );
		FuelHandleList fhlTooltips = fhTooltips->ListChildBlocks( 1 );
		for ( i = fhlTooltips.begin(); i != fhlTooltips.end(); ++i )
		{
			if ( (*i)->FindFirstKeyAndValue() )
			{
				gpstring key, value;
				while ( (*i)->GetNextKeyAndValue( key, value ) )
				{
					UINT16 id = (UINT16)atoi( value.c_str() );
					if ( id >= newTooltipId )
					{
						newTooltipId = id + 1;
					}
				}
			}
		}
	}

	FuelHandle fhIdInterfaces = fhIdMap->GetChildBlock( "interfaces" );
	gpassert( fhIdInterfaces );
	FuelHandle fhIdTooltips = fhIdMap->GetChildBlock( "tooltips" );
	gpassert( fhIdTooltips );

	FuelHandle fhUi( "ui:interfaces" );
	if ( fhUi.IsValid() )
	{
		FuelHandleList fhlInterfaceGroups = fhUi->ListChildBlocks( 1 );
		for ( FuelHandleList::iterator iGroups = fhlInterfaceGroups.begin(); iGroups != fhlInterfaceGroups.end(); ++iGroups )
		{
			FuelHandleList fhlInterfaces = (*iGroups)->ListChildBlocks( 1 );
			for ( FuelHandleList::iterator i = fhlInterfaces.begin(); i != fhlInterfaces.end(); ++i )
			{
//				std::cout << (*i)->GetAddress() << endl;	// $$$ add quiet mode flag to app... -sb

				FuelHandle fhIdInterface;
				UINT16 interfaceId;
				UINT16 newWindowId;

				FuelHandle fhInterface = (*i);
				
				if ( fhInterface->IsDirectory() )
				{
					fhInterface = fhInterface->GetChildBlock( fhInterface->GetName() );
				}
				if ( !fhInterface )
				{
					continue;
				}

				FuelHandleList fhlWindows = fhInterface->ListChildBlocks( -1 );
				for ( FuelHandleList::iterator j = fhlWindows.begin(); j != fhlWindows.end(); ++j )
				{
					if ( (*j)->FindFirstKeyAndValue() )
					{
						gpstring key, value;
						while ( (*j)->GetNextKeyAndValue( key, value ) )
						{
							if ( key.same_no_case( "text" ) && !value.empty() )
							{
								if ( !fhIdInterface )
								{
									fhIdInterface = fhIdInterfaces->GetChildBlock( (*i)->GetName() );
									if ( !fhIdInterface.IsValid() )
									{
										interfaceId = newInterfaceId;
										newWindowId = (interfaceId << 8) + 1;
										fhIdInterface = fhIdInterfaces->CreateChildBlock( (*i)->GetName() );
										fhIdInterface->SetType( gpstringf( "%d", interfaceId ) );
										++newInterfaceId;
									}
									else
									{
										interfaceId = (UINT16)atoi( fhIdInterface->GetType() );
										newWindowId = (interfaceId << 8) + 1;
										if ( fhIdInterface->FindFirstKeyAndValue() )
										{
											gpstring key, value;
											while ( fhIdInterface->GetNextKeyAndValue( key, value ) )
											{
												UINT16 id = (UINT16)atoi( value );
												if ( id >= newWindowId )
												{
													newWindowId = id + 1;
												}
											}
										}
									}

									if ( interfaceId >= 230 )
									{
										gperrorf(( "Interface limit reached.\n" ));
									}
								
									UiText.insert( UiMap::value_type( interfaceId << 8, gpstringf( "/* %s */", (*i)->GetName() ) ) );
								}

								int windowId;
								if ( !fhIdInterface->Get( (*j)->GetName(), windowId ) )
								{
									windowId = newWindowId;
									++newWindowId;
								}

								if ( (windowId & 0xff) >= 256 )
								{
									gperrorf(( "Window limit in interface \"%s\" was reached.\n", (*i)->GetName() ));
								}

								if ( value.size() > 4000 )
								{
									gperrorf(( "Found string exceeding 4000 characters in interface \"%s\".", (*i)->GetName() ));
								}

								fhIdInterface->Set( (*j)->GetName(), windowId );
								UiText.insert( UiMap::value_type( windowId, value ) );
							}
						}
					}
				}
			}
		}
	}

	FuelHandle fhTooltips( "ui:tooltips" );
	if ( fhTooltips )
	{
		UiText.insert( UiMap::value_type( TOOLTIP_ID_OFFSET, "/* tooltips */" ) );

		FuelHandleList fhlInterfaces = fhTooltips->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = fhlInterfaces.begin(); i != fhlInterfaces.end(); ++i )
		{
			FuelHandle fhIdInterface = fhIdTooltips->GetChildBlock( (*i)->GetName() );
			if ( !fhIdInterface )
			{
				fhIdInterface = fhIdTooltips->CreateChildBlock( (*i)->GetName() );
			}

			FuelHandle fhTip = (*i)->GetChildBlock( "tips" );
			if ( fhTip )
			{
				if ( fhTip->FindFirstKeyAndValue() )
				{
					gpstring key, value;
					while ( fhTip->GetNextKeyAndValue( key, value ) )
					{
						if ( !value.empty() )
						{
							if ( value[ 0 ] == '@' )
							{
								FuelHandle hText = (*i)->GetChildBlock( "text" );
								if ( hText )
								{
									hText->Get( value.right( value.size() - 1 ).c_str(), value );
									if ( value.empty() )
									{
										continue;
									}
								}
							}

							int tipId;
							if ( !fhIdInterface->Get( key, tipId ) )
							{
								tipId = newTooltipId;
								fhIdInterface->Set( key, tipId );
								++newTooltipId;
							}
							
							if ( value.size() > 4000 )
							{
								gperrorf(( "Found tooltip string exceeding 4000 characters for window \"%s\".", key.c_str() ));
							}

							UiText.insert( UiMap::value_type( tipId, value ) );
						}
					}
				}
			}
			else
			{
				fhTip = (*i)->GetChildBlock( "help" );
				if ( fhTip )
				{
					if ( fhTip->FindFirstKeyAndValue() )
					{
						gpstring key, value;
						while ( fhTip->GetNextKeyAndValue( key, value ) )
						{
							if ( !value.empty() )
							{
								if ( value[ 0 ] == '@' )
								{
									FuelHandle hText = (*i)->GetChildBlock( "text" );
									if ( hText )
									{
										hText->Get( value.right( value.size() - 1 ).c_str(), value );
										if ( value.empty() )
										{
											continue;
										}
									}
								}

								int tipId;
								if ( !fhIdInterface->Get( key, tipId ) )
								{
									tipId = newTooltipId;
									fhIdInterface->Set( key, tipId );
									++newTooltipId;
								}
								
								if ( value.size() > 4000 )
								{
									gperrorf(( "Found tooltip string exceeding 4000 characters for window \"%s\".", key.c_str() ));
								}

								UiText.insert( UiMap::value_type( tipId, value ) );
							}
						}
					}
				}
			}
		}
	}

	localDb->SaveChanges();
}


void PackUiDLL( gpstring fuelPath, bool bClearUiMap = false )
{
	// Initialize file manager for fuel
	FileSys::MasterFileMgr masterFileMgr;
	FileSys::PathFileMgr * pPathFileMgr		= new PathFileMgr( fuelPath );
	masterFileMgr.AddFileMgr( pPathFileMgr, true );

	// Create our fuel database
	FuelSys fuelSys;
	TextFuelDB * pFuelDB = fuelSys.AddTextDb( "default" );
	pFuelDB->Init( FuelDB::OPTION_JIT_READ | FuelDB::OPTION_READ, fuelPath );

	// Retrieve text from ui
	FindUiText( bClearUiMap );

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
	{for( UiMap::iterator i = UiText.begin(); i != UiText.end(); ++i )
	{
		if ( stringtool::ContainsDevText( i->second ) )
		{
			gperrorf(( "Found suspicious looking dev text in the string: \"%s\"", i->second.c_str() ));
		}

		fprintf( rcFile, "\t%d, \"%s\"\n", i->first, i->second.c_str() );
	}}
	fputs( "}\n\n", rcFile );

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

	std::cout << "UI word count : " << UiText.size();
}


void UnpackUiDLL( gpstring dllName, gpstring fuelPath )
{
	HMODULE hDLL = LoadLibraryEx( dllName, NULL, LOAD_LIBRARY_AS_DATAFILE );
	if ( !hDLL )
	{
		gperrorf(( "The specified DLL '%s' could not be loaded, error is '%s'.", dllName.c_str(), stringtool::GetLastErrorText().c_str() ));
		return;
	}

	LocStringExtractor extractor;
	if ( !extractor.Init( hDLL, false ) )
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
	FileSys::MasterFileMgr masterFileMgr;
	FileSys::PathFileMgr * pPathFileMgr		= new PathFileMgr( fuelPath );
	masterFileMgr.AddFileMgr( pPathFileMgr, true );

	// Create our fuel database
	FuelSys fuelSys;
	TextFuelDB * localDb = fuelSys.AddTextDb( "localize" );
	localDb->Init( FuelDB::OPTION_WRITE | FuelDB::OPTION_JIT_READ, fuelPath, fuelPath );
	FuelHandle fhRoot( "::localize" );
	if ( !fhRoot )
	{
		gperror( "Fuel was not initialized." );
		return;
	}

	// Read ui localization map
	FuelHandle fhIdMap( "::localize:localization_ui_map" );
	if ( !fhIdMap )
	{
		gperror( "The ui localization map was not found." );
		return;
	}

	FuelHandle fhIdInterfaces = fhIdMap->GetChildBlock( "interfaces" );
	gpassert( fhIdInterfaces );	
	FuelHandleList fhlInterfaces = fhIdInterfaces->ListChildBlocks( 1 );
	{for ( FuelHandleList::iterator i = fhlInterfaces.begin(); i != fhlInterfaces.end(); ++i )
	{
		if ( (*i)->FindFirstKeyAndValue() )
		{
			gpstring key, value;
			while ( (*i)->GetNextKeyAndValue( key, value ) )
			{
				int id;
				stringtool::Get( value, id );
				UiEntries.insert( UiEntryMap::value_type( id, UiEntry( (*i)->GetName(), key ) ) );
			}
		}
	}}

	FuelHandle fhIdTooltips = fhIdMap->GetChildBlock( "tooltips" );
	gpassert( fhIdTooltips );
	fhlInterfaces = fhIdTooltips->ListChildBlocks( 1 );
	{for ( FuelHandleList::iterator i = fhlInterfaces.begin(); i != fhlInterfaces.end(); ++i )
	{
		if ( (*i)->FindFirstKeyAndValue() )
		{
			gpstring key, value;
			while ( (*i)->GetNextKeyAndValue( key, value ) )
			{
				int id;
				stringtool::Get( value, id );
				UiEntries.insert( UiEntryMap::value_type( id, UiEntry( (*i)->GetName(), key ) ) );
			}
		}
	}}

	// Read strings from dll
	using LocStringExtractor::StringColl;
	const StringColl& StringText = LangText.begin()->second;
	LANGID langId = LangText.begin()->first;

	FuelHandleList fhlLanguage;
	fhRoot->ListChildBlocks( 1, gpstringf( "0x%04X", langId ).c_str(), "ui", fhlLanguage );
	{for ( FuelHandleList::iterator i = fhlLanguage.begin(); i != fhlLanguage.end(); ++i )
	{
		fhRoot->DestroyChildBlock( *i, false );
	}}

	FuelHandle fhLanguage = fhRoot->CreateChildBlock( "ui", "dscore-ui.gas" );
	if ( !fhLanguage )
	{
		gperror( "Unable to create language fuel block!" );
		return;
	}
	fhLanguage->SetType( gpstringf( "0x%04X", langId ) );

	{for ( StringColl::const_iterator i = StringText.begin(); i != StringText.end(); ++i )
	{
		UiEntryMap::iterator findEntry = UiEntries.find( (*i).first );
		if ( findEntry != UiEntries.end() )
		{
			FuelHandle fhInterface = fhLanguage->GetChildBlock( findEntry->second.sInterface );
			if ( !fhInterface )
			{
				fhInterface = fhLanguage->CreateChildBlock( findEntry->second.sInterface );
			}
			gpassert( fhInterface );
			FuelHandle fhWindow = fhInterface->GetChildBlock( findEntry->second.sWindow );
			if ( !fhWindow )
			{
				fhWindow = fhInterface->CreateChildBlock( findEntry->second.sWindow );
			}
			gpassert( fhWindow );

			if ( i->first < TOOLTIP_ID_OFFSET )
			{
				fhWindow->Set( "text", i->second.m_To );
			}
			else
			{
				fhWindow->Set( "tooltip", i->second.m_To );
			}
		}
	}}

	// Write fuel
	localDb->SaveChanges();
}


int main( int argc, char * argv[] )
{
	if( argc < 2 )
	{
		std::cout << "Usage 1: Pack all ui text from Fuel into UI.DLL" << endl;
		std::cout << "  UILoc <Fuel Path> <clean>" << endl << endl;
		std::cout << "Usage 2: Unpack text from UI.DLL back into Fuel" << endl;
		std::cout << "  UILoc <UI.DLL>" << endl << endl;
		return( 0 );
	}

	// No annoying asserts for errors
	ReportSys::EnableErrorAssert( false );
	gGlobalSink.AddSink( new ReportSys::Win32ConsoleSink, true );

	if ( gpstring( argv[1] ).right( 3 ).same_no_case( "dll" ) )
	{
		UnpackUiDLL( argv[1], FileSys::GetCurrentDirectory() );
	}
	else
	{
		bool bClean = false;
		if ( argc >= 3 )
		{
			if ( gpstring( argv[2] ).same_no_case( "clean" ) )
			{
				bClean = true;
			}
		}
		PackUiDLL( argv[1], bClean );
	}

	return( 0 );
}
