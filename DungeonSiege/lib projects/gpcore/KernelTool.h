//////////////////////////////////////////////////////////////////////////////
//
// File     :  KernelTool.h (GPCore)
// Author(s):  Scott Bilas
//
// Summary  :  This is a misc kernel object utility collection. Feel free to
//             add to it.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __KERNEL_H
#define __KERNEL_H

//////////////////////////////////////////////////////////////////////////////

#include "GpColl.h"

#if !GP_RETAIL
// debug options - set to 1 to enable
#define       KERNEL_CRITICAL_TRACK_LOCKS		0	// maintain linked list of currently locked criticals
#define       KERNEL_CRITICAL_TIME_LOCKS		0	// make sure that wait time for locks doesn't exceed given amount
extern double KERNEL_CRITICAL_MAX_WAIT_TIME_PRI;	// number of milliseconds to wait on primary thread for a critical before breaking
extern double KERNEL_CRITICAL_MAX_WAIT_TIME_AUX;	// number of milliseconds to wait on an auxiliary thread for a critical before breaking
extern bool   KERNEL_CRITICAL_TIME_BREAKPOINT;		// set true to breakpoint on timeout
#endif

namespace kerneltool  {  // begin of namespace kerneltool

//////////////////////////////////////////////////////////////////////////////
// class Tracker declaration

// $ access the root in the watch window with g_TypeLockedRoot where Type is
//   Critical or RwCritical or whatever.

template <typename T>
class Tracker
{
public:
	SET_NO_INHERITED( Tracker );

	Tracker( void )
	{
#		if KERNEL_CRITICAL_TRACK_LOCKS
		m_TrackCount = 0;
		m_Next       = (T*)this;
		m_Prev       = (T*)this;
#		endif // KERNEL_CRITICAL_TRACK_LOCKS

#		if KERNEL_CRITICAL_TIME_LOCKS
		m_WaitTick  = 0.0;
		m_TimeCount = 0;
		::InitializeCriticalSection( &m_WaitCritical );
#		endif // KERNEL_CRITICAL_TIME_LOCKS

	}

#	if KERNEL_CRITICAL_TRACK_LOCKS || KERNEL_CRITICAL_TIME_LOCKS
	Tracker( bool )
	{
#		if KERNEL_CRITICAL_TRACK_LOCKS
		m_TrackCount       = 0;
		m_Next             = (T*)this;
		m_Prev             = (T*)this;
		ms_Root            = (T*)this;
#		endif // KERNEL_CRITICAL_TRACK_LOCKS
	}
#	endif // KERNEL_CRITICAL_TRACK_LOCKS || KERNEL_CRITICAL_TIME_LOCKS

