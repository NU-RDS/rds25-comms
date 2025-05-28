#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <iostream>

#define COLOR_RETURN "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"

// Internal debugging macros
#define __DEBUG_LINE __LINE__
#define __DEBUG_FILE __FILE__
#define __DEBUG_STRINGIFY_HELPER(x) #x
#define __DEBUG_STRINGIFY(x) __DEBUG_STRINGIFY_HELPER(x)

// Should return a string like this: [system:file_name.cpp:line_number] in the color defined by
// color_str
#define __DEBUG_PREFIX(system_str, color_str) \
    color_str "[" system_str "][" __DEBUG_FILE ":" __DEBUG_STRINGIFY(__DEBUG_LINE) "]" \
                                                                                   " " COLOR_RETURN
#define __DEBUG_ERROR_PREFIX(system_str, color_str) \
    COLOR_RED "[" system_str "][" __DEBUG_FILE ":" __DEBUG_STRINGIFY(__DEBUG_LINE) "]" \
                                                                                   " " COLOR_RETURN

#ifdef DEBUG

#define DEBUG_PRINT(system_str, color_str, fmt, ...) \
    fprintf(stdout, __DEBUG_PREFIX(system_str, color_str) fmt, ##__VA_ARGS__)

#define DEBUG_PRINTLN(system_str, color_str, fmt, ...) \
    fprintf(stdout, __DEBUG_PREFIX(system_str, color_str) fmt "\n", ##__VA_ARGS__)

#define DEBUG_PRINT_ERROR(system_str, color_str, fmt, ...) \
    fprintf(stderr, __DEBUG_ERROR_PREFIX(system_str, color_str) fmt, ##__VA_ARGS__)

#define DEBUG_PRINT_ERRORLN(system_str, color_str, fmt, ...) \
    fprintf(stderr, __DEBUG_ERROR_PREFIX(system_str, color_str) fmt "\n", ##__VA_ARGS__)

#define DEBUG_PRINT_FATAL_ERROR(system_str, color_str, fmt, ...)                              \
    do {                                                                                      \
        fprintf(stderr, __DEBUG_ERROR_PREFIX(system_str, color_str) fmt "\n", ##__VA_ARGS__); \
        abort();                                                                              \
    } while (0)

#define DEBUG_PRINT_FATAL_ERRORLN(system_str, color_str, fmt, ...)                            \
    do {                                                                                      \
        fprintf(stderr, __DEBUG_ERROR_PREFIX(system_str, color_str) fmt "\n", ##__VA_ARGS__); \
        abort();                                                                              \
    } while (0)

#else  // DEBUG

#define DEBUG_PRINT(system_str, color_str, fmt, ...) (void)0
#define DEBUG_PRINTLN(system_str, color_str, fmt, ...) (void)0
#define DEBUG_PRINT_ERROR(system_str, color_str, fmt, ...) (void)0
#define DEBUG_PRINT_ERRORLN(system_str, color_str, fmt, ...) (void)0
#define DEBUG_PRINT_FATAL_ERROR(system_str, color_str, fmt, ...) (void)0
#define DEBUG_PRINT_FATAL_ERRORLN(system_str, color_str, fmt, ...) (void)0

#endif  // DEBUG

#define COMMS_SYSTEM_STR "COMMS"
#define COMMS_COLOR_STR COLOR_BLUE

#ifdef COMMS_DEBUG

#define COMMS_DEBUG_PRINT(fmt, ...) \
    DEBUG_PRINT(COMMS_SYSTEM_STR, COMMS_COLOR_STR, fmt, ##__VA_ARGS__)
#define COMMS_DEBUG_PRINTLN(fmt, ...) \
    DEBUG_PRINTLN(COMMS_SYSTEM_STR, COMMS_COLOR_STR, fmt, ##__VA_ARGS__)
#define COMMS_DEBUG_PRINT_ERROR(fmt, ...) \
    DEBUG_PRINT_ERROR(COMMS_SYSTEM_STR, COMMS_COLOR_STR, fmt, ##__VA_ARGS__)
#define COMMS_DEBUG_PRINT_ERRORLN(fmt, ...) \
    DEBUG_PRINT_ERRORLN(COMMS_SYSTEM_STR, COMMS_COLOR_STR, fmt, ##__VA_ARGS__)
#define COMMS_DEBUG_PRINT_FATAL_ERROR(fmt, ...) \
    DEBUG_PRINT_FATAL_ERROR(COMMS_SYSTEM_STR, COMMS_COLOR_STR, fmt, ##__VA_ARGS__)
#define COMMS_DEBUG_PRINT_FATAL_ERRORLN(fmt, ...) \
    DEBUG_PRINT_FATAL_ERRORLN(COMMS_SYSTEM_STR, COMMS_COLOR_STR, fmt, ##__VA_ARGS__)

#else  // COMMS_DEBUG

#define COMMS_DEBUG_PRINT(fmt, ...) (void)0
#define COMMS_DEBUG_PRINTLN(fmt, ...) (void)0
#define COMMS_DEBUG_PRINT_ERROR(fmt, ...) (void)0
#define COMMS_DEBUG_PRINT_ERRORLN(fmt, ...) (void)0
#define COMMS_DEBUG_PRINT_FATAL_ERROR(fmt, ...) (void)0
#define COMMS_DEBUG_PRINT_FATAL_ERRORLN(fmt, ...) (void)0

#endif

#endif  // __DEBUG_H__