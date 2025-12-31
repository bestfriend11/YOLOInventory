//////////////////////////////////////////////////////////////////////////////
//
// File     :  Preferences.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "PrecompEditor.h"
#include "Preferences.h"
#include "MainFrm.h"
#include "MenuCommands.h"
#include "EditorTerrain.h"
#include "resource.h"
#include "WorldState.h"
#include "LeftView.h"
#include "CustomViewer.h"
#include "siege_options.h"
#include "gps_manager.h"
#include "EditorObjects.h"
#include "FileSysUtils.h"
#include "SiegeEditor.h"
#include "Config.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Preferences::Preferences()
	: m_bWireframe( false )
	, m_bDrawDoors( false )
	, m_bDrawFloors( false )
	, m_bDrawBoxes( false )
	, m_bSLights( true )
	, m_bDLights( true )
	, m_bSShadows( false )
	, m_bDShadows( false )
	, m_bSShading( true )
	, m_bDShading( true )
	, m_bFPS( false )
	, m_bDrawWater( false )
	, m_bDrawNormals( false )
	, m_bDrawLNodes( false )
	, m_bIgnoreVertexColor( true )
	, m_bPolystats( false )
	, m_bUpdateEffects( false )
	, m_bDisablePref( false )
	, m_bDrawDoorNums( true )	
	, m_bPaintSelectNodes( false )
	, m_bSnapToGround( true )
	, m_content_display( DEV_NAME )
	, m_bPlacementMode( true )	
	, m_bDrawDirectional( true )
	, m_bShowObjects( true )
	, m_bShowLights( true )
	, m_bShowGizmos( true )
	, m_bShowTuningPoints( false )	
	, m_bShowToolBar( true )
	, m_bShowDlgBar( true )
	, m_bShowViewBar( true )
	, m_bShowNodeBar( true )
	, m_bShowObjectBar( true )
	, m_bShowLightBar( true )
	, m_bShowCameraBar( true )
	, m_bShowModeBar( true )
	, m_bShowPreferenceBar( true )
	, m_zoomRate( 0.5f )	
	, m_bModeLights( true )
	, m_bModeGizmos( true )
	, m_bModeObjects( true )
	, m_bModeNodes( true )
	, m_bModeDecals( true )
	, m_bModeLogicalNodes( false )	
	, m_bModeNodePlacement( false )
	, m_bMovementMode( true )
	, m_bUpdateWorld( false )
	, m_bUpdateSiegeEngine( false )
	, m_bDrawStitchDoors( false )
	, m_bSelectionLock( false )
	, m_bPlacementLock( false )
	, m_bUpdateSound( false )
	, m_bUpdateWeather( false )
	, m_bHitFloorOnly( false )
	, m_bLinkMode( false )
	, m_bCustomViewActive( false )
	, m_bSaveStitch( true )
	, m_bSaveNodes( true )
	, m_bSaveLights( true )
	, m_bSaveHotpoints( true )
	, m_bSaveGameObjects( true )
	, m_bSaveStartingPos( true )
	, m_bRightScroll( true )
	, m_bDisplayHex( false )
	, m_decalHoriz( 4.0f )
	, m_decalVert( 4.0f )
	, m_bRotateMode( true )
	, m_cameraZoomStep( 1.0f )
	, m_cameraTranslateStep( 1.0f )
	, m_cameraRotateStep( 1.0f )
	, m_bDrawCommands( false )
	, m_bSelectNoninteractive( true )
	, m_bSelectInteractive( true )
	, m_bSelectGizmos( true )
	, m_bSelectNonGizmos( true )
	, m_bSelectAll( true )
	, m_bDisplayFourCC( false )	
	, m_MoveLockMode( MOVE_LOCK_NONE )
	, m_rotationIncrement( 45.0f )
	, m_bHudSelected( false )
	, m_cameraSnapIncrement( 90.0f )
	, m_bLockCustomView( false )
	, m_bModeTuningPoints( false )
	, m_bDrawLogicalFlags( false )
	, m_bDrawLightHuds( false )
	, m_lastHudGroup( DHG_ALL )
	, m_bPreviewWireframe( false )	
	, m_dwPreviewColor( 0xff0080c0 )
	, m_dwClearColor( 0x00000000 )
	, m_bRestrictCameraToGame( false )
	, m_bDrawCameraTarget( false )
	, m_GridSize( 100 )
	, m_GridInterval( 100 )
	, m_bEnableGrid( false )
	, m_GridColor( 0xff0000ff )
	, m_bDrawLabelIntervals( false )
	, m_bDrawLabelAmount( false )
	, m_DrawLabelAmount( 100 )
	, m_bPlaceMeterPoints( false )
	, m_bDrawMeterPoints( true )
	, m_MeterPointColor( 0xff00ff00 )
	, m_bShowMeterPointDistance( false )
	, m_bVisualDrawDestId( true )
	, m_bVisualDrawDoorNumbers( false )
	, m_bVisualDrawMeshBox( false )
	, m_bAllowInvalidMatch( false )
	, m_VisualSnapSensitivity( 1.0f )
	, m_VisualValidColor( 0xff00ff00 )
	, m_VisualInvalidColor( 0xffff0000 )
	, m_VisualDestDoorColor( 0xffffff00 )
	, m_bVisualRotateOrient( false )
	, m_VisualSnapDelay( 0.1f )
	, m_bVisualAllowDoorGuess( false )
	, m_bVisualDrawDoors( true )
{
	m_PreviewWindowPlacement.length = 0;

	if ( gSiegeEngine.GetOptions().ShowFPSEnabled() ) 
	{
		gSiegeEngine.GetOptions().ToggleFPS();
	}
}


void Preferences::SetWireframe( bool bWireframe )
{
	m_bWireframe = bWireframe;
	if (( !gSiegeEngine.GetOptions().IsWireframeEnabled() && m_bWireframe ) ||
		( gSiegeEngine.GetOptions().IsWireframeEnabled() && !m_bWireframe ))
	{
		gSiegeEngine.GetOptions().ToggleWireframe();
	}	
}


void Preferences::SetDrawDoors( bool bDrawDoors )
{
	m_bDrawDoors = bDrawDoors;
	if (( !gSiegeEngine.GetOptions().IsDrawDoorsEnabled() && m_bDrawDoors ) ||
		( gSiegeEngine.GetOptions().IsDrawDoorsEnabled() && !m_bDrawDoors ))
	{
		gSiegeEngine.GetOptions().ToggleDoorDrawing();
	}

	gMainFrm.GetPreferenceBar().GetToolBarCtrl().CheckButton( ID_DRAW_DOORS, gPreferences.GetDrawDoors() );	
}



void Preferences::SetDrawFloors( bool bDrawFloors )
{
	m_bDrawFloors = bDrawFloors;
	if (( !gSiegeEngine.GetOptions().DrawFloors() && m_bDrawFloors ) ||
		( gSiegeEngine.GetOptions().DrawFloors() && !m_bDrawFloors ))
	{
		gSiegeEngine.GetOptions().ToggleFloors();
	}
}



void Preferences::SetDrawBoxes( bool bDrawBoxes )
{
	m_bDrawBoxes = bDrawBoxes;
	if (( !gSiegeEngine.GetOptions().NodeBoxes() && m_bDrawBoxes ) ||
		( gSiegeEngine.GetOptions().NodeBoxes() && !m_bDrawBoxes ))
	{
		gSiegeEngine.GetOptions().ToggleNodeBoxes();
	}	
}



