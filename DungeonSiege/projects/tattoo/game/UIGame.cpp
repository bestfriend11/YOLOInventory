/**************************************************************************

Filename		: UIGame.cpp

Class			: UIGame

Description		: Handles all the in-game interface elements.  This includes
				  the GUI, and all user input handling

Last Modified	: 2/14/00

Author			: Bartosz Kijanka, Chad Queen

**************************************************************************/


// Include Files
#include "Precomp_Game.h"

#include "AiAction.h"
#include "AiQuery.h"
#include "AppModule.h"
#include "DebugProbe.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "GameAuditor.h"
#include "GpConsole.h"
#include "Go.h"
#include "GoActor.h"
#include "GoAspect.h"
#include "GoAttack.h"
#include "GoBody.h"
#include "GoCore.h"
#include "GoData.h"
#include "GoDb.h"
#include "GoDefs.h"
#include "GoFollower.h"
#include "GoInventory.h"
#include "GoParty.h"
#include "GoPhysics.h"
#include "GoMind.h"
#include "gps_manager.h"
#include "GuiHelper.h"
#include "InputBinder.h"
#include "MCP.h"
#include "mood.h"
#include "Nema_Aspect.h"
#include "Nema_Blender.h"
#include "Nema_Chore.h"
#include "NetPipe.h"
#include "Player.h"
#include "RatioStack.h"
#include "Server.h"
#include "Services.h"
#include "siege_compass.h"
#include "Siege_Console.h"
#include "siege_frustum.h"
#include "siege_hotpoint_database.h"
#include "Siege_Mouse_shadow.h"
#include "siege_options.h"
#include "StdHelp.h"
#include "TattooGame.h"
#include "UI_ChatBox.h"
#include "UI_ComboBox.h"
#include "UI_EditBox.h"
#include "UI_GridBox.h"
#include "UI_InfoSlot.h"
#include "UI_ItemSlot.h"
#include "UI_ListBox.h"
#include "UI_ListReport.h"
#include "UI_Messenger.h"
#include "UI_mouse_listener.h"
#include "UI_PopupMenu.h"
#include "UI_Shell.h"
#include "UI_StatusBar.h"
#include "UI_TextBox.h"
#include "UI_Window.h"
#include "UICamera.h"
#include "UICharacterDisplay.h"
#include "UICommands.h"
#include "UIEmoteList.h"
#include "UIFrontEnd.h"
#include "UIGame.h"
#include "UIGame_Console_Commands.h"
#include "UIGameConsole.h"
#include "UIInventoryManager.h"
#include "UIItemOverlay.h"
#include "UIMenuManager.h"
#include "UIMultiplayer.h"
#include "UIZoneMatch.h"
#include "UIPlayerRanks.h"
#include "UIOptions.h"
#include "UIPartyManager.h"
#include "UIPartyMember.h"
#include "UIStoreManager.h"
#include "UITeamManager.h"
#include "Victory.h"
#include "weather.h"
#include "WorldFx.h"
#include "WorldMap.h"
#include "WorldMessage.h"
#include "WorldOptions.h"
#include "WorldSound.h"
#include "WorldState.h"
#include "WorldTerrain.h"
#include "WorldTime.h"


// Namespaces
using namespace siege;

const unsigned int APPROX_NODE_SIZE = 16;


/**************************************************************************

Function		: UIGame()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIGame :: UIGame()
	:	m_LastHitGO( GOID_INVALID )
	,	m_bLastHitTerrainPositionValid( false )
	,	m_bSelectionDown( false )
	,	m_bSpecialDown( false )
	,	m_bContextDown( false )	
	,	m_bInvertCameraXAxis( false )
	,	m_bInvertCameraYAxis( false )
	,	m_BackendMode( BMODE_NONE )
	,	m_bIsVisible( false )
	,	m_bActive( false )
	,	m_sInfoMode( "screen_name" )
	,	m_bEditBoxVisible( false )
	,	m_bPlayerRanksVisible( false )
	,	m_bPlayerLabelsVisible( true )
	,   m_bGameTimerVisible( false )
	,	m_pUIInventoryManager( 0 )
	,	m_pUIMenuManager( 0 )
	,	m_pUIStoreManager( 0 )
	,	m_pUITeamManager( 0 )
	,	m_pUIItemOverlay( 0 )
	,	m_pUIGameConsole( 0 )
	,	m_bCameraScreenEdgeRotate( true )
	,	m_bCameraKeyRotating( false )
	,	m_bScreenHorizTrack( true )
	,	m_bScreenVertTrack( true )
	,	m_AreaOfEffectTarget( GOID_INVALID )
	,	m_AreaOfEffectScriptID( 0 )
	,	m_TimeToDisplayFormation( DBL_MAX )
	,	m_rolloverTime( 0 )
	,	m_rolloverGoid( GOID_INVALID )
	,	m_bAutoInventoryUp( false )
	,	m_bFormationMode( false )
	,	m_camHorizontalEdgeBuffer( 0.98f )
	,	m_camVerticalEdgeBuffer( 0.98f )
	,	m_bLastPause( false )
	,	m_bOneShotCharacterInit( false )
	,	m_progressNodeCount( 0 )
	,	m_DefeatFadeOut( DEFEAT_FADE_OUT_TIME )		
	,	m_DragTimer( 0.0f )
	,	m_SelectionMode( SELECTION_INVALID )
	,   m_DragX( 0 )
	,	m_DragY( 0 )
	,	m_oldCameraMode( CMODE_NONE )
	,	m_BackendDialog( DIALOG_NONE )
	,	m_BackendDialogJustShown( false )
	,	m_MapShowBorderMeters( 0.0f )
	,	m_bFadedOut( false )
	,	m_pDefeatBg( 0 )
	,	m_bResizeLabels( true )
	,	m_bJipLoad( false )
	,	m_bEscNisToFrontEnd( false )
{
	
#	if !GP_RETAIL
	// Register console command from this UI layer
	m_pConsoleCommands		= new UIGame_console_commands;

	m_pPolyText = 0;
	m_bPolyAlert = false;

	// Initialize the LOD to Polygon count map
	FastFuelHandle hPolyCount( "config:debug_polymap" );
	if ( hPolyCount.IsValid() )
	{
		FastFuelFindHandle hFind( hPolyCount );
		if ( hFind.IsValid() )
		{
			if ( hFind.FindFirstKeyAndValue() )
			{
				gpstring sKey;
				gpstring sValue;
				while ( hFind.GetNextKeyAndValue( sKey, sValue ) )
				{
					float lod = 0.0f;
					int polys = 0;
					stringtool::Get( sKey, lod );
					stringtool::Get( sValue, polys );
					
					m_LodToPolyMap.insert( std::make_pair( lod, polys ) );
				}
			}
		}
	}

#	endif // !GP_RETAIL

	// Initialize our camera
	m_pUICamera				= new UICamera;

	// Initialize our command handler
	m_pUICommands			= new UICommands;

	// Initialize our helpers
	m_pUIPartyManager		= new UIPartyManager;
	m_pUIInventoryManager	= new UIInventoryManager;
	m_pUIMenuManager		= new UIMenuManager;
	m_pUIStoreManager		= new UIStoreManager;
	m_pUIGameConsole		= new UIGameConsole;
	m_pUIPlayerRanks		= new UIPlayerRanks;
	m_pUIEmoteList			= new UIEmoteList;
	m_pUITeamManager		= new UITeamManager;	

	// Input Bindings
	m_pGameInputBinder		= new InputBinder( "ingamedefaults" );
	gAppModule.RegisterBinder( m_pGameInputBinder, 999 );

	m_pUserInputBinder			= new InputBinder( "game" );
	gAppModule.RegisterBinder( m_pUserInputBinder, 1000 );

	m_pNisInputBinder		= new InputBinder( "nis" );
	gAppModule.RegisterBinder( m_pNisInputBinder, 1001 );

	PublishInterfaceToInputBinder();
	BindInputToPublishedInterface();

	m_RolloverObjData.object		= GOID_INVALID;
	m_RolloverObjData.pText			= 0;
	m_RolloverObjData.secondsTotal	= 0.0f;

	// Object selection indicators

	// Read the selection indicator colors out of a gas file
	FastFuelHandle handle( "config:global_settings" );
	FastFuelHandle hBlock = handle.GetChildNamed( "selection_indicators" );

	gpstring sTextureName;

	if ( hBlock.Get( "texture_default", sTextureName, false ) )
	{
		m_SelectionTextureDefault = gSiegeEngine.Renderer().CreateTexture( sTextureName, false, 0, TEX_LOCKED );
	} 
	else 
	{
		gperrorf(("Failed to load default selection circle texture from path: %s", sTextureName.c_str() ));
		m_SelectionTextureDefault = 0;
	}

	if ( hBlock.Get( "texture_focus", sTextureName, false ) )
	{
		m_SelectionTextureFocus = gSiegeEngine.Renderer().CreateTexture( sTextureName, false, 0, TEX_LOCKED );
	} 
	else 
	{
		gperrorf(("Failed to load default focus selection texture from path: %s", sTextureName.c_str() ));
		m_SelectionTextureFocus = 0;
	}

	gpstring sColor;

	if( hBlock.Get( "item_color", sColor ) ) 
	{
		if( stringtool::GetNumDelimitedStrings( sColor, ',' ) == 3 ) 
		{
			vector_3	color;
			stringtool::GetDelimitedValue( sColor, ',', 0, color.x );
			stringtool::GetDelimitedValue( sColor, ',', 1, color.y );
			stringtool::GetDelimitedValue( sColor, ',', 2, color.z );
			m_Item_selector_color = MAKEDWORDCOLOR( color );
		}
	} 
	else 
	{
		m_Item_selector_color				= 0xff00008f;
	}

	if ( hBlock.Get( "enemy_color", sColor ) ) {
		if ( stringtool::GetNumDelimitedStrings( sColor, ',' ) == 3 ) 
		{
			vector_3	color;
			stringtool::GetDelimitedValue( sColor, ',', 0, color.x );
			stringtool::GetDelimitedValue( sColor, ',', 1, color.y );
			stringtool::GetDelimitedValue( sColor, ',', 2, color.z );
			m_Enemy_selector_color = MAKEDWORDCOLOR( color );
		}
	} 
	else 
	{
		m_Enemy_selector_color				= 0xff8f0000;
	}

	if( hBlock.Get( "friend_color", sColor ) ) 
	{
		if( stringtool::GetNumDelimitedStrings( sColor, ',' ) == 3 ) 
		{
			vector_3	color;
			stringtool::GetDelimitedValue( sColor, ',', 0, color.x );
			stringtool::GetDelimitedValue( sColor, ',', 1, color.y );
			stringtool::GetDelimitedValue( sColor, ',', 2, color.z );
			m_Friend_selector_color = MAKEDWORDCOLOR( color );
		}
	} 
	else 
	{
		m_Friend_selector_color	= 0xff8f0000;
	}

	if ( hBlock.Get( "party_member_color", sColor ) ) {
		if ( stringtool::GetNumDelimitedStrings( sColor, ',' ) == 3 ) 
		{
			vector_3	color;
			stringtool::GetDelimitedValue( sColor, ',', 0, color.x );
			stringtool::GetDelimitedValue( sColor, ',', 1, color.y );
			stringtool::GetDelimitedValue( sColor, ',', 2, color.z );
			m_Party_member_selector_color = MAKEDWORDCOLOR( color );
		}
	} 
	else 
	{
		m_Party_member_selector_color		= 0xff008f00;
	}

	if(	hBlock.Get( "focus_go_color", sColor ) ) 
	{
		if( stringtool::GetNumDelimitedStrings( sColor, ',' ) == 3 )
		{
			vector_3	color;
			stringtool::GetDelimitedValue( sColor, ',', 0, color.x );
			stringtool::GetDelimitedValue( sColor, ',', 1, color.y );
			stringtool::GetDelimitedValue( sColor, ',', 2, color.z );
			m_Focus_go_selector_color = MAKEDWORDCOLOR( color );
		}
	} 
	else 
	{
		m_Focus_go_selector_color			= 0xff00ff00;
	}

	if ( hBlock.Get( "dead_enemy_member_color", sColor ) ) 
	{
		if( stringtool::GetNumDelimitedStrings( sColor, ',' ) == 3 ) 
		{
			vector_3	color;
			stringtool::GetDelimitedValue( sColor, ',', 0, color.x );
			stringtool::GetDelimitedValue( sColor, ',', 1, color.y );
			stringtool::GetDelimitedValue( sColor, ',', 2, color.z );
			m_Dead_enemy_selector_color = MAKEDWORDCOLOR( color );
		}
	}
	else 
	{
		m_Dead_enemy_selector_color			= 0xff000000;
	}

	if ( hBlock.Get( "dead_friend_member_color", sColor ) ) 
	{
		if( stringtool::GetNumDelimitedStrings( sColor, ',' ) == 3 ) 
		{
			vector_3	color;
			stringtool::GetDelimitedValue( sColor, ',', 0, color.x );
			stringtool::GetDelimitedValue( sColor, ',', 1, color.y );
			stringtool::GetDelimitedValue( sColor, ',', 2, color.z );
			m_Dead_friend_selector_color = MAKEDWORDCOLOR( color );
		}
	}
	else 
	{
		m_Dead_friend_selector_color			= 0xff000000;
	}


	DWORD dwPortraitInactiveColor = 0;
	FastFuelHandle hPortraitColors = handle.GetChildNamed( "portrait_colors" );
	if ( hPortraitColors.IsValid() && hPortraitColors.Get( "portrait_inactive_color", dwPortraitInactiveColor ) )
	{
		gUIPartyManager.SetPortraitInactiveColor( dwPortraitInactiveColor );
	}

	DWORD dwGridInvalidColor = 0;
	DWORD dwActiveSpellBookColor = 0;	
	DWORD dwGridRollover = 0;
	DWORD dwGridRolloverOverlap = 0;
	DWORD dwGridMagicColor = 0;

	FastFuelHandle hInvHighlights = handle.GetChildNamed( "inventory_highlights" );
	if ( hInvHighlights.IsValid() )
	{
		hInvHighlights.Get( "grid_invalid", dwGridInvalidColor );
		hInvHighlights.Get( "grid_active_spell_book", dwActiveSpellBookColor );
		hInvHighlights.Get( "grid_rollover", dwGridRollover );
		hInvHighlights.Get( "grid_rollover_overlap", dwGridRolloverOverlap );
		hInvHighlights.Get( "grid_rollover_overlap", dwGridRolloverOverlap );
		hInvHighlights.Get( "grid_magic", dwGridMagicColor );

		gUIInventoryManager.SetGridInvalidColor( dwGridInvalidColor );
		gUIInventoryManager.SetActiveBookColor( dwActiveSpellBookColor );
		gUIInventoryManager.SetGridRolloverColor( dwGridRollover );
		gUIInventoryManager.SetGridRolloverOverlapColor( dwGridRolloverOverlap );
		gUIInventoryManager.SetGridMagicColor( dwGridMagicColor );
	}

	DWORD dwTextPositiveColor = 0;
	DWORD dwTextNegativeColor = 0;

	FastFuelHandle hCharHighlights = handle.GetChildNamed( "character_panel_highlights" );
	if ( hCharHighlights.IsValid() )
	{
		hCharHighlights.Get( "text_positive", dwTextPositiveColor );
		hCharHighlights.Get( "text_negative", dwTextNegativeColor );

		gUIPartyManager.SetPositiveTextColor( dwTextPositiveColor );
		gUIPartyManager.SetNegativeTextColor( dwTextNegativeColor );
	}

	const float SHORT_LEG		= -0.5f;
	const float LONG_LEG		=  1.5f;
	const float ABOVE_GROUND	=  0.125f;

	m_SelectionVerts[0].uv.u= 0;
	m_SelectionVerts[0].uv.v= 0;

	m_SelectionVerts[1].uv.u= 2;
	m_SelectionVerts[1].uv.v= 0;

	m_SelectionVerts[2].uv.u= 0;
	m_SelectionVerts[2].uv.v= 2;

	m_SelectionVerts[0].x = SHORT_LEG;
	m_SelectionVerts[0].z = SHORT_LEG;

	m_SelectionVerts[1].x = LONG_LEG;
	m_SelectionVerts[1].z = SHORT_LEG;

	m_SelectionVerts[2].x = SHORT_LEG;
	m_SelectionVerts[2].z = LONG_LEG;

	m_SelectionVerts[0].y =
	m_SelectionVerts[1].y =
	m_SelectionVerts[2].y = ABOVE_GROUND;

	FastFuelHandle hTipColors = handle.GetChildNamed( "tooltip_colors" );
	FastFuelFindHandle hFind( hTipColors );
	if ( hTipColors.IsValid() && hFind.IsValid() && hFind.FindFirstKeyAndValue() )
	{
		gpstring sKey, sValue;
		while ( hFind.GetNextKeyAndValue( sKey, sValue ) )
		{
			int color = 0;
			stringtool::Get( sValue, color );
			gUIShell.AddTipColor( sKey, color );
		}
	}

	// MP player labels
	m_pPlayerLabelText = new UIText;
	m_pPlayerLabelText->SetName( "mp_player_label" );
	m_pPlayerLabelText->SetAutoSize( true );
	m_pPlayerLabelText->SetJustification( JUSTIFY_CENTER );
	m_pPlayerLabelText->SetPassThrough( true );

	gpstring labelFont = "b_gui_fnt_12p_copperplate-light";
	FastFuelHandle hMpInGame( "config:multiplayer:ingame" );
	if ( hMpInGame.IsValid() )
	{
		labelFont = hMpInGame.GetString( "player_label_font", labelFont );
	}
	m_pPlayerLabelText->SetFont( labelFont );

	FastFuelHandle hMegaMap = handle.GetChildNamed( "mega_map" );
	if ( hMegaMap )
	{
		hMegaMap.Get( "show_border_meters", m_MapShowBorderMeters );
	}

	FastFuelHandle hLocale = handle.GetChildNamed( "locale" );
	if ( hLocale )
	{
		hLocale.Get( "resize_labels", m_bResizeLabels );
	}

	FastFuelHandle hMisc = handle.GetChildNamed( "gui_misc" );
	if ( hMisc )
	{
		int maxDisplay = 0;
		hMisc.Get( "max_generic_state_display", maxDisplay );
		gUIPartyManager.SetMaxGenericStateDisplay( maxDisplay );
	}
}



/**************************************************************************

Function		: ~UIGame()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIGame :: ~UIGame()
{
	Delete( m_pGameInputBinder );
	Delete( m_pUserInputBinder );
	Delete( m_pNisInputBinder );
#	if !GP_RETAIL
	Delete( m_pConsoleCommands );
#	endif // !GP_RETAIL
	Delete( m_pUICamera );
	Delete( m_pUICommands );
	Delete( m_pUIPartyManager );
	Delete( m_pUIInventoryManager );
	Delete( m_pUIStoreManager );
	Delete( m_pUIMenuManager );
	Delete( m_pUITeamManager );
	Delete( m_pUIItemOverlay );
	Delete( m_pUIGameConsole );
	Delete( m_pUIPlayerRanks );
	Delete( m_pUIEmoteList );
	Delete( m_pPlayerLabelText );

	if( m_SelectionTextureDefault )
	{
		gSiegeEngine.Renderer().DestroyTexture( m_SelectionTextureDefault );
	}
	if( m_SelectionTextureFocus )
	{
		gSiegeEngine.Renderer().DestroyTexture( m_SelectionTextureFocus );
	}

	if ( GoDb::DoesSingletonExist() )
	{
		gGoDb.SetSelectionChangedCb( NULL );
	}
}


void UIGame :: SetIsVisible( bool flag )
{
	m_bIsVisible = flag;
	m_pGameInputBinder->SetIsActive( flag );
	m_pUserInputBinder->SetIsActive( flag );

	if ( flag == true )
	{
		gUIShell.HideInterface( "nis_subtitle" );
	}
}


bool UIGame :: FormationIncreaseSpacing()
{
	if( GetShowFormation() && (m_BackendMode == BMODE_ASSIGN_FORMATION_POS) )
	{
		Formation * formation;
		gServer.GetScreenFormation( formation );
		if( formation )
		{
			formation->SetSpotSpacing( max( formation->GetMinSpotSpacing(), formation->GetSpotSpacing() + 0.25f ) );
		}
	}
	return true;
}


bool UIGame :: FormationDecreaseSpacing()
{
	if( GetShowFormation() && (m_BackendMode == BMODE_ASSIGN_FORMATION_POS) )
	{
		Formation * formation;
		gServer.GetScreenFormation( formation );
		if( formation )
		{
			formation->SetSpotSpacing( min( formation->GetMaxSpotSpacing(), formation->GetSpotSpacing() - 0.25f ) );
		}
	}
	return true;
}


void UIGame :: RankPartyMembers()
{
	GopColl party;
	GopColl::iterator i;
	Go * pParty = gServer.GetScreenParty();
	if( pParty )
	{
		party = pParty->GetChildren();
	}	

	int highestRank = 0;
	int rankMax = 63;
	for( i = party.begin(); i != party.end(); ++i )
	{		
		if( *i )
		{
			int rank = gUIPartyManager.GetMemberPortraitRank( *i );
			//gpgenericf(( "party member = %s, rank = %d\n", (*i)->GetTemplateName(), rank ));
			if ( rank == -1 )
			{
				(*i)->GetMind()->SetRank( rankMax );		
			}
			else
			{
				(*i)->GetMind()->SetRank( rank );	
			}
			if ( rank > highestRank )
			{
				highestRank = rank;
			}
		}		
	}

	for( i = party.begin(); i != party.end(); ++i )
	{		
		if( *i )
		{
			if ( (int)(*i)->GetMind()->GetRank() == rankMax )
			{
				(*i)->GetMind()->SetRank( ++highestRank );
			}
		}
	}
}


bool UIGame :: GetShowFormation()
{
	return ( ::GetGlobalSeconds() >= m_TimeToDisplayFormation );
}


void UIGame :: SetShowFormation( float delay )
{
	double currentTime = ::GetGlobalSeconds();
	m_TimeToDisplayFormation = currentTime + delay;

	if( m_TimeToDisplayFormation > currentTime )
	{
		////////////////////////////////////////
		//	turn off formation

		if( !m_bFormationMode )
		{
			Formation * formation;
			gServer.GetScreenFormation( formation );
			formation->SetDrawAssignedSpots( false );
		}
	}
	else if( m_TimeToDisplayFormation <= currentTime )
	{
		////////////////////////////////////////
		//	turn on formation

		Formation * formation;

		if( gServer.GetScreenFormation( formation ) )
		{
			formation->SetDrawAssignedSpots( true );
		}
	}
}


/**************************************************************************

Function		: Update()

Purpose			: Updates all UI systems

Returns			: void

**************************************************************************/
void UIGame :: Update( double secondsElapsed )
{
	// Failsafe - If we're disconnected, why are we updating?
	if ( gWorldState.GetCurrentState() == WS_MP_SESSION_LOST )
	{
		return;
	}

	// process item label grab filters
	static bool ctrlKeyWasUp = true;
	if ( ctrlKeyWasUp && gAppModule.GetControlKey() )
	{
		ctrlKeyWasUp = false;
	}
	else if ( !ctrlKeyWasUp && !gAppModule.GetControlKey() )
	{
		m_pUIItemOverlay->PickupSelectedItems();

		ctrlKeyWasUp = true;
	}
	m_pUIItemOverlay->SetHitItemOverlay( false );

	// toggle action Q waypoint drawing
	GoMind::SetOptions( GoMind::OPTION_DRAW_HUMAN_WAYPOINTS, gAppModule.GetControlKey() && GP_DEBUG );

	/////////////////////////////////////////////////////////
	//	set to draw formation related stuff

	Go * party = gServer.GetScreenParty();

	if( party && ( ::GetGlobalSeconds() >= m_TimeToDisplayFormation ) )
	{
		Formation * pFormation;

		if( gServer.GetScreenFormation( pFormation ) )
		{
			SetShowFormation();
			pFormation->SetDrawAssignedSpots( true );
		}
	}

#if !GP_RETAIL

	if ( ReportSys::AreWarningsEnabled() )
	{
		// get warning count
		int warningCount = ReportSys::GetWarningCount();

		// update gui
		static int s_WarningCount;
		if( s_WarningCount != warningCount ) {
			s_WarningCount = warningCount;
			SetGUIWarningCount( s_WarningCount );
		}
	}

	if ( ReportSys::AreErrorsEnabled() )
	{
		// get error count - include asserts, fatals, and serious errors
		int errorCount = ReportSys::GetErrorCount();

		// update gui
		static int s_ErrorCount;
		if( s_ErrorCount != errorCount ) {
			s_ErrorCount = errorCount;
			SetGUIErrorCount( s_ErrorCount );
		}
	}

	CheckLodPolyCount();

#endif

	gUIPartyManager.Update		( (float)secondsElapsed );
	gUIInventoryManager.Update	( (float)secondsElapsed );	
	gUIMenuManager.UpdateSave	();
	gUIDialogueHandler.Update	( (float)secondsElapsed );
	gUIStoreManager.Update		();

	CameraInputRecalibrate();

	gUITeamManager.Update( (float)secondsElapsed );

	// Camera update
	if( m_bCameraScreenEdgeRotate && !m_bCameraKeyRotating && !gUIMenuManager.GetModalActive() )
	{
		float horizEdgeDiff = 1.0f - m_camHorizontalEdgeBuffer;
		float horizSpeed	= 1.0f;
		if( !IsZero( horizEdgeDiff ) )
		{
			horizSpeed		= (FABSF( gAppModule.GetNormalizedCursorX() ) - m_camHorizontalEdgeBuffer) / horizEdgeDiff;
		}

		float vertEdgeDiff	= 1.0f - m_camVerticalEdgeBuffer;
		float vertSpeed		= 1.0f;
		if( !IsZero( vertEdgeDiff ) )
		{
			vertSpeed		= (FABSF( gAppModule.GetNormalizedCursorY() ) - m_camVerticalEdgeBuffer) / vertEdgeDiff;
		}

		if( (gAppModule.GetNormalizedCursorX() <= -m_camHorizontalEdgeBuffer) && m_bScreenHorizTrack )
		{
			if ( m_bInvertCameraXAxis )
			{
				gUICamera.SetRotateLeft( true, horizSpeed );				
			}
			else
			{
				gUICamera.SetRotateRight( true, horizSpeed );											
			}
		}
		else if( (gAppModule.GetNormalizedCursorX() >= m_camHorizontalEdgeBuffer) && m_bScreenHorizTrack )
		{
			if ( m_bInvertCameraXAxis )
			{
				gUICamera.SetRotateRight( true, horizSpeed );						
			}
			else
			{
				gUICamera.SetRotateLeft( true, horizSpeed );				
			}
		}
		else
		{
			if( gUICamera.IsRotatingRight() )
			{
				gUICamera.SetRotateRight( false, 0.0f );
			}
			if( gUICamera.IsRotatingLeft() )
			{
				gUICamera.SetRotateLeft( false, 0.0f );
			}
		}

		if( (gAppModule.GetNormalizedCursorY() <= -m_camVerticalEdgeBuffer) && m_bScreenVertTrack )
		{
			if ( m_bInvertCameraYAxis )			
			{
				gUICamera.SetRotateDown( true, vertSpeed );
			}
			else
			{
				gUICamera.SetRotateUp( true, vertSpeed );
			}
		}
		else if( (gAppModule.GetNormalizedCursorY() >= m_camVerticalEdgeBuffer) && m_bScreenVertTrack )
		{
			if ( m_bInvertCameraYAxis )			
			{
				gUICamera.SetRotateUp( true, vertSpeed );
			}
			else
			{
				gUICamera.SetRotateDown( true, vertSpeed );
			}
		}
		else
		{
			if( gUICamera.IsRotatingUp() )
			{
				gUICamera.SetRotateUp( false, 0.0f );
			}
			if( gUICamera.IsRotatingDown() )
			{
				gUICamera.SetRotateDown( false, 0.0f );
			}
		}
	}

	{
		//GPPROFILERSAMPLE( "UIGame :: Update GameGUI Shell " );
		gUIShell.Update( secondsElapsed, gAppModule.GetCursorX(), gAppModule.GetCursorY() );
	}

	// This identifies rollover objects to highlight them	
	if ( gWorldState.GetCurrentState() != WS_SP_NIS )
	{
		if ( !gUIShell.GetRolloverConsumed() )
		{
			GetUICommands()->ContextCursorAction();
		}
		else 
		{
			GoHandle hTest( gUIInventoryManager.GetGUIRolloverGOID() );
			if ( hTest.IsValid() && hTest->HasActor() )
			{
				GetUICommands()->ContextCursorAction( false );
			}
			else
			{		
				GetUICommands()->SetCursor( CURSOR_GUI_STANDARD );
			}		
		}
	}

	// Update the information box
	GetUIPartyManager()->UpdateInfoBox();

	// show time remaining at 60 and 10 seconds
	if( ( (int)gVictory.GetGameTimeRemaining() == 60 ) || ( (int)gVictory.GetGameTimeRemaining() == 10 ) )
	{
		m_bGameTimerVisible = true;
		gUIShell.ShowInterface( "multiplayer_timer" );
	}

	if( m_bGameTimerVisible || m_pUIPlayerRanks->IsVisible() )
	{
		if( gVictory.GetTimeLimit() > 0 )
		{
			UIText * pTextRemaining = (UIText *)gUIShell.FindUIWindow( "text_time_remaining_elapsed", "multiplayer_timer" );
			if ( pTextRemaining )
			{
				pTextRemaining->SetText( gpwtranslate( $MSG$ "Time Remaining:" ) );
			}

			gpwstring sTime;

			int hours = ((int)gVictory.GetGameTimeRemaining() / 60) / 60;
			int mins = ((int)gVictory.GetGameTimeRemaining() / 60) - (hours * 60);
			int secs = (int)gVictory.GetGameTimeRemaining() - (hours * 60 * 60) - (mins * 60);

			if ( hours )
			{
				sTime.assignf( gpwtranslate( $MSG$ "%d:%.2d:%.2d" ), hours, mins, secs );
			}
			else
			{
				sTime.assignf( gpwtranslate( $MSG$ "%d:%.2d" ), mins, secs );
			}

			UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_game_timer", "multiplayer_timer" );
			if( pText )
			{
				pText->SetText( sTime );
			}
			pText = (UIText *)gUIShell.FindUIWindow( "game_timer_text", "multiplayer_ranks" );
			if ( pText )
			{
				gpwstring leaderTime = pTextRemaining->GetText();
				leaderTime.appendf( L" %s", sTime.c_str() );
				pText->SetText( leaderTime );
			}
		}
		else
		{
			UIText * pTextElapsed = (UIText *)gUIShell.FindUIWindow( "text_time_remaining_elapsed", "multiplayer_timer" );
			if ( pTextElapsed )
			{
				pTextElapsed->SetText( gpwtranslate( $MSG$ "Time Elapsed:" ) );
			}

			gpwstring sTime;

			int hours = ((int)gVictory.GetGameTimeElapsed() / 60) / 60;
			int mins = ((int)gVictory.GetGameTimeElapsed() / 60) - (hours * 60);
			int secs = (int)gVictory.GetGameTimeElapsed() - (hours * 60 * 60) - (mins * 60);

			if ( hours )
			{
				sTime.assignf( L"%d:%.2d", hours, mins );
			}
			else
			{
				sTime.assignf( L"%d:%.2d", mins, secs );
			}

			UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_game_timer", "multiplayer_timer" );
			if( pText )
			{
				pText->SetText( sTime );
			}
			pText = (UIText *)gUIShell.FindUIWindow( "game_timer_text", "multiplayer_ranks" );
			if ( pText )
			{
				gpwstring leaderTime = pTextElapsed->GetText();
				leaderTime.appendf( L" %s", sTime.c_str() );
				pText->SetText( leaderTime );
			}
		}
	}

	if( m_pUIPlayerRanks->IsVisible() )
	{
		m_pUIPlayerRanks->Update( secondsElapsed );
	}
	
	if( ::IsMultiPlayer() )
	{
		if( m_pUIEmoteList->IsVisible() )
		{
			m_pUIEmoteList->Update( secondsElapsed );
		}
	}

	if ( gWorldState.GetCurrentState() == WS_SP_DEFEAT )
	{
		ShowDefeatScreen( secondsElapsed );
	}

	if ( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		gUIMultiplayer.UpdateJIPInterface();
	}

	if ( ::IsMultiPlayer() && gUIMultiplayer.IsPlayersPanelVisible() )
	{
		gUIMultiplayer.UpdatePlayersPanel();
	}

	if( m_bLastPause != gAppModule.IsUserPaused() )
	{		
		gUIMenuManager.GuiPause( gAppModule.IsUserPaused() );

		// $$$ Remove when sure it is no longer needed
		/*
		UIWindow * pPaused = gUIShell.FindUIWindow( "paused_window", "pause" );
		pPaused->ReceiveMessage( UIMessage( MSG_CREATED ) );
		*/

		m_bLastPause = gAppModule.IsUserPaused();
	}

	if ( ::IsMultiPlayer() && (gUIMultiplayer.GetMultiplayerMode() == MM_GUN) )
	{
		gUIZoneMatch.UpdateInGame( secondsElapsed );
	}

	if ( m_DragTimer > 0.0f )
	{
		m_DragTimer -= (float)secondsElapsed;
		if ( m_DragTimer <= 0.0f )
		{
			m_DragTimer = 0.0f;
			if ( m_DragX == gAppModule.GetCursorX() &&
				 m_DragY == gAppModule.GetCursorY() &&
				 gAppModule.GetLButton() )
			{
				SetSelectionBox( true );			
			}
		}
	}

	m_BackendDialogJustShown = false;
}


