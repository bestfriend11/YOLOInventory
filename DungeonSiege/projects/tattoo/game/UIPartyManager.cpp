/**************************************************************************

Filename		: UIPartyManager.cpp

Class			: UIPartyManager

Description		: This is a class that handles all the necessary functions
				  needed to maintain the player's party.  This includes:
				  equipment, inventory, interface settings/movement, and
				  user input/interaction with gui elements as it relates
				  to the party.

Author			: Chad Queen

					(C)opyright Gas Powered Games 1999

**************************************************************************/


#include "Precomp_Game.h"

// Include Files
#include "AiAction.h"
#include "AiQuery.h"
#include "AppModule.h"
#include "Enchantment.h"
#include "GameAuditor.h"
#include "Go.h"
#include "GoActor.h"
#include "GoAspect.h"
#include "GoAttack.h"
#include "GoConversation.h"
#include "GoCore.h"
#include "GoData.h"
#include "GoDb.h"
#include "GoDefend.h"
#include "GoDefs.h"
#include "GoInventory.h"
#include "GoMagic.h"
#include "GoMind.h"
#include "GoParty.h"
#include "GoStash.h"
#include "GoStore.h"
#include "GoSupport.h"
#include "nema_aspect.h"
#include "GuiHelper.h"
#include "InputBinder.h"
#include "Job.h"
#include "NamingKey.h"
#include "Player.h"
#include "Server.h"
#include "Siege_Engine.h"
#include "Siege_Mouse_Shadow.h"
#include "Siege_Pos.h"
#include "Siege_Compass.h"
#include "StdHelp.h"
#include "UI_Animation.h"
#include "UI_ChatBox.h"
#include "UI_Dockbar.h"
#include "UI_EditBox.h"
#include "UI_GridBox.h"
#include "UI_InfoSlot.h"
#include "UI_ItemSlot.h"
#include "UI_Messenger.h"
#include "UI_Shell.h"
#include "UI_StatusBar.h"
#include "UI_Tab.h"
#include "UI_TextBox.h"
#include "UI_Window.h"
#include "UICamera.h"
#include "UICharacterDisplay.h"
#include "UICommands.h"
#include "UIDialogueHandler.h"
#include "UIEmoteList.h"
#include "UIFrontEnd.h"
#include "UIGame.h"
#include "UIInventoryManager.h"
#include "UIMenuManager.h"
#include "UIPartyManager.h"
#include "UIPartyMember.h"
#include "UIPlayerRanks.h"
#include "UIStoreManager.h"
#include "UIGameConsole.h"
#include "UIItemOverlay.h"
#include "World.h"
#include "WorldMap.h"
#include "WorldOptions.h"
#include "WorldState.h"
#include "Victory.h"
#include "ContentDb.h"
#include "WorldTerrain.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "gps_manager.h"
#include "FormulaHelper.h"


// Threshold for teleportation (in meters)
const float TELEPORT_THRESHOLD	= 15.0f;


// Constructor
UIPartyManager::UIPartyManager()
	:	m_ScreenParty( GOID_INVALID )
	,	m_ScreenPartySize( 0 )
	,	m_SelectedMember( 0 )
	,	m_bExpertMode( false )
	,	m_bExpertModeActive( false )
	,	m_bInventoryActive( false )
	,	m_bStartupTrack( true )
	,	m_pDialogueHandler( 0 )
	,	m_bDrawStatusBars( false )
	,	m_bMustHaveSelection( true )
	,	m_dwInactiveColor( 0xffc0c0c0 )
	,	m_memberRehire( GOID_INVALID )
	,	m_memberDisband( GOID_INVALID )
	,	m_bMultipleDisband( false )
	,	m_bSingleInventoryVisible( false )
	,	m_bFieldCommands( true )
	,	m_memberCommands( GOID_INVALID )
	,	m_bEquipmentVisible( true )
	,	m_bSelecting( false )
	,	m_dwPosTextColor( 0 )
	,	m_dwNegTextColor( 0 )
	,	m_bCommandsActive( true )	
	,	m_bFormationsActive( true )
	,	m_bInitializingOrders( false )
	,	m_bRolloverCast( false )
	,	m_rollCastObject( GOID_INVALID )
	,	m_rollCastMember( GOID_INVALID )
	,	m_rollCastSpell( GOID_INVALID )
	,	m_bFollowMode( false )
	,	m_bRolloverGet( false )
	,	m_bRolloverAttack( false )
	,   m_bUpdateRespawn( false )
	,	m_fullInventoryHeight( 0 )
	,	m_bDrawMemberLabels( false )
	,	m_bActivateInvSound( false )
	,	m_bGridPlace( true )
	,	m_SpellBookFocus( GOID_INVALID )
	,	m_dataBarVariation( 0 )
	,	m_maxGenericStateDisplay( 0 )
	,	m_RolloverTimer( 0.0f )
{
	// Initialize our party member variables to invalid values
	memset( (void *)m_UIPartyMembers, 0, sizeof (UIPartyMember *) * MAX_PARTY_MEMBERS );
	memset( (void *)m_pWindow, 0, sizeof ( UIWindow *) * STATS_MAX_INDEX );
	memset( (void *)m_pMultiBars, 0, sizeof (UIWindow *) * MB_COUNT );

	gWorld.RegisterRolloverHighlightCallback( makeFunctor( *this, &UIPartyManager::RolloverHighlightCb ) );	
	gWorld.RegisterPartyCallback( makeFunctor( *this, &UIPartyManager::PartyCb ) );
	gWorld.RegisterCastCallback( makeFunctor( *this, &UIPartyManager::CastCb ) );
}



void UIPartyManager::Init()
{
	// Set up an array of all our statistic-related GUI pointers for easy access and mainpulation
	m_pWindow[NAME]						=	gUI.GetGameGUI().FindUIWindow( "character_name", "character" );
	m_pWindow[CLASS]					=	gUI.GetGameGUI().FindUIWindow( "character_class", "character" );
	m_pWindow[MAX_HEALTH]				=	gUI.GetGameGUI().FindUIWindow( "character_text_health_value", "character" );
	m_pWindow[MAX_MANA]					=	gUI.GetGameGUI().FindUIWindow( "character_text_mana_value", "character" );
	m_pWindow[HEALTH_BAR]				=	gUI.GetGameGUI().FindUIWindow( "status_bar_health", "character" );
	m_pWindow[MANA_BAR]					=	gUI.GetGameGUI().FindUIWindow( "status_bar_mana", "character" );

	m_pWindow[GOLD_VALUE]				=	gUI.GetGameGUI().FindUIWindow( "inventory_gold" );
	m_pWindow[PACKMULE_GOLD_VALUE]		=	gUI.GetGameGUI().FindUIWindow( "pack_mule_inventory_gold" );

	m_pWindow[MELEE_DAMAGE_VALUE]		=	gUI.GetGameGUI().FindUIWindow( "text_melee_damage_value", "character" );	
	m_pWindow[RANGED_DAMAGE_VALUE]		=	gUI.GetGameGUI().FindUIWindow( "text_ranged_damage_value", "character" );
	
	m_pWindow[ARMOR_VALUE]				=	gUI.GetGameGUI().FindUIWindow( "character_text_armor_value", "character" );

	m_pWindow[MELEE_EXP_VALUE]			=	gUI.GetGameGUI().FindUIWindow( "character_text_skills_melee_value", "character" );
	m_pWindow[RANGED_EXP_VALUE]			=	gUI.GetGameGUI().FindUIWindow( "character_text_skills_ranged_value", "character" );
	m_pWindow[DARKMAGIC_EXP_VALUE]		=	gUI.GetGameGUI().FindUIWindow( "character_text_skills_dark_magic_value", "character" );
	m_pWindow[GOODMAGIC_EXP_VALUE]		=	gUI.GetGameGUI().FindUIWindow( "character_text_skills_good_magic_value", "character" );
	m_pWindow[MELEE_BAR]				=	gUI.GetGameGUI().FindUIWindow( "character_status_melee_bar", "character" );
	m_pWindow[RANGED_BAR]				=	gUI.GetGameGUI().FindUIWindow( "character_status_ranged_bar", "character" );
	m_pWindow[GOODMAGIC_BAR]			=	gUI.GetGameGUI().FindUIWindow( "character_status_good_magic_bar", "character" );
	m_pWindow[DARKMAGIC_BAR]			=	gUI.GetGameGUI().FindUIWindow( "character_status_dark_magic_bar", "character" );

	m_pWindow[STRENGTH_BAR]				=	gUI.GetGameGUI().FindUIWindow( "character_status_strength_bar", "character" );
	m_pWindow[DEXTERITY_BAR]			=	gUI.GetGameGUI().FindUIWindow( "character_status_dexterity_bar", "character" );
	m_pWindow[INTELLIGENCE_BAR]			=	gUI.GetGameGUI().FindUIWindow( "character_status_intelligence_bar", "character" );

	m_pWindow[INT_VALUE]				=	gUI.GetGameGUI().FindUIWindow( "character_text_intelligence_value", "character" );
	m_pWindow[STRENGTH_VALUE]			=	gUI.GetGameGUI().FindUIWindow( "character_text_strength_value", "character" );
	m_pWindow[DEX_VALUE]				=	gUI.GetGameGUI().FindUIWindow( "character_text_dexterity_value", "character" );

	// This is for advanced infobar
	m_pWindow[TEXT_INFORMATION]			=	gUI.GetGameGUI().FindUIWindow( "text_box_info", "data_bar" );

	m_pWindow[CHAR_RECT]				=	gUI.GetGameGUI().FindUIWindow( "character_paperdoll_listener", "character" );

	m_pWindow[ENEMY_HEALTH_BAR]			=	gUI.GetGameGUI().FindUIWindow( "health_bar_enemy" );	

	// This is for the character portrait selection indicatoris
	m_pWindow[PICTURE_SELECT_1]			=	gUI.GetGameGUI().FindUIWindow( "awp_portrait_selection_1", "character_awp" );
	m_pWindow[PICTURE_SELECT_2]			=	gUI.GetGameGUI().FindUIWindow( "awp_portrait_selection_2", "character_awp" );
	m_pWindow[PICTURE_SELECT_3]			=	gUI.GetGameGUI().FindUIWindow( "awp_portrait_selection_3", "character_awp" );
	m_pWindow[PICTURE_SELECT_4]			=	gUI.GetGameGUI().FindUIWindow( "awp_portrait_selection_4", "character_awp" );
	m_pWindow[PICTURE_SELECT_5]			=	gUI.GetGameGUI().FindUIWindow( "awp_portrait_selection_5", "character_awp" );
	m_pWindow[PICTURE_SELECT_6]			=	gUI.GetGameGUI().FindUIWindow( "awp_portrait_selection_6", "character_awp" );
	m_pWindow[PICTURE_SELECT_7]			=	gUI.GetGameGUI().FindUIWindow( "awp_portrait_selection_7", "character_awp" );
	m_pWindow[PICTURE_SELECT_8]			=	gUI.GetGameGUI().FindUIWindow( "awp_portrait_selection_8", "character_awp" );

	m_pWindow[SELECT_ALL]				=	gUI.GetGameGUI().FindUIWindow( "button_selectall", "field_commands" );
	m_pWindow[POTION_HEALTH]			=	gUI.GetGameGUI().FindUIWindow( "button_health_potions", "data_bar" );
	m_pWindow[POTION_MANA]				=	gUI.GetGameGUI().FindUIWindow( "button_mana_potions", "data_bar" );
	m_pWindow[DATA_DOCKBAR]				=	gUI.GetGameGUI().FindUIWindow( "data_bar", "data_bar" );

	m_pWindow[TRADE_GOLD]				=	gUIShell.FindUIWindow( "inventory_gold_human_1", "multi_inventory" );
	gUIShell.ShowGroup( "multi_inventory_human_1_gold", false, false, "multi_inventory" );

	if ( ::IsMultiPlayer() )
	{
		m_pMultiBars[MB_HEALTH_1]		=	gUIShell.FindUIWindow( "health_bar_2", "status_bars" );
		m_pMultiBars[MB_MANA_1]			=	gUIShell.FindUIWindow( "mana_bar_2", "status_bars" );

		m_pMultiBars[MB_HEALTH_2]		=	gUIShell.FindUIWindow( "health_bar_3", "status_bars" );
		m_pMultiBars[MB_MANA_2]			=	gUIShell.FindUIWindow( "mana_bar_3", "status_bars" );

		m_pMultiBars[MB_HEALTH_3]		=	gUIShell.FindUIWindow( "health_bar_4", "status_bars" );
		m_pMultiBars[MB_MANA_3]			=	gUIShell.FindUIWindow( "mana_bar_4", "status_bars" );

		m_pMultiBars[MB_HEALTH_4]		=	gUIShell.FindUIWindow( "health_bar_5", "status_bars" );
		m_pMultiBars[MB_MANA_4]			=	gUIShell.FindUIWindow( "mana_bar_5", "status_bars" );

		m_pMultiBars[MB_HEALTH_5]		=	gUIShell.FindUIWindow( "health_bar_6", "status_bars" );
		m_pMultiBars[MB_MANA_5]			=	gUIShell.FindUIWindow( "mana_bar_6", "status_bars" );

		m_pMultiBars[MB_HEALTH_6]		=	gUIShell.FindUIWindow( "health_bar_7", "status_bars" );
		m_pMultiBars[MB_MANA_6]			=	gUIShell.FindUIWindow( "mana_bar_7", "status_bars" );

		m_pMultiBars[MB_HEALTH_7]		=	gUIShell.FindUIWindow( "health_bar_8", "status_bars" );
		m_pMultiBars[MB_MANA_7]			=	gUIShell.FindUIWindow( "mana_bar_8", "status_bars" );
	}

	if ( ::IsSinglePlayer() )
	{
		for ( int i = 0; i != MAX_PARTY_MEMBERS; ++i )
		{
			gpstring sWindow;
			sWindow.assignf( "member_%d", i+1 );
			m_UIMemberLabels[i] = (UIText *)gUIShell.FindUIWindow( sWindow, "member_labels" );
			if ( m_UIMemberLabels[i] )
			{
				m_UIMemberLabels[i]->SetAutoSize( true );
			}
		}
	}


	UIWindow * pWindow = gUI.GetGameGUI().FindUIWindow( "gridbox_13x4", "inventory" );
	SetFullInventoryHeight( pWindow->GetRect().bottom - pWindow->GetRect().top - 3 );

	// Init the dialogue handler
	if ( !UIDialogueHandler::DoesSingletonExist() )
	{
		m_pDialogueHandler = new UIDialogueHandler();
	}	

	m_ScreenPartySize = 0;
	m_bStartupTrack = true;
	m_memberCommands = GOID_INVALID;

	InitUserEdTooltips();

	gVictory.RegisterInventoryUpdateCallback( makeFunctor( *this, &UIPartyManager::UpdateQuestCallback ) );

	SetCommandsActive( true );
	SetFormationsActive( true );

	if ( ::IsSinglePlayer() )
	{
		MaximizeFieldCommands();
		InitOrders();
	}
	else
	{
		MinimizeFieldCommands();
		gUIShell.ShowGroup( "mp_chat", true, false, "field_commands" );
	}

	m_bFollowMode = true;
	UICheckbox * pCheck = (UICheckbox *)gUIShell.FindUIWindow( "button_follow" );
	if ( pCheck )
	{
		pCheck->SetState( m_bFollowMode );
	}	

	for ( int iGroup = 0; iGroup != MAX_PARTY_MEMBERS; ++iGroup )
	{
		m_group[iGroup].focusGoid = GOID_INVALID;
		m_group[iGroup].bExplicitlySet = false;
		m_group[iGroup].group.clear();
	}
	
	gUIGame.SetItemLabels( true );

	m_bUpdateRespawn = false;

	UIWindow * pCollectLoot = gUIShell.FindUIWindow( "button_collect_loot", "field_commands" );
	if ( pCollectLoot )
	{	
		if ( ::IsSinglePlayer() )
		{
			pCollectLoot->SetRolloverKey( "collect_loot" );
		}
		else
		{
			pCollectLoot->SetRolloverKey( "collect_loot_mp" );
		}
	}

	m_SpellBookFocus = GOID_INVALID;

	// Reset the guarded collection
	m_GuardedCharacters.clear();
}

// Destructor
UIPartyManager::~UIPartyManager()
{
	DeconstructUIParty();
	Delete( m_pDialogueHandler );
}


void UIPartyManager::Deinit()
{
	memset( (void *)m_pWindow, 0, sizeof ( UIWindow *) * STATS_MAX_INDEX );

	for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i )
	{
		if (( m_UIPartyMembers[i] == NULL ) || ( m_UIPartyMembers[i]->GetGoid() == GOID_INVALID ))
		{
			continue;
		}
		else 
		{
			GoHandle hMember( m_UIPartyMembers[i]->GetGoid() );
			hMember->GetInventory()->SetGridbox( NULL );
	
		}
	}

	if ( UIDialogueHandler::DoesSingletonExist() )
	{
		gUIDialogueHandler.Deinit();
	}
}

void UIPartyManager::Update( float secondsElapsed )
{
	// First, handle any disband requests
	if ( m_disbandRequests.size() != 0 && !gUIShell.IsInterfaceVisible( "backend_dialog" ) )
	{
		DisbandDialog();		
	}

	UpdateRespawn();

	// Rebuild party UI if needed
	Goid ScreenParty = GOID_INVALID;
	unsigned int ScreenPartySize = 0;

	bool bRebuild = false;

	Player * pPlayer = gServer.GetScreenPlayer();
	if( pPlayer == NULL )
	{
		return;
	}

	Go * hScreenParty = pPlayer->GetParty();

	if( hScreenParty )
	{
		ScreenPartySize = hScreenParty->GetChildren().size();

		if( m_ScreenPartySize != ScreenPartySize )
		{
			bRebuild = true;
		}
	}

	if( m_ScreenParty != ScreenParty )
	{
		bRebuild = true;
	}

	if( bRebuild ) 
	{
		DeconstructUIParty();
		ConstructUIParty();

		m_ScreenParty		= ScreenParty;
		m_ScreenPartySize	= ScreenPartySize;
	}

	// Update the onscreen player statistics
	UpdatePlayerStats();

	EnsureSelection();

	m_bActivateInvSound = false;

	if ( GetRolloverTimer() != 0.0f && GetRolloverItem() != GOID_INVALID )
	{
		SetRolloverTimer( GetRolloverTimer() - secondsElapsed );
		if ( GetRolloverTimer() <= 0.0f )
		{
			SetRolloverTimer( 0.0f );
			gUIInventoryManager.DisplayWorldRolloverItemData( GetRolloverItem() );
		}
	}
}


void UIPartyManager::EnsureSelection()
{
	// Make sure at least one party member is selected at all times
	if ( GetMustHaveSelection() )
	{
		if ( !IsPartyMemberSelected() )
		{
			bool bSelectFocus	= false;
			Goid focusGoid		= GOID_INVALID;

			bool bSelected = false;
			
			// Get last selected members - insert currently focused Go at the
			// front if there is one (so it's preferred, GoParty may set it!)
			GoidColl lastSelectedMembers( m_LastSelectedMembers );
			if ( gGoDb.GetFocusGo() != GOID_INVALID )
			{
				lastSelectedMembers.insert( lastSelectedMembers.begin(), gGoDb.GetFocusGo() );
			}

			// First, try to select the last selected member(s)
			GoidColl::iterator i;
			for ( i = lastSelectedMembers.begin(); i != lastSelectedMembers.end(); ++i )
			{				
				GoHandle hPlayer( *i );
				if ( hPlayer.IsValid() && IsPlayerPartyMember( *i ) && 
					(hPlayer->GetAspect()->GetLifeState() == LS_ALIVE_CONSCIOUS) )
				{
					gGoDb.Select( *i );
					bSelected = true;

					if ( hPlayer->GetInventory()->IsPackOnly() && hPlayer->IsFocused() )
					{
						bSelectFocus = true;
					}

					if ( !hPlayer->GetInventory()->IsPackOnly() )
					{
						focusGoid = *i;
					}
				}
			}

			// No matches, so let's try to select one of our conscious party members
			if ( !bSelected )
			{
				for ( int j = 0; j != MAX_PARTY_MEMBERS; ++j )
				{
					if ( m_UIPartyMembers[j] == NULL || m_UIPartyMembers[j]->GetGoid() == GOID_INVALID )
					{
						continue;
					}

					GoHandle hPlayer( m_UIPartyMembers[j]->GetGoid() );
					if ( hPlayer.IsValid() )
					{
						if( hPlayer->IsScreenPartyMember() && 
							hPlayer->GetAspect()->GetLifeState() == LS_ALIVE_CONSCIOUS )
						{
							gGoDb.Select( hPlayer->GetGoid() );
							bSelected = true;

							if ( hPlayer->GetInventory()->IsPackOnly() && hPlayer->IsFocused() )
							{
								bSelectFocus = true;
							}

							if ( !hPlayer->GetInventory()->IsPackOnly() )
							{
								focusGoid = hPlayer->GetGoid();
							}
						}
					}
				}
			}

			if ( bSelectFocus && focusGoid != GOID_INVALID )
			{
				gGoDb.RSSetFocusGo( focusGoid );
			}
						
			
			if ( !bSelected && m_UIPartyMembers[0] && ( m_UIPartyMembers[0]->GetGoid() != GOID_INVALID ) )
			{
				gGoDb.Select( m_UIPartyMembers[0]->GetGoid() );
			}
		}

		GoHandle hMember( gGoDb.GetFocusGo() );
		if ( hMember.IsValid() && hMember->HasInventory() && hMember->GetInventory()->IsPackOnly() )
		{
			GopColl::const_iterator i;
			Go * pNonPacker = 0;
			for ( i = gGoDb.GetSelection().begin(); i != gGoDb.GetSelection().end(); ++i )
			{
				if( (*i)->IsScreenPartyMember() && (*i)->HasInventory() && !(*i)->GetInventory()->IsPackOnly() )
				{
					pNonPacker = *i;
					break;
				}
			}
			
			if ( pNonPacker )
			{
				gGoDb.RSSetFocusGo( pNonPacker->GetGoid() );
			}
		}
	}
}


void UIPartyManager::Draw()
{
	bool bClearZ	= true;
	for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i )
	{
		if( m_UIPartyMembers[i] == NULL )
		{
			continue;
		}

		if ( m_UIPartyMembers[i]->GetGoid() != GOID_INVALID ) {

			if( bClearZ )
			{
				// Clear the Z buffer
				gSiegeEngine.Renderer().GetDevice()->Clear( 0, NULL, D3DCLEAR_ZBUFFER, 0x00000000, 0.0f, 0 );
				D3DRECT char_rect;
				char_rect.x1		= m_pWindow[CHAR_RECT]->GetRect().left;
				char_rect.y1		= m_pWindow[CHAR_RECT]->GetRect().top;
				char_rect.x2		= m_pWindow[CHAR_RECT]->GetRect().right;
				char_rect.y2		= m_pWindow[CHAR_RECT]->GetRect().bottom;

				gSiegeEngine.Renderer().GetDevice()->Clear( 1, &char_rect, D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0 );

				// Set the ClearZ flag to false
				bClearZ	= false;
			}

			if ( m_UIPartyMembers[i]->GetPaperDollVisible() )
			{
				m_UIPartyMembers[i]->RenderPaperDoll();
			}
		}
	}	
}


bool UIPartyManager::SortMemberInventory( int index )
{
	if ( index == 0 && GetSingleInventoryVisible() )
	{
		index = m_SelectedMember;
	}

	if (( m_UIPartyMembers[index] == NULL ) || ( m_UIPartyMembers[index]->GetGoid() == GOID_INVALID ))
	{
		return false;
	}
	else
	{
		GoHandle hMember( m_UIPartyMembers[index]->GetGoid() );
		hMember->GetInventory()->GetGridbox()->AutoSort();
		gUIInventoryManager.CalculateGridHighlights( hMember->GetInventory()->GetGridbox() );
	}	

	return true;
}


bool UIPartyManager::SortInventory()
{
	for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i )
	{
		if (( m_UIPartyMembers[i] == NULL ) || ( m_UIPartyMembers[i]->GetGoid() == GOID_INVALID ))
		{
			continue;
		}
		else if ( m_UIPartyMembers[i]->IsInventoryExpanded() )
		{
			GoHandle hMember( m_UIPartyMembers[i]->GetGoid() );
			if ( hMember->IsSelected() )
			{
				hMember->GetInventory()->GetGridbox()->AutoSort();
				gUIInventoryManager.CalculateGridHighlights( hMember->GetInventory()->GetGridbox() );
			}
		}
	}

	return true;
}


