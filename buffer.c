#include "buffer.h"

#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#define _T  (0x00) // positive
#define _t  (0xF0) // negative
#define _T1 (0x01) // 8bit
#define _T2 (0x02) // 16bit
#define _T3 (0x03) // 32bit
#define _T4 (0x04) // 64bit

static const unsigned MAX_TO_RELAIN_IN_EXPAND = 2048;

struct buffer_chain {
    struct buffer_chain *next;
    size_t offset;            // valid data start offset
    size_t length;            // valid data length
    size_t size;              // data size
    uint8_t *data;
};

struct buffer {
    struct buffer_chain *head;
    struct buffer_chain *tail;
    size_t length;
};

static struct buffer_chain * buffer_chain_new(size_t size)
{
    struct buffer_chain *bc = (struct buffer_chain *)malloc(sizeof(struct buffer_chain));
    bc->next = NULL;
    bc->offset = 0;
    bc->length = 0;
    bc->size = size;
    bc->data = (uint8_t *)malloc(size);
    return bc;
}

static void buffer_chain_delete(struct buffer_chain **pbc)
{
    free((*pbc)->data);
    free(*pbc);
    *pbc = NULL;
}

static void buffer_chain_align(struct buffer_chain *bc)
{
    if (bc->offset > 0) {
        memmove(bc->data, bc->data + bc->offset, bc->length);
        bc->offset = 0;
    }
}

static bool buffer_chain_should_realign(struct buffer_chain *bc, size_t size)
{
    return (bc->size - bc->length >= size) && (bc->length < bc->size/2)
           && (bc->length <= MAX_TO_RELAIN_IN_EXPAND);
}

static void buffer_clear(struct buffer *buf)
{
    buf->head = NULL;
    buf->tail = NULL;
    buf->length = 0;
}

struct buffer * buffer_new()
{
    struct buffer *buf = (struct buffer *)malloc(sizeof(struct buffer));
    buffer_clear(buf);
    return buf;
}

void buffer_delete(struct buffer **pbuf)
{
    struct buffer_chain *bc = (*pbuf)->head;
    while (bc) {
        struct buffer_chain *tmp = bc->next;
        buffer_chain_delete(&bc);
        bc = tmp;
    }
    free(*pbuf);
    *pbuf = NULL;
}

// Append data to the end of buffer.
int buffer_push_back_data(struct buffer *buf, const void *data, size_t size)
{
    struct buffer_chain *bc = buf->tail;
    if (bc == NULL) {
        bc = buffer_chain_new(size);
        if (!bc) {
            return -1;
        }
        buf->head = buf->tail = bc;
    }
    int remain = bc->size - bc->offset - bc->length;
    if (remain >= (int)size) {
        memcpy(bc->data + bc->offset + bc->length, data, size);
        bc->length += size;
        buf->length += size;
    } else if (buffer_chain_should_realign(bc, size)) {
        buffer_chain_align(bc);
        memcpy(bc->data + bc->length, data, size);
        bc->length += size;
        buf->length += size;
    } else {
        if (remain > 0) {
            memcpy(bc->data + bc->offset + bc->length, data, remain);
            bc->length +=  remain;
            buf->length += remain;
            data = (const uint8_t *)data + remain;
            size -= remain;
        }

        struct buffer_chain *tmp = buffer_chain_new(size);
        if (tmp == NULL) {
            return -1;
        }
        bc->next = tmp;
        buf->tail = tmp;
        memcpy(tmp->data, data, size);
        tmp->length += size;
        buf->length += size;
    }
    return 0;
}

// Move all data from in_buf into buffer.
int buffer_push_back_buffer(struct buffer *buf, struct buffer *in_buf)
{
    if (buf->head == NULL) {
        buf->head = buf->tail = in_buf->head;
        buf->length = in_buf->length;
    } else {
        buf->tail->next = in_buf->head;
        buf->length += in_buf->length;
    }
    buffer_clear(in_buf);
    return buf->length;
}

// Prepend data to the beginning of the buffer.
int buffer_push_front_data(struct buffer *buf, const void *data, size_t size)
{
    struct buffer_chain *bc = buf->head;
    if (bc == NULL) {
        bc = buffer_chain_new(size);
        if (!bc) {
            return -1;
        }
        buf->head = buf->tail = bc;
    }
    if (bc->length == 0) {
        bc->offset = bc->size;
    }
    if (bc->offset >= size) {
        memcpy(bc->data + bc->offset - size, data, size);
        bc->length += size;
        bc->offset -= size;
        buf->length += size;
        return 0;
    } else if (bc->offset) {
        memcpy(bc->data,(uint8_t *)data + size - bc->offset, bc->offset);
        bc->length += bc->offset;
        buf->length += bc->offset;
        size -= bc->offset;
        bc->offset = 0;
    }
    struct buffer_chain *tmp = buffer_chain_new(size);
    if (!tmp) {
        return -1;
    }
    buf->head = tmp;
    tmp->next = bc;
    tmp->length = size;
    tmp->offset = tmp->size - size;
    memcpy(tmp->data + tmp->offset, data, size);
    buf->length += size;
    return 0;
}

