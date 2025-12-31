//////////////////////////////////////////////////////////////////////////////
//
// File     :  siege_loader.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Siege.h"
#include "Siege_Loader.h"

#include "AppModule.h"
#include "KernelTool.h"
#include "RatioStack.h"
#include "StdHelp.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////

#if !GP_RETAIL

namespace Debug
{
	kerneltool::Event s_DbgExit, s_DbgAssert( false ), s_DbgCrash( false ), s_DbgFatal( false );

FEX	void TestExit2  ( void )  {  s_DbgExit  .Set();  }
FEX	void TestAssert2( void )  {  s_DbgAssert.Set();  }
FEX	void TestCrash2 ( void )  {  s_DbgCrash .Set();  }
FEX	void TestFatal2 ( void )  {  s_DbgFatal .Set();  }
}

#endif // !GP_RETAIL
//////////////////////////////////////////////////////////////////////////////

namespace siege  {  // begin of namespace siege

//////////////////////////////////////////////////////////////////////////////
// class LoadMgrThread declaration and implementation

class LoadMgrThread : public kerneltool::Thread
{
public:
	SET_INHERITED( LoadMgrThread, kerneltool::Thread );

	enum
	{
		RETURN_SUCCESS,		// asked to exit and succeeded
		RETURN_FATAL,		// fatal occurred inside thread, aborted unexpectedly
	};

	LoadMgrThread( void )
		: Inherited( OPTION_DEF | OPTION_IGNORE_EXCEPTIONS | OPTION_BEGIN_THREAD_WAIT )
	{
		Inherited::BeginThread( "SiegeLoad" );
	}

	virtual ~LoadMgrThread( void )
	{
		if ( IsValid() && IsThreadRunning() )
		{
			m_RequestQuit.Set();
			Wait( 5000 );
		}
	}

	void Pause   ( void )			{  m_AllowExecute.Reset();  }
	void Resume  ( void )			{  m_AllowExecute.Set  ();  }
	bool IsPaused( void ) const		{  return ( !m_AllowExecute.IsSignaled() );  }

private:

	virtual DWORD Execute( void )
	{
		DWORD rc = 0;
		__try
		{
			__try
			{
				// continue infinitely until normal exit or an external exception
				while ( !ReportSys::IsFatalOccurring() )
				{
					// wait for "go"
					HANDLE syncObjects[ 2 ];
					syncObjects[ 0 ] = m_RequestQuit.GetValid();
					syncObjects[ 1 ] = m_AllowExecute.GetValid();
					DWORD waitCode = ::WaitForMultipleObjects( ELEMENT_COUNT( syncObjects ), syncObjects, FALSE, INFINITE );

					// requested to quit?
					if ( waitCode == WAIT_OBJECT_0 )
					{
						break;
					}

					// only update if we have the focus
					if ( gAppModule.IsAppActive() || gAppModule.TestOptions( AppModule::OPTION_ALWAYS_MINIMALLY_ACTIVE ) )
					{
						gSiegeLoadMgr.ProcessSomeRequests();
					}

					// special test funcs
#					if !GP_RETAIL
					if ( Debug::s_DbgExit.IsSignaled() )
					{
						rc = 1;
						break;
					}
					if ( Debug::s_DbgAssert.IsSignaled() )
					{
						::Sleep( 1000 );
						gpassertm( 0, "Second thread asserting" );
					}
					if ( Debug::s_DbgCrash.IsSignaled() )
					{
						::Sleep( 1000 );
						volatile int x = 0, y = 0;
						volatile int z = x / y;
						UNREFERENCED_PARAMETER( z );
					}
					if ( Debug::s_DbgFatal.IsSignaled() )
					{
						::Sleep( 1000 );
						gpwatson( "Second thread fataling" );
					}
#					endif // !GP_RETAIL

					// if not overflowing then we can sleep. If we are overflowing then we want to catch up!
					if( !gSiegeLoadMgr.IsOverflowing() )
					{
						// don't be too chunky
						::Sleep( 50 );
					}
				}
			}
			__except( ::GlobalExceptionFilter( GetExceptionInformation() ) )
			{
				// ignore, but fail
				rc = 'Fatl';
			}
		}
		__except ( ::ExceptionDlgFilter( GetExceptionInformation(), ::GetLastError(), "Siege Loader Thread" ), EXCEPTION_EXECUTE_HANDLER )
		{
			// just report it
			rc = 'SeEx';
		}

		// notify app to shut down if we had an exception
		if ( rc != 0 )
		{
			gAppModule.RequestFatalQuit();
		}

		// quit this thread
		return ( rc );
	}

