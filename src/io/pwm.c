/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Markus Hintersteiner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * **/

/**
 * Huge thanks to 'GitJer' on GitHub for giving me a starting point for the PWM input code!
 * Check them out here: https://github.com/GitJer/Some_RPI-Pico_stuff/tree/main/PwmIn/PwmIn_4pins
*/

#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/irq.h"

#include "led.h"
#include "flash.h"
#include "../config.h"

#include "pwm.h"
#include "pwm.pio.h"

/* Begin PWM input functions */

static PIO pio;
static uint32_t pulsewidth[4], period[4];
uint gb_num_pins;

static void pwm_internal_handler() {
    int state_machine = -1;
    // check which IRQ was raised:
    for (int i = 0; i < 4; i++) {
        if (pio0_hw->irq & 1<<i) {
            // clear interrupt
            pio0_hw->irq = 1 << i;
            // read pulse width from the FIFO
            pulsewidth[i] = pio_sm_get(pio, i);
            // read low period from the FIFO
            period[i] = pio_sm_get(pio, i);
            // clear interrupt
            pio0_hw->irq = 1 << i;
        }
    }
}

void pwm_enable(uint *pin_list, uint num_pins) {
    gb_num_pins = num_pins;
    pio = pio0;
    FBW_DEBUG_printf("[pwm] loading PWM IN pio into pio0\n");
    uint offset = pio_add_program(pio, &pwm_program);
    FBW_DEBUG_printf("[pwm] starting %d state machines\n", num_pins);
    for (int i = 0; i < num_pins; i++) {
        FBW_DEBUG_printf("[pwm] preparing state machine %d\n", i);
        pulsewidth[i] = 0;
        period[i] = 0;
        FBW_DEBUG_printf("[pwm] giving PIO control of pin %d\n", pin_list[i]);
        gpio_pull_down(pin_list[i]);
        pio_gpio_init(pio, pin_list[i]);
        FBW_DEBUG_printf("[pwm] setting state machine config\n");
        pio_sm_config c = pwm_program_get_default_config(offset);
        FBW_DEBUG_printf("[pwm] setting state machine pins\n");
        sm_config_set_jmp_pin(&c, pin_list[i]);
        sm_config_set_in_pins(&c, pin_list[i]);
        sm_config_set_in_shift(&c, false, false, 0);
        FBW_DEBUG_printf("[pwm] initializing and enabling state machine\n");
        pio_sm_init(pio, i, offset, &c);
        pio_sm_set_enabled(pio, i, true);
    }
    FBW_DEBUG_printf("[pwm] state machines ok\n");
    FBW_DEBUG_printf("[pwm] setting up and enabling PIO interrupts\n");
    irq_set_exclusive_handler(PIO0_IRQ_0, pwm_internal_handler);
    irq_set_enabled(PIO0_IRQ_0, true);
    pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS | PIO_IRQ0_INTE_SM1_BITS | PIO_IRQ0_INTE_SM2_BITS | PIO_IRQ0_INTE_SM3_BITS ;
}

void pwm_read(float *readings, uint pin) {
    if (pin < gb_num_pins) {
        // determine whole period
        period[pin] += pulsewidth[pin];
        // the measurements are taken with 2 clock cycles per timer tick
        // hence, it is 2*0.000000008
        *(readings + 0) = (float)pulsewidth[pin] * 2 * 0.000000008;
        *(readings + 1) = (float)period[pin] * 2 * 0.000000008;
        *(readings + 2) = ((float)pulsewidth[pin] / (float)period[pin]);
        pulsewidth[pin] = 0;
        period[pin] = 0;
    }
}

float pwm_readPW(uint pin) {
    // the measurements are taken with 2 clock cycles per timer tick
    // hence, it is 2*0.000000008
    return ((float)pulsewidth[pin] * 0.000000016);
}

float pwm_readDC(uint pin) {
    return ((float)pulsewidth[pin] / (float)period[pin]);
}

float pwm_readP(uint pin) {
    // the measurements are taken with 2 clock cycles per timer tick
    // hence, it is 2*0.000000008
    return ((float)period[pin] * 0.000000016);
}

