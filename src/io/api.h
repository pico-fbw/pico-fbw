#ifndef __API_H
#define __API_H

#include "../config.h"

#ifdef API_ENABLED

// TODO: api documentation on wiki
// ensure to add a note in docs about how SET_MODE command can succeed but be booted out of the mode later

#ifdef LIB_PICO_STDIO_UART
    // UART works better if there is a small character delay
    #define API_CHAR_TIMEOUT_US 100
#else
    // USB doesn't seem to need this
    #define API_CHAR_TIMEOUT_US 0
#endif

#define ENABLE_API_TIMEOUT_MS 3000 // Allow API calls this amount of time in ms after power on (prevents API calls during bootup)

/**
 * Polls the API for new data (incoming commands) and responds if necessary.
*/
void api_poll();

#endif // API_ENABLED

#endif // __API_H
