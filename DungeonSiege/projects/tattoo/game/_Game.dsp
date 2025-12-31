# Microsoft Developer Studio Project File - Name="_Game" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=_Game - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "_Game.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "_Game.mak" CFG="_Game - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "_Game - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "_Game - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "_Game - Win32 Retail" (based on "Win32 (x86) Static Library")
!MESSAGE "_Game - Win32 Profiling" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/Projects/Tattoo/Game", YOVAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "_Game - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\tattoo\_Game\Release"
# PROP Intermediate_Dir "\temp\vc6\tattoo\_Game\Release"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"precomp_game.h" /FD /Zm500 /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "_Game - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\tattoo\_Game\Debug"
# PROP Intermediate_Dir "\temp\vc6\tattoo\_Game\Debug"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /Zi /Od /D "_DEBUG" /D "WIN32" /Yu"precomp_game.h" /FD /GZ /Zm500 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "_Game - Win32 Retail"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Retail"
# PROP BASE Intermediate_Dir "Retail"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\tattoo\_Game\Retail"
# PROP Intermediate_Dir "\temp\vc6\tattoo\_Game\Retail"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MT /W3 /O2 /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Oy /Ob1 /Gf /Gy /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /Yu"precomp_game.h" /FD /Gs /Zm500 /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "_Game - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Profiling"
# PROP BASE Intermediate_Dir "Profiling"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\_Game\Profiling"
# PROP Intermediate_Dir "\temp\vc6\lib\_Game\Profiling"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"precomp_game.h" /FD /Zm500 /Gs /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /D GP_PROFILING=1 /Yu"precomp_game.h" /FD /Zm500 /Gs /Gh /c
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

# Name "_Game - Win32 Release"
# Name "_Game - Win32 Debug"
# Name "_Game - Win32 Retail"
# Name "_Game - Win32 Profiling"
# Begin Group "Game"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\GameMovie.cpp
# End Source File
# Begin Source File

SOURCE=.\GameMovie.h
# End Source File
# Begin Source File

SOURCE=.\GamePersist.cpp
# End Source File
# Begin Source File

SOURCE=.\GamePersist.h
# End Source File
# Begin Source File

SOURCE=.\GameState.cpp
# End Source File
# Begin Source File

SOURCE=.\GameState.h
# End Source File
# Begin Source File

SOURCE=.\TattooGame.cpp
# End Source File
# Begin Source File

SOURCE=.\TattooGame.h
# End Source File
# End Group
# Begin Group "UI"

# PROP Default_Filter ""
# Begin Group "Frontend"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\UICharacterSelect.cpp
# End Source File
# Begin Source File

SOURCE=.\UICharacterSelect.h
# End Source File
# Begin Source File

SOURCE=.\UIFrontend.cpp
# End Source File
# Begin Source File

SOURCE=.\UIFrontend.h
# End Source File
# Begin Source File

SOURCE=.\UIOptions.cpp
# End Source File
# Begin Source File

SOURCE=.\UIOptions.h
# End Source File
# End Group
# Begin Group "Console"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\UIGame_console_commands.cpp
# End Source File
# Begin Source File

SOURCE=.\UIGame_console_commands.h
# End Source File
# Begin Source File

SOURCE=.\UIGameConsole.cpp
# End Source File
# Begin Source File

SOURCE=.\UIGameConsole.h
# End Source File
# Begin Source File

SOURCE=.\UIUserConsole.cpp
# End Source File
# Begin Source File

SOURCE=.\UIUserConsole.h
# End Source File
# End Group
# Begin Group "In Game"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\UICamera.cpp
# End Source File
# Begin Source File

SOURCE=.\UICamera.h
# End Source File
# Begin Source File

SOURCE=.\UICharacterDisplay.cpp
# End Source File
# Begin Source File

SOURCE=.\UICharacterDisplay.h
# End Source File
# Begin Source File

SOURCE=.\UICommands.cpp
# End Source File
# Begin Source File

SOURCE=.\UICommands.h
# End Source File
# Begin Source File

SOURCE=.\UIDialogueHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\UIDialogueHandler.h
# End Source File
# Begin Source File

SOURCE=.\UIGame.cpp
# End Source File
# Begin Source File

SOURCE=.\UIGame.h
# End Source File
# Begin Source File

SOURCE=.\UIInventoryManager.cpp
# End Source File
# Begin Source File

SOURCE=.\UIInventoryManager.h
# End Source File
# Begin Source File

SOURCE=.\UIItemOverlay.cpp
# End Source File
# Begin Source File

SOURCE=.\UIItemOverlay.h
# End Source File
# Begin Source File

SOURCE=.\UIMenuManager.cpp
# End Source File
# Begin Source File

SOURCE=.\UIMenuManager.h
# End Source File
# Begin Source File

SOURCE=.\UIPartyManager.cpp
# End Source File
# Begin Source File

SOURCE=.\UIPartyManager.h
# End Source File
# Begin Source File

SOURCE=.\UIPartyMember.cpp
# End Source File
# Begin Source File

SOURCE=.\UIPartyMember.h
# End Source File
# Begin Source File

SOURCE=.\UIPlayerRanks.cpp
# End Source File
# Begin Source File

SOURCE=.\UIPlayerRanks.h
# End Source File
# Begin Source File

SOURCE=.\UIStoreManager.cpp
# End Source File
# Begin Source File

SOURCE=.\UIStoreManager.h
# End Source File
# Begin Source File

SOURCE=.\UITeamManager.cpp
# End Source File
# Begin Source File

SOURCE=.\UITeamManager.h
# End Source File
# End Group
# Begin Group "Multiplayer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\UIMultiplayer.cpp
# End Source File
# Begin Source File

SOURCE=.\UIMultiplayer.h
# End Source File
# Begin Source File

SOURCE=.\UIEmoteList.cpp
# End Source File
# Begin Source File

SOURCE=.\UIEmoteList.h
# End Source File
# Begin Source File

SOURCE=.\UIZoneMatch.cpp
# End Source File
# Begin Source File

SOURCE=.\UIZoneMatch.h
# End Source File
# Begin Source File

SOURCE=.\UIZoneMatchFilter.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\zping.cpp
# End Source File
# Begin Source File

SOURCE=.\zping.h
# End Source File
# End Group
# Begin Group "Intro"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\UIIntro.cpp
# End Source File
# Begin Source File

SOURCE=.\UIIntro.h
# End Source File
# End Group
# Begin Group "Outro"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\UIOutro.cpp
# End Source File
# Begin Source File

SOURCE=.\UIOutro.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\UI.CPP
# End Source File
# Begin Source File

SOURCE=.\UI.H
# End Source File
# End Group
# Begin Group "Precompiled Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\precomp_game.cpp
# ADD CPP /Yc"precomp_game.h"
# End Source File
# Begin Source File

SOURCE=.\precomp_game.h
# End Source File
# End Group
# End Target
# End Project