	kerneltool::Event m_RequestQuit;		// startup state: false
	kerneltool::Event m_AllowExecute;		// startup state: false
	gpstring          m_ProfileOutput;

	SET_NO_COPYING( LoadMgrThread );
};

//////////////////////////////////////////////////////////////////////////////
// class LoadMgr implementation

LoadMgr :: LoadMgr( void )
{
	m_NextLoadOrderId	= LOADORDERID_INVALID;
	m_LoadMgrThread		= new LoadMgrThread;
	m_QueuedCommitType	= 0;
	m_bQueuedMode		= false;
	m_OverflowTime		= 2.0f;
	m_GoLoadCost		= 100;
	m_LoGoLoadCost		= 21;
	m_SnodLoadCost		= 45;
	m_SurfLoadCost		= 7;
	m_BaseLoadAmount	= 100;
	m_GrowLoadAmount	= 0.00002f;
	m_bOverflow			= false;
	m_CommitCount		= 1;
}

LoadMgr :: ~LoadMgr( void )
{
	// stop the loader
	StopLoadingThread( STOP_LOADER_AND_CANCEL );
	delete ( m_LoadMgrThread );

	// kill any remaining load orders
	CancelAll();

	// kill loader objects
	UninstallAllLoaders();

	// tests
	gpassert( m_OrderIndex.empty() );
	gpassert( m_LoadOrderDb.empty() );
}

void LoadMgr :: InstallLoader( FOURCC type, Loader* takeLoader, float timeNeeded )
{
	std::pair <LoaderDb::iterator, bool> rc
			= m_LoaderDb.insert( std::make_pair( type, LoaderEntry() ) );
	gpassert( rc.second );

	LoaderEntry& entry = rc.first->second;
	entry.m_Loader = takeLoader;
	entry.m_TimeNeeded = timeNeeded;
}

void LoadMgr :: UninstallLoader( FOURCC type )
{
	// first cancel any outstanding orders for this loader
	Cancel( type );

	// now remove the loader
	LoaderDb::iterator found = m_LoaderDb.find( type );
	if ( found != m_LoaderDb.end() )
	{
		delete ( found->second.m_Loader );
		m_LoaderDb.erase( found );
	}
}

void LoadMgr :: UninstallAllLoaders( void )
{
	// first cancel all outstanding orders
	CancelAll();

	// now remove all the loaders
	LoaderDb::iterator i = m_LoaderDb.begin(), iend = m_LoaderDb.end();
	while ( i != iend )
	{
		delete ( (*i).second.m_Loader );
		i = m_LoaderDb.erase( i );
	}
}

bool LoadMgr :: IsLoadingThreadRunning( void ) const
{
	return ( !m_LoadMgrThread->IsPaused() );
}

void LoadMgr :: StartLoadingThread( void )
{
	if ( !IsLoadingThreadRunning() && gSiegeEngine.GetMultithreaded() )
	{
		m_LoadMgrThread->Resume();
	}
}

void LoadMgr :: StopLoadingThread( eStopLoaderType stopType )
{
	// lock out processing requests while we pause 'er
	kerneltool::Critical::Lock locker( m_ProcessCritical );

	// stop the thread (this will allow it to finish whatever it's currently working on)
	m_LoadMgrThread->Pause();

	// commit/cancel anything that's left over
	if ( stopType == STOP_LOADER_AND_COMMIT )
	{
		CommitAll();
	}
	else if ( stopType == STOP_LOADER_AND_CANCEL )
	{
		CancelAll();
	}
}

void LoadMgr :: SetQueuedMode( bool set )
{
	m_bQueuedMode = set;
	if ( !set )
	{
		m_QueuedCommitType = 0;
	}
}


// query
bool LoadMgr :: AreOrdersPending()
{
	kerneltool::Critical::Lock locker( m_DbCritical );
	return( !m_LoadOrderDb.empty() );
}

LoadOrderId LoadMgr :: Load( FOURCC type, const gpstring& name, DWORD id, bool bDuplicate, void* extra )
{
	LoadOrderId orderId = LOADORDERID_INVALID;

	// try insert index entry
	std::pair <OrderIndex::iterator, bool> rc;
	{
		kerneltool::Critical::Lock locker( m_DbCritical );
		IndexEntry newIndexEntry( name, id );
		if( bDuplicate || (m_OrderIndex.find( newIndexEntry ) == m_OrderIndex.end()) )
		{
			rc.first	= m_OrderIndex.insert( newIndexEntry );
			rc.second	= true;
		}
		else
		{
			rc.second	= false;
		}
	}

	// add order
	if ( rc.second )
	{
		bool eraseIndex = true;

		// find loader
		LoaderDb::iterator found = m_LoaderDb.find( type );
		if_gpassert( found != m_LoaderDb.end() )
		{
			// build load order
			LoadOrder order;
			order.m_Data = found->second.m_Loader->OnCreateLoadOrder( type, name, id, extra );
			order.m_Id	 = id;
			if ( order.m_Data != NULL )
			{
				kerneltool::Critical::Lock locker( m_DbCritical );

				orderId = GetNextLoadOrderId();
				eraseIndex = false;

				// fill load order
				order.m_ReqType     = type;
				order.m_OrderIter   = rc.first;
				order.m_TimeNeeded  = found->second.m_TimeNeeded;
				order.m_StartTime	= ::GetSystemSeconds();
				order.m_LoadOrderId = orderId;
				order.m_CommitCount	= 0;

				// add it
				m_LoadOrderDb.insert( order );
			}
		}

		// if screwup, kill the index entry
		if ( eraseIndex )
		{
			kerneltool::Critical::Lock locker( m_DbCritical );
			m_OrderIndex.erase( rc.first );
		}
	}
	else
	{
		// already there!
		orderId = LOADORDERID_PENDING;
	}

	// commit right away if no thread running
	if ( !IsLoadingThreadRunning() && (!IsQueuedMode() || (type == m_QueuedCommitType)) )
	{
		orderId = Commit( orderId ) ? LOADORDERID_DONE : LOADORDERID_INVALID;
	}

	// done
	return ( orderId );
}

LoadOrderId LoadMgr :: LoadByName( FOURCC type, const gpstring& name, bool bDuplicate, void* extra )
{
	return ( Load( type, name, 0, bDuplicate, extra ) );
}

LoadOrderId LoadMgr :: LoadById( FOURCC type, DWORD id, bool bDuplicate, void* extra )
{
	return ( Load( type, gpstring::EMPTY, id, bDuplicate, extra ) );
}

void LoadMgr :: Cancel( FOURCC type )
{
	// Enter DB critical
	m_DbCritical.Enter();

	LoadOrderDb::iterator i = m_LoadOrderDb.begin(), iend = m_LoadOrderDb.end();
	while ( i != iend )
	{
		if ( (*i).m_ReqType == type )
		{
			if( (*i).m_CommitCount != 0 )
			{
				// Save the committing count
				DWORD commitCount	= (*i).m_CommitCount;

				// Leave DB critical
				m_DbCritical.Leave();

				// Wait for order to be processed.  Due to re-entry and scheduling problems
				// this really does have to be a spin loop.  Any criticals used in this
				// situation will cause deadlocks.
				while( commitCount == m_CommitCount )
				{
					Sleep( 1 );
				}

				// Re-enter DB critical so loop can continue
				m_DbCritical.Enter();
			}
			else
			{
				KillOrder( *i );
				i = m_LoadOrderDb.erase( i );
			}
		}
		else
		{
			++i;
		}
	}

	// Leave DB critical
	m_DbCritical.Leave();
}

bool LoadMgr :: Cancel( LoadOrderId orderId )
{
	// Enter DB critical
	m_DbCritical.Enter();

	LoadOrderDb::iterator i, ibegin = m_LoadOrderDb.begin(), iend = m_LoadOrderDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( (*i).m_LoadOrderId == orderId )
		{
			if( (*i).m_CommitCount != 0 )
			{
				// Save the committing count
				DWORD commitCount	= (*i).m_CommitCount;

				// Leave DB critical
				m_DbCritical.Leave();

				// Wait for order to be processed.  Due to re-entry and scheduling problems
				// this really does have to be a spin loop.  Any criticals used in this
				// situation will cause deadlocks.
				while( commitCount == m_CommitCount )
				{
					Sleep( 1 );
				}
				return false;
			}
			else
			{
				KillOrder( *i );
				m_LoadOrderDb.erase( i );

				// Leave DB critical
				m_DbCritical.Leave();

				return true;
			}
		}
	}

