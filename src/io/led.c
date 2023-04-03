#include "pico/stdlib.h"

#include "led.h"

struct repeating_timer timer;

bool led_callback(struct repeating_timer *t) {
    gpio_xor_mask(1u << PICO_DEFAULT_LED_PIN);
}

void led_init() {
    #ifndef PICO_DEFAULT_LED_PIN
      	#warning No default LED pin found. Power LED functionality may be impacted.
    #endif
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
}

void led_blink(uint32_t freq_ms) {
    led_blink_stop();
    add_repeating_timer_ms(freq_ms, led_callback, NULL, &timer);
}

void led_blink_stop() {
    cancel_repeating_timer(&timer);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
}