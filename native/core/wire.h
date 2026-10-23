#ifndef IIBIN_WIRE_H
#define IIBIN_WIRE_H
#include <stddef.h>
#include <stdint.h>
#define IIBIN_MAX_BYTES (16u * 1024u * 1024u)
typedef struct { unsigned char *data; size_t size, capacity; } iibin_buffer;
int iibin_append(iibin_buffer *, const void *, size_t);
int iibin_write_varint(iibin_buffer *, uint64_t);
int iibin_read_varint(const unsigned char *, size_t, size_t *, uint64_t *);
void iibin_buffer_free(iibin_buffer *);
#endif
