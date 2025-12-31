//////////////////////////////////////////////////////////////////////////////
//
// File     :  DebugHelp.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains various tools for assisting debugging
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __DEBUGHELP_H
#define __DEBUGHELP_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"
#include "DllBinder.h"
#include "GpColl.h"
#include "KernelTool.h"

#include <DbgHelp.h>
#include <PsApi.h>
#include <vector>

//////////////////////////////////////////////////////////////////////////////
// class DbgHelpDll declaration

// $ purpose: wrapper for DbgHelpDll.dll

class DbgHelpDll : public DllBinder, public RefCountSingleton <DbgHelpDll>
{
public:
	SET_INHERITED( DbgHelpDll, DllBinder );

	DbgHelpDll( void );

	DllProc3 <BOOL,								// SymInitialize(
			IN     HANDLE,							// hProcess,
			IN     LPSTR,							// UserSearchPath,
			IN     BOOL>							// fInvadeProcess )
		SymInitialize;

	DllProc1 <BOOL,								// SymCleanup(
			IN     HANDLE>							// hProcess )
		SymCleanup;

	DllProc1 <DWORD,							// SymSetOptions(
			IN     DWORD>							// SymOptions )
		SymSetOptions;

	DllProc0 <DWORD>							// SymGetOptions( void )
		SymGetOptions;

	DllProc2 <LPVOID,							// SymFunctionTableAccess(
			IN     HANDLE,							// hProcess,
			IN     DWORD>							// AddrBase )
		SymFunctionTableAccess;

	DllProc2 <DWORD,							// SymGetModuleBase(
			IN     HANDLE,							// hProcess,
			IN     DWORD>							// dwAddr )
		SymGetModuleBase;

	DllProc9 <BOOL,								// StackWalk(
			IN     DWORD,							// MachineType,
			IN     HANDLE,							// hProcess,
			IN     HANDLE,							// hThread,
			IN OUT LPSTACKFRAME,					// StackFrame,
			IN OUT LPVOID,							// ContextRecord,
			IN     PREAD_PROCESS_MEMORY_ROUTINE,	// ReadMemoryRoutine,
			IN     PFUNCTION_TABLE_ACCESS_ROUTINE,	// FunctionTableAccessRoutine,
			IN     PGET_MODULE_BASE_ROUTINE,		// GetModuleBaseRoutine,
			IN     PTRANSLATE_ADDRESS_ROUTINE>		// TranslateAddress )
		StackWalk;

	DllProc3 <BOOL,								// SymGetModuleInfo(
			IN     HANDLE,							// hProcess,
			IN     DWORD,							// dwAddr,
			OUT    PIMAGEHLP_MODULE>				// ModuleInfo )
		SymGetModuleInfo;

	DllProc4 <BOOL,								// SymGetSymFromAddr(
			IN     HANDLE,							// hProcess,
			IN     DWORD,							// dwAddr,
			OUT    PDWORD,							// pdwDisplacement,
			OUT    PIMAGEHLP_SYMBOL>				// Symbol )
		SymGetSymFromAddr;

	DllProc4 <BOOL,								// SymGetLineFromAddr(
			IN     HANDLE,							// hProcess,
			IN     DWORD,							// dwAddr,
			OUT    PDWORD,							// pdwDisplacement,
			OUT    PIMAGEHLP_LINE>					// Line )
		SymGetLineFromAddr;

	DllProc6 <DWORD,							// SymLoadModule(
			IN     HANDLE,							// hProcess,
			IN     HANDLE,							// hFile,
			IN     PSTR,							// ImageName,
			IN     PSTR,							// ModuleName,
			IN     DWORD,							// BaseOfDll,
			IN     DWORD>							// SizeOfDll )
		SymLoadModule;

	DllProc4 <DWORD,							// UnDecorateSymbolName(
			PCSTR,									// DecoratedName,
			PSTR,									// UnDecoratedName,
			DWORD,									// UndecoratedLength,
			DWORD>									// Flags )
		UnDecorateSymbolName;

	DllProc7 <BOOL,								// MiniDumpWriteDump(
			HANDLE,									// hProcess,
			DWORD,									// ProcessId,
			HANDLE,									// hFile,
			MINIDUMP_TYPE,							// DumpType,
			PMINIDUMP_EXCEPTION_INFORMATION,		// ExceptionParam,
			PMINIDUMP_USER_STREAM_INFORMATION,		// UserStreamParam,
			PMINIDUMP_CALLBACK_INFORMATION>			// CallbackParam )
		MiniDumpWriteDump;

private:
	SET_NO_COPYING( DbgHelpDll );
};

