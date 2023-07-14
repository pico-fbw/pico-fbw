#ifndef __LED_H
#define __LED_H

/**
 * Initialize the onboard LED (in powered state).
*/
void led_init();

/**
 * Starts an LED blink cycle (which will interrupt the processor) at the specified frequency in ms.
 * @param freq_ms The frequency of the blink cycle in ms.
 * @param pulse_ms The duration of the pulse in ms. Set to 0 for no pulse (LED is either on or off).
*/
void led_blink(uint32_t freq_ms, uint32_t pulse_ms);

/**
 * Stop the LED blink/pulse cycle if it is currently running.
*/
void led_stop();

#endif // __LED_H
