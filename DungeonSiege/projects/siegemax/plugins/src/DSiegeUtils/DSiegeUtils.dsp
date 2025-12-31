# Microsoft Developer Studio Project File - Name="DSiegeUtils" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=DSiegeUtils - Win32 Debug DLL 3dsmax50
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DSiegeUtils.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DSiegeUtils.mak" CFG="DSiegeUtils - Win32 Debug DLL 3dsmax50"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DSiegeUtils - Win32 Release DLL gmax" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DSiegeUtils - Win32 Debug DLL gmax" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DSiegeUtils - Win32 Release DLL 3dsmax" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DSiegeUtils - Win32 Debug DLL 3dsmax" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DSiegeUtils - Win32 Release DLL 3dsmax50" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DSiegeUtils - Win32 Debug DLL 3dsmax50" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "DSiegeUtils"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL gmax"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "DSiegeUtils___Win32_Release_DLL_gmax"
# PROP BASE Intermediate_Dir "DSiegeUtils___Win32_Release_DLL_gmax"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DSiegeUtils___Win32_Release_DLL_gmax"
# PROP Intermediate_Dir "DSiegeUtils___Win32_Release_DLL_gmax"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "D:\gmaxDEV\maxsdk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /YX /FD /LD /c
# ADD CPP /nologo /MT /W3 /O2 /I "p:\gmaxDEV\maxsdk\include" /I "p:\gmaxDEV\maxsdk\include\maxscrpt" /I "D:\gmaxDEV\maxsdk\include" /I "D:\gmaxDEV\maxsdk\include\maxscrpt" /I "C:\3dsmax42\Cstudio\Sdk" /D "NDEBUG" /D "SIEGEMAX" /D "_WINDOWS" /D "WIN32" /D "_USRDLL" /YX /FD /LD /c
# SUBTRACT CPP /WX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 msvcrt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib Ws2_32.lib Mpr.lib /nologo /base:"0x7cb00000" /subsystem:windows /dll /machine:I386 /nodefaultlib:"msvcrtd.lib" /nodefaultlib:"libcmtd.lib" /nodefaultlib:"libcmt.lib" /out:"D:\gmaxDEV\plugins\DSiegeUtils.dle" /libpath:"D:\gmaxDEV\maxsdk\lib" /release
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib Ws2_32.lib Mpr.lib bmm.lib /nologo /base:"0x7cb00000" /subsystem:windows /dll /machine:I386 /out:"P:\gmaxDEV\gamepacks\SiegeMAX\plugins\DSiegeUtils.dlu" /libpath:"P:\gmaxDEV\maxsdk\lib" /libpath:"P:\ds1\main\gpg\lib" /release
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL gmax"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DSiegeUtils___Win32_Debug_DLL_gmax"
# PROP BASE Intermediate_Dir "DSiegeUtils___Win32_Debug_DLL_gmax"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DSiegeUtils___Win32_Debug_DLL_gmax"
# PROP Intermediate_Dir "DSiegeUtils___Win32_Debug_DLL_gmax"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "D:\gmaxDEV\maxsdk\include" /D "_WINDOWS" /D "_USRDLL" /D "WIN32" /D "_DEBUG" /D "SIEGEMAX" /D "NO_GPMEM_DBG_NEW" /YX /FD /LD /c
# ADD CPP /nologo /MT /W3 /Gm /Zi /Od /I "p:\gmaxDEV\maxsdk\include" /I "p:\gmaxDEV\maxsdk\include\maxscrpt" /I "D:\gmaxDEV\maxsdk\include" /I "D:\gmaxDEV\maxsdk\include\maxscrpt" /I "C:\3dsmax42\Cstudio\Sdk" /D "NDEBUG" /D "SIEGEMAX" /D "_WINDOWS" /D "WIN32" /D "_USRDLL" /FD /LD /c
# SUBTRACT CPP /YX /Yc
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 msvcrt.lib libcmt.lib Ws2_32.lib Mpr.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib /nologo /base:"0x7cb00000" /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /nodefaultlib:"libcmtd.lib" /nodefaultlib:"libcmt.lib" /out:"D:\ds1\main\gpg\projects\siegemax\plugins\gmax\DSiegeUtils.dle" /pdbtype:sept /libpath:"D:\gmaxDEV\maxsdk\lib" /libpath:"D:\ds1\main\gpg\lib"
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib Ws2_32.lib Mpr.lib bmm.lib /nologo /base:"0x7cb00000" /subsystem:windows /dll /incremental:no /debug /machine:I386 /nodefaultlib:"_xmsvcrtd.lib" /nodefaultlib:"_xlibcmt.lib" /nodefaultlib:"_xmsvcrt.lib" /nodefaultlib:"_xlibcmtd.lib" /nodefaultlib:"xlibcmt.lib" /out:"P:\gmaxDEV\gamepacks\SiegeMAX\plugins\DSiegeUtils.dlu" /pdbtype:sept /libpath:"D:\gmaxDEV\maxsdk\lib" /libpath:"D:\ds1\main\gpg\lib"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DSiegeUtils___Win32_Release_DLL_3dsmax"
# PROP BASE Intermediate_Dir "DSiegeUtils___Win32_Release_DLL_3dsmax"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DSiegeUtils___Win32_Release_DLL_3dsmax"
# PROP Intermediate_Dir "DSiegeUtils___Win32_Release_DLL_3dsmax"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "C:\3dsmax42\maxsdk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /YX /FD /LD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /O1 /I "C:\3dsmax42\maxsdk\include" /I "D:\gmaxDEV\maxsdk\include" /I "D:\gmaxDEV\maxsdk\include\maxscript" /I "D:\gmaxDEV\maxsdk\include\maxscrpt" /I "C:\3dsmax42\cstudio\sdk" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "WIN32" /D "SIEGEMAX" /YX /FD /LD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 Ws2_32.lib Mpr.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib /nologo /base:"0x7cb00000" /subsystem:windows /dll /debug /machine:I386 /out:"D:\ds1\main\gpg\projects\siegemax\plugins\3dsmax\DSiegeUtils.dle" /pdbtype:sept /libpath:"C:\3dsmax42\maxsdk\lib"
# ADD LINK32 msvcrt.lib libcmt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib Ws2_32.lib Mpr.lib bmm.lib /nologo /base:"0x7cb00000" /subsystem:windows /dll /incremental:no /machine:I386 /nodefaultlib:"msvcrtd.lib" /nodefaultlib:"libcmtd.lib" /out:"c:\3dsmax42\plugins\dsiegeutils.dlu" /pdbtype:sept /libpath:"C:\3dsmax42\maxsdk\lib" /libpath:"P:\ds1\main\gpg\lib"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DSiegeUtils___Win32_Debug_DLL_3dsmax"
# PROP BASE Intermediate_Dir "DSiegeUtils___Win32_Debug_DLL_3dsmax"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DSiegeUtils___Win32_Debug_DLL_3dsmax"
# PROP Intermediate_Dir "DSiegeUtils___Win32_Debug_DLL_3dsmax"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "C:\3dsmax42\maxsdk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /YX /FD /LD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "C:\3dsmax42\maxsdk\include" /I "C:\3dsmax42\maxsdk\include\maxscrpt" /I "C:\3dsmax42\cstudio\sdk" /D "NDEBUG" /D "SIEGEMAX" /D "_WINDOWS" /D "WIN32" /D "_USRDLL" /YX /FD /LD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 Ws2_32.lib Mpr.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib /nologo /base:"0x7cb00000" /subsystem:windows /dll /debug /machine:I386 /out:"D:\ds1\main\gpg\projects\siegemax\plugins\3dsmax\DSiegeUtils.dle" /pdbtype:sept /libpath:"C:\3dsmax42\maxsdk\lib"
# ADD LINK32 msvcrt.lib libcmt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib Ws2_32.lib Mpr.lib bmm.lib /nologo /base:"0x7cb00000" /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /nodefaultlib:"libcmtd.lib" /out:"c:\3dsmax42\plugins\dsiegeutils.dlu" /pdbtype:sept /libpath:"C:\3dsmax42\maxsdk\lib" /libpath:"P:\ds1\main\gpg\lib"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax50"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DSiegeUtils___Win32_Release_DLL_3dsmax50"
# PROP BASE Intermediate_Dir "DSiegeUtils___Win32_Release_DLL_3dsmax50"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DSiegeUtils___Win32_Release_DLL_3dsmax50"
# PROP Intermediate_Dir "DSiegeUtils___Win32_Release_DLL_3dsmax50"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /Zi /O1 /I "C:\3dsmax42\maxsdk\include" /I "D:\gmaxDEV\maxsdk\include" /I "D:\gmaxDEV\maxsdk\include\maxscript" /I "D:\gmaxDEV\maxsdk\include\maxscrpt" /I "C:\3dsmax42\cstudio\sdk" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "WIN32" /D "SIEGEMAX" /YX /FD /LD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /O1 /I "E:\3dsmax5\maxsdk\include" /I "E:\3dsmax5\maxsdk\include\maxscrpt" /I "C:\3dsmax42\cstudio\sdk" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "WIN32" /D "SIEGEMAX" /YX /FD /LD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 msvcrt.lib libcmt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib Ws2_32.lib Mpr.lib bmm.lib /nologo /base:"0x7cb00000" /subsystem:windows /dll /incremental:no /machine:I386 /nodefaultlib:"msvcrtd.lib" /nodefaultlib:"libcmtd.lib" /out:"c:\3dsmax42\plugins\dsiegeutils.dlu" /pdbtype:sept /libpath:"C:\3dsmax42\maxsdk\lib" /libpath:"P:\ds1\main\gpg\lib"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 msvcrt.lib libcmt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib Ws2_32.lib Mpr.lib bmm.lib /nologo /base:"0x7cb00000" /subsystem:windows /dll /incremental:no /machine:I386 /nodefaultlib:"msvcrtd.lib" /nodefaultlib:"libcmtd.lib" /out:"e:\3dsmax5\plugins\dsiegeutils.dlu" /pdbtype:sept /libpath:"e:\3dsmax5\maxsdk\lib" /libpath:"P:\ds1\main\gpg\lib"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax50"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DSiegeUtils___Win32_Debug_DLL_3dsmax50"
# PROP BASE Intermediate_Dir "DSiegeUtils___Win32_Debug_DLL_3dsmax50"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DSiegeUtils___Win32_Debug_DLL_3dsmax50"
# PROP Intermediate_Dir "DSiegeUtils___Win32_Debug_DLL_3dsmax50"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "C:\3dsmax42\maxsdk\include" /I "C:\3dsmax42\maxsdk\include\maxscrpt" /I "C:\3dsmax42\cstudio\sdk" /D "NDEBUG" /D "SIEGEMAX" /D "_WINDOWS" /D "WIN32" /D "_USRDLL" /YX /FD /LD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "E:\3dsmax5\maxsdk\include" /I "E:\3dsmax5\maxsdk\include\maxscrpt" /I "C:\3dsmax42\cstudio\sdk" /D "NDEBUG" /D "SIEGEMAX" /D "_WINDOWS" /D "WIN32" /D "_USRDLL" /YX /FD /LD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 msvcrt.lib libcmt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib Ws2_32.lib Mpr.lib bmm.lib /nologo /base:"0x7cb00000" /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /nodefaultlib:"libcmtd.lib" /out:"c:\3dsmax42\plugins\dsiegeutils.dlu" /pdbtype:sept /libpath:"C:\3dsmax42\maxsdk\lib" /libpath:"P:\ds1\main\gpg\lib"
# ADD LINK32 msvcrt.lib libcmt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib Ws2_32.lib Mpr.lib bmm.lib /nologo /base:"0x7cb00000" /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /nodefaultlib:"libcmtd.lib" /out:"E:\3dsmax5\plugins\dsiegeutils.dlu" /pdbtype:sept /libpath:"E:\3dsmax5\maxsdk\lib" /libpath:"P:\ds1\main\gpg\lib"