// Move all data from the in_buf to the beginning of the buffer.
int buffer_push_front_buffer(struct buffer *buf, struct buffer *in_buf)
{
    if (buf->head == NULL) {
        buf->head = buf->tail = in_buf->head;
        buf->length = in_buf->length;
    } else if (in_buf->head != NULL) {
        in_buf->tail->next = buf->head;
        buf->head = in_buf->head;
        buf->length += in_buf->length;
    }
    buffer_clear(in_buf);
    return buf->length;
}

// Read data from buffer and delete the bytes read.
int buffer_pop_front_data(struct buffer *buf, void *out_data, size_t size)
{
    if (size>buf->length) {
        size = buf->length;
    }
    int cpsize = size;
    struct buffer_chain *bc = buf->head;
    while (cpsize>0 && bc) {
        size_t cpsize1 = cpsize;
        if (cpsize1 > bc->length) {
            cpsize1 = bc->length;
        }
        out_data = (uint8_t *)out_data + size - cpsize;
        memcpy(out_data, bc->data + bc->offset, cpsize1);
        cpsize -= cpsize1;
        if (cpsize>0) {
            struct buffer_chain *tmp = bc->next;
            buffer_chain_delete(&bc);
            bc = tmp;
        } else {
            bc->offset += cpsize1;
            bc->length -= cpsize1;
        }
        buf->head = bc;
        buf->length -= cpsize1;
    }
    return size;
}

// Move data from beginning of buffer to out_buf.
int buffer_pop_front_buffer(struct buffer *buf, struct buffer *out_buf, size_t size)
{
    if (size>buf->length) {
        size = buf->length;
    }
    int cpsize = size;
    if (size>0) {
        if (out_buf->head == NULL) {
            out_buf->head = out_buf->tail = buf->head;
        } else {
            out_buf->tail->next = buf->head;
        }
    }
    struct buffer_chain *bc = buf->head;
    while (cpsize>0 && bc) {
        int cpsize1 = bc->length;
        if (cpsize1 > cpsize) {
            cpsize1 = cpsize;
        }
        cpsize -= cpsize1;
        out_buf->tail = bc;
        out_buf->length += cpsize1;
        buf->length -= cpsize1;
        if (cpsize1 < (int)bc->length) {
            size_t tmpsize = bc->length - cpsize1;
            struct buffer_chain *tmp = buffer_chain_new(tmpsize);
            if (!tmp) {
                return -1;
            }
            memcpy(tmp->data,(uint8_t *)bc->data+bc->offset+cpsize1,tmpsize);
            tmp->length = tmpsize;
            buf->head = tmp;
            tmp->next = bc->next;
            if (tmp->next == NULL) {
                buf->tail = tmp;
            }
            bc->length -= tmpsize;
            bc->next = NULL;
            break;
        }
        bc = bc->next;
    }
    return size;
}

// Read data from the beginning of buffer and leave the buffer unchanged.
int buffer_copy_front_data(struct buffer *buf, void *out_data, size_t size)
{
    if (size>buf->length) {
        size = buf->length;
    }
    int cpsize = size;
    struct buffer_chain *bc = buf->head;
    while (cpsize>0 && bc) {
        int cpsize1 = cpsize;
        if (cpsize1 > (int)bc->length) {
            cpsize1 = bc->length;
        }
        out_data = (uint8_t *)out_data + size - cpsize;
        memcpy(out_data, bc->data + bc->offset, cpsize1);
        cpsize -= cpsize1;
        bc = bc->next;
    }
    return size;
}

// Remove a specified number of bytes data from the beginning of buffer.
int buffer_remove_front(struct buffer *buf, size_t size)
{
    if (size>buf->length) {
        size = buf->length;
    }
    int cpsize = size;
    struct buffer_chain *bc = buf->head;
    while (cpsize>0 && bc) {
        int cpsize1 = cpsize;
        if (cpsize1 > (int)bc->length) {
            cpsize1 = bc->length;
        }
        cpsize -= cpsize1;
        if (cpsize>0) {
            struct buffer_chain *tmp = bc->next;
            buffer_chain_delete(&bc);
            bc = tmp;
        } else {
            bc->offset += cpsize1;
            bc->length -= cpsize1;
        }
        buf->head = bc;
        buf->length -= cpsize1;
    }
    return size;
}

