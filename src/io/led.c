/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
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

static struct repeating_timer timer;
static struct repeating_timer pulse_timer;
static uint32_t gb_pulse_ms;

/**
 * Toggle's the LED's state (on -> off / off -> on).
*/
static inline void led_toggle() {
    #ifdef LED_PIN
        #if defined(RASPBERRYPI_PICO)
            gpio_xor_mask(1u << LED_PIN);
        #elif defined(RASPBERRYPI_PICO_W)
            cyw43_arch_gpio_put(LED_PIN, !cyw43_arch_gpio_get(LED_PIN));
            snprintf(buf, sizeof(buf), '\0'); // There's a bug in the cyw43 arch where the LED just acts sporadically, this fixes it
        #endif
    #endif
}

static inline int64_t led_callback_alarm(alarm_id_t id, void *data) {
    led_toggle();
    return 0;
}

static inline bool led_callback(struct repeating_timer *t) {
    // Toggle LED immediately, then schedule a toggle after the pulse duration if applicable/non-zero
    led_toggle();
    add_alarm_in_ms(gb_pulse_ms, led_callback_alarm, NULL, false);
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
    #endif
}

void led_blink(uint32_t freq_ms, uint32_t pulse_ms) {
    #ifdef LED_PIN
        led_stop();
        // If we are going to be pulsing then turn the LED off so it's off for most of the cycle (looks weird otherwise)
        if (pulse_ms != 0) {
            #if defined(RASPBERRYPI_PICO)
                gpio_put(LED_PIN, 0);
            #elif defined(RASPBERRYPI_PICO_W)
                cyw43_arch_gpio_put(LED_PIN, 0);
            #endif
        }
        gb_pulse_ms = pulse_ms;
        add_repeating_timer_ms(freq_ms, led_callback, NULL, &timer);
    #endif
}

void led_stop() {
    #ifdef LED_PIN
        cancel_repeating_timer(&timer);
        cancel_repeating_timer(&pulse_timer);
        #if defined(RASPBERRYPI_PICO)
            gpio_put(LED_PIN, 1);
        #elif defined(RASPBERRYPI_PICO_W)
            cyw43_arch_gpio_put(LED_PIN, 1);
        #endif
    #endif
}
