/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/adc.h"

#if PLATFORM_SUPPORTS_ADC

// clang-format off

#include "pico/config.h"
#include "hardware/adc.h"

#include "platform/gpio.h"

// The Pico includes an RT6150 buck-boost SMPS which includes a PS pin to switch between PFM and PWM modes
// PFM mode (default) is very efficient but can exhibit quite a bit of output ripple/noise
// See page 19 of the Pico datasheet (https://datasheets.raspberrypi.com/pico/pico-datasheet.pdf) for more info
#ifdef RASPBERRYPI_PICO_W
    #define PIN_SMPS_MODE 1 // WL_GPIO1 on CYW43
#else
    #define PIN_SMPS_MODE PICO_SMPS_MODE_PIN
#endif
#define SMPS_MODE_PFM STATE_LOW
#define SMPS_MODE_PWM STATE_HIGH

#define ADC_GPIO_BASE 26 // GPIO26 is ADC0, 27-29 are ADC1-3 respectively
#define PIN_TO_ADC_CHANNEL(pin) (pin - ADC_GPIO_BASE)
#define ADC_CONVERT_FACTOR (3.3 / (1 << 12)) // Assuming 3.3v reference voltage and 12-bit resolution

// clang-format on

void adc_setup(const u32 pins[], u32 num_pins) {
    adc_init();
    for (u32 i = 0; i < num_pins; i++)
        adc_gpio_init(pins[i]);
}

f64 adc_read_raw(u32 pin) {
    // Force the SMPS into PWM mode while reading the ADC to reduce noise
    gpio_set(PIN_SMPS_MODE, SMPS_MODE_PWM);
    adc_select_input(PIN_TO_ADC_CHANNEL(pin));
    f64 reading = (f64)adc_read() * ADC_CONVERT_FACTOR;
    gpio_set(PIN_SMPS_MODE, SMPS_MODE_PFM);
    return reading;
}

#endif // PLATFORM_SUPPORTS_ADC
