@echo off
set _tank_config=%1
if %1* == * set _tank_config=language

rem Change the following envionment variables for your machine
rem   _tankbuilder: path to tankbuilder.exe
rem   _versionpath: the directory where dungeonsiege.exe resides (needed to extract version info)
rem   _resourcepath: the root of all files and directories that goes into the tank file (make sure there is a lqd_dictionary.ldc6 in there)
rem   _batchpath: the path where make_tank.gas resides

set _tankbuilder=c:\work\ds1\main\tools\tankbuilder.exe
set _versionpath=c:\work\ds1\main\gpg\projects\tattoo\exe\retail
set _resourcepath=c:\work\ds1\main\cdimage
set _batchpath=c:\work\ds1\main\gpg\batch

set _tankoptions=-dump -verify -nologo -home %_batchpath% -in %_resourcepath%+ -build -nobak -is_gpg -version_path %_versionpath%

%_tankbuilder% %_tankoptions% -config %_tank_config%

set _tankoptions=
set _tankbuilder=
set _versionpath=
set _resourcepath=
set _batchpath=
set _tank_config=
