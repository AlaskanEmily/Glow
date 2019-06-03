# Any copyright is dedicated to the Public Domain.
# http://creativecommons.org/publicdomain/zero/1.0/

CC?=gcc
CXX?=g++
CFLAGS?=-Wall -Werror -ansi -g
FPICFLAGS?=-fPIC
LINKER?=$(CC)
SHAREDFLAGS?=-shared
AR?=ar
RANLIB?=ranlib
GLOWTARGET?=x11

X11INCLUDE=-I/usr/X11R6/include

glow_x11.o: glow_x11.c glow.h
	$(CC) $(CFLAGS) $(X11INCLUDE) -c glow_x11.c -o glow_x11.o

glow_win32.o: glow_win32.c glow.h
	$(CC) $(CFLAGS) -DGLOW_DLL -D_WIN32 -DGLOW_EXPORTS -c glow_win32.c -o glow_win32.o

glow_x11.os: glow_x11.c glow.h
	$(CC) $(CFLAGS) $(FPICFLAGS) $(X11INCLUDE) -c glow_x11.c -o glow_x11.os

glow_sdl2.o: glow_sdl2.c glow.h
	$(CC) $(CFLAGS) -c glow_sdl2.c -o glow_sdl2.o

glow_sdl2.os: glow_sdl2.c glow.h
	$(CC) $(CFLAGS) $(FPICFLAGS) -c glow_sdl2.c -o glow_sdl2.os

libglow.so: glow_$(GLOWTARGET).os
	$(LINKER) $(SHAREDFLAGS) glow_$(GLOWTARGET).os -o libglow.so

libglow.a: glow_$(GLOWTARGET).o
	$(AR) rc libglow.a glow_$(GLOWTARGET).o
	$(RANLIB) libglow.a