	// Leave DB critical
	m_DbCritical.Leave();

	return false;
}

bool LoadMgr :: CancelById( FOURCC type, DWORD id )
{
	// Enter DB critical
	m_DbCritical.Enter();

	LoadOrderDb::iterator i, ibegin = m_LoadOrderDb.begin(), iend = m_LoadOrderDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( ((*i).m_ReqType == type) && ((*i).m_Id == id) )
		{
			if( (*i).m_CommitCount != 0 )
			{
				// Save the committing count
				DWORD commitCount	= (*i).m_CommitCount;

				// Leave DB critical
				m_DbCritical.Leave();

				// Wait for order to be processed.  Due to re-entry and scheduling problems
				// this really does have to be a spin loop.  Any criticals used in this
				// situation will cause deadlocks.
				while( commitCount == m_CommitCount )
				{
					Sleep( 1 );
				}
				return false;
			}
			else
			{
				KillOrder( *i );
				m_LoadOrderDb.erase( i );

				// Leave DB critical
				m_DbCritical.Leave();

				return true;
			}
		}
	}

	// Leave DB critical
	m_DbCritical.Leave();

	return false;
}

void LoadMgr :: CancelAll( void )
{
	kerneltool::Critical::Lock locker( m_ProcessCritical );

	LoadOrderDb::iterator i = m_LoadOrderDb.begin(), iend = m_LoadOrderDb.end();
	for( ; i != iend; ++i )
	{
		KillOrder( *i );
	}
	m_LoadOrderDb.clear();
}

