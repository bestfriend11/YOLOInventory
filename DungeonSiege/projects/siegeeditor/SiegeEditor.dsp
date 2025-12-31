# Microsoft Developer Studio Project File - Name="SiegeEditor" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=SiegeEditor - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SiegeEditor.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SiegeEditor.mak" CFG="SiegeEditor - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SiegeEditor - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "SiegeEditor - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/Projects/SiegeEditor", NQCGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SiegeEditor - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\siegeeditor\Release"
# PROP Intermediate_Dir "\temp\vc6\siegeeditor\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"PrecompEditor.h" /FD /Zm600 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 shmfc4mt.lib shlw32mt.lib dinput.lib dxguid.lib dplayx.lib ddraw.lib d3dim.lib quartz.lib strmbasd.lib comctl32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"Release/SiegeEditor.exe"
# SUBTRACT LINK32 /incremental:yes /nodefaultlib

!ELSEIF  "$(CFG)" == "SiegeEditor - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\siegeeditor\Debug"
# PROP Intermediate_Dir "\temp\vc6\siegeeditor\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"PrecompEditor.h" /FD /GZ /Zm700 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dinput.lib dxguid.lib dplayx.lib ddraw.lib d3dim.lib quartz.lib strmbasd.lib comctl32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"Debug/SiegeEditorD.exe" /pdbtype:sept /force:multiple
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "SiegeEditor - Win32 Release"
# Name "SiegeEditor - Win32 Debug"
# Begin Group "Shell"

# PROP Default_Filter ""
# Begin Group "Settings"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Preferences.cpp
# End Source File
# Begin Source File

SOURCE=.\Preferences.h
# End Source File
# End Group
# Begin Group "SmartHeap"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Include\SH6\msvc\SHMFC.CPP

!IF  "$(CFG)" == "SiegeEditor - Win32 Release"

# ADD CPP /I "..\..\include\sh6\msvc"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "SiegeEditor - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\Local\CustomExports.cpp
# End Source File
# Begin Source File

SOURCE=.\Exports.cpp
# End Source File
# Begin Source File

SOURCE=.\FakeGame.cpp
# End Source File
# Begin Source File

SOURCE=.\GameState.cpp
# End Source File
# Begin Source File

SOURCE=.\GameState.h
# End Source File
# Begin Source File

SOURCE=.\ImageListDefines.h
# End Source File
# Begin Source File

SOURCE=.\SEVersion.h
# End Source File
# Begin Source File

SOURCE=.\SiegeEditorShell.cpp
# End Source File
# Begin Source File

SOURCE=.\SiegeEditorShell.h
# End Source File
# Begin Source File

SOURCE=.\SiegeEditorTypes.h
# End Source File
# End Group
# Begin Group "Windows"

# PROP Default_Filter ""
# Begin Group "Main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\SiegeEditor.cpp
# End Source File
# Begin Source File

SOURCE=.\SiegeEditor.h
# End Source File
# Begin Source File

SOURCE=.\hlp\SiegeEditor.hpj

!IF  "$(CFG)" == "SiegeEditor - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__SIEGE="hlp\AfxCore.rtf"	"hlp\$(TargetName).hm"	
# Begin Custom Build - Making help file...
OutDir=\temp\vc6\siegeeditor\Release
TargetName=SiegeEditor
InputPath=.\hlp\SiegeEditor.hpj
InputName=SiegeEditor

"$(OutDir)\$(InputName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	start /wait hcw /C /E /M "hlp\$(InputName).hpj" 
	if errorlevel 1 goto :Error 
	if not exist "hlp\$(InputName).hlp" goto :Error 
	copy "hlp\$(InputName).hlp" $(OutDir) 
	goto :done 
	:Error 
	echo hlp\$(InputName).hpj(1) : error: 
	type "hlp\$(InputName).log" 
	:done 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "SiegeEditor - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__SIEGE="hlp\AfxCore.rtf"	"hlp\$(TargetName).hm"	
