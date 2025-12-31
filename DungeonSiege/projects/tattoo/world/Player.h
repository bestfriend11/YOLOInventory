#pragma once
/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: Player.h

	Author(s)	: Bartosz Kijanka

	Purpose		: This declares the interface for the player object, which
				  can also be thought of as a game "client" object.

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#ifndef __PLAYER_H
#define __PLAYER_H

#include "GoDefs.h"
#include "cameraposition.h"


class AverageAccumulator;

// Loot filter flags to determine what the player will pick up or see on labels.
enum eLootFilter
{	
	LTF_INVALID				= 0,
	LTF_MISC				= 1 << 0,
	LTF_NORMAL_WEAPONS		= 1 << 1,
	LTF_ENCHANTED_WEAPONS	= 1 << 2,
	LTF_NORMAL_ARMOR		= 1 << 3,
	LTF_ENCHANTED_ARMOR		= 1 << 4,
	LTF_JEWELRY				= 1 << 5,
	LTF_QUEST_ITEMS			= 1 << 6,
	LTF_HEALTH_POTIONS		= 1 << 7,
	LTF_MANA_POTIONS		= 1 << 8,
	LTF_SPELLS				= 1 << 9,
	LTF_GOLD				= 1 << 10,
	LTF_LOREBOOKS			= 1 << 11,

	LTF_ALL					= LTF_MISC | LTF_NORMAL_WEAPONS | LTF_ENCHANTED_WEAPONS | LTF_NORMAL_ARMOR | LTF_ENCHANTED_ARMOR | LTF_JEWELRY | LTF_QUEST_ITEMS | LTF_HEALTH_POTIONS | LTF_MANA_POTIONS | LTF_SPELLS | LTF_GOLD | LTF_LOREBOOKS,
};

const char* ToString      ( eLootFilter e );
gpstring    ToFullString  ( eLootFilter e );
bool        FromString    ( const char* str, eLootFilter& e );
bool        FromFullString( const char* str, eLootFilter& e );

MAKE_ENUM_BIT_OPERATORS( eLootFilter );

////////////////////////////////////////////////////////////////////////////////
//	
class Player
{

public:

	Player( PlayerId playerId );

	Player( const gpwstring&  name,
			unsigned int      machineId,
			ePlayerController controller,
			PlayerId          playerId );

	~Player();

	bool Xfer( FuBi::PersistContext& persist );
	void Update();
	bool IsInitialized()								{ return m_bInitialized; }
	bool IsMarkedForDeletion();
	
	void MarkForSyncedDeletion()						{ m_bMarkedForSyncedDeletion = true; }
	bool IsMarkedForSyncedDeletion()					{ return m_bMarkedForSyncedDeletion; }

	///////////////////////////////////////
	//	identification

	gpwstring const & GetName() const					{ return m_sName; }
FEX	void GetName( gpstring& name ) const				{ name = ::ToAnsi( GetName() ); }
FEX FuBiCookie RSSetName( gpwstring const & name );
FEX FuBiCookie RCSetName( gpwstring const & name );
	FUBI_MEMBER_SET( RCSetName, +SERVER );

	void SetName( gpwstring const & name )				{ m_sName = name;  SetIsDirty(); }

	FEX PlayerId GetId( void ) const					{ gpassert( IsPower2( MakeInt( m_Id ) ) ); return( m_Id ); }

FEX ePlayerController GetController() const				{ return( m_Controller ); }
FEX static int GetHumanPlayerCount()					{ return( ms_HumanPlayerCount ); }

	bool IsRemote() const								{ return FuBi::IsMachineRemote( GetMachineId() ); }
	bool IsLocal() const								{ return !IsRemote(); }

	bool IsComputerPlayer() const						{ return m_Controller == PC_COMPUTER; }
FEX bool IsScreenPlayer() const;
	bool IsLocalHumanPlayer() const						{ return IsLocal() && (m_Controller == PC_HUMAN); }

FEX	bool IsReadyToPlay()								{ return m_bReadyToPlay; }
FEX FuBiCookie RSSetReadyToPlay( bool flag );

	bool IsJIP()										{ return( m_bJIP ); }
FEX FuBiCookie RCSetJIP( bool flag );
	FUBI_MEMBER_SET( RCSetJIP, +SERVER );

	bool IsOnZone()										{ return( m_bZone ); }
FEX FuBiCookie RSSetIsOnZone( bool flag );
FEX FuBiCookie RCSetIsOnZone( bool flag );
	FUBI_MEMBER_SET( RCSetIsOnZone, +SERVER );

	// in a multiplayer game, this is the NetPipe connection ID belonging to this player
FEX DWORD GetMachineId() const							{ return IsComputerPlayer() ? RPC_TO_SERVER : m_MachineId; }

