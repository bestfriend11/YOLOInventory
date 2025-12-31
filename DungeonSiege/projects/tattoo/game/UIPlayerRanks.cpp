/*=======================================================================================

	UIPlayerRanks

	author  :	Adam Swensen
	created :	Aug 2001

---------------------------------------------------------------------------------------*/


#include "precomp_game.h"
#include "uiplayerranks.h"

#include "fuel.h"
#include "victory.h"
#include "ui_emotes.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_text.h"
#include "server.h"
#include "player.h"
#include "GoActor.h"
#include "UIPartyManager.h"
#include "WorldTime.h"
#include "ui_textureman.h"


UIPlayerRanks :: UIPlayerRanks()
	: m_bVisible( false )
	, m_bInitialized( false )
	, m_PlayersVisible( MAX_VISIBLE_PLAYERS )
{
}

UIPlayerRanks :: ~UIPlayerRanks()
{
	Deactivate();
}

void UIPlayerRanks :: Init()
{
	if ( ::IsSinglePlayer() )
	{
		return;
	}

	gpassert( MAX_VISIBLE_PLAYERS == Server::MAX_PLAYERS );

	// Reinit ui
	Deactivate();
	gUIShell.ActivateInterface( "ui:interfaces:backend:multiplayer_ranks", false );

	m_bInitialized = false;
	m_bVisible = false;
	m_PlayersVisible = 0;

	m_Teams.resize( MAX_VISIBLE_PLAYERS );
	m_Players.resize( MAX_VISIBLE_PLAYERS );
	m_Classes.resize( MAX_VISIBLE_PLAYERS );
	m_Locations.resize( MAX_VISIBLE_PLAYERS );
	{for ( int playerSlot = 0; playerSlot < MAX_VISIBLE_PLAYERS; ++playerSlot )
	{
		m_Teams[ playerSlot ] = gUIShell.FindUIWindow( gpstringf( "player%dteam", playerSlot + 1 ), "multiplayer_ranks" );
		if ( !m_Teams[ playerSlot ] )
		{
			gperrorf( ( "Data error: Window: %s does not exist.", gpstringf( "player%dteam", playerSlot + 1 ) ) );
		}
		
		m_Players[ playerSlot ] = gUIShell.FindUIWindow( gpstringf( "player%dname", playerSlot + 1 ), "multiplayer_ranks" );
		if ( !m_Players[ playerSlot ] )
		{
			gperrorf( ( "Data error: Window: %s does not exist.", gpstringf( "player%dname", playerSlot + 1 ) ) );
		}
		
		m_Classes[ playerSlot ] = gUIShell.FindUIWindow( gpstringf( "player%dclass", playerSlot + 1 ), "multiplayer_ranks" );
		if ( !m_Classes[ playerSlot ] )
		{
			gperrorf( ( "Data error: Window: %s does not exist.", gpstringf( "player%dclass", playerSlot + 1 ) ) );
		}

		m_Locations[ playerSlot ] = gUIShell.FindUIWindow( gpstringf( "player%dlocation", playerSlot + 1 ), "multiplayer_ranks" );
		if ( !m_Locations[ playerSlot ] )
		{
			gperrorf( ( "Data error: Window: %s does not exist.", gpstringf( "player%dlocation", playerSlot + 1 ) ) );
		}		
	}}

	int rightEdge = 0;
	if ( m_Players[ 0 ] )
	{
		//rightEdge = m_Players[ 0 ]->GetRect().right;
		rightEdge = m_Classes[ 0 ]->GetRect().right;
	}

	// Create stat columns
	m_Stats.clear();

	int column = 1;
	const Victory::RankingsColl &rankings = gVictory.GetGameType()->GetInGameRankings();
	for ( Victory::RankingsColl::const_iterator rank = rankings.begin(); rank != rankings.end(); ++rank, ++column )
	{
		FastFuelHandle hStatHeader( "ui:interfaces:backend:multiplayer_ranks:multiplayer_ranks:stat_header_text" );
		gpassert( hStatHeader.IsValid() );
		UIText * pStatHeader = new UIText( hStatHeader, "multiplayer_ranks", NULL );
		pStatHeader->SetName( gpstringf( "stat%d_header", column ) );
		pStatHeader->SetRect( rightEdge + 2, rightEdge + STAT_COLUMN_WIDTH + 2, pStatHeader->GetRect().top, pStatHeader->GetRect().bottom );
		pStatHeader->SetText( gVictory.GetStatScreenName( *rank ) );
		pStatHeader->SetTag( column );
		pStatHeader->SetVisible( true );
		gUIShell.AddWindowToInterface( pStatHeader, "multiplayer_ranks" );

		FastFuelHandle hStatBg( "ui:interfaces:backend:multiplayer_ranks:multiplayer_ranks:stat_bg" );
		gpassert( hStatBg.IsValid() );
		UIWindow * pStatBg = new UIWindow( hStatBg, "multiplayer_ranks", NULL );
		pStatBg->SetName( gpstringf( "stat%d_bg", column ) );
		pStatBg->SetRect( rightEdge + 2, rightEdge + STAT_COLUMN_WIDTH + 2, pStatBg->GetRect().top, pStatBg->GetRect().bottom );
		pStatBg->SetAlpha( (column % 2 == 0) ? 0.2f : 0.3f );
		pStatBg->SetVisible( true );
		gUIShell.AddWindowToInterface( pStatBg, "multiplayer_ranks" );

		UIColl &StatColl = m_Stats.insert( UIStatsMap::value_type( *rank, UIColl() ) ).first->second;
		StatColl.resize( MAX_VISIBLE_PLAYERS );

		for ( int playerSlot = 0; playerSlot < MAX_VISIBLE_PLAYERS; ++playerSlot )
		{
			FastFuelHandle hStat( gpstringf( "ui:interfaces:backend:multiplayer_ranks:multiplayer_ranks:player%dstat", playerSlot + 1 ) );
			gpassert( hStat.IsValid() );
			StatColl[ playerSlot ] = new UIText( hStat, "multiplayer_ranks", NULL );
			StatColl[ playerSlot ]->SetName( gpstringf( "player%dstat%d", playerSlot + 1, column ) );
			StatColl[ playerSlot ]->SetRect( rightEdge + 2, rightEdge + STAT_COLUMN_WIDTH + 2, StatColl[ playerSlot ]->GetRect().top, StatColl[ playerSlot ]->GetRect().bottom );
			gUIShell.AddWindowToInterface( StatColl[ playerSlot ], "multiplayer_ranks" );
		}

		rightEdge = pStatHeader->GetRect().right;
	}

	//[player_ranks]
	//{
	//	// kill_trends info
	//	kt_update_period_in_sec = 60;
	//	kt_history_length = 10; 		
	//}

	m_killTrendUpdatePeriod = 60;
	m_killTrendHistoryLength = 10; 

	FastFuelHandle hPlayerRanks( "config:multiplayer:player_ranks");
	if( hPlayerRanks.IsValid() )
	{
		m_killTrendUpdatePeriod		= hPlayerRanks.GetFloat("kt_update_period_in_sec", m_killTrendUpdatePeriod);
		m_killTrendHistoryLength	= hPlayerRanks.GetInt("kt_history_length", m_killTrendHistoryLength); 
	}

	if ( !rankings.empty() )
	{
		m_SortStat = *rankings.begin();
	}

	{for ( int playerSlot = 0; playerSlot < MAX_VISIBLE_PLAYERS; ++playerSlot )
	{
		if ( m_Locations[ playerSlot ] )
		{
			m_Locations[ playerSlot ]->SetRect( rightEdge + 4, rightEdge + m_Locations[ playerSlot ]->GetRect().Width() + 4, m_Locations[ playerSlot ]->GetRect().top, m_Locations[ playerSlot ]->GetRect().bottom );
		}
	}}

	if ( m_Locations[ 0 ] )
	{
		rightEdge += m_Locations[ 0 ]->GetRect().Width() + 4;
	}

	// Resize windows
	UIWindow * pWindow = gUIShell.FindUIWindow( "location_header_text", "multiplayer_ranks" );
	if ( pWindow )
	{
		pWindow->SetRect( m_Locations[ 0 ]->GetRect().left, m_Locations[ 0 ]->GetRect().right, pWindow->GetRect().top, pWindow->GetRect().bottom );
	}

	pWindow = gUIShell.FindUIWindow( "player_selection", "multiplayer_ranks" );
	if( pWindow )
	{
		pWindow->SetRect( pWindow->GetRect().left, rightEdge, pWindow->GetRect().top, pWindow->GetRect().bottom );
	}
	pWindow = gUIShell.FindUIWindow( "players_background", "multiplayer_ranks" );
	if( pWindow )
	{
		pWindow->SetRect( pWindow->GetRect().left, rightEdge + 4, pWindow->GetRect().top, pWindow->GetRect().bottom );
	}
	pWindow = gUIShell.FindUIWindow( "game_timer_text", "multiplayer_ranks" );
	if( pWindow )
	{
		pWindow->SetRect( pWindow->GetRect().left, rightEdge + 4, pWindow->GetRect().top, pWindow->GetRect().bottom );
	}
	pWindow = gUIShell.FindUIWindow( "background", "multiplayer_ranks" );
	if( pWindow )
	{
		pWindow->SetRect( pWindow->GetRect().left, rightEdge + 14, pWindow->GetRect().top, pWindow->GetRect().bottom );
	}
	m_bInitialized = true;
}

