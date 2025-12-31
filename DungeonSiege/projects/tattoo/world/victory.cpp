/*=======================================================================================

	Victory

	author:		Adam Swensen
	creation:	8/28/2000

=======================================================================================*/

#include "precomp_world.h"
#include "BlindCallback.h"
#include "victory.h"

#include "fuel.h"
#include "gameauditor.h"
#include "server.h"
#include "stringtool.h"
#include "player.h"
#include "WorldTime.h"
#include "WorldMap.h"
#include "WorldState.h"
#include "WorldMessage.h"
#include "config.h"
#include "GoActor.h"
#include "GoAspect.h"
#include "GoMind.h"
#include "GoInventory.h"
#include "SkritEngine.h"
#include "gps_manager.h"

#include "FuBiPersistImpl.h"
#include "FuBiTraits.h"
#include "FileSysXfer.h"

#include "UIGame.h"
#include "UIPlayerRanks.h"

FUBI_EXPORT_ENUM( eGameStat, GS_BEGIN, GS_COUNT );

///////////////////////////////////////////////////////////////
//  Game Stats

static const char * s_GameStat[] =
{
	"kills",
	"kills_trend",
	"deaths",
	"gibs",
	"human_kills",
	"melee_gained",
	"ranged_gained",
	"cmagic_gained",
	"nmagic_gained",
	"experience_gained",
	"gold_gained",
	"loot_gained",
	"loot_value"
};

COMPILER_ASSERT( ELEMENT_COUNT( s_GameStat ) == GS_COUNT );

static stringtool::EnumStringConverter s_GSConverter( s_GameStat, GS_BEGIN, GS_END );

bool FromString( const char* str, eGameStat & gs )
{
	return ( s_GSConverter.FromString( str, rcast <int&> ( gs ) ) );
}

const char* ToString( eGameStat gs )
{
	return ( s_GSConverter.ToString( gs ) );
}


///////////////////////////////////////////////////////////////

SKRIT_IMPORT void GameHandleWorldMessage( Skrit::Object* skrit, const char* funcName, WorldMessage* /*msg*/ )
	{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_CALLV( GameHandleWorldMessage, skrit, funcName );  }
SKRIT_IMPORT void GameCheckTeamForVictory( Skrit::Object* skrit, const char* funcName, int /*team*/ )
	{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_CALLV( GameCheckTeamForVictory, skrit, funcName );  }


///////////////////////////////////////////////////////////////

SKRIT_IMPORT void QuestReward( Skrit::HObject skrit, int functionIndex, Goid )
{
	CHECK_PRIMARY_THREAD_ONLY;
	SKRIT_CALLV( QuestReward, skrit, functionIndex );
}


///////////////////////////////////////////////////////////////

Victory :: Victory()
{
	Init();
}


Victory :: ~Victory()
{
	for ( GameTypeColl::iterator i = m_GameTypes.begin(); i != m_GameTypes.end(); ++i )
	{
		delete *i;
		*i = NULL;
	}
}


void Victory :: Init( bool forNewMap )
{
//	Settings

	if ( !forNewMap )
	{
		m_TimeLimit = 0.0f;
	}

	m_LoadedMap.clear();

	m_GameEnded = false;

#	if !GP_RETAIL
	m_bAllowDefeat = gConfig.GetBool( "debug\\allow_defeat", true );
#	else
	m_bAllowDefeat = true;
#	endif

	m_bDefeated = false;

	m_EndGameTime = 0.0f;

	m_DefeatDelay = 4.0f;

	m_VictoryDelay = 2.5f;

	m_VictoryConditions.clear();

	m_QuestsChar.clear();

	m_TeamMode = NO_TEAMS;

	m_MpVictoryCallback = 0;

//	Quests

	m_questIdCounter			= 0;
	m_QuestUpdateCallback		= 0;
	m_activeQuest				= 0;
	m_updateQuestCompletion		= false;
	m_questCompletionActivator	= GOID_INVALID;
	m_sCompletedQuest.clear();
	m_Quests.clear();
	m_Chapters.clear();

//	Game Types

	for ( GameTypeColl::iterator i = m_GameTypes.begin(); i != m_GameTypes.end(); ++i )
	{
		delete *i;
		*i = NULL;
	}
	m_GameTypes.clear();

	m_pGameType = NULL;

//	Game Stats

	m_pPlayerStats = gGameAuditor.AddDb( "playerstats" );
	m_pTeamStats   = gGameAuditor.AddDb( "teamstats" );
	m_pGameStats   = gGameAuditor.AddDb( "gamestats" );	

	m_pEndGameAudit = gGameAuditor.AddDb( "endgame" );

	m_GameStatScreenNames.clear();
	m_GameStatScreenNames.push_back( gpwtranslate( $MSG$ "Kills" ) );
	m_GameStatScreenNames.push_back( gpwtranslate( $MSG$ "Kills Trend" ) );
	m_GameStatScreenNames.push_back( gpwtranslate( $MSG$ "Deaths" ) );
	m_GameStatScreenNames.push_back( gpwtranslate( $MSG$ "Gibs" ) );
	m_GameStatScreenNames.push_back( gpwtranslate( $MSG$ "PvP Kills" ) );
	m_GameStatScreenNames.push_back( gpwtranslate( $MSG$ "Melee" ) );
	m_GameStatScreenNames.push_back( gpwtranslate( $MSG$ "Ranged" ) );
	m_GameStatScreenNames.push_back( gpwtranslate( $MSG$ "Combat" ) );
	m_GameStatScreenNames.push_back( gpwtranslate( $MSG$ "Nature" ) );
	m_GameStatScreenNames.push_back( gpwtranslate( $MSG$ "Total" ) );
	m_GameStatScreenNames.push_back( gpwtranslate( $MSG$ "Gold Aquired" ) );
	m_GameStatScreenNames.push_back( gpwtranslate( $MSG$ "Loot Aquired" ) );
	m_GameStatScreenNames.push_back( gpwtranslate( $MSG$ "Value Loot" ) );

	m_PlayerKillTotal = 0;
	m_timelineMaxEntries = 3000; // this will give us 100 hours of play!
	m_timelineTimeDelta = 120; // this is in _seconds_
	
	FastFuelHandle hTimeLineParameters( "config:multiplayer:time_lines");
	if( hTimeLineParameters.IsValid() )
	{
		m_timelineMaxEntries = hTimeLineParameters.GetInt("time_line_history_length", m_timelineMaxEntries);
		m_timelineTimeDelta =  hTimeLineParameters.GetFloat("time_line_update_period_in_sec", m_timelineTimeDelta);
	}

	if( IsServerLocal() )
	{
		StatTimelineMap statTimeMap;
		statTimeMap[0] = StatTimeline( );

		m_PlayerStatTimelines[ "kills" ]			= statTimeMap;
		m_PlayerStatTimelines[ "kills_trend" ]		= statTimeMap;
		m_PlayerStatTimelines[ "deaths" ]			= statTimeMap;
		m_PlayerStatTimelines[ "human_kills" ]		= statTimeMap;
		m_PlayerStatTimelines[ "melee_gained" ]		= statTimeMap;
		m_PlayerStatTimelines[ "ranged_gained" ]	= statTimeMap;
		m_PlayerStatTimelines[ "cmagic_gained" ]	= statTimeMap;
		m_PlayerStatTimelines[ "nmagic_gained" ]	= statTimeMap;
		m_PlayerStatTimelines[ "experience_gained" ] = statTimeMap;
		m_PlayerStatTimelines[ "gold_gained" ]		= statTimeMap;
		m_PlayerStatTimelines[ "loot_gained" ]		= statTimeMap;
		m_PlayerStatTimelines[ "loot_value" ]		= statTimeMap;
	}
	else
	{
		m_PlayerStatTimelines.clear();
	}
}

// xfer our characters completed quests!
// this way when we load a game, it will always
// have the correct quests set!
FUBI_DECLARE_XFER_TRAITS( Victory::QuestCharMWColl )
{
	static bool XferDef( PersistContext &persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		return ( persist.XferMap( key, obj ) );
	}
};
FUBI_DECLARE_XFER_TRAITS( Victory::QuestCharMColl )
{
	static bool XferDef( PersistContext &persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		return ( persist.XferMap( key, obj ) );
	}
};
bool Victory::XferCharacter( FuBi::PersistContext& persist, const char* key )
{
	persist.EnterBlock( key );

	persist.XferMapAppend  ( "m_QuestsChar", m_QuestsChar );			
	persist.Xfer  ( "m_activeQuest", m_activeQuest );			
	
	persist.LeaveBlock();
	return ( true );
}


FUBI_DECLARE_XFER_TRAITS( Victory::QuestEvent )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		persist.EnterBlock( key );				
		persist.Xfer	  ( "sConvFuelAddress", obj.sConvFuelAddress );			
		persist.Xfer	  ( "sDescription",		obj.sDescription );			
		persist.Xfer	  ( "sSpeaker",			obj.sSpeaker );	
		persist.Xfer	  ( "order",			obj.order );	
		persist.Xfer	  ( "bRequired",		obj.bRequired );
		persist.Xfer	  ( "bPlayUpdate",		obj.bPlayUpdate );
		persist.LeaveBlock();
		return ( true );
	}
};


FUBI_DECLARE_XFER_TRAITS( Victory::Quest )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		persist.EnterBlock( key );		
		persist.XferVector( "events",				obj.events  );
		persist.Xfer	  ( "id",					obj.id );		
		persist.Xfer	  ( "bCompleted",			obj.bCompleted );		
		persist.Xfer	  ( "bActive",				obj.bActive );	
		persist.Xfer	  ( "bViewed",				obj.bViewed );
		persist.Xfer	  ( "currentOrder",			obj.currentOrder );		
		persist.Xfer	  ( "sName",				obj.sName );						
		persist.Xfer	  ( "sSkrit",				obj.sSkrit );		
		persist.Xfer	  ( "sVictorySample",		obj.sVictorySample );		
		persist.Xfer	  ( "sScreenName",			obj.sScreenName );				
		persist.Xfer	  ( "sChapter",				obj.sChapter );							
		persist.Xfer	  ( "sQuestImage",			obj.sQuestImage );
		persist.Xfer	  ( "sActiveDesc",			obj.sActiveDesc );			
		persist.LeaveBlock();
		return ( true );
	}
};


FUBI_DECLARE_XFER_TRAITS( Victory::ChapterEvent )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		persist.EnterBlock( key );				
		persist.Xfer	  ( "sDescription",		obj.sDescription );			
		persist.Xfer	  ( "sSample",			obj.sSample );			
		persist.Xfer	  ( "order",			obj.order );			
		persist.LeaveBlock();
		return ( true );
	}
};


FUBI_DECLARE_XFER_TRAITS( Victory::Chapter )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		persist.EnterBlock( key );	
		persist.Xfer	  ( "sName",			obj.sName );			
		persist.Xfer	  ( "sScreenName",		obj.sScreenName );			
		persist.Xfer	  ( "sImage",			obj.sImage );			
		persist.Xfer	  ( "currentOrder",		obj.currentOrder );			
		persist.Xfer	  ( "bActive",			obj.bActive );	
		persist.XferVector( "events",			obj.events );
		persist.LeaveBlock();
		return ( true );
	}
};

