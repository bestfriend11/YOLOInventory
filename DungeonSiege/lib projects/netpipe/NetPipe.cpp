#include "precomp_NetPipe.h"

#include "NetPipe.h"
#include "fubi.h"
#include "fubidefs.h"
#include "stringtool.h"
#include "stdhelp.h"
#include "NetLog.h"
#include "ReportSys.h"
#include "gpprofiler.h"
#include "gpmath.h"
#include "AppModule.h"
#include "TankMgr.h"


////////////////////////////////////////////////////////////////////////////////
//	constants and macros

extern GUID GPG_NetPipe_GUID = { 0xff29fd03, 0xe218, 0x11d2, 0xb5, 0xca, 0x0, 0x60, 0x8, 0x68, 0x82, 0x8b };
extern GUID GPG_GUID_INVALID = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

extern GUID DEBUG_GUID_1 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
extern GUID DEBUG_GUID_2 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 };
extern GUID DEBUG_GUID_3 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3 };


const float MAX_DISCIPLINE_PER_MIN		= 1.0f;
const float MAX_DISCIPLINE_PER_SEC		= MAX_DISCIPLINE_PER_MIN / 60.0f;
const DWORD MAX_PACKET_SIZE				= 1024 * 4;

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#define RELEASE_DELETE(p)    { if(p) { (p)->Release(); delete (p); (p)=NULL; } }

HRESULT WINAPI DPServerMessageHandler(	PVOID pvUserContext, 
                                        DWORD dwMessageId, 
										PVOID pMsgBuffer );

HRESULT WINAPI DPClientMessageHandler(	PVOID pvUserContext, 
										DWORD dwMessageId, 
										PVOID pMsgBuffer );

class DPlayCloseServerThread : public kerneltool::Thread
{
public:
	DPlayCloseServerThread( IDirectPlay8Server *pDp );

protected:
	DWORD Execute();
	IDirectPlay8Server * m_pDP;
};


class DPlayCloseClientThread : public kerneltool::Thread
{
public:
	DPlayCloseClientThread( IDirectPlay8Client *pDp );

protected:
	DWORD Execute();
	IDirectPlay8Client * m_pDP;
};


////////////////////////////////////////////////////////////////////////////////
//

static const char* s_NetPipeEventStrings[] =
{
	"NE_OK",
	"NE_ERROR",
	"NE_INVALID",
	
	"NE_MACHINE_CONNECTED",
	"NE_MACHINE_DISCONNECTED",
	"NE_MACHINE_DISCONNECTED_KICKED",
	"NE_MACHINE_DISCONNECTED_BANNED",
	"NE_MACHINE_DISCONNECTED_TIMEOUT",

	"NE_FAILED_CONNECT",
	"NE_FAILED_CONNECT_LOCKED",
	"NE_FAILED_CONNECT_BANNED",
	"NE_FAILED_CONNECT_INCOMPATIBLE_CONTENT",
	"NE_FAILED_CONNECT_HOST_REJECTED",
	"NE_FAILED_CONNECT_INVALID_PASSWORD",
	"NE_FAILED_CONNECT_SESSION_FULL",

	"NE_SESSION_CREATED",
	"NE_SESSION_CONNECTED",

	"NE_SESSION_TERMINATED",

	"NE_SESSION_CHANGED",
	"NE_SESSION_ADDED",
	"NE_NETWORK_UPDATE_MAX_LATENCY",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_NetPipeEventStrings ) == NE_COUNT );

static stringtool::EnumStringConverter s_NetPipeEventConverter(
		s_NetPipeEventStrings, NE_BEGIN, NE_END, NE_OK );

const char* ToString( eNetPipeEvent ne )
{
	return ( s_NetPipeEventConverter.ToString( ne ) );
}


////////////////////////////////////////////////////////////////////////////////
//	types

static const char* s_PacketHeaderTypeStrings[] =
{
	"PHT_INVALID",
	"PHT_PACKET",
	"PHT_RETRY_PACKET",
	"PHT_LOCAL_RETRY_PACKET",
	"PHT_PACKET_COLL",
	"PHT_TIME",
	"PHT_VERSION",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_PacketHeaderTypeStrings ) == PHT_COUNT );

static stringtool::EnumStringConverter s_PacketHeaderTypeConverter(
		s_PacketHeaderTypeStrings, PHT_BEGIN, PHT_END, PHT_INVALID );

const char* ToString( ePacketHeaderType pht )
{
	return ( s_PacketHeaderTypeConverter.ToString( pht ) );
}

const char* ToScreenString( ePacketHeaderType pht )
{
	// skip "PHT_"
	return ( ToString( pht ) + 4 );
}


////////////////////////////////////////////////////////////////////////////////
//


bool IsMachineDisconnected( eNetPipeEvent event )
{
	switch( event )
	{
		case NE_MACHINE_DISCONNECTED:	
		case NE_MACHINE_DISCONNECTED_KICKED:
		case NE_MACHINE_DISCONNECTED_BANNED:
		case NE_MACHINE_DISCONNECTED_TIMEOUT:
		{
			return( true );
			break;
		}
		default:
		{
			return( false );
		}
	};
}


////////////////////////////////////////////////////////////////////////////////
//

kerneltool::Critical	g_IncomingRawCritical;
PacketColl				g_IncomingRaw;

MachineToPacketMap				g_IncomingPackets;
MachineToPacketMap::iterator	g_IncomingMachineIterator;
PacketColl::iterator			g_IncomingPacketIterator;


////////////////////////////////////////////////////////////////////////////////
//


NetPipeVersion :: NetPipeVersion()
{
	ZeroObject( *this );

	m_Type 			= PHT_VERSION;
	m_Size 			= sizeof( NetPipeVersion );

	#if GP_DEBUG
	m_BuildType	= BT_DEBUG;
	#elif GP_RELEASE
	m_BuildType	= BT_RELEASE;
	#elif GP_RETAIL
	m_BuildType	= BT_RETAIL;
	#endif

	m_TransportVersion	= '0100';
	
	m_ProductVersion 	= gNetPipe.GetProductVersion();
	m_ExeCrc			= gAppModule.GetAppCrc32();
	m_FuBiSyncSpec 		= gFuBiSysExports.MakeSynchronizeSpec();

	m_bCompressed       = gNetPipe.IsCompressed();
	m_bTimeoutDetect	= gNetPipe.HasTurtle();
}

////////////////////////////////////////////////////////////////////////////////
//	debug

bool IsValidIncomingMachineId( DWORD machineId )
{
	return( ( machineId != RPC_TO_ALL ) && ( machineId != RPC_TO_OTHERS ) && ( machineId != RPC_INVALID_ADDR ) );
}








///////////////////////////////////////////////////////////////////////////////////////////
//	class NetPipe implementation
///////////////////////////////////////////////////////////////////////////////////////////




NetPipe::AutoContentResourceColl::~AutoContentResourceColl()
{
	for( AutoContentResourceColl::iterator i = begin(); i != end(); ++i )
	{
		delete( *i );
	}
	clear();
}




IDirectPlay8Server * g_pDPlayServer = 0;
IDirectPlay8Client * g_pDPlayClient = 0;


///////////////////////////////////////////////////////////////////////////////
//	
NetPipe :: NetPipe()
{

#if !GP_DISABLE_MULTIPLAYER

	// test assumptions
	gpassert( sizeof( unsigned int ) == sizeof( unsigned long ) );
	gpassert( sizeof( DWORD ) == sizeof( unsigned long ) );

	gpassert( !g_pDPlayServer && !g_pDPlayClient );
	g_pDPlayServer = 0;
	g_pDPlayClient = 0;

	////////////////////////////////////////
	//	class variables

	m_Initialized						= false		   ;
	m_BytesSentComp						= 0;
	m_BytesRcvdComp						= 0;
	m_BytesSentUnComp					= 0;
	m_BytesRcvdUnComp					= 0;

	m_LastMessageReceivedSize       = 0                ;
	m_BytesSent                     = 0                ;
	m_BytesReceived                 = 0                ;
	m_BytesSentLast                 = 0                ;
	m_BytesReceivedLast             = 0                ;
	m_LastGetBytesSentDeltaTime     = 0                ;
	m_LastGetBytesReceivedDeltaTime = 0                ;

	m_LastSessionEnumerationTime	  = 0              ;
	m_SessionCollDirty              = false            ;

	m_OpenSession                   = NULL             ;
	m_RequestedOpenSession          = NULL             ;

	m_SelectedAdapterGuid           = GPG_GUID_INVALID ;

	m_LastLatencyQueryTime          = 0                ;
	m_LatencyQueryPeriod            = 0.5f             ;
	
	m_LastUpdateTime                = 0                ;
	m_LastClockDisciplineTime       = 0                ;

	m_TimeBroadcastPeriod			= 1.0			   ;

	// timeouts etc
#	if DEBUG
	m_ConnectionTimeout				  = 60.0;
#	else
	m_ConnectionTimeout				  = 20.0;
#	endif

	m_OutboundTrafficBackupThreashold = 1024;
	m_LastTimeOutboundTrafficOK		  = GetGlobalSeconds() ;

	m_bEnumInProgress				= false;
	m_bConnectInProgress			= false;

	m_bCompressed	= true;
	m_bHasTurtle	= true;
	m_Port			= gConfig.GetInt( "port", 0 );
	m_DPNSVR		= gConfig.GetBool( "dpnsvr", true );

	FuBi::SetRpcIdsToDefaults();

	RegisterNotifyCallback( makeFunctor( *this, &NetPipe::DummyNetPipeMessageCallbackHandler ) );

	GPCoInitialize();

#endif
}


///////////////////////////////////////////////////////////////////////////////
//	
NetPipe :: ~NetPipe()
{	
	Shutdown();
}


////////////////////////////////////////////////////////////////////////////////
//	

gpstring sInitFailure( $MSG$ "Could not initialize DirectPlay 8 (this is needed "
								 "for multiplayer games).\n\n[Note: This error can "
								 "occur if DirectX 8 (or newer) is not installed on "
								 "this system. Please re-run the game's "
								 "installation program to set this up. Alternately, "
								 "you can point your web browser at "
								 "www.microsoft.com/directx or use Windows Update "
								 "to install the latest version of DirectX.]\n\n"
								 "Error code = 0x%08X" );

void NetPipe :: InitClient( NetPipeMessageCb cb )
{
#if !GP_DISABLE_MULTIPLAYER

	kerneltool::Critical::Lock lock1( m_SendCritical		);
	kerneltool::Critical::Lock lock2( m_ReceiveCritical		);
	kerneltool::Critical::Lock lock3( m_NotifyCritical		);
	kerneltool::Critical::Lock lock4( m_SessionCritical		);
	kerneltool::Critical::Lock lock5( g_IncomingRawCritical );

#if !GP_RETAIL
	if( !HasDungeonSiegeSpecialEnableDsrMpFile() )
	{
		return;
	}
#endif

    m_bCompressed = !gConfig.GetBool(	"nocompress" );
	m_bHasTurtle  = gConfig.GetBool(	"turtle", true );

	RegisterNotifyCallback( cb );

	// leave these checks in
    StopSessionEnumeration();

	m_NetPipeMessageColl.clear();

	g_IncomingMachineIterator	= g_IncomingPackets.end();
	m_RequestedOpenSession		= NULL;

	////////////////////////////////////////
	//	DPlay

	bool success = false;
	bool initializedClient = false;

	if( !g_pDPlayClient )
	{
		gpassert( g_pDPlayClient == 0 );

		gpassert( RPC_TO_SERVER	== FuBi::RPC_TO_SERVER_LOCAL );
		gpassert( RPC_TO_LOCAL	== FuBi::RPC_TO_LOCAL_DEFAULT );

		// init COM library
		GPCoInitialize();

		// Create IDirectPlay8Peer
		HRESULT hr = CoCreateInstance(	CLSID_DirectPlay8Client,
										NULL, 
										CLSCTX_INPROC_SERVER, 		// The code that creates and manages objects of this class runs in the same process as the caller of the function specifying the class context
										IID_IDirectPlay8Client,	
										(LPVOID*) &g_pDPlayClient );
		if( FAILED( hr ) )
		{
			gpwatsonf( ( sInitFailure.c_str(), hr ) );
		}

		// Init IDirectPlay8Peer
		success = SUCCEEDED( g_pDPlayClient->Initialize( NULL, DPClientMessageHandler, 0 ) );
		initializedClient = success;

		////////////////////////////////////////
		//	set info

		gpwstring name( ToUnicode( SysInfo::GetComputerName() ) );
		gpassert( !name.empty() );
		DPN_PLAYER_INFO dpInfo;
		ZeroMemory( &dpInfo, sizeof(DPN_PLAYER_INFO) );

		dpInfo.dwSize      = sizeof(DPN_PLAYER_INFO);
		dpInfo.dwInfoFlags = DPNINFO_NAME;
		dpInfo.pwszName    = const_cast<WCHAR*>( name.c_str() );

		TRYDX( g_pDPlayClient->SetClientInfo( &dpInfo, NULL, NULL, DPNOP_SYNC ) );

		////////////////////////////////////////
		//	set CAPs

		if( !m_bHasTurtle )
		{
			DPN_CAPS caps;
			memset( &caps, 0, sizeof( DPN_CAPS ) );
			caps.dwSize = sizeof( DPN_CAPS );
			TRYDX( g_pDPlayClient->GetCaps( &caps, 0 ) );

			caps.dwTimeoutUntilKeepAlive = INFINITE;
			TRYDX( g_pDPlayClient->SetCaps( &caps, 0 ) );
		}
	}

	RegisterNotifyCallback( cb );

	if( g_pDPlayClient )
	{
		// get tcpip provider caps

		//	CLSID_DP8SP_TCPIP
		DPN_SP_CAPS spcaps;
		ZeroMemory( &spcaps, sizeof(DPN_SP_CAPS) );
		spcaps.dwSize = sizeof(DPN_SP_CAPS);

		TRYDX( g_pDPlayClient->GetSPCaps( &CLSID_DP8SP_TCPIP, &spcaps, 0 ) );

		gpgeneric( "TCPIP caps:\n" );
		gpgeneric( "-----------\n" );
		gpgenericf(( "dwSize                     = 0x%08x\n", spcaps.dwSize                     ));
		gpgenericf(( "dwFlags                    = 0x%08x\n", spcaps.dwFlags                    ));
		gpgenericf(( "dwNumThreads               = 0x%08x\n", spcaps.dwNumThreads               ));
		gpgenericf(( "dwDefaultEnumCount         = 0x%08x\n", spcaps.dwDefaultEnumCount         ));
		gpgenericf(( "dwDefaultEnumRetryInterval = 0x%08x\n", spcaps.dwDefaultEnumRetryInterval ));
		gpgenericf(( "dwDefaultEnumTimeout       = 0x%08x\n", spcaps.dwDefaultEnumTimeout       ));
		gpgenericf(( "dwMaxEnumPayloadSize       = 0x%08x\n", spcaps.dwMaxEnumPayloadSize       ));
		gpgenericf(( "dwBuffersPerThread         = 0x%08x\n", spcaps.dwBuffersPerThread         ));
		gpgenericf(( "dwSystemBufferSize         = 0x%08x\n", spcaps.dwSystemBufferSize         ));
	}

	m_Initialized = success;

#endif
}


