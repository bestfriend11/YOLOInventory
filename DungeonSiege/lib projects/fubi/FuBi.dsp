# Microsoft Developer Studio Project File - Name="FuBi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=FuBi - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FuBi.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FuBi.mak" CFG="FuBi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FuBi - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "FuBi - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "FuBi - Win32 Retail" (based on "Win32 (x86) Static Library")
!MESSAGE "FuBi - Win32 Profiling" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GPG/Lib Projects/FuBi", NJEBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FuBi - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\fubi\Release"
# PROP Intermediate_Dir "\temp\vc6\lib\fubi\Release"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"precomp_fubi.h" /FD /Zm500 /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "FuBi - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\temp\vc6\lib\fubi\debug"
# PROP Intermediate_Dir "\temp\vc6\lib\fubi\debug"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /Zi /Od /D "_DEBUG" /D "WIN32" /Yu"precomp_fubi.h" /FD /GZ /Zm500 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "FuBi - Win32 Retail"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Retail"
# PROP BASE Intermediate_Dir "Retail"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\fubi\Retail"
# PROP Intermediate_Dir "\temp\vc6\lib\fubi\Retail"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MT /W3 /O2 /Ob0 /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /Yu"precomp_fubi.h" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Oy /Ob1 /Gf /Gy /D "NDEBUG" /D GP_RETAIL=1 /D "WIN32" /Yu"precomp_fubi.h" /FD /Gs /Zm500 /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "FuBi - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Profiling"
# PROP BASE Intermediate_Dir "Profiling"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\temp\vc6\lib\FuBi\Profiling"
# PROP Intermediate_Dir "\temp\vc6\lib\FuBi\Profiling"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD BASE CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /Yu"precomp_fubi.h" /FD /Zm500 /Gs /c
# ADD CPP /nologo /G6 /MT /W3 /Zi /Og /Oi /Os /Ob1 /Gf /Gy /D "NDEBUG" /D "WIN32" /D GP_PROFILING=1 /Yu"precomp_fubi.h" /FD /Zm500 /Gs /Gh /c
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

# Name "FuBi - Win32 Release"
# Name "FuBi - Win32 Debug"
# Name "FuBi - Win32 Retail"
# Name "FuBi - Win32 Profiling"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\FuBi.cpp
# End Source File
# Begin Source File

SOURCE=.\FuBiBitPacker.cpp
# End Source File
# Begin Source File

SOURCE=.\FuBiNetTransport.cpp
# End Source File
# Begin Source File

SOURCE=.\FuBiPersist.cpp
# End Source File
# Begin Source File

SOURCE=.\FuBiPersistBinary.cpp
# End Source File
# Begin Source File

SOURCE=.\FuBiPersistImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\FuBiSchema.cpp
# End Source File
# Begin Source File

SOURCE=.\FuBiSchemaImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\FuBiSignature.cpp
# End Source File
# Begin Source File

SOURCE=.\FuBiTraits.cpp
# End Source File
# Begin Source File

SOURCE=.\FuBiTraitsImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\FuBiTypes.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\FuBi.h
# End Source File
# Begin Source File

SOURCE=.\FuBiBitPacker.h
# End Source File
# Begin Source File

SOURCE=.\FuBiDefs.h
# End Source File
# Begin Source File

SOURCE=.\FuBiNetTransport.h
# End Source File
# Begin Source File

SOURCE=.\FuBiPersist.h
# End Source File
# Begin Source File

SOURCE=.\FuBiPersistBinary.h
# End Source File
# Begin Source File

SOURCE=.\FuBiPersistImpl.h
# End Source File
# Begin Source File

SOURCE=.\FuBiSchema.h
# End Source File
# Begin Source File

SOURCE=.\FuBiSchemaImpl.h
# End Source File
# Begin Source File

SOURCE=.\FuBiSignature.h
# End Source File
# Begin Source File

SOURCE=.\FuBiTraits.h
# End Source File
# Begin Source File

SOURCE=.\FuBiTraitsImpl.h
# End Source File
# Begin Source File

SOURCE=.\FuBiTypes.h
# End Source File
# End Group
# Begin Group "Precompiled Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Precomp_FuBi.cpp
# ADD CPP /Yc"precomp_fubi.h"
# End Source File
# Begin Source File

