/*-------------------------------------------------------------------------
  C:\MYTHICA\Extern\GUN\include\GunNet.hpp
  
  Header to include for using GUNNet
  
  Owner: curtc
  
  Copyright 1986-2000 Microsoft Corporation, All Rights Reserved
 *-----------------------------------------------------------------------*/
#pragma once

#include <gunqueue.h>

namespace Gun2
{

// Forward decls of all interfaces 
interface IGUNNetQueue;
interface IGUNNetSendQueue;
interface IGUNNetPayloadQueue;
interface IGUNNetClient;
interface IGUNNetClientQueuedEvents;
interface IGUNNetRecipient;
interface IGUNNetConnection;
interface IGUNNetGroup;
interface IGUNNetServer;
interface IGUNNetServerQueuedEvents;
interface IGUNNetServerCallbackEvents;
interface IGUNNetPeer;
interface IGUNNetPeerQueuedEvents;
interface IGUNNetPeerCallbackEvents;
interface IGUNNetPayloadReader;
interface IGUNNetPayloadReaderEvents;

const GUNMSGID c_gmiReservedFirst = GUNMSGID_GunNet_First; // all app-defined events, if any, must be below this
// all GUN events id ranges need to be defined here, so as to have a single reference point of id ranges
const GUNMSGID c_gmiNetFirst  = c_gmiReservedFirst; // this is NOT the same id space that game message ids come from
const GUNMSGID c_gmiNetLast   = c_gmiReservedFirst + 199;

const GUNMSGID c_gmiNetAppMsg = GUNMSGID_GunNet_First + 0;
const GUNMSGID c_gmiNetSysMsg = GUNMSGID_GunNet_First + 1;


typedef unsigned short IB;  // index of bytes to var-length data from start of struct
typedef unsigned short CB;  // count of bytes 
typedef unsigned short GUNNETMSGID; // id of GUNNet message. This is VERY DIFFERENT form a GUNMSGID used in IGUNQueue!

typedef  DWORD  GUNHANDLE, *PGUNHANDLE;

enum GUNPRIORITY 
{
  GUN_PRIO_LOW, 
  GUN_PRIO_NORMAL, 
  GUN_PRIO_HIGH, 
  GUN_PRIO_ALL // only used for GetSendQueueInfo
}; 

enum GUNRELIABLE 
{
  GUN_RELIABLE, 
  GUN_NONRELIABLE, 
  GUN_NONRELIABLE_NOTIFY
};

enum GUNCOMPLETION 
{
  GUNCOMP_SUCCESS, 
  GUNCOMP_TIMEOUT, 
  GUNCOMP_NOCONNECTION,
  GUNCOMP_USERCANCEL
};

enum GUNREQUESTRESULT 
{
  GUNREQR_SUCCESS, 
  GUNREQR_TIMEOUT, 
  GUNREQR_DENIED
};

enum GUNDISCONNECT 
{
  GUNDISCON_SESSIONGONE, 
  GUNDISCON_SERVERBOOT, 
  GUNDISCON_CLIENTDISCONNECT, 
  GUNDISCON_CONNECTIONLOST
};

enum GUNDELETE_CLIENT_REASON 
{
  GUNDCR_CLIENTINITIATED, 
  GUNDCR_SERVERINITIATED, 
  GUNDCR_CONNECTIONLOST, 
  GUNDCR_SESSIONCLOSED
};

enum GUNPAUSE
{
  GUNPAUSE_START     = 1, // use this to start a pause
  GUNPAUSE_CONTINUE  = 2, // use this to continue after a pause
  GUNPAUSE_ABORT     = 3, // use this to tell clients session will not be continuing (so they can disconnect, which they still have to do manually)
  GUNPAUSE_MESSAGE   = 4  // use this to send a message during a pause (the only way you have to communicate during a pause)
};

struct GUNNetConnectionINFO
{
  DWORD dwSize;
  DWORD dwRoundTripLatencyMS;
  DWORD dwThroughputBPS;
  DWORD dwPeakThroughputBPS;

  DWORD dwBytesSentGuaranteed;
  DWORD dwPacketsSentGuaranteed;
  DWORD dwBytesSentNonGuaranteed;
  DWORD dwPacketsSentNonGuaranteed;

  DWORD dwBytesRetried;    // Guaranteed only
  DWORD dwPacketsRetried;  // Guaranteed only
  DWORD dwBytesDropped;    // Non Guaranteed only
  DWORD dwPacketsDropped;  // Non Guaranteed only

  DWORD dwMessagesTransmittedHighPriority;
  DWORD dwMessagesTimedOutHighPriority;
  DWORD dwMessagesTransmittedNormalPriority;
  DWORD dwMessagesTimedOutNormalPriority;
  DWORD dwMessagesTransmittedLowPriority;
  DWORD dwMessagesTimedOutLowPriority;

