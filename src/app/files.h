#include "arena.h"
#include "str.h"
#include "types.h"

typedef struct {
    str_t content;
    bool error;
    bool doesnt_exist;
} read_result_t;

str_t mycompress(Arena *a_ptr, str_t src);
str_t mydecompress(Arena *a_ptr, str_t in);

bool file_write(Arena *arena_ptr, str_t content, str_t file_path);
read_result_t read_file(Arena *arena_ptr, str_t file_path);
