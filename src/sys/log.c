/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdio.h>
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

// TODO: refactor logging to use malloc and just...be cleaner

static LogEntry logEntries[MAX_LOG_ENTRIES];
static u32 logCount = 0;
static LogEntry lastLogEntry;

/* --- LED --- */

static LogEntry lastDisplayedEntry;
static u32 pulseCallback;
static u32 pulseMs = 0;
static u32 toggleCallback;
static u32 toggleMs = 0;

static inline void led_reset() {
    cancel_callback(pulseCallback);
    cancel_callback(toggleCallback);
    pulseMs = 0;
    gpio_set(PIN_LED, 1);
}

static inline i32 led_pulse_callback(u32 id) {
    gpio_toggle(PIN_LED);
    return 0; // Don't reschedule
    (void)id; // Suppress unused parameter warning (required for callback functions, but not used here)
}

static inline i32 led_callback(u32 id) {
    // Toggle LED immediately...
    gpio_toggle(PIN_LED);
    // then, if we need to pulse, schedule an additional toggle (to turn off the LED)
    if (pulseMs != 0) {
        pulseCallback = callback_in_ms(pulseMs, led_pulse_callback);
    }
    return toggleMs;
    (void)id;
}

/* --- Logging --- */

/**
 * Reset the last log entry to default values.
 * @note This makes it so that the next entry will be logged no mater what.
 */
static void log_resetLast() {
    lastLogEntry.type = NONE;
    lastLogEntry.code = UINT16_MAX;
}

static LogEntry queuedEntry;

/**
 * Visually displays a log entry.
 * @param entry The entry to display.
 */
static void log_displayEntry(LogEntry *entry) {
    if (runtime_is_fbw()) {
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
                display_lines(typeMsg, entry->msg, DISP_LOG_CONCAT, codeStr, true);
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
                display_lines(line1, line2, DISP_LOG_CONCAT, codeStr, true);
            } else {
                display_lines(NULL, line1, line2, NULL, true);
            }
        } else {
            display_lines(typeMsg, NULL, DISP_LOG_CONCAT, codeStr, true);
        }
    } else {
        led_reset();
        // If pulse has been enabled, turn the LED off now so it pulses to the on state, not the off state (looks better)
        if (entry->pulse != 0) {
            gpio_set(PIN_LED, 0);
            pulseMs = entry->pulse;
        }
        // Display on Pico built-in LED
        toggleCallback = callback_in_ms((u32)entry->code, led_callback);
    }
    lastDisplayedEntry = *entry;
}

static inline i32 logProcessQueue(u32 id) {
    if (!boot_is_booted())
        return 500;
    log_displayEntry(&queuedEntry);
    return 0;
    (void)id;
}

void log_init() {
#ifdef LED_PIN
#if defined(RASPBERRYPI_PICO)
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
#endif
#endif
    if (!runtime_is_fbw())
        gpio_set(PIN_LED, 1);
    logCount = 0;
    log_resetLast();
}

void log_message(LogType type, char msg[64], i32 code, u32 pulse_ms, bool force) {
    if (logCount < MAX_LOG_ENTRIES) {
        LogEntry entry;
        entry.type = type;
        strncpy(entry.msg, msg, sizeof(entry.msg));
        entry.msg[sizeof(entry.msg) - 1] = '\0';
        entry.code = code;
        entry.pulse = pulse_ms;
        entry.timestamp = time_us();
        logEntries[logCount++] = entry;

        // Display the entry if the error is more severe than the last,
        // there was a code given, the type is severe enough, it was forced, or of the same type (but newer)
        if (type > LOG && code > -1) {
            if ((type >= lastLogEntry.type && code <= lastLogEntry.code) || force) {
                if (boot_is_booted() || type == FATAL || type == INFO) {
                    log_displayEntry(&entry);
                } else {
                    // The system isn't booted and the type isn't severe enough to warrant displaying it at the moment,
                    // so we'll check every 500ms if the system is booted and display it if it is
                    queuedEntry = entry;
                    callback_in_ms(500, logProcessQueue);
                }
            }
        }
        lastLogEntry = entry;

        // Format an error string to be printed
        const char *typeMsg = NULL;
        switch (type) {
        case INFO:
            typeMsg = MSG_INFO;
            break;
        case WARNING:
            typeMsg = MSG_WARN;
            break;
        case ERROR:
            typeMsg = MSG_ERROR;
            break;
        case FATAL:
            typeMsg = MSG_FATAL;
            break;
        default:
            break;
        }
        if (typeMsg) {
            if (code > -1) {
                print("%s (FBW-%ld) %s", typeMsg, code, msg);
            } else {
                print("%s %s", typeMsg, msg);
            }
        } else {
            print("%s", msg);
        }

        if (type == FATAL) {
            // Halt execution for fatal errors
            print("\nFatal error encountered, halting pico-fbw!");
            while (true)
                sys_periodic();
        }
    }
}

void log_clear(LogType type) {
    // Create a temporary array to hold all log entries, and find any log entries of the specified type
    LogEntry tempEntries[MAX_LOG_ENTRIES];
    u32 tempEntriesCount = 0;
    for (u32 i = 0; i < logCount; i++) {
        if (logEntries[i].type != type) {
            // Only copy entries that are not of the specified type, thus deleting them from the temporary array
            tempEntries[tempEntriesCount++] = logEntries[i];
        }
    }
    // Now overwrite the original array
    memcpy(logEntries, tempEntries, sizeof(tempEntries));
    logCount = tempEntriesCount;
    // If the last entry is of the specified type to clear, reset it so the next entry is properly logged
    if (lastLogEntry.type == type)
        log_resetLast();
    // Reset the error display if the current displayed error is of this type
    if (lastDisplayedEntry.type == type) {
        // Go through all log types in reverse order to find the most fatal error (if it exists), and display it instead
        bool hadError = false;
        for (LogType type = FATAL; type >= INFO; type--) {
            for (u32 i = 0; i < logCount; i++) {
                if (logEntries[i].type == type) {
                    log_displayEntry(&logEntries[i]);
                    hadError = true;
                    break;
                }
            }
            if (hadError)
                break;
        }
        // If there was no error, reset the display
        if (!hadError) {
            if (runtime_is_fbw()) {
                display_powerSave();
            } else {
                led_reset();
            }
        }
    }
}

u8 log_count() { return logCount; }

u8 log_count_errs() {
    u32 count = 0;
    for (u32 i = 0; i < logCount; i++) {
        if (logEntries[i].type == WARNING || logEntries[i].type == ERROR || logEntries[i].type == FATAL)
            count++;
    }
    return count;
}

LogEntry *log_get(u32 index) {
    if (index < logCount) {
        return &logEntries[index];
    }
    return NULL;
}
