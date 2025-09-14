#include "files.h"
#include "arena.h"
#include "str.h"
#include <errno.h>
#include <zlib.h>

str_t mycompress(Arena *a_ptr, str_t src) {
    z_stream strm;
    memset(&strm, 0, sizeof(strm));

    uLongf out_cap = compressBound(src.len) + 18; // gzip overhead

    byte *out_ptr = arena_alloc_align(a_ptr, out_cap, 1);
    // Initialize gzip compression: level=default, method=deflate,
    // windowBits=15+16 (gzip header/trailer), memLevel=8 (1..9),
    // strategy=default
    int ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8,
                           Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        LOG_ERROR("error in deflateInit2");
        return (str_t){0};
    }

    strm.next_in = (Bytef *)src.data;
    strm.avail_in = (uInt)src.len;

    strm.next_out = (Bytef *)out_ptr;
    strm.avail_out = (uInt)out_cap;

    do {
        ret = deflate(&strm, Z_FINISH);
        if (ret == Z_STREAM_END)
            break;
        if (ret != Z_OK && ret != Z_BUF_ERROR) {
            LOG_ERROR("deflate failed: ret=%d in=%u out=%u", ret, strm.avail_in,
                      strm.avail_out);
            deflateEnd(&strm);
            return (str_t){0};
        }
        if (strm.avail_out == 0) {
            out_cap *= 2;
            out_ptr = arena_realloc_align(a_ptr, out_ptr, out_cap * 2, 1);
            assert(out_ptr != NULL);
        }

    } while (1);

    if (ret != Z_STREAM_END) {
        LOG_ERROR("error in deflate");
        deflateEnd(&strm);
        return (str_t){0};
    }
    deflateEnd(&strm);

    return (str_t){.data = out_ptr, .len = strm.total_out};
}

str_t mydecompress(Arena *a_ptr, str_t in) {
    z_stream strm;
    memset(&strm, 0, sizeof(strm));

    int ret = inflateInit2(&strm, 15 + 32); // auto-detect zlib or gzip
    if (ret != Z_OK) {
        LOG_ERROR("error in inflateInit");
        return (str_t){0};
    }

    uLongf out_cap = (1 << 10); // 1KB
    byte *out_ptr = arena_alloc_align(a_ptr, out_cap, 1);
    strm.next_in = (Bytef *)in.data;
    strm.avail_in = in.len;
    strm.next_out = (Bytef *)out_ptr;
    strm.avail_out = out_cap;

    do {
        ret = inflate(&strm, Z_FINISH);
        if (ret == Z_STREAM_END)
            break;
        if (ret != Z_OK && ret != Z_BUF_ERROR) {
            LOG_ERROR("inflate failed: ret=%d in=%u out=%u", ret, strm.avail_in,
                      strm.avail_out);
            inflateEnd(&strm);
            return (str_t){0};
        }
        if (strm.avail_out == 0) {
            out_cap *= 2;
            out_ptr = arena_realloc_align(a_ptr, out_ptr, out_cap * 2, 1);
            assert(out_ptr != NULL);
        }
    } while (1);

    inflateEnd(&strm);
    return (str_t){.data = out_ptr, .len = strm.total_out};
}

read_result_t read_file(Arena *arena_ptr, str_t file_path) {
    byte *path = str_to_char_ptr(arena_ptr, file_path);
    if (path == NULL)
        return (read_result_t){.error = true};

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror("fopen fails");
        if (errno == ENOENT)
            return (read_result_t){.doesnt_exist = true, .error = true};
        return (read_result_t){.error = true};
    }

    str_buffer_t str_buff = str_buffer_new(arena_ptr, (1 << 14)); // 4KB
    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, 1024, file)) > 0) {
        str_t ss = {.len = n, .data = buf};
        str_buffer_append_str(arena_ptr, &str_buff, ss);
    }

    if (ferror(file)) {
        perror("fread");
        goto close_file;
    }

    fclose(file);
    return (read_result_t){.content = str_buffer_to_str(str_buff)};

close_file:
    fclose(file);
    return (read_result_t){.error = true};
}

bool file_write(Arena *arena_ptr, str_t content, str_t file_path) {
    byte *path = str_to_char_ptr(arena_ptr, file_path);
    if (path == NULL)
        return false;

    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        perror("fopen fails");
        goto close_file;
    }

    size_t n = fwrite(content.data, 1, content.len, file);
    if (n != content.len) {
        perror("write to file failes");
        goto close_file;
    }

    fclose(file);
    return true;

close_file:
    fclose(file);
    return false;
}
