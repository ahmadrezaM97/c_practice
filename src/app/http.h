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
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
} HTTP_StatusCode;

typedef struct {
    Str key;
    Str value;
} HTTP_header;

typedef struct {
    Str method;
    Str url;
    Str version;
    Str body;
    Vector headers;
} HTTP_Request;

typedef struct {
    HTTP_StatusCode status_code;
    Vector headers;
    Str body;
} HTTP_Response;

Str http_status_message(HTTP_StatusCode code);

bool parse_HTTP_headers(Arena* a, Str request_str, HTTP_Request* request);
void handle_http_request(Arena* a, HTTP_Request* request, HTTP_Response* response);
Str response_to_str(Arena* a, HTTP_Response* response);


void* handle_http_connection(void* arg);