// Expands the available space in buffer.
int buffer_expand(struct buffer *buf, size_t size)
{
    struct buffer_chain *bc = buf->tail;
    if (bc && bc->size - bc->length >= size) {
        return 0;
    }
    bc = buffer_chain_new(size);
    if (!bc) {
        return -1;
    }
    if (buf->head==NULL) {
        buf->head = buf->tail = bc;
    } else {
        if (buf->tail->length == 0) {
            struct buffer_chain **pbc = &buf->tail;
            *pbc = bc;
            buffer_chain_delete(&buf->tail);
            log_debug("free last\n");
        } else {
            buf->tail->next = bc;
        }
        buf->tail = bc;
    }
    return 0;
}

// Makes the data at the beginning of buffer contiguous.
unsigned char * buffer_merge(struct buffer *buf, size_t size)
{
    if (buf->head==NULL) {
        return NULL;
    }
    if (size > buf->length) {
        size = buf->length;
    }
    struct buffer_chain *bc = buf->head;
    if (bc->length >= size) {
        uint8_t *data = (uint8_t *)(bc->data) + bc->offset;
        return data;
    }
    int remain = bc->size - bc->offset - bc->length;
    if (remain<=size && buffer_chain_should_realign(bc, size)) {
        buffer_chain_align(bc);
        return bc->data;
    }
    struct buffer_chain *newbc = buffer_chain_new(size);
    if (!newbc) {
        return NULL;
    }
    while (size > 0) {
        int cpsize = bc->length;
        if ((int)size < cpsize) {
            cpsize = size;
        }
        memcpy((uint8_t *)newbc->data+newbc->length,(uint8_t *)bc->data+bc->offset,cpsize);
        newbc->length += cpsize;
        size -= cpsize;
        if (cpsize < (int)bc->length) {
            bc->length -= cpsize;
            bc->offset += cpsize;
            break;
        } else {
            struct buffer_chain *tmp = bc->next;
            buffer_chain_delete(&bc);
            bc = tmp;
        }
    }
    newbc->next = bc;
    buf->head = newbc;
    if (newbc->next == NULL) {
        buf->tail = newbc;
    } else if (bc->next == NULL) {
        buf->tail = bc;
    }
    return newbc->data;
}

// Return the total number of bytes stored in the buffer.
size_t buffer_get_length(struct buffer *buf)
{
    return buf->length;
}


// Read from a file descriptor and store the result in buffer.
int buffer_read(struct buffer *buf, int fd, int size)
{
    struct buffer_chain *bc = buf->tail;
    if (bc == NULL) {
        bc = buffer_chain_new(size);
        if (!bc) {
            return 0;
        }
        buf->head = buf->tail = bc;
    }
    int remain = bc->size - bc->offset - bc->length;
    if (remain<=size && buffer_chain_should_realign(bc, size)) {
        buffer_chain_align(bc);
    } else {
        bc = buffer_chain_new(size);
        if (!bc) {
            return 0;
        }
        buf->tail->next = bc;
        buf->tail = bc;
    }

    int n = read(fd,(uint8_t *)bc->data+bc->offset+bc->length,size);
    if (n < 0) {
        switch (errno) {
        case EINTR:
            break;
        case EAGAIN:
            log_error("buffer_read : EAGAIN capture.\n");
            break;
        default:
            return -1; // other error
        }
        return 0; // read no data
    } else if (n==0) {
        return -2; // fd closed
    }

    bc->length += n;
    buf->length += n;
    return n;
}

// Write contents of buffer to a file descriptor and delete the bytes write.
int buffer_write(struct buffer *buf, int fd, size_t size)
{
    if (size>buf->length) {
        size = buf->length;
    }
    int wsize = 0;
    struct buffer_chain *bc = buf->head;
    while (size>0 && bc) {
        int cpsize = bc->length;
        int n = write(fd, (uint8_t *)bc->data+bc->offset, cpsize);
        if (n<0) {
            switch (errno) {
            case EINTR:
                continue;
            case EAGAIN:
                return wsize;
            }
            return -1;
        }
        size -= n;
        wsize += n;
        buf->length -= n;

        if (n==cpsize) {
            struct buffer_chain *tmp = bc->next;
            buffer_chain_delete(&bc);
            bc = tmp;
            buf->head = bc;
            if (buf->head == NULL) {
                buf->tail = NULL;
            }
            // write full of a chain, then go on to write
        } else {
            bc->length -= n;
            bc->offset += n;
            break;
            // write few of a chain, then stop to write
        }
    }
    return wsize;
}

