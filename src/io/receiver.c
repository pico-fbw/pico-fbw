/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <math.h>
#include "platform/helpers.h"
#include "platform/pwm.h"
#if SIMCONNECT
    #include "platform/simconnect.h"
#endif
#include "platform/time.h"

#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"
#include "sys/runtime.h"

#include "receiver.h"

// Special offset threshold for the switch pin (can be more negative than other pins)
#define SWITCH_MIN_OFFSET -200.0f

/**
 * Gets the calibration value for the specified pin.
 * @param pin the pin to get the calibration value of
 * @return the calibration value from receiver calibration.
 * Be aware that this value may not be cohesive;
 * this function does not check to see whether or not a calibration has been done, so it is able to return random data.
 */
static f32 offset_of(i16 pin) {
    // Look up the correct value to fetch based on the pin
    u32 val = PWM_OFFSET_AIL; // Default/fallback as well as AIL
    if (pin == (i16)config.pins[PINS_INPUT_ELE]) {
        val = PWM_OFFSET_ELE;
    } else if (pin == (i16)config.pins[PINS_INPUT_RUD]) {
        val = PWM_OFFSET_RUD;
    } else if (pin == (i16)config.pins[PINS_INPUT_SWITCH]) {
        val = PWM_OFFSET_SW;
    } else if (pin == (i16)config.pins[PINS_INPUT_THROTTLE]) {
        val = PWM_OFFSET_THR;
    } // No else statement needed as we already have a default value
    return calibration.pwm[val];
}

static f32 read_raw(i16 pin, ReceiverMode mode) {
    f32 pulsewidth = pwm_read_raw(pin);
    if (pulsewidth < 0.f) {
        return 0.f; // Invalid pin
    }
    // Map pulsewidth to either 0-180.f (degree) or 0-100.f (percent)
    // We expect a pulsewidth between 1000-2000μs
    switch (mode) {
        case RECEIVER_MODE_DEGREE:
            return mapf(pulsewidth, 1000.f, 2000.f, 0.f, 180.f);
        case RECEIVER_MODE_PERCENT:
            return mapf(pulsewidth, 1000.f, 2000.f, 0.f, 100.f);
    }
    return 0.f;
}

void receiver_enable(const i16 pins[], u32 num_pins) {
    printpre("receiver", "enabling PWM input on %lu pins", num_pins);
    if (!pwm_setup_read(pins, num_pins)) {
        log_message(TYPE_FATAL, "Failed to enable PWM input!", 500, 0, true);
    }
}

f32 receiver_get(i16 pin, ReceiverMode mode) {
#if !SIMCONNECT
    f32 raw = read_raw(pin, mode);
    if (raw < 0) {
        return raw;
    }
#else
    SCFlightControl control;
    if (pin == (i16)config.pins[PINS_INPUT_AIL]) {
        control = FCTRL_AIL;
    } else if (pin == (i16)config.pins[PINS_INPUT_ELE]) {
        control = FCTRL_ELE;
    } else if (pin == (i16)config.pins[PINS_INPUT_RUD]) {
        control = FCTRL_RUD;
    } else if (pin == (i16)config.pins[PINS_INPUT_THROTTLE]) {
        control = FCTRL_THR;
    } else {
        return 0; // Not simulated
    }
    f32 raw = simconnect_get(control);
    (void)mode;
#endif // !SIMCONNECT
    return raw + offset_of(pin);
}

/**
 * Maps a pin to its calibration location in the array.
 * @param pin the pin to map
 * @param loc pointer to store the calibration location
 * @return true if the pin is valid, false otherwise
 */
static bool pin_to_calibration_location(i16 pin, CalibrationPWM *loc) {
    if (pin == (i16)config.pins[PINS_INPUT_AIL]) {
        *loc = PWM_OFFSET_AIL;
    } else if (pin == (i16)config.pins[PINS_INPUT_ELE]) {
        *loc = PWM_OFFSET_ELE;
    } else if (pin == (i16)config.pins[PINS_INPUT_RUD]) {
        *loc = PWM_OFFSET_RUD;
    } else if (pin == (i16)config.pins[PINS_INPUT_SWITCH]) {
        *loc = PWM_OFFSET_SW;
    } else if (pin == (i16)config.pins[PINS_INPUT_THROTTLE]) {
        *loc = PWM_OFFSET_THR;
    } else {
        return false;
    }
    return true;
}

/**
 * Validates a calibration offset value.
 * @param pin the pin being calibrated
 * @param offset the offset value to validate
 * @return true if the offset is valid, false otherwise
 */
static bool validate_calibration_offset(i16 pin, f32 offset) {
    f32 max_offset = config.general[GENERAL_MAX_CALIBRATION_OFFSET];
    // The switch pin can have high negative offsets (but not positive ones)
    if (pin == (i16)config.pins[PINS_INPUT_SWITCH]) {
        if (offset < SWITCH_MIN_OFFSET || offset > max_offset) {
            printpre("receiver", "ERROR: (FBW-500) pin %d's calibration value is too high!", pin);
            return false;
        }
        return true;
    }
    // All other pins must be within the standard limits
    if (fabsf(offset) <= max_offset) {
        printpre("receiver", "ERROR: (FBW-500) pin %d's calibration value is too high!", pin);
        return false;
    }
    return true;
}