// Constructs the UI information for all the party members
// currently within the party manager
void UIPartyManager::ConstructUIParty()
{
	GopColl partyColl;

	Player * pPlayer = gServer.GetScreenPlayer();
	if( pPlayer == NULL )
	{
		// Disable GUI features for parties less than max
		for ( int j = 0; j < MAX_PARTY_MEMBERS; ++j )
		{
			gpstring sCharacter;			
			sCharacter.assignf( "character_%d", j+1 );
			gUI.GetGameGUI().ShowGroup( sCharacter, false );
			sCharacter.assignf( "character_%d_min", j+1 );
			gUI.GetGameGUI().ShowGroup( sCharacter, false );
			sCharacter.assignf( "character_%d_max", j+1 );
			gUI.GetGameGUI().ShowGroup( sCharacter, false );
		}
		return;
	}

	Go * pParty = pPlayer->GetParty();
	if( pParty )
	{
		partyColl = pParty->GetChildren();
	}

	// Set Defaults
	for ( int o = 0; o < MAX_PARTY_MEMBERS; ++o ) 
	{
		UIPartyMember *default_member = new UIPartyMember( GOID_INVALID );
		m_UIPartyMembers[o] = default_member;
	}

	if ( gWorldState.GetCurrentState() != WS_RELOADING )
	{
		gUIStoreManager.DeactivateStore();
		CloseAllCharacterMenus();
	}

	// Load in the actual party
	GopColl::iterator i;
	int j = 0;
	int numMembers = 0;

	for ( i = partyColl.begin(); i != partyColl.end(); ++i ) 
	{		
		PartyIndexMap::iterator iIndex = m_PartyIndexMap.find( (*i)->GetGoid() );
		if ( iIndex != m_PartyIndexMap.end() )
		{
			j = (*iIndex).second;
		}
		else
		{
			for ( int k = 0; k != MAX_PARTY_MEMBERS; ++k ) 
			{
				if ( m_UIPartyMembers[k] && m_UIPartyMembers[k]->GetGoid() == GOID_INVALID )
				{
					j = k;
					break;
				}
			}
		}

		// Make sure this member is acceptable
		GoHandle pMember( (*i)->GetGoid() );
		if ( !pMember.IsValid() )
		{
			gpassertm( 0, "Hmmmm, trying to add an invalid object to your party, this is not good." );
		}
		if ( pMember->HasActor() == false )
		{
			continue;
		}

		AssignDefaultGroupKey( j, pMember->GetGoid() );
		
		// Whoah!  Too many party members
		if ( j >= MAX_PARTY_MEMBERS )
		{
			break;
		}
		// We already have this guy loaded and ready for action
		if ( m_UIPartyMembers[j]->GetGoid() == (*i)->GetGoid() )
		{
			continue;
		}
		else if ( m_UIPartyMembers[j] )
		{
			// There's old stuff here, let's toss it
			delete m_UIPartyMembers[j];
			m_UIPartyMembers[j] = NULL;
		}

		// Set default values for the member
		UIPartyMember *new_member = new UIPartyMember( (*i)->GetGoid() );
		new_member->SetCharacterExpanded( true );
		new_member->SetInventoryExpanded( true );
		
		if ( pMember->GetInventory()->GetActiveSpellBook() || pMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
		{
			if ( !pMember->GetInventory()->GetActiveSpellBook() )
			{
				pMember->GetInventory()->SetActiveSpellBook( pMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) );
			}
			new_member->SetSpellsExpanded( true );
		}
		else
		{
			new_member->SetSpellsExpanded( false );
		}
		
		new_member->SetGoodMagicSelected( true );
		m_UIPartyMembers[j] = new_member;

		if ( m_bStartupTrack && gWorldState.GetPreviousState() != WS_LOADING_SAVE_GAME && gWorldState.GetCurrentState() != WS_LOADING_SAVE_GAME ) 
		{
			gGoDb.Select( pMember->GetGoid() );
			gUICamera.SetCameraMode( CMODE_TRACK );
			m_bStartupTrack = false;
		}

		// Set the appropriate gridbox for this character

		bool bGridExists = false;
		if ( pMember->GetInventory()->HasGridbox() )
		{
			bGridExists = true;
		}

		UIGridbox * pGridbox = GetGridboxFromGO( pMember, j );

		if (!pGridbox) 
		{
			gpgeneric( "Party member does not have a gridbox! This is either something bad, or a possessed creature.\n" );
			Delete( new_member );
			m_UIPartyMembers[j] = new UIPartyMember( GOID_INVALID );
			j--;
			continue;
		}

		GoInventory *pInventory = pMember->GetInventory();
		if( pInventory == NULL ) 
		{
			continue;
		}

		gpassertf(	pGridbox, ("Invalid gridbox inventory size: %dx%d for actor: %s",
					pInventory->GetGridWidth(),
					pInventory->GetGridHeight(),
					pMember->GetCommon()->GetScreenName().c_str()) );

		pInventory->SetGridbox( pGridbox );
		pGridbox->SetVisible( false );
		gUI.GetGameGUI().InitializePulloutInterfaceToWindow( "inventory", "stats_main", X_ALIGN_RIGHT );

		if ( !bGridExists )
		{
			pInventory->ProcessGridboxRequests();

			GopColl items;
			GopColl::iterator iItem;

			if ( pInventory->ListItems( IL_MAIN, items ) )
			{
				for ( iItem = items.begin(); iItem != items.end(); ++iItem )
				{
					if ( !pGridbox->ContainsID( MakeInt( (*iItem)->GetGoid() ) ) && !(*iItem)->HasGold() && !pInventory->IsEquipped( *iItem ) )
					{
						pInventory->GetGridbox()->AutoItemPlacement( pInventory->BuildUiName( *iItem ) );					
					}
				}
			}		
		}

		UIWindow * pRollover = 0;

		gpstring sCharacter;		
		sCharacter.assignf( "awp_itemslot_portrait_%d", j+1 ); 		
		{
			UIItemSlot	*pWindow	= (UIItemSlot *)gUI.GetGameGUI().FindUIWindow( sCharacter );
			UIItem		*pItem		= GetItemFromGO( pMember, true, true );
			if ( pWindow && pItem ) 
			{
				pItem->SetItemID( MakeInt(pMember->GetGoid()) );
				pWindow->PlaceItem( pItem, false );
				pWindow->SetAcceptInput( false );				
				pWindow->SetUVRect( 0.0f, 0.609375f, 0.28125f, 1.0f );
			}
		}

		if ( !(*i)->GetInventory()->IsPackOnly() )
		{
			sCharacter.assignf( "multi_portrait_human_%d", j+1 );
		}
		else
		{
			sCharacter.assignf( "multi_portrait_packmule_%d", j+1 );
		}

		{
			UIItemSlot	*pWindow	= (UIItemSlot *)gUI.GetGameGUI().FindUIWindow( sCharacter );
			UIItem		*pItem		= GetItemFromGO( pMember, true, true );
			if ( pWindow && pItem ) 
			{
				pItem->SetItemID( MakeInt(pMember->GetGoid()) );
				pWindow->PlaceItem( pItem, false );
				pWindow->SetAcceptInput( false );				
				pWindow->SetUVRect( 0.0f, 0.609375f, 0.28125f, 1.0f );
			}
		}


		sCharacter.assignf( "awp_status_health_%d", j+1 );
		{
			UIStatusBar *pHealth	= (UIStatusBar *)gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pHealth, GUI_HEALTH );
		}
		sCharacter.assignf( "awp_status_mana_%d", j+1 );
		{
			UIStatusBar *pMana		= (UIStatusBar *)gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pMana, GUI_MANA );
		}
		sCharacter.assignf( "awp_itemslot_portrait_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_PORTRAIT );
		}	

		sCharacter.assignf( "awp_death_portrait_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_DEATH );
		}		
		sCharacter.assignf( "ghost_timer_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_GHOST_TIMER );
		}		
		sCharacter.assignf( "awp_radio_button_character_%d_slot_1", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_AIS_MELEE_WEAPON );
		}
		sCharacter.assignf( "awp_radio_button_character_%d_slot_2", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_AIS_RANGED_WEAPON );
		}
		sCharacter.assignf( "awp_radio_button_character_%d_slot_3", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_AIS_PRIMARY_SPELL );
		}
		sCharacter.assignf( "awp_radio_button_character_%d_slot_4", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_AIS_SECONDARY_SPELL );
		}
		sCharacter.assignf( "awp_radio_button_character_%d_slot_active", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_ACTIVE_MINIMIZED );
		}						
		sCharacter.assignf( "fuel_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_FUEL_BAR );
		}
		sCharacter.assignf( "fuel_min_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_MIN_FUEL_BAR );
		}
		sCharacter.assignf( "health_bar_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_HEALTH_FLOAT );
		}
		sCharacter.assignf( "mana_bar_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_MANA_FLOAT );
		}
		sCharacter.assignf( "window_expert_panel_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_EXPERT_PANEL );
		}		
		sCharacter.assignf( "bar_slot_1_skill_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_SKILL_BAR_1 );
		}
		sCharacter.assignf( "bar_slot_2_skill_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_SKILL_BAR_2 );
		}
		sCharacter.assignf( "bar_slot_3_skill_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_SKILL_BAR_3 );
		}
		sCharacter.assignf( "bar_slot_4_skill_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_SKILL_BAR_4 );
		}
		sCharacter.assignf( "bar_slot_active_skill_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_SKILL_BAR_ACTIVE );
		}
		sCharacter.assignf( "awp_portrait_rollover_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_PORTRAIT_ROLLOVER );
			pWindow->SetVisible( false );
			pRollover = pWindow;
		}
		sCharacter.assignf( "window_pack_panel_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_PACK_PANEL_MAX );
			pWindow->SetVisible( false );									
		}
		sCharacter.assignf( "window_pack_panel_min_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_PACK_PANEL_MIN );
			pWindow->SetVisible( false );									
		}

		UIWindow * pWarning = 0;
		UIWindow * pUnconscious = 0;
		UIWindow * pGuarded = 0;
		sCharacter.assignf( "awp_portrait_health_warning_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_HEALTH_WARNING );
			pWindow->SetVisible( false );
			pWarning = pWindow;
		}
		sCharacter.assignf( "awp_portrait_unconscious_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_UNCONSCIOUS );
			pWindow->SetVisible( false );
			pUnconscious = pWindow;
		}	
		sCharacter.assignf( "awp_guard_%d", j+1 );
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_GUARDED );
			pWindow->SetVisible( false );
			pGuarded = pWindow;
		}

		if ( !(*i)->GetInventory()->IsPackOnly() )
		{
			sCharacter.assignf( "gold_icon_human_%d", j+1 );
		}
		else
		{
			sCharacter.assignf( "gold_icon_packmule_%d", j+1 );
		}

		// multi-inventory gold
		{
			UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );
			if ( pWindow )
			{
				m_UIPartyMembers[j]->SetGUIWindow( pWindow, GUI_MULTI_GOLD );
				pWindow->SetVisible( false );
				pRollover = pWindow;
			}
		}


		sCharacter.assignf( "character_%d", j+1 );
		gUI.GetGameGUI().ShowGroup( sCharacter, true );
		sCharacter.assignf( "character_%d_max", j+1 );
		gUI.GetGameGUI().ShowGroup( sCharacter, true );
		
		if ( (*i)->GetInventory()->IsPackOnly() )
		{
			UIWindow * pWindow = m_UIPartyMembers[j]->GetGUIWindow( GUI_PACK_PANEL_MAX );
			if ( pWindow )
			{
				pWindow->SetVisible( true );
			}			
		}
		else
		{
			UIWindow * pWindow = m_UIPartyMembers[j]->GetGUIWindow( GUI_PACK_PANEL_MAX );
			if ( pWindow )
			{
				pWindow->SetVisible( false );
			}
		}

		if ( pRollover )
		{
			pRollover->SetVisible( false );
		}
		if ( pWarning )
		{
			pWarning->SetVisible( false );
		}
		if ( pUnconscious )
		{
			pUnconscious->SetVisible( false );
		}
		if ( pGuarded )
		{
			pGuarded->SetVisible( false );
		}
		gUIShell.GetAnimation()->UpdateFlashAnimation( 0.0f );

		pInventory->SetInventoryDirty( true );
		gRules.UpdateClassDesignation( (*i)->GetGoid() );

		numMembers++;

		gpstring sMember;
		sMember.assignf( "was_party_member_0x%x", (*i)->GetGoid() );
		gGameAuditor.SetBool( sMember, true );
				
		// Calculate height
		matrix_3x3 orient;
		vector_3 center, halfDiag;
		(*i)->GetAspect()->GetNodeSpaceOrientedBoundingVolume( orient, center, halfDiag );
		halfDiag = orient * halfDiag;
		float heightOffset = (center.y - (*i)->GetPlacement()->GetPosition().pos.y) + halfDiag.y + 0.2f;
		m_UIPartyMembers[j]->SetHeight( heightOffset );
	}
	

	// Init Portraits / Key bindings
	for ( o = 0; o < MAX_PARTY_MEMBERS; ++o )
	{
		if ( m_UIPartyMembers[o] && ( m_UIPartyMembers[o]->GetGoid() != GOID_INVALID ))
		{
			GoHandle hCharacter( m_UIPartyMembers[o]->GetGoid() );
			if( hCharacter.IsValid() )
			{
				gpstring sName;
				sName.assignf( "inventory_portrait_%d", o+1 );
				UIItemSlot *pMultiSlot = (UIItemSlot *)gUI.GetGameGUI().FindUIWindow( sName );
				UIItem	*pMultiItem		= GetItemFromGO( hCharacter, true, true );
				if ( pMultiSlot && pMultiItem )
				{
					pMultiItem->SetItemID( MakeInt(hCharacter->GetGoid()) );
					pMultiSlot->PlaceItem( pMultiItem, false );
					pMultiSlot->SetAcceptInput( false );
				}

				gUIInventoryManager.InitWeaponsConfig( o );
			}
		}			
	}

	// Disable GUI features for parties less than max
	{
		for ( int iMember = 0; iMember < MAX_PARTY_MEMBERS; ++iMember )
		{
			if ( m_UIPartyMembers[iMember]->GetGoid() == GOID_INVALID )
			{
				gpstring sCharacter;			
				sCharacter.assignf( "character_%d", iMember+1 );
				gUI.GetGameGUI().ShowGroup( sCharacter.c_str(), false );
				sCharacter.assignf( "character_%d_max", iMember+1 );
				gUI.GetGameGUI().ShowGroup( sCharacter.c_str(), false );
				sCharacter.assignf( "character_%d_min", iMember+1 );
				gUI.GetGameGUI().ShowGroup( sCharacter.c_str(), false );

				sCharacter.assignf( "health_bar_%d", iMember+1 );
				{
					UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter.c_str() );
					if ( pWindow )
					{
						pWindow->SetVisible( false );
					}
				}
				sCharacter.assignf( "mana_bar_%d", iMember+1 );			
				{
					UIWindow *pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter.c_str() );
					m_UIPartyMembers[iMember]->SetGUIWindow( pWindow, GUI_MANA_FLOAT );
					if ( pWindow )
					{
						pWindow->SetVisible( false );
					}
				}	
				
				sCharacter.assignf( "awp_itemslot_portrait_%d", iMember+1 ); 		
				{
					UIItemSlot	* pWindow	= (UIItemSlot *)gUI.GetGameGUI().FindUIWindow( sCharacter );					
					if ( pWindow ) 
					{
						UIItem * pItem = gUIShell.GetItem( pWindow->GetItemID() );
						if ( pItem )
						{
							pWindow->ClearItem();
							pItem->RemoveItemParent( pWindow );
						}
					}
				}

				sCharacter.assignf( "multi_portrait_human_%d", iMember+1 );
				{
					UIItemSlot	* pWindow	= (UIItemSlot *)gUI.GetGameGUI().FindUIWindow( sCharacter );					
					if ( pWindow ) 
					{
						UIItem * pItem = gUIShell.GetItem( pWindow->GetItemID() );
						if ( pItem )
						{
							pWindow->ClearItem();
							pItem->RemoveItemParent( pWindow );
						}
					}
				}

				sCharacter.assignf( "multi_portrait_packmule_%d", iMember+1 );			
				{
					UIItemSlot	* pWindow	= (UIItemSlot *)gUI.GetGameGUI().FindUIWindow( sCharacter );					
					if ( pWindow ) 
					{
						UIItem * pItem = gUIShell.GetItem( pWindow->GetItemID() );
						if ( pItem )
						{
							pWindow->ClearItem();
							pItem->RemoveItemParent( pWindow );
						}
					}
				}					
			}	
		}
	}

	// Set Expert Mode on or off
	ExpertMode();
	AdjustAwpToggleLocation();

	SelectValidPortraits( gGoDb.GetSelection() );

	if ( gWorldState.GetCurrentState() != WS_RELOADING )
	{
		InitOrders();
	}

	// Init Gold	
	if ( m_UIPartyMembers[0] )
	{
		GoHandle hMember( m_UIPartyMembers[0]->GetGoid() );
		if ( hMember.IsValid() )
		{
			hMember->GetInventory()->SetGoldDirty();
		}
	}	

	UIButton * pDisband = (UIButton *)gUI.GetGameGUI().FindUIWindow( "button_disband", "field_commands" );
	if ( pDisband )
	{
		if ( numMembers == 1 )
		{		
			pDisband->DisableButton();		
		}
		else
		{	
			pDisband->EnableButton();		
		}
	}

	// Syncronize pack members of the party
	pParty = pPlayer->GetParty();
	if( pParty )
	{
		i = pParty->GetChildBegin();

		gRules.SetPackMemberStats( (*i)->GetGoid() );
	}

	// Rebuild Party Index
	{
		m_PartyIndexMap.clear();
		for ( int iMember = 0; iMember < MAX_PARTY_MEMBERS; ++iMember )
		{
			if ( GoDb::DoesSingletonExist() )
			{
				if ( m_UIPartyMembers[iMember] && m_UIPartyMembers[iMember]->GetGoid() != GOID_INVALID )
				{				
					GoHandle hMember( m_UIPartyMembers[iMember]->GetGoid() );
					if ( hMember && hMember->IsScreenPartyMember() )
					{
						m_PartyIndexMap.insert( std::make_pair( m_UIPartyMembers[iMember]->GetGoid(), iMember ) );			
					}
				}
			}		
		}	
	}

	// Make sure the stash is loaded up as well
	if ( pPlayer )
	{
		GoHandle hStash( pPlayer->GetStash() );
		if ( hStash )
		{
			hStash->GetStash()->Init();
			hStash->GetInventory()->ProcessGridboxRequests();
		}
	}
}


bool UIPartyManager::ExpertMode()
{
	gpstring sCharacter;
	gUIShell.SetRolloverName( "" );
	if ( IsExpertModeEnabled() == false )
	{
		UIWindow * pMax = gUIShell.FindUIWindow( "button_awp_maximize", "character_awp" );
		if ( pMax )
		{
			pMax->SetVisible( false );
		}
		UIWindow * pMin = gUIShell.FindUIWindow( "button_awp_minimize", "character_awp" );
		if ( pMin )
		{
			pMin->SetVisible( true );
		}

		for ( int j = 0; j < MAX_PARTY_MEMBERS; ++j )
		{
			if ( m_UIPartyMembers[j] && m_UIPartyMembers[j]->GetGoid() != GOID_INVALID )
			{
				sCharacter.assignf( "character_%d_max", j+1 );
				gUI.GetGameGUI().ShowGroup( sCharacter, true );

				sCharacter.assignf( "character_%d_min", j+1 );
				gUI.GetGameGUI().ShowGroup( sCharacter, false );						

				GoHandle hGo( m_UIPartyMembers[j]->GetGoid() );
				if ( hGo.IsValid() )
				{
					if ( hGo->GetInventory()->IsPackOnly() )
					{
						UIWindow * pWindow = m_UIPartyMembers[j]->GetGUIWindow( GUI_PACK_PANEL_MAX );
						if ( pWindow )
						{
							pWindow->SetVisible( true );
						}
					}
					else
					{
						UIWindow * pWindow = m_UIPartyMembers[j]->GetGUIWindow( GUI_PACK_PANEL_MAX );
						if ( pWindow )
						{
							pWindow->SetVisible( false );
						}
					}

					hGo->GetInventory()->SetInventoryDirty( true );
				}
			}
		}
	}
	else
	{
		UIWindow * pMax = gUIShell.FindUIWindow( "button_awp_maximize", "character_awp" );
		if ( pMax )
		{
			pMax->SetVisible( true );
		}
		UIWindow * pMin = gUIShell.FindUIWindow( "button_awp_minimize", "character_awp" );
		if ( pMin )
		{
			pMin->SetVisible( false );
		}

		for ( int j = 0; j < MAX_PARTY_MEMBERS; ++j )
		{
			if ( m_UIPartyMembers[j] && m_UIPartyMembers[j]->GetGoid() != GOID_INVALID )
			{
				sCharacter.assignf( "character_%d_max", j+1 );
				gUI.GetGameGUI().ShowGroup( sCharacter, false );

				sCharacter.assignf( "character_%d_min", j+1 );
				gUI.GetGameGUI().ShowGroup( sCharacter, true );						

				GoHandle hGo( m_UIPartyMembers[j]->GetGoid() );
				if ( hGo.IsValid() )
				{
					if ( hGo->GetInventory()->IsPackOnly() )
					{
						UIWindow * pWindow = m_UIPartyMembers[j]->GetGUIWindow( GUI_PACK_PANEL_MIN );
						if ( pWindow )
						{
							pWindow->SetVisible( true );
						}
					}
					else
					{
						UIWindow * pWindow = m_UIPartyMembers[j]->GetGUIWindow( GUI_PACK_PANEL_MIN );
						if ( pWindow )
						{
							pWindow->SetVisible( false );
						}
					}

					hGo->GetInventory()->SetInventoryDirty( true );
				}
			}
		}
	}

	return true;
}


bool UIPartyManager::ActivateExpertMode()
{
	if ( !gUIMenuManager.IsOptionsMenuActive() )
	{
		gSoundManager.PlaySample( "s_e_gui_element_small" );
		SetExpertMode( !GetExpertMode() );
		ExpertMode();
	}

	return true;
}


// Deconstructs the UI information for all the party members
// currently within the party manager
void UIPartyManager::DeconstructUIParty()
{
	m_PartyIndexMap.clear();
	for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i )
	{
		if ( GoDb::DoesSingletonExist() )
		{
			if ( m_UIPartyMembers[i] && m_UIPartyMembers[i]->GetGoid() != GOID_INVALID )
			{				
				GoHandle hMember( m_UIPartyMembers[i]->GetGoid() );
				if ( hMember && hMember->IsScreenPartyMember() )
				{
					m_PartyIndexMap.insert( std::make_pair( m_UIPartyMembers[i]->GetGoid(), i ) );			
				}
			}
		}

		Delete( m_UIPartyMembers[i] );
	}
}



// Display a single characters information and UI preferences
void UIPartyManager::ConstructCharacterInterface( int ID, bool bDeselectAll, bool bStore )
{
	if ( m_bSelecting )
	{
		return;
	}

	if ( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return;
	}

	gUIGameConsole.EnableConsoleUpdate( true );

	gUIInventoryManager.SetGUIRolloverGOID( GOID_INVALID );
	gUIShell.SetRolloverItemslotID( 0 );
	if ( m_UIPartyMembers[ID] == 0 )
	{
		return;
	}	

	// Handle store stuff first
	GoHandle hStore( gUIStoreManager.GetActiveStore() );
	if ( hStore.IsValid() )
	{
		GoHandle hActor( m_UIPartyMembers[ID]->GetGoid() );
		if ( !gAIQuery.IsInRange(	hStore->GetPlacement()->GetPosition(), hActor->GetPlacement()->GetPosition(),
									hStore->GetStore()->GetActivateRange() ) )				
		{
			gUIStoreManager.DeactivateStore();			
			return;
		}
		else if ( gUIStoreManager.GetStoreActive() && hStore->GetStore()->IsItemStore() && !gUIShell.IsInterfaceVisible( "store" ) )
		{					
			gUIStoreManager.ActivateStore( hStore, hActor );
			return;
		}
		else if ( gUIInventoryManager.GetStashActive() )
		{
			gUIInventoryManager.SwitchMemberStash( hActor->GetParent()->GetGoid() );
		}
		else
		{
			gUIStoreManager.DeactivateStore();
		}		
	}
	
	gpstring sBar;
	for ( int i = 1; i <= MAX_PARTY_MEMBERS; ++i )
	{
		sBar.assignf( "inventory_bar_%d", i );
		gUI.GetGameGUI().ShowGroup( sBar.c_str(), false );
		if ( m_UIPartyMembers[i-1] && ( m_UIPartyMembers[i-1]->GetGoid() != GOID_INVALID ))
		{
			m_UIPartyMembers[i-1]->SetPaperDollVisible( false );
		}
	}

	GoHandle hMember( m_UIPartyMembers[ID]->GetGoid() );
	if ( !hMember.IsValid() )
	{
		// This should never happen, let's error
		gperror( "The party member that we're opening the character panel for is invalid, please tell Chad." );
		return;
	}

	hMember->PlayVoiceSound( "gui_inventory_open", false );

	if ( ( m_SelectedMember == ID ) && ( gUI.GetGameGUI().IsInterfaceVisible( "character" ) ) )
	{
		gUIShell.HideInterface( "character" );
		gUIShell.HideInterface( "spell" );		
		gUIShell.ShowGroup( "human_inventory", false );
		gUIShell.ShowGroup( "pack_mule_inventory", false );		
		m_UIPartyMembers[m_SelectedMember]->SetPaperDollVisible( false );
		SetSingleInventoryVisible( false );
		gUICamera.SetXScreenTrack( 0 );
		hMember->GetInventory()->GetGridbox()->SetVisible( false );

		gSiegeEngine.GetCompass().SetVisible( true );
		gUIShell.ShowInterface( "compass_hotpoints" );	

		if ( !hMember->GetInventory()->IsPackOnly() )
		{			
			SetExpertModeActive( false );
			return;
		}
	}
	if ( m_SelectedMember == ID && hMember.IsValid() && hMember->GetInventory()->IsPackOnly() &&
		 hMember->GetInventory()->GetGridbox()->GetVisible() && GetSingleInventoryVisible() )
	{
		SetExpertModeActive( false );
		SetSingleInventoryVisible( false );
		gUIShell.ShowGroup( "human_inventory", false );
		gUIShell.ShowGroup( "pack_mule_inventory", false );		
		m_UIPartyMembers[m_SelectedMember]->SetPaperDollVisible( false );		
		hMember->GetInventory()->GetGridbox()->SetVisible( false );
		return;
	}

	m_SelectedMember = ID;

	if ( hMember->GetLifeState() != LS_ALIVE_CONSCIOUS )
	{
		if ( hMember->GetLifeState() == LS_GHOST )
		{
			gpscreen( $MSG$ "You cannot open your inventory when you are a ghost." );
		}
		return;
	}

	SetInventoriesVisible( false );	
	CloseAllCharacterMenus();

	gUIGameConsole.EnableConsoleUpdate( false );

	// Disable any other open gridboxes bars; if applicable
	for ( int j = 0; j < MAX_PARTY_MEMBERS; ++j )
	{
		if (( m_UIPartyMembers[j] ) && ( m_UIPartyMembers[j]->GetGoid() != GOID_INVALID ))
		{
			GoHandle hUnused( m_UIPartyMembers[j]->GetGoid() );
			if ( hUnused.IsValid() )
			{
				hUnused->GetInventory()->GetGridbox()->SetVisible( false );
			}
		}
	}
	
	gpstring sSpecies;
	if ( hMember->GetInventory()->IsPackOnly() )
	{
		sSpecies = "packmule";
	}
	else
	{
		sSpecies = "human";
	}

	sBar.assignf( "dialog_box_inv_bg_%s_%d", sSpecies.c_str(), ID+1 );
	UIWindow *pWindow = gUI.GetGameGUI().FindUIWindow( sBar );
	if ( pWindow )
	{
		if ( pWindow->GetScale() == 0.5f )
		{
			gpstring sName;
			sName.assignf( "multi_inventory_%s_%d", sSpecies.c_str(), ID+1 );
			gUI.GetGameGUI().SetGroupScaleToAlignWithWindow( sName, sBar, 2.0 );
			gUI.GetGameGUI().SetGroupScaleToAlignWithWindow( sName, sBar, 1.0 );
		}
	}

	SetExpertModeActive( true );
	if (( m_UIPartyMembers[ID]->IsCharacterExpanded() == true ) && ( !hMember->GetInventory()->IsPackOnly() ))
	{
		SetCharacterActive( true );		
	}
	else {
		SetCharacterActive( false );		
	}

	// Display the windows the user last had visible for this character:
	if ( m_UIPartyMembers[ID]->IsInventoryExpanded() == true	)
	{
		SetInventoryActive( true, bStore );
	}
	else {
		SetInventoryActive( false );
	}

	ShowEquipment();

	if (( m_UIPartyMembers[ID]->IsSpellsExpanded() == true ) && ( !hMember->GetInventory()->IsPackOnly() ) &&
		  (hMember->GetInventory()->GetActiveSpellBook() || hMember->GetInventory()->GetEquipped( ES_SPELLBOOK )) )
	{
		SetSpellsActive( true );
	}
	else
	{
		SetSpellsActive( false );
	}

	if ( hMember.IsValid() )
	{	
		if ( bDeselectAll )
		{
			gGoDb.DeselectAll();
		}

		m_bSelecting = true;
		hMember->Select();
		
		if ( !hMember->GetInventory()->IsPackOnly() )
		{
			m_UIPartyMembers[m_SelectedMember]->SetPaperDollVisible( true );
		}
	}

	if ( gUIShell.IsInterfaceVisible( "character" ) && gUIShell.IsInterfaceVisible( "inventory" ) )
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( "dialog_box_inv_bg" );
		int x = gAppModule.GetGameWidth() -((gAppModule.GetGameWidth() - pWindow->GetRect().right)/2);
		float normalX = ((float)x - ( (float)gAppModule.GetGameWidth()/2.0f))/((float)gAppModule.GetGameWidth()/2.0f);
		gUICamera.SetXScreenTrack( normalX );
	}
	else
	{
		gUICamera.SetXScreenTrack( 0 );
	}

	if ( gUIShell.IsInterfaceVisible( "character" ) && gUIShell.IsInterfaceVisible( "inventory" ) &&
		 gUIShell.IsInterfaceVisible( "spell" ))
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( "dialog_box_spellbook_header" );
		int x = gAppModule.GetGameWidth() - ((gAppModule.GetGameWidth() - pWindow->GetRect().right)/2);
		float normalX = ((float)x - ( (float)gAppModule.GetGameWidth()/2.0f))/((float)gAppModule.GetGameWidth()/2.0f);
		gUICamera.SetXScreenTrack( normalX );
	}

	m_bSelecting = false;
}



void UIPartyManager::SetCharacterActive ( bool bActive )
{
	if ( bActive == true ) 
	{
		GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid() );
		m_UIPartyMembers[m_SelectedMember]->SetCharacterExpanded( true );
		gUI.GetGameGUI().ShowInterface( "character" );		

		gUIInventoryManager.UpdateActiveEquipment( m_SelectedMember );
		hMember->GetInventory()->SetInventoryDirty( true );
		hMember->GetInventory()->GetGridbox()->SetScale( 1.0f );
	}
	else 
	{
		GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid() );
		m_UIPartyMembers[m_SelectedMember]->SetCharacterExpanded( false );
		gUI.GetGameGUI().HideInterface( "character" );		
	}
}


void UIPartyManager::SetSelectedCharacterActive()
{
	SetCharacterActive( !m_UIPartyMembers[m_SelectedMember]->IsCharacterExpanded() );
}

Goid UIPartyManager::GetSelectedCharacter() 
{ 
	if ( m_UIPartyMembers[m_SelectedMember] && m_UIPartyMembers[m_SelectedMember]->GetGoid() != GOID_INVALID )
	{
		return m_UIPartyMembers[m_SelectedMember]->GetGoid(); 
	}
	
	return GOID_INVALID;
} 

// Sets the inventory active state
void UIPartyManager::SetInventoryActive( bool bActive, bool bStore )
{
	SetSingleInventoryVisible( bActive );
	if ( bActive == true )
	{		
		GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid() );
		hMember->GetInventory()->GetGridbox()->SetVisible( true );

		if ( bStore && hMember->GetInventory()->IsPackOnly() )
		{			
			gpstring sName;
			sName.assignf( "multi_inventory_packmule_%d", m_SelectedMember+1 );			
			gUIShell.ShowGroup( sName, true );
			gpstring sBar;
			sBar.assignf( "dialog_box_inv_bg_packmule_%d", m_SelectedMember+1 );			

			UIWindow * pWindow = gUIShell.FindUIWindow( "window_slots_panel_1", "character_awp" );		
			gpstring sBg;
			sBg.assignf( "dialog_box_inv_bg_packmule_%d", m_SelectedMember+1 );
			UIWindow * pDialog = gUIShell.FindUIWindow( sBg, "multi_inventory" );
			if ( pDialog )
			{
				gUIShell.ShiftGroup( "multi_inventory", sName, (pWindow->GetRect().left-pDialog->GetRect().left), -pDialog->GetRect().top );
			}	
			
			UIWindow * pStoreDialog = gUIShell.FindUIWindow( "dialog_box_store_frame", "store" );
			if ( pDialog->GetRect().right != pStoreDialog->GetRect().left )
			{
				gUIShell.ShiftInterface( "store", -(pStoreDialog->GetRect().left-pDialog->GetRect().right), 0 );
			}

			gpstring sGridPlace;
			sGridPlace.assignf( "grid_place_packmule_%d", m_SelectedMember+1 );
			UIWindow * pGridPlace = gUIShell.FindUIWindow( sGridPlace, "multi_inventory" );
			hMember->GetInventory()->GetGridbox()->SetRect(	pGridPlace->GetRect().left, pGridPlace->GetRect().right,
															pGridPlace->GetRect().top, pGridPlace->GetRect().bottom );				

			m_UIPartyMembers[m_SelectedMember]->GetGUIWindow( GUI_MULTI_GOLD )->SetVisible( false );
		}
		else
		{
			int height = hMember->GetInventory()->GetGridbox()->GetHeight();
			if ( height < GetFullInventoryHeight() ) 
			{
				hMember->GetInventory()->GetGridbox()->SetScale( 1.0 );
			}		
		}

		m_UIPartyMembers[m_SelectedMember]->SetInventoryExpanded( true );
				
		if ( hMember->GetInventory()->IsPackOnly() )
		{	
			if ( !bStore )
			{
				gUIShell.ShowGroup( "pack_mule_inventory", true );
				hMember->GetInventory()->GetGridbox()->SetVisible( true );
				UIWindow * pGrid = gUIShell.FindUIWindow( "gridbox_13x12", "inventory" );
				hMember->GetInventory()->GetGridbox()->SetRect( pGrid->GetRect().left,
																pGrid->GetRect().right,
																pGrid->GetRect().top,
																pGrid->GetRect().bottom );
			}		
		}
		else
		{
			gUIShell.ShowGroup( "human_inventory", true );
			hMember->GetInventory()->GetGridbox()->SetVisible( true );
			UIWindow * pGrid = gUIShell.FindUIWindow( "gridbox_13x4", "inventory" );
			hMember->GetInventory()->GetGridbox()->SetRect( pGrid->GetRect().left,
															pGrid->GetRect().right,
															pGrid->GetRect().top,
															pGrid->GetRect().bottom );
		}
	}
	else
	{
		GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid() );
		hMember->GetInventory()->GetGridbox()->SetVisible( false );
		m_UIPartyMembers[m_SelectedMember]->SetInventoryExpanded( true );
		gUIInventoryManager.SetGUIRolloverGOID( GOID_INVALID );
		gUIShell.ShowGroup( "pack_mule_inventory", false );
		gUIShell.ShowGroup( "human_inventory", false );
		hMember->GetInventory()->GetGridbox()->SetVisible( false );
	}
}


void UIPartyManager::SetSelectedInventoryActive()
{
	SetInventoryActive( !m_UIPartyMembers[m_SelectedMember]->IsInventoryExpanded() );
}



