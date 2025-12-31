# Microsoft Developer Studio Project File - Name="Siege" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Siege - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Siege.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Siege.mak" CFG="Siege - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Siege - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Siege - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "Siege - Win32 Retail" (based on "Win32 (x86) Static Library")
!MESSAGE "Siege - Win32 Profiling" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/LIB Projects/Siege", NEZAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Siege - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\siege\Release"
# PROP Intermediate_Dir "\temp\vc6\lib\siege\Release"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /Yu"precomp_siege.h" /FD /Zm500 /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Siege - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\lib\siege\Debug"
# PROP Intermediate_Dir "\temp\vc6\lib\siege\Debug"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /Yu"precomp_siege.h" /FD /GZ /Zm500 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Siege - Win32 Retail"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Retail"
# PROP BASE Intermediate_Dir "Retail"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\Siege\Retail"
# PROP Intermediate_Dir "\temp\vc6\lib\Siege\Retail"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MT /W3 /O2 /D "WIN32" /D "NDEBUG" /D GP_RETAIL=1 /Yu"precomp_siege.h" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Oy /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /D GP_RETAIL=1 /Yu"precomp_siege.h" /FD /Zm500 /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Siege - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Profiling"
# PROP BASE Intermediate_Dir "Profiling"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\Siege\Profiling"
# PROP Intermediate_Dir "\temp\vc6\lib\Siege\Profiling"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "WIN32" /D "NDEBUG" /Yu"precomp_siege.h" /FD /Zm500 /Gs /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /D GP_PROFILING=1 /Yu"precomp_siege.h" /FD /Zm500 /Gs /Gh /c
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

# Name "Siege - Win32 Release"
# Name "Siege - Win32 Debug"
# Name "Siege - Win32 Retail"
# Name "Siege - Win32 Profiling"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\siege_bsp.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_cache.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_cache_handle.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_camera.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_compass.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_console.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_database.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_database_guid.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_decal.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_decal_database.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_engine.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_frustum.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_hotpoint_database.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_label.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_light_database.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_lines.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_loader.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_logical_mesh.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_logical_node.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_mesh.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_mesh_io.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_mouse_shadow.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_node.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_node_io.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_pathfinder.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_persist.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_transition.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_viewfrustum.cpp
# End Source File
# Begin Source File

SOURCE=.\siege_walker.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\siege_bsp.h
# End Source File
# Begin Source File

SOURCE=.\siege_cache.h
# End Source File
# Begin Source File

SOURCE=.\siege_cache_handle.h
# End Source File
# Begin Source File

SOURCE=.\siege_camera.h
# End Source File
# Begin Source File

SOURCE=.\siege_compass.h
# End Source File
# Begin Source File

SOURCE=.\siege_console.h
# End Source File
# Begin Source File

SOURCE=.\siege_database.h
# End Source File
# Begin Source File

SOURCE=.\siege_database_guid.h
# End Source File
# Begin Source File

SOURCE=.\siege_decal.h
# End Source File
# Begin Source File

SOURCE=.\siege_decal_database.h
# End Source File
# Begin Source File

SOURCE=.\siege_defs.h
# End Source File
# Begin Source File

SOURCE=.\siege_engine.h
# End Source File
# Begin Source File

SOURCE=.\siege_frustum.h
# End Source File
# Begin Source File

SOURCE=.\siege_hotpoint_database.h
# End Source File
# Begin Source File

SOURCE=.\siege_label.h
# End Source File
# Begin Source File

SOURCE=.\siege_light_database.h
# End Source File
# Begin Source File

SOURCE=.\siege_light_structs.h
# End Source File
# Begin Source File

SOURCE=.\siege_lines.h
# End Source File
# Begin Source File

SOURCE=.\siege_loader.h
# End Source File
# Begin Source File

SOURCE=.\siege_logical_mesh.h
# End Source File
# Begin Source File

SOURCE=.\siege_logical_node.h
# End Source File
# Begin Source File

SOURCE=.\siege_mesh.h
# End Source File
# Begin Source File

SOURCE=.\siege_mesh_door.h
# End Source File
# Begin Source File

SOURCE=.\siege_mesh_io.h
# End Source File
# Begin Source File

SOURCE=.\siege_mouse_shadow.h
# End Source File
# Begin Source File

SOURCE=.\siege_node.h
# End Source File
# Begin Source File

SOURCE=.\siege_node_io.h
# End Source File
# Begin Source File

SOURCE=.\siege_options.h
# End Source File
# Begin Source File

SOURCE=.\siege_pathfinder.h
# End Source File
# Begin Source File

SOURCE=.\siege_persist.h
# End Source File
# Begin Source File

SOURCE=.\siege_pos.h
# End Source File
# Begin Source File

SOURCE=.\siege_transition.h
# End Source File
# Begin Source File

SOURCE=.\siege_viewfrustum.h
# End Source File
# Begin Source File

SOURCE=.\siege_walker.h
# End Source File
# End Group
# Begin Group "Precompiled Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\precomp_siege.cpp
# ADD CPP /Yc"precomp_siege.h"
# End Source File
# Begin Source File

SOURCE=.\precomp_siege.h
# End Source File
# End Group
# End Target
# End Project
