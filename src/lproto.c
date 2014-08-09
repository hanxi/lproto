#include <lua.h>
#include <lauxlib.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>

#define MALLOC malloc
#define FREE free

#define BUFFER_MAX_LEN 10240

static char gwbuffer[BUFFER_MAX_LEN] = {0};
static char * grbuffer = NULL;

static const char T1 = 0x01; // 正,8位
static const char t1 = 0xF1; // 负,8位
static const char T2 = 0x02; // 正,16位
static const char t2 = 0xF2; // 负,16位
static const char T3 = 0x04; // 正,32位
static const char t3 = 0xF4; // 负,32位
static const char T4 = 0x08; // 正,64位
static const char t4 = 0xF8; // 夫,64位

int
lunpackinit(lua_State * L) {
    grbuffer = lua_touserdata(L,1);
    return 0;
}

static inline int
writeinteger(uint64_t value, uint8_t *ptr, int bufferlen) {
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
        ptr[1] = value & 0xff;
        ptr[2] = (value >> 8) & 0xff;
        ptr[3] = (value >> 16) & 0xff;
        ptr[4] = (value >> 24) & 0xff;
        ptr[5] = (value >> 32) & 0xff;
        ptr[6] = (value >> 40) & 0xff;
        ptr[7] = (value >> 48) & 0xff;
        ptr[8] = (value >> 56) & 0xff;
        return 9;
    } else if((value & 0xFFFF0000) > 0) {
        assert(4 == sizeof(int32_t));
        if(bufferlen < 5) {
            return 0;
        }

        *ptr = positive ? T3 : t3;
        ptr[1] = value & 0xff;
        ptr[2] = (value >> 8) & 0xff;
        ptr[3] = (value >> 16) & 0xff;
        ptr[4] = (value >> 24) & 0xff;
        return 5;
    } else if((value & 0xFF00) > 0) {
        assert(2 == sizeof(int16_t));
        if(bufferlen < 3) {
            return 0;
        }

        *ptr = positive ? T2 : t2;
        ptr[1] = value & 0xff;
        ptr[2] = (value >> 8) & 0xff;
        return 3;
    } else {
        assert(1 == sizeof(int8_t));
        if(bufferlen < 2) {
            return 0;
        }

        *ptr = positive ? T1 : t1;
        ptr[1] = value & 0xff;
        return 2;
    }
    return 0;
}

static inline int
writestring(const char* value,char *ptr,int bufferlen) {
    int slen = (int)strlen(value);
    int ret = writeinteger(slen,(uint8_t *)ptr,bufferlen);

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
    int offset = lua_tointeger(L,2);

    int bufferlen = BUFFER_MAX_LEN-offset;   // remain buffer length
    int wsz = 0;

    char * ptr = gwbuffer + offset;
    if (tp==LUA_TNUMBER) {
        uint64_t value = lua_tonumber(L,1);
        wsz = writeinteger(value,(uint8_t *)ptr,bufferlen);
    } else if (tp==LUA_TSTRING) {
        const char * value = lua_tostring(L,1);
        wsz = writestring(value,ptr,bufferlen);
    } else {
        fprintf(stderr,"error:unknow type %s\n",lua_typename(L,lua_type(L,1)));
    }
    lua_settop(L,0);

    lua_pushnumber(L,wsz);
    return 1;
}

static int
readinteger(const uint8_t * ptr, int bufferlen, int64_t *value) {
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
    if (isInt64) {
        assert(sizeof(int64_t)==8);
        if ((bufferlen - tlen) < sizeof(int64_t)) {
            return -1;
        }

        int64_t v64 = (int64_t)ptr[1]
                    | (int64_t)ptr[2] << 8
                    | (int64_t)ptr[3] << 16
                    | (int64_t)ptr[4] << 24
                    | (int64_t)ptr[5] << 32
                    | (int64_t)ptr[6] << 40
                    | (int64_t)ptr[7] << 48
                    | (int64_t)ptr[8] << 56;

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

        int32_t v32 = ptr[1] | ptr[2] << 8 | ptr[3] << 16 | ptr[4] << 24;
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

        int16_t v16 = ptr[1] | ptr[2] << 8;
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

        //int8_t v8 = *((int8_t*)(ptr + tlen));
        int8_t v8 = ptr[1];
        *value = positive ? v8 : -v8;
        tlen += sizeof(int8_t);

        if( isInt64 || isInt32 || isInt16) {
            return 0;
        }
        return tlen;
    }
    return 0;
}

void luaStackDump(lua_State* pL){
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

#define TINTEGER 1
#define TSTRING  2

int
lread(lua_State * L) {
    int tp = lua_tointeger(L,1);
    int offset = lua_tointeger(L,2);
    lua_settop(L,0);

    int bufferlen = offset;
    int rsz = 0;

    char * ptr = grbuffer + offset;
    if (tp == TINTEGER) {
        int64_t value = 0;
        rsz = readinteger((const uint8_t *)ptr,bufferlen,&value);
        lua_pushnumber(L,value);
    } else if (tp == TSTRING) {
        int64_t len = 0;
        int sz = readinteger((const uint8_t *)ptr,bufferlen,&len);
        lua_pushlstring(L,ptr+sz,len);
        rsz = sz + len;
    } else {
        fprintf(stderr,"error:unknow type %d\n",tp);
    }

    lua_pushnumber(L,rsz);
    lua_insert(L,1);
    return 2;
}

int
lnewpack(lua_State * L) {
    int sz = lua_tointeger(L,1);

    void * buffer = MALLOC(sz);
    memcpy(buffer,gwbuffer,sz);
    lua_pushlightuserdata(L,buffer);
    return 2;
}

int
ldeletepack(lua_State * L) {
    void * buffer = lua_touserdata(L,1);
    FREE(buffer);
    return 0;
}

static inline int
read_2byte(char * buffer, int pos) {
    int r = (int)buffer[pos] << 8 | (int)buffer[pos+1];
    return r;
}

static inline void
write_2byte(char * buffer, int r, int pos) {
    buffer[pos] = (r >> 8) & 0xff;
    buffer[pos+1] = r & 0xff;
}

int
lread_2byte(lua_State * L) {
    int offset = lua_tointeger(L,1);
    lua_settop(L,0);

    char * ptr = grbuffer + offset;
    int _2b = read_2byte(ptr, 0);
    lua_pushnumber(L,_2b);

    int rsz = 2;
    lua_pushnumber(L,rsz);
    lua_insert(L,1);
    return 2;
}

int
lwrite_2byte(lua_State * L) {
    int _2b = lua_tointeger(L,1);
    int offset = lua_tointeger(L,2);
    lua_settop(L,0);

    char * ptr = gwbuffer + offset;
    write_2byte(ptr, _2b, 0);

    int wsz = 2;
    lua_pushnumber(L,wsz);
    return 1;
}

int luaopen_lproto_c(lua_State *L) {
    luaL_checkversion(L);
    luaL_Reg l[] ={
        { "write_2byte", lwrite_2byte },
        { "write", lwrite },
        { "newpack", lnewpack },
        { "deletepack", ldeletepack },
        { "unpackinit", lunpackinit },
        { "read_2byte", lread_2byte },
        { "read", lread },
        { NULL, NULL },
    };

    luaL_newlib(L,l);
    return 1;
}

