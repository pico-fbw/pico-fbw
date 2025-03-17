/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/defs.h"
#include "platform/gpio.h"
#include "platform/sys.h"
#include "platform/time.h"

#include "sys/boot.h"
#include "sys/configuration.h"
#include "sys/print.h"
#include "sys/runtime.h"

#include "log.h"

// TODO: redo logging system (it's clunky to use and still kind of like you're using the display)
// - use ringbuf instead of heap
// - make logging functions simpler and more intuitive to call
// - more general system (less specific to LED)

#define MSG_INFO "INFO"
#define MSG_WARN "WARNING"
#define MSG_ERROR "ERROR"
#define MSG_FATAL "FATAL"

static LogEntry *logs = NULL;
static u32 numLogs = 0;
static LogEntry *lastEntry = NULL, *lastDisplayedEntry = NULL;
static CallbackData *queueCallback = NULL;

/* --- LED --- */

#ifdef PIN_LED

static CallbackData *pulseCallback = NULL;
static u32 pulseMs = 0;
static CallbackData *toggleCallback = NULL;
static u32 toggleMs = 0;

// Resets the LED to the on state and cancels any scheduled state change callbacks
static void led_reset() {
    cancel_callback(toggleCallback);
    toggleMs = 0;
    // We cannot cancel the pulse callback as it does not reschedule itself, and the callback system doesn't support
    // cancelling a one-time callback
    pulseMs = 0;
    gpio_set(PIN_LED, STATE_HIGH);
}

// Callback to pulse the LED
static i32 led_pulse_callback(void *data) {
    gpio_toggle(PIN_LED);
    return 0; // Don't reschedule
    (void)data;
}

// Callback to toggle the LED and schedule an additional pulse if needed
static i32 led_callback(void *data) {
    // Toggle LED immediately...
    gpio_toggle(PIN_LED);
    // then, if we need to pulse, schedule an additional toggle (to turn off the LED)
    if (pulseMs != 0) {
        pulseCallback = callback_in_ms(pulseMs, led_pulse_callback, NULL);
    }
    return toggleMs;
    (void)data;
}

#endif // PIN_LED

/* --- Logging --- */

/**
 * Displays a log entry's code as a series of blinks on the LED (if equipped).
 * @param entry the entry to display.
 */
static void display_on_led(LogEntry *entry) {
#ifdef PIN_LED
    led_reset();
    // If pulse has been enabled, turn the LED off now so it pulses to the on state, not the off state (looks better)
    if (entry->pulse != 0) {
        gpio_set(PIN_LED, STATE_LOW);
        pulseMs = entry->pulse;
    }
    // Display on built-in LED
    toggleMs = entry->code;
    toggleCallback = callback_in_ms(toggleMs, led_callback, NULL);
#endif
    lastDisplayedEntry = entry;
}

// Callback to process the a queued log entry from boot
static i32 process_queue(void *data) {
    if (!boot_is_booted()) {
        return 500; // Not booted yet, check back in 500ms
    }
    LogEntry *queuedEntry = (LogEntry *)data;
    if (queuedEntry) {
        display_on_led(queuedEntry);
    }
    return 0;
}

/**
 * Resets the last log entry.
 * @note This makes it so that the next log entry will be displayed regardless.
 */
static void reset_last() {
    if (lastEntry) {
        lastEntry->type = TYPE_NONE;
        lastEntry->code = UINT16_MAX;
    }
}

void log_init() {
#ifdef PIN_LED
    gpio_setup(PIN_LED, MODE_OUTPUT);
    gpio_set(PIN_LED, STATE_HIGH);
#endif
    logs = NULL;
    numLogs = 0;
    reset_last();
}

