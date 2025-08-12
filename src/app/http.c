#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "log.h"
#include "http.h"
#include "str.h"


#define BUF_SIZE 4096


HTTP_Request new_http_request(Arena* a) {
    HTTP_Request request = { 0 };
    if (!vector_init(a, &request.headers, 10, sizeof(HTTP_header))) {
        LOG_ERROR("Failed to initialize headers vector");
    }
    return request;
}

HTTP_Response new_http_response(Arena* a) {
    HTTP_Response response = { 0 };
    if (!vector_init(a, &response.headers, 10, sizeof(HTTP_header))) {
        LOG_ERROR("Failed to initialize headers vector");
    }
    return response;
}

int find_header(Vector headers, Str key) {
    for (size_t i = 0; i < headers.len; i++) {
        HTTP_header h = *((HTTP_header*)vector_get(headers, i));
        if (str_cmp(h.key, key)) {
            return i;
        }
    }

    return -1;
}


void* handle_http_connection(void* arg) {
    // This is a common C trick for passing an integer to a thread.
    // The integer file descriptor is cast to a void* in the calling function
    // and then cast back here. This avoids a malloc/free cycle.
    // The cast to `intptr_t` is important to ensure the pointer value is
    // converted to an integer of the correct size without data loss.
    int fd = (int)(intptr_t)arg;
    LOG_INFO("Handling HTTP connection from client %d", fd);

    Arena cnn_arena = arena_new((1 << 22)); // 4MB


    char remaining_byte_buf[(1 << 12)]; // 4KB
    size_t remaing_byte_len = 0;

    while (true) {
        LOG_INFO("inside first loop");

        StrBuffer read_bytes_buff = new_str_buffer(&cnn_arena, (1 << 13)); //8KB

        for (size_t i = 0;i < remaing_byte_len;i++) {
            str_buffer_append_char(&cnn_arena, &read_bytes_buff, remaining_byte_buf[i]);
        }


        bool header_parsed = false;
        int header_end_pos = -1;
        int request_end_pos = -1;

        HTTP_Request http_request = new_http_request(&cnn_arena);

        while (request_end_pos == -1 || read_bytes_buff.len < (size_t)request_end_pos) {
            LOG_INFO("inside second loop");

            LOG_INFO("header_parsed: %d", header_parsed);
            LOG_INFO("request_end_pos: %d", request_end_pos);
            LOG_INFO("read_bytes_buff.len: %zu", read_bytes_buff.len);

            char buf[BUF_SIZE];
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n == 0) {
                LOG_INFO("client %d closed connection", fd);
                goto end_cnn;
            }
            if (n < 0) {
                perror("read failed");
                goto end_cnn;
            }

            for (size_t i = 0; i < (size_t)n; i++) {
                str_buffer_append_char(&cnn_arena, &read_bytes_buff, buf[i]);
            }

            if (!header_parsed)
            {
                str_print(str_buffer_to_str(read_bytes_buff));
                int crlf_token_start = str_find(str_buffer_to_str(read_bytes_buff), S(CRLF_CRLF));
                LOG_INFO("crlf_token_start: %d", crlf_token_start);
                if (crlf_token_start != -1) {
                    LOG_INFO("crlf_token_start: %d", crlf_token_start);

                    header_parsed = true;
                    parse_HTTP_headers(&cnn_arena, str_span(str_buffer_to_str(read_bytes_buff), 0, crlf_token_start), &http_request);
                    int index = find_header(http_request.headers, S("Content-Length"));
                    if (index != -1) {
                        HTTP_header* h = vector_get(http_request.headers, index);
                        int content_length = str_atoi(h->value);
                        request_end_pos += content_length;
                    }

                    header_end_pos = crlf_token_start + strlen(CRLF_CRLF);
                    request_end_pos = header_end_pos;
                }
            }
        }

        Str full_request = str_span(str_buffer_to_str(read_bytes_buff), 0, request_end_pos);
        http_request.body = str_span(full_request, header_end_pos, request_end_pos);

        HTTP_Response http_response = new_http_response(&cnn_arena);
        handle_http_request(&cnn_arena, &http_request, &http_response);

        Str response_str = response_to_str(&cnn_arena, &http_response);
        for (size_t i = 0; i < response_str.len; i++) {
            LOG_INFO("response_str.data[%zu]: %c", i, response_str.data[i]);
        }
        size_t total_send = 0;
        while (total_send < response_str.len) {
            LOG_INFO("total_send: %zu", total_send);
            ssize_t n = write(fd, response_str.data + total_send, response_str.len - total_send);
            if (n < 0) {
                perror("write");
                goto end_cnn;
            }
            total_send += n;
        }


        size_t leftover = read_bytes_buff.len - request_end_pos;
        if (leftover > sizeof(remaining_byte_buf)) leftover = sizeof(remaining_byte_buf);
        memcpy(remaining_byte_buf, read_bytes_buff.data + request_end_pos, leftover);
        remaing_byte_len = leftover;
        LOG_INFO("remaing_byte_len: %zu", remaing_byte_len);
        arena_rest(&cnn_arena);
        LOG_INFO("arena_rest");
    }

end_cnn:
    arena_destroy(&cnn_arena);
    close(fd);
    return NULL;
}


