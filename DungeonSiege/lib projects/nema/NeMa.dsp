# Microsoft Developer Studio Project File - Name="NeMa" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=NeMa - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NeMa.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NeMa.mak" CFG="NeMa - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NeMa - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "NeMa - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "NeMa - Win32 Retail" (based on "Win32 (x86) Static Library")
!MESSAGE "NeMa - Win32 Profiling" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/LIB Projects/NeMa", ARSAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NeMa - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\nema\Release"
# PROP Intermediate_Dir "\temp\vc6\lib\nema\Release"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"precomp_nema.h" /FD /Zm500 /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "NeMa - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\lib\nema\Debug"
# PROP Intermediate_Dir "\temp\vc6\lib\nema\Debug"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /Zi /Od /D "_DEBUG" /D "WIN32" /Yu"precomp_nema.h" /FD /GZ /Zm500 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "NeMa - Win32 Retail"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Retail"
# PROP BASE Intermediate_Dir "Retail"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\NeMa\Retail"
# PROP Intermediate_Dir "\temp\vc6\lib\NeMa\Retail"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MT /W3 /O2 /Ob2 /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /Yu"precomp_nema.h" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Oy /Ob1 /Gf /Gy /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /Yu"precomp_nema.h" /FD /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "NeMa - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Profiling"
# PROP BASE Intermediate_Dir "Profiling"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\NeMa\Profiling"
# PROP Intermediate_Dir "\temp\vc6\lib\NeMa\Profiling"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"precomp_nema.h" /FD /Zm500 /Gs /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /D GP_PROFILING=1 /Yu"precomp_nema.h" /FD /Zm500 /Gs /Gh /c
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

# Name "NeMa - Win32 Release"
# Name "NeMa - Win32 Debug"
# Name "NeMa - Win32 Retail"
# Name "NeMa - Win32 Profiling"
# Begin Group "precompiled header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\precomp_nema.cpp

!IF  "$(CFG)" == "NeMa - Win32 Release"

# PROP Intermediate_Dir "\temp\vc6\lib\nema\Release"
# ADD CPP /Yc"precomp_nema.h"

!ELSEIF  "$(CFG)" == "NeMa - Win32 Debug"

# PROP Intermediate_Dir "\temp\vc6\lib\nema\Debug"
# ADD CPP /Yc"precomp_nema.h"

!ELSEIF  "$(CFG)" == "NeMa - Win32 Retail"

# PROP Intermediate_Dir "\temp\vc6\lib\nema\Retail"
# ADD CPP /Yc"precomp_nema.h"

!ELSEIF  "$(CFG)" == "NeMa - Win32 Profiling"

# PROP Intermediate_Dir "\temp\vc6\lib\nema\Profiling"
# ADD CPP /Yc"precomp_nema.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\precomp_nema.h

!IF  "$(CFG)" == "NeMa - Win32 Release"

!ELSEIF  "$(CFG)" == "NeMa - Win32 Debug"

!ELSEIF  "$(CFG)" == "NeMa - Win32 Retail"

!ELSEIF  "$(CFG)" == "NeMa - Win32 Profiling"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\nema_aspect.cpp
# End Source File
# Begin Source File

SOURCE=.\nema_aspect.h
# End Source File
# Begin Source File

SOURCE=.\nema_aspectmgr.h
# End Source File
# Begin Source File

SOURCE=.\nema_blender.cpp
# End Source File
# Begin Source File

SOURCE=.\nema_blender.h
# End Source File
# Begin Source File

SOURCE=.\nema_chore.cpp
# End Source File
# Begin Source File

SOURCE=.\nema_chore.h
# End Source File
# Begin Source File

SOURCE=.\nema_iostructs.h
# End Source File
# Begin Source File

SOURCE=.\nema_kevents.cpp
# End Source File
# Begin Source File

SOURCE=.\nema_kevents.h
# End Source File
# Begin Source File

SOURCE=.\nema_keymgr.cpp
# End Source File
# Begin Source File

SOURCE=.\nema_keymgr.h
# End Source File
# Begin Source File

SOURCE=.\nema_persist.cpp
# End Source File
# Begin Source File

SOURCE=.\nema_persist.h
# End Source File
# Begin Source File

SOURCE=.\nema_prskeys.cpp
# End Source File
# Begin Source File

SOURCE=.\nema_prskeys.h
# End Source File
# Begin Source File

SOURCE=.\nema_skritsupport.cpp
# End Source File
# Begin Source File

SOURCE=.\nema_skritsupport.h
# End Source File
# Begin Source File

SOURCE=.\nema_types.h
# End Source File
# End Target
# End Project
