# Microsoft Developer Studio Project File - Name="_Exe" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=_Exe - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "_Exe.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "_Exe.mak" CFG="_Exe - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "_Exe - Win32 Retail" (based on "Win32 (x86) Application")
!MESSAGE "_Exe - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "_Exe - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "_Exe - Win32 Profiling" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/Projects/Tattoo/Exe", XOVAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "_Exe - Win32 Retail"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Retail"
# PROP BASE Intermediate_Dir "Retail"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\tattoo\_Exe\Retail"
# PROP Intermediate_Dir "\temp\vc6\tattoo\_Exe\Retail"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Oy /Ob1 /Gf /Gy /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /Yu"precomp_exe.h" /FD /Gs /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "RETAIL_ICON"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 dxmgr.lib dinput.lib dxguid.lib dplayx.lib ddraw.lib d3dim.lib winmm.lib dsound.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib quartz.lib strmbasd.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Release/Tattoo.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 shlw32mt.lib libcmt.lib libcpmt.lib winmm.lib dsound.lib binkw32.lib dxguid.lib dplayx.lib ddraw.lib d3dim.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib quartz.lib strmbasd.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /include:"_SmartHeap_malloc" /out:"Retail/DungeonSiege.exe" /pdbtype:sept /opt:ref
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "_Exe - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\tattoo\_Exe\Release"
# PROP Intermediate_Dir "\temp\vc6\tattoo\_Exe\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"precomp_exe.h" /FD /Gs /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "RELEASE_ICON"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 shlw32mt.lib libcmt.lib libcpmt.lib binkw32.lib dxguid.lib dplayx.lib ddraw.lib d3dim.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib quartz.lib strmbasd.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /include:"_SmartHeap_malloc" /out:"Release/DungeonSiegeR.exe" /pdbtype:sept /opt:ref
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "_Exe - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\tattoo\_Exe\Debug"
# PROP Intermediate_Dir "\temp\vc6\tattoo\_Exe\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /Zi /Od /D "_DEBUG" /D "WIN32" /Yu"precomp_exe.h" /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "DEBUG_ICON"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 binkw32.lib dxguid.lib dplayx.lib ddraw.lib d3dim.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib quartz.lib strmbasd.lib wsock32.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"Debug/DungeonSiegeD.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "_Exe - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Profiling"
# PROP BASE Intermediate_Dir "Profiling"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\tattoo\_Exe\Profiling"
# PROP Intermediate_Dir "\temp\vc6\tattoo\_Exe\Profiling"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"precomp_exe.h" /FD /Gs /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /D GP_PROFILING=1 /Yu"precomp_exe.h" /FD /Gs /Gh /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 binkw32.lib shlw32mt.lib libcmt.lib libcpmt.lib dxguid.lib dplayx.lib ddraw.lib d3dim.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib quartz.lib strmbasd.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /include:"_SmartHeap_malloc" /out:"Release/DungeonSiegeR.exe" /opt:ref
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 shlw32mt.lib libcmt.lib libcpmt.lib binkw32.lib dxguid.lib dplayx.lib ddraw.lib d3dim.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib quartz.lib strmbasd.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /include:"_SmartHeap_malloc" /out:"Profiling/DungeonSiegeP.exe" /pdbtype:sept /opt:ref
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "_Exe - Win32 Retail"
# Name "_Exe - Win32 Release"
# Name "_Exe - Win32 Debug"
# Name "_Exe - Win32 Profiling"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Local\CustomExports.cpp

!IF  "$(CFG)" == "_Exe - Win32 Retail"

!ELSEIF  "$(CFG)" == "_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "_Exe - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "_Exe - Win32 Profiling"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# End Group
# Begin Group "Precompiled Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\precomp_exe.cpp
# ADD CPP /Yc"precomp_exe.h"
# End Source File
# Begin Source File

SOURCE=.\precomp_exe.h
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Res\App.ico
# End Source File
# Begin Source File

SOURCE=.\Res\AppD.ico
# End Source File
# Begin Source File

SOURCE=.\Res\Resource.h
# End Source File
# Begin Source File

SOURCE=.\Res\Tattoo.rc

!IF  "$(CFG)" == "_Exe - Win32 Retail"

# ADD BASE RSC /l 0x409 /i "Res"
# ADD RSC /l 0x409 /i "Res" /d GP_RETAIL=1

!ELSEIF  "$(CFG)" == "_Exe - Win32 Release"

# ADD BASE RSC /l 0x409 /i "Res"
# ADD RSC /l 0x409 /i "Res" /d GP_RELEASE=1

!ELSEIF  "$(CFG)" == "_Exe - Win32 Debug"

# ADD BASE RSC /l 0x409 /i "Res"
# ADD RSC /l 0x409 /i "Res" /d GP_DEBUG=1

!ELSEIF  "$(CFG)" == "_Exe - Win32 Profiling"

# ADD BASE RSC /l 0x409 /i "Res" /d GP_RELEASE=1
# ADD RSC /l 0x409 /i "Res" /d GP_RELEASE=1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Res\TattooVersion.rc2
# End Source File
# End Group
# End Target
# End Project
