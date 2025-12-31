# /* Windows 32 bit (LIB) - Microsoft 32-bit C/C++ Compiler 12.00
#  * Copyright (C) 1996-2000 Markus F.X.J. Oberhumer
#  * type `make -f b/win32/mc120.mak'
#  */

override b_win32 = true
liblzo = lzo.lib
o = .obj
exe = .exe

CC = cl -nologo -MD
CFLAGS          = -Iinclude -O2 -GF -W3
LZO_COMPILE_C   = $(CC) $(CFLAGS) $($(<ba)_CFLAGS) -c $(<bs)
LZO_COMPILE_ASM = $(CC) $($(<ba)_CFLAGS) -c $(<bs)
LZO_ARLIB       = lib -nologo -out:$(@bs) @b\\win32\\mc120.rsp
LZO_LINK        = $(CC) -Fe$(@bs) $(^bs) setargv.obj

VPATH = .;src;ltest;examples;tests
include b/lzo.mk

