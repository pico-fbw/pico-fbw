/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "pico/time.h"

#include "platform.h"

#include "../config.h"

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

static inline bool led_callback(struct repeating_timer *t) {
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
                snprintf(buf, sizeof(buf), '\0'); // There's a bug in the cyw43 arch where the LED just acts sporadically, this fixes it
            #endif
        #endif // LED_PIN
    }
    return true;
}

void error_throw(ErrorType type, ErrorLevel level, uint code, const char *msg) {
    if (platform_is_fbw()) {
        switch (type) {
            case ERROR_PWM:
                errs[1][0] = (level == ERROR_LEVEL_WARN) ? true : (errs[0][0] = false);
                break;
            case ERROR_IMU:
                errs[1][1] = (level == ERROR_LEVEL_WARN) ? true : (errs[0][1] = false);
                break;
            case ERROR_GPS:
                errs[1][2] = (level == ERROR_LEVEL_WARN) ? true : (errs[0][2] = false);
                break;
            case ERROR_PID:
                errs[1][3] = (level == ERROR_LEVEL_WARN) ? true : (errs[0][3] = false);
                break;
            case ERROR_WIFLY:
                errs[1][4] = (level == ERROR_LEVEL_WARN) ? true : (errs[0][4] = false);
                break;
            case ERROR_GENERAL:
                errs[1][5] = (level == ERROR_LEVEL_WARN) ? true : (errs[0][5] = false);
                break;
        }
        cancel_repeating_timer(&timer);
        add_repeating_timer_ms(500, led_callback, NULL, &timer);
    } else {
        // Display the error on the LED
        if (code < currentError) {
            cancel_repeating_timer(&timer);
            add_repeating_timer_ms((uint32_t)code, led_callback, NULL, &timer);
            currentError = code;
        }
    }

    // Format an error string to be printed
    const char *levelMsg;
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
    FBW_DEBUG_printf("%s: (FBW-%d) %s\n", levelMsg, code, msg);

    if (level == ERROR_LEVEL_FATAL) {
        while (true); // Halt execution for fatal errors
    }
}
