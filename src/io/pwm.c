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

#include "hardware/pio.h"
#include "hardware/irq.h"

#include "error.h"
#include "flash.h"
#include "../config.h"

#include "pwm.h"
#include "pwm.pio.h"

static uint32_t pulsewidth[8], period[8];
uint gb_num_pins = 0;

static void pio0Handler(void) {
    // Check which IRQ was raised
    for (int i = 0; i < 4; i++) {
        if (pio0_hw->irq & 1 << i) {
            pio0_hw->irq = 1 << i; // Clear interrupt
            pulsewidth[i] = pio_sm_get(pio0, i); // Read pulsewidth from FIFO
            period[i] = pio_sm_get(pio0, i); // Read low period from the FIFO
            pio0_hw->irq = 1 << i; // Clear interrupt
        }
    }
}

static void pio1Handler(void) {
    for (int i = 0; i < 4; i++) {
        if (pio1_hw->irq & 1 << i) {
            pio1_hw->irq = 1 << i;
            pulsewidth[i + 4] = pio_sm_get(pio1, i);
            period[i + 4] = pio_sm_get(pio1, i);
            pio1_hw->irq = 1 << i;
        }
    }
}

/**
 * Sets up a PWM state machine for a single pin.
 * @param pio The PIO instance to use
 * @param offset The PIO program offset to use
 * @param pin The pin to use
 * @param sm The state machine to use
*/
static void setup_sm(PIO pio, uint offset, uint pin, uint sm) {
    FBW_DEBUG_printf("[pwm] preparing state machine %d on pin %d\n", sm, pin);
    pulsewidth[sm] = 0;
    period[sm] = 0;
    gpio_pull_down(pin);
    pio_gpio_init(pio, pin);
    pio_sm_config cfg = pwm_program_get_default_config(offset);
    sm_config_set_jmp_pin(&cfg, pin);
    sm_config_set_in_pins(&cfg, pin);
    sm_config_set_in_shift(&cfg, false, false, 0);
    pio_sm_init(pio, sm, offset, &cfg);
    pio_sm_set_enabled(pio, sm, true);
}

void pwm_enable(uint *pin_list, uint num_pins) {
    FBW_DEBUG_printf("[pwm] loading PWM IN into PIO0 with %d state machines\n", num_pins > 4 ? 4 : num_pins);
    uint offset0 = pio_add_program(pio0, &pwm_program);
    for (uint i = 0; i < (num_pins > 4 ? 4 : num_pins); i++) {
        setup_sm(pio0, offset0, pin_list[i], i);
    }
    irq_set_exclusive_handler(PIO0_IRQ_0, pio0Handler);
    irq_set_enabled(PIO0_IRQ_0, true);
    pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS | PIO_IRQ0_INTE_SM1_BITS | PIO_IRQ0_INTE_SM2_BITS | PIO_IRQ0_INTE_SM3_BITS;
    
    // If there are more than 4 pins, PIO1 must also be used
    if (num_pins > 4) {
        FBW_DEBUG_printf("[pwm] loading PWM IN into PIO1 with %d state machines\n", num_pins - 4);
        uint offset1 = pio_add_program(pio1, &pwm_program);
        for (uint i = 4; i < num_pins; i++) {
            setup_sm(pio1, offset1, pin_list[i], i - 4);
        }
        irq_set_exclusive_handler(PIO1_IRQ_0, pio1Handler);
        irq_set_enabled(PIO1_IRQ_0, true);
        pio1_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS | PIO_IRQ0_INTE_SM1_BITS | PIO_IRQ0_INTE_SM2_BITS | PIO_IRQ0_INTE_SM3_BITS;
    }
    gb_num_pins = num_pins;
}

float pwm_readPulseWidth(uint pin) {
    // the measurements are taken with 2 clock cycles per timer tick
    // hence, it is 2*0.000000008
    return ((float)pulsewidth[pin] * 0.000000016);
}

float pwm_readDutyCycle(uint pin) {
    return ((float)pulsewidth[pin] / (float)period[pin]);
}

float pwm_readPeriod(uint pin) {
    // the measurements are taken with 2 clock cycles per timer tick
    // hence, it is 2*0.000000008
    return ((float)period[pin] * 0.000000016);
}

/**
 * Gets the calibration value for the specified pin.
 * @param pin the pin (0-7) to get the value of
 * @return the calibration value from PWM calibration.
 * Be aware that this value may not be cohesive; this function does not check to see whether or not a calibration has been done, so it is able to return random data.
*/
static inline float pwmOffsetOf(uint pin) {
    // Check if we originally only calibrated one PWM and if so, set pin to that regardless
    #ifndef CONFIGURE_INPUTS_SEPERATELY
        pin = 0;
    #endif
    // Read from the correct sector based on the pin
    if (pin <= 3) {
        return flash_read(FLASH_SECTOR_PWM0, pin + 1); // Read +1 to skip the flag (index correctly)
    } else {
        return flash_read(FLASH_SECTOR_PWM1, pin - 3); // Read -3 to index correctly
    }
}

