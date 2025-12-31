#include "precomp_world.h"

#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "Go.h"
#include "GoDb.h"
#include "go.h"
#include "godb.h"
#include "GoCore.h"
#include "GoSupport.h"
#include "messagedispatch.h"
#include "skritbotmgr.h"
#include "worldmessage.h"
#include "worldstate.h"
#include "worldtime.h"
#include "world.h"
#include "player.h"
#include "victory.h"


FUBI_DECLARE_SELF_TRAITS( MessageDispatch::Message );
FUBI_DECLARE_SELF_TRAITS( MessageDispatch::UnloadedMessage );
FUBI_DECLARE_SELF_TRAITS( WorldMessage );



#if GP_RETAIL
#define CHECK_DB_LOCKED( MSG )
#else // GP_RETAIL

void ReportErrorGo( const char* type, Goid goid )
{
	GoHandle go( goid );
	if ( go )
	{
		gperrorf(( "%s: Goid = 0x%08X, Scid = 0x%08X, Template = '%s'\n",
				   type, goid, go->GetScid(), go->GetTemplateName() ));
	}
	else if ( goid == GOID_INVALID )
	{
		gperrorf(( "%s: <system>\n", type ));
	}
	else
	{
		gperrorf(( "%s: Goid = 0x%08X (invalid)\n", type, goid ));
	}
}

#define CHECK_DB_LOCKED( MSG )											\
	if ( GoDb::DoesSingletonExist() && gGoDb.IsLockedForSaveLoad() )	\
	{																	\
		ReportSys::AutoReport autoReport( &gErrorContext );				\
		gperrorf(( "Very bad! Someone is attempting to "				\
				   "send a world message during a load/save!!\n\n"		\
				   "Message: %s\n",										\
				   ::ToString( MSG.GetEvent() ) ));						\
		ReportErrorGo( "From   ", MSG.GetSendFrom() );					\
		ReportErrorGo( "To     ", MSG.GetSendTo() );					\
	}
#endif // GP_RETAIL


bool MessageDispatch::BroadcastCallback :: Callback( Go* go )
{
	if ( go->IsInAnyWorldFrustum() || !TestWorldEventTraits( m_Msg.GetEvent(), WMT_GO_IN_FRUSTUM ) )
	{
		gMessageDispatch.SendToOne( m_Msg, go );
	}

	return ( true );
}


bool MessageDispatch::Message :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_Message", m_Message );
	persist.Xfer( "m_Flag", rcast <DWORD&> ( m_Flag ) );
	return ( true );
}


bool MessageDispatch::Message :: ShouldPersist( void ) const
{
	GoHandle go( m_Message.GetSendTo() );
	return ( !go || !go->IsLodfi() );
}


bool MessageDispatch::UnloadedMessage :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_DelayTime",  m_DelayTime  );
	persist.Xfer( "m_Message",    m_Message    );

	return ( true );
}


MessageDispatch::MessageDispatch( void )
{
	m_NumMessagesSinceLastUpdate		= 0;
	m_NumObjectsMessagedSinceLastUpdate	= 0;
}


MessageDispatch::~MessageDispatch( void )
{
}


struct MsgShouldPersistHelper
{
	template <typename T>
	bool operator () ( const T& obj ) const
	{
		return ( obj.second.ShouldPersist() );
	}
};

bool MessageDispatch::Xfer( FuBi::PersistContext& persist )
{
	persist.XferIfMap( "m_DelayedMessageDb",  m_DelayedMessageDb, MsgShouldPersistHelper() );
	persist.XferMap  ( "m_ScidMessageDb",     m_ScidMessageDb     );
	persist.XferMap  ( "m_UnloadedMessageDb", m_UnloadedMessageDb );

	return ( true );
}


void MessageDispatch::Shutdown()
{
	m_DelayedMessageDb .clear();
	m_ScidMessageDb    .clear();
	m_UnloadedMessageDb.clear();

	m_NumMessagesSinceLastUpdate = 0;
	m_NumObjectsMessagedSinceLastUpdate = 0;
}


