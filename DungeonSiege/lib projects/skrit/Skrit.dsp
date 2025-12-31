# Microsoft Developer Studio Project File - Name="Skrit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Skrit - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Skrit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Skrit.mak" CFG="Skrit - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Skrit - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Skrit - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "Skrit - Win32 Retail" (based on "Win32 (x86) Static Library")
!MESSAGE "Skrit - Win32 Profiling" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/Lib Projects/Skrit", NJEBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Skrit - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\Skrit\Release"
# PROP Intermediate_Dir "\temp\vc6\lib\Skrit\Release"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"precomp_Skrit.h" /FD /Zm500 /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Skrit - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\lib\Skrit\debug"
# PROP Intermediate_Dir "\temp\vc6\lib\Skrit\debug"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /Zi /Od /D "_DEBUG" /D "WIN32" /Yu"precomp_Skrit.h" /FD /GZ /Zm500 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Skrit - Win32 Retail"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Retail"
# PROP BASE Intermediate_Dir "Retail"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\Skrit\Retail"
# PROP Intermediate_Dir "\temp\vc6\lib\Skrit\Retail"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MT /W3 /O2 /Ob0 /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /Yu"precomp_Skrit.h" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Oy /Ob1 /Gf /Gy /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /Yu"precomp_Skrit.h" /FD /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Skrit - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Profiling"
# PROP BASE Intermediate_Dir "Profiling"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\Skrit\Profiling"
# PROP Intermediate_Dir "\temp\vc6\lib\Skrit\Profiling"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"precomp_Skrit.h" /FD /Zm500 /Gs /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /D GP_PROFILING=1 /Yu"precomp_Skrit.h" /FD /Zm500 /Gs /Gh /c
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

# Name "Skrit - Win32 Release"
# Name "Skrit - Win32 Debug"
# Name "Skrit - Win32 Retail"
# Name "Skrit - Win32 Profiling"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Skrit.cpp
# End Source File
# Begin Source File

SOURCE=.\SkritByteCode.cpp
# End Source File
# Begin Source File

SOURCE=.\SkritEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\SkritMachine.cpp
# End Source File
# Begin Source File

SOURCE=.\SkritObject.cpp
# End Source File
# Begin Source File

SOURCE=.\SkritStructure.cpp
# End Source File
# Begin Source File

SOURCE=.\SkritSupport.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "Listing Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SkritByteCode.inc
# End Source File
# End Group
# Begin Source File

SOURCE=.\Skrit.h
# End Source File
# Begin Source File

SOURCE=.\SkritByteCode.h
# End Source File
# Begin Source File

SOURCE=.\SkritDefs.h
# End Source File
# Begin Source File

SOURCE=.\SkritEngine.h
# End Source File
# Begin Source File

SOURCE=.\SkritMachine.h
# End Source File
# Begin Source File

SOURCE=.\SkritObject.h
# End Source File
# Begin Source File

SOURCE=.\SkritStructure.h
# End Source File
# Begin Source File

SOURCE=.\SkritSupport.h
# End Source File
# End Group
# Begin Group "Precompiled Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Precomp_Skrit.cpp
# ADD CPP /Yc"precomp_Skrit.h"
# End Source File
# Begin Source File

SOURCE=.\Precomp_Skrit.h
# End Source File
# End Group
# Begin Group "Lex/Yacc"

# PROP Default_Filter ""
# Begin Group "Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\Skrit-L.cpp"
# ADD CPP /Yu
# End Source File
# Begin Source File

SOURCE=".\Skrit-Y.cpp"
# End Source File
# Begin Source File

SOURCE=.\Skrit.l

!IF  "$(CFG)" == "Skrit - Win32 Release"

USERDEP__SKRIT="YYLex.cpp"	"Skrit-L.bat"	
# Begin Custom Build - Lexing...
InputPath=.\Skrit.l

BuildCmds= \
	Skrit-L.bat release

"Skrit-L-release.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"Skrit-L-release.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "Skrit - Win32 Debug"

USERDEP__SKRIT="YYLex.cpp"	"Skrit-L.bat"	
# Begin Custom Build - Lexing...
InputPath=.\Skrit.l

BuildCmds= \
	Skrit-L.bat debug

"Skrit-L-debug.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"Skrit-L-debug.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "Skrit - Win32 Retail"

USERDEP__SKRIT="YYLex.cpp"	"Skrit-L.bat"	
# Begin Custom Build - Lexing...
InputPath=.\Skrit.l

BuildCmds= \
	Skrit-L.bat retail

"Skrit-L-retail.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"Skrit-L-retail.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "Skrit - Win32 Profiling"

USERDEP__SKRIT="YYLex.cpp"	"Skrit-L.bat"	
# Begin Custom Build - Lexing...
InputPath=.\Skrit.l

BuildCmds= \
	Skrit-L.bat profiling

"Skrit-L-profiling.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"Skrit-L-profiling.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Skrit.y

!IF  "$(CFG)" == "Skrit - Win32 Release"

USERDEP__SKRIT_="YYParse.cpp"	"Skrit-Y.bat"	"Skrit-Y.sed"	
# Begin Custom Build - Yaccing...
InputPath=.\Skrit.y

BuildCmds= \
	Skrit-Y.bat release

"Skrit-Y-release.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"Skrit-Y-release.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "Skrit - Win32 Debug"

USERDEP__SKRIT_="YYParse.cpp"	"Skrit-Y.bat"	"Skrit-Y.sed"	
# Begin Custom Build - Yaccing...
InputPath=.\Skrit.y

BuildCmds= \
	Skrit-Y.bat debug

"Skrit-Y-debug.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"Skrit-Y-debug.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "Skrit - Win32 Retail"

USERDEP__SKRIT_="YYParse.cpp"	"Skrit-Y.bat"	"Skrit-Y.sed"	
# Begin Custom Build - Yaccing...
InputPath=.\Skrit.y

BuildCmds= \
	Skrit-Y.bat retail

"Skrit-Y-retail.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"Skrit-Y-retail.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "Skrit - Win32 Profiling"

USERDEP__SKRIT_="YYParse.cpp"	"Skrit-Y.bat"	"Skrit-Y.sed"	
# Begin Custom Build - Yaccing...
InputPath=.\Skrit.y

BuildCmds= \
	Skrit-Y.bat profiling

"Skrit-Y-profiling.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"Skrit-Y-profiling.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\YYLex.cpp

!IF  "$(CFG)" == "Skrit - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Skrit - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Skrit - Win32 Retail"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Skrit - Win32 Profiling"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\YYParse.cpp

!IF  "$(CFG)" == "Skrit - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Skrit - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Skrit - Win32 Retail"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Skrit - Win32 Profiling"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Gen"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\Skrit-L.bat"
# End Source File
# Begin Source File

SOURCE=".\Skrit-Y.bat"
# End Source File
# Begin Source File

SOURCE=".\Skrit-Y.sed"
# End Source File
# End Group
# End Group
# End Target
# End Project
