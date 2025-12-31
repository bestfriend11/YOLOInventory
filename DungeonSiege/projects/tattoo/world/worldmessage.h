//////////////////////////////////////////////////////////////////////////////
//
// File     :  WorldMessage.h
// Author(s):  Bartosz Kijanka, Scott Bilas
//
// Summary  :  The message flags below correspond to world events which may
//             happen in Tattoo.  Any time an event occurs in Tattoo which can
//             be described by any of the flags below, the code creating the
//             event has the responsibility of creating a WorldMessage and
//             sending it to any GameObjects involved in the event.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __WORLDMESSAGE_H
#define __WORLDMESSAGE_H

//////////////////////////////////////////////////////////////////////////////

#include "GoDefs.h"
#include "FuBiDefs.h"

//////////////////////////////////////////////////////////////////////////////
// enum MESSAGE_DISPATCH_FLAG declaration

enum MESSAGE_DISPATCH_FLAG
{
	MD_NONE										= 0,

	// delivery flags
	MD_BROADCAST								= 1	<< 0,	// broadcast to all go's
	MD_CC_PARENT								= 1	<< 1,	// cc the parent on this one
	MD_CC_CHILDREN								= 1 << 2,	// cc the children on this one
	MD_FROM_TRIGGER								= 1 << 3,
	MD_FROM_ELEVATOR							= 1 << 4,	
	MD_RPC										= 1 << 5,
	MD_DELAYED									= 1 << 6,	// delayed-send

	// delayed flags
	MD_DELAYED_ON_COLLISION_REPLACE_DUPLICATE	= 1 << 8,	// replaces old with new if there is a conflict
	MD_DELAYED_ON_COLLISION_DISCARD_NEW			= 1 << 9,	// ignores new message

	MD_DELAYED_COLLISION_ALLOWED				= MD_DELAYED_ON_COLLISION_REPLACE_DUPLICATE | MD_DELAYED_ON_COLLISION_DISCARD_NEW
};

MAKE_ENUM_BIT_OPERATORS( MESSAGE_DISPATCH_FLAG );

//////////////////////////////////////////////////////////////////////////////
// enum eWorldEvent declaration

// $ note: the event names must appear in the same order in the .cpp file
//         string table as they appear here in the enumeration.

enum eWorldEvent
{
	SET_BEGIN_ENUM( WE_, 0 ),

	WE_INVALID								,
	WE_UNKNOWN								,

	////////////////////////////////////////
	// system

	WE_CONSTRUCTED			 				,	// this should get sent to any GO right after it's been constructed and as the last thing in the init/load procedure
	WE_DESTRUCTED				 			,	// this should be sent to any GO right before it's deleted
	WE_JOB_DESTRUCTED						,	// sent just to job when it's being deleted during normal gameplay.  we_destructed is sent to job on gomind shutdown
	WE_ENTERED_WORLD						,	// object entered the world frustum
	WE_LEFT_WORLD							,	// object left the world frustum
	WE_FRUSTUM_MEMBERSHIP_CHANGED			,	// when frustum membership of go changes... data1 contains OLD membership
	WE_FRUSTUM_ACTIVE_STATE_CHANGED			,	// when the active frustums mask changes, this is broadcast.  data1 contains OLD mask
	WE_UPGRADED								,	// object has been upgraded by siege (visible for screen player)
	WE_DOWNGRADED							,	// object has been downgraded by siege (visible for screen player)
	WE_EXPIRED_AUTO	 						,	// object has automatically expired outside of world frustum and will be deleted next
	WE_EXPIRED_FORCED						,	// object has been force-expired and will be deleted next
	WE_PRE_SAVE_GAME						,	// called right before the game is saved
	WE_POST_SAVE_GAME						,	// called right after the game is saved
	WE_POST_RESTORE_GAME					,	// called right after the game is restored
	WE_TERRAIN_TRANSITION_DONE	  	   		,	// Notify object that it's task is complete
	WE_WORLD_STATE_TRANSITION_DONE			,	// state change that was requested is done
	WE_CAMERA_COMMAND_DONE					,	// Notify object that camera command is done
	WE_PLAYER_CHANGED						,	// called right after player reassignment, old player* is stored in data1
	WE_PLAYER_DATA_CHANGED					,	// broadcast when any stats on a Player change
	WE_SCIDBITS_CHANGED						,	// sent to an object when its scidbits change

