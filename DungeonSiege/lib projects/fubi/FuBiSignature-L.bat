@echo off

rem Pass in build name as first param

rem Stupid MKS Lex puts all the static constants before the #includes. So the
rem PCH can't be used, plus the type of the yy_state_t is char, which can cause
rem problems. This file fixes that up.

lex -LC -p SignatureBase -P YYLex.cpp -o FuBiSignature-L-%1.cpp.tmp -D FuBiSignature-L-%1.h.out FuBiSignature.l

if exist FuBiSignature-L-%1.cpp.out del FuBiSignature-L-%1.cpp.out
echo #include "Precomp_FuBi.h" > FuBiSignature-L-%1.cpp.out
echo #include "FuBiSignature.h" >> FuBiSignature-L-%1.cpp.out
type FuBiSignature-L-%1.cpp.tmp >> FuBiSignature-L-%1.cpp.out
del FuBiSignature-L-%1.cpp.tmp
