#include "http.h"
#include "files.h"
#include "log.h"
#include "str.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 4096

http_request_t new_http_request(Arena *a_ptr) {
    return (http_request_t){.headers = http_header_vec_new(a_ptr, 10)};
}

http_response_t new_http_response(Arena *a_ptr) {
    return (http_response_t){.headers = http_header_vec_new(a_ptr, 10)};
}

int find_header(http_header_vec_t headers_vec, str_t key) {
    for (size_t i = 0; i < http_header_vec_len(headers_vec); i++) {
        http_header_t h = http_header_vec_get(headers_vec, i);
        if (str_cmp(h.key, key)) {
            return i;
        }
    }

    return -1;
}

str_t response_to_str(Arena *a, http_response_t *response);

void *handle_http_connection(void *arg) {
    // This is a common C trick for passing an integer to a thread.
    // The integer file descriptor is cast to a void* in the calling function
    // and then cast back here. This avoids a malloc/free cycle.
    // The cast to `intptr_t` is important to ensure the pointer value is
    // converted to an integer of the correct size without data loss.
    int fd = (int)(intptr_t)arg;
    LOG_INFO("Handling HTTP connection from client %d", fd);

    Arena cnn_arena = arena_new((1 << 21)); // 2MB

    char remaining_byte_buf[(1 << 12)]; // 4KB
    size_t remaing_byte_len = 0;

    bool should_close = false;
    while (!should_close) {
        str_buffer_t read_bytes_buff =
            str_buffer_new(&cnn_arena, (1 << 13)); // 8KB

        for (size_t i = 0; i < remaing_byte_len; i++) {
            str_buffer_append_char(&cnn_arena, &read_bytes_buff,
                                   remaining_byte_buf[i]);
        }

        bool header_parsed = false;
        int header_end_pos = -1;
        int request_end_pos = -1;

        http_request_t http_request = new_http_request(&cnn_arena);

        while (request_end_pos == -1 ||
               read_bytes_buff.len < (size_t)request_end_pos) {
            char buf[BUF_SIZE];
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n == 0) {
                goto end_cnn;
            }
            if (n < 0) {
                perror("read failed");
                goto end_cnn;
            }

            for (size_t i = 0; i < (size_t)n; i++) {
                str_buffer_append_char(&cnn_arena, &read_bytes_buff, buf[i]);
            }

            if (!header_parsed) {
                int crlf_token_start =
                    str_find(str_buffer_to_str(read_bytes_buff), S(CRLF_CRLF));
                if (crlf_token_start != -1) {
                    header_parsed = true;
                    parse_HTTP_headers(
                        &cnn_arena,
                        str_span(str_buffer_to_str(read_bytes_buff), 0,
                                 crlf_token_start),
                        &http_request);

                    header_end_pos = crlf_token_start + strlen(CRLF_CRLF);
                    request_end_pos = header_end_pos;

                    int index =
                        find_header(http_request.headers, S("Content-Length"));
                    if (index != -1) {
                        http_header_t h =
                            http_header_vec_get(http_request.headers, index);
                        int content_length = str_atoi(h.value);
                        request_end_pos += content_length;
                    }
                }
            }
        }

        str_t full_request =
            str_span(str_buffer_to_str(read_bytes_buff), 0, request_end_pos);
        str_t body = str_span(full_request, header_end_pos, request_end_pos);

        http_request.body = body;

        http_response_t http_response = new_http_response(&cnn_arena);
        LOG_INFO("Received request: %.*s", (int)full_request.len,
                 full_request.data);
        should_close =
            handle_http_request(&cnn_arena, &http_request, &http_response);
        if (should_close)
            LOG_INFO("Connection will be closed after this response");
        else
            LOG_INFO("Keeping connection alive");
        str_t response_str = response_to_str(&cnn_arena, &http_response);

        size_t total_send = 0;
        while (total_send < response_str.len) {
            ssize_t n = write(fd, response_str.data + total_send,
                              response_str.len - total_send);
            if (n <= 0) {
                perror("write");
                goto end_cnn;
            }
            total_send += n;
        }

        size_t leftover = read_bytes_buff.len - request_end_pos;
        if (leftover > sizeof(remaining_byte_buf))
            leftover = sizeof(remaining_byte_buf);
        memcpy(remaining_byte_buf, read_bytes_buff.data + request_end_pos,
               leftover);
        remaing_byte_len = leftover;
        arena_rest(&cnn_arena);
        printf("finished one request-response cycle\n");
    }

end_cnn:
    arena_destroy(&cnn_arena);
    close(fd);
    return NULL;
}