	///////////////////////////////////////
	//	allies

FEX	int GetTeam();

	DWORD GetFriendTo()									{ return m_FriendTo; }
	void SSetFriendTo( DWORD friends );

FEX FuBiCookie RCSetFriendTo( DWORD friends );
	FUBI_MEMBER_SET( RCSetFriendTo, +SERVER );

	DWORD GetFriendToMe()								{ return m_FriendToMe; }
	void SSetFriendToMe( DWORD friends );
FEX FuBiCookie RCSetFriendToMe( DWORD friends );
	FUBI_MEMBER_SET( RCSetFriendToMe, +SERVER );

	///////////////////////////////////////
	//	startup

	// called by server - create the starting world frustum for this player
	void SCreateInitialFrustum();

FEX void RCSetInitialFrustum( DWORD machineId, FrustumId frustumId, const SiegePos& fpos, const CameraPosition& cpos );
	FUBI_MEMBER_SET( RCSetInitialFrustum, +SERVER );

	FrustumId GetInitialFrustum() const					{ return m_InitialFrustumId; }
	void SetInitialFrustum( FrustumId frustumId );

	// called by server - add player's party and hero to world, automatically creating the necessary frusta
	void SAddPlayerToWorld( bool createHero = true );
	bool AddedPlayerToWorld()							{ return m_bAddedPlayerToWorld; }
	void SCreateTradeGold( void );

	void	SetLoadProgress( float ratio );
	float	GetLoadProgress()							{ return m_LoadRatio; }

FEX	void	SCreateStash( void );
FEX	void	RCSetStash( Goid stash );
	Goid	GetStash()									{ return m_Stash; }

// Player's hero.

FEX void RSSetHeroName( gpwstring const & name );
FEX FuBiCookie RCSetHeroName( gpwstring const & name );
	FUBI_MEMBER_SET( RCSetHeroName, +SERVER );

	void SetHeroName( gpwstring const & name );
	gpwstring const & GetHeroName() const				{  return m_HeroName;  }
	void SetHero( Goid goid )							{  m_HeroGoid = goid;  }
	Goid GetHero( void ) const							{  return ( m_HeroGoid );  }
FEX	bool IsHeroSpecReady( void ) const					{  return ( !m_HeroName.empty() && !m_HeroCloneSourceTemplateName.empty() );  }

FEX FuBiCookie RSSetHeroCloneSourceTemplate( gpstring const & name );
FEX FuBiCookie RCSetHeroCloneSourceTemplate( gpstring const & name );
	FUBI_MEMBER_SET( RCSetHeroCloneSourceTemplate, +SERVER );

	void SetHeroCloneSourceTemplate( gpstring const & name );
	gpstring const & GetHeroCloneSourceTemplate() const	{ return m_HeroCloneSourceTemplateName; }

	// Stash import template name handling 
FEX void RSSetStashCloneSourceTemplate( gpstring const & name );
FEX void RCSetStashCloneSourceTemplate( gpstring const & name );

	void SetStashCloneSourceTemplate( gpstring const & name );
	gpstring const & GetStashCloneSourceTemplate() const { return m_StashCloneSourceTemplateName; }

FEX FuBiCookie RSSetHeroSkin( ePlayerSkin skin, gpstring const & name );
FEX FuBiCookie RCSetHeroSkin( ePlayerSkin skin, gpstring const & name );
	FUBI_MEMBER_SET( RCSetHeroSkin, +SERVER );

	gpstring const & GetHeroSkin( ePlayerSkin skin )	{ return m_HeroSkins[ skin ]; }

FEX FuBiCookie RSSetHeroHead( gpstring const & name );
FEX FuBiCookie RCSetHeroHead( gpstring const & name );
	FUBI_MEMBER_SET( RCSetHeroHead, +SERVER );
	
	gpstring const & GetHeroHead()						{ return m_HeroHead; }

	void SetHeroPortrait( DWORD texture, bool addRef );
	DWORD GetHeroPortrait() const						{ return m_HeroPortrait; }

FEX FuBiCookie RSSetHeroUberLevel( float level );
FEX FuBiCookie RCSetHeroUberLevel( float level );
	FUBI_MEMBER_SET( RCSetHeroUberLevel, +SERVER );
	void  SetHeroUberLevel( float level );
	float GetHeroUberLevel()							{ return m_HeroUberLevel; }

// Character importing.

FEX void RSImportCharacter( const gpstring& spec, int characterIndex );
FEX void RCImportCharacter( DWORD machineId, const gpstring& spec, int characterIndex );
	FUBI_MEMBER_SET( RCImportCharacter, +SERVER );

FEX void RSImportStash( const gpstring& spec );
FEX void RCImportStash( DWORD machineId, const gpstring& spec );

// World state.

