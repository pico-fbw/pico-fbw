/**
 * This file utilizes code under the MIT License. See "LICENSE" for details.
*/

/**
 * Huge thanks to 'GitJer' on GitHub for most of the PWM input code!
 * Check them out here: https://github.com/GitJer/Some_RPI-Pico_stuff/tree/main/PwmIn/PwmIn_4pins
*/

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "pico/platform.h"
#include "pico/time.h"

#include "hardware/irq.h"
#include "hardware/pio.h"

#include "display.h"
#include "flash.h"

#include "../sys/log.h"

#include "pwm.h"
#include "pwm.pio.h"

typedef struct PWMState {
    uint32_t pulsewidth, period;
    uint8_t pin;
} PWMState;

static PWMState states[8];
uint gb_num_pins = 0; // Number of pins being used for PWM

static void pio0Handler(void) {
    for (uint i = 0; i < 4; i++) {
        // Check if the IRQ has been raised for this state machine
        if (pio0_hw->irq & 1 << i) {
            pio0_hw->irq = 1 << i; // Clear interrupt
            states[i].pulsewidth = pio_sm_get(pio0, i); // Read pulsewidth from FIFO
            states[i].period = pio_sm_get(pio0, i); // Read low period from the FIFO
            pio0_hw->irq = 1 << i; // Clear interrupt
        }
    }
}

static void pio1Handler(void) {
    for (uint i = 0; i < 4; i++) {
        if (pio1_hw->irq & 1 << i) {
            pio1_hw->irq = 1 << i;
            states[i + 4].pulsewidth = pio_sm_get(pio1, i);
            states[i + 4].period = pio_sm_get(pio1, i);
            pio1_hw->irq = 1 << i;
        }
    }
}

/**
 * Sets up a PWM state machine for a single pin.
 * @param pio The PIO instance to use
 * @param offset The PIO program offset to use
 * @param pin The pin to use
 * @note This function will automatically claim and enable a state machine on the specified PIO instance,
 * or throw an error if none were available
*/
static void setup_sm(const PIO pio, const uint offset, uint pin) {
    // Find a usable state machine for this pin
    int sm = pio_claim_unused_sm(pio, false);
    if (sm >= 0) {
        if (print.fbw) printf("[pwm] pin %d assigned to state machine %d\n", pin, sm);
        // Initialize the state machine's PWMState
        states[(pio == pio0) ? sm : sm + 4].pulsewidth = 0; // Positions 0-3 are for PIO0 0-3 and positions 4-7 are for PIO1 0-3, hence the +4 offset
        states[(pio == pio0) ? sm : sm + 4].period = 0;
        states[(pio == pio0) ? sm : sm + 4].pin = pin;
        // Configure the physical pin (pull down as per PWM standard, give PIO access)
        gpio_pull_down(pin);
        pio_gpio_init(pio, pin);
        // Tell the state machine which pin we will be using
        pio_sm_config cfg = pwm_program_get_default_config(offset);
        sm_config_set_jmp_pin(&cfg, pin);
        sm_config_set_in_pins(&cfg, pin);
        // Set shift direction
        sm_config_set_in_shift(&cfg, false, false, 0);
        // Set config and enable state machine
        pio_sm_init(pio, sm, offset, &cfg);
        pio_sm_set_enabled(pio, sm, true);
    } else {
        // A state machine was not available, this is fatal because PWM manages the mode switch and control surfaces...maybe important
        log_message(FATAL, "No usable state machines available!", 500, 0, true);
    }
}

void pwm_enable(uint pin_list[], uint num_pins) {
    if (pio_can_add_program(pio0, &pwm_program)) {
        if (print.fbw) printf("[pwm] loading PWM IN into PIO0 with %d state machines\n", num_pins > 4 ? 4 : num_pins);
        uint offset = pio_add_program(pio0, &pwm_program);
        for (uint i = 0; i < (num_pins > 4 ? 4 : num_pins); i++) {
            setup_sm(pio0, offset, pin_list[i]);
        }
        irq_set_exclusive_handler(PIO0_IRQ_0, pio0Handler);
        irq_set_enabled(PIO0_IRQ_0, true);
        pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS | PIO_IRQ0_INTE_SM1_BITS | PIO_IRQ0_INTE_SM2_BITS | PIO_IRQ0_INTE_SM3_BITS;
    } else {
        log_message(FATAL, "Failed to load PIO0!", 500, 0, true);
    }

    // If there are more than 4 pins, PIO1 must also be used
    if (num_pins > 4) {
        if (pio_can_add_program(pio1, &pwm_program)) {
            if (print.fbw) printf("[pwm] loading PWM IN into PIO1 with %d state machines\n", num_pins - 4);
            uint offset = pio_add_program(pio1, &pwm_program);
            for (uint i = 4; i < num_pins; i++) {
                setup_sm(pio1, offset, pin_list[i]);
            }
            irq_set_exclusive_handler(PIO1_IRQ_0, pio1Handler);
            irq_set_enabled(PIO1_IRQ_0, true);
            pio1_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS | PIO_IRQ0_INTE_SM1_BITS | PIO_IRQ0_INTE_SM2_BITS | PIO_IRQ0_INTE_SM3_BITS;
        } else {
            log_message(FATAL, "Failed to load PIO1!", 500, 0, true);
        }
    }
    gb_num_pins = num_pins;
}


