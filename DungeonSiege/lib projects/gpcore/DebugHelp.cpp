//////////////////////////////////////////////////////////////////////////////
//
// File     :  DebugHelp.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"

#include "DebugHelp.h"
#include "DrWatson.h"
#include "FileSysUtils.h"
#include "FuBiDefs.h"
#include "ReportSys.h"
#include "StdHelp.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////
// class DbgHelpDll implementation

DbgHelpDll :: DbgHelpDll( void )
	: Inherited( "DBGHELP.DLL" )
{
#	define ADD_ENTRY( f )  AddProc( &f, #f )
#	define OPT_ENTRY( f )  AddProc( &f, #f, true )

	ADD_ENTRY( SymInitialize          );
	ADD_ENTRY( SymCleanup             );
	ADD_ENTRY( SymSetOptions          );
	ADD_ENTRY( SymGetOptions          );
	ADD_ENTRY( SymFunctionTableAccess );
	ADD_ENTRY( SymGetModuleBase       );
	ADD_ENTRY( StackWalk              );
	ADD_ENTRY( SymGetModuleInfo       );
	ADD_ENTRY( SymGetSymFromAddr      );
	ADD_ENTRY( SymGetLineFromAddr     );
	ADD_ENTRY( SymLoadModule          );
	ADD_ENTRY( UnDecorateSymbolName   );
	OPT_ENTRY( MiniDumpWriteDump      );	// requires v5.1

#	undef ADD_ENTRY
}

//////////////////////////////////////////////////////////////////////////////
// class DbgSymbolEngine implementation

// $ much of this code is based on the amazing John Robbins "SUPERASSERT" code.
//   see his book "Debugging Applications" for more info. the rest
//   (specifically anything not involving debughlp.dll) is adapted from
//   Matt Pietrek's MSJ 97 "Under the Hood" article.

DbgSymbolEngine::StackQueryCbColl DbgSymbolEngine::ms_StackQueryCbColl;

DbgSymbolEngine :: DbgSymbolEngine( void )
{
	// init members
	m_Initialized = false;

// Build symbol search path.

	// $ use paths from SymInitialize() docs
	gpstring searchPath( FileSys::GetCurrentDirectory( false ) );

	// + exe dir
	searchPath += ';';
	searchPath += FileSys::GetModuleDirectory( NULL, false );

	// + default symbol dirs
	gpstring temp;
	if ( stringtool::GetEnvironmentVariable( "_NT_SYMBOL_PATH", temp ) )
	{
		searchPath += ';';
		searchPath += temp;
	}
	if ( stringtool::GetEnvironmentVariable( "_NT_ALTERNATE_SYMBOL_PATH", temp ) )
	{
		searchPath += ';';
		searchPath += temp;
	}
	if ( stringtool::GetEnvironmentVariable( "SYSTEMROOT", temp ) )
	{
		searchPath += ';';
		searchPath += temp;
	}

// Setup symbol engine.

	// get at dll first, may not exist
	if ( m_DbgHelpDll->Load() )
	{
		// set our options
		DWORD old = m_DbgHelpDll->SymGetOptions();
		m_DbgHelpDll->SymSetOptions( old | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES );

		// attempt to initialize symbol engine
		if ( m_DbgHelpDll->SymInitialize( (HANDLE)this, ccast <char*> ( searchPath.c_str() ), FALSE ) )
		{
			m_Initialized = true;
		}
	}
}

DbgSymbolEngine :: ~DbgSymbolEngine( void )
{
	if ( m_Initialized )
	{
		m_DbgHelpDll->SymCleanup( (HANDLE)this );
	}
}

static DbgSymbolEngine* s_CurrentEngine = NULL;

