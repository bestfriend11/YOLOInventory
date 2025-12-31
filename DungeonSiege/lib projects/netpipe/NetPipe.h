#pragma once
/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		:	NetPipe.h

	Author(s)	:	Bartosz Kijanka

	Purpose		:	Abstracton for DP8 interface.

  
	$$ ToDo:
	--------
	-	better updating of session params... don't use operator =, make Update() method
	-	NetPipeSession keep track of time they were last updated, and have interface to query delta - use by UI
	-	dynamically change the name of session to include some stats like map name...
	-	embed extra informaiton in session name in some standard format... comma delimited?
	-	have each player send to Server the local NetPipe clock drift and WorldTime clock drift once per second

	(C)opyright 2001 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#ifndef __NETPIPE_H
#define __NETPIPE_H


class  NetPipe;
class  NetPipeSession;
class  NetPipeSessionHandle;
struct NetPipeMachine;

#include <dxerr8.h>
#include <dplay8.h>
#include <dplobby8.h>

#include "FuBi.h"
#include "ContentDb.h"
#include "gpcore.h"
#include "GpZlib.h"
#include "config.h"
#include "Kerneltool.h"
#include <string>
#include <vector>

extern GUID GPG_NetPipe_GUID;
extern GUID GPG_GUID_INVALID;
extern GUID DEBUG_GUID_1;
extern GUID DEBUG_GUID_2;
extern GUID DEBUG_GUID_3;


/*===========================================================================

	Class		: NetPipeMachine


	Author(s)	: Bartosz Kijanka

	Purpose		: This represents a member in a session.  Each player/client
				  must have a connection to a session if he is to communicate
				  with other players/clients of the game.

---------------------------------------------------------------------------*/
struct NetPipeMachine
{

public:

	NetPipeMachine();

	bool IsLocal() const					{ return( m_bLocal ); }
	bool IsRemote() const					{ return( !m_bLocal ); }
	bool IsHost() const						{ return( m_bHost ); }
	float GetTimeElapsedSinceLastReceive()	{ return( float( GetGlobalSeconds() - m_LastReceiveTime ) ); }
	float GetTimeElapsedSinceLastSend()		{ return( float( GetGlobalSeconds() - m_LastSendTime ) ); }

	////////////////////////////////////////
	//	debug

	void Dump( ReportSys::ContextRef ctx = NULL );

	////////////////////////////////////////
	//	data members

    zlib::DeflateStreamer m_compress;
    zlib::InflateStreamer m_decompress;

	gpwstring m_wsName;
	gpwstring m_wsAddress;
	DWORD     m_Id;

	double	  m_LastReceiveTime;
	double    m_LastSendTime;

	bool      m_bLocal; 
	bool      m_bHost;
	float	  m_Latency;
	DWORD	  m_SendQueue;

	DWORD	  m_BytesSent;
	DWORD	  m_BytesReceived;

	DWORD	  m_PlayersSynced;
	bool	  m_LocalPlayerCreated;

#if !GP_RETAIL
	DWORD	  m_lastPacketId;
#endif

	DPN_CONNECTION_INFO m_dciLast;

	SET_NO_COPYING( NetPipeMachine );
};


// can't use auto vector due to way collections are manipulated--what we really need is ref counting :-)
typedef std::vector< NetPipeMachine *> NetPipeMachineColl;

const DWORD NETPIPE_MAX_MACHINES = 8;


enum eNetPipeEvent
{
	SET_BEGIN_ENUM( NE_, 0 ),

	NE_OK,
	NE_ERROR,
	NE_INVALID,
	
	NE_MACHINE_CONNECTED,
	NE_MACHINE_DISCONNECTED,
	NE_MACHINE_DISCONNECTED_KICKED,
	NE_MACHINE_DISCONNECTED_BANNED,
	NE_MACHINE_DISCONNECTED_TIMEOUT,

	NE_FAILED_CONNECT,
	NE_FAILED_CONNECT_LOCKED,
	NE_FAILED_CONNECT_BANNED,
	NE_FAILED_CONNECT_INCOMPATIBLE_CONTENT,
	NE_FAILED_CONNECT_HOST_REJECTED,
	NE_FAILED_CONNECT_INVALID_PASSWORD,
	NE_FAILED_CONNECT_SESSION_FULL,

	NE_SESSION_CREATED,
	NE_SESSION_CONNECTED,

	NE_SESSION_TERMINATED,

