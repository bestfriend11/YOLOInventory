# Microsoft Developer Studio Project File - Name="QMem" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=QMem - Win32 Profiling
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "QMem.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "QMem.mak" CFG="QMem - Win32 Profiling"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "QMem - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "QMem - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "QMem - Win32 Retail" (based on "Win32 (x86) Static Library")
!MESSAGE "QMem - Win32 Profiling" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "QMem"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "QMem - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\QMem\Release"
# PROP Intermediate_Dir "\temp\vc6\lib\QMem\Release"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /FD /Gs /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "QMem - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\lib\QMem\Debug"
# PROP Intermediate_Dir "\temp\vc6\lib\QMem\Debug"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "QMem - Win32 Retail"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Retail"
# PROP BASE Intermediate_Dir "Retail"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\QMem\Retail"
# PROP Intermediate_Dir "\temp\vc6\lib\QMem\Retail"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Oy /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /FD /Gs /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Oy /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /D GP_RETAIL=1 /FD /Gs /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "QMem - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Profiling"
# PROP BASE Intermediate_Dir "Profiling"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\QMem\Profiling"
# PROP Intermediate_Dir "\temp\vc6\lib\QMem\Profiling"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /G6 /MT /W3 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /Yu"Precomp_Qmem.h" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /GX /Zi /Og /Oi /Os /Ob1 /D "NDEBUG" /D "WIN32" /D GP_PROFILING=1 /Yu"Precomp_Qmem.h" /FD /c
# SUBTRACT CPP /Ox
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

# Name "QMem - Win32 Release"
# Name "QMem - Win32 Debug"
# Name "QMem - Win32 Retail"
# Name "QMem - Win32 Profiling"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\QMem.cpp
# End Source File
# Begin Source File

SOURCE=.\QMemAsm.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\QMemLogDb.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\QMem.h
# End Source File
# Begin Source File

SOURCE=.\QMemLogDb.h
# End Source File
# Begin Source File

SOURCE=.\QProf.h
# End Source File
# End Group
# Begin Group "Docs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\QProf.html
# End Source File
# End Group
# Begin Group "Pch"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Precomp_QMem.cpp
# ADD BASE CPP /Yc"Precomp_Qmem.h"
# ADD CPP /Yc"Precomp_Qmem.h"
# End Source File
# Begin Source File

SOURCE=.\Precomp_QMem.h
# End Source File
# End Group
# End Target
# End Project
