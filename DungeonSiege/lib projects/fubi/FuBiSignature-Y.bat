@echo off

rem Pass in build name as first param

rem Stupid MKS Yacc doesn't do -p properly so I have to reprocess with sed. See
rem the .sed file for what it screws up on.

yacc -LC -p SignatureBase -P YYParse.cpp -o FuBiSignature-Y-%1.cpp.tmp -D FuBiSignature-Y-%1.h.tmp FuBiSignature.y

if exist FuBiSignature-Y-%1.cpp.out del FuBiSignature-Y-%1.cpp.out
sed -f FuBiSignature-Y.sed FuBiSignature-Y-%1.cpp.tmp > FuBiSignature-Y-%1.cpp.out
del FuBiSignature-Y-%1.cpp.tmp

if exist FuBiSignature-Y-%1.h.out del FuBiSignature-Y-%1.h.out
sed -f FuBiSignature-Y.sed FuBiSignature-Y-%1.h.tmp > FuBiSignature-Y-%1.h.out
del FuBiSignature-Y-%1.h.tmp
