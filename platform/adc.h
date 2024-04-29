#pragma once

#include "platform/defs.h"

#if PLATFORM_SUPPORTS_ADC

// clang-format off

#include "platform/types.h"

// clang-format on

/**
 * Sets up `pins[]` for reading ADC signals.
 * @param pins array of pins to setup for reading ADC signals
 * @param num_pins number of pins in `pins[]`
 */
void adc_setup(const u32 pins[], u32 num_pins);

/**
 * Reads the ADC signal on `pin`.
 * @param pin pin to read ADC signal from
 * @return the voltage of the ADC signal in volts, or -1 if the pin is invalid
 */
f64 adc_read_raw(u32 pin);

#endif // PLATFORM_SUPPORTS_ADC