str_t http_status_message(HTTP_STATUS_CODE code) {
    switch (code) {
    case HTTP_STATUS_OK:
        return S("OK");
    case HTTP_STATUS_BAD_REQUEST:
        return S("Bad Request");
    case HTTP_STATUS_NOT_FOUND:
        return S("Not Found");
    case HTTP_STATUS_INTERNAL_SERVER_ERROR:
        return S("Internal Server Error");
    case HTTP_STATUS_CREATED_SUCCESSFULLY:
        return S("Created");
    default:
        return S("Unknown Status");
    }
}

str_t response_to_str(Arena *a, http_response_t *response) {
    str_buffer_t response_buffer =
        str_buffer_new(a, http_header_vec_len(response->headers) * 10 +
                              response->body.len + 100);

    // add status line
    str_buffer_append_str(a, &response_buffer, S("HTTP/1.1 "));
    str_buffer_append_str(a, &response_buffer,
                          int_to_str(a, response->status_code));
    str_buffer_append_str(a, &response_buffer, S(" "));
    str_buffer_append_str(a, &response_buffer,
                          http_status_message(response->status_code));
    str_buffer_append_str(a, &response_buffer, S(CRLF));

    // add headers
    for (size_t i = 0; i < http_header_vec_len(response->headers); i++) {
        http_header_t h = http_header_vec_get(response->headers, i);
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

void set_response_without_body(Arena *a_ptr, http_response_t *response,
                               HTTP_STATUS_CODE statuc_code) {
    http_header_t content_length = {.key = S("Content-Length"),
                                    .value = S("0")};
    http_header_vec_push(a_ptr, &response->headers, content_length);
    response->status_code = statuc_code;
}

void set_response_with_body(Arena *a_ptr, http_response_t *response_ptr,
                            str_t data, str_t content_type, int statuc_code) {
    http_header_t content_length_header = {
        .key = S("Content-Length"), .value = int_to_str(a_ptr, (int)data.len)};
    http_header_t content_type_header =
        (http_header_t){.key = S("Content-Type"), .value = content_type};
    http_header_vec_push(a_ptr, &response_ptr->headers, content_type_header);
    http_header_vec_push(a_ptr, &response_ptr->headers, content_length_header);

    response_ptr->status_code = statuc_code;
    response_ptr->body = data;
}

bool handle_http_request(Arena *a_ptr, http_request_t *request_ptr,
                         http_response_t *response_ptr) {

    // check if need to close connection
    bool should_close = false;
    int index = find_header(request_ptr->headers, S("Connection"));
    if (index != -1) {
        if (str_cmp(http_header_vec_get(request_ptr->headers, index).value,
                    S("close"))) {
            should_close = true;
            http_header_t close_connection = {.key = S("Connection"),
                                              .value = S("close")};
            http_header_vec_push(a_ptr, &response_ptr->headers,
                                 close_connection);
        }
    }

    str_t dir = S("/tmp/data/codecrafters.io/http-server-tester/");
    str_vec_t urls = str_split_s(a_ptr, request_ptr->url, S("/"));
    if (strvec_len(urls) == 0) {
        set_response_without_body(a_ptr, response_ptr, HTTP_STATUS_OK);
        return should_close;
    }

    str_t first_part = strvec_get(urls, 0);
    if (str_cmp(request_ptr->method, S("POST"))) {
        if (str_cmp(first_part, S("files"))) {
            str_t file_name = strvec_get(urls, 1);
            if (strvec_len(urls) != 2) {
                set_response_without_body(a_ptr, response_ptr,
                                          HTTP_STATUS_INTERNAL_SERVER_ERROR);
                return should_close;
            }

            str_t file_content = request_ptr->body;
            str_t file_path = str_concat(a_ptr, dir, file_name);

            if (!file_write(a_ptr, file_content, file_path)) {
                LOG_ERROR("failed to write");
                set_response_without_body(a_ptr, response_ptr,
                                          HTTP_STATUS_INTERNAL_SERVER_ERROR);
                return should_close;
            }

            set_response_without_body(a_ptr, response_ptr,
                                      HTTP_STATUS_CREATED_SUCCESSFULLY);
            return should_close;
        }
    }

    if (str_cmp(request_ptr->method, S("GET"))) {
        if (str_cmp(first_part, S("files"))) {
            str_t file_name = strvec_get(urls, 1);
            if (strvec_len(urls) != 2) {
                set_response_without_body(a_ptr, response_ptr,
                                          HTTP_STATUS_NOT_FOUND);
                return should_close;
            }

            str_t file_path = str_concat(a_ptr, dir, file_name);
            read_result_t result = read_file(a_ptr, file_path);
            if (result.doesnt_exist) {
                set_response_without_body(a_ptr, response_ptr,
                                          HTTP_STATUS_NOT_FOUND);
                return should_close;
            }

            if (result.error) {
                LOG_ERROR("failed read file");
                set_response_without_body(a_ptr, response_ptr,
                                          HTTP_STATUS_INTERNAL_SERVER_ERROR);
                return should_close;
            }
            set_response_with_body(a_ptr, response_ptr, result.content,
                                   S("application/octet-stream"),
                                   HTTP_STATUS_OK);
            return should_close;
        }

        if (str_cmp(first_part, S("echo"))) {
            // encoding

            bool gzip = false;
            int index = find_header(request_ptr->headers, S("Accept-Encoding"));
            if (index != -1) {
                http_header_t encoding_header =
                    http_header_vec_get(request_ptr->headers, (size_t)index);

                str_print(encoding_header.value);
                str_vec_t available_encodings_list =
                    str_split_s(a_ptr, encoding_header.value, S(","));
                str_vec_t valid_encodings = strvec_new(a_ptr, 1);
                for (size_t i = 0; i < strvec_len(available_encodings_list);
                     i++) {

                    str_t encoding = strvec_get(available_encodings_list, i);
                    encoding = str_trim(encoding);
                    if (str_cmp(encoding, S("gzip"))) {
                        strvec_push(a_ptr, &valid_encodings, encoding);
                        gzip = true;
                    }
                }

                if (strvec_len(valid_encodings) > 0) {
                    str_t valid_encodings_str =
                        str_join(a_ptr, valid_encodings, S(", "));
                    if (strvec_len(valid_encodings) > 0) {
                        http_header_t content_type_header =
                            (http_header_t){.key = S("Content-Encoding"),
                                            .value = valid_encodings_str};

                        http_header_vec_push(a_ptr, &response_ptr->headers,
                                             content_type_header);
                    }
                }
            }

            str_t text = strvec_get(urls, 1);
            if (strvec_len(urls) != 2) {
                set_response_without_body(a_ptr, response_ptr,
                                          HTTP_STATUS_NOT_FOUND);
                return should_close;
            }

            str_t result = str_trim(text);
            if (gzip)
                result = mycompress(a_ptr, str_trim(text));
            set_response_with_body(a_ptr, response_ptr, result, S("text/plain"),
                                   HTTP_STATUS_OK);
            return should_close;
        }

        if (str_cmp(first_part, S("user-agent"))) {
            int index = find_header(request_ptr->headers, S("User-Agent"));
            if (index == -1) {
                set_response_without_body(a_ptr, response_ptr,
                                          HTTP_STATUS_BAD_REQUEST);
                return should_close;
            }

            http_header_t h = http_header_vec_get(request_ptr->headers, index);
            str_t user_agent = str_trim(h.value);

            http_header_vec_push(a_ptr, &response_ptr->headers,
                                 (http_header_t){.key = S("Content-Type"),
                                                 .value = S("text/plain")});
            http_header_vec_push(
                a_ptr, &response_ptr->headers,
                (http_header_t){.key = S("Content-Length"),
                                .value =
                                    int_to_str(a_ptr, (int)user_agent.len)});

            set_response_with_body(a_ptr, response_ptr, str_trim(user_agent),
                                   S("text/plain"), HTTP_STATUS_OK);
            return should_close;
        }
    }

    set_response_without_body(a_ptr, response_ptr, HTTP_STATUS_NOT_FOUND);
    return should_close;
}

bool parse_HTTP_headers(Arena *a_ptr, str_t requestheader_str,
                        http_request_t *http_request_ptr) {
    str_vec_t rows = str_split_s(a_ptr, requestheader_str, S(CRLF));

    str_vec_t first_row = str_split_s(a_ptr, strvec_get(rows, 0), S(" "));

    http_request_ptr->method = strvec_get(first_row, 0);
    http_request_ptr->url = strvec_get(first_row, 1);
    http_request_ptr->version = strvec_get(first_row, 2);

    for (size_t i = 1; i < strvec_len(rows); i++) {
        str_vec_t header_parts =
            str_split_s(a_ptr, strvec_get(rows, i), S(":"));

        http_header_t h = {.key = str_trim(strvec_get(header_parts, 0)),
                           .value = str_trim(strvec_get(header_parts, 1))};

        http_header_vec_push(a_ptr, &(http_request_ptr->headers), h);
    }

    return true;
}
