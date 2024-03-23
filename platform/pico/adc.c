/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "hardware/adc.h"

#include "platform/adc.h"

#define ADC_GPIO_BASE 26 // GPIO26 is ADC0, 27-29 are ADC1-3 respectively
#define PIN_TO_ADC_CHANNEL(pin) (pin - ADC_GPIO_BASE)
#define ADC_CONVERT_FACTOR (3.3 / (1 << 12)) // Assuming 3.3v reference voltage and 12-bit resolution

static bool adcActive = false;

void adc_setup(const u32 pins[], u32 num_pins) {
    if (!adcActive) {
        adc_init();
        adcActive = true;
    }
    for (u32 i = 0; i < num_pins; i++)
        adc_gpio_init(pins[i]);
}

double adc_read_raw(u32 pin) {
    adc_select_input(PIN_TO_ADC_CHANNEL(pin));
    return (double)adc_read() * ADC_CONVERT_FACTOR;
}
