# Microsoft Developer Studio Project File - Name="Rapi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Rapi - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Rapi.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Rapi.mak" CFG="Rapi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Rapi - Win32 Retail" (based on "Win32 (x86) Static Library")
!MESSAGE "Rapi - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Rapi - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "Rapi - Win32 Profiling" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/LIB Projects/Rapi", NKLAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Rapi - Win32 Retail"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Retail"
# PROP BASE Intermediate_Dir "Retail"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\Rapi\Retail"
# PROP Intermediate_Dir "\temp\vc6\lib\Rapi\Retail"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MT /W3 /O2 /D "WIN32" /D "NDEBUG" /D GP_RETAIL=1 /Yu"precomp_rapi.h" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Oy /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /D GP_RETAIL=1 /Yu"precomp_rapi.h" /FD /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\LIB\Rapi.LIB"
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Rapi - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\rapi\Release"
# PROP Intermediate_Dir "\temp\vc6\lib\rapi\Release"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /Yu"precomp_rapi.h" /FD /Zm500 /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Rapi - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\lib\rapi\Debug"
# PROP Intermediate_Dir "\temp\vc6\lib\rapi\Debug"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /Yu"precomp_rapi.h" /FD /GZ /Zm500 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Rapi - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Profiling"
# PROP BASE Intermediate_Dir "Profiling"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\Rapi\Profiling"
# PROP Intermediate_Dir "\temp\vc6\lib\Rapi\Profiling"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /Yu"precomp_rapi.h" /FD /Zm500 /Gs /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /D GP_PROFILING=1 /Yu"precomp_rapi.h" /FD /Zm500 /Gs /Gh /c
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

# Name "Rapi - Win32 Retail"
# Name "Rapi - Win32 Release"
# Name "Rapi - Win32 Debug"
# Name "Rapi - Win32 Profiling"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Rapi.cpp
# End Source File
# Begin Source File

SOURCE=.\RapiAppModule.cpp
# End Source File
# Begin Source File

SOURCE=.\RapiFont.cpp
# End Source File
# Begin Source File

SOURCE=.\RapiImage.cpp
# End Source File
# Begin Source File

SOURCE=.\RapiLight.cpp
# End Source File
# Begin Source File

SOURCE=.\RapiMouse.cpp
# End Source File
# Begin Source File

SOURCE=.\RapiOwner.cpp
# End Source File
# Begin Source File

SOURCE=.\RapiPrimitive.cpp
# End Source File
# Begin Source File

SOURCE=.\RapiStaticObject.cpp
# End Source File
# Begin Source File

SOURCE=.\RapiSurface.cpp
# End Source File
# Begin Source File

SOURCE=.\RapiTexture.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Rapi.h
# End Source File
# Begin Source File

SOURCE=.\RapiAppModule.h
# End Source File
# Begin Source File

SOURCE=.\RapiFont.h
# End Source File
# Begin Source File

SOURCE=.\RapiImage.h
# End Source File
# Begin Source File

SOURCE=.\RapiLight.h
# End Source File
# Begin Source File

SOURCE=.\RapiMath.h
# End Source File
# Begin Source File

SOURCE=.\RapiMouse.h
# End Source File
# Begin Source File

SOURCE=.\RapiOwner.h
# End Source File
# Begin Source File

SOURCE=.\RapiPrimitive.h
# End Source File
# Begin Source File

SOURCE=.\RapiStaticObject.h
# End Source File
# Begin Source File

SOURCE=.\RapiSurface.h
# End Source File
# Begin Source File

SOURCE=.\RapiTexture.h
# End Source File
# End Group
# Begin Group "Precompiled Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\precomp_rapi.cpp
# ADD CPP /Yc"precomp_rapi.h"
# End Source File
# Begin Source File

SOURCE=.\precomp_rapi.h
# End Source File
# End Group
# End Target
# End Project
