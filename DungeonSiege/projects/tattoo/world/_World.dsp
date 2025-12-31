# Microsoft Developer Studio Project File - Name="_World" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=_World - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "_World.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "_World.mak" CFG="_World - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "_World - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "_World - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "_World - Win32 Retail" (based on "Win32 (x86) Static Library")
!MESSAGE "_World - Win32 Profiling" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/Projects/Tattoo/World", ZOVAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "_World - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\tattoo\_World\Release"
# PROP Intermediate_Dir "\temp\vc6\tattoo\_World\Release"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"Precomp_World.h" /FD /Zm600 /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "_World - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\tattoo\_World\Debug"
# PROP Intermediate_Dir "\temp\vc6\tattoo\_World\Debug"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /Zi /Od /D "_DEBUG" /D "WIN32" /Yu"Precomp_World.h" /FD /GZ /Zm600 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "_World - Win32 Retail"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Retail"
# PROP BASE Intermediate_Dir "Retail"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\tattoo\_World\Retail"
# PROP Intermediate_Dir "\temp\vc6\tattoo\_World\Retail"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MT /W3 /O2 /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /Yu"Precomp_World.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Oy /Ob1 /Gf /Gy /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /Yu"Precomp_World.h" /FD /Gs /Zm600 /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "_World - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Profiling"
# PROP BASE Intermediate_Dir "Profiling"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\_World\Profiling"
# PROP Intermediate_Dir "\temp\vc6\lib\_World\Profiling"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"Precomp_World.h" /FD /Zm600 /Gs /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /D GP_PROFILING=1 /Yu"Precomp_World.h" /FD /Zm600 /Gs /Gh /c
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

# Name "_World - Win32 Release"
# Name "_World - Win32 Debug"
# Name "_World - Win32 Retail"
# Name "_World - Win32 Profiling"
# Begin Group "Ai"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AiAction.cpp
# End Source File
# Begin Source File

SOURCE=.\AiAction.h
# End Source File
# Begin Source File

SOURCE=.\AiQuery.cpp
# End Source File
# Begin Source File

SOURCE=.\AiQuery.h
# End Source File
# Begin Source File

SOURCE=.\Job\Job.cpp
# End Source File
# Begin Source File

SOURCE=.\Job\Job.h
# End Source File
# Begin Source File

SOURCE=.\MCP.cpp
# End Source File
# Begin Source File

SOURCE=.\MCP.h
# End Source File
# Begin Source File

SOURCE=.\MCP_Defs.h
# End Source File
# Begin Source File

SOURCE=.\MCP_Pipeline.cpp
# End Source File
# Begin Source File

SOURCE=.\MCP_Plan.cpp
# End Source File
# Begin Source File

SOURCE=.\MCP_Request.cpp
# End Source File
# Begin Source File

SOURCE=.\MCP_Request.h
# End Source File
# Begin Source File

SOURCE=.\MCP_Sequencer.cpp
# End Source File
# End Group
# Begin Group "Camera"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CameraAgent.cpp
# End Source File
# Begin Source File

SOURCE=.\CameraAgent.h
# End Source File
# Begin Source File

SOURCE=.\CameraPosition.h
# End Source File
# End Group
# Begin Group "Environment"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Mood.cpp
# End Source File
# Begin Source File

SOURCE=.\Mood.h
# End Source File
# Begin Source File

SOURCE=.\TimeOfDay.cpp
# End Source File
# Begin Source File

SOURCE=.\TimeOfDay.h
# End Source File
# Begin Source File

SOURCE=.\Trigger_Actions.cpp
# End Source File
# Begin Source File

SOURCE=.\Trigger_Actions.h
# End Source File
# Begin Source File

SOURCE=.\Trigger_Conditions.cpp
# End Source File
# Begin Source File

SOURCE=.\Trigger_Conditions.h
# End Source File
# Begin Source File

SOURCE=.\Trigger_Defs.h
# End Source File
# Begin Source File

SOURCE=.\Trigger_Groups.cpp
# End Source File
# Begin Source File

SOURCE=.\Trigger_Groups.h
# End Source File
# Begin Source File

SOURCE=.\Trigger_Sys.cpp
# End Source File
# Begin Source File

