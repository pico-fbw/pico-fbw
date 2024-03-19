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

#include "platform/pwm.h"

#define NUM_PWM_CHANNELS 8

typedef struct PWMChannel {
    u32 pin;
    // PWM input: rise/falling edge timestamps and pulsewidth (in ticks)
    u32 tRise, tFall;
    u32 tPulsewidth;
    // PWM output: mcpwm_cmpr_handle_t for the channel and the period (in μs)
    mcpwm_cmpr_handle_t out;
    u32 period;
    // Internal
    bool active;
} PWMChannel;

static PWMChannel channels[NUM_PWM_CHANNELS];

static size_t numMCPWMCaptureChannels = 0; // Number of MCPWM capture channels in use
static size_t numMCPWMOperators = 0;       // Number of MCPWM operators in use

/**
 * @return a pointer to the first available PWM channel, or NULL if none are available
 */
static PWMChannel *get_available_channel() {
    for (size_t i = 0; i < count_of(channels); i++) {
        if (!channels[i].active) {
            channels[i].active = true;
            return &channels[i];
        }
    }
    return NULL;
}

/**
 * @return a pointer to the PWM channel for the given pin, or NULL if none match
 */
static PWMChannel *get_channel(u32 pin) {
    for (size_t i = 0; i < count_of(channels); i++) {
        if (channels[i].pin == pin)
            return &channels[i];
    }
    return NULL;
}

static bool pwm_read_callback(mcpwm_cap_channel_handle_t cap_channel, const mcpwm_capture_event_data_t *event, void *data) {
    PWMChannel *channel = (PWMChannel *)data;
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

bool pwm_setup_read(u32 pins[], u32 num_pins) {
    // Static variable; this means that the timer can be reused for multiple channels
    static mcpwm_cap_timer_handle_t timer;
    mcpwm_capture_timer_config_t config = {
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
    };

    for (u32 i = 0; i < num_pins; i++) {
        // Create a new timer group if necessary
        bool newTimerNeeded = numMCPWMCaptureChannels % SOC_MCPWM_CAPTURE_CHANNELS_PER_TIMER == 0;
        if (newTimerNeeded) {
            config.group_id = numMCPWMCaptureChannels / SOC_MCPWM_CAPTURE_CHANNELS_PER_TIMER;
            if (config.group_id >= SOC_MCPWM_TIMERS_PER_GROUP)
                return false;
            if (mcpwm_new_capture_timer(&config, &timer) != ESP_OK)
                return false;
        }
        // Get an available channel in the array, this will allow us to interact with the channel later
        PWMChannel *channel = get_available_channel();
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
        numMCPWMCaptureChannels++;

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

bool pwm_setup_write(u32 pins[], u32 num_pins, u32 freq) {
    static mcpwm_timer_handle_t timer;
    u32 period = 1000000 / freq; // Period in μs
    mcpwm_timer_config_t config = {
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000, // 1MHz, esp does not like going lower
        .period_ticks = period,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };

    for (u32 i = 0; i < num_pins; i++) {
        bool newTimerNeeded = numMCPWMOperators % SOC_MCPWM_OPERATORS_PER_GROUP == 0;
        if (newTimerNeeded) {
            config.group_id = numMCPWMOperators / SOC_MCPWM_OPERATORS_PER_GROUP;
            if (config.group_id >= SOC_MCPWM_TIMERS_PER_GROUP)
                return false;
            if (mcpwm_new_timer(&config, &timer) != ESP_OK)
                return false;
        }

        PWMChannel *channel = get_available_channel();
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
        numMCPWMOperators++;
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

float pwm_read_raw(u32 pin) {
    PWMChannel *channel = get_channel(pin);
    if (!channel)
        return -1.f;
    // Convert the pulsewidth from ticks to μs
    return channel->tPulsewidth * (1E6f / esp_clk_apb_freq());
}

void pwm_write_raw(u32 pin, float pulsewidth) {
    PWMChannel *channel = get_channel(pin);
    if (!channel)
        return;
    // `cmp_ticks` corresponds to the pulsewidth (in μs), so we can just write the pulsewidth directly
    mcpwm_comparator_set_compare_value(channel->out, (u32)pulsewidth);
}
