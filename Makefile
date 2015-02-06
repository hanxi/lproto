<<<<<<< HEAD
PLAT ?= none
PLATS = linux macosx

CC ?= gcc

.PHONY : none $(PLATS) clean lua all

ifneq ($(PLAT), none)

.PHONY : default

default :
	$(MAKE) $(PLAT)

endif

none :
	@echo "Please do 'make PLATFORM' where PLATFORM is one of these:"
	@echo "   $(PLATS)"

LUA_CLIB_PATH ?= luaclib
=======
all : test-proto lproto.so
>>>>>>> github-dev

CC= gcc -std=gnu99

<<<<<<< HEAD
linux : CFLAGS += -std=c99

LIBS = -lpthread -lm
SHARED = -fPIC --shared
EXPORT = -Wl,-E

$(PLATS) : all

linux : PLAT = linux
macosx : PLAT = macosx

macosx linux : LIBS += -ldl
macosx : SHARED = -fPIC -dynamiclib -Wl,-undefined,dynamic_lookup
linux : LIBS += -lrt -lpthread
=======
#5.1 5.2.3 5.3.0
LUA_VERSION=5.3.0

LUALIB=-Llua-$(LUA_VERSION)/src -llua
LUAINC=-Ilua-$(LUA_VERSION)/src

LIB=$(LUALIB) -lm -ldl
INC=$(LUAINC)
>>>>>>> github-dev

test-proto : test/test-proto.c proto.c ldef.c
	$(CC) -g -Wall -o $@ $^ $(INC) $(LIB)

lproto.so : lproto.c proto.c ldef.c
	$(CC) -O2 -Wall -shared -fPIC -o $@ $^ $(INC)

clean :
	rm test-proto lproto.so