!ENDIF 

# Begin Target

# Name "DSiegeUtils - Win32 Release DLL gmax"
# Name "DSiegeUtils - Win32 Debug DLL gmax"
# Name "DSiegeUtils - Win32 Release DLL 3dsmax"
# Name "DSiegeUtils - Win32 Debug DLL 3dsmax"
# Name "DSiegeUtils - Win32 Release DLL 3dsmax50"
# Name "DSiegeUtils - Win32 Debug DLL 3dsmax50"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ASPexp.cpp
# End Source File
# Begin Source File

SOURCE=.\calcbsp.cpp

!IF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL gmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL gmax"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax50"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax50"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DllEntry.cpp

!IF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL gmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL gmax"

# ADD BASE CPP /Zi
# ADD CPP /Zi /YX

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax"

# ADD BASE CPP /Zi
# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax"

# ADD BASE CPP /Zi
# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax50"

# ADD BASE CPP /Zi
# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax50"

# ADD BASE CPP /Zi
# ADD CPP /Zi

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DSiegeUtils.cpp

!IF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL gmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL gmax"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax50"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax50"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DSiegeUtils.def
# End Source File
# Begin Source File

SOURCE=.\DSiegeUtils.rc
# End Source File
# Begin Source File

SOURCE=.\DSmxs.cpp

