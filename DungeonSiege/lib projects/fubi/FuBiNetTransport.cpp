//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiNetTransport.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_FuBi.h"
#include "FuBiNetTransport.h"
#include "FuBi.h"
#include "FileSysUtils.h"

//////////////////////////////////////////////////////////////////////////////

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// helpers

#if !GP_RETAIL

static gpstring GetMailslotName( int exeId )
{
	return ( gpstringf( "\\mailslot\\%s_%d", FileSys::GetFileName( FileSys::GetModuleFileName() ), exeId ) );
}

static gpstring MakeMailslotName( int exeId, const char* server = NULL )
{
	if ( (server == NULL) || (*server == '\0') )
	{
		// it's local
		server = ".";
	}

	gpstring pipe;
	pipe += "\\\\";
	pipe += server;
	pipe += GetMailslotName( exeId );
	return ( pipe );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class NetSendDebugTransport implementation

#if !GP_RETAIL

void NetSendDebugTransport :: ScanServer( DWORD localId, const char* server )
{
	// just do a cheap scan for these pipe names
	for ( DWORD i = RPC_TEST_ADDR_START ; i < (RPC_TEST_ADDR_START + 10) ; ++i )
	{
		// skip local
		if ( i == localId )
		{
			continue;
		}

		// attempt to open
		FileSys::File mailslot;
		if ( mailslot.OpenExisting( MakeMailslotName( i, server ), FileSys::File::ACCESS_WRITE_ONLY ) )
		{
			// add it
			AddServer( i, server );
		}
	}
}

void NetSendDebugTransport :: GetServers( ServerColl& servers )
{
	ExeColl::const_iterator i, begin = m_ExeColl.begin(), end = m_ExeColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		servers.push_back( i->first );
	}
}

Cookie NetSendDebugTransport :: BeginRPC( DWORD toAddress, bool /*shouldRetry*/ )
{
	// special - the "server" is always id RPC_ADDR_START, and "others" same as "all" for us
	if ( toAddress == RPC_TO_SERVER )
	{
		toAddress = RPC_TEST_ADDR_START;
	}
	else if ( toAddress == RPC_TO_OTHERS )
	{
		toAddress = RPC_TO_ALL;
	}

	if ( m_LastExe != toAddress )
	{
		m_LastExe = toAddress;
		UpdateBuffer();
	}

	// always success
	return ( RPC_SUCCESS );
}

Cookie NetSendDebugTransport :: BeginMultiRPC( DWORD* toAddress, int Addresses, bool shouldRetry )
{
	UNREFERENCED_PARAMETER( toAddress );
	UNREFERENCED_PARAMETER( Addresses );
	UNREFERENCED_PARAMETER( shouldRetry );

	GP_Unimplemented$$$();
	return ( RPC_FAILURE_IGNORE );
}

DWORD NetSendDebugTransport :: EndRPC( void )
{
	// this space intentionally left blank...
	return ( 0 );
}

bool NetSendDebugTransport :: Transport( void )
{
	bool success = true;

	// finish
	UpdateBuffer();

	// get at global buffer
	Buffer& globalData = m_BufferColl[ RPC_TO_ALL ];

	// for each client...
	ExeColl::iterator i, begin = m_ExeColl.begin(), end = m_ExeColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		// get at buffer for this client
		Buffer& clientData = m_BufferColl[ i->first ];

		// skip client if no data
		if ( globalData.empty() && clientData.empty() )
		{
			continue;
		}

		// write it out to mail slot
		gpstring mailslotName = MakeMailslotName( i->first, i->second );
		FileSys::File mailslot;
		if ( mailslot.OpenExisting( mailslotName, FileSys::File::ACCESS_WRITE_ONLY ) )
		{
			// write out global data first
			if ( !globalData.empty() && !mailslot.Write( &*globalData.begin(), globalData.size() ) )
			{
				success = false;
				gperrorf(( "Error writing to mailslot '%s' ('%s')\n",
						   mailslotName.c_str(), stringtool::GetLastErrorText().c_str() ));
			}

			// write buffer out and clear it
			if ( !clientData.empty() )
			{
				if ( !mailslot.Write( &*clientData.begin(), clientData.size() ) )
				{
					success = false;
					gperrorf(( "Error writing to mailslot '%s' ('%s')\n",
							   mailslotName.c_str(), stringtool::GetLastErrorText().c_str() ));
				}
				clientData.clear();
			}
		}
		else
		{
			gperrorf(( "Unable to connect to mailslot '%s' ('%s')\n",
					   mailslotName.c_str(), stringtool::GetLastErrorText().c_str() ));
		}
	}

	// clear global data
	globalData.clear();

	// done
	return ( success );
}

