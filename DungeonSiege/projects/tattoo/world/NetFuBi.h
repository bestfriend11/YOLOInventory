#pragma once
/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: 	NetFuBi.h

	Author(s)	: 	Bartosz Kijanka

	Purpose		: 	These classes implement a number of key features for
					our networking layer:

					- IO CPU optimization via send packet accumulation - allowing
					  less IO calls, and saving on packet header space.

					- IO bandwidth optimization via send packet assumulation and compression

	Conventions:
	------------

	All network communication is synchronized to "network time".  Network time
	is based on system time, but is discrete:  Network time is only incremented
	at the beginning of the system-wide master update loop.

	Network time is a double floating point, but it changes at discrete intervals.
	Because of this, multiple packets may be sent or received at the SAME
	"network time."  

	Other Conventions:
	==================

	- Server is always the authority on time.
	- Each client, individually, tries to keep his clock close to the server's


	Todo:
	=====

	- Add time drift detection
	- Time Drift average +/- average incoming packet time from current WorldTime
	- Add app clock discipline
	- may want to NOT accumulate sends on client machines...
	- cleanup non-present machines


	Todo(Debug):
	============
	- NetProbe:
		- server drift averages
		- buffer over and under run values and indicators
	- watch for abnormal jumps in master clock
	- watch for buffer over and under-runs

	(C)opyright 2001 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#ifndef __NETFUBI_H
#define __NETFUBI_H


#include "FuBiNetTransport.h"
#include "NetPipe.h"


class NetPipe;
class GoFollower;

typedef FuBi::NetSendTransport::Buffer ByteBuffer;


////////////////////////////////////////
//	class PacketInfo

struct PacketInfo
{

public:

	PacketInfo()
	{
		ZeroObject( *this );
	}

	PacketInfo( DWORD const machineId, const ByteBuffer & buffer, const ePacketHeaderType packetType)
	{
		ZeroObject( *this );
		
		m_machineId			= machineId;
		m_Type				= packetType;
		m_Buffer			= buffer;
	}

	~PacketInfo(){}

	bool IsRetry() { return PHT_RETRY_PACKET == m_Type;}

	DWORD				m_machineId;
	ePacketHeaderType	m_Type;
   	ByteBuffer  		m_Buffer;
	DWORD				m_RetryCount;
	double				m_TimeStored;
	bool				m_MarkedForDeletion;
};


////////////////////////////////////////
//	types

class AutoPacketsList : public std::list<PacketInfo *> 
{
public:
	typedef std::list<PacketInfo *> Parent;
    ~AutoPacketsList();
    void FreeClear();
    iterator erase(iterator it)
    {
    	delete (*it);
    	return Parent::erase(it);
    }
	iterator erase(iterator /*first*/, iterator /*last*/)
	{
		gpassert(0); // not supported
		return end();
	}
};

class AutoMachineToPacketsMap : public std::map< DWORD, AutoPacketsList * >
{
public:
	typedef std::map< DWORD, AutoPacketsList * > Parent;
	~AutoMachineToPacketsMap();
	void FreeClear();
    iterator erase(iterator it)
    {
    	delete (*it).second;
    	return Parent::erase(it);
    }
	iterator erase(iterator /*first*/, iterator /*last*/)
	{
		gpassert(0); // not supported
		return end();
	}
};


/*===========================================================================

	Class		: NetFuBiSend

	Author(s)	: Bartosz Kijanka

	Purpose		: Talk to NetPipe and send buffer to appropriate player.

---------------------------------------------------------------------------*/
class NetFuBiSend : public FuBi::NetSendTransport, public Singleton< NetFuBiSend >
{
public:

	// setup

	NetFuBiSend();
	~NetFuBiSend();

	void Init();
	void Shutdown();
	void Reset();
	void OnMachineDisconnected( DWORD machineId );
	void SetMute( bool flag )						{  m_bMute = flag; }

	static NetFuBiSend& GetSingleton( void )		{  return ( Singleton<NetFuBiSend>::GetSingleton() );  }

	void Update();
	void ForceUpdate();

FEX	void SetSendDelay( float t )					{ m_SendDelay = t; }
    float GetSendDelay( )					        { return m_SendDelay; }

	////////////////////////////////////////
	//

