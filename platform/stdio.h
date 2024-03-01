#pragma once

#include <stdlib.h>
#include <sys/cdefs.h>
#include "platform/int.h"

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
