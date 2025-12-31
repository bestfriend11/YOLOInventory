@echo off

rem Pass in build name as first param

rem Stupid MKS Yacc doesn't do -p properly so I have to reprocess with sed. See
rem the .sed file for what it screws up on.

yacc -LC -p Base -P YYParse.cpp -o Skrit-Y-%1.cpp.tmp -D Skrit-Y-%1.h.tmp Skrit.y

if exist Skrit-Y-%1.cpp.out del Skrit-Y-%1.cpp.out
sed -f Skrit-Y.sed Skrit-Y-%1.cpp.tmp > Skrit-Y-%1.cpp.out
del Skrit-Y-%1.cpp.tmp

if exist Skrit-Y-%1.h.out del Skrit-Y-%1.h.out
sed -f Skrit-Y.sed Skrit-Y-%1.h.tmp > Skrit-Y-%1.h.out
del Skrit-Y-%1.h.tmp
