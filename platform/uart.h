#pragma once

#include <stdbool.h>
#include "platform/int.h"

/**
 * Sets up the given TX and RX pins for UART communication at the given baudrate.
 * @param tx the transmit pin to use
 * @param rx the recieve pin to use
 * @param baud the baudrate to run the UART at
 * @return true if the setup was successful
 * @note Many platforms have limitations on which pins and baudrates can be used for UART.
 * Check the documentation of the platform you are using to ensure you are using valid pins and baudrates.
*/
bool uart_setup(u32 tx, u32 rx, u32 baud);

/**
 * Reads a line from the specifed UART pins, if available.
 * @param tx the transmit pin to use
 * @param rx the recieve pin to use
 * @return A pointer to the line read if there was one (automatically null-terminated),
 *        or NULL if there was no input available.
 * @note This function does not free the memory allocated for the line if read, ensure to free() it after use.
*/
char *uart_read(u32 tx, u32 rx);

/**
 * Writes the given string to the specified UART pins.
 * @param tx the transmit pin to use
 * @param rx the recieve pin to use
 * @param str the string to write
 * @return true if the write was successful
*/
bool uart_write(u32 tx, u32 rx, const char *str);
