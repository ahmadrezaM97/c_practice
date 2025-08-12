#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include "arena.h"

#ifdef __cplusplus
extern "C" {
#endif

    /* ——— Vector type ——— */
    typedef struct {
        void* data_ptr;
        size_t len;
        size_t cap;
        size_t elem_size;
    } Vector;

    /* ——— Core API ——— */

    bool vector_init(Arena* a, Vector* v, size_t cap, size_t elem_size);
    bool vector_push(Arena* a, Vector* v, const void* elem);
    void* vector_get(Vector v, size_t index);
    void vector_erase(Vector* v, size_t index);


#endif /* VECTOR_H */