void UIPlayerRanks :: Deactivate()
{
	for ( UIColl::iterator i = m_Teams.begin(); i != m_Teams.end(); ++i )
	{
		(*i)->SetTextureIndex( 0 );
		(*i)->SetHasTexture( false );
	}
	m_Teams.clear();

	gUIShell.DeactivateInterface( "multiplayer_ranks" );
}

void UIPlayerRanks :: SetVisible( bool visible )
{
	if ( !m_bInitialized )
	{
		return;
	}

	if ( visible && !m_bVisible )
	{
		gUIPartyManager.CloseAllCharacterMenus();
		gUIShell.ShowInterface( "multiplayer_ranks" );
		m_bVisible = true;

		int diffX = 0;

		UIWindow *pBackground = gUIShell.FindUIWindow( "background", "multiplayer_ranks" );
		if ( pBackground )
		{
			UIRect &rect = pBackground->GetRect();
			diffX = (gUIShell.GetScreenWidth() - rect.Width()) - rect.left;
		}

		GUInterface *pRanks = gUIShell.FindInterface( "multiplayer_ranks" );
		if ( pRanks )
		{
			for ( UIWindowMap::iterator i = pRanks->ui_windows.begin(); i != pRanks->ui_windows.end(); ++i )
			{
				UIRect &rect = i->second->GetRect();
				i->second->SetRect( rect.left + diffX, rect.right + diffX, rect.top, rect.bottom );
			}
		}
	}
	else if ( !visible && m_bVisible )
	{
		gUIShell.HideInterface( "multiplayer_ranks" );
		m_bVisible = false;
	}
}