	NE_SESSION_CHANGED,
	NE_SESSION_ADDED,
	NE_NETWORK_UPDATE_MAX_LATENCY,

	SET_END_ENUM( NE_ ),
};

const char* ToString( eNetPipeEvent ne );


bool IsMachineDisconnected( eNetPipeEvent event );


struct NetPipeMessage
{
	NetPipeMessage( NetPipeMessage const & source )
	{
		m_Event		= source.m_Event	;
		m_Double	= source.m_Double	;
		m_Int		= source.m_Int		;
	}

	NetPipeMessage( eNetPipeEvent event )
	{
		ZeroObject( *this );
		m_Event = event;
	}

	NetPipeMessage( eNetPipeEvent event, double var )
	{
		ZeroObject( *this );
		m_Event		= event;
		m_Double	= var;
	}

	NetPipeMessage( eNetPipeEvent event, DWORD var )
	{
		ZeroObject( *this );
		m_Event		= event;
		m_Int		= var;
	}

	eNetPipeEvent	m_Event;
	double			m_Double;
	DWORD			m_Int;
};


typedef CBFunctor1< NetPipeMessage const & > NetPipeMessageCb;
typedef std::vector< NetPipeMessage > NetPipeMessageColl;


////////////////////////////////////////
//	packet enumerations

enum ePacketHeaderType
{
	SET_BEGIN_ENUM( PHT_, 0 ),

	PHT_INVALID,
	PHT_PACKET,
	PHT_RETRY_PACKET,
	PHT_LOCAL_RETRY_PACKET,
	PHT_PACKET_COLL,
	PHT_TIME,
	PHT_VERSION,

	SET_END_ENUM( PHT_ ),
};

const char* ToString      ( ePacketHeaderType pht );
const char* ToScreenString( ePacketHeaderType pht );

#pragma pack ( push, 1 )


////////////////////////////////////////
//	struct PacketCollHeader

struct PacketCollHeader
{
	PacketCollHeader()
	{
		ZeroObject( *this );
		m_Type = PHT_PACKET_COLL;
	}

	BYTE	m_Type;
	DWORD	m_Size;			// = sizeof( PacketPacketCollHeader ) + trailing data size
};


////////////////////////////////////////
//	struct PacketHeader

struct PacketHeader
{
	PacketHeader( ePacketHeaderType type )
	{
		ZeroObject( *this );
		m_Type = scast<BYTE>(type);
	}

	BYTE  	m_Type;
	DWORD 	m_Size;			// = sizeof( PacketHeader ) + trailing data size
};


////////////////////////////////////////
//	struct TimeHeader

struct TimeHeader
{
	TimeHeader()
	{
		ZeroObject( *this );
		m_Type = PHT_TIME;
	}

	BYTE 	m_Type;

	float	m_Time;
};

////////////////////////////////////////
//	struct NetPipeVersion

struct NetPipeVersion
{
	enum eBuildType
	{
		BT_DEBUG = 0,
		BT_RELEASE,
		BT_RETAIL,
	};

	NetPipeVersion();

	BYTE						m_Type;					// packet type
	DWORD						m_Size;

	DWORD						m_TransportVersion;
	BYTE						m_BuildType;
	gpversion					m_ProductVersion;
	DWORD						m_ExeCrc;
	FuBi::SysExports::SyncSpec	m_FuBiSyncSpec;

	bool                        m_bCompressed		: 1;
	bool						m_bTimeoutDetect	: 1;
};

#pragma pack ( pop )

bool IsProtocolCompatible( NetPipeVersion const & l, NetPipeVersion const & r );
bool IsExecutableCompatible( NetPipeVersion const & l, NetPipeVersion const & r );
void GetIncompatabilityString( NetPipeVersion const & l, NetPipeVersion const & r, gpstring & out );



/*===========================================================================

	Class		: 	AverageAccumulator

	Author(s)	: 	Bartosz Kijanka

	Purpose		: 	Keep a running average.

---------------------------------------------------------------------------*/
class AverageAccumulator
{
public:

	AverageAccumulator( DWORD maxSamples )
	{
		m_SampleCount	= 0;
		m_MaxSamples	= maxSamples;
		m_Samples.resize( m_MaxSamples, 0 );
		m_Average		= 0.0f;
		m_Dirty			= true;
	}

	~AverageAccumulator()
	{
	}

