//////////////////////////////////////////////////////////////////////////////
//
// File     :  MenuCommands.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "PrecompEditor.h"
#include "SiegeEditorShell.h"
#include "MenuCommands.h"
#include "EditorRegion.h"
#include "WorldOptions.h"
#include "Preferences.h"
#include "EditorTerrain.h"
#include "MeshPreviewer.h"
#include "siege_pos.h"
#include "LeftView.h"
#include "CustomViewer.h"
#include "ImageListDefines.h"
#include "EditorLights.h"
#include "EditorCamera.h"
#include "SiegeEditorView.h"
#include "CommandActions.h"
#include "EditorTriggers.h"
#include "EditorGizmos.h"
#include "EditorObjects.h"
#include "EditorHotpoints.h"
#include "MeshPreviewer.h"

#include "Dialog_Region_Load.h"
#include "Dialog_Region_New.h"
#include "Dialog_Save_Region.h"
#include "Dialog_Save_As_DSMap.h"
#include "Dialog_Save_As_DSMod.h"
#include "Dialog_Extract_Tank.h"
#include "Dialog_New_Map.h"
#include "Dialog_Preferences.h"
#include "Dialog_Keyboard_Shortcuts.h"
#include "Dialog_Stitch_Builder.h"
#include "Dialog_Node_Set.h"
#include "Dialog_Mesh_List.h"
#include "Dialog_Node_List.h"
#include "Dialog_Stitch_Manager.h"
#include "Dialog_Content_Search.h"
#include "Dialog_Object_Spot_Snapper.h"
#include "Dialog_Find.h"
#include "Dialog_Lighting_Ambience.h"
#include "Dialog_Lighting_Edit_Directional.h"
#include "Dialog_Map_Settings.h"
#include "Dialog_Region_Settings.h"
#include "Dialog_Object_Validation.h"
#include "Dialog_Bookmark_List.h"
#include "Dialog_Party_Editor.h"
#include "Dialog_Batch_LNC_Generation.h"
#include "Dialog_Map_Hotpoints.h"
#include "Dialog_Region_Edit_Hotpoints.h"
#include "Dialog_Object_List.h"
#include "Dialog_Huds.h"
#include "Dialog_Hotpoint_Information.h"
#include "Dialog_Hotpoint_Properties.h"
#include "Dialog_Region_Hotpoint_Direction.h"
#include "Dialog_Command_Editor.h"
#include "Dialog_Path_Maker.h"
#include "Dialog_Path_New.h"
#include "Dialog_Path_Reporter.h"
#include "Dialog_Decal_Preferences.h"
#include "Dialog_Sfx_Editor.h"
#include "Dialog_Starting_Position_Properties.h"
#include "Dialog_Selection_Filters.h"
#include "Dialog_LogicalNode_Properties.h"
#include "Dialog_Portraits.h"
#include "Dialog_Movement_Locking.h"
#include "Dialog_Hide_Components.h"
#include "Dialog_Scid_Remapper.h"
#include "Dialog_Path_Batch_Generation.h"
#include "Dialog_Object_Grouping.h"
#include "Dialog_Object_Macros.h"
#include "Dialog_Object_Next.h"
#include "Dialog_Node_Fade_Groups.h"
#include "Dialog_Conversation_Manager.h"
#include "Dialog_Conversation_List_Global.h"
#include "Dialog_Quest_Manager.h"
#include "Dialog_Stitch_Tool.h"
#include "Dialog_Node_Matches.h"
#include "Dialog_Visual_Node_Placement_Settings.h"
#include "PropPage_Node_Fade_Settings.h"
#include "PropPage_Node_Set.h"
#include "PropPage_Node_Stitching.h"
#include "PropPage_Node_General.h"
#include "PropPage_Color.h"
#include "PropPage_Properties.h"
#include "PropPage_Lighting.h"
#include "ObjectPropertySheet.h"
#include "NodePropertySheet.h"
#include "MainFrm.h"
#include "Window_Mesh_Previewer.h"
#include "resource.h"
#include "WorldIndexBuilder.h"
#include "FuelDB.h"
#include "Dialog_Trigger_Editor.h"
#include "Object_Property_Sheet.h"
#include "PropPage_Properties_Equipment.h"
#include "PropPage_Properties_Inventory.h"
#include "PropPage_Properties_Lighting.h"
#include "PropPage_Properties_Rotation.h"
#include "PropPage_Properties_Template.h"
#include "PropPage_Properties_Triggers.h"
#include "PropPage_Properties_Gizmo.h"
#include "Dialog_Decal.h"
#include "Dialog_Camera_Controller.h"
#include "Dialog_Starting_Groups.h"
#include "Dialog_Mood_Editor.h"
#include "Dialog_Grid_Settings.h"
#include "Dialog_Meter_Point_Editor.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


MenuCommands::MenuCommands()
	: m_pPreferences( 0 )
	, m_pPreviewer( 0 )
	, m_pMeshList( 0 )
	, m_pNodeList( 0 )
	, m_pObjectSheet( 0 )
	, m_pp_color( 0 )		
	, m_pp_lighting( 0 )
	, m_pContentSearch( 0 )
	, m_pNewRegion( 0 )
	, m_pFind( 0 ) 
	, m_pAmbience( 0 )
	, m_pEditDirectional( 0 )
	, m_pSpotSnapper( 0 )	
	, m_pHotpoints( 0 )
	, m_pObjectList( 0 )
	, m_pObjectProperties( 0 )
	, m_pp_equipment( 0 )
	, m_pp_inventory( 0 )
	, m_pp_light( 0 )
	, m_pp_rotation( 0 )
	, m_pp_template( 0 )
	, m_pp_triggers( 0 )
	, m_pSequenceEditor( 0 )
	, m_pDecalPreferences( 0 )
	, m_pDecalProperties( 0 )
	, m_pCameraController( 0 )
	, m_pTuningPaths( 0 )
	, m_pSfxEditor( 0 )
	, m_pPortraits( 0 )
	, m_pp_gizmo( 0 )
	, m_pObjectGrouping( 0 )
	, m_pObjectMacros( 0 )
	, m_pNodeFadeGroups( 0 )
	, m_pStitchTool( 0 )
	, m_pNodeMatches( 0 )
	, m_pMoodEditor( 0 )
	, m_pGridSettings( 0 )
	, m_pMeterPointEditor( 0 )
	, m_pVisualNodePlacement( 0 )
{	
}