void buffer_dump(struct buffer *buf, const char *name)
{
    log_debug("name=%s:",name);
    struct buffer_chain *bc = buf->head;
    while (bc) {
        int i=0;
        for (i=0; i<(int)bc->length; i++) {
            log_debug("%02X ",*((unsigned char *)bc->data+bc->offset+i));
        }
        bc = bc->next;
    }
    log_debug("\n");
}

void buffer_print(struct buffer *buf, const char *name)
{
    struct buffer_chain *bc = buf->head;
    while (bc) {
        log_debug("name=%s,offset=%ld,length=%ld,size=%ld,data=",name,bc->offset,bc->length,bc->size);
        int i=0;
        for (i=0; i<(int)bc->length; i++) {
            log_debug("%c",*((char *)bc->data+bc->offset+i));
        }
        log_debug("\n");
        bc = bc->next;
    }
}

// use big-endian
int buffer_push_back_integer(struct buffer *buf, int64_t value)
{
    assert(8 == sizeof(int64_t));
    assert(4 == sizeof(int32_t));
    assert(2 == sizeof(int16_t));
    assert(1 == sizeof(int8_t));

    uint8_t sign = _T;
    if (value < 0) {
        sign = _t;
        value = -value;
    }

    uint8_t data[9];
    int size = 0;
    if ((value & 0xFFFFFFFF00000000) > 0) {
        data[0] = sign | _T4;
        data[1] = (value >> 56) & 0xFF;
        data[2] = (value >> 48) & 0xFF;
        data[3] = (value >> 40) & 0xFF;
        data[4] = (value >> 32) & 0xFF;
        data[5] = (value >> 24) & 0xFF;
        data[6] = (value >> 16) & 0xFF;
        data[7] = (value >> 8 ) & 0xFF;
        data[8] = value & 0xFF;
        size = 9;
    } else if ((value & 0xFFFF0000) > 0) {
        data[0] = sign | _T3;
        data[1] = (value >> 24) & 0xFF;
        data[2] = (value >> 16) & 0xFF;
        data[3] = (value >> 8) & 0xFF;
        data[4] = value & 0xFF;
        size = 5;
    } else if ((value & 0xFF00) > 0) {
        data[0] = sign | _T2;
        data[1] = (value >> 8) & 0xFF;
        data[2] = value & 0xFF;
        size = 3;
    } else {
        data[0] = sign | _T1;
        data[1] = value & 0xFF;
        size = 2;
    }

    buffer_push_back_data(buf,data,size);
    return size;
}

int buffer_pop_front_integer(struct buffer *buf, int64_t *value)
{
    assert(8 == sizeof(int64_t));
    assert(4 == sizeof(int32_t));
    assert(2 == sizeof(int16_t));
    assert(1 == sizeof(int8_t));

    int size = 0;
    int length = buf->length;
    if (length < 2) {
        return 0;
    }

    uint8_t data[9];
    buffer_copy_front_data(buf,data,1);

    uint8_t sign = data[0];
    bool positive = ((sign & 0xF0)==0);
    uint8_t flag = sign & 0x0F;

    switch (flag) {
    case _T1:
        size = 2;
        buffer_pop_front_data(buf,data,2);
        *value = (int64_t)data[1];
        break;
    case _T2:
        size = 3;
        buffer_pop_front_data(buf,data,3);
        *value = (int64_t)data[1] << 8
                 | (int64_t)data[2];
        break;
    case _T3:
        size = 5;
        buffer_pop_front_data(buf,data,5);
        *value = (int64_t)data[1] << 24
                 | (int64_t)data[2] << 16
                 | (int64_t)data[3] << 8
                 | (int64_t)data[4];
        break;
    case _T4:
        size = 9;
        buffer_pop_front_data(buf,data,9);
        *value = (int64_t)data[1] << 56
                 | (int64_t)data[2] << 48
                 | (int64_t)data[3] << 40
                 | (int64_t)data[4] << 32
                 | (int64_t)data[5] << 24
                 | (int64_t)data[6] << 16
                 | (int64_t)data[7] << 8
                 | (int64_t)data[8];
        break;
    default:
        size = 0;
        log_error("unknow type integer\n");
        break;
    }
    *value = positive ? *value : -*value;
    return 0;
}




