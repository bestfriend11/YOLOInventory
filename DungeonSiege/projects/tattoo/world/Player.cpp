#include "precomp_world.h"

#include "ContentDb.h"
#include "FileSysUtils.h"
#include "FuBiPersistImpl.h"
#include "FuBiTraits.h"
#include "FuelDb.h"
#include "InputBinder.h"
#include "Player.h"
#include "StdHelp.h"
#include "Server.h"
#include "GoDefs.h"
#include "Go.h"
#include "GoActor.h"
#include "GoAspect.h"
#include "GoInventory.h"
#include "GoMind.h"
#include "GoParty.h"
#include "GoMagic.h"
#include "NetPipe.h"
#include "NetFuBi.h"
#include "World.h"
#include "WorldState.h"
#include "WorldOptions.h"
#include "WorldMap.h"
#include "WorldTerrain.h"
#include "WorldTime.h"
#include "siege_camera.h"
#include "siege_frustum.h"
#include "StdHelp.h"
#include "WorldFx.h"
#include "AppModule.h"
#include "MCP.h"
#include "siege_mouse_shadow.h"


FUBI_EXPORT_BITFIELD_ENUM( eLootFilter );

// String/enum conversions for WorldOptions

static const stringtool::BitEnumStringConverter::Entry s_LootFilterStrings[] =
{		
	{	"ltf_invalid",				LTF_INVALID,			},
	{	"ltf_misc",					LTF_MISC,				},
	{	"ltf_normal_weapons",		LTF_NORMAL_WEAPONS		},
	{	"ltf_enchanted_weapons",	LTF_ENCHANTED_WEAPONS	},
	{	"ltf_normal_armor",			LTF_NORMAL_ARMOR		},
	{	"ltf_enchanted_armor",		LTF_ENCHANTED_ARMOR		},
	{	"ltf_jewelry",				LTF_JEWELRY				},
	{	"ltf_quest_items",			LTF_QUEST_ITEMS			},
	{	"ltf_health_potions",		LTF_HEALTH_POTIONS		},
	{	"ltf_mana_potions",			LTF_MANA_POTIONS		},
	{	"ltf_spells",				LTF_SPELLS				},
	{	"ltf_gold",					LTF_GOLD				},
	{	"ltf_lorebooks",			LTF_LOREBOOKS			},
	{	"ltf_all",					LTF_ALL					},
};

static stringtool::BitEnumStringConverter
	s_LootFilterConverter( s_LootFilterStrings, ELEMENT_COUNT( s_LootFilterStrings ), TT_ACTOR );

const char* ToString( eLootFilter e )
{
	return ( s_LootFilterConverter.ToString( e ) );
}

gpstring ToFullString( eLootFilter e )
{
	return ( s_LootFilterConverter.ToFullString( e ) );
}

bool FromString( const char* str, eLootFilter & e )
{
	return ( s_LootFilterConverter.FromString( str, rcast <DWORD&> ( e ) ) );
}

bool FromFullString( const char* str, eLootFilter & e )
{
	return ( s_LootFilterConverter.FromFullString( str, rcast <DWORD&> ( e ) ) );
}

//---------------------------------------------

bool IsValid( Player * player )
{
	return( player && player->IsInitialized() && !player->IsMarkedForDeletion() );
}


DWORD s_ScreenPlayerVisibleCount = 0;
int Player :: ms_HumanPlayerCount = 0;

Player :: Player( PlayerId id )
{
	InitData();
	m_Id = id;
	m_sName.assignf( L"'placeholder %d'", MakeIndex( id ) );
}


Player :: Player(	const gpwstring&  name,
					unsigned int      machineId,
					ePlayerController controller,
					PlayerId          playerId )
{
	InitData();

	m_bInitialized	= true;      
	m_sName			= name;      
	m_Id			= playerId;  
	m_MachineId		= machineId; 
	m_Controller	= controller;

	// take id
	gpassert( IsPower2( MakeInt( m_Id ) ) );

	// am i the screen player? first human player always is...
	if( (m_Controller == PC_HUMAN) && IsLocal() )
	{
		gpassert( gServer.GetScreenPlayer() == NULL );
		gServer.SetScreenPlayer( this );
	}

	// am i human? track if so
	if ( m_Controller == PC_HUMAN )
	{
		++ms_HumanPlayerCount;
	}

	// take a random hero name
	const char* hero = gContentDb.GetRandomHeroName();
	if ( hero != NULL )
	{
		m_HeroCloneSourceTemplateName = hero;
	}

#if !GP_RETAIL
	if( controller != PC_COMPUTER )
	{
		gpassertm( playerId != PLAYERID_COMPUTER, "Looks like the computer player wasn't created." );
	}
#endif

	// configuration
	m_MaxPartySavePeriod = 120;
	m_MinPartySavePeriod = 10;
	FastFuelHandle config( "config:global_settings:save_game_settings" );
	if ( config )
	{
		config.Get( "max_party_save_period", m_MaxPartySavePeriod );
		config.Get( "min_party_save_period", m_MinPartySavePeriod );
	}

	if( gServer.IsMultiPlayer() )
	{
		m_LatencyAverage = new AverageAccumulator( 40 );
	}
}


Player :: ~Player()
{
	if( m_bInitialized )
	{
		// nuke all go's belonging to me
		gGoDb.DeletePlayerGos( GetId() );

		// was i human? track if so
		if ( m_Controller == PC_HUMAN )
		{
			--ms_HumanPlayerCount;
		}
		
		// was i the screen player?
		if( gServer.GetScreenPlayer() == this )
		{
			gServer.SetScreenPlayer( NULL );
		}

		// kill my starting frustum
		if ( (m_Controller == PC_HUMAN) && (m_InitialFrustumId != FRUSTUMID_INVALID) )
		{
			gWorldTerrain.DestroyWorldFrustum( m_InitialFrustumId );
		}

		if( m_FrustumIsOwned == false )
		{
			gGoDb.RemoveFrustumFromPlayer( GetId() );
		}
		
		// release portrait if any
		SetHeroPortrait( 0, false );

		m_bInitialized = false;

		if( m_LatencyAverage )
		{
			Delete( m_LatencyAverage );
		}
	}
}


