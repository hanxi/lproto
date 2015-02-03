#include "proto.h"

#include "log.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>

#define TINTEGER 1
#define TSTRING 2
#define TFLOAT 3
#define TARRAY 4
#define TTABLE 5

#define ARRAY_KEY "1"

#define _T  (0x00) // positive
#define _t  (0xF0) // negative
#define _T1 (0x01) // 8bit
#define _T2 (0x02) // 16bit
#define _T3 (0x03) // 32bit
#define _T4 (0x04) // 64bit

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
    //struct buffer *buf;
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
                strncpy(tmp_node->key, ARRAY_KEY, sizeof(tmp_node->key));
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
    if (strcmp(node->key,ARRAY_KEY)!=0) {
        log_debug("['%s'] = ",node->key);
    } else {
        log_debug("[%s] = ",node->key);
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
        if (n==0) {
            log_debug("}\n");
        } else {
            log_debug("},\n");
        }
        break;
    }
}

static int buffer_pack_integer(uint8_t *buf, int sz, int64_t value)
{
    uint8_t sign = _T;
    if (value < 0) {
        sign = _t;
        value = -value;
    }

    int size = 0;
    if ((value & 0xFFFFFFFF00000000) > 0) {
        size = 9;
        if (sz < size) {
            return 0;
        }
        buf[0] = sign | _T4;
        buf[1] = (value >> 56) & 0xFF;
        buf[2] = (value >> 48) & 0xFF;
        buf[3] = (value >> 40) & 0xFF;
        buf[4] = (value >> 32) & 0xFF;
        buf[5] = (value >> 24) & 0xFF;
        buf[6] = (value >> 16) & 0xFF;
        buf[7] = (value >> 8 ) & 0xFF;
        buf[8] = value & 0xFF;
    } else if ((value & 0xFFFF0000) > 0) {
        size = 5;
        if (sz < size) {
            return 0;
        }
        buf[0] = sign | _T3;
        buf[1] = (value >> 24) & 0xFF;
        buf[2] = (value >> 16) & 0xFF;
        buf[3] = (value >> 8) & 0xFF;
        buf[4] = value & 0xFF;
    } else if ((value & 0xFF00) > 0) {
        size = 3;
        if (sz < size) {
            return 0;
        }
        buf[0] = sign | _T2;
        buf[1] = (value >> 8) & 0xFF;
        buf[2] = value & 0xFF;
    } else {
        size = 2;
        if (sz < size) {
            return 0;
        }
        buf[0] = sign | _T1;
        buf[1] = value & 0xFF;
    }
    return size;
}

static int buffer_unpack_integer(const uint8_t *buf, int sz, int64_t *value)
{
    if (sz < 2) {
        return 0;
    }

    int size = 0;

    uint8_t sign = buf[0];
    bool positive = ((sign & 0xF0)==0);
    uint8_t flag = sign & 0x0F;

    switch (flag) {
    case _T1:
        size = 2;
        if (sz < size) {
            return 0;
        }
        *value = (int64_t)buf[1];
        break;
    case _T2:
        size = 3;
        if (sz < size) {
            return 0;
        }
        *value = (int64_t)buf[1] << 8
                 | (int64_t)buf[2];
        break;
    case _T3:
        size = 5;
        if (sz < size) {
            return 0;
        }
        *value = (int64_t)buf[1] << 24
                 | (int64_t)buf[2] << 16
                 | (int64_t)buf[3] << 8
                 | (int64_t)buf[4];
        break;
    case _T4:
        size = 9;
        if (sz < size) {
            return 0;
        }
        *value = (int64_t)buf[1] << 56
                 | (int64_t)buf[2] << 48
                 | (int64_t)buf[3] << 40
                 | (int64_t)buf[4] << 32
                 | (int64_t)buf[5] << 24
                 | (int64_t)buf[6] << 16
                 | (int64_t)buf[7] << 8
                 | (int64_t)buf[8];
        break;
    default:
        size = 0;
        log_error("unknow type integer\n");
        break;
    }
    *value = positive ? *value : -*value;
    return size;
}

static int buffer_pack_data(uint8_t *buf, int sz, const void *data, int dsz)
{
    int size = buffer_pack_integer(buf, sz, dsz);
    if (size == 0) {
        return 0;
    }
    buf += size;
    size += dsz;
    if (sz < size) {
        return 0;
    }
    memcpy(buf, data, dsz);
    return size;
}

static int buffer_unpack_data(const uint8_t *buf, int sz, const void **data, int *dsz)
{
    int64_t len = 0;
    int size = buffer_unpack_integer(buf, sz, &len);
    if (size == 0) {
        return 0;
    }
    buf += size;
    size += len;
    if (sz < size) {
        return 0;
    }
    *dsz = len;
    *data = buf;
    return size;
}