	void Clear()
	{
		m_SampleCount	= 0;
		m_Average		= 0.0f;
		m_Dirty			= true;
	}

	float GetAverage()
	{
		if( m_Dirty )
		{
			unsigned int end = ( m_SampleCount < m_MaxSamples ) ? m_SampleCount : m_MaxSamples;

			float avg = 0;

			for( unsigned int i = 0; i != end ; ++i )
			{
				avg += m_Samples[i];
			}

			if( m_SampleCount < m_MaxSamples )
			{
				m_Average = m_SampleCount ? ( avg / float( m_SampleCount ) ) : 0.f;
			}
			else
			{
				m_Average = avg / float( m_MaxSamples );
			}
			m_Dirty = false;
		}
		return( m_Average );
	}

	void AddSample( float sample )
	{
		m_Samples[ m_SampleCount % m_MaxSamples ] = sample;
		//gpgenericf(( "added sample %f at point %d\n", sample, samplePoint ));
		++m_SampleCount;
		m_Dirty = true;
	}

	void Saturate( float sample )
	{
		for( DWORD i = 0; i != m_MaxSamples; ++i )
		{
			m_Samples[i] = sample;
		}
		m_SampleCount = m_MaxSamples;
		m_Dirty = true;
	}

	bool IsDirty()						{ return m_Dirty; }

	float GetSampleBufferFullRatio()	{ return float( m_SampleCount ) / float( m_MaxSamples ); }

private:

	bool	m_Dirty;
	float 	m_Average;

	DWORD	m_SampleCount;
	DWORD 	m_MaxSamples;
	std::vector< float > m_Samples;
};


/*===========================================================================

	Class		: 	NetPipeSession

	Author(s)	: 	Bartosz Kijanka

	Purpose		: 	A session is an abstract "meeting place" to which players go to
				  	gather before they play a game.  You must connect to a session
				  	before you may join a game.

---------------------------------------------------------------------------*/
class NetPipeSession
{

public:

#pragma pack ( push, 1 )

	struct Info
	{
		Info();

		DWORD						m_Size;

		NetPipeVersion				m_NetPipeVersion;
		ContentDb::SyncSpec			m_ContentSyncSpec;

		DWORD						m_WorldState;
		DWORD						m_Magic;
	};

#pragma pack ( pop )

	// existence
	NetPipeSession();
	~NetPipeSession();

	void SUpdate();

	////////////////////////////////////////////////////////////////////////////////
	//	session identification

	gpwstring const & GetHostAddress() const				{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return( m_wsHostAddress ); }
	void SetHostAddress( gpwstring const & addr )			{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_wsHostAddress = addr; }

	gpwstring const & GetDeviceAddress() const				{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return( m_wsDeviceAddress );	}
	void SetDeviceAddress( gpwstring const & addr )			{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_wsDeviceAddress = addr; }

	GUID GetApplicationGuid() const							{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return( m_ApplicationGuid ); }
	void SetApplicationGuid( GUID guid )					{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_ApplicationGuid = guid; }

	DWORD GetMagic() const									{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return( m_Info.m_Magic ); }
	DWORD GetLocalId() const								{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return( m_LocalId ); }

	GUID GetApplicationInstanceGuid() const					{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return( m_ApplicationInstanceGuid ); }
	void SetApplicationInstanceGuid( GUID guid )			{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_ApplicationInstanceGuid = guid; }

	gpwstring const & GetSessionName() const				{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return( m_wsSessionName ); }
	void SetSessionName( gpwstring const & name )			{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_wsSessionName = name; }

	////////////////////////////////////////////////////////////////////////////////
	//	session states

	bool GetHasPassword() const								{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return( m_bHasPassword ); }
	void SetHasPassword( bool Flag )						{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_bHasPassword = Flag;  }

	gpwstring const & GetSessionPassword() const			{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return( m_wsSessionPassword ); }
	void SetSessionPassword( gpwstring const & password )	{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_wsSessionPassword = password; m_bHasPassword = !password.empty(); }

	float GetLatency() const								{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return( m_Latency ); }
	void  SetLatency( float latency )						{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_Latency = latency; }

	bool IsLocked()	const									{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return m_bLocked; }
	void SetIsLocked( bool Flag )							{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_bLocked = Flag; }

	bool IsExecutableCompatible() const						{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return m_bExecutableCompatible; }
	void SetIsExecutableCompatible( bool flag )				{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_bExecutableCompatible = flag; }

