//////////////////////////////////////////////////////////////////////////////
//
// File     :  siege_loader.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a resource (pre)loading and management system.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __SIEGE_LOADER_H
#define __SIEGE_LOADER_H

//////////////////////////////////////////////////////////////////////////////

#include "GpCore.h"
#include "KernelTool.h"

#include <map>

namespace siege  {  // begin of namespace siege

//////////////////////////////////////////////////////////////////////////////
// type declarations

	class LoadMgrThread;

// Handle definition.

	struct LoadOrderId_;
	typedef const LoadOrderId_* LoadOrderId;

	COMPILER_ASSERT( sizeof( LoadOrderId ) == sizeof( DWORD ) );

	const LoadOrderId LOADORDERID_INVALID = (LoadOrderId)-1;		// invalid request/failure
	const LoadOrderId LOADORDERID_PENDING = (LoadOrderId)-2;		// request was already pending
	const LoadOrderId LOADORDERID_DONE    = (LoadOrderId)-3;		// request was finished immediately

	inline bool IsOrderInstance( LoadOrderId id )  {  return ( id < LOADORDERID_DONE );  }

	enum eCommitCode
	{
		COMMIT_FAILURE,		// failed, kill it
		COMMIT_SUCCESS,		// succeeded, kill it
		COMMIT_RETRY,		// spin for a bit, and retry
	};

// Structure.

	struct LoadOrderId_
	{
	private:

	// cannot actually instantiate one of these... it's for use as a handle only

		LoadOrderId_( void );
		LoadOrderId_( const LoadOrderId_& );
	   ~LoadOrderId_( void );
	};

// Standard FOURCC's.

	const FOURCC LOADER_SIEGE_NODE_TYPE = 'Snod';
	const FOURCC LOADER_TEXTURE_TYPE	= 'Surf';

//////////////////////////////////////////////////////////////////////////////
// class Loader declaration

class Loader
{
public:
	SET_NO_INHERITED( Loader );

	Loader( void )  {  }
	virtual ~Loader( void ) = 0  {  }

	virtual void*       OnCreateLoadOrder ( FOURCC type, const gpstring& name, DWORD id, void* extra = NULL ) = 0;
	virtual void        OnDestroyLoadOrder( void* data ) = 0;
	virtual eCommitCode OnCommitLoadOrder ( void* data ) = 0;

#	if !GP_RETAIL
	virtual gpstring OnStringizeLoadOrder( const void* data ) = 0;
#	endif // !GP_RETAIL

private:
	SET_NO_COPYING( Loader );
};

//////////////////////////////////////////////////////////////////////////////
// class LoadMgr declaration

class LoadMgr : public Singleton <LoadMgr>
{
public:
	SET_NO_INHERITED( LoadMgr );

// Setup.

	// ctor/dtor
	LoadMgr( void );
   ~LoadMgr( void );

	// registration
	void InstallLoader      ( FOURCC type, Loader* takeLoader, float timeNeeded = 1.0f );	// this takes ownership of the loader - higher priorities are lower numbers
	void UninstallLoader    ( FOURCC type );
	void UninstallAllLoaders( void );

	// thread
	bool IsLoadingThreadRunning( void ) const;
	void StartLoadingThread    ( void );
	void StopLoadingThread     ( eStopLoaderType stopType = STOP_LOADER_AND_COMMIT );
	bool IsQueuedMode          ( void ) const					{ return m_bQueuedMode; }
	void SetQueuedMode         ( bool set = true );
	void SetQueuedCommitType   ( FOURCC type )					{ m_QueuedCommitType = type; }

	// timing
	void SetOverflowTime( float overflowTime )					{ m_OverflowTime = overflowTime; }
	bool IsOverflowing()										{ return m_bOverflow; }

	// loading costs
	void SetGoLoadCost( DWORD GoLoadCost )						{ m_GoLoadCost = GoLoadCost; }
	void SetLODFIGoLoadCost( DWORD LoGoLoadCost )				{ m_LoGoLoadCost = LoGoLoadCost; }
	void SetSiegeNodeLoadCost( DWORD SnodLoadCost )				{ m_SnodLoadCost = SnodLoadCost; }
	void SetTextureLoadCost( DWORD SurfLoadCost )				{ m_SurfLoadCost = SurfLoadCost; }
	void SetBaseLoadAmount( DWORD BaseLoadAmount )				{ m_BaseLoadAmount = BaseLoadAmount; }
	void SetGrowLoadAmount( float GrowLoadAmount )				{ m_GrowLoadAmount = GrowLoadAmount; }
	
