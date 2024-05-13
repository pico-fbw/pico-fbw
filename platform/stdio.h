#pragma once

#include "platform/defs.h" // Platforms can define/import __printflike

#include <stdlib.h>
#if __has_include(<sys/cdefs.h>)
    #include <sys/cdefs.h>
#endif
#include "platform/types.h"

#ifndef __printflike
    #define __printflike(fmtarg, firstvararg)
    #warning "__printflike is not defined, there will be no format string checking for printf-like functions."
#endif

/**
 * Sets up all relavent stdio sources.
 */
void stdio_setup();

/**
 * Reads a line from the stdin, if available.
 * @return A pointer to the line read if there was one (automatically null-terminated),
 *         or NULL if there was no input available.
 * @note This function does not free the memory allocated for the line if read, ensure to free() it after use.
 */
char *stdin_read();

/**
 * A wrapper for the printf() function that outputs to all initialized stdout sources.
 * @param fmt the format string
 * @param ... the arguments to be formatted
 * @return the number of characters printed
 */
int __printflike(1, 2) wrap_printf(const char *fmt, ...);

/**
 * Attemps to reallocate a buffer.
 * @param buf the buffer to reallocate
 * @param size the new size of the buffer
 * @return a pointer to the new buffer if successful, or NULL if the reallocation failed
 */
static inline char *try_realloc(char *buf, size_t size) {
    char *nbuf = (char *)realloc((void *)buf, size);
    if (!nbuf)
        free(buf);
    return nbuf;
}
