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

CFLAGS = -g -Wall $(MYCFLAGS)

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

LUA_CLIB = lproto

all : \
	$(foreach v, $(LUA_CLIB), $(LUA_CLIB_PATH)/$(v).so) 

$(LUA_CLIB_PATH) :
	mkdir $(LUA_CLIB_PATH)

$(LUA_CLIB_PATH)/lproto.so : src/lproto.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ $(LIBS)

clean :
	rm -f $(LUA_CLIB_PATH)/*.so