void MessageDispatch::Update( bool force )
{
	GPPROFILERSAMPLE( "MessageDispatch :: Update", SP_HANDLE_MESSAGE );

	if( !force )
	{
		SINGLETON_ONCE_PER_SIM;
	}
	
	m_NumMessagesSinceLastUpdate		= 0;
	m_NumObjectsMessagedSinceLastUpdate	= 0;

	// avoid going in here if there are no messages
	if ( !m_DelayedMessageDb.empty() )
	{
		double currentTime = gWorldTime.GetTime();
		MessageDb messages;

		// grab messages
		{
			kerneltool::Critical::Lock locker( m_DelayedCritical );
			messages.swap( m_DelayedMessageDb );
		}

		// process delayed messages
		MessageDb::iterator i = messages.begin(), end = messages.end();
		while ( (i != end) && (i->first <= currentTime) )
		{
			WorldMessage& message = i->second.m_Message;
			bool send = false;

			// only do if not on deck
			if ( !gGoDb.IsOnDeck( message.GetSendTo() ) )
			{
				// also if it has to be in the world, check for that
				if ( ::TestWorldEventTraits( message.GetEvent(), WMT_GO_IN_FRUSTUM ) )
				{
					Goid goid = message.GetSendTo();
					if ( message.m_SendToScid != SCID_INVALID )
					{
						goid = gGoDb.FindGoidByScid( message.m_SendToScid );
					}

					// get the go to see if it's in the world
					GoHandle go( goid );
					if ( !go || go->IsInAnyWorldFrustum() || go->IsOmni() )
					{
						send = true;
					}
				}
				else
				{
					// send regardless of where it is
					send = true;
				}
			}

			if ( send )
			{
				// send it
				Send( message, i->second.m_Flag );

				// delete & advance
				i = messages.erase( i );
			}
			else
			{
				// skip
				++i;
			}
		}

		// move remainder back
		{
			kerneltool::Critical::Lock locker( m_DelayedCritical );
			messages.swap( m_DelayedMessageDb );

			end = messages.end();
			for ( i = messages.begin() ; i != end ; ++i )
			{
				m_DelayedMessageDb.insert( *i );
			}
		}
	}

	// process loaded scids
	while ( !m_ScidLoadedDb.empty() )
	{
		Scid scid = *m_ScidLoadedDb.begin();
		m_ScidLoadedDb.erase( m_ScidLoadedDb.begin() );

		std::pair <ScidMessageDb::iterator, ScidMessageDb::iterator> rc = m_ScidMessageDb.equal_range( scid );
		if ( rc.first != rc.second )
		{
			// copy these messages local, to avoid recursion issues
			typedef stdx::fast_vector <Message> MessageColl;
			MessageColl scidMessages;
			for ( ScidMessageDb::iterator i = rc.first ; i != rc.second ; ++i )
			{
				scidMessages.push_back( i->second );
			}

			// nuke old entries
			m_ScidMessageDb.erase( rc.first, rc.second );

			// now process
			MessageColl::iterator j, jbegin = scidMessages.begin(), jend = scidMessages.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				Send( j->m_Message, j->m_Flag );
			}
		}
	}
}


bool MessageDispatch::OnGoEnterWorld( Go* go )
{
	kerneltool::Critical::Lock locker( m_DelayedCritical );
	gpassert( go != NULL );
	gpassert( ::IsServerLocal() || m_UnloadedMessageDb.empty() );

	// instanced scid may have been loaded
	if ( !go->IsSpawned() )
	{
		m_ScidLoadedDb.insert( go->GetScid() );
	}

	// find unloaded messages
	std::pair <UnloadedMessageDb::iterator, UnloadedMessageDb::iterator>
			rc = m_UnloadedMessageDb.equal_range( go->GetGoid() );
	if ( rc.first == rc.second )
	{
		return ( false );
	}

	// move them back
	while ( rc.first != rc.second )
	{
		const UnloadedMessage& msg = rc.first->second;
		double newTime = PreciseAdd(gWorldTime.GetTime(), msg.m_DelayTime);
		m_DelayedMessageDb.insert( std::make_pair( newTime, msg.m_Message ) );
		rc.first = m_UnloadedMessageDb.erase( rc.first );
	}

	return ( true );
}