void log_message(LogType type, const char *msg, i32 code, u32 pulse_ms, bool force) {
    LogEntry *new = realloc(logs, (numLogs + 1) * sizeof(LogEntry));
    if (!new) {
        return;
    }
    logs = new;
    numLogs++;
    LogEntry *entry = &logs[numLogs - 1];
    entry->type = type;
    entry->msg = msg;
    entry->code = code;
    entry->pulse = pulse_ms;
    entry->timestamp = time_us();

    // Display the entry if: the error is more severe than the last,
    // there was a code given, the type is severe enough, it was forced, or of the same type (but newer)
    if (type >= TYPE_INFO && code > -1) {
        if (force || (lastEntry && (type >= lastEntry->type || code <= lastEntry->code))) {
            if (boot_is_booted() || force || type == TYPE_FATAL || type == TYPE_INFO) {
                display_on_led(entry);
            } else {
                // The system isn't booted and the type isn't severe enough to warrant displaying it at the moment,
                // so we'll check back every 500ms if the system is booted and display if it is
                if (queueCallback) {
                    cancel_callback(queueCallback);
                }
                queueCallback = callback_in_ms(500, process_queue, (void *)entry);
            }
        }
    }
    lastEntry = entry;

    // Format an error string to be printed
    const char *typeMsg = "";
    const char *colorCode = "";
    switch (type) {
        case TYPE_INFO:
            typeMsg = MSG_INFO;
            colorCode = COLOR_BLUE;
            break;
        case TYPE_WARNING:
            typeMsg = MSG_WARN;
            colorCode = COLOR_YELLOW;
            break;
        case TYPE_ERROR:
            typeMsg = MSG_ERROR;
            colorCode = COLOR_LIGHT_RED;
            break;
        case TYPE_FATAL:
            typeMsg = MSG_FATAL;
            colorCode = COLOR_LIGHT_RED_BOLD;
            break;
        default:
            break;
    }
    if (typeMsg) {
        if (code > -1) {
            print("%s%s: (FBW-%ld) %s%s", colorCode, typeMsg, code, msg, COLOR_RESET);
        } else {
            print("%s%s: %s%s", colorCode, typeMsg, msg, COLOR_RESET);
        }
    } else {
        print("%s", msg);
    }

    if (type == TYPE_FATAL) {
        // Halt execution for fatal errors
        print("\n" COLOR_LIGHT_RED "Fatal error encountered, halting pico-fbw!");
        while (true) {
            // Keep the system running but hang (callbacks still run for LED)
            sys_periodic();
        }
    }
}

void log_clear(LogType type) {
    // Delete any entries with matching type from the log array
    for (u32 i = 0; i < numLogs; i++) {
        if (logs[i].type == type) {
            for (u32 j = i; j < numLogs - 1; j++) {
                logs[j] = logs[j + 1];
            }
            numLogs--;
            i--;
            LogEntry *new = realloc(logs, numLogs * sizeof(LogEntry));
            if (!new) {
                return;
            }
            logs = new;
        }
    }
    if (lastEntry) {
        // If the last entry is of the specified type to clear, reset it so the next entry is properly logged
        if (lastEntry->type == type) {
            reset_last();
        }
        // Reset the error display if the current displayed error is of this type
        if (lastEntry->type == type) {
            // Go through all log types in reverse order to find the most fatal error (if it exists), and display it
            bool hadError = false;
            for (LogType type = TYPE_FATAL; type >= TYPE_INFO; type--) {
                for (u32 i = 0; i < numLogs; i++) {
                    if (logs[i].type >= type) {
                        display_on_led(&logs[i]);
                        hadError = true;
                        break;
                    }
                }
                if (hadError) {
                    break;
                }
            }
            // If there was no error, reset the LED
            if (!hadError) {
#ifdef PIN_LED
                led_reset();
#endif
            }
        }
    }
}

u32 log_count() {
    return numLogs;
}

u32 log_count_errs() {
    u32 count = 0;
    for (u32 i = 0; i < numLogs; i++) {
        if (logs[i].type >= TYPE_WARNING) {
            count++;
        }
    }
    return count;
}

LogEntry *log_get(u32 index) {
    if (index < numLogs && logs) {
        return &logs[index];
    }
    return NULL;
}