FUBI_DECLARE_SELF_TRAITS( Victory::StatEntry );

bool Victory::StatEntry :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "Time", Time );
	persist.Xfer( "Value", Value );
	return ( true );
}

bool Victory :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_LoadedMap",				m_LoadedMap );
	persist.Xfer( "m_bDefeated",				m_bDefeated );
	persist.Xfer( "m_GameEnded",				m_GameEnded );
	persist.Xfer( "m_TimeLimit",				m_TimeLimit );
	persist.Xfer( "m_questIdCounter",			m_questIdCounter );
	persist.Xfer( "m_activeQuest",				m_activeQuest );
	persist.Xfer( "m_sCompletedQuest",			m_sCompletedQuest );
	persist.Xfer( "m_updateQuestCompletion",	m_updateQuestCompletion );
	persist.Xfer( "m_questCompletionActivator", m_questCompletionActivator );
	persist.XferVector( "m_VictoryConditions",	m_VictoryConditions );
	persist.XferVector( "m_Quests",				m_Quests );	
	persist.XferVector( "m_Chapters",			m_Chapters );

	return true;
}


void Victory :: SSyncOnMachine( DWORD machineId )
{
	CHECK_SERVER_ONLY;

	// set all the rankings, stats that can be visible on the screen, for the specified client.
	{
		AuditorDb::GameEntryColl & stats = m_pPlayerStats->GetEntries();
		for ( AuditorDb::GameEntryColl::iterator stat = stats.begin(); stat != stats.end(); ++stat )
		{
			RCSetPlayerStat(MakeStatId( stat->m_Key.c_str() ), stat->m_Tag, stat->m_Int, machineId );
		}
	}
	{
		AuditorDb::GameEntryColl & stats = m_pTeamStats->GetEntries();
		for ( AuditorDb::GameEntryColl::iterator stat = stats.begin(); stat != stats.end(); ++stat )
		{
			RCSetTeamStat( MakeStatId( stat->m_Key.c_str() ), stat->m_Tag, stat->m_Int, machineId );
		}
	}

	// sync quest information on machine
	{
		FuBi::SerialBinaryWriter writer;
		FuBi::PersistContext persist( &writer );
		XferForSyncQuests( persist );
		RCSyncQuests( machineId, writer.GetBuffer() );
	}
	
	// sync end game timelines!
	{
		// get a basis for xfer
		FuelHandle fuel( "::import:root" );
		fuel = fuel->CreateChildBlock( "EndGameTimelines", "EndGameTimelines" );

		FuBi::FuelWriter baseWriter( fuel );
		FuBi::PersistContext persist( &baseWriter );

		XferForSyncEndGameTimeline( persist );

		gpstring syncBlock;
		FileSys::StringWriter writer( syncBlock );
		fuel->GetChildBlock( "EndGameTimelines" )->Write( writer );		

		RCSyncEndGameTimeline( machineId, syncBlock );
	}
}


void Victory :: LoadFromMap()
{
	// preserve selected game type across map loads, if possible
	gpstring  selectedGameType = GetGameType() ? GetGameType()->GetName() : "";
	eTeamMode selectedTeamMode = m_TeamMode;

	Init( true );

	m_LoadedMap = gWorldMap.GetMapName();
	if ( m_LoadedMap.empty() )
	{
		return;
	}

	FastFuelHandle hMap( gWorldMap.MakeMapDirFuel() );
	gpassertm( hMap.IsValid(), "The map isn't set." );

//	Load Quests

	FastFuelHandleColl quests;
	gWorldMap.GetAllQuestsFuel( quests );
	for ( FastFuelHandleColl::iterator i = quests.begin(); i != quests.end(); ++i )
	{
		Quest quest( m_questIdCounter++ );

		// First get the name
		quest.sName = i->GetName();

		// Get the chapter this quest belongs to
		i->Get( "chapter", quest.sChapter );

		// Now get and translate the name the user sees
		gpstring sScreenName;
		i->Get( "screen_name",  sScreenName );
		quest.sScreenName = ReportSys::TranslateW( sScreenName );

		// Get the quest completion sound
		i->Get( "victory_sample", quest.sVictorySample );

		// Get the quest image
		i->Get( "quest_image", quest.sQuestImage );

		if ( i->Get( "skrit", quest.sSkrit ) )
		{
			quest.sSkrit.assignf( "world\\maps\\%s\\quests\\%s.skrit", m_LoadedMap.c_str(), quest.sSkrit.c_str() );
		}

		// Retrieve all the quest events.
		FastFuelHandleColl hlEvents;
		FastFuelHandleColl::iterator iEvent;
		i->ListChildren( hlEvents, 1 );
		for ( iEvent = hlEvents.begin(); iEvent != hlEvents.end(); ++iEvent )
		{
			QuestEvent event;
			iEvent->Get( "description", event.sDescription );
			event.sDescription = ReportSys::TranslateW( event.sDescription );
			
			iEvent->Get( "order",		event.order );

			event.bRequired = false;
			iEvent->Get( "required",	event.bRequired );

			iEvent->Get( "address", event.sConvFuelAddress );

			iEvent->Get( "speaker", event.sSpeaker );
			event.sSpeaker = ReportSys::TranslateW( event.sSpeaker );

			event.bPlayUpdate = true;
			iEvent->Get( "play_update_sound", event.bPlayUpdate );

			quest.events.push_back( event );
		}
		
		m_Quests.push_back( quest );
	}

	// Load chapter info
	FastFuelHandleColl chapters;
	gWorldMap.GetAllChaptersFuel( chapters );
	for ( i = chapters.begin(); i != chapters.end(); ++i )
	{
		Chapter chapter;

		chapter.sName = (*i).GetName();

		i->Get( "screen_name", chapter.sScreenName );
		chapter.sScreenName = ReportSys::TranslateW( chapter.sScreenName );

		i->Get( "chapter_image", chapter.sImage );

		FastFuelHandleColl hlEvents;
		FastFuelHandleColl::iterator iEvent;
		i->ListChildren( hlEvents, 1 );
		for ( iEvent = hlEvents.begin(); iEvent != hlEvents.end(); ++iEvent )
		{
			ChapterEvent event;
			iEvent->Get( "description", event.sDescription );
			event.sDescription = ReportSys::TranslateW( event.sDescription );
			
			iEvent->Get( "order", event.order );
			
			iEvent->Get( "sample", event.sSample );		
	
			chapter.events.push_back( event );
		}

		chapter.bActive = false;
		chapter.currentOrder = 0;

		m_Chapters.push_back( chapter );
	}	

//	Load Game Types
	FastFuelHandle hMapInfo = hMap.GetChildNamed( "info" );
	if ( !hMapInfo )
	{
		gperrorf(( "VERY BAD - Map: %s does not have an 'info' directory defined.", hMap.GetName() )); 
		return;
	}

	FastFuelHandle hGameType = hMapInfo.GetChildNamed( "gametypes" );
	if ( hGameType.IsValid() )
	{
		hGameType.Get( "default_gametype", m_DefaultGameType );

		FastFuelHandleColl hGameTypeColl;
		hGameType.ListChildrenTyped( hGameTypeColl, "gametype" );
		for ( FastFuelHandleColl::iterator i = hGameTypeColl.begin(); i != hGameTypeColl.end(); ++i )
		{
			GameType * pGameType = NULL;

			for ( GameTypeColl::iterator j = m_GameTypes.begin(); j != m_GameTypes.end(); ++j )
			{
				if ( (*j)->m_Name.same_no_case( (*i).GetName() ) )
				{
					pGameType = *j;
					pGameType->Load( *i );
					break;
				}
			}

			if ( !pGameType )
			{
				pGameType = new GameType;

				FastFuelHandle hGameTypeDef( gpstring( "world:global:gametypes:" ).append( (*i).GetName() ) );
				if ( !hGameTypeDef.IsValid() )
				{
					gperrorf(( "Found game type '%s' in map with no definition at 'world:global:gametypes'.", (*i).GetName() ));
					continue;
				}

				GameType * pGameType = new GameType;
				if ( !pGameType->LoadDefinition( hGameTypeDef ) ||
					 !pGameType->Load( *i ) )
				{
					Delete( pGameType );
					continue;
				}

				pGameType->m_Id = m_GameTypes.size();

				m_GameTypes.push_back( pGameType );
			}

			if ( pGameType && pGameType->m_Name.same_no_case( m_DefaultGameType ) )
			{
				m_pGameType = pGameType;
			}
		}
	}

	if ( m_GameTypes.empty() )
	{
		// No game types in map, load automatic types.

		FastFuelHandle hGameTypes( "world:global:gametypes" );
		if ( hGameTypes.IsValid() )
		{
			FastFuelHandleColl hGameTypeColl;

			hGameTypes.ListChildrenTyped( hGameTypeColl, "gametype" );

			for ( FastFuelHandleColl::iterator i = hGameTypeColl.begin(); i != hGameTypeColl.end(); ++i )
			{
				if ( (*i).GetBool( "automatic", false ) )
				{
					GameType * pGameType = new GameType;

					if ( pGameType->LoadDefinition( *i ) )
					{
						pGameType->m_Id = m_GameTypes.size();

						m_GameTypes.push_back( pGameType );
					}
					else
					{
						Delete( pGameType );
					}
				}
			}
		}
	}

	if ( m_GameTypes.empty() )
	{
		// Last resort.  Create basic default type.

		GameType * pGameType = new GameType;
		pGameType->m_ScreenName = gpwtranslate( $MSG$ "Default" );
		m_GameTypes.push_back( pGameType );
	}

	if ( !selectedGameType.empty() )
	{
		if ( SetGameType( selectedGameType ) )
		{
			m_TeamMode = selectedTeamMode;
		}
	}

	if ( !m_pGameType )
	{
		// No default game set by map.

		m_pGameType = *m_GameTypes.begin();
	}

//	Load Victory Conditions

	ConditionColl victoryConditions;
	FastFuelHandle hVictory = hMapInfo.GetChildNamed( "victory" );
	if ( hVictory.IsValid() )
	{
		hVictory.Get( "victory_delay", m_VictoryDelay );
		hVictory.Get( "defeat_delay", m_DefeatDelay );

		FastFuelHandleColl hVictoryConditions;
		hVictory.ListChildrenNamed( hVictoryConditions, "condition*" );
		for ( FastFuelHandleColl::iterator i = hVictoryConditions.begin(); i != hVictoryConditions.end(); ++i )
		{
			VictoryCondition condition;
			condition.Load( *i );
			victoryConditions.push_back( condition );
			m_VictoryConditions.push_back( condition );
		}
	}

	// Add map's victory conditions to necessary game types
	for ( GameTypeColl::iterator iGameType = m_GameTypes.begin(); iGameType != m_GameTypes.end(); ++iGameType )
	{
		if ( !(*iGameType)->m_bIgnoreMapVictoryConditions )
		{
			for ( ConditionColl::iterator iCondition = victoryConditions.begin(); iCondition != victoryConditions.end(); ++iCondition )
			{
				(*iGameType)->m_VictoryConditions.push_back( *iCondition );
			}
		}
	}
}