// Display load progress as we start the game
void UIGame :: UpdateLoadProgress( float progress, bool clearScreen, const gpwstring & sInfo )	
{
	gpstring sInterface = ::IsSinglePlayer() ? "load_bar" : "multiplayer_load";

	if( (( IsMultiPlayer() && gNetPipe.HasOpenSession() ) || IsSinglePlayer() ) && gUIShell.FindInterface( sInterface ) )
	{
		UIStatusBar *pStatus = (UIStatusBar *) gUIShell.FindUIWindow( "loading_bar", sInterface );

		if( pStatus && Server::DoesSingletonExist() && gServer.GetLocalHumanPlayer() )
		{
			// special: if progress is <0 then don't update local (i.e. somebody
			// wants us to repaint only so we can update multiplayer progress)
			if ( progress >= 0 )
			{
				pStatus->SetStatus( progress );

#				if !GP_RETAIL
				UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "text_box_load_info", "debug_help" );
				if ( pTextBox )
				{
					pTextBox->SetText( sInfo );
				}
#				endif
			}
			else
			{
				progress = pStatus->GetStatus();
			}

			float highestProgress = 0.0f;

			UIText * pLocalText = NULL;
			if ( Server::DoesSingletonExist() )
			{
				Player * player = gServer.GetLocalHumanPlayer();
				if( player )
				{
					if ( ::IsMultiPlayer() )
					{
						pLocalText = (UIText *)gUIShell.FindUIWindow( "text_player_1" );
						if ( pLocalText )
						{
							pLocalText->SetText( player->GetHeroName() );
						}
					}

					player->SetLoadProgress( progress );
					highestProgress = progress;
				}
			}

			// Handle other players, if necessary
			if ( ::IsMultiPlayer() )
			{
				for ( int counter = 2; counter <= MAX_PARTY_MEMBERS; ++counter )
				{
					gpstring sTemp;
					sTemp.assignf( "load_bar_%d", counter );					
					gUIShell.ShowGroup( sTemp, false, false, sInterface );
				}				

				// we want the loading screen for all players that are currently jiping, not just
				// ourselves.
				//if ( !IsJipLoad() )
				//{
					int index = 2;
					Server::PlayerColl players = gServer.GetPlayers();
					Server::PlayerColl::iterator iPlayer;
					Player * pFastestPlayer = gServer.GetLocalHumanPlayer();
					for ( iPlayer = players.begin(); iPlayer != players.end(); ++iPlayer )
					{
						if ( IsValid( *iPlayer ) )
						{
							if ( (*iPlayer)->GetId() != gServer.GetLocalHumanPlayer()->GetId() && !(*iPlayer)->IsComputerPlayer() 
								&& ( ! IsInGame( (*iPlayer)->GetWorldState() ) ) )
							{
								if ( (*iPlayer)->GetLoadProgress() > highestProgress )
								{
									highestProgress = (*iPlayer)->GetLoadProgress();
									pFastestPlayer = *iPlayer;
								}
							}
						}
					}

					if ( pLocalText )
					{
						if ( pFastestPlayer == gServer.GetLocalHumanPlayer() )
						{
							pLocalText->SetColor( 0xff00ff00 );
						}
						else
						{
							pLocalText->SetColor( 0xffffffff );
						}
					}

					for ( iPlayer = players.begin(); iPlayer != players.end(); ++iPlayer )
					{
						if ( IsValid( *iPlayer ) )
						{
							if ( (*iPlayer)->GetId() != gServer.GetLocalHumanPlayer()->GetId() && !(*iPlayer)->IsComputerPlayer() 
								&& ( ! IsInGame( (*iPlayer)->GetWorldState() ) ) )
							{
								gpstring sTemp;

								sTemp.assignf( "text_player_%d", index );
								UIText * pText = (UIText *)gUIShell.FindUIWindow( sTemp );
								if ( pText )
								{
									pText->SetText( (*iPlayer)->GetHeroName() );

									if ( pFastestPlayer == *iPlayer )
									{
										pText->SetColor( 0xff00ff00 );
										pLocalText->SetColor( 0xffffffff );
									}
									else
									{
										pText->SetColor( 0xffffffff );
									}
								}	

								sTemp.assignf( "load_bar_%d", index );
								pStatus = (UIStatusBar *)gUIShell.FindUIWindow( sTemp );
								if ( pStatus )
								{
									pStatus->SetStatus( (*iPlayer)->GetLoadProgress() );
								}							

								gUIShell.ShowGroup( sTemp, true, false, sInterface );							
								
								++index;
							}
						}
					}
				//}
			}

			bool bFog = gDefaultRapi.SetFogActivatedState( false );
			bool bFlip = gDefaultRapi.IsFlipAllowed();
			gDefaultRapi.SetFlipAllowed( false );
			if( gUIShell.GetRenderer().Begin3D( clearScreen ) )
			{
				gUIShell.DrawInterface( sInterface );

#	if !GP_RETAIL
				gUIShell.ShowInterface( "debug_help" );
				gUIShell.DrawInterface( "debug_help" );
				gUIShell.HideInterface( "debug_help" );
#	endif

				gUIShell.GetRenderer().End3D( true, true );
			}
			gDefaultRapi.SetFlipAllowed( bFlip );
			gDefaultRapi.SetFogActivatedState( bFog );

		}
	}
}

void UIGame :: UpdateSaveGameProgress( float progress, bool clearScreen, const gpwstring & sInfo )
{
	if ( gUIShell.FindInterface( "quick_save" ) )
	{
		UIStatusBar *pStatus = (UIStatusBar *) gUIShell.FindUIWindow( "saveload_bar", "quick_save" );

		if ( pStatus )
		{
			bool bFog = gDefaultRapi.SetFogActivatedState( false );
			bool bFlip = gDefaultRapi.IsFlipAllowed();
			gDefaultRapi.SetFlipAllowed( false );
			if( gUIShell.GetRenderer().Begin3D( clearScreen ) )
			{
				pStatus->SetStatus( progress );

#	if !GP_RETAIL
			
				UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "text_box_save_info", "debug_help" );
				if ( pTextBox )
				{
					pTextBox->SetText( sInfo );
				}
#	endif

				UIWindowVec::iterator i;
				UIWindowVec loading_windows = gUIShell.ListWindowsOfGroup( "saveload_bar" );

				for( i = loading_windows.begin(); i != loading_windows.end(); ++i )
				{
					(*i)->Draw();
				}

				pStatus->Draw();

				gUI.GetGameGUI().GetRenderer().End3D( true, true );
			}
			gDefaultRapi.SetFlipAllowed( bFlip );
			gDefaultRapi.SetFogActivatedState( bFog );
		}
	}

}


// Update the ZDelta fogging
void UIGame :: UpdateZDeltaFog( const float delta )
{
	UNREFERENCED_PARAMETER( delta );
	// $$$$ Re-implement this if we really want it, otherwise forget it
}



/**************************************************************************

Function		: PublishInterfaceToInputBinder()

Purpose			: Publishes information to the input binder

Returns			: void

**************************************************************************/
void UIGame :: PublishInterfaceToInputBinder()
{
	//////////////////////////////////////////////////////////////////////////////////
	// NIS input bindings
	GetNisInputBinder().PublishVoidInterface( "exit_nis",							makeFunctor( *this,					&UIGame :: ExitNis						) );

	//////////////////////////////////////////////////////////////////////////////////
	// Non-configurable in game bindings
	GetGameInputBinder().PublishIntInterface ( "cursor_dx",							makeFunctor( *this,                &UIGame :: OnDeltaXCursor                ) );
	GetGameInputBinder().PublishIntInterface ( "cursor_dy",							makeFunctor( *this,                &UIGame :: OnDeltaYCursor                ) );
	GetGameInputBinder().PublishIntInterface ( "cursor_dz",							makeFunctor( *this,                &UIGame :: OnDeltaZCursor                ) );

	GetGameInputBinder().PublishVoidInterface( "selection_down",					makeFunctor( *this,                &UIGame :: OnSelectionDown               ) );
	GetGameInputBinder().PublishVoidInterface( "selection_up",						makeFunctor( *this,                &UIGame :: OnSelectionUp                 ) );

	GetGameInputBinder().PublishVoidInterface( "special_down",						makeFunctor( *this,                &UIGame :: OnSpecialDown                 ) );
	GetGameInputBinder().PublishVoidInterface( "special_up",						makeFunctor( *this,                &UIGame :: OnSpecialUp                   ) );

	GetGameInputBinder().PublishVoidInterface( "context_down",						makeFunctor( *this,                &UIGame :: OnContextDown                 ) );
	GetGameInputBinder().PublishVoidInterface( "context_up",						makeFunctor( *this,                &UIGame :: OnContextUp                   ) );

	GetGameInputBinder().PublishVoidInterface( "ingame_pressed_escape",				makeFunctor( *this,			       &UIGame :: InGamePressedEscape			) );
	

	//////////////////////////////////////////////////////////////////////////////////
	// User-configurable in game bindings

	GetUserInputBinder().PublishVoidInterface( "select_all_party_members",			makeFunctor( *this,					&UIGame :: SelectAllPartyMembers         ) );
	GetUserInputBinder().PublishVoidInterface( "select_next_player",				makeFunctor( *this,					&UIGame :: SelectNextPlayer              ) );
	GetUserInputBinder().PublishVoidInterface( "select_last_player",				makeFunctor( *this,					&UIGame :: SelectLastPlayer              ) );
	GetUserInputBinder().PublishVoidInterface( "select_lead_character",				makeFunctor( *this,					&UIGame :: SelectLeadPlayer				 ) );

	GetUserInputBinder().PublishVoidInterface( "game_pause",						makeFunctor( *this,					&UIGame :: RSGamePauseToggle             ) );
	GetUserInputBinder().PublishVoidInterface( "game_speed_up",						makeFunctor( *this,					&UIGame :: GameSpeedUp                   ) );
	GetUserInputBinder().PublishVoidInterface( "game_speed_down",					makeFunctor( *this,					&UIGame :: GameSpeedDown                 ) );
	GetUserInputBinder().PublishVoidInterface( "game_speed_reset",					makeFunctor( *this,					&UIGame :: GameSpeedReset                ) );
	GetUserInputBinder().PublishVoidInterface( "game_speed_min",					makeFunctor( *this,					&UIGame :: GameSpeedMin                  ) );
	GetUserInputBinder().PublishVoidInterface( "game_speed_max",					makeFunctor( *this,					&UIGame :: GameSpeedMax                  ) );
	GetUserInputBinder().PublishVoidInterface( "game_single_step",					makeFunctor( *this,					&UIGame :: GameSingleStep                ) );

	if( !gTattooGame.TestOptions( TattooGame::GOPTION_MARKETING_DEMO ) )
	{
		GetUserInputBinder().PublishVoidInterface( "camera_none_mode",					makeFunctor( *this,                &UIGame :: SetCameraNoneModeOn           ) );
		GetUserInputBinder().PublishVoidInterface( "camera_dolly_mode",					makeFunctor( *this,                &UIGame :: SetCameraDollyModeOn          ) );
		GetUserInputBinder().PublishVoidInterface( "camera_zoom_mode",					makeFunctor( *this,                &UIGame :: SetCameraZoomModeOn           ) );
		GetUserInputBinder().PublishVoidInterface( "camera_orbit_mode",					makeFunctor( *this,                &UIGame :: SetCameraOrbitModeOn          ) );
		GetUserInputBinder().PublishVoidInterface( "camera_crane_mode",					makeFunctor( *this,                &UIGame :: SetCameraCraneModeOn          ) );
		GetUserInputBinder().PublishVoidInterface( "camera_track_toggle",				makeFunctor( *this,                &UIGame :: ToggleCameraTrackAndHold      ) );
	}

	GetUserInputBinder().PublishInputInterface( "camera_zoom_in",					makeFunctor( *this,                &UIGame :: CameraZoomIn                  ) );
	GetUserInputBinder().PublishInputInterface( "camera_zoom_out",					makeFunctor( *this,                &UIGame :: CameraZoomOut                 ) );
	GetUserInputBinder().PublishKeyInterface( "camera_rotate_left",					makeFunctor( *this,                &UIGame :: CameraRotateLeft              ) );
	GetUserInputBinder().PublishKeyInterface( "camera_rotate_right",				makeFunctor( *this,                &UIGame :: CameraRotateRight             ) );
	GetUserInputBinder().PublishKeyInterface( "camera_rotate_up",					makeFunctor( *this,                &UIGame :: CameraRotateUp	            ) );
	GetUserInputBinder().PublishKeyInterface( "camera_rotate_down",					makeFunctor( *this,                &UIGame :: CameraRotateDown				) );
	GetUserInputBinder().PublishKeyInterface( "camera_free_look",					makeFunctor( *this,                &UIGame :: CameraFreeLook                ) );

	GetUserInputBinder().PublishVoidInterface( "selection_delete",					makeFunctor( *this,                &UIGame :: Selection_Delete              ) );
	GetUserInputBinder().PublishVoidInterface( "selection_copy",					makeFunctor( *this,                &UIGame :: Selection_Copy                ) );
	GetUserInputBinder().PublishVoidInterface( "selection_paste",					makeFunctor( *this,                &UIGame :: Selection_Paste               ) );

	GetUserInputBinder().PublishVoidInterface( "attack",							makeFunctor( *GetUICommands(),     UICommands::InputCommandAttack         ) );
	GetUserInputBinder().PublishVoidInterface( "cast",								makeFunctor( *GetUICommands(),     UICommands::InputCommandCast           ) );
	GetUserInputBinder().PublishVoidInterface( "move",								makeFunctor( *GetUICommands(),     UICommands::InputCommandMove           ) );
	GetUserInputBinder().PublishVoidInterface( "guard",								makeFunctor( *GetUICommands(),     UICommands::InputCommandGuard          ) );	
	GetUserInputBinder().PublishVoidInterface( "stop",								makeFunctor( *GetUICommands(),     UICommands::SelectionCommandStop       ) );

	GetUserInputBinder().PublishVoidInterface( "party_heal_body_with_potions",		makeFunctor( *this,                &UIGame :: PartyHealBodyWithPotions      ) );
	GetUserInputBinder().PublishVoidInterface( "party_heal_magic_with_potions",		makeFunctor( *this,                &UIGame :: PartyHealMagicWithPotions     ) );	

	GetUserInputBinder().PublishVoidInterface( "rotate_selected_slots",				makeFunctor( *m_pUIInventoryManager, &UIInventoryManager::RotateSelectedSlots  ) );
	GetUserInputBinder().PublishVoidInterface( "rotate_selected_slots_prev",		makeFunctor( *m_pUIInventoryManager, &UIInventoryManager::RotateSelectedSlotsPrev ) );

	GetUserInputBinder().PublishVoidInterface( "rotate_primary_spell_slot",			makeFunctor( *m_pUIInventoryManager, &UIInventoryManager::RotatePrimarySpells  ) );
	GetUserInputBinder().PublishVoidInterface( "rotate_secondary_spell_slot",		makeFunctor( *m_pUIInventoryManager, &UIInventoryManager::RotateSecondarySpells  ) );

	GetUserInputBinder().PublishVoidInterface( "inventory",							makeFunctor( *GetUIPartyManager(), UIPartyManager::ActivateInventory      ) );
	GetUserInputBinder().PublishVoidInterface( "character_stats",					makeFunctor( *GetUIPartyManager(), UIPartyManager::ActivateCharacterStats ) );
	GetUserInputBinder().PublishVoidInterface( "magic",								makeFunctor( *GetUIPartyManager(), UIPartyManager::ActivateSpellBook      ) );
	GetUserInputBinder().PublishVoidInterface( "expert_gui_mode",					makeFunctor( *GetUIPartyManager(), UIPartyManager::ActivateExpertMode	  ) );

	GetUserInputBinder().PublishVoidInterface( "selection_speed_up",				makeFunctor( *this,                &UIGame :: Selection_SpeedUp)            ) ;
	GetUserInputBinder().PublishVoidInterface( "selection_speed_down",				makeFunctor( *this,                &UIGame :: Selection_SpeedDown)          ) ;
	GetUserInputBinder().PublishVoidInterface( "selection_speed_reset",				makeFunctor( *this,                &UIGame :: Selection_SpeedReset)         ) ;

	GetUserInputBinder().PublishVoidInterface( "disband_selected",					makeFunctor( *GetUIPartyManager(), UIPartyManager :: DisbandButton)         ) ;	

#	if !GP_RETAIL

	if( !gTattooGame.TestOptions( TattooGame::GOPTION_MARKETING_DEMO ) )
	{
		GetUserInputBinder().PublishVoidInterface( "teleport",							makeFunctor( *this,                &UIGame :: TeleportGameObjectToMouse     ) );

		GetInputBinder().PublishVoidInterface( "debug_mind_probe",						makeFunctor( *this,                &UIGame :: DebugMindProbe				) );
		GetInputBinder().PublishVoidInterface( "debug_mind2_probe",						makeFunctor( *this,                &UIGame :: DebugMind2Probe				) );
		GetInputBinder().PublishVoidInterface( "debug_combat_probe",					makeFunctor( *this,                &UIGame :: DebugCombatProbe				) );
		GetInputBinder().PublishVoidInterface( "debug_body_probe",						makeFunctor( *this,                &UIGame :: DebugBodyProbe				) );
		GetInputBinder().PublishVoidInterface( "debug_aspect_probe",					makeFunctor( *this,                &UIGame :: DebugAspectProbe				) );
		GetInputBinder().PublishVoidInterface( "debug_verbose_toggle",					makeFunctor( *this,                &UIGame :: DebugProbeVerboseToggle		) );
		GetUserInputBinder().PublishVoidInterface( "debug_object_update_break",			makeFunctor( *this,                &UIGame :: DebugObjectUpdateBreak        ) );

		GetUserInputBinder().PublishVoidInterface( "debug_object_hud",					makeFunctor( *this,                &UIGame :: DebugHudObject                ) );
		GetUserInputBinder().PublishVoidInterface( "debug_actors_hud",					makeFunctor( *this,                &UIGame :: DebugHudActors                ) );
		GetUserInputBinder().PublishVoidInterface( "debug_items_hud",					makeFunctor( *this,                &UIGame :: DebugHudItems                 ) );
		GetUserInputBinder().PublishVoidInterface( "debug_commands_hud",				makeFunctor( *this,                &UIGame :: DebugHudCommands              ) );
		GetUserInputBinder().PublishVoidInterface( "debug_mics_hud",					makeFunctor( *this,                &UIGame :: DebugHudMisc                  ) );
		GetUserInputBinder().PublishVoidInterface( "debug_all_hud",						makeFunctor( *this,                &UIGame :: DebugHudAll                   ) );
		GetUserInputBinder().PublishVoidInterface( "debug_hud_labels",					makeFunctor( *this,                &UIGame :: DebugHudLabels                ) );
		GetUserInputBinder().PublishVoidInterface( "debug_hud_errors",					makeFunctor( *this,                &UIGame :: DebugHudErrors                ) );
		GetUserInputBinder().PublishVoidInterface( "debug_hud_depth",					makeFunctor( *this,                &UIGame :: DebugHudDepth	                ) );
		GetUserInputBinder().PublishVoidInterface( "debug_toggle_selection_brain",		makeFunctor( *this,                &UIGame :: DebugToggleSelectionBrain     ) );

		GetUserInputBinder().PublishVoidInterface( "debug_increment_animation",			makeFunctor( *this,                &UIGame :: DebugIncrementAnimation		) );
	}

	GetUserInputBinder().PublishVoidInterface( "patrol",								makeFunctor( *GetUICommands(),     UICommands::InputCommandPatrol         ) );

#	endif // !GP_RETAIL

	GetUserInputBinder().PublishVoidInterface( "move_order_free",					makeFunctor( *this,                &UIGame :: MoveOrderFree						) );
	GetUserInputBinder().PublishVoidInterface( "move_order_limited",				makeFunctor( *this,                &UIGame :: MoveOrderLimited					) );
	GetUserInputBinder().PublishVoidInterface( "move_order_never",					makeFunctor( *this,                &UIGame :: MoveOrderNever					) );

	GetUserInputBinder().PublishVoidInterface( "fight_order_always",				makeFunctor( *this,                &UIGame :: FightOrderAlways					) );
	GetUserInputBinder().PublishVoidInterface( "fight_order_back_only",				makeFunctor( *this,                &UIGame :: FightOrderBackOnly				) );
	GetUserInputBinder().PublishVoidInterface( "fight_order_never",					makeFunctor( *this,                &UIGame :: FightOrderNever					) );
	
	GetUserInputBinder().PublishVoidInterface( "target_closest",					makeFunctor( *this,                &UIGame :: TargetOrderClosest				) );
	GetUserInputBinder().PublishVoidInterface( "target_weakest",					makeFunctor( *this,                &UIGame :: TargetOrderWeakest				) );
	GetUserInputBinder().PublishVoidInterface( "target_strongest",					makeFunctor( *this,                &UIGame :: TargetOrderStrongest				) );

	GetUserInputBinder().PublishVoidInterface( "cycle_formations",					makeFunctor( *GetUIPartyManager(),		UIPartyManager :: CycleFormations		) );		
	GetUserInputBinder().PublishVoidInterface( "formation_increase_spacing",		makeFunctor( *this,                &UIGame :: FormationIncreaseSpacing			) );
	GetUserInputBinder().PublishVoidInterface( "formation_decrease_spacing",		makeFunctor( *this,                &UIGame :: FormationDecreaseSpacing			) );

	GetUserInputBinder().PublishVoidInterface( "set_group_1",						makeFunctor( *GetUIPartyManager(), UIPartyManager::SetGroup1					) );
	GetUserInputBinder().PublishVoidInterface( "set_group_2",						makeFunctor( *GetUIPartyManager(), UIPartyManager::SetGroup2					) );
	GetUserInputBinder().PublishVoidInterface( "set_group_3",						makeFunctor( *GetUIPartyManager(), UIPartyManager::SetGroup3					) );
	GetUserInputBinder().PublishVoidInterface( "set_group_4",						makeFunctor( *GetUIPartyManager(), UIPartyManager::SetGroup4					) );
	GetUserInputBinder().PublishVoidInterface( "set_group_5",						makeFunctor( *GetUIPartyManager(), UIPartyManager::SetGroup5					) );
	GetUserInputBinder().PublishVoidInterface( "set_group_6",						makeFunctor( *GetUIPartyManager(), UIPartyManager::SetGroup6					) );
	GetUserInputBinder().PublishVoidInterface( "set_group_7",						makeFunctor( *GetUIPartyManager(), UIPartyManager::SetGroup7					) );
	GetUserInputBinder().PublishVoidInterface( "set_group_8",						makeFunctor( *GetUIPartyManager(), UIPartyManager::SetGroup8					) );
	
	GetUserInputBinder().PublishVoidInterface( "get_group_1",						makeFunctor( *GetUIPartyManager(), UIPartyManager::GetGroup1					) );
	GetUserInputBinder().PublishVoidInterface( "get_group_2",						makeFunctor( *GetUIPartyManager(), UIPartyManager::GetGroup2					) );
	GetUserInputBinder().PublishVoidInterface( "get_group_3",						makeFunctor( *GetUIPartyManager(), UIPartyManager::GetGroup3					) );
	GetUserInputBinder().PublishVoidInterface( "get_group_4",						makeFunctor( *GetUIPartyManager(), UIPartyManager::GetGroup4					) );
	GetUserInputBinder().PublishVoidInterface( "get_group_5",						makeFunctor( *GetUIPartyManager(), UIPartyManager::GetGroup5					) );
	GetUserInputBinder().PublishVoidInterface( "get_group_6",						makeFunctor( *GetUIPartyManager(), UIPartyManager::GetGroup6					) );
	GetUserInputBinder().PublishVoidInterface( "get_group_7",						makeFunctor( *GetUIPartyManager(), UIPartyManager::GetGroup7					) );
	GetUserInputBinder().PublishVoidInterface( "get_group_8",						makeFunctor( *GetUIPartyManager(), UIPartyManager::GetGroup8					) );

	GetUserInputBinder().PublishVoidInterface( "sort_inventory",					makeFunctor( *GetUIPartyManager(), UIPartyManager::SortInventory				) );

	GetUserInputBinder().PublishVoidInterface( "field_commands",					makeFunctor( *GetUIPartyManager(), UIPartyManager::ActivateFieldCommands		) );
	GetUserInputBinder().PublishVoidInterface( "toggle_status_bars",				makeFunctor( *GetUIPartyManager(), UIPartyManager::ToggleStatusBars				) );	

#	if !GP_RETAIL
	GetUserInputBinder().PublishVoidInterface( "super_recovery",					makeFunctor( *this,                &UIGame :: SuperRecovery						) );
#	endif // !GP_RETAIL

	GetUserInputBinder().PublishVoidInterface( "toggle_gui_edit_box",				makeFunctor( *this,				   &UIGame :: ToggleGUIEditBox					) );	
	GetUserInputBinder().PublishVoidInterface( "toggle_gui_edit_box_team",			makeFunctor( *this,				   &UIGame :: ToggleGUIEditBoxTeam				) );	
	GetUserInputBinder().PublishVoidInterface( "toggle_gui_edit_box_everyone",		makeFunctor( *this,				   &UIGame :: ToggleGUIEditBoxEveryone			) );	

	GetUserInputBinder().PublishKeyInterface ( "toggle_player_ranks",				makeFunctor( *this,					&UIGame :: TogglePlayerRanks				) );
	GetUserInputBinder().PublishVoidInterface( "toggle_player_labels",				makeFunctor( *this,					&UIGame :: TogglePlayerLabels				) );	
	GetUserInputBinder().PublishVoidInterface( "toggle_game_timer",					makeFunctor( *this,					&UIGame :: ToggleGameTimer					) );
	GetUserInputBinder().PublishKeyInterface ( "toggle_emote_list",					makeFunctor( *GetUIEmoteList(),		&UIEmoteList :: ToggleEmoteList				) );
		
	GetUserInputBinder().PublishKeyInterface ( "emote_list_1",						makeFunctor( *GetUIEmoteList(),		&UIEmoteList :: EmoteListSelect1			) );
	GetUserInputBinder().PublishKeyInterface ( "emote_list_2",						makeFunctor( *GetUIEmoteList(),		&UIEmoteList :: EmoteListSelect2			) );
	GetUserInputBinder().PublishKeyInterface ( "emote_list_3",						makeFunctor( *GetUIEmoteList(),		&UIEmoteList :: EmoteListSelect3			) );
	GetUserInputBinder().PublishKeyInterface ( "emote_list_4",						makeFunctor( *GetUIEmoteList(),		&UIEmoteList :: EmoteListSelect4			) );
	GetUserInputBinder().PublishKeyInterface ( "emote_list_5",						makeFunctor( *GetUIEmoteList(),		&UIEmoteList :: EmoteListSelect5			) );
	GetUserInputBinder().PublishKeyInterface ( "emote_list_6",						makeFunctor( *GetUIEmoteList(),		&UIEmoteList :: EmoteListSelect6			) );
	GetUserInputBinder().PublishKeyInterface ( "emote_list_7",						makeFunctor( *GetUIEmoteList(),		&UIEmoteList :: EmoteListSelect7			) );
	GetUserInputBinder().PublishKeyInterface ( "emote_list_8",						makeFunctor( *GetUIEmoteList(),		&UIEmoteList :: EmoteListSelect8			) );
	GetUserInputBinder().PublishKeyInterface ( "emote_list_9",						makeFunctor( *GetUIEmoteList(),		&UIEmoteList :: EmoteListSelect9			) );
	GetUserInputBinder().PublishKeyInterface ( "emote_list_0",						makeFunctor( *GetUIEmoteList(),		&UIEmoteList :: EmoteListSelect0			) );
	
	GetUserInputBinder().PublishVoidInterface( "toggle_item_labels",				makeFunctor( *this,					&UIGame :: ToggleItemLabels					) );

	GetUserInputBinder().PublishVoidInterface( "toggle_mini_map",					makeFunctor( *this,					&UIGame :: ToggleMiniMap					) );

	GetUserInputBinder().PublishVoidInterface( "save_game",							makeFunctor( *m_pUIMenuManager,		&UIMenuManager::SaveGameDialog				) );
	GetUserInputBinder().PublishVoidInterface( "load_game",							makeFunctor( *m_pUIMenuManager,		&UIMenuManager::LoadGameDialog				) );
	GetUserInputBinder().PublishVoidInterface( "game_options",						makeFunctor( *this,					&UIGame :: ShowInGameOptions				) );

	GetUserInputBinder().PublishVoidInterface( "toggle_quest_log",					makeFunctor( *GetUIPartyManager(),  UIPartyManager::ToggleQuestLog				) );

	GetUserInputBinder().PublishVoidInterface( "quick_save",						makeFunctor( *m_pUIMenuManager,		&UIMenuManager :: QuickSaveDialog			) );
	GetUserInputBinder().PublishVoidInterface( "quick_load",						makeFunctor( *m_pUIMenuManager,		&UIMenuManager :: QuickLoadDialog			) );
	
	GetUserInputBinder().PublishVoidInterface( "set_awp_01",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: SetWeaponsConfig1	) );
	GetUserInputBinder().PublishVoidInterface( "set_awp_02",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: SetWeaponsConfig2	) );
	GetUserInputBinder().PublishVoidInterface( "set_awp_03",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: SetWeaponsConfig3	) );
	GetUserInputBinder().PublishVoidInterface( "set_awp_04",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: SetWeaponsConfig4	) );
	GetUserInputBinder().PublishVoidInterface( "set_awp_05",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: SetWeaponsConfig5	) );
	GetUserInputBinder().PublishVoidInterface( "set_awp_06",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: SetWeaponsConfig6	) );
	GetUserInputBinder().PublishVoidInterface( "set_awp_07",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: SetWeaponsConfig7	) );
	GetUserInputBinder().PublishVoidInterface( "set_awp_08",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: SetWeaponsConfig8	) );
	GetUserInputBinder().PublishVoidInterface( "set_awp_09",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: SetWeaponsConfig9	) );
	GetUserInputBinder().PublishVoidInterface( "set_awp_10",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: SetWeaponsConfig0	) );

	GetUserInputBinder().PublishVoidInterface( "get_awp_01",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: GetWeaponsConfig1	) );
	GetUserInputBinder().PublishVoidInterface( "get_awp_02",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: GetWeaponsConfig2	) );
	GetUserInputBinder().PublishVoidInterface( "get_awp_03",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: GetWeaponsConfig3	) );
	GetUserInputBinder().PublishVoidInterface( "get_awp_04",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: GetWeaponsConfig4	) );
	GetUserInputBinder().PublishVoidInterface( "get_awp_05",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: GetWeaponsConfig5	) );
	GetUserInputBinder().PublishVoidInterface( "get_awp_06",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: GetWeaponsConfig6	) );
	GetUserInputBinder().PublishVoidInterface( "get_awp_07",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: GetWeaponsConfig7	) );
	GetUserInputBinder().PublishVoidInterface( "get_awp_08",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: GetWeaponsConfig8	) );
	GetUserInputBinder().PublishVoidInterface( "get_awp_09",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: GetWeaponsConfig9	) );
	GetUserInputBinder().PublishVoidInterface( "get_awp_10",						makeFunctor( *m_pUIInventoryManager, &UIInventoryManager :: GetWeaponsConfig0	) );

	GetUserInputBinder().PublishVoidInterface( "collect_loot",						makeFunctor( *GetUIPartyManager(),		UIPartyManager :: CollectLoot			) );

	GetUserInputBinder().PublishVoidInterface( "chat_history_up",					makeFunctor( *m_pUIGameConsole,		UIGameConsole :: ChatHistoryUp			) );
	GetUserInputBinder().PublishVoidInterface( "chat_history_down",					makeFunctor( *m_pUIGameConsole,		UIGameConsole :: ChatHistoryDown		) );		
	GetUserInputBinder().PublishVoidInterface( "chat_history_clear",				makeFunctor( *m_pUIGameConsole,		UIGameConsole :: ChatHistoryClear		) );		
	GetUserInputBinder().PublishVoidInterface( "chat_history_lock",					makeFunctor( *m_pUIGameConsole,		UIGameConsole :: ChatHistoryLock		) );			

	GetUserInputBinder().PublishVoidInterface( "close_dialogs",						makeFunctor( *m_pUIMenuManager,  UIMenuManager :: KeyCloseAllDialogs		) );
	GetUserInputBinder().PublishVoidInterface( "tutorial_tips",						makeFunctor( *m_pUIMenuManager,  UIMenuManager :: ViewTutorialTips			) );
}



