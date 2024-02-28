/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include "platform/pwm.h"
#include "platform/time.h"

#include "io/display.h"

#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"
#include "sys/runtime.h"

#include "receiver.h"

/** @return true if the value is within the maximum calibration offset */
#define WITHIN_MAX_CALIBRATION_OFFSET(value, offset) ((value) >= -offset && (value) <= offset)

/**
 * Gets the calibration value for the specified pin.
 * @param pin the pin to get the calibration value of
 * @return the calibration value from PWM calibration.
 * Be aware that this value may not be cohesive;
 * this function does not check to see whether or not a calibration has been done, so it is able to return random data.
 */
static inline float pwmOffsetOf(u32 pin) {
    // Look up the correct value to fetch based on the pin
    u32 val;
    if (pin == (u32)config.pins[PINS_INPUT_ELE]) {
        val = PWM_OFFSET_ELE;
    } else if (pin == (u32)config.pins[PINS_INPUT_RUD]) {
        val = PWM_OFFSET_RUD;
    } else if (pin == (u32)config.pins[PINS_INPUT_SWITCH]) {
        val = PWM_OFFSET_SW;
    } else if (pin == (u32)config.pins[PINS_INPUT_THROTTLE]) {
        val = PWM_OFFSET_THR;
    } else {
        val = PWM_OFFSET_AIL; // Default as well as AIL
    }
    // Read from the correct sector based on the value
    return calibration.pwm[val];
}

static inline float readRaw(u32 pin, ReceiverMode mode) {
    i32 pulsewidth = pwm_read_raw(pin);
    if (pulsewidth < 0)
        return INFINITY; // Invalid pin
    return ((mode == RECEIVER_MODE_DEG ? 180E3f : 100E3f) * ((float)pulsewidth * 1.6E-8f - 1E-3f));
}

void receiver_enable(u32 pins[], u32 num_pins) {
    print("[pwm] enabling PWM input on %lu pins", num_pins);
    if (!pwm_setup_read(pins, num_pins))
        log_message(FATAL, "Failed to enable PWM input!", 500, 0, true);
}

float receiver_get(u32 pin, ReceiverMode mode) { return readRaw(pin, mode) + pwmOffsetOf(pin); }

bool receiver_calibrate(const u32 pins[], u32 num_pins, const float deviations[], u32 num_samples, u32 sample_delay_ms,
                        u32 run_times) {
    log_message(INFO, "Calibrating PWM", 100, 0, false);
    sleep_ms_blocking(2000); // Wait a few moments for tx/rx to set itself up
    for (u32 i = 0; i < num_pins; i++) {
        u32 pin = pins[i];
        print("[pwm] calibrating pin %lu (%lu/%lu)\n", pin, i + 1, num_pins);
        if (runtime_is_fbw())
            display_string("Please do not touch the transmitter!", ((i + 1) * 100) / num_pins);
        float deviation = deviations[i];
        float final_difference = 0.0f;
        bool isThrottle = pins[i] == (u32)config.pins[PINS_INPUT_THROTTLE];
        for (u32 t = 0; t < run_times; t++) {
            print("[pwm] running trial %lu out of %lu", t + 1, run_times);
            float total_difference = 0.0f;
            for (u32 s = 0; s < num_samples; s++) {
                total_difference +=
                    (deviation - (isThrottle ? readRaw(pin, RECEIVER_MODE_ESC) : readRaw(pin, RECEIVER_MODE_DEG)));
                sleep_ms_blocking(sample_delay_ms);
            }
            // Check to see if the deviation is 270 (this value occurs with a pulsewidth of 0 or 1, aka not connected)
            if ((total_difference / (float)num_samples) == 270.0f) {
                print("[pwm] WARNING: pin %lu's calibration value seems abnormal, is it connected?", pin);
                return false;
            }
            // Add the total difference recorded divided by the samples we took (average) to the final difference
            final_difference = final_difference + (total_difference / (float)num_samples);
        }
        // Get our final average and save it to the correct byte in our array which we write to flash
        // Any pins over 4 (thus, pins belonging to PIO1) will be in the second array
        print("pin %lu's final offset is %f", pin, (final_difference / (float)run_times));
        // Find the correct location in the array to write to
        CalibrationPWM loc;
        if (pin == (u32)config.pins[PINS_INPUT_AIL]) {
            loc = PWM_OFFSET_AIL;
        } else if (pin == (u32)config.pins[PINS_INPUT_ELE]) {
            loc = PWM_OFFSET_ELE;
        } else if (pin == (u32)config.pins[PINS_INPUT_RUD]) {
            loc = PWM_OFFSET_RUD;
        } else if (pin == (u32)config.pins[PINS_INPUT_SWITCH]) {
            loc = PWM_OFFSET_SW;
        } else if (pin == (u32)config.pins[PINS_INPUT_THROTTLE]) {
            loc = PWM_OFFSET_THR;
        } else {
            print("[pwm] ERROR: [FBW-500] pin %lu is not a valid pin to calibrate!", pin);
            return false;
        }
        // Check to ensure the value is within limits before adding it to be written
        if (!WITHIN_MAX_CALIBRATION_OFFSET((final_difference / run_times), config.general[GENERAL_MAX_CALIBRATION_OFFSET])) {
            if (pin == (u32)config.pins[PINS_INPUT_SWITCH]) {
                // The switch pin is a little special; it can have high offsets but only if they are negative, otherwise modes
                // won't register properly
                if ((final_difference / (float)run_times) < -200.0f ||
                    (final_difference / (float)run_times) > config.general[GENERAL_MAX_CALIBRATION_OFFSET])
                    goto error;
            } else
                goto error;
        error:
            print("[pwm] ERROR: [FBW-500] pin %lu's calibration value is too high!", pin);
            return false;
        }
        calibration.pwm[loc] = final_difference / (float)run_times;
    }
    calibration.pwm[PWM_CALIBRATED] = true;
    calibration.pwm[PWM_MODE] = (ControlMode)config.general[GENERAL_CONTROL_MODE];
    print("[pwm] saving calibration to flash");
    config_save();
    log_clear(INFO);
    return true;
}