void Victory :: Update()
{
	GPPROFILERSAMPLE( "Victory :: Update", SP_MISC );

	if ( GameEnded() )
	{
		return;
	}

	// check to see if the time limit has been hit
	if ( ( m_TimeLimit > 0 ) && ( GetGameTimeRemaining() <= 0 ) )
	{
		RSSetGameEnded( true );
		return;
	}

	// check for single player defeat
	if ( ::IsSinglePlayer() )
	{
		if ( m_bDefeated && m_bAllowDefeat && (m_DefeatDelay >= 0) )
		{
			m_DefeatDelay -= gWorldTime.GetSecondsElapsed();
			if ( m_DefeatDelay <= 0 )
			{
				gWorldStateRequest( WS_SP_DEFEAT );
			}
		}
		else if ( !m_bDefeated && gServer.GetScreenPlayer() )
		{
			// verify that everyone in the player's party is alive
			Go * pParty = gServer.GetScreenParty();
			if ( pParty )
			{
				bool allDead = true;
	 			GopColl partyColl = pParty->GetChildren();
				for ( GopColl::iterator i = partyColl.begin(); i != partyColl.end(); ++i )
				{
					if ( (*i)->IsAnyHumanPartyMember() && !(*i)->GetInventory()->IsPackOnly() && IsConscious( (*i)->GetLifeState() ) )
					{
						allDead = false;
						break;
					}
				}

				if ( allDead && m_bAllowDefeat )
				{
					if ( ::IsSinglePlayer() )
					{
						for ( GopColl::iterator i = partyColl.begin(); i != partyColl.end(); ++i )
						{
							if ( (*i)->IsAnyHumanPartyMember() && !(*i)->GetInventory()->IsPackOnly() && !IsConscious( (*i)->GetLifeState() ) )
							{
								WorldMessage( WE_KILLED, GOID_INVALID, (*i)->GetGoid(), false ).Send();
								(*i)->GetAspect()->SSetLifeState( LS_DEAD_NORMAL );
								WorldMessage msgEngaged( WE_ENGAGED_KILLED );
								(*i)->GetMind()->MessageEngagedMe( msgEngaged );
							}
						}
					}
					
					GuiDefeatCallback();
					m_bDefeated = true;
					//m_DefeatDelay = 4.0f;
				}
			}
		}
	}
	else if ( ::IsMultiPlayer() )
	{
		if( ::IsServerLocal() )
		{
			// these are just the values we need a history for.
			RCUpdateAllPlayerHistory();

			UpdatePlayerHistoryStats( "kills_trend" );
			UpdatePlayerHistoryStats( "kills" );
			UpdatePlayerHistoryStats( "deaths" );
			UpdatePlayerHistoryStats( "human_kills" );
			UpdatePlayerHistoryStats( "melee_gained" );
			UpdatePlayerHistoryStats( "ranged_gained" );
			UpdatePlayerHistoryStats( "cmagic_gained" );
			UpdatePlayerHistoryStats( "nmagic_gained" );
			UpdatePlayerHistoryStats( "experience_gained" );
			UpdatePlayerHistoryStats( "gold_gained" );
			UpdatePlayerHistoryStats( "loot_gained" );
			UpdatePlayerHistoryStats( "loot_value" );
		}
		CheckTeamsForVictory();		
	}

	// check victory conditions
	if ( !m_VictoryConditions.empty() )
	{
		bool bVictory = true;

		for ( ConditionColl::const_iterator i = m_VictoryConditions.begin(); i != m_VictoryConditions.end(); ++i )
		{
			if ( (i->m_Met > 0) && i->m_bExclusive )
			{
				bVictory = true;
				break;
			}
			else if ( i->m_Met == 0 )
			{
				bVictory = false;
			}
		}

		if ( bVictory )
		{
			m_VictoryDelay -= gWorldTime.GetSecondsElapsed();
			if ( m_VictoryDelay <= 0 )
			{
				RSSetGameEnded( true );
			}
			return;
		}
	}

	if ( ::IsServerLocal() )
	{
		UpdateQuestCompletion();

		// give time to game type skrit
		if ( m_pGameType != NULL )
		{
			m_pGameType->m_Skrit->Poll( gWorldTime.GetSecondsElapsed(), true );
		}
	}
}


void Victory :: HandleWorldMessage( WorldMessage & message )
{
	CHECK_SERVER_ONLY;

	// check game skrit
	if ( !m_pGameType || (m_pGameType->m_Skrit == NULL) )
	{
		return;
	}

	::GameHandleWorldMessage( m_pGameType->m_Skrit, "handle_world_message$", &message );
}


FuBiCookie Victory :: RSSetGameEnded( bool bSet )
{
	FUBI_RPC_CALL_RETRY( RSSetGameEnded, RPC_TO_SERVER )

	if ( gWorldMap.GetShowIntro() && ::IsMultiPlayer() )
	{
		RSSetMainCampaignWon( bSet );
		return RPC_SUCCESS;
	}

	if ( bSet )
	{			
		gWorldState.SSetWorldStateOnMachine( RPC_TO_ALL, WS_ANY, WS_GAME_ENDED );		
	}

	RCSetGameEnded( bSet );

	return RPC_SUCCESS;
}	


FuBiCookie Victory :: RCSetGameEnded( bool bSet )
{
	FUBI_RPC_CALL_RETRY( RCSetGameEnded, RPC_TO_ALL )

	SetGameEnded( bSet );

	return RPC_SUCCESS;
}


void Victory :: SetGameEnded( bool bSet )
{		
	m_GameEnded	= bSet;

	if ( bSet )
	{
		m_EndGameTime	= (float)gWorldTime.GetTime();

		// Store end game stats

		for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
		{
			if ( !IsValid(*i) || (*i)->IsComputerPlayer() )
			{
				continue;
			}

			Player * player = *i;

			Go * party = player->GetParty();
			if( party && !party->GetChildren().empty() )
			{
				GoHandle hero( *(party->GetChildren().begin()) );
				if ( hero )
				{
					m_pEndGameAudit->Set( MakeInt( player->GetId() ), "playerclass", ToAnsi( hero->GetActor()->GetClass() ) );
				}
			}
		}
	}
	else
	{
		m_VictoryConditions.clear();
	}
}

void Victory :: RSSetMainCampaignWon( bool bSet )
{
	FUBI_RPC_THIS_CALL( RSSetMainCampaignWon, RPC_TO_SERVER )

	RCSetMainCampaignWon( bSet );
}


void Victory :: RCSetMainCampaignWon( bool bSet )
{
	FUBI_RPC_THIS_CALL( RCSetMainCampaignWon, RPC_TO_ALL )
	
	if ( gWorldMap.GetShowIntro() && ::IsMultiPlayer() )	
	{
		if ( bSet )
		{
			GuiVictoryCallback();
		}
		else
		{
			m_VictoryConditions.clear();
		}

		return;
	}
}

void Victory :: ToggleAllowDefeat()
{
	m_bAllowDefeat = !m_bAllowDefeat;
	gConfig.SetBool( "debug\\allow_defeat", m_bAllowDefeat );
	gpgenericf(( "Defeat is %s\n", m_bAllowDefeat ? "ENABLED" : "DISABLED" ));
}


double Victory :: GetGameTimeRemaining()
{
	double remaining = PreciseSubtract(m_TimeLimit, gWorldTime.GetTime());
	return ( (remaining > 0.0) ? remaining : 0.0 );
}


double Victory :: GetGameTimeElapsed()
{
	return ( gWorldTime.GetTime() );
}


void Victory :: SSetTimeLimit( float timeLimit, DWORD machineId )
{
	CHECK_SERVER_ONLY;
	RCSetTimeLimit( timeLimit, machineId );
}


FuBiCookie Victory :: RCSetTimeLimit( float timeLimit, DWORD machineId )
{
	FUBI_RPC_CALL_RETRY( RCSetTimeLimit, machineId );

	m_TimeLimit = timeLimit;

	gServer.SetSettingsDirty();

	return RPC_SUCCESS;
}


///////////////////////////////////////////////////////////////

void Victory :: VictoryCondition :: Load( FastFuelHandle hCondition )
{
	hCondition.Get( "name", m_Name );
	hCondition.Get( "screen_name", m_ScreenName );
	hCondition.Get( "description", m_Description );

	hCondition.Get( "exclusive", m_bExclusive );

	gpstring value;
	hCondition.Get( "default_value", value );
	if ( !value.same_no_case( "none" ) )
	{
		m_Value = atoi( value.c_str() );
	}
}


FUBI_DECLARE_SELF_TRAITS( Victory::VictoryCondition );

bool Victory :: VictoryCondition :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_Name", m_Name );
	persist.Xfer( "m_Description", m_Description );
	persist.Xfer( "m_Met", m_Met );
	persist.Xfer( "m_bExclusive", m_bExclusive );
	persist.Xfer( "m_Value", m_Value );

	return true;
}

bool Victory :: IsVictoryConditionMet( const char * condition )
{
	for ( ConditionColl::iterator i = m_VictoryConditions.begin(); i != m_VictoryConditions.end(); ++i )
	{
		if ( i->m_Name.same_no_case( condition ) )
		{
			return ( i->m_Met > 0 );
		}
	}
	return false;
}

bool Victory :: IsVictoryConditionMet ( const char * condition, int team )
{
	for ( ConditionColl::iterator i = m_VictoryConditions.begin(); i != m_VictoryConditions.end(); ++i )
	{
		if ( i->m_Name.same_no_case( condition ) )
		{
			return ( (i->m_Met & (1 << team)) > 0 );
		}
	}
	return false;
}

void Victory :: SSetVictoryConditionMet( const char * condition, bool met )
{
	CHECK_SERVER_ONLY;

	RCSetVictoryConditionMet( condition, met );
}

void Victory :: RCSetVictoryConditionMet( const char * condition, bool met )
{
	FUBI_RPC_THIS_CALL( RCSetVictoryConditionMet, RPC_TO_ALL );

	SetVictoryConditionMet( condition, met );
}

void Victory :: SetVictoryConditionMet( const char * condition, bool met )
{
	for ( ConditionColl::iterator i = m_VictoryConditions.begin(); i != m_VictoryConditions.end(); ++i )
	{
		if ( i->m_Name.same_no_case( condition ) )
		{
			i->m_Met = met ? 1 : 0;
			break;
		}
	}
}

void Victory :: SSetTeamVictoryConditionMet( const char * condition, bool met, int team )
{
	CHECK_SERVER_ONLY;

	RCSetTeamVictoryConditionMet( condition, met, team );
}

void Victory :: RCSetTeamVictoryConditionMet( const char * condition, bool met, int team )
{
	FUBI_RPC_THIS_CALL( RCSetVictoryConditionMet, RPC_TO_ALL );

	SetTeamVictoryConditionMet( condition, met, team );
}

