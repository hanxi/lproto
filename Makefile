LUA_CLIB_PATH ?= luaclib

CFLAGS = -g -Wall $(MYCFLAGS)

#linux : CFLAGS += -O2

LIBS =  -lm
SHARED = -fPIC --shared
EXPORT = -Wl,-E

LIBS += -ldl -lrt 

LUA_CLIB = lproto

all : \
	$(foreach v, $(LUA_CLIB), $(LUA_CLIB_PATH)/$(v).so) 

$(LUA_CLIB_PATH) :
	mkdir $(LUA_CLIB_PATH)

$(LUA_CLIB_PATH)/lproto.so : src/lproto.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ $(LIBS)

clean :
	rm -f $(LUA_CLIB_PATH)/*.so
