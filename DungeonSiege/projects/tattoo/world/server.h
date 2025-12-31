#pragma once
/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: Server.h

	Author(s)	: Bartosz Kijanka

	Purpose		: This contains the Server base declaration as well as the local
				  and remote server declarations.

	(C)opyright 2000 Gas Powered Games, Inc.

	--------------------------------------------------------------------------

  General Approach:
	-----------------

	-	ALL of the simulation runs on the server.  Client's are mainly visualization
		terminals for the server-kept game state.

	-	Client/Server functionality is distributed throughout the code, rather than
		being localized to a single Server or Client object.  This is done mainly for
		performance reasons.  Drawing clear implimentation lines between the server
		and client functionality would mean much more interface code and much more
		memory and processor overhead in the case of a Server and Client existing
		on a single machine.  In a "strict" scenario, the Server would keep it's own
		collection of objects, as would the client... ON THE SAME MACHINE.  So, we would
		have to have duplicates of most of the run-time objects.  BUT, if we're clever
		and careful, we can distribute the client/server functionality throughout
		all our key objects, and operate on one set of objects on the Server machine,
		regardless of how many clients are also on that machine.

	-	Current design does assume one client per machine.

	-	Server machine can also host one local client, and n remote clients.


	Naming conventions:
	-------------------

	Method prefixes:

	S*		- "Server"				-	Method intented to be called only on the server machine.

	RS*		- "RPC-to-Server"		-	Method which can be called by any machine.  The system will
										make sure that the method will be called on the server machine.
										A server can share a machine with a client.

	RC*		- "RPC to Client(s)"	-	Method which can be called by either the server or one of the clients.
										Often times, this method is called as a result of an RS* method being
										called.  In such a case, the RS* method can be thought of as a REQUEST
										and the RC* method as a COMPLY.  Permissions should be set in such a
										cast to make sure that the RC* method can only be called by the Server.

	Types of possible RPC calls:
	----------------------------
	- send to server only
	- send to all
	- send to all but self
	- send to particular client, by id ( Each player object has a unique Id )

	Remember to:

		$$$ Need to optimize sends, such that all FUBI calls to singleton objects are made only once per machine.  Example: player hosts computer, and
			himself on the same box, while playing a remote human.

		$$$	We have to try to address incorrect usage issue.  Many multiplayer events will consists of the high-level calls lower-level
			pairs.  There is currently nothing preventing people from calling the lower-level pair directly.  All hi/lo pairs should
			require that only the hi part of the pair is called as the entry point in to the process.  I.e. call DoDamage() not SetLife().

	###

	-	RPC RETRY MECHANISM:

		We will need a retry mechanism for sending RPC commands since some commands may fail even though the packets arrived at the
		destination and were processed.  The main RPC commands in question are: create content, destroy content, equip/unequip object,
		link/unlink spatial parent and child, and change player ownership of an object.  So, these commands are mainly existence and
		ownership.  We could simply implement a retry list with the server if we're talking about very few commands.  But, we will
		likely find that before the project is finished we will use the redelivery for more than what we initially thought.  It makes
		sense to think of putting the redelivery mechanism into the transport later, such that any FUBI command can take advantage of
		it.

		Redelivery needs to take each player into account.  An RPC command should try to redeliver until all players have acknowledged
		"success" about processing the call.

		Debug: we will need hooks into the redelivery mechanism such that we can see which RPC calls haven't been successful.  Conditions
		for redelivery should be parameterized so we can tune how often redelivery is attempted.  Also, retries should fail after
		some amount of tries or time.  This will have to be tuned as well.  If the connection is lost with the player, we don't want
		the retry lists to blow up.

	###

	-	RPC ADDRESSING:

		The addressing is indicated a the FUBI call level.  The address is taken into account by the NetFubiSend where the fubi command
		is packaged and sent to the NetPipe.

		Three types of addressing possible:

		1. send to server

		2. send to all ( send to all but sender )

		3. send to specific machine/player

		// call on server ( very common )
		method()
		{
			FUBI_RPC_CALL( method, RPC_TO_SERVER );

			// do work meant only for the server here
		}


		// call on all ( very common )
		method()
		{
			FUBI_RPC_CALL( method, RPC_TO_ALL );

			// do work to local objects here
		}


		// call on specific machine/player ( not common ):
		method()
		{
			// the connection Id is essentially the network address for the machine
			FUBI_RPC_CALL( method, Player.GetMachineId() );

			// do machine/player specific work here
		}

	###

	-	GAME STATE CONVERGENCE:

		We will still want to run a "correction" task on the server which scans through all the server-ran content, and broadcasts
		a full update for a GO to all the clients.  This includes making sure that GOs thare were recently deleted are indeed deleted
		on all the clients.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#ifndef __SERVER_H