MenuCommands::~MenuCommands()
{
	delete m_pPreferences;
	delete m_pPreviewer;
	delete m_pObjectSheet;
	delete m_pp_color;	
	delete m_pp_lighting;
	delete m_pContentSearch;
	delete m_pNewRegion;
	delete m_pFind;
	delete m_pAmbience;
	delete m_pEditDirectional;
	delete m_pSpotSnapper;	
	delete m_pHotpoints;
	delete m_pObjectList;
	delete m_pObjectProperties;
	delete m_pp_equipment;
	delete m_pp_inventory;
	delete m_pp_light;
	delete m_pp_rotation;
	delete m_pp_template;
	delete m_pp_triggers;
	delete m_pSequenceEditor;
	delete m_pDecalPreferences;
	delete m_pDecalProperties;
	delete m_pCameraController;
	delete m_pTuningPaths;
	delete m_pSfxEditor;
	delete m_pp_gizmo;
	delete m_pObjectGrouping;
	delete m_pObjectMacros;
	delete m_pNodeFadeGroups;
	delete m_pStitchTool;
	delete m_pNodeMatches;
	delete m_pMoodEditor;
	delete m_pGridSettings;
	delete m_pMeterPointEditor;
	delete m_pVisualNodePlacement;
}


void MenuCommands::Preferences()
{		
	if ( m_pPreferences ) 
	{
		delete m_pPreferences;
		m_pPreferences = 0;
	}
	m_pPreferences = new Dialog_Preferences();
	m_pPreferences->Create( IDD_DIALOG_PREFERENCES );		
}


void MenuCommands::MeshList()
{
	if ( m_pMeshList ) 
	{
		delete m_pMeshList;
		m_pMeshList = 0;
	}

	m_pMeshList = new Dialog_Mesh_List();
	m_pMeshList->Create( IDD_DIALOG_MESH_LIST );
}


void MenuCommands::NodeList()
{
	if ( m_pNodeList ) 
	{
		delete m_pNodeList;
		m_pNodeList = 0;
	}

	m_pNodeList = new Dialog_Node_List();
	m_pNodeList->Create( IDD_DIALOG_NODE_LIST );
}


void MenuCommands::ContentSearch()
{
	if ( m_pContentSearch ) 
	{
		delete m_pContentSearch;
		m_pContentSearch = 0;
	}

	m_pContentSearch = new Dialog_Content_Search();
	m_pContentSearch->Create( IDD_DIALOG_CONTENT_SEARCH );
}


void MenuCommands::NewRegion()
{	
	if ( m_pNewRegion ) 
	{
		delete m_pNewRegion;
		m_pNewRegion = 0;
	}

	m_pNewRegion = new Dialog_Region_New();
	m_pNewRegion->Create( IDD_DIALOG_REGION_NEW );
}


void MenuCommands::Find()
{
	if ( m_pFind ) {
		delete m_pFind; 
		m_pFind = 0;
	}

	m_pFind = new Dialog_Find();
	m_pFind->Create( IDD_DIALOG_FIND );
}


void MenuCommands::EditRegionHotpoints()
{
	if ( m_pHotpoints )
	{
		delete m_pHotpoints;
		m_pHotpoints = 0;
	}

	m_pHotpoints = new Dialog_Region_Hotpoint_Direction;
	m_pHotpoints->Create( IDD_DIALOG_REGION_HOTPOINT_DIRECTION );
}


void MenuCommands::Ambience()
{
	if ( m_pAmbience ) 
	{
		delete m_pAmbience;
		m_pAmbience = 0;
	}

	m_pAmbience = new Dialog_Lighting_Ambience();
	m_pAmbience->Create( IDD_DIALOG_LIGHTING_AMBIENCE );
}


void MenuCommands::EditDirectionalLights()
{
	if ( m_pEditDirectional ) 
	{
		delete m_pEditDirectional;
		m_pEditDirectional = 0;
	}
	
	m_pEditDirectional = new Dialog_Lighting_Edit_Directional();
	m_pEditDirectional->Create( IDD_DIALOG_LIGHTING_EDIT_DIRECTIONAL );
}


void MenuCommands::SpotSnapper()
{
	if ( m_pSpotSnapper ) 
	{
		delete m_pSpotSnapper;
		m_pSpotSnapper = 0;
	}

	m_pSpotSnapper = new Dialog_Object_Spot_Snapper();
	m_pSpotSnapper->Create( IDD_DIALOG_OBJECT_SPOT_SNAPPER );
}


void MenuCommands::MeshPreviewer()
{
	if ( gPreviewer.IsInitialized() )
	{
		return;
	}

	if ( m_pPreviewer ) 
	{
		// delete m_pPreviewer;
		// m_pPreviewer = 0;
		m_pPreviewer->ShowWindow( SW_SHOW );
		m_pPreviewer->UpdateWindow();
		gPreviewer.SetPreviewerWindow( m_pPreviewer );
		gPreviewer.SetInitialized( true );
		return;
	}
	m_pPreviewer = new Window_Mesh_Previewer;
	RECT rect;
	GetClientRect( gEditor.GetMainHWnd(), &rect );
	CMenu				m_wndProperties;
	m_pPreviewer->CreateEx(	WS_EX_NOPARENTNOTIFY, m_sPreviewerClass.c_str(), "Mesh Previewer", WS_POPUP | WS_OVERLAPPEDWINDOW | WS_VISIBLE,
							rect.right - 512, rect.bottom - 512, 512, 512, gEditor.GetMainHWnd(), LoadMenu( AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MENU_PREVIEWER) ) );				
		
	gPreviewer.SetPreviewerWindow( m_pPreviewer );
	gPreviewer.Init();
}


void MenuCommands::ObjectList()
{
	if ( m_pObjectList )
	{
		delete m_pObjectList;
		m_pObjectList = 0;
	}

	m_pObjectList = new Dialog_Object_List;
	BOOL bSuccess = m_pObjectList->Create( IDD_DIALOG_OBJECT_LIST );
}


void MenuCommands::SequenceEditor()
{
	if ( m_pSequenceEditor )
	{
		delete m_pSequenceEditor;
		m_pSequenceEditor = 0;
	}

	m_pSequenceEditor = new Dialog_Command_Editor;
	m_pSequenceEditor->Create( IDD_DIALOG_COMMAND_EDITOR );
}