	eWorldState GetWorldState()								{ return m_WorldState; }

FEX FuBiCookie RSSetWorldState( eWorldState state );
FEX FuBiCookie RCSetWorldState( eWorldState state );				// just echos state to other clients
	FUBI_MEMBER_SET( RCSetWorldState, +SERVER );

	eWorldState GetLastServerAssignedWorldState()			{ return m_LastServerAssignedWorldState; }
	void SetLastServerAssignedWorldState( eWorldState ws )	{ m_LastServerAssignedWorldState = ws; }

// Latency

FEX	void RCQueryLatency( float serverTime );		// server will query clients
FEX	void RSAckLatency( float serverTime );			// client will ack
	float GetAverageLatency();
	float GetInstantaneousLatency();

	AverageAccumulator * GetLatencyAverage()				{ return m_LatencyAverage; }

// World location

	void SSetWorldLocation( const gpstring & locationName );
FEX	FuBiCookie RCSetWorldLocation( unsigned int locationId );
	FUBI_MEMBER_SET( RCSetWorldLocation, +SERVER );

	gpwstring const & GetWorldLocation() const;

// Starting position.

FEX	FuBiCookie RSSetStartingGroup( const gpstring & sGroup );
FEX FuBiCookie RCSetStartingGroup( const gpstring & sGroup );
	FUBI_MEMBER_SET( RCSetStartingGroup, +SERVER );

	void SetStartingGroup( const gpstring & sGroup );
	gpstring const & GetStartingGroup() 				{ return m_sStartingGroup; }

	void SSetStartingPosition( const SiegePos& Pos, const CameraPosition& CamPos );
FEX FuBiCookie RCSetStartingPosition( const SiegePos& Pos, const CameraPosition& CamPos );
	FUBI_MEMBER_SET( RCSetStartingPosition, +SERVER );
	
	void SetStartingPosition( const SiegePos& Pos, const CameraPosition& CamPos );

	const SiegePos& GetStartingPosition() const			{ return m_StartingPosition; }

	// Party saving.
FEX	void SetPartyDirty();
	bool CheckPartyDirty( bool ignoreMinTime );
	
	///////////////////////////////////////
	//	owned resources

	// reduntant list of player-owned resources
	Go* GetParty();

	void AddParty( Go * pParty );
	void RemoveParty( Go * pParty );
	const GopColl& GetPartyColl( void ) const			{ return m_Parties; }

// Loot filtering options/manipulation
	
	eLootFilter BuildLootFilterFlags	( Goid loot );

	// Loot pickup filtering options
FEX	void		RSSetLootPickupFilter		( eLootFilter ltf );
FEX	void		SetLootPickupFilter			( eLootFilter ltf )					{ m_LootFilterPickup = ltf; }		
FEX eLootFilter GetLootPickupFilter			( void );
FEX eLootFilter BuildLootPickupFilter		( void );
FEX	void		SetLootPickupFilterOption	( eLootFilter options , bool bSet );
FEX	bool		GetLootPickupFilterOption	( eLootFilter options );
	void		SetLootPickupFilterDefaults	( void );
FEX	bool		IsAllowedLootPickup			( Goid loot );	

	// Item label viewing options/manipulation
FEX	void		RSSetLootLabelFilter		( eLootFilter ltf );
FEX	void		SetLootLabelFilter			( eLootFilter ltf )					{ m_LootFilterLabel = ltf; }
FEX eLootFilter GetLootLabelFilter			( void );
FEX eLootFilter BuildLootLabelFilter		( void );
FEX	void		SetLootLabelFilterOption	( eLootFilter options , bool bSet );
FEX	bool		GetLootLabelFilterOption	( eLootFilter options );
	void		SetLootLabelFilterDefaults	( void );
FEX	bool		IsAllowedLootLabel			( Goid loot );	

	// these querries are only valid between human players
	bool GetIsEnemy(	Player * player );
	bool GetIsFriend(	Player * player );

	bool GetIsEnemy(	Go * go );
	bool GetIsFriend(	Go * go );
	bool GetIsNeutral( Go * go );

	// todo: the player is where we will store or point to records of all events that transpire in the game

	void SetIsDirty();
	bool GetIsDirty()									{ return m_IsDirty; }

	void SSyncOnMachine( DWORD MachineId, bool fullXfer );

// Gold Picking Up and Trading functions. Not on inventory because these functions are for keeping state
// during actions.

	bool GetIsTrading() const							{ return m_bIsTrading; }
	void SetIsTrading( bool x )							{ m_bIsTrading = x; }
	Goid GetTradeGold() const							{ return m_TradeGold; };
	void ClearTradeGold()								{ m_TradeGold = GOID_INVALID; };

