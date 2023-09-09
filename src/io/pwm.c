/**
 * This file utilizes code under the MIT License. See "LICENSE" for details.
*/

/**
 * Huge thanks to 'GitJer' on GitHub for most of the PWM input code!
 * Check them out here: https://github.com/GitJer/Some_RPI-Pico_stuff/tree/main/PwmIn/PwmIn_4pins
*/

/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "pico/time.h"
#include "pico/types.h"

#include "hardware/irq.h"
#include "hardware/pio.h"

#include "error.h"
#include "flash.h"
#include "../config.h"

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
static void setup_sm(PIO pio, uint offset, uint pin) {
    // Find a usable state machine for this pin
    int sm = pio_claim_unused_sm(pio, false);
    if (sm >= 0) {
        FBW_DEBUG_printf("[pwm] pin %d assigned to state machine %d\n", pin, sm);
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
        error_throw(ERROR_PWM, ERROR_LEVEL_FATAL, 500, 0, true, "No usable state machines were available for PWM!");
    }
}

void pwm_enable(uint pin_list[], uint num_pins) {
    if (pio_can_add_program(pio0, &pwm_program)) {
        FBW_DEBUG_printf("[pwm] loading PWM IN into PIO0 with %d state machines\n", num_pins > 4 ? 4 : num_pins);
        uint offset = pio_add_program(pio0, &pwm_program);
        for (uint i = 0; i < (num_pins > 4 ? 4 : num_pins); i++) {
            setup_sm(pio0, offset, pin_list[i]);
        }
        irq_set_exclusive_handler(PIO0_IRQ_0, pio0Handler);
        irq_set_enabled(PIO0_IRQ_0, true);
        pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS | PIO_IRQ0_INTE_SM1_BITS | PIO_IRQ0_INTE_SM2_BITS | PIO_IRQ0_INTE_SM3_BITS;
    } else {
        error_throw(ERROR_PWM, ERROR_LEVEL_FATAL, 500, 0, true, "Failed to load PWM program into PIO0!");
    }

    // If there are more than 4 pins, PIO1 must also be used
    if (num_pins > 4) {
        if (pio_can_add_program(pio1, &pwm_program)) {
            FBW_DEBUG_printf("[pwm] loading PWM IN into PIO1 with %d state machines\n", num_pins - 4);
            uint offset = pio_add_program(pio1, &pwm_program);
            for (uint i = 4; i < num_pins; i++) {
                setup_sm(pio1, offset, pin_list[i]);
            }
            irq_set_exclusive_handler(PIO1_IRQ_0, pio1Handler);
            irq_set_enabled(PIO1_IRQ_0, true);
            pio1_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS | PIO_IRQ0_INTE_SM1_BITS | PIO_IRQ0_INTE_SM2_BITS | PIO_IRQ0_INTE_SM3_BITS;
        } else {
            error_throw(ERROR_PWM, ERROR_LEVEL_FATAL, 500, 0, true, "Failed to load PWM program into PIO1!");
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
    switch (pin) {
        case INPUT_AIL_PIN:
            val = 1; // Start at 1 so we don't read the flag and index correctly
            break;
        case INPUT_ELEV_PIN:
            val = 2;
            break;
        case INPUT_RUD_PIN:
            val = 3;
            break;
        case INPUT_THR_PIN:
            val = 4;
            break;
        case INPUT_SW_PIN:
            val = 5;
            break;
        default:
            val = 1;
    }
    // Read from the correct sector based on the value
    return flash_read(FLASH_SECTOR_PWM, val);
}

/**
 * Reads the raw degree value without any calibration offsets applied.
 * @param pin the GPIO pin to read
 * @return the raw degree value
*/
static inline float readDegRaw(uint pin) {
    // Find the GPIO pin's state machine
    for (uint8_t i = 0; i < (sizeof(states) / sizeof(states[0])); i++) {
        if (states[i].pin == pin) {
            return (180000 * ((float)states[i].pulsewidth * 0.000000016 - 0.001));
        }
    }
}

float pwm_readDeg(uint pin) {
    for (uint8_t i = 0; i < (sizeof(states) / sizeof(states[0])); i++) {
        if (states[i].pin == pin) {
            return (180000 * ((float)states[i].pulsewidth * 0.000000016 - 0.001) + pwmOffsetOf(pin));
        }
    }
}

bool pwm_calibrate(uint pin_list[], uint num_pins, float deviations[], uint num_samples, uint sample_delay_ms, uint run_times) {
    FBW_DEBUG_printf("[pwm] starting pwm calibration\n");
    if (gb_num_pins < 1) return false; // Ensure PWM has been initialized
    error_throw(ERROR_PWM, ERROR_LEVEL_STATUS, 100, 0, false, ""); // Start blinking LED to signify we are calibrating
    // Create an array where we will arrange our data to later write
    // The first position holds a flag to indicate that calibration has been completed; subsequent values will hold the calibration data
    float calibration_data[CONFIG_SECTOR_SIZE] = {FLAG_PWM};
    for (uint i = 0; i < num_pins; i++) {
        uint pin = pin_list[i];
        FBW_DEBUG_printf("[pwm] calibrating pin %d (%d/%d)\n", pin, i + 1, num_pins);
        float deviation = deviations[i];
        float final_difference = 0.0f;
        for (uint t = 0; t < run_times; t++) {
            FBW_DEBUG_printf("[pwm] running trial %d out of %d\n", t + 1, run_times);
            float total_difference = 0.0f;
            for (uint i = 0; i < num_samples; i++) {
                total_difference += (deviation - readDegRaw(pin));
                sleep_ms(sample_delay_ms);
            }
            // Check to see if the deviation is 270 (this value occurs with a pulsewidth of 0 or 1, aka not connected)
            if ((total_difference / num_samples) == 270.0f) {
                FBW_DEBUG_printf("ERROR: [FBW-500] pin %d's calibration value seems abnormal, is it connected?\n", pin);
                return false;
            }
            // Add the total difference recorded divided by the samples we took (average) to the final difference
            final_difference = final_difference + (total_difference / num_samples);
        }
        // Get our final average and save it to the correct byte in our array which we write to flash
        // Any pins over 4 (thus, pins belonging to PIO1) will be in the second array
        FBW_DEBUG_printf("[pwm] pin %d's final offset is %f\n", pin, (final_difference / run_times));
        // Find the correct location in the array to write to
        uint loc;
        switch (pin) {
            case INPUT_AIL_PIN:
                loc = 1;
                break;
            case INPUT_ELEV_PIN:
                loc = 2;
                break;
            case INPUT_RUD_PIN:
                loc = 3;
                break;
            case INPUT_THR_PIN:
                loc = 5;
                break;
            case INPUT_SW_PIN:
                loc = 4;
                break;
            default:
                FBW_DEBUG_printf("ERROR: [FBW-500] pin %d is not a valid pin to calibrate!\n", pin);
                return false;
        }
        // Check to ensure the value is within limits before adding it to be written
        if (!WITHIN_MAX_CALIBRATION_OFFSET(final_difference / run_times)) {
            if (pin == INPUT_SW_PIN) {
                // The switch pin is a little special; it can have high offsets but only if they are negative, otherwise modes won't register properly
                if ((final_difference / run_times) < -200.0f || (final_difference / run_times) > MAX_CALIBRATION_OFFSET) {
                    goto error;
                }
            } else {
                goto error;
            }
            error:
                FBW_DEBUG_printf("ERROR: [FBW-500] pin %d's calibration value is too high!\n", pin);
                return false;
        }
        calibration_data[loc] = final_difference / run_times;
    }
    // Write the current control mode that was used when calibrating data so the data can later be validated
    #if defined(CONTROL_3AXIS)
        #ifdef ATHR_ENABLED
            calibration_data[6] = CTRLMODE_3AXIS_ATHR;
        #else
            calibration_data[6] = CTRLMODE_3AXIS;
        #endif
    #elif defined(CONTROL_FLYINGWING)
        #ifdef ATHR_ENABLED
            calibration_data[6] = CTRLMODE_FLYINGWING_ATHR;
        #else
            calibration_data[6] = CTRLMODE_FLYINGWING;
        #endif
    #endif
    FBW_DEBUG_printf("[pwm] writing calibration data to flash\n");
    flash_write(FLASH_SECTOR_PWM, calibration_data);
    error_clear(ERROR_PWM, false);
    return true;
}

int pwm_isCalibrated() {
    // Read the calibration flag
    if (flash_read(FLASH_SECTOR_PWM, 0) == FLAG_PWM) {
        // Ensure the values are within bounds
        uint pins[] = {INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_RUD_PIN, INPUT_SW_PIN, INPUT_THR_PIN};
        uint num_pins = (sizeof(pins) / sizeof(pins[0]));
        for (uint i = 0; i < num_pins; i++) {
            if (!WITHIN_MAX_CALIBRATION_OFFSET(pwmOffsetOf(pins[i]))) {
                if (pwmOffsetOf(pins[i]) == INPUT_SW_PIN) {
                    if (pwmOffsetOf(pins[i]) < -200.0f || pwmOffsetOf(pins[i]) > MAX_CALIBRATION_OFFSET) {
                        return -2;
                    }
                } else {
                    return -2;
                }
            }
        }
        // Finally, ensure that the control mode we are in is the same as the one in which we calibrated
        ControlMode currentControlMode;
        #if defined(CONTROL_3AXIS)
            #ifdef ATHR_ENABLED
                currentControlMode = CTRLMODE_3AXIS_ATHR;
            #else
                currentControlMode = CTRLMODE_3AXIS;
            #endif
        #elif defined(CONTROL_FLYINGWING)
            #ifdef ATHR_ENABLED
                currentControlMode = CTRLMODE_FLYINGWING_ATHR;
            #else
                currentControlMode = CTRLMODE_FLYINGWING;
            #endif
        #endif
        if (currentControlMode != flash_read(FLASH_SECTOR_PWM, 6)) {
            return -3;
        }
        // All checks have passed
        return 0;
    } else {
        // Usually, this will turn out to be either 0 if the flash has not been programmed yet or 1 if it has been previously erased/reset
        return -1;
    }
}
