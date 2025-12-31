# Microsoft Developer Studio Project File - Name="GpCore" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=GpCore - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GPCore.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GPCore.mak" CFG="GpCore - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GpCore - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "GpCore - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "GpCore - Win32 Retail" (based on "Win32 (x86) Static Library")
!MESSAGE "GpCore - Win32 Profiling" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/LIB Projects/GpCore", PTLAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GpCore - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\gpcore\Release"
# PROP Intermediate_Dir "\temp\vc6\lib\gpcore\Release"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"precomp_gpcore.h" /FD /Zm500 /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "GpCore - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\lib\gpcore\Debug"
# PROP Intermediate_Dir "\temp\vc6\lib\gpcore\Debug"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /Zi /Od /D "_DEBUG" /D "WIN32" /Yu"precomp_gpcore.h" /FD /GZ /Zm500 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "GpCore - Win32 Retail"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Retail"
# PROP BASE Intermediate_Dir "Retail"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\GpCore\Retail"
# PROP Intermediate_Dir "\temp\vc6\lib\GpCore\Retail"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MT /W3 /O2 /Ob0 /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /Yu"precomp_gpcore.h" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Oy /Ob1 /Gf /Gy /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /Yu"precomp_gpcore.h" /FD /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\lib\gpcore.lib"
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "GpCore - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Profiling"
# PROP BASE Intermediate_Dir "Profiling"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\GpCore\Profiling"
# PROP Intermediate_Dir "\temp\vc6\lib\GpCore\Profiling"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"precomp_gpcore.h" /FD /Zm500 /Gs /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /D GP_PROFILING=1 /Yu"precomp_gpcore.h" /FD /Zm500 /Gs /Gh /c
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

# Name "GpCore - Win32 Release"
# Name "GpCore - Win32 Debug"
# Name "GpCore - Win32 Retail"
# Name "GpCore - Win32 Profiling"
# Begin Group "Fuel"

# PROP Default_Filter ""
# Begin Group "Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Fuel\fuel.h
# End Source File
# Begin Source File

SOURCE=.\Fuel\fuel_types.h
# End Source File
# Begin Source File

SOURCE=.\Fuel\fueldb.h
# End Source File
# End Group
# Begin Group "Implementation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Fuel\fuel.cpp
# End Source File
# Begin Source File

SOURCE=.\Fuel\fueldb.cpp
# End Source File
# End Group
# End Group
# Begin Group "Math"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Math\axis_aligned_bounding_box_3.h
# End Source File
# Begin Source File

SOURCE=.\Math\filter_1.h
# End Source File
# Begin Source File

SOURCE=.\Math\gpmath.h
# End Source File
# Begin Source File

SOURCE=.\Math\line_segment_3.h
# End Source File
# Begin Source File

SOURCE=.\Math\matrix_3x3.cpp
# End Source File
# Begin Source File

SOURCE=.\Math\matrix_3x3.h
# End Source File
# Begin Source File

SOURCE=.\Math\oriented_bounding_box_3.cpp
# End Source File
# Begin Source File

SOURCE=.\Math\oriented_bounding_box_3.h
# End Source File
# Begin Source File

SOURCE=.\Math\plane_3.h
# End Source File
# Begin Source File

SOURCE=.\Math\Point2.cpp
# End Source File
# Begin Source File

SOURCE=.\Math\Point2.h
# End Source File
# Begin Source File

SOURCE=.\Math\quat.cpp
# End Source File
# Begin Source File

SOURCE=.\Math\quat.h
# End Source File
# Begin Source File

SOURCE=.\Math\simd.h
# End Source File
# Begin Source File

SOURCE=.\Math\space_2.cpp
# End Source File
# Begin Source File

SOURCE=.\Math\space_2.h
# End Source File
# Begin Source File

SOURCE=.\Math\space_3.cpp
# End Source File
# Begin Source File

