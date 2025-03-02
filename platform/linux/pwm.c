/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <gpiod.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "platform/defs.h"
#include "platform/gpio.h"
#include "platform/helpers.h"
#include "platform/time.h"

#include "platform/pwm.h"

#define NUM_PWM_IN_CHANNELS 8
#define NUM_PWM_OUT_CHANNELS 8
#define PWM_IN_TIMEOUT 1000    // Timeout for PWM input thread gpio events, µs
#define PWM_OUT_THREAD_PRIO 80 // Real-time priority for PWM output thread

extern struct gpiod_line_request *requests[MAX_GPIOD_REQUESTS]; // Defined in gpio.c

typedef struct PWMInChannel {
    i16 pin;
    f32 pulsewidth; // Read pulsewidth, µs
    // -- Internal state --
    struct gpiod_line_request *request; // Stored request from gpio_setup, used to reference the pin
    u64 lastRise;                       // Last rising-edge timestamp, ns
    bool active;                        // Whether or not the channel is in use
} PWMInChannel;

typedef struct PWMOutChannel {
    i16 pin;
    u64 pulsewidth; // Pulsewidth to write, µs
    // -- Internal state --
    u64 period;       // Period of the PWM signal, µs
    pthread_t thread; // Thread to handle this channel
    bool active;
} PWMOutChannel;

static PWMInChannel inChannels[NUM_PWM_IN_CHANNELS] = {[0 ... NUM_PWM_IN_CHANNELS - 1] = {.active = false}};
static pthread_t inThread = 0; // Thread to handle all PWM input channels

static PWMOutChannel outChannels[NUM_PWM_OUT_CHANNELS] = {[0 ... NUM_PWM_OUT_CHANNELS - 1] = {.active = false}};

/**
 * Busy-waits for the given number of microseconds.
 * @param us the number of microseconds to wait
 */
static inline void busy_wait_us(u64 us) {
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    while (true) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        u64 elapsed_us = (now.tv_sec - start.tv_sec) * 1000000UL + (now.tv_nsec - start.tv_nsec) / 1000UL;
        if (elapsed_us >= us) {
            break;
        }
    }
}

/**
 * Gets the corresponding PWM IN channel for the given pin, optionally creating a new one if it doesn't exist.
 * @param pin the pin to get the channel for
 * @param new whether or not to create a new channel if it doesn't exist
 * @return a pointer to the PWM IN channel representing the given pin, or NULL if none match
 */
static PWMInChannel *get_in_channel(i16 pin, bool new) {
    for (u32 i = 0; i < count_of(inChannels); i++) {
        if (inChannels[i].pin == pin) {
            return &inChannels[i];
        }
    }
    if (!new) {
        return NULL; // No channel found, and we weren't asked to create a new one
    }
    // Create a new channel
    for (u32 i = 0; i < count_of(inChannels); i++) {
        if (!inChannels[i].active) {
            inChannels[i].pin = pin;
            inChannels[i].active = true;
            return &inChannels[i];
        }
    }
    return NULL; // No available channels
}

/**
 * Gets the corresponding PWM OUT channel for the given pin, optionally creating a new one if it doesn't exist.
 * @param pin the pin to get the channel for
 * @param new whether or not to create a new channel if it doesn't exist
 * @return a pointer to the PWM OUT channel representing the given pin, or NULL if none match
 */
static PWMOutChannel *get_out_channel(i16 pin, bool new) {
    for (u32 i = 0; i < count_of(outChannels); i++) {
        if (outChannels[i].pin == pin) {
            return &outChannels[i];
        }
    }
    if (!new) {
        return NULL;
    }
    for (u32 i = 0; i < count_of(outChannels); i++) {
        if (!outChannels[i].active) {
            outChannels[i].pin = pin;
            outChannels[i].active = true;
            return &outChannels[i];
        }
    }
    return NULL;
}

