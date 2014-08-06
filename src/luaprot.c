#include <lua.h>
#include <lauxlib.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

static const char T1 = 0x01; // 正,8位
static const char t1 = 0xF1; // 负,8位
static const char T2 = 0x02; // 正,16位
static const char t2 = 0xF2; // 负,16位
static const char T3 = 0x04; // 正,32位
static const char t3 = 0xF4; // 负,32位
static const char T4 = 0x08; // 正,64位
static const char t4 = 0xF8; // 夫,64位

#define BUFFER_MAX_LEN 10240
static char gbuffer[BUFFER_MAX_LEN] = {0};

struct block {
    int sz;
    char * ptr;
    char * buffer;
};

static struct block gblock;

int
lpackinit(lua_State * L) {
    gblock.sz = 0;
    gblock.ptr = gbuffer;
    gblock.buffer = gbuffer;
    return 0;
}

int
lunpackinit(lua_State * L) {
    int sz = lua_tointeger(L,1);
    const char * buffer = lua_tostring(L,2);
    memcpy(gbuffer,buffer,sz);
    gblock.sz = sz;
    gblock.ptr = gbuffer;
    gblock.buffer = gbuffer;
    return 0;
}

static inline int
writeinteger(int64_t value,char *ptr,int bufferlen) {
    bool positive = true;
    if(value < 0) {
        positive = false;
        value = -value;
    }

    if((value & 0xFFFFFFFF00000000) > 0) {
        assert(8 == sizeof(int64_t));
        if(bufferlen < 9) {
            return 0;
        }

        *ptr = positive ? T4 : t4;
        *((int64_t*)(ptr + 1)) = value;
        return 9;
    } else if((value & 0xFFFF0000) > 0) {
        assert(4 == sizeof(int32_t));
        if(bufferlen < 5) {
            return 0;
        }

        *ptr = positive ? T3 : t3;
        *((int32_t*)(ptr + 1)) = value;
        return 5;
    } else if((value & 0xFF00) > 0) {
        assert(2 == sizeof(int16_t));
        if(bufferlen < 3) {
            return 0;
        }

        *ptr = positive ? T2 : t2;
        *((int16_t*)(ptr + 1)) = value;
        return 3;
    } else {
        assert(1 == sizeof(int8_t));
        if(bufferlen < 2) {
            return 0;
        }

        *ptr = positive ? T1 : t1;
        *((int8_t*)(ptr + 1)) = value;
        return 2;
    }
    return 0;
}

static inline int
writestring(const char* value,char *ptr,int bufferlen) {
    int slen = (int)strlen(value);
    int ret = writeinteger(slen,ptr,bufferlen);

    if(ret > 0) {
        if(bufferlen < ret+slen) {
            return 0;
        }
        memcpy(ptr+ret,value,slen);
        int tlen = ret+slen;
        return tlen;
    }

    return 0;
}

int
lwrite(lua_State * L) {
    int tp = lua_type(L,1);
    int bufferlen = BUFFER_MAX_LEN-gblock.sz;
    int wsz = 0;
    if (tp==LUA_TNUMBER) {
        long value = lua_tonumber(L,1);
        wsz = writeinteger(value,gblock.ptr,bufferlen);
    } else if (tp==LUA_TSTRING) {
        const char * value = lua_tostring(L,1);
        wsz = writestring(value,gblock.ptr,bufferlen);
    } else {
        fprintf(stderr,"error:unknow type %s\n",lua_typename(L,lua_type(L,1)));
    }
    gblock.ptr += wsz;
    gblock.sz += wsz;
    lua_pushnumber(L,gblock.sz);
    return 1;
}

