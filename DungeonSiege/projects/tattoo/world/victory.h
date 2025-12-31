/*=======================================================================================

	Victory

	author:		Adam Swensen
	creation:	8/28/2000

	purpose:	Manages the state of the game.
				Handles game stats, quests, victory conditions.

=======================================================================================*/


#pragma once
#ifndef __VICTORY_H
#define __VICTORY_H

class AuditorDb;

namespace FuBi
{
	class FuelWriter;
}

///////////////////////////////////////////////////////////////
//  Internally tracked game stats

enum eGameStat
{
	SET_BEGIN_ENUM( GS_, 0 ),

	GS_KILLS,
	GS_KILLS_TREND,
	GS_DEATHS,
	GS_GIBS,
	GS_HUMAN_KILLS,
	GS_MELEE_GAINED,
	GS_RANGED_GAINED,
	GS_CMAGIC_GAINED,
	GS_NMAGIC_GAINED,
	GS_EXPERIENCE_GAINED,
	GS_GOLD_GAINED,
	GS_LOOT_GAINED,
	GS_LOOT_VALUE,

	SET_END_ENUM( GS_ ),
};

const char * ToString( eGameStat gs );
bool FromString( const char* str, eGameStat & gs );

///////////////////////////////////////////////////////////////

class Victory : public Singleton< Victory >
{
public:

	Victory();
	~Victory();

	void Init( bool forNewMap = false );

	bool Xfer( FuBi::PersistContext& persist );
	bool XferCharacter( FuBi::PersistContext& persist, const char* key );

	void SSyncOnMachine( DWORD machineId );

	void LoadFromMap();

	void Update();

	void HandleWorldMessage( WorldMessage & message );

	///////////////////////////////////////////////////////////////
	//  Game State

FEX	FuBiCookie RSSetGameEnded	( bool bSet );
FEX	FuBiCookie RCSetGameEnded	( bool bSet );
	void SetGameEnded			( bool bSet );

	bool GameEnded() const												{  return m_GameEnded;  }

FEX	void RSSetMainCampaignWon( bool bSet );
FEX void RCSetMainCampaignWon( bool bSet );

	void ToggleAllowDefeat();  // for debug console command

	///////////////////////////////////////////////////////////////
	//  Game Settings

	void SSetTimeLimit( float timeLimit, DWORD machineId );
FEX FuBiCookie RCSetTimeLimit( float timeLimit, DWORD machineId );

	void	SetTimeLimit( float timeLimit )								{  m_TimeLimit = timeLimit;  }
	float	GetTimeLimit() const										{  return m_TimeLimit;  }

FEX double	GetGameTimeRemaining();
FEX double	GetGameTimeElapsed();
	
	float	GetEndGameTime() const										{  return m_EndGameTime;  }

	///////////////////////////////////////////////////////////////
	//  Victory Conditions

	struct VictoryCondition
	{
		VictoryCondition()		: m_Met( 0 )
								, m_bExclusive( false )
								, m_Value( 0 )	{}

		gpstring	m_Name;
		gpwstring	m_ScreenName;
		gpwstring	m_Description;

		gpstring	m_Team;  // the team this condition is active for

		DWORD		m_Met;  // teams which have met the condition

		bool		m_bExclusive;  // game ends once condition has been met

		int			m_Value;  // value related to condition, settable in UI

		void Load( FastFuelHandle hCondition );

		bool Xfer( FuBi::PersistContext& persist );
	};

	typedef std::vector< VictoryCondition > ConditionColl;

FEX	bool IsVictoryConditionMet( const char * condition );
FEX bool IsVictoryConditionMet( const char * condition, int team );

FEX void SSetVictoryConditionMet( const char * condition, bool met );
FEX void RCSetVictoryConditionMet( const char * condition, bool met );
	FUBI_MEMBER_SET( RCSetVictoryConditionMet, +SERVER );
	void SetVictoryConditionMet( const char * condition, bool met );

FEX void SSetTeamVictoryConditionMet( const char * condition, bool met, int team );
FEX void RCSetTeamVictoryConditionMet( const char * condition, bool met, int team );
	FUBI_MEMBER_SET( RCSetTeamVictoryConditionMet, +SERVER );
	void SetTeamVictoryConditionMet( const char * condition, bool met, int team );

FEX	int  GetVictoryConditionValue( const char * condition );
FEX bool SetVictoryConditionValue( const char * condition, int value );

