#ifndef ldef_h
#define ldef_h

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#ifndef luaL_checkversion

#define luaL_checkversion(L)
#define lua_rawlen lua_objlen

LUALIB_API void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup);

#define luaL_newlibtable(L,l) \
  lua_createtable(L, 0, sizeof(l)/sizeof((l)[0]) - 1)

#define luaL_newlib(L,l)  (luaL_newlibtable(L,l), luaL_setfuncs(L,l,0))

#endif

#endif

