//////////////////////////////////////////////////////////////////////////////
//
// File     :  UIZoneMatchFilter.cpp
// Author(s):  Jessica Tams
//
// Copyright © 2002 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "ui_combobox.h"


void UIZoneMatch :: SetDefaultFilterSelections( UIListbox *pList, const char* selectedOption, eZoneGameFlag flag )
{
	if( pList )
	{
		pList->SetVisible( false  );
	
		// if this flag is selected, then select this element.
		if( m_ActiveFilters & flag )
		{
			pList->SelectElement( m_ActiveFilters & flag );
		}
		else if( m_ActivePairFilters & flag )
		{
			pList->SelectElement( (int) ZONE_GAME_FLAG_DISABLED );
		}
		else
		{
			pList->SelectElement( (int) ZONE_GAME_NO_FLAG );
		}

		UIComboBox * pCombo = (UIComboBox *)gUIShell.FindUIWindow( selectedOption, "zonematch_games_filter" );
		if ( pCombo )
		{
			pCombo->SetText( pList->GetSelectedText() );
		}
	}
}



/////////////////////////////////////////////////////////////////////////////
//
//
//			Show the filter interface and set the appropriate default values
//
//
/////////////////////////////////////////////////////////////////////////////
void UIZoneMatch :: ShowFiltersInterface()
{
	gUIShell.ShowInterface( "zonematch_games_filter" );

	///////////////////////////////////////////////////////////////////////////////
	//
	//		PLAYER VS PLAYER
	//
	///////////////////////////////////////////////////////////////////////////////
	UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_pvp", "zonematch_games_filter" );
	if( pList )
	{
		pList->RemoveAllElements();
		pList->AddElement( gpwtranslate( $MSG$ "--Show All--" ), (int) ZONE_GAME_NO_FLAG );
		pList->AddElement( gpwtranslate( $MSG$ "PVP ENABLED" ), (int) ZONE_GAME_PVP_ENABLED );
		pList->AddElement( gpwtranslate( $MSG$ "PVP DISABLED" ), (int) ZONE_GAME_FLAG_DISABLED );

		SetDefaultFilterSelections( pList, "combo_box_pvp", ZONE_GAME_PVP_ENABLED );
	}
	
	///////////////////////////////////////////////////////////////////////////////
	//
	//		TEAMS ENABLED
	//
	///////////////////////////////////////////////////////////////////////////////
	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_team", "zonematch_games_filter" );
	if( pList )
	{
		pList->RemoveAllElements();
		pList->AddElement( gpwtranslate( $MSG$ "--Show All--" ), (int) ZONE_GAME_NO_FLAG );
		pList->AddElement( gpwtranslate( $MSG$ "TEAMS ENABLED" ), (int) ZONE_GAME_TEAMS_ENABLED );
		pList->AddElement( gpwtranslate( $MSG$ "TEAMS DISABLED" ), (int) ZONE_GAME_FLAG_DISABLED );

		
		SetDefaultFilterSelections( pList, "combo_box_team", ZONE_GAME_TEAMS_ENABLED );
	}
	///////////////////////////////////////////////////////////////////////////////
	//
	//		REGULAR/VETERAN/ELITE
	//
	///////////////////////////////////////////////////////////////////////////////
	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_worldlevel", "zonematch_games_filter" );
	if( pList )
	{
		pList->RemoveAllElements();
		//pList->AddElement( gpwtranslate( $MSG$ "--Show All--" ), (int) ZONE_GAME_NO_FLAG );
		pList->AddElement( gpwtranslate( $MSG$ "REGULAR Games" ), (int)  ZONE_GAME_WORLD_REGULAR );
		pList->AddElement( gpwtranslate( $MSG$ "VETERAN Games" ), (int)  ZONE_GAME_WORLD_VETERAN );
		pList->AddElement( gpwtranslate( $MSG$ "ELITE Games" ), (int)  ZONE_GAME_WORLD_ELITE );
		
		SetDefaultFilterSelections( pList, "combo_box_worldlevel", ZONE_GAME_WORLD );
	}
		
	///////////////////////////////////////////////////////////////////////////////
	//
	//		DROP INVENTORY ON DEATH
	//
	///////////////////////////////////////////////////////////////////////////////
	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_inventory", "zonematch_games_filter" );
	if( pList )
	{
		pList->RemoveAllElements();
		pList->AddElement( gpwtranslate( $MSG$ "--Show All--" ), (int) ZONE_GAME_NO_FLAG );
//		pList->AddElement( gpwtranslate( $MSG$ "Drop Equipped" ), (int)  ZONE_GAME_DROP_EQUIPPED_ON_DEATH );
		pList->AddElement( gpwtranslate( $MSG$ "DROP ITEMS on Death" ), (int)  ZONE_GAME_DROP_ANYTHING_ON_DEATH );
//		pList->AddElement( gpwtranslate( $MSG$ "Drop Everything" ), (int)  ZONE_GAME_DROP_EVERYTHING_ON_DEATH );
		pList->AddElement( gpwtranslate( $MSG$ "KEEP ITEMS on Death" ), (int)  ZONE_GAME_FLAG_DISABLED );


		SetDefaultFilterSelections( pList, "combo_box_inventory", ZONE_GAME_DROP_ANYTHING_ON_DEATH );
	}

	///////////////////////////////////////////////////////////////////////////////
	//
	//		UBER LEVEL OF THE CHARACTER
	//
	///////////////////////////////////////////////////////////////////////////////
	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_uber", "zonematch_games_filter" );
	if( pList )
	{
		pList->RemoveAllElements();
		pList->AddElement( gpwtranslate( $MSG$ "--Show All--" ), (int) ZONE_GAME_NO_FLAG );
		pList->AddElement( gpwtranslate( $MSG$ "Under Level 10" ), (int)  ZONE_GAME_HAS_UBER_CHARACTERS_10 );
		pList->AddElement( gpwtranslate( $MSG$ "Under Level 25" ), (int)  ZONE_GAME_HAS_UBER_CHARACTERS_25 );
		pList->AddElement( gpwtranslate( $MSG$ "Under Level 50" ), (int)  ZONE_GAME_HAS_UBER_CHARACTERS_50 );
		pList->AddElement( gpwtranslate( $MSG$ "Under Level 100" ), (int)  ZONE_GAME_HAS_UBER_CHARACTERS_100 );

		if( pList )
		{
			pList->SetVisible( false  );
		
			// if this flag is selected, then select this element.
			if( m_ActiveFilters & ZONE_GAME_HAS_UBER_CHARACTERS_MAX && m_ActivePairFilters & ZONE_GAME_HAS_UBER_CHARACTERS_MAX )
			{
				pList->SelectElement( ZONE_GAME_NO_FLAG );
			}
			else if( m_ActiveFilters & ZONE_GAME_HAS_UBER_CHARACTERS_100 && m_ActivePairFilters & ZONE_GAME_HAS_UBER_CHARACTERS_100 )
			{
				pList->SelectElement( (int) ZONE_GAME_HAS_UBER_CHARACTERS_100 );
			}
			else if( m_ActiveFilters & ZONE_GAME_HAS_UBER_CHARACTERS_50  && m_ActivePairFilters & ZONE_GAME_HAS_UBER_CHARACTERS_50 )
			{
				pList->SelectElement( (int) ZONE_GAME_HAS_UBER_CHARACTERS_50 );
			}
			else if( m_ActiveFilters & ZONE_GAME_HAS_UBER_CHARACTERS_25  && m_ActivePairFilters & ZONE_GAME_HAS_UBER_CHARACTERS_25 )
			{
				pList->SelectElement( (int) ZONE_GAME_HAS_UBER_CHARACTERS_25 );
			}
			else if( m_ActiveFilters & ZONE_GAME_HAS_UBER_CHARACTERS_10  && m_ActivePairFilters & ZONE_GAME_HAS_UBER_CHARACTERS_10 )
			{
				pList->SelectElement( (int) ZONE_GAME_HAS_UBER_CHARACTERS_10 );
			}
			else
			{
				pList->SelectElement( ZONE_GAME_NO_FLAG );
			}

			UIComboBox * pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_box_uber", "zonematch_games_filter" );
			if ( pCombo )
			{
				pCombo->SetText( pList->GetSelectedText() );
			}
		}
	}
	///////////////////////////////////////////////////////////////////////////////
	//
	//		FULL GAMES
	//
	///////////////////////////////////////////////////////////////////////////////
	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_full", "zonematch_games_filter" );
	if( pList )
	{
		pList->RemoveAllElements();
		pList->AddElement( gpwtranslate( $MSG$ "--Show All--" ), (int) ZONE_GAME_NO_FLAG );
		//pList->AddElement( gpwtranslate( $MSG$ "Yes" ), (int) ZONE_GAME_FULL );
		pList->AddElement( gpwtranslate( $MSG$ "Not Full" ), (int) ZONE_GAME_FLAG_DISABLED );

		SetDefaultFilterSelections( pList, "combo_box_full", ZONE_GAME_FULL );
	}
	
	///////////////////////////////////////////////////////////////////////////////
	//
	//		PASSWORD PROTECTION
	//
	///////////////////////////////////////////////////////////////////////////////
	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_password", "zonematch_games_filter" );
	if( pList )
	{
		pList->RemoveAllElements();
		pList->AddElement( gpwtranslate( $MSG$ "--Show All--" ), (int) ZONE_GAME_NO_FLAG );
		//pList->AddElement( gpwtranslate( $MSG$ "Yes" ), (int) ZONE_GAME_PASSWORD_PROTECTED );
		pList->AddElement( gpwtranslate( $MSG$ "No Password" ), (int) ZONE_GAME_FLAG_DISABLED );

		SetDefaultFilterSelections( pList, "combo_box_password", ZONE_GAME_PASSWORD_PROTECTED );
	}

	///////////////////////////////////////////////////////////////////////////////
	//
	//		GAME IN PROGRESS -- JOIN IN PROGRESS
	//
	///////////////////////////////////////////////////////////////////////////////
	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_progress", "zonematch_games_filter" );
	if( pList )
	{
		pList->RemoveAllElements();
		pList->AddElement( gpwtranslate( $MSG$ "--Show All--" ), (int) ZONE_GAME_NO_FLAG );
		pList->AddElement( gpwtranslate( $MSG$ "In Progress" ), (int) ZONE_GAME_IN_PROGRESS );
		pList->AddElement( gpwtranslate( $MSG$ "In Staging Area" ), (int) ZONE_GAME_FLAG_DISABLED );

		SetDefaultFilterSelections( pList, "combo_box_progress", ZONE_GAME_IN_PROGRESS );
	}

	///////////////////////////////////////////////////////////////////////////////
	//
	//		LANGUAGE OF THE HOST
	//
	///////////////////////////////////////////////////////////////////////////////
	// enable when GUN supports this.
//	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_language", "zonematch_games_filter" );
//	if( pList )
//	{
//		pList->RemoveAllElements();
//		pList->AddElement( gpwtranslate( $MSG$ "Any Language" ), (int) ZONE_GAME_NO_FLAG );
//		pList->AddElement( gpwtranslate( $MSG$ "My Language" ), (int) m_LanguageFilterFlag );
//
//		pList->SetVisible( false  );
//
//		int selectionMask = m_LanguageFilterFlag;
//		pList->SelectElement( m_ActiveFilters & selectionMask );
//		
//		UIComboBox * pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_box_language", "zonematch_games_filter" );
//		if ( pCombo )
//		{
//			pCombo->SetText( pList->GetSelectedText() );
//		}
//	}
}	


