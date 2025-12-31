# Microsoft Developer Studio Project File - Name="OBBTree" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=OBBTree - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "OBBTree.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "OBBTree.mak" CFG="OBBTree - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OBBTree - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "OBBTree - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "OBBTree - Win32 Retail" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/LIB Projects/OBBTree", ETFAAAAA"
# PROP Scc_LocalPath "."
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "OBBTree - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\obbtree\Release"
# PROP Intermediate_Dir "\temp\vc6\lib\obbtree\Release"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /O1 /Ob2 /D "WIN32" /D "NDEBUG" /Yu"precomp_obbtree.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "OBBTree - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\lib\obbtree\Debug"
# PROP Intermediate_Dir "\temp\vc6\lib\obbtree\Debug"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /YX /FD /c
# ADD CPP /nologo /MTd /W4 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /Yu"precomp_obbtree.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "OBBTree - Win32 Retail"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Retail"
# PROP BASE Intermediate_Dir "Retail"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\OBBTree\Retail"
# PROP Intermediate_Dir "\temp\vc6\lib\OBBTree\Retail"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MT /W3 /O2 /Ob0 /D "WIN32" /D "NDEBUG" /D GP_RETAIL=1 /Yu"precomp_obbtree.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /G6 /MT /W3 /Zi /O1 /Ob2 /D "WIN32" /D "NDEBUG" /D GP_RETAIL=1 /Yu"precomp_obbtree.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "OBBTree - Win32 Release"
# Name "OBBTree - Win32 Debug"
# Name "OBBTree - Win32 Retail"
# Begin Group "cpp"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\OBBTree.cpp
# End Source File
# End Group
# Begin Group "h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\OBBTree.h
# End Source File
# End Group
# Begin Group "Precompiled Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\precomp_obbtree.cpp
# ADD CPP /Yc"precomp_obbtree.h"
# End Source File
# Begin Source File

SOURCE=.\precomp_obbtree.h
# End Source File
# End Group
# End Target
# End Project