	// query
	bool AreOrdersPending();

// Commands.

	// load order creation
	LoadOrderId Load      ( FOURCC type, const gpstring& name, DWORD id, bool bDuplicate = false, void* extra = NULL );
	LoadOrderId LoadByName( FOURCC type, const gpstring& name, bool bDuplicate = false, void* extra = NULL );
	LoadOrderId LoadById  ( FOURCC type, DWORD id, bool bDuplicate = false, void* extra = NULL );

	// load order cancellation
	void Cancel    ( FOURCC type );
	bool Cancel    ( LoadOrderId orderId );
	bool CancelById( FOURCC type, DWORD id );
	void CancelAll ( void );

	// forced commit
	bool Commit   ( LoadOrderId orderId );
	bool Commit   ( FOURCC type, bool showProgress = false );
	bool CommitAll( bool showProgress = false );

	// critical access
	kerneltool::Critical& GetProcessCritical( void )			{  return ( m_ProcessCritical );  }

// Debug stats.

#	if !GP_RETAIL

	struct TypeStats
	{
		int m_PendingOrderCount;

		TypeStats( void )
		{
			m_PendingOrderCount = 0;
		}
	};

	typedef std::map <FOURCC, TypeStats> TypeStatDb;

	// gather simple info on what's in the works
	int GetTypeStats( TypeStatDb& coll );

#	endif // !GP_RETAIL

private:

	struct LoadOrder;
	friend LoadMgrThread;

	// utility
	eCommitCode CommitOrder        ( LoadOrder& order, bool killToo = true );
	void        KillOrder          ( LoadOrder& order );
	LoadOrderId GetNextLoadOrderId ( void );
	void        ProcessSomeRequests( void );

	struct LoaderEntry
	{
		my Loader* m_Loader;
		float m_TimeNeeded;

		LoaderEntry( void )
			{  m_Loader = NULL;  m_TimeNeeded = 1.0f;  }
	};

	struct IndexEntry
	{
		gpstring m_ReqName;
		DWORD    m_ReqId;

		IndexEntry( const gpstring& reqName = gpstring::EMPTY, DWORD reqId = 0 )
			: m_ReqName( reqName ), m_ReqId( reqId )
		{
			m_ReqName.to_lower();
		}

		struct Pred
		{
			bool operator () ( const IndexEntry& l, const IndexEntry& r ) const;
		};
	};

	typedef std::multiset <IndexEntry, IndexEntry::Pred> OrderIndex;

	struct LoadOrder
	{
		typedef OrderIndex::iterator OrderIter;

		// spec
		FOURCC      m_ReqType;
		OrderIter   m_OrderIter;
		float       m_TimeNeeded;
		double		m_StartTime;

		// state
		LoadOrderId m_LoadOrderId;
		void*       m_Data;
		DWORD		m_Id;
		DWORD		m_CommitCount;

		struct Pred
		{
			bool operator () ( const LoadOrder& l, const LoadOrder& r ) const;
		};
	};

	typedef std::map <FOURCC, LoaderEntry> LoaderDb;
	typedef std::multiset <LoadOrder, LoadOrder::Pred> LoadOrderDb;

	// spec
	LoaderDb				m_LoaderDb;

	// state
	LoadOrderId				m_NextLoadOrderId;
	OrderIndex				m_OrderIndex;
	LoadOrderDb				m_LoadOrderDb;
	my LoadMgrThread*		m_LoadMgrThread;
	kerneltool::Critical	m_DbCritical;
	kerneltool::Critical	m_ProcessCritical;
	DWORD					m_CommitCount;

	// queuing
	bool					m_bQueuedMode;
	FOURCC					m_QueuedCommitType;

	// timing
	float					m_OverflowTime;
	bool					m_bOverflow;

	// loading costs
	DWORD					m_GoLoadCost;
	DWORD					m_LoGoLoadCost;
	DWORD					m_SnodLoadCost;
	DWORD					m_SurfLoadCost;
	DWORD					m_BaseLoadAmount;
	float					m_GrowLoadAmount;


	SET_NO_COPYING( LoadMgr );
};

#define gSiegeLoadMgr siege::LoadMgr::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace siege

#endif  // __SIEGE_LoadMgr_H

//////////////////////////////////////////////////////////////////////////////