bool MessageDispatch::OnGoLeaveWorld( Go* go )
{
	kerneltool::Critical::Lock locker( m_DelayedCritical );
	gpassert( go != NULL );

	size_t oldSize = m_UnloadedMessageDb.size();

	// find messages pending for this go
	MessageDb::iterator i, ibegin = m_DelayedMessageDb.begin(), iend = m_DelayedMessageDb.end();
	for ( i = ibegin ; i != iend ; )
	{
		const WorldMessage& message = i->second.m_Message;

		if ( message.GetSendTo() == go->GetGoid() )
		{
			// don't queue up messages when a go leaves the world if we're a 
			// client, it will only cause more problems than it would solve 
			// (because all non-all clients go's are deleted from the client
			// when they exit the world).
			if ( ::IsServerLocal() )
			{
				// if it's a request for deletion, just leave it, otherwise that
				// may hang around for eternity.
				if ( message.GetEvent() != WE_REQ_DELETE )
				{
					m_UnloadedMessageDb.insert( std::make_pair( go->GetGoid(), UnloadedMessage( i->first, i->second ) ) );
				}
			}

			// erase it from delayed db
			i = m_DelayedMessageDb.erase( i );
		}
		else
		{
			++i;
		}
	}

	return ( m_UnloadedMessageDb.size() != oldSize );
}


void MessageDispatch::OnGoDestroyed( Go* go )
{
	kerneltool::Critical::Lock locker( m_DelayedCritical );
	gpassert( go != NULL );

	// remove from delayed db
	MessageDb::iterator i = m_DelayedMessageDb.begin(), end = m_DelayedMessageDb.end();
	while ( i != end )
	{
		WorldMessage& message = i->second.m_Message;

		if ( message.GetSendTo() == go->GetGoid() )
		{
			// just nuke 'er
			i = m_DelayedMessageDb.erase( i );
		}
		else
		{
			if ( message.GetSendFrom() == go->GetGoid() )
			{
				// it's ok, keep the message movin' along, but since i'm going
				// away, re-mark the sender.
				message.SetSendFrom( GOID_INVALID );
			}

			++i;
		}
	}

	// remove from unloaded message db, leaving send to scid messages
	{
		std::pair <UnloadedMessageDb::iterator, UnloadedMessageDb::iterator> rc = m_UnloadedMessageDb.equal_range( go->GetGoid() );
		for( ; rc.first != rc.second; )
		{
			Message& message = (*rc.first).second.m_Message;

			if( message.m_Message.GetSendToScid() != SCID_INVALID )
			{
				// Put message back into the scid message db so it is not lost
				m_ScidMessageDb.insert( ScidMessageDb::value_type( message.m_Message.m_SendToScid, Message( message.m_Message, message.m_Flag ) ) );
			}
			rc.first = m_UnloadedMessageDb.erase( rc.first );
		}
	}

	// remove from scid-related db's
	Scid scid = go->GetScid();
	if ( ::IsInstance( scid, false ) )
	{
		// remove from the just-loaded db
		ScidLoadedDb::iterator found = m_ScidLoadedDb.find( scid );
		if ( found != m_ScidLoadedDb.end() )
		{
			m_ScidLoadedDb.erase( found );
		}

		// remove from the message db
		bool bIsRetired	= false;
		gGoDb.FindGoidByScid( scid, &bIsRetired );
		if( bIsRetired )
		{
			std::pair <ScidMessageDb::iterator, ScidMessageDb::iterator> rc = m_ScidMessageDb.equal_range( scid );
			m_ScidMessageDb.erase( rc.first, rc.second );
		}
	}
}


void MessageDispatch::RegisterBroadcastCallback( gpstring const & name, WorldMessageBroadcastCb cb )
{
	for( GeneralCallbackColl::iterator i = m_GeneralCallbackColl.begin(); i != m_GeneralCallbackColl.end(); ++i )
	{
		if( (*i).m_Name.same_no_case( name ) )
		{
			gpassertm( 0, "Callback of said name already registered" );
			return;
		}
	}

	m_GeneralCallbackColl.push_back( GeneralCallback( name, cb ) );
}


