/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/defs.h"
#include "platform/gpio.h"
#include "platform/sys.h"
#include "platform/time.h"

#include "io/display.h"

#include "sys/boot.h"
#include "sys/configuration.h"
#include "sys/print.h"
#include "sys/runtime.h"

#include "log.h"

#define MSG_INFO "INFO"
#define MSG_WARN "WARNING"
#define MSG_ERROR "ERROR"
#define MSG_FATAL "FATAL"

#define DISP_LOG_URL "pico-fbw.org/"

static LogEntry *logs = NULL;
static u32 numLogs = 0;
static LogEntry *lastEntry, *lastDisplayedEntry, *queuedEntry;

/* --- LED --- */

static CallbackData *pulseCallback;
static u32 pulseMs = 0;
static CallbackData *toggleCallback;
static u32 toggleMs = 0;

// Resets the LED to the on state and cancels any scheduled state change callbacks
static void led_reset() {
    cancel_callback(toggleCallback);
    toggleMs = 0;
    cancel_callback(pulseCallback);
    pulseMs = 0;
#ifdef PIN_LED
    gpio_set(PIN_LED, HIGH);
#endif
}

// Callback to pulse the LED
i32 led_pulse_callback() {
#ifdef PIN_LED
    gpio_toggle(PIN_LED);
#endif
    return 0; // Don't reschedule
}

// Callback to toggle the LED and schedule an additional pulse if needed
i32 led_callback() {
#ifdef PIN_LED
    // Toggle LED immediately...
    gpio_toggle(PIN_LED);
    // then, if we need to pulse, schedule an additional toggle (to turn off the LED)
    if (pulseMs != 0)
        pulseCallback = callback_in_ms(pulseMs, led_pulse_callback);
    return toggleMs;
#endif
}

/* --- Logging --- */

/**
 * Visually displays a log entry.
 * @param entry The entry to display.
 */
static void display_log(LogEntry *entry) {
#if PLATFORM_SUPPORTS_DISPLAY
    if ((bool)config.system[SYSTEM_USE_DISPLAY]) {
        // Display the entry on the built-in display on FBW
        // We don't want to use the builtin display_string() function because it might wrap text weirdly;
        // we want control over what goes on each line
        char typeMsg[DISPLAY_MAX_LINE_LEN];
        switch (entry->type) {
            case WARNING:
                strcpy(typeMsg, MSG_WARN);
                break;
            case ERROR:
                strcpy(typeMsg, MSG_ERROR);
                break;
            case FATAL:
                strcpy(typeMsg, MSG_FATAL);
                break;
            default:
                break;
        }
        char codeStr[DISPLAY_MAX_LINE_LEN];
        sprintf(codeStr, "%ld", entry->code);
        if (strlen(entry->msg) < DISPLAY_MAX_LINE_LEN + 1) {
            if (entry->type != INFO) {
                display_lines(typeMsg, entry->msg, DISP_LOG_URL, codeStr, true);
            } else {
                // INFO messages shouldn't point people to a URL
                display_lines(typeMsg, entry->msg, NULL, NULL, true);
            }
        } else if (strlen(entry->msg) < DISPLAY_MAX_LINE_LEN * 2) {
            // Get a substring of the first and last DISPLAY_MAX_LINE_LEN chars to display across two lines
            char line1[DISPLAY_MAX_LINE_LEN + 1] = {[0 ... DISPLAY_MAX_LINE_LEN - 1] = ' '};
            char line2[DISPLAY_MAX_LINE_LEN + 1] = {[0 ... DISPLAY_MAX_LINE_LEN - 1] = ' '};
            if (entry->msg[DISPLAY_MAX_LINE_LEN] != ' ') {
                strncpy(line1, entry->msg, DISPLAY_MAX_LINE_LEN - 1);
                line1[DISPLAY_MAX_LINE_LEN - 1] = '-';
                line1[DISPLAY_MAX_LINE_LEN] = '\0';
                strncpy(line2, entry->msg + DISPLAY_MAX_LINE_LEN - 1, DISPLAY_MAX_LINE_LEN);
                line2[DISPLAY_MAX_LINE_LEN] = '\0';
            } else {
                // We don't need a dash here because the line break falls cleanly between words
                strncpy(line1, entry->msg, DISPLAY_MAX_LINE_LEN);
                line1[DISPLAY_MAX_LINE_LEN] = '\0';
                strncpy(line2, entry->msg + DISPLAY_MAX_LINE_LEN, DISPLAY_MAX_LINE_LEN);
                line2[DISPLAY_MAX_LINE_LEN] = '\0';
            }
            if (entry->type != INFO) {
                display_lines(line1, line2, DISP_LOG_URL, codeStr, true);
            } else {
                display_lines(NULL, line1, line2, NULL, true);
            }
        } else {
            display_lines(typeMsg, NULL, DISP_LOG_URL, codeStr, true);
        }
    }
#endif
#ifdef PIN_LED
    led_reset();
    // If pulse has been enabled, turn the LED off now so it pulses to the on state, not the off state (looks better)
    if (entry->pulse != 0) {
        gpio_set(PIN_LED, LOW);
        pulseMs = entry->pulse;
    }
    // Display on built-in LED
    toggleMs = entry->code;
    toggleCallback = callback_in_ms(toggleMs, led_callback);
#endif
    lastDisplayedEntry = entry;
}