# Begin Custom Build - Making help file...
OutDir=\temp\vc6\siegeeditor\Debug
TargetName=SiegeEditorD
InputPath=.\hlp\SiegeEditor.hpj
InputName=SiegeEditor

"$(OutDir)\$(InputName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	start /wait hcw /C /E /M "hlp\$(InputName).hpj" 
	if errorlevel 1 goto :Error 
	if not exist "hlp\$(InputName).hlp" goto :Error 
	copy "hlp\$(InputName).hlp" $(OutDir) 
	goto :done 
	:Error 
	echo hlp\$(InputName).hpj(1) : error: 
	type "hlp\$(InputName).log" 
	:done 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SiegeEditorDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\SiegeEditorDoc.h
# End Source File
# Begin Source File

SOURCE=.\SiegeEditorView.cpp
# End Source File
# Begin Source File

SOURCE=.\SiegeEditorView.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Dialogs"

# PROP Default_Filter ""
# Begin Group "Keyboard Shortcuts"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Keyboard_Shortcuts.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Keyboard_Shortcuts.h
# End Source File
# End Group
# Begin Group "Preferences"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Huds.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Huds.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Preferences.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Preferences.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Selection_Filters.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Selection_Filters.h
# End Source File
# End Group
# Begin Group "Stitch Builder"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Stitch_Builder.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Stitch_Builder.h
# End Source File
# End Group
# Begin Group "Stitch Manager"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Stitch_Manager.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Stitch_Manager.h
# End Source File
# End Group
# Begin Group "Object Properties"

# PROP Default_Filter ""
# Begin Group "Hide Components"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Hide_Components.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Hide_Components.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Object_Property_Sheet.cpp
# End Source File
# Begin Source File

SOURCE=.\Object_Property_Sheet.h
# End Source File
# Begin Source File

SOURCE=.\PropPage_Color.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPage_Color.h
# End Source File
# Begin Source File

SOURCE=.\PropPage_Lighting.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPage_Lighting.h
# End Source File
# Begin Source File

SOURCE=.\PropPage_Properties_Equipment.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPage_Properties_Equipment.h
# End Source File
# Begin Source File

SOURCE=.\PropPage_Properties_Gizmo.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPage_Properties_Gizmo.h
# End Source File
# Begin Source File

SOURCE=.\PropPage_Properties_Inventory.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPage_Properties_Inventory.h
# End Source File
# Begin Source File

SOURCE=.\PropPage_Properties_Lighting.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPage_Properties_Lighting.h
# End Source File
# Begin Source File

SOURCE=.\PropPage_Properties_Rotation.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPage_Properties_Rotation.h
# End Source File
# Begin Source File

SOURCE=.\PropPage_Properties_Template.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPage_Properties_Template.h
# End Source File
# Begin Source File

SOURCE=.\PropPage_Properties_Triggers.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPage_Properties_Triggers.h
# End Source File
# End Group
# Begin Group "Region Dialogs"

# PROP Default_Filter ""
# Begin Group "Region Load"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Region_Load.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Region_Load.h
# End Source File
# End Group
# Begin Group "New Region"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Region_New.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Region_New.h
# End Source File
# End Group
# Begin Group "Region Save"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Save_Region.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Save_Region.h
# End Source File
# End Group
# Begin Group "Save Progress"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Progress_Save.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Progress_Save.h
# End Source File
# End Group
# Begin Group "Region Settings"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Region_Settings.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Region_Settings.h
# End Source File
# End Group
# Begin Group "Hotpoint Editor"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Hotpoint_Information.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Hotpoint_Information.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Region_Edit_Hotpoints.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Region_Edit_Hotpoints.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Region_Hotpoint_Direction.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Region_Hotpoint_Direction.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Region_New_Hotpoint.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Region_New_Hotpoint.h
# End Source File
# End Group
# End Group
# Begin Group "Node Dialogs"