void MenuCommands::DecalPreferences()
{
	if ( m_pDecalPreferences )
	{
		delete m_pDecalPreferences;
		m_pDecalPreferences = 0;
	}

	m_pDecalPreferences = new Dialog_Decal_Preferences;
	m_pDecalPreferences->Create( IDD_DIALOG_DECAL_PREFERENCES );
}


void MenuCommands::DecalProperties()
{
	if ( m_pDecalProperties )
	{
		delete m_pDecalProperties;
		m_pDecalProperties = 0;
	}

	m_pDecalProperties = new Dialog_Decal;
	m_pDecalProperties->Create( IDD_DIALOG_DECAL );
}


void MenuCommands::CameraController()
{
	if ( m_pCameraController )
	{
		delete m_pCameraController;
		m_pCameraController = 0;
	}

	m_pCameraController = new Dialog_Camera_Controller;
	m_pCameraController->Create( IDD_DIALOG_CAMERA_CONTROLLER );
}


void MenuCommands::TuningPaths()
{
	if ( m_pTuningPaths )
	{
		delete m_pTuningPaths;
		m_pTuningPaths = 0;
	}

	m_pTuningPaths = new Dialog_Path_Maker;
	m_pTuningPaths->Create( IDD_DIALOG_PATH_MAKER );
}


void MenuCommands::SiegeFxEditor()
{
	if ( m_pSfxEditor )
	{
		delete m_pSfxEditor;
		m_pSfxEditor = 0;
	}

	m_pSfxEditor = new Dialog_Sfx_Editor;
	m_pSfxEditor->Create( IDD_DIALOG_SFX );
}


void MenuCommands::GeneratePortraits()
{
	if ( m_pPortraits )
	{
		delete m_pPortraits;
		m_pPortraits = 0;
	}

	m_pPortraits = new Dialog_Portraits;
	m_pPortraits->Create( IDD_DIALOG_PORTRAITS );
}


void MenuCommands::ObjectGrouping()
{
	if ( m_pObjectGrouping )
	{
		delete m_pObjectGrouping;
		m_pObjectGrouping = 0;
	}

	m_pObjectGrouping = new Dialog_Object_Grouping;
	m_pObjectGrouping->Create( IDD_DIALOG_OBJECT_GROUPING );
}


void MenuCommands::ObjectMacros()
{
	if ( m_pObjectMacros )
	{
		delete m_pObjectMacros;
		m_pObjectMacros = 0;
	}

	m_pObjectMacros = new Dialog_Object_Macros;
	m_pObjectMacros->Create( IDD_DIALOG_MACROS );
}


void MenuCommands::NodeFadeGroups()
{
	if ( m_pNodeFadeGroups )
	{
		delete m_pNodeFadeGroups;
		m_pNodeFadeGroups = 0;
	}

	m_pNodeFadeGroups = new Dialog_Node_Fade_Groups;
	m_pNodeFadeGroups->Create( IDD_DIALOG_NODE_FADE_GROUPS );
}


void MenuCommands::AutoStitchTool()
{
	if ( m_pStitchTool )
	{
		delete m_pStitchTool;
		m_pStitchTool = 0;
	}

	m_pStitchTool = new Dialog_Stitch_Tool;
	m_pStitchTool->Create( IDD_DIALOG_STITCH_TOOL );
}


void MenuCommands::NodeMatches()
{
	if ( m_pNodeMatches )
	{
		delete m_pNodeMatches;
		m_pNodeMatches = 0;
	}

	m_pNodeMatches = new Dialog_Node_Matches;
	m_pNodeMatches->Create( IDD_DIALOG_NODE_MATCHES );
}


void MenuCommands::MoodEditor()
{
	if ( m_pMoodEditor )
	{
		delete m_pMoodEditor;
		m_pMoodEditor = 0;
	}

	m_pMoodEditor = new Dialog_Mood_Editor;
	m_pMoodEditor->Create( IDD_DIALOG_MOOD_EDITOR );
}


void MenuCommands::GridSettings()
{
	if ( m_pGridSettings )
	{
		delete m_pGridSettings;
		m_pGridSettings = 0;
	}

	m_pGridSettings = new Dialog_Grid_Settings;
	m_pGridSettings->Create( IDD_DIALOG_GRID_SETTINGS );
}


void MenuCommands::MeterPointEditor()
{
	if ( m_pMeterPointEditor )
	{
		delete m_pMeterPointEditor;
		m_pMeterPointEditor = 0;
	}

	m_pMeterPointEditor = new Dialog_Meter_Point_Editor;
	m_pMeterPointEditor->Create( IDD_DIALOG_METER_POINT_EDITOR );
}


void MenuCommands::VisualNodePlacement()
{
	if ( m_pVisualNodePlacement )
	{
		delete m_pVisualNodePlacement;
		m_pVisualNodePlacement = 0;
	}

	m_pVisualNodePlacement = new Dialog_Visual_Node_Placement_Settings;
	m_pVisualNodePlacement->Create( IDD_DIALOG_VISUAL_NODE_PLACEMENT_SETTINGS );
}

void MenuCommands::ObjectProperties()
{
	delete m_pObjectSheet;
	delete m_pp_color;	
	delete m_pp_lighting;

	m_pObjectSheet	= 0;
	m_pp_color		= 0;
	m_pp_lighting	= 0;
	
	m_pObjectSheet	= new CObjectPropertySheet( "Directional Light Properties", gMainFrm.GetCWnd(), 0 );
	m_pp_color		= new PropPage_Color();	
	m_pp_lighting	= new PropPage_Lighting();
	m_pObjectSheet->m_psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;	
	
	m_pObjectSheet->AddPage( m_pp_color );
	m_pObjectSheet->AddPage( m_pp_lighting );
	m_pObjectSheet->SetTitle( "Directional Light Properties" );
	m_pObjectSheet->Create();
}