void Player :: InitData()
{
	m_bInitialized                 = false;
	m_bMarkedForSyncedDeletion	   = false;
	m_IsDirty                      = true;        
	m_sName                        = L"unknown";   
	m_Id                           = 0;           
	m_MachineId                    = 0;           
	m_Controller                   = PC_INVALID;  
	m_InitialFrustumId             = FRUSTUMID_INVALID;
	m_bReadyToPlay                 = false;       
	m_bJIP                         = false;       
	m_bZone                        = false;       
	m_bAddedPlayerToWorld		   = false;
	m_HeroGoid                     = GOID_INVALID;
	m_HeroPortrait                 = 0;           
	m_HeroUberLevel                = 0.0f;        
	m_bIsTrading                   = false;       
	m_TradeGold                    = GOID_INVALID;
	m_TradeGoldAmount              = 0;           
	m_TradeState                   = TRADE_NONE;  
	m_TradeDest                    = GOID_INVALID;
	m_TradeSource                  = GOID_INVALID;
	m_LastPartySave                = 0;           
	m_PartySaveDirty               = false;       
	m_WorldState                   = WS_INVALID;  
	m_LastServerAssignedWorldState = WS_INVALID;  
	m_LoadRatio                    = 0;           
	m_LastLoadRatioTime            = 0;           
	m_CheckScreenPlayerVisibleTime = 0;           
	m_FriendTo                     = 0;           
	m_FriendToMe                   = 0;           
	m_WorldLocation                = 0;           
	m_FrustumIsOwned               = true;
	m_LatencyAverage			   = NULL;
	m_Stash						   = GOID_INVALID;
	m_LootFilterPickup			   = LTF_INVALID;
	m_LootFilterLabel			   = LTF_INVALID;
}


bool Player :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_sName",                        m_sName                        );
	persist.Xfer( "m_bReadyToPlay",                 m_bReadyToPlay                 );
	persist.Xfer( "m_WorldState",                   m_WorldState                   );
	persist.Xfer( "m_CheckScreenPlayerVisibleTime", m_CheckScreenPlayerVisibleTime );
	persist.Xfer( "m_MachineId",                    m_MachineId                    );
	persist.Xfer( "m_HeroName",                     m_HeroName                     );
	persist.Xfer( "m_HeroGoid",                     m_HeroGoid                     );
	persist.Xfer( "m_HeroCloneSourceTemplateName",  m_HeroCloneSourceTemplateName  );
	persist.Xfer( "m_StashCloneSourceTemplateName",	m_StashCloneSourceTemplateName );
	persist.Xfer( "m_HeroHead",                     m_HeroHead                     );
	persist.Xfer( "m_StartingPosition",             m_StartingPosition             );
	persist.Xfer( "m_sStartingGroup",               m_sStartingGroup               );
	persist.Xfer( "m_TradeGold",                    m_TradeGold                    );
	persist.Xfer( "m_WorldLocation",				m_WorldLocation				   );
	persist.Xfer( "m_Stash",						m_Stash						   );

	persist.XferExistingColl( "m_HeroSkins", m_HeroSkins, ARRAY_END( m_HeroSkins ) );

	if ( persist.IsRestoring() )
	{
		m_IsDirty = false;
	}

	return ( true );
}


void Player :: Update()
{
	GPPROFILERSAMPLE( "Player :: Update", SP_MISC );

	//----- update screen player visibility

	Player * player = gServer.GetScreenPlayer();

	if ( this == player )
	{
		if ( gWorldTime.GetTime() >= m_CheckScreenPlayerVisibleTime )
		{
			Go * party = gServer.GetScreenParty();

			if ( party )
			{
				gServer.SetScreenPlayerVisibleCount( gServer.GetScreenPlayerVisibleCount() + 1 );

				for( GopColl::const_iterator i = party->GetChildBegin(); i != party->GetChildEnd(); ++i )
				{
					if ( !(*i)->HasMind() )
					{
						gpwarningf(( "Mindless party member found.  Member '%s'\n", (*i)->GetTemplateName() ));
						continue;
					}
					// go through enemies
					GoidColl::const_iterator ib = (*i)->GetMind()->GetEnemiesVisible().begin();
					GoidColl::const_iterator ie = (*i)->GetMind()->GetEnemiesVisible().end();
					GoidColl::const_iterator j;
					for( j = ib; j != ie; ++j )
					{
						GoHandle visible( *j );
						if ( visible && visible->HasAspect() )
						{
							visible->GetAspect()->SetIsScreenPlayerVisibleCount( gServer.GetScreenPlayerVisibleCount() );
						}
					}

					// go through friends
					ib = (*i)->GetMind()->GetFriendsVisible().begin();
					ie = (*i)->GetMind()->GetFriendsVisible().end();

					for( j = ib; j != ie; ++j )
					{
						GoHandle visible( *j );
						if ( visible && visible->HasAspect() )
						{
							visible->GetAspect()->SetIsScreenPlayerVisibleCount( gServer.GetScreenPlayerVisibleCount() );
						}
					}

					// go through items
					ib = (*i)->GetMind()->GetItemsVisible().begin();
					ie = (*i)->GetMind()->GetItemsVisible().end();

					for( j = ib; j != ie; ++j )
					{
						GoHandle visible( *j );
						if ( visible && visible->HasAspect() )
						{
							visible->GetAspect()->SetIsScreenPlayerVisibleCount( gServer.GetScreenPlayerVisibleCount() );
						}
					}
				}
			}
			m_CheckScreenPlayerVisibleTime = PreciseAdd( gWorldTime.GetTime(), 0.5 );
		}
	}
}


bool Player :: IsMarkedForDeletion()
{
	return gServer.IsMarkedForDeletion( GetId() );
}


void Player :: SetIsDirty()
{
	m_IsDirty = true;
	WorldMessage( WE_PLAYER_DATA_CHANGED, GOID_INVALID, GOID_ANY, MakeInt( GetId() ) ).Send();
}


FuBiCookie Player :: RSSetName( gpwstring const & name )
{
	FUBI_RPC_THIS_CALL_RETRY( RSSetName, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	RCSetName( name );

	return RPC_SUCCESS;
}


FuBiCookie Player :: RCSetName( gpwstring const & name )
{
#if !GP_RETAIL

	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}

#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSetName, RPC_TO_ALL );

	m_sName = name;

	return RPC_SUCCESS;
}


bool Player :: IsScreenPlayer() const
{
	return ( gServer.GetScreenPlayer() == this );
}


