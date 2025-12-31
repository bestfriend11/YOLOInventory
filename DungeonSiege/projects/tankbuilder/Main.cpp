//////////////////////////////////////////////////////////////////////////////
//
// File     :  Main.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_TankBuilder.h"
#include "Main.h"

#include "Config.h"
#include "FileSysUtils.h"
#include "Filters.h"
#include "FuBiBitPacker.h"
#include "FuBiPersistBinary.h"
#include "Fuel.h"
#include "GpLzo.h"
#include "GpZlib.h"
#include "MasterFileMgr.h"
#include "RapiImage.h"
#include "ReportSys.h"
#include "SkritEngine.h"
#include "StdHelp.h"
#include "StringTool.h"
#include "TankBuilder.h"
#include "TankBuilderVersion.h"

//////////////////////////////////////////////////////////////////////////////
// function main() implementation

int main( void )
{
	return ( BuilderApp().AutoRun() );
}

//////////////////////////////////////////////////////////////////////////////
// class VerifyThread declaration

class VerifyThread : public kerneltool::Thread
{
public:
	SET_INHERITED( VerifyThread, kerneltool::Thread );

	VerifyThread( FileSys::TankFileMgr& tankFileMgr )
		: m_TankFileMgr( tankFileMgr )
	{
		// this space intentionally left blank...
	}

	virtual ~VerifyThread( void )
	{
		WaitForExit();
	}

private:
	virtual DWORD Execute( void )
	{
		BuilderApp::ProcessFiles( m_TankFileMgr, "", "*", true, BuilderApp::PROCESS_VERIFY );
		return ( 0 );
	}

	FileSys::TankFileMgr& m_TankFileMgr;

	SET_NO_COPYING( VerifyThread );
};

//////////////////////////////////////////////////////////////////////////////
// class ConsoleSink declaration

class ConsoleSink : public ReportSys::FileSink
{
public:
	SET_INHERITED( ConsoleSink, ReportSys::FileSink );

	ConsoleSink( HANDLE file )
		: Inherited( file )  {  }
	virtual ~ConsoleSink( void )
		{  }

	virtual bool OutputString( const ReportSys::Context* context, const char* text, int len,
							   bool startLine, bool endLine )
	{
		if ( startLine )
		{
			gStatusSink.PrepForOtherOutput();
		}
		return ( Inherited::OutputString( context, text, len, startLine, endLine ) );
	}

private:
	SET_NO_COPYING( ConsoleSink );
};

//////////////////////////////////////////////////////////////////////////////
// class BuilderApp implementation

BuilderApp :: BuilderApp( void )
{
	GetConfigInit().SetProductName( "Tank Builder" );
	FileSys::SetProductId( DS1_FOURCC_ID );
}

BuilderApp :: ~BuilderApp( void )
{
	// this space intentionally left blank...
}

bool BuilderApp :: BeginReportStatus( bool animateStatus )
{
	static double s_LastUpdate = ::GetGlobalSeconds();
	if ( (::GetGlobalSeconds() - s_LastUpdate) < 0.1 )
	{
		return ( false );
	}

	s_LastUpdate = ::GetGlobalSeconds();
	gStatusContext.BeginReport();

	if ( animateStatus )
	{
		AnimateStatus();
	}

	return ( true );
}

void BuilderApp :: EndReportStatus( bool animateStatus )
{
	if ( animateStatus )
	{
		AnimateStatus();
	}
	gStatusContext.EndReport();
}

void BuilderApp :: AnimateStatus( void )
{
	static const char* s_Animate[] =
	{
		"|       ", "|.      ", "|..     ", " ...    ", "  ...   ", "   ...  ",
		"    ... ", "     ..|", "      .|", "       |", "      .|", "     ..|",
		"    ... ", "   ...  ", "  ...   ", " ...    ", "|..     ", "|.      ",
	};
	static const char** s_AnimIter = s_Animate, ** s_AnimEnd = ARRAY_END( s_Animate );

	gpstatusf(( "%s\n", *s_AnimIter ));

	if ( ++s_AnimIter == s_AnimEnd )
	{
		s_AnimIter = s_Animate;
	}
}

bool BuilderApp :: OnInitCore( void )
{
	// build status sink
	stdx::make_auto_ptr( m_StatusSink, new StatusSink );
	stdx::make_auto_ptr( m_StatusContext, new ReportSys::LocalContext( m_StatusSink.get(), false ) );
	m_StatusContext->SetOptions( ReportSys::Context::OPTION_AUTOREPORT );

	// build log name
	gpstring outDir = m_CommandLine.FindPostTag( "-out" );
	if ( outDir.empty() )
	{
		outDir = FileSys::GetCurrentDirectory();
	}
	stringtool::AppendTrailingBackslash( outDir );

	// build log sink
	ReportSys::Sink* logSink = new ReportSys::LogFileSink <> ( outDir + "errors-tankbuilder.log", false, "Log" );

	// no annoying asserts for errors
	ReportSys::EnableErrorAssert( false );
	ReportSys::EnablePerfAssert( false );

	// setup error output
	ReportSys::FileSink* fileSink = new ConsoleSink( ::GetStdHandle( STD_ERROR_HANDLE ) );
	gErrorContext.AddSink( fileSink, true );
	gErrorContext.AddSink( logSink, true );

	// setup warning output
	fileSink = new ConsoleSink( ::GetStdHandle( STD_ERROR_HANDLE ) );
	gWarningContext.AddSink( fileSink, true );
	gWarningContext.AddSink( logSink, false );

	// setup perf output
	fileSink = new ConsoleSink( ::GetStdHandle( STD_ERROR_HANDLE ) );
	gPerfContext.AddSink( fileSink, true );
	gPerfContext.AddSink( logSink, false );

	// setup normal output
	fileSink = new ConsoleSink( ::GetStdHandle( STD_OUTPUT_HANDLE ) );
	gGenericContext.AddSink( fileSink, true );

	return ( true );
}

