#ifndef __ERROR_H
#define __ERROR_H

#include "pico/types.h"

typedef enum ErrorType {
    ERROR_NONE,
    ERROR_PWM,
    ERROR_IMU,
    ERROR_GPS,
    ERROR_PID,
    ERROR_WIFLY,
    ERROR_GENERAL
} ErrorType;

typedef enum ErrorLevel {
    ERROR_LEVEL_WARN,
    ERROR_LEVEL_ERR,
    ERROR_LEVEL_FATAL
} ErrorLevel;

typedef enum Error {
    FBW_100,
    FBW_250,
    FBW_500,
    FBW_800,
    FBW_1000,
    FBW_2000
} Error;

/**
 * Throws an error.
 * @param type The type of error.
 * @param level The level of the error.
 * @param code The error code (FBW-<code>).
 * @param msg The human-readable error message to be printed.
*/
void error_throw(ErrorType type, ErrorLevel level, uint code, const char *msg);

#endif // __ERROR_H