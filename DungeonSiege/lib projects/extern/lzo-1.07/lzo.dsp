# Microsoft Developer Studio Project File - Name="lzo" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=lzo - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "lzo.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "lzo.mak" CFG="lzo - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "lzo - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "lzo - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/Lib Projects/Extern/lzo-1.07", OBHGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "lzo - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\lzo\Release"
# PROP Intermediate_Dir "\temp\vc6\lib\lzo\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /MT /w /W0 /O2 /Ob2 /I "include" /D "NDEBUG" /D "WIN32" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\lib\lzo.lib"

!ELSEIF  "$(CFG)" == "lzo - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\lib\lzo\Debug"
# PROP Intermediate_Dir "\temp\vc6\lib\lzo\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /w /W0 /Z7 /Od /I "include" /D "_DEBUG" /D "WIN32" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\lib\lzod.lib"

!ENDIF 

# Begin Target

# Name "lzo - Win32 Release"
# Name "lzo - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\alloc.c
# End Source File
# Begin Source File

SOURCE=.\src\io.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1_99.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1a.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1a_99.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_1.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_2.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_3.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_4.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_5.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_6.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_7.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_8.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_9.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_99.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_9x.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_cc.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_d1.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_d2.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_rr.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_xx.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_1.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_2.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_3.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_4.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_5.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_6.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_7.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_8.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_9.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_99.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_9x.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_cc.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_d1.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_d2.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_rr.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_xx.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1f_1.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1f_9x.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1f_d1.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1f_d2.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1x_1.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1x_1k.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1x_1l.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1x_1o.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1x_9x.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1x_d1.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1x_d2.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1x_d3.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1x_o.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1y_1.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1y_9x.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1y_d1.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1y_d2.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1y_d3.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1y_o.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1z_9x.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1z_d1.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1z_d2.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo1z_d3.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo2a_9x.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo2a_d1.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo2a_d2.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo_crc.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo_dll.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo_init.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo_ptr.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo_str.c
# End Source File
# Begin Source File

SOURCE=.\src\lzo_util.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\src\compr1b.h
# End Source File
# Begin Source File

SOURCE=.\src\compr1c.h
# End Source File
# Begin Source File

SOURCE=.\src\config1.h
# End Source File
# Begin Source File

SOURCE=.\src\config1a.h
# End Source File
# Begin Source File

SOURCE=.\src\config1b.h
# End Source File
# Begin Source File

SOURCE=.\src\config1c.h
# End Source File
# Begin Source File

SOURCE=.\src\config1f.h
# End Source File
# Begin Source File

SOURCE=.\src\config1x.h
# End Source File
# Begin Source File

SOURCE=.\src\config1y.h
# End Source File
# Begin Source File

SOURCE=.\src\config1z.h
# End Source File
# Begin Source File

SOURCE=.\src\config2a.h
# End Source File
# Begin Source File

SOURCE=.\src\fake16.h
# End Source File
# Begin Source File

SOURCE=.\include\lzo1.h
# End Source File
# Begin Source File

SOURCE=.\include\lzo16bit.h
# End Source File
# Begin Source File

SOURCE=.\include\lzo1a.h
# End Source File
# Begin Source File

SOURCE=.\src\lzo1a_de.h
# End Source File
# Begin Source File

SOURCE=.\include\lzo1b.h
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_cc.h
# End Source File
# Begin Source File

SOURCE=.\src\lzo1b_de.h
# End Source File
# Begin Source File

SOURCE=.\include\lzo1c.h
# End Source File
# Begin Source File

SOURCE=.\src\lzo1c_cc.h
# End Source File
# Begin Source File

SOURCE=.\include\lzo1f.h
# End Source File
# Begin Source File

SOURCE=.\include\lzo1x.h
# End Source File
# Begin Source File

SOURCE=.\include\lzo1y.h
# End Source File
# Begin Source File

SOURCE=.\include\lzo1z.h
# End Source File
# Begin Source File

SOURCE=.\include\lzo2a.h
# End Source File
# Begin Source File

SOURCE=.\src\lzo_conf.h
# End Source File
# Begin Source File

SOURCE=.\src\lzo_dict.h
# End Source File
# Begin Source File

SOURCE=.\src\lzo_ptr.h
# End Source File
# Begin Source File

SOURCE=.\src\lzo_util.h
# End Source File
# Begin Source File

SOURCE=.\include\lzoconf.h
# End Source File
# Begin Source File

SOURCE=.\include\lzoutil.h
# End Source File
# Begin Source File

SOURCE=.\src\stats1a.h
# End Source File
# Begin Source File

SOURCE=.\src\stats1b.h
# End Source File
# Begin Source File

SOURCE=.\src\stats1c.h
# End Source File
# End Group
# End Target
# End Project