void Preferences::SetSLights( bool bSLights )
{
	m_bSLights = bSLights;
	if (( !gSiegeEngine.GetOptions().IsStaticLightingEnabled() && m_bSLights ) ||
		( gSiegeEngine.GetOptions().IsStaticLightingEnabled() && !m_bSLights ))
	{
		gSiegeEngine.GetOptions().ToggleStaticLighting();
	}
}



void Preferences::SetDLights( bool bDLights )
{
	m_bDLights = bDLights;
	if (( !gSiegeEngine.GetOptions().IsDynamicLightingEnabled() && m_bDLights ) ||
		( gSiegeEngine.GetOptions().IsDynamicLightingEnabled() && !m_bDLights ))
	{
		gSiegeEngine.GetOptions().ToggleDynamicLighting();
	}
}



void Preferences::SetSShadows( bool bSShadows )
{
	m_bSShadows = bSShadows;
	if (( !gSiegeEngine.GetOptions().IsStaticShadowsEnabled() && m_bSShadows ) ||
		( gSiegeEngine.GetOptions().IsStaticShadowsEnabled() && !m_bSShadows ))
	{
		gSiegeEngine.GetOptions().ToggleStaticShadows();
	}
}



void Preferences::SetDShadows( bool bDShadows )
{
	m_bDShadows = bDShadows;
	if (( !gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() && m_bDShadows ) ||
		( gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() && !m_bDShadows ))
	{
		gSiegeEngine.GetOptions().ToggleDynamicShadows();
	}
}


void Preferences::SetSShading( bool bSShading )
{
	m_bSShading = bSShading;
	if (( !gSiegeEngine.GetOptions().IsStaticShadingEnabled() && m_bSShading ) ||
		( gSiegeEngine.GetOptions().IsStaticShadingEnabled() && !m_bSShading ))
	{
		gSiegeEngine.GetOptions().ToggleStaticShading();
	}
}


void Preferences::SetDShading( bool bDShading )
{
	m_bDShading = bDShading;
	if (( !gSiegeEngine.GetOptions().IsDynamicShadingEnabled() && m_bDShading ) ||
		( gSiegeEngine.GetOptions().IsDynamicShadingEnabled() && !m_bDShading ))
	{
		gSiegeEngine.GetOptions().ToggleDynamicShading();
	}
}


void Preferences::SetPolyStats( bool bPolystats )
{
	m_bPolystats = bPolystats;
	if (( !gSiegeEngine.GetOptions().ShowPolyStatsEnabled() && m_bPolystats ) ||
		( gSiegeEngine.GetOptions().ShowPolyStatsEnabled() && !m_bPolystats ))
	{
		gSiegeEngine.GetOptions().TogglePolyStats();
	}	
}


void Preferences::SetFPS( bool bFPS ) 
{
	m_bFPS = bFPS;
	if (( !gSiegeEngine.GetOptions().ShowFPSEnabled() && m_bFPS ) ||
		( gSiegeEngine.GetOptions().ShowFPSEnabled() && !m_bFPS ))
	{
		gSiegeEngine.GetOptions().ToggleFPS();
	}	
}


void Preferences::SetDrawWater( bool bDrawWater )
{
	m_bDrawWater = bDrawWater;
	if (( !gSiegeEngine.GetOptions().DrawWater() && m_bDrawWater ) ||
		( gSiegeEngine.GetOptions().DrawWater() && !m_bDrawWater ))
	{
		gSiegeEngine.GetOptions().ToggleWater();
	}	
}


void Preferences::SetDrawLNodes( bool bDrawLNodes )
{
	m_bDrawLNodes = bDrawLNodes;
	if (( !gSiegeEngine.GetOptions().IsDrawLogicalNodes() && m_bDrawLNodes ) ||
		( gSiegeEngine.GetOptions().IsDrawLogicalNodes() && !m_bDrawLNodes ))
	{
		gSiegeEngine.GetOptions().ToggleDrawLogicalNodes();
	}	
	
	gMainFrm.GetPreferenceBar().GetToolBarCtrl().CheckButton( ID_DRAW_LOGICAL_NODES, gPreferences.GetDrawLNodes() );
}


void Preferences::SetDrawNormals( bool bDrawNormals )
{
	m_bDrawNormals = bDrawNormals;
	if (( !gSiegeEngine.GetOptions().IsNormalDrawingEnabled() && m_bDrawNormals ) ||
		( gSiegeEngine.GetOptions().IsNormalDrawingEnabled() && !m_bDrawNormals ))
	{
		gSiegeEngine.GetOptions().ToggleNormalDrawing();
	}		
}


void Preferences::SetUpdateWorld( bool bUpdate )
{
	gWorldState.Update();

	m_bUpdateWorld = bUpdate;
	if ( m_bUpdateWorld )
	{		
		if ( gWorldState.GetPendingState() != WS_SP_INGAME )
		{
			gWorldStateRequest( WS_SP_INGAME );
		}
	}
	else
	{
		if (( gWorldState.GetPendingState() != WS_INIT ) && ( gWorldState.GetCurrentState() != WS_INIT ))
		{
			gWorldStateRequest( WS_INIT );
		}
	}
}


void Preferences::SetUpdateSiegeEngine( bool bUpdate )
{
	m_bUpdateSiegeEngine = bUpdate;
}


void Preferences::SetSnapToGround( bool bSelect )
{ 
	m_bSnapToGround = bSelect;
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_OBJECT_AUTOSNAPTOGROUND, gPreferences.GetSnapToGround() ? MF_CHECKED : MF_UNCHECKED );
	gMainFrm.GetObjectBar().GetToolBarCtrl().CheckButton( ID_OBJECT_AUTOSNAPTOGROUND, gPreferences.GetSnapToGround() );
}


void Preferences::SetDrawDirectionalLights( bool bSelect )
{
	m_bDrawDirectional = bSelect; 
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_LIGHTING_DRAWDIRECTIONALLIGHTS, gPreferences.GetDrawDirectionalLights() ? MF_CHECKED : MF_UNCHECKED );	
	gMainFrm.GetLightBar().GetToolBarCtrl().CheckButton( ID_LIGHTING_DRAWDIRECTIONALLIGHTS, gPreferences.GetDrawDirectionalLights() );	
}


void Preferences::SetPlacementMode( bool bSelect )
{ 
	m_bPlacementMode = bSelect;	
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_OBJECT_PLACEMENTMODE, gPreferences.GetPlacementMode() ? MF_CHECKED : MF_UNCHECKED );		
	gMainFrm.GetObjectBar().GetToolBarCtrl().CheckButton( ID_OBJECT_PLACEMENTMODE, gPreferences.GetPlacementMode() );
}


void Preferences::SetMovementMode( bool bSelect )
{ 
	m_bMovementMode = bSelect;	
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_OBJECT_SELECTIONMODE, gPreferences.GetMovementMode() ? MF_CHECKED : MF_UNCHECKED );		
	gMainFrm.GetObjectBar().GetToolBarCtrl().CheckButton( ID_OBJECT_SELECTIONMODE, gPreferences.GetMovementMode() );
}


void Preferences::SetSelectionLock( bool bSelect )
{ 
	m_bSelectionLock = bSelect;	
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_OBJECT_MODE_SELECTIONLOCKMODE, gPreferences.GetSelectionLock() ? MF_CHECKED : MF_UNCHECKED );
}


