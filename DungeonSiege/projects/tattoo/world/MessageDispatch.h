#pragma once
/*=======================================================================================

  MessageDispatch

  purpose:	This class dispatches messages around the world.  It's the post-office.

  author:	Bartosz Kijanka

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
#ifndef __MESSAGEDISPATCH_H
#define __MESSAGEDISPATCH_H




//----- Forward declarations
class MessageDispatch;
class WorldMessage;

#include <list>

#include "GoDefs.h"
#include "worldmessage.h"
#include "worldtime.h"
#include "KernelTool.h"




const unsigned int MIN_DELTA_T = 1;





class MessageDispatch : public Singleton < MessageDispatch >
{

public:

	typedef CBFunctor1< WorldMessage & > WorldMessageBroadcastCb;
	
	friend class WorldMessage;
	friend class Go;

	MessageDispatch( void );
	~MessageDispatch( void );

	bool Xfer( FuBi::PersistContext& persist );

	// kill everything
	void Shutdown();
	void Update( bool force = false );

	// metrics
	DWORD GetNumMessagesSentSinceLastUpdate()		{ return( m_NumMessagesSinceLastUpdate ); }
	DWORD GetNumObjectsMessagedSinceLastUpdate()	{ return( m_NumObjectsMessagedSinceLastUpdate ); }

	DWORD GetDelayedMessageDbSize()					{ return( m_DelayedMessageDb.size() ); }
	DWORD GetUnloadedMessageDbSize()				{ return( m_UnloadedMessageDb.size() ); }
	DWORD GetScidMessageDbSize()					{ return( m_ScidMessageDb.size() ); }
	DWORD GetScidLoadedListSize()					{ return( m_ScidLoadedDb.size() ); }

	bool OnGoEnterWorld( Go* go );
	bool OnGoLeaveWorld( Go* go );
	void OnGoDestroyed ( Go* go );

	// callbacks

	void RegisterBroadcastCallback( gpstring const & name, WorldMessageBroadcastCb cb );
	void UnregisterBroadcastCallback( gpstring const & name );
	void WorldMessageBroadcastCbStub( WorldMessage & ) {};


private:

	class GeneralCallback
	{
	public:
		GeneralCallback( gpstring const & name, WorldMessageBroadcastCb & functor )
		{
			m_Name		= name;
			m_Functor	= functor;
		};

		~GeneralCallback(){};

		gpstring				m_Name;
		WorldMessageBroadcastCb	m_Functor;
	};

	typedef std::vector< GeneralCallback > GeneralCallbackColl;

FEX	void SSend ( WorldMessage & message, MESSAGE_DISPATCH_FLAG flags = MD_NONE );
FEX	void RCSend( WorldMessage & message, MESSAGE_DISPATCH_FLAG flags = MD_NONE );
	void Send  ( WorldMessage & message, MESSAGE_DISPATCH_FLAG flags = MD_NONE );

	// send message LATER
FEX void SSendDelayed ( WorldMessage & message, MESSAGE_DISPATCH_FLAG flags = MD_DELAYED )	{  SSend( message, flags | MD_DELAYED );  }
	void SendDelayed  ( WorldMessage & message, MESSAGE_DISPATCH_FLAG flags = MD_DELAYED );

	// force message send to ONE particular object
	bool SendToOne( const WorldMessage& message, Go* go );

	//----- data

	struct BroadcastCallback
	{
		const WorldMessage& m_Msg;

		BroadcastCallback( const WorldMessage& msg ) : m_Msg( msg )  {  }

		bool Callback( Go* go );
	};

	friend BroadcastCallback;


	struct Message
	{
		WorldMessage          m_Message;

		MESSAGE_DISPATCH_FLAG m_Flag;

		Message( void )
			: m_Flag( MD_NONE )  {}

		Message( const WorldMessage& message, MESSAGE_DISPATCH_FLAG flag )
			: m_Message( message ), m_Flag( flag )  {  }

		bool Xfer( FuBi::PersistContext& persist );
		bool ShouldPersist( void ) const;
	};

	struct UnloadedMessage
	{
		float   m_DelayTime;		// seconds remaining before messages was supposed to be delivered
		Message m_Message;			// the message

		UnloadedMessage( void )  {  }
		UnloadedMessage( double previousTime, const Message& msg )
			: m_Message( msg )
		{
			m_DelayTime = scast <float> ( PreciseSubtract(previousTime, gWorldTime.GetTime()) );
		}

		bool Xfer( FuBi::PersistContext& persist );
	};

	typedef std::multimap <double, Message>         MessageDb;
	typedef std::multimap <Scid,   Message>         ScidMessageDb;
	typedef std::multimap <Goid,   UnloadedMessage> UnloadedMessageDb;
	typedef std::set <Scid>                         ScidLoadedDb;

	kerneltool::Critical	m_DelayedCritical;

	MessageDb				m_DelayedMessageDb;
	ScidMessageDb			m_ScidMessageDb;
	UnloadedMessageDb		m_UnloadedMessageDb;
	ScidLoadedDb			m_ScidLoadedDb;

	unsigned int			m_NumMessagesSinceLastUpdate;
	unsigned int			m_NumObjectsMessagedSinceLastUpdate;

	GeneralCallbackColl		m_GeneralCallbackColl;

	FUBI_SINGLETON_CLASS( MessageDispatch, "MessageDispatch - WorldMessage dispatcher." );
	SET_NO_COPYING( MessageDispatch );
};


#define gMessageDispatch MessageDispatch::GetSingleton()



#endif