bool LoadMgr :: Commit( LoadOrderId orderId )
{
	kerneltool::Critical::Lock locker( m_ProcessCritical );

	bool success = true;

	LoadOrderDb::iterator i, ibegin = m_LoadOrderDb.begin(), iend = m_LoadOrderDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( (*i).m_LoadOrderId == orderId )
		{
			if ( !CommitOrder( *i ) )
			{
				success = false;
			}
			m_LoadOrderDb.erase( i );
			break;
		}
	}

	return ( success );
}

bool LoadMgr :: Commit( FOURCC type, bool showProgress )
{
	kerneltool::Critical::Lock locker( m_ProcessCritical );

	bool success = true;

	std::auto_ptr <RatioSample> ratioSample;
	if ( showProgress )
	{
		// do a quick count
		int count = 0;
		LoadOrderDb::iterator i = m_LoadOrderDb.begin(), iend = m_LoadOrderDb.end();
		for ( ; i != iend ; ++i )
		{
			if ( (*i).m_ReqType == type )
			{
				++count;
			}
		}

		// build sample point
		stdx::make_auto_ptr( ratioSample, new RatioSample( "load_content", count ) );
	}

	LoadOrderDb::iterator i = m_LoadOrderDb.begin(), iend = m_LoadOrderDb.end();
	while ( i != iend )
	{
		if ( (*i).m_ReqType == type )
		{
			if ( ratioSample.get() )
			{
#				if GP_RETAIL
				ratioSample->Advance();
#				else // GP_RETAIL
				Loader& loader = *m_LoaderDb[ (*i).m_ReqType ].m_Loader;
				gpstring str;
				str.assignf( "'%s' %s",
						stringtool::MakeFourCcString( (*i).m_ReqType ).c_str(),
						loader.OnStringizeLoadOrder( (*i).m_Data ).c_str() );
				ratioSample->Advance( str );
#				endif // GP_RETAIL
			}

			if ( !CommitOrder( *i ) )
			{
				success = false;
			}
			i = m_LoadOrderDb.erase( i );
		}
		else
		{
			++i;
		}
	}

	return ( success );
}

