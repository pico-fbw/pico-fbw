#ifndef __LOG_H
#define __LOG_H

#include <stdint.h>
#include "pico/types.h"

typedef enum LogType {
    NONE,
    LOG, // Logs are simply printed to the console
    INFO, // Info is logged in RAM and can be read back later, and is additionally displayed on the system's LED/OLED
    WARNING,
    ERROR,
    FATAL // Fatal errors will halt the program
} LogType;

typedef struct LogEntry {
    LogType type;
    char msg[64];
    int code;
    uint32_t pulse;
    uint64_t timestamp;
} LogEntry;

#define MSG_INFO "INFO:"
#define MSG_WARN "WARNING:"
#define MSG_ERROR "ERROR:"
#define MSG_FATAL "FATAL:"

#define DISP_LOG_CONCAT "pico-fbw.org/"

#define MAX_LOG_ENTRIES 20

/**
 * Initialize the logging system.
 * @note This also initializes the onboard LED.
 * On the Pico W, this must be called AFTER the cyw43 arch has been initialized, otherwise the program may
 * crash due to attempting to set the LED through the uninitialized cyw43 arch.
*/
void log_init();

/**
 * Logs a message.
 * @param type The type of log to make.
 * @param msg The human-readable error message to be printed.
 * @param code The log code (FBW-<code>), -1 for no code.
 * Will be printed to the console, logged, and displayed if applicable.
 * @param pulse_ms The pulse duration in milliseconds--only applicable for the onboard LED. (0 for no pulse)
 * @param force Whether to force the error; if left as false, errors of lower codes will take precedence.
*/
void log_message(LogType type, char msg[64], int code, uint pulse_ms, bool force);

/**
 * Clears all logs of the specified type.
 * @param type The type of log to clear.
*/
void log_clear(LogType type);

/**
 * @return The current number of log entries.
*/
uint8_t log_count();

/**
 * @return The current number of log entries with severity higher than WARNING (WARNING, ERROR, and FATAL).
*/
uint8_t log_countErrs();

/**
 * @return The log entry at the given index.
*/
LogEntry *log_get(uint index);

#endif // __LOG_H