#include "proto.h"
#include "buffer.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define TINTEGER 1
#define TSTRING 2
#define TFLOAT 3
#define TARRAY 4
#define TTABLE 5

#define MAX_KEY_LEN 32
#define MAX_DEFALUT_STR_LEN 32

struct field_list;

struct field {
    struct field *next;
    struct field_list *child;
    char type;
    char key[MAX_KEY_LEN];
    union {
        char str_value[MAX_DEFALUT_STR_LEN];
        int int_value;
    } default_value;
};

struct field_list {
    struct field *head;
    struct field *tail;
    int len;
};

void print_field(struct field *node, int n);

static struct field * field_new()
{
    struct field *node = (struct field *)malloc(sizeof(struct field));
    node->next = NULL;
    node->child = NULL;
    return node;
}

static void field_delete(struct field **pnode)
{
    if ((*pnode)->child != NULL) {
        struct field *node = (*pnode)->child->head;
        while (node) {
            struct field *tmp_node = node->next;
            field_delete(&node);
            node = tmp_node;
        }
    }
    free(*pnode);
    *pnode = NULL;
}

static struct field_list * field_list_new()
{
    struct field_list *fl = (struct field_list *)malloc(sizeof(struct field_list));
    fl->head = NULL;
    fl->tail = NULL;
    fl->len = 0;
    return fl;
}

void lua_dump_stack(lua_State *L)
{
    int top = lua_gettop(L);
    int i=1;
    for(i = 1; i <= top; i++) {
        int t = lua_type(L,i);
        printf("LUA堆栈位置[%d]的值:",i);
        switch(t) {
        case LUA_TSTRING:
            printf("'%s'",lua_tostring(L, i));
            break;
        case LUA_TBOOLEAN:
            printf(lua_toboolean(L, i)?"true":"false");
            break;
        case LUA_TNUMBER:
            printf("%g",lua_tonumber(L, i));
            break;
        default:
            printf("%s",lua_typename(L, t));
            break;
        }
        printf("\n");
    }
    printf("--------------------------------\n");
}

static int field_cmp(struct field *node1, struct field *node2)
{
    int ret = strcmp(node1->key,node2->key);
    //printf("field_cmp:key1=%s,key2=%s,ret=%d\n",node1->key,node2->key,ret);
    return ret;
}

// insert and sort
static void field_list_insert(struct field_list *fl, struct field *node)
{
    if (fl->head == NULL) {
        fl->head = fl->tail = node;
        fl->len = 1;
    } else {
        fl->len++;
        struct field **pnode = &fl->head;
        while (*pnode) {
            struct field *tmp = *pnode;
            if (field_cmp(tmp,node)>0) {
                *pnode = node;
                node->next = tmp;
                return;
            }
            pnode = &tmp->next;
        }
        fl->tail->next = node;
        fl->tail = node;
    }
}

static int load_proto(lua_State *L, struct field *node, const char *key)
{
    strncpy(node->key, key, sizeof(node->key) - 1);
    switch (lua_type(L, -1)) {
    case LUA_TTABLE:
        lua_pushnil(L);
        if (lua_next(L, -2)) {
            node->child = field_list_new();
            if (lua_isnumber(L ,-2)) { // array
                node->type = TARRAY;
                struct field *tmp_node = field_new();
                load_proto(L, tmp_node, "");
                field_list_insert(node->child, tmp_node);
                lua_pop(L, 2);
            } else { // table
                node->type = TTABLE;
                lua_pop(L, 2);
                lua_pushnil(L);
                while (lua_next(L, -2)) {
                    struct field *tmp_node = field_new();
                    load_proto(L, tmp_node, lua_tostring(L, -2));
                    field_list_insert(node->child, tmp_node);
                    lua_pop(L, 1);
                }
            }
        }
        break;
    case LUA_TSTRING:
        node->type = TSTRING;
        strncpy(node->default_value.str_value, lua_tostring(L,-1),
                sizeof(node->key)-1);
        break;
    case LUA_TNUMBER: {
        lua_Number v = lua_tonumber(L, -1);
        if (v==(lua_Integer)v) {
            node->type = TINTEGER;
            node->default_value.int_value = v;
        } else {
            node->type = TFLOAT;
            strncpy(node->default_value.str_value, lua_tostring(L,-1),
                    sizeof(node->default_value.str_value)-1);
        }
    }
    break;
    }
    return 0;
}

static void print_space(int n)
{
    int i=0;
    for (i=0; i<n; i++) {
        printf("    ");
    }
}

void print_field(struct field *node, int n)
{
    if (!node) {
        return;
    }
    print_space(n);
    if (strlen(node->key)>0) {
        printf("['%s'] = ",node->key);
    } else {
        printf("[1] = ");
    }
    switch (node->type) {
    case TINTEGER:
        printf("%d,\n",node->default_value.int_value);
        break;
    case TFLOAT:
        printf("%s,\n",node->default_value.str_value);
        break;
    case TSTRING:
        printf("'%s',\n",node->default_value.str_value);
        break;
    case TTABLE:
    case TARRAY:
        printf("{\n");
        struct field * tmp_node = NULL;
        for (tmp_node=node->child->head; tmp_node!=NULL; tmp_node=tmp_node->next) {
            print_field(tmp_node, n+1);
        }
        print_space(n);
        printf("},\n");
        break;
    }
}