#define __SERVER_H


#include "GoDefs.h"


class NetPipe;
class NetFuBiReceive;
class NetFuBiSend;
struct NetPipeMessage;


/*===========================================================================

	Class:		Server

	Author(s):	Bartosz Kijanka

	Purpose:	This acts as the interface definition for any server object.

				The is the object that keeps track of all the players
				currently in the game.

				What kind of events should be cleared through the server?
				- player creation and destruction
				- map selection, start region selection
				- health or mana modifications
				- changes in item posession

---------------------------------------------------------------------------*/


////////////////////////////////////////////////////////////////////////////////
//	Settings
enum eDropInvOption
{
	SET_BEGIN_ENUM( DIO_, 0 ),

	DIO_INVALID,
	DIO_NOTHING,	
	DIO_EQUIPPED,
	DIO_INVENTORY,	
	DIO_ALL,


	SET_END_ENUM( DIO_ )
};

const char* ToString  ( eDropInvOption pc );
bool        FromString( const char* str, eDropInvOption& pc );

////////////////////////////////////////////////////////////////////////////////
//	Team
struct Team
{
	FUBI_POD_CLASS( Team );

	DWORD m_Id;
	DWORD m_Players;
	DWORD m_TextureId;

	Team( void )
	{
		ZeroObject( *this );
		m_Id = 1;
	}
};


////////////////////////////////////////////////////////////////////////////////
//	Server
class Server : public Singleton< Server >
{

public:

	enum  {  MAX_PLAYERS = 8  };

	typedef stdx::fast_vector <Player*> PlayerColl;
	typedef stdx::fast_vector <PlayerId> PlayerIdColl;
	typedef stdx::fast_vector< TeamId > TeamIdColl;

	Server( bool multiPlayerMode );
	~Server();

	////////////////////////////////////////
	//	critical events

	void Update( float actualDeltaTime );
	void Init( bool multiPlayerMode );
	void Shutdown( eShutdown sd );
	void ShutdownTransport( bool full );
	bool IsInitialized() { return m_Initialized; }

	void OnWorldStateChange( eWorldState state );

	void UpdateNetworkIO( bool dispatchReceived = true );
	void ResetTransport();
	void OnNetPipeNotify( NetPipeMessage const & msg );

	void SSyncOnMachine( DWORD machineId );

FEX void RSKeepMeAlive();					// clients will call this during times of no activity to prevent being booted
FEX void RCKeepMeAlive( DWORD machineId );	// clients will call this during times of no activity to prevent being booted
	FUBI_MEMBER_SET( RCKeepMeAlive, +SERVER );

	////////////////////////////////////////
	//	locale
	
	bool IsLocal()									{ return FuBi::IsServerLocal(); }
	bool IsRemote()									{ return !IsLocal(); }
	bool IsSinglePlayer()							{ return !m_MultiPlayerMode; }
	bool IsMultiPlayer()							{ return m_MultiPlayerMode; }

	////////////////////////////////////////////////////////////////////////////////
	//	player construction

FEX	FuBiCookie RSCreatePlayer( const wchar_t * name, ePlayerController controller );