SOURCE=.\Trigger_Sys.h
# End Source File
# Begin Source File

SOURCE=.\Weather.cpp
# End Source File
# Begin Source File

SOURCE=.\Weather.h
# End Source File
# End Group
# Begin Group "Go"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ContentDb.cpp
# End Source File
# Begin Source File

SOURCE=.\ContentDb.h
# End Source File
# Begin Source File

SOURCE=.\Go.cpp
# End Source File
# Begin Source File

SOURCE=.\Go.h
# End Source File
# Begin Source File

SOURCE=.\GoBase.cpp
# End Source File
# Begin Source File

SOURCE=.\GoBase.h
# End Source File
# Begin Source File

SOURCE=.\GoData.cpp
# End Source File
# Begin Source File

SOURCE=.\GoData.h
# End Source File
# Begin Source File

SOURCE=.\GoDb.cpp
# End Source File
# Begin Source File

SOURCE=.\GoDb.h
# End Source File
# Begin Source File

SOURCE=.\GoDefs.cpp
# End Source File
# Begin Source File

SOURCE=.\GoDefs.h
# End Source File
# Begin Source File

SOURCE=.\GoRpc.cpp
# End Source File
# Begin Source File

SOURCE=.\GoRpc.h
# End Source File
# Begin Source File

SOURCE=.\GoSupport.cpp
# End Source File
# Begin Source File

SOURCE=.\GoSupport.h
# End Source File
# Begin Source File

SOURCE=.\PContentDb.cpp
# End Source File
# Begin Source File

SOURCE=.\PContentDb.h
# End Source File
# End Group
# Begin Group "Go Components"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\GoActor.cpp
# End Source File
# Begin Source File

SOURCE=.\GoActor.h
# End Source File
# Begin Source File

SOURCE=.\GoAspect.cpp
# End Source File
# Begin Source File

SOURCE=.\GoAspect.h
# End Source File
# Begin Source File

SOURCE=.\GoAttack.cpp
# End Source File
# Begin Source File

SOURCE=.\GoAttack.h
# End Source File
# Begin Source File

SOURCE=.\GoBody.cpp
# End Source File
# Begin Source File

SOURCE=.\GoBody.h
# End Source File
# Begin Source File

SOURCE=.\GoConversation.cpp
# End Source File
# Begin Source File

SOURCE=.\GoConversation.h
# End Source File
# Begin Source File

SOURCE=.\GoCore.cpp
# End Source File
# Begin Source File

SOURCE=.\GoCore.h
# End Source File
# Begin Source File

SOURCE=.\GoDefend.cpp
# End Source File
# Begin Source File

SOURCE=.\GoDefend.h
# End Source File
# Begin Source File

SOURCE=.\GoEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\GoEdit.h
# End Source File
# Begin Source File

SOURCE=.\GoFollower.cpp
# End Source File
# Begin Source File

SOURCE=.\GoFollower.h
# End Source File
# Begin Source File

SOURCE=.\GoGizmo.cpp
# End Source File
# Begin Source File

SOURCE=.\GoGizmo.h
# End Source File
# Begin Source File

SOURCE=.\GoInventory.cpp
# End Source File
# Begin Source File

SOURCE=.\GoInventory.h
# End Source File
# Begin Source File

SOURCE=.\GoMagic.cpp
# End Source File
# Begin Source File

SOURCE=.\GoMagic.h
# End Source File
# Begin Source File

SOURCE=.\GoMind.cpp
# End Source File
# Begin Source File

SOURCE=.\GoMind.h
# End Source File
# Begin Source File

SOURCE=.\GoParty.cpp
# End Source File
# Begin Source File

SOURCE=.\GoParty.h
# End Source File
# Begin Source File

SOURCE=.\GoPhysics.cpp
# End Source File
# Begin Source File

SOURCE=.\GoPhysics.h
# End Source File
# Begin Source File

SOURCE=.\GoShmo.cpp
# End Source File
# Begin Source File

SOURCE=.\GoShmo.h
# End Source File
# Begin Source File

SOURCE=.\GoStash.cpp
# End Source File
# Begin Source File

SOURCE=.\GoStash.h
# End Source File
# Begin Source File

SOURCE=.\GoStore.cpp
# End Source File
# Begin Source File