/**
 * Performs a single calibration trial for a pin.
 * @param pin the pin to calibrate
 * @param deviation the expected deviation value
 * @param num_samples number of samples to take
 * @param sample_delay_ms delay between samples in milliseconds
 * @param is_throttle whether this pin is the throttle
 * @return the average difference, or NAN on error
 */
static f32 run_calibration_trial(i16 pin, f32 deviation, u32 num_samples, u32 sample_delay_ms, bool is_throttle) {
    f32 total_difference = 0.0f;
    for (u32 s = 0; s < num_samples; s++) {
        f32 read = is_throttle ? read_raw(pin, RECEIVER_MODE_PERCENT) : read_raw(pin, RECEIVER_MODE_DEGREE);
        if (read == INFINITY) {
            printpre("receiver", "ERROR: (FBW-500) pin %d is not a valid pin to calibrate!", pin);
            return NAN;
        }
        total_difference += deviation - read;
        sleep_ms_blocking(sample_delay_ms);
    }

    f32 average = total_difference / (f32)num_samples;
    // Check if the deviation is 270 (occurs with pulsewidth of 0 or 1, i.e., not connected)
    if (average == 270.f) {
        printpre("receiver", "WARNING: pin %d's calibration value seems abnormal, is it connected?", pin);
        return NAN;
    }
    return average;
}

/**
 * Calibrates a single pin by running multiple trials.
 * @param pin the pin to calibrate
 * @param deviation the expected deviation value
 * @param num_samples number of samples per trial
 * @param sample_delay_ms delay between samples in milliseconds
 * @param run_times number of trials to run
 * @return the final calibration offset, or NAN on error
 */
static f32 calibrate_single_pin(i16 pin, f32 deviation, u32 num_samples, u32 sample_delay_ms, u32 run_times) {
    bool is_throttle = pin == (i16)config.pins[PINS_INPUT_THROTTLE];
    f32 final_difference = 0.0f;
    for (u32 t = 0; t < run_times; t++) {
        printpre("receiver", "running trial %lu out of %lu", t + 1, run_times);
        f32 trial_average = run_calibration_trial(pin, deviation, num_samples, sample_delay_ms, is_throttle);
        if (isnan(trial_average)) {
            return NAN;
        }
        final_difference += trial_average;
    }
    return final_difference / (f32)run_times;
}

bool receiver_calibrate(const i16 pins[], u32 num_pins, f32 deviations[], u32 num_samples, u32 sample_delay_ms,
                        u32 run_times) {
    log_message(TYPE_INFO, "Calibrating receiver", 100, 0, true);
    sleep_ms_blocking(2000); // Wait a few moments for tx/rx to set itself up
    for (u32 i = 0; i < num_pins; i++) {
        i16 pin = pins[i];
        f32 deviation = deviations[i];
        printpre("receiver", "calibrating pin %d (%lu/%lu)", pin, i + 1, num_pins);
        // Run calibration trials for this pin
        f32 offset = calibrate_single_pin(pin, deviation, num_samples, sample_delay_ms, run_times);
        if (isnan(offset)) {
            return false;
        }
        print("pin %d's final offset is %f", pin, offset);

        // Find the correct location in the calibration array
        CalibrationPWM loc;
        if (!pin_to_calibration_location(pin, &loc)) {
            printpre("receiver", "ERROR: (FBW-500) pin %d is not a valid pin to calibrate!", pin);
            return false;
        }
        // Validate the calibration offset
        if (!validate_calibration_offset(pin, offset)) {
            return false;
        }
        // Store the calibration value
        calibration.pwm[loc] = offset;
    }
    // Mark calibration as complete and save
    calibration.pwm[PWM_CALIBRATED] = true;
    calibration.pwm[PWM_MODE] = (ControlMode)config.general[GENERAL_CONTROL_MODE];
    printpre("receiver", "saving calibration");
    config_save();
    log_clear(TYPE_INFO);
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

void receiver_get_pins(i16 *pins, u32 *num_pins, f32 *deviations) {
    // Consistant between all control modes
    pins[0] = (i16)config.pins[PINS_INPUT_AIL];
    pins[1] = (i16)config.pins[PINS_INPUT_ELE];
    deviations[0] = 90.0f;
    deviations[1] = 90.0f;
    // Control mode specific pins
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
            pins[2] = (i16)config.pins[PINS_INPUT_RUD];
            pins[3] = (i16)config.pins[PINS_INPUT_SWITCH];
            pins[4] = (i16)config.pins[PINS_INPUT_THROTTLE];
            deviations[2] = 90.0f; // We expect all controls to be centered except switch and throttle
            deviations[3] = 0.0f;
            deviations[4] = 0.0f;
            *num_pins = 5;
            break;
        case CTRLMODE_3AXIS:
            pins[2] = (i16)config.pins[PINS_INPUT_RUD];
            pins[3] = (i16)config.pins[PINS_INPUT_SWITCH];
            deviations[2] = 90.0f;
            deviations[3] = 0.0f;
            *num_pins = 4;
            break;
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_FLYINGWING_ATHR:
            pins[2] = (i16)config.pins[PINS_INPUT_SWITCH];
            pins[3] = (i16)config.pins[PINS_INPUT_THROTTLE];
            deviations[2] = 0.0f;
            deviations[3] = 0.0f;
            *num_pins = 4;
            break;
        case CTRLMODE_2AXIS:
        case CTRLMODE_FLYINGWING:
            pins[2] = (i16)config.pins[PINS_INPUT_SWITCH];
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
