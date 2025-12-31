//////////////////////////////////////////////////////////////////////////////
//
// File     :  MenuCommands.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __MENUCOMMANDS_H
#define __MENUCOMMANDS_H


// Include Files
#include "gpcore.h"
#include "gpstring.h"


// Forward Declarations
class Dialog_Preferences;
class Window_Mesh_Previewer;
class Dialog_Node_List;
class Dialog_Mesh_List;
class Dialog_Content_Search;
class PropPage_Color;
class PropPage_Lighting;
class CObjectPropertySheet;
class Dialog_Region_New;
class Dialog_Content_Search;
class Dialog_Find;
class Dialog_Lighting_Ambience;
class Dialog_Lighting_Edit_Directional;
class Dialog_Object_Spot_Snapper;
class Dialog_Region_Edit_Hotpoints;
class Dialog_Object_List;
class Object_Property_Sheet;
class PropPage_Properties_Equipment;
class PropPage_Properties_Inventory;
class PropPage_Properties_Lighting;
class PropPage_Properties_Rotation;
class PropPage_Properties_Template;
class PropPage_Properties_Triggers;
class PropPage_Properties_Gizmo;
class Dialog_Command_Editor;
class Dialog_Decal_Preferences;
class Dialog_Decal;
class Dialog_Camera_Controller;
class Dialog_Path_Maker;
class Dialog_Sfx_Editor;
class Dialog_Portraits;
class Dialog_Object_Grouping;
class Dialog_Object_Macros;
class Dialog_Node_Fade_Groups;
class Dialog_Region_Hotpoint_Direction;
class Dialog_Stitch_Tool;
class Dialog_Node_Matches;
class Dialog_Mood_Editor;
class Dialog_Grid_Settings;
class Dialog_Meter_Point_Editor;
class Dialog_Visual_Node_Placement_Settings;


// This class contains all the functions to handle the window commands that require the use
// of member varialbes ( for modeless dialogs, etc. ) that can
// be received through the menu of SE
class MenuCommands : public Singleton <MenuCommands>
{
public:

	MenuCommands();
	~MenuCommands();
	
	void Init();
	void Preferences();
	void MeshPreviewer();
	void MeshList();
	void NodeList();	
	void ContentSearch();
	void NewRegion();
	void Find();
	void Ambience();
	void EditDirectionalLights();
	void SpotSnapper();	
	void EditRegionHotpoints();
	void ObjectList();
	void SequenceEditor();
	void ObjectProperties();
	void ObjProperties();
	void TriggerProperties();	
	void EquipmentProperties();
	void InventoryProperties();
	void DecalPreferences();
	void DecalProperties();
	void CameraController();
	void TuningPaths();
	void SiegeFxEditor();
	void GeneratePortraits();
	void ObjectGrouping();
	void ObjectMacros();
	void NodeFadeGroups();	
	void AutoStitchTool();
	void NodeMatches();
	void MoodEditor();
	void GridSettings();
	void MeterPointEditor();
	void VisualNodePlacement();

	CObjectPropertySheet * GetObjectDialog() { return m_pObjectSheet; }
	
private:

	Dialog_Preferences					* m_pPreferences;
	Window_Mesh_Previewer				* m_pPreviewer;
	Dialog_Mesh_List					* m_pMeshList;
	Dialog_Node_List					* m_pNodeList;
	Dialog_Content_Search				* m_pContentSearch;
	Dialog_Region_New					* m_pNewRegion;
	Dialog_Find							* m_pFind;
	Dialog_Lighting_Ambience			* m_pAmbience;
	Dialog_Lighting_Edit_Directional	* m_pEditDirectional;
	Dialog_Object_Spot_Snapper			* m_pSpotSnapper;	
	Dialog_Region_Hotpoint_Direction	* m_pHotpoints;
	Dialog_Object_List					* m_pObjectList;
	Dialog_Command_Editor				* m_pSequenceEditor;
	Dialog_Decal_Preferences			* m_pDecalPreferences;
	Dialog_Decal						* m_pDecalProperties;
	Dialog_Camera_Controller			* m_pCameraController;
	Dialog_Path_Maker					* m_pTuningPaths;
	Dialog_Sfx_Editor					* m_pSfxEditor;
	Dialog_Portraits					* m_pPortraits;
	Dialog_Object_Grouping				* m_pObjectGrouping;
	Dialog_Object_Macros				* m_pObjectMacros;
	Dialog_Node_Fade_Groups				* m_pNodeFadeGroups;
	Dialog_Stitch_Tool					* m_pStitchTool;
	Dialog_Node_Matches					* m_pNodeMatches;
	Dialog_Mood_Editor					* m_pMoodEditor;
	Dialog_Grid_Settings				* m_pGridSettings;
	Dialog_Meter_Point_Editor			* m_pMeterPointEditor;
	Dialog_Visual_Node_Placement_Settings	* m_pVisualNodePlacement;
	
