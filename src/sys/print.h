/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#pragma once

// This is a header-only file that simply wraps printf with correct format and debug settings.

#include <stdbool.h>
#include <stdio.h>
#include "platform/defs.h" // A platform can define NO_COLOR_OUTPUT to disable terminal color output

#ifndef NO_COLOR_OUTPUT
    // Color codes for ANSI terminal colors
    #define COLOR_BLUE "\x1b[38;2;59;130;246m"
    #define COLOR_YELLOW "\x1b[38;2;234;179;8m"
    #define COLOR_LIGHT_GREEN "\x1b[38;2;22;163;74m"
    #define COLOR_LIGHT_RED "\x1b[38;2;248;113;113m"
    #define COLOR_LIGHT_RED_BOLD "\x1b[1;38;2;248;113;113m"
    #define COLOR_RESET "\x1b[0m"
#else
    #define COLOR_BLUE ""
    #define COLOR_YELLOW ""
    #define COLOR_LIGHT_GREEN ""
    #define COLOR_LIGHT_RED ""
    #define COLOR_LIGHT_RED_BOLD ""
    #define COLOR_RESET ""
#endif

typedef struct PrintDefs {
    bool fbw, aahrs, aircraft, gps, network;
} PrintDefs;

/**
 * printf wrapper for raw output
 * @param ... the format string and arguments to print (same as printf)
 * @note This does not include a newline character and does not check whether printing is enabled.
 */
#define printraw(...) printf(__VA_ARGS__)

/**
 * printf wrapper
 * @param ... the format string and arguments to print (same as printf)
 * @note This function automatically appends a newline.
 */
#define print(...)                                                                                                     \
    if (shouldPrint.fbw) {                                                                                             \
        printraw(__VA_ARGS__);                                                                                         \
        printraw("\n");                                                                                                \
    }

/**
 * printf wrapper with a prefix
 * @param prefix the prefix to print before the message
 * @param ... the format string and arguments to print (same as printf)
 * @note This function automatically appends a newline.
 */
#define printpre(prefix, ...)                                                                                          \
    if (shouldPrint.fbw) {                                                                                             \
        printraw("%s[%s]%s ", COLOR_LIGHT_GREEN, prefix, COLOR_RESET);                                                 \
        printraw(__VA_ARGS__);                                                                                         \
        printraw("\n");                                                                                                \
    }

/**
 * printf wrapper to be used with PrintDefs
 * @param sys the system to print from (must be a valid member of PrintDefs, such as `fbw` or `aahrs`)
 * @param ... the format string and arguments to print (same as printf)
 * @note This function automatically appends a newline.
 */
#define printsys(sys, ...)                                                                                             \
    if (shouldPrint.sys) {                                                                                             \
        printpre(#sys, __VA_ARGS__);                                                                                   \
    }

// shouldPrint is defined in configuration.c
extern PrintDefs shouldPrint;