void Preferences::SetPlacementLock( bool bSelect )
{ 
	m_bPlacementLock = bSelect;	
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_OBJECT_MODE_PLACEMENTLOCKMODE, gPreferences.GetPlacementLock() ? MF_CHECKED : MF_UNCHECKED );
}


void Preferences::SetPreferenceDisable( bool bDisablePref )
{
	if ( m_bDisablePref == bDisablePref ) 
	{
		return;
	}
	else 
	{
		m_bDisablePref = bDisablePref;
	}

	if ( bDisablePref == true ) 
	{
		if ( GetWireframe() ) 
		{
			gSiegeEngine.GetOptions().ToggleWireframe();
		}
		if ( GetDrawDoors() ) 
		{
			gSiegeEngine.GetOptions().ToggleDoorDrawing();
		}
		if ( GetDrawFloors() ) 
		{
			gSiegeEngine.GetOptions().ToggleFloors();
		}
		if ( GetDrawBoxes() ) 
		{
			gSiegeEngine.GetOptions().ToggleNodeBoxes();
		}
		if ( GetSLights() ) 
		{
			gSiegeEngine.GetOptions().ToggleStaticLighting();
		}
		if ( GetDLights() ) 
		{
			gSiegeEngine.GetOptions().ToggleDynamicLighting();
		}
		if ( GetSShadows() ) 
		{
			gSiegeEngine.GetOptions().ToggleStaticShadows();
		}
		if ( GetDShadows() ) 
		{
			gSiegeEngine.GetOptions().ToggleDynamicShadows();
		}
		if ( GetSShading() ) 
		{
			gSiegeEngine.GetOptions().ToggleStaticShading();
		}
		if ( GetDShading() ) 
		{
			gSiegeEngine.GetOptions().ToggleDynamicShading();
		}
		if ( GetPolyStats() ) 
		{
			gSiegeEngine.GetOptions().TogglePolyStats();
		}
		if ( GetDrawNormals() )
		{
			gSiegeEngine.GetOptions().ToggleNormalDrawing();
		}
		if ( GetDrawLNodes() )
		{
			gSiegeEngine.GetOptions().ToggleDrawLogicalNodes();
		}
		if ( GetDrawWater() )
		{
			gSiegeEngine.GetOptions().ToggleWater();
		}		
	}
}


