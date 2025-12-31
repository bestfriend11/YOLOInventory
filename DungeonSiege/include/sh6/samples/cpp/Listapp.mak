# Microsoft Developer Studio Generated NMAKE File, Based on Listapp.dsp
!IF "$(CFG)" == ""
CFG=Listapp - Win32 Debug
!MESSAGE No configuration specified. Defaulting to Listapp - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Listapp - Win32 Release" && "$(CFG)" != "Listapp - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Listapp.mak" CFG="Listapp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Listapp - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Listapp - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Listapp - Win32 Release"

OUTDIR=.\WinRel
INTDIR=.\WinRel
# Begin Custom Macros
OutDir=.\WinRel
# End Custom Macros

ALL : "$(OUTDIR)\Listapp.exe"


CLEAN :
	-@erase "$(INTDIR)\LIST.OBJ"
	-@erase "$(INTDIR)\LISTAPP.OBJ"
	-@erase "$(INTDIR)\LISTAPP.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\Listapp.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\Listapp.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\LISTAPP.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Listapp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=shlsmpmt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\Listapp.pdb" /machine:I386 /include:"_SmartHeap_malloc" /out:"$(OUTDIR)\Listapp.exe" 
LINK32_OBJS= \
	"$(INTDIR)\LIST.OBJ" \
	"$(INTDIR)\LISTAPP.OBJ" \
	"$(INTDIR)\LISTAPP.res"

"$(OUTDIR)\Listapp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Listapp - Win32 Debug"

OUTDIR=.\WinDebug
INTDIR=.\WinDebug
# Begin Custom Macros
OutDir=.\WinDebug
# End Custom Macros

ALL : "$(OUTDIR)\Listapp.exe"


CLEAN :
	-@erase "$(INTDIR)\LIST.OBJ"
	-@erase "$(INTDIR)\LISTAPP.OBJ"
	-@erase "$(INTDIR)\LISTAPP.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\Listapp.exe"
	-@erase "$(OUTDIR)\Listapp.ilk"
	-@erase "$(OUTDIR)\Listapp.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D MEM_DEBUG=1 /Fp"$(INTDIR)\Listapp.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\LISTAPP.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Listapp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=haw32m.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\Listapp.pdb" /debug /machine:I386 /out:"$(OUTDIR)\Listapp.exe" 
LINK32_OBJS= \
	"$(INTDIR)\LIST.OBJ" \
	"$(INTDIR)\LISTAPP.OBJ" \
	"$(INTDIR)\LISTAPP.res"

"$(OUTDIR)\Listapp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("Listapp.dep")
!INCLUDE "Listapp.dep"
!ELSE 
!MESSAGE Warning: cannot find "Listapp.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "Listapp - Win32 Release" || "$(CFG)" == "Listapp - Win32 Debug"
SOURCE=.\LIST.CPP

"$(INTDIR)\LIST.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\LISTAPP.CPP

"$(INTDIR)\LISTAPP.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\LISTAPP.RC

"$(INTDIR)\LISTAPP.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