	WE_MP_MACHINE_CONNECTED					,	// when a machine connects to our box
	WE_MP_MACHINE_DISCONNECTED				,	// when a machine connects to our box
	WE_MP_FAILED_CONNECT					,	// when this machine attemps to connect to a remote session bug has been banned
	WE_MP_SESSION_CHANGED					,	// a session has been enumerated or changed, index in data1
	WE_MP_SESSION_TERMINATED				,	// session removed from the enumeration, index in data1
	WE_MP_SESSION_ADDED						,	// session added to the enumeration, index in data1
	WE_MP_SESSION_CONNECTED					,	// send after netpipe finished the host or connect procedure
	WE_MP_PLAYER_CREATED					,	// sent whenever a player object is created on local machine
	WE_MP_PLAYER_DESTROYED					,	// sent whenever a player object is destroyed on local machine
	WE_MP_PLAYER_READY						,	// player is now ready, playerid in data1
	WE_MP_PLAYER_NOT_READY					,	// player no longer ready, playerid in data1
	WE_MP_PLAYER_WORLD_STATE_CHANGED		,	// whenever the player object's world state changes, this gets broadcast	
	WE_MP_PLAYER_SET_CHARACTER_LEVEL		,	// whenever the player's character level is updated
	WE_MP_SET_MAP							,	// sent when the server selects a map
	WE_MP_SET_MAP_WORLD						,	// sent when the server selects a map world
	WE_MP_SET_GAME_TYPE						,	// sent when the server selects a game type
	WE_MP_TERRAIN_FULLY_LOADED				,	// when no new siege nodes have been loaded this sim... only sent in JIP mode

	////////////////////////////////////////
	// ui

	WE_SELECTED								,	// object was selected
	WE_DESELECTED							,	// object was deselected
	WE_FOCUSED								,	// object was focused (set to first in selection)
	WE_UNFOCUSED							,	// object was unfocused (unselected probably)
	WE_HOTGROUP								,	// object entered the hot group
	WE_UNHOTGROUP							,	// object left the hot group
	WE_MOUSEHOVER							,	// object had a mouse over begin
	WE_UNMOUSEHOVER							,	// object lost its mouse over

	////////////////////////////////////////
	// basic gameplay events

	WE_COLLIDED				 				,	// Collision occured with terrain or game object with damage
	WE_GLANCED								,	// Collision occured with terrain or game object without damage
	WE_DAMAGED								,
	WE_DROPPED								,
	WE_EQUIPPED				 				,
	WE_EXPLODED								,
	WE_GO_TIMER_DONE						,	// go components may send themselves this message
	WE_GOT					 				,
	WE_HEALED								,	// $$$ unused?
	WE_KILLED					 			,
	WE_LEVELED_UP				 			,
	WE_LOST_CONSCIOUSNESS					,
	WE_PICKED_UP							,
	WE_REQ_ACTIVATE				  	   		,
	WE_REQ_CAST					 			,
	WE_REQ_CAST_CHARGE			 			,
	WE_REQ_DEACTIVATE		  		   		,
	WE_REQ_DELETE				  	   		,	// request deletion for self
	WE_REQ_DIE,
	WE_REQ_TALK_BEGIN						,
	WE_REQ_TALK_END							,
	WE_REQ_USE				  		   		,	// an object gets this message sent to it when a player wants to "use" it.
	WE_RESURRECTED							,	// $$$ unused?
	WE_SPELL_COLLISION						,	// from = GOID_INVALID if terrain collision OR goid of game object that was hit, Data1 = SiegePos
	WE_SPELL_EXPIRATION_TIMER_RESET			,	// special command given from one skrit to another for extending spell duration
	WE_SPELL_SYNC_BEGIN						,	// For synchronizing with skrit
	WE_SPELL_SYNC_END						,	//
	WE_SPELL_SYNC_MID						,	//
	WE_START_BURNING						,	// Makes this go catch on fire
	WE_START_SIMULATING		  		   		,	// Tells a game object to start simulating itself using physics
	WE_STOP_BURNING							,	// Makes the go stop burning if on fire
	WE_TRIGGER_ACTIVATE						,
	WE_TRIGGER_DEACTIVATE					,
	WE_UNEQUIPPED				 			,
	WE_WEAPON_LAUNCHED						,
	WE_WEAPON_SWUNG							,

	////////////////////////////////////////
	// MCP and anim

	WE_MCP_ALL_SECTIONS_SENT	   			,	// sent by MCP whenever all the the PLAN sections for a GO have been transmitted to the CLIENTS
	WE_MCP_CHORE_CHANGING					,
	WE_MCP_FACING_LOCKEDON					,
	WE_MCP_FACING_UNLOCKED					,

