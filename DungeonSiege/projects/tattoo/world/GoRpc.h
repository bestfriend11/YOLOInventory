//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoRpc.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains RPC helper methods for the Go system.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GORPC_H
#define __GORPC_H

//////////////////////////////////////////////////////////////////////////////

#include "FuBiNetTransport.h"
#include "GoDefs.h"

//////////////////////////////////////////////////////////////////////////////
// class GoRpcMgr declaration

class GoRpcMgr : public FuBi::NetSendTransport, public FuBi::IRebroadcaster, public Singleton <GoRpcMgr>
{
public:
	using Singleton <GoRpcMgr>::DoesSingletonExist;

	SET_INHERITED( GoRpcMgr, FuBi::NetSendTransport );

	GoRpcMgr( void );
   ~GoRpcMgr( void );

	void CommitPending( void );

	typedef stdx::fast_vector <int> OffsetColl;

	struct Packet
	{
		bool                m_IsRetrying;		// is this a retry rpc?
		Goid                m_RouterGoid;		// goid this all belongs to
		GoidColl            m_PendingGoids;		// goids that were pending at time of send
		Buffer              m_Buffer;			// data for this rpc
		FuBi::RpcVerifySpec m_VerifySpec;		// rpc verify data at the time of sending

		Packet( void )
		{
			m_IsRetrying = false;
			m_RouterGoid = GOID_INVALID;
		}
	};

	typedef stdx::fast_vector <Packet> PacketColl;

#	if !GP_RETAIL
	static bool CheckSafeGoRpcParam( Goid goid, const FuBi::RpcVerifySpec* spec = NULL, bool isRpcPacked = false );
#	endif // !GP_RETAIL

private:

	// net transport callbacks
	virtual FuBi::Cookie BeginRPC     ( DWORD toAddress = RPC_TO_ALL, bool shouldRetry = false );
	virtual FuBi::Cookie BeginMultiRPC( DWORD* toAddress, int addresses, bool shouldRetry = false );
	virtual DWORD        EndRPC       ( void );

	// rebroadcasting callbacks
	virtual bool                 OnVerifySendRpc      ( const FuBi::RpcVerifyColl& specs, const FuBi::AddressColl& addresses, bool isRpcPacked );
	virtual bool                 OnVerifyDispatchRpc  ( bool preCall, const FuBi::RpcVerifySpec& spec );
	virtual FuBi::eBroadcastType OnProcessBroadcastRpc( const FuBi::RpcVerifySpec& spec, FuBi::AddressColl& addresses );
	virtual void                 OnBeginRedirect      ( const FuBi::RpcVerifyColl& specs );
	virtual void                 OnEndRedirect        ( void );

	// stats reporting callbacks
	virtual void OnSendRpc    ( const FuBi::FunctionSpec* spec, DWORD thisPtr, DWORD packetSize );
	virtual void OnDispatchRpc( const FuBi::FunctionSpec* spec, DWORD thisPtr, DWORD packetSize );

	Packet              m_NextPacket;		// deferred packet
	PacketColl          m_PendingColl;		// all pending packets

	SET_NO_COPYING( GoRpcMgr );
};

#define gGoRpcMgr Singleton <GoRpcMgr>::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

#endif  // __GORPC_H

//////////////////////////////////////////////////////////////////////////////
