# Microsoft Developer Studio Project File - Name="GUIMaster" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=GUIMaster - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GUIMaster.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GUIMaster.mak" CFG="GUIMaster - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GUIMaster - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "GUIMaster - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/Projects/GUIMaster", BJEGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GUIMaster - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\guimaster\Release"
# PROP Intermediate_Dir "\temp\vc6\guimaster\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 shmfc4mt.lib shlw32mt.lib /nologo /subsystem:windows /machine:I386 /out:"Release/GUIMaster.exe" /force:multiple
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "GUIMaster - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\guimaster\Debug"
# PROP Intermediate_Dir "\temp\vc6\guimaster\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dinput.lib dxguid.lib dplayx.lib ddraw.lib d3dim.lib dsound.lib opengl32.lib glu32.lib glaux.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib dinput.lib dxguid.lib comctl32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"Debug/GUIMaster.exe" /pdbtype:sept /force:multiple
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "GUIMaster - Win32 Release"
# Name "GUIMaster - Win32 Debug"
# Begin Group "Windows"

# PROP Default_Filter ""
# Begin Group "Main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\GUIMaster.cpp
# End Source File
# Begin Source File

SOURCE=.\GUIMaster.h
# End Source File
# Begin Source File

SOURCE=.\GUIMasterDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\GUIMasterDoc.h
# End Source File
# Begin Source File

SOURCE=.\GUIMasterView.cpp
# End Source File
# Begin Source File

SOURCE=.\GUIMasterView.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\res\GUIMaster.ico
# End Source File
# Begin Source File

SOURCE=.\GUIMaster.rc
# End Source File
# Begin Source File

SOURCE=.\res\GUIMaster.rc2
# End Source File
# Begin Source File

SOURCE=.\res\GUIMasterDoc.ico
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# End Group
# Begin Group "Workspace"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\GUIWorkspace.cpp
# End Source File
# Begin Source File

SOURCE=.\GUIWorkspace.h
# End Source File
# End Group
# Begin Group "Menu"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Menu.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu.h
# End Source File
# End Group
# Begin Group "Dialogs"

# PROP Default_Filter ""
# Begin Group "Controls"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Controls.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Controls.h
# End Source File
# End Group
# Begin Group "Preferences"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Preferences.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Preferences.h
# End Source File
# End Group
# Begin Group "Create Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Create_Interface.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Create_Interface.h
# End Source File
# End Group
# Begin Group "Load Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Load_Interface.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Load_Interface.h
# End Source File
# End Group
# Begin Group "Startup"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Startup.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Startup.h
# End Source File
# End Group
# Begin Group "Interfaces"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Show_Interfaces.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Show_Interfaces.h
# End Source File
# End Group
# Begin Group "Properties"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Properties.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Properties.h
# End Source File
# End Group
# Begin Group "Common Settings"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Common_Settings.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Common_Settings.h
# End Source File
# End Group
# Begin Group "Group Settings"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Group.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Group.h
# End Source File
# End Group
# Begin Group "Text"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Text.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Text.h
# End Source File
# End Group
# Begin Group "Background"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_Create_Background.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog_Create_Background.h
# End Source File
# End Group
# End Group
# End Group
# Begin Group "Shell"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\GUIMasterShell.cpp
# End Source File
# Begin Source File

SOURCE=.\GUIMasterShell.h
# End Source File
# Begin Source File

SOURCE=.\GUIMasterTypes.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\res\toolbar1.bmp
# End Source File
# End Target
# End Project