bool Preferences::Load()
{
	bool bFoundPreferences = true;
	// STEP 1:
	// Load the main window placement settings out of the registry
	{
		HKEY hKey = NULL;
		RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Gas Powered Games\\SiegeEditor\\Settings", 0, KEY_READ, &hKey );

		DWORD dwType = 0;
		DWORD dwSize = 0;

		WINDOWPLACEMENT wp;
		RegQueryValueEx( hKey, "window_placement", 0, &dwType, (unsigned char *)&wp, &dwSize );		
		if ( RegQueryValueEx( hKey, "window_placement", 0, &dwType, (unsigned char *)&wp, &dwSize ) == ERROR_SUCCESS ) 
		{
			gMainFrm.SetWindowPlacement( &wp );
		}

		RegQueryValueEx( hKey, "preview_window_placement", 0, &dwType, (unsigned char *)&m_PreviewWindowPlacement, &dwSize );
		
		RegCloseKey( hKey );
	}

	// STEP 2:
	// Load the other settings 

	FuelDB * pPreferenceDb = gFuelSys.AddTextDb( "preferences" );
	gpstring sDir = gConfig.ResolvePathVars( "%se_user_path%" );
	pPreferenceDb->Init( FuelDB::OPTION_READ | FuelDB::OPTION_WRITE | FuelDB::OPTION_JIT_READ, sDir, sDir );
	FuelHandle hPreferences( "::preferences:preferences" );
	if ( hPreferences )
	{
		FuelHandle hWindowSettings = hPreferences->GetChildBlock( "window_settings" );
		if ( hWindowSettings )
		{
			int value = 0;
			hWindowSettings->Get( "splitter1_row0", value );
			gMainFrm.GetSplitterWnd().SetRowInfo( 0, value, 0 );

			hWindowSettings->Get( "splitter1_col0", value );
			gMainFrm.GetSplitterWnd().SetColumnInfo( 0, value, 0 );

			hWindowSettings->Get( "splitter2_row0", value );
			gMainFrm.GetConsoleSplitterWnd().SetRowInfo( 0, value, 0 );

			hWindowSettings->Get( "splitter2_col0", value );
			gMainFrm.GetConsoleSplitterWnd().SetColumnInfo( 0, value, 0 );

			hWindowSettings->Get( "splitter2_row1", value );
			gMainFrm.GetConsoleSplitterWnd().SetRowInfo( 1, value, 0 );

			gMainFrm.GetSplitterWnd().RecalcLayout();
			gMainFrm.GetConsoleSplitterWnd().RecalcLayout();
		}		

		FuelHandle hGeneral = hPreferences->GetChildBlock( "general_settings" );
		if ( hGeneral )
		{
			hGeneral->Get( "wireframe", m_bWireframe );			
			hGeneral->Get( "draw_floors", m_bDrawFloors );			
			hGeneral->Get( "draw_boxes", m_bDrawBoxes );
			hGeneral->Get( "draw_door_numbers", m_bDrawDoorNums );
			hGeneral->Get( "draw_water", m_bDrawWater );
			hGeneral->Get( "draw_stitch_doors", m_bDrawStitchDoors );
			hGeneral->Get( "hit_floor_only", m_bHitFloorOnly );		
			hGeneral->Get( "slights", m_bSLights );
			hGeneral->Get( "dlights", m_bDLights );
			hGeneral->Get( "sshadows", m_bSShadows );
			hGeneral->Get( "dshadows", m_bDShadows );
			hGeneral->Get( "sshading", m_bSShading );
			hGeneral->Get( "dshading", m_bDShading );
			hGeneral->Get( "poly_stats", m_bPolystats );
			hGeneral->Get( "fps", m_bFPS );
			hGeneral->Get( "draw_normals", m_bDrawNormals );
			hGeneral->Get( "update_effects", m_bUpdateEffects );				
			hGeneral->Get( "update_world", m_bUpdateWorld );
			hGeneral->Get( "update_siege_engine", m_bUpdateSiegeEngine );
			hGeneral->Get( "update_weather", m_bUpdateWeather );
			hGeneral->Get( "update_sound", m_bUpdateSound );
			hGeneral->Get( "selection_lock", m_bSelectionLock );
			hGeneral->Get( "placement_lock", m_bPlacementLock );
			hGeneral->Get( "snap_to_ground", m_bSnapToGround );			
			hGeneral->Get( "draw_directional", m_bDrawDirectional );					
			hGeneral->Get( "show_view_bar", m_bShowViewBar );
			hGeneral->Get( "show_node_bar", m_bShowNodeBar );
			hGeneral->Get( "show_object_bar", m_bShowObjectBar );
			hGeneral->Get( "show_light_bar", m_bShowLightBar );
			hGeneral->Get( "show_dlg_bar", m_bShowDlgBar );
			hGeneral->Get( "show_camera_bar", m_bShowCameraBar );
			hGeneral->Get( "show_mode_bar", m_bShowModeBar );	
			hGeneral->Get( "save_stitch", m_bSaveStitch );
			hGeneral->Get( "save_nodes", m_bSaveNodes );
			hGeneral->Get( "save_lights", m_bSaveLights );
			hGeneral->Get( "save_hotpoints", m_bSaveHotpoints );
			hGeneral->Get( "save_game_objects", m_bSaveGameObjects );
			hGeneral->Get( "save_start_positions", m_bSaveStartingPos );
			hGeneral->Get( "right_click_scroll", m_bRightScroll );
			hGeneral->Get( "disable_prefs", m_bDisablePref );
			hGeneral->Get( "custom_tab_active", m_bCustomViewActive );			
			hGeneral->Get( "lock_custom_view", m_bLockCustomView );
			hGeneral->Get( "draw_commands", m_bDrawCommands );
			hGeneral->Get( "draw_logical_flags", m_bDrawLogicalFlags );
			hGeneral->Get( "draw_light_huds", m_bDrawLightHuds );
			hGeneral->Get( "select_interactive", m_bSelectInteractive );
			hGeneral->Get( "select_non_interactive", m_bSelectNoninteractive );
			hGeneral->Get( "select_gizmos", m_bSelectGizmos );		
			hGeneral->Get( "select_non_gizmos", m_bSelectNonGizmos );
			hGeneral->Get( "select_all", m_bSelectAll );
			hGeneral->Get( "decal_horizontal", m_decalHoriz );
			hGeneral->Get( "decal_vertical", m_decalVert );	
			hGeneral->Get( "rotation_increment", m_rotationIncrement );
			hGeneral->Get( "camera_zoom_step", m_cameraZoomStep );
			hGeneral->Get( "camera_rotate_step", m_cameraRotateStep );
			hGeneral->Get( "camera_translate_step", m_cameraTranslateStep );
			hGeneral->Get( "zoom_rate", m_zoomRate );
			hGeneral->Get( "camera_snap_increment", m_cameraSnapIncrement );
			hGeneral->Get( "hud_selected", m_bHudSelected );
			hGeneral->Get( "restrict_camera_to_game", m_bRestrictCameraToGame );
			hGeneral->Get( "draw_camera_target", m_bDrawCameraTarget );	
			hGeneral->Get( "preview_wireframe", m_bPreviewWireframe );			
			hGeneral->Get( "preview_color", m_dwPreviewColor );			
			hGeneral->Get( "clear_color", m_dwClearColor );
			SetClearColor( m_dwClearColor );	
			
			int iValue = 0;
			hGeneral->Get( "debug_hud_options", iValue );
			gWorldOptions.ClearDebugHudOptions( gWorldOptions.GetDebugHudOptions() );
			gWorldOptions.SetDebugHudOptions( (eDebugHudOptions)iValue, true );

			hGeneral->Get( "debug_hud_group", iValue );				
			gWorldOptions.SetDebugHudGroup( (eDebugHudGroup)iValue );
			if ( (eDebugHudGroup)iValue != DHO_NONE )
			{
				SetDrawDebugHuds( true );
			}
			hGeneral->Get( "debug_hud_last_group", iValue );
			m_lastHudGroup = (eDebugHudGroup)iValue;
			if ( m_lastHudGroup == DHG_NONE )
			{
				m_lastHudGroup = DHG_ALL;
			}

			hGeneral->Get( "content_display", iValue );			
			SetContentDisplay( (eDisplayType)iValue );

			hGeneral->Get( "move_lock_mode", iValue );
			SetMoveLockMode( (eMoveLockMode)iValue );

			gpstring sNodeSet;
			hGeneral->Get( "node_set", sNodeSet );
			if ( !sNodeSet.empty() )
			{
				gEditorTerrain.SetNodeSetCharacters( sNodeSet );
			}

			bool bValue = false; 
			hGeneral->Get( "next_random_pitch", bValue );
			gEditorObjects.GetNextObjectSettings().bRandPitch = bValue;

			hGeneral->Get( "next_random_yaw", bValue );
			gEditorObjects.GetNextObjectSettings().bRandYaw = bValue;

			hGeneral->Get( "next_random_roll", bValue );
			gEditorObjects.GetNextObjectSettings().bRandRoll = bValue;

			hGeneral->Get( "next_random_scale", bValue );
			gEditorObjects.GetNextObjectSettings().bRandomScale = bValue;

			float fValue = 0.0f;
			hGeneral->Get( "next_random_scale_min", fValue );
			gEditorObjects.GetNextObjectSettings().minMultiplier = fValue;

			hGeneral->Get( "next_random_scale_max", fValue );			
			gEditorObjects.GetNextObjectSettings().maxMultiplier = fValue;

			hGeneral->Get( "precision_amount", fValue );
			gGizmoManager.SetPrecisionAmount( fValue );		
				
			// Special
			hGeneral->Get( "draw_doors", m_bDrawDoors );
			hGeneral->Get( "ignore_vertex_color", m_bIgnoreVertexColor );				
			hGeneral->Get( "show_tuning_points", m_bShowTuningPoints );
			hGeneral->Get( "show_objects", m_bShowObjects );
			hGeneral->Get( "show_lights", m_bShowLights );
			hGeneral->Get( "show_gizmos", m_bShowGizmos );
			hGeneral->Get( "placement_mode", m_bPlacementMode );
			hGeneral->Get( "movement_mode", m_bMovementMode );								
			hGeneral->Get( "paint_select_nodes", m_bPaintSelectNodes );
			hGeneral->Get( "draw_logical_nodes", m_bDrawLNodes );
			hGeneral->Get( "mode_objects", m_bModeObjects );
			hGeneral->Get( "mode_lights", m_bModeLights );
			hGeneral->Get( "mode_gizmos", m_bModeGizmos );
			hGeneral->Get( "mode_nodes", m_bModeNodes );
			hGeneral->Get( "mode_decals", m_bModeDecals );
			hGeneral->Get( "mode_tuning_points", m_bModeTuningPoints );
			hGeneral->Get( "mode_logical_nodes", m_bModeLogicalNodes );
			hGeneral->Get( "mode_node_placement", m_bModeNodePlacement );
			hGeneral->Get( "rotate_mode", m_bRotateMode );	

			hGeneral->Get( "grid_enable", m_bEnableGrid );
			hGeneral->Get( "grid_size", m_GridSize );
			hGeneral->Get( "grid_interval", m_GridInterval );
			hGeneral->Get( "grid_color", m_GridColor );
			hGeneral->Get( "grid_draw_label_intervals",	m_bDrawLabelIntervals );
			hGeneral->Get( "grid_draw_label_amount", m_bDrawLabelAmount );
			hGeneral->Get( "grid_label_amount", m_DrawLabelAmount );

			hGeneral->Get( "meter_points_draw", m_bDrawMeterPoints );
			hGeneral->Get( "meter_point_color", m_MeterPointColor );
			hGeneral->Get( "meter_point_show_distance", m_bShowMeterPointDistance );

			// Visual Node Placement Settings
			hGeneral->Get( "visual_draw_dest_id", m_bVisualDrawDestId );
			hGeneral->Get( "visual_draw_door_numbers", m_bVisualDrawDoorNumbers );
			hGeneral->Get( "visual_draw_mesh_box", m_bVisualDrawMeshBox );
			hGeneral->Get( "visual_allow_invalid_match", m_bAllowInvalidMatch );
			hGeneral->Get( "visual_snap_sensitivity", m_VisualSnapSensitivity );
			hGeneral->Get( "visual_valid_color", m_VisualValidColor );
			hGeneral->Get( "visual_invalid_color", m_VisualInvalidColor );
			hGeneral->Get( "visual_dest_door_color", m_VisualDestDoorColor );
			hGeneral->Get( "visual_rotate_orient", m_bVisualRotateOrient );
			hGeneral->Get( "visual_snap_delay", m_VisualSnapDelay );
			hGeneral->Get( "visual_allow_door_guess", m_bVisualAllowDoorGuess );

			gpstring sOrigin;
			hGeneral->Get( "grid_origin", sOrigin );
			stringtool::GetDelimitedValue( sOrigin, ',', 0, m_GridOrigin.x );
			stringtool::GetDelimitedValue( sOrigin, ',', 1, m_GridOrigin.y );
			stringtool::GetDelimitedValue( sOrigin, ',', 2, m_GridOrigin.z );
		
			gpstring sHidden;
			hGeneral->Get( "hidden_components", sHidden );			
			int numStrings = stringtool::GetNumDelimitedStrings( sHidden, ',' );
			for ( int ns = 0; ns != numStrings; ++ns )
			{
				gpstring sComponent;
				stringtool::GetDelimitedValue( sHidden, ',', ns, sComponent );
				gEditorObjects.AddHiddenComponent( sComponent );
			}					
		}
	}
	else
	{
		// Do all default handling here
		gSiegeEditor.ShowMaximized();

		// Splitter window handling
		{
			int curY1 = 0, curY2 = 0;
			int minY1 = 0, minY2 = 0;
			gMainFrm.GetConsoleSplitterWnd().GetRowInfo( 0, curY1, minY1 );
			gMainFrm.GetConsoleSplitterWnd().GetRowInfo( 1, curY2, minY2 );
			if ( (curY1+curY2) > 100 )
			{
				gMainFrm.GetConsoleSplitterWnd().SetRowInfo( 0, curY1+curY2-100, 0 );
			}

			gMainFrm.GetConsoleSplitterWnd().SetRowInfo( 1, 100, 0 );

			gMainFrm.GetSplitterWnd().RecalcLayout();
			gMainFrm.GetConsoleSplitterWnd().RecalcLayout();
		}		

		gWorldOptions.ClearDebugHudOptions( gWorldOptions.GetDebugHudOptions() );
		gWorldOptions.SetDebugHudOptions( DHO_ALL, true );				
		gWorldOptions.SetDebugHudGroup( DHG_MISC );
		SetDrawDebugHuds( true );		
		SetHudSelected( true );
		bFoundPreferences = false;
	}

	SetWireframe( m_bWireframe );
	SetDrawFloors( m_bDrawFloors );
	SetDrawBoxes( m_bDrawBoxes );
	SetDrawDoorNumbers( m_bDrawDoorNums );
	SetDrawWater( m_bDrawWater );
	SetDrawStitchDoors( m_bDrawStitchDoors );
	SetHitFloorsOnly( m_bHitFloorOnly );
	SetSLights( m_bSLights );
	SetDLights( m_bDLights );
	SetSShadows( m_bSShadows );
	SetDShadows( m_bDShadows );
	SetSShading( m_bSShading );
	SetDShading( m_bDShading );
	SetPolyStats( m_bPolystats );
	SetFPS( m_bFPS );
	SetDrawNormals( m_bDrawNormals );
	SetUpdateEffects( m_bUpdateEffects );
	SetUpdateWorld( m_bUpdateWorld );
	SetUpdateSiegeEngine( m_bUpdateSiegeEngine );
	SetUpdateWeather( m_bUpdateWeather );
	SetUpdateSound( m_bUpdateSound );
	SetSelectionLock( m_bSelectionLock );
	SetPlacementLock( m_bPlacementLock );
	SetSnapToGround( m_bSnapToGround );
	SetDrawDirectionalLights( m_bDrawDirectional );
	SetShowViewBar( m_bShowViewBar );
	SetShowNodeBar( m_bShowNodeBar );
	SetShowObjectBar( m_bShowObjectBar );
	SetShowLightBar( m_bShowLightBar );
	SetShowDlgBar( m_bShowDlgBar );
	SetShowCameraBar( m_bShowCameraBar );
	SetShowModeBar( m_bShowModeBar );
	SetSaveStitch( m_bSaveStitch );
	SetSaveNodes( m_bSaveNodes );
	SetSaveLights( m_bSaveLights );
	SetSaveHotpoints( m_bSaveHotpoints );
	SetSaveGameObjects( m_bSaveGameObjects );
	SetSaveStartingPos( m_bSaveStartingPos );
	SetRightClickScroll( m_bRightScroll );
	SetPreferenceDisable( m_bDisablePref );
	SetCustomTabActive( m_bCustomViewActive );
	SetLockCustomView( m_bLockCustomView );
	SetDrawCommands( m_bDrawCommands );
	SetDrawLogicalFlags( m_bDrawLogicalFlags );
	SetDrawLightHuds( m_bDrawLightHuds );
	SetSelectInteractive( m_bSelectInteractive );
	SetSelectNoninteractive( m_bSelectNoninteractive );
	SetSelectGizmos( m_bSelectGizmos );
	SetSelectNonGizmos( m_bSelectNonGizmos );
	SetSelectAll( m_bSelectAll );
	SetDecalHorizontal( m_decalHoriz );
	SetDecalVertical( m_decalVert );
	SetRotationIncrement( m_rotationIncrement );
	SetCameraZoomStep( m_cameraZoomStep );
	SetCameraRotateStep( m_cameraRotateStep );
	SetCameraTranslateStep( m_cameraTranslateStep );
	SetZoomRate( m_zoomRate );
	SetCameraSnapIncrement( m_cameraSnapIncrement );
	SetHudSelected( m_bHudSelected );

	SetDrawDoors( m_bDrawDoors );
	SetIgnoreVertexColor( m_bIgnoreVertexColor );
	SetShowTuningPoints( m_bShowTuningPoints );
	SetShowObjects( m_bShowObjects );
	SetShowLights( m_bShowLights );
	SetShowGizmos( m_bShowGizmos );
	SetPlacementMode( m_bPlacementMode );
	SetMovementMode( m_bMovementMode );
	SetPaintSelectNodes( m_bPaintSelectNodes );
	SetDrawLNodes( m_bDrawLNodes );
	SetModeObjects( m_bModeObjects );
	SetModeGizmos( m_bModeGizmos );
	SetModeLights( m_bModeLights );
	SetModeNodes( m_bModeNodes );
	SetModeDecals( m_bModeDecals );
	SetModeTuningPoints( m_bModeTuningPoints );
	SetModeLogicalNodes( m_bModeLogicalNodes );
	SetModeNodePlacement( m_bModeNodePlacement );
	SetObjectRotateMode( m_bRotateMode );
	gLeftView.SwitchContentDisplay();
	
	return bFoundPreferences;
}


