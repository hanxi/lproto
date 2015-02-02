#ifndef proto_h
#define proto_h

#include <lua.h>

#define DEFAULT_STR_LEN 64
struct proto;

// Create a new protocal, the lua stack must be a lua protocal.
struct proto * proto_new(lua_State *L, const char *proto_name);
void proto_delete(struct proto **pp);

// Print the protocal struct template.
void proto_print_struct(struct proto *p);
// Print the protocal buffer.
void proto_dump_buffer(struct proto *p);

// serialize proto
int proto_serialize(lua_State *L, struct proto *p);
// unserialize proto
int proto_unserialize(lua_State *L, struct proto *p);

#endif