	Object_Property_Sheet				* m_pObjectProperties;
	PropPage_Properties_Equipment		* m_pp_equipment;
	PropPage_Properties_Inventory		* m_pp_inventory;
	PropPage_Properties_Lighting		* m_pp_light;
	PropPage_Properties_Rotation		* m_pp_rotation;
	PropPage_Properties_Template		* m_pp_template;
	PropPage_Properties_Triggers		* m_pp_triggers;
	PropPage_Properties_Gizmo			* m_pp_gizmo;

	CObjectPropertySheet				* m_pObjectSheet;
	PropPage_Color						* m_pp_color;	
	PropPage_Lighting					* m_pp_lighting;

	gpstring m_sPreviewerClass;	
};


#define gMenuCommands MenuCommands::GetSingleton()



// Standard menu functions

class MenuFunctions
{
public:

	MenuFunctions() {};

	bool CheckRegionEnable();

	void LoadRegion();
	void SaveRegion();
	void SaveMapAsDsmap();
	void SaveFolderAsDsmod();
	void ConvertToFiles();
	void NewRegion();
	void CloseRegion();
	void CreateBackups();
	void NewMap();	
	void Preferences();
	void ShowObjects();
	void ShowLights();
	void ShowGizmos();
	void ShowTuningPoints();
	void KeyMapper();
	void PaintSelectNodes();
	void NodeProperties();
	void StitchBuilder();	
	void MeshPreviewer();
	void NodeSet();
	void SetAsTargetNode();
	void SelectAllNodes();
	void MeshList();
	void NodeList();
	void AutoConnectNodes();
	void StitchManager();
	void SelectLastAdded();
	void KeyLeft();
	void KeyRight();
	void KeyUp();
	void KeyDown();
	void KeyForward();
	void KeyBackward();
	void KeyDelete();
	void AutoSnapToGround();
	void PlacementMode();
	void MovementMode();
	void DrawDirectionalLights();
	void ContentSearch();
	void AutoConnectSelectedNodes();
	void AddGOToCustomView();
	void Cut();
	void Copy();
	void Paste();
	void Find();
	void Ambience();
	void EditDirectionalLights();
	void CalculateAllLights();
	void UpdateSelectedNodeLighting();
	void UpdateAllNodeLighting();
	void MapSettings();
	void RegionSettings();
	void SpotSnapper();
	void Validation();
	void Bookmarks();
	void PartyEditor();
	void BatchLNCGeneration();	
	void ResetCamera();
	void SideView();
	void TopView();
	void AttachNode();
	void SwitchView();	
	void ShowAllNodes();	
	void Undo();
	void IgnoreVertexColor();
	void MapHotpoints();
	void EditRegionHotpoints();
	void ObjectList();
	void HideObject();
	void ShowAllObjects();	
	void SelectionLock();
	void PlacementLock();
	void ModeObjects();
	void ModeLights();	
	void ModeGizmos();
	void ModeNodes();
	void ModeDecals();
	void ModeLogicalNodes();
	void ModeTuningPoints();
	void ModeNodePlacement();
	void ExecuteTriggers();
	void ResetTriggers();
	void HideNodes();
	void LinkMode();
	void HitDetectFloorsOnly();
	void Wireframe();
	void DoorDraw();
	void DrawStitchDoors();
	void FloorDraw();
	void HotpointProperties();
	void StartingPosProperties();
	void SetCameraToCurrentPosition();
	void StartingPositionInfo();
	void StartingCameraPreview();
	void HotpointInformation();
	void SequenceEditor();
	void GrabScid();
	void GrabGuid();
	void GrabLightGuid();	
	void ObjectProperties();	
	void ObjProperties();
	void TriggerProperties();	
	void EquipmentProperties();
	void InventoryProperties();
	void DecalPreferences();
	void DecalProperties();
	void RotateMode();
	void SetCameraToGo();
	void SetGoToCamera();
	void CameraController();
	void TuningPaths();
	void SiegeFxEditor();
	void SelectionFilters();
	void LogicalNodeProperties();
	void RotateSelection();
	void GeneratePortraits();
	void DebugHuds();
	void DrawDebugHuds();
	void DebugHudAllToggle();
	void DebugHudActorsToggle();
	void DebugHudItemsToggle();
	void DebugHudCommandsToggle();
	void DebugHudMiscToggle();
	void MovementLocking();
	void MovementLockX();
	void MovementLockY();
	void MovementLockZ();
	void MovementLockNone();
	void ObjectDragCopy();
	void HideComponents();
	void ScidRemapper();
	void BatchPathDataGeneration();
	void GrabDecalGuid();
	void RecalcDecals();
	void SetFocusToMainWindow();
	void ObjectGrouping();
	void ObjectMacros();
	void NextObject();
	void NodeFadeGroups();
	void StartingGroups();
	void DrawLogicalNodes();
	void DrawDoors();
	void NodeCaps();
	void ConversationEditor();
	void GlobalConversationEditor();
	void QuestEditor();
	void AutoStitchTool();
	void NodeMatches();
	void DrawCameraTarget();
	void RestrictToGameCamera();
	void MoodEditor();
	void GridSettings();
	void MeterPointEditor();
	void VisualNodePlacement();
};


bool IsFocusInMainWindow();

#endif