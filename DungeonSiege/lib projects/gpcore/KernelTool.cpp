//////////////////////////////////////////////////////////////////////////////
//
// File     :  KernelTool.cpp (GPCore)
// Author(s):  Scott Bilas
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "AppModule.h"
#include "KernelTool.h"
#include "GpMath.h"

#include <Process.h>
#include <TlHelp32.h>

#if KERNEL_CRITICAL_TRACK_LOCKS
kerneltool::Critical g_CriticalLockedRoot( true );
kerneltool::RwCritical g_RwCriticalLockedRoot( true );
#endif // KERNEL_CRITICAL_TRACK_LOCKS

#if KERNEL_CRITICAL_TIME_LOCKS
double KERNEL_CRITICAL_MAX_WAIT_TIME_PRI = 0.001;
double KERNEL_CRITICAL_MAX_WAIT_TIME_AUX = 0.250;
bool   KERNEL_CRITICAL_TIME_BREAKPOINT   = true;
#endif // KERNEL_CRITICAL_TIME_LOCKS

namespace kerneltool  {  // begin of namespace kerneltool

//////////////////////////////////////////////////////////////////////////////
// class Optex implementation

// $ this is based on Jeffrey Richter's work in MSDN (see OPTEX)

Optex :: Optex( void )
	: m_Event( false, false )
{
	m_LockCount    = -1;	// no threads have entered the Optex
	m_ThreadId     =  0;	// the Optex is unowned
	m_RecurseCount =  0;	// the Optex is unowned
}

DWORD Optex :: Enter( DWORD milliseconds ) const
{
	// the calling thread's ID
	DWORD threadId = ::GetCurrentThreadId();

	// assume that the thread waits successfully
	DWORD rc = WAIT_OBJECT_0;

	if ( ::InterlockedIncrement( &m_LockCount ) == 0 )
	{
		// no thread owns me
		m_ThreadId = threadId;		// we own it
		m_RecurseCount = 1;			// we own it once
	}
	else
	{
		// some thread owns me
		if ( m_ThreadId == threadId )
		{
			// current thread already owns me
			++m_RecurseCount;		// we own it again
		}
		else
		{
			// another thread owns me, wait for it to release
			rc = m_Event.Wait( milliseconds );
			if ( rc != WAIT_TIMEOUT )
			{
				// current thread got ownership of me
				m_ThreadId = threadId;		// we own it now
				m_RecurseCount = 1;			// we own it once
			}
		}
	}

	// return why we continue execution
	return ( rc );
}

void Optex :: Leave( void ) const
{
	// only call this func after a successful Enter()
	gpassert( m_ThreadId == ::GetCurrentThreadId() );

	if ( --m_RecurseCount > 0 )
	{
		// still owned
		::InterlockedDecrement( &m_LockCount );
	}
	else
	{
		// no longer owned
		m_ThreadId = 0;
		if ( ::InterlockedDecrement( &m_LockCount ) >= 0 )
		{
			// other threads are waiting, wake one on them
			m_Event.Set();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// class Critical implementation

#if KERNEL_CRITICAL_TRACK_LOCKS

Critical :: Critical( bool )
	: Inherited( true )
{
	::InitializeCriticalSection( &m_Critical );
}

#endif // KERNEL_CRITICAL_TRACK_LOCKS

//////////////////////////////////////////////////////////////////////////////
// class RwCritical implementation

static const DWORD s_EnterFunc = (DWORD)&Critical::StaticEnter;
static const DWORD s_LeaveFunc = (DWORD)&Critical::StaticLeave;
static const DWORD s_RwCriticalOffset = sizeof( Tracker <RwCritical> );

RwCritical::Proc RwCritical::ms_EnterRead  = SysInfo::IsMultiProcessor() ? RwCritical::MP_EnterRead  : RwCritical::SP_EnterRead;
RwCritical::Proc RwCritical::ms_LeaveRead  = SysInfo::IsMultiProcessor() ? RwCritical::MP_LeaveRead  : RwCritical::SP_LeaveRead;
RwCritical::Proc RwCritical::ms_EnterWrite = SysInfo::IsMultiProcessor() ? RwCritical::MP_EnterWrite : RwCritical::SP_EnterWrite;
RwCritical::Proc RwCritical::ms_LeaveWrite = SysInfo::IsMultiProcessor() ? RwCritical::MP_LeaveWrite : RwCritical::SP_LeaveWrite;

#if KERNEL_CRITICAL_TRACK_LOCKS

RwCritical :: RwCritical( bool )
	: Inherited( true )
{
	gpassert( ((DWORD)&m_ReadLockCount  - ((DWORD)this + sizeof( Tracker <RwCritical> ))) == 0 );
	gpassert( ((DWORD)&m_WriteLockCount - ((DWORD)this + sizeof( Tracker <RwCritical> ))) == 4 );
	gpassert( ((DWORD)&m_WriteLock      - ((DWORD)this + sizeof( Tracker <RwCritical> ))) == 8 );
}

#endif // KERNEL_CRITICAL_TRACK_LOCKS

#define KERNELTOOLASM_SP
#include "KernelToolAsm.cpp"
#undef KERNELTOOLASM_SP

#define KERNELTOOLASM_MP
#include "KernelToolAsm.cpp"
#undef KERNELTOOLASM_MP

//////////////////////////////////////////////////////////////////////////////
// class Thread implementation

#ifdef _MT

// static constants
const Thread::eOptions Thread::OPTION_DEF = Thread::OPTION_NONE;

// static members
Critical        Thread::ms_Critical;
Thread::TInfoDb Thread::ms_TInfoDb;

bool Thread :: BeginThread( const char* threadName )
{
	// can't begin twice. call reset first.
	gpassert( IsNull() );

	// reset this to the "unknown" value
	m_LastReturn = 0xFFFFFFFF;

	// take the name
	m_Name = threadName ? threadName : "";

	// create the thread. note that _beginthreadex() does not automatically
	//  close the handle (_beginthread() does, so don't use it).
	SetHandle( rcast <HANDLE> ( _beginthreadex(
			NULL,                     // security (not used)
			0,                        // stack size (use default)
			StaticThreadProc,         // thread function
			rcast <void*> (this),     // arg to thread function so we know "this"
			0,                        // start thread immediately
			&m_ID )));                // target for the thread id

	// success if the thread isn't null
	bool success = !IsNull();

	// if valid, wait for it to start (maybe)
	if ( success && TestOptions( OPTION_BEGIN_THREAD_WAIT ) )
	{
		WaitForStart();
	}

	// done
	return ( success );
}

void Thread :: SetThreadName( const char* threadName )
{
	// first must wait for it to actually start if it hasn't already
	WaitForStart();

	// take it
	m_Name = threadName ? threadName : "";

	// set it
	SetThreadName( m_ID, threadName );
}

bool Thread :: RegisterCurrentThread( const char* name )
{
	return ( RegisterThread( ::GetCurrentThreadId(), ::GetCurrentThread(), name ) );
}

bool Thread :: RegisterThread( DWORD id, HANDLE h, const char* name )
{
	Critical::Lock locker( ms_Critical );

	bool success = true;

	// insert/lookup
	std::pair <TInfoDb::iterator, bool> rc = ms_TInfoDb.insert( std::make_pair( id, TInfo() ) );
	if ( rc.second )
	{
		// set data for new thread entry
		TInfo& th = rc.first->second;

		// dup handle locally
		if ( !::DuplicateHandle( ::GetCurrentProcess(),
								 h,
								 ::GetCurrentProcess(),
								 &th.m_Handle,
								 0,
								 FALSE,
								 DUPLICATE_SAME_ACCESS ) )
		{
			// failed, wtf? - remove the entry
			th.Release();
			ms_TInfoDb.erase( rc.first );
			success = false;
		}

		// set the name of this thread always
		SetThreadName( id, name );
	}
	else
	{
		// already there, verify name hasn't changed (that must be done using
		// SetThreadName)
		gpassert( rc.first->second.m_Name.same_with_case( name ? name : "" ) );
	}

	return ( success );
}

void Thread :: UnregisterCurrentThread( void )
{
	UnregisterById( ::GetCurrentThreadId() );
}

void Thread :: UnregisterByName( const char* name )
{
	Critical::Lock locker( ms_Critical );

	TInfoDb::iterator i, begin = ms_TInfoDb.begin(), end = ms_TInfoDb.end();
	for ( i = begin ; i != end ; )
	{
		if ( i->second.m_Name.same_no_case( name ) )
		{
			i = ms_TInfoDb.erase( i );
		}
		else
		{
			++i;
		}
	}
}

void Thread :: UnregisterById( DWORD id )
{
	Critical::Lock locker( ms_Critical );

	TInfoDb::iterator found = ms_TInfoDb.find( id );
	if ( found != ms_TInfoDb.end() )
	{
		ms_TInfoDb.erase( found );
	}
}

static void InternalSetThreadName( DWORD id, const char* name )
{
	/*	Here is a little bit of hacky code from Tony Goodhew (VC++ PM) talk in
		Amsterdam for TechEd '99. This uses an undocumented feature of VC++ to
		"name" the threads for easier debugging. Supports names up to 9 chars.
	*/

#	if GP_RETAIL

	UNREFERENCED_PARAMETER( id );
	UNREFERENCED_PARAMETER( name );

#	else // GP_RETAIL

	struct THREADNAME_INFO
	{
		DWORD	dwType;			// must be 0x1000
		LPCSTR	szName;			// pointer to name (in user addr space)
		DWORD	dwThreadID;		// thread ID
		DWORD	dwFlags;		// reserved for future use, must be zero

		THREADNAME_INFO( DWORD id, const char* name )
		{
			dwType     = 0x1000;
			szName     = name ? name : "User";
			dwThreadID = id;
			dwFlags    = 0;
		}
	};

	__try
	{
		THREADNAME_INFO info( id, name );
		::RaiseException( 0x406D1388, 0, sizeof( info ) / sizeof( DWORD ), (DWORD*)&info );
	}
	__except( EXCEPTION_CONTINUE_EXECUTION )
	{
		// this space intentionally left blank...
	}

#	endif // GP_RETAIL
}

void Thread :: SetThreadName( DWORD id, const char* name )
{
	Critical::Lock locker( ms_Critical );

	TInfoDb::iterator found = ms_TInfoDb.find( id );
	if ( found != ms_TInfoDb.end() )
	{
		found->second.m_Name = name;
	}

	InternalSetThreadName( id, name );
}

bool Thread :: GetThreadIds( TInfoDb& db, bool includeMyThread )
{
	HANDLE hsnapshot = ::CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
	if ( hsnapshot == (HANDLE)-1 )
	{
		return ( false );
	}

	DWORD myProcessId = ::GetCurrentProcessId();
	DWORD myThreadId  = ::GetCurrentThreadId ();

	THREADENTRY32 entry;
	entry.dwSize = sizeof( entry );

	for (  BOOL success = ::Thread32First( hsnapshot, &entry )
		 ; success
		 ; success = ::Thread32Next(hsnapshot, &entry) )
	{
		// for each thread belonging to this process
		if ( entry.th32OwnerProcessID == myProcessId )
		{
			// if it's not my thread or that's ok
			if ( includeMyThread || (entry.th32ThreadID != myThreadId) )
			{
				// store it (deref all we need)
				db[ entry.th32ThreadID ];
			}
		}
	}

	::CloseHandle( hsnapshot );
	return ( true );
}

bool Thread :: GetThreads( TInfoDb& db, bool includeMyThread )
{
	gpassert( db.empty() );

	// attempt to get OpenThread() - not supported in Win98, but WinME/Win2k ok
	typedef HANDLE (WINAPI* OpenThreadProc)( DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId );
	static OpenThreadProc s_OpenThreadProc = (OpenThreadProc)::GetProcAddress( ::LoadLibrary( "KERNEL32.DLL" ), "OpenThread" );

	// start with registered threads
	GetRegisteredThreads( db, includeMyThread );

	// get our thread id's
	TInfoDb iddb;
	if ( !GetThreadIds( iddb, includeMyThread ) )
	{
		return ( false );
	}

	bool success = true;

	// fill in unregistered threads
	TInfoDb::const_iterator i, begin = iddb.begin(), end = iddb.end();
	for ( i = begin ; i != end ; ++i )
	{
		// access entry, open it if new and not Win98
		TInfo& info = db[ i->first ];
		if ( (info.m_Handle == NULL) && (s_OpenThreadProc != NULL) )
		{
			// dup the handle
			HANDLE h = (*s_OpenThreadProc)( THREAD_ALL_ACCESS, FALSE, i->first );
			if ( h != NULL )
			{
				info.m_Handle = h;
			}
			else
			{
				success = false;
			}
		}
	}

	// done
	return ( success );
}

void Thread :: GetRegisteredThreads( TInfoDb& db, bool includeMyThread )
{
	// copy
	{
		Critical::Lock locker( ms_Critical );
		db = ms_TInfoDb;
	}

	// remove local thread if it's there and we don't want it
	if ( !includeMyThread )
	{
		TInfoDb::iterator found = db.find( ::GetCurrentThreadId() );
		if ( found != db.end() )
		{
			db.erase( found );
		}
	}
}

gpstring Thread :: FindCurrentThreadName( void )
{
	return ( FindThreadNameById( ::GetCurrentThreadId() ) );
}

gpstring Thread :: FindThreadNameById( DWORD id )
{
	Critical::Lock locker( ms_Critical );

	TInfoDb::iterator found = ms_TInfoDb.find( id );
	if ( found != ms_TInfoDb.end() )
	{
		return ( found->second.m_Name );
	}

	return ( gpstring::EMPTY );
}

gpstring Thread :: MakeCurrentThreadName( void )
{
	return ( MakeThreadNameById( ::GetCurrentThreadId() ) );
}

gpstring Thread :: MakeThreadNameById( DWORD id )
{
	gpstring str = FindThreadNameById( id );
	if ( str.empty() )
	{
		str.assignf( "#0x%X", id );
	}
	return ( str );
}

unsigned int __stdcall Thread :: StaticThreadProc( void* arg )
{
	// this should never happen
	gpassert( arg != NULL );

	// set our FPU state consistent with other user-defined threads
	if ( AppModule::DoesSingletonExist() && gAppModule.TestOptions( AppModule::OPTION_FAST_FTOL ) )
	{
		SetFpuChopMode();
	}

	// call thread proc
	return ( rcast <Thread*> ( arg )->ThreadProc() );
}

unsigned int __stdcall Thread :: ThreadProc( void )
{
	// register me
	RegisterCurrentThread( m_Name );

	// notify we've started
	m_StartupEvent.Set();

	__try
	{
		__try
		{
			// call the dynamic function
			m_LastReturn = Execute();
		}
		__finally
		{
			// unregister the thread no matter what
			UnregisterCurrentThread();
		}
	}
	__except( ::GlobalExceptionFilter( GetExceptionInformation(),
			  TestOptions( OPTION_IGNORE_EXCEPTIONS ) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH,
			  TestOptions( OPTION_IGNORE_EXCEPTIONS ) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH,
			  true ) )
	{
		// $ just ignore and continue
		m_LastReturn = GetExceptionCode();
	}

	DWORD dwResult = m_LastReturn;

	// optionally delete "this" (special case only!)
	if ( TestOptions( OPTION_DELETE_SELF ) )
	{
		delete ( this );
	}

	// return last result - use local copy so as not to deref deleted memory
	return ( dwResult );
}

#endif // _MT

//////////////////////////////////////////////////////////////////////////////
// struct Thread::TInfo implementation

#ifdef _MT

Thread::TInfo :: TInfo( void )
{
	m_Handle = NULL;
}

Thread::TInfo :: TInfo( const TInfo& info )
	: m_Name( info.m_Name )
{
	// init
	m_Handle = NULL;

	// copy handle
	if ( info.m_Handle != NULL )
	{
		::DuplicateHandle( ::GetCurrentProcess(),
						   info.m_Handle,
						   ::GetCurrentProcess(),
						   &m_Handle,
						   0,
						   FALSE,
						   DUPLICATE_SAME_ACCESS );
	}
}

Thread::TInfo :: ~TInfo( void )
{
	Reset();
}

HANDLE Thread::TInfo :: Release( void )
{
	HANDLE h = m_Handle;
	m_Handle = NULL;
	return ( h );
}

void Thread::TInfo :: Reset( void )
{
	if ( m_Handle != NULL )
	{
		::CloseHandle( m_Handle );
	}

	m_Name.clear();
}

Thread::TInfo& Thread::TInfo :: operator = ( const TInfo& info )
{
	// safety...
	if ( this != &info )
	{
		// release old
		Reset();

		// init
		m_Name = info.m_Name;

		// copy handle
		::DuplicateHandle( ::GetCurrentProcess(),
						   info.m_Handle,
						   ::GetCurrentProcess(),
						   &m_Handle,
						   0,
						   FALSE,
						   DUPLICATE_SAME_ACCESS );
	}

	// done
	return ( *this );
}

#endif // _MT

//////////////////////////////////////////////////////////////////////////////
// class Thread::Pauser implementation

#ifdef _MT

void Thread::Pauser :: Pause( void )
{
	gpassert( m_TInfoDb.empty() );

	GetThreads( m_TInfoDb, false );

	for ( TInfoDb::iterator i = m_TInfoDb.begin() ; i != m_TInfoDb.end() ; )
	{
		HANDLE h = i->second.m_Handle;
		if ( h != NULL )
		{
			::SuspendThread( h );
			++i;
		}
		else
		{
			// chuck it, no need for it
			i = m_TInfoDb.erase( i );
		}
	}
}

void Thread::Pauser :: Resume( void )
{
	TInfoDb::const_iterator i, begin = m_TInfoDb.begin(), end = m_TInfoDb.end();
	for ( i = begin ; i != end ; ++i )
	{
		::ResumeThread( i->second.m_Handle );
	}

	m_TInfoDb.clear();
}

#endif // _MT

//////////////////////////////////////////////////////////////////////////
// Debug thread lock implementations
//

#if !GP_RETAIL

DWORD FILESYS_BLOCKED_ON_READFILE_COUNT			= 0;
LONG  FILESYS_IS_READING_FILE					= 0;
LONG  FILESYS_PRIMARY_THREAD_READS				= 0;
DWORD KERNEL_CRITICAL_MAX_WAIT_COUNT			= 0;
DWORD KERNEL_CRITICAL_BLOCKED_CRITICAL_COUNT	= 0;

// Since Win95/Win98 doesn't have a TryEnterCriticalSection function we have to
// dynamically load this function.
// This means that tracking locks will only work on NT
BOOL (__stdcall *kerneltool::GP_TryEnterCriticalSection)(LPCRITICAL_SECTION lpCrit);

BOOL __stdcall Win95_TryEnterCriticalSection(LPCRITICAL_SECTION lpCrit)
{
	::EnterCriticalSection( lpCrit );
	return TRUE;
}

BOOL __stdcall Debug_TryEnterCriticalSection(LPCRITICAL_SECTION lpCrit)
{
	if (TryEnterCriticalSection(lpCrit))
	{
		return TRUE;
	}
	else
	{
		if (FILESYS_IS_READING_FILE) //doing a dirty cross thread read here which is ok 
		{
			InterlockedIncrement((LONG*)&FILESYS_BLOCKED_ON_READFILE_COUNT);
		}
		return FALSE;
	}
}

// Class initialized at startup to load the TryEnterCriticalSection function which is needed
// before any critical sections can be used. Hence the pragma 
class InitKernelTool
{
public:
	InitKernelTool()
	{
		if (SysInfo::IsOsNt())
		{
			kerneltool::GP_TryEnterCriticalSection = Debug_TryEnterCriticalSection;
		}
		else
		{
			kerneltool::GP_TryEnterCriticalSection = Win95_TryEnterCriticalSection;;
		}
	}
};

DWORD GetRenderThreadFileOpCount( void )
{
	return FILESYS_PRIMARY_THREAD_READS;
}

DWORD GetRenderThreadFileOpBlockedCount( void )
{
	return FILESYS_BLOCKED_ON_READFILE_COUNT;
}

DWORD GetBlockedCriticalCount( void )
{
	return KERNEL_CRITICAL_BLOCKED_CRITICAL_COUNT;
}

DWORD GetLongWaitCriticalCount( void )
{
	return KERNEL_CRITICAL_MAX_WAIT_COUNT;
}

void EnterFileOp( void )
{
	if (Tracker<Critical>::ms_PrimaryThreadId == GetCurrentThreadId())
	{
		InterlockedIncrement((LONG*)&FILESYS_PRIMARY_THREAD_READS);
	}

	InterlockedIncrement(&FILESYS_IS_READING_FILE);
}

void LeaveFileOp( void )
{
	InterlockedDecrement(&FILESYS_IS_READING_FILE);
}

#endif //!GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace kerneltool

//////////////////////////////////////////////////////////////////////////////

#if !GP_RETAIL

//needed to make sure that dynamic function GP_TryEnterCriticalSection is loaded before
//anyone makes use of Critical class
#pragma warning ( disable : 4073 )
#pragma init_seg( lib )
kerneltool::InitKernelTool gIKT;

#endif //!GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
