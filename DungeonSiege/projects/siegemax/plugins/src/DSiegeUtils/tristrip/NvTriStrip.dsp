# Microsoft Developer Studio Project File - Name="NvTriStrip" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=NvTriStrip - Win32 Release DLL gmax
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NvTriStrip.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NvTriStrip.mak" CFG="NvTriStrip - Win32 Release DLL gmax"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NvTriStrip - Win32 Release DLL gmax" (based on "Win32 (x86) Static Library")
!MESSAGE "NvTriStrip - Win32 Debug DLL gmax" (based on "Win32 (x86) Static Library")
!MESSAGE "NvTriStrip - Win32 Release DLL 3dsmax" (based on "Win32 (x86) Static Library")
!MESSAGE "NvTriStrip - Win32 Debug DLL 3dsmax" (based on "Win32 (x86) Static Library")
!MESSAGE "NvTriStrip - Win32 Release DLL 3dsmax50" (based on "Win32 (x86) Static Library")
!MESSAGE "NvTriStrip - Win32 Debug DLL 3dsmax50" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "NVIDIA_XBOX03"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NvTriStrip - Win32 Release DLL gmax"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NvTriStrip___Win32_Release_DLL_gmax"
# PROP BASE Intermediate_Dir "NvTriStrip___Win32_Release_DLL_gmax"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "NvTriStrip___Win32_Release_DLL_gmax"
# PROP Intermediate_Dir "NvTriStrip___Win32_Release_DLL_gmax"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "SIEGEMAX" /D "_WINDOWS" /D "WIN32" /D "_USRDLL" /YX /FD /c
# SUBTRACT CPP /WX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Debug DLL gmax"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NvTriStrip___Win32_Debug_DLL_gmax"
# PROP BASE Intermediate_Dir "NvTriStrip___Win32_Debug_DLL_gmax"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "NvTriStrip___Win32_Debug_DLL_gmax"
# PROP Intermediate_Dir "NvTriStrip___Win32_Debug_DLL_gmax"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /Gm /Zi /Od /D "NDEBUG" /D "SIEGEMAX" /D "_WINDOWS" /D "WIN32" /D "_USRDLL" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Release DLL 3dsmax"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NvTriStrip___Win32_Release_DLL_3dsmax"
# PROP BASE Intermediate_Dir "NvTriStrip___Win32_Release_DLL_3dsmax"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "NvTriStrip___Win32_Release_DLL_3dsmax"
# PROP Intermediate_Dir "NvTriStrip___Win32_Release_DLL_3dsmax"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "WIN32" /D "SIEGEMAX" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Debug DLL 3dsmax"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NvTriStrip___Win32_Debug_DLL_3dsmax"
# PROP BASE Intermediate_Dir "NvTriStrip___Win32_Debug_DLL_3dsmax"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "NvTriStrip___Win32_Debug_DLL_3dsmax"
# PROP Intermediate_Dir "NvTriStrip___Win32_Debug_DLL_3dsmax"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "SIEGEMAX" /D "_WINDOWS" /D "WIN32" /D "_USRDLL" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Release DLL 3dsmax50"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NvTriStrip___Win32_Release_DLL_3dsmax50"
# PROP BASE Intermediate_Dir "NvTriStrip___Win32_Release_DLL_3dsmax50"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "NvTriStrip___Win32_Release_DLL_3dsmax50"
# PROP Intermediate_Dir "NvTriStrip___Win32_Release_DLL_3dsmax50"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "WIN32" /D "SIEGEMAX" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "WIN32" /D "SIEGEMAX" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Debug DLL 3dsmax50"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NvTriStrip___Win32_Debug_DLL_3dsmax50"
# PROP BASE Intermediate_Dir "NvTriStrip___Win32_Debug_DLL_3dsmax50"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "NvTriStrip___Win32_Debug_DLL_3dsmax50"
# PROP Intermediate_Dir "NvTriStrip___Win32_Debug_DLL_3dsmax50"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "SIEGEMAX" /D "_WINDOWS" /D "WIN32" /D "_USRDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "SIEGEMAX" /D "_WINDOWS" /D "WIN32" /D "_USRDLL" /YX /FD /c
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

# Name "NvTriStrip - Win32 Release DLL gmax"
# Name "NvTriStrip - Win32 Debug DLL gmax"
# Name "NvTriStrip - Win32 Release DLL 3dsmax"
# Name "NvTriStrip - Win32 Debug DLL 3dsmax"
# Name "NvTriStrip - Win32 Release DLL 3dsmax50"
# Name "NvTriStrip - Win32 Debug DLL 3dsmax50"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\NvTriStrip.cpp

!IF  "$(CFG)" == "NvTriStrip - Win32 Release DLL gmax"

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Debug DLL gmax"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Release DLL 3dsmax"

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Debug DLL 3dsmax"

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Release DLL 3dsmax50"

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Debug DLL 3dsmax50"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\NvTriStripObjects.cpp

!IF  "$(CFG)" == "NvTriStrip - Win32 Release DLL gmax"

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Debug DLL gmax"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Release DLL 3dsmax"

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Debug DLL 3dsmax"

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Release DLL 3dsmax50"

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Debug DLL 3dsmax50"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\VertexCache.cpp

!IF  "$(CFG)" == "NvTriStrip - Win32 Release DLL gmax"

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Debug DLL gmax"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Release DLL 3dsmax"

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Debug DLL 3dsmax"

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Release DLL 3dsmax50"

!ELSEIF  "$(CFG)" == "NvTriStrip - Win32 Debug DLL 3dsmax50"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\NvTriStrip.h
# End Source File
# Begin Source File

SOURCE=.\NvTriStripObjects.h
# End Source File
# Begin Source File

SOURCE=.\VertexCache.h
# End Source File
# End Group
# End Target
# End Project
