//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiNetTransport.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a network transport interface for FuBi RPC's.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FUBINETTRANSPORT_H
#define __FUBINETTRANSPORT_H

//////////////////////////////////////////////////////////////////////////////

#include "FuBiDefs.h"
#include <vector>

//////////////////////////////////////////////////////////////////////////////

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// notes

// $ for destination addr constants used in BeginRPC(), see FuBiDefs.h

//////////////////////////////////////////////////////////////////////////////
// class NetSendTransport declaration

class NetSendTransport : public ManualSingleton <NetSendTransport>
{
public:
	SET_NO_INHERITED( NetSendTransport );

	NetSendTransport( bool primary = true )		: ManualSingleton <NetSendTransport> ( primary )  {  }
	virtual ~NetSendTransport( void )			{  }

// Type control.

	// begin accumulating buffer
	virtual Cookie BeginRPC( DWORD toAddress = RPC_TO_ALL, bool shouldRetry = false ) = 0;
	virtual Cookie BeginMultiRPC( DWORD* toAddress, int Addresses, bool shouldRetry = false ) = 0;

	// send buffer via some network transport
	virtual DWORD EndRPC( void ) = 0;

// Sending.

	// this accumulates a FuBi packet in the buffer
	UINT Store( const void* mem, size_t size )
	{
		gpassert( (mem != NULL) || (size == 0) );	// parameter validation
		gpassert( size < 100000 );					// sanity check

		UINT old = m_Buffer.size();
		if ( size != 0 )				// sending nothing is ok (handle general case)
		{
			const BYTE* memb = rcast <const BYTE*> ( mem );
			m_Buffer.insert( m_Buffer.end(), memb, memb + size );
		}
		return ( old );
	}

	template <typename T>
	UINT Store( const T& obj )
	{
		UINT old = m_Buffer.size();
		m_Buffer.insert( m_Buffer.end(),
						 rcast <const BYTE*> ( &obj ),
						 rcast <const BYTE*> ( &obj + 1 ) );
		return ( old );
	}

// Patching.

	UINT  GetIndex       ( void       ) const	{  return ( m_Buffer.size() );  }
	void* GetPtrFromIndex( UINT index )			{  gpassert( index <= m_Buffer.size() );  return ( &*(m_Buffer.begin() + index) );  }

// Flow control.

	//virtual bool Transport( void ) = 0;

	typedef stdx::fast_vector <BYTE> Buffer;

protected:

	Buffer m_Buffer;

	SET_NO_COPYING( NetSendTransport );
};

#define gFuBiNetSendTransport NetSendTransport::GetSingleton()

//////////////////////////////////////////////////////////////////////////////
// class NetReceiveTransport declaration

class NetReceiveTransport : public ManualSingleton <NetReceiveTransport>
{
public:
	SET_NO_INHERITED( NetSendTransport );

	NetReceiveTransport( void )					{  m_ReceivePoint = NULL;  }
	virtual ~NetReceiveTransport( void )		{  }

// Receive.

	void ReceiveCopy( void* mem, size_t size )
	{
		gpassert( mem != NULL );					// parameter validation
		gpassert( size < 100000 );					// sanity check
		gpassert( Remaining() >= (int)size );		// there better be room

		if ( size != 0 )
		{
			::memcpy( mem, m_ReceivePoint, size );
			m_ReceivePoint += size;
		}
	}

	template <typename T>
	void ReceiveCopy( T& obj )
	{
		gpassert( Remaining() >= sizeof( T ) );
		::memcpy( &obj, m_ReceivePoint, sizeof( T ) );
		m_ReceivePoint += sizeof( T );
	}

	void* ReceivePtr( size_t size )
	{
		gpassert( Remaining() >= size );
		void* mem = m_ReceivePoint;
		m_ReceivePoint += size;
		return ( mem );
	}

	template <typename T>
	void ReceivePtr( T*& obj )
	{
		gpassert( Remaining() >= sizeof( T ) );
		obj = rcast <T*> ( m_ReceivePoint );
		m_ReceivePoint += sizeof( T );
	}

	typedef stdx::fast_vector <BYTE> Buffer;

	void TakeBuffer( Buffer& buffer )
	{
		m_Buffer.swap( buffer );
		m_ReceivePoint = &*m_Buffer.begin();
	}

	UINT Remaining( void ) const				{  return ( m_Buffer.end() - m_ReceivePoint );  }

protected:

	Buffer m_Buffer;
	BYTE*  m_ReceivePoint;

	SET_NO_COPYING( NetReceiveTransport );
};

#define gFuBiNetReceiveTransport NetReceiveTransport::GetSingleton()

// this is necessary because i can't call template member functions with an
//  explicit type specifier like i can with globals. vc++ limitation?
template <typename T>
inline const T& ReceiveRef( void )
{
	const T* obj;
	gFuBiNetReceiveTransport.ReceivePtr( obj );
	return ( *obj );
}

//////////////////////////////////////////////////////////////////////////////
// class NetSendDebugTransport declaration

#if !GP_RETAIL

class NetSendDebugTransport : public NetSendTransport
{
public:
	SET_INHERITED( NetSendDebugTransport, NetSendTransport );

	NetSendDebugTransport( void )					{  m_LastExe = 0;  }
	virtual ~NetSendDebugTransport( void )			{  }

	typedef stdx::fast_vector <DWORD> ServerColl;

	void AddServer ( DWORD id, const char* server )	{  m_ExeColl[ id ] = server ? server : "";  }
	void ScanServer( DWORD localId, const char* server );
	void GetServers( ServerColl& servers );

// Type control.

	virtual Cookie BeginRPC     ( DWORD toAddress, bool shouldRetry );
	virtual Cookie BeginMultiRPC( DWORD* toAddress, int Addresses, bool shouldRetry = false );
	virtual DWORD  EndRPC       ( void );

// Flow control.

	bool Transport( void );

private:
	void UpdateBuffer( void );

	typedef std::map <DWORD, Buffer> BufferColl;
	typedef std::map <DWORD, gpstring> ExeColl;

	ExeColl    m_ExeColl;
	BufferColl m_BufferColl;
	DWORD      m_LastExe;

	SET_NO_COPYING( NetSendDebugTransport );
};

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class NetReceiveDebugTransport declaration

#if !GP_RETAIL

class NetReceiveDebugTransport : public NetReceiveTransport
{
public:
	SET_INHERITED( NetReceiveDebugTransport, NetReceiveTransport );

	NetReceiveDebugTransport( void );
	virtual ~NetReceiveDebugTransport( void );

	bool  SetExeId( DWORD id );
	DWORD GetExeId( void ) const;

// Flow control.

	bool Transport( void );

private:

	void Dispatch( DWORD callerAddr );

	DWORD  m_ExeId;
	HANDLE m_Mailslot;

	SET_NO_COPYING( NetReceiveDebugTransport );
};

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

#endif  // __FUBINETTRANSPORT_H

//////////////////////////////////////////////////////////////////////////////