void UIZoneMatch::SetFilter( UIListbox *pList, eZoneGameFlag flag )
{
	if( pList )
	{
		if( pList->GetSelectedTag() != ZONE_GAME_NO_FLAG )
		{
			if( pList->GetSelectedTag() == ZONE_GAME_FLAG_DISABLED )
			{
				// turn off all filters for this flag or ones linked too it
				m_ActivePairFilters		|= flag;
			}
			else
			{
				// select our filter, and turn off all others linked with this one.
				m_ActiveFilters			|= pList->GetSelectedTag();
				m_ActivePairFilters		|= flag ^ pList->GetSelectedTag();
			}		
		}
	}
}

///////////////////////////////////////////////////////////////////////////////	
//
//			Hide the filter interface and save out all the data!
//
//	pvp
//	password
//	progress
//	full
//	uber
///////////////////////////////////////////////////////////////////////////////
void UIZoneMatch::HideFiltersInterface()
{
	m_ActiveFilters = 0;
	m_ActivePairFilters = 0;
	///////////////////////////////////////////////////////////////////////////////
	//		PLAYER VS PLAYER
	///////////////////////////////////////////////////////////////////////////////
	UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_pvp", "zonematch_games_filter" );
	if( pList )
	{
		SetFilter( pList, ZONE_GAME_PVP_ENABLED );
	}
	
	///////////////////////////////////////////////////////////////////////////////
	//		PASSWORD PROTECTION
	///////////////////////////////////////////////////////////////////////////////
	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_password", "zonematch_games_filter" );
	if( pList )
	{
		SetFilter( pList, ZONE_GAME_PASSWORD_PROTECTED );
	}

	///////////////////////////////////////////////////////////////////////////////
	//		GAME IN PROGRESS -- JOIN IN PROGRESS
	///////////////////////////////////////////////////////////////////////////////
	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_progress", "zonematch_games_filter" );
	if( pList )
	{
		SetFilter( pList, ZONE_GAME_IN_PROGRESS );
	}
	
	///////////////////////////////////////////////////////////////////////////////
	//		FULL GAMES
	///////////////////////////////////////////////////////////////////////////////
	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_full", "zonematch_games_filter" );
	if( pList )
	{
		SetFilter( pList, ZONE_GAME_FULL );
	}
	
	///////////////////////////////////////////////////////////////////////////////
	//		UBER LEVEL OF THE CHARACTER
	///////////////////////////////////////////////////////////////////////////////
	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_uber", "zonematch_games_filter" );
	if( pList )
	{
		// now set all the ubers above this too.  for they will also be true.  special
		// case different from other filters.
		if( pList )
		{
			if( pList->GetSelectedTag() == ZONE_GAME_NO_FLAG )
			{
				// set all the filters to the exclusive mode
				m_ActiveFilters			|= ZONE_GAME_HAS_UBER_CHARACTERS;
				m_ActivePairFilters		|= ZONE_GAME_HAS_UBER_CHARACTERS;
			}
			else if( pList->GetSelectedTag() == ZONE_GAME_FLAG_DISABLED )
			{
				// set the filters to the disabled mode
				m_ActivePairFilters		|= ZONE_GAME_HAS_UBER_CHARACTERS;
			}
			else 
			{
				// set the filters to the exclusive mode
				m_ActiveFilters			|= ZONE_GAME_HAS_UBER_CHARACTERS;
				m_ActivePairFilters		|= ZONE_GAME_HAS_UBER_CHARACTERS;

				// now set the filters that we don't want
				if( pList->GetSelectedTag() < ZONE_GAME_HAS_UBER_CHARACTERS_25 ) 
				{
					m_ActiveFilters		^= ZONE_GAME_HAS_UBER_CHARACTERS_25;
				}
				if( pList->GetSelectedTag() < ZONE_GAME_HAS_UBER_CHARACTERS_50) 
				{
					m_ActiveFilters		^= ZONE_GAME_HAS_UBER_CHARACTERS_50;
				}	
				if( pList->GetSelectedTag() < ZONE_GAME_HAS_UBER_CHARACTERS_100) 
				{
					m_ActiveFilters		^= ZONE_GAME_HAS_UBER_CHARACTERS_100;
				}
				if( pList->GetSelectedTag() < ZONE_GAME_HAS_UBER_CHARACTERS_MAX) 
				{
					m_ActiveFilters		^= ZONE_GAME_HAS_UBER_CHARACTERS_MAX;
				}
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	//		REGULAR/VETERAN/ELITE
	///////////////////////////////////////////////////////////////////////////////
	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_worldlevel", "zonematch_games_filter" );
	if( pList )
	{
		SetFilter( pList, ZONE_GAME_WORLD );
	}
	
	///////////////////////////////////////////////////////////////////////////////
	//		NEW CHARACTERS ONLY
	///////////////////////////////////////////////////////////////////////////////
//	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_newchar", "zonematch_games_filter" );
//	if( pList )
//	{
//		SetFilter( pList, ZONE_GAME_NEW_CHARACTERS_ONLY );
//	}

	///////////////////////////////////////////////////////////////////////////////
	//		DROP INVENTORY ON DEATH
	///////////////////////////////////////////////////////////////////////////////
	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_inventory", "zonematch_games_filter" );
	if( pList )
	{
		SetFilter( pList, ZONE_GAME_DROP_ANYTHING_ON_DEATH );
	}

	///////////////////////////////////////////////////////////////////////////////
	//		TEAMS ENABLED
	///////////////////////////////////////////////////////////////////////////////
	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_team", "zonematch_games_filter" );
	if( pList )
	{
		SetFilter( pList, ZONE_GAME_TEAMS_ENABLED );
	}

	///////////////////////////////////////////////////////////////////////////////
	//		HOST LANGUAGE
	///////////////////////////////////////////////////////////////////////////////
// enable when GUN supports this.
//	pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_language", "zonematch_games_filter" );
//	if( pList )
//	{
//		m_ActiveFilters |= pList->GetSelectedTag();	
//		if( pList->GetSelectedTag() != ZONE_GAME_NO_FLAG )
//		{
//			m_ActivePairFilters  |= ( m_LanguageFilterFlag );
//		}	
//	}
//	else
//	{
//		// always make sure that we only add this to the query
//		m_ActivePairFilters  |= ( m_LanguageFilterFlag );
//	}

	/////////////////////////////////////////////////////////////////
	//
	//	Filters are all ready to go now.  
	//
	if ( m_GameViewType == SHOW_FILTERED_GAMES )
	{
		m_bRefreshGamesList = true;
	}

	SaveZoneSettings();

	gUIShell.HideInterface( "zonematch_games_filter" );
}

///////////////////////////////////////////////////////////////
//
//		This is GUN's fault.  I promise.  Really.
//
//
///////////////////////////////////////////////////////////////
//
// ( m_ActiveFilters, m_ActivePairFilters )
//
//	Disabled	== ( 0, 1 ) ( not active, negative pair active )
//	Enabled		== ( 1, 0 ) ( active, negative pair not active )
//	All Combos	== ( 0, 0 ) ( nothing active or not active )
//	Exclusive	== ( 1, 1 ) ( active and pair active )
//
//	m_Filters			== All Possible Filters
//	m_ExclusiveFilters	== All Exclusive Filters
//

gpstring UIZoneMatch::CreateFiltersList( )
{
	// calculate all the required filters then we will add those to all the combos
	// of the non-required filters
	gpstring	filteredFlags;
	DWORD		currentMultiplier = 0;
	std::vector<DWORD>::iterator iFilters;
	for ( iFilters = m_Filters.begin(); iFilters != m_Filters.end(); ++iFilters )
	{
		if( (*iFilters & m_ActiveFilters) && !(*iFilters & m_ActivePairFilters) )
		{
			currentMultiplier |= *iFilters;
		}
	}

	filteredFlags.append( CreateFiltersListExclusive( currentMultiplier ) );
	
	gpstring passedFlags;
	// if we dont' have any exclusive filters, do the combos with out any exclusives...
	if( filteredFlags.empty() )
	{
		filteredFlags.append( CreateFiltersListCombos( passedFlags, currentMultiplier, m_Filters.size() - 1 ) );
	}

	return filteredFlags;
}

gpstring UIZoneMatch::CreateFiltersListExclusive( DWORD currentMultiplier )
{
	gpstring filteredFlags;

	//or in our exclusive filters to the combos
	for( unsigned int j = 0; j < m_ExclusiveFilters.size(); j++ )
	{
		for( unsigned int i = 0; i < m_Filters.size(); i++ )
		{
			// filter is exclusive, and has been set to exclusive enabled
			if( (m_Filters[i] & m_ExclusiveFilters[j]) && (m_Filters[i] & m_ActiveFilters) && (m_Filters[i] & m_ActivePairFilters) )
			{
				if( !filteredFlags.empty() )
				{
					filteredFlags.appendf( "," );
				}
				gpstring passedFlags;
				filteredFlags.appendf( "%s", CreateFiltersListCombos(passedFlags, currentMultiplier|m_Filters[i], m_Filters.size() - 1 ) );
			}
		}
	}

	return filteredFlags;
}

gpstring UIZoneMatch::CreateFiltersListCombos( gpstring filteredFlags, DWORD currentMultiplier, int position )
{
	if( position == 1 )
	{
		//"a"
		if ( !(m_Filters[0] & m_ActiveFilters) && !(m_Filters[0] & m_ActivePairFilters) )
		{
			if( !filteredFlags.empty() )
			{
				filteredFlags.appendf( "," );
			}
			filteredFlags.appendf( "%d", m_Filters[0] | currentMultiplier ); 	
		}
		//"b"
		if ( !(m_Filters[1] & m_ActiveFilters) && !(m_Filters[1] & m_ActivePairFilters) )
		{
			if( !filteredFlags.empty() )
			{
				filteredFlags.appendf( "," );
			}
			filteredFlags.appendf( "%d", m_Filters[1] | currentMultiplier ); 	
		}
		//"ab"
		if ( !(m_Filters[0] & m_ActiveFilters) && !(m_Filters[0] & m_ActivePairFilters) 
			&& !(m_Filters[1] & m_ActiveFilters) && !(m_Filters[1] & m_ActivePairFilters) )
		{
			if( !filteredFlags.empty() )
			{
				filteredFlags.appendf( "," );
			}
			filteredFlags.appendf( "%d", m_Filters[0] | m_Filters[1] | currentMultiplier ); 	
		}
		return filteredFlags;
	}

	gpstring passedFilterFlags;
	passedFilterFlags = filteredFlags;
	
	//aCb = a, b, ab
	// this is the "a" part
	gpstring currentFilterFlags = CreateFiltersListCombos( passedFilterFlags, currentMultiplier, position - 1 ).c_str();
	if( ! currentFilterFlags.empty() )
	{
		if( !filteredFlags.empty() )
		{
			filteredFlags.appendf( "," );
		}
		filteredFlags.appendf( "%s", currentFilterFlags ); 
	}

	if ( !(m_Filters[position] & m_ActiveFilters) && !(m_Filters[position] & m_ActivePairFilters) )
	{
		// this is the "b" part
		if( !filteredFlags.empty() )
		{
			filteredFlags.appendf( "," );
		}
		filteredFlags.appendf( "%d", m_Filters[position] | currentMultiplier ); 

		// this is the "ab" part
		currentFilterFlags = CreateFiltersListCombos( passedFilterFlags, m_Filters[position] | currentMultiplier, position - 1 ).c_str();
		if( ! currentFilterFlags.empty() )
		{
			if( !filteredFlags.empty() )
			{
				filteredFlags.appendf( "," );
			}
			filteredFlags.appendf( "%s", currentFilterFlags );
		}
	}		

	if( (unsigned int) position == m_Filters.size() - 1 )
	{
		if( !filteredFlags.empty() )
		{
			filteredFlags.appendf( "," );
		}
		filteredFlags.appendf( "%d", currentMultiplier ); 
	}
	return filteredFlags;
}

/////////////////////////////////////////////////////////////////////
//
//	helper function to set the correct flags for the uber level
//	of characters.
//
//
void UIZoneMatch::UpdateHostUberLevelFlags()
{
	if ( IsHosting() )
	{
		HostRefreshPlayerList();
		int maxUberLevel = 0;
		for ( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
		{
			if ( IsValid( *i ) && !(*i)->IsComputerPlayer() )
			{
				if ( (*i)->GetHeroUberLevel() >= maxUberLevel )
				{
					maxUberLevel = (int) (*i)->GetHeroUberLevel();
				}
			}
		}

		if( (maxUberLevel/10) == 0 )
		{
			HostUpdateGameFlag( (eZoneGameFlag) (ZONE_GAME_HAS_UBER_CHARACTERS ^ ZONE_GAME_HAS_UBER_CHARACTERS_10), false );
			HostUpdateGameFlag( ZONE_GAME_HAS_UBER_CHARACTERS_10, true );
		}
		else if( (maxUberLevel/25) == 0 )
		{
			HostUpdateGameFlag( (eZoneGameFlag) (ZONE_GAME_HAS_UBER_CHARACTERS ^ ZONE_GAME_HAS_UBER_CHARACTERS_25), false );
			HostUpdateGameFlag( ZONE_GAME_HAS_UBER_CHARACTERS_25, true );
		}
		else if( (maxUberLevel/50) == 0 )
		{
			HostUpdateGameFlag( (eZoneGameFlag) (ZONE_GAME_HAS_UBER_CHARACTERS ^ ZONE_GAME_HAS_UBER_CHARACTERS_50), false );
			HostUpdateGameFlag( ZONE_GAME_HAS_UBER_CHARACTERS_50, true );
		}
		else if( (maxUberLevel/100) == 0 )
		{
			HostUpdateGameFlag( (eZoneGameFlag) (ZONE_GAME_HAS_UBER_CHARACTERS ^ ZONE_GAME_HAS_UBER_CHARACTERS_100), false );
			HostUpdateGameFlag( ZONE_GAME_HAS_UBER_CHARACTERS_100, true );
		}
		else
		{
			HostUpdateGameFlag( (eZoneGameFlag) (ZONE_GAME_HAS_UBER_CHARACTERS ^ ZONE_GAME_HAS_UBER_CHARACTERS_MAX), false );
			HostUpdateGameFlag( ZONE_GAME_HAS_UBER_CHARACTERS_MAX, true );
		}
	}
}
/////////////////////////////////////////////////////////////////////
//
//	helper function to set the correct flags for the map world level
//
//
void UIZoneMatch::SetHostWorldFlags()
{
	bool bWorldSet = false;
	if ( gWorldMap.IsInitialized() )
	{
		MpWorldColl worlds;
		gWorldMap.GetMpWorlds( worlds );
		for ( MpWorldColl::iterator i = worlds.begin(); i != worlds.end(); ++i )
		{
			if ( i->m_Name.same_no_case( gWorldMap.GetMpWorldName() ) )
			{
				UpdateHostWorldFlags( i->m_ScreenName );
				bWorldSet = true;
				break;
			}
		}
	}
	if( bWorldSet == false )
	{
		UpdateHostWorldFlags( gpwstring(L"Regular") );
	}

}

void UIZoneMatch::UpdateHostWorldFlags( gpwstring mapName )
{
	if( IsHosting() )
	{
		//remove those not the one we want, then set the one we want.
		if( mapName.same_no_case(L"Elite") )
		{
			HostUpdateGameFlag( (eZoneGameFlag) (ZONE_GAME_WORLD ^ ZONE_GAME_WORLD_ELITE), false );
			HostUpdateGameFlag( ZONE_GAME_WORLD_ELITE, true );
		}
		else if( mapName.same_no_case(L"Veteran") )
		{
			HostUpdateGameFlag( (eZoneGameFlag) (ZONE_GAME_WORLD ^ ZONE_GAME_WORLD_VETERAN), false );
			HostUpdateGameFlag( ZONE_GAME_WORLD_VETERAN, true );
		}
		else
		{
			HostUpdateGameFlag( (eZoneGameFlag) (ZONE_GAME_WORLD ^ ZONE_GAME_WORLD_REGULAR), false );
			HostUpdateGameFlag( ZONE_GAME_WORLD_REGULAR, true );
		}
	}
}