# PROP Default_Filter ""
# Begin Group "Mesh List"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Mesh_List.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Mesh_List.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_New_Mesh_Group.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_New_Mesh_Group.h
# End Source File
# End Group
# Begin Group "Node List"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Node_List.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Node_List.h
# End Source File
# End Group
# Begin Group "Node Set"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Node_Set.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Node_Set.h
# End Source File
# End Group
# Begin Group "Node Properties"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\NodePropertySheet.cpp
# End Source File
# Begin Source File

SOURCE=.\NodePropertySheet.h
# End Source File
# Begin Source File

SOURCE=.\PropPage_Node_Fade_Settings.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPage_Node_Fade_Settings.h
# End Source File
# Begin Source File

SOURCE=.\PropPage_Node_General.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPage_Node_General.h
# End Source File
# Begin Source File

SOURCE=.\PropPage_Node_Set.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPage_Node_Set.h
# End Source File
# Begin Source File

SOURCE=.\PropPage_Node_Stitching.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPage_Node_Stitching.h
# End Source File
# End Group
# Begin Group "Node Fade Groups"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_New_Node_Fade_Group.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_New_Node_Fade_Group.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Node_Fade_Groups.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Node_Fade_Groups.h
# End Source File
# End Group
# Begin Group "Node Matches"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Node_Match_Progress.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Node_Match_Progress.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Node_Matches.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Node_Matches.h
# End Source File
# End Group
# Begin Group "Node Placement"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Visual_Node_Placement_Settings.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Visual_Node_Placement_Settings.h
# End Source File
# End Group
# End Group
# Begin Group "Trigger Dialogs"

# PROP Default_Filter ""
# Begin Group "Trigger Actions"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Trigger_Add_Action.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Trigger_Add_Action.h
# End Source File
# End Group
# Begin Group "Trigger Conditions"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Trigger_Add_Condition.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Trigger_Add_Condition.h
# End Source File
# End Group
# End Group
# Begin Group "Object Dialogs"

# PROP Default_Filter ""
# Begin Group "Content Search"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Content_Search.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Content_Search.h
# End Source File
# End Group
# Begin Group "Find"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Find.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Find.h
# End Source File
# End Group
# Begin Group "Party Editor"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Party_Editor.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Party_Editor.h
# End Source File
# End Group
# Begin Group "Spot Snapper"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Object_Spot_Snapper.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Object_Spot_Snapper.h
# End Source File
# End Group
# Begin Group "Validation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Object_Validation.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Object_Validation.h
# End Source File
# End Group
# Begin Group "Object List"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Object_List.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Object_List.h
# End Source File
# End Group
# Begin Group "Movement Locking"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Movement_Locking.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Movement_Locking.h
# End Source File
# End Group
# Begin Group "Scid Remapper"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Scid_Remapper.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Scid_Remapper.h
# End Source File
# End Group
# Begin Group "Item Viewer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Item_Viewer.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Item_Viewer.h
# End Source File
# End Group
# Begin Group "Next Object"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Object_Next.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Object_Next.h
# End Source File
# End Group
# End Group
# Begin Group "Light Dialogs"

# PROP Default_Filter ""
# Begin Group "Ambience"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Lighting_Ambience.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Lighting_Ambience.h
# End Source File
# End Group
# Begin Group "Directional Lights"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Lighting_Edit_Directional.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Lighting_Edit_Directional.h
# End Source File
# Begin Source File

SOURCE=.\ObjectPropertySheet.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjectPropertySheet.h
# End Source File
# End Group
# End Group
# Begin Group "Map Dialogs"

