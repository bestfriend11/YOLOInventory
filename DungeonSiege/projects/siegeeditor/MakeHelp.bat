@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by SIEGEEDITOR.HPJ. >"hlp\SiegeEditor.hm"
echo. >>"hlp\SiegeEditor.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\SiegeEditor.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\SiegeEditor.hm"
echo. >>"hlp\SiegeEditor.hm"
echo // Prompts (IDP_*) >>"hlp\SiegeEditor.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\SiegeEditor.hm"
echo. >>"hlp\SiegeEditor.hm"
echo // Resources (IDR_*) >>"hlp\SiegeEditor.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\SiegeEditor.hm"
echo. >>"hlp\SiegeEditor.hm"
echo // Dialogs (IDD_*) >>"hlp\SiegeEditor.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\SiegeEditor.hm"
echo. >>"hlp\SiegeEditor.hm"
echo // Frame Controls (IDW_*) >>"hlp\SiegeEditor.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\SiegeEditor.hm"
REM -- Make help for Project SIEGEEDITOR


echo Building Win32 Help files
start /wait hcw /C /E /M "hlp\SiegeEditor.hpj"
if errorlevel 1 goto :Error
if not exist "hlp\SiegeEditor.hlp" goto :Error
if not exist "hlp\SiegeEditor.cnt" goto :Error
echo.
if exist Debug\nul copy "hlp\SiegeEditor.hlp" Debug
if exist Debug\nul copy "hlp\SiegeEditor.cnt" Debug
if exist Release\nul copy "hlp\SiegeEditor.hlp" Release
if exist Release\nul copy "hlp\SiegeEditor.cnt" Release
echo.
goto :done

:Error
echo hlp\SiegeEditor.hpj(1) : error: Problem encountered creating help file

:done
echo.
