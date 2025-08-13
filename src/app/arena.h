#pragma once

#include <stddef.h>   // size_t, max_align_t
#include <stdint.h>   // uint8_t
#include <stdlib.h>   // malloc, free
#include <assert.h>   // assert
#include "log.h"
#include "types.h"

typedef struct {
    byte* ptr;
    size_t size;
}Arena_Header;



typedef struct {
    byte* base;
    size_t offset;
    size_t cap;

    Arena_Header headers[1000];
    size_t header_size;
} Arena;


static inline size_t align_up(size_t x, size_t align) {
    assert(align != 0 && ((align & (align - 1)) == 0));
    size_t mask = align - 1;
    return (x + mask) & ~mask;
}



static inline Arena arena_new(size_t capacity) {
    Arena a;
    a.base = malloc(capacity);
    a.cap = a.base ? capacity : 0;
    a.offset = 0;
    a.header_size = 0;
    return a;
}

static inline void arena_rest(Arena* a) {
    a->offset = 0;
}

static inline void arena_destroy(Arena* a) {
    if (a == NULL) return;
    free(a->base);
    a->base = NULL;
    a->offset = 0;
    a->cap = 0;
}


static inline double bytes_to_mb(size_t bytes) {
    return (double)bytes / (1024.0 * 1024.0);
}


static inline void* arena_alloc_align(Arena* a, size_t size, size_t align) {
    size_t new_pos_byte_offset = align_up(a->offset, align);
    // printf("->> %f %f %f\n", bytes_to_mb(size), bytes_to_mb(a->cap), bytes_to_mb(a->offset));
    assert(size < a->cap - new_pos_byte_offset);



    byte* ptr = a->base + new_pos_byte_offset;
    a->offset = new_pos_byte_offset + size;

    Arena_Header h = { .ptr = ptr, .size = size };
    a->headers[a->header_size++] = h;
    return ptr;
}

static inline void* arena_realloc_align(Arena* a, byte* ptr, size_t size, size_t align) {
    int header_index = -1;
    for (size_t i = 0;i < a->header_size;i++) {
        if (a->headers[i].ptr == ptr) {
            header_index = i;
        }
    }
    assert(header_index != -1);
    size_t old_size = a->headers[header_index].size;
    if ((size_t)(header_index) == a->header_size - 1) {
        assert(a->offset + size - old_size < a->cap);
        a->offset += (size - old_size);
        a->headers[header_index].size = size;
        return ptr;
    }

    byte* new_ptr = arena_alloc_align(a, size, align);
    assert(new_ptr != NULL);
    memcpy(ptr, new_ptr, old_size);
    return new_ptr;

}


static inline void* arena_realloc(Arena* a, byte* ptr, size_t size) {
    return arena_realloc_align(a, ptr, size, _Alignof(max_align_t));
}

static inline void* arena_alloc(Arena* a, size_t size) {
    return arena_alloc_align(a, size, _Alignof(max_align_t));
}