# PROP Default_Filter ""
# Begin Group "New Map"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_New_Map.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_New_Map.h
# End Source File
# End Group
# Begin Group "Map Settings"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Map_Settings.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Map_Settings.h
# End Source File
# End Group
# Begin Group "Bookmarks"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Bookmark_List.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Bookmark_List.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Bookmark_Properties.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Bookmark_Properties.h
# End Source File
# End Group
# Begin Group "Batch LNC Generation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Batch_LNC_Generation.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Batch_LNC_Generation.h
# End Source File
# End Group
# Begin Group "Start Positions"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Starting_Group_Add_Level.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Starting_Group_Add_Level.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Starting_Group_New.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Starting_Group_New.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Starting_Group_Properties.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Starting_Group_Properties.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Starting_Groups.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Starting_Groups.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Starting_Position_Properties.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Starting_Position_Properties.h
# End Source File
# End Group
# Begin Group "Hotpoints"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Hotpoint_Properties.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Hotpoint_Properties.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Map_Hotpoints.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Map_Hotpoints.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Map_New_Hotpoint.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Map_New_Hotpoint.h
# End Source File
# End Group
# End Group
# Begin Group "Generic"

# PROP Default_Filter ""
# Begin Group "New Folder"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_NewFolder.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_NewFolder.h
# End Source File
# End Group
# End Group
# Begin Group "Color"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ColorPickerDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ColorPickerDlg.h
# End Source File
# Begin Source File

SOURCE=.\Common.cpp
# End Source File
# Begin Source File

SOURCE=.\Common.h
# End Source File
# Begin Source File

SOURCE=.\DIB.cpp
# End Source File
# Begin Source File

SOURCE=.\DIB.h
# End Source File
# End Group
# Begin Group "Command Sequence Editor"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Command_Editor.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Command_Editor.h
# End Source File
# End Group
# Begin Group "Decals"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Decal.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Decal.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Decal_Preferences.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Decal_Preferences.h
# End Source File
# End Group
# Begin Group "Special Effect Editor"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Sfx_Editor.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Sfx_Editor.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Sfx_Load_Script.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Sfx_Load_Script.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Sfx_New_Script.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Sfx_New_Script.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Sfx_Parameters.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Sfx_Parameters.h
# End Source File
# End Group
# Begin Group "Logical Nodes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_LogicalNode_Properties.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_LogicalNode_Properties.h
# End Source File
# End Group
# Begin Group "Portraits"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Portraits.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Portraits.h
# End Source File
# End Group
# Begin Group "Custom View"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Custom_View_Rename.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Custom_View_Rename.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Load_Custom_View.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Load_Custom_View.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Save_Custom_View.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Save_Custom_View.h
# End Source File
# End Group
# Begin Group "Tuning Paths"

# PROP Default_Filter ""
# Begin Group "Batch Tuning Path Generation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Path_Batch_Generation.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Path_Batch_Generation.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Dialog_Path_Maker.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Path_Maker.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Path_Reporter.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Path_Reporter.h
# End Source File
# End Group
# Begin Group "Object Grouping"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Object_Grouping.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Object_Grouping.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Object_Grouping_New.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Object_Grouping_New.h
# End Source File
# End Group
# Begin Group "Object Macros"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_New_Object_Macro.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_New_Object_Macro.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Object_Macros.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Object_Macros.h
# End Source File
# End Group
# Begin Group "Conversation Dialogs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Conversation_Activate_Quests.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_Activate_Quests.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_Add_Goodbye.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_Add_Goodbye.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_Complete_Quests.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_Complete_Quests.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_Custom_Buttons.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_Custom_Buttons.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_Editor.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_Editor.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_Line_Editor.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_Line_Editor.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_List_Global.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_List_Global.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_Manager.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_Manager.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_New.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_New.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_New_Global.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Conversation_New_Global.h
# End Source File
# End Group
# Begin Group "Quests"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Quest_Add.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Quest_Add.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Quest_Chapter_Add.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Quest_Chapter_Add.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Quest_Chapter_Editor.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Quest_Chapter_Editor.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Quest_Chapter_Update.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Quest_Chapter_Update.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Quest_Editor.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Quest_Editor.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Quest_Manager.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Quest_Manager.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Quest_Update.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Quest_Update.h
# End Source File
# End Group
# Begin Group "Stitch Tool"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Stitch_Tool.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Stitch_Tool.h
# End Source File
# End Group
# Begin Group "Moods"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Mood_Add.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Mood_Add.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Mood_Add_Folder.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Mood_Add_Folder.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Mood_Editor.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Mood_Editor.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Mood_Fog.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Mood_Fog.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Mood_General.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Mood_General.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Mood_Music.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Mood_Music.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Mood_Weather.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Mood_Weather.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Resource_Tree.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Resource_Tree.h
# End Source File
# End Group
# Begin Group "Grid"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Grid_Settings.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Grid_Settings.h
# End Source File
# End Group
# Begin Group "Meter Points"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Meter_Point_Editor.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Meter_Point_Editor.h
# End Source File
# End Group
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\icons\ambience.ico
# End Source File
# Begin Source File

