#include "proto.h"

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
void print_field(struct field_list *fl, int n);

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

static struct field * field_new()
{
    struct field *node = (struct field *)malloc(sizeof(struct field));
    node->next = NULL;
    node->child = NULL;
    return node;
}

static void field_list_delete(struct field_list **fl)
{
    struct field *node = (*fl)->head;
    while (node)  {
        if (node->child != NULL) {
            field_list_delete(&node->child);
        }

        struct field *tmp_node = node;
        node = node->next;
        free(tmp_node);
    }
    *fl = NULL;
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
    printf("field_cmp:key1=%s,key2=%s,ret=%d\n",node1->key,node2->key,ret);
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

static int load_table(lua_State *L, struct field_list *fl)
{
    lua_pushnil(L);
    //lua_dump_stack(L);
    while (lua_next(L, -2)) {
        //lua_dump_stack(L);
        struct field *node = field_new();
        strncpy(node->key, lua_tostring(L,-2), sizeof(node->key)-1);
        int type = lua_type(L, -1);
        switch (type) {
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
        case LUA_TTABLE:
            lua_pushnil(L);
            if (lua_next(L, -2)) {
                if (lua_isnumber(L, -2)) {
                    node->type = TARRAY;
                    node->child = field_list_new();
                    struct field *tmp_node = field_new();
                    switch (lua_type(L, -1)) {
                    case LUA_TSTRING:
                        tmp_node->type = TSTRING;
                        strncpy(tmp_node->default_value.str_value, lua_tostring(L,-1),
                                sizeof(tmp_node->default_value.str_value) - 1);
                        break;
                    case LUA_TNUMBER:{
                        lua_Number v = lua_tonumber(L, -1);
                        if (v==(lua_Integer)v) {
                            tmp_node->type = TINTEGER;
                            tmp_node->default_value.int_value = v;
                        } else {
                            tmp_node->type = TFLOAT;
                            strncpy(tmp_node->default_value.str_value, lua_tostring(L,-1),
                                    sizeof(tmp_node->default_value.str_value)-1);
                        }
                        }
                        break;
                    case LUA_TTABLE: {
                        tmp_node->type = TTABLE;
                        tmp_node->child = field_list_new();
                        int ret = load_table(L,tmp_node->child);
                        if (ret) {
                            return ret;
                        }
                    }
                        break;
                    default:
                        fprintf(stderr,"load_table array element unknow field type : type=%s",lua_typename(L,type));
                        return -1;
                    }
                    field_list_insert(node->child,tmp_node);
                    lua_pop(L,2);
                } else {
                    node->type = TTABLE;
                    node->child = field_list_new();
                    lua_pop(L,2);
                    int ret = load_table(L,node->child);
                    if (ret) {
                        return ret;
                    }
                }
            } else {
                fprintf(stderr,"load_table null table : key=%s",lua_tostring(L,-2));
                return -1;
            }
            break;
        }
        field_list_insert(fl, node);
        lua_pop(L, 1);
    }
    return 0;
}

static void print_space(int n)
{
    int i=0;
    for (i=0; i<n; i++) {
        printf("----");
    }
}

void print_field(struct field_list *fl, int n)
{
    if (!fl) {
        return;
    }
    struct field *node = NULL;
    for(node = fl->head; node != NULL; node = node->next) {
        print_space(n);
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
            printf("[%s] = {\n",node->key);
            print_field(node->child, n+1);
            print_space(n);
            printf("}\n");
            break;
        case TARRAY:
            {
            struct field *tmp_node = node->child->head;
            switch (tmp_node->type) {
            case TINTEGER:
                printf("[%s] = { (array)(interger)\n",node->key);
                print_space(n+1);
                printf("[1] = %d\n",tmp_node->default_value.int_value);
                print_space(n);
                printf("}\n");
                break;
            case TFLOAT:
                printf("[%s] = { (array)(float)\n",node->key);
                print_space(n+1);
                printf("[1] = %s\n",tmp_node->default_value.str_value);
                print_space(n);
                printf("}\n");
                break;
            case TSTRING:
                printf("[%s] = { (array)(string)\n",node->key);
                print_space(n+1);
                printf("[1] = '%s'\n",tmp_node->default_value.str_value);
                print_space(n);
                printf("}\n");
                break;
            case TTABLE:
                printf("[%s] = { (array)(table)\n",node->key);
                print_space(n+1);
                printf("[1] = {\n");
                print_field(tmp_node->child,n+2);
                print_space(n+1);
                printf("}\n");
                print_space(n);
                printf("}\n");
                break;
            }
            }
            break;
        }
    }
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
    tbl = {\
        a = 1,\
        d = 4,\
        b = 2,\
        c = 3,\
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
    struct field_list *fl = field_list_new();
    load_table(L, fl);
    print_field(fl,0);
    printf("==================================\n");
    field_list_delete(&fl);
    print_field(fl,0);
}

int main()
{
    test_load_table();
    return 0;
}