DbgSymbolEngine::eError DbgSymbolEngine :: StackWalk(
		HANDLE hprocess, HANDLE hthread, const CONTEXT& regs,
		AddressColl& addresses, DWORD startFrame )
{
	static const int MAX_STACK_WALK_DEPTH = 100;

	eError error = ERR_INTERNAL;
	bool skip = (startFrame != 0);

	// build stack object
	STACKFRAMEX frame( regs );

	// check for dbghelp.dll available
	if ( m_Initialized )
	{
		// dbhelp wants non-const context
		CONTEXT localRegs = regs;

		// default - warn about depth of stack walk - probably infinite recursion
		error = ERR_STACK_TOO_DEEP;

		// don't go too deep
		for ( int i = 0 ; i < MAX_STACK_WALK_DEPTH ; ++i )
		{
			// continue until we can't walk any more
			gpassert( s_CurrentEngine == NULL );
			s_CurrentEngine = this;
			bool success = !!m_DbgHelpDll->StackWalk(
					IMAGE_FILE_MACHINE_I386,
					hprocess,
					hthread,
					&frame,
					&localRegs,
					NULL,
					(PFUNCTION_TABLE_ACCESS_ROUTINE)m_DbgHelpDll->SymFunctionTableAccess.m_Proc,
					GetModuleBase,
					NULL);
			gpassert( s_CurrentEngine == this );
			s_CurrentEngine = NULL;
			if ( !success )
			{
				error = ERR_SUCCESS;
				break;
			}

			// also check that the address is not zero. sometimes StackWalk()
			//  returns TRUE with a frame of zero.
			if ( frame.AddrPC.Offset != 0 )
			{
				// throw out entries until we hit our target frame
				if ( skip )
				{
					if ( frame.AddrFrame.Offset == startFrame )
					{
						skip = false;
					}
				}

				// record this if we're not skipping
				if ( !skip )
				{
#					if 0
					// $ i disabled this dup stack checking code because it
					//   ended up filtering out minor recursion in the stack
					//   and detecting it as corruption. the MAX_STACK_WALK_DEPTH
					//   should be good enough. if it's not, put the filter back
					//   in with a max spin count of 20 or so on it.

					// see if no change
					if ( !addresses.empty() && (addresses.back() == frame.AddrPC.Offset) )
					{
						error = ERR_CORRUPT_STACK;
						break;
					}
#					endif // 0

					// add to set
					addresses.push_back( frame.AddrPC.Offset );
				}
			}
		}
	}
	else
	{
		// $ do standard x86 stack walk - no FPO data available so this will
		//   fail with nonstandard stack frames (check your optimizer settings).

		// assume it will succeed
		error = ERR_SUCCESS;

		__try
		{
			DWORD pc = regs.Eip;
			DWORD* frame = (DWORD*)regs.Ebp;
			DWORD* prevFrame;
			int count = 0;

			do
			{
				// throw out entries until we hit our target frame
				if ( skip )
				{
					if ( (DWORD)frame == startFrame )
					{
						skip = false;
					}
				}

				// record this if we're not skipping
				if ( !skip )
				{
#					if 0
					// $ see note from other walker above

					// see if no change
					if ( !addresses.empty() && (addresses.back() == pc) )
					{
						error = ERR_CORRUPT_STACK;
						break;
					}
#					endif // 0

					// add to set
					addresses.push_back( pc );
				}

				pc        = frame[ 1 ];
				prevFrame = frame;
				frame     = (DWORD*)frame[ 0 ];		// jump back to next higher frame on stack

				if ( ++count >= MAX_STACK_WALK_DEPTH )
				{
					error = ERR_STACK_TOO_DEEP;
					break;
				}
			}
			while (    ::IsDwordAligned( frame )							// frame pointer must be aligned on a dword boundary
					&& (frame > prevFrame)									// only go up the stack
					&& !::IsBadWritePtr( frame, sizeof( LPCVOID ) * 2 ) );	// can two dwords be read from the supposed frame address?
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
			// exception == corrupt stack probably
			error = ERR_CORRUPT_STACK;
		}
	}

	return ( error );
}

DbgSymbolEngine::eError DbgSymbolEngine :: StackWalk( AddressColl& addresses, DWORD startFrame )
{
	eError error = ERR_INTERNAL;

	// bail early if can't get the context
	CONTEXT context;
	context.ContextFlags = CONTEXT_FULL;
	if ( GetThreadContext( GetCurrentThread(), &context ) )
	{
		error = StackWalk( GetCurrentProcess(), GetCurrentThread(), context, addresses, startFrame );
	}

	return ( error );
}