SOURCE=.\Math\space_3.h
# End Source File
# Begin Source File

SOURCE=.\Math\sphere_3.h
# End Source File
# Begin Source File

SOURCE=.\Math\spline.h
# End Source File
# Begin Source File

SOURCE=.\Math\triangle_3.cpp
# End Source File
# Begin Source File

SOURCE=.\Math\triangle_3.h
# End Source File
# Begin Source File

SOURCE=.\Math\vector_3.cpp
# End Source File
# Begin Source File

SOURCE=.\Math\vector_3.h
# End Source File
# End Group
# Begin Group "Util Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AppModule.cpp
# End Source File
# Begin Source File

SOURCE=.\AtlX.cpp
# End Source File
# Begin Source File

SOURCE=.\Config.cpp
# End Source File
# Begin Source File

SOURCE=.\DebugHelp.cpp
# End Source File
# Begin Source File

SOURCE=.\DllBinder.cpp
# End Source File
# Begin Source File

SOURCE=.\DrWatson.cpp
# End Source File
# Begin Source File

SOURCE=.\gpassert.cpp
# End Source File
# Begin Source File

SOURCE=.\GpColl.cpp
# End Source File
# Begin Source File

SOURCE=.\gpconsole.cpp
# End Source File
# Begin Source File

SOURCE=.\gpglobal.cpp
# End Source File
# Begin Source File

SOURCE=.\GpLzo.cpp
# End Source File
# Begin Source File

SOURCE=.\gpmem.cpp
# End Source File
# Begin Source File

SOURCE=.\gpprofiler.cpp
# End Source File
# Begin Source File

SOURCE=.\GpReport.cpp
# End Source File
# Begin Source File

SOURCE=.\GpStats.cpp
# End Source File
# Begin Source File

SOURCE=.\gpstring.cpp
# End Source File
# Begin Source File

SOURCE=.\GpZLib.cpp
# End Source File
# Begin Source File

SOURCE=.\InputBinder.cpp
# End Source File
# Begin Source File

SOURCE=.\KernelTool.cpp
# End Source File
# Begin Source File

SOURCE=.\KernelToolAsm.cpp

!IF  "$(CFG)" == "GpCore - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "GpCore - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "GpCore - Win32 Retail"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "GpCore - Win32 Profiling"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\keys.cpp
# End Source File
# Begin Source File

SOURCE=.\LocHelp.cpp
# End Source File
# Begin Source File

SOURCE="..\Extern\lzo-free\lzo_free.c"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\namingkey.cpp
# End Source File
# Begin Source File

SOURCE=.\ProgressStack.cpp
# End Source File
# Begin Source File

SOURCE=.\RatioStack.cpp
# End Source File
# Begin Source File

SOURCE=.\ReportSys.cpp
# End Source File
# Begin Source File

SOURCE=.\ReportSysDialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\ResHandle.cpp
# End Source File
# Begin Source File

SOURCE=.\stringtool.cpp
# End Source File
# Begin Source File

SOURCE=.\stringtoolaw.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\Timeline.cpp
# End Source File
# Begin Source File

SOURCE=.\winx.cpp
# End Source File
# End Group
# Begin Group "Util Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AppModule.h
# End Source File
# Begin Source File

SOURCE=.\AtlX.h
# End Source File
# Begin Source File

SOURCE=.\BlindCallback.h
# End Source File
# Begin Source File

SOURCE=.\BucketVector.h
# End Source File
# Begin Source File

SOURCE=.\Config.h
# End Source File
# Begin Source File

SOURCE=.\DebugHelp.h
# End Source File
# Begin Source File

SOURCE=.\DllBinder.h
# End Source File
# Begin Source File

SOURCE=.\DrWatson.h
# End Source File
# Begin Source File

SOURCE=.\gpassert.h
# End Source File
# Begin Source File

SOURCE=.\GpColl.h
# End Source File
# Begin Source File