FuBiCookie Player :: RSSetReadyToPlay( bool flag )
{
	FUBI_RPC_THIS_CALL_RETRY( RSSetReadyToPlay, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;
	gpassert( gWorld.IsMultiPlayer() );

	// a player may only request to change this state if the server is in the staging area

	if( ( gWorldState.GetCurrentState() == WS_MP_STAGING_AREA_SERVER ) || 
		( gWorldState.GetPendingState() == WS_MP_STAGING_AREA_CLIENT ) || 
		IsInGame( gWorldState.GetCurrentState() ) )
	{
		RCSetReadyToPlay( flag );
		return RPC_SUCCESS;
	}
}


FuBiCookie Player :: RCSetReadyToPlay( bool flag )
{
#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSetReadyToPlay, RPC_TO_ALL );

	m_bReadyToPlay = flag;

	SetIsDirty();

	WorldMessage( flag ? WE_MP_PLAYER_READY : WE_MP_PLAYER_NOT_READY, GOID_INVALID, GOID_ANY, MakeInt( GetId() ) ).Send();

	return RPC_SUCCESS;
}


FuBiCookie Player :: RCSetJIP( bool flag )
{
#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSetJIP, RPC_TO_ALL );

	m_bJIP = flag;

	return RPC_SUCCESS;
}


FuBiCookie Player :: RSSetIsOnZone( bool flag )
{
	FUBI_RPC_THIS_CALL_RETRY( RSSetIsOnZone, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	RCSetIsOnZone( flag );

	return RPC_SUCCESS;
}


FuBiCookie Player :: RCSetIsOnZone( bool flag )
{

#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSetIsOnZone, RPC_TO_ALL );

	m_bZone = flag;

	SetIsDirty();

	return RPC_SUCCESS;
}


void Player :: SSetStartingPosition( const SiegePos& Pos, const CameraPosition& CamPos )
{
	SetStartingPosition( Pos, CamPos );

	if ( IsRemote() )
	{
		RCSetStartingPosition( Pos, CamPos );
	}
}


FuBiCookie Player :: RCSetStartingPosition( const SiegePos& Pos, const CameraPosition& CamPos )
{
	FUBI_RPC_TAG();

	gpassertm( Pos.node.IsValid(), "Player :: RCSetStartingPosition - Invalid starting position node." );
	gpgenericf(( "Player :: RCSetStartingPosition - for player %S\n", GetName().c_str() ));

#if !GP_RETAIL
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( GetMachineId(), GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSetStartingPosition, GetMachineId() );

	SetStartingPosition( Pos, CamPos );

	return RPC_SUCCESS;
}


void Player :: SetStartingPosition( const SiegePos& Pos, const CameraPosition& CamPos )
{
	m_StartingPosition			= Pos;
	m_StartingCameraPosition	= CamPos;
	SetIsDirty();
}


void Player :: SCreateInitialFrustum()
{
	CHECK_SERVER_ONLY;

	gpassert( m_InitialFrustumId == FRUSTUMID_INVALID );

	if ( m_Controller == PC_HUMAN )
	{
		// give me a starting frustum
		gWorldTerrain.SRegisterWorldFrustum( m_InitialFrustumId, this );

		RCSetInitialFrustum( RPC_TO_LOCAL, m_InitialFrustumId, m_StartingPosition, m_StartingCameraPosition );
		if( IsRemote() )
		{
			RCSetInitialFrustum( GetMachineId(), m_InitialFrustumId, m_StartingPosition, m_StartingCameraPosition );
		}

		gGoDb.SOnInitialFrustumRegistered( this );
		m_FrustumIsOwned = false;
	}
}


void Player :: RCSetInitialFrustum( DWORD machineId, FrustumId frustumId, const SiegePos& fpos, const CameraPosition& cpos )
{
#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( machineId, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL( RCSetInitialFrustum, machineId );

	// all player instances

	SetInitialFrustum( frustumId );

	siege::SiegeFrustum* frustum = gSiegeEngine.GetFrustum( MakeInt( frustumId ) );
	gpassert( frustum != NULL );
	frustum->SetPosition( fpos );
	frustum->SetDimensions( gWorldMap.GetWorldFrustumRadius(), gWorldMap.GetWorldFrustumDepth(), gWorldMap.GetWorldFrustumHeight() );
	frustum->SetInterestRadius( gWorldMap.GetWorldInterestRadius() );

	// only for player on target RPC machine

	if( IsLocal() )
	{
		gSiegeEngine.GetCamera().SetCameraAndTargetPosition( cpos.cameraPos.pos, cpos.targetPos.pos );
		gSiegeEngine.GetCamera().UpdateCamera();
	}
}


void Player :: SetInitialFrustum( FrustumId frustumId )
{
	m_InitialFrustumId = frustumId;
	SetIsDirty();
}


void Player :: SAddPlayerToWorld( bool createHero )
{
	CHECK_SERVER_ONLY;

	if ( GetController() != PC_HUMAN )
	{
		return;
	}

	gpassert( !m_bAddedPlayerToWorld );

	gpgenericf(( "Player :: SAddPlayerToWorld - adding party for %S\n", GetName().c_str() ));

	// verify some stuff
	gpassertm( m_StartingPosition.node.IsValid(), "Player created with invalid starting position!" )
	gpassertm( !m_HeroCloneSourceTemplateName.empty(), "Player created with no hero clone source template name!" );

	// make sure no parties got loaded (should be impossible, but you know...)
	gpassertm( m_Parties.empty(), "Player created and somehow we have parties!" );

	// create starting party
	GoCloneReq partyReq( gContentDb.GetDefaultPartyHumanoidTemplate(), GetId() );
	partyReq.SetStartingPos( m_StartingPosition );
	partyReq.SetSnapToTerrain( true );
	partyReq.m_AllClients = true;
	Goid partyGoid = gGoDb.SCloneGo( partyReq );
	gpassert( IsValid( partyGoid ) );

	// give frustum to the party
	GoHandle party( partyGoid );
	party->GetParty()->TakeFrustum( m_InitialFrustumId );

	m_FrustumIsOwned = true;

	// create hero spec
	if ( createHero )
	{
		gpgenericf(( "Player :: SAddPlayerToWorld - adding hero for %S\n", GetName().c_str() ));

		GoCloneReq heroReq;
		heroReq.SetStartingPos( m_StartingPosition );
		heroReq.SetSnapToTerrain( true );
		heroReq.m_PlayerId = m_Id;
		heroReq.m_Parent = partyGoid;
		heroReq.m_AllClients = true;

		// make sure we recalc everything (#9802).
		heroReq.m_XferCharacterPost = true;

		// create the hero
		const gpstring& templateName = GetHeroCloneSourceTemplate();
		m_HeroGoid = gGoDb.SCloneGo( heroReq, templateName );

		// configure
		GoHandle hero( m_HeroGoid );
		if ( hero )
		{
			// if we don't have a hero name, then use this guy's screen name
			if ( m_HeroName.empty() )
			{
				m_HeroName = hero->GetCommon()->GetScreenName();
			}

			// if not an imported character
			if ( templateName[ 0 ] != ':' )
			{
				// set the name same as player
				hero->GetCommon()->SSetScreenName( GetHeroName() );

				// set its textures
				gpstring* i, * ibegin = m_HeroSkins, * iend = ARRAY_END( m_HeroSkins );
				for ( i = ibegin ; i != iend ; ++i )
				{
					if ( !i->empty() )
					{
						hero->GetAspect()->RCSetDynamicTexture( (ePlayerSkin)(i - ibegin), *i );
					}
				}

				if ( !m_HeroHead.empty() )
				{
					hero->GetInventory()->SSetCustomHead( m_HeroHead );
				}				
			}
			
			// Set the loot filter defaults because this is a new character or it has never had the loot filter, 
			// otherwise they won't pick up any items or see any labels on the ground.
			{
				if ( BuildLootPickupFilter() == LTF_INVALID )
				{				
					SetLootPickupFilterDefaults();				
				}

				if ( BuildLootLabelFilter() == LTF_INVALID )
				{
					SetLootLabelFilterDefaults();
				}
			}

			// always tell client to use saved portrait for this
			hero->GetActor()->RCUpdatePortraitTexture();
		}

		SCreateTradeGold();		

		// Create the stash go
		SCreateStash();		
	}	

	// force the godb to commit these guys
	gGoDb.CommitAllRequests();

	// since multiplayer player originally created at matchmaking, sync up again here
	if ( m_MachineId != RPC_TO_LOCAL )
	{
		SSyncOnMachine( m_MachineId, false );
	}	

	m_bAddedPlayerToWorld = true;
}


void Player :: SCreateTradeGold( void )
{
	// $$$ FUTURE: make it so gold is not a child of the hero, and is instead a
	//     child of the party. this will be necessary for multiplayer with multi-
	//     character parties supported. if one guy dies with the party gold can't
	//     be having it drop on the ground. we can't fix this yet because of the
	//     work involved migrating it. lots of code assumes the party's children
	//     are always party members, and gold will fuck them up. find a better
	//     way... perhaps a "hidden" flag that will get skipped when iterating
	//     children??

	// this gold is for trading
	if ( m_TradeGold == GOID_INVALID )
	{
		GoCloneReq goldReq;
		goldReq.m_PlayerId   = m_Id;
		goldReq.m_Parent     = m_HeroGoid;		// parent used for indentifying owning member during trades
		goldReq.m_AllClients = true;			// can broadcast information to all clients
		goldReq.m_OmniGo     = true;			// this object doesn't have physical presence in the world

		// create the gold
		m_TradeGold = gGoDb.SCloneGo( goldReq, gContentDb.GetDefaultGoldTemplate() );
		if ( m_TradeGold != GOID_INVALID )
		{
			GoHandle gold( m_TradeGold );
			gold->GetAspect()->SSetIsVisible( false );
		}
	}
}


void Player :: SetLoadProgress( float ratio )
{
	if( PreciseSubtract(::GetGlobalSeconds(), m_LastLoadRatioTime) > 0.5 )
	{
		RSSetLoadProgress( ratio );
		m_LastLoadRatioTime = ::GetGlobalSeconds();
	}
}


void Player :: RSSetLoadProgress( float ratio )
{
	FUBI_RPC_THIS_CALL( RSSetLoadProgress, RPC_TO_SERVER );
	RCSetLoadProgress( ratio );
}


void Player :: RCSetLoadProgress( float ratio )
{
	FUBI_RPC_THIS_CALL( RCSetLoadProgress, RPC_TO_ALL );
	m_LoadRatio = ratio;
}


void Player :: RSSetHeroName( gpwstring const & name )
{
	FUBI_RPC_THIS_CALL( RSSetHeroName, RPC_TO_SERVER );
	RCSetHeroName( name );
}


void Player :: SCreateStash( void )
{	
	CHECK_SERVER_ONLY;

	if ( m_StashCloneSourceTemplateName.empty() )
	{
		m_StashCloneSourceTemplateName = gContentDb.GetDefaultStashTemplate();
	}

	GoCloneReq stashReq;
	stashReq.m_OmniGo		= true;
	stashReq.m_AllClients	= true;
	stashReq.m_PlayerId		= GetId();		
	m_Stash = gGoDb.SCloneGo( stashReq, m_StashCloneSourceTemplateName );
	
	RCSetStash( m_Stash );
}

void Player :: RCSetStash( Goid stash )
{
	FUBI_RPC_THIS_CALL( RCSetStash, RPC_TO_ALL );
	m_Stash = stash;	
}


FuBiCookie Player :: RCSetHeroName( gpwstring const & name )
{
#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSetHeroName, RPC_TO_ALL );

	SetHeroName( name );

	return RPC_SUCCESS;
}


void Player :: SetHeroName( gpwstring const & name )
{
	m_HeroName = name;

	SetIsDirty();
}


FuBiCookie Player :: RSSetHeroCloneSourceTemplate( gpstring const & name )
{
	FUBI_RPC_TAG();

	gpgenericf(( "Player :: RSSetHeroCloneSourceTemplate - player %S request set to '%s'\n", GetName().c_str(), name.c_str() ));

	// set it locally immediately - this is required because the next thing
	// the client is going to do is query the hero's clone source template
	// name, which requires that this commit immediately.
	SetHeroCloneSourceTemplate( name );

	FUBI_RPC_THIS_CALL_RETRY( RSSetHeroCloneSourceTemplate, RPC_TO_SERVER );

	// $$$ security

	return RCSetHeroCloneSourceTemplate( name );
}


FuBiCookie Player :: RCSetHeroCloneSourceTemplate( gpstring const & name )
{
#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSetHeroCloneSourceTemplate, RPC_TO_ALL );

	SetHeroCloneSourceTemplate( name );

	return RPC_SUCCESS;
}


void Player :: SetHeroCloneSourceTemplate( gpstring const & name )
{
	gpgenericf(( "Player :: SetHeroCloneSourceTemplate - player %S set to '%s'\n", GetName().c_str(), name.c_str() ));
	m_HeroCloneSourceTemplateName = name;

	SetIsDirty();

}


void Player :: RSSetStashCloneSourceTemplate( gpstring const & name )
{
	FUBI_RPC_TAG(); 

	SetStashCloneSourceTemplate( name );

	FUBI_RPC_THIS_CALL( RSSetStashCloneSourceTemplate, RPC_TO_SERVER );

	RCSetStashCloneSourceTemplate( name );
}


void Player :: RCSetStashCloneSourceTemplate( gpstring const & name )
{
	FUBI_RPC_THIS_CALL( RCSetStashCloneSourceTemplate, RPC_TO_ALL );

	SetStashCloneSourceTemplate( name );
}


void Player :: SetStashCloneSourceTemplate( gpstring const & name )
{
	m_StashCloneSourceTemplateName = name;

	SetIsDirty();
}


FuBiCookie Player :: RSSetHeroSkin( ePlayerSkin skin, gpstring const & name )
{
	FUBI_RPC_THIS_CALL_RETRY( RSSetHeroSkin, RPC_TO_SERVER );

	// $$$ security

	return RCSetHeroSkin( skin, name );
}


FuBiCookie Player :: RCSetHeroSkin( ePlayerSkin skin, gpstring const & name )
{
#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSetHeroSkin, RPC_TO_ALL );

	m_HeroSkins[ skin ] = name;

	SetIsDirty();

	return RPC_SUCCESS;
}


FuBiCookie Player :: RSSetHeroHead( gpstring const & name )
{
	FUBI_RPC_THIS_CALL_RETRY( RSSetHeroHead, RPC_TO_SERVER );

	// $$$ security

	return RCSetHeroHead( name );
}


FuBiCookie Player :: RCSetHeroHead( gpstring const & name )
{
#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSetHeroHead, RPC_TO_ALL );

	m_HeroHead = name;

	SetIsDirty();

	return RPC_SUCCESS;
}


void Player :: SetHeroPortrait( DWORD texture, bool addRef )
{
	if ( m_HeroPortrait != texture )
	{
		if ( m_HeroPortrait != 0 )
		{
			gDefaultRapi.DestroyTexture( m_HeroPortrait );
		}

		m_HeroPortrait = texture;

		if ( addRef && m_HeroPortrait )
		{
			SetIsDirty();
		}
	}
}


FuBiCookie Player :: RSSetHeroUberLevel( float level )
{
	FUBI_RPC_THIS_CALL_RETRY( RSSetHeroUberLevel, RPC_TO_SERVER );

	RCSetHeroUberLevel( level );

	gWorldMap.SVerifyValidPlayerStartingPositions();

	return RPC_SUCCESS;
}


FuBiCookie Player :: RCSetHeroUberLevel( float level )
{
#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSetHeroUberLevel, RPC_TO_ALL );

	SetHeroUberLevel( level );

	return RPC_SUCCESS;
}


void Player :: SetHeroUberLevel( float level )
{
	float oldLevel = m_HeroUberLevel;

	m_HeroUberLevel = level;

	if ( (int)oldLevel != (int)level )
	{
		WorldMessage( WE_MP_PLAYER_SET_CHARACTER_LEVEL, GOID_INVALID, GOID_ANY, MakeInt( GetId() ) ).Send();
	}

	SetIsDirty();
}


void Player :: RSImportCharacter( const gpstring& spec, int characterIndex )
{
	FUBI_RPC_TAG();

	gpnet( "Player :: RSImportCharacter\n" );

	// $ algo: import locally first then have the server tell everyone else
	//   to import. the goal here is to avoid having the client who is importing
	//   a character have to re-download what they just uploaded to server.

	RCImportCharacter( RPC_TO_LOCAL, spec, characterIndex );
	FUBI_RPC_THIS_CALL( RSImportCharacter, RPC_TO_SERVER );

	Server::PlayerColl::iterator i, ibegin = gServer.GetPlayersBegin(), iend = gServer.GetPlayersEnd();
	for ( i = ibegin ; i != iend ; ++i )
	{
		gpnetf(( "attempting to import : player id 0x%02x, name = %S, valid = %d, controller = %s", (*i)->GetId(), (*i)->GetName().c_str(), IsValid( *i ), ToString( (*i)->GetController() ) ));

		// tell everyone except the guy who RS'd
		if ( IsValid( *i ) && !(*i)->IsComputerPlayer() && ((*i)->GetId() != GetId()) )
		{
			gpnet( " - IMPORTING\n" );
			RCImportCharacter( (*i)->GetMachineId(), spec, characterIndex );
		}
		else
		{
			gpnet( " - SKIPPING\n" );
		}
	}

	SDirtyPlayersForSync();
}


void Player :: RCImportCharacter( DWORD machineId, const gpstring& spec, int characterIndex )
{
	FUBI_RPC_TAG();

#if !GP_RETAIL
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( machineId, GetId() );
	}
#endif

	gpnetf(( "Player :: RCImportCharacter - machineId = 0x%08x, index = 0x%08x\n", machineId, characterIndex ));

	FUBI_RPC_THIS_CALL( RCImportCharacter, machineId );

	m_ImportData = spec;

	// update imported character if any
	if ( !m_ImportData.empty() )
	{
		gpnet( "Player :: RCImportCharacter - calling GoDb :: ImportCharacter()\n" );
		gGoDb.ImportCharacter( GetId(), characterIndex, m_ImportData );
	}

	SetIsDirty();
}


void Player :: RSImportStash( const gpstring& spec )
{
	FUBI_RPC_TAG();

	// Let's send it to the caller first so we don't have to send it again to this machine when the server gets to it
	RCImportStash( GetMachineId(), spec );

	FUBI_RPC_THIS_CALL( RSImportStash, RPC_TO_SERVER );

	Server::PlayerColl::iterator i, ibegin = gServer.GetPlayersBegin(), iend = gServer.GetPlayersEnd();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// tell everyone except the guy who RS'd
		if ( IsValid( *i ) && !(*i)->IsComputerPlayer() && ((*i)->GetId() != GetId()) )
		{			
			RCImportStash( (*i)->GetMachineId(), spec );
		}		
	}

	SDirtyPlayersForSync();
}


