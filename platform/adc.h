#pragma once

#include "platform/int.h"

/**
 * Sets up `pins[]` for reading ADC signals.
 * @param pins array of pins to setup for reading ADC signals
 * @param num_pins number of pins in `pins[]`
 */
void adc_setup(u32 pins[], u32 num_pins);

/**
 * Reads the ADC signal on `pin`.
 * @param pin pin to read ADC signal from
 * @return the voltage of the ADC signal in volts, or -1 if the pin is invalid
 */
double adc_read_raw(u32 pin);