int serialize_proto(lua_State *L, struct field *node, struct buffer *buf)
{
    if (!node) {
        fprintf(stderr, "bug in serialize_proto, why node is NULL\n");
        return 0;
    }
    int before_len = buffer_get_length(buf);
    int tp = lua_type(L, -1);
    switch (node->type) {
    case TINTEGER: {
        lua_Integer v = node->default_value.int_value;
        if (tp==LUA_TNUMBER || tp==LUA_TSTRING) {
            v = lua_tointeger(L, -1);
        }
        buffer_push_back_integer(buf, v);
    }
    break;
    case TFLOAT:
    case TSTRING: {
        size_t len;
        const char *v = lua_tolstring(L, -1, &len);
        if (!v) {
            v = node->default_value.str_value;
            len = strlen(v);
        }
        buffer_push_back_integer(buf, len);
        buffer_push_back_data(buf, v, len);
    }
    break;
    case TARRAY:
        if (node->child) {
            int sz = lua_rawlen(L,-1);
            buffer_push_back_integer(buf, sz);

            struct field *tmp_node = node->child->head;
            int i = 0;
            for (i=0; i<sz; i++) {
                lua_rawgeti(L, -1, i+1);
                serialize_proto(L, tmp_node, buf);
                lua_pop(L, 1);
            }
        } else {
            fprintf(stderr, "bug in TARRAY, why haven't child\n");
        }
        break;
    case TTABLE:
        if (node->child) {
            struct field *tmp_node = node->child->head;
            while (tmp_node) {
                lua_rawgetp(L, -1, tmp_node->key);
                serialize_proto(L, tmp_node, buf);
                tmp_node = tmp_node->next;
                lua_pop(L, 1);
            }
        } else {
            fprintf(stderr, "bug in TTABLE, why haven't child\n");
        }
        break;
    default:
        fprintf(stderr, "bug in serialize_proto, node->type unknow\n");
        break;
    }
    int after_len = buffer_get_length(buf);
    return after_len-before_len;
}

int unserialize_proto(lua_State *L, struct field *node, struct buffer *buf)
{
    int before_len = buffer_get_length(buf);
    if (!node) {
        fprintf(stderr,"bug in unserialize_proto\n");
        return 0;
    }
    switch (node->type) {
    case TINTEGER: {
        int64_t v = 0;
        buffer_pop_front_integer(buf, &v);
        lua_pushinteger(L, v);
    }
    break;
    case TFLOAT: {
        int64_t len = 0;
        buffer_pop_front_integer(buf, &len);
        if (len <= buffer_get_length(buf)) {
            char *v = (char *)malloc(len);
            buffer_pop_front_data(buf, v, len);
            lua_pushlstring(L, v, len);
            lua_tonumber(L, -1);
            free(v);
        }
    }
    break;
    case TSTRING: {
        int64_t len = 0;
        buffer_pop_front_integer(buf, &len);
        if (len <= buffer_get_length(buf)) {
            char *v = (char *)malloc(len);
            buffer_pop_front_data(buf, v, len);
            lua_pushlstring(L, v, len);
            free(v);
        }
    }
    break;
    case TARRAY:
        if (node->child) {
            struct field *tmp_node = node->child->head;
            int64_t sz = 0;
            buffer_pop_front_integer(buf, &sz);
            lua_createtable(L,sz,0);
            int i = 0;
            for (i=0; i<sz; i++) {
                unserialize_proto(L, tmp_node, buf);
                lua_rawseti(L, -2, i+1);
            }
        }
        break;
    case TTABLE:
        if (node->child) {
            struct field *tmp_node = node->child->head;
            lua_createtable(L,0,node->child->len);
            while (tmp_node) {
                unserialize_proto(L, tmp_node, buf);
                lua_rawsetp(L, -2, tmp_node->key);
                tmp_node = tmp_node->next;
            }
        }
        break;
    default:
        fprintf(stderr, "bug in unserialize_proto, node->type unknow\n");
        break;
    }
    int after_len = buffer_get_length(buf);
    return before_len-after_len;
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
        fprintf(stderr,"%s\n",lua_tostring(L,-1));
        lua_close(L);
        exit(1);
    }
    lua_getglobal(L,"prot");
    struct field *node = field_new();
    lua_dump_stack(L);
    load_proto(L, node, "prot");
    print_field(node,0);
    printf("==================================\n");

    lua_dump_stack(L);
    struct buffer *buf = buffer_new();
    int l = serialize_proto(L, node, buf);
    buffer_dump(buf,"serialize_proto");
    printf("l1=%d\n",l);
    l = unserialize_proto(L, node, buf);
    buffer_dump(buf,"unserialize_proto");
    printf("l2=%d\n",l);

    buffer_delete(&buf);
    field_delete(&node);
    print_field(node,0);
}

int main()
{
    test_load_table();
    return 0;
}