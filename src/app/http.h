#pragma once

#include <stdlib.h>
#include "types.h"
#include "str.h"
#include "vector.h"

#define CRLF "\r\n"
#define CRLF_CRLF "\r\n\r\n"
#define MAX_HEADERS 32
#define MAX_METHOD_LEN 8
#define MAX_PATH_LEN 256
#define MAX_VERSION_LEN 16
#define MAX_HEADER_NAME 64
#define MAX_HEADER_VALUE 256

typedef enum {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_CREATED_SUCCESSFULLY = 201,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
} HTTP_STATUS_CODE;

typedef struct {
    str_t key;
    str_t value;
} http_header_t;

typedef struct {
    Vector inner;
} http_header_vec_t;

size_t static inline http_header_vec_len(http_header_vec_t v) {
    return v.inner.len;
}

http_header_vec_t static inline http_header_vec_new(Arena* a_ptr, size_t cap)
{
    return (http_header_vec_t) { .inner = vector_new(a_ptr, cap, sizeof(http_header_t)) };
}


http_header_t static inline http_header_vec_get(http_header_vec_t vec, size_t i)
{
    return *(http_header_t*)vector_get(vec.inner, i);
}

bool static inline http_header_vec_push(Arena* a_ptr, http_header_vec_t* v_ptr, http_header_t s)
{
    return vector_push(a_ptr, &(v_ptr->inner), (void*)(&s));
}

typedef struct {
    str_t method;
    str_t url;
    str_t version;
    str_t body;
    http_header_vec_t headers;
} http_request_t;

typedef struct {
    HTTP_STATUS_CODE status_code;
    http_header_vec_t headers;
    str_t body;
} http_response_t;

str_t http_status_message(HTTP_STATUS_CODE code);

bool parse_HTTP_headers(Arena* a, str_t request_str, http_request_t* request);
bool handle_http_request(Arena* a, http_request_t* request, http_response_t* response);
str_t response_to_str(Arena* a, http_response_t* response);


void* handle_http_connection(void* arg);




