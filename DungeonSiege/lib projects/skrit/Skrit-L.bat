@echo off

rem Pass in build name as first param

rem Stupid MKS Lex puts all the static constants before the #includes. So the
rem PCH can't be used, plus the type of the yy_state_t is char, which can cause
rem problems. This file fixes that up.

lex -LC -p Base -P YYLex.cpp -o Skrit-L-%1.cpp.tmp -D Skrit-L-%1.h.out Skrit.l

if exist Skrit-L-%1.cpp.out del Skrit-L-%1.cpp.out
echo #include "Precomp_Skrit.h" > Skrit-L-%1.cpp.out
echo #include "Skrit.h" >> Skrit-L-%1.cpp.out
type Skrit-L-%1.cpp.tmp >> Skrit-L-%1.cpp.out
del Skrit-L-%1.cpp.tmp
