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
    }default_value;
};

struct field_list {
    struct field *head;
    struct field *tail;
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

// sort
static void field_list_insert(struct field_list *fl, struct field *node)
{
    if (fl->head == NULL) {
        fl->head = fl->tail = node;
    } else {
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
        case LUA_TNUMBER:{
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

int serialize_proto(lua_State *L, struct field_list *fl, struct buffer *buf)
{
    struct field *node = fl->head;
    while (node) {
        lua_getfield(L, -1, node->key);
        switch (node->type) {
        case TINTEGER:
            printf("[%s] = %d\n",node->key,node->default_value.int_value);
            break;
        case TFLOAT:
            printf("[%s] = %s\n",node->key,node->default_value.str_value);
            break;
        case TSTRING:
            printf("[%s] = '%s'\n",node->key,node->default_value.str_value);
            break;
        case TTABLE:
            break;
        }
        node = node->next;
    }
    return 0;
}

int unserialize_proto()
{
    return 0;
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
    //lua_dump_stack(L);
    struct field *node = field_new();
    load_proto(L, node, "prot");
    print_field(node,0);
    printf("==================================\n");
    field_delete(&node);
    print_field(node,0);
}

int main()
{
    test_load_table();
    return 0;
}