	bool IsContentCompatible() const						{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return m_bContentCompatible; }
	void SetIsContentCompatible( bool flag )				{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_bContentCompatible = flag; }

	bool IsHost() const										{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return m_bHost; }
	void SetIsHost( bool Flag )								{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_bHost = Flag; }

	float GetTimeElapsedSinceLastUpdate()					{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return float( GetGlobalSeconds() - m_TimeLastUpdated ); }
	void SetTimeLastUpdated( double time )					{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_TimeLastUpdated = time; }

	////////////////////////////////////////////////////////////////////////////////
	//	extra session info

	// $$ not completely safe
	Info & GetInfo()										{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return m_Info; }

	////////////////////////////////////////////////////////////////////////////////
	//	machines/connections

	unsigned int GetNumMachinesMax() const					{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return( m_NumMachinesMax ); }
	void SetNumMachinesMax( unsigned int num )				{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_NumMachinesMax = num; }

	unsigned int GetNumMachinesPresent() const				{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return( m_NumMachinesPresent ); }
	void SetNumMachinesPresent( unsigned int num )			{ kerneltool::RwCritical::WriteLock lock( m_Critical ); m_NumMachinesPresent = num; }

	NetPipeMachineColl::const_iterator GetMachinesBegin( void ) const			{  return ( m_Machines.begin() );  }
	NetPipeMachineColl::const_iterator GetMachinesEnd  ( void ) const			{  return ( m_Machines.end  () );  }
	NetPipeMachineColl::iterator       GetMachinesBegin( void )					{  return ( m_Machines.begin() );  }
	NetPipeMachineColl::iterator       GetMachinesEnd  ( void )					{  return ( m_Machines.end  () );  }

	NetPipeMachineColl const & GetMachines() const 			{ kerneltool::RwCritical::ReadLock lock( m_Critical ); return m_Machines; }

	NetPipeMachine * GetMachine( DWORD guid );
	NetPipeMachine const * GetLocalNetPipeMachine();
	bool HasMachine( DWORD MachineId );

	////////////////////////////////////////
	//	util

	void AddMachine( NetPipeMachine * pmachine );
	void RemoveMachine( DWORD id );

	void AddBanned( DWORD machineId );
	bool IsBanned( gpwstring const & address );

	////////////////////////////////////////
	//	debug

	void Dump( ReportSys::ContextRef ctx = NULL );

	NetPipeSession & operator = ( NetPipeSession const & rhs );


private:

	NetPipeMachine * PrivateGetMachine( DWORD machineId );

	kerneltool::RwCritical	m_Critical;

	////////////////////////////////////////
	//	session

	double					m_TimeLastUpdated;

	gpwstring				m_wsHostAddress;
	gpwstring				m_wsDeviceAddress;

	Info					m_Info;

	static DWORD			m_InstanceCount;
	DWORD					m_LocalId;

	////////////////////////////////////////
	//	app

	GUID					m_ApplicationGuid;
	GUID               		m_ApplicationInstanceGuid;

	gpwstring              	m_wsSessionName;
	gpwstring              	m_wsSessionPassword;
	float					m_Latency;
	bool					m_bHasPassword;

	DWORD                 	m_Flags;

	bool					m_bExecutableCompatible;
	bool					m_bContentCompatible;
	bool                  	m_bLocked;
	bool                  	m_bHost;

	////////////////////////////////////////
	//	Machines

	NetPipeMachineColl					m_Machines;
	DWORD             					m_NumMachinesMax;
	DWORD             					m_NumMachinesPresent;

	typedef std::vector< gpwstring > BannedColl;
	BannedColl m_BannedColl;

	SET_NO_INHERITED( NetPipeSession );
};

bool IsContentCompatible( NetPipeSession::Info const & l, NetPipeSession::Info const & r );
void GetIncompatabilityString( NetPipeSession::Info const & l, NetPipeSession::Info const & r, gpstring & out );


class NetPipeSessionColl : public std::vector< NetPipeSession * >
{
public:
	typedef std::vector< NetPipeSession * > Parent;
	~NetPipeSessionColl()
	{
		clear();
	}

	void clear()
	{
		for( NetPipeSessionColl::iterator i = this->begin(); i != this->end(); ++i )
		{
			delete (*i);
		}
		Parent::clear();
	}

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

typedef std::vector< NetPipeSessionHandle > NetPipeSessionHandleColl;



////////////////////////////////////////////////////////////////////////////////
//


struct IncomingPacket
{
	enum eReceivedBy
	{
		RB_SERVER = 1,
		RB_CLIENT,
	};