/**
 * Gets the calibration value for the specified pin.
 * @param pin the pin to get the calibration value of
 * @return the calibration value from PWM calibration.
 * Be aware that this value may not be cohesive; this function does not check to see whether or not a calibration has been done, so it is able to return random data.
*/
static inline float pwmOffsetOf(uint pin) {
    // Look up the correct value to fetch based on the pin
    uint val;
    if (pin == (uint)flash.pins[PINS_INPUT_AIL]) {
        val = PWM_OFFSET_AIL;
    } else if (pin == (uint)flash.pins[PINS_INPUT_ELEV]) {
        val = PWM_OFFSET_ELEV;
    } else if (pin == (uint)flash.pins[PINS_INPUT_RUD]) {
        val = PWM_OFFSET_RUD;
    } else if (pin == (uint)flash.pins[PINS_INPUT_SWITCH]) {
        val = PWM_OFFSET_SW;
    } else if (pin == (uint)flash.pins[PINS_INPUT_THROTTLE]) {
        val = PWM_OFFSET_THR;
    } else {
        val = PWM_OFFSET_AIL;
    }
    // Read from the correct sector based on the value
    return flash.pwm[val];
}

/**
 * Reads the raw PWM value without any calibration offsets applied.
 * @param pin the GPIO pin to read
 * @param mode the mode of the PWM
 * @return the raw degree value
*/
static inline float readRaw(uint pin, PWMMode mode) {
    // Find the GPIO pin's state machine
    for (uint8_t i = 0; i < count_of(states); i++) {
        if (states[i].pin == pin) {
            switch (mode) {
                case PWM_MODE_DEG:
                    return (180000 * ((float)states[i].pulsewidth * 0.000000016 - 0.001)); // 0-180
                case PWM_MODE_ESC:
                    return (100000 * ((float)states[i].pulsewidth * 0.000000016 - 0.001)); // 0-100
            }
        }
    }
}

float pwm_read(uint pin, PWMMode mode) {
    for (uint8_t i = 0; i < (sizeof(states) / sizeof(states[0])); i++) {
        if (states[i].pin == pin) {
            switch (mode) {
                case PWM_MODE_DEG:
                    return (180000 * ((float)states[i].pulsewidth * 0.000000016 - 0.001) + pwmOffsetOf(pin));
                case PWM_MODE_ESC:
                    return (100000 * ((float)states[i].pulsewidth * 0.000000016 - 0.001) + pwmOffsetOf(pin));
            }
        }
    }
}

