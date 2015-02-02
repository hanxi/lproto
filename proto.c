#include "proto.h"

#include "log.h"
#include "buffer.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define TINTEGER 1
#define TSTRING 2
#define TFLOAT 3
#define TARRAY 4
#define TTABLE 5


struct field_list;

struct field {
    struct field *next;
    struct field_list *child;
    char type;
    char key[DEFAULT_STR_LEN];
    union {
        char str_value[DEFAULT_STR_LEN];
        int int_value;
    } default_value;
};

struct field_list {
    struct field *head;
    struct field *tail;
    int len;
};

struct proto {
    struct field *root;
    struct buffer *buf;
};

struct field * field_new()
{
    struct field *node = (struct field *)malloc(sizeof(struct field));
    node->next = NULL;
    node->child = NULL;
    return node;
}

void field_delete(struct field **pnode)
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

static int field_cmp(struct field *node1, struct field *node2)
{
    int ret = strcmp(node1->key,node2->key);
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

static int load_proto(lua_State *L, struct field *node)
{
    switch (lua_type(L, -1)) {
    case LUA_TTABLE:
        lua_pushnil(L);
        if (lua_next(L, -2)) {
            node->child = field_list_new();
            if (lua_isnumber(L ,-2)) { // array
                node->type = TARRAY;
                struct field *tmp_node = field_new();
                strncpy(tmp_node->key, "1", sizeof(tmp_node->key));
                load_proto(L, tmp_node);
                field_list_insert(node->child, tmp_node);
                lua_pop(L, 2);
            } else { // table
                node->type = TTABLE;
                lua_pop(L, 2);
                lua_pushnil(L);
                while (lua_next(L, -2)) {
                    struct field *tmp_node = field_new();
                    strncpy(tmp_node->key, lua_tostring(L,-2), sizeof(tmp_node->key));
                    load_proto(L, tmp_node);
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
        log_debug("    ");
    }
}

static void print_field(struct field *node, int n)
{
    if (!node) {
        return;
    }
    print_space(n);
    if (strlen(node->key)>0) {
        log_debug("['%s'] = ",node->key);
    } else {
        log_debug("[1] = ");
    }
    switch (node->type) {
    case TINTEGER:
        log_debug("%d,\n",node->default_value.int_value);
        break;
    case TFLOAT:
        log_debug("%s,\n",node->default_value.str_value);
        break;
    case TSTRING:
        log_debug("'%s',\n",node->default_value.str_value);
        break;
    case TTABLE:
    case TARRAY:
        log_debug("{\n");
        struct field * tmp_node = NULL;
        for (tmp_node=node->child->head; tmp_node!=NULL; tmp_node=tmp_node->next) {
            print_field(tmp_node, n+1);
        }
        print_space(n);
        log_debug("},\n");
        break;
    }
}

static int field_serialize(lua_State *L, struct field *node, struct buffer *buf)
{
    if (!node) {
        log_error("bug in serialize_proto, why node is NULL\n");
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
            int sz = luaL_len(L,-1);
            buffer_push_back_integer(buf, sz);

            struct field *tmp_node = node->child->head;
            int i = 0;
            for (i=0; i<sz; i++) {
                lua_rawgeti(L, -1, i+1);
                field_serialize(L, tmp_node, buf);
                lua_pop(L, 1);
            }
        } else {
            log_error("bug in TARRAY, why haven't child\n");
        }
        break;
    case TTABLE:
        if (node->child) {
            struct field *tmp_node = node->child->head;
            while (tmp_node) {
                lua_getfield(L, -1, tmp_node->key);
                field_serialize(L, tmp_node, buf);
                lua_pop(L, 1);
                tmp_node = tmp_node->next;
            }
        } else {
            log_error("bug in TTABLE, why haven't child\n");
        }
        break;
    default:
        log_error("bug in serialize_proto, node->type unknow\n");
        break;
    }
    int after_len = buffer_get_length(buf);
    return after_len-before_len;
}

static int field_unserialize(lua_State *L, struct field *node, struct buffer *buf)
{
    int before_len = buffer_get_length(buf);
    if (!node) {
        log_error("bug in unserialize_proto\n");
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
                field_unserialize(L, tmp_node, buf);
                lua_rawseti(L, -2, i+1);
            }
        }
        break;
    case TTABLE:
        if (node->child) {
            struct field *tmp_node = node->child->head;
            lua_createtable(L,0,node->child->len);
            while (tmp_node) {
                field_unserialize(L, tmp_node, buf);
                lua_setfield(L, -2, tmp_node->key);
                tmp_node = tmp_node->next;
            }
        }
        break;
    default:
        log_error("bug in unserialize_proto, node->type unknow\n");
        break;
    }
    int after_len = buffer_get_length(buf);
    return before_len-after_len;
}

struct proto * proto_new(lua_State *L, const char *proto_name)
{
    struct proto *p = (struct proto*)malloc(sizeof(struct proto));
    p->root = field_new();
    strncpy(p->root->key, proto_name, sizeof(p->root->key) - 1);
    load_proto(L, p->root);
    p->buf = buffer_new();
    return p;
}

void proto_print_struct(struct proto *p)
{
    print_field(p->root, 0);
}

void proto_dump_buffer(struct proto *p)
{
    buffer_dump(p->buf, p->root->key);
}

void proto_delete(struct proto **pp)
{
    field_delete(&(*pp)->root);
    buffer_delete(&(*pp)->buf);
    *pp = NULL;
}

int proto_serialize(lua_State *L, struct proto *p)
{
    return field_serialize(L, p->root, p->buf);
}

int proto_unserialize(lua_State *L, struct proto *p)
{
    return field_unserialize(L, p->root, p->buf);
}