!IF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL gmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL gmax"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax50"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax50"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ExpBase.cpp
# End Source File
# Begin Source File

SOURCE=.\maxhelpers.cpp
# End Source File
# Begin Source File

SOURCE=.\postprocess.cpp

!IF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL gmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL gmax"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax50"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax50"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PRSexp.cpp
# End Source File
# Begin Source File

SOURCE=.\SNOExp.cpp

!IF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL gmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL gmax"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax50"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax50"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vertnormals.cpp

!IF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL gmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL gmax"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Release DLL 3dsmax50"

!ELSEIF  "$(CFG)" == "DSiegeUtils - Win32 Debug DLL 3dsmax50"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ASPexp.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax42\Cstudio\Sdk\BIPEXP.H
# End Source File
# Begin Source File

SOURCE=.\calcbsp.h
# End Source File
# Begin Source File

SOURCE=.\DSiegeUtils.h
# End Source File
# Begin Source File

SOURCE=.\DSmxs.h
# End Source File
# Begin Source File

SOURCE=.\ExpBase.h
# End Source File
# Begin Source File

SOURCE=.\maxhelpers.h
# End Source File
# Begin Source File

SOURCE="..\..\..\..\..\lib projects\nema\nema_iostructs.h"
# End Source File
# Begin Source File

SOURCE=C:\3dsmax42\Cstudio\Sdk\PHYEXP.H
# End Source File
# Begin Source File

SOURCE=.\postprocess.h
# End Source File
# Begin Source File

SOURCE=.\PRSexp.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\SNOExp.h
# End Source File
# Begin Source File

SOURCE=.\vertnormals.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