SOURCE=.\Precomp_FuBi.h
# End Source File
# End Group
# Begin Group "Lex/Yacc"

# PROP Default_Filter ""
# Begin Group "Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\FuBiSignature-L.cpp"
# End Source File
# Begin Source File

SOURCE=".\FuBiSignature-Y.cpp"
# End Source File
# Begin Source File

SOURCE=.\FuBiSignature.l

!IF  "$(CFG)" == "FuBi - Win32 Release"

USERDEP__FUBIS="YYLex.cpp"	"FuBiSignature-L.bat"	
# Begin Custom Build - Lexing...
InputPath=.\FuBiSignature.l

BuildCmds= \
	FuBiSignature-L.bat release

"FuBiSignature-L-release.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"FuBiSignature-L-release.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "FuBi - Win32 Debug"

USERDEP__FUBIS="YYLex.cpp"	"FuBiSignature-L.bat"	
# Begin Custom Build - Lexing...
InputPath=.\FuBiSignature.l

BuildCmds= \
	FuBiSignature-L.bat debug

"FuBiSignature-L-debug.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"FuBiSignature-L-debug.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "FuBi - Win32 Retail"

USERDEP__FUBIS="YYLex.cpp"	"FuBiSignature-L.bat"	
# Begin Custom Build - Lexing...
InputPath=.\FuBiSignature.l

BuildCmds= \
	FuBiSignature-L.bat retail

"FuBiSignature-L-retail.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"FuBiSignature-L-retail.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "FuBi - Win32 Profiling"

USERDEP__FUBIS="YYLex.cpp"	"FuBiSignature-L.bat"	
# Begin Custom Build - Lexing...
InputPath=.\FuBiSignature.l

BuildCmds= \
	FuBiSignature-L.bat profiling

"FuBiSignature-L-profiling.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"FuBiSignature-L-profiling.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\FuBiSignature.y

!IF  "$(CFG)" == "FuBi - Win32 Release"

USERDEP__FUBISI="YYParse.cpp"	"FuBiSignature-Y.sed"	"FuBiSignature-Y.bat"	
# Begin Custom Build - Yaccing...
InputPath=.\FuBiSignature.y

BuildCmds= \
	FuBiSignature-Y.bat release

"FuBiSignature-Y-release.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"FuBiSignature-Y-release.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "FuBi - Win32 Debug"

USERDEP__FUBISI="YYParse.cpp"	"FuBiSignature-Y.sed"	"FuBiSignature-Y.bat"	
# Begin Custom Build - Yaccing...
InputPath=.\FuBiSignature.y

BuildCmds= \
	FuBiSignature-Y.bat debug

"FuBiSignature-Y-debug.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"FuBiSignature-Y-debug.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "FuBi - Win32 Retail"

USERDEP__FUBISI="YYParse.cpp"	"FuBiSignature-Y.sed"	"FuBiSignature-Y.bat"	
# Begin Custom Build - Yaccing...
InputPath=.\FuBiSignature.y

BuildCmds= \
	FuBiSignature-Y.bat retail

"FuBiSignature-Y-retail.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"FuBiSignature-Y-retail.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "FuBi - Win32 Profiling"

USERDEP__FUBISI="YYParse.cpp"	"FuBiSignature-Y.sed"	"FuBiSignature-Y.bat"	
# Begin Custom Build - Yaccing...
InputPath=.\FuBiSignature.y

BuildCmds= \
	FuBiSignature-Y.bat profiling

"FuBiSignature-Y-profiling.cpp.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"FuBiSignature-Y-profiling.h.out" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\YYLex.cpp

!IF  "$(CFG)" == "FuBi - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "FuBi - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "FuBi - Win32 Retail"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "FuBi - Win32 Profiling"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\YYParse.cpp

!IF  "$(CFG)" == "FuBi - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "FuBi - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "FuBi - Win32 Retail"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "FuBi - Win32 Profiling"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Gen"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\FuBiSignature-L.bat"
# End Source File
# Begin Source File

SOURCE=".\FuBiSignature-Y.bat"
# End Source File
# Begin Source File

SOURCE=".\FuBiSignature-Y.sed"
# End Source File
# End Group
# End Group
# End Target
# End Project
