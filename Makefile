all : test-proto lproto.so

CC= gcc -std=gnu99

#5.1 5.2.3 5.3.0
LUA_VERSION=5.3.0

LUALIB=-Llua-$(LUA_VERSION)/src -llua
LUAINC=-Ilua-$(LUA_VERSION)/src

LIB=$(LUALIB) -lm -ldl
INC=$(LUAINC)

OS = $(shell uname -s)
ifndef LIBFLAG
	ifeq ($(OS),Darwin)
		LIBFLAG=-bundle -undefined dynamic_lookup -all_load
	else ifeq ($(OS),FreeBSD)
		LIBFLAG=-shared -Wl,-rpath=/usr/local/lib/gcc48
		CC=gcc48
	else
		LIBFLAG=-shared
	endif
endif

test-proto : test/test-proto.c proto.c ldef.c
	$(CC) -g -Wall -o $@ $^ $(INC) $(LIB)

lproto.so : lproto.c proto.c ldef.c
	$(CC) -O2 -Wall $(LIBFLAG) -o $@ $^ $(INC)

clean :
	rm test-proto lproto.so

