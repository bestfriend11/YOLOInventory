# /* DOS 32 bit - gcc (djgpp v2)
#  * Copyright (C) 1996-2000 Markus F.X.J. Oberhumer
#  * type `make -f b/dos32/dj2.mak'
#  */

override b_dos32 = true
liblzo = liblzo.a
o = .o
exe = .exe

CC = gcc
CFLAGS          = @b/gcc.opt @b/dos32/dj2.opt
ASFLAGS         =
LZO_COMPILE_C   = $(CC) $(CFLAGS) @b/gcc_lzo.opt $($(<ba)_CFLAGS) -c $<
LZO_COMPILE_S   = $(CC) -x assembler-with-cpp -Wall $(ASFLAGS) $($(<ba)_CFLAGS) -c $<
LZO_ARLIB       = ar rcs $@ $^
LZO_LINK        = $(CC) -s -o $@ $^
### LZO_LINK       += -Wl,-Map,$(basename $@).map

liblzo_extra_SOURCES = $(LZO_ASM_SOURCES_GCC_i386)

VPATH = .;src;src/i386/src;ltest;examples;tests
include b/lzo.mk

install: all
	cp -p include/lzo*.h $(DJDIR)/include/
	cp -p liblzo.a $(DJDIR)/lib/