// Sets the spell screen active state
bool UIPartyManager::SetSpellsActive( bool bActive )
{
	if ( bActive == true ) 
	{
		gSiegeEngine.GetCompass().SetVisible( false );
		gUIShell.HideInterface( "compass_hotpoints" );

		if ( gUIInventoryManager.GetStashActive() )
		{
			return false;
		}

		if ( gUIStoreManager.GetStoreActive() )
		{
			UIWindow * pSpellDialog = gUIShell.FindUIWindow( "dialog_box_spells_main", "spell" );
			UIWindow * pStoreDialog = gUIShell.FindUIWindow( "dialog_box_store_frame", "store" );
			if ( pSpellDialog->GetRect().right != pStoreDialog->GetRect().left )
			{
				gUIShell.ShiftInterface( "store", -(pStoreDialog->GetRect().left-pSpellDialog->GetRect().right), 0 );
			}
		}

		GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid() );		
		if ( hMember->GetLifeState() != LS_ALIVE_CONSCIOUS )
		{
			return false;
		}

		if ( !m_UIPartyMembers[m_SelectedMember]->IsSpellsExpanded() )
		{
			Go * pSpellBook = hMember->GetInventory()->GetActiveSpellBook();
			if ( !pSpellBook )
			{
				return false;
			}			
		}
		
		if ( hMember->GetInventory()->GetActiveSpellBook() )
		{
			m_SpellBookFocus = hMember->GetInventory()->GetActiveSpellBook()->GetGoid();
		}
		else
		{
			m_SpellBookFocus = GOID_INVALID;
			return false;
		}
		gUI.GetGameGUI().ShowInterface( "spell" );

		m_UIPartyMembers[m_SelectedMember]->SetSpellsExpanded( true );		

		gUIInventoryManager.DisplayMagic();		

		UIWindow * pWindow = gUIShell.FindUIWindow( "dialog_box_spellbook_header" );
		int x = gAppModule.GetGameWidth() - ((gAppModule.GetGameWidth() - pWindow->GetRect().right)/2);
		float normalX = ((float)x - ( (float)gAppModule.GetGameWidth()/2.0f))/((float)gAppModule.GetGameWidth()/2.0f);
		gUICamera.SetXScreenTrack( normalX );				

		gUIInventoryManager.CalculateGridHighlights( GetGridboxFromGO( hMember, m_SelectedMember ) );		
	}
	else 
	{
		if ( !gUIStoreManager.GetStoreActive() && !gUIInventoryManager.GetStashActive() )
		{
			gSiegeEngine.GetCompass().SetVisible( true );
			gUIShell.ShowInterface( "compass_hotpoints" );
		}

		if ( gUIStoreManager.GetStoreActive() )
		{
			UIWindow * pInvDialog = gUIShell.FindUIWindow( "dialog_box_inv_bg", "inventory" );
			UIWindow * pStoreDialog = gUIShell.FindUIWindow( "dialog_box_store_frame", "store" );
			if ( pInvDialog->GetRect().right != pStoreDialog->GetRect().left )
			{
				gUIShell.ShiftInterface( "store", -(pStoreDialog->GetRect().left-pInvDialog->GetRect().right), 0 );
			}	
		}

		GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid() );
		m_UIPartyMembers[m_SelectedMember]->SetSpellsExpanded( false );
		gUI.GetGameGUI().HideInterface( "spell" );
		m_SpellBookFocus = GOID_INVALID;
		
		if ( m_UIPartyMembers[m_SelectedMember]->GetPaperDollVisible() )
		{
			UIWindow * pWindow = gUIShell.FindUIWindow( "dialog_box_inv_bg" );
			int x = gAppModule.GetGameWidth() -((gAppModule.GetGameWidth() - pWindow->GetRect().right)/2);
			float normalX = ((float)x - ( (float)gAppModule.GetGameWidth()/2.0f))/((float)gAppModule.GetGameWidth()/2.0f);
			gUICamera.SetXScreenTrack( normalX );
		}

		hMember->GetInventory()->SetActiveSpellBook( 0 );

		gUIInventoryManager.CalculateGridHighlights( GetGridboxFromGO( hMember, m_SelectedMember ) );
	}

	if ( m_UIPartyMembers[m_SelectedMember]->IsSpellsExpanded() )
	{
		GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid() );
		UIButton * pWindow = (UIButton *)gUIShell.FindUIWindow( "button_spellbook_hide", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( true );
		}
		if ( !hMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
		{
			pWindow->DisableButton();
		}
		else
		{
			pWindow->EnableButton();
		}

		pWindow = (UIButton *)gUIShell.FindUIWindow( "button_spellbook_show", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}

		if ( !hMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
		{
			pWindow->DisableButton();
		}
		else
		{
			pWindow->EnableButton();
		}
	}
	else if ( !gUIStoreManager.GetStoreActive() && !gUIInventoryManager.GetStashActive() )
	{
		GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid() );
		UIButton * pWindow = (UIButton *)gUIShell.FindUIWindow( "button_spellbook_hide", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}
		if ( !hMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
		{
			pWindow->DisableButton();
		}
		else
		{
			pWindow->EnableButton();
		}

		pWindow = (UIButton *)gUIShell.FindUIWindow( "button_spellbook_show", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( true );
		}

		if ( !hMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
		{
			pWindow->DisableButton();
		}
		else
		{
			pWindow->EnableButton();
		}
	}

	return true;
}


void UIPartyManager::SetSelectedSpellsActive()
{
	GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid() );
	Go * pSpellBook = hMember->GetInventory()->GetEquipped( ES_SPELLBOOK );
	if ( !pSpellBook )
	{
		return;
	}

	hMember->GetInventory()->SetActiveSpellBook( pSpellBook );	
	bool bActive = !m_UIPartyMembers[m_SelectedMember]->IsSpellsExpanded();
	SetSpellsActive( bActive );	
	if ( bActive )
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( "button_spellbook_hide", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( true );
		}
		pWindow = gUIShell.FindUIWindow( "button_spellbook_show", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}
	}
	else
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( "button_spellbook_hide", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}
		pWindow = gUIShell.FindUIWindow( "button_spellbook_show", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( true );
		}
	}
	pSpellBook->PlayVoiceSound( "gui_spellbook_open", false );	
}


void UIPartyManager::CloseSpellBook()
{
	GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid() );
	Go * pSpellBook = hMember->GetInventory()->GetActiveSpellBook();
	if ( pSpellBook == hMember->GetInventory()->GetEquipped( ES_SPELLBOOK ) )
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( "button_spellbook_hide", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}
		pWindow = gUIShell.FindUIWindow( "button_spellbook_show", "character" );
		if ( pWindow )
		{
			pWindow->SetVisible( true );
		}		
	}

	if ( pSpellBook )
	{
		pSpellBook->PlayVoiceSound( "gui_spellbook_open", false );	
		SetSpellsActive( false );
	}
}


void UIPartyManager::RespawnCharacter()
{
	if ( m_UIPartyMembers[m_SelectedMember] )
	{
		GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid() );
		if ( hMember.IsValid() )
		{
			RSCreateTombstone( hMember->GetGoid() );
			// Reset Lifestate

			if ( hMember->GetLifeState() != LS_DEAD_NORMAL )
			{
				m_bUpdateRespawn = true;
			}

			RSReviveCharacter( hMember->GetGoid() );		
		}		
	}
}


void UIPartyManager::UpdateRespawn()
{
	if ( m_bUpdateRespawn )
	{
		if ( m_UIPartyMembers[m_SelectedMember] )
		{
			GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid() );
			if ( hMember.IsValid() && hMember->GetLifeState() == LS_DEAD_NORMAL )
			{
				RSReviveCharacter( hMember->GetGoid() );
				m_bUpdateRespawn = false;
			}
		}
	}
}


FuBiCookie UIPartyManager::RSCreateTombstone( Goid member )
{
	FUBI_RPC_THIS_CALL_RETRY( RSCreateTombstone, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	GoHandle hMember( member );
	if ( hMember.IsValid() )
	{
		GoCloneReq cloneReq( gContentDb.GetDefaultTombstoneTemplate(), hMember->GetPlayerId() );
		cloneReq.SetStartingPos( hMember->GetPlacement()->GetPosition() );
		cloneReq.SetPrepareToDrawNow();
		Goid newObj = gGoDb.SCloneGo( cloneReq );
		GoHandle hTombstone( newObj );
		if ( hTombstone.IsValid() )
		{
			hTombstone->GetCommon()->SSetScreenName( hMember->GetCommon()->GetScreenName() );
			hTombstone->GetAspect()->SSetIsSelectable( true );
			gGoDb.StartExpiration( hTombstone, true );
		}		
	}	

	return RPC_SUCCESS;
}


// $$$ let's move this into Rules -bk
FuBiCookie UIPartyManager::RSReviveCharacter( Goid member )
{
	FUBI_RPC_THIS_CALL_RETRY( RSReviveCharacter, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	GoHandle hMember( member );
	if ( hMember && hMember->HasAspect() )
	{
		if ( hMember->GetAspect()->GetLifeState() == LS_DEAD_NORMAL )
		{
			hMember->GetAspect()->SSetLifeState( LS_GHOST );
			hMember->GetAspect()->SetIsRespawningAsGhost( false );

			hMember->GetInventory()->SetInventoryDirty();

			GoActor *pActor = hMember->QueryActor();
			if ( pActor != NULL )
			{
				pActor->RSResetUnconsciousDuration( 0 );
			}

			WorldMessage( WE_GAINED_CONSCIOUSNESS, GOID_INVALID, hMember->GetGoid() ).Send();
		}
		else
		{
			hMember->GetAspect()->SSetLifeState( LS_DEAD_NORMAL );		
			hMember->GetAspect()->SetIsRespawningAsGhost( true );			
			WorldMessage( WE_KILLED, GOID_INVALID, member, false ).Send();

			gVictory.SIncrementStat( GS_DEATHS, hMember->GetPlayer() );

			WorldMessage msgEngaged( WE_ENGAGED_KILLED );
			hMember->GetMind()->MessageEngagedMe( msgEngaged );		
		}
	}

	return RPC_SUCCESS;
}


void UIPartyManager::SelectGroup( int index )
{
	if ( gUIShell.GetItemActive() )
	{
		return;
	}

	bool bDeselect = false;	

	if ( m_group[index].bExplicitlySet )
	{	
		GoidColl::iterator i;
		for ( i = m_group[index].group.begin(); i != m_group[index].group.end(); ++i )
		{
			GoHandle hMember( *i );
			if ( hMember.IsValid() && hMember->IsScreenPartyMember() )
			{
				if ( !hMember->IsInActiveScreenWorldFrustum() && gUIShell.GetItemActive() )
				{
					continue;
				}

				if ( gUIStoreManager.GetActiveStoreBuyer() != hMember->GetGoid() )
				{
					gUIStoreManager.HandleStoreSwitch( hMember->GetGoid() );
				}

				if ( bDeselect == false )
				{
					gGoDb.DeselectAll();
					bDeselect = true;
				}

				hMember->Select();
			}
		}

		GoHandle hFocus( m_group[index].focusGoid );
		if ( hFocus.IsValid() && hFocus->IsScreenPartyMember() )
		{
			gGoDb.RSSetFocusGo( hFocus->GetGoid() );
		}
	}
	else
	{
		// do the portrait stuff here
		for ( int i = 0; i != MAX_PARTY_MEMBERS; ++i )
		{
			if ( m_UIPartyMembers[i] && m_UIPartyMembers[i]->GetGoid() != GOID_INVALID )
			{
				GoHandle hMember( m_UIPartyMembers[i]->GetGoid() );
				if ( hMember.IsValid() && hMember->IsScreenPartyMember() )
				{
					if ( GetMemberPortraitRank( hMember ) == index )
					{
						if ( !hMember->IsInActiveScreenWorldFrustum() && gUIShell.GetItemActive() )
						{
							continue;
						}

						if ( gUIStoreManager.GetActiveStoreBuyer() != hMember->GetGoid() )
						{
							gUIStoreManager.HandleStoreSwitch( hMember->GetGoid() );
						}

						if ( gUIInventoryManager.GetStashMember() != hMember->GetGoid() )
						{
							gUIInventoryManager.HandleStashSwitch( hMember->GetGoid() );
						}						

						if ( bDeselect == false )
						{
							gGoDb.DeselectAll();
							bDeselect = true;
						}
						hMember->Select();
					}
				}
			}
		}
	}
}

bool UIPartyManager::GetGroup1()
{		
	SelectGroup( 0 );
	return true;
}


bool UIPartyManager::GetGroup2()
{
	SelectGroup( 1 );
	return true;
}


bool UIPartyManager::GetGroup3()
{
	SelectGroup( 2 );
	return true;
}


bool UIPartyManager::GetGroup4()
{
	SelectGroup( 3 );
	return true;
}

bool UIPartyManager::GetGroup5()
{
	SelectGroup( 4 );
	return true;
}

bool UIPartyManager::GetGroup6()
{
	SelectGroup( 5 );
	return true;
}

bool UIPartyManager::GetGroup7()
{
	SelectGroup( 6 );
	return true;
}

bool UIPartyManager::GetGroup8()
{
	SelectGroup( 7 );
	return true;
}

void UIPartyManager::GetSelectedGroup( SelectedMembers & group )
{	
	group.group.clear();
	GetSelectedPartyMembers( group.group );
	group.focusGoid = gGoDb.GetFocusGo();
	group.bExplicitlySet = true;	
}

bool UIPartyManager::SetGroup1()
{
	GetSelectedGroup( m_group[0] );	
	return true;
}


bool UIPartyManager::SetGroup2()
{
	GetSelectedGroup( m_group[1] );	
	return true;
}


bool UIPartyManager::SetGroup3()
{
	GetSelectedGroup( m_group[2] );	
	return true;
}


bool UIPartyManager::SetGroup4()
{
	GetSelectedGroup( m_group[3] );
	return true;
}

bool UIPartyManager::SetGroup5()
{
	GetSelectedGroup( m_group[4] );
	return true;
}

bool UIPartyManager::SetGroup6()
{
	GetSelectedGroup( m_group[5] );
	return true;
}

bool UIPartyManager::SetGroup7()
{
	GetSelectedGroup( m_group[6] );
	return true;
}

bool UIPartyManager::SetGroup8()
{
	GetSelectedGroup( m_group[7] );
	return true;
}

bool UIPartyManager::DoesPartyHaveGhost( GoidColl & selectedParty )
{
	GoidColl::iterator i;
	for ( i = selectedParty.begin(); i != selectedParty.end(); ++i )
	{
		GoHandle hMember( *i );
		if ( hMember.IsValid() && hMember->IsGhost() )
		{
			return true;
		}
	}

	return false;
}


void UIPartyManager::CastOnCharacterPortrait( int charIndex )
{
	if ( m_UIPartyMembers[charIndex] && 
		 m_UIPartyMembers[charIndex]->GetGoid() != GOID_INVALID )
	{
		GoHandle hMember( m_UIPartyMembers[charIndex]->GetGoid() );
		if ( hMember.IsValid() )
		{
			gUIGame.GetUICommands()->CommandCast( hMember->GetGoid() );
		}
	}

	return;
}


void UIPartyManager::DrawStatusBar( Go * pObject, UIStatusBar * pHealth, UIStatusBar * pMana )
{
	bool bHide = false;
	if ( gWorldState.GetCurrentState() != WS_SP_NIS &&
		 gWorldState.GetCurrentState() != WS_MEGA_MAP )
	{
		if ( GetDrawStatusBars() )
		{			
			if ( pObject && pObject->IsInActiveWorldFrustum() )
			{					
				vector_3 vPos = pObject->GetPlacement()->GetPosition().ScreenPos();

				// Determine scale of status bar
				float distance = gSiegeEngine.GetCamera().GetTargetPosition().Distance( gSiegeEngine.GetCamera().GetCameraPosition() );				
				float scale = gSiegeEngine.GetCamera().GetMinDistance() / distance;	
				
				if ( scale < 0.5f )
				{
					scale = 0.5f;
				}

				int barHeight = 0;
				if ( pHealth )
				{
					pHealth->SetVisible( true );
					pHealth->SetStatus( 1.0f );
				
					float width		= pHealth->GetFloatRect().right		- pHealth->GetFloatRect().left;
					float height	= pHealth->GetScale() == 1.0f		?	(pHealth->GetFloatRect().bottom	- pHealth->GetFloatRect().top) :
																			(pHealth->GetFloatRect().bottom	- pHealth->GetFloatRect().top) / (pHealth->GetScale());
					pHealth->SetScale( scale );
					
					pHealth->GetFloatRect().left	= ((vPos.x) - (width/2.0f));
					pHealth->GetFloatRect().right	= pHealth->GetFloatRect().left + width;
					pHealth->GetFloatRect().top		= ((vPos.y));
					pHealth->GetFloatRect().bottom	= pHealth->GetFloatRect().top + (height * scale);
					pHealth->SetStatus( pObject->GetAspect()->GetLifeRatio() );

					barHeight = pHealth->GetRect().Height();
				}

				if ( pMana )
				{
					// Setup the mana bar				
					pMana->SetVisible( true );
					pMana->SetStatus( 1.0f );				
					
					float width		= pMana->GetFloatRect().right	- pMana->GetFloatRect().left;				
					float height	= pMana->GetScale() == 1.0f					?	(pMana->GetFloatRect().bottom	- pMana->GetFloatRect().top) :
																			(pMana->GetFloatRect().bottom	- pMana->GetFloatRect().top) / (pMana->GetScale());
					pMana->SetScale( scale );

					pMana->GetFloatRect().left		= (vPos.x) - (width/2.0f);
					pMana->GetFloatRect().right		= pMana->GetFloatRect().left + width;
					pMana->GetFloatRect().top		= (vPos.y) + (height * scale);
					pMana->GetFloatRect().bottom	= pMana->GetFloatRect().top + (float)barHeight;
					pMana->SetStatus( pObject->GetAspect()->GetManaRatio() );	
					
					pMana->GetRect().bottom = pMana->GetRect().top + barHeight;
				}
			}
			else if ( pObject && !pObject->IsInActiveWorldFrustum() )
			{
				bHide = true;
			}
		}
		else if ( pObject )
		{				
			bHide = true;			
		}
	}
	else 
	{		
		bHide = true;
	}

	if ( bHide )
	{
		if ( pHealth )
		{
			pHealth->SetVisible( false );
		}
		if ( pMana )
		{
			pMana->SetVisible( false );			
		}
	}
}


void UIPartyManager::DrawMemberStatusBars( int memberIndex )
{
	GoHandle hMember( m_UIPartyMembers[memberIndex]->GetGoid() );
	
	// Setup the health bar
	UIStatusBar * pHealthBar = ((UIStatusBar *)m_UIPartyMembers[memberIndex]->GetGUIWindow( GUI_HEALTH_FLOAT ));
	UIStatusBar * pManaBar = ((UIStatusBar *)m_UIPartyMembers[memberIndex]->GetGUIWindow( GUI_MANA_FLOAT ));	

	if ( hMember.IsValid() && pHealthBar && pManaBar )
	{
		if ( ::IsMultiPlayer() )
		{
			pHealthBar->SetBorderColor( 0xff00ff00 );
			pManaBar->SetBorderColor( 0xff00ff00 );
		}
		DrawStatusBar( hMember, pHealthBar, pManaBar );
	}
}


void UIPartyManager::DrawTeamStatusBars()
{
	Player *localPlayer = gServer.GetLocalHumanPlayer();

	GoHandle hHero( localPlayer->GetHero() );
	if ( !hHero.IsValid() )
	{
		return;
	}

	siege::SiegeFrustum * pFrustum = gSiegeEngine.GetFrustum( (DWORD)gGoDb.GetGoFrustum( hHero->GetGoid() ) );

	Team * pHomeTeam = gServer.GetTeam( gServer.GetLocalHumanPlayer()->GetId() );
	if ( pHomeTeam )
	{
		Server::PlayerColl players = gServer.GetPlayers();
		Server::PlayerColl::iterator i;
		eMultiBar healthBar = MB_HEALTH_1;
		eMultiBar manaBar = MB_MANA_1;
		if ( GetDrawStatusBars() )
		{
			for ( i = players.begin(); i != players.end(); ++i )
			{
				if ( IsValid(*i) && gServer.GetLocalHumanPlayer() )
				{
					if ( gServer.GetTeam( (*i)->GetId() ) == pHomeTeam && (*i)->GetId() != gServer.GetLocalHumanPlayer()->GetId() )
					{
						if ( (*i)->GetParty() )
						{
							GoHandle hMember( *((*i)->GetParty()->GetChildren().begin()) );

							if ( !hMember.IsValid() || !hMember->HasPlacement() || !hMember->HasAspect() )
							{
								continue;
							}

							bool drawLabel = false;

							SiegeNode * pNode = gSiegeEngine.IsNodeValid( hMember->GetPlacement()->GetPosition().node );
							if ( pNode )
							{
								if ( !pNode->IsOwnedByFrustum( pFrustum->GetIdBitfield() ) )
								{
									continue;
								}
							}	
							else 
							{
								continue;
							}

							if ( gWorldState.GetCurrentState() == WS_MEGA_MAP )
							{
								if ( hMember->GetAspect()->GetIsScreenPlayerVisible() || localPlayer->GetIsFriend( *i ) )
								{
									drawLabel = true;
								}
							}
							else
							{
								vector_3 worldPos = hMember->GetPlacement()->GetPosition().WorldPos();
								if ( !gSiegeEngine.GetCamera().GetFrustum().CullSphere( worldPos, 1.0f ) )
								{
									drawLabel = true;
								}			
							}

							if ( drawLabel && m_pMultiBars[(int)healthBar] && m_pMultiBars[(int)manaBar] )
							{
								DrawStatusBar( hMember, (UIStatusBar *)m_pMultiBars[(int)healthBar], (UIStatusBar *)m_pMultiBars[(int)manaBar] );
								int healthIndex = (int)healthBar + 2;
								int manaIndex = (int)manaBar + 2;
								healthBar = (eMultiBar)healthIndex;
								manaBar = (eMultiBar)manaIndex;
							}
						}
					}
				}
			}
		}

		for ( int iHide = (int)healthBar; iHide != (int)MB_COUNT; ++iHide )
		{
			m_pMultiBars[(eMultiBar)iHide]->SetVisible( false );
		}
	}	
}


void UIPartyManager::DrawEnemyStatusBars( Go * pObject )
{
	bool bHide = false;
	if ( gWorldState.GetCurrentState() != WS_SP_NIS &&
		 gWorldState.GetCurrentState() != WS_MEGA_MAP )
	{
		if (( GetDrawStatusBars() ) && IsAlive( pObject->GetLifeState() ))
		{
			UIStatusBar * pHealthBar = (UIStatusBar *)m_pWindow[ENEMY_HEALTH_BAR];	

			DrawStatusBar( pObject, pHealthBar, NULL );
		}
		else
		{
			bHide = true;
		}
	}
	else 
	{
		bHide = true;
	}

	if ( bHide )
	{
		m_pWindow[ENEMY_HEALTH_BAR]->SetVisible( false );		
	}
}


// If the character screen is visible, update the statistical
// information for the selected character.
void UIPartyManager::UpdatePlayerStats()
{
	for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i )
	{			
		if ( m_UIPartyMembers[i] && (m_UIPartyMembers[i]->GetGoid() != GOID_INVALID ) )
		{
			GoHandle hMember( m_UIPartyMembers[i]->GetGoid() );

			if( !hMember.IsValid() )
			{
				continue;
			}			

			if ( !hMember->IsInActiveWorldFrustum() )
			{
				gUIShell.InsertDragBox( ((UIItemSlot *)m_UIPartyMembers[i]->GetGUIWindow( GUI_PORTRAIT ))->GetRect(), m_dwInactiveColor );
			}

			if ( hMember->HasActor() ) 
			{
				if ( m_UIPartyMembers[i]->GetGUIWindow( GUI_DEATH ) )
				{
					if ( !IsAlive( hMember->GetAspect()->GetLifeState() ) )
					{
						m_UIPartyMembers[i]->GetGUIWindow( GUI_DEATH )->SetVisible( true );						

						if ( hMember->GetAspect()->GetLifeState() == LS_GHOST )
						{							
							m_UIPartyMembers[i]->GetGUIWindow( GUI_GHOST_TIMER )->SetVisible( true );
							int seconds = (int)floorf(hMember->GetAspect()->GetGhostTimeout());
							int minutes = seconds / 60;
							seconds = seconds - (minutes * 60);
							gpstring sGhostTimer;
							sGhostTimer.assignf( "%d:%s%d", minutes, seconds < 10 ? "0" : "", seconds );
							((UIText *)m_UIPartyMembers[i]->GetGUIWindow( GUI_GHOST_TIMER ))->SetText( ToUnicode( sGhostTimer ) );
						}
						else						
						{
							m_UIPartyMembers[i]->GetGUIWindow( GUI_GHOST_TIMER )->SetVisible( false );						
						}
					}
					else
					{
						m_UIPartyMembers[i]->GetGUIWindow( GUI_DEATH )->SetVisible( false );
						m_UIPartyMembers[i]->GetGUIWindow( GUI_GHOST_TIMER )->SetVisible( false );						
					}
				}

				// indicate that status is changing if the character is offscreen
				UIStatusBar * pHealthStatus = ((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_HEALTH ));
				UIStatusBar * pManaStatus	= ((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_MANA ));

				if ( !hMember->GetAspect()->GetIsVisibleThisFrame() && 
					 pHealthStatus->GetStatus() > hMember->GetAspect()->GetLifeRatio() )
				{
					pHealthStatus->ReceiveMessage( UIMessage( MSG_STARTANIMATION ) );					
				}			
				pHealthStatus->SetStatus( hMember->GetAspect()->GetLifeRatio() );

				if ( !hMember->GetAspect()->GetIsVisibleThisFrame() &&
					 pManaStatus->GetStatus() > hMember->GetAspect()->GetManaRatio() )
				{
					pManaStatus->ReceiveMessage( UIMessage( MSG_STARTANIMATION ) );					
				}				
				pManaStatus->SetStatus( hMember->GetAspect()->GetManaRatio() );

				gpwstring sText;
				sText.assignf( L"%d", hMember->GetInventory()->GetGold() );
				
				float	melee = 0;
				float	ranged = 0;
				float	good_magic = 0;
				float	dark_magic = 0;
				Skill *pSkill = 0;
				
				UIStatusBar * pUnconscious = (UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_UNCONSCIOUS );
				if ( pUnconscious )
				{
					if ( hMember->GetAspect()->GetLifeState() == LS_ALIVE_UNCONSCIOUS )
					{
						if ( !pUnconscious->GetVisible() )
						{
							pUnconscious->SetVisible( true );
							pUnconscious->ReceiveMessage( UIMessage( MSG_STARTANIMATION ) );
							pUnconscious->SetBaseMax( hMember->GetAspect()->GetCurrentLife()-1.0f );
						}

						if ( hMember->GetAspect()->GetCurrentLife() < pUnconscious->GetBaseMax() )
						{
							pUnconscious->SetBaseMax( hMember->GetAspect()->GetCurrentLife()-1.0f );
						}
						
						if ( pUnconscious->GetBaseMax() != 0.0f )
						{
							pUnconscious->SetStatus( (hMember->GetAspect()->GetCurrentLife()-1.0f) / pUnconscious->GetBaseMax() );
						}
					}
					else
					{
						pUnconscious->SetVisible( false );
					}
				}

				UIWindow * pWarning = m_UIPartyMembers[i]->GetGUIWindow( GUI_HEALTH_WARNING );
				if ( pWarning )
				{
					if ( hMember->GetAspect()->GetLifeRatio() <= 0.33333f && !(hMember->GetAspect()->GetLifeState() == LS_ALIVE_UNCONSCIOUS) )
					{
						if ( !pWarning->GetVisible() )
						{
							pWarning->SetVisible( true );
							pWarning->ReceiveMessage( UIMessage( MSG_STARTANIMATION ) );
							hMember->PlayVoiceSound( "order_low_health_warn", false );
						}
					}
					else
					{
						pWarning->SetVisible( false );
					}
				}
				
				if ( !IsAlive( hMember->GetAspect()->GetLifeState() ) )
				{
					pWarning->SetVisible( false );
					pUnconscious->SetVisible( false );
				}

				if ( hMember->GetInventory()->IsPackOnly() )
				{
					continue;
				}
				
				Go * pItem = hMember->GetInventory()->GetItem( hMember->GetInventory()->GetSelectedLocation() );
				if ( pItem )
				{
					float activeSkill = 0.0f;
					if( hMember->GetActor()->GetSkill( pItem->IsSpell() ? pItem->GetMagic()->GetSkillClass() : pItem->GetAttack()->GetSkillClass(), &pSkill ) )
					{
						activeSkill = pSkill->GetLevelMasteryRatio();
					}
					if ( m_UIPartyMembers[i]->GetGUIWindow( GUI_SKILL_BAR_ACTIVE ) )
					{
						((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_SKILL_BAR_ACTIVE ))->SetStatus( activeSkill );
					}

					if ( pItem->HasAttack() && pItem->GetAttack()->GetAttackClass() == AC_MINIGUN )
					{
						((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_MIN_FUEL_BAR ))->SetStatus( pItem->GetAspect()->GetManaRatio() );
					}
					else 
					{
						((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_MIN_FUEL_BAR ))->SetStatus( 0.0f );
					}
				}
				else if ( m_UIPartyMembers[i]->GetGUIWindow( GUI_SKILL_BAR_ACTIVE ) )
				{
					((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_SKILL_BAR_ACTIVE ))->SetStatus( 0.0f );
					((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_MIN_FUEL_BAR ))->SetStatus( 0.0f );
				}

				pItem = hMember->GetInventory()->GetItem( IL_ACTIVE_MELEE_WEAPON );
				if ( pItem )
				{
					if( hMember->GetActor()->GetSkill( pItem->GetAttack()->GetSkillClass(), &pSkill ) )
					{
						melee = pSkill->GetLevelMasteryRatio();
					}
					((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_SKILL_BAR_1 ))->SetStatus( melee );
				}
				else
				{
					((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_SKILL_BAR_1 ))->SetStatus( 0.0f );
				}

				pItem = hMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON );
				if ( pItem )
				{
					if( hMember->GetActor()->GetSkill( pItem->GetAttack()->GetSkillClass(), &pSkill ) )
					{
						ranged = pSkill->GetLevelMasteryRatio();
					}
					((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_SKILL_BAR_2 ))->SetStatus( ranged );

					if ( pItem->HasAttack() && pItem->GetAttack()->GetAttackClass() == AC_MINIGUN )
					{
						((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_FUEL_BAR ))->SetStatus( pItem->GetAspect()->GetManaRatio() );
					}
					else
					{
						((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_FUEL_BAR ))->SetStatus( 0.0f );
					}
				}
				else
				{
					((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_SKILL_BAR_2 ))->SetStatus( 0.0f );
					((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_FUEL_BAR ))->SetStatus( 0.0f );
				}

				pItem = hMember->GetInventory()->GetItem( IL_ACTIVE_PRIMARY_SPELL );
				if ( pItem )
				{
					if( hMember->GetActor()->GetSkill( pItem->GetMagic()->GetSkillClass(), &pSkill ) )
					{
						good_magic = pSkill->GetLevelMasteryRatio();
					}
					((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_SKILL_BAR_3 ))->SetStatus( good_magic );
				}
				else
				{
					((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_SKILL_BAR_3 ))->SetStatus( 0.0f );
				}

				pItem = hMember->GetInventory()->GetItem( IL_ACTIVE_SECONDARY_SPELL );
				if ( pItem )
				{
					if( hMember->GetActor()->GetSkill( pItem->GetMagic()->GetSkillClass(), &pSkill ) )
					{
						dark_magic = pSkill->GetLevelMasteryRatio();
					}
					((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_SKILL_BAR_4 ))->SetStatus( dark_magic );
				}
				else
				{
					((UIStatusBar *)m_UIPartyMembers[i]->GetGUIWindow( GUI_SKILL_BAR_4 ))->SetStatus( 0.0f );
				}				
				
				// update guard status so we can see who is being guarded
				m_UIPartyMembers[i]->GetGUIWindow( GUI_GUARDED )->SetVisible( false );
				GoidColl::iterator iGuarded;
				for ( iGuarded = m_GuardedCharacters.begin(); iGuarded != m_GuardedCharacters.end(); ++iGuarded )
				{
					if ( (*iGuarded) == hMember->GetGoid() )
					{
						m_UIPartyMembers[i]->GetGUIWindow( GUI_GUARDED )->SetVisible( true );
					}
				}
			}

			if ( !IsAlive( hMember->GetAspect()->GetLifeState() ) )
			{
				continue;
			}

			if ( hMember->IsSelected() && hMember->GetAspect()->GetIsVisible() )
			{
				DrawMemberStatusBars( i );
			}
		}
	}

	if ( m_UIPartyMembers[m_SelectedMember] && gUI.GetGameGUI().IsInterfaceVisible( "character" ) )
	{
		GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid() );
		if( !hMember.IsValid() )
		{
			return;
		}
		
		gpstring sTemp;
				
		((UIText *)m_pWindow[NAME])->SetText( hMember->GetCommon()->GetScreenName() );
		((UIText *)m_pWindow[CLASS])->SetText( hMember->GetActor()->GetClass() );
		
		sTemp.assignf( "%d", hMember->GetInventory()->GetGold()-hMember->GetPlayer()->GetTradeGoldAmount() );
		if ( !hMember->GetInventory()->IsPackOnly() )
		{
			((UIText *)m_pWindow[GOLD_VALUE])->SetText( ToUnicode( sTemp ) );			
		}		
		
		int healthDelta = 0, manaDelta = 0, armorDelta = 0, meleeDamageDelta = 0, rangedDamageDelta = 0;
		CalculateAlterationEffects( hMember, healthDelta, manaDelta, armorDelta, meleeDamageDelta, rangedDamageDelta ); 

		float damage_min = 0, damage_max = 0; 
		Go * pWeapon = hMember->GetInventory()->GetItem( IL_ACTIVE_MELEE_WEAPON );
		if( pWeapon != NULL ) 
		{	
			float damageWeaponMin = 0.0f, damageWeaponMax = 0.0f;
			gRules.GetDamageRange( hMember->GetGoid(), pWeapon->GetGoid(), damage_min, damage_max, false );
			pWeapon->GetAttack()->EvaluateCustomMinMaxDamage( damageWeaponMin, damageWeaponMax );
			damage_min += damageWeaponMin;
			damage_max += damageWeaponMax;		
		} 
		else 
		{
			gRules.GetDamageRange( hMember->GetGoid(), hMember->GetGoid(), damage_min, damage_max, false );						
		}	

		sTemp.assignf( "%d-%d", (int)floorf( damage_min ), (int)floorf( damage_max ) );
		((UIText *)m_pWindow[MELEE_DAMAGE_VALUE])->SetText( ToUnicode( sTemp ) );		
		CalculateStatTextColor( meleeDamageDelta, ((UIText *)m_pWindow[MELEE_DAMAGE_VALUE]) );

		damage_min = 0; 
		damage_max = 0;
		pWeapon = hMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON );
		if( pWeapon != NULL ) 
		{
			float damageWeaponMin = 0, damageWeaponMax = 0;
			pWeapon->GetAttack()->EvaluateTotalMinMaxDamage( damageWeaponMin, damageWeaponMax, false );
			
			damage_min += damageWeaponMin;
			damage_max += damageWeaponMax;						
		}

		damage_min += hMember->GetAttack()->GetDamageBonusMinRanged();
		damage_max += hMember->GetAttack()->GetDamageBonusMaxRanged();

		sTemp.assignf( "%d-%d", (int)floorf( damage_min ), (int)floorf( damage_max ) );		
		((UIText *)m_pWindow[RANGED_DAMAGE_VALUE])->SetText( ToUnicode( sTemp ) );						
		CalculateStatTextColor( rangedDamageDelta, ((UIText *)m_pWindow[RANGED_DAMAGE_VALUE]) );
		
		sTemp.assignf( "%d", (int)floorf(gRules.GetTotalDefense( hMember->GetGoid() )));
		((UIText *)m_pWindow[ARMOR_VALUE])->SetText( ToUnicode( sTemp ) );
		CalculateStatTextColor( armorDelta, ((UIText *)m_pWindow[ARMOR_VALUE]) );

		GoActor *pActorMember = hMember->GetActor();
		Skill * pSkill = 0;
		
		sTemp.assignf( "%d", (int)floorf(pActorMember->GetSkillLevel( "melee" )) );
		((UIText *)m_pWindow[MELEE_EXP_VALUE])->SetText( ToUnicode( sTemp ) );		
		pActorMember->GetSkill( "melee", &pSkill );	
		CalculateSkillTextColor( pSkill, ((UIText *)m_pWindow[MELEE_EXP_VALUE]) );
		
		sTemp.assignf( "%d", (int)floorf(pActorMember->GetSkillLevel( "ranged" )) );
		((UIText *)m_pWindow[RANGED_EXP_VALUE])->SetText( ToUnicode( sTemp ) );
		pActorMember->GetSkill( "ranged", &pSkill );	
		CalculateSkillTextColor( pSkill, ((UIText *)m_pWindow[RANGED_EXP_VALUE]) );
		
		sTemp.assignf( "%d", (int)floorf(pActorMember->GetSkillLevel( "combat magic" )) );
		((UIText *)m_pWindow[DARKMAGIC_EXP_VALUE])->SetText( ToUnicode( sTemp ) );
		pActorMember->GetSkill( "combat magic", &pSkill );	
		CalculateSkillTextColor( pSkill, ((UIText *)m_pWindow[DARKMAGIC_EXP_VALUE]) );

		sTemp.assignf( "%d", (int)floorf(pActorMember->GetSkillLevel( "nature magic" )) );		
		((UIText *)m_pWindow[GOODMAGIC_EXP_VALUE])->SetText( ToUnicode( sTemp ) );
		pActorMember->GetSkill( "nature magic", &pSkill );	
		CalculateSkillTextColor( pSkill, ((UIText *)m_pWindow[GOODMAGIC_EXP_VALUE]) );

		sTemp.assignf( "%d", (int)floorf(pActorMember->GetSkillLevel( "intelligence" )) );		
		((UIText *)m_pWindow[INT_VALUE])->SetText( ToUnicode( sTemp ) );
		pActorMember->GetSkill( "intelligence", &pSkill );	
		CalculateSkillTextColor( pSkill, ((UIText *)m_pWindow[INT_VALUE]) );
		
		sTemp.assignf( "%d", (int)floorf(pActorMember->GetSkillLevel( "strength" )) );
		((UIText *)m_pWindow[STRENGTH_VALUE])->SetText( ToUnicode( sTemp ) );
		pActorMember->GetSkill( "strength", &pSkill );	
		CalculateSkillTextColor( pSkill, ((UIText *)m_pWindow[STRENGTH_VALUE]) );
		
		sTemp.assignf( "%d", (int)floorf(pActorMember->GetSkillLevel( "dexterity" )) );
		((UIText *)m_pWindow[DEX_VALUE])->SetText( ToUnicode( sTemp ) );
		pActorMember->GetSkill( "dexterity", &pSkill );	
		CalculateSkillTextColor( pSkill, ((UIText *)m_pWindow[DEX_VALUE]) );
				
		const int current_life	= ( fix_precision( hMember->GetAspect()->GetCurrentLife() ) < 0 ) ? 0 : fix_precision( hMember->GetAspect()->GetCurrentLife() );
		const int max_life		= fix_precision( hMember->GetAspect()->GetMaxLife() );

		sTemp.assignf( "%d/%d", current_life, max_life );		
		((UIText *)m_pWindow[MAX_HEALTH])->SetText( ToUnicode( sTemp ) );		
		CalculateStatTextColor( healthDelta, ((UIText *)m_pWindow[MAX_HEALTH]) );
	
		sTemp.assignf( "%d/%d", (int)floorf(hMember->GetAspect()->GetCurrentMana()), fix_precision(hMember->GetAspect()->GetMaxMana()) );
		((UIText *)m_pWindow[MAX_MANA])->SetText( ToUnicode( sTemp ) );		
		CalculateStatTextColor( manaDelta, ((UIText *)m_pWindow[MAX_MANA]) );

		float	melee		= 0;
		float	ranged		= 0;
		float	good_magic	= 0;
		float	dark_magic	= 0;
		float	intel		= 0;
		float	str			= 0;
		float	dex			= 0;

		pSkill = 0;

		if( pActorMember->GetSkill( "melee", &pSkill ) ) 
		{
			melee = pSkill->GetLevelMasteryRatio();
		}
		if( pActorMember->GetSkill( "ranged", &pSkill ) ) 
		{
			ranged = pSkill->GetLevelMasteryRatio();
		}
		if( pActorMember->GetSkill( "nature magic", &pSkill ) )
		{
			good_magic = pSkill->GetLevelMasteryRatio();
		}
		if( pActorMember->GetSkill( "combat magic", &pSkill ) ) 
		{
			dark_magic = pSkill->GetLevelMasteryRatio();
		}
		if( pActorMember->GetSkill( "strength", &pSkill ) )
		{
			str = pSkill->GetLevelMasteryRatio();			
		}
		if( pActorMember->GetSkill( "dexterity", &pSkill ) )
		{
			dex = pSkill->GetLevelMasteryRatio();			
		}
		if( pActorMember->GetSkill( "intelligence", &pSkill ) )
		{
			intel = pSkill->GetLevelMasteryRatio();			
		}

		((UIStatusBar *)m_pWindow[MELEE_BAR])->SetStatus( melee );
		((UIStatusBar *)m_pWindow[RANGED_BAR])->SetStatus( ranged );
		((UIStatusBar *)m_pWindow[GOODMAGIC_BAR])->SetStatus( good_magic );
		((UIStatusBar *)m_pWindow[DARKMAGIC_BAR])->SetStatus( dark_magic );
		((UIStatusBar *)m_pWindow[HEALTH_BAR])->SetStatus( hMember->GetAspect()->GetLifeRatio() );
		((UIStatusBar *)m_pWindow[MANA_BAR])->SetStatus( hMember->GetAspect()->GetManaRatio() );
		((UIStatusBar *)m_pWindow[STRENGTH_BAR])->SetStatus( str );		
		((UIStatusBar *)m_pWindow[DEXTERITY_BAR])->SetStatus( dex );	
		((UIStatusBar *)m_pWindow[INTELLIGENCE_BAR])->SetStatus( intel );	
	}
	else if ( m_bSingleInventoryVisible )
	{
		GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid() );
		if( !hMember.IsValid() )
		{
			return;
		}

		if ( hMember->GetInventory()->IsPackOnly() )
		{	
			gpstring sTemp;
			sTemp.assignf( "%d", hMember->GetInventory()->GetGold() - hMember->GetPlayer()->GetTradeGoldAmount() );		
			((UIText *)m_pWindow[PACKMULE_GOLD_VALUE])->SetText( ToUnicode( sTemp ) );					
		}		

		if ( ::IsMultiPlayer() && m_pWindow[TRADE_GOLD] )
		{
			gpstring sTemp;
			sTemp.assignf( "%d", hMember->GetInventory()->GetGold() - hMember->GetPlayer()->GetTradeGoldAmount() );		
			((UIText *)m_pWindow[TRADE_GOLD])->SetText( ToUnicode( sTemp ) );					
		}
	}	
}


