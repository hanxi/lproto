#include "../log.h"
#include "../proto.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <assert.h>
#include <stdlib.h>

static void lua_dump_stack(lua_State *L)
{
    int top = lua_gettop(L);
    int i=1;
    log_debug("--------------------------------\n");
    for(i = 1; i <= top; i++) {
        int t = lua_type(L,i);
        log_debug("Lua stack [%d] is value : ",i);
        switch(t) {
        case LUA_TSTRING:
            log_debug("'%s'",lua_tostring(L, i));
            break;
        case LUA_TBOOLEAN:
            log_debug(lua_toboolean(L, i)?"true":"false");
            break;
        case LUA_TNUMBER:
            log_debug("%g",lua_tonumber(L, i));
            break;
        default:
            log_debug("%s",lua_typename(L, t));
            break;
        }
        log_debug("\n");
    }
    log_debug("--------------------------------\n");
}

static const char *lua_str = "\
print('hello lua')\
prot = { \
    str = 'hanxi',\
    int = 0,\
    float = 0.1,\
    iarray = {0,},\
    farray = {1.1},\
    sarray = {'sa'},\
    tbl_array = {\
        {x=1,y='tb',},\
    },\
    tbl = {\
        a = 1,\
        d = 4,\
        b = 2,\
        c = 3,\
        tbl_sarray = {'abcd',},\
        tbl_iarray = {1},\
        tbl_farray = {1.5},\
        tbl_tblarray = {xx=1.5,yy='bbb',},\
    },\
}\
";

void test_load_table()
{
    lua_State *L = luaL_newstate();
    luaL_checkversion(L);
    luaL_openlibs(L);   // link lua lib
    int err = luaL_loadstring(L, lua_str);
    assert(err == LUA_OK);
    err = lua_pcall(L, 0, 0, 0);
    if(err) {
        log_error("%s\n",lua_tostring(L,-1));
        lua_close(L);
        exit(1);
    }
    lua_getglobal(L,"prot");
    struct proto *prot = proto_new(L, "proto");
    lua_dump_stack(L);

    log_debug("==================================\n");
    proto_print_struct(prot);
    log_debug("==================================\n");

    int sz = 76;
    char *buf = (char *)malloc(sz);
    int l = proto_serialize(L, prot, buf, sz);
    log_debug("l1=%d,sz=%d\n",l,sz);

    l = proto_unserialize(L, prot, buf, sz);
    log_debug("l2=%d\n",l);

    free(buf);
    proto_delete(&prot);

    lua_dump_stack(L);
}

int main()
{
    test_load_table();
    return 0;
}