void Player :: RCImportStash( DWORD machineId, const gpstring& spec )
{
	FUBI_RPC_THIS_CALL( RCImportStash, machineId );

	m_ImportStashData = spec;

	// update imported stash data
	if ( !m_ImportStashData .empty() )
	{
		gGoDb.ImportStash( GetId(), m_ImportStashData );
	}

	SetIsDirty();
}


FuBiCookie Player :: RSSetWorldState( eWorldState state )
{
	FUBI_RPC_THIS_CALL_RETRY( RSSetWorldState, RPC_TO_SERVER );
	RCSetWorldState( state );
	return RPC_SUCCESS;
}


FuBiCookie Player :: RCSetWorldState( eWorldState state )
{
	// $$ only the server needs to know everyone's worldstate
/*
#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}
#endif
*/
	FUBI_RPC_THIS_CALL_RETRY( RCSetWorldState, RPC_TO_ALL );

	m_WorldState = state;

	SetIsDirty();

	WorldMessage( WE_MP_PLAYER_WORLD_STATE_CHANGED, GOID_INVALID, GOID_ANY, MakeInt( GetId() ) ).Send();

	return RPC_SUCCESS;
}


void Player :: SSetWorldLocation( const gpstring & locationName )
{
	unsigned int id = gWorldMap.GetWorldLocationId( locationName );

	if ( id != m_WorldLocation )
	{
		RCSetWorldLocation( id );
	}
}