void UIPartyManager::ShowHireStats( Go * pHire )
{
	if ( gUIShell.IsInterfaceVisible( "hire_stats" ) )
	{
		gUIShell.HideInterface( "hire_stats" );
		return;
	}

	gUIShell.ShowInterface( "hire_stats" );

	gpstring sTemp;
	
	UIWindow * pWindow = 0;

	gRules.UpdateClassDesignation( pHire->GetGoid() );
		
	pWindow = gUIShell.FindUIWindow( "character_name", "hire_stats" );
	((UIText *)pWindow)->SetText( pHire->GetCommon()->GetScreenName() );

	pWindow = gUIShell.FindUIWindow( "character_class", "hire_stats" );
	((UIText *)pWindow)->SetText( pHire->GetActor()->GetClass() );
		
	int healthDelta = 0, manaDelta = 0, armorDelta = 0, meleeDamageDelta = 0, rangedDamageDelta = 0;
	CalculateAlterationEffects( pHire, healthDelta, manaDelta, armorDelta, meleeDamageDelta, rangedDamageDelta ); 

	float damage_min = 0, damage_max = 0; 
	Go * pWeapon = pHire->GetInventory()->GetItem( IL_ACTIVE_MELEE_WEAPON );
	if( pWeapon != NULL ) 
	{
		gRules.GetDamageRange( pHire->GetGoid(), pWeapon->GetGoid(), damage_min, damage_max );
		pWeapon->GetAttack()->EvaluateCustomMinMaxDamage( damage_min, damage_max );
	} 
	else 
	{
		gRules.GetDamageRange( pHire->GetGoid(), pHire->GetGoid(), damage_min, damage_max );			
	}
	
	sTemp.assignf( "%d-%d", (int)floorf( damage_min ), (int)floorf( damage_max ) );
	pWindow = gUIShell.FindUIWindow( "text_melee_damage_value", "hire_stats" );
	((UIText *)pWindow)->SetText( ToUnicode( sTemp ) );		
	CalculateStatTextColor( meleeDamageDelta, ((UIText *)pWindow) );

	damage_min = 0; 
	damage_max = 0;
	pWeapon = pHire->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON );
	if( pWeapon != NULL ) 
	{
		gRules.GetDamageRange( pHire->GetGoid(), pWeapon->GetGoid(), damage_min, damage_max );
		pWeapon->GetAttack()->EvaluateCustomMinMaxDamage( damage_min, damage_max );
	}

	sTemp.assignf( "%d-%d", (int)floorf( damage_min ), (int)floorf( damage_max ) );		
	pWindow = gUIShell.FindUIWindow( "text_ranged_damage_value", "hire_stats" );
	((UIText *)pWindow)->SetText( ToUnicode( sTemp ) );						
	CalculateStatTextColor( rangedDamageDelta, ((UIText *)pWindow) );
	
	sTemp.assignf( "%d", (int)floorf(gRules.GetTotalDefense( pHire->GetGoid() )));
	pWindow = gUIShell.FindUIWindow( "character_text_armor_value", "hire_stats" );
	((UIText *)pWindow)->SetText( ToUnicode( sTemp ) );
	CalculateStatTextColor( armorDelta, ((UIText *)pWindow) );

	GoActor *pActorMember = pHire->GetActor();
	Skill * pSkill = 0;
	
	sTemp.assignf( "%d", (int)floorf(pActorMember->GetSkillLevel( "melee" )) );
	pWindow = gUIShell.FindUIWindow( "character_text_skills_melee_value", "hire_stats" );
	((UIText *)pWindow)->SetText( ToUnicode( sTemp ) );		
	pActorMember->GetSkill( "melee", &pSkill );	
	CalculateSkillTextColor( pSkill, ((UIText *)pWindow) );
	
	sTemp.assignf( "%d", (int)floorf(pActorMember->GetSkillLevel( "ranged" )) );
	pWindow = gUIShell.FindUIWindow( "character_text_skills_ranged_value", "hire_stats" );
	((UIText *)pWindow)->SetText( ToUnicode( sTemp ) );
	pActorMember->GetSkill( "ranged", &pSkill );	
	CalculateSkillTextColor( pSkill, ((UIText *)pWindow) );
	
	sTemp.assignf( "%d", (int)floorf(pActorMember->GetSkillLevel( "combat magic" )) );
	pWindow = gUIShell.FindUIWindow( "character_text_skills_dark_magic_value", "hire_stats" );
	((UIText *)pWindow)->SetText( ToUnicode( sTemp ) );
	pActorMember->GetSkill( "combat magic", &pSkill );	
	CalculateSkillTextColor( pSkill, ((UIText *)pWindow) );

	sTemp.assignf( "%d", (int)floorf(pActorMember->GetSkillLevel( "nature magic" )) );		
	pWindow = gUIShell.FindUIWindow( "character_text_skills_good_magic_value", "hire_stats" );
	((UIText *)pWindow)->SetText( ToUnicode( sTemp ) );
	pActorMember->GetSkill( "nature magic", &pSkill );	
	CalculateSkillTextColor( pSkill, ((UIText *)pWindow) );

	sTemp.assignf( "%d", (int)floorf(pActorMember->GetSkillLevel( "intelligence" )) );		
	pWindow = gUIShell.FindUIWindow( "character_text_intelligence_value", "hire_stats" );
	((UIText *)pWindow)->SetText( ToUnicode( sTemp ) );
	pActorMember->GetSkill( "intelligence", &pSkill );	
	CalculateSkillTextColor( pSkill, ((UIText *)pWindow) );
	
	sTemp.assignf( "%d", (int)floorf(pActorMember->GetSkillLevel( "strength" )) );
	pWindow = gUIShell.FindUIWindow( "character_text_strength_value", "hire_stats" );
	((UIText *)pWindow)->SetText( ToUnicode( sTemp ) );
	pActorMember->GetSkill( "strength", &pSkill );	
	CalculateSkillTextColor( pSkill, ((UIText *)pWindow) );
	
	sTemp.assignf( "%d", (int)floorf(pActorMember->GetSkillLevel( "dexterity" )) );
	pWindow = gUIShell.FindUIWindow( "character_text_dexterity_value", "hire_stats" );
	((UIText *)pWindow)->SetText( ToUnicode( sTemp ) );
	pActorMember->GetSkill( "dexterity", &pSkill );	
	CalculateSkillTextColor( pSkill, ((UIText *)pWindow) );

	// life = 1 if { life| 0 < life < 2 }
	int current_life = (int)floorf(pHire->GetAspect()->GetCurrentLife());
	current_life = ( (current_life > 0)&&(current_life < 2) )? 1 : current_life;

	sTemp.assignf( "%d/%d", current_life, (int)floorf(pHire->GetAspect()->GetMaxLife()) );		
	pWindow = gUIShell.FindUIWindow( "character_text_health_value", "hire_stats" );
	((UIText *)pWindow)->SetText( ToUnicode( sTemp ) );		
	CalculateStatTextColor( healthDelta, ((UIText *)pWindow) );

	sTemp.assignf( "%d/%d", (int)floorf(pHire->GetAspect()->GetCurrentMana()), fix_precision(pHire->GetAspect()->GetMaxMana()) );
	pWindow = gUIShell.FindUIWindow( "character_text_mana_value", "hire_stats" );
	((UIText *)pWindow)->SetText( ToUnicode( sTemp ) );		
	CalculateStatTextColor( manaDelta, ((UIText *)pWindow) );

	float	melee		= 0;
	float	ranged		= 0;
	float	good_magic	= 0;
	float	dark_magic	= 0;
	float	intel		= 0;
	float	str			= 0;
	float	dex			= 0;

	pSkill = 0;

	if( pActorMember->GetSkill( "melee", &pSkill ) ) 
	{
		melee = pSkill->GetLevelMasteryRatio();
	}
	if( pActorMember->GetSkill( "ranged", &pSkill ) ) 
	{
		ranged = pSkill->GetLevelMasteryRatio();
	}
	if( pActorMember->GetSkill( "nature magic", &pSkill ) )
	{
		good_magic = pSkill->GetLevelMasteryRatio();
	}
	if( pActorMember->GetSkill( "combat magic", &pSkill ) ) 
	{
		dark_magic = pSkill->GetLevelMasteryRatio();
	}
	if( pActorMember->GetSkill( "strength", &pSkill ) )
	{
		str = pSkill->GetLevelMasteryRatio();			
	}
	if( pActorMember->GetSkill( "dexterity", &pSkill ) )
	{
		dex = pSkill->GetLevelMasteryRatio();			
	}
	if( pActorMember->GetSkill( "intelligence", &pSkill ) )
	{
		intel = pSkill->GetLevelMasteryRatio();			
	}

	pWindow = gUIShell.FindUIWindow( "character_status_melee_bar", "hire_stats" );
	((UIStatusBar *)pWindow)->SetStatus( melee );
	pWindow = gUIShell.FindUIWindow( "character_status_ranged_bar", "hire_stats" );
	((UIStatusBar *)pWindow)->SetStatus( ranged );
	pWindow = gUIShell.FindUIWindow( "character_status_good_magic_bar", "hire_stats" );
	((UIStatusBar *)pWindow)->SetStatus( good_magic );
	pWindow = gUIShell.FindUIWindow( "character_status_dark_magic_bar", "hire_stats" );
	((UIStatusBar *)pWindow)->SetStatus( dark_magic );
	pWindow = gUIShell.FindUIWindow( "status_bar_health", "hire_stats" );
	((UIStatusBar *)pWindow)->SetStatus( pHire->GetAspect()->GetLifeRatio() );
	pWindow = gUIShell.FindUIWindow( "status_bar_mana", "hire_stats" );
	((UIStatusBar *)pWindow)->SetStatus( pHire->GetAspect()->GetManaRatio() );
	pWindow = gUIShell.FindUIWindow( "character_status_strength_bar", "hire_stats" );
	((UIStatusBar *)pWindow)->SetStatus( str );		
	pWindow = gUIShell.FindUIWindow( "character_status_dexterity_bar", "hire_stats" );
	((UIStatusBar *)pWindow)->SetStatus( dex );	
	pWindow = gUIShell.FindUIWindow( "character_status_intelligence_bar", "hire_stats" );
	((UIStatusBar *)pWindow)->SetStatus( intel );		
}


void UIPartyManager::CalculateSkillTextColor( Skill * pSkill, UIText * pText )
{
	if ( IsNegative( (pSkill->GetLevel() - pSkill->GetLevelBias() - pSkill->GetNaturalLevel()) ) )
	{
		// Negative effect
		pText->SetColor( m_dwNegTextColor );
	}
	else if ( IsPositive( (pSkill->GetLevel() - pSkill->GetLevelBias() - pSkill->GetNaturalLevel()) ) )
	{
		// Positive effect
		pText->SetColor( m_dwPosTextColor );
	}
	else
	{
		pText->SetColor( 0xffffffff );
	}
}


void UIPartyManager::CalculateAlterationEffects( Go * pMember, 
												int & healthDelta, int & manaDelta, 
												int & armorDelta, int & meleeDamageDelta,
												int & rangedDamageDelta )
{
	healthDelta = 0;
	manaDelta	= 0;
	armorDelta	= 0;
	meleeDamageDelta = 0;
	rangedDamageDelta = 0;
	if ( pMember->HasDefend() )
	{
		if ( pMember->GetDefend()->GetDefense() > pMember->GetDefend()->GetNaturalDefense() )
		{
			armorDelta = 1;
		}
		else if ( pMember->GetDefend()->GetDefense() < pMember->GetDefend()->GetNaturalDefense() )
		{
			armorDelta = -1;
		}
	}
	if ( pMember->HasAspect() )
	{
		if ( pMember->GetAspect()->GetMaxMana() > pMember->GetAspect()->GetNaturalMaxMana() )
		{
			manaDelta = 1;
		}
		else if ( pMember->GetAspect()->GetMaxMana() < pMember->GetAspect()->GetNaturalMaxMana() )
		{
			manaDelta = -1;
		}

		if ( pMember->GetAspect()->GetMaxLife() > pMember->GetAspect()->GetNaturalMaxLife() )
		{
			healthDelta = 1;
		}
		else if ( pMember->GetAspect()->GetMaxLife() < pMember->GetAspect()->GetNaturalMaxLife() )
		{
			healthDelta = -1;
		}
	}
	if ( pMember->HasAttack() )
	{
		float damageMin = 0, damageMax = 0; 
		Go * pWeapon = pMember->GetInventory()->GetItem( IL_ACTIVE_MELEE_WEAPON );
		if ( pWeapon )
		{	
			float damageWeaponMin = 0, damageWeaponMax = 0;
			gRules.GetDamageRange( pMember->GetGoid(), pMember->GetGoid(), damageMin, damageMax );
			pWeapon->GetAttack()->EvaluateTotalMinMaxDamage( damageWeaponMin, damageWeaponMax );
			damageWeaponMin += damageMin;
			damageWeaponMax += damageMax;

			gRules.GetDamageRange( pMember->GetGoid(), pWeapon->GetGoid(), damageMin, damageMax );
			
			// Let's subtract any bonuses to the original values here
			damageMin -= pMember->GetAttack()->GetDamageBonusMinMelee();
			damageMax -= pMember->GetAttack()->GetDamageBonusMaxMelee();			
			
			if ( (damageMin + damageMax) < (damageWeaponMin + damageWeaponMax) )
			{
				meleeDamageDelta = 1;
			}
			else if ( (damageMin + damageMax) > (damageWeaponMin + damageWeaponMax) )
			{
				meleeDamageDelta = -1;
			}
		}		
		else
		{
			gRules.GetDamageRange( pMember->GetGoid(), pMember->GetGoid(), damageMin, damageMax );
			
			float damageWeaponMin = 0, damageWeaponMax = 0;

			damageWeaponMin = damageMin;
			damageWeaponMax = damageMax;
			
			// Let's subtract any bonuses to the original values here
			damageMin -= pMember->GetAttack()->GetDamageBonusMinMelee();
			damageMax -= pMember->GetAttack()->GetDamageBonusMaxMelee();		
			
			if ( (damageMin + damageMax) < (damageWeaponMin + damageWeaponMax) )
			{
				meleeDamageDelta = 1;
			}
			else if ( (damageMin + damageMax) > (damageWeaponMin + damageWeaponMax) )
			{
				meleeDamageDelta = -1;
			}
		}

		damageMin = 0.0f;
		damageMax = 0.0f;
		pWeapon = pMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON );
		if ( pWeapon )
		{
			float damageWeaponMin = 0, damageWeaponMax = 0;
			pWeapon->GetAttack()->EvaluateTotalMinMaxDamage( damageWeaponMin, damageWeaponMax );			
			
			gRules.GetDamageRange( pMember->GetGoid(), pWeapon->GetGoid(), damageMin, damageMax );
			
			// Let's subtract any bonuses to the original values here
			damageMin -= pMember->GetAttack()->GetDamageBonusMinRanged();
			damageMax -= pMember->GetAttack()->GetDamageBonusMaxRanged();			
			
			if ( (damageMin + damageMax) < (damageWeaponMin + damageWeaponMax) )
			{
				rangedDamageDelta = 1;
			}
			else if ( (damageMin + damageMax) > (damageWeaponMin + damageWeaponMax) )
			{
				rangedDamageDelta = -1;
			} 		
		}
	}
}

void UIPartyManager::CalculateStatTextColor( int delta, UIText * pText )
{
	if ( delta == 0 )
	{
		pText->SetColor( 0xffffffff );
	}
	else if ( delta == 1 )
	{
		pText->SetColor( m_dwPosTextColor );
	}
	else if ( delta == -1 )
	{
		pText->SetColor( m_dwNegTextColor );
	}
}


// Close all open character menus
bool UIPartyManager::CloseAllCharacterMenus()
{
	bool bClosed = false;
	// All characters share these interface windows, so let's just not draw them
	if ( gUIShell.IsInterfaceVisible( "character" ) )
	{
		gUI.GetGameGUI().HideInterface( "character" );		
		bClosed = true;
	}

	if ( gUIShell.IsInterfaceVisible( "spell" ) )
	{
		gUI.GetGameGUI().HideInterface( "spell" );
		bClosed = true;
	}	

	gUIShell.ShowGroup( "human_inventory", false );
	gUIShell.ShowGroup( "pack_mule_inventory", false );

	// Let's get a list of our party members and hide their gridboxes
	GopColl partyColl;
	Go * pParty = gServer.GetScreenParty();
	if( pParty )
	{
		partyColl = pParty->GetChildren();
	}
	GopColl::iterator i;
	for ( i = partyColl.begin(); i != partyColl.end(); ++i )
	{
		GoHandle hMember( (*i)->GetGoid() );

		if ( !hMember->HasActor() ) 
		{
			continue;
		}

		GoInventory *pInventory = hMember->GetInventory();
		if( pInventory == NULL ) 
		{
			continue;
		}			
		
		if ( pInventory->HasGridbox() && pInventory->GetGridbox()->GetVisible() )
		{
			pInventory->GetGridbox()->SetVisible( false );
			bClosed = true;
		}
	}

	SetInventoriesVisible( false );

	ResetPartyGridScale();

	for ( int o = 0; o < MAX_PARTY_MEMBERS; ++o )
	{
		gpstring sName;
		sName.assignf( "multi_inventory_human_%d", o+1 );
		gUIShell.ShowGroup( sName.c_str(), false );

		sName.assignf( "multi_inventory_human_%d", o+1 );
		gUIShell.ShowGroup( sName.c_str(), false );

		sName.assignf( "multi_inventory_packmule_%d", o+1 );
		gUIShell.ShowGroup( sName.c_str(), false );
		if ( m_UIPartyMembers[o] )
		{
			m_UIPartyMembers[o]->SetPaperDollVisible( false );
		}
	}	

	if ( gUIStoreManager.GetActiveStore() != GOID_INVALID )
	{
		gSiegeEngine.GetCompass().SetVisible( true );
		gUIShell.ShowInterface( "compass_hotpoints" );
		gUIGame.SetBackendMode( BMODE_NONE );
	
		GoHandle hStore( gUIStoreManager.GetActiveStore() );
		if ( gUIStoreManager.GetStoreActive() && hStore.IsValid() && hStore->HasStore() && hStore->GetStore()->IsItemStore() )
		{
			gUIShell.ShiftInterface( "character", 22, 0 );
			gUIShell.ShiftInterface( "inventory", 22, 0 );					
			gUIShell.ShiftInterface( "spell", 22, 0 );					
			hStore->GetStore()->RSRemoveShopper( gUIStoreManager.GetActiveStoreBuyer() );
		}

		gUIStoreManager.SetStoreActive( false );
		gUIStoreManager.SetActiveStore( GOID_INVALID );
		gUIShell.HideInterface( "store" );
		gUIShell.HideInterface( "hire" );
	}

	if ( gUIInventoryManager.GetStashActive() )
	{
		gUIInventoryManager.CloseStash();
	}

	gUIInventoryManager.SetGUIRolloverGOID( GOID_INVALID );

	SetExpertModeActive( false );
	gUICamera.SetXScreenTrack( 0 );

	gUIStoreManager.SetStoreActive( false );

	if ( gUIShell.IsInterfaceVisible( "journal" ) )
	{
		gUIDialogueHandler.ExitJournal();
		bClosed = true;
	}

	if ( gUIShell.IsInterfaceVisible( "lore_book" ) )
	{
		HideLoreBook();
		bClosed = true;
	}	

	SetSingleInventoryVisible( false );

	if ( gUIDialogueHandler.ExitDialogue() )
	{
		bClosed = true;
	}

	if ( gUIShell.IsInterfaceVisible( "world_tip" ) ) 
	{
		gUIMenuManager.CloseTips( true );
		bClosed = true;
	}

	if ( !(m_UIPartyMembers[0] && m_UIPartyMembers[0]->GetGoid() == GOID_INVALID) )
	{
		if ( gUIMenuManager.CloseActiveFadeInterface() )
		{
			bClosed = true;
		}
	}

	gUIInventoryManager.SetGUIRolloverGOID( GOID_INVALID );
	gUIShell.SetRolloverName( "" );	

	if ( gWorldState.GetCurrentState() != WS_SP_NIS )
	{
		gSiegeEngine.GetCompass().SetVisible( true );
		gUIShell.ShowInterface( "compass_hotpoints" );
	}
	
	if ( gUIInventoryManager.GetSourceTrade() && gUIInventoryManager.GetDestTrade() && !gUIInventoryManager.GetIsCompletingTrade() )
	{
		gUIInventoryManager.RSCancelTrade( gUIInventoryManager.GetSourceTrade(), gUIInventoryManager.GetDestTrade() );
		gUIShell.HideInterface( "multiplayer_trade" );
		bClosed = true;
	}

	if ( gUIGame.GetUIPlayerRanks()->IsVisible() )
	{
		gUIGame.GetUIPlayerRanks()->SetVisible( false );
		bClosed = true;
	}	

	if( ::IsMultiPlayer() )
	{
		if ( gUIGame.GetUIEmoteList()->IsVisible() )
		{
			gUIGame.GetUIEmoteList()->SetVisible( false );
			bClosed = true;
		}	
	}

	gUIGameConsole.EnableConsoleUpdate( true );	

	m_openInventories.clear();

	return bClosed;
}