  DWORD dwBytesReceivedGuaranteed;
  DWORD dwPacketsReceivedGuaranteed;
  DWORD dwBytesReceivedNonGuaranteed;
  DWORD dwPacketsReceivedNonGuaranteed;
  DWORD dwMessagesReceived;
};

struct GUNIPADDRESS
{
  BYTE b1;
  BYTE b2;
  BYTE b3;
  BYTE b4;
  WORD port;
};

struct GUN_NET_ADAPTER
{
  WCHAR         wszName[128];
  GUNIPADDRESS  ipaddress;
  GUID          guid;
};

// All generated messages derive from this
class CGUNNetMessage
{
public:
  GUNNETMSGID  GetMessageID() {return m_msgid;}

private:
  CGUNNetMessage() {}
  
  GUNNETMSGID m_msgid;
};

struct GUNNET_VARPARM // only used by generated code to pack variable length fields
{
  IB    ibcbDst; // index into message where CB for item goes (an IB to a CB)
  PVOID pvSrc; // where to copy from
  CB    cbSrc; // how much to copy or reserve
};


// simple package to indicate the main object a message is sent to
enum GUNNET_OBJECT_TYPE {GUN_OBJECT_CLIENT, GUN_OBJECT_SERVER, GUN_OBJECT_PEER};
struct GUNNetObject
{
  GUNNET_OBJECT_TYPE objtype;
  union
  {
    IGUNNetClient * pClient;
    IGUNNetServer * pServer;
    IGUNNetPeer   * pPeer;
  };
};



// ---------------------------------------------------------------------
//                    Message-creation interfaces  
// ---------------------------------------------------------------------
interface IGUNNetQueue
{
friend class CGUNNetMessage;
// TODO: make private and create necessary proxy
  // don't use these directly. Use Create / Copy methods on messages instead.
  STDMETHOD(       QueueNewMessage)(CGUNNetMessage ** ppmsg, GUNNETMSGID msgid, CB cbFixed, GUNNET_VARPARM * rgVarParms, int cVarParms) = 0;
  STDMETHOD(       QueueExistingMessage)(CGUNNetMessage ** ppmsgNew, CGUNNetMessage * pmsgSrc, CB cbMsg) = 0;
public:
  // GetRecipient will always return either a group or a connection, as IGUNNetRecipient is a base interface only
  STDMETHOD_(IGUNNetRecipient *, GetRecipient)() = 0; 
  STDMETHOD_(CB,    GetTotalSize)() = 0; 
  STDMETHOD_(CB,    GetUsedSize)() = 0; 
  STDMETHOD_(CB,    GetFreeSize)() = 0;
};


interface IGUNNetSendQueue : public IGUNNetQueue
{
  // IGUNNetQueue
  STDMETHOD(       QueueNewMessage)(CGUNNetMessage ** ppmsg, GUNNETMSGID msgid, CB cbFixed, GUNNET_VARPARM * rgVarParms, int cVarParms) = 0; 
  STDMETHOD(       QueueExistingMessage)(CGUNNetMessage ** ppmsgNew, CGUNNetMessage * pmsgSrc, CB cbMsg) = 0;
  // GetRecipient will always return either a group or a connection, as IGUNNetRecipient is a base interface only
  STDMETHOD_(IGUNNetRecipient *, GetRecipient)() = 0; 
  STDMETHOD_(CB,    GetTotalSize)() = 0; 
  STDMETHOD_(CB,    GetUsedSize)() = 0; 
  STDMETHOD_(CB,    GetFreeSize)() = 0;

  // IGUNNetSendQueue
  // SetPriority/SetReliable/SetTimeout may only be called when the queue is empty--mainly just avoid bugs
  // However the settings persist through all subsequent buffers on the interface, so you don't need to call for every send
  STDMETHOD_(void,  SetPriority)(GUNPRIORITY prio) = 0; 
  STDMETHOD_(GUNPRIORITY, GetPriority)() = 0; 
  STDMETHOD_(void,  SetReliable)(GUNRELIABLE reliable) = 0; 
  STDMETHOD_(GUNRELIABLE, GetReliable)() = 0; 
  STDMETHOD_(void,  SetTimeout)(DWORD dwtimeout) = 0;
  STDMETHOD_(DWORD, GetTimeout)() = 0;
  STDMETHOD_(void,  SetSendContext)(PVOID pvContextSend) = 0;
  STDMETHOD_(PVOID, GetSendContext)() = 0;
  STDMETHOD(        Flush)() = 0;
  STDMETHOD(        Cancel)() = 0;
  STDMETHOD_(void,  SetAutoFlush)(DWORD dwAutoFlush) = 0; // 0 means never auto-flush
  STDMETHOD_(DWORD, GetAutoFlush)() = 0;
  STDMETHOD_(void,  SetSendComplete)(bool fSendComplete) = 0; // whether you want notification when send is completed
  STDMETHOD_(bool,  GetSendComplete)() = 0;
};


interface IGUNNetPayloadQueue : public IGUNNetQueue
{
  // IGUNNetQueue
  STDMETHOD(       QueueNewMessage)(CGUNNetMessage ** ppmsg, GUNNETMSGID msgid, CB cbFixed, GUNNET_VARPARM * rgVarParms, int cVarParms) = 0; 
  STDMETHOD(       QueueExistingMessage)(CGUNNetMessage ** ppmsgNew, CGUNNetMessage * pmsgSrc, CB cbMsg) = 0;
  // GetRecipient will always return either a group or a connection, as IGUNNetRecipient is a base interface only
  STDMETHOD_(IGUNNetRecipient *, GetRecipient)() = 0; 
  STDMETHOD_(CB,    GetTotalSize)() = 0; 
  STDMETHOD_(CB,    GetUsedSize)() = 0; 
  STDMETHOD_(CB,    GetFreeSize)() = 0;