SOURCE=.\gpconsole.h
# End Source File
# Begin Source File

SOURCE=.\gpcore.h
# End Source File
# Begin Source File

SOURCE=.\gpcoredefs.h
# End Source File
# Begin Source File

SOURCE=.\gpglobal.h
# End Source File
# Begin Source File

SOURCE=.\GpLzo.h
# End Source File
# Begin Source File

SOURCE=.\gpmem.h
# End Source File
# Begin Source File

SOURCE=.\gpmem_new_off.h
# End Source File
# Begin Source File

SOURCE=.\gpmem_new_on.h
# End Source File
# Begin Source File

SOURCE=.\gpprofiler.h
# End Source File
# Begin Source File

SOURCE=.\GpReport.h
# End Source File
# Begin Source File

SOURCE=.\GpStats.h
# End Source File
# Begin Source File

SOURCE=.\GpStatsDefs.h
# End Source File
# Begin Source File

SOURCE=.\gpstring.h
# End Source File
# Begin Source File

SOURCE=.\gpstring_std.h
# End Source File
# Begin Source File

SOURCE=.\GpZLib.h
# End Source File
# Begin Source File

SOURCE=.\IgnoredWarnings.h
# End Source File
# Begin Source File

SOURCE=.\InputBinder.h
# End Source File
# Begin Source File

SOURCE=.\KernelTool.h
# End Source File
# Begin Source File

SOURCE=.\Keys.h
# End Source File
# Begin Source File

SOURCE=.\LocHelp.h
# End Source File
# Begin Source File

SOURCE="..\Extern\lzo-free\lzo_free.h"
# End Source File
# Begin Source File

SOURCE=.\msodw.h
# End Source File
# Begin Source File

SOURCE=.\namingkey.h
# End Source File
# Begin Source File

SOURCE=.\ProgressStack.h
# End Source File
# Begin Source File

SOURCE=.\PtrHelp.h
# End Source File
# Begin Source File

SOURCE=.\RatioStack.h
# End Source File
# Begin Source File

SOURCE=.\ReportSys.h
# End Source File
# Begin Source File

SOURCE=.\ReportSysDialogs.h
# End Source File
# Begin Source File

SOURCE=.\ResHandle.h
# End Source File
# Begin Source File

SOURCE=.\stdhelp.h
# End Source File
# Begin Source File

SOURCE=.\stringtool.h
# End Source File
# Begin Source File

SOURCE=.\Timeline.h
# End Source File
# Begin Source File

SOURCE=.\WinGeometry.h
# End Source File
# Begin Source File

SOURCE=.\winx.h
# End Source File
# End Group
# Begin Group "FileSys Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FileSys\FileSys.cpp
# End Source File
# Begin Source File

SOURCE=.\FileSys\FileSysDiyMap.cpp
# End Source File
# Begin Source File

SOURCE=.\FileSys\FileSysUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\FileSys\FileSysUtilsAW.cpp

!IF  "$(CFG)" == "GpCore - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "GpCore - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "GpCore - Win32 Retail"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "GpCore - Win32 Profiling"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\FileSys\FileSysXfer.cpp
# End Source File
# Begin Source File

SOURCE=.\FileSys\MasterFileMgr.cpp
# End Source File
# Begin Source File

SOURCE=.\FileSys\PathFileMgr.cpp
# End Source File
# Begin Source File

SOURCE=.\FileSys\TankBuilder.cpp
# End Source File
# Begin Source File

SOURCE=.\FileSys\TankFileMgr.cpp
# End Source File
# Begin Source File

SOURCE=.\FileSys\TankStructure.cpp
# End Source File
# End Group
# Begin Group "FileSys Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FileSys\FileSys.h
# End Source File
# Begin Source File

SOURCE=.\FileSys\FileSysDefs.h
# End Source File
# Begin Source File

SOURCE=.\FileSys\FileSysDiyMap.h
# End Source File
# Begin Source File