	WE_MCP_GOAL_CHANGED						,
	WE_MCP_GOAL_REACHED						,
	WE_MCP_INVALIDATED						,
	WE_MCP_SECTION_COMPLETE_WARNING			,
	WE_MCP_SECTION_COMPLETED				,

	WE_MCP_NODE_BLOCKED 					,	// The required connections between nodes have changed

	WE_MCP_DEPENDANCY_CREATED				,
	WE_MCP_DEPENDANCY_REMOVED				,
	WE_MCP_DEPENDANCY_BROKEN				,
	WE_MCP_MUTUAL_DEPENDANCY				,

	WE_ANIM_LOOPED							,	// nema event 'loop'
	WE_ANIM_DONE							,	// nema event 'done'
	WE_ANIM_ATTACH_AMMO						,	// nema sends this message when ammo is to be created/attached 
	WE_ANIM_WEAPON_FIRE						,	// nema sends this message upon ammo release for projectile and contact for melee
	WE_ANIM_WEAPON_SWUNG					,	// nema sends this message upon melee attack begin/end (data1 is begin=0/end=1)
	WE_ANIM_SFX								,	// Special Effect (SFX number is stored in data1)
	WE_ANIM_DIE								,	// Sent at the instant the character 'expires'
	WE_ANIM_OTHER							,	// other misc nema event (stored in data1)

	////////////////////////////////////////
	// AI sensors

	WE_AI_ACTIVATE							,	// if an actor that is not currently processing AI receives this message, he will start processing AI
	WE_AI_DEACTIVATE						,	// if an actor receives this message, he will stop processing AI, use with WE_AI_ACTIVATE to control when an actor will run AI
	WE_ALERT_ENEMY_SPOTTED					,
	WE_ALERT_PROJECTILE_NEAR_MISSED			,
	WE_ATTACKED_MELEE						,
	WE_ATTACKED_RANGED						,
	WE_ENEMY_ENTERED_INNER_COMFORT_ZONE		,
	WE_ENEMY_ENTERED_OUTER_COMFORT_ZONE		,
	WE_ENEMY_SPOTTED						,
	WE_ENGAGED_FLED							,	// sent to actor: engaged object has started to flee
	WE_ENGAGED_HIT_KILLED					,	// sent to actor: the engaged object was hit and killed by the actor
	WE_ENGAGED_HIT_LIVED					,	// sent to actor: engaged object was hit and still lived
	WE_ENGAGED_INVALID						,	// sent to actor: ERROR condition - engaged object has become invalid
	WE_ENGAGED_KILLED						,	// sent to actor: the engaged object was killed, but not by the actor
	WE_ENGAGED_LOST							,	// sent to actor: engaged object was probably unloaded
	WE_ENGAGED_LOST_CONSCIOUSNESS			,	// sent to actor: engaged actor has lost consciousness
	WE_ENGAGED_MISSED						,	// sent to actor: engaged object was attacked and missed
	WE_FRIEND_ENTERED_INNER_COMFORT_ZONE	,
	WE_FRIEND_ENTERED_OUTER_COMFORT_ZONE	,
	WE_FRIEND_SPOTTED						,
	WE_GAINED_CONSCIOUSNESS					,
	WE_JOB_CURRENT_ACTION_BASE_JAT_CHANGED	,
	WE_JOB_FAILED							,	// some objects may receive this message at the UNsuccessful end of a job	// $$$ unused?
	WE_JOB_FINISHED							,	// some objects may receive this message at the successful end of a job
	WE_JOB_REACHED_TRAVEL_DISTANCE			,	// sent when the job has moved the actor past x distance from the time the job has started
	WE_JOB_TIMER_DONE						,	// when a SimpleStateMachine (SSM) timer expires, the SSM sends itself this message
	WE_LIFE_RATIO_REACHED_HIGH				,	// $$$ unused?
	WE_LIFE_RATIO_REACHED_LOW				,
	WE_MANA_RATIO_REACHED_HIGH				,
	WE_MANA_RATIO_REACHED_LOW				,
	WE_MIND_ACTION_Q_EMPTIED				,	// when the last action in the action queue was just deleted, the mind will message itself this message
	WE_MIND_ACTION_Q_IDLED					,	// when the action Q has idles for some fixed amount of time, the GO receives this message
	WE_MIND_PROCESSING_NEW_JOB				,	//
	WE_REQ_JOB_END							,	// when job is requested to end in a way that honors atomic states and graceful shutdown, unlike Job::MarkForDeletion()
	WE_REQ_SENSOR_FLUSH						,	// if an actor receives this message, he will flush his sensort if the Goid in Data1 appears in his visible object list

