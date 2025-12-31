# Microsoft Developer Studio Project File - Name="maxextension" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=maxextension - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "maxextension.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "maxextension.mak" CFG="maxextension - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "maxextension - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "maxextension - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "maxextension - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NOGPCORE" /YX /FD /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Release/doa.bsc"
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 maxscrpt.lib COMCTL32.LIB KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB bmm.lib core.lib geom.lib gfx.lib mesh.lib util.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0X2D290000" /subsystem:windows /dll /incremental:yes /machine:I386 /def:"doa.def" /out:"e:\3dsmax2.5 (release)\plugins\doa.dlx" /libpath:"E:\3DSMAX2.5 (Release)\MSSDK\LIB" /libpath:"E:\3DSMAX2.5 (Release)\MAXSDK\LIB"
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "maxextension - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /YX /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "E:\3DSMAX3\Maxsdk\Include\Maxscrpt" /I "E:\3DSMAX3\MAXSDK\INCLUDE" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NOGPCORE" /FR /YX /FD /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib mpr.lib maxscrpt.lib core.lib mnmath.lib COMCTL32.LIB KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB bmm.lib geom.lib gfx.lib mesh.lib maxutil.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0X2D290000" /subsystem:windows /dll /debug /machine:I386 /out:"E:\3DSMAX3\plugins\doa.dlx" /pdbtype:sept /libpath:"E:\3DSMAX3\MAXSDK\LIB" /libpath:""E:\3DSMAX2.5
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "maxextension - Win32 Release"
# Name "maxextension - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DllMain.cpp
# End Source File
# Begin Source File

SOURCE=.\doa.cpp

!IF  "$(CFG)" == "maxextension - Win32 Release"

!ELSEIF  "$(CFG)" == "maxextension - Win32 Debug"

# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\doa.def
# End Source File
# End Group
# End Target
# End Project