SOURCE=.\FileSys\FileSysHandle.h
# End Source File
# Begin Source File

SOURCE=.\FileSys\FileSysHandleMgr.h
# End Source File
# Begin Source File

SOURCE=.\FileSys\FileSysUtils.h
# End Source File
# Begin Source File

SOURCE=.\FileSys\FileSysXfer.h
# End Source File
# Begin Source File

SOURCE=.\FileSys\MasterFileMgr.h
# End Source File
# Begin Source File

SOURCE=.\FileSys\PathFileMgr.h
# End Source File
# Begin Source File

SOURCE=.\FileSys\TankBuilder.h
# End Source File
# Begin Source File

SOURCE=.\FileSys\TankFileMgr.h
# End Source File
# Begin Source File

SOURCE=.\FileSys\TankStructure.h
# End Source File
# End Group
# Begin Group "Options"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\RetailOptions.h
# End Source File
# Begin Source File

SOURCE=.\Local\UserCore.h
# End Source File
# End Group
# Begin Group "Profiling"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Quantify\pure.h
# End Source File
# Begin Source File

SOURCE=.\Quantify\pure_api.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Quantify\Quantify.cpp
# End Source File
# Begin Source File

SOURCE=.\Quantify\Quantify.h
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Res\GpCoreRes.rc

!IF  "$(CFG)" == "GpCore - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__GPCOR="Res\GpCoreRes.rh"	
# Begin Custom Build - De-RES-ing GpCore resources...
InputDir=.\Res
InputPath=.\Res\GpCoreRes.rc
InputName=GpCoreRes

"$(InputDir)\$(InputName)-Release.dat" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	rc $(InputPath) 
	deres $(InputDir)\$(InputName)-Release.dat GP_ =$(InputDir)\$(InputName).res 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "GpCore - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__GPCOR="Res\GpCoreRes.rh"	
# Begin Custom Build - De-RES-ing GpCore resources...
InputDir=.\Res
InputPath=.\Res\GpCoreRes.rc
InputName=GpCoreRes

"$(InputDir)\$(InputName)-Debug.dat" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	rc $(InputPath) 
	deres $(InputDir)\$(InputName)-Debug.dat GP_ =$(InputDir)\$(InputName).res 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "GpCore - Win32 Retail"

# PROP Ignore_Default_Tool 1
USERDEP__GPCOR="Res\GpCoreRes.rh"	
# Begin Custom Build - De-RES-ing GpCore resources...
InputDir=.\Res
InputPath=.\Res\GpCoreRes.rc
InputName=GpCoreRes

"$(InputDir)\$(InputName)-Retail.dat" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	rc $(InputPath) 
	deres $(InputDir)\$(InputName)-Retail.dat GP_ =$(InputDir)\$(InputName).res 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "GpCore - Win32 Profiling"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
USERDEP__GPCOR="Res\GpCoreRes.rh"	
# Begin Custom Build - De-RES-ing GpCore resources...
InputDir=.\Res
InputPath=.\Res\GpCoreRes.rc
InputName=GpCoreRes

"$(InputDir)\$(InputName)-Profiling.dat" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	rc $(InputPath) 
	deres $(InputDir)\$(InputName)-Profiling.dat GP_ =$(InputDir)\$(InputName).res 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Res\GpCoreRes.rh
# End Source File
# End Group
# Begin Group "Precompiled Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\precomp_gpcore.cpp
# ADD CPP /Yc"precomp_gpcore.h"
# End Source File
# Begin Source File

SOURCE=.\precomp_gpcore.h
# End Source File
# End Group
# Begin Group "$$$ Marked For Death"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\circular_buffer.h
# End Source File
# Begin Source File

SOURCE=.\system_report.cpp
# End Source File
# Begin Source File

SOURCE=.\system_report.h
# End Source File
# End Group
# End Target
# End Project
