#pragma once

// This is a header-only file that simply wraps the HAL-provided printf with the correct format and debug settings
// for easier usage, and also so the HAL can compile independently.

#include <stdbool.h>
#include "platform/stdio.h"

typedef struct PrintDefs {
    bool fbw, aahrs, aircraft, gps, network;
} PrintDefs;

/**
 * printf wrapper for the fbw system
 * @param sys the system to print from (must be a valid member of PrintDefs, such as `fbw` or `aahrs`)
 * @param ... the format string and arguments to print (same as printf)
 */
#define printfbw(sys, ...)                                                                                                     \
    if (shouldPrint.sys) {                                                                                                     \
        wrap_printf("[%s] ", #sys);                                                                                            \
        wrap_printf(__VA_ARGS__);                                                                                              \
        wrap_printf("\n");                                                                                                     \
    }

/**
 * printf wrapper for raw output
 * @param ... the format string and arguments to print (same as printf)
 * @note This does not include a newline character and does not check the shouldPrint.
 */
#define printraw(...) wrap_printf(__VA_ARGS__)

/**
 * printf wrapper
 * @param ... the format string and arguments to print (same as printf)
 */
#define print(...)                                                                                                             \
    if (shouldPrint.fbw) {                                                                                                     \
        printraw(__VA_ARGS__);                                                                                                 \
        printraw("\n");                                                                                                        \
    }

// shouldPrint is defined in configuration.c
extern PrintDefs shouldPrint;