void Preferences::Save()
{
	// STEP 1:
	// Save the window placement settings in the registry.
	{
		HKEY hKey = NULL;
		RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Gas Powered Games\\SiegeEditor\\Settings", 0, KEY_WRITE, &hKey );
		
		// Set window placement properties like minimized, maximized, etc.
		WINDOWPLACEMENT wp;
		wp.length = sizeof(WINDOWPLACEMENT);
		gMainFrm.GetWindowPlacement( &wp );
		RegSetValueEx( hKey, "window_placement", 0, REG_BINARY, (unsigned char *)&wp, sizeof( WINDOWPLACEMENT ) );
		
		if ( m_PreviewWindowPlacement.length != 0 )
		{
			RegSetValueEx( hKey, "preview_window_placement", 0, REG_BINARY, (unsigned char *)&m_PreviewWindowPlacement, sizeof( WINDOWPLACEMENT ) );
		}

		RegCloseKey( hKey );
	}

	// STEP 2:
	// Save the rest of the settings in an easily editable gas file.
	FuelHandle hEditor( "::preferences:root" );
	if ( hEditor )
	{
		// Delete the old preferences
		FuelHandle hPreferences = hEditor->GetChildBlock( "preferences" );
		if ( hPreferences )
		{
			hEditor->DestroyChildBlock( hPreferences );
		}

		// Create the new preferences
		hPreferences = hEditor->CreateChildBlock( "preferences", "preferences.gas" );

		if ( hPreferences )
		{
			// Splitter Window Settings
			FuelHandle hWindowSettings = hPreferences->CreateChildBlock( "window_settings" );
			if ( hWindowSettings )
			{
				{			
					int row_0		= 0;
					int row_0_min	= 0;
					gMainFrm.GetSplitterWnd().GetRowInfo( 0, row_0, row_0_min );
					hWindowSettings->Set( "splitter1_row0", row_0 );

					int col_0		= 0;
					int col_0_min	= 0;
					gMainFrm.GetSplitterWnd().GetColumnInfo( 0, col_0, col_0_min );		
					hWindowSettings->Set( "splitter1_col0", col_0 );
				}
				{
					int row_0		= 0;
					int row_0_min	= 0;
					gMainFrm.GetConsoleSplitterWnd().GetRowInfo( 0, row_0, row_0_min );
					hWindowSettings->Set( "splitter2_row0", row_0 );				

					int col_0		= 0;
					int col_0_min	= 0;
					gMainFrm.GetConsoleSplitterWnd().GetColumnInfo( 0, col_0, col_0_min );
					hWindowSettings->Set( "splitter2_col0", col_0 );				

					int row_1		= 0;
					int row_1_min	= 0;
					gMainFrm.GetConsoleSplitterWnd().GetRowInfo( 1, row_1, row_1_min );		
					hWindowSettings->Set( "splitter2_row1", row_1 );				
				}
			}

			// General Settings
			FuelHandle hGeneral = hPreferences->CreateChildBlock( "general_settings" );
			if ( hGeneral )
			{
				hGeneral->Set( "wireframe", m_bWireframe );
				hGeneral->Set( "draw_floors", m_bDrawFloors );
				hGeneral->Set( "draw_boxes", m_bDrawBoxes );
				hGeneral->Set( "draw_door_numbers", m_bDrawDoorNums );
				hGeneral->Set( "draw_water", m_bDrawWater );		
				hGeneral->Set( "draw_stitch_doors", m_bDrawStitchDoors );
				hGeneral->Set( "hit_floor_only", m_bHitFloorOnly );
				hGeneral->Set( "node_set", gEditorTerrain.GetNodeSetCharacters() );

				hGeneral->Set( "slights", m_bSLights );
				hGeneral->Set( "dlights", m_bDLights );
				hGeneral->Set( "sshadows", m_bSShadows );
				hGeneral->Set( "dshadows", m_bDShadows );
				hGeneral->Set( "sshading", m_bSShading );
				hGeneral->Set( "dshading", m_bDShading );
				hGeneral->Set( "poly_stats", m_bPolystats );
				hGeneral->Set( "fps", m_bFPS );
				hGeneral->Set( "draw_normals", m_bDrawNormals );
				
				hGeneral->Set( "update_effects", m_bUpdateEffects );				
				hGeneral->Set( "update_world", m_bUpdateWorld );
				hGeneral->Set( "update_siege_engine", m_bUpdateSiegeEngine );
				hGeneral->Set( "update_weather", m_bUpdateWeather );
				hGeneral->Set( "update_sound", m_bUpdateSound );
				
				hGeneral->Set( "selection_lock", m_bSelectionLock );
				hGeneral->Set( "placement_lock", m_bPlacementLock );
				hGeneral->Set( "snap_to_ground", m_bSnapToGround );
				hGeneral->Set( "move_lock_mode", m_MoveLockMode );

				hGeneral->Set( "draw_directional", m_bDrawDirectional );
				hGeneral->Set( "content_display", m_content_display );

				hGeneral->Set( "show_view_bar", m_bShowViewBar );
				hGeneral->Set( "show_node_bar", m_bShowNodeBar );
				hGeneral->Set( "show_object_bar", m_bShowObjectBar );
				hGeneral->Set( "show_light_bar", m_bShowLightBar );
				hGeneral->Set( "show_dlg_bar", m_bShowDlgBar );
				hGeneral->Set( "show_camera_bar", m_bShowCameraBar );
				hGeneral->Set( "show_mode_bar", m_bShowModeBar );
				
				hGeneral->Set( "save_stitch", m_bSaveStitch );
				hGeneral->Set( "save_nodes", m_bSaveNodes );
				hGeneral->Set( "save_lights", m_bSaveLights );
				hGeneral->Set( "save_hotpoints", m_bSaveHotpoints );
				hGeneral->Set( "save_game_objects", m_bSaveGameObjects );
				hGeneral->Set( "save_start_positions", m_bSaveStartingPos );

				hGeneral->Set( "right_click_scroll", m_bRightScroll );
				hGeneral->Set( "disable_prefs", m_bDisablePref );

				hGeneral->Set( "custom_tab_active", m_bCustomViewActive );				
				hGeneral->Set( "lock_custom_view", m_bLockCustomView );

				hGeneral->Set( "draw_commands", m_bDrawCommands );
				hGeneral->Set( "draw_logical_flags", m_bDrawLogicalFlags );
				hGeneral->Set( "draw_light_huds", m_bDrawLightHuds );

				hGeneral->Set( "next_random_pitch", gEditorObjects.GetNextObjectSettings().bRandPitch );
				hGeneral->Set( "next_random_yaw", gEditorObjects.GetNextObjectSettings().bRandYaw );
				hGeneral->Set( "next_random_roll", gEditorObjects.GetNextObjectSettings().bRandRoll );
				hGeneral->Set( "next_random_scale", gEditorObjects.GetNextObjectSettings().bRandomScale );
				hGeneral->Set( "next_random_scale_min", gEditorObjects.GetNextObjectSettings().minMultiplier );
				hGeneral->Set( "next_random_scale_max", gEditorObjects.GetNextObjectSettings().maxMultiplier );

				hGeneral->Set( "select_interactive", m_bSelectInteractive );
				hGeneral->Set( "select_non_interactive", m_bSelectNoninteractive );
				hGeneral->Set( "select_gizmos", m_bSelectGizmos );
				hGeneral->Set( "select_non_gizmos", m_bSelectNonGizmos );
				hGeneral->Set( "select_all", m_bSelectAll );

				hGeneral->Set( "decal_horizontal", m_decalHoriz );
				hGeneral->Set( "decal_vertical", m_decalVert );

				hGeneral->Set( "precision_amount", gGizmoManager.GetPrecisionAmount() );
				hGeneral->Set( "rotation_increment", m_rotationIncrement );

				hGeneral->Set( "camera_zoom_step", m_cameraZoomStep );
				hGeneral->Set( "camera_rotate_step", m_cameraRotateStep );
				hGeneral->Set( "camera_translate_step", m_cameraTranslateStep );
				hGeneral->Set( "zoom_rate", m_zoomRate );
				hGeneral->Set( "camera_snap_increment", m_cameraSnapIncrement );				
				
				hGeneral->Set( "debug_hud_options", (int)gWorldOptions.GetDebugHudOptions() );
				hGeneral->Set( "debug_hud_group", (int)gWorldOptions.GetDebugHudGroup() );
				hGeneral->Set( "debug_hud_last_group", (int)m_lastHudGroup );
				hGeneral->Set( "hud_selected", m_bHudSelected );

				hGeneral->Set( "restrict_camera_to_game", m_bRestrictCameraToGame );
				hGeneral->Set( "draw_camera_target", m_bDrawCameraTarget );
					
				// Special
				hGeneral->Set( "draw_doors", m_bDrawDoors );
				hGeneral->Set( "ignore_vertex_color", m_bIgnoreVertexColor );				
				hGeneral->Set( "show_tuning_points", m_bShowTuningPoints );
				hGeneral->Set( "show_objects", m_bShowObjects );
				hGeneral->Set( "show_lights", m_bShowLights );
				hGeneral->Set( "show_gizmos", m_bShowGizmos );
				hGeneral->Set( "placement_mode", m_bPlacementMode );
				hGeneral->Set( "movement_mode", m_bMovementMode );								
				hGeneral->Set( "paint_select_nodes", m_bPaintSelectNodes );
				hGeneral->Set( "draw_logical_nodes", m_bDrawLNodes );
				hGeneral->Set( "mode_objects", m_bModeObjects );
				hGeneral->Set( "mode_lights", m_bModeLights );
				hGeneral->Set( "mode_gizmos", m_bModeGizmos );
				hGeneral->Set( "mode_nodes", m_bModeNodes );
				hGeneral->Set( "mode_decals", m_bModeDecals );
				hGeneral->Set( "mode_tuning_points", m_bModeTuningPoints );
				hGeneral->Set( "mode_logical_nodes", m_bModeLogicalNodes );
				hGeneral->Set( "mode_node_placement", m_bModeNodePlacement );
				hGeneral->Set( "rotate_mode", m_bRotateMode );
				
				hGeneral->Set( "preview_wireframe", m_bPreviewWireframe );				
				hGeneral->Set( "preview_color", m_dwPreviewColor );

				hGeneral->Set( "clear_color", m_dwClearColor );

				hGeneral->Set( "grid_enable", m_bEnableGrid );
				hGeneral->Set( "grid_size", m_GridSize );
				hGeneral->Set( "grid_interval", m_GridInterval );
				hGeneral->Set( "grid_color", m_GridColor );
				hGeneral->Set( "grid_draw_label_intervals",	m_bDrawLabelIntervals );
				hGeneral->Set( "grid_draw_label_amount", m_bDrawLabelAmount );
				hGeneral->Set( "grid_label_amount", m_DrawLabelAmount );

				hGeneral->Set( "meter_points_draw", m_bDrawMeterPoints );
				hGeneral->Set( "meter_point_color", m_MeterPointColor );
				hGeneral->Set( "meter_point_show_distance", m_bShowMeterPointDistance );

				hGeneral->Set( "visual_draw_dest_id", m_bVisualDrawDestId );
				hGeneral->Set( "visual_draw_door_numbers", m_bVisualDrawDoorNumbers );
				hGeneral->Set( "visual_draw_mesh_box", m_bVisualDrawMeshBox );
				hGeneral->Set( "visual_allow_invalid_match", m_bAllowInvalidMatch );
				hGeneral->Set( "visual_snap_sensitivity", m_VisualSnapSensitivity );
				hGeneral->Set( "visual_valid_color", m_VisualValidColor );
				hGeneral->Set( "visual_invalid_color", m_VisualInvalidColor );
				hGeneral->Set( "visual_dest_door_color", m_VisualDestDoorColor );
				hGeneral->Set( "visual_rotate_orient", m_bVisualRotateOrient );
				hGeneral->Set( "visual_snap_delay", m_VisualSnapDelay );
				hGeneral->Set( "visual_allow_door_guess", m_bVisualAllowDoorGuess );

				gpstring sOrigin;
				sOrigin.assignf( "%f,%f,%f", m_GridOrigin.x, m_GridOrigin.y, m_GridOrigin.z );
				hGeneral->Set( "grid_origin", sOrigin );
				
				StringVec hiddenComponents = gEditorObjects.GetHiddenComponents();
				StringVec::iterator iHidden;
				int ssize = 0;
				gpstring sMasterHidden;
				for ( iHidden = hiddenComponents.begin(); iHidden != hiddenComponents.end(); ++iHidden )
				{
					if ( sMasterHidden.size() == 0 )
					{
						sMasterHidden.appendf( "%s", (*iHidden).c_str() );
					}
					else
					{
						sMasterHidden.appendf( ",%s", (*iHidden).c_str() );
					}			
				}				
				hGeneral->Set( "hidden_components", sMasterHidden );
				
			}		
		}

		hPreferences->GetDB()->SaveChanges();
	}

	// remove import db
	gFuelSys.Remove( FuelHandle( "::preferences:root" )->GetDB() );
}