	IncomingPacket( DPNID from, DPNHANDLE handle, PBYTE buffer, DWORD size )
	{
		ZeroObject( *this );

		m_From				= from;
		m_hBufferHandle 	= handle;
		m_pBuffer			= buffer;
		m_Size				= size;
	}

	bool		m_MarkedForDeletion;
	eReceivedBy	m_ReceivedBy;
	DPNID		m_From;
	DPNHANDLE 	m_hBufferHandle;
	PBYTE	 	m_pBuffer;
	DWORD	 	m_Size;
};

typedef std::list< IncomingPacket * > PacketColl;
typedef std::map< DWORD, PacketColl * > MachineToPacketMap;


/*===========================================================================

	Class		: NetPipe

	Author(s)	: Bartosz Kijanka

	Purpose		: This is the main interface class for making a network
				  connection.  It is through this that find a game session and
				  make a connection to it.  After a physical connection is
				  made to a session, the game gas to do the rest: make
				  a Player/Client at assosiate it with that connection.

---------------------------------------------------------------------------*/

extern const DWORD MAX_PACKET_SIZE;

class NetPipe : public Singleton< NetPipe >
{


public:

	typedef stdx::fast_vector< BYTE > ByteBuffer;
	typedef std::map< ByteBuffer *, ByteBuffer * > ByteBufferMap;

	struct ContentResource
	{
		ContentResource()
		{
			m_Guid		= GPG_GUID_INVALID;
			m_Filename	= "undefined";
		}

		GUID		m_Guid;
		gpstring	m_Filename;
	};

	class AutoContentResourceColl : public stdx::fast_vector< ContentResource * >
	{
	public:
		AutoContentResourceColl(){};
		~AutoContentResourceColl();
	};

	NetPipe();
	~NetPipe();

	void InitClient( NetPipeMessageCb cb );
	void InitServer( void );

	void SetCompressed( bool compressed );
	void Shutdown();

	bool IsInitialized();
	void SetProductVersion( gpversion const & version )			{ m_ProductVersion = version; }
	gpversion const & GetProductVersion() const					{ return m_ProductVersion; }

	bool HasTurtle()											{ return m_bHasTurtle; }

	////////////////////////////////////////
	//	update - incoming traffic gets queued up and processed only on a call to Update

	// call this once per frame
	void UpdateLatencies();
	void UpdateCallbacks();

	// call on demand
	bool Send( DWORD ToMachineId, unsigned char * pData, unsigned int Size, bool guarantee = true );

	bool FindFirstReceivePacket();
	bool GetReceivePacket( DWORD & fromMachine, BYTE *& buffer, DWORD & size );
	void ReturnProcessedReceivePackets();
	void ReturnReceivePackets( DWORD machineId );
	void ReturnAllReceivePackets();

	////////////////////////////////////////
	//	time

	float GetMaxLatency()
	{
	    return m_latencyMax;
	}

	DWORD GetPort()												{ return m_Port; }
	bool GetDPNSVR()											{ return m_DPNSVR; }
	float GetConnectionTimeout()								{ return m_ConnectionTimeout; }
	void SetConnectionTimeout( float timeout )					{ m_ConnectionTimeout = timeout; }

	void SetOutboundTrafficBackupThreashold( DWORD bytes )		{ m_OutboundTrafficBackupThreashold = bytes; }
	float GetOutboundTrafficBackupDuration();

	////////////////////////////////////////
	//	sessions

	eNetPipeEvent	RequestSessionEnumeration( IDirectPlay8Address * hostAddress = NULL );
	void			StopSessionEnumeration();

	double 	GetLastSessionEnumerationTime()						{ return( m_LastSessionEnumerationTime ); }

	float	GetTimeElapsedSinceSessionEnumerateRequest()		{ return( float( GetGlobalSeconds() - m_LastSessionEnumerationTime ) ); }

	// $$ this is called during session enumeration only...
	NetPipeSession * AddSession( NetPipeSession const & session );
	void	RemoveSession( GUID guid );

	bool	IsSessionCollDirty()								{ return( m_SessionCollDirty ); }
	void	SetIsSessionCollDirty( bool flag )					{ m_SessionCollDirty = flag; }