ReceiverCalibrationStatus receiver_is_calibrated() {
    // Read the calibration flag
    if ((bool)calibration.pwm[PWM_CALIBRATED]) {
        // Ensure that the control mode we are in is the same as the one in which we calibrated
        if ((ControlMode)config.general[GENERAL_CONTROL_MODE] != (ControlMode)calibration.pwm[PWM_MODE]) {
            return RECEIVERCALIBRATION_INVALID;
        }
        return RECEIVERCALIBRATION_OK;
    } else {
        return RECEIVERCALIBRATION_INCOMPLETE;
    }
}

void receiver_get_pins(u32 *pins, u32 *num_pins, float *deviations) {
    // Consistant between all control modes
    pins[0] = (u32)config.pins[PINS_INPUT_AIL];
    pins[1] = (u32)config.pins[PINS_INPUT_ELE];
    deviations[0] = 90.0f;
    deviations[1] = 90.0f;
    // Control mode specific pins
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
    case CTRLMODE_3AXIS_ATHR:
        pins[2] = (u32)config.pins[PINS_INPUT_RUD];
        pins[3] = (u32)config.pins[PINS_INPUT_SWITCH];
        pins[4] = (u32)config.pins[PINS_INPUT_THROTTLE];
        deviations[2] = 90.0f; // We expect all controls to be centered except switch and throttle
        deviations[3] = 0.0f;
        deviations[4] = 0.0f;
        *num_pins = 5;
        break;
    case CTRLMODE_3AXIS:
        pins[2] = (u32)config.pins[PINS_INPUT_RUD];
        pins[3] = (u32)config.pins[PINS_INPUT_SWITCH];
        deviations[2] = 90.0f;
        deviations[3] = 0.0f;
        *num_pins = 4;
        break;
    case CTRLMODE_2AXIS_ATHR:
        pins[2] = (u32)config.pins[PINS_INPUT_SWITCH];
        pins[3] = (u32)config.pins[PINS_INPUT_THROTTLE];
        deviations[2] = 0.0f;
        deviations[3] = 0.0f;
        *num_pins = 4;
        break;
    case CTRLMODE_2AXIS:
        pins[2] = (u32)config.pins[PINS_INPUT_SWITCH];
        deviations[2] = 0.0f;
        *num_pins = 3;
        break;
    case CTRLMODE_FLYINGWING_ATHR:
        pins[2] = (u32)config.pins[PINS_INPUT_SWITCH];
        pins[3] = (u32)config.pins[PINS_INPUT_THROTTLE];
        deviations[2] = 0.0f;
        deviations[3] = 0.0f;
        *num_pins = 4;
        break;
    case CTRLMODE_FLYINGWING:
        pins[2] = (u32)config.pins[PINS_INPUT_SWITCH];
        deviations[2] = 0.0f;
        *num_pins = 3;
        break;
    }
}

bool receiver_has_athr() {
    return (ControlMode)config.general[GENERAL_CONTROL_MODE] == CTRLMODE_3AXIS_ATHR ||
           (ControlMode)config.general[GENERAL_CONTROL_MODE] == CTRLMODE_2AXIS_ATHR ||
           (ControlMode)config.general[GENERAL_CONTROL_MODE] == CTRLMODE_FLYINGWING_ATHR;
}

bool receiver_has_rud() {
    return (ControlMode)config.general[GENERAL_CONTROL_MODE] == CTRLMODE_3AXIS ||
           (ControlMode)config.general[GENERAL_CONTROL_MODE] == CTRLMODE_3AXIS_ATHR;
}