DbgSymbolEngine::eError DbgSymbolEngine :: StackTrace( HANDLE hprocess, HANDLE hthread, DWORD threadId, const CONTEXT& regs, ReportSys::ContextRef ctx, DWORD startFrame )
{
	return ( PrivateStackTrace( hprocess, hthread, threadId, &regs, ctx, startFrame, NULL ) );
}

DbgSymbolEngine::eError DbgSymbolEngine :: StackTrace( HANDLE hprocess, HANDLE hthread, DWORD threadId, const AddressColl& addresses, ReportSys::ContextRef ctx, DWORD startFrame )
{
	return ( PrivateStackTrace( hprocess, hthread, threadId, NULL, ctx, startFrame, &addresses ) );
}

DbgSymbolEngine::eError DbgSymbolEngine :: StackTrace( ReportSys::ContextRef ctx, DWORD startFrame )
{
	return ( PrivateStackTrace( GetCurrentProcess(), GetCurrentThread(), GetCurrentThreadId(), NULL, ctx, startFrame, NULL ) );
}

bool DbgSymbolEngine :: AddressToString( DWORD address, ReportSys::ContextRef ctx )
{
	bool success = true;

	// always stick the address in first
	IMAGEHLP_SYMBOLX <> symbol;
	symbol.Address = address;
	ctx->OutputF( "0x%08X ", address );

	// now the name of the module
	ctx->OutputF( "%s: ", AddressToModuleName( address ).c_str() );

	// get the function
	DWORD disp;
	if ( m_Initialized && m_DbgHelpDll->SymGetSymFromAddr( (HANDLE)this, address, &disp, &symbol ) )
	{
		ctx->OutputF( "%s", symbol.Name );
		if ( disp )
		{
			ctx->OutputF( " + %d bytes", disp );
		}

		// if i got a symbol, give the source and line a whirl
		IMAGEHLP_LINEX line;
		if ( m_DbgHelpDll->SymGetLineFromAddr( (HANDLE)this, address, &disp, &line ) )
		{
			ctx->OutputF( " ( %s, Line %d", FileSys::GetFileName( line.FileName ), line.LineNumber );
			if ( disp )
			{
				ctx->OutputF( " + %d bytes", disp );
			}
			ctx->Output( " )" );
		}
	}
	else
	{
		success = false;

		MEMORY_BASIC_INFORMATION mbi;
		if ( ::VirtualQuery( (LPCVOID)address, &mbi, sizeof( mbi ) ) )
		{
			DWORD hmodule = (DWORD)mbi.AllocationBase;

			PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hmodule;
			if (   !::IsBadReadPtr( dosHeader, sizeof( IMAGE_DOS_HEADER ) )
				&& (dosHeader->e_magic == IMAGE_DOS_SIGNATURE) )
			{
				PIMAGE_NT_HEADERS     ntHeader = (PIMAGE_NT_HEADERS)(hmodule + dosHeader->e_lfanew);
				PIMAGE_SECTION_HEADER section  = IMAGE_FIRST_SECTION( ntHeader );

				DWORD rva = address - hmodule;

				// iterate through the section table, looking for the one that
				// encompasses the linear address.
				for ( DWORD i = 0 ; i < ntHeader->FileHeader.NumberOfSections ; ++i, ++section )
				{
					DWORD sectionStart = section->VirtualAddress;
					DWORD sectionEnd = sectionStart + max( section->SizeOfRawData, section->Misc.VirtualSize );

					// is the address in this section?
					if ( (rva >= sectionStart) && (rva <= sectionEnd) )
					{
						// yes, address is in the section - output
						ctx->OutputF( "%04X:%08X", i + 1, rva - sectionStart );

						// abort
						success = true;
						break;
					}
				}
			}
		}

		// dunno what it is
		if ( !success )
		{
			ctx->OutputF( "<unknown symbol/offset>" );
		}
	}

	return ( success );
}