bool Preferences::CanPlace()
{
	if ( GetPlacementMode() == false )
	{
		return false;
	}

	if ( GetPlacementLock() )
	{
		if ( GetKeyState( VK_MENU ) & 0x80 )
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	return true;
}


bool Preferences::CanSelect()
{
	if ( GetSelectionLock() )
	{
		int test = GetKeyState( VK_CONTROL );
		if ( GetKeyState( VK_CONTROL ) & 0x80 )
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	return true;
}


void Preferences::SetShowObjects( bool bShow )			
{ 
	m_bShowObjects = bShow;
	gGizmoManager.SetShowObjects( bShow );
	gWorldOptions.SetShowObjects( bShow );
	
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_VIEW_OBJECTS, gPreferences.GetShowObjects() ? MF_CHECKED : MF_UNCHECKED );						
	gMainFrm.GetViewBar().GetToolBarCtrl().CheckButton( ID_VIEW_OBJECTS, gPreferences.GetShowObjects() );
}


void Preferences::SetShowLights( bool bShow )
{
	m_bShowLights = bShow; 	

	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_VIEW_LIGHTS, gPreferences.GetShowLights() ? MF_CHECKED : MF_UNCHECKED );		
	gMainFrm.GetViewBar().GetToolBarCtrl().CheckButton( ID_VIEW_LIGHTS, gPreferences.GetShowLights() );
}