  // IGUNNetPayloadQueue
  STDMETHOD(       SetPayloadSizeHint)(CB cbPredicted) = 0; // can only call before queuing any messages
  // Can only call CommitAndSend when CREATING a new event with a payload, NOT on a response to an event (will assert)
  STDMETHOD(       CommitAndSend)() = 0; 
};


interface IGUNNetPayloadReader // GUNNet implements this and give you one whenever you get a payload to process
{
  STDMETHOD_(void,  ProcessMessages)(IGUNNetPayloadReaderEvents * pEvents, PVOID pvContext) = 0;
};

interface IGUNNetPayloadReaderEvents // app must implement this
{
  STDMETHOD_(CB,    OnPayloadMessage)(CGUNNetMessage * pmsg, IGUNNetConnection * pcnxnFrom, GUNNetObject * pgnobjTo, CB cbRemaining, PVOID pvContext) = 0; // returns # of bytes consumes by message
};


// ---------------------------------------------------------------------
//                      Recipient interfaces
// ---------------------------------------------------------------------
enum GUNRECIPIENTTYPE 
{
  GUN_RECIP_CONNECTION, 
  GUN_RECIP_GROUP, 
  GUN_RECIP_SERVER, 
  GUN_RECIP_PEER, 
  GUN_RECIP_EVERYONE // that is, a recipient that represents all clients, so you don't have to maintain your own group for all clients
};

// An entity that you can send messages to
interface IGUNNetRecipient
{
  STDMETHOD_(IGUNNetSendQueue *, GetDefaultSendQueue)() = 0;
  STDMETHOD(        CreateAdditionalSendQueue)(OUT IGUNNetSendQueue ** ppSendQ) = 0;
  STDMETHOD_(void,  DeleteAdditionalSendQueue)(    IGUNNetSendQueue *   pSendQ) = 0;
  STDMETHOD_(PVOID, GetContext)() = 0;
  STDMETHOD_(GUNRECIPIENTTYPE, GetRecipientType)() = 0;
  STDMETHOD_(DWORD, GetID)() = 0;
};

// A single client connection
interface IGUNNetConnection : public IGUNNetRecipient
{
  // IGUNNetRecipient
  STDMETHOD_(IGUNNetSendQueue *, GetDefaultSendQueue)() = 0;
  STDMETHOD(        CreateAdditionalSendQueue)(OUT IGUNNetSendQueue ** ppSendQ) = 0;
  STDMETHOD_(void,  DeleteAdditionalSendQueue)(   IGUNNetSendQueue *   pSendQ) = 0;
  STDMETHOD_(PVOID, GetContext)() = 0;
  STDMETHOD_(GUNRECIPIENTTYPE, GetRecipientType)() = 0;
  STDMETHOD_(DWORD, GetID)() = 0;
  
  // IGUNNetConnection
  STDMETHOD(        GetConnectionInfo)(OUT GUNNetConnectionINFO * pConnectionInfo) = 0;
  STDMETHOD(        GetConnectionAddress)(OUT GUNIPADDRESS * ipAddress) = 0;
  STDMETHOD(        Disconnect)(OUT IGUNNetPayloadQueue ** ppPayloadQueue) = 0;
  STDMETHOD(        GetSendQueueInfo)(DWORD *const pdwNumMsgs, DWORD *const pdwNumBytes, GUNPRIORITY  prio) = 0;
};

// Zero or more client connections
interface IGUNNetGroup : public IGUNNetRecipient
{
  // IGUNNetRecipient
  STDMETHOD_(IGUNNetSendQueue *, GetDefaultSendQueue)() = 0;
  STDMETHOD(        CreateAdditionalSendQueue)(OUT IGUNNetSendQueue ** ppSendQ) = 0;
  STDMETHOD_(void,  DeleteAdditionalSendQueue)(   IGUNNetSendQueue *   pSendQ) = 0;
  STDMETHOD_(PVOID, GetContext)() = 0;
  STDMETHOD_(GUNRECIPIENTTYPE, GetRecipientType)() = 0;
  STDMETHOD_(DWORD, GetID)() = 0;
  