bool DbgSymbolEngine :: AddressToSymName( gpstring& name, DWORD address, bool withParams )
{
	IMAGEHLP_SYMBOLX <> symbol;
	if ( AddressToSymbol( symbol, address, withParams ) )
	{
		name = symbol.Name;
		return ( true );
	}
	else
	{
		name.clear();
		return ( false );
	}
}

bool DbgSymbolEngine :: AddressToSymbol( IMAGEHLP_SYMBOLX <>& symbol, DWORD address, bool withParams )
{
	symbol.Address = address;

	DWORD disp = 0;
	bool success = false;

	DWORD old = m_DbgHelpDll->SymGetOptions();
	if ( withParams )
	{
		// disable the undecorated name thing so we can do full params
		m_DbgHelpDll->SymSetOptions( old & ~SYMOPT_UNDNAME );
	}

	if ( m_Initialized && m_DbgHelpDll->SymGetSymFromAddr( (HANDLE)this, address, &disp, &symbol ) )
	{
		if ( withParams )
		{
			gpstring mangledName = symbol.Name;
			if ( m_DbgHelpDll->UnDecorateSymbolName(
					mangledName,
					symbol.Name,
					symbol.MaxNameLength,
					  UNDNAME_COMPLETE
					| UNDNAME_32_BIT_DECODE
					| UNDNAME_NO_LEADING_UNDERSCORES
					| UNDNAME_NO_MS_KEYWORDS
					| UNDNAME_NO_FUNCTION_RETURNS
					| UNDNAME_NO_ALLOCATION_MODEL
					| UNDNAME_NO_ALLOCATION_LANGUAGE
					| UNDNAME_NO_ACCESS_SPECIFIERS
					| UNDNAME_NO_THROW_SIGNATURES
					| UNDNAME_NO_MEMBER_TYPE
					| UNDNAME_NO_RETURN_UDT_MODEL ) )
			{
				success = true;
			}
		}
		else
		{
			success = true;
		}
	}

	if ( withParams )
	{
		m_DbgHelpDll->SymSetOptions( old );
	}

	return ( success );
}

bool DbgSymbolEngine :: AddressToLocation( gpstring& fileName, DWORD& line, DWORD address )
{
	DWORD disp = 0;
	IMAGEHLP_LINEX lineInfo;

	bool success = false;

	if ( m_Initialized && m_DbgHelpDll->SymGetLineFromAddr( (HANDLE)this, address, &disp, &lineInfo ) )
	{
		fileName = lineInfo.FileName;
		line = lineInfo.LineNumber;
		success = true;
	}
	else
	{
		fileName.clear();
		line = 0;
	}

	return ( success );
}

bool DbgSymbolEngine :: LoadModuleSymbols( const char* imageName, DWORD imageBase )
{
	gpstring localName;
	if ( imageName == NULL )
	{
		localName = FileSys::GetModuleFileName();
		imageName = localName;
		imageBase = (DWORD)::GetModuleHandle( NULL );
	}

	DWORD oldOptions = m_DbgHelpDll->SymGetOptions();
	m_DbgHelpDll->SymSetOptions( oldOptions & ~SYMOPT_DEFERRED_LOADS );

	bool rc = !!m_DbgHelpDll->SymLoadModule( this, NULL, ccast <char*> ( imageName ), NULL, imageBase, 0 );

	m_DbgHelpDll->SymSetOptions( oldOptions );

	return ( rc );
}

bool DbgSymbolEngine :: LoadCurrentModuleSymbols( void )
{
	DWORD rc = 0;
	if ( m_Initialized )
	{
		DWORD oldOptions = m_DbgHelpDll->SymGetOptions();
		m_DbgHelpDll->SymSetOptions( oldOptions & ~SYMOPT_DEFERRED_LOADS );

		gpassert( s_CurrentEngine == NULL );
		s_CurrentEngine = this;
		rc = GetModuleBase( GetCurrentProcess(), FuBi::GetEIP() );
		gpassert( s_CurrentEngine == this );
		s_CurrentEngine = NULL;

		m_DbgHelpDll->SymSetOptions( oldOptions );
	}

	return ( rc != 0 );
}

