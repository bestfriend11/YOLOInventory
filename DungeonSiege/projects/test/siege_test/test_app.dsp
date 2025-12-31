# Microsoft Developer Studio Project File - Name="test_app" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=test_app - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "test_app.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "test_app.mak" CFG="test_app - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "test_app - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "test_app - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/Projects/Siege_test", EOGAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "test_app - Win32 Release"

# PROP BASE Use_MFC 0
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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "STRICT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 dxguid.lib dinput.lib glu32.lib opengl32.lib winmm.lib dsound.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386

!ELSEIF  "$(CFG)" == "test_app - Win32 Debug"

# PROP BASE Use_MFC 0
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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "STRICT" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dxguid.lib dinput.lib glu32.lib opengl32.lib winmm.lib dsound.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "test_app - Win32 Release"
# Name "test_app - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "siege_soul"

# PROP Default_Filter ""
# Begin Group "tnk"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\tnk\soul_tnk_file_system.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\tnk\soul_tnk_file_system.h"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\tnk\soul_tnk_read_stream.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\tnk\soul_tnk_read_stream.h"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\tnk\soul_tnk_write_stream.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\tnk\soul_tnk_write_stream.h"
# End Source File
# End Group
# Begin Group "win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\win32\soul_win32_directory_iterator.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\win32\soul_win32_directory_iterator.h"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\win32\soul_win32_file_iterator.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\win32\soul_win32_file_iterator.h"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\win32\soul_win32_file_system.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\win32\soul_win32_file_system.h"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\win32\soul_win32_read_stream.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\win32\soul_win32_read_stream.h"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\win32\soul_win32_statistics_display.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\win32\soul_win32_statistics_display.h"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\win32\soul_win32_write_stream.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\win32\soul_win32_write_stream.h"
# End Source File
# End Group
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\ai\soul_configuration_handle.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\ai\soul_configuration_handle.h"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\ai\soul_handle.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\ai\soul_handle.h"
# End Source File
# End Group
# Begin Source File

SOURCE=.\flamethrower_siege_context.cpp
# End Source File
# Begin Source File

SOURCE="..\..\LIB Projects\Soul\exchange\ai\FOR_TEMPORARY_SIEGE_INTEGRATION_ONLY_Render.cpp"
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\flamethrower_siege_context.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