void MenuCommands::ObjProperties()
{
	if ( m_pObjectProperties )
	{
		Delete( m_pObjectProperties );
		Delete( m_pp_equipment );
		Delete( m_pp_inventory );
		Delete( m_pp_light );
		Delete( m_pp_rotation );
		Delete( m_pp_template );
		Delete( m_pp_triggers );
		Delete( m_pp_gizmo );
	}

	m_pObjectProperties = new Object_Property_Sheet( "Object Properties", gMainFrm.GetCWnd() );
	m_pObjectProperties->m_psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;	

	m_pp_template	= new PropPage_Properties_Template;
	m_pp_triggers	= new PropPage_Properties_Triggers;
	m_pp_rotation	= new PropPage_Properties_Rotation;
	m_pp_equipment	= new PropPage_Properties_Equipment;
	m_pp_inventory	= new PropPage_Properties_Inventory;
	m_pp_light		= new PropPage_Properties_Lighting;

	m_pObjectProperties->AddPage( m_pp_template );
	m_pObjectProperties->AddPage( m_pp_triggers );
	m_pObjectProperties->AddPage( m_pp_rotation );
	m_pObjectProperties->AddPage( m_pp_equipment );
	m_pObjectProperties->AddPage( m_pp_inventory );
	m_pObjectProperties->AddPage( m_pp_light );	

	m_pObjectProperties->SetTitle( "Object Properties" );
	m_pObjectProperties->Create( NULL, WS_THICKFRAME | WS_VISIBLE | WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX, 0 );
}


void MenuCommands::TriggerProperties()
{
	ObjProperties();
	m_pObjectProperties->SetActivePage( 1 );
}


void MenuCommands::EquipmentProperties()
{
	ObjProperties();
	m_pObjectProperties->SetActivePage( 3 );
}


void MenuCommands::InventoryProperties()
{
	ObjProperties();
	m_pObjectProperties->SetActivePage( 4 );
}


void MenuCommands::Init()
{
	m_sPreviewerClass = AfxRegisterWndClass( 0, 0, (HBRUSH)GetStockObject(BLACK_BRUSH), 0 );
}


bool MenuFunctions::CheckRegionEnable()
{
	if ( gEditorRegion.GetRegionEnable() == false ) 
	{
		MessageBoxEx( gEditor.GetHWnd(), "You must have a region open before you can do this operation.", "No Region Loaded", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return false;
	}

	return true;
}


void MenuFunctions::LoadRegion()
{
	Dialog_Region_Load dlgRegionLoad;
	dlgRegionLoad.DoModal();	
}


void MenuFunctions::SaveRegion()
{
	Dialog_Save_Region dlgRegionSave;
	dlgRegionSave.DoModal();
}


void MenuFunctions::SaveMapAsDsmap()
{
	Dialog_Save_As_DSMap().DoModal();
}


void MenuFunctions::SaveFolderAsDsmod()
{
	Dialog_Save_As_DSMod().DoModal();
}


void MenuFunctions::ConvertToFiles()
{
	Dialog_Extract_Tank().DoModal();
}


void MenuFunctions::NewRegion()
{
	gMenuCommands.NewRegion();
}


void MenuFunctions::NewMap()
{
	Dialog_New_Map dlgNewMap;
	dlgNewMap.DoModal();
}


void MenuFunctions::KeyMapper()
{
	Dialog_Keyboard_Shortcuts dlgKeys;
	dlgKeys.DoModal();
}


void MenuFunctions::CreateBackups()
{
	if ( gEditorRegion.GetSaveBackups() ) 
	{
		CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_FILE_CREATEBACKUPSONSAVE, MF_UNCHECKED );
		gEditorRegion.SetSaveBackups( false );
	}
	else 
	{
		CheckMenuItem( GetMenu( gEditor.GetMainHWnd() ), ID_FILE_CREATEBACKUPSONSAVE, MF_CHECKED );
		gEditorRegion.SetSaveBackups( true );
	}
}

void MenuFunctions::AutoSnapToGround()
{
	if ( gPreferences.GetSnapToGround() ) 
	{				
		gPreferences.SetSnapToGround( false );		
	}
	else 
	{		
		gPreferences.SetSnapToGround( true );	
	}
}


void MenuFunctions::Preferences()
{	
	gMenuCommands.Preferences();		
}


void MenuFunctions::ShowObjects()
{
	if ( gPreferences.GetShowObjects() ) 
	{		
		gPreferences.SetShowObjects( false );				
	}	
	else 
	{		
		gPreferences.SetShowObjects( true );	
	}
}


void MenuFunctions::ShowLights()
{
	if ( gPreferences.GetShowLights() ) 
	{		
		gPreferences.SetShowLights( false );		
	}	
	else 
	{	
		gPreferences.SetShowLights( true );	
	}
}


void MenuFunctions::ShowGizmos()
{
	if ( gPreferences.GetShowGizmos() ) 
	{		
		gPreferences.SetShowGizmos( false );		
	}	
	else 
	{		
		gPreferences.SetShowGizmos( true );	
	}
}


void MenuFunctions::ShowTuningPoints()
{
	if ( gPreferences.GetShowTuningPoints() ) 
	{		
		gPreferences.SetShowTuningPoints( false );	
	}	
	else 
	{		
		gPreferences.SetShowTuningPoints( true );	
	}
}


void MenuFunctions::ModeObjects()
{
	if ( gPreferences.GetModeObjects() ) 
	{		
		gPreferences.SetModeObjects( false );	
	}	
	else 
	{
		gPreferences.SetModeObjects( true );	
	}	
}


void MenuFunctions::ModeLights()
{
	if ( gPreferences.GetModeLights() ) 
	{		
		gPreferences.SetModeLights( false );	
	}	
	else 
	{		
		gPreferences.SetModeLights( true );	
	}
}


void MenuFunctions::ModeGizmos()
{
	if ( gPreferences.GetModeGizmos() ) 
	{		
		gPreferences.SetModeGizmos( false );		
	}	
	else 
	{	
		gPreferences.SetModeGizmos( true );	
	}
}


void MenuFunctions::ModeNodes()
{
	if ( gPreferences.GetModeNodes() )
	{		
		gPreferences.SetModeNodes( false );	
	}	
	else 
	{		
		gPreferences.SetModeNodes( true );		
	}
}


void MenuFunctions::ModeDecals()
{
	if ( gPreferences.GetModeDecals() )
	{	
		gPreferences.SetModeDecals( false );	
	}	
	else 
	{	
		gPreferences.SetModeDecals( true );	
	}
}


void MenuFunctions::ModeLogicalNodes()
{
	if ( gPreferences.GetModeLogicalNodes() )
	{		
		gPreferences.SetModeLogicalNodes( false );	
	}	
	else 
	{	
		gPreferences.SetModeLogicalNodes( true );	
	}
}


