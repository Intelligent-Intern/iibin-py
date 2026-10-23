#include "wire.h"
#include <stdlib.h>
#include <string.h>
int iibin_append(iibin_buffer *b, const void *data, size_t n) {
    if (n > IIBIN_MAX_BYTES || b->size > IIBIN_MAX_BYTES-n) return 0;
    size_t needed=b->size+n;
    if (needed>b->capacity) {
        size_t cap=b->capacity?b->capacity:128;
        while(cap<needed) cap*=2;
        if(cap>IIBIN_MAX_BYTES) cap=IIBIN_MAX_BYTES;
        void *p=realloc(b->data,cap); if(!p) return 0;
        b->data=p; b->capacity=cap;
    }
    if(n) memcpy(b->data+b->size,data,n);
    b->size=needed; return 1;
}
int iibin_write_varint(iibin_buffer *b, uint64_t v) {
    unsigned char bytes[10]; size_t n=0;
    do { bytes[n++]=(unsigned char)((v&127)|(v>127?128:0)); v>>=7; }while(v);
    return iibin_append(b,bytes,n);
}
int iibin_read_varint(const unsigned char *d,size_t n,size_t *p,uint64_t *v) {
    *v=0;
    for(unsigned i=0;i<10;i++) {
        if(*p>=n) return 0;
        unsigned char c=d[(*p)++];
        if(i==9 && c>1) return 0;
        *v|=((uint64_t)(c&127))<<(7*i);
        if(!(c&128)) return 1;
    }
    return 0;
}
void iibin_buffer_free(iibin_buffer *b){free(b->data);b->data=NULL;b->size=b->capacity=0;}
