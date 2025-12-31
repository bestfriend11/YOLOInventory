//////////////////////////////////////////////////////////////////////////////
//
// File     :  KernelToolAsm.cpp
// Author(s):  Scott Bilas
//
// Summary  :  This is a special inline file meant to be #included multiple
//             times by KernelTool.cpp ONLY. Use to define multi- and single-
//             processor versions of functions easily.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#if !defined( KERNELTOOLASM_SP ) && !defined( KERNELTOOLASM_MP )
#error ! Do NOT include this file! It's meant for KernelTool.cpp only!
#endif

#ifdef KERNELTOOLASM_SP

#define XFUNC( FUNC ) SP_##FUNC
#define XLOCK

#else

#define XFUNC( FUNC ) MP_##FUNC
#define XLOCK lock

#endif

// offset for tracker base
#if KERNEL_CRITICAL_TRACK_LOCKS || KERNEL_CRITICAL_TIME_LOCKS
#define ECX_ADJUST __asm add ecx, s_RwCriticalOffset
#else
#define ECX_ADJUST
#endif

// standard stack frame for non-retail
#if GP_RETAIL
#define PROLOGUE ECX_ADJUST
#define EPILOGUE
#else // GP_RETAIL
#define PROLOGUE __asm push ebp __asm mov ebp, esp ECX_ADJUST
#define EPILOGUE pop ebp
#endif // GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

DECL_NAKED void __fastcall RwCritical :: XFUNC( EnterRead )( const void* )
{
	PROLOGUE;

	// algo:
	//
	// 1. grab write lock first to make sure that nobody is writing.
	//
	// 2. if nobody was writing, grab a read lock then release the write lock.
	//
	// 3. otherwise check to see if there are any read locks outstanding.
	//
	//    a. if there were, just continue as normal (the existing write lock has
	//       to wait just a little longer)
	//
	//    b. if there aren't, grab a write lock then read lock then release
	//       write lock

	__asm
	{
		// grab write lock first
		XLOCK inc	dword ptr [ecx+4]				// atomic ++m_WriteLockCount
		jnz			slow_enter						// take the slow route if write lock already taken

	fast_enter:

		// grab read then release write
		XLOCK inc	dword ptr [ecx]					// atomic ++m_ReadLockCount
		XLOCK dec	dword ptr [ecx+4]				// atomic --m_WriteLockCount
		EPILOGUE									// clean up stack frame
		ret											// cool, all done

	slow_enter:

		// are there existing read locks?
		mov			eax, dword ptr [ecx]			// get the read count
		inc			eax								// -1 is 0 for easier test
		jnz			fast_enter						// there were existing read locks outstanding

		// grab critical
		push		ecx								// save 'this'
		add			ecx, 8							// ecx <- &m_WriteLock
		call		dword ptr s_EnterFunc			// grab write critical

		// grab read then release critical and write
		mov			ecx, dword ptr [esp]			// ecx <- this
		XLOCK inc	dword ptr [ecx]					// atomic ++m_ReadLockCount
		add			ecx, 8							// ecx <- &m_WriteLock
		call		dword ptr s_LeaveFunc			// release write critical
		mov			ecx, dword ptr [esp]			// ecx <- this
		XLOCK dec	dword ptr [ecx+4]				// atomic --m_WriteLockCount
		pop			ecx								// drop stack
		EPILOGUE									// clean up stack frame
		ret											// return
	}
}

DECL_NAKED void __fastcall RwCritical :: XFUNC( LeaveRead )( const void* )
{
	PROLOGUE;

	__asm
	{
		// just release
		XLOCK dec	dword ptr [ecx]					// release the read lock we had
		EPILOGUE									// clean up stack frame
		ret
	}
}

DECL_NAKED void __fastcall RwCritical :: XFUNC( EnterWrite )( const void* )
{
	PROLOGUE;

	// algo:
	//
	// 1. grab write lock first to tell people we're about to write
	//
	// 2. grab the write critical to lock out further requests
	//
	// 3. loop until any reads that were in progress are done

	static const DWORD s_EnterFunc = (DWORD)&Critical::StaticEnter;

	__asm
	{
		// grab locks
		XLOCK inc	dword ptr [ecx+4]				// atomic ++m_WriteLockCount
		push		ecx								// save 'this'
		add			ecx, 8							// ecx <- &m_WriteLock
		call		dword ptr s_EnterFunc			// grab write critical

		// spin until remaining read locks are cleared out
		// $$ future: track the time spent in this sleep if KERNEL_CRITICAL_XXX set
	spin:
		mov			ecx, dword ptr [esp]			// ecx <- &m_ReadLockCount
		mov			eax, dword ptr [ecx]			// eax <- m_ReadLockCount
		inc			eax								// add to 0
		jz			done							// if 0, nobody is using a lock so done
		push		0								// 0 msec
		call		dword ptr Sleep					// Sleep( 0 )
		jmp			spin							// continue loop

	done:

		pop			ecx								// drop stack
		EPILOGUE									// clean up stack frame
		ret											// return
	}
}

DECL_NAKED void __fastcall RwCritical :: XFUNC( LeaveWrite )( const void* )
{
	PROLOGUE;

	__asm
	{
		// just release
		XLOCK dec	dword ptr [ecx+4]				// release the write lock we had
		add			ecx, 8							// ecx <- m_WriteLock
		call		dword ptr s_LeaveFunc			// release write critical
		EPILOGUE									// clean up stack frame
		ret
	}
}

//////////////////////////////////////////////////////////////////////////////

#undef XFUNC
#undef XLOCK

//////////////////////////////////////////////////////////////////////////////
// main

// REGRESSION TEST

#if 0

kerneltool::RwCritical s_Critical;
int s_Beat = 0;

struct LockerThread : kerneltool::Thread
{
	bool m_ReadLock;
	int  m_Iters;

	LockerThread( const char* name, bool readLock )
	{
		m_ReadLock = readLock;
		m_Iters = 0;
		BeginThread( name );
		WaitForStart();
	}

	virtual DWORD Execute( void )
	{
		if ( m_ReadLock )
		{
			for ( ; ; )
			{
				Sleep( Random( 5, 15 ) );
				kerneltool::RwCritical::ReadLock locker0( s_Critical );
				Sleep( 0 );
				kerneltool::RwCritical::ReadLock locker1( s_Critical );
				++s_Beat;
				++m_Iters;
			}
		}
		else
		{
			for ( ; ; )
			{
				Sleep( Random( 0, 5 ) );
				kerneltool::RwCritical::WriteLock locker0( s_Critical );
				Sleep( 0 );
				kerneltool::RwCritical::WriteLock locker1( s_Critical );
				++s_Beat;
				++m_Iters;
			}
		}
	}
};

int main( int /*argc*/, const char* /*argv*/[] )
{
	LockerThread test0( "test0:read", true );
	LockerThread test1( "test1:read", true );
	LockerThread test2( "test2:read", true );
	LockerThread test3( "test3:read", true );
	LockerThread test4( "test4:read", true );
	LockerThread test5( "test5:read", true );
	LockerThread test6( "test6:write", false );
	LockerThread test7( "test7:write", false );
	LockerThread test8( "test8:write", false );
	LockerThread test9( "test9:write", false );
	for ( ; ; )
	{
		int oldValue = s_Beat;
		Sleep( 100 );
		if ( oldValue == s_Beat )
		{
			// deadlock detected!
			::DebugBreak();
		}
	}

	return ( 0 );
}

#endif // 0

//////////////////////////////////////////////////////////////////////////////
