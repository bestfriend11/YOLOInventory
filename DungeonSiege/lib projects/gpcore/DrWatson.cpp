//////////////////////////////////////////////////////////////////////////////
//
// File     :  DrWatson.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"

#include "AppModule.h"
#include "DrWatson.h"
#include "FileSysUtils.h"
#include "StringTool.h"

// watson include
#include "msodw.h"

//////////////////////////////////////////////////////////////////////////////

namespace DrWatson {

//////////////////////////////////////////////////////////////////////////////

static eOptions s_Options
	= OPTION_ENABLE
#	if GP_USE_DRWATSON_ONLY
	| OPTION_FORCE_ENABLE
#	endif // GP_USE_DRWATSON_ONLY
	;

static const int MAX_FILES = 15;
static char s_RegisteredFiles[ MAX_FILES ][ MAX_PATH ];

typedef stdx::fast_vector <InfoCb> InfoCbColl;
static InfoCbColl s_RegisteredInfoCbs;
static gpstring s_OutputDirectory = FileSys::GetLocalWritableDir();

void SetOptions( eOptions options, bool set )
{
	s_Options = (eOptions)(set ? (s_Options | options) : (s_Options & ~options));
}

void ClearOptions( eOptions options )
{
	SetOptions( options, false );
}

bool TestOptions( eOptions options )
{
	return ( (s_Options & options) != 0 );
}

void RegisterFileForReport( const char* fileName )
{
	if ( fileName && *fileName )
	{
		for ( int i = 0 ; i != MAX_FILES ; ++i )
		{
			char* str = (char*)s_RegisteredFiles + (i * MAX_PATH);
			if ( ::same_no_case( str, fileName ) )
			{
				break;
			}
			if ( *str == '\0' )
			{
				::strcpy( str, fileName );
				break;
			}
		}
	}
}

void RegisterInfoCallback( InfoCb cb, bool front )
{
	if ( front )
	{
		s_RegisteredInfoCbs.insert( s_RegisteredInfoCbs.begin(), cb );
	}
	else
	{
		s_RegisteredInfoCbs.push_back( cb );
	}
}

#pragma warning ( disable : 4509 )

void ExecuteInfoCallbacks( Info& info )
{
	for ( int index = 0 ; index < (int)s_RegisteredInfoCbs.size() ; ++index )
	{
		__try
		{
			s_RegisteredInfoCbs[ index ]( info );
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
			// build output filename
			char fileName[ MAX_PATH ];
			::strcpy( fileName, GetOutputDirectory() );
			::strcat( fileName, "InfoCbFailure.txt" );

			// open the file
			FileSys::File file;
			if ( file.OpenAlways( fileName, FileSys::File::ACCESS_READ_WRITE ) )
			{
				// header
				file.WriteText( "[Info]\n\n" );

				// timestamp
				SYSTEMTIME st;
				::GetLocalTime( &st );
				file.WriteF(
						"    %d/%d/%d %02d:%02d:%02d %s\n",
						st.wMonth, st.wDay, st.wYear,
						(st.wHour > 12) ? (st.wHour - 12) : (st.wHour ? st.wHour : 12), st.wMinute, st.wSecond,
						(st.wHour >= 12) ? "PM" : "AM" );

				// xinfo
				DWORD* data = (DWORD*)&s_RegisteredInfoCbs[ index ];
				file.WriteF(
						"    Exception occurred on callback %d, data = 0x%08X 0x%08X 0x%08X\n",
						index, data[ 0 ], data[ 1 ], data[ 2 ] );

				// add file
				RegisterFileForReport( fileName );
			}
		}
	}
}

void InitOutputDirectory( const char* path )
{
	s_OutputDirectory = path ? path : FileSys::GetLocalWritableDir();
	stringtool::AppendTrailingBackslash( s_OutputDirectory );
}

const gpstring& GetOutputDirectory( void )
{
	return ( s_OutputDirectory );
}

//
//
// Trigger Watson - passed the exception parameters
//
// 0=Problem with watson
// 1=User chose Debug (return EXCEPTION_CONTINUE_SEARCH so debugger will pick it up)
// 2=User chose Quit
//

LONG WINAPI ExceptionFilter( const EXCEPTION_POINTERS *pep )
{
	EXCEPTION_RECORD *per = pep->ExceptionRecord;

//
// If a 'debugging' exception, don't use Watson
//
	if (  (EXCEPTION_BREAKPOINT   == per->ExceptionCode)
		|| (EXCEPTION_SINGLE_STEP == per->ExceptionCode) )
	{
		return ( EXCEPTION_CONTINUE_SEARCH );
	}

//
// Setup shared memory to communicate to Watson
//
	HANDLE hFileMap;
	DWSharedMem *pdwsm;
	SECURITY_ATTRIBUTES sa;

	memset( &sa, 0, sizeof( sa ) );
	sa.nLength = sizeof( sa );
	sa.bInheritHandle = TRUE;
	
	hFileMap = CreateFileMapping( INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, sizeof( DWSharedMem ), NULL );
	if ( hFileMap == NULL )
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
		
	pdwsm = (DWSharedMem*) MapViewOfFile( hFileMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0 );
	if( pdwsm==NULL )
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	memset( pdwsm, 0, sizeof(DWSharedMem) );

//
// Create Muxtexs used to communicate with Watson
//
	HANDLE hEventDone = CreateEvent(&sa, FALSE, FALSE, NULL);	// event DW signals when done
	HANDLE hEventAlive = CreateEvent(&sa, FALSE, FALSE, NULL);	// heartbeat event DW signals per EVENT_TIMEOUT
	HANDLE hMutex = CreateMutex(&sa, FALSE, NULL);              // to protect the signaling of EventDone  

	if ( !DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(),GetCurrentProcess(), &pdwsm->hProc, PROCESS_ALL_ACCESS, TRUE, 0) )
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (hEventAlive == NULL || hEventDone == NULL || hMutex == NULL )
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

//
// Fill in Watson structure information
//
	pdwsm->pid = GetCurrentProcessId();
	pdwsm->tid = GetCurrentThreadId();
	pdwsm->hEventAlive = hEventAlive;
	pdwsm->hEventDone = hEventDone;
	pdwsm->hMutex = hMutex;
	pdwsm->dwSize = sizeof(DWSharedMem);
	pdwsm->pep = (EXCEPTION_POINTERS*)pep;
	pdwsm->eip = (DWORD) pep->ExceptionRecord->ExceptionAddress;