bool BuilderApp :: OnInitServices( void )
{
	// get root dir
	gpstring root = m_CommandLine.FindPostTag( "-home", FileSys::GetCurrentDirectory() );

	// setup filesys
	m_PathFileMgr = stdx::make_auto_ptr( new FileSys::PathFileMgr );
	m_PathFileMgr->SetMasterFileMgr();

	// open that path
	if ( !m_PathFileMgr->SetRoot( root ) )
	{
		gperrorf(( "Error: unable to open home directory '%s'\n", root.c_str() ));
		return ( false );
	}

	// setup fuel
	m_FuelSys = stdx::make_auto_ptr( new FuelSys );
	FuelDB* fuelDb = gFuelSys.AddTextDb( "default" );
	fuelDb->Init( FuelDB::OPTION_READ | FuelDB::OPTION_JIT_READ | FuelDB::OPTION_NO_WRITE_LIQUID, m_PathFileMgr->GetRootPath() );
	fuelDb->SetOptions( FuelDB::OPTION_NO_READ_DEV_BLOCKS, !m_CommandLine.HasTag( "-dev" ) );

	// setup filters and aliases
	::RegisterTankFilters();
	RapiImageReader::RegisterFileTypeAliases();

	// done
	return ( true );
}

bool BuilderApp :: OnExecute( void )
{
	bool hasCommand = false;
	bool isBuild = false;

	double startTime = ::GetSystemSeconds();

// Process options.

	// echo command line?
	if ( m_CommandLine.HasTag( "-echo" ) )
	{
		gpgenericf(( "Command Line: [[ %s ]]\n", ::GetCommandLine() ));
	}

	// check nologo
	bool nologo = m_CommandLine.HasTag( "-nologo" );
	if ( !nologo )
	{
		gpgenericf((
				"Gas Powered Games (R) Tank Builder Utility Version %s\n"
				"%s\n\n",
				SysInfo::GetExeFileVersionText( gpversion::MODE_AUTO_LONG ),
				COPYRIGHT_TEXT ));
	}

	// special for precompiled contentdb
	gpstring precompilePath = m_CommandLine.FindPostTag( "-precompile_contentdb" );
	if ( !precompilePath.empty() )
	{
		if ( FileSys::DoesPathExist( precompilePath ) )
		{
			// get output filename
			gpstring precompileFile = precompilePath;
			stringtool::AppendTrailingBackslash( precompileFile );
			precompileFile += "world\\contentdb\\contentdb.lut";

			// create output file
			FileSys::File outFile;
			if ( outFile.CreateAlways( precompileFile ) )
			{
				hasCommand = true;

				// add new db for scanning contentdb
				FuelDB* contentDb = gFuelSys.AddBinaryDb( "precompile_contentdb" );
				BinFuel::SetDictionaryPath( precompilePath );
				contentDb->Init( FuelDB::OPTION_READ | FuelDB::OPTION_JIT_READ  | FuelDB::OPTION_NO_WRITE_LIQUID, precompilePath );
				contentDb->SetOptions( FuelDB::OPTION_NO_READ_DEV_BLOCKS, !m_CommandLine.HasTag( "-dev" ) );

				// a template
				struct Template
				{
					gpstring m_Name;
					gpstring m_Parent;
				};

				// templates go here
				typedef std::map <gpstring /*name*/, gpstring /*parent*/, istring_less> TemplateDb;
				typedef std::map <gpstring /*path*/, TemplateDb, istring_less> TemplatePathDb;
				TemplatePathDb templatePathDb;

				// scan templates
				{
					gpgeneric( "Reading templates...\n" );
					FastFuelHandle templates( "::precompile_contentdb:world:contentdb:templates" );
					FastFuelHandleColl templateList;
					templates.ListChildrenTyped( templateList, "template", -1 );
					FastFuelHandleColl::const_iterator i, begin = templateList.begin(), end = templateList.end();
					for ( i = begin ; i != end ; ++i )
					{
						gpstring path = i->GetDirectory().GetAddress();
						TemplateDb& db = templatePathDb[ path ];
						db[ i->GetName() ] = i->GetString( "specializes" );
					}
					gpgenericf(( "%d templates processed, stored at '%s'\n", templateList.size(), precompileFile.c_str() ));
				}

				// done
				gFuelSys.Remove( contentDb );
				BinFuel::SetDictionaryPath( "" );

				// output 'em
				TemplatePathDb::iterator i, ibegin = templatePathDb.begin(), iend = templatePathDb.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					outFile.WriteF( "%s\n", i->first.c_str() );

					TemplateDb::iterator j, jbegin = i->second.begin(), jend = i->second.end();
					for ( j = jbegin ; j != jend ; ++j )
					{
						outFile.WriteF( "\t%s", j->first.c_str() );
						if ( !j->second.empty() )
						{
							outFile.WriteF( " = %s", j->second.c_str() );
						}
						outFile.WriteF( "\n" );
					}
				}
			}
			else
			{
				gperrorf((
					"Unable to create file '%s' for precompiled contentdb "
					"creation, error is '%s' (0x%08X)!\n",
					precompileFile.c_str(), stringtool::GetLastErrorText().c_str(),
					::GetLastError() ));
			}
		}
		else
		{
			gperrorf((
					"Unable to find path '%s' for precompiled contentdb "
					"creation, error is '%s' (0x%08X)!\n",
					stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
		}
	}

	// build our spec, get the filename while we're at it
	gpwstring tankFileName;
	std::auto_ptr <BuildSpec> buildSpec;
	if ( m_CommandLine.HasTag( "-build" ) )
	{
		isBuild = true;
		hasCommand = true;

		stdx::make_auto_ptr( buildSpec, BuildSpec::CreateBuildSpec( m_CommandLine ) );
		if ( buildSpec.get() != NULL )
		{
			tankFileName = buildSpec->m_TankFileName;
		}
	}
	else
	{
		tankFileName = BuildSpec::GetTankFileName( m_CommandLine );
	}

// Process commands.

	// logging
	if ( m_CommandLine.HasTag( "-log" ) && !tankFileName.empty() )
	{
		gpstring logName( ::ToAnsi( tankFileName ) );
		stringtool::ReplaceExtension( logName, ".log" );
		gGlobalSink.AddSink( new ReportSys::LogFileSink <> ( logName, false, "Log" ), true );
	}

	// build lqd?
	if ( m_CommandLine.HasTag( "-buildlqd" ) )
	{
		hasCommand = true;

		BinaryFuelDB* bdb = gFuelSys.AddBinaryDb( "lqd_builder" );
		BinFuel::SetDictionaryPath( FileSys::GetCurrentDirectory() );
		bdb->Init( FuelDB::OPTION_READ | FuelDB::OPTION_WRITE, FileSys::GetCurrentDirectory(), FileSys::GetCurrentDirectory() );
		bdb->RecompileAll( !m_CommandLine.HasTag( "-nodeleteliquid" ) && !m_CommandLine.HasTag( "-nodeletelqd" ) );
		gFuelSys.Remove( bdb );
	}

	// building?
	if ( buildSpec.get() != NULL )
	{
		hasCommand = true;
		if ( !buildSpec->AutoBuild() )
		{
			return ( false );
		}

		OutputRatios();
	}

	// easy build?
	TokenColl ezFiles;
	if ( m_CommandLine.FindPostTags( ezFiles, "-ez" ) > 0 )
	{
		isBuild = true;
		hasCommand = true;

		if ( tankFileName.empty() )
		{
			gperror( "No filename specified for build\n" );
			return ( false );
		}

		// do it
		if ( !EasyBuildTank( tankFileName, ezFiles ) )
		{
			return ( false );
		}

		OutputRatios();
	}

	// dump the file? only do this if not building (if building, it goes into
	// a text file)
	if ( !isBuild && m_CommandLine.HasTag( "-dump" ) )
	{
		hasCommand = true;

		Tank::TankFile tankFile;
		if ( tankFile.Open( ::ToAnsi( tankFileName ) ) )
		{
			tankFile.Dump();
		}
		else
		{
			gperrorf(( "Unable to open tank file '%S', error is '%s' (0x%08X)\n",
					   tankFileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
			return ( false );
		}
	}

	// output dir?
	gpstring outDir = m_CommandLine.FindPostTag( "-out" );

	// find?
	if ( !isBuild && m_CommandLine.HasTag( "-find" ) )
	{
		hasCommand = true;

		// get all the specs - default to ALL, heh
		TokenColl findFiles;
		if ( m_CommandLine.FindPostTags( findFiles, "-find" ) == 0 )
		{
			findFiles.push_back( "+" );
		}

		// do it
		if ( !FindFiles( tankFileName, findFiles ) )
		{
			return ( false );
		}
	}

	// extract?
	if ( !isBuild && m_CommandLine.HasTag( "-extract" ) )
	{
		hasCommand = true;

		// get all the specs - default to ALL, heh
		TokenColl extractFiles;
		if ( m_CommandLine.FindPostTags( extractFiles, "-extract" ) == 0 )
		{
			extractFiles.push_back( "+" );
		}

		// do it
		if ( !ExtractFiles( tankFileName, extractFiles, outDir ) )
		{
			return ( false );
		}
	}

	// verify?
	if ( !isBuild && m_CommandLine.HasTag( "-verify" ) )
	{
		hasCommand = true;

		// get all the specs - default to ALL, heh
		TokenColl verifyFiles;
		if ( m_CommandLine.FindPostTags( verifyFiles, "-verify" ) == 0 )
		{
			verifyFiles.push_back( "+" );
		}

		// do it
		if ( !VerifyFiles( tankFileName, verifyFiles ) )
		{
			return ( false );
		}
	}

	// query version?
	if ( !isBuild && m_CommandLine.HasTag( "-getversion" ) )
	{
		hasCommand = true;

		// just get it
		gpstring versionName = m_CommandLine.FindPostTag( "-getversion" );
		gpversion version;
		if ( version.InitFromResources( versionName ) )
		{
			gpgenericf(( "Version detected: %s\n", ::ToString( version, gpversion::MODE_COMPLETE ).c_str() ));
		}
		else
		{
			gperrorf(( "Unable to query version info for '%s', error is '%s'\n", versionName.c_str(), stringtool::GetLastErrorText().c_str() ));
		}
	}

	// thread test?
	if ( !isBuild && m_CommandLine.HasTag( "-threadcheck" ) )
	{
		gpstring wild = m_CommandLine.FindPostTag( "-threadcheck" );
		if ( !wild.empty() )
		{
			typedef std::vector <my FileSys::TankFileMgr*> TankFileColl;
			TankFileColl tankFileColl;

			typedef std::vector <my VerifyThread*> VerifyThreadColl;
			VerifyThreadColl verifyThreadColl;

			FileSys::FileFinder finder( wild );
			gpstring name;
			while ( finder.Next( name ) )
			{
				FileSys::TankFileMgr* tankFileMgr = new FileSys::TankFileMgr;
				if ( tankFileMgr->OpenTank( name ) )
				{
					tankFileColl.push_back( tankFileMgr );
					verifyThreadColl.push_back( new VerifyThread( *tankFileMgr ) );
				}
				else
				{
					gperrorf(( "Unable to open tank file '%s', error is '%s'\n", name.c_str(), stringtool::GetLastErrorText().c_str() ));
					delete ( tankFileMgr );
				}
			}

			if ( !tankFileColl.empty() )
			{
				VerifyThreadColl::iterator i, ibegin = verifyThreadColl.begin(), iend = verifyThreadColl.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					(*i)->BeginThread( FileSys::GetFileNameOnly( tankFileColl[ i - ibegin ]->GetTankFileName() ) );
				}

				stdx::for_each( verifyThreadColl, stdx::delete_by_ptr() );
				stdx::for_each( tankFileColl, stdx::delete_by_ptr() );
			}
			else
			{
				gperrorf(( "Unable to find/load any tank files from spec '%s'\n", wild.c_str() ));
			}
		}
		else
		{
			gperror( "Missing filespec for -threadcheck\n" );
		}
	}

	// command mode?
	if ( m_CommandLine.HasTag( "-cmd" ) )
	{
		hasCommand = true;
		if ( !CmdProcessor( ::ToAnsi( tankFileName ) ).Execute() )
		{
			return ( false );
		}
	}

// Finish.

	if ( hasCommand )
	{
		// out time used
		if ( !nologo )
		{
			int hours, minutes;
			float seconds = (float)(::GetSystemSeconds() - startTime);
			::ConvertTime( hours, minutes, seconds );
			gpgenericf(( "\nTotal CPU usage: %02d:%02d:%02d\n", hours, minutes, (int)seconds ));
		}
	}
	else
	{
		// give help?
		gpgeneric( "Usage: TANKBUILDER.EXE <commands...> [<file>.tank]\n"
				   "\n"
				   "Options:\n"
				   "\n"
				   "\t-nologo               - suppress copyright message\n"
				   "\t-log                  - log all build output to <tankname>.log\n"
				   "\t-config <name>        - choose a config from gas file (leading path ok)\n"
				   "\t-file <name>          - set name of tank file to process\n"
				   "\t-in <path>            - set input root directory\n"
				   "\t-out <path>           - set output directory (for new tank or extr files)\n"
				   "\n"
				   "Commands:\n"
				   "\n"
				   "\t-build                - build tank (full build)\n"
				   "\t-ez <spec> [...]      - easy tank build with spec files\n"
				   "\t-dump                 - dump tank\n"
				   "\t-extract <spec> [...] - extract file(s) spec\n"
				   "\t-find <spec> [...]    - find file(s) by spec\n"
				   "\t-cmd                  - enter command mode\n"
				   "\n"
				   "Spec:\n"
				   "\n"
				   "\t* Wildcards (?, *) are ok\n"
				   "\t* Append '+' for recursion through subdirectories\n"
				 );
	}

	return ( true );
}

bool BuilderApp :: OnShutdown( void )
{
	delete ( m_FuelSys    .release() );
	delete ( m_PathFileMgr.release() );

	return ( true );
}

bool BuilderApp :: EasyBuildTank( const gpwstring& tankFileName, const TokenColl& tokens )
{

// Create the builder.

	Tank::Builder builder(  Tank::Builder::OPTION_USE_TEMP_FILE
						  | Tank::Builder::OPTION_BACKUP_OLD );
	builder.SetCreatorId( CREATOR_GPG );
	builder.SetTitleText( FileSys::GetFileNameOnly( tankFileName ) );
	builder.SetDescriptionText( L"Built with EZ" );

// Create the tank.

	if ( !builder.CreateTankAlways( tankFileName ) )
	{
		gperrorf(( "Unable to create tank '%S', error is '%s' (0x%08X)\n",
				   tankFileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
		return ( false );
	}

// Add files.

	// get current time for root dir
	SYSTEMTIME systemTime;
	::GetSystemTime( &systemTime );
	FILETIME dirTime;
	::SystemTimeToFileTime( &systemTime, &dirTime );

	// for each spec
	TokenColl::const_iterator i, begin = tokens.begin(), end = tokens.end();
	for ( i = begin ; i != end ; ++i )
	{
//$$$$		AddFiles( builder, dirTime, *i, "", true, "", "*", "", "" );
	}

// Commit.

	// commit
	if ( !builder.CommitTank() )
	{
		gperrorf(( "Unable to commit tank '%S', error is '%s' (0x%08X)\n",
				   tankFileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
		return ( false );
	}

	return ( true );
}

bool BuilderApp :: ProcessFiles( const gpwstring& tankFileName, const TokenColl& tokens, eProcess process, const gpstring& outDir )
{
	bool success = true;

	FileSys::TankFileMgr tankFileMgr;
	if ( tankFileMgr.OpenTank( ::ToAnsi( tankFileName ) ) )
	{
		if ( !outDir.empty() && !::SetCurrentDirectory( outDir ) )
		{
			gperrorf(( "Unable to set output dir to '%s', error is '%s' (0x%08X)\n",
					   outDir.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
			return ( false );
		}

		TokenColl::const_iterator i, begin = tokens.begin(), end = tokens.end();
		for ( i = begin ; i != end ; ++i )
		{
			// possible patterns:
			//
			// dir\dir\+				process all files in dir\dir recursively
			// dir\dir\					process all files in dir\dir
			// dir\dir+					process all files in dir\dir recursively
			// dir\dir					process all files in dir\dir
			// dir\dir\file+			process anything named "file" in dir/dir recursively
			// dir\dir\file				process the file named "file" in dir/dir
			// dir\dir\file*wild+		process files matching wildcard "file*wild" in dir/dir recursively
			// dir\dir\file*wild		process files matching wildcard "file*wild" in dir/dir
			// dir\dir*wild+			illegal
			// dir\dir*wild				illegal
			// dir*wild\xxx+			illegal
			// dir*wild\xxx				illegal

			gpstring spec = *i;
			gpassert( !spec.empty() );

			// recursive?
			bool recursive = false;
			if ( *spec.rbegin_split() == '+' )
			{
				recursive = true;
				spec.erase( spec.end_split() - 1 );
			}

			// get some vars (assume it's a path to start)
			stringtool::RemoveLeadingBackslash( spec );
			gpstring path( spec ), fileSpec( "*" );

			// definite path?
			if ( !spec.empty() && ((*spec.rbegin() != '\\') && (*spec.rbegin() != '/')) )
			{
				const char* filename = FileSys::GetFileName( spec );

				// definite filespec?
				if ( stringtool::HasDosWildcards( filename ) )
				{
					fileSpec = filename;
					path.assign( spec, filename );
				}
				else
				{
					// try as a singular file
					if ( spec.c_str() == filename )
					{
						// it's in the root dir
						fileSpec = filename;
						path.clear();
					}
					else
					{
						if (    FileSys::DoesResourcePathExist( gpstring( spec, filename ), &tankFileMgr )
							&& !FileSys::DoesResourcePathExist( spec, &tankFileMgr ) )
						{
							fileSpec = filename;
							path.assign( spec, filename );
						}
					}
				}
			}

			// verify path
			if ( path.empty() || FileSys::DoesResourcePathExist( path, &tankFileMgr ) )
			{
				// process
				if ( !ProcessFiles( tankFileMgr, path, fileSpec, recursive, process ) )
				{
					success = false;
				}
			}
			else
			{
				gperrorf(( "Illegal/nonexistent path spec '%s'\n", spec.c_str() ));
			}
		}
	}
	else
	{
		gperrorf(( "Unable to open tank file '%S', error is '%s' (0x%08X)\n",
				   tankFileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
		success = false;
	}

	return ( success );
}

bool BuilderApp :: ProcessFiles( FileSys::TankFileMgr& tankFileMgr, const gpstring& path, const gpstring& fileSpec, bool recursive, eProcess process )
{
	bool success = true;

	// $$$ status update!!! % complete etc.!!!

	// build full path
	gpstring spec = path.empty() ? fileSpec : (path + "\\" + fileSpec);

	// do files
	FileSys::FindHandle find = tankFileMgr.Find( spec, FileSys::FINDFILTER_FILES );
	if ( find )
	{
		gpstring fileName;
		while ( tankFileMgr.GetNext( find, fileName, true ) )
		{
			if ( process == PROCESS_EXTRACT )
			{
				FileSys::FileHandle file = tankFileMgr.Open( fileName );
				FileSys::MemHandle mem = tankFileMgr.Map( file );
				if ( mem )
				{
					// attempt to create dir structure
					bool localSuccess = true;
					if ( recursive && !FileSys::CreateFullFileNameDirectory( fileName ) )
					{
						gperrorf(( "Unable to create directories to output file '%s', error is '%s' (0x%08X)\n",
								   fileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
						localSuccess = false;
					}

					if ( localSuccess )
					{
						// get output file name
						gpstring outName = recursive ? fileName : FileSys::GetFileName( fileName );

						FileSys::File out;
						if ( out.CreateNew( outName ) )
						{
							tankFileMgr.Touch( mem );

							gpgenericf(( "Extracting '%s'\n", fileName.c_str() ));
							if ( !out.Write( tankFileMgr.GetData( mem ), tankFileMgr.GetSize( mem ) ) )
							{
								localSuccess = false;
							}

							if ( !localSuccess )
							{
								gperrorf(( "Error during extraction of file '%s', error is '%s' (0x%08X)\n",
										   fileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
							}
						}
						else
						{
							gperrorf(( "Unable to create file '%s', error is '%s' (0x%08X)\n",
									   fileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
						}
					}

					if ( !localSuccess )
					{
						success = false;
					}
				}
				else
				{
					gperrorf(( "Unable to extract file '%s' from tank, error is '%s' (0x%08X)\n",
							   fileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
				}

				tankFileMgr.Close( mem );
				tankFileMgr.Close( file );
			}
			else if ( process == PROCESS_VERIFY )
			{
				tankFileMgr.SetOptions( FileSys::TankFileMgr::OPTION_VALIDATE_CRC );

				FileSys::FileHandle file = tankFileMgr.Open( fileName );
				FileSys::MemHandle mem = tankFileMgr.Map( file );
				if ( !mem )
				{
					gperrorf(( "Failure to open file '%s'!\n", fileName.c_str() ));
					success = false;
				}

				tankFileMgr.Close( mem );
				tankFileMgr.Close( file );

				tankFileMgr.SetOptions( FileSys::TankFileMgr::OPTION_VALIDATE_CRC, false );
			}
			else
			{
				gpgenericf(( "%s\n", fileName.c_str() ));
			}
		}

		tankFileMgr.Close( find );
	}

	// recurse
	if ( recursive )
	{
		gpstring localPath( path );
		if ( !localPath.empty() )
		{
			localPath += "\\*";
		}

		find = tankFileMgr.Find( localPath, FileSys::FINDFILTER_DIRECTORIES );
		if ( find )
		{
			gpstring pathName;
			while ( tankFileMgr.GetNext( find, pathName, true ) )
			{
				if ( !ProcessFiles( tankFileMgr, pathName, fileSpec, recursive, process ) )
				{
					success = false;
				}
			}

			tankFileMgr.Close( find );
		}
	}

	return ( success );
}

void BuilderApp :: OutputRatios( void )
{
	// output ratio
	const zlib::PerfCounter& zlibCounter = zlib::GetPerfCounters();
	if ( (zlibCounter.m_BytesDeflatedIn > 0) && (zlibCounter.m_SecondsDeflating > 0) )
	{
		int ratio = 100 - int( zlibCounter.m_BytesDeflatedOut * 100.0f / zlibCounter.m_BytesDeflatedIn );
		int rate = int( (zlibCounter.m_BytesDeflatedIn / 1024) / zlibCounter.m_SecondsDeflating );
		gpgenericf(( "\nZLIB: Avg compression = %d%% at %dK bytes/sec\n", ratio, rate ));
	}

	// output ratio
	const lzo::PerfCounter& lzoCounter = lzo::GetPerfCounters();
	if ( (lzoCounter.m_BytesDeflatedIn > 0) && (lzoCounter.m_SecondsDeflating > 0) )
	{
		int ratio = 100 - int( lzoCounter.m_BytesDeflatedOut * 100.0f / lzoCounter.m_BytesDeflatedIn );
		int rate = int( (lzoCounter.m_BytesDeflatedIn / 1024) / lzoCounter.m_SecondsDeflating );
		gpgenericf(( "\nLZO: Avg compression = %d%% at %dK bytes/sec\n", ratio, rate ));
	}
}

//////////////////////////////////////////////////////////////////////////////
// class CmdProcessor implementation

CmdProcessor :: CmdProcessor( const gpstring& initialTank )
{
	m_MasterFileMgr = NULL;
	m_BitPacker     = NULL;

	if ( !initialTank.empty() )
	{
		Open( initialTank );
	}
}

CmdProcessor :: ~CmdProcessor( void )
{
	Delete( m_BitPacker );

	Close();
}

bool CmdProcessor :: Execute( void )
{
	for ( ; ; )
	{
		// build name
		gpstring name;
		if ( m_TankNameColl.empty() && m_PathNameColl.empty() )
		{
			name = "#";
		}
		else if ( (m_TankNameColl.size() == 1) && m_PathNameColl.empty() )
		{
			name = FileSys::GetFileName( m_TankNameColl.front() );
		}
		else
		{
			name.assignf( "(%d)", m_TankNameColl.size() + m_PathNameColl.size() );
		}

		// out prompt
		gpgenericf(( "[%s:%s%s",
				name.c_str(),
				m_Processor.GetCurrentDir().c_str(),
				m_Processor.GetCurrentDir().empty() ? "\\]" : "]" ));

		// in command
		char inBuffer[ 20000 ] = { 0 };
		::fgets( inBuffer, ELEMENT_COUNT( inBuffer ), stdin );

		// kill terminating null/eof
		char& end = inBuffer[ ::strlen( inBuffer ) - 1 ];
		if ( end == '\n' )
		{
			end = '\0';
		}

		// process
		char* cmd = ::strtok( inBuffer, stringtool::ALL_WHITESPACE );
		if ( (cmd != NULL) && (*cmd != '\0') )
		{
			char* next = cmd + ::strlen( cmd ) + 1;
			if ( same_no_case( cmd, "dir" ) )
			{
				if ( IsOpen() )
				{
					if ( !m_Processor.Dir( next ) )
					{
						gperrorf(( "Invalid dir/file/pattern '%s'\n", next ));
					}
				}
				else
				{
					gperror( "No tanks currently open\n" );
				}
			}
			else if ( same_no_case( cmd, "cd" ) )
			{
				if ( IsOpen() )
				{
					if ( !m_Processor.Cd( next ) )
					{
						gperrorf(( "Invalid dir '%s'\n", next ));
					}
				}
				else
				{
					gperror( "No tanks currently open\n" );
				}
			}
			else if ( same_no_case( cmd, "copy" ) )
			{
				if ( IsOpen() )
				{
					if ( !m_Processor.Copy( next ) )
					{
						gperrorf(( "Invalid file/pattern '%s'\n", next ));
					}
				}
				else
				{
					gperror( "No tanks currently open\n" );
				}
			}
			else if ( same_no_case( cmd, "open" ) )
			{
				Open( next );
			}
			else if ( same_no_case( cmd, "close" ) )
			{
				Close();
			}
			else if ( same_no_case( cmd, "dumpsave" ) )
			{
				if ( IsOpen() )
				{
					FileSys::FileHandle xdatf = m_MasterFileMgr->Open( "world.xdat" );
					FileSys::FileHandle xidxf = m_MasterFileMgr->Open( "world.xidx" );
					FileSys::MemHandle  xdatm = m_MasterFileMgr->Map ( xdatf );
					FileSys::MemHandle  xidxm = m_MasterFileMgr->Map ( xidxf );

					if ( xdatm && xidxm )
					{
						FuBi::TreeBinaryReader reader;
						if ( reader.Init(
								const_mem_ptr( m_MasterFileMgr->GetData( xdatm ), m_MasterFileMgr->GetSize( xdatm ) ),
								const_mem_ptr( m_MasterFileMgr->GetData( xidxm ), m_MasterFileMgr->GetSize( xidxm ) ) ) )
						{
							FileSys::File file;
							if ( file.CreateNew( "world.gas" ) )
							{
								ReportSys::LocalContext ctx( new ReportSys::FileSink( file, false ), true );
								reader.Dump( "world", &ctx );
							}
							else
							{
								gperrorf(( "Unable to create output file 'world.gas', error is 0x%08X ('%s')\n",
										   ::GetLastError(), stringtool::GetLastErrorText().c_str() ));
							}
						}
						else
						{
							gperror( "Unable to open save game data! Must be corrupt or something...\n" );
						}
					}
					else
					{
						gperror( "Not a save game file! Couldn't find .xdat/.xidx files...\n" );
					}
				}
				else
				{
					gperror( "No tanks currently open\n" );
				}
			}
			else if ( same_no_case( cmd, "list" ) )
			{
				if ( IsOpen() )
				{
					List();
				}
				else
				{
					gperror( "No tanks currently open\n" );
				}
			}
			else if ( same_no_case( cmd, "exit" ) || same_no_case( cmd, "quit" ) )
			{
				gpgeneric( "Bye!\n" );
				break;
			}
			else if ( same_no_case( cmd, "help" ) || same_no_case( cmd, "?" ) )
			{
				gpgeneric( "Commands: dir, cd, copy, open, close, list, exit\n" );
			}
			else if ( same_no_case( cmd, "bytestore" ) )
			{
				if ( m_BitPacker != NULL )
				{
					gpgeneric( "Deleting old bitpacker\n" );
					Delete( m_BitPacker );
				}

				stringtool::Extractor extractor( " \t\n", next, true );
				typedef stdx::fast_vector <int> IntVec;
				IntVec ints;
				bool success = false;
				extractor.GetInts( ints, &success, 16 );

				if ( success )
				{
					if ( !ints.empty() )
					{
						IntVec::const_iterator i, ibegin = ints.begin(), iend = ints.end();
						for ( i = ibegin ; i != iend ; ++i )
						{
							if ( !(*i & 0xFFFFFF00) )
							{
								m_BitBuffer.push_back( (BYTE)*i );
							}
							else
							{
								gperrorf(( "Number 0x%08X out of range!\n", *i ));
							}
						}

						gpgenericf(( "Read in %d entries, bitbuffer now at %d bits\n",
								     ints.size(), m_BitBuffer.size_bytes() * 8 ));
					}
					else
					{
						gpgeneric( "Emptying the bitstore\n" );
						m_BitBuffer.clear();
					}
				}
				else
				{
					gperror( "Something bad in there...\n" );
				}
			}
			else if ( same_no_case( cmd, "bitread" ) )
			{
				if ( BuildPacker( 1 ) )
				{
					gpgenericf(( "Bit: %d\n", m_BitPacker->ReadBit() ? 1 : 0 ));
				}
			}
			else if ( same_no_case( cmd, "byteread" ) )
			{
				if ( BuildPacker( 8 ) )
				{
					BYTE b;
					m_BitPacker->XferRaw( b );
					gpgenericf(( "Byte: %d\n", b ));
				}
			}
			else if ( same_no_case( cmd, "floatread" ) )
			{
				if ( BuildPacker( 32 ) )
				{
					float f;
					m_BitPacker->XferFloat( f );
					gpgenericf(( "Float: %g\n", f ));
				}
			}
			else if ( same_no_case( cmd, "intread" ) )
			{
				if ( BuildPacker( 32 ) )
				{
				}
			}
			else
			{
				gperrorf(( "Unrecognized command '%s'\n", cmd ));
			}
		}
	}

	return ( false );
}

bool CmdProcessor :: Open( gpstring spec )
{
	// make sure we have a master
	if ( m_MasterFileMgr == NULL )
	{
		m_MasterFileMgr = new FileSys::MultiFileMgr;
		m_Processor.SetFileMgr( m_MasterFileMgr );
	}

	// add stuff to it
	FileSys::MultiFileMgr::AddResults addResults;
	bool success = false, paths = false;
	if ( spec[ 0 ] == '!' )
	{
		paths = true;
		spec.erase( 0, 1 );
		success = m_MasterFileMgr->AddBitsPaths( ::ToUnicode( spec ), true, &addResults );
	}
	else
	{
		success = m_MasterFileMgr->AddTankPaths( ::ToUnicode( spec ), true, &addResults );
	}

	// process successes
	if ( !addResults.m_Added.empty() )
	{
		std::list <gpwstring>::iterator i, ibegin = addResults.m_Added.begin(), iend = addResults.m_Added.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gpstring ia = ::ToAnsi( *i );
			if ( paths )
			{
				m_PathNameColl.push_back( ia );
				gpgenericf(( "Added path '%s'\n", ia.c_str() ));
			}
			else
			{
				m_TankNameColl.push_back( ia );
				gpgenericf(( "Opened tank '%s'\n", ia.c_str() ));
			}
		}
	}
	else if ( addResults.m_NotAdded.empty() && addResults.m_BadPaths.empty() )
	{
		gpgenericf(( "No tank files found...\n" ));
	}

	// process failures
	std::list <FileSys::MultiFileMgr::NotAdded>::iterator j, jbegin = addResults.m_NotAdded.begin(), jend = addResults.m_NotAdded.end();
	for ( j = jbegin ; j != jend ; ++j )
	{
		gpgenericf(( "Error opening tank '%S', error is 0x%08X (%s)\n",
					 j->m_Name.c_str(), j->m_LastError, stringtool::GetLastErrorText( j->m_LastError ).c_str() ));
	}

	// process bad paths
	std::list <gpwstring>::iterator k, kbegin = addResults.m_BadPaths.begin(), kend = addResults.m_BadPaths.end();
	for ( k = kbegin ; k != kend ; ++k )
	{
		gpgenericf(( "Bad path '%S'\n", k->c_str() ));
	}

	return ( success );
}

bool CmdProcessor :: Close( void )
{
	if ( IsOpen() )
	{
		m_Processor.SetFileMgr( NULL );
		Delete( m_MasterFileMgr );
		m_TankNameColl.clear();
		m_PathNameColl.clear();
	}

	return ( true );
}

bool CmdProcessor :: List( void )
{
	if ( IsOpen() )
	{
		StringColl::const_iterator i, ibegin = m_TankNameColl.begin(), iend = m_TankNameColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gpgenericf(( "[T]%s\n", i->c_str() ));
		}
		ibegin = m_PathNameColl.begin(), iend = m_PathNameColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gpgenericf(( "[P]%s\n", i->c_str() ));
		}
		return ( true );
	}
	else
	{
		return ( false );
	}
}

bool CmdProcessor :: IsOpen( void ) const
{
	return ( !m_TankNameColl.empty() || !m_PathNameColl.empty() );
}

bool CmdProcessor :: BuildPacker( int bitsNeeded )
{
	if ( m_BitPacker == NULL )
	{
		m_BitPacker = new FuBi::BitPacker( const_mem_ptr( &*m_BitBuffer.begin(), m_BitBuffer.size_bytes() ) );
	}

	bool ok = m_BitPacker->GetBitsLeft() >= bitsNeeded;
	if ( !ok )
	{
		gpgeneric( "Not enough bits in storage!\n" );
	}

	return ( ok );
}

//////////////////////////////////////////////////////////////////////////////