/**************************************************************************

Function		: BindInputToPublishedInterface()

Purpose			: Binds input to an interface

Returns			: void

**************************************************************************/
void UIGame :: BindInputToPublishedInterface()
{
	// Bind default game interfaces
	GetGameInputBinder().BindInputToPublishedInterface( MouseInput( "dx",          Keys::Q_IGNORE_QUALIFIERS ), "cursor_dx"           );
	GetGameInputBinder().BindInputToPublishedInterface( MouseInput( "dy",          Keys::Q_IGNORE_QUALIFIERS ), "cursor_dy"           );
	GetGameInputBinder().BindInputToPublishedInterface( MouseInput( "dz",          Keys::Q_IGNORE_QUALIFIERS ), "cursor_dz"           );
	GetGameInputBinder().BindInputToPublishedInterface( MouseInput( "left_down",   Keys::Q_IGNORE_QUALIFIERS ), "selection_down"      );
	GetGameInputBinder().BindInputToPublishedInterface( MouseInput( "left_up",     Keys::Q_IGNORE_QUALIFIERS ), "selection_up"        );
	GetGameInputBinder().BindInputToPublishedInterface( MouseInput( "middle_down", Keys::Q_IGNORE_QUALIFIERS ), "special_down"        );
	GetGameInputBinder().BindInputToPublishedInterface( MouseInput( "middle_up",   Keys::Q_IGNORE_QUALIFIERS ), "special_up"          );
	GetGameInputBinder().BindInputToPublishedInterface( MouseInput( "right_down",  Keys::Q_IGNORE_QUALIFIERS ), "context_down"        );
	GetGameInputBinder().BindInputToPublishedInterface( MouseInput( "right_up",    Keys::Q_IGNORE_QUALIFIERS ), "context_up"          );
	GetGameInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_ESCAPE,Keys::Q_KEY_DOWN | Keys::Q_IGNORE_BUTTONS ), "ingame_pressed_escape"        );

	// Bind user interfaces
	GetUserInputBinder().BindDefaultInputsToPublishedInterfaces();
	GetNisInputBinder().BindDefaultInputsToPublishedInterfaces();
}



/**************************************************************************

Function		: OnDeltaXCursor()

Purpose			: Sets the mouse delta information

Returns			: bool

**************************************************************************/
bool UIGame :: OnDeltaXCursor( INT32 x )
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	m_RolloverObjData.secondsTotal = 0;

	// handle rotating formation

	if( GetShowFormation() && (m_BackendMode == BMODE_ASSIGN_FORMATION_POS) )
	{

		Player * player = gServer.GetScreenPlayer();

		if( player )
		{
			Go * party = player->GetParty();
			if( party )
			{
				Formation * formation = party->GetParty()->GetFormation();
				if( formation )
				{
// postponed -bk
//					if( gAppModule.GetControlKey() )
//					{
//						formation->RotateMembers( -float(x) * ( PI / 200.0f ) );
//					}
//					else
					{
						formation->Rotate( -float(x) * ( PI / 200.0f ) );
					}
				}
			}
		}
	}
	else
	{
		bool bFixed =	gUICamera.GetCameraMode() == CMODE_DOLLY ||
						gUICamera.GetCameraMode() == CMODE_ZOOM ;						
		
		// Update the camera accordingly
		if ( m_bInvertCameraXAxis || bFixed )
		{
			gUICamera.MouseDeltaX(x);
		}
		else
		{
			gUICamera.MouseDeltaX(-x);
		}
	}
	return true;
};



/**************************************************************************

Function		: OnDeltaYCursor()

Purpose			: Sets the mouse delta information

Returns			: bool

**************************************************************************/
bool UIGame :: OnDeltaYCursor( INT32 y )
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	m_RolloverObjData.secondsTotal = 0;

	bool bFixed =	gUICamera.GetCameraMode() == CMODE_DOLLY ||
					gUICamera.GetCameraMode() == CMODE_ZOOM;
					

	// Update the camera accordingly
	if ( m_bInvertCameraYAxis && !bFixed )
	{
		gUICamera.MouseDeltaY(-y);
	}
	else
	{
		gUICamera.MouseDeltaY(y);
	}

	return true;
}



/**************************************************************************

Function		: OnDeltaZCursor()

Purpose			: Sets the mouse delta information

Returns			: bool

**************************************************************************/

bool UIGame :: OnDeltaZCursor( INT32 z )
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	if( z < 0 )
	{
		FormationIncreaseSpacing();
	}
	else
	{
		FormationDecreaseSpacing();
	}
	return false;
}



/**************************************************************************

Function		: OnSpecialDown()

Purpose			: Handles the mouse input for the special-mapped button down
				  ( currently mapped to the middle mouse button )

Returns			: bool

**************************************************************************/
bool UIGame :: OnSpecialDown()
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	// Tell everyone that we have special down
	m_bSpecialDown = true;

	// Put the camera into free look
	if( !gUICamera.GetFreeLook() )
	{
		gUICamera.SetFreeLook( true );
	}

	return false;
}



/**************************************************************************

Function		: OnSpecialUp()

Purpose			: Handles the mouse input for the special-mapped button up
				  ( currently mapped to the middle mouse button )

Returns			: bool

**************************************************************************/
bool UIGame :: OnSpecialUp()
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	// Tell everyone that we no longer have special down
	m_bSpecialDown = false;

	// Take the camera out of free look
	if( gUICamera.GetFreeLook() )
	{
		gUICamera.SetFreeLook( false );
	}

	return false;
}


/**************************************************************************

Function		: OnSelectionDown()

Purpose			: Handles the mouse input for the selection-mapped button
				  down.	  ( currently mapped to the left mouse button )

Returns			: bool

**************************************************************************/
bool UIGame :: OnSelectionDown()
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	if ( m_pUIItemOverlay->GetHitItemOverlay() )
	{
		return false;
	}
	else if ( m_pUIItemOverlay->GetDragItemOverlay() )
	{
		return true;
	}

	gpstring command;

	////////////////////////////////////////
	//	possibly turn on formation editing

	if( GetShowFormation() )
	{
		Formation * formation;
		if( gServer.GetScreenFormation( formation ) )
		{
			GoHandle actor( GetGoUnderCursor() );
			Player * player = gServer.GetScreenPlayer();

			if( actor.IsValid() && ( actor->GetPlayer() == player ) )
			{
				SetDragged( actor->GetGoid() );
				m_BackendMode = BMODE_ASSIGN_FORMATION_SPOT;

				formation->SetDrawFreeSpots( true );

				return true;
			}
		}
	}
	
	////////////////////////////////////////////////////////////////////////////////
	//	process normal interface

	if( gSiegeEngine.GetMouseShadow().IsHit() )
	{
		////////////////////////////////////////
		//	hit object
	}
	else if( gSiegeEngine.GetMouseShadow().IsTerrainHit() )
	{
		////////////////////////////////////////
		//	hit terrain

		if( m_BackendMode == BMODE_ASSIGN_FORMATION_POS )
		{
			Formation * formation	= gServer.GetScreenFormation();
			formation->SetNextShape();
			if ( gUIPartyManager.GetFormationsActive() && gUIPartyManager.GetFieldCommands() )
			{
				gUIPartyManager.SetFormationsActive( true );
			}
		}
		else 
		{
			if ( !gUIShell.GetItemActive() )
			{
				SetBackendMode( BMODE_SELECTION_BOX );
				SetSelectionBox( true );					
			}			
		}
	}
	else
	{
		if ( !gUIShell.GetItemActive() )
		{
			SetBackendMode( BMODE_SELECTION_BOX );
			SetSelectionBox( true );				
		}
	}
	
	m_bSelectionDown = true;
	return true;
}


/**************************************************************************

Function		: OnSelectionUp()

Purpose			: Handles the mouse input for the selection-mapped button
				  up.	  ( currently mapped to the left mouse button )

Returns			: bool

**************************************************************************/
bool UIGame :: OnSelectionUp()
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	if ( m_pUIItemOverlay->GetHitItemOverlay() )
	{
		return false;
	}
	else if ( m_pUIItemOverlay->GetDragItemOverlay() )
	{
		m_pUIItemOverlay->SetDragItemOverlay( false );
	}

	////////////////////////////////////////////////////////////////////////////////
	//	process specific modes
	
	if ( m_BackendMode == BMODE_DRAG_ITEM )
	{
		Goid object = MakeGoid( gUIShell.GetRolloverItemslotID() );
		if ( object == GOID_INVALID )
		{
			object = GetGoUnderCursor();
		}
		gUIInventoryManager.DragSelectedItemsToActor( object );
		gUIGame.SetBackendMode( BMODE_NONE );
	}
	else if ( m_BackendMode == BMODE_SELECTION_BOX && gSiegeEngine.GetMouseShadow().UseSelectFrustum() )
	{
		if ( !gAppModule.GetControlKey() )
		{
			gGoDb.DeselectAll();
		}
		else
		{
			SelectionChanged();
		}

		SelectObjectsInBox();

		m_BackendMode = BMODE_NONE;
	}
	else if ( m_BackendMode == BMODE_DRAG_INVENTORY && gSiegeEngine.GetMouseShadow().IsHit() && gUIShell.GetItemActive() )
	{
		Goid object = MakeGoid(gSiegeEngine.GetMouseShadow().GetHit());

		UIItemVec item_vec;
		gUIShell.FindActiveItems( item_vec );
		UIItemVec::iterator i = item_vec.begin();
		GoHandle hItem( MakeGoid((*i)->GetItemID()) );

		GoHandle hObject( object );
		Goid member = gUIGame.GetActorWhoCarriesObject( hItem->GetGoid() );
		GoHandle hMember( member );		

		bool bSuccess = false;
		bool bTrading = false;
		if ( !hObject->IsGhost() )
		{			
			if ( gUIPartyManager.IsPlayerPartyMember( object ) )
			{		
				for ( i = item_vec.begin(); i != item_vec.end(); ++i )
				{
					bSuccess = GetUICommands()->CommandGive( MakeGoid((*i)->GetItemID()), MakeGoid(gSiegeEngine.GetMouseShadow().GetHit()) );
				}
			}
			else if ( hObject->HasActor() && hObject->GetGoid() != hMember->GetGoid() && hMember->GetMind()->IsFriend( hObject ) &&
				  ( hObject->GetPlayer()->IsRemote() && hObject->GetPlayer()->GetController() == PC_HUMAN ) )
			{			
				gUIInventoryManager.RSReqInitiateTrade( hMember, hObject, hItem );
				bSuccess = true;
				bTrading = true;
			}
		}

		if ( bSuccess )
		{
			if ( !bTrading )
			{
				gUIShell.GetMessenger()->SendUIMessage( UIMessage( MSG_DEACTIVATEITEMS ) );
			}
		}
		else
		{
			gUIInventoryManager.DropSelectedItemsFromActor();
		}
	}
	else if ( (m_BackendMode == BMODE_DRAG_INVENTORY || m_BackendMode == BMODE_NONE)  && gUIShell.GetItemActive() )
	{
		gUIInventoryManager.DropSelectedItemsFromActor();
	}
	else if ( m_BackendMode != BMODE_ASSIGN_FORMATION_POS )
	{	
		m_LastHitGO						= GOID_INVALID;
		m_LastHitTerrainPosition		= SiegePos::INVALID;
		m_bLastHitTerrainPositionValid	= false;

		bool bObjectAction = false;
		if( gSiegeEngine.GetMouseShadow().IsHit() )
		{
			////////////////////////////////////////
			//	hit object

			bObjectAction = true;
			GoHandle hObject( MakeGoid( gSiegeEngine.GetMouseShadow().GetHit() ) );
			if ( hObject.IsValid() && hObject->HasAspect() && hObject->GetAspect()->GetForceNoRender() )
			{
				bObjectAction = false;
			}

			GopColl::const_iterator i, ibegin = gGoDb.GetMouseShadowedGosBegin(), iend = gGoDb.GetMouseShadowedGosEnd();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( (*i)->IsSelectable() && !(*i)->IsInsideInventory() )
				{
					m_LastHitGO = (*i)->GetGoid();
					break;
				}
			}

			////////////////////////////////////////
			//	give commands to selection

			GoidColl selectedparty;
			gUIPartyManager.GetSelectedPartyMembers( selectedparty );

			GoHandle lastHitGo( m_LastHitGO );


			if( lastHitGo && !selectedparty.empty() )
			{					
				if ( lastHitGo->IsScreenPartyMember() )
				{
					bObjectAction = SelectObjectsInBox();
				}				

				bool stackCommands = gAppModule.GetControlKey();

				if( !lastHitGo->IsSelected() )
				{
					GetUICommands()->ContextActionOnGameObject( m_LastHitGO, stackCommands );
				}
				else
				{
					GopColl::iterator j, jbegin = gGoDb.GetSelectionBegin(), jend = gGoDb.GetSelectionEnd();
					for ( j = jbegin ; j != jend ; ++j )
					{
						if( !gUIPartyManager.IsPlayerPartyMember( (*j)->GetGoid() ) )
						{
							GetUICommands()->ContextActionOnGameObject( (*j)->GetGoid(), stackCommands );
							stackCommands = true;
						}
					}				
				}
			}
		}
		
		if ( !bObjectAction )
		{
			if( gSiegeEngine.GetMouseShadow().IsTerrainHit() )
			{
				////////////////////////////////////////
				//	mouse hit terrain

				m_LastHitTerrainPosition = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
				m_bLastHitTerrainPositionValid	= true;
				gpassertm( m_LastHitTerrainPosition.node.IsValid(), "UIGame :: OnSelectionDown - Goal node is invalid!!!" );

				Go * party				= gServer.GetScreenParty();
				Formation * formation	= gServer.GetScreenFormation();

				GoidColl selectedparty;
				gUIPartyManager.GetSelectedPartyMembers( selectedparty );

				if( party && formation && formation->GetPosition() != SiegePos::INVALID )
				{

					// If we're going to be moving, we have to shut down any stores
					gUIPartyManager.HandleMenusFromCommand();

					////////////////////////////////////////
					//	move to formation
					RankPartyMembers();
					bool bMoved = formation->Move( gAppModule.GetControlKey() ? QP_BACK : QP_CLEAR, AO_USER, gUIPartyManager.GetFollowMode() );

					GoHandle hMember;
					GoidColl selectedparty;
					gUIPartyManager.GetSelectedPartyMembers( selectedparty );					
					if( !selectedparty.empty() ) 
					{
						hMember = selectedparty.front();
						if ( bMoved )
						{
							hMember->PlayVoiceSound( "order_move", false );
						}
						else
						{
							hMember->PlayVoiceSound( "order_cant_move", false );
						}
					}	
				}
				else if( m_bLastHitTerrainPositionValid && !selectedparty.empty() )
				{
					////////////////////////////////////////
					//	move selection

					gpassertm( m_LastHitTerrainPosition.node.IsValid(), "UIGame :: OnContextUp - Goal node is invalid!!!" );
					GetUICommands()->ContextActionOnTerrain( m_LastHitTerrainPosition );
				}

				if( formation )
				{
					formation->SetDrawFreeSpots( false );
					SetShowFormation( FLT_MAX );
				}
			}
			else 
			{					
				Formation * formation	= gServer.GetScreenFormation();
				if( formation && formation->GetPosition() != SiegePos::INVALID )
				{
					RankPartyMembers();
					bool bMoved = formation->Move( gAppModule.GetControlKey() ? QP_BACK : QP_CLEAR, AO_USER, gUIPartyManager.GetFollowMode() );
					formation->SetDrawFreeSpots( false );
					formation->SetDrawAssignedSpots( false );
					SetShowFormation( FLT_MAX );

					GoHandle hMember;
					GoidColl selectedparty;
					gUIPartyManager.GetSelectedPartyMembers( selectedparty );					
					if( !selectedparty.empty() ) 
					{
						hMember = selectedparty.front();
						if ( bMoved )
						{
							hMember->PlayVoiceSound( "order_move", false );
						}
						else
						{
							hMember->PlayVoiceSound( "order_cant_move", false );
						}
					}
				}
			}
		}
		m_BackendMode = BMODE_NONE;		
	}

	// Hide Formation Indicators
	if ( m_BackendMode != BMODE_ASSIGN_FORMATION_POS )
	{
		Formation * formation = gServer.GetScreenFormation();
		if ( formation )
		{
			formation->HideSpots();
			formation->SetDirty( false );
		}
	}

	gUIPartyManager.EndDataBarDock();
	SetSelectionBox( false );
	m_bSelectionDown = false;
	return true;
}


/**************************************************************************

Function		: OnContextDown()

Purpose			: Handles the mouse input for the context-mapped button
				  down.	  ( currently mapped to the right mouse button )

Returns			: bool

**************************************************************************/
bool UIGame :: OnContextDown()
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	if ( m_pUIItemOverlay->GetHitItemOverlay() )
	{
		return false;
	}
	else if ( m_pUIItemOverlay->GetDragItemOverlay() )
	{
		return true;
	}
	
	SetSelectionBox( false );
	m_bContextDown = true;

	m_LastHitGO						= GOID_INVALID;
	m_LastHitTerrainPosition.pos	= vector_3(0,0,0);
	m_bLastHitTerrainPositionValid	= false;
	
	bool bObjectAction = false;
	if( gSiegeEngine.GetMouseShadow().IsHit() )
	{
		////////////////////////////////////////
		//	hit object

		bObjectAction = true;
		GoHandle hObject( MakeGoid( gSiegeEngine.GetMouseShadow().GetHit() ) );
		if ( (hObject.IsValid() && hObject->HasAspect() && hObject->GetAspect()->GetForceNoRender()) ||
			 (hObject.IsValid() && hObject->IsScreenPartyMember() && hObject->IsSelected() && 
			  GetUICommands()->GetActiveContextCursor() == CURSOR_GUI_STANDARD ) )
		{
			bObjectAction = false;
		}
		
		if ( !gAppModule.GetRButton() )
		{
			gUIInventoryManager.PickupUIItem();			
		}
	}

	if ( !bObjectAction )
	{
		if( gSiegeEngine.GetMouseShadow().IsTerrainHit()  )
		{
			////////////////////////////////////////
			//	hit terrain

			m_LastHitTerrainPosition = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
			m_bLastHitTerrainPositionValid	= true;
			gpassertm( m_LastHitTerrainPosition.node.IsValid(), "UIGame :: OnContextDown - Goal node is invalid!!!" );

			if( m_LastHitGO == GOID_INVALID )
			{
				///////////////////////////////////////////
				//	place formation, if appropriate

				Go * party				= gServer.GetScreenParty();
				Formation * formation	= gServer.GetScreenFormation();

				if( Length( gSiegeEngine.GetDifferenceVector( gSiegeEngine.GetMouseShadow().GetFloorHitPos(), party->GetPlacement()->GetPosition() ) ) > 0.25f )
				{
					if( party && formation )
					{
						RankPartyMembers();

						m_TimeToDisplayFormation = ::GetGlobalSeconds() + 0.33;

						Player * player = gServer.GetScreenPlayer();
						gpassert( player == formation->GetParty()->GetGo()->GetPlayer() );

						m_BackendMode = BMODE_ASSIGN_FORMATION_POS;

						vector_3 Direction = -gSiegeEngine.GetDifferenceVector( gSiegeEngine.GetMouseShadow().GetFloorHitPos(), party->GetPlacement()->GetPosition() );

						formation->SetPosition( gSiegeEngine.GetMouseShadow().GetFloorHitPos() );
						formation->SetDirection( Direction );
						formation->SetShape( formation->GetShape() );
					}
				}
			}
		}
	}
	
	return true;
}


/**************************************************************************

Function		: OnContextUp()

Purpose			: Handles the mouse input for the context-mapped button
				  up.	  ( currently mapped to the right mouse button )

Returns			: bool

**************************************************************************/
bool UIGame :: OnContextUp()
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	if ( m_pUIItemOverlay->GetHitItemOverlay() )
	{
		return false;
	}
	else if ( m_pUIItemOverlay->GetDragItemOverlay() )
	{
		m_pUIItemOverlay->SetDragItemOverlay( false );
	}

	m_LastHitGO						= GOID_INVALID;
	m_LastHitTerrainPosition		= SiegePos::INVALID;
	m_bLastHitTerrainPositionValid	= false;

	bool bObjectAction = false;
	if ( m_BackendMode == BMODE_DRAG_ITEM )
		{
			Goid object = MakeGoid( gUIShell.GetRolloverItemslotID() );
			if ( object == GOID_INVALID )
			{
				object = GetGoUnderCursor();
			}
			gUIInventoryManager.DragSelectedItemsToActor( object );
			gUIGame.SetBackendMode( BMODE_NONE );
	}
	else if ( gSiegeEngine.GetMouseShadow().IsHit() )
	{
		////////////////////////////////////////
		//	hit object

		bObjectAction = true;
		GoHandle hObject( MakeGoid( gSiegeEngine.GetMouseShadow().GetHit() ) );
		if ( hObject.IsValid() && hObject->HasAspect() && hObject->GetAspect()->GetForceNoRender() )
		{
			bObjectAction = false;
		}

		GopColl::const_iterator i, ibegin = gGoDb.GetMouseShadowedGosBegin(), iend = gGoDb.GetMouseShadowedGosEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( (*i)->IsSelectable() && !(*i)->IsInsideInventory() )
			{
				m_LastHitGO = (*i)->GetGoid();
				break;
			}
		}

		////////////////////////////////////////
		//	give commands to selection

		GoidColl selectedparty;
		gUIPartyManager.GetSelectedPartyMembers( selectedparty );

		GoHandle lastHitGo( m_LastHitGO );

		if( lastHitGo && !selectedparty.empty() )
		{	
			bool bHandled = false;

			bool stackCommands = gAppModule.GetControlKey();

			if( !lastHitGo->IsSelected() )
			{
				bHandled = GetUICommands()->ContextActionOnGameObject( m_LastHitGO, stackCommands );
			}
			else
			{
				GopColl selection = gGoDb.GetSelection();
				GopColl::iterator j, jbegin = selection.begin(), jend = selection.end();
				for ( j = jbegin ; j != jend ; ++j )
				{	
					if ( !(*j)->IsScreenPartyMember() || ( (*j)->IsScreenPartyMember() && (*j) == lastHitGo ) )
					{
						bHandled = GetUICommands()->ContextActionOnGameObject( (*j)->GetGoid(), stackCommands );
						stackCommands = true;					
					}
				}				
			}

			if ( !bHandled )
			{
				bObjectAction = false;
			}
		}			
	}
	
	if ( !bObjectAction )
	{
		if ( gSiegeEngine.GetMouseShadow().IsTerrainHit()  )
		{
			////////////////////////////////////////
			//	hit terrain

			m_LastHitTerrainPosition = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
			m_bLastHitTerrainPositionValid	= true;

			if ( !GetUICommands()->ContextCastOnTerrain( m_LastHitTerrainPosition, gAppModule.GetControlKey() ) )
			{
				GoidColl selectedparty;
				gUIPartyManager.GetSelectedPartyMembers( selectedparty );

				gpassertm( m_LastHitTerrainPosition.node.IsValid(), "UIGame :: OnContextDown - Goal node is invalid!!!" );
				
				Go * party				= gServer.GetScreenParty();
				Formation * formation	= gServer.GetScreenFormation();

				if ( formation && formation->GetDrawAssignedSpots() )
				{			
					if( party && formation && formation->GetPosition() != SiegePos::INVALID )
					{
						// If we're going to be moving, we have to shut down any stores
						gUIPartyManager.HandleMenusFromCommand();

						////////////////////////////////////////
						//	move formation
						RankPartyMembers();
						bool bMoved = formation->Move( gAppModule.GetControlKey() ? QP_BACK : QP_CLEAR, AO_USER, gUIPartyManager.GetFollowMode(), true );
						formation->SetDrawFreeSpots( false );
						formation->SetDrawAssignedSpots( false );

						GoHandle hMember;
						GoidColl selectedparty;
						gUIPartyManager.GetSelectedPartyMembers( selectedparty );					
						if( !selectedparty.empty() ) 
						{
							hMember = selectedparty.front();
							if ( bMoved )
							{
								hMember->PlayVoiceSound( "order_move", false );
							}
							else
							{
								hMember->PlayVoiceSound( "order_cant_move", false );
							}
						}					
					}
					else if( m_bLastHitTerrainPositionValid && !selectedparty.empty() )
					{
						////////////////////////////////////////
						//	move selection

						gpassertm( m_LastHitTerrainPosition.node.IsValid(), "UIGame :: OnContextUp - Goal node is invalid!!!" );
						GetUICommands()->ContextActionOnTerrain( m_LastHitTerrainPosition );
					}
				}
				else if ( formation )
				{
					formation->SetPosition( SiegePos::INVALID );
				}

				SetShowFormation( FLT_MAX );								
			}
			else
			{
				Formation * formation	= gServer.GetScreenFormation();
				formation->SetDrawFreeSpots( false );
				formation->SetDrawAssignedSpots( false );
				formation->SetPosition( SiegePos::INVALID );
				SetShowFormation( FLT_MAX );
			}
		}
		else
		{
			Formation * formation	= gServer.GetScreenFormation();
			if ( formation && formation->GetPosition() != SiegePos::INVALID )
			{
				RankPartyMembers();
				bool bMoved = formation->Move( gAppModule.GetControlKey() ? QP_BACK : QP_CLEAR, AO_USER, gUIPartyManager.GetFollowMode() );
				formation->SetDrawFreeSpots( false );
				formation->SetDrawAssignedSpots( false );
				SetShowFormation( FLT_MAX );

				GoHandle hMember;
				GoidColl selectedparty;
				gUIPartyManager.GetSelectedPartyMembers( selectedparty );					
				if( !selectedparty.empty() ) 
				{
					hMember = selectedparty.front();
					if ( bMoved )
					{
						hMember->PlayVoiceSound( "order_move", false );
					}
					else
					{
						hMember->PlayVoiceSound( "order_cant_move", false );
					}
				}
			}
			else
			{
				GoidColl selectedparty;
				gUIPartyManager.GetSelectedPartyMembers( selectedparty );
				GoHandle hMember;
				if ( !selectedparty.empty() ) 
				{
					hMember = selectedparty.front();
					hMember->PlayVoiceSound( "order_cant_move", false );
				}								 
			}
		}
	}

	if ( !gUIShell.GetItemActive() )
	{
		m_BackendMode = BMODE_NONE;
	}

	// Hide Formation Indicators
	if ( m_BackendMode != BMODE_ASSIGN_FORMATION_POS )
	{
		Formation * formation = gServer.GetScreenFormation();
		if ( formation )
		{
			formation->HideSpots();
			formation->SetDirty( false );
		}
	}

	SetSelectionBox( false );
	m_bContextDown = false;
	return true;
}




/**************************************************************************

Function		: GamePause()

Purpose			: Pauses the game

Returns			: bool

**************************************************************************/
bool UIGame :: RSGamePauseToggle()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	if ( gUIShell.IsInterfaceVisible( "in_game_menu" ) || 
		 gUIShell.IsInterfaceVisible( "world_tip" ) )
	{
		return true;
	}
	if ( gServer.GetAllowPausing() || gServer.IsLocal() )
	{
		gTattooGame.RSUserPause( !gAppModule.IsUserPaused() );
	}
	//TattooGame.RSPauseAllClients(  );
	return true;
}


/*

void UIGame :: RSSetGamePause( bool flag )
{
	FUBI_RPC_CALL( RSSetGamePause, RPC_TO_SERVER );

	// $$$ check security

	RCSetGamePause( flag );
}


void UIGame :: RCSetGamePause( bool flag )
{
	FUBI_RPC_CALL( RCSetGamePause, RPC_TO_ALL );

	gUIShell.ShowGroup( "paused", flag );
	gAppModule.UserPause( flag );
}
*/


/**************************************************************************

Function		: GameSpeedUp()

Purpose			: Steps up the game speed by a set value

Returns			: bool

**************************************************************************/
bool UIGame :: GameSpeedUp()
{
#	if GP_RETAIL
	if( ::IsMultiPlayer() )
	{
		return true;		// $$ speed changing not allowed in retail MP for now
	}
#	endif // GP_RETAIL

	float multiplier = gAppModule.GetTimeMultiplier();

#if GP_RETAIL

	multiplier += 0.1f;

	if ( multiplier > OPTION_MAX_GAME_SPEED )
	{
		multiplier = OPTION_MAX_GAME_SPEED;
	}

#else

	if ( multiplier >= 10.0f )
	{
		multiplier = 10.0f;
	}
	else if ( multiplier >= 0.2f )
	{
		multiplier += 0.1f;
	}
	else if ( multiplier >= 0.02f )
	{
		multiplier += 0.01f;
	}
	else
	{
		multiplier += 0.001f;
	}

#endif

	gWorldOptions.RSSetGameSpeed( multiplier );

	return true;
}



/**************************************************************************

Function		: GameSlowDown()

Purpose			: Steps down the game speed by a set value

Returns			: bool

**************************************************************************/
bool UIGame :: GameSpeedDown()
{
#	if GP_RETAIL
	if( ::IsMultiPlayer() )
	{
		return true;		// $$ speed changing not allowed in retail MP for now
	}
#	endif // GP_RETAIL

	float multiplier = gAppModule.GetTimeMultiplier();

#if GP_RETAIL

	multiplier -= 0.1f;

	if ( multiplier < OPTION_MIN_GAME_SPEED )
	{
		multiplier = OPTION_MIN_GAME_SPEED;
	}

#else

	if ( multiplier <= 0.001f )
	{
		multiplier = 0.001f;
	}
	else if ( multiplier <= 0.02f )
	{
		multiplier -= 0.001f;
	}
	else if ( multiplier <= 0.2f )
	{
		multiplier -= 0.01f;
	}
	else if ( multiplier <= 10.0f )
	{
		multiplier -= 0.1f;
	}

#endif

	gWorldOptions.RSSetGameSpeed( multiplier );

	return true;
}


/**************************************************************************

Function		: GameSpeedReset()

Purpose			: Sets the game speed to 1.0

Returns			: bool

**************************************************************************/
bool UIGame :: GameSpeedReset()
{
#	if GP_RETAIL
	if( ::IsMultiPlayer() )
	{
		return true;		// $$ speed changing not allowed in retail MP for now
	}
#	endif // GP_RETAIL

	gWorldOptions.RSSetGameSpeed( 1.0f );

	return true;
}


bool UIGame :: GameSpeedMin()
{
#	if GP_RETAIL
	if( ::IsMultiPlayer() )
	{
		return true;		// $$ speed changing not allowed in retail MP for now
	}
#	endif // GP_RETAIL

	gWorldOptions.RSSetGameSpeed( 0.001f );

	return true;
}


bool UIGame :: GameSpeedMax()
{
#	if GP_RETAIL
	if( ::IsMultiPlayer() )
	{
		return true;		// $$ speed changing not allowed in retail MP for now
	}
#	endif // GP_RETAIL

	gWorldOptions.RSSetGameSpeed( 10.0f );

	return true;
}


/**************************************************************************

Function		: GameSingleStep()

Purpose			: Steps down the game speed by a set value

Returns			: bool

**************************************************************************/
bool UIGame :: GameSingleStep(void)
{
	if( !::IsMultiPlayer() )
	{
		gAppModule.UserPause();
		gAppModule.SetSingleStep();
	}
	else
	{
		gpwarning( "Sorry - can't single step in multiplayer just yet." );
	}

	return true;
}

void UIGame :: OnAppActivate( bool activate )
{
	if ( !activate )
	{
		m_bSelectionDown	= false;
		m_bSpecialDown		= false;
		m_bContextDown		= false;

		SetSelectionBox( false );
		if ( UIShell::DoesSingletonExist() && !gUIShell.GetItemActive() )
		{
			SetBackendMode( BMODE_NONE );
		}		

		if ( UICamera::DoesSingletonExist() )
		{
			if ( WorldState::DoesSingletonExist() && AppModule::DoesSingletonExist() )
			{
				CameraInputRecalibrate( true );	
			}
			gUICamera.SetFreeLook( false );
		}
		

		if ( Server::DoesSingletonExist() )
		{
			Formation * pFormation;
			if( gServer.GetScreenFormation( pFormation ) )
			{
				pFormation->HideSpots();
				SetShowFormation( FLT_MAX );
			}		
		}
	}
}