   ~Tracker( void )
	{
#		if KERNEL_CRITICAL_TIME_LOCKS
		::DeleteCriticalSection( &m_WaitCritical );
#		endif // KERNEL_CRITICAL_TIME_LOCKS
	}

void PreEnterTrack( void ) const
{

#	if KERNEL_CRITICAL_TIME_LOCKS
	::EnterCriticalSection( &m_WaitCritical );
	if ( !m_TimeCount )
	{
		// If this was the first entry into the critical, then save the time
		m_WaitTick = SysInfo::GetCPUSeconds();
	}
	// Increment the tick count to indicate someone is waiting
	++m_TimeCount;
	::LeaveCriticalSection( &m_WaitCritical );
#	endif // KERNEL_CRITICAL_TIME_LOCKS
}

void PostEnterTrack( void ) const
{
#		if KERNEL_CRITICAL_TIME_LOCKS

	::EnterCriticalSection( &m_WaitCritical );
	if ( m_WaitTick != 0.0f )
	{
		double waitDelta = SysInfo::GetCPUSeconds() - m_WaitTick;
//		::InterlockedExchangeAdd( (LONG *) &ms_TotalBlockTime, waitDelta );
		double waitTime
				= (::GetCurrentThreadId() == ms_PrimaryThreadId)
				? KERNEL_CRITICAL_MAX_WAIT_TIME_PRI
				: KERNEL_CRITICAL_MAX_WAIT_TIME_AUX;
		if ( waitDelta > waitTime )
		{
			// increment count
			KERNEL_CRITICAL_MAX_WAIT_COUNT++;

			// Clear the tick counter so when the user continues from the
			// debugger the break doesn't continue to go off
			m_WaitTick = 0.0;
		}
	}

	// Decrement the wait count
	--m_TimeCount;
	if ( !m_TimeCount )
	{
		// If this is the last to leave, reset the tick count
		m_WaitTick = 0.0;
	}

	::LeaveCriticalSection( &m_WaitCritical );
#		endif // KERNEL_CRITICAL_TIME_LOCKS

#		if KERNEL_CRITICAL_TRACK_LOCKS
	if ( ms_Root != NULL )
	{
		::InterlockedIncrement( &ms_Root->m_TrackCount );
		if ( ++m_TrackCount == 1 )
		{
			m_Next = ms_Root->m_Next;
			m_Prev = ms_Root;
			m_Next->m_Prev = (T*)this;
			m_Prev->m_Next = (T*)this;
		}
	}
#		endif // KERNEL_CRITICAL_TRACK_LOCKS
}

void LeaveTrack( void ) const
{
#		if KERNEL_CRITICAL_TRACK_LOCKS
	if ( ms_Root != NULL )
	{
		::InterlockedDecrement( &ms_Root->m_TrackCount );
		if ( --m_TrackCount == 0 )
		{
			m_Next->m_Prev = m_Prev;
			m_Prev->m_Next = m_Next;
			m_Next = NULL;
			m_Prev = NULL;
		}
	}
#		endif // KERNEL_CRITICAL_TRACK_LOCKS

}

#	if KERNEL_CRITICAL_TIME_LOCKS
	static DWORD GetAndClearTotalBlockTime( void )
	{
		return ( ::InterlockedExchange( (LONG *) &ms_TotalBlockTime, 0 ) );
	}
#	endif // KERNEL_CRITICAL_TIME_LOCKS

#	if KERNEL_CRITICAL_TRACK_LOCKS
	mutable LONG  m_TrackCount;
	mutable T*    m_Next;
	mutable T*    m_Prev;
	static  T*    ms_Root;
#	endif // KERNEL_CRITICAL_TRACK_LOCKS

#	if KERNEL_CRITICAL_TIME_LOCKS
	mutable double           m_WaitTick;
	mutable int              m_TimeCount;
	mutable CRITICAL_SECTION m_WaitCritical;
	static  DWORD            ms_TotalBlockTime;
#	endif // KERNEL_CRITICAL_TIME_LOCKS

#if !GP_RETAIL
	static  DWORD            ms_PrimaryThreadId;
#endif

	SET_NO_COPYING( Tracker );
};

#if KERNEL_CRITICAL_TRACK_LOCKS
template <typename T> T* Tracker <T>::ms_Root = NULL;
#endif // KERNEL_CRITICAL_TRACK_LOCKS

#if KERNEL_CRITICAL_TIME_LOCKS
template <typename T> DWORD Tracker <T>::ms_TotalBlockTime = 0;
#endif // KERNEL_CRITICAL_TIME_LOCKS

#if !GP_RETAIL
template <typename T> DWORD Tracker <T>::ms_PrimaryThreadId = ::GetCurrentThreadId();
#endif 

//////////////////////////////////////////////////////////////////////////////
// class Object declaration

// purpose:
//   wrapper for win32 kernel objects.

class Object
{
public:
	SET_NO_INHERITED( Object );

	// construction
	inline        Object( void );												// clears m_Handle
	inline       ~Object( void );												// calls Close() if m_Handle not null

	// handle access
	inline HANDLE Get     ( void ) const;										// gives back the current handle (may be null)
	inline HANDLE GetValid( void ) const;										// gives back the current handle (asserts if invalid or doesn't exist)
	inline bool   IsNull  ( void ) const;										// currently unused?
	inline bool   IsValid ( void ) const;										// is handle valid (asserts if there isn't one) - calls GetHandleInformation() to validate

	// kernel operations
	inline DWORD  Wait           ( DWORD milliseconds = INFINITE ) const;		// waits on this
	inline bool   IsSignaled     ( void ) const;								// checks to see if this object is signaled
	inline void   SetHandle      ( HANDLE handle );								// asserts if there already is a handle
	inline void   ResetHandle    ( void );										// close handle if owned
	inline bool   DuplicateHandle( HANDLE& dstHandle, HANDLE srcHandle );		// calls SetHandle() with a duplicate of the given handle

private:
	HANDLE m_Handle;	// kernel handle

	SET_NO_COPYING( Object );
};

//////////////////////////////////////////////////////////////////////////////
// class Event declaration

// purpose:
//   wrapper for win32 kernel events.

class Event : public Object
{
public:
	SET_INHERITED( Event, Object );