// Thread to handle PWM input.
static void *pwm_in_thread(void *arg) {
    // Allocate an edge event buffer of default size
    struct gpiod_edge_event_buffer *buf = gpiod_edge_event_buffer_new(0);
    if (!buf) {
        return NULL;
    }
    size_t capacity = gpiod_edge_event_buffer_get_capacity(buf);
    while (true) {
        for (u32 i = 0; i < count_of(inChannels); i++) {
            PWMInChannel *channel = &inChannels[i];
            if (!channel->active || !channel->request) {
                continue;
            }
            // Wait for an event
            if (gpiod_line_request_wait_edge_events(channel->request, PWM_IN_TIMEOUT * 1000L) != 1) {
                continue;
            }
            int events = gpiod_line_request_read_edge_events(channel->request, buf, capacity);
            for (u64 j = 0; (i64)j < events; j++) {
                struct gpiod_edge_event *event = gpiod_edge_event_buffer_get_event(buf, j);
                if (gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_RISING_EDGE) {
                    // Record rising edge
                    channel->lastRise = gpiod_edge_event_get_timestamp_ns(event);
                } else {
                    // Falling edge, calculate pulsewidth
                    u64 fall = gpiod_edge_event_get_timestamp_ns(event);
                    if (channel->lastRise != 0 && fall > channel->lastRise) {
                        channel->pulsewidth = (fall - channel->lastRise) / 1E3f;
                    }
                }
            }
        }
    }
    gpiod_edge_event_buffer_free(buf);
    return NULL;
    (void)arg;
}

// Thread to handle PWM output.
static void *pwm_out_thread(void *arg) {
    while (true) {
        PWMOutChannel *channel = (PWMOutChannel *)arg;
        if (!channel->active || channel->pulsewidth == 0) {
            sched_yield();
            continue;
        }
        // Begin the PWM pulse
        gpio_set(channel->pin, STATE_HIGH);
        // Wait for the pulsewidth to elapse, then turn off the signal
        // Use busy-wait instead of sleep to avoid jitter
        busy_wait_us(channel->pulsewidth);
        gpio_set(channel->pin, STATE_LOW);
        // The off period is much longer and therefore timing is less critical,
        // so we can afford to sleep and free some CPU time
        sleep_us_blocking(channel->period - channel->pulsewidth);
    }
    return NULL;
}

bool pwm_setup_read(const i16 pins[], u32 num_pins) {
    for (u32 i = 0; i < num_pins; i++) {
        PWMInChannel *channel = get_in_channel(pins[i], true);
        // Enable GPIO input w/ edge detection
        gpio_setup(channel->pin, MODE_INPUT_EDGEDET);
        channel->request = requests[channel->pin];
    }
    // inThread only needs to be created once, even if more channels are added later
    if (inThread) {
        return true;
    }
    return pthread_create(&inThread, NULL, pwm_in_thread, NULL) == 0;
}

bool pwm_setup_write(const i16 pins[], u32 num_pins, u32 freq) {
    for (u32 i = 0; i < num_pins; i++) {
        PWMOutChannel *channel = get_out_channel(pins[i], true);
        channel->pulsewidth = 0UL; // Default zero pulsewidth/off
        channel->period = 1000000UL / freq;
        gpio_setup(channel->pin, MODE_OUTPUT);
        // A thread with real-time priority needs to be created for each channel
        pthread_attr_t attr;
        struct sched_param param;
        pthread_attr_init(&attr);
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        param.sched_priority = PWM_OUT_THREAD_PRIO;
        pthread_attr_setschedparam(&attr, &param);
        int created = pthread_create(&channel->thread, NULL, pwm_out_thread, channel);
        pthread_attr_destroy(&attr);
        if (created != 0) {
            return false;
        }
    }
    return true;
}

f32 pwm_read_raw(i16 pin) {
    PWMInChannel *channel = get_in_channel(pin, false);
    if (!channel) {
        return -1.f;
    }
    return channel->pulsewidth;
}

void pwm_write_raw(i16 pin, f32 pulsewidth) {
    PWMOutChannel *channel = get_out_channel(pin, false);
    if (!channel) {
        return;
    }
    channel->pulsewidth = (u64)pulsewidth;
}
