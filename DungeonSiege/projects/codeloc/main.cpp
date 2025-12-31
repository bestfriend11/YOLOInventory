/*=====================================================================

	Name	:	CodeLoc
	Date	:	Jan, 2001
	Author	:	Adam Swensen

	Purpose	:	Exports tagged strings in game code to an external dll
				for localization.

---------------------------------------------------------------------*/

#include "gpcore.h"

#include "fueldb.h"
#include "gpmem.h"
#include "filesys.h"
#include "pathfilemgr.h"
#include "masterfilemgr.h"
#include "filesysutils.h"
#include "stringtool.h"
#include "reportsys.h"
#include "stdhelp.h"

#include <shellapi.h>
#include <iostream>

const char * RC_HEADER = "#include <windows.h>\n"
						 "LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US\n"
						 "#pragma code_page(1252)\n\n";

using namespace std;
using namespace FileSys;

typedef std::vector< gpstring > StringColl;
StringColl cppFiles;

typedef std::map< UINT16, gpstring > MsgMap;
MsgMap Msgs;

bool FindFiles( AutoFindHandle & hFindRoot )
{
	FindData TempFindData;
	AutoFindHandle hFindDirectories;

	hFindDirectories.Find( hFindRoot, "*.cpp", FINDFILTER_FILES );
	if( hFindDirectories )
	{
		while( hFindDirectories.GetNext( TempFindData, true ) )
		{
			for ( StringColl::iterator i = cppFiles.begin(); i != cppFiles.end(); ++i )
			{
				if ( i->compare_no_case( TempFindData.m_Name ) > 0 )
				{
					cppFiles.insert( i, TempFindData.m_Name );
					break;
				}
			}
			if ( i == cppFiles.end() )
			{
				cppFiles.push_back( TempFindData.m_Name );
			}
		}
	}

	hFindDirectories.Find( hFindRoot, "*", FINDFILTER_DIRECTORIES );
	if( hFindDirectories )
	{
		gpstring sDirName;
		while( hFindDirectories.GetNext( sDirName, true ) )
		{
			if ( !DoesFileExist( sDirName + "\\tattoo.loc" ) )
			{
				continue;
			}

			AutoFindHandle hRecurseDir( ( sDirName + "\\*" ).c_str() );
			if( hRecurseDir )
			{
				FindFiles( hRecurseDir );
			}
		}
	}

	return( true );
}


void AddString( gpstring msgString )
{
	if ( msgString.empty() )
	{
		return;
	}

	// parse out escape codes
	int posCode = msgString.find( "\\" );
	while ( posCode != gpstring::npos )
	{
		if ( (posCode + 1) > (msgString.size() - 1) )
		{
			break;
		}
		if ( msgString[posCode+1] == '\"' )
		{
			msgString.replace( posCode, 2, "\"\"" );
		}
		else if ( msgString[posCode+1] == 'n' )
		{
			msgString.erase( posCode, 2 );
			msgString.insert( posCode, "\\012" );
		}
		else if ( msgString[posCode+1] != '\\' )
		{
			gperrorf(( "An illegal escape code (\\%c) was found in the string: \"%s\"", msgString[posCode+1], msgString.c_str() ));
		}
		posCode = msgString.find( "\\", posCode+2 );
	}

	UINT16 crc = GetCRC16( 0, msgString.c_str(), msgString.size() );
	if ( crc < 100 )
	{
		crc += 100;
	}

	// add string to the msg map
	bool bDupeCRC = false;
	bool bDupeString = false;
	MsgMap::iterator j = Msgs.find( crc );
	while ( j != Msgs.end() )
	{
		if ( msgString.same_with_case( (*j).second ) )
		{
			// duplicate string
			bDupeString = true;
			break;
		}
		j = Msgs.find( ++crc );
		bDupeCRC = true;
	}
	if ( !bDupeString )
	{
		// check and warn about suspicious looking names
		if ( stringtool::ContainsDevText( msgString ) )
		{
			gperrorf(( "Found suspicious looking dev text in the string: \"%s\"", msgString.c_str() ));
		}

		if ( bDupeCRC )
		{
			//gpwarning( gpstringf( "Found Duplicate CRC : \"%s\"", msgString.c_str() ) );
		}
		Msgs.insert( MsgMap::value_type( crc, msgString ) );
	}
	msgString.clear();
}