void MenuFunctions::ModeTuningPoints()
{
	if ( gPreferences.GetModeTuningPoints() )
	{		
		gPreferences.SetModeTuningPoints( false );	
	}	
	else 
	{		
		gPreferences.SetModeTuningPoints( true );	
	}
}


void MenuFunctions::ModeNodePlacement()
{
	gPreferences.SetModeNodePlacement( !gPreferences.GetModeNodePlacement() );	
}


void MenuFunctions::PaintSelectNodes()
{
	if ( gPreferences.GetPaintSelectNodes() ) 
	{		
		gPreferences.SetPaintSelectNodes( false );		
	}	
	else 
	{		
		gPreferences.SetPaintSelectNodes( true );	
	}	
}


void MenuFunctions::PlacementMode()
{	
	if ( gPreferences.GetPlacementMode() ) 
	{		
		gPreferences.SetPlacementMode( false );		
	}	
	else 
	{		
		gPreferences.SetPlacementMode( true );	
	}
}


void MenuFunctions::RotateMode()
{
	if ( gPreferences.GetObjectRotateMode() )
	{		
		gPreferences.SetObjectRotateMode( false );	
	}
	else
	{		
		gPreferences.SetObjectRotateMode( true );	
	}
}


void MenuFunctions::MovementMode()
{
	if ( gPreferences.GetMovementMode() ) 
	{		
		gPreferences.SetMovementMode( false );	
	}	
	else 
	{		
		gPreferences.SetMovementMode( true );	
	}
}


void MenuFunctions::DrawDirectionalLights()
{
	if ( gPreferences.GetDrawDirectionalLights() ) 
	{		
		gPreferences.SetDrawDirectionalLights( false );	
	}	
	else 
	{		
		gPreferences.SetDrawDirectionalLights( true );	
	}	
}


void MenuFunctions::NodeProperties()
{
	if ( gEditorTerrain.GetSelectedNode() == siege::UNDEFINED_GUID ) 
	{
		return;
	}

	NodePropertySheet PSheet( "Node Properties", gMainFrm.GetCWnd(), 0 );	
	PSheet.m_psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	PropPage_Node_General		pp_node_general;
	PropPage_Node_Fade_Settings pp_fade_settings;
	PropPage_Node_Set			pp_node_set;
	PropPage_Node_Stitching		pp_node_stitching;
	
	PSheet.AddPage( &pp_node_general );
	PSheet.AddPage( &pp_fade_settings );
	PSheet.AddPage( &pp_node_set );
	PSheet.AddPage( &pp_node_stitching );	
	
	PSheet.SetTitle( "Node Properties" );
	PSheet.DoModal();
}


void MenuFunctions::ObjectProperties()
{
	if ( gGizmoManager.GetNumGizmosSelected() == 0 ) 
	{
		return;
	}

	gMenuCommands.ObjectProperties();
}


void MenuFunctions::ObjProperties()
{
	if ( gGizmoManager.GetNumGizmosSelected() == 0 ) 
	{
		return;
	}

	gMenuCommands.ObjProperties();
}


void MenuFunctions::TriggerProperties()
{
	if ( gGizmoManager.GetNumGizmosSelected() == 0 ) 
	{
		return;
	}

	gMenuCommands.TriggerProperties();
}


void MenuFunctions::EquipmentProperties()
{
	if ( gGizmoManager.GetNumGizmosSelected() == 0 ) 
	{
		return;
	}

	gMenuCommands.EquipmentProperties();
}


void MenuFunctions::InventoryProperties()
{
	if ( gGizmoManager.GetNumGizmosSelected() == 0 ) 
	{
		return;
	}

	gMenuCommands.InventoryProperties();
}

void MenuFunctions::StitchBuilder()
{
	Dialog_Stitch_Builder dlgBuilder;
	dlgBuilder.DoModal();
}


void MenuFunctions::MeshPreviewer()
{
	gMenuCommands.MeshPreviewer();	
}


void MenuFunctions::ContentSearch()
{
	gMenuCommands.ContentSearch();
}


void MenuFunctions::NodeSet()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	Dialog_Node_Set dlgNodeSet;
	dlgNodeSet.DoModal();
}


void MenuFunctions::SetAsTargetNode()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gEditorTerrain.SetTargetNode( gEditorTerrain.GetSelectedNode() );
}


void MenuFunctions::SelectAllNodes()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}
	gEditorTerrain.SelectAllNodes();	
}


void MenuFunctions::MeshList()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}
	gMenuCommands.MeshList();
}


void MenuFunctions::NodeList()
{	
	if ( !CheckRegionEnable() ) 
	{
		return;
	}
	gMenuCommands.NodeList();
}


void MenuFunctions::AutoConnectNodes()
{
	gEditorTerrain.AutoNodeConnection();
}

void MenuFunctions::AutoConnectSelectedNodes()
{
	gEditorTerrain.AutoNodeConnectSelected();
}


void MenuFunctions::StitchManager()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	Dialog_Stitch_Manager dlgStitch;
	dlgStitch.DoModal();
}

void MenuFunctions::SelectLastAdded()
{
	if ( gEditorTerrain.GetLastAddedNode() != siege::UNDEFINED_GUID )
	{
		gEditorTerrain.SetSelectedNode( gEditorTerrain.GetLastAddedNode() );
		gEditorTerrain.SetSourceDoors( gEditorTerrain.GetLastAddedNode() );
		gLeftView.PublishSourceDoorList();
	}
}


void MenuFunctions::KeyLeft()
{
	if ( gEditor.GetHWnd() != GetFocus() ) 
	{
		return;
	}

	if ( gGizmoManager.GetNumGizmosSelected() != 0 ) 
	{
		// Precision Move Left
		gGizmoManager.PrecisionMove( PRECISION_LEFT );
	}
	else 
	{
		gLeftView.RotateSourceDoorLeft();
	}
}


void MenuFunctions::KeyRight()
{
	if ( gEditor.GetHWnd() != GetFocus() ) 
	{
		return;
	}

	if ( gGizmoManager.GetNumGizmosSelected() != 0 ) 
	{
		gGizmoManager.PrecisionMove( PRECISION_RIGHT );
	}
	else 
	{
		gLeftView.RotateSourceDoorRight();
	}	
}


