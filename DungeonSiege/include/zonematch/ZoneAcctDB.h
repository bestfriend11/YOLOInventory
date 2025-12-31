/******************************************************************************

    ZoneAcctDB.h

	Copyright (c) Microsoft Corp. 1999-2000. All rights reserved.

    Content: Interface for the ZoneAcctDB user account database layer
	 
******************************************************************************/



#ifndef _ZONEACCTDB_H
#define _ZONEACCTDB_H

#include "zonetecherr.h"


// {C9705619-CAE0-4fd5-9D58-FAFE58D89E19}
DEFINE_GUID( CLSID_ZoneAcctDB, 0xc9705619, 0xcae0, 0x4fd5, 0x9d, 0x58, 0xfa, 0xfe, 0x58, 0xd8, 0x9e, 0x19);
// {3FF6F9FC-5019-4dec-8163-4BF7903DF54D}
DEFINE_GUID( IID_IZoneAcctDB, 0x3ff6f9fc, 0x5019, 0x4dec, 0x81, 0x63, 0x4b, 0xf7, 0x90, 0x3d, 0xf5, 0x4d);


enum PASSWORD_VALIDATION_RESULT
{
    PW_VALIDATION_SUCCESS,
    PW_VALIDATION_INVALIDPW,
    PW_VALIDATION_NOUSER,
    PW_VALIDATION_ERROR
};

typedef void ( WINAPI *LPFN_VERIFYPW_CALLBACK)(PASSWORD_VALIDATION_RESULT pwResult);
typedef void ( WINAPI *LPFN_CREATEACCOUNT_CALLBACK)(HRESULT hr);
typedef void ( WINAPI *LPFN_UPDATELOGIN_CALLBACK)(HRESULT hr);
typedef void ( WINAPI *LPFN_LOGINUSER_CALLBACK)(PASSWORD_VALIDATION_RESULT pwResult);

struct SPlayerAccount
{
    char *m_pszUserName;
    char *m_pszPassword;
    int m_iRoleId;
    LPFN_CREATEACCOUNT_CALLBACK m_pfn;
};

interface IZoneAcctDB : public IUnknown
{
    virtual HRESULT Init(WCHAR *wszServerName, WCHAR*wszDatabaseName) PURE;
    virtual HRESULT FVerifyPassword(char *szUserName, char*szPassword, 
                        LPFN_VERIFYPW_CALLBACK pfnCallback) PURE;
    virtual HRESULT CreateAccount(SPlayerAccount *pacct) PURE;
    virtual HRESULT UpdateLastLogin(char *szUserName, LPFN_UPDATELOGIN_CALLBACK pfnCallback) PURE;
    virtual HRESULT LoginUser(char *szUserName, char*szPassword, 
                        LPFN_LOGINUSER_CALLBACK pfnCallback) PURE;
};


