/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

// https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/peripherals/mcpwm.html
#include "driver/mcpwm_cap.h"
#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_timer.h"
#include "esp_private/esp_clk.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"

#include "platform/helpers.h"

#include "platform/pwm.h"

#define NUM_PWM_IN_CHANNELS 8
#define NUM_PWM_OUT_CHANNELS 8

typedef struct PWMInChannel {
    u32 pin;
    u32 tRise, tFall; // rise/falling edge timestamps (in ticks)
    u32 tPulsewidth;  // pulsewidth (in ticks)
    bool active;      // whether or not channel is in use; internal
} PWMInChannel;

typedef struct PWMOutChannel {
    u32 pin;
    mcpwm_cmpr_handle_t out; // mcpwm_cmpr_handle_t for the channel
    u32 period;              // period (in μs)
    bool active;
} PWMOutChannel;

static PWMInChannel inChannels[NUM_PWM_IN_CHANNELS];
static size_t numCaptureChannels = 0; // Number of MCPWM capture channels in use
static int currentCaptureTimer = 0;   // Current capture timer being used to create channels

static PWMOutChannel outChannels[NUM_PWM_OUT_CHANNELS];
static size_t numOperators = 0; // Number of MCPWM operators in use
static int currentTimer = 0;    // Current timer being used to create operators

/**
 * @return a pointer to the first available PWM IN channel, or NULL if none are available
 */
static PWMInChannel *get_available_in_channel() {
    for (u32 i = 0; i < count_of(inChannels); i++) {
        if (!inChannels[i].active) {
            inChannels[i].active = true;
            return &inChannels[i];
        }
    }
    return NULL;
}

/**
 * @return a pointer to the first available PWM OUT channel, or NULL if none are available
 */
static PWMOutChannel *get_available_out_channel() {
    for (u32 i = 0; i < count_of(outChannels); i++) {
        if (!outChannels[i].active) {
            outChannels[i].active = true;
            return &outChannels[i];
        }
    }
    return NULL;
}

/**
 * @return a pointer to the PWM IN channel representing the given pin, or NULL if none match
 */
static PWMInChannel *get_in_channel(u32 pin) {
    for (u32 i = 0; i < count_of(inChannels); i++) {
        if (inChannels[i].pin == pin)
            return &inChannels[i];
    }
    return NULL;
}

/**
 * @return a pointer to the PWM OUT channel representing the given pin, or NULL if none match
 */
static PWMOutChannel *get_out_channel(u32 pin) {
    for (u32 i = 0; i < count_of(outChannels); i++) {
        if (outChannels[i].pin == pin)
            return &outChannels[i];
    }
    return NULL;
}

static bool pwm_read_callback(mcpwm_cap_channel_handle_t cap_channel, const mcpwm_capture_event_data_t *event, void *data) {
    PWMInChannel *channel = (PWMInChannel *)data;
    if (event->cap_edge == MCPWM_CAP_EDGE_POS) {
        // Rising edge
        channel->tRise = event->cap_value;
        channel->tFall = channel->tRise;
    } else {
        // Falling edge
        channel->tFall = event->cap_value;
        channel->tPulsewidth = channel->tFall - channel->tRise;
    }
    // Return value signifies whether another FreeRTOS task has been woken up, which can never occur here
    return false;
    (void)cap_channel; // Unused
}