// Bring up selected character's inventory
bool UIPartyManager::ActivateInventory( bool bSelecting )
{
	if ( !gUIGame.IsActive() || gUIMenuManager.IsOptionsMenuActive() ) 
	{
		return false;
	}	

	gUIInventoryManager.SetGUIRolloverGOID( GOID_INVALID );
	m_openInventories.clear();

	POINT row1Pt;
	POINT row2Pt;
	UIWindow * pWindow = gUIShell.FindUIWindow( "window_slots_panel_1", "character_awp" );		

	row1Pt.y = 0;
	row1Pt.x = pWindow->GetRect().right;
	row2Pt.x = row1Pt.x;
	row2Pt.y = (int)((float)gAppModule.GetGameHeight() * 0.520833f);

	const int spacer = 32;

	// If no one is selected, just exit out
	if ( gGoDb.GetSelection().size() == 0 ) 
	{
		return true;
	}

	// This list will hold all the selected party members
	GoidColl SelMemColl;
	GoidColl::iterator iSel;

	// Let's get a list of our party members to see if any of them are currently selected
	GoidColl partyColl;
	GetPortraitOrderedParty( partyColl );
	
	// Put all selected party members into our collection
	GoidColl::const_iterator i;
	GoHandle hMember;
	for ( i = partyColl.begin(); i != partyColl.end(); ++i )
	{
		hMember = *i;
		if ( gUIGame.Contains( (*i), gGoDb.GetSelection() ) && hMember->GetAspect()->GetLifeState() == LS_ALIVE_CONSCIOUS )
		{
			SelMemColl.push_back( (*i) );
		}	
	}

	// Shut down the stash if it is open
	gUIInventoryManager.CloseStash();

	// Abort if no one from the party is selected
	if ( SelMemColl.size() == 0 )
	{
		return true;
	}
	else if ( SelMemColl.size() == 1 )
	{
		int index = 0;
		if ( IsPlayerPartyMember( Goid(*(SelMemColl.begin())), index ) )
		{
			ConstructCharacterInterface( index );
			return true;
		}
	}

	if ( hMember.IsValid() && !m_bActivateInvSound )
	{
		hMember->PlayVoiceSound( "gui_inventory_open", false );
		m_bActivateInvSound = true;
	}

	GoidColl sortedColl = SelMemColl;

	GoHandle hStore( gUIStoreManager.GetActiveStore() );
	if ( hStore.IsValid() )
	{
		gUIStoreManager.DeactivateStore();			
	}

	if ( GetInventoriesVisible() && !bSelecting )
	{
		// Close down anything if it is open
		CloseAllCharacterMenus();	
	
		SetInventoriesVisible( false );
		return true;
	}

	// Close down anything if it is open
	CloseAllCharacterMenus();	
	
	SetInventoriesVisible( true );	

	gSiegeEngine.GetCompass().SetVisible( false );
	gUIShell.HideInterface( "compass_hotpoints" );

	gUIGameConsole.EnableConsoleUpdate( false );

	// Next, try to see if the inventories will fit full sized onto the window.
	{
		unsigned int total_width = row1Pt.x;		
		std::vector< int > widthIndicies;

		int j = 0;
		for ( iSel = sortedColl.begin(); iSel != sortedColl.end(); ++iSel ) 
		{
			GoHandle hMember( *iSel );

			int width = hMember->GetInventory()->GetGridbox()->GetWidth();
			int height = hMember->GetInventory()->GetGridbox()->GetHeight();

			if ( height < GetFullInventoryHeight() ) 
			{
				hMember->GetInventory()->GetGridbox()->SetScale( 1.0 );

				// Set the scale of the inventory headers
				for ( int o = 0; o < MAX_PARTY_MEMBERS; ++o ) 
				{
					if ( m_UIPartyMembers[o] && ( m_UIPartyMembers[o]->GetGoid() == hMember->GetGoid() )) 
					{
						gpstring sName;
						gpstring sWindow;
						if ( hMember->GetInventory()->IsPackOnly() )
						{
							sName.assignf( "multi_inventory_packmule_%d", o+1 );						
							sWindow.assignf( "dialog_box_inv_bg_packmule_%d", o+1 );
						}
						else
						{
							sName.assignf( "multi_inventory_human_%d", o+1 );						
							sWindow.assignf( "dialog_box_inv_bg_human_%d", o+1 );
						}		
						
						gUI.GetGameGUI().SetGroupScaleToAlignWithWindow( sName, sWindow, 2.0 );
					}
				}

				width = hMember->GetInventory()->GetGridbox()->GetWidth();
				height = hMember->GetInventory()->GetGridbox()->GetHeight();
			}

			widthIndicies.push_back( total_width );
			total_width += hMember->GetInventory()->GetGridbox()->GetWidth();
			total_width += spacer;
			++j;
		}

		if ( total_width <= (unsigned int)gAppModule.GetGameWidth() ) 
		{
			// The inventories will all fit full sized, so let's bring them up.
			j = 0;
			for ( iSel = sortedColl.begin(); iSel != sortedColl.end(); ++iSel ) 
			{		
				GoHandle hMember( *iSel );
				for ( int o = 0; o < MAX_PARTY_MEMBERS; ++o ) 
				{
					if ( m_UIPartyMembers[o] && ( m_UIPartyMembers[o]->GetGoid() == hMember->GetGoid() )) 
					{
						if ( gUIShell.IsInterfaceVisible( "multiplayer_trade" ) ) 
						{
							m_UIPartyMembers[o]->GetGUIWindow( GUI_MULTI_GOLD )->SetVisible( true );
						}
						else
						{
							m_UIPartyMembers[o]->GetGUIWindow( GUI_MULTI_GOLD )->SetVisible( false );
						}

						GoHandle hMember( (*iSel) );
						gpstring sSpecies;
						if ( hMember->GetInventory()->IsPackOnly() )
						{
							sSpecies = "packmule";
						}
						else
						{
							sSpecies = "human";
						}

						gpstring sName;
						sName.assignf( "multi_inventory_%s_%d", sSpecies.c_str(), o+1 );
						gUIShell.ShowGroup( sName, true );
						
						gpstring sBg;
						sBg.assignf( "dialog_box_inv_bg_%s_%d", sSpecies.c_str(), o+1 );
						UIWindow * pDialog = gUIShell.FindUIWindow( sBg, "multi_inventory" );
						if ( pDialog )
						{
							gUIShell.ShiftGroup( "multi_inventory", sName, (row1Pt.x-pDialog->GetRect().left) + (widthIndicies[j]-row1Pt.x), row1Pt.y-pDialog->GetRect().top );
						}

						gpstring sGridPlace;
						sGridPlace.assignf( "grid_place_%s_%d", sSpecies.c_str(), o+1 );
						UIWindow * pGridPlace = gUIShell.FindUIWindow( sGridPlace, "multi_inventory" );

						int xOffsetError = 0;
						int yOffsetError = 0;
						if ( !hMember->GetInventory()->IsPackOnly() )
						{
							UIWindow * pHumanGrid = gUIShell.FindUIWindow( "gridbox_13x4", "inventory" );
							xOffsetError = (pHumanGrid->GetRect().right-pHumanGrid->GetRect().left) - 
										   (pGridPlace->GetRect().right-pGridPlace->GetRect().left);
							yOffsetError = (pHumanGrid->GetRect().bottom-pHumanGrid->GetRect().top) - 
										   (pGridPlace->GetRect().bottom-pGridPlace->GetRect().top);
						}

						hMember->GetInventory()->GetGridbox()->SetRect(	pGridPlace->GetRect().left, pGridPlace->GetRect().right+xOffsetError,
																		pGridPlace->GetRect().top, pGridPlace->GetRect().bottom+yOffsetError );

						hMember->GetInventory()->GetGridbox()->SetVisible( true );
						m_openInventories.push_back( hMember->GetGoid() );										
					}
				}

				++j;
			}

			// We succeeding in displaying the available inventories full size, our work here is done.
			return true;
		}		
	}

	// If the last attempt failed to display the inventories, let's show all we can using the
	// scaled inventory size.
	{
		unsigned int total_width = row1Pt.x;
		unsigned int total_height = row1Pt.y;
		int j = 0;

		for ( iSel = sortedColl.begin(); iSel != sortedColl.end(); ++iSel ) 
		{
			GoHandle hMember( (*iSel) );

			int width = hMember->GetInventory()->GetGridbox()->GetWidth()/2;			
			int height = hMember->GetInventory()->GetGridbox()->GetHeight()/2;

			// If we're over the screen width and past our second row, we can't fit any more
			if (((int)((total_width + width)) > gAppModule.GetGameWidth()) &&
				((int)total_height > row1Pt.y)) 
			{
				return true;
			}

			// If we've exceeded our limit on our row, let's go the next available		
			else if ( (total_width + width) > (unsigned int)gAppModule.GetGameWidth() ) 
			{
				total_height = row2Pt.y;
				total_width = row2Pt.x;
			}

			// Now let's officially set the scale and rectangle of the gridbox
			int test_height = hMember->GetInventory()->GetGridbox()->GetHeight();
			bool bChangeBars = false;
			if ( test_height < (GetFullInventoryHeight()/2) ) 
			{
				width = hMember->GetInventory()->GetGridbox()->GetWidth();
				height = hMember->GetInventory()->GetGridbox()->GetHeight();
			}
			else 
			{
				hMember->GetInventory()->GetGridbox()->SetScale( 0.5 );
				bChangeBars = true;
			}
	
			int currentLeft = total_width;
			total_width += (hMember->GetInventory()->GetGridbox()->GetWidth());

			// Change to align just on the appropriate inventory
			for ( int o = 0; o < MAX_PARTY_MEMBERS; ++o ) 
			{
				if ( m_UIPartyMembers[o] && ( m_UIPartyMembers[o]->GetGoid() == hMember->GetGoid() )) 
				{
					if ( gUIShell.IsInterfaceVisible( "multiplayer_trade" ) ) 
					{
						m_UIPartyMembers[o]->GetGUIWindow( GUI_MULTI_GOLD )->SetVisible( true );
					}
					else
					{
						m_UIPartyMembers[o]->GetGUIWindow( GUI_MULTI_GOLD )->SetVisible( false );
					}				

					gpstring sName;
					gpstring sWindow;
					if ( hMember->GetInventory()->IsPackOnly() )
					{
						sName.assignf( "multi_inventory_packmule_%d", o+1 );						
						sWindow.assignf( "dialog_box_inv_bg_packmule_%d", o+1 );
					}
					else
					{
						sName.assignf( "multi_inventory_human_%d", o+1 );						
						sWindow.assignf( "dialog_box_inv_bg_human_%d", o+1 );
					}						

					gUI.GetGameGUI().ShowGroup( sName, true );

					if ( bChangeBars ) 
					{
						gUI.GetGameGUI().SetGroupScaleToAlignWithWindow( sName, sWindow, 0.5 );
					}	
										
					gpstring sSpecies;
					if ( hMember->GetInventory()->IsPackOnly() )
					{
						sSpecies = "packmule";
					}
					else
					{
						sSpecies = "human";
					}
					
					gpstring sBg;
					sBg.assignf( "dialog_box_inv_bg_%s_%d", sSpecies.c_str(), o+1 );
					UIWindow * pDialog = gUIShell.FindUIWindow( sBg, "multi_inventory" );
					if ( pDialog )
					{
						gUIShell.ShiftGroup( "multi_inventory", sName, (row1Pt.x-pDialog->GetRect().left) + (currentLeft-row1Pt.x), (int)total_height - pDialog->GetRect().top );
					}

					gpstring sGridPlace;
					sGridPlace.assignf( "grid_place_%s_%d", sSpecies.c_str(), o+1 );
					UIWindow * pGridPlace = gUIShell.FindUIWindow( sGridPlace, "multi_inventory" );
					hMember->GetInventory()->GetGridbox()->SetRect(	pGridPlace->GetRect().left, pGridPlace->GetRect().right,
																	pGridPlace->GetRect().top, pGridPlace->GetRect().bottom );
					hMember->GetInventory()->GetGridbox()->SetVisible( true );

					m_openInventories.push_back( hMember->GetGoid() );				
				}
			}
			total_width += spacer/2;
			++j;
		}
	}
	return true;
}


void UIPartyManager::ActivateSingleInventory( Go * pMember )
{
	if ( !gUIGame.IsActive() ) 
	{
		return;
	}

	gUIInventoryManager.SetGUIRolloverGOID( GOID_INVALID );

	POINT row1Pt;
	UIWindow * pWindow = gUIShell.FindUIWindow( "window_slots_panel_1", "character_awp" );		
	UIWindow * pCharPane = gUIShell.FindUIWindow( "character_pane_1", "character" );
	row1Pt.y = pCharPane->GetRect().top;
	row1Pt.x = pWindow->GetRect().right;

	// Close down anything if it is open
	CloseAllCharacterMenus();
	
	// Set the scale of the inventory headers
	for ( int o = 0; o < MAX_PARTY_MEMBERS; ++o ) 
	{
		if ( m_UIPartyMembers[o] && ( m_UIPartyMembers[o]->GetGoid() == pMember->GetGoid() )) 
		{
			gpstring sName;
			gpstring sWindow;
			if ( pMember->GetInventory()->IsPackOnly() )
			{
				sName.assignf( "multi_inventory_packmule_%d", o+1 );						
				sWindow.assignf( "dialog_box_inv_bg_packmule_%d", o+1 );
			}
			else
			{
				sName.assignf( "multi_inventory_human_%d", o+1 );						
				sWindow.assignf( "dialog_box_inv_bg_human_%d", o+1 );
			}	
			
			int height = pMember->GetInventory()->GetGridbox()->GetHeight();
			if ( height < GetFullInventoryHeight() ) 
			{
				pMember->GetInventory()->GetGridbox()->SetScale( 1.0 );				
				gUI.GetGameGUI().SetGroupScaleToAlignWithWindow( sName, sWindow, 2.0 );
				gUI.GetGameGUI().SetGroupScaleToAlignWithWindow( "multi_inventory_human_1_gold", sWindow, 2.0 );
			}

			gpstring sSpecies;
			if ( pMember->GetInventory()->IsPackOnly() )
			{
				sSpecies = "packmule";
			}
			else
			{
				sSpecies = "human";
			}				

			gUIShell.ShowGroup( sName, true );
			
			gpstring sBg;
			sBg.assignf( "dialog_box_inv_bg_%s_%d", sSpecies.c_str(), o+1 );
			UIWindow * pDialog = gUIShell.FindUIWindow( sBg, "multi_inventory" );
			if ( pDialog )
			{
				gUIShell.ShiftGroup( "multi_inventory", sName, (row1Pt.x-pDialog->GetRect().left), row1Pt.y-pDialog->GetRect().top );
				gUIShell.ShiftGroup( "multi_inventory", "multi_inventory_human_1_gold", (row1Pt.x-pDialog->GetRect().left), row1Pt.y-pDialog->GetRect().top );
			}

			gpstring sGridPlace;
			sGridPlace.assignf( "grid_place_%s_%d", sSpecies.c_str(), o+1 );
			UIWindow * pGridPlace = gUIShell.FindUIWindow( sGridPlace, "multi_inventory" );
			
			int xOffsetError = 0;
			int yOffsetError = 0;
			if ( !pMember->GetInventory()->IsPackOnly() )
			{
				UIWindow * pHumanGrid = gUIShell.FindUIWindow( "gridbox_13x4", "inventory" );
				xOffsetError = (pHumanGrid->GetRect().right-pHumanGrid->GetRect().left) - 
							   (pGridPlace->GetRect().right-pGridPlace->GetRect().left);
				yOffsetError = (pHumanGrid->GetRect().bottom-pHumanGrid->GetRect().top) - 
							   (pGridPlace->GetRect().bottom-pGridPlace->GetRect().top);
			}

			pMember->GetInventory()->GetGridbox()->SetRect(	pGridPlace->GetRect().left, pGridPlace->GetRect().right+xOffsetError,
															pGridPlace->GetRect().top, pGridPlace->GetRect().bottom+yOffsetError );
			pMember->GetInventory()->GetGridbox()->SetVisible( true );
			
			gpstring sGold;
			sGold.assignf( "button_gold_%s_%d", sSpecies.c_str(), o+1 );
			UIButton * pGold = (UIButton *)gUIShell.FindUIWindow( sGold, "multi_inventory" );
			if ( pGold )
			{
				pGold->EnableButton();
			}

			SetSingleInventoryVisible( true );
		}
	}		
}


// Bring up selected character's character stats
bool UIPartyManager::ActivateCharacterStats()
{
	if ( gUIGame.IsActive() && !gUIMenuManager.IsOptionsMenuActive() ) {
		for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i ) {
			GoHandle hGO( m_UIPartyMembers[i]->GetGoid() );
			if ( gUI.GetGameGUI().IsInterfaceVisible( "character" ) )
			{
				GoHandle hMember( m_UIPartyMembers[i]->GetGoid() );
				if ( hMember.IsValid() && IsAlive( hMember->GetLifeState() ) )
				{
					ConstructCharacterInterface( m_SelectedMember );
				}
				return true;
			}
			if ( hGO.IsValid() && gGoDb.IsSelected( hGO->GetGoid() ) )
			{
				ConstructCharacterInterface( i );
				return true;
			}
		}
	}
	return true;
}


// Bring up selected character's spell book
bool UIPartyManager::ActivateSpellBook()
{
	if ( gUIGame.IsActive() && !gUIMenuManager.IsOptionsMenuActive() ) 
	{
		for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i ) 
		{
			GoHandle hGO( m_UIPartyMembers[i]->GetGoid() );
			if ( hGO.IsValid() && gGoDb.IsSelected( hGO->GetGoid() ) )
			{				
				if ( gUI.GetGameGUI().IsInterfaceVisible( "character" ) )
				{					
					hGO->GetInventory()->SetActiveSpellBook( hGO->GetInventory()->GetEquipped( ES_SPELLBOOK ) );
					if ( SetSpellsActive( !m_UIPartyMembers[m_SelectedMember]->IsSpellsExpanded() ) )
					{
						hGO->PlayVoiceSound( "gui_spellbook_open", false );	
					}
				}				
			}
		}
	}
	return true;
}

void UIPartyManager :: RenameSelectedSpellBook( const gpwstring & customName )
{
	GoHandle hMember( m_UIPartyMembers[m_SelectedMember]->GetGoid()	);
	if ( hMember )
	{
		Go * pSpellBook = hMember->GetInventory()->GetActiveSpellBook();
		if ( pSpellBook )
		{
			pSpellBook->GetCommon()->SetCustomName( customName );
		}
	}
}


gpstring UIPartyManager::GetActiveIconFromGO( Go * pGO )
{
	if ( pGO->GetGui()->GetActiveIcon().empty() )
	{
		return "none";
	}

	gpstring filename;

	if (!gNamingKey.BuildIMGLocation(pGO->GetGui()->GetActiveIcon().c_str(), filename) || !FileSys::DoesResourceFileExist( filename.c_str() ) ) {
		// Perhaps we should load a 'special' icon to indicate failure? --biddle
		gpwarningf(("Failed to locate Active Icon texture %s", pGO->GetGui()->GetActiveIcon().c_str()));
		gNamingKey.BuildIMGLocation("b_i_glb_placeholder", filename);
	}
	return filename;
}


void UIPartyManager::DrawStatusBars()
{
	if ( m_UIPartyMembers[0] && m_UIPartyMembers[0]->GetGoid() )
	{
		GoHandle hMember( m_UIPartyMembers[0]->GetGoid() );
		Goid object = MakeGoid(gSiegeEngine.GetMouseShadow().GetHit());

		for ( int i = 0; i != MAX_PARTY_MEMBERS; ++i )
		{
			GoHandle hPartyMember( m_UIPartyMembers[i]->GetGoid() );
			if ( hPartyMember.IsValid() &&
				 (hPartyMember->IsSelected() || hPartyMember->GetGoid() == object) &&
				 hPartyMember->GetAspect()->GetIsVisible() )
			{
				DrawMemberStatusBars( i );				
			}
			else if ( hPartyMember.IsValid() )
			{
				m_UIPartyMembers[i]->GetGUIWindow( GUI_HEALTH_FLOAT )->SetVisible( false );
				m_UIPartyMembers[i]->GetGUIWindow( GUI_MANA_FLOAT )->SetVisible( false );			
			}
		}
		
		if ( object != GOID_INVALID )
		{
			GoHandle hObject( object );
			if( hObject.IsValid() && hMember.IsValid() ) {
				if ( hObject->HasActor() && hMember->GetMind()->IsEnemy( hObject ) )
				{
					DrawEnemyStatusBars( hObject );
				}				
			}
		}
	}
}


/**************************************************************************

Function		: UpdateInfoBox()

Purpose			: Update the information box

Returns			: void

**************************************************************************/
void UIPartyManager::UpdateInfoBox()
{
	GPPROFILERSAMPLE( "UIPartyManager :: UpdateInfoBox", SP_UI );

	// Close dead or unconscious peoples' interfaces
	bool bConsciousMember = false;
	for ( int x = 0; x < MAX_PARTY_MEMBERS; ++x )
	{
		if ( m_UIPartyMembers[x] && ( m_UIPartyMembers[x]->GetGoid() != GOID_INVALID ))
		{
			GoHandle hMember( m_UIPartyMembers[x]->GetGoid() );
			if (( hMember.IsValid() && (hMember->GetLifeState() != LS_ALIVE_CONSCIOUS )) )
			{
				if ( m_UIPartyMembers[x]->IsCharacterExpanded() )
				{
					if ( gUI.GetGameGUI().IsInterfaceVisible( "character" ) && ( x == m_SelectedMember ) )
					{
						CloseAllCharacterMenus();						
					}
				}
				
				if ( ::IsMultiPlayer() && hMember->GetLifeState() != LS_GHOST )
				{
					if ( gUIMenuManager.GetMultiRespawnOption() == false )
					{
						gUIMenuManager.SetMultiRespawnOption( true );
					}
				}
				else if ( gUIMenuManager.GetMultiRespawnOption() && hMember->GetLifeState() == LS_GHOST )
				{
					if ( gUIMenuManager.GetMultiRespawnOption() == true )
					{
						gUIMenuManager.SetMultiRespawnOption( false );
					}
				}
			}
			else 
			{
				if ( gUIMenuManager.GetMultiRespawnOption() != false )
				{
					gUIMenuManager.SetMultiRespawnOption( false );
				}
				bConsciousMember = true;
			}
		}
	}	

	Goid object = MakeGoid(gSiegeEngine.GetMouseShadow().GetHit());

	Goid temp = gUIGame.GetUIItemOverlay()->GetHoverItem();
	if ( temp != GOID_INVALID )
	{
		object = temp;
	}

	if ( gSiegeEngine.GetMouseShadow().GetGlobalHitColl().size() > 1 )
	{
		siege::SiegeMouseShadow::HitColl hitColl = gSiegeEngine.GetMouseShadow().GetGlobalHitColl();
		siege::SiegeMouseShadow::HitColl::iterator iHit;
		for ( iHit = hitColl.begin(); iHit != hitColl.end(); ++iHit )
		{
			GoHandle hHit( MakeGoid( *iHit ) );
			if ( hHit.IsValid() && hHit->IsScreenPartyMember() && !::IsAlive( hHit->GetAspect()->GetLifeState() ) )
			{
				object = hHit->GetGoid();
				break;
			}
		}
	}

	if ( gUI.GetGameGUI().GetRolloverConsumed() )
	{
		object = GOID_INVALID;
	}

	GoHandle hMember;
	for ( int o = 0; o < MAX_PARTY_MEMBERS; ++o )
	{
		if ( m_UIPartyMembers[o] && ( m_UIPartyMembers[o]->GetGoid() != GOID_INVALID ))
		{
			hMember = m_UIPartyMembers[o]->GetGoid();
			if ( hMember.IsValid() )
			{
				if ( gUIGame.Contains( hMember->GetGoid(), gGoDb.GetSelection() ) )
				{
					break;
				}
			}
			else
			{
				gperror( "Member is not valid, please let Chad know." );
			}
		}
	}

	if ( m_dataBarVariation != 0 )
	{
		UITextBox * pText = ((UITextBox *)m_pWindow[TEXT_INFORMATION]);
		UIDockbar * pDataBar = (UIDockbar *)gUIShell.FindUIWindow( "data_bar", "data_bar" );	
		if ( pDataBar->GetDockLocation() == DL_TOP )
		{
			pText->SetRect( pText->GetRect().left, pText->GetRect().right, pText->GetRect().top, pText->GetRect().bottom-m_dataBarVariation );										
			pDataBar->SetRect( pDataBar->GetRect().left, pDataBar->GetRect().right, pDataBar->GetRect().top, pDataBar->GetRect().bottom-m_dataBarVariation );
		}
		else
		{
			pText->SetRect( pText->GetRect().left, pText->GetRect().right, pText->GetRect().top+m_dataBarVariation, pText->GetRect().bottom );										
			pDataBar->SetRect( pDataBar->GetRect().left, pDataBar->GetRect().right, pDataBar->GetRect().top+m_dataBarVariation, pDataBar->GetRect().bottom );
		}		
		m_dataBarVariation = 0;
	}

	if ( !hMember.IsValid() )
	{
		return;
	}

	if ( GetRolloverItem() != object && GetRolloverItem() != GOID_INVALID )
	{
		SetRolloverItem( GOID_INVALID );
		SetRolloverTimer( 0.0f );
		UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "gui_rollover_textbox" );		
		pTextBox->SetVisible( false );
	}

	if (( object == GOID_INVALID ) && ( gUIInventoryManager.GetGUIRolloverGOID() == GOID_INVALID ))
	{
		gpstring sName = gUIShell.GetRolloverName();
		if ( !sName.empty() )		
		{
			gpwstring sLine = RetrieveGuiUserEdTooltip( sName );
			if ( !sLine.empty() )
			{
				((UITextBox *)m_pWindow[TEXT_INFORMATION])->SetText( sLine );
				m_pWindow[ENEMY_HEALTH_BAR]->SetVisible( false );	
				
				if ( sName.same_no_case( "button_gold" ) )
				{
					gpwstring sLine2;
					sLine2.assignf( gpwtranslate( $MSG$ "Gold: %d" ), hMember->GetInventory()->GetGold() );
					((UITextBox *)m_pWindow[TEXT_INFORMATION])->SetLineText( 1, sLine2 );
				}

				return;
			}
		}

		((UITextBox *)m_pWindow[TEXT_INFORMATION])->SetText( L"" );
		m_pWindow[ENEMY_HEALTH_BAR]->SetVisible( false );		
		return;
	}	

	GoHandle pObject;
	if ( gUIInventoryManager.GetGUIRolloverGOID() != GOID_INVALID )
	{
		if ( gUIShell.GetRolloverItemslotID() == 0 && gUIShell.GetRolloverName().empty() )
		{
			((UITextBox *)m_pWindow[TEXT_INFORMATION])->SetText( L"" );
			m_pWindow[ENEMY_HEALTH_BAR]->SetVisible( false );			
			gUIInventoryManager.SetGUIRolloverGOID( GOID_INVALID );
			return;
		}
		pObject = gUIInventoryManager.GetGUIRolloverGOID();
	}
	else
	{
		pObject = object;
	}

	if ( gUIMenuManager.GetModalActive() )
	{
		return;
	}

	gpwstring sTemp;	

	if( pObject.IsValid() )
	{
		// We don't care.
		if ( !pObject->HasActor() && !pObject->HasGui() )
		{
			if ( !pObject->GetCommon()->GetDisplayRollover() )
			{
				return;
			}
		}

		UITextBox * pText = ((UITextBox *)m_pWindow[TEXT_INFORMATION]);
		gpwstring sLine2;

		bool bEnemy = hMember->GetMind()->IsEnemy( pObject );
		if ( pObject->HasActor() && hMember.IsValid() && !pObject->IsScreenPartyMember() && ( bEnemy || pObject->GetActor()->GetCanShowHealth() ) )
		{
			// It's a valid enemy, RUN!!!
			if ( bEnemy )
			{
				DrawEnemyStatusBars( pObject );
			}

			gpwstring sLine;
						
			int current_life = fix_precision( pObject->GetAspect()->GetCurrentLife() );
			if( (current_life > 0) && (current_life <1) )
			{
				current_life = 1;
			}

			current_life = ( current_life < 0 ) ? 0 : current_life;
			int max_life = fix_precision( pObject->GetAspect()->GetMaxLife() );

			switch( pObject->GetAspect()->GetLifeState() )
			{
				case LS_ALIVE_UNCONSCIOUS:
					sLine.assignf( gpwtranslate( $MSG$ "<c:0x%x>Unconscious %s: Health %d/%d</c>" ), gUIShell.GetTipColor( "rollover_unconscious" ), pObject->GetCommon()->GetScreenName().c_str(), current_life, max_life );
					break;
				case LS_DEAD_CHARRED:				
				case LS_DEAD_NORMAL:
					sLine.assignf( gpwtranslate( $MSG$ "<c:0x%x>Dead %s: Health %d/%d</c>" ), gUIShell.GetTipColor( "rollover_dead" ), pObject->GetCommon()->GetScreenName().c_str(), current_life, max_life );
					break;
				case LS_GHOST:
					sLine.assignf( gpwtranslate( $MSG$ "<c:0x%x>Ghost %s: Health %d/%d</c>" ), gUIShell.GetTipColor( "rollover_ghost" ), pObject->GetCommon()->GetScreenName().c_str(), current_life, max_life );
					break;
				default:					
					sLine.assignf( gpwtranslate( $MSG$ "%s: Health %d/%d" ), pObject->GetCommon()->GetScreenName().c_str(), current_life, max_life );
					break;
			}
			
			pText->SetText( sLine );

			if ( pObject->HasActor() )
			{					
				gpwstring sGeneric;				
				pObject->GetActor()->GetGenericStateInfo( sGeneric, GetMaxGenericStateDisplay() );				

				gpwstring sFullText;
				sFullText.assignf( L"%s\n%s", sLine.c_str(), sGeneric.c_str() );				
				
				UIDockbar * pDataBar = (UIDockbar *)gUIShell.FindUIWindow( "data_bar", "data_bar" );				
				while ( !pText->DoesTextFit( sFullText ) )				
				{					
					m_dataBarVariation += pText->GetFontHeight();

					if ( pDataBar->GetDockLocation() == DL_TOP )
					{
						pText->SetRect( pText->GetRect().left, pText->GetRect().right, pText->GetRect().top, pText->GetRect().bottom+pText->GetFontHeight() );										
						pDataBar->SetRect( pDataBar->GetRect().left, pDataBar->GetRect().right, pDataBar->GetRect().top, pDataBar->GetRect().bottom+pText->GetFontHeight() );
					}
					else
					{
						pText->SetRect( pText->GetRect().left, pText->GetRect().right, pText->GetRect().top-pText->GetFontHeight(), pText->GetRect().bottom );										
						pDataBar->SetRect( pDataBar->GetRect().left, pDataBar->GetRect().right, pDataBar->GetRect().top-pText->GetFontHeight(), pDataBar->GetRect().bottom );
					}
				}

				if ( sLine2.empty() && GetRolloverAttack() )
				{					
					sLine2 = RetrieveItemUserEdTooltip( "left_creature" );					
				}

				if ( !sGeneric.empty() )
				{
					if ( !sLine2.empty() )
					{
						sLine2.assignf( L"%s : %s", sLine2.c_str(), sGeneric.c_str() );
					}
					else
					{
						sLine2 = sGeneric;
					}
				}				
			}
			
			if ( GetRolloverCast() )
			{	
				GoHandle hRolloverObject( GetRolloverCastObject() );
				GoHandle hRolloverSpell( GetRolloverCastSpell() );
				GoHandle hRolloverMember( GetRolloverCastMember() );
				if ( hRolloverObject && hRolloverSpell && hRolloverMember )
				{
					if ( !sLine2.empty() )
					{
						sLine2.appendf( gpwtranslate( $MSG$ " : <c:0x%x>%s, Mana Cost: %0.0f</c>" ), gUIShell.GetTipColor( "rollover_cast" ),	hRolloverSpell->GetCommon()->GetScreenName().c_str(),
																	hRolloverSpell->GetMagic()->EvaluateManaCost( hRolloverMember, hRolloverObject ) );
					}
					else
					{
						sLine2.appendf( gpwtranslate( $MSG$ "<c:0x%x>%s, Mana Cost: %0.0f</c>" ), gUIShell.GetTipColor( "rollover_cast" ),	hRolloverSpell->GetCommon()->GetScreenName().c_str(),
																	hRolloverSpell->GetMagic()->EvaluateManaCost( hRolloverMember, hRolloverObject ) );					
					}
				}
			}

			if ( !sLine2.empty() )
			{
				pText->SetLineText( 1, sLine2 );
			}

			return;
		}
		else
		{
			m_pWindow[ENEMY_HEALTH_BAR]->SetVisible( false );			
		}

		gpwstring sName;
		if ( gUIGame.GetInfoMode().same_no_case( "screen_name" ) )
		{			
			sName = pObject->GetCommon()->GetScreenName();

			bool bSet = false;
			if ( pObject->HasGui() && pObject->HasMagic() && pObject->GetMagic()->HasNonInnateEnchantments() && IsEquippableSlot( pObject->GetGui()->GetEquipSlot() ) )
			{		
				gpwstring sTranslated = pObject->GetCommon()->GetTemplateScreenName();
				sName = sTranslated;
				if ( !sTranslated.same_no_case( pObject->GetCommon()->GetScreenName() ) )
				{
					if ( !pObject->GetGui()->GetIsIdentified() )
					{
						sName = sTranslated;
						bSet = true;
					}
					else
					{
						sName = pObject->GetCommon()->GetScreenName();
					}
				}							
			}	

			if ( sName.empty() )
			{
				sName = ToUnicode( pObject->GetTemplateName() );		// fallback when they forget screen name
			}
		}
		else if ( gUIGame.GetInfoMode().same_no_case( "block_name" ) )
		{
			sName = ToUnicode( pObject->GetTemplateName() );
		}
	
		if ( pObject->IsScreenPartyMember() || ( !gWorld.IsSinglePlayer() && pObject->GetPlayer()->GetController() != PC_COMPUTER && hMember->GetMind()->IsFriend( pObject ) ) ) 
		{			
			const int current_life	= ( fix_precision( pObject->GetAspect()->GetCurrentLife() ) < 0 ) ? 0 : fix_precision( pObject->GetAspect()->GetCurrentLife() );
			const int max_life		= fix_precision( pObject->GetAspect()->GetMaxLife() );
			const int current_mana	= fix_precision(pObject->GetAspect()->GetCurrentMana());
			const int max_mana		= fix_precision(pObject->GetAspect()->GetMaxMana());

			switch( pObject->GetAspect()->GetLifeState() )
			{
				case LS_ALIVE_UNCONSCIOUS:
					sTemp.assignf( gpwtranslate( $MSG$ "<c:0x%x>Unconscious %s: Health %d/%d, Mana %d/%d</c>" ), gUIShell.GetTipColor( "rollover_unconscious" ), sName.c_str(), current_life, max_life,	current_mana, max_mana );
					break;
				case LS_DEAD_CHARRED:
				case LS_DEAD_NORMAL:
					sTemp.assignf( gpwtranslate( $MSG$ "<c:0x%x>Dead %s: Health %d/%d, Mana %d/%d</c>" ), gUIShell.GetTipColor( "rollover_dead" ), sName.c_str(), current_life, max_life, current_mana, max_mana );
					break;
				case LS_GHOST:
					sTemp.assignf( gpwtranslate( $MSG$ "<c:0x%x>Ghost %s: Health %d/%d</c>" ), gUIShell.GetTipColor( "rollover_ghost" ), pObject->GetCommon()->GetScreenName().c_str(), current_life, max_life );
					break;
				default:
					sTemp.assignf( gpwtranslate( $MSG$ "%s: Health %d/%d, Mana %d/%d" ), sName.c_str(), current_life, max_life, current_mana, max_mana );			
					break;
			}
			
			if ( pObject->HasActor() )
			{				
				pObject->GetActor()->GetGenericStateInfo( sLine2, GetMaxGenericStateDisplay() );
				gpwstring sFullText;
				sFullText.assignf( L"%s\n%s", sName.c_str(), sLine2.c_str() );				
				
				UIDockbar * pDataBar = (UIDockbar *)gUIShell.FindUIWindow( "data_bar", "data_bar" );			
				while ( !pText->DoesTextFit( sFullText ) )				
				{
					m_dataBarVariation += pText->GetFontHeight();

					if ( pDataBar->GetDockLocation() == DL_TOP )
					{
						pText->SetRect( pText->GetRect().left, pText->GetRect().right, pText->GetRect().top, pText->GetRect().bottom+pText->GetFontHeight() );										
						pDataBar->SetRect( pDataBar->GetRect().left, pDataBar->GetRect().right, pDataBar->GetRect().top, pDataBar->GetRect().bottom+pText->GetFontHeight() );
					}
					else
					{
						pText->SetRect( pText->GetRect().left, pText->GetRect().right, pText->GetRect().top-pText->GetFontHeight(), pText->GetRect().bottom );										
						pDataBar->SetRect( pDataBar->GetRect().left, pDataBar->GetRect().right, pDataBar->GetRect().top-pText->GetFontHeight(), pDataBar->GetRect().bottom );
					}
				}
			}

			if ( sLine2.empty() )
			{
				gpstring sWindow = gUIShell.GetRolloverName();
				if ( !sWindow.empty() )		
				{
					sLine2 = RetrieveItemUserEdTooltip( sWindow );				
				}					
			}
		}
		else if ( pObject->IsGold() )
		{
			sTemp.assignf( gpwtranslate( $MSG$ "%s, Value: %d" ), sName.c_str(), pObject->GetAspect()->GetGoldValue() );
		}
		else 
		{
			if ( !pObject->HasParent() && pObject->IsItem() && GetRolloverItem() == GOID_INVALID )
			{
				SetRolloverTimer( gUIShell.GetRolloverDelay() );
				SetRolloverItem( pObject->GetGoid() );
			}

			bool bContinue = true;
			sTemp = sName;
			gpstring sWindow = gUIShell.GetRolloverName();
			if ( !sWindow.empty() )		
			{
				if ( sWindow.compare_no_case( "awp_character_slot_", sWindow.size()-1 ) == 0 )
				{
					GoHandle hEquipped( gUIInventoryManager.GetGUIRolloverGOID() );
					if ( hEquipped.IsValid() )
					{
						Goid owner = gUIGame.GetActorWhoCarriesObject( hEquipped->GetGoid() );
						if ( owner == GOID_INVALID && hEquipped->HasParent() )
						{
							owner = gUIGame.GetActorWhoCarriesObject( hEquipped->GetParent()->GetGoid() );
						}
						GoHandle hOwner( owner );
						if ( hOwner.IsValid() )
						{
							if ( hOwner->GetInventory()->GetSelectedItem() &&
								 hOwner->GetInventory()->GetSelectedItem()->GetGoid() == hEquipped->GetGoid() )
							{
								if ( hOwner->GetInventory()->GetSelectedItem()->IsSpell() )
								{
									sWindow = "active_item_spell";
								}
								else
								{
									sWindow = "active_item_weapon";
								}
							}
						}
					}
				}

				sLine2 = RetrieveItemUserEdTooltip( sWindow );
				if ( !sLine2.empty() )
				{					
					bContinue = false;
				}
			}			
			
			if ( bContinue && bConsciousMember )
			{
				if ( !pObject->HasActor() && pObject->GetCommon()->GetDisplayRolloverLife() && pObject->HasAspect() )
				{
					const int current_life	= ( fix_precision( pObject->GetAspect()->GetCurrentLife() ) < 0 ) ? 0 : fix_precision( pObject->GetAspect()->GetCurrentLife() );
					const int max_life		= fix_precision( pObject->GetAspect()->GetMaxLife() );
					sTemp.appendf( gpwtranslate( $MSG$ ": Health %d/%d" ), current_life, max_life );
				}

				if ( pObject->HasActor() )
				{					
					pObject->GetActor()->GetGenericStateInfo( sLine2 );					
				}
				if ( sLine2.empty() )
				{
					if ( !pObject->GetCommon()->GetRolloverHelpKey().empty() )
					{						
						if ( ::IsMultiPlayer() )
						{
							gpstring sKeyMp = pObject->GetCommon()->GetRolloverHelpKey() + "_mp";
							sLine2 = RetrieveItemUserEdTooltip( sKeyMp );
						}

						if ( sLine2.empty() )
						{
							sLine2 = RetrieveItemUserEdTooltip( pObject->GetCommon()->GetRolloverHelpKey() );
						}
					}
					else if ( pObject->HasGui() && pObject->GetGui()->IsLoreBook() )
					{
						sLine2 = RetrieveItemUserEdTooltip( "lore_book" );					
					}
					else if ( pObject->IsBreakable() && !pObject->HasActor() )
					{	
						if ( gUIGame.GetUICommands()->GetActiveContextCursor() == CURSOR_GUI_SMASH )
						{						
							sLine2 = RetrieveItemUserEdTooltip( "left_breakable" );
						}
						else
						{
							sLine2 = RetrieveItemUserEdTooltip( "cant_break" );
						}
					}
					else if ( pObject->IsUsable() )
					{
						sLine2 = RetrieveItemUserEdTooltip( "left_usable" );						
					}						
					else if ( GetRolloverGet() )
					{						
						sLine2 = RetrieveItemUserEdTooltip( "left_ground_item" );
					}
					else if ( hMember->GetInventory()->IsEquipped( pObject ) )
					{
						if ( pObject->HasGui() && pObject->GetGui()->IsSpellBook() )
						{
							sLine2 = RetrieveItemUserEdTooltip( "item_paperdoll_spellbook_rollover" );
						}
						else
						{
							sLine2 = RetrieveItemUserEdTooltip( "item_paperdoll_rollover" );
						}
					}
					else if ( pObject->IsSpellBook() )
					{
						sLine2 = RetrieveItemUserEdTooltip( "item_spellbook" );
					}
					else if ( pObject->IsItem() && hMember->GetInventory()->Contains( pObject ) && hMember->GetInventory()->GetLocation( pObject ) == IL_MAIN )
					{
						if ( IsEquippableSlot( pObject->GetGui()->GetEquipSlot() ) )
						{
							sLine2 = RetrieveItemUserEdTooltip( "item_equippable" );				
						}
						else
						{
							if ( pObject->IsSpell() )
							{
								sLine2 = RetrieveItemUserEdTooltip( "item_spell_inv" );
							}
							else if ( pObject->IsPotion() )
							{
								sLine2 = RetrieveItemUserEdTooltip( "potion" );
							}
						}			
					}			
					else if ( pObject->IsSpell() && pObject->HasParent() && pObject->GetParent()->HasGui() &&
							  pObject->GetParent()->GetGui()->IsSpellBook() )
					{
						if ( pObject->GetParent()->GetInventory()->GetLocation( pObject ) == IL_SPELL_1 ||
							 pObject->GetParent()->GetInventory()->GetLocation( pObject ) == IL_SPELL_2 )
						{							 
							sLine2 = RetrieveItemUserEdTooltip( "item_spell_book_active_slot" );										
						}
						else
						{
							sLine2 = RetrieveItemUserEdTooltip( "item_spell_book_slot" );										
						}
					}		
					else if ( pObject->HasParent() && pObject->GetParent()->HasStore() && !pObject->GetParent()->IsScreenPartyMember() )
					{
						sLine2 = RetrieveItemUserEdTooltip( "item_store" );
					}
				}
			}
		}

		if ( pText ) 
		{	
			if ( GetRolloverCast() )
			{
				GoHandle hRolloverObject( GetRolloverCastObject() );
				GoHandle hRolloverSpell( GetRolloverCastSpell() );
				GoHandle hRolloverMember( GetRolloverCastMember() );
				if ( hRolloverObject && hRolloverSpell && hRolloverMember )
				{
					if ( !sLine2.empty() )
					{
						sLine2.appendf( gpwtranslate( $MSG$ " : <c:0x%x>%s, Mana Cost: %0.0f</c>" ), gUIShell.GetTipColor( "rollover_cast" ),	hRolloverSpell->GetCommon()->GetScreenName().c_str(),
																	hRolloverSpell->GetMagic()->EvaluateManaCost( hRolloverMember, hRolloverObject ) );
					}
					else
					{
						sLine2.appendf( gpwtranslate( $MSG$ "<c:0x%x>%s, Mana Cost: %0.0f</c>" ), gUIShell.GetTipColor( "rollover_cast" ),	hRolloverSpell->GetCommon()->GetScreenName().c_str(),
																	hRolloverSpell->GetMagic()->EvaluateManaCost( hRolloverMember, hRolloverObject ) );
					}
				}
			}			

			gpwstring sFullText;
			sFullText.assignf( L"%s\n%s", sTemp.c_str(), sLine2.c_str() );
			pText->SetText( sFullText );			
		}
	}
}


