#pragma once

#include <stddef.h>   // size_t, max_align_t
#include <stdint.h>   // uint8_t
#include <stdlib.h>   // malloc, free
#include <assert.h>   // assert

typedef struct {
    uint8_t* base;
    size_t offset;
    size_t cap;
} Arena;

static inline size_t align_up(size_t x, size_t align) {
    assert(align != 0 && ((align & (align - 1)) == 0));
    size_t mask = align - 1;
    return (x + mask) & ~mask;
}

static inline Arena arena_new(size_t capacity) {
    Arena a;
    a.base = (uint8_t*)malloc(capacity);
    a.cap = a.base ? capacity : 0;
    a.offset = 0;
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

static inline void* arena_alloc_align(Arena* a, size_t size, size_t align) {
    size_t new_pos_byte_offset = align_up(a->offset, align);
    if (size > a->cap - new_pos_byte_offset) return NULL;
    void* ptr = a->base + new_pos_byte_offset;
    a->offset = new_pos_byte_offset + size;
    return ptr;
}

static inline void* arena_alloc(Arena* a, size_t size) {
    return arena_alloc_align(a, size, _Alignof(max_align_t));
}