SOURCE=.\arrow.cur
# End Source File
# Begin Source File

SOURCE=.\res\arrow3.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\auto_connect_nodes.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\auto_connect_selected_nodes.ico
# End Source File
# Begin Source File

SOURCE=.\res\Back.Ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\basic_node.ico
# End Source File
# Begin Source File

SOURCE=.\res\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00002.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00003.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00004.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00005.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00006.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CCHSB.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ccrgb.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\Programming\MFC\color_dialog\vc6projects\ColorSpace\res\ColorSpace.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\Programming\MFC\color_dialog\vc6projects\ColorSpace\res\ColorSpaceDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\cur00001.cur
# End Source File
# Begin Source File

SOURCE=.\res\cur00002.cur
# End Source File
# Begin Source File

SOURCE=.\res\cur00003.cur
# End Source File
# Begin Source File

SOURCE=.\res\cur00004.cur
# End Source File
# Begin Source File

SOURCE=.\res\icons\current_node_set.ico
# End Source File
# Begin Source File

SOURCE=.\res\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\res\icons\Decal.ico
# End Source File
# Begin Source File

SOURCE=.\res\DISK04.ICO
# End Source File
# Begin Source File

SOURCE=.\res\DOCL.ICO
# End Source File
# Begin Source File

SOURCE=.\res\down2.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\draw_directional_lights.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\edit_directional_lights.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\edit_inventory.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\feet.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\Game_object.ico
# End Source File
# Begin Source File

SOURCE=.\RES\GLOBE.ICO
# End Source File
# Begin Source File

SOURCE=.\res\gpglogo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Hammer.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00002.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00003.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00004.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00005.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00006.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00007.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00008.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon18.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon19.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon2.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon20.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon21.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon22.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon23.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon24.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon25.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon26.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon27.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon3.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon4.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon5.Ico
# End Source File
# Begin Source File

SOURCE=.\res\icon6.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon7.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon_go1.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\movement_mode.ico
# End Source File
# Begin Source File

SOURCE=.\res\NavigateTo.Ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\node_properties.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\object_properties.ico
# End Source File
# Begin Source File

SOURCE=.\res\pencil.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\Point_light.ico
# End Source File
# Begin Source File

SOURCE=.\res\pointer.cur
# End Source File
# Begin Source File

SOURCE=.\res\pointer1.cur
# End Source File
# Begin Source File

SOURCE=.\res\pointer2.cur
# End Source File
# Begin Source File

SOURCE=.\res\icons\recalc_all_lights.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\Region.ico
# End Source File
# Begin Source File

SOURCE=.\Resource.h

!IF  "$(CFG)" == "SiegeEditor - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Making help include file...
TargetName=SiegeEditor
InputPath=.\Resource.h

"hlp\$(TargetName).hm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo. >"hlp\$(TargetName).hm" 
	echo // Commands (ID_* and IDM_*) >>"hlp\$(TargetName).hm" 
	makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Prompts (IDP_*) >>"hlp\$(TargetName).hm" 
	makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Resources (IDR_*) >>"hlp\$(TargetName).hm" 
	makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Dialogs (IDD_*) >>"hlp\$(TargetName).hm" 
	makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Frame Controls (IDW_*) >>"hlp\$(TargetName).hm" 
	makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\$(TargetName).hm" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "SiegeEditor - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Making help include file...