	void SPCreateHumanPlayer();
	bool CreateComputerPlayer();

	////////////////////////////////////////
	//	player destruction

	bool SMarkPlayerForDeletion( PlayerId playerId );
	bool MarkPlayerForDeletion( PlayerId playerId, bool deletePlaceHolder = false );

	bool IsMarkedForDeletion( PlayerId playerId );
	void CommitDeleteRequests( bool addPlaceHolders = true );

	////////////////////////////////////////
	//	player queries

FEX	Player * GetPlayer( PlayerId playerId );
	Player * GetPlayerUnsafe( PlayerId playerId );
FEX	Player * GetPlayer( gpwstring const & sName );

	DWORD GetNumHumanPlayers();
	int GetPlayersOnMachine( PlayerColl& out, DWORD machineId );
	bool HasPlayersOnMachine( DWORD machineId );
FEX	Player * GetHumanPlayerOnMachine( DWORD machineId );

FEX	bool HasPlayer( PlayerId playerId )				{ return GetPlayer( playerId ) != NULL; }
FEX	Player * GetComputerPlayer()					{ return GetPlayer( PLAYERID_COMPUTER ); }
FEX	Player * GetLocalHumanPlayer();

	const PlayerColl& GetPlayers() const			{ return m_PlayerColl; }
	PlayerColl::iterator GetPlayersBegin()			{ return m_PlayerColl.begin(); }
	PlayerColl::iterator GetPlayersEnd()			{ return m_PlayerColl.end(); }

	float GetMaxPlayerLatency() const;
	UINT32 GetTotalHumanControlledGOs( void );

FEX void SetLatencyQueryFrequency( int f )			{ m_LatencyQueryFrequency = f; }

	// Get the player controlled Gos that are friendly to this.
FEX void GetFriendlyHumanCharacters( PlayerId playerId, bool inFrustum, bool checkTeam, GopColl & output );

	////////////////////////////////////////
	//	heros

	void SPlaceHeroesInWorld();

	////////////////////////////////////////////////////////////////////////////////
	//	teams

	void SInitTeams();
FEX	FuBiCookie RCInitTeams( DWORD MachineId );
	FUBI_MEMBER_SET( RCInitTeams, +SERVER );

	void SSetDefaultTeams();

	void SSetTeams( DWORD MachineId );
FEX FuBiCookie RCSetTeam( const Team & team, gpstring const & texture, DWORD MachineId );
	FUBI_MEMBER_SET( RCSetTeam, +SERVER );

FEX FuBiCookie RCSetTeamSign( TeamId team, gpstring const & texture );
	FUBI_MEMBER_SET( RCSetTeamSign, +SERVER );

/*
	When you join a team, you un-ally with all players on your previous team and make new -mutual- alliances
	with the players on your new team.
*/
FEX	FuBiCookie RSSelectNextTeam( PlayerId player, bool reverse = false );
FEX FuBiCookie RSSelectTeam( PlayerId player, gpstring const & texture );

FEX FuBiCookie RSJoinTeam( TeamId team, PlayerId player );
FEX FuBiCookie RCJoinTeam( TeamId team, PlayerId player );
	FUBI_MEMBER_SET( RCJoinTeam, +SERVER );

	Team * GetTeam( PlayerId playerId );

	TeamIdColl GetUsedTeams();

	void ResetTeams();

	bool GetAllowPlayerKills()						{ return m_bAllowPlayerKills; }
	void SetAllowPlayerKills( bool allow )			{ m_bAllowPlayerKills = allow; }

	////////////////////////////////////////
	//	game settings - $ maybe all this moves to WorldOptions?

	void SetSettingsDirty()							{	m_bSettingsDirty = true; }
	bool GetSettingsDirty()							{	bool dirty = m_bSettingsDirty;
														m_bSettingsDirty = false;
														return dirty;	}
	// max players
	unsigned int GetMaxPlayers();
	void SSetMaxPlayers( unsigned int maxPlayers );
FEX	FuBiCookie RCSetMaxPlayers( unsigned int maxPlayers );
	void SetMaxPlayers( unsigned int maxPlayers );

