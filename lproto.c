#include "log.h"
#include "proto.h"

#include <lua.h>
#include <lauxlib.h>

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
static int ldumpproto(lua_State *L)
{
    struct proto *p = lua_touserdata(L, 1);
    if (p == NULL) {
        return luaL_argerror(L, 1, "Need a proto object");
    }
    proto_dump_buffer(p);
    return 0;
}

/*
 * @param: proto lightuserdata
 * @return: length int
 */
static int lgetlength(lua_State *L)
{
    struct proto *p = lua_touserdata(L, 1);
    if (p == NULL) {
        return luaL_argerror(L, 1, "Need a proto object");
    }
    int len = proto_get_buffer_length(p);
    lua_pushinteger(L, len);
    return 1;
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

/*
 * @param: proto lightuserdata
 * @param: table
 */
static int lencode(lua_State *L)
{
    struct proto *p = lua_touserdata(L, 1);
    if (p == NULL) {
        return luaL_argerror(L, 1, "Need a proto object");
    }
    proto_serialize(L, p);
    return 0;
}

/*
 * @param: proto lightuserdata
 * @return: table
 */
static int ldecode(lua_State *L)
{
    struct proto *p = lua_touserdata(L, 1);
    if (p == NULL) {
        return luaL_argerror(L, 1, "Need a proto object");
    }
    proto_unserialize(L, p);
    return 1;
}

int luaopen_lproto_core(lua_State *L)
{
    luaL_checkversion(L);
    luaL_Reg l[] = {
        { "newproto", lnewproto },
        { "deleteproto", ldeleteproto },
        { "dumpproto", ldumpproto },
        { "getlength", lgetlength },
        { "printstruct", lprintstruct},
        { "encode", lencode },
        { "decode", ldecode },
        { NULL, NULL },
    };
    luaL_newlib(L, l);
    return 1;
}