void MessageDispatch::UnregisterBroadcastCallback( gpstring const & name )
{
	for( GeneralCallbackColl::iterator i = m_GeneralCallbackColl.begin(); i != m_GeneralCallbackColl.end(); ++i )
	{
		if( (*i).m_Name.same_no_case( name ) )
		{
			m_GeneralCallbackColl.erase( i );
			return;
		}
	}

	gpassertm( 0, "Callback of said name not registered" );
}


void MessageDispatch::SSend( WorldMessage & message, MESSAGE_DISPATCH_FLAG flags )
{
	CHECK_SERVER_ONLY;

	if( !TestWorldEventTraits( message.GetEvent(), WMT_RPC ) )
	{
		gperrorf(( "WorldMessage event '%s' can't be sent across the network! Ignored.", ::ToString( message.GetEvent() ) ));
		return;
	}

	flags |= MD_RPC;

	// once for everyone else (route through Go so it only goes to those who need it)
	if ( ::IsMultiPlayer() )
	{
		bool sendGlobal = true;

		// only bother with this if not send-to-scid
		if ( message.GetSendToScid() == SCID_INVALID )
		{
			// try to use 'to' as router
			GoHandle to( message.GetSendTo() );
			if ( to )
			{
				to->RCSend( message, flags );
				sendGlobal = false;
			}
			else
			{
				// try 'from' as router now
				GoHandle from( message.GetSendFrom() );
				if ( from )
				{
					from->RCSend( message, flags );
					sendGlobal = false;
				}
			}
		}

		// have to do global i guess...
		if ( sendGlobal )
		{
			RCSend( message, flags );
		}
	}

	if ( flags & MD_DELAYED )
	{
		SendDelayed( message, flags );
	}
	else
	{
		Send( message, flags );
	}
}


void MessageDispatch::RCSend( WorldMessage & message, MESSAGE_DISPATCH_FLAG flags )
{
	FUBI_RPC_CALL( RCSend, RPC_TO_OTHERS );

	if ( flags & MD_DELAYED )
	{
		SendDelayed( message, flags );
	}
	else
	{
		Send( message, flags );
	}
}