	FUBI_MEMBER_SET( RCSetMaxPlayers, +SERVER );

	// game password
	gpwstring const & GetGamePassword();
	void SSetGamePassword( gpwstring const & password );
FEX FuBiCookie RCSetGamePassword( gpwstring const & password );
	FUBI_MEMBER_SET( RCSetGamePassword, +SERVER );
	void SetGamePassword( gpwstring const & password );

	// allow start location
	bool GetAllowStartLocationSelection()			{ return m_bAllowStartLocationSelection; }
	void SSetAllowStartLocationSelection( bool allow );
FEX	FuBiCookie RCSetAllowStartLocationSelection( bool allow );
	FUBI_MEMBER_SET( RCSetAllowStartLocationSelection, +SERVER );

	// allow only new characters
	bool GetAllowNewCharactersOnly()				{ return m_bAllowNewCharactersOnly; }
	void SSetAllowNewCharactersOnly( bool allow );
FEX	FuBiCookie RCSetAllowNewCharactersOnly( bool allow );
	FUBI_MEMBER_SET( RCSetAllowNewCharactersOnly, +SERVER );

	// allow pausing
	bool GetAllowPausing();
	void SSetAllowPausing( bool bSet );
FEX void RCSetAllowPausing( bool bSet );
	FUBI_MEMBER_SET( RCSetAllowPausing, +SERVER );

	// allow JIP
	bool GetAllowJIP()								{ return m_bAllowJIP; }
	void SSetAllowJIP( bool bSet );
FEX void RCSetAllowJIP( bool bSet );
	FUBI_MEMBER_SET( RCSetAllowJIP, +SERVER );

	// ghost timeout
	float GetGhostTimeout()							{ return m_GhostTimeout; }
FEX	void SSetGhostTimeout( float timeout );
FEX	void RCSetGhostTimeout( float timeout );
	FUBI_MEMBER_SET( RCSetGhostTimeout, +SERVER );
	void SetGhostTimeout( float timeout )			{ m_GhostTimeout = timeout; }

	bool GetAllowRespawn( void ) const				{ return m_bAllowRespawn; }
FEX	void SSetAllowRespawn( bool bSet );
FEX	void RCSetAllowRespawn( bool bSet );
	void SetAllowRespawn( bool bSet )				{ m_bAllowRespawn = bSet; }

FEX	eDropInvOption GetDropInvOption( void ) const	{ return m_DropInvOption; }
FEX void SSetDropInvOption( eDropInvOption option );
FEX void RCSetDropInvOption( eDropInvOption option );
	void SetDropInvOption( eDropInvOption option )	{ m_DropInvOption = option; }


	/////////////////////////////////////////////////
	//	misc

	void SetScreenPlayer( Player * pPlayer )		{ m_pScreenPlayer = pPlayer; }
	Player * GetScreenPlayer()						{ return m_pScreenPlayer; }	
	bool HasScreenPlayer() const					{ return m_pScreenPlayer != NULL; }
FEX Go* GetScreenParty();
	bool HasScreenParty()							{ return GetScreenParty() != NULL; }
FEX	Go* GetScreenHero();
FEX	Go* GetScreenStash();

	bool GetScreenFormation( Formation * & formation );
	Formation * GetScreenFormation();

	DWORD GetScreenPlayerVisibleCount()				{ return m_ScreenPlayerVisibleCount; }
	void SetScreenPlayerVisibleCount( DWORD count )	{ m_ScreenPlayerVisibleCount = count; }

	////////////////////////////////////////
	//	debug

#if !GP_RETAIL
	void DebugCheckToldMachineToCreatePlayer( DWORD machineId, PlayerId pid );
	bool DebugToldMachineToCreatePlayer( DWORD machineId, PlayerId pid );
#endif




private:

	typedef std::vector< NetPipeMessage * > NetPipeDelayedMessageColl;

	////////////////////////////////////////
	//	debug

#if !GP_RETAIL
	void DebugOnMachineConnected( DWORD machineId );
	void DebugOnMachineDisconnected( DWORD machineId );
	void DebugOnCreatePlayerOnMachine( DWORD machineId, PlayerId pid );
#endif

	////////////////////////////////////////////////////////////////////////////////
	//	SP interfaces

	bool GetFirstFreePlayerId( PlayerId & pid );

	////////////////////////////////////////////////////////////////////////////////
	//	SP interfaces

	void MarkAllPlayersForDeletion();

	bool CreatePlayer(	const gpwstring& name,
						PlayerId playerId,
						ePlayerController Controller,
						DWORD machineId );

FEX	FuBiCookie RCMarkPlayerForDeletion( PlayerId playerId );
	FUBI_MEMBER_SET( RCMarkPlayerForDeletion, +SERVER );

	////////////////////////////////////////////////////////////////////////////////
	//	MP interfaces

FEX FuBiCookie RCSyncOnMachineHelper( DWORD machineId, const_mem_ptr syncBlock );
	void XferForSync( FuBi::PersistContext& persist );

FEX	FuBiCookie RCCreatePlayerOnMachine(		DWORD machineId,
											const wchar_t * szClientName,
											PlayerId playerId,
											ePlayerController controller,
											DWORD PlayerHostMachineId );
	FUBI_MEMBER_SET( RCCreatePlayerOnMachine, +SERVER );

	//	event handlers
	void OnSessionLost();
	void OnConnectionCreated( DWORD const ConnectionID );
	void OnConnectionDestroyed( DWORD const ConnectionID, DWORD const event );
	void PrivateOnNetPipeNotify( NetPipeMessage const & msg );

	void SSyncPlayers();

	////////////////////////////////////////////////////////////////////////////////
	//	DATA

	////////////////////////////////////////
	// debug

#	if !GP_RETAIL
	FUBI_EXPORT void DebugListPlayers();
	typedef stdx::fast_vector< DWORD > PidColl;
	typedef std::map< DWORD, PidColl > MachinePidArray;
	MachinePidArray m_MachinePidArray;
#	endif // !GP_RETAIL

	bool				m_MultiPlayerMode;

	////////////////////////////////////////
	//	players

	PlayerColl			m_PlayerColl;
	PlayerIdColl		m_ReqDeletePlayers;
	Player *			m_pScreenPlayer;

	double				m_NextLatencyQueryTime;
	int					m_LatencyQueryFrequency;

	////////////////////////////////////////
	//	options

	bool				m_Initialized;
	bool				m_bSettingsDirty;
	bool				m_bAllowStartLocationSelection;
	bool				m_bAllowNewCharactersOnly;
	bool				m_bAllowPausing;
	bool				m_bAllowJIP;
	bool				m_bAllowRespawn;
	bool				m_bAllowPlayerKills;
	eDropInvOption		m_DropInvOption;
	float				m_GhostTimeout;  // time user stays in ghost mode until it wears off

	////////////////////////////////////////
	//	owned objects

	my GoDb           * m_pGoDb;          
	my NetPipe        * m_pNetPipe;       
	my NetFuBiSend    * m_pNetFuBiSend;
	my NetFuBiReceive * m_pNetFuBiReceive;

	// state info

	DWORD						m_ScreenPlayerVisibleCount;
	NetPipeDelayedMessageColl	m_NetPipeMessages;

	////////////////////////////////////////
	//	teams

	typedef std::map< gpstring, unsigned int, istring_less > TeamSignMap;  // texture name, texture id
	TeamSignMap			m_TeamSigns;
	Team				m_Teams[ MAX_PLAYERS ];

	FUBI_SINGLETON_CLASS( Server, "This is the game server object." );
	SET_NO_COPYING( Server );
};


#define gServer Server::GetSingleton()


#endif  // __SERVER_H
