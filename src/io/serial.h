#ifndef __SERIAL_H
#define __SERIAL_H

#include "hardware/uart.h"

// Timeout between waiting for characters in the stdio read function (in microseconds)
#define STDIO_TIMEOUT_US 1000

// Timeout between waiting for characters in the UART read function (in microseconds)
// This is used for GPS modules which operate at a lower baud, so this value must be higher
#define UART_TIMEOUT_US (STDIO_TIMEOUT_US * 4)

/**
 * Reads a line from stdin if available.
 * @return A pointer to the line read if there was one (automatically null-terminated),
 *         or NULL if there was no input available.
 * @note This function does not free the memory allocated for the line if read, ensure to free() it after use.
 * @note On the Pico, stdin includes all serial I/O compiled into the build by CMake; by default on pico-fbw this includes both USB and UART.
*/
char *stdin_read_line();

/**
 * Reads a line from a specified UART instance if available.
 * @param uart The UART instance to read from.
 * @return A pointer to the read line, or NULL if no line could be read.
 * @note This function does not free the memory allocated for the line if read, ensure to free() it after use.
*/
char *uart_read_line(uart_inst_t *uart);

#endif // __SERIAL_H
