#pragma once

#include "arena.h"
#include <string.h>
#include <stdbool.h>

#define S(s)                                                                   \
    (Str) { .data = (s), .len = strlen(s) }
#define STR_FMT "%.*s"
#define STR_ARG(s) (int)(s).len, (s).data


typedef struct {
    char* data;
    size_t len;
    size_t cap;
} StrBuffer;

typedef struct
{
    char* data;
    size_t len;
} Str;

typedef struct {
    Str* items;
    size_t len;
    size_t cap;
} StrVec;

Str str_new(Arena* a, const char* s);
Str str_concat(Arena* a, Str s1, Str s2);
Str str_trim(Str s);
StrVec str_split(Arena* a, Str s, char de);
StrVec str_split_s(Arena* a, Str s, Str de);
void str_print(Str s);
Str str_copy(Arena* a, Str src);
StrVec strvec_new(Arena* a, size_t cap);
void strvec_push(Arena* a, StrVec* vec, Str s);
Str strvec_get(StrVec vec, size_t i);
bool str_contain(Str s, Str sub_s);
int str_find(Str s, Str sub_s);
Str str_span(Str s, size_t l, size_t r);
bool str_cmp(Str, Str);
Str int_to_str(Arena* a, int number);
StrBuffer new_str_buffer(Arena* a, size_t cap);
void str_buffer_append_str(Arena* a, StrBuffer* buffer, Str s);
void str_buffer_append_char(Arena* a, StrBuffer* buffer, char c);
Str str_buffer_to_str(StrBuffer buffer);
int str_atoi(Str s);

