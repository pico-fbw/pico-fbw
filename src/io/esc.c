/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <stdlib.h>
#include "platform/pwm.h"
#include "platform/time.h"

#include "io/display.h"
#include "io/receiver.h"

#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"
#include "sys/runtime.h"

#include "esc.h"

/**
 * Waits up to timeout_ms for the throttle input to move, then wait for duration_ms after it stops moving, and write to *detent.
 * @param pin the GPIO pin the ESC is attached to
 * @param detent the detent to write to
 * @param timeout_ms the timeout (before the throttle is moved) in milliseconds
 * @param duration_ms the duration (after the throttle stops moving) in milliseconds
 * @return Whether a timeout occured.
*/
static bool waitForDetent(u32 pin, float *detent, u32 timeout_ms, u32 duration_ms) {
    Timestamp wait = timestamp_in_ms(timeout_ms);
    u16 lastReading = receiver_get(pin, RECEIVER_MODE_ESC);
    bool hasMoved = (abs(((u16)receiver_get(pin, RECEIVER_MODE_ESC) - lastReading)) > config.control[CONTROL_DEADBAND]);
    while (!hasMoved && !timestamp_reached(&wait)) {
        hasMoved = (abs(((u16)receiver_get(pin, RECEIVER_MODE_ESC) - lastReading)) > config.control[CONTROL_DEADBAND]);
    }
    if (timestamp_reached(&wait)) {
        print("[ESC] ESC calibration timed out!");
        return false;
    }

    while (true) {
        esc_set((u32)config.pins[PINS_ESC_THROTTLE], (u16)receiver_get(pin, RECEIVER_MODE_ESC));
        hasMoved = (abs(((u16)receiver_get(pin, RECEIVER_MODE_ESC) - lastReading)) > config.control[CONTROL_DEADBAND]);
        if (!hasMoved) {
            wait = timestamp_in_ms(duration_ms);
            while (!hasMoved && !timestamp_reached(&wait)) {
                hasMoved = (abs(((u16)receiver_get(pin, RECEIVER_MODE_ESC) - lastReading)) > config.control[CONTROL_DEADBAND]);
            }
            if (timestamp_reached(&wait)) break;
        }
        lastReading = receiver_get(pin, RECEIVER_MODE_ESC);
    }
    *detent = (float)lastReading;
    esc_set((u32)config.pins[PINS_ESC_THROTTLE], 0);
    return true;
}

void esc_enable(u32 pin) {
    print("[ESC] setting up ESC on pin %lu", pin);
    u32 pins[] = {pin};
    pwm_setup_write(pins, 1, 50);
    esc_set(pin, 0); // Set initial position to 0 to be safe
}

void esc_set(u32 pin, float speed) {
    // Ensure speed is within range 0-100% and convert from percentage to duty cycle (0-255)
    speed = clampf(speed, 0, 100);
    pwm_write_raw(pin, (u16)((speed / 100.f) * 255.f));
}

bool esc_calibrate(u32 pin) {
    log_message(INFO, "Calibrating ESC", 200, 0, false);
    if (runtime_is_fbw())
        display_string("Select idle thrust.", 0);
    if (!waitForDetent(pin, &calibration.esc[ESC_DETENT_IDLE], (u32)20E3, 4000)) return false;
    if (runtime_is_fbw())
        display_string("Select max continuous thrust.", 33);
    if (!waitForDetent(pin, &calibration.esc[ESC_DETENT_MCT], (u32)10E3, 2000)) return false;
    if (runtime_is_fbw())
        display_string("Select max thrust.", 66);
    if (!waitForDetent(pin, &calibration.esc[ESC_DETENT_MAX], (u32)10E3, 1000)) return false;
    print("[ESC] final detents: %d, %d, %d", (u16)calibration.esc[ESC_DETENT_IDLE],
          (u16)calibration.esc[ESC_DETENT_MCT], (u16)calibration.esc[ESC_DETENT_MAX]);
    calibration.esc[ESC_CALIBRATED] = true;
    print("[ESC] saving detents to flash");
    config_save();
    log_clear(INFO);
    return true;
}

bool esc_isCalibrated() {
    return (bool)calibration.esc[ESC_CALIBRATED];
}