/*

////////////////////////////////////////////////////////////////////////////////
//
// ZoneNet structures
//

#pragma pack (push, 2)

class ZoneNetAddress;
interface IZoneNet;

typedef void ( WINAPI *LPFN_GETADDR_CALLBACK)( IN ZoneNetAddress *pAddr, IN LPVOID pContext );
typedef void ( WINAPI *LPFN_APC_CALLBACK)( IN LPVOID pContext );

//
// ZoneNetAddress
//
// simple class for handling IP address and port pairs
// data is stored in host byte order
//
class ZoneNetAddress
{
public:

    ZoneNetAddress() : addr(INADDR_NONE), port(0) {}

    ZoneNetAddress( IN unsigned long a, IN unsigned short p ) : addr(a), port(p) {}
    ZoneNetAddress( IN LPSOCKADDR_IN pSA ) : addr(ntohl(pSA->sin_addr.s_addr)), port(ntohs(pSA->sin_port)) {}

    //
    // note: b/c of the use of the underlying WinSock API inet_ntoa()
    // the string return is only valid until the next inet_ntoa() or GetAddress()
    // call is made.
    //
    char* GetAddress()
        {
            IN_ADDR inaddr;
            inaddr.s_addr = htonl(addr);
            return inet_ntoa( inaddr );
        }

    BOOL IsValid() { return INADDR_NONE != addr; }

    
    unsigned long  addr;
    unsigned short port;
};


//
// ZoneNetSettings
//
// structure containing parameters to configure the server layer
// and underlying socket settings
//
struct ZoneNetSettings
{
    DWORD cbSize;  // sizeof structure

    enum FIELD   { NET_NONE             = 0x00,

                   NET_ENABLE_POOLS     = 0x01,
                   NET_MAX_TCP_SEND     = 0x02,
                   NET_MAX_UDP_SEND     = 0x04,
                   NET_MAX_QUEUED_BYTES = 0x08,
                   NET_SEND_TIMEOUT     = 0x10,

                   NET_DEBUG            = 0x80000000,

                   NET_ALL              = 0xFFFFFFFF
                 };

    DWORD dwFields; // bit flags indicating structure members in use
                    // comprised of or'ed ZoneNetSettings::FIELD values

    BOOL  bEnablePools;     // enable pooling of memory vs dynamically allocating from heap as needed
    DWORD dwMaxTcpSend;     // max number of bytes to issue in a single TCP send
    DWORD dwMaxUdpSend;     // max number of bytes to issue in a single UDP send
    DWORD dwMaxQueuedBytes; // max number of bytes to queue prior to a send
    DWORD dwSendTimeout;    // max timeout in milliseconds for a send to complete
    BOOL  bDebug;
};


//
// ZoneNetServerSettings
//
// structure containing parameters to configure the server layer
// and underlying socket settings
//
struct ZoneNetServerSettings
{
    DWORD cbSize;  // sizeof structure

    enum FIELD   { SERVER_NONE           = 0x0,

                   SERVER_SOCKET_BACKLOG = 0x1,
                   SERVER_QUEUED_ACCEPTS = 0x2,

                   SERVER_ALL            = 0xFFFFFFFF
                 };

    DWORD dwFields; // bit flags indicating structure members in use
                    // comprised of or'ed ZoneNetServerSettings::FIELD values

    int   wSocketBacklog;  // TCP - backlog passed to listen()
    WORD  wQueuedAccepts;  // TCP - number of async AcceptEx() to issue for client connections
};


//
// ZoneNetBatchSettings
//
// structure containing parameters to configure the packet batching layer
//
struct ZoneNetBatchSettings
{
    DWORD cbSize;  // sizeof structure

    enum FIELD   { BATCH_NONE        = 0x0,
                   BATCH_MAX_BYTES   = 0x1,
                   BATCH_MAX_PACKETS = 0x2,
                   BATCH_MAX_DELAY   = 0x4,
                   BATCH_MANUAL      = 0x8,
                   BATCH_ALL         = 0xFFFFFFFF
                  };

    DWORD dwFields; // bit flags indicating structure members in use
                    // comprised of or'ed ZoneNetBatchSettings::FIELD values

    DWORD dwMaxBytes;    // max number of bytes to queue before sending batch
    DWORD dwMaxPackets;  // max number of application packets to queue before sending batch
    DWORD dwMaxDelay;    // max number of milliseconds to hold packets before sending batch
    BOOL  bManual;       // packets will accumulate until SendBatch() is called
	BOOL  bSendPending;  // true if the caller has manually called sendbatch, but that sendbatch
						 // had to be delayed.  This could happen if one send was in the process of
						 // happening when SendBatch was called.
};


//
// ZoneNetPerfMonitorBin
//
// structure for maintain network perf data for round trip data exchanges
//
struct ZoneNetPacketMonitorBin
{
    DWORD cbSize;  // sizeof structure
    DWORD cbDataSent;
    DWORD cbDataRecv;
};

struct ZoneNetPacketMonitorTotals
{
    DWORD cbSize;  // sizeof structure
    DWORD dwPacketsDropped;
    DWORD dwPacketsSent;
    DWORD dwBatchesSent;
    DWORD dwBytesSent;
    DWORD dwPacketsReceived;
    DWORD dwBytesReceived;
};

struct ZoneNetEncryptionKey
{
	char key[8];	// key data
};

struct ZoneNetCompressionKey
{
	char key[8];	// key data
};

struct ZoneNetDecompressionKey
{
	char key[8];	// key data
};

#pragma pack (pop)

#define MAX_UDP_PACKET 2048

////////////////////////////////////////////////////////////////////////////////
//
// IZoneNet
//
// basic interface for controlling network connections
//
//
interface IZoneNet : public IUnknown
{
    //
    // Init
    //
    virtual HRESULT STDMETHODCALLTYPE Init() = 0;

    //
    // Close
    //
    virtual HRESULT STDMETHODCALLTYPE Close() = 0;

    //
    // SetSettings
    //
    virtual HRESULT STDMETHODCALLTYPE SetSettings( IN const ZoneNetSettings* pSettings ) = 0;

    //
    // GetSettings
    //
    virtual HRESULT STDMETHODCALLTYPE GetSettings( OUT ZoneNetSettings* pSettings ) = 0;


    //
    // CreateConnection creates a raw transport or connection object of the specified type
    //
    virtual HRESULT STDMETHODCALLTYPE CreateObject( IN CLSID clsid, IN REFIID riid, OUT LPVOID* ppObject ) = 0;

    //
    // Wait
    //
    // places the calling thread in an alertable state to handle network messages
    // for up to the given timeout period in milliseconds
    //
    virtual HRESULT STDMETHODCALLTYPE Wait( IN DWORD dwTimeout = INFINITE, IN DWORD dwReserved = 0) = 0;

    //
    // StopWaiting
    //
    // causes a premature timeout to wake up a thread blocked in Wait() or MsgWait()
    // and return
    //
    virtual HRESULT STDMETHODCALLTYPE StopWaiting() = 0;

    //
    // DisconnectAll
    //
    // disconnect all transports and connections associated with the IZoneNet
    //
    virtual HRESULT STDMETHODCALLTYPE DisconnectAll() = 0;

    //
    // QueueAPC
    //
	//  This method will add an APC to the ZoneNet queue.  The caller
	// provides a function pointer, pfn (which should be a pointer to the function
	// that they want called) and a pointer to a context, pContext, which will be passed
	// as a parameter to the pfn function that the user supplied.
	//  When the user calls IZoneNet::Wait and that thread begins to wait, the first
	// APC on the queue will be called, and that entry removed from the queue.  This
	// process repeats until the queue is empty or the caller calls IZoneNet::StopWaiting.
	//
    virtual HRESULT STDMETHODCALLTYPE QueueAPC( IN LPFN_APC_CALLBACK pfn, IN LPVOID pContext ) = 0;

    // Resolves the hostname passed in as a string.  Produces a ZoneNetAddress which is
    // passed to the callback function.  ZoneNetAddress is owned by ZoneNet object and
    // will be deleted after the callback has been made.  Callback is responsible for
    // making a copy of that object if they need one.
    virtual HRESULT STDMETHODCALLTYPE ResolveHostNameAsync( IN LPCSTR pszHost, IN unsigned short p, IN LPFN_GETADDR_CALLBACK pfn, IN LPVOID pContext ) = 0;

    // Resolve the hostname passed in as a string.  Populates the supplied ZoneNetAddress
    // object.  Caller must provide the ZoneNetAddress, and will retain ownership of it.
    virtual HRESULT STDMETHODCALLTYPE ResolveHostNameSync( IN LPCSTR pszHost, IN unsigned short p, IN OUT ZoneNetAddress *pZoneNetAddr ) = 0;
};


enum ZONE_NET_STATE    { NET_STATE_INVALID,
                         NET_STATE_INIT,
                         NET_STATE_LISTENING,
                         NET_STATE_CONNECTING,
                         NET_STATE_CONNECTED,
                         NET_STATE_CONNECT_FAILED,
                         NET_STATE_CLOSING,
                         NET_STATE_CLOSED };

////////////////////////////////////////////////////////////////////////////////
//
// IZoneNetTransport
//
//
//
interface IZoneNetTransport : public IUnknown
{
    /////////////////////////////////////////////////////////////
    //
    // General methods
    //

    //
    // GetNetwork
    //
    // returns the IZoneNet object on which the connection was created
    //
    virtual HRESULT STDMETHODCALLTYPE GetNetwork( OUT IZoneNet** ppNet ) = 0;

    //
    // SetContext
    //
    // sets a pointer to application defined data
    //
    virtual HRESULT STDMETHODCALLTYPE SetContext( IN LPVOID pContext ) = 0;

    //
    // GetContext
    //
    // gets a pointer to application defined data
    //
    virtual HRESULT STDMETHODCALLTYPE GetContext( OUT LPVOID* ppContext ) = 0;

    //
    // GetState
    //
    // returns the current status of the connection object
    //
    virtual HRESULT STDMETHODCALLTYPE GetState( OUT ZONE_NET_STATE* pState ) = 0;

    //
    // GetSocket
    //
    // retrieves the socket handle associated with the Transport
    //
    virtual HRESULT STDMETHODCALLTYPE GetSocket( OUT SOCKET* pSocket ) = 0;

    //
    // SetConnectedSocket
    //
    // associates a connected socket handle with the Transport
    //
    virtual HRESULT STDMETHODCALLTYPE SetConnectedSocket( IN SOCKET sConnection ) = 0;

    //
    // GetLocalAddress
    //
    // retrieves the local address of the Transport
    //
    virtual HRESULT STDMETHODCALLTYPE GetLocalAddress( OUT ZoneNetAddress* pAddr ) = 0;

    //
    // GetRemoteAddress
    //
    // retrieves the remote address of the established Transport
    //
    virtual HRESULT STDMETHODCALLTYPE GetRemoteAddress( OUT ZoneNetAddress* pAddr ) = 0;


    //
    // Close
    //
    // gracefully terminates the Transport
    //
    virtual HRESULT STDMETHODCALLTYPE Close() = 0;


    //
    // LPFN_CLOSE_CALLBACK
    //
    typedef void ( WINAPI *LPFN_CLOSE_CALLBACK)( IN IZoneNetTransport* pTransport,
                                                 IN DWORD dwError   // error code of close - 0 is locally issued Close()
                                               );


    //
    // LPFN_SEND_CALLBACK
    //
    // callback triggered on the transmission of data
    //
    typedef void ( WINAPI *LPFN_SEND_CALLBACK)( IN IZoneNetTransport* pTransport,
                                                IN LPBYTE pBuf,         // pointer to data sent
                                                IN DWORD  cbBuf,        // size of buffer pointed to by pBuf
                                                IN DWORD  cbSent        // number of bytes sent
                                              );

    //
    // LPFN_RECEIVE_CALLBACK
    //
    // callback triggered on the receipt of data
    //
    typedef void ( WINAPI *LPFN_RECEIVE_CALLBACK)( IN IZoneNetTransport* pTransport,
                                                   IN LPBYTE pBuf,         // pointer to data received
                                                   IN DWORD  cbBuf,        // size of buffer pointed to by pBuf
                                                   IN DWORD  cbReceived    // number of bytes received
                                                 );

    enum CALLBACK_TYPE { CALLBACK_CLOSE,
                         CALLBACK_SEND,
                         CALLBACK_RECEIVE,
                         CALLBACK_MAX = CALLBACK_RECEIVE
                         };

    //
    // SetCallback
    //
    virtual HRESULT STDMETHODCALLTYPE SetCallback( IN CALLBACK_TYPE type, IN FARPROC pfn ) = 0;

    //
    // GetCallback
    //
    virtual HRESULT STDMETHODCALLTYPE GetCallback( IN CALLBACK_TYPE type, OUT FARPROC* ppfn ) = 0;

    //
    // The cbMinSend and cbMinReceive parameters of Send() and Receive affect the frequency
    // of the Send or Receive callbacks being triggered
    // Setting the Min values to 0, put the transport into a stream mode.
    // The Send/Receive callbacks will be trigger upon each OS I/O completion.
    // Setting the Min values to non-0 numbers put the transport into a message mode.
    // The Send/Receive callbacks will be trigger after the min( cbMinXXX, cbData ) has been sent
    // which may span multiple OS I/O completions.
    //

    //
    // Send
    //
    // sends/queues data to sent over the network
    //
    virtual HRESULT STDMETHODCALLTYPE Send( IN const LPBYTE pData,
                                            IN DWORD cbData,
                                            IN DWORD cbMinSend = 0xFFFFFFFF
                                          ) = 0;

    //
    // Receive
    //
    // waits for data to be received from the network
    //
    virtual HRESULT STDMETHODCALLTYPE Receive( IN LPBYTE pData,
                                               IN DWORD  cbData,
                                               IN DWORD  cbMinReceive = 0xFFFFFFFF
                                             ) = 0;



    /////////////////////////////////////////////////////////////
    //
    // Client methods
    //

    //
    // Connect
    //
    // Connect is used by client applications to establish a connection with a particular server
    //
    typedef void ( WINAPI *LPFN_CONNECT_CALLBACK)( IN IZoneNetTransport* pTransport,
                                                   IN int wsaError,   // WinSock error code if underlying Connect() failed
                                                   IN LPVOID pContext
                                                 );

    virtual HRESULT STDMETHODCALLTYPE Connect( IN ZoneNetAddress* pRemote,
                                               IN ZoneNetAddress* pLocal,              // optional for TCP Transports
                                               IN LPFN_CONNECT_CALLBACK pfnConnect,
                                               IN LPVOID        pContext,
                                               IN BOOL          bAsync               // if TRUE, an additionally thread will be used to perform an async Transport
                                             ) = 0;


    //////////////////////////////////////////////////////////////
    //
    // Server methods
    //

    //
    // SetServerSettings
    //
    virtual HRESULT STDMETHODCALLTYPE SetServerSettings( IN const ZoneNetServerSettings* pSettings ) = 0;

    //
    // GetServerSettings
    //
    virtual HRESULT STDMETHODCALLTYPE GetServerSettings( OUT ZoneNetServerSettings* pSettings ) = 0;

    //
    // Listen
    //
    // Listen puts a connection in a "server" state so that it receives incoming
    // client Transports
    //
    typedef void ( WINAPI *LPFN_ACCEPT_CALLBACK)( IN IZoneNetTransport* pListeningTransport,
                                                  IN IZoneNetTransport* pAcceptedTransport,
                                                  IN LPVOID pContext
                                                );
    virtual HRESULT STDMETHODCALLTYPE Listen( IN const ZoneNetAddress* pLocal,
                                              IN LPFN_ACCEPT_CALLBACK pfnAccept,
                                              IN LPVOID pContext
                                            ) = 0;



};



////////////////////////////////////////////////////////////////////////////////
//
// IZoneNetPacketMonitor
//
// an interface which stores a histogram of the past network traffic
// for performance analysis
//
// note: array of bin is treated as a circular buffer, thus there should
// not be an assumption that the data is in a time sequenced order
//
interface IZoneNetPacketMonitor : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetBinCount( OUT DWORD *pdwBinCount ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetBins( IN ZoneNetPacketMonitorBin*   pBins, DWORD dwBinCount ) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetBins( OUT ZoneNetPacketMonitorBin** ppBins ) = 0;

    virtual HRESULT STDMETHODCALLTYPE ResetTotals() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTotals( OUT ZoneNetPacketMonitorTotals* pTotals ) = 0;

    virtual HRESULT STDMETHODCALLTYPE ReportSend( IN DWORD cbSent ) = 0;
    virtual HRESULT STDMETHODCALLTYPE ReportBatchSend() = 0;
    virtual HRESULT STDMETHODCALLTYPE ReportReceive( IN DWORD cbReceived) = 0;

    virtual HRESULT STDMETHODCALLTYPE ReportPacket(
                                   IN BOOL  bDropped,
                                   IN DWORD dwLocalSend,
                                   IN DWORD dwRemoteRecv,
                                   IN DWORD dwRemoteSend,
                                   IN DWORD dwLocalRecv,
                                   IN DWORD cbDataSent,
                                   IN DWORD cbDataRecv
                                ) = 0;

};


////////////////////////////////////////////////////////////////////////////////
//
// IZoneNetAuthentication
//
// an interface to a ZoneTech object which handles authentication
//
interface IZoneNetAuthentication : public IUnknown
{

    // yet to be defined

};


////////////////////////////////////////////////////////////////////////////////
//
// IZoneNetConnection
//
// an interface which simplifies and wraps the network layer abstraction classes
//
interface IZoneNetConnection : public IUnknown
{
    /////////////////////////////////////////////////////////////
    //
    // General methods
    //

    //
    // GetNetwork
    //
    // returns the IZoneNet object on which the connection was created
    //
    virtual HRESULT STDMETHODCALLTYPE GetNetwork( OUT IZoneNet** ppNet ) = 0;

    //
    // SetContext
    //
    // sets a pointer to application defined data
    //
    virtual HRESULT STDMETHODCALLTYPE SetContext( IN LPVOID pContext ) = 0;

    //
    // GetContext
    //
    // gets a pointer to application defined data
    //
    virtual HRESULT STDMETHODCALLTYPE GetContext( OUT LPVOID* ppContext ) = 0;

    //
    // GetState
    //
    // returns the current status of the connection object
    //
    virtual HRESULT STDMETHODCALLTYPE GetState( OUT ZONE_NET_STATE* pState ) = 0;

    //
    // GetLocalAddr
    //
    // retrieves the local address of the conn
    //
    virtual HRESULT STDMETHODCALLTYPE GetLocalAddress( OUT ZoneNetAddress* pAddr ) = 0;

    //
    // GetRemoteAddr
    //
    // retrieves the remote address of the established connection
    //
    virtual HRESULT STDMETHODCALLTYPE GetRemoteAddress( OUT ZoneNetAddress* pAddr ) = 0;


	//
	// SetMinReceiveBufferSize
	//
	// This optional method tells the IZoneNetConnection object the minimum allocated buffer
	// size that should be used for receiving data.  When set, it is guaranteed that
	// any receive buffer will be allocated with at least cbMinBufferSize bytes of
	// allocated memory.
	virtual HRESULT STDMETHODCALLTYPE SetMinReceiveBufferSize( IN const DWORD cbMinBufferSize ) = 0;

    //////////////////////////////////////////////////////////
    //
    // Batching
    //

    //
    // SetBatching
    //
    virtual HRESULT STDMETHODCALLTYPE SetBatching( IN const ZoneNetBatchSettings* pSettings ) = 0;

    //
    // GetBatching
    //
    virtual HRESULT STDMETHODCALLTYPE GetBatching( OUT ZoneNetBatchSettings* pSettings ) = 0;

    //
    // SendBatch
    //
    // manually forces a send to occur on the accumulated data in the batch
    //
    virtual HRESULT STDMETHODCALLTYPE SendBatch() = 0;


    //////////////////////////////////////////////////////////
    //
    // Heartbeat and Ping
    //

    //
    // SetHeartbeat
    //
    // establishes the delay between keep-alive heartbearts on the connection
    //
    virtual HRESULT STDMETHODCALLTYPE SetHeartbeat( IN DWORD dwDelay ) = 0;

    //
    // GetHeartbeat
    //
    // retreives the delay between keep-alive heartbearts on the connection
    //
    virtual HRESULT STDMETHODCALLTYPE GetHeartbeat( OUT DWORD* pdwDelay ) = 0;

    //
    // GetHeartbeatStatus
    //
    // retrieves the roundtrip elapsed time of the most recent heartbeat
    //
    virtual HRESULT STDMETHODCALLTYPE GetHeartbeatStatus( OUT DWORD* pdwRoundTripTime ) = 0;

    //
    // SendHeartbeat
    //
    // manually forces a heartbeat to be sent immediately
    //
    virtual HRESULT STDMETHODCALLTYPE SendHeartbeat() = 0;

    //
    // Ping
    //
    // sends a message to the remote peer which is echoed back.
    //
    virtual HRESULT STDMETHODCALLTYPE Ping( IN DWORD dwCount = 3,   // number of sends
                                            IN DWORD cbData = 32,   // size of data in each send
                                            IN LPBYTE pData = NULL  // optional point do data to send ( should be size of cbData )
                                          ) = 0;


    //////////////////////////////////////////////////////////
    //
    // Data transfer
    //

    //
    // LPFN_CLOSE_CALLBACK
    //
    typedef void ( WINAPI *LPFN_CLOSE_CALLBACK)( IN IZoneNetConnection* pConnection,
                                                 IN DWORD dwError   // error code of close - 0 is locally issued Close()
                                               );

    //
    // LPFN_RECEIVE_CALLBACK
    //
	// callback triggered on the receipt of data
    typedef void ( WINAPI *LPFN_RECEIVE_CALLBACK)( IN IZoneNetConnection* pConnection,
                                                   IN LPBYTE pData,
                                                   IN DWORD cbData
                                                 );

    typedef void ( WINAPI *LPFN_PING_CALLBACK)( IN IZoneNetConnection* pConnection,
                                                IN DWORD dwElapsedTime,
                                                IN LPVOID pContext);

    enum CALLBACK_TYPE { CALLBACK_CLOSE   = IZoneNetTransport::CALLBACK_CLOSE,
                         CALLBACK_RECEIVE = IZoneNetTransport::CALLBACK_RECEIVE,
                         CALLBACK_PING    = IZoneNetTransport::CALLBACK_MAX + 1
						};

    //
    // SetCallback
    //
    virtual HRESULT STDMETHODCALLTYPE SetCallback( IN CALLBACK_TYPE type, IN FARPROC pfn ) = 0;

    //
    // GetCallback
    //
    virtual HRESULT STDMETHODCALLTYPE GetCallback( IN CALLBACK_TYPE type, OUT FARPROC* ppfn ) = 0;

    //
    // Send
    //
    // note: underlying implementations guarantee sequencing of data
    //
    enum SEND_FLAGS   { SEND_DEFAULT         = 0x0,
                        SEND_ENCRYPT         = 0x1,
                        SEND_COMPRESS        = 0x2,
                        SEND_GUARANTEE       = 0x4,
                        SEND_STREAM          = 0x8,
                        SEND_PING            = 0x10,
                        SEND_PING_REPLY      = 0x20,
                        SEND_HEARTBEAT_NULL  = 0x40,
                        SEND_HEARTBEAT_REPLY = 0x80
                      };

    virtual HRESULT STDMETHODCALLTYPE Send( IN const LPBYTE pData,
                                            IN DWORD cbData,
                                            IN DWORD flags = SEND_DEFAULT
                                          ) = 0;

    //
    // Forward
    //
    // Forwards a received buffer to a different connection, optionally appending
    // additional data
    //
    virtual HRESULT STDMETHODCALLTYPE Forward( IN IZoneNetConnection* pReceivingConnection,
                                               IN LPVOID pReceivedData,
                                               IN DWORD cbReceivedData,
                                               IN DWORD flags,
                                               IN const LPVOID pDataToAppend,
                                               IN const DWORD cbDataToAppend ) = 0;

    //
    // Close
    //
    // gracefully terminates the connection
    //
    virtual HRESULT STDMETHODCALLTYPE Close() = 0;


    //////////////////////////////////////////////////////////
    //
    // Performance measurement
    //

    //
    // SetPacketMonitor
    //
    // setting the packet monitor to NULL or the number of packet monitor
    // bins to 0, turns off packet monitoring
    //
    virtual HRESULT STDMETHODCALLTYPE SetPacketMonitor( IN IZoneNetPacketMonitor* pMonitor ) = 0;

    //
    // GetPacketMonitor
    //
    virtual HRESULT STDMETHODCALLTYPE GetPacketMonitor( OUT IZoneNetPacketMonitor** ppMonitor ) = 0;

    //
    // LPFN_REMOTE_PACKET_MONITOR_CALLBACK
    //
    typedef void ( WINAPI *LPFN_REMOTE_PACKET_MONITOR_CALLBACK)
                                         ( IN IZoneNetConnection* pConnection,
                                           IN ZoneNetPacketMonitorBin* pBins,
                                           IN DWORD dwBinCount,
                                           IN LPVOID pContext
                                         );
    //
    // GetRemotePacketMonitorBins
    //
    // issues a network message requesting the remote peer to send a copy of
    // its PacketMonitorBin data, if any, to the local peer
    //
    virtual HRESULT STDMETHODCALLTYPE GetRemotePacketMonitorBins( OUT LPFN_REMOTE_PACKET_MONITOR_CALLBACK pfn, LPVOID pContext ) = 0;


    //////////////////////////////////////////////////////////////
    //
    // Authentication methods
    //

    //
    // SetAuthenticationHandler
    //
    virtual HRESULT STDMETHODCALLTYPE SetAuthenticationHandler( IN IZoneNetAuthentication* pAuth ) = 0;

    //
    // GetAuthenticationHandler
    //
    virtual HRESULT STDMETHODCALLTYPE GetAuthenticationHandler( OUT IZoneNetAuthentication** ppAuth ) = 0;

    //
    // GetUserName
    //
    // provided the connection is authenticated, the callee may retreive
    // the username associated with the connection
    //
    virtual HRESULT STDMETHODCALLTYPE GetUserName( OUT LPSTR pszUserName, IN DWORD cbUserName ) = 0;




    /////////////////////////////////////////////////////////////
    //
    // Client methods
    //

    //
    // Connect
    //
    // Connect is used by client applications to establish a connection with a particular server
    //
    typedef void ( WINAPI *LPFN_CONNECT_CALLBACK)( IN IZoneNetConnection* pConnection,
                                                   IN int wsaError,   // WinSock error code if underlying Connect() failed
                                                   IN LPVOID pContext
                                                 );

    virtual HRESULT STDMETHODCALLTYPE Connect(
                             IN ZoneNetAddress* pRemote,
                             IN ZoneNetAddress* pLocal,               // optional for TCP connections
                             IN LPFN_CONNECT_CALLBACK pfnConnect,
                             IN LPVOID        pContext,
                             IN BOOL          bAsync               // if TRUE, an additionally thread will be used to perform an async connection
                           ) = 0;


    //////////////////////////////////////////////////////////////
    //
    // Server methods
    //

    //
    // SetServerSettings
    //
    virtual HRESULT STDMETHODCALLTYPE SetServerSettings( IN const ZoneNetServerSettings* pSettings ) = 0;

    //
    // GetServerSettings
    //
    virtual HRESULT STDMETHODCALLTYPE GetServerSettings( OUT ZoneNetServerSettings* pSettings ) = 0;

    //
    // Listen
    //
    // Listen puts a connection in a "server" state so that it receives incoming
    // client connections
    //


    typedef void ( WINAPI *LPFN_ACCEPT_CALLBACK)( IN IZoneNetConnection* pListeningConnection,
                                                  IN IZoneNetConnection* pAcceptedConnection,
                                                  IN LPVOID pContext );
    virtual HRESULT STDMETHODCALLTYPE Listen(
                            IN const ZoneNetAddress* pLocal,
                            IN LPFN_ACCEPT_CALLBACK pfnAccept,
                            IN LPVOID pContext
                          ) = 0;


    //
    // Authenticate
    //
    // forces the connected client to (re)authenticate with the server
    //
    virtual HRESULT STDMETHODCALLTYPE Authenticate() = 0;

    enum LOGGING_TYPE { LOG_NONE      = 0x00,   // disable logging
                        LOG_ERRORS    = 0x01,   // logs all errors
                        LOG_VERBOSE   = 0x02,   // logs all user and internal function calls, all parameters, 
                                                // all function decisions, all errors
                        LOG_CONNECTION= 0x04,   // log all connection function calls
                        LOG_TRANSPORT = 0x08,   // log all transport function calls
                        LOG_NETWORK   = 0x10    // log all network function calls
                      };

    enum LOGGING_FORMAT {  LOGFORMAT_NONE       = 0x00,
                           LOGFORMAT_ADDRESS    = 0x01, // include the connection address in all output, useful if sharing
                                                    // a ISequentialStream object across multiple connections
                           LOGFORMAT_THREAD     = 0x02, // indent based on function calls (useful in LOG_SPARSE and LOG_VERBOSE)
                           LOGFORMAT_INDENT     = 0x04
                        };

    virtual void SetLogging(DWORD ltype, DWORD lformat, ISequentialStream *pStm) = 0;
};
*/

#endif // _ZONENET_H
