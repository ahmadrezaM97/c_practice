// String-on-arena implementation
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include "str.h"

Str str_copy(Arena* a, Str src)
{
    Str res = { .data = NULL, .len = 0 };
    if (src.len == 0)
        return res;

    assert(src.data != NULL);

    char* data = arena_alloc_align(a, src.len, _Alignof(char));
    if (data == NULL)
        return res;
    memcpy(data, src.data, src.len);

    res.len = src.len;
    res.data = data;

    return res;
}

void str_print(Str s)
{
    printf(STR_FMT "\n", STR_ARG(s));
}

static inline int min_int(int a, int b)
{
    return a < b ? a : b;
}

Str str_new(Arena* a, const char* s)
{
    Str res = { .data = NULL, .len = 0 };
    if (s == NULL)
        return res;

    // precondition -> s != NULL
    res.len = strlen(s);

    char* dst = arena_alloc_align(a, res.len, _Alignof(char));
    if (!dst)
        return res;
    memcpy(dst, s, res.len);
    res.data = dst;

    return res;
}

Str str_concat(Arena* a, Str s1, Str s2)
{
    assert(s1.len == 0 || s1.data != NULL);
    assert(s2.len == 0 || s2.data != NULL);
    assert(s1.len <= SIZE_MAX - s2.len);

    Str res = { .data = NULL, .len = 0 };

    if ((s1.len > 0 && s1.data == NULL) || (s2.len > 0 && s2.data == NULL))
        return res;
    if (s1.len > SIZE_MAX - s2.len)
        return res;

    size_t total = s1.len + s2.len;

    char* dst = arena_alloc_align(a, total, _Alignof(char));
    if (!dst)
        return res;
    res.data = dst;
    res.len = total;

    memcpy(res.data, s1.data, s1.len);
    memcpy(res.data + s1.len, s2.data, s2.len);

    return res;
}

bool str_cmp(Str s1, Str s2)
{
    if (s1.len != s2.len)
        return false;

    for (size_t i = 0; i < s1.len; i++)
        if (s1.data[i] != s2.data[i])
            return false;

    return true;
}

int str_find(Str s, Str sub_s)
{
    for (size_t i = 0;i < s.len;i++) {
        Str tmp = str_span(s, i, i + sub_s.len);
        if (str_cmp(tmp, sub_s))return i;
    }
    return -1;
}

bool str_contain(Str s, Str sub_s) {
    return str_find(s, sub_s) != -1;
}

StrVec strvec_new(Arena* a, size_t cap)
{
    StrVec res = { .items = NULL, .len = 0, .cap = 0 };
    if (cap == 0)
        return res;

    Str* ptr = arena_alloc_align(a, cap * sizeof(Str), _Alignof(Str));
    if (ptr == NULL)
        return res;
    res.items = ptr;
    res.cap = cap;
    return res;
}

Str strvec_get(StrVec vec, size_t i)
{
    assert(i < vec.len);
    return vec.items[i];
}

Str int_to_str(Arena* a, int number) {
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%d", number);

    Str result = str_new(a, buffer);
    return result;
}


StrBuffer new_str_buffer(Arena* a, size_t cap) {
    StrBuffer res = { .data = NULL, .len = 0, .cap = 0 };
    if (cap == 0)
        return res;
    char* ptr = arena_alloc_align(a, cap, _Alignof(char));
    if (ptr == NULL)
        return res;
    res.data = ptr;
    res.cap = cap;
    return res;
}


void str_buffer_append_char(Arena* a, StrBuffer* b, char c) {
    if (b->len + 1 > b->cap) {
        size_t new_cap = b->cap > 0 ? b->cap * 2 : 1;
        while (new_cap < b->len + 1) new_cap *= 2;
        char* new_data = arena_realloc_align(a, b->data, new_cap, _Alignof(char));
        assert(new_data != NULL);
        b->data = new_data;
        b->cap = new_cap;
    }
    *(b->data + b->len) = c;
    b->len++;
}