	void CheckTeamsForVictory();

	typedef CBFunctor0 DefeatGuiCb;
	void RegisterGuiDefeatCallback( DefeatGuiCb callback ) {  m_DefeatCallback = callback; }
	void GuiDefeatCallback() { if ( m_DefeatCallback ) {  m_DefeatCallback(); } }

	typedef CBFunctor0 MpVictoryGuiCb;
	void RegisterGuiVictoryCallback( MpVictoryGuiCb callback ) { m_MpVictoryCallback = callback; }
	void GuiVictoryCallback() { if ( m_MpVictoryCallback ) { m_MpVictoryCallback(); } }

	///////////////////////////////////////////////////////////////
	//  Quest Support

// Quests
	struct QuestEvent
	{
		int			order;
		gpwstring	sDescription;
		gpwstring	sSpeaker;
		gpstring	sConvFuelAddress;
		bool		bRequired;
		bool		bPlayUpdate;
	};
	typedef std::vector< QuestEvent > QuestEventColl;

	struct Quest
	{
		Quest()					{  id = 0; bCompleted = false; bActive = false; currentOrder = 0; bViewed = false; }
		Quest( int setId )		{  id = setId; bCompleted = false; bActive = false; currentOrder = 0; bViewed = false; }
		
		QuestEventColl events;

		int		  currentOrder;
		int		  id;
		bool      bCompleted;
		bool      bActive;
		bool	  bViewed;
		gpstring  sName;				
		gpstring  sSkrit;
		gpstring  sVictorySample;
		gpwstring sScreenName;
		gpstring  sChapter;
		gpstring  sQuestImage;		
		gpwstring sActiveDesc;		

		bool	Xfer( FuBi::PersistContext& persist );
	};
	typedef std::vector< Quest > QuestColl;

	typedef std::map< gpstring, int, istring_less > QuestCharMWColl;				//( questName, )
	typedef std::map< gpstring, QuestCharMWColl, istring_less > QuestCharMColl;		//( worldLevel, )
	typedef std::map< gpstring, QuestCharMColl, istring_less > QuestCharColl;		//( mapName, )

	typedef CBFunctor1< int > QuestUpdateCb;
	void RegisterInventoryUpdateCallback( QuestUpdateCb callback ) {  m_QuestUpdateCallback = callback; }
	void UpdateQuestCallback( int id ) { if ( m_QuestUpdateCallback ) {  m_QuestUpdateCallback( id ); } }

	Quest* FindQuestByName( const gpstring& name );
	Quest* FindQuestById	( int id );

FEX	void RSActivateQuest( const char * name, int state );
FEX	void RCActivateQuest( const char * name, int state );
	void ActivateQuest( const char * name, int state, bool bIgnoreCompletion = false, bool bSaveForCharacter = true );

FEX void RSDeactivateQuest( const char * name );
FEX void RCDeactivateQuest( const char * name );
	void DeactivateQuest( const char * name );

	void UpdateQuestCompletion();
FEX	void RSUpdateQuestCompletion( const char * name, Goid activator );

FEX void RSCompletedQuest( const char * name, Goid activator );
FEX void RCCompletedQuest( const char * name, Goid activator );
	void CompletedQuest( const char * name, Goid activator, bool bSaveForCharacter = true  );

FEX	bool IsQuestCharActive( const gpstring & name );
FEX	bool IsQuestCharCompleted( const gpstring & name );
FEX	bool IsQuestCompleted( const gpstring & name );
FEX bool IsQuestActive( const gpstring & name );
FEX int  GetQuestOrder( const gpstring & name );
FEX int  GetQuestCharOrder( );
FEX int  GetQuestCharOrder( const gpstring& questName );
FEX int  GetQuestCharOrder( const gpstring& questName, const gpstring& worldName );
FEX int  GetQuestCharOrder( const gpstring& questName, const gpstring& worldName, const gpstring& mapName );
FEX void SetQuestCharOrder( const gpstring& questName, int value );
	
FEX void RCSyncQuests( DWORD machineId, const_mem_ptr  );
	void XferForSyncQuests( FuBi::PersistContext& persist );