void Victory :: SetTeamVictoryConditionMet( const char * condition, bool met, int team )
{
	for ( ConditionColl::iterator i = m_VictoryConditions.begin(); i != m_VictoryConditions.end(); ++i )
	{
		if ( i->m_Name.same_no_case( condition ) )
		{
			if ( met )
			{
				i->m_Met |= (1 << team);
			}
			else
			{
				i->m_Met &= ~(1 << team);
			}
			break;
		}
	}
}

int Victory :: GetVictoryConditionValue( const char * condition )
{
	for ( ConditionColl::iterator i = m_VictoryConditions.begin(); i != m_VictoryConditions.end(); ++i )
	{
		if ( i->m_Name.same_no_case( condition ) )
		{
			return ( i->m_Value );
		}
	}
	return ( 0 );
}

bool Victory :: SetVictoryConditionValue( const char * condition, int value )
{
	for ( ConditionColl::iterator i = m_VictoryConditions.begin(); i != m_VictoryConditions.end(); ++i )
	{
		if ( i->m_Name.same_no_case( condition ) )
		{
			i->m_Value = value;

			return true;
		}
	}
	return false;
}


void Victory :: CheckTeamsForVictory()
{
	if ( !m_pGameType || (m_pGameType->m_Skrit == NULL) )
	{
		return;
	}

	Server::TeamIdColl usedTeams = gServer.GetUsedTeams();
	for ( Server::TeamIdColl::iterator team = usedTeams.begin(); team != usedTeams.end(); ++team )
	{
		::GameCheckTeamForVictory( m_pGameType->m_Skrit, "check_team_for_victory$", MakeInt( *team ) );
	}
}