	////////////////////////////////////////
	// patch messages

	WE_MP_SESSION_CREATED					,	// sent on server when session is successfully hosted

	SET_END_ENUM( WE_ ),
};

FUBI_EXPORT const char* ToString  ( eWorldEvent val );
FUBI_EXPORT bool        FromString( const char* str, eWorldEvent& val );

//////////////////////////////////////////////////////////////////////////////
// enum eWorldEventTraits declaration

enum eWorldEventTraits
{
	WMT_NONE				=      0,
	WMT_IS_CLIENT_OK		= 1 << 0,			// ok to send this on the client machines?
	WMT_IS_MCP_EVENT		= 1 << 1,			// this is an mcp class event
	WMT_IS_ENGAGED_EVENT	= 1 << 2,			// this is an ai's engaged event
	WMT_GO_IN_FRUSTUM		= 1 << 3,			// the go receiving the message must be inside a valid world frustum
	WMT_INGAME_ONLY			= 1 << 4,			// post these messages as delayed (don't send directly) if we are not ingame
	WMT_SEND_ONLY			= 1 << 5,			// this message is "send-only" - cannot be sent delayed!
	WMT_ENGINE_ONLY			= 1 << 6,			// this message cannot be sent by anybody except the engine!
	WMT_FLAMETHROWER_WANTS	= 1 << 7,			// flamethrower wants this message
	WMT_SKRITBOT_WANTS		= 1 << 8,			// skrit bots want this message
	WMT_RPC					= 1 << 9,			// ok for event to be networked to other machines
	WMT_AI_WANTS			= 1 << 10,			// the AI wants this message
};

MAKE_ENUM_BIT_OPERATORS( eWorldEventTraits );

eWorldEventTraits GetWorldEventTraits   ( eWorldEvent e );
bool              TestWorldEventTraits  ( eWorldEvent e, eWorldEventTraits t );
bool              TestWorldEventTraitsEq( eWorldEvent e, eWorldEventTraits t );

inline bool IsValidClientEvent ( eWorldEvent e )	{  return ( IsServerLocal() || TestWorldEventTraits( e, WMT_IS_CLIENT_OK ) );  }
inline bool IsMCPMessage       ( eWorldEvent e )	{  return ( TestWorldEventTraits( e, WMT_IS_MCP_EVENT ) );  }
inline bool IsEngagedEvent     ( eWorldEvent e )	{  return ( TestWorldEventTraits( e, WMT_IS_ENGAGED_EVENT ) );  }
inline bool IsSendOnlyEvent    ( eWorldEvent e )	{  return ( TestWorldEventTraits( e, WMT_SEND_ONLY ) );  }
inline bool IsEngineOnlyEvent  ( eWorldEvent e )	{  return ( TestWorldEventTraits( e, WMT_ENGINE_ONLY ) );  }
inline bool IsFlamethrowerEvent( eWorldEvent e )	{  return ( TestWorldEventTraits( e, WMT_FLAMETHROWER_WANTS ) );  }
inline bool IsRPCEvent		   ( eWorldEvent e )	{  return ( TestWorldEventTraits( e, WMT_RPC ) );  }


//////////////////////////////////////////////////////////////////////////////
// class WorldMessage declaration

class WorldMessage
{
public:

// Setup.

	WorldMessage( eWorldEvent Event );

	WorldMessage( eWorldEvent Event, Goid To );

	WorldMessage( eWorldEvent Event, Goid From, Goid To );

	WorldMessage( eWorldEvent Event, Goid From, Goid To, DWORD data );

	WorldMessage( eWorldEvent Event, Scid To );

	WorldMessage( eWorldEvent Event, Goid From, Scid To );

	WorldMessage( eWorldEvent Event, Goid From, Scid To, DWORD data );

	WorldMessage();

	~WorldMessage(){}

	bool Xfer( FuBi::PersistContext& persist );

	void Send( MESSAGE_DISPATCH_FLAG Flags = MD_NONE ) const;
	void SendDelayed( float delay = 0, MESSAGE_DISPATCH_FLAG Flags = MD_NONE ) const;

	void SSend( MESSAGE_DISPATCH_FLAG Flags = MD_NONE ) const;
	void SSendDelayed( float delay = 0, MESSAGE_DISPATCH_FLAG Flags = MD_NONE ) const;

// Event info.

	FUBI_EXPORT eWorldEvent GetEvent() const					{  return ( m_Event );  }