	NetPipeSessionHandle GetSessionByLocalId( DWORD id );
	NetPipeSessionHandle GetSession( gpwstring const & name );		// $$$ names are not unique - prefer not using this interface
	NetPipeSessionHandle GetFrontSession();

	NetPipeSessionHandleColl GetSessions();

	eNetPipeEvent FindSessionAtIp( gpwstring const & ip );

	eNetPipeEvent ConnectTo( NetPipeSessionHandle & session, gpwstring const & password );

	void    CancelConnect();
	bool	IsConnectInProgress()								{ return m_bConnectInProgress; }
	void	SetIsConnectInProgress( bool value )				{ m_bConnectInProgress = value; }

	bool 	Host(	gpwstring const & name,
					gpwstring const & password,
					DWORD const maxMachines );

	bool DisconnectMachine( DWORD MachineId, eNetPipeEvent event = NE_MACHINE_DISCONNECTED );

	NetPipeSessionHandle GetOpenSessionHandle();

	bool HasOpenSession()										{ return m_OpenSession != NULL; }
	void ConnectComplete();

	bool CloseOpenSession( bool sendEvent = true );

	////////////////////////////////////////////////////////////////////////////////
	//	session content compatability info - only valid for current open session

	AutoContentResourceColl & GetLocalContentColl()					{ return m_LocalContentColl; }
	AutoContentResourceColl & GetServerRequiredContentColl()		{ return m_ServerRequiredContentColl; }

	void ClearLocalContentResourceColl();
	void ClearServerRequiredResourceColl();

	void AddLocalContentResource( GUID const & guid, gpstring const & filename );

	bool Contains( AutoContentResourceColl const & subset, GUID const & guid );
	bool Equal( AutoContentResourceColl const & l, AutoContentResourceColl const & r );

	ByteBuffer * MakeTempBuffer();
	void FreeTempBuffer( ByteBuffer * buffer );

	void StoreColl( AutoContentResourceColl const & coll, ByteBuffer & buffer );

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

	void Read( ByteBuffer const & buffer, AutoContentResourceColl & coll, DWORD readPoint = 0 );

	template <typename T>
	void ReadCopy( T& obj, ByteBuffer const & in, DWORD & readPoint )
	{
		gpassert( ( in.size() - readPoint ) >= sizeof( T ) );
		::memcpy( &obj, &in[readPoint], sizeof( T ) );
		readPoint += sizeof( T );
	}

	////////////////////////////////////////
	//	util

	kerneltool::Critical & GetSendCritical()					{ return( m_SendCritical );		}
	kerneltool::Critical & GetReceiveCritical()					{ return( m_ReceiveCritical );	}
	kerneltool::Critical & GetNotifyCritical()					{ return( m_NotifyCritical );	}
	kerneltool::Critical & GetSessionCritical()					{ return( m_SessionCritical );	}

	unsigned int GetTotalBytesSent()							{ return m_BytesSent; }
	unsigned int GetTotalBytesReceived()						{ return m_BytesReceived; }
	void AddBytesReceived( DWORD bytes )						{ m_BytesReceived += bytes; }

	unsigned int GetBytesSentDelta( const double TimeNow, double & TimeThen )
	{
		unsigned int Result = m_BytesSent - m_BytesSentLast;
		m_BytesSentLast = m_BytesSent;

		TimeThen = m_LastGetBytesSentDeltaTime;
		m_LastGetBytesSentDeltaTime = TimeNow;

		return Result;
	}

	unsigned int GetBytesReceivedDelta( const double TimeNow, double & TimeThen )
	{
		unsigned int Result = m_BytesReceived - m_BytesReceivedLast;
		m_BytesReceivedLast = m_BytesReceived;

		TimeThen = m_LastGetBytesReceivedDeltaTime;
		m_LastGetBytesReceivedDeltaTime = TimeNow;

		return Result;
	}

	void Notify( NetPipeMessage const & info );
	void OnMachineConnected( NetPipeMachine * pmachine );

	bool IsCompressed()											{ return m_bCompressed; }
	double GetTimeSendPeriod()									{ return m_TimeBroadcastPeriod; }
	void ConnectComplete( eNetPipeEvent hr );

	////////////////////////////////////////
	//	debug

	void Dump( ReportSys::ContextRef ctx = NULL );


private:

	void RegisterNotifyCallback( NetPipeMessageCb cb )			{ m_NetPipeMessageCallback = cb; }
	void SetOpenSession( NetPipeSession * session );

	void ClearSessions();
	void DummyNetPipeMessageCallbackHandler( NetPipeMessage const & ) {};

	////////////////////////////////////////
	//	data

	bool					m_Initialized;
	gpversion				m_ProductVersion;

	kerneltool::Critical	m_SendCritical;
	kerneltool::Critical	m_ReceiveCritical;

	kerneltool::Critical    m_NotifyCritical;
	kerneltool::Critical    m_SessionCritical;

	GUID					m_SelectedAdapterGuid;

	DWORD					m_Port;
	bool					m_DPNSVR;

	NetPipeSessionColl		m_Sessions;
	NetPipeSession *		m_OpenSession;

	NetPipeSession *		m_RequestedOpenSession;
	bool					m_SessionCollDirty;

	NetPipeMessageColl		m_NetPipeMessageColl;
	NetPipeMachineColl		m_AddMachineColl;

	AutoContentResourceColl	m_LocalContentColl;
	AutoContentResourceColl	m_ServerRequiredContentColl;

	ByteBuffer				m_UserConnectData;
	ByteBufferMap			m_TempBufferMap;
	
	////////////////////////////////////////
	//	bandwidth metering

	unsigned int m_LastMessageReceivedSize;
	unsigned int m_BytesSent;
	unsigned int m_BytesReceived;
	unsigned int m_BytesSentComp;
	unsigned int m_BytesRcvdComp;
	unsigned int m_BytesSentUnComp;
	unsigned int m_BytesRcvdUnComp;
	unsigned int m_BytesSentLast;
	unsigned int m_BytesReceivedLast;
	double m_LastGetBytesSentDeltaTime;
	double m_LastGetBytesReceivedDeltaTime;

	////////////////////////////////////////
	//	clock synch/latency
	double					m_LastUpdateTime;
	double					m_LastClockDisciplineTime;

	double					m_LastLatencyQueryTime;
	float					m_LatencyQueryPeriod;

	double					m_LastTimeBroadcastTime;
	float					m_TimeBroadcastPeriod;

	////////////////////////////////////////
	//	time

	float	m_ConnectionTimeout;
	double	m_LastSessionEnumerationTime;
	double	m_LastTimeOutboundTrafficOK;
	DWORD	m_OutboundTrafficBackupThreashold;

	float	m_latencyMax;

	NetPipeMessageCb m_NetPipeMessageCallback;

	bool	m_bEnumInProgress;
	bool	m_bConnectInProgress;
	bool	m_bCompressed;
	bool	m_bHasTurtle;

	SET_NO_COPYING( NetPipe );
	SET_NO_INHERITED( NetPipe );
	FUBI_SINGLETON_CLASS( NetPipe, "Network IO" );
};

#define gNetPipe NetPipe::GetSingleton()




/*===========================================================================

	Class		: 	NetPipeSessionHandle

	Author(s)	: 	Bartosz Kijanka

	Purpose		: 	Makes access to Session thread safe.

---------------------------------------------------------------------------*/
class NetPipeSessionHandle
{
public:

	NetPipeSessionHandle( NetPipe * pipe, NetPipeSession * session )
	{
		pipe->GetSessionCritical().Enter();
		gpassert( pipe );

		m_pPipe		= pipe;
		m_pSession	= session;
	}

	NetPipeSessionHandle( NetPipeSessionHandle const & source )
	{
		gpassert( source.m_pPipe );

		m_pPipe		= source.m_pPipe;
		m_pSession	= source.m_pSession;
		m_pPipe->GetSessionCritical().Enter();
	}
	
	NetPipeSessionHandle & operator = ( const NetPipeSessionHandle & )
	{
		gpassert( "Take it easy, Ace." );
		return ( *this );
	}

	~NetPipeSessionHandle()
	{
		m_pPipe->GetSessionCritical().Leave();
	}

	NetPipeSession * operator->() const		{ if( !m_pSession ) { gperror( "Dereferencing NULL NetPipeSessionHandle" ); } return( m_pSession ); }
	NetPipeSession * GetPointer() const		{ return( m_pSession ); }
	operator bool () const					{ return( m_pSession != NULL ); }


private:

	NetPipe			* m_pPipe;
	NetPipeSession	* m_pSession;

	SET_NO_INHERITED( NetPipeSessionHandle );
};












#endif