	void EnableForceUpdate( bool enable = true )	{  m_EnableForceUpdate = enable;  }
	void DisableForceUpdate()						{  EnableForceUpdate( false );  }
	bool IsForceUpdateEnabled() const				{  return ( m_EnableForceUpdate );  }

	virtual FuBiCookie BeginRPC( DWORD MachineID = FuBi::RPC_TO_LOCAL, bool retry = false );
	virtual FuBiCookie BeginMultiRPC( DWORD* MachineIDStart, int addressCount, bool retry = false );
	virtual DWORD EndRPC( void );

	////////////////////////////////////////
	//	debug

#	if !GP_RETAIL

	void Dump( ReportSys::ContextRef ctx = NULL );
	void GetDebugCallHistogram( ReportSys::ContextRef ctx = NULL, bool fReset = false );
FEX	void AddReportFilters( const gpstring& filter );
FEX	void SetReportFilters( const gpstring& filter );
	bool IsFilterMatch( const char* functionName ) const;

	static gpstring DebugGetRpcName( unsigned char const * buffer );
	static gpstring DebugGetRpcInfo( DWORD machineId, unsigned char const * buffer, DWORD packetSize, bool isRpcPacked );

#	endif


private:

	void PrivateForceUpdate();

	void GetAndIncrementNextOutoingSerialForMachine( DWORD const machineId, DWORD & serial );

	void StorePacket(		const DWORD machineId,
					 		const bool retry,
					 		const ByteBuffer & packet );
	void Send( bool force = false );

	bool BuildBlob(		    const DWORD machineId,
					 		ByteBuffer & out );

	bool ExtractPackets(	AutoPacketsList * packets, // just to save finding it again from machineId
							ByteBuffer & out );

	void DeleteMarkedForDeletionPackets( AutoPacketsList * ppackets );

	// send FuBi command over the NetPipe
	bool Send( const DWORD machineId, ByteBuffer & buffer );


	////////////////////////////////////////
	//	helpers for buffer writing

	void Store( const void* mem, size_t size, ByteBuffer & out )
	{
		gpassert( (mem != NULL) || (size == 0) );

		if ( size != 0 )
		{
			const BYTE* memb = rcast <const BYTE*> ( mem );
			out.insert( out.end(), memb, memb + size );
		}
	}

	template <typename T>
	void Store( const T& obj, ByteBuffer & out )
	{
		out.insert( out.end(), rcast <const BYTE*> ( &obj ), rcast <const BYTE*> ( &obj + 1 ) );
	}


	////////////////////////////////////////
	//	data

	kerneltool::Critical		m_Critical;

	bool						m_bMute;
	bool						m_EnableForceUpdate;
	bool						m_ForceUpdateRequested;
	double						m_CurrentDiscreteTime;
	double						m_TimeLastSend;
	float 	   					m_SendDelay;

	DWORD						m_TempToMachineId;
	DWORD *						m_TempToMachineIdStart;
	DWORD						m_TempToMachineIdCount;
	bool						m_TempRetry;

    AutoMachineToPacketsMap     m_machinepackets;

	DWORD						m_LocalMachineSerialIdPrefix;

	////////////////////////////////////////
	//	debug

#	if !GP_RETAIL

	struct DebugCallInfo
	{
		DebugCallInfo( DWORD instanceSize )
		{
			ZeroObject( *this );
			m_CallCount 		= 1;
			m_CallTotalSize 	= instanceSize;
			m_CallInstanceSize 	= instanceSize;
		}

		DWORD m_CallCount;
		DWORD m_CallTotalSize;
		DWORD m_CallInstanceSize;
	};

	typedef std::map< gpstring, DebugCallInfo, istring_less > StringToInstanceCountMap;

	StringToInstanceCountMap m_DebugCallHistogram;
	std::vector <gpstring> m_ReportFilters;

#	endif // !GP_RETAIL

	SET_NO_COPYING( NetFuBiSend );

	FUBI_SINGLETON_CLASS( NetFuBiSend, "Outgoing packet manager." );
};

#define gNetFuBiSend Singleton<NetFuBiSend>::GetSingleton()




/*===========================================================================

	Class		: NetFuBiReceive

	Author(s)	: Bartosz Kijanka

	Purpose		: Talk to NetPipe and receive buffer to appropriate player.

---------------------------------------------------------------------------*/
class NetFuBiReceive : public FuBi::NetReceiveTransport, public Singleton< NetFuBiReceive >
{

public:

	NetFuBiReceive();
	~NetFuBiReceive();

	static NetFuBiReceive& GetSingleton( void )				{ return ( Singleton<NetFuBiReceive>::GetSingleton() ); }

	void Init();
	void Shutdown();
	void Reset();
	void OnMachineDisconnected( DWORD machineId );
	void SetMute( bool flag )								{ m_bMute = flag; }

	void Update();

#if !GP_RETAIL
	void Dump( ReportSys::ContextRef ctx = NULL );
    void Dump( AutoMachineToPacketsMap & mp, ReportSys::ContextRef ctx );
	DWORD DebugGetDbDataSize( AutoPacketsList const & packets );

FEX	void AddReportFilters( const gpstring& filter );
FEX	void SetReportFilters( const gpstring& filter );
#endif

FEX void SetLocalRetryDuration( float duration );


private:

	////////////////////////////////////////
	//	struct RetrySuccessInfo

	struct RetrySuccessInfo
	{
		RetrySuccessInfo( DWORD serial, double successTime )
		{
			m_Serial 		= serial;
			m_SuccessTime	= successTime;
		};

		DWORD 	m_Serial;
		double	m_SuccessTime;
	};

	class AutoSerialSuccessMap : public std::map< DWORD, RetrySuccessInfo * >
	{
	public:
		AutoSerialSuccessMap(){};
		~AutoSerialSuccessMap();

		void FreeClear();
	};

	class AutoMachineSerialSuccessMap : public std::map< DWORD, AutoSerialSuccessMap * >
	{
	public:
		AutoMachineSerialSuccessMap(){};
		~AutoMachineSerialSuccessMap();

		void FreeClear();
	};

	typedef std::multimap< double, RetrySuccessInfo * > TimeSuccessMap;

	class AutoMachineTimeSuccessMap : public std::map< DWORD, TimeSuccessMap * >
	{
	public:
		AutoMachineTimeSuccessMap(){};
		~AutoMachineTimeSuccessMap();

		void FreeClear();
	};

	////////////////////////////////////////
	//	reading from buffer and storing in db

	void Receive();

		void 	ReadBuffer();
		void 	ReadBlob();
//		double 	ReadTime();
		void 	ReadPacket();

	void StoreReceivePacket( DWORD machineId, const ePacketHeaderType, ByteBuffer const & packet );

	////////////////////////////////////////
	//	processing packets in db

	bool ProcessReceivePackets( );
	void InsertLocalRetryPacket( DWORD machineId, PacketInfo const * packetInfo );

	bool ProcessLocalRetryPackets( );

	FuBiCookie ExecuteCurrentRpc( const DWORD fromMachineId );

	////////////////////////////////////////
	//	misc helpers

	template <typename T>
	void ReadCopy( T& obj )
	{
		gpassert( ((m_ReceiveBuffer+m_ReceiveBufferSize) - m_ReceiveBufferPoint) >= sizeof( T ) );
		::memcpy( &obj, m_ReceiveBufferPoint, sizeof( T ) );
		m_ReceiveBufferPoint += sizeof( T );
	}

	////////////////////////////////////////////////////////////////////////////////
	//	data

	bool						m_bMute;
	DWORD						m_ReceiveFromMachineId;
	BYTE *  					m_ReceiveBuffer;
	BYTE *						m_ReceiveBufferPoint;
	DWORD						m_ReceiveBufferSize;

	DWORD						m_ReadCurrentMachineId;
   
    AutoMachineToPacketsMap     m_machinepackets;
    AutoMachineToPacketsMap     m_machineLRpackets; // local retries

	double						m_LastUpdateTime;
	double						m_SecondsElapsedSinceLastUpdate;

	double						m_LocalRetryDuration;
	float						m_LocalRetryPeriod;
	double						m_LocalRetryLastProcessedTime;

	////////////////////////////////////////
	//	debug

	bool						m_InUpdateScope;
	bool						m_InExecuteCurrentRpcScope;

#	if !GP_RETAIL
	std::vector <gpstring>		m_ReportFilters;
#	endif // !GP_RETAIL

	SET_NO_COPYING( NetFuBiReceive );
	FUBI_SINGLETON_CLASS( NetFuBiReceive, "Incoming packet manager." );
};

#define gNetFuBiReceive Singleton<NetFuBiReceive>::GetSingleton()


#endif  // __NETFUBI_H


