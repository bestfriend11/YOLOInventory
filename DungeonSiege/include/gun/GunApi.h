//
//  Copyright (c) 2001, Microsoft Corp. All rights reserved.
//
#ifndef GUN2API_HEADER_H__
#define GUN2API_HEADER_H__

#include <GunErrors.h>

#pragma once

interface __declspec( 
        uuid( "EEEEEE21-85EA-40f7-8157-CD07A138B49F" ) ) IGunConnection;
interface __declspec( 
        uuid( "EEEEEE22-85EA-40f7-8157-CD07A138B49F" ) ) IGunNews;
interface __declspec( 
        uuid( "EEEEEE23-85EA-40f7-8157-CD07A138B49F" ) ) IGunAccount;
interface __declspec( 
        uuid( "EEEEEE24-85EA-40f7-8157-CD07A138B49F" ) ) IGunFriends;
interface __declspec( 
        uuid( "EEEEEE25-85EA-40f7-8157-CD07A138B49F" ) ) IGunChat;
interface __declspec( 
        uuid( "EEEEEE26-85EA-40f7-8157-CD07A138B49F" ) ) IGunHost;
interface __declspec( 
        uuid( "EEEEEE27-85EA-40f7-8157-CD07A138B49F" ) ) IGunBrowser;
interface __declspec( 
        uuid( "EEEEEE28-85EA-40f7-8157-CD07A138B49F" ) ) IGunRowManager;

#ifdef PSTATS_SAMPLE
interface __declspec( 
        uuid( "264C4126-6DC2-475c-A92F-85B88A22DB66" ) ) IGunPStats; // sample interface
#endif

class __declspec( 
        uuid( "76633332-52A2-4460-B089-3541A8443C9A" ) ) CGunObject;


// Also allow the old name style.
//
#define CLSID_GunObject     __uuidof( CGunObject )
#define IID_IGunConnection  __uuidof( IGunConnection )
#define IID_IGunNews        __uuidof( IGunNews )
#define IID_IGunAccount     __uuidof( IGunAccount )
#define IID_IGunFriends     __uuidof( IGunFriends )
#define IID_IGunChat        __uuidof( IGunChat )
#define IID_IGunHost        __uuidof( IGunHost )
#define IID_IGunBrowser     __uuidof( IGunBrowser )
#define IID_IGunRowManager  __uuidof( IGunRowManager )

#ifdef PSTATS_SAMPLE
#define IID_IGunPStats		__uuidof( IGunPStats )  //sample interface
#endif

namespace Gun2
{
    interface IGUNQueueRead;
}

STDAPI CreateGunApiObject( IUnknown **ppUnk );

//////////////////////////////////////////////////////////////////////////////
// IGunConnection
//
interface IGunConnection: public IUnknown
{
    enum ConnectStatus { CREATED, CONNECTING, CONNECTED,
                         DISCONNECTED, FAILED };

    virtual STDMETHODIMP Init(
                IN const GUID& guidApplication,
                IN LPCWSTR wszGameVersion,
                IN DWORD Locale,
                IN LPCWSTR wszCdKey )=0;

    virtual STDMETHODIMP Connect(
                IN LPCSTR pszServerName,
                IN unsigned short wPort )=0;

    virtual STDMETHODIMP Disconnect()=0;

    virtual STDMETHODIMP GetStatus(
                OUT ConnectStatus *pStatus )=0;

    virtual STDMETHODIMP GetLastError()=0;

    virtual STDMETHODIMP DispatchQueue( Gun2::IGUNQueueRead * pClientDispatch ) = 0;

};


//////////////////////////////////////////////////////////////////////////////
// IGunNews
//
interface IGunNews: public IUnknown
{
    virtual STDMETHODIMP GetNews(
                IN LPCSTR pszServerName,
                IN LPCSTR pszObjectname,
                IN unsigned short wPort,
                IN PVOID pvContext )=0;
};