void UIPlayerRanks :: Update( double /*secondsElapsed*/ )
{
	if ( !m_bInitialized )
	{
		return;
	}

	const int PlayerCount = Player::GetHumanPlayerCount();
	gpassert( PlayerCount <= MAX_VISIBLE_PLAYERS );

	if ( m_PlayersVisible != PlayerCount )
	{
		////////////////////////////
		//
		// Hide non-visible slots
		//
		////////////////////////////
		for ( int playerSlot = 0; playerSlot < MAX_VISIBLE_PLAYERS; ++playerSlot )
		{
			bool bSlotVisible = (playerSlot < PlayerCount);

			m_Teams[ playerSlot ]->SetVisible( bSlotVisible );
			m_Players[ playerSlot ]->SetVisible( bSlotVisible );
			m_Classes[ playerSlot ]->SetVisible( bSlotVisible );
			m_Locations[ playerSlot ]->SetVisible( bSlotVisible );

			for ( UIStatsMap::iterator i = m_Stats.begin(); i != m_Stats.end(); ++i )
			{
				i->second[ playerSlot ]->SetVisible( bSlotVisible );
			}
		}

		////////////////////////////
		//
		// Resize windows
		//
		////////////////////////////
		int bottomEdge = m_Players[ PlayerCount - 1 ]->GetRect().bottom;
		
		for ( UINT column = 1; column <= m_Stats.size(); ++column )
		{
			UIWindow * pWindow = gUIShell.FindUIWindow( gpstringf( "stat%d_bg", column ), "multiplayer_ranks" );
			if ( pWindow )
			{
				pWindow->SetRect( pWindow->GetRect().left, pWindow->GetRect().right, pWindow->GetRect().top, bottomEdge );
			}
		}

		UIWindow * pWindow = gUIShell.FindUIWindow( "players_background", "multiplayer_ranks" );
		if( pWindow )
		{
			pWindow->SetRect( pWindow->GetRect().left, pWindow->GetRect().right, pWindow->GetRect().top, bottomEdge + 4 );
		}
		pWindow = gUIShell.FindUIWindow( "game_timer_text", "multiplayer_ranks" );
		if( pWindow )
		{
			pWindow->SetRect( pWindow->GetRect().left, pWindow->GetRect().right, bottomEdge + 10, bottomEdge + 26 );
		}
		pWindow = gUIShell.FindUIWindow( "background", "multiplayer_ranks" );
		if( pWindow )
		{
			pWindow->SetRect( pWindow->GetRect().left, pWindow->GetRect().right, pWindow->GetRect().top, bottomEdge + 36 );
		}

		m_PlayersVisible = PlayerCount;
	}
	////////////////////////////
	//
	// Update interface
	//
	////////////////////////////

	DWORD Plrs = 0;
	int plrIdx = 0;

	{for ( int i = 0; i < PlayerCount; ++i )
	{
		int maxStat = 0;
		Server::PlayerColl::const_iterator usePlayer = gServer.GetPlayers().end();
		for ( Server::PlayerColl::const_iterator findMax = gServer.GetPlayers().begin(); findMax != gServer.GetPlayers().end(); ++findMax )
		{
			if ( !IsValid(*findMax) || (*findMax)->IsComputerPlayer() )
			{
				continue;
			}

			int numStat = gVictory.GetPlayerStat( m_SortStat, (*findMax)->GetId() );
			if ( (numStat >= maxStat) && ((Plrs & MakeInt( (*findMax)->GetId() )) == 0) )
			{
				usePlayer = findMax;
				maxStat = numStat;
			}
		}

		if ( usePlayer != gServer.GetPlayers().end() )
		{
			Plrs |= MakeInt( (*usePlayer)->GetId() );

			Team * team = gServer.GetTeam( (*usePlayer)->GetId() );
			if ( team && (team->m_TextureId != 0) )
			{
				m_Teams[ plrIdx ]->SetTextureIndex( team->m_TextureId );
				m_Teams[ plrIdx ]->SetHasTexture( true );
			}
			else
			{
				m_Teams[ plrIdx ]->SetTextureIndex( 0 );
				m_Teams[ plrIdx ]->SetHasTexture( false );
			}

			((UIText *)m_Players[ plrIdx ])->SetText( (*usePlayer)->GetHeroName() );
			
			Go * party = (*usePlayer)->GetParty();
			if( party && !party->GetChildren().empty() )
			{
				GoHandle hero( *(party->GetChildren().begin()) );
				if ( hero )
				{
					if ( hero->GetActor()->GetClass().empty() )
					{
						gRules.UpdateClassDesignation( hero->GetGoid() );
					}
					((UIText *)m_Classes[ plrIdx ])->SetText( ReportSys::TranslateW( hero->GetActor()->GetClass() ) );
				}
			}

			if ( !(*usePlayer)->GetWorldLocation().empty() &&
				(gServer.GetLocalHumanPlayer()->GetFriendTo() & MakeInt( (*usePlayer)->GetId() )) == 0 )
			{
				((UIText *)m_Locations[ plrIdx ])->SetText( gpwtranslate( $MSG$ "Unknown" ) );
			}
			else
			{
				((UIText *)m_Locations[ plrIdx ])->SetText( ReportSys::TranslateW( (*usePlayer)->GetWorldLocation() ) );
			}
			
			for ( UIStatsMap::iterator stat = m_Stats.begin(); stat != m_Stats.end(); ++stat )
			{
				int value = gVictory.GetPlayerStat( stat->first, (*usePlayer)->GetId() );
				gpwstring displayStringValue = UpdateColorIcon( stat->first, (*usePlayer)->GetId() );
				displayStringValue.appendf( L"%d", value );
				gUIEmotes.PrepareString(displayStringValue);
				((UIText *)stat->second[ plrIdx ])->SetText( displayStringValue );
			}

			if ( *usePlayer == gServer.GetLocalHumanPlayer() )
			{
				UIWindow * pWindow = gUIShell.FindUIWindow( "player_selection", "multiplayer_ranks" );
				if ( pWindow )
				{
					pWindow->SetRect( pWindow->GetRect().left, pWindow->GetRect().right, m_Teams[ plrIdx ]->GetRect().top, m_Teams[ plrIdx ]->GetRect().bottom );
					pWindow->SetVisible( true );
				}
			}

			++plrIdx;
		}
	}}
}

gpwstring UIPlayerRanks :: UpdateColorIcon( const char* statType, PlayerId playerIndex, bool visible )
{
	gpwstring colorIconText;
	colorIconText.assignf(L"");

	if( !strcmp( statType, "kills_trend" ) )
	{
		int statValue = gVictory.GetPlayerStat( statType, playerIndex );
		
		if ( visible ) 
		{
			if( statValue > 0 )
			{
				colorIconText.assign(L"(playerrank_green)(playerrank_up)");
			}
			else if( statValue < 0 )
			{
				colorIconText.assign(L"(playerrank_red)(playerrank_down)");
			}
		}	
	}
	return colorIconText;
}

void UIPlayerRanks :: SortByColumn( int column )
{
	int index = 1;
	const Victory::RankingsColl &rankings = gVictory.GetGameType()->GetInGameRankings();
	for ( Victory::RankingsColl::const_iterator rank = rankings.begin(); rank != rankings.end(); ++rank, ++index )
	{
		if ( index == column )
		{
			m_SortStat = *rank;
			return;
		}
	}
}