  // IGUNNetGroup
  STDMETHOD_(int,   GetCountConnections)() = 0;
  STDMETHOD(        AddConnection)(IGUNNetConnection * pCnxn) = 0;
  STDMETHOD(        DeleteConnection)(IGUNNetConnection * pCnxn) = 0;
  // In EnumConnections, CALLER must free prgCnxns, using the correct allocator for the server/peer
  STDMETHOD(        EnumConnections)(OUT IGUNNetConnection *** prgpCnxns, OUT int * pccnxns) = 0;
  STDMETHOD_(void,  Destroy)() = 0; // destroys group AND interface
};


// ---------------------------------------------------------------------
//                      Client interfaces
// ---------------------------------------------------------------------
interface IGUNNetClient : public IGUNNetConnection
{
  // IGUNNetRecipient
  STDMETHOD_(IGUNNetSendQueue *, GetDefaultSendQueue)() = 0;
  STDMETHOD(        CreateAdditionalSendQueue)(OUT IGUNNetSendQueue ** ppSendQ) = 0;
  STDMETHOD_(void,  DeleteAdditionalSendQueue)(   IGUNNetSendQueue *   pSendQ) = 0;
  STDMETHOD_(PVOID, GetContext)() = 0;
  
  // IGUNNetConnection
  STDMETHOD(        GetConnectionInfo)(OUT GUNNetConnectionINFO * pConnectionInfo) = 0;
  STDMETHOD(        GetConnectionAddress)(OUT GUNIPADDRESS * ipAddress) = 0;
  STDMETHOD(        Disconnect)(OUT IGUNNetPayloadQueue ** ppPayloadQueue) = 0;
  STDMETHOD(        GetSendQueueInfo)(DWORD *const pdwNumMsgs, DWORD *const pdwNumBytes, GUNPRIORITY  prio) = 0;

  // IGUNNetClient
  STDMETHOD(        ProcessEvent)(GUNQueueMessage * pmsg, PVOID pvContext) = 0;
  STDMETHOD(        StartFindSessions)(char *  szServer, REFGUID guidApp, OUT IGUNNetPayloadQueue ** ppPayloadQueue,
                                PVOID pvFindSessionContext, OUT GUNHANDLE * pHandle) = 0;
  STDMETHOD(        StopFindingSessions)(GUNHANDLE handle) = 0;
  // If you pass a server name to Connect, only the port in addr is used (i.e. b1-b4 is ignored)
  STDMETHOD(        Connect)(char * szServer, GUNIPADDRESS addr, REFGUID guidApp, REFGUID guidInstance,
                                OUT IGUNNetPayloadQueue ** ppPayloadQueue, OUT GUNHANDLE * pHandle) = 0;
  STDMETHOD_(void,  Delete)() = 0; // this interface is not addref'd
  STDMETHOD(        CancelSend)(GUNHANDLE handle) = 0;
  STDMETHOD(        CancelConnect)(GUNHANDLE handle) = 0;
  STDMETHOD_(void,  SetContext)(PVOID pvContext) = 0;
};


interface IGUNNetClientQueuedEvents
{
  // Only event app is required to implement
  STDMETHOD_(CB,    OnAppMessage)(CGUNNetMessage * pmsg, IGUNNetConnection * pcnxnFrom, GUNNetObject * pgnobjTo, CB cbRemaining, PVOID pvContext, DWORD timeReceived) = 0; // returns # of bytes consumes by message
  STDMETHOD_(void,  OnConnectComplete)(GUNHANDLE handle, GUNREQUESTRESULT reqresult, 
                                IGUNNetPayloadReader * ppayloadreader, PVOID pvContext) 
    {}
  STDMETHOD_(void,  OnSessionFound)(GUNIPADDRESS ipAddressServer, 
                                PVOID pvFindSessionsContext, IGUNNetPayloadReader * ppayloadreader, PWSTR wszInstanceName, 
                                GUID guidInstance, GUID guidApp, DWORD dwTimeRoundtrip, PVOID pvContext) 
    {}
  // OnPreSend lets you modify the data buffer that's about to be sent. You can modify the data in place 
  // (if the modified size is equal to or smaller than cbBuffTotal), or allocate your own send buffer
  // and set *pcbBuffUsed to new buffer size. New buffer MUST be allocated with allocator passed into 
  // IGUNNetClient::Init, since IGUNNetClient will automatically free the buffer when the send is complete.
  // OnPreSend is NOT queued. You must handle in thread-safe way.
  STDMETHOD_(void,  OnPreSend)(IGUNNetSendQueue * pSendQueue, OUT GUNHANDLE ** ppHandle, 
                                IN OUT PBYTE * ppBuff, IN OUT CB * pcbBuffUsed, CB cbBuffTotal, IGUNNetClient * pclient) 
    {}
  STDMETHOD_(bool,  OnPreReceive)(IN OUT PBYTE * ppBuff, IN OUT CB * pcbBuff, PVOID pvContext) 
    {return true;}
  STDMETHOD_(void,  OnBadPacket)(IGUNNetConnection * pConnection, IN OUT PBYTE * ppBuff, 
                                OUT CB * pcbBuff, PVOID pvContext) 
    {}
  STDMETHOD_(void,  OnSendComplete)(GUNCOMPLETION completion, GUNHANDLE handle, 
                                DWORD timeToSend, PVOID pvSendQContext, PVOID pvContext)
    {}
  STDMETHOD_(void,  OnDisconnected)(GUNDISCONNECT disconnect, IGUNNetPayloadReader * ppayloadreader, PVOID pvContext) 
    {}
  STDMETHOD_(void,  OnSessionPause)(GUNPAUSE pause, LPCWSTR wszMessage, IGUNNetPayloadReader * ppayloadreader)
    {}
  STDMETHOD_(void,  OnClockSync)(DWORD dwServerTicks)
    {}
};


// ---------------------------------------------------------------------
//                        Server interfaces
// ---------------------------------------------------------------------
interface IGUNNetServer
{
  STDMETHOD(        Host)(PWSTR wszInstanceName, bool fDirectConnect ,REFGUID guidApp, OUT GUID & guidInstance) = 0;
  STDMETHOD(        Stop)() = 0;
  STDMETHOD_(void,  Delete)() = 0; // this interface is not addref'd
  STDMETHOD_(DWORD, GetCountQueuedEvents)() = 0;
  STDMETHOD(        ProcessEvent)(GUNQueueMessage * pmsg, IGUNNetServerQueuedEvents * pEvents, PVOID pvContext) = 0;
  STDMETHOD(        CancelSend)(GUNHANDLE handle) = 0;
  