//////////////////////////////////////////////////////////////////////////////
// IGunAccount
//
interface IGunAccount: public IUnknown
{
    virtual STDMETHODIMP LoginUser(
                IN LPCWSTR wszUserName,
                IN LPCWSTR wszPassword,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP LogoutUser( IN PVOID pvContext )=0;

    virtual STDMETHODIMP CreateAccount(
                IN LPCWSTR wszUserName,
                IN LPCWSTR wszPassword,
                IN LPCWSTR wszQuestion,
                IN LPCWSTR wszAnswer,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP ChangePassword(
                IN LPCWSTR wszOldPassword,
                IN LPCWSTR wszNewPassword,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP ChangePasswordFromHint(
                IN LPCWSTR wszUserName,
                IN LPCWSTR wszHintAnswer,
                IN LPCWSTR wszNewPassword,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP GetAccountStatus(
                IN LPCWSTR wszUserName,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP GetPasswordHint(
                IN LPCWSTR wszUserName,
                IN PVOID pvContext )=0;
};


//////////////////////////////////////////////////////////////////////////////
// IGunFriends
//
interface IGunFriends: public IUnknown
{
    enum ControlFlags {
                  NORMAL =0x00,
        ASK_BEFORE_ACCEPT=0x01,
              NO_FRIENDS =0x02
    };

    enum StateFlags {
               LOGGED_OUT =0x00,
                LOGGED_IN =0x01,
        AWAY_FROM_KEYBOARD=0x02,
           DO_NOT_DISTURB =0x04,
                  IN_GAME =0x08,
                  IN_CHAT =0x10
    };

    enum FLAG_OPERATION { GET, SET, CLEAR };
    enum FLAG_WORD { CONTROL, STATE };

    virtual STDMETHODIMP GetFriendsList(
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP FriendAdd(
                IN LPCWSTR wszName,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP FriendDelete(
                IN LPCWSTR wszName,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP GetIgnoredList(
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP IgnoredAdd(
                IN LPCWSTR wszName,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP IgnoredDelete(
                IN LPCWSTR wszName,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP Flags(
                IN FLAG_OPERATION op,
                IN FLAG_WORD    word,
                IN DWORD        dwFlags,
                IN PVOID        pvContext )=0;

    virtual STDMETHODIMP WhereIs(
                IN LPCWSTR wszName,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP SetLocation(
                IN DWORD dwStateFlags,
                IN LPCWSTR wszLocation,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP AcceptAddFriend(
                IN bool bReply,
                IN LPCWSTR wszName,
                IN PVOID pvContext )=0;
};


//////////////////////////////////////////////////////////////////////////////
// IGunChat
//
interface IGunChat: public IUnknown
{
    enum ChatModeFlags {
        STANDARD =0x01,
            USER =0x02,
        PASSWORD =0x04,
         PRIVATE =0x08,
          SECRET =0x10
    };

    virtual STDMETHODIMP GetListOfRooms( IN PVOID pvContext )=0;
    
    virtual STDMETHODIMP JoinRoom(
                IN LPCWSTR wszRoomName,
                IN LPCWSTR wszPassword,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP LeaveRoom(
                IN DWORD crid,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP LeaveAllRooms(
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP GetUsersInRoom(
                IN DWORD crid,
                IN PVOID pvContext )=0;
    
    virtual STDMETHODIMP SendToRoom(
                IN DWORD crid,
                IN LPCWSTR wszText,
                IN PVOID pvContext )=0;
    
    virtual STDMETHODIMP SendToUser(
                IN LPCWSTR wszUserName,
                IN LPCWSTR wszText,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP GetSquelchList(
                IN PVOID pvContext )=0;
    
    virtual STDMETHODIMP SquelchAdd(
                IN LPCWSTR wszName,
                IN PVOID pvContext )=0;
    
    virtual STDMETHODIMP SquelchDelete(
                IN LPCWSTR wszUserName,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP CreateRoom(
                IN LPCWSTR wszRoomName,
                IN LPCWSTR wszTopic,
                IN DWORD Mode,
                IN DWORD cUserLimit,
                IN LPCWSTR wszPassword,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP GetRoomOwner(
                IN DWORD crid,
                IN PVOID pvContext )=0; 

    // These methods can only be used by the room owner.
    //
    virtual STDMETHODIMP AssignOwnership(
                IN DWORD crid,
                IN LPCWSTR wszUserName,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP SetRoomTopic(
                IN DWORD crid,
                IN LPCWSTR wszTopic,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP SetRoomMode(
                IN DWORD crid,
                IN DWORD Mode,
                IN LPCWSTR wszPassword,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP InviteUser(
                IN DWORD crid,
                IN LPCWSTR wszUserName,
                IN LPCWSTR wszInviteText,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP KickOutUser(
                IN DWORD crid,
                IN LPCWSTR wszUserName,
                IN LPCWSTR wszReasonText,
                IN PVOID pvContext )=0;
};


//////////////////////////////////////////////////////////////////////////////
// IGunHostEnum
//
// The Enumeration callback interface in used by
// EnumPlayers, and EnumItemValues.
//
interface IGunHostEnum      // IUnknown is not vital.
{
    virtual STDMETHODIMP OnEnumPlayer(
                IN DWORD dwItemID,
                IN LPCWSTR pwszPlayerName )=0;

    virtual STDMETHODIMP OnEnumItemValue(
                IN LPCSTR pszKey,
                IN LPCWSTR pwszValue )=0;
};


//////////////////////////////////////////////////////////////////////////////
// IGunHost
//
interface IGunHost: public IUnknown
{
    enum ITEM_ID { SERVER_ITEM_ID=0 };

    virtual STDMETHODIMP CreateGame(
                IN LPCWSTR pwszName )=0;

    virtual STDMETHODIMP EndGame()=0;

    // Player Management.
    //
    virtual STDMETHODIMP AddPlayer(
                IN DWORD dwItemId,
                IN LPCWSTR pszPlayerName )=0; 

    virtual STDMETHODIMP DeletePlayer(
                IN DWORD wdItemId )=0;

    virtual STDMETHODIMP GetNumPlayers(
                OUT DWORD* pdwNumPlayers )=0;

    virtual STDMETHODIMP EnumPlayers(
                IN IGunHostEnum* pHost )=0;

    // Attribute Management.  (Game and Players)
    //
    virtual STDMETHODIMP SetItemValue(
                IN DWORD dwItemId,
                IN LPCSTR pszKey,
                IN LPCWSTR pwszValue )=0;

    virtual STDMETHODIMP GetItemValue(
                IN DWORD dwItemId,
                IN LPCSTR pszKey,
                OUT LPWSTR pwszValue,
                IN DWORD cch )=0;

    virtual STDMETHODIMP DeleteItemValue(
                IN DWORD dwItemId,
                IN LPCSTR pszKey )=0;

    virtual STDMETHODIMP GetNumItemValues(
                IN DWORD dwItemId,
                OUT DWORD *pdwNumValues )=0;

    virtual STDMETHODIMP EnumItemValues(
                IN DWORD dwItemId,
                IN IGunHostEnum* pHost )=0;
};


//////////////////////////////////////////////////////////////////////////////
// IGunBrowser
//
interface IGunBrowser: public IUnknown
{
    virtual STDMETHODIMP GetViewList(
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP GetHeaderRowOfView(
                IN DWORD dwViewId,
                IN PVOID pvContext )=0;
    
    virtual STDMETHODIMP GetPage(
                IN DWORD dwViewId,
                IN DWORD dwPageNumber,
                IN DWORD dwNumber,
                IN LPCWSTR wszString,
                IN PVOID pvContext )=0;

    virtual STDMETHODIMP GetRowPage(
                IN DWORD dwViewId,
                IN DWORD dwRowId,
                IN DWORD dwNumber,
                IN LPCWSTR wszString,
                IN PVOID pvContext )=0;
};  

#ifdef PSTATS_SAMPLE

//////////////////////////////////////////////////////////////////////////////
// IGunPStats
//
interface IGunPStats: public IUnknown
{

    virtual STDMETHODIMP GetStats(
                IN LPCWSTR wszString,
                IN PVOID pvContext )=0;
};  

#endif

//////////////////////////////////////////////////////////////////////////////
// CGunRow
//
class CGunRow
{
private:
    // Use IGunRowManager ONLY, to make rows!
    //
    CGunRow();
    CGunRow( const CGunRow & );
    ~CGunRow();

public:
    inline DWORD Count() const
                { return cStrings; };
    inline LPCWSTR Strings( DWORD i ) const
                { return apwszStrings[i]; };

private:
        DWORD  cStrings;
        LPCWSTR apwszStrings[1];
};

typedef const CGunRow *LPGUNROW;



//////////////////////////////////////////////////////////////////////////////
// IGunRowManager
//
interface IGunRowManager : public IUnknown
{
    virtual STDMETHODIMP AllocateRow(
                IN DWORD cValues,
                IN LPCWSTR const * apwszValues,
                OUT LPGUNROW* ppRow )=0;

    virtual STDMETHODIMP FreeRow(
                IN LPGUNROW pRow )=0;

    virtual STDMETHODIMP DupRow(
                IN LPGUNROW pRow,
                OUT LPGUNROW* ppRow )=0;
};




namespace Gun2 {

enum MessageId;

enum MessageMasks {
    c_MsgTypeMask    = 0x00000f00,
    c_CategoryIdMask = 0x000ff000
};

const DWORD c_TypeShift=8;
const DWORD c_TypeCount=7;

enum MessageType
{
    c_MsgTypeInvalid    = 0x00000000,
    c_MsgTypeCommand    = 0x00000100,
    c_MsgTypeResult     = 0x00000200,
    c_MsgTypeEvent      = 0x00000300,
    c_MsgTypeInternal   = 0x00000400,
    c_MsgTypePlugin     = 0x00000500,
    c_MsgTypePluginEvent= 0x00000600
};

const DWORD c_CategoryShift=12;
const DWORD c_CategoryCount=14;

enum CategoryId
{
    c_InvalidCategory       = 0x00000000,
    c_CategoryConnect       = 0x00001000,
    c_CategoryAccounts      = 0x00002000,
    c_CategoryNews          = 0x00003000,
    c_CategoryFriends       = 0x00004000,
    c_CategoryChat          = 0x00005000,
    c_CategoryMisc          = 0x00006000,
    c_CategoryMatch         = 0x00007000,
    // datacenter categories..
    c_CategoryFriendsServer = 0x00008000,
    c_CategoryChatServer    = 0x00009000,
    c_CategoryDS            = 0x0000a000,
    c_CategorySwitchboard   = 0x0000b000,
    c_CategoryMatchServer   = 0x0000c000,
#ifdef PSTATS_SAMPLE
	// sample category
	c_CategoryPStats		= 0x0000d000
#endif
};


inline CategoryId GetMessageCategory( MessageId msgId )
{
	return static_cast<CategoryId>(msgId & c_CategoryIdMask);
}
    
inline MessageType GetMessageType( MessageId msgId )
{
	return static_cast<MessageType>(msgId & c_MsgTypeMask);
}

enum ConnectMsgId
{
    c_idConnect=0,
    c_idDisconnect,
	c_idConnectFailed,
    c_idConnectInfo
};

enum AccountMsgId
{
    c_idLogin=0,
    c_idLogout,          
    c_idCreateAccount,
    c_idChangePassword,
    c_idChangePasswordFromHint,
    c_idGetAccountStatus,
    c_idGetPasswordHint,
    c_idLoginDup,
};

enum NewsMsgId
{
    c_idGetNews,
    c_idNews,
};

enum FriendsMsgId
{
    c_idGetFriendsList=0,
    c_idFriendAdd,  
    c_idFriendDelete,
    c_idGetIgnoredList,
    c_idIgnoredAdd,  
    c_idIgnoredDelete,
    c_idGetIdFromName,
    c_idGetNameFromId,
    c_idFlags,
    c_idWhereIs,
    c_idSetLocation,
    c_idAcceptAddFriend,
	c_idFriendStart,
    c_idFriendStatus,
	c_idFriendRequest,
	c_idFriendAddComplete,
};

enum ChatMsgId
{
    c_idGetListOfRooms=0,
    c_idJoinRoom,
    c_idLeaveRoom,
    c_idLeaveAllRooms,
    c_idGetUsersInRoom,
    c_idSendToRoom,
    c_idSendToUser,
    c_idGetSquelchList,
    c_idSquelchAdd, 
    c_idSquelchDelete,
    c_idCreateRoom,  
    c_idGetRoomOwner,
    c_idAssignOwnership,
    c_idSetRoomMode,
    c_idSetRoomTopic,
    c_idInviteUser,
    c_idKickOutUser,
    c_idRoomEnter,
    c_idRoomLeave,
    c_idRoomSpeak,
    c_idUserSpeak,
    c_idKickedOut,
    c_idReceiveOwnership,
    c_idInvited,
};

enum MatchMsgId
{
    c_idHostData=0,
    c_idHostClose,
    c_idGetViewList,
    c_idGetHeaderRow,
    c_idGetPage,
    c_idGetRowPage,
    c_idGetPageWithFriends, // only used in the backend.
};

#ifdef PSTATS_SAMPLE
enum PStatsMsgId
{
	c_idGetStats=0,
};
#endif

enum MessageId
{
    c_InvalidEvent       = 0,
    c_InvalidMessage     = 0,

    //////////// Connect Messages ///////////////
    c_msgConnectEvent           = c_MsgTypeEvent | c_CategoryConnect | c_idConnect,
    c_msgDisconnectEvent        = c_MsgTypeEvent | c_CategoryConnect | c_idDisconnect,
    c_msgConnectFailedEvent     = c_MsgTypeEvent | c_CategoryConnect | c_idConnectFailed,

    //////////// Account Messages ///////////////
    c_msgLoginResult            = c_MsgTypeResult  | c_CategoryAccounts | c_idLogin,
    c_msgLogoutResult           = c_MsgTypeResult  | c_CategoryAccounts | c_idLogout,
    c_msgCreateAccountResult    = c_MsgTypeResult  | c_CategoryAccounts | c_idCreateAccount,
    c_msgChangePasswordResult   = c_MsgTypeResult  | c_CategoryAccounts | c_idChangePassword,
    c_msgChangePasswordFromHintResult=c_MsgTypeResult|c_CategoryAccounts|c_idChangePasswordFromHint,
    c_msgGetAccountStatusResult = c_MsgTypeResult  | c_CategoryAccounts | c_idGetAccountStatus,
    c_msgGetPasswordHintResult  = c_MsgTypeResult  | c_CategoryAccounts | c_idGetPasswordHint,

    c_msgLoginDupEvent          = c_MsgTypeEvent | c_CategoryAccounts | c_idLoginDup,
    c_msgLogoutEvent            = c_MsgTypeEvent | c_CategoryAccounts | c_idLogout,

    //////////// News Messages ///////////////
    c_msgGetNewsResult          = c_MsgTypeResult  | c_CategoryNews | c_idGetNews,

    c_msgNewsEvent              = c_MsgTypeEvent   | c_CategoryNews | c_idNews,

    //////////// Friends Messages ///////////////
    c_msgGetFriendsListResult   = c_MsgTypeResult  | c_CategoryFriends | c_idGetFriendsList,
    c_msgFriendAddResult        = c_MsgTypeResult  | c_CategoryFriends | c_idFriendAdd,  
    c_msgFriendDeleteResult     = c_MsgTypeResult  | c_CategoryFriends | c_idFriendDelete,
	c_msgGetIgnoredListResult   = c_MsgTypeResult  | c_CategoryFriends | c_idGetIgnoredList,
	c_msgIgnoredAddResult       = c_MsgTypeResult  | c_CategoryFriends | c_idIgnoredAdd,  
	c_msgIgnoredDeleteResult    = c_MsgTypeResult  | c_CategoryFriends | c_idIgnoredDelete,
	c_msgGetIdFromNameResult    = c_MsgTypeResult  | c_CategoryFriends | c_idGetIdFromName,
	c_msgGetNameFromIdResult    = c_MsgTypeResult  | c_CategoryFriends | c_idGetNameFromId,
	c_msgFlagsResult            = c_MsgTypeResult  | c_CategoryFriends | c_idFlags,
	c_msgWhereIsResult          = c_MsgTypeResult  | c_CategoryFriends | c_idWhereIs,
	c_msgSetLocationResult      = c_MsgTypeResult  | c_CategoryFriends | c_idSetLocation,
	c_msgAcceptAddFriendResult  = c_MsgTypeResult  | c_CategoryFriends | c_idAcceptAddFriend,

	c_msgFriendStartEvent       = c_MsgTypeEvent  | c_CategoryFriends | c_idFriendStart,
	c_msgFriendStatusEvent      = c_MsgTypeEvent  | c_CategoryFriends | c_idFriendStatus,
	c_msgFriendRequestEvent     = c_MsgTypeEvent  | c_CategoryFriends | c_idFriendRequest,
	c_msgFriendAddCompleteEvent = c_MsgTypeEvent  | c_CategoryFriends | c_idFriendAddComplete,

    //////////// Chat Messages ///////////////
    c_msgGetListOfRoomsResult  = c_MsgTypeResult  | c_CategoryChat | c_idGetListOfRooms,
    c_msgJoinRoomResult        = c_MsgTypeResult  | c_CategoryChat | c_idJoinRoom,
    c_msgLeaveRoomResult       = c_MsgTypeResult  | c_CategoryChat | c_idLeaveRoom,
    c_msgLeaveAllRoomsResult   = c_MsgTypeResult  | c_CategoryChat | c_idLeaveAllRooms,
    c_msgGetUsersInRoomResult  = c_MsgTypeResult  | c_CategoryChat | c_idGetUsersInRoom,
    c_msgSendToRoomResult      = c_MsgTypeResult  | c_CategoryChat | c_idSendToRoom,
    c_msgSendToUserResult      = c_MsgTypeResult  | c_CategoryChat | c_idSendToUser,
    c_msgGetSquelchListResult  = c_MsgTypeResult  | c_CategoryChat | c_idGetSquelchList,
    c_msgSquelchAddResult      = c_MsgTypeResult  | c_CategoryChat | c_idSquelchAdd, 
    c_msgSquelchDeleteResult   = c_MsgTypeResult  | c_CategoryChat | c_idSquelchDelete,
    c_msgCreateRoomResult      = c_MsgTypeResult  | c_CategoryChat | c_idCreateRoom,  
    c_msgGetRoomOwnerResult    = c_MsgTypeResult  | c_CategoryChat | c_idGetRoomOwner,
    c_msgAssignOwnershipResult = c_MsgTypeResult  | c_CategoryChat | c_idAssignOwnership,
    c_msgSetRoomModeResult     = c_MsgTypeResult  | c_CategoryChat | c_idSetRoomMode,
    c_msgSetRoomTopicResult    = c_MsgTypeResult  | c_CategoryChat | c_idSetRoomTopic,
    c_msgInviteUserResult      = c_MsgTypeResult  | c_CategoryChat | c_idInviteUser,
    c_msgKickOutUserResult     = c_MsgTypeResult  | c_CategoryChat | c_idKickOutUser,

    c_msgRoomEnterEvent        = c_MsgTypeEvent | c_CategoryChat | c_idRoomEnter,
    c_msgRoomLeaveEvent        = c_MsgTypeEvent | c_CategoryChat | c_idRoomLeave,
    c_msgRoomSpeakEvent        = c_MsgTypeEvent | c_CategoryChat | c_idRoomSpeak,
    c_msgUserSpeakEvent        = c_MsgTypeEvent | c_CategoryChat | c_idUserSpeak,
    c_msgKickedOutEvent        = c_MsgTypeEvent | c_CategoryChat | c_idKickedOut,
    c_msgReceiveOwnershipEvent = c_MsgTypeEvent | c_CategoryChat | c_idReceiveOwnership,
    c_msgInvitedEvent          = c_MsgTypeEvent | c_CategoryChat | c_idInvited,

    // Browser messages
    c_msgGetViewListResult     = c_MsgTypeResult  | c_CategoryMatch | c_idGetViewList,
    c_msgGetHeaderRowResult    = c_MsgTypeResult  | c_CategoryMatch | c_idGetHeaderRow,
    c_msgGetPageResult         = c_MsgTypeResult  | c_CategoryMatch | c_idGetPage,
    c_msgGetRowPageResult      = c_MsgTypeResult  | c_CategoryMatch | c_idGetRowPage,

#ifdef PSTATS_SAMPLE
	// PStats sample messages
	c_msgGetStatsResult			= c_MsgTypeResult | c_CategoryPStats | c_idGetStats,
#endif

#ifdef IN_GUN_SYSTEM
#include "GunSysMessages.h"
#endif
};

} // namespace Gun2

interface IXmlPacket;

namespace Gun2 {

typedef DWORD UserModeFlags;
typedef DWORD ACCOUNT_STATUS;
typedef DWORD LOGOUT_REASON;
typedef DWORD FRIEND_ADD_STATUS;
    
enum AccountStatus {
    NORMAL=0,
    CLOSED=1,
};

enum LogoutReason { 
    USER_REQUEST=0,
        DUPLOGIN=1,
           ADMIN=2,
};

enum FriendAddStatus {
           ALLOWED=0,
            DENIED=1,
    USER_LOGGEDOUT=2,
};


struct ChatRoomAttrs {
	LPCWSTR pwszName;
	LPCWSTR pwszTopic;
    DWORD mode;
	DWORD cUserCount;
	DWORD cUserLimit;
};

struct ViewDescription {
    DWORD dwId;
    LPCWSTR pwszName;
};

struct UserStatus {
	LPCWSTR pwszUserName;
    UserModeFlags mode;
	LPCWSTR pwszLocation;
};


class CBaseMessage
{
public:
    /////////////////////////////
    // Accessors to the MessageId
    //
    // MessageId is always set in the constructor, so accessors for it
    // cannot error.  So they don't have to be STDMETHODs

    inline MessageId GetMessageId() const
        {   return m_msgId; }

    inline CategoryId GetMessageCategory() const
        {   return static_cast<CategoryId>((m_msgId & c_CategoryIdMask)); }
    
    inline MessageType GetMessageType() const
        {   return static_cast<MessageType>((m_msgId & c_MsgTypeMask)); }

//
// CLIENT CODE DOES NOT USE ANYTHING BELOW HERE
//
protected:
    ////////////////////////////////////////////////////////////////////
    // Derived Messages must implement RenderUnicode()
    ////////////////////////////////////////////////////////////////////
    //
    virtual STDMETHODIMP RenderUnicode( IN LPWSTR pwszDes,
                                        IN DWORD cchSize,
                                        OUT DWORD* pcchUsed )const=0;

    // Every Base Message class implements this.
    //
    STDMETHODIMP RenderUnicodeHeader( IN LPWSTR pwszBuf,
                                  IN LPCSTR pszTag,
                                  IN DWORD cchSize,
                                  OUT DWORD* pcchUser ) const;

    ////////////////////////////////////////////////////////////////////
    // Utility helper functions used by the derived classes.
    //
    STDMETHODIMP ComputeMessageText() const;

    static STDMETHODIMP RenderPrintf( LPWSTR pwsz,
                                      DWORD cchSize,DWORD* pcchUsed,
                                      LPCWSTR pwszFormat, ... );

    //
// CLIENT CODE DOES NOT USE ANYTHING BELOW HERE
//
public:
    /////////////////////////////
    // Ctor / Dtor's
    //
    // Create a writable Message object.
    CBaseMessage( IN MessageId msgId ); 

    // Create a readonly Message object.
    CBaseMessage( IN MessageId msgId,
                 IN IXmlPacket* pXmlPkt ); 

    virtual ~CBaseMessage();

    /////////////////////////////
    // Second stage construction
    //
    // Call this on message created from data.
    virtual STDMETHODIMP InitFromPacket( void );

    // Every Message has an InitNew( ... ) method but because
    // they are take different parameters, I can't declare it here.

    /////////////////////////////
    // Operations
    //
    STDMETHODIMP WriteMessageText( IN LPSTR pszDes,
                                   IN DWORD cchSize,
                                   OUT DWORD* pcchUsed ) const;

protected:
    bool        m_bInited;
    MessageId   m_msgId;
    IXmlPacket* m_pXmlPkt;

    // These get initialized the first time the message is converted
    // to xml and then are used as a cache.
    mutable LPSTR  m_pszText;
    mutable DWORD  m_cbTextBuf;
    mutable DWORD  m_cbTextLen;
};


//////////////////////////////////////////////////////////////////////////////
// CBaseResult
//
class CBaseResult: public CBaseMessage
{
public:
    inline STDMETHODIMP
    CBaseResult::GetContext( OUT PVOID* pContext)
    {
        if( !m_bInited ) 
            return ZT_E_NOT_INITED;
        else
            *pContext = m_Context;
        return S_OK;
    }

    inline STDMETHODIMP
    CBaseResult::GetHRESULT( OUT HRESULT* phr)
    {
        if( !m_bInited )
            return ZT_E_NOT_INITED;
        else
            *phr= m_hr;
        return S_OK;
    }

//
// CLIENT CODE DOES NOT USE ANYTHING BELOW HERE
//
public:
    CBaseResult( MessageId msgId, IXmlPacket * pXmlPkt );
    CBaseResult( MessageId msgId );
    virtual ~CBaseResult();

    STDMETHODIMP InitFromPacket( void );

protected:
    STDMETHODIMP InitNewResult( PVOID Context, HRESULT hr );

    virtual STDMETHODIMP RenderUnicode( IN LPWSTR pwszDes,
                                        IN DWORD cchSize,
                                        OUT DWORD* pcchUsed )const=0;

    STDMETHODIMP RenderUnicodeHeader( IN LPWSTR pwszBuf,
                                      IN LPCSTR pszTag,
                                      IN DWORD cchSize,
                                      OUT DWORD* pcchUser ) const;

private:
    PVOID  m_Context;
    HRESULT  m_hr;
};


//////////////////////////////////////////////////////////////////////////////
// CBaseEvent
//
class CBaseEvent: public CBaseMessage
{
//
// CLIENT CODE DOES NOT USE ANYTHING IN HERE.
// but this is the base class of all events so it exists here.
//
public:
    CBaseEvent( MessageId msgId, IXmlPacket * pXmlPkt );
    CBaseEvent( MessageId msgId );
    virtual ~CBaseEvent();

    STDMETHODIMP InitFromPacket( void );

protected:
    STDMETHODIMP InitNewEvent( DWORD no );
    virtual STDMETHODIMP RenderUnicode( IN LPWSTR pwszDes,
                                        IN DWORD cchSize,
                                        OUT DWORD* pcchUsed )const=0;

    STDMETHODIMP RenderUnicodeHeader( IN LPWSTR pwszBuf,
                                      IN LPCSTR pszTag,
                                      IN DWORD cchSize,
                                      OUT DWORD* pcchUser ) const;

};


}; // Gun2 Namespace

#include "GunApiMessages.h"

#endif // GUN2API_HEADER_H__
