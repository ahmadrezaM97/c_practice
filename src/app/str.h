#pragma once

#include "arena.h"
#include "vector.h"

#include <string.h>
#include <stdbool.h>

#define S(s)                                                                   \
    (str_t) { .data = (s), .len = strlen(s) }
#define STR_FMT "%.*s"
#define STR_ARG(s) (int)(s).len, (s).data


typedef struct {
    byte* data;
    size_t len;
    size_t cap;
} str_buffer_t;

typedef struct
{
    byte* data;
    size_t len;
} str_t;


typedef struct { Vector inner; } str_vec_t;

str_vec_t static inline strvec_new(Arena* a_ptr, size_t cap)
{
    return (str_vec_t) { .inner = vector_new(a_ptr, cap, sizeof(str_t)) };
}

str_t static inline strvec_get(str_vec_t vec, size_t i)
{
    str_t* s = (str_t*)vector_get(vec.inner, i);
    return *s;
}

size_t static inline strvec_len(str_vec_t vec) {
    return vec.inner.len;
}

bool static inline strvec_push(Arena* a_ptr, str_vec_t* v_ptr, str_t s)
{
    return vector_push(a_ptr, &(v_ptr->inner), (void*)(&s));
}


str_t str_new(Arena* a, const byte* s);
str_t str_concat(Arena* a, str_t s1, str_t s2);
str_t str_trim(str_t s);
str_vec_t str_split(Arena* a, str_t s, char de);
str_vec_t str_split_s(Arena* a, str_t s, str_t de);
void str_print(str_t s);
str_t str_copy(Arena* a, str_t src);
bool str_contain(str_t s, str_t sub_s);
int str_find(str_t s, str_t sub_s);
str_t str_span(str_t s, size_t l, size_t r);
bool str_cmp(str_t, str_t);
str_t int_to_str(Arena* a, int number);
str_buffer_t str_buffer_new(Arena* a, size_t cap);
void str_buffer_append_str(Arena* a, str_buffer_t* buffer, str_t s);
void str_buffer_append_char(Arena* a, str_buffer_t* buffer, char c);
str_t str_buffer_to_str(str_buffer_t buffer);
int str_atoi(str_t s);
byte* str_to_char_ptr(Arena* a, str_t s);
str_t str_join(Arena* a_ptr, str_vec_t vec, str_t d);