typedef DbgHelpDll::Handle HDbgHelpDll;

//////////////////////////////////////////////////////////////////////////////
// helper class declarations

struct STACKFRAMEX : STACKFRAME
{
	STACKFRAMEX( void )  {  ::ZeroObject( *this );  }
	STACKFRAMEX( const CONTEXT& context );
};

template <const int NAMELEN = 256>
struct IMAGEHLP_SYMBOLX : IMAGEHLP_SYMBOL
{
	COMPILER_ASSERT( NAMELEN > 1 );
	CHAR NameExtra[ NAMELEN - 1 ];

	IMAGEHLP_SYMBOLX( void )
	{
		::ZeroObject( *this );

		SizeOfStruct  = sizeof( IMAGEHLP_SYMBOL );
		MaxNameLength = NAMELEN;
	}
};

struct IMAGEHLP_MODULEX : IMAGEHLP_MODULE
{
	IMAGEHLP_MODULEX( void )
	{
		::ZeroObject( *this );
		SizeOfStruct = sizeof( IMAGEHLP_MODULE );
	}
};

struct IMAGEHLP_LINEX : IMAGEHLP_LINE
{
	IMAGEHLP_LINEX( void )
	{
		::ZeroObject( *this );
		SizeOfStruct = sizeof( IMAGEHLP_LINE );
	}
};

//////////////////////////////////////////////////////////////////////////////
// class DbgSymbolEngine declaration

class DbgSymbolEngine
{
public:
	SET_NO_INHERITED( DbgSymbolEngine );

	DbgSymbolEngine( void );
   ~DbgSymbolEngine( void );

	bool IsInitialized( void ) const					{  return ( m_Initialized );  }

	typedef std::vector <ULONG> AddressColl;
	typedef stdx::fast_vector <gpstring> StringVec;
	typedef CBFunctor3wRet <DWORD /*threadId*/, gpstring& /*stackName*/, StringVec& /*stack*/, bool> StackQueryCb;
	typedef stdx::fast_vector <StackQueryCb> StackQueryCbColl;

	enum eError
	{
		// success codes
		ERR_SUCCESS,				// call succeeded

		// warning codes
		ERR_CORRUPT_STACK,			// don't trust the walk, it may be corrupt stack
		ERR_STACK_TOO_DEEP,			// stack was too deep to walk all the way, bailed early
		ERR_BUFFER_TOO_SMALL,		// output buffer too small to store info

		// error codes
		ERR_INTERNAL,				// internal symbol engine error - see GetLastError() for more info
		ERR_UNSUPPORTED,			// unsupported functionality
	};

	static bool IsFatal( eError error )					{  return ( error >= ERR_INTERNAL );  }

	eError StackWalk( HANDLE hprocess, HANDLE hthread, const CONTEXT& regs, AddressColl& addresses, DWORD startFrame = 0 );
	eError StackWalk( AddressColl& addresses, DWORD startFrame = 0 );

	eError StackTrace( HANDLE hprocess, HANDLE hthread, DWORD threadId, const CONTEXT& regs, ReportSys::ContextRef ctx = NULL, DWORD startFrame = 0 );
	eError StackTrace( HANDLE hprocess, HANDLE hthread, DWORD threadId, const AddressColl& addresses, ReportSys::ContextRef ctx = NULL, DWORD startFrame = 0 );
	eError StackTrace( ReportSys::ContextRef ctx = NULL, DWORD startFrame = 0 );

	bool AddressToString  ( DWORD address, ReportSys::ContextRef context = NULL );
	bool AddressToSymName ( gpstring& name, DWORD address, bool withParams = false );
	bool AddressToSymbol  ( IMAGEHLP_SYMBOLX <>& symbol, DWORD address, bool withParams = false );
	bool AddressToLocation( gpstring& fileName, DWORD& line, DWORD address );

	bool LoadModuleSymbols       ( const char* imageName, DWORD imageBase = 0 );
	bool LoadCurrentModuleSymbols( void );

	bool MiniDump( bool verbose, const EXCEPTION_POINTERS* xinfo, gpstring* outFileName = NULL );

