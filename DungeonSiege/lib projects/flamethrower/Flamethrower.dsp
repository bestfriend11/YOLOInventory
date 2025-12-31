# Microsoft Developer Studio Project File - Name="Flamethrower" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Flamethrower - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Flamethrower.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Flamethrower.mak" CFG="Flamethrower - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Flamethrower - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Flamethrower - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "Flamethrower - Win32 Retail" (based on "Win32 (x86) Static Library")
!MESSAGE "Flamethrower - Win32 Profiling" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/LIB Projects/Flamethrower", JSJAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Flamethrower - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\flamethrower\Release"
# PROP Intermediate_Dir "\temp\vc6\lib\flamethrower\Release"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"precomp_flamethrower.h" /FD /Zm500 /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Flamethrower - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\lib\flamethrower\Debug"
# PROP Intermediate_Dir "\temp\vc6\lib\flamethrower\Debug"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /Zi /Od /D "_DEBUG" /D "WIN32" /Yu"precomp_flamethrower.h" /FD /GZ /Zm500 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Flamethrower - Win32 Retail"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Retail"
# PROP BASE Intermediate_Dir "Retail"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\Flamethrower\Retail"
# PROP Intermediate_Dir "\temp\vc6\lib\Flamethrower\Retail"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MT /W3 /O2 /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /Yu"precomp_flamethrower.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Oy /Ob1 /Gf /Gy /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /Yu"precomp_flamethrower.h" /FD /Gs /Zm500 /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Flamethrower - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Profiling"
# PROP BASE Intermediate_Dir "Profiling"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\Flamethrower\Profiling"
# PROP Intermediate_Dir "\temp\vc6\lib\Flamethrower\Profiling"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"precomp_flamethrower.h" /FD /Zm500 /Gs /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /D GP_PROFILING=1 /Yu"precomp_flamethrower.h" /FD /Zm500 /Gs /Gh /c
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

# Name "Flamethrower - Win32 Release"
# Name "Flamethrower - Win32 Debug"
# Name "Flamethrower - Win32 Retail"
# Name "Flamethrower - Win32 Profiling"
# Begin Group "Precompiled Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\precomp_flamethrower.cpp
# ADD CPP /Yc"precomp_flamethrower.h"
# End Source File
# Begin Source File

SOURCE=.\precomp_flamethrower.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Flamethrower.cpp
# End Source File
# Begin Source File

SOURCE=.\Flamethrower.h
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_effect_base.cpp
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_effect_base.h
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_effect_factory.cpp
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_effect_factory.h
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_effect_target.cpp
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_effect_target.h
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_effects.cpp
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_effects.h
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_effects2.cpp
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_effects2.h
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_effects3.cpp
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_effects3.h
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_FuBi_support.cpp
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_FuBi_support.h
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_interpreter.cpp
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_interpreter.h
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_interpreter_commands.cpp
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_interpreter_commands.h
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_interpreter_macros.cpp
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_interpreter_macros.h
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_script_helpers.cpp
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_script_helpers.h
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_tracker.cpp
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_tracker.h
# End Source File
# Begin Source File

SOURCE=.\Flamethrower_types.h
# End Source File
# End Target
# End Project
