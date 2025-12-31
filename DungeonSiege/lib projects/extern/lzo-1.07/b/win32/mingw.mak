# /* Windows 32 bit (LIB) - Mingw32 GNU-Win32 980309 (gcc 2.8.1)
#  * Copyright (C) 1996-2000 Markus F.X.J. Oberhumer
#  * type `make -f b/win32/mingw.mak'
#  */

override b_win32 = true
liblzo = liblzo.a
o = .o
exe = .exe

CC = gcc
CFLAGS          = -Iinclude -O2 -fomit-frame-pointer -Wall -W -Wno-uninitialized
ASFLAGS         =
LZO_COMPILE_C   = $(CC) $(CFLAGS) -x c $($(<ba)_CFLAGS) -c $<
LZO_COMPILE_S   = $(CC) -x assembler-with-cpp -Wall $(ASFLAGS) $($(<ba)_CFLAGS) -c $<
LZO_ARLIB       = ar rcs $@ $^
LZO_LINK        = $(CC) -s -o $@ $^
### LZO_LINK       += -Wl,-Map,$(basename $@).map

liblzo_extra_SOURCES = $(LZO_ASM_SOURCES_GCC_i386)

VPATH = .;src;src/i386/src;ltest;examples;tests
include b/lzo.mk

