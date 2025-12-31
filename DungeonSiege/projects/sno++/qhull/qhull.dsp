# Microsoft Developer Studio Project File - Name="qhull" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=qhull - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "qhull.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "qhull.mak" CFG="qhull - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "qhull - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "qhull - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/LIB Projects/qhull", SDFAAAAA"
# PROP Scc_LocalPath "."
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "qhull - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\sno\lib\qhull\Release"
# PROP Intermediate_Dir "\temp\vc6\sno\lib\qhull\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "qhull - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\sno\lib\qhull\Debug"
# PROP Intermediate_Dir "\temp\vc6\sno\lib\qhull\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Z7 /Od /I "E:\GPG_HOME\GPG\Projects\SNO_30\Math" /D "_DEBUG" /D "STRICT" /D "WIN32" /D "_WINDOWS" /D "MAX_EXPORTER" /YX /FD /c
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

# Name "qhull - Win32 Release"
# Name "qhull - Win32 Debug"
# Begin Source File

SOURCE=.\qh_geom.c
# End Source File
# Begin Source File

SOURCE=.\qh_geom.h
# End Source File
# Begin Source File

SOURCE=.\qh_geom2.c
# End Source File
# Begin Source File

SOURCE=.\qh_global.c
# End Source File
# Begin Source File

SOURCE=.\qh_io.c
# End Source File
# Begin Source File

SOURCE=.\qh_io.h
# End Source File
# Begin Source File

SOURCE=.\qh_mem.c
# End Source File
# Begin Source File

SOURCE=.\qh_mem.h
# End Source File
# Begin Source File

SOURCE=.\qh_merge.c
# End Source File
# Begin Source File

SOURCE=.\qh_merge.h
# End Source File
# Begin Source File

SOURCE=.\qh_poly.c
# End Source File
# Begin Source File

SOURCE=.\qh_poly.h
# End Source File
# Begin Source File

SOURCE=.\qh_poly2.c
# End Source File
# Begin Source File

SOURCE=.\qh_set.c
# End Source File
# Begin Source File

SOURCE=.\qh_set.h
# End Source File
# Begin Source File

SOURCE=.\qh_stat.c
# End Source File
# Begin Source File

SOURCE=.\qh_stat.h
# End Source File
# Begin Source File

SOURCE=.\qh_user.c
# End Source File
# Begin Source File

SOURCE=.\qh_user.h
# End Source File
# Begin Source File

SOURCE=.\qhull.c
# End Source File
# Begin Source File

SOURCE=.\qhull.h
# End Source File
# Begin Source File

SOURCE=.\qhull_a.h
# End Source File
# End Target
# End Project