	// trading gold
FEX FuBiCookie RSSetTradeGoldAmount   ( int gold );

	int		GetTradeGoldAmount()						{ return m_TradeGoldAmount; }
	void	SSetTradeGoldAmount( int gold );
FEX	void	RCSetTradeGoldAmount( int gold, DWORD machineId );
	FUBI_MEMBER_SET( RCSetTradeGoldAmount, +SERVER );

	enum eTradeState
	{
		TRADE_NONE,
		TRADE_PROCESSING,
		TRADE_VALID,
		TRADE_INVALID,		
	};

	void		SetTradeState( eTradeState tradeState )	{ m_TradeState = tradeState; }
	eTradeState GetTradeState()							{ return m_TradeState; }

	void		SetTradeSource( Goid tradeSource )		{ m_TradeSource = tradeSource; }
	Goid		GetTradeSource()						{ return m_TradeSource; }
	
	void		SetTradeDest( Goid tradeDest )			{ m_TradeDest = tradeDest; }
	Goid		GetTradeDest()							{ return m_TradeDest; }


private:

	void SDirtyPlayersForSync();
	void InitData();

	////////////////////////////////////////
	//	sync

FEX FuBiCookie RCSetReadyToPlay( bool flag );
	FUBI_MEMBER_SET( RCSetReadyToPlay, +SERVER );

FEX FuBiCookie RCSyncOnMachineHelper( DWORD MachineId, const_mem_ptr syncBlock, bool fullXfer );
	void XferForSync( FuBi::PersistContext& persist );

FEX void RSSetLoadProgress( float ratio );
FEX void RCSetLoadProgress( float ratio );	// echo state to clients
	FUBI_MEMBER_SET( RCSetLoadProgress, +SERVER );

	////////////////////////////////////////////////////////////////////////////////
	//	data

	static int		ms_HumanPlayerCount;

	bool	m_bInitialized;
	bool	m_IsDirty;
	bool	m_bMarkedForSyncedDeletion;

	////////////////////////////////////////
	//	id

	gpwstring         m_sName;     
	PlayerId          m_Id;        
	DWORD             m_MachineId; 
	ePlayerController m_Controller;

	////////////////////////////////////////
	//	startup

	FrustumId		m_InitialFrustumId;
	SiegePos		m_StartingPosition;
	CameraPosition	m_StartingCameraPosition;
	gpstring		m_sStartingGroup;  
	bool			m_bReadyToPlay;
	bool			m_bJIP;
	bool			m_bZone;
	Goid			m_Stash;					// Goid of the stash object the player has access to

	////////////////////////////////////////
	//	hero

	bool		m_bAddedPlayerToWorld;
	gpwstring	m_HeroName;                   
	Goid     	m_HeroGoid;                   
	gpstring 	m_HeroCloneSourceTemplateName;
	gpstring	m_StashCloneSourceTemplateName;

	gpstring 	m_ImportData;
	gpstring	m_ImportStashData;
	gpstring 	m_HeroSkins[ PS_COUNT ];
	gpstring 	m_HeroHead;
	DWORD 	 	m_HeroPortrait;
	float		m_HeroUberLevel;

	bool        m_bIsTrading;        
	Goid        m_TradeGold;          // global Go created to use as necessary item for trading and picking up
	int         m_TradeGoldAmount;    // how  much gold we want to trade on complete of trade
	eTradeState	m_TradeState;
	Goid		m_TradeDest;
	Goid		m_TradeSource;

	////////////////////////////////////////
	//	party

	GopColl 	m_Parties;
	float       m_MaxPartySavePeriod;
	float       m_MinPartySavePeriod;
	double      m_LastPartySave;     
	bool        m_PartySaveDirty;    

	////////////////////////////////////////
	//	misc state

	eWorldState m_WorldState;                  
	eWorldState m_LastServerAssignedWorldState;

	float       m_LoadRatio;                   
	double		m_LastLoadRatioTime;
	double      m_CheckScreenPlayerVisibleTime;

	DWORD       m_FriendTo;                    
	DWORD       m_FriendToMe;     
	
	bool			m_FrustumIsOwned;

	unsigned int	m_WorldLocation;


	eLootFilter	m_LootFilterPickup;
	eLootFilter m_LootFilterLabel;

	////////////////////////////////////////
	//	net

	AverageAccumulator * m_LatencyAverage;

	static DWORD	PlayerToNet( Player * p );
	static Player *	NetToPlayer( DWORD p, FuBiCookie* cookie );

	FUBI_RPC_CLASS( Player, PlayerToNet, NetToPlayer, "Player - represents each player inside the game." );

	SET_NO_COPYING( Player );
};


////////////////////////////////////////////////////////////////////////////////
//
bool IsValid( Player * player );




#endif