FuBiCookie Player :: RCSetWorldLocation( unsigned int locationId )
{
#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSetWorldLocation, RPC_TO_ALL );

	m_WorldLocation = locationId;

	if ( ::IsSinglePlayer() || gServer.GetLocalHumanPlayer()->GetTeam() == GetTeam() )
	{
		gpscreenf(( $MSG$ "%S has entered %S.", GetHeroName().c_str(), GetWorldLocation().c_str() ));
	}

	return RPC_SUCCESS;
}


void Player :: RCQueryLatency( float serverTime )
{
	FUBI_RPC_THIS_CALL( RCQueryLatency, GetMachineId() );

	RSAckLatency( serverTime );
}


void Player :: RSAckLatency( float serverTime )
{
	FUBI_RPC_THIS_CALL( RSAckLatency, RPC_TO_SERVER );

	if( m_LatencyAverage )
	{
		float latency = (float) ( PreciseSubtract( ::GetGlobalSeconds(), serverTime) ) + 0.016f;
		gpassert( latency > 0 );

		if( m_LatencyAverage->GetAverage() < latency )
		{
			m_LatencyAverage->Saturate( m_LatencyAverage->GetAverage() + ( ( latency - m_LatencyAverage->GetAverage() ) * 0.10f ) );
		}
		else
		{
			m_LatencyAverage->AddSample( latency );
		}
	}
}