bool DbgSymbolEngine :: MiniDump( bool verbose, const EXCEPTION_POINTERS* xinfo, gpstring* outFileName )
{
	bool success = false;

	if ( *m_DbgHelpDll && m_DbgHelpDll->MiniDumpWriteDump )
	{
		// make sure pointing at something valid
		gpstring localFileName;
		if ( outFileName == NULL )
		{
			outFileName = &localFileName;
		}

		// get the version and make it readable
		gpstring textVersion = SysInfo::GetExeFileVersionText( gpversion::MODE_AUTO_LONG );
		if ( !textVersion.empty() )
		{
			for ( int i = 0 ; textVersion[ i ] != '\0' ; )
			{
				if ( !iscsym( textVersion[ i ] ) && !strchr( "[]().-!", textVersion[ i ] ) )
				{
					if ( (i > 0) && (textVersion[ i - 1 ] == '_')  )
					{
						textVersion.erase( i, 1 );
					}
					else
					{
						textVersion.at_split( i ) = '_';
						++i;
					}
				}
				else
				{
					++i;
				}
			}
		}

		// first generate a filename
		*outFileName += FileSys::GetFileNameOnly( FileSys::GetModuleFileName() );
		if ( !textVersion.empty() )
		{
			*outFileName += '_';
			*outFileName += textVersion;
		}
		*outFileName += '_';
		*outFileName = FileSys::MakeUniqueFileName( *outFileName, "mdmp", DrWatson::GetOutputDirectory() );

		// valid xinfo?
		MINIDUMP_EXCEPTION_INFORMATION minfo;
		minfo.ThreadId          = ::GetCurrentThreadId();
		minfo.ExceptionPointers = ccast <EXCEPTION_POINTERS*> ( xinfo );
		minfo.ClientPointers    = true;

		// create the file
		FileSys::File dumpFile;
		if ( dumpFile.CreateAlways( *outFileName ) )
		{
			success = !!m_DbgHelpDll->MiniDumpWriteDump(
					::GetCurrentProcess(),
					::GetCurrentProcessId(),
					dumpFile,
					_MINIDUMP_TYPE( MiniDumpWithDataSegs | MiniDumpWithHandleData | (verbose ? MiniDumpWithFullMemory : 0) ),
					(xinfo != NULL) ? &minfo : NULL,
					NULL,
					NULL );

			if ( success )
			{
				gpdebuggerf(( "Minidump -> '%s'\n", outFileName->c_str() ));
			}
		}
	}
	else
	{
		::SetLastError( ERROR_DLL_INIT_FAILED );
	}

	if ( !success && (outFileName != NULL) )
	{
		outFileName->clear();
	}

	return ( success );
}

void DbgSymbolEngine :: RegisterStackQueryCb( StackQueryCb cb )
{
	if ( !stdx::has_any( ms_StackQueryCbColl, cb ) )
	{
		ms_StackQueryCbColl.push_back( cb );
	}
}

void DbgSymbolEngine :: UnregisterStackQueryCb( StackQueryCb cb )
{
	StackQueryCbColl::iterator found = stdx::find( ms_StackQueryCbColl, cb );
	if ( found != ms_StackQueryCbColl.end() )
	{
		ms_StackQueryCbColl.erase( found );
	}
}

bool DbgSymbolEngine :: DumpGameStack( DWORD threadId, ReportSys::ContextRef ctx )
{
	// $$$ should probably try/catch(...) this thing in case the translation accesses bad memory...

	// game stack?
	bool hasNonSystemStacks = false;

	gpstring name;
	StringVec stack;

	StackQueryCbColl::const_iterator i, ibegin = ms_StackQueryCbColl.begin(), iend = ms_StackQueryCbColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		name.clear();
		stack.clear();

		if ( (*i)( threadId, name, stack ) && !stack.empty() )
		{
			hasNonSystemStacks = true;

			ctx->OutputF(
					name.empty()
					? "-=-=- Game Stack -=-=-\n\n"
					: "-=-=- Game Stack: %s -=-=-\n\n",
					name.c_str() );

			StringVec::reverse_iterator j, jbegin = stack.rbegin(), jend = stack.rend();
			for ( j = jbegin ; j != jend ; ++j )
			{
				ctx->Output( *j );
				ctx->OutputEol();
			}

			ctx->OutputEol();
		}
	}

	return ( hasNonSystemStacks );
}