bool UIGame :: SetCameraNoneModeOn()
{
	gAppModule.SetMouseModeFixed( false );

	gUICamera.SetCameraMode( CMODE_NONE );
	return true;
}

bool UIGame :: SetCameraDollyModeOn()
{
	gAppModule.SetMouseModeFixed();

	gUICamera.SetCameraMode( CMODE_DOLLY );
	return true;
}

bool UIGame :: SetCameraZoomModeOn()
{
	gAppModule.SetMouseModeFixed();

	gUICamera.SetCameraMode( CMODE_ZOOM );
	return true;
}

bool UIGame :: SetCameraOrbitModeOn()
{
	gAppModule.SetMouseModeFixed();

	gUICamera.SetCameraMode( CMODE_ORBIT );
	return true;
}

bool UIGame :: SetCameraCraneModeOn()
{
	gAppModule.SetMouseModeFixed();

	gUICamera.SetCameraMode( CMODE_CRANE );
	return true;
}

bool UIGame :: SetCameraTrackModeOn()
{
	gAppModule.SetMouseModeFixed( false );

	gUICamera.SetCameraMode( CMODE_TRACK );
	return true;
}

bool UIGame :: ToggleCameraTrackAndHold()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	gAppModule.SetMouseModeFixed( false );

	UIWindow * pWindow = gUIShell.FindUIWindow( "window_camera", "compass_hotpoints" );

	// If we are already in track mode, then goto hold mode
	if( gUICamera.GetCameraMode() == CMODE_TRACK )
	{	
		if ( pWindow )
		{
			pWindow->SetVisible( true );
			pWindow->ReceiveMessage( UIMessage( MSG_STARTANIMATION ) );
		}
		gUICamera.SetCameraMode( CMODE_HOLD );
	}
	else
	{
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}
		// Any other mode causes us to switch into track
		gUICamera.SetCameraMode( CMODE_TRACK );
	}
	return true;
}

bool UIGame :: CameraZoomIn( const Input & input )
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	if ( gWorldState.GetCurrentState() == WS_MEGA_MAP )
	{
		// Zoom in the minimap
		if ( input.IsKey() )
		{
			const KeyInput & key = input.GetKey();
			if ( key.GetQualifiers() & Keys::Q_KEY_DOWN )
			{
				if ( !gSiegeEngine.NodeWalker().GetMiniMapZoomIn() && !gSiegeEngine.NodeWalker().GetMiniMapZoomOut() )
				{
					gSiegeEngine.NodeWalker().SetMiniMapZoomIn( true );					
				}				
			}
			else if ( key.GetQualifiers() & Keys::Q_KEY_UP )
			{
				if ( gSiegeEngine.NodeWalker().GetMiniMapZoomIn() )
				{
					gSiegeEngine.NodeWalker().SetMiniMapZoomIn( false );
				}
			}
		}
		else if ( input.IsMouse() )
		{
			const MouseInput & mouse = input.GetMouse();
			if ( mouse.GetName().same_no_case( "wheel_up" ) || mouse.GetName().same_no_case( "wheel_down" ) )
			{
				gSiegeEngine.NodeWalker().StepMiniMapZoomIn();
			}
		}

		if ( gSiegeEngine.NodeWalker().GetMiniMapMeters() <= GetMapShowBorderMeters() )
		{
			gUIShell.ShowGroup( "map_border", false, false, "mini_map" );
		}
	}
	else
	{
		// Zoom in the game cam
		if ( input.IsKey() )
		{
			const KeyInput & key = input.GetKey();
			if ( key.GetQualifiers() & Keys::Q_KEY_DOWN )
			{
				if ( !gUICamera.IsZoomingIn() && !gUICamera.IsZoomingOut() )
				{
					gUICamera.SetZoomingIn( true );
				}
			}
			else if ( key.GetQualifiers() & Keys::Q_KEY_UP )
			{
				if ( gUICamera.IsZoomingIn() )
				{
					gUICamera.SetZoomingIn( false );
				}
			}
		}
		else if ( input.IsMouse() )
		{
			const MouseInput & mouse = input.GetMouse();
			if ( mouse.GetName().same_no_case( "wheel_up" ) || mouse.GetName().same_no_case( "wheel_down" ) )
			{
				m_RolloverObjData.pText->SetVisible( false );
				m_RolloverObjData.secondsTotal = 0;

				float delta = gUICamera.MouseDeltaZ( abs( mouse.GetDelta() ) );
				UpdateZDeltaFog( delta );
			}
		}
	}

	return true;
}

bool UIGame :: CameraZoomOut( const Input & input )
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	if ( gWorldState.GetCurrentState() == WS_MEGA_MAP )
	{
		// Zoom in the minimap
		if ( input.IsKey() )
		{
			const KeyInput & key = input.GetKey();
			if ( key.GetQualifiers() & Keys::Q_KEY_DOWN )
			{
				if ( !gSiegeEngine.NodeWalker().GetMiniMapZoomIn() && !gSiegeEngine.NodeWalker().GetMiniMapZoomOut() )
				{
					gSiegeEngine.NodeWalker().SetMiniMapZoomOut( true );					
				}				
			}
			else if ( key.GetQualifiers() & Keys::Q_KEY_UP )
			{
				if ( gSiegeEngine.NodeWalker().GetMiniMapZoomOut() )
				{
					gSiegeEngine.NodeWalker().SetMiniMapZoomOut( false );
				}
			}
		}
		else if ( input.IsMouse() )
		{
			const MouseInput & mouse = input.GetMouse();
			if ( mouse.GetName().same_no_case( "wheel_up" ) || mouse.GetName().same_no_case( "wheel_down" ) )
			{
				gSiegeEngine.NodeWalker().StepMiniMapZoomOut();
			}
		}

		if ( gSiegeEngine.NodeWalker().GetMiniMapMeters() > GetMapShowBorderMeters() )
		{
			gUIShell.ShowGroup( "map_border", true, false, "mini_map" );
		}
	}
	else
	{
		if ( input.IsKey() )
		{
			const KeyInput & key = input.GetKey();
			if ( key.GetQualifiers() & Keys::Q_KEY_DOWN )
			{
				if ( !gUICamera.IsZoomingIn() && !gUICamera.IsZoomingOut() )
				{
					gUICamera.SetZoomingOut( true );
				}
			}
			else if ( key.GetQualifiers() & Keys::Q_KEY_UP )
			{
				if ( gUICamera.IsZoomingOut() )
				{
					gUICamera.SetZoomingOut( false );
				}
			}
		}
		else if ( input.IsMouse() )
		{
			const MouseInput & mouse = input.GetMouse();
			if ( mouse.GetName().same_no_case( "wheel_up" ) || mouse.GetName().same_no_case( "wheel_down" ) )
			{
				m_RolloverObjData.pText->SetVisible( false );
				m_RolloverObjData.secondsTotal = 0;

				float delta = gUICamera.MouseDeltaZ( -abs( mouse.GetDelta() ) );
				UpdateZDeltaFog( delta );
			}
		}
	}
	return true;
}

bool UIGame :: CameraRotateLeft( const KeyInput & key )
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	if ( key.GetQualifiers() & Keys::Q_KEY_DOWN )
	{
		if ( m_bInvertCameraXAxis )
		{
			if ( !gUICamera.IsRotatingLeft() )
			{
				gUICamera.SetRotateLeft( true, 1.0f );
			}
		}
		else
		{
			if ( !gUICamera.IsRotatingRight() )
			{	
				gUICamera.SetRotateRight( true, 1.0f );				
			}
		}
		m_bCameraKeyRotating = true;
		
	}
	else if ( key.GetQualifiers() & Keys::Q_KEY_UP )
	{
		if ( m_bInvertCameraXAxis )
		{
			if ( gUICamera.IsRotatingLeft() )
			{	
				gUICamera.SetRotateLeft( false, 1.0f );
			}
		}
		else
		{
			if ( gUICamera.IsRotatingRight() )
			{	
				gUICamera.SetRotateRight( false, 1.0f );				
			}
		}
		m_bCameraKeyRotating = false;	
	}
	return true;
}

bool UIGame :: CameraRotateRight( const KeyInput & key )
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	if ( key.GetQualifiers() & Keys::Q_KEY_DOWN )
	{		
		if ( m_bInvertCameraXAxis )
		{
			if ( !gUICamera.IsRotatingRight() )
			{
				gUICamera.SetRotateRight( true, 1.0f );
			}
		}
		else
		{
			if ( !gUICamera.IsRotatingLeft() )
			{
				gUICamera.SetRotateLeft( true, 1.0f );				
			}
		}
		
		m_bCameraKeyRotating = true;		
	}
	else if ( key.GetQualifiers() & Keys::Q_KEY_UP )
	{
		if ( m_bInvertCameraXAxis )
		{			
			if ( gUICamera.IsRotatingRight() )
			{
				gUICamera.SetRotateRight( false, 1.0f );
			}
		}
		else
		{
			if ( gUICamera.IsRotatingLeft() )
			{
				gUICamera.SetRotateLeft( false, 1.0f );				
			}
		}
		m_bCameraKeyRotating = false;		
	}
	return true;
}

bool UIGame :: CameraRotateUp( const KeyInput & key )
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	if ( key.GetQualifiers() & Keys::Q_KEY_DOWN )
	{
		if ( m_bInvertCameraYAxis )
		{
			if ( !gUICamera.IsRotatingUp() )
			{
				gUICamera.SetRotateUp( true, 1.0f );
			}
		}
		else
		{
			if ( !gUICamera.IsRotatingDown() )
			{
				gUICamera.SetRotateDown( true, 1.0f );				
			}
		}
		m_bCameraKeyRotating = true;		
	}
	else if ( key.GetQualifiers() & Keys::Q_KEY_UP )
	{
		if ( m_bInvertCameraYAxis )
		{
			if ( gUICamera.IsRotatingUp() )
			{
				gUICamera.SetRotateUp( false, 1.0f );				
			}
		}
		else
		{
			if ( gUICamera.IsRotatingDown() )
			{
				gUICamera.SetRotateDown( false, 1.0f );
			}
		}			
		m_bCameraKeyRotating = false;		
	}
	return true;
}

bool UIGame :: CameraRotateDown( const KeyInput & key )
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	if ( key.GetQualifiers() & Keys::Q_KEY_DOWN )
	{
		if ( m_bInvertCameraYAxis )
		{
			if ( !gUICamera.IsRotatingDown() )
			{	
				gUICamera.SetRotateDown( true, 1.0f );
			}
		}
		else
		{
			if ( !gUICamera.IsRotatingUp() )
			{
				gUICamera.SetRotateUp( true, 1.0f );				
			}
		}
		m_bCameraKeyRotating = true;		
	}
	else if ( key.GetQualifiers() & Keys::Q_KEY_UP )
	{
		if ( m_bInvertCameraYAxis )
		{
			if ( gUICamera.IsRotatingDown() )
			{
				gUICamera.SetRotateDown( false, 1.0f );
			}
		}
		else
		{
			if ( gUICamera.IsRotatingUp() )
			{
				gUICamera.SetRotateUp( false, 1.0f );				
			}
		}
		m_bCameraKeyRotating = false;		
	}
	return true;
}

bool UIGame :: CameraFreeLook( const KeyInput & key )
{
	if ( gUIMenuManager.GetModalActive() )
	{
		return true;
	}

	if ( key.GetQualifiers() & Keys::Q_KEY_DOWN )
	{
		// Put the camera into free look
		if( !gUICamera.GetFreeLook() )
		{
			gUICamera.SetFreeLook( true );
		}
	}
	else if ( key.GetQualifiers() & Keys::Q_KEY_UP )
	{
		// Take the camera out of free look
		if( gUICamera.GetFreeLook() )
		{
			gUICamera.SetFreeLook( false );
		}
	}
	return true;
}

void UIGame :: CameraInputRecalibrate( bool bForce )
{
	if ( bForce ||
		 gAppModule.GetLButton() || 
		 gAppModule.GetMButton() ||
		 gAppModule.GetRButton() ||
		 gAppModule.GetXButton1() ||
		 gAppModule.GetXButton2() )
	{
		if ( gWorldState.GetCurrentState() == WS_MEGA_MAP )
		{
			if ( gSiegeEngine.NodeWalker().GetMiniMapZoomOut() )
			{
				gSiegeEngine.NodeWalker().SetMiniMapZoomOut( false );
			}
			if ( gSiegeEngine.NodeWalker().GetMiniMapZoomIn() )
			{
				gSiegeEngine.NodeWalker().SetMiniMapZoomIn( false );
			}
		}

		if ( gUICamera.IsZoomingIn() )
		{
			gUICamera.SetZoomingIn( false );
		}
		if ( gUICamera.IsZoomingOut() )
		{
			gUICamera.SetZoomingOut( false );
		}
		if ( gUICamera.IsRotatingLeft() )
		{
			gUICamera.SetRotateLeft( false, 1.0f );
			m_bCameraKeyRotating = false;
		}
		if ( gUICamera.IsRotatingRight() )
		{
			gUICamera.SetRotateRight( false, 1.0f );
			m_bCameraKeyRotating = false;
		}
		if ( gUICamera.IsRotatingUp() )
		{
			gUICamera.SetRotateUp( false, 1.0f );
			m_bCameraKeyRotating = false;
		}
		if ( gUICamera.IsRotatingDown() )
		{
			gUICamera.SetRotateDown( false, 1.0f );
			m_bCameraKeyRotating = false;
		}		
	}
}

/**************************************************************************

Function		: SelectAllPartyMembers()

Purpose			: Selects all members of the player's party

Returns			: bool

**************************************************************************/
bool UIGame :: SelectAllPartyMembers()
{
	GoidColl party;	
	gUIPartyManager.GetPortraitOrderedParty( party );		

	bool bPlaySound		= false;
	bool bSelectFocus	= false;
	Goid selectGoid		= GOID_INVALID;

	GoidColl::iterator i, ibegin = party.begin(), iend = party.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Go* actor = GetGo( *i );
		if (   actor->IsInActiveWorldFrustum()
			&& !actor->IsSelected()					// $ keep this redundant check so we don't re-insert already-selected go's
			&& IsAlive( actor->GetLifeState() ) )
		{
			actor->Select();

			if ( !bPlaySound )
			{
				actor->PlayVoiceSound( "select_char", false );		
				bPlaySound = true;
			}
		}

		if( actor->IsFocused() &&
			(actor->HasInventory() && actor->GetInventory()->IsPackOnly()) )
		{
			bSelectFocus	= true;
		}

		if( actor->IsInActiveScreenWorldFrustum() && 
			(actor->HasInventory() && !actor->GetInventory()->IsPackOnly()) && (selectGoid == GOID_INVALID) )
		{
			selectGoid = (*i);
		}
	}
	
	if( bSelectFocus && (selectGoid != GOID_INVALID) )
	{
		gGoDb.RSSetFocusGo( selectGoid );
	}

	SelectionChanged();

	return true;
}



/**************************************************************************

Function		: SelectPlayer()

Purpose			: Select the next/prev available player

Returns			: bool

**************************************************************************/
bool UIGame :: SelectPlayer( bool forward )
{
	// $ early bailout - ignore request if an item is active
	if ( gUIShell.GetItemActive() )
	{
		return ( true );
	}

	// $$ enhancement: make this travel through party members in the same order
	//    that they appear in the GUI. so when the player drags and drops portraits
	//    in the GUI, it should rearrange party member order accordingly. i can
	//    add a func to Go that allows nondestructive rearrangement of children. -sb	

	int selected = 0;

	// first gather info - how many selected?
	GoidColl children;
	gUIPartyManager.GetPortraitOrderedParty( children );

	GoidColl::iterator i, ibegin = children.begin(), iend = children.end(), ifocus = iend;
	for ( i = ibegin ; i != iend ; ++i )
	{
		GoHandle hMember( *i );
		if ( hMember->IsSelected() )
		{
			gpassert( IsAlive( hMember->GetLifeState() ) || hMember->GetLifeState() == LS_GHOST );
			gpassert( hMember->IsInActiveWorldFrustum() );

			++selected;
			if ( hMember->IsFocused() )
			{
				ifocus = i;
			}
		}
	}
	i = ifocus;

	if ( (selected == 0) || (ifocus == iend) )
	{
		// select anyone
		GoHandle hMember( *ibegin );
		if ( (ibegin != iend) && IsAlive( hMember->GetLifeState() ) )
		{
			int index = 0;
			UIPartyMember * pMember = gUIPartyManager.GetCharacterFromGoid( hMember->GetGoid(), index );
			if ( pMember )
			{
				gUIPartyManager.SelectPortraitIndirect( index );
			}
	
			hMember->Select();
		}
	}
	else if ( selected == 1 )
	{
		gpassert( ifocus != iend );
		Go* firstInactive = NULL;

		// iter until we find a live active one
		for ( ; ; )
		{
			if ( forward )
			{
				++i;
				if ( i == iend )
				{
					i = ibegin;
				}
			}
			else
			{
				if ( i == ibegin )
				{
					i = iend;
				}
				--i;
			}

			GoHandle hMember( *i );
			if ( IsAlive( hMember->GetLifeState() ) || hMember->GetLifeState() == LS_GHOST )
			{
				if ( hMember->IsInActiveWorldFrustum() )
				{
					// found another inactive one
					break;
				}
				else if ( firstInactive == NULL )
				{
					// remember this in case we need it later
					firstInactive = hMember;
				}
			}
		}

		gGoDb.DeselectAll();

		// did we find the same one again?
		if ( i == ifocus )
		{
			// did we find an inactive live go?
			if ( firstInactive != NULL )
			{
				int index = 0;
				UIPartyMember * pMember = gUIPartyManager.GetCharacterFromGoid( firstInactive->GetGoid(), index );
				if ( pMember )
				{
					gUIPartyManager.SelectPortraitIndirect( index );
				}
				firstInactive->Select();
			}
		}
		else
		{
			// just select it
			GoHandle hMember( *i );
			int index = 0;
			UIPartyMember * pMember = gUIPartyManager.GetCharacterFromGoid( hMember->GetGoid(), index );
			if ( pMember )
			{
				gUIPartyManager.SelectPortraitIndirect( index );
			}
			hMember->Select();
		}
	}
	else
	{
		// just select next/prev selected party member
		for ( ; ; )
		{
			if ( forward )
			{
				++i;
				if ( i == iend )
				{
					i = ibegin;
				}
			}
			else
			{
				if ( i == ibegin )
				{
					i = iend;
				}
				--i;
			}

			GoHandle hMember( *i );

			if ( hMember->IsSelected() )
			{
				int index = 0;
				UIPartyMember * pMember = gUIPartyManager.GetCharacterFromGoid( hMember->GetGoid(), index );
				if ( pMember )
				{
					gUIPartyManager.SelectPortraitIndirect( index );
				}
				hMember->Select();
				break;
			}
		}
	}	

	return true;
}

bool UIGame :: SelectNextPlayer()
{
	return ( SelectPlayer( true ) );
}

bool UIGame :: SelectLastPlayer()
{
	return ( SelectPlayer( false ) );
}

bool UIGame :: SelectLeadPlayer()
{
	gGoDb.DeselectAll();

	GoidColl party;	
	gUIPartyManager.GetPortraitOrderedParty( party );	
	if ( party.size() != 0 )
	{
		gGoDb.Select( *(party.begin()) );
	}
	return true;
}


/**************************************************************************

Function		: CreateSelectedActorPartyList()

Purpose			: Gets a list of the selected party members

Returns			: void

**************************************************************************/
void UIGame :: CreateSelectedActorPartyList( GoidColl& selectedparty )
{
	GopColl::const_iterator i;

	for ( i = gGoDb.GetSelection().begin(); i != gGoDb.GetSelection().end(); ++i )
	{
		if( gGoDb.IsSelected( (*i)->GetGoid() ) && (*i)->HasActor() )
		{
			selectedparty.push_back( (*i)->GetGoid() );
		}
	}
}



/**************************************************************************

Function		: SelectObjectsInBox()

Purpose			: Select the objects in the drag select box

Returns			: void

**************************************************************************/
bool UIGame :: SelectObjectsInBox()
{
	if ( !gSiegeEngine.GetMouseShadow().SelectionBox() && gGoDb.GetSelection().size() == 1 )
	{
		Goid object = MakeGoid( gSiegeEngine.GetMouseShadow().GetHit() );
		GoHandle hObject( object );
		if ( hObject->IsScreenPartyMember() && hObject->IsSelected() )
		{
			return false;
		}
	}

	// Clear selection if we're not holding shift
	if( !gAppModule.GetControlKey() ) 
	{
		Selection_Clear();
	}

	bool playedSelectSound = false;

	GopColl::const_iterator i, ibegin = gGoDb.GetMouseShadowedGosBegin(), iend = gGoDb.GetMouseShadowedGosEnd();
	GopColl selected;
	for ( i = ibegin ; i != iend ; ++i )
	{
		Go* go = *i;
		if( !go->IsInsideInventory() )
		{
			// developer only - can select any item in the world, regardless of type
#			if !GP_RETAIL
			if( gAppModule.GetControlKey() && gSiegeEngine.GetOptions().SelectAllObjects() )
			{
				gUIPartyManager.SetMustHaveSelection( false );
				gGoDb.DeselectAll();
				go->Select();
			}
			else
#			endif // !GP_RETAIL

			// No control - select actors, items, weapons, armor and spells
			if ( go->IsSelectable() &&
				(go->HasActor() && go->IsScreenPartyMember() ) || ( go->IsItem() && go->HasGui() ) )
			{
				go->ToggleSelected();
				selected.push_back( go );
			}			

			// just play the selection sound on the first dude in the set
			if ( go->IsSelected() && !playedSelectSound )
			{
				if ( go->PlayVoiceSound( "select_char" ) != GPGSound::INVALID_SOUND_ID )
				{
					playedSelectSound = true;
				}
			}
		}
	}

	gUIPartyManager.EnsureSelection();

	if ( !gAppModule.GetControlKey() )
	{
		gUIPartyManager.SelectValidPortraits( gGoDb.GetSelection() );
	}

	return true;
}




/**************************************************************************

Function		: DrawSelectionIndicators()

Purpose			: Draws selection indicators around selected objects

Returns			: void

**************************************************************************/

#include "rapiprimitive.h"

void UIGame :: PrivateRenderGoSelection( Go* go, siege::database_guid& last_node, DWORD color, bool bFocus )
{
	// $$ note: must rotate selection object to align with the visual
	//          orientation if it's not a symmetric selection texture (i.e. it
	//          has an embedded arrow or something)

	Rapi& renderer = gSiegeEngine.Renderer();
	const SiegePos& position = go->GetPlacement()->GetPosition();

	if( last_node != position.node )
	{
		siege::SiegeNode* pNode	= gSiegeEngine.IsNodeValid( position.node );
		if( !pNode )
		{
			return;
		}

		// Save this node as active
		last_node = position.node;

		// Set the transform
		renderer.SetWorldMatrix( pNode->GetCurrentOrientation(), pNode->GetCurrentCenter() );
	}

	vector_3 offset	= bFocus ? vector_3( 0.0f, 0.01f, 0.0f ) : vector_3::ZERO;

	renderer.PushWorldMatrix();
	renderer.TranslateWorldMatrix( position.pos + offset );
	renderer.RotateWorldMatrix( go->GetPlacement()->GetOrientation().BuildMatrix() );

	if ( go->HasAspect() )
	{

		const nema::Aspect& asp = *(go->GetAspect()->GetAspectHandle());

		vector_3 v = asp.GetBoundingSphereCenter();

		if (go->HasBody())
		{
			const BoneTranslator& bt = go->GetBody()->GetBoneTranslator();

			int b0 = -1;
			int b1 = -1;
			int b2 = -1;

			if (   bt.Translate( BoneTranslator::BODY_ANTERIOR,  b0 )
			    && bt.Translate( BoneTranslator::BODY_MID,       b1 )
			    && bt.Translate( BoneTranslator::BODY_POSTERIOR, b2 ) 
				)
			{
				vector_3 vt(DoNotInitialize);
				asp.GetIndexedBonePosition(b0,v);
				asp.GetIndexedBonePosition(b1,vt);
				v += vt;
				asp.GetIndexedBonePosition(b2,vt);
				v += vt;
				v.x *= (1/3.0f);
				v.z *= (1/3.0f);
			}

		}
		
		v.y = 0;
			
		renderer.TranslateWorldMatrix( v );

		// update renderer
		float new_scale = go->GetAspect()->GetSelectionIndicatorScale();
		renderer.ScaleWorldMatrix( vector_3( new_scale, 1, new_scale ) );
	}

	m_SelectionVerts[0].color = m_SelectionVerts[1].color = m_SelectionVerts[2].color = color;
	if( bFocus )
	{
		renderer.DrawPrimitive( D3DPT_TRIANGLELIST, m_SelectionVerts, 3, SVERTEX, &m_SelectionTextureFocus, 1 );
	}
	else
	{
		renderer.DrawPrimitive( D3DPT_TRIANGLELIST, m_SelectionVerts, 3, SVERTEX, &m_SelectionTextureDefault, 1 );
	}

	renderer.PopWorldMatrix();
}

void UIGame :: DrawSelectionIndicators()
{
	// $ early bailout if no godb
	if ( !GoDb::DoesSingletonExist() )
	{
		return;
	}

	// what? no want?
	if ( !gWorldOptions.GetSelectionRings() )
	{
		return;
	}

	// early bailout if nothing to render
	if ( gGoDb.GetMouseShadowedGos().empty() && gGoDb.GetSelection().empty() )
	{
		return;
	}

	// yet another early bailout if world state not selection friendly
	if ( gWorldState.GetCurrentState() == WS_SP_NIS )
	{
		return;
	}

	// setup
	Rapi& renderer = gSiegeEngine.Renderer();
	renderer.SetTextureStageState(	0,	D3DTOP_MODULATE,
										D3DTOP_SELECTARG1,
										D3DTADDRESS_CLAMP,
										D3DTADDRESS_CLAMP,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DBLEND_SRCALPHA,
										D3DBLEND_INVSRCALPHA,
										true );

	bool bAlphaBlend = renderer.EnableAlphaBlending( true );

	renderer.GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE,	TRUE );
	renderer.GetDevice()->SetRenderState( D3DRS_CULLMODE,		D3DCULL_NONE );
	renderer.GetDevice()->SetRenderState( D3DRS_SHADEMODE,		D3DSHADE_FLAT );
	renderer.GetDevice()->SetRenderState( D3DRS_ALPHAREF,		0x00000000 );

	// cache
	siege::database_guid	last_node	= UNDEFINED_GUID;

	Goid focus_goid = gGoDb.GetFocusGo();

	// render rollover
	GopColl::const_iterator i, ibegin = gGoDb.GetMouseShadowedGosBegin(), iend = gGoDb.GetMouseShadowedGosEnd();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// skip duds
		Go* go = *i;
		if ( !go->HasAspect() || !go->GetAspect()->GetDrawSelectionIndicator() || !go->GetAspect()->GetIsVisible() ||
			 ( go->HasPhysics() && go->GetPhysics()->IsSimulating() ) || go->IsInsideInventory() || go->GetAspect()->GetForceNoRender() )
		{
			continue;
		}			

		gpassert( go->IsSelectable() || gSiegeEngine.GetOptions().SelectAllObjects() || gWorldOptions.GetShowAll() || gWorldOptions.GetShowGizmos() );	
		
		// inventory draggables

		// *** selection_indicators:item_color
		DWORD color = m_Item_selector_color;
		bool bFocus = false;

		if ( !go->HasGui() )
		{
			Go* party = gServer.GetScreenParty();
			gpassert( party != NULL );

			// friendlies
			if ( go->GetParent() == party )
			{
				if( focus_goid == go->GetGoid() )
				{
					// *** selection_indicators:focus_go_color
					color = m_Focus_go_selector_color;
					bFocus = true;
				}
				else
				{
					// *** selection_indicators:party_member_color
					color = m_Party_member_selector_color;
					bFocus = false;
				}
			}
			else
			{
				if ( gAIQuery.Is( NULL, go, QT_EVIL ) )
				{
					// enemies
					if ( IsAlive( go->GetAspect()->GetLifeState() ) )
					{
						// *** selection_indicators:enemy_color
						color = m_Enemy_selector_color;
					}
					else
					{
						// *** selection_indicators:dead_enemy_color
						color = m_Dead_enemy_selector_color;
					}
				}
				else
				{
					if ( IsAlive( go->GetAspect()->GetLifeState() ) )
					{
						// *** selection_indicators:friend_color
						color = m_Friend_selector_color;
					}
					else
					{
						// *** selection_indicators:dead_friend_color
						color = m_Dead_friend_selector_color;
					}
				}
			}
		}

		// render
		PrivateRenderGoSelection( go, last_node, color, bFocus );
	}

	// render selected
	GopColl::const_iterator j, jbegin = gGoDb.GetSelectionBegin(), jend = gGoDb.GetSelectionEnd();
	for ( j = jbegin ; j != jend ; ++j )
	{
		// only render if not a rollover
		if ( !(*j)->IsMouseShadowed() )
		{
			if( focus_goid == (*j)->GetGoid() )
			{
				PrivateRenderGoSelection( *j, last_node, m_Focus_go_selector_color, true );
			}
			else
			{
				PrivateRenderGoSelection( *j, last_node, m_Party_member_selector_color, false );
			}
		}
	}

	renderer.EnableAlphaBlending( bAlphaBlend );

	// restore renderer
	renderer.GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE,	TRUE );
	renderer.GetDevice()->SetRenderState( D3DRS_CULLMODE,		D3DCULL_CW );
	renderer.GetDevice()->SetRenderState( D3DRS_SHADEMODE,		D3DSHADE_GOURAUD );
}


/**************************************************************************

Function		: DrawAreaOfEffectVolume()

Purpose			: Draws area of effect volume of weapon/spell go

Returns			: void

**************************************************************************/
void UIGame :: DrawAreaOfEffectVolume( Goid weapon, Goid target )
{
	// Reset the volume parameters if this is a new weapon volume
	if( m_AreaOfEffectTarget != target ) {
		// Reset the script callback
		gWorldFx.ClearScriptCallback( m_AreaOfEffectScriptID );
		gWorldFx.StopScript( m_AreaOfEffectScriptID );
		m_AreaOfEffectTarget = GOID_INVALID;
		m_AreaOfEffectScriptID = 0;
	}

	if( m_AreaOfEffectScriptID == 0 ) {
		GoHandle hTarget( target );

		if( !IsAlive( hTarget->GetLifeState() ) ) {
			return;
		}

		GoHandle hWeapon( weapon );

		if( hWeapon.IsValid() && hTarget.IsValid() ) {

			SiegePos volume_origin( hTarget->GetPlacement()->GetPosition() );
			hTarget->GetAspect()->GetNodeSpaceCenter( volume_origin.pos );

			// $$$ Note! For a projectile weapon such as a bow that shoots projectiles
			// that have area of effect damage that is where the damage radius should
			// be extracted and not from the bow itself - it is too slow to look up
			// the value from the Scid every frame so just make the bow and the arrow
			// area of effect damage radius coincide. This will be fixed with NWO
			const float damage_radius = hWeapon->GetAttack()->GetAreaDamageRadius();

			// Create the target string
			def_tracker tracker = WorldFx::MakeTracker();
			tracker->AddStaticTarget( volume_origin, vector_3::ZERO, 1.0f );

			// Create the param string
			gpstring sParams;
			sParams.assignf( "[scale(%g)]", damage_radius );

			m_AreaOfEffectTarget = target;
			m_AreaOfEffectScriptID = gWorldFx.RunScript( "aoe_volume", tracker, sParams, GOID_INVALID, WE_INVALID );

			gWorldFx.SetScriptCallback( m_AreaOfEffectScriptID, makeFunctor( *this, &UIGame::AreaOfEffectScriptCallback ) );
		}
	}
}


/**************************************************************************

Function		: AreaOfEffectScriptCallback()

Purpose			: Lets the UI know when the area of effect script volume
				: has finished

Returns			: void

**************************************************************************/
void UIGame :: AreaOfEffectScriptCallback( SFxSID id )
{
	UNREFERENCED_PARAMETER( id );

	m_AreaOfEffectScriptID = 0;
	m_AreaOfEffectTarget = GOID_INVALID;
}


/**************************************************************************

Function		: Selection_Clear()

Purpose			: Clear all selected objects

Returns			: void

**************************************************************************/
void UIGame :: Selection_Clear()
{
	gGoDb.DeselectAll();
}



/**************************************************************************

Function		: Selection_Delete()

Purpose			: Destroy all selected objects

Returns			: bool

**************************************************************************/
bool UIGame :: Selection_Delete()
{
	GopColl::const_iterator i;
	for( i = gGoDb.GetSelection().begin(); i != gGoDb.GetSelection().end(); ++i )
	{
		gGoDb.RSMarkGoAndChildrenForDeletion( (*i)->GetGoid(), true, true );
	}
	return true;
}



