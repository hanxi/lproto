#ifndef buffer_h
#define buffer_h

#include <sys/types.h>

struct buffer;

struct buffer * buffer_new();
void buffer_delete(struct buffer **pbuf);

// operator buffer back
// Append data to the end of buffer.
int buffer_push_back_data(struct buffer *buf, const void *data, size_t size);
// Move all data from in_buf into buffer.
int buffer_push_back_buffer(struct buffer *buf, struct buffer *in_buf);

// operator buffer front
// Prepend data to the beginning of the buffer.
int buffer_push_front_data(struct buffer *buf, const void *data, size_t size);
// Move all data from the in_buf to the beginning of the buffer.
int buffer_push_front_buffer(struct buffer *buf, struct buffer *in_buf);
// Read data from buffer and delete the bytes read.
int buffer_pop_front_data(struct buffer *buf, void *out_data, size_t size);
// Move data from beginning of buffer to out_buf.
int buffer_pop_front_buffer(struct buffer *buf, struct buffer *out_buf, size_t size);

// copy data
// Read data from the beginning of buffer and leave the buffer unchanged.
int buffer_copy_front_data(struct buffer *buf, void *out_data, size_t size);

// remove data
// Remove a specified number of bytes data from the beginning of buffer.
int buffer_remove_front(struct buffer *buf, size_t size);

// Expands the available space in buffer.
int buffer_expand(struct buffer *buf, size_t size);

// Makes the data at the beginning of buffer contiguous.
unsigned char * buffer_merge(struct buffer *buf, size_t size);

// Return the total number of bytes stored in the buffer.
size_t buffer_get_length(struct buffer *buf);

// Read from a file descriptor and store the result in buffer.
int buffer_read(struct buffer *buf, int fd, int size);
// Write contents of buffer to a file descriptor and delete the bytes write.
int buffer_write(struct buffer *buf, int fd, size_t size);

// pack data
int buffer_push_back_integer(struct buffer *buf, int64_t value);

// upack data
int buffer_pop_front_integer(struct buffer *buf, int64_t *value);


void buffer_print(struct buffer *buf, const char *name);

#endif

