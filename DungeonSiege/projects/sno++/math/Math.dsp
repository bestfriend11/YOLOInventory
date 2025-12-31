# Microsoft Developer Studio Project File - Name="Math" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Math - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Math.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Math.mak" CFG="Math - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Math - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Math - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/LIB Projects/Math", VVAAAAAA"
# PROP Scc_LocalPath ".."
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "Math - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\sno\lib\math\Release"
# PROP Intermediate_Dir "\temp\vc6\sno\lib\math\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"\temp\vc6\lib\math\Release\MathR.lib"

!ELSEIF  "$(CFG)" == "Math - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\sno\lib\math\Debug"
# PROP Intermediate_Dir "\temp\vc6\sno\lib\math\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MAX_EXPORTER" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"\temp\vc6\lib\math\Debug\MathD.lib"

!ENDIF 

# Begin Target

# Name "Math - Win32 Release"
# Name "Math - Win32 Debug"
# Begin Source File

SOURCE=.\axis_aligned_bounding_box_3.h
# End Source File
# Begin Source File

SOURCE=.\calcbsp.cpp
# End Source File
# Begin Source File

SOURCE=.\calcbsp.h
# End Source File
# Begin Source File

SOURCE=.\filter_1.h
# End Source File
# Begin Source File

SOURCE=.\gpcore.h
# End Source File
# Begin Source File

SOURCE=.\gpmath.h
# End Source File
# Begin Source File

SOURCE=.\interval.h
# End Source File
# Begin Source File

SOURCE=.\line_segment_3.h
# End Source File
# Begin Source File

SOURCE=.\matrix_3x3.cpp
# End Source File
# Begin Source File

SOURCE=.\matrix_3x3.h
# End Source File
# Begin Source File

SOURCE=.\oriented_bounding_box_3.h
# End Source File
# Begin Source File

SOURCE=.\plane_3.h
# End Source File
# Begin Source File

SOURCE=.\space_3.cpp
# End Source File
# Begin Source File

SOURCE=.\space_3.h
# End Source File
# Begin Source File

SOURCE=.\sphere_3.h
# End Source File
# Begin Source File

SOURCE=.\triangle_3.cpp
# End Source File
# Begin Source File

SOURCE=.\triangle_3.h
# End Source File
# Begin Source File

SOURCE=.\vector_3.h
# End Source File
# End Target
# End Project
