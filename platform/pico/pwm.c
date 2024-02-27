/**
 * This file utilizes code under the MIT License. See "LICENSE" for details.
*/

/**
 * The PWM input code is a modification of code provided by `GitJer`: https://github.com/GitJer/Some_RPI-Pico_stuff/tree/main/PwmIn/PwmIn_4pins
 * Thanks so much for that!
 * The PWM output code is a modification of the pico-servo library by 'markushi': https://github.com/markushi/pico-servo
 * Thanks again!
*/

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "pwm.pio.h"

#include "platform/pwm.h"

// For PWM input: stores data from each state machine
typedef struct PWMState {
    u32 pulsewidth, period;
    u32 pin;
} PWMState;

static PWMState states[8];

// For PWM output: stores the wrap value for the PWM clock
static u16 wrap;

// Handles PWM input from state machines in PIO0.
// Called when an interrupt is raised by a state machine, and will read the pulsewidth and period from the state machine.
static void pio0Handler() {
    for (u32 i = 0; i < 4; i++) {
        // Check if the IRQ has been raised for this state machine
        if (pio0_hw->irq & 1 << i) {
            pio0_hw->irq = 1 << i; // Clear interrupt
            states[i].pulsewidth = pio_sm_get(pio0, i); // Read pulsewidth from FIFO
            states[i].period = pio_sm_get(pio0, i); // Read low period from the FIFO
            pio0_hw->irq = 1 << i; // Clear interrupt
        }
    }
}

// Equivalent to pio0Handler, but for PIO1 state machines.
static void pio1Handler() {
    for (u32 i = 0; i < 4; i++) {
        if (pio1_hw->irq & 1 << i) {
            pio1_hw->irq = 1 << i;
            states[i + 4].pulsewidth = pio_sm_get(pio1, i);
            states[i + 4].period = pio_sm_get(pio1, i);
            pio1_hw->irq = 1 << i;
        }
    }
}

/* You may wonder why there is a limit of seven pins even though there are eight state machines.
This is because the Pico W reserves one state machine for itself, and even though we could use the full eight on a regular Pico,
this keeps compatability between models. */

/**
 * Sets up a PWM state machine for a single pin.
 * @param pio The PIO instance to use
 * @param offset The PIO program offset to use
 * @param pin The pin to use
 * @return true if the state machine was set up successfully, false if no state machines were available
*/
static bool setup_sm(const PIO pio, const u32 offset, u32 pin) {
    gpio_set_function(pin, (pio == pio0) ? GPIO_FUNC_PIO0 : GPIO_FUNC_PIO1);
    // Find a usable state machine for this pin
    i32 sm = pio_claim_unused_sm(pio, false);
    if (sm >= 0) {
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
        return false;
    }
    return true;
}

bool pwm_setup_read(u32 pins[], u32 num_pins) {
    // Load the PWM program into all 4 PIO0 state machines
    if (pio_can_add_program(pio0, &pwm_program)) {
        u32 offset = pio_add_program(pio0, &pwm_program);
        for (u32 i = 0; i < (num_pins > 4 ? 4 : num_pins); i++) {
            if (!setup_sm(pio0, offset, pins[i])) return false;
        }
        // Set up the interrupt handler
        irq_set_exclusive_handler(PIO0_IRQ_0, pio0Handler);
        irq_set_enabled(PIO0_IRQ_0, true);
        pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS | PIO_IRQ0_INTE_SM1_BITS | PIO_IRQ0_INTE_SM2_BITS | PIO_IRQ0_INTE_SM3_BITS;
    } else return false; // Failed to load into PIO0
    // If there are more than 4 pins, PIO1 must also be used
    if (num_pins > 4) {
        if (pio_can_add_program(pio1, &pwm_program)) {
            u32 offset = pio_add_program(pio1, &pwm_program);
            for (u32 i = 4; i < num_pins; i++) {
                if (!setup_sm(pio1, offset, pins[i])) return false;
            }
            irq_set_exclusive_handler(PIO1_IRQ_0, pio1Handler);
            irq_set_enabled(PIO1_IRQ_0, true);
            pio1_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS | PIO_IRQ0_INTE_SM1_BITS | PIO_IRQ0_INTE_SM2_BITS | PIO_IRQ0_INTE_SM3_BITS;
        } else return false;
    }
    return true;
}

void pwm_setup_write(u32 pins[], u32 num_pins, u32 freq) {
    for (u32 i = 0; i < num_pins; i++) {
        gpio_set_function(pins[i], GPIO_FUNC_PWM);
        // PWM clock is system clock / 16
        pwm_config config = pwm_get_default_config();
        pwm_config_set_clkdiv(&config, 16.f);
        // Wrap is roughly determined by the frequency (won't be super precise due to hardware limitations)
        wrap = clock_get_hz(clk_sys) / (16.f * freq);
        pwm_config_set_wrap(&config, wrap);
        pwm_init(pwm_gpio_to_slice_num(pins[i]), &config, true);
    }
}

i32 pwm_read_raw(u32 pin) {
    // Find the pin's state machine
    for (u32 i = 0; i < count_of(states); i++) {
        if (states[i].pin == pin) {
            return states[i].pulsewidth;
        }
    }
    return -1; // Pin not found
}

void pwm_write_raw(u32 pin, u16 duty) {
    pwm_set_gpio_level(pin, (duty * wrap) / 255);
}
