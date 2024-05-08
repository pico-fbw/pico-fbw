#pragma once

#include <stdlib.h>
#include "platform/types.h"

// For __printflike:
#if defined(_WIN32)

#include <sal.h>
#if defined(_MSC_VER) && _MSC_VER > 1400
#define __printflike(fmtarg, firstvararg) _Printf_format_string_ fmtarg
#else
#define __printflike(fmtarg, firstvararg)
#endif

#elif defined(__APPLE__)

#include <sys/cdefs.h>

#elif defined(__linux__)

#if defined(__GNUC__) && __GNUC__ >= 3
#define __printflike(fmtarg, firstvararg) __attribute__((__format__(__printf__, fmtarg, firstvararg)))
#else
#define __printflike(fmtarg, firstvararg)
#endif

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