void MessageDispatch::Send( WorldMessage & message, MESSAGE_DISPATCH_FLAG flags )
{
	CHECK_DB_LOCKED( message );

	flags = (MESSAGE_DISPATCH_FLAG)(flags & ~MD_DELAYED);

	bool processed = false;
	message.SetIsBeingSent( true );
	++m_NumMessagesSinceLastUpdate;

	// bots get it first
#	if GP_ENABLE_SKRITBOT
	if ( TestWorldEventTraits( message.GetEvent(), WMT_SKRITBOT_WANTS ) && ::IsPrimaryThread() )
	{
		message.m_bCC = true;
		gSkritBotMgr.HandleMessage( message );
		message.m_bCC = false;
	}
#	endif // GP_ENABLE_SKRITBOT

	// check to see if this message must be sent ingame only
	eWorldState worldState = gWorldState.GetCurrentState();
	if ( (!IsInGame( worldState ) || (worldState == WS_RELOADING)) && (GetWorldEventTraits( message.GetEvent() ) & WMT_INGAME_ONLY) )
	{
		// $ early bailout - send delayed
		message.SetDelayTime( 0 );
		SendDelayed( message, flags );
		return;
	}

	// sending to ANY means that we are implicitly broadcasting
	if ( message.GetSendTo() == GOID_ANY )
	{
		flags |= MD_BROADCAST;
	}

	// do we just broadcast? ($ early bailout)
	if ( flags & MD_BROADCAST )
	{
		if( message.m_SendToScid != SCID_INVALID )
		{
			gperror( "You can't broadcast to a particular SCID.  Params don't make sense.  What do you really mean?" );
			return;
		}

		// have the godb call us back - the callback will filter inactive go's
		message.m_bBroadcast = true;
		if ( GoDb::DoesSingletonExist() )
		{
			BroadcastCallback cb( message );
			gGoDb.ForEachGlobalGo( makeFunctor( cb, &BroadcastCallback::Callback) );
			
			// special case for these
			if ( message.HasEvent( WE_POST_RESTORE_GAME ) )
			{
				gGoDb.ForEachLocalGo( makeFunctor( cb, &BroadcastCallback::Callback) );
			}
		}
		
		GeneralCallbackColl::iterator im;
		for( im = m_GeneralCallbackColl.begin(); im != m_GeneralCallbackColl.end(); ++im )
		{
			(*im).m_Functor( message );
		}

		message.m_bBroadcast = false;

		// if we broadcasted, that's all the sending we need to do
		return;
	}

	Goid goid;

	if( message.m_SendToScid != SCID_INVALID )
	{
		CHECK_PRIMARY_THREAD_ONLY;

		goid = gGoDb.FindGoidByScid( message.m_SendToScid );

		if(  goid == GOID_INVALID )
		{
			// save the message, if the Scid can't be sent to
			m_ScidMessageDb.insert( ScidMessageDb::value_type( message.m_SendToScid, Message( message, flags ) ) );
			return;
		}
	}
	else
	{
		goid = message.GetSendTo();
	}

	GoHandle go( goid );
	if ( !go )
	{
		gpwarningf(( "MessageDispatch::Send - Sending to invalid object. Goid = 0x%08x\n", goid ));
		return;
	}
	else if( flags & MD_FROM_TRIGGER )
	{
		CHECK_PRIMARY_THREAD_ONLY;

		GoHandle trigger( message.GetSendFrom() );
		GoHandle target( goid );

		if( !trigger || (!trigger->IsInAnyWorldFrustum() && !trigger->IsOmni()) )
		{
			gpwarningf((	"Possible content problem: origin of message is a trigger which is no longer in world or has been deleted. Message %s, intended Go %s scid 0x%08x\n",
							::ToString( message.GetEvent() ),
							target->GetTemplateName(),
							target->GetScid() ));
		}

		//	messages from triggers get special treatment... if target is outside of any frustum, the message
		//	is queued
		if ( TestWorldEventTraits( message.GetEvent(), WMT_GO_IN_FRUSTUM ) && !target->IsInAnyWorldFrustum() && !target->IsOmni() )
		{
			gpwarningf((	"Possible content problem: Trigger %s, scid 0x%08x sending to Go %s scid 0x%08x which is outside of world frustum.\n",
							trigger ? trigger->GetTemplateName() : gpstringf( "0x%08X", message.GetSendFrom() ).c_str(),
							trigger ? trigger->GetScid() : 0,
							target->GetTemplateName(),
							target->GetScid() ));

			if ( ::IsServerLocal() )
			{
				m_UnloadedMessageDb.insert( std::make_pair( goid, UnloadedMessage( 0, Message( message, flags ) ) ) );
			}
			return;
		}
	}

	// send to addresee
	processed = SendToOne( message, go );

	// send to victory/game conditions
	if ( IsServerLocal() && ::IsPrimaryThread() )
	{
		gVictory.HandleWorldMessage( message );
	}

	// cc the parent
	Go* parent = go->GetParent();

	if ( (parent != NULL)										// must have one
		&& (parent->GetGoid() != goid)							// unless that's our target
		&& (parent->HasParty() || (flags & MD_CC_PARENT)) )		// always cc for parties or if cc flag set
	{
		message.m_bCC = true;
		SendToOne( message, parent );
		message.m_bCC = false;
		processed = true;
	}

	// do we cc the children?
	if ( flags & MD_CC_CHILDREN )
	{
		// $ note we copy the children before messaging them - they may remove
		//   themselves from the child list during iteration and invalidate the
		//   GopColl iterators we've got.
		GopColl children = go->GetChildren();

		message.m_bCC = true;
		GopColl::iterator i, begin = children.begin(), end = children.begin();
		for ( i = begin ; i != end ; ++i )
		{
			SendToOne( message, *i );
		}
		message.m_bCC = false;
		processed = true;
	}

	// cc any watchers
	GoidColl watched;
	if ( gGoDb.GetWatchedBy( go, watched ) > 0 )
	{
		CHECK_PRIMARY_THREAD_ONLY;

		// $ note that we have copied the watched by set into a goid coll rather
		//   than using the iterators (in GetWatchedByIters). this is to prevent
		//   objects removing themselves and possibly other objects from the
		//   watching db which would nuke our iterators.

		message.m_bCC = true;
		GoidColl::iterator i, ibegin = watched.begin(), iend = watched.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// sender doesn't get CC'd on his own messages
			if( *i == message.m_SendFrom )
			{
				continue;
			}
			GoHandle watcher( *i );
			if ( watcher )
			{
				SendToOne( message, watcher );
			}
		}
		message.m_bCC = false;
	}

	message.SetIsBeingSent( false );

	gpassert( processed == true );
}