DbgSymbolEngine::eError DbgSymbolEngine :: PrivateStackTrace(
		HANDLE hprocess, HANDLE hthread, DWORD threadId, const CONTEXT* regs,
		ReportSys::ContextRef ctx, DWORD startFrame, const AddressColl* addresses )
{
	// wrap in a report
	ReportSys::AutoReport autoReport( ctx );

	// prep for system stack
	if ( DumpGameStack( threadId, ctx ) )
	{
		ctx->Output( "-=-=- System Stack -=-=-\n\n" );
	}

	// get the stack addresses
	eError rc = ERR_SUCCESS;
	AddressColl localAddresses;
	if ( (addresses == NULL) || addresses->empty() )
	{
		rc = (regs != NULL ) ? StackWalk( hprocess, hthread, *regs, localAddresses, startFrame )
							 : StackWalk( localAddresses, startFrame );
		addresses = &localAddresses;
	}
	if ( IsFatal( rc ) )
	{
		ctx->Output( "<Internal error collecting stack trace>\n" );
		return ( rc );
	}

	// empty? impossible - must be a fuckup
	if ( addresses->empty() )
	{
		// probably failed
		rc = ERR_CORRUPT_STACK;
		ctx->Output( "Unable to find assert entry point, possible corrupt stack...walking again anyway...\n\n" );

		// try again
		eError localRc = (regs != NULL) ? StackWalk( hprocess, hthread, *regs, localAddresses, 0 )
										: StackWalk( localAddresses, 0 );
		if ( IsFatal( localRc ) || addresses->empty() )
		{
			ctx->Output( "<Internal error collecting stack trace>\n" );
			return ( localRc );
		}
	}

	// convert the addresses and out to the dump target
	{
		AddressColl::const_iterator i, begin = addresses->begin(), end = addresses->end();
		for ( i = begin ; i != end ; ++i )
		{
			AddressToString( *i, ctx );
			ctx->Output( "\n" );
		}
	}

	// warn about stack corruption
	if ( rc == ERR_CORRUPT_STACK )
	{
		ctx->Output( "\n** Stack trace may be invalid! **\n\n" );
	}

	// warn about unfinished
	if ( rc == ERR_STACK_TOO_DEEP )
	{
		ctx->Output( "\n** Stack too deep, recursive? **\n" );
	}

	// success
	return ( rc );
}

gpstring DbgSymbolEngine :: AddressToModuleName( DWORD address ) const
{
	gpstring name;

	IMAGEHLP_MODULEX module;
	if ( m_Initialized && m_DbgHelpDll->SymGetModuleInfo( (HANDLE)this, address, &module ) )
	{
		// take what it says
		name = module.ImageName;
	}
	else
	{
		// debughelp failed, eh? try it the hard way...
		MEMORY_BASIC_INFORMATION mbi;
		gpstring moduleName;
		if ( ::VirtualQuery( (LPCVOID)address, &mbi, sizeof( mbi ) ) )
		{
			name = FileSys::GetModuleFileName( (HMODULE)mbi.AllocationBase );
		}
	}

	// postprocess
	if ( name.empty() )
	{
		return ( "<unknown module>" );
	}
	else
	{
		return ( FileSys::GetFileName( name ) );
	}
}

