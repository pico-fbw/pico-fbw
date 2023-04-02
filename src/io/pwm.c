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
 * Huge thanks to 'GitJer' on GitHub for writing this PIO and giving me a starting point for the PWM input code!
 * Check them out here: https://github.com/GitJer/Some_RPI-Pico_stuff/tree/main/PwmIn/PwmIn_4pins
*/

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/irq.h"

#include "pwm.h"
#include "pwm.pio.h"

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
    // pio 0 is used
    pio = pio0;
    // load the pio program into the pio memory
    uint offset = pio_add_program(pio, &pwm_program);
    // start num_of_pins state machines
    for (int i = 0; i < num_pins; i++) {
        // prepare state machine i
        pulsewidth[i] = 0;
        period[i] = 0;

        // configure the used pins (pull down, controlled by PIO)
        gpio_pull_down(pin_list[i]);
        pio_gpio_init(pio, pin_list[i]);
        // make a sm config
        pio_sm_config c = pwm_program_get_default_config(offset);
        // set the 'jmp' pin
        sm_config_set_jmp_pin(&c, pin_list[i]);
        // set the 'wait' pin (uses 'in' pins)
        sm_config_set_in_pins(&c, pin_list[i]);
        // set shift direction
        sm_config_set_in_shift(&c, false, false, 0);
        // init the pio sm with the config
        pio_sm_init(pio, i, offset, &c);
        // enable the sm
        pio_sm_set_enabled(pio, i, true);
    }
    // set the IRQ handler
    irq_set_exclusive_handler(PIO0_IRQ_0, pwm_internal_handler);
    // enable the IRQ
    irq_set_enabled(PIO0_IRQ_0, true);
    // allow irqs from the low 4 state machines
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

float pwm_readD(uint pin) {
    return ((float)pulsewidth[pin] / (float)period[pin]);
}

float pwm_readP(uint pin) {
    // the measurements are taken with 2 clock cycles per timer tick
    // hence, it is 2*0.000000008
    return ((float)period[pin] * 0.000000016);
}

float pwm_readDeg(uint pin) {
    return (((((float)pulsewidth[pin] * 0.000000016) - 0.001) / 0.001) * 180.0f + 2);
}