bool LoadMgr :: CommitAll( bool showProgress )
{
	kerneltool::Critical::Lock locker( m_ProcessCritical );

	std::auto_ptr <RatioSample> ratioSample;
	if ( showProgress )
	{
		stdx::make_auto_ptr( ratioSample, new RatioSample( "load_content", m_LoadOrderDb.size() ) );
	}

	bool success = true;

	LoadOrderDb::iterator i = m_LoadOrderDb.begin(), iend = m_LoadOrderDb.end();
	while ( i != iend )
	{
		if ( ratioSample.get() )
		{
#			if GP_RETAIL
			ratioSample->Advance();
#			else // GP_RETAIL
			Loader& loader = *m_LoaderDb[ (*i).m_ReqType ].m_Loader;
			gpstring str;
			str.assignf( "'%s' %s",
					stringtool::MakeFourCcString( (*i).m_ReqType ).c_str(),
					loader.OnStringizeLoadOrder( (*i).m_Data ).c_str() );
			ratioSample->Advance( str );
#			endif // GP_RETAIL
		}

		if ( !CommitOrder( *i ) )
		{
			success = false;
		}
		i = m_LoadOrderDb.erase( i );
	}

	return ( success );
}

#if !GP_RETAIL

int LoadMgr :: GetTypeStats( TypeStatDb& coll )
{
	int count = 0;

	kerneltool::Critical::Lock locker( m_DbCritical );

	LoadOrderDb::iterator i, ibegin = m_LoadOrderDb.begin(), iend = m_LoadOrderDb.end();
	for ( i = ibegin ; i != iend ; ++i, ++count )
	{
		TypeStats& stats = coll[ (*i).m_ReqType ];
		++stats.m_PendingOrderCount;
	}

	return ( count );
}

#endif

eCommitCode LoadMgr :: CommitOrder( LoadOrder& order, bool killToo )
{
	Loader& loader = *m_LoaderDb[ order.m_ReqType ].m_Loader;
	eCommitCode rc = COMMIT_FAILURE;

	__try
	{
		// Calculate the amount of time spent between submitting this order
		// and the time it was committed.  Do not include the time spent actually
		// committing the order itself.
		if( PreciseSubtract(::GetSystemSeconds(), order.m_StartTime) - order.m_TimeNeeded > m_OverflowTime )
		{
			m_bOverflow	= true;
		}
		else
		{
			m_bOverflow = false;
		}

		rc = loader.OnCommitLoadOrder( order.m_Data );

		// $$$ gather stats
		// $$$ out status
		// $$$ handle failure

		if ( killToo )
		{
			gpassert( rc != COMMIT_RETRY );
			KillOrder( order );
		}
	}
	__except( ::GlobalExceptionFilter( GetExceptionInformation(), EXCEPTION_CONTINUE_SEARCH ) )
	{
		// this space intentionally left blank...
	}

	return ( rc );
}