void Preferences::SetShowGizmos( bool bShow )			
{ 
	m_bShowGizmos = bShow; 
	gGizmoManager.ShowGizmosOfType( GIZMO_OBJECTGIZMO, bShow );
	gGizmoManager.ShowGizmosOfType( GIZMO_HOTPOINT, bShow );
	gGizmoManager.ShowGizmosOfType( GIZMO_DECAL, bShow );
	gGizmoManager.ShowGizmosOfType( GIZMO_STARTINGPOSITION, bShow );

	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_VIEW_GIZMOS, gPreferences.GetShowGizmos() ? MF_CHECKED : MF_UNCHECKED );		
	gMainFrm.GetViewBar().GetToolBarCtrl().CheckButton( ID_VIEW_GIZMOS, gPreferences.GetShowGizmos() );
}

void Preferences::SetShowTuningPoints( bool bShow )
{
	m_bShowTuningPoints = bShow;
	gGizmoManager.ShowGizmosOfType( GIZMO_TUNINGPOINT, bShow );

	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_VIEW_TUNINGPOINTS, gPreferences.GetShowTuningPoints() ? MF_CHECKED : MF_UNCHECKED );
	gMainFrm.GetViewBar().GetToolBarCtrl().CheckButton( ID_VIEW_TUNINGPOINTS, gPreferences.GetShowTuningPoints() );
}