void NetSendDebugTransport :: UpdateBuffer( void )
{
	Buffer& buffer = m_BufferColl[ m_LastExe ];
	buffer.insert( buffer.end(), m_Buffer.begin(), m_Buffer.end() );
	m_Buffer.clear();
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class NetReceiveDebugTransport implementation

#if !GP_RETAIL

NetReceiveDebugTransport :: NetReceiveDebugTransport( void )
{
	m_ExeId    = RPC_INVALID_ADDR;
	m_Mailslot = INVALID_HANDLE_VALUE;
}

NetReceiveDebugTransport :: ~NetReceiveDebugTransport( void )
{
	SetExeId( RPC_INVALID_ADDR );
}

bool NetReceiveDebugTransport :: SetExeId( DWORD id )
{
	// bail earily if no change
	if ( m_ExeId == id )
	{
		return ( true );
	}

	// take it
	m_ExeId = id;

	// close the old if open
	if ( m_Mailslot != INVALID_HANDLE_VALUE )
	{
		::CloseHandle( m_Mailslot );
		m_Mailslot = INVALID_HANDLE_VALUE;
	}

	// open the new
	if ( m_ExeId != RPC_INVALID_ADDR )
	{
		// make sure it's open - increment exe id until it is
		for ( ; ; )
		{
			gpstring mailslotName = MakeMailslotName( m_ExeId );
			m_Mailslot = ::CreateMailslot( mailslotName, 0, 0, NULL );
			if ( m_Mailslot == INVALID_HANDLE_VALUE )
			{
				if ( ::GetLastError() == ERROR_ALREADY_EXISTS )
				{
					++m_ExeId;
				}
				else
				{
					gperrorf(( "Unable to create mailslot '%s' ('%s')\n",
							   mailslotName.c_str(), stringtool::GetLastErrorText().c_str() ));
					break;
				}
			}
			else
			{
				return ( true );
			}
		}
	}

	// must have been a non-dup error
	return ( false );
}

DWORD NetReceiveDebugTransport :: GetExeId( void ) const
{
	return ( m_ExeId );
}

bool NetReceiveDebugTransport :: Transport( void )
{
	// ensure it's open
	if ( m_ExeId == RPC_INVALID_ADDR )
	{
		SetExeId( RPC_TEST_ADDR_START );
	}
	gpassert( m_Mailslot != INVALID_HANDLE_VALUE );

	// get the messages
	m_Buffer.resize( 0 );
	DWORD nextSize;
	while (    ::GetMailslotInfo( m_Mailslot, NULL, &nextSize, NULL, NULL )
			&& (nextSize != MAILSLOT_NO_MESSAGE) )
	{
		// read it in
		DWORD oldSize = m_Buffer.size();
		m_Buffer.resize( oldSize + nextSize );
		if ( !::ReadFile( m_Mailslot, &*(m_Buffer.begin() + oldSize), nextSize, &nextSize, NULL ) )
		{
			gperrorf(( "Error reading from mailslot ('%s')\n", stringtool::GetLastErrorText().c_str() ));
			return ( false );
		}
	}

	// dispatch it
	Dispatch( RPC_INVALID_ADDR /* just for testing! */ );

	// done
	return ( true );
}

void NetReceiveDebugTransport :: Dispatch( DWORD callerAddr )
{
	// reset pointer
	m_ReceivePoint = &*m_Buffer.begin();
	while ( m_ReceivePoint != &*m_Buffer.end() )
	{
		gFuBiSysExports.DispatchNextRpc( callerAddr, m_Buffer.size() );
	}

	// ready for another receive session (note that this doesn't free any memory)
	m_Buffer.clear();
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
