#pragma once

// This is a header-only file that simply wraps the HAL-provided printf with the correct format and debug settings
// for easier usage, and also so the HAL can compile independently.

#include <stdbool.h>
#include "platform/defs.h" // A platform can define NO_COLOR_OUTPUT to disable terminal color output
#include "platform/stdio.h"

#if !NO_COLOR_OUTPUT
    // Color codes for ANSI terminal colors
    #define COLOR_BLUE "\x1b[38;2;59;130;246m"
    #define COLOR_YELLOW "\x1b[38;2;234;179;8m"
    #define COLOR_LIGHT_GREEN "\x1b[38;2;22;163;74m"
    #define COLOR_LIGHT_RED "\x1b[38;2;248;113;113m"
    #define COLOR_DARK_RED "\x1b[31m"
    #define COLOR_RESET "\x1b[0m"
#else
    #define COLOR_BLUE ""
    #define COLOR_YELLOW ""
    #define COLOR_LIGHT_GREEN ""
    #define COLOR_LIGHT_RED ""
    #define COLOR_DARK_RED ""
    #define COLOR_RESET ""
#endif

typedef struct PrintDefs {
    bool fbw, aahrs, aircraft, gps, network;
} PrintDefs;

/**
 * printf wrapper for raw output
 * @param ... the format string and arguments to print (same as printf)
 * @note This does not include a newline character and does not check the shouldPrint.
 */
#define printraw(...) wrap_printf(__VA_ARGS__)

/**
 * printf wrapper
 * @param ... the format string and arguments to print (same as printf)
 * @note This function automatically appends a newline.
 */
#define print(...)                                                                                                             \
    if (shouldPrint.fbw) {                                                                                                     \
        printraw(__VA_ARGS__);                                                                                                 \
        printraw("\n");                                                                                                        \
    }

/**
 * printf wrapper with a prefix
 * @param prefix the prefix to print before the message
 * @param ... the format string and arguments to print (same as printf)
 * @note This function automatically appends a newline.
 */
#define printpre(prefix, ...)                                                                                                  \
    if (shouldPrint.fbw) {                                                                                                     \
        printraw("%s[%s]%s ", COLOR_LIGHT_GREEN, prefix, COLOR_RESET);                                                         \
        printraw(__VA_ARGS__);                                                                                                 \
        printraw("\n");                                                                                                        \
    }

/**
 * printf wrapper to be used with PrintDefs
 * @param sys the system to print from (must be a valid member of PrintDefs, such as `fbw` or `aahrs`)
 * @param ... the format string and arguments to print (same as printf)
 * @note This function automatically appends a newline.
 */
#define printfbw(sys, ...)                                                                                                     \
    if (shouldPrint.sys) {                                                                                                     \
        printpre(#sys, __VA_ARGS__);                                                                                           \
    }

// shouldPrint is defined in configuration.c
extern PrintDefs shouldPrint;