	// custom game-specific "stack" translators (use for scripting etc.)
	static void RegisterStackQueryCb  ( StackQueryCb cb );
	static void UnregisterStackQueryCb( StackQueryCb cb );
	static bool DumpGameStack         ( DWORD threadId, ReportSys::ContextRef ctx = NULL );
	static bool DumpGameStack         ( ReportSys::ContextRef ctx = NULL )	{  return ( DumpGameStack( ::GetCurrentThreadId(), ctx ) );  }

private:
	eError   PrivateStackTrace( HANDLE hprocess, HANDLE hthread, DWORD threadId, const CONTEXT* regs, ReportSys::ContextRef ctx, DWORD startFrame, const AddressColl* addresses );
	gpstring AddressToModuleName( DWORD address ) const;

	static DWORD __stdcall GetModuleBase( HANDLE process, DWORD addr );

	bool        m_Initialized;
	HDbgHelpDll m_DbgHelpDll;

	static StackQueryCbColl ms_StackQueryCbColl;

	SET_NO_COPYING( DbgSymbolEngine );
};

class DbgSymbolEngineSingleton : public OnDemandSingleton <DbgSymbolEngine>  {  };
#define gDbgSymbolEngine DbgSymbolEngineSingleton::GetSingleton()

//////////////////////////////////////////////////////////////////////////////
// class PsApiDll declaration

class PsApiDll : public DllBinder, public OnDemandSingleton <PsApiDll>
{
public:
	SET_INHERITED( PsApiDll, DllBinder );

	PsApiDll( void );

	DllProc3 <BOOL,							// GetProcessMemoryInfo(
			HANDLE,								// Process
			PPROCESS_MEMORY_COUNTERS,			// ppsmemCounters
			DWORD>								// cb )
		GetProcessMemoryInfo;

	DllProc4 <BOOL,							// GetModuleInformation(
			HANDLE,								// hProcess
			HMODULE,							// hModule
			LPMODULEINFO,						// lpmodinfo
			DWORD>								// cb )
		GetModuleInformation;

private:
	SET_NO_COPYING( PsApiDll );
};

#define gPsApiDll PsApiDll::GetSingleton()

//////////////////////////////////////////////////////////////////////////////
// class Kernel32Dll declaration

class Kernel32Dll : public DllBinder, public OnDemandSingleton <Kernel32Dll>
{
public:
	SET_INHERITED( Kernel32Dll, DllBinder );

	Kernel32Dll( void );

	DllProc2 <BOOL,							// GetProcessPriorityBoost(
			HANDLE,								// hProcess
			PBOOL>								// pDisablePriorityBoost
		GetProcessPriorityBoost;

	DllProc2 <BOOL,							// GetThreadPriorityBoost(
			HANDLE,								// hThread
			PBOOL>								// pDisablePriorityBoost
		GetThreadPriorityBoost;

	DllProc5 <BOOL,							// GetThreadTimes(
			HANDLE,								// hThread
			LPFILETIME,							// lpCreationTime
			LPFILETIME,							// lpExitTime
			LPFILETIME,							// lpKernelTime
			LPFILETIME>							// lpUserTime
		GetThreadTimes;

private:
	SET_NO_COPYING( Kernel32Dll );
};

#define gKernel32Dll Kernel32Dll::GetSingleton()

//////////////////////////////////////////////////////////////////////////////
// class HangBuddy declaration

/*
	This class implements a watchdog thread that sits and waits for certain
	keys to be held down. When the user thinks they've got a hang, they can
	press some keys which will pause all threads, do a crash dump of each, and
	then resume them all. Also may act as an assassin.
*/

class HangBuddy : public kerneltool::Thread
{
public:
	SET_INHERITED( HangBuddy, kerneltool::Thread );

	// pass in an array of VK codes for various needs. these all must be pressed
	// simultaneously and held down for at least the period in order to be
	// caught.

	HangBuddy(
		const DWORD* dumpKeys, int numDumpKeys,		// keys to do a hang stack dump
		const DWORD* nukeKeys, int numNukeKeys,		// keys to nuke the game
		const DWORD* debugKeys, int numDebugKeys,	// keys to cause a debug break
		DWORD periodMsec );							// how often to wake up and check for keys

	virtual ~HangBuddy( void );

private:
	typedef std::vector <DWORD> KeyColl;

	DWORD Execute( void );
    bool  AreKeysDown( const KeyColl& coll ) const;
	bool  DumpThreadStacks( void ) const;

	// spec
	KeyColl m_DumpKeys;
	KeyColl m_NukeKeys;
	KeyColl m_DebugKeys;
	DWORD   m_PeriodMsec;

	// state
    kerneltool::Event m_RequestDie;					// request to die
	bool m_KeysDown;								// keys were down last time we checked

	SET_NO_COPYING( HangBuddy );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __DEBUGHELP_H

//////////////////////////////////////////////////////////////////////////////