	// construction
    inline      Event( bool manualReset = true, bool initialState = false );	// creates event, passes handle to inherited
	inline     ~Event( void );													// does nothing

	// kernel operations
	inline bool Set  ( void );													// signals the event
	inline bool Reset( void );													// resets the event (only necessary if manualReset true in ctor)
	inline bool Pulse( void );													// signals the event, lets all blocking threads through, then resets it

private:
	SET_NO_COPYING( Event );
};

//////////////////////////////////////////////////////////////////////////////
// class Mutex declaration

// purpose:
//   wrapper for win32 kernel mutexes.

class Mutex : public Object
{
public:
	SET_INHERITED( Mutex, Object );

	// construction
    inline      Mutex( bool initialOwner = false );								// creates mutex, passes handle to inherited
    inline      Mutex( LPCTSTR name, bool initialOwner = false );				// creates mutex, passes handle to inherited
	inline     ~Mutex( void );													// does nothing

	// kernel operations
	inline bool Release( void ) const;											// releases the mutex after previously acquiring it via Wait()

	struct Lock
	{
		const Mutex* m_Mutex;
		bool m_Locked;

		Lock( const Mutex& mutex, DWORD milliseconds = INFINITE )
			: m_Mutex( &mutex )
		{
			m_Locked = m_Mutex->Wait( milliseconds ) != WAIT_TIMEOUT;
		}

	   ~Lock( void )
		{
			if ( m_Locked )
			{
				m_Mutex->Release();
			}
		}
	};

private:
	SET_NO_COPYING( Mutex );
};

//////////////////////////////////////////////////////////////////////////////
// class Optex declaration

// $ this is based on Jeffrey Richter's work in MSDN (see OPTEX). this class
//   is designed to work as an inter-process critical section (optimized
//   mutex). it's about 2x or so the cost of a critical, but 6x cheaper than a
//   full mutex, which requires a kernel transition.

class Optex
{
public:
	SET_NO_INHERITED( Optex );

	Optex( void );
   ~Optex( void )		{  }

	DWORD Enter( DWORD milliseconds = INFINITE ) const;
	void  Leave( void ) const;

	struct Lock
	{
		Optex* m_Optex;
		bool m_Locked;

		Lock( Optex& optex, DWORD milliseconds = INFINITE )
			: m_Optex( &optex )
		{
			m_Locked = m_Optex->Enter( milliseconds ) != WAIT_TIMEOUT;
		}

	   ~Lock( void )
		{
			if ( m_Locked )
			{
				m_Optex->Leave();
			}
		}
	};

private:
	mutable LONG  m_LockCount;
	mutable DWORD m_ThreadId;
	mutable LONG  m_RecurseCount;
	mutable Event m_Event;

	SET_NO_COPYING( Optex );
};

//////////////////////////////////////////////////////////////////////////////
// class Critical declaration

class Critical : public Tracker <Critical>
{
public:
#	if KERNEL_CRITICAL_TRACK_LOCKS
	Critical( bool );
#	endif // KERNEL_CRITICAL_TRACK_LOCKS

	SET_INHERITED( Critical, Tracker <Critical> );

	inline      Critical( void );
	inline     ~Critical( void );

	inline void Enter( void ) const;
	inline void Leave( void ) const;

	static void __fastcall StaticEnter( Critical* crit )  {  crit->Enter();  }
	static void __fastcall StaticLeave( Critical* crit )  {  crit->Leave();  }

	class Lock				// use this to acquire a critical section
	{
	public:
		SET_NO_INHERITED( Lock );

		Lock( const Critical& critical )
			: m_Critical( critical )  {  m_Critical.Enter();  }
	   ~Lock( void )
	   		{  m_Critical.Leave();  }

	private:
		const Critical& m_Critical;

		SET_NO_COPYING( Lock );
	};

	class OptionalLock		// use this to acquire a critical section that may not exist
	{
	public:
		SET_NO_INHERITED( OptionalLock );

		OptionalLock( const Critical* critical )
			: m_Critical( critical )  {  if ( m_Critical != NULL )  {  m_Critical->Enter();  }  }
	   ~OptionalLock( void )
	   		{  if ( m_Critical != NULL )  {  m_Critical->Leave();  }  }

	private:
		const Critical* m_Critical;