	#ifdef _DEBUG
	pdwsm->bfmsoctdsOffer = msoctdsQuit | msoctdsDebug;
	#else
	pdwsm->bfmsoctdsOffer = msoctdsQuit;
	#endif

	pdwsm->bfDWBehaviorFlags = fDwUseHKLM;

	if ( AppModule::DoesSingletonExist() )
	{
		wcscpy(pdwsm->wzFormalAppName, gAppModule.GetAppName() );
	}
	
	char AppName[MAX_PATH];
	GetModuleFileNameA(NULL, AppName, DW_MAX_PATH);
	mbstowcs( pdwsm->wzModuleFileName, AppName, strlen(AppName) );

	strcpy(pdwsm->szRegSubPath, "Microsoft\\PCHealth\\ErrorReporting\\DW");

	pdwsm->bfmsoctdsLetRun = msoctdsQuit | msoctdsDebug;

//
// Include requested files in report.
//
	{
		char additionalFiles[ ELEMENT_COUNT( pdwsm->wzAdditionalFile ) ] = "";

		for ( int i = 0 ; i != MAX_FILES ; ++i )
		{
			char* str = (char*)s_RegisteredFiles + (i * MAX_PATH);
			if ( *str == '\0' )
			{
				break;
			}

			if ( FileSys::File().OpenExisting( str ) )
			{
				if ( additionalFiles[ 0 ] != '\0' )
				{
					strcat( additionalFiles, "|" );
				}
				strcat( additionalFiles, str );
			}
		}

		// build list of files to upload with watson
		::MultiByteToWideChar( CP_ACP, 0, additionalFiles, -1, pdwsm->wzAdditionalFile, ELEMENT_COUNT( pdwsm->wzAdditionalFile ) );
	}

//
// Now start watson
//
	char szCommandLine[MAX_PATH];
	DWORD dw;
	BOOL fDwRunning = TRUE;  
	PROCESS_INFORMATION	piInfo;
	STARTUPINFO			siInfo;
		
	memset(&siInfo, 0, sizeof(siInfo));
	siInfo.cb = sizeof(siInfo);
	memset(&piInfo, 0, sizeof(piInfo));
	
	wsprintfA( szCommandLine,"dw15.exe -x -s %u", (DWORD)hFileMap );
	LONG RetCode = EXCEPTION_CONTINUE_SEARCH;

	__try
	{
		if( CreateProcessA(NULL, szCommandLine, NULL, NULL, TRUE, CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, NULL, NULL, &siInfo, &piInfo) )
		{
			while( fDwRunning )
			{
				if( WaitForSingleObject(hEventAlive, DW_TIMEOUT_VALUE ) == WAIT_OBJECT_0 )
				{
					if (WaitForSingleObject(hEventDone, 1) == WAIT_OBJECT_0)
					{
						fDwRunning = FALSE;
					}
					continue;
				}
					
				 // we timed-out waiting for DW to respond, try to quit
				dw = WaitForSingleObject(hMutex, DW_TIMEOUT_VALUE);
				if( dw == WAIT_TIMEOUT )
				{
					fDwRunning = FALSE; // either DW's hung or crashed, we must carry on  
				}
				else
				{
					if( dw == WAIT_ABANDONED )
					{
						fDwRunning = FALSE;
						ReleaseMutex(hMutex);
					}
					else
					{
						// DW has not woken up?
						// tell DW we're through waiting for it's sorry self
						if( WaitForSingleObject(hEventAlive, 1) != WAIT_OBJECT_0 )
						{
							SetEvent(hEventDone);
							fDwRunning = FALSE;
						}
						else
						{
						// are we done
							if( WaitForSingleObject(hEventDone, 1) == WAIT_OBJECT_0 )
							{
								fDwRunning = FALSE;
							}
						}
						ReleaseMutex(hMutex);
					}
				}
			}

			CloseHandle( piInfo.hProcess );
			CloseHandle( piInfo.hThread );
			CloseHandle( hEventAlive );
			CloseHandle( hEventDone );
			CloseHandle( hMutex );

			RetCode = pdwsm->msoctdsResult & msoctdsQuit ? 2 : 1;
		}
	}
	__except(1)
	{
	}
	
	UnmapViewOfFile(pdwsm);
	CloseHandle(hFileMap);

	return RetCode;
}


//////////////////////////////////////////////////////////////////////////////

} // end of namespace DrWatson

//////////////////////////////////////////////////////////////////////////////