DWORD DbgSymbolEngine :: GetModuleBase( HANDLE process, DWORD address )
{
	HDbgHelpDll dbgHelpDll;
	IMAGEHLP_MODULEX module;

// Check in the symbol engine first.

	// check to see if the module is already loaded
	if ( dbgHelpDll->SymGetModuleInfo( s_CurrentEngine, address, &module ) )
	{
		return ( module.BaseOfImage );
	}

// The module is not loaded, so let's go fishing.

	// $ note: this auto-load code only works if we're working on symbols for
	//         a running process. if debugging symbols for a crash dump file
	//         (for example) need to analyze the crash file to determine what
	//         modules to load symbols for and where in memory they are located.

	MEMORY_BASIC_INFORMATION memInfo;

	// do the VirtualQueryEx to see if i can find the start of this module.
	//  since the HMODULE is the start of a module in memory, voila, this will
	//  give me the HMODULE.
	if ( !::VirtualQueryEx( process, (LPCVOID)address, &memInfo, sizeof( memInfo ) ) )
	{
		return ( 0 );
	}

	// using the address base, try to grab the module filename
	gpstring name = FileSys::GetModuleFileName( (HINSTANCE)memInfo.AllocationBase );

	// load the module
	return ( dbgHelpDll->SymLoadModule(
			s_CurrentEngine                                     ,
			NULL                                                ,
			ccast <char*> ( name.empty() ? NULL : name.c_str() ),
			NULL                                                ,
			(DWORD)memInfo.AllocationBase                       ,
			0                                                    ) );
}

//////////////////////////////////////////////////////////////////////////////
// helper class implementations

STACKFRAMEX :: STACKFRAMEX( const CONTEXT& context )
{
	::ZeroObject( *this );

	AddrPC.Mode      = AddrModeFlat;
	AddrPC.Offset    = context.Eip;
	AddrFrame.Mode   = AddrModeFlat;
	AddrFrame.Offset = context.Ebp;
	AddrStack.Mode   = AddrModeFlat;
	AddrStack.Offset = context.Esp;
}

//////////////////////////////////////////////////////////////////////////////
// class PsApiDll implementation

PsApiDll :: PsApiDll( void )
	: Inherited( "PSAPI.DLL" )
{
#	define ADD_ENTRY( f )  AddProc( &f, #f )

	ADD_ENTRY( GetProcessMemoryInfo );

#	undef ADD_ENTRY
}

//////////////////////////////////////////////////////////////////////////////
// class Kernel32Dll implementation

Kernel32Dll :: Kernel32Dll( void )
	: Inherited( "KERNEL32.DLL" )
{
#	define ADD_ENTRY( f )  AddProc( &f, #f )

	ADD_ENTRY( GetProcessPriorityBoost );
	ADD_ENTRY( GetThreadPriorityBoost  );
	ADD_ENTRY( GetThreadTimes          );

#	undef ADD_ENTRY
}

//////////////////////////////////////////////////////////////////////////////
// class HangBuddy implementation

HangBuddy :: HangBuddy(
		const DWORD* dumpKeys, int numDumpKeys,
		const DWORD* nukeKeys, int numNukeKeys,
		const DWORD* debugKeys, int numDebugKeys,
		DWORD periodMsec )
	: m_DumpKeys( dumpKeys, dumpKeys + numDumpKeys )
	, m_NukeKeys( nukeKeys, nukeKeys + numNukeKeys )
	, m_DebugKeys( debugKeys, debugKeys + numDebugKeys )
{
	m_PeriodMsec = periodMsec;
	m_KeysDown = false;

	// start watchdog
	BeginThread( "HangBuddy" );
}

HangBuddy :: ~HangBuddy( void )
{
	m_RequestDie.Set();
	WaitForExit( 5000 );
}