TargetName=SiegeEditorD
InputPath=.\Resource.h

"hlp\$(TargetName).hm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo. >"hlp\$(TargetName).hm" 
	echo // Commands (ID_* and IDM_*) >>"hlp\$(TargetName).hm" 
	makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Prompts (IDP_*) >>"hlp\$(TargetName).hm" 
	makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Resources (IDR_*) >>"hlp\$(TargetName).hm" 
	makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Dialogs (IDD_*) >>"hlp\$(TargetName).hm" 
	makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Frame Controls (IDW_*) >>"hlp\$(TargetName).hm" 
	makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\$(TargetName).hm" 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\res\icons\rotate_mode.ico
# End Source File
# Begin Source File

SOURCE=.\res\se.ico
# End Source File
# Begin Source File

SOURCE=.\res\search.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\select_all_nodes.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\set_target_node.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\side_camera.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\Siege_node.ico
# End Source File
# Begin Source File

SOURCE=.\res\SiegeEditor.ico
# End Source File
# Begin Source File

SOURCE=.\res\SiegeEditor.rc2
# End Source File
# Begin Source File

SOURCE=.\res\SiegeEditorDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\splashedit.bmp
# End Source File
# Begin Source File

SOURCE=.\Splsh16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icons\Spot_light.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\Start_position.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\target_node_camera_view.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\toggle_paint_select.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\toggle_placement_mode.ico
# End Source File
# Begin Source File

SOURCE=.\res\Tool5.ico
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\toolbar1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Tools.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\top_down_camera.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\view_mesh_list.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\view_node_list.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\view_stitch_manager.ico
# End Source File
# End Group
# Begin Group "Help Files"

# PROP Default_Filter "cnt;rtf"
# Begin Source File

SOURCE=.\hlp\AfxCore.rtf
# End Source File
# Begin Source File

SOURCE=.\hlp\AppExit.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Bullet.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurArw2.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurArw4.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurHelp.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditCopy.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditCut.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditPast.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditUndo.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileNew.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileOpen.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FilePrnt.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileSave.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\HlpSBar.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\HlpTBar.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecFirst.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecLast.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecNext.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecPrev.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Scmax.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\ScMenu.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Scmin.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\SiegeEditor.cnt

!IF  "$(CFG)" == "SiegeEditor - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Copying contents file...
OutDir=\temp\vc6\siegeeditor\Release
InputPath=.\hlp\SiegeEditor.cnt
InputName=SiegeEditor

"$(OutDir)\$(InputName).cnt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "hlp\$(InputName).cnt" $(OutDir)

# End Custom Build

!ELSEIF  "$(CFG)" == "SiegeEditor - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Copying contents file...
OutDir=\temp\vc6\siegeeditor\Debug
InputPath=.\hlp\SiegeEditor.cnt
InputName=SiegeEditor

"$(OutDir)\$(InputName).cnt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "hlp\$(InputName).cnt" $(OutDir)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "SE Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SiegeEditor.rc
# End Source File
# End Group
# Begin Group "Side Panel"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\LeftView.cpp
# End Source File
# Begin Source File

SOURCE=.\LeftView.h
# End Source File
# End Group
# Begin Group "Bottom Panel"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BottomView.cpp
# End Source File
# Begin Source File

SOURCE=.\BottomView.h
# End Source File
# End Group
# Begin Group "Menu Commands"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\MenuCommands.cpp
# End Source File
# Begin Source File

SOURCE=.\MenuCommands.h
# End Source File
# End Group
# Begin Group "Keyboard Mapper"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\KeyMapper.cpp
# End Source File
# Begin Source File