float pwm_readDeg(uint pin) {
    return (180000 * ((float)pulsewidth[pin] * 0.000000016 - 0.001) + pwm_getCalibrationValue(pin));
}

// Function to read the raw degree value without any calibrations applied, only used internally in calibration for now.
static float pwm_readDegRaw(uint pin) {
    return (180000 * ((float)pulsewidth[pin] * 0.000000016 - 0.001));
}


bool pwm_calibrate(float deviation, uint num_samples, uint sample_delay_ms, uint run_times) {
    FBW_DEBUG_printf("[pwm] pwm calibration begin\n");
    // Start blinking LED to signify we are calibrating
    led_blink(100);
    // Create an array where we will arrange our data to later write
    float calibration_data[CONFIG_SECTOR_SIZE];
    // The first four bytes will signify if we have run a calibration before, a value of 0.5 corresponds to true in this case so we add that to the array
    calibration_data[0] = 0.5f;
    // Check if we are calibrating per pin or not
    #ifdef CONFIGURE_INPUTS_SEPERATELY
        // If yes, complete the calibration for all pins
        uint num_pins = 4;
    #else
        // If not, only register pin 0 to calibrate 
        uint num_pins = 1;
    #endif
    for (uint pin = 0; pin < num_pins; pin++) {
        FBW_DEBUG_printf("[pwm] calibrating pin %d out of %d\n", pin, num_pins);
        // If we are testing the switch, set the deviation to zero so it works for both a two and three-pos switch
        if (pin == 3) {
            deviation = 0.0f;
        }
        // Reset the final difference for every pin we test
        float final_difference = 0.0f;
        // These loops simply poll the specified pin based on the specifications we have provided in the function
        for (uint t = 0; t < run_times; t++) {
            FBW_DEBUG_printf("[pwm] running trial %d out of %d\n", t, run_times);
            // Reset the total difference every time we run
            float total_difference = 0.0f;
            for (uint i = 0; i < num_samples; i++) {
                total_difference += (deviation - pwm_readDegRaw(pin));
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
        FBW_DEBUG_printf("[pwm] pin %d final offset is %f\n", pin, (final_difference / run_times));
        calibration_data[pin + 1] = final_difference / run_times;
    }
    // Before we write, make sure values aren't way out of spec
    if (calibration_data[1] > MAX_CALIBRATION_OFFSET || calibration_data[1] < -MAX_CALIBRATION_OFFSET || calibration_data[2] > MAX_CALIBRATION_OFFSET || calibration_data[2] < -MAX_CALIBRATION_OFFSET || calibration_data[3] > MAX_CALIBRATION_OFFSET || calibration_data[3] < -MAX_CALIBRATION_OFFSET || calibration_data[4] > MAX_CALIBRATION_OFFSET || calibration_data[4] < -MAX_CALIBRATION_OFFSET) {
        FBW_DEBUG_printf("ERROR: [FBW-500] a calibration value is too high!\n");
        return false;
    }
    // Write calibration data to sector "0", last sector of flash
    FBW_DEBUG_printf("[pwm] writing calibration data to flash\n");
    flash_write(0, calibration_data);
    led_blink_stop();
    return true;
}

bool pwm_checkCalibration() {
    // Read the first value from the first sector of flash, this holds the value that is changed when calibration is completed, and
    // return true/false based on what value we actually saw vs. what we expect
    if (flash_read(0, 0) == 0.5f) {
        return true;
    // Usually, this will turn out to be either 0 if the flash has not been programmed yet or 1 if it has been previously erased/reset
    } else {
        return false;
    }
}

float pwm_getCalibrationValue(uint pin) {
    // Check if we originally only calibrated one PWM and if so, set pin to that regardless
    #ifndef CONFIGURE_INPUTS_SEPERATELY
        pin = 0;
    #endif
    // Read the value of pin + 1 from the first sector, this will ensure we don't read the calibration flag instead
    return flash_read(0, pin + 1);
}