static int field_serialize(lua_State *L, struct field *node, uint8_t *buf, int size)
{
    int before_size = size;
    if (!node) {
        log_error("erro in serialize_proto, why node is NULL.\n");
        return 0;
    }
    if (size <= 0) {
        log_error("erro in serialize_proto, buffer no length.\n");
        return -1;
    }
    int tp = lua_type(L, -1);
    switch (node->type) {
    case TINTEGER: {
        lua_Integer v = node->default_value.int_value;
        if (tp==LUA_TNUMBER || tp==LUA_TSTRING) {
            v = lua_tointeger(L, -1);
        }
        size -= buffer_pack_integer(buf, size, v);
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
        size -= buffer_pack_data(buf, size, v, len);
    }
    break;
    case TARRAY:
        if (node->child) {
            int len = luaL_len(L, -1);
            int sz = buffer_pack_integer(buf, size, len);
            if (sz == 0) {
                return -1;
            }
            size -= sz;
            buf += sz;

            struct field *tmp_node = node->child->head;
            int i = 0;
            for (i=0; i<len; i++) {
                lua_rawgeti(L, -1, i+1);
                int sz = field_serialize(L, tmp_node, buf, size);
                lua_pop(L, 1);
                if (sz <= 0) {
                    return -1;
                }
                size -= sz;
                buf += sz;
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
                int sz = field_serialize(L, tmp_node, buf, size);
                lua_pop(L, 1);
                if (sz <= 0) {
                    return -1;
                }
                size -= sz;
                buf += sz;
                tmp_node = tmp_node->next;
            }
        } else {
            log_error("error in TTABLE, why haven't child\n");
        }
        break;
    default:
        log_error("bug in serialize_proto, node->type unknow\n");
        break;
    }
    return before_size - size;
}

static int field_unserialize(lua_State *L, struct field *node, const uint8_t *buf, int size)
{
    int before_size = size;
    if (!node) {
        log_error("error in unserialize_proto\n");
        return 0;
    }
    if (size <= 0) {
        log_error("error in unserialize_proto. buffer not complete.\n");
        return -1;
    }
    switch (node->type) {
    case TINTEGER: {
        int64_t v = 0;
        size -= buffer_unpack_integer(buf, size, &v);
        lua_pushinteger(L, v);
    }
    break;
    case TFLOAT: {
        int len = 0;
        const void *data = NULL;
        size -= buffer_unpack_data(buf, size, &data, &len);
        lua_pushlstring(L, data, len);
        lua_tonumber(L, -1);
    }
    break;
    case TSTRING: {
        int len = 0;
        const void *data = NULL;
        size -= buffer_unpack_data(buf, size, &data, &len);
        lua_pushlstring(L, data, len);
    }
    break;
    case TARRAY:
        if (node->child) {
            struct field *tmp_node = node->child->head;
            int64_t len = 0;
            int sz = buffer_unpack_integer(buf, size, &len);
            if (sz == 0) {
                lua_pushnil(L);
                return -1;
            }
            size -= sz;
            buf += sz;

            lua_createtable(L,len,0);
            int i = 0;
            for (i=0; i<len; i++) {
                int sz = field_unserialize(L, tmp_node, buf, size);
                lua_rawseti(L, -2, i+1);
                if (sz <= 0) {
                    log_error("TARRAY no length.(size=%d)\n",size);
                    return -1;
                }
                size -= sz;
                buf += sz;
            }
        }
        break;
    case TTABLE:
        if (node->child) {
            struct field *tmp_node = node->child->head;
            lua_createtable(L,0,node->child->len);
            while (tmp_node) {
                int sz = field_unserialize(L, tmp_node, buf, size);
                lua_setfield(L, -2, tmp_node->key);
                tmp_node = tmp_node->next;
                if (sz <= 0) {
                    log_error("TTABLE no length.\n");
                    return -1;
                }
                size -= sz;
                buf += sz;
            }
        }
        break;
    default:
        log_error("bug in unserialize_proto, node->type unknow\n");
        break;
    }
    return before_size-size;
}

struct proto * proto_new(lua_State *L, const char *proto_name)
{
    struct proto *p = (struct proto*)malloc(sizeof(struct proto));
    p->root = field_new();
    strncpy(p->root->key, proto_name, sizeof(p->root->key) - 1);
    load_proto(L, p->root);
    return p;
}

void proto_print_struct(struct proto *p)
{
    print_field(p->root, 0);
}

void proto_delete(struct proto **pp)
{
    field_delete(&(*pp)->root);
    *pp = NULL;
}

int proto_serialize(lua_State *L, struct proto *p, void *buf, int sz)
{
    return field_serialize(L, p->root, buf, sz);
}

int proto_unserialize(lua_State *L, struct proto *p, const void *buf, int sz)
{
    return field_unserialize(L, p->root, buf, sz);
}