SOURCE=.\KeyMapper.h
# End Source File
# End Group
# Begin Group "Custom Viewer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CustomViewer.cpp
# End Source File
# Begin Source File

SOURCE=.\CustomViewer.h
# End Source File
# End Group
# Begin Group "Mesh Previewer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Window_Mesh_Previewer.cpp
# End Source File
# Begin Source File

SOURCE=.\Window_Mesh_Previewer.h
# End Source File
# End Group
# Begin Group "Controls"

# PROP Default_Filter ""
# Begin Group "Grid Control"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\gridctrl\BtnDataBase.cpp
# End Source File
# Begin Source File

SOURCE=.\gridctrl\BtnDataBase.h
# End Source File
# Begin Source File

SOURCE=.\gridctrl\CellRange.h
# End Source File
# Begin Source File

SOURCE=.\gridctrl\GridBtnCell.cpp
# End Source File
# Begin Source File

SOURCE=.\gridctrl\GridBtnCell.h
# End Source File
# Begin Source File

SOURCE=.\gridctrl\GridBtnCellBase.cpp
# End Source File
# Begin Source File

SOURCE=.\gridctrl\GridBtnCellBase.h
# End Source File
# Begin Source File

SOURCE=.\gridctrl\GridBtnCellCombo.cpp
# End Source File
# Begin Source File

SOURCE=.\gridctrl\GridBtnCellCombo.h
# End Source File
# Begin Source File

SOURCE=.\gridctrl\GridCell.cpp
# End Source File
# Begin Source File

SOURCE=.\gridctrl\GridCell.h
# End Source File
# Begin Source File

SOURCE=.\gridctrl\GridCellBase.cpp
# End Source File
# Begin Source File

SOURCE=.\gridctrl\GridCellBase.h
# End Source File
# Begin Source File

SOURCE=.\gridctrl\GridCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\gridctrl\GridCtrl.h
# End Source File
# Begin Source File

SOURCE=.\gridctrl\GridDropTarget.cpp
# End Source File
# Begin Source File

SOURCE=.\gridctrl\GridDropTarget.h
# End Source File
# Begin Source File

SOURCE=.\gridctrl\InPlaceEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\gridctrl\InPlaceEdit.h
# End Source File
# Begin Source File

SOURCE=.\gridctrl\InPlaceList.cpp
# End Source File
# Begin Source File

SOURCE=.\gridctrl\InPlaceList.h
# End Source File
# Begin Source File

SOURCE=.\gridctrl\MemDC.h
# End Source File
# Begin Source File

SOURCE=.\gridctrl\TitleTip.cpp
# End Source File
# Begin Source File

SOURCE=.\gridctrl\TitleTip.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\listeditctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\listeditctrl.h
# End Source File
# Begin Source File

SOURCE=.\PropertyGrid.cpp
# End Source File
# Begin Source File

SOURCE=.\PropertyGrid.h
# End Source File
# Begin Source File

SOURCE=.\ResizableDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\ResizableDialog.h
# End Source File
# Begin Source File

SOURCE=.\ResizablePage.cpp
# End Source File
# Begin Source File

SOURCE=.\ResizablePage.h
# End Source File
# Begin Source File

SOURCE=.\ResizableSheet.cpp
# End Source File
# Begin Source File

SOURCE=.\ResizableSheet.h
# End Source File
# Begin Source File

SOURCE=.\ResizeCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\ResizeCtrl.h
# End Source File
# Begin Source File

SOURCE=.\RoundSliderCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\RoundSliderCtrl.h
# End Source File
# End Group
# Begin Group "Splash"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Splash.cpp
# End Source File
# Begin Source File

SOURCE=.\Splash.h
# End Source File
# End Group
# Begin Group "Camera Controller"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Camera_Controller.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Camera_Controller.h
# End Source File
# End Group
# End Group
# Begin Group "Terrain"

# PROP Default_Filter ""
# Begin Group "Logical Node Creator"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\LogicalNodeCreator.cpp
# End Source File
# Begin Source File