bool pwm_calibrate(uint pin_list[], uint num_pins, float deviations[], uint num_samples, uint sample_delay_ms, uint run_times) {
    if (gb_num_pins < 1) return false; // Ensure PWM has been initialized
    log_message(INFO, "Calibrating PWM", 100, 0, false);
    sleep_ms(2000); // Wait a few moments for tx/rx to set itself up
    // The first position of PWM sector holds a flag to indicate that calibration has been completed;
    // subsequent values will hold the calibration data
    flash.pwm[PWM_FLAG] = FLAG_PWM;
    float calibration_data[FLOAT_SECTOR_SIZE] = {FLAG_PWM};
    for (uint i = 0; i < num_pins; i++) {
        uint pin = pin_list[i];
        if (print.fbw) printf("[pwm] calibrating pin %d (%d/%d)\n", pin, i + 1, num_pins);
        char pBar[DISPLAY_MAX_LINE_LEN] = { [0 ... DISPLAY_MAX_LINE_LEN - 1] = ' '};
        display_pBarStr(pBar, (uint)(((i + 1) * 100) / num_pins));
        display_text("Please do not", "touch the", "transmitter!", pBar, true);
        float deviation = deviations[i];
        float final_difference = 0.0f;
        bool isThrottle = pin_list[i] == (uint)flash.pins[PINS_INPUT_THROTTLE];
        for (uint t = 0; t < run_times; t++) {
            if (print.fbw) printf("[pwm] running trial %d out of %d\n", t + 1, run_times);
            float total_difference = 0.0f;
            for (uint i = 0; i < num_samples; i++) {
                total_difference += (deviation - (isThrottle ? readRaw(pin, PWM_MODE_ESC) : readRaw(pin, PWM_MODE_DEG)));
                sleep_ms(sample_delay_ms);
            }
            // Check to see if the deviation is 270 (this value occurs with a pulsewidth of 0 or 1, aka not connected)
            if ((total_difference / num_samples) == 270.0f) {
                if (print.fbw) printf("ERROR: [FBW-500] pin %d's calibration value seems abnormal, is it connected?\n", pin);
                return false;
            }
            // Add the total difference recorded divided by the samples we took (average) to the final difference
            final_difference = final_difference + (total_difference / num_samples);
        }
        // Get our final average and save it to the correct byte in our array which we write to flash
        // Any pins over 4 (thus, pins belonging to PIO1) will be in the second array
        if (print.fbw) printf("[pwm] pin %d's final offset is %f\n", pin, (final_difference / run_times));
        // Find the correct location in the array to write to
        SectorPWM loc;
        if (pin == (uint)flash.pins[PINS_INPUT_AIL]) {
            loc = PWM_OFFSET_AIL;
        } else if (pin == (uint)flash.pins[PINS_INPUT_ELEV]) {
            loc = PWM_OFFSET_ELEV;
        } else if (pin == (uint)flash.pins[PINS_INPUT_RUD]) {
            loc = PWM_OFFSET_RUD;
        } else if (pin == (uint)flash.pins[PINS_INPUT_SWITCH]) {
            loc = PWM_OFFSET_SW;
        } else if (pin == (uint)flash.pins[PINS_INPUT_THROTTLE]) {
            loc = PWM_OFFSET_THR;
        } else {
            if (print.fbw) printf("ERROR: [FBW-500] pin %d is not a valid pin to calibrate!\n", pin);
            return false;
        }
        // Check to ensure the value is within limits before adding it to be written
        if (!WITHIN_MAX_CALIBRATION_OFFSET((final_difference / run_times), flash.general[GENERAL_MAX_CALIBRATION_OFFSET])) {
            if (pin == (uint)flash.pins[PINS_INPUT_SWITCH]) {
                // The switch pin is a little special; it can have high offsets but only if they are negative, otherwise modes won't register properly
                if ((final_difference / run_times) < -200.0f || (final_difference / run_times) > flash.general[GENERAL_MAX_CALIBRATION_OFFSET]) {
                    goto error;
                }
            } else {
                goto error;
            }
            error:
                if (print.fbw) printf("ERROR: [FBW-500] pin %d's calibration value is too high!\n", pin);
                return false;
        }
        flash.pwm[loc] = final_difference / run_times;
    }
    flash.pwm[PWM_MODE] = (ControlMode)flash.general[GENERAL_CONTROL_MODE];
    if (print.fbw) printf("[pwm] writing calibration data\n");
    flash_save();
    log_clear(INFO);
    return true;
}

PWMCalibrationStatus pwm_isCalibrated() {
    // Read the calibration flag
    if (flash.pwm[PWM_FLAG] == FLAG_PWM) {
        // Ensure that the control mode we are in is the same as the one in which we calibrated
        if ((ControlMode)flash.general[GENERAL_CONTROL_MODE] != (ControlMode)flash.pwm[PWM_MODE]) {
            return PWMCALIBRATION_INVALID;
        }
        return PWMCALIBRATION_OK;
    } else {
        return PWMCALIBRATION_INCOMPLETE;
    }
}

void pwm_getPins(uint *pins, uint *num_pins, float *deviations) {
    // Consistant between all control modes
    pins[0] = (uint)flash.pins[PINS_INPUT_AIL];
    pins[1] = (uint)flash.pins[PINS_INPUT_ELEV];
    deviations[0] = 90.0f;
    deviations[1] = 90.0f;
    // Control mode specific pins
    switch ((ControlMode)flash.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
            pins[2] = (uint)flash.pins[PINS_INPUT_RUD];
            pins[3] = (uint)flash.pins[PINS_INPUT_SWITCH];
            pins[4] = (uint)flash.pins[PINS_INPUT_THROTTLE];
            deviations[2] = 90.0f; // We expect all controls to be centered except switch and throttle
            deviations[3] = 0.0f;
            deviations[4] = 0.0f;
            *num_pins = 5;
            break;
        case CTRLMODE_3AXIS:
            pins[2] = (uint)flash.pins[PINS_INPUT_RUD];
            pins[3] = (uint)flash.pins[PINS_INPUT_SWITCH];
            deviations[2] = 90.0f;
            deviations[3] = 0.0f;
            *num_pins = 4;
            break;
        case CTRLMODE_FLYINGWING_ATHR:
            pins[2] = (uint)flash.pins[PINS_INPUT_SWITCH];
            pins[3] = (uint)flash.pins[PINS_INPUT_THROTTLE];
            deviations[2] = 0.0f;
            deviations[3] = 0.0f;
            *num_pins = 4;
            break;
        case CTRLMODE_FLYINGWING:
            pins[2] = (uint)flash.pins[PINS_INPUT_SWITCH];
            deviations[2] = 0.0f;
            *num_pins = 3;
            break;
    }
}
