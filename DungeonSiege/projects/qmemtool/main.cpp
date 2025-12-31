//////////////////////////////////////////////////////////////////////////////
//
// File     :  Main.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_QMemTool.h"
#include "QMem.h"

#include "FileSysUtils.h"
#include "FileSysXfer.h"
#include "FuBi.h"
#include "KernelTool.h"
#include "ReportSys.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////
// main

void f4( void )
{

}

void f2( void )
{
	char* x = new char[ 100 ];
	f4();
	delete [] x;
}

void f3( void )
{
	char* x = new char[ 300 ];
	delete [] x;
}

void f1( void )
{
	char* x = new char[ 200 ];
	f2();
	char* y = new char[ 200 ];
	f3();
	delete [] x;
	delete [] y;
}

void f0( void )
{
	f1();
}

void f5( void )
{
	f2();
	f2();
	f3();
}

void func3( void )
{
}

void func2( void )
{
//	gpgenericf(( "func2\n" ));
}

void func( int a, char b, float c )
{
//	gpgenericf(( "a = %d, b = '%c', c = %f\n", a, b, c ));
}

struct Thread : kerneltool::Thread
{
	Thread( int id )
	{
		BeginThread( gpstringf( "T%d", id ) );
		WaitForStart();
	}

   ~Thread( void )
	{
	   WaitForExit();
	}

	virtual DWORD Execute( void )
	{
		::Sleep( 5 );
		func3();
		::Sleep( 5 );
		func2();
		::Sleep( 5 );
		func( 1, 2, 3 );
		::Sleep( 5 );

		return ( 0 );
	}
};

void profile( void )
{
	func2();
	func( 10, 'z', 12.345f );
/*
	{
		Thread t0( 0 ), t1( 1 ), t2( 2 );
	}

	FuBi::SysExports exports;
	exports.ImportBindings();
*/
	f0();
	f5();
}

int main( void )
{
	ReportSys::FileSink consoleSink( ::GetStdHandle( STD_OUTPUT_HANDLE ) );
	gGlobalSink.AddSink( &consoleSink, false );

	stringtool::CommandLineExtractor cmdLine;

	if ( cmdLine.HasTag( "-test" ) )
	{
#		if GP_PROFILING

		gpstring fileName = cmdLine.FindPostTag( "-test", "test.dat" );

		QMem::LogFileWriter logFile;
		if ( !logFile.Create( fileName ) )
		{
			return ( -1 );
		}

		logFile.EnableLogging( cmdLine.HasTag( "-cpu" ) );
		DWORD time = GetTickCount();
		profile();
		DWORD delta = GetTickCount() - time;
		logFile.DisableLogging();

		printf( "Time = %f\n", delta / 1000.0 );

#		else // GP_PROFILING

		gperrorf(( "Not supported in this build! Use the Profiling build instead.\n" ));

#		endif // GP_PROFILING

		return ( 0 );
	}

	stringtool::CommandLineExtractor::TokenColl tokens;
	if ( cmdLine.FindPostTags( tokens, "-process" ) > 0 )
	{
		// get the filename of the dat file to open
		gpstring datFileName = tokens[ 0 ];

		// get the filename of the exe file next
		gpstring exeName;
		if ( tokens.size() > 1 )
		{
			exeName = tokens[ 1 ];
		}

		// try to open file
		QMem::LogFileReader reader;
		if ( !reader.Open( datFileName, exeName ) )
		{
			return ( false );
		}

		// add some exclusions for memory logging
		if ( !cmdLine.HasTag( "-noskip" ) )
		{
			reader.AddMemIgnoreMethod( "gpmalloc(unsigned int)" );
			reader.AddMemIgnoreMethod( "gpstats::AreStatsEnabled(void)" );
			reader.AddMemIgnoreMethod( "gpstats::GetCurrentSystem(void)" );
			reader.AddMemIgnoreMethod( "gprealloc(void *,unsigned int)" );
			reader.AddMemIgnoreMethod( "operator new(unsigned int)" );
			reader.AddMemIgnoreMethod( "std::_Allocate(int,char *)" );
			reader.AddMemIgnoreMethod( "std::allocator<char>::allocate(unsigned int,void const *)" );
		}

		// set options
		reader.SetOptions( QMem::LogFileReader::OPTION_LOG_CPU,        cmdLine.HasTag( "-cpu"      ) );
		reader.SetOptions( QMem::LogFileReader::OPTION_DEBUG_MODE,     cmdLine.HasTag( "-debug"    ) );
		reader.SetOptions( QMem::LogFileReader::OPTION_CHANGED_ALWAYS, cmdLine.HasTag( "-full"     ) );
		reader.SetOptions( QMem::LogFileReader::OPTION_SINGLE_THREAD,  cmdLine.HasTag( "-single"   ) );
		reader.SetOptions( QMem::LogFileReader::OPTION_NO_ZERO_ALLOCS, cmdLine.HasTag( "-nozero"   ) );
		reader.SetOptions( QMem::LogFileReader::OPTION_REMOVE_ALLOCS,  cmdLine.HasTag( "-deallocs" ) );

		// optional xml/qprof output
		bool xml   = cmdLine.HasTag( "-xml" );
		bool qprof = cmdLine.HasTag( "-qprof" );
		if ( xml || qprof )
		{
			// build the output filename - just replace .dat with .xml or .qprof
			gpstring outFileName;
			if ( tokens.size() > 2 )
			{
				outFileName = tokens[ 2 ];
			}
			else
			{
				outFileName = datFileName;
				stringtool::ReplaceExtension( outFileName, xml ? ".xml" : ".qprof" );
			}

			// build the file and dump the trace
			FileSys::File outFile;
			if ( outFile.CreateAlways(
					outFileName,
					FileSys::File::ACCESS_READ_WRITE,
					FileSys::File::SHARE_READ_ONLY ) )
			{
				if ( xml )
				{
					reader.LogToXml( outFile );
				}
				else
				{
					reader.LogToQProf( outFile );
				}
			}
			else
			{
				gperrorf(( "Unable to open file '%s' for outputting %s data, error is '%s'\n",
						   outFileName.c_str(), xml ? "XML" : "QProf", stringtool::GetLastErrorText().c_str() ));
			}
		}
		else
		{
			reader.LogToQuantify();
		}
	}
	else
	{
		gpgeneric(
			"QMEMTOOL Usage:\n"
			"\n"
			"-process <file> - process a qmem dat file\n"
			"-cpu            - do cpu cycles instead of memory\n"
			"-debug          - when output-to-text, add extra debug info\n"
			"-full           - don't trim stacks with no change in sample data\n"
			"-single         - only pay attention to primary thread\n"
			"-nozero         - convert all 0-samples into 1-samples\n"
			"-deallocs       - remove alloc/dealloc pairs\n"
			"-xml [<file>]   - output to xml\n"
			"-qprof [<file>] - output to qprof (API debugging format)\n"
			"\n" );
	}

	return ( 0 );
}

//////////////////////////////////////////////////////////////////////////////