Str http_status_message(HTTP_StatusCode code) {
    switch (code) {
    case HTTP_STATUS_OK: return S("OK");
    case HTTP_STATUS_BAD_REQUEST: return S("Bad Request");
    case HTTP_STATUS_NOT_FOUND: return S("Not Found");
    case HTTP_STATUS_INTERNAL_SERVER_ERROR: return S("Internal Server Error");
    default: return S("Unknown Status");
    }
}


Str response_to_str(Arena* a, HTTP_Response* response) {
    StrBuffer response_buffer = new_str_buffer(a, (size_t)response->headers.len * 100 + response->body.len + 100);

    // add status line
    str_buffer_append_str(a, &response_buffer, S("HTTP/1.1 "));
    str_buffer_append_str(a, &response_buffer, int_to_str(a, response->status_code));
    str_buffer_append_str(a, &response_buffer, S(" "));
    str_buffer_append_str(a, &response_buffer, http_status_message(response->status_code));
    str_buffer_append_str(a, &response_buffer, S(CRLF));

    // add headers
    for (size_t i = 0; i < response->headers.len; i++) {
        HTTP_header h = *((HTTP_header*)vector_get(response->headers, i));
        str_buffer_append_str(a, &response_buffer, h.key);
        str_buffer_append_str(a, &response_buffer, S(": "));
        str_buffer_append_str(a, &response_buffer, h.value);
        str_buffer_append_str(a, &response_buffer, S(CRLF));
    }
    str_buffer_append_str(a, &response_buffer, S(CRLF));
    // add body
    str_buffer_append_str(a, &response_buffer, response->body);
    return str_buffer_to_str(response_buffer);
}

void set_statuc_code_and_close_connection(Arena* a_ptr, HTTP_Response* response, HTTP_StatusCode statuc_code) {
    HTTP_header content_length = { .key = S("Content-Length"), .value = int_to_str(a_ptr, 0) };
    vector_push(a_ptr, &response->headers, (void*)(&content_length));
    response->status_code = statuc_code;
}


void handle_http_request(Arena* a_ptr, HTTP_Request* request, HTTP_Response* response_ptr) {
    if (str_cmp(request->method, S("GET"))) {
        StrVec urls = str_split_s(a_ptr, request->url, S("/"));
        if (urls.len == 0) {
            set_statuc_code_and_close_connection(a_ptr, response_ptr, HTTP_STATUS_NOT_FOUND);
            return;
        }
        Str first_part = strvec_get(urls, 0);

        if (str_cmp(first_part, S("echo"))) {
            Str text = strvec_get(urls, 1);
            if (urls.len != 2) {
                set_statuc_code_and_close_connection(a_ptr, response_ptr, HTTP_STATUS_NOT_FOUND);
                return;
            }

            HTTP_header content_type = { .key = S("Content-Type"), .value = S("text/plain") };
            HTTP_header content_length = { .key = S("Content-Length"), .value = int_to_str(a_ptr, (int)text.len) };

            vector_push(a_ptr, &response_ptr->headers, (void*)(&content_type));
            vector_push(a_ptr, &response_ptr->headers, (void*)(&content_length));

            response_ptr->body = str_trim(text);

            response_ptr->status_code = HTTP_STATUS_OK;
            return;
        }

        if (str_cmp(first_part, S("user-agent"))) {
            int index = find_header(request->headers, S("User-Agent"));
            if (index == -1) {
                set_statuc_code_and_close_connection(a_ptr, response_ptr, HTTP_STATUS_BAD_REQUEST);
                return;
            }

            HTTP_header* h = vector_get(request->headers, index);
            Str user_agent = str_trim(h->value);

            HTTP_header content_type = { .key = S("Content-Type"), .value = S("text/plain") };
            HTTP_header content_length = { .key = S("Content-Length"), .value = int_to_str(a_ptr, (int)user_agent.len) };


            vector_push(a_ptr, &response_ptr->headers, (void*)(&content_type));
            vector_push(a_ptr, &response_ptr->headers, (void*)(&content_length));

            response_ptr->body = str_trim(user_agent);

            response_ptr->status_code = HTTP_STATUS_OK;
            return;
        }
    }

    set_statuc_code_and_close_connection(a_ptr, response_ptr, HTTP_STATUS_NOT_FOUND);
    return;
}




bool parse_HTTP_headers(Arena* a_ptr, Str requestheader_str, HTTP_Request* http_request_ptr) {
    StrVec rows = str_split_s(a_ptr, requestheader_str, S(CRLF));

    StrVec first_row = str_split_s(a_ptr, strvec_get(rows, 0), S(" "));

    http_request_ptr->method = strvec_get(first_row, 0);
    http_request_ptr->url = strvec_get(first_row, 1);
    http_request_ptr->version = strvec_get(first_row, 2);

    for (size_t i = 1; i < rows.len; i++) {
        StrVec header_parts = str_split_s(a_ptr, strvec_get(rows, i), S(":"));

        HTTP_header h;
        h.key = str_trim(strvec_get(header_parts, 0));
        h.value = str_trim(strvec_get(header_parts, 1));

        vector_push(a_ptr, &(http_request_ptr->headers), (void*)(&h));
    }

    return true;
}