void LoadMgr :: KillOrder( LoadOrder& order )
{
	// kill load order
	Loader& loader = *m_LoaderDb[ order.m_ReqType ].m_Loader;
	loader.OnDestroyLoadOrder( order.m_Data );

	// kill index entry
	m_OrderIndex.erase( order.m_OrderIter );
}

LoadOrderId LoadMgr :: GetNextLoadOrderId( void )
{
	m_NextLoadOrderId = (LoadOrderId)((DWORD)m_NextLoadOrderId + 1);
	return ( IsOrderInstance( m_NextLoadOrderId ) ? m_NextLoadOrderId : GetNextLoadOrderId() );
}

void LoadMgr :: ProcessSomeRequests( void )
{
	// limit how much can be loaded based on how much there is to load.
	int cost = 0;
	int size = 0;

	while ( m_bOverflow || ( cost < ( m_BaseLoadAmount + m_GrowLoadAmount*size*size*size ) ) )
	{
		kerneltool::Critical::Lock processLocker( m_ProcessCritical );
		LoadOrderDb::iterator loadOrderIter;

		// $ early bailout special: once we have the process critical need to
		//   check again for pausing, which may have occurred just now.
		if ( !IsLoadingThreadRunning() )
		{
			break;
		}

		bool killIt = true;
		{
			m_DbCritical.Enter();

			// $ early bailout if nothing there!
			if ( m_LoadOrderDb.empty() )
			{
				// leave the db critical
				m_DbCritical.Leave();
				break;
			}

			// grab iterator
			loadOrderIter = m_LoadOrderDb.begin();
			size = m_LoadOrderDb.size();
			(*loadOrderIter).m_CommitCount	= m_CommitCount;

			// leave the db critical
			m_DbCritical.Leave();



			// Update the cost of the objects we have loaded this frame.
			DWORD type = (*loadOrderIter).m_ReqType;
			cost += ( ( type == 'Go..' ) || ( type == 'GoPr' ) ) ? m_GoLoadCost : ( ( type == 'GoLo' ) ? m_LoGoLoadCost : ( ( type == 'Snod' ) ? m_SnodLoadCost : m_SurfLoadCost ) );

			// commit the order
			killIt = CommitOrder( (*loadOrderIter), false ) != COMMIT_RETRY;

			// increment out commit count
			m_CommitCount++;

			// enter the db critical
			m_DbCritical.Enter();

			if ( killIt )
			{
				// erase the order
				KillOrder( (*loadOrderIter) );
				m_LoadOrderDb.erase( loadOrderIter );
			}

			// leave the db critical
			m_DbCritical.Leave();
		}
	}

	// Clear overflow status since we are definitely not overflowing if we get here
	m_bOverflow = false;
}

//////////////////////////////////////////////////////////////////////////////
// struct LoadMgr::IndexEntry implementation

bool LoadMgr::IndexEntry::Pred :: operator () ( const IndexEntry& l, const IndexEntry& r ) const
{
	int rc = l.m_ReqName.compare_with_case( r.m_ReqName );
	if ( rc < 0 )
	{
		return ( true );
	}
	else if ( rc > 0 )
	{
		return ( false );
	}
	else
	{
		return ( l.m_ReqId < r.m_ReqId );
	}
}

//////////////////////////////////////////////////////////////////////////////
// struct LoadMgr::LoadOrder implementation

bool LoadMgr::LoadOrder::Pred :: operator () ( const LoadOrder& l, const LoadOrder& r ) const
{
	return( PreciseAdd(l.m_StartTime, l.m_TimeNeeded) < PreciseAdd(r.m_StartTime, r.m_TimeNeeded) );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace siege

//////////////////////////////////////////////////////////////////////////////
