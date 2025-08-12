#pragma once

#include <stdio.h>
#include <time.h>

// Define color codes for logging levels
#define LOG_COLOR_RESET   "\x1b[0m"
#define LOG_COLOR_RED     "\x1b[31m"
#define LOG_COLOR_YELLOW  "\x1b[33m"
#define LOG_COLOR_CYAN    "\x1b[36m"
#define LOG_COLOR_GRAY    "\x1b[90m"

// Generic log macro
#define LOG(level, color, ...) \
    do { \
        time_t t = time(NULL); \
        struct tm* tm_info = localtime(&t); \
        char time_buf[26]; \
        strftime(time_buf, 26, "%Y-%m-%d %H:%M:%S", tm_info); \
        fprintf(stderr, "%s " color "[%s] " LOG_COLOR_GRAY "%s:%d: " LOG_COLOR_RESET, \
                time_buf, level, __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (0)

// Specific log level macros
#ifdef DEBUG
#define LOG_DEBUG(...) LOG("DEBUG", LOG_COLOR_GRAY, __VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif

#define LOG_INFO(...)  LOG("INFO ", LOG_COLOR_CYAN, __VA_ARGS__)
#define LOG_WARN(...)  LOG("WARN ", LOG_COLOR_YELLOW, __VA_ARGS__)
#define LOG_ERROR(...) LOG("ERROR", LOG_COLOR_RED, __VA_ARGS__)