void MessageDispatch::SendDelayed( WorldMessage & message, MESSAGE_DISPATCH_FLAG flags )
{
	CHECK_DB_LOCKED( message );
	CHECK_PRIMARY_THREAD_ONLY;

	flags = (MESSAGE_DISPATCH_FLAG)(flags & ~MD_DELAYED);

	gpassertf( !IsSendOnlyEvent( message.GetEvent() ),
			 ( "Bad! Cannot send message %s as delayed!", ToString( message.GetEvent() ) ));

	kerneltool::Critical::Lock locker( m_DelayedCritical );

	//----- handle message collisions

	if ( flags & (MD_DELAYED_ON_COLLISION_DISCARD_NEW | MD_DELAYED_ON_COLLISION_REPLACE_DUPLICATE) )
	{
		MessageDb::iterator i = m_DelayedMessageDb.begin(), end = m_DelayedMessageDb.end();
		while ( i != end )
		{
			if ( i->second.m_Message == message )
			{
#if !GP_RETAIL
				Goid sendToGoid = message.GetSendTo();
				if( !GoHandle( sendToGoid ) )
				{
					sendToGoid = gGoDb.FindGoidByScid( message.m_SendToScid );
				}

				GoHandle sendTo( sendToGoid );
				GoHandle sendFrom( message.GetSendFrom() );

				gpwarningf((	"WorldMessage collision : event %s, sent from %s to %s\n",
								::ToString( message.GetEvent() ),
								sendFrom ? sendFrom->GetTemplateName() : "unknown",
								sendTo ? sendTo->GetTemplateName() : ToString( message.m_SendToScid ).c_str() ));
#endif
				// if there are other delayed messages with the same name and receiver - don't add this one
				if ( flags & MD_DELAYED_ON_COLLISION_DISCARD_NEW )
				{
					return;
				}

				// if there are other delayed messages with the same name and receiver - erase them
				if ( flags & MD_DELAYED_ON_COLLISION_REPLACE_DUPLICATE )
				{
					// erase and skip the advance, keep looking for more. if
					// they're not JobMessages, they have already been shown to
					// be equal.
					i = m_DelayedMessageDb.erase( i );
					continue;
				}
			}

			// advance
			++i;
		}
	}

	//----- save message as delayed message

	m_DelayedMessageDb.insert( std::make_pair( message.GetMaturityTime(), Message( message, flags ) ) );
}


bool MessageDispatch::SendToOne( const WorldMessage& message, Go* go )
{
	gpassert( go != NULL );

	go->HandleMessage( message );
	++m_NumObjectsMessagedSinceLastUpdate;

#	if ( !GP_RETAIL )
	{
		GoHandle from( MakeGoid(message.GetData1()) );
		if ( go->IsHuded() || (from && from->IsHuded()) )
		{
			// get to pos
			const SiegePos* toPos = NULL;
			if ( go->HasPlacement() )
			{
				toPos = &go->GetPlacement()->GetPosition();
			}

			// get from pos
			const SiegePos* fromPos = toPos;
			if ( from && from->HasPlacement() )
			{
				fromPos = &from->GetPlacement()->GetPosition();
				if ( toPos == NULL )
				{
					toPos = fromPos;
				}
			}

			// draw
			if ( toPos != NULL )
			{
				gpassert( fromPos != NULL );
				gWorld.DrawDebugLinkedTerrainPoints(
						*fromPos,
						*toPos,
						0xff00ff60,
						ToString( message.GetEvent() ) );
			}
		}
	}
#	endif

	return ( true );
}


