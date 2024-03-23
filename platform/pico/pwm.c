/**
 * This file utilizes code under the MIT License. See "LICENSE" for details.
 */

/**
 * The PWM input code is a modification of code provided by `GitJer`:
 * https://github.com/GitJer/Some_RPI-Pico_stuff/tree/main/PwmIn/PwmIn_4pins Thanks so much for that!
 *
 * The PWM output code is a
 * modification of the pico-servo library by 'markushi': https://github.com/markushi/pico-servo Thanks again!
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <assert.h>
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "pwm.pio.h"

#include "platform/pwm.h"

// For PWM input

// Struct to store data from each state machine
typedef struct PWMInData {
    u32 pin;
    u32 pulsewidth, period;
} PWMInData;

static PWMInData inData[NUM_PIO_STATE_MACHINES * NUM_PIOS]; // (8)

// For PWM output:

#define SERVO_TOP_MAX (UINT16_MAX - 1) // Maximum "top" is set at 65534 to be able to achieve 100% duty with 65535.

// Array to store the frequency of each PWM slice
static u32 frequencies[NUM_PWM_SLICES]; // (8)

// Handles PWM input from state machines in PIO0.
// Called when an interrupt is raised by a state machine, and will read the pulsewidth and period from the state machine.
static void pio0Handler() {
    for (u32 i = 0; i < NUM_PIO_STATE_MACHINES; i++) {
        // Check if the IRQ has been raised for this state machine
        if (pio0_hw->irq & 1 << i) {
            pio0_hw->irq = 1 << i;                                         // Clear interrupt
            inData[i].pulsewidth = pio_sm_get(pio0, i);                    // Read pulsewidth from FIFO
            inData[i].period = pio_sm_get(pio0, i) + inData[i].pulsewidth; // Read period from FIFO (PIO only stores low period
                                                                           // so we add the pulsewidth to get the full period)
            pio0_hw->irq = 1 << i;                                         // Clear interrupt
        }
    }
}

// Equivalent to pio0Handler, but for PIO1 state machines.
static void pio1Handler() {
    for (u32 i = 0; i < NUM_PIO_STATE_MACHINES; i++) {
        if (pio1_hw->irq & 1 << i) {
            pio1_hw->irq = 1 << i;
            inData[i + NUM_PIO_STATE_MACHINES].pulsewidth = pio_sm_get(pio1, i);
            inData[i + NUM_PIO_STATE_MACHINES].period = pio_sm_get(pio1, i) + inData[i + NUM_PIO_STATE_MACHINES].pulsewidth;
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
        // Initialize the state machine's PWMInData
        // Positions 0-3 are for PIO0 0-3 and positions 4-7 are for PIO1 0-3, hence the offset
        inData[(pio == pio0) ? (u32)sm : (u32)sm + NUM_PIO_STATE_MACHINES].pin = pin;
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
    } else
        return false; // A state machine was not available
    return true;
}

bool pwm_setup_read(const u32 pins[], u32 num_pins) {
    // Load the PWM program into all 4 PIO0 state machines
    if (pio_can_add_program(pio0, &pwm_program)) {
        u32 offset = pio_add_program(pio0, &pwm_program);
        for (u32 i = 0; i < (num_pins > NUM_PIO_STATE_MACHINES ? NUM_PIO_STATE_MACHINES : num_pins); i++) {
            assert(pwm_gpio_to_channel(pins[i]) == PWM_CHAN_B); // Only PWM channel B can be used for input
            if (!setup_sm(pio0, offset, pins[i]))
                return false;
        }
        // Set up the interrupt handler
        irq_set_exclusive_handler(PIO0_IRQ_0, pio0Handler);
        irq_set_enabled(PIO0_IRQ_0, true);
        pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS | PIO_IRQ0_INTE_SM1_BITS | PIO_IRQ0_INTE_SM2_BITS | PIO_IRQ0_INTE_SM3_BITS;
    } else
        return false; // Failed to load into PIO0
    // If there are more than NUM_PIO_STATE_MACHINES (4) pins, PIO1 must also be used
    if (num_pins > NUM_PIO_STATE_MACHINES) {
        if (pio_can_add_program(pio1, &pwm_program)) {
            u32 offset = pio_add_program(pio1, &pwm_program);
            for (u32 i = NUM_PIO_STATE_MACHINES; i < num_pins; i++) {
                assert(pwm_gpio_to_channel(pins[i]) == PWM_CHAN_B);
                if (!setup_sm(pio1, offset, pins[i]))
                    return false;
            }
            irq_set_exclusive_handler(PIO1_IRQ_0, pio1Handler);
            irq_set_enabled(PIO1_IRQ_0, true);
            pio1_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS | PIO_IRQ0_INTE_SM1_BITS | PIO_IRQ0_INTE_SM2_BITS | PIO_IRQ0_INTE_SM3_BITS;
        } else
            return false;
    }
    return true;
}

bool pwm_setup_write(const u32 pins[], u32 num_pins, u32 freq) {
    for (u32 i = 0; i < num_pins; i++) {
        assert(pwm_gpio_to_channel(pins[i]) == PWM_CHAN_A || pwm_gpio_to_channel(pins[i]) == PWM_CHAN_B);
        gpio_set_function(pins[i], GPIO_FUNC_PWM);
        // Approximate a clock divider and wrap value for the PWM clock to try and match the desired frequency
        u8 slice = pwm_gpio_to_slice_num(pins[i]);
        frequencies[slice] = freq;
        u32 div16_top = 16 * clock_get_hz(clk_sys) / freq;
        u32 top = 1;
        while (true) {
            // Try a few small prime factors
            if (div16_top >= 16 * 5 && div16_top % 5 == 0 && top * 5 <= SERVO_TOP_MAX) {
                div16_top /= 5;
                top *= 5;
            } else if (div16_top >= 16 * 3 && div16_top % 3 == 0 && top * 3 <= SERVO_TOP_MAX) {
                div16_top /= 3;
                top *= 3;
            } else if (div16_top >= 16 * 2 && top * 2 <= SERVO_TOP_MAX) {
                div16_top /= 2;
                top *= 2;
            } else {
                break;
            }
        }
        if (div16_top < 16)
            return false; // Too large
        else if (div16_top >= 256 * 16)
            return false; // Too small
        pwm_hw->slice[slice].div = div16_top;
        pwm_hw->slice[slice].top = top;
        pwm_set_enabled(slice, true);
    }
    return true;
}

float pwm_read_raw(u32 pin) {
    // Find the pin's state machine
    for (u32 i = 0; i < count_of(inData); i++) {
        if (inData[i].pin == pin) {
            // Calculate the pulsewidth from the state machine's data:
            // - Multiply by 2 because PWM measurements are taken with 2 clock cycles per timer tick
            // - Divide by the system clock frequency to get the pulsewidth in seconds
            // - Multiply by 1000 to convert to μs
            return (float)inData[i].pulsewidth * 2 / clock_get_hz(clk_sys) * 1E6f;
        }
    }
    return -1.f; // Pin not found
}

void pwm_write_raw(u32 pin, float pulsewidth) {
    // Calculate the duty cycle from the given pulsewidth and period (calculated from the frequency)
    float period = 1E6f / frequencies[pwm_gpio_to_slice_num(pin)];
    pwm_set_gpio_level(pin, (u16)((pulsewidth / period) * UINT16_MAX));
}