bool pwm_setup_read(const u32 pins[], u32 num_pins) {
    // Static variable; this means that the timer can be reused for multiple channels
    // Once it has been maxed out with channels, a new timer will be created
    static mcpwm_cap_timer_handle_t timer;
    mcpwm_capture_timer_config_t config = {
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
        .group_id = currentCaptureTimer,
    };

    for (u32 i = 0; i < num_pins; i++) {
        // Create a new capture timer if necessary
        bool newTimerNeeded = numCaptureChannels % SOC_MCPWM_CAPTURE_CHANNELS_PER_TIMER == 0;
        if (newTimerNeeded) {
            currentCaptureTimer = numCaptureChannels / SOC_MCPWM_CAPTURE_CHANNELS_PER_TIMER;
            if (currentCaptureTimer >= SOC_MCPWM_TIMERS_PER_GROUP)
                return false;
            config.group_id = currentCaptureTimer;
            if (mcpwm_new_capture_timer(&config, &timer) != ESP_OK)
                return false;
        }

        // Get an available channel in the array, this will allow us to interact with the channel later
        PWMInChannel *channel = get_available_in_channel();
        if (!channel)
            return false;
        channel->pin = pins[i];
        // Create a new capture channel and register a callback for it
        mcpwm_cap_channel_handle_t capture;
        mcpwm_capture_channel_config_t captureConfig = {
            .gpio_num = pins[i],
            .prescale = 1,
            // Capture on both edges (so we can obtain pulsewidth)
            .flags.neg_edge = true,
            .flags.pos_edge = true,
        };
        if (mcpwm_new_capture_channel(timer, &captureConfig, &capture) != ESP_OK)
            return false;
        numCaptureChannels++;

        mcpwm_capture_event_callbacks_t callbacks = {
            .on_cap = pwm_read_callback,
        };
        if (mcpwm_capture_channel_register_event_callbacks(capture, &callbacks, channel) != ESP_OK)
            return false;
        // Enable the capture channel and start the timer if necessary
        if (mcpwm_capture_channel_enable(capture) != ESP_OK)
            return false;
        if (newTimerNeeded) {
            if (mcpwm_capture_timer_enable(timer) != ESP_OK)
                return false;
            if (mcpwm_capture_timer_start(timer) != ESP_OK)
                return false;
        }
    }
    return true;
}

bool pwm_setup_write(const u32 pins[], u32 num_pins, u32 freq) {
    const u32 period = 1000000 / freq; // Period in μs
    static mcpwm_timer_handle_t timer;
    mcpwm_timer_config_t config = {
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000, // 1MHz, esp does not like going lower
        .period_ticks = period,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        .group_id = currentTimer,
    };

    for (u32 i = 0; i < num_pins; i++) {
        bool newTimerNeeded = numOperators % SOC_MCPWM_OPERATORS_PER_GROUP == 0;
        if (newTimerNeeded) {
            currentTimer = numOperators / SOC_MCPWM_OPERATORS_PER_GROUP;
            if (currentTimer >= SOC_MCPWM_TIMERS_PER_GROUP)
                return false;
            config.group_id = currentTimer;
            if (mcpwm_new_timer(&config, &timer) != ESP_OK)
                return false;
        }

        PWMOutChannel *channel = get_available_out_channel();
        if (!channel)
            return false;
        channel->pin = pins[i];
        channel->period = period;
        // Create a new operator, comparator, and generator for the channel
        mcpwm_oper_handle_t operator;
        const mcpwm_operator_config_t operatorConfig = {
            .group_id = config.group_id,
        };
        if (mcpwm_new_operator(&operatorConfig, &operator) != ESP_OK)
            return false;
        numOperators++;
        if (mcpwm_operator_connect_timer(operator, timer) != ESP_OK)
            return false;

        mcpwm_cmpr_handle_t comparator;
        const mcpwm_comparator_config_t comparatorConfig = {
            .flags.update_cmp_on_tez = true,
        };
        if (mcpwm_new_comparator(operator, & comparatorConfig, &comparator) != ESP_OK)
            return false;
        channel->out = comparator;

        mcpwm_gen_handle_t generator;
        const mcpwm_generator_config_t generatorConfig = {
            .gen_gpio_num = pins[i],
        };
        if (mcpwm_new_generator(operator, & generatorConfig, &generator) != ESP_OK)
            return false;
        // Tell the generator what actions to take on timer and compare events (this will produce the correct PWM waveform)
        mcpwm_generator_set_action_on_timer_event(
            generator, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
        mcpwm_generator_set_action_on_compare_event(
            generator, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW));
        // If we created a new timer, enable and start it
        if (newTimerNeeded) {
            if (mcpwm_timer_enable(timer) != ESP_OK)
                return false;
            if (mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP) != ESP_OK)
                return false;
        }
    }
    return true;
}

f32 pwm_read_raw(u32 pin) {
    PWMInChannel *channel = get_in_channel(pin);
    if (!channel)
        return -1.f;
    // Convert the pulsewidth from ticks to μs
    return channel->tPulsewidth * (1E6f / esp_clk_apb_freq());
}

void pwm_write_raw(u32 pin, f32 pulsewidth) {
    PWMOutChannel *channel = get_out_channel(pin);
    if (!channel)
        return;
    // `cmp_ticks` corresponds to the pulsewidth (in μs), so we can just write the pulsewidth directly
    mcpwm_comparator_set_compare_value(channel->out, (u32)pulsewidth);
}