/**
 * Reads the raw degree value without any offsets applied.
 * @param pin the pin (0-7) to read
 * @return the raw degree value
*/
static float readDegRaw(uint pin) {
    return (180000 * ((float)pulsewidth[pin] * 0.000000016 - 0.001));
}

float pwm_readDeg(uint pin) {
    return (180000 * ((float)pulsewidth[pin] * 0.000000016 - 0.001) + pwmOffsetOf(pin));
}

bool pwm_calibrate(float deviations[], uint num_samples, uint sample_delay_ms, uint run_times) {
    FBW_DEBUG_printf("[pwm] starting pwm calibration\n");
    if (gb_num_pins < 1) return false; // Ensure PWM has been initialized
    error_throw(ERROR_PWM, ERROR_LEVEL_STATUS, 100, 0, false, ""); // Start blinking LED to signify we are calibrating
    // Create arrays where we will arrange our data to later write
    float calibration_data0[CONFIG_SECTOR_SIZE] = {FLAG_PWM}; // The first position holds a flag to indicate that calibration has been completed
    float calibration_data1[CONFIG_SECTOR_SIZE] = {FLAG_PWM};
    #ifdef CONFIGURE_INPUTS_SEPERATELY
        // If we are calibrating per pin, register all the pins we want to calibrate
        uint num_pins = gb_num_pins;  
    #else
        // If not, only register pin 0 to be calibrated
        uint num_pins = 1;
    #endif
    for (uint pin = 0; pin < num_pins; pin++) {
        FBW_DEBUG_printf("[pwm] calibrating pin %d out of %d\n", pin, num_pins);
        float deviation = deviations[pin];
        float final_difference = 0.0f;
        for (uint t = 0; t < run_times; t++) {
            FBW_DEBUG_printf("[pwm] running trial %d out of %d\n", t, run_times);
            float total_difference = 0.0f;
            for (uint i = 0; i < num_samples; i++) {
                total_difference += (deviation - readDegRaw(pin));
                sleep_ms(sample_delay_ms);
            }
            // Check to see if the deviation is 270 (this value occurs with a pulsewidth of 0/1 aka not connected)
            if ((total_difference / num_samples) == 270.0f) {
                FBW_DEBUG_printf("ERROR: [FBW-500] pin %d's calibration value seems abnormal, is it connected?\n", pin);
                return false;
            }
            // Add the total difference recorded divided by the samples we took (average) to the final difference
            final_difference = final_difference + (total_difference / num_samples);
        }
        // Get our final average and save it to the correct byte in our array which we write to flash
        // Any pins over 4 (thus, pins belonging to PIO1) will be in the second array
        FBW_DEBUG_printf("[pwm] pin %d final offset is %f\n", pin, (final_difference / run_times));
        if (num_pins > 4) {
            calibration_data1[pin - 3] = final_difference / run_times;
        } else {
            calibration_data0[pin + 1] = final_difference / run_times;
        }
    }
    // Check values one last time and then write to flash
    for (uint8_t i = 0; i < (num_pins > 4 ? 4 : num_pins); i++) {
        if (!WITHIN_MAX_CALIBRATION_OFFSET(calibration_data0[i + 1])) {
            FBW_DEBUG_printf("ERROR: [FBW-500] calibration value %d is too high!\n", i + 1);
            return false;
        }
    }
    if (num_pins > 4) {
        for (uint8_t i = 4; i < num_pins; i++) {
            if (!WITHIN_MAX_CALIBRATION_OFFSET(calibration_data1[i - 3])) {
                FBW_DEBUG_printf("ERROR: [FBW-500] calibration value %d is too high!\n", i + 1);
                return false;
            }
        }
    }
    FBW_DEBUG_printf("[pwm] writing calibration data to flash\n");
    flash_write(FLASH_SECTOR_PWM0, calibration_data0);
    if (num_pins > 4) flash_write(FLASH_SECTOR_PWM1, calibration_data1);
    error_clear(ERROR_PWM, false);
    return true;
}

int pwm_isCalibrated() {
    // Read the calibration flags
    if (flash_read(FLASH_SECTOR_PWM0, 0) == FLAG_PWM && flash_read(FLASH_SECTOR_PWM1, 0) == FLAG_PWM) {
        // If we know how many pins there are, ensure the values are normal before we give the okay
        if (gb_num_pins > 0) {
            for (uint8_t i = 0; i <= (gb_num_pins - 1); i++) {
                if (!WITHIN_MAX_CALIBRATION_OFFSET(pwmOffsetOf(i))) {
                    return -2;
                }
            }
        }
        return 0;
    } else {
        // Usually, this will turn out to be either 0 if the flash has not been programmed yet or 1 if it has been previously erased/reset
        return -1;
    }
}