void MenuFunctions::KeyUp()
{
	if ( gEditor.GetHWnd() != GetFocus() ) 
	{
		return;
	}

	if ( gGizmoManager.GetNumGizmosSelected() != 0 ) 
	{
		gGizmoManager.PrecisionMove( PRECISION_UP );
	}
	else 
	{
		gLeftView.RotateDestDoorUp();
	}
}


void MenuFunctions::KeyDown()
{
	if ( gEditor.GetHWnd() != GetFocus() ) 
	{
		return;
	}

	if ( gGizmoManager.GetNumGizmosSelected() != 0 ) 
	{
		gGizmoManager.PrecisionMove( PRECISION_DOWN );
	}
	else 
	{
		gLeftView.RotateDestDoorDown();
	}
}


void MenuFunctions::KeyForward()
{
	if ( gEditor.GetHWnd() != GetFocus() ) 
	{
		return;
	}

	if ( gGizmoManager.GetNumGizmosSelected() != 0 ) 
	{
		gGizmoManager.PrecisionMove( PRECISION_FORWARD );
	}
}


void MenuFunctions::KeyBackward()
{
	if ( gEditor.GetHWnd() != GetFocus() ) 
	{
		return;
	}

	if ( gGizmoManager.GetNumGizmosSelected() != 0 ) 
	{
		gGizmoManager.PrecisionMove( PRECISION_BACKWARD );
	}
}


void MenuFunctions::KeyDelete()
{
	if ( gEditor.GetHWnd() != GetFocus() ) 
	{
		return;
	}

	if ( gGizmoManager.GetNumGizmosSelected() != 0 ) 
	{
		int result = MessageBoxEx( gEditor.GetHWnd(), "Are you sure you wish to delete these object(s)?", "Confirm Object Deletion", MB_YESNO | MB_ICONEXCLAMATION, LANG_ENGLISH );
		if ( result == IDYES ) 
		{
			gGizmoManager.DeleteSelectedGizmos();
		}
	}
	else if (( gEditorTerrain.GetSelectedNode() != siege::UNDEFINED_GUID ) && gPreferences.GetModeNodes()) 
	{
		int result = MessageBoxEx( gEditor.GetHWnd(), "Are you sure you wish to delete these nodes(s)?\n\rAny nodes that do not trace back to the target node will also be deleted.", "Confirm Node Deletion", MB_YESNO | MB_ICONQUESTION, LANG_ENGLISH );
		if ( result == IDYES ) 
		{
			gEditorTerrain.DeleteSelectedNodes();
		}
	}
}


void MenuFunctions::AddGOToCustomView()
{
	std::vector< DWORD > objects;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedObjectIDs( objects );
	for ( i = objects.begin(); i != objects.end(); ++i ) 
	{
		GoHandle hObject( MakeGoid( *i ) );
		if ( hObject.IsValid() ) 
		{			
			gCustomViewer.AddToCustomView( hObject->GetTemplateName(), IMAGE_GO, gLeftView.GetCustomView(), gLeftView.GetSelectedCustomItem() );			
		}
	}
}


void MenuFunctions::Cut()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gGizmoManager.Cut();
}


void MenuFunctions::Copy()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gGizmoManager.Copy();
}


void MenuFunctions::Paste()
{
	if ( !CheckRegionEnable() ) {
		return;
	}

	gGizmoManager.Paste();
}


void MenuFunctions::Find()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gMenuCommands.Find();
}


void MenuFunctions::Ambience()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gMenuCommands.Ambience();
}


void MenuFunctions::EditDirectionalLights()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gMenuCommands.EditDirectionalLights();
}


void MenuFunctions::CalculateAllLights()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gEditorLights.ComputeStaticLighting();
}


void MenuFunctions::UpdateSelectedNodeLighting()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gEditorTerrain.UpdateSelectedNodeLighting();
}


void MenuFunctions::UpdateAllNodeLighting()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gEditorTerrain.UpdateAllNodeLighting();
}


void MenuFunctions::MapSettings()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	Dialog_Map_Settings dlgRegion;
	dlgRegion.DoModal();
}


void MenuFunctions::RegionSettings()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	Dialog_Region_Settings dlgMap;
	dlgMap.DoModal();
}


void MenuFunctions::SpotSnapper()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gMenuCommands.SpotSnapper();
}


void MenuFunctions::Validation()
{
	Dialog_Object_Validation dlgValidate;
	dlgValidate.DoModal();
}


void MenuFunctions::Bookmarks()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	Dialog_Bookmark_List dlgBookmarks;
	dlgBookmarks.DoModal();
}


void MenuFunctions::PartyEditor()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	Dialog_Party_Editor dlgParty;
	dlgParty.DoModal();	
}


void MenuFunctions::BatchLNCGeneration()
{
	Dialog_Batch_LNC_Generation dlgLNC;
	dlgLNC.DoModal();
}


void MenuFunctions::ResetCamera()
{
	gEditorCamera.Reset();
}


void MenuFunctions::SideView()
{
	gEditorCamera.SideView();
}


void MenuFunctions::TopView()
{
	gEditorCamera.OverheadView();
}


void MenuFunctions::AttachNode()
{
	gLeftView.OnAttach();
}

void MenuFunctions::SwitchView()
{
	if ( GetFocus() == gLeftView.GetSafeHwnd() ) 
	{
		SetFocus( gSiegeEditorView.GetSafeHwnd() );
	}
	else if ( GetFocus() == gEditor.GetMainHWnd() ) 
	{
		SetFocus( gLeftView.GetTreeHWnd() );
	}
	else 
	{
		SetFocus( gSiegeEditorView.GetSafeHwnd() );
	}
}

void MenuFunctions::ShowAllNodes()
{
	gEditorTerrain.ShowAllNodes();
}

void MenuFunctions::Undo()
{
	gCommandManager.Undo();
}


void MenuFunctions::IgnoreVertexColor()
{
	gPreferences.SetIgnoreVertexColor( !gPreferences.GetIgnoreVertexColor() );
	gMainFrm.GetPreferenceBar().GetToolBarCtrl().CheckButton( ID_LIGHTING_IGNOREVERTEXCOLOR, gPreferences.GetIgnoreVertexColor() );
}


bool IsFocusInMainWindow()
{
	if ( (GetFocus() == gLeftView.GetSafeHwnd()) || (GetFocus() == gEditor.GetHWnd()) ||
		(GetFocus() == gEditor.GetMainHWnd() )) 
	{
		return true;
	}
	return false;
}