float Player :: GetAverageLatency()
{
	if( m_LatencyAverage )
	{
		return min( 0.5f, (m_LatencyAverage->GetAverage() * 0.5f) );
	}
	else
	{
		return 0;
	}
}


float Player :: GetInstantaneousLatency()
{
	if( NetPipe::DoesSingletonExist() )
	{
		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
		if( session )
		{
			if( session->GetMachine( m_MachineId ) )
			{
				return session->GetMachine( m_MachineId )->m_Latency;
			}
		}
	}

	return 0;
}


gpwstring const & Player :: GetWorldLocation() const
{
	return ( gWorldMap.GetWorldLocationScreenName( m_WorldLocation ) );
}


FuBiCookie Player :: RSSetStartingGroup( const gpstring & sGroup )
{
	FUBI_RPC_THIS_CALL_RETRY( RSSetStartingGroup, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	WorldMap::SGroupColl::const_iterator i = gWorldMap.GetStartGroups().begin();
	for ( ; i != gWorldMap.GetStartGroups().end(); ++i )
	{
		if ( i->sGroup.same_no_case( sGroup ) )
		{
			FuBiCookie result = RCSetStartingGroup( sGroup );

			gWorldMap.SAssignStartingPositions();

			return( result );
		}
	}

	return RPC_FAILURE;
}


FuBiCookie Player :: RCSetStartingGroup( const gpstring & sGroup )
{
#if !GP_RETAIL
	FUBI_RPC_TAG();

	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSetStartingGroup, RPC_TO_ALL );

	SetStartingGroup( sGroup );

	return RPC_SUCCESS;
}


void Player :: SetStartingGroup( const gpstring & sGroup )
{
	m_sStartingGroup = sGroup;
	SetIsDirty();
}


void Player :: SetPartyDirty()
{
	// if we weren't dirty before set time to now so we don't save immediately
	if ( !m_PartySaveDirty )
	{
		m_LastPartySave = gWorldTime.GetTime();
	}

	m_PartySaveDirty = true;
}


bool Player :: CheckPartyDirty( bool ignoreMinTime )
{
	bool dirty = false;

	if ( ignoreMinTime && m_PartySaveDirty )
	{
		dirty = true;
	}
	else
	{
		float period = m_PartySaveDirty ? m_MinPartySavePeriod : m_MaxPartySavePeriod;
		if ( gWorldTime.GetTime() >= (PreciseAdd(m_LastPartySave, period)) )
		{
			dirty = true;
		}
	}

	if ( dirty )
	{
		m_PartySaveDirty = false;
		m_LastPartySave = gWorldTime.GetTime();
	}

	return ( dirty );
}


Go * Player :: GetParty()
{
	return m_Parties.empty() ? NULL : m_Parties.front();
}


void Player :: AddParty( Go * pParty )
{
	gpassert( !stdx::has_any( m_Parties, pParty ) );
	m_Parties.push_back( pParty );
}


void Player :: RemoveParty( Go * pParty )
{
	gpassert( stdx::has_any( m_Parties, pParty ) );
	m_Parties.erase( stdx::find( m_Parties, pParty ) );
}



eLootFilter Player :: BuildLootFilterFlags( Goid loot )
{
	eLootFilter lootType = LTF_INVALID;
	GoHandle hLoot( loot );
	if ( !hLoot.IsValid() )
	{
		return lootType;
	}
	
	lootType |= (hLoot->IsWeapon() && (!hLoot->HasMagic() ||
				 (hLoot->HasMagic() && 
				 !hLoot->GetMagic()->HasNonInnateEnchantments())))		? LTF_NORMAL_WEAPONS		: LTF_INVALID;
	lootType |= (hLoot->IsWeapon() && hLoot->HasMagic() &&
				 hLoot->GetMagic()->HasNonInnateEnchantments())			? LTF_ENCHANTED_WEAPONS		: LTF_INVALID;
	lootType |= (hLoot->HasDefend() && (!hLoot->HasMagic() ||
				 (hLoot->HasMagic() && 
				 !hLoot->GetMagic()->HasNonInnateEnchantments())))		? LTF_NORMAL_ARMOR			: LTF_INVALID;
	lootType |= (hLoot->HasDefend() && hLoot->HasMagic() &&
				 hLoot->GetMagic()->HasNonInnateEnchantments())			? LTF_ENCHANTED_ARMOR		: LTF_INVALID;
	lootType |= (hLoot->HasGui() &&										
				 (hLoot->GetGui()->GetEquipSlot() == ES_RING ||	
				 hLoot->GetGui()->GetEquipSlot() == ES_AMULET))			? LTF_JEWELRY				: LTF_INVALID;
	lootType |=	(hLoot->HasGui() && !hLoot->GetGui()->GetCanSell())		? LTF_QUEST_ITEMS			: LTF_INVALID;
	lootType |= (hLoot->HasMagic() && 
				 hLoot->GetMagic()->IsHealthPotion())					? LTF_HEALTH_POTIONS		: LTF_INVALID;
	lootType |= (hLoot->HasMagic() &&
				 hLoot->GetMagic()->IsManaPotion())						? LTF_MANA_POTIONS			: LTF_INVALID;
	lootType |= (hLoot->IsSpell())										? LTF_SPELLS				: LTF_INVALID;
	lootType |= (hLoot->IsGold())										? LTF_GOLD					: LTF_INVALID;				
	lootType |= (hLoot->HasGui() && hLoot->GetGui()->IsLoreBook())		? LTF_LOREBOOKS				: LTF_INVALID;
	
	// All valid items that don't have a loot filter from above are marked as a miscellaneous item.
	if ( lootType == LTF_INVALID )
	{
		lootType |= LTF_MISC;
	}
	
	return lootType;
}


