# Microsoft Developer Studio Project File - Name="GameGUI" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=GameGUI - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GameGUI.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GameGUI.mak" CFG="GameGUI - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GameGUI - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "GameGUI - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "GameGUI - Win32 Retail" (based on "Win32 (x86) Static Library")
!MESSAGE "GameGUI - Win32 Profiling" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/LIB Projects/GameGUI", ABTAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GameGUI - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\gamegui\Release"
# PROP Intermediate_Dir "\temp\vc6\lib\gamegui\Release"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /Yu"precomp_gamegui.h" /FD /Zm500 /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "GameGUI - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\lib\gamegui\Debug"
# PROP Intermediate_Dir "\temp\vc6\lib\gamegui\Debug"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /Yu"precomp_gamegui.h" /FD /GZ /Zm500 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "GameGUI - Win32 Retail"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Retail"
# PROP BASE Intermediate_Dir "Retail"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\GameGUI\Retail"
# PROP Intermediate_Dir "\temp\vc6\lib\GameGUI\Retail"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MT /W3 /O2 /D "WIN32" /D "NDEBUG" /D GP_RETAIL=1 /Yu"precomp_gamegui.h" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Oy /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /D GP_RETAIL=1 /Yu"precomp_gamegui.h" /FD /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "GameGUI - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Profiling"
# PROP BASE Intermediate_Dir "Profiling"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\GameGUI\Profiling"
# PROP Intermediate_Dir "\temp\vc6\lib\GameGUI\Profiling"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /Yu"precomp_gamegui.h" /FD /Zm500 /Gs /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /D GP_PROFILING=1 /Yu"precomp_gamegui.h" /FD /Zm500 /Gs /Gh /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "GameGUI - Win32 Release"
# Name "GameGUI - Win32 Debug"
# Name "GameGUI - Win32 Retail"
# Name "GameGUI - Win32 Profiling"
# Begin Group "Shell"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ui_interface.h
# End Source File
# Begin Source File

SOURCE=.\ui_shell.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_shell.h
# End Source File
# Begin Source File

SOURCE=.\ui_types.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_types.h
# End Source File
# End Group
# Begin Group "UIWindow"

# PROP Default_Filter ""
# Begin Group "Base"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ui_scroll_window.h
# End Source File
# Begin Source File

SOURCE=.\ui_window.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_window.h
# End Source File
# End Group
# Begin Group "Controls"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ui_button.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_button.h
# End Source File
# Begin Source File

SOURCE=.\ui_chatbox.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_chatbox.h
# End Source File
# Begin Source File

SOURCE=.\ui_checkbox.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_checkbox.h
# End Source File
# Begin Source File

SOURCE=.\ui_combobox.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_combobox.h
# End Source File
# Begin Source File

SOURCE=.\ui_cursor.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_cursor.h
# End Source File
# Begin Source File

SOURCE=.\ui_dialogbox.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_dialogbox.h
# End Source File
# Begin Source File

SOURCE=.\ui_dockbar.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_dockbar.h
# End Source File
# Begin Source File

SOURCE=.\ui_editbox.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_editbox.h
# End Source File
# Begin Source File

SOURCE=.\ui_gridbox.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_gridbox.h
# End Source File
# Begin Source File

SOURCE=.\ui_infoslot.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_infoslot.h
# End Source File
# Begin Source File

SOURCE=.\ui_item.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_item.h
# End Source File
# Begin Source File

SOURCE=.\ui_itemslot.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_itemslot.h
# End Source File
# Begin Source File

SOURCE=.\ui_listbox.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_listbox.h
# End Source File
# Begin Source File

SOURCE=.\ui_listreport.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_listreport.h
# End Source File
# Begin Source File

SOURCE=.\ui_mouse_listener.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_mouse_listener.h
# End Source File
# Begin Source File

SOURCE=.\ui_multistagebutton.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_multistagebutton.h
# End Source File
# Begin Source File

SOURCE=.\ui_popupmenu.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_popupmenu.h
# End Source File
# Begin Source File

SOURCE=.\ui_radiobutton.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_radiobutton.h
# End Source File
# Begin Source File

SOURCE=.\ui_slider.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_slider.h
# End Source File
# Begin Source File

SOURCE=.\ui_statusbar.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_statusbar.h
# End Source File
# Begin Source File

SOURCE=.\ui_tab.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_tab.h
# End Source File
# Begin Source File

SOURCE=.\ui_text.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_text.h
# End Source File
# Begin Source File

SOURCE=.\ui_textbox.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_textbox.h
# End Source File
# End Group
# End Group
# Begin Group "Texture Management"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ui_textureman.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_textureman.h
# End Source File
# End Group
# Begin Group "Messenger"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ui_messenger.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_messenger.h
# End Source File
# End Group
# Begin Group "Emotes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ui_emotes.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_emotes.h
# End Source File
# End Group
# Begin Group "Animation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ui_animation.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_animation.h
# End Source File
# End Group
# Begin Group "Precompiled Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\precomp_gamegui.cpp
# ADD CPP /Yc"precomp_gamegui.h"
# End Source File
# Begin Source File

SOURCE=.\precomp_gamegui.h
# End Source File
# End Group
# End Target
# End Project