SOURCE=.\GoStore.h
# End Source File
# End Group
# Begin Group "Physics"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\Sim\PhysCalc.cpp"
# End Source File
# Begin Source File

SOURCE=".\Sim\PhysCalc.h"
# End Source File
# Begin Source File

SOURCE=".\Sim\Sim.cpp"
# End Source File
# Begin Source File

SOURCE=".\Sim\Sim.h"
# End Source File
# Begin Source File

SOURCE=.\Sim\SimObj.cpp
# End Source File
# Begin Source File

SOURCE=".\Sim\SimObj.h"
# End Source File
# End Group
# Begin Group "Rules"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Enchantment.cpp
# End Source File
# Begin Source File

SOURCE=.\Enchantment.h
# End Source File
# Begin Source File

SOURCE=.\Enchantment.inc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\FormulaHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\FormulaHelper.h
# End Source File
# Begin Source File

SOURCE=.\GameAuditor.cpp
# End Source File
# Begin Source File

SOURCE=.\GameAuditor.h
# End Source File
# Begin Source File

SOURCE=.\Rules.cpp
# End Source File
# Begin Source File

SOURCE=.\Rules.h
# End Source File
# Begin Source File

SOURCE=.\victory.cpp
# End Source File
# Begin Source File

SOURCE=.\victory.h
# End Source File
# End Group
# Begin Group "Server"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\NetFuBi.cpp
# End Source File
# Begin Source File

SOURCE=.\NetFuBi.h
# End Source File
# Begin Source File

SOURCE=.\NetLog.cpp
# End Source File
# Begin Source File

SOURCE=.\NetLog.h
# End Source File
# Begin Source File

SOURCE=.\Player.cpp
# End Source File
# Begin Source File

SOURCE=.\Player.h
# End Source File
# Begin Source File

SOURCE=.\Server.cpp
# End Source File
# Begin Source File

SOURCE=.\Server.h
# End Source File
# End Group
# Begin Group "World"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BoneTranslator.cpp
# End Source File
# Begin Source File

SOURCE=.\BoneTranslator.h
# End Source File
# Begin Source File

SOURCE=.\Choreographer.cpp
# End Source File
# Begin Source File

SOURCE=.\Choreographer.h
# End Source File
# Begin Source File

SOURCE=.\DebugProbe.cpp
# End Source File
# Begin Source File

SOURCE=.\DebugProbe.h
# End Source File
# Begin Source File

SOURCE=.\GuiHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\GuiHelper.h
# End Source File
# Begin Source File

SOURCE=.\MessageDispatch.cpp
# End Source File
# Begin Source File

SOURCE=.\MessageDispatch.h
# End Source File
# Begin Source File

SOURCE=.\SkritBotMgr.cpp
# End Source File
# Begin Source File

SOURCE=.\SkritBotMgr.h
# End Source File
# Begin Source File

SOURCE=.\TimeMgr.cpp
# End Source File
# Begin Source File

SOURCE=.\TimeMgr.h
# End Source File
# Begin Source File

SOURCE=.\World.cpp
# End Source File
# Begin Source File

SOURCE=.\World.h
# End Source File
# Begin Source File

SOURCE=.\WorldFx.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldFx.h
# End Source File
# Begin Source File

SOURCE=.\WorldMap.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldMap.h
# End Source File
# Begin Source File

SOURCE=.\WorldMessage.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldMessage.h
# End Source File
# Begin Source File

SOURCE=.\WorldOptions.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldOptions.h
# End Source File
# Begin Source File

SOURCE=.\WorldPathFinder.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldPathFinder.h
# End Source File
# Begin Source File

SOURCE=.\WorldSound.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldSound.h
# End Source File
# Begin Source File

SOURCE=.\WorldState.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldState.h
# End Source File
# Begin Source File

SOURCE=.\WorldTerrain.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldTerrain.h
# End Source File
# Begin Source File

SOURCE=.\WorldTime.h
# End Source File
# End Group
# Begin Group "Precompiled Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Precomp_World.cpp
# ADD CPP /Yc"Precomp_World.h"
# End Source File
# Begin Source File

SOURCE=.\Precomp_World.h
# End Source File
# End Group
# End Target
# End Project