		SET_NO_COPYING( OptionalLock );
	};

private:
	mutable CRITICAL_SECTION m_Critical;

	SET_NO_COPYING( Critical );
};

#if !KERNEL_CRITICAL_TRACK_LOCKS && !KERNEL_CRITICAL_TIME_LOCKS
COMPILER_ASSERT( sizeof( Critical ) == sizeof( CRITICAL_SECTION ) );
#endif // !KERNEL_CRITICAL_TRACK_LOCKS && !KERNEL_CRITICAL_TIME_LOCKS

//////////////////////////////////////////////////////////////////////////////
// class RwCritical declaration

class RwCritical : public Tracker <RwCritical>
{
public:
#	if KERNEL_CRITICAL_TRACK_LOCKS
	RwCritical( bool );
#	endif // KERNEL_CRITICAL_TRACK_LOCKS

	SET_INHERITED( RwCritical, Tracker <RwCritical> );

	inline      RwCritical( void );
	inline     ~RwCritical( void );

	inline void EnterRead ( void ) const;
	inline void LeaveRead ( void ) const;

	inline void EnterWrite( void ) const;
	inline void LeaveWrite( void ) const;

	inline bool IsReadLocked ( void ) const;
	inline bool IsWriteLocked( void ) const;

	// use this to acquire a read lock on a critical section. read locks allow
	// other read locks to occur without blocking. only write locks are blocked
	// out.
	class ReadLock
	{
	public:
		SET_NO_INHERITED( ReadLock );

		ReadLock( const RwCritical& critical )
			: m_Critical( critical )  {  m_Critical.EnterRead();  }
	   ~ReadLock( void )
			{  m_Critical.LeaveRead();  }

	private:
		const RwCritical& m_Critical;

		SET_NO_COPYING( ReadLock );
	};

	// use this to acquire a write lock on a critical section. write locks block
	// out all other threads.
	class WriteLock
	{
	public:
		SET_NO_INHERITED( WriteLock );

		WriteLock( const RwCritical& critical )
			: m_Critical( critical )  {  m_Critical.EnterWrite();  }
	   ~WriteLock( void )
			{  m_Critical.LeaveWrite();  }

	private:
		const RwCritical& m_Critical;

		SET_NO_COPYING( WriteLock );
	};

private:
	static void __fastcall SP_EnterRead ( const void* crit );
	static void __fastcall MP_EnterRead ( const void* crit );
	static void __fastcall SP_LeaveRead ( const void* crit );
	static void __fastcall MP_LeaveRead ( const void* crit );
	static void __fastcall SP_EnterWrite( const void* crit );
	static void __fastcall MP_EnterWrite( const void* crit );
	static void __fastcall SP_LeaveWrite( const void* crit );
	static void __fastcall MP_LeaveWrite( const void* crit );

	typedef void (__fastcall *Proc)( const void* );

	static Proc ms_EnterRead;
	static Proc ms_LeaveRead;
	static Proc ms_EnterWrite;
	static Proc ms_LeaveWrite;

	mutable LONG     m_ReadLockCount;
	mutable LONG     m_WriteLockCount;
	mutable Critical m_WriteLock;

	SET_NO_COPYING( RwCritical );
};

#if !KERNEL_CRITICAL_TRACK_LOCKS && !KERNEL_CRITICAL_TIME_LOCKS
COMPILER_ASSERT( sizeof( RwCritical ) == (8 + sizeof( CRITICAL_SECTION )) );
#endif // !KERNEL_CRITICAL_TRACK_LOCKS && !KERNEL_CRITICAL_TIME_LOCKS

//////////////////////////////////////////////////////////////////////////////
// class Thread declaration

#ifdef _MT

// purpose:
//   wrapper for win32 kernel threads. derive from this class and override
//   its Execute() method. when the thread is spawned it will run through
//   Execute() exactly once before it dies. asserts if try to nuke the object
//   while the thread still alive.

class Thread : public Object
{
public:
	SET_INHERITED( Thread, Object );

// Types.

	enum eOptions								// options to configure a Thread
	{
		// one of
		OPTION_IGNORE_EXCEPTIONS    = 1 << 0,		// at top level, ignore (quietly handle) exceptions, setting a return code of the exception code that occurred
		OPTION_BEGIN_THREAD_WAIT    = 1 << 1,		// wait for thread to finish starting after asking it to begin
		OPTION_DELETE_SELF			= 1 << 2,		// delete "this" when thread finishes executing (careful!)