void Player :: RSSetLootPickupFilter( eLootFilter ltf )
{
	FUBI_RPC_TAG();

	SetLootPickupFilter( ltf );

	FUBI_RPC_THIS_CALL( RSSetLootPickupFilter, RPC_TO_SERVER );

	gGoDb.SSetQuestString( GetHero(), "none", "loot_pickup_filter", ToFullString( ltf ) );

	m_LootFilterPickup = ltf;
}


eLootFilter Player :: BuildLootPickupFilter()	
{	
	eLootFilter ltf = LTF_INVALID;
	gpstring sFilter = gGoDb.GetQuestString( GetHero(), "none", "loot_pickup_filter" );
	if ( !sFilter.empty() )
	{
		if ( !FromFullString( sFilter, ltf ) )
		{
			ltf = LTF_INVALID;
		}
	}
	
	RSSetLootPickupFilter( ltf );

	return ( ltf );	
}


eLootFilter Player :: GetLootPickupFilter( void )
{
	if ( m_LootFilterPickup == LTF_INVALID )
	{
		// Must not be cached yet, build it now.
		BuildLootPickupFilter();
	}
	
	return m_LootFilterPickup;
}


void Player :: SetLootPickupFilterOption( eLootFilter options, bool bSet )	
{ 
	RSSetLootPickupFilter( (eLootFilter)( bSet ? (GetLootPickupFilter() | options) : (GetLootPickupFilter() & ~options)) );  
}


bool Player :: GetLootPickupFilterOption( eLootFilter options )		
{ 
	return ( (GetLootPickupFilter() & options) != 0 );  
}


void Player :: SetLootPickupFilterDefaults( void )
{
	RSSetLootPickupFilter( LTF_ALL );
}


bool Player :: IsAllowedLootPickup( Goid loot )
{
	eLootFilter options = BuildLootFilterFlags( loot );
	if ( options == LTF_INVALID )
	{
		return false;
	}

	return GetLootPickupFilterOption( options );
}


void Player :: RSSetLootLabelFilter( eLootFilter ltf )
{
	FUBI_RPC_TAG();

	SetLootLabelFilter( ltf );

	FUBI_RPC_THIS_CALL( RSSetLootLabelFilter, RPC_TO_SERVER );

	gGoDb.SSetQuestString( GetHero(), "none", "loot_label_filter", ToFullString( ltf ) );

	m_LootFilterLabel = ltf;
}


eLootFilter Player :: BuildLootLabelFilter()	
{
	eLootFilter ltf = LTF_INVALID;
	gpstring sFilter = gGoDb.GetQuestString( GetHero(), "none", "loot_label_filter" );
	if ( !sFilter.empty() )
	{
		if ( !FromFullString( sFilter, ltf ) )
		{
			ltf = LTF_INVALID;
		}
	}
	
	RSSetLootLabelFilter( ltf );

	return ( ltf );	
}


eLootFilter Player :: GetLootLabelFilter( void )
{
	if ( m_LootFilterLabel == LTF_INVALID )
	{
		// Must not be cached yet, build it now.
		BuildLootLabelFilter();
	}
	
	return m_LootFilterLabel;
}


void Player :: SetLootLabelFilterOption( eLootFilter options, bool bSet )	
{ 
	RSSetLootLabelFilter( (eLootFilter)( bSet ? (GetLootLabelFilter() | options) : (GetLootLabelFilter() & ~options)) );  
}


bool Player :: GetLootLabelFilterOption( eLootFilter options )		
{ 
	return ( (GetLootLabelFilter() & options) != 0 );  
}


void Player :: SetLootLabelFilterDefaults( void )
{
	RSSetLootLabelFilter( LTF_ALL );
}


bool Player :: IsAllowedLootLabel( Goid loot )
{
	eLootFilter options = BuildLootFilterFlags( loot );
	if ( options == LTF_INVALID )
	{
		return false;
	}

	return GetLootLabelFilterOption( options );
}


FuBiCookie Player :: RSSetTradeGoldAmount( int gold )
{
	FUBI_RPC_THIS_CALL_RETRY( RSSetTradeGoldAmount   , RPC_TO_SERVER );

	SSetTradeGoldAmount(gold);

	return (RPC_SUCCESS);
};


void Player :: SSetTradeGoldAmount( int gold )
{ 
	CHECK_SERVER_ONLY; 
	
	m_TradeGoldAmount = gold; 

	RCSetTradeGoldAmount( gold, GetMachineId() );
}


void Player :: RCSetTradeGoldAmount( int gold, DWORD machineId )
{
#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL( RCSetTradeGoldAmount, machineId );

	m_TradeGoldAmount = gold;
}


void Player :: SSyncOnMachine( DWORD MachineId, bool fullXfer )
{
	CHECK_SERVER_ONLY;

	gpnetf(( "Player :: SSyncOnMachine - machineId = 0x%08x, full = %d\n", MachineId, fullXfer ));

	FuBi::SerialBinaryWriter writer;
	FuBi::PersistContext persist( &writer, fullXfer );
	XferForSync( persist );

	gpassert( !IsComputerPlayer() );

	RCSyncOnMachineHelper( MachineId, writer.GetBuffer(), fullXfer );

	// $$ track who we've synced with

	if( fullXfer && NetPipe::DoesSingletonExist() && gNetPipe.HasOpenSession() )
	{
		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

		if( session )
		{
			NetPipeMachine * machine = session->GetMachine( MachineId );
			if( machine )
			{
				machine->m_PlayersSynced |= MakeInt( GetId() );
			}
		}
	}
}


void Player :: SDirtyPlayersForSync()
{
	CHECK_SERVER_ONLY;

	if( NetPipe::DoesSingletonExist() && gNetPipe.HasOpenSession() )
	{
		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

		if( session )
		{
			for ( NetPipeMachineColl::iterator i = session->GetMachinesBegin() ; i != session->GetMachinesEnd(); ++i )
			{
				gpassert( (*i)->IsRemote() );

				if( (*i)->IsRemote() )
				{
					NetPipeMachine * machine = *i;
					if( machine && !machine->m_LocalPlayerCreated )
					{
						gpgenericf(( "Player :: SDirtyPlayersForSync - dirtying %S, pid 0x%08x\n", (*i)->m_wsName.c_str(), (*i)->m_Id ));
						machine->m_PlayersSynced &= ~MakeInt( GetId() );
					}
				}
			}
		}
	}
}


FuBiCookie Player :: RCSyncOnMachineHelper( DWORD MachineId, const_mem_ptr syncBlock, bool fullXfer )
{
#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( MachineId, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSyncOnMachineHelper, MachineId );

	gpnetf(( "Player :: RCSyncOnMachineHelper - machineId = 0x%08x, full = %d\n", MachineId, fullXfer ));

	FuBi::SerialBinaryReader reader( syncBlock );
	FuBi::PersistContext persist( &reader, fullXfer );
	XferForSync( persist );

	// update imported character if any
	if ( !m_ImportData.empty() )
	{
		gpnet( "Player :: RCSyncOnMachineHelper - calling GoDb :: ImportCharacter()\n" );
		gGoDb.ImportCharacter( GetId(), 0, m_ImportData );
	}

	// update imported stash if any
	if ( !m_ImportStashData.empty() )
	{
		gpnet( "Player :: RCSyncOnMachineHelper - calling GoDb :: ImportStash()\n" );
		gGoDb.ImportStash( GetId(), m_ImportStashData );		
	}

	return ( RPC_SUCCESS );
}


