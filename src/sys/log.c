/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/config.h"
#include "pico/time.h"
#include "pico/types.h"

#include "hardware/watchdog.h"

#include "../io/platform.h"

#include "config.h"

#include "log.h"

#if defined(RASPBERRYPI_PICO)
    #include "hardware/gpio.h"
    #define LED_PIN PICO_DEFAULT_LED_PIN
#elif defined(RASPBERRYPI_PICO_W)
    #include "pico/cyw43_arch.h"
    #define LED_PIN CYW43_WL_GPIO_LED_PIN
    char buf[1];
#else
    #undef LED_PIN
#endif

static LogEntry logEntries[MAX_LOG_ENTRIES];
static uint logCount = 0;
static LogEntry lastLogEntry;

/* --- LED --- */

static LogEntry lastDisplayedEntry;
static struct repeating_timer timer;
static uint32_t gb_pulse_ms = 0;

static inline void led_toggle() {
    #ifdef LED_PIN
        #if defined(RASPBERRYPI_PICO)
            gpio_xor_mask(1u << LED_PIN);
        #elif defined(RASPBERRYPI_PICO_W)
            cyw43_arch_gpio_put(LED_PIN, !cyw43_arch_gpio_get(LED_PIN));
            snprintf(buf, sizeof(buf), 0); // LED acts weird without this?!
        #endif
    #endif
}

static inline void led_set(bool on) {
    #ifdef LED_PIN
        #if defined(RASPBERRYPI_PICO)
            gpio_put(LED_PIN, on);
        #elif defined(RASPBERRYPI_PICO_W)
            cyw43_arch_gpio_put(LED_PIN, on);
            snprintf(buf, sizeof(buf), 0);
        #endif
    #endif
}

static inline void led_reset() {
    cancel_repeating_timer(&timer);
    gb_pulse_ms = 0;
    led_set(1);
}

static inline int64_t led_pulse_callback(alarm_id_t id, void *data) {
    led_toggle();
    return 0;
}

static inline bool led_callback(struct repeating_timer *t) {
    // Toggle LED immediately, then schedule an additional pulse toggle if applicable (non-zero)
    led_toggle();
    if (gb_pulse_ms != 0) {
        add_alarm_in_ms(gb_pulse_ms, led_pulse_callback, NULL, false);
    }
    return true;
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

/**
 * Visually displays a log entry.
 * @param entry The entry to display.
 * @param code The code to display.
 * @param pulse_ms The pulse time in milliseconds (zero for no pulse).
*/
static void log_displayEntry(LogEntry entry, uint32_t code, uint32_t pulse_ms) {
    if (platform_is_fbw()) {
        // Format and display on the display
        // TODO
        // Note: do not show code for info

    } else {
        // Display on Pico built-in LED
        cancel_repeating_timer(&timer);
        add_repeating_timer_ms(code, led_callback, NULL, &timer);
        // If pulse has been enabled, turn the LED off now so it pulses to the on state, not the off state (looks better)
        if (pulse_ms != 0) {
            led_set(0);
            gb_pulse_ms = pulse_ms;
        }
    }
    lastDisplayedEntry = entry;
}

void log_init() {
    #ifdef LED_PIN
        #if defined(RASPBERRYPI_PICO)
            gpio_init(LED_PIN);
            gpio_set_dir(LED_PIN, GPIO_OUT);
        #endif
    #endif
    if (!platform_is_fbw()) led_set(1); // Save power on FBW boards
    logCount = 0;
    log_resetLast();
}

void log_message(LogType type, char msg[64], int code, uint pulse_ms, bool force) {
    if (logCount < MAX_LOG_ENTRIES) {
        LogEntry entry;
        entry.type = type;
        strncpy(entry.msg, msg, sizeof(entry.msg));
        entry.msg[sizeof(entry.msg) - 1] = '\0';
        entry.code = code;
        entry.timestamp = time_us_64();
        logEntries[logCount++] = entry;

        // Display the entry if the error is more severe than the last (or it was forced),
        // there was a code given, and the type is severe enough
        if (type > LOG && code > -1) {
            if ((type > lastLogEntry.type && code < lastLogEntry.code) || force) {
                log_displayEntry(entry, (uint32_t)code, (uint32_t)pulse_ms);
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
        }
        if (typeMsg) {
            printf("%s: (FBW-%d) %s\n", typeMsg, code, msg);
        } else {
            printf("%s\n", msg);
        }

        if (type == FATAL) {
            // Halt execution for fatal errors
            while (true) {
                watchdog_update();
                tight_loop_contents();
            }
        }
    }
}

void log_clear(LogType type) {
    // Create a temporary array to hold all log entries, and find any log entries of the specified type
    LogEntry tempEntries[MAX_LOG_ENTRIES];
    uint tempEntriesCount = 0;
    for (uint i = 0; i < logCount; i++) {
        if (logEntries[i].type != type) {
            // Only copy entries that are not of the specified type, thus deleting them from the temporary array
            tempEntries[tempEntriesCount++] = logEntries[i];
        }
    }
    // Now overwrite the original array
    memcpy(logEntries, tempEntries, sizeof(tempEntries));
    logCount = tempEntriesCount;
    // If the last entry is of the specified type to clear, reset it so the next entry is properly logged
    if (lastLogEntry.type == type) log_resetLast();
    // Reset the error display if the current displayed error is of this type
    if (lastDisplayedEntry.type == type) {
        if (!platform_is_fbw()) led_reset();
        // Go through all log types in reverse order to find the most fatal error (if it exists), and display it instead
        for (LogType type = FATAL; type >= INFO; type--) {
            bool hadError = false;
            for (uint i = 0; i < logCount; i++) {
                if (logEntries[i].type == type) {
                    log_displayEntry(logEntries[i], (uint32_t)logEntries[i].code, 0);
                    hadError = true;
                    break;
                }
            }
            if (hadError) break;
        }
    }
}

uint8_t log_count() { return logCount; }

LogEntry *log_get(uint index) {
    if (index < logCount) {
        return &logEntries[index];
    }
    return NULL;
}