bool UIPartyManager::IsPlayerPartyMember( Goid object, int & index )
{
	for ( int i = 0; i != MAX_PARTY_MEMBERS; ++i ) {
		if (( m_UIPartyMembers[i] ) && (m_UIPartyMembers[i]->GetGoid() != GOID_INVALID) )  {
			if ( object == m_UIPartyMembers[i]->GetGoid() ) {
				GoHandle hMember( m_UIPartyMembers[i]->GetGoid() );
				if( hMember && hMember->HasActor() )
				{
					index = i;
					return true;
				}
			}
		}
	}
	index = INVALID_MEMBER_INDEX;
	return false;
}


bool UIPartyManager::IsPlayerPartyMember( Goid object )
{
	GoHandle hObject( object );
	return ( hObject ? hObject->IsScreenPartyMember() : false );
}



void UIPartyManager::SelectValidPortraits( const GopColl & objects )
{
	if ( !m_pWindow[PICTURE_SELECT_1] || !gUIGame.IsActive() || !IsInGame( gWorldState.GetCurrentState() ) ) 
	{
		return;
	}

	m_pWindow[PICTURE_SELECT_1]->SetVisible( false );
	m_pWindow[PICTURE_SELECT_2]->SetVisible( false );
	m_pWindow[PICTURE_SELECT_3]->SetVisible( false );
	m_pWindow[PICTURE_SELECT_4]->SetVisible( false );
	m_pWindow[PICTURE_SELECT_5]->SetVisible( false );
	m_pWindow[PICTURE_SELECT_6]->SetVisible( false );
	m_pWindow[PICTURE_SELECT_7]->SetVisible( false );
	m_pWindow[PICTURE_SELECT_8]->SetVisible( false );

	bool bSet = false;
	bool bCloseInv = false;
	int activeIndex = 0;
	GoHandle hActivate;
	int numPartySel = 0;

	bool bSelectFocus	= false;
	Goid focusGoid		= GOID_INVALID;

	for ( GopColl::const_iterator i = objects.begin(); i != objects.end(); ++i ) 
	{
		int index = INVALID_MEMBER_INDEX;
	
		if ( IsPlayerPartyMember( (*i)->GetGoid(), index ) ) 
		{
			if ( index != INVALID_MEMBER_INDEX ) 
			{
				numPartySel++;

				switch ( index ) {
				case 0:
					m_pWindow[PICTURE_SELECT_1]->SetVisible( true );
					break;
				case 1:
					m_pWindow[PICTURE_SELECT_2]->SetVisible( true );
					break;
				case 2:
					m_pWindow[PICTURE_SELECT_3]->SetVisible( true );
					break;
				case 3:
					m_pWindow[PICTURE_SELECT_4]->SetVisible( true );
					break;
				case 4:
					m_pWindow[PICTURE_SELECT_5]->SetVisible( true );
					break;
				case 5:
					m_pWindow[PICTURE_SELECT_6]->SetVisible( true );
					break;
				case 6:
					m_pWindow[PICTURE_SELECT_7]->SetVisible( true );
					break;
				case 7:
					m_pWindow[PICTURE_SELECT_8]->SetVisible( true );
					break;
				}

				if ( !bSet )
				{	
					if ( gUIShell.IsInterfaceVisible( "character" ) || GetSingleInventoryVisible() || GetInventoriesVisible() )
					{
						if ( index != m_SelectedMember || GetInventoriesVisible() )
						{
							GoHandle hSelected( GetSelectedCharacter() );
							if ( hSelected.IsValid() && hSelected->GetLifeState() != LS_ALIVE_CONSCIOUS )
							{
								bCloseInv = true;
							}
							else
							{
								if ( IsAlive( (*i)->GetLifeState() ) )
								{
									bSet = true;
									hActivate = (*i)->GetGoid();	
									activeIndex = index;
								}
								else
								{
									bCloseInv = true;
								}
							}
						}						
					}					
				}	
				
				if ( (*i)->GetInventory()->IsPackOnly() && (*i)->IsFocused() )
				{
					bSelectFocus = true;					
				}

				if ( !(*i)->GetInventory()->IsPackOnly() )
				{
					focusGoid = (*i)->GetGoid();
				}
			}
		}
	}

	if ( bSelectFocus && focusGoid != GOID_INVALID )
	{
		gGoDb.RSSetFocusGo( focusGoid );
	}

	if ( hActivate.IsValid() )
	{
		if ( numPartySel == 1 )
		{
			ConstructCharacterInterface( activeIndex );
		}
		else
		{
			ActivateInventory( true );
		}
	}
	else if ( bCloseInv )
	{
		CloseAllCharacterMenus();
	}
}


void UIPartyManager::PortraitRollover( UIWindow * pWindow )
{
	if ( m_UIPartyMembers[pWindow->GetIndex()] && m_UIPartyMembers[pWindow->GetIndex()]->GetGoid() != GOID_INVALID )
	{
		m_UIPartyMembers[pWindow->GetIndex()]->GetGUIWindow( GUI_PORTRAIT_ROLLOVER )->SetVisible( true );
	}
}


void UIPartyManager::PortraitRolloff( UIWindow * pWindow )
{
	if ( m_UIPartyMembers[pWindow->GetIndex()] && m_UIPartyMembers[pWindow->GetIndex()]->GetGoid() != GOID_INVALID )
	{
		m_UIPartyMembers[pWindow->GetIndex()]->GetGUIWindow( GUI_PORTRAIT_ROLLOVER )->SetVisible( false );
	}
}



bool UIPartyManager::ActivateFieldCommands()
{
	if ( gUIMenuManager.IsOptionsMenuActive() )
	{
		return true;
	}

	gSoundManager.PlaySample( "s_e_gui_element_small" );

	if ( m_bFieldCommands )
	{		
		MinimizeFieldCommands();
	}
	else
	{	
		MaximizeFieldCommands();
		InitOrders();
	}
	return true;
}


void UIPartyManager::HideInventory( gpstring sGroup, int charIndex )
{	
	gUI.GetGameGUI().ShowGroup( sGroup, false );
	if ( m_UIPartyMembers[charIndex] )
	{
		GoHandle hMember( m_UIPartyMembers[charIndex]->GetGoid() );
		if ( hMember.IsValid() )
		{
			hMember->PlayVoiceSound( "gui_inventory_close", false );
			hMember->GetInventory()->GetGridbox()->SetVisible( false );

			GoidColl::iterator i;
			for ( i = m_openInventories.begin(); i != m_openInventories.end(); ++i )
			{
				if ( (*i) == hMember->GetGoid() )
				{
					m_openInventories.erase( i );
					break;
				}
			}
		}
	}	
	
}


void UIPartyManager::ButtonCloseCharacter( int charIndex )
{
	bool bAnyVisible = false;
	if ( GetInventoriesVisible() && !GetSingleInventoryVisible() )
	{
		{
			GoHandle hMember( m_UIPartyMembers[charIndex]->GetGoid() );
			if ( hMember.IsValid() )
			{
				gpstring sSpecies;
				if ( hMember->GetInventory()->IsPackOnly() )
				{
					sSpecies = "packmule";
				}
				else
				{
					sSpecies = "human";
				}

				gpstring sName;
				sName.assignf( "multi_inventory_%s_%d", sSpecies.c_str(), charIndex+1 );
				gUIShell.ShowGroup( sName, false );

				hMember->GetInventory()->GetGridbox()->SetVisible( false );
			}
		}	

		for ( int o = 0; o != MAX_PARTY_MEMBERS; ++o )
		{
			if ( m_UIPartyMembers[o] ) 
			{
				GoHandle hMember( m_UIPartyMembers[o]->GetGoid() );
				if ( hMember.IsValid() )
				{
					if ( hMember->GetInventory()->GetGridbox()->GetVisible() )
					{
						bAnyVisible = true;
					}
				}
			}
		}
	}

	if ( !bAnyVisible )
	{
		gUIStoreManager.DeactivateStore();
		CloseAllCharacterMenus();		
		SetInventoriesVisible( false );
	}

	if ( m_UIPartyMembers[charIndex] )
	{
		GoHandle hMember( m_UIPartyMembers[charIndex]->GetGoid() );
		if ( hMember.IsValid() )
		{
			hMember->PlayVoiceSound( "gui_inventory_close", false );
		}
	}
}


void UIPartyManager::SelectPortrait( int charIndex )
{
	if ( gUIShell.GetItemActive() )
	{
		return;
	}

	GoHandle hActor( m_UIPartyMembers[charIndex]->GetGoid() );
	if ( hActor )
	{
		if ( hActor->IsSelected() && gGoDb.GetSelection().size() == 1 )
		{
			ConstructCharacterInterface( charIndex );
			return;
		}

		if ( gUIStoreManager.GetActiveStoreBuyer() != hActor->GetGoid() )
		{
			gUIStoreManager.HandleStoreSwitch( hActor->GetGoid() );
		}

		if ( gUIInventoryManager.GetStashMember() != hActor->GetGoid() )
		{
			gUIInventoryManager.HandleStashSwitch( hActor->GetGoid() );
		}

		UIItemVec item_vec;
		gUI.GetGameGUI().FindActiveItems( item_vec );
		if ( item_vec.size() != 0 )
		{
			return;
		}

		if( !gAppModule.GetControlKey() )
		{
			gGoDb.DeselectAll();
		}

		if ( !hActor->IsSelected() )
		{
			hActor->PlayVoiceSound( "select_char", false );		
		}
		hActor->ToggleSelected();
	}
}


void UIPartyManager::SelectPortraitIndirect( int charIndex )
{
	GoHandle hActor( m_UIPartyMembers[charIndex]->GetGoid() );
	if ( hActor )
	{
		if ( gUIStoreManager.GetActiveStoreBuyer() != hActor->GetGoid() )
		{
			gUIStoreManager.HandleStoreSwitch( hActor->GetGoid() );
		}

		if ( gUIInventoryManager.GetStashMember() != hActor->GetGoid() )
		{
			gUIInventoryManager.HandleStashSwitch( hActor->GetGoid() );
		}
		
		hActor->PlayVoiceSound( "select_char", false );		
	}
}


void UIPartyManager::RotatePaperDoll( float rotateValue )
{
	if ( m_bEquipmentVisible == false )
	{
		for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i ) 
		{
			if ( m_UIPartyMembers[i]->GetPaperDollVisible() ) 
			{
				m_UIPartyMembers[i]->GetUICharacterDisplay()->SetRotationX( rotateValue );
			}
		}
	}	
}


void UIPartyManager::SetTargetingOrder( eFocusOrders order )
{
	GopColl::const_iterator i;
	for ( i = gGoDb.GetSelection().begin(); i != gGoDb.GetSelection().end(); ++i ) {
		int index = 0;
		if ( IsPlayerPartyMember( (*i)->GetGoid(), index ) ) {			
			(*i)->GetMind()->RSSetFocusOrders( order ) ;
		}
	}
}


void UIPartyManager::SetAttackOrder( eCombatOrders order )
{
	GopColl::const_iterator i;
	for ( i = gGoDb.GetSelection().begin(); i != gGoDb.GetSelection().end(); ++i ) {
		int index = 0;
		if ( IsPlayerPartyMember( (*i)->GetGoid(), index ) ) {
			(*i)->GetMind()->RSSetCombatOrders( order );
		}
	}
}


void UIPartyManager::SetMovementOrder( eMovementOrders order )
{
	GopColl::const_iterator i;
	for ( i = gGoDb.GetSelection().begin(); i != gGoDb.GetSelection().end(); ++i ) {
		int index = 0;
		if ( IsPlayerPartyMember( (*i)->GetGoid(), index ) ) {
			(*i)->GetMind()->RSSetMovementOrders( order );
		}
	}
}


void UIPartyManager::MinimizeFieldCommands()
{
	m_bFieldCommands = false;
	gUIShell.ShowGroup( "fc_max", false );
	gUIShell.ShowGroup( "orders", false );
	gUIShell.ShowGroup( "formations", false );
	gUIShell.ShowGroup( "fc_min", true );	
}


void UIPartyManager::MaximizeFieldCommands()
{
	m_bFieldCommands = true;
	gUIShell.ShowGroup( "fc_max", true );
	gUIShell.ShowGroup( "orders", m_bCommandsActive );
	gUIShell.ShowGroup( "formations", m_bFormationsActive );
	gUIShell.ShowGroup( "fc_min", false );

	UIWindow * pWindow1 = gUIShell.FindUIWindow( "button_field_commands_min", "field_commands" );
	UIWindow * pWindow2 = gUIShell.FindUIWindow( "button_field_commands_max", "field_commands" );
	if ( pWindow1 && pWindow2 )
	{
		if ( GetCommandsActive() )
		{
			pWindow1->SetVisible( true );
			pWindow2->SetVisible( false );
		}
		else
		{
			pWindow1->SetVisible( false );
			pWindow2->SetVisible( true );
		}
	}

	InitOrders();
}


void UIPartyManager::GetSelectedPartyMembers( GoidColl & party )
{	
	GopColl::const_iterator i;
	for ( i = gGoDb.GetSelection().begin(); i != gGoDb.GetSelection().end(); ++i )
	{
		if ( (*i)->IsScreenPartyMember() )
		{
			party.push_back( (*i)->GetGoid() );
		}
	}
}


void UIPartyManager::GetPartyMembers( GoidColl & party )
{
	for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i ) 
	{
		if ( m_UIPartyMembers[i] ) 
		{
			GoHandle hMember( m_UIPartyMembers[i]->GetGoid() );
			if ( hMember.IsValid() )
			{
				party.push_back( hMember->GetGoid() );
			}
		}
	}
}


UIPartyMember * UIPartyManager::GetCharacterFromGoid( Goid member, int & index )
{
	for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i ) 
	{
		if ( m_UIPartyMembers[i] && m_UIPartyMembers[i]->GetGoid() == member ) 
		{
			index = i;
			return m_UIPartyMembers[i];
		}
	}

	return 0;
}


bool UIPartyManager::IsPartyMemberSelected()
{	
	GopColl::const_iterator i;
	bool bSelected = false;
	for ( i = gGoDb.GetSelection().begin(); i != gGoDb.GetSelection().end(); ++i )
	{
		if ( IsPlayerPartyMember( (*i)->GetGoid() ) )
		{
			if ( !bSelected )
			{
				m_LastSelectedMembers.clear();
			}
			m_LastSelectedMembers.push_back( (*i)->GetGoid() );

			bSelected = true;
		}
	}

	return bSelected;
}


bool UIPartyManager::ToggleStatusBars()
{
	if ( !gUIMenuManager.IsOptionsMenuActive() )
	{
		SetDrawStatusBars( !GetDrawStatusBars() );
	}
	return true;
}


void UIPartyManager::SetDrawMemberLabels( bool bSet )
{ 
	m_bDrawMemberLabels = bSet; 
	if ( GetDrawMemberLabels() )
	{
		gUIShell.ShowInterface( "member_labels" );
	}
	else
	{
		gUIShell.HideInterface( "member_labels" );
	}
}


bool UIPartyManager::ToggleMemberLabels()
{
	SetDrawMemberLabels( !GetDrawMemberLabels() );
	return true;
}


void UIPartyManager::DrawMemberLabels()
{
	if ( ::IsSinglePlayer() && GetDrawMemberLabels() )
	{
		for ( int i = 0; i != MAX_PARTY_MEMBERS; ++i )
		{
			m_UIMemberLabels[i]->SetVisible( false );
			if ( m_UIPartyMembers[i] )
			{
				GoHandle hMember( m_UIPartyMembers[i]->GetGoid() );
				if ( hMember && hMember->IsInActiveScreenWorldFrustum() )
				{
					vector_3 worldPos = hMember->GetPlacement()->GetPosition().WorldPos();
					if ( gSiegeEngine.GetCamera().GetFrustum().CullSphere( worldPos, 1.0f ) )
					{
						continue;
					}

					m_UIMemberLabels[i]->SetVisible( true );
					
					SiegePos labelPos( hMember->GetPlacement()->GetPosition() );

					labelPos.pos.y += m_UIPartyMembers[i]->GetHeight();

					if ( gUIGame.GetResizeLabels() )
					{
						float dist = Length( gSiegeEngine.GetCamera().GetCameraPosition() - labelPos.WorldPos() );
						if ( dist < 8 )
						{
							m_UIMemberLabels[i]->SetFont( "b_gui_fnt_16p_copperplate-light" );
						}
						else if ( dist < 20 )
						{
							m_UIMemberLabels[i]->SetFont( "b_gui_fnt_14p_copperplate-light" );
						}
						else
						{
							m_UIMemberLabels[i]->SetFont( "b_gui_fnt_12p_copperplate-light" );
						}
					}	
					else
					{
						m_UIMemberLabels[i]->SetFont( "b_gui_fnt_12p_copperplate-light" );
					}
					

					m_UIMemberLabels[i]->SetText( hMember->GetCommon()->GetScreenName(), true );

					vector_3 screenPos = labelPos.ScreenPos();	
					int screenX = (int)(screenPos.x + 0.5f);
					int screenY = (int)(screenPos.y + 0.5f);

					m_UIMemberLabels[i]->SetRect( screenX - (m_UIMemberLabels[i]->GetRect().Width() / 2), screenX + (m_UIMemberLabels[i]->GetRect().Width() / 2),
												  screenY - m_UIMemberLabels[i]->GetRect().Height(), screenY );				

				}
			}
		}		
	}
}


void UIPartyManager::UpdateQuestCallback( int /* questId */ )
{
	UIWindow * pWindow = gUIShell.FindUIWindow( "window_quest_indicator", "data_bar" );
	if ( pWindow )
	{
		pWindow->ReceiveMessage( UIMessage( MSG_STARTANIMATION ) );
		pWindow->SetVisible( true );
	}
}


FUBI_DECLARE_XFER_TRAITS( SelectedMembers )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		// these are ok to overwrite
		if ( persist.IsRestoring() )
		{
			obj.group.clear();
		}

		persist.EnterBlock( key );
		persist.XferVector( "group", obj.group  );
		persist.Xfer	  ( "focusGoid", obj.focusGoid );	
		persist.Xfer	  ( "bExplicitlySet", obj.bExplicitlySet );		
		persist.LeaveBlock();
		return ( true );
	}
};


