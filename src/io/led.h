#ifndef led_h
#define led_h

/**
 * Initialize the onboard LED (in powered state).
*/
void led_init();

/**
 * Start the LED blink cycle (which will interrupt the processor) at the specified frequency in ms.
*/
void led_blink(uint32_t freq_ms);

/**
 * Stop the LED blink cycle if it is currently running.
*/
void led_blink_stop();

#endif // led_h