///////////////////////////////////////////////////////////////
void Victory :: RSActivateQuest( const char * name, int order )
{
	FUBI_RPC_THIS_CALL( RSActivateQuest, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	RCActivateQuest( name, order );
}

void Victory :: RCActivateQuest( const char * name, int order )
{
	FUBI_RPC_THIS_CALL( RCActivateQuest, RPC_TO_ALL );

	ActivateQuest( name, order );	
}

void Victory :: ActivateQuest( const char * name, int order, bool bIgnoreCompletion, bool bSaveForCharacter )
{
	Quest* quest = FindQuestByName( name );
	if( !quest )
	{
		return;
	}

	if ( order == quest->currentOrder && ( !quest->bCompleted || bIgnoreCompletion ) )
	{				
		if ( !quest->bActive )
		{
			gpscreen( $MSG$ "A new quest has been added to your Journal." );										
			quest->bActive = true;
		}
		else
		{
			gpscreenf( ( $MSG$ "A quest has been updated in your Journal.", quest->sScreenName.c_str() ) );										
		}				

		bool bPlayUpdate = true;
		QuestEventColl::iterator iEvent;
		for ( iEvent = quest->events.begin(); iEvent != quest->events.end(); ++iEvent )
		{
			if ( (*iEvent).order == quest->currentOrder )
			{	
				bPlayUpdate = (*iEvent).bPlayUpdate;
				if ( !(*iEvent).sDescription.empty() )
				{
					quest->sActiveDesc = (*iEvent).sDescription;
				}						
			}
		}

		m_activeQuest = quest->id;

		quest->currentOrder++;	

		// set or saved out quest state! this is used for mp characters
		if( bSaveForCharacter &&  GetQuestCharOrder( quest->sName ) < quest->currentOrder )
		{
			SetQuestCharOrder(quest->sName, quest->currentOrder);
		}
		
		UpdateQuestCallback( quest->id );
		
		if ( gServer.GetScreenHero() && bPlayUpdate )
		{
			gServer.GetScreenHero()->PlayVoiceSound( "quest_update", false );
		}
	}
}
int Victory :: GetQuestCharOrder( )
{
	return m_QuestsChar[gWorldMap.GetMapName()][gWorldMap.GetMpWorldName()].begin()->second;
}

int Victory :: GetQuestCharOrder( const gpstring& questName )
{
	return GetQuestCharOrder( questName, gWorldMap.GetMpWorldName(), gWorldMap.GetMapName() );
}

int Victory :: GetQuestCharOrder( const gpstring& questName, const gpstring& worldName )
{
	return GetQuestCharOrder( questName, worldName, gWorldMap.GetMapName() );
}

int Victory :: GetQuestCharOrder( const gpstring& questName, const gpstring& worldName, const gpstring& mapName )
{
	return m_QuestsChar[mapName][worldName][questName];
}

void Victory :: SetQuestCharOrder( const gpstring& questName, int value )
{
	m_QuestsChar[gWorldMap.GetMapName()][gWorldMap.GetMpWorldName()][questName] = value;
}

void Victory :: RSDeactivateQuest( const char * name )
{
	FUBI_RPC_THIS_CALL( RSDeactivateQuest, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	RCDeactivateQuest( name );
}

void Victory :: RCDeactivateQuest( const char * name )
{
	FUBI_RPC_THIS_CALL( RCDeactivateQuest, RPC_TO_ALL );

	DeactivateQuest( name );
}

void Victory :: DeactivateQuest( const char * name )
{
	Quest* quest = FindQuestByName( name );
	
	if( !quest )
	{
		return;
	}

	quest->bActive = false;
}

void Victory :: UpdateQuestCompletion()
{
	if ( m_updateQuestCompletion )
	{
		if ( m_questCompletionActivator != GOID_INVALID )
		{
			RSUpdateQuestCompletion( m_sCompletedQuest, m_questCompletionActivator );								
		}

		m_updateQuestCompletion		= false;
		m_questCompletionActivator	= GOID_INVALID;
		m_sCompletedQuest.clear();
	}	
}

void Victory :: RSUpdateQuestCompletion( const char * name, Goid activator )
{
	FUBI_RPC_THIS_CALL( RSUpdateQuestCompletion, RPC_TO_SERVER );		

	Quest* quest = FindQuestByName( name );
	
	if( !quest )
	{
		return;
	}
	
	Skrit::CreateReq createReq( quest->sSkrit );			
	Skrit::HAutoObject hSkrit = Skrit::HObject( createReq );			
	QuestReward( hSkrit, 0, activator );

}

void Victory :: RSCompletedQuest( const char * name, Goid activator )
{
	FUBI_RPC_THIS_CALL( RSCompletedQuest, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;
	
	RCCompletedQuest( name, activator );
}

void Victory :: RCCompletedQuest( const char * name, Goid activator )
{
	FUBI_RPC_THIS_CALL( RCCompletedQuest, RPC_TO_ALL );

	CompletedQuest( name, activator );
}

void Victory :: CompletedQuest( const char * name, Goid activator, bool bSaveForCharacter )
{
	Quest* quest = FindQuestByName( name );		

	if( !quest )
	{
		return;
	}
	
	QuestEventColl::iterator iEvent;
	for ( iEvent = quest->events.begin(); iEvent != quest->events.end(); ++iEvent )
	{
		if ( (*iEvent).order >= quest->currentOrder && (*iEvent).bRequired )
		{
			return;
		}
	}			

	if ( quest->bActive && !quest->bCompleted )
	{
		gpscreen( $MSG$ "Quest completed." );
		m_activeQuest = quest->id;
		UpdateQuestCallback( quest->id );
		quest->bCompleted = true;
		if( bSaveForCharacter )
		{
			SetQuestCharOrder(quest->sName, quest->currentOrder + 1);
		}

		if ( !quest->sVictorySample.empty() )
		{
			gSoundManager.PlaySample( quest->sVictorySample );
		}	
		else if ( gServer.GetScreenHero() )
		{
			gServer.GetScreenHero()->PlayVoiceSound( "quest_complete", false );
		}

		if ( !quest->sSkrit.empty() )
		{
			m_updateQuestCompletion		= true;
			m_questCompletionActivator	= activator;
			m_sCompletedQuest			= name;
		}
	}
}

// if we have started any quests!
// this is only used in Mulitplayer, in SP, this always is false
bool Victory :: IsQuestCharActive( const gpstring & name )
{
	gpassert( ::IsMultiPlayer() );

	return  GetQuestCharOrder( name ) > 0;
}

// if we are on the last part of the quest!!!
// this is only used in Mulitplayer, in SP, this always is false
bool Victory :: IsQuestCharCompleted( const gpstring & name )
{
	gpassert( ::IsMultiPlayer() );
	Quest* quest = FindQuestByName( name );

	if( !quest )
	{
		return false;
	}

	// check to see if we completed this quest at any other sitting!
	QuestEventColl::iterator iEvent;
	
	for ( iEvent = quest->events.begin(); iEvent != quest->events.end(); ++iEvent )
	{
		if ( (*iEvent).order >= GetQuestCharOrder( name ) )
		{
			return false;
		}
	}		

	return true;
}

bool Victory :: IsQuestCompleted( const gpstring& name )
{
	Quest* quest = FindQuestByName( name );
	
	if( !quest )
	{
		return false;
	}

	return quest->bCompleted;			
}

bool Victory :: IsQuestActive( const gpstring & name )
{
	Quest* quest = FindQuestByName( name );		

	if( !quest )
	{
		return false;
	}

	return ( quest->bActive && !quest->bCompleted);			
	
}

int Victory :: GetQuestOrder( const gpstring & name )
{
	Quest* quest = FindQuestByName( name );		

	if( !quest )
	{
		return -1;
	}

	if ( quest->currentOrder == 0 )
	{
		return 0;
	}

	return quest->currentOrder-1;
}

void Victory :: RCSyncQuests( DWORD machineId, const_mem_ptr syncBlock )
{
	FUBI_RPC_THIS_CALL( RCSyncQuests, machineId );		
	
	FuBi::SerialBinaryReader reader( syncBlock );
	FuBi::PersistContext persist( &reader );
	XferForSyncQuests( persist );
}

void Victory :: XferForSyncQuests( FuBi::PersistContext& persist )
{

#if GP_DEBUG
	// Make sure client and hosts both have the same number of quests, otherwise the sync will fail.
	if ( ::IsServerLocal() )
	{
		int hostQuests = m_Quests.size();
		persist.Xfer( "", hostQuests );
	}
	else
	{
		unsigned int hostQuests = 0;
		persist.Xfer( "", hostQuests );
		if ( m_Quests.size() != hostQuests )
		{
			gperrorf( ( "Quest Data difference: Host has %d quests, while client only has %d quests", hostQuests, (int)m_Quests.size() ) );
		} 
	}
#endif
	QuestColl::iterator i;
	for ( i = m_Quests.begin(); i != m_Quests.end(); ++i )
	{
		persist.Xfer( "", (*i).bCompleted );
		persist.Xfer( "", (*i).bActive );
		persist.Xfer( "", (*i).currentOrder );

		if ( ::IsServerRemote() )
		{
			if ( (*i).currentOrder != 0 )
			{
				--(*i).currentOrder;
			}
			if ( (*i).bCompleted && (*i).bActive )
			{
				ActivateQuest( (*i).sName, (*i).currentOrder, true );
				CompletedQuest( (*i).sName, GOID_INVALID );
			}
			else if ( !(*i).bCompleted && (*i).bActive )
			{
				ActivateQuest( (*i).sName, (*i).currentOrder );
			}
		}
	}	
}

Victory::Quest* Victory :: FindQuestByName( const gpstring& name )
{
	for ( QuestColl::iterator i = m_Quests.begin(); i != m_Quests.end(); ++i )
	{
		if ( (*i).sName.same_no_case( name ) )
		{
			return i;
		}
	}
	// this is the end of the list!
	return NULL;
}

Victory::Quest* Victory :: FindQuestById( int id )
{
	for ( QuestColl::iterator i = m_Quests.begin(); i != m_Quests.end(); ++i )
	{
		if ( (*i).id == id )
		{
			return i;
		}
	}
	// this is the end of the list!
	return NULL;
}


bool Victory :: GetQuest( int id, Quest & quest )
{
	for ( QuestColl::iterator i = m_Quests.begin(); i != m_Quests.end(); ++i )
	{
		if ( (*i).id == id )
		{
			quest = (*i);
			return true;
		}
	}
	return false;
}

void Victory :: SetQuestViewed( int id, bool bViewed )
{
	for ( QuestColl::iterator i = m_Quests.begin(); i != m_Quests.end(); ++i )
	{
		if ( (*i).id == id )
		{
			(*i).bViewed = bViewed;
		}
	}
}

bool Victory :: GetQuestConversations( int id, QuestConversations & questConversations )
{
	bool bFound = false;
	for ( QuestColl::iterator i = m_Quests.begin(); i != m_Quests.end(); ++i )
	{
		if ( (*i).id == id )
		{
			QuestEventColl::iterator iEvent;
			for ( iEvent = (*i).events.begin(); iEvent != (*i).events.end(); ++iEvent )
			{
				// show the dialogue if we have talked to the person now, or ever as this character.
				if ( (*iEvent).order < (*i).currentOrder || (*iEvent).order < GetQuestCharOrder( (*i).sName ) )
				{
					gpstring sAddress = gWorldMap.MakeMapDirAddress();
					sAddress.appendf( ":regions:%s", (*iEvent).sConvFuelAddress );
					FastFuelHandle hConversation( sAddress );
					if ( hConversation.IsValid() )
					{
						FastFuelHandleColl hlTexts;
						hConversation.ListChildrenNamed( hlTexts, "text*" );
						FastFuelHandleColl::iterator iTexts;
						for ( iTexts = hlTexts.begin(); iTexts != hlTexts.end(); ++iTexts )
						{
							bool bQuestDialog = false;
							(*iTexts).Get( "quest_dialog", bQuestDialog );
							if ( bQuestDialog )
							{
								QuestConversation qc;
								(*iTexts).Get( "screen_text", qc.sConversation );
								(*iTexts).Get( "sample", qc.sSample );
								qc.sSpeaker = (*iEvent).sSpeaker;

								questConversations.push_back( qc );
								bFound = true;
								break;
							}
						}
					}
				}
			}			
		}
	}

	return bFound;
}


Victory::Chapter* Victory :: FindChapterByName( const gpwstring & sScreenName )
{
	for ( Victory::ChapterColl::iterator i = m_Chapters.begin(); i != m_Chapters.end(); ++i )
	{
		if ( (*i).sScreenName.same_no_case( sScreenName ) )
		{
			return i;
		}
	}
	return NULL;
}



bool Victory :: GetChapter( const gpwstring & sScreenName, Chapter & chapter )
{
	for ( ChapterColl::iterator i = m_Chapters.begin(); i != m_Chapters.end(); ++i )
	{
		if ( (*i).sScreenName.same_no_case( sScreenName ) )
		{
			chapter = *i;
			return true;
		}
	}

	return false;
}


void Victory :: RSActivateChapter( const char * name, int state )
{
	FUBI_RPC_THIS_CALL( RSActivateChapter, RPC_TO_SERVER );

	RCActivateChapter( name, state );
}


void Victory :: RCActivateChapter( const char * name, int state )
{
	FUBI_RPC_THIS_CALL( RCActivateChapter, RPC_TO_ALL );

	ActivateChapter( name, state );
}


void Victory :: ActivateChapter( const char * name, int state )
{
	for ( ChapterColl::iterator i = m_Chapters.begin(); i != m_Chapters.end(); ++i )
	{
		if ( (*i).sName.same_no_case( name ) )
		{
			(*i).bActive = true;
			(*i).currentOrder = state;
		}
	}
}

///////////////////////////////////////////////////////////////

Victory :: GameType :: GameType()
	:	m_Id( 0 )
	,	m_Skrit( NULL )
	,	m_MinTeams( 0 )
	,	m_bAllowHumanPlayerKills( false )
	,	m_bAllowTeamAlliances( false )
	,	m_bAllowCharacterSave( true )
	,	m_bIgnoreMapVictoryConditions( false )
	,	m_FriendColor( 0xFFFFFFFF )
	,	m_EnemyColor( 0xFFFFFFFF )
{
}

Victory :: GameType :: ~GameType()
{
	Delete( m_Skrit );
}

bool Victory :: GameType :: Load( FastFuelHandle hMapGameType )
{
	FastFuelHandle hVictory = hMapGameType.GetChildNamed( "victory" );
	if ( hVictory.IsValid() )
	{
		hVictory.Get( "ignore_map_victory_conditions", m_bIgnoreMapVictoryConditions );

		FastFuelHandleColl victoryConditions;
		hVictory.ListChildrenNamed( victoryConditions, "condition*" );
		for ( FastFuelHandleColl::iterator i = victoryConditions.begin(); i != victoryConditions.end(); ++i )
		{
			VictoryCondition condition;
			condition.Load( *i );
			m_VictoryConditions.push_back( condition );
		}
	}

	FastFuelHandle hTeams = hMapGameType.GetChildNamed( "teams" );
	if ( hTeams.IsValid() )
	{
		FastFuelHandleColl hTeamsColl;
		hTeams.ListChildrenTyped( hTeamsColl, "team" );
		for ( FastFuelHandleColl::iterator iTeam = hTeamsColl.begin(); iTeam != hTeamsColl.end(); ++iTeam )
		{
			Victory::TeamMap::iterator findTeam = m_Teams.find( gpstring( (*iTeam).GetName() ) );
			if ( findTeam == m_Teams.end() )
			{
				gpassertf( 0, ( "Found an undefined team '%s' in the map.", (*iTeam).GetName() ));
				continue;
			}

			FastFuelHandle hAssets = (*iTeam).GetChildNamed( "assets" );
			if ( hAssets.IsValid() )
			{
				for ( Victory::TeamAssetColl::iterator iAsset = findTeam->second.m_Assets.begin(); iAsset != findTeam->second.m_Assets.end(); ++iAsset )
				{
					DWORD scid;
					if ( hAssets.Get( iAsset->m_Name, scid ) )
					{
						iAsset->m_Asset = MakeScid( scid );
					}
					else
					{
						gperrorf(( "Team '%s' doesn't have a required asset '%s' defined in the map for game type '%s'.", 
										(*iTeam).GetName(), iAsset->m_Name.c_str(), m_Name.c_str() ));
						return false;
					}
				}
			}

			FastFuelHandle hStartLocations = (*iTeam).GetChildNamed( "start_locations" );
			if ( hStartLocations.IsValid() )
			{
				FastFuelFindHandle fhStartLocations( hStartLocations );
				if ( fhStartLocations.IsValid() && fhStartLocations.FindFirstKeyAndValue() )
				{
					gpstring key, startLocation;
					while ( fhStartLocations.GetNextKeyAndValue( key, startLocation ) )
					{
						findTeam->second.m_StartLocations.push_back( startLocation );
					}
				}
			}

			if ( findTeam->second.m_bRequireStartLocation && findTeam->second.m_StartLocations.empty() )
			{
				gperrorf(( "Team '%s' has no start locations defined in the map for game type '%s'.", 
								(*iTeam).GetName(), m_Name.c_str() ));
				return false;
			}

		}
	}

	for ( Victory::TeamMap::iterator iTeam = m_Teams.begin(); iTeam != m_Teams.end(); ++iTeam )
	{
		Victory::TeamAssetColl::iterator iAsset = iTeam->second.m_Assets.begin(), iAssetEnd = iTeam->second.m_Assets.end();
		for ( ; iAsset != iAssetEnd; ++iAsset )
		{
			if ( iAsset->m_Asset == SCID_INVALID )
			{
				gperrorf(( "Team '%s' needs asset '%s' defined in game type '%s'.",
							iTeam->first.c_str(), iAsset->m_Name.c_str(), m_Name.c_str() ));
				return false;
			}
		}

		if ( iTeam->second.m_bRequireStartLocation && iTeam->second.m_StartLocations.empty() )
		{
			gperrorf(( "Team '%s' needs start location(s) defined in game type '%s'.",
						iTeam->first.c_str(), m_Name.c_str() ));
			return false;
		}
	}

	return true;
}

bool Victory :: GameType :: LoadDefinition( FastFuelHandle hGameType )
{
	bool bAutomatic = hGameType.GetBool( "automatic", false );

	m_Name = hGameType.GetName();

	if ( !hGameType.Get( "screen_name", m_ScreenName ) )
	{
		gperror( "No screen name found for gametype." );
		return false;
	}

	hGameType.Get( "description", m_Description );

	// get game skrit code
	gpstring skrit;
	if ( hGameType.Get( "skrit", skrit ) && !skrit.empty() )
	{
		gpassert( !m_Skrit );

		// attempt to compile the skrit
		gpstring skritName( m_Name );
		skritName += ":skrit";
		Skrit::CreateReq createReq( skritName );
		m_Skrit = gSkritEngine.CreateNewObject( createReq, const_mem_ptr( skrit.begin(), skrit.size() ) );
		if ( m_Skrit != NULL )
		{
			FuBi::Record* record = m_Skrit->GetRecord();
			if ( record != NULL )
			{
				record->SetXferMode( GP_DEBUG ? FuBi::XMODE_STRICT : FuBi::XMODE_QUIET );
			}
		}
		else
		{
			gperrorf(( "The skrit for game type '%s' failed to compile.", m_Name.c_str() ));
			return false;
		}
	}

	hGameType.Get( "allow_character_save", m_bAllowCharacterSave );
	hGameType.Get( "allow_player_kills", m_bAllowHumanPlayerKills );
	
	FastFuelHandle hColors = hGameType.GetChildNamed( "colors" );
	if ( hColors.IsValid() )
	{
		hColors.Get( "friends", m_FriendColor );
		hColors.Get( "enemies", m_EnemyColor );
	}

	FastFuelHandle hTeams = hGameType.GetChildNamed( "teams" );
	if ( hTeams.IsValid() )
	{
		hTeams.Get( "minimum_teams", m_MinTeams );

		hTeams.Get( "allow_alliances", m_bAllowTeamAlliances );

		FastFuelHandleColl hTeamsColl;
		hTeams.ListChildrenTyped( hTeamsColl, "team" );
		for ( FastFuelHandleColl::iterator iTeam = hTeamsColl.begin(); iTeam != hTeamsColl.end(); ++iTeam )
		{
			gpstring teamName = (*iTeam).GetName();
			
			Victory::TeamMap::iterator findTeam = m_Teams.find( teamName );
			if ( findTeam != m_Teams.end() )
			{
				gpassertf( 0, ( "Found a duplicate team name '%s' in game type '%s'.", teamName.c_str(), hGameType.GetName() ));
				continue;
			}

			Victory::TeamInfo teamInfo;

			if ( !(*iTeam).Get( "id", teamInfo.m_Id ) )
			{
				teamInfo.m_Id = m_Teams.size() + 1;
			}

			if ( !(*iTeam).Get( "team_sign", teamInfo.m_TextureName ) )
			{
				gperrorf(( "There is no team sign texture defined for team '%s' in game type '%s'.", (*iTeam).GetName(), hGameType.GetName() ));
				return false;
			}

			(*iTeam).Get( "require_start_location", teamInfo.m_bRequireStartLocation );

			gpassertm( !(bAutomatic && teamInfo.m_bRequireStartLocation), "An automatic game type can't require start locations." );

			FastFuelHandle hTeamAssets = (*iTeam).GetChildNamed( "assets" );
			if ( hTeamAssets.IsValid() )
			{
				gpassertm( !bAutomatic, "An automatic game type can't have assets." );

				FastFuelFindHandle findAssets( hTeamAssets );
				if ( findAssets.IsValid() && findAssets.FindFirstKeyAndValue() )
				{
					TeamAsset asset;
					asset.m_Asset = SCID_INVALID;

					while ( findAssets.GetNextKeyAndValue( asset.m_Name, asset.m_TemplateName ) )
					{
						teamInfo.m_Assets.push_back( asset );
					}
				}
			}

			m_Teams.insert( Victory::TeamMap::value_type( teamName, teamInfo ) );
		}
	}

	FastFuelHandle hVictory = hGameType.GetChildNamed( "victory" );
	if ( hVictory.IsValid() )
	{
		hVictory.Get( "ignore_map_victory_conditions", m_bIgnoreMapVictoryConditions );

		FastFuelHandleColl victoryConditions;
		hVictory.ListChildrenNamed( victoryConditions, "condition*" );
		for ( FastFuelHandleColl::iterator i = victoryConditions.begin(); i != victoryConditions.end(); ++i )
		{
			VictoryCondition condition;
			condition.Load( *i );
			m_VictoryConditions.push_back( condition );
		}
	}

	FastFuelHandle hMessages = hGameType.GetChildNamed( "messages" );
	if ( hMessages.IsValid() )
	{
		FastFuelHandleColl messages;
		hMessages.ListChildren( messages );
		for ( FastFuelHandleColl::iterator i = messages.begin(); i != messages.end(); ++i )
		{		
			MessageColl messages;					

			FastFuelFindHandle hFind( *i );
			if ( hFind.IsValid() )
			{
				if ( hFind.FindFirstValue( "screen_name" ) )
				{				
					gpstring sValue;

					while ( hFind.GetNextValue( sValue ) != FVP_NONE )
					{												
						if ( !sValue.empty() )
						{		
							messages.push_back( ReportSys::TranslateW( sValue.c_str() ) );
						}
					}
				}
			}

			if ( messages.size() != 0 )
			{
				m_Messages.insert( Victory::MessageMap::value_type( gpstring( (*i).GetName() ), messages ) );
			}
		}
	}

	FastFuelHandle hStats = hGameType.GetChildNamed( "stats" );
	if ( hStats.IsValid() )
	{
		FastFuelHandle hCustomStats = hStats.GetChildNamed( "custom" );
		if ( hCustomStats.IsValid() )
		{
			FastFuelHandleColl stats;
			hCustomStats.ListChildren( stats );
			for ( FastFuelHandleColl::iterator i = stats.begin(); i != stats.end(); ++i )
			{
				if ( strlen( (*i).GetType() ) == 0 )
				{
					gpwstring screenName;
					(*i).Get( "screen_name", screenName );

					if_gpassertf( !screenName.empty(), ( "Stat '%s' has no screen_name.", (*i).GetName() ) )
					{
						m_Stats.push_back( Victory::StatScreenPair( gpstring( (*i).GetName() ), screenName ) );
					}
				}
			}
		}

		FastFuelHandle hInGameRanks = hStats.GetChildNamed( "in_game" );
		if ( hInGameRanks.IsValid() )
		{
			m_InGameRankings.clear();

			FastFuelFindHandle findRanks( hInGameRanks );
			if ( findRanks.IsValid() && findRanks.FindFirstKeyAndValue() )
			{
				gpstring key, stat;
				while ( findRanks.GetNextKeyAndValue( key, stat ) )
				{
					m_InGameRankings.push_back( stat );
				}
			}
		}

		FastFuelHandle hEndGameRanks = hStats.GetChildNamed( "end_game" );
		if ( hEndGameRanks.IsValid() )
		{
			m_EndGameRankings.clear();

			FastFuelFindHandle findRanks( hEndGameRanks );
			if ( findRanks.IsValid() && findRanks.FindFirstKeyAndValue() )
			{
				gpstring key, stat;
				while ( findRanks.GetNextKeyAndValue( key, stat ) )
				{
					m_EndGameRankings[key].push_back( stat );
				}
			}
		}
	}

	return true;
}

const wchar_t * Victory :: GameType :: GetMessage( const gpstring & message ) const
{
	Victory::MessageMap::const_iterator findMessage = m_Messages.find( message );
	if ( findMessage != m_Messages.end() )
	{
		int numMessages = findMessage->second.size()-1;

		int randomNum = Random( numMessages );

		return findMessage->second[randomNum].c_str();
	}
	else
	{
		return L"";
	}
}

///////////////////////////////////////////////////////////////

gpstring Victory :: GetGameTypeName( int id )
{
	CHECK_SERVER_ONLY;

	for ( GameTypeColl::iterator i = m_GameTypes.begin(); i != m_GameTypes.end(); ++i )
	{
		if ( (*i)->m_Id == id )
		{
			return (*i)->m_Name;
		}
	}
	return gpstring();
}

void Victory :: SSetGameType( const gpstring & gameType )
{
	CHECK_SERVER_ONLY;

	eTeamMode teamMode = m_TeamMode;

	RCSetGameType( gameType, RPC_TO_ALL );

	// did teams change with this game type?
	if ( (m_TeamMode != teamMode) || (m_TeamMode == DEFINED_TEAMS) || (m_TeamMode == NO_TEAMS) )
	{
		gServer.SInitTeams();
	}
}

FuBiCookie Victory :: RCSetGameType( const gpstring & gameType, DWORD machineId )
{
	FUBI_RPC_CALL_RETRY( RCSetGameType, machineId );

	if ( SetGameType( gameType ) )
	{
		gWorldOptions.SetAllowAutoSaveParty( GetGameType()->GetAllowCharacterSave() );

		gServer.SetAllowPlayerKills( GetGameType()->GetAllowHumanPlayerKills() );

		if ( (GetMinTeams() > 0) && GetTeams().empty() )
		{
			m_TeamMode = UNDEFINED_TEAMS;
		}
		else if ( (GetMinTeams() > 0) && !GetTeams().empty() )
		{
			m_TeamMode = DEFINED_TEAMS;
		}
		else
		{
			m_TeamMode = NO_TEAMS;
		}

		WorldMessage( WE_MP_SET_GAME_TYPE, GOID_INVALID, GOID_ANY ).Send();

		gServer.SetSettingsDirty();

		return RPC_SUCCESS;
	}

	return RPC_FAILURE;
}

bool Victory :: SetGameType( const gpstring & gameType )
{
	for ( GameTypeColl::iterator i = m_GameTypes.begin(); i != m_GameTypes.end(); ++i )
	{
		if ( (*i)->m_Name.same_no_case( gameType ) )
		{
			m_pGameType = *i;

			return true;
		}
	}
	return false;
}

Victory::TeamMap & Victory :: GetTeams()
{
	gpassert( m_pGameType );

	return ( m_pGameType->m_Teams );
}

int Victory :: GetMinTeams()
{
	gpassert( m_pGameType );

	return ( m_pGameType->m_MinTeams );
}

Victory::ConditionColl const & Victory :: GetVictoryConditions()
{
	gpassert( m_pGameType );

	return ( m_pGameType->m_VictoryConditions );
}


///////////////////////////////////////////////////////////////

inline int Victory :: MakeStatId( const char * stat )
{
	eGameStat getStat;
	if ( !FromString( stat, getStat ) )
	{
		int statIndex = 0;
		for ( StatsColl::iterator findStat = m_pGameType->m_Stats.begin(); findStat != m_pGameType->m_Stats.end(); ++findStat, ++statIndex )
		{
			if ( findStat->first.same_no_case( stat ) )
			{
				return ( statIndex + GS_COUNT );
			}
		}
		return ( -1 );
	}
	else
	{
		return ( (int)getStat );
	}
}

/*
void Victory :: SIncrementStat( eGameStat stat, int value )
{
	CHECK_SERVER_ONLY;

	int oldValue = m_pGameStats->GetInt( ToString( stat ) );
	RCSetGameStat( (int)stat, oldValue + value );
}
*/

void Victory :: SIncrementStat( eGameStat stat, Player * pPlayer, int value )
{
	CHECK_SERVER_ONLY;

	int player = MakeInt( pPlayer->GetId() );
	int oldValue = m_pPlayerStats->GetInt( player, ToString( stat ) );
	RCSetPlayerStat( (int)stat, player, oldValue + value );

	Team * team = gServer.GetTeam( pPlayer->GetId() );
	if ( team )
	{
		oldValue = m_pTeamStats->GetInt( team->m_Id, ToString( stat ) );
		RCSetTeamStat( (int)stat, team->m_Id, oldValue + value );
	}
}

/*
void Victory :: SIncrementStat( const char * stat, int value )
{
	CHECK_SERVER_ONLY;

	int statIndex = 0;

	for ( RankingsMap::const_iterator i = GetRankings().begin(); i != GetRankings().end(); ++i, ++statIndex )
	{
		if ( i->second.same_no_case( stat ) )
		{
			int oldValue = m_pGameStats->GetInt( stat );
			RCSetGameStat( statIndex, oldValue + value );

			return;
		}
	}

	gpassertf( 0, ( "Tried to set undefined Game Stat : %s", stat ) );
}
*/

void Victory :: SIncrementStat( const char * stat, Player * pPlayer, int value )
{
	CHECK_SERVER_ONLY;

	int statIndex = MakeStatId( stat );
	if ( statIndex < 0 )
	{
		gpassertf( 0, ( "Tried to set undefined Game Stat : %s", stat ) );
		return;
	}

	int player = MakeInt( pPlayer->GetId() );
	int oldValue = m_pPlayerStats->GetInt( player, stat );
	RCSetPlayerStat( statIndex, player, oldValue + value );

	Team * team = gServer.GetTeam( pPlayer->GetId() );
	if ( team )
	{
		oldValue = m_pTeamStats->GetInt( team->m_Id, stat );
		RCSetTeamStat( statIndex, team->m_Id, oldValue + value );
	}
}

void Victory :: RCClearPlayerStats( PlayerId playerId )
{
	FUBI_RPC_TAG();

	if ( !m_pGameType )
	{
		// map not yet set
		return;
	}

	FUBI_RPC_THIS_CALL( RCClearPlayerStats, RPC_TO_ALL );

	{for ( int i = 0; i < GS_COUNT; ++i )
	{
		m_pPlayerStats->Set( MakeInt( playerId ), s_GameStat[ i ], 0 );
	}}

	{for ( StatsColl::iterator i = m_pGameType->m_Stats.begin(); i != m_pGameType->m_Stats.end(); ++i )
	{
		m_pPlayerStats->Set( MakeInt( playerId ), i->first.c_str(), 0 );
	}}
}

FuBiCookie Victory :: RCSetPlayerStat( int stat, int player, int value, DWORD machineId )
{
	FUBI_RPC_CALL_RETRY( RCSetPlayerStat, machineId );

	const char * statName = NULL;

	if ( stat < 0 )
	{
		return RPC_FAILURE;
	}
	else if ( stat < GS_COUNT )
	{
		statName = ToString( (eGameStat)stat );
	}
	else
	{
		if ( (stat - GS_COUNT) < (int)m_pGameType->m_Stats.size() )
		{
			statName = m_pGameType->m_Stats[ stat - GS_COUNT ].first.c_str();
		}
		else
		{
			gpassertm( 0, "Tried to set an invalid stat." );
			return RPC_FAILURE;
		}
	}

	m_pPlayerStats->Set( player, statName, value );

	return RPC_SUCCESS;
}

FuBiCookie Victory :: RCSetTeamStat( int stat, int team, int value, DWORD machineId )
{
	FUBI_RPC_CALL_RETRY( RCSetTeamStat, machineId );

	const char * statName = NULL;

	if ( stat < 0 )
	{
		return RPC_FAILURE;
	}
	else if ( stat < GS_COUNT )
	{
		statName = ToString( (eGameStat)stat );
	}
	else
	{
		if ( (stat - GS_COUNT) < (int)m_pGameType->m_Stats.size() )
		{
			statName = m_pGameType->m_Stats[ stat - GS_COUNT ].first.c_str();
		}
		else
		{
			gpassertm( 0, "Tried to set an invalid stat." );
			return RPC_FAILURE;
		}
	}

	m_pTeamStats->Set( team, statName, value );

	return RPC_SUCCESS;
}

/*
FuBiCookie Victory :: RCSetGameStat( int stat, int value, DWORD machineId )
{
	FUBI_RPC_CALL_RETRY( RCSetGameStat, machineId );

	RankingsMap::const_iterator findStat = GetRankings().find( stat );
	if ( findStat == GetRankings().end() )
	{
		gpassertm( 0, "Tried to set an invalid stat." );
		return RPC_FAILURE;
	}

	m_pGameStats->Set( findStat->second.c_str(), value );
	return RPC_SUCCESS;
}
*/

int Victory :: GetPlayerStat( const char * stat, PlayerId player )
{
	return ( m_pPlayerStats->GetInt( MakeInt( player ), stat ) );
}

int	Victory :: GetTeamStat( const char * stat, int team )
{
	return ( m_pTeamStats->GetInt( team, stat ) );
}

int Victory :: GetGameStat( const char * stat )
{
	return ( m_pGameStats->GetInt( stat ) );
}

const gpwstring & Victory :: GetStatScreenName( const char * stat ) const
{
	eGameStat getStat;
	if ( FromString( stat, getStat ) )
	{
		return ( m_GameStatScreenNames[ (int)getStat ] );
	}
	else
	{
		for ( StatsColl::iterator findStat = m_pGameType->m_Stats.begin(); findStat != m_pGameType->m_Stats.end(); ++findStat )
		{
			if ( findStat->first.same_no_case( stat ) )
			{
				return ( findStat->second );
			}
		}
	}
	return ( gpwstring::EMPTY );
}


gpstring Victory :: GetNameFromScreenName( gpwstring &screenName ) const
{
	int stat = 0;
	for ( std::vector< gpwstring >::const_iterator findStat = m_GameStatScreenNames.begin(); findStat != m_GameStatScreenNames.end(); ++findStat )
	{
		if ( (*findStat).same_no_case( screenName ) )
		{
			return ( s_GameStat[ stat ] );
		}

		++stat;
	}
	return ( gpstring::EMPTY );
}


void Victory :: SDisplayMessage( const char * msgName )
{
	RCDisplayMessage( ToAnsi( GetGameType()->GetMessage( msgName ) ).c_str() );
}

void Victory :: SDisplayMessage1Party( const char * msgName, Go * party )
{
	if ( party == NULL )
	{
		return;
	}

	gpwstring message;
	message.assignf( GetGameType()->GetMessage( msgName ), party->GetCommon()->GetScreenName().c_str() );

	RCDisplayMessage( ToAnsi( message ).c_str() );
}

void Victory :: SDisplayMessage2Party( const char * msgName, Go * party1, Go * party2 )
{
	if ( (party1 == NULL) || (party2 == NULL) )
	{
		return;
	}

	gpwstring message;
	message.assignf( GetGameType()->GetMessage( msgName ), party1->GetCommon()->GetScreenName().c_str(), party2->GetCommon()->GetScreenName().c_str() );

	RCDisplayMessage( ToAnsi( message ).c_str() );
}

void Victory :: SLeaderMessage( eGameStat stat, const char * msgName )
{
	// Get largest stat
	Server::PlayerColl players = gServer.GetPlayers();
	Server::PlayerColl::iterator i;
	int	largestStat = 0;
	Player * pPlayer = 0;
	for ( i = players.begin(); i != players.end(); ++i )
	{
		if ( IsValid(*i) && !(*i)->IsComputerPlayer() )
		{
			int statValue = GetPlayerStat( ToString( stat ), (*i)->GetId() );

			if ( statValue > largestStat )
			{
				largestStat = statValue;
				pPlayer = (*i);
			}			
		}
	}
	
	// If more than one person has the largest stat, don't display anything
	if ( pPlayer )
	{
		for ( i = players.begin(); i != players.end(); ++i )
		{
			if ( *i && !(*i)->IsComputerPlayer() )
			{
				int statValue = GetPlayerStat( ToString( stat ), (*i)->GetId() );

				if ( statValue == largestStat && pPlayer->GetId() != (*i)->GetId() )
				{
					return;
				}			
			}
		}
	}

	// Display the stat message
	if ( pPlayer && ( (int)pPlayer->GetId() != gGameAuditor.GetInt( "mp_leader" )) )
	{
		gpwstring sFormat = GetGameType()->GetMessage( msgName );
		if ( !sFormat.empty() )
		{	
			GoHandle hLeader( pPlayer->GetHero() );
			if ( hLeader.IsValid() )
			{
				gGameAuditor.SetInt( "mp_leader", (int)pPlayer->GetId() );
				gpwstring message;			
				message.assignf( sFormat.c_str(), hLeader->GetCommon()->GetScreenName().c_str(), largestStat );
				RCDisplayMessage( ToAnsi( message ).c_str() );
			}
		}
	}
}

void Victory :: RCDisplayMessage( const char * message )
{
	FUBI_RPC_THIS_CALL( RCDisplayMessage, RPC_TO_ALL );

	gpscreen( ReportSys::TranslateA( message ) );
}

void Victory :: TrackPlayerExperience( int playerId, const char * skillName, double experience )
{
	double total = m_pPlayerStats->IncrementDouble( playerId, ToString( GS_EXPERIENCE_GAINED ), experience );
	m_pPlayerStats->SetInt( playerId, ToString( GS_EXPERIENCE_GAINED ), (int)total );

	StatTimelineMap::iterator findTimeline = m_PlayerExperience.find( MakePlayerId( playerId ) );
	if ( findTimeline == m_PlayerExperience.end() )
	{
		findTimeline = m_PlayerExperience.insert( StatTimelineMap::value_type( MakePlayerId( playerId ), StatTimeline() ) ).first;
	}
	AddTimelineEntry( findTimeline->second, GetGameTimeElapsed(), (float)total );

	if ( same_no_case( skillName, "melee" ) )
	{
		total = m_pPlayerStats->IncrementDouble( playerId, ToString( GS_MELEE_GAINED ), experience );
		m_pPlayerStats->SetInt( playerId, ToString( GS_MELEE_GAINED ), (int)total );
	}
	else if ( same_no_case( skillName, "ranged" ) )
	{
		total = (int)m_pPlayerStats->IncrementDouble( playerId, ToString( GS_RANGED_GAINED ), experience );
		m_pPlayerStats->SetInt( playerId, ToString( GS_RANGED_GAINED ), (int)total );
	}
	else if ( same_no_case( skillName, "nature magic" ) )
	{
		total = (int)m_pPlayerStats->IncrementDouble( playerId, ToString( GS_NMAGIC_GAINED ), experience );
		m_pPlayerStats->SetInt( playerId, ToString( GS_NMAGIC_GAINED ), (int)total );
	}
	else if ( same_no_case( skillName, "combat magic" ) )
	{
		total = (int)m_pPlayerStats->IncrementDouble( playerId, ToString( GS_CMAGIC_GAINED ), experience );
		m_pPlayerStats->SetInt( playerId, ToString( GS_CMAGIC_GAINED ), (int)total );
	}
}

///////////////////////////////////////////////////////////////////
//
//	Managing the player History
//		to add a new history event add support to:
//			UpdateAllPlayerHistory	-- does any global calculations and calls UpdatePlayerHistory for each player
//			UpdatePlayerHistory		-- updates player history, this will likely call AddHistoryEntry
//			GetPlayerHistoryValue	-- get the current value of the player history if any
//			AddHistoryEntry			-- adds an entry to the list
//
//		since history entries require some preperation, we need to have code for each...
//
void Victory :: RCUpdateAllPlayerHistory( )
{
	FUBI_RPC_THIS_CALL( RCUpdateAllPlayerHistory, RPC_TO_ALL );

	UpdateAllPlayerHistories();
}

void Victory :: UpdateAllPlayerHistories( bool forceEntry )
{
	UpdateAllPlayerHistory( "kills_trend", forceEntry );
	UpdateAllPlayerHistory( "kills", forceEntry );
	UpdateAllPlayerHistory( "deaths", forceEntry );
	UpdateAllPlayerHistory( "human_kills", forceEntry );
	UpdateAllPlayerHistory( "melee_gained", forceEntry );
	UpdateAllPlayerHistory( "ranged_gained", forceEntry );
	UpdateAllPlayerHistory( "cmagic_gained", forceEntry );
	UpdateAllPlayerHistory( "nmagic_gained", forceEntry );
	UpdateAllPlayerHistory( "experience_gained", forceEntry );
	UpdateAllPlayerHistory( "gold_gained", forceEntry );
	UpdateAllPlayerHistory( "loot_gained", forceEntry );
	UpdateAllPlayerHistory( "loot_value", forceEntry );
}

void Victory :: UpdatePlayerHistoryStats( const char * historyName )
{
	// update player stat value if needed.  GetPlayerHistoryValue returns true if there is a 
	// stat value that needs to be tracked in addition to the history for realtime numbers!
	// most stats will return false!
	if( IsServerLocal() )
	{
		for ( Server::PlayerColl::const_iterator player = gServer.GetPlayers().begin(); player != gServer.GetPlayers().end(); ++player )
		{
			if ( IsValid(*player) && !(*player)->IsComputerPlayer() )
			{
				float newValue;
				if( GetPlayerHistoryValue( newValue, (*player)->GetId(), historyName ) )
				{
					// only update if changed
					if( newValue != GetPlayerStat( historyName, (*player)->GetId() ) )
					{
						RCSetPlayerStat( MakeStatId( historyName ), MakeInt( (*player)->GetId() ), (int) newValue );
					}
				}
			}
		}
	}
}

void Victory :: UpdateAllPlayerHistory( const char * historyName, bool forceEntry )
{
	// any special global initalization for out history type.
	if( !strcmp( historyName, "kills_trend" ) )
	{
		m_PlayerKillTotal = 0;
	}
	
	for ( Server::PlayerColl::const_iterator player = gServer.GetPlayers().begin(); player != gServer.GetPlayers().end(); ++player )
	{
		if ( IsValid(*player) && !(*player)->IsComputerPlayer() )
		{
			// update the history for each player
			UpdatePlayerHistory( (*player)->GetId(), historyName, forceEntry );				
		}
	}
}
//
// update the history for one player.
// this is where the specific stat code is put!
//
void Victory :: UpdatePlayerHistory( PlayerId playerId, const char * historyName, bool forceEntry )
{
	StatTimelineMap::iterator findTimeline = m_PlayerStatTimelines[ historyName ].find( playerId );

	if( !strcmp( historyName, "kills_trend" ) )
	{
		// make sure that we have at least one entry in the stack to start off.
		if ( findTimeline == m_PlayerStatTimelines[ historyName ].end() )
		{
			findTimeline = m_PlayerStatTimelines[ historyName ].insert( StatTimelineMap::value_type( playerId, StatTimeline( ) ) ).first;
			(findTimeline->second).push_back( StatEntry( -10 - m_timelineTimeDelta, 0 ) );
		}

		AddHistoryEntry( findTimeline->second, GetGameTimeElapsed(), (float) GetPlayerStat( "kills", playerId ), gUIGame.GetUIPlayerRanks()->GetKillTrendHistoryLength(), gUIGame.GetUIPlayerRanks()->GetKillTrendUpdatePeriod(), forceEntry );

		// use the history to compute the current kill total for this segment of time
		if ( findTimeline != m_PlayerStatTimelines[ historyName ].end() )
		{
			StatTimeline::iterator timelineEnd = (findTimeline->second).end();
			if( timelineEnd != (findTimeline->second).begin() )
			{
				--timelineEnd;
				// add our kills onto the total!
				m_PlayerKillTotal = m_PlayerKillTotal + (int)((*timelineEnd).Value - (*(findTimeline->second).begin()).Value) ;
			}				
		}
	}
	else
	{
		// make sure that we have at least one entry in the stack to start off.
		if ( findTimeline == m_PlayerStatTimelines[ historyName ].end() )
		{
			findTimeline = m_PlayerStatTimelines[ historyName ].insert( StatTimelineMap::value_type( playerId, StatTimeline( ) ) ).first;
			(findTimeline->second).push_back( StatEntry( 0, 0 ) );
		}

		AddHistoryEntry( findTimeline->second, GetGameTimeElapsed(), (float) GetPlayerStat( historyName, playerId ), m_timelineMaxEntries, m_timelineTimeDelta, forceEntry );
	}
}


//
// compute a given history value.
// this is for histories that have a current value
// that needs to be maintained for realtime feedback
//
bool Victory :: GetPlayerHistoryValue( float &outHistoryValue, PlayerId playerId, const char * historyName )
{
	if( !strcmp( historyName, "kills_trend" ) )
	{
		StatTimelineMap::iterator findPlayerTimeline = m_PlayerStatTimelines[ historyName ].find( playerId );
		
		if ( findPlayerTimeline == m_PlayerStatTimelines[ historyName ].end() )
		{
			gperrorf(("There is no history for player: %d!",MakeInt(playerId) ));
		}
		else
		{
			StatTimeline::iterator timelineEnd = (findPlayerTimeline->second).end();

			if( timelineEnd != (findPlayerTimeline->second).begin() )
			{
				--timelineEnd;
				if( m_PlayerKillTotal != 0 )
				{
					outHistoryValue = ( ( (*timelineEnd).Value - (*(findPlayerTimeline->second).begin()).Value ) - m_PlayerKillTotal/gServer.GetNumHumanPlayers() );
					return true;
				}
			}			
		}
	}

	return false;
}

//
// compute a given history value.
// this is for histories that have a current value
// that needs to be maintained for realtime feedback
//

int Victory :: GetPlayerHistorySize( PlayerId playerId, const char * historyName )
{
	StatTimelineMap::iterator findPlayerTimeline = m_PlayerStatTimelines[ historyName ].find( playerId );
	if ( findPlayerTimeline == m_PlayerStatTimelines[ historyName ].end() )
	{
		gperrorf(("There is no history for player: %d!",MakeInt(playerId) ));
	}
	else
	{
		return findPlayerTimeline->second.size();
	}

	return -1;
}

bool Victory :: GetPlayerHistoryValue( float &outHistoryValue, double &outHistoryTime, int index, PlayerId playerId, const char * historyName )
{
	StatTimelineMap::iterator findPlayerTimeline = m_PlayerStatTimelines[ historyName ].find( playerId );

	if ( findPlayerTimeline == m_PlayerStatTimelines[ historyName ].end() )
	{
		gperrorf(("There is no history for player: %d!",MakeInt(playerId) ));
	}
	else
	{
		if( (findPlayerTimeline->second).begin() != (findPlayerTimeline->second).end() )
		{
			outHistoryValue = (findPlayerTimeline->second)[ index ].Value;
			outHistoryTime = (findPlayerTimeline->second)[ index ].Time;
			return true;
		}			
	}

	return false;
}


void Victory :: AddHistoryEntry( StatTimeline & timeline, double time, float value, int numEntries, float timeDiff, bool forceEntry )
{
	// if it is time to add another history entry, then do so.
	// if not, just update our current value!
	StatTimeline::iterator timelineLast = timeline.end();
	if( timeline.begin() != timeline.end() )
	{	
		--timelineLast;
	}

	if( timeline.begin() == timeline.end() || PreciseSubtract( GetGameTimeElapsed(), (*timelineLast).Time ) > timeDiff)
	{
		// only add a new value if it is different from the last value
		if( (*timelineLast).Value != value || forceEntry )
		{
			// take of the oldest value if we have too many entries
			if ( (int)timeline.size() >= numEntries )
			{	
				timeline.pop_front();
			}

			// add the new history value
			timeline.push_back( StatEntry( time, value ) );
		}
	}
	else
	{
		// replace the current timeline value with upto date info
		(*timelineLast).Value = value;
	}
}


//
// multiplayer jip support
//

// tell this machine to sync up the info from the server.  this will use the info
// that we put into the block on the server (xfer called first)
void Victory :: RCSyncEndGameTimeline( DWORD machineId, gpstring& syncBlock )
{
	FUBI_RPC_THIS_CALL( RCSyncEndGameTimeline, machineId );		

	// create a fuel block for the jiping player
	FuelHandle fuel( "::import:root" );
	fuel = fuel->CreateChildBlock( "EndGameTimelines", "EndGameTimelines" );

	// read fuel into new block
	fuel->AddChildren( syncBlock, syncBlock.length() );

	// create input streams
	FuBi::FuelReader reader( fuel );
	FuBi::PersistContext persist( &reader );

	XferForSyncEndGameTimeline( persist );
}

FUBI_DECLARE_XFER_TRAITS( Victory::StatTimeline )
{
	static bool XferDef( PersistContext &persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		return ( persist.XferDeque( key, obj ) );
	}
};
FUBI_DECLARE_XFER_TRAITS( Victory::StatTimelineMap )
{
	static bool XferDef( PersistContext &persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		return ( persist.XferMap( key, obj ) );
	}
};

void Victory :: XferForSyncEndGameTimeline( FuBi::PersistContext& persist )
{
	persist.EnterBlock( "EndGameTimelines" );
	{
		persist.XferMap( "m_PlayerStatTimelines", m_PlayerStatTimelines );
	}

	persist.LeaveBlock();
}
// end jip support

float Victory :: GetPlayerExperienceTimelineValue( PlayerId playerId, float time )
{
	StatTimelineMap::iterator findTimeline = m_PlayerExperience.find( playerId );
	if ( findTimeline != m_PlayerExperience.end() )
	{
		StatTimeline &timeline = findTimeline->second;
		StatEntry prev;

		for ( StatTimeline::iterator i = timeline.begin(); i != timeline.end(); ++i )
		{
			if ( i->Time > time )
			{
				break;
			}
			prev = *i;
		}

		return ( prev.Value );
		/*
		if ( i == timeline.end() )
		{
			return ( prev.Value );
		}

		float value = (i->Value - prev.Value) / (i->Time - prev.Time);
		value = value * time + (i->Value - value * i->Time);
		return ( value );
		*/
	}

	return ( 0.0f );
}

void Victory :: AddTimelineEntry( StatTimeline & timeline, double time, float value )
{
	if ( timeline.size() == MAX_TIMELINE_ENTRIES )
	{
		double span = -1.0f;

		StatTimeline::iterator prev = timeline.begin(), s1 = timeline.end(), s2 = timeline.end();
		for ( StatTimeline::iterator i = timeline.begin(); i != timeline.end(); ++i )
		{
			if ( i == timeline.begin() )
			{
				continue;
			}
			if ( (span < 0.0f) || (PreciseSubtract(i->Time, prev->Time) < span) )
			{
				s1 = prev;
				s2 = i;
				span = s2->Time - s1->Time;
			}
			prev = i;
		}

		if ( s1 != timeline.end() )
		{
			s1->Time += s2->Time;
			s1->Time *= 0.5f;
			s1->Value += s2->Value;
			s1->Value *= 0.5f;
			timeline.erase( s2 );
		}
	}

	if ( timeline.size() < MAX_TIMELINE_ENTRIES )
	{
		timeline.push_back( StatEntry( time, value ) );
	}
}

Victory::StatTimeline Victory :: GetTimeline( PlayerId playerId ) const
{
	StatTimelineMap::const_iterator findTimeline = m_PlayerExperience.find( playerId );
	if ( findTimeline != m_PlayerExperience.end() )
	{
		return ( findTimeline->second );
	}
	return ( StatTimeline() );
}
