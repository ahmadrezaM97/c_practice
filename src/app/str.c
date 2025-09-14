// String-on-arena implementation
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include "str.h"
#include "types.h"

str_t str_copy(Arena* a, str_t src)
{
    str_t res = { .data = NULL, .len = 0 };
    if (src.len == 0)
        return res;

    assert(src.data != NULL);

    byte* data = arena_alloc_align(a, src.len, _Alignof(byte));
    if (data == NULL)
        return res;
    memcpy(data, src.data, src.len);

    res.len = src.len;
    res.data = data;

    return res;
}

void str_print(str_t s)
{
    printf(STR_FMT "\n", STR_ARG(s));
}

static inline int min_int(int a, int b)
{
    return a < b ? a : b;
}

str_t str_new(Arena* a, const byte* s)
{
    str_t res = { .data = NULL, .len = 0 };
    if (s == NULL)
        return res;

    // precondition -> s != NULL
    res.len = strlen(s);

    byte* dst = arena_alloc_align(a, res.len, _Alignof(char));
    if (!dst)
        return res;
    memcpy(dst, s, res.len);
    res.data = dst;

    return res;
}

str_t str_concat(Arena* a, str_t s1, str_t s2)
{
    assert(s1.len == 0 || s1.data != NULL);
    assert(s2.len == 0 || s2.data != NULL);
    assert(s1.len <= SIZE_MAX - s2.len);

    str_t res = { .data = NULL, .len = 0 };

    if ((s1.len > 0 && s1.data == NULL) || (s2.len > 0 && s2.data == NULL))
        return res;
    if (s1.len > SIZE_MAX - s2.len)
        return res;

    size_t total = s1.len + s2.len;

    byte* dst = arena_alloc_align(a, total, _Alignof(char));
    if (!dst)
        return res;
    res.data = dst;
    res.len = total;

    memcpy(res.data, s1.data, s1.len);
    memcpy(res.data + s1.len, s2.data, s2.len);

    return res;
}

bool str_cmp(str_t s1, str_t s2)
{
    if (s1.len != s2.len)
        return false;

    for (size_t i = 0; i < s1.len; i++)
        if (s1.data[i] != s2.data[i])
            return false;

    return true;
}

int str_find(str_t s, str_t sub_s)
{
    for (size_t i = 0;i < s.len;i++) {
        str_t tmp = str_span(s, i, i + sub_s.len);
        if (str_cmp(tmp, sub_s))return i;
    }
    return -1;
}

bool str_contain(str_t s, str_t sub_s) {
    return str_find(s, sub_s) != -1;
}



str_t int_to_str(Arena* a, int number) {
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%d", number);

    str_t result = str_new(a, buffer);
    return result;
}


str_buffer_t str_buffer_new(Arena* a, size_t cap) {
    str_buffer_t res = { .data = NULL, .len = 0, .cap = 0 };
    if (cap == 0)
        return res;
    byte* ptr = arena_alloc_align(a, cap, _Alignof(char));
    if (ptr == NULL)
        return res;
    res.data = ptr;
    res.cap = cap;
    return res;
}


void str_buffer_append_char(Arena* a, str_buffer_t* b, char c) {
    if (b->len + 1 > b->cap) {
        size_t new_cap = b->cap > 0 ? b->cap * 2 : 1;
        while (new_cap < b->len + 1) new_cap *= 2;
        byte* new_data = arena_realloc_align(a, b->data, new_cap, _Alignof(char));
        assert(new_data != NULL);
        b->data = new_data;
        b->cap = new_cap;
    }
    *(b->data + b->len) = c;
    b->len++;
}

void str_buffer_append_str(Arena* a, str_buffer_t* b, str_t s) {
    // printf("add\n");
    if (b->len > b->cap - s.len) {
        size_t new_cap = b->cap > 0 ? b->cap * 2 : 1;
        while (b->len > new_cap - s.len) new_cap *= 2;
        byte* new_data = arena_realloc_align(a, b->data, new_cap, _Alignof(char));
        assert(new_data != NULL);
        b->data = new_data;
        b->cap = new_cap;
    }
    // printf("copy\n");
    memcpy(b->data + b->len, s.data, s.len);
    b->len += s.len;
}

int str_atoi(str_t s) {
    int result = 0;
    for (size_t i = 0; i < s.len; i++) {
        result = result * 10 + (s.data[i] - '0');
    }
    return result;
}
str_t str_buffer_to_str(str_buffer_t buffer) {
    str_t res = { .data = buffer.data, .len = buffer.len };
    return res;
}



str_t str_span(str_t s, size_t l, size_t r)
{
    str_t res = { .data = NULL, .len = 0 };
    if (l >= r)
        return res;

    r = min_int(r, s.len);
    res.len = r - l;
    res.data = s.data + l;
    return res;
}

byte* str_to_char_ptr(Arena* a, str_t s) {
    byte* ptr = arena_alloc_align(a, s.len + 1, _Alignof(char));
    if (ptr == NULL)return NULL;
    memcpy(ptr, s.data, s.len);
    ptr[s.len] = '\0';
    return ptr;
}

str_t str_trim(str_t s)
{
    size_t l = 0;
    size_t r = s.len;

    while (l < s.len && isspace(s.data[l]))
        l++;
    while (r > 0 && l < r && isspace(s.data[r - 1]))
        r--;

    return str_span(s, l, r);
}

// -- strvec-...

str_t str_join(Arena* a_ptr, str_vec_t vec, str_t d) {
    size_t total_len = 0;
    for (size_t i = 0;i < strvec_len(vec);i++) {
        total_len += strvec_get(vec, i).len;
    }
    total_len += d.len * (strvec_len(vec) - 1);

    str_buffer_t buf = str_buffer_new(a_ptr, total_len);
    for (size_t i = 0;i < strvec_len(vec);i++) {
        str_buffer_append_str(a_ptr, &buf, strvec_get(vec, i));
        if (i != strvec_len(vec) - 1) {
            str_buffer_append_str(a_ptr, &buf, d);
        }
    }
    return str_buffer_to_str(buf);
}

str_vec_t str_split_s(Arena* a, str_t s, str_t de)
{
    str_vec_t res = strvec_new(a, 1);
    size_t start = 0;
    size_t i = 0;
    while (i < s.len)
    {
        str_t tmp = str_span(s, i, i + de.len);
        if (str_cmp(tmp, de))
        {
            if (start != i)
            {
                str_t v = str_span(s, start, i);
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
        str_t v = str_span(s, start, s.len);
        strvec_push(a, &res, v);
    }

    return res;
}

str_vec_t str_split(Arena* a, str_t s, char de)
{
    str_vec_t res = strvec_new(a, 1);

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
                str_t v = str_span(s, l, r);
                strvec_push(a, &res, v);
            }
            l = r = i + 1;
        }
    }

    if (l <= r && r <= s.len)
    {
        str_t v = str_span(s, l, r);
        strvec_push(a, &res, v);
    }
    return res;
}