SOURCE=.\LogicalNodeCreator.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\EditorTerrain.cpp
# End Source File
# Begin Source File

SOURCE=.\EditorTerrain.h
# End Source File
# End Group
# Begin Group "Objects"

# PROP Default_Filter ""
# Begin Group "Triggers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\EditorTriggers.cpp
# End Source File
# Begin Source File

SOURCE=.\EditorTriggers.h
# End Source File
# End Group
# Begin Group "Gizmo Manager"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\GizmoManager.cpp
# End Source File
# Begin Source File

SOURCE=.\GizmoManager.h
# End Source File
# End Group
# Begin Group "SCID Manager"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SCIDManager.cpp
# End Source File
# Begin Source File

SOURCE=.\SCIDManager.h
# End Source File
# End Group
# Begin Group "Game Objects"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\EditorObjects.cpp
# End Source File
# Begin Source File

SOURCE=.\EditorObjects.h
# End Source File
# End Group
# Begin Group "Gizmos"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\EditorGizmos.cpp
# End Source File
# Begin Source File

SOURCE=.\EditorGizmos.h
# End Source File
# End Group
# End Group
# Begin Group "Lights"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\EditorLights.cpp
# End Source File
# Begin Source File

SOURCE=.\EditorLights.h
# End Source File
# End Group
# Begin Group "Region"

# PROP Default_Filter ""
# Begin Group "World Index Builder"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\worldindexbuilder.cpp
# End Source File
# Begin Source File

SOURCE=.\worldindexbuilder.h
# End Source File
# End Group
# Begin Group "Region Hotpoints"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\EditorHotpoints.cpp
# End Source File
# Begin Source File

SOURCE=.\EditorHotpoints.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\EditorRegion.cpp
# End Source File
# Begin Source File

SOURCE=.\EditorRegion.h
# End Source File
# End Group
# Begin Group "Map"

# PROP Default_Filter ""
# Begin Group "Stitch Helper"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Stitch_Helper.cpp
# End Source File
# Begin Source File

SOURCE=.\Stitch_Helper.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\EditorMap.cpp
# End Source File
# Begin Source File

SOURCE=.\EditorMap.h
# End Source File
# End Group
# Begin Group "Camera"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\EditorCamera.cpp
# End Source File
# Begin Source File

SOURCE=.\EditorCamera.h
# End Source File
# End Group
# Begin Group "Component List"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ComponentList.cpp
# End Source File
# Begin Source File

SOURCE=.\ComponentList.h
# End Source File
# End Group
# Begin Group "Previewer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\MeshPreviewer.cpp
# End Source File
# Begin Source File

SOURCE=.\MeshPreviewer.h
# End Source File
# End Group
# Begin Group "Precompiled Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\PrecompEditor.cpp
# ADD CPP /Yc"PrecompEditor.h"
# End Source File
# Begin Source File

SOURCE=.\PrecompEditor.h
# End Source File
# End Group
# Begin Group "Command Actions"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CommandActions.cpp
# End Source File
# Begin Source File

SOURCE=.\CommandActions.h
# End Source File
# End Group
# Begin Group "Tuning"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\EditorTuning.cpp
# End Source File
# Begin Source File

SOURCE=.\EditorTuning.h
# End Source File
# End Group
# Begin Group "Special Effects"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\EditorSfx.cpp
# End Source File
# Begin Source File

SOURCE=.\EditorSfx.h
# End Source File
# End Group
# Begin Group "Tank Building"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Build_Tank.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Build_Tank.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Extract_Tank.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Extract_Tank.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Save_As_DSMap.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Save_As_DSMap.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_Save_As_DSMod.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Save_As_DSMod.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\build_incrementer.dsm
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
# Section SiegeEditor : {72ADFD78-2C39-11D0-9903-00A0C91BC942}
# 	1:10:IDB_SPLASH:115
# 	2:21:SplashScreenInsertKey:4.0
# End Section
