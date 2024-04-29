#pragma once

#include <stdbool.h>
#include "platform/types.h"

typedef enum LogType {
    NONE,
    LOG,  // Logs are simply printed to the console
    INFO, // Info is logged in RAM and can be read back later, and is additionally displayed on the system's LED/OLED
    WARNING,
    ERROR,
    FATAL, // Fatal errors will halt the program
} LogType;

typedef struct LogEntry {
    LogType type;
    const char *msg;
    i32 code;
    u32 pulse;
    u64 timestamp;
} LogEntry;

/**
 * Initialize the logging system.
 * @note This also initializes the onboard LED where applicable.
 */
void log_init();

/**
 * Logs a message.
 * @param type the type of log to make
 * @param msg the human-readable error message to be printed
 * @param code the log code (FBW-<code>), -1 for no code
 * @param pulse_ms the pulse duration in milliseconds--only applicable for the onboard LED (0 for no pulse)
 * @param force whether to force the error; if left as false, errors of lower codes will take precedence
 */
void log_message(LogType type, const char *msg, i32 code, u32 pulse_ms, bool force);

/**
 * Clears all logs of the specified type.
 * @param type the type of log to clear
 */
void log_clear(LogType type);

/**
 * @return the current number of log entries
 */
u32 log_count();

/**
 * @return the current number of log entries with severity higher than WARNING (WARNING, ERROR, and FATAL)
 */
u32 log_count_errs();

/**
 * @return the log entry at the given index
 */
LogEntry *log_get(u32 index);