  // In EnumConnections, CALLER must free prgCnxns, using the correct allocator for the server
  STDMETHOD(        EnumConnections)(OUT IGUNNetConnection *** prgpCnxns, int * pccnxns) = 0;
  STDMETHOD(        CreateGroup)(PWSTR wszGroupName, IGUNQueue * pqueueEvents, PVOID pvContext, OUT IGUNNetGroup ** ppGroup) = 0;
  STDMETHOD(        GetConnectionInfo)(OUT GUNNetConnectionINFO * pConnectionInfo) = 0;
  STDMETHOD_(DWORD, GetCountReceiveThreads)() = 0;
  STDMETHOD(        SetCountReceiveThreads)(DWORD cThreads) = 0; // can only INCREASE # of threads
  // In EnumNetAdapters, CALLER must free prgNetAdapters, using the correct allocator for the server
  // Obviously the port in the ip address you get back is meaningless here
  STDMETHOD(        EnumNetAdapters)(GUN_NET_ADAPTER ** prgNetAdapters, int * pcNetAdapters) = 0;
  // AddNetAdapter is very flexible. You can specify just the port, just the adapter, or both. When specifying the adapter,
  // you can do in one of two ways: by guid, or ip address, checked in that order (for non-zero). 
  // The name portion of the adapter is ignored.
  // Obviously you can just pass in selected values you get back from EnumNetAdapters, and only adjusting the port, for example.
  STDMETHOD(        AddNetAdapter)(GUN_NET_ADAPTER * pNetAdapter) = 0;
  STDMETHOD_(IGUNNetConnection *, GetConnection)(DWORD id) = 0;
  STDMETHOD_(PVOID, GetContext)() = 0;
  STDMETHOD_(void,  SetContext)(PVOID pvContext) = 0;
  STDMETHOD(        PauseSession)(GUNPAUSE pause, LPCWSTR wszMessage, OUT IGUNNetPayloadQueue ** ppPayloadQueue) = 0; 
  STDMETHOD_(IGUNNetRecipient *, GetEveryone)() = 0; // use to send to everyone
  STDMETHOD_(LONG,  GetMaxReceiveThreadsUsed)() = 0;
};


interface IGUNNetServerCallbackEvents
{
  STDMETHOD_(void,  OnPreSend)(IGUNNetSendQueue * pSendQueue, OUT GUNHANDLE ** ppHandle, 
                                IN OUT PBYTE * ppBuff, IN OUT CB * pcbBuffUsed, CB cbBuffTotal, IGUNNetServer * pserver) 
    {}
  // OnFindSessionRequest returns whether connection can see session
  // If you refuse to let the connection see the session, they you can't also send a response, since there IS no response if you refuse to see session.
  // pPayloadQueue is only valid until the function returns
  STDMETHOD_(bool,  OnFindSessionRequest)(GUNIPADDRESS ipAddressClient, IGUNNetPayloadReader * ppayloadreader, 
                                IGUNNetPayloadQueue * pPayloadQResponse, IGUNNetServer * pserver) 
    {return true;}
  // OnConnectRequest returns whether connection is allowed
  // pPayloadQueue is only valid until the function returns
  STDMETHOD_(bool,  OnConnectRequest)(GUNIPADDRESS ipAddressClient, IGUNNetPayloadReader * ppayloadreader, 
                                IGUNNetPayloadQueue * pPayloadQResponse, OUT PVOID * ppvConnectContext,
                                IGUNNetServer * pserver) 
    {return true;}
  // Only need to handle OnNewConnectionFirstChance if you want to divert future events about this connection to non-default queue
  STDMETHOD_(void,  OnNewConnectionFirstChance)(IGUNNetConnection * pConnection, OUT PVOID * ppvConnectContext,
                                OUT IGUNQueue ** ppqueue, IGUNNetServer * pserver) 
    {}
  STDMETHOD_(void,  OnConnectRequestAborted)(PVOID pvConnectContext, IGUNNetServer * pserver) 
    {}
};


interface IGUNNetServerQueuedEvents
{
  // events that pass pvContext are fired in app's thread in response to app calling ProcessEvent.
  // events that pass IGUNNetServer* are fired in random thread that app doesn't own and must be able to deal
  STDMETHOD_(CB,    OnAppMessage)(CGUNNetMessage * pmsg, IGUNNetConnection * pcnxnFrom, GUNNetObject * pgnobjTo, CB cbRemaining, PVOID pvContext, DWORD timeReceived) = 0;