void UIPartyManager::Xfer( FuBi::PersistContext& persist )
{
	if ( persist.IsRestoring() )
	{
		// Restore AWP postions
		{
			UIDockbar * pBar = (UIDockbar *)gUIShell.FindUIWindow( "data_bar", "data_bar" );
			int x = 0, y = 0;
			persist.Xfer( "data_bar_x", x );
			persist.Xfer( "data_bar_y", y );		
			pBar->Dock( true, x, y );

			for ( int i = 1; i != MAX_PARTY_MEMBERS+1; ++i )
			{
				gpstring sWindow;
				sWindow.assignf( "window_portrait_panel_overlay_%d", i );
				UIDockbar * pDockbar = (UIDockbar *)gUIShell.FindUIWindow( sWindow, "character_awp" );
				if ( pDockbar )
				{
					gpstring sTemp = sWindow;
					sTemp.append( "_x" );
					persist.Xfer( sTemp, x );
					sTemp = sWindow;
					sTemp.append( "_y" );
					persist.Xfer( sTemp, y );
					// pDockbar->Dock( true, x, y );				
					pDockbar->ForceDock( x, y );
				}				
			}	
		}

		// Restore the grid place setting in case the characters are trading
		m_bGridPlace = true;
		persist.Xfer( "grid_place", m_bGridPlace );		

		// Restore Compass Settings
		{
			bool bCompassActive		= true;
			bool bCompassVisible	= true;
			bool bCompassMinimized	= true;

			persist.Xfer( "compass_active",		bCompassActive );
			persist.Xfer( "compass_visible",	bCompassVisible );
			persist.Xfer( "compass_minimized",  bCompassMinimized );
	
			gSiegeEngine.GetCompass().SetActive( bCompassActive );
			gSiegeEngine.GetCompass().SetVisible( bCompassVisible );
			
			if ( bCompassActive && bCompassVisible && !bCompassMinimized )
			{
				gUIMenuManager.ShowCompass( true );
			}
			else
			{
				gUIMenuManager.ShowCompass( false );
			}			 
		}

		// Restore Label Settings
		{			
			bool bLabels = false;
			persist.Xfer( "labels_visible", bLabels );
			gUIGame.GetUIItemOverlay()->SetVisible( bLabels );
			gWorldOptions.SetIndicateItems( true );
		}

		// Restore Follow Mode
		persist.Xfer( "m_bFollowMode", m_bFollowMode );
		UICheckbox * pCheck = (UICheckbox *)gUIShell.FindUIWindow( "button_follow" );
		if ( pCheck )
		{
			pCheck->SetState( m_bFollowMode );
		}

		// Restore former party order		
		persist.XferMap( "m_PartyIndexMap", m_PartyIndexMap );

		// Xfer the flashing journal
		UIWindow * pWindow = gUIShell.FindUIWindow( "window_quest_indicator", "data_bar" );
		if ( pWindow )
		{
			bool bIsAnimating = false;
			persist.Xfer( "journal_animating", bIsAnimating );
			if ( bIsAnimating )
			{
				pWindow->ReceiveMessage( UIMessage( MSG_STARTANIMATION ) );
				pWindow->SetVisible( true );
			}
			else
			{
				gUIAnimation.RemoveAnimationsForWindow( pWindow );
				pWindow->SetVisible( false );
			}
		}
	}
	else if ( persist.IsSaving() )
	{
		// Save AWP positions
		{
			UIWindow * pBar = gUIShell.FindUIWindow( "data_bar", "data_bar" );
			int x = pBar->GetRect().left;
			int y = pBar->GetRect().top;
			persist.Xfer( "data_bar_x", x );
			persist.Xfer( "data_bar_y", y );
			
			for ( int i = 1; i != MAX_PARTY_MEMBERS+1; ++i )
			{
				gpstring sWindow;
				sWindow.assignf( "window_portrait_panel_overlay_%d", i );
				UIWindow * pWindow = gUIShell.FindUIWindow( sWindow, "character_awp" );
				if ( pWindow )
				{
					x = pWindow->GetRect().left;
					y = pWindow->GetRect().top;
					
					gpstring sTemp = sWindow;
					sTemp.append( "_x" );
					persist.Xfer( sTemp, x );
					sTemp = sWindow;
					sTemp.append( "_y" );
					persist.Xfer( sTemp, y );
				}
			}
		}

		// Save the grid place setting in case the characters are trading		
		m_bGridPlace = gUIShell.GetGridPlace();
		persist.Xfer( "grid_place", m_bGridPlace );

		// Save Compass Settings
		{
			bool bCompassVisible	= gSiegeEngine.GetCompass().GetVisible();
			bool bCompassActive		= gSiegeEngine.GetCompass().GetActive();
			bool bCompassMinimized	= gSiegeEngine.GetCompass().GetMinimized();
			persist.Xfer( "compass_visible",	bCompassVisible );	
			persist.Xfer( "compass_active",		bCompassActive );				
			persist.Xfer( "compass_minimized",	bCompassMinimized );				
		}

		// Save Label Settings
		{
			bool bLabels = gUIGame.GetUIItemOverlay()->IsVisible();
			persist.Xfer( "labels_visible", bLabels );
		}	

		// Save Follow Mode
		persist.Xfer( "m_bFollowMode", m_bFollowMode );		

		// Save party index map
		persist.XferMap( "m_PartyIndexMap", m_PartyIndexMap );
		
		// Xfer the flashing journal
		UIWindow * pWindow = gUIShell.FindUIWindow( "window_quest_indicator", "data_bar" );
		if ( pWindow )
		{
			bool bIsAnimating =	gUIAnimation.IsAnimating( pWindow );
			persist.Xfer( "journal_animating", bIsAnimating );			
		}		
	}

	persist.Xfer( "m_group1", m_group[0] );
	persist.Xfer( "m_group2", m_group[1] );
	persist.Xfer( "m_group3", m_group[2] );
	persist.Xfer( "m_group4", m_group[3] );
	persist.Xfer( "m_group5", m_group[4] );
	persist.Xfer( "m_group6", m_group[5] );
	persist.Xfer( "m_group7", m_group[6] );
	persist.Xfer( "m_group8", m_group[7] );

	persist.Xfer( "m_bFieldCommands", m_bFieldCommands );
	if ( persist.IsRestoring() )
	{
		if ( m_bFieldCommands )
		{
			MaximizeFieldCommands();
		}
		else
		{
			MinimizeFieldCommands();
		}
	}
	
	persist.Xfer( "m_bCommandsActive", m_bCommandsActive );
	if ( persist.IsRestoring() )
	{
		if ( m_bFieldCommands )
		{
			SetCommandsActive( m_bCommandsActive );
		}
	}

	persist.Xfer( "m_bFormationsActive", m_bFormationsActive );
	persist.Xfer( "m_bDrawMemberLabels", m_bDrawMemberLabels );
	persist.XferVector( "m_GuardedCharacters", m_GuardedCharacters );
}


void UIPartyManager::PostXfer()
{
	if ( m_bFieldCommands )
	{
		SetFormationsActive( m_bFormationsActive );
	}

	gUIShell.SetGridPlace( m_bGridPlace );

	gVictory.RegisterInventoryUpdateCallback( makeFunctor( *this, &UIPartyManager::UpdateQuestCallback ) );
}


void UIPartyManager::EnableHealthButton( bool bEnable )
{
	if ( m_pWindow[POTION_HEALTH] )
	{
		if ( bEnable )
		{
			((UIButton *)m_pWindow[POTION_HEALTH])->EnableButton();
		}
		else
		{
			((UIButton *)m_pWindow[POTION_HEALTH])->DisableButton();
		}
	}
}


void UIPartyManager::EnableManaButton( bool bEnable )
{
	if ( m_pWindow[POTION_MANA] )
	{
		if ( bEnable )
		{
			((UIButton *)m_pWindow[POTION_MANA])->EnableButton();
		}
		else
		{
			((UIButton *)m_pWindow[POTION_MANA])->DisableButton();
		}
	}
}


void UIPartyManager::SetLastSelectedMembers( GoidColl & members )
{
	m_LastSelectedMembers = members;
}


void UIPartyManager::GetLastSelectedMembers( GoidColl & members )
{
	members = m_LastSelectedMembers;
}


bool UIPartyManager::DisbandButton()
{
	m_disbandRequests.clear();
	GoidColl selection;
	GoidColl::iterator i;
	GetSelectedPartyMembers( selection );
	for ( i = selection.begin(); i != selection.end(); ++i )
	{
		Go * pTalker = 0;
		if ( CanDisband( *i, pTalker ) )
		{
			m_disbandRequests.push_back( *i );
		}
	}

	if ( m_disbandRequests.size() > 1 )
	{
		SetMultipleDisband( true );
	}
	else 
	{
		SetMultipleDisband( false );
	}	

	return true;
}


bool UIPartyManager::CanDisband( Goid member, Go * & pTalker )
{		
	Go*				party			= gServer.GetScreenParty();	
	
	{	
		GopColl::const_iterator i, ibegin = party->GetChildBegin(), iend = party->GetChildEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( (*i)->GetGoid() != member && !(*i)->GetInventory()->IsPackOnly()  )
			{
				pTalker = *i;
				break;
			}
		}
	}

	if ( !pTalker )
	{
		return false;
	}

	{
		int alive = 0;
		GopColl partyColl = party->GetChildren();
		for ( GopColl::iterator i = partyColl.begin(); i != partyColl.end(); ++i )
		{
			if ( (*i)->IsAnyHumanPartyMember() && !(*i)->GetInventory()->IsPackOnly() && IsConscious( (*i)->GetLifeState() ) )
			{
				alive++;
			}
		}

		GoHandle hMember( member );
		if ( hMember.IsValid() )
		{
			if ( hMember->IsAnyHumanPartyMember() && !hMember->GetInventory()->IsPackOnly() && IsConscious( hMember->GetLifeState() ) )
			{
				if ( (alive-1) == 0 )
				{
					return false;
				}
			}
		}
	}

	return true;
}


void UIPartyManager::DisbandDialog()
{
	if ( m_disbandRequests.size() == 0 || gUIDialogueHandler.GetTalkAttempt() || gUIShell.IsInterfaceVisible( "dialogue_box" ) )
	{
		return;
	}

	GoidColl::iterator iRequest = m_disbandRequests.begin();
	GoHandle hDisband( *iRequest );
	Go * pTalker = 0;
	if ( hDisband.IsValid() && CanDisband( hDisband->GetGoid(), pTalker ) )
	{
		CloseAllCharacterMenus();

		m_memberDisband = hDisband->GetGoid();

		gpwstring disbandText;
		disbandText.assignf( gpwtranslate( $MSG$ "Are you sure you want to disband %s?" ), hDisband->GetCommon()->GetScreenName().c_str() );

		gUIGame.ShowDialog( DIALOG_DISBAND, disbandText );
	}
	else if ( m_disbandRequests.size() == 1 )
	{
		m_disbandRequests.clear();
	}
	else
	{
		m_disbandRequests.erase( iRequest );
	}
}


void UIPartyManager::DisbandCancelSelectedMember()
{
	if ( m_memberDisband != GOID_INVALID )
	{
		GoidColl::iterator i;
		for ( i = m_disbandRequests.begin(); i != m_disbandRequests.end(); ++i )
		{
			if ( (*i) == m_memberDisband )
			{
				m_disbandRequests.erase( i );
				return;
			}
		}
	}
}


void UIPartyManager::DisbandCancelAll()
{
	m_disbandRequests.clear();
}

void UIPartyManager::DisbandSelectedMember()
{
	if ( m_disbandRequests.size() == 0 )
	{
		return;
	}

	GoidColl::iterator iRequest = m_disbandRequests.begin();
	GoHandle hDisband( *iRequest );
	Go * pTalker = 0;
	if ( hDisband.IsValid() && CanDisband( hDisband->GetGoid(), pTalker ) )
	{
		CloseAllCharacterMenus();

		Go * party = gServer.GetScreenParty();
	
		int iMember = 0;
		if ( GetCharacterFromGoid( hDisband->GetGoid(), iMember ) )
		{
			gpstring sCharacter;
			sCharacter.assignf( "awp_itemslot_portrait_%d", iMember+1 ); 		
			{
				UIItemSlot	* pWindow	= (UIItemSlot *)gUI.GetGameGUI().FindUIWindow( sCharacter );
				UIItem		* pItem		= gUIShell.GetItem( pWindow->GetItemID() );
				if ( pWindow && pItem ) 
				{
					pWindow->ClearItem();
					pItem->RemoveItemParent( pWindow );
				}
			}
			
			if ( !hDisband->GetInventory()->IsPackOnly() )
			{
				sCharacter.assignf( "multi_portrait_human_%d", iMember+1 );
			}
			else
			{
				sCharacter.assignf( "multi_portrait_packmule_%d", iMember+1 );
			}
			{
				UIItemSlot	* pWindow	= (UIItemSlot *)gUI.GetGameGUI().FindUIWindow( sCharacter );
				UIItem		* pItem		= gUIShell.GetItem( pWindow->GetItemID() );
				if ( pWindow && pItem ) 
				{
					pWindow->ClearItem();
					pItem->RemoveItemParent( pWindow );
				}
			}
		}

		// request to remove child
		party->GetParty()->RSRemoveMemberNow( hDisband );
		gGoDb.Deselect( hDisband->GetGoid() );
		hDisband->GetInventory()->GetGridbox()->PurgeItems();		
		
		m_memberDisband = GOID_INVALID;

		gpstring sMember;
		if ( hDisband->HasConversation() && hDisband->GetConversation()->GetCanTalk() )
		{			
			sMember.assignf( "party_disband_0x%x", hDisband->GetScid() );
			gGameAuditor.Set( sMember.c_str(), true );	
			gUIGame.GetUICommands()->CommandTalk( pTalker, hDisband );	
			gUIDialogueHandler.SetTalkAttempt( true );
		}		
		else
		{			
			sMember.assignf( "party_disband_0x%x", hDisband->GetScid() );
			gGameAuditor.Set( sMember.c_str(), false );
			sMember.assignf( "party_disbanded_0x%x", hDisband->GetScid() );
			gGameAuditor.Set( sMember.c_str(), true );							
		}
	}	

	m_disbandRequests.erase( iRequest );
}


void UIPartyManager::DisbandDeadMember( Go * pMember )
{
	Go * pTalker = 0;
	if ( CanDisband( pMember->GetGoid(), pTalker ) )
	{
		CloseAllCharacterMenus();
		Go * party = gServer.GetScreenParty();			
		party->GetParty()->RSRemoveMemberNow( pMember );
		gGoDb.Deselect( pMember->GetGoid() );

		gpstring sTemp;
		sTemp.assignf( "party_disband_0x%x", pMember->GetScid() );
		gGameAuditor.Set( sTemp.c_str(), false );	
		sTemp.assignf( "party_disbanded_0x%x", pMember->GetScid() );
		gGameAuditor.Set( sTemp.c_str(), false );	
		sTemp.assignf( "party_allow_rejoin_0x%x", pMember->GetScid() );
		gGameAuditor.Set( sTemp.c_str(), false );	
	}
}


void UIPartyManager::ArrangeFormation( const gpstring & sShape )
{
	if ( gWorldState.GetCurrentState() != WS_LOADING_SAVE_GAME )
	{
		if( gServer.GetScreenPlayer() && gServer.GetScreenParty() && IsInGame( gWorldState.GetCurrentState() ) )
		{
			Go * party				= gServer.GetScreenParty();
			Formation * formation	= gServer.GetScreenFormation();
			
			if ( formation )
			{
				if ( !sShape.empty() )
				{
					formation->SetShape( sShape );
				}
				else
				{
					formation->SetShape( formation->GetShape() );
				}
			}

			gUIGame.RankPartyMembers();

			if( party && formation && formation->GetPosition() == SiegePos::INVALID && party->GetChildren().size() != 1 )				
			{			
				formation->SetPosition( party->GetPlacement()->GetPosition() );			
				formation->Move( QP_CLEAR, AO_USER );
			}	
		}
	}
}

bool UIPartyManager::ToggleQuestLog()
{
	gUIDialogueHandler.ToggleJournal();
	return true;
}


void UIPartyManager::LoreBookDialog( Go * pLoreBook )
{
	CloseAllCharacterMenus();
	gUIShell.ActivateInterface( "ui:interfaces:backend:lore_book", true );

	gpstring sMap = gWorldMap.MakeMapDirAddress();
	sMap.assignf( "%s:info:lore:%s", sMap.c_str(), pLoreBook->GetGui()->GetLoreKey().c_str() );
	FastFuelHandle hLore( sMap );
	if ( hLore.IsValid() )
	{
		gpstring sDesc;
		hLore.Get( "description", sDesc );		

		UITextBox * pLoreBox = (UITextBox *)gUIShell.FindUIWindow( "lore_box", "lore_book" );
		if ( pLoreBox )
		{
			pLoreBox->SetLeadElement( 0 );
			pLoreBox->RecalculateSlider();

			pLoreBox->SetText( ReportSys::TranslateW( sDesc.c_str() ) );
		}

		UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_book_title", "lore_book" );
		if ( pText )
		{
			pText->SetText( pLoreBook->GetCommon()->GetScreenName() );
		}		
	}

	gSiegeEngine.GetCompass().SetVisible( false );
	gUIShell.HideInterface( "compass_hotpoints" );
}


void UIPartyManager::HideLoreBook()
{
	gUIShell.MarkInterfaceForDeactivation( "lore_book" );
	gSiegeEngine.GetCompass().SetVisible( true );
	gUIShell.ShowInterface( "compass_hotpoints" );
}

void UIPartyManager::GetPortraitOrderedParty( GoidColl & partyColl )
{		
	std::map< Goid, UIRect > m_partyRects;
	std::map< Goid, UIRect >::iterator iRect;
	std::map< Goid, UIRect >::iterator iRect2;

	for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i )
	{
		if( m_UIPartyMembers[i] == NULL )
		{
			continue;
		}

		if ( m_UIPartyMembers[i]->GetGoid() == GOID_INVALID )
		{
			continue;
		}
	
		m_partyRects.insert( std::pair< Goid, UIRect >( m_UIPartyMembers[i]->GetGoid(),
														m_UIPartyMembers[i]->GetGUIWindow( GUI_PORTRAIT )->GetRect() ) );
	}
	
	iRect = m_partyRects.begin();	
	while ( m_partyRects.size() != 0 )
	{
		bool bGreatest = true;
		for ( iRect2 = m_partyRects.begin(); iRect2 != m_partyRects.end(); ++iRect2 )
		{		
			if ( (*iRect).first != (*iRect2).first )
			{
				if ( (*iRect).second.top > (*iRect2).second.top )					 
				{
					bGreatest = false;
				}
			}
		}

		if ( bGreatest )
		{
			partyColl.push_back( (*iRect).first );
			iRect = m_partyRects.erase( iRect );
			if ( iRect == m_partyRects.end() )
			{
				iRect = m_partyRects.begin();
			}
		}
		else
		{
			++iRect;
			if ( iRect == m_partyRects.end() )
			{
				iRect = m_partyRects.begin();
			}
		}
	}
}


int UIPartyManager::GetMemberPortraitRank( Go * pMember )
{
	std::multimap< Goid, UIRect > m_partyRects;
	std::multimap< Goid, UIRect >::iterator iRect;
	std::multimap< Goid, UIRect >::iterator iRect2;

	if ( pMember && pMember->HasActor() && !pMember->GetInventory()->HasGridbox() )
	{
		return -1;
	}

	for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i )
	{
		gpstring sCharacter;
		sCharacter.assignf( "awp_itemslot_portrait_%d", i+1 );
		UIWindow * pWindow	= gUI.GetGameGUI().FindUIWindow( sCharacter );		

		Goid member = GOID_INVALID;
		if ( m_UIPartyMembers[i] != NULL )
		{
			GoHandle hMember( m_UIPartyMembers[i]->GetGoid() );
			if ( hMember.IsValid() )
			{
				member = hMember->GetGoid();
			}
		}

		m_partyRects.insert( std::pair< Goid, UIRect >( member, pWindow->GetRect() ) );
	}

	int rank = 0;

	iRect = m_partyRects.begin();	
	while ( m_partyRects.size() != 0 )
	{
		bool bGreatest = true;
		for ( iRect2 = m_partyRects.begin(); iRect2 != m_partyRects.end(); ++iRect2 )
		{		
			if ( (*iRect).second.top > (*iRect2).second.top )					 
			{
				bGreatest = false;
			}			
		}

		if ( bGreatest )
		{	
			if ( (*iRect).first == pMember->GetGoid() )
			{
				return rank;
			}
			
			GoHandle hRanker( (*iRect).first );

			iRect = m_partyRects.erase( iRect );
			if ( iRect == m_partyRects.end() )
			{
				iRect = m_partyRects.begin();
			}				
			
			if ( hRanker.IsValid() )
			{
				rank++;
			}
			else if ( !hRanker.IsValid() )
			{
				rank++;
			}
		}
		else
		{
			++iRect;
			if ( iRect == m_partyRects.end() )
			{
				iRect = m_partyRects.begin();
			}
		}
	}

	return rank;
}


bool UIPartyManager::IsCharacterInterfaceBusy( Go * pMember, bool bForce )
{
	if ( pMember && pMember->HasMind() )
	{
		if ( bForce || (IsExpertModeEnabled() && gUIShell.IsInterfaceVisible( "character" )))
		{
			if ( (pMember->HasInventory() && pMember->GetInventory()->HasGridbox() && !pMember->GetInventory()->GetGridbox()->GetLocalPlace()) ||
				 !gUIShell.GetGridPlace() )
			{
				return true;			
			}
		}
	}
	return false;
}


void UIPartyManager::MovementOrderRotate()
{
	UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_movement", "field_commands" );
	if ( pText )
	{
		UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_movement_free", "field_commands" );
		if ( pRadio && pRadio->GetCheck() )
		{
			pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_movement_engage", "field_commands" );
			if ( pRadio )
			{
				pRadio->SetMidState( false );
				pRadio->SetCheck( true );
				pText->SetText( gpwtranslate( $MSG$ "Engage" ) );
				SetMovementOrder( MO_LIMITED );
			}
		}
		else 
		{
			pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_movement_engage", "field_commands" );
			if ( pRadio && pRadio->GetCheck() )
			{
				pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_movement_holdground", "field_commands" );
				if ( pRadio )
				{
					pRadio->SetMidState( false );
					pRadio->SetCheck( true );
					pText->SetText( gpwtranslate( $MSG$ "Hold Ground" ) );
					SetMovementOrder( MO_HOLD );
				}
			}
			else
			{
				UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_movement_free", "field_commands" );		
				if ( pRadio )
				{
					pRadio->SetMidState( false );
					pRadio->SetCheck( true );
					pText->SetText( gpwtranslate( $MSG$ "Move Freely" ) );
					SetMovementOrder( MO_FREE );
				}
			}
		}
		
	}
}


void UIPartyManager::AttackOrderRotate()
{
	UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_attack", "field_commands" );
	if ( pText )
	{
		UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_attack_free", "field_commands" );
		if ( pRadio && pRadio->GetCheck() )
		{
			pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_attack_fightback", "field_commands" );
			if ( pRadio )
			{
				pRadio->SetMidState( false );
				pRadio->SetCheck( true );
				pText->SetText( gpwtranslate( $MSG$ "Defend" ) );
				SetAttackOrder( CO_LIMITED );
			}
		}
		else 
		{
			pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_attack_fightback", "field_commands" );
			if ( pRadio && pRadio->GetCheck() )
			{
				pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_attack_holdfire", "field_commands" );
				if ( pRadio )
				{
					pRadio->SetMidState( false );
					pRadio->SetCheck( true );
					pText->SetText( gpwtranslate( $MSG$ "Hold Fire" ) );
					SetAttackOrder( CO_HOLD );
				}
			}
			else
			{
				UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_attack_free", "field_commands" );		
				if ( pRadio )
				{
					pRadio->SetMidState( false );
					pRadio->SetCheck( true );
					pText->SetText( gpwtranslate( $MSG$ "Attack Freely" ) );
					SetAttackOrder( CO_FREE );
				}
			}
		}
		
	}
}


void UIPartyManager::TargetingOrderRotate()
{
	UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_targeting", "field_commands" );
	if ( pText )
	{
		UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_target_closest", "field_commands" );
		if ( pRadio && pRadio->GetCheck() )
		{
			pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_target_strongest", "field_commands" );
			if ( pRadio )
			{
				pRadio->SetMidState( false );
				pRadio->SetCheck( true );
				pText->SetText( gpwtranslate( $MSG$ "Target Strongest" ) );
				SetTargetingOrder( FO_STRONGEST );
			}
		}
		else
		{
			pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_target_strongest", "field_commands" );
			if ( pRadio && pRadio->GetCheck() )
			{
				pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_target_weakest", "field_commands" );
				if ( pRadio )
				{
					pRadio->SetMidState( false );
					pRadio->SetCheck( true );
					pText->SetText( gpwtranslate( $MSG$ "Target Weakest" ) );
					SetTargetingOrder( FO_WEAKEST );
				}
			}
			else 
			{
				UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_target_closest", "field_commands" );		
				if ( pRadio )
				{
					pRadio->SetMidState( false );
					pRadio->SetCheck( true );
					pText->SetText( gpwtranslate( $MSG$ "Target Closest" ) );
					SetTargetingOrder( FO_CLOSEST );
				}			
			}
		}		
	}
}


void UIPartyManager::HandleGuiOrder( const gpstring & sOrder )
{
	if ( gWorldState.GetCurrentState() == WS_LOADING_SAVE_GAME )
	{
		return;
	}

	if ( !m_bInitializingOrders )
	{
		if ( sOrder.same_no_case( "radio_button_attack_fightback" ) )
		{
			UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_attack", "field_commands" );
			UIButton *	pButton	= (UIButton *)gUIShell.FindUIWindow( "button_attack", "field_commands" );
			if ( pText )
			{
				pText->SetText( gpwtranslate( $MSG$ "Defend" ) );
				pButton->SetRolloverKey( "button_attack_fightback" );
				gUIShell.SetRolloverName( "button_attack_fightback" );
			}
			SetAttackOrder( CO_LIMITED );
		}
		else if ( sOrder.same_no_case( "radio_button_attack_free" ) )
		{
			UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_attack", "field_commands" );
			UIButton *	pButton	= (UIButton *)gUIShell.FindUIWindow( "button_attack", "field_commands" );
			if ( pText )
			{
				pText->SetText( gpwtranslate( $MSG$ "Attack Freely" ) );
				pButton->SetRolloverKey( "button_attack_free" );
				gUIShell.SetRolloverName( "button_attack_free" );
			}
			SetAttackOrder( CO_FREE );
		}
		else if ( sOrder.same_no_case( "radio_button_attack_holdfire" ) )
		{
			UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_attack", "field_commands" );
			UIButton *	pButton	= (UIButton *)gUIShell.FindUIWindow( "button_attack", "field_commands" );
			if ( pText )
			{
				pText->SetText( gpwtranslate( $MSG$ "Hold Fire" ) );
				pButton->SetRolloverKey( "button_attack_holdfire" );
				gUIShell.SetRolloverName( "button_attack_holdfire" );
			}
			SetAttackOrder( CO_HOLD );
		}
		else if ( sOrder.same_no_case( "radio_button_movement_engage" ) )
		{
			UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_movement", "field_commands" );
			UIButton *	pButton	= (UIButton *)gUIShell.FindUIWindow( "button_movement", "field_commands" );
			if ( pText )
			{
				pText->SetText( gpwtranslate( $MSG$ "Engage" ) );
				pButton->SetRolloverKey( "button_movement_engage" );
				gUIShell.SetRolloverName( "button_movement_engage" );
			}
			SetMovementOrder( MO_LIMITED );
		}
		else if ( sOrder.same_no_case( "radio_button_movement_free" ) )
		{
			UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_movement", "field_commands" );
			UIButton *	pButton	= (UIButton *)gUIShell.FindUIWindow( "button_movement", "field_commands" );
			if ( pText )
			{
				pText->SetText( gpwtranslate( $MSG$ "Move Freely" ) );
				pButton->SetRolloverKey( "button_movement_freely" );
				gUIShell.SetRolloverName( "button_movement_freely" );
			}
			SetMovementOrder( MO_FREE );
		}
		else if ( sOrder.same_no_case( "radio_button_movement_holdground" ) )
		{
			UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_movement", "field_commands" );
			UIButton *	pButton	= (UIButton *)gUIShell.FindUIWindow( "button_movement", "field_commands" );
			if ( pText )
			{
				pText->SetText( gpwtranslate( $MSG$ "Hold Ground" ) );
				pButton->SetRolloverKey( "button_movement_holdground" );
				gUIShell.SetRolloverName( "button_movement_holdground" );
			}
			SetMovementOrder( MO_HOLD );
		}
		else if ( sOrder.same_no_case( "radio_button_target_strongest" ) )
		{
			UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_targeting", "field_commands" );
			UIButton *	pButton	= (UIButton *)gUIShell.FindUIWindow( "button_targeting", "field_commands" );
			if ( pText )
			{
				pText->SetText( gpwtranslate( $MSG$ "Target Strongest" ) );
				pButton->SetRolloverKey( "button_target_strongest" );
				gUIShell.SetRolloverName( "button_target_strongest" );
			}
			SetTargetingOrder( FO_STRONGEST );
		}
		else if ( sOrder.same_no_case( "radio_button_target_weakest" ) )
		{
			UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_targeting", "field_commands" );
			UIButton *	pButton	= (UIButton *)gUIShell.FindUIWindow( "button_targeting", "field_commands" );
			if ( pText )
			{
				pText->SetText( gpwtranslate( $MSG$ "Target Weakest" ) );
				pButton->SetRolloverKey( "button_target_weakest" );
				gUIShell.SetRolloverName( "button_target_weakest" );
			}
			SetTargetingOrder( FO_WEAKEST );
		}	
		else if ( sOrder.same_no_case( "radio_button_target_closest" ) )
		{
			UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_targeting", "field_commands" );
			UIButton *	pButton	= (UIButton *)gUIShell.FindUIWindow( "button_targeting", "field_commands" );
			if ( pText )
			{
				pText->SetText( gpwtranslate( $MSG$ "Target Closest" ) );
				pButton->SetRolloverKey( "button_target_closest" );
				gUIShell.SetRolloverName( "button_target_closest" );
			}
			SetTargetingOrder( FO_CLOSEST );
		}
	}
}


