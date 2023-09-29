/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "pico/config.h"
#include "pico/time.h"
#include "pico/types.h"

#include "platform.h"

#include "../sys/config.h"

#include "error.h"

#if defined(RASPBERRYPI_PICO)
    #include "hardware/gpio.h"
    #define LED_PIN PICO_DEFAULT_LED_PIN
#elif defined(RASPBERRYPI_PICO_W)
    #include "pico/cyw43_arch.h"
    #define LED_PIN CYW43_WL_GPIO_LED_PIN
    char buf[1];
#else
    #undef LED_PIN
#endif

#define ERROR_MSG_WARN "WARNING"
#define ERROR_MSG_ERR "ERROR"
#define ERROR_MSG_FATAL "FATAL"

static bool errs[6][6] = {
    {true, true, true, true, true, true},
    {false, false, false, false, false, false}
};
static uint currentError = 9999;

static struct repeating_timer timer;
static uint32_t gb_pulse_ms = 0;

// TODO: this randomly stops after a bit sometimes?
// also Pico (w)'s LED is just randomly going out as well; the timer is stopping?

static inline void led_toggle() {
    if (platform_is_fbw()) {
        for (uint i = 0; i < 6; i++) {
            errs[0][i] = errs[1][i] ? !errs[0][i] : errs[0][i];
        }
        marbe_s((uint8_t)errs[0][0], (uint8_t)errs[0][1], (uint8_t)errs[0][2], (uint8_t)errs[0][3], (uint8_t)errs[0][4], (uint8_t)errs[0][5]);
    } else {
        #ifdef LED_PIN
            #if defined(RASPBERRYPI_PICO)
                gpio_xor_mask(1u << LED_PIN);
            #elif defined(RASPBERRYPI_PICO_W)
                cyw43_arch_gpio_put(LED_PIN, !cyw43_arch_gpio_get(LED_PIN));
                snprintf(buf, sizeof(buf), 0); // LED acts weird without this
            #endif
        #endif // LED_PIN
    }
}

static inline int64_t led_pulse_callback(alarm_id_t id, void *data) {
    led_toggle();
    return 0;
}

static inline bool led_callback(struct repeating_timer *t) {
    // Toggle LED immediately, then schedule a pulse toggle if applicable (non-zero)
    led_toggle();
    if (gb_pulse_ms != 0) {
        add_alarm_in_ms(gb_pulse_ms, led_pulse_callback, NULL, false);
    }
    return true;
}

void led_init() {
    #ifdef LED_PIN
        #if defined(RASPBERRYPI_PICO)
            gpio_init(LED_PIN);
            gpio_set_dir(LED_PIN, GPIO_OUT);
            gpio_put(LED_PIN, 1);
        #elif defined(RASPBERRYPI_PICO_W)
            cyw43_arch_gpio_put(LED_PIN, 1);
        #endif
    #endif // LED_PIN
}

void error_throw(ErrorType type, ErrorLevel level, uint code, uint pulse_ms, bool force, const char *msg) {
    if (platform_is_fbw() && level != ERROR_LEVEL_NONE) {
        switch (type) {
            case ERROR_PWM:
                errs[1][0] = (level == ERROR_LEVEL_STATUS || level == ERROR_LEVEL_WARN) ? true : (errs[0][0] = false);
                break;
            case ERROR_IMU:
                errs[1][1] = (level == ERROR_LEVEL_STATUS || level == ERROR_LEVEL_WARN) ? true : (errs[0][1] = false);
                break;
            case ERROR_GPS:
                errs[1][2] = (level == ERROR_LEVEL_STATUS || level == ERROR_LEVEL_WARN) ? true : (errs[0][2] = false);
                break;
            case ERROR_PID:
                errs[1][3] = (level == ERROR_LEVEL_STATUS || level == ERROR_LEVEL_WARN) ? true : (errs[0][3] = false);
                break;
            case ERROR_WIFLY:
                errs[1][4] = (level == ERROR_LEVEL_STATUS || level == ERROR_LEVEL_WARN) ? true : (errs[0][4] = false);
                break;
            case ERROR_GENERAL:
                errs[1][5] = (level == ERROR_LEVEL_STATUS || level == ERROR_LEVEL_WARN) ? true : (errs[0][5] = false);
                break;
        }
        cancel_repeating_timer(&timer);
        add_repeating_timer_ms(500, led_callback, NULL, &timer);
    } else {
        // Display the error blink pattern on the LED
        if (code < currentError || force) {
            cancel_repeating_timer(&timer);
            add_repeating_timer_ms((uint32_t)code, led_callback, NULL, &timer);
            currentError = code;
        }
    }
    // If pulse has been enabled, turn the LED off so it pulses to the on state, not off (looks better)
    if (pulse_ms != 0) {
        #ifdef LED_PIN
            #if defined(RASPBERRYPI_PICO)
                gpio_put(LED_PIN, 0);
            #elif defined(RASPBERRYPI_PICO_W)
                cyw43_arch_gpio_put(LED_PIN, 0);
            #endif
        #endif // LED_PIN
    }
    gb_pulse_ms = pulse_ms;

    // Format an error string to be printed
    const char *levelMsg = NULL;
    switch (level) {
        case ERROR_LEVEL_WARN:
            levelMsg = ERROR_MSG_WARN;
            break;
        case ERROR_LEVEL_ERR:
            levelMsg = ERROR_MSG_ERR;
            break;
        case ERROR_LEVEL_FATAL:
            levelMsg = ERROR_MSG_FATAL;
            break;
    }
    if (levelMsg) {
        printf("%s: (FBW-%d) %s\n", levelMsg, code, msg);
    }

    if (level == ERROR_LEVEL_FATAL) {
        while (true); // Halt execution for fatal errors
    }
}

void error_clear(ErrorType type, bool all) {
    if (platform_is_fbw()) {
        switch (type) {
            case ERROR_PWM:
                errs[0][0] = true;
                errs[1][0] = false;
                if (!all) break;
            case ERROR_IMU:
                errs[0][1] = true;
                errs[1][1] = false;
                if (!all) break;
            case ERROR_GPS:
                errs[0][2] = true;
                errs[1][2] = false;
                if (!all) break;
            case ERROR_PID:
                errs[0][3] = true;
                errs[1][3] = false;
                if (!all) break;
            case ERROR_WIFLY:
                errs[0][4] = true;
                errs[1][4] = false;
                if (!all) break;
            case ERROR_GENERAL:
                errs[0][5] = true;
                errs[1][5] = false;
                break;
        }
        led_toggle();
    } else {
        #ifdef LED_PIN
            #if defined(RASPBERRYPI_PICO)
                gpio_put(LED_PIN, 1);
            #elif defined(RASPBERRYPI_PICO_W)
                cyw43_arch_gpio_put(LED_PIN, 1);
            #endif
        #endif // LED_PIN
        currentError = 9999;
    }
    gb_pulse_ms = 0;
    cancel_repeating_timer(&timer);
}