  // In OnSendComplete, pRecipient will always be either a group or a connection, as IGUNNetRecipient is a base interface only
  STDMETHOD_(void,  OnSendComplete)(IGUNNetRecipient * pRecipient, GUNCOMPLETION completion, GUNHANDLE handle, 
                                DWORD timeToSend, PVOID pvSendQContext, PVOID pvContext) 
    {}
  // OnPreReceive returns whether to continue with processing contents
  STDMETHOD_(bool,  OnPreReceive)(IGUNNetConnection * pConnection, IN OUT PBYTE * ppBuff, 
                                IN OUT CB * pcbBuff, PVOID pvContext) 
    {return true;}
  STDMETHOD_(void,  OnBadPacket)(IGUNNetConnection * pConnection, IN OUT PBYTE * ppBuff, 
                                OUT CB * pcbBuff, PVOID pvContext) 
    {}
  STDMETHOD_(void,  OnNewConnection)(IGUNNetConnection * pConnection, OUT PVOID * ppvConnectContext, PVOID pvContext) 
    {}
  STDMETHOD_(void,  OnDeleteConnection)(IGUNNetConnection * pConnection, GUNDELETE_CLIENT_REASON dcr, PVOID pvContext) 
    {}
};


// ---------------------------------------------------------------------
//                      Peer interfaces
// ---------------------------------------------------------------------
interface IGUNNetPeer
{
  STDMETHOD(        Host)(PWSTR wszInstanceName, REFGUID guidApp, OUT GUID & guidInstance) = 0;
  STDMETHOD(        Stop)() = 0;

  STDMETHOD(        StartFindSessions)(char * szHost, REFGUID guidApp, OUT IGUNNetPayloadQueue ** ppPayloadQueue,
                                PVOID pvFindSessionContext, OUT GUNHANDLE * pHandle) = 0;
  STDMETHOD(        StopFindingSessions)(GUNHANDLE handle) = 0;
  STDMETHOD(        Connect)(char * szHost, GUNIPADDRESS addr, REFGUID guidApp, REFGUID guidInstance,
                                OUT IGUNNetPayloadQueue ** ppPayloadQueue, OUT GUNHANDLE * pHandle) = 0;
  STDMETHOD(        Disconnect)(OUT IGUNNetPayloadQueue ** ppPayloadQueue) = 0;
  STDMETHOD_(void,  Delete)() = 0; // this interface is not addref'd