/**************************************************************************

Function		: Selection_Copy()

Purpose			: Copy selected objects

Returns			: bool

**************************************************************************/
bool UIGame :: Selection_Copy()
{
	m_Clipboard.clear();
	GopColl::const_iterator i;
	unsigned int count=0;
	for( i = gGoDb.GetSelection().begin(); i != gGoDb.GetSelection().end(); ++i ) {
		m_Clipboard.push_back( (*i)->GetGoid() );
		++count;
	}
	gpgenericf(( "copied %d objects to debug clipboard.\n", count ));
	return true;
}



/**************************************************************************

Function		: Selection_Paste()

Purpose			: Paste selected objects

Returns			: bool

**************************************************************************/
bool UIGame :: Selection_Paste()
{
	GoidColl::const_iterator i, begin = m_Clipboard.begin(), end = m_Clipboard.end();
	for ( i = begin ; i != end ; /*++i $$$fill in later*/ )
	{
		GoCloneReq cloneReq( *i );
		cloneReq.SetStartingPos( gSiegeEngine.GetMouseShadow().GetFloorHitPos() );
		gGoDb.RSCloneGo( cloneReq );

		// $$$ to do: modify this function so that it remembers the relative
		//     offsets of the copied objects from a base point and then adjust
		//     the positions and orientations of the pasted versions of those
		//     objects by those offsets.
		break;
	}

	gpgenericf(( "pasted %d objects from clipboard.\n", m_Clipboard.size() ));

	return true;
}


// Get the Goid of the current object under the cursor ( if there is one )
Goid UIGame :: GetGoUnderCursor() const
{
	Goid temp = gUIGame.GetUIItemOverlay()->GetHoverItem();
	if ( temp != GOID_INVALID )
	{
		return temp;;
	}

	GopColl::const_iterator i, ibegin = gGoDb.GetMouseShadowedGosBegin(), iend = gGoDb.GetMouseShadowedGosEnd();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( !(*i)->IsInsideInventory() )
		{
			return ( (*i)->GetGoid() );
		}
	}

	return ( GOID_INVALID );
}


/**************************************************************************

Function		: Selection_SpeedUp()

Purpose			: Steps down the speedbias on all selected characters

Returns			: bool

**************************************************************************/
bool UIGame :: Selection_SpeedUp()
{
	// $$$ nwo
	/*
	GODB::iterator i;
	for( i = gGODB.begin(); i != gGODB.end(); ++i ) {

		if (!(*i)->HasActor()) continue;
		if (!(*i)->IsSelected()) continue;

		float sb = (*i)->GetStats()->GetSpellSpeedBias() + 0.1f;
		sb = min(5.0f,sb);
		(*i)->GetStats()->SetSpellSpeedBias(sb);
	}
	*/

	return true;
}


/**************************************************************************

Function		: Selection_SpeedDown()

Purpose			: Steps down the speedbias on all selected characters

Returns			: bool

**************************************************************************/
bool UIGame :: Selection_SpeedDown()
{
	// $$$ nwo
	/*
	GODB::iterator i;
	for( i = gGODB.begin(); i != gGODB.end(); ++i ) {

		if (!(*i)->HasActor()) continue;
		if (!(*i)->IsSelected()) continue;

		float sb = (*i)->GetStats()->GetSpellSpeedBias() - 0.1f;
		sb = max(0.1f,sb);
		(*i)->GetStats()->SetSpellSpeedBias(sb);
	}
	*/

	return true;
}



/**************************************************************************

Function		: Selection_SpeedReset()

Purpose			: Reset the speedbias on all selected characters

Returns			: bool

**************************************************************************/
bool UIGame :: Selection_SpeedReset()
{
	// $$$ nwo
	/*
	GODB::iterator i;
	for( i = gGODB.begin(); i != gGODB.end(); ++i ) {

		if (!(*i)->HasActor()) continue;
		if (!(*i)->IsSelected()) continue;

		(*i)->GetStats()->SetSpellSpeedBias((*i)->GetStats()->GetInitialSpeedBias());
	}
	*/
	return true;
}



/**************************************************************************

Function		: GetActorWhoCarriesObject()

Purpose			: Get the Goid of the current object under the cursor
				  ( if there is one )

Returns			: bool

**************************************************************************/
Goid UIGame :: GetActorWhoCarriesObject( Goid object )
{
	GoHandle hObject( object );
	if ( hObject.IsValid() ) {
		Go * parent = 0;
		parent = hObject->GetParent();
		if ( parent )
		{
			if ( parent->HasActor() ) {
				return parent->GetGoid();
			}
		}
	}
	return GOID_INVALID;
}


/**************************************************************************

Function		: Contains()

Purpose			: See if a game object exits within a specified GoidColl

Returns			: bool

**************************************************************************/
bool UIGame :: Contains( Goid go, GoidColl objects )
{
	GoidColl::iterator i;
	for( i = objects.begin(); i != objects.end(); ++i )
	{
		if( *i == go ) {
			return( true );
		}
	}
	return( false );
}
bool UIGame :: Contains( Goid go, GopColl objects )
{
	GopColl::iterator i;
	for( i = objects.begin(); i != objects.end(); ++i )
	{
		if( (*i)->GetGoid() == go ) {
			return( true );
		}
	}
	return( false );
}
bool UIGame :: Contains( Goid go, GopSet objects )
{
	GopSet::iterator i;
	for( i = objects.begin(); i != objects.end(); ++i )
	{
		if( (*i)->GetGoid() == go ) {
			return( true );
		}
	}
	return( false );
}



/**************************************************************************

Function		: SetSelectionBox()

Purpose			: Turns the selection box drawing off or on.

Returns			: void

**************************************************************************/
void UIGame :: SetSelectionBox( bool flag )
{
	if ( !gUIPartyManager.IsInDataDockBar() || flag == false )
	{
		if( flag )
		{
			// Update the mouse shadow
			gSiegeEngine.GetMouseShadow().SetBeginPosition( (float)-gAppModule.GetNormalizedCursorX(),
															(float) gAppModule.GetNormalizedCursorY() );
		}
		gSiegeEngine.GetMouseShadow().SetSelectionBox( flag );
	}
}




/**************************************************************************

Function		: Draw()

Purpose			: Draws the GameGUI

Returns			: void

**************************************************************************/
void UIGame :: Draw( double seconds_elapsed )
{
	// Failsafe - If we're disconnected, why are we drawing?
	if ( gWorldState.GetCurrentState() == WS_MP_SESSION_LOST )
	{
		return;
	}

	// the ::Draw method is a pure virtual, so it we have the seconds_elapsed variable, even
	// though we do not use it.
	UNREFERENCED_PARAMETER( seconds_elapsed );

	//GPPROFILERSAMPLE( "UIGame :: Draw() - GameGUI Draw()" );

	m_pUIItemOverlay->Draw();

	gSiegeEngine.Renderer().GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );

	if ( ::IsMultiPlayer() )
	{
		if ( m_bPlayerLabelsVisible )
		{
			DrawMultiplayerLabels();
		}
		gUIPartyManager.DrawTeamStatusBars();		
	}
	else if ( ::IsSinglePlayer() )
	{
		gUIPartyManager.DrawMemberLabels();
	}

	gUIShell.Draw();	

	gSiegeEngine.Renderer().GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
	
	gUIMenuManager.DrawSave();

	if ( !gUIOptions.IsActive() &&
		 !gUIMenuManager.IsLoadSaveActive() && 
		 !gUIMenuManager.GetGoldTradeDialogActive() && 
		 !gUIMenuManager.GetGoldInvalidDialogActive() &&
		 !gUIMenuManager.GetExitDialogActive() &&
		 !gUIMenuManager.IsOptionsMenuActive() )
	{
		GetUIPartyManager()->Draw();
	}
	
/*
	if( m_BackendMode == BMODE_ASSIGN_FORMATION_SPOT )
	{
		GoHandle hDragged( GetDragged() );

		if( hDragged.IsValid() )
		{
			if( gSiegeEngine.GetMouseShadow().IsTerrainHit() )
			{
// $$$$ make this call a non-dev function!! -sb
#if !GP_RETAIL
				gWorld.DrawDebugLinkedTerrainPoints(	hDragged->GetPlacement()->GetPosition(),
														gSiegeEngine.GetMouseShadow().GetFloorHitPos(),
														0xff808000,
														"assign formation spot" );
#endif // !GP_RETAIL

				Formation * formation;

				if( gServer.GetScreenFormation( formation ) )
				{
					int w, l;
					formation->Intersects( gSiegeEngine.GetMouseShadow().GetFloorHitPos(), w, l );
				}
			}
		}
	}
*/
}




/**************************************************************************

Function		: RequestQuit()

Purpose			: Puts in a request to exit the backend

Returns			: void

**************************************************************************/
bool UIGame :: RequestQuit()
{
	if ( IsInGame( gWorldState.GetCurrentState() ) && gWorldState.GetCurrentState() != WS_SP_NIS )
	{
		gUIMenuManager.ExitWindowsDialog();		
	}

	return false;
}



/**************************************************************************

Function		: SetAsActive()

Purpose			: Sets the visibility and activates/initializes all gui
				  components

Returns			: void

**************************************************************************/
void UIGame :: SetAsActive( bool flag )
{

	if ( flag == true ) 
	{
		RatioSample ratioSample( "", (::IsMultiPlayer() ? 43 : 39) );

		m_DefeatFadeOut = DEFEAT_FADE_OUT_TIME;
		m_pDefeatBg = 0;
		m_bFadedOut = false;
		gUIMenuManager.SetIsDefeated( false );
		gUIShell.ClearActiveItems();

		if ( m_bActive == true )
		{
			m_bActive = flag;
			return;
		}

		gSiegeEngine.GetCompass().SetActive( true );
		SetIsVisible( true );

#if !GP_RETAIL
		m_pPolyText = NULL;
#endif
		// Activate Interfaces ( Warning: Order Dependent )
		gUIShell.ActivateInterface( "ui:interfaces:cursors",								true	);		ratioSample.Advance( "cursors" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:mini_map",						false	);		ratioSample.Advance( "mini_map" );
		gUIShell.DeactivateInterfaceWindows( "mini_map" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:status_bars",					true	);		ratioSample.Advance( "status_bars" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:member_labels",					true	);		ratioSample.Advance( "member_labels" );

		Delete( m_pUIItemOverlay );
		m_pUIItemOverlay = new UIItemOverlay;

		gUIShell.ActivateInterface( "ui:interfaces:backend:data_bar",						true	);		ratioSample.Advance( "data_bar" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:character_awp",					true	);		ratioSample.Advance( "character_awp" );
		
		gUITeamManager.Init();

		gUIShell.ActivateInterface( "ui:interfaces:backend:field_commands",					true	);		ratioSample.Advance( "field_commands" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:multi_inventory",				true	);		ratioSample.Advance( "multi_inventory" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:inventory",						true	);		ratioSample.Advance( "inventory" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:store",							false	);		ratioSample.Advance( "store" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:party_stash",					false	);		ratioSample.Advance( "party_stash" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:spell",							false	);		ratioSample.Advance( "spell" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:character",						false	);		ratioSample.Advance( "character" );		
		gUIShell.ActivateInterface( "ui:interfaces:backend:lore_book",						false	);		ratioSample.Advance( "lore_book" );		
		
		if ( ::IsMultiPlayer() )
		{
			gUIShell.ActivateInterface( "ui:interfaces:backend:multiplayer_trade",			false	);		ratioSample.Advance( "multiplayer_trade" );
		}

#if !GP_RETAIL
		gUIShell.ActivateInterface( "ui:interfaces:backend:debug",							true	);		// $ leave this off so don't have to adjust total at top
#endif
		gUIShell.ActivateInterface( "ui:interfaces:backend:rollover_help",					true	);		ratioSample.Advance( "rollover_help" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:gold_transfer",					false	);		ratioSample.Advance( "gold_transfer" );
		
		gUIShell.ActivateInterface( "ui:interfaces:backend:jip_glass",						false	);		ratioSample.Advance( "jip_glass" );
		
		if ( ::IsMultiPlayer() )
		{
			gUIShell.ActivateInterface( "ui:interfaces:backend:game_console_mp",			true	);		ratioSample.Advance( "game_console_mp" );
		}
		else
		{
			gUIShell.ActivateInterface( "ui:interfaces:backend:game_console",				true	);		ratioSample.Advance( "game_console" );
		}

		gUIShell.ShowGroup( "console_multiplayer", false );
		gUIShell.ActivateInterface( "ui:interfaces:backend:console_output",					true	);		ratioSample.Advance( "console_output" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:dialogue_box",					false	);		ratioSample.Advance( "dialogue_box" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:hire_stats",						false	);		ratioSample.Advance( "hire_stats" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:nis_subtitle",					false	);		ratioSample.Advance( "nis_subtitle" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:multiplayer_respawn",			false	);		ratioSample.Advance( "multiplayer_respawn" );

		gUIShell.DeactivateInterface( "icons" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:icons",							true	);		ratioSample.Advance( "icons" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:compass_hotpoints",				true	);		ratioSample.Advance( "compass_hotpoints" );
				
		if ( ::IsMultiPlayer() )
		{			
			gUIShell.ActivateInterface( "ui:interfaces:multiplayer:join_in_progress",			false	);		ratioSample.Advance( "join_in_progress" );
			gUIShell.ActivateInterface( "ui:interfaces:multiplayer:in_game_host_options",		false	);		ratioSample.Advance( "in_game_host_options" );
			gUIShell.ActivateInterface( "ui:interfaces:multiplayer:in_game_host_options_kick",	false	);		ratioSample.Advance( "in_game_host_options_kick" );
			gUIShell.ActivateInterface( "ui:interfaces:multiplayer:in_game_menu",				false	);		ratioSample.Advance( "in_game_menu" );
			if ( gServer.IsLocal() )
			{
				gUIShell.ShowGroup( "server_menu", true,  false, "in_game_menu" );
				gUIShell.ShowGroup( "client_menu", false, false, "in_game_menu" );
			}
			else
			{
				gUIShell.ShowGroup( "client_menu", true,  false, "in_game_menu" );
				gUIShell.ShowGroup( "server_menu", false, false, "in_game_menu" );
			}
		}
		else
		{
			gUIShell.ActivateInterface( "ui:interfaces:backend:in_game_menu",				false	);		ratioSample.Advance( "in_game_menu" );
		}
		
		gUIShell.ActivateInterface( "ui:interfaces:backend:defeat_menu",					false	);		ratioSample.Advance( "defeat_menu" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:loadsave_game",					false	);		ratioSample.Advance( "loadsave_game" );

		gUIShell.ActivateInterface( "ui:interfaces:backend:multiplayer_timer",				false	);		ratioSample.Advance( "multiplayer_timer" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:pause",							true	);		ratioSample.Advance( "pause" );

		// Manually set the draw order of Options on top.
		gUIShell.DeactivateInterface( "options_game"     );
		gUIShell.DeactivateInterface( "options_video"    );
		gUIShell.DeactivateInterface( "options_audio"    );
		gUIShell.DeactivateInterface( "options_input"    );
		gUIShell.DeactivateInterface( "options_bindings" );
		gUIShell.DeactivateInterface( "options_item_filters" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:options_game",     false );		ratioSample.Advance( "options_game" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:options_video",    false );		ratioSample.Advance( "options_video" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:options_audio",    false );		ratioSample.Advance( "options_audio" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:options_input",    false );		ratioSample.Advance( "options_input" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:options_bindings", false );		ratioSample.Advance( "options_bindings" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:options_item_filters", false );	ratioSample.Advance( "options_item_filters" );
		
		// Activate topmost dialogs		
		gUIShell.ActivateInterface( "ui:interfaces:backend:defeat_dialog",	  false	);		ratioSample.Advance( "defeat_dialog" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:world_tip",	      false	);		ratioSample.Advance( "world_tip" );
		gUIShell.ActivateInterface( "ui:interfaces:backend:backend_dialog",	  false	);		ratioSample.Advance( "backend_dialog" );

		// Activate tooltips
		gUIShell.DeactivateInterface( "tool_tip" );
		gUIShell.ActivateInterface( "ui:interfaces:frontend:tool_tip", true );		ratioSample.Advance( "tool_tip" );

		gUIShell.ShowGroup( "skills",				false );
		gUIShell.ShowGroup( "paused",				false );		
		gUIShell.ShowGroup( "potential_member",		false );
		gUIShell.ShowGroup( "shop",					false );
		gUIShell.ShowGroup( "pack_mule_inventory",	false );
		gUIShell.ShowGroup( "human_inventory",		false );		

		// Rollover help windows
		m_RolloverObjData.pText = (UITextBox *)gUIShell.FindUIWindow( "gui_rollover_textbox" );

		gpstring sBar;
		gpstring sMagic;
		for ( int i = 1; i <= MAX_PARTY_MEMBERS; ++i )
		{
			sBar.assignf( "multi_inventory_human_%d", i );
			gUIShell.ShowGroup( sBar, false );
			sBar.assignf( "multi_inventory_packmule_%d", i );
			gUIShell.ShowGroup( sBar, false );
			sMagic.assignf( "spell_slot_%d", i );
			gUIShell.ShowGroup( sMagic, false );
		}
		gUIShell.ShowGroup( "inventory_header", false );
		gUIShell.ShowGroup( "store_inventory_bar", false );

		gUIShell.GetMessenger()->RegisterCallback( makeFunctor( *this, &UIGame :: GameGUICallback ) );
		m_BackendMode = BMODE_NONE;
		gUIPartyManager.Init();
		gUIInventoryManager.Init();

		// Register Selection handlers with world
		if ( GoDb::DoesSingletonExist() )
		{
			gGoDb.SetSelectionChangedCb( makeFunctor( *this, &UIGame::SelectionChanged ) );
		}

		if ( ::IsMultiPlayer() )
		{
			m_pUIPlayerRanks->Init();
			m_pUIEmoteList->Init();

			// default timer to visible if there's a time limit
			if ( gVictory.GetTimeLimit() > 0.0f )
			{
				m_bGameTimerVisible = true;
				gUIShell.ShowInterface( "multiplayer_timer" );
			}
		}

		GetUICommands()->InitCursors();
		gUIMenuManager.ApplyConsoleOptions();
		gUIMenuManager.Init();
		gUIStoreManager.Init();

		m_bOneShotCharacterInit = false;
		if ( !m_bOneShotCharacterInit )
		{
			// Disable GUI features for parties less than max
			for ( int j = 0; j < MAX_PARTY_MEMBERS; ++j )
			{
				gpstring sCharacter;
				sCharacter.assignf( "character_%d", j+1 );
				gUIShell.ShowGroup( sCharacter, false );
			}
			m_bOneShotCharacterInit = true;
		}

		if ( gWorldState.GetCurrentState() != WS_SP_DEFEAT )
		{
			m_DefeatFadeOut = DEFEAT_FADE_OUT_TIME;
		}				
	}
	else
	{
		m_progressNodeCount = 0;

		if ( m_bActive == false )
		{
			m_bActive = flag;
			return;
		}

		gUITeamManager.DeInit();
		gUIMenuManager.DeInit();

		gUIShell.HideInterface( "backend_dialog"      );
		gUIShell.HideInterface( "character"           );
		gUIShell.HideInterface( "character_awp"       );
		gUIShell.HideInterface( "compass_hotpoints"   );
		gUIShell.HideInterface( "console_output"      );
		gUIShell.HideInterface( "data_bar"            );
		gUIShell.HideInterface( "debug"               );
		gUIShell.HideInterface( "defeat_dialog"       );
		gUIShell.HideInterface( "defeat_menu"         );
		gUIShell.HideInterface( "dialogue_box"        );
		gUIShell.HideInterface( "hire_stats"		  );
		gUIShell.HideInterface( "field_commands"      );
		gUIShell.HideInterface( "game_console"        );		
		gUIShell.HideInterface( "game_console_mp"     );		
		gUIShell.HideInterface( "icons"               );
		gUIShell.HideInterface( "in_game_menu"        );
		gUIShell.HideInterface( "inventory"           );
		gUIShell.HideInterface( "inventory_bar"       );
		gUIShell.HideInterface( "loadsave_game"       );
		gUIShell.HideInterface( "lore_book"           );
		gUIShell.HideInterface( "multiplayer_respawn" );
		gUIShell.HideInterface( "multiplayer_timer"   );
		gUIShell.HideInterface( "multiplayer_trade"   );
		gUIShell.HideInterface( "nis_subtitle"        );
		gUIShell.HideInterface( "pause"               );		
		gUIShell.HideInterface( "rollover_help"       );
		gUIShell.HideInterface( "spell"               );
		gUIShell.HideInterface( "status_bars"         );
		gUIShell.HideInterface( "store"               );
		gUIShell.HideInterface( "member_labels"		  );
		gUIShell.HideInterface( "jip_glass"			  );
		gUIShell.HideInterface( "join_in_progress"	  );	
		gUIShell.HideInterface( "in_game_host_options" );
		gUIShell.HideInterface( "in_game_host_options_kick" );	
		gUIShell.HideInterface( "world_tip" );
		gUIShell.HideInterface( "journal" );
		gUIShell.HideInterface( "party_stash" );

		if ( gUIOptions.IsActive() )
		{
			gUIOptions.Deactivate();			
		}	

		gUIMenuManager.SetGoldTradeDialogActive( false );

		m_pUIItemOverlay->SetVisible( false );

		m_pUIPlayerRanks->SetVisible( false );

		if( ::IsMultiPlayer() )
		{
			m_pUIEmoteList->SetVisible( false );
		}

		gSiegeEngine.GetCompass().SetActive( false );

		gUIMenuManager.SetExitDialogActive( false );

		gUIShell.GetMessenger()->UnRegisterCallback( makeFunctor( *this, &UIGame :: GameGUICallback ) );
		m_BackendMode = BMODE_NONE;

		GetUICommands()->SetCursor( CURSOR_GUI_STANDARD );

		if ( GoDb::DoesSingletonExist() )
		{
			gGoDb.SetSelectionChangedCb( NULL );
		}
	}

	m_bActive = flag;
}


void UIGame :: Deinit()
{
	// Deactivate Interfaces
	gUIShell.DeactivateInterface( "character" );
	gUIShell.DeactivateInterface( "inventory" );
	gUIShell.DeactivateInterface( "multi_inventory" );		
	gUIShell.DeactivateInterface( "multiplayer_trade" );
	gUIShell.DeactivateInterface( "character_awp" );
	gUIShell.DeactivateInterface( "icons" );
	gUIShell.DeactivateInterface( "status_bars" );
	gUIShell.DeactivateInterface( "data_bar" );	
	gUIShell.DeactivateInterface( "spell" );
	gUIShell.DeactivateInterface( "field_commands" );
	gUIShell.DeactivateInterface( "journal" );
	gUIShell.DeactivateInterface( "inventory_bar" );	
	gUIShell.DeactivateInterface( "store" );			
	gUIShell.DeactivateInterface( "debug" );
	gUIShell.DeactivateInterface( "rollover_help" );
	gUIShell.DeactivateInterface( "gold_transfer" );
	gUIShell.DeactivateInterface( "game_console" );		
	gUIShell.DeactivateInterface( "game_console_mp" );			
	gUIShell.DeactivateInterface( "dialogue_box" );
	gUIShell.DeactivateInterface( "hire_stats" );
	gUIShell.DeactivateInterface( "nis_subtitle" );
	gUIShell.DeactivateInterface( "compass_hotpoints" );
	gUIShell.DeactivateInterface( "mini_map" );
	gUIShell.DeactivateInterface( "defeat_menu"	);
	gUIShell.DeactivateInterface( "loadsave_game" );
	gUIShell.DeactivateInterface( "in_game_menu" );
	gUIShell.DeactivateInterface( "backend_dialog" );
	gUIShell.DeactivateInterface( "multiplayer_timer" );
	gUIShell.DeactivateInterface( "multiplayer_respawn" );	
	gUIShell.DeactivateInterface( "pause" );
	gUIShell.DeactivateInterface( "lore_book" );	
	gUIShell.DeactivateInterface( "console_output" );	
	gUIShell.DeactivateInterface( "defeat_dialog" );
	gUIShell.DeactivateInterface( "join_in_progress" );
	gUIShell.DeactivateInterface( "in_game_host_options" );		
	gUIShell.DeactivateInterface( "in_game_host_options_kick" );
	gUIShell.DeactivateInterface( "member_labels" );
	gUIShell.DeactivateInterface( "jip_glass" );
	gUIShell.DeactivateInterface( "world_tip" );
	gUIShell.DeactivateInterface( "party_stash" );
	gUIShell.DeactivateInterface( "emote_list" );
	
	m_pUIPlayerRanks->Deactivate();

	m_PlayerLabels.clear();

	m_bOneShotCharacterInit = false;
#if !GP_RETAIL
	m_pPolyText = NULL;
#endif

	gUIPartyManager.Deinit();	
}


/**************************************************************************

Function		: SetInfoMode()

Purpose			: Sets the way information is read from objects in the
				  information box.  Used by the console.

Returns			: void

**************************************************************************/
void UIGame :: SetInfoMode( gpstring mode )
{
	if ( ( mode.same_no_case( "block_name" ) ) ||
		 ( mode.same_no_case( "screen_name" ) ) ||
		 ( mode.same_no_case( "dev_name" ) ) ) {
		m_sInfoMode = mode;
	}
}



#if !GP_RETAIL

// Display a specified cursor; For console use ONLY!
void UIGame :: DisplayCursor( gpstring filename )
{
	GUInterface *pInterface = gUIShell.FindInterface( "cursors" );
	FastFuelHandle fhCursor( "UI:Interfaces:Cursors:cursors:cursor_template" );
	UIWindow *pWindow = gUIShell.Instantiate( fhCursor, pInterface );
	gpstring sTexture = "art\\bitmaps\\gui\\cursors\\" + filename;
	UICursor *pCursor = (UICursor*)pWindow;
	pCursor->LoadAnimatedTexture( sTexture, true );
	pCursor->SetName( filename );
	if ( pCursor )
	{
		pCursor->Activate();
	}
}

#endif // !GP_RETAIL


void UIGame :: DrawMultiplayerLabels()
{
	Player *localPlayer = gServer.GetLocalHumanPlayer();

	GoHandle hHero( localPlayer->GetHero() );
	if ( !hHero.IsValid() )
	{
		return;
	}

	siege::SiegeFrustum * pFrustum = gSiegeEngine.GetFrustum( (DWORD)gGoDb.GetGoFrustum( hHero->GetGoid() ) );
	if ( !pFrustum )
	{
		return;
	}

	for ( Server::PlayerColl::const_iterator iPlayer = gServer.GetPlayers().begin(); iPlayer != gServer.GetPlayers().end(); ++iPlayer )
	{
		if ( !IsValid( *iPlayer ) || (*iPlayer)->IsComputerPlayer() ) //|| (*iPlayer == localPlayer) )
		{
			continue;
		}		

		Go * pParty = (*iPlayer)->GetParty();
		if ( pParty )
		{
	 		GopColl partyColl = pParty->GetChildren();
			for ( GopColl::iterator i = partyColl.begin(); i != partyColl.end(); ++i )
			{
				if ( !(*i)->HasPlacement() || !(*i)->HasAspect() )
				{
					continue;
				}

				bool drawLabel = false;

				SiegeNode * pNode = gSiegeEngine.IsNodeValid( (*i)->GetPlacement()->GetPosition().node );
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
					if ( (*i)->GetAspect()->GetIsScreenPlayerVisible() || localPlayer->GetIsFriend( *iPlayer ) )
					{
						drawLabel = true;
					}
				}
				else
				{
					vector_3 worldPos = (*i)->GetPlacement()->GetPosition().WorldPos();
					if ( !gSiegeEngine.GetCamera().GetFrustum().CullSphere( worldPos, 1.0f ) )
					{
						drawLabel = true;
					}			
				}

				if ( drawLabel )
				{
					PlayerLabelMap::iterator findLabel = m_PlayerLabels.find( (*i)->GetGoid() );

					if ( findLabel == m_PlayerLabels.end() )
					{
						PlayerLabel newLabel;

						newLabel.m_LifeState = (*i)->GetLifeState();

						matrix_3x3 orient;
						vector_3 center, halfDiag;
						(*i)->GetAspect()->GetNodeSpaceOrientedBoundingVolume( orient, center, halfDiag );
						halfDiag = orient * halfDiag;
						newLabel.m_HeightOffset = (center.y - (*i)->GetPlacement()->GetPosition().pos.y) + halfDiag.y + 0.4f;

						std::pair< PlayerLabelMap::iterator, bool > result = m_PlayerLabels.insert( PlayerLabelMap::value_type( (*i)->GetGoid(), newLabel ) );
						gpassert( result.second );
						findLabel = result.first;
					}

					SiegePos labelPos( (*i)->GetPlacement()->GetPosition() );
					labelPos.pos.y += findLabel->second.m_HeightOffset;
					vector_3 screenPos = labelPos.ScreenPos();
					
					int screenX = (int)(screenPos.x + 0.5f);
					int screenY = (int)(screenPos.y + 0.5f);

					if ( localPlayer == *iPlayer )
					{
						m_pPlayerLabelText->SetColor( gVictory.GetGameType()->GetFriendColor() );
					}
					else if ( localPlayer->GetTeam() == (*iPlayer)->GetTeam() )
					{
						m_pPlayerLabelText->SetColor( gVictory.GetGameType()->GetFriendColor() );
					}
					else // ( localPlayer->GetIsEnemy( *iPlayer ) )
					{
						m_pPlayerLabelText->SetColor( gVictory.GetGameType()->GetEnemyColor() );
					}

					if ( GetResizeLabels() )
					{
						float dist = Length( gSiegeEngine.GetCamera().GetCameraPosition() - labelPos.WorldPos() );
						if ( dist < 8 )
						{
							m_pPlayerLabelText->SetFont( "b_gui_fnt_16p_copperplate-light" );
						}
						else if ( dist < 20 )
						{
							m_pPlayerLabelText->SetFont( "b_gui_fnt_14p_copperplate-light" );
						}
						else
						{
							m_pPlayerLabelText->SetFont( "b_gui_fnt_12p_copperplate-light" );
						}
					}
					else
					{
						m_pPlayerLabelText->SetFont( "b_gui_fnt_12p_copperplate-light" );
					}
					
					m_pPlayerLabelText->SetText( (*i)->GetCommon()->GetScreenName(), true );

					m_pPlayerLabelText->SetRect( screenX - (m_pPlayerLabelText->GetRect().Width() / 2), screenX + (m_pPlayerLabelText->GetRect().Width() / 2),
												 screenY - m_pPlayerLabelText->GetRect().Height(), screenY );
					m_pPlayerLabelText->Draw();

					Team * team = gServer.GetTeam( (*iPlayer)->GetId() );
					if ( team && (team->m_TextureId > 0) )
					{
						int textureSize = (int)(1.5f * m_pPlayerLabelText->GetRect().Height());

						unsigned int textureId = team->m_TextureId;

						gUIShell.GetRenderer().SetTextureStageState( 0,
													D3DTOP_MODULATE,
													D3DTOP_MODULATE,
													D3DTADDRESS_CLAMP,
													D3DTADDRESS_CLAMP,
													D3DTEXF_POINT,
													D3DTEXF_POINT,
													D3DTEXF_POINT,
													D3DBLEND_SRCALPHA,
													D3DBLEND_INVSRCALPHA,
													true );

						tVertex Verts[4];
						memset( Verts, 0, sizeof( tVertex ) * 4 );

						Verts[0].x			= (float)(screenX - textureSize / 2) + 0.5f;
						Verts[0].y			= (float)screenY - m_pPlayerLabelText->GetRect().Height() - 4 - textureSize + 0.5f;
						Verts[0].z			= 0.0f;
						Verts[0].rhw		= 1.0f;
						Verts[0].uv.u		= 0.0f;
						Verts[0].uv.v		= 1.0f;
						Verts[0].color		= 0xFFFFFFFF;

						Verts[1].x			= Verts[0].x;
						Verts[1].y			= (float)screenY - m_pPlayerLabelText->GetRect().Height() - 4 + 0.5f;
						Verts[1].z			= 0.0f;
						Verts[1].rhw		= 1.0f;
						Verts[1].uv.u		= 0.0f;
						Verts[1].uv.v		= 0.0f;
						Verts[1].color		= 0xFFFFFFFF;

						Verts[2].x			= Verts[0].x + textureSize;
						Verts[2].y			= Verts[0].y;
						Verts[2].z			= 0.0f;
						Verts[2].rhw		= 1.0f;
						Verts[2].uv.u		= 1.0f;
						Verts[2].uv.v		= 1.0f;
						Verts[2].color		= 0xFFFFFFFF;

						Verts[3].x			= Verts[2].x;
						Verts[3].y			= Verts[1].y;
						Verts[3].z			= 0.0f;
						Verts[3].rhw		= 1.0f;
						Verts[3].uv.u		= 1.0f;
						Verts[3].uv.v		= 0.0f;
						Verts[3].color		= 0xFFFFFFFF;

						gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &textureId, 1 );
					}
				}
			}
		}
	}
}


void UIGame :: ShowMultiplayerHostOptions()
{
	if ( !gServer.IsLocal() )
	{
		return;
	}

	if ( gUIShell.IsInterfaceVisible( "in_game_menu" ) )
	{
		m_pUIMenuManager->OptionsMenu();
	}

	UICheckbox * pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_allow_player_joining", "in_game_host_options" );
	if ( pCheckbox )
	{
		pCheckbox->SetState( gServer.GetAllowJIP() );
	}

	pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_allow_player_pausing", "in_game_host_options" );
	if ( pCheckbox )
	{
		pCheckbox->SetState( gServer.GetAllowPausing() );
	}

	UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "change_game_password_edit_box", "in_game_host_options" );
	if ( pEditBox )
	{
		pEditBox->SetText( gServer.GetGamePassword() );
		//pEditBox->SetText( L"" );
	}

	UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "kick_players_listbox", "in_game_host_options_kick" );
	if ( pListbox )
	{
		pListbox->RemoveAllElements();

		for ( Server::PlayerColl::const_iterator player = gServer.GetPlayers().begin(); player != gServer.GetPlayers().end(); ++player )
		{
			if ( !IsValid(*player) || (*player)->IsComputerPlayer() || (*player == gServer.GetLocalHumanPlayer()) )
			{
				continue;
			}

			pListbox->AddElement( (*player)->GetName(), MakeInt( (*player)->GetId() ) );
		}

		UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_kick", "in_game_host_options_kick" );
		if ( pButton )
		{
			pButton->SetEnabled( pListbox->GetNumElements() > 0 );
		}
		pButton = (UIButton *)gUIShell.FindUIWindow( "button_ban", "in_game_host_options_kick" );
		if ( pButton )
		{
			pButton->SetEnabled( pListbox->GetNumElements() > 0 );
		}
	}

	UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_choose_player_limit", "in_game_host_options" );
	if ( pCombo )
	{
		pCombo->SetText( gpwstringf( gpwtranslate( $MSG$ "%d Players" ), gServer.GetMaxPlayers() ) );	
		
		UIListbox * listBox = (UIListbox *)gUIShell.FindUIWindow( "listbox_choose_player_limit" );
		if ( listBox )
		{
			listBox->RemoveAllElements();
			for ( int i = gServer.GetNumHumanPlayers()+1; i <= Server::MAX_PLAYERS; ++i )
			{
				listBox->AddElement( gpwstringf( gpwtranslate( $MSG$ "%d Players" ), i ), i );
			}
		}
	}

	pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_allow_choose_start_locations", "in_game_host_options" );
	if ( pCheckbox )
	{
		pCheckbox->SetState( gServer.GetAllowStartLocationSelection() );
	}

	pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_restrict_players_to_new_characters", "in_game_host_options" );
	if ( pCheckbox )
	{
		pCheckbox->SetState( gServer.GetAllowNewCharactersOnly() );
	}	

	gUIShell.ShowInterface( "in_game_host_options" );
	gUIShell.HideInterface( "in_game_host_options_kick" );
}


void UIGame :: CloseMultiplayerHostOptions( bool setOptions )
{
	if ( !gServer.IsLocal() )
	{
		return;
	}

	if ( setOptions )
	{
		UICheckbox * pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_allow_player_joining", "in_game_host_options" );
		if ( pCheckbox )
		{
			if ( gUIMultiplayer.GetMultiplayerMode() == MM_GUN )
			{
				if ( gServer.GetAllowJIP() && !pCheckbox->GetState() )
				{
					gUIZoneMatch.OnInGameDisableJIP();
				}
				else if ( !gServer.GetAllowJIP() && pCheckbox->GetState() )
				{
					gUIZoneMatch.OnInGameEnableJIP();					
				}
			}
			
			if ( pCheckbox->GetState() != gServer.GetAllowJIP() )
			{				
				if ( pCheckbox->GetState() )
				{
					SSendMessage( gpwtranslate( $MSG$ "Host has allowed new players to join the game while in progress." ) );
				}
				else
				{
					SSendMessage( gpwtranslate( $MSG$ "Host has prevented new players from joining the game while in progress." ) );
				}							
			}

			gServer.SSetAllowJIP( pCheckbox->GetState() );
		}

		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_allow_player_pausing", "in_game_host_options" );
		if ( pCheckbox )
		{
			if ( pCheckbox->GetState() != gServer.GetAllowPausing() )
			{				
				if ( pCheckbox->GetState() )
				{
					SSendMessage( gpwtranslate( $MSG$ "Host has allowed all players to be able to pause the game." ) );
				}
				else
				{
					SSendMessage( gpwtranslate( $MSG$ "Host has restricted all players from pausing the game." ) );
				}						
			}	

			gServer.SSetAllowPausing( pCheckbox->GetState() );					
		}

		UIComboBox *pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_choose_player_limit", "in_game_host_options" );
		if ( pCombo )
		{
			for ( unsigned int players = 2; players <= Server::MAX_PLAYERS; ++players )
			{
				if ( pCombo->GetText().same_no_case( gpwstringf( gpwtranslate( $MSG$ "%d Players" ), players ), players ) )
				{
					if ( players < (unsigned int)Player::GetHumanPlayerCount() )
					{						
						return;
					}

					NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

					if( session && ( players != session->GetNumMachinesMax() ) )
					{
						gServer.SSetMaxPlayers( players );

						if ( gUIZoneMatch.IsHosting() )
						{
							gUIZoneMatch.HostUpdateGameSetting( ZONE_GAME_MAX_PLAYERS, gpwstringf( L"%d", gServer.GetMaxPlayers() ) );
						}
						
						gpwstring sMessage;
						sMessage.assignf( gpwtranslate( $MSG$ "Host set the player limit to %d players." ) , players );

						SSendMessage( sMessage );						
					}
					break;
				}
			}
		}

		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_allow_choose_start_locations", "in_game_host_options" );
		if ( pCheckbox )
		{
			if ( gServer.GetAllowStartLocationSelection() != pCheckbox->GetState() )
			{
				gServer.SSetAllowStartLocationSelection( pCheckbox->GetState() );

				if ( gServer.GetAllowStartLocationSelection() )
				{					
					SSendMessage( gpwtranslate( $MSG$ "Host has allowed the selection of Start Locations." ) );
				}
				else
				{					
					SSendMessage( gpwtranslate( $MSG$ "Host has prevented the selection of Start Locations." ) );
				}

				if ( gUIZoneMatch.IsHosting() )
				{
					gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_ALLOW_START_SELCTION, gServer.GetAllowStartLocationSelection() );
				}
			}
		}

		pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_restrict_players_to_new_characters", "in_game_host_options" );
		if ( pCheckbox )
		{
			if ( gServer.GetAllowNewCharactersOnly() != pCheckbox->GetState() )
			{
				gServer.SSetAllowNewCharactersOnly( pCheckbox->GetState() );

				if ( gServer.GetAllowNewCharactersOnly() )
				{					
					SSendMessage( gpwtranslate( $MSG$ "Host has restricted all players to new characters." ) );
				}
				else
				{					
					SSendMessage( gpwtranslate( $MSG$ "Host has allowed saved characters to be used." ) );
				}

				if ( gUIZoneMatch.IsHosting() )
				{
					gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_NEW_CHARACTERS_ONLY, gServer.GetAllowNewCharactersOnly() );
				}
			}
		}

		if ( gUIZoneMatch.IsHosting() )
		{
			gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_ALLOW_JIP, gServer.GetAllowJIP() );
			gUIZoneMatch.HostUpdateGameFlag( ZONE_GAME_ALLOW_PAUSING, gServer.GetAllowPausing() );
		}
	}

	gUIShell.HideInterface( "in_game_host_options" );
	gUIShell.HideInterface( "in_game_host_options_kick" );
}