void MenuFunctions::MapHotpoints()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	Dialog_Map_Hotpoints dlg;
	dlg.DoModal();
}


void MenuFunctions::EditRegionHotpoints()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gMenuCommands.EditRegionHotpoints();	
}


void MenuFunctions::ObjectList()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gMenuCommands.ObjectList();
}


void MenuFunctions::SequenceEditor()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gMenuCommands.SequenceEditor();
}


void MenuFunctions::HideObject()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gGizmoManager.HideSelectedObjects();

}


void MenuFunctions::ShowAllObjects()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gGizmoManager.ShowAllObjects();
}


void MenuFunctions::DebugHuds()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	Dialog_Huds dlgHuds;
	dlgHuds.DoModal();		
}


void MenuFunctions::DrawDebugHuds()
{
	gPreferences.SetDrawDebugHuds( !gPreferences.GetDrawDebugHuds() );
}


void MenuFunctions::SelectionLock()
{
	if ( gPreferences.GetSelectionLock() ) 
	{		
		gPreferences.SetSelectionLock( false );		
	}	
	else 
	{		
		gPreferences.SetSelectionLock( true );		
	}		
}


void MenuFunctions::PlacementLock()
{
	if ( gPreferences.GetPlacementLock() ) 
	{	
		gPreferences.SetPlacementLock( false );		
	}	
	else 
	{			
		gPreferences.SetPlacementLock( true );		
	}	
}


void MenuFunctions::MovementLocking()
{
	Dialog_Movement_Locking dlg;
	dlg.DoModal();
}

void MenuFunctions::ExecuteTriggers()
{
	gEditorTriggers.ExecuteSelectedTriggers();
}


void MenuFunctions::ResetTriggers()
{
	gEditorTriggers.ResetSelectedTriggers();
}


void MenuFunctions::HideNodes()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gEditorTerrain.HideSelectedNodes();
}

void MenuFunctions::HotpointProperties()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	Dialog_Hotpoint_Properties dlgHp;
	dlgHp.DoModal();
}


void MenuFunctions::StartingPosProperties()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

}

void MenuFunctions::SetCameraToCurrentPosition()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	gEditorGizmos.SetSelectedCameraPos();	
}


void MenuFunctions::StartingCameraPreview()
{
	gEditorGizmos.SetCameraToSelectedPos();
}


void MenuFunctions::LinkMode()
{
	gPreferences.SetLinkMode( true );
}


void MenuFunctions::HitDetectFloorsOnly()
{
	gPreferences.SetHitFloorsOnly( !gPreferences.GetHitFloorsOnly() );
}


void MenuFunctions::Wireframe()
{
	gPreferences.SetWireframe( !gPreferences.GetWireframe() );
}

void MenuFunctions::DoorDraw()
{
	gPreferences.SetDrawDoors( !gPreferences.GetDrawDoors() );
}


void MenuFunctions::DrawStitchDoors()
{
	gPreferences.SetDrawStitchDoors( !gPreferences.GetDrawStitchDoors() );
}


void MenuFunctions::FloorDraw()
{
	gPreferences.SetDrawFloors( !gPreferences.GetDrawFloors() );
}


void MenuFunctions::HotpointInformation()
{
	Dialog_Hotpoint_Information dlgHp;
	dlgHp.DoModal();	
}


void MenuFunctions::GrabScid()
{
	gEditorObjects.GrabSelectedScid();
}


void MenuFunctions::GrabGuid()
{
	gEditorTerrain.GrabSelectedGUID();
}

void MenuFunctions::GrabDecalGuid()
{
	gEditorGizmos.GrabDecalGuid();
}

void MenuFunctions::GrabLightGuid()
{
	gEditorLights.GrabSelectedLightGuid();
}

void MenuFunctions::DecalPreferences()
{
	gMenuCommands.DecalPreferences();
}

void MenuFunctions::DecalProperties()
{
	gMenuCommands.DecalProperties();
}

void MenuFunctions::SetCameraToGo()
{
	GoHandle hObject( MakeGoid( gGizmoManager.GetSelectedGizmoID() ) );
	if ( hObject.IsValid() )
	{
		gEditorCamera.SetEditorCameraToGoCamera( hObject );
	}
}

void MenuFunctions::SetGoToCamera()
{
	GoHandle hObject( MakeGoid( gGizmoManager.GetSelectedGizmoID() ) );
	if ( hObject.IsValid() )
	{
		gEditorCamera.SetGoCameraToEditorCamera( hObject );
	}
}

void MenuFunctions::CameraController()
{
	gMenuCommands.CameraController();
}

void MenuFunctions::TuningPaths()
{
	gMenuCommands.TuningPaths();
}

void MenuFunctions::SiegeFxEditor()
{
	gMenuCommands.SiegeFxEditor();
}

void MenuFunctions::StartingPositionInfo()
{
	Dialog_Starting_Position_Properties dlg;
	dlg.DoModal();
}


void MenuFunctions::SelectionFilters()
{
	Dialog_Selection_Filters dlg;
	dlg.DoModal();
}


void MenuFunctions::LogicalNodeProperties()
{
	Dialog_LogicalNode_Properties dlg;
	dlg.DoModal();
}


void MenuFunctions::RotateSelection()
{
	gGizmoManager.RotateSelection();
}


void MenuFunctions::GeneratePortraits()
{
	gMenuCommands.GeneratePortraits();
}


void MenuFunctions::DebugHudAllToggle()
{
	if ( gWorldOptions.GetDebugHudGroup() == DHG_ALL )
	{
		gWorldOptions.SetDebugHudGroup( DHG_NONE );
	}
	else
	{
		gWorldOptions.SetDebugHudGroup( DHG_ALL );
	}
}


void MenuFunctions::DebugHudActorsToggle()
{
	if ( gWorldOptions.GetDebugHudGroup() == DHG_ACTORS )
	{
		gWorldOptions.SetDebugHudGroup( DHG_NONE );
	}
	else
	{
		gWorldOptions.SetDebugHudGroup( DHG_ACTORS );
	}
}


void MenuFunctions::DebugHudItemsToggle()
{
	if ( gWorldOptions.GetDebugHudGroup() == DHG_ITEMS )
	{
		gWorldOptions.SetDebugHudGroup( DHG_NONE );
	}
	else
	{
		gWorldOptions.SetDebugHudGroup( DHG_ITEMS );
	}
}