  STDMETHOD(        CancelSend)(GUNHANDLE handle) = 0;
  STDMETHOD(        CancelConnect)(GUNHANDLE handle) = 0;
  STDMETHOD(        GetConnectionInfo)(OUT GUNNetConnectionINFO * pConnectionInfo) = 0;
  STDMETHOD_(DWORD, GetCountQueuedEvents)() = 0;
  STDMETHOD(        ProcessEvent)(GUNQueueMessage * pmsg, IGUNNetPeerQueuedEvents * pEvents, PVOID pvContext) = 0;
  // In EnumConnections, CALLER must free prgCnxns, using the correct allocator for the Peer
  STDMETHOD(        EnumConnections)(OUT IGUNNetConnection *** prgpCnxns, int * pccnxns) = 0;
  STDMETHOD_(DWORD, GetCountReceiveThreads)() = 0;
  STDMETHOD(        SetCountReceiveThreads)() = 0; // can only INCREASE # of threads
  // In EnumNetAdapters, CALLER must free prgNetAdapters, using the correct allocator for the Peer
  // Obviously the port in the ip address you get back is meaningless here
  STDMETHOD(        EnumNetAdapters)(GUN_NET_ADAPTER ** prgNetAdapters, int * pcNetAdapters) = 0;
  // AddNetAdapter is very flexible. You can specify just the port, just the adapter, or both. When specifying the adapter,
  // you can do in one of two ways: by guid, or ip address, checked in that order (for non-zero). 
  // The name portion of the adapter is ignored.
  // Obviously you can just pass in selected values you get back from EnumNetAdapters, and only adjusting the port, for example.
  STDMETHOD(        AddNetAdapter)(GUN_NET_ADAPTER * pNetAdapter) = 0;
  STDMETHOD_(IGUNNetConnection *, GetConnection)(DWORD id) = 0;
  STDMETHOD_(DWORD, GetID)() = 0;
  STDMETHOD_(bool,  IsHost)() = 0;
  STDMETHOD(        PauseSession)(GUNPAUSE pause, LPCWSTR wszMessage, OUT IGUNNetPayloadQueue ** ppPayloadQueue) = 0; // from host only
  STDMETHOD_(IGUNNetRecipient *, GetEveryone)() = 0; // use to send to everyone
};


interface IGUNNetPeerCallbackEvents
{
  STDMETHOD_(void,  OnPreSend)(IGUNNetSendQueue * pSendQueue, OUT GUNHANDLE ** ppHandle, 
                                IN OUT PBYTE * ppBuff, IN OUT CB * pcbBuffUsed, CB cbBuffTotal, IGUNNetPeer * pPeer) 
    {}
  // OnFindSessionRequest returns whether connection can see session
  // If you refuse to let the connection see the session, they you can't also send a response, since there IS no response if you refuse to see session.
  // pPayloadQueue is only valid until the function returns
  STDMETHOD_(bool,  OnFindSessionRequest)(GUNIPADDRESS ipAddressClient, IGUNNetPayloadReader * ppayloadreader, 
                                IGUNNetPayloadQueue * pPayloadQResponse, IGUNNetPeer * pPeer) 
    {return true;}
  // OnConnectRequest returns whether connection is allowed
  // pPayloadQueue is only valid until the function returns
  STDMETHOD_(bool,  OnConnectRequest)(GUNIPADDRESS ipAddressClient, IGUNNetPayloadReader * ppayloadreader, 
                                IGUNNetPayloadQueue * pPayloadQResponse, OUT PVOID * ppvConnectContext,
                                IGUNNetPeer * pPeer) 
    {return true;}
  // Only need to handle OnNewConnectionFirstChance if you want to divert future events about this connection to non-default queue
  STDMETHOD_(void,  OnNewConnectionFirstChance)(IGUNNetConnection * pConnection, OUT PVOID * ppvConnectContext,
                                OUT IGUNQueue ** ppqueue, IGUNNetPeer * pPeer) 
    {}
  STDMETHOD_(void,  OnConnectRequestAborted)(PVOID pvConnectContext, IGUNNetPeer * pPeer) 
    {}
  STDMETHOD_(void,  OnClockSync)(DWORD dwServerTicks)
    {}
};


interface IGUNNetPeerQueuedEvents
{
  // events that pass pvContext are fired in app's thread in response to app calling ProcessEvent.
  // events that pass IGUNNetPeer* are fired in random thread that app doesn't own and must be able to deal
  STDMETHOD_(CB,    OnAppMessage)(CGUNNetMessage * pmsg, IGUNNetConnection * pcnxnFrom, GUNNetObject * pgnobjTo, CB cbRemaining, PVOID pvContext, DWORD timeReceived) = 0;

