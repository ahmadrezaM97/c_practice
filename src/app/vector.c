#include "vector.h"
#include "types.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

bool vector_init(Arena* a, Vector* v, size_t cap, size_t elem_size) {
  void* ptr = arena_alloc(a, cap * elem_size);
  if (ptr == NULL) {
    return false;
  }
  v->data_ptr = ptr;
  v->len = 0;
  v->cap = cap;
  v->elem_size = elem_size;
  return true;
}

static int vector_grow(Arena* a, Vector* v) {
  size_t new_cap = v->cap ? v->cap * 2 : 1;
  void* new_data = arena_alloc(a, new_cap * v->elem_size);
  if (new_data == NULL) {
    return -1;
  }
  if (v->data_ptr) {
    memcpy(new_data, v->data_ptr, v->len * v->elem_size);
  }
  v->data_ptr = new_data;
  v->cap = new_cap;
  return 0;
}

bool vector_push(Arena* a, Vector* v, const void* elem) {
  assert(v != NULL && elem != NULL);

  if (v->len >= v->cap || v->data_ptr == NULL) {
    if (vector_grow(a, v) != 0) {
      return false;
    }
  }
  void* dest = (byte*)v->data_ptr + (v->len * v->elem_size);
  memcpy(dest, elem, v->elem_size);
  v->len++;
  return true;
}

void* vector_get(Vector v, size_t index) {
  assert(index < v.len);
  return (byte*)v.data_ptr + (index * v.elem_size);
}

void vector_erase(Vector* v, size_t index) {
  assert(index < v->len);
  byte* base = (byte*)v->data_ptr;
  byte* dest = base + (index * v->elem_size);
  byte* src = dest + v->elem_size;
  size_t count = (v->len - index - 1) * v->elem_size;
  memmove(dest, src, count);
  v->len--;
}