// Teleports selected game object to where the mouse is pointing
bool UIGame :: TeleportGameObjectToMouse()
{
	if ( !gServer.IsLocal() )
	{
		return ( false );
	}

	if ( gSiegeEngine.GetMouseShadow().IsTerrainHit() )
	{
		GopColl selection = gGoDb.GetSelection();
		if( selection.size() == 1 )
		{
			GopColl::iterator go = selection.begin();

			// go to our teleport location
			SiegePos new_loc = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
			
			if ((*go)->HasFollower())
			{
				gMCPManager.Flush( (*go)->GetGoid() );
				Quat telport_rot_diff = (*go)->GetFollower()->GetCurrentOrientationRelativeToNode(new_loc.node);
				SiegeRot new_rot(telport_rot_diff,new_loc.node);
				gMCPManager.TeleportGo( (*go)->GetGoid(), new_loc, new_rot );
			} 
			else
			{
				(*go)->GetPlacement()->SetPosition( new_loc );
				gMCPManager.ResetGo( (*go)->GetGoid() );
			}


			if( (*go)->HasMind() )
			{
				// flush
				(*go)->GetMind()->SDoJob( MakeJobReq( JAT_STOP, JQ_ACTION, QP_CLEAR, AO_USER ) );
			}
		}
	}

	return true;
}


Goid UIGame :: ClosestSelectedCharacter( Goid go )
{
	GoHandle goalObject( go );

	return( ClosestSelectedCharacter( goalObject->GetPlacement()->GetPosition() ) );
}


Goid UIGame :: ClosestSelectedCharacter( SiegePos const & position )
{
	GoHandle cur;
	float closest = 100000.0f;
	Goid closestGUID = GOID_INVALID;

	GoidColl selectedparty;
	gUIPartyManager.GetSelectedPartyMembers( selectedparty );

	GoidColl::iterator i;
	for( i=selectedparty.begin(); i<selectedparty.end(); ++i )
	{
		cur = *i;
		// Since the pack mule only can hold equipment, it does not count as a "character"
		if ( cur->GetInventory()->IsPackOnly() ) {
			continue;
		}
		vector_3 diff = gSiegeEngine.GetDifferenceVector( cur->GetPlacement()->GetPosition(), position );
		float distSquared = Length2( diff );

		if( distSquared < closest )
		{	//Found closer selected character
			closest = distSquared;
			closestGUID = cur->GetGoid();
		}
	}
	return( closestGUID );
}


#if !GP_RETAIL

bool UIGame :: SetDebugProbeObject()
{
	Goid GOCursor = GOID_INVALID;

	if( gSiegeEngine.GetMouseShadow().IsHit() )
	{
		GOCursor = MakeGoid(gSiegeEngine.GetMouseShadow().GetHit());
	}

	gDebugProbe.SetProbeObject( GOCursor );

	return GOCursor != GOID_INVALID;
}


bool UIGame :: DebugMindProbe()
{
	if( SetDebugProbeObject() )
	{
		gDebugProbe.SetMode( DP_MIND );
	}
	else
	{
		gDebugProbe.SetMode( DP_NONE );
	}
	return true;
}


bool UIGame :: DebugMind2Probe()
{
	if( SetDebugProbeObject() )
	{
		gDebugProbe.SetMode( DP_MIND2 );
	}
	else
	{
		gDebugProbe.SetMode( DP_NONE );
	}
	return true;
}


bool UIGame :: DebugCombatProbe()
{
	if( SetDebugProbeObject() )
	{
		gDebugProbe.SetMode( DP_COMBAT );
	}
	else
	{
		gDebugProbe.SetMode( DP_NONE );
	}
	return true;
}


bool UIGame :: DebugBodyProbe()
{
	if( SetDebugProbeObject() )
	{
		gDebugProbe.SetMode( DP_BODY );
	}
	else
	{
		gDebugProbe.SetMode( DP_NONE );
	}
	return true;
}


bool UIGame :: DebugAspectProbe()
{
	if( SetDebugProbeObject() )
	{
		gDebugProbe.SetMode( DP_ASPECT );
	}
	else
	{
		gDebugProbe.SetMode( DP_NONE );
	}
	return true;
}


bool UIGame :: DebugProbeVerboseToggle()
{
	gDebugProbe.SetVerbose( !gDebugProbe.GetVerbose() );
	return true;
}


bool UIGame :: DebugHudObject()
{
	Goid GOCursor = GOID_INVALID;

	if( gSiegeEngine.GetMouseShadow().IsHit() )
	{
		GOCursor = MakeGoid(gSiegeEngine.GetMouseShadow().GetHit());

		if( gWorldOptions.GetDebugHUDObject() == GOCursor )
		{
			GOCursor = GOID_INVALID;
		}
	}

	if( GOCursor != GOID_INVALID )
	{
		gpscreen( "specific object HUD\n" );
	}

	gWorldOptions.SetDebugHUDObject( GOCursor );

	return true;
}




bool UIGame :: DebugHudActors()
{
	eDebugHudGroup NewHUD;
	if( gWorldOptions.GetDebugHudGroup() == DHG_ACTORS ) {
		NewHUD = DHG_NONE;
	}
	else {
		gpscreen( "actors HUD\n" );
		NewHUD = DHG_ACTORS;
	}

	gWorldOptions.SetDebugHudGroup( NewHUD );

	return true;
}


bool UIGame :: DebugHudItems()
{
	eDebugHudGroup NewHUD;
	if( gWorldOptions.GetDebugHudGroup() == DHG_ITEMS ) {
		NewHUD = DHG_NONE;
	}
	else {
		gpscreen( "items HUD\n" );
		NewHUD = DHG_ITEMS;
	}

	gWorldOptions.SetDebugHudGroup( NewHUD );

	return true;
}


bool UIGame :: DebugHudCommands()
{
	eDebugHudGroup NewHUD;
	if( gWorldOptions.GetDebugHudGroup() == DHG_COMMANDS )
	{
		NewHUD = DHG_NONE;
	}
	else {
		gpscreen( "commands HUD\n" );
		NewHUD = DHG_COMMANDS;
	}

	gWorldOptions.SetDebugHudGroup( NewHUD );

	return true;
}


bool UIGame :: DebugHudMisc()
{
	eDebugHudGroup NewHUD;
	if( gWorldOptions.GetDebugHudGroup() == DHG_MISC ) {
		NewHUD = DHG_NONE;
	}
	else {
		gpscreen( "misc HUD\n" );
		NewHUD = DHG_MISC;
	}

	gWorldOptions.SetDebugHudGroup( NewHUD );

	return true;
}


bool UIGame :: DebugHudAll()
{
	eDebugHudGroup NewHUD;
	if( gWorldOptions.GetDebugHudGroup() == DHG_ALL ) {
		NewHUD = DHG_NONE;
	}
	else
	{
		gpscreen( "all objects HUD\n" );
		NewHUD = DHG_ALL;
	}

	gWorldOptions.SetDebugHudGroup( NewHUD );

	return true;
}


bool UIGame :: DebugHudLabels()
{
	gpscreen( "HUD labels toggled\n" );
	gWorldOptions.ToggleDebugHudOptions( DHO_LABELS );

	return true;
}


bool UIGame :: DebugHudErrors()
{
	gpscreen( "HUD errors toggled\n" );
	gWorldOptions.ToggleDebugHudOptions( DHO_ERRORS );

	return true;
}


bool UIGame :: DebugHudDepth()
{
	gpscreen( "HUD depth toggled\n" );
	gWorldOptions.ToggleDebugHudOptions( DHO_DEPTH );
	return true;
}




bool UIGame :: DebugObjectUpdateBreak()
{
	if( gSiegeEngine.GetMouseShadow().IsHit() )
	{
		gGoDb.DebugBreakOn( MakeGoid(gSiegeEngine.GetMouseShadow().GetHit()) );
	}

	return true;
}


bool UIGame :: DebugToggleSelectionBrain()
{
	GopColl::const_iterator i;

	for ( i = gGoDb.GetSelection().begin(); i != gGoDb.GetSelection().end(); ++i )
	{
		GoMind const * mind = (*i)->GetMind();
		JobQueue const & jq = mind->GetQ( JQ_BRAIN );

		if( jq.empty() )
		{
			gpgenericf(( "Re-init brain for '%s'\n", (*i)->GetCommon()->GetScreenName().c_str() ));

			Job * pJob = (*i)->GetMind()->SDoJob( MakeJobReq( JAT_BRAIN, JQ_BRAIN, QP_CLEAR, AO_REFLEX ) );

			if( !pJob )
			{
				gpassertm( 0, "Failed to set da brain." );
			}
		}
		else {
			gpgenericf(( "Cleared brain for '%s'\n", (*i)->GetCommon()->GetScreenName().c_str() ));
			(*i)->GetMind()->Clear( JQ_BRAIN );
		}
	}

	return true;
}


bool UIGame :: DebugIncrementAnimation()
{
	// Increment the animation of the go under the cursor ONLY if we are paused
	if (gAppModule.IsUserPaused())
	{
		GoHandle go(MakeGoid(gSiegeEngine.GetMouseShadow().GetHit()));
		if( go && go->HasAspect() )
		{
			nema::Aspect& asp = *go->GetAspect()->GetAspectHandle().Get();

			if (asp.HasBlender()) 
			{
				if (asp.GetBlender()->DurationOfCurrentTimeWarp() <= asp.GetBlender()->TimeOfCurrentTimeWarp())
				{
					asp.RestartAnimationStateMachine();
				}

				float delta = gAppModule.GetControlKey() ? 1/60.0f : 1/20.0f;

				asp.UpdateAnimationStateMachine(delta);
			}
		}
	}

	return true;
}


// Increment the yellow number of the gui to signify another warning has been found
void UIGame :: SetGUIWarningCount( unsigned int count )
{
	UIText *pText = (UIText *)gUIShell.FindUIWindow( "warning_number" );
	if ( !pText ) {
		return;
	}

	gpwstring sNumber;
	sNumber.assignf( L"%d", count );
	pText->SetText( sNumber );
	pText->SetVisible( true );
}



// Increment the red number of the gui to signify another warning has been found
void UIGame :: SetGUIErrorCount( unsigned int count )
{
	UIText *pText = (UIText *)gUIShell.FindUIWindow( "error_number" );
	if ( !pText ) {
		return;
	}

	gpwstring sNumber;
	sNumber.assignf( L"%d", count );
	pText->SetText( sNumber );
	pText->SetVisible( true );
}


// Reset the warning count to zero
void UIGame :: ResetGUIWarningCount()
{
	UIText *pText = (UIText *)gUIShell.FindUIWindow( "warning_number" );
	if ( !pText ) {
		return;
	}
	pText->SetText( L"0" );
	pText->SetVisible( false );
}


// Reset the error count to zero
void UIGame :: ResetGUIErrorCount()
{
	UIText *pText = (UIText *)gUIShell.FindUIWindow( "error_number" );
	if ( !pText ) {
		return;
	}
	
	pText->SetText( L"0" );
	pText->SetVisible( false );
}


void UIGame :: CheckLodPolyCount()
{
	if ( !m_pPolyText )
	{
		m_pPolyText = (UIText *)gUIShell.FindUIWindow( "text_polycount", "debug" );
	}

	if ( !m_pPolyText )
	{
		return;
	}

	if ( !GetPolyAlert() )
	{
		m_pPolyText->SetVisible( false );
		return;
	}	

	float detail = gWorldOptions.GetObjectDetailLevel();
	LodToPolyMap::iterator i;
	for ( i = m_LodToPolyMap.begin(); i != m_LodToPolyMap.end(); ++i )
	{
		if ( detail <= (*i).first )
		{
			if ( gSiegeEngine.GetTotalTrianglesRendered() > (*i).second )
			{				
				if ( m_pPolyText )
				{
					gpwstring sTemp;
					sTemp.assignf( L"PolyAlert! LOD-%0.3f, Total: %d, Excess: %d", 
									detail,
									gSiegeEngine.GetTotalTrianglesRendered(),
									gSiegeEngine.GetTotalTrianglesRendered()-(*i).second );
					m_pPolyText->SetText( sTemp );
					m_pPolyText->SetVisible( true );
				}
			}
			else
			{				
				if ( m_pPolyText )
				{
					m_pPolyText->SetVisible( false );
				}
			}
		}
	}
}

#endif // !GP_RETAIL


bool UIGame :: MoveOrderFree()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	GopColl::iterator i;
	for ( i = gGoDb.GetSelectionBegin(); i != gGoDb.GetSelectionEnd(); ++i ) 
	{
		if( (*i)->HasActor() ) 
		{
			(*i)->GetMind()->RSSetMovementOrders( MO_FREE );
		}
	}
	
	gUIPartyManager.InitOrders();
	return true;
}


bool UIGame :: MoveOrderLimited()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	GopColl::iterator i;
	for ( i = gGoDb.GetSelectionBegin(); i != gGoDb.GetSelectionEnd(); ++i ) 
	{
		if( (*i)->HasActor() )
		{
			(*i)->GetMind()->RSSetMovementOrders( MO_LIMITED );
		}
	}
	
	gUIPartyManager.InitOrders();
	return true;
}


bool UIGame :: MoveOrderNever()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	GopColl::iterator i;
	for ( i = gGoDb.GetSelectionBegin(); i != gGoDb.GetSelectionEnd(); ++i ) 
	{
		if( (*i)->HasActor() ) 
		{
			(*i)->GetMind()->RSSetMovementOrders( MO_HOLD );
		}
	}
	
	gUIPartyManager.InitOrders();
	return true;
}



bool UIGame :: FightOrderAlways()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	GopColl::iterator i;
	for ( i = gGoDb.GetSelectionBegin(); i != gGoDb.GetSelectionEnd(); ++i ) 
	{
		if( (*i)->HasActor() ) 
		{
			(*i)->GetMind()->RSSetCombatOrders( CO_FREE );
		}
	}
	
	gUIPartyManager.InitOrders();
	return true;
}


bool UIGame :: FightOrderBackOnly()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	GopColl::iterator i;
	for ( i = gGoDb.GetSelectionBegin(); i != gGoDb.GetSelectionEnd(); ++i ) 
	{
		if( (*i)->HasActor() ) 
		{
			(*i)->GetMind()->RSSetCombatOrders( CO_LIMITED );
		}
	}
	
	gUIPartyManager.InitOrders();
	return true;
}


bool UIGame :: FightOrderNever()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	GopColl::iterator i;
	for ( i = gGoDb.GetSelectionBegin(); i != gGoDb.GetSelectionEnd(); ++i ) 
	{
		if( (*i)->HasActor() ) 
		{
			(*i)->GetMind()->RSSetCombatOrders( CO_HOLD );
		}
	}
	
	gUIPartyManager.InitOrders();
	return true;
}

bool UIGame :: TargetOrderClosest()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	GopColl::iterator i;
	for ( i = gGoDb.GetSelectionBegin(); i != gGoDb.GetSelectionEnd(); ++i ) 
	{
		if( (*i)->HasActor() ) 
		{
			(*i)->GetMind()->RSSetFocusOrders( FO_CLOSEST );
		}
	}
	
	gUIPartyManager.InitOrders();
	return true;
}


bool UIGame :: TargetOrderWeakest()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	GopColl::iterator i;
	for ( i = gGoDb.GetSelectionBegin(); i != gGoDb.GetSelectionEnd(); ++i ) 
	{
		if( (*i)->HasActor() ) 
		{
			(*i)->GetMind()->RSSetFocusOrders( FO_WEAKEST );
		}
	}
	
	gUIPartyManager.InitOrders();
	return true;
}


bool UIGame :: TargetOrderStrongest()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	GopColl::iterator i;
	for ( i = gGoDb.GetSelectionBegin(); i != gGoDb.GetSelectionEnd(); ++i ) 
	{
		if( (*i)->HasActor() ) 
		{
			(*i)->GetMind()->RSSetFocusOrders( FO_STRONGEST );
		}
	}

	gUIPartyManager.InitOrders();	
	return true;
}

bool UIGame :: PartyHealBodyWithPotions()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP || gUIShell.GetItemActive() )
	{
		return true;
	}
	
	GopColl partyColl;
	Go * pParty = gServer.GetScreenParty();

	if( pParty != NULL )
	{
		partyColl = pParty->GetChildren();

		if( partyColl.empty() )
		{
			return false;
		}
	}

	GoidColl ids;
	partyColl.Translate( ids );
	gAIAction.RSDrinkLifeHealingPotions( ids, AO_USER );
	return true;
}


bool UIGame :: PartyHealMagicWithPotions()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP || gUIShell.GetItemActive() )
	{
		return true;
	}

	GopColl partyColl;
	Go * pParty = gServer.GetScreenParty();

	if( pParty != NULL )
	{
		partyColl = pParty->GetChildren();

		if( partyColl.empty() )
		{
			return false;
		}
	}

	GoidColl ids;
	partyColl.Translate( ids );

	gAIAction.RSDrinkManaHealingPotions( ids, AO_USER );
	return true;
}


bool UIGame :: PartyHealBodyWithMagic()
{
	GopColl partyColl;
	Go * pParty = gServer.GetScreenParty();

	if( pParty != NULL )
	{
		partyColl = pParty->GetChildren();

		if( partyColl.empty() )
		{
			return false;
		}
	}

	GoidColl ids;
	partyColl.Translate( ids );

	gAIAction.RSCastLifeHealingSpells( ids, AO_USER );
	return true;
}


#if !GP_RETAIL