	FUBI_EXPORT bool HasEvent( eWorldEvent Event ) const		{  return ( m_Event == Event );  }

	void SetEvent( eWorldEvent Event )							{  m_Event = Event;  }

	void SetData1( unsigned int data )							{  m_Data1 = data;  }

	unsigned int GetData1() const								{  return ( m_Data1 );  }
	FUBI_EXPORT int FUBI_RENAME( GetData1 )() const				{  return ( (int)GetData1() );  }

// Routing info.

	bool IsCC() const											{  return ( m_bCC );  }

	bool IsBroadcast() const									{  return ( m_bBroadcast );  }

	FUBI_EXPORT Goid GetSendFrom() const						{  return ( m_SendFrom );  }
	FUBI_EXPORT Goid GetSendTo() const							{  return ( m_SendTo );  }
	FUBI_EXPORT Scid GetSendToScid() const						{  return ( m_SendToScid );  }

	void SetSendFrom( Goid From )								{  m_SendFrom = From; }

	void SetSendTo( Goid To )									{  gpassert( To != GOID_INVALID ); m_SendTo = To;  }

	// how long to wait before sending this message
	void SetDelayTime( float Delay );

	// what's the actual gametime when this for the above
	double GetMaturityTime() const		  						{  return ( m_MaturityTime );  }

// Status.

	bool IsBeingSent() const				  					{  return ( m_bBeingSent );  }

	void SetIsBeingSent( bool Param )	  						{  m_bBeingSent = Param;  }

// Operators.

	bool operator == ( WorldMessage const & message ) const
	{
		if( (GetEvent() == message.GetEvent()) && ( m_SendFrom == message.GetSendFrom() ) )
		{
			if ( m_SendToScid != SCID_INVALID )
			{
				return( m_SendToScid == message.m_SendToScid );
			}
			else
			{
				return( m_SendTo == message.m_SendTo );
			}
		}
		return false;
	}

// Debug.

	static unsigned int GetConstructionCount()					{  return ( ms_ConstructionCount );  }
	static void ClearConstructionCount()						{  ms_ConstructionCount = 0;  }

#if !GP_RETAIL
	double GetDebugSendTime() const 							{  return ( m_DebugSendTime ); }
#endif


private:

	friend class MessageDispatch;

	void Initialize();				// initialize all - used by constructors

// Data.

	static unsigned int ms_ConstructionCount;

	eWorldEvent m_Event;

	Goid m_SendFrom;		// what GameObject the message is origigination from
	Goid m_SendTo;			// what GameObject the message is heading for
	Scid m_SendToScid;		// what GameObject the message is heading for, based on static content identifier

	double m_MaturityTime;	// if this is nonzero, this message will be sent when gametime is EQUAL or GREATER THAN maturity time

	bool m_bBeingSent			: 1;
	bool m_bCC					: 1;
	bool m_bBroadcast			: 1;

#if !GP_RETAIL
	double m_DebugSendTime;
#endif

	// generic parameters - let's keep this as small as possible and don't cast from a pointer!!
	unsigned int m_Data1;

	SET_NO_INHERITED( WorldMessage );
	FUBI_POD_CLASS( WorldMessage );
};

// send to goid
FEX void SendWorldMessage( eWorldEvent event, Goid from, Goid to, DWORD data );
FEX void SendWorldMessage( eWorldEvent event, Goid from, Goid to );

FEX void PostWorldMessage( eWorldEvent event, Goid from, Goid to, DWORD data, float delay );
FEX void PostWorldMessage( eWorldEvent event, Goid from, Goid to, float delay );

// send to scid (delayed until scid enters world)
FEX void SendWorldMessage( eWorldEvent event, Goid from, Scid to, DWORD data );
FEX void SendWorldMessage( eWorldEvent event, Goid from, Scid to );
// rpc variants
FEX void SSendWorldMessage( eWorldEvent event, Goid from, Scid to, DWORD data );
FEX void SSendWorldMessage( eWorldEvent event, Goid from, Scid to );

FEX void PostWorldMessage( eWorldEvent event, Goid from, Scid to, DWORD data, float delay );
FEX void PostWorldMessage( eWorldEvent event, Goid from, Scid to, float delay );
// rpc variants
FEX void SPostWorldMessage( eWorldEvent event, Goid from, Scid to, DWORD data, float delay );
FEX void SPostWorldMessage( eWorldEvent event, Goid from, Scid to, float delay );

//////////////////////////////////////////////////////////////////////////////

#endif  // __WORLDMESSAGE_H

//////////////////////////////////////////////////////////////////////////////
