# Any copyright is dedicated to the Public Domain.
# http://creativecommons.org/publicdomain/zero/1.0/

.if defined(OS) && (${OS} == "WIN")

all: glow.dll glow.lib

glow_win32.obj: glow_win32.c glow.h
	cl /Foglow_win32.obj /c glow_win32.c /DGLOW_DLL /nologo

glow.dll glow.lib:
	link /nologo /dll /out:glow.dll /implib:glow.lib OpenGL32.lib gdi32.lib user32.lib glow_win32.obj

.elif defined(OS) && (${OS} == "CYGWIN")

all: libglow.dll

.include "gcc.mk"

libglow.dll: glow_win32.o
	$(CC) -shared -o libglow.dll glow_win32.o -lOpenGL32 -lGDI32

.else

all: 
	echo ${OS}

# .include "gcc.mk"

.endif