void str_buffer_append_str(Arena* a, StrBuffer* b, Str s) {
    // printf("add\n");
    if (b->len > b->cap - s.len) {
        size_t new_cap = b->cap > 0 ? b->cap * 2 : 1;
        while (b->len > new_cap - s.len) new_cap *= 2;
        char* new_data = arena_realloc_align(a, b->data, new_cap, _Alignof(char));
        assert(new_data != NULL);
        b->data = new_data;
        b->cap = new_cap;
    }
    // printf("copy\n");
    memcpy(b->data + b->len, s.data, s.len);
    b->len += s.len;
}

int str_atoi(Str s) {
    int result = 0;
    for (size_t i = 0; i < s.len; i++) {
        result = result * 10 + (s.data[i] - '0');
    }
    return result;
}
Str str_buffer_to_str(StrBuffer buffer) {
    Str res = { .data = buffer.data, .len = buffer.len };
    return res;
}


void strvec_push(Arena* a, StrVec* vec, Str s)
{
    if (vec->len + 1 > vec->cap)
    {
        assert(vec->cap < SIZE_MAX / 2);
        size_t new_cap = vec->cap > 0 ? vec->cap * 2 : 1;
        if (vec->cap > 0 && vec->cap > SIZE_MAX / 2)
            return;
        if (new_cap > SIZE_MAX / sizeof * vec->items)
            return;
        Str* old_ptr = vec->items;
        Str* new_ptr = arena_realloc_align(a, old_ptr, new_cap * sizeof(Str), _Alignof(Str));
        assert(new_ptr != NULL);
        vec->items = new_ptr;
        vec->cap = new_cap;
    }
    vec->items[vec->len] = s;
    vec->len++;
}

Str str_span(Str s, size_t l, size_t r)
{
    Str res = { .data = NULL, .len = 0 };
    if (l >= r)
        return res;

    r = min_int(r, s.len);
    res.len = r - l;
    res.data = s.data + l;
    return res;
}

char* str_to_char_ptr(Arena* a, Str s) {
    char* ptr = arena_alloc_align(a, s.len + 1, _Alignof(char));
    if (ptr == NULL)return NULL;
    memcpy(ptr, s.data, s.len);
    ptr[s.len] = '\0';
    return ptr;
}

Str str_trim(Str s)
{
    size_t l = 0;
    size_t r = s.len;

    while (l < s.len && isspace(s.data[l]))
        l++;
    while (r > 0 && l < r && isspace(s.data[r - 1]))
        r--;

    return str_span(s, l, r);
}


StrVec str_split_s(Arena* a, Str s, Str de)
{
    StrVec res = strvec_new(a, 1);
    size_t start = 0;
    size_t i = 0;
    while (i < s.len)
    {
        Str tmp = str_span(s, i, i + de.len);
        if (str_cmp(tmp, de))
        {
            if (start != i)
            {
                Str v = str_span(s, start, i);
                strvec_push(a, &res, v);
            }
            start = i + de.len;
            i += de.len;
        }
        else {
            i++;
        }
    }
    if (start < s.len)
    {
        Str v = str_span(s, start, s.len);
        strvec_push(a, &res, v);
    }

    return res;
}

StrVec str_split(Arena* a, Str s, char de)
{
    StrVec res = strvec_new(a, 1);

    size_t l = 0;
    size_t r = 0;

    for (size_t i = 0; i < s.len; i++)
    {
        if (s.data[i] != de)
            r = i;

        if (s.data[i] == de)
        {
            if (l <= r)
            {
                Str v = str_span(s, l, r);
                strvec_push(a, &res, v);
            }
            l = r = i + 1;
        }
    }

    if (l <= r && r <= s.len)
    {
        Str v = str_span(s, l, r);
        strvec_push(a, &res, v);
    }
    return res;
}
