/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "platform/gpio.h"
#include "platform/helpers.h"
#include "platform/time.h"

#include "platform/pwm.h"

#define PWM_IN_HZ 1000 // Rate at which the PWM input thread should read signals
#define NUM_PWM_IN_CHANNELS 8
#define NUM_PWM_OUT_CHANNELS 8

typedef struct PWMInChannel {
    i16 pin;
    f32 pulsewidth; // read pulsewidth, µs
    // -- Internal state --
    PinState lastState;       // last state of the pin
    struct timespec riseEdge; // last rising edge timestamp
    bool active;              // whether or not the channel is in use
} PWMInChannel;

typedef struct PWMOutChannel {
    i16 pin;
    f32 pulsewidth; // pulsewidth to write, µs
    u64 period;     // period of the PWM signal, ns
    bool active;
} PWMOutChannel;

static PWMInChannel inChannels[NUM_PWM_IN_CHANNELS] = {[0 ... NUM_PWM_IN_CHANNELS - 1] = {.active = false}};
static pthread_t inThread; // Thread to handle all PWM input channels
static u32 inSleep;        // Amount of time to sleep inThread thread between operations, µs

static PWMOutChannel outChannels[NUM_PWM_OUT_CHANNELS] = {[0 ... NUM_PWM_OUT_CHANNELS - 1] = {.active = false}};
static pthread_t outThread;

/**
 * @return a pointer to the PWM IN channel representing the given pin, or NULL if none match
 */
static PWMInChannel *get_in_channel(i16 pin) {
    for (u32 i = 0; i < count_of(inChannels); i++) {
        if (inChannels[i].pin == pin) {
            return &inChannels[i];
        }
    }
    return NULL;
}

/**
 * @return a pointer to the PWM OUT channel representing the given pin, or NULL if none match
 */
static PWMOutChannel *get_out_channel(i16 pin) {
    for (u32 i = 0; i < count_of(outChannels); i++) {
        if (outChannels[i].pin == pin) {
            return &outChannels[i];
        }
    }
    return NULL;
}

// Thread to handle PWM input.
static void *pwm_in_thread(void *arg) {
    while (true) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        for (u32 i = 0; i < NUM_PWM_IN_CHANNELS; i++) {
            PWMInChannel *channel = &inChannels[i];
            if (!channel->active) {
                continue;
            }
            PinState currentState = gpio_state(channel->pin);
            if (channel->lastState == STATE_LOW && currentState == STATE_HIGH) {
                // Detected rising edge
                channel->riseEdge = now;
            } else if (channel->lastState == STATE_HIGH && currentState == STATE_LOW) {
                // Detected falling edge, compute the difference in microseconds.
                channel->pulsewidth =
                    (now.tv_sec - channel->riseEdge.tv_sec) * 1E6 + (now.tv_nsec - channel->riseEdge.tv_nsec) / 1E3;
            }
            channel->lastState = currentState;
        }
        sleep_us_blocking(inSleep);
    }
    return NULL;
    (void)arg;
}

// Thread to handle PWM output.
static void *pwm_out_thread(void *arg) {
    while (true) {
        for (u32 i = 0; i < NUM_PWM_OUT_CHANNELS; i++) {
            PWMOutChannel *channel = &outChannels[i];
            if (!channel->active) {
                continue;
            }
            gpio_set(channel->pin, STATE_HIGH);
            sleep_us_blocking(channel->pulsewidth);
            gpio_set(channel->pin, STATE_LOW);
            sleep_us_blocking(channel->period - channel->pulsewidth);
        }
    }
    return NULL;
    (void)arg;
}

bool pwm_setup_read(const i16 pins[], u32 num_pins) {
    assert(num_pins <= NUM_PWM_IN_CHANNELS);
    for (u32 i = 0; i < num_pins; i++) {
        PWMInChannel *channel = &inChannels[i];
        channel->pin = pins[i];
        channel->active = true;
        gpio_setup(channel->pin, MODE_INPUT);
    }
    inSleep = 1E9 / PWM_IN_HZ;
    return pthread_create(&inThread, NULL, pwm_in_thread, NULL) == 0;
}

bool pwm_setup_write(const i16 pins[], u32 num_pins, u32 freq) {
    assert(num_pins <= NUM_PWM_OUT_CHANNELS);
    for (u32 i = 0; i < num_pins; i++) {
        PWMOutChannel *channel = &outChannels[i];
        channel->pin = pins[i];
        channel->pulsewidth = 0; // Default zero pulsewidth
        channel->period = 1E9 / freq;
        channel->active = true;
        gpio_setup(channel->pin, MODE_OUTPUT);
    }
    return pthread_create(&outThread, NULL, pwm_out_thread, NULL) == 0;
}

f32 pwm_read_raw(i16 pin) {
    PWMInChannel *channel = get_in_channel(pin);
    if (!channel) {
        return -1.f;
    }
    return channel->pulsewidth;
}

void pwm_write_raw(i16 pin, f32 pulsewidth) {
    PWMOutChannel *channel = get_out_channel(pin);
    if (!channel) {
        return;
    }
    channel->pulsewidth = pulsewidth;
}
