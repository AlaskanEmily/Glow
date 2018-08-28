# Any copyright is dedicated to the Public Domain.
# http://creativecommons.org/publicdomain/zero/1.0/

UNAME=$(shell uname)
HAIKU=Haiku

ifeq "$(UNAME)" "Haiku"

all: libglow.so libglow.a

CXXFLAGS=-fno-rtti -fno-exceptions -g -std=c++98 -Wall

glow_haiku.os: glow_haiku.cpp glow.h
	g++ $(CXXFLAGS) -fPIC -c glow_haiku.cpp -o glow_haiku.os

glow_haiku.o: glow_haiku.cpp glow.h
	g++ $(CXXFLAGS) -c glow_haiku.cpp -o glow_haiku.o

libglow.a: glow_haiku.o
	ar rc libglow.a glow_haiku.o
	ranlib libglow.a

libglow.so: glow_haiku.os
	g++ -g -fPIC -shared glow_haiku.os -o libglow.so

else

all: libglow.so libglow.a

include gcc.mk

endif
