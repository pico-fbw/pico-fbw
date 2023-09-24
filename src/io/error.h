#ifndef __ERROR_H
#define __ERROR_H

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
    ERROR_LEVEL_NONE,
    ERROR_LEVEL_STATUS,
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
 * Initialize the onboard LED (in powered state).
*/
void led_init();

/**
 * Throws an error.
 * @param type The type of error.
 * @param level The level of the error.
 * @param code The error code (FBW-<code>).
 * @param pulse_ms The pulse duration in milliseconds (0 for no pulse).
 * @param force Whether to force the error; if left as false, errors of lower codes will take precedence.
 * @param msg The human-readable error message to be printed (can be left blank for ERROR_LEVEL_NONE or ERROR_LEVEL_STATUS as no message is printed).
*/
void error_throw(ErrorType type, ErrorLevel level, uint code, uint pulse_ms, bool force, const char *msg);

/**
 * Clears an error
 * @param type The type of error.
 * @param all Whether to clear all errors; true will disregard the parameter "type".
*/
void error_clear(ErrorType type, bool all);

#endif // __ERROR_H