DWORD HangBuddy :: Execute( void )
{
	DWORD rc = 0xFFFFFFFF;

	while ( rc == 0xFFFFFFFF )
	{
		switch ( m_RequestDie.Wait( m_PeriodMsec ) )
		{
			case ( WAIT_OBJECT_0 ):
			{
				// somebody asked us to shut down via dtor
				rc = 0;
			}
			break;

			case ( WAIT_TIMEOUT ):
			{
				if ( AreKeysDown( m_DumpKeys ) && !m_KeysDown )
				{
					m_KeysDown = true;

					::OutputDebugString( "HangBuddy: Doing hang crash dump...\r\n" );

					bool success = false;
					__try
					{
						success = DumpThreadStacks();
					}
					__except( EXCEPTION_EXECUTE_HANDLER )
					{
						// $ ignore exceptions
					}

					if ( success )
					{
						::OutputDebugString( "HangBuddy: ...was successful!\r\n" );
						::MessageBeep( MB_OK );
					}
					else
					{
						::OutputDebugString( "HangBuddy: ...failed!\r\n" );
						::MessageBeep( MB_ICONEXCLAMATION );
					}
				}
				else if ( AreKeysDown( m_NukeKeys ) && !m_KeysDown )
				{
					m_KeysDown = true;
					::OutputDebugString( "HangBuddy: Nuking process...\r\n" );
					rc = 'Nuke';
					::ExitProcess( 'Nuke' );
				}
				else if ( AreKeysDown( m_DebugKeys ) && !m_KeysDown )
				{
					m_KeysDown = true;
					::OutputDebugString( "HangBuddy: Debug break...\r\n" );
					::DebugBreak();
				}
				else
				{
					m_KeysDown = false;
				}
			}
			break;

			default:
			{
				// there was an error with the wait function (WAIT_FAILED?)
				gpassert( 0 );
				rc = 'Err!';
			}
		}
	}

	return ( rc );
}

bool HangBuddy :: AreKeysDown( const KeyColl& coll ) const
{
	// time to check for hang keys again
	KeyColl::const_iterator i, begin = coll.begin(), end = coll.end();
	for (i = begin ; i != end ; ++i)
	{
		// msb not set if key is currently up
		if ( ::GetAsyncKeyState( *i ) >= 0 )
		{
			 break;
		}
	}

	// return true if (a) array is not empty, and (b) all keys were down
	return ( (begin != end) && (i == end) );
}

bool HangBuddy :: DumpThreadStacks( void ) const
{
	// pause all threads
	kerneltool::Thread::AutoPauser autoPauser;
	const kerneltool::Thread::TInfoDb& db = autoPauser.GetThreadInfoDb();

	// do minidump
	gpstring miniDumpName;
	gDbgSymbolEngine.MiniDump( false, NULL, &miniDumpName );
	DWORD miniDumpError = ::GetLastError();

	// build filename for report
	gpstring fileName = DrWatson::GetOutputDirectory();
	fileName += FileSys::GetFileNameOnly( FileSys::GetModuleFileName() );
	fileName += ".hang";
	fileName.insert( 0, FileSys::GetLocalWritableDir() );

	// build context and add sinks
	ReportSys::LocalContext context;
	context.AddSink( new ReportSys::FileSink( fileName, false ), true );

	// out generic session header
	ReportSys::FileSink::OutputGenericHeader( &context, "Hang Report" );

	// minidump?
	if ( !miniDumpName.empty() )
	{
		context.OutputF( "Minidump -> '%s'\n\n", miniDumpName.c_str() );
	}
	else
	{
		context.OutputF( "Minidump FAILED! Error is 0x%08X ('%s')\n\n",
						 miniDumpError,
						 stringtool::GetLastErrorText( miniDumpError ).c_str() );
	}

	// out process traits
	SeException::DumpProcessTraits( context );

	// only bother if something to do
	bool success = !db.empty();
	if ( success )
	{
		kerneltool::Thread::TInfoDb::const_iterator i, ibegin = db.begin(), iend = db.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// get the thread handle
			HANDLE h = i->second.m_Handle;

			// get the thread context
			CONTEXT regs;
			regs.ContextFlags = CONTEXT_FULL;
			if ( ::GetThreadContext( h, &regs ) )
			{
				// header
				context.OutputF( "*** Report for thread %s ***\n\n", i->second.m_Name.c_str() );

				// tack trace
				context.Indent();
				SeException::DumpContext( h, i->first, regs, &context );
				context.OutputEol();
				context.Outdent();
			}
			else
			{
				context.OutputF( "*** Unable to dump context for thread %s\n"
								 "    (OpenThread() may have failed or this is a Win98 system ***\n\n", i->second.m_Name.c_str() );
				success = false;
			}
		}
	}
	else
	{
		context.Output( "No auxiliary threads found, nothing to do!\n" );
	}

	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