void UIPartyManager::InitOrders()
{		
	m_bInitializingOrders = true;
	if ( !m_bFieldCommands )
	{
		// Early out number 1: not visible
		return;
	}

	GoidColl selection;	
	GetSelectedPartyMembers( selection );
	if ( selection.size() == 0 )
	{	
		return;
	}
	GoHandle hMember( *(selection.begin()) );
	if ( !hMember.IsValid() )
	{		
		return;
	}

	m_memberCommands = hMember->GetGoid();
	
	GoidColl partyColl;
	GoidColl::iterator i;
	GetSelectedPartyMembers( partyColl );

	// Figure out if the orders are different for Combat
	bool bCoDiff	= false;
	bool bCoLimited = false;
	bool bCoFree	= false;
	bool bCoHold	= false;
	for ( i = partyColl.begin(); i != partyColl.end(); ++i )
	{
		GoHandle hPartyMember( *i );
		if ( hPartyMember.IsValid() )
		{
			if ( hMember->GetMind()->GetCombatOrders() != hPartyMember->GetMind()->GetCombatOrders() )
			{
				bCoDiff = true;
				if ( hPartyMember->GetMind()->GetCombatOrders() == CO_LIMITED )
				{
					bCoLimited = true;
				}
				else if ( hPartyMember->GetMind()->GetCombatOrders() == CO_FREE )
				{
					bCoFree = true;
				}
				else if ( hPartyMember->GetMind()->GetCombatOrders() == CO_HOLD )
				{
					bCoHold = true;
				}
			}
		}
	}

	{
		UIRadioButton * pRadioCoLimited	= (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_attack_fightback", "field_commands" );
		UIRadioButton * pRadioCoFree	= (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_attack_free", "field_commands" );
		UIRadioButton * pRadioCoHold	= (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_attack_holdfire", "field_commands" );
		UIText *		pText			= (UIText *)gUIShell.FindUIWindow( "text_attack", "field_commands" );
		UIButton *		pButton			= (UIButton *)gUIShell.FindUIWindow( "button_attack", "field_commands" );

		if ( hMember->GetMind()->GetCombatOrders() == CO_LIMITED )
		{		
			pText->SetText( gpwtranslate( $MSG$ "Defend" ) );
			pRadioCoLimited->SetMidState( bCoDiff );	
			pRadioCoLimited->SetCheck( true );			
			pButton->SetRolloverKey( "button_attack_fightback" );
		}
		if ( hMember->GetMind()->GetCombatOrders() == CO_FREE )
		{		
			pText->SetText( gpwtranslate( $MSG$ "Attack Freely" ) );
			pRadioCoFree->SetMidState( bCoDiff );			
			pRadioCoFree->SetCheck( true );									
			pButton->SetRolloverKey( "button_attack_free" );
		}
		if ( hMember->GetMind()->GetCombatOrders() == CO_HOLD )
		{		
			pText->SetText( gpwtranslate( $MSG$ "Hold Fire" ) );
			pRadioCoHold->SetMidState( bCoDiff );				
			pRadioCoHold->SetCheck( true );						
			pButton->SetRolloverKey( "button_attack_holdfire" );
		}

		if ( bCoFree )
		{
			pRadioCoFree->SetMidState( true );
		}
		if ( bCoLimited )
		{
			pRadioCoLimited->SetMidState( true );
		}
		if ( bCoHold )
		{
			pRadioCoHold->SetMidState( true );
		}
	}

	// Figure out if the orders are different for Movement
	bool bMoDiff	= false;
	bool bMoHold = false;
	bool bMoFree	= false;
	bool bMoLimited = false;
	for ( i = partyColl.begin(); i != partyColl.end(); ++i )
	{
		GoHandle hPartyMember( *i );
		if ( hPartyMember.IsValid() )
		{
			if ( hMember->GetMind()->GetMovementOrders() != hPartyMember->GetMind()->GetMovementOrders() )
			{
				bMoDiff = true;
				if ( hPartyMember->GetMind()->GetMovementOrders() == MO_HOLD )
				{
					bMoHold = true;
				}
				else if ( hPartyMember->GetMind()->GetMovementOrders() == MO_FREE )
				{
					bMoFree = true;
				}
				else if ( hPartyMember->GetMind()->GetMovementOrders() == MO_LIMITED )
				{
					bMoLimited = true;
				}
			}
		}
	}

	{
		UIRadioButton * pRadioMoFree	= (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_movement_free", "field_commands" );
		UIRadioButton * pRadioMoHold	= (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_movement_holdground", "field_commands" );
		UIRadioButton * pRadioMoLimited = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_movement_engage", "field_commands" );
		UIText *		pText			= (UIText *)gUIShell.FindUIWindow( "text_movement", "field_commands" );		
		UIButton *		pButton			= (UIButton *)gUIShell.FindUIWindow( "button_movement", "field_commands" );

		if ( hMember->GetMind()->GetMovementOrders() == MO_FREE )
		{		
			pText->SetText( gpwtranslate( $MSG$ "Move Freely" ) );
			pRadioMoFree->SetMidState( bMoDiff );			
			pRadioMoFree->SetCheck( true );			
			pButton->SetRolloverKey( "button_movement_freely" );
		}
		if ( hMember->GetMind()->GetMovementOrders() == MO_HOLD )
		{		
			pText->SetText( gpwtranslate( $MSG$ "Hold Ground" ) );
			pRadioMoHold->SetMidState( bMoDiff );				
			pRadioMoHold->SetCheck( true );							
			pButton->SetRolloverKey( "button_movement_holdground" );
		}
		if ( hMember->GetMind()->GetMovementOrders() == MO_LIMITED )
		{	
			pText->SetText( gpwtranslate( $MSG$ "Engage" ) );
			pRadioMoLimited->SetMidState( bMoDiff );
			pRadioMoLimited->SetCheck( true );						
			pButton->SetRolloverKey( "button_movement_engage" );
		}

		if ( bMoFree )
		{
			pRadioMoFree->SetMidState( true );
		}
		if ( bMoLimited )
		{
			pRadioMoLimited->SetMidState( true );
		}
		if ( bMoHold )
		{
			pRadioMoHold->SetMidState( true );
		}
	}

	// Figure out if the orders are different for Focus
	bool bFoDiff = false;
	bool bFoStrongest = false;
	bool bFoWeakest = false;
	bool bFoClosest = false;
	for ( i = partyColl.begin(); i != partyColl.end(); ++i )
	{
		GoHandle hPartyMember( *i );
		if ( hPartyMember.IsValid() )
		{
			if ( hMember->GetMind()->GetFocusOrders() != hPartyMember->GetMind()->GetFocusOrders() )
			{
				bFoDiff = true;				
				if ( hPartyMember->GetMind()->GetFocusOrders() == FO_STRONGEST )
				{
					bFoStrongest = true;
				}
				else if ( hPartyMember->GetMind()->GetFocusOrders() == FO_WEAKEST )
				{
					bFoWeakest = true;
				}	
				else if ( hPartyMember->GetMind()->GetFocusOrders() == FO_CLOSEST )
				{
					bFoClosest = true;
				}
			}
		}
	}	

	{
		UIRadioButton * pRadioFoStrongest	= (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_target_strongest", "field_commands" );
		UIRadioButton * pRadioFoWeakest		= (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_target_weakest", "field_commands" );
		UIRadioButton * pRadioFoClosest		= (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_target_closest", "field_commands" );
		UIText *		pText				= (UIText *)gUIShell.FindUIWindow( "text_targeting", "field_commands" );
		UIButton *		pButton				= (UIButton *)gUIShell.FindUIWindow( "button_targeting", "field_commands" );

		if ( hMember->GetMind()->GetFocusOrders() == FO_STRONGEST )
		{	
			pText->SetText( gpwtranslate( $MSG$ "Target Strongest" ) );
			pRadioFoStrongest->SetMidState( bFoDiff );
			pRadioFoStrongest->SetCheck( true );
			pButton->SetRolloverKey( "button_target_strongest" );
		}
		if ( hMember->GetMind()->GetFocusOrders() == FO_WEAKEST ) 
		{		
			pText->SetText( gpwtranslate( $MSG$ "Target Weakest" ) );			
			pRadioFoWeakest->SetMidState( bFoDiff );			
			pRadioFoWeakest->SetCheck( true );		
			pButton->SetRolloverKey( "button_target_weakest" );
		}
		if ( hMember->GetMind()->GetFocusOrders() == FO_CLOSEST ) 
		{
			pText->SetText( gpwtranslate( $MSG$ "Target Closest" ) );			
			pRadioFoClosest->SetMidState( bFoDiff );			
			pRadioFoClosest->SetCheck( true );
			pButton->SetRolloverKey( "button_target_closest" );
		}

		if ( bFoStrongest )
		{
			pRadioFoStrongest->SetMidState( true );
		}
		if ( bFoWeakest )
		{
			pRadioFoWeakest->SetMidState( true );
		}
		if ( bFoClosest )
		{
			pRadioFoClosest->SetMidState( true );
		}
	}	
	
	m_bInitializingOrders = false;
}


void UIPartyManager::SetCommandsActive( bool bSet )
{
	m_bCommandsActive = bSet;
	gUIShell.ShowGroup( "orders", bSet );
	
	SetFormationsActive( bSet );

	UIWindow * pWindow1 = gUIShell.FindUIWindow( "button_field_commands_min", "field_commands" );
	UIWindow * pWindow2 = gUIShell.FindUIWindow( "button_field_commands_max", "field_commands" );
	if ( pWindow1 && pWindow2 )
	{
		if ( bSet )
		{
			pWindow1->SetVisible( true );
			pWindow2->SetVisible( false );
		}
		else
		{
			pWindow1->SetVisible( false );
			pWindow2->SetVisible( true );
		}
	}
}


void UIPartyManager::SetFormationsActive( bool bSet )
{
	m_bFormationsActive = bSet;
	gUIShell.ShowGroup( "formations", bSet );		

	if ( bSet )
	{

		Formation * formation = gServer.GetScreenFormation();
		if ( formation )
		{
			if ( formation->GetShape().same_no_case( "line" ) )
			{
				UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "button_formation_row", "field_commands" );
				if ( !pRadio->GetCheck() )
				{
					pRadio->SetCheck( true );
				}
			}
			else if ( formation->GetShape().same_no_case( "double_line" ) )
			{
				UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "button_formation_double_row", "field_commands" );
				if ( !pRadio->GetCheck() )
				{
					pRadio->SetCheck( true );
				}
			}
			else if ( formation->GetShape().same_no_case( "column" ) )
			{
				UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "button_formation_column", "field_commands" );
				if ( !pRadio->GetCheck() )
				{
					pRadio->SetCheck( true );
				}
			}
			else if ( formation->GetShape().same_no_case( "double_column" ) )
			{
				UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "button_formation_double_column", "field_commands" );
				if ( !pRadio->GetCheck() )
				{
					pRadio->SetCheck( true );
				}
			}
			else if ( formation->GetShape().same_no_case( "wedge" ) )
			{
				UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "button_formation_pyramid", "field_commands" );
				if ( !pRadio->GetCheck() )
				{
					pRadio->SetCheck( true );
				}
			}
			else if ( formation->GetShape().same_no_case( "circle" ) )
			{
				UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "button_formation_circle", "field_commands" );
				if ( !pRadio->GetCheck() )
				{
					pRadio->SetCheck( true );
				}
			}
		}
	}
}


void UIPartyManager::InitUserEdTooltips()
{
	FastFuelHandle hUserEd( "ui:user_education_tooltips" );
	if ( hUserEd.IsValid() )
	{
		FastFuelHandle hItem = hUserEd.GetChildNamed( "item_rollover" );
		if ( hItem.IsValid() )
		{
			FastFuelFindHandle hFind( hItem );
			if ( hFind.FindFirstKeyAndValue() )
			{
				gpstring sKey;
				gpstring sValue;
				while ( hFind.GetNextKeyAndValue( sKey, sValue ) )
				{
					UserEdTooltip ut;
					ut.sEvent = sKey;
					ut.sTip = sValue;

					m_userItemEdMap.push_back( ut );
				}
			}
		}

		FastFuelHandle hGui = hUserEd.GetChildNamed( "gui_rollover" );
		if ( hGui.IsValid() )
		{
			FastFuelFindHandle hFind( hGui );
			if ( hFind.FindFirstKeyAndValue() )
			{
				gpstring sKey;
				gpstring sValue;
				while ( hFind.GetNextKeyAndValue( sKey, sValue ) )
				{
					UserEdTooltip ut;
					ut.sEvent = sKey;
					ut.sTip = sValue;

					m_userGuiEdMap.push_back( ut );
				}
			}
		}
	}

	std::sort( m_userItemEdMap.begin(), m_userItemEdMap.end(), strmap_less() );
	std::sort( m_userGuiEdMap.begin(), m_userGuiEdMap.end(), strmap_less() );	
}


gpwstring UIPartyManager::RetrieveItemUserEdTooltip( gpstring sEvent )
{
	if ( !gUIShell.GetToolTipsVisible() )
	{
		return ( L"" );
	}

	UserEdMap::iterator i = stdx::binary_search( m_userItemEdMap.begin(), m_userItemEdMap.end(), sEvent, strmap_less() );
	if ( i != m_userItemEdMap.end() )
	{
		gpwstring sTranslated = ReportSys::TranslateW( (*i).sTip.c_str() );

		unsigned int macroBegin = sTranslated.find( L'<' );
		unsigned int macroEnd	= sTranslated.find( L'>' );
		while ( macroBegin != gpwstring::npos && macroEnd != gpwstring::npos )
		{
			gpwstring sMacro = sTranslated.substr( macroBegin+1, (macroEnd-macroBegin)-1 );

			if ( sMacro.same_no_case( L"weapon_binding_get" ) )
			{
				gpstring sSave, sRecall;
				gUIInventoryManager.GetWeaponBinding( gUIInventoryManager.GetGUIRolloverGOID(), sSave, sRecall );
				sMacro = ToUnicode( sRecall );
			}
			else if ( sMacro.same_no_case( L"weapon_binding_set" ) )
			{
				gpstring sSave, sRecall;
				gUIInventoryManager.GetWeaponBinding( gUIInventoryManager.GetGUIRolloverGOID(), sSave, sRecall );
				sMacro = ToUnicode( sSave );
			}
			else if ( sMacro.same_no_case( L"portrait_binding_get" ) )
			{
				gpstring sSave, sRecall;
				GetGroupBinding( gUIInventoryManager.GetGUIRolloverGOID(), sSave, sRecall );
				sMacro = ToUnicode( sRecall );
			}
			else if ( sMacro.same_no_case( L"portrait_binding_set" ) )
			{
				gpstring sSave, sRecall;
				GetGroupBinding( gUIInventoryManager.GetGUIRolloverGOID(), sSave, sRecall );
				sMacro = ToUnicode( sSave );
			}

			Input input;
			gpwstring sKey;
			if ( gUIGame.GetInputBinder().GetInputBindingForGui( ToAnsi( sMacro ), input ) )
			{
				gpwstring sQualifier;				
				if ( input.GetKey().GetControlKey() && input.GetKey().GetKey() != Keys::KEY_CONTROL )
				{
					sQualifier.append( gpwtranslate( $MSG$ "Ctrl" ) );					
				}
				if ( input.GetKey().GetShiftKey() && input.GetKey().GetKey() != Keys::KEY_SHIFT )
				{
					sQualifier.append( gpwtranslate( $MSG$ "Shift" ) );					
				}
				if ( input.GetKey().GetAltKey() && input.GetKey().GetKey() != Keys::KEY_ALT )
				{
					sQualifier.append( gpwtranslate( $MSG$ "Alt" ) );					
				}

				if ( !sQualifier.empty() )
				{
					sKey.appendf( L"%s-%s", sQualifier.c_str(), Keys::ToScreenString( input.GetKey().GetKey() ).c_str() );				
				}
				else
				{
					sKey.appendf( L"%s", Keys::ToScreenString( input.GetKey().GetKey() ).c_str() );				
				}
			}						
			
			if ( sKey.empty() )
			{
				if ( sTranslated[macroBegin-1] == L'(' )
				{
					macroBegin -= 2;
					macroEnd += 1;
				}
			}

			gpwstring sBegin	= sTranslated.substr( 0, macroBegin );
			sBegin.append( sKey );
			gpwstring sEnd		= sTranslated.substr( macroEnd+1, sTranslated.size() );			
			

			sTranslated = sBegin;
			sTranslated += sEnd;
						
			macroBegin	= sTranslated.find( L'<' );
			macroEnd	= sTranslated.find( L'>' );
		}	
		
		return sTranslated;
	}
	
	return ( L"" );
}


gpwstring UIPartyManager::RetrieveGuiUserEdTooltip( gpstring sEvent )
{
	if ( !gUIShell.GetToolTipsVisible() )
	{
		return ( L"" );
	}

	UserEdMap::iterator i = stdx::binary_search( m_userGuiEdMap.begin(), m_userGuiEdMap.end(), sEvent, strmap_less() );
	if ( i != m_userGuiEdMap.end() )
	{
		gpwstring sTranslated = ReportSys::TranslateW( (*i).sTip.c_str() );

		unsigned int macroBegin = sTranslated.find( L'<' );
		unsigned int macroEnd	= sTranslated.find( L'>' );
		while ( macroBegin != gpwstring::npos && macroEnd != gpwstring::npos )
		{
			gpwstring sMacro = sTranslated.substr( macroBegin+1, (macroEnd-macroBegin)-1 );
			
			Input input;
			gpwstring sKey;
			if ( gUIGame.GetInputBinder().GetInputBindingForGui( ToAnsi( sMacro ), input ) )
			{
				gpwstring sQualifier;				
				if ( input.GetKey().GetControlKey() && input.GetKey().GetKey() != Keys::KEY_CONTROL )
				{
					sQualifier.append( gpwtranslate( $MSG$ "Ctrl" ) );					
				}
				if ( input.GetKey().GetShiftKey() && input.GetKey().GetKey() != Keys::KEY_SHIFT )
				{
					sQualifier.append( gpwtranslate( $MSG$ "Shift" ) );					
				}
				if ( input.GetKey().GetAltKey() && input.GetKey().GetKey() != Keys::KEY_ALT )
				{
					sQualifier.append( gpwtranslate( $MSG$ "Alt" ) );					
				}

				if ( !sQualifier.empty() )
				{
					sKey.appendf( L"%s-%s", sQualifier.c_str(), Keys::ToScreenString( input.GetKey().GetKey() ).c_str() );				
				}
				else
				{
					sKey.appendf( L"%s", Keys::ToScreenString( input.GetKey().GetKey() ).c_str() );				
				}
			}	
			
			if ( sKey.empty() )
			{
				if ( sTranslated[macroBegin-1] == L'(' )
				{
					macroBegin -= 2;
					macroEnd += 1;
				}
			}

			gpwstring sBegin	= sTranslated.substr( 0, macroBegin );
			sBegin.append( sKey );
			gpwstring sEnd		= sTranslated.substr( macroEnd+1, sTranslated.size() );			
			
			sTranslated = sBegin;
			sTranslated += sEnd;
						
			macroBegin = sTranslated.find( L'<' );
			macroEnd	= sTranslated.find( L'>' );
		}	
		
		return sTranslated;
	}
	
	return ( L"" );
}

void UIPartyManager::ShowEquipment()
{
	gUIShell.ShowGroup( "equipment", true );

	if ( m_UIPartyMembers[m_SelectedMember] )
	{				
		if ( m_UIPartyMembers[m_SelectedMember]->IsSpellsExpanded() )
		{
			UIWindow * pWindow = gUIShell.FindUIWindow( "button_spellbook_hide", "character" );
			if ( pWindow )
			{
				pWindow->SetVisible( true );
			}
			pWindow = gUIShell.FindUIWindow( "button_spellbook_show", "character" );
			if ( pWindow )
			{
				pWindow->SetVisible( false );
			}
		}
		else
		{
			UIWindow * pWindow = gUIShell.FindUIWindow( "button_spellbook_hide", "character" );
			if ( pWindow )
			{
				pWindow->SetVisible( false );
			}
			pWindow = gUIShell.FindUIWindow( "button_spellbook_show", "character" );
			if ( pWindow )
			{
				pWindow->SetVisible( true );
			}
		}
	}

	gUIInventoryManager.UpdateActiveEquipment( m_SelectedMember );
	
	m_bEquipmentVisible = true;
}


void UIPartyManager::HideEquipment()
{
	gUIShell.ShowGroup( "equipment", false );

	if ( m_UIPartyMembers[m_SelectedMember] )
	{				
		if ( m_UIPartyMembers[m_SelectedMember]->IsSpellsExpanded() )
		{
			UIWindow * pWindow = gUIShell.FindUIWindow( "button_spellbook_hide", "character" );
			if ( pWindow )
			{
				pWindow->SetVisible( false );
			}
			pWindow = gUIShell.FindUIWindow( "button_spellbook_show", "character" );
			if ( pWindow )
			{
				pWindow->SetVisible( false );
			}
		}
		else
		{
			UIWindow * pWindow = gUIShell.FindUIWindow( "button_spellbook_hide", "character" );
			if ( pWindow )
			{
				pWindow->SetVisible( false );
			}
			pWindow = gUIShell.FindUIWindow( "button_spellbook_show", "character" );
			if ( pWindow )
			{
				pWindow->SetVisible( false );
			}
		}
	}

	m_bEquipmentVisible = false;
}


void UIPartyManager::OnScreenSize()
{
	for ( int i = 0; i < MAX_PARTY_MEMBERS; ++i ) 
	{
		if ( m_UIPartyMembers[i] ) 
		{
			GoHandle hMember( m_UIPartyMembers[i]->GetGoid() );
			if ( hMember.IsValid() )
			{
				m_UIPartyMembers[i]->GetUICharacterDisplay()->OnScreenSize();				
			}
		}
	}	
}


void UIPartyManager::HandleMenusFromCommand()
{
	if ( gWorldState.GetCurrentState() == WS_RELOADING )
	{
		return;
	}

	// If a store interface is up, we must shut it down
	gUIStoreManager.SetAutoActivator( GOID_INVALID );
	if ( gUIStoreManager.GetStoreActive() )
	{
		gUIStoreManager.DeactivateStore();
	}
	else if ( gUIInventoryManager.GetStashActive() )
	{
		gUIInventoryManager.CloseStash();
	}
	else if ( gUIDialogueHandler.GetTalkerGUID() != GOID_INVALID )
	{
		gUIDialogueHandler.ExitDialogue( true, true );
	}
}


bool UIPartyManager::CollectLoot()
{	
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	HandleMenusFromCommand();

	GoidColl partyColl;
	GetSelectedPartyMembers( partyColl );

	gAIAction.RSCollectLoot( partyColl );

	return true;
}


bool UIPartyManager::CycleFormations()
{
	if( gServer.GetScreenPlayer() && gServer.GetScreenParty() )
	{
		gpstring sShape = gServer.GetScreenParty()->GetParty()->GetFormation()->GetShape();		

		if ( sShape.same_no_case( "line" ) )
		{
			UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "button_formation_double_row", "field_commands" );
			pRadio->SetCheck( true );
			gUIPartyManager.ArrangeFormation( "double_line" );
		}
		else if ( sShape.same_no_case( "double_line" ) )
		{
			UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "button_formation_column", "field_commands" );
			pRadio->SetCheck( true );
			gUIPartyManager.ArrangeFormation( "column" );
		}
		else if ( sShape.same_no_case( "column" ) )
		{
			UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "button_formation_double_column", "field_commands" );
			pRadio->SetCheck( true );
			gUIPartyManager.ArrangeFormation( "double_column" );
		}
		else if ( sShape.same_no_case( "double_column" ) )
		{
			UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "button_formation_pyramid", "field_commands" );
			pRadio->SetCheck( true );
			gUIPartyManager.ArrangeFormation( "wedge" );
		}
		else if ( sShape.same_no_case( "wedge" ) )
		{
			UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "button_formation_circle", "field_commands" );
			pRadio->SetCheck( true );
			gUIPartyManager.ArrangeFormation( "circle" );
		}		
		else if ( sShape.same_no_case( "circle" ) )
		{
			UIRadioButton * pRadio = (UIRadioButton *)gUIShell.FindUIWindow( "button_formation_row", "field_commands" );
			pRadio->SetCheck( true );
			gUIPartyManager.ArrangeFormation( "line" );
		}
	}
	
	return true;
}


bool UIPartyManager::IsInDataDockBar()
{
	if ( m_pWindow[DATA_DOCKBAR] )
	{
		POINT pt;
		pt.x = gAppModule.GetCursorX();
		pt.y = gAppModule.GetCursorY();
		if ( m_pWindow[DATA_DOCKBAR]->GetRect().Contains( pt ) )
		{
			return true;
		}
	}

	return false;
}


void UIPartyManager::EndDataBarDock()
{
	if ( m_pWindow[DATA_DOCKBAR] )
	{
		((UIDockbar *)m_pWindow[DATA_DOCKBAR])->EndDock();
	}
}


Goid UIPartyManager::DoesActivePartyHaveTemplate( const char * sTemplate, Goid target )
{
	Goid party = GOID_INVALID;
	if ( target == GOID_INVALID )
	{
		party = gServer.GetScreenParty() ? gServer.GetScreenParty()->GetGoid() : GOID_INVALID;
	}
	else
	{
		GoHandle hTarget( target );
		if ( hTarget.IsValid() && hTarget->HasParent() )
		{
			party = hTarget->GetParent()->GetGoid();
		}
	}

	GoHandle hParty( party );
	if ( hParty.IsValid() )
	{	
		GopColl partyColl = hParty->GetChildren();
		GopColl::iterator iMember; 
		for ( iMember = partyColl.begin(); iMember != partyColl.end(); ++iMember )
		{				
			if ( ::IsMultiPlayer() || ( ::IsSinglePlayer() && (*iMember)->IsInAnyScreenWorldFrustum() ) )
			{
				GopColl items;
				GopColl::iterator iItem;
				(*iMember)->GetInventory()->ListItems( IL_ALL, items );
				for ( iItem = items.begin(); iItem != items.end(); ++iItem )
				{
					if ( gpstring( sTemplate ).same_no_case( (*iItem)->GetTemplateName() ) )
					{ 
						return (*iItem)->GetGoid();
					}			
				}
			}
		}
	}	

	return GOID_INVALID;
}


void UIPartyManager::SetFollowMode( bool bSet )
{
	m_bFollowMode = bSet; 

	for ( int i = 0; i != MAX_PARTY_MEMBERS; ++i )
	{
		if ( m_UIPartyMembers[i] && m_UIPartyMembers[i]->GetGoid() != GOID_INVALID )
		{
			GoHandle hMember( m_UIPartyMembers[i]->GetGoid() );
			if ( hMember.IsValid() )
			{
				hMember->GetMind()->RSStop( AO_REFLEX );
			}
		}
	}
}


void UIPartyManager::AssignDefaultGroupKey( int index, Goid member )
{
	if ( m_group[index].group.size() == 0 )
	{
		m_group[index].focusGoid = member;
		m_group[index].group.push_back( member );
		m_group[index].bExplicitlySet = false;		
	}
}


void UIPartyManager::GetGroupBinding( Goid member, gpstring & sSave, gpstring & sRecall )
{
	for ( int index = 0; index != MAX_PARTY_MEMBERS; ++index )
	{
		if ( m_group[index].bExplicitlySet ) 

		{
			GoidColl::iterator i;
			for ( i = m_group[index].group.begin(); i != m_group[index].group.end(); ++i )
			{
				if ( member == *i )
				{
					sSave.assignf( "set_group_%d", index + 1 );
					sRecall.assignf( "get_group_%d", index + 1);
				}
			}
		}
		else
		{
			// do the portrait stuff here
			for ( int i = 0; i != MAX_PARTY_MEMBERS; ++i )
			{
				if ( m_UIPartyMembers[i] && m_UIPartyMembers[i]->GetGoid() != GOID_INVALID )
				{
					GoHandle hMember( m_UIPartyMembers[i]->GetGoid() );
					if ( hMember.IsValid() && hMember->IsScreenPartyMember() && member == hMember->GetGoid() )
					{
						int rank = GetMemberPortraitRank( hMember );													
						sSave.assignf( "set_group_%d", rank + 1 );
						sRecall.assignf( "get_group_%d", rank + 1);				
					}
				}
			}

		}
	}
}


void UIPartyManager::ResetPartyGridScale()
{
	GoidColl partyColl;
	GetPortraitOrderedParty( partyColl );
	
	// Put all selected party members into our collection
	GoidColl::const_iterator i;
	for ( i = partyColl.begin(); i != partyColl.end(); ++i )
	{
		GoHandle hMember( *i );
		if ( hMember.IsValid() )
		{		
			int height = hMember->GetInventory()->GetGridbox()->GetHeight();

			if ( height < GetFullInventoryHeight() ) 
			{
				hMember->GetInventory()->GetGridbox()->SetScale( 1.0 );

				// Set the scale of the inventory headers
				for ( int o = 0; o < MAX_PARTY_MEMBERS; ++o ) 
				{
					if ( m_UIPartyMembers[o] && ( m_UIPartyMembers[o]->GetGoid() == hMember->GetGoid() )) 
					{
						gpstring sName;
						gpstring sWindow;
						if ( hMember->GetInventory()->IsPackOnly() )
						{
							sName.assignf( "multi_inventory_packmule_%d", o+1 );						
							sWindow.assignf( "dialog_box_inv_bg_packmule_%d", o+1 );
						}
						else
						{
							sName.assignf( "multi_inventory_human_%d", o+1 );						
							sWindow.assignf( "dialog_box_inv_bg_human_%d", o+1 );
						}						
						
						gUI.GetGameGUI().SetGroupScaleToAlignWithWindow( sName, sWindow, 2.0 );
					}
				}			
			}
		}
	}
}


void UIPartyManager::RolloverHighlightCb( bool & bHighlight )
{
	if ( gUIGame.GetUICommands()->GetActiveContextCursor() == CURSOR_GUI_CAST )
	{
		bHighlight = true;
	}
	else
	{
		bHighlight = false;
	}
}


void UIPartyManager::PartyCb()
{
	InitOrders();
}


void UIPartyManager::SAddCharacterAsGuarded( Goid source, Goid character )
{
	CHECK_SERVER_ONLY;

	GoHandle hSource( source );
	if ( hSource && hSource->GetPlayer() && !hSource->GetPlayer()->IsComputerPlayer() )
	{
		RCAddCharacterAsGuarded( character, hSource->GetPlayer()->GetMachineId() );
	}
}


void UIPartyManager::RCAddCharacterAsGuarded( Goid character, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCAddCharacterAsGuarded, machineId );

	AddCharacterAsGuarded( character );
}


void UIPartyManager::AddCharacterAsGuarded( Goid character )
{
	// Safety first - make sure that the character isn't already in the collection
	GoidColl::iterator iGuarded;
	for ( iGuarded = m_GuardedCharacters.begin(); iGuarded != m_GuardedCharacters.end(); ++iGuarded )
	{
		if ( (*iGuarded) == character )
		{
			return;
		}
	}

	m_GuardedCharacters.push_back( character );	
}


void UIPartyManager::SRemoveCharacterAsGuarded( Goid source, Goid character )
{
	CHECK_SERVER_ONLY;

	GoHandle hSource( source );
	if ( hSource && hSource->GetPlayer() && !hSource->GetPlayer()->IsComputerPlayer() )
	{
		RCRemoveCharacterAsGuarded( character, hSource->GetPlayer()->GetMachineId() );
	}
}


void UIPartyManager::RCRemoveCharacterAsGuarded( Goid character, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCRemoveCharacterAsGuarded, machineId );

	RemoveCharacterAsGuarded( character );
}


void UIPartyManager::RemoveCharacterAsGuarded( Goid character )
{
	GoidColl::iterator iGuarded;
	for ( iGuarded = m_GuardedCharacters.begin(); iGuarded != m_GuardedCharacters.end(); ++iGuarded )
	{
		if ( (*iGuarded) == character )
		{
			m_GuardedCharacters.erase( iGuarded );
			return;
		}
	}
}


void UIPartyManager::AdjustAwpToggleLocation( void )
{
	GoidColl partyColl;
	GetPortraitOrderedParty( partyColl );
	// let's move the awp expand button beneath all the portraits.
	GoidColl::iterator last = partyColl.end()-1;
	if ( partyColl.size() != 0 )
	{
		int index = 0;		
		if ( GetCharacterFromGoid( *last, index ) )
		{
			UIWindow * pAwpMin = gUIShell.FindUIWindow( "button_awp_minimize", "character_awp" );
			UIWindow * pAwpMax = gUIShell.FindUIWindow( "button_awp_maximize", "character_awp" );
			int portraitBottom = m_UIPartyMembers[index]->GetGUIWindow( GUI_PORTRAIT )->GetRect().bottom;

			int height = pAwpMin->GetRect().bottom - pAwpMin->GetRect().top;
			pAwpMin->SetRect( pAwpMin->GetRect().left, pAwpMin->GetRect().right, portraitBottom + 20, portraitBottom + 20 + height );			
			pAwpMax->SetRect( pAwpMax->GetRect().left, pAwpMax->GetRect().right, portraitBottom + 20, portraitBottom + 20 + height );
		}
	}
}


void UIPartyManager::CastCb( Goid spell )
{
	GoHandle hSpell( spell );
	if ( hSpell )
	{
		Go * pMember = hSpell->GetParent()->GetParent();
		if ( pMember )
		{
			int index = 0;
			if ( GetCharacterFromGoid( pMember->GetGoid(), index ) )
			{
				if ( pMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL )
				{
					gUIAnimation.AddClockAnimation( hSpell->GetMagic()->GetCastReloadDelay(),
													m_UIPartyMembers[index]->GetGUIWindow( GUI_AIS_PRIMARY_SPELL ),
													0x7700ff00 );
				}
				else if ( pMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL )
				{
					gUIAnimation.AddClockAnimation( hSpell->GetMagic()->GetCastReloadDelay(),
													m_UIPartyMembers[index]->GetGUIWindow( GUI_AIS_SECONDARY_SPELL ),
													0x7700ff00 );
				}
				
				gUIAnimation.AddClockAnimation( hSpell->GetMagic()->GetCastReloadDelay(),
													m_UIPartyMembers[index]->GetGUIWindow( GUI_ACTIVE_MINIMIZED ),
													0x7700ff00 );
			}
		}
	}
}