	bool			  GetQuest( int id, Quest & quest );
	QuestColl const & GetQuests()							{  return m_Quests;  }
	int				  GetLastUpdatedQuest()					{ return m_activeQuest; }
	void			  SetQuestViewed( int id, bool bViewed );

	struct QuestConversation
	{
		gpwstring sSpeaker;
		gpwstring sConversation;
		gpstring sSample;
	};
	typedef std::vector< QuestConversation > QuestConversations;

	bool			  GetQuestConversations( int id, QuestConversations & questConversations );

// Chapters

	struct ChapterEvent
	{
		int			order;
		gpwstring	sDescription;
		gpstring	sSample;
	};
	typedef std::vector< ChapterEvent > ChapterEventColl;

	struct Chapter
	{
		gpstring			sName;
		gpwstring			sScreenName;		
		gpstring			sImage;
		int					currentOrder;
		bool				bActive;
		ChapterEventColl	events;

	};
	typedef std::vector< Chapter > ChapterColl;

	bool				GetChapter	( const gpwstring & sScreenName, Chapter & chapter );	
	Chapter *			FindChapterByName	( const gpwstring & sScreenName );	
	const ChapterColl & GetChapters	()											{ return m_Chapters; }
	bool				HasChapters ()											{ return ( m_Chapters.size() != 0 ? true : false ); }

FEX	void RSActivateChapter( const char * name, int state );
FEX	void RCActivateChapter( const char * name, int state );
	void ActivateChapter( const char * name, int state );

	///////////////////////////////////////////////////////////////
	//  Game Types

	struct TeamAsset
	{
		gpstring	m_Name;
		gpstring	m_TemplateName;
		Scid		m_Asset;
	};

	typedef std::vector< TeamAsset > TeamAssetColl;
	typedef std::map< Scid, gpstring > TeamAssetMap;
	typedef std::vector< gpstring > TeamStartColl;

	struct TeamInfo
	{
		int				m_Id;
		gpstring		m_TextureName;
		TeamAssetColl	m_Assets;
		TeamStartColl	m_StartLocations;
		bool			m_bRequireStartLocation;
	};

	typedef std::map< gpstring, TeamInfo, istring_less > TeamMap;

	typedef std::pair< gpstring, gpwstring > StatScreenPair;
	typedef std::vector< StatScreenPair > StatsColl;
	typedef std::vector< gpstring > RankingsColl;
	typedef std::map< gpstring, RankingsColl, istring_less > EndRankingsColl; // type of ranking(gas specified), ranking heading(should match code s_GameStat) 

	typedef std::vector< gpwstring > MessageColl;
	typedef std::map< gpstring, MessageColl, istring_less > MessageMap;

	class GameType
	{
		friend class Victory;

	public:

		GameType();
		~GameType();

		bool Load( FastFuelHandle hMapGameType );
		bool LoadDefinition( FastFuelHandle hGameType );
		bool Xfer( FuBi::PersistContext& persist );

		gpstring const & GetName() const								{  return m_Name;  }
		gpwstring const & GetScreenName() const							{  return m_ScreenName;  }
		gpwstring const & GetDescription() const						{  return m_Description;  }
		int GetId() const												{  return m_Id;  }

		RankingsColl const & GetInGameRankings() const					{  return m_InGameRankings;  }
		EndRankingsColl const & GetEndGameRankings() const						{  return m_EndGameRankings;  }

		bool GetAllowHumanPlayerKills() const							{  return m_bAllowHumanPlayerKills;  }
		bool GetAllowTeamAlliances() const								{  return m_bAllowTeamAlliances;  }
		bool GetAllowCharacterSave() const								{  return m_bAllowCharacterSave;  }

