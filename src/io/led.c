/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdio.h>
#include "pico/time.h"

#include "led.h"

#if defined(RASPBERRYPI_PICO)
    #include "hardware/gpio.h"
    #define LED_PIN PICO_DEFAULT_LED_PIN
#elif defined(RASPBERRYPI_PICO_W)
    #include "pico/cyw43_arch.h"
    #define LED_PIN CYW43_WL_GPIO_LED_PIN
    char buf[1];
#else
    #warning Neither a Pico or Pico W build target were found, LED functionality is disabled.
    #undef LED_PIN
#endif

#ifdef LED_PIN
    static struct repeating_timer timer;
#endif

static inline bool led_callback(struct repeating_timer *t) {
    #ifdef LED_PIN
        #if defined(RASPBERRYPI_PICO)
            gpio_xor_mask(1u << LED_PIN);
        #elif defined(RASPBERRYPI_PICO_W)
            cyw43_arch_gpio_put(LED_PIN, !cyw43_arch_gpio_get(LED_PIN));
            snprintf(buf, sizeof(buf), " "); // There's a bug in the cyw43 arch where the LED just acts sporadically, this fixes it
        #endif
    #endif
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
    #endif
}

void led_blink(uint32_t freq_ms) {
    #ifdef LED_PIN
        led_blink_stop();
        add_repeating_timer_ms(freq_ms, led_callback, NULL, &timer);
    #endif
}

void led_blink_stop() {
    #ifdef LED_PIN
        cancel_repeating_timer(&timer);
        #if defined(RASPBERRYPI_PICO)
            gpio_put(LED_PIN, 1);
        #elif defined(RASPBERRYPI_PICO_W)
            cyw43_arch_gpio_put(LED_PIN, 1);
        #endif
    #endif
}
