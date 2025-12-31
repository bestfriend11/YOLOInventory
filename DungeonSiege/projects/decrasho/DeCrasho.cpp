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

#include "Precomp_GpCore.h"

#include "DebugHelp.h"
#include "FileSysXfer.h"
#include "ReportSys.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////
// function main() implementation

int main( int argc, char* argv[] )
{
	ReportSys::EnableErrorAssert( false );
	gErrorContext  .AddSink( new ReportSys::FileSink( ::GetStdHandle( STD_ERROR_HANDLE  ) ), true );
	gWarningContext.AddSink( new ReportSys::FileSink( ::GetStdHandle( STD_ERROR_HANDLE  ) ), true );
	gGenericContext.AddSink( new ReportSys::FileSink( ::GetStdHandle( STD_OUTPUT_HANDLE ) ), true );

	if ( (argc < 3) || (argc > 4) )
	{
		gperrorf(( "Usage: DECRASHO <exe> <in> [<out>]\n" ));
		return ( -1 );
	}

	gpgenericf(( "Opening file '%s'\n", argv[ 2 ] ));

	FileSys::File in;
	if ( !in.OpenExisting( argv[ 2 ] ) )
	{
		gperrorf(( "Unable to open file, error is '%s'\n", stringtool::GetLastErrorText().c_str() ));
		return ( -2 );
	}

	gpstring outFile;
	if ( argc < 4 )
	{
		outFile = argv[ 2 ];
		outFile += ".decrasho";
	}
	else
	{
		outFile = argv[ 3 ];
	}
	
	gpgenericf(( "Creating file '%s'\n", outFile.c_str() ));

	FileSys::File out;
	if ( !out.CreateAlways( outFile, FileSys::File::ACCESS_READ_WRITE, FileSys::File::SHARE_READ_ONLY ) )
	{
		gperrorf(( "Unable to create file, error is '%s'\n", stringtool::GetLastErrorText().c_str() ));
		return ( -2 );
	}

	gpgeneric( "Initializing IMAGEHLP.DLL...\n" );

	DbgSymbolEngine sym;
	if ( !sym.IsInitialized() )
	{
		gperrorf(( "Unable to initialize symbol engine, error is '%s'\n", stringtool::GetLastErrorText().c_str() ));
		return ( -3 );
	}
	
	if ( !sym.LoadModuleSymbols( argv[ 1 ] ) )
	{
		gperrorf(( "Unable to find/use symbols for '%s', error is '%s'\n", argv[ 1 ], stringtool::GetLastErrorText().c_str() ));
		return ( -4 );
	}

	gpgeneric( "Processing...\n" );

	FileSys::FileReader iter( in );
	bool processing = false;

	for ( gpstring line ; iter.ReadLine( line ) ; line.clear() )
	{
		// figure out preceding w/s
		size_t end = ::strncspn_not( line, " \t" );

		// process
		if ( processing && !line.empty() && (line[ end ] != '\0') )
		{
			DWORD addr;
			char modName[ 101 ];
			int rc = ::sscanf( line, "%x %100s", &addr, modName );
			if ( rc == 2 )
			{
				gpstring decoded;
				ReportSys::LocalContext ctx( new ReportSys::StringSink( decoded ), true );
				if ( sym.AddressToString( addr, &ctx ) )
				{
					out.WriteF( "        %s\n", decoded.c_str() );
				}
				else
				{
					gpstring name;
					if ( !sym.AddressToSymName( name, addr, true ) )
					{
						name = "<ERROR>";
					}

					out.WriteF( "%*.s 0x%08X %s %s\n", end - 1, line.c_str(), addr, modName, name.c_str() );
				}
			}
			else
			{
				processing = false;
				out.WriteText( line );
				out.WriteText( "\n" );
			}
		}
		else
		{
			if (   (line.find( "Stack trace:" ) != line.npos)
				|| (line.find( "System Stack" ) != line.npos) )
			{
				processing = true;
			}

			out.WriteText( line );
			out.WriteText( "\n" );
		}
	}

	return ( 0 );
}

//////////////////////////////////////////////////////////////////////////////