		DWORD GetFriendColor() const									{  return m_FriendColor;  }
		DWORD GetEnemyColor() const										{  return m_EnemyColor;  }

		const wchar_t * GetMessage( const gpstring & message ) const;

	private:

		gpstring		m_Name;
		gpwstring		m_ScreenName;
		gpwstring		m_Description;

		int				m_Id;  // internal id

		Skrit::Object*	m_Skrit;

		TeamMap			m_Teams;  // game type defined teams

		ConditionColl	m_VictoryConditions;  // game type defined victory conditions

		StatsColl		m_Stats;  // stats defined by game type

		RankingsColl	m_InGameRankings;  // leaderboard
		EndRankingsColl	m_EndGameRankings;  // game summary screen

		MessageMap		m_Messages;

		// game settings

		int				m_MinTeams;  // minimum number of teams allowed by game type

		bool			m_bAllowHumanPlayerKills;
		bool			m_bAllowTeamAlliances;
		bool			m_bAllowCharacterSave;

		DWORD			m_FriendColor;
		DWORD			m_EnemyColor;

		// internal settings

		bool			m_bIgnoreMapVictoryConditions;
	};

	typedef std::vector< GameType* > GameTypeColl;

	GameTypeColl const & GetMapGameTypes()								{  return m_GameTypes;  }

	gpstring GetGameTypeName( int id );

	void SSetGameType( const gpstring & gameType );
FEX	FuBiCookie RCSetGameType( const gpstring & gameType, DWORD machineId );
	FUBI_MEMBER_SET( RCSetGameType, +SERVER );
	bool SetGameType( const gpstring & gameType );

	GameType const * GetGameType()										{  return m_pGameType;  }

	ConditionColl const & GetVictoryConditions();

	//	Teams

	enum eTeamMode
	{
		NO_TEAMS,
		DEFINED_TEAMS,
		UNDEFINED_TEAMS,
	};

	eTeamMode GetTeamMode()												{  return m_TeamMode;  }

	int GetMinTeams();

	TeamMap & GetTeams();

	//  Game Stats

	inline int MakeStatId( const char * stat );

//	void SIncrementStat( eGameStat stat, int value = 1 );
//FEX void SIncrementStat( const char * stat, int value );
	void SIncrementStat( eGameStat stat, Player * player, int value = 1 );
FEX	void SIncrementStat( const char * stat, Player * player, int value );

FEX	void RCClearPlayerStats( PlayerId playerId );
	FUBI_MEMBER_SET( RCClearPlayerStats, +SERVER );

FEX	FuBiCookie RCSetPlayerStat( int stat, int player, int value, DWORD machineId = RPC_TO_ALL );
	FUBI_MEMBER_SET( RCSetPlayerStat, +SERVER );
FEX	FuBiCookie RCSetTeamStat( int stat, int team, int value, DWORD machineId = RPC_TO_ALL );
	FUBI_MEMBER_SET( RCSetTeamStat, +SERVER );
//FEX FuBiCookie RCSetGameStat( int stat, int value, DWORD machineId = RPC_TO_ALL );
//	FUBI_MEMBER_SET( RCSetGameStat, +SERVER );

FEX int GetPlayerStat( const char * stat, PlayerId player );
FEX	int	GetTeamStat  ( const char * stat, int team );
FEX	int GetGameStat  ( const char * stat );

	const gpwstring & GetStatScreenName( const char * stat ) const;
		  gpstring GetNameFromScreenName( gpwstring &screenName ) const;

	void TrackPlayerExperience	( int playerId, const char * skillName, double experience );
	void UpdatePlayerHistory	( PlayerId playerId, const char * historyName, bool forceEntry = false );
	void UpdatePlayerHistoryStats( const char * historyName );
	void UpdateAllPlayerHistory	( const char * historyName, bool forceEntry = false );
	void UpdateAllPlayerHistories( bool forceEntry = false );
FEX	void RCUpdateAllPlayerHistory( );
	