void Preferences::SetModeObjects( bool bMode )
{ 
	m_bModeObjects = bMode; 
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_VIEW_MODE_OBJECTS, gPreferences.GetModeObjects() ? MF_CHECKED : MF_UNCHECKED );				
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_VIEW_MODE_OBJECTS, gPreferences.GetModeObjects() );
}


void Preferences::SetModeLights( bool bMode )
{ 
	m_bModeLights = bMode; 
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_VIEW_MODE_LIGHTS, gPreferences.GetModeLights() ? MF_CHECKED : MF_UNCHECKED );		
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_VIEW_MODE_LIGHTS, gPreferences.GetModeLights() );
}


void Preferences::SetModeGizmos( bool bMode )
{ 
	m_bModeGizmos = bMode; 
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_VIEW_MODE_GIZMOS, gPreferences.GetModeGizmos() ? MF_CHECKED : MF_UNCHECKED );		
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_VIEW_MODE_GIZMOS, gPreferences.GetModeGizmos() );
}


void Preferences::SetModeNodes( bool bMode )
{
	m_bModeNodes = bMode; 
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_VIEW_MODE_NODE, gPreferences.GetModeNodes() ? MF_CHECKED : MF_UNCHECKED );		
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_VIEW_MODE_NODE, gPreferences.GetModeNodes() );
}


void Preferences::SetModeDecals( bool bMode )
{ 
	m_bModeDecals = bMode; 
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_VIEW_MODE_DECALS, gPreferences.GetModeDecals() ? MF_CHECKED : MF_UNCHECKED );	
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_VIEW_MODE_DECALS, gPreferences.GetModeDecals() );
}


void Preferences::SetModeLogicalNodes( bool bMode )
{ 
	m_bModeLogicalNodes = bMode; 
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_VIEW_MODE_LOGICALNODES, gPreferences.GetModeLogicalNodes() ? MF_CHECKED : MF_UNCHECKED );		
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_VIEW_MODE_LOGICALNODES, gPreferences.GetModeLogicalNodes() );
	if ( !m_bModeLogicalNodes )
	{
		gEditorTerrain.DeselectAllLogicalIds();
	}
}


void Preferences::SetModeTuningPoints( bool bMode )
{ 
	m_bModeTuningPoints = bMode; 
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_VIEW_MODE_TUNINGPOINTS, gPreferences.GetModeTuningPoints() ? MF_CHECKED : MF_UNCHECKED );		
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_VIEW_MODE_TUNINGPOINTS, gPreferences.GetModeTuningPoints() );
}


void Preferences::SetModeNodePlacement( bool bMode )
{
	m_bModeNodePlacement = bMode; 
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_VIEW_MODE_NODEPLACEMENT, gPreferences.GetModeNodePlacement() ? MF_CHECKED : MF_UNCHECKED );		
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_VIEW_MODE_NODEPLACEMENT, gPreferences.GetModeNodePlacement() );
}


void Preferences::SetPaintSelectNodes( bool bSelect )
{
	m_bPaintSelectNodes = bSelect; 
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_NODE_PAINTSELECTNODES, gPreferences.GetPaintSelectNodes() ? MF_CHECKED : MF_UNCHECKED );		
	gMainFrm.GetNodeBar().GetToolBarCtrl().CheckButton( ID_NODE_PAINTSELECTNODES, gPreferences.GetPaintSelectNodes() );
}


void Preferences::SetUpdateSound( bool bUpdate )
{
	m_bUpdateSound = bUpdate;
	gSoundManager.Enable( bUpdate, bUpdate );	
}


void Preferences::SetIgnoreVertexColor( bool bIgnoreVertexColor ) 
{ 
	m_bIgnoreVertexColor = bIgnoreVertexColor; 
	gMainFrm.GetPreferenceBar().GetToolBarCtrl().CheckButton( ID_LIGHTING_IGNOREVERTEXCOLOR, m_bIgnoreVertexColor );
}


void Preferences::SetCustomTabActive( bool bSet )
{
	m_bCustomViewActive = bSet;
	if (( bSet ) && ( gLeftView.GetTabCtrl().GetCurSel() != 1 ))
	{
		gLeftView.GetTabCtrl().SetCurSel( 1 );
		gLeftView.GetMainView().ShowWindow( SW_HIDE );
		gLeftView.GetCustomView().ShowWindow( SW_SHOW );
	}
}


void Preferences::SetObjectRotateMode( bool bRotate )
{ 
	m_bRotateMode = bRotate;
	CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_OBJECT_ROTATE_MODE, gPreferences.GetObjectRotateMode() ? MF_CHECKED : MF_UNCHECKED );		
	gMainFrm.GetObjectBar().GetToolBarCtrl().CheckButton( ID_OBJECT_ROTATE_MODE, gPreferences.GetObjectRotateMode() );			
}


void Preferences::SetMoveLockMode( eMoveLockMode mode )
{ 
	m_MoveLockMode = mode; 
	gMainFrm.GetObjectBar().GetToolBarCtrl().CheckButton( ID_OBJECT_MOVEMENTLOCK_X, mode == MOVE_LOCK_X ? TRUE : FALSE );			
	gMainFrm.GetObjectBar().GetToolBarCtrl().CheckButton( ID_OBJECT_MOVEMENTLOCK_Y, mode == MOVE_LOCK_Y ? TRUE : FALSE );
	gMainFrm.GetObjectBar().GetToolBarCtrl().CheckButton( ID_OBJECT_MOVEMENTLOCK_Z, mode == MOVE_LOCK_Z ? TRUE : FALSE );
}


void Preferences::SetDrawDebugHuds( bool bSet )
{	
	gMainFrm.GetPreferenceBar().GetToolBarCtrl().CheckButton( ID_DRAW_DEBUGHUDS, bSet );			

	if ( bSet && gWorldOptions.GetDebugHudGroup() == DHG_NONE )
	{
		gWorldOptions.SetDebugHudGroup( m_lastHudGroup );
	}
	else if ( !bSet )
	{
		if ( gWorldOptions.GetDebugHudGroup() != DHG_NONE )
		{
			m_lastHudGroup = gWorldOptions.GetDebugHudGroup();
		}

		gWorldOptions.SetDebugHudGroup( DHG_NONE );
	}
}


bool Preferences::GetDrawDebugHuds()
{
	if ( gWorldOptions.GetDebugHudGroup() != DHG_NONE )
	{
		return true;
	}

	return false;
}


void Preferences::SetClearColor( DWORD dwColor )
{
	gDefaultRapi.SetClearColor( dwColor );
	m_dwClearColor = dwColor;
}