// Callback to process the a queued log entry from boot
i32 process_queue() {
    if (!boot_is_booted())
        return 500; // Not booted yet, check back in 500ms
    display_log(queuedEntry);
    return 0;
}

/**
 * Resets the last log entry.
 * @note This makes it so that the next log entry will be displayed regardless.
 */
void reset_last() {
    if (lastEntry) {
        lastEntry->type = NONE;
        lastEntry->code = UINT16_MAX;
    }
}

void log_init() {
#ifdef PIN_LED
    gpio_setup(PIN_LED, OUTPUT);
    gpio_set(PIN_LED, HIGH);
#endif
    logs = NULL;
    numLogs = 0;
    reset_last();
}

void log_message(LogType type, const char *msg, i32 code, u32 pulse_ms, bool force) {
    numLogs++;
    LogEntry *new = reallocarray(logs, numLogs, sizeof(LogEntry));
    if (!new) {
        numLogs--;
        return;
    }
    logs = new;
    LogEntry *entry = &logs[numLogs - 1];
    entry->type = type;
    entry->msg = msg;
    entry->code = code;
    entry->pulse = pulse_ms;
    entry->timestamp = time_us();

    // Display the entry if: the error is more severe than the last,
    // there was a code given, the type is severe enough, it was forced, or of the same type (but newer)
    if (type > LOG && code > -1) {
        if (force || (lastEntry && (type >= lastEntry->type && code <= lastEntry->code))) {
            if (boot_is_booted() || force || type == FATAL || type == INFO) {
                display_log(entry);
            } else {
                // The system isn't booted and the type isn't severe enough to warrant displaying it at the moment,
                // so we'll check back every 500ms if the system is booted and display if it is
                queuedEntry = entry;
                callback_in_ms(500, process_queue);
            }
        }
    }
    lastEntry = entry;

    // Format an error string to be printed
    const char *typeMsg = NULL;
    const char *colorCode = "";
    switch (type) {
        case INFO:
            typeMsg = MSG_INFO;
            colorCode = COLOR_BLUE;
            break;
        case WARNING:
            typeMsg = MSG_WARN;
            colorCode = COLOR_YELLOW;
            break;
        case ERROR:
            typeMsg = MSG_ERROR;
            colorCode = COLOR_LIGHT_RED;
            break;
        case FATAL:
            typeMsg = MSG_FATAL;
            colorCode = COLOR_DARK_RED;
            break;
        default:
            break;
    }
    if (typeMsg) {
        if (code > -1) {
            print("%s%s: (FBW-%ld) %s%s", colorCode, typeMsg, code, msg, COLOR_RESET);
        } else {
            print("%s%s %s%s", colorCode, typeMsg, msg, COLOR_RESET);
        }
    } else
        print("%s", msg);

    if (type == FATAL) {
        // Halt execution for fatal errors
        print("\nFatal error encountered, halting pico-fbw!");
        while (true)
            // Keep the system running but hang (callbacks still run for LED)
            sys_periodic();
    }
}

void log_clear(LogType type) {
    // Delete any entries with matching type from the log array
    for (u32 i = 0; i < numLogs; i++) {
        if (logs[i].type == type) {
            for (u32 j = i; j < numLogs - 1; j++)
                logs[j] = logs[j + 1];
            numLogs--;
            i--;
            LogEntry *new = reallocarray(logs, numLogs, sizeof(LogEntry));
            if (!new)
                return;
            logs = new;
        }
    }
    if (lastEntry) {
        // If the last entry is of the specified type to clear, reset it so the next entry is properly logged
        if (lastEntry->type == type)
            reset_last();
        // Reset the error display if the current displayed error is of this type
        if (lastEntry->type == type) {
            // Go through all log types in reverse order to find the most fatal error (if it exists), and display it instead
            bool hadError = false;
            for (LogType type = FATAL; type >= INFO; type--) {
                for (u32 i = 0; i < numLogs; i++) {
                    if (logs[i].type == type) {
                        display_log(&logs[i]);
                        hadError = true;
                        break;
                    }
                }
                if (hadError)
                    break;
            }
            // If there was no error, reset the display
            if (!hadError) {
                if ((bool)config.system[SYSTEM_USE_DISPLAY])
                    display_power_save();
                else
                    led_reset();
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
        if (logs[i].type == WARNING || logs[i].type == ERROR || logs[i].type == FATAL)
            count++;
    }
    return count;
}

LogEntry *log_get(u32 index) {
    if (index < numLogs && logs)
        return &logs[index];
    return NULL;
}