		// OR one of
		OPTION_NONE = 0,							// disable all options
		OPTION_ALL  = 0xFFFFFFFF,					// enable all options
	};

	static const eOptions OPTION_DEF;				// defaults

	struct TInfo
	{
		HANDLE   m_Handle;
		gpstring m_Name;

		TInfo( void );
		TInfo( const TInfo& info );
	   ~TInfo( void );

		HANDLE Release( void );
		void   Reset  ( void );

		TInfo& operator = ( const TInfo& info );
	};

	typedef stdx::linear_map <DWORD /*id*/, TInfo> TInfoDb;

// Setup.

	// construction
	        inline  Thread( eOptions options = OPTION_DEF );
	virtual inline ~Thread( void );

	// configuration
	void SetOptions   ( eOptions options, bool set = true )	{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void ClearOptions ( eOptions options )					{  SetOptions( options, false );  }
	void ToggleOptions( eOptions options )					{  m_Options = (eOptions)(m_Options ^ options);  }
	bool TestOptions  ( eOptions options ) const			{  return ( (m_Options & options) != 0 );  }
	bool TestOptionsEq( eOptions options ) const			{  return ( (m_Options & options) == options );  }

// Commands.

	// $ note: thread name uses a special undocumented vc++ 6 feature, and the
	//         max name length is 9.

	// kernel operations
	       bool  BeginThread ( const char* threadName = NULL );					// spawns new thread that starts out in Execute() (name is optional)
	inline void  Nuke        ( void );											// forces nasty shutdown of thread
	inline DWORD WaitForStart( DWORD milliseconds = INFINITE ) const;			// waits for the thread to start up (returns result of wait)
	inline DWORD WaitForExit ( DWORD milliseconds = INFINITE ) const;			// waits for the thread to finish (asserts if no thread) - returns what the thread returned
	inline void  Reset       ( void );											// closes handle to ready next BeginThread() (asserts if thread still running)

	// query
	inline DWORD GetThreadId    ( void ) const;									// get thread id - asserts if no thread or thread not running
	inline DWORD GetLastReturn  ( void ) const;									// gets last return value. note that if no thread was run this will still return it (it'll be 0xFFFFFFFF).
	inline bool  IsThreadRunning( void ) const;									// is thread running? (asserts if no thread)

	// special
	void SetThreadName( const char* threadName );								// update the thread's name

// Thread registry.

	// registration
	static bool RegisterCurrentThread  ( const char* name );
	static bool RegisterThread         ( DWORD id, HANDLE h, const char* name );
	static void UnregisterCurrentThread( void );
	static void UnregisterByName       ( const char* name );
	static void UnregisterById         ( DWORD id );

	// settings
	static void SetCurrentThreadName( const char* name );
	static void SetThreadName       ( DWORD id, const char* name );

	// query
	static bool     GetThreadIds         ( TInfoDb& db, bool includeMyThread = true );
	static bool     GetThreads           ( TInfoDb& db, bool includeMyThread = true );
	static void     GetRegisteredThreads ( TInfoDb& db, bool includeMyThread = true );
	static gpstring FindCurrentThreadName( void );
	static gpstring FindThreadNameById   ( DWORD id );
	static gpstring MakeCurrentThreadName( void );
	static gpstring MakeThreadNameById   ( DWORD id );

// Utility types.

	class Pauser
	{
	public:
		void Pause ( void );
		void Resume( void );

		const TInfoDb& GetThreadInfoDb( void ) const  {  return ( m_TInfoDb );  }

	private:
		TInfoDb m_TInfoDb;
	};

	// $ pauses all threads except the current one on construction, resumes
	//   them on destruction.

	class AutoPauser : Pauser
	{
	public:
		AutoPauser( void )		{  Pause ();  }
	   ~AutoPauser( void )		{  Resume();  }

		using Pauser::GetThreadInfoDb;
	};

protected:
	// derived override
	virtual DWORD Execute( void ) = 0;

private:
	static unsigned int __stdcall StaticThreadProc( void* arg );				// static redirector
	       unsigned int __stdcall       ThreadProc( void );						// the "true" thread proc

	// state
	eOptions     m_Options;			// tuning options for thread
	unsigned int m_ID;				// the thread id
	gpstring     m_Name;			// its name
	DWORD        m_LastReturn;		// what it returned when finished
	Event        m_StartupEvent;	// set when we start up

	// statics
	static Critical ms_Critical;	// critical lock for thread db
	static TInfoDb  ms_TInfoDb;		// all registered threads

	SET_NO_COPYING( Thread );
};

MAKE_ENUM_BIT_OPERATORS( Thread::eOptions );

#endif // _MT

//////////////////////////////////////////////////////////////////////////////
// Debug thread lock declarations

// purpose:
//   functions expose information about thread locking

#if !GP_RETAIL

extern DWORD KERNEL_CRITICAL_BLOCKED_CRITICAL_COUNT;
extern DWORD KERNEL_CRITICAL_MAX_WAIT_COUNT;

DWORD GetRenderThreadFileOpCount( void );			// how many read or open files occur on main thread
DWORD GetRenderThreadFileOpBlockedCount( void );	// if secondary thread holds lock and does file op and blocks main thread
DWORD GetBlockedCriticalCount( void );				// number of times TryEnterCriticalSection failed entering
DWORD GetLongWaitCriticalCount( void );				// number of times wait on critical was longer than MAX_WAIT_TIME_PRI
   													// there might be something other than OS operations

void EnterFileOp( void );
void LeaveFileOp( void );

// helper function to get around fact that TryEnterCriticalSection not on Win95,98,ME
extern BOOL (__stdcall *GP_TryEnterCriticalSection)(LPCRITICAL_SECTION lpCrit);

#endif

//////////////////////////////////////////////////////////////////////////////
// class Object inline implementation

inline Object :: Object( void )
	{  m_Handle = NULL;  }

inline Object :: ~Object( void )
	{  ResetHandle();  }

inline HANDLE Object :: Get( void ) const
	{  return ( m_Handle );  }

inline HANDLE Object :: GetValid( void ) const
	{  gpassert( IsValid() );  return ( Get() );  }

inline bool Object :: IsNull( void ) const
	{  return ( m_Handle == NULL );  }

inline bool Object :: IsValid( void ) const
{
#	if 1
	return ( m_Handle != NULL );
#	else
	DWORD flags;
		//$$$ check to see if we're running in NT - win95 does not support
		//  gethandleinfo().
	return (   ( m_Handle != NULL )
			&& ( ::GetHandleInformation( m_Handle, &flags ) != false ) );
#	endif
}

inline DWORD Object :: Wait( DWORD milliseconds ) const
	{  return ( ::WaitForSingleObject( GetValid(), milliseconds ) );  }

inline bool Object :: IsSignaled( void ) const
	{  return (Wait(0) == WAIT_OBJECT_0);  }

inline void Object :: SetHandle( HANDLE handle )
{
	gpassert( IsNull() && ( handle != NULL ) );
	m_Handle = handle;
	gpassert( IsValid() );
}

inline void Object :: ResetHandle( void )
{
	if ( !IsNull() )
	{
		gpassert( IsValid() );
		::CloseHandle( m_Handle );
		m_Handle = NULL;
	}
}

inline bool Object :: DuplicateHandle( HANDLE& dstHandle, HANDLE srcHandle )
{
	HANDLE process = ::GetCurrentProcess();
	return ( ::DuplicateHandle(
			 process,
			 srcHandle,
			 process,
			 &dstHandle,
			 0,
			 false,
			 DUPLICATE_SAME_ACCESS ) != 0 );
}

//////////////////////////////////////////////////////////////////////////////
// class Event inline implementation

inline Event :: Event( bool manualReset, bool initialState )
	{  SetHandle( ::CreateEvent( NULL, manualReset, initialState, NULL ) );  }

inline Event :: ~Event( void )
	{  }

inline bool Event :: Set( void )
	{  return ( ::SetEvent( GetValid() ) != 0 );  }

inline bool Event :: Reset( void )
	{  return ( ::ResetEvent( GetValid() ) != 0 );  }

inline bool Event :: Pulse( void )
	{  return ( ::PulseEvent( GetValid() ) != 0 );  }

//////////////////////////////////////////////////////////////////////////////
// class Mutex inline implementation

inline Mutex :: Mutex( bool initialOwner )
	{  SetHandle( ::CreateMutex( NULL, initialOwner, NULL ) );  }

inline Mutex :: Mutex( LPCTSTR name, bool initialOwner )
	{  SetHandle( ::CreateMutex( NULL, initialOwner, name ) );  }

inline Mutex :: ~Mutex( void )
	{  }

inline bool Mutex :: Release( void ) const
	{  return ( ::ReleaseMutex( GetValid() ) != 0 );  }

//////////////////////////////////////////////////////////////////////////////
// class Critical inline implementation

inline Critical :: Critical( void )
{
	::InitializeCriticalSection( &m_Critical );
}

inline Critical :: ~Critical( void )
{
	::DeleteCriticalSection( &m_Critical );
}

inline void Critical :: Enter( void ) const
{
	PreEnterTrack();

#	if !GP_RETAIL
	if ( !GP_TryEnterCriticalSection( &m_Critical ) )
	{
		InterlockedIncrement( (LONG*)&KERNEL_CRITICAL_BLOCKED_CRITICAL_COUNT );
		::EnterCriticalSection( &m_Critical );
	}
#	else
	::EnterCriticalSection( &m_Critical );
#	endif

	PostEnterTrack();
}

inline void Critical :: Leave( void ) const
{
	LeaveTrack();
	::LeaveCriticalSection( &m_Critical );
}

//////////////////////////////////////////////////////////////////////////////
// class RwCritical inline implementation

inline RwCritical :: RwCritical( void )
{
	m_ReadLockCount  = -1;
	m_WriteLockCount = -1;

#	if GP_DEBUG
	// hack around compiler
	DWORD baseSize = sizeof( Tracker <RwCritical> );
	if ( baseSize == 1 )
	{
		baseSize = 0;
	}
	gpassert( ((DWORD)&m_ReadLockCount  - ((DWORD)this + baseSize)) == 0 );
	gpassert( ((DWORD)&m_WriteLockCount - ((DWORD)this + baseSize)) == 4 );
	gpassert( ((DWORD)&m_WriteLock      - ((DWORD)this + baseSize)) == 8 );
#	endif // GP_DEBUG
}

inline RwCritical :: ~RwCritical( void )
{
	// this space intentionally left blank...
}

inline void RwCritical :: EnterRead( void ) const
{
	PreEnterTrack();
	(*ms_EnterRead )( this );
	PostEnterTrack();
}

inline void RwCritical :: LeaveRead( void ) const
{
	LeaveTrack();
	(*ms_LeaveRead )( this );
}

inline void RwCritical :: EnterWrite( void ) const
{
	PreEnterTrack();
	(*ms_EnterWrite)( this );
	PostEnterTrack();
}

inline void RwCritical :: LeaveWrite( void ) const
{
	LeaveTrack();
	(*ms_LeaveWrite)( this );
}

inline bool RwCritical :: IsReadLocked( void ) const
{
	return ( m_ReadLockCount >= 0 );
}

inline bool RwCritical :: IsWriteLocked( void ) const
{
	return ( m_WriteLockCount >= 0 );
}

//////////////////////////////////////////////////////////////////////////////
// class Thread inline implementation

#ifdef _MT

inline Thread :: Thread( eOptions options )
	{  m_Options = options;  m_ID = 0;  m_LastReturn = 0xFFFFFFFF;  }

inline Thread :: ~Thread( void )
	{  gpassert( IsNull() || !IsThreadRunning() || TestOptions( OPTION_DELETE_SELF ) );  }

inline void Thread :: Nuke( void )
{
	gpassert( IsThreadRunning() );
	::TerminateThread( GetValid(), 'Nuke' );
}

inline DWORD Thread :: WaitForStart( DWORD milliseconds ) const
{
	return ( m_StartupEvent.Wait( milliseconds ) );
}

inline DWORD Thread :: WaitForExit( DWORD milliseconds ) const
{
	Wait( milliseconds );
	return ( GetLastReturn() );
}

inline void Thread :: Reset( void )
{
	// cannot reset until thread is done. use Nuke() to terminate it if you
	//  can't negotiate a proper shutdown.
	gpassert( !IsValid() || !IsThreadRunning() );

	// clear out the handles to get ready for another run
	ResetHandle();
	m_ID = 0;
}

inline DWORD Thread :: GetThreadId( void ) const
	{  gpassert( IsThreadRunning() );  return ( m_ID );  }

inline DWORD Thread :: GetLastReturn( void ) const
	{  gpassert( !IsThreadRunning() );  return ( m_LastReturn );  }

inline bool Thread :: IsThreadRunning( void ) const
	{  return ( !IsSignaled() );  }

#endif // _MT

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace kerneltool

//////////////////////////////////////////////////////////////////////////////

#endif  // __KERNEL_H

//////////////////////////////////////////////////////////////////////////////
