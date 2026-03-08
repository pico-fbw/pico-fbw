/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <stdarg.h>
#include <stdio.h>
#include "pico.h"
#include "pico/stdio.h"
#ifndef RASPBERRYPI_PICO_W
    #include <assert.h>
    #include "platform/time.h"
    #include "tusb.h"
#endif

#include "platform/stdio.h"

#ifndef RASPBERRYPI_PICO_W

// clang-format off

    // Interval between tinyusb tasks (in milliseconds)
    #define TINYUSB_TASK_INTERVAL_MS (PICO_STDIO_USB_TASK_INTERVAL_US / 1000)
    static_assert(TINYUSB_TASK_INTERVAL_MS > 0, "TINYUSB_TASK_INTERVAL_MS must be greater than 0");

    // Callback for processing tinyusb events
    static inline i32 tinyusb_task(void *data) {
        tud_task();
        return TINYUSB_TASK_INTERVAL_MS; // Reschedule
        (void)data;
    }

// clang-format on

#endif

// Timeout between waiting for characters in the stdio read function (in microseconds)
#define STDIO_TIMEOUT_US 1000

void stdio_setup() {
#ifndef RASPBERRYPI_PICO_W
    // On devices other than the Pico W, we use a custom tinyusb device stack, so we must initialize it ourselves
    assert(tusb_init());
    // We must handle USB events manually due to custom USB stack
    assert(callback_in_ms(TINYUSB_TASK_INTERVAL_MS, tinyusb_task, NULL));
#endif
    // The stdio types that are initialized here depend on what gets defined in platform/pico/CMakeLists.txt
    stdio_init_all();
}

char *stdin_read() {
    char *buf = NULL;
    u32 i = 0;
    while (true) {
        // Try to get a character from stdin before the timeout
        i32 c = getchar_timeout_us(STDIO_TIMEOUT_US);
        if (c == '\n' || c == '\r' || c == PICO_ERROR_TIMEOUT) {
            // Either timed out or met the end of a line, end
            break;
        } else {
            // Recieved a valid character, resize the buffer and store it
            buf = try_realloc(buf, (i + 1) * sizeof(char));
            if (!buf) {
                return NULL;
            }
            buf[i++] = c;
        }
    }
    // Done reading, null-terminate the buffer if we read a line
    if (i != 0) {
        buf = try_realloc(buf, (i + 1) * sizeof(char));
        if (!buf) {
            return NULL; // Nothing was read
        }
        buf[i] = '\0';
    }
    return buf;
}