void MenuFunctions::DebugHudCommandsToggle()
{
	if ( gWorldOptions.GetDebugHudGroup() == DHG_COMMANDS )
	{
		gWorldOptions.SetDebugHudGroup( DHG_NONE );
	}
	else
	{
		gWorldOptions.SetDebugHudGroup( DHG_COMMANDS );
	}
}


void MenuFunctions::DebugHudMiscToggle()
{
	if ( gWorldOptions.GetDebugHudGroup() == DHG_MISC )
	{
		gWorldOptions.SetDebugHudGroup( DHG_NONE );
	}
	else
	{
		gWorldOptions.SetDebugHudGroup( DHG_MISC );
	}
}


void MenuFunctions::MovementLockX()
{
	if ( gPreferences.GetMoveLockMode() == MOVE_LOCK_X )
	{
		gPreferences.SetMoveLockMode( MOVE_LOCK_NONE );
	}
	else
	{
		gPreferences.SetMoveLockMode( MOVE_LOCK_X );
	}
}


void MenuFunctions::MovementLockY()
{
	if ( gPreferences.GetMoveLockMode() == MOVE_LOCK_Y )
	{
		gPreferences.SetMoveLockMode( MOVE_LOCK_NONE );
	}
	else
	{
		gPreferences.SetMoveLockMode( MOVE_LOCK_Y );
	}
}


void MenuFunctions::MovementLockZ()
{
	if ( gPreferences.GetMoveLockMode() == MOVE_LOCK_Z )
	{
		gPreferences.SetMoveLockMode( MOVE_LOCK_NONE );
	}
	else
	{
		gPreferences.SetMoveLockMode( MOVE_LOCK_Z );
	}
}


void MenuFunctions::MovementLockNone()
{
	gPreferences.SetMoveLockMode( MOVE_LOCK_NONE );
}


void MenuFunctions::ObjectDragCopy()
{
	gGizmoManager.DragCopy();
}


void MenuFunctions::HideComponents()
{
	Dialog_Hide_Components dlg;
	dlg.DoModal();
}


void MenuFunctions::ScidRemapper()
{
	Dialog_Scid_Remapper dlg;
	dlg.DoModal();
}


void MenuFunctions::BatchPathDataGeneration()
{
	Dialog_Path_Batch_Generation dlg;
	dlg.DoModal();
}


void MenuFunctions::RecalcDecals()
{
	gEditorGizmos.RecalcDecals();
}


void MenuFunctions::SetFocusToMainWindow()
{
	SetFocus( gSiegeEditorView.GetSafeHwnd() );
}


void MenuFunctions::ObjectGrouping()
{
	gMenuCommands.ObjectGrouping();
}


void MenuFunctions::ObjectMacros()
{
	gMenuCommands.ObjectMacros();
}


void MenuFunctions::NextObject()
{
	Dialog_Object_Next dlg;
	dlg.DoModal();
}


void MenuFunctions::NodeFadeGroups()
{
	gMenuCommands.NodeFadeGroups();
}


void MenuFunctions::StartingGroups()
{
	Dialog_Starting_Groups dlg;
	dlg.DoModal();
}


void MenuFunctions::DrawLogicalNodes()
{
	gPreferences.SetDrawLNodes( !gPreferences.GetDrawLNodes() );
	gMainFrm.GetPreferenceBar().GetToolBarCtrl().CheckButton( ID_DRAW_LOGICAL_NODES, gPreferences.GetDrawLNodes() );
}


void MenuFunctions::DrawDoors()
{
	gPreferences.SetDrawDoors( !gPreferences.GetDrawDoors() );
	gMainFrm.GetPreferenceBar().GetToolBarCtrl().CheckButton( ID_DRAW_DOORS, gPreferences.GetDrawDoors() );
}


void MenuFunctions::NodeCaps()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	bool bVisible = !gMainFrm.GetNodeBar().GetToolBarCtrl().IsButtonChecked( ID_HIDE_NODE_CAPS );
	gEditorTerrain.NodeCaps( bVisible );
	gMainFrm.GetNodeBar().GetToolBarCtrl().CheckButton( ID_HIDE_NODE_CAPS, bVisible );
}


void MenuFunctions::ConversationEditor()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	if ( gGizmoManager.GetNumGizmosSelected() == 1 )
	{
		GoHandle hObject( MakeGoid(gGizmoManager.GetSelectedGizmoID()) );
		if ( hObject.IsValid() && hObject->HasActor() && hObject->HasConversation() )
		{
			Dialog_Conversation_Manager dlg;
			dlg.DoModal();
			return;
		}
	}

	MessageBoxEx( gEditor.GetHWnd(), "You must have one actor selected that has a conversation block in its template.", "Conversation Manager", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
}


void MenuFunctions::GlobalConversationEditor()
{
	if ( !CheckRegionEnable() ) 
	{
		return;
	}

	Dialog_Conversation_List_Global dlg;
	dlg.DoModal();
}


void MenuFunctions::QuestEditor()
{
	if ( !CheckRegionEnable() )
	{
		return;
	}

	Dialog_Quest_Manager dlg;
	dlg.DoModal();
}


void MenuFunctions::AutoStitchTool()
{
	if ( !CheckRegionEnable() )
	{
		return;
	}

	gMenuCommands.AutoStitchTool();
}


void MenuFunctions::NodeMatches()
{
	if ( !CheckRegionEnable() )
	{
		return;
	}

	gMenuCommands.NodeMatches();
}


void MenuFunctions::DrawCameraTarget()
{
	if ( !CheckRegionEnable() )
	{
		return;
	}

	gPreferences.SetDrawCameraTarget( !gPreferences.GetDrawCameraTarget() );
}


void MenuFunctions::RestrictToGameCamera()
{
	if ( !CheckRegionEnable() )
	{
		return;
	}

	gPreferences.SetRestrictCameraToGame( !gPreferences.GetRestrictCameraToGame() );
}


void MenuFunctions::MoodEditor()
{
	gMenuCommands.MoodEditor();
}


void MenuFunctions::GridSettings()
{
	gMenuCommands.GridSettings();
}


void MenuFunctions::MeterPointEditor()
{
	gMenuCommands.MeterPointEditor();
}


void MenuFunctions::VisualNodePlacement()
{
	gMenuCommands.VisualNodePlacement();
}