  // In OnSendComplete, pRecipient will always be either a group or a connection, as IGUNNetRecipient is a base interface only
  STDMETHOD_(void,  OnSendComplete)(IGUNNetRecipient * pRecipient, GUNCOMPLETION completion, GUNHANDLE handle, 
                                DWORD timeToSend, PVOID pvSendQContext, PVOID pvContext) 
    {}
  // OnPreReceive returns whether to continue with processing contents
  STDMETHOD_(bool,  OnPreReceive)(IGUNNetConnection * pConnection, IN OUT PBYTE * ppBuff, 
                                IN OUT CB * pcbBuff, PVOID pvContext) 
    {return true;}
  STDMETHOD_(void,  OnBadPacket)(IGUNNetConnection * pConnection, IN OUT PBYTE * ppBuff, 
                                OUT CB * pcbBuff, PVOID pvContext) 
    {}
  STDMETHOD_(void,  OnNewConnection)(IGUNNetConnection * pConnection, OUT PVOID * ppvConnectContext, PVOID pvContext) 
    {}
  STDMETHOD_(void,  OnDeleteConnection)(IGUNNetConnection * pConnection, GUNDELETE_CLIENT_REASON dcr, PVOID pvContext) 
    {}
  STDMETHOD_(void,  OnConnectComplete)(GUNHANDLE handle, GUNREQUESTRESULT reqresult, 
                                IGUNNetPayloadReader * ppayloadreader, PVOID pvContext) 
    {}
  STDMETHOD_(void,  OnSessionFound)(GUNIPADDRESS ipAddressHost, 
                                PVOID pvFindSessionsContext, IGUNNetPayloadReader * ppayloadreader, PWSTR wszInstanceName, 
                                GUID guidInstance, GUID guidApp, DWORD dwTimeRoundtrip, PVOID pvContext) 
    {}
  STDMETHOD_(void,  OnDisconnected)(GUNDISCONNECT disconnect, IGUNNetPayloadReader * ppayloadreader, PVOID pvContext) 
    {}
  STDMETHOD_(void,  OnNewHost)(IGUNNetConnection * pConnection) // if pConnection==NULL that means the local client is the new host
    {}
  STDMETHOD_(void,  OnSessionPause)(GUNPAUSE pause, LPCWSTR wszMessage, IGUNNetPayloadReader * ppayloadreader)
    {}
};



// ---------------------------------------------------------------------
//                    Object creation functions
// ---------------------------------------------------------------------

// This is what you call to get your initial client
STDAPI CreateGUNNetClient(IGUNNetClient ** ppClient, IGUNAllocator * pAllocator,
                           IGUNNetClientQueuedEvents * pEvents, IGUNQueue * pqueSystem);

// This is what you call to get your initial server
STDAPI CreateGUNNetServer(IGUNNetServer ** ppServer, IGUNAllocator * pAllocator,
                           IGUNNetServerCallbackEvents * pEvents, IGUNQueue * pqueSystem);

// This is what you call to get your initial peer
STDAPI CreateGUNNetPeer(IGUNNetPeer ** ppPeer, IGUNAllocator * pAllocator,
                           IGUNNetPeerCallbackEvents * pEvents, IGUNQueue * pqueSystem);


// ---------------------------------------------------------------------
//     Helper macros to make generated protocol headers look nicer
//
//     Note, you can generate the actual code instead of the macros by putting NOMACROS before any messages are defined
// ---------------------------------------
#define _GUN_CREATE_IMPL {return pnetqueue ? \
        pnetqueue->QueueNewMessage((CGUNNetMessage**) ppmsg, GNMI, sizeof(MSG), NULL, 0) : E_INVALIDARG;}

#define _GUN_CREATE_IMPL_VAR_START(cvarparms) \
    {GUNNET_VARPARM rgVarParm[cvarparms]; int i = 0;
#define _GUN_CREATE_IMPL_VAR(name) \
    rgVarParm[i].ibcbDst = (IB)(&((MSG*)0)->cb##name); \
    rgVarParm[i].pvSrc = (PVOID) name; \
    rgVarParm[i].cbSrc = (CB)(sizeof(name[0]) * c##name); \
    i++;
#define _GUN_CREATE_IMPL_VAR_END return pnetqueue ? \
        pnetqueue->QueueNewMessage((CGUNNetMessage**) ppmsg, GNMI, sizeof(MSG), rgVarParm, sizeof(rgVarParm)/sizeof(rgVarParm[0])) : E_INVALIDARG;}
#define _GUN_VAR_PARM(x) CB cb##x;
//#define _GUN_GET_VAR_IMPL(t, n) {if (pCount) *pCount = cb##n / sizeof(t); return (t *) (PBYTE*)this + ib##n;}

#define _GUN_COPY_IMPL  {return pnetqueue->QueueExistingMessage((CGUNNetMessage**) ppmsg, this, GetMessageSize());}

#define GUN_MSG_DISPATCH(T) \
  case MSGID_##T: \
  { \
    MSG_##T * pmsg##T = (MSG_##T *) pmsg; \
    pmsg##T->OnReceive(pcnxnFrom, pgnobjTo, pvContext); \
    cb = pmsg##T->GetMessageSize(); \
    break; \
  }

HRESULT ConvertStrToIP(LPCSTR szHostName, OUT GUNIPADDRESS * pipAddress);

} // namespace Gun2