int main( int argc, char * argv[] )
{
	if( argc < 3 )
	{
		std::cout << "Usage:" << endl;
		std::cout << "  Codeloc <code path> <export dll> /show" << endl << endl;
		std::cout << "Example:" << endl;
		std::cout << "  Codeloc x:\\codepath\\ makedll.dll /show" << endl << endl;
		return( 0 );
	}

	bool bQuiet = true;
	if ( (argc > 3) && gpstring( argv[3] ).same_no_case( "/show" ) )
	{
		bQuiet = false;
	}

	// No annoying asserts for errors
	ReportSys::EnableErrorAssert( false );
	gGlobalSink.AddSink( new ReportSys::Win32ConsoleSink, true );

	gpstring sCodePath( argv[1] );
	gpstring sDLLfile( argv[2] );
	stringtool::AppendTrailingBackslash( sCodePath );

	// Initialize FileSys, MasterFileMgr
	std::auto_ptr< FileSys::IFileMgr > mFileMgr;
	FileSys::MasterPathFileMgrBuilder builder;
	if( !builder.SetHomePath( sCodePath ) )
	{
		std::cout << "The location you specified was invalid: " << sCodePath << endl;
		return( 0 );
	}
	gpstring pathMsg;
	stdx::make_auto_ptr( mFileMgr, builder.CreateMasterFileMgr( &pathMsg ) );

	// Find cpp files
	gpstring sRoot = sCodePath + "*";
	AutoFindHandle hFindRoot( sRoot.c_str(), FINDFILTER_DIRECTORIES );
	std::cout << "\nFinding *.cpp in " << sCodePath << "...\n";
	FindFiles( hFindRoot );
	std::cout << "Processing " << cppFiles.size() << " files...\n\n";

	// Read cpp files, find $MSG$ tags
	{for ( StringColl::iterator i = cppFiles.begin(); i != cppFiles.end(); ++i )
	{
		if ( !bQuiet )
		{
			std::cout << (*i) << endl;
		}

		FileHandle cppFile( (*i) );
		if ( !cppFile )
		{
			std::cout << "  error reading file." << endl;
			continue;
		}

		bool bLookIntoNextLine = false;
		gpstring msgString;
		gpstring sLine;
		int lineNumber = 0;
		while ( cppFile.ReadLine( sLine ) )
		{
			++lineNumber;
			int posMsg = sLine.find( "$MSG$" );

			if ( bLookIntoNextLine )
			{
				int posAttached = 0;
				while ( bLookIntoNextLine )
				{
					// read ahead a bit to see if there is another attached string
					bool bAppending = false;
					int linelen = sLine.size();
					while ( posAttached <= sLine.size() - 1 )
					{
						if ( sLine[posAttached] == '\"' )
						{
							bAppending = true;
							break;
						}
						else if ( !((sLine[posAttached] == ' ') || (sLine[posAttached] == '\t')) )
						{
							bLookIntoNextLine = false;
							break;
						}
						++posAttached;
					}
					if ( (posAttached == sLine.size()) && bLookIntoNextLine )
					{
						posMsg = gpstring::npos;
						break;
					}
					if ( bAppending )
					{
						int posQuote = posAttached;
						int posEndQuote = sLine.find( "\"", posQuote+1 );
						if ( posEndQuote == gpstring::npos )
						{
							gperrorf(( "ending quote not found after $MSG$ tag in file '%s':\n[%d] %s\n", (*i).c_str(), lineNumber, sLine.c_str() ));
							posMsg = sLine.size();
							bLookIntoNextLine = false;
							break;
						}
						while ( sLine[posEndQuote-1] == '\\' )
						{
							if ( (sLine[posEndQuote-2] == '\\') && (sLine[posEndQuote-3] != '\\') )
							{
								break; // false alarm, the quote is not an escape code.
							}
							posEndQuote = sLine.find( "\"", posEndQuote+1 );
							if ( posEndQuote == gpstring::npos )
							{
								gperrorf(( "ending quote not found after $MSG$ tag in file '%s':\n[%d] %s\n", (*i).c_str(), lineNumber, sLine.c_str() ));
								posMsg = sLine.size();
								bLookIntoNextLine = false;
								break;
							}
						}
						if ( (posQuote == gpstring::npos) || (posEndQuote == gpstring::npos) )
						{
							bLookIntoNextLine = false;
							break;
						}

						msgString.append( sLine.substr( posQuote+1, posEndQuote-posQuote-1 ) );
						posAttached = posEndQuote + 1;
					}
				}
			}

			// add a multi-part string
			if ( !bLookIntoNextLine )
			{
				if ( msgString.size() > 4000 )
				{
					gperrorf(( "Found a string larger than 4000 characters in file '%s':\n[%d] %s\n", (*i).c_str(), lineNumber, sLine.c_str() ));
				}

				AddString( msgString );
			}

			while ( posMsg != gpstring::npos )
			{
				bool bLooking = true; // looking for a string, and/or attached strings
				bool bAppending = false;
				while ( bLooking )
				{
					int posQuote = sLine.find( "\"", posMsg );
					if ( posQuote == gpstring::npos )
					{
						gperrorf(( "initial quote not found after $MSG$ tag in file '%s':\n[%d] %s\n", (*i).c_str(), lineNumber, sLine.c_str() ));
						posMsg = sLine.size();
						break;
					}
					int posEndQuote = sLine.find( "\"", posQuote+1 );
					if ( posEndQuote == gpstring::npos )
					{
						gperrorf(( "ending quote not found after $MSG$ tag in file '%s':\n[%d] %s\n", (*i).c_str(), lineNumber, sLine.c_str() ));
						posMsg = sLine.size();
						break;
					}
					while ( sLine[posEndQuote-1] == '\\' )
					{
						if ( (sLine[posEndQuote-2] == '\\') && (sLine[posEndQuote-3] != '\\') )
						{
							break; // false alarm, the quote is not an escape code.
						}
						posEndQuote = sLine.find( "\"", posEndQuote+1 );
						if ( posEndQuote == gpstring::npos )
						{
							gperrorf(( "ending quote not found after $MSG$ tag in file '%s':\n[%d] %s\n", (*i).c_str(), lineNumber, sLine.c_str() ));
							break;
						}
					}
					if ( (posQuote == gpstring::npos) || (posEndQuote == gpstring::npos) )
					{
						posMsg = sLine.size();
						break;
					}

					if ( bAppending )
					{
						msgString.append( sLine.substr( posQuote+1, posEndQuote-posQuote-1 ) );
					}
					else
					{
						msgString = sLine.substr( posQuote+1, posEndQuote-posQuote-1 );
					}

					// read ahead a bit to see if there is another attached string
					bAppending = false;
					int posAttached = posEndQuote + 1;
					while ( posAttached <= sLine.size() - 1 )
					{
						if ( sLine[posAttached] == '\"' )
						{
							bLooking = true;
							bAppending = true;
							posMsg = posAttached;
							break;
						}
						else if ( !((sLine[posAttached] == ' ') || (sLine[posAttached] == '\t')) )
						{
							bLooking = false;
							break;
						}
						++posAttached;
					}
					if ( bLooking && !bAppending )
					{
						bLookIntoNextLine = true;
						posMsg = sLine.size();
						break;
					}
				}

				if ( !bLookIntoNextLine )
				{
					if ( msgString.size() > 4000 )
					{
						gperrorf(( "Found a string larger than 4000 characters in file '%s':\n[%d] %s\n", (*i).c_str(), lineNumber, sLine.c_str() ));
					}

					AddString( msgString );
				}

				// find another $MSG$ on this line
				posMsg = sLine.find( "$MSG$", posMsg + 5 );
			}
			sLine.clear();
		}

		cppFile.Close();
	}}

	// Make RC file
	FILE *rcFile = fopen( "temp.rc", "w" );
	if ( !rcFile )
	{
		gperror( "Unable to open file \"temp.rc\" for writing\n" );
		return ( -1 );
	}
	fputs( RC_HEADER, rcFile );

	// Make string table
	fputs( "STRINGTABLE\n"
		   "{\n", rcFile );
	{for( MsgMap::iterator i = Msgs.begin(); i != Msgs.end(); ++i )
	{
		fprintf( rcFile, "\t%d, \"%s\"\n", i->first, i->second.c_str() );
	}}
	fputs( "}\n\n", rcFile );

	// Make English lookup table (so LocStudio won't notice)
	fputs( "1 RCDATA\n"
		   "{\n", rcFile );
	{for( MsgMap::iterator i = Msgs.begin(); i != Msgs.end(); ++i )
	{
		fprintf( rcFile, "\t%d, \"%s\\0\",\n", i->first, i->second.c_str() );
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
		return ( -1 );
	}
	WaitForSingleObject( info.hProcess, INFINITE );
	CloseHandle( info.hProcess );

	// Make LINK struct
	gpstring params;
	info.lpFile = "link";
	info.lpParameters = params.assignf( "temp.res /NOLOGO /FIXED /DLL /OUT:%s /NOENTRY /MACHINE:IX86", sDLLfile.c_str() );

	// Make DLL
	if ( !ShellExecuteEx( &info ) )
	{
		gperror( "Could not execute LINK\n" );
		return ( -1 );
	}
	WaitForSingleObject( info.hProcess, INFINITE );
	CloseHandle( info.hProcess );

	int errors = ReportSys::GetErrorCount();
	if ( errors > 0 )
	{
		gperrorf(( "\n** There were %d errors during run.\n", errors ));
	}

	return( 0 );
}
