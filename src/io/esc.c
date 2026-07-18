/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <stdlib.h>
#include "platform/helpers.h"
#include "platform/pwm.h"
#if SIMCONNECT
    #include "platform/simconnect.h"
#endif
#include "platform/time.h"

#include "io/receiver.h"
#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"
#include "sys/runtime.h"

#include "esc.h"

/**
 * Waits up to `timeout_ms` for the throttle input to move,
 * then wait for `duration_ms` after it stops moving, and write to `detent`.
 * @param pin the GPIO pin the ESC is attached to
 * @param detent the detent to write to
 * @param timeout_ms the timeout (before the throttle is moved) in milliseconds
 * @param duration_ms the duration (after the throttle stops moving) in milliseconds
 * @return whether a timeout occured
 */
static bool wait_for_detent(i16 pin, f32 *detent, u32 timeout_ms, u32 duration_ms) {
    Timestamp wait = timestamp_in_ms(timeout_ms);
    u16 lastReading = receiver_get(pin, RECEIVER_MODE_PERCENT);
    bool hasMoved =
        (abs(((u16)receiver_get(pin, RECEIVER_MODE_PERCENT) - lastReading)) > config.control.controlDeadband);
    while (!hasMoved && !timestamp_reached(&wait)) {
        hasMoved =
            (abs(((u16)receiver_get(pin, RECEIVER_MODE_PERCENT) - lastReading)) > config.control.controlDeadband);
    }
    if (timestamp_reached(&wait)) {
        printpre("ESC", "ESC calibration timed out!");
        return false;
    }

    while (true) {
        esc_set((i16)config.pins.escThrottle, (u16)receiver_get(pin, RECEIVER_MODE_PERCENT));
        hasMoved =
            (abs(((u16)receiver_get(pin, RECEIVER_MODE_PERCENT) - lastReading)) > config.control.controlDeadband);
        if (!hasMoved) {
            wait = timestamp_in_ms(duration_ms);
            while (!hasMoved && !timestamp_reached(&wait)) {
                hasMoved = (abs(((u16)receiver_get(pin, RECEIVER_MODE_PERCENT) - lastReading)) >
                            config.control.controlDeadband);
            }
            if (timestamp_reached(&wait)) {
                break;
            }
        }
        lastReading = receiver_get(pin, RECEIVER_MODE_PERCENT);
    }
    *detent = (f32)lastReading;
    esc_set((i16)config.pins.escThrottle, 0);
    return true;
}

void esc_enable(i16 pin) {
    printpre("ESC", "setting up ESC on pin %d", pin);
    if (!pwm_setup_write((const i16[]){pin}, 1, (u32)config.general.escHz)) {
        log_message(TYPE_FATAL, "Failed to enable PWM output!", 500, 0, true);
    }
    esc_set(pin, 0.f); // Set initial position to 0 to be safe
}

void esc_set(i16 pin, f32 speed) {
#if !SIMCONNECT
    // Ensure speed is within range 0-100% and convert from percentage to pulsewidth
    f32 percent = clampf(speed, 0.f, 100.f);
    // ESCs expect a pulsewidth between 1000-2000µs (1000µs is 0%, 2000µs is 100%)
    pwm_write_raw(pin, mapf(percent, 0.f, 100.f, 1000.f, 2000.f));
#else
    simconnect_set(FCTRL_THR, speed);
    (void)pin;
#endif
}

bool esc_calibrate(i16 pin) {
    log_message(TYPE_INFO, "Calibrating ESC", 200, 0, false);
    if (!wait_for_detent(pin, &calibration.esc.detentIdle, (u32)20E3, 4000)) {
        return false;
    }
    if (!wait_for_detent(pin, &calibration.esc.detentMct, (u32)10E3, 2000)) {
        return false;
    }
    if (!wait_for_detent(pin, &calibration.esc.detentMax, (u32)10E3, 1000)) {
        return false;
    }
    printpre("ESC", "final detents: %d, %d, %d", (u16)calibration.esc.detentIdle, (u16)calibration.esc.detentMct,
             (u16)calibration.esc.detentMax);
    calibration.esc.calibrated = true;
    printpre("ESC", "saving detents to flash");
    config_save();
    log_clear(TYPE_INFO);
    return true;
}

bool esc_is_calibrated() {
    return (bool)calibration.esc.calibrated;
}