void NetPipe :: InitServer( void )
{
#if !GP_DISABLE_MULTIPLAYER

	kerneltool::Critical::Lock lock1( m_SendCritical    );
	kerneltool::Critical::Lock lock2( m_ReceiveCritical );
	kerneltool::Critical::Lock lock3( m_NotifyCritical  );
	kerneltool::Critical::Lock lock4( m_SessionCritical );
	kerneltool::Critical::Lock lock5( g_IncomingRawCritical );

#if !GP_RETAIL
	if( !HasDungeonSiegeSpecialEnableDsrMpFile() )
	{
		return;
	}
#endif

	////////////////////////////////////////
	//	DPlay

	bool success = false;
	bool initializedServer = false;

	if( !g_pDPlayServer )
	{
		gpassert( g_pDPlayServer == 0 );
		gpassert( RPC_TO_SERVER	== FuBi::RPC_TO_SERVER_LOCAL );
		gpassert( RPC_TO_LOCAL	== FuBi::RPC_TO_LOCAL_DEFAULT );

		HRESULT hr = CoCreateInstance(	CLSID_DirectPlay8Server,
										NULL, 
										CLSCTX_INPROC_SERVER, 		// The code that creates and manages objects of this class runs in the same process as the caller of the function specifying the class context
										IID_IDirectPlay8Server,	
										(LPVOID*) &g_pDPlayServer );

		if( FAILED( hr ) )
		{
			gpwatsonf(( sInitFailure.c_str(), hr ));
		}

		success = !FAILED( g_pDPlayServer->Initialize( NULL, DPServerMessageHandler, 0 ) );

		initializedServer = success;

		success = true;
	}

	////////////////////////////////////////
	//	set info

	gpwstring name( ToUnicode( SysInfo::GetComputerName() ) );
	DPN_PLAYER_INFO dpInfo;
	ZeroMemory( &dpInfo, sizeof(DPN_PLAYER_INFO) );

	dpInfo.dwSize      = sizeof(DPN_PLAYER_INFO);
	dpInfo.dwInfoFlags = DPNINFO_NAME;
	dpInfo.pwszName    = const_cast<WCHAR*>( name.c_str() );

	TRYDX( g_pDPlayServer->SetServerInfo( &dpInfo, NULL, NULL, DPNOP_SYNC ) );

	////////////////////////////////////////
	//	set CAPs

	if( !m_bHasTurtle )
	{
		DPN_CAPS caps;
		memset( &caps, 0, sizeof( DPN_CAPS  ) );
		caps.dwSize = sizeof( DPN_CAPS );
		TRYDX( g_pDPlayServer->GetCaps( &caps, 0 ) );

		caps.dwTimeoutUntilKeepAlive = INFINITE;
		TRYDX( g_pDPlayServer->SetCaps( &caps, 0 ) );
	}

	m_Initialized = success;	// $$

#endif

}