static int
readinteger(const char * ptr, int bufferlen, int64_t *value) {
    if(bufferlen <= 1) {
        return 0;
    }
    int tlen = 1;
    char T = *ptr;
    bool positive = ((T & 0xF0) == 0);
    bool isInt64 = T & 0x08;
    bool isInt32 = T & 0x04;
    bool isInt16 = T & 0x02;
    bool isInt8  = T & 0x01;
    //printf("bool:%d,%d,%d,%d\n",isInt64,isInt32,isInt16,isInt8);
    if (isInt64) {
        assert(sizeof(int64_t)==8);
        if ((bufferlen - tlen) < sizeof(int64_t)) {
            return -1;
        }

        int64_t v64 = *((int64_t*)(ptr + tlen));
        *value = positive ? v64 : -v64;
        tlen += sizeof(int64_t);

        if( isInt32 || isInt16 || isInt8) {
            return 0;
        }
        return tlen;
    } else if (isInt32) {
        assert(sizeof(int32_t)==4);
        if ((bufferlen - tlen) < sizeof(int32_t)) {
            return -1;
        }

        int32_t v32 = *((int32_t*)(ptr + tlen));
        *value = positive ? v32 : -v32;
        tlen += sizeof(int32_t);

        if( isInt64 || isInt16 || isInt8) {
            return 0;
        }
        return tlen;
    } else if (isInt16) {
        assert(sizeof(int16_t)==2);
        if ((bufferlen - tlen) < sizeof(int16_t)) {
            return -1;
        }

        int16_t v16 = *((int16_t*)(ptr + tlen));
        *value = positive ? v16 : -v16;
        tlen += sizeof(int16_t);

        if( isInt64 || isInt32 || isInt8) {
            return 0;
        }
        return tlen;
    } else if (isInt8) {
        assert(sizeof(int8_t)==1);
        if ((bufferlen - tlen) < sizeof(int8_t)) {
            return -1;
        }

        int8_t v8 = *((int8_t*)(ptr + tlen));
        *value = positive ? v8 : -v8;
        tlen += sizeof(int8_t);

        if( isInt64 || isInt32 || isInt16) {
            return 0;
        }
        return tlen;
    }
    return 0;
}

void FGLuaStackDump(lua_State* pL){
	int i;
	int top = lua_gettop(pL);
	for(i=1;i<=top;i++){
		int t = lua_type(pL, i);
		printf("LUA堆栈位置[%d]的值:",i);
		switch (t) {
			case LUA_TSTRING:
				printf("'%s'",lua_tostring(pL, i));
				break;
			case LUA_TBOOLEAN:
				printf(lua_toboolean(pL, i)?"true":"false");
				break;
			case LUA_TNUMBER:
				printf("%g",lua_tonumber(pL, i));
				break;
			default:
				printf("%s",lua_typename(pL, t));
				break;
		}
		printf("\n");
	}
}
int
lread(lua_State * L) {
    const char * tp = lua_tostring(L,1);
    lua_pop(L,1);
    int bufferlen = gblock.sz;
    int ssz = 0;
    if (strcmp(tp,"number")==0) {
        int64_t value = 0;
        ssz = readinteger(gblock.ptr,bufferlen,&value);
        lua_pushnumber(L,value);
    } else if (strcmp(tp,"string")==0) {
        int64_t len = 0;
        int sz = readinteger(gblock.ptr,bufferlen,&len);
        lua_pushlstring(L,gblock.ptr+sz,len);
        ssz = sz + len;
    } else {
        fprintf(stderr,"error:unknow type %s\n",tp);
    }
    gblock.ptr += ssz;
    gblock.sz -= ssz;
    lua_pushnumber(L,gblock.sz);
    lua_insert(L,1);
    //printf("read:%d\n",gblock.sz);
    return 2;
}

int
lgetpack(lua_State * L) {
    lua_pushnumber(L,gblock.sz);
    lua_pushlstring(L,gblock.buffer,gblock.sz);
    return 2;
}

int luaopen_luaprot(lua_State *L) {
    luaL_checkversion(L);
    luaL_Reg l[] ={
        { "packinit", lpackinit },
        { "write", lwrite },
        { "getpack", lgetpack },
        { "unpackinit", lunpackinit },
        { "read", lread },
        { NULL, NULL },
    };

    luaL_newlib(L,l);
    return 1;
}