void Player :: XferForSync( FuBi::PersistContext& persist )
{
	// update this as more stuff needs to be synced

	persist.Xfer( "", m_sName         );
	persist.Xfer( "", m_bReadyToPlay  );
	persist.Xfer( "", m_WorldState    );
	persist.Xfer( "", m_HeroName      );
	persist.Xfer( "", m_HeroUberLevel );
	persist.Xfer( "", m_HeroGoid      );
	persist.Xfer( "", m_HeroHead      );
	persist.Xfer( "", m_TradeGold     );
	persist.Xfer( "", m_FriendTo      );
	persist.Xfer( "", m_FriendToMe    );
	persist.Xfer( "", m_bJIP          );
	persist.Xfer( "", m_bZone         );
	persist.Xfer( "", m_WorldLocation );
	persist.Xfer( "", m_Stash		  );
	
	gpstring* i, * ibegin = m_HeroSkins, * iend = ARRAY_END( m_HeroSkins );
	for ( i = ibegin ; i != iend ; ++i )
	{
		persist.Xfer( "", *i );
	}

	if ( persist.IsFullXfer() )
	{
		persist.Xfer( "", m_HeroCloneSourceTemplateName		);
		persist.Xfer( "", m_StashCloneSourceTemplateName	);
		persist.Xfer( "", m_ImportData						);
		persist.Xfer( "", m_ImportStashData					);
		persist.Xfer( "", m_sStartingGroup					);
		persist.Xfer( "", m_StartingPosition				);
	}

	gpnet(  "Player :: XferForSync - post xfer\n" );
	gpnet(  "=================================\n" );

	gpnetf(( "m_sName         = %S\n",     m_sName.c_str()        ));
	gpnetf(( "m_bReadyToPlay  = %d\n",     m_bReadyToPlay         ));
	gpnetf(( "m_WorldState    = %s\n",     ToString(m_WorldState) ));
	gpnetf(( "m_HeroName      = %S\n",     m_HeroName             ));
	gpnetf(( "m_HeroUberLevel = %f\n",     m_HeroUberLevel        ));
	gpnetf(( "m_HeroGoid      = 0x%08x\n", m_HeroGoid             ));
	gpnetf(( "m_HeroHead      = %s\n",     m_HeroHead.c_str()     ));
	gpnetf(( "m_TradeGold     = 0x%08x\n", m_TradeGold            ));
	gpnetf(( "m_FriendTo      = 0x%08x\n", m_FriendTo             ));
	gpnetf(( "m_FriendToMe    = 0x%08x\n", m_FriendToMe           ));
	gpnetf(( "m_bJIP          = %d\n",     m_bJIP                 ));
	gpnetf(( "m_bZone         = %d\n",     m_bZone                ));
	gpnetf(( "m_WorldLocation = %d\n",     m_WorldLocation        ));
	gpnetf(( "m_Stash		  = 0x%08x\n", m_Stash				  ));	

	if ( persist.IsFullXfer() )
	{
		gpnetf(( "m_HeroCloneSourceTemplateName		= %s\n", m_HeroCloneSourceTemplateName.c_str()		));
		gpnetf(( "m_StashCloneSourceTemplateName	= %s\n", m_StashCloneSourceTemplateName.c_str()		));
		gpnetf(( "m_ImportData						= %s\n", m_ImportData.c_str()						));
		gpnetf(( "m_ImportStashData					= %s\n", m_ImportStashData.c_str()					));
		gpnetf(( "m_sStartingGroup					= %s\n", m_sStartingGroup.c_str()					));
		gpnetf(( "m_StartingPosition				= %s\n", ToString( m_StartingPosition ).c_str()		));
	}
}


bool Player :: GetIsEnemy( Go * go )
{
	if ( !go->HasAspect() || !go->HasMind() )
	{
		return false;
	}

	return go->GetActor()->GetAlignment() == AA_EVIL;
}


bool Player :: GetIsFriend( Go * go )
{
	if ( !go->HasAspect() || !go->HasMind() )
	{
		return false;
	}

	return go->GetActor()->GetAlignment() == AA_GOOD;
}


bool Player :: GetIsNeutral( Go * go )
{
	if ( !go->HasAspect() || !go->HasMind() )
	{
		return false;
	}

	return go->GetActor()->GetAlignment() == AA_NEUTRAL;
}


bool Player :: GetIsEnemy( Player * player )
{
	if ( gWorld.IsMultiPlayer() && !IsComputerPlayer() && !player->IsComputerPlayer() && player != this && gServer.GetAllowPlayerKills() )
	{
		return ( GetFriendTo() & MakeInt( player->GetId() ) ) == 0;
	}
	else
	{
		return false;
	}
}


bool Player :: GetIsFriend( Player * player )
{
	return !GetIsEnemy( player );
}


FuBiCookie Player :: RCSetFriendTo( DWORD friends )
{
#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSetFriendTo, RPC_TO_ALL );

	m_FriendTo = friends;
	return RPC_SUCCESS;
}


FuBiCookie Player :: RCSetFriendToMe( DWORD friends )
{
#if !GP_RETAIL
	FUBI_RPC_TAG();
	if( gServer.IsLocal() )
	{
		gServer.DebugCheckToldMachineToCreatePlayer( RPC_TO_ALL, GetId() );
	}
#endif

	FUBI_RPC_THIS_CALL_RETRY( RCSetFriendToMe, RPC_TO_ALL );

	m_FriendToMe = friends;
	return RPC_SUCCESS;
}


int Player :: GetTeam()
{
	Team * pTeam = gServer.GetTeam( m_Id );

	if ( pTeam )
	{
		return ( pTeam->m_Id );
	}
	
	return ( -1 );
}


void Player :: SSetFriendTo( DWORD friends )
{
	CHECK_SERVER_ONLY;

	RCSetFriendTo( friends );
}


void Player :: SSetFriendToMe( DWORD friends )
{
	CHECK_SERVER_ONLY;

	RCSetFriendToMe( friends );
}


DWORD Player :: PlayerToNet( Player * p )
{
	return MakeInt( p->GetId() );
}


Player * Player :: NetToPlayer( DWORD p, FuBiCookie* /*cookie*/ )
{
	return gServer.GetPlayerUnsafe( MakePlayerId( p ) );
}