	float GetPlayerExperienceTimelineValue( PlayerId playerId, float time );
	bool GetPlayerHistoryValue( float &outHistoryValue, PlayerId playerId, const char * historyName );
	bool GetPlayerHistoryValue( float &outHistoryValue, double &outHistoryTime, int index, PlayerId playerId, const char * historyName );
	int  GetPlayerHistorySize ( PlayerId playerId, const char * historyName );

	AuditorDb * GetEndGameAuditor()				{  return m_pEndGameAudit;  }

	// Timelines
	enum  {  MAX_TIMELINE_ENTRIES = 32  };

	struct StatEntry
	{
		StatEntry()								{  Time = 0.0f; Value = 0.0f;  }
		StatEntry( double time, float value )	{  Time = time; Value = value;  }
		double	Time;
		float	Value;

		bool Xfer( FuBi::PersistContext& persist );
	};

	typedef std::deque< StatEntry > StatTimeline;
	typedef std::map< PlayerId, StatTimeline > StatTimelineMap;
	typedef std::map< gpstring, StatTimelineMap, istring_less > StatTimelineColl; // collection of all StatTimelineMaps'

	void AddTimelineEntry( StatTimeline & timeline, double time, float value );
	void AddHistoryEntry( StatTimeline & timeline, double time, float value, int numEntries, float timeDiff, bool forceEntry = false );

	StatTimeline GetTimeline( PlayerId playerId ) const;

	// timeline sync
	void XferForSyncEndGameTimeline( FuBi::PersistContext& persist );
FEX	void RCSyncEndGameTimeline( DWORD machineId, gpstring& syncBlock );
	

	// Game Messaging

FEX void SDisplayMessage( const char * msgName );
FEX void SDisplayMessage1Party( const char * msgName, Go * party );
FEX void SDisplayMessage2Party( const char * msgName, Go * party1, Go * party2 );
FEX void SLeaderMessage( eGameStat stat, const char * msgName );

FEX void RCDisplayMessage( const char * message );

private:
	
//	Game Settings

	gpstring			m_LoadedMap;

	bool				m_GameEnded;

	bool				m_bAllowDefeat;
	bool				m_bDefeated;

	float				m_DefeatDelay;
	float				m_VictoryDelay;

	float				m_TimeLimit;

	float				m_EndGameTime;

//	Victory Conditions

	ConditionColl		m_VictoryConditions;

//	Quests

	QuestColl			m_Quests;
	QuestCharColl		m_QuestsChar;
	int					m_questIdCounter;
	int					m_activeQuest;
	ChapterColl			m_Chapters;

	// Quest completion
	Goid				m_questCompletionActivator;
	bool				m_updateQuestCompletion;
	gpstring			m_sCompletedQuest;

	QuestUpdateCb		m_QuestUpdateCallback;

	DefeatGuiCb			m_DefeatCallback;

	MpVictoryGuiCb		m_MpVictoryCallback;

//	Game Types

	GameTypeColl		m_GameTypes;  // game types available for chosen map

	gpstring			m_GameType;
	gpstring			m_DefaultGameType;  // default game type for chosen map
	
	GameType*			m_pGameType;

	TeamMap				m_Teams;
	TeamAssetMap		m_TeamAssets;

	eTeamMode			m_TeamMode;

//	Game Stats

	AuditorDb *			m_pPlayerStats;
	AuditorDb *			m_pTeamStats;
	AuditorDb *			m_pGameStats;

	AuditorDb *			m_pEndGameAudit;

	StatTimelineMap		m_PlayerExperience;
	StatTimelineColl	m_PlayerStatTimelines;

	float				m_timelineTimeDelta;
	int					m_timelineMaxEntries;

	float				m_PlayerKillTotal;

	std::vector< gpwstring > m_GameStatScreenNames;

	FUBI_SINGLETON_CLASS( Victory, "Contains game types, quests, victory conditions." );

	SET_NO_COPYING( Victory );
};

#define gVictory Victory::GetSingleton()


#endif