bool UIGame :: SuperRecovery()
{
	GopColl partyColl;
	Go * pParty = gServer.GetScreenParty();

	if( pParty != NULL )
	{
		partyColl = pParty->GetChildren();

		if( partyColl.empty() )
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	// Go through the members of the party
	for( GopColl::iterator i = partyColl.begin(); i != partyColl.end(); ++i )
	{
		GoHandle hMember( (*i)->GetGoid() );
		if ( hMember.IsValid() ) {
			float max_life = hMember->GetAspect()->GetMaxLife();
			float max_mana = hMember->GetAspect()->GetMaxMana();
			hMember->GetAspect()->SSetCurrentLife( max_life );
			hMember->GetAspect()->SSetCurrentMana( max_mana );
		}
	}
	return true;
}

#endif // !GP_RETAIL


bool UIGame :: ToggleGUIEditBox()
{
	if ( !gUIMenuManager.GetIsDefeated() )
	{
		ShowGUIEditBox();
	}
	return true;
}

bool UIGame :: ToggleGUIEditBoxTeam()
{
	ShowGUIEditBox( true );
	return true;
}

bool UIGame :: ToggleGUIEditBoxEveryone()
{
	ShowGUIEditBox( false, true );
	return true;
}

bool UIGame :: ShowGUIEditBox( bool bTeam, bool bEveryone )
{
	if ( !gUIMenuManager.IsOptionsMenuActive() )
	{
		UIWindow * pWindow;
		if ( ::IsMultiPlayer() )			
		{
			pWindow = gUIShell.FindUIWindow( "console_edit", "game_console_mp" );
		}
		else
		{
			pWindow = gUIShell.FindUIWindow( "console_edit", "game_console" );
		}
		if ( pWindow )
		{
			bool bShow = !pWindow->GetVisible();
			gUIShell.ShowGroup( "console_edit", bShow );
			m_bEditBoxVisible = bShow;

			// make sure to update our chatbox! so it knows that it's friend is active!
			UIChatBox * pChatBox;
			pChatBox = (UIChatBox*)gUIShell.FindUIWindow( "console_box", "console_output" );
			if( pChatBox )
			{
				pChatBox->SetEditBoxActive( bShow );
			}
		}
		return UpdateGuiEditBox( bTeam, bEveryone );
	}

	return false;
}

bool UIGame :: UpdateGuiEditBox( bool bTeam, bool bEveryone )
{
	if ( !gUIMenuManager.IsOptionsMenuActive() )
	{
		if ( bTeam && gVictory.GetTeamMode() == Victory::NO_TEAMS )
		{
			return false;
		}

		if ( ::IsMultiPlayer() )			
		{
			gpstring sInterface = "game_console_mp";
			if ( IsStagingArea( gWorldState.GetCurrentState() ) )
			{
				sInterface = "staging_area";
			}
			else if ( IsEndGame( gWorldState.GetCurrentState() ) )
			{
				sInterface = "end_game_summary";
			}

			UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_players", sInterface );
			if( pList )
			{
				
				int tag = pList->GetSelectedTag();
				pList->SetVisible( false );
				pList->RemoveAllElements();

				// add a header for the people in the game non-selectable "Players In Game"
				// this looked bad..  take out for now.
				//pList->AddElement( gpwtranslate( $MSG$ "Players In Game" ) , (int)CHAT_PLAYERS_END );
				//pList->SetElementSelectable( CHAT_PLAYERS_END, false);									

				NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
				if ( session )
				{
					pList->AddElement( gpwtranslate( $MSG$ "Everyone in Game" ), (int)CHAT_ALL );

					if ( gVictory.GetTeamMode() != Victory::NO_TEAMS )
					{
						pList->AddElement( gpwtranslate( $MSG$ "Team" ), (int)CHAT_TEAM );
					}

					Server::PlayerColl players = gServer.GetPlayers();
					Server::PlayerColl::iterator i;
					for ( i = players.begin(); i != players.end(); ++i )
					{
						if ( IsValid( *i ) && !(*i)->IsComputerPlayer() && !(*i)->IsLocalHumanPlayer() )
						{	
							gpwstring heroName;
							heroName.assign( (*i)->GetName() );

							// use the hero name in game...  become the character :)
							if( IsInGame( gWorldState.GetCurrentState() ) )
							{
								heroName.assign( (*i)->GetHeroName() );
							}

							pList->AddElement( heroName, (int)((*i)->GetId()) );
						}
					}

					// add a header to seperate freinds from game players with a non-selectable 
					pList->AddElement( L"" , (int)(CHAT_FRIEND) );
					pList->SetElementSelectable((int)(CHAT_FRIEND), false);									
				}

				if( IsOnZone() )
				{
					// put all your friends in the little message list.  then we can talk to them too!
					// but only do it if they are online.  use a -1 to denote that they are not a real
					// person in the game, but a special friend.

					UIZoneMatch::FriendsMap friendsNames = gUIZoneMatch.GetFriendsNames();
					if( friendsNames.size() > 0 )
					{
						// make sure our icons exist
						enum {ICON_USER_ACTIVE, ICON_USER_OFFLINE};
						pList->AddIcon( "b_gui_fe_m_mp_zone_user_active", ICON_USER_ACTIVE );
						pList->AddIcon( "b_gui_fe_m_mp_zone_user_asleep", ICON_USER_OFFLINE );

						int count = 1;
						// add a header for the friends list first, with a non-selectable "friends"
						// but only if we have elements above it
						if( pList->GetNumElements() )
						{
							pList->AddElement( gpwtranslate( $MSG$ "Zonematch Friends" ), (int)(CHAT_FRIEND) - count );
							pList->SetElementSelectable((int)(CHAT_FRIEND) - count, false);
							count++;
						}
						
						UIZoneMatch::FriendsMap::iterator i;					
						for ( i = friendsNames.begin(); i != friendsNames.end(); ++i )
						{	
							if( gUIZoneMatch.IsFriendOnline( i->second ) )
							{
								// we send PLAYERID_INVALID because they aren't a real player.
								gpwstring name;
								name.assignf( L" %s", i->first );
								pList->AddElement( name, (int)(CHAT_FRIEND) - count );	
								pList->SetElementIcon((int)(CHAT_FRIEND) - count, ICON_USER_ACTIVE);
								
								count ++;
							}
						}
						for ( i = friendsNames.begin(); i != friendsNames.end(); ++i )
						{				
							// make the friend not selectable if we can't talk to them
							// but still display them for usability just like in the regular zm ui
							if( ! gUIZoneMatch.IsFriendOnline( i->second ) )
							{						
								gpwstring name;
								name.assignf( L" %s", i->first );
								pList->AddElement( name, (int)(CHAT_FRIEND) - count );
								pList->SetElementSelectable((int)(CHAT_FRIEND) - count, false);
								pList->SetElementIcon((int)(CHAT_FRIEND) - count, ICON_USER_OFFLINE);

								count ++;
							}
						}
					}
				}

				pList->SelectElement( tag );

				if ( session )
				{
					if ( pList->GetSelectedTag() == UIListbox::INVALID_TAG || bEveryone )
					{
						pList->SelectElement( (int)CHAT_ALL );
					}					
					
					if ( bTeam )
					{
						pList->SelectElement( (int)CHAT_TEAM );
					}
				}

				UIComboBox * pCombo = (UIComboBox *)gUIShell.FindUIWindow( "combo_box_players", sInterface );
				if ( pCombo )
				{
					pCombo->SetText( pList->GetSelectedText() );
				}

			}					
		}
	}

	return true;
}


bool UIGame :: HideGUIEditBox()
{		
	gUIShell.ShowGroup( "console_edit", false );
	gUIShell.ShowGroup( "console_multiplayer", false );
	m_bEditBoxVisible = false;	
	
	// make sure to update our chatbox! so it knows that it's friend is active!
	UIChatBox * pChatBox;
	pChatBox = (UIChatBox*)gUIShell.FindUIWindow( "console_box", "console_output" );
	if( pChatBox )
	{
		pChatBox->SetEditBoxActive( false );
	}

	return true;
}


void UIGame :: SSendMessage( const gpwstring & sMessage )
{
	CHECK_SERVER_ONLY;

	RCSendMessage( sMessage );
}


void UIGame :: RCSendMessage( const gpwstring & sMessage )
{
	FUBI_RPC_THIS_CALL( RCSendMessage, RPC_TO_ALL );

	gpscreen( ::ToAnsi( sMessage ) );
}

bool UIGame :: TogglePlayerRanks( const KeyInput & key )
{
	if ( ::IsMultiPlayer() )
	{
		static bool keyUp = true;

		if ( keyUp && (key.GetQualifiers() & Keys::Q_KEY_DOWN) )
		{
			keyUp = false;
			m_pUIPlayerRanks->SetVisible( !m_pUIPlayerRanks->IsVisible() );
		}
		else if ( key.GetQualifiers() & Keys::Q_KEY_UP )
		{
			keyUp = true;
		}
	}
	return true;
}

bool UIGame :: TogglePlayerLabels()
{
	if ( ::IsMultiPlayer() )
	{
		m_bPlayerLabelsVisible = !m_bPlayerLabelsVisible;
	}
	else
	{
		gUIPartyManager.ToggleMemberLabels();
	}

	return true;
}


bool UIGame :: ToggleGameTimer()
{
	//if ( ::IsMultiPlayer() )
	{
		m_bGameTimerVisible = !m_bGameTimerVisible;
		if ( m_bGameTimerVisible )
		{
			gUIShell.ShowInterface( "multiplayer_timer" );
		}
		else
		{
			gUIShell.HideInterface( "multiplayer_timer" );
		}
	}
	return true;
}


bool UIGame :: ToggleItemLabels()
{
	if ( gAppModule.IsAppActive() )
	{
		if ( gWorldOptions.GetIndicateItems() )
		{
			gWorldOptions.SetIndicateItems( false );
			m_pUIItemOverlay->SetVisible( false );
			gSoundManager.PlaySample( "s_e_gui_element_button_SED" );
		}
		else
		{
			gWorldOptions.SetIndicateItems( true );
			m_pUIItemOverlay->SetVisible( true );
			gSoundManager.PlaySample( "s_e_gui_element_button_SED" );
		}
	}
	return true;
}


void UIGame::SetItemLabels( bool bSet )
{	
	gWorldOptions.SetIndicateItems( bSet );
	if ( m_pUIItemOverlay )
	{		
		m_pUIItemOverlay->SetVisible( bSet );
	}		
}


bool UIGame :: ToggleMiniMap()
{
	if ( !gUIMenuManager.IsOptionsMenuActive() )
	{
		if( gWorldState.GetCurrentState() != WS_MEGA_MAP )
		{
			if ( gWorldState.GetPendingState() != WS_MEGA_MAP )
			{
				gWorldStateRequest( WS_MEGA_MAP );
				UICheckbox * pCheckBox = (UICheckbox *)gUIShell.FindUIWindow( "button_mega_map" );
				if ( pCheckBox )
				{
					pCheckBox->SetState( true );
				}				
			}
		}
		else
		{
			if ( gWorldState.GetPreviousState() == WS_SP_NIS )
			{
				if ( gWorldState.GetPendingState() != WS_SP_INGAME &&
					 gWorldState.GetCurrentState() != WS_SP_INGAME )
				{
					gWorldStateRequest( WS_SP_INGAME );

					UICheckbox * pCheckBox = (UICheckbox *)gUIShell.FindUIWindow( "button_mega_map" );
					if ( pCheckBox )
					{
						pCheckBox->SetState( false );
					}
				}
			}
			else
			{
				if ( (gWorldState.GetPendingState() != WS_SP_INGAME &&
					  gWorldState.GetCurrentState() != WS_SP_INGAME) &&
					  (gWorldState.GetPendingState() != WS_MP_INGAME &&
					  gWorldState.GetCurrentState() != WS_MP_INGAME) )
				{
					if ( ::IsSinglePlayer() )
					{
						gWorldStateRequest( WS_SP_INGAME );
					}
					else
					{
						gWorldStateRequest( WS_MP_INGAME );
					}

					UICheckbox * pCheckBox = (UICheckbox *)gUIShell.FindUIWindow( "button_mega_map" );
					if ( pCheckBox )
					{
						pCheckBox->SetState( false );
					}
				}
			}			
		}
	}

	return true;
}


bool UIGame :: InGamePressedEscape()
{
	if ( IsInGame( gWorldState.GetCurrentState() ) )
	{
		if ( GetBackendMode() == BMODE_SELECTION_BOX ||
			 GetBackendMode() == BMODE_DRAG_ITEM )
		{
			gUIMessenger.SendUIMessage( UIMessage( MSG_DEACTIVATEITEMS ) );
			gUIGame.SetSelectionBox( false );
			return true;
		}

		if ( gUIShell.GetItemActive() )
		{
			return true;
		}

		if ( gUIMenuManager.GetMultiRespawnOption() )
		{
			gUIMenuManager.ResumeGame();
			if ( gUIShell.IsInterfaceVisible( "multiplayer_respawn" ) )
			{
				gUIMenuManager.HideMultiRespawnMenu();
			}
			else
			{			
				gUIMenuManager.ShowMultiRespawnMenu();
			}
			return true;
		}
		else if ( gUIMenuManager.GetGoldTradeDialogActive() )
		{
			gUIMenuManager.SetGoldTradeDialogActive( false );
			return true;
		}
		else if ( gUIPartyManager.CloseAllCharacterMenus() )
		{
			gUIPartyManager.SetInventoriesVisible( false );	
			gUIMenuManager.CloseAllDialogs();
			gUIPartyManager.DisbandCancelAll();
			return true;
		}
		else if ( gUIMenuManager.CloseAllDialogs() )
		{
			return true;
		}
		else if ( gWorldState.GetCurrentState() == WS_MEGA_MAP )
		{
			ToggleMiniMap();
		}
		else if ( m_BackendDialog == DIALOG_EXIT_GAME )
		{
			gUIMenuManager.ReturnToMenu();
		}
		else if ( gUIShell.IsInterfaceVisible( "loadsave_game" ) )
		{
			m_pUIMenuManager->CloseLoadSave();
			m_pUIMenuManager->ResumeGame();
		}
		else if ( gUIOptions.IsActive() )
		{
			gUIOptions.Deactivate();
			m_pUIMenuManager->ResumeGame();
		}
		else if ( m_bEditBoxVisible )
		{
			HideGUIEditBox();
		}
		else
		{
			bool bOptions = true; 
			Formation * pFormation;
			if( gServer.GetScreenFormation( pFormation ) )
			{
				if ( pFormation->GetDrawAssignedSpots() )
				{
					pFormation->HideSpots();
					SetShowFormation( FLT_MAX );
					bOptions = false;
				}
			}
			
			if ( bOptions )
			{
				m_pUIMenuManager->OptionsMenu();
			}
		}
	}

	return true;
}


// Order the selected objects in using the distance from the
// source object.
void UIGame :: OrderDraggedRelativeToObject( Goid object )
{
	GoidColl newDragged;
	Goid relativeobject = object;
	GoHandle rel( relativeobject );
	GoHandle cur;

	while ( !m_DraggedObjects.empty() ) {

		float closestDist = 100000.0f;
		GoidColl::iterator closestIter = m_DraggedObjects.begin();
		GoidColl::iterator i;

		for ( i = m_DraggedObjects.begin(); i != m_DraggedObjects.end(); ++i )	{
			cur = *i;
			if ( cur.IsValid()  == false )
			{
				continue;
			}
			vector_3 diff = gSiegeEngine.GetDifferenceVector( rel->GetPlacement()->GetPosition(),cur->GetPlacement()->GetPosition() );
			float newDist = Length2( diff );
			if( newDist < closestDist )
			{
				closestDist = newDist;
				closestIter = i;
			}
		}

		newDragged.push_back( *closestIter );
		rel = *closestIter;
		m_DraggedObjects.erase( closestIter );

	}

	m_DraggedObjects.clear();
	GoidColl::iterator i;
	for ( i = newDragged.begin(); i != newDragged.end(); ++i ) {
		m_DraggedObjects.push_back( *i );
	}
}


void UIGame :: ResetDragTimer()
{
	m_DragTimer = 0.10f;
	m_DragX = gAppModule.GetCursorX();
	m_DragY = gAppModule.GetCursorY();
}


// Find out what the user is dragging
Goid UIGame :: GetDragged()
{
	if( !m_DraggedObjects.empty() ) {
		return( m_DraggedObjects.front() );
	}
	return( GOID_INVALID );
}


// Maintains a list of any game objects that are selected.
void UIGame :: SetDragged( Goid object )
{
	m_DraggedObjects.clear();
	m_DraggedObjects.push_back( object );

}


void UIGame :: SetDragged( const GoidColl& objects )
{
	m_DraggedObjects = objects;
}


void UIGame :: SetDragged( const GopColl& objects )
{
	m_DraggedObjects.clear();
	objects.Translate( m_DraggedObjects );
}


void UIGame :: SelectionChanged( void )
{
	GetUIPartyManager()->SelectValidPortraits( gGoDb.GetSelection() );
	gUIPartyManager.InitOrders();	
}


GoidColl & UIGame :: GetSelectedItems()
{
	m_SelectedItems.clear();
	GopColl selection = gGoDb.GetSelection();	
	GopColl::iterator i;
	for ( i = selection.begin(); i != selection.end(); ++i )
	{
		if ( (*i)->IsItem() )
		{
			m_SelectedItems.push_back( (*i)->GetGoid() );
		}
	}	

	return m_SelectedItems;
}


bool UIGame :: ShowInGameOptions()
{
	if ( gUIShell.IsInterfaceVisible( "in_game_menu" ) )
	{
		m_pUIMenuManager->OptionsMenu();
	}	

	if ( gUIOptions.IsActive() )
	{
		gUIOptions.Deactivate();
	}
	else
	{
		gUIPartyManager.CloseAllCharacterMenus();
		m_pUIPlayerRanks->SetVisible( false );
		if( ::IsMultiPlayer() )
		{
			m_pUIEmoteList->SetVisible( false );
		}
		gUIOptions.Activate();
	}

	return true;
}


bool UIGame :: ExitNis()
{
	if ( gWorldState.GetCurrentState() == WS_SP_NIS )
	{
		// draw black first
		gDefaultRapi.ClearAllBuffers();

		// see if we should go to front end instead
		if ( m_bEscNisToFrontEnd )
		{
			// abort!!! first exit NIS mode
			gWorldStateRequest( WS_SP_INGAME );
			gWorldState.Update();

			// now go back to front end
			gUIShell.SetGuiDisabled( false );
			gWorld.RestoreWorldViewport( 0 );
			gWorldStateRequest( WS_GAME_ENDED );
		}
		else
		{
			// need to temporarily disable sounds while we skip
			gSoundManager.StopAllStreams();
			gSoundManager.PauseAllSounds();
			gSoundManager.SetStasis( true );

			// fast forward time until we exit this state
			while ( gWorldState.GetCurrentState() == WS_SP_NIS )
			{
				gTattooGame.SkipTime( 0.25 );
			}

			// Re-enable sounds
			gSoundManager.SetStasis( false );
			gSoundManager.ResumeAllSounds();
		}
	}

	return true;
}


void UIGame :: ShowDefeatScreen( double secondsElapsed )
{	
	if ( m_DefeatFadeOut == DEFEAT_FADE_OUT_TIME )
	{
		m_pDefeatBg = gUIShell.FindUIWindow( "defeat_background" );
		if ( m_pDefeatBg )
		{
			m_pDefeatBg->SetAlpha( 0.0f );
		}
		UIWindow *pDefeatUI = gUIShell.FindUIWindow( "defeat_dialog_background" );
		if ( pDefeatUI )
		{
			pDefeatUI->SetVisible( false );
		}
		pDefeatUI = gUIShell.FindUIWindow( "defeat_text_background" );
		if ( pDefeatUI )
		{
			pDefeatUI->SetVisible( false );
		}
		pDefeatUI = gUIShell.FindUIWindow( "defeat_dialog_text" );
		if ( pDefeatUI )
		{
			pDefeatUI->SetVisible( false );
		}
		pDefeatUI = gUIShell.FindUIWindow( "defeat_yes" );
		if ( pDefeatUI )
		{
			pDefeatUI->SetVisible( false );
		}
		pDefeatUI = gUIShell.FindUIWindow( "defeat_no" );
		if ( pDefeatUI )
		{
			pDefeatUI->SetVisible( false );
		}
		
		gUIShell.ShowInterface( "defeat_menu" );

		m_DefeatFadeOut -= (float)secondsElapsed;

		m_bFadedOut = false;
	}
	else if ( (m_DefeatFadeOut <= 0.0f) && (!m_bFadedOut) )
	{
		if ( m_pDefeatBg )
		{
			m_pDefeatBg->SetAlpha( 0.55f );
		}

		gUIShell.ShowInterface( "defeat_dialog" );
		UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_defeat_game", "defeat_dialog" );
		if ( pText )
		{
			if ( gTattooGame.HasCampaignSaveGameInfos( true ) )
			{	
				if ( gUIMenuManager.HasDefeatHelp() )
				{
					gUIShell.ShowGroup( "defeat_yesnohelp", true );
					gUIShell.ShowGroup( "defeat_yesno", false );
				}
				else
				{
					gUIShell.ShowGroup( "defeat_yesno", true );
					gUIShell.ShowGroup( "defeat_yesnohelp", false );
				}
				gUIShell.ShowGroup( "defeat_ok", false );
				pText->SetText( gpwtranslate( $MSG$ "Do you want to load a saved game?" ) );				
			}
			else
			{
				gUIShell.ShowGroup( "defeat_ok", true );
				gUIShell.ShowGroup( "defeat_yesno", false );
				gUIShell.ShowGroup( "defeat_yesnohelp", false );
				pText->SetText( gpwtranslate( $MSG$ "Click OK to return to the Main Menu." ) );								
			}			
		}
		m_bFadedOut = true;
	}
	else if ( !m_bFadedOut )
	{
		m_DefeatFadeOut -= (float)secondsElapsed;
		if ( m_pDefeatBg )
		{
			m_pDefeatBg->SetAlpha( SINF( ((DEFEAT_FADE_OUT_TIME - m_DefeatFadeOut) / DEFEAT_FADE_OUT_TIME) * PIHALF ) * 0.55f );
		}
	}
}


void UIGame :: ShowDialog( eBackendDialog dialog, gpwstring dialogText )
{
	m_BackendDialog = dialog;
	m_BackendDialogJustShown = true;

	bool bYesNoDialog = true;
	bool bOkayDialog = false;

	UITextBox *pDialogText = (UITextBox *)gUIShell.FindUIWindow( "backend_dialog_text_box", "backend_dialog" );
	if ( !pDialogText )
	{
		return;
	}

	pDialogText->SetText( dialogText );

	gUIShell.ShowGroup( "disable_dialog_option", false, false, "backend_dialog" );

	switch ( dialog )
	{
		case DIALOG_OK:
		case DIALOG_DEFEAT_MAIN_MENU:
		case DIALOG_NO_QUICK_SAVE:
		{
			bYesNoDialog = false;
			bOkayDialog = true;	
			break;
		}		

		case DIALOG_OVERWRITE_SAVED_GAME:
		{
			pDialogText->SetText( gpwtranslate( $MSG$ "Are you sure that you want to overwrite the existing saved game?" ) );
			break;
		}

		case DIALOG_EXIT_GAME:
		{
			if ( ::IsMultiPlayer() )
			{
				if ( gServer.IsLocal() )
				{
					pDialogText->SetText( gpwtranslate( $MSG$ "This will end the game for all players. Changes to the world cannot be saved in multiplayer.\nAre you sure you wish to exit to Game Summary?" ) );
				}
				else
				{
					pDialogText->SetText( gpwtranslate( $MSG$ "This will disconnect you from the current game.\nAre you sure you wish to exit?" ) );
				}
			}
			else
			{
				pDialogText->SetText( gpwtranslate( $MSG$ "Are you sure you want to exit to the Main Menu without saving?" ) );
			}
			break;
		}

		case DIALOG_EXIT_WINDOWS:
		{
			pDialogText->SetText( gpwtranslate( $MSG$ "Do you want to exit to Windows?" ) );
			break;
		}

		case DIALOG_EXIT_GAME_STAGING_AREA:
		{
			pDialogText->SetText( gpwtranslate( $MSG$ "This will end the game for all players. Changes to the world cannot be saved in multiplayer.\nAre you sure you wish to return to the Staging Area?" ) );
			break;
		}

		case DIALOG_PACK_REHIRE:
		{
			pDialogText->SetText( gpwtranslate( $MSG$ "Would you like to add this character back into the party?" ) );
			break;
		}

		case DIALOG_DEFEAT_LOAD_GAME:
		{
			pDialogText->SetText( gpwtranslate( $MSG$ "Do you want to load a saved game?" ) );
			break;
		}

		case DIALOG_QUICK_SAVE:
		{
			pDialogText->SetText( gpwtranslate( $MSG$ "Do you wish to load your quick save?" ) );
			break;
		}

		case DIALOG_QUICK_LOAD:
		{
			pDialogText->SetText( gpwtranslate( $MSG$ "Do you wish to quick load?" ) );
			break;
		}		

		case DIALOG_CONFIRM_SAVE_DELETE:
		{
			pDialogText->SetText( gpwtranslate( $MSG$ "Do you wish to delete this saved game?" ) );
			break;
		}
		
		case DIALOG_CONFIRM_SELL_ALL:
		{
			gUIShell.ShowGroup( "disable_dialog_option", true, false, "backend_dialog" );
			break;
		}			
	}

	UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( "backend_button_yes", "backend_dialog" );
	if ( pButton )
	{
		pButton->SetVisible( bYesNoDialog );
	}
	pButton = (UIButton *)gUIShell.FindUIWindow( "backend_button_no", "backend_dialog" );
	if ( pButton )
	{
		pButton->SetVisible( bYesNoDialog );
	}
	pButton = (UIButton *)gUIShell.FindUIWindow( "backend_button_ok", "backend_dialog" );
	if ( pButton )
	{
		pButton->SetVisible( bOkayDialog );
	}

	if ( dialog != DIALOG_NONE )
	{
		gUIGame.GetGameInputBinder().SetIsActive( false );
		gUIGame.GetUserInputBinder().SetIsActive( false );

		gUIShell.ShowInterface( "backend_dialog" );
	}
	else
	{
		// make sure that we are ingame if we set our
		// game input binder to true.  we don't want
		// this getting turned on when we are exiting into
		// another interface. (13290)
		if( !IsEndGame(gWorldState.GetPendingState()) &&  !IsEndGame(gWorldState.GetCurrentState()) )
		{
			gUIGame.GetGameInputBinder().SetIsActive( true );
			gUIGame.GetUserInputBinder().SetIsActive( true );
		}

		gUIShell.HideInterface( "backend_dialog" );
	}
}


bool UIGame :: Xfer( FuBi::PersistContext& persist )
{
	persist.EnterBlock( "m_pUIPartyManager" );
	m_pUIPartyManager->Xfer( persist );	
	persist.LeaveBlock();

	persist.EnterBlock( "m_pUIInventoryManager" );
	m_pUIInventoryManager->Xfer( persist );
	persist.LeaveBlock();

	bool bTutorialTips = gUIMenuManager.GetEnableTips();
	persist.Xfer( "tutorial_tips", bTutorialTips );
	if ( persist.IsRestoring() )
	{
		gUIMenuManager.SetEnableTips( bTutorialTips );
	}	

	return ( true );
}


// Receives callback information from the gui
void UIGame :: GameGUICallback( gpstring const & message, UIWindow & ui_window )
{
	if ( !IsVisible() )
	{
		return;
	}
	
	if ( message.same_no_case( "item_interaction" ) )
	{
		OnSelectionUp();
	}
	if ( message.same_no_case( "options_menu" ) )
	{
		gUIMenuManager.OptionsMenu();
	}
	else if ( message.same_no_case( "show_exit_windows_dialog" ) )
	{
		gUIMenuManager.ExitWindowsDialog();
	}
	else if ( message.same_no_case( "show_exit_game_dialog" ) )
	{
		gUIMenuManager.ExitGameDialog();
	}
	else if ( message.same_no_case( "multiplayer_exit_to_staging_area" ) )
	{
		gUIGame.ShowDialog( DIALOG_EXIT_GAME_STAGING_AREA );
	}
	else if ( message.same_no_case( "load_game" ) )
	{
		gUIMenuManager.LoadGameDialog();
	}
	else if ( message.same_no_case( "save_game" ) )
	{
		gUIMenuManager.SaveGameDialog();
	}
	else if ( message.same_no_case( "options" ) )
	{
		ShowInGameOptions();
	}
	else if ( message.same_no_case( "resume_game" ) )
	{
		gUIMenuManager.ResumeGame();
	}
	else if ( message.same_no_case( "back_to_menu" ) )
	{
		gUIMenuManager.ReturnToMenu();
	}
	else if ( message.same_no_case( "loadsave_select_game" ) )
	{
		gUIMenuManager.SelectSaveGame();
	}
	else if ( message.same_no_case( "loadsave_game_ok" ) )
	{
		if ( gUIMenuManager.ProcessLoadSave() )
		{
			gUIMenuManager.CloseLoadSave();
			if ( !gUIMenuManager.GetExitAfterSave() )
			{
				gUIMenuManager.ResumeGame();			
			}
		}
	}
	else if ( message.same_no_case( "loadsave_game_delete" ) )
	{
		gUIMenuManager.DeleteSavedGameDialog();
	}
	else if ( message.same_no_case( "loadsave_game_cancel" ) )
	{
		if ( gWorldState.GetCurrentState() == WS_SP_DEFEAT )
		{
			gUIMenuManager.ExitGame();
		}
		else
		{
			gUIMenuManager.CloseLoadSave();
			gUIMenuManager.ResumeGame();
		}
	}
	else if ( message.same_no_case( "awp_slot_rollover" ) )
	{
		gUIInventoryManager.AwpSlotRollover( &ui_window );
	}
	else if ( message.same_no_case( "awp_slot_rolloff" ) )
	{
		gUIInventoryManager.AwpSlotRolloff( &ui_window );
	}
	else if ( message.same_no_case( "request_grid_pickup" ) )
	{
		gUIInventoryManager.GridRequestPickup( (UIGridbox *)&ui_window );
	}
	else if ( message.same_no_case( "grid_request_placement" ) )
	{
		gUIInventoryManager.GridRequestPlacement( (UIGridbox *)&ui_window );
	}
	else if ( message.same_no_case( "inventory" ) )
	{
		if ( !gUIShell.GetItemActive() )
		{
			gUIPartyManager.ConstructCharacterInterface( ui_window.GetIndex() );
		}
	}
	else if ( message.same_no_case( "hide_inventory" ) )
	{
		gUIPartyManager.HideInventory( ui_window.GetGroup(), ui_window.GetIndex() );
	}
	else if ( message.same_no_case( "hide_store_inventory" ) )
	{
		gUIStoreManager.DeactivateStore();
	}
	else if ( message.same_no_case( "character" ) )
	{
		if ( ::IsMultiPlayer() && ui_window.GetIndex() != 0 )
		{
			gUITeamManager.GuardOnCharacterPortrait( ui_window.GetIndex()-1 );
		}
		else
		{
			gUIPartyManager.SelectPortrait( ui_window.GetIndex() );
		}
	}
	else if ( message.same_no_case( "character_exit" ) )
	{
		gUIPartyManager.ButtonCloseCharacter( ui_window.GetIndex() );
	}
	else if ( message.same_no_case( "character_expand" ) )
	{
		gUIPartyManager.SetSelectedCharacterActive();
	}
	else if ( message.same_no_case( "inventory_expand" ) )
	{
		gUIPartyManager.SetSelectedInventoryActive();
	}
	else if ( message.same_no_case( "spell_close" ) )
	{
		gUIPartyManager.CloseSpellBook();
	}
	else if ( message.same_no_case( "spell_expand" ) )
	{
		gUIPartyManager.SetSelectedSpellsActive();
	}
	else if ( message.same_no_case( "action_stop" ) )
	{
		GetUICommands()->CommandStop();
	}
	else if ( message.same_no_case( "item_placement" ) )
	{
		gUIInventoryManager.GridItemPlacement( (UIGridbox *)&ui_window );
	}
	else if ( message.same_no_case( "item_pickup" ) )
	{
		gUIInventoryManager.GridItemPickup( (UIGridbox *)&ui_window );
	}
	else if ( message.same_no_case( "grid_autoplaced" ) )
	{
		gUIInventoryManager.GridItemAutoplaced( (UIGridbox *)&ui_window );
	}
	else if ( message.same_no_case( "itemslot_equip" ) )
	{
		gUIInventoryManager.ItemslotEquip( (UIItemSlot *)&ui_window );
	}
	else if ( message.same_no_case( "infoslot_equip" ) )
	{
		gUIInventoryManager.InfoslotEquip( (UIInfoSlot *)&ui_window );
	}
	else if ( message.same_no_case( "infoslot_request_remove" ) )
	{
		gUIInventoryManager.InfoslotRequestRemove( (UIInfoSlot *)&ui_window );
	}
	else if ( message.same_no_case( "itemslot_request_remove" ) )
	{
		gUIInventoryManager.ItemslotRequestRemove( (UIItemSlot *)&ui_window );
	}
	else if ( message.same_no_case( "activate_item" ) )
	{
		gUIInventoryManager.ActivateItem( MakeGoid( ((UIItem *)&ui_window)->GetItemID() ) );
	}
	else if ( message.same_no_case( "check_buyer_gold" ) )
	{
		gUIStoreManager.GridCheckBuyerGold( (UIGridbox *)&ui_window );
	}
	else if ( message.same_no_case( "check_inventory_space" ) )
	{
		gUIStoreManager.GridCheckDestInvSpace( (UIGridbox *)&ui_window );
	}
	else if ( message.same_no_case( "remove_item" ) )
	{
		gUIInventoryManager.ItemslotRemoveItem( &ui_window );
	}
	else if ( message.same_no_case( "place_item" ) )
	{
		gUIInventoryManager.ItemslotPlaceItem( (UIItemSlot *)&ui_window );
	}
	else if ( message.same_no_case( "multiple_item_drag" ) ) {
		gUIInventoryManager.GridMultipleItemDrag( (UIGridbox *)&ui_window );
	}
	else if ( message.same_no_case( "character_slot_1" ) )
	{
		gUIInventoryManager.ActivateMeleeWeapon( ui_window.GetIndex() );
	}
	else if ( message.same_no_case( "character_slot_2" ) )
	{
		gUIInventoryManager.ActivateRangedWeapon( ui_window.GetIndex() );
	}
	else if ( message.same_no_case( "character_slot_3" ) )
	{
		gUIInventoryManager.ActivatePrimarySpell( ui_window.GetIndex() );
	}
	else if ( message.same_no_case( "character_slot_4" ) )
	{
		gUIInventoryManager.ActivateSecondarySpell( ui_window.GetIndex() );
	}
	else if ( message.same_no_case( "ui_grid_item_rollover" ) )
	{
		gUIInventoryManager.GridItemRollover( (UIGridbox *)&ui_window );
	}
	else if ( message.same_no_case( "ui_grid_item_rolloff" ) )
	{
		gUIInventoryManager.GridItemRolloff( (UIGridbox *)&ui_window );
		if ( GetIsAutoInventoryUp() && gUIShell.IsInterfaceVisible( "character" ) )
		{
			SetAutoInventoryUp( false );
			gUIPartyManager.CloseAllCharacterMenus();
		}
	}
	else if ( message.same_no_case( "ui_slot_item_rollover" ) )
	{
		gUIInventoryManager.SlotRollover( &ui_window );
	}
	else if ( message.same_no_case( "ui_slot_item_rolloff" ) )
	{
		gUIInventoryManager.SlotRolloff( &ui_window );
	}
	else if ( message.same_no_case( "item_use" ) )
	{
		gUIInventoryManager.GridUseItem( (UIGridbox *)&ui_window );
	}		
	else if ( message.same_no_case( "list_spells" ) )
	{
		gUIInventoryManager.ListSpells( &ui_window );
	}
	else if ( message.same_no_case( "select_spell" ) )
	{
		gUIInventoryManager.ListSelectSpell( &ui_window );
	}	
	else if ( message.same_no_case( "hide_spell_list" ) )
	{
		gUIInventoryManager.HideSpellList();
	}	
	else if ( message.same_no_case( "console_submit_text" ) )
	{
		gUIGameConsole.ReceiveConsoleText( ((UIEditBox *)(&ui_window))->GetText() );
		((UIEditBox *)(&ui_window))->SetText( L"" );
	}
	else if ( message.same_no_case( "mute_dialogue" ) )
	{
		gUIDialogueHandler.MuteDialogue();
	}
	else if ( message.same_no_case( "unmute_dialogue" ) )
	{
		gUIDialogueHandler.UnMuteDialogue();
	}
	else if ( message.same_no_case( "reset_dialogue" ) )
	{
		gUIDialogueHandler.ResetDialogue();
	}
	else if ( message.same_no_case( "stop_dialogue" ) )
	{
		gUIDialogueHandler.StopDialogue( true );
	}
	else if ( message.same_no_case( "exit_dialogue" ) )
	{
		gUIDialogueHandler.ExitDialogue( true, true );
	}
	else if ( message.same_no_case( "gold_transfer" ) )
	{
		gUIInventoryManager.GoldTransferSelectedMember();
	}
	else if ( message.same_no_case( "gold_transfer_stash" ) )
	{
		gUIInventoryManager.GoldTransferStash();
	}
	else if ( message.same_no_case( "multi_gold_transfer" ) )
	{
		gUIPartyManager.SetSelectedCharacterIndex( ui_window.GetIndex() );
		gUIInventoryManager.GoldTransferSelectedMember();
	}
	else if ( message.same_no_case( "gold_transfer_up" ) )
	{
		gUIInventoryManager.ButtonGoldTransferInc();
	}
	else if ( message.same_no_case( "gold_transfer_down" ) )
	{
		gUIInventoryManager.ButtonGoldTransferDec();
	}
	else if ( message.same_no_case( "gold_transfer_complete" ) )
	{
		gUIInventoryManager.ButtonGoldTransferComplete();		
	}
	else if ( message.same_no_case( "gold_transfer_cancel" ) )
	{
		gUIMenuManager.SetGoldTradeDialogActive( false );
	}
	else if ( message.same_no_case( "store_tab_change" ) )
	{
		gUIStoreManager.TabStoreChange();
	}
	else if ( message.same_no_case( "hire_buy" ) )
	{
		gUIStoreManager.ButtonHire();
	}
	else if ( message.same_no_case( "hire_cancel" ) )
	{
		gUIPartyManager.CloseAllCharacterMenus();
	}
	else if ( message.same_no_case( "transfer_inventory" ) )
	{
		gUIStoreManager.TransferInventory();
	}
	else if ( message.same_no_case( "listener_rollover" ) )
	{
		if ( ui_window.GetName().same_no_case( "spellbook_listener" ) )
		{
			gUIInventoryManager.ListenerSpellBookRollover();
		}
		else
		{
			gUIInventoryManager.ListenerCharacterRollover();
		}
	}
	else if ( message.same_no_case( "listener_mouse_move" ) )
	{
		if ( !ui_window.GetName().same_no_case( "spellbook_listener" ) )
		{
			gUIPartyManager.RotatePaperDoll( -((float)(((UIMouseListener *)&ui_window)->GetDeltaX())/100.0f) );
		}
	}
	else if ( message.same_no_case( "listener_rolloff" ) )
	{
		if ( ui_window.GetName().same_no_case( "spellbook_listener" ) )
		{
			gUIInventoryManager.ListenerSpellBookRolloff();
		}
		else
		{
			gUIInventoryManager.ListenerCharacterRolloff();
		}
	}
	else if ( message.same_no_case( "listener_lbutton_up" ) )
	{
		if ( !ui_window.GetName().same_no_case( "spellbook_listener" ) )
		{
			gUIInventoryManager.ContextPlaceItem();	
		}
	}
	else if ( message.same_no_case( "equip_spell" ) )
	{
		gUIInventoryManager.ButtonEquipSpell( (UIButton *)&ui_window );
	}
	else if ( message.same_no_case( "unequip_spell" ) )
	{
		gUIInventoryManager.ButtonUnEquipSpell( (UIButton *)&ui_window );
	}
	else if ( message.same_no_case( "trade_cancel" ) )
	{
		gUIInventoryManager.RSCancelTrade( gUIInventoryManager.GetSourceTrade(), gUIInventoryManager.GetDestTrade() );
	}
	else if ( message.same_no_case( "trade_source_accept" ) )
	{
		if ( gUIInventoryManager.GetSourceTrade() && gUIInventoryManager.GetDestTrade() )
		{		
			gUIInventoryManager.RSTradeAccept(	true, 
												gUIInventoryManager.GetSourceTrade()->GetPlayer(), 
												gUIInventoryManager.GetDestTrade()->GetPlayer() );					
		}
		else if ( gUIInventoryManager.GetSourceTrade() )
		{
			if ( gUIInventoryManager.GetSourceTrade()->GetPlayer()->GetIsTrading() )
			{
				gUIInventoryManager.RSCancelTrade( gUIInventoryManager.GetSourceTrade(), NULL );
			}
		}
	}
	else if ( message.same_no_case( "trade_source_unaccept" ) )
	{
		if ( gUIInventoryManager.GetSourceTrade() && gUIInventoryManager.GetDestTrade() )
		{
			gUIInventoryManager.RSTradeAccept(	false,
												gUIInventoryManager.GetSourceTrade()->GetPlayer(), 
												gUIInventoryManager.GetDestTrade()->GetPlayer() );			
		}
		else if ( gUIInventoryManager.GetSourceTrade() )
		{
			if ( gUIInventoryManager.GetSourceTrade()->GetPlayer()->GetIsTrading() )
			{
				gUIInventoryManager.RSCancelTrade( gUIInventoryManager.GetSourceTrade(), NULL );
			}
		}
	}	
	else if ( message.same_no_case( "picture_item_drop" ) )
	{
		if ( !::IsMultiPlayer() || ui_window.GetIndex() == 0 )
		{
			UIItemVec item_vec;
			gUIShell.FindActiveItems( item_vec );
			UIItemVec::iterator i;

			bool bSuccess = false;
			for ( i = item_vec.begin(); i != item_vec.end(); ++i ) 
			{
				bSuccess = GetUICommands()->CommandGive( MakeGoid((*i)->GetItemID()), MakeGoid(gUIShell.GetRolloverItemslotID()) );
			}

			if ( bSuccess )
			{
				gUIShell.GetMessenger()->SendUIMessage( UIMessage( MSG_DEACTIVATEITEMS ) );
			}
		}
		else
		{
			gUITeamManager.ActivateTrade( ui_window.GetIndex()-1 );				
		}
	}
	else if ( message.same_no_case( "activate_compass" ) )
	{
		gSiegeEngine.GetCompass().SetActive( !gSiegeEngine.GetCompass().GetActive() );
		if ( gSiegeEngine.GetCompass().GetActive() )
		{
			gUIShell.ShowInterface( "compass_hotpoints" );
		}
		else
		{
			gUIShell.HideInterface( "compass_hotpoints" );
		}
	}
	else if ( message.same_no_case( "disband_selected_members" ) )
	{			
		gUIPartyManager.DisbandButton();
	}
	else if ( message.same_no_case( "heal_selected_members" ) )
	{
		PartyHealBodyWithPotions();
	}
	else if ( message.same_no_case( "select_all_members" ) )
	{
		SelectAllPartyMembers();
	}
	else if ( message.same_no_case( "activate_field_commands" ) )
	{
		gUIPartyManager.ActivateFieldCommands();
	}
	else if ( message.same_no_case( "activate_mini_map" ) )
	{
		ToggleMiniMap();
	}
	else if ( message.same_no_case( "y_top_dock_sucessful" ) )
	{
		if ( ui_window.GetGroup().same_no_case( "information_dockbar" ) )
		{
			gUIMenuManager.StatusBarDock( true, (UIDockbar *)&ui_window );
		}	
	}
	else if ( message.same_no_case( "y_bottom_dock_sucessful" ) )
	{
		if ( ui_window.GetGroup().same_no_case( "information_dockbar" ) )
		{			
			gUIMenuManager.StatusBarDock( false, (UIDockbar *)&ui_window );
		}		
	}
	else if ( message.same_no_case( "x_switch_y_dock_sucessful" ) )
	{
		gUIPartyManager.ArrangeFormation( gpstring::EMPTY );
		gUIMenuManager.ConsumeCharacterInput( &ui_window );	
		gUIPartyManager.AdjustAwpToggleLocation();
	}
	else if ( message.same_no_case( "dialogue_accept_member" ) )
	{
		gUIStoreManager.ButtonAcceptMember();
	}
	else if ( message.same_no_case( "dialogue_decline_member" ) )
	{
		gUIStoreManager.ButtonDeclineMember();
	}
	else if ( message.same_no_case( "dialogue_shop" ) )
	{
		gUIStoreManager.ButtonShop();
	}
	else if ( message.same_no_case( "dialogue_buy_packmule" ) )
	{
		gUIStoreManager.ButtonBuyPackmule();
	}
	else if ( message.same_no_case( "dialogue_more" ) ) 
	{
		gUIDialogueHandler.AdvanceDialogue();
	}
	else if ( message.same_no_case( "exit_inventory" ) )
	{
		gUIPartyManager.CloseAllCharacterMenus();
	}
	else if ( message.same_no_case( "exit_mini_map" ) )
	{
		ToggleMiniMap();
	}
	else if ( message.same_no_case( "convert_persist_items" ) )
	{
		gUIInventoryManager.ConvertPersistItems( (UIGridbox *)&ui_window );
	}

	//
	// PART 2
	//
	if ( message.same_no_case( "portrait_rollover" ) )
	{
		gUIPartyManager.PortraitRollover( &ui_window );
	}
	else if ( message.same_no_case( "portrait_rolloff" ) )
	{
		gUIPartyManager.PortraitRolloff( &ui_window );
	}
	else if ( message.same_no_case( "item_overlay_rdown" ) )
	{
		if ( !gUIShell.GetItemActive() )
		{
			Go * pItemGo = (Go *)ui_window.GetTag();
			if ( pItemGo && !gUIShell.GetItemActive() )
			{
				if ( pItemGo->IsSelected() )
				{
					SetDragged( gGoDb.GetSelection() );

					GopColl::const_iterator j, jbegin = gGoDb.GetSelectionBegin(), jend = gGoDb.GetSelectionEnd();
					for ( j = jbegin ; j != jend ; ++j )
					{
						if ( ((*j)->HasActor() && (*j)->GetActor()->GetCanBePickedUp()) || !(*j)->HasActor() )
						{
							UIItem *pItem = GetItemFromGO( *j );
							if ( pItem )
							{
								pItem->SetItemID( MakeInt( (*j)->GetGoid() ) );
							}
						}
					}

					GoHandle hActor( gUIPartyManager.GetSelectedCharacter() );
					if ( hActor->GetInventory() && hActor->GetInventory()->GetGridbox() )
					{
						hActor->GetInventory()->GetGridbox()->SetMultipleItems( true );
					}
				}
				else
				{
					gUIGame.SetDragged( pItemGo->GetGoid() );

					if ( gUIShell.IsInterfaceVisible( "character" ) )
					{
						gUIGame.GetUICommands()->CommandGet( pItemGo->GetGoid(), false, SOURCE_ACTOR, true );
					}
					else
					{
						UIItem *pItem = GetItemFromGO( pItemGo );
						if ( pItem )
						{
							pItem->SetItemID( MakeInt( pItemGo->GetGoid() ) );
						}
					}
				}

				gUIGame.SetBackendMode( BMODE_DRAG_ITEM );
				gUIGame.SetSelectionBox( false );
			}
			
			m_pUIItemOverlay->SetDragItemOverlay( true );		
		}
	}
	else if ( message.same_no_case( "item_overlay_ldown" ) )
	{
		if ( !gUIShell.GetItemActive() )
		{
			m_pUIItemOverlay->SetHitItemOverlay( true );			
		}
	}
	else if ( message.same_no_case( "item_overlay_rselect" ) )
	{	
	}
	else if ( message.same_no_case( "item_overlay_lselect" ) )
	{
		if ( !gUIShell.GetItemActive() )
		{
			Go * pItemGo = (Go *)ui_window.GetTag();
			if ( pItemGo )
			{
				if ( gAppModule.GetControlKey() )
				{
					m_pUIItemOverlay->SelectItem( pItemGo->GetGoid(), !m_pUIItemOverlay->IsItemSelected( pItemGo->GetGoid() ) );
				}
				else
				{
					// pickup selected items

					bool stackCommands = false;

					if ( !pItemGo->IsSelected() )
					{
						GetUICommands()->ContextActionOnGameObject( pItemGo->GetGoid() );
						stackCommands = true;
					}

					GopColl::iterator j, jbegin = gGoDb.GetSelectionBegin(), jend = gGoDb.GetSelectionEnd();
					for ( j = jbegin ; j != jend ; ++j )
					{
						if( !gUIPartyManager.IsPlayerPartyMember( (*j)->GetGoid() ) )
						{
							GetUICommands()->ContextActionOnGameObject( (*j)->GetGoid(), stackCommands );
							stackCommands = true;
						}
					}
				}
			}
			m_pUIItemOverlay->SetHitItemOverlay( true );			
		}
	}
	else if ( message.same_no_case( "item_overlay_release" ) )
	{
		if ( m_BackendMode == BMODE_DRAG_ITEM )
		{
			Goid object = MakeGoid( gUIShell.GetRolloverItemslotID() );
			if ( object == GOID_INVALID )
			{
				object = GetGoUnderCursor();
			}
			gUIInventoryManager.DragSelectedItemsToActor( object );

			m_pUIItemOverlay->SetDraggedGoid( GOID_INVALID );
			m_BackendMode = BMODE_NONE;
		}

		if ( m_pUIItemOverlay->GetDragItemOverlay() )
		{
			m_pUIItemOverlay->SetDragItemOverlay( false );
		}
	}
	else if ( message.same_no_case( "item_overlay_rselectall" ) )
	{
		Go * pItemGo = (Go *)ui_window.GetTag();
		if ( pItemGo )
		{
			m_pUIItemOverlay->SelectAllItemsLike( pItemGo );
		}
	}
	else if ( message.same_no_case( "item_overlay_lselectall" ) )
	{
		Go * pItemGo = (Go *)ui_window.GetTag();
		if ( pItemGo )
		{
			m_pUIItemOverlay->SelectAllItemsLike( pItemGo );
		}
	}
	else if ( message.same_no_case( "item_overlay_rollover" ) )
	{
		((UIText *)&ui_window)->SetColor( 0xFFFF0000 );			
	}
	else if ( message.same_no_case( "item_overlay_rolloff" ) )
	{
		((UIText *)&ui_window)->SetColor( 0xFFFFFFFF );			
	}
	else if ( message.same_no_case( "button_journal_1" ) )
	{
		gUIDialogueHandler.JournalButton1();
	}
	else if ( message.same_no_case( "button_journal_2" ) )
	{
		gUIDialogueHandler.JournalButton2();
	}
	else if ( message.same_no_case( "button_journal_exit" ) )
	{
		gUIDialogueHandler.ExitJournal();
	}
	else if ( message.same_no_case( "journal_select" ) )
	{
		gUIDialogueHandler.SelectQuest();
	}
	else if ( message.same_no_case( "infoslot_place" ) )
	{
		gUIInventoryManager.InfoslotPlaceItem( ( UIInfoSlot *)&ui_window );
	}
	else if ( message.same_no_case( "infoslot_remove" ) )
	{
		gUIInventoryManager.InfoslotRemoveItem( ( UIInfoSlot *)&ui_window );
	}
	else if ( message.same_no_case( "itemslot_use" ) )
	{
		gUIInventoryManager.ItemslotUseItem( (UIItemSlot *)&ui_window );
	}	
	else if ( message.same_no_case( "drink_health_potions" ) )
	{
		PartyHealBodyWithPotions();
	}
	else if ( message.same_no_case( "drink_mana_potions" ) ) 
	{
		PartyHealMagicWithPotions();
	}
	else if ( message.same_no_case( "toggle_quest_log" ) )
	{
		gUIDialogueHandler.ToggleJournal();
	}
	else if ( message.same_no_case( "toggle_field_commands" ) )
	{
		gSoundManager.PlaySample( "s_e_gui_element_small" );
		gUIPartyManager.SetCommandsActive( !gUIPartyManager.GetCommandsActive() );
	}
	else if ( message.same_no_case( "toggle_formations" ) )
	{
		gUIPartyManager.SetFormationsActive( !gUIPartyManager.GetFormationsActive() );
	}
	else if ( message.same_no_case( "formation_circle" ) )
	{
		Formation * formation = gServer.GetScreenFormation();
		if ( !formation->GetDrawAssignedSpots() )
		{
			gUIPartyManager.ArrangeFormation( "circle" );
		}
	}
	else if ( message.same_no_case( "formation_column" ) )
	{
		Formation * formation = gServer.GetScreenFormation();
		if ( !formation->GetDrawAssignedSpots() )
		{
			gUIPartyManager.ArrangeFormation( "column" );
		}
	}
	else if ( message.same_no_case( "formation_double_column" ) )
	{
		Formation * formation = gServer.GetScreenFormation();
		if ( !formation->GetDrawAssignedSpots() )
		{
			gUIPartyManager.ArrangeFormation( "double_column" );
		}
	}
	else if ( message.same_no_case( "formation_double_row" ) )
	{
		Formation * formation = gServer.GetScreenFormation();
		if ( !formation->GetDrawAssignedSpots() )
		{
			gUIPartyManager.ArrangeFormation( "double_line" );
		}
	}
	else if ( message.same_no_case( "formation_pyramid" ) )
	{
		Formation * formation = gServer.GetScreenFormation();
		if ( !formation->GetDrawAssignedSpots() )
		{
			gUIPartyManager.ArrangeFormation( "wedge" );
		}
	}
	else if ( message.same_no_case( "formation_row" ) )
	{
		Formation * formation = gServer.GetScreenFormation();
		if ( !formation->GetDrawAssignedSpots() )
		{
			gUIPartyManager.ArrangeFormation( "line" );
		}
	}
	else if ( message.same_no_case( "orders_movement" ) )
	{
		gUIPartyManager.MovementOrderRotate();
	}
	else if ( message.same_no_case( "orders_attack" ) )
	{
		gUIPartyManager.AttackOrderRotate();
	}
	else if ( message.same_no_case( "orders_targeting" ) )
	{
		gUIPartyManager.TargetingOrderRotate();
	}
	else if ( message.same_no_case( "orders_change" ) )
	{
		gUIPartyManager.HandleGuiOrder( ui_window.GetName() );
	}
	else if ( message.same_no_case( "compass_hide" ) )
	{
		gUIMenuManager.ShowCompass( false );
	}
	else if ( message.same_no_case( "compass_show" ) )
	{
		gUIMenuManager.ShowCompass( true );
	}
	else if ( message.same_no_case( "paperdoll_begin_view" ) ) 
	{
		gUIPartyManager.HideEquipment();
	}
	else if ( message.same_no_case( "paperdoll_end_view" ) ) 
	{
		if ( IsInGame( gWorldState.GetCurrentState() ) )
		{
			gUIPartyManager.ShowEquipment();
		}
	}
	else if ( message.same_no_case( "arrange_inventory" ) )
	{
		gUIPartyManager.SortMemberInventory( ui_window.GetIndex() );
	}	
	else if ( message.same_no_case( "maximize_fc" ) )
	{
		gUIPartyManager.ActivateFieldCommands();
	}
	else if ( message.same_no_case( "minimize_fc" ) )
	{
		gUIPartyManager.ActivateFieldCommands();
	}
	else if ( message.same_no_case( "backend_dialog_ok" ) )
	{
		if ( m_BackendDialogJustShown )
		{
			return;
		}

		switch ( m_BackendDialog )
		{
			case DIALOG_OVERWRITE_SAVED_GAME:
			{
				if ( gUIMenuManager.ProcessLoadSave( true ) )
				{
					gUIMenuManager.CloseLoadSave();
					gUIMenuManager.ResumeGame();
				}
				break;
			}

			case DIALOG_EXIT_GAME:
			{
				gUIMenuManager.ExitGame();
				break;
			}

			case DIALOG_EXIT_WINDOWS:
			{
				gUIMenuManager.ExitToWindows();
				break;
			}

			case DIALOG_EXIT_GAME_STAGING_AREA:
			{
				// $$$ temporarily disabling return to staging area
				/*
				if( gServer.IsLocal() )
				{
					for ( Server::PlayerColl::iterator i = gServer.GetPlayersBegin(); i != gServer.GetPlayersEnd(); ++i )
					{
						Player * player = *i;
						if ( player != NULL && !player->IsComputerPlayer() && player->IsRemote() )
						{
							gWorldState.SSetWorldStateOnMachine( player->GetMachineId(), WS_MP_INGAME, WS_MP_STAGING_AREA_CLIENT );
						}
					}
					gWorldStateRequest( WS_MP_STAGING_AREA_SERVER );
				}
				*/
				break;
			}

			case DIALOG_DISBAND:
			{
				gUIPartyManager.DisbandSelectedMember();
				break;
			}

			case DIALOG_PACK_REHIRE:
			{
				gUIStoreManager.ButtonPackRehire();
				break;
			}

			case DIALOG_DEFEAT_LOAD_GAME:
			{
				gUIMenuManager.LoadGameDialog();
				break;
			}

			case DIALOG_DEFEAT_MAIN_MENU:
			{
				gUIMenuManager.ExitGame();
				break;
			}

			case DIALOG_QUICK_SAVE:
			{
				gUIMenuManager.QuickSave();
				break;
			}

			case DIALOG_QUICK_LOAD:
			{
				gUIMenuManager.QuickLoad();
				break;
			}

			case DIALOG_CONFIRM_SAVE_DELETE:
			{
				gUIMenuManager.DeleteSavedGame();
				break;
			}

			case DIALOG_NO_QUICK_SAVE:
			{
				if ( !gUIMenuManager.GetGameWasPaused() )
				{
					gTattooGame.RSUserPause( false );
				}

				break;
			}		
			
			case DIALOG_CONFIRM_SELL_ALL:
			{
				gUIStoreManager.StoreSellAll( gUIStoreManager.GetSelectedSellTrait() ); 
				UICheckbox * pCheck = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_disable_dialog", "backend_dialog" );
				if ( pCheck && pCheck->GetState() )
				{
					gUIStoreManager.SetEnableSellAllDialog( false );
				}
			}
		}

		ShowDialog( DIALOG_NONE );
	}
	else if ( message.same_no_case( "backend_dialog_cancel" ) ||
			  message.same_no_case( "backend_dialog_esc_cancel" ) )
	{
		switch ( m_BackendDialog )
		{
			case DIALOG_EXIT_GAME:
			case DIALOG_EXIT_WINDOWS:
			case DIALOG_EXIT_GAME_STAGING_AREA:
			{
				if ( m_BackendDialog == DIALOG_EXIT_WINDOWS || ::IsMultiPlayer() )
				{
					gUIMenuManager.ReturnToMenu();
				}
				else if ( m_BackendDialog == DIALOG_EXIT_GAME && !message.same_no_case( "backend_dialog_esc_cancel" ) )
				{
					gUIMenuManager.SaveGameDialog();
				}
				break;
			}

			case DIALOG_DISBAND:
			{
				if ( message.same_no_case( "backend_dialog_esc_cancel" ) )
				{
					gUIPartyManager.DisbandCancelAll();
				}
				else
				{
					gUIPartyManager.DisbandCancelSelectedMember();
				}
				break;
			}

			case DIALOG_DEFEAT_LOAD_GAME:
			{
				gUIMenuManager.ExitGame();
				break;
			}

			case DIALOG_QUICK_SAVE:
			{
				if ( !gUIMenuManager.IsOptionsMenuActive() && !gUIMenuManager.GetGameWasPaused() )
				{
					gTattooGame.RSUserPause( false );
				}
				break;
			}

			case DIALOG_QUICK_LOAD:
			{
				if ( !gUIMenuManager.IsOptionsMenuActive() && !gUIMenuManager.GetGameWasPaused() )
				{
					gTattooGame.RSUserPause( false );
				}
				break;
			}
		}

		ShowDialog( DIALOG_NONE );
	}
	else if ( message.same_no_case( "resume_from_respawn" ) )
	{
		gUIMenuManager.HideMultiRespawnMenu();
	}
	else if ( message.same_no_case( "respawn" ) )
	{
		gUIPartyManager.RespawnCharacter();
		gUIMenuManager.SetMultiRespawnOption( false );
	}
	else if ( message.same_no_case( "collect_loot" ) )
	{
		gUIPartyManager.CollectLoot();
	}
	else if ( message.same_no_case( "console_expand" ) )
	{
		gUIGameConsole.ConsoleRollover();
	}
	else if ( message.same_no_case( "console_shrink" ) )
	{
		gUIGameConsole.ConsoleRolloff();
	}
	else if ( message.same_no_case( "follow" ) )
	{
		gUIPartyManager.SetFollowMode( ((UICheckbox *)&ui_window)->GetState() );
	}
	else if ( message.same_no_case( "close_book" ) )
	{
		gUIPartyManager.HideLoreBook();
	}
	else if ( message.same_no_case( "character_cast" ) )
	{
		gUIPartyManager.CastOnCharacterPortrait( ui_window.GetIndex() );
	}
	else if ( message.same_no_case( "team_cast" ) )
	{
		gUITeamManager.CastOnCharacterPortrait( ui_window.GetIndex()-1 );
	}
	else if ( message.same_no_case( "save_edit_change" ) )
	{
		gUIMenuManager.CheckLoadSaveEnable();
	}
	else if ( message.same_no_case( "labels_on" ) )
	{
		SetItemLabels( true );		
	}
	else if ( message.same_no_case( "labels_off" ) )
	{
		SetItemLabels( false );
	}
	else if ( message.same_no_case( "camera_track" ) )
	{
		ToggleCameraTrackAndHold();
	}
	else if ( message.same_no_case( "store_next" ) )
	{
		gUIStoreManager.DisplayNextPage();
	}
	else if ( message.same_no_case( "store_previous" ) )
	{
		gUIStoreManager.DisplayPreviousPage();
	}
	else if ( message.same_no_case( "hide_console" ) )
	{
		HideGUIEditBox();
	}
	else if ( message.same_no_case( "close_tips" ) )
	{
		gUIMenuManager.CloseTips( true );
	}
	else if ( message.same_no_case( "tip_next" ) )
	{
		gUIMenuManager.ShowNextTip();
	}
	else if ( message.same_no_case( "tip_previous" ) )
	{
		gUIMenuManager.ShowPreviousTip();
	}
	else if ( message.same_no_case( "tip_off" ) )
	{
		gUIMenuManager.SetEnableTips( false, true );
	}
	else if ( message.same_no_case( "tip_on" ) )
	{
		gUIMenuManager.SetEnableTips( true, true );
	}
	else if ( message.same_no_case( "toggle_tips" ) )
	{
		gUIMenuManager.ToggleTips();
	}
	else if ( message.same_no_case( "defeat_help" ) )
	{
		gUIMenuManager.DefeatHelp();
	}
	else if ( message.same_no_case( "defeat_yes" ) )
	{
		gUIMenuManager.LoadGameDialog();
	}
	else if ( message.same_no_case( "defeat_no" ) )
	{
		if ( ui_window.GetVisible() )
		{
			gUIMenuManager.ExitGame();
		}
		else
		{
			gUIMenuManager.LoadGameDialog();
		}
	}
	else if ( message.same_no_case( "fade_chapter" ) )
	{
		gUIMenuManager.FadeOutInterface( &ui_window );
	}
	else if ( message.same_no_case( "dialogue_button_1" ) )
	{
		gUIDialogueHandler.GenericButton1();
	}
	else if ( message.same_no_case( "dialogue_button_2" ) )
	{
		gUIDialogueHandler.GenericButton2();
	}
	else if ( message.same_no_case( "multiplayer_options" ) )
	{
		ShowMultiplayerHostOptions();
	}
	else if ( message.same_no_case( "show_host_game_settings" ) )
	{
		gUIShell.HideInterface( "in_game_host_options_kick" );
		gUIShell.ShowInterface( "in_game_host_options" );		
	}
	else if ( message.same_no_case( "show_host_kick_players" ) )
	{
		gUIShell.HideInterface( "in_game_host_options" );
		gUIShell.ShowInterface( "in_game_host_options_kick" );
	}
	else if ( message.same_no_case( "multiplayer_options_cancel" ) )
	{
		CloseMultiplayerHostOptions( false );
	}
	else if ( message.same_no_case( "multiplayer_options_ok" ) )
	{
		CloseMultiplayerHostOptions( true );
	}
	else if ( message.same_no_case( "accept_change_game_password" ) )
	{
		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

		if ( gServer.IsLocal() && session )
		{
			UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "change_game_password_edit_box", "in_game_host_options" );
			if ( pEditBox )
			{
				if ( !gServer.GetGamePassword().same_no_case( pEditBox->GetText() ) )
				{
					gServer.SSetGamePassword( pEditBox->GetText() );

					ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "The password has been changed." ) );
					
					SSendMessage( gpwtranslate( $MSG$ "The game password has been changed." ) );
				}
			}
		}
	}
	else if ( message.same_no_case( "host_kick_player" ) )
	{
		if ( gServer.IsLocal() )
		{
			UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "kick_players_listbox", "in_game_host_options_kick" );
			if ( pListbox && (pListbox->GetSelectedTag() != UIListbox::INVALID_TAG) )
			{
				gUIMultiplayer.SKickPlayer( MakePlayerId( pListbox->GetSelectedTag() ) );
				pListbox->RemoveElement( pListbox->GetSelectedTag() );
				UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_kick", "in_game_host_options_kick" );
				if ( pButton )
				{
					pButton->SetEnabled( pListbox->GetNumElements() > 0 );
				}
				pButton = (UIButton *)gUIShell.FindUIWindow( "button_ban", "in_game_host_options_kick" );
				if ( pButton )
				{
					pButton->SetEnabled( pListbox->GetNumElements() > 0 );
				}
			}
		}
	}
	else if ( message.same_no_case( "host_ban_player" ) )
	{
		if ( gServer.IsLocal() )
		{
			UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "kick_players_listbox", "in_game_host_options_kick" );
			if ( pListbox && (pListbox->GetSelectedTag() != UIListbox::INVALID_TAG) )
			{
				gUIMultiplayer.SBanPlayer( MakePlayerId( pListbox->GetSelectedTag() ) );
				pListbox->RemoveElement( pListbox->GetSelectedTag() );
				UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_kick", "in_game_host_options_kick" );
				if ( pButton )
				{
					pButton->SetEnabled( pListbox->GetNumElements() > 0 );
				}
				pButton = (UIButton *)gUIShell.FindUIWindow( "button_ban", "in_game_host_options_kick" );
				if ( pButton )
				{
					pButton->SetEnabled( pListbox->GetNumElements() > 0 );
				}
			}
		}
	}
	else if ( message.same_no_case( "show_jip_player_popup" ) )
	{
		if ( gServer.IsLocal() )
		{
			UIPopupMenu *pMenu = (UIPopupMenu *)gUIShell.FindUIWindow( "jip_player_popup_menu", "join_in_progress" );
			if ( pMenu )
			{
				pMenu->RemoveAllElements();

				PlayerId playerId = MakePlayerId( ui_window.GetTag() );

				PlayerId localPlayerId = PLAYERID_INVALID;
				if ( gServer.GetLocalHumanPlayer() )
				{
					localPlayerId = gServer.GetLocalHumanPlayer()->GetId();
				}

				if ( (playerId != PLAYERID_INVALID) &&
					 (playerId != localPlayerId) )
				{
					pMenu->SetTag( ui_window.GetTag() );
					pMenu->AddElement( gpwtranslate( $MSG$ "Eject Player" ), MENU_JIP_PLAYER_KICK );
					pMenu->AddElement( gpwtranslate( $MSG$ "Eject and Ban Player" ), MENU_JIP_PLAYER_BAN );
					pMenu->ShowAtCursor( UIPopupMenu::ALIGN_DOWN_RIGHT );
				}
			}
		}
	}
	else if ( message.same_no_case( "jip_player_popup_menu_select" ) )
	{
		if ( gServer.IsLocal() )
		{
			UIPopupMenu *pMenu = (UIPopupMenu *)&ui_window;

			switch ( pMenu->GetSelectedElement() )
			{
				case MENU_JIP_PLAYER_KICK:
				{
					gUIMultiplayer.SKickPlayer( MakePlayerId( pMenu->GetTag() ) );
					break;
				}

				case MENU_JIP_PLAYER_BAN:
				{
					gUIMultiplayer.SBanPlayer( MakePlayerId( pMenu->GetTag() ) );
					break;
				}
			}
		}
	}
	else if ( message.same_no_case( "hire_stats_close" ) )
	{
		gUIShell.HideInterface( "hire_stats" );
	}
	else if ( message.same_no_case( "hire_stats_show" ) )
	{
		gUIDialogueHandler.ShowHireStats();
	}
	else if ( message.same_no_case( "close_player_panel" ) )
	{
		gUIMultiplayer.HidePlayersPanel();
	}
	else if ( message.same_no_case( "sort_player_ranks" ) )
	{
		m_pUIPlayerRanks->SortByColumn( ui_window.GetTag() );
	}
	else if ( message.same_no_case( "modal_active" ) )
	{
		gUIMenuManager.SetModalActive( true );
	}
	else if ( message.same_no_case( "modal_inactive" ) )
	{
		gUIMenuManager.SetModalActive( false );
	}
	else if ( message.same_no_case( "mp_chat" ) )
	{
		ToggleGUIEditBox();
	}
	else if ( message.same_no_case( "toggle_pause" ) )
	{
		if ( gServer.GetAllowPausing() || gServer.IsLocal() )
		{
			gTattooGame.RSUserPause( !gAppModule.IsUserPaused() );
		}
	}
	else if ( message.same_no_case( "eg_close" ) )
	{
		gUIShell.MarkInterfaceForDeactivation( "end_game" );
		gTattooGame.RSUserPause( false );
		gVictory.RSSetGameEnded( false );
	}
	else if ( message.same_no_case( "trade_reset_gold" ) )
	{
		if ( gUIInventoryManager.GetSourceTrade() && !gUIShell.GetItemActive() )
		{
			gUIInventoryManager.RSTradeZeroGold( gUIInventoryManager.GetSourceTrade(), gUIInventoryManager.GetDestTrade() );
		}
	}
	else if ( message.same_no_case( "game_console_mp_refresh_listbox_players" ) )
	{
		if( IsMultiPlayer() )
		{
			gUIMultiplayer.UpdatePlayerListBox();
		}
	}
	else if( message.same_no_case( "ingame_chat_max" ) )
	{
		// resize the chat box
		gUIGameConsole.ResizeChatWindow( CHAT_SIZE_MAX );
	}
	else if( message.same_no_case( "ingame_chat_mid" ) )
	{
		// resize the chat box
		gUIGameConsole.ResizeChatWindow( CHAT_SIZE_MID );
	}
	else if( message.same_no_case( "ingame_chat_min" ) )
	{
		// resize the chat box
		gUIGameConsole.ResizeChatWindow( CHAT_SIZE_MIN );
	}
	else if ( message.same_no_case( "hide_stash" ) )
	{
		gUIInventoryManager.CloseStash();
	}
	else if ( message.same_no_case( "store_sell_all" ) )
	{
		gUIStoreManager.DialogStoreSellAll();		
	}
	else if ( message.same_no_case( "store_sell_all_potions" ) )
	{
		gUIStoreManager.DialogStoreSellAll( QT_POTION );		
	}
	else if ( message.same_no_case( "store_sell_all_spells" ) )
	{
		gUIStoreManager.DialogStoreSellAll( QT_SPELL );		
	}
	else if ( message.same_no_case( "store_sell_all_armor" ) )
	{
		gUIStoreManager.DialogStoreSellAll( QT_ARMOR_WEARABLE );		
	}
	else if ( message.same_no_case( "store_sell_all_weapons" ) )		
	{		
		gUIStoreManager.DialogStoreSellAll( QT_WEAPON );		
	}
	else if ( message.same_no_case( "store_sell_all_shields" ) )
	{
		gUIStoreManager.DialogStoreSellAll( QT_SHIELD );		
	}
	else if ( message.same_no_case( "toggle_expert_mode" ) )
	{
		gUIPartyManager.ActivateExpertMode();
	}
	//////////////////////////////////////////////
	//		Emotes
	//////////////////////////////////////////////
	else if ( message.same_no_case( "select_emote_list" ) )
	{
		if( ::IsMultiPlayer() )
		{
			GetUIEmoteList()->SelectEmote();
		}
	}
	else if ( message.same_no_case( "cancel_options_emote_list" ) )
	{
		if( ::IsMultiPlayer() )
		{
			GetUIEmoteList()->SetVisible( false );
		}
	}		
	else if ( message.same_no_case( "custom_name_spellbook" ) )
	{
		gUIPartyManager.RenameSelectedSpellBook( ((UIEditBox *)&ui_window)->GetText() );
	}
}
