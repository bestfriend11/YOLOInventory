/*=======================================================================================

	UIPlayerRanks

	author  :	Adam Swensen
	created :	Aug 2001

	purpose :	Displays in-game player rankings for multiplayer.

---------------------------------------------------------------------------------------*/

#pragma once
#ifndef __UIPLAYERRANKS_H
#define __UIPLAYERRANKS_H


class UIWindow;


class UIPlayerRanks
{
public:
	
	UIPlayerRanks();
	~UIPlayerRanks();
	
	void Init();
	
	void Deactivate();

	void SetVisible( bool visible );
	bool IsVisible()							{  return m_bVisible;  }
	
	void Update( double secondsElapsed );
	
	void SortByColumn( int column );
	
	gpwstring UpdateColorIcon( const char* statType, PlayerId playerIndex, bool visible = true );

	float	GetKillTrendUpdatePeriod()	{ return m_killTrendUpdatePeriod; }
	int		GetKillTrendHistoryLength() { return m_killTrendHistoryLength; }
	
private:

	enum
	{
		MAX_VISIBLE_PLAYERS = 8,
		STAT_COLUMN_WIDTH = 60,
	};

	bool		m_bVisible;
	bool		m_bInitialized;
	int			m_PlayersVisible;

	gpstring	m_SortStat;

	typedef std::vector< UIWindow * > UIColl;
	typedef std::map< gpstring, UIColl, istring_less > UIStatsMap;

	UIColl		m_Teams;
	UIColl		m_Players;
	UIColl		m_Classes;
	UIColl		m_Locations;
	UIStatsMap	m_Stats;

	float m_killTrendUpdatePeriod;
	int	m_killTrendHistoryLength;

};

#endif