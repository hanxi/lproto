#include "log.h"
#include "proto.h"
#include "ldef.h"

#define ENCODE_BUFFERSIZE 2050
#define ENCODE_MAXSIZE 0x1000000

/*
 * @tparam: protostruct table
 * @tparam: protoname string
 * @return: proto lightuserdata
 */
static int lnewproto(lua_State *L)
{
    if (lua_type(L, 1) != LUA_TTABLE) {
        return luaL_argerror(L, 1, "Need a proto table");
    }
    const char * proto_name = lua_tostring(L, 2);
    lua_pop(L, 1);
    struct proto *p = proto_new(L, proto_name);
    if (!p) {
        log_error("proto_new error.\n");
        return 0;
    }
    lua_pushlightuserdata(L, p);
    return 1;
}

/*
 * @param: proto lightuserdata
 */
static int ldeleteproto(lua_State *L)
{
    struct proto *p = lua_touserdata(L, 1);
    if (p == NULL) {
        return luaL_argerror(L, 1, "Need a proto object");
    }
    proto_delete(&p);
    return 0;
}

/*
 * @param: proto lightuserdata
 */
static int lprintstruct(lua_State *L)
{
    struct proto *p = lua_touserdata(L, 1);
    if (p == NULL) {
        return luaL_argerror(L, 1, "Need a proto object");
    }
    proto_print_struct(p);
    return 0;
}

static void * expand_buffer(lua_State *L, int osz, int isz)
{
    void *output;
    do {
        osz *=2;
    } while (osz < isz);
    if (osz > ENCODE_MAXSIZE) {
        luaL_error(L, "object is too large (>%d).", ENCODE_MAXSIZE);
        return NULL;
    }
    output = lua_newuserdata(L, osz);
    lua_replace(L, lua_upvalueindex(1));
    lua_pushinteger(L, osz);
    lua_replace(L, lua_upvalueindex(2));
    return output;
}

/*
 * @param: proto lightuserdata
 * @param: table
 */
static int lencode(lua_State *L)
{
    void *buffer = lua_touserdata(L, lua_upvalueindex(1));
    int sz = lua_tointeger(L, lua_upvalueindex(2));
    struct proto *p = lua_touserdata(L, 1);
    if (p == NULL) {
        return luaL_argerror(L, 1, "Need a proto object");
    }
    luaL_checktype(L, 2, LUA_TTABLE);
    for (;;) {
        int r = proto_serialize(L, p, buffer, sz);
        if (r<0) {
            buffer = expand_buffer(L, sz, sz*2);
            sz *= 2;
        } else {
            lua_pushlstring(L, buffer, r);
            return 1;
        }
    }
    return 0;
}

static const void * getbuffer(lua_State *L, int index, size_t *sz)
{
    const void *buffer = NULL;
    int t = lua_type(L, index);
    if (t == LUA_TSTRING) {
        buffer = lua_tolstring(L, index, sz);
    } else {
        if (t!=LUA_TUSERDATA && t!=LUA_TLIGHTUSERDATA) {
            luaL_argerror(L, 1, "Need a proto object");
            return NULL;
        }
        buffer = lua_touserdata(L, index);
        *sz = luaL_checkinteger(L, index+1);
    }
    return buffer;
}

/*
 * @param: proto lightuserdata
 * @param: string source / (lightuserdata, integer)
 * @return: table
 */
static int ldecode(lua_State *L)
{
    struct proto *p = lua_touserdata(L, 1);
    if (p == NULL) {
        return luaL_argerror(L, 1, "Need a proto object");
    }
    size_t sz = 0;
    const void *buffer = getbuffer(L, 2, &sz);
    int r = proto_unserialize(L, p, buffer, sz);
    if (r < 0) {
        return luaL_error(L, "decode error");
    }
    lua_pushinteger(L, r);
    return 2;
}

static void pushfunction_withbuffer(lua_State *L, const char *name, lua_CFunction func)
{
    lua_newuserdata(L, ENCODE_BUFFERSIZE);
    lua_pushinteger(L, ENCODE_BUFFERSIZE);
    lua_pushcclosure(L ,func, 2);
    lua_setfield(L, -2, name);
}

int luaopen_lproto_core(lua_State *L)
{
    luaL_checkversion(L);
    luaL_Reg l[] = {
        { "newproto", lnewproto },
        { "deleteproto", ldeleteproto },
        { "printstruct", lprintstruct},
        { "decode", ldecode },
        { NULL, NULL },
    };
    luaL_newlib(L, l);
    pushfunction_withbuffer(L, "encode", lencode);
    return 1;
}