////////////////////////////////////////////////////////////////////////////////
//	
void NetPipe :: SetCompressed( bool compressed )
{
	kerneltool::Critical::Lock lock1( m_SendCritical    );
	kerneltool::Critical::Lock lock2( m_ReceiveCritical );
	kerneltool::Critical::Lock lock3( m_NotifyCritical  );
	kerneltool::Critical::Lock lock4( m_SessionCritical );
	kerneltool::Critical::Lock lock5( g_IncomingRawCritical );

	if( HasOpenSession() || m_RequestedOpenSession || m_bEnumInProgress || m_bConnectInProgress )
	{
		gperror( "Trying to change compression setting after starting to use NetPipe.  Tell Bart." );
	}
	else
	{
		m_bCompressed = !gConfig.GetBool( "nocompress" ) && compressed;
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
void NetPipe :: Shutdown()
{
	kerneltool::Critical::Lock lock1( m_SendCritical    );
	kerneltool::Critical::Lock lock2( m_ReceiveCritical );
	kerneltool::Critical::Lock lock3( m_NotifyCritical  );
	kerneltool::Critical::Lock lock4( m_SessionCritical );
	kerneltool::Critical::Lock lock5( g_IncomingRawCritical );

	m_Initialized			= false;
	m_RequestedOpenSession	= NULL;

	////////////////////////////////////////
	//	set this peer's CAPs

	RegisterNotifyCallback( makeFunctor( *this, &NetPipe::DummyNetPipeMessageCallbackHandler ) );

	if( g_pDPlayServer || g_pDPlayClient )
	{
		StopSessionEnumeration();

		if( g_pDPlayServer )
		{
			//Create thread to handle close and release
			//because DPlay blocks on close because it waits
			//for communication with server/peer
			DPlayCloseServerThread *pClose= new DPlayCloseServerThread( g_pDPlayServer );
			g_pDPlayServer = NULL;
			pClose->BeginThread("DPlayCloseServer");
		}

		if( g_pDPlayClient )
		{
			DPlayCloseClientThread *pClose= new DPlayCloseClientThread( g_pDPlayClient );
			g_pDPlayClient = NULL;
			pClose->BeginThread("DPlayCloseClient");
		}

		gpassert( !g_pDPlayServer );
		gpassert( !g_pDPlayClient );
	}

	ReturnAllReceivePackets();

	ClearLocalContentResourceColl();
	ClearServerRequiredResourceColl();

	ClearSessions();

	m_SelectedAdapterGuid	= GPG_GUID_INVALID;
	
	FuBi::SetRpcIdsToDefaults();
}


////////////////////////////////////////////////////////////////////////////////
//	SERVER function
void NetPipe :: UpdateLatencies()
{
	NetPipeSessionHandle session = GetOpenSessionHandle();

	if( session && session->IsHost() )
	{
		gpassert( session->IsHost() );

		////////////////////////////////////////
		//	query for latencies1

		double currentTime = GetGlobalSeconds();

		if( ( currentTime - m_LastLatencyQueryTime ) > m_LatencyQueryPeriod )
		{
		    m_latencyMax = 0.0f;

			NetPipeMachineColl::iterator iMachine;

			for( iMachine = session->GetMachinesBegin(); iMachine != session->GetMachinesEnd(); ++iMachine )
			{
				if( (*iMachine)->m_Id == RPC_TO_LOCAL )
				{
					continue;
				}

                DPN_CONNECTION_INFO dci;
                dci.dwSize = sizeof(dci);

				HRESULT hr = g_pDPlayServer->GetConnectionInfo( (*iMachine)->m_Id, &dci, 0 );

                if( SUCCEEDED(hr) )
                {
					// dplay could complain that the player is already marked as gone--don't barf on that
					float latency = dci.dwRoundTripLatencyMS / 1000.0f;
					(*iMachine)->m_Latency = latency;
					if (latency > m_latencyMax)
					{
					    m_latencyMax = latency;
					}
                }
				else
				{
					//TRYDX( hr );
				}
			}

			m_LastLatencyQueryTime = currentTime;

			gNetPipe.Notify( NetPipeMessage( NE_NETWORK_UPDATE_MAX_LATENCY, m_latencyMax ) );
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//
void NetPipe :: UpdateCallbacks()
{
	////////////////////////////////////////
	// move messages local

	NetPipeMessageColl messages;
	{
		kerneltool::Critical::Lock lock( m_NotifyCritical );
		m_NetPipeMessageColl.swap( messages );
	}

	////////////////////////////////////////
	//	send messages accumulated on other thread

	NetPipeMessageColl::iterator i;
	for( i = messages.begin(); i != messages.end(); ++i )
	{
		m_NetPipeMessageCallback( (*i) );
	}

	////////////////////////////////////////
	// process added machines

	NetPipeSessionHandle session = GetOpenSessionHandle();

	if( session )
	{
		NetPipeMachineColl addMachines;
		{
			kerneltool::Critical::Lock lock( m_NotifyCritical );
			m_AddMachineColl.swap( addMachines );
		}

		if( !addMachines.empty() )
		{
			for( NetPipeMachineColl::iterator iMachine = addMachines.begin(); iMachine != addMachines.end(); ++iMachine )
			{
				gpgenericf(( "NetPipe :: DirectPlayMessageHandler - Machine connected %S, id 0x%08x\n", (*iMachine)->m_wsName.c_str(), (*iMachine)->m_Id ));
				session->AddMachine( (*iMachine) );
				m_NetPipeMessageCallback( NetPipeMessage( NE_MACHINE_CONNECTED, (*iMachine)->m_Id ) );
			}
		}
	}
	
	////////////////////////////////////////
	//	update session timeouts etc

	if( session && m_bHasTurtle )
	{
		if( session->IsHost() )
		{
			gpassert( g_pDPlayServer );

			// check for timed out clients
			NetPipeMachineColl machines = session->GetMachines();

			bool underThreashold = true;

			for( NetPipeMachineColl::iterator i = machines.begin(); i != machines.end(); ++i )
			{
				if( ((*i)->m_Id != RPC_TO_LOCAL) && (*i)->GetTimeElapsedSinceLastReceive() > m_ConnectionTimeout )
				{
					gpgenericf(( "Connection for machine %S, id 0x%08x timed out.  Pretending machine has disconnected.\n",
								(*i)->m_wsName.c_str(),
								(*i)->m_Id ));
					m_NetPipeMessageCallback( NetPipeMessage( NE_MACHINE_DISCONNECTED, (*i)->m_Id ) );
					gNetPipe.DisconnectMachine( (*i)->m_Id, NE_MACHINE_DISCONNECTED_TIMEOUT );
				}

				////////////////////////////////////////
				//	outbound traffic backlog detection

				if( underThreashold )
				{
					DWORD messages = 0;
					DWORD bytes = 0;
					HRESULT hr = g_pDPlayServer->GetSendQueueInfo( (*i)->m_Id, &messages, &bytes, 0 );

					if( SUCCEEDED(hr) )
					{
						if( bytes >= m_OutboundTrafficBackupThreashold )
						{
							underThreashold = false;
						}
					}
					else
					{
						//TRYDX(hr);
						messages = 0;
						bytes = 0;
					}
				}
			}

			if( underThreashold )
			{
				m_LastTimeOutboundTrafficOK = GetGlobalSeconds();
			}
		}
		else
		{
			// check for timed out server
			if( session->HasMachine( RPC_TO_SERVER ) )
			{
				if( session->GetMachine( RPC_TO_SERVER )->GetTimeElapsedSinceLastReceive() > m_ConnectionTimeout )
				{
					gpgenericf(( "Connection for machine %S, id 0x%08x timed out.  Pretending machine has disconnected.\n",
								session->GetMachine( RPC_TO_SERVER )->m_wsName.c_str(),
								session->GetMachine( RPC_TO_SERVER )->m_Id ));

					CloseOpenSession( true );
				}
			}
		}
	}

	static double lastTimeDumped;
	if( GetGlobalSeconds() - lastTimeDumped > 5.0 )
	{
		if( gNetLog.IsLogging() )
		{
			gpstring log;

			ReportSys::LocalContext localCont;
			ReportSys::StringSink tempSink;
			localCont.AddSink( &tempSink, false );

			Dump( &localCont );

			log.append( tempSink.GetBuffer() );

			gpnet( log );
		}

		lastTimeDumped = GetGlobalSeconds();
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
void NetPipe :: SetOpenSession( NetPipeSession * session )
{
	gpgenericf(( "Setting m_OpenSession to %S\n", session ? session->GetSessionName().c_str() : L"NULL" ));
	m_OpenSession = session;
}


////////////////////////////////////////////////////////////////////////////////
//	
void NetPipe :: ClearSessions()
{
	kerneltool::Critical::Lock sessionLock( m_SessionCritical );

	m_Sessions.clear();

	SetOpenSession(  NULL );
	m_RequestedOpenSession	= NULL;
	m_SessionCollDirty		= true;
}


////////////////////////////////////////////////////////////////////////////////
//	
bool NetPipe :: IsInitialized()
{
	return g_pDPlayClient != NULL && m_Initialized;
}


////////////////////////////////////////////////////////////////////////////////
//	
eNetPipeEvent NetPipe :: RequestSessionEnumeration( IDirectPlay8Address * pDP8AddressHost )
{
	gpassertm( g_pDPlayClient, "You can't do network IO if you haven't initialized DPlay." );

	if( m_bEnumInProgress || !g_pDPlayClient )
	{
		return( NE_OK );
	}

	gpgeneric( "NetPipe :: RequestSessionEnumeration - begin\n" );

	m_bEnumInProgress = true;

	////////////////////////////////////////
	//	addresses

	IDirectPlay8Address * pDP8AddressDevice	= NULL;

	TRYDX( CoCreateInstance( 	CLSID_DirectPlay8Address,
								NULL, 
					            CLSCTX_INPROC_SERVER, 
					            IID_IDirectPlay8Address,
								(LPVOID*) &pDP8AddressDevice ) );

	gpassert( pDP8AddressDevice ); 
	TRYDX( pDP8AddressDevice->SetSP( &CLSID_DP8SP_TCPIP ) );

	if( m_SelectedAdapterGuid != GPG_GUID_INVALID )
	{
		TRYDX( pDP8AddressDevice->SetDevice( &m_SelectedAdapterGuid ) );
	}

    if( m_Port )
    {
        TRYDX( pDP8AddressDevice->AddComponent( DPNA_KEY_PORT, 
												&m_Port,
												sizeof(m_Port),
												DPNA_DATATYPE_DWORD ) );
    }

	DPN_APPLICATION_DESC appDesc;
	memset( &appDesc, 0, sizeof( DPN_APPLICATION_DESC ) );
	appDesc.dwSize  = sizeof( DPN_APPLICATION_DESC );
	appDesc.guidApplication = GPG_NetPipe_GUID;

	DPNHANDLE enumHostsHandle = NULL;

	HRESULT result = g_pDPlayClient->EnumHosts( 	&appDesc,
													pDP8AddressHost,
													pDP8AddressDevice,
													NULL,
													0,
													INFINITE,
													0, 
													INFINITE,
													NULL,
													&enumHostsHandle,
													0 );
	SAFE_RELEASE( pDP8AddressDevice );

    // If enumeration succeeds (that is, we started it ok), then someone else has to stop it later
    // But if it fails (due to bogus/unresolvable machine name), we're not enuming anymore.

	bool success = true;

    if( FAILED(result) )
	{
		success = false;
        StopSessionEnumeration();
	}

    if (DPNERR_ADDRESSING != result && DPNERR_INVALIDDEVICEADDRESS != result && DPNERR_INVALIDHOSTADDRESS != result)
    {
    	TRYDX( result );
    }

	m_LastSessionEnumerationTime = GetGlobalSeconds();

	return success ? NE_OK : NE_ERROR;
}


////////////////////////////////////////////////////////////////////////////////
//	
void NetPipe :: StopSessionEnumeration()
{
	if( g_pDPlayClient && m_bEnumInProgress )
	{
		g_pDPlayClient->CancelAsyncOperation(NULL, DPNCANCEL_ENUM);
	    m_bEnumInProgress = false;
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
void NetPipe :: CancelConnect()
{
	if( g_pDPlayClient && m_bConnectInProgress )
	{
		g_pDPlayClient->CancelAsyncOperation(NULL, DPNCANCEL_CONNECT);
	    m_bConnectInProgress = false;
	}
    m_RequestedOpenSession = NULL;
}


////////////////////////////////////////////////////////////////////////////////
//	
NetPipeSessionHandle NetPipe :: GetSessionByLocalId( DWORD id )
{
	kerneltool::Critical::Lock lock( m_SessionCritical );

	NetPipeSessionColl::iterator i;

	for( i = m_Sessions.begin(); i != m_Sessions.end(); ++i )
	{
		if( (*i)->GetLocalId() == id )
		{
			return( NetPipeSessionHandle( this, *i ) );
		}
	}
	return( NetPipeSessionHandle( this, NULL ) );
}


////////////////////////////////////////////////////////////////////////////////
//	
NetPipeSessionHandle NetPipe :: GetSession( gpwstring const & name )
{
	kerneltool::Critical::Lock lock( m_SessionCritical );

	NetPipeSessionColl::iterator i;

	for( i = m_Sessions.begin(); i != m_Sessions.end(); ++i )
	{
		if( (*i)->GetSessionName().same_no_case( name ) )
		{
			return( NetPipeSessionHandle( this, *i ) );
		}
	}
	return( NetPipeSessionHandle( this, NULL ) );
}


////////////////////////////////////////////////////////////////////////////////
//	
NetPipeSessionHandle NetPipe :: GetFrontSession()
{
	if( !m_Sessions.empty() )
	{
		return( NetPipeSessionHandle( this, m_Sessions.front() ) );
	}
	else
	{
		return( NetPipeSessionHandle( this, NULL ) );
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
NetPipeSession * NetPipe :: AddSession( NetPipeSession const & session )
{
	kerneltool::Critical::Lock lock( m_SessionCritical );

	DWORD sessionIndex = 0;

	////////////////////////////////////////
	//	try to find existing record of this session
	
	NetPipeSession * found = NULL;

	for( NetPipeSessionColl::iterator i = m_Sessions.begin(); i != m_Sessions.end(); ++i, ++sessionIndex )
	{
		////////////////////////////////////////
		//	if already have this one, update the record
	
		if( ( (*i)->GetApplicationInstanceGuid() == session.GetApplicationInstanceGuid() ) && ( (*i)->GetMagic() == session.GetMagic() ) )
		{
			*(*i) = session;
			found = *i;
			Notify( NetPipeMessage( NE_SESSION_CHANGED, sessionIndex ) );
			break;
		}
	}

	////////////////////////////////////////
	//	else, make new record and keep it

	if( !found )
	{
		NetPipeSession * pSession = new NetPipeSession();
		*pSession = session;
		m_Sessions.push_back( pSession );
		found = pSession;
		Notify( NetPipeMessage( NE_SESSION_ADDED, sessionIndex ) );
	}

	m_SessionCollDirty = true;
	return( found );
}


////////////////////////////////////////////////////////////////////////////////
//	
void NetPipe :: RemoveSession( GUID instanceGuid )
{
	kerneltool::Critical::Lock lock( m_SessionCritical );

	////////////////////////////////////////
	//	find sesion
	
	NetPipeSession * found = NULL;

	for( NetPipeSessionColl::iterator i = m_Sessions.begin(); i != m_Sessions.end(); )
	{
		////////////////////////////////////////
		//	since DPlay doesn't have session ids - remove all sessions with same instance guid

		if( (*i)->GetApplicationInstanceGuid() == instanceGuid )
		{
			found = *i;
			i = m_Sessions.erase( i );

			////////////////////////////////////////
			//	check open session

			if( m_OpenSession == found )
			{
				SetOpenSession( NULL );
			}

			m_SessionCollDirty = true;
		}
		else
		{
			++i;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
eNetPipeEvent NetPipe :: FindSessionAtIp( gpwstring const & sIpAddress )
{
	gpgenericf(( "NetPipe :: FindSessionAtIp - %S\n", sIpAddress.c_str() ));

	NetPipeSession session;

	StopSessionEnumeration();

	////////////////////////////////////////
	//	set host and device addresses

	//----- host address

	IDirectPlay8Address * pDP8AddressHost	= NULL;

	TRYDX( CoCreateInstance( 	CLSID_DirectPlay8Address,
								NULL, 
			              		CLSCTX_INPROC_SERVER, 
			              		IID_IDirectPlay8Address,
								(LPVOID*) &pDP8AddressHost ) );

	gpassert( pDP8AddressHost );

	TRYDX( pDP8AddressHost->SetSP( &CLSID_DP8SP_TCPIP ) );

    if( m_Port )
    {
        TRYDX( pDP8AddressHost->AddComponent(	DPNA_KEY_PORT, 
												&m_Port,
												sizeof(m_Port),
												DPNA_DATATYPE_DWORD ) );
    }

	TRYDX( pDP8AddressHost->AddComponent(	DPNA_KEY_HOSTNAME, 
	                                       	sIpAddress.c_str(),
	                                       	(sIpAddress.size()+1)*2,
	                                        DPNA_DATATYPE_STRING ) );

	ClearSessions();

	gpassert( pDP8AddressHost );

	eNetPipeEvent result = RequestSessionEnumeration( pDP8AddressHost );

	SAFE_RELEASE( pDP8AddressHost );

	return result;
}


////////////////////////////////////////////////////////////////////////////////
//	
eNetPipeEvent NetPipe :: ConnectTo( NetPipeSessionHandle & session, gpwstring const & password )
{
	if( !session )
	{
		gperror( "You can't connect to a NULL session!" );
		return NE_ERROR;
	}

	if( HasOpenSession() )
	{
		gpnet( "NetPipe :: ConnectTo - already have open session.  Request ignored.\n" );
		return NE_ERROR;
	}

	if( !session->IsExecutableCompatible() )
	{
		gperror( "You can't connect to a session of incompatible executable version!" );
		return NE_ERROR;
	}

	m_bConnectInProgress = true;

	gpassertm( g_pDPlayClient, "You can't do network IO if you haven't initialized DPlay." );

	////////////////////////////////////////
	//	DPN_APPLICATION_DESC

	DPN_APPLICATION_DESC appDesc;
	memset( &appDesc, 0, sizeof( DPN_APPLICATION_DESC ) );
	appDesc.dwSize = sizeof( DPN_APPLICATION_DESC );

	//appDesc.pwszSessionName 	= name.c_str();		// not needed ?
	appDesc.guidApplication		= GPG_NetPipe_GUID;
	appDesc.guidInstance		= session->GetApplicationInstanceGuid();
	appDesc.pwszPassword		= password.empty() ? NULL : const_cast< WCHAR* >( password.c_str() );

	////////////////////////////////////////
	//	addresses

	IDirectPlay8Address * pHostAddress		= NULL;
	IDirectPlay8Address * pDeviceAddress  	= NULL;
	
	TRYDX( CoCreateInstance( 	CLSID_DirectPlay8Address,
								NULL, 
								CLSCTX_INPROC_SERVER, 
								IID_IDirectPlay8Address,
								(void **) &pHostAddress ) );

	if( !session->GetHostAddress().empty() )
	{
		pHostAddress->BuildFromURLW( const_cast<WCHAR*>(session->GetHostAddress().c_str() ) );
		gpgenericf(( "DP host address url = %S\n", session->GetHostAddress().c_str() ));
	}


	TRYDX( CoCreateInstance( 	CLSID_DirectPlay8Address,
								NULL, 
								CLSCTX_INPROC_SERVER,
	                        	IID_IDirectPlay8Address,
	                        	(void**) &pDeviceAddress ) );

	if( !session->GetDeviceAddress().empty() )
	{
		pDeviceAddress->BuildFromURLW( const_cast<WCHAR*>(session->GetDeviceAddress().c_str() ) );
		gpgenericf(( "DP device address url = %S\n", session->GetDeviceAddress().c_str() ));
	}

	{
		kerneltool::Critical::Lock sessionLock( m_SessionCritical );
		SetOpenSession( NULL );
	}

	m_RequestedOpenSession = const_cast<NetPipeSession*>(session.GetPointer());

	m_UserConnectData.clear();
	NetPipeSession::Info localInfo;

	Store( localInfo, m_UserConnectData );
	Store( m_LocalContentColl, m_UserConnectData );


	DPNHANDLE hAsyncHandle;
	// connect
	HRESULT result = g_pDPlayClient->Connect(	&appDesc,
												pHostAddress,
												pDeviceAddress,
												NULL,
												NULL,
												m_UserConnectData.empty() ? 0 : &m_UserConnectData[0],
												m_UserConnectData.size(),
												NULL,
												&hAsyncHandle,
												0 );
	SAFE_RELEASE( pHostAddress );
	SAFE_RELEASE( pDeviceAddress );

	gpgenericf(( "NetPipe :: ConnectTo - connect initiated to session %S\n", session->GetSessionName().c_str() ));

	
	return SUCCEEDED( result ) ? NE_OK : NE_ERROR;
}


////////////////////////////////////////////////////////////////////////////////
//	
bool NetPipe :: Host(	gpwstring const & name,
						gpwstring const & password,
						DWORD const maxMachines )
{
	kerneltool::Critical::Lock sessionLock( m_SessionCritical );

	gpnet("NetPipe :: Host called\n");

	HRESULT result	= S_OK;
	bool success	= false;


	if( !g_pDPlayServer )
	{
		InitServer();
	}

	if( !g_pDPlayServer )
	{
		gperror( "You can't do network IO if you haven't initialized DPlay." );
		return false;
	}

    StopSessionEnumeration();

	ClearSessions();

	//----- device address


	IDirectPlay8Address * pDP8AddressDevice	= NULL;

	TRYDX( CoCreateInstance( 	CLSID_DirectPlay8Address,
								NULL, 
			              		CLSCTX_INPROC_SERVER, 
			              		IID_IDirectPlay8Address,
								(LPVOID*) &pDP8AddressDevice ) );
	
	gpassert( pDP8AddressDevice ); 

	TRYDX( pDP8AddressDevice->SetSP( &CLSID_DP8SP_TCPIP ) );

	if( m_SelectedAdapterGuid != GPG_GUID_INVALID )
	{
		TRYDX( pDP8AddressDevice->SetDevice( &m_SelectedAdapterGuid ) );
	}

    if( m_Port )
    {
        TRYDX( pDP8AddressDevice->AddComponent( DPNA_KEY_PORT, 
												&m_Port,
												sizeof(m_Port),
												DPNA_DATATYPE_DWORD ) );
    }

	////////////////////////////////////////
	//	create session with DP

	DPN_APPLICATION_DESC appDesc;
	memset( &appDesc, 0, sizeof( DPN_APPLICATION_DESC ) );
	appDesc.dwSize = sizeof( DPN_APPLICATION_DESC );

	appDesc.pwszSessionName	= const_cast<WCHAR*>(name.c_str());
	appDesc.guidApplication	= GPG_NetPipe_GUID;
	appDesc.pwszPassword	= password.empty() ? NULL : const_cast<WCHAR*>(password.c_str());
	appDesc.dwMaxPlayers	= maxMachines;
	appDesc.dwFlags			= DPNSESSION_CLIENT_SERVER 
								| ( appDesc.pwszPassword ? DPNSESSION_REQUIREPASSWORD : 0 )
								| ( gNetPipe.GetDPNSVR() ? 0 : DPNSESSION_NODPNSVR );

	NetPipeSession::Info info;
	appDesc.pvApplicationReservedData		= &info;
	appDesc.dwApplicationReservedDataSize	= sizeof( NetPipeSession::Info );

	NetPipeSession session;
	session.SetSessionName( name );
	session.SetApplicationGuid( GPG_NetPipe_GUID );
	session.SetApplicationInstanceGuid( GPG_NetPipe_GUID );
	session.SetNumMachinesMax( maxMachines );
	session.SetIsLocked( true );
	session.SetIsHost( false );
	session.SetIsExecutableCompatible( true );
	session.SetIsContentCompatible( true );
	session.SetSessionPassword( password );
	session.SetHasPassword( password.empty() ? false : true );

	m_RequestedOpenSession = AddSession( session );

	result = g_pDPlayServer->Host(	&appDesc,
									&pDP8AddressDevice,
									1,
									NULL,
									NULL,
									NULL,
									0 );
	success = CHECKDX( result );

	SAFE_RELEASE( pDP8AddressDevice );

#if 1

	////////////////////////////////////////
	//	create our own app session object for tracking

	if( success )	// $$ session creation fails sometimes even though we have a success code!?
	{
		SetOpenSession( m_RequestedOpenSession );
		gpassert( m_OpenSession );
		gpassert( m_OpenSession->IsLocked() );
		gpassert( m_OpenSession->IsExecutableCompatible() );
		gpassert( m_OpenSession->IsContentCompatible() );

		m_OpenSession->SetIsHost( true );
		m_OpenSession->SetApplicationInstanceGuid( appDesc.guidInstance );

		// retrieve host addresses
		DWORD numHostAddresses = 0;

		HRESULT hr = g_pDPlayServer->GetLocalHostAddresses( NULL, &numHostAddresses, 0 );

		if (SUCCEEDED(hr))
		{
			if ( numHostAddresses > 0 )
			{
				IDirectPlay8Address ** pHostAddresses = new IDirectPlay8Address *[ numHostAddresses ];
				HRESULT result = g_pDPlayServer->GetLocalHostAddresses( pHostAddresses, &numHostAddresses, 0 );

				if ( result == S_OK )
				{
					for ( DWORD i = 0; i < numHostAddresses; ++i )
					{
						gpwstring wsAddress;

						DWORD stringSize = 0;
						pHostAddresses[ i ]->GetURLW( const_cast<WCHAR*>(wsAddress.c_str()), &stringSize );
						gpassert( stringSize );
						wsAddress.resize( stringSize );
						pHostAddresses[ i ]->GetURLW( const_cast<WCHAR*>(wsAddress.c_str()), &stringSize );
						gpassert( wsAddress.size() == stringSize );

						// $$$ take into account multiple host ip's
						m_OpenSession->SetHostAddress( wsAddress );
					}
				}
				delete [] pHostAddresses;
			}
		}
	}
	else
	{
		ClearSessions();
	}

	m_RequestedOpenSession = NULL;

	gpgenericf(( "NetPipe :: Host - session %S = %s\n", session.GetSessionName().c_str(), success ? "OK" : "FAILED" ));

#else

//	NetPipeSession session;
	session.SetSessionName( L"dummy" );
	session.SetApplicationGuid( GPG_NetPipe_GUID );
	session.SetApplicationInstanceGuid( GPG_NetPipe_GUID );
	session.SetNumMachinesMax( maxMachines );
	session.SetIsLocked( true );
	session.SetIsExecutableCompatible( true );
	session.SetIsContentCompatible( true );
	session.SetSessionPassword( password );
	session.SetHasPassword( password.empty() ? false : true );

	m_RequestedOpenSession = AddSession( session );

	success = true;

	SetOpenSession( m_RequestedOpenSession );

	m_OpenSession->SetIsHost( true );

	m_RequestedOpenSession = NULL;

#endif

	if( success )
	{
		Notify( NetPipeMessage( NE_SESSION_CREATED ) );
	}

	return CHECKDX( result );
}


////////////////////////////////////////////////////////////////////////////////
//	
bool NetPipe :: DisconnectMachine( DWORD MachineId, eNetPipeEvent event )
{
	NetPipeSessionHandle session = GetOpenSessionHandle();

	if( session )
	{
		gpassert( g_pDPlayServer );
		gpassert( FuBi::IsServerLocal() );
		gpassert( session->HasMachine( MachineId ) );
		gpassertm( session->IsHost(), "Only the host can disconnect machines." );

		gpgenericf((	"NetPipe :: DisconnectMachine - name = %s, Machine id = 0x%08x\n",
					session->HasMachine( MachineId ) ? ToAnsi( session->GetMachine( MachineId )->m_wsName ).c_str() : "not present", MachineId ));
	}

	// #11509
	ReturnReceivePackets( MachineId );

	m_NetPipeMessageCallback( NetPipeMessage( NE_MACHINE_DISCONNECTED, MachineId ) );

	if( session )
	{
		session->RemoveMachine( MachineId );

		HRESULT hr = g_pDPlayServer->DestroyClient( MachineId, &event, 4, 0 );
		if( hr != DPNERR_CONNECTIONLOST )
		{
			TRYDX( hr );
		}
	}

	return( true );
}


////////////////////////////////////////////////////////////////////////////////
//	
NetPipeSessionHandle NetPipe :: GetOpenSessionHandle()
{
	kerneltool::Critical::Lock sessionLock( m_SessionCritical );

	return NetPipeSessionHandle( this, m_OpenSession );
}


////////////////////////////////////////////////////////////////////////////////
//	
bool NetPipe :: CloseOpenSession( bool sendEvent )
{
	gpgeneric( "NetPipe :: CloseOpenSession()\n" );

	gpassertm( g_pDPlayClient, "You can't do network IO if you haven't initialized DPlay." );

	kerneltool::Critical::Lock sessionLock( m_SessionCritical );

	if( m_OpenSession )
	{
		bool amHost = m_OpenSession->IsHost();

		SetOpenSession( NULL );

		// closes previously opened session
		if( amHost && g_pDPlayServer )
		{
			g_pDPlayServer->Close( 0 );
		}

		// DPlay note - there doesn't appear to be a way to locally disconnect from a session if you're not the host
		//				the host can boot you, or close the session, but you can't do either.  So, if I'm a client,
		//				I'm just going to pretend that the session was terminated.

		if( sendEvent )
		{
			m_NetPipeMessageCallback( NetPipeMessage( NE_SESSION_TERMINATED ) );
		}
	}

	Shutdown();

	return( true );
}


////////////////////////////////////////////////////////////////////////////////
//	session content compatability info - only valid for current open session


void NetPipe :: ClearLocalContentResourceColl()
{
	for( AutoContentResourceColl::iterator i = m_LocalContentColl.begin(); i != m_LocalContentColl.end(); ++i )
	{
		delete (*i);
	}
	m_LocalContentColl.clear();
}


void NetPipe :: ClearServerRequiredResourceColl()
{
	for( AutoContentResourceColl::iterator i = m_ServerRequiredContentColl.begin(); i != m_ServerRequiredContentColl.end(); ++i )
	{
		delete (*i);
	}

	m_ServerRequiredContentColl.clear();
}


void NetPipe :: AddLocalContentResource( GUID const & guid, gpstring const & filename )
{
	ContentResource * resource = new ContentResource;
	resource->m_Guid		= guid;
	resource->m_Filename	= filename;

	m_LocalContentColl.push_back( resource );
}


void NetPipe :: Store( AutoContentResourceColl const & coll, ByteBuffer & buffer )
{
	gpassert( !coll.empty() );

	Store( coll.size(), buffer );

	for( AutoContentResourceColl::const_iterator i = coll.begin(); i != coll.end(); ++i )
	{
		Store( (*i)->m_Guid, buffer );
		DWORD stringSize = (*i)->m_Filename.size();

		Store( stringSize, buffer );
		Store( (*i)->m_Filename.c_str(), (*i)->m_Filename.size(), buffer );
	}
}


void NetPipe :: Read( ByteBuffer const & buffer, AutoContentResourceColl & coll, DWORD readPoint )
{
	DWORD readCount = 0;

	ReadCopy( readCount, buffer, readPoint );

	for( DWORD i = 0; i != readCount; ++i )
	{
		ContentResource * resource = new ContentResource;

		ReadCopy( resource->m_Guid, buffer, readPoint );

		DWORD stringSize = 0;

		ReadCopy( stringSize, buffer, readPoint );

		resource->m_Filename.assign( (CHAR*)( &buffer[readPoint] ), stringSize );

		readPoint += stringSize;

		coll.push_back( resource );
	}
}


bool NetPipe :: Contains( AutoContentResourceColl const & coll, GUID const & guid )
{
	for( AutoContentResourceColl::const_iterator i = coll.begin(); i != coll.end(); ++i )
	{
		if( (*i)->m_Guid == guid )
		{
			return true;
		}
	}
	return false;
}


bool NetPipe :: Equal( AutoContentResourceColl const & l, AutoContentResourceColl const & r )
{
	if( l.size() != r.size() )
	{
		return false;
	}

	for( AutoContentResourceColl::const_iterator i = l.begin(); i != l.end(); ++i )
	{
		if( !Contains( r, (*i)->m_Guid ) )
		{
			return false;
		}
	}
	return true;
}


NetPipe::ByteBuffer * NetPipe :: MakeTempBuffer()
{
	NetPipe::ByteBuffer * buffer = new ByteBuffer;
	gpverify( m_TempBufferMap.insert( std::make_pair( buffer, buffer ) ).second );
	return buffer;
}


void NetPipe :: FreeTempBuffer( ByteBuffer * buffer )
{
	gpassert( buffer );

	ByteBufferMap::iterator i = m_TempBufferMap.find( buffer );
	if( i != m_TempBufferMap.end() )
	{
		delete (*i).second;
		m_TempBufferMap.erase( i );
	}
	else
	{
		gpassertm( 0, "was told to release an unknown buffer" );
	}
}


////////////////////////////////////////////////////////////////////////////////
//	caller of this method must have and keep all the criticals while processing
//	receive buffers
//
bool NetPipe :: FindFirstReceivePacket()
{
	// stuff the new packets into the db

	PacketColl newpackets;

	{
		kerneltool::Critical::Lock lock( g_IncomingRawCritical );
		newpackets.swap( g_IncomingRaw );
	}

	NetPipeSessionHandle session = GetOpenSessionHandle();

	// assume receive critical is grabbed by caller

	for( PacketColl::iterator in = newpackets.begin(); in != newpackets.end(); ++in )
	{
		MachineToPacketMap::iterator im = g_IncomingPackets.find( (*in)->m_From );
		if( im == g_IncomingPackets.end() )
		{
			im = g_IncomingPackets.insert( std::make_pair( (*in)->m_From, new PacketColl ) ).first;
		}

		(*im).second->push_back( (*in) );

		if( !session )
		{
			gpnet(( "NET HAZZARD: DPlay sending us messages when there is no open session.\n" ));
		}

		if( session && !session->HasMachine( (*in)->m_From ) )
		{
			gpnetf(( "NET HAZZARD: Received DPN_MSGID_RECEIVE - from machine 0x%08x and the session doesn't know about this machine/player\n", (*in)->m_From ));
		}

		if( session && session->HasMachine( (*in)->m_From ) )
		{
			session->GetMachine( (*in)->m_From )->m_LastReceiveTime = GetGlobalSeconds();
		}
	}

	// process packet db

	if( !session )
	{
		gpnet( "FindFirstReceiveBuffer: no open session - will not read incoming packets\n" );
		g_IncomingMachineIterator	= g_IncomingPackets.end();
		return( false );
	}

	if( g_IncomingPackets.empty() )
	{
//		gpnet( "FindFirstReceiveBuffer: - no incoming packets found\n" );
		g_IncomingMachineIterator	= g_IncomingPackets.end();
		return( false );
	}
	else
	{
		MachineToPacketMap::iterator im;
/*
#if !GP_RETAIL
		gpnet( "FindFirstReceiveBuffer:\n" );
		for( im = g_IncomingPackets.begin(); im != g_IncomingPackets.end(); ++im )
		{
			gpnetf((	"machine id 0x%08x, packet colls = %d, has machine = %s\n",
						(*im).first,
						(*im).second->size(),
						session->HasMachine( (*im).first ) ? "TRUE" : "FALSE" ));
		}
#endif
*/
		for( im = g_IncomingPackets.begin(); im != g_IncomingPackets.end(); ++im )
		{
			if( session->HasMachine( (*im).first ) && !(*im).second->empty() )
			{
				g_IncomingMachineIterator	= im;
				g_IncomingPacketIterator	= (*im).second->begin();
				return( true );
			}
		}
	}

	g_IncomingMachineIterator = g_IncomingPackets.end();
	return false;
}


////////////////////////////////////////////////////////////////////////////////
//	
bool NetPipe :: GetReceivePacket( DWORD & machineId, BYTE *& buffer, DWORD & size )
{
	GPPROFILERSAMPLE( "NetPipe :: GetReceiveBuffer", SP_NET | SP_NET_RECEIVE );
/*
	gpnetf((	"GetReceiveBuffer for machine 0x%08x - msgcount=%d current=%d\n",
				(*g_IncomingMachineIterator).first,
				(*g_IncomingMachineIterator).second->size(), g_IncomingPacketIterator - (*g_IncomingMachineIterator).second->begin() ));
*/
	for( ;; )
	{
		if( g_IncomingMachineIterator == g_IncomingPackets.end() )
		{
			return false;
		}

		if( g_IncomingPacketIterator == (*g_IncomingMachineIterator).second->end() )
		{
			++g_IncomingMachineIterator;
			if( g_IncomingMachineIterator == g_IncomingPackets.end() )
			{
				return false;
			}
			else
			{
				g_IncomingPacketIterator = (*g_IncomingMachineIterator).second->begin();
				if( g_IncomingPacketIterator == (*g_IncomingMachineIterator).second->end() )
				{
					return false;
				}
			}
		}

		gpassert( g_IncomingPacketIterator != (*g_IncomingMachineIterator).second->end() );

		m_BytesReceived += (*g_IncomingPacketIterator)->m_Size;

		///////////////////////////////////////////////////////////////
		// error checking - make sure we know about this machine...

		machineId = (*g_IncomingMachineIterator).first;

		NetPipeSessionHandle session = GetOpenSessionHandle();	// gets session critical

		if( session->HasMachine( machineId ) )
		{
			NetPipeMachine * pmachine = session->GetMachine( machineId );
			pmachine->m_BytesReceived += (*g_IncomingPacketIterator)->m_Size;

			gpassert( !(*g_IncomingPacketIterator)->m_MarkedForDeletion );

			m_BytesRcvdComp += (*g_IncomingPacketIterator)->m_Size;

			buffer = (*g_IncomingPacketIterator)->m_pBuffer;
			size   = (*g_IncomingPacketIterator)->m_Size;

			//checking for corrupt packets
			gpnetf((" Compressed Receive Pre-Inflate first byte=0x%x machine 0x%08X\n", buffer[0], machineId ));

			if( IsCompressed() )
			{
				PacketCollHeader * packetCollHeader = (PacketCollHeader *)buffer;
				gpassert( packetCollHeader->m_Type == PHT_PACKET_COLL );

				DWORD uncompressBufferSize		= packetCollHeader->m_Size + 1024;

				BYTE * compressedBodyData		= (BYTE*)(packetCollHeader+1);
				DWORD compressedBodyDataSize	= size - sizeof( PacketCollHeader );

				BYTE * uncompressBuffer = new BYTE[ uncompressBufferSize ];
				memcpy( uncompressBuffer, packetCollHeader, sizeof( PacketCollHeader ) );

				BYTE * uncompressBufferBody		= uncompressBuffer + sizeof( PacketCollHeader );
				DWORD uncompressBufferBodySize	= uncompressBufferSize - sizeof( PacketCollHeader );

				// we want to decompress in one chunk
		        DWORD sizeUncompressed = pmachine->m_decompress.InflateChunk(	mem_ptr( uncompressBufferBody, uncompressBufferBodySize ), 
																				const_mem_ptr( compressedBodyData, compressedBodyDataSize ), NULL );
				gpassert( sizeUncompressed == packetCollHeader->m_Size - sizeof( PacketCollHeader ) );

				if( sizeUncompressed > 0 )
				{
					DWORD totalSizeUncompressed = sizeUncompressed + sizeof( PacketCollHeader );

					gpnetf(( "Received from 0x%08X, %d/%d = %2.0f%% savings\n", machineId, size, totalSizeUncompressed,
		        				100.0f * ( float( totalSizeUncompressed )  - float((*g_IncomingPacketIterator)->m_Size )) / float( totalSizeUncompressed ) ));

					size   = totalSizeUncompressed;
					buffer = uncompressBuffer;

					m_BytesRcvdUnComp += size;
				}
				else
				{
					gperror( "Failed to inflate packet chunk." );
				}
			}

			(*g_IncomingPacketIterator)->m_MarkedForDeletion = true;

			++g_IncomingPacketIterator;
			return true;
		}
		else
		{
			gpgenericf(( "NetPipe :: GetReceiveBuffer - skipping packet from a machine we don't know about. machineId = 0x%08x\n", machineId ));
			++g_IncomingMachineIterator;
			continue;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
void NetPipe :: ReturnAllReceivePackets()
{
	GPPROFILERSAMPLE( "NetPipe :: ReturnAllReceivePackets", SP_NET | SP_NET_RECEIVE );

	kerneltool::Critical::Lock lock( GetReceiveCritical() );

	FindFirstReceivePacket();

	for( MachineToPacketMap::iterator im = g_IncomingPackets.begin(); im != g_IncomingPackets.end(); ++im )
	{
		for( PacketColl::iterator ip = (*im).second->begin(); ip != (*im).second->end(); ++ip )
		{
			if( (*ip)->m_ReceivedBy == IncomingPacket::RB_SERVER )
			{
				if( g_pDPlayServer )
				{
					HRESULT hr = g_pDPlayServer->ReturnBuffer( (*ip)->m_hBufferHandle, 0 );
					if( hr != DPNERR_NOCONNECTION ) // #5825 - allowed error
					{
						TRYDX( hr );
					}
				}
				delete (*ip);
			}
			else if( (*ip)->m_ReceivedBy == IncomingPacket::RB_CLIENT )
			{
				if( g_pDPlayClient )
				{
					HRESULT hr = g_pDPlayClient->ReturnBuffer( (*ip)->m_hBufferHandle, 0 );
					if( hr != DPNERR_NOCONNECTION ) // #5825 - allowed error
					{
						TRYDX( hr );
					}
				}
				delete (*ip);
			}
			else
			{
				gpassertm( 0, "bad flow" );
			}
		}

		delete (*im).second;
	}

	g_IncomingPackets.clear();

	FindFirstReceivePacket();
}


////////////////////////////////////////////////////////////////////////////////
//	
void NetPipe :: ReturnProcessedReceivePackets()
{
	GPPROFILERSAMPLE( "NetPipe :: ReturnProcessedReceivePackets", SP_NET | SP_NET_RECEIVE );

	kerneltool::Critical::Lock lock( GetReceiveCritical() );

	for( MachineToPacketMap::iterator im = g_IncomingPackets.begin(); im != g_IncomingPackets.end(); ++im )
	{
		for( PacketColl::iterator ip = (*im).second->begin(); ip != (*im).second->end(); )
		{
			if( (*ip)->m_MarkedForDeletion )
			{
				if( (*ip)->m_ReceivedBy == IncomingPacket::RB_SERVER )
				{
					if( g_pDPlayServer )
					{
						HRESULT hr = g_pDPlayServer->ReturnBuffer( (*ip)->m_hBufferHandle, 0 );
						if( hr != DPNERR_NOCONNECTION ) // #5825 - allowed error
						{
							TRYDX( hr );
						}
					}
					delete (*ip);
				}
				else if( (*ip)->m_ReceivedBy == IncomingPacket::RB_CLIENT )
				{
					if( g_pDPlayClient )
					{
						HRESULT hr = g_pDPlayClient->ReturnBuffer( (*ip)->m_hBufferHandle, 0 );
						if( hr != DPNERR_NOCONNECTION ) // #5825 - allowed error
						{
							TRYDX( hr );
						}
					}
					delete (*ip);
				}
				else
				{
					gpassertm( 0, "bad flow" );
				}

				ip = (*im).second->erase( ip );
			}
			else
			{
				++ip;
			}
		}
	}

	FindFirstReceivePacket();
}


////////////////////////////////////////////////////////////////////////////////
//	
void NetPipe :: ReturnReceivePackets( DWORD machineId )
{
	GPPROFILERSAMPLE( "NetPipe :: ReturnReceivePackets", SP_NET | SP_NET_RECEIVE );

	kerneltool::Critical::Lock lock( GetReceiveCritical() );

	MachineToPacketMap::iterator im = g_IncomingPackets.find( machineId );

	if( im != g_IncomingPackets.end() )
	{
		for( PacketColl::iterator ip = (*im).second->begin(); ip != (*im).second->end(); ++ip )
		{
			if( g_pDPlayServer || g_pDPlayClient )
			{
				if( (*ip)->m_ReceivedBy == IncomingPacket::RB_SERVER )
				{
					if( g_pDPlayServer )
					{
						HRESULT hr = g_pDPlayServer->ReturnBuffer( (*ip)->m_hBufferHandle, 0 );
						if( hr != DPNERR_NOCONNECTION ) // #5825 - allowed error
						{
							TRYDX( hr );
						}
					}
				}
				else if( (*ip)->m_ReceivedBy == IncomingPacket::RB_CLIENT )
				{
					if( g_pDPlayClient )
					{
						HRESULT hr = g_pDPlayClient->ReturnBuffer( (*ip)->m_hBufferHandle, 0 );
						if( hr != DPNERR_NOCONNECTION ) // #5825 - allowed error
						{
							TRYDX( hr );
						}
					}
				}
				else
				{
					gpassertm( 0, "bad flow" );
				}
			}
			delete (*ip);
		}
		(*im).second->clear();
	}

	FindFirstReceivePacket();
}


////////////////////////////////////////////////////////////////////////////////
//	
float NetPipe :: GetOutboundTrafficBackupDuration()
{
	return float( GetGlobalSeconds() - m_LastTimeOutboundTrafficOK );
}


////////////////////////////////////////////////////////////////////////////////
//	
bool NetPipe :: Send( DWORD toMachineId, unsigned char * data, unsigned int size, bool guarantee )
{
	GPPROFILERSAMPLE( "NetPipe :: Send", SP_NET | SP_NET_SEND );

	////////////////////////////////////////
	//	validate

	gpassert( data && size );

	gpassert( g_pDPlayServer || g_pDPlayClient );
	gpassert( ( !g_pDPlayServer && g_pDPlayClient ) || ( g_pDPlayServer && g_pDPlayClient ) );

	NetPipeSessionHandle session = GetOpenSessionHandle();
	if( !session )
	{
		gpassertm( 0, "Attempting to send with no open session" );
	}

	gpassert( !session->IsHost() || ( session->IsHost() && toMachineId != RPC_TO_SERVER ) );

	if( (toMachineId != RPC_TO_ALL) && (toMachineId != RPC_TO_OTHERS) && !session->HasMachine( toMachineId ) )
	{
		gpassertm( 0, "Trying to send to a machine that isn't in the session." );
		return( false );
	}

	////////////////////////////////////////
	//	send

    // we can't do broadcast sends due to individual compression streams
    // this involves extra iteration in the case of directed sends, but we don't have many (any) of those, so this saves code size

	gpassert( toMachineId != RPC_TO_ALL );

	for( NetPipeMachineColl::const_iterator ic = session->GetMachinesBegin(); ic != session->GetMachinesEnd(); ++ic )
	{
		if( ((*ic)->m_Id == toMachineId) )
		{
			PacketCollHeader * packetCollHeader = (PacketCollHeader *)data;	

			NetPipeMachine * pmachine = *ic;
			DWORD machineid = pmachine->m_Id;
			gpassert(pmachine); 

			BYTE * dataNew		= data;
			DWORD sizeNew		= size;
			m_BytesSentUnComp	+= sizeNew;

			BYTE * compressBuffer = NULL;

			// compress if we're in compressed mode
			DPN_BUFFER_DESC buffDesc;

			if( gNetPipe.IsCompressed() && packetCollHeader->m_Type == PHT_PACKET_COLL )
			{
				GPPROFILERSAMPLE( "NetPipe :: Send - compress", SP_NET | SP_NET_SEND );

				gpassert( packetCollHeader->m_Size == size );
				BYTE * uncompressedDataBody		= (BYTE*)(packetCollHeader+1);
				DWORD uncompressedDataBodySize	= size - sizeof( PacketCollHeader );

				compressBuffer = new BYTE[ uncompressedDataBodySize + 1024 ];	// don't bother with clever compression buffer size heuristics
				memcpy( compressBuffer, data, sizeof( PacketCollHeader ) );

				DWORD compressBufferSize	= ( uncompressedDataBodySize + 1024 ) - sizeof( PacketCollHeader );
				BYTE * compressBufferBody	= compressBuffer + sizeof( PacketCollHeader );

				// we want to compress in one chunk
				DWORD compressedDataSize = pmachine->m_compress.DeflateChunk(	mem_ptr ( compressBufferBody, compressBufferSize ),
																				const_mem_ptr( uncompressedDataBody, uncompressedDataBodySize ), false, NULL );

				const DWORD totalCompressedPacketSize = compressedDataSize + sizeof( PacketCollHeader );

				gpnetf((	"Sending to 0x%08X, %d/%d = %2.0f%% savings, address %S\n", 
							machineid, 
							totalCompressedPacketSize,
							sizeNew,
		        			100.0f * ( float( size ) - float( totalCompressedPacketSize ) ) / float( size ),
							pmachine->m_wsAddress.c_str() ));

				sizeNew			= totalCompressedPacketSize;
		        dataNew			= compressBuffer;

				m_BytesSentComp += totalCompressedPacketSize;

				if( totalCompressedPacketSize > size )
				{
			        gpnet( "**NEGATIVE COMPRESSION**\n" );
				}
			}

			if( sizeNew > MAX_PACKET_SIZE )
			{
				gpwarningf(( "sending unusually large packet.  size = %d bytes\n", size ));
			}

			//Add debug header

#if !GP_RETAIL

			gpnetf(( "Sending packet id %d\n", pmachine->m_lastPacketId ));
			DWORD cbToSend			= sizeNew+4;
			UCHAR * pchToSend		= (UCHAR *) _alloca(cbToSend);
			(*(DWORD*)pchToSend)	= pmachine->m_lastPacketId++;

			CopyMemory((pchToSend+4),dataNew,sizeNew);

			sizeNew = cbToSend;
			dataNew = pchToSend;
#endif
			//back to standard send
			buffDesc.dwBufferSize = sizeNew;
			buffDesc.pBufferData  = dataNew;

			DPNHANDLE asyncHandle = NULL;

			DWORD flags = /* DPNSEND_NONSEQUENTIAL | */ DPNSEND_NOLOOPBACK;
			flags |= guarantee ? DPNSEND_GUARANTEED  : 0;

			(*ic)->m_LastSendTime = GetGlobalSeconds();

			HRESULT hr = g_pDPlayServer ? g_pDPlayServer->SendTo( 	machineid,
																	&buffDesc,
																	1,
																	0,
																	NULL,
																	&asyncHandle,
																	flags )

			:							g_pDPlayClient->Send(		&buffDesc,
																	1,
																	0,
																	NULL,
																	&asyncHandle,
																	flags );
			if( compressBuffer )
			{
				Delete( compressBuffer );
			}

			m_BytesSent				+= sizeNew;
			pmachine->m_BytesSent	+= sizeNew;

			//	$ sometimes we loose a connection right on send... and don't get an event from DPlay beforehand, so don't report this as an error...
			//	$ sometimes we may also loose a player before we've had a chance to process the loss
			if( (hr != DPNERR_CONNECTIONLOST) && (hr != DPNERR_INVALIDPLAYER) && ( hr != DPNERR_NOCONNECTION ) )
			{
				CHECKDX( hr );
			}
		}
	}
	return true; 
}


////////////////////////////////////////////////////////////////////////////////
//	
NetPipeSessionHandleColl NetPipe :: GetSessions()
{ 
    // Let's prune any dead wood
	kerneltool::Critical::Lock lock( m_SessionCritical );

    bool fContinue = true;

    while (fContinue)
    {
    	for( NetPipeSessionColl::iterator i = m_Sessions.begin(); i != m_Sessions.end(); ++i )
    	{
			if( (*i) != m_RequestedOpenSession && (*i) != m_OpenSession )
			{
    			float dtime = (*i)->GetTimeElapsedSinceLastUpdate();
				if (dtime > 10.0f)
				{
					RemoveSession((*i)->GetApplicationInstanceGuid());
					break; // iterator is invalid--start over
				}
			}
    	}
    	fContinue = false;
    }

	NetPipeSessionHandleColl tempColl;
    for( NetPipeSessionColl::iterator j = m_Sessions.begin(); j != m_Sessions.end(); ++j )
	{
		tempColl.push_back( NetPipeSessionHandle( this, *j ) );
	}

	return tempColl; 
}


void NetPipe :: ConnectComplete( eNetPipeEvent hr )
{
	kerneltool::Critical::Lock sessionLock( m_SessionCritical );

	gpnet( "NetPipe :: ConnectComplete\n" );

	gpassert( m_RequestedOpenSession );
	SetOpenSession( hr == NE_OK ? m_RequestedOpenSession : NULL );
	gpassert( m_OpenSession );
	Notify( NetPipeMessage( NE_SESSION_CONNECTED, (DWORD) hr) );
	m_RequestedOpenSession = NULL;
	m_bConnectInProgress = false;

    StopSessionEnumeration();
}


////////////////////////////////////////////////////////////////////////////////
//	
//	SERVER message handler
//
//
HRESULT WINAPI DPServerMessageHandler(	PVOID /*pvUserContext*/,
                                        DWORD dwMessageId,
                                        PVOID pMsgBuffer )
{

	GPPROFILERSAMPLE( "NetPipe :: DirectPlayMessageHandler", SP_NET | SP_NET_RECEIVE );

	HRESULT result = S_OK;

#if !GP_DISABLE_MULTIPLAYER

	if( !g_pDPlayServer )
	{
		gpwarning( "You can't do network IO if you haven't initialized DPlay." );
		return DPNERR_NOTALLOWED;
	}

	if( !NetPipe::DoesSingletonExist() || !gNetPipe.IsInitialized() )
	{
		return DPNERR_NOTALLOWED;
	}

	gplog( OutputDirectPlayMessage( dwMessageId ) );

	gNetLog.OutputDirectPlayMessage( dwMessageId );

	switch( dwMessageId )
	{
		case DPN_MSGID_CREATE_PLAYER:
		{
			kerneltool::Critical::Lock lock( gNetPipe.GetSessionCritical() );

			PDPNMSG_CREATE_PLAYER pCreatePlayerMsg = NULL;
			pCreatePlayerMsg = (PDPNMSG_CREATE_PLAYER)pMsgBuffer;

			gpgenericf(( "Received DPN_MSGID_CREATE_PLAYER - player id 0x%08x\n", pCreatePlayerMsg->dpnidPlayer ));

			gpassert( IsValidIncomingMachineId( pCreatePlayerMsg->dpnidPlayer ) );

			// DPlay will issue create players before connect complete in the
			// case of successful connection 

			{
				NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
				if( !session && gNetPipe.IsConnectInProgress() )
				{
					gNetPipe.ConnectComplete( NE_OK );
				}
			}

			NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

			if( session && session->IsLocked() )
			{
				result = DPNERR_HOSTREJECTEDCONNECTION;
				break;
			}

			////////////////////////////////////////
			//	validate

			if( session && session->HasMachine( pCreatePlayerMsg->dpnidPlayer ) )
			{
				gpgeneric( "NetPipe :: DPHandleSystemMessage - I was told to create a connection to a machine already in the session.\n" );
				break;
			}

			////////////////////////////////////////
			//	get client info

			DWORD dwSize = 0;
			DPN_PLAYER_INFO * pdpClientInfo = NULL;

			if( DPNERR_BUFFERTOOSMALL != g_pDPlayServer->GetClientInfo( pCreatePlayerMsg->dpnidPlayer, pdpClientInfo, &dwSize, 0 ) )
			{
				break;
			}
			pdpClientInfo = (DPN_PLAYER_INFO*) new BYTE[ dwSize ];
			ZeroMemory( pdpClientInfo, dwSize );
			pdpClientInfo->dwSize = sizeof( DPN_PLAYER_INFO );

			if( DPNERR_INVALIDPLAYER == g_pDPlayServer->GetClientInfo( pCreatePlayerMsg->dpnidPlayer, pdpClientInfo, &dwSize, 0 ) )
			{
				// $$ c/s - this always comes through with bad player on hosting game.
				break;
			}

			gpstring sName( ToAnsi( pdpClientInfo->pwszName ) );

			////////////////////////////////////////
			//	get address

			gpwstring wsAddress;
			IDirectPlay8Address * pDP8ClientAddress = NULL;

			if( g_pDPlayServer->GetClientAddress( pCreatePlayerMsg->dpnidPlayer, &pDP8ClientAddress, 0 ) == S_OK )
			{
				DWORD stringSize = 0;
				pDP8ClientAddress->GetURLW( const_cast<WCHAR*>(wsAddress.c_str()), &stringSize );
				gpassert( stringSize );
				wsAddress.resize( stringSize );
				pDP8ClientAddress->GetURLW( const_cast<WCHAR*>(wsAddress.c_str()), &stringSize );
				gpassert( wsAddress.size() == stringSize );

				SAFE_RELEASE( pDP8ClientAddress );
			}

			////////////////////////////////////////
			//	create new connection based on info

			NetPipeMachine * pnewMachine	= new NetPipeMachine();
			gpwstring wsName( pdpClientInfo->pwszName );
			pnewMachine->m_wsName			= wsName;
			pnewMachine->m_wsAddress		= wsAddress;
			pnewMachine->m_Id				= pCreatePlayerMsg->dpnidPlayer;
			pnewMachine->m_bLocal			= ( (pdpClientInfo->dwPlayerFlags & DPNPLAYER_LOCAL) != 0 );
			pnewMachine->m_bHost			= ( (pdpClientInfo->dwPlayerFlags & DPNPLAYER_HOST) != 0 );

			gpassert( !pnewMachine->m_bLocal );
			gpassert( !pnewMachine->m_bHost );

			SAFE_DELETE( pdpClientInfo );

			gNetPipe.OnMachineConnected( pnewMachine );

			break;
		}

		case DPN_MSGID_CLIENT_INFO:
		{
			kerneltool::Critical::Lock lock( gNetPipe.GetSessionCritical() );
			break;
		}

		case DPN_MSGID_DESTROY_PLAYER:
		{
			kerneltool::Critical::Lock lock( gNetPipe.GetSessionCritical() );

			////////////////////////////////////////
			//	get player info

            PDPNMSG_DESTROY_PLAYER pDestroyPlayerMsg;
            pDestroyPlayerMsg = (PDPNMSG_DESTROY_PLAYER)pMsgBuffer;
			gpassert( IsValidIncomingMachineId( pDestroyPlayerMsg->dpnidPlayer ) );

			gpgenericf(( "Received DPN_MSGID_DESTROY_PLAYER, player id 0x%08x\n", pDestroyPlayerMsg->dpnidPlayer ));

			NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

			if( !session )
			{
				gpgenericf(( "Received DPN_MSGID_DESTROY_PLAYER, player id 0x%08x - during no open session\n", pDestroyPlayerMsg->dpnidPlayer ));
				break;
			}

			////////////////////////////////////////
			//	validate

			if( !session->HasMachine( pDestroyPlayerMsg->dpnidPlayer ) )
			{
				gpgenericf(( "NetPipe :: DirectPlayMessageHandler - I was told to destroy a connection to a machine not in the session. id = 0x%08x\n", pDestroyPlayerMsg->dpnidPlayer ));
				break;
			}

			////////////////////////////////////////
			//	kill connection to machine

			if( session->HasMachine( pDestroyPlayerMsg->dpnidPlayer ) )
			{
				gpgenericf((	"NetPipe :: DPHandleSystemMessage - Machine disconnected %S, id 0x%08x\n",
								session->GetMachine( pDestroyPlayerMsg->dpnidPlayer )->m_wsName.c_str(),
								pDestroyPlayerMsg->dpnidPlayer ));
			}

			gNetPipe.Notify( NetPipeMessage( NE_MACHINE_DISCONNECTED, pDestroyPlayerMsg->dpnidPlayer ) );

			session->RemoveMachine( pDestroyPlayerMsg->dpnidPlayer );
			gNetPipe.ReturnReceivePackets( pDestroyPlayerMsg->dpnidPlayer );

			break;
		}

		case DPN_MSGID_ENUM_HOSTS_QUERY:
		{
//			gpnet( "Received DPN_MSGID_ENUM_HOSTS_QUERY\n" );

			NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

			if( !session || session->IsLocked() )
			{
				result = DPNERR_NOTALLOWED;
			}
			else
			{
				result = S_OK;
			}
			break;
		}

		case DPN_MSGID_INDICATE_CONNECT:
		{
			gpnet( "Received DPN_MSGID_INDICATE_CONNECT\n" );

			PDPNMSG_INDICATE_CONNECT pIndicateConnect = (PDPNMSG_INDICATE_CONNECT)pMsgBuffer;

			NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

			NetPipe::ByteBuffer * buffer = gNetPipe.MakeTempBuffer();
			gpassert( buffer );

			// check for locked session

			if( !session || session->IsLocked() )
			{
				gNetPipe.Store( (DWORD)NE_FAILED_CONNECT_LOCKED, *buffer );
				pIndicateConnect->pvReplyData		= (void*)&*buffer->begin();
				pIndicateConnect->dwReplyDataSize	= buffer->size();

				result = DPNERR_NOTALLOWED;

				gNetPipe.FreeTempBuffer( buffer );
				break;
			}

			gpwstring wsAddress;
			DWORD stringSize = 0;
			pIndicateConnect->pAddressPlayer->GetURLW( const_cast<WCHAR*>(wsAddress.c_str()), &stringSize );
			gpassert( stringSize );
			wsAddress.resize( stringSize );
			pIndicateConnect->pAddressPlayer->GetURLW( const_cast<WCHAR*>(wsAddress.c_str()), &stringSize );
			gpassert( wsAddress.size() == stringSize );

			// check for banned ip

			if( session->IsBanned( wsAddress ) )
			{
				gNetPipe.Store( (DWORD)NE_FAILED_CONNECT_BANNED, *buffer );
				pIndicateConnect->pvReplyData		= (void*)&*buffer->begin();
				pIndicateConnect->dwReplyDataSize	= buffer->size();

				result = DPNERR_NOTALLOWED;
				break;
			}

			// check for content mismatch

			NetPipe::ByteBuffer remoteResourcesBuffer;
			gNetPipe.Store( (void*)pIndicateConnect->pvUserConnectData, pIndicateConnect->dwUserConnectDataSize, remoteResourcesBuffer );

			DWORD readPoint = 0;
			NetPipeSession::Info localInfo;
			NetPipeSession::Info remoteInfo;
			gNetPipe.ReadCopy( remoteInfo, remoteResourcesBuffer, readPoint );

			if( !IsContentCompatible( localInfo, remoteInfo ) )
			{
				NetPipe::AutoContentResourceColl remoteResources;
				gNetPipe.Read( remoteResourcesBuffer, remoteResources, readPoint );

#if !GP_RETAIL
				gpgeneric( "NetPipe :: on event DPN_MSGID_INDICATE_CONNECT:\n" );
				gpgeneric( "remote machine attempting connection, but incompatible\n" );

				gpstring sIncompat;
				GetIncompatabilityString( localInfo, remoteInfo, sIncompat );
				gpgeneric( sIncompat.c_str() );

				gpgeneric( "\nlisting remote resources:\n" );
				for( NetPipe::AutoContentResourceColl::iterator id = remoteResources.begin(); id != remoteResources.end(); ++id )
				{
					gpgenericf(( "resource = %s\n", (*id)->m_Filename.c_str() ));
				}

				gpgeneric( "\nlisting local resources:\n" );
				{for( NetPipe::AutoContentResourceColl::iterator id = gNetPipe.GetLocalContentColl().begin(); id != gNetPipe.GetLocalContentColl().end(); ++id )
				{
					gpgenericf(( "resource = %s\n", (*id)->m_Filename.c_str() ));
				}}
#endif
				gpassert( !gNetPipe.Equal( gNetPipe.GetLocalContentColl(), remoteResources ) );

				// content was not matched, so send list of server required content to client
				gNetPipe.Store( (DWORD)NE_FAILED_CONNECT_INCOMPATIBLE_CONTENT, *buffer );
				gNetPipe.Store( gNetPipe.GetLocalContentColl(), *buffer );

				result = DPNERR_NOTALLOWED;
			}
			else
			{
				gNetPipe.Store( (DWORD)NE_SESSION_CONNECTED, *buffer );
				// content matches, so let client join
				result = S_OK;
			}

			pIndicateConnect->pvReplyData		= (void*)&*buffer->begin();
			pIndicateConnect->dwReplyDataSize	= buffer->size();
			pIndicateConnect->pvReplyContext	= buffer;

			break;
		}
		case DPN_MSGID_RETURN_BUFFER:
		{
			gpnet( "Received DPN_MSGID_RETURN_BUFFER\n" );

			NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

			PDPNMSG_RETURN_BUFFER pMsg = (PDPNMSG_RETURN_BUFFER)pMsgBuffer;

			if( pMsg->pvUserContext )
			{
				gNetPipe.FreeTempBuffer( (NetPipe::ByteBuffer*)pMsg->pvUserContext );
			}

			result = S_OK;
			break;
		}
		case DPN_MSGID_INDICATED_CONNECT_ABORTED:
		{
			gpnet( "Received DPN_MSGID_INDICATED_CONNECT_ABORTED\n" );
			break;
		}
		case DPN_MSGID_RECEIVE:
		{
        	DPNMSG_RECEIVE * pReceiveMsg = (PDPNMSG_RECEIVE)pMsgBuffer;

			gpassert( IsValidIncomingMachineId( pReceiveMsg->dpnidSender ) );

			gpnetf(("Received DPN_MSGID_RECEIVE from machine 0x%08x\n", pReceiveMsg->dpnidSender ));

			if( !NetPipe::DoesSingletonExist() )
			{
				gpnet(( "NET HAZZARD: DPlay sending us packets when NetPipe() does not exist.\n" ));
				result = DPN_OK;
				break;
			}

			{
				kerneltool::Critical::Lock lock( g_IncomingRawCritical );

				UCHAR* pReceiveData = pReceiveMsg->pReceiveData;
				DWORD  dwSize=pReceiveMsg->dwReceiveDataSize;
#if !GP_RETAIL
				DWORD packetId = *(DWORD*)pReceiveData;
				pReceiveData+=4;
				dwSize -= 4;
				// checking for corrupt packets
				gpnetf(( "Compressed Receive DPlay first byte=0x%x machine=0x%08X packetid=%d compressed=%d\n",(char)pReceiveData[0],pReceiveMsg->dpnidSender,packetId,gNetPipe.IsCompressed() ));
#endif
				// Record the buffer handle so the buffer can be returned later
				IncomingPacket * info = new IncomingPacket(		pReceiveMsg->dpnidSender,
																pReceiveMsg->hBufferHandle,
																pReceiveData,
																dwSize		);
				info->m_ReceivedBy = IncomingPacket::RB_SERVER;

				g_IncomingRaw.push_back( info );

				// Tell DirectPlay to assume that ownership of the buffer 
				// has been transferred to the application, and so it will 
				// neither free nor modify it until ownership is returned 
				// to DirectPlay through the ReturnBuffer() call.
				result = DPNSUCCESS_PENDING;
			}

			break;
		}

		case DPN_MSGID_TERMINATE_SESSION:
		{
			kerneltool::Critical::Lock lock( gNetPipe.GetSessionCritical() );

			gpnet( "Received DPN_MSGID_TERMINATE_SESSION\n" );

			gpgeneric( "NetPipe :: DPHandleSystemMessage - Session lost.\n" );

        	PDPNMSG_TERMINATE_SESSION pReceiveMsg = (PDPNMSG_TERMINATE_SESSION)pMsgBuffer;

			if( pReceiveMsg->dwTerminateDataSize == 4 )
			{
				gNetPipe.Notify( NetPipeMessage( NE_SESSION_TERMINATED, *(DWORD*)pReceiveMsg->pvTerminateData ) );
			}
			else
			{
				gNetPipe.Notify( NetPipeMessage( NE_SESSION_TERMINATED ) );
			}

			gNetPipe.ReturnAllReceivePackets();

			break;
		}
		default:
		{
			break;
		}
	}

#endif

	return( result );
}


////////////////////////////////////////////////////////////////////////////////
//	
//
//	CLIENT message handler
//
//
HRESULT WINAPI DPClientMessageHandler(	PVOID /*pvUserContext*/,
                                        DWORD dwMessageId,
                                        PVOID pMsgBuffer )
{
	GPPROFILERSAMPLE( "NetPipe :: DPClientMessageHandler", SP_NET | SP_NET_RECEIVE );

	HRESULT result = S_OK;

#if !GP_DISABLE_MULTIPLAYER

	if( !g_pDPlayClient )
	{
		gpwarning( "You can't do network IO if you haven't initialized DPlay." );
		return DPNERR_NOTALLOWED;
	}

	if( !NetPipe::DoesSingletonExist() || !gNetPipe.IsInitialized() )
	{
		return DPNERR_NOTALLOWED;
	}

	gplog( OutputDirectPlayMessage( dwMessageId ) );
	gNetLog.OutputDirectPlayMessage( dwMessageId );

	switch( dwMessageId )
	{
		case DPN_MSGID_CONNECT_COMPLETE:
		{
			kerneltool::Critical::Lock lock( gNetPipe.GetSessionCritical() );

			gpnet( "Received DPN_MSGID_CONNECT_COMPLETE\n" );

			PDPNMSG_CONNECT_COMPLETE pconnect = (PDPNMSG_CONNECT_COMPLETE) pMsgBuffer;

			bool success = false;
			
			if( S_OK == pconnect->hResultCode )
			{
				NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

				// create player may have done this if connecting, otherwise do for hosting

				if( !session )
				{
					gNetPipe.ConnectComplete( NE_OK );
				}
				success = true;
			}
			else
			{
				eNetPipeEvent event = NE_FAILED_CONNECT;

				if( pconnect->dwApplicationReplyDataSize > 0 )
				{
					// handle netpipe errors

					event = (eNetPipeEvent)*((DWORD*)(pconnect->pvApplicationReplyData));

					if( event == NE_FAILED_CONNECT_INCOMPATIBLE_CONTENT )
					{
						gNetPipe.ClearServerRequiredResourceColl();

						NetPipe::ByteBuffer buffer;
						BYTE * begin	= ((BYTE*)pconnect->pvApplicationReplyData)+4;
						BYTE * end		= ((BYTE*)pconnect->pvApplicationReplyData)+pconnect->dwApplicationReplyDataSize;
						buffer.assign( begin, end );
						gNetPipe.Read( buffer, gNetPipe.GetServerRequiredContentColl() );
					}
				}
				else
				{
					// handle dplay-only errors

					switch( pconnect->hResultCode )
					{
					case DPNERR_HOSTREJECTEDCONNECTION:
						event = NE_FAILED_CONNECT_HOST_REJECTED;
						break;
					case DPNERR_INVALIDPASSWORD:
						event = NE_FAILED_CONNECT_INVALID_PASSWORD;
						break;
					case DPNERR_SESSIONFULL:
						event = NE_FAILED_CONNECT_SESSION_FULL;
						break;
					};
				}

				// not bothering to create NetPipe or World message for every possible error, see ShowFailedToJoinMessage
				gNetPipe.Notify( NetPipeMessage( NE_FAILED_CONNECT, (DWORD)event ) );
			}

			////////////////////////////////////////////////////////////////////////////////
			//	get server info and make machine connection info for it

			if( success )
			{
				DWORD dwSize = 0;
				DPN_PLAYER_INFO * pdpServerInfo = NULL;

				g_pDPlayClient->GetServerInfo( pdpServerInfo, &dwSize, 0 );
				pdpServerInfo = (DPN_PLAYER_INFO*) new BYTE[ dwSize ];
				ZeroMemory( pdpServerInfo, dwSize );
				pdpServerInfo->dwSize = sizeof(DPN_PLAYER_INFO);
				TRYDX_SUCCEED( g_pDPlayClient->GetServerInfo( pdpServerInfo, &dwSize, 0 ) );

				gpstring sName( ToAnsi( pdpServerInfo->pwszName ) );

				////////////////////////////////////////
				//	get address
				
				gpwstring wsAddress;
				IDirectPlay8Address * pDP8ServerAddress = NULL;

				if( g_pDPlayClient->GetServerAddress( &pDP8ServerAddress, 0 ) == S_OK )
				{
					DWORD stringSize = 0;
					pDP8ServerAddress->GetURLW( const_cast<WCHAR*>(wsAddress.c_str()), &stringSize );
					gpassert( stringSize );
					wsAddress.resize( stringSize );
					pDP8ServerAddress->GetURLW( const_cast<WCHAR*>(wsAddress.c_str()), &stringSize );
					gpassert( wsAddress.size() == stringSize );

					SAFE_RELEASE( pDP8ServerAddress );
				}

				////////////////////////////////////////
				//	create new connection based on info

				NetPipeMachine * pnewMachine = new NetPipeMachine();
				gpwstring wsName( pdpServerInfo->pwszName );

				// change RPC_TO_SERVER to reflect remote server
				FuBi::SetServerIsRemote();

				pnewMachine->m_wsName       = wsName;
				pnewMachine->m_wsAddress    = wsAddress;
				pnewMachine->m_Id           = RPC_TO_SERVER;
				pnewMachine->m_bLocal       = false;
				pnewMachine->m_bHost        = true;

				SAFE_DELETE( pdpServerInfo );

				gNetPipe.OnMachineConnected( pnewMachine );
			}

			// finish
			gNetPipe.SetIsConnectInProgress( false );

			break;
		}

		case DPN_MSGID_CREATE_PLAYER:
		{
			kerneltool::Critical::Lock lock( gNetPipe.GetSessionCritical() );
			gpassert( 0 );
			break;
		}

		case DPN_MSGID_CLIENT_INFO:
		{
			kerneltool::Critical::Lock lock( gNetPipe.GetSessionCritical() );
			gpassert( 0 );
			break;
		}

		case DPN_MSGID_ENUM_HOSTS_RESPONSE:
		{
			kerneltool::Critical::Lock lock( gNetPipe.GetSessionCritical() );

			gpnet( "Received DPN_MSGID_ENUM_HOSTS_RESPONSE\n" );

			NetPipeSessionHandle hSession = gNetPipe.GetOpenSessionHandle();

			if( hSession && hSession->IsLocked() )
			{
				break;
			}

			PDPNMSG_ENUM_HOSTS_RESPONSE pEnumResponse = (PDPNMSG_ENUM_HOSTS_RESPONSE)pMsgBuffer;

			////////////////////////////////////////
			//	PDPNMSG_ENUM_HOSTS_RESPONSE

			NetPipeSession session;

			gpwstring wsAddress;
			DWORD stringSize = 0;
			pEnumResponse->pAddressSender->GetURLW( const_cast<WCHAR*>(wsAddress.c_str()), &stringSize );
			gpassert( stringSize );
			wsAddress.resize( stringSize );
			pEnumResponse->pAddressSender->GetURLW( const_cast<WCHAR*>(wsAddress.c_str()), &stringSize );
			gpassert( wsAddress.size() == stringSize );
	
			session.SetHostAddress( wsAddress );

			gpstring debugHostAddress( ToAnsi( wsAddress ) );
	
			stringSize = 0;
			pEnumResponse->pAddressDevice->GetURLW( const_cast<WCHAR*>(wsAddress.c_str()), &stringSize );
			gpassert( stringSize );
			wsAddress.resize( stringSize );
			pEnumResponse->pAddressDevice->GetURLW( const_cast<WCHAR*>(wsAddress.c_str()), &stringSize );
			gpassert( wsAddress.size() == stringSize );

			gpstring debugDeviceAddress( ToAnsi( wsAddress ) );

			session.SetDeviceAddress( wsAddress );

			session.SetLatency( float(pEnumResponse->dwRoundTripLatencyMS) / float( 1000 ) );

			////////////////////////////////////////
			//	get DPN_APPLICATION_DESC

			DPN_APPLICATION_DESC const * appDesc = pEnumResponse->pApplicationDescription;

			session.SetApplicationGuid(         appDesc->guidApplication );
			session.SetApplicationInstanceGuid( appDesc->guidInstance    );

			gpstring debugSessionName( ToAnsi( appDesc->pwszSessionName ) );
			session.SetSessionName( gpwstring( appDesc->pwszSessionName  ) );

			gpwstring wsPassword;
			if( appDesc->pwszPassword )
			{
				wsPassword = appDesc->pwszPassword;
				session.SetSessionPassword( wsPassword );
			}
			
			if( appDesc->dwFlags & DPNSESSION_REQUIREPASSWORD )
			{
				session.SetHasPassword( true );
			}

			session.SetNumMachinesMax(  appDesc->dwMaxPlayers     );
			session.SetNumMachinesPresent( appDesc->dwCurrentPlayers );

			////////////////////////////////////////
			//	get version info

			gpassert( appDesc->pvApplicationReservedData );
			NetPipeSession::Info info;
			memcpy( &info, appDesc->pvApplicationReservedData, min( appDesc->dwApplicationReservedDataSize, DWORD( sizeof( NetPipeSession::Info ) ) ) );
			session.GetInfo() = info;

			////////////////////////////////////////
			//	reject enumeration of session without exact version match

			NetPipeSession::Info localInfo;

			if( IsProtocolCompatible( localInfo.m_NetPipeVersion, info.m_NetPipeVersion ) )
			{
				session.SetIsExecutableCompatible( IsExecutableCompatible( localInfo.m_NetPipeVersion, info.m_NetPipeVersion ) );
				session.SetIsContentCompatible( IsContentCompatible( localInfo, info ) );
				gNetPipe.AddSession( session );
			}
			else
			{
				// do nothing; completely ignore protocol incompatible games
			}

			break;
		}

		case DPN_MSGID_RECEIVE:
		{
        	DPNMSG_RECEIVE * pReceiveMsg = (PDPNMSG_RECEIVE)pMsgBuffer;

			//gpassert( IsValidIncomingMachineId( pReceiveMsg->dpnidSender ) );
			gpassert( !g_pDPlayServer );

			gpnetf(("Received DPN_MSGID_RECEIVE from machine 0x%08x\n", pReceiveMsg->dpnidSender ));

			if( !NetPipe::DoesSingletonExist() )
			{
				gpnet(( "NET HAZZARD: DPlay sending us packets when NetPipe() does not exist.\n" ));
				result = DPN_OK;
				break;
			}

			{
				kerneltool::Critical::Lock lock( g_IncomingRawCritical );

				UCHAR* pReceiveData = pReceiveMsg->pReceiveData;
				DWORD  dwSize=pReceiveMsg->dwReceiveDataSize;
#if !GP_RETAIL

				DWORD packetId = *(DWORD*)pReceiveData;
				pReceiveData+=4;
				dwSize -= 4;
				// checking for corrupt packets
				gpnetf(("Compressed Receive DPlay first byte=0x%x machine=0x%08X packetid=%d compressed=%d\n",(char)pReceiveData[0],pReceiveMsg->dpnidSender,packetId,gNetPipe.IsCompressed()));

#endif
				// Record the buffer handle so the buffer can be returned later
				IncomingPacket * info = new IncomingPacket(		RPC_TO_SERVER,
																pReceiveMsg->hBufferHandle,
																pReceiveData,
																dwSize		);
				info->m_ReceivedBy = IncomingPacket::RB_CLIENT;

				g_IncomingRaw.push_back( info );

				// Tell DirectPlay to assume that ownership of the buffer 
				// has been transferred to the application, and so it will 
				// neither free nor modify it until ownership is returned 
				// to DirectPlay through the ReturnBuffer() call.
				result = DPNSUCCESS_PENDING;
			}

			break;
		}

		case DPN_MSGID_TERMINATE_SESSION:
		{
			kerneltool::Critical::Lock lock( gNetPipe.GetSessionCritical() );

			gpnet( "Received DPN_MSGID_TERMINATE_SESSION\n" );

			gpgeneric( "NetPipe :: DPHandleSystemMessage - Session lost.\n" );

        	DPNMSG_TERMINATE_SESSION * pReceiveMsg = (DPNMSG_TERMINATE_SESSION*)pMsgBuffer;

			if( pReceiveMsg->dwTerminateDataSize == 4 )
			{
				gNetPipe.Notify( NetPipeMessage( NE_SESSION_TERMINATED, *(DWORD*)pReceiveMsg->pvTerminateData ) );
			}
			else
			{
				gNetPipe.Notify( NetPipeMessage( NE_SESSION_TERMINATED ) );
			}

			gNetPipe.ReturnAllReceivePackets();

			break;
		}
		default:
		{
			break;
		}
	}

#endif

	return( result );
}


void NetPipe :: Notify( NetPipeMessage const & msg )
{
	kerneltool::Critical::Lock lock( m_NotifyCritical );
	m_NetPipeMessageColl.push_back( msg );
}


void NetPipe :: OnMachineConnected( NetPipeMachine * pmachine )
{
	kerneltool::Critical::Lock lock( m_NotifyCritical );

	m_AddMachineColl.push_back( pmachine );
}


//#	if !GP_RETAIL
void NetPipe :: Dump( ReportSys::ContextRef ctx )
{
//	kerneltool::Critical::Lock lock1( m_SendCritical    );
//	kerneltool::Critical::Lock lock2( m_ReceiveCritical );
//	kerneltool::Critical::Lock lock3( m_NotifyCritical  );
//	kerneltool::Critical::Lock lock4( m_SessionCritical );
//	kerneltool::Critical::Lock lock5( m_LatencyCritical );

	////////////////////////////////////////
	//	dump owned vars

	ctx->Output( "[NETPIPE]\n" );
	ctx->Indent();

	//ctx->OutputF( "receive buffer size = %d, last msg received size = %d\n", m_ReceiveBufferSize, m_LastMessageReceivedSize );
	if (!IsCompressed())
	{
		m_BytesSentComp = m_BytesSentUnComp;
		m_BytesRcvdUnComp = m_BytesRcvdComp;
	}
	ctx->OutputF( "[throughput] compressed = %s, turtle = %s, sent = %d/%d=%d%%, received = %d/%d=%d%%\n",
					m_bCompressed ? "TRUE" : "FALSE",
					m_bHasTurtle ? "TRUE" : "FALSE",
					m_BytesSentComp, m_BytesSentUnComp, 
					m_BytesSentUnComp ? 100 * (int)(m_BytesSentUnComp - m_BytesSentComp) / m_BytesSentUnComp : 0,
					m_BytesRcvdComp, m_BytesRcvdUnComp, 
					m_BytesRcvdUnComp ? 100 * (int)(m_BytesRcvdUnComp - m_BytesRcvdComp) / m_BytesRcvdUnComp : 0   );

	m_BytesSentComp   = 0;
	m_BytesRcvdComp   = 0;
	m_BytesSentUnComp = 0;
	m_BytesRcvdUnComp = 0;

	ctx->OutputF( "[buffers] g_IncomingMessages = %d, duration of outbound traffic backup = %2.2f\n",
					g_IncomingPackets.size(), GetOutboundTrafficBackupDuration() );

	////////////////////////////////////////
	//	dump sessions

	if( !m_Sessions.empty() )
	{
		ctx->Output( "[Sessions]\n" );
		ctx->Indent();
		for( NetPipeSessionColl::iterator is = m_Sessions.begin(); is != m_Sessions.end(); ++is )
		{
			(*is)->Dump( ctx );
		}
		ctx->Outdent();
	}

	ctx->Outdent();

}
//#	endif // !GP_RETAIL



NetPipeMachine :: NetPipeMachine()
{
	m_bLocal				= false;
	m_bHost					= false;
	m_wsName				= L"unknown";
	m_Id					= 0;
	m_LastReceiveTime		= GetGlobalSeconds();
	m_LastSendTime			= GetGlobalSeconds();
	m_Id					= 0;
	m_Latency				= 0;
#if !GP_RETAIL
	m_lastPacketId			= 0;
#endif

	m_BytesSent				= 0;
	m_BytesReceived			= 0;

	m_PlayersSynced			= 0;
	m_LocalPlayerCreated	= false;

	if( gNetPipe.IsCompressed() )
	{
		gpnet( "NetPipeMachine Created Compressed\n" );
        m_decompress.Init();
        m_compress.Init(Z_BEST_COMPRESSION);
        m_compress.SetChunkBased(true);
	}


	ZeroObject( m_dciLast );
};








///////////////////////////////////////////////////////////////////////////////////////////
//	class NetPipeSession implementation
///////////////////////////////////////////////////////////////////////////////////////////


NetPipeSession::Info::Info()
{
	m_Size				= sizeof( Info );
	m_WorldState		= 0;
	m_Magic				= 0;

	m_ContentSyncSpec	= gContentDb.MakeSynchronizeSpec();
}


bool IsProtocolCompatible( NetPipeVersion const & l, NetPipeVersion const & r )
{
	return l.m_TransportVersion == r.m_TransportVersion;
}


bool IsExecutableCompatible( NetPipeVersion const & l, NetPipeVersion const & r )
{
	return	IsProtocolCompatible( l, r )
			&& ( l.m_bCompressed == r.m_bCompressed )
			&& ( l.m_bTimeoutDetect == r.m_bTimeoutDetect )
			&& ( l.m_BuildType == r.m_BuildType )
			&& ( l.m_ExeCrc == r.m_ExeCrc )
			&& ( l.m_FuBiSyncSpec == r.m_FuBiSyncSpec )
			&& ( l.m_ProductVersion == r.m_ProductVersion );
}


void GetIncompatabilityString( NetPipeVersion const & l, NetPipeVersion const & r, gpstring & out )
{
	out.appendf( "%s - NetPipeVersion::m_bCompressed\n", l.m_bCompressed == r.m_bCompressed ? "OK " : "BAD" );
	out.appendf( "%s - NetPipeVersion::m_bTimeoutDetect\n", l.m_bTimeoutDetect == r.m_bTimeoutDetect ? "OK " : "BAD" );
	out.appendf( "%s - NetPipeVersion::m_BuildType\n", l.m_BuildType == r.m_BuildType ? "OK " : "BAD" );
	out.appendf( "%s - NetPipeVersion::m_ExeCrc\n", l.m_ExeCrc == r.m_ExeCrc ? "OK " : "BAD" );
	out.appendf( "%s - NetPipeVersion::m_FuBiSyncSpec\n", l.m_FuBiSyncSpec == r.m_FuBiSyncSpec ? "OK " : "BAD" );
	out.appendf( "%s - NetPipeVersion::m_ProductVersion\n", l.m_ProductVersion == r.m_ProductVersion ? "OK " : "BAD" );
	out.appendf( "%s - NetPipeVersion::m_TransportVersion\n", l.m_TransportVersion == r.m_TransportVersion ? "OK " : "BAD" );
}


bool IsContentCompatible( NetPipeSession::Info const & l, NetPipeSession::Info const & r )
{
	return( IsExecutableCompatible( l.m_NetPipeVersion, r.m_NetPipeVersion )
			&& !memcmp( &l.m_ContentSyncSpec, &r.m_ContentSyncSpec, sizeof( ContentDb::SyncSpec ) ) );
}


void GetIncompatabilityString( NetPipeSession::Info const & l, NetPipeSession::Info const & r, gpstring & out )
{
	GetIncompatabilityString( l.m_NetPipeVersion, r.m_NetPipeVersion, out );
	out.appendf( "%s - NetPipeSession::Info::m_ContentSyncSpec\n", !memcmp( &l.m_ContentSyncSpec, &r.m_ContentSyncSpec, sizeof( ContentDb::SyncSpec ) ) ? "OK " : "BAD" );
}


////////////////////////////////////////////////////////////////////////////////
//
DWORD NetPipeSession::m_InstanceCount = 0;

NetPipeSession::NetPipeSession()
{
	// every session has a unique ID
	++m_InstanceCount;

	m_ApplicationGuid         = GPG_NetPipe_GUID;    
	m_ApplicationInstanceGuid = GPG_GUID_INVALID;
	m_LocalId				  = m_InstanceCount;
	m_Info.m_Magic		  = m_InstanceCount;

	m_wsHostAddress           = L"";                 
	m_wsDeviceAddress         = L"";                 
	m_Flags                   = 0;                   
	m_wsSessionName           = L"unknown";          
	m_wsSessionPassword       = L"";
	m_Latency				  = 0;
	m_NumMachinesMax          = NETPIPE_MAX_MACHINES;
	m_NumMachinesPresent      = 0;
	m_bExecutableCompatible	  = false;
	m_bContentCompatible	  = false;
	m_bLocked                 = false;
	m_bHost                   = false;
	m_bHasPassword			  = false;
}


////////////////////////////////////////////////////////////////////////////////
//
NetPipeSession::~NetPipeSession()
{
	for( NetPipeMachineColl::iterator iMachine = m_Machines.begin(); iMachine != m_Machines.end(); ++iMachine )
	{
		delete (*iMachine);
	}
	m_Machines.clear();
}


////////////////////////////////////////////////////////////////////////////////
//
NetPipeSession & NetPipeSession::operator = ( NetPipeSession const & rhs )
{
	kerneltool::RwCritical::ReadLock rlock( rhs.m_Critical );
	kerneltool::RwCritical::WriteLock wlock( m_Critical );

	m_wsHostAddress			  = rhs.m_wsHostAddress;
	m_wsDeviceAddress         = rhs.m_wsDeviceAddress;
	m_ApplicationGuid         = rhs.m_ApplicationGuid;
	m_ApplicationInstanceGuid = rhs.m_ApplicationInstanceGuid;
	m_wsSessionName           = rhs.m_wsSessionName;          
	m_wsSessionPassword       = rhs.m_wsSessionPassword;      
	m_Latency				  = rhs.m_Latency;
	m_Flags                   = rhs.m_Flags;                  
	m_NumMachinesMax		  = rhs.m_NumMachinesMax;
	m_NumMachinesPresent      = rhs.m_NumMachinesPresent;
	m_bHasPassword			  = rhs.m_bHasPassword;

	m_bExecutableCompatible	  = rhs.m_bExecutableCompatible;
	m_bContentCompatible	  = rhs.m_bContentCompatible;
	m_bLocked				  = rhs.m_bLocked;
	//m_bHost				  = rhs.m_bHost;	// ignore

	m_Info				  = rhs.m_Info;
	//m_LocalId				  = rhs.m_InstanceCount;

	m_TimeLastUpdated		  = m_bLocked ? 0 : GetGlobalSeconds();

	return( *this );
}


////////////////////////////////////////////////////////////////////////////////
//
void NetPipeSession :: SUpdate()
{
	kerneltool::RwCritical::ReadLock lock( m_Critical );

	gpassert( FuBi::IsServerLocal() );

	if( !g_pDPlayServer )
	{
		return;
	}

    // Get the application description
    DPN_APPLICATION_DESC * pAppDesc = NULL;
    DWORD dwSize = 0;
    HRESULT h = g_pDPlayServer->GetApplicationDesc( pAppDesc, &dwSize, 0 );
	gpassert( h == DPNERR_BUFFERTOOSMALL );
	if( h == DPNERR_BUFFERTOOSMALL )
	{
		pAppDesc = (DPN_APPLICATION_DESC*) new BYTE[dwSize];
		ZeroMemory( pAppDesc, sizeof(DPN_APPLICATION_DESC) );
		pAppDesc->dwSize = sizeof(DPN_APPLICATION_DESC);

   		h = g_pDPlayServer->GetApplicationDesc( pAppDesc, &dwSize, 0 );
		TRYDX( h );
		if( !FAILED( h ) )
		{
			pAppDesc->pwszSessionName				= const_cast<WCHAR*>( GetSessionName().c_str() );
			pAppDesc->dwMaxPlayers 					= GetNumMachinesMax();
			pAppDesc->dwCurrentPlayers				= GetNumMachinesPresent();
			pAppDesc->pwszPassword					= GetSessionPassword().empty() ? NULL : const_cast<WCHAR*>( GetSessionPassword().c_str() );
			pAppDesc->dwFlags						= pAppDesc->pwszPassword ? DPNSESSION_REQUIREPASSWORD : 0;
			pAppDesc->pvApplicationReservedData		= &m_Info;
			pAppDesc->dwApplicationReservedDataSize = sizeof( Info );

			// Tell DirectPlay about the change
			TRYDX( g_pDPlayServer->SetApplicationDesc( pAppDesc, 0 ) );
		}
		// Cleanup the data
		SAFE_DELETE( pAppDesc );
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
bool NetPipeSession :: HasMachine( DWORD MachineId )
{
	kerneltool::RwCritical::ReadLock lock( m_Critical );

	return( GetMachine( MachineId ) != NULL );
}


////////////////////////////////////////////////////////////////////////////////
//	
void NetPipeSession :: AddMachine( NetPipeMachine * pmachine )
{
	kerneltool::RwCritical::WriteLock lock( m_Critical );

	gpassert( !HasMachine( pmachine->m_Id ) );
	m_Machines.push_back( pmachine );

	gpgenericf(( "NetPipeSession :: AddMachine - name = %S, machineId = 0x%08x\n", pmachine->m_wsName.c_str(), pmachine->m_Id ));
}


////////////////////////////////////////////////////////////////////////////////
//	
void NetPipeSession :: RemoveMachine( DWORD machineId )
{
	kerneltool::RwCritical::WriteLock lock( m_Critical );

	gpassert( (machineId != RPC_TO_ALL) && (machineId != RPC_TO_OTHERS) );
	gpassert( HasMachine( machineId ) );

	NetPipeMachineColl :: iterator i;

	for( i=m_Machines.begin(); i<m_Machines.end(); ++i )
	{
		if( (*i)->m_Id == machineId )
		{
			gpgenericf(( "NetPipeSession :: RemoveMachine - name = %S, machineId = 0x%08x\n", (*i)->m_wsName.c_str(), (*i)->m_Id ));
			delete (*i);
			m_Machines.erase( i );
			return;
		}
	}

	gperrorf(( "NetPipeSession :: RemoveMachine - Machine not found, id 0x%08x", machineId ));
}


////////////////////////////////////////////////////////////////////////////////
//	
void NetPipeSession :: AddBanned( DWORD machineId )
{
	kerneltool::RwCritical::WriteLock lock( m_Critical );

	NetPipeMachine *pBanned = GetMachine( machineId );

	if ( pBanned )
	{
		m_BannedColl.push_back( pBanned->m_wsAddress );
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
bool NetPipeSession :: IsBanned( gpwstring const & address )
{
	kerneltool::RwCritical::ReadLock lock( m_Critical );

	bool banned = false;

	for ( BannedColl::iterator i = m_BannedColl.begin(); i != m_BannedColl.end(); ++i )
	{
		if ( i->same_no_case( address ) )
		{
			banned = true;
			break;
		}
	}

	return ( banned );
}


////////////////////////////////////////////////////////////////////////////////
//	
NetPipeMachine * NetPipeSession :: GetMachine( DWORD machineId )
{
	kerneltool::RwCritical::ReadLock lock( m_Critical );

	gpassert( (machineId != RPC_TO_ALL) && (machineId != RPC_TO_OTHERS) );

	NetPipeMachineColl :: iterator i;

	for( i = m_Machines.begin(); i != m_Machines.end(); ++i )
	{
		if( (*i)->m_Id == machineId )
		{
			return( *i );
		}
	}

	return NULL;
}


//#	if !GP_RETAIL
////////////////////////////////////////////////////////////////////////////////
//	
void NetPipeSession :: Dump( ReportSys::ContextRef ctx )
{
	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	kerneltool::RwCritical::ReadLock lock( m_Critical );

	ctx->OutputF( "%S, pw=%S, locked=%d, dt last update=%2.2f, host=%d, %s, version=%s\n",
					m_wsSessionName.c_str(),
					m_wsSessionPassword.c_str(),
					m_bLocked,
					GetGlobalSeconds() - m_TimeLastUpdated,
					m_bHost,
					session.GetPointer() == this ? "***OPEN***" : "",
					IsExecutableCompatible() ? "OK" : "INCOMPATIBLE!" );

	if( !m_Machines.empty() )
	{
		ctx->Output( "[Machines]\n" );
		ctx->Indent();
		ctx->OutputF( "max connections=%d, connections used=%d\n", m_NumMachinesMax, m_NumMachinesPresent );

		for( NetPipeMachineColl::iterator ic = m_Machines.begin(); ic != m_Machines.end(); ++ic )
		{
			(*ic)->Dump( ctx );
		}
		ctx->Outdent();
	}
}
//#	endif // !GP_RETAIL




////////////////////////////////////////////////////////////////////////////////
//	NetPipeMachine

	// debug
//#	if !GP_RETAIL
void NetPipeMachine :: Dump( ReportSys::ContextRef ctx )
{
	gpassert( g_pDPlayServer || g_pDPlayClient );

	ctx->OutputF( "name=%s, id=0x%08x, local=%d, host=%d\n",
					ToAnsi( m_wsName ).c_str(),
					m_Id,
					m_bLocal,
					m_bHost );

    if (!m_bLocal)
    {
        DWORD dwMsgs = 0;
        DWORD dwBytes = 0;
		
        DPN_CONNECTION_INFO dci;
        dci.dwSize = sizeof(dci);

        HRESULT hr = g_pDPlayServer ? g_pDPlayServer->GetConnectionInfo( m_Id, &dci, 0 )
			: g_pDPlayClient->GetConnectionInfo( &dci, 0 );

		if( SUCCEEDED(hr) )
        {
            hr = g_pDPlayServer ? g_pDPlayServer->GetSendQueueInfo( m_Id, &dwMsgs, &dwBytes, 0 )
				:	g_pDPlayClient->GetSendQueueInfo( &dwMsgs, &dwBytes, 0 );

			if ( SUCCEEDED(hr) )
			{
				// assuming 1 second interval
				ctx->OutputF(	"  latency=%2.2f thru=%d thruMax=%d sent=%d/%d rcvd=%d/%d sndq=%d/%d\n",
								float( dci.dwRoundTripLatencyMS ) / 1000.0f,
								dci.dwThroughputBPS, 
								dci.dwPeakThroughputBPS,
								( dci.dwBytesSentGuaranteed - m_dciLast.dwBytesSentGuaranteed ) + ( dci.dwBytesSentNonGuaranteed - m_dciLast.dwBytesSentNonGuaranteed ), 
								( dci.dwPacketsSentGuaranteed - m_dciLast.dwPacketsSentGuaranteed ) + ( dci.dwPacketsSentNonGuaranteed - m_dciLast.dwPacketsSentNonGuaranteed ),
								( dci.dwBytesReceivedGuaranteed - m_dciLast.dwBytesReceivedGuaranteed ) + ( dci.dwPacketsReceivedNonGuaranteed - m_dciLast.dwPacketsReceivedNonGuaranteed ), 
								( dci.dwPacketsReceivedGuaranteed - m_dciLast.dwPacketsReceivedGuaranteed ) + ( dci.dwPacketsReceivedNonGuaranteed - m_dciLast.dwPacketsReceivedNonGuaranteed ),
								dwBytes,
								dwMsgs	);
				m_dciLast = dci; // save current info for next time
			}
			else
			{
				ctx->OutputF( " GetSendQueueInfo returned an error code 0x%08x [%s]\n", hr, LookupDxErrorMessage(hr));
			}
		}
		else
		{
			ctx->OutputF( " GetConnectionInfo returned an error code 0x%08x [%s]\n", hr, LookupDxErrorMessage(hr));
		}

		ctx->OutputF( "  raw snd = %d, raw rec = %d, dt snd = %2.2f, dt rec = %2.2f\n", m_BytesSent, m_BytesReceived, GetTimeElapsedSinceLastSend(), GetTimeElapsedSinceLastReceive() );
    }
}
//#	endif // !GP_RETAIL

/////////////////////////////////////////////////////////////////////////////////
// DPlay close thread implementation


DPlayCloseServerThread::DPlayCloseServerThread( IDirectPlay8Server *pDP) : Thread( OPTION_DELETE_SELF )
{
	gpassert( pDP );
	m_pDP=pDP;
}

DWORD DPlayCloseServerThread::Execute()
{
	if(m_pDP)
	{
		gpnet("DPlayCloseThread starting DPlay Close\n");
		m_pDP->Close(0);
		gpnet("DPlayCloseThread finished DPlay Close\n");

		SAFE_RELEASE(m_pDP);
	}
	return 0;
}


DPlayCloseClientThread::DPlayCloseClientThread( IDirectPlay8Client *pDP) : Thread(OPTION_DELETE_SELF)
{
	gpassert(pDP);
	m_pDP=pDP;
}

DWORD DPlayCloseClientThread::Execute()
{
	if (m_pDP)
	{
		gpnet("DPlayCloseThread starting DPlay Close\n");
		m_pDP->Close(0);
		gpnet("DPlayCloseThread finished DPlay Close\n");

		SAFE_RELEASE(m_pDP);
	}
	return 0;
}


