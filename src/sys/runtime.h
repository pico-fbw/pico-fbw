#pragma once

#include <stdbool.h>

typedef enum SwitchType {
    SWITCH_TYPE_2_POS,
    SWITCH_TYPE_3_POS,
} SwitchType;

/**
 * Runs the main runtime loop code of the system.
 * Should be called in an infinite loop after the boot process is complete.
 * @param update_aircraft whether or not to update the aircraft's state (aka run the current mode's code)
 */
void runtime_loop(bool update_aircraft);

/**
 * Runs a minimal version of the main runtime loop code of the system.
 * Should be called in an infinite loop after the boot process is complete, but usually after a watchdog event.
 */
void runtime_loop_minimal();

/**
 * Sleeps the system for a `ms` milleseconds, non-blocking.
 * @param ms the number of milliseconds to sleep for
 * @param update_aircraft whether or not to update the aircraft's state (aka run the current mode's code)
 * @note This function simply calls `runtime_loop()` repeatedly until the time has passed,
 * so it's useful if you need to sleep for long periods of time but still update sensors and such.
 */
void runtime_sleep_ms(u32 ms, bool update_aircraft);
